#include "DrawCostTrial.h"
#include <cstdio>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
using namespace diagnostic::draw_cost;
void Need(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
int __cdecl wmain(int argc,wchar_t** argv){try{
 Need(argc==3,"usage: GPU test d3d9.dll output_directory");
 HMODULE module=LoadLibraryExW(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);Need(module!=nullptr,"load isolated backend");
 using Create=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);auto create=reinterpret_cast<Create>(GetProcAddress(module,"Direct3DCreate9Ex"));Need(create!=nullptr,"Direct3DCreate9Ex");
 HWND window=CreateWindowExW(0,L"STATIC",L"Draw cost GPU fixture",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);Need(window!=nullptr,"hidden test window");
 ComPtr<IDirect3D9Ex> api;Need(SUCCEEDED(create(D3D_SDK_VERSION,&api)),"create API");
 D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;pp.BackBufferWidth=64;pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 ComPtr<IDirect3DDevice9Ex> device;Need(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&device)),"create device");
 Collector collector;collector.Hooks(true,true);Need(collector.Initialize(device.Get()),"GPU timestamps unsupported");
 for(uint64_t frame=1;frame<=192;++frame){collector.NextFrame(frame,true);collector.BeginGpuFrame(device.Get());
  for(unsigned lane=1;lane<=2;++lane){auto high=collector.Begin(Kind::HighExecute,lane);device->Clear(0,nullptr,D3DCLEAR_TARGET,0xffa03010,1,0);
   auto vegetation=collector.Begin(Kind::VegetationHigh,lane);device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff30a010,1,0);collector.End(vegetation);
   auto low=collector.Begin(Kind::LowExecute,lane);device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff1030a0,1,0);collector.End(low);collector.End(high);}
  collector.EndGpuFrame();Need(SUCCEEDED(device->PresentEx(nullptr,nullptr,nullptr,nullptr,0)),"present fixture");
 }
 collector.NextFrame(193,false);for(unsigned i=0;i<8;++i){device->PresentEx(nullptr,nullptr,nullptr,nullptr,0);collector.Poll();Sleep(1);}
 std::filesystem::path output=argv[2];std::filesystem::create_directories(output);collector.Write(output);collector.ResetGpu();
 std::ifstream meta(output/L"draw.json");std::string text((std::istreambuf_iterator<char>(meta)),{});Need(text.find("\"gpu_span_rows\":0,")==std::string::npos,"no completed GPU samples");Need(text.find("\"unsupported_errors\":0,")!=std::string::npos,"GPU query error");
 device.Reset();api.Reset();DestroyWindow(window);FreeLibrary(module);
 std::puts("PASS actual backend asynchronous GPU queries, nested scopes, both eyes, no GetData flushes or polling waits");return 0;
 }catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
