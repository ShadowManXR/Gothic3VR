#include "CombatMovementRecovery.h"
#include <cstdio>
#include <initializer_list>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
CombatMoveState Idle(){CombatMoveState s;s.player=1;s.movement=2;s.routine=3;s.allowed=s.combat=s.idle=s.physicalMelee=true;
    s.mode=4;s.action=0;s.animation=2;s.combatCallback=0x1696e0;s.instructionAction=2;return s;}
int main(){
    CombatMovementRecovery recovery;auto s=Idle();
    CHECK(CombatMovementRecovery::Classify(s)==CombatMoveRepair::Orphan);
    for(unsigned t=1000;t<1300;t+=20)CHECK(recovery.Step(s,t)==CombatMoveRepair::None);
    CHECK(recovery.Step(s,1300)==CombatMoveRepair::Orphan);
    for(unsigned t=1320;t<5000;t+=20)CHECK(recovery.Step(s,t)==CombatMoveRepair::None); // no repeated mutations
    s.combat=false;CHECK(recovery.Step(s,5000)==CombatMoveRepair::None);s.combat=true;
    s.callback=s.combatCallback;
    for(unsigned t=5020;t<5320;t+=20)CHECK(recovery.Step(s,t)==CombatMoveRepair::None);
    CHECK(recovery.Step(s,5320)==CombatMoveRepair::PhysicalAttack);
    // Pauses, world/player replacement, unavailable input and state changes
    // must restart the window instead of instantly cancelling a new action.
    recovery.Reset();CHECK(recovery.Step(s,0)==CombatMoveRepair::None);
    CHECK(recovery.Step(s,3000)==CombatMoveRepair::None);
    auto changed=s;changed.player=10;CHECK(recovery.Step(changed,3020)==CombatMoveRepair::None);
    for(unsigned t=3040;t<3300;t+=20)CHECK(recovery.Step(changed,t)==CombatMoveRepair::None);
    changed.allowed=false;CHECK(recovery.Step(changed,3300)==CombatMoveRepair::None);
    changed.allowed=true;CHECK(recovery.Step(changed,3320)==CombatMoveRepair::None);
    // Never interrupt hit/parry reactions, KO, dying, get-up, finishers, aim,
    // reload/casting, other instruction callbacks or native special movement.
    for(int action=0;action<=112;++action){auto n=s;n.action=action;
        if(action!=0)CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::None);}
    for(int action=15;action<=112;++action){auto n=s;n.instructionAction=action;CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::None);}
    for(int action=1;action<=14;++action){auto n=s;n.instructionAction=action;CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::PhysicalAttack);}
    for(int mode:{0,6,7,8,9,10,11,12,13,14,15}){auto n=s;n.mode=mode;CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::None);}
    auto n=s;n.callback=99;CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::None);
    n=s;n.physicalMelee=false;CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::None);
    n=s;n.idle=false;CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::None);
    n=s;n.animation=17;CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::None);
    n=s;n.combatCallback=0;CHECK(CombatMovementRecovery::Classify(n)==CombatMoveRepair::None);
    recovery.Reset();CHECK(recovery.Step(s,1000)==CombatMoveRepair::None);CHECK(recovery.Step(s,900)==CombatMoveRepair::None);
    puts("PASS combat movement orphan/physical-attack recovery with native reaction and lifecycle ownership preserved");return 0;
}
