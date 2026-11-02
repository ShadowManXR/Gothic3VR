#pragma once
#include "D3D9Bridge.h"
#include "StereoOpenXrApi.h"
#include "VirtualHook.h"
#include "EyeDrawTiming.h"
#include "D3D9TemporalProbe.h"
#include "RuntimeProbePolicy.h"
#include <atomic>
#include <functional>
#ifdef G3VR_STEREO_FRAME_CAPTURE
#include "D3D9WorldWriteTrace.h"
#endif
#ifdef G3VR_UI_COMPOSITION_CAPTURE
#include "D3D9UiAlphaTrace.h"
#endif

namespace g3vr
{
// Replay only GPU draw commands in the native UI phase. The game UI, input,
// animations and callbacks execute once, and the desktop draw is unchanged.
class UiCapture
{
    using Draw=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,UINT);
    using Indexed=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,INT,UINT,UINT,UINT,UINT);
    using Up=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,void const*,UINT);
    using IndexedUp=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,UINT,UINT,void const*,D3DFORMAT,void const*,UINT);
    inline static Draw draw=nullptr;
    inline static Indexed indexed=nullptr;
    inline static Up up=nullptr;
    inline static IndexedUp indexedUp=nullptr;
    inline static std::atomic<UiCapture*> owner{nullptr};
    inline static thread_local UiCapture* capturing=nullptr;
    inline static thread_local UiCapture* measuring=nullptr;
    inline static thread_local bool observingWorld=false;
    diagnostic::performance::EyeDrawTiming drawTiming;
    diagnostic::VirtualHook hooks[4];
    stereo::ComPtr<IDirect3DDevice9Ex> device;
    stereo::ComPtr<IDirect3DSurface9> backbuffer;
    DWORD thread=0;
    UINT width=0,height=0;
    bool active=false;unsigned frameCommands=0;
    struct SavedState
    {
        IDirect3DDevice9* device;
        stereo::ComPtr<IDirect3DSurface9> target,depth;
        D3DVIEWPORT9 viewport{};RECT scissor{};
        static constexpr D3DRENDERSTATETYPE types[]={D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_COLORWRITEENABLE,
            D3DRS_SEPARATEALPHABLENDENABLE,D3DRS_SRCBLENDALPHA,D3DRS_DESTBLENDALPHA,D3DRS_BLENDOPALPHA};
        DWORD states[7]{};
        explicit SavedState(IDirect3DDevice9* d):device(d)
        {
            stereo::Hr(d->GetRenderTarget(0,&target),"UI original target");
            HRESULT hr=d->GetDepthStencilSurface(&depth);if(hr!=D3DERR_NOTFOUND)stereo::Hr(hr,"UI original depth");
            stereo::Hr(d->GetViewport(&viewport),"UI original viewport");stereo::Hr(d->GetScissorRect(&scissor),"UI original scissor");
            for(unsigned i=0;i<7;++i)stereo::Hr(d->GetRenderState(types[i],&states[i]),"UI original render state");
        }
        void Restore() noexcept
        {
            device->SetRenderTarget(0,target.Get());device->SetDepthStencilSurface(depth.Get());
            device->SetViewport(&viewport);device->SetScissorRect(&scissor);
            for(unsigned i=0;i<7;++i)device->SetRenderState(types[i],states[i]);
        }
    };
    bool DefaultCanvas(SavedState const& saved,bool simpleQuad){
        // Exact stock GUI base canvas observed before HUD and every menu:
        // one fixed-function XYZ/diffuse/UV quad, on the complete viewport.
        // Its atlas carries a faint backdrop over the entire desktop. Replaying
        // it on a floating XR panel creates a rectangular color boundary.
        // Later sprites (including the same atlas) and menu artwork stay native.
        if(!simpleQuad||saved.states[2]!=15||saved.states[3]||saved.viewport.X||saved.viewport.Y||saved.viewport.Width!=width||saved.viewport.Height!=height)return false;
        DWORD fvf=0,alpha=0,src=0,dst=0,test=0;device->GetFVF(&fvf);
        device->GetRenderState(D3DRS_ALPHABLENDENABLE,&alpha);device->GetRenderState(D3DRS_SRCBLEND,&src);device->GetRenderState(D3DRS_DESTBLEND,&dst);device->GetRenderState(D3DRS_ALPHATESTENABLE,&test);
        if(fvf!=(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)||!alpha||!test||src!=D3DBLEND_SRCALPHA||dst!=D3DBLEND_INVSRCALPHA)return false;
        stereo::ComPtr<IDirect3DVertexShader9> vs;stereo::ComPtr<IDirect3DPixelShader9> ps;device->GetVertexShader(&vs);device->GetPixelShader(&ps);if(vs||ps)return false;
        stereo::ComPtr<IDirect3DBaseTexture9> base;device->GetTexture(0,&base);if(!base||base->GetType()!=D3DRTYPE_TEXTURE)return false;
        D3DSURFACE_DESC desc{};return SUCCEEDED(static_cast<IDirect3DTexture9*>(base.Get())->GetLevelDesc(0,&desc))&&desc.Width==2048&&desc.Height==2048;
    }
    bool Owns(IDirect3DDevice9* d) const {return active&&d==device.Get()&&thread==GetCurrentThreadId();}
    template<class Command> static HRESULT TimeDraw(IDirect3DDevice9* d,unsigned kind,Command const& command,diagnostic::temporal::Geometry const* geometry=nullptr){
        if constexpr(!diagnostic::probes::Enabled&&!diagnostic::probes::StaticTrial)return command();
        // An opt-in evidence callback may replay one draw into its private RT.
        // Never recursively observe/profile that diagnostic replay as gameplay.
        if(observingWorld)return command();
        auto* scope=measuring;if(!scope||scope->device.Get()!=d||D3D9SkinnedMotion::Replaying())return command();
#ifdef G3VR_NATIVE_MOTION
        if(D3D9NativeMotion::Replaying())return command();
#endif
#ifdef G3VR_NATIVE_SKY
        if(D3D9SkyMotion::Replaying())return command();
#endif
        bool replaced=false;
        if(scope->worldObserver){
            struct Guard {Guard(){observingWorld=true;}~Guard(){observingWorld=false;}} guard;
            replaced=scope->worldObserver(scope->worldObserverContext,d,kind,geometry);
        }
        if(replaced&&kind==1&&geometry)return D3D_OK;
        if constexpr(!diagnostic::probes::Enabled)return command();
        auto observeStarted=scope->temporal.Active()?diagnostic::performance::Now():0;
        if(observeStarted){scope->temporal.Observe(d);if(geometry){if(kind==1)scope->temporal.ObserveMotion(d,*geometry);else scope->temporal.ObserveAuxiliaryDraw(d,*geometry,kind);}}
        auto drawStarted=diagnostic::performance::Now();if(observeStarted)scope->temporal.AddObservationTicks(drawStarted-observeStarted);
        auto stamp=scope->drawTiming.BeginCall(kind,drawStarted);
        HRESULT result=command();scope->drawTiming.EndCall(stamp,diagnostic::performance::Now());
        if(SUCCEEDED(result)&&kind==1&&geometry&&scope->temporal.Active())scope->temporal.AfterDraw(d,*geometry);
        return result;
    }
    template<class Command> void Replay(IDirect3DDevice9* d,Command const& command,bool simpleQuad) noexcept
    {
        try
        {
            SavedState saved(d);
            // UI preview meshes have their own render targets. Capture their
            // final on-screen sprite, never their intermediate render passes.
            if(saved.target.Get()!=backbuffer.Get())return;
            if(frameCommands++==0&&DefaultCanvas(saved,simpleQuad))return;
            Scope restore{[&]{saved.Restore();}};
            stereo::Hr(d->SetDepthStencilSurface(nullptr),"UI depth unbind");
            stereo::Hr(d->SetRenderTarget(0,shared.surface9.Get()),"UI capture target");
            stereo::Hr(d->SetViewport(&saved.viewport),"UI viewport");stereo::Hr(d->SetScissorRect(&saved.scissor),"UI scissor");
            stereo::Hr(d->SetRenderState(D3DRS_ZENABLE,FALSE),"UI disable depth");
            stereo::Hr(d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE),"UI disable depth write");
            stereo::Hr(d->SetRenderState(D3DRS_COLORWRITEENABLE,15),"UI RGBA write");
            stereo::Hr(d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,TRUE),"UI separate alpha");
            stereo::Hr(d->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ONE),"UI source alpha");
            stereo::Hr(d->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_INVSRCALPHA),"UI destination alpha");
            stereo::Hr(d->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD),"UI alpha blend op");
            stereo::Hr(command(),"UI GPU replay");++draws;
#ifdef G3VR_UI_COMPOSITION_CAPTURE
            alphaTrace.Record(shared.surface9.Get(),saved.states[2],saved.states[3],simpleQuad);
#endif
        }
        catch(std::exception const& e){strcpy_s(error,e.what());End();}
        catch(...){strcpy_s(error,"UI GPU replay exception");End();}
    }
    static HRESULT STDMETHODCALLTYPE DrawHook(IDirect3DDevice9* d,D3DPRIMITIVETYPE type,UINT first,UINT count)
    {
#ifdef G3VR_STEREO_FRAME_CAPTURE
        if(auto* p=owner.load())p->writeTrace.Draw(d,0,count);
#endif
        auto* p=capturing;if(p&&p->Owns(d))p->Replay(d,[=]{return draw(d,type,first,count);},count==2&&(type==D3DPT_TRIANGLELIST||type==D3DPT_TRIANGLESTRIP));
        diagnostic::temporal::Geometry geometry{uint32_t(type),0,0,0,first,count};
        return TimeDraw(d,0,[=]{return draw(d,type,first,count);},&geometry);
    }
    static HRESULT STDMETHODCALLTYPE IndexedHook(IDirect3DDevice9* d,D3DPRIMITIVETYPE type,INT base,UINT min,UINT vertices,UINT first,UINT count)
    {
#ifdef G3VR_STEREO_FRAME_CAPTURE
        if(auto* p=owner.load())p->writeTrace.Draw(d,1,count);
#endif
        auto* p=capturing;if(p&&p->Owns(d))p->Replay(d,[=]{return indexed(d,type,base,min,vertices,first,count);},count==2&&(type==D3DPT_TRIANGLELIST||type==D3DPT_TRIANGLESTRIP));
        diagnostic::temporal::Geometry geometry{uint32_t(type),base,min,vertices,first,count};
        return TimeDraw(d,1,[=]{return indexed(d,type,base,min,vertices,first,count);},&geometry);
    }
    static HRESULT STDMETHODCALLTYPE UpHook(IDirect3DDevice9* d,D3DPRIMITIVETYPE type,UINT count,void const* vertices,UINT stride)
    {
#ifdef G3VR_STEREO_FRAME_CAPTURE
        if(auto* p=owner.load())p->writeTrace.Draw(d,2,count);
#endif
        auto* p=capturing;if(p&&p->Owns(d))p->Replay(d,[=]{return up(d,type,count,vertices,stride);},count==2&&(type==D3DPT_TRIANGLELIST||type==D3DPT_TRIANGLESTRIP));
        diagnostic::temporal::Geometry geometry{uint32_t(type),0,0,0,0,count};
        return TimeDraw(d,2,[=]{return up(d,type,count,vertices,stride);},&geometry);
    }
    static HRESULT STDMETHODCALLTYPE IndexedUpHook(IDirect3DDevice9* d,D3DPRIMITIVETYPE type,UINT min,UINT vertices,UINT count,void const* indices,D3DFORMAT format,void const* data,UINT stride)
    {
#ifdef G3VR_STEREO_FRAME_CAPTURE
        if(auto* p=owner.load())p->writeTrace.Draw(d,3,count);
#endif
        auto* p=capturing;if(p&&p->Owns(d))p->Replay(d,[=]{return indexedUp(d,type,min,vertices,count,indices,format,data,stride);},count==2&&(type==D3DPT_TRIANGLELIST||type==D3DPT_TRIANGLESTRIP));
        diagnostic::temporal::Geometry geometry{uint32_t(type),0,min,vertices,0,count};
        return TimeDraw(d,3,[=]{return indexedUp(d,type,min,vertices,count,indices,format,data,stride);},&geometry);
    }
public:
    using WorldObserver=bool(*)(void*,IDirect3DDevice9*,unsigned,diagnostic::temporal::Geometry const*)noexcept;
    WorldObserver worldObserver=nullptr;void* worldObserverContext=nullptr;
    D3D9TemporalProbe temporal;
#ifdef G3VR_STEREO_FRAME_CAPTURE
    diagnostic::WorldWriteTrace writeTrace;
#endif
#ifdef G3VR_UI_COMPOSITION_CAPTURE
    UiAlphaTrace alphaTrace;
#endif
    bool BeginWorldTiming(){
        if constexpr(!diagnostic::probes::Enabled){
            if constexpr(diagnostic::probes::StaticTrial){if(!active&&!measuring&&worldObserver){measuring=this;return true;}}
            return false;
        }
        if(active||measuring)return false;drawTiming.Begin(diagnostic::performance::Now());measuring=this;return true;
    }
    bool EndWorldTiming(diagnostic::performance::EyeDrawSample& sample){
        if constexpr(!diagnostic::probes::Enabled){if(measuring==this)measuring=nullptr;return false;}
        if(measuring!=this)return false;measuring=nullptr;return drawTiming.Finish(diagnostic::performance::Now(),sample);
    }
    SharedEye shared;
    uint64_t draws=0;
    char error[192]{};
    void Initialize(IDirect3DDevice9Ex* d,ID3D11Device* d11,UINT width,UINT height,DxvkGpuBridge* dxvk=nullptr)
    {
        if(owner.load())throw std::runtime_error("UI capture already installed");
        device=d;this->width=width;this->height=height;shared.Create(d,d11,width,height,dxvk);
        stereo::Hr(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&backbuffer),"UI native backbuffer");
#ifdef G3VR_STEREO_FRAME_CAPTURE
        writeTrace.Initialize(d,backbuffer.Get());
#endif
        auto table=*reinterpret_cast<void***>(d);
        void* replacements[]={reinterpret_cast<void*>(&DrawHook),reinterpret_cast<void*>(&IndexedHook),reinterpret_cast<void*>(&UpHook),reinterpret_cast<void*>(&IndexedUpHook)};
        for(unsigned i=0;i<4;++i)
            if(!diagnostic::Executable(table[81+i])||!hooks[i].Prepare(table,81+i,replacements[i]))throw std::runtime_error("Cannot prepare native UI GPU draw hooks");
        draw=reinterpret_cast<Draw>(hooks[0].original);indexed=reinterpret_cast<Indexed>(hooks[1].original);
        up=reinterpret_cast<Up>(hooks[2].original);indexedUp=reinterpret_cast<IndexedUp>(hooks[3].original);
        owner.store(this);
        for(auto& hook:hooks)if(!hook.Install())throw std::runtime_error("Cannot install native UI GPU draw hooks");
    }
    void Begin()
    {
        End();error[0]=0;frameCommands=0;thread=GetCurrentThreadId();
        SavedState saved(device.Get());Scope restore{[&]{saved.Restore();}};
        stereo::Hr(device->SetDepthStencilSurface(nullptr),"UI clear depth unbind");
        stereo::Hr(device->SetRenderTarget(0,shared.surface9.Get()),"UI clear target");
        D3DVIEWPORT9 viewport{0,0,width,height,0,1};RECT clip{0,0,LONG(width),LONG(height)};
        stereo::Hr(device->SetViewport(&viewport),"UI clear full viewport");stereo::Hr(device->SetScissorRect(&clip),"UI clear full scissor");
        stereo::Hr(device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"UI clear transparent");
        active=true;capturing=this;
    }
    // Native PlayVideo presents outside normal application UI callbacks.
    // Copy its completed backbuffer once; do not replay/advance video draws.
    void CopyBackbuffer(){
        End();error[0]=0;stereo::ComPtr<IDirect3DSurface9> source;
        stereo::Hr(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&source),"cinema backbuffer");
        D3DSURFACE_DESC desc{};stereo::Hr(source->GetDesc(&desc),"cinema dimensions");
        if(desc.Width!=width||desc.Height!=height)throw std::runtime_error("Cinema resolution changed");
        stereo::Hr(device->StretchRect(source.Get(),nullptr,shared.surface9.Get(),nullptr,D3DTEXF_NONE),"cinema shared copy");
    }
    void End() noexcept {active=false;if(capturing==this)capturing=nullptr;if(measuring==this)measuring=nullptr;drawTiming.Cancel();temporal.Stop();}
    void Reset() noexcept
    {
#ifdef G3VR_STEREO_FRAME_CAPTURE
        writeTrace.Reset();
#endif
        End();worldObserver=nullptr;worldObserverContext=nullptr;temporal.ReleaseStorage();for(auto& hook:hooks)hook.Restore();
        if(owner.load()==this)owner.store(nullptr);
        shared.Reset();backbuffer.Reset();device.Reset();
    }
    ~UiCapture(){Reset();}
};
}
