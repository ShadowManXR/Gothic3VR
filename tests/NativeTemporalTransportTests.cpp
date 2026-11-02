// Offline native-driver GPU test. Readback is ground truth in this executable
// only; neither production bridge nor GameTemporalInputs maps CPU pixels.
#include "NativeTemporalBridge.h"
#include "GameTemporalInputs.h"
#include "NativeTemporalFixturePS.h"
#include <DirectXPackedVector.h>
#include <dxgi.h>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include <string>
using Microsoft::WRL::ComPtr;
using namespace DirectX::PackedVector;
void Require(bool v,char const* why){if(!v)throw std::runtime_error(why);}
void Hr(HRESULT hr,char const* why){if(FAILED(hr)){std::printf("HRESULT=%08lx %s\n",unsigned long(hr),why);throw std::runtime_error(why);}}
template<class F>void Reject(F f){bool bad=false;try{f();}catch(std::exception const&){bad=true;}Require(bad,"Invalid native transport operation accepted");}
struct Module{HMODULE value=nullptr;~Module(){if(value)FreeLibrary(value);}};
struct Window{HWND value=CreateWindowW(L"STATIC",L"Offline native temporal test",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(value)DestroyWindow(value);}};
std::array<float,4> Color(unsigned eye,unsigned phase,unsigned quad,unsigned layer){return {-3.25f-float(eye*3+phase*2+quad+layer)*.5f,5.5f+float(phase+quad+layer)*.25f,1,1};}
float Depth(unsigned eye,unsigned phase,unsigned quad){return .1f+eye*.1f+phase*.05f+quad*.13f;}
void Draw(IDirect3DDevice9Ex* d,IDirect3DPixelShader9* ps,IDirect3DSurface9* color,IDirect3DSurface9* depth,unsigned w,unsigned h,unsigned eye,unsigned phase,unsigned layer,bool empty=false){
    Hr(d->SetDepthStencilSurface(nullptr),"unbind source depth");Hr(d->SetRenderTarget(0,color),"source target");Hr(d->SetDepthStencilSurface(depth),"source depth");
    d->SetVertexShader(nullptr);d->SetPixelShader(ps);d->SetFVF(D3DFVF_XYZRHW);
    d->SetRenderState(D3DRS_ZENABLE,depth?TRUE:FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
    d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    for(unsigned quad=0;quad<4;++quad){unsigned x=quad%2,y=quad/2;float left=float(x?w/2:0)-.5f,right=float(x?w:w/2)-.5f,top=float(y?h/2:0)-.5f,bottom=float(y?h:h/2)-.5f;
        float z=Depth(eye,phase,quad);struct V{float x,y,z,rhw;};V vertices[]={{left,top,z,1},{right,top,z,1},{left,bottom,z,1},{right,bottom,z,1}};
        auto c=empty?std::array<float,4>{}:Color(eye,phase,quad,layer);Hr(d->SetPixelShaderConstantF(0,c.data(),1),"fixture constants");Hr(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V)),"fixture original draw");}
}
template<class F>void Read(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Texture2D* texture,F inspect){
    D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);desc.BindFlags=desc.MiscFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;Hr(device->CreateTexture2D(&desc,nullptr,&staging),"test D3D11 staging");context->CopyResource(staging.Get(),texture);D3D11_MAPPED_SUBRESOURCE map{};
    Hr(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"test D3D11 readback");
    try{for(unsigned y=0;y<desc.Height;++y)for(unsigned x=0;x<desc.Width;++x)inspect(static_cast<char*>(map.pData)+y*map.RowPitch,x,y);}catch(...){context->Unmap(staging.Get(),0);throw;}
    context->Unmap(staging.Get(),0);
}
void Verify(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Texture2D* texture,unsigned w,unsigned h,unsigned eye,unsigned phase,unsigned layer,bool depth,bool d16){
    Read(d,c,texture,[&](char* row,unsigned x,unsigned y){auto quad=(x>=w/2?1:0)+(y>=h/2?2:0);
        if(depth){auto got=reinterpret_cast<float*>(row)[x];Require(std::isfinite(got)&&std::abs(got-Depth(eye,phase,quad))<(d16?1.1f/65535.f:2.e-7f),"Native drawn depth -> D3D11 differs/stale/flipped");}
        else{auto expected=Color(eye,phase,quad,layer);auto* got=reinterpret_cast<uint16_t*>(row)+x*4;for(unsigned k=0;k<4;++k)Require(got[k]==XMConvertFloatToHalf(expected[k]),"Signed native RGBA16F -> D3D11 differs/stale/flipped");}});
}
void Fence11(ID3D11Device* d,ID3D11DeviceContext* c){D3D11_QUERY_DESC desc{D3D11_QUERY_EVENT,0};ComPtr<ID3D11Query> query;Hr(d->CreateQuery(&desc,&query),"test read fence");c->End(query.Get());c->Flush();auto start=GetTickCount64();for(;;){BOOL done=FALSE;auto hr=c->GetData(query.Get(),&done,sizeof(done),D3D11_ASYNC_GETDATA_DONOTFLUSH);Hr(hr,"test read completion");if(hr==S_OK&&done)return;Require(GetTickCount64()-start<2000,"test read timeout");SwitchToThread();}}
void StatesUnchanged(IDirect3DDevice9Ex* d,IDirect3DSurface9* color,IDirect3DSurface9* depth,IDirect3DPixelShader9* ps,unsigned w,unsigned h){
    ComPtr<IDirect3DSurface9> rt,ds;ComPtr<IDirect3DPixelShader9> shader;D3DVIEWPORT9 viewport{};DWORD fvf=0,write=0,func=0;
    Hr(d->GetRenderTarget(0,&rt),"inspect target");Hr(d->GetDepthStencilSurface(&ds),"inspect depth");Hr(d->GetPixelShader(&shader),"inspect shader");d->GetViewport(&viewport);d->GetFVF(&fvf);d->GetRenderState(D3DRS_COLORWRITEENABLE,&write);d->GetRenderState(D3DRS_ZFUNC,&func);
    Require(rt.Get()==color&&ds.Get()==depth&&shader.Get()==ps&&viewport.Width==w&&viewport.Height==h&&fvf==D3DFVF_XYZRHW&&write==15&&func==D3DCMP_ALWAYS,"Native temporal copy changed caller pipeline state");
}
int wmain(int argc,wchar_t** argv){try{
    unsigned w=argc>1?_wtoi(argv[1]):97,h=argc>2?_wtoi(argv[2]):65;Require(w&&h&&w<=3840&&h<=2160,"Fixture dimensions");
    wchar_t system[MAX_PATH]{};auto n=GetSystemDirectoryW(system,MAX_PATH);Require(n&&n<MAX_PATH,"system DLL directory");Module library;library.value=LoadLibraryW((std::wstring(system)+L"\\d3d9.dll").c_str());Require(library.value!=nullptr,"native system D3D9");
    auto create=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(library.value,"Direct3DCreate9Ex"));Require(create!=nullptr,"native D3D9Ex entry");Window window;Require(window.value!=nullptr,"hidden test window");
    ComPtr<IDirect3D9Ex> api;Hr(create(D3D_SDK_VERSION,&api),"native D3D9Ex");D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.value;pp.BackBufferWidth=w;pp.BackBufferHeight=h;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    ComPtr<IDirect3DDevice9Ex> native;Hr(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&native),"native D3D9 device");
    LUID luid{};Hr(api->GetAdapterLUID(0,&luid),"native LUID");ComPtr<IDXGIFactory1> factory;Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");ComPtr<IDXGIAdapter1> adapter;
    for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> candidate;if(factory->EnumAdapters1(i,&candidate)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 desc{};Hr(candidate->GetDesc1(&desc),"DXGI desc");if(!memcmp(&luid,&desc.AdapterLuid,sizeof(luid))){adapter=candidate;break;}}Require(adapter!=nullptr,"matching D3D11 adapter");
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context),"D3D11 device");
    ComPtr<IDirect3DPixelShader9> ps;Hr(native->CreatePixelShader(reinterpret_cast<DWORD const*>(NativeTemporalFixturePS),&ps),"fixture pixel shader");ComPtr<IDirect3DSurface9> original;Hr(native->GetRenderTarget(0,&original),"original target");
    unsigned cases=0;
    for(auto format:{D3DFMT_D16,D3DFMT_D24S8})for(bool discard:{false,true}){
        ComPtr<IDirect3DSurface9> depth;Hr(native->CreateDepthStencilSurface(w,h,format,D3DMULTISAMPLE_NONE,0,discard,&depth,nullptr),"fixture depth");
        ComPtr<IDirect3DTexture9> textures[3];ComPtr<IDirect3DSurface9> colors[3];for(unsigned i=0;i<3;++i){Hr(native->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&textures[i],nullptr),"fixture motion/late sky texture");Hr(textures[i]->GetSurfaceLevel(0,&colors[i]),"fixture motion/late sky surface");}
        g3vr::NativeTemporalBridge bridge;Require(bridge.Initialize(native.Get()),"Native NVIDIA bridge unavailable");
        ComPtr<ID3D11Texture2D> outputs[2][3];
        for(unsigned eye=0;eye<2;++eye){Require(bridge.Register(depth.Get(),device.Get(),outputs[eye][0])==g3vr::TemporalEncoding::Depth32Float,"Native depth encoding");for(unsigned i=1;i<3;++i)Require(bridge.Register(colors[i-1].Get(),device.Get(),outputs[eye][i])==g3vr::TemporalEncoding::Color16F,"Native motion encoding");}
        ComPtr<ID3D11Texture2D> invalid;Reject([&]{bridge.Register(depth.Get(),device.Get(),invalid);});Require(outputs[0][0]!=outputs[1][0],"Shared native depth aliased eye outputs");
        for(unsigned phase=0;phase<3;++phase){
            for(unsigned eye=0;eye<2;++eye){Hr(native->BeginScene(),"begin actual scene");Draw(native.Get(),ps.Get(),colors[0].Get(),nullptr,w,h,eye,phase,0);Draw(native.Get(),ps.Get(),colors[1].Get(),depth.Get(),w,h,eye,phase,1);
                Reject([&]{bridge.Queue(nullptr);});bridge.Queue(outputs[eye][0].Get());Reject([&]{bridge.Queue(outputs[eye][0].Get());});Reject([&]{bridge.Queue(outputs[1-eye][0].Get());});for(unsigned i=1;i<3;++i)bridge.Queue(outputs[eye][i].Get());bridge.Complete();
                StatesUnchanged(native.Get(),colors[1].Get(),depth.Get(),ps.Get(),w,h);Hr(native->EndScene(),"end actual scene");Reject([&]{bridge.Queue(outputs[eye][0].Get());});}
            // Right eye has now overwritten ALL three original native inputs.
            for(unsigned eye=0;eye<2;++eye)for(unsigned layer=0;layer<3;++layer)Verify(device.Get(),context.Get(),outputs[eye][layer].Get(),w,h,eye,phase,layer?layer-1:0,layer==0,format==D3DFMT_D16);
            Fence11(device.Get(),context.Get());bridge.ReleaseReads();bridge.ReleaseReads();
        }
        bridge.Reset();bridge.Reset();
        // Actual game provider, including NVIDIA fallback and compute decode /
        // composition. Never invent valid motion for unknown coverage.
        g3vr::GameTemporalInputs live;live.Initialize(native.Get(),device.Get(),context.Get(),w,h);std::array<float,16> identity{};for(unsigned i=0;i<4;++i)identity[i*5]=1;
        Reject([&]{live.Finalize(0,{1,7,1011111111ull,0},nullptr,false);});
        for(unsigned frame=1;frame<=3;++frame){for(unsigned eye=0;eye<2;++eye){Hr(native->BeginScene(),"live scene");Draw(native.Get(),ps.Get(),colors[0].Get(),nullptr,w,h,eye,0,0,true);Draw(native.Get(),ps.Get(),colors[1].Get(),depth.Get(),w,h,eye,0,1,frame==3);
                diagnostic::temporal::GeometryFrameStamp stamp{frame,7,1000000000ull+frame*11111111ull,eye};auto result=live.Capture(eye,stamp,identity.data(),identity.data(),depth.Get(),colors[0].Get(),colors[1].Get());Require(result.cameraHistory==(frame>1),"Native live eye history");
                StatesUnchanged(native.Get(),colors[1].Get(),depth.Get(),ps.Get(),w,h);Hr(native->EndScene(),"live scene end");}
            for(unsigned eye=0;eye<2;++eye){Verify(device.Get(),context.Get(),live.Depth(eye),w,h,eye,0,0,true,format==D3DFMT_D16);
                Read(device.Get(),context.Get(),live.Motion(eye),[&](char* row,unsigned x,unsigned y){auto* packed=reinterpret_cast<uint16_t*>(row)+x*4;float got[4];for(unsigned k=0;k<4;++k)got[k]=XMConvertHalfToFloat(packed[k]);if(frame==3){for(unsigned k=0;k<4;++k)Require(got[k]==0,"Unknown native pixels granted motion history");}
                    else{Require(got[2]==1&&got[3]==float(frame>1),"Native live motion flags");auto expected=Color(eye,0,(x>=w/2?1:0)+(y>=h/2?2:0),1);for(unsigned k=0;k<2;++k)Require(got[k]==(frame>1?XMConvertHalfToFloat(XMConvertFloatToHalf(expected[k])):0),"Native live motion values");}});}
            // End-of-eye phase: sky source is reused across eyes, while the
            // opaque snapshots are already private. A is reactivity in this
            // late source, then becomes geometric history in combined output.
            for(unsigned eye=0;eye<2;++eye){diagnostic::temporal::GeometryFrameStamp stamp{frame,7,1000000000ull+frame*11111111ull,eye};
                Require(!live.Finalized(eye,frame)&&!live.Reactive(eye),"Opaque capture was falsely reported finalized");auto mismatched=stamp;++mismatched.frame;Reject([&]{live.Finalize(eye,mismatched,nullptr,false);});
                if(frame>1){Hr(native->BeginScene(),"late sky fixture begin");Draw(native.Get(),ps.Get(),colors[2].Get(),nullptr,w,h,eye,0,2);Hr(native->EndScene(),"late sky fixture end");}
                live.Finalize(eye,stamp,frame>1?colors[2].Get():nullptr,frame>1);Require(live.Finalized(eye,frame)&&live.Reactive(eye),"Late GPU outputs unavailable");Reject([&]{live.Finalize(eye,stamp,nullptr,false);});
            }
            for(unsigned eye=0;eye<2;++eye){
                Read(device.Get(),context.Get(),live.Motion(eye),[&](char* row,unsigned x,unsigned y){auto* p=reinterpret_cast<uint16_t*>(row)+x*4;Require(XMConvertHalfToFloat(p[2])==1&&XMConvertHalfToFloat(p[3])==float(frame==3),"Late geometric occupancy/history flags");auto expected=Color(eye,0,(x>=w/2?1:0)+(y>=h/2?2:0),2);for(unsigned k=0;k<2;++k)Require(p[k]==XMConvertFloatToHalf(frame==3?expected[k]:0),"Late motion overwritten by next eye or conflict trusted");});
                Read(device.Get(),context.Get(),live.Reactive(eye),[&](char* row,unsigned x,unsigned){Require(static_cast<unsigned char>(row[x])==255,"Sky reactivity was mistaken for history validity");});
            }
        }
        live.Reset();live.Reset();Require(!live.Depth(0)&&!live.Motion(1),"Native live reset retained outputs");
        native->SetDepthStencilSurface(nullptr);native->SetRenderTarget(0,original.Get());++cases;std::printf("PASS %ux%u depth=%u discard=%u: drawn depth, signed motion x2, aliased L/R, 3 phases, state, provider, unknown history\n",w,h,format,discard);
    }
    native->SetPixelShader(nullptr);ps.Reset();original.Reset();context->ClearState();Fence11(device.Get(),context.Get());auto* pointer=native.Detach();Require(pointer->Release()==0,"Native temporal resources/device references leaked");
    std::printf("PASS %u native NVIDIA GPU transport/provider cases; no game or headset started, no scene-completeness / NGX claim\n",cases);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
