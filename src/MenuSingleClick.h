#pragma once
#include "PressOnce.h"
namespace diagnostic {
class MenuSingleClick {
    PressOnce press;uint64_t until=0,previous=0;
public:
    void Reset(){press.Reset();until=previous=0;}
    bool Step(bool allowed,bool held,uint64_t now){
        if(!allowed||!now){Reset();return false;}
        if(previous&&(now<previous||now-previous>100))Reset();
        previous=now;
        if(press.Step(1,true,held,now))until=now+80;
        return now&&now<until;
    }
};
}
