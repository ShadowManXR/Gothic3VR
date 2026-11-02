#include "HeldWeaponVisuals.h"
#include "FocusHook.h"
#include <cstdio>
#include <vector>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Item {char const* name="";int type=0;bool valid=true,visible=true;float alpha=.4f;Item* parent=nullptr;std::vector<Item*> children;};
struct Access {
    using Entity=Item;
    bool Valid(Item* p)const{return p&&p->valid;}
    Item* Parent(Item* p)const{return p->parent;}
    int Type(Item* p)const{return p->type;}
    bool IsFlameSword(Item* p)const{return visual::FlameSword(p->name);}
    bool Visible(Item* p)const{return p->visible;}
    int Children(Item* p)const{return int(p->children.size());}
    Item* Child(Item* p,int i)const{return p->children[i];}
    void SetVisible(Item* p,bool state)const{p->visible=state;for(auto* child:p->children)SetVisible(child,state);}
};
struct InventoryAccess : Access {
    Item* slots[19]{};
    Item* Slot(int slot)const{return slots[slot];}
};
struct Emitter {unsigned ticks=0,draws=0;short lastPass=0;bool suppressed=false;};
FocusHook processHook,alphaHook;
visual::FlameTailTargets callbackTargets;
__declspec(naked) void __fastcall NativeProcess(Emitter*,void*){
    __asm {
        push ebx
        push ebp
        push esi
        push edi
        mov esi,ecx
        inc dword ptr [esi]
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    }
}
__declspec(naked) void __fastcall NativeAlpha(Emitter*,void*,short){
    __asm {
        push ebp
        mov ebp,esp
        and esp,0fffffff8h
        inc dword ptr [ecx+4]
        mov ax,word ptr [ebp+8]
        mov word ptr [ecx+8],ax
        mov esp,ebp
        pop ebp
        ret 4
    }
}
void __fastcall Process(Emitter* self,void*){if(!callbackTargets.Contains(reinterpret_cast<uintptr_t>(self)))reinterpret_cast<void(__thiscall*)(Emitter*)>(processHook.original)(self);}
void __fastcall Alpha(Emitter* self,void*,short pass){if(!callbackTargets.Contains(reinterpret_cast<uintptr_t>(self)))reinterpret_cast<void(__thiscall*)(Emitter*,short)>(alphaHook.original)(self,pass);}
int main(){
    for(char const* name:{"VFX_Magic_Weapons_FlameSword_01","VFX_Magic_Weapons_FlameSword_04"}){
        CHECK(visual::FlameWorldTail(name,0,false));CHECK(!visual::FlameWorldTail(name,1,false));CHECK(!visual::FlameWorldTail(name,0,true));
    }
    for(char const* name:{"VFX_Magic_Weapons_FlameSword_02","VFX_Magic_Weapons_FlameSword_03","VFX_Magic_FlameSword_Fire_01","VFX_Env_Fire_01","VFX_Magic_Weapons_FlameSword_010",""})CHECK(!visual::FlameWorldTail(name,0,false));
    CHECK(!visual::FlameWorldTail(nullptr,0,false));CHECK(visual::FlameSword("it_1h_magICflamesword_01"));CHECK(!visual::FlameSword("It_Scroll_Flamesword"));
    Item hero,npc,right,left,tail,group,otherWeapon,npcTail,world;
    right.name="It_1H_FlameSword_01";right.type=2;right.parent=&hero;
    left.name="It_1H_MagicFlameSword_01";left.type=2;left.parent=&hero;
    group.parent=&right;tail.parent=&group;otherWeapon=right;otherWeapon.parent=&npc;npcTail.parent=&otherWeapon;
    CHECK(visual::HeldFlameOwner(Access{},&tail,&hero,&right,&left));
    tail.parent=&left;CHECK(visual::HeldFlameOwner(Access{},&tail,&hero,&right,&left));
    CHECK(!visual::HeldFlameOwner(Access{},&npcTail,&hero,&right,&left));CHECK(!visual::HeldFlameOwner(Access{},&world,&hero,&right,&left));
    tail.parent=&hero;CHECK(!visual::HeldFlameOwner(Access{},&tail,&hero,&right,&left));
    tail.parent=&right;right.name="It_1H_Katana_01";CHECK(!visual::HeldFlameOwner(Access{},&tail,&hero,&right,&left));
    right.name="It_1H_FlameSword_01";right.type=12;CHECK(!visual::HeldFlameOwner(Access{},&tail,&hero,&right,&left));right.type=2;
    CHECK(!visual::HeldFlameOwner(Access{},&tail,&hero,nullptr,&left));right.valid=false;CHECK(!visual::HeldFlameOwner(Access{},&tail,&hero,&right,&left));right.valid=true;
    tail.parent=&group;group.parent=&tail;CHECK(!visual::HeldFlameOwner(Access{},&tail,&hero,&right,&left));
    // Scoped menu hiding preserves mixed child flags despite recursive native
    // setters, and does not touch opacity or retain a world after return.
    Item staff,decoration,light,secondStaff;staff.type=12;decoration.visible=false;light.visible=true;
    staff.children={&decoration};decoration.children={&light};staff.alpha=.2f;decoration.alpha=.7f;
    for(int type:{12,51})CHECK(visual::HideMenuStaff(true,true,false,type)&&visual::HideMenuStaff(true,false,true,type));
    for(int type:{2,3,6,8,9,49})CHECK(!visual::HideMenuStaff(true,true,true,type));
    CHECK(!visual::HideMenuStaff(false,true,true,12)&&!visual::HideMenuStaff(true,false,false,12));
    for(unsigned eye=0;eye<2;++eye){visual::VisibilityScope<Access> scope;
        CHECK(scope.Capture(&staff)&&scope.Capture(&staff)&&scope.Count()==3);CHECK(scope.Capture(&secondStaff)&&scope.Count()==4);
        scope.Hide();CHECK(!staff.visible&&!decoration.visible&&!light.visible&&!secondStaff.visible);
        scope.Restore();CHECK(staff.visible&&!decoration.visible&&light.visible&&secondStaff.visible&&scope.Count()==0);
        CHECK(staff.alpha==.2f&&decoration.alpha==.7f);scope.Restore();
    }
    visual::VisibilityScope<Access,2> full;CHECK(!full.Capture(&staff));CHECK(full.Count()==0&&staff.visible&&!decoration.visible&&light.visible);
    visual::VisibilityScope<Access> teardown;CHECK(teardown.Capture(&staff));teardown.Hide();teardown.Abandon();staff.valid=false;teardown.Restore();CHECK(teardown.Count()==0);staff.valid=true;
    // Running and sheathing: only actual back-slot equipment is hidden, with
    // both eyes using the same snapshot and exact mixed child flags restored.
    InventoryAccess inventory;Item blade,shield,bow,crossbow,held,ammo,armor,spell,ornament,glow;
    blade.type=2;shield.type=9;bow.type=5;crossbow.type=6;held.type=3;ammo.type=7;armor.type=10;spell.type=8;
    ornament.visible=false;blade.children={&ornament};ornament.children={&glow};
    inventory.slots[1]=&held;inventory.slots[3]=&armor;inventory.slots[5]=&shield;
    inventory.slots[6]=&blade;inventory.slots[7]=&bow;inventory.slots[8]=&crossbow;inventory.slots[14]=&ammo;
    visual::VisibilityScope<Access> stowed;
    CHECK(visual::CaptureStowedWeapons(inventory,true,stowed)&&stowed.Count()==6);
    for(unsigned eye=0;eye<2;++eye){stowed.Hide();CHECK(!blade.visible&&!shield.visible&&!bow.visible&&!crossbow.visible&&!ornament.visible&&!glow.visible);
        CHECK(held.visible&&ammo.visible&&armor.visible&&spell.visible&&hero.visible&&npc.visible);}
    stowed.Restore();CHECK(blade.visible&&shield.visible&&bow.visible&&crossbow.visible&&!ornament.visible&&glow.visible);
    CHECK(visual::CaptureStowedWeapons(inventory,false,stowed)&&stowed.Count()==0); // dialogue/activity/knockdown
    // Item moved into the hand: an overlapping back reference must not hide it.
    inventory.slots[1]=&blade;inventory.slots[2]=&shield;
    CHECK(visual::CaptureStowedWeapons(inventory,true,stowed)&&stowed.Count()==2);stowed.Hide();
    CHECK(blade.visible&&shield.visible&&!bow.visible&&!crossbow.visible);stowed.Restore();
    // Finish sheathing; a fresh frame sees it on the back, not in the hand.
    inventory.slots[1]=inventory.slots[2]=nullptr;
    CHECK(visual::CaptureStowedWeapons(inventory,true,stowed)&&stowed.Count()==6);stowed.Hide();stowed.Restore();
    CHECK(blade.visible&&shield.visible&&!ornament.visible&&glow.visible);
    for(int type:{2,3,5,6,9,12,51,52}){blade.type=type;CHECK(visual::CaptureStowedWeapons(inventory,true,stowed)&&stowed.Count()==6);stowed.Restore();}
    blade.type=8;CHECK(visual::CaptureStowedWeapons(inventory,true,stowed)&&stowed.Count()==3);stowed.Restore();
    blade.type=2;blade.valid=false;CHECK(!visual::CaptureStowedWeapons(inventory,true,stowed));stowed.Abandon();
    CHECK(shield.visible&&bow.visible&&crossbow.visible);blade.valid=true;
    // Real Win32 thiscall->fastcall callbacks: both forwarded and skipped
    // paths balance the stack for no-argument Process and short OnRenderAlpha.
    CHECK(processHook.Install(const_cast<void*>(ResolveJumpChain(reinterpret_cast<void*>(&NativeProcess))),reinterpret_cast<void*>(&Process),visual::ParticleProcessPrefix,6));
    CHECK(alphaHook.Install(const_cast<void*>(ResolveJumpChain(reinterpret_cast<void*>(&NativeAlpha))),reinterpret_cast<void*>(&Alpha),visual::ParticleAlphaPrefix,6));
    Emitter core,ghost;ghost.suppressed=true;
    CHECK(callbackTargets.Add(reinterpret_cast<uintptr_t>(&ghost)));
    for(unsigned i=0;i<10000;++i){NativeProcess(&core,nullptr);NativeAlpha(&core,nullptr,short(-123));NativeProcess(&ghost,nullptr);NativeAlpha(&ghost,nullptr,short(32760));}
    CHECK(core.ticks==10000&&core.draws==10000&&core.lastPass==-123&&ghost.ticks==0&&ghost.draws==0);
    callbackTargets.Reset();ghost.suppressed=false;NativeProcess(&ghost,nullptr);NativeAlpha(&ghost,nullptr,37);CHECK(ghost.ticks==1&&ghost.draws==1&&ghost.lastPass==37);
    puts("PASS reversible stowed equipment in both eyes, draw/sheath overlap, native third-person visibility, exact world-tail layers, menu staff and10000 real x86 hook/forward/skip cycles");return 0;
}
