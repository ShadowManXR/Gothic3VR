#pragma once
#include "VirtualHook.h"
#include "NativeApplication.h"
#include <atomic>
#include "LocatorBinaryProfile.h"
namespace diagnostic {
// The original locator polls eCKeyboard::KeyPressed (the engine's buffered
// events), not Win32 key state. Route only its imported KeyPressed call through
// this adapter. No global keyboard hook, no third-party file modifications.
class LocatorNativePulse {
    std::atomic<uint64_t> expires{0};std::atomic<unsigned> toggleKey{100};bool release=false;
public:
    std::atomic<uint64_t> polls{0},consumed{0};
    void Queue(uint64_t now,unsigned key=100){toggleKey.store(key);expires.store(now+1500);}
    void Cancel(){expires.store(0);}
    bool Read(unsigned key,uint64_t now){
        if(key!=toggleKey.load())return false;
        ++polls;
        if(release){release=false;return false;} // Native mod must observe a release between presses.
        auto due=expires.exchange(0);if(!due||now>=due)return false;
        ++consumed;release=true;return true;
    }
};
struct LocatorNativeInput {
    VirtualHook hook,projectionHook;LocatorNativePulse pulse;HMODULE module=nullptr;
    unsigned objectRva=0,keyRva=0,projectionRva=0;
    static constexpr char ProjectionSymbol[]="?GetWorldToScreenMatrix@eCCameraBase@@QBE?AVbCMatrix@@XZ";
    static constexpr char Symbol[]="?KeyPressed@eCKeyboard@@QBE_NW4eEKeyboardStateOffset@eCInpShared@@@Z";
    bool Install(HMODULE source,void* replacement){
        if(hook.installed)return true;
        wchar_t path[MAX_PATH]{};unsigned char hash[32]{};
        if(!source||!GetModuleFileNameW(source,path,MAX_PATH)||!FileSha256(path,hash))return false;
        auto profile=LocatorProfile(hash);if(!profile.object)return false;
        objectRva=profile.object;keyRva=profile.key;projectionRva=profile.projection;
        auto cell=reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(source)+keyRva);
        auto target=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"Engine.dll"),Symbol));
        if(!target||!Readable(cell,sizeof(void*))||*cell!=target||!hook.Prepare(cell,0,replacement)||!hook.Install())return false;
        module=source;return true;
    }
    bool Snapshot(int& key,int& enabled)const{
        key=enabled=-1;if(!module)return false;
        auto cell=reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(module)+objectRva);
        if(!Readable(cell,sizeof(void*)))return false;auto object=static_cast<unsigned char*>(*cell);
        if(!Readable(object,0x228))return false;
        memcpy(&key,object+0x220,4);enabled=object[0x18]!=0;return true;
    }
    bool Viewport(unsigned& width,unsigned& height)const{
        if(!module)return false;auto cell=reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(module)+objectRva);
        if(!Readable(cell,sizeof(void*)))return false;auto object=static_cast<unsigned char*>(*cell);if(!Readable(object,0x18))return false;
        memcpy(&width,object+0x10,4);memcpy(&height,object+0x14,4);return width>=32&&height>=32&&width<=16384&&height<=16384;
    }
    bool InstallProjection(void* replacement){
        if(projectionHook.installed)return true;if(!module||!hook.installed)return false;
        auto cell=reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(module)+projectionRva);
        auto target=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"Engine.dll"),ProjectionSymbol));
        return target&&Readable(cell,sizeof(void*))&&*cell==target&&projectionHook.Prepare(cell,0,replacement)&&projectionHook.Install();
    }
    bool Restore(){pulse.Cancel();bool projection=projectionHook.Restore();return hook.Restore()&&projection;}
};
}
