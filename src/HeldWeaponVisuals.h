#pragma once
#include <cctype>
#include <cstdint>
namespace diagnostic { namespace visual {
// Built from the two held weapon trees once per tracked frame. Particle
// callbacks compare addresses only: no memory queries, names, scene traversal,
// player/equipment lookups or SDK calls on unrelated scene emitters.
class FlameTailTargets {
    uintptr_t particles[8]{};unsigned count=0;
public:
    void Reset(){count=0;}
    unsigned Count()const{return count;}
    bool Add(uintptr_t particle){
        if(!particle)return false;
        for(unsigned i=0;i<count;++i)if(particles[i]==particle)return true;
        if(count==8){Reset();return false;}particles[count++]=particle;return true;
    }
    bool Contains(uintptr_t particle)const{
        for(unsigned i=0;i<count;++i)if(particles[i]==particle)return true;return false;
    }
};
template<class Access> bool CollectFlameWeapon(Access const& access,typename Access::Entity* weapon,FlameTailTargets& targets){
    if(!weapon)return true;
    if(!access.Valid(weapon))return false;
    if(access.Type(weapon)!=2||!access.IsFlameSword(weapon))return true;
    typename Access::Entity* nodes[64]{weapon};unsigned count=1;
    for(unsigned i=0;i<count;++i){auto* node=nodes[i];auto particle=access.WorldTail(node);
        if(particle&&!targets.Add(particle))return false;
        int children=access.Children(node);if(children<0||children>64)return false;
        for(int j=0;j<children;++j){auto* child=access.Child(node,j);
            if(count==64||!access.Valid(child)||access.Parent(child)!=node)return false;
            for(unsigned k=0;k<count;++k)if(nodes[k]==child)return false;
            nodes[count++]=child;
        }
    }return true;
}
inline bool SameName(char const* a,char const* b){
    if(!a||!b)return false;
    for(unsigned i=0;i<128;++i){unsigned char x=a[i],y=b[i];if(std::tolower(x)!=std::tolower(y))return false;if(!x)return true;}return false;
}
inline bool FlameSword(char const* name){
    return SameName(name,"It_1H_FlameSword_01")||SameName(name,"It_1H_MagicFlameSword_01");
}
inline bool SummonedFlameSword(char const* name,int type){
    return type==2&&SameName(name,"It_1H_MagicFlameSword_01");
}
// Native templates:01 is the high-count world-space flame copy;04 is its
// world-space particle tail.03 supplies the same fire texture in blade-local
// space, while02 supplies local sparks. Keep those attached visual layers.
inline bool FlameWorldTail(char const* name,int coordinateSystem,bool instanced){
    return coordinateSystem==0&&!instanced&&
        (SameName(name,"VFX_Magic_Weapons_FlameSword_01")||SameName(name,"VFX_Magic_Weapons_FlameSword_04"));
}
inline bool HideMenuStaff(bool firstPerson,bool menu,bool radial,int type){
    return firstPerson&&(menu||radial)&&(type==12||type==51);
}
// Native inventory slots are actual attachment locations, not quick slots.
// BackLeft/BackRight/Bow/Crossbow are stowed; never hide a current hand item,
// including the transition where native equipment references overlap.
inline bool StowedWeaponType(int type){
    return type==2||type==3||type==5||type==6||type==9||type==12||type==51||type==52;
}
template<class Access,class Scope> bool CaptureStowedWeapons(Access const& access,bool firstPerson,Scope& scope){
    if(!firstPerson)return true;
    auto* right=access.Slot(1);auto* left=access.Slot(2);
    constexpr int slots[]{5,6,7,8};
    for(int slot:slots){auto* item=access.Slot(slot);
        if(!item||item==right||item==left)continue;
        if(!access.Valid(item))return false;
        if(StowedWeaponType(access.Type(item))&&!scope.Capture(item))return false;
    }return true;
}
template<class Access> bool HeldFlameOwner(Access const& access,typename Access::Entity* effect,
    typename Access::Entity* player,typename Access::Entity* right,typename Access::Entity* left){
    if(!effect||effect==player||!access.Valid(effect))return false;
    auto* parent=access.Parent(effect);
    for(unsigned depth=0;depth<16&&parent&&parent!=player;++depth){
        if(!access.Valid(parent))return false;
        if(parent==right||parent==left)return access.Type(parent)==2&&access.IsFlameSword(parent);
        parent=access.Parent(parent);
    }return false;
}
// Frame-local visibility only. Collect the entire tree before hiding it;
// EnableRendering is recursive, so restore parents before their children.
template<class Access,unsigned Capacity=64> class VisibilityScope {
    using Entity=typename Access::Entity;
    struct Entry {Entity* entity;bool visible;};Entry entries[Capacity]{};
    unsigned count=0;bool active=false;Access access;
    bool Collect(Entity* entity,unsigned depth){
        if(depth>12||!access.Valid(entity)||count==Capacity)return false;
        for(unsigned i=0;i<count;++i)if(entries[i].entity==entity)return false;
        entries[count++]={entity,access.Visible(entity)};int n=access.Children(entity);
        if(n<0||n>int(Capacity))return false;
        for(int i=0;i<n;++i)if(!Collect(access.Child(entity,i),depth+1))return false;
        return true;
    }
public:
    bool Capture(Entity* entity){
        if(active)return false;
        for(unsigned i=0;i<count;++i)if(entries[i].entity==entity)return true;
        if(!Collect(entity,0)){count=0;return false;}return true;
    }
    void Hide(){active=true;for(unsigned i=0;i<count;++i)access.SetVisible(entries[i].entity,false);}
    void Restore(){
        if(active){active=false;for(unsigned i=0;i<count;++i)if(access.Valid(entries[i].entity))access.SetVisible(entries[i].entity,entries[i].visible);}
        count=0;
    }
    void Abandon(){count=0;active=false;}
    unsigned Count()const{return count;}
};
// Complete relocation-free x86 instructions in the verified Engine.dll.
constexpr unsigned ParticleProcessRva=0x14ed10,ParticleAlphaRva=0x14ee30;
inline constexpr unsigned char ParticleProcessPrefix[6]{0x53,0x55,0x56,0x57,0x8b,0xf1};
inline constexpr unsigned char ParticleAlphaPrefix[6]{0x55,0x8b,0xec,0x83,0xe4,0xf8};
}}
