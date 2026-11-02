#pragma once
#include "SceneCostEntries.h"
#include "SceneCostTrial.h"
namespace diagnostic::scene_cost {
struct EntityEntry {Entry entry;Scope scope;uint32_t relocation=0;unsigned operand=0;};
inline constexpr EntityEntry EntityEntries[]={
 {{"?OnPreProcess@eCDynamicEntity@@MAEXXZ",0x22aa90,5,{0x56,0x8b,0xf1,0x8b,0x06},false},Scope::DynamicPre},
 {{"?OnProcess@eCDynamicEntity@@MAEXXZ",0x22a980,7,{0x51,0x56,0x8b,0xf1,0x8b,0x4e,0x30},false},Scope::DynamicProcess},
 {{"?OnPostProcess@eCDynamicEntity@@MAEXXZ",0x22ab50,5,{0x56,0x8b,0xf1,0x8b,0x06},false},Scope::DynamicPost},
 {{"?Process@eCPhysicObject@@QAEXXZ",0x165530,5,{0xa1,0xcc,0x2d,0x60,0x10},false},Scope::PhysicObject,0x602dcc,1},
 {{"?OnProcess@gCCharacterMovement_PS@@MAEXXZ",0xe2120,6,{0x55,0x8b,0xec,0x83,0xe4,0xf8},true},Scope::CharacterMovement},
 {{"?OnProcess@eCVisualAnimation_PS@@UAEXXZ",0x32d40,6,{0x55,0x8b,0xec,0x83,0xe4,0xf8},false},Scope::CharacterAnimation},
 {{"?OnProcess@eCAudioEmitter_PS@@MAEXXZ",0x57ef0,8,{0x83,0xec,0x0c,0xa1,0x88,0x2a,0x60,0x10},false},Scope::AudioEmitter,0x602a88,4},
 {{"?CallTheNextOnProcessRoutine@gCNavigationAdmin@@QAEXXZ",0x1205a0,5,{0x51,0x53,0x55,0x8b,0xd9},true},Scope::NavigationRoutine}
};
inline constexpr unsigned EntityEntryCount=sizeof(EntityEntries)/sizeof(EntityEntries[0]);
inline bool EntityPrefix(EntityEntry const& entry,uint32_t base,unsigned char (&out)[16])noexcept{
 std::memcpy(out,entry.entry.prefix,sizeof(out));if(!entry.relocation)return true;
 if(base>UINT32_MAX-entry.relocation||entry.operand+4>entry.entry.bytes)return false;
 uint32_t address=base+entry.relocation;std::memcpy(out+entry.operand,&address,4);return true;
}
}
