#include "SwingVelocity.h"
#include "CombatGestures.h"
#include "ControllerControls.h"
#include "ControllerInput.h"
#include "WorldStatusImage.h"
#include "StereoCameraState.h"
#include "ActivityCamera.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
void SaveImage(wchar_t const* path,int status){auto image=WorldStatusImage(status);auto layout=WorldStatusVisual(status);
    BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+layout.width*layout.height*4;
    BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=layout.width;info.biHeight=-int(layout.height);info.biPlanes=1;info.biBitCount=32;
    FILE* output=nullptr;_wfopen_s(&output,path,L"wb");if(!output)return;fwrite(&file,sizeof(file),1,output);fwrite(&info,sizeof(info),1,output);
    for(unsigned y=0;y<layout.height;++y)fwrite(image.data()+y*RadialPixels,4,layout.width,output);fclose(output);
}
int wmain(int argc,wchar_t** argv){
    combat::SwingVelocity velocity;combat::StrokeBudget budget;uint64_t time=1000;int spent=0;
    auto blade=[](float x){return melee::Blade{{x,0,0},{x,0,1}};};
    // Existing distance could qualify a long slow sweep. Speed must qualify
    // the same meaningful path before any damage query or native debit.
    for(int i=0;i<=150;++i,time+=20){velocity.Step(blade(i*.01f),time,1,true,100);bool ready=velocity.Ready(50);
        CHECK(!ready);budget.Decide(1,false,ready,ready,10,20,100,false,[&](int cost){spent+=cost;return true;});}
    CHECK(spent==0&&velocity.Speed()<1); //1.5m at0.5m/s
    velocity.Step(blade(1.56f),time,1,true,100);CHECK(!velocity.Ready(50));time+=20; // tiny fast flick after the slow sweep
    velocity.Reset();budget.Reset();
    for(int i=0;i<=40;++i,time+=20){velocity.Step(blade(i*.03f),time,2,true,100);bool ready=velocity.Ready(50);
        budget.Decide(2,false,ready,ready,10,20,100,false,[&](int cost){spent+=cost;return true;});}
    CHECK(velocity.Ready(50)&&spent==10&&velocity.Speed()>1.4f); //1.5m/s, once per stroke
    velocity.Step(blade(1.20f),time-20,2,false,100);CHECK(velocity.Ready(50)); // duplicate XR render does not erase ledger
    velocity.Step(blade(1.21f),time,2,true,100);CHECK(!velocity.Ready(50));time+=20; // slow contact after fast stroke cannot damage
    velocity.Reset();for(int i=0;i<120;++i,time+=10){velocity.Step(blade((i%2)*.02f),time,3,true,100);CHECK(!velocity.Ready(50));}
    velocity.Reset();for(int i=0;i<=50;++i,time+=10){float a=i*.02f;velocity.Step({{0,0,0},{std::sin(a),0,std::cos(a)}},time,4,true,100);}CHECK(velocity.Ready(50));
    time+=101;velocity.Step(blade(2),time,4,true,100);CHECK(!velocity.Ready(50));
    puts("PASS slow travel, slow-then-flick, fast stroke debit, stopped contact, tremor, angular endpoint speed, duplicate frames and tracking gap");
    NativeStickDirections steering;float x,z;
    for(float side:{-.4f,-.1f,0.f,.1f,.4f}){steering.Reset();CHECK(steering.Step(side,1,x,z)&&x==0&&z==1);}
    CHECK(steering.Step(.8f,1,x,z)&&x>.1f&&z>.1f);CHECK(steering.Step(.6f,1,x,z)&&x>.1f);
    CHECK(steering.Step(.4f,1,x,z)&&x==0);CHECK(steering.Step(.6f,1,x,z)&&x==0);
    CHECK(steering.Step(-1,0,x,z)&&x==-1&&z==0);steering.Reset();CHECK(steering.Step(.2f,-1,x,z)&&x==0&&z==-1);
    CHECK(!steering.Step(.1f,.1f,x,z)&&x==0&&z==0);
    // Actual11760 capture: right .84,-.13 and left -.95,-.30 wrongly
    // generated a full Backward action from a small off-axis component.
    for(float sign:{-1.f,1.f})for(float drift:{-.4f,-.3f,-.13f,0.f,.13f,.3f,.4f}){
        steering.Reset();CHECK(steering.Step(sign,drift,x,z)&&x==sign&&z==0);
    }
    steering.Reset();CHECK(steering.Step(.84f,-.13f,x,z)&&x==1&&z==0);
    CHECK(steering.Step(-.95f,-.30f,x,z)&&x==-1&&z==0);
    CHECK(steering.Step(1,-.8f,x,z)&&x>.1f&&z<-.1f); // deliberate backward diagonal
    CHECK(steering.Step(1,-.6f,x,z)&&z<-.1f); // held secondary direction hysteresis
    CHECK(steering.Step(1,-.4f,x,z)&&z==0);CHECK(steering.Step(1,-.6f,x,z)&&z==0);
    CHECK(steering.Step(1,.6f,x,z)&&z==0); // opposite sign must enter deliberately
    CHECK(steering.Step(1,.8f,x,z)&&z>.1f);
    puts("PASS native forward/back cone, deliberate diagonal, entry/exit hysteresis, pure strafe and neutral");
    ControllerTranslator translator;ControllerFrame frame;frame.focused=true;frame.timeMs=10000;for(auto& hand:frame.hands)hand.active=true;
    frame.hands[0].stick[0]=-.9f;translator.Translate(frame,frame.timeMs,true,false,2,1,2,0);
    frame.timeMs+=10;auto commands=translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,true);
    CHECK(!commands.held[MenuLeft]&&!commands.held[Left]); //held world input cannot turn the lock
    frame.timeMs+=10;frame.hands[0].stick[0]=0;translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,true);
    frame.timeMs+=100;translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,true);frame.timeMs+=10;frame.hands[0].stick[0]=-.9f;commands=translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,true);
    CHECK(commands.lockpick&&commands.held[MenuLeft]&&!commands.held[Left]&&!commands.held[Forward]&&!commands.held[Right]);
    ControllerInput input;input.Initialize(L"Z:\\no-such-native-options.ini");CHECK(input.Binding(MenuLeft).offset==0x1e&&input.Binding(MenuRight).offset==0x20);
    int leftDown=0,leftUp=0;input.Apply(commands,[&](unsigned type,unsigned offset,bool down){if(type==0&&offset==0x1e){if(down)++leftDown;else ++leftUp;}return true;});CHECK(leftDown==1);
    input.Apply(commands,[&](unsigned,unsigned,bool){return true;});frame.timeMs+=10;frame.focused=false;commands=translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,true);
    input.Apply(commands,[&](unsigned type,unsigned offset,bool down){if(type==0&&offset==0x1e&&!down)++leftUp;return true;});CHECK(leftUp==1&&!commands.held[MenuLeft]);
    CHECK(LockpickControls(100,"PS_Player","PS_Player")&&LockpickControls(0,"TS_Lockpick","PS_Player")&&!LockpickControls(73,"TS_UseMob","PS_Player"));
    puts("PASS neutral-armed menu/lockpick direction, native A/D menu keys and release on tracking focus loss");
    CHECK(ActivityThirdPerson(15,2,"PS_Player","PS_Player")&&ActivityThirdPerson(0,2,"_AI_FinishingAttack","PS_Player"));
    StereoCameraState camera{true,true,true,false,false,false,true};CHECK(!ComfortFirstPerson(camera,true,true,false));camera.activity=false;CHECK(ComfortFirstPerson(camera,true,true,false));
    puts("PASS finishing native action/task keeps third-person/full body then permits calibrated first-person return");
    auto eye=WorldStatusImage(1),finish=WorldStatusImage(2),shield=WorldStatusImage(4);
    unsigned ink=0,red=0,skullInk=0,shieldInk=0,softEdge=0;bool premultiplied=true;
    for(unsigned y=0;y<RadialPixels;++y)for(unsigned x=0;x<RadialPixels;++x){
        auto c=eye[y*RadialPixels+x];if(c)++ink;
        auto f=finish[y*RadialPixels+x];if(f)++skullInk;
        auto p=shield[y*RadialPixels+x];if(p)++shieldInk;
        if(x>=artwork::StatusIconPixels||y>=artwork::StatusIconPixels)CHECK(c==0&&f==0&&p==0);
        for(auto color:{c,f,p}){auto a=color>>24;CHECK(a<=239);if(a>0&&a<200)++softEdge;
            premultiplied&=((color>>16)&255)<=a&&((color>>8)&255)<=a&&(color&255)<=a;}
        if(((f>>16)&255)>((f>>8)&255)*1.4f&&((f>>16)&255)>45)++red;
    }
    CHECK(ink>2500&&ink<14000&&premultiplied&&softEdge>100&&eye[0]==0);
    CHECK(skullInk>4000&&skullInk<14000&&red>1500&&finish[0]==0);
    CHECK(shieldInk>4000&&shieldInk<14000&&shield[0]==0);
    CHECK(eye!=finish&&finish!=shield&&eye!=shield);
    // Embedded art is copied exactly, with transparent unused swapchain area.
    for(int status:{1,2,4}){auto pixels=WorldStatusImage(status);auto const* source=artwork::Status(status);
        CHECK(source);for(unsigned y=0;y<artwork::StatusIconPixels;++y)
            CHECK(!memcmp(pixels.data()+y*RadialPixels,source+y*artwork::StatusIconPixels,artwork::StatusIconPixels*4));}
    CHECK(!artwork::Status(0)&&!artwork::Status(3)&&!artwork::Status(5));
    auto notice=WorldStatusImage(3);CHECK(notice[0]>>24==239&&notice[StatusWidth]==0);
    for(int status:{0,5}){auto pixels=WorldStatusImage(status);for(auto pixel:pixels)CHECK(!pixel);}
    for(float width:{1.04f,2.6f,4.68f})for(float distance:{1.f,2.f,5.f})for(float vertical:{-1.5f,-.7f,-.3f})for(float aspect:{9.f/16.f,1.f}){
        auto shared=WorldStatusVisual(1,width,distance,vertical,aspect);
        for(int status:{1,2,4}){auto layout=WorldStatusVisual(status,width,distance,vertical,aspect);
            CHECK(layout.width==artwork::StatusIconPixels&&layout.height==artwork::StatusIconPixels&&layout.x==0&&layout.y==shared.y);
            CHECK(layout.metersWide==.07f&&layout.metersHigh==.07f);
            float lowerEdge=(layout.y-layout.metersHigh*.5f)/StatusDistance;
            float quickbarTop=(vertical-.36f*width*aspect)/distance;
            CHECK(std::fabs(lowerEdge-quickbarTop-.025f/StatusDistance)<.00001f);
        }
    }
    CHECK(WorldStatusVisual(1).y<0&&WorldStatusVisual(3).width==256);
    if(argc>=3){SaveImage(argv[1],1);SaveImage(argv[2],2);}if(argc>=4)SaveImage(argv[3],4);
    puts("PASS embedded bronze eye/steel shield/crimson skull, exact alpha texels and soft edges, unchanged notice, transparent padding and common centered 7cm placement above HUD");return 0;
}
