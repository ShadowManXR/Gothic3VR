#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace diagnostic {
// Normalize in panel coordinates, so 1080p/4K/5K have the same feel. Slow
// motion is filtered more strongly; deliberate movement raises the cutoff.
class MenuPointer {
    bool valid=false,previousClick=false;
    uint64_t previousTime=0,clickUntil=0;
    float raw[2]{},filtered[2]{},velocity[2]{},output[2]{},clickPoint[2]{};
    static float Alpha(float cutoff,float seconds){float ratio=6.2831853f*cutoff*seconds;return ratio/(1+ratio);}
public:
    void Reset(){valid=false;previousClick=false;clickUntil=0;}
    void Filter(float& u,float& v,uint64_t time,bool click) {
        float sample[2]={u,v};
        if(!std::isfinite(u)||!std::isfinite(v)){Reset();return;}
        if(!valid||time<previousTime||time-previousTime>100){
            for(unsigned i=0;i<2;++i){raw[i]=filtered[i]=output[i]=sample[i];velocity[i]=0;}
            valid=true;previousTime=time;previousClick=click;clickUntil=0;return;
        }
        if(time>previousTime){
            float seconds=float(time-previousTime)*.001f,derivativeAlpha=Alpha(1.f,seconds);
            for(unsigned i=0;i<2;++i){float speed=(sample[i]-raw[i])/seconds;velocity[i]+=derivativeAlpha*(speed-velocity[i]);raw[i]=sample[i];}
            float alpha=Alpha(2.5f+10.f*std::hypot(velocity[0],velocity[1]),seconds);
            for(unsigned i=0;i<2;++i)filtered[i]+=alpha*(sample[i]-filtered[i]);
            previousTime=time;
        }
        // The initial trigger squeeze can move the wrist. Keep the last
        // indicated point for 100 ms; held-trigger dragging resumes afterward.
        if(click&&!previousClick){clickUntil=time+100;clickPoint[0]=output[0];clickPoint[1]=output[1];}
        previousClick=click;if(!click)clickUntil=0;
        for(unsigned i=0;i<2;++i){if(clickUntil&&time<clickUntil)output[i]=clickPoint[i];else if(std::fabs(filtered[i]-output[i])>=.00075f)output[i]=filtered[i];}
        u=std::clamp(output[0],0.f,1.f);v=std::clamp(output[1],0.f,1.f);
    }
};
class ControllerCameraReset {
    bool armed=false,timing=false;
    uint64_t began=0;
public:
    bool Step(bool allowed,bool left,bool right,uint64_t time) {
        if(!allowed){armed=false;timing=false;return false;}
        if(!left&&!right){armed=true;timing=false;return false;}
        if(!left||!right){timing=false;return false;}
        if(!armed)return false;
        if(!timing||time<began){timing=true;began=time;return false;}
        if(time-began<500)return false;
        armed=false;timing=false;return true;
    }
};
}
