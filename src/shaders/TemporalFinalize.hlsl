cbuffer Frame : register(b0) { uint Width,Height,SkyAvailable,SkyCameraHistory; };
Texture2D<float4> Opaque : register(t0);
Texture2D<float4> Sky : register(t1);
RWTexture2D<float4> Combined : register(u0);
RWTexture2D<float> Reactive : register(u1);
[numthreads(8,8,1)]
void Finalize(uint3 id:SV_DispatchThreadID) {
    if(id.x>=Width||id.y>=Height)return;
    int3 p=int3(id.xy,0);float4 base=Opaque.Load(p),sky=SkyAvailable!=0?Sky.Load(p):float4(0,0,0,0);
    float4 result=0;float reactive=1;
    bool malformed=!all(isfinite(base))||any(base.ba<0)||any(base.ba>1)||any(base.ba!=floor(base.ba))||base.a>base.b;
    malformed=malformed||!all(isfinite(sky))||sky.b<0||sky.b>1||sky.b!=floor(sky.b)||sky.a<0||sky.a>1;
    malformed=malformed||(base.b==0&&any(base!=0))||(sky.b==0&&any(sky!=0));
    float bound=length(float2(Width,Height))+2;
    if(!malformed) {
        if(base.b==1&&sky.b==1){result=float4(0,0,1,0);} // Conflicting layers never choose an arbitrary history.
        else if(base.b==1){bool valid=base.a==1&&length(base.rg)<=bound;result=float4(valid?base.rg:float2(0,0),1,valid?1:0);reactive=valid?0:1;}
        else if(sky.b==1){bool valid=SkyCameraHistory!=0&&length(sky.rg)<=bound;result=float4(valid?sky.rg:float2(0,0),1,valid?1:0);reactive=valid?sky.a:1;}
    }
    Combined[id.xy]=result;Reactive[id.xy]=reactive;
}
