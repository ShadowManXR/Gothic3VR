#include "ControllerInput.h"
#include "RadialImage.h"
#include <cstdio>
#include <vector>
int wmain(int argc,wchar_t** argv) {
    using namespace diagnostic;unsigned failures=0;auto check=[&](bool ok,char const* text){printf("%s %s\n",ok?"PASS":"FAIL",text);failures+=!ok;};
    for(int i=0;i<10;++i){float angle=float(i)*6.28318530718f/10;check(RadialSector(std::sin(angle),std::cos(angle),10)==i,"all ten native quick slots are reachable clockwise from top");}
    check(RadialSector(0,0,10)==-1&&RadialSector(NAN,1,10)==-1,"center and invalid axes do not create a selection");
    QuickRadial radial;auto step=[&](bool l,bool r,bool page,float x,float y,uint64_t t,bool confirm=false,bool cancel=false){return radial.Step(true,l,r,page,x,y,t,confirm,cancel);};
    check(!step(true,false,false,0,1,1000).state.visible,"held R3 at startup needs release before opening");step(false,false,false,0,0,1010);
    check(step(true,false,false,0,1,1020).state.selected==0,"fresh R3 opens radial with slot one selected");
    auto release=step(false,false,false,0,0,1030);check(release.capture&&release.choice==-1&&release.state.visible&&release.state.selected==0,"release of R3 leaves radial open and spring-to-center retains selection");
    bool persistent=true;for(uint64_t time=1040;time<6000;time+=10)persistent&=step(false,false,false,0,0,time).state.visible;
    check(persistent,"radial remains open for five seconds without holding the joystick");
    check(step(false,false,false,0,0,6010,true).choice==0&&step(false,false,false,0,0,6020,true).choice==-1,"fresh A press commits exactly once, held A cannot repeat or reopen");
    step(false,false,false,0,0,6030);step(true,false,false,0,1,6040);check(!step(true,true,false,0,1,6050).state.visible&&step(false,false,false,0,0,6060).choice==-1,"reset chord cancels radial without triggering a quick slot");
    step(true,false,false,0,1,6070);radial.Step(false,true,false,false,0,1,6080);check(step(false,false,false,0,0,6090).choice==-1,"focus or menu change cancels selection");
    step(true,false,false,0,1,6100);check(step(false,false,true,.951057f,.309017f,6110).state.page==1&&step(false,false,true,.951057f,.309017f,6120).state.page==1,"Y advances menu page once per press");
    check(step(false,false,false,0,0,6130,true).choice==12,"menu page selects journal from ten independent sectors after thumb moves to A");
    step(false,false,false,0,0,6140);step(true,false,false,0,1,6150);check(step(false,false,false,0,0,6160,false,true).choice==-1,"B cancels selection without inventory action");
    ControllerTranslator translate;ControllerFrame frame{};frame.focused=true;frame.timeMs=2000;for(auto& hand:frame.hands)hand.active=true;
    translate.Translate(frame,2000,true,false,2,1,2,0);frame.timeMs+=10;frame.hands[1].stickClick=true;frame.hands[1].stick[1]=1;frame.hands[1].stick[0]=1;frame.hands[1].trigger=1;
    auto commands=translate.Translate(frame,2010,true,false,2,1,2,0);
    check(commands.radial.visible&&!commands.held[Forward]&&!commands.held[Use1]&&commands.turnX==0,"radial captures movement, turning and attacks");
    frame.timeMs+=10;frame.hands[1].stick[0]=0;frame.hands[1].stick[1]=1;translate.Translate(frame,2020,true,false,2,1,2,0);
    frame.timeMs+=10;frame.hands[1].stickClick=false;commands=translate.Translate(frame,2030,true,false,2,1,2,0);check(commands.radial.visible&&commands.pulse==-1,"translator retains radial when R3 is released");
    frame.hands[1].stick[1]=0;frame.hands[1].primary=true;frame.timeMs+=10;commands=translate.Translate(frame,2040,true,false,2,1,2,0);check(commands.pulse==Quick1&&!commands.held[Jump],"A confirms retained native slot one without jumping");
    frame.hands[1].primary=false;frame.hands[1].trigger=0;frame.timeMs+=10;translate.Translate(frame,2050,true,false,2,1,2,0);
    frame.timeMs+=10;frame.hands[1].stickClick=true;translate.Translate(frame,2060,true,false,2,1,2,0);frame.timeMs+=10;frame.hands[0].stickClick=true;commands=translate.Translate(frame,2070,true,false,2,1,2,0);
    frame.timeMs+=500;commands=translate.Translate(frame,2570,true,false,2,1,2,0);check(commands.recenter&&!commands.radial.visible&&commands.pulse==-1,"R3 L3 camera reset retains priority over open radial");
    ControllerTranslator sameStick;ControllerFrame same{};same.focused=true;for(auto& h:same.hands)h.active=true;
    auto radialFrame=[&](uint64_t t){same.timeMs=t;return sameStick.Translate(same,t,true,false,2,1,2,0);};
    radialFrame(6000);same.hands[1].stickClick=true;commands=radialFrame(6010);
    check(commands.radial.visible&&commands.radial.selected==-1,"R3 click alone opens without an accidental sector");
    same.hands[1].stickClick=false;same.hands[0].stick[0]=1;same.hands[0].stick[1]=1;commands=radialFrame(6020);
    check(commands.radial.selected==-1&&!commands.held[Forward]&&commands.turnX==0,"left stick neither chooses radial sector nor moves the player while open");
    same.hands[1].stick[0]=1;commands=radialFrame(6030);
    check(commands.radial.selected==3&&commands.turnX==0,"right stick changes sector while camera turning remains captured");
    same.hands[1].primary=true;commands=radialFrame(6040);
    check(commands.pulse==Quick4&&commands.turnX==0&&!commands.held[Jump],"A confirms the right-stick sector without turning or jumping");
    same.hands[1].primary=false;commands=radialFrame(6050);
    check(commands.turnX==0,"held selection axis cannot rotate the camera after radial closes");
    same.hands[1].stick[0]=0;radialFrame(6060);same.hands[1].stick[0]=1;commands=radialFrame(6070);
    check(commands.turnX==1,"camera turn resumes after the right stick returns to neutral");
    ControllerTranslator inventoryRadial;frame.hands[1].stickClick=frame.hands[0].stickClick=false;frame.hands[1].trigger=0;frame.timeMs=2600;inventoryRadial.Translate(frame,2600,true,true,2,1,2,0);
    frame.hands[1].stickClick=true;frame.timeMs=2610;commands=inventoryRadial.Translate(frame,2610,true,true,2,1,2,0);check(commands.radial.visible&&!commands.pointer&&!commands.held[UiClick],"radial is available inside inventory while retaining previous mouse position for native item hotkey assignment");
    frame.hands[1].stickClick=false;frame.hands[1].stick[1]=1;frame.timeMs=2620;commands=inventoryRadial.Translate(frame,2620,true,true,2,1,2,0);check(commands.radial.visible,"inventory radial remains open across physical button release");
    frame.hands[1].stick[1]=0;frame.hands[1].primary=true;frame.timeMs=2630;commands=inventoryRadial.Translate(frame,2630,true,true,2,1,2,0);check(commands.pulse==Quick1,"inventory radial A confirms native quick key for item already under cursor");frame.hands[1].primary=false;
    ControllerTranslator titleRadial;titleRadial.Translate(frame,2620,true,true,2,1,2,0,false);frame.hands[1].stickClick=true;frame.timeMs=2630;commands=titleRadial.Translate(frame,2630,true,true,2,1,2,0,false);check(!commands.radial.visible&&commands.pulse==-1,"radial stays unavailable before a game world is loaded");
    auto cached=ControllersDuringRenderWait(frame,true,2650);check(cached.focused&&cached.hands[1].stickClick&&cached.timeMs==frame.timeMs,"skipped graphics frame preserves the recent physical controller state without a fake release");
    check(!ControllersDuringRenderWait(frame,false,2650).focused&&ControllersDuringRenderWait(frame,true,2800).timeMs==frame.timeMs,"actual focus loss clears cache while slow rendering preserves sample timestamp for age rejection");
    // Replay the live failure: each wheel update used to cost ~235 ms, after
    // which Translate cancelled it before the next render sampled controllers.
    ControllerTranslator stalled;ControllerFrame stalledFrame{};stalledFrame.focused=true;stalledFrame.timeMs=4000;for(auto& hand:stalledFrame.hands)hand.active=true;
    auto translateStalled=[&](uint64_t now,bool allowed=true){return stalled.Translate(stalledFrame,now,allowed,false,2,1,2,0);};
    translateStalled(4000);stalledFrame.timeMs=4010;stalledFrame.hands[1].stickClick=true;
    check(translateStalled(4010).radial.visible,"live replay first R3 press opens radial");
    commands=translateStalled(4250);check(commands.radial.visible&&commands.pulse==-1&&!commands.held[Forward]&&commands.turnX==0,"235 ms render stall preserves radial without emitting stale native input");
    stalledFrame.timeMs=4260;check(translateStalled(4260).radial.visible,"held R3 after render stall is not mistaken for another press");
    stalledFrame.timeMs=4270;stalledFrame.hands[1].stickClick=false;stalledFrame.hands[1].primary=true;
    check(translateStalled(4270).radial.visible,"A with no selected sector leaves radial open");
    stalledFrame.timeMs=4280;stalledFrame.hands[1].primary=false;stalledFrame.hands[1].stick[1]=1;stalledFrame.hands[0].stickClick=true;
    check(translateStalled(4280).radial.selected==0,"right stick selection and incidental L3 alone do not cancel radial");
    commands=translateStalled(4520);check(commands.radial.visible&&commands.radial.selected==0,"selection survives second long graphics stall");
    stalledFrame.timeMs=4530;stalledFrame.hands[1].stick[1]=0;stalledFrame.hands[0].stickClick=false;translateStalled(4530);
    stalledFrame.timeMs=4540;stalledFrame.hands[1].primary=true;commands=translateStalled(4540);
    check(commands.pulse==Quick1&&!commands.radial.visible&&!commands.held[Jump],"fresh A after stall commits exactly the retained slot and suppresses jump");
    stalledFrame.timeMs=4550;check(translateStalled(4550).pulse==-1,"held confirm after commit cannot repeat quick-slot action");
    stalledFrame.timeMs=4560;stalledFrame.hands[1].primary=false;translateStalled(4560);stalledFrame.timeMs=4570;stalledFrame.hands[1].stickClick=true;translateStalled(4570);
    stalledFrame.focused=false;check(!translateStalled(4810).radial.visible,"real focus loss still cancels radial during a render stall");
    wchar_t directory[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,directory);GetTempFileNameW(directory,L"gqr",0,path);
    WritePrivateProfileStringW(L"SessionKey.QuickUse1",L"Key1.Type",L"0",path);WritePrivateProfileStringW(L"SessionKey.QuickUse1",L"Key1.Offset",L"37",path);
    WritePrivateProfileStringW(L"SessionKey.QuickUse0",L"Key1.Type",L"0",path);WritePrivateProfileStringW(L"SessionKey.QuickUse0",L"Key1.Offset",L"11",path);
    WritePrivateProfileStringW(L"SessionKey.QuickSave",L"Key1.Type",L"0",path);WritePrivateProfileStringW(L"SessionKey.QuickSave",L"Key1.Offset",L"66",path); // deliberately remapped from F5
    WritePrivateProfileStringW(L"SessionKey.TakeAll",L"Key1.Type",L"0",path);WritePrivateProfileStringW(L"SessionKey.TakeAll",L"Key1.Offset",L"22",path);
    ControllerInput input;input.Initialize(path);struct Sent{unsigned type,offset;bool down;};std::vector<Sent> sent;auto emit=[&](unsigned type,unsigned offset,bool down){sent.push_back({type,offset,down});return true;};
    commands={};commands.pulse=Quick1;input.Update(commands,3000,true,emit);commands={};input.Update(commands,3020,true,emit);
    check(sent.size()==1&&sent[0].offset==37&&sent[0].down,"radial honors user's native quick-slot binding and holds long enough for native polling");
    input.Update(commands,3080,true,emit);check(sent.size()==2&&!sent.back().down,"quick-slot pulse releases at eighty milliseconds");
    commands.pulse=Quick0;input.Update(commands,3090,true,emit);check(sent.back().offset==11&&sent.back().down,"slot zero maps to QuickUse0 rather than quick slot ten");input.Update({},3100,false,emit);check(!sent.back().down,"focus loss immediately releases an outstanding quick-slot key");
    sent.clear();commands={};commands.pulse=QuickSave;input.Update(commands,3200,true,emit);commands={};input.Update(commands,3220,true,emit);
    check(sent.size()==1&&sent[0].type==0&&sent[0].offset==66&&sent[0].down,"Quicksave uses the remapped native save action, never TakeAll or hard-coded F5");
    input.Update(commands,3280,true,emit);check(sent.size()==2&&!sent.back().down,"Quicksave emits one bounded press/release");sent.clear();commands={};commands.pulse=MainMenu;input.Update(commands,3400,true,emit);input.Update({},3480,true,emit);
    check(sent.size()==2&&sent[0].offset==1&&sent[0].down&&!sent[1].down,"Main menu delivers native Escape once without using the reserved controller menu button");DeleteFileW(path);
    ControllerTranslator menuTranslator;ControllerFrame mf{};mf.focused=true;mf.timeMs=4000;for(auto& hand:mf.hands)hand.active=true;
    auto translateMenu=[&](){return menuTranslator.Translate(mf,mf.timeMs,true,false,2,1,2,0);};translateMenu();
    mf.timeMs+=10;mf.hands[1].stickClick=true;translateMenu();mf.timeMs+=10;mf.hands[1].stickClick=false;mf.hands[1].secondary=true;translateMenu();
    mf.timeMs+=10;mf.hands[1].secondary=false;mf.hands[1].stick[0]=-.70710678f;mf.hands[1].stick[1]=-.70710678f;auto mc=translateMenu();
    check(mc.radial.page==1&&mc.radial.selected==5,"Main menu occupies the former Weapon sector");mf.timeMs+=10;mf.hands[1].primary=true;mc=translateMenu();
    check(mc.pulse==MainMenu&&!mc.held[Weapon]&&!mc.held[Jump],"Main menu selection never draws a weapon or jumps");mf.timeMs+=10;check(translateMenu().pulse==-1,"Held confirm cannot repeat Main menu");
    ControllerTranslator saveTranslator;ControllerFrame sf{};sf.focused=true;sf.timeMs=5000;for(auto& hand:sf.hands)hand.active=true;
    auto translateSave=[&](){return saveTranslator.Translate(sf,sf.timeMs,true,false,2,1,2,0);};translateSave();
    sf.timeMs+=10;sf.hands[1].stickClick=true;translateSave();sf.timeMs+=10;sf.hands[1].stickClick=false;sf.hands[1].secondary=true;translateSave();
    sf.timeMs+=10;sf.hands[1].secondary=false;sf.hands[1].stick[0]=-1;sf.hands[1].stick[1]=0;auto sc=translateSave();
    check(sc.radial.page==1&&sc.radial.selected==6,"Former TakeAll utility sector remains in the same position");
    sf.timeMs+=10;sf.hands[1].primary=true;sc=translateSave();check(sc.pulse==QuickSave&&!sc.held[Jump],"Confirming Quicksave selects native save without jumping");
    sf.timeMs+=10;check(translateSave().pulse==-1,"Holding confirm cannot repeat Quicksave");
    RadialState imageState{true,2,argc>2&&_wtoi(argv[2])?1:0};auto pixels=RadialImage(imageState);check(pixels.size()==RadialPixels*RadialPixels&&pixels[0]==0&&(pixels[384*768+384]>>24)==239,"radial image has transparent outside and premultiplied panel pixels");
    check(!wcscmp(RadialLabel(1,5),L"Main menu"),"Main menu label replaces Weapon");
    check(!wcscmp(RadialLabel(1,6),L"Quicksave"),"Utility label matches the Quicksave action");
    check(RadialCount(0)==10&&RadialCount(1)==8&&!wcscmp(RadialLabel(1,7),L"Back"),"utility page excludes sneak and walk; quickslots retain all ten sectors");
    auto benchmark=GetTickCount64();for(int page=0;page<2;++page)for(int selected=-1;selected<RadialCount(page);++selected)RadialImage({true,selected,page});
    printf("Cached radial render: 20 variants in %llu ms\n",static_cast<unsigned long long>(GetTickCount64()-benchmark));
    if(argc>1){FILE* file=nullptr;_wfopen_s(&file,argv[1],L"wb");if(file){BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+unsigned(pixels.size()*4);BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=768;info.biHeight=-768;info.biPlanes=1;info.biBitCount=32;fwrite(&header,sizeof(header),1,file);fwrite(&info,sizeof(info),1,file);fwrite(pixels.data(),4,pixels.size(),file);fclose(file);}}
    return failures?1:0;
}
