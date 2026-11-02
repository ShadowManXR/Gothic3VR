#pragma once
#include <cstdint>
namespace diagnostic::temporal {
// Engine.dll 1.75.14. Installed only after the existing whole-file hash check.
// Native Execute calls EndSolid, then performs the dynamic occludee depth pass.
inline constexpr char EndSolidSymbol[]="?DoEndRenderSolid@eCModuleAdmin@@QAEXXZ";
inline constexpr uint32_t EndSolidRva=0x1d8e0;
inline constexpr unsigned char EndSolidPrefix[]={0x56,0x57,0x8b,0xf9,0x8b,0x47,0x38};
inline constexpr uint32_t EndSolidCallRva=0x1a0d89;
inline constexpr unsigned char EndSolidCall[]={0xe8,0x52,0xcb,0xe7,0xff};
inline constexpr uint32_t EndSolidReturnRva=EndSolidCallRva+sizeof(EndSolidCall);
inline constexpr char AnimatedRenderSymbol[]="?Render@eCRenderContextAnimation@@UAE_NABVeCRenderOp2@@@Z";
inline constexpr uint32_t AnimatedRenderRva=0x448f0;
inline constexpr unsigned char AnimatedRenderPrefix[]={0x55,0x8b,0xec,0x83,0xe4,0xf8};
struct GeometryRenderEntry {char const* symbol;uint32_t rva;unsigned size;unsigned char prefix[8];};
// Complete position-independent instructions, validated against the same whole
// Engine.dll hash before installation. Render contexts may queue instanced
// draws; ExecuteInstance needs its own unordered entity scope.
inline constexpr GeometryRenderEntry GeometryRenderEntries[]={
    {"?Render@eCRenderContextMesh@@UAE_NABVeCRenderOp2@@@Z",0x1d9210,8,{0x83,0xec,0x0c,0x55,0x8b,0x6c,0x24,0x14}},
    {"?Render@eCRenderContextLegacy@@UAE_NABVeCRenderOp2@@@Z",0x1ad8b0,5,{0x55,0x56,0x57,0x8b,0xf9}},
    {"?Render@eCRenderContextSpeedTree@@UAE_NABVeCRenderOp2@@@Z",0x1e78f0,6,{0x56,0x8b,0x71,0x1c,0x8b,0x06}},
    {"?ExecuteInstance@eCVertexbufferPoolItemMain@@IAEXXZ",0x1dabf0,6,{0x55,0x8b,0xec,0x83,0xe4,0xf8}}
};
inline constexpr unsigned InstanceArrayOffset=0x6c,InstanceCountOffset=0x70,InstanceStride=20;
inline constexpr unsigned WarmupWorldFrames=60;
class WorldGate {
    uint64_t last=0;unsigned count=0;
public:
    bool Begin(unsigned eye,uint64_t frame,int context)noexcept {
        if(eye==0&&frame!=last){
            if(context!=1||(last&&frame!=last+1))count=0;
            if(context==1&&count<WarmupWorldFrames)++count;
            last=frame;
        }
        return context==1&&count>=WarmupWorldFrames;
    }
};
}
