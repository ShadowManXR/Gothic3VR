#include "PhysicalCrossbow.h"
#include "RangedRenderRecovery.h"
#include "TrackedPalette.h"
#include "FrozenArmPalette.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    crossbow::FrameSupport support;arms::Mat mesh,grip,pose;arms::Vec wrist;arms::Position(mesh,{100,140,100});arms::Vec contact{0,0,30};arms::Position(grip,{100,140,138});
    support.Begin(2,true);CHECK(support.Prepare(2,true,mesh,contact,grip,arms::Mat{},arms::Vec{},100));CHECK(support.Holding(2));
    CHECK(support.Read(pose,wrist));auto expected=wrist;TrackedPalette palette;arms::Mat buffer[1];auto native=buffer[0];
    // Repeated meshes and both eyes must keep the same left contact even on
    // a native matrix-cache hit. Restoration cannot leak into the next draw.
    for(unsigned eye=0;eye<2;++eye)for(unsigned pass=0;pass<5;++pass){CHECK(ranged::ApplyTrackedFrame(true,false,true));CHECK(palette.Begin(buffer,1));CHECK(support.Read(pose,wrist));CHECK(arms::Length(wrist-expected)<.0001f);buffer[0]=pose;CHECK(palette.Restore(buffer,1));CHECK(!memcmp(&buffer[0],&native,sizeof(native)));}
    CHECK(!ranged::ApplyTrackedFrame(true,false,false)&&!ranged::ApplyTrackedFrame(false,true,true));
    support.Begin(2,true);CHECK(support.Holding(2)&&!support.Read(pose,wrist));arms::Position(mesh,{110,140,100});CHECK(support.Prepare(2,true,mesh,contact,grip,arms::Mat{},arms::Vec{},100));CHECK(support.Read(pose,wrist)&&std::fabs(wrist.x-expected.x-10)<.0001f);
    support.Begin(2,false);CHECK(!support.Holding(2)&&!support.Read(pose,wrist));
    {
        crossbow::FrameSupport attached;FrozenArmPalette frozen;
        arms::Mat weapon,controller;arms::Vec localContact{0,0,30},palmLocal{1,5,-2};
        float calibrationAngles[3]{.25f,-.4f,.2f};auto calibration=arms::Rotation(NativeCameraRotation(calibrationAngles));
        arms::Vec supportPalm;CHECK(melee::SupportFistContact(calibration,palmLocal,0,100,supportPalm));
        arms::Position(weapon,{100,140,100});arms::Position(controller,arms::Point(weapon,localContact));
        attached.Begin(9,true);CHECK(attached.Prepare(9,true,weapon,localContact,controller,calibration,palmLocal,100));
        // Within the physical release allowance, controller rotation/separation
        // cannot pull the visual palm off the foregrip. Solve it BEFORE freezing;
        // both eyes and repeated native mesh-cache hits consume that one pose.
        for(unsigned frame=0;frame<200;++frame){
            float angle[3]{frame*.013f,frame*.021f,frame*.017f};weapon=arms::Rotation(NativeCameraRotation(angle));
            arms::Position(weapon,{100+frame*.1f,140+std::sin(frame*.05f)*20,100});
            controller=arms::Rotation(NativeCameraRotation(calibrationAngles));
            arms::Position(controller,arms::Point(weapon,localContact)+arms::Vec{10,5,0}); // within the physical contact allowance
            attached.Begin(9,true);CHECK(!attached.Read(pose,wrist));
            CHECK(attached.Prepare(9,true,weapon,localContact,controller,calibration,palmLocal,100));
            CHECK(attached.Read(pose,wrist)&&attached.Holding(9));
            auto hand=arms::Multiply(calibration,pose);arms::Position(hand,wrist);
            auto foregrip=arms::Point(weapon,localContact);
            // Independent surface check: closed-hand knuckles plus the 2 cm
            // palm-pad normal reach the foregrip. The wrist/open tracking pivot
            // must stay behind it, not be mistaken for the visible grip point.
            auto palmPad=arms::Point(hand,palmLocal*2)+arms::Vector(pose,{2,0,0});
            CHECK(arms::Length(palmPad-foregrip)<.001f);
            CHECK(arms::Length(wrist-foregrip)>8&&arms::Length(arms::Point(hand,palmLocal)-foregrip)>5);
            frozen.Reset();CHECK(frozen.Capture(1,2,&hand,1));
            for(unsigned eye=0;eye<2;++eye)for(unsigned pass=0;pass<4;++pass){
                arms::Mat nativeHand=arms::HandAtPalm(controller,calibration,palmLocal),borrowed=nativeHand;TrackedPalette restore;
                CHECK(restore.Begin(&borrowed,1)&&frozen.Apply(1,2,&borrowed,1));
                CHECK(arms::Length(arms::Point(borrowed,supportPalm)-arms::Point(weapon,localContact))<.001f);
                CHECK(restore.Restore(&borrowed,1)&&!memcmp(&borrowed,&nativeHand,sizeof(nativeHand)));
            }
        }
        arms::Position(controller,arms::Point(weapon,localContact)+arms::Vec{100,0,0});
        CHECK(!attached.Prepare(9,true,weapon,localContact,controller,calibration,palmLocal,100));
        CHECK(!attached.Holding(9)&&!attached.Read(pose,wrist));
        attached.Begin(9,false);frozen.Reset();CHECK(!attached.Holding(9)&&!attached.Read(pose,wrist));
        CHECK(!attached.Prepare(10,true,weapon,localContact,controller,calibration,palmLocal,100)); // a new weapon needs a nearby grab
    }
    CHECK(ranged::KeepCrossbowNock(true,true,true,true,true));CHECK(!ranged::KeepCrossbowNock(false,true,true,true,true)&&!ranged::KeepCrossbowNock(true,false,true,true,true)&&!ranged::KeepCrossbowNock(true,true,false,true,true)&&!ranged::KeepCrossbowNock(true,true,true,false,true));
    puts("PASS cached crossbow render override, attached-palm frozen stereo rig despite controller separation, restoration, release and owned held-bolt retention gates");return 0;
}
