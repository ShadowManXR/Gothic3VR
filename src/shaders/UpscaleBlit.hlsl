Texture2D<float4> image : register(t0);
float4 vs(uint id:SV_VertexID):SV_Position{
    return float4(id==1?3:-1,id==2?-3:1,0,1);
}
float4 ps(float4 p:SV_Position):SV_Target{return image.Load(int3(int2(p.xy),0));}
