#pragma once
#include "NativePhasePerformance.h"
#include <cstdint>
#include <cstring>
namespace diagnostic::performance {
// Exact Engine.dll 1.75.14 exports. Whole-file validation remains mandatory.
// Each prefix ends before any relative instruction; absolute operands remain
// valid when copied into a trampoline in this same process.
struct ExecutionEntry {char const* symbol;uint32_t rva;unsigned bytes;unsigned char prefix[16];NativePhase phase;};
inline constexpr ExecutionEntry ExecutionEntries[]={
    {"?Render@eCRenderOp2@@QAEXABVeCFrustumDatabase@@_N@Z",0x1ac140,7,{0x51,0x56,0x8b,0xf1,0x83,0x3e,0x00},NativePhase::SampledRenderOperation},
    {"?ExecuteAllInstances@eCVertexbufferPoolItemMain@@SGXXZ",0x1db2f0,9,{0x51,0x56,0x57,0x8b,0x3d,0xb8,0xa5,0x60,0x10},NativePhase::SampledInstanceBatches},
    {"?BeginMaterial@eCShaderMaterialAdmin@@QAE_NXZ",0x2378e0,5,{0x51,0x53,0x55,0x8b,0xd9},NativePhase::SampledMaterialBegin},
    {"?EndMaterial@eCShaderMaterialAdmin@@QAE_NXZ",0x237a60,11,{0x51,0x56,0x8b,0xf1,0x80,0xbe,0x29,0x06,0x00,0x00,0x00},NativePhase::SampledMaterialEnd},
    {"?Execute@eCGfxEffect@@QAE?AW4bEResult@@XZ",0x8f010,7,{0x56,0x8b,0xf1,0x83,0x7e,0x10,0x00},NativePhase::SampledEffectExecute},
    {"?Execute@eCGfxTextureBlock@@QAE?AW4bEResult@@XZ",0x966e0,9,{0x56,0x8b,0xf1,0x8b,0x86,0x94,0x00,0x00,0x00},NativePhase::SampledTextureExecute},
    {"?Execute@eCGfxVisual@@QAE?AW4bEResult@@XZ",0x97e00,6,{0x56,0x8b,0xf1,0x8b,0x4e,0x20},NativePhase::SampledVisualExecute},
    {"?RenderDynamicOccludees@eCOcclusionProcessor@@QAEXAAV?$bTPtrArray@PAVeCFrustumElementItem@@@@@Z",0x1a4830,7,{0x83,0xec,0x20,0x8b,0x44,0x24,0x24},NativePhase::SampledDynamicOccludees},
    {"?RenderDecals@eCDecalAdmin@@QAE_NPAVeCFrustumDatabase@@@Z",0x6fd80,6,{0x55,0x8b,0xec,0x83,0xe4,0xf8},NativePhase::SampledDecals},
    {"?RenderShadowMap@eCShadowMapBase@@UAE_NXZ",0x103d50,5,{0x56,0x8b,0xf1,0x8b,0x06},NativePhase::SampledShadowMap},
    {"?RenderLowPoly@eCCameraBase@@QAEXXZ",0x5ec80,6,{0x55,0x8b,0xec,0x83,0xe4,0xf8},NativePhase::SampledLowPoly},
    {"?BeginZPass@eCShaderMaterialAdmin@@QAE_NPAVeCDepthMap@@_N@Z",0x237ab0,5,{0x83,0xec,0x24,0x53,0x55},NativePhase::SampledZPassBegin},
    {"?EndZPass@eCShaderMaterialAdmin@@QAE_NXZ",0x237d80,5,{0x51,0x53,0x55,0x8b,0xd9},NativePhase::SampledZPassEnd},
    {"?RenderZPass@eCRenderContextAnimation@@UAE_N_N@Z",0x447e0,7,{0x83,0xec,0x08,0x57,0x8b,0x79,0x20},NativePhase::SampledAnimatedZPass},
    {"?RenderZPass@eCRenderContextMesh@@UAE_N_N@Z",0x1d9170,6,{0x83,0xec,0x0c,0x53,0x56,0x57},NativePhase::SampledMeshZPass},
    {"?ExecuteZPass@eCRenderOp@@QAEXABVeCFrustumDatabase@@@Z",0x1abcb0,6,{0x56,0x8b,0xf1,0x8b,0x46,0x0c},NativePhase::SampledRenderOperationZ},
    {"?DoBeginRenderSolid@eCModuleAdmin@@QAEXXZ",0x1d8a0,7,{0x56,0x57,0x8b,0xf9,0x8b,0x47,0x38},NativePhase::SampledSolidBegin}
};
inline constexpr unsigned ExecutionEntryCount=sizeof(ExecutionEntries)/sizeof(ExecutionEntries[0]);
inline constexpr uint32_t ExecutionPreferredBase=0x10000000;
inline constexpr uint32_t InstanceBatchGlobalRva=0x60a5b8;
inline constexpr unsigned InstanceBatchAddressOffset=5;
// The loader relocates the absolute MOV operand at ExecuteAllInstances+5.
// Adjust only that verified HIGHLOW relocation, not opcodes or branch targets.
// Unknown instructions and existing detours must still fail the full match.
inline bool ExecutionPrefixAtBase(ExecutionEntry const& entry,uint32_t base,unsigned char (&output)[16])noexcept {
    if(!base||entry.bytes<5||entry.bytes>sizeof(output))return false;
    std::memcpy(output,entry.prefix,sizeof(output));
    if(entry.phase==NativePhase::SampledInstanceBatches){
        if(entry.rva!=0x1db2f0||entry.bytes!=9||base>UINT32_MAX-InstanceBatchGlobalRva)return false;
        uint32_t stored=0;std::memcpy(&stored,output+InstanceBatchAddressOffset,sizeof(stored));
        if(stored!=ExecutionPreferredBase+InstanceBatchGlobalRva)return false;
        uint32_t relocated=base+InstanceBatchGlobalRva;
        std::memcpy(output+InstanceBatchAddressOffset,&relocated,sizeof(relocated));
    }
    return true;
}
// Time the entire selected stereo frame, preserving nested scopes and both
// eyes. Temporal diagnostic readbacks are explicitly excluded by the caller.
inline constexpr unsigned ExecutionSamplePeriod=32;
inline bool SampleSceneExecution(uint64_t stereoEpoch,int context,bool temporal)noexcept {
    return context==1&&!temporal&&stereoEpoch&&stereoEpoch%ExecutionSamplePeriod==0;
}
}
