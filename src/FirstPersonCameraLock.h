#pragma once
#include <cstring>
namespace diagnostic
{
inline bool IsFirstPersonPermission(char const* name)
{
    return name&&std::strcmp(name,"IsFirstPersonCameraAllowed")==0;
}
// Always execute the current native/modded script dispatcher, forwarding all
// arguments. Only a camera permission refusal may be overridden by the caller's
// VR/player predicate. Damage, AI, animations and error results stay native.
template<typename Callback,typename Admin,typename Name,typename Entity,typename Predicate>
int CallCameraScript(Callback original,Admin* admin,Name const& name,Entity* self,Entity* other,int args,Predicate permit)
{
    int result=original(admin,name,self,other,args);
    if(result==0&&permit(name,self))return 1;
    return result;
}
}
