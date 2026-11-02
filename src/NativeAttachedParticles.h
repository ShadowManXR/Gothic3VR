#pragma once
#include <g3sdk/Script.h>
#include "ParticleAttachment.h"
#include "NativeHeldWeaponVisuals.h"
#include "VirtualHook.h"
namespace diagnostic::visual {
inline arms::Mat ParticleMatrix(bCMatrix const& native){arms::Mat out;static_assert(sizeof(out)==sizeof(native));memcpy(&out,&native,sizeof(out));return out;}
struct AttachedParticleAccess {
    using Entity=eCEntity;
    bool Valid(Entity* p)const{return Readable(p,sizeof(eCEntity));}
    Entity* Parent(Entity* p)const{return p->GetParent();}
    int Children(Entity* p)const{return p->GetChildrenCount();}
    Entity* Child(Entity* p,int i)const{return p->GetChildAt(i);}
    bool Projectile(Entity* p)const{return p->HasPropertySet(eEPropertySetType_Projectile)!=GEFalse;}
    arms::Mat World(Entity* p)const{return ParticleMatrix(p->GetWorldMatrix());}
    uintptr_t PrivateParticle(Entity* node)const{
        auto* particle=static_cast<eCParticle_PS*>(node->GetPropertySet(eEPropertySetType_Particle));
        // Local/instanced emitters already receive the native owner matrix.
        // No simulation, template, visibility or shared-buffer changes.
        return Readable(particle,sizeof(*particle))&&particle->GetEntity()==node&&!particle->GetInstanced()&&
            particle->GetCoordinateSystem().GetValue()==eECoordinateSystem_Independent?reinterpret_cast<uintptr_t>(particle):0;
    }
};
inline bool CaptureHeldParticles(eCEntity* player,ControllerFrame const& controllers,uint64_t now,ParticleAttachmentFrame& frame){
    frame.Reset();if(!Readable(player,sizeof(eCDynamicEntity)))return true;
    eCEntity* previous=nullptr;
    for(unsigned slot=0;slot<2;++slot){unsigned hand=slot?0:1;if(!arms::FreshGrip(controllers,hand,now))continue;
        auto* weapon=gCEntity::GetWeapon(player,slot==0);
        if(weapon==player||weapon==previous||!Readable(weapon,sizeof(eCEntity)))continue;
        char name[128]{};int type=int(gCEntity::GetUseType(weapon));
        if(!HeldParticleRoot(type,type==2&&ReadName(weapon,name)&&FlameSword(name)))continue;
        previous=weapon;
        if(!CollectHeldParticles(AttachedParticleAccess{},weapon,frame)){frame.Reset();return false;}
    }return true;
}
}
