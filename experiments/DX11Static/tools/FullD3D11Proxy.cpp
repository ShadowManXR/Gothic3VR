// Opt-in whole-API trial: forwards the complete wrapper export set. The only
// adaptation is GPU sharing for explicitly requested VR render targets.
#include <windows.h>
#include <d3d9.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <array>
#include <algorithm>
#include <intrin.h>
#include <memory>
#include <new>
#include <cstring>
using Microsoft::WRL::ComPtr;
namespace {
INIT_ONCE once=INIT_ONCE_STATIC_INIT;HMODULE backend=nullptr;
using GetProc=FARPROC(WINAPI*)(HMODULE,LPCSTR);
using Create11=HRESULT(WINAPI*)(IDXGIAdapter*,D3D_DRIVER_TYPE,HMODULE,UINT,D3D_FEATURE_LEVEL const*,UINT,UINT,ID3D11Device**,D3D_FEATURE_LEVEL*,ID3D11DeviceContext**);
using CreateSwap11=HRESULT(WINAPI*)(IDXGIAdapter*,D3D_DRIVER_TYPE,HMODULE,UINT,D3D_FEATURE_LEVEL const*,UINT,UINT,DXGI_SWAP_CHAIN_DESC const*,IDXGISwapChain**,ID3D11Device**,D3D_FEATURE_LEVEL*,ID3D11DeviceContext**);
using CreateTexture11=HRESULT(STDMETHODCALLTYPE*)(ID3D11Device*,D3D11_TEXTURE2D_DESC const*,D3D11_SUBRESOURCE_DATA const*,ID3D11Texture2D**);
GetProc realGetProc=&GetProcAddress;Create11 realCreate=nullptr;CreateSwap11 realCreateSwap=nullptr;
// Deliberately pinned for the life of the trial process, never unloaded while
// native wrapper objects can still call its callbacks.
ID3D11Device* capturedDevice=nullptr;
unsigned diagnostics[8]{};
bool FromBackend(void* address){if(!backend)return false;auto* base=reinterpret_cast<unsigned char*>(backend);auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);return reinterpret_cast<uintptr_t>(address)>=reinterpret_cast<uintptr_t>(base)&&reinterpret_cast<uintptr_t>(address)-reinterpret_cast<uintptr_t>(base)<nt->OptionalHeader.SizeOfImage;}
thread_local bool backendCreating=false;
void Remember(ID3D11Device* device){if(device&&device!=capturedDevice){device->AddRef();auto* previous=capturedDevice;capturedDevice=device;if(previous)previous->Release();diagnostics[0]=1;}}
HRESULT WINAPI CreateDevice(IDXGIAdapter* a,D3D_DRIVER_TYPE t,HMODULE s,UINT flags,D3D_FEATURE_LEVEL const* levels,UINT n,UINT sdk,ID3D11Device** d,D3D_FEATURE_LEVEL* level,ID3D11DeviceContext** c){
    ++diagnostics[1];bool native=backendCreating||FromBackend(_ReturnAddress());auto hr=realCreate(a,t,s,flags,levels,n,sdk,d,level,c);if(native&&SUCCEEDED(hr)&&d)Remember(*d);return hr;
}
HRESULT WINAPI CreateSwap(IDXGIAdapter* a,D3D_DRIVER_TYPE t,HMODULE s,UINT flags,D3D_FEATURE_LEVEL const* levels,UINT n,UINT sdk,DXGI_SWAP_CHAIN_DESC const* desc,IDXGISwapChain** swap,ID3D11Device** d,D3D_FEATURE_LEVEL* level,ID3D11DeviceContext** c){
    ++diagnostics[2];bool native=backendCreating||FromBackend(_ReturnAddress()),previous=backendCreating;backendCreating=native;auto hr=realCreateSwap(a,t,s,flags,levels,n,sdk,desc,swap,d,level,c);backendCreating=previous;if(native&&SUCCEEDED(hr)&&d)Remember(*d);return hr;
}
bool SetCell(void** cell,void* value){DWORD protect=0;if(!VirtualProtect(cell,sizeof(void*),PAGE_READWRITE,&protect))return false;InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(cell),value);DWORD unused;return VirtualProtect(cell,sizeof(void*),protect,&unused)!=FALSE;}
void* HookCreate(FARPROC entry,void* replacement){
    // Verified local x86 hotpatch prologue: mov edi,edi; push ebp; mov ebp,esp.
    // All five bytes are complete position-independent instructions.
    unsigned char const prefix[]{0x8b,0xff,0x55,0x8b,0xec};if(!entry||memcmp(reinterpret_cast<void*>(entry),prefix,5))return nullptr;
    auto* trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,10,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!trampoline)return nullptr;
    memcpy(trampoline,reinterpret_cast<void*>(entry),5);trampoline[5]=0xe9;int32_t back=int32_t(reinterpret_cast<uintptr_t>(entry)+5-reinterpret_cast<uintptr_t>(trampoline+10));memcpy(trampoline+6,&back,4);
    DWORD old=0;if(!VirtualProtect(trampoline,10,PAGE_EXECUTE_READ,&old))return nullptr;FlushInstructionCache(GetCurrentProcess(),trampoline,10);
    unsigned char patch[5]{0xe9};int32_t jump=int32_t(reinterpret_cast<uintptr_t>(replacement)-reinterpret_cast<uintptr_t>(entry)-5);memcpy(patch+1,&jump,4);
    if(!VirtualProtect(reinterpret_cast<void*>(entry),5,PAGE_EXECUTE_READWRITE,&old))return nullptr;
    memcpy(reinterpret_cast<void*>(entry),patch,5);DWORD unused;VirtualProtect(reinterpret_cast<void*>(entry),5,old,&unused);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(entry),5);return trampoline;
}
BOOL CALLBACK LoadBackend(PINIT_ONCE,void*,void**){
    wchar_t path[32768]{};auto n=GetEnvironmentVariableW(L"G3VR_D3D11_WRAPPER_DLL",path,32768);if(!n||n>=32768)return FALSE;
    HMODULE pinned=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<wchar_t const*>(&LoadBackend),&pinned);
    auto d11=LoadLibraryExW(L"d3d11.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!d11)return FALSE;
    realCreate=reinterpret_cast<Create11>(HookCreate(realGetProc(d11,"D3D11CreateDevice"),reinterpret_cast<void*>(&CreateDevice)));
    realCreateSwap=reinterpret_cast<CreateSwap11>(HookCreate(realGetProc(d11,"D3D11CreateDeviceAndSwapChain"),reinterpret_cast<void*>(&CreateSwap)));
    if(!realCreate||!realCreateSwap)return FALSE;
    backend=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!backend)return FALSE;
    return TRUE;
}
char const* names[]={"D3DPERF_BeginEvent","D3DPERF_EndEvent","D3DPERF_GetStatus","D3DPERF_QueryRepeatFrame","D3DPERF_SetMarker","D3DPERF_SetOptions","D3DPERF_SetRegion","DebugSetMute","Direct3DCreate9","Direct3DCreate9Ex","Direct3DShaderValidatorCreate9"};
struct Request {unsigned width=0,height=0,count=0;ID3D11Device* device=nullptr;ComPtr<ID3D11Texture2D> texture;HRESULT error=S_OK;};
thread_local Request* request=nullptr;CreateTexture11 realTexture=nullptr;
struct Eye {ComPtr<ID3D11Texture2D> source,target;ComPtr<ID3D11ShaderResourceView> srv;ComPtr<ID3D11RenderTargetView> rtv;ComPtr<ID3D11CommandList> conversion;ComPtr<ID3D11DeviceContext> immediate;};
std::array<std::unique_ptr<Eye>,8> eyes;SRWLOCK eyeLock=SRWLOCK_INIT;
HRESULT Conversion(ID3D11Device* device,ID3D11Texture2D* source,std::unique_ptr<Eye>& eye){
    eye.reset(new(std::nothrow) Eye);if(!eye)return E_OUTOFMEMORY;eye->source=source;D3D11_TEXTURE2D_DESC d{};source->GetDesc(&d);
    D3D11_SHADER_RESOURCE_VIEW_DESC sd{};sd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;sd.Texture2D.MipLevels=1;
    sd.Format=d.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS||d.Format==DXGI_FORMAT_R8G8B8A8_UNORM?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_B8G8R8A8_UNORM;
    auto hr=device->CreateShaderResourceView(source,&sd,&eye->srv);if(FAILED(hr))return hr;
    d.Format=DXGI_FORMAT_B8G8R8A8_UNORM;d.MiscFlags=d.CPUAccessFlags=0;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;d.Usage=D3D11_USAGE_DEFAULT;
    hr=device->CreateTexture2D(&d,nullptr,&eye->target);if(FAILED(hr))return hr;hr=device->CreateRenderTargetView(eye->target.Get(),nullptr,&eye->rtv);if(FAILED(hr))return hr;
    char const* shader="Texture2D<float4> image:register(t0);float4 VS(uint id:SV_VertexID):SV_POSITION{float2 uv=float2((id<<1)&2,id&2);return float4(uv*float2(2,-2)+float2(-1,1),0,1);}float4 PS(float4 p:SV_POSITION):SV_Target{return image.Load(int3(int2(p.xy),0));}";
    ComPtr<ID3DBlob> vertex,pixel,error;hr=D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vertex,&error);if(FAILED(hr))return hr;
    hr=D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&pixel,&error);if(FAILED(hr))return hr;
    ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;hr=device->CreateVertexShader(vertex->GetBufferPointer(),vertex->GetBufferSize(),nullptr,&vs);if(FAILED(hr))return hr;
    hr=device->CreatePixelShader(pixel->GetBufferPointer(),pixel->GetBufferSize(),nullptr,&ps);if(FAILED(hr))return hr;
    ComPtr<ID3D11DeviceContext> deferred;hr=device->CreateDeferredContext(0,&deferred);if(FAILED(hr))return hr;
    deferred->ClearState();ID3D11RenderTargetView* target=eye->rtv.Get();deferred->OMSetRenderTargets(1,&target,nullptr);
    D3D11_VIEWPORT viewport{0,0,float(d.Width),float(d.Height),0,1};deferred->RSSetViewports(1,&viewport);deferred->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deferred->VSSetShader(vs.Get(),nullptr,0);deferred->PSSetShader(ps.Get(),nullptr,0);ID3D11ShaderResourceView* input=eye->srv.Get();deferred->PSSetShaderResources(0,1,&input);deferred->Draw(3,0);deferred->ClearState();
    hr=deferred->FinishCommandList(FALSE,&eye->conversion);if(FAILED(hr))return hr;device->GetImmediateContext(&eye->immediate);return S_OK;
}
HRESULT STDMETHODCALLTYPE CreateTexture(ID3D11Device* self,D3D11_TEXTURE2D_DESC const* input,D3D11_SUBRESOURCE_DATA const* data,ID3D11Texture2D** output){
    if(request&&input){++diagnostics[3];diagnostics[4]=unsigned(input->Format);diagnostics[6]=(self==request->device?1:0);}
    auto* r=request;bool selected=r&&self==r->device&&input&&input->Width==r->width&&input->Height==r->height&&input->ArraySize==1&&input->MipLevels==1&&input->SampleDesc.Count==1&&(input->BindFlags&D3D11_BIND_RENDER_TARGET)&&
        (input->Format==DXGI_FORMAT_B8G8R8A8_UNORM||input->Format==DXGI_FORMAT_R8G8B8A8_UNORM||input->Format==DXGI_FORMAT_R8G8B8A8_TYPELESS||input->Format==DXGI_FORMAT_B8G8R8A8_TYPELESS);
    if(!selected)return realTexture(self,input,data,output);
    ++r->count;auto desc=*input;desc.MiscFlags|=D3D11_RESOURCE_MISC_SHARED;desc.BindFlags|=D3D11_BIND_SHADER_RESOURCE;
    auto hr=realTexture(self,&desc,data,output);r->error=hr;if(SUCCEEDED(hr)&&output)r->texture=*output;return hr;
}
void EndRequest(){if(request){request=nullptr;auto** table=*reinterpret_cast<void***>(capturedDevice);SetCell(&table[5],reinterpret_cast<void*>(realTexture));}}
}
extern "C" void WINAPI G3VR_WrapperDiagnostics(unsigned* output){if(output)std::memcpy(output,diagnostics,sizeof(diagnostics));}
extern "C" HRESULT WINAPI G3VR_UpdateWrapperEye(ID3D11Texture2D* target){
    AcquireSRWLockShared(&eyeLock);for(auto const& eye:eyes)if(eye&&eye->target.Get()==target){eye->immediate->ExecuteCommandList(eye->conversion.Get(),TRUE);ReleaseSRWLockShared(&eyeLock);return S_OK;}ReleaseSRWLockShared(&eyeLock);return S_FALSE;
}
extern "C" void WINAPI G3VR_ReleaseWrapperEye(ID3D11Texture2D* target){
    AcquireSRWLockExclusive(&eyeLock);for(auto& eye:eyes)if(eye&&eye->target.Get()==target){eye.reset();break;}ReleaseSRWLockExclusive(&eyeLock);
}
extern "C" FARPROC __cdecl G3WrapperResolve(unsigned index){
    if(index>=std::size(names)||!InitOnceExecuteOnce(&once,&LoadBackend,nullptr,nullptr))TerminateProcess(GetCurrentProcess(),0x47334431);
    auto result=realGetProc(backend,names[index]);if(!result)TerminateProcess(GetCurrentProcess(),0x47334432);return result;
}
#define FORWARD(name,index) extern "C" __declspec(naked) void __cdecl G3Forward_##name(){__asm{push ecx} __asm{push edx} __asm{push index} __asm{call G3WrapperResolve} __asm{add esp,4} __asm{pop edx} __asm{pop ecx} __asm{jmp eax}}
FORWARD(D3DPERF_BeginEvent,0) FORWARD(D3DPERF_EndEvent,1) FORWARD(D3DPERF_GetStatus,2) FORWARD(D3DPERF_QueryRepeatFrame,3)
FORWARD(D3DPERF_SetMarker,4) FORWARD(D3DPERF_SetOptions,5) FORWARD(D3DPERF_SetRegion,6) FORWARD(DebugSetMute,7)
FORWARD(Direct3DCreate9,8) FORWARD(Direct3DCreate9Ex,9) FORWARD(Direct3DShaderValidatorCreate9,10)

// Mirrors the existing SharedEye contract. No CPU frame transport. Creation is
// exceptional setup work; the CreateTexture2D patch is removed before returning.
extern "C" HRESULT WINAPI G3VR_CreateWrapperSharedEye(IDirect3DDevice9Ex* native,ID3D11Device* consumer,UINT width,UINT height,IDirect3DTexture9** texture9,IDirect3DSurface9** surface9,ID3D11Texture2D** texture11){
    if(!native||!consumer||!width||!height||!texture9||!surface9||!texture11)return E_INVALIDARG;
    *texture9=nullptr;*surface9=nullptr;*texture11=nullptr;if(!capturedDevice||request)return E_NOINTERFACE;
    auto** table=*reinterpret_cast<void***>(capturedDevice);auto** cell=&table[5];realTexture=reinterpret_cast<CreateTexture11>(*cell);
    Request capture;capture.width=width;capture.height=height;capture.device=capturedDevice;
    struct Restore{void** cell;void* function;~Restore(){EndRequest();}} restore{cell,*cell};
    if(!SetCell(cell,reinterpret_cast<void*>(&CreateTexture)))return E_FAIL;request=&capture;
    ComPtr<IDirect3DTexture9> t9;auto hr=native->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&t9,nullptr);if(FAILED(hr))return hr;
    ComPtr<IDirect3DSurface9> s9;hr=t9->GetSurfaceLevel(0,&s9);if(FAILED(hr))return hr;
    // Most wrappers allocate the real image lazily. Force one harmless private
    // clear, then restore every native target plus its viewport.
    ComPtr<IDirect3DSurface9> saved[4],depth;D3DVIEWPORT9 viewport{};D3DCAPS9 caps{};
    hr=native->GetDeviceCaps(&caps);if(FAILED(hr))return hr;unsigned count=std::min(4u,unsigned(caps.NumSimultaneousRTs));
    for(unsigned i=0;i<count;++i){hr=native->GetRenderTarget(i,&saved[i]);if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)return hr;}
    hr=native->GetDepthStencilSurface(&depth);if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)return hr;
    hr=native->GetViewport(&viewport);if(FAILED(hr))return hr;
    struct Targets{IDirect3DDevice9Ex* d;ComPtr<IDirect3DSurface9>* rt;IDirect3DSurface9* z;unsigned count;D3DVIEWPORT9 vp;
        ~Targets(){EndRequest();d->SetDepthStencilSurface(nullptr);for(unsigned i=1;i<count;++i)d->SetRenderTarget(i,nullptr);for(unsigned i=0;i<count;++i)d->SetRenderTarget(i,rt[i].Get());d->SetDepthStencilSurface(z);d->SetViewport(&vp);}
    } targets{native,saved,depth.Get(),count,viewport};
    hr=native->SetDepthStencilSurface(nullptr);if(FAILED(hr))return hr;
    for(unsigned i=1;i<count;++i){hr=native->SetRenderTarget(i,nullptr);if(FAILED(hr))return hr;}
    hr=native->SetRenderTarget(0,s9.Get());if(FAILED(hr))return hr;
    hr=native->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);if(FAILED(hr))return hr;
    EndRequest();
    if(FAILED(capture.error))return capture.error;
    diagnostics[5]=capture.count;if(capture.count!=1||!capture.texture)return E_NOINTERFACE;
    ComPtr<IDXGIResource> resource;hr=capture.texture.As(&resource);if(FAILED(hr))return hr;
    HANDLE shared=nullptr;hr=resource->GetSharedHandle(&shared);if(FAILED(hr)||!shared)return FAILED(hr)?hr:E_FAIL;
    ComPtr<ID3D11Texture2D> opened;hr=consumer->OpenSharedResource(shared,IID_PPV_ARGS(&opened));if(FAILED(hr))return hr;
    std::unique_ptr<Eye> eye;hr=Conversion(consumer,opened.Get(),eye);if(FAILED(hr))return hr;
    AcquireSRWLockExclusive(&eyeLock);Eye* published=nullptr;for(auto& slot:eyes)if(!slot){slot=std::move(eye);published=slot.get();break;}ReleaseSRWLockExclusive(&eyeLock);if(!published)return E_OUTOFMEMORY;
    published->target.CopyTo(texture11);*texture9=t9.Detach();*surface9=s9.Detach();return S_OK;
}
