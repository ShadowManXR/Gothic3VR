#pragma once
#include <array>
#include <cstddef>

namespace g3vr {
template<class Resource> class DeferredFrameCopies {
    struct Copy {Resource* destination=nullptr;Resource* source=nullptr;};
    std::array<Copy,3> pending{};
    std::size_t count=0;
public:
    std::size_t Count() const noexcept {return count;}
    bool Add(Resource* destination,Resource* source) noexcept {
        if(!destination||!source||count==pending.size())return false;
        for(std::size_t i=0;i<count;++i)if(pending[i].destination==destination||pending[i].source==source)return false;
        pending[count++]={destination,source};return true;
    }
    template<class Wait,class Submit> void Drain(Wait wait,Submit submit) {
        if(!count)return;
        // Submit the D3D11 reads only after every source write on D3D9 has
        // finished. All resources remain owned by Runtime for this frame.
        wait();
        while(count){
            submit(pending[0].destination,pending[0].source);
            for(std::size_t i=1;i<count;++i)pending[i-1]=pending[i];
            pending[--count]={};
        }
    }
};
// Each source is written once per XR frame. Keep its acquired destination
// until ALL reads have completed, before releasing images or reusing sources.
// No work is carried into the next frame and no frame of latency is added.
template<class Handle> class FrameCopyBatch {
    std::array<Handle,4> pending{};
    std::size_t count=0;
    bool completed=false;
public:
    std::size_t Count() const noexcept {return count;}
    bool Add(Handle handle) noexcept {
        if(handle==Handle{}||completed||count==pending.size())return false;
        for(std::size_t i=0;i<count;++i)if(pending[i]==handle)return false;
        pending[count++]=handle;return true;
    }
    template<class Wait,class Release> void Drain(Wait wait,Release release) {
        if(!count)return;
        // A failed fence leaves every image owned. A failed release retains
        // only unreleased images, so cancellation can retry without duplicates.
        if(!completed){wait();completed=true;}
        while(count){
            release(pending[0]);
            for(std::size_t i=1;i<count;++i)pending[i-1]=pending[i];
            pending[--count]=Handle{};
        }
        completed=false;
    }
};
}
