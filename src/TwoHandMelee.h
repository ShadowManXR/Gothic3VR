#pragma once
#include "PhysicalMelee.h"
#include "WeaponTypes.h"
#include "TrackingMath.h"
namespace diagnostic { namespace melee {
inline bool LocalContact(Vec lo,Vec hi,float units,int type,Blade& out){
    if(!PhysicalMeleeType(type)||!LocalBlade(lo,hi,units,out,TwoHandType(type)?3.5f:2.2f,TwoHandType(type)?.25f:.08f))return false;
    // A fighting staff can strike with either end, unlike a sword's hilt.
    if(WeaponType(type)==WeaponKind::Staff){Vec d=hi-lo;int axis=d.y>d.x?1:0;if(d.z>(axis?d.y:d.x))axis=2;
        // A wide/asymmetric blade head does not define the shaft center.
        out.root=out.tip={std::clamp(0.f,lo.x,hi.x),std::clamp(0.f,lo.y,hi.y),std::clamp(0.f,lo.z,hi.z)};
        if(axis==0){out.root.x=lo.x;out.tip.x=hi.x;}else if(axis==1){out.root.y=lo.y;out.tip.y=hi.y;}else{out.root.z=lo.z;out.tip.z=hi.z;}}
    return true;
}
inline bool HandleSegment(Vec lo,Vec hi,float units,int type,Blade& out){
    Blade contact;if(!TwoHandType(type)||!LocalContact(lo,hi,units,type,contact))return false;
    Vec d=hi-lo;int axis=d.y>d.x?1:0;if(d.z>(axis?d.y:d.x))axis=2;
    float low[3]{lo.x,lo.y,lo.z},high[3]{hi.x,hi.y,hi.z},root[3]{contact.root.x,contact.root.y,contact.root.z},tip[3]{contact.tip.x,contact.tip.y,contact.tip.z};
    float a=low[axis]+.02f*units,b=high[axis]-.02f*units;
    if(WeaponType(type)!=WeaponKind::Staff){bool forward=tip[axis]>root[axis];
        if(type==3){if(forward)b=std::min(b,root[axis]-.02f*units);else a=std::max(a,root[axis]+.02f*units);}
        else {if(forward)b=std::min(b,high[axis]*.7f);else a=std::max(a,low[axis]*.7f);}}
    if(!std::isfinite(a)||!std::isfinite(b)||b-a<.04f*units)return false;
    // Mesh pivots lie on the shaft; a wide axe head must not shift the
    // shaft sideways to the midpoint of the whole mesh bounding box.
    float start[3]{std::clamp(0.f,low[0],high[0]),std::clamp(0.f,low[1],high[1]),std::clamp(0.f,low[2],high[2])};
    float end[3]{start[0],start[1],start[2]};start[axis]=a;end[axis]=b;
    out={{start[0],start[1],start[2]},{end[0],end[1],end[2]}};return true;
}
inline Blade WorldSegment(Blade const& local,arms::Mat const& mesh){return {arms::Point(mesh,local.root),arms::Point(mesh,local.tip)};}
// Release by distance from the actual locked contact, not the separation
// between controllers (which differs greatly between a sword and a staff).
inline bool SupportContactInReach(Vec physical,Vec attached,float units,float maximum=.30f){
    return arms::Finite(physical)&&arms::Finite(attached)&&std::isfinite(units)&&units>0&&std::isfinite(maximum)&&maximum>0&&arms::Length(physical-attached)<=maximum*units;
}
// The shaft sits under the knuckle crease of a closed support fist. The
// generic palm landmark is halfway from wrist to knuckles: using it as a
// shaft contact makes the hilt intersect the heel/wrist. Palm-facing depth
// is mirrored, as on the accepted staff grip, independent of its settings.
inline bool SupportFistContact(arms::Mat const& calibration,Vec palmLocal,unsigned side,float units,Vec& out){
    arms::Mat inverse;
    if(side>1||!arms::Finite(palmLocal)||!std::isfinite(units)||units<=0||!arms::Inverse(calibration,inverse))return false;
    out=palmLocal*2+arms::Vector(inverse,{(side?-1.f:1.f)*.02f*units,0,0});return arms::Finite(out);
}
class SupportAttachment {
    Vec point;arms::Mat rotation;bool valid=false;
public:
    void Reset(){valid=false;}
    Vec LocalPoint()const{return point;}
    bool RebindCalibration(arms::Mat const& before,arms::Mat const& after){
        arms::Mat inverse;if(!valid||!arms::Inverse(after,inverse))return false;
        auto next=arms::Multiply(arms::Multiply(inverse,before),rotation);arms::Position(next,{});
        if(!arms::Affine(next))return false;rotation=next;return true;
    }
    bool Capture(arms::Mat const& mesh,Blade const& handle,Vec palm,arms::Mat const& handGrip){
        Reset();arms::Mat inverse;if(!arms::Affine(mesh)||!arms::Affine(handGrip)||!arms::Inverse(mesh,inverse))return false;
        point=arms::Point(inverse,ClosestPoint(WorldSegment(handle,mesh),palm));
        rotation=arms::Multiply(handGrip,inverse);arms::Position(rotation,{});valid=arms::Finite(point)&&arms::Affine(rotation);return valid;
    }
    bool Resolve(arms::Mat const& mesh,arms::Mat const& calibration,Vec palmLocal,arms::Mat& handGrip,Vec& wrist,Vec& contact)const{
        if(!valid||!arms::Affine(mesh)||!arms::Affine(calibration)||!arms::Finite(palmLocal))return false;
        handGrip=arms::Multiply(rotation,mesh);arms::Position(handGrip,{});contact=arms::Point(mesh,point);
        wrist=contact-arms::Vector(arms::Multiply(calibration,handGrip),palmLocal);return arms::Finite(wrist)&&arms::Finite(contact)&&arms::Affine(handGrip);
    }
};
inline bool SupportedGrip(arms::Mat const& primary,Vec axis,Vec secondary,float sign,arms::Mat& out){
    if(!arms::Affine(primary)||!arms::Finite(secondary))return false;arms::Mat rotate;
    if(!arms::Between(axis,(secondary-arms::Position(primary))*sign,rotate))return false;
    out=arms::Multiply(primary,rotate);arms::Position(out,arms::Position(primary));return arms::Affine(out);
}
inline bool SupportedGripAt(arms::Mat const& primary,Vec axis,Vec contact,Vec secondary,float sign,arms::Mat& out){
    auto pivot=primary;arms::Position(pivot,contact);if(!SupportedGrip(pivot,axis,secondary,sign,out))return false;
    arms::Position(out,arms::Position(primary));return true;
}
inline bool SupportOrientation(ControllerFrame const& frame,float sign,Orientation& out){
    auto const& right=frame.hands[1].grip;auto const& left=frame.hands[0].grip;
    Orientation q{right.quaternion[3],right.quaternion[0],right.quaternion[1],right.quaternion[2]};if(!Normalize(q))return false;
    Vec direction{(left.position[0]-right.position[0])*sign,(left.position[1]-right.position[1])*sign,(left.position[2]-right.position[2])*sign};
    auto rotation=arms::Rotation(Matrix(q));arms::Mat turn;
    // Use the same minimal rotation as the rendered grip. A world/up frame
    // becomes singular when a staff points vertically and can flip its roll.
    if(!arms::Between(arms::Vector(rotation,{0,0,-1}),direction,turn))return false;
    auto basis=arms::Multiply(rotation,turn);
    float matrix[12]{basis.m[0],basis.m[4],basis.m[8],0,basis.m[1],basis.m[5],basis.m[9],0,basis.m[2],basis.m[6],basis.m[10],0},values[4];
    if(!TrackingQuaternion(matrix,values))return false;out={values[0],values[1],values[2],values[3]};return true;
}
inline bool PoseOrientation(arms::Mat const& pose,Orientation& out){
    float matrix[12]{pose.m[0],pose.m[4],pose.m[8],0,pose.m[1],pose.m[5],pose.m[9],0,pose.m[2],pose.m[6],pose.m[10],0},q[4];
    if(!arms::Affine(pose)||!TrackingQuaternion(matrix,q))return false;out={q[0],q[1],q[2],q[3]};return Normalize(out);
}
// A rigid two-hand grip has one orientation. Positions choose its direction;
// only twist around that direction is contributed by controller rotations.
// Transport the previous frame, rather than rebuilding it from right wrist
// pitch/yaw or a world-up vector (both can produce flips during a swing).
class CoordinatedGrip {
    arms::Mat pose,previous[2],rightLocalPose;Vec direction,offset,lineCorrection,localShaft;bool shaftGuided=false;uint64_t time=0,worldUpdates=0;bool valid=false;float sign=1;
    static bool Direction(arms::Mat const& left,arms::Mat const& right,float side,Vec& out){out=(arms::Position(left)-arms::Position(right))*side;return arms::Affine(left)&&arms::Affine(right)&&arms::Unit(out);}
    static bool Twist(arms::Mat const& before,arms::Mat const& after,arms::Mat const& transport,Vec axis,float& angle){
        auto moved=arms::Multiply(before,transport);arms::Position(moved,{});auto current=after;arms::Position(current,{});arms::Mat inverse;
        if(!arms::Inverse(moved,inverse))return false;Orientation q;if(!PoseOrientation(arms::Multiply(inverse,current),q))return false;
        if(q.w<0){q.w=-q.w;q.x=-q.x;q.y=-q.y;q.z=-q.z;}
        float along=q.x*axis.x+q.y*axis.y+q.z*axis.z;
        // A 180-degree wrist swing perpendicular to the weapon has no
        // defined twist and must not arbitrarily roll the whole weapon.
        angle=std::fabs(q.w)+std::fabs(along)<.00001f?0.f:2*std::atan2(along,q.w);return std::isfinite(angle);
    }
public:
    void Reset(){valid=false;time=worldUpdates=0;}
    bool Valid()const{return valid;}
    uint64_t ReusedTrackingWorldUpdates()const{return worldUpdates;}
    bool Capture(arms::Mat const& initial,arms::Mat const& left,arms::Mat const& right,uint64_t stamp,float side,Vec const* shaft=nullptr){
        Reset();shaftGuided=shaft!=nullptr;lineCorrection={};arms::Mat inverse;pose=initial;arms::Position(pose,{});
        if(!stamp||!arms::Affine(initial)||!Direction(left,right,side,direction)||!arms::Inverse(pose,inverse))return false;
        if(shaftGuided){
            Vec actual=*shaft;if(!arms::Unit(actual))return false;
            auto rightRotation=right;arms::Position(rightRotation,{});arms::Mat inverseRight;
            if(!arms::Inverse(rightRotation,inverseRight))return false;
            // Controller centres do not lie on the hilt. Preserve the acquired
            // mesh pose, then guide its actual shaft using this fixed offset.
            auto line=(arms::Position(left)-arms::Position(right))*side;
            lineCorrection=arms::Vector(inverseRight,actual*arms::Length(line)-line);direction=actual;
            rightLocalPose=arms::Multiply(pose,inverseRight);localShaft=arms::Vector(inverse,actual);
        }
        offset=arms::Vector(inverse,arms::Position(initial)-arms::Position(right));previous[0]=left;previous[1]=right;sign=side;time=stamp;
        arms::Position(pose,arms::Position(initial));valid=arms::Finite(offset);return valid;
    }
    bool Step(arms::Mat const& left,arms::Mat const& right,uint64_t stamp,arms::Mat& out){
        Vec next;if(!valid||stamp<time||!arms::Affine(left)||!arms::Affine(right)){Reset();return false;}
        if(!Direction(left,right,sign,next)){
            // Coincident controller centres cannot define an axis. Keep the
            // last rigid pose; the actual palm-distance gate still owns release.
            arms::Position(pose,arms::Position(right)+arms::Vector(pose,offset));
            previous[0]=left;previous[1]=right;time=stamp;out=pose;return true;
        }
        if(shaftGuided){next=(arms::Position(left)-arms::Position(right))*sign+arms::Vector(right,lineCorrection);
            if(!arms::Unit(next)){Reset();return false;}}
        // Duplicate XR time does not imply duplicate world-space input: the
        // native player/camera origin can move or turn between XR polls.
        // Only identical transforms can reuse the complete rendered pose.
        if(stamp==time&&!memcmp(&left,&previous[0],sizeof(left))&&!memcmp(&right,&previous[1],sizeof(right))){out=pose;return true;}
        if(stamp==time)++worldUpdates;
        if(shaftGuided){
            // Solve from the acquired dominant-hand frame, not a sequence of
            // transported wrist deltas. Left wrist rotation is never an input.
            // Keep support steering within a 45-degree swing cone so a short
            // hilt cannot lever the right wrist backwards. Moving both hands
            // together (including player yaw) rotates the whole neutral frame.
            auto rightRotation=right;arms::Position(rightRotation,{});
            auto neutral=arms::Multiply(rightLocalPose,rightRotation);
            Vec neutralAxis=arms::Vector(neutral,localShaft);if(!arms::Unit(neutralAxis)){Reset();return false;}
            float cosine=std::clamp(arms::Dot(neutralAxis,next),-1.f,1.f);
            constexpr float limit=.78539816339f;
            if(cosine<std::cos(limit)){
                Vec tangent=next-neutralAxis*cosine;
                if(!arms::Unit(tangent)){
                    // The exactly opposite direction has no preferred turn.
                    // Retain the last constrained side instead of flipping.
                    tangent=direction-neutralAxis*arms::Dot(direction,neutralAxis);
                    if(!arms::Unit(tangent)){next=neutralAxis;}
                    else next=neutralAxis*std::cos(limit)+tangent*std::sin(limit);
                }else next=neutralAxis*std::cos(limit)+tangent*std::sin(limit);
            }
            arms::Mat turn;if(!arms::Between(neutralAxis,next,turn)){Reset();return false;}
            pose=arms::Multiply(neutral,turn);arms::Position(pose,arms::Position(right)+arms::Vector(pose,offset));
            direction=next;previous[0]=left;previous[1]=right;time=stamp;out=pose;return arms::Affine(out);
        }
        arms::Mat transport;if(!arms::Between(direction,next,transport)){Reset();return false;}
        float twists[2];if(!Twist(previous[0],left,transport,next,twists[0])||!Twist(previous[1],right,transport,next,twists[1])){Reset();return false;}
        // Equal twists add, opposing twists cancel. Unwrap around the first
        // controller so values straddling +/-pi cannot average to zero.
        float difference=twists[1]-twists[0];constexpr float pi=3.14159265359f;
        while(difference>pi)difference-=2*pi;while(difference<-pi)difference+=2*pi;
        float half=(twists[0]+difference*.5f)*.5f;Orientation roll{std::cos(half),next.x*std::sin(half),next.y*std::sin(half),next.z*std::sin(half)};
        pose=arms::Multiply(arms::Multiply(pose,transport),arms::Rotation(Matrix(roll)));
        arms::Position(pose,arms::Position(right)+arms::Vector(pose,offset));
        direction=next;previous[0]=left;previous[1]=right;time=stamp;out=pose;return arms::Affine(out);
    }
};
// Begin support at the currently rendered grip/mesh, without rotating it to
// the controller-centre line. Palm/calibration offsets make those axes differ.
// Capture the identical orientation in tracking space for combat, so world
// movement cannot masquerade as a physical swing.
inline bool CaptureHeldGrip(CoordinatedGrip& world,CoordinatedGrip& tracking,
                            arms::Mat const& held,arms::Mat const& worldLeft,arms::Mat const& worldRight,
                            arms::Mat const& rawLeft,arms::Mat const& rawRight,uint64_t stamp,float sign,Vec const* shaft=nullptr){
    world.Reset();tracking.Reset();
    auto rendered=held,controller=worldRight,raw=rawRight;arms::Mat inverse;
    arms::Position(rendered,{});arms::Position(controller,{});arms::Position(raw,{});
    if(!arms::Affine(held)||!arms::Affine(worldRight)||!arms::Affine(rawRight)||!arms::Inverse(controller,inverse))return false;
    auto rawHeld=arms::Multiply(arms::Multiply(rendered,inverse),raw);arms::Position(rawHeld,arms::Position(rawRight));
    Vec rawShaft;if(shaft)rawShaft=arms::Vector(raw,arms::Vector(inverse,*shaft));
    if(world.Capture(held,worldLeft,worldRight,stamp,sign,shaft)&&tracking.Capture(rawHeld,rawLeft,rawRight,stamp,sign,shaft?&rawShaft:nullptr))return true;
    world.Reset();tracking.Reset();return false;
}
inline bool TrackingGrips(ControllerFrame const& frame,arms::Mat (&out)[2]){
    for(unsigned h=0;h<2;++h){auto const& p=frame.hands[h].grip;Orientation q{p.quaternion[3],p.quaternion[0],p.quaternion[1],p.quaternion[2]};
        if(!p.valid||!p.tracked||!Normalize(q))return false;out[h]=arms::Rotation(Matrix(q));arms::Position(out[h],{p.position[0],p.position[1],p.position[2]});if(!arms::Affine(out[h]))return false;}
    return true;
}
class TwoHandGrip {
    uintptr_t weapon=0;uint64_t session=0,time=0;bool armed=false,down=false,toggleMode=false;Vec lastLeft,lastRight;
public:
    bool holding=false;float sign=1;
    void Reset(){weapon=0;session=time=0;armed=down=holding=false;sign=1;lastLeft=lastRight={};}
    bool Update(uintptr_t item,int type,ControllerFrame const& frame,uint64_t originSession,bool allowed,bool freeLeft,
                Vec primary,Vec secondary,Vec axis,float units,float radius,float span,Blade const* handle=nullptr,bool toggle=false){
        bool fresh=allowed&&freeLeft&&TwoHandType(type)&&item&&frame.focused&&frame.timeMs&&
            frame.hands[0].grip.valid&&frame.hands[0].grip.tracked&&frame.hands[1].grip.valid&&frame.hands[1].grip.tracked;
        if(!fresh||!arms::Finite(primary)||!arms::Finite(secondary)||!std::isfinite(units)||units<=0||!arms::Unit(axis)||!std::isfinite(frame.hands[0].squeeze)){Reset();return false;}
        auto const& l=frame.hands[0].grip;auto const& r=frame.hands[1].grip;Vec rawLeft{l.position[0],l.position[1],l.position[2]},rawRight{r.position[0],r.position[1],r.position[2]};
        if(!arms::Finite(rawLeft)||!arms::Finite(rawRight)){Reset();return false;}
        bool discontinuity=time&&(frame.timeMs-time>100||arms::Length(rawLeft-lastLeft)>.4f||arms::Length(rawRight-lastRight)>.4f);
        if(toggleMode!=toggle||item!=weapon||originSession!=session||frame.timeMs<time||(!holding&&discontinuity)){
            Reset();weapon=item;session=originSession;toggleMode=toggle;discontinuity=false;
        }
        time=frame.timeMs;lastLeft=rawLeft;lastRight=rawRight;
        float squeeze=frame.hands[0].squeeze;
        if(holding&&discontinuity){
            // A loading stall is not a release or a queued toggle click.
            down=squeeze>.65f;armed=squeeze<.35f;
            if(!toggle&&squeeze<.35f)holding=false;return holding;
        }
        if(squeeze<.35f){down=false;armed=true;if(!toggle)holding=false;return holding;}
        bool pressed=squeeze>.65f&&!down;if(squeeze>.65f)down=true;
        Vec difference=secondary-primary;float separation=arms::Length(difference)/units;
        if(toggle&&holding&&pressed&&armed){holding=false;armed=false;return false;}
        // Once constrained, controller separation is guidance, not a release
        // gesture. Crossing the hands must not drop an attached support palm.
        if(holding)return true;
        if(!pressed||!armed)return false;armed=false;
        float along=arms::Dot(difference,axis)/units;
        float lateral=arms::Length(difference-axis*(along*units))/units;
        if(separation<.05f)return false;
        if(handle){if(!arms::Finite(handle->root)||!arms::Finite(handle->tip)||arms::Length(ClosestPoint(*handle,secondary)-secondary)/units>radius)return false;}
        else if(along<-.3f||along>span||lateral>radius)return false;
        sign=along>=0?1.f:-1.f;holding=true;return true;
    }
};
}}
