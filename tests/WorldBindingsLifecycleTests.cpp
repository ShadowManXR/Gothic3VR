#include "WorldBindingsLifecycle.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    WorldBindingsLifecycle life;bool entityAlive=true;unsigned nativeReads=0,restores=0;bool borrowed=true;
    auto tick=[&](bool running){if(!life.EntityUpdates(running))return true;++nativeReads;return entityAlive;};
    CHECK(tick(true));life.BeginStop();
    // Restore while the old world is valid, invalidate ownership, then let
    // native teardown free it. Reentrant callbacks cannot inspect its entities.
    if(borrowed&&entityAlive){++restores;borrowed=false;}entityAlive=false;
    CHECK(tick(true)&&nativeReads==1&&restores==1);life.BeginStop();life.EndStop();CHECK(life.Blocked());
    life.EndStop();CHECK(!life.Blocked()&&tick(false)&&nativeReads==1);
    entityAlive=true;CHECK(tick(true)&&nativeReads==2); // next save/session works
    life.BeginShutdown();entityAlive=false;CHECK(tick(true)&&tick(false)&&nativeReads==2);
    life.EndStop();CHECK(life.Blocked());
    puts("PASS restore-before-native-stop, nested teardown reentry, no entity access after unload, new-world resume and irreversible engine shutdown");return 0;
}
