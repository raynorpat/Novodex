#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <new>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "ObjectModel.h"
#include "ContactGeneration.h"

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

static unsigned renderCount;
static unsigned renderRows[12][16];
static unsigned lineCount;
static unsigned lineRows[8][7];
static unsigned foldOracle(unsigned digest, const void* data, size_t length) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    for(size_t i = 0; i < length; ++i)
        digest = (digest ^ bytes[i]) * 16777619u;
    return digest;
}
static void __fastcall captureLine(void*, void*, const unsigned* start,
    const unsigned* end, unsigned color) {
    if(lineCount < 8) {
        unsigned* row = lineRows[lineCount];
        memcpy(row, start, 12);
        memcpy(row + 3, end, 12);
        row[6] = color;
    }
    ++lineCount;
}
static void __fastcall capturePose(void*, void*, unsigned count,
    const unsigned* pose, unsigned color, unsigned radius, unsigned reserved) {
    if(renderCount < 12) {
        unsigned* row = renderRows[renderCount];
        row[0] = count;
        memcpy(row + 1, pose, 48);
        row[13] = color;
        row[14] = radius;
        row[15] = reserved;
    }
    ++renderCount;
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
    void** oracleSphereTable = *reinterpret_cast<void***>(oracleSphere);
    void** candidateSphereTable = *reinterpret_cast<void***>(candidateSphere);
    unsigned oracleDigest = 2166136261u;
    for(unsigned slot = 0; slot < 19; ++slot) {
        const unsigned rva = static_cast<unsigned>(
            reinterpret_cast<const unsigned char*>(oracleSphereTable[slot]) - base);
        oracleDigest = foldOracle(oracleDigest, &rva, sizeof(rva));
        HMODULE owner = 0;
        BOOL ok = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(candidateSphereTable[slot]), &owner);
        if(!ok || owner != GetModuleHandleW(0)) ++failures;
        ++cases;
    }
    typedef bool (__thiscall* SphereSlot7)(void*, float*, const void*);
    SphereSlot7 oracleSphereSlot7 = reinterpret_cast<SphereSlot7>(oracleSphereTable[7]);
    SphereSlot7 candidateSphereSlot7 = reinterpret_cast<SphereSlot7>(candidateSphereTable[7]);
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
        oracleDigest = foldOracle(oracleDigest, &ro, sizeof(ro));
        oracleDigest = foldOracle(oracleDigest, &oracleOut, sizeof(oracleOut));
        bool rc = candidateSphereSlot7(candidateSphere,
            reinterpret_cast<float*>(&candidateOut), unread);
        if(ro != rc || oracleOut != candidateOut || !ro)
            ++failures;
        ++cases;
    }

    // Isolate the sphere-specific slot-3 arm: A/B remain at their shipped
    // zero values, while guard C toggles against the zero reference word.
    float* guardA = reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x123bc8);
    float* guardB = reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x123bd8);
    float* guardC = reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x123bc4);
    float* scale = reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x123b4c);
    float* ref = reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x1041f0);
    nxBindDebugRenderGuards(guardA, guardB, scale, ref);
    nxBindDebugRenderGuardC(guardC);
    void* rendererTable[16] = {};
    rendererTable[8] = reinterpret_cast<void*>(&captureLine);
    rendererTable[14] = reinterpret_cast<void*>(&capturePose);
    void* rendererObject[1] = { rendererTable };
    typedef void (__thiscall* SphereSlot3)(void*, const void*);
    SphereSlot3 oracleSphereSlot3 = reinterpret_cast<SphereSlot3>(oracleSphereTable[3]);
    SphereSlot3 candidateSphereSlot3 = reinterpret_cast<SphereSlot3>(candidateSphereTable[3]);
    unsigned savedGuardC;
    memcpy(&savedGuardC, guardC, 4);
    DWORD oldProtection, ignoredProtection;
    if(!VirtualProtect(guardC, 4, PAGE_READWRITE, &oldProtection)) return 2;
    const unsigned guardCases[4] = {
        0, 0x80000000u, 0x3f800000u, 0x7fc00000u
    };
    const unsigned radius = 0x40200000u;
    memcpy(oracleSphere + 0xe0, &radius, 4);
    memcpy(candidateSphere + 0xe0, &radius, 4);
    const float rotation[9] = { 1,2,3, 4,5,6, 7,8,9 };
    const float translation[3] = { 4,-2,8 };
    memcpy(oracleSphere + 0x0c, rotation, sizeof(rotation));
    memcpy(candidateSphere + 0x0c, rotation, sizeof(rotation));
    memcpy(oracleSphere + 0x30, translation, sizeof(translation));
    memcpy(candidateSphere + 0x30, translation, sizeof(translation));
    NxShapeRaycastFn oracleRaycast = reinterpret_cast<NxShapeRaycastFn>(oracleSphereTable[5]);
    NxShapeRaycastFn candidateRaycast = reinterpret_cast<NxShapeRaycastFn>(candidateSphereTable[5]);
    const NxRay rays[4] = {
        NxRay(NxVec3(-1,-2,8), NxVec3(1,0,0)),
        NxRay(NxVec3(4,-2,8), NxVec3(1,0,0)),
        NxRay(NxVec3(4,-2,13), NxVec3(0,0,-1)),
        NxRay(NxVec3(4,5,8), NxVec3(0,1,0))
    };
    const float limits[2] = { 2.0f, 10.0f };
    for(unsigned ray = 0; ray < 4; ++ray)
    for(unsigned limit = 0; limit < 2; ++limit)
    for(unsigned normal = 0; normal < 2; ++normal) {
        NxRaycastHit oracleHit, candidateHit;
        memset(&oracleHit, 0xcd, sizeof(oracleHit));
        memset(&candidateHit, 0xcd, sizeof(candidateHit));
        const unsigned flags = normal ? NX_RAYCAST_NORMAL : 0u;
        const NxCollisionShape* ro = oracleRaycast(
            reinterpret_cast<const NxCollisionShape*>(oracleSphere),
            &rays[ray], limits[limit], 0, flags, &oracleHit);
        const NxCollisionShape* rc = candidateRaycast(
            reinterpret_cast<const NxCollisionShape*>(candidateSphere),
            &rays[ray], limits[limit], 0, flags, &candidateHit);
        if(ro) oracleHit.shape = reinterpret_cast<NxShape*>(0x12345678u);
        if(rc) candidateHit.shape = reinterpret_cast<NxShape*>(0x12345678u);
        const unsigned oraclePresent = ro != 0;
        oracleDigest = foldOracle(oracleDigest, &oraclePresent, sizeof(oraclePresent));
        oracleDigest = foldOracle(oracleDigest, &oracleHit, sizeof(oracleHit));
        if(bool(ro) != bool(rc) ||
           memcmp(&oracleHit, &candidateHit, sizeof(oracleHit)) != 0) {
            fprintf(stderr, "sphere slot 5 ray=%u limit=%u normal=%u differs\n",
                ray, limit, normal);
            ++failures;
        }
        ++cases;
    }
    for(unsigned slot = 8; slot <= 11; ++slot) {
        float oracleOut[6], candidateOut[6];
        memset(oracleOut, 0xcd, sizeof(oracleOut));
        memset(candidateOut, 0xcd, sizeof(candidateOut));
        reinterpret_cast<BoundsSlot>(oracleSphereTable[slot])(
            oracleSphere, oracleOut);
        reinterpret_cast<BoundsSlot>(candidateSphereTable[slot])(
            candidateSphere, candidateOut);
        if(memcmp(oracleOut, candidateOut, sizeof(oracleOut)) != 0) {
            fprintf(stderr, "sphere slot %u bounds differ\n", slot);
            ++failures;
        }
        ++cases;
    }
    typedef float (__thiscall* RadiusGetter)(void*);
    float oracleRadius = reinterpret_cast<RadiusGetter>(oracleSphereTable[15])(
        oracleSphere);
    float candidateRadius = reinterpret_cast<RadiusGetter>(candidateSphereTable[15])(
        candidateSphere);
    if(memcmp(&oracleRadius, &candidateRadius, 4) != 0) ++failures;
    ++cases;
    for(unsigned slot = 16; slot <= 18; ++slot) {
        if(reinterpret_cast<SelfSlot>(oracleSphereTable[slot])(oracleSphere) != oracleSphere ||
           reinterpret_cast<SelfSlot>(candidateSphereTable[slot])(candidateSphere) != candidateSphere)
            ++failures;
        ++cases;
    }
    for(unsigned enabled = 0; enabled < 2; ++enabled)
    for(unsigned low = 0; low < 8; ++low)
    for(unsigned g = 0; g < 4; ++g) {
        const unsigned short flags = static_cast<unsigned short>((enabled ? 8u : 0u) | low);
        memcpy(oracleSphere + 0xde, &flags, 2);
        memcpy(candidateSphere + 0xde, &flags, 2);
        memcpy(guardC, guardCases + g, 4);
        renderCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        oracleSphereSlot3(oracleSphere, rendererObject);
        const unsigned oracleCount = renderCount;
        unsigned oracleRows[12][16];
        memcpy(oracleRows, renderRows, sizeof(oracleRows));
        oracleDigest = foldOracle(oracleDigest, &oracleCount, sizeof(oracleCount));
        oracleDigest = foldOracle(oracleDigest, oracleRows, sizeof(oracleRows));
        renderCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        candidateSphereSlot3(candidateSphere, rendererObject);
        const unsigned expectedCount = enabled && g >= 2 ? 3u : 0u;
        if(oracleCount != expectedCount || oracleCount != renderCount ||
           memcmp(oracleRows, renderRows, sizeof(renderRows)) != 0) {
            fprintf(stderr, "sphere slot 3 enabled=%u low=%u guard=%u count=%u/%u differs\n",
                enabled, low, g, oracleCount, renderCount);
            ++failures;
        }
        ++cases;
    }
    unsigned savedA, savedB, savedScale;
    memcpy(&savedA, guardA, 4);
    memcpy(&savedB, guardB, 4);
    memcpy(&savedScale, scale, 4);
    const unsigned one = 0x3f800000u;
    memcpy(scale, &one, 4);
    for(unsigned mask = 0; mask < 4; ++mask)
    for(unsigned cGuard = 0; cGuard < 2; ++cGuard)
    for(unsigned low = 0; low < 2; ++low) {
        const unsigned zero = 0;
        memcpy(guardA, mask & 1 ? &one : &zero, 4);
        memcpy(guardB, mask & 2 ? &one : &zero, 4);
        memcpy(guardC, cGuard ? &one : &zero, 4);
        const unsigned short flags = static_cast<unsigned short>(8u | low);
        memcpy(oracleSphere + 0xde, &flags, 2);
        memcpy(candidateSphere + 0xde, &flags, 2);
        renderCount = lineCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        memset(lineRows, 0xcd, sizeof(lineRows));
        oracleSphereSlot3(oracleSphere, rendererObject);
        const unsigned oraclePoseCount = renderCount;
        const unsigned oracleLineCount = lineCount;
        unsigned oraclePoseRows[12][16], oracleLineRows[8][7];
        memcpy(oraclePoseRows, renderRows, sizeof(renderRows));
        memcpy(oracleLineRows, lineRows, sizeof(lineRows));
        renderCount = lineCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        memset(lineRows, 0xcd, sizeof(lineRows));
        candidateSphereSlot3(candidateSphere, rendererObject);
        const unsigned expectedLines = (mask & 1) ? 3u : 0u;
        const unsigned expectedPoses = ((mask & 2) ? 3u : 0u) +
            (cGuard ? 3u : 0u);
        if(oraclePoseCount != expectedPoses || renderCount != oraclePoseCount ||
           oracleLineCount != expectedLines || lineCount != oracleLineCount ||
           memcmp(oraclePoseRows, renderRows, sizeof(renderRows)) != 0 ||
           memcmp(oracleLineRows, lineRows, sizeof(lineRows)) != 0) {
            fprintf(stderr, "sphere shared render mask=%u c=%u low=%u lines=%u/%u poses=%u/%u differs\n",
                mask, cGuard, low, oracleLineCount, lineCount,
                oraclePoseCount, renderCount);
            ++failures;
        }
        ++cases;
    }
    memcpy(guardA, &savedA, 4);
    memcpy(guardB, &savedB, 4);
    memcpy(scale, &savedScale, 4);
    memcpy(guardC, &savedGuardC, 4);
    VirtualProtect(guardC, 4, oldProtection, &ignoredProtection);
    unsigned oracleBefore = oracleFreeCount;
    unsigned candidateBefore = candidateAllocator.freeCount;
    reinterpret_cast<DtorSlot>(oracleSphereTable[0])(oracleSphere, 0);
    reinterpret_cast<DtorSlot>(candidateSphereTable[0])(candidateSphere, 0);
    if(oracleFreeCount - oracleBefore != 1 ||
       candidateAllocator.freeCount - candidateBefore != 1)
        ++failures;
    ++cases;
    unsigned char* oracleSphereHeap = static_cast<unsigned char*>(malloc(0xe4));
    unsigned char* candidateSphereHeap = static_cast<unsigned char*>(malloc(0xe4));
    if(!oracleSphereHeap || !candidateSphereHeap) return 2;
    memset(oracleSphereHeap, 0xcd, 0xe4);
    memset(candidateSphereHeap, 0xcd, 0xe4);
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x277c0)(
        oracleSphereHeap, 0, 0);
    new(candidateSphereHeap) SphereShape(0, 0);
    void** oracleHeapSphereTable = *reinterpret_cast<void***>(oracleSphereHeap);
    void** candidateHeapSphereTable = *reinterpret_cast<void***>(candidateSphereHeap);
    oracleBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    reinterpret_cast<DtorSlot>(oracleHeapSphereTable[0])(oracleSphereHeap, 1);
    reinterpret_cast<DtorSlot>(candidateHeapSphereTable[0])(candidateSphereHeap, 1);
    if(oracleFreeCount - oracleBefore != 2 ||
       candidateAllocator.freeCount - candidateBefore != 2)
        ++failures;
    ++cases;

    // CAPSULE slot 7 writes a zero dword and returns false. A poisoned
    // second argument tests that the oracle does not read the sweep record.
    unsigned char oracleCapsule[0xec], candidateCapsule[0xec];
    memset(oracleCapsule, 0xcd, sizeof(oracleCapsule));
    memset(candidateCapsule, 0xcd, sizeof(candidateCapsule));
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x21a60)(
        oracleCapsule, 0, 0);
    CapsuleShape& capsule = *new(candidateCapsule) CapsuleShape(0, 0);
    void** oracleCapsuleTable = *reinterpret_cast<void***>(oracleCapsule);
    void** candidateCapsuleTable = *reinterpret_cast<void***>(candidateCapsule);
    HMODULE capsuleTableOwner = 0;
    const BOOL capsuleTableMapped = GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(candidateCapsuleTable), &capsuleTableOwner);
    const bool capsuleTableInstalled = capsuleTableMapped &&
        capsuleTableOwner == GetModuleHandleW(0);
    if(!capsuleTableInstalled) ++failures;
    ++cases;
    if(capsuleTableInstalled)
    for(unsigned slot = 0; slot < 19; ++slot) {
        HMODULE owner = 0;
        const BOOL ok = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(candidateCapsuleTable[slot]), &owner);
        if(!ok || owner != GetModuleHandleW(0)) ++failures;
        ++cases;
    }
    typedef bool (__thiscall* CapsuleSweepSlot)(void*, unsigned*, const void*);
    CapsuleSweepSlot oracleCapsuleSweep =
        reinterpret_cast<CapsuleSweepSlot>(oracleCapsuleTable[7]);
    const unsigned capsuleSeeds[] = {
        0u, 0xffffffffu, 0x7fc00001u, 0xdeadbeefu
    };
    for(unsigned seed : capsuleSeeds) {
        unsigned oracleOut = seed, candidateOut = seed;
        const void* unread = reinterpret_cast<const void*>(0xdeadbeefu);
        const bool ro = oracleCapsuleSweep(oracleCapsule, &oracleOut, unread);
        const bool rc = capsuleTableInstalled
            ? reinterpret_cast<CapsuleSweepSlot>(candidateCapsuleTable[7])(
                candidateCapsule, &candidateOut, unread)
            : capsule.nxCapsuleSweepZero(&candidateOut, unread);
        oracleDigest = foldOracle(oracleDigest, &ro, sizeof(ro));
        oracleDigest = foldOracle(oracleDigest, &oracleOut, sizeof(oracleOut));
        if(ro != rc || oracleOut != candidateOut || ro || oracleOut != 0)
            ++failures;
        ++cases;
    }
    const NxRay capsuleRays[4] = {
        NxRay(NxVec3(-3,0,0), NxVec3(1,0,0)),
        NxRay(NxVec3(0,0,0), NxVec3(1,0,0)),
        NxRay(NxVec3(0,4,0), NxVec3(0,-1,0)),
        NxRay(NxVec3(3,3,3), NxVec3(1,0,0))
    };
    const float capsuleRadius = 0.5f, capsuleHalfHeight = 1.0f;
    memcpy(oracleCapsule + 0xe0, &capsuleRadius, 4);
    memcpy(oracleCapsule + 0xe4, &capsuleHalfHeight, 4);
    capsule.mFloatE0 = capsuleRadius;
    capsule.mFloatE4 = capsuleHalfHeight;
    NxShapeRaycastFn oracleCapsuleRaycast =
        reinterpret_cast<NxShapeRaycastFn>(oracleCapsuleTable[5]);
    for(unsigned ray = 0; ray < 4; ++ray)
    for(unsigned limit = 0; limit < 2; ++limit)
    for(unsigned normal = 0; normal < 2; ++normal) {
        NxRaycastHit oracleHit, candidateHit;
        memset(&oracleHit, 0xcd, sizeof(oracleHit));
        memset(&candidateHit, 0xcd, sizeof(candidateHit));
        const unsigned flags = normal ? NX_RAYCAST_NORMAL : 0u;
        const NxCollisionShape* ro = oracleCapsuleRaycast(
            reinterpret_cast<const NxCollisionShape*>(oracleCapsule),
            &capsuleRays[ray], limits[limit], 0, flags, &oracleHit);
        const NxCollisionShape* rc = capsuleTableInstalled
            ? reinterpret_cast<NxShapeRaycastFn>(candidateCapsuleTable[5])(
                reinterpret_cast<const NxCollisionShape*>(candidateCapsule),
                &capsuleRays[ray], limits[limit], 0, flags, &candidateHit)
            : NxShapeRaycastCapsule(
                reinterpret_cast<const NxCollisionShape*>(candidateCapsule),
                nullptr, &capsuleRays[ray], limits[limit], 0, flags,
                &candidateHit);
        if(ro) oracleHit.shape = reinterpret_cast<NxShape*>(0x12345678u);
        if(rc) candidateHit.shape = reinterpret_cast<NxShape*>(0x12345678u);
        const unsigned oraclePresent = ro != 0;
        oracleDigest = foldOracle(oracleDigest, &oraclePresent, sizeof(oraclePresent));
        oracleDigest = foldOracle(oracleDigest, &oracleHit, sizeof(oracleHit));
        if(bool(ro) != bool(rc) ||
           memcmp(&oracleHit, &candidateHit, sizeof(oracleHit)) != 0) {
            fprintf(stderr, "capsule slot 5 ray=%u limit=%u normal=%u differs\n",
                ray, limit, normal);
            ++failures;
        }
        ++cases;
    }
    unsigned capSaveA, capSaveB, capSaveC;
    memcpy(&capSaveA, guardA, 4); memcpy(&capSaveB, guardB, 4);
    memcpy(&capSaveC, guardC, 4);
    DWORD capOldProtection, capIgnoredProtection;
    if(!VirtualProtect(guardC, 4, PAGE_READWRITE, &capOldProtection)) return 2;
    const unsigned capZero = 0, capOne = 0x3f800000u;
    memcpy(guardA, &capZero, 4); memcpy(guardB, &capZero, 4);
    const float capRotations[2][9] = {
        {1,0,0, 0,1,0, 0,0,1},
        {0,-1,0, 1,0,0, 0,0,1}
    };
    const float capTranslations[2][3] = {{0,0,0},{3,4,5}};
    for(unsigned pose = 0; pose < 2; ++pose)
    for(unsigned low = 0; low < 2; ++low)
    for(unsigned enabled = 0; enabled < 2; ++enabled)
    for(unsigned cGuard = 0; cGuard < 2; ++cGuard) {
        memcpy(oracleCapsule + 0x0c, capRotations[pose], 36);
        memcpy(candidateCapsule + 0x0c, capRotations[pose], 36);
        memcpy(oracleCapsule + 0x30, capTranslations[pose], 12);
        memcpy(candidateCapsule + 0x30, capTranslations[pose], 12);
        const unsigned short flags = static_cast<unsigned short>(
            low | (enabled ? 8u : 0u));
        memcpy(oracleCapsule + 0xde, &flags, 2);
        memcpy(candidateCapsule + 0xde, &flags, 2);
        memcpy(guardC, cGuard ? &capOne : &capZero, 4);
        renderCount = lineCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        memset(lineRows, 0xcd, sizeof(lineRows));
        reinterpret_cast<SphereSlot3>(oracleCapsuleTable[3])(
            oracleCapsule, rendererObject);
        const unsigned oraclePoseCount = renderCount;
        const unsigned oracleLineCount = lineCount;
        unsigned oraclePoseRows[12][16], oracleLineRows[8][7];
        memcpy(oraclePoseRows, renderRows, sizeof(oraclePoseRows));
        memcpy(oracleLineRows, lineRows, sizeof(oracleLineRows));
        oracleDigest = foldOracle(oracleDigest, &oraclePoseCount,
            sizeof(oraclePoseCount));
        oracleDigest = foldOracle(oracleDigest, &oracleLineCount,
            sizeof(oracleLineCount));
        oracleDigest = foldOracle(oracleDigest, oraclePoseRows,
            sizeof(oraclePoseRows));
        oracleDigest = foldOracle(oracleDigest, oracleLineRows,
            sizeof(oracleLineRows));
        renderCount = lineCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        memset(lineRows, 0xcd, sizeof(lineRows));
        if(capsuleTableInstalled)
            reinterpret_cast<SphereSlot3>(candidateCapsuleTable[3])(
                candidateCapsule, rendererObject);
        else
            capsule.nxCapsuleDebugRenderDispatch(rendererObject);
        const unsigned expected = enabled && cGuard ? 1u : 0u;
        if(oraclePoseCount != 6u * expected ||
           oracleLineCount != 4u * expected ||
           renderCount != oraclePoseCount || lineCount != oracleLineCount ||
           memcmp(oraclePoseRows, renderRows, sizeof(renderRows)) != 0 ||
           memcmp(oracleLineRows, lineRows, sizeof(lineRows)) != 0) {
            fprintf(stderr,
                "capsule slot 3 pose=%u low=%u enabled=%u c=%u lines=%u/%u poses=%u/%u differs\n",
                pose, low, enabled, cGuard, oracleLineCount, lineCount,
                oraclePoseCount, renderCount);
            ++failures;
        }
        ++cases;
    }
    unsigned capSaveScale;
    memcpy(&capSaveScale, scale, 4);
    memcpy(scale, &capOne, 4);
    memcpy(oracleCapsule + 0x0c, capRotations[1], 36);
    memcpy(candidateCapsule + 0x0c, capRotations[1], 36);
    memcpy(oracleCapsule + 0x30, capTranslations[1], 12);
    memcpy(candidateCapsule + 0x30, capTranslations[1], 12);
    for(unsigned a = 0; a < 2; ++a)
    for(unsigned b = 0; b < 2; ++b)
    for(unsigned c = 0; c < 2; ++c)
    for(unsigned low = 0; low < 2; ++low) {
        memcpy(guardA, a ? &capOne : &capZero, 4);
        memcpy(guardB, b ? &capOne : &capZero, 4);
        memcpy(guardC, c ? &capOne : &capZero, 4);
        const unsigned short flags = static_cast<unsigned short>(8u | low);
        memcpy(oracleCapsule + 0xde, &flags, 2);
        memcpy(candidateCapsule + 0xde, &flags, 2);
        renderCount = lineCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        memset(lineRows, 0xcd, sizeof(lineRows));
        reinterpret_cast<SphereSlot3>(oracleCapsuleTable[3])(
            oracleCapsule, rendererObject);
        const unsigned oraclePoseCount = renderCount;
        const unsigned oracleLineCount = lineCount;
        unsigned oraclePoseRows[12][16], oracleLineRows[8][7];
        memcpy(oraclePoseRows, renderRows, sizeof(oraclePoseRows));
        memcpy(oracleLineRows, lineRows, sizeof(oracleLineRows));
        oracleDigest = foldOracle(oracleDigest, &oraclePoseCount,
            sizeof(oraclePoseCount));
        oracleDigest = foldOracle(oracleDigest, &oracleLineCount,
            sizeof(oracleLineCount));
        oracleDigest = foldOracle(oracleDigest, oraclePoseRows,
            sizeof(oraclePoseRows));
        oracleDigest = foldOracle(oracleDigest, oracleLineRows,
            sizeof(oracleLineRows));
        renderCount = lineCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        memset(lineRows, 0xcd, sizeof(lineRows));
        if(capsuleTableInstalled)
            reinterpret_cast<SphereSlot3>(candidateCapsuleTable[3])(
                candidateCapsule, rendererObject);
        else
            capsule.nxCapsuleDebugRenderDispatch(rendererObject);
        if(oraclePoseCount != 3u*b + 6u*c ||
           oracleLineCount != 3u*a + 4u*c ||
           renderCount != oraclePoseCount || lineCount != oracleLineCount ||
           memcmp(oraclePoseRows, renderRows, sizeof(renderRows)) != 0 ||
           memcmp(oracleLineRows, lineRows, sizeof(lineRows)) != 0) {
            fprintf(stderr,
                "capsule shared render a=%u b=%u c=%u low=%u lines=%u/%u poses=%u/%u differs\n",
                a, b, c, low, oracleLineCount, lineCount,
                oraclePoseCount, renderCount);
            ++failures;
        }
        ++cases;
    }
    memcpy(scale, &capSaveScale, 4);
    memcpy(guardA, &capSaveA, 4); memcpy(guardB, &capSaveB, 4);
    memcpy(guardC, &capSaveC, 4);
    VirtualProtect(guardC, 4, capOldProtection, &capIgnoredProtection);
    const unsigned capsuleRadiusBits[] = {
        0x00000000u, 0x80000000u, 0x3f000000u,
        0xbf800000u, 0x7f800000u, 0x7fc00001u
    };
    for(unsigned bits : capsuleRadiusBits) {
        memcpy(oracleCapsule + 0xe0, &bits, 4);
        memcpy(candidateCapsule + 0xe0, &bits, 4);
        const float ro = reinterpret_cast<RadiusGetter>(oracleCapsuleTable[15])(
            oracleCapsule);
        const float rc = capsuleTableInstalled
            ? reinterpret_cast<RadiusGetter>(candidateCapsuleTable[15])(
                candidateCapsule)
            : capsule.nxCapsuleGetRadius();
        oracleDigest = foldOracle(oracleDigest, &ro, sizeof(ro));
        if(memcmp(&ro, &rc, 4) != 0) ++failures;
        ++cases;
    }
    const float capsuleDimensions[2][2] = {{0.5f,1.0f},{1.75f,3.0f}};
    for(unsigned dim = 0; dim < 2; ++dim) {
        memcpy(oracleCapsule + 0xe0, capsuleDimensions[dim], 8);
        memcpy(candidateCapsule + 0xe0, capsuleDimensions[dim], 8);
        for(unsigned slot = 8; slot <= 11; ++slot) {
            float oracleOut[6], candidateOut[6];
            const float boundsSeed[6] = {100,100,100,-100,-100,-100};
            if(slot == 9) {
                memcpy(oracleOut, boundsSeed, sizeof(boundsSeed));
                memcpy(candidateOut, boundsSeed, sizeof(boundsSeed));
            } else {
                memset(oracleOut, 0xcd, sizeof(oracleOut));
                memset(candidateOut, 0xcd, sizeof(candidateOut));
            }
            reinterpret_cast<BoundsSlot>(oracleCapsuleTable[slot])(
                oracleCapsule, oracleOut);
            reinterpret_cast<BoundsSlot>(candidateCapsuleTable[slot])(
                candidateCapsule, candidateOut);
            oracleDigest = foldOracle(oracleDigest, oracleOut, sizeof(oracleOut));
            if(memcmp(oracleOut, candidateOut, sizeof(oracleOut)) != 0) {
                fprintf(stderr, "capsule slot %u dim=%u differs\n", slot, dim);
                ++failures;
            }
            ++cases;
        }
    }
    for(unsigned slot = 16; slot <= 18; ++slot) {
        if(reinterpret_cast<SelfSlot>(oracleCapsuleTable[slot])(
               oracleCapsule) != oracleCapsule ||
           reinterpret_cast<SelfSlot>(candidateCapsuleTable[slot])(
               candidateCapsule) != candidateCapsule)
            ++failures;
        ++cases;
    }
    typedef void (__thiscall* RadiusSetter)(void*, float);
    const unsigned capsuleSetBits[] = {
        0x00000000u, 0x3f800000u, 0xbf000000u, 0x7fc00001u
    };
    for(unsigned bits : capsuleSetBits) {
        float radius;
        memcpy(&radius, &bits, 4);
        reinterpret_cast<RadiusSetter>(oracleCapsuleTable[14])(
            oracleCapsule, radius);
        reinterpret_cast<RadiusSetter>(candidateCapsuleTable[14])(
            candidateCapsule, radius);
        unsigned oracleStored, candidateStored;
        memcpy(&oracleStored, oracleCapsule + 0xe0, 4);
        memcpy(&candidateStored, candidateCapsule + 0xe0, 4);
        oracleDigest = foldOracle(oracleDigest, &oracleStored,
            sizeof(oracleStored));
        if(oracleStored != candidateStored || oracleStored != bits)
            ++failures;
        ++cases;
    }
    typedef bool (__thiscall* CapsuleSaveSlot)(void*, void*);
    unsigned char oracleRecord[0x58], candidateRecord[0x58];
    memset(oracleRecord, 0xcd, sizeof(oracleRecord));
    memset(candidateRecord, 0xcd, sizeof(candidateRecord));
    const bool oracleSaved = reinterpret_cast<CapsuleSaveSlot>(
        oracleCapsuleTable[13])(oracleCapsule, oracleRecord);
    const bool candidateSaved = reinterpret_cast<CapsuleSaveSlot>(
        candidateCapsuleTable[13])(candidateCapsule, candidateRecord);
    oracleDigest = foldOracle(oracleDigest, &oracleSaved, sizeof(oracleSaved));
    oracleDigest = foldOracle(oracleDigest, oracleRecord,
        sizeof(oracleRecord));
    if(oracleSaved != candidateSaved ||
       memcmp(oracleRecord, candidateRecord, sizeof(oracleRecord)) != 0) {
        fprintf(stderr, "capsule slot 13 save differs\n");
        ++failures;
    }
    ++cases;
    typedef void (__thiscall* CapsuleLoadSlot)(void*, const void*);
    unsigned char loadRecord[0x58] = {};
    const float loadDimensions[2] = {1.5f, 4.0f};
    const unsigned loadWord = 0x13572468u;
    const unsigned short loadGroup = 3u;
    memcpy(loadRecord + 0x4c, loadDimensions, sizeof(loadDimensions));
    memcpy(loadRecord + 0x54, &loadWord, 4);
    memcpy(loadRecord + 0x3c, &loadGroup, 2);
    reinterpret_cast<CapsuleLoadSlot>(oracleCapsuleTable[12])(
        oracleCapsule, loadRecord);
    reinterpret_cast<CapsuleLoadSlot>(candidateCapsuleTable[12])(
        candidateCapsule, loadRecord);
    oracleDigest = foldOracle(oracleDigest, oracleCapsule + 0xe0, 12);
    if(memcmp(oracleCapsule + 0xe0, candidateCapsule + 0xe0, 12) != 0 ||
       memcmp(oracleCapsule + 0xd8, candidateCapsule + 0xd8, 8) != 0) {
        fprintf(stderr, "capsule slot 12 load differs\n");
        ++failures;
    }
    ++cases;
    typedef bool (__thiscall* CapsuleMassSlot)(void*, MassFrame*, float, unsigned);
    const float capsuleDensities[2] = {1.0f,2.0f};
    for(unsigned low = 0; low < 2; ++low)
    for(unsigned density = 0; density < 2; ++density) {
        const unsigned short flags = static_cast<unsigned short>(low);
        memcpy(oracleCapsule + 0xde, &flags, 2);
        memcpy(candidateCapsule + 0xde, &flags, 2);
        unsigned char oracleFrame[0x34] = {}, candidateFrame[0x34] = {};
        const bool ro = reinterpret_cast<CapsuleMassSlot>(oracleCapsuleTable[4])(
            oracleCapsule, reinterpret_cast<MassFrame*>(oracleFrame),
            capsuleDensities[density], 0);
        const bool rc = reinterpret_cast<CapsuleMassSlot>(candidateCapsuleTable[4])(
            candidateCapsule, reinterpret_cast<MassFrame*>(candidateFrame),
            capsuleDensities[density], 0);
        oracleDigest = foldOracle(oracleDigest, &ro, sizeof(ro));
        oracleDigest = foldOracle(oracleDigest, oracleFrame,
            sizeof(oracleFrame));
        if(ro != rc || memcmp(oracleFrame, candidateFrame,
                sizeof(oracleFrame)) != 0) {
            fprintf(stderr, "capsule slot 4 low=%u density=%u differs\n",
                low, density);
            ++failures;
        }
        ++cases;
    }
    unsigned char baseRecord[0x58] = {};
    const float basePose[12] = {
        0,-1,0, 1,0,0, 0,0,1, 6,-3,2
    };
    memcpy(baseRecord + 8, basePose, sizeof(basePose));
    const unsigned short baseGroup = 5u;
    memcpy(baseRecord + 0x3c, &baseGroup, 2);
    typedef bool (__thiscall* CapsuleApplySlot)(void*, const void*);
    const bool appliedO = reinterpret_cast<CapsuleApplySlot>(oracleCapsuleTable[1])(
        oracleCapsule, baseRecord);
    const bool appliedC = reinterpret_cast<CapsuleApplySlot>(candidateCapsuleTable[1])(
        candidateCapsule, baseRecord);
    oracleDigest = foldOracle(oracleDigest, &appliedO, sizeof(appliedO));
    oracleDigest = foldOracle(oracleDigest, oracleCapsule + 0x6c, 48);
    if(appliedO != appliedC ||
       memcmp(oracleCapsule + 0x6c, candidateCapsule + 0x6c, 48) != 0 ||
       memcmp(oracleCapsule + 0xd8, candidateCapsule + 0xd8, 8) != 0) {
        fprintf(stderr, "capsule slot 1 apply differs\n");
        ++failures;
    }
    ++cases;
    unsigned char oracleBaseRecord[0x58], candidateBaseRecord[0x58];
    memset(oracleBaseRecord, 0xcd, sizeof(oracleBaseRecord));
    memset(candidateBaseRecord, 0xcd, sizeof(candidateBaseRecord));
    const bool baseSavedO = reinterpret_cast<CapsuleSaveSlot>(
        oracleCapsuleTable[2])(oracleCapsule, oracleBaseRecord);
    const bool baseSavedC = reinterpret_cast<CapsuleSaveSlot>(
        candidateCapsuleTable[2])(candidateCapsule, candidateBaseRecord);
    oracleDigest = foldOracle(oracleDigest, &baseSavedO, sizeof(baseSavedO));
    oracleDigest = foldOracle(oracleDigest, oracleBaseRecord,
        sizeof(oracleBaseRecord));
    if(baseSavedO != baseSavedC ||
       memcmp(oracleBaseRecord, candidateBaseRecord,
           sizeof(oracleBaseRecord)) != 0) {
        fprintf(stderr, "capsule slot 2 save differs\n");
        ++failures;
    }
    ++cases;
    typedef void (__thiscall* CapsuleOwnerSlot)(void*, unsigned);
    unsigned char oracleBeforeOwner[0xec], candidateBeforeOwner[0xec];
    memcpy(oracleBeforeOwner, oracleCapsule, sizeof(oracleBeforeOwner));
    memcpy(candidateBeforeOwner, candidateCapsule, sizeof(candidateBeforeOwner));
    reinterpret_cast<CapsuleOwnerSlot>(oracleCapsuleTable[6])(
        oracleCapsule, 1);
    reinterpret_cast<CapsuleOwnerSlot>(candidateCapsuleTable[6])(
        candidateCapsule, 1);
    const unsigned ownerUnchangedO =
        memcmp(oracleBeforeOwner, oracleCapsule, sizeof(oracleBeforeOwner)) == 0;
    const unsigned ownerUnchangedC =
        memcmp(candidateBeforeOwner, candidateCapsule,
            sizeof(candidateBeforeOwner)) == 0;
    oracleDigest = foldOracle(oracleDigest, &ownerUnchangedO,
        sizeof(ownerUnchangedO));
    if(!ownerUnchangedO || ownerUnchangedO != ownerUnchangedC) {
        fprintf(stderr, "capsule slot 6 detached owner update differs\n");
        ++failures;
    }
    ++cases;
    oracleBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    reinterpret_cast<DtorSlot>(oracleCapsuleTable[0])(oracleCapsule, 0);
    if(capsuleTableInstalled)
        reinterpret_cast<DtorSlot>(candidateCapsuleTable[0])(candidateCapsule, 0);
    else
        capsule.nxCapsuleScalarDeletingDtor(0);
    const unsigned oracleStackFrees = oracleFreeCount - oracleBefore;
    oracleDigest = foldOracle(oracleDigest, &oracleStackFrees,
        sizeof(oracleStackFrees));
    if(oracleStackFrees != 1 ||
       candidateAllocator.freeCount - candidateBefore != oracleStackFrees)
        ++failures;
    ++cases;
    unsigned char* oracleCapsuleHeap = static_cast<unsigned char*>(malloc(0xec));
    unsigned char* candidateCapsuleHeap = static_cast<unsigned char*>(malloc(0xec));
    if(!oracleCapsuleHeap || !candidateCapsuleHeap) return 2;
    memset(oracleCapsuleHeap, 0xcd, 0xec);
    memset(candidateCapsuleHeap, 0xcd, 0xec);
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x21a60)(
        oracleCapsuleHeap, 0, 0);
    new(candidateCapsuleHeap) CapsuleShape(0, 0);
    void** oracleCapsuleHeapTable = *reinterpret_cast<void***>(oracleCapsuleHeap);
    void** candidateCapsuleHeapTable = *reinterpret_cast<void***>(candidateCapsuleHeap);
    oracleBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    reinterpret_cast<DtorSlot>(oracleCapsuleHeapTable[0])(oracleCapsuleHeap, 1);
    if(capsuleTableInstalled)
        reinterpret_cast<DtorSlot>(candidateCapsuleHeapTable[0])(
            candidateCapsuleHeap, 1);
    else
        reinterpret_cast<CapsuleShape*>(candidateCapsuleHeap)->nxCapsuleScalarDeletingDtor(1);
    const unsigned oracleHeapFrees = oracleFreeCount - oracleBefore;
    oracleDigest = foldOracle(oracleDigest, &oracleHeapFrees,
        sizeof(oracleHeapFrees));
    if(oracleHeapFrees != 2 ||
       candidateAllocator.freeCount - candidateBefore != oracleHeapFrees)
        ++failures;
    ++cases;
    nxSetSdkAllocatorBridge(0);
    printf("shape vtable oracle_digest=%08x cases=%u failures=%u\n",
        oracleDigest, cases, failures);
    return failures ? 1 : 0;
}
