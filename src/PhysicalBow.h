#pragma once
#include "TrackedArmsMath.h"
#include "BowCharge.h"

namespace diagnostic { namespace bow {
using arms::Vec;using arms::Mat;
struct Sample {
    uintptr_t player=0,weapon=0;uint64_t time=0,session=0;float units=100;
    Mat left,right,leftAim,drawAim;Vec trackingLeft,trackingRight,nockLocal,launchNock,launchDirection;bool valid=false,trigger=false,nativeNock=false,rightTracked=false,crossbow=false,drawAimValid=false;unsigned aimHand=0;
};
struct Shot {Vec origin,direction;float power=0;bool fire=false;};
inline Vec StringRest(Sample const& s){return arms::Position(s.left)+arms::Vector(s.leftAim,s.nockLocal);}
inline Mat const& DrawAxis(Sample const& s){return s.drawAimValid?s.drawAim:s.leftAim;}
inline Vec DrawRest(Sample const& s){return arms::Position(s.left)+arms::Vector(DrawAxis(s),s.nockLocal);}
inline bool Aim(Sample const& s,Vec& origin,Vec& direction,float& distance){
    if(!s.valid||!arms::Affine(s.left)||!arms::Affine(s.leftAim)||!std::isfinite(s.units)||s.units<=0)return false;
    origin=StringRest(s);direction=arms::Vector(s.leftAim,{0,0,1});
    distance=s.rightTracked?arms::Length(origin-arms::Position(s.right))/s.units:0;
    // The shaft runs from the native string nock past the bow's left edge.
    // Rendered arrow, sight and projectile share that evaluated geometry.
    if(s.nativeNock){if(!arms::Finite(s.launchNock))return false;origin=s.launchNock;direction=s.launchDirection;}
    // Sight, visible shaft and projectile share the evaluated bow geometry.
    // Losing the string hand still leaves a usable left-hand display pose.
    return std::isfinite(distance)&&arms::Unit(direction);
}
inline bool FocusAim(Sample const& sample,WorldAim& aim){
    Vec origin,direction;float separation;
    if(!Aim(sample,origin,direction,separation))return false;
    // The evaluated launch direction may include the string hand or two-hand
    // crossbow support. The uncorrected controller matrix is not that ray.
    Mat frame;
    if(!arms::BodyFrame(direction,arms::Vector(sample.leftAim,{0,1,0}),frame)&&
       !arms::BodyFrame(direction,std::fabs(direction.y)<.9f?Vec{0,1,0}:Vec{0,0,1},frame))return false;
    Rotation3 rotation;
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)rotation.m[row*3+col]=frame.m[col*4+row];
    WorldAim result;NativeCameraEuler(rotation,result.rotation);
    result.origin[0]=origin.x;result.origin[1]=origin.y;result.origin[2]=origin.z;
    result.direction[0]=direction.x;result.direction[1]=direction.y;result.direction[2]=direction.z;
    for(float v:result.rotation)if(!std::isfinite(v))return false;
    aim=result;return true;
}
// The same calibrated aim feeds native focus, the stereo reticle and shots.
// A bounded string-hand correction can be applied after grip/IK resolution.
inline bool TrackedAim(ControllerFrame const& frame,Rotation3 const& native,float const (&base)[3],Orientation center,float const (&origin)[3],float units,float height,int const* hand,int const* angles,Mat& grip,Mat& aim,unsigned side=0){
    if(side>1||!frame.hands[side].aim.tracked||!arms::GripWorld(frame.hands[side].grip,native,base,center,origin,units,height,grip)||
       !arms::GripWorld(frame.hands[side].aim,native,base,center,origin,units,height,aim))return false;
    Mat inverse;if(!arms::Inverse(grip,inverse))return false;Mat relative=arms::Multiply(aim,inverse);
    if(!arms::CalibrateGrip(grip,side,hand,units))return false;
    aim=arms::Multiply(relative,grip);
    float r=.01745329252f,radians[3]{angles[0]*r,angles[1]*r,angles[2]*r};
    aim=arms::Multiply(arms::Rotation(NativeCameraRotation(radians)),aim);arms::Position(aim,arms::Position(grip));return arms::Affine(aim);
}
// Native bow attachment translation belongs to the animated carry pose.
// Anchor the mesh centre/grip region directly at the tracked left grip instead.
inline bool AnchorMesh(Vec minimum,Vec maximum,Mat const& orientation,Vec grip,Mat& pose){
    if(!arms::Finite(minimum)||!arms::Finite(maximum)||!arms::Finite(grip)||!arms::Affine(orientation)||minimum.x>maximum.x||minimum.y>maximum.y||minimum.z>maximum.z||arms::Length(maximum-minimum)<.001f)return false;
    pose=orientation;arms::Position(pose,grip-arms::Vector(pose,(minimum+maximum)*.5f));return arms::Affine(pose);
}
inline Mat ArrowPose(Vec origin,Vec direction,Vec up){
    Mat result;arms::BodyFrame(direction,up,result);arms::Position(result,origin);return result;
}
inline Mat MeshPose(Mat const& aim,bool crossbow=false){
    // Captured Gothic bow bounds: Z spans the limbs; Y is the thin plane.
    // Model +X points forward, +Z runs up the bow; model origin is the grip.
    Mat axes;if(crossbow){axes.m[0]=-1;axes.m[10]=-1;}else {axes.m[0]=0;axes.m[1]=0;axes.m[2]=1;axes.m[4]=1;axes.m[5]=0;axes.m[6]=0;axes.m[8]=0;axes.m[9]=1;axes.m[10]=0;}
    return arms::Multiply(axes,aim);
}
inline Mat AnchoredMesh(Mat const& aim,Vec modelGrip,bool crossbow=false){Mat pose=MeshPose(aim,crossbow);arms::Position(pose,arms::Position(aim)-arms::Vector(pose,modelGrip));return pose;}
inline bool PalmLocal(Mat const& bindHand,Vec index,Vec pinky,Vec& local){
    Mat inverse;if(!arms::Finite(index)||!arms::Finite(pinky)||!arms::Inverse(bindHand,inverse))return false;
    Vec knuckles=(index+pinky)*.5f,offset=knuckles-arms::Position(bindHand);
    if(arms::Length(offset)<.001f)return false;
    local=arms::Point(inverse,arms::Position(bindHand)+offset*.5f);return arms::Finite(local);
}
inline Mat ResolvedPalm(Mat const& pose,Mat const& calibration,Vec wrist,Vec palmLocal){
    Mat hand=arms::Multiply(calibration,pose);arms::Position(hand,wrist);
    Mat out=pose;arms::Position(out,arms::Point(hand,palmLocal));return out;
}
inline Vec GripContactOffset(int const* offsets,float units,unsigned hand=0){
    // In the calibrated left-hand grip frame, fingers run -Y and the palm
    // faces +X. Offsets belong to the bow contact, never to the hand IK target.
    return {(hand?-1:1)*offsets[1]*.01f*units,-offsets[0]*.01f*units,offsets[2]*.01f*units};
}
inline Mat ResolvedBowGrip(Mat const& pose,Mat const& calibration,Vec wrist,Vec palmLocal,int const* offsets,float units,unsigned hand=0){
    Mat out=ResolvedPalm(pose,calibration,wrist,palmLocal);arms::Position(out,arms::Position(out)+arms::Vector(pose,GripContactOffset(offsets,units,hand)));return out;
}
// Standard and elemental arrow meshes share this nock position.
inline constexpr float DefaultArrowTail=-94.02172f;
inline float AmmoTail(float meshMinZ,float meshMaxZ,float entityMinZ,float entityMaxZ,float fallback){
    if(std::isfinite(meshMinZ)&&std::isfinite(meshMaxZ)&&meshMinZ<meshMaxZ)return meshMinZ;
    // Combined bounds can include a light or an effect. Use them only
    // when they fit within a plausible ammo model (game units are centimeters).
    if(entityMinZ>=-200.f&&entityMinZ<entityMaxZ&&entityMaxZ<=200.f&&entityMaxZ-entityMinZ<=200.f)
        return entityMinZ;
    return fallback;
}
inline Mat NockedArrow(Vec nock,Vec direction,Vec up,float tail){Mat pose=ArrowPose(nock,direction,up);arms::Position(pose,nock-arms::Vector(pose,{0,0,tail}));return pose;}
// Terminal finger bones mark the last knuckle. Continue its native bind axis
// through the distal phalanx, then let the current finger rotation pose it.
inline bool StringFingerEnd(Mat const& middleBind,Mat const& tipBind,Vec& end){
    Mat inverse;if(!arms::Affine(middleBind)||!arms::Inverse(tipBind,inverse))return false;
    end=arms::Vector(inverse,arms::Position(tipBind)-arms::Position(middleBind))*.7f;
    return arms::Finite(end)&&arms::Length(end)>.0001f;
}
// Use the CURRENT posed thumb/index tips in hand-local space, not the palm
// centre from the open bind pose. The rigid hand delta then carries both tips
// to the string even when the native bow animation curls the fingers.
inline bool StringFingerContact(Mat const& hand,Mat const& thumb,Vec thumbEnd,
    Mat const& index,Vec indexEnd,Vec& contact){
    Mat inverse;if(!arms::Affine(thumb)||!arms::Affine(index)||!arms::Finite(thumbEnd)||!arms::Finite(indexEnd)||!arms::Inverse(hand,inverse))return false;
    contact=arms::Point(inverse,(arms::Point(thumb,thumbEnd)+arms::Point(index,indexEnd))*.5f);
    return arms::Finite(contact)&&arms::Length(contact)>.0001f;
}
// The visible string can trail physical draw while native skill-based charge
// catches up. Attach only the rendered finger contact to its nock; never feed
// this visual constraint back into the tracked sample used for charge/aim.
inline bool StringHandGrip(Sample const& sample,bool pulling,bool geometryReady,Mat const& grip,
    Mat const& calibration,Vec contactLocal,Mat& attached,Vec& wrist){
    if(!pulling||!geometryReady||!sample.valid||sample.crossbow||sample.aimHand!=0||
       !sample.trigger||!sample.rightTracked||!sample.nativeNock||!sample.player||!sample.weapon||
       !arms::Finite(sample.launchNock)||!arms::Affine(grip)||!arms::Affine(calibration)||!arms::Finite(contactLocal))return false;
    attached=grip;arms::Position(attached,sample.launchNock);
    wrist=arms::WristAtPalm(attached,calibration,contactLocal);return arms::Finite(wrist);
}
class Draw {
    Sample previous;bool havePrevious=false,armed=false,previousTrigger=false;
    uint64_t started=0,cooldown=0;float brace=0;Vec lastOrigin,lastDirection;float lastPower=0;
    Charge charge;
public:
    bool pulling=false;float drawMeters=0,power=0;
    void Cancel(){havePrevious=armed=previousTrigger=pulling=false;drawMeters=power=0;started=0;lastPower=0;charge.Reset();}
    Shot Step(Sample const& s,bool allowed,uint64_t now,float fullDraw=.5f,float grabRadius=.2f,double fullChargeSeconds=0,double scaledSeconds=0,bool unrestricted=false){
        Shot out;Vec origin,direction;float distance=0;
        bool fresh=s.valid&&s.rightTracked&&s.player&&s.weapon&&now>=s.time&&now-s.time<=100&&
            arms::Finite(s.trackingLeft)&&arms::Finite(s.trackingRight)&&arms::Affine(s.left)&&arms::Affine(s.right)&&arms::Affine(DrawAxis(s))&&std::isfinite(s.units)&&s.units>0;
        if(!allowed||!fresh){Cancel();return out;}
        if(havePrevious&&(s.player!=previous.player||s.weapon!=previous.weapon||s.session!=previous.session||s.time<previous.time||s.time-previous.time>100||
            arms::Length(s.trackingLeft-previous.trackingLeft)>.4f||arms::Length(s.trackingRight-previous.trackingRight)>.4f)){Cancel();}
        if(havePrevious&&s.time==previous.time)return out;
        previous=s;havePrevious=true;
        if(!s.trigger)armed=true;
        bool pressed=s.trigger&&!previousTrigger,released=!s.trigger&&previousTrigger;previousTrigger=s.trigger;
        if(pressed&&armed&&now>=cooldown){
            distance=arms::Length(DrawRest(s)-arms::Position(s.right))/s.units;
            if(std::isfinite(distance)&&distance<=std::clamp(grabRadius,.05f,.3f)){
                Vec direction=arms::Vector(DrawAxis(s),{0,0,1});if(!arms::Unit(direction))return out;
                pulling=true;started=s.time;brace=arms::Dot(DrawRest(s)-arms::Position(s.right),direction)/s.units;drawMeters=power=lastPower=0;charge.Reset();
            }
            armed=false;
        }
        if(!pulling)return out;
        bool aim=Aim(s,origin,direction,distance)&&distance<1.5f;
        if(!aim){
            if(s.trigger&&arms::Length(arms::Position(s.left)-arms::Position(s.right))<.001f*s.units)return out;
            Cancel();return out;
        }
        if(s.trigger){
            Vec drawDirection=arms::Vector(DrawAxis(s),{0,0,1});if(!arms::Unit(drawDirection)){Cancel();return out;}
            drawMeters=std::max(0.f,arms::Dot(DrawRest(s)-arms::Position(s.right),drawDirection)/s.units-brace);
            power=charge.Step(drawMeters/std::clamp(fullDraw,.2f,.8f),pressed?0:scaledSeconds,fullChargeSeconds);
            lastOrigin=s.nativeNock&&arms::Finite(s.launchNock)?s.launchNock:origin;lastDirection=direction;lastPower=power;return out;
        }
        // Use the last held pose: releasing the trigger can slightly twitch the hand.
        if(released&&s.time>=started+(unrestricted?0:100)&&drawMeters>=.12f&&lastPower>0){out={lastOrigin,lastDirection,lastPower,true};cooldown=now+(unrestricted?0:300);}
        pulling=false;drawMeters=power=0;return out;
    }
};
}}
