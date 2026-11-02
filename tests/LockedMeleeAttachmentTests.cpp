#include "LockedMeleeAttachment.h"
#include "DualWield.h"
#include "StaffMelee.h"
#include "TwoHandWeight.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
bool Near(arms::Mat const& a,arms::Mat const& b,float epsilon=.025f){for(int i=0;i<16;++i)if(std::fabs(a.m[i]-b.m[i])>epsilon)return false;return true;}
arms::Mat Hand(arms::Mat const& calibration,arms::Mat const& grip){return arms::HandAtPalm(grip,calibration,{0,-4.7f,1});}
arms::Mat Rotation(float x,float y,float z){float angles[3]{x,y,z};return arms::Rotation(NativeCameraRotation(angles));}
bool Relative(arms::Mat const& mesh,arms::Mat const& hand,arms::Mat& relative){
    // Measure the small local offset before multiplication; inverting absolute
    // 40,000cm translations creates unrelated float cancellation in the test.
    auto a=mesh,b=hand;arms::Position(a,{});arms::Position(b,{});arms::Mat inverse;
    if(!arms::Inverse(b,inverse))return false;relative=arms::Multiply(a,inverse);
    arms::Position(relative,arms::Vector(inverse,arms::Position(mesh)-arms::Position(hand)));return arms::Affine(relative);
}
int main(){
    // Rebuilding the armor skeleton must preserve mesh-to-controller pose,
    // including the cached staff profile, across mirrored/scaled bind frames.
    for(int type:{2,3,12,51,52})for(unsigned side=0;side<2;++side){
        melee::LockedMeleeAttachment lock;
        auto oldCal=Rotation(.3f,-.4f,.2f),newCal=Rotation(-.2f,.7f,-.3f);
        for(int k:{0,1,2,4,5,6,8,9,10})newCal.m[k]*=1.08f;
        arms::Vec oldPalm{side?1.f:-1.f,-4.7f,.3f},newPalm{side?2.f:-2.f,-5.2f,-.4f};
        auto attachment=Rotation(.7f,.1f,-.4f);arms::Position(attachment,{3,-1,2});
        CHECK(lock.Capture(32,type,attachment,arms::Mat{}));
        auto profile=Rotation(-.1f,-.2f,.3f);arms::Position(profile,{2,1,4});
        CHECK(lock.LockProfile(profile,arms::Mat{}));
        CHECK(lock.RebindHand(oldCal,oldPalm,newCal,newPalm));
        melee::Blade blade{{0,0,0},{0,120,0}};int angles[3]{-15,5,8};
        for(int i=0;i<90;++i){auto grip=Rotation(i*.03f,-i*.02f,i*.05f);arms::Position(grip,{20,130,30});
            auto before=arms::HandAtPalm(grip,oldCal,oldPalm),after=arms::HandAtPalm(grip,newCal,newPalm);
            CHECK(Near(arms::Multiply(attachment,before),arms::Multiply(lock.Attachment(),after),.001f));
            arms::Mat staff;CHECK(lock.ProfileAtHand(after,staff)&&Near(staff,arms::Multiply(profile,before),.001f));}
        melee::OneHandPose original,changed;arms::Mat controller;
        CHECK(original.PrepareAttachment(attachment,oldCal,blade,controller,angles,100,oldPalm));
        CHECK(changed.PrepareAttachment(lock.Attachment(),newCal,blade,controller,angles,100,newPalm));
        CHECK(Near(original.mesh,changed.mesh,.001f));
        lock.Reset();CHECK(!lock.RebindHand(oldCal,oldPalm,newCal,newPalm));
    }
    auto calibration=Rotation(.2f,-.3f,.1f),nativeAttachment=Rotation(.7f,.4f,-.6f);arms::Position(nativeAttachment,{2,3,-4});
    auto nativeHand=Rotation(.8f,-1.2f,.5f);arms::Position(nativeHand,{20,120,30});auto nativeMesh=arms::Multiply(nativeAttachment,nativeHand);
    int angles[3]{-15,5,8};arms::Vec palm{0,-4.7f,1};
    // Dramatic native idle/draw/carry changes cannot change either the mesh
    // contact or orientation relative to the actual rendered hand.
    for(int type:{2,3,52,12,51})for(unsigned side=0;side<2;++side){
        melee::LockedMeleeAttachment locked;uintptr_t key=100+type*2+side;
        CHECK(locked.Capture(key,type,nativeMesh,nativeHand));auto captured=locked.Attachment();
        melee::Blade blade;CHECK(melee::LocalContact({-3,-30,-3},{3,140,3},100,type,blade));
        bool staff=WeaponType(type)==WeaponKind::Staff;
        arms::Mat initialGrip;arms::Position(initialGrip,{side?25.f:-25.f,135,40});arms::Mat mesh,hand,reference;
        if(staff){auto contact=arms::Point(Hand(calibration,initialGrip),palm);
            CHECK(melee::StaffWeaponPose(blade,{0,0,0},initialGrip,contact,angles,mesh));hand=Hand(calibration,initialGrip);
            CHECK(locked.LockProfile(mesh,hand));
        }else {melee::OneHandPose prepared;CHECK(prepared.PrepareAttachment(captured,calibration,blade,initialGrip,angles,100,palm));mesh=prepared.mesh;hand=Hand(calibration,prepared.grip);}
        CHECK(Relative(mesh,hand,reference));
        for(int i=0;i<240;++i){auto animatedHand=Rotation(i*.11f,-i*.07f,i*.09f),animatedMesh=Rotation(-i*.03f,i*.15f,-i*.05f);
            arms::Position(animatedHand,{100.f+i,70.f-i,100});arms::Position(animatedMesh,{-30.f-i,20.f+i,60});
            CHECK(locked.Capture(key,type,animatedMesh,animatedHand)&&Near(locked.Attachment(),captured,.00001f));
            auto grip=Rotation(i*.055f,i*.037f,-i*.049f);arms::Position(grip,{40000.f+i,100.f+std::sin(i*.2f)*40,-8000.f-i});
            if(staff){hand=Hand(calibration,grip);CHECK(locked.ProfileAtHand(hand,mesh));}
            else {melee::OneHandPose prepared;CHECK(prepared.PrepareAttachment(locked.Attachment(),calibration,blade,grip,angles,100,palm));mesh=prepared.mesh;hand=Hand(calibration,prepared.grip);}
            arms::Mat relative;CHECK(Relative(mesh,hand,relative)&&Near(relative,reference));
        }
        locked.Reset();CHECK(!locked.Matches(key,type)&&!locked.ProfileReady());
        CHECK(locked.Capture(key+1,type,arms::Mat{},arms::Mat{})&&Near(locked.Attachment(),arms::Mat{}));
    }
    puts("PASS 1H/2H/axe/staff/halberd contacts and orientations stay fixed in both palms through 240 wide swings, native animation changes and world movement");
    // Weight is applied to the mesh first; the resolved grip must reproduce
    // that exact mesh through normal skinning and keep the real palm contact.
    melee::OneHandPose heavy;arms::Mat controller;arms::Position(controller,{70,150,60});
    CHECK(heavy.PrepareAttachment(nativeAttachment,calibration,{{0,0,0},{0,120,0}},controller,angles,100,palm));
    auto originalHand=Hand(calibration,heavy.grip);auto originalPalm=arms::Point(originalHand,palm);arms::Mat reference;
    CHECK(Relative(heavy.mesh,originalHand,reference));
    for(int i=0;i<160;++i){auto weighted=Rotation(i*.025f,-i*.04f,i*.013f);arms::Position(weighted,arms::Position(controller));
        arms::Mat weightedMesh,resolved;CHECK(melee::WeightedMesh(heavy.mesh,controller,weighted,originalPalm,weightedMesh));
        CHECK(melee::GripFromMesh(heavy.grip,heavy.mesh,weightedMesh,resolved));auto visualHand=Hand(calibration,resolved);
        arms::Mat relative;CHECK(Relative(weightedMesh,visualHand,relative)&&Near(relative,reference));
        CHECK(arms::Length(arms::Point(visualHand,palm)-originalPalm)<.001f);
        auto skin=heavy;skin.grip=resolved;CHECK(Near(skin.AtWrist(arms::WristAtPalm(resolved,calibration,palm)),weightedMesh,.001f));
    }
    puts("PASS unsupported 2H weight rotates the whole hand/weapon assembly without sliding at the palm or changing the mesh-to-hand transform");
    // Regression: calibrated/weighted weapon axis differs from the raw hand
    // centre line. Acquiring support must not align those axes (the45deg snap).
    for(int type:{3,52})for(float side:{-1.f,1.f}){
        arms::Mat raw[2];arms::Position(raw[1],{.2f,1.3f,-.3f});arms::Position(raw[0],{.2f+side*.09f,1.3f,-.3f+side*.09f});
        auto origin=Rotation(0,.8f,0);arms::Position(origin,{40000,0,-8000});
        auto worldGrip=[&](arms::Mat const& input,arms::Mat const& frame){auto result=arms::Multiply(input,frame);
            arms::Position(result,arms::Point(frame,arms::Position(input)*100));return result;};
        auto worldRight=worldGrip(raw[1],origin),worldLeft=worldGrip(raw[0],origin);
        melee::OneHandPose acquired;CHECK(acquired.PrepareAttachment(nativeAttachment,calibration,{{0,0,0},{0,120,0}},worldRight,angles,100,palm));
        auto weighted=arms::Multiply(Rotation(0,-.45f,.2f),worldRight);arms::Position(weighted,arms::Position(worldRight));
        auto beforeHand=Hand(calibration,acquired.grip);auto primaryPalm=arms::Point(beforeHand,palm);
        arms::Mat beforeMesh,heldGrip;CHECK(melee::WeightedMesh(acquired.mesh,worldRight,weighted,primaryPalm,beforeMesh));
        CHECK(melee::GripFromMesh(acquired.grip,acquired.mesh,beforeMesh,heldGrip));
        auto renderedHand=Hand(calibration,heldGrip);arms::Mat heldRelative;CHECK(Relative(beforeMesh,renderedHand,heldRelative));
        melee::SupportAttachment leftContact;melee::Blade hilt{{0,-30,0},{0,10,0}};
        CHECK(leftContact.Capture(beforeMesh,hilt,arms::Position(worldLeft),worldLeft));
        auto shaftAxis=arms::Vector(beforeMesh,{0,120,0});
        melee::CoordinatedGrip world,tracking;CHECK(melee::CaptureHeldGrip(world,tracking,heldGrip,worldLeft,worldRight,raw[0],raw[1],8000,side,&shaftAxis));
        arms::Mat worldPose,trackingPose;CHECK(world.Step(worldLeft,worldRight,8000,worldPose)&&Near(worldPose,heldGrip,.00001f));
        auto skin=acquired;skin.grip=worldPose;auto shown=skin.AtWrist(arms::WristAtPalm(worldPose,calibration,palm));CHECK(Near(shown,beforeMesh,.008f));
        CHECK(tracking.Step(raw[0],raw[1],8000,trackingPose));arms::Mat initialLeftRelative;bool leftBound=false;
        for(int i=0;i<60;++i){
            auto move=Rotation(i*.014f,i*.017f,-i*.011f);arms::Position(move,{i*.003f,0,0});
            auto r=arms::Multiply(raw[1],move),l=arms::Multiply(raw[0],move);
            auto worldOrigin=Rotation(0,.8f+i*.006f,0);arms::Position(worldOrigin,{40000.f+i*2.f,0,-8000});
            worldRight=worldGrip(r,worldOrigin);worldLeft=worldGrip(l,worldOrigin);uint64_t stamp=8000+(i/3)*10;
            CHECK(world.Step(worldLeft,worldRight,stamp,worldPose)&&tracking.Step(l,r,stamp,trackingPose));
            auto expectedRotation=arms::Multiply(trackingPose,worldOrigin);arms::Position(expectedRotation,{});auto actualRotation=worldPose;arms::Position(actualRotation,{});
            CHECK(Near(actualRotation,expectedRotation,.003f)); //rendered/combat orientations agree despite native movement between XR polls
            skin.grip=worldPose;shown=skin.AtWrist(arms::WristAtPalm(worldPose,calibration,palm));auto visibleHand=Hand(calibration,worldPose);arms::Mat relative;
            CHECK(Relative(shown,visibleHand,relative)&&Near(relative,heldRelative,.01f));
            arms::Mat leftGrip;arms::Vec wrist,contact;CHECK(leftContact.Resolve(shown,calibration,palm,leftGrip,wrist,contact));
            auto visibleLeft=arms::Multiply(calibration,leftGrip);arms::Position(visibleLeft,wrist);arms::Mat leftRelative;CHECK(Relative(visibleLeft,shown,leftRelative));
            if(!leftBound){initialLeftRelative=leftRelative;leftBound=true;}
            CHECK(Near(leftRelative,initialLeftRelative,.01f));
            CHECK(arms::Length(arms::Point(visibleLeft,palm)-contact)<.01f);
        }
        CHECK(world.ReusedTrackingWorldUpdates()>0);
    }
    puts("PASS 2H/axe acquire retains current weighted mesh and both locked palms; oblique45deg hand line cannot snap weapon; tracking/world motion stays coherent");
    melee::CoordinatedGrip badWorld,badTracking;arms::Mat badGrip;badGrip.m[0]=NAN;
    CHECK(!melee::CaptureHeldGrip(badWorld,badTracking,badGrip,controller,controller,controller,controller,9000,1)&&!badWorld.Valid()&&!badTracking.Valid());
    // Staff support produces one weapon orientation even when the two
    // controllers twist differently. Both visible palms remain bound to it.
    melee::Blade shaft{{0,-80,0},{0,130,0}},handle{{0,-60,0},{0,90,0}};
    arms::Mat right,left,staffMesh;arms::Position(right,{20,130,30});int staffAngles[3]{};
    auto rightHand=Hand(calibration,right);auto contact=arms::Point(rightHand,palm);
    CHECK(melee::StaffWeaponPose(shaft,{0,0,0},right,contact,staffAngles,staffMesh));
    auto axis=arms::Vector(staffMesh,{0,1,0});CHECK(arms::Unit(axis));arms::Position(left,arms::Position(right)+axis*30);
    melee::LockedMeleeAttachment staffLock;CHECK(staffLock.Capture(77,12,nativeMesh,nativeHand)&&staffLock.LockProfile(staffMesh,rightHand));
    CHECK(Relative(staffMesh,rightHand,reference));
    melee::SupportAttachment support;CHECK(support.Capture(staffMesh,handle,arms::Position(left),left));
    melee::CoordinatedGrip common;CHECK(common.Capture(staffMesh,left,right,1000,1));
    arms::Mat leftRelative;bool capturedLeft=false;
    for(int i=0;i<120;++i){auto motion=Rotation(i*.031f,i*.043f,-i*.027f);arms::Position(motion,{100.f+i,20,50});
        auto rawRight=arms::Multiply(right,motion),rawLeft=arms::Multiply(left,motion),twist=Rotation(0,0,i*.035f);
        auto origin=arms::Position(rawRight);rawRight=arms::Multiply(twist,rawRight);arms::Position(rawRight,origin);
        arms::Mat supported;CHECK(common.Step(rawLeft,rawRight,1000+i*10,supported));
        auto rawHand=Hand(calibration,rawRight);arms::Mat referenceMesh;CHECK(staffLock.ProfileAtHand(rawHand,referenceMesh));
        CHECK(melee::AnchorStaffWeapon(supported,{0,0,0},arms::Point(rawHand,palm)));
        arms::Mat resolved;CHECK(melee::GripFromMesh(rawRight,referenceMesh,supported,resolved));
        auto visualHand=Hand(calibration,resolved);arms::Mat relative;
        CHECK(Relative(supported,visualHand,relative)&&Near(relative,reference,.003f));
        CHECK(arms::Length(arms::Point(visualHand,palm)-arms::Point(supported,{0,0,0}))<.003f);
        arms::Mat leftGrip;arms::Vec wrist,palmContact;CHECK(support.Resolve(supported,calibration,palm,leftGrip,wrist,palmContact));
        auto visualLeft=arms::Multiply(calibration,leftGrip);arms::Position(visualLeft,wrist);arms::Mat inverse;
        CHECK(arms::Inverse(supported,inverse));auto binding=arms::Multiply(visualLeft,inverse);
        if(!capturedLeft){leftRelative=binding;capturedLeft=true;}CHECK(Near(binding,leftRelative,.003f));
        CHECK(arms::Length(arms::Point(visualLeft,palm)-palmContact)<.003f);
    }
    puts("PASS coordinated staff with different wrist rotations locks both palm contacts and orientations through large swings");
    melee::LockedMeleeAttachment a,b;CHECK(a.Capture(1,2,nativeMesh,nativeHand)&&b.Capture(2,2,arms::Mat{},arms::Mat{}));
    auto untouched=a.Attachment();CHECK(b.Capture(3,2,nativeHand,nativeMesh)&&Near(a.Attachment(),untouched));
    CHECK(!a.Capture(1,6,nativeMesh,nativeHand));CHECK(a.Capture(1,3,arms::Mat{},arms::Mat{})&&Near(a.Attachment(),arms::Mat{}));
    auto bad=arms::Mat{};bad.m[0]=NAN;arms::Mat result;CHECK(!melee::GripFromMesh(bad,nativeMesh,nativeMesh,result));
    puts("PASS independent dual-wield bindings, equipment/type replacement, reset and invalid transform refusal");return 0;
}
