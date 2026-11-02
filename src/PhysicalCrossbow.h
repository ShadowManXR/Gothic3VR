#pragma once
#include "PhysicalBow.h"
#include "TwoHandMelee.h"
namespace diagnostic { namespace crossbow {
// One shot per deliberate press. Holding the trigger, duplicate eye samples,
// tracking loss and reopening a menu never queue a shot after reload.
class Trigger {
    uintptr_t player=0,weapon=0;uint64_t session=0,time=0;bool previous=false,armed=false,have=false;arms::Vec tracking;
public:
    void Cancel(){player=weapon=0;session=time=0;previous=armed=have=false;}
    bow::Shot Step(bow::Sample const& s,bool allowed,uint64_t now,bool reloadReady){
        bow::Shot out;arms::Vec origin,direction;float separation;
        if(!allowed||!s.crossbow||!s.rightTracked||!s.nativeNock||s.aimHand!=1||!s.player||!s.weapon||now<s.time||now-s.time>100||!arms::Finite(s.trackingRight)||!bow::Aim(s,origin,direction,separation)){Cancel();return out;}
        if(!have||s.player!=player||s.weapon!=weapon||s.session!=session||s.time<time||s.time-time>100||arms::Length(s.trackingRight-tracking)>.4f){
            Cancel();player=s.player;weapon=s.weapon;session=s.session;have=true;
        }else if(s.time==time)return out;
        time=s.time;tracking=s.trackingRight;
        if(!s.trigger)armed=true;
        bool pressed=s.trigger&&!previous;previous=s.trigger;
        if(!pressed)return out;
        bool ready=armed&&reloadReady;armed=false;
        if(ready)out={origin,direction,1,true};
        return out;
    }
};
class Support {
    melee::SupportAttachment attachment;uintptr_t weapon=0;
public:
    void Reset(){attachment.Reset();weapon=0;}
    bool Holding(uintptr_t item)const{return item&&weapon==item;}
    bool RebindCalibration(arms::Mat const& before,arms::Mat const& after){return weapon&&attachment.RebindCalibration(before,after);}
    bool Resolve(uintptr_t item,bool held,arms::Mat const& mesh,arms::Vec localContact,
                 arms::Mat const& grip,arms::Mat const& calibration,arms::Vec palmLocal,float units,
                 arms::Mat& pose,arms::Vec& wrist,float lossDistance=.40f){
        if(!item||!held||!std::isfinite(units)||units<=0){Reset();return false;}
        if(weapon!=item)Reset();
        auto rawWrist=arms::WristAtPalm(grip,calibration,palmLocal);auto palm=bow::ResolvedPalm(grip,calibration,rawWrist,palmLocal);
        if(weapon&&!melee::SupportContactInReach(arms::Position(palm),arms::Point(mesh,attachment.LocalPoint()),units,lossDistance)){Reset();return false;}
        if(!weapon){
            auto contact=arms::Point(mesh,localContact);
            if(arms::Length(arms::Position(palm)-contact)>.15f*units||!attachment.Capture(mesh,melee::Blade{localContact,localContact},contact,grip))return false;
            weapon=item;
        }
        arms::Vec contact;if(!attachment.Resolve(mesh,calibration,palmLocal,pose,wrist,contact)){Reset();return false;}return true;
    }
};
// Contact is prepared once before both eyes and every mesh pass. A late draw
// callback cannot reinterpret an older controller timestamp as a grip release.
class FrameSupport {
    Support attachment;arms::Mat pose;arms::Vec wrist;bool valid=false;
public:
    void Reset(){attachment.Reset();valid=false;}
    void Begin(uintptr_t item,bool held){valid=false;if(!held||!attachment.Holding(item))attachment.Reset();}
    bool Holding(uintptr_t item)const{return attachment.Holding(item);}
    bool RebindCalibration(arms::Mat const& before,arms::Mat const& after){valid=false;return attachment.RebindCalibration(before,after);}
    bool Prepare(uintptr_t item,bool held,arms::Mat const& mesh,arms::Vec contact,arms::Mat const& grip,arms::Mat const& calibration,arms::Vec palm,float units,float lossDistance=.40f){
        // The open-hand tracking pivot lies near the wrist when the native
        // carry pose closes the fist. Use the same knuckle/palm contact surface
        // as accepted 2H support, without changing controller calibration.
        arms::Vec supportPalm;
        if(!melee::SupportFistContact(calibration,palm,0,units,supportPalm)){Reset();return false;}
        valid=attachment.Resolve(item,held,mesh,contact,grip,calibration,supportPalm,units,pose,wrist,lossDistance);return valid;
    }
    bool Read(arms::Mat& out,arms::Vec& position)const{if(!valid)return false;out=pose;position=wrist;return true;}
};
} }
