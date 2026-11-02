#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic { namespace arms {
constexpr float ArmPi=3.14159265359f,ArmDegree=ArmPi/180.f;
struct TwistAngles {float wrist=0,forearm=0,upper=0;};
// Solve the best axial rotation in the plane perpendicular to the bone.
// Projecting removes longitudinal IK stretch, so arm extension cannot become
// wrist roll. For rigid frames this equals quaternion swing/twist extraction.
inline bool AxialTwist(Mat const& from,Mat const& to,Vec axis,float& angle,bool& defined){
    if(!Affine(from)||!Affine(to)||!Unit(axis))return false;
    float cosine=0,sine=0,fromScale=0,toScale=0;
    Vec a[3]{Vector(from,{1,0,0}),Vector(from,{0,1,0}),Vector(from,{0,0,1})};
    Vec b[3]{Vector(to,{1,0,0}),Vector(to,{0,1,0}),Vector(to,{0,0,1})};
    for(unsigned i=0;i<3;++i){fromScale+=Dot(a[i],a[i]);toScale+=Dot(b[i],b[i]);}
    float scale=std::sqrt(fromScale*toScale);if(!std::isfinite(scale)||scale<1e-8f)return false;
    for(unsigned i=0;i<3;++i){
        auto x=a[i]-axis*Dot(a[i],axis),y=b[i]-axis*Dot(b[i],axis);
        cosine+=Dot(x,y)/scale;sine+=Dot(axis,Cross(x,y))/scale;
    }
    defined=cosine*cosine+sine*sine>1e-8f;
    angle=defined?std::atan2(sine,cosine):0.f;return std::isfinite(angle);
}
// The first few degrees stay at the wrist, then the forearm takes over. Once
// forearm pronation/supination approaches its limit the upper arm contributes.
// Smooth knees avoid a mechanical snap when crossing either limit.
inline float SoftJointLimit(float angle,float free,float limit){
    float value=std::fabs(angle),range=limit-free;
    if(value>free)value=free+range*(-std::expm1(-(value-free)/range));
    return std::copysign(value,angle);
}
inline TwistAngles DistributeTwist(float angle){
    TwistAngles out;out.wrist=SoftJointLimit(angle,12*ArmDegree,22*ArmDegree);
    float remainder=angle-out.wrist;
    out.forearm=SoftJointLimit(remainder,60*ArmDegree,85*ArmDegree);
    out.upper=remainder-out.forearm;return out;
}
class ArmTwistChain {
    uintptr_t owner=0;uint64_t session=0,time=0;float previous=0,total=0;bool valid=false;
public:
    void Reset(){owner=0;session=time=0;previous=total=0;valid=false;}
    bool Step(float wrapped,bool defined,uintptr_t actor,uint64_t origin,uint64_t stamp,TwistAngles& out){
        if(!actor||!stamp||!std::isfinite(wrapped)){Reset();return false;}
        bool fresh=!valid||actor!=owner||origin!=session||stamp<time||stamp-time>100;
        if(fresh){total=defined?wrapped:0;previous=defined?wrapped:0;}
        else if(defined){float change=wrapped-previous;
            while(change>ArmPi)change-=2*ArmPi;while(change<-ArmPi)change+=2*ArmPi;
            total+=change;previous=wrapped;
        }
        owner=actor;session=origin;time=stamp;valid=true;out=DistributeTwist(total);
        return std::isfinite(out.wrist)&&std::isfinite(out.forearm)&&std::isfinite(out.upper);
    }
};
inline bool TwistDelta(Mat const& before,Vec start,Vec end,float angle,Mat& out){
    Vec axis=end-start;if(!Affine(before)||!Unit(axis)||!std::isfinite(angle))return false;
    float half=angle*.5f;auto turn=Rotation(Matrix({std::cos(half),axis.x*std::sin(half),axis.y*std::sin(half),axis.z*std::sin(half)}));
    Position(turn,start-Vector(turn,start));out=Multiply(before,turn);return Affine(out);
}
// Keep the existing IK positions/stretch and the exact final hand matrix.
// Bind-pose references prevent native idle/equip animations from becoming
// controller twist. Only the visual upper/forearm deltas are changed.
inline bool ProgressiveArmTwist(Mat const& upper,Mat const& lower,
    Mat const& referenceUpper,Mat const& referenceLower,Mat const& referenceHand,
    Mat const& targetHand,Vec elbow,Vec wrist,Mat& upperDelta,Mat& lowerDelta,
    ArmTwistChain& chain,uintptr_t actor,uint64_t session,uint64_t stamp,TwistAngles& angles){
    Mat lowerSwing,upperSwing;
    Vec shoulder=Position(upper),axis=wrist-elbow;
    if(!Between(Position(referenceHand)-Position(referenceLower),axis,lowerSwing)||
       !Between(Position(referenceLower)-Position(referenceUpper),elbow-shoulder,upperSwing))return false;
    auto neutralHand=Multiply(referenceHand,lowerSwing),neutralLower=Multiply(referenceLower,lowerSwing),neutralUpper=Multiply(referenceUpper,upperSwing);
    float desired=0,lowerCorrection=0,upperCorrection=0;bool defined=false,lowerDefined=false,upperDefined=false;
    if(!AxialTwist(neutralHand,targetHand,axis,desired,defined)||
       !AxialTwist(Multiply(lower,lowerDelta),neutralLower,axis,lowerCorrection,lowerDefined)||
       !AxialTwist(Multiply(upper,upperDelta),neutralUpper,elbow-shoulder,upperCorrection,upperDefined)||
       !chain.Step(desired,defined,actor,session,stamp,angles))return false;
    Mat nextUpper,nextLower;
    if(!TwistDelta(upperDelta,shoulder,elbow,(upperDefined?upperCorrection:0)+angles.upper,nextUpper)||
       !TwistDelta(lowerDelta,elbow,wrist,(lowerDefined?lowerCorrection:0)+angles.forearm+angles.upper,nextLower))return false;
    upperDelta=nextUpper;lowerDelta=nextLower;return true;
}
}}
