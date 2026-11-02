cbuffer Packing : register(b0) {
    uint Width,Height,DecodeSrgb,MotionPolicy;
    row_major float4x4 CurrentClipToPreviousClip;
    float4 Camera; // previous-current raster jitter, camera history, unused.
    uint4 Overlay,PreviousOverlay;
};
Texture2D<float4> Color : register(t0);
Texture2D<float4> Motion : register(t1);
Texture2D<float> Depth : register(t2);
Texture2D<float> ReactiveMask : register(t3);
RWTexture2D<float4> LinearColor : register(u0);
RWTexture2D<float2> PixelMotion : register(u1);
RWTexture2D<float> PackedReactive : register(u2);
RWTexture2D<float> RawBackground : register(u3);
float3 Linearize(float3 v) {
    return float3(v.x<=.04045?v.x/12.92:pow(max(0,(v.x+.055)/1.055),2.4),
                  v.y<=.04045?v.y/12.92:pow(max(0,(v.y+.055)/1.055),2.4),
                  v.z<=.04045?v.z/12.92:pow(max(0,(v.z+.055)/1.055),2.4));
}
[numthreads(8,8,1)]
void Pack(uint3 id:SV_DispatchThreadID) {
    if(id.x>=Width||id.y>=Height)return;
    float4 color=Color.Load(int3(id.xy,0));
    LinearColor[id.xy]=float4(DecodeSrgb!=0?Linearize(color.rgb):color.rgb,1);
    float4 motion=Motion.Load(int3(id.xy,0));float reactive=ReactiveMask.Load(int3(id.xy,0));
    float z=Depth.Load(int3(id.xy,0));
    // Presentation exclusion, NOT a claim that clear depth identifies sky or
    // static material. Keep all clear, unoccupied background in native color.
    bool background=MotionPolicy!=0&&z==1&&motion.b==0&&motion.a==0;
    RawBackground[id.xy]=background?1:0;
    bool geometry=all(isfinite(motion))&&motion.b==1&&motion.a==1&&length(motion.rg)<=length(float2(Width,Height))+2;
    float2 movement=motion.rg;
    if(MotionPolicy!=0&&!geometry){
        // Native D3D9 integer raster centers, not a D3D11 +.5 sample offset.
        float2 ndc=float2(id.xy)/float2(Width,Height)*float2(2,-2)+float2(-1,1);
        float4 previous=mul(float4(ndc,z,1),CurrentClipToPreviousClip);
        bool valid=Camera.z!=0&&isfinite(z)&&z>=0&&z<=1&&all(isfinite(previous))&&previous.w>1.e-5&&previous.z>=0&&previous.z<=previous.w&&all(abs(previous.xy)<=previous.w);
        movement=valid?(previous.xy/previous.w-ndc)*float2(Width,Height)*float2(.5,-.5)-Camera.xy:float2(0,0);
        reactive=1; // Unsupported geometry remains explicitly reactive.
    }
    bool overlay=(id.x>=Overlay.x&&id.y>=Overlay.y&&id.x<Overlay.z&&id.y<Overlay.w)||
                 (id.x>=PreviousOverlay.x&&id.y>=PreviousOverlay.y&&id.x<PreviousOverlay.z&&id.y<PreviousOverlay.w);
    PixelMotion[id.xy]=movement;PackedReactive[id.xy]=overlay?1:reactive;
}
