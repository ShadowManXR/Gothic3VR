#pragma once
#include <cmath>
namespace diagnostic {
// Collect before mutation, and keep ownership within one stereo render only.
// The setter must alter this entity only, not recursively alter its children.
struct RenderAlphaSnapshot {
    using Set=void(__cdecl*)(void*,float);
    struct Entry {void* object;Set set;float alpha;};
    static constexpr unsigned Capacity=128;
    Entry entries[Capacity]{};unsigned count=0;bool active=false;
    bool Remember(void* object,float alpha,Set set){
        if(active||!object||!set||!std::isfinite(alpha)||alpha<0||alpha>1)return false;
        for(unsigned i=0;i<count;++i)if(entries[i].object==object&&entries[i].set==set)return true;
        if(count==Capacity)return false;entries[count++]={object,set,alpha};return true;
    }
    void Opaque(){active=true;for(unsigned i=0;i<count;++i)entries[i].set(entries[i].object,1.f);}
    void Restore(){if(!active)return;for(unsigned i=0;i<count;++i)entries[i].set(entries[i].object,entries[i].alpha);active=false;}
};
}
