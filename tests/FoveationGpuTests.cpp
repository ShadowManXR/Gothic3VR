#include "DxvkFoveation.h"
#include "StereoRenderer.h"
#include "FoveationPattern.h"
#include <d3dcompiler.h>
#include <vector>
#include <cstdio>
using stereo::ComPtr;
void Need(bool b,char const* m){if(!b)throw std::runtime_error(m);}
struct Module{HMODULE value{};~Module(){if(value)FreeLibrary(value);}};
struct Window{HWND value=CreateWindowW(L"STATIC",L"FFR GPU test",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(value)DestroyWindow(value);}};
std::vector<DWORD> Draw(IDirect3DDevice9Ex* d,unsigned w,unsigned h,bool depth){
    struct V{float x,y,z,rhw;};V v[]={{-.5f,-.5f,.5f,1},{float(w)-.5f,-.5f,.5f,1},{-.5f,float(h)-.5f,.5f,1},{float(w)-.5f,float(h)-.5f,.5f,1}};
    d->SetFVF(D3DFVF_XYZRHW);d->SetVertexShader(nullptr);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ZENABLE,depth?D3DZB_TRUE:D3DZB_FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
    stereo::Hr(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0),"clear");stereo::Hr(d->BeginScene(),"scene");stereo::Hr(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(V)),"draw");stereo::Hr(d->EndScene(),"end");
    ComPtr<IDirect3DSurface9> rt,sys;stereo::Hr(d->GetRenderTarget(0,&rt),"RT");stereo::Hr(d->CreateOffscreenPlainSurface(w,h,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&sys,nullptr),"readback");stereo::Hr(d->GetRenderTargetData(rt.Get(),sys.Get()),"copy pixels");D3DLOCKED_RECT m{};stereo::Hr(sys->LockRect(&m,nullptr,D3DLOCK_READONLY),"lock");std::vector<DWORD> out(w*h);for(unsigned y=0;y<h;++y)memcpy(out.data()+y*w,static_cast<char*>(m.pBits)+y*m.Pitch,w*4);sys->UnlockRect();return out;
}
int wmain(int argc,wchar_t** argv){try{
    Need(argc==2||argc==3,"Pass custom DXVK d3d9.dll");Module dll;dll.value=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Need(dll.value!=nullptr,"load fork");auto create=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(dll.value,"Direct3DCreate9Ex"));Need(create!=nullptr,"export");Window window;ComPtr<IDirect3D9Ex> api;stereo::Hr(create(D3D_SDK_VERSION,&api),"create9");
    unsigned w=512,h=512;D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.BackBufferWidth=w;p.BackBufferHeight=h;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.hDeviceWindow=window.value;p.EnableAutoDepthStencil=TRUE;p.AutoDepthStencilFormat=D3DFMT_D24S8;ComPtr<IDirect3DDevice9Ex> native;stereo::Hr(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&p,nullptr,&native),"device");
    char const* source="float4 main(float2 p:VPOS):COLOR{return float4(frac(p.x*.5),frac(p.y*.5),.3,1);}";ComPtr<ID3DBlob> shader,errors;auto result=D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"main","ps_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&shader,&errors);if(FAILED(result)&&errors)std::printf("%s\n",static_cast<char*>(errors->GetBufferPointer()));stereo::Hr(result,"test shader");ComPtr<IDirect3DPixelShader9> ps;stereo::Hr(native->CreatePixelShader(static_cast<DWORD const*>(shader->GetBufferPointer()),&ps),"pixelshader");native->SetPixelShader(ps.Get());
    auto baseline=Draw(native.Get(),w,h,true);Need(baseline[w*100+100]!=baseline[w*100+101],"Baseline must resolve alternating native pixels");
    if(argc==3){g3vr::DxvkFoveation unsupported;Need(!unsupported.Initialize(native.Get()),"Stock DXVK must safely decline FFR");Need(Draw(native.Get(),w,h,true)==baseline,"Unsupported FFR changed rendering");std::puts("PASS stock DXVK fallback without FFR exports");return 0;}
    for(unsigned cycle=0;cycle<2;++cycle){g3vr::DxvkFoveation ffr;Need(ffr.Initialize(native.Get()),"Actual DXVK device did not enable NV FFR features");ffr.Configure(w,h,1,70);ffr.BeginEye(0);auto coarse=Draw(native.Get(),w,h,true);ffr.EndEye();auto restored=Draw(native.Get(),w,h,true);Need(restored==baseline,"World scope leaked to following native draws");
        unsigned centerDifferent=0,edgeDifferent=0,edgePixels=0;for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){float dx=(float(x)+.5f)/w-.5f,dy=(float(y)+.5f)/h-.5f;float r=diagnostic::FfrOvalDistanceSquared(dx*2,dy*2);if(r<.4f*.4f)centerDifferent+=coarse[y*w+x]!=baseline[y*w+x];if(r>.9f*.9f){++edgePixels;edgeDifferent+=coarse[y*w+x]!=baseline[y*w+x];}}
        std::printf("cycle%u native center mismatches %u; coarse periphery changed %u/%u pixels\n",cycle,centerDifferent,edgeDifferent,edgePixels);Need(centerDifferent==0,"FFR degraded the native center");Need(edgeDifferent>edgePixels/4,"No real coarse shading occurred in periphery");
        for(int mode=1;mode<=3;++mode){
            ffr.Configure(w,h,mode,40);ffr.BeginEye(0);auto narrowOval=Draw(native.Get(),w,h,true);
            // Intentionally leave the eye scope bound while changing the map.
            // Configure must disable and drain preceding reads before updating.
            ffr.Configure(w,h,mode,95);ffr.BeginEye(1);auto wideOval=Draw(native.Get(),w,h,true);
            unsigned changed=0,smallErrors=0,largeErrors=0;for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
                float dx=(float(x)+.5f)/w-.5f,dy=(float(y)+.5f)/h-.5f;float r=diagnostic::FfrOvalDistanceSquared(dx*2,dy*2);
                if(r>.6f*.6f&&r<.85f*.85f){changed+=narrowOval[y*w+x]!=wideOval[y*w+x];smallErrors+=narrowOval[y*w+x]!=baseline[y*w+x];largeErrors+=wideOval[y*w+x]!=baseline[y*w+x];}}
            std::printf("cycle%u mode%d diameter40->95 annulus changed=%u coarse=%u native=%u\n",cycle,mode,changed,smallErrors,largeErrors);
            Need(changed>10000&&smallErrors>10000&&largeErrors==0,"Live diameter did not enlarge the actual GPU native-detail oval");
            ffr.Configure(w,h,mode,95);ffr.BeginEye(0);Need(Draw(native.Get(),w,h,true)==wideOval,"Unchanged settings modified the rate map");
            ffr.Configure(w,h,mode,40);ffr.BeginEye(1);Need(Draw(native.Get(),w,h,true)==narrowOval,"Repeated live changes did not restore the original detail oval");
        }
        ffr.Configure(w,h,2,60);ffr.BeginEye(0);auto landscape=Draw(native.Get(),w,h,true);
        unsigned horizontalErrors=0,verticalChanges=0;
        for(unsigned y=252;y<260;++y)for(unsigned x=414;x<422;++x){horizontalErrors+=landscape[y*w+x]!=baseline[y*w+x];verticalChanges+=landscape[x*w+y]!=baseline[x*w+y];}
        Need(horizontalErrors==0&&verticalChanges>16,"GPU detail region is not a landscape oval");
        ffr.BeginEye(1);auto ui=Draw(native.Get(),w,h,false);ffr.EndEye();Need(ui==baseline,"Depth-disabled HUD/postprocess received FFR");ffr.Reset();Need(Draw(native.Get(),w,h,true)==baseline,"FFR teardown did not restore native rendering");
    }
    std::puts("PASS actual DXVK FFR: native center, reduced peripheral shading, both eye scopes, UI bypass, repeated teardown");return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
