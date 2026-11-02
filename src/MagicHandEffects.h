#pragma once
#include "TrackedArmsMath.h"
#include <cctype>
namespace diagnostic::visual {
inline int EffectHand(char const* bone){
    if(!bone)return -1;
    if(!strcmp(bone,"Hero_Left_Hand_Hand_1"))return 0;
    if(!strcmp(bone,"Hero_Right_Hand_Hand_1"))return 1;
    return -1;
}
inline bool MagicHandEffect(char const* name){
    if(!name)return false;char const* prefix="vfx_magic_";
    for(unsigned i=0;prefix[i];++i)if(std::tolower(static_cast<unsigned char>(name[i]))!=prefix[i])return false;
    return true;
}
// Frame-local, reversible render poses. A held item and a bone-bound effect
// have distinct native owners; neither should inherit the other's offset.
template<class Access,unsigned Capacity=32> class HandEffectScope {
    using Entity=typename Access::Entity;
    struct Entry {Entity* entity;unsigned hand;arms::Mat local,world;bool changed;};
    Entry entries[Capacity]{};unsigned count=0;Access access;
public:
    unsigned Count()const{return count;}
    bool Capture(Entity* p,unsigned hand){
        if(hand>1||!access.Valid(p))return false;
        for(unsigned i=0;i<count;++i)if(entries[i].entity==p)return entries[i].hand==hand;
        if(count==Capacity)return false;
        auto local=access.Local(p),world=access.World(p);
        if(!arms::Affine(local)||!arms::Affine(world))return false;
        entries[count++]={p,hand,local,world,false};return true;
    }
    void Restore(){for(unsigned i=0;i<count;++i){auto& e=entries[i];if(e.changed){e.changed=false;if(access.Valid(e.entity))access.SetLocal(e.entity,e.local);}}}
    void Reset(){Restore();count=0;}
    void Apply(arms::Mat const (&delta)[2],bool const (&ready)[2]){
        Restore();
        for(unsigned i=0;i<count;++i){auto& e=entries[i];if(!ready[e.hand]||!access.Valid(e.entity))continue;
            auto pose=arms::Multiply(e.world,delta[e.hand]);if(!arms::Affine(pose))continue;
            e.changed=true;access.SetWorld(e.entity,pose);
        }
    }
};
}
