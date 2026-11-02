// Offline build tool: assemble only exact locally captured position profiles.
// No runtime shader compilation and no generic bytecode rewriting in game.
#include <windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "TemporalNativeGeometry.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <cstdio>
using Microsoft::WRL::ComPtr;
using diagnostic::temporal::NativeGeometryKind;
void Require(bool b,char const* s){if(!b)throw std::runtime_error(s);}
std::string Previous(std::string const& text,unsigned literalStart){
    std::regex reg(R"(\bc([0-9]+)\b)");std::string result;size_t end=0;
    for(auto i=std::sregex_iterator(text.begin(),text.end(),reg);i!=std::sregex_iterator();++i){auto const& match=*i;result+=text.substr(end,size_t(match.position())-end);unsigned n=std::stoul(match[1]);result+='c'+std::to_string(n>=literalStart?n:n+96);end=size_t(match.position()+match.length());}
    result+=text.substr(end);return result;
}
int wmain(int argc,wchar_t** argv){try{
    bool masked=argc==6&&std::wstring(argv[5])==L"--masked-wind";
    Require(argc==5||masked,"native.cso kind output.h symbol [--masked-wind]");auto kind=NativeGeometryKind(_wtoi(argv[2]));Require(unsigned(kind)<=3&&(!masked||kind==NativeGeometryKind::Wind),"Unknown exact native geometry profile");
    std::ifstream source(std::filesystem::path(argv[1]),std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(source)),{});Require(source&&bytes.size()>16&&bytes.size()<32768,"Captured native shader size");
    uint64_t hash=14695981039346656037ull;for(auto b:bytes){hash^=b;hash*=1099511628211ull;}Require(hash==diagnostic::temporal::NativeGeometryHash(kind),"Captured native shader hash differs from exact profile");
    std::string body,decl="dcl_position v0\n",defs;unsigned literals=0;
    if(kind==NativeGeometryKind::RigidWorld){literals=7;defs="def c7, 1, 0, 0, 0\n";body="dp4 r0.x, v0, c4\ndp4 r0.y, v0, c5\ndp4 r0.z, v0, c6\nmov r0.w, c7.x\n";}
    else if(kind==NativeGeometryKind::InstancesA||kind==NativeGeometryKind::InstancesB){literals=82;defs=kind==NativeGeometryKind::InstancesA?"def c82, 3, -2, 1, 0\n":"def c82, 3, -2, 1, 4\n";
        auto input=kind==NativeGeometryKind::InstancesA?"v6":"v7";decl+="dcl_texcoord4 "+std::string(input)+"\n";
        body="frc r0.x, "+std::string(input)+".x\nadd r0.x, -r0.x, "+input+".x\nmul r0.y, r0.x, c82.x\nmova a0.x, r0.y\ndp4 r0.x, v0, c0[a0.x]\ndp4 r0.y, v0, c1[a0.x]\ndp4 r0.z, v0, c2[a0.x]\nmov r0.w, c82.z\n";
    }else{literals=29;decl+="dcl_texcoord1 v4\n";defs="def c29, 0, 0.159154937, 0.5, 130\ndef c30, 6.28318548, -3.14159274, 0.0076923077, 50\ndef c31, 0.25, 0.75, 1, 0.959999979\n";
        body=R"(if_lt c29.x, v4.x
mov r0.xy, c24
mad r0, v0.zzxx, r0.xyxy, c25.xyxy
mad r0, r0, c29.y, c29.z
frc r0, r0
mad r0, r0, c30.x, c30.y
sincos r1.x, r0.x
mul r0.x, r1.x, c23.x
mov r1.xz, v0
mad r0.x, v4.x, r0.x, r1.z
sincos r2.y, r0.y
mul r0.y, r2.y, c23.y
mad r2.z, v4.x, r0.y, r0.x
sincos r3.y, r0.z
mul r0.x, r3.y, c23.x
mad r0.x, v4.x, r0.x, r1.x
sincos r1.x, r0.w
mul r0.y, r1.x, c23.y
mad r2.x, v4.x, r0.y, r0.x
mov r0.w, c29.w
add r0.x, r0.w, c22.y
mov r1.xz, -c22
mov r1.y, -r0.x
mov r2.y, v0.y
add r0.xyz, r2, r1
dp3 r0.y, r0, r0
rsq r0.y, r0.y
rcp r0.w, r0.y
min r0.w, r0.w, c29.w
add r0.w, -r0.w, c29.w
mul r0.w, r0.w, c30.z
mul r0.xy, r0.xzzw, r0.y
mul r0.xy, r0.w, r0
mad r0.xy, r0, c30.w, r2.xzzw
else
mov r0.xy, v0.xzzw
endif
mov r0.z, v0.y
mov r0.w, c31.z
)";
    }
    unsigned vp=kind==NativeGeometryKind::RigidWorld?0:kind==NativeGeometryKind::Wind?7:68;auto operand=kind==NativeGeometryKind::Wind?"r0.xzyw":"r0";
    std::string now=body;for(unsigned i=0;i<4;++i)now+="dp4 r27."+std::string(1,"xyzw"[i])+", "+operand+", c"+std::to_string(vp+i)+"\n";
    auto before=Previous(now,literals);before=std::regex_replace(before,std::regex(R"(\br27\b)"),"r28");
    std::string materialDecl,material;
    if(masked){
        // Current wind-displaced world position remains in r0.xzy. Mirror
        // native rsq/rcp order and COLOR interpolation, not a linear UV fade.
        decl+="dcl_texcoord0 v3\n";materialDecl="dcl_texcoord3 o4.xy\ndcl_color o5\n";
        material="mov o4.xy, v3\nmov o5.xyz, c29.x\nadd r26.xyz, -r0.xzyw, c18\ndp3 r26.w, r26, r26\nrsq r26.w, r26.w\nrcp r26.w, r26.w\nadd r26.x, -r26.w, c20.x\nmul_sat o5.w, r26.x, c21.x\n";
    }
    std::string validity="mov o3.x, c251.x\n";
    if(kind==NativeGeometryKind::InstancesA||kind==NativeGeometryKind::InstancesB){auto input=kind==NativeGeometryKind::InstancesA?"v6":"v7";defs+="def c250, 0, 17, 0, -1\n";
        validity="frc r0.x, "+std::string(input)+".x\nadd r0.x, -r0.x, "+input+".x\nmova a0.x, r0.x\nmov o3.x, c230[a0.x].x\n";
        before+="if_lt "+std::string(input)+".x, c250.x\nmov r27, c250.zzzw\nendif\nif_ge "+input+".x, c250.y\nmov r27, c250.zzzw\nendif\n";
    }
    std::string text="vs_3_0\n"+defs+decl+"dcl_position o0\ndcl_texcoord0 o1\ndcl_texcoord1 o2\ndcl_texcoord2 o3.x\n"+materialDecl+now+material+before+"mov o0, r27\nmov o1, r27\nmov o2, r28\n"+validity;
    using Assemble=HRESULT(WINAPI*)(void const*,SIZE_T,char const*,D3D_SHADER_MACRO const*,ID3DInclude*,UINT,ID3DBlob**,ID3DBlob**);
    auto compiler=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);Require(compiler!=nullptr,"Offline SM3 assembler DLL missing");
    auto assemble=reinterpret_cast<Assemble>(GetProcAddress(compiler,"D3DAssemble"));Require(assemble!=nullptr,"Offline SM3 assembler entry missing");
    std::ofstream sourceAsm(std::filesystem::path(argv[3]).wstring()+L".asm.txt");sourceAsm<<text;sourceAsm.close();
    ComPtr<ID3DBlob> code,errors;auto result=assemble(text.data(),text.size(),"ExactNativeMotion",nullptr,nullptr,0,&code,&errors);
    if(FAILED(result)){std::fprintf(stderr,"Assembler HRESULT 0x%08x\n",unsigned(result));if(errors)std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);throw std::runtime_error("Native motion assembly failed");}
    auto target=std::filesystem::path(argv[3]);std::ofstream out(target);std::wstring symbolWide=argv[4];std::string symbol;for(wchar_t c:symbolWide){Require(c<=127,"Non-ASCII C++ symbol");symbol.push_back(static_cast<char>(c));}Require(std::regex_match(symbol,std::regex("[A-Za-z_][A-Za-z_0-9]*")),"Invalid C++ symbol");
    out<<"// Offline exact native position profile FNV "<<std::hex<<hash<<"\n#pragma once\ninline constexpr unsigned char "<<symbol<<"[]={\n";auto* data=static_cast<unsigned char*>(code->GetBufferPointer());
    out<<std::dec;for(size_t i=0;i<code->GetBufferSize();++i){if(i%16==0)out<<"\n";out<<unsigned(data[i])<<',';}out<<"\n};\n";Require(bool(out),"Shader header write failed");std::ofstream assembly(target.wstring()+L".asm.txt");assembly<<text;
    std::printf("Native profile%u FNV%016llx companion%zu bytes assembled offline\n",unsigned(kind),static_cast<unsigned long long>(hash),code->GetBufferSize());return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
