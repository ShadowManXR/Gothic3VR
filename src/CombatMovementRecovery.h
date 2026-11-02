#pragma once
#include <cstdint>
namespace diagnostic {
enum class CombatMoveRepair { None, Orphan, PhysicalAttack };
struct CombatMoveState {
    uintptr_t player=0,movement=0,routine=0;
    bool allowed=false,combat=false,idle=false,physicalMelee=false;
    int mode=0,action=-1,animation=-1,instructionAction=-1;
    uintptr_t callback=0,combatCallback=0;
};
// A null SPU instruction cannot retire its combat translation. A physical VR
// attack also must not leave an animation-driven attack owning translation
// after the player script has returned to its standing idle loop. In contrast,
// incoming-hit, parry, finisher, recovery and all other live instructions remain
// owned by the native game. Require a continuous, identity-stable idle window.
class CombatMovementRecovery {
    CombatMoveState previous;CombatMoveRepair pending=CombatMoveRepair::None;
    uint64_t since=0,last=0;bool emitted=false;
public:
    static constexpr uint64_t SettleMs=300,MaxGapMs=100;
    void Reset(){previous={};pending=CombatMoveRepair::None;since=last=0;emitted=false;}
    static CombatMoveRepair Classify(CombatMoveState const& s){
        if(!s.allowed||!s.player||!s.movement||!s.routine||!s.combat||!s.idle||
           s.mode<1||s.mode>5||s.action!=0||s.animation!=2||!s.combatCallback)return CombatMoveRepair::None;
        if(!s.callback)return CombatMoveRepair::Orphan;
        if(s.physicalMelee&&s.callback==s.combatCallback&&s.instructionAction>=1&&s.instructionAction<=14)
            return CombatMoveRepair::PhysicalAttack;
        return CombatMoveRepair::None;
    }
    CombatMoveRepair Step(CombatMoveState const& s,uint64_t now){
        auto kind=Classify(s);
        if(kind==CombatMoveRepair::None){Reset();return kind;}
        bool same=pending==kind&&previous.player==s.player&&previous.movement==s.movement&&previous.routine==s.routine&&
            previous.callback==s.callback&&previous.instructionAction==s.instructionAction&&now>=last&&now-last<=MaxGapMs;
        if(!same){since=now;emitted=false;}
        previous=s;pending=kind;last=now;
        if(emitted||now<since||now-since<SettleMs)return CombatMoveRepair::None;
        emitted=true;return kind;
    }
};
}
