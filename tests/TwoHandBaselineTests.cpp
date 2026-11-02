#include "TwoHandMelee.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    CHECK(WeaponType(12)==WeaponKind::TwoHand&&WeaponType(51)==WeaponKind::TwoHand);
    CHECK(MeleePoseType(12)==3&&MeleePoseType(51)==3&&MeleePoseType(52)==52&&MeleePoseType(2)==2);
    // Real Blade Staff bounds: the forward, single-ended segment and support
    // handle must match a standard 2H weapon, including its negative-Z reach.
    arms::Vec lo{-3,-3,-166.6f},hi{3,3,95.7f};melee::Blade standard,standardHandle;
    CHECK(melee::LocalContact(lo,hi,100,3,standard)&&melee::HandleSegment(lo,hi,100,3,standardHandle));
    CHECK(standard.tip.z==-166.6f&&standard.root.z<0&&standard.root.z>-30);
    float rotations[3]{.3f,-.4f,.2f};auto attachment=arms::Rotation(NativeCameraRotation(rotations));arms::Position(attachment,{4,2,-5});
    auto calibration=arms::Rotation(NativeCameraRotation(rotations));arms::Mat grip;arms::Position(grip,{20,130,40});
    for(int type:{12,51}){
        melee::Blade contact,handle;CHECK(melee::LocalContact(lo,hi,100,type,contact)&&melee::HandleSegment(lo,hi,100,type,handle));
        CHECK(arms::Length(contact.root-standard.root)<.001f&&arms::Length(contact.tip-standard.tip)<.001f);
        CHECK(arms::Length(handle.root-standardHandle.root)<.001f&&arms::Length(handle.tip-standardHandle.tip)<.001f);
        int angles[3]{};arms::Mat posed;CHECK(melee::SwordGrip(attachment,calibration,contact,grip,angles,posed));
        auto mesh=arms::Multiply(arms::Multiply(attachment,calibration),posed);
        auto direction=arms::Vector(mesh,contact.tip-contact.root);CHECK(arms::Unit(direction));
        CHECK(arms::Length(direction-arms::Vec{0,0,1})<.001f);
        // The same stored 2H calibration is applied without a name-specific
        // reversal or a second set of staff angles.
        int saved[3]{-30,0,3};arms::Mat expected;
        CHECK(melee::SwordGrip(attachment,calibration,standard,grip,saved,expected));
        CHECK(melee::SwordGrip(attachment,calibration,contact,grip,saved,posed));
        for(unsigned i=0;i<16;++i)CHECK(std::fabs(posed.m[i]-expected.m[i])<.001f);
    }
    puts("PASS staff/halberd standard 2H forward pose, actual Blade Staff geometry, shared support handle and stored calibration");return 0;
}
