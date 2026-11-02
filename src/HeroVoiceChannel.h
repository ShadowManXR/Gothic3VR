#pragma once
#include <cstdint>
namespace diagnostic {
// FMOD_MODE values documented in the FMOD Core API. Keep loop/stream flags;
// only the hero's current speech CHANNEL becomes non-spatial in VR.
inline constexpr uint32_t Voice2D=0x8,Voice3D=0x10;
class HeroVoiceChannel {
    uintptr_t owner=0,channel=0;uint32_t originalMode=0;
public:
    bool Empty()const{return !channel;}
    bool Tracks(uintptr_t unit)const{return !Empty()&&owner==unit;}
    void Forget(){owner=channel=0;originalMode=0;}
    // The caller supplies a live, currently owned channel on every call.
    // Saved handles are identity tokens and are never dereferenced on their
    // own, including during world replacement and engine shutdown.
    template<class Api> int Update(uintptr_t unit,uintptr_t liveChannel,bool enabled,Api& api){
        if(owner!=unit||channel!=liveChannel)Forget();
        if(!unit||!liveChannel)return 0;
        if(!enabled){
            if(Empty())return 0;
            int result=api.SetMode(liveChannel,originalMode);
            Forget();return result?-1:2;
        }
        if(!Empty())return 0; // No FMOD calls on ordinary speech updates.
        uint32_t mode=0;if(api.GetMode(liveChannel,mode))return -1;
        // PUP implements getPan as a modal "unsupported" warning, despite
        // exporting the function. Native speech is 3D; its 2D pan is irrelevant
        // after restoring 3D mode. Already-2D channels are never modified.
        if(!(mode&Voice3D))return 0;
        if(api.SetMode(liveChannel,(mode&~Voice3D)|Voice2D))return -1;
        if(api.SetPan(liveChannel,0.f)){
            api.SetMode(liveChannel,mode);return -1;
        }
        owner=unit;channel=liveChannel;originalMode=mode;return 1;
    }
};
}
