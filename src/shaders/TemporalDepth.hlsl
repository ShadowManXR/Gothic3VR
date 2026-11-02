#ifdef PACKED24
Texture2D<uint> Input:register(t0);
#else
Texture2D<float> Input:register(t0);
#endif
RWTexture2D<float> Output:register(u0);
[numthreads(8,8,1)]
void Decode(uint3 id:SV_DispatchThreadID){
    uint w,h;Output.GetDimensions(w,h);if(id.x>=w||id.y>=h)return;
#ifdef PACKED24
    float z=float(Input.Load(int3(id.xy,0))&0x00ffffffu)*(1.0/16777215.0);
#else
    float z=Input.Load(int3(id.xy,0));
#endif
    Output[id.xy]=isfinite(z)&&z>=0&&z<=1?z:1;
}
