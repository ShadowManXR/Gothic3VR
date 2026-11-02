#include "GameResolution.h"
#include "EarlyResolutionSettings.h"
#include <cstdio>
using namespace diagnostic;
int main(){int failures=0;auto check=[&](bool b,char const* m){if(!b){++failures;std::printf("FAIL %s\n",m);}};
    auto p=MakeUpscalePlan(3840,2160,UpscaleMethod::NIS,75,25);check(p.inputWidth==2880&&p.inputHeight==1620&&p.outputWidth==3840&&p.Scaled(),"75 percent reduces real scene pixels");
    p=MakeUpscalePlan(5120,2880,UpscaleMethod::CAS,50,200);check(p.inputWidth==2560&&p.inputHeight==1440&&p.sharpness==100,"CAS 50 percent and sharpness bounded");
    p=MakeUpscalePlan(3840,2160,UpscaleMethod::Off,50,25);check(!p.Scaled()&&p.inputWidth==3840,"Off restores full native output");
    p=MakeUpscalePlan(3200,3200,UpscaleMethod::NIS,100,0);check(!p.Scaled(),"100 percent sharpen only");
    p=MakeUpscalePlan(2560,1440,UpscaleMethod::CAS,50,25,false);check(!p.Scaled(),"Use game settings never compounds scaling");
    p=MakeUpscalePlan(2880,2880,UpscaleMethod::NIS,63,25);check(!(p.inputWidth%2)&&p.inputWidth==p.inputHeight,"Even square input");
    auto changed=MatchUpscaleInput(p,2560,1440);check(changed.inputWidth==2560&&changed.outputWidth==2560&&!changed.Scaled(),"Unrestarted game uses matching safe dimensions");
    check(ValidUpscaleMethod(-1)==UpscaleMethod::Off&&ValidUpscaleMethod(900)==UpscaleMethod::Off,"Invalid method safe fallback");
    p=MakeUpscalePlan(3840,2160,UpscaleMethod::Off,50,25,true,true);check(p.fxaa&&!p.Scaled(),"FXAA alone never lowers scene resolution");
    check(MatchUpscaleInput(p,2560,1440).fxaa,"Safe input resize preserves FXAA");
    p=MakeUpscalePlan(3840,2160,UpscaleMethod::Off,50,25,true,true,true);check(p.smaa&&!p.fxaa&&!p.Scaled(),"SMAA and FXAA mutually exclusive; no resolution change");
    check(MatchUpscaleInput(p,2560,1440).smaa&&!MatchUpscaleInput(p,2560,1440).fxaa,"Safe input resize preserves SMAA");
    for(int quality=0;quality<3;++quality){auto q=MakeUpscalePlan(3840,2160,UpscaleMethod::Off,100,0,true,false,true,quality);check(q.smaaQuality==quality&&MatchUpscaleInput(q,1280,720).smaaQuality==quality,"SMAA preset retained through safe input resize");}
    check(ValidSmaaQuality(-1)==0&&ValidSmaaQuality(999)==0,"Unknown SMAA preset safely falls back to Medium");
    wchar_t dir[MAX_PATH],ini[MAX_PATH];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"ups",0,ini);
    WritePrivateProfileStringW(L"Custom",L"Keep",L"yes",ini);VrSettings s;s.renderWidth=3840;s.renderHeight=2160;s.upscaleMethod=UpscaleMethod::NIS;s.upscaleScalePercent=75;s.upscaleSharpness=37;
    s.fxaaEnabled=true;check(SaveVrSettings(ini,s),"Save graphics settings atomically");auto loaded=LoadVrSettings(ini);check(loaded.upscaleMethod==s.upscaleMethod&&loaded.upscaleScalePercent==75&&loaded.upscaleSharpness==37&&loaded.fxaaEnabled,"Graphics and independent FXAA settings roundtrip");
    wchar_t value[32];GetPrivateProfileStringW(L"Custom",L"Keep",L"",value,32,ini);check(!wcscmp(value,L"yes"),"Preserve custom settings");
    unsigned w=0,h=0;check(EarlyResolutionTarget(loaded,nullptr,w,h)&&w==2880&&h==1620,"Early engine patch uses same internal scene size");
    WritePrivateProfileStringW(L"Options.Video",L"Resolution.Width",L"1920",ini);WritePrivateProfileStringW(L"Options.Video",L"Resolution.Height",L"1080",ini);WritePrivateProfileStringW(L"Options.Video",L"WindowMode",L"0",ini);
    check(ApplyGameResolution(ini,loaded),"Native video options written");check(GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,ini)==2880&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Height",0,ini)==1620,"Launcher applies internal size, not output");
    check(ApplyGameResolution(ini,loaded)&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,ini)==2880,"Repeated launches do not reduce again");
    loaded.upscaleMethod=UpscaleMethod::Off;check(ApplyGameResolution(ini,loaded)&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,ini)==3840,"Off restores selected output size");
    loaded.renderWidth=loaded.renderHeight=0;loaded.upscaleMethod=UpscaleMethod::NIS;check(ApplyGameResolution(ini,loaded)&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,ini)==3840,"Use game leaves native options alone");
    WritePrivateProfileStringW(L"Stereo",L"UpscaleMethod",L"500",ini);WritePrivateProfileStringW(L"Stereo",L"UpscaleScalePercent",L"-100",ini);WritePrivateProfileStringW(L"Stereo",L"UpscaleSharpness",L"500",ini);loaded=LoadVrSettings(ini);
    check(loaded.upscaleMethod==UpscaleMethod::Off&&loaded.upscaleScalePercent==50&&loaded.upscaleSharpness==100,"INI values bounded");
    DeleteFileW(ini);std::wstring backup=std::wstring(ini)+L".bak";DeleteFileW(backup.c_str());return failures?1:0;
}
