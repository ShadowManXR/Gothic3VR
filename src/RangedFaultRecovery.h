#pragma once
#include "RangedRenderRecovery.h"
namespace diagnostic { namespace ranged {
// Unlike render retry, invalid samples must not erase a shooting fault delay.
// Recovery never queues a shot: the caller resets physical trigger edges.
class FaultRecovery {
    Identity owner;uint64_t failure=0;bool failed=false;
public:
    void Fail(uint64_t now){failure=now;failed=true;}
    bool Observe(Identity key,uint64_t now){
        if(!key.Valid())return false;
        if(!(owner==key)){owner=key;failed=false;}
        if(failed&&now>=failure&&now-failure<500)return false;
        failed=false;return true;
    }
    bool Failed()const{return failed;}
};
}}
