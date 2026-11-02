#pragma once
#include <cstdint>
namespace diagnostic {
// Emits one pulse on a deliberate second press, never on a held button. A
// short stable release rejects analog bounce; context loss discards the pair.
class DoubleTapPress {
    uint64_t previous=0,first=0,released=0;bool armed=false;
public:
    void Reset(){previous=first=released=0;armed=false;}
    bool Step(bool allowed,bool down,uint64_t now){
        if(!allowed||!now){Reset();return false;}
        if(previous&&(now<previous||now-previous>100))Reset();
        previous=now;
        if(first&&now-first>450)first=0;
        if(!down){if(!released)released=now;if(now-released>=30)armed=true;return false;}
        released=0;if(!armed)return false;armed=false;
        if(first&&now-first>=80){first=0;return true;}
        first=now;return false;
    }
};
}
