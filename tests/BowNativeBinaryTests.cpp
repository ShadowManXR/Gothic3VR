#include "NativeApplication.h"
#include "BowCharge.h"
#include <cstdio>
#include <vector>
int __cdecl wmain(int argc,wchar_t** argv){
    if(argc!=2)return 1;
    unsigned char hash[32]{},expected[32]{0x4d,0x29,0x18,0x92,0x81,0xec,0x26,0xea,0xc7,0xc7,0x04,0xfa,0x67,0xed,0x32,0x3d,0xf5,0xc0,0x90,0xd5,0x0b,0x06,0x86,0x26,0xcf,0x60,0x0a,0xec,0xf4,0x26,0x2a,0x24};
    if(!diagnostic::FileSha256(argv[1],hash)||memcmp(hash,expected,32))return 2;
    HANDLE file=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return 3;
    DWORD size=GetFileSize(file,nullptr),read=0;std::vector<unsigned char> raw(size);
    bool ok=ReadFile(file,raw.data(),size,&read,nullptr)&&read==size;CloseHandle(file);if(!ok)return 4;
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(raw.data());auto nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(raw.data()+dos->e_lfanew);auto sections=IMAGE_FIRST_SECTION(nt);
    auto at=[&](unsigned rva,size_t bytes)->unsigned char const*{
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];if(rva>=s.VirtualAddress&&rva-s.VirtualAddress+bytes<=s.SizeOfRawData){size_t offset=s.PointerToRawData+rva-s.VirtualAddress;if(offset+bytes<=raw.size())return raw.data()+offset;}}return nullptr;
    };
    unsigned char prefix[]{0x55,0x8b,0xec,0x83,0xe4,0xf8},finish[]{0xc2,0x10,0};
    auto start=at(0x86450,6),end=at(0x86a0c,3);if(!start||!end||memcmp(start,prefix,6)||memcmp(end,finish,3))return 5;
    auto importName=[&](unsigned cell,char const* dll,char const* name){
        auto directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        for(unsigned off=0;off<directory.Size;off+=sizeof(IMAGE_IMPORT_DESCRIPTOR)){
            auto descriptor=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR const*>(at(directory.VirtualAddress+off,sizeof(IMAGE_IMPORT_DESCRIPTOR)));if(!descriptor||!descriptor->Name)break;
            auto dllText=at(descriptor->Name,strlen(dll)+1);if(!dllText||_stricmp(reinterpret_cast<char const*>(dllText),dll))continue;
            if(cell<descriptor->FirstThunk||(cell-descriptor->FirstThunk)%4)continue;
            auto original=at(descriptor->OriginalFirstThunk+cell-descriptor->FirstThunk,4);if(!original)return false;DWORD nameRva;memcpy(&nameRva,original,4);
            auto text=at(nameRva+2,strlen(name)+1);return text&&!memcmp(text,name,strlen(name)+1);
        }return false;
    };
    if(!importName(0xe4834,"Script.dll","?Shoot@PSProjectile@@QAEXXZ"))return 6;
    // Verify the native callback retains cloning, owner assignment and one-arrow
    // consumption. Tests read the binary; they do not initialize Game DLLs.
    struct Call{unsigned rva,cell;char const* name;};
    Call calls[]{
        {0x8659f,0xe48a0,"?Spawn@Entity@@SG?AV1@VTemplate@@ABVbCMatrix@@@Z"},
        {0x865fe,0xe4828,"?SetOwner@PSInteraction@@QAE_NABVEntity@@@Z"},
        {0x86889,0xe4834,"?Shoot@PSProjectile@@QAEXXZ"},
        {0x8698f,0xe450c,"?DropItemsFromSlot@PSInventory@@QAE?AVEntity@@W4gESlot@@H@Z"}
    };
    for(auto const& call:calls){auto bytes=at(call.rva,6);DWORD absolute=nt->OptionalHeader.ImageBase+call.cell;
        if(!bytes||bytes[0]!=0xff||bytes[1]!=0x15||memcmp(bytes+2,&absolute,4)||!importName(call.cell,"Script.dll",call.name))return 7;
    }
    auto slot=at(0x8697c,4);unsigned char dropOne[]{0x6a,1,0x6a,1};if(!slot||memcmp(slot,dropOne,4))return 8;
    // The callback takes the held ammunition template, rather than selecting a
    // fixed Arrow template. Crossbows leave their native left-weapon/right-bolt
    // slots intact even though VR renders the crossbow in the right palm.
    unsigned char right[]{0x6a,1};auto sourceSlot=at(0x864e7,2);if(!sourceSlot||memcmp(sourceSlot,right,2))return 9;
    if(!importName(0xe4cc0,"Script.dll","?FindStackIndex@PSInventory@@QBEHW4gESlot@@@Z")||
       !importName(0xe4304,"Script.dll","?GetItemFromSlot@PSInventory@@QBE?AVEntity@@W4gESlot@@@Z")||
       !importName(0xe48a4,"Script.dll","?GetTemplate@Entity@@QBE?AVTemplate@@XZ"))return 10;
    // Native reload dispatch selects Cock (43), uses the hero overlay Begin
    // motion, and tests active Master before Advanced crossbow perks.
    auto bytesAt=[&](unsigned rva,std::initializer_list<unsigned char> expected){auto p=at(rva,expected.size());return p&&std::equal(expected.begin(),expected.end(),p);};
    if(!bytesAt(0x85b78,{0xb8,0x2b,0,0,0})||!bytesAt(0x85b8d,{0x6a,4,0x6a,0x2b}))return 11;
    if(!importName(0xe4430,"Script.dll","?GetOverlayAni@Entity@@QBE?AVbCString@@W4gEAction@@W4gEPhase@@@Z")||
       !importName(0xe4280,"Script.dll","?IsSkillActive@PSInventory@@QBE_NABVTemplate@@@Z"))return 12;
    auto textAt=[&](unsigned rva,char const* text){auto p=at(rva,strlen(text)+1);return p&&!memcmp(p,text,strlen(text)+1);};
    if(!textAt(0xfcfd4,"Perk_Crossbow_3")||!textAt(0xfcfe4,"Perk_Crossbow_2")||
       !bytesAt(0x4c54,{0x68,0xd4,0xcf,0x0f,0x10})||!bytesAt(0x4c93,{0x68,0xe4,0xcf,0x0f,0x10}))return 13;
    float master=0,advanced=0;auto a=at(0x107518,4),b=at(0x107638,4);if(!a||!b)return 14;memcpy(&master,a,4);memcpy(&advanced,b,4);
    if(master!=2||advanced!=1.5f||!bytesAt(0x4c86,{0xf3,0x0f,0x10,0x05,0x18,0x75,0x10,0x10})||
       !bytesAt(0x4cb8,{0xf3,0x0f,0x10,0x05,0x38,0x76,0x10,0x10}))return 15;
    // Both native magic projectile paths share this verified import. Native
    // casting obtains the selected spell from the actual right-held item.
    for(unsigned call:{0x4a82du,0x53042u})if(!bytesAt(call,{0xff,0x15,0x34,0x48,0x0e,0x10}))return 16;
    if(!bytesAt(0x7b124,{0x6a,1})||!importName(0xe43b8,"Script.dll","?GetSpell@PSItem@@QBE?AVEntity@@XZ"))return 17;
    if(!bytesAt(0x52f27,{0xff,0x15,0x28,0x48,0x0e,0x10})||!bytesAt(0x52f39,{0xff,0x15,0x68,0x47,0x0e,0x10})||
       !importName(0xe4768,"Script.dll","?SetSpell@PSInteraction@@QAEXABVEntity@@@Z"))return 18;
    // The installed damage bonus handler retains Bow II/III, hunter/race,
    // melee tier/dual-wield/staff and magic-school progression. Physical bow
    // replaces aiming, not this native bonus calculation.
    struct PerkRef{unsigned call,string;char const* name;};
    for(auto const& perk:std::initializer_list<PerkRef>{
        {0x2e63d,0xff250,"Perk_Bow_3"},{0x2e677,0xff25c,"Perk_Bow_2"},
        {0x2e6c4,0xff268,"Perk_Hunter_1"},{0x2e715,0xff278,"Perk_Hunter_2"},
        {0x2e75c,0xff288,"Perk_OrcHunter"},{0x2e777,0xfcfd4,"Perk_Crossbow_3"},
        {0x2e7aa,0xfcfe4,"Perk_Crossbow_2"},{0x2e875,0xfefd4,"Perk_1H1H_2"},
        {0x2e888,0xfefe0,"Perk_1H_3"},{0x2e8ba,0xfefec,"Perk_1H_2"},
        {0x2e8e7,0xff010,"Perk_Staff_3"},{0x2e919,0xff020,"Perk_Staff_2"},
        {0x2e92b,0xfeff8,"Perk_Axe_3"},{0x2e95d,0xff004,"Perk_Axe_2"},
        {0x2e99e,0xff298,"Perk_OrcSlayer"},{0x2e9ea,0xff2a8,"Perk_Paladin"},
        {0x2e565,0xff230,"Perk_Watermage"},{0x2e5b4,0xff240,"Perk_Darkmage"}}){
        auto code=at(perk.call,5);DWORD absolute=nt->OptionalHeader.ImageBase+perk.string;
        if(!code||code[0]!=0x68||memcmp(code+1,&absolute,4)||!textAt(perk.string,perk.name))return 19;
    }
    // CP PowerAim writes charge 0.2 + 0.8 * min(time / 1.95,1).
    // Player Shoot multiplies that charge by 200 for projectile velocity.
    if(!bytesAt(0x84c0f,{0xdc,0x35,0xd0,0x76,0x10,0x10})||
       !bytesAt(0x84c3b,{0xdc,0x0d,0x08,0x75,0x10,0x10})||
       !bytesAt(0x84c51,{0xdc,0x05,0xc8,0x76,0x10,0x10})||
       !importName(0xe4b40,"Script.dll","??4PropertyDamageHitMultiplier@PSDamage@@QAEAAV01@ABM@Z"))return 20;
    diagnostic::bow::ChargeProfile charge;
    struct Constant{unsigned rva;double* destination;};
    for(auto c:std::initializer_list<Constant>{{0x1076d0,&charge.seconds},{0x1076c8,&charge.base},{0x107508,&charge.gain},{0x107418,&charge.velocity}}){auto p=at(c.rva,8);if(!p)return 21;memcpy(c.destination,p,8);}
    if(!charge.Valid()||fabs(charge.seconds-1.95)>.0001||fabs(charge.base-.2)>.0001||fabs(charge.gain-.8)>.0001||charge.velocity!=200)return 22;
    puts("PASS native ranged ABI/ammo/perks and PowerAim charge duration, damage multiplier and linear Shoot velocity constants");return 0;
}
