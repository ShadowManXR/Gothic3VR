#pragma once
#include "TemporalInstanceIdentity.h"
namespace diagnostic::temporal {
// Pure bounded state, intended for native VB/IB Lock/Unlock/Release observers.
// It does not install hooks. Real history must NOT trust this until the caller
// observes every write/release for its buffers before retaining frame history.
class BufferVersions {
    struct Buffer {uintptr_t pointer=0;uint64_t object=0,content=0;uint32_t bytes=0;bool index=false,locked=false,write=false,poisoned=false;};
    std::array<Buffer,2048> buffers;uint64_t serial=0;
    Buffer* Find(uintptr_t pointer)noexcept{for(auto& b:buffers)if(b.pointer==pointer&&pointer)return &b;return nullptr;}
    Buffer const* Find(uintptr_t pointer)const noexcept{for(auto const& b:buffers)if(b.pointer==pointer&&pointer)return &b;return nullptr;}
public:
    unsigned overflow=0;
    void Reset()noexcept{for(auto& b:buffers)b={};overflow=0;} // serial survives
    bool Observe(uintptr_t pointer,uint32_t bytes,bool index)noexcept{
        if(!pointer||!bytes)return false;
        if(auto* b=Find(pointer)){if(b->bytes==bytes&&b->index==index)return true;*b={pointer,++serial,0,bytes,index};return true;}
        for(auto& b:buffers)if(!b.pointer){b={pointer,++serial,0,bytes,index};return true;}
        ++overflow;return false;
    }
    void Created(uintptr_t pointer,uint32_t bytes,bool index)noexcept{if(auto* b=Find(pointer))*b={};Observe(pointer,bytes,index);}
    void Lock(uintptr_t pointer,bool write,bool succeeded)noexcept{if(!succeeded)return;if(auto* b=Find(pointer)){if(b->locked){b->poisoned=true;return;}b->locked=true;b->write=write;if(write)b->content=++serial;}}
    void Unlock(uintptr_t pointer,bool succeeded)noexcept{if(auto* b=Find(pointer)){if(!b->locked||!succeeded){b->poisoned=true;return;}if(b->write)b->content=++serial;b->locked=b->write=false;}}
    void Released(uintptr_t pointer,unsigned references)noexcept{if(!references)if(auto* b=Find(pointer))*b={};}
    GeometryVersion Geometry(uintptr_t vertices,uintptr_t indices)const noexcept{
        GeometryVersion result;auto* v=Find(vertices);auto* i=Find(indices);if(!v||!i||v->index||!i->index||v->locked||i->locked||v->poisoned||i->poisoned)return result;
        result={v->object,v->content,i->object,i->content,true};return result;
    }
};
}
