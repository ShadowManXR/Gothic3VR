// Reverse GPU transport; shares image-import helpers, not production bridge symbols.
#include "../../../src/DxvkInteropResources.h"
#include "NativeWriteback.h"
#include <d3dcompiler.h>

namespace g3vr::dx11 {
namespace {
using Microsoft::WRL::ComPtr;
using namespace g3vr::dxvkinterop;
void Check(HRESULT hr,char const* text){stereo::Hr(hr,text);}
ComPtr<ID3DBlob> Shader(char const* source,char const* entry,char const* profile){
    UINT flags=D3DCOMPILE_IEEE_STRICTNESS | (strcmp(profile,"ps_3_0")==0?D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY:D3DCOMPILE_ENABLE_STRICTNESS);
    ComPtr<ID3DBlob> code,error;auto hr=D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,entry,profile,flags,0,&code,&error);
    if(FAILED(hr))throw std::runtime_error(error?static_cast<char const*>(error->GetBufferPointer()):"Shader compile failed");return code;
}
char const* packShader=R"(
Texture2D<float> depthMap:register(t0);
float4 VS(uint id:SV_VertexID):SV_Position {return float4(id==2?3:-1,id==1?3:-1,0,1);}
float4 PS(float4 p:SV_Position):SV_Target {
 // At far depths float32 cannot represent q+0.5. floor(q+0.5) rounds
 // odd D24 integers upwards before floor runs, losing equality with a prepass.
 float d=depthMap.Load(int3(p.xy,0));uint q=(uint)min(16777215.0,round(saturate(d)*16777215.0));
 return float4(q&255,(q>>8)&255,(q>>16)&255,d<1?255:0)/255.0;
})";
char const* compositeShader=R"(
sampler2D colorMap:register(s0);sampler2D depthMap:register(s1);
struct Output {float4 color:COLOR0;float depth:DEPTH;};
Output PS(float2 uv:TEXCOORD0) {
 float4 p=tex2D(depthMap,uv);clip(p.a-0.5);Output o;
 // Recover integer bytes first. A normalized dot and a rounded reciprocal
 // introduce another D24 error. Split the normalization into exact powers of2.
 o.color=tex2D(colorMap,uv);float3 bytes=floor(p.rgb*255.0+0.5);float q=dot(bytes,float3(1,256,65536));float base=q*(1.0/16777216.0);o.depth=base+base*(1.0/16777216.0);return o;
})";
struct Restore9 {
    IDirect3DDevice9Ex* device;ComPtr<IDirect3DStateBlock9> block;
    ComPtr<IDirect3DSurface9> rt[4],depth;D3DVIEWPORT9 viewport{};RECT scissor{};unsigned count=0;
    ComPtr<IDirect3DVertexBuffer9> stream;UINT streamOffset=0,streamStride=0,streamFrequency=1;
    Restore9(IDirect3DDevice9Ex* d,IDirect3DStateBlock9* saved):device(d),block(saved){
        D3DCAPS9 caps{};Check(d->GetDeviceCaps(&caps),"Native caps");count=std::min(4u,unsigned(caps.NumSimultaneousRTs));
        Check(block->Capture(),"Capture native state");
        for(unsigned i=0;i<count;++i){auto hr=d->GetRenderTarget(i,&rt[i]);Require(SUCCEEDED(hr)||hr==D3DERR_NOTFOUND,"Get native RT");}
        auto hr=d->GetDepthStencilSurface(&depth);Require(SUCCEEDED(hr)||hr==D3DERR_NOTFOUND,"Get native depth");
        Check(d->GetViewport(&viewport),"Get native viewport");Check(d->GetScissorRect(&scissor),"Get native scissor");
        // DrawPrimitiveUP clears stream 0. Preserve it explicitly: a reused
        // state block created with an empty stream does not restore this on all drivers.
        Check(d->GetStreamSource(0,&stream,&streamOffset,&streamStride),"Get native stream");Check(d->GetStreamSourceFreq(0,&streamFrequency),"Get native stream frequency");
    }
    ~Restore9(){device->SetDepthStencilSurface(nullptr);for(unsigned i=1;i<count;++i)device->SetRenderTarget(i,nullptr);
        for(unsigned i=0;i<count;++i)device->SetRenderTarget(i,rt[i].Get());device->SetDepthStencilSurface(depth.Get());
        block->Apply();device->SetStreamSource(0,stream.Get(),streamOffset,streamStride);device->SetStreamSourceFreq(0,streamFrequency);device->SetViewport(&viewport);device->SetScissorRect(&scissor);}
};
struct QuadVertex {float x,y,z,rhw,u,v;};
}
struct NativeWriteback::Impl {
    ComPtr<IDirect3DDevice9Ex> d9;ComPtr<ID3D11Device> d11;ComPtr<ID3D11DeviceContext> context;
    unsigned width=0,height=0;PassStamp ready{};bool failed=false;
    std::unique_ptr<Vulkan> vk;std::unique_ptr<Image> images[2];SharedEye legacy[2];
    ComPtr<IDirect3DQuery9> fence9;ComPtr<ID3D11Query> fence11;
    ComPtr<ID3D11Texture2D> packed;ComPtr<ID3D11RenderTargetView> packedRT;
    ComPtr<ID3D11VertexShader> packVS;ComPtr<ID3D11PixelShader> packPS;
    ComPtr<ID3D11RasterizerState> packRaster;ComPtr<ID3D11DepthStencilState> packDepth;
    ComPtr<IDirect3DPixelShader9> compositePS;
    ComPtr<IDirect3DStateBlock9> nativeState;
    bool ReadyToDestroy()noexcept {return !vk||vk->ReadyToDestroy();}
    IDirect3DTexture9* Texture(unsigned i){return vk?images[i]->source.Get():legacy[i].texture9.Get();}
    ID3D11Texture2D* Export(unsigned i){return vk?images[i]->texture11.Get():legacy[i].texture11.Get();}
    void Validate(ID3D11Texture2D* texture,DXGI_FORMAT format){
        Require(texture!=nullptr,"Missing producer texture");D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);ComPtr<ID3D11Device> owner;texture->GetDevice(&owner);
        bool matchingFormat=desc.Format==format||(format==DXGI_FORMAT_B8G8R8A8_UNORM&&desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
        Require(owner.Get()==d11.Get()&&desc.Width==width&&desc.Height==height&&matchingFormat&&desc.SampleDesc.Count==1&&desc.MipLevels==1&&desc.ArraySize==1,"Producer device/format/dimensions mismatch");
    }
    void Pack(ID3D11ShaderResourceView* depth){
        Require(depth!=nullptr,"Missing producer depth");ComPtr<ID3D11Resource> resource;depth->GetResource(&resource);ComPtr<ID3D11Texture2D> texture;Check(resource.As(&texture),"Producer depth texture");
        Validate(texture.Get(),DXGI_FORMAT_R32_TYPELESS);D3D11_SHADER_RESOURCE_VIEW_DESC srv{};depth->GetDesc(&srv);
        Require(srv.Format==DXGI_FORMAT_R32_FLOAT&&srv.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D&&srv.Texture2D.MostDetailedMip==0,"Producer depth SRV format");
        context->OMSetRenderTargets(0,nullptr,nullptr);ID3D11RenderTargetView* rt=packedRT.Get();context->OMSetRenderTargets(1,&rt,nullptr);
        D3D11_VIEWPORT viewport{0,0,float(width),float(height),0,1};context->RSSetViewports(1,&viewport);context->RSSetState(packRaster.Get());
        context->OMSetBlendState(nullptr,nullptr,UINT_MAX);context->OMSetDepthStencilState(packDepth.Get(),0);
        context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(packVS.Get(),nullptr,0);context->HSSetShader(nullptr,nullptr,0);context->DSSetShader(nullptr,nullptr,0);context->GSSetShader(nullptr,nullptr,0);context->PSSetShader(packPS.Get(),nullptr,0);
        context->PSSetShaderResources(0,1,&depth);context->Draw(3,0);ID3D11ShaderResourceView* empty=nullptr;context->PSSetShaderResources(0,1,&empty);context->OMSetRenderTargets(0,nullptr,nullptr);
    }
    void SubmitReverse(){
        auto& v=*vk;VkCheck(v.EndCommandBuffer(v.commands),"End reverse copy");VkCheck(v.ResetFences(v.device,1,&v.fence),"Reset reverse fence");
        VkDeviceMemory memory[]{images[0]->allocation,images[1]->allocation};uint64_t acquire[]{1,1},release[]{0,0};uint32_t timeout[]{2000,2000};
        VkWin32KeyedMutexAcquireReleaseInfoKHR mutex{VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR};
        mutex.acquireCount=mutex.releaseCount=2;mutex.pAcquireSyncs=mutex.pReleaseSyncs=memory;mutex.pAcquireKeys=acquire;mutex.pReleaseKeys=release;mutex.pAcquireTimeouts=timeout;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.pNext=&mutex;submit.commandBufferCount=1;submit.pCommandBuffers=&v.commands;
        v.interop->LockSubmissionQueue();auto result=v.QueueSubmit(v.queue,1,&submit,v.fence);v.interop->ReleaseSubmissionQueue();VkCheck(result,"Submit reverse copy");v.inFlight=true;
        VkCheck(v.WaitForFences(v.device,1,&v.fence,VK_TRUE,2000000000ull),"Reverse copy completion");v.inFlight=false;
    }
    void ReverseRecord(Image& i){
        VkImageMemoryBarrier b[2]{};for(auto& x:b){x.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;x.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
        b[0].image=i.image;b[0].oldLayout=VK_IMAGE_LAYOUT_GENERAL;b[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b[0].srcQueueFamilyIndex=VK_QUEUE_FAMILY_EXTERNAL;b[0].dstQueueFamilyIndex=vk->family;b[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        b[1].image=i.sourceImage;b[1].oldLayout=b[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b[1].srcQueueFamilyIndex=b[1].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b[1].srcAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;b[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vk->CmdPipelineBarrier(vk->commands,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,b);
        VkImageCopy region{};region.srcSubresource=region.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};region.extent=i.extent;
        vk->CmdCopyImage(vk->commands,i.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,i.sourceImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
        b[0].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b[0].newLayout=VK_IMAGE_LAYOUT_GENERAL;b[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b[0].dstAccessMask=0;b[0].srcQueueFamilyIndex=vk->family;b[0].dstQueueFamilyIndex=VK_QUEUE_FAMILY_EXTERNAL;
        b[1].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b[1].newLayout=i.originalLayout;b[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b[1].dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        vk->CmdPipelineBarrier(vk->commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,2,b);
    }
};
NativeWriteback::NativeWriteback()=default;
NativeWriteback::~NativeWriteback(){Reset();}
void NativeWriteback::Reset()noexcept {if(impl&&!impl->ReadyToDestroy())impl.release();else impl.reset();}
bool NativeWriteback::UsesDxvk()const{return impl&&bool(impl->vk);}
void NativeWriteback::Initialize(IDirect3DDevice9Ex* d9,ID3D11Device* d11,ID3D11DeviceContext* context,unsigned width,unsigned height){
    Require(d9&&d11&&context&&width&&height&&width<=8192&&height<=8192,"Invalid reverse bridge initialization");
    ComPtr<ID3D11Device> owner;context->GetDevice(&owner);Require(owner.Get()==d11&&context->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Reverse bridge needs matching immediate context");
    ComPtr<IDirect3D9> api9;Check(d9->GetDirect3D(&api9),"Native API");ComPtr<IDirect3D9Ex> apiEx;Check(api9.As(&apiEx),"Native Ex API");D3DDEVICE_CREATION_PARAMETERS creation{};Check(d9->GetCreationParameters(&creation),"Native creation parameters");LUID a{};Check(apiEx->GetAdapterLUID(creation.AdapterOrdinal,&a),"Native adapter LUID");
    ComPtr<IDXGIDevice> dxgi;Check(d11->QueryInterface(IID_PPV_ARGS(&dxgi)),"Producer DXGI device");ComPtr<IDXGIAdapter> adapter;Check(dxgi->GetAdapter(&adapter),"Producer adapter");DXGI_ADAPTER_DESC desc{};Check(adapter->GetDesc(&desc),"Producer adapter description");Require(a.HighPart==desc.AdapterLuid.HighPart&&a.LowPart==desc.AdapterLuid.LowPart,"Cross-adapter reverse copy refused");
    auto s=std::make_unique<Impl>();s->d9=d9;s->d11=d11;s->context=context;s->width=width;s->height=height;
    Check(d9->CreateQuery(D3DQUERYTYPE_EVENT,&s->fence9),"Reverse D3D9 fence");D3D11_QUERY_DESC query{D3D11_QUERY_EVENT,0};Check(d11->CreateQuery(&query,&s->fence11),"Reverse DX11 fence");
    Check(d9->CreateStateBlock(D3DSBT_ALL,&s->nativeState),"Persistent native state block");
    ComPtr<ID3D9VkInteropDevice> interop;auto hr=d9->QueryInterface(IID_PPV_ARGS(&interop));if(hr!=E_NOINTERFACE){Check(hr,"Detect DXVK interop");s->vk=std::make_unique<Vulkan>(d9);}
    for(unsigned i=0;i<2;++i){if(s->vk){s->images[i]=std::make_unique<Image>(*s->vk);auto& image=*s->images[i];image.Create(d9,d11,width,height);
            VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};Check(image.interop->GetVulkanImageInfo(nullptr,nullptr,&info),"Reverse backing properties");Require((info.usage&VK_IMAGE_USAGE_TRANSFER_DST_BIT)!=0,"Native image cannot receive GPU writeback");
            // Establish initialized native contents/layout through the owning API.
            Check(d9->ColorFill(image.surface.Get(),nullptr,0),"Initialize reverse destination");}
        else s->legacy[i].Create(d9,d11,width,height);}
    D3D11_TEXTURE2D_DESC td{};td.Width=width;td.Height=height;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
    Check(d11->CreateTexture2D(&td,nullptr,&s->packed),"Packed depth texture");Check(d11->CreateRenderTargetView(s->packed.Get(),nullptr,&s->packedRT),"Packed depth RTV");
    auto vs=Shader(packShader,"VS","vs_5_0"),ps=Shader(packShader,"PS","ps_5_0"),native=Shader(compositeShader,"PS","ps_3_0");
    Check(d11->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&s->packVS),"Pack VS");Check(d11->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&s->packPS),"Pack PS");
    Check(d9->CreatePixelShader(static_cast<DWORD const*>(native->GetBufferPointer()),&s->compositePS),"Native color/depth PS");
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;Check(d11->CreateRasterizerState(&raster,&s->packRaster),"Pack rasterizer");
    D3D11_DEPTH_STENCIL_DESC depth{};depth.DepthEnable=FALSE;Check(d11->CreateDepthStencilState(&depth,&s->packDepth),"Pack depth state");
    Reset();impl=std::move(s);
}
void NativeWriteback::Transfer(ID3D11Texture2D* color,ID3D11ShaderResourceView* depth,PassStamp const& stamp){
    Require(impl!=nullptr,"Reverse bridge is not initialized");auto& s=*impl;s.ready={};Require(!s.failed&&stamp.Valid(),"Invalid reverse bridge/frame");s.Validate(color,DXGI_FORMAT_B8G8R8A8_UNORM);s.Pack(depth);
    s.failed=true; // A partial ownership transfer cannot be retried using the old stamp.
    if(s.vk){
        for(auto& image:s.images){Require(image->nativeMutex->AcquireSync(0,2000)==S_OK,"Reverse producer mutex timeout");image->readHeld=true;}
        s.context->CopyResource(s.Export(0),color);s.context->CopyResource(s.Export(1),s.packed.Get());Wait11(s.context.Get(),s.fence11.Get());
        for(auto& image:s.images){Check(image->nativeMutex->ReleaseSync(1),"Release reverse producer mutex");image->readHeld=false;
            Check(image->interop->GetVulkanImageInfo(&image->sourceImage,&image->originalLayout,nullptr),"Reverse native layout");Require(image->originalLayout!=VK_IMAGE_LAYOUT_UNDEFINED,"Uninitialized native writeback image");
            VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};s.vk->interop->TransitionTextureLayout(image->interop.Get(),&range,image->originalLayout,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);}
        Wait9(s.fence9.Get());s.vk->Begin();for(auto& image:s.images)s.ReverseRecord(*image);s.SubmitReverse();
    }else{Wait9(s.fence9.Get());s.context->CopyResource(s.Export(0),color);s.context->CopyResource(s.Export(1),s.packed.Get());Wait11(s.context.Get(),s.fence11.Get());}
    s.ready=stamp;s.failed=false;
}
void NativeWriteback::Composite(PassStamp const& expected,bool preserveNativeAlpha){
    Require(impl&&!impl->failed&&expected.Valid()&&impl->ready==expected,"No current native color/depth for this eye/pass");auto& s=*impl;
    ComPtr<IDirect3DSurface9> rt,depth;Check(s.d9->GetRenderTarget(0,&rt),"Composite target");Check(s.d9->GetDepthStencilSurface(&depth),"Composite depth");D3DSURFACE_DESC r{},z{};Check(rt->GetDesc(&r),"Composite target description");Check(depth->GetDesc(&z),"Composite depth description");
    Require(r.Width==s.width&&r.Height==s.height&&z.Width==s.width&&z.Height==s.height&&r.MultiSampleType==D3DMULTISAMPLE_NONE&&z.MultiSampleType==D3DMULTISAMPLE_NONE&&(r.Format==D3DFMT_A8R8G8B8||r.Format==D3DFMT_X8R8G8B8)&&(z.Format==D3DFMT_D24S8||z.Format==D3DFMT_D24X8),"Unsupported native target/depth");
    Restore9 restore(s.d9.Get(),s.nativeState.Get());auto* d=s.d9.Get();for(unsigned i=1;i<restore.count;++i)Check(d->SetRenderTarget(i,nullptr),"Disable MRT");
    auto state=[&](D3DRENDERSTATETYPE t,DWORD value){Check(d->SetRenderState(t,value),"Native composite state");};
    state(D3DRS_ZENABLE,TRUE);state(D3DRS_ZWRITEENABLE,TRUE);state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);state(D3DRS_STENCILENABLE,FALSE);state(D3DRS_TWOSIDEDSTENCILMODE,FALSE);
    state(D3DRS_ALPHABLENDENABLE,FALSE);state(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);state(D3DRS_ALPHATESTENABLE,FALSE);state(D3DRS_FOGENABLE,FALSE);state(D3DRS_SCISSORTESTENABLE,FALSE);state(D3DRS_CLIPPLANEENABLE,0);
    state(D3DRS_CULLMODE,D3DCULL_NONE);state(D3DRS_FILLMODE,D3DFILL_SOLID);state(D3DRS_SRGBWRITEENABLE,FALSE);state(D3DRS_DEPTHBIAS,0);state(D3DRS_SLOPESCALEDEPTHBIAS,0);state(D3DRS_COLORWRITEENABLE,preserveNativeAlpha?7:15);
    state(D3DRS_DITHERENABLE,FALSE);state(D3DRS_MULTISAMPLEMASK,UINT_MAX);state(D3DRS_WRAP0,0);Check(d->SetStreamSourceFreq(0,1),"Composite stream frequency");
    D3DVIEWPORT9 viewport{0,0,s.width,s.height,0,1};Check(d->SetViewport(&viewport),"Composite viewport");Check(d->SetVertexShader(nullptr),"Composite VS");Check(d->SetPixelShader(s.compositePS.Get()),"Composite PS");Check(d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1),"Composite FVF");
    for(unsigned i=0;i<2;++i){Check(d->SetTexture(i,s.Texture(i)),"Composite texture");for(auto t:{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER})Check(d->SetSamplerState(i,t,D3DTEXF_POINT),"Point sampling");Check(d->SetSamplerState(i,D3DSAMP_MIPFILTER,D3DTEXF_NONE),"No mip filtering");for(auto t:{D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV})Check(d->SetSamplerState(i,t,D3DTADDRESS_CLAMP),"Clamp sampling");Check(d->SetSamplerState(i,D3DSAMP_SRGBTEXTURE,FALSE),"Encoded bytes sampling");}
    float w=float(s.width)-.5f,h=float(s.height)-.5f;QuadVertex q[]{{-.5f,-.5f,0,1,0,0},{w,-.5f,0,1,1,0},{-.5f,h,0,1,0,1},{w,h,0,1,1,1}};
    Check(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,q,sizeof(QuadVertex)),"Composite native color/depth");s.ready={};
}
}

