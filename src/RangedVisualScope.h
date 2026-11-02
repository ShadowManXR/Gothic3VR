#pragma once
#include <cmath>
#include <cstdint>
namespace diagnostic { namespace ranged {
// A held projectile has no animation factory. Its live inventory slot, weapon
// and projectile state are the binding, independently of the hero's palette.
struct NockedIdentity {
    uintptr_t player=0,weapon=0,item=0;int ammo=0;
    bool Same(NockedIdentity const& current,bool flying)const{
        return !flying&&player&&weapon&&item&&player!=item&&weapon!=item&&
            (ammo==4||ammo==7)&&player==current.player&&weapon==current.weapon&&
            item==current.item&&ammo==current.ammo;
    }
};
// EnableRendering recursively changes children in the native engine. Capture
// the entire bounded tree before mutation, then restore parent before children
// so each original flag/alpha is recovered. This scope never outlives an eye.
template<class Access,unsigned Capacity=32> class VisualScope {
    using Entity=typename Access::Entity;
    struct Entry {Entity* item;bool visible;float alpha;};
    Entry entries[Capacity]{};unsigned count=0;bool active=false;Access access;
    bool Collect(Entity* item,unsigned depth){
        if(depth>8||!access.Valid(item)||count==Capacity)return false;
        for(unsigned i=0;i<count;++i)if(entries[i].item==item)return false;
        float alpha=access.Alpha(item);if(!std::isfinite(alpha)||alpha<0||alpha>1)return false;
        entries[count++]={item,access.Visible(item),alpha};
        int children=access.Children(item);if(children<0||children>int(Capacity))return false;
        for(int i=0;i<children;++i)if(!Collect(access.Child(item,i),depth+1))return false;
        return true;
    }
public:
    bool Active()const{return active;}
    bool Capture(Entity* root){
        if(active)return false;count=0;
        if(!Collect(root,0)){count=0;return false;}active=true;return true;
    }
    void Show(){if(active)for(unsigned i=0;i<count;++i){access.SetVisible(entries[i].item,true);access.SetAlpha(entries[i].item,1.f);}}
    void Restore(bool sameOwner){
        if(!active)return;active=false;unsigned n=count;count=0;
        if(sameOwner)for(unsigned i=0;i<n;++i){access.SetVisible(entries[i].item,entries[i].visible);access.SetAlpha(entries[i].item,entries[i].alpha);}
    }
    void Abandon(){active=false;count=0;}
};
// Launched ammo belongs to native physics, collision and decay. Reveal it once
// at the verified player Shoot callback; never retain it in a held-item scope.
template<class Access> bool RevealLaunch(typename Access::Entity* item,bool owned,bool flying){
    Access access;if(!owned||flying||!access.Valid(item))return false;
    access.SetVisible(item,true);access.SetAlphaTree(item,1.f);return true;
}
}}
