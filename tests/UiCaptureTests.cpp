#include "D3D9UiCapture.h"
#include <cstdio>
#include <d3dcompiler.h>
#include <iterator>
using stereo::ComPtr;
using stereo::Hr;
struct Vertex {float x,y,z,rhw;DWORD color;};
std::vector<unsigned char> Rgba(stereo::Renderer& r,ID3D11Texture2D* texture)
{
    D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;
    desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;Hr(r.device->CreateTexture2D(&desc,nullptr,&staging),"UI readback");
    r.context->CopyResource(staging.Get(),texture);D3D11_MAPPED_SUBRESOURCE map{};
    Hr(r.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"UI readback map");
    std::vector<unsigned char> pixels(desc.Width*desc.Height*4);
    for(UINT y=0;y<desc.Height;++y)memcpy(pixels.data()+y*desc.Width*4,static_cast<char const*>(map.pData)+y*map.RowPitch,desc.Width*4);
    r.context->Unmap(staging.Get(),0);return pixels;
}
void DrawUi(IDirect3DDevice9* d)
{
    Hr(d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE),"UI FVF");Hr(d->SetVertexShader(nullptr),"UI no VS");Hr(d->SetPixelShader(nullptr),"UI no PS");
    Hr(d->SetTexture(0,nullptr),"UI no texture");Hr(d->SetRenderState(D3DRS_LIGHTING,FALSE),"UI no lighting");
    Hr(d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE),"UI no culling");
    Hr(d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE),"UI alpha enabled");
    Hr(d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA),"UI src blend");
    Hr(d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA),"UI dst blend");
    Hr(d->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD),"UI blend op");
    Hr(d->SetRenderState(D3DRS_ZENABLE,FALSE),"UI native no depth");
    Hr(d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE),"UI native z write flag");
    Hr(d->SetRenderState(D3DRS_COLORWRITEENABLE,7),"UI native RGB-only writes");
    Hr(d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE),"UI native no separate alpha");
    Hr(d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE),"UI scissor enabled");
    RECT clip{0,0,64,64};Hr(d->SetScissorRect(&clip),"UI native scissor");
    Hr(d->BeginScene(),"UI BeginScene");
    WORD indices[]={0,1,2,2,1,3};
    for(unsigned kind=0;kind<4;++kind)
    {
        float x=kind%2?32.f:8.f,y=kind/2?32.f:8.f;
        DWORD colors[]={0x80ff0000,0xff00ff00,0xff000000,0xffffff00};
        Vertex v[]={{x,y,0,1,colors[kind]},{x+12,y,0,1,colors[kind]},
            {x,y+12,0,1,colors[kind]},{x+12,y+12,0,1,colors[kind]}};
        if(kind<2)
        {
            ComPtr<IDirect3DVertexBuffer9> vb;Hr(d->CreateVertexBuffer(sizeof(v),0,D3DFVF_XYZRHW|D3DFVF_DIFFUSE,D3DPOOL_DEFAULT,&vb,nullptr),"UI vertex buffer");
            void* data=nullptr;Hr(vb->Lock(0,0,&data,0),"UI VB lock");memcpy(data,v,sizeof(v));vb->Unlock();
            Hr(d->SetStreamSource(0,vb.Get(),0,sizeof(Vertex)),"UI stream");
            if(kind==0)Hr(d->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2),"UI DrawPrimitive");
            else
            {
                ComPtr<IDirect3DIndexBuffer9> ib;Hr(d->CreateIndexBuffer(sizeof(indices),0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr),"UI index buffer");
                Hr(ib->Lock(0,0,&data,0),"UI IB lock");memcpy(data,indices,sizeof(indices));ib->Unlock();
                Hr(d->SetIndices(ib.Get()),"UI indices");Hr(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2),"UI indexed draw");
            }
        }
        else if(kind==2)Hr(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(Vertex)),"UI DrawPrimitiveUP");
        else Hr(d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,4,2,indices,D3DFMT_INDEX16,v,sizeof(Vertex)),"UI indexed UP");
    }
    Hr(d->EndScene(),"UI EndScene");
}
void DrawDefaultCanvas(IDirect3DDevice9* d){
    ComPtr<IDirect3DTexture9> texture;Hr(d->CreateTexture(2048,2048,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,nullptr),"canvas atlas");
    D3DLOCKED_RECT lock{};Hr(texture->LockRect(0,&lock,nullptr,D3DLOCK_DISCARD),"canvas atlas upload");for(unsigned y=0;y<2048;++y){auto* row=reinterpret_cast<DWORD*>(static_cast<char*>(lock.pBits)+y*lock.Pitch);for(unsigned x=0;x<2048;++x)row[x]=0x0a190000;}texture->UnlockRect(0);
    Hr(d->SetTexture(0,texture.Get()),"canvas texture");d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&identity);
    d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);d->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);d->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);d->SetRenderState(D3DRS_ALPHAREF,0);d->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATER);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
    struct V {float x,y,z;DWORD color;float u,v;};V v[]={{-1,1,.5f,0xffffffff,0,0},{1,1,.5f,0xffffffff,1,0},{-1,-1,.5f,0xffffffff,0,1},{1,-1,.5f,0xffffffff,1,1}};
    Hr(d->BeginScene(),"canvas begin");Hr(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(V)),"canvas draw");Hr(d->EndScene(),"canvas end");
}
void TestTemporalShaderProbe(IDirect3DDevice9* d,g3vr::UiCapture& ui)
{
    ComPtr<IDirect3DStateBlock9> saved;Hr(d->CreateStateBlock(D3DSBT_ALL,&saved),"probe saved state");Hr(saved->Capture(),"probe state snapshot");
    char const* vertex="float4 main(float4 p:POSITION):POSITION{return p;}";
    char const* pixel="float4 main():COLOR{return float4(1,0,0,1);}";
    ComPtr<ID3DBlob> vsCode,psCode,error;
    Hr(D3DCompile(vertex,strlen(vertex),nullptr,nullptr,nullptr,"main","vs_3_0",0,0,&vsCode,&error),"probe test VS compile");
    Hr(D3DCompile(pixel,strlen(pixel),nullptr,nullptr,nullptr,"main","ps_3_0",0,0,&psCode,&error),"probe test PS compile");
    ComPtr<IDirect3DVertexShader9> vs;ComPtr<IDirect3DPixelShader9> ps;
    Hr(d->CreateVertexShader(static_cast<DWORD const*>(vsCode->GetBufferPointer()),&vs),"probe test VS");
    Hr(d->CreatePixelShader(static_cast<DWORD const*>(psCode->GetBufferPointer()),&ps),"probe test PS");
    Hr(d->SetVertexShader(vs.Get()),"probe bind VS");Hr(d->SetPixelShader(ps.Get()),"probe bind PS");
    float constants[1024];for(unsigned i=0;i<1024;++i)constants[i]=float(i)*.25f;
    Hr(d->SetVertexShaderConstantF(0,constants,256),"probe test constants");
    Hr(d->SetRenderState(D3DRS_ZENABLE,TRUE),"probe depth test");Hr(d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE),"probe depth write");
    auto directory=std::filesystem::temp_directory_path()/("G3VR-temporal-unit-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(directory);
    float identity[16]{};for(unsigned i=0;i<4;++i)identity[i*5]=1;
    for(unsigned eye=0;eye<2;++eye){ui.temporal.Begin(eye,17,64,64);ui.temporal.Matrices(identity,identity);for(unsigned i=0;i<5000;++i)ui.temporal.Observe(d);ui.temporal.End(eye);if(ui.temporal.Queries(eye)!=128)throw std::runtime_error("temporal probe query budget not enforced");}
    if(!ui.temporal.Ready(17)||ui.temporal.Ready(18))throw std::runtime_error("temporal probe mismatched stereo frames accepted");
    // Entering the UI phase must not discard the completed world observation.
    ui.End();if(!ui.temporal.Ready(17))throw std::runtime_error("UI phase discarded temporal world metadata");
    std::ostringstream line;ui.temporal.Write(line,0,directory);auto text=line.str();
    for(char const* field:{"\"metadata_only\":true","\"format\":80","\"stage\":\"vertex\"","\"stage\":\"pixel\"","\"file_written\":true","\"constant_registers_c0_c255\":[0,0.25,0.5,0.75","\"depth_pixels_captured\":false","\"motion_vectors_captured\":false"})if(text.find(field)==std::string::npos)throw std::runtime_error("temporal depth/shader metadata missing");
    unsigned bytecodes=0;for(auto const& item:std::filesystem::directory_iterator(directory)){std::ifstream file(item.path(),std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());bool match=(bytes.size()==vsCode->GetBufferSize()&&!memcmp(bytes.data(),vsCode->GetBufferPointer(),bytes.size()))||(bytes.size()==psCode->GetBufferSize()&&!memcmp(bytes.data(),psCode->GetBufferPointer(),bytes.size()));if(!match)throw std::runtime_error("temporal shader bytecode differs from bound shader");++bytecodes;}
    if(bytecodes!=2)throw std::runtime_error("temporal shader exports missing");
    float actual[1024]{};Hr(d->GetVertexShaderConstantF(0,actual,256),"probe constant readback");if(memcmp(constants,actual,sizeof(constants)))throw std::runtime_error("probe modified shader constants");
    ComPtr<IDirect3DVertexShader9> actualVs;ComPtr<IDirect3DPixelShader9> actualPs;d->GetVertexShader(&actualVs);d->GetPixelShader(&actualPs);
    if(actualVs.Get()!=vs.Get()||actualPs.Get()!=ps.Get())throw std::runtime_error("probe changed bound shaders");
    ui.temporal.Discard();if(ui.temporal.Active()||ui.temporal.Ready(17))throw std::runtime_error("cancelled temporal probe retained a completed pair");
    ui.temporal.Observe(d);if(ui.temporal.Queries(0)!=128)throw std::runtime_error("disabled temporal probe still queried device");
    Hr(saved->Apply(),"probe restore test setup");
}
int main()
{
    try
    {
        WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=DefWindowProcW;wc.lpszClassName=L"G3UiGpuTest";RegisterClassW(&wc);
        HWND window=CreateWindowW(wc.lpszClassName,L"UI GPU test",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,wc.hInstance,nullptr);
        if(!window)throw std::runtime_error("UI hidden window");ComPtr<IDirect3D9Ex> d3d;Hr(Direct3DCreate9Ex(D3D_SDK_VERSION,&d3d),"UI D3D9Ex");
        D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
        pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
        ComPtr<IDirect3DDevice9Ex> d;Hr(d3d->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&d),"UI device");
        stereo::Renderer r;r.Initialize();ComPtr<IDirect3DSurface9> back,depth;
        Hr(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back),"UI backbuffer");
        Hr(d->CreateDepthStencilSurface(64,64,D3DFMT_D16,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr),"UI native depth");
        Hr(d->SetDepthStencilSurface(depth.Get()),"UI bind native depth");
        ComPtr<IDirect3DQuery9> q;Hr(d->CreateQuery(D3DQUERYTYPE_EVENT,&q),"UI GPU fence");
        g3vr::SharedEye desktop;desktop.Create(d.Get(),r.device.Get(),64,64);
        auto readDesktop=[&]{Hr(d->StretchRect(back.Get(),nullptr,desktop.surface9.Get(),nullptr,D3DTEXF_NONE),"UI desktop read");g3vr::Wait9(q.Get());return Rgba(r,desktop.texture11.Get());};
        // Establish exactly the result that native desktop rendering produces.
        Hr(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff2040c0,1,0),"UI desktop clear");DrawUi(d.Get());auto control=readDesktop();
        auto table=*reinterpret_cast<void***>(d.Get());void* originals[4]{};for(unsigned i=0;i<4;++i)originals[i]=table[81+i];
        {
            g3vr::UiCapture ui;ui.Initialize(d.Get(),r.device.Get(),64,64);
            Hr(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff2040c0,1,0),"UI desktop clear");
            ui.Begin();
#ifdef G3VR_UI_COMPOSITION_CAPTURE
            ui.alphaTrace.Begin(d.Get(),64,64);
#endif
            DrawUi(d.Get());ui.End();g3vr::Wait9(q.Get());
#ifdef G3VR_UI_COMPOSITION_CAPTURE
            auto traceFile=std::filesystem::temp_directory_path()/(L"G3VR-ui-alpha-"+std::to_wstring(GetCurrentProcessId()));
            if(ui.alphaTrace.Count()!=4||!ui.alphaTrace.Save(traceFile.wstring()))throw std::runtime_error("UI alpha atlas capture failed");
            {std::ifstream f(std::filesystem::path(traceFile.wstring()+L".draws.csv"));std::string text((std::istreambuf_iterator<char>(f)),{});if(text.find("ffffff00")==std::string::npos)throw std::runtime_error("UI alpha atlas sampled the wrong GPU pixels");}
#endif
            if(ui.error[0]||ui.draws!=4)throw std::runtime_error(ui.error[0]?ui.error:"UI four native draw methods not intercepted");
            auto captured=Rgba(r,ui.shared.texture11.Get());auto alpha=[&](unsigned x,unsigned y){return captured[(y*64+x)*4+3];};
            auto color=[&](unsigned x,unsigned y,unsigned c){return captured[(y*64+x)*4+c];};
            if(alpha(1,1)!=0||alpha(10,10)<126||alpha(10,10)>130||color(10,10,2)<126||color(10,10,2)>130||
                alpha(34,10)!=255||color(34,10,1)!=255||alpha(10,34)!=255||color(10,34,0)||color(10,34,1)||color(10,34,2)||alpha(34,34)!=255)
                throw std::runtime_error("UI transparent, premultiplied translucent, opaque color or opaque black pixels differ");
            if(readDesktop()!=control)throw std::runtime_error("UI capture changed native desktop pixels");
#ifdef G3VR_CLEAN_RUNTIME
            // Production UI hooks must forward world draws unchanged even if
            // an obsolete development callback asks to suppress every draw.
            Hr(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff2040c0,1,0),"clean world clear");
            unsigned observed=0;ui.worldObserverContext=&observed;
            ui.worldObserver=[](void* p,IDirect3DDevice9*,unsigned,diagnostic::temporal::Geometry const*)noexcept{++*static_cast<unsigned*>(p);return true;};
            diagnostic::performance::EyeDrawSample sample;
            ui.temporal.Begin(0,16,64,64);
            if(ui.BeginWorldTiming())throw std::runtime_error("clean runtime accepted world probe");
            DrawUi(d.Get());
            if(ui.EndWorldTiming(sample)||observed||ui.temporal.Queries(0)||readDesktop()!=control||ui.draws!=4)
                throw std::runtime_error("clean world forwarding ran a probe or changed native pixels");
            ui.temporal.Discard();ui.worldObserver=nullptr;ui.worldObserverContext=nullptr;
#else
            // World profiling must call each original API exactly once, leave
            // pixels/state identical, and never count UI replay commands.
            Hr(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff2040c0,1,0),"world profiling clear");
            diagnostic::performance::EyeDrawSample worldSample;
            unsigned observed[4]{};ui.worldObserverContext=observed;ui.worldObserver=[](void* p,IDirect3DDevice9* device,unsigned kind,diagnostic::temporal::Geometry const*)noexcept{if(kind<4){auto& count=static_cast<unsigned*>(p)[kind];++count;if(kind==0&&count==1)device->DrawPrimitive(D3DPT_TRIANGLELIST,0,0);}return false;};
            ui.temporal.Begin(0,16,64,64);
            if(!ui.BeginWorldTiming()||ui.BeginWorldTiming())throw std::runtime_error("world timing nested scope accepted");
            DrawUi(d.Get());if(!ui.EndWorldTiming(worldSample))throw std::runtime_error("world timing missing sample");
            ui.worldObserver=nullptr;ui.worldObserverContext=nullptr;for(auto count:observed)if(count!=1)throw std::runtime_error("native evidence observer missed or repeated an original draw");
            ui.temporal.End(0);if(ui.temporal.Queries(0)!=1)throw std::runtime_error("temporal observer not called by real native draw hook");ui.temporal.Discard();
            for(auto count:worldSample.calls)if(count!=1)throw std::runtime_error("world timing did not count each of four original API calls once");
            if(worldSample.drawTicks>worldSample.elapsed||readDesktop()!=control||ui.draws!=4)throw std::runtime_error("world timing changed native/UI pixels or double-counted elapsed time");
            // A completed replacement can suppress only the original indexed
            // API, while other native/UI commands still execute normally.
            Hr(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff2040c0,1,0),"replacement hook clear");
            ui.worldObserver=[](void*,IDirect3DDevice9*,unsigned kind,diagnostic::temporal::Geometry const*)noexcept{return kind==1;};
            if(!ui.BeginWorldTiming())throw std::runtime_error("replacement timing begin");
            DrawUi(d.Get());if(!ui.EndWorldTiming(worldSample)||worldSample.calls[1]!=0||worldSample.calls[0]!=1||worldSample.calls[2]!=1||worldSample.calls[3]!=1||readDesktop()==control)throw std::runtime_error("replacement did not omit exactly the indexed API");
            ui.worldObserver=nullptr;
            TestTemporalShaderProbe(d.Get(),ui);
            if(!ui.BeginWorldTiming())throw std::runtime_error("world timing failed to rearm");
            ui.End();if(ui.EndWorldTiming(worldSample))throw std::runtime_error("cancelled world timing leaked into next phase");
#endif
            ComPtr<IDirect3DSurface9> target,actualDepth;d->GetRenderTarget(0,&target);d->GetDepthStencilSurface(&actualDepth);
            DWORD rgba=0,separate=1,zwrite=0;d->GetRenderState(D3DRS_COLORWRITEENABLE,&rgba);d->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,&separate);d->GetRenderState(D3DRS_ZWRITEENABLE,&zwrite);
            if(target.Get()!=back.Get()||actualDepth.Get()!=depth.Get()||rgba!=7||separate||!zwrite)throw std::runtime_error("UI native target, depth or render state not restored");
            // Clear must not inherit a small native UI viewport/scissor.
            D3DVIEWPORT9 narrowViewport{0,0,16,16,0,1};RECT smallClip{0,0,8,8};
            d->SetViewport(&narrowViewport);d->SetScissorRect(&smallClip);
            ui.Begin();ui.End();g3vr::Wait9(q.Get());captured=Rgba(r,ui.shared.texture11.Get());
            for(auto pixel:captured)if(pixel)throw std::runtime_error("UI previous menu pixels survived clear");
            D3DVIEWPORT9 actualViewport{};RECT actualClip{};d->GetViewport(&actualViewport);d->GetScissorRect(&actualClip);
            if(memcmp(&actualViewport,&narrowViewport,sizeof(narrowViewport))||memcmp(&actualClip,&smallClip,sizeof(smallClip)))throw std::runtime_error("UI clear changed native viewport or scissor");
            D3DVIEWPORT9 full{0,0,64,64,0,1};d->SetViewport(&full);
            DrawUi(d.Get());if(ui.draws!=4)throw std::runtime_error("UI draws intercepted outside UI phase");
            ComPtr<IDirect3DSurface9> preview;
            Hr(d->CreateRenderTarget(64,64,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&preview,nullptr),"UI preview target");
            ui.Begin();d->SetRenderTarget(0,preview.Get());DrawUi(d.Get());ui.End();
            if(ui.draws!=4)throw std::runtime_error("UI intermediate preview target was replayed into the overlay");
            d->SetRenderTarget(0,back.Get());
            // A native movie can write RGB while leaving alpha zero. Capture
            // the actual backbuffer without repeating decoder/native callbacks.
            d->Clear(0,nullptr,D3DCLEAR_TARGET,0x00123456,1,0);ui.CopyBackbuffer();g3vr::Wait9(q.Get());captured=Rgba(r,ui.shared.texture11.Get());
            size_t videoPixel=(32*64+32)*4;
            if(captured[videoPixel]!=0x56||captured[videoPixel+1]!=0x34||captured[videoPixel+2]!=0x12)throw std::runtime_error("Cinema RGB backbuffer copy failed");
            d->GetRenderTarget(0,&target);d->GetViewport(&actualViewport);d->GetScissorRect(&actualClip);
            if(target.Get()!=back.Get()||ui.draws!=4)throw std::runtime_error("Cinema copy changed native target or replayed UI draws");
            // Reproduce the measured 4% native full-canvas tint. Omit only
            // its first panel replay, preserving native desktop and later UI.
            ComPtr<IDirect3DStateBlock9> beforeCanvas;Hr(d->CreateStateBlock(D3DSBT_ALL,&beforeCanvas),"canvas test state");beforeCanvas->Capture();
            D3DVIEWPORT9 canvasViewport{0,0,64,64,0,1};RECT canvasClip{0,0,64,64};d->SetViewport(&canvasViewport);d->SetScissorRect(&canvasClip);
            d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff2040c0,1,0);DrawDefaultCanvas(d.Get());auto canvasControl=readDesktop();
            d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff2040c0,1,0);ui.Begin();DrawDefaultCanvas(d.Get());ui.End();g3vr::Wait9(q.Get());auto emptyCanvas=Rgba(r,ui.shared.texture11.Get());
            for(auto c:emptyCanvas)if(c)throw std::runtime_error("Default GUI canvas still tints the transparent XR panel");
            if(readDesktop()!=canvasControl)throw std::runtime_error("Removing XR backdrop changed desktop rendering");
            ui.Begin();DrawDefaultCanvas(d.Get());DrawDefaultCanvas(d.Get());ui.End();g3vr::Wait9(q.Get());auto secondCanvas=Rgba(r,ui.shared.texture11.Get());
            if(secondCanvas[(32*64+32)*4+3]!=10)throw std::runtime_error("Later sprites using the GUI atlas were removed");
            ui.Begin();DrawDefaultCanvas(d.Get());DrawUi(d.Get());ui.End();g3vr::Wait9(q.Get());auto foreground=Rgba(r,ui.shared.texture11.Get());
            if(foreground[(1*64+1)*4+3]!=0||foreground[(10*64+10)*4+3]<126||foreground[(34*64+34)*4+3]!=255)throw std::runtime_error("Canvas fix damaged translucent or opaque UI elements");
            beforeCanvas->Apply();d->SetTexture(0,nullptr);
        }
        for(unsigned i=0;i<4;++i)if(table[81+i]!=originals[i])throw std::runtime_error("UI GPU hooks not restored at shutdown");
        r.context->ClearState();r.context->Flush();desktop.Reset();back.Reset();q.Reset();
        d->SetStreamSource(0,nullptr,0,0);d->SetIndices(nullptr);d->SetDepthStencilSurface(nullptr);depth.Reset();
        auto* native=d.Detach();if(native->Release()!=0)throw std::runtime_error("UI native references leaked at shutdown");
        DestroyWindow(window);puts("PASS four real GPU UI draw paths; transparent/translucent/opaque black; desktop identical; native state and hooks restored; no stale UI or native leaks");return 0;
    }
    catch(std::exception const& e){fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
