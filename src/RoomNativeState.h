#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <initializer_list>
namespace diagnostic {
// CP 1.75/PUP profile, used only after the movement hook verifies Game.dll's
// complete SHA256 and the sensor's size, active flag and bound movement PS.
// These sensor bytes are vectors, flags, a borrowed movement pointer and a
// quaternion. They contain no owned allocations. No property-set header is
// overwritten. Keep joystick goals/flags intact during the collision probe.
struct RoomNativeState {
    unsigned char sensor[0x4c]{};
    float goal[3]{},latest[3]{},framePosition[3]{};
    unsigned char moving=0;
    void Save(void const* s,void const* m){
        auto a=static_cast<unsigned char const*>(s),b=static_cast<unsigned char const*>(m);
        memcpy(sensor,a+0x14,sizeof(sensor));memcpy(goal,b+0x2c0,sizeof(goal));memcpy(latest,b+0x360,sizeof(latest));
        // Native SaveFrameStates (0xed080) stores the previous root at +0x11c.
        // CalcAniVelocity (0xe1cd0), CorrectGoalMiss (0xe4de0) and collision
        // correction (0xe6760) compare the current root with this history.
        memcpy(framePosition,b+0x11c,sizeof(framePosition));moving=b[0x264];
    }
    void RestoreSensor(void* s)const{memcpy(static_cast<unsigned char*>(s)+0x14,sensor,sizeof(sensor));}
    void RestoreGuard(void* m)const{static_cast<unsigned char*>(m)[0x264]=moving;}
    static void Request(void* s,float const (&position)[3]){
        auto a=static_cast<unsigned char*>(s);memcpy(a+0x44,position,sizeof(position));a[0x40]=1;
    }
    static void Translate(void* destination,float const (&original)[3],float const (&delta)[3]){
        float value[3];for(unsigned i=0;i<3;++i)value[i]=original[i]+delta[i];memcpy(destination,value,sizeof(value));
    }
    void Commit(void* s,void* m,float const (&delta)[3])const{
        auto a=static_cast<unsigned char*>(s),b=static_cast<unsigned char*>(m);RestoreSensor(s);
        for(unsigned offset:{0x14u,0x20u,0x2cu,0x44u}){
            float old[3];memcpy(old,sensor+offset-0x14,sizeof(old));Translate(a+offset,old,delta);
        }
        Translate(b+0x2c0,goal,delta);Translate(b+0x360,latest,delta);
        // A room rebase must not look like native animation/goal overshoot on
        // the next frame. Retain only the joystick's real frame displacement.
        Translate(b+0x11c,framePosition,delta);RestoreGuard(m);
    }
    static void BeginTransfer(void* m){
        // Native controlled movement brackets its body/entity translation
        // with this same flag (Game.dll 0xe699a..0xe6a1b). Without it the
        // OnUpdatedWorldMatrix callback calls ResetMotion and zeros speed.
        static_cast<unsigned char*>(m)[0x264]=1;
    }
};
}
