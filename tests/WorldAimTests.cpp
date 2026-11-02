#include "WorldAim.h"
#include <cstdio>
#include <limits>
int main() {
    unsigned failures=0;auto check=[&](bool v,char const* message){if(!v){++failures;printf("FAIL %s\n",message);}};
    auto close=[](float a,float b){return std::fabs(a-b)<.001f;};
    float base[3]={100,200,300},centerPosition[3]={0,1.6f,0};diagnostic::Orientation center;
    diagnostic::ControllerPose pose{true,{.25f,1.3f,-.5f},{0,0,0,1}};diagnostic::WorldAim aim;diagnostic::Rotation3 native;
    check(diagnostic::ControllerWorldAim(native,base,center,centerPosition,pose,100,aim)&&close(aim.origin[0],125)&&close(aim.origin[1],170)&&close(aim.origin[2],350)&&close(aim.direction[2],1),"hand origin and -Z OpenXR ray use the same world conversion as stereo eyes");
    float target[3]={125,170,550},azi,elev;
    check(diagnostic::WorldAimAngles(aim,target,azi,elev)&&close(azi,0)&&close(elev,0),"object on hand ray has zero focus angles even when offset from player/head");
    target[0]+=200;check(diagnostic::WorldAimAngles(aim,target,azi,elev)&&close(azi,45),"native horizontal focus angle convention");
    target[0]-=200;target[1]-=200;check(diagnostic::WorldAimAngles(aim,target,azi,elev)&&close(elev,-45),"ground objects use controller pitch independently from native camera");
    pose.quaternion[0]=-std::sin(.3926990817f);pose.quaternion[3]=std::cos(.3926990817f);
    check(diagnostic::ControllerWorldAim(native,base,center,centerPosition,pose,100,aim)&&aim.direction[1]<-.7f&&diagnostic::WorldAimAngles(aim,target,azi,elev)&&close(elev,0),"aiming down aligns a ground item without pitching the headset or game horizon");
    pose.quaternion[0]=0;pose.quaternion[3]=1;float yaw[3]={0,1.5707963268f,0};
    check(diagnostic::ControllerWorldAim(diagnostic::NativeCameraRotation(yaw),base,center,centerPosition,pose,100,aim)&&close(aim.origin[0],150)&&close(aim.origin[2],275)&&close(aim.direction[0],1),"body turn rotates both ray position and direction");
    center={std::cos(.4f),0,std::sin(.4f),0};pose.quaternion[1]=center.y;pose.quaternion[3]=center.w;
    check(diagnostic::ControllerWorldAim(native,base,center,centerPosition,pose,100,aim)&&close(aim.direction[2],1),"headset recenter is applied to controller direction exactly once");
    pose.valid=false;check(!diagnostic::ControllerWorldAim(native,base,center,centerPosition,pose,100,aim),"untracked hand is rejected");pose.valid=true;
    pose.position[1]=std::numeric_limits<float>::quiet_NaN();check(!diagnostic::ControllerWorldAim(native,base,center,centerPosition,pose,100,aim),"nonfinite pose cannot select or draw");
    diagnostic::ControllerFrame frame{};frame.focused=true;frame.timeMs=100;frame.hands[1].active=true;frame.hands[1].aim.valid=true;
    check(diagnostic::WorldAimReady(frame,150,true,false)&&!diagnostic::WorldAimReady(frame,201,true,false)&&!diagnostic::WorldAimReady(frame,99,true,false)&&!diagnostic::WorldAimReady(frame,150,true,true)&&!diagnostic::WorldAimReady(frame,150,false,false),"tracking age, clock order, menus and focus gate world targeting");
    diagnostic::WorldAim ray;ray.direction[2]=1;float rating=0;
    float nearMin[3]{-2,-2,50},nearMax[3]{2,2,54};
    check(diagnostic::WorldAimBox(ray,nearMin,nearMax,120,100,rating),"small ground item on the hand ray is selectable inside 1.2 metre reach");
    float farMin[3]{-5,-5,140},farMax[3]{5,5,150};
    check(!diagnostic::WorldAimBox(ray,farMin,farMax,120,100,rating),"desktop long distance focus is rejected even when directly on the hand ray");
    float npcMin[3]{-25,-10,110},npcMax[3]{25,160,150};
    check(diagnostic::WorldAimBox(ray,npcMin,npcMax,120,100,rating),"NPC near surface is reachable even when its centre is beyond reach");
    float contactMin[3]{-2,-2,-6},contactMax[3]{2,2,-2};
    check(diagnostic::WorldAimBox(ray,contactMin,contactMax,120,100,rating)&&rating<.05f,"target does not disappear when the controller touches or passes its centre");
    float sideMin[3]{30,-2,50},sideMax[3]{40,2,55};
    check(!diagnostic::WorldAimBox(ray,sideMin,sideMax,120,100,rating),"short reach does not select a nearby object well away from the hand direction");
    float insideMin[3]{-10,-10,-10},insideMax[3]{10,10,10};
    check(diagnostic::WorldAimBox(ray,insideMin,insideMax,120,100,rating)&&rating==0,"controller within object bounds keeps focus without a centre angle singularity");
    float invalidMin[3]{5,0,0},invalidMax[3]{-5,0,0};
    check(!diagnostic::WorldAimBox(ray,invalidMin,invalidMax,120,100,rating),"invalid object bounds cannot become a near-contact target");
    ray.origin[0]=87600;ray.origin[1]=5300;ray.origin[2]=-9500;ray.direction[0]=1;ray.direction[2]=0;
    float worldMin[3]{87650,5298,-9502},worldMax[3]{87654,5302,-9498};
    check(diagnostic::WorldAimBox(ray,worldMin,worldMax,120,100,rating),"surface reach works at the actual large Gothic coordinates and rotated controller headings");
    diagnostic::WorldAim standing;standing.origin[1]=110;standing.direction[1]=-.70710678f;standing.direction[2]=.70710678f;
    float swordMin[3]{-1,0,105},swordMax[3]{1,2,115};
    check(!diagnostic::WorldAimBox(standing,swordMin,swordMax,120,100,rating)&&diagnostic::WorldAimBox(standing,swordMin,swordMax,180,100,rating,true),"ground sword can be picked up from a standing hand without extending NPC reach");
    swordMin[0]=12;swordMax[0]=14;
    check(!diagnostic::WorldAimBox(standing,swordMin,swordMax,180,100,rating)&&diagnostic::WorldAimBox(standing,swordMin,swordMax,180,100,rating,true),"thin loot tolerates small pointing error that the 0.9.5 ray rejected");
    swordMin[0]=50;swordMax[0]=52;
    check(!diagnostic::WorldAimBox(standing,swordMin,swordMax,180,100,rating,true),"forgiving pickup ray does not select distant off-axis loot");
    float distantLootMin[3]{-2,-2,220},distantLootMax[3]{2,2,230};
    check(!diagnostic::WorldAimBox(standing,distantLootMin,distantLootMax,180,100,rating,true),"ground pickup retains a short finite surface reach");
    diagnostic::WorldAim calibrated;calibrated.origin[0]=25;calibrated.origin[1]=130;calibrated.origin[2]=50;calibrated.direction[2]=1;
    auto original=calibrated;
    check(diagnostic::WorldAimElevationBias(calibrated,15)&&close(calibrated.direction[1]/calibrated.direction[2],.15f)&&
        close(calibrated.origin[0],original.origin[0])&&close(calibrated.origin[1],original.origin[1])&&close(calibrated.origin[2],original.origin[2]),"selection is raised 15 cm per metre from a level hand without moving its tracked origin");
    float raisedTarget[3]{25,145,150};
    check(diagnostic::WorldAimAngles(calibrated,raisedTarget,azi,elev)&&close(azi,0)&&close(elev,0),"raised object uses the corrected interaction point for native focus angles");
    auto nativeSelection=diagnostic::NativeCameraRotation(calibrated.rotation);
    check(close(nativeSelection.m[2],calibrated.direction[0])&&close(nativeSelection.m[5],calibrated.direction[1])&&close(nativeSelection.m[8],calibrated.direction[2]),"native focus camera and candidate selection use the same raised direction");
    float rolled[3]{-.45f,.9f,1.2f};auto handRotation=diagnostic::NativeCameraRotation(rolled);
    diagnostic::WorldAim rolledAim;for(unsigned i=0;i<3;++i)rolledAim.direction[i]=handRotation.m[i*3+2];
    float oldElevation=std::atan2(rolledAim.direction[1],std::hypot(rolledAim.direction[0],rolledAim.direction[2]));
    float oldYaw=std::atan2(rolledAim.direction[0],rolledAim.direction[2]);diagnostic::WorldAimElevationBias(rolledAim,15);
    check(close(std::atan2(rolledAim.direction[1],std::hypot(rolledAim.direction[0],rolledAim.direction[2]))-oldElevation,std::atan(.15f))&&
        close(std::atan2(rolledAim.direction[0],rolledAim.direction[2]),oldYaw),"controller roll cannot turn the requested vertical adjustment into a lateral bias");
    calibrated=original;check(diagnostic::WorldAimElevationBias(calibrated,0)&&!memcmp(&calibrated,&original,sizeof(original)),"zero calibration leaves the unadjusted ray exactly intact");
    calibrated=original;diagnostic::WorldAimElevationBias(calibrated,-15);float lowerTarget[3]{25,115,150};
    check(diagnostic::WorldAimAngles(calibrated,lowerTarget,azi,elev)&&close(elev,0),"negative calibration selects below the unchanged hand without requiring downward wrist compensation");
    calibrated=original;diagnostic::WorldAimElevationBias(calibrated,200);
    check(close(calibrated.direction[1]/calibrated.direction[2],2.f),"the launcher extended range is used by the actual selection ray instead of retaining the old fifty-percent cap");
    calibrated.direction[1]=1;calibrated.direction[2]=0;diagnostic::WorldAimElevationBias(calibrated,15);
    check(calibrated.direction[1]>.99f&&calibrated.direction[2]>0,"nearly vertical upward selection does not flip backwards");
    printf("World aim: %u failures\n",failures);return failures?1:0;
}
