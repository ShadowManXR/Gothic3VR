#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>

namespace diagnostic
{
// eCApplication inherits a large window hierarchy; its callbacks are above
// slot 128. This is a sanity ceiling, not the size of a live table. Each slot
// is separately checked for readable memory and matched to a named export.
constexpr size_t MaxVirtualSlots = 512;
inline bool Readable(void const* address, size_t bytes)
{
    if (!address || !bytes) return false;
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(address, &info, sizeof(info)) || info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    DWORD protection = info.Protect & 0xff;
    if (protection != PAGE_READONLY && protection != PAGE_READWRITE && protection != PAGE_WRITECOPY &&
        protection != PAGE_EXECUTE_READ && protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY)
        return false;
    auto first = reinterpret_cast<uintptr_t>(address);
    auto end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    return first < end && bytes <= end - first;
}

inline bool Executable(void const* address)
{
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(address, &info, sizeof(info)) || info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    DWORD protection = info.Protect & 0xff;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

// Imported virtual methods may appear in a derived DLL's table as an IAT jump
// stub. Existing inline detours can add another jump. Resolve only these two
// well-defined x86 forms for comparison with the named engine export.
inline void const* ResolveJumpChain(void const* address)
{
    auto code = reinterpret_cast<unsigned char const*>(address);
    for (unsigned hop = 0; hop < 8; ++hop)
    {
        if (!Readable(code, 6) || !Executable(code)) return nullptr;
        if (code[0] == 0xe9)
        {
            int32_t displacement = 0;
            std::memcpy(&displacement, code + 1, sizeof(displacement));
            code = reinterpret_cast<unsigned char const*>(reinterpret_cast<uintptr_t>(code) + 5 + displacement);
        }
        else if (code[0] == 0xff && code[1] == 0x25)
        {
            uintptr_t cellAddress = 0;
            std::memcpy(&cellAddress, code + 2, sizeof(cellAddress));
            if (!Readable(reinterpret_cast<void const*>(cellAddress), sizeof(void*))) return nullptr;
            std::memcpy(&code, reinterpret_cast<void const*>(cellAddress), sizeof(code));
        }
        else return code;
    }
    return nullptr;
}

// Verify a Game.dll FF25 stub against its PE import names, not just the
// destination address. A mod may replace the named IAT entry with a detour or
// trampoline; chaining through the original stub preserves that behavior.
inline bool NamedImportTarget(void const* image, void const* stub, char const* dll,
                              char const* symbol, void*& target)
{
    target = nullptr;
    if (!Readable(image, sizeof(IMAGE_DOS_HEADER)) || !Readable(stub, 6) || !Executable(stub)) return false;
    auto base = static_cast<unsigned char const*>(image);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER const*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || dos->e_lfanew > 0x100000) return false;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS32 const*>(base + dos->e_lfanew);
    if (!Readable(nt, sizeof(*nt)) || nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_IMPORT) return false;
    size_t imageSize = nt->OptionalHeader.SizeOfImage;
    auto inside = [&](size_t rva, size_t bytes) {
        return rva < imageSize && bytes <= imageSize - rva && Readable(base + rva, bytes);
    };
    auto code = static_cast<unsigned char const*>(stub);
    uintptr_t stubAddress = reinterpret_cast<uintptr_t>(stub), baseAddress = reinterpret_cast<uintptr_t>(image);
    if (stubAddress < baseAddress || !inside(stubAddress - baseAddress, 6) || code[0] != 0xff || code[1] != 0x25)
        return false;
    uintptr_t cell = 0;
    std::memcpy(&cell, code + 2, sizeof(cell));
    auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!directory.VirtualAddress || directory.Size < sizeof(IMAGE_IMPORT_DESCRIPTOR)) return false;
    size_t dllLength = std::strlen(dll) + 1, symbolLength = std::strlen(symbol) + 1;
    for (size_t offset = 0; offset + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= directory.Size && offset < 65536;
         offset += sizeof(IMAGE_IMPORT_DESCRIPTOR))
    {
        size_t descriptorRva = static_cast<size_t>(directory.VirtualAddress) + offset;
        if (!inside(descriptorRva, sizeof(IMAGE_IMPORT_DESCRIPTOR))) return false;
        auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR const*>(base + descriptorRva);
        if (!descriptor->Name) break;
        if (!inside(descriptor->Name, dllLength) ||
            _strnicmp(reinterpret_cast<char const*>(base + descriptor->Name), dll, dllLength) != 0) continue;
        if (!descriptor->OriginalFirstThunk || !descriptor->FirstThunk) return false;
        for (size_t index = 0; index < 4096; ++index)
        {
            size_t nameRva = static_cast<size_t>(descriptor->OriginalFirstThunk) + index * 4;
            size_t cellRva = static_cast<size_t>(descriptor->FirstThunk) + index * 4;
            if (!inside(nameRva, 4) || !inside(cellRva, 4)) return false;
            DWORD name = 0;
            std::memcpy(&name, base + nameRva, 4);
            if (!name) break;
            if (baseAddress + cellRva != cell || IMAGE_SNAP_BY_ORDINAL32(name)) continue;
            if (!inside(static_cast<size_t>(name) + 2, symbolLength) ||
                std::memcmp(base + name + 2, symbol, symbolLength) != 0) return false;
            std::memcpy(&target, base + cellRva, sizeof(target));
            return Executable(target);
        }
    }
    return false;
}

// Decode ONLY MSVC x86's virtual-member dispatch thunk. This is compiler-generated
// code in our DLL, not disassembly of or a byte patch to Gothic 3. Unknown shapes
// are rejected. Slots are derived from the SDK declaration, never hardcoded RVAs.
inline bool DecodeVirtualSlot(unsigned char const* code, size_t length, size_t& slot)
{
    if (length < 4 || code[0] != 0x8b || code[1] != 0x01 || code[2] != 0xff) return false;
    uint32_t offset = 0;
    if (code[3] == 0x20) offset = 0;
    else if (code[3] == 0x60 && length >= 5) offset = code[4];
    else if (code[3] == 0xa0 && length >= 8) std::memcpy(&offset, code + 4, sizeof(offset));
    else return false;
    if (offset % sizeof(void*) != 0 || offset / sizeof(void*) >= MaxVirtualSlots) return false;
    slot = offset / sizeof(void*);
    return true;
}

template<typename Member>
bool VirtualSlot(Member member, size_t& slot)
{
    static_assert(sizeof(void*) == 4, "MSVC x86 required");
    static_assert(sizeof(Member) == sizeof(void*), "Single-inheritance member pointer required");
    unsigned char const* code = nullptr;
    std::memcpy(&code, &member, sizeof(code));
    // Incremental linking may place a jump ahead of the dispatch thunk.
    for (unsigned hop = 0; hop < 4; ++hop)
    {
        if (!Readable(code, 8) || !Executable(code)) return false;
        if (code[0] != 0xe9) return DecodeVirtualSlot(code, 8, slot);
        int32_t displacement = 0;
        std::memcpy(&displacement, code + 1, sizeof(displacement));
        code = code + 5 + displacement;
    }
    return false;
}

struct VirtualHook
{
    void** slot = nullptr;
    void* original = nullptr;
    void* replacement = nullptr;
    bool installed = false;
    DWORD error = ERROR_SUCCESS;
    bool protectionRestored = true;

    bool Prepare(void** table, size_t index, void* target)
    {
        if (installed || index >= MaxVirtualSlots || !Readable(table + index, sizeof(void*)) ||
            !Executable(table[index]) || !Executable(target))
        {
            error = ERROR_INVALID_ADDRESS;
            return false;
        }
        slot = table + index;
        original = *slot;
        replacement = target;
        return true;
    }

    bool Install()
    {
        DWORD oldProtection = 0;
        if (!slot || installed) { error = ERROR_INVALID_PARAMETER; return false; }
        if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtection))
        { error = GetLastError(); return false; }
        void* previous = InterlockedCompareExchangePointer(
            reinterpret_cast<void* volatile*>(slot), replacement, original);
        installed = previous == original;
        DWORD unused = 0;
        protectionRestored = VirtualProtect(slot, sizeof(void*), oldProtection, &unused) != FALSE;
        if (!protectionRestored) error = GetLastError();
        else error = installed ? ERROR_SUCCESS : ERROR_RETRY;
        return installed && protectionRestored;
    }

    bool Restore()
    {
        if (!installed) return true;
        DWORD oldProtection = 0;
        if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtection))
        { error = GetLastError(); return false; }
        void* previous = InterlockedCompareExchangePointer(
            reinterpret_cast<void* volatile*>(slot), original, replacement);
        DWORD unused = 0;
        protectionRestored = VirtualProtect(slot, sizeof(void*), oldProtection, &unused) != FALSE;
        // Never overwrite a later hook installed by another plugin. The caller
        // keeps this module pinned, so a later hook can still chain through us.
        if (previous != replacement) { error = ERROR_RETRY; return false; }
        installed = false;
        error = protectionRestored ? ERROR_SUCCESS : GetLastError();
        return protectionRestored;
    }
};
}
