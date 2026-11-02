#include "NockLease.h"
#include <cstdio>
using namespace diagnostic::ranged;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    NockLease lease;int reads=0,touches=0;NockSlot slot{22,7};
    auto read=[&]{++reads;return slot;};auto validate=[&](uintptr_t item){++touches;return item==22;};
    lease.Borrow(11,22,7);CHECK(lease.WithCurrent(11,read,validate)&&reads==1&&touches==1);
    slot={99,8};CHECK(!lease.WithCurrent(11,read,validate)&&touches==1); // old arrow deleted by native Shoot/equipment
    slot={22,8};CHECK(!lease.WithCurrent(11,read,validate)&&touches==1); // address reused, different stack
    int before=reads;CHECK(!lease.WithCurrent(33,read,validate)&&reads==before); // old save/player
    lease.Clear();CHECK(!lease.WithCurrent(11,read,validate)&&reads==before&&touches==1);
    slot={44,9};lease.Borrow(11,44,9);CHECK(lease.WithCurrent(11,read,[](uintptr_t item){return item==44;}));
    lease.Clear();lease.Clear();CHECK(lease.item==0&&lease.player==0&&lease.stack==-1);
    puts("PASS native slot replacement never dereferences stale ammo; cancelling borrow makes no native calls; fresh ammo rebinds");return 0;
}
