#include "MeleeHandling.h"
#include "LockedMeleeAttachment.h"
#include "StaffMelee.h"
#include <cstdio>
#include <limits>
using namespace diagnostic;using namespace diagnostic::arms;using namespace diagnostic::melee;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
bool Near(Vec a,Vec b,float epsilon=.002f){return Length(a-b)<epsilon;}
bool Near(Mat const& a,Mat const& b,float epsilon=.002f){for(unsigned i=0;i<16;++i)if(std::fabs(a.m[i]-b.m[i])>epsilon)return false;return true;}
Mat Pose(float angle,Vec p={}){auto out=Rotation(Matrix({std::cos(angle*.5f),0,std::sin(angle*.5f),0}));Position(out,p);return out;}
int main(){
    // Verify the exact engine/XR handedness, scale, origin and camera offset
    // conversion using the real GripWorld path, not an invented world frame.
    for(float yaw:{0.f,.8f,2.4f})for(float scale:{70.f,100.f,145.f}){
        float nativeAngles[3]{.2f,yaw,-.1f},base[3]{41000,120,-27000},origin[3]{.4f,1.5f,-.3f};
        Orientation centre{.9238795f,0,.3826834f,0};ControllerPose zero;zero.valid=zero.tracked=true;zero.quaternion[3]=1;
        Mat anchor;CHECK(GripWorld(zero,NativeCameraRotation(nativeAngles),base,centre,origin,scale,-25,anchor));
        for(float angle:{-.9f,0.f,1.2f}){ControllerPose raw=zero;raw.position[0]=.25f;raw.position[1]=1.3f;raw.position[2]=-.6f;
            raw.quaternion[0]=std::sin(angle*.5f);raw.quaternion[3]=std::cos(angle*.5f);
            Mat world,tracking,roundtrip;CHECK(GripWorld(raw,NativeCameraRotation(nativeAngles),base,centre,origin,scale,-25,world));
            CHECK(HandlingToTracking(world,anchor,scale,tracking)&&HandlingToWorld(tracking,anchor,scale,roundtrip));
            Mat expected=Rotation(Matrix({raw.quaternion[3],raw.quaternion[0],0,0}));Position(expected,{.25f,1.3f,-.6f});
            CHECK(Near(tracking,expected,.0002f)&&Near(roundtrip,world,.02f));
        }
    }
    for(auto profile:{HandlingProfile::OneHand,HandlingProfile::SupportedTwoHand,HandlingProfile::StaffOneHand,HandlingProfile::StaffTwoHand}){
        MeleeHandling off;Mat target=Pose(1.3f,{.3f,1.2f,-.5f}),out;
        CHECK(off.Step(1,1,1000,profile,target,0,0,out)&&!memcmp(&target,&out,sizeof(target)));
        float errors[3]{};int index=0;
        for(int weight:{0,20,100}){MeleeHandling filter;Mat initial=Pose(0,{0,1.2f,-.5f});
            CHECK(filter.Step(1,1,1000,profile,initial,weight,0,out));
            CHECK(filter.Step(1,1,1011,profile,target,weight,0,out));
            Orientation q;CHECK(PoseOrientation(out,q));errors[index++]=Angle(q,{std::cos(.65f),0,std::sin(.65f),0});
            if(weight){CHECK(Length(Position(out)-Position(target))<=.02f+.035f*weight*.01f+.0001f);
                if(profile==HandlingProfile::StaffOneHand)CHECK(Angle(q,{})<=StaffOneHandAngularSpeed(weight)*.011f+.001f);
                else CHECK(errors[index-1]<=(8+18*weight*.01f)*.01745329252f+.001f);}
            auto first=out;CHECK(filter.Step(1,1,1011,profile,target,weight,0,out)&&Near(first,out));
            float previous=errors[index-1];
            for(uint64_t stamp=1022;stamp<2100;stamp+=11){CHECK(filter.Step(1,1,stamp,profile,target,weight,0,out));CHECK(PoseOrientation(out,q));
                float error=Angle(q,{std::cos(.65f),0,std::sin(.65f),0});CHECK(error<=previous+.001f);previous=error;}
            CHECK(previous<.005f); // settles without spring overshoot
        }
        CHECK(errors[0]<.001f&&errors[1]>errors[0]&&errors[2]>errors[1]);
        MeleeHandling stable;double rawEnergy=0,filteredEnergy=0;uint64_t stamp=3000;
        CHECK(stable.Step(1,1,stamp,profile,Mat{},0,40,out));
        for(int i=0;i<300;++i){stamp+=11;float jitter=(i&1?.015f:-.015f);target=Pose(jitter,{jitter*.02f,0,0});
            CHECK(stable.Step(1,1,stamp,profile,target,0,40,out));Orientation q;CHECK(PoseOrientation(out,q));
            rawEnergy+=jitter*jitter;float angle=Angle(q,{});filteredEnergy+=angle*angle;}
        CHECK(filteredEnergy<rawEnergy*.1);
        target=Pose(.5f,{.1f,0,0});CHECK(stable.Step(2,1,stamp+11,profile,target,200,150,out)&&Near(out,target));
        CHECK(stable.Step(2,2,stamp+22,profile,Mat{},200,150,out)&&Near(out,Mat{}));
        CHECK(stable.Step(2,2,stamp+250,profile,target,200,150,out)&&Near(out,target));
        CHECK(stable.Step(2,2,12,profile,Mat{},200,150,out)&&Near(out,Mat{}));
        CHECK(stable.Step(2,2,23,profile,target,0,0,out)&&!memcmp(&target,&out,sizeof(target)));
        Mat invalid;invalid.m[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!stable.Step(2,2,34,profile,invalid,20,20,out));
    }
    // Fast unsupported shaft rotations remain genuinely slower at higher
    // weight; the old small angle-gap clamp must not pull them back to the
    // current controller every frame. Translation and the rigid grip stay put.
    {
        float travelled[3]{};unsigned index=0;
        for(int weight:{25,110,200}){
            MeleeHandling staff;Mat out;CHECK(staff.Step(11,7,1000,HandlingProfile::StaffOneHand,Mat{},weight,50,out));
            Orientation previous{};
            for(unsigned frame=1;frame<=40;++frame){
                auto target=Pose(frame*.08f);CHECK(staff.Step(11,7,1000+frame*10,HandlingProfile::StaffOneHand,target,weight,50,out));
                Orientation q;CHECK(PoseOrientation(out,q));float step=Angle(previous,q);
                CHECK(step<=StaffOneHandAngularSpeed(weight)*.01f+.001f);travelled[index]+=step;previous=q;
                CHECK(Near(Position(out),{}));
            }++index;
        }
        CHECK(travelled[0]>travelled[1]+.2f&&travelled[1]>travelled[2]+.2f);
        MeleeHandling supported,reference;Mat a,b;
        // Staff Two-Hand keeps the original generic response, even under the
        // same high-speed gesture and heavy slider values.
        for(unsigned frame=0;frame<100;++frame){auto target=Pose(std::sin(frame*.12f)*1.7f,{.1f*std::sin(frame*.1f),1.2f,-.5f});
            CHECK(supported.Step(12,7,2000+frame*10,HandlingProfile::StaffTwoHand,target,110,50,a));
            CHECK(reference.Step(12,7,2000+frame*10,HandlingProfile::OneHand,target,110,50,b));
            CHECK(!memcmp(&a,&b,sizeof(a)));
        }
        // Switch support on/off at a duplicated render timestamp without a
        // new solve or an orientation snap. Normal two-hand policy resumes.
        auto before=a;auto newDirection=Pose(2,Position(before));
        CHECK(supported.Step(12,7,2990,HandlingProfile::StaffOneHand,newDirection,110,50,a)&&Near(a,before));
        CHECK(supported.Step(12,7,2990,HandlingProfile::StaffTwoHand,newDirection,110,50,a)&&Near(a,before));
    }
    // One final frame transports both attached palms and the staff/2H mesh.
    // A moving native world must not count as a physical stroke or re-filter.
    MeleeHandling pair;SupportAttachment support;Mat before=Pose(.4f,{40000,150,-20000}),mesh=before,left=before;
    Position(left,Point(before,{0,0,25}));Vec palm=Position(left);Blade handle{{0,0,5},{0,0,50}};
    CHECK(support.Capture(mesh,handle,palm,left));
    Mat raw=Pose(.4f,{.2f,1.3f,-.5f}),filtered;
    CHECK(pair.Step(1,1,5000,HandlingProfile::SupportedTwoHand,raw,40,20,filtered));
    raw=Pose(.7f,{.25f,1.3f,-.5f});CHECK(pair.Step(1,1,5011,HandlingProfile::SupportedTwoHand,raw,40,20,filtered));
    for(float yaw:{0.f,1.f,2.7f}){
        Mat anchor=Pose(yaw,{50000,300,-30000}),worldTarget,worldFiltered,delta;
        CHECK(HandlingToWorld(raw,anchor,100,worldTarget)&&HandlingToWorld(filtered,anchor,100,worldFiltered)&&HandlingAssembly(worldTarget,worldFiltered,delta));
        Mat supportGrip=worldTarget;Position(supportGrip,Point(worldTarget,{0,0,25}));
        Vec wrist=Position(supportGrip),contact=Point(worldTarget,{0,0,25});
        auto movedGrip=Multiply(supportGrip,delta);auto movedMesh=Multiply(worldTarget,delta);
        CHECK(Near(Position(movedGrip),Point(movedMesh,{0,0,25}),.03f));
        CHECK(Near(Point(delta,wrist),Position(movedGrip),.03f)&&Near(Point(delta,contact),Point(movedMesh,{0,0,25}),.03f));
        Mat recovered;CHECK(HandlingToTracking(worldFiltered,anchor,100,recovered)&&Near(recovered,filtered,.0002f));
        Mat again;CHECK(pair.Step(1,1,5011,HandlingProfile::SupportedTwoHand,raw,40,20,again)&&Near(again,filtered));
    }
    // Switching from one-hand staff to support seeds the already rendered
    // pose, so changing the handling profile cannot realign the weapon.
    CHECK(pair.Step(1,1,5011,HandlingProfile::StaffOneHand,filtered,25,15,raw)&&Near(raw,filtered));
    CHECK(pair.Step(1,1,5011,HandlingProfile::StaffTwoHand,raw,30,20,filtered)&&Near(raw,filtered));
    auto held=filtered;Mat newTarget=Pose(1.2f,{.3f,1.3f,-.5f});
    CHECK(pair.Step(1,1,5011,HandlingProfile::StaffOneHand,newTarget,25,15,filtered)&&Near(filtered,held));
    CHECK(pair.Step(1,1,5022,HandlingProfile::StaffOneHand,newTarget,25,15,filtered)&&!Near(filtered,newTarget));
    CHECK(pair.Step(1,1,5033,HandlingProfile::StaffTwoHand,newTarget,30,20,filtered)&&!Near(filtered,newTarget));
    puts("PASS four independent weight/stabilization profiles, true XR/world mapping, exact off, bounded lag, jitter reduction, monotonic settling, resets, shared rigid palms and walking/turning invariance");return 0;
}
