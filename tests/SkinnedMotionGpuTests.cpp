#include "D3D9SkinnedMotion.h"
#include "D3D9AnimatedMotionProbe.h"
#include "SkinDepthFixturePS.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <memory>
#include <sstream>
using Microsoft::WRL::ComPtr;
using diagnostic::temporal::SkinPose;
using g3vr::NativeSkinVertex;
using g3vr::SkinnedMotionDraw;
void Require(bool v,char const* message){if(!v)throw std::runtime_error(message);}
void Hr(HRESULT h,char const* message){Require(SUCCEEDED(h),message);}
struct Module {HMODULE handle=nullptr;~Module(){if(handle)FreeLibrary(handle);}};
struct Window {HWND handle=CreateWindowW(L"STATIC",L"Skinned motion GPU test",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(handle)DestroyWindow(handle);}};
SkinPose Identity(){SkinPose p;for(unsigned i=0;i<64;++i)for(unsigned j=0;j<3;++j)p.bones[i*12+j*4+j]=1;for(unsigned j=0;j<4;++j)p.clip[j*4+j]=p.world[j*4+j]=1;return p;}
float Half(uint16_t h){unsigned sign=h>>15,exponent=(h>>10)&31,mantissa=h&1023;float n=exponent?std::ldexp(float(1024+mantissa),int(exponent)-25):std::ldexp(float(mantissa),-24);return sign?-n:n;}
struct Fixture {
    ComPtr<IDirect3DDevice9Ex> device;
    ComPtr<IDirect3DTexture9> nativeTarget,extraTarget,oldPalette;
    ComPtr<IDirect3DSurface9> target,extra,depth,stagingColor,stagingMotion;
    ComPtr<IDirect3DVertexShader9> nativeVs;ComPtr<IDirect3DPixelShader9> nativePs;
    ComPtr<IDirect3DVertexBuffer9> vertices;ComPtr<IDirect3DIndexBuffer9> indices;ComPtr<IDirect3DVertexDeclaration9> declaration;
    std::array<NativeSkinVertex,6> mesh;
    unsigned width=64,height=64;
    void Initialize(IDirect3DDevice9Ex* d,wchar_t const* shader){
        device=d;Hr(d->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&nativeTarget,nullptr),"native target");Hr(nativeTarget->GetSurfaceLevel(0,&target),"native surface");
        Hr(d->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&extraTarget,nullptr),"MRT target");Hr(extraTarget->GetSurfaceLevel(0,&extra),"MRT surface");
        Hr(d->CreateDepthStencilSurface(width,height,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&depth,nullptr),"native depth");
        Hr(d->CreateOffscreenPlainSurface(width,height,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&stagingColor,nullptr),"native readback fixture");
        Hr(d->CreateOffscreenPlainSurface(width,height,D3DFMT_A16B16G16R16F,D3DPOOL_SYSTEMMEM,&stagingMotion,nullptr),"motion readback fixture");
        Hr(d->CreateTexture(1,1,1,0,D3DFMT_A32B32G32R32F,D3DPOOL_DEFAULT,&oldPalette,nullptr),"original vertex texture");
        std::ifstream f(std::filesystem::path(shader),std::ios::binary|std::ios::ate);Require(bool(f),"captured native shader missing");auto size=f.tellg();Require(size>0&&size<32768&&size%4==0,"native shader size");std::vector<DWORD> code(size_t(size)/4);f.seekg(0);f.read(reinterpret_cast<char*>(code.data()),size);Require(bool(f),"native shader read");
        Hr(d->CreateVertexShader(code.data(),&nativeVs),"captured Gothic VS");Hr(d->CreatePixelShader(reinterpret_cast<DWORD const*>(SkinDepthFixturePS),&nativePs),"depth fixture PS");
        D3DVERTEXELEMENT9 elements[]={
            {0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},{0,12,D3DDECLTYPE_FLOAT4,0,D3DDECLUSAGE_BLENDWEIGHT,0},
            {0,28,D3DDECLTYPE_UBYTE4,0,D3DDECLUSAGE_BLENDINDICES,0},{0,32,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_NORMAL,0},
            {0,44,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_TANGENT,0},{0,56,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_COLOR,1},
            {0,60,D3DDECLTYPE_FLOAT2,0,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
        Hr(d->CreateVertexDeclaration(elements,&declaration),"native declaration");
        Hr(d->CreateVertexBuffer(sizeof(mesh),D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&vertices,nullptr),"native geometry buffer");
        Hr(d->CreateIndexBuffer(6,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&indices,nullptr),"native index buffer");
        void* map=nullptr;Hr(indices->Lock(0,0,&map,0),"indices lock");uint16_t values[]{0,1,2};memcpy(map,values,6);Hr(indices->Unlock(),"indices unlock");
    }
    void Upload(){void* map=nullptr;Hr(vertices->Lock(0,0,&map,D3DLOCK_DISCARD),"vertices lock");memcpy(map,mesh.data(),sizeof(mesh));Hr(vertices->Unlock(),"vertices unlock");}
    SkinnedMotionDraw Draw(SkinPose const& now,SkinPose const& before,unsigned base=0){SkinnedMotionDraw d;d.vertices=vertices;d.indices=indices;d.geometry={D3DPT_TRIANGLELIST,int32_t(base),0,3,0,1};d.current=now;d.previous=before;d.historyValid=true;return d;}
    void Native(std::vector<SkinnedMotionDraw> const& draws,float clearDepth=1,g3vr::D3D9AnimatedMotionProbe* observer=nullptr){
        device->SetDepthStencilSurface(nullptr);device->SetRenderTarget(1,nullptr);Hr(device->SetRenderTarget(0,target.Get()),"native target bind");Hr(device->SetDepthStencilSurface(depth.Get()),"native depth bind");
        D3DVIEWPORT9 viewport{0,0,width,height,0,1};device->SetViewport(&viewport);device->SetRenderState(D3DRS_ZENABLE,TRUE);device->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);device->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
        for(auto key:{D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_FOGENABLE,D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS})device->SetRenderState(key,0);
        device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);
        Hr(device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,clearDepth,0),"native clear");
        Hr(device->SetVertexDeclaration(declaration.Get()),"native declaration bind");device->SetStreamSource(0,vertices.Get(),0,68);device->SetStreamSourceFreq(0,1);device->SetIndices(indices.Get());
        Hr(device->SetVertexShader(nativeVs.Get()),"native VS bind");Hr(device->SetPixelShader(nativePs.Get()),"native PS bind");
        Hr(device->BeginScene(),"native scene");for(auto const& d:draws){device->SetVertexShaderConstantF(0,d.current.bones.data(),192);device->SetVertexShaderConstantF(192,d.current.clip.data(),4);
            if(observer){diagnostic::temporal::Object object;object.id[0]=5;object.context=123;object.visual=456;object.world=d.current.world;auto token=observer->Push(object);observer->Observe(device.Get(),d.geometry);observer->Pop(token);}
            Hr(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,d.geometry.base,0,3,0,1),"native original skin raster");}
        if(observer)observer->CaptureRaster(device.Get(),depth.Get());
        Require(!g3vr::D3D9SkinnedMotion::Replaying(),"motion replay recursion flag leaked");Hr(device->EndScene(),"native scene end");
    }
    std::vector<uint32_t> Color(){Hr(device->GetRenderTargetData(target.Get(),stagingColor.Get()),"native fixture readback");D3DLOCKED_RECT lock{};Hr(stagingColor->LockRect(&lock,nullptr,D3DLOCK_READONLY),"native fixture map");std::vector<uint32_t> result(width*height);for(unsigned y=0;y<height;++y)memcpy(result.data()+y*width,static_cast<unsigned char const*>(lock.pBits)+y*lock.Pitch,width*4);stagingColor->UnlockRect();return result;}
    std::vector<std::array<float,4>> Pixels(g3vr::D3D9SkinnedMotion& renderer){Hr(device->GetRenderTargetData(renderer.Surface(),stagingMotion.Get()),"motion fixture readback");D3DLOCKED_RECT lock{};Hr(stagingMotion->LockRect(&lock,nullptr,D3DLOCK_READONLY),"motion fixture map");std::vector<std::array<float,4>> result(width*height);for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){auto p=reinterpret_cast<uint16_t const*>(static_cast<unsigned char const*>(lock.pBits)+y*lock.Pitch)+x*4;for(unsigned c=0;c<4;++c)result[y*width+x][c]=Half(p[c]);}stagingMotion->UnlockRect();return result;}
};
diagnostic::temporal::Vertex Vertex(NativeSkinVertex const& v){diagnostic::temporal::Vertex p;for(unsigned k=0;k<3;++k)p.position[k]=v.position[k];for(unsigned k=0;k<4;++k){p.weights[k]=v.weights[k];p.indices[k]=v.indices[k];}return p;}
bool Reference(SkinnedMotionDraw const& d,NativeSkinVertex const* triangle,unsigned x,unsigned y,unsigned w,unsigned h,float (&motion)[2],bool& valid){
    float now[3][4],old[3][4];double screen[3][2];bool previous=true;
    for(unsigned i=0;i<3;++i){Require(diagnostic::temporal::Project(d.current,Vertex(triangle[i]),now[i]),"CPU current projection invalid");previous&=diagnostic::temporal::Project(d.previous,Vertex(triangle[i]),old[i]);screen[i][0]=(double(now[i][0])/now[i][3]+1)*w*.5;screen[i][1]=(1-double(now[i][1])/now[i][3])*h*.5;}
    auto edge=[](double const* a,double const* b,double px,double py){return (px-a[0])*(b[1]-a[1])-(py-a[1])*(b[0]-a[0]);};
    double area=edge(screen[1],screen[2],screen[0][0],screen[0][1]);double b[3]{edge(screen[1],screen[2],x,y)/area,edge(screen[2],screen[0],x,y)/area,edge(screen[0],screen[1],x,y)/area};
    if(*std::min_element(b,b+3)<.035)return false;double denominator=0,a[4]{},p[4]{};for(unsigned i=0;i<3;++i)denominator+=b[i]/now[i][3];
    for(unsigned i=0;i<3;++i)for(unsigned c=0;c<4;++c){double weight=b[i]/now[i][3]/denominator;a[c]+=weight*now[i][c];p[c]+=weight*old[i][c];}
    valid=d.historyValid&&previous&&p[3]>1.e-5&&p[2]>=0&&p[2]<=p[3]&&std::fabs(p[0])<=p[3]&&std::fabs(p[1])<=p[3];motion[0]=valid?float((p[0]/p[3]-a[0]/a[3])*w*.5):0;motion[1]=valid?float((a[1]/a[3]-p[1]/p[3])*h*.5):0;return true;
}
struct Totals {unsigned cases=0,pixels=0;double maxError=0;};
void Run(Fixture& f,g3vr::D3D9SkinnedMotion& renderer,std::vector<SkinnedMotionDraw> const& draws,Totals& total,bool occluded=false,std::array<float,2> currentJitter={},std::array<float,2> previousJitter={}){
    f.Upload();f.Native(draws,occluded?.1f:1.f);auto color=f.Color();
    // Deliberately non-default native state, including a vertex texture and
    // second render target. The companion pass must restore all of it.
    f.device->SetRenderTarget(1,f.extra.Get());f.device->SetTexture(D3DVERTEXTEXTURESAMPLER0,f.oldPalette.Get());f.device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
    f.device->SetStreamSourceFreq(0,D3DSTREAMSOURCE_INDEXEDDATA|1);
    D3DVIEWPORT9 saved{2,3,58,56,0,1};f.device->SetViewport(&saved);RECT scissor{4,5,50,51};f.device->SetScissorRect(&scissor);f.device->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
    std::array<float,1024> constants{};f.device->GetVertexShaderConstantF(0,constants.data(),256);float pc[4]{.1f,.2f,.3f,.4f};f.device->SetPixelShaderConstantF(0,pc,1);
    D3DRENDERSTATETYPE keys[]={D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ZFUNC,D3DRS_CULLMODE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_COLORWRITEENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_FOGENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS};
    DWORD states[sizeof(keys)/sizeof(keys[0])]{};for(unsigned i=0;i<sizeof(states)/sizeof(states[0]);++i)f.device->GetRenderState(keys[i],&states[i]);
    Hr(f.device->BeginScene(),"motion scene");auto report=renderer.Render(f.depth.Get(),draws.data(),unsigned(draws.size()),currentJitter,previousJitter);Hr(f.device->EndScene(),"motion scene end");Require(report.draws==draws.size(),"motion skipped draw");
    D3DVIEWPORT9 after{};f.device->GetViewport(&after);Require(!memcmp(&after,&saved,sizeof(after)),"native viewport changed");RECT rect{};f.device->GetScissorRect(&rect);Require(!memcmp(&rect,&scissor,sizeof(rect)),"native scissor changed");
    std::array<float,1024> afterConstants{};f.device->GetVertexShaderConstantF(0,afterConstants.data(),256);Require(afterConstants==constants,"native VS constants changed");float afterPc[4]{};f.device->GetPixelShaderConstantF(0,afterPc,1);Require(!memcmp(pc,afterPc,sizeof(pc)),"native PS constants changed");
    for(unsigned i=0;i<sizeof(states)/sizeof(states[0]);++i){DWORD value=0;f.device->GetRenderState(keys[i],&value);Require(value==states[i],"native render state changed");}
    UINT frequency=0;f.device->GetStreamSourceFreq(0,&frequency);Require(frequency==(D3DSTREAMSOURCE_INDEXEDDATA|1),"native stream frequency changed");
    ComPtr<IDirect3DVertexShader9> vs;ComPtr<IDirect3DPixelShader9> ps;ComPtr<IDirect3DVertexDeclaration9> decl;
    f.device->GetVertexShader(&vs);f.device->GetPixelShader(&ps);f.device->GetVertexDeclaration(&decl);Require(vs.Get()==f.nativeVs.Get()&&ps.Get()==f.nativePs.Get()&&decl.Get()==f.declaration.Get(),"native shaders/declaration changed");
    ComPtr<IDirect3DSurface9> target,depth,extra;f.device->GetRenderTarget(0,&target);f.device->GetRenderTarget(1,&extra);f.device->GetDepthStencilSurface(&depth);Require(target.Get()==f.target.Get()&&extra.Get()==f.extra.Get()&&depth.Get()==f.depth.Get(),"native targets/depth changed");
    ComPtr<IDirect3DBaseTexture9> texture;f.device->GetTexture(D3DVERTEXTEXTURESAMPLER0,&texture);Require(texture.Get()==f.oldPalette.Get(),"native vertex texture changed");DWORD sampler=0;f.device->GetSamplerState(D3DVERTEXTEXTURESAMPLER0,D3DSAMP_ADDRESSU,&sampler);Require(sampler==D3DTADDRESS_WRAP,"native vertex sampler changed");
    Require(f.Color()==color,"companion pass altered native scene color");auto pixels=f.Pixels(renderer);unsigned checked=0,occupied=0;
    for(unsigned y=0;y<f.height;++y)for(unsigned x=0;x<f.width;++x){auto const& pixel=pixels[y*f.width+x];for(float value:pixel)Require(std::isfinite(value),"nonfinite raster motion");if(pixel[2]>.5f)++occupied;
        if(occluded){Require(pixel[2]==0&&pixel[3]==0,"motion ignored opaque depth / cleared native depth");continue;}
        for(auto const& d:draws){float expected[2];bool valid=false;if(!Reference(d,f.mesh.data()+d.geometry.base,x,y,f.width,f.height,expected,valid))continue;
            Require(pixel[2]>.5f,"native-depth equality lost an interior skin pixel");Require((pixel[3]>.5f)==valid,"wrong motion history validity mask");
            if(valid){expected[0]-=previousJitter[0]-currentJitter[0];expected[1]-=previousJitter[1]-currentJitter[1];}
            for(unsigned c=0;c<2;++c){double error=std::fabs(pixel[c]-expected[c]);total.maxError=std::max(total.maxError,error);if(error>.12){std::printf("pixel%u,%u channel%u actual%.6f expected%.6f error%.6f\n",x,y,c,pixel[c],expected[c],error);throw std::runtime_error("GPU motion disagrees with CPU four-weight/perspective reference");}}
            ++checked;break;}}
    if(!occluded)Require(checked>50&&occupied>50,"insufficient raster coverage");total.pixels+=checked;++total.cases;
}
int wmain(int argc,wchar_t** argv){try{
    Require(argc==3||argc==4,"Pass actual x86 DXVK, captured native VS and optional captured-pose cases");Module module;module.handle=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Require(module.handle,"DXVK load");
    auto create=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(module.handle,"Direct3DCreate9Ex"));Require(create,"DXVK API");ComPtr<IDirect3D9Ex> api;Hr(create(D3D_SDK_VERSION,&api),"D3D9Ex");Window window;Require(window.handle,"hidden window");
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.handle;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;ComPtr<IDirect3DDevice9Ex> device;
    Hr(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.handle,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&device),"real DXVK device");Fixture f;f.Initialize(device.Get(),argv[2]);
    auto baseline=device->AddRef();device->Release();g3vr::D3D9SkinnedMotion left,right;left.Initialize(device.Get(),64,64);right.Initialize(device.Get(),64,64);Require(left.Texture()!=right.Texture(),"eye motion textures alias");Totals total;
    auto now=Identity(),old=now;float positions[3][3]{{-.75f,-.65f,.5f},{.75f,-.65f,.5f},{0,.75f,.5f}};
    for(unsigned i=0;i<3;++i){for(unsigned c=0;c<3;++c)f.mesh[i].position[c]=positions[i][c];for(unsigned j=0;j<4;++j){f.mesh[i].weights[j]=float(j+1)*.1f;f.mesh[i].indices[j]=static_cast<unsigned char>(j);}}
    for(unsigned i=0;i<4;++i){old.bones[i*12+3]=-.1f+float(i)*.02f;old.bones[i*12+7]=.05f-float(i)*.01f;}
    Run(f,left,{f.Draw(now,old)},total);old=now;old.clip[3]=.125f;Run(f,right,{f.Draw(now,old)},total);
    auto noHistory=f.Draw(now,old);noHistory.historyValid=false;Run(f,left,{noHistory},total);old=now;old.clip[15]=-1;Run(f,left,{f.Draw(now,old)},total);
    old=now;old.clip[3]=3;Run(f,left,{f.Draw(now,old)},total);
    old=now;Run(f,left,{f.Draw(now,old)},total,true);
    now.clip[3]=2*.25f/64;now.clip[7]=-2*(-.25f)/64;Run(f,left,{f.Draw(now,old)},total,false,{.25f,-.25f});
    now=Identity();old=now;now.clip[10]=old.clip[10]=.5f;now.clip[14]=old.clip[14]=1;now.clip[15]=old.clip[15]=0;old.clip[3]=.05f;
    for(unsigned i=0;i<3;++i){float z=float(i+1);f.mesh[i].position[0]=positions[i][0]*z;f.mesh[i].position[1]=positions[i][1]*z;f.mesh[i].position[2]=z;}Run(f,left,{f.Draw(now,old)},total);
    // Two different atlas rows and geometry ranges in the same raster pass.
    now=Identity();old=now;old.bones[3]=-.125f;for(unsigned i=0;i<3;++i){f.mesh[i].position[0]=positions[i][0]*.45f-.5f;f.mesh[i].position[1]=positions[i][1];f.mesh[i].position[2]=.5f;f.mesh[i+3]=f.mesh[i];f.mesh[i+3].position[0]+=1;}
    auto second=f.Draw(now,now,3);second.previous.clip[7]=.125f;Run(f,left,{f.Draw(now,old),second},total);
    if(argc==4){std::ifstream file(std::filesystem::path(argv[3]),std::ios::binary);uint32_t header[4]{};file.read(reinterpret_cast<char*>(header),sizeof(header));Require(file&&header[0]==0x4d543347&&header[1]==1&&header[2]>0&&header[2]<=32&&header[3]==0,"captured pose packet header");
        for(unsigned i=0;i<header[2];++i){uint32_t identity[10]{};SkinPose current,previous;file.read(reinterpret_cast<char*>(identity),sizeof(identity));file.read(reinterpret_cast<char*>(&current),sizeof(current));file.read(reinterpret_cast<char*>(&previous),sizeof(previous));file.read(reinterpret_cast<char*>(f.mesh.data()),68*3);Require(file&&identity[0]<2&&identity[1]==identity[2]+1&&current.Valid()&&previous.Valid(),"captured pose packet record");Run(f,identity[0]?right:left,{f.Draw(current,previous)},total);}
        char trailing=0;Require(!file.read(&trailing,1),"captured packet trailing bytes");}
    // Actual observer -> per-eye history -> GPU buffer retention -> opaque
    // callback replay -> binary readback, including a read inside BeginScene.
    auto observer=std::make_unique<g3vr::D3D9AnimatedMotionProbe>();observer->width=observer->height=64;
    for(unsigned i=0;i<3;++i)for(unsigned c=0;c<3;++c)f.mesh[i].position[c]=positions[i][c];f.Upload();
    auto before=Identity(),current=before;before.bones[3]=-.1f;
    observer->Begin(0,100,1000000000ull);f.Native({f.Draw(before,before)},1,observer.get());observer->Stop();
    observer->Begin(0,101,1033333333ull);f.Native({f.Draw(current,before)},1,observer.get());observer->Stop();
    auto prefix="motion-probe-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64());auto directory=std::filesystem::temp_directory_path();
    std::ostringstream raster;Hr(device->BeginScene(),"diagnostic readback scene");bool saved=observer->WriteRaster(raster,0,directory,prefix);Hr(device->EndScene(),"diagnostic readback scene end");
    Require(saved&&raster.str().find("\"xr_frame_serial\":101")!=std::string::npos,"observer raster export failed");
    auto rasterFile=directory/(prefix+"-motion-101-eye0.rgba16f");Require(std::filesystem::file_size(rasterFile)==64*64*8,"observer raster file size");
    std::vector<uint16_t> pixels(64*64*4);std::ifstream rasterInput(rasterFile,std::ios::binary);rasterInput.read(reinterpret_cast<char*>(pixels.data()),pixels.size()*2);Require(bool(rasterInput),"observer raster file read");rasterInput.close();unsigned validPixels=0;
    for(unsigned i=0;i<64*64;++i)if(Half(pixels[i*4+3])>.5f)++validPixels;Require(validPixels>50,"observer exported no valid skin pixels");std::filesystem::remove(rasterFile);
    observer->Begin(0,102,2000000000ull);f.Native({f.Draw(current,current)},1,observer.get());observer->Stop();std::ostringstream stale;observer->Write(stale,0);Require(stale.str().find("\"history_status\":\"frame_gap_or_reset\"")!=std::string::npos,"large display-time gap reused skin history");observer.reset();
    auto bad=f.Draw(Identity(),Identity());bad.geometry.vertices=UINT32_MAX;bool refused=false;try{left.Render(f.depth.Get(),&bad,1);}catch(std::exception const&){refused=true;}Require(refused,"out-of-bounds geometry accepted");
    Hr(device->BeginScene(),"empty motion scene");left.Render(f.depth.Get(),nullptr,0);Hr(device->EndScene(),"empty motion end");for(auto p:f.Pixels(left))Require(p==std::array<float,4>{},"empty frame retained motion");
    left.Reset();right.Reset();auto final=device->AddRef();device->Release();Require(final==baseline,"motion renderer retained native device/resources");
    std::printf("PASS %u real DXVK skin-motion raster cases, %u interior pixels, max CPU-reference error %.6f pixels; original Gothic shader depth, 4 bones, camera/perspective/jitter/occlusion, validity masks, atlas rows, stereo targets, bounds, empty frame and native-state/resource restoration. Full game scene/NGX not enabled.\n",total.cases,total.pixels,total.maxError);return 0;
}catch(std::exception const& e){std::printf("FAIL %s\n",e.what());return 1;}}
