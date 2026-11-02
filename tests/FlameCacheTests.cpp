#include "HeldWeaponVisuals.h"
#include <windows.h>
#include <cstdio>
#include <vector>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Node {char const* name="";int type=0,coordinates=0;bool valid=true,instanced=false;uintptr_t particle=0;Node* parent=nullptr;std::vector<Node*> children;};
unsigned accesses=0;
struct Access {
    using Entity=Node;
    bool Valid(Node* n)const{++accesses;return n&&n->valid;}
    int Type(Node* n)const{++accesses;return n->type;}
    bool IsFlameSword(Node* n)const{++accesses;return visual::FlameSword(n->name);}
    Node* Parent(Node* n)const{++accesses;return n->parent;}
    int Children(Node* n)const{++accesses;return int(n->children.size());}
    Node* Child(Node* n,int i)const{++accesses;return n->children[i];}
    uintptr_t WorldTail(Node* n)const{++accesses;return visual::FlameWorldTail(n->name,n->coordinates,n->instanced)?n->particle:0;}
};
__declspec(noinline) bool Lookup(visual::FlameTailTargets const& targets,uintptr_t item){return targets.Contains(item);}
int main(){
    visual::FlameTailTargets targets;Access access;Node sword,copy,local,glow,tail,npc,world;
    sword.name="It_1H_FlameSword_01";sword.type=2;
    copy.name="VFX_Magic_Weapons_FlameSword_01";copy.particle=101;
    tail.name="VFX_Magic_Weapons_FlameSword_04";tail.particle=104;
    local.name="VFX_Magic_Weapons_FlameSword_03";local.coordinates=1;local.particle=103;
    glow.name="VFX_Magic_Weapons_FlameSword_02";glow.coordinates=1;glow.particle=102;
    sword.children={&copy,&local,&glow,&tail};for(auto* n:sword.children)n->parent=&sword;
    CHECK(visual::CollectFlameWeapon(access,&sword,targets));CHECK(targets.Count()==2&&targets.Contains(101)&&targets.Contains(104));
    CHECK(!targets.Contains(102)&&!targets.Contains(103)&&!targets.Contains(0)&&!targets.Contains(1));
    auto before=accesses;unsigned hits=0;LARGE_INTEGER frequency,start,end;QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
    for(unsigned i=0;i<2000000;++i)hits+=Lookup(targets,uintptr_t(1000+i));
    QueryPerformanceCounter(&end);CHECK(hits==0&&accesses==before);
    printf("PASS two million unrelated emitter lookups: %.3fms, zero object/name/property/memory/owner queries\n",double(end.QuadPart-start.QuadPart)*1000/frequency.QuadPart);
    CHECK(visual::CollectFlameWeapon(access,&sword,targets)&&targets.Count()==2); // duplicate eye/frame collection cannot grow list
    Node left=sword,leftCopy=copy;left.name="It_1H_MagicFlameSword_01";left.children={&leftCopy};leftCopy.parent=&left;leftCopy.particle=201;
    CHECK(visual::CollectFlameWeapon(access,&left,targets)&&targets.Count()==3&&targets.Contains(201));
    targets.Reset();CHECK(targets.Count()==0&&!targets.Contains(101)&&!targets.Contains(201));
    // Normal melee/empty hands never traverse a child tree, even with a huge
    // unrelated scene. Only caller-supplied currently equipped roots are seen.
    sword.name="It_1H_Katana_01";before=accesses;
    CHECK(visual::CollectFlameWeapon(access,&sword,targets)&&targets.Count()==0&&accesses-before==3);
    CHECK(visual::CollectFlameWeapon(access,nullptr,targets)&&targets.Count()==0);
    before=accesses;for(unsigned i=0;i<2000000;++i)hits+=Lookup(targets,uintptr_t(i));CHECK(hits==0&&accesses==before);
    sword.name="It_1H_FlameSword_01";npc=sword;npc.name="It_1H_Katana_01";copy.parent=&npc;
    CHECK(!visual::CollectFlameWeapon(access,&sword,targets));targets.Reset();copy.parent=&sword;
    copy.valid=false;CHECK(!visual::CollectFlameWeapon(access,&sword,targets));targets.Reset();copy.valid=true;
    sword.children={&sword};sword.parent=&sword;CHECK(!visual::CollectFlameWeapon(access,&sword,targets));targets.Reset();
    for(uintptr_t i=1;i<=8;++i)CHECK(targets.Add(i));CHECK(!targets.Add(9)&&targets.Count()==0); // overflow cannot leave partial ownership
    CHECK(!targets.Add(0)&&targets.Count()==0);
    CHECK(targets.Add(101));targets.Reset();CHECK(!targets.Contains(101)); // world stop / tracking loss / native menu
    CHECK(targets.Add(201)&&!targets.Contains(101));targets.Reset();CHECK(targets.Count()==0); // new save cannot inherit old pointers
    puts("PASS bounded equipped-root scan, dual wield, empty/normal weapon fast path, live child ownership, lifecycle clears, corrupt/cyclic tree and capacity refusal");return 0;
}
