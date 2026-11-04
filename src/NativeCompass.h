#pragma once
#include <g3sdk/Script.h>
#include "NativeApplication.h"
#include "VirtualHook.h"

namespace diagnostic {
// CP 1.75.14 Game.dll: the HUD binds IDC_STATIC_COMPASS_2 (20545) to
// its member at +0x2b8, whose dedicated vtable is at 0x2d825c.
// Replace only OnPaint: no persistent visibility state or resource edits.
class NativeCompass {
    VirtualHook paint;
public:
    bool Install(void* replacement) {
        auto game=GetModuleHandleW(L"Game.dll");wchar_t path[MAX_PATH];unsigned char digest[32]{};
        constexpr unsigned char expected[32]={0x47,0x28,0xa4,0x7b,0x24,0x4a,0xdc,0xb0,0xea,0x2e,0xc3,0x50,0x1d,0xb0,0x49,0x94,0x90,0x9d,0xcf,0xcf,0xf3,0x38,0xc7,0xb8,0x07,0x5f,0x2b,0x68,0xdd,0x8d,0x6e,0x00};
        if(!game||!GetModuleFileNameW(game,path,MAX_PATH)||!FileSha256(path,digest)||memcmp(digest,expected,32))return false;
        auto base=reinterpret_cast<unsigned char*>(game);
        auto table=reinterpret_cast<void**>(base+0x2d825c);size_t slot=0;
        if(!VirtualSlot(&CFFGFCWnd::OnPaint,slot)||slot!=44||!Readable(table+slot,sizeof(void*))||table[slot]!=base+0xa6940)return false;
        return paint.Prepare(table,slot,replacement)&&paint.Install();
    }
    void Paint(CFFGFCWnd* window)const {
        reinterpret_cast<void(__thiscall*)(CFFGFCWnd*)>(paint.original)(window);
    }
};
}
