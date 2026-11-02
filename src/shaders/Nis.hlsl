// NVIDIA NIS, MIT licensed headers in third_party/upscalers/nis.
#define NIS_HLSL 1
#ifndef NIS_SCALER
#define NIS_SCALER 1
#endif
#define NIS_HDR_MODE 0
#define NIS_BLOCK_WIDTH 32
#if NIS_SCALER
#define NIS_BLOCK_HEIGHT 24
#else
#define NIS_BLOCK_HEIGHT 32
#endif
#define NIS_THREAD_GROUP_SIZE 256
#define NIS_VIEWPORT_SUPPORT 1
Texture2D coef_scaler : register(t1);
Texture2D coef_usm : register(t2);
#include "NIS_Common.h"
#include "NIS_Scaler.h"
[numthreads(NIS_THREAD_GROUP_SIZE,1,1)]
void main(uint3 block : SV_GroupID,uint3 thread : SV_GroupThreadID){
#if NIS_SCALER
    NVScaler(block.xy,thread.x);
#else
    NVSharpen(block.xy,thread.x);
#endif
}
