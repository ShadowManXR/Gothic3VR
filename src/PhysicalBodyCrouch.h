#pragma once
#include "TrackedBodyMath.h"
namespace diagnostic {
inline bool FirstPersonHandsOnly(bool firstPerson,bool selected,bool deepCrouch){return firstPerson&&(selected||deepCrouch);}
// Physical HMD drop changes the render pose only. Native sneak state, camera
// calibration, entity collision and controller/world origins are untouched.
class PhysicalBodyCrouch {
    uintptr_t actor=0;uint64_t session=0,last=0;float drop=0;bool hidden=false;
public:
    void Reset(){actor=0;session=last=0;drop=0;hidden=false;}
    void Update(uintptr_t player,uint64_t tracking,uint64_t now,bool allowed,float standing,float head,float nativeDrop){
        if(!allowed||!player||!tracking||!now||!std::isfinite(standing)||!std::isfinite(head)||!std::isfinite(nativeDrop)){Reset();return;}
        bool fresh=actor!=player||session!=tracking||now<last||(last&&now-last>250);
        if(fresh){Reset();actor=player;session=tracking;last=now;}
        float depth=std::max(0.f,standing-head),native=std::clamp(nativeDrop,0.f,.7f),total=depth+native;
        if(!hidden&&total>.75f)hidden=true;else if(hidden&&total<.65f)hidden=false;
        float target=std::min(std::max(0.f,depth-.04f),std::max(0.f,.5f-native));
        float dt=float(now-last)*.001f;
        if(fresh)drop=target;else if(dt>0)drop+=(target-drop)*float(-std::expm1(-dt/.085f));
        if(std::fabs(drop-target)<.0005f)drop=target;last=now;
    }
    float DropMeters()const{return drop;}
    bool HandsOnly()const{return hidden;}
};
namespace arms {
struct CrouchLeg {int root=-1,upper=-1,lower=-1,foot=-1,toe=-1;};
inline bool LowerRenderBody(Mat const* bone,BodyPart const* parts,unsigned count,CrouchLeg const (&legs)[2],float drop,Mat& body,Mat* delta){
    if(!bone||!parts||!delta||!std::isfinite(drop)||drop<=0||!Affine(body))return false;
    Mat upper[2],lower[2];
    for(unsigned h=0;h<2;++h){auto const& leg=legs[h];for(int i:{leg.root,leg.upper,leg.lower,leg.foot,leg.toe})if(i<0||unsigned(i)>=count)return false;
        Mat oldUpper=Multiply(bone[leg.upper],delta[leg.upper]),oldLower=Multiply(bone[leg.lower],delta[leg.lower]),oldFoot=Multiply(bone[leg.foot],delta[leg.foot]);
        Vec shift{0,drop,0},hip=Position(oldUpper),knee=Position(oldLower),foot=Position(oldFoot);Solution solved;
        if(!Solve(hip-shift,knee-shift,foot-shift,foot,Vector(body,{0,0,1}),solved)||Length(solved.wrist-foot)>.01f)return false;
        if(!JointDelta(oldUpper,knee,hip-shift,solved.elbow,upper[h])||!JointDelta(oldLower,foot,solved.elbow,foot,lower[h]))return false;
    }
    // All validation and both IK chains complete before changing the pose.
    Mat down;Position(down,{0,-drop,0});
    for(unsigned i=0;i<count;++i)if(parts[i]==BodyPart::Neutral)delta[i]=Multiply(delta[i],down);
    for(unsigned h=0;h<2;++h){auto const& leg=legs[h];delta[leg.root]=Multiply(delta[leg.root],down);delta[leg.upper]=Multiply(delta[leg.upper],upper[h]);delta[leg.lower]=Multiply(delta[leg.lower],lower[h]);}
    body=Multiply(body,down);return true;
}
inline Mat CollapseSkinAt(Vec point){Mat out{};for(unsigned i=0;i<12;++i)out.m[i]=0;out.m[0]=out.m[5]=out.m[10]=.000001f;Position(out,point);return out;}
// Collapsing non-hand skin weights to collinear wrist anchors makes their
// triangles subpixel. A tiny scale retains finite shader normals instead of
// normalizing a zero vector. Wrist/hand/finger palettes stay exact; no alpha change
// can accidentally hide the hands or other entities sharing this material.
inline void HandsOnlySkin(Mat* palette,BoneTag const* tags,unsigned count,Vec left,Vec right,bool const (&tracked)[2]){
    if(!palette||!tags||!Finite(left)||!Finite(right))return;
    if(!tracked[0])left=right;if(!tracked[1])right=left;
    Mat hidden[3]{CollapseSkinAt(left),CollapseSkinAt(right),CollapseSkinAt((left+right)*.5f)};
    for(unsigned i=0;i<count;++i){auto tag=tags[i];if(tag.side>=0&&tag.side<2&&tag.part==Hand&&tracked[tag.side])continue;
        palette[i]=hidden[tag.side>=0&&tag.side<2?tag.side:2];}
}
}
}
