#pragma once
#include <cstdint>
namespace diagnostic {
struct RoomMotionScope {
    uintptr_t movement=0;uint32_t thread=0;uint64_t suppressed=0;
    void Begin(void* playerMovement,uint32_t gameThread){movement=reinterpret_cast<uintptr_t>(playerMovement);thread=gameThread;}
    void End(){movement=0;thread=0;}
    bool Suppress(void* candidate,uint32_t currentThread){
        if(!movement||movement!=reinterpret_cast<uintptr_t>(candidate)||thread!=currentThread)return false;
        ++suppressed;return true;
    }
};
}
