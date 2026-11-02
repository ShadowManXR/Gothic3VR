#include "TwoHandMelee.h"
#include "SupportGripToggle.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
ControllerFrame Frame(uint64_t time,float squeeze){ControllerFrame f;f.focused=true;f.timeMs=time;
    for(auto& hand:f.hands){hand.active=hand.grip.valid=hand.grip.tracked=true;}f.hands[0].grip.position[2]=-.2f;f.hands[0].squeeze=squeeze;return f;}
int main(){
    for(int type:{2,3,12,51,52})CHECK(PhysicalMeleeType(type));
    for(int type:{0,4,5,6,7,8,9,15,24,49})CHECK(!PhysicalMeleeType(type));
    CHECK(!TwoHandType(2)&&TwoHandType(3)&&TwoHandType(12)&&TwoHandType(51)&&TwoHandType(52));
    melee::Blade contact;
    CHECK(melee::LocalContact({-1,-3,-1},{1,12,1},100,2,contact)&&contact.tip.y==12);
    CHECK(!melee::LocalContact({-1,-1,-1},{1,2,1},100,2,contact));
    CHECK(melee::LocalContact({-3,-95,-3},{3,115,3},100,12,contact)&&contact.root.y==-95&&contact.tip.y==115);
    CHECK(melee::LocalContact({-3,-30,-3},{3,240,3},100,51,contact));
    CHECK(!melee::LocalContact({-3,-30,-3},{3,400,3},100,51,contact));
    CHECK(!melee::LocalContact({-3,-95,-3},{3,115,3},100,9,contact));
    // Real handle acquisition, including a native attachment 4 cm away
    // from the controller axis and an asymmetric axe head.
    melee::Blade handle;CHECK(melee::HandleSegment({-2,-2,-20},{2,2,140},100,3,handle));
    CHECK(handle.root.x==0&&handle.tip.x==0&&handle.tip.z<20&&handle.root.z>-20);
    CHECK(melee::HandleSegment({-2,-3,-30},{30,3,160},100,52,contact)&&contact.root.x==0&&contact.tip.x==0);
    CHECK(!melee::HandleSegment({-2,-2,-20},{2,2,140},100,2,contact));
    arms::Mat mesh;arms::Position(mesh,{4,120,30});auto worldHandle=melee::WorldSegment(handle,mesh);
    melee::TwoHandGrip realGrip;auto realFrame=Frame(800,0);arms::Vec palm{4,120,20};
    CHECK(!realGrip.Update(42,3,realFrame,1,true,true,{4,120,35},palm,{0,0,1},100,.02f,0,&worldHandle));
    realFrame.timeMs+=10;realFrame.hands[0].squeeze=.8f;
    CHECK(realGrip.Update(42,3,realFrame,1,true,true,{4,120,35},palm,{0,0,1},100,.02f,0,&worldHandle));
    realFrame.timeMs+=10;realFrame.hands[0].squeeze=0;CHECK(!realGrip.Update(42,3,realFrame,1,true,true,{4,120,35},palm,{0,0,1},100,.02f,0,&worldHandle));
    realFrame.timeMs+=10;realFrame.hands[0].squeeze=.8f;
    CHECK(!realGrip.Update(42,3,realFrame,1,true,true,{4,120,35},{0,120,20},{0,0,1},100,.02f,0,&worldHandle));
    melee::SupportAttachment attached;arms::Mat handGrip,calibration;float angles[3]{.4f,-.3f,.2f};
    calibration=arms::Rotation(NativeCameraRotation(angles));arms::Position(handGrip,palm);
    CHECK(attached.Capture(mesh,handle,palm,handGrip));auto localPoint=attached.LocalPoint();
    // Palm contact stays fixed in mesh coordinates through translation,
    // pitch and roll; wrist placement includes actual hand calibration.
    for(int i=0;i<30;++i){float rotation[3]{i*.07f,i*.05f,-i*.08f};auto moved=arms::Rotation(NativeCameraRotation(rotation));arms::Position(moved,{40000.f+i,130.f,-8000.f});
        arms::Mat pose;arms::Vec wrist,anchor;CHECK(attached.Resolve(moved,calibration,{0,-4.7f,0},pose,wrist,anchor));
        auto hand=arms::Multiply(calibration,pose);arms::Position(hand,wrist);
        CHECK(arms::Length(arms::Point(hand,{0,-4.7f,0})-arms::Point(moved,localPoint))<.01f);
        CHECK(arms::Length(anchor-arms::Point(moved,localPoint))<.01f);
    }
    // Same item + rebuilt armor rig: transfer the grip orientation while
    // retaining the identical contact point in weapon-local coordinates.
    arms::Mat oldPose,newPose;arms::Vec oldWrist,newWrist,oldContact,newContact;
    CHECK(attached.Resolve(mesh,calibration,{0,-4.7f,0},oldPose,oldWrist,oldContact));
    float changedAngles[3]{-.3f,.5f,-.1f};auto changedCalibration=arms::Rotation(NativeCameraRotation(changedAngles));
    CHECK(attached.RebindCalibration(calibration,changedCalibration));
    CHECK(attached.Resolve(mesh,changedCalibration,{1,-5,0},newPose,newWrist,newContact));
    auto before=arms::Multiply(calibration,oldPose),after=arms::Multiply(changedCalibration,newPose);
    for(int k:{0,1,2,4,5,6,8,9,10})CHECK(std::fabs(before.m[k]-after.m[k])<.001f);
    CHECK(arms::Length(newContact-oldContact)<.001f);
    attached.Reset();arms::Mat unusedPose;arms::Vec unusedWrist,unusedAnchor;
    CHECK(!attached.Resolve(mesh,calibration,{0,-4.7f,0},unusedPose,unusedWrist,unusedAnchor));
    // Prediction must match the native entity transform used after skinning:
    // original mesh * inverse(original hand) * new calibrated hand.
    arms::Mat nativeHand=calibration;arms::Position(nativeHand,{10,120,30});arms::Mat nativeInverse;
    CHECK(arms::Inverse(nativeHand,nativeInverse));arms::Mat nativeAttachment;arms::Position(nativeAttachment,{4,0,0});
    auto originalMesh=arms::Multiply(nativeAttachment,nativeHand),attachment=arms::Multiply(originalMesh,nativeInverse);
    arms::Mat tracked;arms::Position(tracked,{20,130,40});arms::Mat supportedGrip;
    CHECK(melee::SupportedGripAt(tracked,{0,0,1},{24,130,32},{34,140,55},1,supportedGrip));
    CHECK(arms::Length(arms::Position(supportedGrip)-arms::Position(tracked))<.001f);
    auto newHand=arms::Multiply(calibration,supportedGrip);arms::Position(newHand,arms::Point(supportedGrip,{0,0,-8}));
    auto predicted=arms::Multiply(attachment,newHand),actual=arms::Multiply(originalMesh,arms::Multiply(nativeInverse,newHand));
    for(int i=0;i<16;++i)CHECK(std::fabs(predicted.m[i]-actual.m[i])<.001f);
    auto vertical=Frame(900,.8f);Orientation a,b;
    vertical.hands[0].grip.position[1]=.2f;vertical.hands[0].grip.position[2]=.002f;CHECK(melee::SupportOrientation(vertical,1,a));
    vertical.hands[0].grip.position[2]=-.002f;CHECK(melee::SupportOrientation(vertical,1,b)&&melee::Angle(a,b)<.03f);
    // Coordinated swords/axes: wrist pitch/yaw cannot compete with the
    // positional two-hand axis or move the captured controller offset.
    auto rotationAround=[](arms::Vec axis,float angle){
        return arms::Rotation(Matrix(Orientation{std::cos(angle*.5f),axis.x*std::sin(angle*.5f),axis.y*std::sin(angle*.5f),axis.z*std::sin(angle*.5f)}));};
    // A 4cm controller-centre offset is not the shaft direction. Acquisition
    // keeps the current pose; the actual shaft follows the corrected hand line.
    for(float side:{-1.f,1.f}){
        melee::CoordinatedGrip guided;arms::Mat l,r,held,result;
        arms::Vec shaft{0,0,1};arms::Position(l,{side*4,0,side*12});arms::Position(held,{2,0,-4});
        CHECK(guided.Capture(held,l,r,5000,side,&shaft));CHECK(guided.Step(l,r,5000,result));
        for(int k=0;k<16;++k)CHECK(std::fabs(result.m[k]-held.m[k])<.001f);
        // Arbitrary left wrist turns alone must not steer or roll the sword.
        for(int i=1;i<=30;++i){float angles[3]{i*.08f,-i*.12f,i*.05f};l=arms::Rotation(NativeCameraRotation(angles));arms::Position(l,{side*4,0,side*12});
            CHECK(guided.Step(l,r,5000+i*10,result));for(int k=0;k<16;++k)CHECK(std::fabs(result.m[k]-held.m[k])<.001f);}
        arms::Position(l,{side*10,side*6,side*12});CHECK(guided.Step(l,r,5310,result));
        arms::Vec expected{6,6,std::sqrt(160.f)};CHECK(arms::Unit(expected));
        CHECK(arms::Length(arms::Vector(result,{0,0,1})-expected)<.001f);
        // With a fixed dominant hand, very large support steering cannot
        // turn its wrist backwards. Returning restores exactly the start pose.
        arms::Position(l,{side*30,0,-side*40});CHECK(guided.Step(l,r,5320,result));
        CHECK(std::fabs(arms::Dot(arms::Vector(result,{0,0,1}),shaft)-std::sqrt(.5f))<.001f);
        arms::Position(l,{side*4,0,side*12});CHECK(guided.Step(l,r,5330,result));
        for(int k=0;k<16;++k)CHECK(std::fabs(result.m[k]-held.m[k])<.001f);
        CHECK(guided.Step(l,r,10000,result)); // multi-second loading gap, still tracked
        for(int k=0;k<16;++k)CHECK(std::fabs(result.m[k]-held.m[k])<.001f);
        arms::Position(l,arms::Position(r));CHECK(guided.Step(l,r,10010,result)&&guided.Valid());
        // Apply common player locomotion/turn while reusing raw XR timestamps.
        melee::CoordinatedGrip world,tracking;arms::Mat raw[2];arms::Position(raw[0],{side*.04f,0,side*.12f});
        arms::Mat origin;arms::Position(origin,{40000,0,-8000});auto wl=arms::Multiply(l,origin),wr=origin;
        // Use matching input positions for acquisition and keep the same shaft.
        arms::Position(wl,arms::Point(origin,{side*4,0,side*12}));auto wh=arms::Multiply(held,origin);
        CHECK(melee::CaptureHeldGrip(world,tracking,wh,wl,wr,raw[0],raw[1],6000,side,&shaft));
        for(int i=1;i<=50;++i){auto movement=rotationAround({0,1,0},i*.02f);arms::Position(movement,{40000.f+i*3,0,-8000.f+i});
            wr=movement;wl=movement;arms::Position(wl,arms::Point(movement,{side*4,0,side*12}));
            CHECK(world.Step(wl,wr,6000+i/3*10,result));auto wanted=arms::Multiply(held,movement);
            for(int k=0;k<16;++k)CHECK(std::fabs(result.m[k]-wanted.m[k])<.015f);
            arms::Mat local;CHECK(tracking.Step(raw[0],raw[1],6000+i/3*10,local));
            CHECK(arms::Length(arms::Vector(local,{0,0,1})-shaft)<.001f);}
    }
    melee::CoordinatedGrip common;arms::Mat left,right,initial,commonPose;
    arms::Position(left,{0,0,20});arms::Position(initial,{4,0,-8});
    CHECK(common.Capture(initial,left,right,100,1));
    right=rotationAround({1,0,0},.9f);CHECK(common.Step(left,right,110,commonPose));
    CHECK(arms::Length(arms::Vector(commonPose,{0,0,1})-arms::Vec{0,0,1})<.0001f);
    CHECK(arms::Length(arms::Position(commonPose)-arms::Position(initial))<.0001f);
    Orientation commonQ;CHECK(melee::PoseOrientation(commonPose,commonQ)&&melee::Angle(commonQ,{1,0,0,0})<.001f);
    // Equal roll rotates the common pose once; opposite rolls cancel.
    for(int opposite:{0,1}){left=right=arms::Mat{};arms::Position(left,{0,0,20});CHECK(common.Capture(initial,left,right,200,1));
        for(int i=1;i<=20;++i){float angle=i*.025f;left=rotationAround({0,0,1},angle);right=rotationAround({0,0,1},opposite?-angle:angle);arms::Position(left,{0,0,20});
            CHECK(common.Step(left,right,200+i*10,commonPose));CHECK(melee::PoseOrientation(commonPose,commonQ));
            Orientation expected{std::cos((opposite?0.f:angle)*.5f),0,0,std::sin((opposite?0.f:angle)*.5f)};CHECK(melee::Angle(commonQ,expected)<.001f);}
    }
    // A common swing through vertical and past 180 degrees remains a
    // rigid transform. No world-up reconstruction, pole flip or roll drift.
    left=right=arms::Mat{};arms::Position(left,{0,0,20});CHECK(common.Capture(initial,left,right,500,1));
    for(int i=1;i<=100;++i){auto motion=rotationAround({1,0,0},i*.04f);arms::Position(motion,{400.f+i,30,-100});
        left=arms::Multiply(arms::Mat{},motion);right=left;arms::Position(left,arms::Point(motion,{0,0,20}));
        CHECK(common.Step(left,right,500+i*10,commonPose));auto expected=arms::Multiply(initial,motion);
        for(int k=0;k<16;++k)CHECK(std::fabs(commonPose.m[k]-expected.m[k])<.002f);
        auto duplicate=commonPose;CHECK(common.Step(left,right,500+i*10,commonPose));for(int k=0;k<16;++k)CHECK(commonPose.m[k]==duplicate.m[k]);
    }
    CHECK(common.Step(left,right,1701,commonPose)&&common.Valid()); // Loading is not a detach.
    CHECK(!common.Step(left,right,1600,commonPose)&&!common.Valid()); // A time reset is a new session.
    CHECK(!common.Step(left,right,1711,commonPose));
    // Arbitrary common rotation (not just a vertical plane) must also
    // preserve the weapon frame and attached palm under native turning.
    left=right=arms::Mat{};arms::Position(left,{0,0,20});CHECK(common.Capture(initial,left,right,2000,1));
    arms::Vec oblique{.3f,.4f,.5f};CHECK(arms::Unit(oblique));
    for(int i=1;i<=80;++i){auto motion=rotationAround(oblique,i*.03f);arms::Position(motion,{50,130,70});
        right=motion;left=motion;arms::Position(left,arms::Point(motion,{0,0,20}));CHECK(common.Step(left,right,2000+i*10,commonPose));
        auto expected=arms::Multiply(initial,motion);for(int k=0;k<16;++k)CHECK(std::fabs(commonPose.m[k]-expected.m[k])<.003f);
    }
    // The native game advances between two renders which reuse one XR
    // controller timestamp. Reusing raw tracking must not freeze world pose.
    left=right=arms::Mat{};arms::Position(left,{0,0,20});CHECK(common.Capture(initial,left,right,3000,1));
    for(int i=1;i<=60;++i){auto motion=rotationAround({0,1,0},i*.012f);arms::Position(motion,{i*3.f,0,i*1.5f});
        right=motion;left=motion;arms::Position(left,arms::Point(motion,{0,0,20}));
        auto trackingStamp=uint64_t(3000+(i/3)*10);CHECK(common.Step(left,right,trackingStamp,commonPose));
        auto expected=arms::Multiply(initial,motion);for(int k=0;k<16;++k)CHECK(std::fabs(commonPose.m[k]-expected.m[k])<.003f);
        auto duplicate=commonPose;CHECK(common.Step(left,right,trackingStamp,commonPose));for(int k=0;k<16;++k)CHECK(commonPose.m[k]==duplicate.m[k]);
    }
    CHECK(common.ReusedTrackingWorldUpdates()==40);
    melee::TwoHandGrip grip;auto f=Frame(1000,0);arms::Vec primary{10,120,30},secondary{10,120,50},axis{0,0,1};
    auto update=[&](bool allowed=true,bool freeLeft=true,int type=3,uintptr_t item=42,uint64_t session=1){return grip.Update(item,type,f,session,allowed,freeLeft,primary,secondary,axis,100,.12f,.35f);};
    CHECK(!update());f.timeMs+=10;f.hands[0].squeeze=.8f;CHECK(update()&&grip.sign==1);
    CHECK(update()); // Two stereo eyes cannot toggle a held grip.
    auto translated=arms::Vec{40000,0,-8000};primary=primary+translated;secondary=secondary+translated;f.timeMs+=10;CHECK(update());
    secondary=primary+arms::Vec{20,0,20};arms::Mat dominant;arms::Position(dominant,primary);arms::Mat supported;
    CHECK(melee::SupportedGrip(dominant,axis,secondary,grip.sign,supported));
    auto wanted=secondary-primary;CHECK(arms::Unit(wanted)&&arms::Length(arms::Vector(supported,{0,0,1})-wanted)<.001f);
    CHECK(arms::Length(arms::Position(supported)-primary)<.001f);
    f.timeMs+=10;f.hands[0].squeeze=.45f;CHECK(update()); // Hysteresis.
    f.timeMs+=10;f.hands[0].squeeze=.2f;CHECK(!update());
    secondary=primary+arms::Vec{0,0,-15};f.timeMs+=10;f.hands[0].squeeze=.8f;CHECK(update()&&grip.sign==-1);
    CHECK(melee::SupportedGrip(dominant,axis,secondary,grip.sign,supported)&&arms::Vector(supported,{0,0,1}).z>.99f);
    // Large inter-controller separation alone is valid on a long staff.
    // Release is checked against the actual constrained palm contact instead.
    f.timeMs+=10;secondary=primary+arms::Vec{0,0,130};CHECK(update());
    CHECK(melee::SupportContactInReach({0,0,29.9f},{},100));
    CHECK(!melee::SupportContactInReach({0,0,30.1f},{},100));
    CHECK(!melee::SupportContactInReach({0,0,1},{},0));
    for(float metres:{.10f,.40f,1.f})for(float units:{100.f,125.f}){
        CHECK(melee::SupportContactInReach({(metres-.001f)*units,0,0},{},units,metres));
        CHECK(!melee::SupportContactInReach({(metres+.001f)*units,0,0},{},units,metres));}
    CHECK(!melee::SupportContactInReach({},{},100,NAN)&&!melee::SupportContactInReach({},{},100,0));
    f.timeMs+=10;f.hands[0].squeeze=0;CHECK(!update());f.hands[0].squeeze=.8f;
    for(int reason=0;reason<7;++reason){grip.Reset();f=Frame(2000+reason*1000,0);secondary=primary+arms::Vec{0,0,20};CHECK(!update());
        f.timeMs+=10;f.hands[0].squeeze=.8f;CHECK(update());f.timeMs+=10;
        if(reason==0)f.hands[0].grip.tracked=false;if(reason==1)f.focused=false;if(reason==2)f.timeMs+=101;
        if(reason==3)f.hands[0].grip.position[0]=1;
        if(reason==2||reason==3){CHECK(update());continue;} // Stalls/position discontinuities don't invent release clicks.
        CHECK(!update(reason!=4,reason!=5,reason==6?2:3));
        f=Frame(f.timeMs+10,.8f);CHECK(!update()); // No automatic re-grab after a discontinuity.
    }
    grip.Reset();f=Frame(10000,0);CHECK(!update());f.timeMs+=10;f.hands[0].squeeze=.8f;secondary=primary+arms::Vec{20,0,20};CHECK(!update()); // Away from the handle.
    // Toggle is edge-triggered, requires an initial release and a real contact.
    grip.Reset();f=Frame(20000,.8f);secondary=primary+arms::Vec{0,0,20};
    auto toggle=[&](bool allowed=true){return grip.Update(42,3,f,1,allowed,true,primary,secondary,axis,100,.12f,.35f,nullptr,true);};
    CHECK(!toggle());f.timeMs+=10;f.hands[0].squeeze=0;CHECK(!toggle());
    f.timeMs+=10;f.hands[0].squeeze=.8f;CHECK(toggle()&&toggle());
    f.timeMs+=10;f.hands[0].squeeze=0;CHECK(toggle());
    f.timeMs+=2500;CHECK(toggle()); // Loading with the button released retains the latch.
    f.timeMs+=2500;f.hands[0].squeeze=.8f;CHECK(toggle()); // A hidden edge during loading is not a queued release.
    f.timeMs+=10;f.hands[0].squeeze=0;CHECK(toggle());
    f.timeMs+=10;f.hands[0].squeeze=.8f;CHECK(!toggle()&&!toggle());
    f.timeMs+=10;f.hands[0].squeeze=0;CHECK(!toggle());
    f.timeMs+=10;f.hands[0].squeeze=.8f;secondary=primary+arms::Vec{30,0,20};CHECK(!toggle());
    f.timeMs+=10;secondary=primary+arms::Vec{0,0,20};CHECK(!toggle());
    f.timeMs+=10;f.hands[0].squeeze=0;CHECK(!toggle());f.timeMs+=10;f.hands[0].squeeze=.8f;CHECK(toggle());
    CHECK(!toggle(false));f.timeMs+=10;CHECK(!toggle());
    // The crossbow uses the same click semantics, but can reject a far click
    // after testing its actual support contact. Raw shield input is untouched.
    SupportGripToggle latch;uint64_t tick=30000;
    CHECK(!latch.Step(8,1,tick,true,true,.8f));
    CHECK(!latch.Step(8,1,tick+=10,true,true,0));
    CHECK(latch.Step(8,1,tick+=10,true,true,.8f));latch.Confirm(false);
    CHECK(!latch.Step(8,1,tick+=10,true,true,.8f));
    CHECK(!latch.Step(8,1,tick+=10,true,true,0));CHECK(latch.Step(8,1,tick+=10,true,true,.8f));latch.Confirm(true);
    CHECK(latch.Step(8,1,tick,true,true,.8f)&&latch.Step(8,1,tick+=10,true,true,0));
    CHECK(latch.Step(8,1,tick+=3000,true,true,0));
    CHECK(latch.Step(8,1,tick+=3000,true,true,.8f));
    CHECK(latch.Step(8,1,tick+=10,true,true,0));
    CHECK(!latch.Step(8,1,tick+=10,true,true,.8f));
    CHECK(!latch.Step(8,1,tick+=10,true,true,0));CHECK(latch.Step(8,1,tick+=10,true,true,.8f));
    CHECK(!latch.Step(8,1,tick+=10,false,true,.8f)&&!latch.Step(8,1,tick+=10,true,true,.8f));
    CHECK(!latch.Step(8,1,tick+=10,true,true,0));CHECK(latch.Step(8,1,tick+=10,true,true,.8f));
    CHECK(!latch.Step(9,1,tick+=10,true,true,.8f));
    CHECK(latch.Step(9,1,tick+=10,true,false,.8f)&&!latch.Step(9,1,tick+=10,true,false,0));
    // Left-hand motion alone rotates a supported weapon and can produce a hit.
    // Shared world movement, head movement and mode switches do not.
    melee::Swing swing;uint64_t time=11000;f=Frame(time,.8f);melee::Sample sample;sample.player=1;sample.weapon=42;sample.session=1;sample.valid=sample.twoHand=true;
    sample.hand={0,120,0};sample.blade={{0,120,0},{0,120,120}};
    for(int i=0;i<36;++i){sample.time=time;f.timeMs=time;CHECK(melee::SupportOrientation(f,1,sample.orientation));CHECK(!swing.Step(sample));time+=10;}
    for(int i=0;i<30;++i){sample.time=time;sample.hand.x+=3;sample.blade.root.x+=3;sample.blade.tip.x+=3;CHECK(!swing.Step(sample));time+=10;}
    bool hit=false;for(int i=1;i<=24;++i){sample.time=time;f.timeMs=time;float angle=.025f*i;
        f.hands[0].grip.position[0]=-.2f*std::sin(angle);f.hands[0].grip.position[2]=-.2f*std::cos(angle);
        CHECK(melee::SupportOrientation(f,1,sample.orientation));hit|=swing.Step(sample);time+=10;}
    CHECK(hit&&swing.Hit(7)&&!swing.Hit(7));
    sample.twoHand=false;sample.time=time;sample.orientation={1,0,0,0};CHECK(!swing.Step(sample));
    puts("PASS native weapon families, short melee, standard 2H staff, two-hand grip lifecycle, roll, physical off-hand swing, room-motion rejection and damage deduplication");return 0;
}
