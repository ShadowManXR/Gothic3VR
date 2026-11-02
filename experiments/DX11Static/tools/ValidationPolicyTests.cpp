#include "../../../src/RuntimeProbePolicy.h"
#include "../../../src/NativeTrialMode.h"
#include <iostream>
int main(){
    static_assert(!diagnostic::probes::Enabled&&diagnostic::probes::Dx11ValidationOnly&&diagnostic::probes::StaticTrial);
    if(diagnostic::probes::Now()!=0)return 1;
    for(unsigned i=1;i<=5;++i){wchar_t text[]{wchar_t(L'0'+i),0};if(diagnostic::NativeTrialMode(text,1,true)!=(i==5?5u:0u))return 2;}
    std::cout<<"PASS narrow DX11 mode5 only; general CPU/GPU/temporal probes disabled\n";
}
