#pragma once
#include "TwoHandMelee.h"
namespace diagnostic { namespace melee {
// Resolve the held shaft point in mesh space once for the equipped weapon.
// The native attachment can animate, so it must not drive the VR wrist pose.
inline bool StaffAnchor(arms::Mat const& attachment,Blade const& shaft,Vec nativePalm,Vec& out){
    arms::Mat inverse;
    if(!arms::Finite(shaft.root)||!arms::Finite(shaft.tip)||!arms::Finite(nativePalm)||!arms::Inverse(attachment,inverse))return false;
    out=ClosestPoint(shaft,arms::Point(inverse,nativePalm));return arms::Finite(out)&&arms::Length(shaft.tip-shaft.root)>.00001f;
}
inline bool AnchorStaffWeapon(arms::Mat& mesh,Vec localAnchor,Vec palm){
    if(!arms::Affine(mesh)||!arms::Finite(localAnchor)||!arms::Finite(palm))return false;
    arms::Position(mesh,palm-arms::Vector(mesh,localAnchor));return arms::Affine(mesh);
}
// The two palms face opposite grip-X directions. Apply depth into the palm,
// not through its back; finger-space offsets also belong to the support hand.
inline Vec StaffContactOffset(unsigned side,int forward,int depth,float units){
    return {(side==1?-1.f:1.f)*std::clamp(depth,-10,10)*.01f*units,-std::clamp(forward,-10,10)*.01f*units,0};
}
inline bool StaffContactLocal(arms::Mat const& calibration,Vec palmLocal,unsigned side,int forward,int depth,float units,Vec& out){
    arms::Mat inverse;if(side>1||!arms::Finite(palmLocal)||!std::isfinite(units)||units<=0||!arms::Inverse(calibration,inverse))return false;
    out=palmLocal+arms::Vector(inverse,StaffContactOffset(side,forward,depth,units));return arms::Finite(out);
}
// Contact offsets never change the tracked wrist or hand rotation.
inline bool StaffPalmContact(arms::Mat const& grip,arms::Mat const& calibration,Vec wrist,Vec palmLocal,
                             float units,int towardFingers,Vec& contact,int depth=0,unsigned side=1){
    if(!arms::Affine(grip)||!arms::Affine(calibration)||!arms::Finite(wrist)||!arms::Finite(palmLocal)||!std::isfinite(units)||units<=0)return false;
    Vec local;if(!StaffContactLocal(calibration,palmLocal,side,towardFingers,depth,units,local))return false;
    auto hand=arms::Multiply(calibration,grip);arms::Position(hand,wrist);
    contact=arms::Point(hand,local);return arms::Finite(contact);
}
// Construct a weapon frame independently of the tracked hand. Keep the
// accepted long-end direction; calibrating the mesh never rotates the palm.
inline bool StaffWeaponPose(Blade const& shaft,Vec localAnchor,arms::Mat const& grip,Vec palm,
                            int const* angles,arms::Mat& mesh){
    if(!arms::Affine(grip)||!arms::Finite(localAnchor)||!arms::Finite(palm)||!angles)return false;
    Vec axis=arms::Length(shaft.root-localAnchor)>arms::Length(shaft.tip-localAnchor)?shaft.root-shaft.tip:shaft.tip-shaft.root;
    if(!arms::Unit(axis))return false;
    arms::Mat local,inv;if(!arms::BodyFrame(axis,std::fabs(axis.y)<.9f?Vec{0,1,0}:Vec{1,0,0},local)||!arms::Inverse(local,inv))return false;
    float radians[3]{angles[0]*.01745329252f,angles[1]*.01745329252f,angles[2]*.01745329252f};
    auto corrected=arms::Multiply(arms::Rotation(NativeCameraRotation(radians)),grip);arms::Mat worldFrame;
    if(!arms::BodyFrame(arms::Vector(corrected,{0,0,1}),arms::Vector(corrected,{0,1,0}),worldFrame))return false;
    mesh=arms::Multiply(inv,worldFrame);return AnchorStaffWeapon(mesh,localAnchor,palm);
}
// Express the rendered shaft in tracking space, removing native locomotion.
inline bool TrackingWeaponBlade(ControllerFrame const& frame,unsigned hand,arms::Mat const& worldController,Blade const& worldBlade,float units,Blade& out){
    if(hand>1||!std::isfinite(units)||units<=0)return false;auto const& p=frame.hands[hand].grip;
    Orientation q{p.quaternion[3],p.quaternion[0],p.quaternion[1],p.quaternion[2]};arms::Mat inverse;
    if(!p.valid||!p.tracked||!Normalize(q)||!arms::Inverse(worldController,inverse))return false;
    auto raw=arms::Rotation(Matrix(q));arms::Position(raw,{p.position[0],p.position[1],p.position[2]});
    auto convert=[&](Vec point){auto local=arms::Point(inverse,point)*(1.f/units);local.z=-local.z;return arms::Point(raw,local);};
    out={convert(worldBlade.root),convert(worldBlade.tip)};return arms::Finite(out.root)&&arms::Finite(out.tip);
}
inline bool TrackingShaftAxis(ControllerFrame const& frame,arms::Mat const& worldController,Vec worldAxis,Vec& out){
    arms::Mat raw[2],inverse;if(!TrackingGrips(frame,raw)||!arms::Inverse(worldController,inverse)||!arms::Finite(worldAxis))return false;
    auto local=arms::Vector(inverse,worldAxis);local.z=-local.z;out=arms::Vector(raw[1],local);return arms::Unit(out);
}
}}
