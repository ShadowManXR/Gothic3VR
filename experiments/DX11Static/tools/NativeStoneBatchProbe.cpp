#include "NativeStoneFixtures.h"
#include "NativeStoneRenderer.h"
#include "OwnedStonePass.h"
#include "NativeMaterialCapture.h"
#include "NativeWriteback.h"
#include <chrono>
#include <iomanip>
#include "../../../src/TemporalObjectHistory.h"
namespace {
struct Additional {std::vector<char> bytes;unsigned calls=0;bool valid=true;IDirect3DDevice9* alteredDevice=nullptr;IDirect3DPixelShader9* alteredPS=nullptr;
    static bool __cdecl Copy(void* p,void* to,unsigned vertices,unsigned bytes)noexcept{auto& s=*static_cast<Additional*>(p);++s.calls;if(!s.valid||vertices!=2097||bytes!=s.bytes.size())return false;memcpy(to,s.bytes.data(),bytes);if(s.alteredDevice&&FAILED(s.alteredDevice->SetPixelShader(s.alteredPS)))return false;return true;}
};
}
int wmain(int argc,wchar_t** argv)try{
    bool ownedOnly=argc==5&&std::wstring(argv[4])==L"--owned-only";Need(argc==4||argc==6||ownedOnly,"Usage: BatchProbe <profile> <output> <dxvk> [width height|--owned-only]");W=argc==6?unsigned(std::stoul(argv[4])):1280;H=argc==6?unsigned(std::stoul(argv[5])):720;Need(W>=128&&H>=96&&W<=4096&&H<=4096,"Batch resolution budget");std::filesystem::path profile=argv[1],output=argv[2];std::filesystem::create_directories(output);{std::ofstream f(output/L"result.json");f<<"{\"result\":\"INCOMPLETE\"}\n";}
    Library library;library.value=LoadLibraryExW(argv[3],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Need(library.value!=nullptr,"Live backend");auto factory=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(library.value,"Direct3DCreate9Ex"));Need(factory!=nullptr,"Live native factory");ComPtr<IDirect3D9Ex> api;Check(factory(D3D_SDK_VERSION,&api),"Live API");Window window;D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=W;pp.BackBufferHeight=H;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.value;pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;ComPtr<IDirect3DDevice9Ex> d9;Check(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&d9),"Live native device");
    LUID luid{};Check(api->GetAdapterLUID(0,&luid),"Live GPU");ComPtr<IDXGIFactory1> dxgi;Check(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)),"Live factory");ComPtr<IDXGIAdapter1> adapter;for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> a;Need(dxgi->EnumAdapters1(i,&a)!=DXGI_ERROR_NOT_FOUND,"Live GPU mismatch");DXGI_ADAPTER_DESC1 desc{};a->GetDesc1(&desc);if(!memcmp(&desc.AdapterLuid,&luid,sizeof(luid))){adapter=a;break;}}
    ComPtr<ID3D11Device> d11;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d11,&level,&context),"Live owned DX11");
    NativeScene mesh;mesh.Create(d9.Get(),d11.Get(),profile/L"native-scene.bin");auto data=Read(profile/L"native-scene.bin",16*1024*1024);Additional additional;additional.bytes.assign(data.begin()+32+mesh.vertices*48,data.begin()+32+mesh.vertices*72);
    auto originalVS=Read(profile/L"native-vertex.cso",32768),originalPS=Read(profile/L"native.cso",32768);ComPtr<IDirect3DVertexShader9> vs;ComPtr<IDirect3DPixelShader9> ps;Check(d9->CreateVertexShader(reinterpret_cast<DWORD const*>(originalVS.data()),&vs),"Live original VS");Check(d9->CreatePixelShader(reinterpret_cast<DWORD const*>(originalPS.data()),&ps),"Live original PS");
    D3DVERTEXELEMENT9 decl[]={{0,0,2,0,0,0},{0,12,2,0,3,0},{0,24,1,0,5,0},{1,0,4,0,10,0},{1,4,2,0,6,0},{2,0,4,0,10,1},{2,4,2,0,5,7},{2,16,1,0,5,6},D3DDECL_END()};ComPtr<IDirect3DVertexDeclaration9> declaration;Check(d9->CreateVertexDeclaration(decl,&declaration),"Live native declaration");d9->SetVertexDeclaration(declaration.Get());d9->SetVertexShader(vs.Get());d9->SetPixelShader(ps.Get());mesh.Bind(d9.Get(),context.Get());
    std::array<TexturePair,6> textures;for(unsigned i=0;i<6;++i){textures[i]=i==0?Color(d9.Get(),d11.Get(),0,128):i==1||i==2?Shadow(d9.Get(),d11.Get(),i,128):i==3?OriginalLightmap(d9.Get(),d11.Get(),profile/L"lightmap-atlas.bin"):OriginalDds(d9.Get(),d11.Get(),profile/(i==4?L"normal.dds":L"diffuse.dds"));d9->SetTexture(i,textures[i].native.Get());for(auto state:{D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV})d9->SetSamplerState(i,state,i>=1&&i<=3?D3DTADDRESS_CLAMP:D3DTADDRESS_WRAP);d9->SetSamplerState(i,D3DSAMP_ADDRESSW,D3DTADDRESS_WRAP);d9->SetSamplerState(i,D3DSAMP_MAGFILTER,i==0?D3DTEXF_POINT:D3DTEXF_LINEAR);d9->SetSamplerState(i,D3DSAMP_MINFILTER,i==0?D3DTEXF_POINT:D3DTEXF_LINEAR);d9->SetSamplerState(i,D3DSAMP_MIPFILTER,i==1||i==2?D3DTEXF_NONE:i==0?D3DTEXF_POINT:D3DTEXF_LINEAR);d9->SetSamplerState(i,D3DSAMP_SRGBTEXTURE,FALSE);d9->SetSamplerState(i,D3DSAMP_MAXANISOTROPY,8);}
    for(auto state:{D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_FOGENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_STENCILENABLE,D3DRS_CLIPPLANEENABLE,D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS})d9->SetRenderState(state,FALSE);d9->SetRenderState(D3DRS_ZENABLE,TRUE);d9->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d9->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d9->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);d9->SetRenderState(D3DRS_COLORWRITEENABLE,7);
    ComPtr<IDirect3DTexture9> color9;ComPtr<IDirect3DSurface9> colorSurface,depth9,staging9;
    Check(d9->CreateTexture(W,H,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&color9,nullptr),"Batch native color");
    Check(color9->GetSurfaceLevel(0,&colorSurface),"Batch native surface");
    Check(d9->CreateDepthStencilSurface(W,H,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&depth9,nullptr),"Batch native depth");
    Check(d9->CreateOffscreenPlainSurface(W,H,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&staging9,nullptr),"Batch readback");
    Check(d9->SetRenderTarget(0,colorSurface.Get()),"Batch target");Check(d9->SetDepthStencilSurface(depth9.Get()),"Batch depth");
    D3DVIEWPORT9 viewport{0,0,W,H,0,1};d9->SetViewport(&viewport);d9->SetRenderState(D3DRS_CULLMODE,D3DCULL_CW);
    ComPtr<ID3D11Texture2D> color11,depth11;ComPtr<ID3D11RenderTargetView> rtv;ComPtr<ID3D11DepthStencilView> dsv;ComPtr<ID3D11ShaderResourceView> depthView;
    D3D11_TEXTURE2D_DESC td{};td.Width=W;td.Height=H;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    Check(d11->CreateTexture2D(&td,nullptr,&color11),"Batch DX11 color");Check(d11->CreateRenderTargetView(color11.Get(),nullptr,&rtv),"Batch RTV");
    td.Format=DXGI_FORMAT_R32_TYPELESS;td.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;Check(d11->CreateTexture2D(&td,nullptr,&depth11),"Batch DX11 depth");
    D3D11_DEPTH_STENCIL_VIEW_DESC ds{};ds.Format=DXGI_FORMAT_D32_FLOAT;ds.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;Check(d11->CreateDepthStencilView(depth11.Get(),&ds,&dsv),"Batch DSV");
    D3D11_SHADER_RESOURCE_VIEW_DESC sr{};sr.Format=DXGI_FORMAT_R32_FLOAT;sr.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;sr.Texture2D.MipLevels=1;Check(d11->CreateShaderResourceView(depth11.Get(),&sr,&depthView),"Batch depth SRV");
    g3vr::dx11::NativeStoneRenderer renderer;renderer.Initialize(d9.Get(),d11.Get(),context.Get(),profile);
    g3vr::dx11::NativeWriteback writeback;writeback.Initialize(d9.Get(),d11.Get(),context.Get(),W,H);
    g3vr::dx11::NativeStoneRenderer residentRenderer;residentRenderer.Initialize(nullptr,d11.Get(),context.Get(),profile);
    g3vr::dx11::ResidentStoneInputs residentInputs{textures[0].view.Get(),{textures[1].view.Get(),textures[2].view.Get()},textures[3].view.Get()};
    float residentClear[]{8.f/255,16.f/255,24.f/255,1};
    ComPtr<ID3D11Query> complete11;D3D11_QUERY_DESC event11{D3D11_QUERY_EVENT,0};Check(d11->CreateQuery(&event11,&complete11),"Resident completion query");
    auto wait11=[&]{context->End(complete11.Get());context->Flush();auto start=GetTickCount64();for(;;){auto hr=context->GetData(complete11.Get(),nullptr,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);if(hr==S_OK)break;Check(hr,"Resident completion wait");Need(GetTickCount64()-start<5000,"Resident GPU timeout");SwitchToThread();}};
    D3D11_TEXTURE2D_DESC readDesc{};color11->GetDesc(&readDesc);readDesc.BindFlags=0;readDesc.Usage=D3D11_USAGE_STAGING;readDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> read11;Check(d11->CreateTexture2D(&readDesc,nullptr,&read11),"Resident readback texture");
    auto residentPixels=[&]{context->CopyResource(read11.Get(),color11.Get());D3D11_MAPPED_SUBRESOURCE p{};Check(context->Map(read11.Get(),0,D3D11_MAP_READ,0,&p),"Resident readback map");std::vector<uint32_t> result(size_t(W)*H);for(unsigned y=0;y<H;++y)memcpy(result.data()+size_t(y)*W,static_cast<char*>(p.pData)+size_t(y)*p.RowPitch,W*4);context->Unmap(read11.Get(),0);return result;};
    g3vr::dx11::NativeMaterialCapture capture;capture.Initialize(output/L"captured",W,H,true);
    auto vertex=Read(profile/L"vertex-constants.bin",16384),pixel=Read(profile/L"constants.bin",8192);
    Need(vertex.size()==12+2*1024*4&&pixel.size()==12+2*896*4,"Batch constants format");
    std::array<g3vr::dx11::NativeMaterialDraw,2> base;
    for(unsigned eye=0;eye<2;++eye){std::array<float,1024> vc;std::array<float,896> pc;memcpy(vc.data(),vertex.data()+12+eye*1024*4,1024*4);memcpy(pc.data(),pixel.data()+12+eye*896*4,896*4);
        d9->SetVertexShaderConstantF(0,vc.data(),256);d9->SetPixelShaderConstantF(0,pc.data(),224);
        std::array<float,16> world{};world[15]=1;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)world[c*4+r]=vc[(12+r)*4+c];
        auto token=capture.Begin(eye,1,"g3_myrtana_ardea_stairs_02.xcmsh","G3_Nature_Wall_Stone_01_F.xshmat",world.data(),{&additional,&Additional::Copy});
        capture.Observe(d9.Get(),1);auto* draw=capture.Latest(eye,1);Need(draw!=nullptr,"Batch fixture capture");base[eye]=*draw;capture.End(token);
    }
    ComPtr<IDirect3DQuery9> complete;Check(d9->CreateQuery(D3DQUERYTYPE_EVENT,&complete),"Benchmark fence");
    auto wait=[&]{Check(complete->Issue(D3DISSUE_END),"Benchmark fence issue");auto start=GetTickCount64();for(;;){auto hr=complete->GetData(nullptr,0,D3DGETDATA_FLUSH);if(hr==S_OK)break;Check(hr,"Benchmark fence wait");Need(GetTickCount64()-start<5000,"Benchmark GPU timeout");SwitchToThread();}};
    auto pixels=[&]{Check(d9->GetRenderTargetData(colorSurface.Get(),staging9.Get()),"Benchmark image readback");D3DLOCKED_RECT p{};Check(staging9->LockRect(&p,nullptr,D3DLOCK_READONLY),"Benchmark image map");std::vector<uint32_t> result(size_t(W)*H);for(unsigned y=0;y<H;++y)memcpy(result.data()+size_t(y)*W,static_cast<char*>(p.pBits)+size_t(y)*p.Pitch,W*4);staging9->UnlockRect();return result;};
    g3vr::dx11::OwnedStoneImagePool timedImagePool(4);
    constexpr uint64_t AtlasEpoch=922;
    g3vr::dx11::OwnedStoneAtlasCache timedAtlasCache(AtlasEpoch);
    g3vr::dx11::StoneAtlasVersion timedAtlasVersion{AtlasEpoch,1,1,1};
    auto atlasPublishStart=std::chrono::steady_clock::now();
    Need(bool(timedAtlasCache.Publish(timedAtlasVersion,{&additional,&Additional::Copy})),"Timed immutable atlas publication");
    double atlasPublishMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-atlasPublishStart).count();
    uint64_t sequence=1;unsigned transfers=0,snapshotCopies=0;double renderWall=0,writebackWall=0,waitWall=0;
    auto run=[&](unsigned route,std::vector<g3vr::dx11::NativeMaterialDraw> const& draws){
        auto begin=std::chrono::steady_clock::now();renderWall=writebackWall=waitWall=0;snapshotCopies=0;
        if(route==5||route==6){auto providerCalls=additional.calls;auto cacheCaptures=timedAtlasCache.Captures();auto renderStart=std::chrono::steady_clock::now();g3vr::dx11::PassStamp stamp{draws[0].frame,route==6?AtlasEpoch:sequence++,0x2468+draws[0].eye,draws[0].eye};
            auto images=timedImagePool.Capture(d11.Get(),context.Get(),residentInputs,stamp);snapshotCopies=4;g3vr::dx11::OwnedStonePass pass;Need(pass.Begin(stamp,images),"Timed owned pass begin");
            diagnostic::temporal::Geometry geometry{D3DPT_TRIANGLELIST,0,0,2097,0,1130};
            for(unsigned i=0;i<draws.size();++i){auto const& draw=draws[i];g3vr::dx11::NativeStaticSnapshot entity{};entity.identity[0]=i+1;entity.worldMeters[15]=1;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)entity.worldMeters[c*4+r]=draw.vertex[(12+r)*4+c]*(c==3?.01f:1.f);strcpy_s(entity.resource,"g3_myrtana_ardea_stairs_02.xcmsh");
                bool appended=route==6?pass.AppendCached(draw,entity,geometry,timedAtlasCache.Acquire(timedAtlasVersion),timedAtlasVersion):pass.Append(draw,entity,geometry);Need(appended,"Timed owned packet capture");}
            Need(pass.Seal()&&pass.Render(residentRenderer,stamp,rtv.Get(),dsv.Get(),W,H,residentClear),"Timed owned pass render");if(route==6)Need(additional.calls==providerCalls&&timedAtlasCache.Captures()==cacheCaptures,"Cached route re-read or republished source bytes");renderWall=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-renderStart).count();auto waitStart=std::chrono::steady_clock::now();wait11();waitWall=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-waitStart).count();return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();}
        if(route>=3){auto renderStart=std::chrono::steady_clock::now();Need(residentRenderer.RenderResidentBatch(draws.data(),unsigned(draws.size()),residentInputs,rtv.Get(),dsv.Get(),W,H,residentClear,route==4),residentRenderer.LastRefusal());renderWall=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-renderStart).count();auto waitStart=std::chrono::steady_clock::now();wait11();waitWall=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-waitStart).count();Need(residentRenderer.ResourceCaptures()==0,"Resident renderer performed native material transport");Need(residentRenderer.LastSubmittedDraws()==(route==4?1:draws.size()),"Resident submission count differs from instance plan");return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();}
        d9->SetVertexShader(vs.Get());d9->SetPixelShader(ps.Get());d9->SetVertexDeclaration(declaration.Get());mesh.Bind(d9.Get(),context.Get());
        Check(d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff081018,1,0),"Benchmark clear");Check(d9->BeginScene(),"Benchmark begin");
        auto commit=[&]{auto transferStart=std::chrono::steady_clock::now();g3vr::dx11::PassStamp stamp{sequence++,1,0xabcdef+draws[0].eye,draws[0].eye};writeback.Transfer(color11.Get(),depthView.Get(),stamp);writeback.Composite(stamp,true);++transfers;writebackWall+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-transferStart).count();};
        if(route==0){for(auto const& draw:draws){d9->SetVertexShaderConstantF(0,draw.vertex.data(),256);d9->SetPixelShaderConstantF(0,draw.pixel.data(),224);Check(d9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mesh.vertices,0,mesh.indices/3),"Benchmark original draw");}}
        else if(route==1){for(auto const& draw:draws){auto renderStart=std::chrono::steady_clock::now();Need(renderer.Render(draw,rtv.Get(),dsv.Get(),W,H),renderer.LastRefusal());renderWall+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-renderStart).count();commit();}}
        else {auto renderStart=std::chrono::steady_clock::now();Need(renderer.RenderBatch(draws.data(),unsigned(draws.size()),rtv.Get(),dsv.Get(),W,H),renderer.LastRefusal());renderWall+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-renderStart).count();commit();}
        Check(d9->EndScene(),"Benchmark end");auto waitStart=std::chrono::steady_clock::now();wait();waitWall=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-waitStart).count();return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    };
    // Negative cases never transfer partially rendered private targets back.
    std::vector<g3vr::dx11::NativeMaterialDraw> invalid{base[0],base[0]};auto saved=renderer.ResourceCaptures();invalid[1].eye=1;
    Need(!renderer.RenderBatch(invalid.data(),2,rtv.Get(),dsv.Get(),W,H)&&renderer.ResourceCaptures()==saved,"Mixed-eye batch acquired resources");
    invalid[1]=base[0];++invalid[1].frame;Need(!renderer.RenderBatch(invalid.data(),2,rtv.Get(),dsv.Get(),W,H),"Mixed-frame batch accepted");
    invalid[1]=base[0];++invalid[1].samplers[1].identity;Need(!renderer.RenderBatch(invalid.data(),2,rtv.Get(),dsv.Get(),W,H),"Changed shadow resource accepted");
    Need(!renderer.RenderBatch(nullptr,1,rtv.Get(),dsv.Get(),W,H)&&!renderer.RenderBatch(invalid.data(),0,rtv.Get(),dsv.Get(),W,H)&&!renderer.RenderBatch(invalid.data(),129,rtv.Get(),dsv.Get(),W,H),"Unbounded batch accepted");
    Check(d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff081018,1,0),"Refused batch scene clear");auto unchanged=pixels();
    Additional unavailable=additional;unavailable.valid=false;invalid={base[0],base[0]};invalid[1].lightmap={&unavailable,&Additional::Copy};
    Need(!renderer.RenderBatch(invalid.data(),2,rtv.Get(),dsv.Get(),W,H),"Unavailable second atlas accepted");Need(pixels()==unchanged,"Partial batch modified native scene");
    // Resident output refuses missing, foreign-device and aliased resources.
    auto missing=residentInputs;missing.shadows[0]=nullptr;Need(!residentRenderer.RenderResidentBatch(base.data(),1,missing,rtv.Get(),dsv.Get(),W,H),"Missing resident shadow accepted");
    ComPtr<ID3D11ShaderResourceView> outputView;Check(d11->CreateShaderResourceView(color11.Get(),nullptr,&outputView),"Resident target alias view");auto aliased=residentInputs;aliased.lightmap=outputView.Get();Need(!residentRenderer.RenderResidentBatch(base.data(),1,aliased,rtv.Get(),dsv.Get(),W,H),"Resident input/output alias accepted");
    ComPtr<ID3D11Device> foreign;ComPtr<ID3D11DeviceContext> foreignContext;Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&foreign,&level,&foreignContext),"Foreign resident test device");
    D3D11_TEXTURE2D_DESC foreignDesc{};foreignDesc.Width=foreignDesc.Height=128;foreignDesc.MipLevels=foreignDesc.ArraySize=foreignDesc.SampleDesc.Count=1;foreignDesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;foreignDesc.BindFlags=D3D11_BIND_SHADER_RESOURCE;ComPtr<ID3D11Texture2D> foreignTexture;ComPtr<ID3D11ShaderResourceView> foreignView;Check(foreign->CreateTexture2D(&foreignDesc,nullptr,&foreignTexture),"Foreign resident texture");Check(foreign->CreateShaderResourceView(foreignTexture.Get(),nullptr,&foreignView),"Foreign resident view");auto wrongDevice=residentInputs;wrongDevice.noise=foreignView.Get();Need(!residentRenderer.RenderResidentBatch(base.data(),1,wrongDevice,rtv.Get(),dsv.Get(),W,H),"Foreign resident device accepted");
    Need(!residentRenderer.RenderBatch(base.data(),1,rtv.Get(),dsv.Get(),W,H),"Resident-only renderer entered the native bridge");
    // Per-instance material or atlas differences must disable instancing.
    auto varied=std::vector<g3vr::dx11::NativeMaterialDraw>{base[0],base[0]};varied[1].pixel[14*4]=.25f;
    Need(residentRenderer.RenderResidentBatch(varied.data(),2,residentInputs,rtv.Get(),dsv.Get(),W,H,residentClear),"Varied lighting fallback refused");wait11();Need(residentRenderer.LastSubmittedDraws()==2,"Different material constants were instanced together");
    Additional changedAtlas=additional;changedAtlas.bytes[0]^=1;varied={base[0],base[0]};varied[1].lightmap={&changedAtlas,&Additional::Copy};
    Need(residentRenderer.RenderResidentBatch(varied.data(),2,residentInputs,rtv.Get(),dsv.Get(),W,H,residentClear),"Varied atlas fallback refused");wait11();Need(residentRenderer.LastSubmittedDraws()==2,"Different atlas coordinates were instanced together");
    // Rotated/scaled instance worlds must preserve native tangent/normal and
    // lighting behavior, not merely translated silhouette positions.
    for(unsigned eye=0;eye<2;++eye){std::vector<g3vr::dx11::NativeMaterialDraw> posed(4,base[eye]);
        for(unsigned i=0;i<posed.size();++i){auto& v=posed[i].vertex;float angle=float(i)*.12f,scale=1.f+float(i)*.03f;v[48]=std::cos(angle)*scale;v[50]=std::sin(angle)*scale;v[53]=scale;v[56]=-std::sin(angle)*scale;v[58]=std::cos(angle)*scale;v[51]+=float(i)*35.f;}
        run(0,posed);auto original=pixels();run(4,posed);auto instancePixels=residentPixels();uint64_t changed=0;unsigned maxError=0;
        for(size_t i=0;i<original.size();++i)for(unsigned shift:{0u,8u,16u}){unsigned a=(original[i]>>shift)&255,b=(instancePixels[i]>>shift)&255,e=a>b?a-b:b-a;maxError=std::max(maxError,e);changed+=e!=0;}
        Need(maxError<=1&&changed<500,"Rotated/scaled resident instances changed native material output");
    }
    // Packet capture owns atlas bytes and immutable same-device image versions.
    auto entityFor=[](g3vr::dx11::NativeMaterialDraw const& draw,unsigned identity){g3vr::dx11::NativeStaticSnapshot entity{};entity.identity[0]=identity;entity.worldMeters[15]=1;
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)entity.worldMeters[c*4+r]=draw.vertex[(12+r)*4+c]*(c==3?.01f:1.f);
        strcpy_s(entity.resource,"g3_myrtana_ardea_stairs_02.xcmsh");return entity;};
    diagnostic::temporal::Geometry ownedGeometry{D3DPT_TRIANGLELIST,0,0,2097,0,1130};
    for(unsigned eye=0;eye<2;++eye){
        g3vr::dx11::PassStamp stamp{1,77,0x9876+eye,eye};
        std::array<ComPtr<ID3D11Texture2D>,4> mutableImages;std::array<ComPtr<ID3D11ShaderResourceView>,4> mutableViews;
        ID3D11ShaderResourceView* initial[]{residentInputs.noise,residentInputs.shadows[0],residentInputs.shadows[1],residentInputs.lightmap};
        for(unsigned i=0;i<4;++i){ComPtr<ID3D11Resource> resource;initial[i]->GetResource(&resource);ComPtr<ID3D11Texture2D> texture;Check(resource.As(&texture),"Owned source texture");D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=desc.MiscFlags=0;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            Check(d11->CreateTexture2D(&desc,nullptr,&mutableImages[i]),"Mutable producer texture");D3D11_SHADER_RESOURCE_VIEW_DESC view{};initial[i]->GetDesc(&view);Check(d11->CreateShaderResourceView(mutableImages[i].Get(),&view,&mutableViews[i]),"Mutable producer view");context->CopyResource(mutableImages[i].Get(),texture.Get());}
        g3vr::dx11::ResidentStoneInputs mutableInputs{mutableViews[0].Get(),{mutableViews[1].Get(),mutableViews[2].Get()},mutableViews[3].Get()};
        auto ownedImages=g3vr::dx11::OwnedStoneImages::Capture(d11.Get(),context.Get(),mutableInputs,stamp);
        g3vr::dx11::OwnedStonePass pass;Need(pass.Begin(stamp,ownedImages),"Owned pass begin");std::vector<g3vr::dx11::NativeMaterialDraw> original(8,base[eye]);
        for(unsigned i=0;i<original.size();++i){original[i].vertex[51]+=float(i)*35.f;
            Additional temporary=additional;auto draw=original[i];draw.lightmap={&temporary,&Additional::Copy};auto entity=entityFor(draw,i+1);
            Need(pass.Append(draw,entity,ownedGeometry),"Owned packet capture failed");temporary.valid=false;std::fill(temporary.bytes.begin(),temporary.bytes.end(),char{});draw.vertex.fill(NAN);
        }
        // Mutate and release EVERY producer image after snapshot capture. The
        // pass must render the old captured image version, not the new handles.
        for(unsigned i=0;i<4;++i){D3D11_TEXTURE2D_DESC desc{};mutableImages[i]->GetDesc(&desc);std::vector<uint32_t> zeros(size_t(desc.Width)*desc.Height);context->UpdateSubresource(mutableImages[i].Get(),0,nullptr,zeros.data(),desc.Width*4,0);mutableViews[i].Reset();mutableImages[i].Reset();}
        ownedImages.reset();Need(pass.Seal()&&pass.Count()==8&&pass.CpuBytes()<8*1024*1024,"Owned packet budget or seal");
        g3vr::dx11::OwnedStonePass moved=std::move(pass);Need(pass.Status()==g3vr::dx11::OwnedStonePass::State::Empty,"Moved pass retained SDK ownership");
        run(0,original);auto expected=pixels();
        Need(moved.Render(residentRenderer,stamp,rtv.Get(),dsv.Get(),W,H,residentClear),"Owned pass render after source destruction");wait11();auto actual=residentPixels();uint64_t changed=0;unsigned maximum=0;
        for(size_t i=0;i<actual.size();++i){Need((actual[i]>>24)==(expected[i]>>24),"Owned pass alpha differs");for(unsigned shift:{0u,8u,16u}){unsigned a=(actual[i]>>shift)&255,b=(expected[i]>>shift)&255,e=a>b?a-b:b-a;maximum=std::max(maximum,e);changed+=e!=0;}}
        if(maximum>1||changed>=500){
            run(4,original);auto immediate=residentPixels();uint64_t lifetimeDifferences=0;
            for(size_t i=0;i<actual.size();++i)lifetimeDifferences+=actual[i]!=immediate[i];
            std::cerr<<"Owned snapshot parity: eye="<<eye<<" max="<<maximum<<" changed="<<changed<<" differs_from_immediate_dx11="<<lifetimeDifferences<<'\n';
            std::ofstream sparse(output/L"native-resident-differences.csv");sparse<<"x,y,native_rgba,dx11_rgba\n";
            for(size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i])sparse<<std::dec<<i%W<<','<<i/W<<','<<std::hex<<expected[i]<<','<<actual[i]<<'\n';
            auto compare=[&](char const* label,std::vector<uint32_t> const& reference,std::vector<uint32_t> const& image){unsigned max=0;uint64_t differences=0,pixels=0;
                for(size_t i=0;i<image.size();++i){pixels+=reference[i]!=image[i];for(unsigned shift:{0u,8u,16u}){unsigned a=(reference[i]>>shift)&255,b=(image[i]>>shift)&255,e=a>b?a-b:b-a;max=std::max(max,e);differences+=e!=0;}}
                std::cerr<<label<<" max="<<max<<" components="<<differences<<" pixels="<<pixels<<'\n';};
            run(3,original);compare("Resident separate draws vs native",expected,residentPixels());
            for(unsigned i=0;i<original.size();++i){std::vector<g3vr::dx11::NativeMaterialDraw> single{original[i]};run(0,single);auto native=pixels();run(4,single);compare(("Single instance "+std::to_string(i)).c_str(),native,residentPixels());}
        }
        Need(maximum<=1&&changed<500,"Owned pass output differs from native reference");
        Need(moved.Status()==g3vr::dx11::OwnedStonePass::State::Consumed&&!moved.Render(residentRenderer,stamp,rtv.Get(),dsv.Get(),W,H,residentClear),"Owned pass replayed twice");
        Need(residentPixels()==actual,"Duplicate owned submission touched output");
    }
    {
        using Pass=g3vr::dx11::OwnedStonePass;g3vr::dx11::PassStamp stamp{1,88,0x12345,0};
        auto images=g3vr::dx11::OwnedStoneImages::Capture(d11.Get(),context.Get(),residentInputs,stamp);auto entity=entityFor(base[0],1);
        for(unsigned variant=0;variant<4;++variant){Pass pass;Need(pass.Begin(stamp,images)&&pass.Append(base[0],entity,ownedGeometry)&&pass.Seal(),"Owned stale test prepare");auto wrong=stamp;
            if(variant==0)++wrong.frame;if(variant==1)++wrong.epoch;if(variant==2)++wrong.camera;if(variant==3)wrong.eye=1;
            auto before=residentPixels();Need(!pass.Render(residentRenderer,wrong,rtv.Get(),dsv.Get(),W,H)&&pass.Status()==Pass::State::Rejected,"Owned stale or foreign pass accepted");Need(residentPixels()==before,"Stale owned pass touched output");}
        Pass mismatch;auto wrong=stamp;++wrong.epoch;Need(!mismatch.Begin(wrong,images),"Owned resource epoch mismatch accepted");
        Pass duplicate;Need(duplicate.Begin(stamp,images)&&duplicate.Append(base[0],entity,ownedGeometry),"Duplicate test begin");Need(!duplicate.Append(base[0],entity,ownedGeometry)&&!duplicate.Seal()&&duplicate.Count()==0,"Duplicate entity left partial pass usable");
        Pass invalidPacketPass;Need(invalidPacketPass.Begin(stamp,images)&&invalidPacketPass.Append(base[0],entity,ownedGeometry),"Invalid packet test begin");auto bad=base[0];bad.pixelHash^=1;entity.identity[0]=2;Need(!invalidPacketPass.Append(bad,entity,ownedGeometry)&&invalidPacketPass.Status()==Pass::State::Rejected&&!invalidPacketPass.Seal(),"Unknown shader left partial pass usable");
        Pass unsealed;Need(unsealed.Begin(stamp,images)&&!unsealed.Render(residentRenderer,stamp,rtv.Get(),dsv.Get(),W,H),"Unsealed pass rendered");
        Pass wrongMesh;Need(wrongMesh.Begin(stamp,images),"Wrong mesh begin");strcpy_s(entity.resource,"unverified_mesh.xcmsh");Need(!wrongMesh.Append(base[0],entity,ownedGeometry),"Foreign geometry packet accepted");entity=entityFor(base[0],1);
        Pass wrongWorld;Need(wrongWorld.Begin(stamp,images),"Wrong world begin");auto misplaced=entity;misplaced.worldMeters[12]+=1;Need(!wrongWorld.Append(base[0],misplaced,ownedGeometry),"Entity/shader world mismatch accepted");
        Additional nonfiniteAtlas=additional;uint32_t infinite=0x7f800000u;memcpy(nonfiniteAtlas.bytes.data()+4,&infinite,4);auto malformed=base[0];malformed.lightmap={&nonfiniteAtlas,&Additional::Copy};entity=entityFor(base[0],1);Pass infinitePacket;Need(infinitePacket.Begin(stamp,images)&&!infinitePacket.Append(malformed,entity,ownedGeometry),"Infinite atlas coordinate accepted");
        uint32_t notANumber=0x7fc00000u;memcpy(nonfiniteAtlas.bytes.data()+4,&notANumber,4);Pass nanPacket;Need(nanPacket.Begin(stamp,images)&&!nanPacket.Append(malformed,entity,ownedGeometry),"NaN atlas coordinate accepted");
        nonfiniteAtlas.bytes=additional.bytes;memcpy(nonfiniteAtlas.bytes.data(),&notANumber,4);Pass packedColor;Need(packedColor.Begin(stamp,images)&&packedColor.Append(malformed,entity,ownedGeometry),"Packed vertex color was classified as floating point");
        Pass tooMany;Need(tooMany.Begin(stamp,images),"Packet limit begin");bool refused=false;for(unsigned i=0;i<129;++i){entity.identity[0]=i+1;if(!tooMany.Append(base[0],entity,ownedGeometry)){refused=true;break;}}Need(refused&&tooMany.Status()==Pass::State::Rejected&&tooMany.CpuBytes()==0&&!tooMany.Seal(),"Packet CPU/count limit left partial pass usable");
        auto rejectImages=[&](g3vr::dx11::ResidentStoneInputs const& input,g3vr::dx11::PassStamp key){bool refused=false;try{g3vr::dx11::OwnedStoneImages::Capture(d11.Get(),context.Get(),input,key);}catch(std::exception const&){refused=true;}Need(refused,"Invalid owned image capture accepted");};
        rejectImages(missing,stamp);rejectImages(wrongDevice,stamp);rejectImages(residentInputs,{});
    }
    {
        g3vr::dx11::OwnedStoneImagePool pool(2);g3vr::dx11::PassStamp key{1,99,0x54321,0};
        auto first=pool.Capture(d11.Get(),context.Get(),residentInputs,key);auto firstView=first->Inputs().noise;auto old=key;++key.frame;
        auto second=pool.Capture(d11.Get(),context.Get(),residentInputs,key);Need(pool.Slots()==2&&pool.Allocations()==8,"Owned image pool allocation accounting");
        bool refused=false;try{auto third=key;++third.frame;pool.Capture(d11.Get(),context.Get(),residentInputs,third);}catch(std::exception const&){refused=true;}
        Need(refused&&first->Stamp()==old&&pool.Allocations()==8,"Pool exhaustion overwrote a live snapshot");
        auto before=pool.Allocations();first.reset();++key.frame;auto reused=pool.Capture(d11.Get(),context.Get(),residentInputs,key);
        Need(reused->Inputs().noise==firstView&&reused->Stamp()==key&&pool.Allocations()==before,"Free image slot was reallocated instead of reused");
        auto secondStamp=second->Stamp();Need(secondStamp.frame==2,"Pinned second image version was mutated");
        // A queued pass keeps the snapshot unavailable even if its producer
        // drops its own reference. Rejecting that pass releases the slot.
        second.reset();g3vr::dx11::OwnedStonePass pinned;auto next=key;++next.frame;auto candidate=pool.Capture(d11.Get(),context.Get(),residentInputs,next);Need(pinned.Begin(next,candidate),"Pinned image pass begin");candidate.reset();
        refused=false;try{auto full=next;++full.frame;pool.Capture(d11.Get(),context.Get(),residentInputs,full);}catch(std::exception const&){refused=true;}Need(refused,"Pool recycled images held by a queued pass");
        Need(!pinned.Seal(),"Empty pinned pass sealed");auto freeAgain=next;++freeAgain.frame;auto fresh=pool.Capture(d11.Get(),context.Get(),residentInputs,freeAgain);Need(fresh->Stamp()==freeAgain&&pool.Allocations()==before,"Rejected pass did not release its image slot");
    }
    // Reuse a validated immutable version across entities, frames and eyes.
    // Neither the SDK provider nor its cache needs to survive the sealed pass.
    for(unsigned eye=0;eye<2;++eye){
        using Pass=g3vr::dx11::OwnedStonePass;g3vr::dx11::PassStamp key{1,314,0x8899+eye,eye};
        auto images=g3vr::dx11::OwnedStoneImages::Capture(d11.Get(),context.Get(),residentInputs,key);
        Pass cached;Need(cached.Begin(key,images),"Cached lifetime begin");
        std::vector<g3vr::dx11::NativeMaterialDraw> original(8,base[eye]);
        {
            g3vr::dx11::OwnedStoneAtlasCache cache(key.epoch);Additional temporary=additional;
            g3vr::dx11::StoneAtlasVersion version{key.epoch,7,1,1};
            Need(bool(cache.Publish(version,{&temporary,&Additional::Copy})),"Cached lifetime publication");
            unsigned calls=temporary.calls;
            for(unsigned i=0;i<original.size();++i){original[i].vertex[51]+=float(i)*35.f;auto draw=original[i];draw.lightmap={};
                Need(cached.AppendCached(draw,entityFor(draw,i+1),ownedGeometry,cache.Acquire(version),version),"Cached packet append");}
            Need(temporary.calls==calls,"Cached append re-read mutable SDK provider");
            cache.Invalidate(version.source);temporary.bytes[0]^=1;++version.revision;
            Need(bool(cache.Publish(version,{&temporary,&Additional::Copy})),"Replacement atlas publication");
            temporary.valid=false;std::fill(temporary.bytes.begin(),temporary.bytes.end(),char{});
        }
        Need(cached.Seal(),"Cached lifetime seal");run(0,original);auto expected=pixels();
        Need(cached.Render(residentRenderer,key,rtv.Get(),dsv.Get(),W,H,residentClear),"Cached pass after producer/cache destruction");wait11();auto actual=residentPixels();
        unsigned maximum=0;uint64_t changed=0;
        for(size_t i=0;i<actual.size();++i){Need((actual[i]>>24)==(expected[i]>>24),"Cached alpha mismatch");for(unsigned shift:{0u,8u,16u}){unsigned a=(actual[i]>>shift)&255,b=(expected[i]>>shift)&255,e=a>b?a-b:b-a;maximum=std::max(maximum,e);changed+=e!=0;}}
        Need(maximum<=1&&changed<500&&residentRenderer.LastSubmittedDraws()==1,"Cached immutable versions changed pixels or prevented instancing");
        // Cached input content may differ. It must take the separate draw path.
        g3vr::dx11::OwnedStoneAtlasCache cache(key.epoch);g3vr::dx11::StoneAtlasVersion v{key.epoch,10,1,1};
        auto old=cache.Publish(v,{&additional,&Additional::Copy});Additional changedSource=additional;changedSource.bytes[0]^=1;++v.revision;
        auto changedVersion=cache.Publish(v,{&changedSource,&Additional::Copy});Pass variedCached;Need(variedCached.Begin(key,images),"Varied cached begin");
        auto draw=base[eye];Need(variedCached.AppendCached(draw,entityFor(draw,1),ownedGeometry,old,old->Version()),"Old pinned version append");
        Need(variedCached.AppendCached(draw,entityFor(draw,2),ownedGeometry,changedVersion,v)&&variedCached.Seal()&&variedCached.Render(residentRenderer,key,rtv.Get(),dsv.Get(),W,H,residentClear),"Varied cached render");wait11();
        Need(residentRenderer.LastSubmittedDraws()==2,"Different immutable atlas contents instanced together");
        for(unsigned variant=0;variant<4;++variant){Pass rejected;Need(rejected.Begin(key,images)&&rejected.AppendCached(draw,entityFor(draw,1),ownedGeometry,changedVersion,v),"Cached rejection begin");auto expectedVersion=v;auto candidate=changedVersion;
            if(variant==0)++expectedVersion.revision;if(variant==1)++expectedVersion.generation;if(variant==2)++expectedVersion.epoch;if(variant==3)candidate.reset();
            Need(!rejected.AppendCached(draw,entityFor(draw,2),ownedGeometry,candidate,expectedVersion)&&rejected.Status()==Pass::State::Rejected&&!rejected.Seal()&&rejected.Count()==0,"Stale cached version left partial pass usable");}
    }
    if(ownedOnly){std::ofstream result(output/L"result.json");result<<"{\"result\":\"PASS\",\"owned_packet_lifetimes\":true,\"source_images_mutated_and_released\":true,\"sdk_provider_destroyed\":true,\"stale_passes_rejected\":true,\"bounded_image_pool_reuse\":true,\"no_live_slot_overwrite\":true,\"cached_atlas_lifetime_and_revision_rejection\":true,\"both_eyes\":true,\"runtime_integration\":false}\n";std::cout<<"PASS owned packets after SDK/producer destruction, immutable image versions, cached atlas revisions, move, stereo and stale/refused passes\n";return 0;}

    std::ofstream results(output/L"benchmark.jsonl");results<<std::setprecision(9);
    results<<"{\"type\":\"cached_atlas_initial_publication\",\"cpu_wall_ms\":"<<atlasPublishMs<<",\"bytes\":"<<timedAtlasCache.LiveBytes()<<",\"repeated_fixture_only\":true,\"native_content_version_producer_implemented\":false}\n";
    for(unsigned count:{1u,8u,32u})for(unsigned eye=0;eye<2;++eye){
        std::vector<g3vr::dx11::NativeMaterialDraw> draws(count,base[eye]);for(unsigned i=0;i<count;++i){draws[i].vertex[12*4+3]+=float(i%8)*35.f;draws[i].vertex[14*4+3]+=float(i/8)*35.f;}
        run(0,draws);auto expected=pixels();
        for(unsigned route=1;route<7;++route){run(route,draws);auto actual=route>=3?residentPixels():pixels();uint64_t changed=0;unsigned maximum=0;
            for(size_t i=0;i<actual.size();++i){Need((actual[i]>>24)==(expected[i]>>24),"Batch changed target alpha");for(unsigned shift:{0u,8u,16u}){unsigned a=(actual[i]>>shift)&255,b=(expected[i]>>shift)&255,e=a>b?a-b:b-a;maximum=std::max(maximum,e);changed+=e!=0;}}
            results<<"{\"type\":\"parity\",\"count\":"<<count<<",\"eye\":"<<eye<<",\"route\":"<<route<<",\"changed_components\":"<<changed<<",\"max_rgb_steps\":"<<maximum<<"}\n";results.flush();Need(maximum<=1&&changed<500,"Batch scene differs from native reference");
        }
        std::array<std::vector<double>,7> samples,renders,writebacks,waits;std::array<uint64_t,7> captures{};std::array<unsigned,7> copies{},snapshots{};
        for(unsigned iteration=0;iteration<11;++iteration)for(unsigned order=0;order<7;++order){unsigned route=(order+iteration)%7;auto before=renderer.ResourceCaptures();transfers=0;auto allocationsBefore=timedImagePool.Allocations();double ms=run(route,draws);if(iteration>=2){Need(timedImagePool.Allocations()==allocationsBefore,"Steady owned pass reallocated GPU images");samples[route].push_back(ms);renders[route].push_back(renderWall);writebacks[route].push_back(writebackWall);waits[route].push_back(waitWall);captures[route]+=renderer.ResourceCaptures()-before;copies[route]+=transfers;snapshots[route]+=snapshotCopies;}}
        for(unsigned route=0;route<7;++route){auto median=[](std::vector<double> values){std::sort(values.begin(),values.end());return values[values.size()/2];};auto& times=samples[route];std::sort(times.begin(),times.end());Need(captures[route]==((route==0||route>=3)?0:route==1?count*times.size():times.size()),"Resource capture batch count mismatch");Need(copies[route]==((route==0||route>=3)?0:route==1?count*times.size():times.size()),"Writeback batch count mismatch");
            results<<"{\"type\":\"timing\",\"count\":"<<count<<",\"eye\":"<<eye<<",\"route\":"<<route<<",\"width\":"<<W<<",\"height\":"<<H<<",\"samples\":"<<times.size()<<",\"median_wall_ms\":"<<times[times.size()/2]<<",\"p95_wall_ms\":"<<times.back()<<",\"median_render_and_feed_ms\":"<<median(renders[route])<<",\"median_writeback_ms\":"<<median(writebacks[route])<<",\"median_final_api_wait_ms\":"<<median(waits[route])<<",\"resource_captures\":"<<captures[route]<<",\"writebacks\":"<<copies[route]<<",\"cached_atlas_route\":"<<(route==6?"true":"false")<<",\"same_device_image_snapshot_copies\":"<<snapshots[route]<<",\"synthetic_repeated_geometry\":true,\"native_scene_preparation_measured\":false}\n";results.flush();}
    }
    std::ofstream summary(output/L"result.json");summary<<"{\"result\":\"PASS\",\"same_material_only\":true,\"both_eyes_tested\":true,\"repeated_mesh_workload\":true,\"game_loaded\":false,\"runtime_integration\":false,\"resident_route_tested\":true,\"resident_instancing_tested\":true,\"native_postprocessing_replaced\":false,\"resolution\":["<<W<<','<<H<<"]}\n";
    std::cout<<"PASS native/per-object/batched/resident/instanced parity, stereo, bounded inputs, refusal and synchronization counts\n";return 0;
}catch(std::exception const& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
