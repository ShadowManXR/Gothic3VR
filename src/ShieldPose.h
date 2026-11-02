#pragma once
#include "TrackedArmsMath.h"
#include "WeaponTypes.h"
#include <cstdint>
#include <cstring>
namespace diagnostic {
inline bool NativeParryCompatible(int right,int left){
    auto offhand=WeaponType(left);
    return PhysicalMeleeType(right)&&offhand!=WeaponKind::OneHand&&offhand!=WeaponKind::Bow&&offhand!=WeaponKind::Crossbow;
}
struct ParryLoadout {
    int right=0,left=0;
    bool Compatible()const{return NativeParryCompatible(right,left);}
    bool LeftShield()const{return WeaponType(right)==WeaponKind::OneHand&&WeaponType(left)==WeaponKind::Shield;}
    unsigned GuardHand()const{return LeftShield()?0:1;}
    bool Pressed(bool rightGrip,bool leftGrip)const{return Compatible()&&(rightGrip||(LeftShield()&&leftGrip));}
};
inline bool NativeParryState(int action,int animation){return (action>=16&&action<=19)||animation==4;}
// Verified native OnAction reads key at 0, duration at 8 and flags at 12.
// Keep native held/wind-up timing. Only suppress the short-release quick attack.
inline bool ParryReleaseMessage(void const* message,unsigned char (&out)[16]){
    auto* bytes=static_cast<unsigned char const*>(message);uint16_t key=0;std::memcpy(&key,bytes,2);
    if(key!=16||(bytes[12]&1)||!(bytes[12]&2))return false;
    std::memset(out,0,16);std::memcpy(out,bytes,13);uint32_t duration;std::memcpy(&duration,bytes+8,4);
    duration=duration<1000?1000:duration;std::memcpy(out+8,&duration,4);return true;
}
namespace arms {
// Retain native shield attachment ROTATION, but anchor the back-centre grip to
// the calibrated palm. Never rotate the hand to accommodate the shield mesh.
class ShieldContact {
    uintptr_t identity=0;Mat orientation;Vec contact;
public:
    void Reset(){identity=0;}
    bool Prepare(uintptr_t key,Mat const& item,Mat const& nativeHand,Vec palm,Vec lo,Vec hi){
        if(!key||!Affine(item)||!Affine(nativeHand)||!Finite(palm)||!Finite(lo)||!Finite(hi)||lo.x>hi.x||lo.y>hi.y||lo.z>hi.z||Length(hi-lo)<.001f){Reset();return false;}
        if(identity==key)return true;Mat inverse,inverseItem;
        if(!Inverse(nativeHand,inverse)||!Inverse(item,inverseItem)){Reset();return false;}
        orientation=Multiply(item,inverse);Position(orientation,{});contact=(lo+hi)*.5f;
        Vec nativePalm=Point(inverseItem,Point(nativeHand,palm));Vec extent=hi-lo;
        // Thickness axis identifies the back of differently oriented shield meshes.
        if(extent.x<=extent.y&&extent.x<=extent.z)contact.x=std::clamp(nativePalm.x,lo.x,hi.x);
        else if(extent.y<=extent.z)contact.y=std::clamp(nativePalm.y,lo.y,hi.y);
        else contact.z=std::clamp(nativePalm.z,lo.z,hi.z);
        identity=key;return true;
    }
    bool Pose(Mat const& hand,Mat const& grip,Vec palm,int const* settings,float units,Mat& moved)const{
        if(!identity||!Affine(hand)||!Affine(grip)||!Finite(palm)||!std::isfinite(units)||units<=0)return false;
        float angles[3]{settings[0]*.01745329252f,settings[1]*.01745329252f,settings[2]*.01745329252f};
        moved=Multiply(Multiply(orientation,Rotation(NativeCameraRotation(angles))),hand);
        Vec offset{settings[5]*.01f*units,-settings[3]*.01f*units,settings[4]*.01f*units};
        Position(moved,Point(hand,palm)+Vector(grip,offset)-Vector(moved,contact));return Affine(moved);
    }
    Vec Contact()const{return contact;}
};
} }
