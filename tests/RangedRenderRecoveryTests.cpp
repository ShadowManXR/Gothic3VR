#include "RangedRenderRecovery.h"
#include "RangedFaultRecovery.h"
#include "TrackedPalette.h"
#include <cstdio>
#include <cstring>
int main(){
    using namespace diagnostic;using namespace ranged;
    int failures=0;auto check=[&](bool ok,char const* text){printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failures;};
    Retry retry;Identity bow{1,2,3},crossbow{1,4,3};
    check(retry.Observe(bow,1000),"first equipped bow can bind");retry.Fail(1000);
    check(!retry.Observe(bow,1100)&&!retry.Observe(bow,1499)&&retry.Observe(bow,1500),"transient prepare fault retries automatically after 500ms, without restarting game");
    retry.Fail(1500);check(retry.Observe(crossbow,1501),"switching from failed bow to crossbow does not carry a fault latch");
    retry.Fail(1501);check(retry.Observe({9,4,3},1502),"new saved player identity can immediately bind even when weapon address is reused");
    retry.Fail(1502);check(retry.Observe({9,4,8},1503),"a new runtime session is not poisoned by the old render binding");
    retry.Fail(1503);check(!retry.Observe({},1504)&&retry.Observe({9,4,8},1505),"invalid per-frame sample drops identity; re-equipping same addresses can recover");
    retry.Fail(1505);check(retry.Observe({9,4,8},12),"clock reversal does not permanently suspend tracking");
    retry.Success();check(retry.Observe({9,4,8},13),"successful pose clears transient failure");
    PaletteOwner original{11,12,13,14,7};
    check(original.Same(original)&&!original.Same({11,12,99,14,7})&&!original.Same({11,12,13,99,7})&&!original.Same({11,12,13,14,8}),"palette restores only to the same actor, node storage and count, not a reused factory");
    arms::Mat native[7],replacement[7];arms::Position(native[0],{1,2,3});arms::Position(replacement[0],{40,50,60});
    TrackedPalette saved;check(saved.Begin(native,7),"borrow native bow palette");arms::Position(native[0],{70,80,90});
    auto untouched=replacement[0];check(!saved.Restore(replacement,7)&&!memcmp(&untouched,&replacement[0],sizeof(untouched))&&!saved.address,"actor replacement discards old snapshot without copying it into new actor storage");
    check(saved.Begin(replacement,7),"new equipment can borrow palette after discarded transition");
    arms::Position(replacement[0],{90,90,90});check(saved.Restore(replacement,7)&&!memcmp(&untouched,&replacement[0],sizeof(untouched)),"recovered binding restores the replacement actor's own native palette");
    FaultRecovery shooting;check(shooting.Observe(bow,2000),"shooting validates the current native identity");shooting.Fail(2000);
    check(!shooting.Observe(bow,2100)&&!shooting.Observe({},2200)&&!shooting.Observe(bow,2499),"missing/stale sample cannot bypass shooting retry cooldown");
    check(shooting.Observe(crossbow,2500)&&!shooting.Failed(),"bow failure does not poison a different current crossbow");shooting.Fail(2500);
    check(!shooting.Observe(crossbow,2999)&&shooting.Observe(crossbow,3000),"same ranged weapon can recover after bounded delay without restarting");
    shooting.Fail(3100);check(shooting.Observe({9,4,3},3101),"new player after save load does not inherit old ranged shooting failure");
    return failures?1:0;
}
