// Included after the native GPU Fixture. Original saved VS and PS are the
// coverage oracle; no CPU model of texture filtering authorizes the mask.
ComPtr<IDirect3DTexture9> AlphaFixtureTexture(IDirect3DDevice9* d,D3DFORMAT format){
    ComPtr<IDirect3DTexture9> staging,texture;Hr(d->CreateTexture(64,64,0,0,format,D3DPOOL_SYSTEMMEM,&staging,nullptr),"alpha staging texture");Hr(d->CreateTexture(64,64,0,0,format,D3DPOOL_DEFAULT,&texture,nullptr),"alpha static texture");
    for(unsigned level=0;level<staging->GetLevelCount();++level){D3DSURFACE_DESC desc{};staging->GetLevelDesc(level,&desc);D3DLOCKED_RECT lock{};Hr(staging->LockRect(level,&lock,nullptr,0),"alpha texture fixture upload");
        if(format==D3DFMT_A8R8G8B8){for(unsigned y=0;y<desc.Height;++y)for(unsigned x=0;x<desc.Width;++x){unsigned values[]={0,119,120,121,255};unsigned alpha=values[(x/3+y/4+level)%5];auto* row=reinterpret_cast<uint32_t*>(static_cast<char*>(lock.pBits)+y*lock.Pitch);row[x]=(alpha<<24)|0xffffff;}}
        else{for(unsigned by=0;by<std::max(1u,(desc.Height+3)/4);++by)for(unsigned bx=0;bx<std::max(1u,(desc.Width+3)/4);++bx){auto* block=reinterpret_cast<unsigned char*>(static_cast<char*>(lock.pBits)+by*lock.Pitch)+bx*16;
                block[0]=255;block[1]=0;uint64_t indices=0;for(unsigned i=0;i<16;++i)indices|=uint64_t((i+bx+by+level)%8)<<(i*3);for(unsigned i=0;i<6;++i)block[i+2]=unsigned char(indices>>(8*i));
                block[8]=block[9]=255;for(unsigned i=10;i<16;++i)block[i]=0;}}
        Hr(staging->UnlockRect(level),"alpha fixture upload complete");}
    Hr(d->UpdateTexture(staging.Get(),texture.Get()),"alpha fixture GPU texture upload");return texture;
}
void AlphaNativeProfiles(Fixture& f,std::filesystem::path const& directory){
    auto load=[&](char const* name){std::ifstream file(directory/name,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});Require(file&&bytes.size()>16&&bytes.size()%4==0,"Original alpha shader missing");return bytes;};
    auto vsBytes=load("temporal-vs-15e5561db503fe47.cso"),psBytes=load("temporal-ps-cef3519ea7bd097.cso");ComPtr<IDirect3DVertexShader9> vs;ComPtr<IDirect3DPixelShader9> ps;
    Hr(f.d->CreateVertexShader(reinterpret_cast<DWORD const*>(vsBytes.data()),&vs),"actual alpha wind VS");Hr(f.d->CreatePixelShader(reinterpret_cast<DWORD const*>(psBytes.data()),&ps),"actual alpha PS");
    auto* profile=FindNativeShaderProfile(0x15e5561db503fe47ull);Require(profile!=nullptr,"Exact alpha wind position profile missing");unsigned cases=0,totalPixels=0;auto shadowTexture=AlphaFixtureTexture(f.d.Get(),D3DFMT_A8R8G8B8);
    for(auto format:{D3DFMT_A8R8G8B8,D3DFMT_DXT5}){auto texture=AlphaFixtureTexture(f.d.Get(),format);
        for(unsigned mode=0;mode<4;++mode)for(DWORD ref:{0u,119u,120u,121u,255u}){if(mode&&ref!=120)continue;
            for(unsigned eye=0;eye<2;++eye){auto capture=std::make_unique<g3vr::D3D9NativeMotionCapture>();
                float triangle[3][3]{{-.75f,-.65f,.5f},{.75f,-.65f,.5f},{0,.75f,.5f}};float uv[3][2]{{-.2f,.15f},{1.75f,.05f},{.2f,1.9f}};
                for(unsigned i=0;i<3;++i){std::copy_n(triangle[i],3,f.mesh[i].position);f.mesh[i].wind[0]=mode==2?.8f:0;for(unsigned k=0;k<2;++k)f.mesh[i].uv[k]=uv[i][k]*(mode?3.3f:1.f);}
                f.Upload();unsigned expectedAny=0;
                for(unsigned frame:{1u,2u}){std::array<float,1024> raw{};for(unsigned i=0;i<4;++i)raw[i*5]=1;raw[3]=eye?.02f:-.02f;
                    raw[19*4+1]=1000;raw[20*4]=.035f;raw[20*4+1]=.02f;raw[21*4]=.4f;raw[21*4+1]=.8f;raw[22*4]=frame*.1f;raw[22*4+1]=-.3f;
                    raw[15*4+2]=-1;raw[17*4]=mode==3?.1f:mode==1?2.3f:10.f;raw[18*4]=1;
                    NativeGeometryPose pose;Require(NormalizeNativePose(*profile,raw.data(),256,pose),"Alpha native normalization");auto draw=f.Draw(NativeGeometryKind::Wind,pose,pose);
                    capture->Begin(eye,frame,1000000000ull+frame*11111111ull,f.w,f.h);
                    std::array<DWORD,std::size(NativeMotionDraw::AlphaSamplerStates)> samplers{};
                    for(unsigned i=0;i<samplers.size();++i){DWORD value=0;switch(NativeMotionDraw::AlphaSamplerStates[i]){
                        case D3DSAMP_ADDRESSU:case D3DSAMP_ADDRESSV:value=mode==2?D3DTADDRESS_CLAMP:mode==3?D3DTADDRESS_BORDER:D3DTADDRESS_WRAP;break;
                        case D3DSAMP_ADDRESSW:value=D3DTADDRESS_WRAP;break;case D3DSAMP_BORDERCOLOR:value=0x78ffffff;break;
                        case D3DSAMP_MAGFILTER:case D3DSAMP_MINFILTER:value=mode==0?D3DTEXF_POINT:mode==2?D3DTEXF_ANISOTROPIC:D3DTEXF_LINEAR;break;
                        case D3DSAMP_MIPFILTER:value=mode?D3DTEXF_LINEAR:D3DTEXF_NONE;break;case D3DSAMP_MAXANISOTROPY:value=16;break;
                        case D3DSAMP_MIPMAPLODBIAS:{float bias=mode==2?.35f:0;memcpy(&value,&bias,4);break;}
                        case D3DSAMP_MAXMIPLEVEL:value=mode==1?2:0;break;default:value=0;break;}samplers[i]=value;}
                    f.Native({draw},false,[&](NativeMotionDraw const& r){
                        f.d->SetPixelShader(ps.Get());for(unsigned slot=0;slot<4;++slot)f.d->SetTexture(slot,slot==3?texture.Get():shadowTexture.Get());
                        f.d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_MIRROR);f.d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
                        for(unsigned i=0;i<samplers.size();++i)f.d->SetSamplerState(3,NativeMotionDraw::AlphaSamplerStates[i],samplers[i]);
                        f.d->SetRenderState(D3DRS_WRAP0,0);f.d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);f.d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);f.d->SetRenderState(D3DRS_ALPHAREF,ref);
                        capture->Observe(f.d.Get(),r.geometry);
                    },vs.Get(),raw.data());
                    if(capture->stats[eye].accepted!=1){auto const& s=capture->stats[eye];std::printf("Alpha admission format%u mode%u ref%u eye%u frame%u accepted%u rejected%u unsupported%u unidentified%u refusal:",format,mode,ref,eye,frame,s.accepted,s.rejected,s.unsupported,s.unidentified);for(auto n:s.refusal)std::printf(" %u",n);std::puts("");}
                    Require(capture->stats[eye].accepted==1,"Exact alpha material observer was rejected");auto color=f.Color();
                    std::array<DWORD,std::size(NativeMotionDraw::AlphaSamplerStates)> originalSampler0{};for(unsigned i=0;i<originalSampler0.size();++i)f.d->GetSamplerState(0,NativeMotionDraw::AlphaSamplerStates[i],&originalSampler0[i]);
                    // Deliberately put an equal-depth background behind holes.
                    // Z/EQUAL alone would incorrectly fill the whole triangle.
                    if(mode==0)Hr(f.d->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,.5f,0),"coplanar alpha hole fixture");
                    Hr(f.d->BeginScene(),"native alpha companion scene");capture->Capture(f.d.Get(),f.depth.Get());Hr(f.d->EndScene(),"native alpha companion end");
                    Require(capture->stats[eye].history==frame-1,"Exact alpha material history was rejected");
                    Require(capture->Renderer(eye)!=nullptr,"Alpha companion raster absent");auto pixels=f.Pixels(*capture->Renderer(eye));
                    unsigned expectedCount=0,gotCount=0;
                    for(unsigned i=0;i<pixels.size();++i){bool expected=(color[i]>>24)!=0,got=pixels[i][2]==1;expectedCount+=expected;gotCount+=got;
                        if(expected!=got){std::printf("Alpha mismatch format%u mode%u ref%u eye%u frame%u xy%u,%u original=%08x motion=%f,%f,%f,%f\n",format,mode,ref,eye,frame,i%f.w,i/f.w,color[i],pixels[i][0],pixels[i][1],pixels[i][2],pixels[i][3]);throw std::runtime_error("Native alpha mask differs from original PS/fixed alpha test");}
                        if(got)Require(pixels[i][3]==float(frame-1),"Alpha output/history flag confused with material opacity");else Require(pixels[i]==std::array<float,4>{},"Transparent hole retained motion");}
                    Require(f.Color()==color,"Alpha replay changed native color");DWORD alpha=0,reference=0;f.d->GetRenderState(D3DRS_ALPHATESTENABLE,&alpha);f.d->GetRenderState(D3DRS_ALPHAREF,&reference);Require(alpha==TRUE&&reference==ref,"Alpha replay changed caller tests");
                    ComPtr<IDirect3DBaseTexture9> bound;f.d->GetTexture(3,&bound);Require(bound.Get()==texture.Get(),"Alpha replay changed caller texture");
                    f.d->GetTexture(0,&bound);Require(bound.Get()==shadowTexture.Get(),"Alpha replay retained its private sampler binding");
                    for(unsigned i=0;i<samplers.size();++i){DWORD value=0;f.d->GetSamplerState(3,NativeMotionDraw::AlphaSamplerStates[i],&value);Require(value==samplers[i],"Alpha replay changed source filtering");f.d->GetSamplerState(0,NativeMotionDraw::AlphaSamplerStates[i],&value);Require(value==originalSampler0[i],"Alpha replay did not restore overwritten filtering");}
                    expectedAny+=expectedCount;totalPixels+=unsigned(pixels.size());++cases;capture->Stop();
                }
                if(ref==255||mode==3)Require(expectedAny==0,"Original alpha boundary did not reject all pixels");else if(mode==0)Require(expectedAny>20,"Insufficient alpha fixture coverage");
            }
        }
        for(unsigned slot=0;slot<4;++slot)f.d->SetTexture(slot,nullptr);
    }
    // The exact pixel shader, compare mode, static texture and declaration
    // remain mandatory. Position equivalence alone cannot authorize alpha.
    auto texture=AlphaFixtureTexture(f.d.Get(),D3DFMT_A8R8G8B8);ComPtr<IDirect3DTexture9> renderTarget;
    Hr(f.d->CreateTexture(64,64,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&renderTarget,nullptr),"unsupported dynamic alpha source");
    for(unsigned bad=0;bad<5;++bad){auto capture=std::make_unique<g3vr::D3D9NativeMotionCapture>();std::array<float,1024> raw{};for(unsigned i=0;i<4;++i)raw[i*5]=1;raw[19*4+1]=1000;raw[17*4]=10;raw[18*4]=1;NativeGeometryPose pose;NormalizeNativePose(*profile,raw.data(),256,pose);
        capture->Begin(0,1,1000000000ull,f.w,f.h);f.Native({f.Draw(NativeGeometryKind::Wind,pose,pose)},false,[&](NativeMotionDraw const& r){
            f.d->SetPixelShader(bad==0?f.nativePs.Get():ps.Get());f.d->SetTexture(3,bad==3?renderTarget.Get():texture.Get());f.d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);f.d->SetRenderState(D3DRS_ALPHAFUNC,bad==1?D3DCMP_GREATEREQUAL:D3DCMP_GREATER);f.d->SetRenderState(D3DRS_ALPHAREF,120);f.d->SetRenderState(D3DRS_WRAP0,bad==2?1:0);
            if(bad==4)f.d->SetVertexDeclaration(f.minimal[3].Get());capture->Observe(f.d.Get(),r.geometry);f.d->SetVertexDeclaration(f.nativeDecl.Get());f.d->SetTexture(3,texture.Get());f.d->SetRenderState(D3DRS_WRAP0,0);
        },vs.Get(),raw.data());Require(capture->stats[0].accepted==0&&capture->stats[0].rejected==1,"Unsupported alpha state/texture/shader/UV was granted motion");capture->Stop();}
    for(unsigned slot=0;slot<4;++slot)f.d->SetTexture(slot,nullptr);
    f.d->SetPixelShader(f.nativePs.Get());f.d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
    std::printf("PASS %u alpha GPU cases / %u pixels and 5 negative material cases: original captured VS+PS, DXT5/A8, threshold edges, interpolation/fade, trilinear/aniso/mips/addressing, equal-depth holes, both eyes/history and caller state\n",cases,totalPixels);
}
