#include "TemporalObjectHistory.h"
#include <cstdio>
#include <limits>
#include <memory>
#define CHECK(x) do{if(!(x)){std::printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
using namespace diagnostic::temporal;
bool Near(float a,float b){return std::fabs(a-b)<.0001f;}
SkinPose Identity(){SkinPose p;for(unsigned b=0;b<64;++b)for(unsigned j=0;j<3;++j)p.bones[b*12+j*4+j]=1;for(unsigned j=0;j<4;++j)p.clip[j*4+j]=p.world[j*4+j]=1;return p;}
DrawKey Key(unsigned id=1){DrawKey k;k.id[0]=id;k.context=0x100;k.visual=0x200;k.vertices=0x300;k.indices=0x400;k.shader=0x8e4a823574960350ull;k.declaration=1;k.stride=64;k.draw={4,0,0,100,0,20};return k;}
int main(){
    CHECK(Layout(0x8e4a823574960350ull).projectionRegister==192&&Layout(0x1d21092550ea2dd4ull).projectionRegister==199&&
        Layout(0x7faf9fa857237e52ull).projectionRegister==199&&Layout(0x8879d99ef7dcd4f9ull).projectionRegister==192&&!Layout(0)&&!Layout(0x5bb23accc4adfbb1ull));
    auto p=Identity();Vertex vertex;vertex.position[0]=.2f;vertex.position[1]=.3f;float clip[4],motion[2];
    CHECK(p.Valid()&&Project(p,vertex,clip)&&Near(clip[0],.2f)&&Near(clip[1],.3f)&&Near(clip[3],1));
    CHECK(MotionPixels(p,p,vertex,1000,800,motion)&&Near(motion[0],0)&&Near(motion[1],0));
    auto moved=p;moved.bones[3]=.1f;moved.bones[7]=.2f;
    CHECK(MotionPixels(moved,p,vertex,1000,800,motion)&&Near(motion[0],-50)&&Near(motion[1],80));
    auto camera=p;camera.clip[3]=-.1f;
    CHECK(MotionPixels(camera,p,vertex,1000,800,motion)&&Near(motion[0],50));
    // Four influences, including affine translation in each three-register
    // bone. The last weight must participate; no implicit 1-sum shortcut.
    vertex.weights[0]=vertex.weights[1]=vertex.weights[2]=vertex.weights[3]=.25f;
    for(unsigned j=0;j<4;++j){vertex.indices[j]=j;moved.bones[j*12+3]=float(j)*.1f;}
    CHECK(MotionPixels(moved,p,vertex,1000,800,motion)&&Near(motion[0],-75));
    vertex.indices[3]=64;CHECK(!Project(p,vertex,clip));vertex.indices[3]=3;
    vertex.weights[3]=0;CHECK(!Project(p,vertex,clip));vertex=Vertex{};
    auto invalid=p;invalid.clip[15]=0;CHECK(!Project(invalid,vertex,clip));
    invalid=p;invalid.bones[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!invalid.Valid()&&!Project(invalid,vertex,clip));
    // Store on heap: fixed diagnostic storage should not consume the small
    // stack of a game render thread.
    auto history=std::make_unique<ObjectHistory>();auto key=Key();
    history->Begin(0,10);CHECK(history->Add(0,key,p)->previous==-1);
    history->Begin(1,10);auto right=p;right.clip[3]=.2f;CHECK(history->Add(1,key,right)->previous==-1);
    history->Begin(0,11);auto a=history->Add(0,key,moved);CHECK(a->previous==0&&a->reason==HistoryReason::Ready);
    CHECK(history->Previous(0).serial==10&&history->Previous(0).entries[0].pose.clip[3]==0);
    history->Begin(1,11);a=history->Add(1,key,right);CHECK(a->previous==0&&history->Previous(1).entries[0].pose.clip[3]==.2f);
    // Reordered submeshes and another character using identical buffers and
    // shaders cannot exchange history; identity is GUID+count, not draw order.
    history->Begin(0,12);auto another=Key(2);CHECK(history->Add(0,another,p)->previous==-1);
    a=history->Add(0,key,p);CHECK(a->previous==0);
    history->Begin(0,13);CHECK(history->Add(0,key,p)->previous==1&&history->Add(0,another,p)->previous==0);
    auto reused=key;++reused.id[4];CHECK(history->Add(0,reused,p)->previous==-1);
    // Repeated identical draws are harmless; conflicting palettes under the
    // same geometry identity are ambiguous, never silently overwritten.
    a=history->Add(0,key,p);CHECK(a->observations==2&&a->previous==1);
    a=history->Add(0,key,moved);CHECK(a->ambiguous&&a->previous==-1&&a->reason==HistoryReason::Ambiguous);
    history->Begin(0,14);CHECK(history->Add(0,key,p)->reason==HistoryReason::Ambiguous);
    history->Begin(0,15);CHECK(history->Add(0,key,p)->reason==HistoryReason::Ready);
    history->Begin(0,17);CHECK(history->Add(0,key,p)->reason==HistoryReason::Gap);
    history->Begin(0,18,2);CHECK(history->Add(0,key,p)->reason==HistoryReason::Gap);
    history->Begin(0,19,2);auto far=p;far.world[12]=2000;CHECK(history->Add(0,key,far)->reason==HistoryReason::Teleport);
    history->Begin(0,20,2);history->Add(0,key,far);CHECK(!history->Add(0,key,invalid)&&history->Current(0).entries[0].ambiguous);
    history->Reset();history->Begin(0,21);CHECK(history->Add(0,key,p)->previous==-1);
    for(unsigned i=2;i<=ObjectHistory::Limit;++i)CHECK(history->Add(0,Key(i),p));
    CHECK(!history->Add(0,Key(999),p)&&history->Current(0).dropped==1);
    history->Begin(0,21);CHECK(history->Current(0).count==ObjectHistory::Limit);
    // An object missed in frame 22 has no valid animation correspondence in 23.
    history->Begin(0,22);history->Begin(0,23);CHECK(history->Add(0,key,p)->previous==-1);
    // The same mesh drawn into another full-size surface or color/depth pass
    // is a different correspondence, even if its shader/palette are identical.
    history->Reset();key.target=10;key.depth=20;key.pass={1,4,3,0,7};history->Begin(0,30);history->Add(0,key,p);
    history->Begin(0,31);auto different=key;different.target=11;CHECK(history->Add(0,different,p)->previous==-1);
    different=key;different.pass[0]=0;CHECK(history->Add(0,different,p)->previous==-1);
    CHECK(history->Add(0,key,p)->previous==0);
    std::puts("PASS actual SM3 four-bone affine/projection math; NGX motion sign; separate eye histories; GUID/count and geometry identity; reordered draws, ambiguity, gaps, reset, teleport, missing objects and bounded capacity");
    return 0;
}
