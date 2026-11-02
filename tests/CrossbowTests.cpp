#include "PhysicalCrossbow.h"
#include "SupportGripToggle.h"
#include "BowModel.h"
#include "NativeMotionDuration.h"
#include <fstream>
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
using Bytes=std::vector<unsigned char>;
void U(Bytes& b,unsigned v){auto p=reinterpret_cast<unsigned char*>(&v);b.insert(b.end(),p,p+4);}
void F(Bytes& b,float v){auto p=reinterpret_cast<unsigned char*>(&v);b.insert(b.end(),p,p+4);}
void S(Bytes& b,char const* s){U(b,unsigned(strlen(s)));b.insert(b.end(),s,s+strlen(s));}
void Chunk(Bytes& b,unsigned id,unsigned version,Bytes const& p){U(b,id);U(b,unsigned(p.size()));U(b,version);b.insert(b.end(),p.begin(),p.end());}
Bytes Actor(){Bytes b{'F','X','A',' ',1,1};char const* names[]{"Bone01","Bone02","Bone03","Bone04","Bone05","Mesh","Crossbow_ROOT"};
    for(unsigned i=0;i<7;++i){Bytes p;F(p,i==3?80.f:i==4?-80.f:0.f);F(p,0);F(p,i==6?0.f:-60.f);F(p,0);F(p,0);F(p,0);F(p,1);F(p,0);F(p,0);F(p,0);F(p,1);F(p,1);F(p,1);F(p,1);F(p,0);F(p,0);F(p,0);S(p,names[i]);S(p,i==6?"":"Crossbow_ROOT");Chunk(b,0,3,p);}
    Bytes mesh;for(unsigned v:{5,14,14,6,1,1,0,0,6,14})U(mesh,v);
    for(unsigned i=0;i<14;++i){U(mesh,i);F(mesh,i<10?float(int(i%3)-1)*.2f:i==10||i==13?-2.f:2.f);F(mesh,i<10?5.f:i<12?-3.f:3.f);F(mesh,i<10?-35.f:i<12?-50.f:10.f);for(unsigned j=0;j<5;++j)F(mesh,0);}
    for(unsigned i:{10,11,12,10,12,13})U(mesh,i);Chunk(b,3,3,mesh);
    Bytes skin;U(skin,5);for(unsigned i=0;i<14;++i){if(i<10){skin.push_back(2);U(skin,3);F(skin,.5f);U(skin,4);F(skin,.5f);}else{skin.push_back(1);U(skin,0);F(skin,1);}}Chunk(b,4,1,skin);return b;
}
Bytes Motion(){Bytes b{'L','M','A',' ',1,1,0};for(unsigned i=0;i<4;++i){char const* names[]{"Bone02","Bone03","Bone04","Bone05"};Bytes p(80);S(p,names[i]);Chunk(b,1,3,p);p.clear();U(p,2);U(p,0x524c);
    for(unsigned k=0;k<2;++k){F(p,float(k)*2);float a=k*(i==2?.15f:i==3?-.15f:0.f);F(p,0);F(p,std::sin(a));F(p,0);F(p,std::cos(a));}Chunk(b,2,1,p);}return b;}
Bytes Read(char const* path){std::ifstream f(path,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(f),{});}
int main(int argc,char** argv){
    bow::Sample s;s.player=1;s.weapon=2;s.session=3;s.valid=true;s.crossbow=true;s.aimHand=1;s.rightTracked=true;s.units=100;s.nativeNock=true;s.launchNock={300,200,100};s.launchDirection={0,0,1};
    crossbow::Trigger fire;crossbow::Reload reload;uint64_t time=1000;auto step=[&](bool trigger,bool allowed=true,unsigned dt=11){time+=dt;s.time=time;s.trigger=trigger;reload.Advance(dt*.001,false);auto shot=fire.Step(s,allowed,time,reload.Ready());if(shot.fire)reload.Commit(.5,1);return shot;};
    CHECK(!step(true).fire);CHECK(!step(false).fire);auto shot=step(true);CHECK(shot.fire&&shot.power==1&&arms::Length(shot.origin-s.launchNock)<.001f&&arms::Dot(shot.direction,s.launchDirection)>.999f);
    CHECK(!fire.Step(s,true,time,reload.Ready()).fire);for(int i=0;i<100;++i)CHECK(!step(true).fire);
    CHECK(!step(false).fire);CHECK(step(true).fire);CHECK(!step(false).fire);CHECK(!step(true).fire);
    // A press during reload is discarded, never queued when the cooldown ends.
    for(int i=0;i<60;++i)CHECK(!step(true).fire);CHECK(!step(false).fire);CHECK(step(true).fire);
    CHECK(!step(true,false).fire);CHECK(!step(true).fire);CHECK(!step(false).fire);for(int i=0;i<60;++i)CHECK(!step(false).fire);CHECK(step(true).fire);
    s.session++;CHECK(!step(true).fire);CHECK(!step(false).fire);s.trackingRight.x=2;CHECK(!step(true).fire);
    CHECK(!fire.Step(s,true,time+101,reload.Ready()).fire);s.crossbow=false;CHECK(!step(false).fire);s.crossbow=true;
    s.nativeNock=false;CHECK(!step(false).fire);CHECK(!step(true).fire);s.nativeNock=true;
    s.rightTracked=false;CHECK(!step(false).fire);CHECK(!step(true).fire);s.rightTracked=true;
    // Native animation duration, active tier speeds, pause and cancellation.
    double duration=0;CHECK(crossbow::MotionDuration(Motion(),duration)&&std::fabs(duration-2)<.00001);
    auto invalid=Motion();invalid.pop_back();CHECK(!crossbow::MotionDuration(invalid,duration));
    // Production hero motions mix LR and LP keys under the same chunk ID 2.
    auto mixed=Motion();Bytes node(80);S(node,"Root");Chunk(mixed,1,3,node);
    Bytes positions;U(positions,2);U(positions,0x504c);
    for(float t:{0.f,2.8f}){F(positions,t);F(positions,65.6f);F(positions,-48.7f);F(positions,1.3f);}Chunk(mixed,2,1,positions);
    CHECK(crossbow::MotionDuration(mixed,duration)&&std::fabs(duration-2.8)<.00001);
    char const* alias="Hero_Stand_CrossBow_Bolt_P0_Cock_Begin_O_Fwd_00_";
    CHECK(crossbow::MotionAliasMatch(alias,"Data/_compiledAnimation/Hero_Stand_CrossBow_Bolt_P0_Cock_Begin_O_Fwd_00_%_00_P0_0.xmot"));
    CHECK(!crossbow::MotionAliasMatch(alias,"Orc_Stand_CrossBow_Bolt_P0_Cock_Begin_O_Fwd_00_%_00_P0_0.xmot"));
    CHECK(!crossbow::MotionAliasMatch(alias,"Mike_CrossBow_Cock.xmot")&&!crossbow::MotionAliasMatch(alias,std::string(alias)+"different.xmot"));
    for(double speed:{1.,1.5,2.}){crossbow::Reload native;CHECK(native.Commit(1.8,speed));CHECK(std::fabs(native.Remaining()-1.8/speed)<.000001);
        native.Advance(10,true);CHECK(!native.Ready());native.Advance(1.8/speed-.01,false);CHECK(!native.Ready());native.Advance(.01,false);CHECK(native.Ready());}
    CHECK(!reload.Commit(1.8,0));CHECK(!reload.Commit(0,2));CHECK(reload.Commit(1.8,2));fire.Cancel();CHECK(!reload.Ready());
    CHECK(!crossbow::MotionDuration(Bytes{},duration));
    if(argc==2){CHECK(crossbow::MotionDuration(Read(argv[1]),duration));printf("native Cock duration %.6f seconds\n",duration);CHECK(duration>0&&duration<120);}
    // Right-handed aiming is independent of the left hand (support optional).
    ControllerFrame frame;frame.hands[1].grip.tracked=frame.hands[1].grip.valid=frame.hands[1].aim.tracked=frame.hands[1].aim.valid=true;frame.hands[1].grip.position[0]=.3f;frame.hands[1].aim=frame.hands[1].grip;
    float base[3]{},origin[3]{};int calibration[6]{},angles[3]{};arms::Mat grip,aim;
    CHECK(bow::TrackedAim(frame,Rotation3{},base,Orientation{},origin,100,0,calibration,angles,grip,aim,1));CHECK(!bow::TrackedAim(frame,Rotation3{},base,Orientation{},origin,100,0,calibration,angles,grip,aim,0));
    bow::Model model;auto actor=Actor(),motion=Motion();CHECK(model.Load(actor,motion,true));CHECK(model.maximumDraw>20&&model.maximumDraw<25);
    if(argc==3){bool loaded=model.Load(Read(argv[1]),Read(argv[2]),true);printf("crossbow asset loaded=%d refused=%u draw=%.3f\n",loaded,model.refusedAt,model.maximumDraw);CHECK(loaded);}
    arms::Vec nock,direction;CHECK(model.ArrowAt(model.TimeForDraw(model.maximumDraw),nock,direction));CHECK(direction.z==-1&&nock.z>model.nock.z+5);
    for(int pitch=-60;pitch<=60;pitch+=20){float a[3]{pitch*.0174533f,.3f,-.2f};auto pose=arms::Rotation(NativeCameraRotation(a));arms::Position(pose,{500,220,700});auto mesh=bow::AnchoredMesh(pose,model.grip,true);
        CHECK(arms::Length(arms::Point(mesh,model.grip)-arms::Position(pose))<.001f);CHECK(arms::Dot(arms::Vector(mesh,direction),arms::Vector(pose,{0,0,1}))>.999f);
        auto bolt=bow::NockedArrow(arms::Point(mesh,nock),arms::Vector(mesh,direction),arms::Vector(pose,{0,1,0}),-20);CHECK(arms::Length(arms::Point(bolt,{0,0,-20})-arms::Point(mesh,nock))<.001f);
        crossbow::Support support;auto hand=pose;arms::Position(hand,arms::Point(mesh,model.foregrip)+arms::Vector(hand,{0,0,8}));arms::Mat attached;arms::Vec wrist;
        CHECK(support.Resolve(2,true,mesh,model.foregrip,hand,arms::Mat{},arms::Vec{},100,attached,wrist));
        arms::Position(mesh,arms::Position(mesh)+arms::Vec{10,0,5});CHECK(support.Resolve(2,true,mesh,model.foregrip,hand,arms::Mat{},arms::Vec{},100,attached,wrist));CHECK(arms::Length(wrist-arms::Point(mesh,model.foregrip))<.001f);
        CHECK(!support.Resolve(2,false,mesh,model.foregrip,hand,arms::Mat{},arms::Vec{},100,attached,wrist));
    }
    // Actual toggle + palm attachment lifecycle, across both eye reads and
    // multiple releases of the physical grip. A far click cannot auto-attach.
    {
        SupportGripToggle toggle;crossbow::FrameSupport support;arms::Mat mesh,hand,attached;arms::Vec wrist;
        uint64_t t=20000;
        auto frame=[&](float squeeze){
            bool held=toggle.Step(2,1,t+=10,true,true,squeeze);support.Begin(2,held);
            bool contact=support.Prepare(2,held,mesh,{},hand,arms::Mat{},{},100);
            toggle.Confirm(support.Holding(2));return contact;
        };
        arms::Position(hand,{100,0,0});CHECK(!frame(0)&&!frame(.8f));
        arms::Position(hand,{});CHECK(!frame(.8f));CHECK(!frame(0));CHECK(frame(.8f));
        CHECK(frame(0));t+=3000;arms::Position(hand,{20,25,30});arms::Position(mesh,{10,20,30});CHECK(frame(0));
        CHECK(support.Read(attached,wrist));auto secondEye=attached;CHECK(support.Read(attached,wrist));
        for(int k=0;k<16;++k)CHECK(attached.m[k]==secondEye.m[k]);
        arms::Position(hand,{110,20,30});CHECK(!frame(0)&&!support.Holding(2)); // physically 1m away
        arms::Position(hand,{10,20,30});CHECK(!frame(0)); // does not reattach without a deliberate click
        CHECK(frame(.8f));CHECK(frame(0));CHECK(!frame(.8f)&&!support.Holding(2));
    }
    // The crossbow's independent limit reaches the real frame attachment,
    // including live changes while already attached and non-default world units.
    for(float units:{100.f,125.f})for(float maximum:{.10f,.40f,1.f}){
        crossbow::FrameSupport support;arms::Mat weapon,controller,pose;arms::Vec wrist;
        CHECK(support.Prepare(9,true,weapon,{},controller,arms::Mat{},{},units,maximum));
        arms::Position(controller,{(maximum-.001f)*units,0,0});
        CHECK(support.Prepare(9,true,weapon,{},controller,arms::Mat{},{},units,maximum));
        arms::Position(controller,{(maximum+.001f)*units,0,0});
        CHECK(!support.Prepare(9,true,weapon,{},controller,arms::Mat{},{},units,maximum)&&!support.Holding(9));
        arms::Position(controller,{});CHECK(support.Prepare(9,true,weapon,{},controller,arms::Mat{},{},units,1.f));
        arms::Position(controller,{.25f*units,0,0});CHECK(support.Prepare(9,true,weapon,{},controller,arms::Mat{},{},units,.40f));
        CHECK(!support.Prepare(9,true,weapon,{},controller,arms::Mat{},{},units,.20f));
    }
    CHECK(!model.Load(Bytes{},motion,true));puts("PASS crossbow native motion/skill reload, pause, press, tracking cancellation, right aim, native cocked string/bolt geometry and attached support");return 0;
}
