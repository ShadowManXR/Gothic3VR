#pragma once
#include <cstdint>
#include <cstring>
namespace diagnostic {
inline bool UnconsciousPose(int action,int animation){return action==33||action==37||animation==17||animation==20;}
inline bool KnockdownPose(int action,int animation){return (action>=29&&action<=33)||action==37||(animation>=15&&animation<=17)||animation==20;}
class KnockoutCamera {
    uintptr_t actor=0;bool recovering=false;
public:
    void Reset(){actor=0;recovering=false;}
    bool Update(uintptr_t player,int action,int animation,bool down,bool dead){
        if(!player){Reset();return false;}
        if(player!=actor){Reset();actor=player;}
        // Every down/KO and its get-up uses the complete native third-person
        // body. Ordinary upright hit reactions remain first-person.
        if(down||KnockdownPose(action,animation))recovering=true;
        if(recovering&&!down&&!KnockdownPose(action,animation)&&(animation==2||animation==3||animation==4))recovering=false;
        return recovering||dead||action==35||action==36||action==38||(animation>=18&&animation<=21);
    }
};
}
