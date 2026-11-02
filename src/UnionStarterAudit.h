#pragma once
#include "UnionProtection.h"
namespace diagnostic::optional_mods {
inline constexpr wchar_t AuditedUnionSha[]=L"3618F86E242BF354A4AF532E7E64BE78449DACEED664DF12EBC6345F1CF5DDE1";
inline constexpr wchar_t CurrentUnionSha[]=L"E805010F1F4E18E9CA7F121A64D1A01272E134E3F7C89A8621196E17CB8C8904";
inline bool AuditedUnionHash(std::wstring const& sha){return sha==AuditedUnionSha||sha==CurrentUnionSha;}
inline fs::path AuditedUnionCache(fs::path const& configured,std::wstring const& sha){
    Require(AuditedUnionHash(sha),"Cannot cache an unverified Union+ executable");
    return configured.parent_path()/(L"UnionStarter_G3VR_"+sha.substr(0,16)+L".exe");
}
inline bool VerifiedUnionFile(fs::path const& file){NoLinks(file);return fs::is_regular_file(file)&&AuditedUnionHash(Hash(file));}
inline fs::path ResolveAuditedUnion(fs::path const& configured){
    if(VerifiedUnionFile(configured))return configured;
    // Keep the verified executable beside Mods, where Union+ expects its data.
    // Its own updater deletes _old.exe on the next start, so that is only a
    // one-time migration source, never our persistent fallback.
    for(auto sha:{CurrentUnionSha,AuditedUnionSha}){
        auto cached=AuditedUnionCache(configured,sha);
        NoLinks(cached);if(fs::is_regular_file(cached)&&Hash(cached)==sha)return cached;
    }
    auto previous=configured.parent_path()/L"UnionStarter_old.exe";NoLinks(previous);
    if(VerifiedUnionFile(previous))return previous;
    throw std::runtime_error("No verified Union+ executable is available. Update Gothic 3 VR for this Union+ version, or select a supported Union+ download. Your VR installation has not been changed.");
}
inline fs::path PrepareAuditedUnion(fs::path const& configured){
    auto selected=ResolveAuditedUnion(configured);auto sha=Hash(selected);auto cached=AuditedUnionCache(configured,sha);NoLinks(cached);
    if(Full(cached)!=Full(selected)&&!fs::exists(cached))
        Require(CopyFileW(selected.c_str(),cached.c_str(),TRUE)!=FALSE,"Cannot preserve the verified Union+ executable beside its Mods folder");
    Require(fs::is_regular_file(cached)&&Hash(cached)==sha,"Verified Union+ copy is damaged; restore it from a supported download");
    return cached;
}
inline void SuspendUnionUpdates(fs::path const& executable,fs::path const& backup){
    auto config=executable.parent_path().parent_path()/L"US.ini";NoLinks(config);auto journal=backup/L"protection.ini";
    // Save evidence before modifying the external INI, so recovery also works
    // if the guard or PC stops during the protected session.
    if(fs::exists(config)){
        auto saved=backup/L"Union-US.ini.before";
        Require(CopyFileW(config.c_str(),saved.c_str(),TRUE)!=FALSE,"Cannot back up Union+ configuration");
        Put(journal,L"Protection",L"UnionConfigSHA256",Hash(saved));
    }
    Put(journal,L"Protection",L"UnionUpdatePolicy",L"1");
    Put(config,L"Main",L"AutomaticUpdateDisabled",L"1");
}
inline void RestoreUnionUpdates(fs::path const& backup){
    auto journal=backup/L"protection.ini";if(Get(journal,L"Protection",L"UnionUpdatePolicy")!=L"1")return;
    fs::path executable=Get(journal,L"Protection",L"ExecutedUnion");Require(!executable.empty(),"Missing protected Union+ path");
    auto config=executable.parent_path().parent_path()/L"US.ini";NoLinks(config);
    auto sha=Get(journal,L"Protection",L"UnionConfigSHA256");std::wstring value;
    bool present=false;
    if(!sha.empty()){
        auto saved=backup/L"Union-US.ini.before";NoLinks(saved);Require(Hash(saved)==sha,"Damaged Union+ configuration backup");
        wchar_t text[128]{};GetPrivateProfileStringW(L"Main",L"AutomaticUpdateDisabled",L"__absent__",text,128,saved.c_str());
        present=wcscmp(text,L"__absent__")!=0;value=text;
    }
    if(present)Put(config,L"Main",L"AutomaticUpdateDisabled",value);
    else Require(WritePrivateProfileStringW(L"Main",L"AutomaticUpdateDisabled",nullptr,config.c_str())!=FALSE,"Cannot restore Union+ update preference");
    Put(journal,L"Protection",L"UnionUpdatePolicy",L"Restored");
}
}
