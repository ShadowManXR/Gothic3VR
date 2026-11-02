#pragma once
#include "WorldAim.h"
#include <cctype>
#include <cstring>

namespace diagnostic { namespace arms {
struct Vec {float x=0,y=0,z=0;};
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator*(Vec a,float b){return {a.x*b,a.y*b,a.z*b};}
inline float Dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec Cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float Length(Vec a){return std::sqrt(Dot(a,a));}
inline bool Finite(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline bool Unit(Vec& a){float n=Length(a);if(!Finite(a)||n<1e-5f)return false;a=a*(1/n);return true;}

// Raw bCMatrix memory, row vectors and translation at floats 12..14.
// Do not call SDK matrix multiplication: its operand convention differs.
struct Mat {float m[16]{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};};
inline Vec Position(Mat const& a){return {a.m[12],a.m[13],a.m[14]};}
inline void Position(Mat& a,Vec p){a.m[12]=p.x;a.m[13]=p.y;a.m[14]=p.z;}
inline bool Affine(Mat const& a){for(float v:a.m)if(!std::isfinite(v))return false;return std::fabs(a.m[3])+std::fabs(a.m[7])+std::fabs(a.m[11])+std::fabs(a.m[15]-1)<.001f;}
inline Mat Multiply(Mat const& a,Mat const& b){Mat c;for(int r=0;r<4;++r)for(int col=0;col<4;++col){c.m[r*4+col]=0;for(int k=0;k<4;++k)c.m[r*4+col]+=a.m[r*4+k]*b.m[k*4+col];}return c;}
inline Vec Vector(Mat const& a,Vec p){return {p.x*a.m[0]+p.y*a.m[4]+p.z*a.m[8],p.x*a.m[1]+p.y*a.m[5]+p.z*a.m[9],p.x*a.m[2]+p.y*a.m[6]+p.z*a.m[10]};}
inline Vec Point(Mat const& a,Vec p){return Vector(a,p)+Position(a);}
inline bool Inverse(Mat const& a,Mat& out){
    if(!Affine(a))return false;
    float A=a.m[0],B=a.m[1],C=a.m[2],D=a.m[4],E=a.m[5],F=a.m[6],G=a.m[8],H=a.m[9],I=a.m[10];
    float det=A*(E*I-F*H)-B*(D*I-F*G)+C*(D*H-E*G);if(!std::isfinite(det)||std::fabs(det)<1e-6f)return false;
    float s=1/det;out=Mat{};
    out.m[0]=(E*I-F*H)*s;out.m[1]=(C*H-B*I)*s;out.m[2]=(B*F-C*E)*s;
    out.m[4]=(F*G-D*I)*s;out.m[5]=(A*I-C*G)*s;out.m[6]=(C*D-A*F)*s;
    out.m[8]=(D*H-E*G)*s;out.m[9]=(B*G-A*H)*s;out.m[10]=(A*E-B*D)*s;
    Position(out,Vector(out,Position(a)*-1));return Affine(out);
}
inline Mat Rotation(Rotation3 const& r){Mat a;for(int row=0;row<3;++row)for(int col=0;col<3;++col)a.m[row*4+col]=r.m[col*3+row];return a;}
inline bool Between(Vec from,Vec to,Mat& result){
    if(!Unit(from)||!Unit(to))return false;
    float d=std::clamp(Dot(from,to),-1.f,1.f);Vec c=Cross(from,to);Orientation q{1+d,c.x,c.y,c.z};
    if(d<-.9999f){Vec axis=Cross(from,std::fabs(from.x)<.8f?Vec{1,0,0}:Vec{0,1,0});if(!Unit(axis))return false;q={0,axis.x,axis.y,axis.z};}
    if(!Normalize(q))return false;result=Rotation(Matrix(q));return true;
}
inline bool BodyFrame(Vec fingers,Vec thumb,Mat& out){
    if(!Unit(fingers))return false;thumb=thumb-fingers*Dot(thumb,fingers);if(!Unit(thumb))return false;
    Vec right=Cross(thumb,fingers);if(!Unit(right))return false;
    out=Mat{};out.m[0]=right.x;out.m[1]=right.y;out.m[2]=right.z;
    out.m[4]=thumb.x;out.m[5]=thumb.y;out.m[6]=thumb.z;
    out.m[8]=fingers.x;out.m[9]=fingers.y;out.m[10]=fingers.z;return true;
}
inline bool GripCalibration(Mat const& hand,Vec index,Vec pinky,Mat& out){
    // Use the knuckle plane, not the thumb base (which lies on the palm's
    // curved surface). OpenXR grip -Z runs from little finger toward index;
    // in Genome that is +Z. Straight fingers run along grip -Y, not aim +Z.
    Mat model,gripAxes,inverse;
    Vec fingers=(index+pinky)*.5f-Position(hand),across=index-pinky;
    if(!Affine(hand)||!BodyFrame(fingers,across,model)||!Inverse(model,inverse)||
       !BodyFrame({0,-1,0},{0,0,1},gripAxes))return false;
    out=Multiply(Multiply(hand,inverse),gripAxes);Position(out,{});return Affine(out);
}
struct Solution {Vec elbow,wrist;bool limited=false;};
inline bool Solve(Vec shoulder,Vec oldElbow,Vec oldWrist,Vec target,Vec pole,Solution& out){
    if(!Finite(shoulder)||!Finite(oldElbow)||!Finite(oldWrist)||!Finite(target)||!Finite(pole))return false;
    float upper=Length(oldElbow-shoulder),lower=Length(oldWrist-oldElbow);
    if(upper<1e-3f||lower<1e-3f)return false;
    Vec direction=target-shoulder;float distance=Length(direction);if(!Unit(direction))return false;
    float epsilon=(upper+lower)*.0001f;
    float clamped=std::clamp(distance,std::fabs(upper-lower)+epsilon,upper+lower-epsilon);out.limited=std::fabs(distance-clamped)>epsilon;
    Vec bend=pole-direction*Dot(pole,direction);
    if(!Unit(bend)){bend=Cross(direction,std::fabs(direction.x)<.8f?Vec{1,0,0}:Vec{0,1,0});if(!Unit(bend))return false;}
    float c=std::clamp((upper*upper+clamped*clamped-lower*lower)/(2*upper*clamped),-1.f,1.f);
    out.wrist=shoulder+direction*clamped;out.elbow=shoulder+(direction*c+bend*std::sqrt(std::max(0.f,1-c*c)))*upper;
    return Finite(out.wrist)&&Finite(out.elbow);
}
inline bool JointDelta(Mat const& oldJoint,Vec oldEnd,Vec newStart,Vec newEnd,Mat& delta){
    Mat rotate;if(!Between(oldEnd-Position(oldJoint),newEnd-newStart,rotate))return false;
    Position(rotate,newStart-Vector(rotate,Position(oldJoint)));delta=rotate;return Affine(delta);
}
// Tracked hands are authoritative. When the controller exceeds the avatar's
// arm length, extend the render skeleton instead of pulling the hand backward.
inline bool SolveTracked(Vec shoulder,Vec oldElbow,Vec oldWrist,Vec target,Vec pole,Solution& out){
    if(!Finite(shoulder)||!Finite(oldElbow)||!Finite(oldWrist)||!Finite(target)||!Finite(pole))return false;
    float a=Length(oldElbow-shoulder),b=Length(oldWrist-oldElbow);if(a<.001f||b<.001f)return false;
    if(Length(target-shoulder)<.001f){Vec bend=pole;if(!Unit(bend)){bend=oldElbow-shoulder;if(!Unit(bend))return false;}
        out.elbow=shoulder+bend*((a+b)*.5f);out.wrist=target;out.limited=true;return true;}
    if(!Solve(shoulder,oldElbow,oldWrist,target,pole,out))return false;
    if(out.limited){float upper=Length(oldElbow-shoulder),lower=Length(oldWrist-oldElbow),distance=Length(target-shoulder);
        if(distance>upper+lower){Vec direction=target-shoulder;if(!Unit(direction))return false;out.elbow=shoulder+direction*(distance*upper/(upper+lower));}
    }
    out.wrist=target;
    return Finite(out.elbow)&&Finite(out.wrist);
}
inline bool TrackedJointDelta(Mat const& oldJoint,Vec oldEnd,Vec newStart,Vec newEnd,Mat& delta){
    Vec segment=oldEnd-Position(oldJoint);float oldLength=Length(segment),newLength=Length(newEnd-newStart);
    Mat rotate;if(oldLength<.001f||newLength<.001f||!Between(segment,newEnd-newStart,rotate)||!Unit(segment))return false;
    // Scale only along the bone, retaining thickness. Then rotate and translate
    // so both skinned endpoints coincide with the solved joints, without gaps.
    float scale=newLength/oldLength-1,axis[3]{segment.x,segment.y,segment.z};Mat stretch;
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)stretch.m[r*4+c]+=scale*axis[r]*axis[c];
    delta=Multiply(stretch,rotate);Position(delta,newStart-Vector(delta,Position(oldJoint)));return Affine(delta);
}
inline bool GripWorld(ControllerPose const& pose,Rotation3 const& native,float const (&base)[3],Orientation center,float const (&origin)[3],float units,float height,Mat& out){
    WorldAim p;if(!ControllerWorldAim(native,base,center,origin,pose,units,p))return false;
    out=Rotation(NativeCameraRotation(p.rotation));Position(out,{p.origin[0],p.origin[1]+height,p.origin[2]});return Affine(out);
}
inline bool FreshGrip(ControllerFrame const& frame,unsigned hand,uint64_t now){return hand<2&&frame.focused&&now>=frame.timeMs&&now-frame.timeMs<=100&&frame.hands[hand].active&&frame.hands[hand].grip.valid&&frame.hands[hand].grip.tracked;}
// OpenXR grip position denotes the palm pivot, not a fixed distance from
// the wrist. Derive that wrist from this rig's actual palm landmark AFTER
// all orientation corrections. Rotating a stationary controller must not
// swing the palm around an invisible 8cm lever.
inline Vec WristAtPalm(Mat const& grip,Mat const& calibration,Vec palmLocal){
    return Position(grip)-Vector(Multiply(calibration,grip),palmLocal);
}
inline Mat HandAtPalm(Mat const& grip,Mat const& calibration,Vec palmLocal){
    Mat hand=Multiply(calibration,grip);Position(hand,WristAtPalm(grip,calibration,palmLocal));return hand;
}
enum Part {None,Upper,Lower,Hand};
inline bool CalibrateGrip(Mat& grip,unsigned hand,int const* values,float units){
    if(hand>1||!values||!Affine(grip)||!std::isfinite(units)||units<=0)return false;
    Vec local{float(values[0])*(hand?1.f:-1.f)*.01f*units,float(values[1])*.01f*units,float(values[2])*.01f*units};
    Vec position=Point(grip,local);float radians[3]{float(values[3])*.01745329252f,float(values[4])*.01745329252f,float(values[5])*.01745329252f};
    grip=Multiply(Rotation(NativeCameraRotation(radians)),grip);Position(grip,position);return Affine(grip);
}
struct BoneTag {int side=-1;Part part=None;bool joint=false,index=false,thumb=false,pinky=false;int finger=-1,segment=-1;};
inline BoneTag Identify(char const* text){
    char n[128]{};unsigned used=0;for(;text&&*text&&used<127;++text)if(std::isalnum(static_cast<unsigned char>(*text)))n[used++]=char(std::tolower(static_cast<unsigned char>(*text)));
    char const* p=n;bool hero=!strncmp(p,"hero",4);if(hero)p+=4;else if(!strncmp(p,"bip01",5))p+=5;else if(!strncmp(p,"bip001",6))p+=6;
    BoneTag tag;if(!strncmp(p,"right",5)){tag.side=1;p+=5;}else if(!strncmp(p,"left",4)){tag.side=0;p+=4;}else if(*p=='r'||*p=='l'){tag.side=*p=='r'?1:0;++p;}else return tag;
    if(hero){
        // Names captured from the actual 73-bone Hero rig. Shoulder_1 is
        // the clavicle; Arm_1, Arm_2 and Hand_1 form the two-bone chain.
        if(!strcmp(p,"armarm1")){tag.part=Upper;tag.joint=true;}
        else if(!strcmp(p,"armarm2")){tag.part=Lower;tag.joint=true;}
        else if(!strcmp(p,"handhand1")){tag.part=Hand;tag.joint=true;}
        else if(!strncmp(p,"handfinger",10)&&strlen(p)==12&&p[10]>='1'&&p[10]<='5'&&p[11]>='1'&&p[11]<='3'){
            tag.part=Hand;tag.thumb=p[10]=='1'&&p[11]=='1';tag.index=p[10]=='2'&&p[11]=='1';tag.pinky=p[10]=='5'&&p[11]=='1';
            tag.finger=p[10]-'1';tag.segment=p[11]-'1';
        }else tag.side=-1;
        return tag;
    }
    if(!strncmp(p,"upperarm",8)){tag.part=Upper;tag.joint=!strcmp(p,"upperarm");}
    else if(!strncmp(p,"forearm",7)){tag.part=Lower;tag.joint=!strcmp(p,"forearm");}
    else if(!strcmp(p,"hand")){tag.part=Hand;tag.joint=true;}
    else if(!strncmp(p,"finger",6)||!strncmp(p,"thumb",5)||!strncmp(p,"index",5)||!strncmp(p,"middle",6)||!strncmp(p,"ring",4)||!strncmp(p,"pinky",5)){
        tag.part=Hand;tag.thumb=!strcmp(p,"finger0")||!strcmp(p,"thumb")||!strcmp(p,"thumb1");tag.index=!strcmp(p,"finger1")||!strcmp(p,"index")||!strcmp(p,"index1");tag.pinky=!strcmp(p,"finger4")||!strcmp(p,"pinky")||!strcmp(p,"pinky1");
    }else tag.side=-1;
    return tag;
}
} }
