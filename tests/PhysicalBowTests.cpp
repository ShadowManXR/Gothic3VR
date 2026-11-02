#include "PhysicalBow.h"
#include "WorldReticle.h"
#include "FrozenArmPalette.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
bow::Sample Pose(uint64_t time,float distance,bool trigger){
    bow::Sample s;s.player=1;s.weapon=2;s.session=3;s.time=time;s.valid=true;s.rightTracked=true;s.trigger=trigger;
    arms::Position(s.left,{0,140,100});arms::Position(s.right,{0,140,100-distance*100});
    s.trackingLeft={0,0,-1};s.trackingRight={0,0,-1+distance};return s;
}
bow::Shot Pull(bow::Draw& draw,uint64_t& time,float total){
    draw.Step(Pose(time,.05f,false),true,time);time+=10;
    draw.Step(Pose(time,.05f,true),true,time);time+=10;
    for(int i=1;i<=30;++i){float distance=.05f+total*float(i)/30;draw.Step(Pose(time,distance,true),true,time);time+=10;}
    auto s=Pose(time,.05f+total,false);auto shot=draw.Step(s,true,time);time+=400;return shot;
}
int main(){
    {
        auto sample=Pose(1000,.7f,true);sample.nativeNock=true;
        arms::Vec contact,thumbEnd,indexEnd;float angles[3]{.2f,-.3f,.4f};auto calibration=arms::Rotation(NativeCameraRotation(angles));
        arms::Mat middle,tip;arms::Position(middle,{0,-2,0});arms::Position(tip,{0,-4,0});
        CHECK(bow::StringFingerEnd(middle,tip,thumbEnd)&&std::fabs(thumbEnd.y+1.4f)<.0001f);
        arms::Position(tip,{0,-5,0});CHECK(bow::StringFingerEnd(middle,tip,indexEnd));
        CHECK(!bow::StringFingerEnd(middle,middle,contact));
        FrozenArmPalette frozen;
        for(unsigned frame=0;frame<180;++frame){
            sample.time=1000+frame*10;sample.launchNock={10+std::sin(frame*.05f)*20,145,90-frame*.15f};
            float wristAngles[3]{frame*.011f,frame*.017f,frame*.013f};auto controller=arms::Rotation(NativeCameraRotation(wristAngles));
            arms::Position(controller,{70,180,0});sample.right=controller;auto original=sample.right;
            // Native wrist and curled fingers differ from the open bind palm.
            // Resolve the posed endpoints, not a fixed world/grip offset.
            auto nativeHand=arms::Rotation(NativeCameraRotation(angles));arms::Position(nativeHand,{120,150,160});
            float curl[3]{.5f+frame*.003f,.2f,.1f};auto thumb=arms::Rotation(NativeCameraRotation(curl));
            arms::Position(thumb,{-2,-5,1});auto index=thumb;arms::Position(index,{2,-7,2});
            thumb=arms::Multiply(thumb,nativeHand);index=arms::Multiply(index,nativeHand);
            CHECK(bow::StringFingerContact(nativeHand,thumb,thumbEnd,index,indexEnd,contact));
            arms::Mat attached;arms::Vec wrist;
            CHECK(bow::StringHandGrip(sample,true,true,controller,calibration,contact,attached,wrist));
            auto hand=arms::Multiply(calibration,attached);arms::Position(hand,wrist);
            CHECK(arms::Length(arms::Point(hand,contact)-sample.launchNock)<.001f);
            CHECK(arms::Length(wrist-sample.launchNock)>5); // the wrist itself must not touch the arrow
            arms::Mat inverse;CHECK(arms::Inverse(nativeHand,inverse));auto delta=arms::Multiply(inverse,hand);
            arms::Mat posed[3]{hand,arms::Multiply(thumb,delta),arms::Multiply(index,delta)};
            CHECK(arms::Length((arms::Point(posed[1],thumbEnd)+arms::Point(posed[2],indexEnd))*.5f-sample.launchNock)<.001f);
            CHECK(!memcmp(&sample.right,&original,sizeof(original))); // physical draw/charge keeps the raw controller
            frozen.Reset();CHECK(frozen.Capture(1,2,posed,3));
            for(unsigned eye=0;eye<2;++eye){arms::Mat rendered[3];CHECK(frozen.Apply(1,2,rendered,3));
                CHECK(arms::Length((arms::Point(rendered[1],thumbEnd)+arms::Point(rendered[2],indexEnd))*.5f-sample.launchNock)<.001f);}
        }
        arms::Mat attached;arms::Vec wrist;
        CHECK(!bow::StringHandGrip(sample,false,true,sample.right,calibration,contact,attached,wrist));
        CHECK(!bow::StringHandGrip(sample,true,false,sample.right,calibration,contact,attached,wrist));
        sample.trigger=false;CHECK(!bow::StringHandGrip(sample,true,true,sample.right,calibration,contact,attached,wrist));sample.trigger=true;
        sample.rightTracked=false;CHECK(!bow::StringHandGrip(sample,true,true,sample.right,calibration,contact,attached,wrist));sample.rightTracked=true;
        sample.crossbow=true;CHECK(!bow::StringHandGrip(sample,true,true,sample.right,calibration,contact,attached,wrist));sample.crossbow=false;
        sample.nativeNock=false;CHECK(!bow::StringHandGrip(sample,true,true,sample.right,calibration,contact,attached,wrist));
    }
    bow::Draw draw;uint64_t time=1000;
    auto weak=Pull(draw,time,.15f),strong=Pull(draw,time,.5f);
    CHECK(weak.fire&&strong.fire&&weak.power<strong.power&&strong.power==1);
    CHECK(std::fabs(weak.direction.z-1)<.001f&&std::fabs(strong.origin.z-100)<.001f);
    CHECK(!draw.Step(Pose(time,.55f,false),true,time).fire);time+=10;
    // Grabbing is centred on the actual string, not the bow grip. Lateral
    // separation alone cannot charge a shot.
    draw.Cancel();auto stringPose=Pose(time,.27f,false);stringPose.nockLocal={0,13,-27};arms::Position(stringPose.right,bow::StringRest(stringPose));
    draw.Step(stringPose,true,time);time+=10;stringPose.time=time;stringPose.trigger=true;
    draw.Step(stringPose,true,time,.5f,.05f);CHECK(draw.pulling);time+=10;
    stringPose.time=time;arms::Position(stringPose.right,bow::StringRest(stringPose)+arms::Vec{30,0,0});draw.Step(stringPose,true,time);CHECK(draw.power==0);time+=10;
    stringPose.time=time;stringPose.trigger=false;CHECK(!draw.Step(stringPose,true,time).fire);time+=400;
    draw.Cancel();stringPose=Pose(time,0,false);stringPose.nockLocal={0,13,-27};arms::Position(stringPose.right,arms::Position(stringPose.left));draw.Step(stringPose,true,time);time+=10;
    stringPose.time=time;stringPose.trigger=true;draw.Step(stringPose,true,time,.5f,.05f);CHECK(!draw.pulling);time+=400;
    CHECK(!Pull(draw,time,.07f).fire);
    // The visible shaft, sight and projectile use the left-side arrow rest.
    // That small shaft angle must not turn lateral string motion into power.
    draw.Cancel();arms::Vec shaft{-.08f,0,1};CHECK(arms::Unit(shaft));bool geometryConsistent=true;
    auto geometricStep=[&](float travel,float lateral,bool pressed){auto s=Pose(time,.05f+travel,pressed);
        arms::Position(s.right,arms::Position(s.right)+arms::Vec{lateral,0,0});s.nativeNock=true;s.launchNock=bow::StringRest(s)-arms::Vec{0,0,travel*100};s.launchDirection=shaft;
        arms::Vec origin,direction;float distance;geometryConsistent=geometryConsistent&&bow::Aim(s,origin,direction,distance)&&
            arms::Length(origin-s.launchNock)<.001f&&arms::Length(direction-shaft)<.001f;
        auto shot=draw.Step(s,true,time);time+=10;return shot;};
    geometricStep(0,0,false);geometricStep(0,0,true);
    for(int i=0;i<12;++i)geometricStep(0,25,true);
    CHECK(draw.pulling&&draw.power==0);
    for(int i=1;i<=30;++i)geometricStep(.5f*i/30,0,true);
    auto geometricShot=geometricStep(.5f,0,false);
    CHECK(geometryConsistent&&geometricShot.fire&&geometricShot.power==1&&arms::Length(geometricShot.direction-shaft)<.001f&&std::fabs(geometricShot.origin.z-50)<.001f);time+=400;
    // Held trigger on equip is not a draw, and a press far from string is ignored.
    draw.Cancel();draw.Step(Pose(time,.05f,true),true,time);CHECK(!draw.pulling);time+=10;
    draw.Step(Pose(time,.7f,false),true,time);time+=10;draw.Step(Pose(time,.7f,true),true,time);CHECK(!draw.pulling);time+=10;
    // Loss of tracking / menus / origin changes cancel rather than fire on release.
    for(int reason=0;reason<6;++reason){
        draw.Cancel();draw.Step(Pose(time,.05f,false),true,time);time+=10;
        draw.Step(Pose(time,.05f,true),true,time);CHECK(draw.pulling);time+=10;
        auto s=Pose(time,.25f,true);draw.Step(s,true,time);time+=10;
        s=Pose(time,.4f,true);if(reason==0)s.valid=false;if(reason==1)s.session=7;if(reason==2)s.weapon=9;if(reason==3)s.trackingRight.z+=1;if(reason==5)s.rightTracked=false;
        CHECK(!draw.Step(s,reason!=4,time).fire&&!draw.pulling);time+=10;
        CHECK(!draw.Step(Pose(time,.55f,false),true,time).fire);time+=10;
    }
    // Very short trigger timing and stale release cannot shoot.
    draw.Cancel();draw.Step(Pose(time,.05f,false),true,time);time+=10;
    draw.Step(Pose(time,.05f,true),true,time);time+=10;draw.Step(Pose(time,.25f,true),true,time);time+=10;
    CHECK(!draw.Step(Pose(time,.25f,false),true,time).fire);time+=10;
    draw.Cancel();draw.Step(Pose(time,0,false),true,time);time+=10;
    draw.Step(Pose(time,0,true),true,time);CHECK(draw.pulling);time+=10;
    auto stale=Pose(time,.3f,true);CHECK(!draw.Step(stale,true,time+101).fire&&!draw.pulling);time+=400;
    // Different world origins and rotations do not change draw power.
    for(arms::Vec dir: {arms::Vec{0,1,0},arms::Vec{1,0,0},arms::Vec{0,0,-1}}){
        draw.Cancel();auto step=[&](float d,bool pressed){auto s=Pose(time,d,pressed);arms::Position(s.left,{40000,5000,-8000});arms::Position(s.right,arms::Position(s.left)-dir*(d*100));s.leftAim=bow::ArrowPose(arms::Position(s.left),dir,std::fabs(dir.y)>.9f?arms::Vec{1,0,0}:arms::Vec{0,1,0});auto shot=draw.Step(s,true,time);time+=10;return shot;};
        step(.05f,false);step(.05f,true);for(int i=1;i<=30;++i)step(.05f+.5f*i/30,true);auto shot=step(.55f,false);
        CHECK(shot.fire&&arms::Length(shot.direction-dir)<.001f&&std::fabs(shot.power-1)<.001f);time+=400;
        auto pose=bow::ArrowPose(shot.origin,shot.direction,std::fabs(dir.y)>.9f?arms::Vec{1,0,0}:arms::Vec{0,1,0});
        CHECK(arms::Affine(pose)&&arms::Length(arms::Vector(pose,{0,0,1})-dir)<.001f);
    }
    auto aimSample=Pose(time,.5f,true);arms::Position(aimSample.right,{-40,170,90});arms::Vec origin,dir;float distance;
    CHECK(bow::Aim(aimSample,origin,dir,distance)&&arms::Length(dir-arms::Vec{0,0,1})<.001f);
    aimSample.rightTracked=false;CHECK(bow::Aim(aimSample,origin,dir,distance));
    aimSample.rightTracked=true;arms::Position(aimSample.right,{0,140,-200});CHECK(bow::Aim(aimSample,origin,dir,distance)&&distance>1.5f);
    // Moving the string hand off-axis does not steer the left-hand sight.
    arms::Position(aimSample.right,{40,110,80});CHECK(bow::Aim(aimSample,origin,dir,distance)&&arms::Length(dir-arms::Vec{0,0,1})<.001f);
    arms::Mat moved=bow::ArrowPose({40000,5500,-9000},{1,0,0},{0,1,0}),anchored;
    arms::Vec minimum{-3,-75,-10},maximum{5,85,30},grip{40100,5140,-8000};
    CHECK(bow::AnchorMesh(minimum,maximum,moved,grip,anchored)&&arms::Length(arms::Point(anchored,(minimum+maximum)*.5f)-grip)<.001f);
    CHECK(arms::Length(arms::Vector(anchored,{0,0,1})-arms::Vec{1,0,0})<.001f);
    CHECK(!bow::AnchorMesh(maximum,minimum,moved,grip,anchored));
    auto mesh=bow::MeshPose(moved);CHECK(arms::Affine(mesh)&&arms::Length(arms::Position(mesh)-arms::Position(moved))<.001f);
    CHECK(arms::Length(arms::Vector(mesh,{1,0,0})-arms::Vector(moved,{0,0,1}))<.001f);
    CHECK(arms::Length(arms::Vector(mesh,{0,0,1})-arms::Vector(moved,{0,1,0}))<.001f);
    float projection[16],view[16],fov[4]{-.7f,.7f,.7f,-.7f};StereoProjection(fov,1,10000,projection);arms::Mat identity;memcpy(view,identity.m,sizeof(view));float u,v;
    CHECK(ProjectReticle({0,0,500},view,projection,u,v)&&std::fabs(u-.5f)<.001f&&std::fabs(v-.5f)<.001f);
    CHECK(ProjectReticle({50,50,500},view,projection,u,v)&&u>.5f&&v<.5f);
    float leftU,rightU;view[12]=3.2f;CHECK(ProjectReticle({0,0,500},view,projection,leftU,v));view[12]=-3.2f;CHECK(ProjectReticle({0,0,500},view,projection,rightU,v)&&leftU>rightU);
    CHECK(!ProjectReticle({0,0,-500},view,projection,u,v)&&!ProjectReticle({10000,0,500},view,projection,u,v));
    ControllerFrame frame;frame.hands[0].grip.valid=frame.hands[0].grip.tracked=frame.hands[0].aim.valid=frame.hands[0].aim.tracked=true;
    frame.hands[0].grip.position[0]=-.3f;frame.hands[0].grip.position[1]=-.4f;frame.hands[0].aim.position[0]=-.3f;
    int calibration[6]{3,2,1,20,0,0},angles[3]{0,15,0};float base[3]{40000,5000,-8000},trackingOrigin[3]{};arms::Mat aimGrip,aimRay;
    CHECK(bow::TrackedAim(frame,NativeCameraRotation(trackingOrigin),base,{1,0,0,0},trackingOrigin,100,-25,calibration,angles,aimGrip,aimRay));
    CHECK(arms::Length(arms::Position(aimRay)-arms::Position(aimGrip))<.001f);
    aimSample.left=aimGrip;aimSample.leftAim=aimRay;arms::Position(aimSample.right,arms::Position(aimGrip)-arms::Vec{0,0,50});
    CHECK(bow::Aim(aimSample,origin,dir,distance)&&dir.x>.2f&&dir.z>.9f);
    frame.hands[0].aim.tracked=false;CHECK(!bow::TrackedAim(frame,NativeCameraRotation(trackingOrigin),base,{1,0,0,0},trackingOrigin,100,-25,calibration,angles,aimGrip,aimRay));
    puts("PASS bow grab/draw/release, power, left-hand sight, mesh grip, duplicate/stale/equip/menu/reset/tracking cancellation");return 0;
}
