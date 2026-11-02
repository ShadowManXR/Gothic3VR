// D3D9 SM3 companion pass for the exact observed 64-bone skinning layout.
// Constant-register columns deliberately mirror the native dp4 assembly.
float4 CurrentBones[192] : register(c0);
float4 CurrentClip[4] : register(c192);
float4 PreviousClip[4] : register(c196);
float4 Atlas : register(c200); // inverse width, inverse height, row+.5, history valid
sampler2D PreviousBones : register(s0);
struct SkinInput {
    float3 position : POSITION0;
    float4 weights : BLENDWEIGHT0;
    float4 indices : BLENDINDICES0;
};
struct MotionVertex {
    float4 position : POSITION0;
    float4 current : TEXCOORD0;
    float4 previous : TEXCOORD1;
    float valid : TEXCOORD2;
};
float4 OldRow(float index,float row) {
    return tex2Dlod(PreviousBones,float4((index*3+row+.5)*Atlas.x,Atlas.z*Atlas.y,0,0));
}
float4 NewRow(float4 indices,float4 weights,int row) {
    float4 value=CurrentBones[int(indices.y)*3+row]*weights.y;
    value=mad(CurrentBones[int(indices.x)*3+row],weights.x,value);
    value=mad(CurrentBones[int(indices.z)*3+row],weights.z,value);
    return mad(CurrentBones[int(indices.w)*3+row],weights.w,value);
}
float4 PreviousRow(float4 indices,float4 weights,float row) {
    float4 value=OldRow(indices.y,row)*weights.y;
    value=mad(OldRow(indices.x,row),weights.x,value);
    value=mad(OldRow(indices.z,row),weights.z,value);
    return mad(OldRow(indices.w,row),weights.w,value);
}
MotionVertex MotionVS(SkinInput input) {
    MotionVertex result;
    float4 p=float4(input.position,1);
    float4 now=float4(dot(p,NewRow(input.indices,input.weights,0)),dot(p,NewRow(input.indices,input.weights,1)),dot(p,NewRow(input.indices,input.weights,2)),1);
    float4 old=float4(dot(p,PreviousRow(input.indices,input.weights,0)),dot(p,PreviousRow(input.indices,input.weights,1)),dot(p,PreviousRow(input.indices,input.weights,2)),1);
    result.current=float4(dot(now,CurrentClip[0]),dot(now,CurrentClip[1]),dot(now,CurrentClip[2]),dot(now,CurrentClip[3]));
    result.previous=float4(dot(old,PreviousClip[0]),dot(old,PreviousClip[1]),dot(old,PreviousClip[2]),dot(old,PreviousClip[3]));
    result.position=result.current;result.valid=Atlas.w;
    if(any(input.indices<0)||any(input.indices>63)||any(input.weights<0)||abs(dot(input.weights,1)-1)>.01)
        result.position=float4(0,0,0,-1);
    return result;
}
float4 PixelParameters : register(c0); // half width, negative half height, previous-current raster jitter in pixels
struct MotionPixel {float4 current:TEXCOORD0;float4 previous:TEXCOORD1;float valid:TEXCOORD2;};
float4 MotionPS(MotionPixel input) : COLOR0 {
    float valid=input.valid>.5&&input.previous.w>1.e-5&&input.previous.z>=0&&input.previous.z<=input.previous.w&&all(abs(input.previous.xy)<=input.previous.w);
    float2 previous=input.previous.xy/max(input.previous.w,1.e-5);
    float2 current=input.current.xy/max(input.current.w,1.e-5);
    float2 motion=valid?((previous-current)*PixelParameters.xy-PixelParameters.zw):float2(0,0);
    // RG: previous-current pixels; B: geometry occupied; A: previous pose valid.
    return float4(motion,1,valid);
}
float4 DepthFixturePS() : COLOR0 {return float4(.25,.5,.75,1);}
// Exact PS cef3519ea7bd097 alpha dependency: saturate(s3.a * COLOR0.a),
// D3DCMP_GREATER. The output alpha remains a history flag, never alpha test.
sampler2D NativeAlpha : register(s0);
float4 NativeAlphaParameters : register(c1); // reference / 255
float4 MaskedMotionPS(MotionPixel input,float2 uv:TEXCOORD3,float4 color:COLOR0) : COLOR0 {
    float alpha=saturate(tex2D(NativeAlpha,uv).a*color.a);
    clip(alpha>NativeAlphaParameters.x?1:-1);
    return MotionPS(input);
}
