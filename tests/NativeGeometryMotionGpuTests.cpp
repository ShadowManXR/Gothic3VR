#include "D3D9NativeMotion.h"
#include "D3D9NativeMotionCapture.h"
#include "TemporalInstanceIdentity.h"
#include "DxvkTemporalBridge.h"
#include "GpuTemporalDepth.h"
#include "GpuTemporalMotion.h"
#include "GameTemporalInputs.h"
#include "D3D9UiCapture.h"
#include "NativeDepthFixturePS.h"
#include <dxgi1_2.h>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstring>
#include <functional>
#include <thread>
#include <sstream>
using Microsoft::WRL::ComPtr;
using namespace diagnostic::temporal;
using g3vr::NativeMotionDraw;
void Require(bool b,char const* s){if(!b)throw std::runtime_error(s);}
void Hr(HRESULT h,char const* s){Require(SUCCEEDED(h),s);}
struct Module{HMODULE value=nullptr;~Module(){if(value)FreeLibrary(value);}};
struct Window{HWND value=CreateWindowW(L"STATIC",L"Native geometry motion",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(value)DestroyWindow(value);}};
struct NativeFixtureVertex{float position[3]{},normal[3]{0,1,0};uint32_t color=0xffffffff,lighting=0xffffffff;float uv[2]{},wind[2]{},instance=0,uv7[3]{},tangent[3]{1,0,0};};
static_assert(sizeof(NativeFixtureVertex)==76);
NativeGeometryPose Identity(NativeGeometryKind kind){
    NativeGeometryPose p;
    if(kind==NativeGeometryKind::RigidWorld){for(unsigned c=0;c<3;++c)p.constants[(4+c)*4+c]=1;}
    if(kind==NativeGeometryKind::InstancesA||kind==NativeGeometryKind::InstancesB)for(unsigned i=0;i<17;++i)for(unsigned c=0;c<3;++c)p.constants[(i*3+c)*4+c]=1;
    unsigned vp=kind==NativeGeometryKind::RigidWorld?0:kind==NativeGeometryKind::Wind?7:68;for(unsigned c=0;c<4;++c)p.constants[(vp+c)*4+c]=1;
    p.constants[22*4+1]=1000;return p;
}
struct Fixture {
    ComPtr<IDirect3DDevice9Ex> d;ComPtr<IDirect3DTexture9> color,extra;
    ComPtr<IDirect3DSurface9> target,extraTarget,depth,colorStaging,motionStaging;
    ComPtr<IDirect3DVertexBuffer9> vb;ComPtr<IDirect3DIndexBuffer9> ib;
    ComPtr<IDirect3DVertexDeclaration9> nativeDecl,minimal[4];ComPtr<IDirect3DPixelShader9> nativePs;
    std::array<ComPtr<IDirect3DVertexShader9>,4> nativeVs;std::array<NativeFixtureVertex,6> mesh;
    unsigned w=96,h=80;
    void Initialize(IDirect3DDevice9Ex* device,std::filesystem::path const& directory){
        d=device;Hr(d->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&color,nullptr),"native color");Hr(color->GetSurfaceLevel(0,&target),"native color surface");
        Hr(d->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&extra,nullptr),"native extra target");Hr(extra->GetSurfaceLevel(0,&extraTarget),"extra surface");
        Hr(d->CreateDepthStencilSurface(w,h,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&depth,nullptr),"native opaque depth");
        Hr(d->CreateOffscreenPlainSurface(w,h,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&colorStaging,nullptr),"test-only color readback");Hr(d->CreateOffscreenPlainSurface(w,h,D3DFMT_A16B16G16R16F,D3DPOOL_SYSTEMMEM,&motionStaging,nullptr),"test-only motion readback");
        for(unsigned i=0;i<4;++i){char name[80];std::snprintf(name,sizeof(name),"temporal-vs-%llx.cso",static_cast<unsigned long long>(NativeGeometryHash(NativeGeometryKind(i))));std::ifstream f(directory/name,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(f)),{});Require(f&&bytes.size()>16&&bytes.size()%4==0,"captured native geometry shader");Hr(d->CreateVertexShader(reinterpret_cast<DWORD const*>(bytes.data()),&nativeVs[i]),"original native shader");
            D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},{0,WORD(i==3?40:48),D3DDECLTYPE_FLOAT1,0,D3DDECLUSAGE_TEXCOORD,BYTE(i==3?1:4)},D3DDECL_END()};
            if(i==0)elements[1]=D3DDECL_END();Hr(d->CreateVertexDeclaration(elements,&minimal[i]),"minimal native position declaration");}
        D3DVERTEXELEMENT9 full[]={{0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},{0,12,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_NORMAL,0},{0,24,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_COLOR,0},{0,28,D3DDECLTYPE_D3DCOLOR,0,D3DDECLUSAGE_COLOR,1},{0,32,D3DDECLTYPE_FLOAT2,0,D3DDECLUSAGE_TEXCOORD,0},{0,40,D3DDECLTYPE_FLOAT2,0,D3DDECLUSAGE_TEXCOORD,1},{0,48,D3DDECLTYPE_FLOAT1,0,D3DDECLUSAGE_TEXCOORD,4},{0,52,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_TEXCOORD,7},{0,64,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_TANGENT,0},D3DDECL_END()};
        Hr(d->CreateVertexDeclaration(full,&nativeDecl),"original full native declaration");Hr(d->CreatePixelShader(reinterpret_cast<DWORD const*>(NativeDepthFixturePS),&nativePs),"fixture native color shader");
        Hr(d->CreateVertexBuffer(sizeof(mesh),D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&vb,nullptr),"native VB");Hr(d->CreateIndexBuffer(6,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr),"native IB");void* map=nullptr;Hr(ib->Lock(0,0,&map,0),"native IB map");uint16_t indices[]{0,1,2};std::memcpy(map,indices,6);Hr(ib->Unlock(),"native IB unmap");
    }
    NativeMotionDraw Draw(NativeGeometryKind kind,NativeGeometryPose const& now,NativeGeometryPose const& before,unsigned base=0){NativeMotionDraw r;r.kind=kind;r.vertices=vb;r.indices=ib;r.declaration=minimal[unsigned(kind)];r.stride=sizeof(NativeFixtureVertex);r.geometry={D3DPT_TRIANGLELIST,int32_t(base),0,3,0,1};r.current=now;r.previous=before;r.historyValid=true;r.instanceHistory.fill(1);return r;}
    void Upload(){void* p=nullptr;Hr(vb->Lock(0,0,&p,D3DLOCK_DISCARD),"native NativeFixtureVertex upload");std::memcpy(p,mesh.data(),sizeof(mesh));Hr(vb->Unlock(),"native NativeFixtureVertex upload end");}
    void Native(std::vector<NativeMotionDraw> const& draws,bool occluded=false,std::function<void(NativeMotionDraw const&)> const& observe={},IDirect3DVertexShader9* shaderOverride=nullptr,float const* constantsOverride=nullptr){
        d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(1,nullptr);Hr(d->SetRenderTarget(0,target.Get()),"native target bind");Hr(d->SetDepthStencilSurface(depth.Get()),"native depth bind");D3DVIEWPORT9 vp{0,0,w,h,0,1};d->SetViewport(&vp);
        for(auto k:{D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_FOGENABLE,D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS})d->SetRenderState(k,0);
        d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
        Hr(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,occluded?.01f:1,0),"native clear");d->SetVertexDeclaration(nativeDecl.Get());d->SetPixelShader(nativePs.Get());d->SetStreamSource(0,vb.Get(),0,sizeof(NativeFixtureVertex));d->SetStreamSourceFreq(0,1);d->SetIndices(ib.Get());Hr(d->BeginScene(),"native scene");
        for(auto const& r:draws){d->SetVertexShader(shaderOverride?shaderOverride:nativeVs[unsigned(r.kind)].Get());d->SetVertexShaderConstantF(0,constantsOverride?constantsOverride:r.current.constants.data(),constantsOverride?256:83);auto const& g=r.geometry;if(observe)observe(r);Hr(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,g.base,0,3,0,1),"native original shader raster");}Hr(d->EndScene(),"native end");
    }
    std::vector<uint32_t> Color(){Hr(d->GetRenderTargetData(target.Get(),colorStaging.Get()),"test color readback");D3DLOCKED_RECT lock{};Hr(colorStaging->LockRect(&lock,nullptr,D3DLOCK_READONLY),"color map");std::vector<uint32_t> pixels(w*h);for(unsigned y=0;y<h;++y)memcpy(pixels.data()+y*w,static_cast<char*>(lock.pBits)+y*lock.Pitch,w*4);colorStaging->UnlockRect();return pixels;}
    static float Half(uint16_t h){unsigned s=h>>15,e=(h>>10)&31,m=h&1023;if(e==31)return m?std::numeric_limits<float>::quiet_NaN():s?-std::numeric_limits<float>::infinity():std::numeric_limits<float>::infinity();float v=e?std::ldexp(float(1024+m),int(e)-25):std::ldexp(float(m),-24);return s?-v:v;}
    std::vector<std::array<float,4>> Pixels(g3vr::D3D9NativeMotion& r){Hr(d->GetRenderTargetData(r.Surface(),motionStaging.Get()),"test-only motion readback");D3DLOCKED_RECT lock{};Hr(motionStaging->LockRect(&lock,nullptr,D3DLOCK_READONLY),"test motion map");std::vector<std::array<float,4>> p(w*h);for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){auto* row=reinterpret_cast<uint16_t*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)+x*4;for(unsigned c=0;c<4;++c)p[y*w+x][c]=Half(row[c]);}motionStaging->UnlockRect();return p;}
};
NativeGeometryVertex RefVertex(NativeFixtureVertex const& v,NativeGeometryKind kind){NativeGeometryVertex r;std::copy_n(v.position,3,r.position);r.auxiliary=kind==NativeGeometryKind::Wind?v.wind[0]:v.instance;return r;}
bool Reference(NativeMotionDraw const& d,NativeFixtureVertex const* vertices,unsigned x,unsigned y,unsigned w,unsigned h,float (&motion)[2],bool& valid){
    float now[3][4],before[3][4];double screen[3][2];for(unsigned i=0;i<3;++i){Require(ProjectNative(d.kind,d.current,RefVertex(vertices[i],d.kind),now[i]),"CPU current native position invalid");Require(ProjectNative(d.kind,d.previous,RefVertex(vertices[i],d.kind),before[i]),"CPU previous native position invalid");screen[i][0]=(double(now[i][0])/now[i][3]+1)*w*.5;screen[i][1]=(1-double(now[i][1])/now[i][3])*h*.5;}
    auto edge=[](double const* a,double const* b,double px,double py){return (px-a[0])*(b[1]-a[1])-(py-a[1])*(b[0]-a[0]);};double area=edge(screen[1],screen[2],screen[0][0],screen[0][1]);if(std::abs(area)<1.e-10)return false;
    double b[]{edge(screen[1],screen[2],x,y)/area,edge(screen[2],screen[0],x,y)/area,edge(screen[0],screen[1],x,y)/area};if(*std::min_element(b,b+3)<.06)return false;double denominator=0,a[4]{},p[4]{};for(unsigned i=0;i<3;++i)denominator+=b[i]/now[i][3];for(unsigned i=0;i<3;++i)for(unsigned c=0;c<4;++c){double weight=b[i]/now[i][3]/denominator;a[c]+=weight*now[i][c];p[c]+=weight*before[i][c];}
    bool identity=d.kind!=NativeGeometryKind::InstancesA&&d.kind!=NativeGeometryKind::InstancesB?true:d.instanceHistory[unsigned(std::floor(vertices[0].instance))]!=0;
    valid=d.historyValid&&identity&&p[3]>1.e-5&&p[2]>=0&&p[2]<=p[3]&&std::abs(p[0])<=p[3]&&std::abs(p[1])<=p[3];motion[0]=valid?float((p[0]/p[3]-a[0]/a[3])*w*.5):0;motion[1]=valid?float((a[1]/a[3]-p[1]/p[3])*h*.5):0;return true;
}
struct Totals{unsigned cases=0,pixels=0;double error=0;};
void CaptureObserverProof(Fixture& f){
    auto capture=std::make_unique<g3vr::D3D9NativeMotionCapture>();capture->Prepare();
    float triangle[3][3]{{-.75f,-.65f,.5f},{.75f,-.65f,.5f},{0,.75f,.5f}};
    for(unsigned i=0;i<3;++i){std::copy_n(triangle[i],3,f.mesh[i].position);f.mesh[i].instance=3.25f;f.mesh[i].wind[0]=.8f;}f.Upload();
    EntityIdentity id;id.value={1,2,3,4,1};
    for(unsigned frame=1;frame<=5;++frame){
        if(frame==3){std::thread write([&]{f.Upload();});write.join();}
        if(frame==5){void* p=nullptr;Hr(f.ib->Lock(0,0,&p,0),"observer IB write");Hr(f.ib->Unlock(),"observer IB write done");}
        for(unsigned eye=0;eye<2;++eye){
            auto kind=eye?NativeGeometryKind::Wind:NativeGeometryKind::InstancesA;auto pose=Identity(kind);
            InstanceEntity entity;entity.id=id;for(unsigned i=0;i<4;++i)entity.world[i*5]=1;
            if(!eye){
                for(unsigned row=0;row<17;++row)pose.constants[(row*3)*4+3]=float(row*20);
                pose.constants[9*4+3]=float(frame)*.015f;entity.world[12]=pose.constants[9*4+3];
            }else{pose.constants[23*4]=.1f;pose.constants[24*4]=.4f;pose.constants[25*4]=float(frame)*.1f;}
            capture->Begin(eye,frame,1000000000ull+frame*11111111ull,f.w,f.h);auto token=capture->Push(&entity,1);
            auto draw=f.Draw(kind,pose,pose);f.Native({draw},false,[&](NativeMotionDraw const& r){capture->Observe(f.d.Get(),r.geometry);});
            capture->Pop(token);Hr(f.d->BeginScene(),"observer raster scene");capture->Capture(f.d.Get(),f.depth.Get());Hr(f.d->EndScene(),"observer raster end");capture->Stop();
            Require(capture->stats[eye].accepted==1&&capture->stats[eye].rejected==0,"actual native observer rejected supported original draw");
            bool valid=frame==2||frame==4;Require(capture->stats[eye].history==unsigned(valid),"actual Lock/Unlock did not invalidate buffer history");
            auto* renderer=capture->Renderer(eye);Require(renderer!=nullptr,"actual observer did not produce a raster");
            unsigned occupied=0,history=0;for(auto pixel:f.Pixels(*renderer)){occupied+=pixel[2]==1;history+=pixel[3]==1;}
            Require(occupied>100&&(valid?history>100:history==0),"actual observer raster validity disagrees with native writes");
            auto indexed=capture->stats[eye].indexed;capture->Observe(f.d.Get(),draw.geometry);Require(capture->stats[eye].indexed==indexed,"Stopped native observer accepted late draws");
            std::ostringstream metadata;capture->Write(metadata,eye,{},"fixture",false);for(unsigned char c:metadata.str())Require(c>=32,"Native metadata contains raw uint8 control characters");
            if(!eye)Require(metadata.str().find("\"identified_instance_rows\":[0,0,0,1,")!=std::string::npos,"Native instance row metadata is not numeric JSON");
        }
    }
    for(unsigned frame:{6u,7u}){auto pose=Identity(NativeGeometryKind::Wind);auto draw=f.Draw(NativeGeometryKind::Wind,pose,pose);
        capture->Begin(0,frame,1000000000ull+frame*11111111ull,f.w,f.h);f.Native({draw},false,[&](NativeMotionDraw const& r){capture->Observe(f.d.Get(),r.geometry);});
        if(frame==7){Hr(f.d->BeginScene(),"missing phase recovery");capture->Capture(f.d.Get(),f.depth.Get());Hr(f.d->EndScene(),"missing phase recovery end");Require(capture->stats[0].history==0,"Unclosed previous native phase granted history");}
        capture->Stop();
    }
    capture->Reset();capture->Begin(0,1,1000000000,f.w,f.h);
    auto rejectedPose=Identity(NativeGeometryKind::RigidWorld);auto rejectedDraw=f.Draw(NativeGeometryKind::RigidWorld,rejectedPose,rejectedPose);
    f.Native({rejectedDraw},false,[&](NativeMotionDraw const& draw){
        for(auto pair:{std::pair<D3DRENDERSTATETYPE,DWORD>{D3DRS_ZENABLE,FALSE},{D3DRS_COLORWRITEENABLE,0},{D3DRS_ALPHABLENDENABLE,TRUE},{D3DRS_ALPHATESTENABLE,TRUE},{D3DRS_STENCILENABLE,TRUE},{D3DRS_CLIPPLANEENABLE,1}}){DWORD before=0,oldFunc=0;Hr(f.d->GetRenderState(pair.first,&before),"refusal save state");Hr(f.d->GetRenderState(D3DRS_STENCILFUNC,&oldFunc),"refusal save stencil compare");if(pair.first==D3DRS_STENCILENABLE)Hr(f.d->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_NEVER),"filtering stencil test");Hr(f.d->SetRenderState(pair.first,pair.second),"refusal set state");capture->Observe(f.d.Get(),draw.geometry);Hr(f.d->SetRenderState(pair.first,before),"refusal restore state");Hr(f.d->SetRenderState(D3DRS_STENCILFUNC,oldFunc),"restore stencil compare");}
        RECT before{};Hr(f.d->GetScissorRect(&before),"refusal save scissor");RECT partial{1,0,LONG(f.w),LONG(f.h)};f.d->SetScissorRect(&partial);f.d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);capture->Observe(f.d.Get(),draw.geometry);f.d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);f.d->SetScissorRect(&before);
        D3DVIEWPORT9 full{};f.d->GetViewport(&full);D3DVIEWPORT9 inset{1,0,f.w-1,f.h,0,1};f.d->SetViewport(&inset);capture->Observe(f.d.Get(),draw.geometry);f.d->SetViewport(&full);
    });capture->Stop();Require(capture->stats[0].indexed==8&&capture->stats[0].rejected==8&&capture->stats[0].accepted==0,"Unsupported native passes were granted motion");
    std::ostringstream refusals;capture->Write(refusals,0,{},"fixture",false);
    Require(refusals.str().find("\"rejection_reasons\":{\"viewport\":1,\"state_query\":0,\"depth_mode\":1,\"colorless_pass\":1,\"alpha_blend\":1,\"alpha_test\":1,\"stencil\":1,\"clip_planes\":1,\"partial_scissor\":1,\"other\":0}")!=std::string::npos,"Native refusal telemetry does not describe actual rejected passes");
    // Reproduce actual world-pass ALWAYS stencil, including the two-sided
    // counterexample absent from the older capture's limited state record.
    capture->Reset();capture->Begin(0,1,1000000000,f.w,f.h);
    InstanceEntity stencilEntity;stencilEntity.id.value={1,2,3,4,1};for(unsigned i=0;i<4;++i)stencilEntity.world[i*5]=1;
    auto stencilScope=capture->Push(&stencilEntity,1);
    f.Native({rejectedDraw},false,[&](NativeMotionDraw const& draw){
        Hr(f.d->SetRenderState(D3DRS_STENCILENABLE,TRUE),"opaque stencil enabled");
        Hr(f.d->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_ALWAYS),"opaque always stencil");
        Hr(f.d->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE,FALSE),"single sided stencil");
        capture->Observe(f.d.Get(),draw.geometry);Require(capture->stats[0].accepted==1,"Unconditional opaque stencil was still rejected");
        f.d->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE,TRUE);f.d->SetRenderState(D3DRS_CCW_STENCILFUNC,D3DCMP_NEVER);
        capture->Observe(f.d.Get(),draw.geometry);Require(capture->stats[0].accepted==1&&capture->stats[0].rejected==1,"Filtering reverse-face stencil was granted motion");
        f.d->SetRenderState(D3DRS_CCW_STENCILFUNC,D3DCMP_ALWAYS);capture->Observe(f.d.Get(),draw.geometry);
        Require(capture->stats[0].accepted==1&&capture->stats[0].duplicates==1,"Both unconditional stencil faces rejected");
    });capture->Pop(stencilScope);capture->Stop();f.d->SetRenderState(D3DRS_STENCILENABLE,FALSE);f.d->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE,FALSE);
    capture->Reset();capture->Prepare();
    for(unsigned frame:{1u,2u}){InstanceEntity group[2];for(unsigned i=0;i<2;++i){group[i].id.value={10+i,2,3,4,1};for(unsigned k=0;k<4;++k)group[i].world[k*5]=1;group[i].world[12]=(i?.2f:-.2f)+(frame-1)*.01f;}
        capture->Begin(0,frame,1000000000ull+frame*11111111ull,f.w,f.h);auto token=capture->Push(group,2);std::vector<NativeMotionDraw> draws;
        for(unsigned i=0;i<2;++i){auto pose=Identity(NativeGeometryKind::InstancesA);pose.constants[9*4+3]=group[i].world[12];draws.push_back(f.Draw(NativeGeometryKind::InstancesA,pose,pose));}
        f.Native(draws,false,[&](NativeMotionDraw const& r){capture->Observe(f.d.Get(),r.geometry);});capture->Pop(token);Hr(f.d->BeginScene(),"instance group capture");capture->Capture(f.d.Get(),f.depth.Get());Hr(f.d->EndScene(),"instance group end");
        auto indexed=capture->stats[0].indexed;capture->Observe(f.d.Get(),draws[0].geometry);Require(capture->stats[0].indexed==indexed,"Closed opaque phase accepted a late draw");capture->Stop();
        Require(capture->stats[0].accepted==2&&capture->stats[0].duplicates==0&&capture->stats[0].history==(frame==2?2u:0u),"Different entity groups sharing native geometry were confused");
    }
    // A stale native callback already fetched before vtable restoration must
    // still forward to the original after the observer is destroyed.
    using Lock=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,UINT,UINT,void**,DWORD);
    using Unlock=HRESULT(STDMETHODCALLTYPE*)(IUnknown*);
    auto** table=*reinterpret_cast<void***>(f.vb.Get());auto queuedLock=reinterpret_cast<Lock>(table[11]);auto queuedUnlock=reinterpret_cast<Unlock>(table[12]);
    capture.reset();void* data=nullptr;Hr(queuedLock(f.vb.Get(),0,0,&data,D3DLOCK_DISCARD),"queued Lock after observer destruction");Hr(queuedUnlock(f.vb.Get()),"queued Unlock after observer destruction");
    // A new observer must be able to reuse restored shared dispatch tables.
    g3vr::D3D9BufferObserver again;again.Begin(1);Require(again.Observe(f.vb.Get())&&again.Observe(f.ib.Get()),"reinstall buffer observers");
    auto before=again.Version(f.vb.Get(),f.ib.Get());f.Upload();auto after=again.Version(f.vb.Get(),f.ib.Get());Require(before.valid&&after.valid&&!before.Same(after),"reinstalled hooks lost native writes");again.Reset();
    again.Begin(2);Require(again.Observe(f.vb.Get())&&again.Observe(f.ib.Get()),"ProcessVertices observer setup");before=again.Version(f.vb.Get(),f.ib.Get());
    auto process=f.d->ProcessVertices(0,0,3,f.vb.Get(),f.minimal[0].Get(),0);after=again.Version(f.vb.Get(),f.ib.Get());
    Require(SUCCEEDED(process)?!before.Same(after):before.Same(after),"ProcessVertices success/failure did not preserve buffer version contract");again.Reset();
    std::printf("PASS native draw observer: original VS/getters -> both-eye history/raster; native worker VB writes and IB writes invalidate; stale callbacks forward and hooks reinstall\n");
}
void EndToEnd(Fixture& f,IDirect3D9Ex* api,g3vr::D3D9NativeMotion& left,g3vr::D3D9NativeMotion& right,std::filesystem::path const& directory){
    LUID luid{};Hr(api->GetAdapterLUID(0,&luid),"native adapter LUID");ComPtr<IDXGIFactory1> factory;Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");ComPtr<IDXGIAdapter1> adapter;
    for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> next;if(factory->EnumAdapters1(i,&next)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 desc{};Hr(next->GetDesc1(&desc),"DXGI description");if(!memcmp(&luid,&desc.AdapterLuid,sizeof(luid))){adapter=next;break;}}Require(adapter.Get()!=nullptr,"Matching geometry transfer adapter");
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context),"native D3D11 device");
    g3vr::DxvkTemporalBridge bridge;Require(bridge.Initialize(f.d.Get()),"native geometry GPU bridge");g3vr::D3D9NativeMotion* renderers[]{&left,&right};
    ComPtr<ID3D11Texture2D> raw[2],motion[2];GpuTemporalDepth decoders[2];GpuTemporalMotion composers[2];
    for(unsigned i=0;i<2;++i){auto encoding=bridge.Register(f.depth.Get(),device.Get(),raw[i]);Require(bridge.Register(renderers[i]->Surface(),device.Get(),motion[i])==g3vr::TemporalEncoding::Color16F,"native motion encoding");decoders[i].Initialize(device.Get(),f.w,f.h,encoding);composers[i].Initialize(device.Get(),f.w,f.h);}
    std::vector<std::array<float,4>> expected[2];float positions[3][3]{{-.75f,-.65f,.5f},{.75f,-.65f,.5f},{0,.75f,.5f}};
    for(unsigned eye=0;eye<2;++eye){auto kind=eye?NativeGeometryKind::Wind:NativeGeometryKind::InstancesA;for(unsigned i=0;i<3;++i){std::copy_n(positions[i],3,f.mesh[i].position);f.mesh[i].instance=3.25f;f.mesh[i].wind[0]=eye?.8f:0;}
        auto now=Identity(kind),old=now;if(eye){now.constants[23*4]=.15f;now.constants[23*4+1]=.08f;now.constants[24*4]=.4f;now.constants[24*4+1]=.25f;old=now;old.constants[25*4]=.7f;}else old.constants[9*4+3]=.1f;
        auto draw=f.Draw(kind,now,old);f.Upload();f.Native({draw});Hr(f.d->BeginScene(),"native chained motion begin");renderers[eye]->Render(f.depth.Get(),&draw,1);Hr(f.d->EndScene(),"native chained motion end");
        bridge.Queue(motion[eye].Get());bridge.Queue(raw[eye].Get());bridge.Complete();expected[eye]=f.Pixels(*renderers[eye]);
    }
    // Raw pixels above are read ONLY to verify the output below. The path
    // connecting native raster, bridge, decode and compositor stays on GPU.
    D3D11_TEXTURE2D_DESC t{};t.Width=f.w;t.Height=f.h;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    std::vector<uint16_t> zero(size_t(f.w)*f.h*4);D3D11_SUBRESOURCE_DATA data{zero.data(),f.w*8,0};ComPtr<ID3D11Texture2D> emptySkin;ComPtr<ID3D11ShaderResourceView> skinView;Hr(device->CreateTexture2D(&t,&data,&emptySkin),"empty skin fixture");Hr(device->CreateShaderResourceView(emptySkin.Get(),nullptr,&skinView),"empty skin view");
    std::vector<unsigned char> unknown(size_t(f.w)*f.h);t.Format=DXGI_FORMAT_R8_UNORM;data={unknown.data(),f.w,0};ComPtr<ID3D11Texture2D> staticMask;ComPtr<ID3D11ShaderResourceView> staticView;Hr(device->CreateTexture2D(&t,&data,&staticMask),"unknown static fixture");Hr(device->CreateShaderResourceView(staticMask.Get(),nullptr,&staticView),"unknown static view");
    for(unsigned eye=0;eye<2;++eye){decoders[eye].Process(context.Get(),raw[eye].Get());ComPtr<ID3D11ShaderResourceView> depthView,motionView;Hr(device->CreateShaderResourceView(decoders[eye].Texture(),nullptr,&depthView),"decoded native depth view");Hr(device->CreateShaderResourceView(motion[eye].Get(),nullptr,&motionView),"native geometry motion view");CameraMotionFrame frame;for(unsigned i=0;i<4;++i)frame.currentClipToPreviousClip[i*5]=1;frame.historyValid=true;composers[eye].ProcessLayers(context.Get(),depthView.Get(),skinView.Get(),motionView.Get(),staticView.Get(),frame);
        D3D11_TEXTURE2D_DESC desc{};composers[eye].Texture()->GetDesc(&desc);desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> check;Hr(device->CreateTexture2D(&desc,nullptr,&check),"test-only composed readback");context->CopyResource(check.Get(),composers[eye].Texture());D3D11_MAPPED_SUBRESOURCE map{};Hr(context->Map(check.Get(),0,D3D11_MAP_READ,0,&map),"test-only composed map");
        unsigned occupied=0;for(unsigned y=0;y<f.h;++y)for(unsigned x=0;x<f.w;++x){auto* p=reinterpret_cast<uint16_t*>(static_cast<char*>(map.pData)+y*map.RowPitch)+x*4;auto const& e=expected[eye][y*f.w+x];occupied+=e[2]==1;for(unsigned c=0;c<4;++c)Require(Fixture::Half(p[c])==e[c],"Native raster -> GPU bridge/decode/composition changed motion or masks");}context->Unmap(check.Get(),0);Require(occupied>100,"Native chained pipeline had no geometry");}
    context->ClearState();D3D11_QUERY_DESC q{D3D11_QUERY_EVENT,0};ComPtr<ID3D11Query> done;Hr(device->CreateQuery(&q,&done),"native chain fence");context->End(done.Get());context->Flush();auto start=GetTickCount64();BOOL complete=FALSE;for(;;){auto hr=context->GetData(done.Get(),&complete,sizeof(complete),D3D11_ASYNC_GETDATA_DONOTFLUSH);Hr(hr,"native chain fence failure");if(hr==S_OK){Require(complete!=FALSE,"native chain fence incomplete");break;}Require(GetTickCount64()-start<2000,"Native chain fence timeout");SwitchToThread();}bridge.ReleaseReads();bridge.Reset();
    std::puts("PASS actual GPU chain for both eyes: native instanced/wind raster -> DXVK GPU transport -> R32 depth decode -> D3D11 combined motion, bit-exact including shared depth overwrite");
    // Exercise the SAME orchestration class linked into StereoRuntime. Verify
    // both final images after the native depth was overwritten by the other eye.
    ComPtr<IDirect3DTexture9> emptySkin9;ComPtr<IDirect3DSurface9> emptySurface;
    Hr(f.d->CreateTexture(f.w,f.h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&emptySkin9,nullptr),"live GPU skin texture");Hr(emptySkin9->GetSurfaceLevel(0,&emptySurface),"live GPU skin surface");
    f.d->SetDepthStencilSurface(nullptr);f.d->SetRenderTarget(0,emptySurface.Get());Hr(f.d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"live GPU empty skin clear");
    g3vr::GameTemporalInputs live;live.Initialize(f.d.Get(),device.Get(),context.Get(),f.w,f.h);std::array<float,16> identity{};for(unsigned i=0;i<4;++i)identity[i*5]=1;
    for(unsigned frame:{1u,2u,4u,5u}){
        bool history=frame==2||frame==5;
        for(unsigned eye=0;eye<2;++eye){auto kind=eye?NativeGeometryKind::Wind:NativeGeometryKind::InstancesA;
            for(unsigned i=0;i<3;++i){std::copy_n(positions[i],3,f.mesh[i].position);f.mesh[i].instance=3.25f;f.mesh[i].wind[0]=eye?.8f:0;}
            auto now=Identity(kind),old=now;if(eye){now.constants[23*4]=.15f;now.constants[24*4]=.4f;old=now;old.constants[25*4]=.8f;}else old.constants[9*4+3]=.1f;
            auto draw=f.Draw(kind,now,old);f.Upload();f.Native({draw});Hr(f.d->BeginScene(),"live GPU native motion");renderers[eye]->Render(f.depth.Get(),&draw,1);
            GeometryFrameStamp stamp{frame,7,1000000000ull+frame*11111111ull,eye};auto result=live.Capture(eye,stamp,identity.data(),identity.data(),f.depth.Get(),emptySurface.Get(),renderers[eye]->Surface());
            Hr(f.d->EndScene(),"live GPU native motion end");expected[eye]=f.Pixels(*renderers[eye]);
            Require(result.cameraHistory==history&&result.frame==frame&&result.eye==eye,"Live GPU eye history/gap contract");
            bool duplicate=false;try{live.Capture(eye,stamp,identity.data(),identity.data(),f.depth.Get(),emptySurface.Get(),renderers[eye]->Surface());}catch(std::exception const&){duplicate=true;}Require(duplicate,"Live GPU duplicate eye accepted");
        }
        for(unsigned eye=0;eye<2;++eye){auto* output=live.Motion(eye);Require(output&&live.Depth(eye),"Live GPU output missing");D3D11_TEXTURE2D_DESC desc{};output->GetDesc(&desc);desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;Hr(device->CreateTexture2D(&desc,nullptr,&staging),"live GPU test-only staging");context->CopyResource(staging.Get(),output);D3D11_MAPPED_SUBRESOURCE read{};Hr(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&read),"live GPU test-only read");
            unsigned occupied=0;for(unsigned y=0;y<f.h;++y)for(unsigned x=0;x<f.w;++x){auto const* actual=reinterpret_cast<uint16_t const*>(static_cast<char*>(read.pData)+y*read.RowPitch)+x*4;auto expectedPixel=expected[eye][y*f.w+x];occupied+=expectedPixel[2]==1;if(!history){expectedPixel[0]=expectedPixel[1]=expectedPixel[3]=0;}for(unsigned c=0;c<4;++c)Require(Fixture::Half(actual[c])==expectedPixel[c],"Live GPU orchestration changed actual motion/coverage");}context->Unmap(staging.Get(),0);Require(occupied>100,"Live GPU fixture had no occupied motion");
        }
    }live.Reset();Require(!live.Motion(0)&&!live.Depth(1),"Live GPU reset retained old frame outputs");
    std::puts("PASS StereoRuntime GPU-input orchestrator: six real shared exports, independent eyes, frame gaps, duplicate rejection, bit-exact native motion and reset; unknown scene pixels stay invalid");
    {
        g3vr::DxvkGpuBridge nativeBridge;Require(nativeBridge.Initialize(f.d.Get()),"live draw-hook native bridge");
        auto ui=std::make_unique<g3vr::UiCapture>();ui->Initialize(f.d.Get(),device.Get(),f.w,f.h,&nativeBridge);ui->temporal.Prepare();
        InstanceEntity entity;entity.id.value={9,8,7,6,1};for(unsigned i=0;i<4;++i)entity.world[i*5]=1;
        for(unsigned frame:{1u,2u}){
            auto pose=Identity(NativeGeometryKind::RigidWorld);pose.constants[4*4+3]=float(frame)*.02f;entity.world[12]=pose.constants[4*4+3];
            ui->temporal.Begin(0,frame,f.w,f.h,1000000000ull+frame*11111111ull);ui->temporal.Matrices(identity.data(),identity.data());auto scope=ui->temporal.PushInstances(&entity,1);Require(ui->BeginWorldTiming(),"actual draw hook measurement");
            f.Native({f.Draw(NativeGeometryKind::RigidWorld,pose,pose)});ui->temporal.PopInstances(scope);Hr(f.d->BeginScene(),"actual draw hook replay scene");ui->temporal.CaptureMotion(f.d.Get(),f.depth.Get());Hr(f.d->EndScene(),"actual draw hook replay end");
            diagnostic::performance::EyeDrawSample timing;ui->EndWorldTiming(timing);ui->temporal.End(0);std::ostringstream metadata;ui->temporal.Write(metadata,0,{},"fixture",false);
            Require(metadata.str().find("\"indexed_draws\":1,\"accepted\":1")!=std::string::npos,"Actual draw hook recaptured its own companion draw");
            Require(metadata.str().find(frame==2?"\"history_draws\":1":"\"history_draws\":0")!=std::string::npos,"Actual draw-hook history did not advance");Require(ui->temporal.NativeSurface(0)&&ui->temporal.SkinSurface(0),"Live hook did not provide both GPU layers");
        }
        // Real sky program and all three direct draw entry points. Sky often
        // has depth disabled, so an indexed/depth-writing survey misses it.
        auto skyFile=directory/L"temporal-vs-67ecae639f0848ac.cso";std::ifstream skyInput(skyFile,std::ios::binary);std::vector<char> skyCode((std::istreambuf_iterator<char>(skyInput)),{});
        Require(skyInput&&skyCode.size()>16&&skyCode.size()%4==0,"Native sky fixture missing");ComPtr<IDirect3DVertexShader9> skyShader;Hr(f.d->CreateVertexShader(reinterpret_cast<DWORD const*>(skyCode.data()),&skyShader),"native sky shader");
        D3DVERTEXELEMENT9 skyElements[]={{0,0,D3DDECLTYPE_FLOAT4,0,D3DDECLUSAGE_POSITION,0},D3DDECL_END()};ComPtr<IDirect3DVertexDeclaration9> skyDeclaration;Hr(f.d->CreateVertexDeclaration(skyElements,&skyDeclaration),"native sky declaration");
        float skyVertices[4][4]{{-1,-1,1,1},{-1,1,1,1},{1,-1,1,1},{1,1,1,1}};uint16_t skyIndices[]{0,1,2,2,1,3};
        ComPtr<IDirect3DVertexBuffer9> skyBuffer;Hr(f.d->CreateVertexBuffer(sizeof(skyVertices),D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&skyBuffer,nullptr),"native sky VB");void* skyMap=nullptr;Hr(skyBuffer->Lock(0,0,&skyMap,0),"native sky upload");memcpy(skyMap,skyVertices,sizeof(skyVertices));Hr(skyBuffer->Unlock(),"native sky upload end");
        unsigned serial=3;for(unsigned kind:{0u,2u,3u}){
            ui->temporal.Begin(0,serial,f.w,f.h,1000000000ull+serial*11111111ull);ui->temporal.Matrices(identity.data(),identity.data());Require(ui->BeginWorldTiming(),"sky hook measurement");
            f.d->SetDepthStencilSurface(nullptr);f.d->SetRenderTarget(1,nullptr);f.d->SetRenderTarget(0,f.target.Get());f.d->SetDepthStencilSurface(f.depth.Get());D3DVIEWPORT9 viewport{0,0,f.w,f.h,0,1};f.d->SetViewport(&viewport);
            f.d->SetVertexShader(skyShader.Get());f.d->SetPixelShader(f.nativePs.Get());f.d->SetVertexDeclaration(skyDeclaration.Get());f.d->SetStreamSource(0,skyBuffer.Get(),0,16);f.d->SetStreamSourceFreq(0,1);
            std::array<float,256*4> skyConstants{};for(unsigned c=0;c<4;++c)skyConstants[c*5]=1;for(unsigned c=0;c<3;++c)skyConstants[(4+c)*4+c]=1;skyConstants[8*4+1]=1;skyConstants[10*4+2]=1;
            skyConstants[12*4]=skyConstants[12*4+1]=skyConstants[13*4]=skyConstants[13*4+1]=.1f;skyConstants[14*4]=.03f;skyConstants[14*4+1]=.04f;f.d->SetVertexShaderConstantF(0,skyConstants.data(),256);
            for(auto state:{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_SCISSORTESTENABLE})f.d->SetRenderState(state,FALSE);f.d->SetRenderState(D3DRS_COLORWRITEENABLE,15);f.d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
            Hr(f.d->BeginScene(),"native sky draw scene");if(kind==0)Hr(f.d->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2),"native sky DrawPrimitive");else if(kind==2)Hr(f.d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,skyVertices,16),"native sky DrawPrimitiveUP");else Hr(f.d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,4,2,skyIndices,D3DFMT_INDEX16,skyVertices,16),"native sky DrawIndexedPrimitiveUP");Hr(f.d->EndScene(),"native sky draw end");
            diagnostic::performance::EyeDrawSample timing;ui->EndWorldTiming(timing);ui->temporal.End(0);std::ostringstream metadata;ui->temporal.Write(metadata,0,{},"fixture",false);auto text=metadata.str();
            Require(text.find("\"native_sky_program\":true")!=std::string::npos&&text.find("\"z_enable\":0")!=std::string::npos&&text.find("\"draw_kind\":"+std::to_string(kind))!=std::string::npos,"Actual direct draw hook missed native sky/depth-disabled pass");
            Require(text.find("\"indexed_draws\":0,\"accepted\":0")!=std::string::npos,"Direct sky survey invented native motion jobs");
            if(kind>=2)Require(text.find("\"user_pointer_geometry_not_retained\":true")!=std::string::npos&&text.find("\"stream0\":[0,0,0,0,0]")!=std::string::npos,"UP survey attributed a stale bound VB to caller data");++serial;
        }
        std::puts("PASS original sky program through real DrawPrimitive/UP/IndexedUP hooks: depth-disabled metadata, no stale user geometry or invented motion");
        ui->Reset();ui.reset();nativeBridge.Reset();
    }
    std::puts("PASS actual UiCapture D3D9 vtable hooks: native draws reach the live collector; replay recursion excluded; both layers produced and teardown restores hooks");
}
void Run(Fixture& f,g3vr::D3D9NativeMotion& renderer,std::vector<NativeMotionDraw> draws,Totals& total,bool occluded=false,IDirect3DVertexShader9* shaderOverride=nullptr,float const* constantsOverride=nullptr){
    std::printf("Case%u kind%u instance%.2f wind%.2f history%d occluded%d\n",total.cases,unsigned(draws[0].kind),f.mesh[0].instance,f.mesh[0].wind[0],draws[0].historyValid,occluded);
    f.Upload();f.Native(draws,occluded,{},shaderOverride,constantsOverride);auto originalColor=f.Color();f.d->SetRenderTarget(1,f.extraTarget.Get());f.d->SetStreamSourceFreq(0,D3DSTREAMSOURCE_INDEXEDDATA|1);D3DVIEWPORT9 vp{2,3,80,64,.1f,.9f};f.d->SetViewport(&vp);RECT scissor{4,5,72,61};f.d->SetScissorRect(&scissor);f.d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
    std::array<float,1024> constants{};f.d->GetVertexShaderConstantF(0,constants.data(),256);float pc[]{.1f,.2f,.3f,.4f};f.d->SetPixelShaderConstantF(0,pc,1);
    D3DRENDERSTATETYPE keys[]{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ZFUNC,D3DRS_CULLMODE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_COLORWRITEENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_FOGENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS};std::array<DWORD,13> states{};for(unsigned i=0;i<states.size();++i)f.d->GetRenderState(keys[i],&states[i]);f.d->SetStreamSource(0,f.vb.Get(),4,sizeof(NativeFixtureVertex));f.d->SetStreamSource(1,f.vb.Get(),8,sizeof(NativeFixtureVertex));
    ComPtr<IDirect3DVertexShader9> nativeVs;f.d->GetVertexShader(&nativeVs);Hr(f.d->BeginScene(),"companion scene");auto report=renderer.Render(f.depth.Get(),draws.data(),unsigned(draws.size()));Hr(f.d->EndScene(),"companion end");Require(report.draws==draws.size(),"Native motion skipped draw");Require(!g3vr::D3D9NativeMotion::Replaying(),"native replay flag leaked");
    D3DVIEWPORT9 actual{};f.d->GetViewport(&actual);Require(!memcmp(&vp,&actual,sizeof(vp)),"native viewport changed");RECT rectangle{};f.d->GetScissorRect(&rectangle);Require(!memcmp(&rectangle,&scissor,sizeof(scissor)),"native scissor changed");DWORD state=0;f.d->GetRenderState(D3DRS_SCISSORTESTENABLE,&state);Require(state==TRUE,"native scissor state changed");
    std::array<float,1024> after{};f.d->GetVertexShaderConstantF(0,after.data(),256);Require(after==constants,"native VS constants changed");float afterPc[4]{};f.d->GetPixelShaderConstantF(0,afterPc,1);Require(!memcmp(pc,afterPc,sizeof(pc)),"native PS constants changed");for(unsigned i=0;i<states.size();++i){DWORD value=0;f.d->GetRenderState(keys[i],&value);Require(value==states[i],"Native render state changed");}ComPtr<IDirect3DVertexBuffer9> restoredVb;UINT offset=0,stride=0;f.d->GetStreamSource(0,&restoredVb,&offset,&stride);Require(restoredVb==f.vb&&offset==4&&stride==sizeof(NativeFixtureVertex),"Native VB/offset/stride changed");ComPtr<IDirect3DPixelShader9> restoredPs;f.d->GetPixelShader(&restoredPs);Require(restoredPs==f.nativePs,"Native pixel shader changed");
    f.d->GetStreamSource(1,&restoredVb,&offset,&stride);Require(restoredVb==f.vb&&offset==8&&stride==sizeof(NativeFixtureVertex),"Native auxiliary stream changed");UINT freq=0;f.d->GetStreamSourceFreq(0,&freq);Require(freq==(D3DSTREAMSOURCE_INDEXEDDATA|1),"native stream frequency changed");ComPtr<IDirect3DVertexShader9> restored;f.d->GetVertexShader(&restored);Require(restored==nativeVs,"native shader changed");ComPtr<IDirect3DVertexDeclaration9> decl;f.d->GetVertexDeclaration(&decl);Require(decl==f.nativeDecl,"native declaration changed");ComPtr<IDirect3DSurface9> target,depth,extra;f.d->GetRenderTarget(0,&target);f.d->GetRenderTarget(1,&extra);f.d->GetDepthStencilSurface(&depth);Require(target==f.target&&depth==f.depth&&extra==f.extraTarget,"native render/depth targets changed");Require(f.Color()==originalColor,"native color overwritten");
    auto pixels=f.Pixels(renderer);unsigned checked=0,occupied=0;for(unsigned y=0;y<f.h;++y)for(unsigned x=0;x<f.w;++x){auto const& pixel=pixels[y*f.w+x];for(auto v:pixel)Require(std::isfinite(v),"Nonfinite native motion");occupied+=pixel[2]>.5f;if(occluded){Require(pixel==std::array<float,4>{},"Motion ignored actual opaque depth");continue;}
        for(auto const& d:draws){float expected[2];bool valid=false;if(!Reference(d,f.mesh.data()+d.geometry.base,x,y,f.w,f.h,expected,valid))continue;Require(pixel[2]==1,"Original-native depth EQUAL lost interior geometry");Require(pixel[3]==float(valid),"Native history mask incorrect");for(unsigned c=0;c<2;++c){double error=std::abs(pixel[c]-expected[c]);total.error=std::max(total.error,error);if(error>.12){std::printf("kind%u pixel%u,%u channel%u actual%f expected%f\n",unsigned(d.kind),x,y,c,pixel[c],expected[c]);throw std::runtime_error("Native position motion differs from CPU reference");}}++checked;break;}}
    if(!occluded)Require(checked>80&&occupied>80,"Insufficient native companion coverage");total.pixels+=checked;++total.cases;
}
void AdditionalNativeProfiles(Fixture& f,g3vr::D3D9NativeMotion& renderer,Totals& total,std::filesystem::path const& directory){
    Require(!FindNativeShaderProfile(123),"Unknown native shader matched a profile");
    unsigned additional=0;
    float triangle[3][3]{{-.7f,-.6f,.5f},{.7f,-.6f,.5f},{0,.7f,.5f}};
    for(auto const& profile:NativeShaderProfiles){if(profile.canonical||profile.kind==NativeGeometryKind::Wind)continue;++additional;
        char name[80];std::snprintf(name,sizeof(name),"temporal-vs-%llx.cso",static_cast<unsigned long long>(profile.hash));
        std::ifstream stream(directory/name,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(stream)),{});
        Require(stream&&bytes.size()>16&&bytes.size()%4==0,"Additional saved native shader missing");ComPtr<IDirect3DVertexShader9> native;
        Hr(f.d->CreateVertexShader(reinterpret_cast<DWORD const*>(bytes.data()),&native),"additional original VS");
        for(unsigned row:{0u,3u,16u}){
            for(unsigned i=0;i<3;++i){std::copy_n(triangle[i],3,f.mesh[i].position);f.mesh[i].instance=float(row)+.25f;}
            auto makeRaw=[&](float shift){std::array<float,1024> raw{};bool rigid=profile.kind==NativeGeometryKind::RigidWorld;
                for(unsigned instance=0;instance<(rigid?1u:17u);++instance){unsigned base=profile.matrix+3*instance;
                    raw[(base+0)*4]=.92f;raw[(base+0)*4+1]=.07f;raw[(base+0)*4+2]=-.03f;
                    raw[(base+1)*4]=-.05f;raw[(base+1)*4+1]=.96f;raw[(base+1)*4+2]=.025f;
                    raw[(base+2)*4]=.01f;raw[(base+2)*4+1]=-.02f;raw[(base+2)*4+2]=.9f;
                    raw[(base+0)*4+3]=instance==row||rigid?shift:float(instance+1)*20;
                }
                for(unsigned c=0;c<4;++c)raw[(profile.projection+c)*4+c]=1;
                raw[(profile.projection+0)*4+1]=.02f;raw[(profile.projection+2)*4]=.007f;
                raw[(profile.projection+3)*4+2]=.12f;return raw;};
            auto raw=makeRaw(.035f),previous=makeRaw(-.025f);NativeGeometryPose now,old;
            Require(NormalizeNativePose(profile,raw.data(),256,now)&&NormalizeNativePose(profile,previous.data(),256,old),"Native register normalization failed");
            auto unchanged=old;Require(!NormalizeNativePose(profile,raw.data(),profile.projection+3,old)&&old.constants==unchanged.constants,"Truncated register file accepted or output modified");
            Run(f,renderer,{f.Draw(profile.kind,now,old)},total,false,native.Get(),raw.data());
        }
        // The production collector must read high registers, identify actual
        // instance rows and advance history; this is not just a shader demo.
        auto capture=std::make_unique<g3vr::D3D9NativeMotionCapture>();
        for(unsigned frame:{1u,2u}){
            bool rigid=profile.kind==NativeGeometryKind::RigidWorld;std::array<float,1024> raw{};
            unsigned matrix=profile.matrix+(rigid?0:16*3);for(unsigned c=0;c<3;++c)raw[(matrix+c)*4+c]=1;
            raw[matrix*4+3]=frame*.02f;for(unsigned c=0;c<4;++c)raw[(profile.projection+c)*4+c]=1;
            NativeGeometryPose pose;Require(NormalizeNativePose(profile,raw.data(),256,pose),"Production pose fixture invalid");
            InstanceEntity entity;entity.id.value={22,33,44,55,1};for(unsigned c=0;c<4;++c)entity.world[c*5]=1;entity.world[12]=frame*.02f;
            for(unsigned i=0;i<3;++i)f.mesh[i].instance=16.25f;if(frame==1)f.Upload();
            capture->Begin(0,frame,1000000000ull+frame*11111111ull,f.w,f.h);auto token=capture->Push(&entity,1);
            auto draw=f.Draw(profile.kind,pose,pose);f.Native({draw},false,[&](NativeMotionDraw const& r){capture->Observe(f.d.Get(),r.geometry);},native.Get(),raw.data());capture->Pop(token);
            Hr(f.d->BeginScene(),"additional observer scene");capture->Capture(f.d.Get(),f.depth.Get());Hr(f.d->EndScene(),"additional observer end");
            Require(capture->stats[0].accepted==1&&capture->stats[0].history==frame-1,"Additional production native profile/history failed");
            unsigned valid=0;for(auto pixel:f.Pixels(*capture->Renderer(0)))valid+=pixel[3]==1;
            Require(frame==1?valid==0:valid>100,"Additional production native raster history failed");
            // The survey must keep a visible ZWRITE=0 color pass even after
            // the opaque boundary, without adding that late draw to motion.
            auto survey=std::make_unique<g3vr::D3D9ShaderSurvey>();survey->Begin(0,frame,f.w,f.h);
            f.d->SetRenderState(D3DRS_COLORWRITEENABLE,0);survey->Observe(f.d.Get(),draw.geometry,false,&entity,1);
            f.d->SetRenderState(D3DRS_COLORWRITEENABLE,7);f.d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);f.d->SetRenderState(D3DRS_ZFUNC,D3DCMP_EQUAL);
            survey->Observe(f.d.Get(),draw.geometry,true,&entity,1);std::ostringstream evidence;survey->Write(evidence,0,{},false);
            Require(evidence.str().find("\"sample_priority\":3,\"after_opaque_boundary\":true")!=std::string::npos&&evidence.str().find("\"z_write\":0")!=std::string::npos&&evidence.str().find(name)!=std::string::npos,"Survey lost a late visible color pass");
            capture->Observe(f.d.Get(),draw.geometry);Require(capture->stats[0].accepted==1,"Late shader survey modified frozen motion history");capture->Stop();
        }
        // Reference-space reset keeps reserved job capacity but destroys the
        // survey. Begin must recreate it without needing a separate Prepare.
        capture->Reset();capture->Begin(0,1,1000000000,f.w,f.h);capture->Stop();
    }
    std::printf("PASS %u additional exact native programs: original GPU depth vs normalized companion; non-diagonal transforms, high-register instances, production identities/history, late color survey and reset\n",additional);
}
void WindNativeProfiles(Fixture& f,g3vr::D3D9NativeMotion& left,g3vr::D3D9NativeMotion& right,Totals& total,std::filesystem::path const& directory){
    unsigned profiles=0;
    for(auto const& profile:NativeShaderProfiles){if(profile.canonical||profile.kind!=NativeGeometryKind::Wind)continue;++profiles;
        char name[80];std::snprintf(name,sizeof(name),"temporal-vs-%llx.cso",static_cast<unsigned long long>(profile.hash));
        std::ifstream stream(directory/name,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(stream)),{});Require(stream&&bytes.size()>16&&bytes.size()%4==0,"Original wind program missing");ComPtr<IDirect3DVertexShader9> shader;Hr(f.d->CreateVertexShader(reinterpret_cast<DWORD const*>(bytes.data()),&shader),"original wind VS");
        auto rawPose=[&](float phase,bool proximity){std::array<float,1024> raw{};for(unsigned i=0;i<4;++i)raw[(profile.projection+i)*4+i]=1;
            raw[profile.projection*4+1]=.02f;raw[(profile.projection+1)*4]=-.01f;
            raw[profile.windPlayer*4]=proximity?-129.8f:0;raw[profile.windPlayer*4+1]=proximity?-130.f:1000.f;
            raw[profile.windAmplitude*4]=.035f;raw[profile.windAmplitude*4+1]=.02f;
            raw[profile.windWavelength*4]=.4f;raw[profile.windWavelength*4+1]=.8f;raw[profile.windPhase*4]=phase;raw[profile.windPhase*4+1]=-.3f*phase;return raw;};
        for(bool proximity:{false,true})for(float weight:{0.f,.8f,-.25f}){
            float triangle[3][3]{{-.7f,-.6f,.5f},{.7f,-.6f,.5f},{0,.7f,.5f}};for(unsigned i=0;i<3;++i){std::copy_n(triangle[i],3,f.mesh[i].position);f.mesh[i].wind[0]=weight;}
            auto raw=rawPose(.1f,proximity),oldRaw=rawPose(.7f,proximity);NativeGeometryPose now,old;
            Require(NormalizeNativePose(profile,raw.data(),256,now)&&NormalizeNativePose(profile,oldRaw.data(),256,old),"Wind register normalization failed");
            auto sentinel=old;unsigned needed=std::max({profile.projection+4,profile.windPlayer+1,profile.windAmplitude+1,profile.windWavelength+1,profile.windPhase+1});
            Require(!NormalizeNativePose(profile,raw.data(),needed-1,old)&&old.constants==sentinel.constants,"Truncated wind constants modified/authorized pose");
            Run(f,proximity?right:left,{f.Draw(profile.kind,now,old)},total,false,shader.Get(),raw.data());
        }
        auto capture=std::make_unique<g3vr::D3D9NativeMotionCapture>();
        for(unsigned frame:{1u,2u}){auto raw=rawPose(frame*.1f,false);NativeGeometryPose pose;Require(NormalizeNativePose(profile,raw.data(),256,pose),"Wind observer raw pose");for(auto& v:f.mesh)v.wind[0]=.8f;if(frame==1)f.Upload();
            capture->Begin(0,frame,1000000000ull+frame*11111111ull,f.w,f.h);auto draw=f.Draw(profile.kind,pose,pose);
            f.Native({draw},false,[&](NativeMotionDraw const& r){capture->Observe(f.d.Get(),r.geometry);},shader.Get(),raw.data());
            Hr(f.d->BeginScene(),"wind observer companion");capture->Capture(f.d.Get(),f.depth.Get());Hr(f.d->EndScene(),"wind observer companion end");
            Require(capture->stats[0].accepted==1&&capture->stats[0].history==frame-1,"Actual wind variant observer/history failed");
            unsigned valid=0;for(auto pixel:f.Pixels(*capture->Renderer(0)))valid+=pixel[3]==1;Require(frame==1?valid==0:valid>100,"Wind observer did not rasterize history");capture->Stop();
        }
        // Recognizing position must never bypass the separate material guard.
        capture->Begin(0,3,1033333333ull,f.w,f.h);auto raw=rawPose(.3f,false);NativeGeometryPose pose;NormalizeNativePose(profile,raw.data(),256,pose);
        f.Native({f.Draw(profile.kind,pose,pose)},false,[&](NativeMotionDraw const& r){f.d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);capture->Observe(f.d.Get(),r.geometry);f.d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);},shader.Get(),raw.data());
        Require(capture->stats[0].accepted==0&&capture->stats[0].rejected==1,"Wind position proof bypassed unknown alpha coverage");capture->Stop();
    }
    Require(profiles==4,"Exact wind proof profile count changed without updating validation");
    std::printf("PASS %u exact wind variants: original VS depth vs normalized companion, both eyes, zero/positive/negative weights, wind phase, player proximity, observer history, alpha rejection\n",profiles);
}
#include "NativeAlphaMotionCases.h"
int wmain(int argc,wchar_t** argv){try{
    Require(argc==3||(argc==4&&(std::wstring(argv[3])==L"--wind-only"||std::wstring(argv[3])==L"--alpha-only")),"Pass x32 D3D9 DLL and actual captured shader directory [--wind-only|--alpha-only]");Module module;module.value=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Require(module.value!=nullptr,"Load native D3D9 backend");auto create=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(module.value,"Direct3DCreate9Ex"));Require(create!=nullptr,"D3D9 export");ComPtr<IDirect3D9Ex> api;Hr(create(D3D_SDK_VERSION,&api),"D3D9 API");Window window;D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.value;pp.BackBufferWidth=96;pp.BackBufferHeight=80;pp.BackBufferFormat=D3DFMT_A8R8G8B8;ComPtr<IDirect3DDevice9Ex> native;Hr(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&native),"real native device");
    Fixture f;f.Initialize(native.Get(),argv[2]);auto baseline=native->AddRef();native->Release();g3vr::D3D9NativeMotion left,right;left.Initialize(native.Get(),f.w,f.h);right.Initialize(native.Get(),f.w,f.h);Require(left.Texture()!=right.Texture(),"Native motion eye textures alias");Totals total;
    if(argc==4){if(std::wstring(argv[3])==L"--alpha-only")AlphaNativeProfiles(f,argv[2]);else WindNativeProfiles(f,left,right,total,argv[2]);left.Reset();right.Reset();auto final=native->AddRef();native->Release();Require(final==baseline,"Focused shader cases leaked native device references");std::printf("PASS %u wind GPU cases, %u pixels, maximum error %.6f pixels\n",total.cases,total.pixels,total.error);return 0;}
    float positions[3][3]{{-.75f,-.65f,.5f},{.75f,-.65f,.5f},{0,.75f,.5f}};
    for(unsigned k=0;k<4;++k){auto kind=NativeGeometryKind(k);for(unsigned i=0;i<3;++i){std::copy_n(positions[i],3,f.mesh[i].position);f.mesh[i].instance=3.25f;f.mesh[i].wind[0]=k==3?.8f:0;}
        auto now=Identity(kind),old=now;unsigned vp=k==0?0:k==3?7:68;
        old.constants[(vp+0)*4+3]=-.125f;Run(f,left,{f.Draw(kind,now,old)},total);
        old=now;if(k==0)old.constants[4*4+3]=.1f;else if(k!=3)old.constants[9*4+3]=.1f;else{now.constants[23*4]=.15f;now.constants[23*4+1]=.08f;now.constants[24*4]=.4f;now.constants[24*4+1]=.25f;old=now;old.constants[25*4]=.7f;old.constants[25*4+1]=-.5f;}Run(f,right,{f.Draw(kind,now,old)},total);
        auto invalid=f.Draw(kind,now,old);invalid.historyValid=false;Run(f,left,{invalid},total);Run(f,left,{f.Draw(kind,now,old)},total,true);
        auto outside=old;outside.constants[vp*4+3]=4;Run(f,left,{f.Draw(kind,now,outside)},total);
        if(k==1||k==2){auto missing=f.Draw(kind,now,old);missing.instanceHistory[3]=0;Run(f,left,{missing},total);for(float instance:{0.f,16.75f}){for(auto& v:f.mesh)v.instance=instance;old=now;old.constants[(unsigned(instance)*3+1)*4+3]=.1f;Run(f,left,{f.Draw(kind,now,old)},total);}
            InstanceEntity a,b;a.id.value={10,2,3,4,5};b.id.value={20,2,3,4,5};for(unsigned j=0;j<4;++j)a.world[j*4+j]=b.world[j*4+j]=1;b.world[12]=.1f;
            auto put=[](NativeGeometryPose& p,unsigned row,InstanceEntity const& e){for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)p.constants[(row*3+r)*4+c]=e.world[c*4+r];};
            now=Identity(kind);old=now;put(old,0,a);put(old,1,b);InstanceEntity oldEntities[]{a,b};auto oldIds=IdentifyInstances(old,oldEntities,2);b.world[13]=.1f;put(now,0,b);put(now,1,a);InstanceEntity newEntities[]{b,a};auto newIds=IdentifyInstances(now,newEntities,2);
            GeometryVersion version{1,2,3,4,true};GeometryFrameStamp beforeFrame{1,7,1000000000,0},afterFrame{2,7,1033333333,0};auto rebased=RebaseInstances(now,newIds,old,oldIds,version,version,afterFrame,beforeFrame);for(auto& vertex:f.mesh)vertex.instance=0;auto joined=f.Draw(kind,now,rebased.previous);joined.instanceHistory=rebased.valid;Require(joined.instanceHistory[0],"Instance reorder lost proven identity");Run(f,right,{joined},total);
        }
        if(k==3){for(auto& v:f.mesh)v.wind[0]=0;Run(f,left,{f.Draw(kind,now,old)},total);for(auto& v:f.mesh)v.wind[0]=.8f;now.constants[22*4]=.2f;now.constants[22*4+1]=-60;now.constants[22*4+2]=.3f;old=now;old.constants[22*4]=-.3f;Run(f,right,{f.Draw(kind,now,old)},total);}
        if(k!=0){ComPtr<IDirect3DVertexBuffer9> auxiliary;Hr(native->CreateVertexBuffer(6*4,D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&auxiliary,nullptr),"split position auxiliary buffer");void* mapped=nullptr;Hr(auxiliary->Lock(0,0,&mapped,0),"split auxiliary upload");for(unsigned i=0;i<6;++i)static_cast<float*>(mapped)[i]=k==3?f.mesh[i].wind[0]:f.mesh[i].instance;Hr(auxiliary->Unlock(),"split auxiliary complete");D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT3,0,D3DDECLUSAGE_POSITION,0},{1,0,D3DDECLTYPE_FLOAT1,0,D3DDECLUSAGE_TEXCOORD,BYTE(k==3?1:4)},D3DDECL_END()};ComPtr<IDirect3DVertexDeclaration9> splitDeclaration;Hr(native->CreateVertexDeclaration(elements,&splitDeclaration),"split minimal declaration");auto split=f.Draw(kind,now,old);split.declaration=splitDeclaration;split.auxiliaryStreams[0]={auxiliary,0,4,1};Run(f,left,{split},total);}
    }
    EndToEnd(f,api.Get(),left,right,argv[2]);
    CaptureObserverProof(f);
    AdditionalNativeProfiles(f,left,total,argv[2]);
    WindNativeProfiles(f,left,right,total,argv[2]);
    auto invalid=f.Draw(NativeGeometryKind::RigidWorld,Identity(NativeGeometryKind::RigidWorld),Identity(NativeGeometryKind::RigidWorld));invalid.geometry.vertices=UINT32_MAX;bool refused=false;try{left.Render(f.depth.Get(),&invalid,1);}catch(std::exception const&){refused=true;}Require(refused,"Native geometry bounds not checked");
    refused=false;try{left.Render(f.depth.Get(),&invalid,g3vr::D3D9NativeMotion::Limit+1);}catch(std::exception const&){refused=true;}Require(refused,"Native frame limit not checked");
    Hr(native->BeginScene(),"empty companion scene");left.Render(f.depth.Get(),nullptr,0);Hr(native->EndScene(),"empty companion end");for(auto p:f.Pixels(left))Require(p==std::array<float,4>{},"Empty native frame retained vectors");left.Reset();right.Reset();auto final=native->AddRef();native->Release();Require(final==baseline,"Native motion retained game device references");
    std::printf("PASS %u native rigid/instanced/wind GPU cases, %u interior pixels, max error %.6f pixels. Original captured shaders supply opaque depth; wind phase/proximity, 17 instances, both eyes, masks, occlusion, native state/lifetime. Not live full-scene coverage or NGX.\n",total.cases,total.pixels,total.error);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
