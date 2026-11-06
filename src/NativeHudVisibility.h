#pragma once
#include <g3sdk/Script.h>
#include "NativeApplication.h"
#include "FocusHook.h"
#include "HudVisibility.h"
namespace diagnostic {
// Supported GFC.dll: WindowProc routes the native paint/erase messages to the
// SDK callbacks. Filtering here covers panel children without changing window
// visibility, input, game values, or the native conditions for showing a bar.
class NativeHudVisibility {
    FocusHook hook;
    unsigned char* gameBase=nullptr;
    unsigned ControlId(CFFGFCWnd* window)const{
        if(!Readable(window,sizeof(*window)))return 0;
        void* table=nullptr;memcpy(&table,window,sizeof(table));
        auto address=reinterpret_cast<uintptr_t>(table),base=reinterpret_cast<uintptr_t>(gameBase);
        return HudViewControlId(window->GetDlgCtrlID(),gameBase&&address>=base?address-base:0);
    }
public:
    bool Install(void* replacement){
        auto module=GetModuleHandleW(L"GFC.dll");wchar_t path[MAX_PATH];unsigned char digest[32]{};
        constexpr unsigned char expected[]={0x83,0x1e,0x08,0x58,0x0f,0x16,0xf7,0xa9,0x4a,0xa7,0x6e,0xad,0x83,0x4d,0xbd,0x25,0x05,0xc8,0x1b,0x22,0x63,0x23,0x92,0x9f,0xb9,0x18,0x06,0xf3,0x0d,0x37,0x6b,0x30};
        if(!module||!GetModuleFileNameW(module,path,MAX_PATH)||!FileSha256(path,digest)||memcmp(digest,expected,32))return false;
        auto game=GetModuleHandleW(L"Game.dll");
        constexpr unsigned char gameExpected[]={0x47,0x28,0xa4,0x7b,0x24,0x4a,0xdc,0xb0,0xea,0x2e,0xc3,0x50,0x1d,0xb0,0x49,0x94,0x90,0x9d,0xcf,0xcf,0xf3,0x38,0xc7,0xb8,0x07,0x5f,0x2b,0x68,0xdd,0x8d,0x6e,0x00};
        if(!game||!GetModuleFileNameW(game,path,MAX_PATH)||!FileSha256(path,digest)||memcmp(digest,gameExpected,32))return false;
        gameBase=reinterpret_cast<unsigned char*>(game);
        auto entry=GetProcAddress(module,"?WindowProc@CFFGFCWnd@@MAEHIII@Z");
        return reinterpret_cast<unsigned char*>(entry)==reinterpret_cast<unsigned char*>(module)+0x3920&&
            hook.Install(entry,replacement,FocusHook::FindPrefix,sizeof(FocusHook::FindPrefix));
    }
    bool Hidden(CFFGFCWnd* window,HudVisibility const& show)const{
        return HiddenHudWindow(window,show,[this](CFFGFCWnd* w){return ControlId(w);},
            [](CFFGFCWnd* w)->CFFGFCWnd*{return Readable(w,sizeof(*w))?w->GetParentWindow():nullptr;});
    }
    int Dispatch(CFFGFCWnd* self,unsigned message,unsigned w,unsigned l)const{
        return reinterpret_cast<int(__thiscall*)(CFFGFCWnd*,unsigned,unsigned,unsigned)>(hook.original)(self,message,w,l);
    }
};
static_assert(IDC_STATIC_COMPASS_2==20545&&IDC_VIEW_BAR_QUICKSLOTS_2==20379&&IDC_LISTCTRL_QUICKSLOTS_2==20380);
static_assert(IDC_VIEW_BAR_FOCUS_2==20386&&IDC_PROGRESSBAR_FOCUS_2==20381);
static_assert(IDC_VIEW_BAR_HEALTH_2==20387&&IDC_VIEW_BAR_ENDURANCE_2==20388&&IDC_VIEW_BAR_MANA_2==20389);
static_assert(IDC_PROGRESSBAR_ENDURANCE_2==20382&&IDC_PROGRESSBAR_MANA_2==20383&&IDC_PROGRESSBAR_HEALTH_2==20384);
}
