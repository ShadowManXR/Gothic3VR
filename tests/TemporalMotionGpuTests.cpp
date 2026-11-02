#include "GpuTemporalMotion.h"
#include "TemporalMotionCS.h"
#include <DirectXPackedVector.h>
#include <wrl/client.h>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cmath>
#include <limits>
#include <fstream>
#include <filesystem>
#include <cstring>
using namespace diagnostic::temporal;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;
void Require(bool b,char const* s){if(!b)throw std::runtime_error(s);}
void Hr(HRESULT hr){Require(SUCCEEDED(hr),"D3D11 fixture operation failed");}
Matrix Identity(){Matrix m{};m[0]=m[5]=m[10]=m[15]=1;return m;}
template<class F>void Reject(F f){bool rejected=false;try{f();}catch(std::exception const&){rejected=true;}Require(rejected,"Invalid input was accepted");}
struct Fixture {
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    unsigned width,height;GpuTemporalMotion composer;
    std::vector<float> depth;std::vector<uint16_t> skin;std::vector<unsigned char> known;
    ComPtr<ID3D11Texture2D> images[3],readback;
    ComPtr<ID3D11ShaderResourceView> views[3];
    Fixture(ID3D11Device* d,ID3D11DeviceContext* c,unsigned w,unsigned h):device(d),context(c),width(w),height(h),depth(size_t(w)*h,.5f),skin(size_t(w)*h*4),known(size_t(w)*h,255){
        composer.Initialize(d,w,h);
        for(unsigned i=0;i<3;++i){D3D11_TEXTURE2D_DESC t{};t.Width=w;t.Height=h;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            t.Format=i==0?DXGI_FORMAT_R32_FLOAT:i==1?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R8_UNORM;
            Hr(d->CreateTexture2D(&t,nullptr,&images[i]));Hr(d->CreateShaderResourceView(images[i].Get(),nullptr,&views[i]));}
        D3D11_TEXTURE2D_DESC t{};composer.Texture()->GetDesc(&t);t.BindFlags=0;t.Usage=D3D11_USAGE_STAGING;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Hr(d->CreateTexture2D(&t,nullptr,&readback));
    }
    void SetSkin(unsigned x,unsigned y,float a,float b,bool valid){auto i=(size_t(y)*width+x)*4;skin[i]=XMConvertFloatToHalf(a);skin[i+1]=XMConvertFloatToHalf(b);skin[i+2]=XMConvertFloatToHalf(1);skin[i+3]=XMConvertFloatToHalf(valid?1.f:0.f);}
    std::array<double,4> Expected(unsigned x,unsigned y,CameraMotionFrame const& f,std::vector<uint16_t> const* native=nullptr){
        size_t i=size_t(y)*width+x;double s[4]{};for(unsigned k=0;k<4;++k)s[k]=XMConvertHalfToFloat(skin[i*4+k]);
        double n[4]{};if(native)for(unsigned k=0;k<4;++k)n[k]=XMConvertHalfToFloat((*native)[i*4+k]);
        auto malformed=[](double const* p){for(unsigned k=0;k<4;++k)if(!std::isfinite(p[k]))return true;return p[2]!=std::floor(p[2])||p[3]!=std::floor(p[3])||p[2]<0||p[3]<0||p[2]>1||p[3]>1||p[3]>p[2];};
        if(malformed(s)||malformed(n))return {};
        double jitterX=f.geometryIncludesRasterJitter?f.previousRasterJitter[0]-f.currentRasterJitter[0]:0,jitterY=f.geometryIncludesRasterJitter?f.previousRasterJitter[1]-f.currentRasterJitter[1]:0;
        if(s[2]==1&&n[2]==1){bool valid=s[3]==1&&n[3]==1&&f.historyValid&&std::hypot(s[0],s[1])<=std::hypot(width,height)+2&&std::hypot(n[0],n[1])<=std::hypot(width,height)+2&&std::hypot(s[0]-n[0],s[1]-n[1])<=.25;return {valid?s[0]-jitterX:0,valid?s[1]-jitterY:0,1,valid?1.:0.};}
        if(s[2]==0)std::copy_n(n,4,s);
        if(s[2]>.5){bool valid=s[2]==1&&s[3]==1&&f.historyValid&&std::hypot(s[0],s[1])<=std::hypot(width,height)+2;return {valid?s[0]-jitterX:0,valid?s[1]-jitterY:0,1,valid?1.:0.};}
        if(known[i]!=255||!std::isfinite(depth[i])||depth[i]<0||depth[i]>=1)return {};
        double current[]{(x+.5)/width*2-1,1-(y+.5)/height*2,depth[i],1},previous[4]{};
        for(unsigned k=0;k<4;++k)for(unsigned j=0;j<4;++j)previous[k]+=current[j]*f.currentClipToPreviousClip[j*4+k];
        bool valid=f.historyValid&&previous[3]>1.e-5&&previous[2]>=0&&previous[2]<=previous[3]&&std::abs(previous[0])<=previous[3]&&std::abs(previous[1])<=previous[3];
        double a=valid?(previous[0]/previous[3]-current[0])*width*.5-(f.previousRasterJitter[0]-f.currentRasterJitter[0]):0;
        double b=valid?(previous[1]/previous[3]-current[1])*height*-.5-(f.previousRasterJitter[1]-f.currentRasterJitter[1]):0;
        return {a,b,1,valid?1.:0.};
    }
    double Run(CameraMotionFrame const& f,std::vector<uint16_t> const* native=nullptr){
        context->UpdateSubresource(images[0].Get(),0,nullptr,depth.data(),width*4,0);
        context->UpdateSubresource(images[1].Get(),0,nullptr,skin.data(),width*8,0);
        context->UpdateSubresource(images[2].Get(),0,nullptr,known.data(),width,0);
        ComPtr<ID3D11Texture2D> nativeImage;ComPtr<ID3D11ShaderResourceView> nativeView;
        if(native){Require(native->size()==size_t(width)*height*4,"Native fixture dimensions");D3D11_TEXTURE2D_DESC t{};t.Width=width;t.Height=height;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA data{native->data(),width*8,0};Hr(device->CreateTexture2D(&t,&data,&nativeImage));Hr(device->CreateShaderResourceView(nativeImage.Get(),nullptr,&nativeView));composer.ProcessLayers(context.Get(),views[0].Get(),views[1].Get(),nativeView.Get(),views[2].Get(),f);}
        else composer.Process(context.Get(),views[0].Get(),views[1].Get(),views[2].Get(),f);
        context->CopyResource(readback.Get(),composer.Texture());D3D11_MAPPED_SUBRESOURCE map{};Hr(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&map));
        double maximum=0;size_t validCount=0,skinCount=0;
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){auto* p=reinterpret_cast<uint16_t*>(static_cast<char*>(map.pData)+y*map.RowPitch)+x*4;auto expected=Expected(x,y,f,native);
            float b=XMConvertHalfToFloat(p[2]),a=XMConvertHalfToFloat(p[3]);
            Require(b==expected[2]&&a==expected[3],"Coverage/history mask differs from double reference");validCount+=a==1;skinCount+=skin[(size_t(y)*width+x)*4+2]!=0;
            for(unsigned k=0;k<2;++k){float got=XMConvertHalfToFloat(p[k]);Require(std::isfinite(got),"Nonfinite GPU motion");double error=std::abs(got-expected[k]);maximum=std::max(maximum,error);
                Require(error<=.02+std::abs(expected[k])*.001,"GPU camera/native composition differs from double reference");if(a==0)Require(got==0,"Invalid history contains motion");}
        }
        context->Unmap(readback.Get(),0);std::printf("PASS composition %ux%u valid=%zu skin=%zu max_error=%.8f pixels\n",width,height,validCount,skinCount,maximum);return maximum;
    }
};
template<class T>void Read(std::ifstream& f,T* values,size_t n){f.read(reinterpret_cast<char*>(values),std::streamsize(n*sizeof(T)));Require(bool(f),"Truncated captured camera fixture");}
void Captured(ID3D11Device* d,ID3D11DeviceContext* c,char const* path){
    // Data are actual captured matrices, depth and skinned geometry vectors.
    // The test's all-static mask is SYNTHETIC and grants no game classification.
    std::ifstream f(std::filesystem::u8path(path),std::ios::binary);uint32_t header[3]{};Read(f,header,3);Require(header[0]==0x314d4347&&header[1]==2&&header[2]==0,"Camera fixture magic/count");
    for(unsigned eye=0;eye<2;++eye){uint32_t info[4]{};Read(f,info,4);Require(info[0]==eye&&info[1]<=8192&&info[2]<=8192&&uint64_t(info[1])*info[2]<=33554432&&info[3]==405,"Captured eye/dimensions/frame");
        Matrix now{},before{};Read(f,now.data(),16);Read(f,before.data(),16);Fixture test(d,c,info[1],info[2]);Read(f,test.depth.data(),test.depth.size());Read(f,test.skin.data(),test.skin.size());
        CameraMotionFrame frame;frame.currentClipToPreviousClip=CameraReprojection(now,before);frame.historyValid=true;test.Run(frame);
        // With unknown static coverage, ONLY native skin remains valid.
        std::fill(test.known.begin(),test.known.end(),0);test.Run(frame);
    }
    char extra=0;f.read(&extra,1);Require(f.eof(),"Trailing captured camera fixture data");
}
int main(int argc,char** argv){try{
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    Hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context));
    Matrix identity=Identity();auto same=CameraReprojection(identity,identity);for(unsigned i=0;i<16;++i)Require(same[i]==float(identity[i]),"Identity reprojection");
    Matrix large=identity;large[12]=87654.3125;large[13]=-73222.75;large[14]=21048.5;
    auto moved=large;moved[12]+=.01;auto accurate=CameraReprojection(large,moved);Require(std::abs(accurate[12]-.01)<1.e-8,"Large-origin cancellation");
    Matrix singular{};Reject([&]{CameraReprojection(singular,identity);});auto nan=identity;nan[4]=std::numeric_limits<double>::quiet_NaN();Reject([&]{CameraReprojection(identity,nan);});
    GpuTemporalMotion empty;CameraMotionFrame frame;frame.currentClipToPreviousClip=same;frame.historyValid=true;
    Reject([&]{empty.Process(context.Get(),nullptr,nullptr,nullptr,frame);});Reject([&]{empty.Initialize(device.Get(),0,10);});
    Fixture test(device.Get(),context.Get(),97,65);test.Run(frame);
    frame.currentClipToPreviousClip[12]=float(6.5/97);frame.currentClipToPreviousClip[13]=float(-4./65);test.Run(frame);
    frame.currentClipToPreviousClip[3]=.2f;frame.currentClipToPreviousClip[7]=-.1f;test.Run(frame);
    // A genuine perspective camera with changed yaw/pitch/translation, rather
    // than just a screen-space offset. Independent double inversion reference.
    Matrix projection{};projection[0]=1.04665291;projection[5]=.950307429;projection[8]=.242512614;projection[9]=.193187416;projection[10]=1.00130165;projection[11]=1;projection[14]=-13.016922;
    auto multiply=[](Matrix const& a,Matrix const& b){Matrix r{};for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x)for(unsigned k=0;k<4;++k)r[y*4+x]+=a[y*4+k]*b[k*4+x];return r;};
    Matrix view=Identity(),oldView=Identity();double angle=.035;oldView[0]=oldView[10]=std::cos(angle);oldView[2]=-std::sin(angle);oldView[8]=std::sin(angle);oldView[12]=.7;oldView[13]=-.15;
    frame.currentClipToPreviousClip=CameraReprojection(multiply(view,projection),multiply(oldView,projection));test.Run(frame);
    frame.currentRasterJitter={.375f,-.25f};frame.previousRasterJitter={-.125f,.25f};test.Run(frame);
    frame.currentClipToPreviousClip=same;
    // Pure raster jitter in clip space must cancel to subpixel zero.
    frame.currentClipToPreviousClip[12]=(frame.previousRasterJitter[0]-frame.currentRasterJitter[0])*2/test.width;
    frame.currentClipToPreviousClip[13]=-(frame.previousRasterJitter[1]-frame.currentRasterJitter[1])*2/test.height;test.Run(frame);
    frame.currentClipToPreviousClip=same;frame.currentRasterJitter=frame.previousRasterJitter={};
    test.SetSkin(30,20,7.5f,-12.25f,true);test.SetSkin(31,20,20,30,false);test.SetSkin(32,20,20000,0,true);
    test.known[0]=0;test.known[1]=128;test.depth[2]=1;test.depth[3]=-1;test.depth[4]=std::numeric_limits<float>::quiet_NaN();test.depth[5]=std::numeric_limits<float>::infinity();
    test.skin[6*4]=XMConvertFloatToHalf(std::numeric_limits<float>::infinity());test.Run(frame);
    test.skin[7*4+2]=XMConvertFloatToHalf(.25f);test.Run(frame);
    frame.historyValid=false;test.Run(frame);frame.historyValid=true;
    auto invalid=frame;invalid.currentRasterJitter[0]=std::numeric_limits<float>::infinity();Reject([&]{test.Run(invalid);});
    Reject([&]{test.composer.Process(context.Get(),test.views[0].Get(),test.views[1].Get(),nullptr,frame);});
    Reject([&]{test.composer.Process(context.Get(),test.views[0].Get(),test.views[0].Get(),test.views[2].Get(),frame);});
    Fixture wrongSize(device.Get(),context.Get(),98,65);Reject([&]{test.composer.Process(context.Get(),wrongSize.views[0].Get(),test.views[1].Get(),test.views[2].Get(),frame);});
    // Preserve existing compute bindings; the dedicated context's graphics
    // shader resource and viewport must also remain unchanged.
    ComPtr<ID3D11ComputeShader> shader;Hr(device->CreateComputeShader(TemporalMotionCS,sizeof(TemporalMotionCS),nullptr,&shader));context->CSSetShader(shader.Get(),nullptr,0);
    D3D11_BUFFER_DESC cb{};cb.ByteWidth=96;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;ComPtr<ID3D11Buffer> buffer;Hr(device->CreateBuffer(&cb,nullptr,&buffer));auto* b=buffer.Get();context->CSSetConstantBuffers(0,1,&b);
    ID3D11ShaderResourceView* seeds[]{test.views[2].Get(),test.views[1].Get(),test.views[0].Get(),test.views[2].Get()};context->CSSetShaderResources(0,4,seeds);context->PSSetShaderResources(5,1,&seeds[0]);
    D3D11_VIEWPORT vp{3,4,57,43,.1f,.9f};context->RSSetViewports(1,&vp);test.Run(frame);
    ComPtr<ID3D11Texture2D> unrelated;D3D11_TEXTURE2D_DESC udesc{};test.composer.Texture()->GetDesc(&udesc);Hr(device->CreateTexture2D(&udesc,nullptr,&unrelated));
    ComPtr<ID3D11UnorderedAccessView> unrelatedView;Hr(device->CreateUnorderedAccessView(unrelated.Get(),nullptr,&unrelatedView));auto* seedUav=unrelatedView.Get();context->CSSetUnorderedAccessViews(0,1,&seedUav,nullptr);test.Run(frame);
    ComPtr<ID3D11UnorderedAccessView> restoredUav;context->CSGetUnorderedAccessViews(0,1,&restoredUav);Require(restoredUav==unrelatedView,"Compute UAV not restored");
    std::vector<uint16_t> rigid(size_t(test.width)*test.height*4);
    auto setNative=[&](unsigned x,unsigned y,float vx,float vy,float coverage,float valid){float values[]{vx,vy,coverage,valid};for(unsigned c=0;c<4;++c)rigid[(size_t(y)*test.width+x)*4+c]=XMConvertFloatToHalf(values[c]);};
    setNative(10,10,1.25f,-2.5f,1,1);setNative(30,20,7.5f,-12.25f,1,1);setNative(31,20,1,2,1,1);setNative(32,20,3,4,1,1);test.Run(frame,&rigid);
    setNative(30,20,-6,9,1,1);test.Run(frame,&rigid); // conflicting coplanar motion
    setNative(10,10,1,2,0,1);test.Run(frame,&rigid); // malformed layer cannot become static
    frame.historyValid=false;test.Run(frame,&rigid);frame.historyValid=true;
    ComPtr<ID3D11ComputeShader> gotShader;UINT count=0;context->CSGetShader(&gotShader,nullptr,&count);Require(gotShader==shader,"Compute shader not restored");
    ComPtr<ID3D11Buffer> gotBuffer;context->CSGetConstantBuffers(0,1,&gotBuffer);Require(gotBuffer==buffer,"Compute constants not restored");
    for(unsigned i=0;i<4;++i){ComPtr<ID3D11ShaderResourceView> got;context->CSGetShaderResources(i,1,&got);Require(got.Get()==seeds[i],"Compute source not restored");}
    ComPtr<ID3D11ShaderResourceView> ps;context->PSGetShaderResources(5,1,&ps);Require(ps.Get()==seeds[0],"Graphics binding changed");D3D11_VIEWPORT gotVp{};count=1;context->RSGetViewports(&count,&gotVp);Require(!std::memcmp(&vp,&gotVp,sizeof(vp)),"Viewport changed");context->ClearState();
    Fixture otherEye(device.Get(),context.Get(),97,65);otherEye.Run(frame);Require(otherEye.composer.Texture()!=test.composer.Texture(),"Eye outputs aliased");
    // Independent physical expectations: projection sampling alone creates no
    // real motion, while an object's 3,-4 pixel movement must survive removal.
    Fixture jittered(device.Get(),context.Get(),97,65);std::fill(jittered.known.begin(),jittered.known.end(),0);
    CameraMotionFrame jf;jf.currentClipToPreviousClip=same;jf.historyValid=true;jf.currentRasterJitter={.375f,-.25f};jf.previousRasterJitter={-.125f,.25f};jf.geometryIncludesRasterJitter=true;
    jittered.SetSkin(10,10,-.5f,.5f,true);jittered.SetSkin(11,10,-.5f,.5f,true);jittered.SetSkin(13,10,-.5f,.5f,false);
    std::vector<uint16_t> nj(size_t(jittered.width)*jittered.height*4);
    auto JN=[&](unsigned x,float dx,float dy){float values[]{dx,dy,1,1};for(unsigned k=0;k<4;++k)nj[(size_t(10)*jittered.width+x)*4+k]=XMConvertFloatToHalf(values[k]);};JN(11,-.5f,.5f);JN(12,2.5f,-3.5f);
    jittered.Run(jf,&nj);D3D11_MAPPED_SUBRESOURCE jm{};Hr(context->Map(jittered.readback.Get(),0,D3D11_MAP_READ,0,&jm));
    for(unsigned x=10;x<=14;++x){auto* p=reinterpret_cast<uint16_t*>(static_cast<char*>(jm.pData)+10*jm.RowPitch)+x*4;float dx=XMConvertHalfToFloat(p[0]),dy=XMConvertHalfToFloat(p[1]);Require(dx==(x==12?3.f:0.f)&&dy==(x==12?-4.f:0.f),"Native/skin jitter was not removed exactly once");}
    context->Unmap(jittered.readback.Get(),0);jf.geometryIncludesRasterJitter=false;jittered.Run(jf,&nj); // Existing de-jittered producer contract is preserved.
    jf.historyValid=false;jittered.Run(jf,&nj);
    if(argc>1)Captured(device.Get(),context.Get(),argv[1]);
    test.composer.Reset();Require(!test.composer.Texture(),"Reset retained output");context->ClearState();context->Flush();
    std::puts("PASS hardware camera/skin motion: odd edges, depth/frustum, unknown coverage, skin priority, jitter, reset, large origins, state, separate eyes. No game provider/NGX claim.");return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
