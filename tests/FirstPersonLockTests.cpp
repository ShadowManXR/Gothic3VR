#include "FirstPersonCameraLock.h"
#include <cstdio>
#include <stdexcept>
struct Admin
{
    int result=0,calls=0,args=0;int* actor=nullptr;int* other=nullptr;
    char const* name=nullptr;bool throwScript=false;
};
int NativeScript(Admin* admin,char const* const& name,int* actor,int* other,int args)
{
    ++admin->calls;admin->name=name;admin->actor=actor;admin->other=other;admin->args=args;
    if(admin->throwScript)throw std::runtime_error("native game exception");
    return admin->result;
}
int __cdecl main()
{
    unsigned checks=0,failures=0;auto check=[&](bool ok,char const* label){++checks;if(!ok){++failures;printf("FAIL %s\n",label);}};
    Admin admin;int player=11,npc=22,other=33;bool vr=true,shutdown=false;unsigned predicates=0;
    auto permit=[&](char const* name,int* actor)
    {
        ++predicates;
        check(admin.calls>0,"original dispatcher runs before the permission override");
        return vr&&!shutdown&&actor==&player&&diagnostic::IsFirstPersonPermission(name);
    };
    auto call=[&](char const* name,int* actor,int args){return diagnostic::CallCameraScript(NativeScript,&admin,name,actor,&other,args,permit);};
    check(call("IsFirstPersonCameraAllowed",&player,71)==1,"VR player retains first-person when knockdown refuses the camera");
    check(admin.calls==1&&admin.actor==&player&&admin.other==&other&&admin.args==71&&diagnostic::IsFirstPersonPermission(admin.name),"native dispatcher receives original arguments exactly once");
    check(call("IsFirstPersonCameraAllowed",&npc,8)==0,"NPC camera permissions stay native");
    vr=false;check(call("IsFirstPersonCameraAllowed",&player,9)==0,"VR disabled restores native refusal immediately");
    vr=true;shutdown=true;check(call("IsFirstPersonCameraAllowed",&player,10)==0,"shutdown restores native camera permission");shutdown=false;
    check(call("AssessDamage",&player,120)==0&&admin.args==120,"damage script and arguments remain unchanged");
    check(call("AI_StandUp",&player,4)==0,"stand-up animation script remains native");
    check(call("IsFirstPersonCameraAllowedExtended",&player,0)==0,"only the exact camera permission script is intercepted");
    admin.result=1;auto previous=predicates;check(call("IsFirstPersonCameraAllowed",&player,0)==1&&previous==predicates,"native allowance returned without override predicate");
    admin.result=-1;check(call("IsFirstPersonCameraAllowed",&player,0)==-1,"native error value is preserved");
    admin.result=120;check(call("AssessDamage",&player,17)==120,"nonboolean damage result is preserved");
    admin.throwScript=true;bool propagated=false;try{call("IsFirstPersonCameraAllowed",&player,0);}catch(std::runtime_error const&){propagated=true;}
    check(propagated,"native script exceptions propagate without being swallowed");
    check(!diagnostic::IsFirstPersonPermission(nullptr),"null script names are not matched");
    printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
