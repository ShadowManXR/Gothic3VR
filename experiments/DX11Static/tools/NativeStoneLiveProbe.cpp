#include "NativeStoneFixtures.h"
#include "NativeStoneRenderer.h"
#include "NativeMaterialCapture.h"
#include "NativeStaticTrial.h"
#include "NativeStonePacketProducer.h"
#include "../../../src/TemporalObjectHistory.h"
namespace {
struct Additional {std::vector<char> bytes;unsigned calls=0;bool valid=true;IDirect3DDevice9* alteredDevice=nullptr;IDirect3DPixelShader9* alteredPS=nullptr;
    static bool __cdecl Copy(void* p,void* to,unsigned vertices,unsigned bytes)noexcept{auto& s=*static_cast<Additional*>(p);++s.calls;if(!s.valid||vertices!=2097||bytes!=s.bytes.size())return false;memcpy(to,s.bytes.data(),bytes);if(s.alteredDevice&&FAILED(s.alteredDevice->SetPixelShader(s.alteredPS)))return false;return true;}
};
}
int wmain(int argc,wchar_t** argv)try{
    bool anisotropicOnly=argc==6&&std::wstring(argv[5])==L"--anisotropic-only";
    Need(argc==5||(argc==6&&(std::wstring(argv[5])==L"--depth-prepass"||anisotropicOnly)),"Usage: LiveProbe <profile> <output> <dxvk> <assets> [--depth-prepass|--anisotropic-only]");bool depthPrepass=argc==6&&!anisotropicOnly;W=1280;H=720;std::filesystem::path profile=argv[1],output=argv[2];std::filesystem::create_directories(output);{std::ofstream f(output/L"result.json");f<<"{\"result\":\"INCOMPLETE\"}\n";}
    Library library;library.value=LoadLibraryExW(argv[3],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Need(library.value!=nullptr,"Live backend");auto factory=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(library.value,"Direct3DCreate9Ex"));Need(factory!=nullptr,"Live native factory");ComPtr<IDirect3D9Ex> api;Check(factory(D3D_SDK_VERSION,&api),"Live API");Window window;D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=W;pp.BackBufferHeight=H;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.value;pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;ComPtr<IDirect3DDevice9Ex> d9;Check(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&d9),"Live native device");
    LUID luid{};Check(api->GetAdapterLUID(0,&luid),"Live GPU");ComPtr<IDXGIFactory1> dxgi;Check(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)),"Live factory");ComPtr<IDXGIAdapter1> adapter;for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> a;Need(dxgi->EnumAdapters1(i,&a)!=DXGI_ERROR_NOT_FOUND,"Live GPU mismatch");DXGI_ADAPTER_DESC1 desc{};a->GetDesc1(&desc);if(!memcmp(&desc.AdapterLuid,&luid,sizeof(luid))){adapter=a;break;}}
    ComPtr<ID3D11Device> d11;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d11,&level,&context),"Live owned DX11");
    NativeScene mesh;mesh.Create(d9.Get(),d11.Get(),profile/L"native-scene.bin");auto data=Read(profile/L"native-scene.bin",16*1024*1024);Additional additional;additional.bytes.assign(data.begin()+32+mesh.vertices*48,data.begin()+32+mesh.vertices*72);
    auto originalVS=Read(profile/L"native-vertex.cso",32768),originalPS=Read(profile/L"native.cso",32768);ComPtr<IDirect3DVertexShader9> vs;ComPtr<IDirect3DPixelShader9> ps;Check(d9->CreateVertexShader(reinterpret_cast<DWORD const*>(originalVS.data()),&vs),"Live original VS");Check(d9->CreatePixelShader(reinterpret_cast<DWORD const*>(originalPS.data()),&ps),"Live original PS");
    D3DVERTEXELEMENT9 decl[]={{0,0,2,0,0,0},{0,12,2,0,3,0},{0,24,1,0,5,0},{1,0,4,0,10,0},{1,4,2,0,6,0},{2,0,4,0,10,1},{2,4,2,0,5,7},{2,16,1,0,5,6},D3DDECL_END()};ComPtr<IDirect3DVertexDeclaration9> declaration;Check(d9->CreateVertexDeclaration(decl,&declaration),"Live native declaration");d9->SetVertexDeclaration(declaration.Get());d9->SetVertexShader(vs.Get());d9->SetPixelShader(ps.Get());mesh.Bind(d9.Get(),context.Get());
    std::array<TexturePair,6> textures;for(unsigned i=0;i<6;++i){textures[i]=i==0?Color(d9.Get(),d11.Get(),0,128):i==1||i==2?Shadow(d9.Get(),d11.Get(),i,128):i==3?OriginalLightmap(d9.Get(),d11.Get(),profile/L"lightmap-atlas.bin"):OriginalDds(d9.Get(),d11.Get(),profile/(i==4?L"normal.dds":L"diffuse.dds"));d9->SetTexture(i,textures[i].native.Get());for(auto state:{D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV})d9->SetSamplerState(i,state,i>=1&&i<=3?D3DTADDRESS_CLAMP:D3DTADDRESS_WRAP);d9->SetSamplerState(i,D3DSAMP_ADDRESSW,D3DTADDRESS_WRAP);d9->SetSamplerState(i,D3DSAMP_MAGFILTER,i==0?D3DTEXF_POINT:D3DTEXF_LINEAR);d9->SetSamplerState(i,D3DSAMP_MINFILTER,i==0?D3DTEXF_POINT:D3DTEXF_LINEAR);d9->SetSamplerState(i,D3DSAMP_MIPFILTER,i==1||i==2?D3DTEXF_NONE:i==0?D3DTEXF_POINT:D3DTEXF_LINEAR);d9->SetSamplerState(i,D3DSAMP_SRGBTEXTURE,FALSE);d9->SetSamplerState(i,D3DSAMP_MAXANISOTROPY,8);}
    if(anisotropicOnly)for(unsigned slot:{4u,5u}){d9->SetSamplerState(slot,D3DSAMP_MINFILTER,D3DTEXF_ANISOTROPIC);d9->SetSamplerState(slot,D3DSAMP_MAGFILTER,D3DTEXF_ANISOTROPIC);d9->SetSamplerState(slot,D3DSAMP_MAXANISOTROPY,16);}
    for(auto state:{D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_FOGENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_STENCILENABLE,D3DRS_CLIPPLANEENABLE,D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS})d9->SetRenderState(state,FALSE);d9->SetRenderState(D3DRS_ZENABLE,TRUE);d9->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d9->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d9->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d9->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);d9->SetRenderState(D3DRS_COLORWRITEENABLE,7);
    Target9 native;native.Create(d9.Get());native.Bind(d9.Get());ComPtr<IDirect3DSurface9> nativeDepth;Check(d9->CreateDepthStencilSurface(W,H,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&nativeDepth,nullptr),"Live reference depth");d9->SetDepthStencilSurface(nativeDepth.Get());Target11 target;target.Create(d11.Get());D3D11_TEXTURE2D_DESC dd{};dd.Width=W;dd.Height=H;dd.MipLevels=dd.ArraySize=dd.SampleDesc.Count=1;dd.Format=DXGI_FORMAT_D32_FLOAT;dd.BindFlags=D3D11_BIND_DEPTH_STENCIL;ComPtr<ID3D11Texture2D> depth;ComPtr<ID3D11DepthStencilView> dsv;Check(d11->CreateTexture2D(&dd,nullptr,&depth),"Live DX11 depth");Check(d11->CreateDepthStencilView(depth.Get(),nullptr,&dsv),"Live DX11 DSV");
    g3vr::dx11::NativeStoneRenderer renderer;renderer.Initialize(d9.Get(),d11.Get(),context.Get(),profile);g3vr::dx11::NativeMaterialCapture capture;capture.Initialize(output/L"captured",W,H,true);
    auto vertex=Read(profile/L"vertex-constants.bin",16384),pixel=Read(profile/L"constants.bin",8192);Need(vertex.size()==12+2*1024*4&&pixel.size()==12+2*896*4,"Live constants format");uint64_t compared=0,changed=0,covered=0,coverageError=0;unsigned maxSteps=0,draws=0;double absolute=0;float maxFloat=0;std::ofstream rows(output/L"cases.jsonl");
    for(unsigned eye=0;eye<2;++eye)for(unsigned mode=0;mode<6;++mode){d9->SetRenderState(D3DRS_CULLMODE,1+mode%3);uint64_t frame=1+eye*6+mode;std::array<float,1024> vc;std::array<float,896> pc;memcpy(vc.data(),vertex.data()+12+eye*1024*4,1024*4);memcpy(pc.data(),pixel.data()+12+eye*896*4,896*4);if(mode==1)pc[17*4]=1;if(mode==2){vc[16*4]=.2f;vc[16*4+1]=0;}if(mode==3){pc[14*4]=.9f;pc[14*4+1]=.1f;}if(mode==4){vc[17*4]=vc[17*4+1]=2;}if(mode==5)pc[13*4]=.35f;
        d9->SetVertexShaderConstantF(0,vc.data(),256);d9->SetPixelShaderConstantF(0,pc.data(),224);d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0);std::array<float,16> world{};world[15]=1;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)world[c*4+r]=vc[(12+r)*4+c];
        auto scope=capture.Begin(eye,frame,"g3_myrtana_ardea_stairs_02.xcmsh","G3_Nature_Wall_Stone_01_F.xshmat",world.data(),{&additional,&Additional::Copy});capture.Observe(d9.Get(),1);auto* draw=capture.Latest(eye,frame);Need(draw&&draw->matchedRow==0&&draw->pixelHash==0x26cb84265f99bd7dull,"Live capture did not bind original material row");Need(!capture.Latest(eye^1,frame)&&!capture.Latest(eye,frame+1),"Stale eye/frame material accepted");
        if(mode==0){auto wrong=*draw;wrong.pixelHash^=1;Need(!renderer.Render(wrong,target.views[0].Get(),dsv.Get(),W,H),"Unknown pixel program accepted");wrong=*draw;wrong.declaration[0].Offset=4;Need(!renderer.Render(wrong,target.views[0].Get(),dsv.Get(),W,H),"Wrong native stream declaration accepted");additional.valid=false;Need(!renderer.Render(*draw,target.views[0].Get(),dsv.Get(),W,H),"Missing live atlas accepted");additional.valid=true;}
        Need(renderer.Render(*draw,target.views[0].Get(),dsv.Get(),W,H),"Live renderer refused the verified fixture");capture.End(scope);Need(!capture.Latest(eye,frame),"Borrowed SDK source outlived native scope");
        Check(d9->BeginScene(),"Live native begin");Check(d9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mesh.vertices,0,mesh.indices/3),"Independent original draw");Check(d9->EndScene(),"Live native end");auto a=native.Pixels(d9.Get(),0),b=target.Pixels(context.Get(),0);uint64_t localChanged=0;unsigned localMax=0;for(size_t i=0;i<a.size();i+=4){bool ca=a[i]!=0||a[i+1]!=0||a[i+2]!=0,cb=b[i]!=0||b[i+1]!=0||b[i+2]!=0;coverageError+=ca!=cb;covered+=ca;for(unsigned j=0;j<3;++j){float e=std::abs(a[i+j]-b[i+j]);maxFloat=std::max(maxFloat,e);absolute+=e;++compared;auto q=[](float x){return int(std::lround(std::clamp(x,0.f,1.f)*255));};unsigned steps=unsigned(std::abs(q(a[i+j])-q(b[i+j])));localMax=std::max(localMax,steps);localChanged+=steps!=0;}}maxSteps=std::max(maxSteps,localMax);changed+=localChanged;++draws;rows<<"{\"eye\":"<<eye<<",\"case\":"<<mode<<",\"unorm_changed\":"<<localChanged<<",\"max_steps\":"<<localMax<<"}\n";rows.flush();
        // A rejected draw in this SAME frame must invalidate the prior view.
        scope=capture.Begin(eye,frame,"g3_myrtana_ardea_stairs_02.xcmsh","G3_Nature_Wall_Stone_01_F.xshmat",world.data());d9->SetRenderState(D3DRS_COLORWRITEENABLE,0);capture.Observe(d9.Get(),1);Need(!capture.Latest(eye,frame),"Rejected draw reused the previous material");d9->SetRenderState(D3DRS_COLORWRITEENABLE,7);capture.End(scope);capture.Flush();
    }
    if(anisotropicOnly){std::ofstream f(output/L"result.json");f<<"{\"result\":\""<<((maxSteps<=1&&!coverageError)?"PASS":"MISMATCH")<<"\",\"cases\":"<<draws<<",\"maximum_rgb_steps\":"<<maxSteps<<",\"changed_components\":"<<changed<<",\"coverage_disagreements\":"<<coverageError<<",\"anisotropy\":16,\"game_loaded\":false}\n";std::cout<<"Anisotropic comparison maxRGB="<<maxSteps<<" changed="<<changed<<" coverage="<<coverageError<<'\n';return 0;}
    // Exercise mode3 itself, including selected identity, SDK-style provider,
    // two independent devices, full GPU material feed and native writeback.
    auto cameras=Read(profile/L"native-cameras.bin",1024);Need(cameras.size()==268&&!memcmp(cameras.data(),"G3CAM001",8),"Missing matching trial cameras");
    ComPtr<IDirect3DTexture9> color;ComPtr<IDirect3DSurface9> colorSurface,readback;Check(d9->CreateTexture(W,H,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&color,nullptr),"Trial native color");Check(color->GetSurfaceLevel(0,&colorSurface),"Trial color surface");Check(d9->CreateOffscreenPlainSurface(W,H,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr),"Trial color readback");d9->SetRenderTarget(1,nullptr);d9->SetRenderTarget(0,colorSurface.Get());d9->SetDepthStencilSurface(nativeDepth.Get());
    auto pixels=[&]{Check(d9->GetRenderTargetData(colorSurface.Get(),readback.Get()),"Trial pixel transfer");D3DLOCKED_RECT map{};Check(readback->LockRect(&map,nullptr,D3DLOCK_READONLY),"Trial pixel lock");std::vector<uint32_t> v(size_t(W)*H);for(unsigned y=0;y<H;++y)memcpy(v.data()+size_t(y)*W,static_cast<char*>(map.pBits)+size_t(y)*map.Pitch,W*4);readback->UnlockRect();return v;};

    auto primeOwned=[&](g3vr::dx11::NativeStaticTrial& owned,uint64_t frame){
        std::array<float,1024> vc;memcpy(vc.data(),vertex.data()+12,1024*4);
        std::array<float,16> world{},view{},projection{};world[15]=1;
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)world[c*4+r]=vc[(12+r)*4+c];
        memcpy(view.data(),cameras.data()+12,64);memcpy(projection.data(),cameras.data()+76,64);
        g3vr::dx11::NativeStaticSnapshot snapshot{};snapshot.identity[0]=77;snapshot.worldMeters=world;
        for(unsigned i=12;i<15;++i)snapshot.worldMeters[i]*=.01f;strcpy_s(snapshot.resource,"g3_myrtana_ardea_stairs_02.xcmsh");
        // Discover late in the right eye; wait for the next complete frame.
        owned.BeginEye(1,frame,1);owned.Observe(0x1234,snapshot,view.data(),projection.data());
        Need(!owned.MaterialObservationEnabled(),"Discovery frame allowed a half stereo capture");owned.EndEye();
    };
    // Mode5 freezes actual native GPU inputs and consumes the packet only at
    // the later solid boundary, after the borrowed material scope has ended.
    {

        // Real pooled addressing: native indices include a minimum, the DIP
        // adds a base vertex, and each stream has a separate byte offset.
        std::array<ComPtr<IDirect3DVertexBuffer9>,3> pooled;ComPtr<IDirect3DIndexBuffer9> pooledIndices;
        size_t sourceAt=32;
        for(unsigned stream=0;stream<3;++stream){unsigned stride=mesh.strides[stream],prefix=9*stride,payload=mesh.vertices*stride;
            Check(d9->CreateVertexBuffer(prefix+payload,D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&pooled[stream],nullptr),"Pooled source VB");void* mapped=nullptr;
            Check(pooled[stream]->Lock(0,prefix+payload,&mapped,0),"Pooled source map");memset(mapped,0,prefix);memcpy(static_cast<char*>(mapped)+prefix,data.data()+sourceAt,payload);pooled[stream]->Unlock();sourceAt+=payload;
            Check(d9->SetStreamSource(stream,pooled[stream].Get(),2*stride,stride),"Pooled source bind");}
        Check(d9->CreateIndexBuffer((11+mesh.indices)*4,D3DUSAGE_WRITEONLY,D3DFMT_INDEX32,D3DPOOL_DEFAULT,&pooledIndices,nullptr),"Pooled source IB");void* mapped=nullptr;
        Check(pooledIndices->Lock(0,(11+mesh.indices)*4,&mapped,0),"Pooled index map");memset(mapped,0,11*4);
        for(unsigned i=0;i<mesh.indices;++i){uint32_t index;memcpy(&index,data.data()+sourceAt+i*4,4);index+=3;memcpy(static_cast<char*>(mapped)+(11+i)*4,&index,4);}pooledIndices->Unlock();d9->SetIndices(pooledIndices.Get());
        for(bool aniso:{false,true}){
        auto directory=output/(aniso?L"trial-mode5-anisotropic":L"trial-mode5");
        for(unsigned slot:{4u,5u}){d9->SetSamplerState(slot,D3DSAMP_MINFILTER,aniso?D3DTEXF_ANISOTROPIC:D3DTEXF_LINEAR);d9->SetSamplerState(slot,D3DSAMP_MAGFILTER,aniso?D3DTEXF_ANISOTROPIC:D3DTEXF_LINEAR);d9->SetSamplerState(slot,D3DSAMP_MAXANISOTROPY,16);}
        g3vr::dx11::NativeStaticTrial ownedTrial;ownedTrial.Initialize(d9.Get(),adapter.Get(),argv[4],directory,W,H,5);primeOwned(ownedTrial,89);
        for(unsigned eye=0;eye<2;++eye){
            std::array<float,1024> vc;std::array<float,896> pc;
            memcpy(vc.data(),vertex.data()+12+eye*1024*4,1024*4);memcpy(pc.data(),pixel.data()+12+eye*896*4,896*4);
            d9->SetVertexShaderConstantF(0,vc.data(),256);d9->SetPixelShaderConstantF(0,pc.data(),224);d9->SetRenderState(D3DRS_CULLMODE,D3DCULL_CW);
            std::array<float,16> world{},view{},projection{};world[15]=1;
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)world[c*4+r]=vc[(12+r)*4+c];
            memcpy(view.data(),cameras.data()+12+eye*128,64);memcpy(projection.data(),cameras.data()+12+eye*128+64,64);
            g3vr::dx11::NativeStaticSnapshot snapshot{};snapshot.identity[0]=77;snapshot.worldMeters=world;
            for(unsigned i=12;i<15;++i)snapshot.worldMeters[i]*=.01f;strcpy_s(snapshot.resource,"g3_myrtana_ardea_stairs_02.xcmsh");
            d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff081018,1,0);ownedTrial.BeginEye(eye,90,1);ownedTrial.Observe(0x1234,snapshot,view.data(),projection.data());
            Additional temporary=additional;Check(d9->BeginScene(),"Owned native begin");
            auto token=ownedTrial.BeginDeferredMaterial(snapshot.identity,world.data(),"G3_Nature_Wall_Stone_01_F.xshmat",{&temporary,&Additional::Copy});
            diagnostic::temporal::Geometry geometry{D3DPT_TRIANGLELIST,4,3,2097,11,1130};
            Need(!ownedTrial.ObserveMaterialDraw(d9.Get(),1,&geometry)&&!ownedTrial.NeedsMaterialWork(),"Owned validation skipped native or failed to queue");
            for(unsigned slot:{4u,5u})for(auto state:{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER}){DWORD current=0;d9->GetSamplerState(slot,state,&current);Need(current==DWORD(aniso?D3DTEXF_ANISOTROPIC:D3DTEXF_LINEAR),"Sampler control changed native filtering");}
            ownedTrial.EndMaterial(token);temporary.valid=false;std::fill(temporary.bytes.begin(),temporary.bytes.end(),char{});
            Check(d9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,4,3,mesh.vertices,11,mesh.indices/3),"Owned native draw preserved");Check(d9->EndScene(),"Owned native end");auto before=pixels();
            Check(d9->BeginScene(),"Owned deferred begin");auto wrong=projection;wrong[14]*=2;
            ownedTrial.SolidBoundary(view.data(),wrong.data());ownedTrial.SolidBoundary(view.data(),projection.data());Check(d9->EndScene(),"Owned deferred end");ownedTrial.EndEye();
            Need(pixels()==before,"Read-only owned validation changed the native scene");
        }
        std::ifstream f(directory/L"owned-validation.json");std::string result((std::istreambuf_iterator<char>(f)),{});
        Need(result.find("\"same_frame_sampler_control_pair\":true")!=std::string::npos&&result.find("\"atlas_content_hits\":1")!=std::string::npos,"Owned live producer did not validate/reuse both control eyes");
        if(!aniso)Need(result.find("\"result\":\"PASS\"")!=std::string::npos&&result.find("\"same_frame_stereo_pair\":true")!=std::string::npos,"Trilinear full parity failed");
        else Need(result.find("\"result\":\"SAMPLER_CONTROL_ONLY\"")!=std::string::npos||result.find("\"result\":\"PASS\"")!=std::string::npos,"Anisotropic control was incorrectly rejected");
        ownedTrial.Reset();}
        for(unsigned slot:{4u,5u}){d9->SetSamplerState(slot,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d9->SetSamplerState(slot,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);}
        mesh.Bind(d9.Get(),context.Get());
    }
    d9->SetRenderState(D3DRS_CULLMODE,D3DCULL_CW);unsigned trialEyes=0;g3vr::dx11::NativeStaticTrial trial;trial.Initialize(d9.Get(),adapter.Get(),argv[4],output/L"trial-mode3",W,H,3);
    for(unsigned frame=100;frame<103;++frame)for(unsigned eye=0;eye<2;++eye){std::array<float,1024> vc;memcpy(vc.data(),vertex.data()+12+eye*1024*4,1024*4);std::array<float,896> pc;memcpy(pc.data(),pixel.data()+12+eye*896*4,896*4);d9->SetVertexShaderConstantF(0,vc.data(),256);d9->SetPixelShaderConstantF(0,pc.data(),224);std::array<float,16> world{},view{},projection{};world[15]=1;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)world[c*4+r]=vc[(12+r)*4+c];memcpy(view.data(),cameras.data()+12+eye*128,64);memcpy(projection.data(),cameras.data()+12+eye*128+64,64);
        if(depthPrepass&&frame>200){Check(d9->BeginScene(),"Native depth prepass begin");d9->SetRenderState(D3DRS_COLORWRITEENABLE,0);Check(d9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mesh.vertices,0,mesh.indices/3),"Native depth prepass");d9->SetRenderState(D3DRS_COLORWRITEENABLE,7);Check(d9->EndScene(),"Native depth prepass end");}
        g3vr::dx11::NativeStaticSnapshot snapshot{};snapshot.identity[0]=77;snapshot.worldMeters=world;for(unsigned i=12;i<15;++i)snapshot.worldMeters[i]*=.01f;strcpy_s(snapshot.resource,"g3_myrtana_ardea_stairs_02.xcmsh");trial.BeginEye(eye,frame,1);trial.Observe(0x1234,snapshot,view.data(),projection.data());
        d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff081018,1,0);Check(d9->BeginScene(),"Mode3 begin");if(frame==100&&eye==0)trial.SolidBoundary(view.data(),projection.data());Need(trial.WantsMaterialEntity(snapshot.identity),"Mode3 did not select the known stairs");
        auto token=trial.BeginDeferredMaterial(snapshot.identity,world.data(),"G3_Nature_Wall_Stone_01_F.xshmat",{&additional,&Additional::Copy});diagnostic::temporal::Geometry geometry{D3DPT_TRIANGLELIST,0,0,2097,0,1130};trial.ObserveMaterialDraw(d9.Get(),1,&geometry);trial.EndMaterial(token);Check(d9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mesh.vertices,0,mesh.indices/3),"Mode3 original draw preserved");Check(d9->EndScene(),"Mode3 original end");auto before=pixels();Check(d9->BeginScene(),"Mode3 composite begin");auto wrong=projection;wrong[14]*=2;trial.SolidBoundary(view.data(),wrong.data());trial.SolidBoundary(view.data(),projection.data());Check(d9->EndScene(),"Mode3 composite end");trial.EndEye();auto after=pixels();unsigned changedRgb=0,maxChange=0;for(size_t i=0;i<before.size();++i)for(unsigned shift:{0u,8u,16u}){unsigned a=(before[i]>>shift)&255,b=(after[i]>>shift)&255,e=a>b?a-b:b-a;maxChange=std::max(maxChange,e);changedRgb+=e!=0;}Need(maxChange<=1&&changedRgb<500,"Mode3 native composite changed the known RGB material");++trialEyes;
    }
    trial.BeginEye(0,103,0);Need(!trial.MaterialObservationEnabled(),"Mode3 remained active in menu");trial.EndEye();trial.Reset();std::ifstream trialLog(output/L"trial-mode3/native-static-trial.jsonl");std::string line;unsigned rendered=0,composited=0,captures=0,references=0;while(std::getline(trialLog,line)){Need(line.find("failed")==std::string::npos&&line.find("capture_unavailable")==std::string::npos,"Mode3 failed or could not collect bounded stereo evidence");if(line.find("dx11_isolated_native_material_comparison")!=std::string::npos){++references;
        auto field=[&](char const* key){auto at=line.find(std::string("\"")+key+"\":");Need(at!=std::string::npos,"Missing isolated metric");return std::stoull(line.substr(line.find(':',at)+1));};
        Need(field("active_rgb_samples")>100&&field("rgb_coverage_disagreements")==0&&field("max_unorm_steps")<=1&&field("changed_rgb_components")<500,"Isolated original DIP and DX11 differ on known fixture");}rendered+=line.find("dx11_native_stone_material_rendered")!=std::string::npos;composited+=line.find("\"composited\":true")!=std::string::npos;captures+=line.find("\"type\":\"dx11_static_trial_capture\"")!=std::string::npos;}Need(references==2&&rendered==4&&composited==6&&captures==2,"Mode3 did not render/compose each stereo frame");
    // Real mode4 replacement: omit native only after isolated live color/depth
    // checks, preserve native alpha and get the same final pixels as original DIP.
    unsigned replacedEyes=0;bool pairOmitted[2]{};g3vr::dx11::NativeStaticTrial replacement;
    replacement.Initialize(d9.Get(),adapter.Get(),argv[4],output/L"trial-mode4",W,H,4);
    for(unsigned frame=200;frame<203;++frame)for(unsigned eye=0;eye<2;++eye){
        std::array<float,1024> vc;memcpy(vc.data(),vertex.data()+12+eye*1024*4,1024*4);
        std::array<float,896> pc;memcpy(pc.data(),pixel.data()+12+eye*896*4,896*4);
        d9->SetVertexShaderConstantF(0,vc.data(),256);d9->SetPixelShaderConstantF(0,pc.data(),224);
        std::array<float,16> world{},view{},projection{};world[15]=1;
        for(unsigned rr=0;rr<3;++rr)for(unsigned c=0;c<4;++c)world[c*4+rr]=vc[(12+rr)*4+c];
        memcpy(view.data(),cameras.data()+12+eye*128,64);memcpy(projection.data(),cameras.data()+12+eye*128+64,64);
        d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff081018,1,0);Check(d9->BeginScene(),"Mode4 control begin");
        Check(d9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mesh.vertices,0,mesh.indices/3),"Mode4 original control");Check(d9->EndScene(),"Mode4 control end");auto expected=pixels();
        d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff081018,1,0);
        if(depthPrepass&&frame>200){Check(d9->BeginScene(),"Native depth prepass begin");d9->SetRenderState(D3DRS_COLORWRITEENABLE,0);Check(d9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mesh.vertices,0,mesh.indices/3),"Native depth prepass");d9->SetRenderState(D3DRS_COLORWRITEENABLE,7);Check(d9->EndScene(),"Native depth prepass end");}
        g3vr::dx11::NativeStaticSnapshot snapshot{};snapshot.identity[0]=77;snapshot.worldMeters=world;
        for(unsigned i=12;i<15;++i)snapshot.worldMeters[i]*=.01f;strcpy_s(snapshot.resource,"g3_myrtana_ardea_stairs_02.xcmsh");
        replacement.BeginEye(eye,frame,1);replacement.Observe(0x1234,snapshot,view.data(),projection.data());
        Check(d9->BeginScene(),"Mode4 replace begin");auto token=replacement.BeginDeferredMaterial(snapshot.identity,world.data(),"G3_Nature_Wall_Stone_01_F.xshmat",{&additional,&Additional::Copy});
        diagnostic::temporal::Geometry geometry{D3DPT_TRIANGLELIST,0,0,2097,0,1130};
        auto invalid=geometry;invalid.vertices=42;Need(!replacement.ObserveMaterialDraw(d9.Get(),1,&invalid),"Mode4 omitted unknown geometry");
        if(frame==200&&eye==0)Need(replacement.NeedsMaterialWork(),"Runtime would bypass the selected material");bool omitted=replacement.ObserveMaterialDraw(d9.Get(),1,&geometry);Need(!omitted||frame>200,"Mode4 stereo proof activated in the proof pair");pairOmitted[eye]=omitted;if(eye==1)Need(pairOmitted[0]==pairOmitted[1],"Mode4 omitted only one eye");if(!omitted)Check(d9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,mesh.vertices,0,mesh.indices/3),"Mode4 unapproved native fallback");Need(!replacement.NeedsMaterialWork(),"Runtime would repeat completed material work");replacedEyes+=omitted;
        Need(!replacement.ObserveMaterialDraw(d9.Get(),1,&geometry),"Mode4 omitted a duplicate native draw");
        replacement.EndMaterial(token);replacement.SolidBoundary(view.data(),projection.data());Check(d9->EndScene(),"Mode4 replace end");replacement.EndEye();
        auto actual=pixels();unsigned different=0;for(size_t i=0;i<actual.size();++i){Need((actual[i]>>24)==(expected[i]>>24),"Mode4 changed native target alpha");
            for(unsigned shift:{0u,8u,16u}){int error=int((actual[i]>>shift)&255)-int((expected[i]>>shift)&255);Need(std::abs(error)<=1,"Mode4 color replacement differs from original");different+=error!=0;}}
        Need(different<500,"Mode4 changed too many native material samples");
    }
    replacement.Reset();Need(replacedEyes==4||replacedEyes==0,"Mode4 partially replaced a stereo pair");if(depthPrepass)Need(replacedEyes==4,"Exact-camera prepass fixture did not replace both eyes");if(!replacedEyes){std::ifstream evidence(output/L"trial-mode4/native-static-trial.jsonl");std::string text((std::istreambuf_iterator<char>(evidence)),{});Need(text.find("dx11_native_depth_prepass_comparison")!=std::string::npos&&text.find("\"passed\":false")!=std::string::npos,"Native fallback did not exercise failed prepass proof");}
    // Deliberately change the native reference program after capture. The live
    // comparison must reject its color mismatch, with no scene writeback/skip.
    g3vr::dx11::NativeStaticTrial rejected;rejected.Initialize(d9.Get(),adapter.Get(),argv[4],output/L"trial-rejected",W,H,4);
    auto debugCode=Compile("float4 main():COLOR0{return float4(1,0,1,1);}","main","ps_3_0");ComPtr<IDirect3DPixelShader9> debugPS;
    Check(d9->CreatePixelShader(static_cast<DWORD const*>(debugCode->GetBufferPointer()),&debugPS),"Mismatch reference PS");
    Additional wrong=additional;wrong.alteredDevice=d9.Get();wrong.alteredPS=debugPS.Get();
    std::array<float,1024> lastVC;memcpy(lastVC.data(),vertex.data()+12,1024*4);d9->SetVertexShaderConstantF(0,lastVC.data(),256);std::array<float,896> lastPC;memcpy(lastPC.data(),pixel.data()+12,896*4);d9->SetPixelShaderConstantF(0,lastPC.data(),224);
    std::array<float,16> badWorld{},badView{},badProjection{};badWorld[15]=1;
    for(unsigned rr=0;rr<3;++rr)for(unsigned c=0;c<4;++c)badWorld[c*4+rr]=lastVC[(12+rr)*4+c];
    memcpy(badView.data(),cameras.data()+12,64);memcpy(badProjection.data(),cameras.data()+12+64,64);
    g3vr::dx11::NativeStaticSnapshot rejectedSnapshot{};rejectedSnapshot.identity[0]=99;rejectedSnapshot.worldMeters=badWorld;
    for(unsigned i=12;i<15;++i)rejectedSnapshot.worldMeters[i]*=.01f;strcpy_s(rejectedSnapshot.resource,"g3_myrtana_ardea_stairs_02.xcmsh");
    d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff081018,1,0);auto untouched=pixels();
    rejected.BeginEye(0,400,1);rejected.Observe(0x5678,rejectedSnapshot,badView.data(),badProjection.data());Check(d9->BeginScene(),"Rejected begin");
    auto rejectedToken=rejected.BeginDeferredMaterial(rejectedSnapshot.identity,badWorld.data(),"G3_Nature_Wall_Stone_01_F.xshmat",{&wrong,&Additional::Copy});
    diagnostic::temporal::Geometry rejectedGeometry{D3DPT_TRIANGLELIST,0,0,2097,0,1130};
    Need(!rejected.ObserveMaterialDraw(d9.Get(),1,&rejectedGeometry),"Mode4 suppressed native after color mismatch");
    rejected.EndMaterial(rejectedToken);Check(d9->EndScene(),"Rejected end");rejected.EndEye();d9->SetPixelShader(ps.Get());
    Need(pixels()==untouched,"Rejected replacement changed scene pixels");
    // A good right eye after a rejected left eye must not replace independently,
    // nor continue the expensive material transport every frame after refusal.
    for(unsigned frame=400;frame<402;++frame)for(unsigned eye=0;eye<2;++eye){
        rejected.BeginEye(eye,frame,1);rejected.Observe(0x5678,rejectedSnapshot,badView.data(),badProjection.data());
        Need(!rejected.MaterialObservationEnabled()&&!rejected.NeedsMaterialWork(),"Rejected stereo pair kept material GPU transport active");
        auto token=rejected.BeginDeferredMaterial(rejectedSnapshot.identity,badWorld.data(),"G3_Nature_Wall_Stone_01_F.xshmat",{&additional,&Additional::Copy});
        Need(!rejected.ObserveMaterialDraw(d9.Get(),1,&rejectedGeometry),"Right eye replaced after left-eye refusal");
        rejected.EndMaterial(token);rejected.EndEye();
    }
    rejected.Reset();Need(pixels()==untouched,"Asymmetric refusal changed scene pixels");
    std::ifstream rejectedLog(output/L"trial-rejected/native-static-trial.jsonl");std::string rejectedText((std::istreambuf_iterator<char>(rejectedLog)),{});
    Need(rejectedText.find("dx11_isolated_native_material_comparison")!=std::string::npos&&rejectedText.find("\"replacement_gate_passed\":false")!=std::string::npos,"Negative fixture did not exercise live color gate");


    // A passing left eye followed by a mismatched right eye can never approve
    // an owned pair, and comparison never publishes private output to the game.
    {
        g3vr::dx11::NativeStaticTrial pairRejected;pairRejected.Initialize(d9.Get(),adapter.Get(),argv[4],output/L"trial-mode5-rejected",W,H,5);primeOwned(pairRejected,499);
        for(unsigned eye=0;eye<2;++eye){
            std::array<float,1024> vc;std::array<float,896> pc;memcpy(vc.data(),vertex.data()+12+eye*1024*4,1024*4);memcpy(pc.data(),pixel.data()+12+eye*896*4,896*4);
            d9->SetVertexShader(vs.Get());d9->SetPixelShader(ps.Get());d9->SetVertexShaderConstantF(0,vc.data(),256);d9->SetPixelShaderConstantF(0,pc.data(),224);
            std::array<float,16> world{},view{},projection{};world[15]=1;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)world[c*4+r]=vc[(12+r)*4+c];
            memcpy(view.data(),cameras.data()+12+eye*128,64);memcpy(projection.data(),cameras.data()+12+eye*128+64,64);
            g3vr::dx11::NativeStaticSnapshot snapshot{};snapshot.identity[0]=77;snapshot.worldMeters=world;for(unsigned i=12;i<15;++i)snapshot.worldMeters[i]*=.01f;strcpy_s(snapshot.resource,"g3_myrtana_ardea_stairs_02.xcmsh");
            Additional source=additional;if(eye){source.alteredDevice=d9.Get();source.alteredPS=debugPS.Get();}
            d9->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff081018,1,0);auto untouchedScene=pixels();
            pairRejected.BeginEye(eye,500,1);pairRejected.Observe(0x1234,snapshot,view.data(),projection.data());Check(d9->BeginScene(),"Owned rejection begin");
            auto token=pairRejected.BeginDeferredMaterial(snapshot.identity,world.data(),"G3_Nature_Wall_Stone_01_F.xshmat",{&source,&Additional::Copy});
            diagnostic::temporal::Geometry geometry{D3DPT_TRIANGLELIST,0,0,2097,0,1130};Need(!pairRejected.ObserveMaterialDraw(d9.Get(),1,&geometry),"Owned rejection skipped native draw");
            pairRejected.EndMaterial(token);pairRejected.SolidBoundary(view.data(),projection.data());Check(d9->EndScene(),"Owned rejection end");pairRejected.EndEye();
            Need(pixels()==untouchedScene,"Failed owned proof changed native scene pixels");
        }
        std::ifstream f(output/L"trial-mode5-rejected/owned-validation.json");std::string result((std::istreambuf_iterator<char>(f)),{});
        Need(result.find("\"result\":\"REFUSED\"")!=std::string::npos&&result.find("\"same_frame_stereo_pair\":false")!=std::string::npos,"One-eye failure approved an owned stereo pair");
        pairRejected.Reset();d9->SetPixelShader(ps.Get());
    }
    // Same source address does not prove identical bytes. Observe the bytes on
    // every Capture, count hits only on exact equality, retain old pinned passes.
    {
        g3vr::dx11::NativeStonePacketProducer producer;producer.Initialize(d9.Get(),d11.Get(),context.Get());
        std::array<float,1024> vc;memcpy(vc.data(),vertex.data()+12,1024*4);std::array<float,896> pc;memcpy(pc.data(),pixel.data()+12,896*4);
        d9->SetVertexShaderConstantF(0,vc.data(),256);d9->SetPixelShaderConstantF(0,pc.data(),224);
        std::array<float,16> world{};world[15]=1;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)world[c*4+r]=vc[(12+r)*4+c];
        g3vr::dx11::NativeStaticSnapshot snapshot{};snapshot.identity[0]=77;snapshot.worldMeters=world;for(unsigned i=12;i<15;++i)snapshot.worldMeters[i]*=.01f;strcpy_s(snapshot.resource,"g3_myrtana_ardea_stairs_02.xcmsh");
        Additional source=additional;g3vr::dx11::NativeMaterialCapture localCapture;localCapture.Initialize(output/L"producer-capture",W,H,true);
        auto token=localCapture.Begin(0,600,"g3_myrtana_ardea_stairs_02.xcmsh","G3_Nature_Wall_Stone_01_F.xshmat",world.data(),{&source,&Additional::Copy});localCapture.Observe(d9.Get(),1);
        auto* draw=localCapture.Latest(0,600);Need(draw!=nullptr,"Producer fixture capture");diagnostic::temporal::Geometry geometry{D3DPT_TRIANGLELIST,0,0,2097,0,1130};g3vr::dx11::PassStamp stamp{600,10,12345,0};
        auto first=producer.Capture(*draw,snapshot,geometry,stamp);auto same=producer.Capture(*draw,snapshot,geometry,stamp);
        Need(first&&same&&producer.AtlasReads()==2&&producer.AtlasHits()==1,"Producer failed exact-content reuse");
        source.bytes[0]^=64;auto changed=producer.Capture(*draw,snapshot,geometry,stamp);
        Need(changed&&producer.AtlasReads()==3&&producer.AtlasHits()==1,"Producer used a stale pointer-identity atlas version");
        source.valid=false;Need(!producer.Capture(*draw,snapshot,geometry,stamp)&&producer.ImageCaptures()==3,"Producer failure reused old image/atlas inputs");
        auto stale=stamp;--stale.epoch;Need(!producer.Capture(*draw,snapshot,geometry,stale),"Producer accepted a stale epoch");localCapture.End(token);
    }
    Need(capture.Count()==2&&additional.calls>=12,"Continuous material evidence budget or provider failed");bool pass=coverageError==0&&covered>100&&maxSteps<=1&&changed<=std::max(uint64_t(1),covered*3/2000)&&maxFloat<.01f;
    std::ofstream f(output/L"result.json");f<<"{\"result\":\""<<(pass?"PASS":"FAIL")<<"\",\"real_live_renderer_class\":true,\"cases\":"<<draws<<",\"trial_mode4_replaced_eyes\":"<<replacedEyes<<",\"trial_mode3_eyes\":"<<trialEyes<<",\"covered_pixels\":"<<covered<<",\"coverage_disagreements\":"<<coverageError<<",\"rgb_changed_components\":"<<changed<<",\"unorm_max_steps\":"<<maxSteps<<",\"max_float_rgb_error\":"<<maxFloat<<",\"mean_rgb_error\":"<<absolute/compared<<",\"bounded_evidence_records\":"<<capture.Count()<<",\"native_noise_lightmap_shadows_gpu\":true,\"fixture_lightmap_provider\":true,\"live_sdk_provider_tested\":false,\"matching_trilinear_filter\":true,\"rgb_only_native_write_mask\":true,\"native_draw_suppressed\":false,\"game_loaded\":false}\n";f.close();Need(pass,"Live stone RGB/material coverage differs from independent native reference");std::cout<<"PASS live renderer/capture,12stereo cases, native noise/lightmap/shadows, stale/foreign inputs refused\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}



