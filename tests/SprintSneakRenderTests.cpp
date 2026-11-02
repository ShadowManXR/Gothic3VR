#include <windows.h>
#include "RenderAlphaSnapshot.h"
#include "ControllerControls.h"
#include "ControllerSneakToggle.h"
#include "NativeSprintPolicy.h"
#include "NativeControllerModes.h"
#include "NativeSneakAction.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Visual {float alpha;bool visible;};
void __cdecl SetAlpha(void* p,float value){static_cast<Visual*>(p)->alpha=value;}
struct Trial {Visual body{0,true},weapon{0,true},head{0,false};RenderAlphaSnapshot snapshot;};
void RenderTrial(Trial& t,bool fault){
    t.snapshot.Remember(&t.body,t.body.alpha,SetAlpha);t.snapshot.Remember(&t.weapon,t.weapon.alpha,SetAlpha);t.snapshot.Remember(&t.head,t.head.alpha,SetAlpha);
    __try {t.snapshot.Opaque();if(fault)RaiseException(0xe0123456,0,0,nullptr);t.snapshot.Opaque();}
    __finally {t.snapshot.Restore();}
}
bool FaultTrial(Trial& t){__try{RenderTrial(t,true);}__except(EXCEPTION_EXECUTE_HANDLER){return true;}return false;}
int main(){
    Trial t;t.snapshot.Remember(&t.body,0,SetAlpha);t.snapshot.Remember(&t.weapon,.2f,SetAlpha);t.snapshot.Remember(&t.head,.6f,SetAlpha);
    t.snapshot.Opaque();CHECK(t.body.alpha==1&&t.weapon.alpha==1&&t.head.alpha==1&&!t.head.visible);
    t.weapon.alpha=0;t.snapshot.Opaque();CHECK(t.weapon.alpha==1); //second-eye/native hide cannot undo opaque state
    t.snapshot.Restore();CHECK(t.body.alpha==0&&t.weapon.alpha==.2f&&t.head.alpha==.6f&&!t.head.visible);
    t.snapshot.Restore();Trial failed;CHECK(FaultTrial(failed)&&failed.body.alpha==0&&failed.weapon.alpha==0&&failed.head.alpha==0&&!failed.head.visible&&!failed.snapshot.active);
    RenderAlphaSnapshot bounded;Visual many[RenderAlphaSnapshot::Capacity+1]{};
    for(unsigned i=0;i<bounded.Capacity;++i)CHECK(bounded.Remember(many+i,0,SetAlpha));CHECK(!bounded.Remember(many+bounded.Capacity,0,SetAlpha)&&!bounded.active);
    puts("PASS two-eye opaque body/equipment, hidden head, exact independent alpha restoration, render fault and collection bounds");
    ControllerSneakToggle sneak;uint64_t now=10000;
    auto y=[&](bool down,bool game=true,bool fresh=true,bool learned=true,uintptr_t player=1){now+=40;return sneak.Step(player,fresh,game,down,learned,now);};
    CHECK(!y(true)&&!y(true)&&!y(false)&&!y(false)&&!y(true)); //startup hold and first intentional tap
    for(int i=0;i<20;++i)CHECK(!y(true)); //holding never acts as the second tap; first tap expires
    CHECK(!y(false)&&!y(false)&&!y(true)&&!y(false)&&!y(false)&&y(true));
    for(int i=0;i<20;++i)CHECK(y(true));CHECK(y(false)&&y(false)&&y(true)); //single press cannot exit
    CHECK(y(false)&&y(false)&&!y(true)); //second press exits
    CHECK(!y(false)&&!y(false)&&!y(true));CHECK(!y(false,false)&&!y(true,false));CHECK(!y(true)&&!y(false)&&!y(false)&&!y(true));
    CHECK(!y(false)&&!y(false)&&y(true));CHECK(!y(false,true,false)&&!sneak.Enabled());
    CHECK(!y(false,true,true,false)&&!y(false,true,true,false)&&!y(true,true,true,false));CHECK(!y(false,true,true,true,2));
    puts("PASS double Y tap on/off, expiry, hold/startup, menu page isolation, learned skill, actor and focus gates");
    ControllerTranslator controls;ControllerFrame f;f.focused=true;f.timeMs=30000;for(auto& h:f.hands)h.active=true;
    auto sample=[&](bool menu=false){f.timeMs+=20;return controls.Translate(f,f.timeMs,true,menu,2,1,2,0);};
    sample();f.hands[0].stick[1]=1;f.hands[0].squeeze=1;CHECK(!sample().held[Sprint]); // support grip remains independent
    f.hands[0].stickClick=true;CHECK(!sample().held[Sprint]);f.hands[0].stickClick=false;CHECK(sample().held[Sprint]);
    for(int i=0;i<100;++i)CHECK(sample().held[Sprint]);f.hands[0].stick[1]=0;CHECK(!sample().held[Sprint]);f.hands[0].stick[1]=1;CHECK(!sample().held[Sprint]);
    f.hands[0].stickClick=true;sample();f.hands[0].stickClick=false;CHECK(sample().held[Sprint]);
    f.hands[0].stickClick=true;sample();f.hands[1].stickClick=true;auto c=sample();CHECK(c.captured&&!c.held[Sprint]&&!c.quickHeal);
    f.hands[0].stickClick=f.hands[1].stickClick=false;for(int i=0;i<3;++i)CHECK(!sample().held[Sprint]);
    auto healTap=[&](){f.hands[0].trigger=0;for(int i=0;i<3;++i)sample();f.hands[0].trigger=1;auto pulse=sample();for(int i=0;i<2;++i)sample();return pulse.quickHeal;};
    CHECK(!healTap()&&healTap());for(int i=0;i<20;++i)CHECK(!sample().quickHeal);sample(true);CHECK(!sample().quickHeal);
    CHECK(!healTap()&&healTap());CHECK(!sample().quickHeal&&f.hands[0].squeeze==1);
    puts("PASS L3 Sprint until stop, double-left-trigger heal, independent support grip and recenter/menu isolation");
    NativeControllerModes mode;now=40000;
    // Replay L3 selected while still standing: do not send the native
    // Shift press until ordinary Run has actually started and braking ends.
    bool requested=true;
    for(int i=0;i<30;++i){bool standing=i<10,braking=i>=10&&i<15;
        bool press=NativeSprintPress(requested,true,false,standing,braking);
        CHECK(press==(i>=15));mode.Update(true,1,now,true,false,press);
        CHECK(mode.Request(1,now,3,4)==4); //Walk never survives a native delayed poll
        CHECK(mode.Request(1,now,5,4)==(press?0:4)); //native Sprint never forced
        now+=10;}
    for(int i=0;i<100;++i){bool press=NativeSprintPress(true,true,false,false,false);CHECK(press);
        mode.Update(true,1,now,true,false,press);CHECK(mode.Request(1,now,5,4)==0&&mode.Request(1,now,4,4)==0&&mode.Request(1,now,3,4)==4);now+=10;}
    CHECK(!NativeSprintPress(true,false,false,false,false)&&!NativeSprintPress(true,true,true,false,false));
    mode.Update(true,1,now,true,true,false);CHECK(mode.Request(1,now,3,4)==0); //native Sneak walk pace preserved
    mode.Update(true,1,now+10,true,false,false);CHECK(mode.Request(1,now+10,3,4)==4&&mode.Request(1,now+10,5,4)==4);
    puts("PASS standing/braking Sprint selection defers native Sprint press until Run starts; only Walk3 normalized; native Sprint and Sneak permissions retained");
    ControllerSneakToggle radialSneak;NativeSneakAction native;int presses=0,releases=0;uint64_t time=50000;
    auto emit=[&](uintptr_t actor,NativeGameAction const& action){if(actor!=1)return false;if(action.flags&1)++presses;else ++releases;return true;};
    for(bool down:{false,false,true,false,false,true}){time+=40;bool wanted=radialSneak.Step(1,true,true,down,true,time);native.Update(1,true,wanted,time,emit);}
    CHECK(radialSneak.Enabled()&&native.Owns(1,time)&&presses==1&&releases==0);
    // R3 wheel/pageY captures controller input while native Sneak stays active.
    // No release/repress or temporary Stand permits native weapon selection.
    for(int i=0;i<40;++i){time+=40;CHECK(!radialSneak.Step(1,true,false,i%3==0,true,time));native.Update(1,true,radialSneak.Enabled(),time,emit);CHECK(native.Owns(1,time));}
    CHECK(presses==1&&releases==0);time+=40;CHECK(radialSneak.Step(1,true,true,false,true,time));native.Update(1,true,true,time,emit);CHECK(presses==1&&releases==0);
    time+=40;native.Update(1,false,false,time,emit);CHECK(releases==1); //actual GUI/world suspension still releases input
    puts("PASS radial keeps native Sneak and no equipment suspension; pageY cannot toggle mode or release native Down");return 0;
}
