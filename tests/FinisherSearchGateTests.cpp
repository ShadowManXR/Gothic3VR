#include "FinisherSearchGate.h"
#include <cstdio>
using diagnostic::combat::FinisherSearchGate;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    FinisherSearchGate gate;
    for(unsigned ms=1000;ms<2000;ms+=10)if(gate.Due(1,2,3,ms,false))gate.Complete(false,ms);
    CHECK(gate.searches==13&&gate.skipped==87);
    CHECK(gate.Due(1,2,3,1991,true));gate.Complete(true,1991);
    CHECK(!gate.Due(1,2,3,1992,false));
    CHECK(!gate.Due(1,2,3,1993,false));
    CHECK(!gate.Due(1,2,3,1994,false));
    CHECK(gate.Due(1,4,3,1994,false));gate.Complete(false,1994);
    CHECK(gate.Due(9,4,3,1995,false));gate.Complete(false,1995);
    CHECK(gate.Due(9,4,5,1996,false));gate.Complete(false,1996);
    CHECK(gate.Due(9,4,5,100,false));gate.Complete(false,100);
    CHECK(!gate.Due(0,4,5,101,false)&&gate.searches==0);
    CHECK(gate.Due(9,4,5,102,false));gate.Complete(false,102);gate.Reset();
    CHECK(gate.Due(9,4,5,103,false));
    CHECK(gate.Pressed(true)&&!gate.Pressed(true)&&!gate.Pressed(false)&&gate.Pressed(true));
    gate.Reset();CHECK(gate.Pressed(true));
    for(unsigned ms=1000;ms<2000;ms+=10)if(gate.Due(1,2,3,ms,false))gate.Complete(true,ms);
    CHECK(gate.searches==13&&gate.skipped==87); // positive focus is revalidated through the live ROI hint
    puts("PASS full ROI cadence for hit/miss, execution press edge, immediate forced refresh and equipment/session/load resets");return 0;
}
