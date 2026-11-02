#pragma once
#include <cstdint>
namespace diagnostic {
class PressOnce {
    uintptr_t owner=0;uint64_t previous=0;bool armed=false;
public:
    void Reset(){owner=0;previous=0;armed=false;}
    bool Step(uintptr_t actor,bool allowed,bool held,uint64_t now){
        if(!allowed||!actor||!now){Reset();return false;}
        if(owner!=actor||now<previous||(previous&&now-previous>100)){Reset();owner=actor;}
        previous=now;if(!held){armed=true;return false;}
        if(!armed)return false;armed=false;return true;
    }
};
}
