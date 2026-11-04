#pragma once
#include "FrozenArmPalette.h"
namespace diagnostic {
class ChestSkinPose {
    FrozenArmPalette frozen;TrackedPalette saved;float capturedRoot[3]{};
public:
    bool Pending()const{return saved.address!=nullptr;}
    void Reset(){saved.Restore(nullptr,0);frozen.Reset();}
    void Forget(){frozen.Reset();}
    bool Matches(uintptr_t player,uintptr_t factory,unsigned count)const{return frozen.Matches(player,factory,count);}
    bool Remember(uintptr_t player,uintptr_t factory,arms::Mat const* skin,unsigned count,float const (&root)[3]){
        for(float value:root)if(!std::isfinite(value)){frozen.Reset();return false;}
        if(Pending()||!frozen.Capture(player,factory,skin,count))return false;
        memcpy(capturedRoot,root,sizeof(root));return true;
    }
    bool Apply(uintptr_t player,uintptr_t factory,arms::Mat* skin,unsigned count,float const (&root)[3]){
        float delta[3];for(unsigned i=0;i<3;++i){delta[i]=root[i]-capturedRoot[i];if(!std::isfinite(delta[i]))return false;}
        if(!Matches(player,factory,count)||!saved.Begin(skin,count))return false;
        frozen.Apply(player,factory,skin,count);
        // Native interaction alignment remains authoritative. Suppress the
        // skeletal animation while retaining the hero's actual world position.
        for(unsigned i=0;i<count;++i){for(unsigned axis=0;axis<3;++axis)skin[i].m[12+axis]+=delta[axis];
            if(!arms::Affine(skin[i])){Restore(skin,count);return false;}}
        return true;
    }
    bool Restore(void* skin,unsigned count){
        bool ok=saved.Restore(skin,count);if(!ok)frozen.Reset();return ok;
    }
};
}
