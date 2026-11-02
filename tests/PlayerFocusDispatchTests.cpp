#include "PlayerFocusDispatch.h"
#include <cstdio>
using diagnostic::PlayerFocusDispatch;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    PlayerFocusDispatch gate;unsigned resolves=0;uintptr_t component=123;
    auto resolve=[&](){++resolves;return component;};
    for(unsigned frame=1;frame<=100;++frame){
        for(unsigned npc=1000;npc<2000;++npc)CHECK(!gate.Matches(npc,1,frame,resolve));
        CHECK(gate.Matches(123,1,frame,resolve));
    }
    CHECK(resolves==100); // 100,100 native hook calls, only100 validated component resolutions
    component=456;CHECK(gate.Matches(456,2,100,resolve)&&resolves==101); // transformed player, same frame
    CHECK(!gate.Matches(123,2,100,resolve)&&resolves==101);
    component=789;CHECK(gate.Matches(789,2,101,resolve)&&resolves==102); // replaced component, next update
    CHECK(!gate.Matches(789,0,101,resolve)&&resolves==102);
    CHECK(gate.Matches(789,2,101,resolve)&&resolves==103);
    gate.Reset();component=321;CHECK(gate.Matches(321,2,101,resolve)&&resolves==104); // reload reuses owner/frame
    CHECK(!gate.Matches(0,2,102,resolve)&&resolves==104);
    component=0;CHECK(!gate.Matches(321,2,102,resolve)&&resolves==105); // unavailable property
    CHECK(!gate.Matches(1000,2,102,resolve)&&resolves==105);
    puts("PASS crowd hook dispatch100100->100 resolutions; player/form/component/frame changes, missing owner/component and world-reset address reuse");return 0;
}
