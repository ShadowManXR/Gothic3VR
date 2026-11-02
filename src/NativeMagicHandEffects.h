#pragma once
#include "MagicHandEffects.h"
#include "NativeAttachedParticles.h"
namespace diagnostic::visual {
struct HandEffectAccess {
    using Entity=eCEntity;
    bool Valid(Entity* p)const{return Readable(p,sizeof(eCEntity));}
    arms::Mat Local(Entity* p)const{return ParticleMatrix(p->GetLocalMatrix());}
    arms::Mat World(Entity* p)const{return ParticleMatrix(p->GetWorldMatrix());}
    void SetLocal(Entity* p,arms::Mat const& m)const{bCMatrix n;memcpy(&n,&m,sizeof(n));p->SetLocalMatrix(n);}
    void SetWorld(Entity* p,arms::Mat const& m)const{bCMatrix n;memcpy(&n,&m,sizeof(n));p->SetToWorldMatrix(n);}
};
using NativeHandEffectScope=HandEffectScope<HandEffectAccess>;
inline bool VerifyHandEffectLayout(HMODULE engine){
    auto* base=reinterpret_cast<unsigned char*>(engine);
    unsigned char count[]{0x8b,0x81,0xc8,0,0,0,0xc3},array[]{0x8d,0xbe,0xc4,0,0,0,0x8b,0x77,4};
    unsigned char fields[]{0x8b,0x7c,0x24,0x10,0x8d,0x4e,0x68,0x89,0x7e,0x60,0x89,0x5e,0x64};
    unsigned char entity[]{0x8b,0x41,8,0x85,0xc0,0x56,0x8d,0x71,4};
    return engine&&GetProcAddress(engine,"?GetNumAttachments@eCVisualAnimation_PS@@QAEKXZ")==reinterpret_cast<FARPROC>(base+0x31160)&&
        GetProcAddress(engine,"?GetEntity@eCActorAttachment@eCVisualAnimation_PS@@QAEPAVeCEntity@@XZ")==reinterpret_cast<FARPROC>(base+0xa900)&&
        Readable(base+0x31160,sizeof(count))&&!memcmp(base+0x31160,count,sizeof(count))&&
        Readable(base+0x30dea,sizeof(array))&&!memcmp(base+0x30dea,array,sizeof(array))&&
        Readable(base+0x2a344,sizeof(fields))&&!memcmp(base+0x2a344,fields,sizeof(fields))&&
        Readable(base+0xa900,sizeof(entity))&&!memcmp(base+0xa900,entity,sizeof(entity));
}
// Exact native animation attachment ownership, not distance/name searches in
// the world. The offsets are gated by the installed engine's accessors above.
inline bool CaptureMagicHands(eCEntity* player,ControllerFrame const& controllers,uint64_t now,
    NativeHandEffectScope& scope,ParticleAttachmentFrame& particles){
    scope.Reset();if(!Readable(player,sizeof(eCDynamicEntity)))return true;
    auto* animation=static_cast<eCVisualAnimation_PS*>(player->GetPropertySet(eEPropertySetType_Animation));
    if(!Readable(animation,sizeof(*animation))||animation->GetEntity()!=player)return true;
    auto* bytes=reinterpret_cast<unsigned char*>(animation);void** attachments=nullptr;unsigned count=0;
    memcpy(&attachments,bytes+0xc4,sizeof(attachments));memcpy(&count,bytes+0xc8,sizeof(count));
    if(count>128||count!=animation->GetNumAttachments()||(count&&!Readable(attachments,count*sizeof(void*))))return false;
    for(unsigned i=0;i<count;++i){auto* a=static_cast<unsigned char*>(attachments[i]);if(!Readable(a,0x78))return false;
        void* owner=nullptr;void* linked=nullptr;memcpy(&owner,a,sizeof(owner));memcpy(&linked,a+0x60,sizeof(linked));
        if(owner!=animation||linked!=animation)continue;
        auto const& bone=*reinterpret_cast<bCString const*>(a+0x68);int length=bone.GetLength();auto* text=bone.GetText();
        if(length<0||length>63||!Readable(text,size_t(length)+1)||text[length])return false;
        int hand=EffectHand(text);if(hand<0||!arms::FreshGrip(controllers,unsigned(hand),now))continue;
        auto* root=reinterpret_cast<eCVisualAnimation_PS::eCActorAttachment*>(a)->GetEntity();char name[128]{};
        if(root==player||!Readable(root,sizeof(eCEntity))||root->HasPropertySet(eEPropertySetType_Projectile)||!ReadName(root,name)||!MagicHandEffect(name))continue;
        if(!scope.Capture(root,unsigned(hand))||!CollectHeldParticles(AttachedParticleAccess{},root,particles))return false;
    }return true;
}
}
