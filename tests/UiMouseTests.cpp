#include "UiMouseCoordinates.h"
#include <cstdio>
int main(){
    unsigned failures=0;auto check=[&](bool ok,char const* message){if(!ok){++failures;printf("FAIL %s\n",message);}};
    using diagnostic::UiMouseAxis;
    check(UiMouseAxis(2559,0,2560,3840)==3839,"monitor right edge reaches full 4K menu");
    check(UiMouseAxis(1439,0,1440,2160)==2159,"monitor bottom edge reaches full menu");
    check(UiMouseAxis(0,0,2560,3840)==0,"left menu edge");
    check(UiMouseAxis(640,640,3200,3840)==0&&UiMouseAxis(3199,640,3200,3840)==3839,"oversized partially visible window still covers entire menu");
    check(UiMouseAxis(-50,0,2560,3840)==0&&UiMouseAxis(3000,0,2560,3840)==3839,"captured pointer outside bounds is clamped");
    check(UiMouseAxis(100,0,3840,3840)==100,"native equal-size coordinates unchanged");
    check(UiMouseAxis(2559,0,2560,3200)==3199&&UiMouseAxis(1439,0,1440,3200)==3199,"square render resolution covers both axes");
    check(UiMouseAxis(100,0,0,3840)==100,"invalid bounds bypass mapping");
    return failures?1:0;
}
