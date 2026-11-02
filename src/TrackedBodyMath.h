#pragma once
#include "TrackedArmsMath.h"

namespace diagnostic { namespace arms {
enum class BodyPart {Other,Neutral,Leg};
inline BodyPart IdentifyBody(char const* name){
    if(!name)return BodyPart::Other;
    if(!strcmp(name,"Hero_ROOT")||!strncmp(name,"Hero_Spine_Spine_",17)||
       !strcmp(name,"Hero_Left_Arm_Shoulder_1")||!strcmp(name,"Hero_Right_Arm_Shoulder_1")||
       !strncmp(name,"Hero_Head_",10))return BodyPart::Neutral;
    if(!strncmp(name,"Hero_Left_Leg_",14)||!strncmp(name,"Hero_Right_Leg_",15)||
       !strncmp(name,"Hero_Left_Foot_",15)||!strncmp(name,"Hero_Right_Foot_",16))return BodyPart::Leg;
    return BodyPart::Other;
}
inline bool BodyWorld(Mat const& entity,Rotation3 const& camera,Mat& out,Vec const& renderCorrection={}){
    if(!Affine(entity))return false;
    float yaw;if(!HorizontalHeading(camera,yaw))return false;
    float angles[3]{0,yaw,0};out=Rotation(NativeCameraRotation(angles));
    for(unsigned row=0;row<3;++row){
        float scale=Length({entity.m[row*4],entity.m[row*4+1],entity.m[row*4+2]});
        if(!std::isfinite(scale)||scale<.001f)return false;
        for(unsigned col=0;col<3;++col)out.m[row*4+col]*=scale;
    }
    Position(out,Position(entity)+renderCorrection);return Affine(out);
}
inline bool Rebase(Mat const& native,Mat const& desired,Mat& delta){
    Mat inverse;if(!Inverse(native,inverse)||!Affine(desired))return false;
    delta=Multiply(inverse,desired);return Affine(delta);
}
inline bool BodyPoseDeltas(Mat const& entity,Mat const& ground,Mat const* bind,Mat const* bone,BodyPart const* parts,
    unsigned count,int pelvis,bool crouched,bool pickup,Mat const* crouchLower,Mat& body,Mat* delta){
    if(!bind||!bone||!parts||!delta||pelvis<0||unsigned(pelvis)>=count||!Affine(ground))return false;
    body=ground;Mat pelvisDelta,groundDelta,desiredPelvis=Multiply(bind[pelvis],ground);
    if(crouched){
        if(!Rebase(entity,ground,groundDelta))return false;
        auto nativePelvis=pickup&&crouchLower?Multiply(crouchLower[pelvis],ground):Multiply(bone[pelvis],groundDelta);
        auto position=Position(body);position.y+=Position(nativePelvis).y-Position(desiredPelvis).y;Position(body,position);
        desiredPelvis=nativePelvis;
    }
    if(!Rebase(bone[pelvis],desiredPelvis,pelvisDelta))return false;
    for(unsigned i=0;i<count;++i){
        if(parts[i]==BodyPart::Neutral){
            if(!Rebase(bone[i],int(i)==pelvis?desiredPelvis:Multiply(bind[i],body),delta[i]))return false;
        }else if(parts[i]==BodyPart::Leg){
            if(pickup&&(!crouched||crouchLower)){
                // Stand still while native collection runs. In sneak retain
                // the last grounded crouch, rather than the pickup knee bend.
                auto const& pose=crouched?crouchLower[i]:bind[i];
                if(!Rebase(bone[i],Multiply(pose,ground),delta[i]))return false;
            }else delta[i]=crouched?groundDelta:pelvisDelta;
        }
    }
    return true;
}
} }
