#include <windows.h>
#include <d3d9.h>
#include "D3D9Bridge.h"
#include "D3D9Reticle.h"
#include <cstdio>
int main(int argc,char** argv)
{
    UINT width=argc==3?UINT(std::atoi(argv[1])):64,height=argc==3?UINT(std::atoi(argv[2])):64;
    if(!width||!height||width>5120||height>4096)return 1;
    try{WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=DefWindowProcW;wc.lpszClassName=L"G3BridgeTest";RegisterClassW(&wc);
        HWND window=CreateWindowW(wc.lpszClassName,L"GPU interop test",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,wc.hInstance,nullptr);
        if(!window)throw std::runtime_error("Hidden test window");stereo::ComPtr<IDirect3D9Ex> d3d;stereo::Hr(Direct3DCreate9Ex(D3D_SDK_VERSION,&d3d),"D3D9Ex");
        D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.hDeviceWindow=window;p.BackBufferWidth=width;p.BackBufferHeight=height;p.BackBufferFormat=D3DFMT_A8R8G8B8;
        stereo::ComPtr<IDirect3DDevice9Ex> device;stereo::Hr(d3d->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&p,nullptr,&device),"test D3D9Ex device");
        g3vr::DxvkGpuBridge optional;if(optional.Initialize(device.Get())||optional.Active())throw std::runtime_error("Native D3D9 unexpectedly selected DXVK backend");
        stereo::Renderer r;r.Initialize();g3vr::SharedEye eye;eye.Create(device.Get(),r.device.Get(),width,height,&optional);
        stereo::ComPtr<IDirect3DSurface9> back;stereo::Hr(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back),"test backbuffer");
        D3DSURFACE_DESC actual{};back->GetDesc(&actual);if(actual.Width!=width||actual.Height!=height)throw std::runtime_error("Native backbuffer dimensions do not match request");
        stereo::ComPtr<IDirect3DQuery9> q9;stereo::Hr(device->CreateQuery(D3DQUERYTYPE_EVENT,&q9),"test 9 fence");
        D3D11_QUERY_DESC desc{D3D11_QUERY_EVENT,0};stereo::ComPtr<ID3D11Query> q11;stereo::Hr(r.device->CreateQuery(&desc,&q11),"test 11 fence");
        auto copy=r.Color(width,height,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
        stereo::Hr(device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff000000,1,0),"reticle clear");
        RECT clip{0,0,1,1};device->SetScissorRect(&clip);device->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);device->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_NEVER);device->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
        if(!diagnostic::DrawReticle(device.Get(),.7f,.3f,true))throw std::runtime_error("Reticle outside desktop centre failed to draw");
        DWORD state=0;device->GetRenderState(D3DRS_SCISSORTESTENABLE,&state);if(state!=TRUE)throw std::runtime_error("Reticle did not restore native scissor state");
        device->GetRenderState(D3DRS_ALPHATESTENABLE,&state);if(state!=TRUE)throw std::runtime_error("Reticle did not restore native alpha test");
        device->GetRenderState(D3DRS_FILLMODE,&state);if(state!=D3DFILL_WIREFRAME)throw std::runtime_error("Reticle did not restore native fill mode");
        stereo::Hr(device->StretchRect(back.Get(),nullptr,eye.surface9.Get(),nullptr,D3DTEXF_NONE),"reticle to shared blit");g3vr::Wait9(q9.Get());
        r.context->CopyResource(copy.Get(),eye.texture11.Get());g3vr::Wait11(r.context.Get(),q11.Get());auto reticle=r.Pixels(copy.Get());
        UINT aimX=UINT(width*.7f),aimY=UINT(height*.3f);unsigned painted=0;int radius=int(diagnostic::ReticleRadius(height))+2;
        for(int y=int(aimY)-radius;y<=int(aimY)+radius;++y)for(int x=int(aimX)-radius;x<=int(aimX)+radius;++x){if(x<0||y<0||x>=int(width)||y>=int(height))continue;auto offset=(size_t(y)*width+x)*3;if(reticle[offset+1]>200)++painted;}
        size_t centre=(size_t(height/2)*width+width/2)*3;
        if(painted<8||reticle[centre]!=0||reticle[centre+1]!=0||reticle[centre+2]!=0)throw std::runtime_error("Stereo scene reticle pixels missing or drawn at fixed HUD centre");
        stereo::Hr(device->BeginScene(),"existing scene reticle test");bool insideScene=diagnostic::DrawReticle(device.Get(),.7f,.3f,false);HRESULT endScene=device->EndScene();
        if(!insideScene||FAILED(endScene))throw std::runtime_error("Reticle disturbed the native BeginScene/EndScene pair");
        device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
        for(unsigned color:{0xffdc1e3cu,0xff1496e6u}){stereo::Hr(device->Clear(0,nullptr,D3DCLEAR_TARGET,color,1,0),"test clear");
            D3DRECT corner{LONG(width-1),LONG(height-1),LONG(width),LONG(height)};
            stereo::Hr(device->Clear(1,&corner,D3DCLEAR_TARGET,0xff00ff00,1,0),"native last-pixel detail");
            stereo::Hr(device->StretchRect(back.Get(),nullptr,eye.surface9.Get(),nullptr,D3DTEXF_NONE),"native to shared blit");g3vr::Wait9(q9.Get());
            r.context->CopyResource(copy.Get(),eye.texture11.Get());g3vr::Wait11(r.context.Get(),q11.Get());auto rgb=r.Pixels(copy.Get());
            // Pixels returns the raw B,G,R bytes for this BGRA texture.
            if(rgb[0]!=(color&255)||rgb[1]!=((color>>8)&255)||rgb[2]!=((color>>16)&255))throw std::runtime_error("Shared texture copy/color mismatch");
            size_t last=(size_t(width)*height-1)*3;if(rgb.size()!=size_t(width)*height*3||rgb[last]!=0||rgb[last+1]!=255||rgb[last+2]!=0)throw std::runtime_error("Native last-pixel detail lost in shared transfer");}
        r.context->ClearState();r.context->Flush();eye.Reset();eye.Reset();
        if(eye.texture11||eye.surface9||eye.texture9)throw std::runtime_error("Shared resources retained after shutdown");
        back.Reset();q9.Reset();q11.Reset();copy.Reset();
        // The game destroys its device after our shutdown callback. Any leaked
        // native shared texture, surface or query would keep this device alive.
        auto* native=device.Detach();if(native->Release()!=0)throw std::runtime_error("VR references still retain the native device at game shutdown");
        DestroyWindow(window);puts("D3D9Ex -> D3D11: two colors verified; all VR native references freed before device destruction");return 0;
    }catch(std::exception const& e){fprintf(stderr,"%s\n",e.what());return 1;}
}
