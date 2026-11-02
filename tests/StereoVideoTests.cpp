#include "StereoFrameCapture.h"
#include "D3D9WorldWriteTrace.h"
#include <d3d9.h>
#include <cstdio>
#include <iterator>
using Microsoft::WRL::ComPtr;
static void Hr(HRESULT h){if(FAILED(h))throw std::runtime_error("GPU test failed");}
int main(){try{
    WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=DefWindowProcW;wc.lpszClassName=L"G3VideoTest";RegisterClassW(&wc);
    HWND window=CreateWindowW(wc.lpszClassName,L"Stereo capture test",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,wc.hInstance,nullptr);
    ComPtr<IDirect3D9Ex> api9;Hr(Direct3DCreate9Ex(D3D_SDK_VERSION,&api9));D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.hDeviceWindow=window;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    ComPtr<IDirect3DDevice9Ex> d9;Hr(api9->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&d9));
    ComPtr<ID3D11Device> d;ComPtr<ID3D11DeviceContext> c;D3D_FEATURE_LEVEL level;Hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&d,&level,&c));
    ComPtr<IDirect3DTexture9> input9[2];ComPtr<IDirect3DSurface9> surface[2];ComPtr<ID3D11Texture2D> input[2];
    for(unsigned eye=0;eye<2;++eye){HANDLE handle=nullptr;Hr(d9->CreateTexture(64,64,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&input9[eye],&handle));Hr(input9[eye]->GetSurfaceLevel(0,&surface[eye]));Hr(d->OpenSharedResource(handle,IID_PPV_ARGS(&input[eye])));Hr(d9->ColorFill(surface[eye].Get(),nullptr,eye?0xff00ff00:0xffff0000));}
    ComPtr<IDirect3DQuery9> q;Hr(d9->CreateQuery(D3DQUERYTYPE_EVENT,&q));Hr(q->Issue(D3DISSUE_END));while(q->GetData(nullptr,0,D3DGETDATA_FLUSH)==S_FALSE)SwitchToThread();
    diagnostic::StereoFrameCapture capture;capture.Start(d.Get(),2);diagnostic::StereoFrameCapture::Frame f;f.serial=1;
    capture.Begin(f);capture.Eye(c.Get(),input[0].Get(),0,1);if(capture.End(false)||capture.Count())throw std::runtime_error("Partial eye retained");
    f.serial=2;capture.Begin(f);capture.Eye(c.Get(),input[0].Get(),0,1);capture.Eye(c.Get(),input[0].Get(),0,2);capture.Eye(c.Get(),input[0].Get(),0,2);capture.Eye(c.Get(),input[1].Get(),1,2);
    if(capture.End(true)||capture.Count()!=1)throw std::runtime_error("First pair count");
    bool refused=false;auto out=std::filesystem::temp_directory_path()/(L"G3VR-video-test-"+std::to_wstring(GetCurrentProcessId()));
    try{capture.Save(c.Get(),out);}catch(...){refused=true;}if(!refused||std::filesystem::exists(out))throw std::runtime_error("Readback allowed during recording");
    f.serial=3;capture.Begin(f);capture.Eye(c.Get(),input[1].Get(),1,3);capture.Eye(c.Get(),input[0].Get(),0,3);if(!capture.End(false)||capture.Count()!=2)throw std::runtime_error("Bounded recording failed");
    capture.Save(c.Get(),out);std::ifstream file(out/L"eyes.bgra",std::ios::binary);std::vector<unsigned char> pixels((std::istreambuf_iterator<char>(file)),{});
    size_t image=diagnostic::StereoFrameCapture::Width*diagnostic::StereoFrameCapture::Height*4;
    if(pixels.size()!=image*4)throw std::runtime_error("Wrong video size");
    for(size_t i=0;i<pixels.size();i+=4){bool green=(i/image)%2==1;if(pixels[i]!=0||pixels[i+1]!=(green?255:0)||pixels[i+2]!=(green?0:255))throw std::runtime_error("Eye identity or image corrupted");}
    std::ifstream meta(out/L"frames.csv");std::string csv((std::istreambuf_iterator<char>(meta)),{});if(csv.find("0,2,")==std::string::npos||csv.find("1,3,")==std::string::npos)throw std::runtime_error("Frame metadata missing");
    // Read the original shared inputs after recording: capture must not change them.
    D3D11_TEXTURE2D_DESC t{};input[0]->GetDesc(&t);t.Usage=D3D11_USAGE_STAGING;t.BindFlags=t.MiscFlags=0;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> check;Hr(d->CreateTexture2D(&t,nullptr,&check));c->CopyResource(check.Get(),input[0].Get());D3D11_MAPPED_SUBRESOURCE m{};Hr(c->Map(check.Get(),0,D3D11_MAP_READ,0,&m));auto* p=static_cast<unsigned char*>(m.pData);bool unchanged=p[0]==0&&p[1]==0&&p[2]==255;c->Unmap(check.Get(),0);if(!unchanged)throw std::runtime_error("Native source changed");
    file.close();meta.close();capture.Reset();
    // Cross the native 2048-array-slice limit in long-duration mode. Keep the
    // entire interval on GPU, then verify both sides of the array boundary.
    capture.Start(d.Get(),1026,true);
    for(unsigned i=0;i<1026;++i){f.serial=10+i;capture.Begin(f);capture.Eye(c.Get(),input[0].Get(),0,f.serial);capture.Eye(c.Get(),input[1].Get(),1,f.serial);bool ended=capture.End(true);if(ended!=(i==1025))throw std::runtime_error("Long video bound failed");}
    capture.Save(c.Get(),out);std::ifstream gray(out/L"eyes.gray",std::ios::binary);std::vector<unsigned char> luma((std::istreambuf_iterator<char>(gray)),{});gray.close();
    size_t grayImage=192*108;if(luma.size()!=grayImage*2*1026)throw std::runtime_error("Long video output size");
    for(size_t i=0;i<luma.size();++i){int expected=(i/grayImage)%2?150:76;if(std::abs(int(luma[i])-expected)>1)throw std::runtime_error("Luma or long array identity failed");}
    capture.Reset();
    ComPtr<IDirect3DSurface9> back;Hr(d9->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back));auto** table=*reinterpret_cast<void***>(d9.Get());unsigned slots[]={37,47,43,34};void* originals[4]{};for(unsigned i=0;i<4;++i)originals[i]=table[slots[i]];
    {diagnostic::WorldWriteTrace trace;trace.Initialize(d9.Get(),back.Get());trace.Start();trace.Eye(91,0);D3DVIEWPORT9 smallViewport{0,0,8,8,0,1};Hr(d9->SetRenderTarget(0,back.Get()));Hr(d9->SetViewport(&smallViewport));Hr(d9->SetTexture(0,input9[0].Get()));trace.Draw(d9.Get(),2,2);Hr(d9->Clear(0,nullptr,D3DCLEAR_TARGET,0xffffffff,1,0));trace.Eye(91,-1);trace.Stop();
        auto count=trace.Count();D3DVIEWPORT9 actual{};Hr(d9->GetViewport(&actual));if(actual.Width!=8||actual.Height!=8||count<5)throw std::runtime_error("Read-only write trace altered native viewport or missed commands");
        Hr(d9->Clear(0,nullptr,D3DCLEAR_TARGET,0xff000000,1,0));if(trace.Count()!=count)throw std::runtime_error("Unarmed trace still collecting");trace.Save(out);std::ifstream log(out/L"world-writes.csv");std::string lines((std::istreambuf_iterator<char>(log)),{});if(lines.find("91,")==std::string::npos||lines.find("stack11")==std::string::npos)throw std::runtime_error("Trace provenance missing");}
    for(unsigned i=0;i<4;++i)if(table[slots[i]]!=originals[i])throw std::runtime_error("Native GPU trace hook not restored");
    std::filesystem::remove_all(out);DestroyWindow(window);std::puts("PASS: real shared eyes, cancellation, deferred readback, long array boundary; read-only native write trace, disarm and hook restoration");return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
