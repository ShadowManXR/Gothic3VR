#include "ResolutionContext.h"
#include <cstdio>
int main(){
    unsigned char gfx[0x618]{},wrong[0x560]{},native[0x560]{};
    using Context=diagnostic::ScopedResolutionContext;using C=Context;
    C::Write(native,0x54c,2560);C::Write(native,0x550,1440);C::Write(native,0x554,75);C::Write(native,0x558,22);
    C::Write(wrong,0,1);C::Write(wrong,0x558,22);
    unsigned char* contexts[]={wrong,native};C::Write(gfx,0x60c,unsigned(reinterpret_cast<uintptr_t>(contexts)));C::Write(gfx,0x610,2);
    Context scope;
    if(!scope.Select(gfx,3200,3200,0)||C::Read(native,0x54c)!=3200||C::Read(native,0x550)!=3200||C::Read(wrong,0x54c)!=0||C::Read(native,0x554)!=75)return 1;
    if(scope.Select(gfx,3840,2160,0))return 2;
    scope.Restore();if(C::Read(native,0x54c)!=2560||C::Read(native,0x550)!=1440)return 3;
    if(scope.Select(gfx,3840,2160,2))return 4;
    if(scope.Select(nullptr,3840,2160,0))return 5;
    C::Write(gfx,0x610,99999);if(scope.Select(gfx,3840,2160,0))return 6;
    C::Write(gfx,0x610,2);if(!scope.Select(gfx,3840,2160,0))return 7;
    scope.Commit();scope.Restore();if(C::Read(native,0x54c)!=3840||C::Read(native,0x550)!=2160)return 8;
    puts("Custom context preserves adapter/format/refresh, rolls back failure, and retains successful dimensions for native UI ModeIndex lookup");return 0;
}
