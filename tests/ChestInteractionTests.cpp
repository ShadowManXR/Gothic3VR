#include "ChestInteraction.h"
#include "CookingInteraction.h"
#include "LecternInteraction.h"
#include "ChestSkinPose.h"
#include "DialogCameraLock.h"
#include "RenderAlphaSnapshot.h"
#include <cstdio>
#include <limits>
#include <initializer_list>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
void __cdecl Alpha(void* object,float value){*static_cast<float*>(object)=value;}
int main(){
    CHECK(LecternInteraction(true,96,nullptr,nullptr));
    for(auto state:{"PS_Interact","PS_Interact_Bookstand","PS_Interact_End"}){
        CHECK(LecternInteraction(true,0,state,nullptr));
        CHECK(LecternInteraction(true,0,nullptr,state));
        CHECK(!LecternInteraction(false,0,state,state));
    }
    CHECK(LecternInteraction(false,0,"PS_Interact_End",nullptr,true));
    CHECK(!LecternInteraction(false,0,"PS_Normal_Loop",nullptr,true));
    CHECK(!LecternInteraction(false,96,"PS_Interact_Bookstand",nullptr));
    for(auto state:{"PS_Interact_Chest","PS_Interact_CampFire","PS_Interact_BookstandOther","PS_Normal_Take"})
        CHECK(!LecternInteraction(true,0,state,nullptr));
    for(auto state:{"PS_Interact","PS_Interact_Chest","PS_Interact_Chest_Loop","PS_Interact_End"}){
        CHECK(ChestInteraction(true,0,state,state));CHECK(!ChestInteraction(false,0,state,state));}
    CHECK(ChestInteraction(true,99,nullptr,nullptr)&&ChestInteraction(true,100,nullptr,nullptr));
    CHECK(!ChestInteraction(false,99,"PS_Interact_Chest","PS_Interact_Chest"));
    CHECK(ChestInteraction(false,0,"PS_Interact_End","PS_Interact_End",true));
    CHECK(!ChestInteraction(false,0,"PS_Normal_Loop","PS_Normal_Loop",true));
    for(auto state:{"PS_Interact_ChestOther","PS_Normal_Take","PS_Interact_CampFire","PS_Interact_Mining"})
        CHECK(!ChestInteraction(true,0,"PS_Player",state));
    CHECK(SkipChestApproach(true,true,true,"PS_Interact"));
    CHECK(!SkipChestApproach(false,true,true,"PS_Interact"));
    CHECK(!SkipChestApproach(true,false,true,"PS_Interact"));
    CHECK(!SkipChestApproach(true,true,false,"PS_Interact"));
    CHECK(!SkipChestApproach(true,true,true,"PS_Interact_End"));
    CHECK(!SkipChestApproach(true,true,true,nullptr));
    CHECK(CookingProp("It_Stick")&&CookingProp("It_Meat"));
    CHECK(!CookingProp(nullptr)&&!CookingProp("It_StickOther")&&!CookingProp("It_Sword"));
    CHECK(CookingTarget(61)&&CookingTarget(88)&&!CookingTarget(89)&&!CookingTarget(106));
    for(auto state:{"PS_Interact_Campfire","PS_Interact_Barbecue","PS_Interact_Loop","PS_Interact_End"}){
        CHECK(CookingInteraction(true,0,state,nullptr));
        CHECK(!CookingInteraction(false,0,state,nullptr));
    }
    CHECK(CookingInteraction(false,0,"PS_Interact_End",nullptr,true));
    CHECK(CookingInteraction(true,73,nullptr,nullptr));
    CHECK(CookingInteraction(true,94,nullptr,nullptr));
    CHECK(!CookingInteraction(true,0,"PS_Normal_Loop",nullptr));
    CHECK(!CookingInteraction(true,0,"PS_Interact_CampfireOther",nullptr));
    ChestViewTransition transition;
    CHECK(!transition.Step(1,true,false,true,0));
    CHECK(transition.Step(1,true,true,false,10));
    CHECK(transition.Step(1,true,false,false,100)); // Task ended, camera still showing hero.
    CHECK(transition.Step(1,true,false,true,200));
    CHECK(transition.Step(1,true,false,false,250)); // A transient first-person frame is insufficient.
    CHECK(transition.Step(1,true,false,true,300));
    CHECK(transition.Step(1,true,false,true,499));
    CHECK(!transition.Step(1,true,false,true,500));
    CHECK(transition.Step(1,true,true,false,600));
    CHECK(transition.Step(1,true,false,false,700));
    CHECK(!transition.Step(1,true,false,false,2700)); // Third-person configuration cannot latch forever.
    CHECK(transition.Step(1,true,true,false,3000));
    CHECK(!transition.Step(2,true,false,false,3010)); // Replaced player.
    CHECK(transition.Step(2,true,true,false,3020));
    CHECK(!transition.Step(2,false,true,false,3030)); // Recovery/dialogue/world exit wins immediately.
    ChestSkinPose pose;arms::Mat gameplay[3],native[3],before[3];float root[3]{10000,300,-15000},moved[3]{10030,310,-15020};
    for(unsigned i=0;i<3;++i){arms::Position(gameplay[i],{root[0]+i*5.f,root[1]+100,root[2]});arms::Position(native[i],{1,2,3});}
    CHECK(!pose.Apply(1,2,native,3,root)); // Cold entry uses scoped avatar hiding.
    CHECK(pose.Remember(1,2,gameplay,3,root));
    for(unsigned eye=0;eye<2;++eye){for(unsigned i=0;i<3;++i){native[i].m[0]=.25f+eye;arms::Position(native[i],{float(eye+i),2,3});}
        memcpy(before,native,sizeof(native));
        CHECK(!pose.Apply(3,2,native,3,moved)&&!memcmp(before,native,sizeof(native)));
        CHECK(!pose.Apply(1,4,native,3,moved)&&!memcmp(before,native,sizeof(native)));
        CHECK(!pose.Apply(1,2,native,2,moved)&&!memcmp(before,native,sizeof(native)));
        CHECK(pose.Apply(1,2,native,3,moved)&&pose.Pending());
        CHECK(!pose.Remember(1,2,native,3,moved));
        for(unsigned i=0;i<3;++i){CHECK(native[i].m[0]==gameplay[i].m[0]);
            for(unsigned axis=0;axis<3;++axis)CHECK(native[i].m[12+axis]==gameplay[i].m[12+axis]+moved[axis]-root[axis]);}
        CHECK(pose.Restore(native,3)&&!pose.Pending()&&!memcmp(before,native,sizeof(native)));}
    // Changed/rebuilt skin arrays are never overwritten during cleanup.
    CHECK(pose.Apply(1,2,native,3,root));arms::Mat replacement[3];auto untouched=replacement[0];
    CHECK(!pose.Restore(replacement,3)&&!memcmp(&untouched,replacement,sizeof(untouched))&&!pose.Matches(1,2,3));
    pose.Reset();CHECK(!pose.Apply(1,2,native,3,root));
    float body=.6f,weapon=.35f;RenderAlphaSnapshot alpha;
    CHECK(alpha.Remember(&body,body,Alpha)&&alpha.Remember(&weapon,weapon,Alpha));
    alpha.Hide();CHECK(body==0&&weapon==0);body=1;alpha.Hide();CHECK(body==0);alpha.Restore();CHECK(body==.6f&&weapon==.35f);
    DialogCameraLock camera;Rotation3 gameplayView;float base[3]{root[0],root[1]+165,root[2]+20};
    CHECK(camera.RememberPlayer(1,gameplayView,base,root,100));
    float angles[3]{.8f,2.f,.2f};auto interactionView=NativeCameraRotation(angles);
    CHECK(camera.CapturePlayer(1,root,interactionView,900,400,100));
    Rotation3 view;float actual[3];CHECK(camera.ApplyPlayer(1,root,view,actual));
    CHECK(RotationError(view,gameplayView)<.0001f&&!memcmp(base,actual,sizeof(base)));
    camera.Release();CHECK(!camera.Active());
    puts("PASS chest-only visual suppression, native lockpick context, two-eye pose restoration, rig changes, cold entry and first-person camera retention");return 0;
}
