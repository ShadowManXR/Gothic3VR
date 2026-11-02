#include "StaticScene.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <functional>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace g3vr::dx11;using namespace DirectX;using Microsoft::WRL::ComPtr;
namespace {
void Need(bool b,char const* m){if(!b)throw std::runtime_error(m);}
void Check(HRESULT h,char const* m){Need(SUCCEEDED(h),m);}
template<class F>void Reject(F&& fn,char const* message){bool failed=false;try{fn();}catch(std::exception const&){failed=true;}Need(failed,message);}
struct Target {
    ComPtr<ID3D11Texture2D> texture,depth,readback;
    ComPtr<ID3D11RenderTargetView> color;ComPtr<ID3D11DepthStencilView> dsv;
    unsigned width=1280,height=720;
    void Create(ID3D11Device* device){
        D3D11_TEXTURE2D_DESC d{};d.Width=width;d.Height=height;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;d.BindFlags=D3D11_BIND_RENDER_TARGET;
        Check(device->CreateTexture2D(&d,nullptr,&texture),"Eye color");Check(device->CreateRenderTargetView(texture.Get(),nullptr,&color),"Eye RTV");
        d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Check(device->CreateTexture2D(&d,nullptr,&readback),"Eye readback");
        d.Usage=D3D11_USAGE_DEFAULT;d.CPUAccessFlags=0;d.BindFlags=D3D11_BIND_DEPTH_STENCIL;d.Format=DXGI_FORMAT_D32_FLOAT;Check(device->CreateTexture2D(&d,nullptr,&depth),"Eye depth");Check(device->CreateDepthStencilView(depth.Get(),nullptr,&dsv),"Eye DSV");
    }
    EyeTarget View()const{return {color.Get(),dsv.Get(),width,height};}
    void Clear(ID3D11DeviceContext* c){float background[]{.025f,.04f,.065f,1};c->ClearRenderTargetView(color.Get(),background);c->ClearDepthStencilView(dsv.Get(),D3D11_CLEAR_DEPTH,1,0);}
    std::vector<uint32_t> Pixels(ID3D11DeviceContext* c){
        c->CopyResource(readback.Get(),texture.Get());D3D11_MAPPED_SUBRESOURCE map{};Check(c->Map(readback.Get(),0,D3D11_MAP_READ,0,&map),"Readback map");std::vector<uint32_t> data(width*height);
        for(unsigned y=0;y<height;++y)std::memcpy(data.data()+y*width,static_cast<uint8_t*>(map.pData)+y*map.RowPitch,width*4);c->Unmap(readback.Get(),0);return data;
    }
};
void Image(std::filesystem::path const& path,std::vector<uint32_t> data,unsigned width,unsigned height){
    for(auto& pixel:data){uint32_t red=pixel&255,blue=(pixel>>16)&255;pixel=(pixel&0xff00ff00)|(red<<16)|blue;}
    BITMAPFILEHEADER h{};h.bfType=0x4d42;h.bfOffBits=sizeof(h)+sizeof(BITMAPINFOHEADER);h.bfSize=h.bfOffBits+width*height*4;
    BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=LONG(width);info.biHeight=-LONG(height);info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;
    std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<char*>(&h),sizeof(h));f.write(reinterpret_cast<char*>(&info),sizeof(info));f.write(reinterpret_cast<char*>(data.data()),data.size()*4);Need(bool(f),"Image write");
}
XMFLOAT4X4 Matrix(FXMMATRIX m){XMFLOAT4X4 out;XMStoreFloat4x4(&out,m);return out;}
std::array<Camera,2> Cameras(float width,float height,XMVECTOR at,XMVECTOR toward,float ipd=.064f){
    std::array<Camera,2> result;auto direction=XMVectorSubtract(toward,at);for(unsigned eye=0;eye<2;++eye){auto p=at+XMVectorSet(eye?ipd*.5f:-ipd*.5f,0,0,0);
        auto view=XMMatrixLookToLH(p,direction,XMVectorSet(0,1,0,0));auto projection=XMMatrixPerspectiveFovLH(XMConvertToRadians(65.f),width/height,.05f,5000.f);result[eye].viewProjection=Matrix(view*projection);}return result;
}
struct Timer {
    ComPtr<ID3D11Query> disjoint,start,end;
    void Create(ID3D11Device* d){D3D11_QUERY_DESC q{D3D11_QUERY_TIMESTAMP_DISJOINT,0};Check(d->CreateQuery(&q,&disjoint),"GPU disjoint");q.Query=D3D11_QUERY_TIMESTAMP;Check(d->CreateQuery(&q,&start),"GPU timestamp start");Check(d->CreateQuery(&q,&end),"GPU timestamp end");}
    void Begin(ID3D11DeviceContext* c){c->Begin(disjoint.Get());c->End(start.Get());}
    double End(ID3D11DeviceContext* c){c->End(end.Get());c->End(disjoint.Get());c->Flush();auto limit=GetTickCount64()+5000;D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d{};
        for(;;){auto hr=c->GetData(disjoint.Get(),&d,sizeof(d),D3D11_ASYNC_GETDATA_DONOTFLUSH);Check(hr,"GPU timing data");if(hr==S_OK)break;Need(GetTickCount64()<limit,"GPU timing timeout");SwitchToThread();}
        Need(!d.Disjoint&&d.Frequency,"GPU timestamp discontinuity");uint64_t a=0,b=0;Check(c->GetData(start.Get(),&a,8,0),"GPU start data");Check(c->GetData(end.Get(),&b,8,0),"GPU end data");Need(b>=a,"GPU timestamp order");return double(b-a)*1000/d.Frequency;
    }
};
unsigned TestNativeAlpha(ID3D11Device* device,ID3D11DeviceContext* context,
    std::filesystem::path const& materialFile,std::filesystem::path const& output){
    std::ifstream source(materialFile,std::ios::binary|std::ios::ate);Need(bool(source),"Native material fixture missing");auto size=source.tellg();Need(size>16&&size<16777216,"Fixture material size");std::vector<char> data(size_t(size),0);source.seekg(0);source.read(data.data(),size);
    size_t pos=12;auto u32=[&](){Need(pos+4<=data.size(),"Fixture material field");uint32_t x;std::memcpy(&x,data.data()+pos,4);pos+=4;return x;};
    auto string=[&](){auto n=u32();Need(n<=4096&&pos+n<=data.size(),"Fixture material string");std::string s(data.data()+pos,n);pos+=n;return s;};
    std::string texture;std::array<char,32> fields{};auto count=u32();Need(count<=4096,"Fixture material count");
    for(unsigned i=0;i<count;++i){auto name=string(),file=string();Need(pos+32<=data.size(),"Fixture material payload");if(name=="G3_Architecture_Strawroof_02_A.xshmat"){texture=file;std::memcpy(fields.data(),data.data()+pos,32);break;}pos+=32;}
    Need(!texture.empty(),"Native masked roof material missing");auto folder=output/L"native-alpha-fixture";std::filesystem::create_directories(folder/L"textures");std::filesystem::copy_file(materialFile.parent_path()/L"textures"/texture,folder/L"textures"/texture,std::filesystem::copy_options::overwrite_existing);
    auto write=[&](char const* name,uint32_t blend,bool invalid){auto path=folder/name;std::ofstream f(path,std::ios::binary);f.write("G3DX11A1",8);uint32_t version=1,number=1;f.write(reinterpret_cast<char*>(&version),4);f.write(reinterpret_cast<char*>(&number),4);
        for(auto const& s:{std::string("native-alpha-probe"),texture}){uint32_t n=uint32_t(s.size());f.write(reinterpret_cast<char*>(&n),4);f.write(s.data(),n);}auto bytes=fields;std::memcpy(bytes.data()+8,&blend,4);if(invalid)bytes[0]^=127;f.write(bytes.data(),32);Need(bool(f),"Fixture material write");return path;};
    auto opaque=write("opaque.g3mat",0,false),masked=write("masked.g3mat",1,false),bad=write("invalid-content.g3mat",1,true);
    Mesh mesh;mesh.resource="native-alpha-fixture";mesh.content=1;Section s;s.material="native-alpha-probe";
    s.vertices={{{-1,-1,0},{0,0,-1},{0,1},{1,1,1,1}},{{1,-1,0},{0,0,-1},{1,1},{1,1,1,1}},{{-1,1,0},{0,0,-1},{0,0},{1,1,1,1}},{{1,1,0},{0,0,-1},{1,0},{1,1,1,1}}};s.indices={0,1,2,2,1,3};mesh.sections.push_back(std::move(s));
    StaticScene renderer;renderer.Initialize(device,context);renderer.Upload(mesh);renderer.LoadMaterialPreview(opaque);Target target;target.Create(device);auto cameras=Cameras(1280,720,XMVectorSet(0,0,-3,1),XMVectorSet(0,0,0,1));Instance instance{1,mesh.resource,mesh.content,Matrix(XMMatrixIdentity())};
    renderer.Prepare({instance},cameras);target.Clear(context);renderer.Render(0,target.View());auto all=target.Pixels(context);auto uploads=renderer.Stats().textureUploads;
    renderer.LoadMaterialPreview(masked);Reject([&]{renderer.Render(0,target.View());},"Changed material library did not invalidate prepared stereo scene");renderer.Prepare({instance},cameras);target.Clear(context);renderer.Render(0,target.View());auto cut=target.Pixels(context);
    unsigned holes=0;for(size_t i=0;i<all.size();++i)holes+=all[i]!=all[0]&&cut[i]==cut[0];Need(holes>1000,"Native roof alpha did not cut visible holes");Need(renderer.Stats().textureUploads==uploads,"Material-only change reuploaded the same native texture");
    Reject([&]{renderer.LoadMaterialPreview(bad);},"Invalid native texture content accepted");target.Clear(context);renderer.Render(0,target.View());Need(target.Pixels(context)==cut,"Rejected material update damaged retained rendering");
    Image(output/L"native-roof-alpha-opaque.bmp",all,1280,720);Image(output/L"native-roof-alpha-masked.bmp",cut,1280,720);return holes;
}
}
int wmain(int argc,wchar_t** argv){try{
    Need(argc==3,"Usage: G3DX11StaticProbe <imported asset directory> <output directory>");std::filesystem::path input=argv[1],output=argv[2];std::filesystem::create_directories(output);
    std::vector<Mesh> meshes;for(auto const& file:std::filesystem::directory_iterator(input))if(file.path().extension()==L".g3mesh")meshes.push_back(ReadMesh(file.path()));Need(meshes.size()>=2,"Need actual Gothic 3 imported meshes");std::sort(meshes.begin(),meshes.end(),[](auto const& a,auto const& b){return a.resource<b.resource;});
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL feature;UINT flags=0;
    bool debug=true;auto hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_DEBUG,nullptr,0,D3D11_SDK_VERSION,&device,&feature,&context);
    if(FAILED(hr)){debug=false;Check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,flags,nullptr,0,D3D11_SDK_VERSION,&device,&feature,&context),"D3D11 hardware device");}Need(feature>=D3D_FEATURE_LEVEL_11_0,"DX11 hardware required");
    ComPtr<IDXGIDevice> dxgi;Check(device.As(&dxgi),"DXGI device");ComPtr<IDXGIAdapter> adapter;Check(dxgi->GetAdapter(&adapter),"DXGI adapter");DXGI_ADAPTER_DESC description{};adapter->GetDesc(&description);
    StaticScene scene;scene.Initialize(device.Get(),context.Get());uint64_t expectedBytes=0,totalVertices=0,totalTriangles=0;
    for(auto const& mesh:meshes){scene.Upload(mesh);for(auto const& s:mesh.sections){expectedBytes+=s.vertices.size()*sizeof(Vertex)+s.indices.size()*4;totalVertices+=s.vertices.size();totalTriangles+=s.indices.size()/3;}}
    auto stats=scene.Stats();Need(stats.geometryUploads==meshes.size()&&stats.geometryBytes==expectedBytes,"Immutable geometry upload accounting");
    for(auto const& mesh:meshes)scene.Upload(mesh);Need(scene.Stats().geometryUploads==stats.geometryUploads,"Same content uploaded geometry twice");
    auto materialFile=input/L"materials"/L"preview.g3mat";bool nativeDiffuse=std::filesystem::exists(materialFile);
    if(nativeDiffuse){scene.LoadMaterialPreview(materialFile);auto s=scene.Stats();Need(s.materials>=14&&s.textureUploads>=14&&s.textureBytes>1000000,"Native diffuse materials did not load");scene.LoadMaterialPreview(materialFile);Need(scene.Stats().textureUploads==s.textureUploads,"Unchanged materials reuploaded textures");}
    Target targets[2];for(auto& t:targets)t.Create(device.Get());
    auto cameras=Cameras(1280,720,XMVectorSet(0,7,-28,1),XMVectorSet(0,4,0,1));
    Instance heroMesh{1,meshes[0].resource,meshes[0].content,Matrix(XMMatrixIdentity())};std::vector<Instance> instances{heroMesh};
    Instance distant=heroMesh;distant.id=2;distant.world=Matrix(XMMatrixTranslation(100000,0,0));instances.push_back(distant);scene.Prepare(instances,cameras);
    Need(scene.Stats().visible==1&&scene.Stats().culled==1,"Union-frustum culling rejected/accepted wrong meshes");auto uploads=scene.Stats().instanceUploads;
    for(unsigned e=0;e<2;++e){targets[e].Clear(context.Get());scene.Render(e,targets[e].View());}
    auto left=targets[0].Pixels(context.Get()),right=targets[1].Pixels(context.Get());size_t differences=0,foreground=0;double centroid[2]{},counts[2]{};
    for(size_t i=0;i<left.size();++i){differences+=left[i]!=right[i];if(left[i]!=left[0])++foreground;
        if(left[i]!=left[0]){centroid[0]+=double(i%1280);++counts[0];}if(right[i]!=right[0]){centroid[1]+=double(i%1280);++counts[1];}}
    Need(foreground>20000&&differences>2000,"Real Gothic geometry or stereo parallax missing");Need(counts[0]&&counts[1]&&std::abs(centroid[0]/counts[0]-centroid[1]/counts[1])>.2,"Stereo camera did not move geometry");
    Image(output/L"ardea-house-left.bmp",left,1280,720);Image(output/L"ardea-house-right.bmp",right,1280,720);
    Need(scene.Stats().instanceUploads==uploads&&scene.Stats().geometryUploads==meshes.size(),"Eye render performed geometry or instance uploads");
    if(nativeDiffuse){auto s=scene.Stats();Need(s.texturedDraws[0]==s.draws[0]&&s.texturedDraws[1]==s.draws[1],"A known Ardea material used diagnostic fallback");}
    scene.Prepare(instances,cameras);Need(scene.Stats().instanceUploads==uploads,"Unchanged preparation uploads instances");
    auto invalid=heroMesh;invalid.content^=1;Reject([&]{scene.Prepare({invalid},cameras);},"Stale resource content accepted");Reject([&]{scene.Render(0,targets[0].View());},"Failed preparation retained drawable stale scene");
    Reject([&]{scene.Prepare({heroMesh,heroMesh},cameras);},"Duplicate entity accepted");scene.Prepare({heroMesh},cameras);
    auto corrupt=meshes[0];corrupt.content^=1;corrupt.sections[0].indices[0]=UINT32_MAX;Reject([&]{scene.Upload(corrupt);},"Out-of-bounds mesh accepted");Need(scene.Stats().geometryUploads==meshes.size(),"Rejected replacement changed cache");
    targets[0].Clear(context.Get());scene.Render(0,targets[0].View());Need(targets[0].Pixels(context.Get())==left,"Rejected replacement damaged retained mesh");
    // A mesh visible only to one eye must remain in the shared preparation.
    auto unionCameras=cameras;unionCameras[1]=Cameras(1280,720,XMVectorSet(100,7,-28,1),XMVectorSet(100,4,0,1))[1];
    auto side=heroMesh;side.id=3;side.world=Matrix(XMMatrixTranslation(100,0,0));scene.Prepare({heroMesh,side},unionCameras);Need(scene.Stats().visible==2,"Single-eye mesh was culled from stereo union");
    // Version/epoch/material/depth guard: diagnostic output never replaces game draws.
    ReplacementProof proof;proof.frame=5;proof.epoch=1;proof.geometryContent=meshes[0].content;proof.liveEntity=true;
    for(unsigned e=0;e<2;++e){proof.eyeFrame[e]=5;proof.eyeEpoch[e]=1;proof.eyeContent[e]=proof.geometryContent;}
    Need(!proof.CanOmitNative(),"Diagnostic materials may not suppress native draws");proof.materialsExact=proof.depthCompatible=true;Need(proof.CanOmitNative(),"Complete replacement proof rejected");proof.eyeEpoch[1]=2;Need(!proof.CanOmitNative(),"Mismatched stereo epoch accepted");
    if(nativeDiffuse){
        std::vector<Instance> gallery;for(unsigned i=0;i<meshes.size();++i){auto const& mesh=meshes[i];gallery.push_back({i+500,mesh.resource,mesh.content,Matrix(XMMatrixTranslation(float(int(i%4)-1)*80,0,float(i/4)*80))});}
        auto galleryCameras=Cameras(1280,720,XMVectorSet(35,190,-250,1),XMVectorSet(35,0,80,1));scene.Prepare(gallery,galleryCameras);Need(scene.Stats().visible==meshes.size(),"Gallery unexpectedly culled a native mesh");
        for(unsigned e=0;e<2;++e){targets[e].Clear(context.Get());scene.Render(e,targets[e].View());}auto all=scene.Stats();Need(all.texturedDraws[0]==all.draws[0]&&all.texturedDraws[1]==all.draws[1],"Gallery contains a native material without a loaded diffuse preview");Image(output/L"ardea-mesh-gallery.bmp",targets[0].Pixels(context.Get()),1280,720);
    }
    // Demonstrate a shared stereo batch of repeated REAL meshes. This is an
    // isolated submission comparison, not a prediction of Gothic's Ardea FPS.
    std::vector<Instance> repeated;for(unsigned i=0;i<120;++i){auto x=heroMesh;x.id=i+100;x.world=Matrix(XMMatrixTranslation(float(i%12)*22,0,float(i/12)*25));repeated.push_back(x);}
    cameras=Cameras(1280,720,XMVectorSet(115,140,-170,1),XMVectorSet(115,0,100,1));scene.Prepare(repeated,cameras);auto before=scene.Stats();
    std::array<std::vector<uint32_t>,2> reference;unsigned unbatched=0,batched=0;
    for(bool batch:{false,true}){for(unsigned e=0;e<2;++e){targets[e].Clear(context.Get());scene.Render(e,targets[e].View(),batch);auto pixels=targets[e].Pixels(context.Get());if(batch)Need(pixels==reference[e],"Batched renderer changed output pixels");else reference[e]=std::move(pixels);}auto s=scene.Stats();if(batch)batched=s.draws[0]+s.draws[1];else unbatched=s.draws[0]+s.draws[1];}
    Need(batched&&unbatched>=batched*10,"Real-mesh instancing failed to reduce draw submissions");Image(output/L"ardea-instancing.bmp",reference[0],1280,720);
    Timer timer;timer.Create(device.Get());double cpu[2]{},gpu[2]{};
    for(unsigned mode=0;mode<2;++mode){std::vector<double> cs,gs;for(unsigned sample=0;sample<12;++sample){for(auto& t:targets)t.Clear(context.Get());timer.Begin(context.Get());auto a=std::chrono::steady_clock::now();for(unsigned e=0;e<2;++e)scene.Render(e,targets[e].View(),mode!=0);auto b=std::chrono::steady_clock::now();double elapsed=timer.End(context.Get());if(sample>=2){cs.push_back(std::chrono::duration<double,std::milli>(b-a).count());gs.push_back(elapsed);}}std::sort(cs.begin(),cs.end());std::sort(gs.begin(),gs.end());cpu[mode]=cs[cs.size()/2];gpu[mode]=gs[gs.size()/2];}
    stats=scene.Stats();Need(stats.geometryUploads==before.geometryUploads&&stats.instanceUploads==before.instanceUploads&&stats.textureUploads==before.textureUploads,"Rendering repeatedly changed immutable resources");
    unsigned alphaHoles=nativeDiffuse?TestNativeAlpha(device.Get(),context.Get(),materialFile,output):0;
    scene.Evict(meshes[0].resource);Reject([&]{scene.Render(0,targets[0].View());},"Eviction retained stale prepared draw pointers");scene.Upload(meshes[0]);scene.Prepare({heroMesh},cameras);
    std::ofstream report(output/L"results.json");
    report<<"{\n  \"result\":\"PASS\",\n  \"prototype_only\":true,\n  \"game_draws_replaced\":false,\n";
    report<<"  \"native_diffuse_preview\":"<<(nativeDiffuse?"true":"false")<<",\n  \"native_lighting_and_depth_parity\":false,\n";
    report<<"  \"materials\":"<<stats.materials<<",\n  \"texture_uploads\":"<<stats.textureUploads<<",\n  \"texture_bytes\":"<<stats.textureBytes<<",\n";
    report<<"  \"native_roof_alpha_holes\":"<<alphaHoles<<",\n  \"hardware_vendor\":"<<description.VendorId<<",\n  \"debug_device\":"<<(debug?"true":"false")<<",\n";
    report<<"  \"real_meshes\":"<<meshes.size()<<",\n  \"vertices\":"<<totalVertices<<",\n  \"triangles\":"<<totalTriangles<<",\n  \"geometry_bytes\":"<<expectedBytes<<",\n";
    report<<"  \"stereo_different_pixels\":"<<differences<<",\n  \"headset_test\":false,\n  \"benchmark_instances\":120,\n  \"benchmark_visible_instances\":"<<before.visible<<",\n";
    report<<"  \"unbatched_stereo_draws\":"<<unbatched<<",\n  \"batched_stereo_draws\":"<<batched<<",\n  \"unbatched_cpu_submit_ms\":"<<cpu[0]<<",\n  \"batched_cpu_submit_ms\":"<<cpu[1]<<",\n";
    report<<"  \"unbatched_gpu_ms\":"<<gpu[0]<<",\n  \"batched_gpu_ms\":"<<gpu[1]<<",\n  \"unchanged_frame_geometry_uploads\":0,\n  \"unchanged_frame_instance_uploads\":0,\n  \"unchanged_frame_texture_uploads\":0\n}\n";
    Need(bool(report),"Report write");
    context->ClearState();context->Flush();std::cout<<"PASS real Ardea meshes, geometric stereo, immutable cache, stereo-union culling, batched/unbatched pixel equality, invalidation, replacement guards\n";std::cout<<"Stereo submissions "<<unbatched<<" -> "<<batched<<"; CPU ms "<<cpu[0]<<" -> "<<cpu[1]<<"; GPU ms "<<gpu[0]<<" -> "<<gpu[1]<<" (isolated prototype only)\n";return 0;
}catch(std::exception const& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
