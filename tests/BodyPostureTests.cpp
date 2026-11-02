#include "BodyPosture.h"
#include "TrackedBodyMath.h"
#include "CameraComfort.h"
#include "TrackedPalette.h"
#include "FrozenArmPalette.h"
#include <cstdio>
using namespace diagnostic;using namespace diagnostic::arms;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
bool Near(Vec a,Vec b,float units){return Length(a-b)<.001f*units;}
Mat Pose(Vec p,float pitch=0,float yaw=0){float a[3]{pitch,yaw,0};auto m=Rotation(NativeCameraRotation(a));Position(m,p);return m;}
int main(){
    CHECK(PickupLowerBody(66,"PS_Player","PS_Normal_Loop"));
    CHECK(PickupLowerBody(65,"PS_Player","PS_Normal_Take"));
    CHECK(PickupLowerBody(0,"_AI_TakeItem","PS_Normal_Loop"));
    for(int action:{15,29,33,37,57,72,73,75,77,78,83,98,99,100,103})
        CHECK(!PickupLowerBody(action,"_AI_Interact","PS_Normal_Loop"));
    CHECK(!PickupLowerBody(0,"_AI_TakeItemOther","PS_Normal_TakeOther"));
    for(float units:{50.f,100.f,200.f}){
        constexpr unsigned n=6;constexpr int pelvis=0;
        BodyPart parts[n]{BodyPart::Neutral,BodyPart::Neutral,BodyPart::Leg,BodyPart::Leg,BodyPart::Leg,BodyPart::Other};
        Mat bind[n]{Pose({0,.95f*units,0}),Pose({0,1.5f*units,0}),Pose({.12f*units,.52f*units,0}),
                    Pose({.12f*units,0,.08f*units}),Pose({-.12f*units,0,.08f*units}),Pose({0,1.2f*units,0})};
        Mat entity=Pose({876*units,53*units,-95*units},0,.6f),entityBefore=entity,ground;
        float a[3]{0,1.3f,0};CHECK(BodyWorld(entity,NativeCameraRotation(a),ground,{3,0,-4}));
        Mat model[n],bone[n],before[n],delta[n],body;
        memcpy(model,bind,sizeof(model));model[0]=Pose({0,.55f*units,0},.45f);
        model[1]=Pose({0,1.1f*units,.14f*units},.3f);
        model[2]=Pose({.12f*units,.29f*units,.18f*units},-.7f);
        for(unsigned i=0;i<n;++i)bone[i]=Multiply(model[i],entity);
        memcpy(before,bone,sizeof(before));
        CHECK(BodyPoseDeltas(entity,ground,bind,bone,parts,n,pelvis,true,false,nullptr,body,delta));
        for(unsigned i=2;i<5;++i)CHECK(Near(Position(Multiply(bone[i],delta[i])),Position(Multiply(model[i],ground)),units));
        CHECK(Near(Position(Multiply(bone[0],delta[0])),Position(Multiply(model[0],ground)),units));
        CHECK(std::fabs(Position(body).y-Position(ground).y+.4f*units)<.001f*units);
        CHECK(std::fabs(Position(Multiply(bone[1],delta[1])).y-(Position(ground).y+1.1f*units))<.001f*units);
        CHECK(!memcmp(before,bone,sizeof(before))&&!memcmp(&entityBefore,&entity,sizeof(entity)));
        // Existing upright motion retains the previous exact pelvis/leg delta.
        CHECK(BodyPoseDeltas(entity,ground,bind,bone,parts,n,pelvis,false,false,nullptr,body,delta));
        Mat old;CHECK(Rebase(bone[0],Multiply(bind[0],ground),old));
        for(unsigned i=2;i<5;++i)CHECK(!memcmp(&old,&delta[i],sizeof(old)));
        // Native pickup's changing knee/pelvis pose cannot raise the feet.
        for(unsigned frame=0;frame<40;++frame){
            auto take=Pose({0,-float(frame%20)*units*.02f,0},float(frame)*.02f);
            for(unsigned i=0;i<n;++i)bone[i]=Multiply(Multiply(model[i],take),entity);
            CHECK(BodyPoseDeltas(entity,ground,bind,bone,parts,n,pelvis,false,true,nullptr,body,delta));
            for(unsigned i=0;i<5;++i)CHECK(Near(Position(Multiply(bone[i],delta[i])),Position(Multiply(bind[i],ground)),units));
            // The last crouch is retained through pickup while moving across
            // terrain; its model-space reference is never stuck in the world.
            auto moving=ground;Position(moving,Position(ground)+Vec{frame*2.f,frame*.5f,-float(frame)});
            CHECK(BodyPoseDeltas(entity,moving,bind,bone,parts,n,pelvis,true,true,model,body,delta));
            for(unsigned i:{0u,2u,3u,4u})CHECK(Near(Position(Multiply(bone[i],delta[i])),Position(Multiply(model[i],moving)),units));
        }
        // Both eyes consume one posed palette, then exact native bytes return.
        TrackedPalette saved;FrozenArmPalette frozen;Mat native[n];memcpy(native,bone,sizeof(native));
        CHECK(saved.Begin(bone,n));for(unsigned i=0;i<n;++i)if(parts[i]!=BodyPart::Other)bone[i]=Multiply(bone[i],delta[i]);
        CHECK(frozen.Capture(1,2,bone,n)&&saved.Restore(bone,n));
        for(unsigned eye=0;eye<2;++eye){CHECK(saved.Begin(bone,n)&&frozen.Apply(1,2,bone,n)&&saved.Restore(bone,n));CHECK(!memcmp(native,bone,sizeof(native)));}

        BodyPosture stance;uint64_t time=1000;stance.Update(1,2,time,units,false,false,0);
        float previous=0;
        for(unsigned i=1;i<=90;++i){time+=16;float drop=-std::min(i/20.f,1.f)*.4f*units;
            stance.Update(1,2,time,units,true,false,drop);
            CHECK(stance.Crouched()&&stance.CameraHeight()<=previous+.0001f&&previous-stance.CameraHeight()<.035f*units);
            previous=stance.CameraHeight();float same=previous;stance.Update(1,2,time,units,true,false,drop);CHECK(stance.CameraHeight()==same);
        }
        CHECK(std::fabs(stance.CameraHeight()+.4f*units)<.001f*units);
        float stable=stance.CameraHeight();
        for(unsigned i=0;i<180;++i){time+=16;stance.Update(1,2,time,units,true,false,(-.4f+std::sin(i*.3f)*.025f)*units);CHECK(std::fabs(stance.CameraHeight()-stable)<.001f*units);}
        time+=16;stance.Update(1,2,time,units,false,true,-.9f*units);CHECK(stance.Crouched()&&stance.Pickup());
        for(unsigned i=0;i<20;++i){time+=16;stance.Update(1,2,time,units,true,false,-.9f*units);CHECK(std::fabs(stance.CameraHeight()-stable)<.001f*units);}
        for(unsigned i=0;i<100;++i){time+=16;stance.Update(1,2,time,units,false,false,0);}
        CHECK(!stance.Crouched()&&!stance.Pickup()&&stance.CameraHeight()==0);
        // Load directly into sneak without calibrating a second crouch offset.
        time+=16;stance.Update(9,2,time,units,true,false,-.4f*units);CHECK(stance.CameraHeight()==-.4f*units);
        CameraHeightAnchor anchor;float root[3]{10*units,5*units,20*units},view[3]{0,6.3f*units,0};
        anchor.Position(9,root,view,stance.NativeHeight());view[1]+=stance.CameraHeight();
        CHECK(std::fabs(view[1]-6.3f*units)<.001f*units);
        root[1]+=.12f*units;anchor.Position(9,root,view,stance.NativeHeight());view[1]+=stance.CameraHeight();
        CHECK(std::fabs(view[1]-6.42f*units)<.001f*units);
        stance.Update(0,2,time,units,false,false,0);CHECK(!stance.Crouched()&&stance.CameraHeight()==0);
        CHECK(!BodyPoseDeltas(entity,ground,bind,bone,parts,n,-1,true,false,nullptr,body,delta));
    }
    puts("PASS grounded native crouch and lowered torso, upright path unchanged, standing/crouched pickup leg suppression, moving ground, frozen two-eye restoration, smooth stance-only camera, no walking bob, load/slope calibration and lifecycle reset");
    return 0;
}
