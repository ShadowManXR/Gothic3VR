#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic {
struct TrackedPalette {
    static constexpr unsigned Limit=256;
    arms::Mat saved[Limit];void* address=nullptr;unsigned count=0;
    bool Begin(void* data,unsigned n){if(address||!data||!n||n>Limit)return false;address=data;count=n;memcpy(saved,data,n*sizeof(arms::Mat));return true;}
    bool Restore(void* current,unsigned n){if(!address)return true;bool same=current==address&&n==count;if(same)memcpy(current,saved,count*sizeof(arms::Mat));address=nullptr;count=0;return same;}
};
}
