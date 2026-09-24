#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <new>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "ObjectModel.h"

struct OracleAllocator {
    static void* __fastcall release(void*, void*, void* p) { free(p); return 0; }
    static void* __fastcall allocate(void*, void*, unsigned size, unsigned) { return malloc(size); }
};
static bool installAllocator(const unsigned char* base) {
    static void* table[6] = {
        reinterpret_cast<void*>(&OracleAllocator::release), 0,
        reinterpret_cast<void*>(&OracleAllocator::allocate),
        reinterpret_cast<void*>(&OracleAllocator::release), 0,
        reinterpret_cast<void*>(&OracleAllocator::release)
    };
    static void* iface[1] = { table };
    unsigned holder = 0;
    memcpy(&holder, base + 0x1041bc, 4);
    if(!holder) return false;
    void* ptr = iface;
    memcpy(reinterpret_cast<void*>(holder), &ptr, 4);
    return true;
}

static bool sha256(const wchar_t* path, char out[65]) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size;
    unsigned char digest[32];
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart > 0 &&
        size.QuadPart < 0x08000000;
    unsigned char* bytes = ok ? static_cast<unsigned char*>(malloc(
        static_cast<size_t>(size.QuadPart))) : 0;
    DWORD read = 0;
    ok = bytes && ReadFile(file, bytes, static_cast<DWORD>(size.QuadPart),
        &read, 0) && read == size.QuadPart &&
        BCryptHash(BCRYPT_SHA256_ALG_HANDLE, 0, 0, bytes, read,
            digest, sizeof(digest)) == 0;
    free(bytes);
    CloseHandle(file);
    if(!ok) return false;
    for(unsigned i = 0; i < 32; ++i)
        sprintf_s(out + 2*i, 3, "%02x", digest[i]);
    return true;
}

// A separate process keeps the Phase 5 layout harness's giant, fragile
// wmain frame unchanged while checking constructed vtable dispatch.
int wmain(int argc, wchar_t** argv)
{
    if(argc != 3) {
        fprintf(stderr, "usage: NxPhysicsBoxVtableTests <oracle directory> <sha256>\n");
        return 2;
    }
    wchar_t path[MAX_PATH];
    if(swprintf_s(path, L"%s\\NxPhysics.dll", argv[1]) < 0)
        return 2;
    char actualHash[65], expectedHash[65];
    size_t converted = 0;
    if(!sha256(path, actualHash) ||
       wcstombs_s(&converted, expectedHash, sizeof(expectedHash), argv[2], _TRUNCATE) ||
       strcmp(actualHash, expectedHash) != 0) {
        fprintf(stderr, "oracle SHA-256 mismatch\n");
        return 2;
    }
    if(!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32) || !AddDllDirectory(argv[1]))
        return 2;
    HMODULE module = LoadLibraryExW(path, 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module) {
        fprintf(stderr, "cannot load oracle: %lu\n", GetLastError());
        return 2;
    }
    const unsigned char* base = reinterpret_cast<const unsigned char*>(module);
    if(!installAllocator(base)) return 2;
    typedef void (__thiscall* BoxCtor)(void*, void*, unsigned);
    typedef void (__thiscall* BoundsSlot)(void*, float*);
    typedef void* (__thiscall* SelfSlot)(void*);
    unsigned failures = 0, cases = 0;
    unsigned char oracleBytes[0x228], candidateBytes[0x228];
    memset(oracleBytes, 0xcd, sizeof(oracleBytes));
    memset(candidateBytes, 0xcd, sizeof(candidateBytes));
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x21870)(oracleBytes, 0, 0);
    new(candidateBytes) BoxShape(0, 0);
    void** oracleTable = *reinterpret_cast<void***>(oracleBytes);
    void** candidateTable = *reinterpret_cast<void***>(candidateBytes);
    for(unsigned slot = 0; slot < 17; ++slot) {
        HMODULE owner = 0;
        BOOL ok = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(candidateTable[slot]), &owner);
        if(!ok || owner != GetModuleHandleW(0)) ++failures;
        ++cases;
    }
    const float dimensions[3][3] = {
        {1.0f, 1.0f, 1.0f}, {2.5f, 3.25f, 4.75f},
        {0.125f, 100.0f, 0.75f}
    };
    for(unsigned i = 0; i < 3; ++i) {
        for(unsigned k = 0; k < 3; ++k) {
            memcpy(oracleBytes + 0xe4 + 4*k, &dimensions[i][k], 4);
            memcpy(candidateBytes + 0xe4 + 4*k, &dimensions[i][k], 4);
            float position = dimensions[i][k] + float(k) * 0.5f;
            memcpy(oracleBytes + 0x30 + 4*k, &position, 4);
            memcpy(candidateBytes + 0x30 + 4*k, &position, 4);
        }
        for(unsigned slot = 10; slot <= 11; ++slot) {
            float oracleOut[4] = {}, candidateOut[4] = {};
            reinterpret_cast<BoundsSlot>(oracleTable[slot])(oracleBytes, oracleOut);
            reinterpret_cast<BoundsSlot>(candidateTable[slot])(candidateBytes, candidateOut);
            if(memcmp(oracleOut, candidateOut, sizeof(oracleOut)) != 0) {
                fprintf(stderr, "box slot %u case %u differs\n", slot, i);
                ++failures;
            }
            ++cases;
        }
    }
    if(reinterpret_cast<SelfSlot>(oracleTable[14])(oracleBytes) != oracleBytes ||
       reinterpret_cast<SelfSlot>(candidateTable[14])(candidateBytes) != candidateBytes)
        ++failures;
    ++cases;
    printf("box vtable cases=%u failures=%u\n", cases, failures);
    return failures ? 1 : 0;
}
