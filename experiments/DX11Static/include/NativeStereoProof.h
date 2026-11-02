#pragma once
#include <cstdint>
#include <limits>

namespace g3vr::dx11 {
// Approval belongs to the stereo pair. Never enable a replacement midway
// through the pair that establishes its proof; one failed eye revokes both.
class NativeStereoProof {
    bool attempted[2]{}, passed[2]{}, refused=false;
    uint64_t readyFrame=0;
public:
    void Record(unsigned eye,uint64_t frame,bool valid) noexcept {
        if(eye>1||!frame){Invalidate();return;}
        if(attempted[eye]){if(!valid)Invalidate();return;}
        attempted[eye]=true;passed[eye]=valid;
        if(!valid){Invalidate();return;}
        if(!refused&&passed[0]&&passed[1]&&frame<std::numeric_limits<uint64_t>::max())readyFrame=frame+1;
    }
    void Invalidate() noexcept { refused=true;readyFrame=0; }
    bool Refused() const noexcept {return refused;}
    bool Allows(uint64_t frame) const noexcept {return !refused&&readyFrame&&frame>=readyFrame;}
};
}
