#include "D3D9AnimatedMotionProbe.h"
#include <windows.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include <memory>
#include <sstream>
#include <cstdio>
using Microsoft::WRL::ComPtr;
#define CHECK(x) do{if(!(x)){std::printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Module {HMODULE handle=nullptr;~Module(){if(handle)FreeLibrary(handle);}};
struct Window {HWND handle=CreateWindowW(L"STATIC",L"Temporal animation getter test",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(handle)DestroyWindow(handle);}};
int wmain(int argc,wchar_t** argv){
    CHECK(argc==3);Module module;module.handle=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);CHECK(module.handle);
    auto create=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(module.handle,"Direct3DCreate9Ex"));CHECK(create);
    Window window;CHECK(window.handle);ComPtr<IDirect3D9Ex> api;CHECK(SUCCEEDED(create(D3D_SDK_VERSION,&api)));
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.handle;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    ComPtr<IDirect3DDevice9Ex> device;CHECK(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.handle,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&device)));
    ComPtr<IDirect3DTexture9> target;ComPtr<IDirect3DSurface9> surface,depth;
    CHECK(SUCCEEDED(device->CreateTexture(64,64,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&target,nullptr))&&SUCCEEDED(target->GetSurfaceLevel(0,&surface)));
    CHECK(SUCCEEDED(device->CreateDepthStencilSurface(64,64,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&depth,nullptr)));
    CHECK(SUCCEEDED(device->SetRenderTarget(0,surface.Get()))&&SUCCEEDED(device->SetDepthStencilSurface(depth.Get())));
    D3DVIEWPORT9 viewport{0,0,64,64,0,1};device->SetViewport(&viewport);device->SetRenderState(D3DRS_ZENABLE,TRUE);device->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    std::ifstream input(std::filesystem::path(argv[2]),std::ios::binary|std::ios::ate);CHECK(input);auto bytes=input.tellg();CHECK(bytes>0&&bytes<32768&&bytes%4==0);std::vector<DWORD> code(size_t(bytes)/4);input.seekg(0);input.read(reinterpret_cast<char*>(code.data()),bytes);CHECK(input);
    ComPtr<IDirect3DVertexShader9> shader;CHECK(SUCCEEDED(device->CreateVertexShader(code.data(),&shader))&&SUCCEEDED(device->SetVertexShader(shader.Get())));
    ComPtr<IDirect3DVertexBuffer9> vb,colors,otherColors;ComPtr<IDirect3DIndexBuffer9> ib;ComPtr<IDirect3DVertexDeclaration9> declaration;
    CHECK(SUCCEEDED(device->CreateVertexBuffer(256,0,0,D3DPOOL_DEFAULT,&vb,nullptr))&&SUCCEEDED(device->CreateIndexBuffer(12,0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr)));
    // Exact single-stream 68-byte declaration assembled by the native
    // HardRenderActor::Create at Engine RVA430e0..431d1.
    D3DVERTEXELEMENT9 elements[]={
        {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
        {0,12,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_BLENDWEIGHT,0},
        {0,28,D3DDECLTYPE_UBYTE4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_BLENDINDICES,0},
        {0,32,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_NORMAL,0},
        {0,44,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TANGENT,0},
        {0,56,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,1},
        {0,60,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},
        // Actual live13712 rejection: extra element7, stream1, offset0,
        // D3DCOLOR(type4), COLOR(usage10), index0 from native lighting.
        {1,0,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},D3DDECL_END()};
    CHECK(SUCCEEDED(device->CreateVertexDeclaration(elements,&declaration))&&SUCCEEDED(device->SetVertexDeclaration(declaration.Get()))&&SUCCEEDED(device->SetStreamSource(0,vb.Get(),0,68))&&SUCCEEDED(device->SetIndices(ib.Get())));
    CHECK(SUCCEEDED(device->CreateVertexBuffer(16,0,0,D3DPOOL_DEFAULT,&colors,nullptr))&&SUCCEEDED(device->CreateVertexBuffer(16,0,0,D3DPOOL_DEFAULT,&otherColors,nullptr)));
    CHECK(SUCCEEDED(device->SetStreamSource(1,colors.Get(),0,4)));
    std::array<float,1024> constants{};for(unsigned b=0;b<64;++b)for(unsigned j=0;j<3;++j)constants[b*12+j*4+j]=1;for(unsigned j=0;j<4;++j)constants[192*4+j*4+j]=1;
    CHECK(SUCCEEDED(device->SetVertexShaderConstantF(0,constants.data(),256)));
    auto baseline=device->AddRef();device->Release();
    auto probe=std::make_unique<g3vr::D3D9AnimatedMotionProbe>();probe->width=probe->height=64;
    g3vr::D3D9BufferObserver buffers;probe->UseBufferObserver(&buffers);
    diagnostic::temporal::Object object;object.id[0]=123;object.id[4]=456;object.context=0x1234;object.visual=0x5678;for(unsigned j=0;j<4;++j)object.world[j*4+j]=1;
    diagnostic::temporal::Geometry geometry{4,0,0,3,0,1};
    for(unsigned frame=10;frame<=11;++frame)for(unsigned eye=0;eye<2;++eye){
        buffers.Begin(frame);
        device->SetStreamSource(1,frame==10?colors.Get():otherColors.Get(),0,4);
        device->SetRenderState(D3DRS_ZWRITEENABLE,eye?FALSE:TRUE);
        device->SetStreamSourceFreq(0,frame==10&&!eye?(D3DSTREAMSOURCE_INDEXEDDATA|1):1);
        probe->Begin(eye,frame);probe->Observe(device.Get(),geometry); // No context => no device queries.
        auto token=probe->Push(object);probe->Observe(device.Get(),geometry);
        auto nested=probe->Push({});probe->Observe(device.Get(),geometry);probe->Pop(nested); // Invalid nested context cannot inherit outer entity.
        probe->Observe(device.Get(),geometry);probe->Pop(token);probe->Stop();
        std::ostringstream record;probe->Write(record,eye);auto text=record.str();
        CHECK(text.find("\"scoped_indexed_draws\":2")!=std::string::npos&&text.find("\"observations\":2")!=std::string::npos&&text.find("\"shader_hash\":\"8e4a823574960350\"")!=std::string::npos);
        CHECK(text.find(frame==11?"\"history_status\":\"ready\"":"\"history_status\":\"frame_gap_or_reset\"")!=std::string::npos);
        if(frame==11)CHECK(text.find("\"previous_frame\":10")!=std::string::npos);
        if(eye)CHECK(text.find("\"pass_state\":[0,")!=std::string::npos);
    }
    // A shadow-sized viewport and color-disabled depth pass must not be
    // mistaken for the world color pass even with a matching animation scope.
    probe->Begin(0,12);auto token=probe->Push(object);viewport.Width=viewport.Height=32;device->SetViewport(&viewport);probe->Observe(device.Get(),geometry);
    viewport.Width=viewport.Height=64;device->SetViewport(&viewport);device->SetRenderState(D3DRS_COLORWRITEENABLE,0);probe->Observe(device.Get(),geometry);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    device->SetStreamSourceFreq(0,D3DSTREAMSOURCE_INDEXEDDATA|2);probe->Observe(device.Get(),geometry);device->SetStreamSourceFreq(0,1);
    device->SetDepthStencilSurface(nullptr);probe->Observe(device.Get(),geometry);device->SetDepthStencilSurface(depth.Get());
    ComPtr<IDirect3DVertexDeclaration9> unsupportedDeclaration;D3DVERTEXELEMENT9 unsupportedElements[]={
        {1,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},D3DDECL_END()};
    CHECK(SUCCEEDED(device->CreateVertexDeclaration(unsupportedElements,&unsupportedDeclaration))&&SUCCEEDED(device->SetVertexDeclaration(unsupportedDeclaration.Get())));
    probe->Observe(device.Get(),geometry);device->SetVertexDeclaration(declaration.Get());unsupportedDeclaration.Reset();
    device->SetVertexShader(nullptr);probe->Observe(device.Get(),geometry);device->SetVertexShader(shader.Get());probe->Pop(token);probe->Stop();
    std::ostringstream rejected;probe->Write(rejected,0);CHECK(rejected.str().find("\"draws\":[]")!=std::string::npos&&rejected.str().find("\"rejected_pass_draws\":5")!=std::string::npos&&rejected.str().find("\"unsupported_shader_draws\":1")!=std::string::npos);
    for(auto name:{"viewport_or_render_state","target_or_depth_surface","vertex_stream_or_frequency","multiple_vertex_streams"})CHECK(rejected.str().find(name)!=std::string::npos);
    ComPtr<IDirect3DVertexShader9> actualShader;device->GetVertexShader(&actualShader);CHECK(actualShader.Get()==shader.Get());actualShader.Reset();
    std::array<float,1024> after{};CHECK(SUCCEEDED(device->GetVertexShaderConstantF(0,after.data(),256))&&after==constants);
    DWORD mask=0;device->GetRenderState(D3DRS_COLORWRITEENABLE,&mask);CHECK(mask==15);D3DVIEWPORT9 afterViewport{};device->GetViewport(&afterViewport);CHECK(!memcmp(&viewport,&afterViewport,sizeof(viewport)));
    for(unsigned frame=14;frame<=16;++frame){buffers.Begin(frame);
        if(frame==15){void* data=nullptr;CHECK(SUCCEEDED(vb->Lock(0,0,&data,0)));CHECK(SUCCEEDED(vb->Unlock()));}
        probe->Begin(0,frame);auto scope=probe->Push(object);probe->Observe(device.Get(),geometry);probe->Pop(scope);probe->Stop();
        std::ostringstream record;probe->Write(record,0);CHECK(record.str().find(frame==16?"\"history_status\":\"ready\"":frame==15?"\"history_status\":\"first_observation\"":"\"history_status\":\"frame_gap_or_reset\"")!=std::string::npos);
    }
    buffers.Reset();probe.reset();auto final=device->AddRef();device->Release();CHECK(final==baseline);
    std::puts("PASS real DXVK/captured Gothic shader/native skin declaration plus live stream1 lighting: independent palette histories survive lighting buffer rotation; secondary POSITION rejected; readonly color, frequency, nested scopes, state and lifetime checks; no game motion texture claim");return 0;
}
