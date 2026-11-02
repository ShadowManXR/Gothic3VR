#pragma once
#include "UnionProtection.h"
#include <fstream>
// Local import only: no third-party executable or artwork is redistributed.
namespace diagnostic::locator {
namespace fs=std::filesystem;
using optional_mods::Require;using optional_mods::NoLinks;using optional_mods::Hash;using optional_mods::Get;using optional_mods::Put;
inline constexpr wchar_t DllSha[]=L"1455D57BA3F5D4A7E18EEC007BF5196E50F574DD410A53E4DEC3341EA5AE48EF";
inline constexpr wchar_t Dll[]=L"scripts/Script_eCDELocator.dll";
inline constexpr wchar_t Ini[]=L"Ini/eCDELocator.ini";
enum GroupIndex {Equipment,Loot,Plants,Interactables,Npcs,Creatures,Corpses,QuestItems,QuestTargets,GroupCount};
inline constexpr wchar_t GroupNames[GroupCount][48]={L"Equipment and ammunition",L"Loot, food and documents",L"Plants (including permanent bonuses)",L"Chests and interactable objects",L"Living NPCs (humans, orcs, undead)",L"Animals and monsters",L"Lootable corpses",L"Quest items (override item filters)",L"Quest targets (override NPC filters)"};
inline constexpr wchar_t const* FilterKeys[]={L"ShowItemMeleeWeapon",L"ShowItemRangedWeapon",L"ShowItemMunition",L"ShowItemArmor",L"ShowItemShield",L"ShowItemHelmet",L"ShowItemRing",L"ShowItemAmulet",L"ShowItemTeleport",L"ShowItemScroll",L"ShowItemMisc",L"ShowItemFood",L"ShowItemPlant",L"ShowItemPlantPerm",L"ShowItemDoc",L"ShowItemPotion",L"ShowItemPotionPerm",L"ShowItemTrophy",L"ShowItemStoneTablet",L"ShowInteractBookstand",L"ShowInteractOre",L"ShowInteractStoneWheel",L"ShowInteractTreasure",L"ShowChest",L"ShowNpcDeadNotEmpty",L"ShowSmallAndFarmAnimals",L"ShowHumanHostile",L"ShowHumanPeaceOrAngry",L"ShowOrcHostile",L"ShowOrcPeaceOrAngry",L"ShowUndeadHostile",L"ShowUndeadPeace",L"ShowMonsterHostile",L"ShowMonsterPeace",L"ShowQuestKillTargets",L"ShowQuestItems"};
inline unsigned Group(unsigned key){if(key<8)return Equipment;if(key==12||key==13)return Plants;if(key<19)return Loot;if(key<24)return Interactables;if(key==24)return Corpses;if(key==25||key==32||key==33)return Creatures;if(key<34)return Npcs;return key==34?QuestTargets:QuestItems;}
struct Settings {bool enabled=false;int distance=3500;bool groups[GroupCount];Settings(){for(auto& group:groups)group=true;}};
#ifdef G3VR_LOCATOR_TESTING
inline fs::path testRoot;
#endif
inline fs::path Root(fs::path const& game){
#ifdef G3VR_LOCATOR_TESTING
 if(!testRoot.empty())return testRoot;
#endif
 return optional_mods::StateRoot(game)/L"Locator";}
inline bool Pending(fs::path const& game){return fs::exists(Root(game)/L"pending.ini");}
inline bool Allowed(fs::path const& relative){auto n=optional_mods::Lower(optional_mods::Relative(relative.wstring()).generic_wstring());
 if(n==L"scripts/script_ecdelocator.dll"||n==L"ini/ecdelocator.ini")return true;
 return n.rfind(L"data/_compiledimage/ecdelocator/icon_",0)==0&&relative.extension()==L".png"&&relative.parent_path().filename()==L"eCDELocator";}
inline void Copy(fs::path const& from,fs::path const& to){NoLinks(from);NoLinks(to);Require(fs::is_regular_file(from),"Locator source file is missing");fs::create_directories(to.parent_path());Require(CopyFileW(from.c_str(),to.c_str(),FALSE)!=FALSE,"Cannot copy locator file");Require(Hash(from)==Hash(to),"Locator copy checksum mismatch");}
inline std::vector<fs::path> Inventory(fs::path const& folder){
 Require(Hash(folder/Dll)==DllSha,"Import the original eCDELocator 1.0.7 download from fyryNy");Require(fs::is_regular_file(folder/Ini),"Locator configuration is missing");
 std::vector<fs::path> names{Dll,Ini};auto icons=folder/L"Data/_compiledImage/eCDELocator";NoLinks(icons);Require(fs::is_directory(icons),"Locator icons folder is missing");
 for(auto const& entry:fs::directory_iterator(icons)){NoLinks(entry.path());Require(entry.is_regular_file(),"Unexpected directory in locator icons");auto relative=entry.path().lexically_relative(folder);Require(Allowed(relative),"Unexpected locator asset");Require(entry.file_size()>8&&entry.file_size()<1024*1024,"Invalid locator icon size");
  std::ifstream png(entry.path(),std::ios::binary);unsigned char signature[8]{};png.read(reinterpret_cast<char*>(signature),8);unsigned char expected[]={137,80,78,71,13,10,26,10};Require(!memcmp(signature,expected,8),"Invalid locator PNG");names.push_back(relative);}
 Require(names.size()==61,"Locator 1.0.7 must include all 59 icons");return names;
}
inline fs::path Package(fs::path const& game){auto folder=Root(game)/L"package";NoLinks(folder);return folder;}
inline bool Imported(fs::path const& game){return fs::is_regular_file(Package(game)/Dll);}
// Import validates everything before replacing an inactive cache. The game is untouched.
inline void Import(fs::path const& game,fs::path const& source){NoLinks(source);Require(!optional_mods::Below(source,game),"Select the extracted download outside the game folder");auto names=Inventory(source);auto root=Root(game);NoLinks(root);fs::create_directories(root);
 auto stage=root/(L"import-"+std::to_wstring(GetTickCount64()));Require(!fs::exists(stage),"Locator import is already pending");
 try{for(auto const& name:names)Copy(source/name,stage/name);Inventory(stage);auto target=Package(game);
  if(fs::exists(target)){Inventory(target); // Do not silently overwrite a different/corrupt download.
   for(auto const& name:names)Require(Hash(source/name)==Hash(target/name),"Cached download differs; keep the original imported files");fs::remove_all(stage);}
  else fs::rename(stage,target);
 }catch(...){if(fs::exists(stage))fs::remove_all(stage);throw;}
}
inline Settings Load(fs::path const& game){Settings s;s.enabled=fs::is_regular_file(game/Dll);auto ini=game/Ini;
 if(fs::is_regular_file(ini)){s.distance=GetPrivateProfileIntW(L"Main",L"SearchDistance",3500,ini.c_str());
  for(unsigned i=0;i<36;++i)s.groups[Group(i)]&=optional_mods::Lower(Get(ini,L"Filter",FilterKeys[i]))!=L"false";}
 return s;
}
inline void Restore(fs::path const& game,fs::path const& session){auto journal=session/L"transaction.ini";auto count=GetPrivateProfileIntW(L"Transaction",L"Count",0,journal.c_str());Require(count>0&&count<=61,"Invalid locator recovery inventory");
 Require(Get(journal,L"Transaction",L"Game")==optional_mods::Full(game),"Locator recovery belongs to a different game");
 // Validate all snapshots before restoring any file.
 for(unsigned i=0;i<count;++i){auto section=L"File"+std::to_wstring(i);auto name=optional_mods::Relative(Get(journal,section.c_str(),L"Path"));Require(Allowed(name),"Unsafe locator recovery path");NoLinks(game/name);if(Get(journal,section.c_str(),L"Existed")==L"1")Require(Hash(session/L"before"/name)==Get(journal,section.c_str(),L"SHA256"),"Damaged locator recovery snapshot");}
 for(unsigned i=0;i<count;++i){auto section=L"File"+std::to_wstring(i);auto name=optional_mods::Relative(Get(journal,section.c_str(),L"Path"));if(Get(journal,section.c_str(),L"Existed")==L"1")Copy(session/L"before"/name,game/name);else if(fs::exists(game/name))Require(DeleteFileW((game/name).c_str())!=FALSE,"Cannot roll back locator file");}
 Put(journal,L"Transaction",L"State",L"RolledBack");
}
inline void Recover(fs::path const& game){auto pointer=Root(game)/L"pending.ini";if(!fs::exists(pointer))return;auto leaf=optional_mods::Relative(Get(pointer,L"Pending",L"Session"));Require(leaf.parent_path().empty(),"Invalid locator session pointer");auto session=Root(game)/leaf;NoLinks(session);
 if(Get(session/L"transaction.ini",L"Transaction",L"State")!=L"Committed")Restore(game,session);
 Require(DeleteFileW(pointer.c_str())!=FALSE,"Cannot clear locator recovery pointer");
}
inline void Apply(fs::path const& game,Settings const& settings){
 Require(settings.distance>=1000&&settings.distance<=5000,"Locator radius must be between 1000 and 5000 game units");NoLinks(game);Require(!optional_mods::Active(game)&&optional_mods::Pending(game).empty(),"Close Union+ and verify VR recovery before changing the locator");
 struct Mutex{HANDLE h;~Mutex(){if(h)CloseHandle(h);}} lock{CreateMutexW(nullptr,FALSE,optional_mods::MutexName(game).c_str())};auto error=GetLastError();Require(lock.h&&error!=ERROR_ALREADY_EXISTS,"A protected mod operation is running");
 auto root=Root(game);NoLinks(root);fs::create_directories(root);Recover(game);auto package=Package(game);std::vector<fs::path> names;
 if(settings.enabled)names=Inventory(package);else if(fs::is_regular_file(game/Dll))names={Dll};else return;
 for(auto const& name:names){NoLinks(game/name);if(fs::exists(game/name)){Require(fs::is_regular_file(game/name),"Locator destination is not a file");if(name!=fs::path(Ini))Require(Hash(game/name)==Hash(package/name),"Existing locator files differ; no files were replaced");
  HANDLE writable=CreateFileW((game/name).c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Require(writable!=INVALID_HANDLE_VALUE,"A locator file is locked or read-only; close the game before applying");CloseHandle(writable);}}
 auto session=root/(L"session-"+std::to_wstring(GetTickCount64()));Require(!fs::exists(session),"Locator transaction is busy");fs::create_directories(session);auto journal=session/L"transaction.ini";auto pointer=root/L"pending.ini";
 Put(journal,L"Transaction",L"Game",optional_mods::Full(game));Put(journal,L"Transaction",L"Count",std::to_wstring(names.size()));
 for(unsigned i=0;i<names.size();++i){auto section=L"File"+std::to_wstring(i);auto name=names[i];Put(journal,section.c_str(),L"Path",name.wstring());bool existed=fs::exists(game/name);Put(journal,section.c_str(),L"Existed",existed?L"1":L"0");if(existed){Copy(game/name,session/L"before"/name);Put(journal,section.c_str(),L"SHA256",Hash(session/L"before"/name));}
  if(settings.enabled)Copy(name==fs::path(Ini)&&existed?game/name:package/name,session/L"after"/name);}
 if(settings.enabled){auto ini=session/L"after"/Ini;Put(ini,L"Main",L"eCDELocatorToggleHotkey",L"F13");Put(ini,L"Main",L"SearchDistance",std::to_wstring(settings.distance));for(unsigned i=0;i<36;++i)Put(ini,L"Filter",FilterKeys[i],settings.groups[Group(i)]?L"true":L"false");}
 Put(journal,L"Transaction",L"State",L"Prepared");Put(pointer,L"Pending",L"Session",session.filename().wstring());
 try{
  for(unsigned i=0;i<names.size();++i){auto section=L"File"+std::to_wstring(i);bool existed=Get(journal,section.c_str(),L"Existed")==L"1";Require(fs::exists(game/names[i])==existed&&(!existed||Hash(game/names[i])==Get(journal,section.c_str(),L"SHA256")),"Locator files changed while preparing; retry with the game closed");}
  if(settings.enabled){ // Install the executable last, only after its configuration/assets exist.
   for(auto const& name:names)if(name!=fs::path(Dll))Copy(session/L"after"/name,game/name);Copy(session/L"after"/Dll,game/Dll);
  }else Require(DeleteFileW((game/Dll).c_str())!=FALSE,"Close Gothic 3 before disabling the locator");
  Put(journal,L"Transaction",L"State",L"Committed");Require(DeleteFileW(pointer.c_str())!=FALSE,"Locator applied; recovery pointer needs cleanup");
 }catch(...){if(Get(journal,L"Transaction",L"State")!=L"Committed"){Restore(game,session);DeleteFileW(pointer.c_str());}throw;}
}
}
