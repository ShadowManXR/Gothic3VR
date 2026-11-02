#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
namespace diagnostic {
inline bool ComfortWalkingMode(int mode){return mode==1||(mode>=2&&mode<=5)||mode==10;}
// Ownership follows actual controller input. A neutral controller cannot stop
// keyboard locomotion that it never started. No room-scale pose enters this state.
class WalkingCommand {
    uintptr_t actor=0;uint64_t session=0,time=0,lastMoving=0;bool active=false,owned=false;
public:
    float x=0,z=0;
    void Reset(){actor=0;session=time=lastMoving=0;active=owned=false;x=z=0;}
    void Update(bool allowed,uintptr_t player,uint64_t origin,uint64_t stamp,float side,float forward){
        if(!allowed||!player||!stamp||!std::isfinite(side)||!std::isfinite(forward)){Reset();return;}
        if(player!=actor||session!=origin||stamp<time||(time&&stamp-time>100)){Reset();actor=player;session=origin;}
        active=std::hypot(side,forward)>.001f;time=stamp;
        // Keep only the last requested direction during native braking. A new
        // command overwrites it immediately; there is no queued stop action.
        if(active){owned=true;lastMoving=stamp;x=side;z=forward;}
    }
    bool Owns(uintptr_t player,uint64_t origin,uint64_t now)const{return player==actor&&origin==session&&now>=time&&now-time<=100&&owned;}
    bool Moving()const{return active;}
    bool Direction(float heading,float (&out)[3])const{
        float n=std::hypot(x,z);if(!lastMoving||!std::isfinite(heading)||n<.001f)return false;
        out[0]=(x*std::cos(heading)+z*std::sin(heading))/n;out[1]=0;out[2]=(-x*std::sin(heading)+z*std::cos(heading))/n;return true;
    }
};
// Restore through the same SDK API even if native processing raises an exception.
template<class Movement> class WalkingProperties {
    Movement* owner=nullptr;float acceleration=0,deceleration=0,inertia=0;bool sensorDirection=false;
public:
    void Begin(Movement* movement,int responseMs){
        if(owner||!movement)return;
        acceleration=movement->GetMoveAcceleration();deceleration=movement->GetMoveDecceleration();inertia=movement->GetAniVelocityInertia();
        float maximum=std::max({std::fabs(movement->GetForwardSpeedMax()),std::fabs(movement->GetStrafeSpeedMax()),std::fabs(movement->GetBackwardSpeedMax())});
        if(!std::isfinite(acceleration)||!std::isfinite(deceleration)||!std::isfinite(inertia)||!std::isfinite(maximum))return;
        float response=std::max(.02f,float(std::clamp(responseMs,0,150))*.001f);
        sensorDirection=movement->GetSensorAffectsDirection();owner=movement;movement->SetMoveAcceleration(std::max(acceleration,maximum/response));movement->SetMoveDecceleration(-std::max(std::fabs(deceleration),maximum/response));movement->SetAniVelocityInertia(0);movement->SetSensorAffectsDirection(false);
    }
    void Restore(){if(!owner)return;auto* movement=owner;owner=nullptr;movement->SetMoveAcceleration(acceleration);movement->SetMoveDecceleration(deceleration);movement->SetAniVelocityInertia(inertia);movement->SetSensorAffectsDirection(sensorDirection);}
};
}
