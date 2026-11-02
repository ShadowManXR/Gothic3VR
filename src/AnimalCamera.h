#pragma once
#include <cstddef>
namespace diagnostic {
// Hash-guarded CP Game.dll: IsInFirstPerson reads 3C1/4D4; SetMode
// also tests request latch 3C5 before selecting its third-person branch.
// Clearing only the mode is insufficient: SetMode otherwise re-enters FP.
struct AnimalCamera {
    static constexpr size_t Active=0x3c1,Request=0x3c5,Temporary=0x4d4,Size=Temporary+1;
    static bool FirstPerson(void const* memory,size_t size){
        if(!memory||size<Size)return false;auto* p=static_cast<unsigned char const*>(memory);
        return p[Active]||p[Request]||p[Temporary];
    }
    static bool Release(void* memory,size_t size){
        if(!memory||size<Size)return false;auto* p=static_cast<unsigned char*>(memory);
        p[Active]=p[Request]=p[Temporary]=0;return true;
    }
};
}
