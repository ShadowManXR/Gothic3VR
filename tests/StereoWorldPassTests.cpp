#include "StereoWorldPass.h"
#include <wrl/client.h>
#include <stdexcept>
#include <cstdio>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("GPU setup failed");}
int main(){try{
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=DefWindowProcW;wc.lpszClassName=L"G3WorldBoundaryTest";RegisterClassW(&wc);
    HWND window=CreateWindowW(wc.lpszClassName,L"World/auxiliary boundary test",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,wc.hInstance,nullptr);
    ComPtr<IDirect3D9Ex> api;Check(Direct3DCreate9Ex(D3D_SDK_VERSION,&api));D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.hDeviceWindow=window;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    ComPtr<IDirect3DDevice9Ex> d;Check(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&d));
    ComPtr<IDirect3DSurface9> back,preview,hdr;Check(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back));
    Check(d->CreateRenderTarget(64,64,D3DFMT_A16B16G16R16F,D3DMULTISAMPLE_NONE,0,FALSE,&hdr,nullptr));
    Check(d->CreateRenderTarget(128,128,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&preview,nullptr));
    D3DVIEWPORT9 full{0,0,64,64,0,1},auxiliary{0,0,8,8,0,1},offset{1,0,63,64,0,1};
    Check(d->SetRenderTarget(0,back.Get()));Check(d->SetViewport(&full));if(!diagnostic::InspectStereoWorldPass(d.Get()).IsWorld())throw std::runtime_error("Main world rejected");
    Check(d->SetViewport(&auxiliary));Check(d->SetRenderState(D3DRS_COLORWRITEENABLE,0));auto aux=diagnostic::InspectStereoWorldPass(d.Get());
    D3DVIEWPORT9 actual{};DWORD mask=15;Check(d->GetViewport(&actual));Check(d->GetRenderState(D3DRS_COLORWRITEENABLE,&mask));
    if(aux.kind!=diagnostic::StereoWorldPass::Kind::AuxiliaryViewport||actual.Width!=8||actual.Height!=8||mask!=0)throw std::runtime_error("Auxiliary depth pass accepted or modified");
    Check(d->SetViewport(&offset));if(diagnostic::InspectStereoWorldPass(d.Get()).IsWorld())throw std::runtime_error("Offset preview accepted");
    Check(d->SetRenderTarget(0,hdr.Get()));Check(d->SetViewport(&full));if(!diagnostic::InspectStereoWorldPass(d.Get()).IsWorld())throw std::runtime_error("Full-size HDR world rejected");
    Check(d->SetRenderTarget(0,preview.Get()));Check(d->SetViewport(&full));auto p=diagnostic::InspectStereoWorldPass(d.Get());if(p.kind!=diagnostic::StereoWorldPass::Kind::AuxiliaryTarget)throw std::runtime_error("Mismatched preview target accepted");
    ComPtr<IDirect3DSurface9> unchanged;Check(d->GetRenderTarget(0,&unchanged));if(unchanged.Get()!=preview.Get())throw std::runtime_error("Inspection changed native target");
    if(diagnostic::InspectStereoWorldPass(nullptr).kind!=diagnostic::StereoWorldPass::Kind::Unavailable)throw std::runtime_error("Unavailable device accepted");
    Check(d->SetRenderTarget(0,back.Get()));Check(d->SetViewport(&full));if(!diagnostic::InspectStereoWorldPass(d.Get()).IsWorld())throw std::runtime_error("Native world did not resume after preview");
    DestroyWindow(window);std::puts("PASS: real D3D9 world vs auxiliary viewport/target, HDR accepted, native states unchanged, normal world resumes");return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
