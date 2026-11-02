#include "VrSettings.h"
#include "FfrFeather.h"
#include <cstdio>
#include <string>
int main(){int failures=0;auto need=[&](bool b,char const* m){if(!b){std::printf("FAIL %s\n",m);++failures;}};
    for(auto size:{512u,3840u,5120u}){
        auto p=diagnostic::FfrPattern(size,size,16,16,.5f,.5f,1,75);unsigned w=(size+15)/16;need(p[w*(w/2)+w/2]==0,"Native center");need(p[0]==1,"Quality corners use 2x2, never4x4/culling");
        auto off=diagnostic::FfrPattern(size,size,16,16,.5f,.5f,0,75);need(std::all_of(off.begin(),off.end(),[](auto n){return n==0;}),"Off native everywhere");
        auto perf=diagnostic::FfrPattern(size,size,16,16,.5f,.5f,3,45);need(perf[0]==2&&perf[w*(w/2)+w/2]==0,"Performance retains center and reduces extremecorners");}
    need(diagnostic::FfrPattern(513,321,16,16,.45f,.55f,1,75).size()==33*21,"Odd viewport rounds mask edges up");
    need(diagnostic::FfrTile(0,0,16,16,512,512,.025f,.025f,1,40)==0,"A tile touching focus center is protected");
    for(auto dimensions:{std::pair<unsigned,unsigned>{1024,1024},{3840,2160},{2160,3840}}){
        unsigned width=dimensions.first,height=dimensions.second,w=(width+7)/8,h=(height+7)/8;
        auto oval=diagnostic::FfrPattern(width,height,8,8,.5f,.5f,2,60);
        unsigned horizontal=0,vertical=0;
        for(unsigned x=0;x<w;++x)horizontal+=oval[(h/2)*w+x]==0;
        for(unsigned y=0;y<h;++y)vertical+=oval[y*w+w/2]==0;
        float normalizedRatio=(float(horizontal)/w)/(float(vertical)/h);
        need(normalizedRatio>1.44f&&normalizedRatio<1.56f,"Landscape oval is wider horizontally, including non-square eye textures");
        float coverage=float(std::count(oval.begin(),oval.end(),uint8_t(0)))/oval.size();
        float ideal=3.141592654f*.6f*.6f/4;
        need(coverage>=ideal&&coverage<ideal+.022f,"Oval retains nominal full-detail area with conservative tile protection");
        // Same distance from focus: protect the horizontal shoulder, reduce
        // detail above/below it. This rejects a circle and a portrait oval.
        need(oval[(h/2)*w+unsigned(.82f*w)]==0,"Horizontal detail extends beyond a circle");
        need(oval[unsigned(.82f*h)*w+w/2]!=0,"Vertical edge is outside the wide oval");
    }
    {
        unsigned w=128;auto left=diagnostic::FfrPattern(2048,2048,16,16,.4f,.5f,3,60);
        auto right=diagnostic::FfrPattern(2048,2048,16,16,.6f,.5f,3,60);
        bool mirrored=true;for(unsigned y=0;y<w;++y)for(unsigned x=0;x<w;++x)mirrored&=left[y*w+x]==right[y*w+w-1-x];
        need(mirrored,"Asymmetric eye centers remain mirrored rather than shifting the oval to one side");
        // Verify the conservative nearest-point rule against dense samples
        // inside each tile: no partially full-detail tile may be reduced.
        bool protectedTiles=true;for(unsigned y=0;y<w;++y)for(unsigned x=0;x<w;++x){
            if(left[y*w+x]==0)continue;
            for(unsigned sy=0;sy<=4;++sy)for(unsigned sx=0;sx<=4;++sx){
                float dx=2*((x+sx*.25f)/w-.4f),dy=2*((y+sy*.25f)/w-.5f);
                // Independent analytic 3:2 ellipse with equal nominal area.
                if(dx*dx/1.5f+dy*dy*1.5f<.6f*.6f)protectedTiles=false;
            }
        }
        need(protectedTiles,"Every tile touching the full-detail oval remains at native shading rate");
    }
    for(int mode=1;mode<=3;++mode){size_t previous=0;for(int diameter=40;diameter<=95;diameter+=5){
        auto p=diagnostic::FfrPattern(3840,2160,16,16,.46f,.52f,mode,diameter);
        size_t native=std::count(p.begin(),p.end(),uint8_t(0));need(native>previous,"Every diameter step enlarges the full-detail area in every profile");previous=native;}}
    {
        auto p=diagnostic::MakeFfrFeather(3840,2160,2,60,20);float previous=0;bool monotonic=true;
        for(unsigned i=0;i<=100;++i){float u=.5f+float(i)*.005f;float weight=diagnostic::FfrFeatherAmount(p,u,.5f);monotonic&=weight>=previous&&weight>=0&&weight<=1;previous=weight;}
        need(monotonic&&previous==1&&diagnostic::FfrFeatherAmount(p,.5f,.5f)==0,"Pixel blur weight is continuous, monotonic and leaves the inner center untouched");
        need(!diagnostic::MakeFfrFeather(3840,2160,2,60,0).enabled&&!diagnostic::MakeFfrFeather(3840,2160,0,60,20).enabled,"Zero width and FFR Off bypass pixel filtering");
        auto left=diagnostic::MakeFfrFeather(2048,2048,2,60,20,.4f,.5f),right=diagnostic::MakeFfrFeather(2048,2048,2,60,20,.6f,.5f);bool mirrored=true;
        for(unsigned i=0;i<100;++i)mirrored&=std::fabs(diagnostic::FfrFeatherAmount(left,i*.01f,.7f)-diagnostic::FfrFeatherAmount(right,1-i*.01f,.7f))<.00001f;
        need(mirrored,"Per-pixel feather respects asymmetric stereo centers without a tiled pattern");
    }
    wchar_t dir[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"ffr",0,path);auto s=diagnostic::LoadVrSettings(path);
    need(s.ffrMode==2&&s.ffrDiameterPercent==60&&s.ffrTransitionPercent==10,"Existing settings keep creator FFR defaults with a 10 percent transition");
    s.ffrMode=2;s.ffrDiameterPercent=80;s.ffrTransitionPercent=20;s.numbers[0]=-35;s.upscaleMethod=diagnostic::UpscaleMethod::Off;need(diagnostic::SaveVrSettings(path,s),"FFR save");auto loaded=diagnostic::LoadVrSettings(path);
    need(loaded.ffrMode==2&&loaded.ffrDiameterPercent==80&&loaded.ffrTransitionPercent==20&&loaded.numbers[0]==-35,"FFR transition roundtrip preserves diameter and calibration");
    s.ffrMode=999;s.ffrDiameterPercent=200;s.ffrTransitionPercent=999;need(diagnostic::SaveVrSettings(path,s),"Bounded FFR save");loaded=diagnostic::LoadVrSettings(path);need(loaded.ffrMode==0&&loaded.ffrDiameterPercent==95&&loaded.ffrTransitionPercent==30,"Malformed FFR parameters bounded");DeleteFileW(path);auto backup=std::wstring(path)+L".bak";DeleteFileW(backup.c_str());return failures?1:0;
}
