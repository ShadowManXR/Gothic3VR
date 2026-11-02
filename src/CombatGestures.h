#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace diagnostic { namespace combat {
enum class SkillGroup {None,OneHand,Large,Staff};
inline SkillGroup NativeSkillGroup(int type){return type==2?SkillGroup::OneHand:type==12?SkillGroup::Staff:(type==3||type==51||type==52)?SkillGroup::Large:SkillGroup::None;}
inline double ChargeSeconds(int tier){return tier>=3?.45:tier==2?.65:.90;}
class PowerCharge {
    uintptr_t weapon=0;uint64_t session=0,time=0,released=0;double seconds=0;bool armed=false,down=false,ready=false,consumed=false;
public:
    void Reset(){weapon=0;session=time=released=0;seconds=0;armed=down=ready=consumed=false;}
    bool Ready()const{return ready&&!consumed;}
    bool Holding()const{return down&&!consumed;}
    bool Step(uintptr_t item,uint64_t origin,uint64_t stamp,bool held,int tier,bool allowed,double scaledSeconds){
        if(!allowed||!item||!stamp||tier<1||!std::isfinite(scaledSeconds)||scaledSeconds<0){Reset();return false;}
        if(item!=weapon||origin!=session||stamp<time||(time&&stamp-time>150)){Reset();weapon=item;session=origin;}
        if(stamp==time)return false;time=stamp;bool was=Ready();
        if(!held){armed=true;if(down&&ready&&!consumed)released=stamp;else if(!released){seconds=0;ready=false;consumed=false;}down=false;
            if(released&&stamp-released>300){released=0;seconds=0;ready=false;consumed=false;}}
        else if(armed){if(!down){seconds=0;ready=consumed=false;released=0;}down=true;if(!consumed){seconds+=std::min(scaledSeconds,.05);ready=seconds>=ChargeSeconds(tier);}}
        return !was&&Ready();
    }
    bool Consume(){if(!Ready())return false;consumed=true;ready=false;released=0;return true;}
};
class StrokeBudget {
    uint64_t generation=0;bool allowed=false,power=false,paid=false;
public:
    void Reset(){generation=0;allowed=power=paid=false;}
    bool ForStroke(uint64_t stroke)const{return stroke==generation;}
    bool Paid()const{return paid;}
    template<class Spend> bool Decide(uint64_t stroke,bool charged,bool damageReady,bool staminaReady,int normalCost,int powerCost,int stamina,bool unlimited,Spend spend){
        if(!damageReady&&!staminaReady)return false;
        if(stroke!=generation){generation=stroke;paid=false;power=charged&&(unlimited||stamina>=powerCost);allowed=unlimited||stamina>=(power?powerCost:normalCost);}
        if(staminaReady&&!paid){paid=true;int cost=power?powerCost:normalCost;allowed=allowed&&(unlimited||stamina>=cost);if(allowed&&cost>0)allowed=spend(cost);}
        return damageReady&&allowed;
    }
    template<class Spend> bool Begin(uint64_t stroke,bool charged,int normalCost,int powerCost,int stamina,bool unlimited,Spend spend){
        if(stroke==generation)return allowed;
        generation=stroke;power=charged&&(unlimited||stamina>=powerCost);int cost=power?powerCost:normalCost;
        allowed=unlimited||stamina>=cost;
        if(allowed&&cost>0)allowed=spend(cost);return allowed;
    }
    bool Power()const{return allowed&&power;}
};
class ExecutionHold {
    uintptr_t target=0;uint64_t time=0;double seconds=0;bool armed=false,fired=false;
public:
    void Reset(){target=0;time=0;seconds=0;armed=fired=false;}
    bool Holding()const{return seconds>0&&!fired;}
    bool Step(uintptr_t victim,bool held,bool eligible,uint64_t stamp,double dt){
        if(!eligible||!victim||!stamp||!std::isfinite(dt)||dt<0){Reset();return false;}
        if(stamp==time)return false;
        if(stamp<time||(time&&stamp-time>150)||target!=victim){Reset();target=victim;}
        time=stamp;if(!held){armed=true;seconds=0;fired=false;return false;}
        if(!armed||fired)return false;seconds+=std::min(dt,.05);if(seconds<.8)return false;fired=true;return true;
    }
};
inline int QuickAttackCost(bool alternative){return alternative?12:10;}
inline constexpr int PowerAttackCost=20;
} }
