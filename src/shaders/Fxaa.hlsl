// NVIDIA FXAA 3.11. See third_party/upscalers/fxaa/LICENSE.txt.
// Quality 12 with conservative subpixel smoothing for VR textural detail.
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 12
#define FXAA_GREEN_AS_LUMA 1
#include "Fxaa3_11.h"

Texture2D image : register(t0);
SamplerState linearClamp : register(s0);
cbuffer FxaaConstants : register(b0) { float2 inverseSize; float2 unused; }

float3 DecodeSrgb(float3 encoded) {
    return float3(encoded.r<=0.04045?encoded.r/12.92:pow((encoded.r+0.055)/1.055,2.4),
                  encoded.g<=0.04045?encoded.g/12.92:pow((encoded.g+0.055)/1.055,2.4),
                  encoded.b<=0.04045?encoded.b/12.92:pow((encoded.b+0.055)/1.055,2.4));
}
float4 ps(float4 position:SV_Position):SV_Target {
    FxaaTex source;source.tex=image;source.smpl=linearClamp;
    // Input is the finished SDR scene in encoded RGB, not an sRGB-decoding SRV.
    float4 color=FxaaPixelShader(position.xy*inverseSize,0,source,source,source,source,
        inverseSize,0,0,0,0.25,0.166,0.0625,0,0,0,0);
    // The OpenXR sRGB RTV encodes once on store. Preserve the original alpha.
    return float4(DecodeSrgb(saturate(color.rgb)),image.Load(int3(int2(position.xy),0)).a);
}
