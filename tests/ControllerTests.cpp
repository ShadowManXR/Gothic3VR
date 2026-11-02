#include "ControllerControls.h"
#include "ControllerInput.h"
#include <cstdio>
#include <vector>
int main(){
    unsigned failures=0;auto check=[&](bool ok,char const* text){printf("%s %s\n",ok?"PASS":"FAIL",text);failures+=!ok;};
    diagnostic::ControllerFrame frame;frame.focused=true;frame.timeMs=1000;for(auto& hand:frame.hands){hand.active=true;hand.aimInView.valid=true;}
    float u=0,v=0;auto& aim=frame.hands[1].aimInView;
    check(diagnostic::ControllerPanelHit(aim,2.6f,1.4625f,2,0,u,v)&&std::fabs(u-.5f)<.001f&&std::fabs(v-.5f)<.001f,"controller ray intersects center of actual VR quad");
    aim.position[0]=1.3f;check(diagnostic::ControllerPanelHit(aim,2.6f,1.4625f,2,0,u,v)&&std::fabs(u-1)<.001f,"right panel edge is reachable");
    aim.position[0]=1.4f;check(!diagnostic::ControllerPanelHit(aim,2.6f,1.4625f,2,0,u,v),"ray outside panel cannot click");
    aim.position[0]=0;aim.quaternion[1]=1;aim.quaternion[3]=0;check(!diagnostic::ControllerPanelHit(aim,2.6f,1.4625f,2,0,u,v),"backwards ray cannot select menu");
    aim.quaternion[1]=0;aim.quaternion[3]=1;aim.position[1]=.2f;check(diagnostic::ControllerPanelHit(aim,2.08f,1.17f,3,.2f,u,v)&&std::fabs(v-.5f)<.001f,"launcher menu offset, scale and distance are respected");
    diagnostic::ControllerTranslator translate;auto commands=translate.Translate(frame,1000,true,false,2.6f,1.4625f,2,0);
    frame.hands[0].stick[1]=.8f;frame.hands[0].stick[0]=-.8f;frame.hands[1].trigger=1;
    commands=translate.Translate(frame,1010,true,false,2.6f,1.4625f,2,0);check(commands.held[diagnostic::Forward]&&commands.held[diagnostic::Left]&&commands.held[diagnostic::Use1],"joystick movement and native use command");
    commands=translate.Translate(frame,1010,true,true,2.6f,1.4625f,2,0);check(!commands.held[diagnostic::Forward]&&!commands.held[diagnostic::UiClick],"conversation transition releases walking and does not click held trigger");
    frame.hands[1].trigger=0;translate.Translate(frame,1010,true,true,2.6f,1.4625f,2,0);frame.hands[1].trigger=1;
    commands=translate.Translate(frame,1010,true,true,2.6f,1.4625f,2,0);check(commands.pointer&&commands.held[diagnostic::UiClick],"new trigger press can select menu after neutral state");
    commands=translate.Translate(frame,1200,true,true,2.6f,1.4625f,2,0);check(!commands.pointer&&!commands.held[diagnostic::UiClick],"stale frame releases pointer and buttons");
    commands=translate.Translate(frame,1010,false,false,2.6f,1.4625f,2,0);check(!commands.held[diagnostic::Use1]&&!commands.held[diagnostic::Forward],"inactive game/VR releases owned inputs");
    frame.focused=false;commands=translate.Translate(frame,1010,true,false,2.6f,1.4625f,2,0);check(!commands.held[diagnostic::Use1],"runtime focus loss releases input");
    diagnostic::ControllerInput input;input.Initialize(L"missing-fixture.ini");
    struct Sent{unsigned type,offset;bool down;};std::vector<Sent> sent;auto sink=[&](unsigned type,unsigned offset,bool down){sent.push_back({type,offset,down});return true;};
    commands={};commands.held[diagnostic::UiClick]=true;input.Apply(commands,sink);input.Apply(commands,sink);
    check(sent.size()==1&&sent[0].type==1&&sent[0].offset==3&&sent[0].down,"held click emits one down event rather than repeated clicks");
    input.Apply(diagnostic::ControllerCommands{},sink);check(sent.size()==2&&!sent[1].down,"neutral frame releases previously held click");
    input.Apply(diagnostic::ControllerCommands{},sink);check(sent.size()==2,"input cleanup is idempotent");
    diagnostic::ControllerTurn slow,fast;slow.Step(1,50,1000);fast.Step(1,100,1000);
    int slowTotal=0,fastTotal=0;for(uint64_t now=1010;now<=2000;now+=10){slowTotal+=slow.Step(1,50,now);fastTotal+=fast.Step(1,100,now);}
    check(slowTotal==300&&fastTotal==600,"turn speed setting scales continuous horizontal mouse movement");
    diagnostic::ControllerTurn oldMaximum,increased;oldMaximum.Step(1,200,1000);increased.Step(1,800,1000);
    int oldTotal=0,increasedTotal=0;for(uint64_t now=1010;now<=2000;now+=10){oldTotal+=oldMaximum.Step(1,200,now);increasedTotal+=increased.Step(1,800,now);}
    check(oldTotal==1200&&increasedTotal==4800,"800 percent turn produces four times the previous launcher maximum at the same frame rate");
    check(fast.Step(.2f,100,2010)==0&&fast.Step(0,100,2020)==0,"deadzone and menu/focus neutral input stop turning");
    check(fast.Step(-1,100,2030)==-6&&fast.Step(1,100,9000)<=30,"turn is signed and a long stall cannot cause a large rotation jump");
    diagnostic::ControllerTranslator modes;frame={};frame.focused=true;frame.timeMs=10000;for(auto& hand:frame.hands)hand.active=true;
    auto modeStep=[&](bool menu=false){frame.timeMs+=10;return modes.Translate(frame,frame.timeMs,true,menu,2,1,2,0);};
    for(int i=0;i<4;++i)modeStep();frame.hands[0].stick[1]=1;frame.hands[0].trigger=1;commands=modeStep();check(!commands.held[diagnostic::Sprint]&&!commands.quickHeal,"a single left-trigger press cannot Sprint or heal");
    frame.hands[0].squeeze=1;commands=modeStep();check(!commands.quickHeal&&!commands.held[diagnostic::Sprint],"support grip cannot complete a heal gesture");
    bool oneHeal=true;for(int i=0;i<120;++i)oneHeal&=!modeStep().quickHeal;check(oneHeal,"a held trigger cannot count as a second tap");
    frame.hands[0].trigger=.6f;check(!modeStep().quickHeal,"analog trigger jitter cannot rearm heal");frame.hands[0].trigger=1;check(!modeStep().quickHeal,"activation-threshold jitter cannot repeat heal");
    auto healTap=[&](){frame.hands[0].trigger=0;for(int i=0;i<4;++i)modeStep();frame.hands[0].trigger=1;auto pulse=modeStep();for(int i=0;i<4;++i)modeStep();return pulse.quickHeal;};
    check(!healTap()&&healTap(),"two deliberate trigger taps heal while support grip stays held");
    check(frame.hands[0].squeeze==1&&!modeStep().quickHeal,"healing neither releases support nor repeats on hold");
    check(!healTap()&&healTap(),"each subsequent potion requires its own pair of taps");
    frame.hands[0].trigger=0;frame.hands[0].stickClick=true;commands=modeStep();check(!commands.held[diagnostic::Sprint]&&!commands.quickHeal,"L3 press reserves recenter without healing");
    frame.hands[0].stickClick=false;commands=modeStep();check(commands.held[diagnostic::Sprint]&&!commands.quickHeal,"L3 tap restores Sprint without touching quick heal");
    check(modeStep().held[diagnostic::Sprint],"Sprint persists without holding L3");
    frame.hands[0].stick[1]=0;check(!modeStep().held[diagnostic::Sprint],"stopping clears Sprint selection");frame.hands[0].stick[1]=1;check(!modeStep().held[diagnostic::Sprint],"moving again starts in Run");
    frame.hands[0].stickClick=true;modeStep();frame.hands[0].stickClick=false;check(modeStep().held[diagnostic::Sprint],"fresh L3 tap restarts Sprint");
    frame.hands[0].stickClick=true;modeStep();frame.hands[0].stickClick=false;check(!modeStep().held[diagnostic::Sprint],"second L3 tap returns Run");
    frame.hands[0].trigger=frame.hands[0].squeeze=1;modeStep(true);commands=modeStep();check(!commands.quickHeal&&!commands.held[diagnostic::Sprint],"held trigger cannot fire across menu transition");
    check(!healTap()&&healTap(),"a fresh double tap is required after leaving a menu");
    frame.hands[0].stick[0]=frame.hands[0].stick[1]=0;modeStep(); // neutral before a new lateral gesture
    frame.hands[0].trigger=0;frame.hands[0].squeeze=.9f;frame.hands[0].stick[0]=-.8f;frame.hands[0].stick[1]=.4f;commands=modeStep();
    check(!commands.held[diagnostic::Sprint]&&commands.moveX==-1&&commands.moveZ==0&&!commands.held[diagnostic::Backward],"support grip preserves deliberate lateral steering without Sprint");
    frame.hands[0].secondary=true;check(modeStep().sneakToggleButton,"Y remains native Sneak input");frame.hands[0].secondary=false;frame.hands[0].menu=true;check(modeStep().held[diagnostic::Back],"Menu retains native Back");
    auto stale=modes.Translate(frame,frame.timeMs+101,true,false,2,1,2,0);check(!stale.held[diagnostic::Sprint]&&!stale.quickHeal,"stale input releases Sprint and heal");
    diagnostic::ControllerTranslator secondary;diagnostic::ControllerFrame controls{};controls.focused=true;
    for(auto& hand:controls.hands)hand.active=true;
    auto controlFrame=[&](uint64_t time,bool menu=false,bool activity=false){controls.timeMs=time;return secondary.Translate(controls,time,true,menu,2,1,2,0,true,false,activity);};
    controlFrame(20000);controls.hands[0].trigger=1;commands=controlFrame(20010);
    check(!commands.held[diagnostic::Use2],"left trigger no longer emits native secondary action");
    controls.hands[1].squeeze=1;commands=controlFrame(20020);
    check(commands.held[diagnostic::Use2],"right grip emits native secondary action even with left trigger held");
    controls.hands[1].squeeze=0;commands=controlFrame(20030);check(!commands.held[diagnostic::Use2],"right grip release releases native secondary action");
    controls.hands[0].trigger=0;commands=controlFrame(20040,false,true);
    controls.hands[1].squeeze=1;commands=controlFrame(20050,false,true);
    check(commands.held[diagnostic::Use2],"right grip also operates directional activity secondary action");
    commands=controlFrame(20060,true);check(!commands.held[diagnostic::Use2]&&!commands.held[diagnostic::UiClick],"held right grip cannot click or send secondary action into a menu");
    commands=controlFrame(20070);check(!commands.held[diagnostic::Use2],"held right grip stays disarmed when leaving a menu");
    controls.hands[1].squeeze=0;controlFrame(20080);controls.hands[1].squeeze=1;commands=controlFrame(20090);
    check(commands.held[diagnostic::Use2],"fresh right grip after neutral re-arms secondary action");
    controls.hands[1].active=false;commands=controlFrame(20100);
    check(!commands.held[diagnostic::Use2],"inactive right controller cannot hold secondary action");
    {
        diagnostic::DoubleTapPress tap;uint64_t stamp=1000;
        auto edge=[&](bool down,unsigned dt=10,bool allowed=true){stamp+=dt;return tap.Step(allowed,down,stamp);};
        bool clean=true;for(int i=0;i<20;++i)clean&=!edge(true);check(clean,"startup-held trigger cannot heal");
        for(int i=0;i<4;++i)edge(false);check(!edge(true),"first deliberate tap is silent");
        check(!edge(false)&&!edge(true),"one-frame contact bounce is not a second tap");
        for(int i=0;i<4;++i)edge(true);for(int i=0;i<4;++i)edge(false);
        check(edge(true),"deliberate second tap emits one heal pulse");
        for(int i=0;i<80;++i)check(!edge(true),"held second tap never repeats");
        for(int i=0;i<4;++i)edge(false);check(!edge(true),"a completed pair does not overlap the next pair");
        for(int i=0;i<50;++i)edge(false);check(!edge(true),"tap pairs expire after 450 ms");
        check(!edge(false,10,false),"menu/focus gate cancels a pending pair");
        for(int i=0;i<4;++i)edge(false);check(!edge(true),"a canceled pair needs a new first tap");
        check(!edge(false,150)&&!edge(true),"a tracking gap cannot complete a previous gesture");
    }
    return failures?1:0;
}
