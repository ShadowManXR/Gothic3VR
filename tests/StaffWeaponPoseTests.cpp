#include "StaffMelee.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
arms::Mat Turn(arms::Vec axis,float angle){return arms::Rotation(Matrix(Orientation{std::cos(angle*.5f),axis.x*std::sin(angle*.5f),axis.y*std::sin(angle*.5f),axis.z*std::sin(angle*.5f)}));}
ControllerFrame Controllers(uint64_t time){ControllerFrame f;f.focused=true;f.timeMs=time;for(auto& h:f.hands){h.active=h.grip.valid=h.grip.tracked=true;}f.hands[0].grip.position[2]=-.4f;return f;}
melee::Sample Sample(uint64_t time,float angle=0){melee::Sample s;s.player=1;s.weapon=2;s.session=3;s.time=time;s.valid=s.twoHand=s.doubleEnded=true;s.hand={0,120,0};
    auto turn=Turn({1,0,0},angle);s.blade={arms::Vector(turn,{0,0,-90})+s.hand,arms::Vector(turn,{0,0,110})+s.hand};
    s.trackingShaftAxis=arms::Vector(turn,{0,0,1});s.orientation={std::cos(angle*.5f),std::sin(angle*.5f),0,0};return s;}
int main(){
    CHECK(WeaponType(12)==WeaponKind::Staff&&WeaponType(51)==WeaponKind::Staff);
    CHECK(WeaponType(3)==WeaponKind::TwoHand&&WeaponType(52)==WeaponKind::TwoHand);
    CHECK(MeleePoseType(12)==12&&MeleePoseType(51)==12&&MeleePoseType(3)==3&&MeleePoseType(52)==52);
    melee::Blade actual,handle;
    for(int type:{12,51}){
        CHECK(melee::LocalContact({-3,-3,-166.6f},{3,3,95.7f},100,type,actual));
        CHECK(actual.root.z==-166.6f&&actual.tip.z==95.7f);
        CHECK(melee::HandleSegment({-3,-3,-166.6f},{3,3,95.7f},100,type,handle));
        CHECK(handle.root.z==-164.6f&&handle.tip.z==93.7f);
    }
    auto attachment=Turn({1,0,0},.6f),calibration=Turn({0,1,0},-.4f),grip=Turn({0,0,1},.3f);
    arms::Position(attachment,{4,-2,1});arms::Position(grip,{20,130,40});
    auto originalGrip=grip;arms::Vec palmLocal{2,-4.7f,3},anchor;
    CHECK(melee::StaffAnchor(attachment,actual,palmLocal,anchor));
    auto hand=arms::Multiply(calibration,grip);arms::Position(hand,arms::Point(grip,{0,0,-8}));
    auto palm=arms::Point(hand,palmLocal);auto originalHand=hand;
    // Asymmetric Blade Staff geometry, arbitrary native attachment rotation,
    // and endpoint order must not turn the palm upside down.
    for(auto ends:{actual,melee::Blade{actual.tip,actual.root}}){
        int angles[3]{};arms::Mat mesh;
        CHECK(melee::StaffWeaponPose(ends,anchor,grip,palm,angles,mesh));
        auto longAxis=arms::Vector(mesh,actual.root-actual.tip);CHECK(arms::Unit(longAxis));
        CHECK(arms::Length(longAxis-arms::Vector(grip,{0,0,1}))<.001f);
        CHECK(arms::Length(arms::Point(mesh,anchor)-palm)<.001f);
        CHECK(!memcmp(&grip,&originalGrip,sizeof(grip))&&!memcmp(&hand,&originalHand,sizeof(hand)));
        // Weapon calibration changes only the mesh; its right palm pivot stays.
        int adjusted[3]{30,-20,60};auto first=mesh;
        CHECK(melee::StaffWeaponPose(ends,anchor,grip,palm,adjusted,mesh));
        CHECK(arms::Length(arms::Point(mesh,anchor)-palm)<.001f);
        CHECK(arms::Length(arms::Vector(mesh,{0,0,1})-arms::Vector(first,{0,0,1}))>.1f);
        CHECK(!memcmp(&grip,&originalGrip,sizeof(grip))&&!memcmp(&hand,&originalHand,sizeof(hand)));
    }
    // Contact displacement is in finger space for any calibrated controller,
    // preserving the accepted weapon direction and the right hand transform.
    for(int i=0;i<30;++i){auto rotated=Turn({0,0,1},i*.12f);arms::Position(rotated,{40000.f+i,130,-8000});auto saved=rotated;
        auto wrist=arms::Point(rotated,{0,0,-8});arms::Vec zero,advanced;
        CHECK(melee::StaffPalmContact(rotated,calibration,wrist,palmLocal,100,0,zero));
        CHECK(melee::StaffPalmContact(rotated,calibration,wrist,palmLocal,100,4,advanced));
        CHECK(arms::Length((advanced-zero)-arms::Vector(rotated,{0,-4,0}))<.01f);
        int angles[3]{};arms::Mat first,moved;
        CHECK(melee::StaffWeaponPose(actual,anchor,rotated,zero,angles,first));
        CHECK(melee::StaffWeaponPose(actual,anchor,rotated,advanced,angles,moved));
        CHECK(arms::Length(arms::Point(moved,anchor)-advanced)<.01f);
        for(int k:{0,1,2,4,5,6,8,9,10})CHECK(first.m[k]==moved.m[k]);
        CHECK(!memcmp(&rotated,&saved,sizeof(rotated)));
    }
    auto shiftedPalm=arms::Point(attachment,{0,0,-150});arms::Vec shifted;
    CHECK(melee::StaffAnchor(attachment,actual,shiftedPalm,shifted));
    int angles[3]{};arms::Mat mesh;CHECK(melee::StaffWeaponPose(actual,shifted,grip,palm,angles,mesh));
    auto longAxis=arms::Vector(mesh,actual.tip-actual.root);CHECK(arms::Unit(longAxis));
    CHECK(arms::Length(longAxis-arms::Vector(grip,{0,0,1}))<.001f);
    // Coordinated weapon motion, including repeated XR time with a changing
    // native origin, keeps both palm contacts. The right grip is never rotated.
    arms::Mat left,right,initial,supported;arms::Position(left,{0,0,-40});
    arms::Vec fixedPalm{2,-4.7f,-5};
    CHECK(melee::StaffWeaponPose(actual,anchor,right,fixedPalm,angles,initial));
    melee::CoordinatedGrip shared;CHECK(shared.Capture(initial,left,right,4000,1));
    melee::SupportAttachment support;CHECK(support.Capture(initial,handle,{2,-4.7f,-40},left));
    for(int i=1;i<=60;++i){auto motion=Turn({1,0,0},i*.03f);arms::Position(motion,{40000.f+i*3,130,-8000.f+i});
        right=motion;left=motion;arms::Position(left,arms::Point(motion,{0,0,-40}));auto beforeRight=right;
        CHECK(shared.Step(left,right,4000+(i/3)*10,supported));
        auto wanted=arms::Point(motion,fixedPalm);CHECK(melee::AnchorStaffWeapon(supported,anchor,wanted));
        CHECK(arms::Length(arms::Point(supported,anchor)-wanted)<.01f);
        CHECK(!memcmp(&right,&beforeRight,sizeof(right)));
        auto expected=arms::Multiply(initial,motion);
        for(int k:{0,1,2,4,5,6,8,9,10})CHECK(std::fabs(supported.m[k]-expected.m[k])<.002f);
        arms::Mat supportGrip;arms::Vec wrist,contact;
        CHECK(support.Resolve(supported,calibration,palmLocal,supportGrip,wrist,contact));
        auto leftHand=arms::Multiply(calibration,supportGrip);arms::Position(leftHand,wrist);
        CHECK(arms::Length(arms::Point(leftHand,palmLocal)-contact)<.01f);
    }
    CHECK(shared.ReusedTrackingWorldUpdates()==40);
    // Shaft center must agree between primary anchoring and support capture,
    // even when a halberd head widens the bounds to one side of the shaft.
    melee::Blade wide,wideHandle;
    CHECK(melee::LocalContact({-2,-3,-166},{30,12,95},100,51,wide));
    CHECK(melee::HandleSegment({-2,-3,-166},{30,12,95},100,51,wideHandle));
    CHECK(wide.root.x==wideHandle.root.x&&wide.root.y==wideHandle.root.y&&wide.root.x==0&&wide.root.y==0);
    // Right palm faces -X, left palm +X. Offsets must use the hand frame,
    // not weapon axes, and the attached hand must resolve that same contact.
    for(unsigned side:{0u,1u}){arms::Vec contactLocal;
        CHECK(melee::StaffContactLocal(calibration,palmLocal,side,4,2,100,contactLocal));
        for(int i=0;i<30;++i){auto pose=Turn({0,0,1},i*.07f);arms::Position(pose,{40000.f+i,130,-8000});
            auto before=pose;auto wrist=arms::Point(pose,{0,0,-8});arms::Vec contact,plain;
            CHECK(melee::StaffPalmContact(pose,calibration,wrist,palmLocal,100,4,contact,2,side));
            CHECK(melee::StaffPalmContact(pose,calibration,wrist,palmLocal,100,0,plain,0,side));
            auto offset=arms::Vector(pose,{side==1?-2.f:2.f,-4,0});CHECK(arms::Length(contact-plain-offset)<.01f);
            auto nativeHand=arms::Multiply(calibration,pose);arms::Position(nativeHand,wrist);
            CHECK(arms::Length(arms::Point(nativeHand,contactLocal)-contact)<.01f&&!memcmp(&pose,&before,sizeof(pose)));
        }
    }
    arms::Vec supportLocal;CHECK(melee::StaffContactLocal(calibration,palmLocal,0,4,2,100,supportLocal));
    for(int i=0;i<30;++i){auto moved=Turn({1,0,0},i*.07f);arms::Position(moved,{40000.f+i,130,-8000});
        arms::Mat supportGrip;arms::Vec wrist,contact;
        CHECK(support.Resolve(moved,calibration,supportLocal,supportGrip,wrist,contact));
        auto nativeHand=arms::Multiply(calibration,supportGrip);arms::Position(nativeHand,wrist);
        CHECK(arms::Length(arms::Point(nativeHand,supportLocal)-contact)<.01f);
        // A wrist/raw-palm attachment is observably different from the grip
        // contact; this regression fails if the offset is omitted on the left.
        CHECK(arms::Length(arms::Point(nativeHand,palmLocal)-contact)>4.f);
    }
    // Contact reaches both tips; damage still uses measured physical energy.
    auto before=Sample(1000,0),after=Sample(1010,.2f);auto sweep=melee::BladeSweep(before.blade,after.blade,100);
    bool root=false,tip=false;for(unsigned i=0;i<sweep.count;++i){auto const& segment=sweep.segments[i];
        root|=arms::Length(segment.a-before.blade.root)<.001f&&arms::Length(segment.b-after.blade.root)<.001f;
        tip|=arms::Length(segment.a-before.blade.tip)<.001f&&arms::Length(segment.b-after.blade.tip)<.001f;}
    CHECK(root&&tip);
    auto frame=Controllers(1000);arms::Vec tracking;
    for(int i=0;i<40;++i){auto motion=Turn({0,1,0},i*.04f);arms::Position(motion,{40000.f+i,120,-8000});
        auto axis=arms::Vector(motion,{0,0,-1});CHECK(melee::TrackingShaftAxis(frame,motion,axis,tracking)&&arms::Length(tracking-arms::Vec{0,0,1})<.001f);}
    melee::Swing swing;uint64_t time=2000;
    for(int i=0;i<36;++i){CHECK(!swing.Step(Sample(time)));time+=10;}
    for(int i=1;i<=70;++i){auto s=Sample(time);s.orientation={std::cos(i*.015f),0,0,std::sin(i*.015f)};CHECK(!swing.Step(s));time+=10;}
    bool hit=false;for(int i=1;i<=30;++i){hit|=swing.Step(Sample(time,i*.025f));time+=10;}
    CHECK(hit&&swing.Hit(7)&&!swing.Hit(7));CHECK(melee::ScaledDamage(120,80,swing.intensity)<=120);CHECK(melee::ScaledDamage(120,80,1)==120);
    swing.Reset();for(int i=0;i<36;++i){CHECK(!swing.Step(Sample(time)));time+=10;}
    for(int i=0;i<50;++i){auto s=Sample(time);s.hand.x+=i*3;s.blade.root.x+=i*3;s.blade.tip.x+=i*3;CHECK(!swing.Step(s));time+=10;}
    CHECK(!melee::StaffWeaponPose(actual,anchor,grip,palm,nullptr,mesh));
    puts("PASS Staff/Halberd shaft pivot, mirrored palm depth, corrected support contact, independent pose and physical both-end combat");return 0;
}
