#pragma once
#include "VirtualHook.h"
#include <bcrypt.h>

namespace diagnostic
{
// Verified against this installation's EXE, its RTTI base hierarchy and the
// callback addresses observed in-game. Only this exact file may use this profile.
constexpr unsigned char NativeExeSha256[32] = {
    0x8d,0x47,0xa0,0x7e,0x11,0x58,0xaf,0xf9,0x41,0xe7,0xaf,0x28,0x21,0x5f,0xc0,0x60,
    0x5e,0xf9,0x54,0x95,0x60,0x04,0xbe,0x93,0x75,0x95,0xcf,0x8a,0xfa,0x5b,0x98,0xb1};
constexpr DWORD NativeVtableRva = 0x1c784;
constexpr DWORD NativeLocatorRva = 0x1d148;
constexpr size_t NativeSlots[4] = {177, 172, 175, 171};
constexpr DWORD NativeCallbackRvas[4] = {0x3e50, 0x3e30, 0x5384, 0x24d0};
constexpr char NativeTypeName[] = ".?AVaCProtoTypeApp@@";

inline bool FileSha256(wchar_t const* path, unsigned char (&digest)[32])
{
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    unsigned char object[4096]{}, buffer[16384]{};
    DWORD objectSize = 0, returned = 0;
    bool success = false;
    do
    {
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) break;
        if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize),
                              sizeof(objectSize), &returned, 0) < 0 || objectSize > sizeof(object)) break;
        if (BCryptCreateHash(algorithm, &hash, object, objectSize, nullptr, 0, 0) < 0) break;
        bool readOk = true;
        for (;;)
        {
            DWORD bytes = 0;
            if (!ReadFile(file, buffer, sizeof(buffer), &bytes, nullptr)) { readOk = false; break; }
            if (!bytes) break;
            if (BCryptHashData(hash, buffer, bytes, 0) < 0) { readOk = false; break; }
        }
        success = readOk && BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    } while (false);
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return success;
}

inline bool NativeApplicationMatches(void const* image, void** table, size_t const (&slots)[4], bool fileMatches)
{
    if (!fileMatches || !Readable(image, sizeof(IMAGE_DOS_HEADER))) return false;
    auto base = static_cast<unsigned char const*>(image);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER const*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || dos->e_lfanew > 0x100000) return false;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS32 const*>(base + dos->e_lfanew);
    if (!Readable(nt, sizeof(*nt)) || nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return false;
    size_t imageSize = nt->OptionalHeader.SizeOfImage;
    auto inside = [&](size_t rva, size_t bytes) {
        return rva < imageSize && bytes <= imageSize - rva && Readable(base + rva, bytes);
    };
    if (reinterpret_cast<void const*>(table) != base + NativeVtableRva ||
        !inside(NativeVtableRva - 4, 4) || !inside(NativeLocatorRva, 20)) return false;
    uintptr_t locatorAddress = 0;
    std::memcpy(&locatorAddress, base + NativeVtableRva - 4, 4);
    if (locatorAddress != reinterpret_cast<uintptr_t>(base + NativeLocatorRva)) return false;
    DWORD locator[5]{};
    std::memcpy(locator, base + NativeLocatorRva, sizeof(locator));
    if (locator[0] != 0 || locator[1] != 0 || locator[3] < reinterpret_cast<uintptr_t>(base)) return false;
    size_t typeRva = locator[3] - reinterpret_cast<uintptr_t>(base);
    if (!inside(typeRva + 8, sizeof(NativeTypeName)) ||
        std::memcmp(base + typeRva + 8, NativeTypeName, sizeof(NativeTypeName)) != 0) return false;
    for (unsigned i = 0; i < 4; ++i)
    {
        if (slots[i] != NativeSlots[i] || !inside(NativeVtableRva + slots[i] * 4, 4) ||
            table[slots[i]] != base + NativeCallbackRvas[i] || !Executable(table[slots[i]])) return false;
    }
    return true;
}
}
