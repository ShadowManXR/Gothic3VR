#include "MeleeSpecialActions.h"
#include "CombatGestures.h"
#include <cstdio>
using namespace diagnostic::combat;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    CHECK(NativeSkillGroup(2)==SkillGroup::OneHand&&NativeSkillGroup(12)==SkillGroup::Staff&&NativeSkillGroup(51)==SkillGroup::Large&&NativeSkillGroup(52)==SkillGroup::Large&&NativeSkillGroup(3)==SkillGroup::Large&&NativeSkillGroup(5)==SkillGroup::None);
    PowerCharge charge;CHECK(!charge.Step(1,1,1000,true,1,true,.01)&&!charge.Ready());charge.Step(1,1,1010,false,1,true,.01);
    unsigned ready=0;for(uint64_t t=1020;t<=2020;t+=10)ready+=charge.Step(1,1,t,true,1,true,.01);CHECK(ready==1&&charge.Ready());CHECK(charge.Consume()&&!charge.Consume());
    for(uint64_t t=2030;t<2500;t+=10)CHECK(!charge.Step(1,1,t,true,1,true,.01)&&!charge.Ready());
    charge.Step(1,1,2500,false,3,true,.01);for(uint64_t t=2510;t<=3000;t+=10)charge.Step(1,1,t,true,3,true,.01);CHECK(charge.Ready());
    charge.Step(1,1,3010,false,3,true,.01);CHECK(charge.Ready());charge.Step(1,1,3100,false,3,true,.01);CHECK(charge.Consume());
    charge.Step(1,1,3110,false,0,true,.01);CHECK(!charge.Ready());
    StrokeBudget budget;int spent=0;auto spend=[&](int value){spent+=value;return true;};
    CHECK(budget.Begin(1,false,12,20,100,false,spend)&&spent==12);for(int i=0;i<30;++i)CHECK(budget.Begin(1,true,12,20,100,false,spend));CHECK(spent==12&&!budget.Power());
    CHECK(budget.Begin(2,true,12,20,100,false,spend)&&budget.Power()&&spent==32);
    CHECK(!budget.Begin(3,true,12,20,0,false,spend)&&spent==32);CHECK(budget.Begin(4,true,12,20,15,false,spend)&&!budget.Power()&&spent==44);
    ExecutionHold finish;CHECK(!finish.Step(7,true,true,1000,.01));finish.Step(7,false,true,1010,.01);
    unsigned commits=0;for(uint64_t t=1020;t<=2200;t+=10)commits+=finish.Step(7,true,true,t,.01);CHECK(commits==1);CHECK(!finish.Step(8,true,true,2210,.01));
    CHECK(!finish.Step(8,false,false,2220,.01));CHECK(QuickAttackCost(false)==10&&QuickAttackCost(true)==12&&PowerAttackCost==20);
    diagnostic::melee::Blade held{{0,0,0},{0,0,1}},stab{{0,0,.04f},{0,0,1.04f}},cut{{.04f,0,0},{.04f,0,1}};
    CHECK(diagnostic::melee::ForwardThrust(held,stab)&&!diagnostic::melee::ForwardThrust(stab,held)&&!diagnostic::melee::ForwardThrust(held,cut)&&!diagnostic::melee::ForwardThrust(held,held));
    CHECK(!diagnostic::melee::ForwardThrust(held,{{0,0,0},{.2f,0,.98f}})); // wrist swing alone is not a native stealth thrust
    CHECK(diagnostic::melee::PhysicalAttackAction(true,true,2,2,0)==11);
    CHECK(diagnostic::melee::PhysicalAttackAction(false,true,2,2,0)==11&&diagnostic::melee::PhysicalAttackAction(true,false,2,2,0)==2);
    CHECK(diagnostic::melee::PhysicalAttackAction(true,true,2,2,5)==11&&diagnostic::melee::PhysicalAttackAction(false,true,2,2,5)==11);
    CHECK(diagnostic::melee::PhysicalAttackAction(true,true,3,2,0)==2&&diagnostic::melee::PhysicalAttackAction(true,true,2,1,0)==2); // blunt keeps native Knock Out power path
    for(int species:{1,2,3,4,6,7,8,9,10,11,12,13,14,19,29,38,41,52,-1,999}){
        CHECK(!diagnostic::melee::NativePierceTarget(species));
        CHECK(diagnostic::melee::PhysicalAttackAction(false,true,2,2,species)==1);
        CHECK(diagnostic::melee::PhysicalAttackAction(true,true,2,2,species)==2);
    }
    puts("PASS charged attack skill timing, equip-held trigger guard, one power per press, release grace, per-stroke cost across targets, exhaustion and deliberate same-victim finisher");return 0;
}
