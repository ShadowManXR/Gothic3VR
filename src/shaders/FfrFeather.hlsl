Texture2D<float4> scene : register(t0);
SamplerState linearClamp : register(s0);
cbuffer FeatherConstants : register(b0) {
    float2 inverseSize; float2 center;
    float radius; float band; float blurPixels; float enabled;
}
float4 ps(float4 position:SV_Position):SV_Target {
    float4 original=scene.Load(int3(int2(position.xy),0));
    float2 uv=position.xy*inverseSize;
    float2 delta=(uv-center)*2*float2(.816496581,1.224744871);
    float distance=length(delta);
    float t=saturate((distance-(radius-band*.25))/max(band,.00001));
    float amount=enabled*t*t*(3-2*t);
    [branch] if(amount<=0)return original;
    float2 step=inverseSize*blurPixels;
    // True pixel filtering, with continuous per-pixel weight. No rate-map
    // checkerboard, temporal noise or frame history. SRV/RTV handle sRGB.
    float3 blur=original.rgb*.25;
    blur+=(scene.SampleLevel(linearClamp,uv+float2(step.x,0),0).rgb+scene.SampleLevel(linearClamp,uv-float2(step.x,0),0).rgb
          +scene.SampleLevel(linearClamp,uv+float2(0,step.y),0).rgb+scene.SampleLevel(linearClamp,uv-float2(0,step.y),0).rgb)*.125;
    blur+=(scene.SampleLevel(linearClamp,uv+step,0).rgb+scene.SampleLevel(linearClamp,uv-step,0).rgb
          +scene.SampleLevel(linearClamp,uv+float2(step.x,-step.y),0).rgb+scene.SampleLevel(linearClamp,uv+float2(-step.x,step.y),0).rgb)*.0625;
    return float4(lerp(original.rgb,blur,amount),original.a);
}
