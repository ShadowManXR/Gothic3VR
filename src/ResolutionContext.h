#pragma once
#include "VirtualHook.h"
namespace diagnostic {
// CP 1.75.20108.14 layout verified against CreateGraphicsSubsystem and the SDK
// eSGfxContextDesc (0x560, ModeDesc at 0x54c). Caller checks the engine hash.
struct ScopedResolutionContext {
    unsigned char* context=nullptr;
    unsigned previousWidth=0,previousHeight=0;
    static unsigned Read(unsigned char const* p,size_t offset){unsigned n;memcpy(&n,p+offset,4);return n;}
    static void Write(unsigned char* p,size_t offset,unsigned n){memcpy(p+offset,&n,4);}
    bool Select(unsigned char* gfx,unsigned width,unsigned height,unsigned adapter) {
        if(context||!Readable(gfx,0x618))return false;
        unsigned count=Read(gfx,0x610);auto contexts=reinterpret_cast<unsigned char**>(Read(gfx,0x60c));
        if(!count||count>4096||!Readable(contexts,count*4))return false;
        for(unsigned i=0;i<count;++i){auto c=contexts[i];
            if(!Readable(c,0x560)||Read(c,0)!=adapter||Read(c,0x558)!=22)continue;
            // Mode dimensions are writable heap data owned by the native gfx
            // admin. Never add an allocation with a mismatched native allocator.
            MEMORY_BASIC_INFORMATION info{};if(!VirtualQuery(c,&info,sizeof(info)))continue;
            DWORD protection=info.Protect&0xff;
            if(protection!=PAGE_READWRITE&&protection!=PAGE_EXECUTE_READWRITE)continue;
            previousWidth=Read(c,0x54c);previousHeight=Read(c,0x550);context=c;
            Write(c,0x54c,width);Write(c,0x550,height);return true;
        }
        return false;
    }
    void Restore(){if(context){Write(context,0x54c,previousWidth);Write(context,0x550,previousHeight);context=nullptr;}}
    // On successful creation the native UI subsequently resolves ModeIndex
    // through this same list. Keep the custom dimensions for this session.
    // Native gfx admin still owns and frees the descriptor normally.
    void Commit(){context=nullptr;}
};
}
