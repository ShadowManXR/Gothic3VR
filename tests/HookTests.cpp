#include "VirtualHook.h"
#include "NativeApplication.h"
#include <cstdio>

struct Sample
{
    virtual void First() { ++first; }
    virtual void Second() { ++second; }
    int first = 0;
    int second = 0;
};

diagnostic::VirtualHook hook;
diagnostic::VirtualHook laterHook;
int observed = 0;
using Original = void(__thiscall*)(Sample*);
void __fastcall Observe(Sample* self, void*)
{
    ++observed;
    reinterpret_cast<Original>(hook.original)(self);
}
void __fastcall LaterObserve(Sample* self, void*)
{
    reinterpret_cast<Original>(laterHook.original)(self);
}
__declspec(noinline) void Dispatch(Sample* object) { object->Second(); }

int __cdecl main()
{
    int failures = 0;
    auto check = [&](bool condition, char const* name) {
        std::printf("%s %s\n", condition ? "PASS" : "FAIL", name);
        if (!condition) ++failures;
    };
    wchar_t tempDirectory[MAX_PATH]{}, tempFile[MAX_PATH]{};
    bool tempReady = GetTempPathW(MAX_PATH, tempDirectory) && GetTempFileNameW(tempDirectory, L"g3d", 0, tempFile);
    check(tempReady, "SHA256 fixture path created");
    if (tempReady)
    {
        HANDLE file = CreateFileW(tempFile, GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        DWORD written = 0;
        bool wrote = file != INVALID_HANDLE_VALUE && WriteFile(file, "abc", 3, &written, nullptr) && written == 3;
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        unsigned char digest[32]{};
        unsigned char const expected[32] = {
            0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
            0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
        check(wrote && diagnostic::FileSha256(tempFile, digest) && std::memcmp(digest, expected, 32) == 0,
              "file SHA256 matches known abc vector");
        check(DeleteFileW(tempFile) != FALSE, "SHA256 fixture cleaned up");
        check(!diagnostic::FileSha256(tempFile, digest), "missing EXE file hash rejected");
    }
    auto image = static_cast<unsigned char*>(VirtualAlloc(nullptr, 0x20000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    check(image != nullptr, "native EXE image fixture allocated");
    if (image)
    {
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
        dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 0x80;
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(image + 0x80);
        nt->Signature = IMAGE_NT_SIGNATURE; nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
        nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC; nt->OptionalHeader.SizeOfImage = 0x20000;
        auto nativeTable = reinterpret_cast<void**>(image + diagnostic::NativeVtableRva);
        auto locator = reinterpret_cast<DWORD*>(image + diagnostic::NativeLocatorRva);
        nativeTable[-1] = locator;
        locator[3] = reinterpret_cast<DWORD>(image + 0x1d180);
        std::memcpy(image + 0x1d188, diagnostic::NativeTypeName, sizeof(diagnostic::NativeTypeName));
        size_t nativeSlots[4]{};
        for (unsigned i = 0; i < 4; ++i)
        {
            nativeSlots[i] = diagnostic::NativeSlots[i];
            nativeTable[nativeSlots[i]] = image + diagnostic::NativeCallbackRvas[i];
        }
        check(diagnostic::NativeApplicationMatches(image, nativeTable, nativeSlots, true),
              "native subclass profile with exact table and RTTI accepted");
        check(!diagnostic::NativeApplicationMatches(image, nativeTable, nativeSlots, false),
              "native override requires verified EXE hash");
        check(!diagnostic::NativeApplicationMatches(image, nativeTable + 1, nativeSlots, true),
              "different live application table rejected");
        ++nativeSlots[1];
        check(!diagnostic::NativeApplicationMatches(image, nativeTable, nativeSlots, true), "different SDK slot rejected");
        --nativeSlots[1]; nativeTable[nativeSlots[1]] = reinterpret_cast<void*>(&Observe);
        check(!diagnostic::NativeApplicationMatches(image, nativeTable, nativeSlots, true), "replacement native callback rejected");
        nativeTable[nativeSlots[1]] = image + diagnostic::NativeCallbackRvas[1];
        image[0x1d188] = 'X';
        check(!diagnostic::NativeApplicationMatches(image, nativeTable, nativeSlots, true), "different RTTI class rejected");
        image[0x1d188] = diagnostic::NativeTypeName[0]; locator[1] = 4;
        check(!diagnostic::NativeApplicationMatches(image, nativeTable, nativeSlots, true), "nonzero base adjustment rejected");
        locator[1] = 0; nt->OptionalHeader.SizeOfImage = 0x1000;
        check(!diagnostic::NativeApplicationMatches(image, nativeTable, nativeSlots, true), "native table outside image rejected");
        nt->OptionalHeader.SizeOfImage = 0x20000;
        DWORD oldProtection = 0;
        bool dataOnly = VirtualProtect(image, 0x20000, PAGE_READWRITE, &oldProtection) != FALSE;
        check(dataOnly && !diagnostic::NativeApplicationMatches(image, nativeTable, nativeSlots, true),
              "non-executable native callback rejected");
        VirtualFree(image, 0, MEM_RELEASE);
    }
    size_t slot = 999;
    unsigned char shortThunk[] = {0x8b,0x01,0xff,0x60,0x04};
    unsigned char longThunk[] = {0x8b,0x01,0xff,0xa0,0x00,0x01,0x00,0x00};
    unsigned char applicationThunk[] = {0x8b,0x01,0xff,0xa0,0xc4,0x02,0x00,0x00};
    unsigned char beyondCeiling[] = {0x8b,0x01,0xff,0xa0,0x00,0x08,0x00,0x00};
    unsigned char badAlignment[] = {0x8b,0x01,0xff,0x60,0x03};
    unsigned char unsupported[] = {0x90,0x90,0x90,0x90};
    check(diagnostic::DecodeVirtualSlot(shortThunk, sizeof(shortThunk), slot) && slot == 1,
          "disp8 virtual thunk");
    check(diagnostic::DecodeVirtualSlot(longThunk, sizeof(longThunk), slot) && slot == 64,
          "disp32 virtual thunk");
    check(diagnostic::DecodeVirtualSlot(applicationThunk, sizeof(applicationThunk), slot) && slot == 177,
          "application callback above slot 128 accepted");
    check(!diagnostic::DecodeVirtualSlot(beyondCeiling, sizeof(beyondCeiling), slot),
          "slot beyond sanity ceiling rejected");
    check(!diagnostic::DecodeVirtualSlot(shortThunk, 4, slot), "truncated thunk rejected");
    check(!diagnostic::DecodeVirtualSlot(badAlignment, sizeof(badAlignment), slot), "unaligned slot rejected");
    check(!diagnostic::DecodeVirtualSlot(unsupported, sizeof(unsupported), slot), "unknown code rejected");
    check(!diagnostic::Readable(nullptr, 4), "null memory rejected");
    check(!diagnostic::Executable(&slot), "data is not executable");
    check(diagnostic::ResolveJumpChain(nullptr) == nullptr, "null import chain rejected");
    auto stub = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    check(stub != nullptr, "import stub test allocation");
    if (stub)
    {
        void* target = reinterpret_cast<void*>(&Observe);
        uintptr_t cell = reinterpret_cast<uintptr_t>(&target);
        stub[0] = 0xff; stub[1] = 0x25;
        std::memcpy(stub + 2, &cell, sizeof(cell));
        DWORD oldProtection = 0;
        bool protectedCode = VirtualProtect(stub, 4096, PAGE_EXECUTE_READ, &oldProtection) != FALSE;
        check(protectedCode && diagnostic::ResolveJumpChain(stub) == diagnostic::ResolveJumpChain(target),
              "IAT import thunk resolves to the original function");
        DWORD unused = 0;
        if (VirtualProtect(stub, 4096, PAGE_EXECUTE_READWRITE, &unused))
        {
            std::memset(stub, 0, 4096);
            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(stub);
            dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 0x80;
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(stub + 0x80);
            nt->Signature = IMAGE_NT_SIGNATURE;
            nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
            nt->OptionalHeader.SizeOfImage = 4096;
            nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
            auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
            directory.VirtualAddress = 0x200; directory.Size = 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR);
            auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(stub + 0x200);
            descriptor->OriginalFirstThunk = 0x300; descriptor->FirstThunk = 0x340; descriptor->Name = 0x400;
            auto name = reinterpret_cast<DWORD*>(stub + 0x300); *name = 0x450;
            auto iat = reinterpret_cast<void**>(stub + 0x340); *iat = target;
            strcpy_s(reinterpret_cast<char*>(stub + 0x400), 32, "Engine.dll");
            char const* symbol = "?OnPreRender@eCApplication@@MAEXXZ";
            strcpy_s(reinterpret_cast<char*>(stub + 0x452), 64, symbol);
            auto jump = stub + 0x500;
            jump[0] = 0xff; jump[1] = 0x25;
            uintptr_t namedCell = reinterpret_cast<uintptr_t>(iat);
            std::memcpy(jump + 2, &namedCell, 4);
            void* verified = nullptr;
            check(diagnostic::NamedImportTarget(stub, jump, "Engine.dll", symbol, verified) && verified == target,
                  "named import accepts an interposed callable target");
            check(!diagnostic::NamedImportTarget(stub, jump, "Engine.dll", "?Other@@", verified),
                  "wrong imported method rejected");
            check(!diagnostic::NamedImportTarget(stub, jump, "Other.dll", symbol, verified),
                  "wrong imported DLL rejected");
            namedCell += 4; std::memcpy(jump + 2, &namedCell, 4);
            check(!diagnostic::NamedImportTarget(stub, jump, "Engine.dll", symbol, verified),
                  "unrelated IAT cell rejected");
            namedCell -= 4; std::memcpy(jump + 2, &namedCell, 4);
            *name = 0x2000;
            check(!diagnostic::NamedImportTarget(stub, jump, "Engine.dll", symbol, verified),
                  "import name outside image rejected");
            *name = 0x450; *iat = &slot;
            check(!diagnostic::NamedImportTarget(stub, jump, "Engine.dll", symbol, verified),
                  "non-executable IAT target rejected");
            *iat = target; directory.Size = 1;
            check(!diagnostic::NamedImportTarget(stub, jump, "Engine.dll", symbol, verified),
                  "truncated import descriptor rejected");
            directory.Size = 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR); jump[1] = 0x20;
            check(!diagnostic::NamedImportTarget(stub, jump, "Engine.dll", symbol, verified),
                  "non-IAT jump rejected");
            check(!diagnostic::NamedImportTarget(nullptr, jump, "Engine.dll", symbol, verified),
                  "null PE image rejected");
        }
        else check(false, "import fixture writable protection");
        VirtualFree(stub, 0, MEM_RELEASE);
    }

    Sample object;
    bool decoded = diagnostic::VirtualSlot(&Sample::Second, slot) && slot == 1;
    check(decoded, "real compiler virtual thunk");
    if (!decoded) return 1;
    auto table = *reinterpret_cast<void***>(&object);
    void* saved = table[slot];
    bool ready = hook.Prepare(table, slot, reinterpret_cast<void*>(&Observe));
    check(ready, "live virtual slot prepared");
    if (ready)
    {
        bool installed = hook.Install();
        check(installed, "virtual slot installed and protection restored");
        if (installed)
        {
            Dispatch(&object);
            check(observed == 1 && object.second == 1 && object.first == 0,
                  "fastcall observer chains original thiscall exactly once");
            check(laterHook.Prepare(table, slot, reinterpret_cast<void*>(&LaterObserve)) && laterHook.Install(),
                  "later observer can chain into existing observer");
            check(!hook.Restore() && table[slot] == reinterpret_cast<void*>(&LaterObserve),
                  "restore refuses to overwrite a later hook");
            check(laterHook.Restore(), "later hook restores our observer");
        }
        check(hook.Restore() && table[slot] == saved, "original virtual slot restored");
        Dispatch(&object);
        check(observed == 1 && object.second == 2, "restored dispatch bypasses observer");
    }
    diagnostic::VirtualHook invalid;
    check(!invalid.Prepare(table, diagnostic::MaxVirtualSlots, reinterpret_cast<void*>(&Observe)), "out-of-range slot rejected");
    check(!invalid.Prepare(table, 0, &slot), "non-executable replacement rejected");
    return failures ? 1 : 0;
}
