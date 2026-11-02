cbuffer Metrics : register(b0) { float4 metrics; };
#define SMAA_RT_METRICS metrics
#define SMAA_HLSL_4
#include "SMAA.hlsl"
Texture2D colorTex : register(t0);
Texture2D edgesTex : register(t1);
Texture2D areaTex : register(t2);
Texture2D searchTex : register(t3);
Texture2D blendTex : register(t4);
struct EdgeVertex { float4 p:SV_Position;float2 uv:TEXCOORD0;float4 offset[3]:TEXCOORD1; };
struct WeightVertex { float4 p:SV_Position;float2 uv:TEXCOORD0;float2 pixel:TEXCOORD1;float4 offset[3]:TEXCOORD2; };
struct BlendVertex { float4 p:SV_Position;float2 uv:TEXCOORD0;float4 offset:TEXCOORD1; };
float4 Triangle(uint id) { return float4(id==1?3:-1,id==2?-3:1,0,1); }
float2 Texcoord(float4 p) { return float2(p.x*.5+.5,.5-p.y*.5); }
EdgeVertex EdgeVS(uint id:SV_VertexID) {
    EdgeVertex o;o.p=Triangle(id);o.uv=Texcoord(o.p);SMAAEdgeDetectionVS(o.uv,o.offset);return o;
}
float4 EdgePS(EdgeVertex i):SV_Target { return float4(SMAALumaEdgeDetectionPS(i.uv,i.offset,colorTex),0,0); }
WeightVertex WeightVS(uint id:SV_VertexID) {
    WeightVertex o;o.p=Triangle(id);o.uv=Texcoord(o.p);SMAABlendingWeightCalculationVS(o.uv,o.pixel,o.offset);return o;
}
float4 WeightPS(WeightVertex i):SV_Target {
    return SMAABlendingWeightCalculationPS(i.uv,i.pixel,i.offset,edgesTex,areaTex,searchTex,0);
}
BlendVertex BlendVS(uint id:SV_VertexID) {
    BlendVertex o;o.p=Triangle(id);o.uv=Texcoord(o.p);SMAANeighborhoodBlendingVS(o.uv,o.offset);return o;
}
float4 BlendPS(BlendVertex i):SV_Target {
    float4 result=SMAANeighborhoodBlendingPS(i.uv,i.offset,colorTex,blendTex);
    // Scene alpha may mask the compositor; spatial AA must not expand it.
    result.a=colorTex.Load(int3(int2(i.p.xy),0)).a;return result;
}
