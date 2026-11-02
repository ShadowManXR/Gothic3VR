#include "NativeOcclusion.h"
#include <wrl/client.h>
#include <stdexcept>
#include <cstdio>
using Microsoft::WRL::ComPtr;
using namespace diagnostic::occlusion;
void Require(bool ok,char const* what){if(!ok)throw std::runtime_error(what);}
void Hr(HRESULT hr,char const* what){if(FAILED(hr)){printf("HRESULT %08lx\n",hr);throw std::runtime_error(what);}}
struct Module {HMODULE handle=nullptr;~Module(){if(handle)FreeLibrary(handle);}};
struct Window {HWND handle=CreateWindowW(L"STATIC",L"Occlusion GPU fixture",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(handle)DestroyWindow(handle);}};
void Draw(IDirect3DDevice9* device,IDirect3DQuery9* query,bool hidden=false){
    struct V {float x,y,z,rhw;DWORD color;};
    V vertices[]={{-.5f,-.5f,.5f,1,0xffffffff},{63.5f,-.5f,.5f,1,0xffffffff},{-.5f,63.5f,.5f,1,0xffffffff},{63.5f,63.5f,.5f,1,0xffffffff}};
    Hr(device->SetRenderState(D3DRS_ZENABLE,FALSE),"Z off");
    Hr(device->SetRenderState(D3DRS_LIGHTING,FALSE),"Lighting off");
    Hr(device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE),"Cull off");
    Hr(device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE),"Blend off");
    Hr(device->SetRenderState(D3DRS_SCISSORTESTENABLE,hidden),"Scissor enable");
    RECT scissor{0,0,0,0};Hr(device->SetScissorRect(&scissor),"Scissor rect");
    Hr(device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE),"FVF");
    Hr(device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1),"Color op");
    Hr(device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE),"Color arg");
    Hr(device->BeginScene(),"Begin scene");Hr(query->Issue(D3DISSUE_BEGIN),"Begin query");
    Hr(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V)),"Draw query quad");
    Hr(query->Issue(D3DISSUE_END),"End query");Hr(device->EndScene(),"End scene");
}
void Complete(IDirect3DDevice9* device){
    ComPtr<IDirect3DQuery9> fence;Hr(device->CreateQuery(D3DQUERYTYPE_EVENT,&fence),"Test-only event fence");
    Hr(fence->Issue(D3DISSUE_END),"Test-only fence issue");
    auto deadline=GetTickCount64()+5000;HRESULT hr=S_FALSE;
    while((hr=fence->GetData(nullptr,0,D3DGETDATA_FLUSH))==S_FALSE&&GetTickCount64()<deadline)SwitchToThread();
    Require(hr==S_OK,"GPU fixture timed out");
}
int wmain(int argc,wchar_t** argv){try{
    Require(argc==2,"Expected D3D9 DLL path or native");Module module;
    if(!wcscmp(argv[1],L"native")){wchar_t path[MAX_PATH]{};Require(GetSystemDirectoryW(path,MAX_PATH)>0,"System directory");wcscat_s(path,L"\\d3d9.dll");module.handle=LoadLibraryW(path);}
    else module.handle=LoadLibraryW(argv[1]);
    Require(module.handle!=nullptr,"Load D3D9 runtime");
    auto create=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(module.handle,"Direct3DCreate9Ex"));Require(create!=nullptr,"D3D9Ex factory");
    ComPtr<IDirect3D9Ex> d3d;Hr(create(D3D_SDK_VERSION,&d3d),"D3D9Ex");Window window;Require(window.handle!=nullptr,"Hidden fixture window");
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.hDeviceWindow=window.handle;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    ComPtr<IDirect3DDevice9Ex> device;Hr(d3d->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.handle,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&device),"Create fixture device");
    ComPtr<IDirect3DQuery9> query;Hr(device->CreateQuery(D3DQUERYTYPE_OCCLUSION,&query),"Occlusion query");
    unsigned pending=0,ready=0;DWORD pixels=0;
    // Reissue the same native-pool-style object without waiting for its prior
    // result. The application deliberately discards that prior result.
    for(unsigned i=0;i<256;++i){Draw(device.Get(),query.Get());auto state=Poll(query.Get(),pixels);
        Require(state==Result::Ready||state==Result::Conservative,"Pending reuse failed");
        if(state==Result::Conservative){Require(pixels==ConservativeVisible,"Wrong conservative result");++pending;}else{Require(pixels==4096,"Wrong ready query pixels");++ready;}}
    Complete(device.Get());Require(Poll(query.Get(),pixels)==Result::Ready&&pixels==4096,"Reissued query lost its final result");
    Draw(device.Get(),query.Get(),true);Complete(device.Get());Require(Poll(query.Get(),pixels)==Result::Ready&&pixels==0,"Ready occluded result was changed");
    // Same object can be used again after a zero result and then after reuse.
    Draw(device.Get(),query.Get());Complete(device.Get());Require(Poll(query.Get(),pixels)==Result::Ready&&pixels==4096,"Query pool reuse after ready zero failed");
    printf("PASS real GPU occlusion reuse=256 ready=%u pending=%u final_visible_pixels=%lu ready_zero_preserved=1; no game/headset\n",ready,pending,pixels);
    return 0;
}catch(std::exception const& e){printf("FAIL %s\n",e.what());return 1;}}
