Texture2D<float4> FilteredEye : register(t0);
Texture2D<float4> OriginalEye : register(t1);
Texture2D<float> RawBackground : register(t2);
cbuffer ProtectedOverlay : register(b0) { uint4 Overlay,PreviousOverlay;uint DecodeSrgb;uint3 Padding; };
float3 Linearize(float3 v) { return float3(v.x<=.04045?v.x/12.92:pow(max(0,(v.x+.055)/1.055),2.4),v.y<=.04045?v.y/12.92:pow(max(0,(v.y+.055)/1.055),2.4),v.z<=.04045?v.z/12.92:pow(max(0,(v.z+.055)/1.055),2.4)); }
float4 VS(uint id:SV_VertexID):SV_Position {
    float2 p=float2((id<<1)&2,id&2);
    return float4(p*float2(2,-2)+float2(-1,1),0,1);
}
float4 PS(float4 p:SV_Position):SV_Target {
    // Linear texture -> sRGB RTV performs exactly one gamma encoding.
    uint2 pixel=uint2(p.xy);
    bool raw=RawBackground.Load(int3(pixel,0))>.5||(pixel.x>=Overlay.x&&pixel.y>=Overlay.y&&pixel.x<Overlay.z&&pixel.y<Overlay.w)||
             (pixel.x>=PreviousOverlay.x&&pixel.y>=PreviousOverlay.y&&pixel.x<PreviousOverlay.z&&pixel.y<PreviousOverlay.w);
    float3 color=raw?OriginalEye.Load(int3(pixel,0)).rgb:FilteredEye.Load(int3(pixel,0)).rgb;
    return float4(raw&&DecodeSrgb!=0?Linearize(color):color,1);
}
