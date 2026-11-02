#pragma once
#include <cstdint>
namespace diagnostic::ranged {
struct NockSlot {uintptr_t item=0;int stack=-1;};
// Borrowed identity only. Clear never calls native inventory code. Resolve
// the live slot before touching an entity a script may already have deleted.
struct NockLease {
    uintptr_t player=0,item=0;int stack=-1;
    void Clear(){player=item=0;stack=-1;}
    void Borrow(uintptr_t owner,uintptr_t entity,int index){player=owner;item=entity;stack=index;}
    template<class ReadSlot,class Validate> bool WithCurrent(uintptr_t owner,ReadSlot read,Validate validate)const{
        if(!player||player!=owner||!item||stack<0)return false;
        auto current=read();
        return current.item==item&&current.stack==stack&&validate(current.item);
    }
};
}
