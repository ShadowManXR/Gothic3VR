#include "NativeCameraReturn.h"
#include "KnockoutCamera.h"
#include "GameStereoMath.h"
#include "CameraComfort.h"
#include <cstdio>
#include <limits>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
float Read(unsigned char const* p,size_t offset){float out=0;std::memcpy(&out,p+offset,4);return out;}
int main(){
    CameraReturnTransition transition;KnockoutCamera recovery;
    CHECK(!transition.Observe(1,2,false)); // initial FP is not a return
    for(unsigned repeat=0;repeat<10;++repeat){
        for(int action:{29,30,31,32,33,37}){
            CHECK(!transition.Observe(1,2,recovery.Update(1,action,16,true,false)));
            for(unsigned i=0;i<50;++i)CHECK(!transition.Observe(1,2,recovery.Update(1,action,16,false,false)));
            CHECK(transition.Blocks(1)&&!transition.Blocks(3));
            CHECK(transition.Observe(1,2,recovery.Update(1,0,2,false,false)));
            CHECK(transition.Observe(1,2,false)); // persist until the camera can commit
            transition.Complete();CHECK(!transition.Blocks(1));for(unsigned i=0;i<50;++i)CHECK(!transition.Observe(1,2,false));
        }
        CHECK(!transition.Observe(1,2,true)); // native dialogue, FP flag may still be set
        CHECK(transition.Observe(1,2,false));transition.Complete();CHECK(!transition.Observe(1,2,false));
    }
    CHECK(!transition.Observe(1,2,true));CHECK(!transition.Observe(3,2,false)); // actor/save
    CHECK(!transition.Observe(3,2,true));CHECK(!transition.Observe(3,4,false)); // camera replaced
    CHECK(!transition.Observe(3,4,true));CHECK(!transition.Observe(0,4,false));
    CHECK(!transition.Observe(3,4,false));CHECK(!transition.Observe(3,4,true));
    CHECK(transition.Observe(3,4,false));CHECK(!transition.Observe(3,4,true)); // second exception wins
    transition.Reset();CHECK(!transition.Observe(3,4,false));

    for(float yaw:{-3.14f,-2.4f,-1.2f,0.f,.6f,1.7f,3.14f})for(float scale:{.5f,1.f,2.f}){
        Rotation3 facing;CHECK(NativeCameraReturn::Facing(std::sin(yaw)*scale,std::cos(yaw)*scale,facing));
        unsigned char camera[0x558],before[sizeof(camera)];std::memset(camera,0x59,sizeof(camera));
        float opposite[3]{0,yaw+3.14159265f,0};auto old=NativeCameraRotation(opposite);
        for(size_t offset:{NativeCameraReturn::Current,NativeCameraReturn::History})for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)
            std::memcpy(camera+offset+(r*4+c)*4,&old.m[c*3+r],4);
        std::memcpy(before,camera,sizeof(camera));CHECK(NativeCameraReturn::Seed(camera,sizeof(camera),facing));
        // Both the FP target and interpolation history must change. Changing
        // SetTarget's actor26C alone leaves the old, opposite view in both.
        for(size_t offset:{NativeCameraReturn::Current,NativeCameraReturn::History}){
            Rotation3 view;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)view.m[c*3+r]=Read(camera,offset+(r*4+c)*4);
            CHECK(RotationError(view,facing)<.00001f);
            CHECK(view.m[2]*std::sin(yaw)+view.m[8]*std::cos(yaw)>.9999f);
        }
        CHECK(Read(camera,0x218)==0&&Read(camera,0x21c)==0);
        for(size_t i=0;i<sizeof(camera);++i){bool written=i>=0x218&&i<0x220;
            for(size_t offset:{NativeCameraReturn::Current,NativeCameraReturn::History})for(unsigned r=0;r<3;++r)
                written|=i>=offset+r*16&&i<offset+r*16+12;
            if(!written)CHECK(camera[i]==before[i]); // position/target/preset/actor untouched
        }
        // Recenter the physical head once, then use the actual stereo transform:
        // both eyes face the native hero; later free head yaw remains tracked.
        Orientation physical{std::cos(.7f),0,std::sin(.7f),0},center;float head[3]{.2f,1.7f,-.3f},origin[3]{};
        CHECK(RecenterTracking(physical,head,true,center,origin));
        for(float ipd:{-.032f,.032f}){
            StereoEyePose eye{};eye.position[0]=head[0]+ipd;eye.position[1]=head[1];eye.position[2]=head[2];
            eye.quaternion[1]=physical.y;eye.quaternion[3]=physical.w;
            float base[3]{1000,165,-700},position[3]{},angles[3]{};
            CHECK(GameEyeTransform(facing,base,center,origin,eye,100,position,angles));
            auto rendered=NativeCameraRotation(angles);CHECK(RotationError(rendered,facing)<.0001f);
            auto turned=Product(physical,Orientation{std::cos(.2f),0,std::sin(.2f),0});
            eye.quaternion[1]=turned.y;eye.quaternion[3]=turned.w;
            CHECK(GameEyeTransform(facing,base,center,origin,eye,100,position,angles));
            CHECK(RotationError(NativeCameraRotation(angles),facing)>.1f);
        }
    }
    unsigned char invalid[0x558],saved[sizeof(invalid)];std::memset(invalid,0x71,sizeof(invalid));std::memcpy(saved,invalid,sizeof(invalid));
    Rotation3 view;CHECK(!NativeCameraReturn::Facing(0,0,view)&&!NativeCameraReturn::Facing(NAN,1,view));
    CHECK(!NativeCameraReturn::Seed(nullptr,sizeof(invalid),view));CHECK(!NativeCameraReturn::Seed(invalid,10,view));
    view.m[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!NativeCameraReturn::Seed(invalid,sizeof(invalid),view));
    CHECK(!std::memcmp(saved,invalid,sizeof(invalid)));
    puts("PASS native third-person exit ownership, all down/get-up poses, dialogue, save/camera resets, opposite FP cache/history correction, byte-limited writes, recenter and independent stereo head tracking");return 0;
}
