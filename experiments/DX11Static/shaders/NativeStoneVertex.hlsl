// Observed VS 7a13cd383a26eb4b. Native coordinates/constants are centimeters.
// This profile preserves the original tangent frame, auxiliary lightmap inputs,
// projected shadows and fog. Runtime replacement still needs resource parity.
cbuffer NativeVertex : register(b0) { float4 vc[256]; float4 rasterOffset; }
struct Input {
    float3 position:POSITION; float3 normal:NORMAL; float2 uv:TEXCOORD0;
    float4 color0:COLOR0; float3 tangent:TANGENT;
    float4 color1:COLOR1; float3 incident:TEXCOORD7; float2 lightmapUV:TEXCOORD6;
};
struct Output {
    float4 position:SV_POSITION;
    float4 v0:COLOR0; float4 v1:COLOR1; float4 v2:TEXCOORD0;
    float4 unused:TEXCOORD1; float4 v3:TEXCOORD2; float4 v4:TEXCOORD3;
    float4 v5:TEXCOORD4; float4 v6:TEXCOORD5;
    float4 v7:TEXCOORD6; float4 v8:TEXCOORD7;
};
Output VS(Input input) {
    Output output;
    // D3DCOLOR is BGRA in the native vertex byte stream; SM5 RGBA8 input
    // decoding needs this explicit swizzle to reproduce the D3D9 declaration.
    float4 color0=input.color0.bgra, color1=input.color1.bgra;
    float3 normal=normalize(float3(dot(input.normal,vc[12].xyz),dot(input.normal,vc[13].xyz),dot(input.normal,vc[14].xyz)));
    float3 tangent=normalize(float3(dot(input.tangent,vc[12].xyz),dot(input.tangent,vc[13].xyz),dot(input.tangent,vc[14].xyz)));
    float3 binormal=normalize(cross(normal,tangent)*(1-2*color0.r));
    float4 position=float4(input.position,1);
    float4 world=float4(dot(position,vc[12]),dot(position,vc[13]),dot(position,vc[14]),1);
    float4 clip=float4(dot(world,vc[0]),dot(world,vc[1]),dot(world,vc[2]),dot(world,vc[3]));
    output.position=clip;
    output.position.xy+=rasterOffset.xy*clip.w;
    output.v0=float4(binormal,color0.a);
    output.v1=color1*float4(4,4,4,1);
    output.v2=float4(input.uv*vc[17].xy,input.uv);
    output.unused=input.uv.xyxy;
    output.v3=float4(input.lightmapUV,input.incident.xy);
    output.v4=float4(world.xyz,saturate(clip.w*vc[16].z+vc[16].w));
    output.v5=float4(normal,input.incident.z);
    output.v6=float4(tangent,saturate(length(vc[15].xyz-world.xyz)*vc[16].y+vc[16].x));
    output.v7=float4(dot(world,vc[4]),dot(world,vc[5]),dot(world,vc[6]),dot(world,vc[7]));
    output.v8=float4(dot(world,vc[8]),dot(world,vc[9]),dot(world,vc[10]),dot(world,vc[11]));
    return output;
}
