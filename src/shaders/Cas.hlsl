// AMD FidelityFX CAS, MIT licensed headers in third_party/upscalers/cas.
cbuffer Parameters : register(b0){uint4 const0;uint4 const1;uint2 inputSize;uint2 outputSize;};
Texture2D<float4> inputImage : register(t0);
RWTexture2D<unorm float4> outputImage : register(u0);
#define A_GPU 1
#define A_HLSL 1
#define CAS_BETTER_DIAGONALS 1
#include "ffx_a.h"
AF3 CasLoad(ASU2 p){return inputImage.Load(int3(clamp(p,int2(0,0),int2(inputSize)-1),0)).rgb;}
void CasInput(inout AF1 r,inout AF1 g,inout AF1 b){}
#include "ffx_cas.h"
void Pixel(uint2 p){
    if(any(p>=outputSize))return;
    float3 c;CasFilter(c.r,c.g,c.b,p,const0,const1,CAS_SHARPEN_ONLY!=0);
    outputImage[p]=float4(c,1);
}
[numthreads(64,1,1)]
void main(uint3 thread:SV_GroupThreadID,uint3 block:SV_GroupID){
    uint2 p=ARmp8x8(thread.x)+uint2(block.x<<4,block.y<<4);
    Pixel(p);Pixel(p+uint2(8,0));Pixel(p+uint2(8,8));Pixel(p+uint2(0,8));
}
