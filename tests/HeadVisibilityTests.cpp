#include "FirstPersonHeadVisibility.h"
#include "TemporaryOverride.h"
#include <windows.h>
#include <cstdio>
using namespace diagnostic;
struct Node {bool visible=true;Node* child=nullptr;bool fault=false;};
void __cdecl SetNode(void* object,bool visible)
{
    auto& node=*static_cast<Node*>(object);node.visible=visible;
    if(node.child)SetNode(node.child,visible);
    if(node.fault&&!visible)RaiseException(0xe0424242,0,0,nullptr);
}
struct Trial {RenderVisibilitySnapshot snapshot;Node* parent;Node* child;unsigned eyes=0;bool fault=false;};
void __cdecl Render(void* object)
{
    auto& trial=*static_cast<Trial*>(object);trial.snapshot.Hide();
    for(unsigned i=0;i<2;++i)
    {
        if(trial.parent->visible||trial.child->visible)RaiseException(0xe0424243,0,0,nullptr);
        ++trial.eyes;
        if(trial.fault)RaiseException(0xe0424242,0,0,nullptr);
    }
}
void __cdecl Restore(void* object){static_cast<Trial*>(object)->snapshot.Restore();}
bool FaultTrial(Trial& trial)
{
    __try {CallWithRestoration(Render,Restore,&trial);}
    __except(GetExceptionCode()==0xe0424242?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
    return false;
}
int __cdecl main()
{
    unsigned failures=0;
    auto check=[&](bool ok,char const* name){std::printf("%s %s\n",ok?"PASS":"FAIL",name);if(!ok)++failures;};
    check(HideHeadInCamera({true,true,true,false,false})&&HideHeadInCamera({true,true,false,true,false}),"first-person and temporary first-person");
    check(!HideHeadInCamera({true,true,true,false,true})&&!HideHeadInCamera({true,true,false,false,false})&&
        !HideHeadInCamera({false,true,true,false,false})&&!HideHeadInCamera({true,false,true,false,false}),"dialog, third-person, menu and unavailable camera preserve head");
    check(IsHeadBodyPart("Slot_Head")&&IsHeadBodyPart("Slot_Hair")&&IsHeadBodyPart("Slot_Beard")&&IsHeadBodyPart("Slot_Helmet")&&
        !IsHeadBodyPart("Slot_Body")&&!IsHeadBodyPart("Slot_Armor")&&!IsHeadBodyPart("Slot_RightHand")&&!IsHeadBodyPart(nullptr),"exact head slots preserve body and hands");
    Node child{false},parent{true,&child};Trial trial;trial.parent=&parent;trial.child=&child;
    check(trial.snapshot.Remember(&parent,parent.visible,SetNode)&&trial.snapshot.Remember(&child,child.visible,SetNode)&&
        trial.snapshot.Remember(&parent,false,SetNode)&&trial.snapshot.count==2,"duplicates preserve first snapshot");
    CallWithRestoration(Render,Restore,&trial);
    check(trial.eyes==2&&parent.visible&&!child.visible&&!trial.snapshot.active,"both eyes hidden, recursive flags restored individually");
    trial.fault=true;
    check(FaultTrial(trial)&&parent.visible&&!child.visible&&!trial.snapshot.active,"render fault restores original visibility");
    trial.fault=false;parent.fault=true;
    check(FaultTrial(trial)&&parent.visible&&!child.visible&&!trial.snapshot.active,"setter fault after mutation restores original visibility");
    RenderVisibilitySnapshot full;Node nodes[RenderVisibilitySnapshot::Capacity+1];
    bool bounded=true;for(unsigned i=0;i<RenderVisibilitySnapshot::Capacity;++i)bounded&=full.Remember(&nodes[i],true,SetNode);
    bounded&=!full.Remember(&nodes[RenderVisibilitySnapshot::Capacity],true,SetNode);full.Restore();
    for(auto const& node:nodes)bounded&=node.visible;
    check(bounded,"capacity refusal before Hide changes nothing");
    return failures?1:0;
}
