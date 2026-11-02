// Conservative composition: static camera reprojection, native skinned and
// rigid/instanced/wind geometry. Unknown/conflicting geometry is not granted
// valid temporal history. This shader does not infer static classification.
cbuffer Frame : register(b0) {
    row_major float4x4 CurrentClipToPreviousClip;
    float4 Pixel; // width,height,previous-current jitter in pixels
    uint HistoryValid;uint NativeAvailable;uint GeometryJitterIncluded;uint Padding;
};
Texture2D<float> Depth : register(t0);
Texture2D<float4> Skin : register(t1);
Texture2D<float> KnownStatic : register(t2);
Texture2D<float4> NativeGeometry : register(t3);
RWTexture2D<float4> Motion : register(u0);
[numthreads(8,8,1)]
void Compose(uint3 id:SV_DispatchThreadID) {
    if(any(id.xy>=uint2(Pixel.xy)))return;
    int3 p=int3(id.xy,0);
    float4 skin=Skin.Load(p),native=NativeAvailable!=0?NativeGeometry.Load(p):float4(0,0,0,0);
    bool malformed=!all(isfinite(skin))||any(skin.ba!=floor(skin.ba))||any(skin.ba<0)||any(skin.ba>1)||skin.a>skin.b;
    malformed=malformed||!all(isfinite(native))||any(native.ba!=floor(native.ba))||any(native.ba<0)||any(native.ba>1)||native.a>native.b;
    if(malformed) {Motion[id.xy]=0;return;}
    if(skin.b==1&&native.b==1) {
        bool valid=skin.a==1&&native.a==1&&HistoryValid!=0
            &&length(skin.rg)<=length(Pixel.xy)+2&&length(native.rg)<=length(Pixel.xy)+2
            &&length(skin.rg-native.rg)<=.25;
        Motion[id.xy]=float4(valid?skin.rg-(GeometryJitterIncluded!=0?Pixel.zw:float2(0,0)):float2(0,0),1,valid?1:0);return;
    }
    if(skin.b==0)skin=native;
    // Skin occupancy wins even when history is invalid: no static fallback.
    if(skin.b>.5) {
        bool valid=all(isfinite(skin))&&skin.b==1&&skin.a==1&&HistoryValid!=0
            &&length(skin.rg)<=length(Pixel.xy)+2;
        Motion[id.xy]=float4(valid?skin.rg-(GeometryJitterIncluded!=0?Pixel.zw:float2(0,0)):float2(0,0),1,valid?1:0);return;
    }
    float z=Depth.Load(p),known=KnownStatic.Load(p);
    if(known!=1||!isfinite(z)||z<0||z>=1) {Motion[id.xy]=0;return;}
    float2 ndc=(float2(id.xy)+.5)/Pixel.xy*float2(2,-2)+float2(-1,1);
    float4 previous=mul(float4(ndc,z,1),CurrentClipToPreviousClip);
    bool valid=HistoryValid!=0&&all(isfinite(previous))&&previous.w>1.e-5
        &&previous.z>=0&&previous.z<=previous.w&&all(abs(previous.xy)<=previous.w);
    float2 movement=valid?(previous.xy/previous.w-ndc)*Pixel.xy*float2(.5,-.5)-Pixel.zw:float2(0,0);
    Motion[id.xy]=float4(movement,1,valid?1:0);
}
