#include "NativeApplication.h"
#include <cstdio>
#include <vector>
void __cdecl ShutdownImportFixture() {}

int __cdecl wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 1;
    unsigned char digest[32]{};
    if (!diagnostic::FileSha256(argv[1], digest) || std::memcmp(digest, diagnostic::NativeExeSha256, 32) != 0)
    { std::puts("FAIL real EXE SHA256 profile"); return 1; }
    std::puts("PASS real EXE SHA256 profile");
    HANDLE file = CreateFileW(argv[1], GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 1;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 1024 || size.QuadPart > 16 * 1024 * 1024)
    { CloseHandle(file); return 1; }
    std::vector<unsigned char> raw(static_cast<size_t>(size.QuadPart));
    DWORD bytes = 0;
    bool read = ReadFile(file, raw.data(), static_cast<DWORD>(raw.size()), &bytes, nullptr) && bytes == raw.size();
    CloseHandle(file);
    if (!read) return 1;
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER const*>(raw.data());
    if (dos->e_lfanew < 0 || static_cast<size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > raw.size()) return 1;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS32 const*>(raw.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt->OptionalHeader.SizeOfImage > 16 * 1024 * 1024 || nt->OptionalHeader.SizeOfHeaders > raw.size()) return 1;
    auto sections = IMAGE_FIRST_SECTION(nt);
    if (reinterpret_cast<unsigned char const*>(sections + nt->FileHeader.NumberOfSections) > raw.data() + raw.size()) return 1;
    size_t imageSize = nt->OptionalHeader.SizeOfImage;
    auto image = static_cast<unsigned char*>(VirtualAlloc(nullptr, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!image) return 1;
    // Build a relocated memory image for data inspection only. No game entry
    // point, import initialization, SDK call or callback is executed.
    std::memcpy(image, raw.data(), nt->OptionalHeader.SizeOfHeaders);
    bool valid = true;
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    {
        auto const& s = sections[i];
        if (s.PointerToRawData > raw.size() || s.SizeOfRawData > raw.size() - s.PointerToRawData ||
            s.VirtualAddress > imageSize || s.SizeOfRawData > imageSize - s.VirtualAddress) { valid = false; break; }
        std::memcpy(image + s.VirtualAddress, raw.data() + s.PointerToRawData, s.SizeOfRawData);
    }
    DWORD delta = reinterpret_cast<DWORD>(image) - nt->OptionalHeader.ImageBase;
    auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (directory.VirtualAddress > imageSize || directory.Size > imageSize - directory.VirtualAddress) valid = false;
    size_t offset = 0;
    while (valid && offset < directory.Size)
    {
        if (sizeof(IMAGE_BASE_RELOCATION) > directory.Size - offset) { valid = false; break; }
        auto block = reinterpret_cast<IMAGE_BASE_RELOCATION const*>(image + directory.VirtualAddress + offset);
        if (block->SizeOfBlock < sizeof(*block) || block->SizeOfBlock > directory.Size - offset) { valid = false; break; }
        auto entries = reinterpret_cast<WORD const*>(block + 1);
        size_t count = (block->SizeOfBlock - sizeof(*block)) / 2;
        for (size_t i = 0; i < count; ++i)
        {
            unsigned type = entries[i] >> 12;
            if (type == IMAGE_REL_BASED_ABSOLUTE) continue;
            size_t rva = static_cast<size_t>(block->VirtualAddress) + (entries[i] & 0xfff);
            if (type != IMAGE_REL_BASED_HIGHLOW || rva > imageSize || 4 > imageSize - rva) { valid = false; break; }
            DWORD value = 0; std::memcpy(&value, image + rva, 4); value += delta; std::memcpy(image + rva, &value, 4);
        }
        offset += block->SizeOfBlock;
    }
    for (unsigned i = 0; valid && i < nt->FileHeader.NumberOfSections; ++i)
    {
        auto const& s = sections[i];
        size_t length = s.Misc.VirtualSize > s.SizeOfRawData ? s.Misc.VirtualSize : s.SizeOfRawData;
        if (!length) continue;
        if (s.VirtualAddress > imageSize || length > imageSize - s.VirtualAddress) { valid = false; break; }
        DWORD protection = (s.Characteristics & IMAGE_SCN_MEM_EXECUTE) ? PAGE_EXECUTE_READ :
                           ((s.Characteristics & IMAGE_SCN_MEM_WRITE) ? PAGE_READWRITE : PAGE_READONLY);
        DWORD old = 0;
        valid = VirtualProtect(image + s.VirtualAddress, length, protection, &old) != FALSE;
    }
    size_t slots[4]{};
    std::memcpy(slots, diagnostic::NativeSlots, sizeof(slots));
    auto table = reinterpret_cast<void**>(image + diagnostic::NativeVtableRva);
    bool matched = valid && diagnostic::NativeApplicationMatches(image, table, slots, true);
    std::printf("%s real EXE relocated aCProtoTypeApp table, RTTI and four callbacks\n", matched ? "PASS" : "FAIL");
    bool shutdownMatched=false;
    if(matched)
    {
        auto stub=static_cast<unsigned char*>(table[165]);uintptr_t cell=0;
        if(diagnostic::Readable(stub,6)&&stub[0]==0xff&&stub[1]==0x25)
        {
            std::memcpy(&cell,stub+2,4);DWORD old=0;
            if(cell>=reinterpret_cast<uintptr_t>(image)&&cell+4<=reinterpret_cast<uintptr_t>(image)+imageSize&&
                VirtualProtect(reinterpret_cast<void*>(cell),4,PAGE_READWRITE,&old))
            {
                // Resolve only this mapped fixture's IAT cell to executable test
                // code. No native game import or shutdown callback is executed.
                *reinterpret_cast<void**>(cell)=reinterpret_cast<void*>(&ShutdownImportFixture);
                DWORD unused;VirtualProtect(reinterpret_cast<void*>(cell),4,old,&unused);
                void* target=nullptr;
                shutdownMatched=diagnostic::NamedImportTarget(image,stub,"Game.dll","?OnDestroyEngine@gCGameApp@@MAEXXZ",target)&&
                    target==reinterpret_cast<void*>(&ShutdownImportFixture)&&
                    !diagnostic::NamedImportTarget(image,stub,"Engine.dll","?OnDestroyEngine@gCGameApp@@MAEXXZ",target);
            }
        }
    }
    std::printf("%s real EXE shutdown SDK slot 165 imports Game::OnDestroyEngine\n",shutdownMatched?"PASS":"FAIL");
    bool workspaceMatched=matched&&table[166]==image+0x4160;
    std::printf("%s real EXE workspace callback SDK slot 166 at RVA4160\n",workspaceMatched?"PASS":"FAIL");
    VirtualFree(image, 0, MEM_RELEASE);
    return matched && shutdownMatched && workspaceMatched ? 0 : 1;
}
