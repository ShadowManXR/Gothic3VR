#include "NativeWriteback.h"
#include "StaticScene.h"
#include "NativeStaticTrial.h"
#include "NativeTrialBinding.h"
#include "NativeMaterialCapture.h"
#include "../../../src/DxvkGpuBridge.h"
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace g3vr::dx11;
using namespace DirectX;
using Microsoft::WRL::ComPtr;
namespace {
void Need(bool value,char const* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT hr,char const* text){if(FAILED(hr))throw std::runtime_error(std::string(text)+" HRESULT="+std::to_string(unsigned(hr)));}
template<class F>void Reject(F&& fn,char const* message){bool rejected=false;try{fn();}catch(std::exception const&){rejected=true;}Need(rejected,message);}
struct Window {HWND value=CreateWindowExW(0,L"STATIC",L"Isolated DX11 native writeback test",WS_OVERLAPPEDWINDOW,0,0,800,600,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(value)DestroyWindow(value);}};
struct Library {HMODULE value=nullptr;~Library(){if(value)FreeLibrary(value);}};
struct Target {
    unsigned w=1280,h=720;
    ComPtr<ID3D11Texture2D> color,depth,readColor,readDepth;
    ComPtr<ID3D11RenderTargetView> rtv;ComPtr<ID3D11DepthStencilView> dsv;ComPtr<ID3D11ShaderResourceView> srv;
    void Create(ID3D11Device* d){D3D11_TEXTURE2D_DESC t{};t.Width=w;t.Height=h;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;t.BindFlags=D3D11_BIND_RENDER_TARGET;
        Check(d->CreateTexture2D(&t,nullptr,&color),"Producer color");D3D11_RENDER_TARGET_VIEW_DESC rt{};rt.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;rt.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;Check(d->CreateRenderTargetView(color.Get(),&rt,&rtv),"sRGB RTV");
        t.BindFlags=0;t.Usage=D3D11_USAGE_STAGING;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Check(d->CreateTexture2D(&t,nullptr,&readColor),"Diagnostic color readback");
        t.Usage=D3D11_USAGE_DEFAULT;t.CPUAccessFlags=0;t.Format=DXGI_FORMAT_R32_TYPELESS;t.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;Check(d->CreateTexture2D(&t,nullptr,&depth),"Producer depth");
        D3D11_DEPTH_STENCIL_VIEW_DESC ds{};ds.Format=DXGI_FORMAT_D32_FLOAT;ds.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;Check(d->CreateDepthStencilView(depth.Get(),&ds,&dsv),"Depth DSV");
        D3D11_SHADER_RESOURCE_VIEW_DESC sr{};sr.Format=DXGI_FORMAT_R32_FLOAT;sr.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;sr.Texture2D.MipLevels=1;Check(d->CreateShaderResourceView(depth.Get(),&sr,&srv),"Depth SRV");
        t.BindFlags=0;t.Usage=D3D11_USAGE_STAGING;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Check(d->CreateTexture2D(&t,nullptr,&readDepth),"Diagnostic depth readback");}
    template<class T>std::vector<T> Read(ID3D11DeviceContext* c,ID3D11Texture2D* source,ID3D11Texture2D* staging){c->OMSetRenderTargets(0,nullptr,nullptr);c->CopyResource(staging,source);D3D11_MAPPED_SUBRESOURCE map{};Check(c->Map(staging,0,D3D11_MAP_READ,0,&map),"Diagnostic map");std::vector<T> values(w*h);for(unsigned y=0;y<h;++y)memcpy(values.data()+y*w,static_cast<char*>(map.pData)+y*map.RowPitch,w*4);c->Unmap(staging,0);return values;}
};
void Bitmap(std::filesystem::path const& path,std::vector<uint32_t> const& pixels,unsigned w,unsigned h){BITMAPFILEHEADER head{};head.bfType=0x4d42;head.bfOffBits=sizeof(head)+sizeof(BITMAPINFOHEADER);head.bfSize=head.bfOffBits+w*h*4;BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=LONG(w);info.biHeight=-LONG(h);info.biPlanes=1;info.biBitCount=32;std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<char*>(&head),sizeof(head));file.write(reinterpret_cast<char*>(&info),sizeof(info));file.write(reinterpret_cast<char const*>(pixels.data()),pixels.size()*4);Need(bool(file),"Hybrid image write");}
struct NativeTarget {
    ComPtr<IDirect3DTexture9> texture;ComPtr<IDirect3DSurface9> color,depth,staging;
    void Create(IDirect3DDevice9Ex* d,unsigned w,unsigned h){Check(d->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,nullptr),"Native target texture");Check(texture->GetSurfaceLevel(0,&color),"Native target surface");Check(d->CreateDepthStencilSurface(w,h,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr),"Native depth/stencil");Check(d->CreateOffscreenPlainSurface(w,h,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&staging,nullptr),"Diagnostic native readback");}
    void Bind(IDirect3DDevice9Ex* d){Check(d->SetDepthStencilSurface(nullptr),"Unbind depth");Check(d->SetRenderTarget(0,color.Get()),"Bind native color");Check(d->SetDepthStencilSurface(depth.Get()),"Bind native depth");}
    std::vector<uint32_t> Pixels(IDirect3DDevice9Ex* d,unsigned w,unsigned h){Check(d->GetRenderTargetData(color.Get(),staging.Get()),"Diagnostic native pixels");D3DLOCKED_RECT r{};Check(staging->LockRect(&r,nullptr,D3DLOCK_READONLY),"Native map");std::vector<uint32_t> pixels(w*h);for(unsigned y=0;y<h;++y)memcpy(pixels.data()+y*w,static_cast<char*>(r.pBits)+y*r.Pitch,w*4);Check(staging->UnlockRect(),"Native unmap");return pixels;}
};
struct Vertex9 {float x,y,z,rhw;DWORD color;};
void Rect9(IDirect3DDevice9Ex* d,float l,float t,float r,float b,float z,DWORD color,bool zwrite=true){
    Check(d->SetVertexShader(nullptr),"Fixture VS");Check(d->SetPixelShader(nullptr),"Fixture PS");Check(d->SetTexture(0,nullptr),"Fixture texture");Check(d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE),"Fixture FVF");
    d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,zwrite);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    Vertex9 v[]{{l-.5f,t-.5f,z,1,color},{r-.5f,t-.5f,z,1,color},{l-.5f,b-.5f,z,1,color},{r-.5f,b-.5f,z,1,color}};Check(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(Vertex9)),"Native fixture draw");}
XMFLOAT4X4 Matrix(FXMMATRIX m){XMFLOAT4X4 x;XMStoreFloat4x4(&x,m);return x;}
uint64_t Hash(XMFLOAT4X4 const& m){uint64_t h=14695981039346656037ull;for(auto p=reinterpret_cast<unsigned char const*>(&m),end=p+sizeof(m);p!=end;++p){h^=*p;h*=1099511628211ull;}return h;}
void TestMaterialCapture(IDirect3DDevice9Ex* d,std::filesystem::path const& output){
    ComPtr<IDirect3DStateBlock9> saved;Check(d->CreateStateBlock(D3DSBT_ALL,&saved),"Material fixture state");Check(saved->Capture(),"Material fixture capture");
    char const* vertex="row_major float4x4 matViewProjection:register(c0);float4 VS(float4 p:POSITION):POSITION{return mul(p,matViewProjection);}";char const* pixel="float4 vecConstantColor:register(c0);float4 PS():COLOR0{return vecConstantColor;}";
    ComPtr<ID3DBlob> vsCode,psCode,error;Check(D3DCompile(vertex,strlen(vertex),nullptr,nullptr,nullptr,"VS","vs_3_0",0,0,&vsCode,&error),"Material fixture VS");Check(D3DCompile(pixel,strlen(pixel),nullptr,nullptr,nullptr,"PS","ps_3_0",0,0,&psCode,&error),"Material fixture PS");
    ComPtr<IDirect3DVertexShader9> vs;ComPtr<IDirect3DPixelShader9> ps;Check(d->CreateVertexShader(static_cast<DWORD*>(vsCode->GetBufferPointer()),&vs),"Material fixture native VS");Check(d->CreatePixelShader(static_cast<DWORD*>(psCode->GetBufferPointer()),&ps),"Material fixture native PS");d->SetVertexShader(vs.Get());d->SetPixelShader(ps.Get());
    float vc[1024]{},pc[896]{};for(unsigned i=0;i<1024;++i)vc[i]=float(i)*.25f;for(unsigned i=0;i<896;++i)pc[i]=float(i)*.5f+.25f;Check(d->SetVertexShaderConstantF(0,vc,256),"Material fixture VS constants");Check(d->SetPixelShaderConstantF(0,pc,224),"Material fixture PS constants");d->SetRenderState(D3DRS_COLORWRITEENABLE,7);
    NativeMaterialCapture capture;auto folder=output/L"material-evidence-test";capture.Initialize(folder,1280,720);
    auto token=capture.Begin(0,100,"fixture.xcmsh","fixture.xshmat");capture.Observe(d,1);capture.Observe(d,1);capture.End(token);Need(capture.Count()==1,"Material draw duplicated");Need(std::filesystem::file_size(folder/L"materials.jsonl")==0,"Material hook performed disk I/O");
    token=capture.Begin(1,100,"fixture.xcmsh","fixture.xshmat");auto nested=capture.Begin(2,0,nullptr,nullptr);capture.Observe(d,1);Need(capture.Count()==1,"Nested foreign object inherited material identity");capture.End(nested);capture.Observe(d,1);capture.End(token);Need(capture.Count()==2&&!capture.InScope(),"Material scope restoration/stereo capture failed");
    d->SetRenderState(D3DRS_COLORWRITEENABLE,0);token=capture.Begin(0,101,"fixture.xcmsh","depth.xshmat");capture.Observe(d,1);capture.End(token);Need(capture.Count()==2,"Depth-only pass mislabeled as material color");d->SetRenderState(D3DRS_COLORWRITEENABLE,7);
    for(unsigned i=0;i<32;++i){auto name="material-"+std::to_string(i);token=capture.Begin(0,102,"fixture.xcmsh",name.c_str());capture.Observe(d,1);capture.End(token);}Need(capture.Count()==16,"Material evidence budget not enforced");
    pc[0]=99;d->SetPixelShaderConstantF(0,pc,224);capture.Flush();std::ifstream log(folder/L"materials.jsonl");std::string line;unsigned records=0;while(std::getline(log,line)){Need(line.find("capture_failed")==std::string::npos&&line.find("\"gpu_readback\":false")!=std::string::npos&&line.find("\"pixel_constants_c0_c223\":[0.25,0.75,1.25")!=std::string::npos,"Bound shader/constant snapshot was stale or malformed");++records;}Need(records==16,"Material evidence incomplete");
    unsigned programs=0;for(auto const& f:std::filesystem::directory_iterator(folder))if(f.path().extension()==L".cso"){std::ifstream in(f.path(),std::ios::binary);std::vector<char> data((std::istreambuf_iterator<char>(in)),{});Need((data.size()==vsCode->GetBufferSize()&&!memcmp(data.data(),vsCode->GetBufferPointer(),data.size()))||(data.size()==psCode->GetBufferSize()&&!memcmp(data.data(),psCode->GetBufferPointer(),data.size())),"Captured bytecode differs from the bound original");++programs;}Need(programs==2,"Identical material programs did not share content identity");
    Check(saved->Apply(),"Material fixture restore");
}
void TestDeferredMaterialCapture(IDirect3DDevice9Ex* d,std::filesystem::path const& output){
#ifdef G3DX11_NATIVE_SHADER_DIR
    ComPtr<IDirect3DStateBlock9> saved;Check(d->CreateStateBlock(D3DSBT_ALL,&saved),"Deferred fixture state");Check(saved->Capture(),"Deferred fixture capture");
    std::ifstream file(std::filesystem::path(G3DX11_NATIVE_SHADER_DIR)/L"temporal-vs-da20ec985ed6afb2.cso",std::ios::binary);std::vector<char> code((std::istreambuf_iterator<char>(file)),{});Need(code.size()>8,"Missing preserved native instance program");
    ComPtr<IDirect3DVertexShader9> vs;Check(d->CreateVertexShader(reinterpret_cast<DWORD const*>(code.data()),&vs),"Deferred original VS");
    char const* pixel="float4 vecConstantColor:register(c0);float4 PS():COLOR0{return vecConstantColor;}";ComPtr<ID3DBlob> psCode,error;Check(D3DCompile(pixel,strlen(pixel),nullptr,nullptr,nullptr,"PS","ps_3_0",0,0,&psCode,&error),"Deferred fixture PS");ComPtr<IDirect3DPixelShader9> ps;Check(d->CreatePixelShader(static_cast<DWORD*>(psCode->GetBufferPointer()),&ps),"Deferred native PS");
    Check(d->SetVertexShader(vs.Get()),"Deferred bind VS");Check(d->SetPixelShader(ps.Get()),"Deferred bind PS");Check(d->SetRenderState(D3DRS_COLORWRITEENABLE,7),"Deferred RGB state");
    float constants[1024]{},color[4]{.5f,.25f,.75f,1};for(unsigned row=0;row<17;++row){constants[row*12]=constants[row*12+5]=constants[row*12+10]=1;constants[row*12+3]=1000.f+row*10;constants[row*12+7]=2000;constants[row*12+11]=-3000;}
    float selected[16]{1,0,0,0,0,1,0,0,0,0,1,0,1030,2000,-3000,1};Check(d->SetVertexShaderConstantF(0,constants,256),"Deferred matrices");Check(d->SetPixelShaderConstantF(0,color,1),"Deferred color");
    NativeMaterialCapture capture;auto folder=output/L"deferred-material-evidence-test";capture.Initialize(folder,1280,720);
    auto token=capture.Begin(0,200,"selected.g3mesh","selected.xshmat",selected);
    Check(d->SetStreamSourceFreq(0,D3DSTREAMSOURCE_INDEXEDDATA|2),"Deferred short batch");capture.Observe(d,1);Need(capture.Count()==0,"Stale matrix outside active instance count accepted");
    Check(d->SetStreamSourceFreq(0,D3DSTREAMSOURCE_INDEXEDDATA|4),"Deferred selected batch");capture.Observe(d,1);Need(capture.Count()==1,"Deferred selected matrix not captured");capture.End(token);
    token=capture.Begin(1,200,"selected.g3mesh","selected.xshmat",selected);auto nested=capture.Begin(2,0,nullptr,nullptr);capture.Observe(d,1);Need(capture.Count()==1,"Foreign nested pool inherited selected draw");capture.End(nested);
    constants[3*12+3]+=1;Check(d->SetVertexShaderConstantF(0,constants,256),"Deferred foreign batch");capture.Observe(d,1);Need(capture.Count()==1,"Other entity's lighting accepted");
    constants[3*12+3]-=1;Check(d->SetVertexShaderConstantF(0,constants,256),"Deferred selected restoration");capture.Observe(d,1);Need(capture.Count()==2,"Deferred second eye refused");capture.End(token);
    Need(std::filesystem::file_size(folder/L"materials.jsonl")==0,"Deferred draw hook performed file I/O");capture.Flush();std::ostringstream stats;capture.WriteDiagnostics(stats);Need(stats.str().find("\"selected_matrix_mismatch\":2")!=std::string::npos,"Deferred refusal counters absent");
    std::ifstream evidence(folder/L"materials.jsonl");std::string line;unsigned rows=0;while(std::getline(evidence,line)){Need(line.find("\"deferred_pool\":true,\"matched_instance_row\":3")!=std::string::npos,"Deferred provenance missing");++rows;}Need(rows==2,"Deferred material stereo evidence incomplete");
    Check(saved->Apply(),"Deferred fixture restore");
#else
    std::cout<<"Deferred material fixture not configured; this optional regression was not run.\n";
#endif
}
}
int wmain(int argc,wchar_t** argv){try{
    Need(argc==4,"Usage: G3DX11HybridProbe <assets> <output> native|<dxvk dll>");std::filesystem::path assets=argv[1],output=argv[2];std::filesystem::create_directories(output);bool dxvk=std::wstring(argv[3])!=L"native";
    // Explicit factory loading, owned hidden window; never touches Gothic3.exe.
    Library library;if(dxvk)library.value=LoadLibraryExW(argv[3],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);else library.value=LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);Need(library.value!=nullptr,"Load native API backend");
    auto factory=reinterpret_cast<HRESULT (WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(library.value,"Direct3DCreate9Ex"));Need(factory!=nullptr,"D3D9Ex factory");ComPtr<IDirect3D9Ex> api;Check(factory(D3D_SDK_VERSION,&api),"Create API");Window window;Need(window.value!=nullptr,"Hidden test window");
    D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=1280;p.BackBufferHeight=720;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.BackBufferCount=1;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.hDeviceWindow=window.value;p.Windowed=TRUE;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    ComPtr<IDirect3DDevice9Ex> native;Check(api->CreateDeviceEx(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED|D3DCREATE_FPU_PRESERVE,&p,nullptr,&native),"Owned native D3D9Ex device");
    LUID luid{};Check(api->GetAdapterLUID(0,&luid),"Native LUID");ComPtr<IDXGIFactory1> dxgi;Check(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)),"DXGI factory");ComPtr<IDXGIAdapter1> adapter;
    for(UINT i=0;;++i){ComPtr<IDXGIAdapter1> candidate;Need(dxgi->EnumAdapters1(i,&candidate)!=DXGI_ERROR_NOT_FOUND,"Matching native GPU not found");DXGI_ADAPTER_DESC1 description{};Check(candidate->GetDesc1(&description),"Adapter description");if(description.AdapterLuid.LowPart==luid.LowPart&&description.AdapterLuid.HighPart==luid.HighPart){adapter=candidate;break;}}
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context),"Owned matching D3D11 device");Need(level>=D3D_FEATURE_LEVEL_11_0,"DX11 hardware required");
    Target target;target.Create(device.Get());NativeTarget nt;nt.Create(native.Get(),target.w,target.h);nt.Bind(native.Get());NativeWriteback bridge;bridge.Initialize(native.Get(),device.Get(),context.Get(),target.w,target.h);Need(bridge.UsesDxvk()==dxvk,"Wrong hybrid backend selected");
    TestMaterialCapture(native.Get(),output);
    TestDeferredMaterialCapture(native.Get(),output);
    std::vector<Mesh> meshes;for(auto const& f:std::filesystem::directory_iterator(assets))if(f.path().extension()==L".g3mesh")meshes.push_back(ReadMesh(f.path()));Need(!meshes.empty(),"Real native mesh missing");std::sort(meshes.begin(),meshes.end(),[](auto const& a,auto const& b){return a.resource<b.resource;});auto const& house=meshes[0];
    StaticScene scene;scene.Initialize(device.Get(),context.Get());scene.Upload(house);scene.LoadMaterialPreview(assets/L"materials"/L"preview.g3mat");
    std::array<Camera,2> cameras;for(unsigned e=0;e<2;++e){auto eye=XMVectorSet(e?.032f:-.032f,7,-28,1);cameras[e].viewProjection=Matrix(XMMatrixLookAtLH(eye,XMVectorSet(0,4,0,1),XMVectorSet(0,1,0,0))*XMMatrixPerspectiveFovLH(XMConvertToRadians(65),1280.f/720,.05f,5000));}
    scene.Prepare({{1,house.resource,house.content,Matrix(XMMatrixIdentity())}},cameras);
    unsigned tested=0,occluded=0,background=0,houseOccluded=0;std::vector<double> times;ComPtr<IDirect3DVertexBuffer9> sentinel;Check(native->CreateVertexBuffer(64,0,0,D3DPOOL_DEFAULT,&sentinel,nullptr),"Sentinel stream");
    g3vr::DxvkGpuBridge depthAudit;float maxDepthError=0;unsigned depthBackingFormat=0;if(dxvk)Need(depthAudit.Initialize(native.Get()),"Initialize diagnostic native depth audit");
    ComPtr<IDirect3DTexture9> sentinelTexture;Check(native->CreateTexture(4,4,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&sentinelTexture,nullptr),"Sentinel texture");
    char const* psSource="float4 main():COLOR0{return float4(1,0,1,1);}";ComPtr<ID3DBlob> psCode;Check(D3DCompile(psSource,strlen(psSource),nullptr,nullptr,nullptr,"main","ps_3_0",0,0,&psCode,nullptr),"Sentinel shader compile");ComPtr<IDirect3DPixelShader9> sentinelPS;Check(native->CreatePixelShader(static_cast<DWORD const*>(psCode->GetBufferPointer()),&sentinelPS),"Sentinel pixel shader");
    for(unsigned iteration=0;iteration<8;++iteration){unsigned eye=iteration%2;scene.Prepare({{1,house.resource,house.content,Matrix(XMMatrixTranslation(float(iteration)*.17f,0,0))}},cameras);float clear[]{.01f,.02f,.03f,1};context->ClearRenderTargetView(target.rtv.Get(),clear);context->ClearDepthStencilView(target.dsv.Get(),D3D11_CLEAR_DEPTH,1,0);scene.Render(eye,{target.rtv.Get(),target.dsv.Get(),1280,720});
        auto colors=target.Read<uint32_t>(context.Get(),target.color.Get(),target.readColor.Get());auto depths=target.Read<float>(context.Get(),target.depth.Get(),target.readDepth.Get());
        PassStamp stamp{iteration+1,7,Hash(cameras[eye].viewProjection),eye};auto begin=std::chrono::steady_clock::now();bridge.Transfer(target.color.Get(),target.srv.Get(),stamp);auto end=std::chrono::steady_clock::now();if(iteration>=2)times.push_back(std::chrono::duration<double,std::milli>(end-begin).count());
        // Wrong frame, eye, resource epoch and projection all refuse before draw.
        auto bad=stamp;bad.eye^=1;Reject([&]{bridge.Composite(bad);},"Wrong eye accepted");bad=stamp;++bad.frame;Reject([&]{bridge.Composite(bad);},"Stale frame accepted");bad=stamp;++bad.epoch;Reject([&]{bridge.Composite(bad);},"Wrong epoch accepted");bad=stamp;++bad.camera;Reject([&]{bridge.Composite(bad);},"Wrong projection accepted");
        nt.Bind(native.Get());Check(native->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0xff182838,1,0x5a),"Native frame clear");Check(native->BeginScene(),"Begin fixture");
        // Existing native foreground must survive composite. Later native green
        // geometry behind the house must fail Z; it fills clear background only.
        Rect9(native.Get(),0,0,640,720,.2f,0xffe04030);
        Check(native->SetStreamSource(0,sentinel.Get(),4,16),"Sentinel stream bind");Check(native->SetRenderState(D3DRS_STENCILENABLE,TRUE),"Sentinel stencil state");Check(native->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_ALWAYS),"Sentinel stencil func");Check(native->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_KEEP),"Sentinel stencil op");Check(native->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_MIRROR),"Sentinel sampler");
        D3DVIEWPORT9 sentinelView{9,7,1200,680,.1f,.9f};Check(native->SetViewport(&sentinelView),"Sentinel viewport");RECT scissor{13,17,800,500};Check(native->SetScissorRect(&scissor),"Sentinel scissor");
        Check(native->SetPixelShader(sentinelPS.Get()),"Bind sentinel PS");for(unsigned i=0;i<2;++i)Check(native->SetTexture(i,sentinelTexture.Get()),"Bind sentinel texture");DWORD savedFVF=0;Check(native->GetFVF(&savedFVF),"Get sentinel FVF");
        bridge.Composite(stamp);Reject([&]{bridge.Composite(stamp);},"Consumed pass reused");DWORD state=0;Check(native->GetRenderState(D3DRS_STENCILENABLE,&state),"Read sentinel state");Need(state==TRUE,"Native render state not restored");Check(native->GetSamplerState(0,D3DSAMP_ADDRESSU,&state),"Read sampler state");Need(state==D3DTADDRESS_MIRROR,"Native sampler not restored");ComPtr<IDirect3DVertexBuffer9> restored;UINT offset=0,stride=0;Check(native->GetStreamSource(0,&restored,&offset,&stride),"Read restored stream");Need(restored.Get()==sentinel.Get()&&offset==4&&stride==16,"Native stream source not restored");D3DVIEWPORT9 vp{};Check(native->GetViewport(&vp),"Read viewport");Need(memcmp(&vp,&sentinelView,sizeof(vp))==0,"Native viewport not restored");RECT sr{};native->GetScissorRect(&sr);Need(memcmp(&sr,&scissor,sizeof(sr))==0,"Native scissor not restored");
        ComPtr<IDirect3DPixelShader9> restoredPS;Check(native->GetPixelShader(&restoredPS),"Get restored shader");Need(restoredPS.Get()==sentinelPS.Get(),"Native pixel shader not restored");for(unsigned i=0;i<2;++i){ComPtr<IDirect3DBaseTexture9> texture;Check(native->GetTexture(i,&texture),"Get restored texture");Need(texture.Get()==sentinelTexture.Get(),"Native texture binding not restored");}Check(native->GetFVF(&state),"Get restored FVF");Need(state==savedFVF,"Native FVF not restored");
        D3DVIEWPORT9 full{0,0,1280,720,0,1};native->SetViewport(&full);native->SetRenderState(D3DRS_STENCILENABLE,FALSE);Rect9(native.Get(),640,0,1280,720,.99999f,0xff30b060);Rect9(native.Get(),700,0,720,720,.1f,0xff3050e0);Check(native->EndScene(),"End fixture");auto result=nt.Pixels(native.Get(),1280,720);
        unsigned objects=0;for(size_t i=0;i<result.size();++i){uint32_t expected;if(i%1280<640){expected=0xffe04030;++occluded;houseOccluded+=depths[i]<1;}else if(i%1280>=700&&i%1280<720){expected=0xff3050e0;}else if(depths[i]<1){expected=colors[i];++objects;}else{expected=0xff30b060;++background;}
            Need(result[i]==expected,"Hybrid color/depth occlusion differs from producer/native reference");++tested;}Need(objects>10000&&houseOccluded>10000,"Native foreground did not overlap the real house");
        if(iteration<2)Bitmap(output/(eye?L"hybrid-house-right.bmp":L"hybrid-house-left.bmp"),result,1280,720);
        if(dxvk&&iteration<2){g3vr::DepthSnapshot snapshot;Need(depthAudit.CaptureDepthSnapshot(nt.depth.Get(),snapshot),"Read native composite depth");Need(snapshot.depth.size()==depths.size(),"Native depth snapshot dimensions");depthBackingFormat=snapshot.vulkanFormat;
            for(size_t i=0;i<depths.size();++i){float expected=i%1280<640?.2f:i%1280>=700&&i%1280<720?.1f:depths[i]<1?depths[i]:.99999f;maxDepthError=std::max(maxDepthError,std::abs(snapshot.depth[i]-expected));}Need(maxDepthError<=2.f/16777215,"Actual native depth differs by more than two D24 steps");}
        // Stencil must survive color/depth composite byte-for-byte. Draw only
        // where sentinel 0x5a remains; full blue proves it was not overwritten.
        Check(native->BeginScene(),"Begin stencil test");native->SetRenderState(D3DRS_STENCILENABLE,TRUE);native->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_EQUAL);native->SetRenderState(D3DRS_STENCILREF,0x5a);native->SetRenderState(D3DRS_STENCILMASK,255);Rect9(native.Get(),0,0,1280,720,0,0xff2040d0,false);Check(native->EndScene(),"End stencil test");auto stencil=nt.Pixels(native.Get(),1280,720);Need(std::all_of(stencil.begin(),stencil.end(),[](uint32_t c){return c==0xff2040d0;}),"Native stencil contents changed");native->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    }
    // Wide normalized-depth range: packed depth must reject a native plane just
    // behind it and accept one just in front. Clear depth=1 must leave it intact.
    unsigned depthCases=0;float solid[]{.2f,.3f,.4f,1};
    for(float z:{0.f,.0001f,.25f,.5f,.9f,.999f,1.f}){
        context->ClearRenderTargetView(target.rtv.Get(),solid);context->ClearDepthStencilView(target.dsv.Get(),D3D11_CLEAR_DEPTH,z,0);auto source=target.Read<uint32_t>(context.Get(),target.color.Get(),target.readColor.Get());PassStamp stamp{100+depthCases,8,1,0};bridge.Transfer(target.color.Get(),target.srv.Get(),stamp);
        nt.Bind(native.Get());Check(native->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff182838,1,0),"Depth precision clear");Check(native->BeginScene(),"Depth precision begin");bridge.Composite(stamp);
        Rect9(native.Get(),0,0,640,720,std::min(1.f,z+.00001f),0xff20e040);Rect9(native.Get(),640,0,1280,720,std::max(0.f,z-.00001f),0xffe02040);Check(native->EndScene(),"Depth precision end");auto pixels=nt.Pixels(native.Get(),1280,720);
        for(size_t i=0;i<pixels.size();++i)Need(pixels[i]==(i%1280>=640?0xffe02040:z==1?0xff20e040:source[i]),"GPU depth pack/unpack failed close-plane precision test");++depthCases;
    }
    // Consecutive producer -> consumer work without diagnostic maps/readbacks
    // between frames, alternating eyes and changing contents every frame.
    for(unsigned frame=0;frame<24;++frame){float color[]{float(frame)/24,.2f,.6f,1};context->ClearRenderTargetView(target.rtv.Get(),color);context->ClearDepthStencilView(target.dsv.Get(),D3D11_CLEAR_DEPTH,.5f,0);PassStamp stamp{200+frame,9,2,frame%2};bridge.Transfer(target.color.Get(),target.srv.Get(),stamp);nt.Bind(native.Get());Check(native->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0),"Burst clear");Check(native->BeginScene(),"Burst begin");bridge.Composite(stamp);Check(native->EndScene(),"Burst end");}
    auto lastSource=target.Read<uint32_t>(context.Get(),target.color.Get(),target.readColor.Get());Need(lastSource==nt.Pixels(native.Get(),1280,720),"Consecutive GPU-only frames retained stale producer contents");
    PassStamp invalidated{300,10,3,0};bridge.Transfer(target.color.Get(),target.srv.Get(),invalidated);Reject([&]{bridge.Transfer(target.color.Get(),target.srv.Get(),{});},"Invalid pass accepted");Reject([&]{bridge.Composite(invalidated);},"Failed transfer retained an earlier ready pass");
    // Exercise the exact live-trial class using the same copied POD/camera API
    // as the game hooks; no mocked graphics and a separate owned DX11 device.
    unsigned trialEyes=0;
    for(unsigned mode:{1u,2u}){NativeStaticTrial trial;trial.Initialize(native.Get(),adapter.Get(),assets,output/(mode==1?L"trial-offscreen":L"trial-composite"),1280,720,mode);
        for(unsigned eye=0;eye<2;++eye){trial.BeginEye(eye,1000,1);Need(trial.WantsContext(0x1234),"Live trial did not request native context");NativeStaticSnapshot snapshot{};snapshot.identity[0]=77;snapshot.worldMeters[0]=snapshot.worldMeters[5]=snapshot.worldMeters[10]=snapshot.worldMeters[15]=1;
            auto resource=house.resource;resource.replace(resource.size()-7,7,".xcmsh");strcpy_s(snapshot.resource,resource.c_str());XMFLOAT4X4 view,proj;XMStoreFloat4x4(&view,XMMatrixLookAtLH(XMVectorSet(eye?3.2f:-3.2f,700,-2800,1),XMVectorSet(0,400,0,1),XMVectorSet(0,1,0,0)));XMStoreFloat4x4(&proj,XMMatrixPerspectiveFovLH(XMConvertToRadians(65),1280.f/720,5,500000));
            if(eye==0){auto behindTown=snapshot;behindTown.identity[0]=88;behindTown.worldMeters[12]=1000;trial.Observe(0x9876,behindTown,reinterpret_cast<float*>(&view),reinterpret_cast<float*>(&proj));Need(trial.WantsContext(0x1234),"Offscreen context latched before central candidate");}
            trial.Observe(0x1234,snapshot,reinterpret_cast<float*>(&view),reinterpret_cast<float*>(&proj));
            auto converted=NativeRasterCameraMeters(reinterpret_cast<float*>(&view),reinterpret_cast<float*>(&proj),1280,720);scene.Prepare({{1,house.resource,house.content,Matrix(XMMatrixIdentity())}},{converted,converted});context->ClearDepthStencilView(target.dsv.Get(),D3D11_CLEAR_DEPTH,1,0);scene.Render(eye,{target.rtv.Get(),target.dsv.Get(),1280,720});auto reference=target.Read<uint32_t>(context.Get(),target.color.Get(),target.readColor.Get());auto z=target.Read<float>(context.Get(),target.depth.Get(),target.readDepth.Get());
            nt.Bind(native.Get());native->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff182838,1,0);Check(native->BeginScene(),"Live trial begin");auto wrong=proj;wrong._43*=2;trial.SolidBoundary(reinterpret_cast<float*>(&view),reinterpret_cast<float*>(&wrong));Check(native->EndScene(),"Wrong pass trial end");auto untouched=nt.Pixels(native.Get(),1280,720);Need(std::all_of(untouched.begin(),untouched.end(),[](uint32_t p){return p==0xff182838;}),"Trial drew in an unrelated depth pass");
            Check(native->BeginScene(),"Matching trial begin");trial.SolidBoundary(reinterpret_cast<float*>(&view),reinterpret_cast<float*>(&proj));Check(native->EndScene(),"Matching trial end");Need(!trial.WantsContext(0x9876),"Trial did not lock the resolved central candidate");
            Need(trial.WantsMaterial(0x1234)&&!trial.WantsMaterial(0x9876),"Material capture was not restricted to the selected live object");
            auto materialScope=trial.BeginMaterial(0x1234,"fixture.xshmat");Need(materialScope!=UINT_MAX,"Selected material scope refused");auto foreignScope=trial.BeginMaterial(0x9876,nullptr);Need(foreignScope!=UINT_MAX,"Foreign nested render did not mask the outer material");trial.EndMaterial(foreignScope);trial.EndMaterial(materialScope);
            Need(trial.WantsMaterialEntity(snapshot.identity),"Deferred selected entity not requested");auto foreignId=snapshot.identity;foreignId[0]++;
            Need(!trial.WantsMaterialEntity(foreignId)&&trial.BeginDeferredMaterial(foreignId,snapshot.worldMeters.data(),"fixture.xshmat")==UINT_MAX,"Foreign queued entity accepted");
            auto deferredScope=trial.BeginDeferredMaterial(snapshot.identity,snapshot.worldMeters.data(),"fixture.xshmat");Need(deferredScope!=UINT_MAX,"Selected deferred entity refused");auto foreignDeferred=trial.BeginDeferredMaterial(foreignId,nullptr,nullptr);Need(foreignDeferred!=UINT_MAX,"Nested queued batch did not mask selected scope");trial.EndMaterial(foreignDeferred);trial.EndMaterial(deferredScope);
            trial.EndEye();auto result=nt.Pixels(native.Get(),1280,720);
            Need(!trial.WantsMaterialEntity(snapshot.identity),"Deferred material entity leaked outside eye");
            unsigned coverage=0;for(size_t i=0;i<result.size();++i){bool housePixel=mode==2&&z[i]<1;Need(result[i]==(housePixel?reference[i]:0xff182838),"Live trial failed native snapshot/camera composition");coverage+=housePixel;}
            if(mode==2)Need(coverage>20000,"Live trial house not rendered");++trialEyes;
        }
        trial.BeginEye(0,1001,0);Need(!trial.WantsContext(0x1234),"Menu did not disable native trial");trial.EndEye();trial.Reset();
        auto folder=output/(mode==1?L"trial-offscreen":L"trial-composite");
        if(mode==2){for(unsigned eye=0;eye<2;++eye)for(auto suffix:{"-native-before.bmp","-composited-after.bmp","-dx11-preview.bmp","-dx11-coverage.bmp"})Need(std::filesystem::file_size(folder/("eye-"+std::to_string(eye)+suffix))>1000,"Automatic trial image missing");
            std::ifstream captureLog(folder/L"native-static-trial.jsonl");std::string line;unsigned captures=0;while(std::getline(captureLog,line)){Need(line.find("capture_unavailable")==std::string::npos,"Automatic trial capture failed");captures+=line.find("\"type\":\"dx11_static_trial_capture\"")!=std::string::npos;}Need(captures==2,"Automatic stereo capture incomplete");}
        else Need(!std::filesystem::exists(folder/L"eye-0-native-before.bmp"),"Offscreen mode performed native readback");
    }
    // User's actual rendering resolutions are much larger than the house test.
    // Check one 4K pass per eye for transport cost/correctness, not an FPS claim.
    Target large;large.w=3840;large.h=2160;large.Create(device.Get());NativeTarget largeNative;largeNative.Create(native.Get(),large.w,large.h);largeNative.Bind(native.Get());
    bridge.Initialize(native.Get(),device.Get(),context.Get(),large.w,large.h);Reject([&]{bridge.Transfer(target.color.Get(),target.srv.Get(),{400,11,4,0});},"Wrong-size producer accepted");
    std::vector<double> largeTimes;for(unsigned frame=0;frame<8;++frame){float color[]{.3f,float(frame)/8,.2f,1};context->ClearRenderTargetView(large.rtv.Get(),color);context->ClearDepthStencilView(large.dsv.Get(),D3D11_CLEAR_DEPTH,.5f,0);PassStamp stamp{500+frame,12,5,frame%2};auto begin=std::chrono::steady_clock::now();bridge.Transfer(large.color.Get(),large.srv.Get(),stamp);auto end=std::chrono::steady_clock::now();if(frame>=2)largeTimes.push_back(std::chrono::duration<double,std::milli>(end-begin).count());largeNative.Bind(native.Get());Check(native->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0),"4K clear");Check(native->BeginScene(),"4K begin");bridge.Composite(stamp);Check(native->EndScene(),"4K end");}
    Need(large.Read<uint32_t>(context.Get(),large.color.Get(),large.readColor.Get())==largeNative.Pixels(native.Get(),large.w,large.h),"4K writeback changed pixels");std::sort(largeTimes.begin(),largeTimes.end());
    std::sort(times.begin(),times.end());std::ofstream report(output/L"results.json");report<<"{\n \"result\":\"PASS\",\n \"backend\":\""<<(dxvk?"DXVK":"D3D9Ex")<<"\",\n \"real_ardea_mesh\":\""<<house.resource<<"\",\n \"frames\":8,\n \"gpu_burst_frames\":24,\n \"depth_precision_cases\":"<<depthCases<<",\n \"pixels_checked\":"<<tested<<",\n \"native_foreground_pixels\":"<<occluded<<",\n \"house_pixels_occluded_by_native\":"<<houseOccluded<<",\n \"preserved_background_pixels\":"<<background<<",\n \"color_depth_transfer_median_ms\":"<<times[times.size()/2]<<",\n \"gpu_only_transfer\":true,\n \"stencil_preserved\":true,\n \"state_restored\":true,\n \"game_draws_replaced\":false,\n \"native_material_parity\":false,\n \"headset_test\":false\n}\n";Need(bool(report),"Hybrid report write");
    report.close();std::ofstream cost(output/L"4k-transfer.json");cost<<"{\"width\":3840,\"height\":2160,\"result\":\"PASS\",\"frames\":8,\"transfer_median_ms_per_eye\":"<<largeTimes[largeTimes.size()/2]<<",\"includes_depth_pack\":true,\"includes_scene_render\":false,\"includes_native_composite_draw\":false,\"game_fps_claim\":false}\n";Need(bool(cost),"4K transfer report write");
    if(dxvk){std::ofstream audit(output/L"native-depth-audit.json");audit<<"{\"result\":\"PASS\",\"stereo_pixels_checked\":1843200,\"max_absolute_depth_error\":"<<maxDepthError<<",\"allowed_D24_steps\":2,\"vulkan_depth_format\":"<<depthBackingFormat<<",\"diagnostic_readback_only\":true}\n";Need(bool(audit),"Depth audit report write");}
    std::ofstream trialReport(output/L"native-trial-test.json");trialReport<<"{\"result\":\"PASS\",\"real_class_and_GPU\":true,\"tested_eyes\":"<<trialEyes<<",\"modes\":[1,2],\"wrong_pass_refused\":true,\"menus_disabled\":true,\"game_hook_tested\":false}\n";Need(bool(trialReport),"Trial test report write");
    context->ClearState();context->Flush();bridge.Reset();std::cout<<"PASS hybrid real house, native foreground/background depth, stereo/pass ownership, stencil and render state; transfer median "<<times[times.size()/2]<<" ms; 4K "<<largeTimes[largeTimes.size()/2]<<" ms per eye (isolated, not game FPS)\n";return 0;
}catch(std::exception const& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
