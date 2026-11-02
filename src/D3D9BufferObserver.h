#pragma once
#include "TemporalBufferVersions.h"
#include "VirtualHook.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <mutex>

namespace g3vr {
// Retain observed resources for two eye-pairs, so pointer reuse cannot masquerade
// as old geometry. Observe all subsequent successful writes through their actual
// native Lock/Unlock methods, including writes outside the render thread.
class D3D9BufferObserver {
    using Lock=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,UINT,UINT,void**,DWORD);
    using Unlock=HRESULT(STDMETHODCALLTYPE*)(IUnknown*);
    using Process=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,UINT,UINT,UINT,IDirect3DVertexBuffer9*,IDirect3DVertexDeclaration9*,DWORD);
    struct Table {void** value=nullptr;diagnostic::VirtualHook lock,unlock;};
    struct DeviceTable {void** value=nullptr;diagnostic::VirtualHook process;};
    struct Resource {Microsoft::WRL::ComPtr<IUnknown> value;uint64_t seen=0;};
    // Dispatch records outlive an observer. A native worker may already have
    // fetched a detour address when Reset restores the vtable; it must still
    // call its original Lock/Unlock after the owner has been destroyed.
    inline static std::array<Table,16> tables;
    inline static std::array<DeviceTable,16> devices;
    inline static std::recursive_mutex gate;
    std::array<Resource,2048> resources;
    diagnostic::temporal::BufferVersions versions;
    uint64_t frame=0;inline static D3D9BufferObserver* owner=nullptr;
    static Table* Find(void** table)noexcept {for(auto& t:tables)if(t.value==table)return &t;return nullptr;}
    static HRESULT STDMETHODCALLTYPE Locked(IUnknown* self,UINT offset,UINT bytes,void** data,DWORD flags){
        std::lock_guard<std::recursive_mutex> guard(gate);auto* p=owner;auto* t=Find(*reinterpret_cast<void***>(self));
        if(!t)return D3DERR_INVALIDCALL;
        auto hr=reinterpret_cast<Lock>(t->lock.original)(self,offset,bytes,data,flags);
        if(p)p->versions.Lock(reinterpret_cast<uintptr_t>(self),(flags&D3DLOCK_READONLY)==0,SUCCEEDED(hr));return hr;
    }
    static HRESULT STDMETHODCALLTYPE Unlocked(IUnknown* self){
        std::lock_guard<std::recursive_mutex> guard(gate);auto* p=owner;auto* t=Find(*reinterpret_cast<void***>(self));
        if(!t)return D3DERR_INVALIDCALL;
        auto hr=reinterpret_cast<Unlock>(t->unlock.original)(self);
        if(p)p->versions.Unlock(reinterpret_cast<uintptr_t>(self),SUCCEEDED(hr));return hr;
    }
    static HRESULT STDMETHODCALLTYPE Processed(IDirect3DDevice9* self,UINT source,UINT destination,UINT count,IDirect3DVertexBuffer9* output,IDirect3DVertexDeclaration9* declaration,DWORD flags){
        std::lock_guard<std::recursive_mutex> guard(gate);auto** table=*reinterpret_cast<void***>(self);
        for(auto& d:devices)if(d.value==table){
            auto hr=reinterpret_cast<Process>(d.process.original)(self,source,destination,count,output,declaration,flags);
            if(owner&&SUCCEEDED(hr)&&count){auto pointer=reinterpret_cast<uintptr_t>(output);owner->versions.Lock(pointer,true,true);owner->versions.Unlock(pointer,true);}return hr;
        }return D3DERR_INVALIDCALL;
    }
    bool DeviceHooks(IUnknown* resource){
        Microsoft::WRL::ComPtr<IDirect3DDevice9> device;
        if(FAILED(static_cast<IDirect3DResource9*>(resource)->GetDevice(&device)))return false;
        auto** table=*reinterpret_cast<void***>(device.Get());DeviceTable* known=nullptr;
        for(auto& d:devices)if(d.value==table){if(d.process.installed)return owner==this;known=&d;break;}
        if(owner&&owner!=this)return false;
        for(auto& d:devices)if(&d==known||(!known&&!d.value)){
            if(!d.process.Prepare(table,85,reinterpret_cast<void*>(&Processed)))return false;
            d.value=table;owner=this;return d.process.Install();
        }return false;
    }
    bool Hooks(IUnknown* value){
        auto** table=*reinterpret_cast<void***>(value);auto* known=Find(table);if(known&&known->lock.installed&&known->unlock.installed)return owner==this;
        if(owner&&owner!=this)return false;
        for(auto& t:tables)if(&t==known||(!known&&!t.value)){
            if(!t.lock.Prepare(table,11,reinterpret_cast<void*>(&Locked))||!t.unlock.Prepare(table,12,reinterpret_cast<void*>(&Unlocked)))return false;
            owner=this;t.value=table;
            if(t.lock.Install()&&t.unlock.Install())return true;
            t.lock.Restore();t.unlock.Restore();return false;
        }return false;
    }
    bool Track(IUnknown* value,UINT bytes,bool index){
        if(!value||!bytes||!Hooks(value)||!DeviceHooks(value))return false;
        for(auto& r:resources)if(r.value.Get()==value){r.seen=frame;return versions.Observe(reinterpret_cast<uintptr_t>(value),bytes,index);}
        for(auto& r:resources)if(!r.value){
            if(!versions.Observe(reinterpret_cast<uintptr_t>(value),bytes,index))return false;
            r.value=value;r.seen=frame;return true;
        }return false;
    }
public:
    D3D9BufferObserver()=default;D3D9BufferObserver(D3D9BufferObserver const&)=delete;
    void Begin(uint64_t serial){
        std::lock_guard<std::recursive_mutex> guard(gate);frame=serial;
        for(auto& r:resources)if(r.value&&(serial<r.seen||serial-r.seen>1)){
            versions.Released(reinterpret_cast<uintptr_t>(r.value.Get()),0);r={};
        }
    }
    bool Observe(IDirect3DVertexBuffer9* value){
        std::lock_guard<std::recursive_mutex> guard(gate);D3DVERTEXBUFFER_DESC d{};
        return value&&SUCCEEDED(value->GetDesc(&d))&&Track(value,d.Size,false);
    }
    bool Observe(IDirect3DIndexBuffer9* value){
        std::lock_guard<std::recursive_mutex> guard(gate);D3DINDEXBUFFER_DESC d{};
        return value&&SUCCEEDED(value->GetDesc(&d))&&Track(value,d.Size,true);
    }
    diagnostic::temporal::GeometryVersion Version(IDirect3DVertexBuffer9* v,IDirect3DIndexBuffer9* i){
        std::lock_guard<std::recursive_mutex> guard(gate);return versions.Geometry(reinterpret_cast<uintptr_t>(v),reinterpret_cast<uintptr_t>(i));
    }
    // Only called after render/capture scopes have stopped. Hooks restore before
    // retained COM resources are released; no native Release detour needed.
    void Reset()noexcept {
        std::lock_guard<std::recursive_mutex> guard(gate);
        if(owner==this){for(auto& t:tables){t.lock.Restore();t.unlock.Restore();}for(auto& d:devices)d.process.Restore();owner=nullptr;}
        for(auto& r:resources)r={};versions.Reset();frame=0;
    }
    ~D3D9BufferObserver(){Reset();}
};
}
