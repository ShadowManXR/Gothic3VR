#include "ControllerFocusQuery.h"
#include "PhysicalBow.h"
#include <cstdio>
#include <limits>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    // Both native AutoCollectFocus(false) and GetCrossHairEntity(true) use
    // weapon aiming, but MUST retain their native query argument. Promoting
    // the automatic query to true crashed with an absent native distance list.
    for(bool crossHair:{false,true}){
        auto bow=PlanControllerFocus(crossHair,true,false);
        CHECK(bow.rangedAim&&bow.nativeCrossHair==crossHair&&bow.weaponAim&&bow.candidateQuery==!crossHair);
        auto crossbow=PlanControllerFocus(crossHair,true,false);
        CHECK(crossbow.rangedAim&&crossbow.nativeCrossHair==crossHair&&crossbow.weaponAim&&crossbow.candidateQuery==!crossHair);
    }
    auto interaction=PlanControllerFocus(false,false,false);
    CHECK(!interaction.rangedAim&&!interaction.nativeCrossHair&&!interaction.weaponAim);
    auto spell=PlanControllerFocus(false,false,true);
    CHECK(spell.rangedAim&&!spell.nativeCrossHair&&spell.weaponAim);
    auto unarmedCrosshair=PlanControllerFocus(true,false,false);
    CHECK(unarmedCrosshair.nativeCrossHair&&!unarmedCrosshair.weaponAim);
    for(bool crossHair:{false,true})for(bool ranged:{false,true})for(bool magic:{false,true}){
        auto plan=PlanControllerFocus(crossHair,ranged,magic);
        CHECK(plan.nativeCrossHair==crossHair); // never request additional native physics tracing
        CHECK(plan.candidateQuery==(ranged&&!crossHair));
    }

    for(bool crossbow:{false,true}){
        bow::Sample s;s.valid=true;s.player=1;s.weapon=2;s.crossbow=crossbow;
        s.nativeNock=true;s.launchNock={125,140,-20};
        s.launchDirection={.25f,-.10f,1};CHECK(arms::Unit(s.launchDirection));
        float controllerEuler[3]{.1f,.6f,.25f};s.leftAim=arms::Rotation(NativeCameraRotation(controllerEuler));
        WorldAim focus;CHECK(bow::FocusAim(s,focus));
        CHECK(std::fabs(focus.origin[0]-s.launchNock.x)<.001f&&std::fabs(focus.origin[1]-s.launchNock.y)<.001f&&std::fabs(focus.origin[2]-s.launchNock.z)<.001f);
        auto actual=NativeCameraRotation(focus.rotation);
        for(unsigned axis=0;axis<3;++axis)CHECK(std::fabs(actual.m[axis*3+2]-focus.direction[axis])<.0001f);
        float farTarget[3]{focus.origin[0]+focus.direction[0]*3000,focus.origin[1]+focus.direction[1]*3000,focus.origin[2]+focus.direction[2]*3000},azi=0,elev=0;
        CHECK(WorldAimAngles(focus,farTarget,azi,elev)&&std::fabs(azi)<.001f&&std::fabs(elev)<.001f);
        float lo[3],hi[3];for(unsigned axis=0;axis<3;++axis){lo[axis]=farTarget[axis]-10;hi[axis]=farTarget[axis]+10;}
        float rating=0;CHECK(!WorldAimBox(focus,lo,hi,120,100,rating)); // interaction remains short-reach
        // Looking straight up/down must still produce a finite ray when the
        // original up vector becomes parallel to the launch direction.
        s.launchDirection={0,1,0};s.leftAim={};CHECK(bow::FocusAim(s,focus));
        actual=NativeCameraRotation(focus.rotation);CHECK(actual.m[5]>.999f);
        s.launchDirection={};CHECK(!bow::FocusAim(s,focus));
        s.launchDirection={0,0,1};s.launchNock.x=std::numeric_limits<float>::quiet_NaN();CHECK(!bow::FocusAim(s,focus));
        s.launchNock={};s.valid=false;CHECK(!bow::FocusAim(s,focus));
    }
    puts("Ranged focus: persistent native HUD route and evaluated projectile direction passed");return 0;
}
