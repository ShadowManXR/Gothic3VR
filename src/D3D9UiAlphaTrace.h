#pragma once
// Bounded diagnostic-only atlas: nine pixels after each of at most 128 UI
// draws. No texture scan, GPU timing or trace exists in ordinary builds.
#include <array>
#include <fstream>
namespace g3vr {
class UiAlphaTrace {
    static constexpr unsigned Limit=128,Points=9;
    struct Draw {DWORD alpha=0,src=0,dst=0,mask=0,separate=0,fvf=0,test=0,srgb=0;UINT tw=0,th=0;bool quad=false,vs=false,ps=false;};
    std::array<Draw,Limit> draws{};stereo::ComPtr<IDirect3DSurface9> atlas;
    IDirect3DDevice9* device=nullptr;unsigned width=0,height=0,count=0;bool active=false,valid=true;
public:
    void Begin(IDirect3DDevice9* d,unsigned w,unsigned h)noexcept{
        device=d;width=w;height=h;count=0;valid=true;active=false;
        if(!atlas&&FAILED(d->CreateRenderTarget(Limit,Points,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&atlas,nullptr)))return;
        active=true;
    }
    unsigned Count()const{return count;}
    void Record(IDirect3DSurface9* source,DWORD nativeMask,DWORD nativeSeparate,bool quad)noexcept{
        if(!active||count==Limit)return;auto& s=draws[count];s={};s.mask=nativeMask;s.separate=nativeSeparate;s.quad=quad;
        stereo::ComPtr<IDirect3DVertexShader9> vs;stereo::ComPtr<IDirect3DPixelShader9> ps;device->GetVertexShader(&vs);device->GetPixelShader(&ps);s.vs=bool(vs);s.ps=bool(ps);
        device->GetRenderState(D3DRS_ALPHABLENDENABLE,&s.alpha);device->GetRenderState(D3DRS_SRCBLEND,&s.src);device->GetRenderState(D3DRS_DESTBLEND,&s.dst);
        device->GetRenderState(D3DRS_ALPHATESTENABLE,&s.test);device->GetRenderState(D3DRS_SRGBWRITEENABLE,&s.srgb);device->GetFVF(&s.fvf);
        stereo::ComPtr<IDirect3DBaseTexture9> base;device->GetTexture(0,&base);
        if(base&&base->GetType()==D3DRTYPE_TEXTURE){D3DSURFACE_DESC desc{};static_cast<IDirect3DTexture9*>(base.Get())->GetLevelDesc(0,&desc);s.tw=desc.Width;s.th=desc.Height;}
        for(unsigned i=0;i<Points;++i){float px[]={.02f,.5f,.98f};int x=int(float(width)*px[i%3]),y=int(float(height)*px[i/3]);RECT from{x,y,x+1,y+1},to{LONG(count),LONG(i),LONG(count+1),LONG(i+1)};
            if(FAILED(device->StretchRect(source,&from,atlas.Get(),&to,D3DTEXF_NONE)))valid=false;}
        ++count;
    }
    bool Save(std::wstring const& file)noexcept{try{
        if(!active)return false;active=false;if(!valid)return false;
        stereo::ComPtr<IDirect3DSurface9> cpu;if(FAILED(device->CreateOffscreenPlainSurface(Limit,Points,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&cpu,nullptr))||FAILED(device->GetRenderTargetData(atlas.Get(),cpu.Get())))return false;
        D3DLOCKED_RECT lock{};if(FAILED(cpu->LockRect(&lock,nullptr,D3DLOCK_READONLY)))return false;
        std::ofstream out(std::filesystem::path(file+L".draws.csv"));out<<"draw,alpha,src,dst,mask,separate,fvf,test,srgb,texture_w,texture_h,quad,vs,ps";
        for(unsigned i=0;i<Points;++i)out<<",p"<<i<<"_bgra";out<<'\n';
        for(unsigned n=0;n<count;++n){auto const& s=draws[n];out<<n<<','<<s.alpha<<','<<s.src<<','<<s.dst<<','<<s.mask<<','<<s.separate<<','<<s.fvf<<','<<s.test<<','<<s.srgb<<','<<s.tw<<','<<s.th<<','<<s.quad<<','<<s.vs<<','<<s.ps;
            for(unsigned i=0;i<Points;++i){uint32_t color=0;memcpy(&color,static_cast<char const*>(lock.pBits)+i*lock.Pitch+n*4,4);out<<','<<std::hex<<color<<std::dec;}out<<'\n';}
        cpu->UnlockRect();return bool(out);
    }catch(...){active=false;return false;}}
};
}
