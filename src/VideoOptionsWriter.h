#pragma once
#include <windows.h>
#include <string>
#include <cwchar>
#include <cstring>

namespace diagnostic {
inline std::string TrimIni(std::string s) {
    auto first=s.find_first_not_of(" \t\r\n"),last=s.find_last_not_of(" \t\r\n");
    return first==std::string::npos?std::string{}:s.substr(first,last-first+1);
}
inline bool SetVideoNumber(std::string& text,char const* key,int value) {
    bool video=false,found=false;size_t insert=text.size();std::string lineEnding=text.find("\r\n")==std::string::npos?"\n":"\r\n";
    for(size_t start=0;start<text.size();) {
        size_t end=text.find('\n',start);if(end==std::string::npos)end=text.size();
        size_t bodyEnd=end;if(bodyEnd>start&&text[bodyEnd-1]=='\r')--bodyEnd;
        std::string line=text.substr(start,bodyEnd-start),trim=TrimIni(line);
        if(start==0&&trim.compare(0,3,"\xef\xbb\xbf")==0)trim.erase(0,3);
        if(!trim.empty()&&trim.front()=='[') {
            if(video){insert=start;break;}
            video=_stricmp(trim.c_str(),"[Options.Video]")==0;if(video)found=true;
        } else if(video) {
            auto equals=line.find('=');if(equals!=std::string::npos&&_stricmp(TrimIni(line.substr(0,equals)).c_str(),key)==0) {
                // Change this value alone; leave the rest of the document byte-for-byte.
                auto comment=line.find(';',equals+1);std::string suffix=comment==std::string::npos?"":" "+line.substr(comment);
                text.replace(start+equals+1,bodyEnd-(start+equals+1),std::to_string(value)+suffix);return true;
            }
        }
        start=end==text.size()?text.size():end+1;
    }
    if(!found)return false;
    std::string entry=std::string(key)+"="+std::to_string(value)+lineEnding;
    if(insert>0&&text[insert-1]!='\n')entry=lineEnding+entry;text.insert(insert,entry);return true;
}
// Recover Gothic 3's startup copy only when the normal file is absent. The
// game renames UserOptions.ini to __UserOptions.ini itself during initialization.
// Caller must first ensure the game is closed.
inline bool RecoverVideoOptions(wchar_t const* path,DWORD& error) {
    error=0;if(GetFileAttributesW(path)!=INVALID_FILE_ATTRIBUTES)return true;
    if(GetLastError()!=ERROR_FILE_NOT_FOUND){error=GetLastError();return false;}
    std::wstring file=path;auto slash=file.find_last_of(L"\\/");if(slash==std::wstring::npos){error=ERROR_INVALID_NAME;return false;}
    std::wstring candidates[]={file.substr(0,slash+1)+L"__"+file.substr(slash+1),file+L".bak"};
    for(auto const& candidate:candidates){
        if(GetFileAttributesW(candidate.c_str())==INVALID_FILE_ATTRIBUTES)continue;
        if(GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,candidate.c_str())<=0||
           GetPrivateProfileIntW(L"Options.Video",L"Resolution.Height",0,candidate.c_str())<=0)continue;
        if(CopyFileW(candidate.c_str(),path,TRUE))return true;error=GetLastError();return false;
    }
    error=ERROR_FILE_NOT_FOUND;return false;
}
// Write directly and preserve encoding and all unrelated text.
inline bool WriteVideoOptions(wchar_t const* path,int width,int height,DWORD& error) {
    error=0;wchar_t directory[MAX_PATH],temporary[MAX_PATH]{},backup[MAX_PATH];
    if(wcslen(path)>=MAX_PATH-5){error=ERROR_FILENAME_EXCED_RANGE;return false;}
    wcscpy_s(directory,path);auto slash=wcsrchr(directory,L'\\');if(!slash){error=ERROR_INVALID_NAME;return false;}*slash=0;
    HANDLE original=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(original==INVALID_HANDLE_VALUE){error=GetLastError();return false;}
    DWORD size=GetFileSize(original,nullptr),read=0;std::string bytes;
    if(size==INVALID_FILE_SIZE||size>2*1024*1024){error=ERROR_INVALID_DATA;CloseHandle(original);return false;}
    bytes.resize(size);bool ok=ReadFile(original,bytes.data(),size,&read,nullptr)&&read==size;
    if(!ok)error=GetLastError();CloseHandle(original);if(!ok)return false;
    bool utf16=bytes.size()>=2&&static_cast<unsigned char>(bytes[0])==255&&static_cast<unsigned char>(bytes[1])==254;
    std::string text=bytes;
    if(utf16) {
        if(bytes.size()%2){error=ERROR_INVALID_DATA;return false;}
        std::wstring wide((bytes.size()-2)/2,L'\0');memcpy(wide.data(),bytes.data()+2,bytes.size()-2);
        int length=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide.data(),int(wide.size()),nullptr,0,nullptr,nullptr);
        if(!length){error=GetLastError();return false;}text.resize(length);
        WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide.data(),int(wide.size()),text.data(),length,nullptr,nullptr);
    }
    if(!SetVideoNumber(text,"Resolution.Width",width)||!SetVideoNumber(text,"Resolution.Height",height)||
       !SetVideoNumber(text,"WindowMode",0)||!SetVideoNumber(text,"Resolution.RefreshRate",0)){error=ERROR_INVALID_DATA;return false;}
    bytes=text;
    if(utf16) {
        int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);
        if(!length){error=GetLastError();return false;}
        std::wstring wide(length,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),wide.data(),length);
        bytes.assign("\xff\xfe",2);bytes.append(reinterpret_cast<char const*>(wide.data()),wide.size()*sizeof(wchar_t));
    }
    if(!GetTempFileNameW(directory,L"g3r",0,temporary)){error=GetLastError();return false;}
    HANDLE output=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(output==INVALID_HANDLE_VALUE){error=GetLastError();DeleteFileW(temporary);return false;}
    DWORD written=0;ok=WriteFile(output,bytes.data(),DWORD(bytes.size()),&written,nullptr)&&written==bytes.size()&&FlushFileBuffers(output);
    if(!ok)error=GetLastError();CloseHandle(output);
    swprintf_s(backup,L"%s.bak",path);
    if(ok&&!CopyFileW(path,backup,FALSE)){error=GetLastError();ok=false;}
    if(ok&&!ReplaceFileW(path,temporary,nullptr,0,nullptr,nullptr)){error=GetLastError();ok=false;
        // Keep the original usable even for a partial filesystem replace error.
        if(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES)CopyFileW(backup,path,TRUE);
    }
    if(!ok)DeleteFileW(temporary);return ok;
}
}
