#pragma once
#include "VrSettings.h"
#include <array>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

namespace diagnostic::edd {
enum class Status { Disabled, Ready, Invalid };
struct Profile { std::array<int,5> values{}; };
inline std::string Trim(std::string s){auto a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos)return {};return s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);}
inline Status Parse(std::string const& text,Profile& result){
    std::map<std::string,std::string> fields;
    bool duplicate=false;
    for(size_t start=0;start<text.size();){
        auto end=text.find('\n',start);auto line=text.substr(start,end==std::string::npos?end:end-start);start=end==std::string::npos?text.size():end+1;
        line=Trim(line.substr(0,line.find(';')));
        if(line.size()<3||line.front()!='['||line.back()!=']')continue;
        auto equal=line.find('=');if(equal==std::string::npos)continue;
        auto key=Trim(line.substr(1,equal-1)),value=Trim(line.substr(equal+1,line.size()-equal-2));
        for(auto& c:key)c=char(std::toupper(static_cast<unsigned char>(c)));
        if(key!="ULTRA DISTANCE"&&key!="SH"&&key!="SL"&&key!="VR"&&key!="CLIP H"&&key!="CLIP L")continue;
        if(!fields.emplace(key,value).second)duplicate=true;
    }
    auto flag=fields.find("ULTRA DISTANCE");if(flag==fields.end())return Status::Disabled;
    auto enabled=flag->second;for(auto& c:enabled)c=char(std::toupper(static_cast<unsigned char>(c)));
    if(enabled=="FALSE")return Status::Disabled;
    if(enabled!="TRUE"||duplicate)return Status::Invalid;
    auto integer=[&](char const* key,int fallback,long long& value,bool required){
        auto item=fields.find(key);if(item==fields.end()){value=fallback;return !required;}
        if(item->second.empty())return false;
        char* end=nullptr;errno=0;value=std::strtoll(item->second.c_str(),&end,10);
        return !errno&&end&&!*end&&value>=-300000&&value<=300000;
    };
    long long high,low,vegetation,clipHigh,clipLow;
    if(!integer("SH",0,high,true)||!integer("SL",0,low,true)||!integer("VR",0,vegetation,true)||
       !integer("CLIP H",0,clipHigh,false)||!integer("CLIP L",0,clipLow,false))return Status::Invalid;
    auto detail=high+clipHigh,distant=low+clipLow;
    if(high<1000||high>50000||detail<1000||detail>50000||low<10000||low>300000||
       distant<10000||distant<=detail||distant>300000||vegetation<1000||vegetation>30000)return Status::Invalid;
    result.values={int(high),int(low),int(detail),int(distant),int(vegetation)};return Status::Ready;
}
inline constexpr wchar_t const* Sections[]={L"Engine.Setup",L"Engine.Setup",L"Option.Sliders",L"Option.Sliders",L"Option.Sliders"};
inline constexpr wchar_t const* Keys[]={L"Render.PrefetchGridCellSize",L"Render.PrefetchGridCellSizeLowPoly",
    L"DistanceHigh.fFarClippingPlane_High",L"DistanceLow.fFarClippingPlaneLowPolyMesh_High",L"VegetationViewRange.fViewDistance_VeryHigh"};

// Import settings from the user's installed EDD ini, using the existing native
// engine. No EDD executables, DLLs, ReShade assets or memory patches are loaded.
inline bool Apply(std::wstring const& gameDirectory,DWORD& failure){
    // Explicit native edits take precedence over the legacy EDD import.
    auto nativeIni=gameDirectory+L"\\Ini\\ge3.ini";
    if(GetPrivateProfileIntW(L"Gothic3VR.IniTweaks",L"OwnDistances",0,nativeIni.c_str())){failure=0;return true;}
    failure=0;auto source=gameDirectory+L"\\G3_edd.ini";
    HANDLE file=CreateFileW(source.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    if(file==INVALID_HANDLE_VALUE){failure=GetLastError();if(failure==ERROR_FILE_NOT_FOUND||failure==ERROR_PATH_NOT_FOUND){failure=0;return true;}return false;}
    DWORD size=GetFileSize(file,nullptr),read=0;
    if(size>131072){CloseHandle(file);failure=ERROR_FILE_TOO_LARGE;return false;}
    std::string text(size,'\0');bool ok=ReadFile(file,text.data(),size,&read,nullptr)!=FALSE&&read==size;auto error=GetLastError();CloseHandle(file);
    if(!ok){failure=error?error:ERROR_READ_FAULT;return false;}
    if(text.compare(0,3,"\xef\xbb\xbf")==0)text.erase(0,3);
    Profile profile;auto state=Parse(text,profile);if(state==Status::Disabled)return true;
    if(state==Status::Invalid){failure=ERROR_INVALID_DATA;return false;}
    auto ini=gameDirectory+L"\\Ini\\ge3.ini";
    bool different=false;
    for(unsigned i=0;i<5;++i){
        wchar_t value[80];GetPrivateProfileStringW(Sections[i],Keys[i],L"",value,80,ini.c_str());
        if(!*value){failure=ERROR_INVALID_DATA;return false;}
        wchar_t* end=nullptr;auto current=wcstod(value,&end);
        different|=end==value||*end||current!=profile.values[i];
    }
    if(!different)return true;
    auto backup=gameDirectory+L"\\Ini\\ge3.before-vr-edd.ini";
    if(!CopyFileW(ini.c_str(),backup.c_str(),TRUE)&&GetLastError()!=ERROR_FILE_EXISTS){failure=GetLastError();return false;}
    ok=SaveProfileAtomic(ini.c_str(),[&](wchar_t const* temporary){
        for(unsigned i=0;i<5;++i){wchar_t value[40];swprintf_s(value,i<2?L"%d":L"%d.0",profile.values[i]);
            if(!WritePrivateProfileStringW(Sections[i],Keys[i],value,temporary))return false;}
        return true;
    });
    if(!ok)failure=GetLastError()?GetLastError():ERROR_WRITE_FAULT;
    return ok;
}
}
