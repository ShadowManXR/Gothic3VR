#include "NativeStaticTrial.h"
#include "NativeTrialBinding.h"
#include "NativeTrialSelection.h"
#include "NativeStereoProof.h"
#include "NativeMaterialCapture.h"
#include "NativeStoneRenderer.h"
#include "NativeStonePacketProducer.h"
#include "../../../src/TemporalObjectHistory.h"
#include <set>
#include "NativeWriteback.h"
#include "../../../src/DxvkGpuBridge.h"
#include <wrl/client.h>
#include <fstream>
#include <iomanip>
#include <map>
#include <chrono>

namespace g3vr::dx11 {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT hr,char const* text){if(FAILED(hr))throw std::runtime_error(std::string(text)+" HRESULT="+std::to_string(unsigned(hr)));}
void Array(std::ostream& file,float const* v,unsigned n){file<<'[';for(unsigned i=0;i<n;++i){if(i)file<<',';file<<v[i];}file<<']';}
// Opt-in trial evidence only: one bounded readback per eye, never a production
// render path. Downsample while copying rows; no full-resolution CPU images.
std::vector<uint32_t> NativePixels(IDirect3DDevice9Ex* device,unsigned width,unsigned height,unsigned step){
    ComPtr<IDirect3DSurface9> source,staging;Check(device->GetRenderTarget(0,&source),"Capture native target");D3DSURFACE_DESC d{};Check(source->GetDesc(&d),"Capture native description");
    if(d.Width!=width||d.Height!=height||d.MultiSampleType!=D3DMULTISAMPLE_NONE||(d.Format!=D3DFMT_A8R8G8B8&&d.Format!=D3DFMT_X8R8G8B8))throw std::runtime_error("Capture unsupported native target");
    Check(device->CreateOffscreenPlainSurface(width,height,d.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr),"Capture native staging");Check(device->GetRenderTargetData(source.Get(),staging.Get()),"Capture native readback");
    unsigned w=(width+step-1)/step,h=(height+step-1)/step;std::vector<uint32_t> result(size_t(w)*h);D3DLOCKED_RECT map{};Check(staging->LockRect(&map,nullptr,D3DLOCK_READONLY),"Capture native map");
    for(unsigned y=0;y<h;++y){auto* row=reinterpret_cast<uint32_t const*>(static_cast<char*>(map.pBits)+size_t(y*step)*map.Pitch);for(unsigned x=0;x<w;++x)result[size_t(y)*w+x]=row[x*step]|0xff000000;}
    Check(staging->UnlockRect(),"Capture native unmap");return result;
}
// One-shot comparison with the ACTUAL native streams/shaders/textures at the
// original DIP. Private targets remove scene occlusion from the parity check.
// The world observer's re-entry guard bypasses only this diagnostic replay.
template<class F>std::vector<uint32_t> NativeReferencePixels(IDirect3DDevice9Ex* d,unsigned width,unsigned height,unsigned step,diagnostic::temporal::Geometry const& geometry,g3vr::DxvkGpuBridge& audit,g3vr::DepthSnapshot& z,std::vector<uint32_t>& prepass,F&& recompose,bool comparePrepass=true,bool samplerControl=false){
    struct Samplers {
        IDirect3DDevice9Ex* d;DWORD saved[2][2]{};bool enabled=false;
        explicit Samplers(IDirect3DDevice9Ex* p):d(p){for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j)Check(d->GetSamplerState(4+i,j?D3DSAMP_MINFILTER:D3DSAMP_MAGFILTER,&saved[i][j]),"Reference saved material sampler");}
        ~Samplers(){if(enabled)for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j)d->SetSamplerState(4+i,j?D3DSAMP_MINFILTER:D3DSAMP_MAGFILTER,saved[i][j]);}
        void Control(){enabled=true;for(unsigned i=0;i<2;++i)for(auto state:{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER})Check(d->SetSamplerState(4+i,state,D3DTEXF_LINEAR),"Reference trilinear sampler control");}
    } samplers(d);
    if(samplerControl)samplers.Control();
    ComPtr<IDirect3DSurface9> color,depth;
    Check(d->CreateRenderTarget(width,height,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&color,nullptr),"Reference color");
    Check(d->CreateDepthStencilSurface(width,height,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&depth,nullptr),"Reference depth");
    struct Targets {
        IDirect3DDevice9Ex* d;ComPtr<IDirect3DSurface9> rt[4],depth;D3DVIEWPORT9 viewport{};RECT scissor{};unsigned count=0;
        explicit Targets(IDirect3DDevice9Ex* device):d(device){
            D3DCAPS9 caps{};Check(d->GetDeviceCaps(&caps),"Reference caps");count=std::min(4u,unsigned(caps.NumSimultaneousRTs));
            for(unsigned i=0;i<count;++i){auto hr=d->GetRenderTarget(i,&rt[i]);if(hr!=D3DERR_NOTFOUND)Check(hr,"Reference saved RT");}
            auto hr=d->GetDepthStencilSurface(&depth);if(hr!=D3DERR_NOTFOUND)Check(hr,"Reference saved depth");
            Check(d->GetViewport(&viewport),"Reference saved viewport");Check(d->GetScissorRect(&scissor),"Reference saved scissor");
        }
        ~Targets(){d->SetDepthStencilSurface(nullptr);for(unsigned i=1;i<count;++i)d->SetRenderTarget(i,nullptr);
            for(unsigned i=0;i<count;++i)d->SetRenderTarget(i,rt[i].Get());d->SetDepthStencilSurface(depth.Get());d->SetViewport(&viewport);d->SetScissorRect(&scissor);}
    } saved(d);
    Check(d->SetDepthStencilSurface(nullptr),"Reference unbind depth");
    for(unsigned i=1;i<saved.count;++i)Check(d->SetRenderTarget(i,nullptr),"Reference unbind MRT");
    Check(d->SetRenderTarget(0,color.Get()),"Reference bind color");Check(d->SetDepthStencilSurface(depth.Get()),"Reference bind depth");
    Check(d->SetViewport(&saved.viewport),"Reference exact viewport");Check(d->SetScissorRect(&saved.scissor),"Reference exact scissor");
    Check(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0),"Reference clear");
    Check(d->DrawIndexedPrimitive(D3DPRIMITIVETYPE(geometry.primitive),geometry.base,geometry.minimum,geometry.vertices,geometry.first,geometry.count),"Reference original DIP replay");
    if(!audit.CaptureDepthSnapshot(depth.Get(),z))throw std::runtime_error("Reference native depth unavailable");
    auto original=NativePixels(d,width,height,step);
    if(!comparePrepass){prepass.clear();return original;}
    // Retain the actual native depth and replace only its color. A cleared-Z
    // reference alone misses quantization errors that reject the entire draw
    // when Gothic has already populated the depth buffer.
    Check(d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"Reference prepass color clear");
    recompose();prepass=NativePixels(d,width,height,step);
    return original;
}
template<class T>std::vector<T> ProducerPixels(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Texture2D* source,unsigned step){
    D3D11_TEXTURE2D_DESC d{};source->GetDesc(&d);d.BindFlags=d.MiscFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;Check(device->CreateTexture2D(&d,nullptr,&staging),"Capture DX11 staging");
    unsigned w=(d.Width+step-1)/step,h=(d.Height+step-1)/step;std::vector<T> result(size_t(w)*h);context->OMSetRenderTargets(0,nullptr,nullptr);context->CopyResource(staging.Get(),source);D3D11_MAPPED_SUBRESOURCE map{};Check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"Capture DX11 map");
    for(unsigned y=0;y<h;++y){auto* row=reinterpret_cast<T const*>(static_cast<char*>(map.pData)+size_t(y*step)*map.RowPitch);for(unsigned x=0;x<w;++x)result[size_t(y)*w+x]=row[x*step];}
    context->Unmap(staging.Get(),0);return result;
}
void Bitmap(std::filesystem::path const& path,std::vector<uint32_t> const& pixels,unsigned w,unsigned h){
    BITMAPFILEHEADER head{};BITMAPINFOHEADER info{};head.bfType=0x4d42;head.bfOffBits=sizeof(head)+sizeof(info);head.bfSize=head.bfOffBits+w*h*4;info.biSize=sizeof(info);info.biWidth=LONG(w);info.biHeight=-LONG(h);info.biPlanes=1;info.biBitCount=32;
    std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<char*>(&head),sizeof(head));file.write(reinterpret_cast<char*>(&info),sizeof(info));file.write(reinterpret_cast<char const*>(pixels.data()),pixels.size()*4);if(!file)throw std::runtime_error("Capture bitmap write failed");
}
}
struct NativeStaticTrial::Impl {
    NativeStonePacketProducer producer;std::unique_ptr<OwnedStonePass> queued;PassStamp queuedStamp;
    g3vr::DepthSnapshot queuedNativeDepth;std::vector<uint32_t> queuedNativeColor;
    g3vr::DepthSnapshot queuedControlDepth;std::vector<uint32_t> queuedControlColor;
    uint64_t controlFrame[2]{},geometryFrame[2]{},selectionFrame=0;
    uint64_t validatedFrame[2]{};unsigned ownedComparisons=0,queuedStep=1;
    g3vr::DxvkGpuBridge depthAudit;NativeStereoProof stereoProof;bool referencePassed[2]{};unsigned replaced[2]{};bool suppressedThisPass=false;
    ComPtr<IDirect3DDevice9Ex> native;ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    StaticScene scene;NativeStoneRenderer stone;bool referenceAttempted[2]{};bool materialReady=false;unsigned materialRendered[2]{},materialRefusals=0;NativeWriteback writeback;ComPtr<ID3D11Texture2D> color,depth;
    ComPtr<ID3D11RenderTargetView> rtv;ComPtr<ID3D11DepthStencilView> dsv;ComPtr<ID3D11ShaderResourceView> srv;
    std::ofstream log;std::map<std::string,Mesh> meshes;unsigned width=0,height=0,mode=0,eye=2,rendered[2]{},matched[2]{},logged=0;
    std::map<std::string,NativeBounds> bounds;NativeTrialSelection selection;unsigned selectionLogs=0;
    NativeMaterialCapture materials;std::map<std::string,unsigned> materialCounts;unsigned expectedMaterials=0,reportedMaterials=0;
    uint64_t frame=0,epoch=1,lastFrame=0,selectedSeen=0,worldFrames=0,started=0;uintptr_t cookie=0;
    NativeStaticSnapshot selected{},sample{};float view[16]{},projection[16]{};Camera camera{};
    bool active=false,observed=false,done=false,fault=false,boundary=false;unsigned unmatchedLogs=0;
    void Select(float const* passView,float const* passProjection){
        if(cookie)return;auto* chosen=selection.Best(passView,passProjection);if(!chosen)return;
        cookie=chosen->cookie;selected=sample=chosen->snapshot;camera=chosen->camera;std::copy(chosen->view.begin(),chosen->view.end(),view);std::copy(chosen->projection.begin(),chosen->projection.end(),projection);observed=true;selectedSeen=selectionFrame=frame;if(!started)started=GetTickCount64();
        expectedMaterials=std::min(16u,2*materialCounts.at(NativeResourceKey(sample.resource)));
        if(selectionLogs++>=4)return;
        log<<"{\"type\":\"dx11_static_trial_selection\",\"frame\":"<<frame<<",\"eye\":"<<eye<<",\"resource\":\""<<NativeResourceKey(sample.resource)<<"\",\"score\":"<<chosen->score<<",\"candidate_count\":"<<selection.Count()<<",\"occlusion_proven\":false,\"candidates\":[";
        unsigned written=0;for(unsigned i=0;i<selection.Count()&&written<8;++i){auto const& c=selection.At(i);if(!SameNativePass(c.view.data(),passView)||!SameNativePass(c.projection.data(),passProjection))continue;
            if(written++)log<<',';log<<"{\"resource\":\""<<NativeResourceKey(c.snapshot.resource)<<"\",\"score\":"<<c.score<<",\"world_m\":";Array(log,c.snapshot.worldMeters.data(),16);log<<'}';}log<<"]}\n";log.flush();
    }
    std::filesystem::path output;bool captureAttempted[2]{},captureThisPass=false;std::vector<uint32_t> before,preview,coverage;unsigned captureStep=1;
    void CaptureOwned(NativeMaterialDraw const& draw,diagnostic::temporal::Geometry const& geometry){
        if(queued||ownedComparisons>=4)throw std::runtime_error("Owned validation capture budget");
        queuedStamp={frame,epoch,NativeCameraSignature(camera),eye};
        queued=producer.Capture(draw,sample,geometry,queuedStamp);
        if(!queued)throw std::runtime_error("Live owned packet producer refused the draw");
        queuedStep=std::max({1u,(width+1279)/1280,(height+719)/720});std::vector<uint32_t> unused;
        if(uint64_t(width)*height>16777216)throw std::runtime_error("Owned validation pixel budget");
        queuedNativeColor=NativeReferencePixels(native.Get(),width,height,queuedStep,geometry,depthAudit,queuedNativeDepth,unused,[]{},false);
        queuedControlColor=NativeReferencePixels(native.Get(),width,height,queuedStep,geometry,depthAudit,queuedControlDepth,unused,[]{},false,true);
        materialReady=true;
        log<<"{\"type\":\"dx11_owned_native_packet_queued\",\"frame\":"<<frame<<",\"eye\":"<<eye<<",\"epoch\":"<<epoch<<",\"camera_signature\":"<<queuedStamp.camera<<",\"native_base\":"<<geometry.base<<",\"native_minimum\":"<<geometry.minimum<<",\"native_first_index\":"<<geometry.first<<",\"native_buffer_content_fingerprint\":false,\"atlas_reads\":"<<producer.AtlasReads()<<",\"atlas_content_hits\":"<<producer.AtlasHits()<<",\"native_gpu_image_captures\":"<<producer.ImageCaptures()<<",\"gpu_input_snapshots\":4,\"native_draw_suppressed\":false}\n";log.flush();
    }
    void CompleteOwned(){
        if(!queued)return;
        if(materials.InScope())throw std::runtime_error("Deferred owned packet consumed before its native material scope ended");
        PassStamp expected{frame,epoch,NativeCameraSignature(camera),eye};
        if(!(queuedStamp==expected))throw std::runtime_error("Deferred live packet pass mismatch");
        unsigned w=(width+queuedStep-1)/queuedStep,h=(height+queuedStep-1)/queuedStep;
        auto compare=[&](bool control){
            bool ok=control?queued->RenderSamplerControl(stone,expected,rtv.Get(),dsv.Get(),width,height):queued->Render(stone,expected,rtv.Get(),dsv.Get(),width,height);
            if(!ok)throw std::runtime_error("Deferred live packet render refused");
            auto image=ProducerPixels<uint32_t>(device.Get(),context.Get(),color.Get(),queuedStep);
            auto z=ProducerPixels<float>(device.Get(),context.Get(),depth.Get(),queuedStep);
            auto const& reference=control?queuedControlColor:queuedNativeColor;auto const& referenceZ=control?queuedControlDepth:queuedNativeDepth;
            if(referenceZ.width!=width||referenceZ.height!=height||referenceZ.depth.size()!=size_t(width)*height||image.size()!=reference.size()||z.size()!=image.size())throw std::runtime_error("Owned validation dimensions");
            unsigned maximum=0;uint64_t changed=0,covered=0,coverageMismatch=0;float maxDepth=0;
            for(size_t i=0;i<image.size();++i){float a=referenceZ.depth[(i/w)*queuedStep*width+(i%w)*queuedStep],b=z[i];
                if(!std::isfinite(a)||!std::isfinite(b))throw std::runtime_error("Nonfinite owned validation depth");
                bool ac=a<1,bc=b<1;coverageMismatch+=ac!=bc;if(ac&&bc){++covered;maxDepth=std::max(maxDepth,std::fabs(a-b));}
                for(unsigned shift:{0u,8u,16u}){unsigned x=(reference[i]>>shift)&255,y=(image[i]>>shift)&255,e=x>y?x-y:y-x;maximum=std::max(maximum,e);changed+=e!=0;}}
            bool geometryPassed=covered>100&&coverageMismatch==0&&maxDepth<=4.f/16777215;
            bool passed=geometryPassed&&maximum<=1;
            if(!control)geometryFrame[eye]=geometryPassed?frame:0;
            auto* frames=control?controlFrame:validatedFrame;frames[eye]=passed?frame:0;bool pair=passed&&frames[0]==frame&&frames[1]==frame;
            auto prefix=std::string("owned-eye-")+std::to_string(eye)+(control?"-sampler-control":"");Bitmap(output/(prefix+"-native.bmp"),reference,w,h);Bitmap(output/(prefix+"-dx11.bmp"),image,w,h);
            log<<"{\"type\":\""<<(control?"dx11_owned_native_sampler_control":"dx11_owned_native_validation")<<"\",\"frame\":"<<frame<<",\"eye\":"<<eye<<",\"sample_step\":"<<queuedStep<<",\"covered_samples\":"<<covered<<",\"coverage_disagreements\":"<<coverageMismatch<<",\"max_rgb_steps\":"<<maximum<<",\"changed_components\":"<<changed<<",\"max_depth_error\":"<<maxDepth<<",\"sdk_scope_ended\":true,\"passed\":"<<(passed?"true":"false")<<",\"stereo_pair_passed\":"<<(pair?"true":"false")<<",\"native_draw_suppressed\":false}\n";log.flush();
            return passed;
        };
        bool controlPassed=compare(true);compare(false);++ownedComparisons;
        bool pair=validatedFrame[0]==frame&&validatedFrame[1]==frame,controlPair=controlFrame[0]==frame&&controlFrame[1]==frame,geometryPair=geometryFrame[0]==frame&&geometryFrame[1]==frame;
        queued.reset();queuedNativeColor.clear();queuedNativeDepth.depth.clear();queuedControlColor.clear();queuedControlDepth.depth.clear();boundary=true;++rendered[eye];
        if(!controlPassed||controlPair||ownedComparisons>=4){
            std::ofstream result(output/L"owned-validation.json");result<<"{\"result\":\""<<(pair?"PASS":controlPair&&geometryPair?"SAMPLER_CONTROL_ONLY":"REFUSED")<<"\",\"native_preserved\":true,\"same_frame_stereo_pair\":"<<(pair?"true":"false")<<",\"same_frame_geometry_pair\":"<<(geometryPair?"true":"false")<<",\"same_frame_sampler_control_pair\":"<<(controlPair?"true":"false")<<",\"atlas_reads\":"<<producer.AtlasReads()<<",\"atlas_content_hits\":"<<producer.AtlasHits()<<",\"image_captures\":"<<producer.ImageCaptures()<<",\"native_draws_suppressed\":0,\"performance_gain_proven\":false}\n";result.flush();done=true;
        }
    }
    bool CaptureMaterialReference(diagnostic::temporal::Geometry const& geometry)noexcept{
        if(eye>1)return false;if(referenceAttempted[eye])return stereoProof.Allows(frame);referenceAttempted[eye]=true;
        try{
            unsigned step=std::max({1u,(width+1279)/1280,(height+719)/720});
            if(uint64_t(width)*height>16777216)throw std::runtime_error("Reference capture budget");
            g3vr::DepthSnapshot nativeDepth;std::vector<uint32_t> prepass;
            auto original=NativeReferencePixels(native.Get(),width,height,step,geometry,depthAudit,nativeDepth,prepass,[&]{
                PassStamp stamp{frame,epoch,NativeCameraSignature(camera),eye};
                writeback.Transfer(color.Get(),srv.Get(),stamp);writeback.Composite(stamp,true);
            });
            auto replacement=ProducerPixels<uint32_t>(device.Get(),context.Get(),color.Get(),step);
            auto replacementDepth=ProducerPixels<float>(device.Get(),context.Get(),depth.Get(),step);
            if(nativeDepth.width!=width||nativeDepth.height!=height||nativeDepth.depth.size()!=size_t(width)*height)throw std::runtime_error("Reference depth dimensions");
            auto prefix=std::string("eye-")+std::to_string(eye);unsigned w=(width+step-1)/step,h=(height+step-1)/step;
            Bitmap(output/(prefix+"-native-isolated.bmp"),original,w,h);Bitmap(output/(prefix+"-dx11-isolated.bmp"),replacement,w,h);
            uint64_t active=0,changed=0,mismatch=0,error=0,depthMismatch=0,depthSamples=0,prepassMismatch=0,prepassError=0;unsigned maximum=0,prepassMaximum=0;float maxDepthError=0;
            for(size_t i=0;i<original.size();++i){float aDepth=nativeDepth.depth[(i/w)*step*width+(i%w)*step],bDepth=replacementDepth[i];
                bool aCovered=aDepth<1,bCovered=bDepth<1;depthMismatch+=aCovered!=bCovered;
                if(aCovered&&bCovered){++depthSamples;maxDepthError=std::max(maxDepthError,std::fabs(aDepth-bDepth));}
                auto a=original[i]&0xffffff,b=replacement[i]&0xffffff;
                auto restored=prepass[i]&0xffffff;prepassMismatch+=(a!=0)!=(restored!=0);
                if(a||b){++active;mismatch+=(a!=0)!=(b!=0);}
                for(unsigned shift:{0u,8u,16u}){unsigned x=(a>>shift)&255,y=(b>>shift)&255,e=x>y?x-y:y-x;
                    changed+=e!=0;error+=e;maximum=std::max(maximum,e);}}
            for(size_t i=0;i<original.size();++i)for(unsigned shift:{0u,8u,16u}){unsigned a=(original[i]>>shift)&255,b=(prepass[i]>>shift)&255,e=a>b?a-b:b-a;prepassError+=e;prepassMaximum=std::max(prepassMaximum,e);}
            // Native SM3 vs SM5 arithmetic can differ by3D24 quanta even with
            // identical coverage; allow4, retaining exact depth coverage.
            bool prepassPassed=active>100&&prepassMismatch==0&&prepassMaximum<=64&&double(prepassError)/(active*3)<=2;
            referencePassed[eye]=prepassPassed&&active>100&&mismatch==0&&maximum<=64&&double(error)/(active*3)<=2&&depthSamples>100&&depthMismatch==0&&maxDepthError<=4.f/16777215;
            log<<"{\"type\":\"dx11_native_depth_prepass_comparison\",\"eye\":"<<eye<<",\"frame\":"<<frame<<",\"coverage_disagreements\":"<<prepassMismatch<<",\"max_unorm_steps\":"<<prepassMaximum<<",\"passed\":"<<(prepassPassed?"true":"false")<<"}\n";
            log<<"{\"type\":\"dx11_isolated_native_material_comparison\",\"frame\":"<<frame<<",\"eye\":"<<eye<<",\"sample_step\":"<<step<<",\"active_rgb_samples\":"<<active<<",\"rgb_coverage_disagreements\":"<<mismatch<<",\"changed_rgb_components\":"<<changed<<",\"max_unorm_steps\":"<<maximum<<",\"mean_absolute_rgb_steps\":"<<(active?double(error)/(active*3):0)<<",\"depth_samples\":"<<depthSamples<<",\"depth_coverage_disagreements\":"<<depthMismatch<<",\"max_depth_error\":"<<maxDepthError<<",\"replacement_gate_passed\":"<<(referencePassed[eye]?"true":"false")<<",\"native_draw_suppressed\":false,\"material_parity_proven\":false,\"diagnostic_readback_only\":true}\n";log.flush();
        }catch(std::exception const&){log<<"{\"type\":\"dx11_isolated_material_capture_unavailable\",\"eye\":"<<eye<<",\"native_preserved\":true}\n";log.flush();}
        stereoProof.Record(eye,frame,referencePassed[eye]);
        return stereoProof.Allows(frame);
    }
    void CaptureBefore()noexcept{
        captureThisPass=false;before.clear();preview.clear();coverage.clear();if(mode<2||eye>1||captureAttempted[eye])return;captureAttempted[eye]=true;captureThisPass=true;
        try{
            if(uint64_t(width)*height>16777216)throw std::runtime_error("Capture memory budget exceeded");
            captureStep=std::max({1u,(width+1279)/1280,(height+719)/720});
            before=NativePixels(native.Get(),width,height,captureStep);preview=ProducerPixels<uint32_t>(device.Get(),context.Get(),color.Get(),captureStep);
            auto values=ProducerPixels<float>(device.Get(),context.Get(),depth.Get(),captureStep);coverage.resize(values.size());for(size_t i=0;i<values.size();++i)coverage[i]=values[i]<1?0xffffffff:0xff000000;
        }catch(...){before.clear();preview.clear();coverage.clear();try{log<<"{\"type\":\"dx11_static_trial_capture_unavailable\",\"eye\":"<<eye<<",\"native_preserved\":true}\n";log.flush();}catch(...){}}
    }
    void CaptureAfter()noexcept{
        if(before.empty())return;
        try{
            auto after=NativePixels(native.Get(),width,height,captureStep);unsigned w=(width+captureStep-1)/captureStep,h=(height+captureStep-1)/captureStep;
            auto prefix=std::string("eye-")+std::to_string(eye);Bitmap(output/(prefix+"-native-before.bmp"),before,w,h);Bitmap(output/(prefix+"-composited-after.bmp"),after,w,h);Bitmap(output/(prefix+"-dx11-preview.bmp"),preview,w,h);Bitmap(output/(prefix+"-dx11-coverage.bmp"),coverage,w,h);
            unsigned changed=0,covered=0;for(size_t i=0;i<before.size();++i){changed+=before[i]!=after[i];covered+=coverage[i]==0xffffffff;}
            log<<"{\"type\":\"dx11_static_trial_capture\",\"frame\":"<<frame<<",\"eye\":"<<eye<<",\"width\":"<<w<<",\"height\":"<<h<<",\"sample_step\":"<<captureStep<<",\"coverage_samples\":"<<covered<<",\"changed_samples\":"<<changed<<",\"material_parity_proven\":false,\"diagnostic_readback_only\":true}\n";log.flush();
        }catch(...){try{log<<"{\"type\":\"dx11_static_trial_capture_unavailable\",\"eye\":"<<eye<<",\"native_preserved\":true}\n";log.flush();}catch(...){}}
        before.clear();preview.clear();coverage.clear();
    }
    void Fail(char const* message)noexcept{fault=true;active=false;try{log<<"{\"type\":\"dx11_static_trial_failed\",\"native_preserved\":true,\"error\":\"";for(char const* p=message;*p;++p){if(*p=='"'||*p=='\\')log<<'\\';if(static_cast<unsigned char>(*p)>=32)log<<*p;}log<<"\"}\n";log.flush();}catch(...){}}
    void Record(double ms,bool composite,char const* type="dx11_static_trial_eye"){
        if(logged>=16)return;++logged;
        log<<"{\"type\":\""<<type<<"\",\"frame\":"<<frame<<",\"epoch\":"<<epoch<<",\"eye\":"<<eye<<",\"resource\":\""<<NativeResourceKey(sample.resource)<<"\",\"identity\":[";
        for(unsigned i=0;i<5;++i){if(i)log<<',';log<<sample.identity[i];}log<<"],\"world_m\":";Array(log,sample.worldMeters.data(),16);log<<",\"native_view_cm\":";Array(log,view,16);log<<",\"native_projection_cm\":";Array(log,projection,16);log<<",\"view_projection_m\":";Array(log,reinterpret_cast<float const*>(&camera.viewProjection),16);
        log<<",\"camera_signature\":"<<NativeCameraSignature(camera)<<",\"composited\":"<<(composite?"true":"false")<<",\"native_draw_suppressed\":"<<(suppressedThisPass?"true":"false")<<",\"diagnostic_capture_frame\":"<<(captureThisPass?"true":"false")<<",\"cpu_wall_ms\":"<<ms<<"}\n";log.flush();
    }
};
NativeStaticTrial::NativeStaticTrial()=default;
NativeStaticTrial::~NativeStaticTrial()=default;
void NativeStaticTrial::Reset()noexcept{impl.reset();}
void NativeStaticTrial::Initialize(IDirect3DDevice9Ex* native,IDXGIAdapter* adapter,std::filesystem::path const& assets,std::filesystem::path const& output,unsigned width,unsigned height,unsigned mode){
    if(!native||!adapter||!width||!height||mode<1||mode>5)throw std::runtime_error("Invalid native static trial configuration");
    auto s=std::make_unique<Impl>();s->native=native;s->width=width;s->height=height;s->mode=mode;s->output=output;
    std::filesystem::create_directories(output);s->log.open(output/L"native-static-trial.jsonl",std::ios::out|std::ios::trunc);if(!s->log)throw std::runtime_error("Cannot write static trial output");s->log<<std::setprecision(9);
    // Separate device/context: the experiment cannot corrupt the VR blit/UI pipeline.
    D3D_FEATURE_LEVEL level;Check(D3D11CreateDevice(adapter,D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&s->device,&level,&s->context),"Trial D3D11 device");if(level<D3D_FEATURE_LEVEL_11_0)throw std::runtime_error("Trial requires DX11 hardware");
    s->scene.Initialize(s->device.Get(),s->context.Get());
    if(mode>=3){s->stone.Initialize(native,s->device.Get(),s->context.Get(),assets/L"native-stone");if(!s->depthAudit.Initialize(native))throw std::runtime_error("Reference requires DXVK depth audit");}
    if(mode==5)s->producer.Initialize(native,s->device.Get(),s->context.Get());
    for(auto const& entry:std::filesystem::directory_iterator(assets))if(entry.path().extension()==L".g3mesh"){
        auto mesh=ReadMesh(entry.path());auto key=NativeResourceKey(mesh.resource.c_str());if(key.empty()||s->meshes.count(key))throw std::runtime_error("Ambiguous native mesh identity");s->scene.Upload(mesh);std::set<std::string> materialNames;for(auto const& section:mesh.sections)materialNames.insert(section.material);s->materialCounts[key]=unsigned(materialNames.size());s->bounds.emplace(key,NativeMeshBounds(mesh));s->meshes.emplace(key,std::move(mesh));}
    if(s->meshes.empty())throw std::runtime_error("No local native meshes for trial");s->scene.LoadMaterialPreview(assets/L"materials"/L"preview.g3mat");
    D3D11_TEXTURE2D_DESC t{};t.Width=width;t.Height=height;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=mode>=3?DXGI_FORMAT_B8G8R8A8_UNORM:DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;t.BindFlags=D3D11_BIND_RENDER_TARGET;
    Check(s->device->CreateTexture2D(&t,nullptr,&s->color),"Trial color");Check(s->device->CreateRenderTargetView(s->color.Get(),nullptr,&s->rtv),"Trial RTV");
    t.Format=DXGI_FORMAT_R32_TYPELESS;t.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;Check(s->device->CreateTexture2D(&t,nullptr,&s->depth),"Trial depth");
    D3D11_DEPTH_STENCIL_VIEW_DESC ds{};ds.Format=DXGI_FORMAT_D32_FLOAT;ds.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;Check(s->device->CreateDepthStencilView(s->depth.Get(),&ds,&s->dsv),"Trial DSV");D3D11_SHADER_RESOURCE_VIEW_DESC sr{};sr.Format=DXGI_FORMAT_R32_FLOAT;sr.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;sr.Texture2D.MipLevels=1;Check(s->device->CreateShaderResourceView(s->depth.Get(),&sr,&s->srv),"Trial SRV");
    if(mode>=2&&mode!=5)s->writeback.Initialize(native,s->device.Get(),s->context.Get(),width,height);
    s->materials.Initialize(output/L"native-materials",width,height,mode>=3);
    s->log<<"{\"type\":\"dx11_static_trial_ready\",\"mode\":"<<mode<<",\"width\":"<<width<<",\"height\":"<<height<<",\"meshes\":"<<s->meshes.size()<<",\"native_preserved\":true,\"materials_exact\":false,\"native_raster_centers\":true,\"automatic_eye_captures\":"<<(mode>=2?"true":"false")<<",\"world_frame_limit\":600,\"time_limit_ms\":60000}\n";s->log.flush();impl=std::move(s);
}
void NativeStaticTrial::BeginEye(unsigned eye,uint64_t frame,int context)noexcept{
    if(!impl)return;auto& s=*impl;
    if(s.mode==5&&s.queued){s.queued.reset();s.Fail("Queued owned pass missed its matching solid boundary");return;}
    s.suppressedThisPass=false;s.active=false;s.observed=false;s.boundary=false;s.materialReady=false;s.eye=eye;s.frame=frame;s.selection.Reset();
    if(s.done||s.fault||eye>1)return;
    if(context!=1){if(s.cookie){s.cookie=0;s.selected={};s.referencePassed[0]=s.referencePassed[1]=false;s.stereoProof.Invalidate();++s.epoch;}return;}
    if(eye==0&&frame!=s.lastFrame){if(s.lastFrame&&frame!=s.lastFrame+1){s.cookie=0;s.selected={};s.referencePassed[0]=s.referencePassed[1]=false;s.stereoProof.Invalidate();++s.epoch;}s.lastFrame=frame;++s.worldFrames;
        if(s.cookie&&frame>s.selectedSeen+90){s.cookie=0;s.selected={};s.referencePassed[0]=s.referencePassed[1]=false;s.stereoProof.Invalidate();++s.epoch;}}
    if(s.worldFrames>=600||(s.started&&GetTickCount64()-s.started>60000)){s.done=true;try{s.materials.WriteDiagnostics(s.log);s.log<<"{\"type\":\"dx11_static_trial_complete\",\"left_rendered\":"<<s.rendered[0]<<",\"right_rendered\":"<<s.rendered[1]<<",\"left_matched\":"<<s.matched[0]<<",\"right_matched\":"<<s.matched[1]<<",\"material_draws_captured\":"<<s.materials.Count()<<",\"left_replaced\":"<<s.replaced[0]<<",\"right_replaced\":"<<s.replaced[1]<<",\"native_preserved\":"<<((s.replaced[0]||s.replaced[1])?"false":"true")<<"}\n";s.log.flush();}catch(...){}return;}
    s.active=true;
}
bool NativeStaticTrial::WantsContext(uintptr_t cookie)const noexcept{return impl&&impl->active&&cookie&&(impl->cookie?impl->cookie==cookie:impl->selection.Probe());}
void NativeStaticTrial::Observe(uintptr_t cookie,NativeStaticSnapshot const& sample,float const* view,float const* projection)noexcept{
    if(!impl||!impl->active||!cookie||(impl->cookie&&impl->cookie!=cookie))return;auto& s=*impl;
    try{if(!ValidNativeIdentity(sample)||!s.meshes.count(NativeResourceKey(sample.resource)))return;
        if(s.mode>=3&&NativeResourceKey(sample.resource)!="g3_myrtana_ardea_stairs_02.g3mesh")return;
        auto camera=NativeRasterCameraMeters(view,projection,s.width,s.height);if(s.cookie&&s.selected.identity!=sample.identity)return;
        if(!s.cookie){auto score=NativeCandidateScore(s.bounds.at(NativeResourceKey(sample.resource)),sample,camera);s.selection.Offer(cookie,sample,camera,view,projection,score);if(score>0)++s.matched[s.eye];if((s.mode==4||s.mode==5)&&score>0)s.Select(view,projection);return;}
        s.sample=sample;s.camera=camera;std::copy_n(view,16,s.view);std::copy_n(projection,16,s.projection);s.observed=true;s.selectedSeen=s.frame;++s.matched[s.eye];
    }catch(std::exception const& e){s.Fail(e.what());}
}
void NativeStaticTrial::SolidBoundary(float const* view,float const* projection)noexcept{
    if(!PendingPass(view,projection))return;auto& s=*impl;if(s.mode==4)return;
    try{D3DVIEWPORT9 vp{};Check(s.native->GetViewport(&vp),"Trial native viewport");if(vp.X||vp.Y||vp.Width!=s.width||vp.Height!=s.height||vp.MinZ!=0||vp.MaxZ!=1)return;s.Select(view,projection);
        if(s.mode==5){s.CompleteOwned();return;}
        auto started=std::chrono::steady_clock::now();if(s.mode>=3){if(!s.materialReady)return;}else {auto const& mesh=s.meshes.at(NativeResourceKey(s.sample.resource));Instance instance{1,mesh.resource,mesh.content,{}};std::memcpy(&instance.world,s.sample.worldMeters.data(),64);
        s.scene.Prepare({instance},{s.camera,s.camera});float clear[]{0,0,0,0};s.context->ClearRenderTargetView(s.rtv.Get(),clear);s.context->ClearDepthStencilView(s.dsv.Get(),D3D11_CLEAR_DEPTH,1,0);s.scene.Render(s.eye,{s.rtv.Get(),s.dsv.Get(),s.width,s.height});}
        // Comparison only: original native geometry, depth and shadows remain.
        bool composite=s.mode>=2;s.CaptureBefore();if(composite){s.writeback.Transfer(s.color.Get(),s.srv.Get(),{s.frame,s.epoch,NativeCameraSignature(s.camera),s.eye});s.writeback.Composite({s.frame,s.epoch,NativeCameraSignature(s.camera),s.eye});}s.CaptureAfter();
        s.boundary=true;++s.rendered[s.eye];s.Record(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count(),composite);
    }catch(std::exception const& e){s.Fail(e.what());}
}
bool NativeStaticTrial::PendingPass(float const* view,float const* projection)const noexcept{return impl&&impl->active&&!impl->boundary&&(impl->cookie?(impl->observed&&SameNativePass(impl->view,view)&&SameNativePass(impl->projection,projection)):impl->selection.Best(view,projection)!=nullptr);}
bool NativeStaticTrial::MaterialObservationEnabled()const noexcept{return impl&&impl->active&&!(impl->mode==4&&impl->stereoProof.Refused())&&!(impl->mode==5&&impl->frame<=impl->selectionFrame)&&impl->cookie&&(impl->mode>=3||impl->materials.Count()<impl->expectedMaterials);}
bool NativeStaticTrial::NeedsMaterialWork()const noexcept{return impl&&impl->active&&!(impl->mode==4&&impl->stereoProof.Refused())&&!impl->materialReady&&impl->observed&&impl->materials.InScope();}
bool NativeStaticTrial::WantsMaterial(uintptr_t cookie)const noexcept{return MaterialObservationEnabled()&&cookie&&impl->cookie==cookie;}
bool NativeStaticTrial::WantsMaterialEntity(std::array<uint32_t,5> const& id)const noexcept{return MaterialObservationEnabled()&&impl->selected.identity==id;}
unsigned NativeStaticTrial::BeginDeferredMaterial(std::array<uint32_t,5> const& id,float const* worldCm,char const* name,NativeLightmapSource lightmap)noexcept{
    if(!impl)return UINT_MAX;auto& s=*impl;
    if(!worldCm||!WantsMaterialEntity(id))return s.materials.InScope()?s.materials.Begin(2,0,nullptr,nullptr):UINT_MAX;
    if(s.mode>=3&&NativeResourceKey(name)!="g3_nature_wall_stone_01_f.xshmat")return s.materials.InScope()?s.materials.Begin(2,0,nullptr,nullptr):UINT_MAX;
    return s.materials.Begin(s.eye,s.frame,s.sample.resource,name,worldCm,lightmap);
}
unsigned NativeStaticTrial::BeginMaterial(uintptr_t cookie,char const* name)noexcept{
    if(!impl)return UINT_MAX;auto& s=*impl;
    if(!WantsMaterial(cookie))return s.materials.InScope()?s.materials.Begin(2,0,nullptr,nullptr):UINT_MAX;
    return s.materials.Begin(s.eye,s.frame,s.sample.resource,name);
}
void NativeStaticTrial::EndMaterial(unsigned token)noexcept{if(impl)impl->materials.End(token);}
bool NativeStaticTrial::ObserveMaterialDraw(IDirect3DDevice9* d,unsigned kind,diagnostic::temporal::Geometry const* geometry)noexcept{
    if(!impl||!impl->active)return false;auto& s=*impl;s.materials.Observe(d,kind);
    if(s.mode<3||s.materialReady||!s.observed||(s.mode==4&&s.stereoProof.Refused()))return false;
    if(kind!=1||!geometry||geometry->primitive!=D3DPT_TRIANGLELIST||geometry->vertices!=2097||geometry->count!=1130)return false;
    try{
        auto* draw=s.materials.Latest(s.eye,s.frame);if(!draw)return false;
        if(s.mode==5){s.CaptureOwned(*draw,*geometry);return false;}
        if(!s.stone.Render(*draw,s.rtv.Get(),s.dsv.Get(),s.width,s.height)){
            if(s.materialRefusals++<4){s.log<<"{\"type\":\"dx11_native_stone_material_refused\",\"reason\":\""<<s.stone.LastRefusal()<<"\"}\n";s.log.flush();}return false;}
        bool permitted=s.CaptureMaterialReference(*geometry);s.materialReady=true;++s.materialRendered[s.eye];
        if(s.materialRendered[s.eye]<=2){s.log<<"{\"type\":\"dx11_native_stone_material_rendered\",\"eye\":"<<s.eye<<",\"frame\":"<<s.frame<<",\"live_atlas_vertices\":true,\"live_gpu_noise_lightmap_shadows\":true,\"replacement_gate_passed\":"<<(permitted?"true":"false")<<",\"native_draw_suppressed\":false,\"material_parity_proven\":false}\n";s.log.flush();}
        if(s.mode!=4||!permitted)return false;
        // Commit color+depth AT the original DIP, not at a later scene boundary.
        // Return true only after successful writeback; all refusals keep native.
        auto started=std::chrono::steady_clock::now();s.CaptureBefore();
        PassStamp stamp{s.frame,s.epoch,NativeCameraSignature(s.camera),s.eye};
        s.writeback.Transfer(s.color.Get(),s.srv.Get(),stamp);s.writeback.Composite(stamp,true);
        s.suppressedThisPass=true;s.boundary=true;++s.replaced[s.eye];++s.rendered[s.eye];
        s.CaptureAfter();s.Record(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count(),true);
        return true;
    }catch(std::exception const& e){s.Fail(e.what());return false;}
}
void NativeStaticTrial::EndEye()noexcept{if(impl){
    impl->materials.Flush();
    try{if(impl->materials.Count()!=impl->reportedMaterials){impl->reportedMaterials=impl->materials.Count();impl->log<<"{\"type\":\"dx11_static_trial_material_evidence\",\"frame\":"<<impl->frame<<",\"eye\":"<<impl->eye<<",\"captured_draws\":"<<impl->reportedMaterials<<",\"expected_material_eye_pairs\":"<<impl->expectedMaterials<<",\"native_draw_suppressed\":false}\n";impl->log.flush();}
        if(impl->active&&impl->observed&&!impl->boundary&&impl->unmatchedLogs<2){++impl->unmatchedLogs;impl->Record(0,false,"dx11_static_trial_observed_without_compatible_boundary");}
        if(impl->active&&impl->mode==1)impl->context->Flush();}catch(...){impl->Fail("Trial end-eye logging failed");}
    impl->active=false;impl->eye=2;if(impl->done||impl->fault)impl.reset();}}
}


