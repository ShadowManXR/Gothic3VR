#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic::visual {
// Transform world-space particles at draw time. Their simulation and shared
// buffers remain native. The second world-matrix call (native restoration)
// passes unchanged; the allowance is reset independently for each draw/eye.
class ParticleWorldScope {
    arms::Mat delta;bool active=false,applied=false;
    inline static thread_local ParticleWorldScope* current=nullptr;
public:
    // Only the thread entering the actual held-emitter draw can see this
    // correction. Every ordinary world draw forwards without a thread-ID or
    // shared-state lookup. A refused nested Begin must leave the outer scope.
    static ParticleWorldScope* Current()noexcept{return current;}
    bool Begin(arms::Mat const& value){if(active||current||!arms::Affine(value))return false;delta=value;active=true;applied=false;current=this;return true;}
    bool Active()const{return active;}
    void End(){if(current==this)current=nullptr;active=applied=false;}
    bool Rewrite(arms::Mat const& native,arms::Mat& out){
        if(!active||applied)return false;
        arms::Mat identity;for(unsigned i=0;i<16;++i)if(!std::isfinite(native.m[i])||std::fabs(native.m[i]-identity.m[i])>.0001f)return false;
        out=arms::Multiply(native,delta);applied=true;return arms::Affine(out);
    }
};
class ParticleAttachmentFrame {
    struct Entry {uintptr_t particle,anchor;arms::Mat inverse;};Entry entries[64]{};unsigned count=0;
public:
    void Reset(){count=0;}
    bool Add(uintptr_t particle,arms::Mat const& native,uintptr_t anchor=0){
        if(!particle)return false;for(unsigned i=0;i<count;++i)if(entries[i].particle==particle)return true;
        arms::Mat inverse;if(count==64||!arms::Inverse(native,inverse)){Reset();return false;}
        entries[count++]={particle,anchor,inverse};return true;
    }
    uintptr_t Anchor(uintptr_t p)const{for(unsigned i=0;i<count;++i)if(entries[i].particle==p)return entries[i].anchor;return 0;}
    bool Contains(uintptr_t p)const{for(unsigned i=0;i<count;++i)if(entries[i].particle==p)return true;return false;}
    bool Delta(uintptr_t p,arms::Mat const& current,arms::Mat& out)const{
        for(unsigned i=0;i<count;++i)if(entries[i].particle==p){out=arms::Multiply(entries[i].inverse,current);return arms::Affine(out);}return false;
    }
};
// Only actual equipped item trees qualify. Never search the hero, ROI or world.
inline bool HeldParticleRoot(int type,bool flameSword){return type==24||type==15||type==12||type==51||(type==2&&flameSword);}
template<class Access> bool CollectHeldParticles(Access const& access,typename Access::Entity* root,ParticleAttachmentFrame& frame,bool nockedAmmo=false){
    if(!access.Valid(root))return false;
    auto native=access.World(root);if(!arms::Affine(native))return false;
    typename Access::Entity* nodes[64]{root};unsigned count=1;
    for(unsigned i=0;i<count;++i){auto* node=nodes[i];if(!access.Valid(node))return false;
        // The ranged renderer may supply the verified, non-flying ammo in
        // the player's hand. Never descend into another projectile.
        if(access.Projectile(node)&&!(i==0&&nockedAmmo))continue;
        auto particle=access.PrivateParticle(node);
        if(particle&&!frame.Add(particle,native,reinterpret_cast<uintptr_t>(root)))return false;
        int n=access.Children(node);if(n<0||n>64)return false;
        for(int j=0;j<n;++j){auto* child=access.Child(node,j);
            if(count==64||!access.Valid(child)||access.Parent(child)!=node)return false;
            for(unsigned k=0;k<count;++k)if(nodes[k]==child)return false;
            nodes[count++]=child;
        }
    }return true;
}
}
