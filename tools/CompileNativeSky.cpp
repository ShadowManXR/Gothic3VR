// Offline only: preserve the exact native sky program and export its clip
// position in one otherwise unused interpolator. No runtime bytecode rewriting.
#include <windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
using Microsoft::WRL::ComPtr;
void Require(bool b,char const* why){if(!b)throw std::runtime_error(why);}
void ReplaceOnce(std::string& s,std::string const& from,std::string const& to){auto pos=s.find(from);Require(pos!=s.npos&&s.find(from,pos+from.size())==s.npos,"Sky native instruction not unique");s.replace(pos,from.size(),to);}
int wmain(int argc,wchar_t** argv){try{
    Require(argc==3,"CompileNativeSky original.cso output.h");std::ifstream f(std::filesystem::path(argv[1]),std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)),{});Require(f&&bytes.size()>16&&bytes.size()<32768,"Sky shader size");
    uint64_t hash=14695981039346656037ull;for(auto b:bytes){hash^=b;hash*=1099511628211ull;}Require(hash==0x9cd6b9e1f1627a6full,"Unsupported sky bytecode");
    ComPtr<ID3DBlob> listing;Require(SUCCEEDED(D3DDisassemble(bytes.data(),bytes.size(),0,nullptr,&listing)),"Sky disassembly");std::string text(static_cast<char const*>(listing->GetBufferPointer()),listing->GetBufferSize());
    ReplaceOnce(text,"dcl_texcoord7 o9.xyz","dcl_texcoord7 o9.xyz\n    dcl_texcoord8 o10");
    ReplaceOnce(text,"mad o0, r0.x, r1, v0","mad o0, r0.x, r1, v0\n    mad o10, r0.x, r1, v0");
    using Assemble=HRESULT(WINAPI*)(void const*,SIZE_T,char const*,D3D_SHADER_MACRO const*,ID3DInclude*,UINT,ID3DBlob**,ID3DBlob**);
    auto module=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);Require(module!=nullptr,"Offline assembler DLL");auto assemble=reinterpret_cast<Assemble>(GetProcAddress(module,"D3DAssemble"));Require(assemble!=nullptr,"Offline assembler entry");
    ComPtr<ID3DBlob> code,error;auto hr=assemble(text.data(),text.size(),"ExactNativeSky",nullptr,nullptr,0,&code,&error);if(FAILED(hr)){if(error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);throw std::runtime_error("Sky assembly failed");}
    auto destination=std::filesystem::path(argv[2]);std::ofstream out(destination);out<<"#pragma once\nstatic unsigned char const NativeSkyVS[]={\n";auto* p=static_cast<unsigned char const*>(code->GetBufferPointer());for(size_t i=0;i<code->GetBufferSize();++i){out<<unsigned(p[i])<<',';if(i%32==31)out<<'\n';}out<<"};\n";Require(bool(out),"Sky header write");
    std::ofstream assembly(destination.wstring()+L".asm.txt");assembly<<text;Require(bool(assembly),"Sky assembly listing write");std::printf("PASS exact sky companion: native position/material unchanged, extra clip interpolator\n");return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
