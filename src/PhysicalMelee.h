#pragma once
#include "TrackedArmsMath.h"
#include "TrackingData.h"
#include <array>

namespace diagnostic { namespace melee {
using arms::Vec;
struct Blade { Vec root,tip; };
inline Blade Interpolate(Blade const& a,Blade const& b,float t){return {a.root+(b.root-a.root)*t,a.tip+(b.tip-a.tip)*t};}
struct BladeCapsule {Vec center;arms::Mat orientation;float halfHeight=0,radius=0;};
// Engine RVA 17495B loads bCCapsule.height and RVA 174967 halves it to
// construct the endpoints. The SDK takes full height, not PhysX half-height.
inline float NativeCapsuleHeight(BladeCapsule const& shape){return 2.f*shape.halfHeight;}
inline bool Capsule(Blade const& blade,float units,BladeCapsule& out){
    Vec direction=blade.tip-blade.root;float length=arms::Length(direction);
    if(!arms::Finite(blade.root)||!arms::Finite(blade.tip)||!std::isfinite(units)||units<=0||length<.001f||!arms::Between({0,1,0},direction,out.orientation))return false;
    out.center=(blade.root+blade.tip)*.5f;out.halfHeight=length*.5f;out.radius=.02f*units;return true;
}
inline Vec ClosestPoint(Blade const& blade,Vec point){Vec d=blade.tip-blade.root;float n=arms::Dot(d,d);return blade.root+d*(n>.000001f?std::clamp(arms::Dot(point-blade.root,d)/n,0.f,1.f):0.f);}
inline unsigned CapsuleSteps(Blade const& a,Blade const& b,float units){
    if(!std::isfinite(units)||units<=0)return 0;
    float motion=std::max(arms::Length(b.root-a.root),arms::Length(b.tip-a.tip));
    return std::isfinite(motion)?unsigned(std::clamp(std::ceil(motion/(.035f*units)),1.f,24.f)):0;
}
// Construct the mesh's alignment frame in grip space. OneHandPose folds this
// into the rigid weapon attachment; it must not replace the tracked hand pose.
inline bool SwordGrip(arms::Mat const& attachment,arms::Mat const& handCalibration,Blade const& local,arms::Mat const& grip,int const* angles,arms::Mat& out){
    if(!arms::Affine(attachment)||!arms::Affine(handCalibration)||!arms::Affine(grip)||!angles)return false;
    Vec axis=arms::Vector(arms::Multiply(attachment,handCalibration),local.tip-local.root);
    arms::Mat forward;if(!arms::Between(axis,{0,0,1},forward))return false;
    float radians[3]{float(angles[0])*.01745329252f,float(angles[1])*.01745329252f,float(angles[2])*.01745329252f};
    out=arms::Multiply(arms::Multiply(forward,arms::Rotation(NativeCameraRotation(radians))),grip);return arms::Affine(out);
}
// Derive the cutting segment from the equipped mesh, preserving its pivot.
inline bool LocalBlade(Vec lo,Vec hi,float units,Blade& out,float maximumLength=2.2f,float minimumLength=.25f) {
    if(!arms::Finite(lo)||!arms::Finite(hi)||!std::isfinite(units)||units<=0)return false;
    float a[3]{lo.x,lo.y,lo.z},b[3]{hi.x,hi.y,hi.z};int axis=0;
    for(int i=0;i<3;++i){if(b[i]<a[i])return false;if(b[i]-a[i]>b[axis]-a[axis])axis=i;}
    float length=b[axis]-a[axis];if(length<minimumLength*units||length>maximumLength*units)return false;
    float start=std::clamp(0.f,a[axis],b[axis]);float end=std::fabs(b[axis])>=std::fabs(a[axis])?b[axis]:a[axis];
    if(std::fabs(end-start)<minimumLength*.8f*units)return false;
    float root[3]{(a[0]+b[0])*.5f,(a[1]+b[1])*.5f,(a[2]+b[2])*.5f};
    float tip[3]{root[0],root[1],root[2]};root[axis]=start+(end-start)*.12f;tip[axis]=end;
    out={{root[0],root[1],root[2]},{tip[0],tip[1],tip[2]}};return true;
}
struct Sample {
    uintptr_t player=0,weapon=0;uint64_t time=0,session=0;
    Blade blade,trackingBlade;bool trackedBlade=false;Vec hand,relativeHand,trackingHand;Orientation orientation;
    Vec trackingShaftAxis{0,0,1};float units=100;bool valid=false,twoHand=false,doubleEnded=false,heavySingleHand=false,trigger=false;
};
inline float Angle(Orientation a,Orientation b){
    if(!Normalize(a)||!Normalize(b))return INFINITY;
    return 2*std::acos(std::clamp(std::fabs(a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z),0.f,1.f));
}
inline int ScaledDamage(int nativeDamage,int weaponBase,float intensity) {
    if(nativeDamage<=0||weaponBase<=0||!std::isfinite(intensity)||intensity<=0)return 0;
    // Native damage already includes attributes, learned perks, item upgrades,
    // weakness and target armour. Weapon base damage is NOT a progression cap.
    return int(std::min(double(nativeDamage),std::floor(double(nativeDamage)*std::clamp(intensity,0.f,1.f)+.0001)));
}
inline bool BlockAttack(bool vr,bool menu,bool meleeWeapon,uint16_t key,bool incapacitated=false) {
    return vr&&!menu&&!incapacitated&&meleeWeapon&&(key==15||key==16); // Use1 / Use2 also serve native recovery.
}
inline bool RecoveryState(int action,int animation,bool down,bool dead){
    return down||dead||(action>=29&&action<=33)||(animation>=15&&animation<=17);
}
class Swing {
    Sample previous;uint64_t settle=0,rest=0,began=0;bool active=false;
    float travel=0,rotation=0;Vec direction{},angularDirection{};std::array<uintptr_t,16> hit{};unsigned hitCount=0;
    void End(){active=false;travel=rotation=0;hitCount=0;began=0;direction=angularDirection={};}
public:
    unsigned generation=0;float intensity=0,speed=0;Sample from,to;
    void Reset(){previous={};settle=rest=0;End();intensity=speed=0;from=to={};}
    bool Step(Sample const& s) {
        intensity=0;
        if(!s.valid||!arms::Finite(s.hand)||!arms::Finite(s.relativeHand)||!arms::Finite(s.trackingHand)||!arms::Finite(s.blade.root)||!arms::Finite(s.blade.tip)||!std::isfinite(s.units)||s.units<=0){Reset();return false;}
        Vec shaft=s.trackingShaftAxis;if(s.doubleEnded&&!arms::Unit(shaft)){Reset();return false;}
        if(!previous.valid||s.player!=previous.player||s.weapon!=previous.weapon||s.session!=previous.session||s.twoHand!=previous.twoHand||s.heavySingleHand!=previous.heavySingleHand||s.doubleEnded!=previous.doubleEnded||s.time<previous.time||s.time-previous.time>100){
            Reset();previous=s;settle=s.time+300;return false;}
        if(s.time==previous.time)return false; // Multiple eyes/renders never duplicate a swing.
        float dt=float(s.time-previous.time)*.001f;
        Vec delta=s.relativeHand-previous.relativeHand;float distance=arms::Length(delta);
        float trackedDistance=arms::Length(s.trackingHand-previous.trackingHand);
        float angle=0;Vec rotationDelta;
        if(s.doubleEnded){Vec before=previous.trackingShaftAxis;if(!arms::Unit(before)){Reset();return false;}
            // A staff rolled about its stationary shaft does not sweep either
            // end into a target. Measure shaft direction, not wrist roll.
            angle=std::acos(std::clamp(arms::Dot(before,shaft),-1.f,1.f));rotationDelta=arms::Cross(before,shaft);
            if(arms::Unit(rotationDelta))rotationDelta=rotationDelta*angle;else rotationDelta={};
        }else{
            angle=Angle(s.orientation,previous.orientation);
            auto angular=Product(s.orientation,{previous.orientation.w,-previous.orientation.x,-previous.orientation.y,-previous.orientation.z});
            if(!Normalize(angular)){Reset();return false;}if(angular.w<0){angular.w=-angular.w;angular.x=-angular.x;angular.y=-angular.y;angular.z=-angular.z;}
            rotationDelta={angular.x,angular.y,angular.z};float sinHalf=arms::Length(rotationDelta);
            if(sinHalf>.000001f)rotationDelta=rotationDelta*(angle/sinHalf);else rotationDelta={};
        }
        // Tracking jumps, teleports and recentering are not weapon strokes.
        if(!std::isfinite(angle)||distance>.5f||angle>1.2f||arms::Length(s.hand-previous.hand)>s.units*1.5f){Reset();previous=s;settle=s.time+300;return false;}
        from=previous;to=s;previous=s;
        // Head movement alone cannot swing a stationary tracked controller.
        float lever=s.doubleEnded?std::clamp(std::max(arms::Length(s.blade.root-s.hand),arms::Length(s.blade.tip-s.hand))/s.units,.22f,3.5f):.22f;
        speed=std::max(std::min(distance,trackedDistance),lever*angle)/dt;
        if(s.time<settle)return false;
        if(speed<.18f){if(!rest)rest=s.time;if(s.time-rest>=140)End();return false;}
        rest=0;
        bool linearReverse=distance>.003f&&arms::Length(direction)>.01f&&arms::Dot(delta,direction)<-.35f*distance*arms::Length(direction);
        bool angularReverse=angle>.015f&&arms::Length(angularDirection)>.008f&&arms::Dot(rotationDelta,angularDirection)<-.35f*angle*arms::Length(angularDirection);
        if(active&&began&&s.time-began>=180&&(linearReverse||angularReverse))End();
        if(!active){if(speed<.35f)return false;active=true;began=s.time;travel=rotation=0;hitCount=0;++generation;}
        travel+=distance;rotation+=angle;direction=direction*.8f+delta*.2f;angularDirection=angularDirection*.8f+rotationDelta*.2f;
        if(travel<.03f&&rotation<.14f)return false;
        intensity=.15f+.85f*std::clamp(std::max(travel/.35f,rotation/1.5f),0.f,1.f);
        return speed>=.18f;
    }
    bool Hit(uintptr_t target){if(!target||!intensity)return false;for(unsigned i=0;i<hitCount;++i)if(hit[i]==target)return false;if(hitCount==hit.size())return false;hit[hitCount++]=target;return true;}
    bool AlreadyHit(uintptr_t target)const{for(unsigned i=0;i<hitCount;++i)if(hit[i]==target)return true;return false;}
};
struct Segment {Vec a,b;};
struct Sweep {std::array<Segment,96> segments{};unsigned count=0;};
inline Sweep BladeSweep(Blade const& a,Blade const& b,float units) {
    Sweep out;if(units<=0||!arms::Finite(a.root)||!arms::Finite(a.tip)||!arms::Finite(b.root)||!arms::Finite(b.tip))return out;
    auto add=[&](Vec p,Vec q){if(out.count<out.segments.size()&&arms::Length(q-p)>.001f)out.segments[out.count++]={p,q};};
    float length=std::max(arms::Length(a.tip-a.root),arms::Length(b.tip-b.root));
    unsigned points=unsigned(std::clamp(std::ceil(length/(.05f*units)),1.f,44.f));
    for(unsigned i=0;i<=points;++i){float t=float(i)/points;add(a.root+(a.tip-a.root)*t,b.root+(b.tip-b.root)*t);}
    float motion=std::max(arms::Length(b.root-a.root),arms::Length(b.tip-a.tip));
    unsigned steps=unsigned(std::clamp(std::ceil(motion/(.05f*units)),1.f,32.f));
    for(unsigned i=0;i<=steps;++i){float t=float(i)/steps;add(a.root+(b.root-a.root)*t,a.tip+(b.tip-a.tip)*t);}
    return out;
}
} }
