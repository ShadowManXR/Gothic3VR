#include "HeadControlMath.h"
#include "TemporaryOverride.h"
#include "NativeMovementInput.h"
#include "DirectLocomotion.h"
#include <windows.h>
#include <cstdio>
#include <limits>
using namespace diagnostic;
struct FakeFrame
{
    Rotation3 native,view;
    float wished[3]{};
    unsigned calls=0,cleanups=0;
    int actionResult=0;
    float consumed[3]{};
};
void __cdecl NativeForward(void* context)
{
    auto& frame=*static_cast<FakeFrame*>(context);++frame.calls;
    // Numeric view rotation columns are world-space camera axes in Genome.
    frame.wished[0]=frame.view.m[2];frame.wished[1]=frame.view.m[5];frame.wished[2]=frame.view.m[8];
}
void __cdecl Restore(void* context)
{
    auto& frame=*static_cast<FakeFrame*>(context);frame.view=frame.native;++frame.cleanups;
}
void __cdecl AuthorAction(void* context)
{
    NativeForward(context);
    static_cast<FakeFrame*>(context)->actionResult=-1;
}
void __cdecl ConsumeCachedDirection(void* context)
{
    auto& frame=*static_cast<FakeFrame*>(context);
    for(unsigned i=0;i<3;++i)frame.consumed[i]=frame.wished[i];
}
void __cdecl Fault(void* context)
{
    ++static_cast<FakeFrame*>(context)->calls;
    RaiseException(0xe0424242,0,0,nullptr);
}
bool ExceptionTrial(FakeFrame& frame)
{
    __try {CallWithRestoration(Fault,Restore,&frame);}
    __except(GetExceptionCode()==0xe0424242?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
    return false;
}
int __cdecl main()
{
    unsigned failures=0,checks=0;
    auto check=[&](bool ok,char const* name){++checks;if(!ok)++failures;std::printf("%s %s\n",ok?"PASS":"FAIL",name);};
    auto approximately=[](float a,float b){return std::fabs(a-b)<.0001f;};
    constexpr float pi=3.14159265359f;
    float turned[3]={.6f,pi/2,.8f};float yaw=0;
    check(HorizontalHeading(NativeCameraRotation(turned),yaw)&&approximately(yaw,pi/2),"heading follows projected gaze despite pitch and roll");
    float flat[3]={0,yaw,0};auto heading=NativeCameraRotation(flat);
    check(approximately(heading.m[2],1)&&approximately(heading.m[5],0)&&approximately(heading.m[8],0),"right-facing W moves +X with no vertical motion");
    check(approximately(heading.m[0],0)&&approximately(heading.m[3],0)&&approximately(heading.m[6],-1),"right-facing D strafes toward -Z");
    check(approximately(-heading.m[2],-1)&&approximately(-heading.m[5],0),"right-facing S moves backward horizontally");
    float up[3]={pi/2,.7f,0};float previous=.7f;
    check(!HorizontalHeading(NativeCameraRotation(up),previous)&&approximately(previous,.7f),"vertical gaze retains previous heading without unstable yaw");
    float down[3]={-pi/2,.7f,0};
    check(!HorizontalHeading(NativeCameraRotation(down),previous),"downward gaze also retains previous heading");
    Rotation3 invalid;invalid.m[2]=std::numeric_limits<float>::quiet_NaN();
    check(!HorizontalHeading(invalid,previous),"invalid heading rejected");
    FakeFrame frame;frame.view=heading;
    CallWithRestoration(NativeForward,Restore,&frame);
    check(frame.calls==1&&frame.cleanups==1&&approximately(frame.wished[0],1),"native consumer called once while temporary heading is active");
    check(RotationError(frame.view,frame.native)<.0001f,"native camera restored before subsequent game processing");
    for(unsigned i=0;i<500;++i){frame.view=heading;CallWithRestoration(NativeForward,Restore,&frame);}
    check(frame.calls==501&&frame.cleanups==501&&RotationError(frame.view,{})<.0001f,"500 input frames restore the baseline without accumulation");
    frame.view=heading;
    check(ExceptionTrial(frame)&&frame.calls==502&&frame.cleanups==502&&RotationError(frame.view,frame.native)<.0001f,
          "game exception propagates and temporary camera is restored");
    // Reproduce the native two-stage ordering. A late camera change alone
    // cannot correct an already-authored command (the 0.4.0 regression).
    FakeFrame cached;NativeForward(&cached);
    cached.view=heading;CallWithRestoration(ConsumeCachedDirection,Restore,&cached);
    check(approximately(cached.consumed[0],0)&&approximately(cached.consumed[2],1),
          "late ProcessMovements override leaves previously authored direction unchanged");
    cached.view=heading;CallWithRestoration(AuthorAction,Restore,&cached);
    ConsumeCachedDirection(&cached);
    check(approximately(cached.consumed[0],1)&&approximately(cached.consumed[2],0)&&cached.actionResult==-1&&
          RotationError(cached.view,cached.native)<.0001f,
          "early action override retains full 90-degree direction and return result after restoration");
    float left[3]={0,-pi/2,0};cached.view=NativeCameraRotation(left);
    CallWithRestoration(AuthorAction,Restore,&cached);ConsumeCachedDirection(&cached);
    check(approximately(cached.consumed[0],-1)&&approximately(cached.consumed[2],0),
          "next held-key action follows new heading without a new key-down or accumulation");
    uint16_t key=0;bool filtered=true;
    for(uint16_t input=0;input<50;++input)
        filtered=filtered && (HorizontalMovementMessage(&input,key)==(input>=1&&input<=4)) && key==input;
    check(filtered,"native message filter selects only W/S/A/D actions and preserves key word");
    float velocity[3]={0,-37,240},wanted[3]={1,0,0},redirected[3]{};
    check(RedirectHorizontalVelocity(velocity,wanted,redirected)&&approximately(redirected[0],240)&&approximately(redirected[2],0),
          "physics velocity follows full 90-degree gaze without native goal interpolation");
    check(redirected[1]==velocity[1]&&approximately(std::hypot(redirected[0],redirected[2]),240),
          "physics steering preserves horizontal speed and vertical velocity");
    wanted[0]=-1;
    check(RedirectHorizontalVelocity(velocity,wanted,redirected)&&approximately(redirected[0],-240),"backward or left input keeps its signed direction");
    wanted[0]=1;wanted[2]=1;
    check(RedirectHorizontalVelocity(velocity,wanted,redirected)&&approximately(std::hypot(redirected[0],redirected[2]),240),
          "diagonal input does not increase native speed");
    float zero[3]{};
    check(!RedirectHorizontalVelocity(zero,wanted,redirected),"zero native speed never creates movement");
    check(!RedirectHorizontalVelocity(velocity,zero,redirected),"no input does not redirect braking or external motion");
    wanted[0]=std::numeric_limits<float>::quiet_NaN();
    check(!RedirectHorizontalVelocity(velocity,wanted,redirected),"nonfinite command rejected");
    bool modes=true;for(int i=0;i<18;++i)modes=modes&&(DirectWalkingMode(i)==((i>=2&&i<=5)||i==10));
    check(modes,"direct steering applies to ground walking modes, excludes jump swim fall and ragdoll");
    std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
