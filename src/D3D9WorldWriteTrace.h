#pragma once
// Only included by G3VR_STEREO_FRAME_CAPTURE. Observes native commands without
// changing arguments, targets, states, draw count, waits or return values.
#include "VirtualHook.h"
#include <d3d9.h>
#include <tlhelp32.h>
#include <wrl/client.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace diagnostic {
class WorldWriteTrace {
    using Target=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,DWORD,IDirect3DSurface9*);
    using View=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DVIEWPORT9 const*);
    using Clear=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,DWORD,D3DRECT const*,DWORD,D3DCOLOR,float,DWORD);
    using Stretch=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,IDirect3DSurface9*,RECT const*,IDirect3DSurface9*,RECT const*,D3DTEXTUREFILTERTYPE);
    inline static Target targetCall=nullptr;inline static View viewCall=nullptr;
    inline static Clear clearCall=nullptr;inline static Stretch stretchCall=nullptr;
    inline static WorldWriteTrace* owner=nullptr;
    VirtualHook hooks[4];
    Microsoft::WRL::ComPtr<IDirect3DDevice9> device;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> backbuffer;
    struct Entry {
        uint64_t serial=0,qpc=0;int eye=-1;unsigned kind=0;
        uintptr_t target=0,texture=0;unsigned targetW=0,targetH=0;
        unsigned x=0,y=0,w=0,h=0,arg=0,count=0,texW=0,texH=0;
        DWORD fvf=0,scissorOn=0,mask=0;RECT scissor{};void* stack[12]{};unsigned stackCount=0;
    };
    static constexpr unsigned MaxEntries=131072;
    std::vector<Entry> entries;
    DWORD thread=0;bool active=false,main=false;unsigned dropped=0,stacks=0;
    uint64_t serial=0;int eye=-1;D3DVIEWPORT9 viewport{};D3DSURFACE_DESC desc{};uintptr_t target=0;
    uint64_t lastLeftQpc=0,frequency=0,stackUntilSerial=0;
    bool Owns(IDirect3DDevice9* d)const noexcept{return active&&d==device.Get()&&GetCurrentThreadId()==thread;}
    Entry* Add(unsigned kind){
        if(entries.size()==MaxEntries){++dropped;return nullptr;}
        Entry e;LARGE_INTEGER q{};QueryPerformanceCounter(&q);e.qpc=q.QuadPart;e.serial=serial;e.eye=eye;e.kind=kind;
        e.target=target;e.targetW=desc.Width;e.targetH=desc.Height;e.x=viewport.X;e.y=viewport.Y;e.w=viewport.Width;e.h=viewport.Height;
        entries.push_back(e);return &entries.back();
    }
    void Stack(Entry& e){if(stacks<4096){e.stackCount=CaptureStackBackTrace(1,12,e.stack,nullptr);++stacks;}}
    void TargetChanged(IDirect3DSurface9* surface){
        bool wasMain=main;main=surface==backbuffer.Get();target=reinterpret_cast<uintptr_t>(surface);desc={};if(surface)surface->GetDesc(&desc);
        device->GetViewport(&viewport);if(main||wasMain)if(auto* e=Add(1)){e->arg=main;}
    }
    static HRESULT STDMETHODCALLTYPE SetTarget(IDirect3DDevice9* d,DWORD index,IDirect3DSurface9* surface){
        auto hr=targetCall(d,index,surface);auto* p=owner;if(SUCCEEDED(hr)&&!index&&p&&p->Owns(d))p->TargetChanged(surface);return hr;
    }
    static HRESULT STDMETHODCALLTYPE SetView(IDirect3DDevice9* d,D3DVIEWPORT9 const* view){
        auto hr=viewCall(d,view);auto* p=owner;if(SUCCEEDED(hr)&&p&&p->Owns(d)){p->viewport=*view;if(p->main)if(auto* e=p->Add(2)){e->arg=1;if(view->Width<p->desc.Width||view->Height<p->desc.Height)p->Stack(*e);}}return hr;
    }
    static HRESULT STDMETHODCALLTYPE ClearTarget(IDirect3DDevice9* d,DWORD count,D3DRECT const* rects,DWORD flags,D3DCOLOR color,float depth,DWORD stencil){
        auto* p=owner;if(p&&p->Owns(d)&&p->main){if(auto* e=p->Add(3)){e->arg=color;e->count=count;e->mask=flags;if(count&&rects)e->scissor={rects[0].x1,rects[0].y1,rects[0].x2,rects[0].y2};if((flags&D3DCLEAR_TARGET)&&((color&0xffffff)>0xe0e0e0))p->Stack(*e);}}
        return clearCall(d,count,rects,flags,color,depth,stencil);
    }
    static HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DDevice9* d,IDirect3DSurface9* source,RECT const* sr,IDirect3DSurface9* destination,RECT const* dr,D3DTEXTUREFILTERTYPE filter){
        auto* p=owner;if(p&&p->Owns(d)&&destination==p->backbuffer.Get()){if(auto* e=p->Add(4)){e->texture=reinterpret_cast<uintptr_t>(source);D3DSURFACE_DESC s{};if(source&&SUCCEEDED(source->GetDesc(&s))){e->texW=s.Width;e->texH=s.Height;}if(dr)e->scissor=*dr;p->Stack(*e);}}
        return stretchCall(d,source,sr,destination,dr,filter);
    }
public:
    void Initialize(IDirect3DDevice9* d,IDirect3DSurface9* back){
        if(owner)throw std::runtime_error("World write trace already installed");device=d;backbuffer=back;auto** table=*reinterpret_cast<void***>(d);
        unsigned slots[]={37,47,43,34};void* replacement[]={reinterpret_cast<void*>(&SetTarget),reinterpret_cast<void*>(&SetView),reinterpret_cast<void*>(&ClearTarget),reinterpret_cast<void*>(&StretchRect)};
        for(unsigned i=0;i<4;++i)if(!Executable(table[slots[i]])||!hooks[i].Prepare(table,slots[i],replacement[i]))throw std::runtime_error("Cannot prepare world write trace");
        targetCall=reinterpret_cast<Target>(hooks[0].original);viewCall=reinterpret_cast<View>(hooks[1].original);clearCall=reinterpret_cast<Clear>(hooks[2].original);stretchCall=reinterpret_cast<Stretch>(hooks[3].original);owner=this;
        for(auto& h:hooks)if(!h.Install())throw std::runtime_error("Cannot install world write trace");
    }
    void Start(){entries.clear();entries.reserve(MaxEntries);dropped=stacks=0;thread=GetCurrentThreadId();serial=0;eye=-1;lastLeftQpc=stackUntilSerial=0;LARGE_INTEGER hz{};QueryPerformanceFrequency(&hz);frequency=hz.QuadPart;active=true;Microsoft::WRL::ComPtr<IDirect3DSurface9> current;device->GetRenderTarget(0,&current);TargetChanged(current.Get());}
    void Eye(uint64_t frame,int index){serial=frame;eye=index;if(active&&GetCurrentThreadId()==thread){
        if(index==0){LARGE_INTEGER q{};QueryPerformanceCounter(&q);if(!lastLeftQpc)stackUntilSerial=frame+4;else if(uint64_t(q.QuadPart)-lastLeftQpc>frequency/20)stackUntilSerial=frame+32;lastLeftQpc=q.QuadPart;}
        device->GetViewport(&viewport);if(auto* e=Add(index<0?6:5)){e->arg=main;}}}
    void Draw(IDirect3DDevice9* d,unsigned kind,unsigned count){
        if(!Owns(d)||!main||(count>2&&viewport.Width>=desc.Width&&viewport.Height>=desc.Height))return;
        auto* e=Add(7);if(!e)return;e->arg=kind;e->count=count;D3DVIEWPORT9 actual{};if(SUCCEEDED(d->GetViewport(&actual))){e->x=actual.X;e->y=actual.Y;e->w=actual.Width;e->h=actual.Height;}
        d->GetRenderState(D3DRS_COLORWRITEENABLE,&e->mask);d->GetRenderState(D3DRS_SCISSORTESTENABLE,&e->scissorOn);d->GetScissorRect(&e->scissor);d->GetFVF(&e->fvf);
        Microsoft::WRL::ComPtr<IDirect3DBaseTexture9> texture;d->GetTexture(0,&texture);e->texture=reinterpret_cast<uintptr_t>(texture.Get());
        if(texture&&texture->GetType()==D3DRTYPE_TEXTURE){D3DSURFACE_DESC t{};if(SUCCEEDED(static_cast<IDirect3DTexture9*>(texture.Get())->GetLevelDesc(0,&t))){e->texW=t.Width;e->texH=t.Height;}}
        if(e->mask&&((e->w<desc.Width||e->h<desc.Height)||(e->scissorOn&&e->scissor.right-e->scissor.left<int(desc.Width/2))||serial<=stackUntilSerial))Stack(*e);
    }
    void Stop()noexcept{active=false;}
    size_t Count()const{return entries.size();}
    void Save(std::filesystem::path const& directory){
        if(active)throw std::runtime_error("World trace cannot write while recording");
        std::ofstream out(directory/L"world-writes.csv");out<<"serial,qpc,eye,kind,target,target_w,target_h,x,y,w,h,arg,count,color_mask,texture,tex_w,tex_h,fvf,scissor_on,scissor_left,scissor_top,scissor_right,scissor_bottom";for(unsigned i=0;i<12;++i)out<<",stack"<<i;out<<'\n';
        for(auto const& e:entries){out<<e.serial<<','<<e.qpc<<','<<e.eye<<','<<e.kind<<','<<e.target<<','<<e.targetW<<','<<e.targetH<<','<<e.x<<','<<e.y<<','<<e.w<<','<<e.h<<','<<e.arg<<','<<e.count<<','<<e.mask<<','<<e.texture<<','<<e.texW<<','<<e.texH<<','<<e.fvf<<','<<e.scissorOn<<','<<e.scissor.left<<','<<e.scissor.top<<','<<e.scissor.right<<','<<e.scissor.bottom;for(auto p:e.stack)out<<','<<reinterpret_cast<uintptr_t>(p);out<<'\n';}
        std::ofstream info(directory/L"world-writes-info.txt");info<<"Kinds: 1=RT changed (arg=backbuffer), 2=viewport (arg=backbuffer), 3=clear (arg=color,count=rects,mask=flags), 4=stretch to backbuffer, 5=eye begin, 6=eye end, 7=draw (arg=method,count=primitives). No commands changed. Dropped="<<dropped<<"\n";
        std::ofstream modules(directory/L"modules.csv");modules<<"base,size,name\n";HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());if(snap!=INVALID_HANDLE_VALUE){MODULEENTRY32W m{};m.dwSize=sizeof(m);if(Module32FirstW(snap,&m))do{char name[1024]{};WideCharToMultiByte(CP_UTF8,0,m.szModule,-1,name,sizeof(name),nullptr,nullptr);modules<<reinterpret_cast<uintptr_t>(m.modBaseAddr)<<','<<m.modBaseSize<<','<<name<<'\n';}while(Module32NextW(snap,&m));CloseHandle(snap);}
        if(!out||!info||!modules)throw std::runtime_error("World write trace save failed");
    }
    void Reset()noexcept{Stop();for(auto& h:hooks)h.Restore();if(owner==this)owner=nullptr;device.Reset();backbuffer.Reset();entries.clear();}
    ~WorldWriteTrace(){Reset();}
};
}
