#include "BowModel.h"
#include "PhysicalBow.h"
#include "TrackedPalette.h"
#include "BowAimFilter.h"
#include <fstream>
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
using Bytes=std::vector<unsigned char>;
void U(Bytes& b,unsigned v){auto* p=reinterpret_cast<unsigned char*>(&v);b.insert(b.end(),p,p+4);}
void F(Bytes& b,float v){auto* p=reinterpret_cast<unsigned char*>(&v);b.insert(b.end(),p,p+4);}
void S(Bytes& b,char const* s){U(b,unsigned(strlen(s)));b.insert(b.end(),s,s+strlen(s));}
void Chunk(Bytes& b,unsigned id,unsigned version,Bytes const& p){U(b,id);U(b,unsigned(p.size()));U(b,version);b.insert(b.end(),p.begin(),p.end());}
Bytes Actor(){Bytes b{'F','X','A',' ',1,1};
    char const* names[]{"Bone01","Bone02","Bone03","Bone05","Bone06","Mesh","Bow_ROOT"};
    for(unsigned i=0;i<7;++i){Bytes p;F(p,0);F(p,0);F(p,i==0?-6.f:i==2?80.f:i==4?-80.f:0.f);F(p,0);F(p,0);F(p,0);F(p,1);F(p,0);F(p,0);F(p,0);F(p,1);F(p,1);F(p,1);F(p,1);F(p,0);F(p,0);F(p,0);S(p,names[i]);S(p,i==6?"":"Bow_ROOT");Chunk(b,0,3,p);}
    Bytes mesh;for(unsigned v:{5,8,8,6,1,1,0,0,6,8})U(mesh,v);
    for(unsigned i=0;i<8;++i){U(mesh,i);F(mesh,i<4?-25+(i==0?.3f:i==1?-.3f:0.f):i==4||i==7?0.f:2.f);F(mesh,i<4?(i==2?.3f:i==3?-.3f:0.f):0);F(mesh,i<4?0:i<6?-10.f:10.f);for(unsigned j=0;j<5;++j)F(mesh,0);}
    for(unsigned i:{4,5,6,4,6,7})U(mesh,i);Chunk(b,3,3,mesh);
    Bytes skin;U(skin,5);for(unsigned i=0;i<8;++i){if(i<4){skin.push_back(2);U(skin,2);F(skin,.5f);U(skin,4);F(skin,.5f);}else{skin.push_back(1);U(skin,0);F(skin,1);}}Chunk(b,4,1,skin);return b;
}
Bytes Motion(){Bytes b{'L','M','A',' ',1,1,0};for(unsigned i=0;i<4;++i){char const* names[]{"Bone02","Bone03","Bone05","Bone06"};Bytes p(80);S(p,names[i]);Chunk(b,1,3,p);p.clear();U(p,2);U(p,0x524c);
    for(unsigned k=0;k<2;++k){F(p,float(k)*2);float a=k*(i==1?.15f:i==3?-.15f:0.f);F(p,0);F(p,std::sin(a));F(p,0);F(p,std::cos(a));}Chunk(b,2,1,p);}return b;}
Bytes Read(char const* path){std::ifstream f(path,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(f),{});}
bool LeftRest(bow::Model const& model){
    if(!model.valid||model.arrowRest.y>=model.arrowSupport.y)return false;
    for(unsigned i=0;i<=10;++i){arms::Vec nock,direction;if(!model.ArrowAt(model.TimeForDraw(model.maximumDraw*i/10),nock,direction))return false;
        if(direction.x<=0)return false;
        auto rest=nock+direction*((model.arrowRest.x-nock.x)/direction.x);
        auto middle=nock+direction*((model.arrowSupport.x-nock.x)/direction.x);
        if(arms::Length(rest-model.arrowRest)>.002f||middle.y>model.arrowRest.y+.002f)return false;
    }return true;
}
int main(int argc,char** argv){bow::Model model;auto actor=Actor(),motion=Motion();CHECK(model.Load(actor,motion));CHECK(model.maximumDraw>20&&model.maximumDraw<25);
    CHECK(LeftRest(model)&&std::fabs(model.arrowRest.y+.6f)<.001f&&std::fabs(model.arrowRest.x)<.001f);
    CHECK(std::fabs(model.grip.z+6)<.001f&&std::fabs(model.arrowSupport.x-1)<.001f&&std::fabs(model.arrowSupport.z)<.001f);
    CHECK(model.nock.z-model.grip.z>5&&arms::Length(model.arrowSupport-model.grip)>5);
    arms::Vec half,full;CHECK(model.NockAt(model.TimeForDraw(10),half)&&std::fabs(model.nock.x-half.x-10)<.002f);
    CHECK(model.NockAt(model.TimeForDraw(10000),full)&&std::fabs(model.nock.x-full.x-model.maximumDraw)<.002f);
    for(int side=0;side<2;++side)for(int pitch=-80;pitch<=80;pitch+=20){int values[6]{10,12,-9,pitch,25,-30};arms::Mat grip;arms::Position(grip,{40000,5000,-9000});CHECK(arms::CalibrateGrip(grip,side,values,100));
        // Actual rig landmarks, mirrored sides and nonidentity hand bind. The
        // expected palm comes from the displayed hand's bone transform, not
        // from the old controller +Z offset (across the knuckle plane).
        float sign=side?1.f:-1.f,bindAngles[3]{.3f,-.5f,.2f};arms::Mat bindHand=arms::Rotation(NativeCameraRotation(bindAngles));
        arms::Position(bindHand,{sign*56.6f,120.9f,-.6f});arms::Vec index{sign*62.4f,113.8f,4.3f},pinky{sign*60.2f,112.f,-3.4f},local;arms::Mat calibration;
        CHECK(arms::GripCalibration(bindHand,index,pinky,calibration)&&bow::PalmLocal(bindHand,index,pinky,local));
        CHECK(arms::Length(arms::Point(bindHand,local)-(arms::Position(bindHand)+(index+pinky)*.5f)*.5f)<.001f);
        arms::Vec wrist{40010,5100,-9030};auto displayed=arms::Multiply(calibration,grip);arms::Position(displayed,wrist);
        auto resolved=bow::ResolvedPalm(grip,calibration,wrist,local);auto pose=bow::AnchoredMesh(resolved,model.grip);CHECK(arms::Length(arms::Point(pose,model.grip)-arms::Point(displayed,local))<.01f);
        CHECK(arms::Length(arms::Position(resolved)-wrist)>4&&arms::Length(arms::Position(resolved)-wrist)<5);
        int offsets[3]{5,2,-3};auto contact=bow::ResolvedBowGrip(grip,calibration,wrist,local,offsets,100);
        CHECK(arms::Length(arms::Position(contact)-arms::Position(resolved)-arms::Vector(grip,{2,-5,-3}))<.01f);
        auto held=bow::AnchoredMesh(contact,model.grip);CHECK(arms::Length(arms::Point(held,model.grip)-arms::Position(contact))<.01f);
        CHECK(arms::Dot(arms::Point(held,model.nock)-arms::Position(contact),arms::Vector(contact,{0,1,0}))>5.9f);
        arms::Vec nock=arms::Point(pose,half),dir=arms::Vector(resolved,{0,0,1});auto arrow=bow::NockedArrow(nock,dir,arms::Vector(resolved,{0,1,0}),-38);
        CHECK(arms::Length(arms::Point(arrow,{0,0,-38})-nock)<.01f);
        // Left-side support is model-relative, including rolled/calibrated
        // hands and full string travel. The tail stays on the real string.
        for(float draw:{0.f,10.f,model.maximumDraw}){arms::Vec drawn,shaft;CHECK(model.ArrowAt(model.TimeForDraw(draw),drawn,shaft));
            auto worldNock=arms::Point(held,drawn),worldRest=arms::Point(held,model.arrowRest),worldDirection=arms::Vector(held,shaft);
            auto sideArrow=bow::NockedArrow(worldNock,worldDirection,arms::Vector(contact,{0,1,0}),-38);
            CHECK(arms::Length(arms::Point(sideArrow,{0,0,-38})-worldNock)<.01f);
            CHECK(arms::Length(arms::Cross(worldRest-worldNock,arms::Vector(sideArrow,{0,0,1})))<.02f);
            CHECK(arms::Dot(worldRest-arms::Point(held,model.arrowSupport),arms::Vector(contact,{1,0,0}))<-.59f);
        }
        arms::Mat palette;CHECK(model.Palette("Bone03",model.TimeForDraw(10),pose,palette));TrackedPalette saved;arms::Mat data[1]{grip},before=data[0];
        for(unsigned eye=0;eye<2;++eye){CHECK(saved.Begin(data,1));data[0]=palette;CHECK(saved.Restore(data,1)&&!memcmp(data,&before,sizeof(before)));}
    }
    arms::Vec badPalm;CHECK(!bow::PalmLocal(arms::Mat{},{0,0,0},{0,0,0},badPalm));
    for(size_t n=0;n<actor.size();++n){auto cut=actor;cut.resize(n);bow::Model bad;CHECK(!bad.Load(cut,motion));}
    auto bad=motion;bad[7+4]=255;CHECK(!model.Load(actor,bad));
    bow::AimFilter filter;ControllerPose aim;aim.valid=aim.tracked=true;float rawError=0,filteredError=0;
    for(unsigned i=0;i<100;++i){float a=(i%2?.4f:-.4f)*.00872664626f;aim.quaternion[0]=aim.quaternion[2]=0;aim.quaternion[1]=std::sin(a);aim.quaternion[3]=std::cos(a);rawError+=std::fabs(a);
        CHECK(filter.Apply(aim,1000+i*11,1,2,80));filteredError+=std::fabs(std::asin(aim.quaternion[1]));}
    CHECK(filteredError<rawError*.3f);
    float large=25*.00872664626f;aim.quaternion[1]=std::sin(large);aim.quaternion[3]=std::cos(large);
    CHECK(filter.Apply(aim,2100,1,2,80)&&std::asin(aim.quaternion[1])>large*.5f);
    aim.quaternion[1]=0;aim.quaternion[3]=1;CHECK(filter.Apply(aim,2111,2,2,80)&&std::fabs(aim.quaternion[1])<.0001f);
    aim.quaternion[1]=std::sin(large);aim.quaternion[3]=std::cos(large);CHECK(filter.Apply(aim,2122,2,2,0)&&std::fabs(std::asin(aim.quaternion[1])-large)<.0001f);
    aim.tracked=false;CHECK(!filter.Apply(aim,2133,2,2,80));
    if(argc==3){bool loaded=model.Load(Read(argv[1]),Read(argv[2]));if(!loaded)printf("refused at %u\n",model.refusedAt);CHECK(loaded&&LeftRest(model));printf("native model grip %.3f %.3f %.3f nock %.3f %.3f %.3f left rest %.3f %.3f %.3f maximum draw %.3f\n",model.grip.x,model.grip.y,model.grip.z,model.nock.x,model.nock.y,model.nock.z,model.arrowRest.x,model.arrowRest.y,model.arrowRest.z,model.maximumDraw);}
    puts("PASS native motion, string centre, inverse draw, calibrated palm, arrow nock, stereo restore and truncated input rejection");return 0;
}
