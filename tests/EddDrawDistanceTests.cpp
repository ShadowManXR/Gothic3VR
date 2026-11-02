#include "EddDrawDistance.h"
#include <cstdio>
#include <fstream>
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
using namespace diagnostic::edd;
int main(){
    Profile p;std::string source="; EDD format is bracketed assignments, not normal INI sections\n[ULTRA DISTANCE = TRUE] ; enabled\n[SH = 40000]\n[SL = 280000]\n[VR = 20000]\n[CLIP H = 0]\n[CLIP L = 0]\n";
    CHECK((Parse(source,p)==Status::Ready&&p.values==std::array<int,5>({40000,280000,40000,280000,20000})));
    CHECK(Parse(source+"[SH = 10000]\n",p)==Status::Invalid);
    CHECK(Parse("[ULTRA DISTANCE = FALSE]",p)==Status::Disabled);
    CHECK(Parse("[ULTRA DISTANCE = TRUE]\n[SH = 999999]\n[SL = 280000]\n[VR = 20000]",p)==Status::Invalid);
    CHECK(Parse("[ULTRA DISTANCE = TRUE]\n[SH = 40000x]\n[SL = 280000]\n[VR = 20000]",p)==Status::Invalid);
    CHECK(Parse("[ULTRA DISTANCE = TRUE]\n[SH = 40000]\n[SL = 20000]\n[VR = 20000]",p)==Status::Invalid);
    CHECK(Parse("[ULTRA DISTANCE = TRUE]\n[SH = 1000]\n[SL = 10000]\n[CLIP L = -8000]\n[VR = 20000]",p)==Status::Invalid);
    wchar_t temporary[MAX_PATH],directory[MAX_PATH];CHECK(GetTempPathW(MAX_PATH,temporary)&&GetTempFileNameW(temporary,L"edd",0,directory));DeleteFileW(directory);CHECK(CreateDirectoryW(directory,nullptr));
    std::wstring root=directory,ini=root+L"\\Ini",path=ini+L"\\ge3.ini",edd=root+L"\\G3_edd.ini";
    CHECK(CreateDirectoryW(ini.c_str(),nullptr));DWORD error=99;
    CHECK(Apply(root,error)&&error==0); // Ordinary installations remain untouched.
    {std::ofstream f(edd.c_str());f<<source;}
    std::string original="; retain native settings\r\n[Engine.Setup]\r\nEntity.ROI=4000.0\r\nRender.PrefetchGridCellSize=10000\r\nRender.PrefetchGridCellSizeLowPoly=34000\r\n[Option.Sliders]\r\nDistanceHigh.fFarClippingPlane_High=10000.0\r\nDistanceLow.fFarClippingPlaneLowPolyMesh_High=100000.0\r\nVegetationViewRange.fViewDistance_VeryHigh=7500.0\r\nUntouched=hello\r\n";
    {std::ofstream f(path.c_str(),std::ios::binary);f<<original;}
    CHECK(Apply(root,error)&&error==0);
    auto read=[](std::wstring const& file){std::ifstream f(file.c_str(),std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());};
    auto applied=read(path);auto backup=ini+L"\\ge3.before-vr-edd.ini";
    CHECK(read(backup)==original&&applied.find("Entity.ROI=4000.0")!=std::string::npos&&applied.find("Untouched=hello")!=std::string::npos);
    CHECK(GetPrivateProfileIntW(L"Option.Sliders",Keys[3],0,path.c_str())==280000);
    CHECK(GetPrivateProfileIntW(L"Engine.Setup",Keys[0],0,path.c_str())==40000);
    CHECK(Apply(root,error)&&read(path)==applied&&read(backup)==original); // No rewrite each launch.
    {std::ofstream f(edd.c_str());f<<"[ULTRA DISTANCE = TRUE]\n[SH = 999999]";}
    CHECK(!Apply(root,error)&&error==ERROR_INVALID_DATA&&read(path)==applied);
    // EDD may restore its original ini after desktop play. A normal VR launch
    // reapplies only the requested distances, preserving the first backup.
    {std::ofstream f(path.c_str(),std::ios::binary);f<<original;}{std::ofstream f(edd.c_str());f<<source;}
    CHECK(Apply(root,error)&&read(path)==applied&&read(backup)==original);
    for(auto file:{edd,path,backup,path+L".bak"})DeleteFileW(file.c_str());RemoveDirectoryW(ini.c_str());RemoveDirectoryW(root.c_str());
    puts("EDD native distance import, validation, repeated launch, restore and backup PASS");return 0;
}
