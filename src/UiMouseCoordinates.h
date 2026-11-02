#pragma once
#include <algorithm>
#include <cstdint>
namespace diagnostic {
inline int UiMouseAxis(int value,int begin,int end,int renderExtent) {
    if(end<=begin+1||renderExtent<2)return value;
    auto relative=std::clamp(value-begin,0,end-begin-1);
    return int((int64_t(relative)*(renderExtent-1)+(end-begin-1)/2)/(end-begin-1));
}
}
