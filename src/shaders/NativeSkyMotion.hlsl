float4 PreviousClip0 : register(c0);
float4 PreviousClip1 : register(c1);
float4 PreviousClip2 : register(c2);
float4 PreviousClip3 : register(c3);
float4 Pixel : register(c4); // half width,-half height,previous-current raster jitter.
float4 Flags : register(c5); // history-valid,whole-material-reactive,alpha-blend-coverage,cloud density.
sampler2D Cloud0 : register(s2);
sampler2D Cloud1 : register(s3);
float4 MotionPS(float2 uv0:TEXCOORD0,float2 uv1:TEXCOORD1,float3 nativeSky:TEXCOORD7,float4 current:TEXCOORD8):COLOR0 {
    float density=saturate(((tex2D(Cloud1,uv1).r+tex2D(Cloud0,uv0).r)*Flags.w-.2)*3.33333325);
    float cloud=density*density*(3-2*density);
    float alpha=saturate(cloud+nativeSky.y);
    // Only SRCALPHA/INVSRCALPHA source passes use this coverage gate.
    if(Flags.z!=0)clip(alpha>0?1:-1);
    float4 old=float4(dot(PreviousClip0,current),dot(PreviousClip1,current),dot(PreviousClip2,current),dot(PreviousClip3,current));
    bool valid=Flags.x!=0&&current.w>1.e-6&&old.w>1.e-6&&all(abs(old.xy)<=old.w);
    float2 movement=valid?(old.xy/old.w-current.xy/current.w)*Pixel.xy-Pixel.zw:float2(0,0);
    // B is actual draw coverage, A is REACTIVITY, not geometry-history validity.
    return float4(movement,1,!valid||Flags.y!=0?1:cloud);
}
