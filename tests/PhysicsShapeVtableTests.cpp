#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <new>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "ObjectModel.h"

static unsigned oracleFreeCount;
struct OracleAllocator {
    static void* __fastcall release(void*, void*, void* p) {
        ++oracleFreeCount;
        free(p);
        return 0;
    }
    static void* __fastcall allocate(void*, void*, unsigned size, unsigned) { return malloc(size); }
};
struct CandidateAllocator : SdkAllocator {
    unsigned freeCount = 0;
    void* malloc(size_t size, NxMemoryType) override { return ::malloc(size); }
    void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType) override {
        return ::malloc(size);
    }
    void* realloc(void* p, size_t size) override { return ::realloc(p, size); }
    void free(void* p) override { ++freeCount; ::free(p); }
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
        fprintf(stderr, "usage: NxPhysicsShapeVtableTests <oracle directory> <sha256>\n");
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
    CandidateAllocator candidateAllocator;
    nxSetSdkAllocatorBridge(&candidateAllocator);
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
    typedef void (__thiscall* DtorSlot)(void*, unsigned);
    reinterpret_cast<DtorSlot>(oracleTable[0])(oracleBytes, 0);
    reinterpret_cast<DtorSlot>(candidateTable[0])(candidateBytes, 0);
    if(oracleFreeCount != 1 || candidateAllocator.freeCount != 1)
        ++failures;
    ++cases;

    unsigned char* oracleHeap = static_cast<unsigned char*>(malloc(0x228));
    unsigned char* candidateHeap = static_cast<unsigned char*>(malloc(0x228));
    if(!oracleHeap || !candidateHeap) return 2;
    memset(oracleHeap, 0xcd, 0x228);
    memset(candidateHeap, 0xcd, 0x228);
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x21870)(oracleHeap, 0, 0);
    new(candidateHeap) BoxShape(0, 0);
    void** oracleHeapTable = *reinterpret_cast<void***>(oracleHeap);
    void** candidateHeapTable = *reinterpret_cast<void***>(candidateHeap);
    reinterpret_cast<DtorSlot>(oracleHeapTable[0])(oracleHeap, 1);
    reinterpret_cast<DtorSlot>(candidateHeapTable[0])(candidateHeap, 1);
    if(oracleFreeCount != 3 || candidateAllocator.freeCount != 3)
        ++failures;
    ++cases;

    // Slot 7 uses the caller's per-axis swept record. Exercise the installed
    // table entries across the same shapes, poses and records as the direct
    // Phase 5 sweep differential.
    typedef bool (__thiscall* SweepSlot)(void*, float*, const float*);
    const float sweepShapes[2][3] = {{1.f,1.5f,2.f},{4.f,65.f,7.f}};
    const float rotations[3][9] = {
        {0,-1,0, 1,0,0, 0,0,1},
        {1,0,0, 0,1,0, 0,0,1},
        {0,0,1, 0,1,0, -1,0,0}
    };
    const float sweepTranslations[3][3] = {{1,2,3},{0,0,0},{0,0,0}};
    const float swept[8][3] = {
        {1,2,3},{2,0,0},{3,5,0},{0,0,0},
        {7,0.5f,100},{-3,-2,-1},{0.25f,-0.5f,4},{-1.5f,2.75f,-6.25f}
    };
    for(unsigned sh = 0; sh < 2; ++sh)
    for(unsigned pose = 0; pose < 3; ++pose) {
        unsigned char o[0x228], c[0x228];
        memset(o, 0xcd, sizeof(o)); memset(c, 0xcd, sizeof(c));
        reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x21870)(o, 0, 0);
        new(c) BoxShape(0, 0);
        memcpy(o + 0xe4, sweepShapes[sh], 12); memcpy(c + 0xe4, sweepShapes[sh], 12);
        memcpy(o + 0x0c, rotations[pose], 36); memcpy(c + 0x0c, rotations[pose], 36);
        memcpy(o + 0x30, sweepTranslations[pose], 12);
        memcpy(c + 0x30, sweepTranslations[pose], 12);
        void** ot = *reinterpret_cast<void***>(o);
        void** ct = *reinterpret_cast<void***>(c);
        for(unsigned sw = 0; sw < 8; ++sw) {
            float oo = 0.5f, co = 0.5f;
            bool ro = reinterpret_cast<SweepSlot>(ot[7])(o, &oo, swept[sw]);
            bool rc = reinterpret_cast<SweepSlot>(ct[7])(c, &co, swept[sw]);
            if(ro != rc || (ro && memcmp(&oo, &co, 4) != 0)) {
                fprintf(stderr, "box slot 7 shape=%u pose=%u sweep=%u differs\n",
                    sh, pose, sw);
                ++failures;
            }
            ++cases;
        }
        reinterpret_cast<DtorSlot>(ot[0])(o, 0);
        reinterpret_cast<DtorSlot>(ct[0])(c, 0);
    }

    // SPHERE slot 7 copies the radius word without interpreting it. Include
    // signed zero, infinity and a NaN payload to pin bitwise preservation.
    unsigned char oracleSphere[0xe4], candidateSphere[0xe4];
    memset(oracleSphere, 0xcd, sizeof(oracleSphere));
    memset(candidateSphere, 0xcd, sizeof(candidateSphere));
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x277c0)(
        oracleSphere, 0, 0);
    SphereShape& sphere = *new(candidateSphere) SphereShape(0, 0);
    typedef bool (__thiscall* SphereSlot7)(void*, float*, const void*);
    SphereSlot7 oracleSphereSlot7 = reinterpret_cast<SphereSlot7>(
        const_cast<unsigned char*>(base) + 0x27c10);
    const unsigned radiusBits[] = {
        0x00000000u, 0x80000000u, 0x3f800000u,
        0xbf800000u, 0x7f800000u, 0x7fc00001u
    };
    for(unsigned bits : radiusBits) {
        memcpy(oracleSphere + 0xe0, &bits, 4);
        memcpy(candidateSphere + 0xe0, &bits, 4);
        unsigned oracleOut = 0xcdcdcdcdu, candidateOut = 0xcdcdcdcdu;
        const void* unread = reinterpret_cast<const void*>(0xdeadbeefu);
        bool ro = oracleSphereSlot7(oracleSphere,
            reinterpret_cast<float*>(&oracleOut), unread);
        bool rc = sphere.nxSphereSweepRadius(
            reinterpret_cast<float*>(&candidateOut), unread);
        if(ro != rc || oracleOut != candidateOut || !ro)
            ++failures;
        ++cases;
    }
    nxSetSdkAllocatorBridge(0);
    printf("shape vtable cases=%u failures=%u\n", cases, failures);
    return failures ? 1 : 0;
}
