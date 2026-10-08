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
// The oracle's [0x101041bc] holder and the candidate's nxFoundationSDKAllocator
// are the same word: both modules import the one NxFoundation.dll this process
// loads, so installAllocator makes the candidate's Foundation-allocator frees
// (the shape constructors' collision objects, the box's own free) count in
// oracleFreeCount too. Every free comparison therefore takes the oracle's
// count before the candidate runs, and counts the candidate's frees as its
// share of the holder's count plus the SDK bridge's -- which keeps every
// oracle-side fold free of candidate frees.
// It sums the candidate's SDK-bridge and Foundation-holder frees, so the
// harness checks how many blocks the candidate freed, no longer which
// allocator it freed them through.
static unsigned candidateFreesSince(const CandidateAllocator& allocator,
        unsigned sdkBefore, unsigned holderBefore) {
    return (allocator.freeCount - sdkBefore) + (oracleFreeCount - holderBefore);
}
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

static unsigned capsuleLoadOracleReturn = 2u;
static unsigned capsuleLoadCandidateReturn = 2u;
static unsigned renderCount;
static unsigned renderRows[12][16];
static unsigned lineCount;
static unsigned lineRows[8][7];
struct OwnerNotifyObservation {
    unsigned slot;
    unsigned calls;
    void* owner;
    void* box;
};
static OwnerNotifyObservation ownerNotifyObservation;
static void __fastcall ownerNotifySlot9(void* owner, void*, void* box) {
    ownerNotifyObservation.slot = 9;
    ++ownerNotifyObservation.calls;
    ownerNotifyObservation.owner = owner;
    ownerNotifyObservation.box = box;
}
static void __fastcall ownerNotifySlot10(void* owner, void*, void* box) {
    ownerNotifyObservation.slot = 10;
    ++ownerNotifyObservation.calls;
    ownerNotifyObservation.owner = owner;
    ownerNotifyObservation.box = box;
}
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

// The box hull rows (scene-raycast Task 4): the rebuild 000973 (0x21420,
// ecx = the box, no stack arguments), BOX slot 12 000981 (0x21990), and the
// facade's slots 9 and 10 (000957, 000959), reached through each side's own
// table at +0xe0 of a box its own constructor built. Its own line and digest,
// so the registered shape vtable line above is untouched; the digest folds
// only the pinned DLL's answers.
struct BoxHullResult { unsigned digest, cases, failures; };
static unsigned gHullFaceCount = 6;
static unsigned gHullVertexCount = 8;
struct HullFakeSlots {
    // __fastcall with an unused edx is __thiscall with no stack arguments.
    static unsigned __fastcall faceCount(void*, void*) { return gHullFaceCount; }
    static unsigned __fastcall vertexCount(void*, void*) { return gHullVertexCount; }
};
static unsigned gHullSeed = 0x2468ace1u;
static void hullRandom(float scale, float* out) {
    gHullSeed = gHullSeed * 1664525u + 1013904223u;
    const int value = static_cast<int>(gHullSeed >> 8) - 0x800000;
    *out = static_cast<float>(value) * (scale / 8388608.0f);
}
// The hull's words with each face record's two list pointers replaced by the
// four words each points at (the two DLLs keep the lists at different
// addresses): dims, eight corners, then per face corners, list A, list B and
// the six floats. 3 + 24 + 6*15 words.
static void hullWords(const unsigned char* shape, unsigned out[117]) {
    memcpy(out, shape + 0xe4, 12 + 96);
    unsigned* w = out + 27;
    for(unsigned r = 0; r < 6; ++r) {
        const unsigned char* record = shape + 0x150 + 0x24*r;
        memcpy(w, record, 4);
        for(unsigned list = 0; list < 2; ++list) {
            const unsigned* pointer;
            memcpy(&pointer, record + 4 + 4*list, 4);
            if(pointer) memcpy(w + 1 + 4*list, pointer, 16);
            else for(unsigned k = 0; k < 4; ++k) w[1 + 4*list + k] = 0xffffffffu;
        }
        memcpy(w + 9, record + 0xc, 24);
        w += 15;
    }
}
static BoxHullResult runBoxHullCases(const unsigned char* base) {
    BoxHullResult result = { 2166136261u, 0, 0 };
    typedef void (__thiscall* BoxCtor)(void*, void*, unsigned);
    typedef void (__thiscall* DtorSlot)(void*, unsigned);
    typedef void (__fastcall* RebuildRow)(void*);
    typedef bool (__thiscall* LoadSlot)(void*, const void*);
    typedef unsigned (__thiscall* FaceSlot)(void*, const float*, const float*);
    typedef unsigned (__thiscall* FeatureSlot)(void*, const float*, const float*, unsigned*);
    const BoxCtor oracleCtor = reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x21870);
    const RebuildRow oracleRebuild = reinterpret_cast<RebuildRow>(const_cast<unsigned char*>(base) + 0x21420);
    unsigned char o[0x228], c[0x228];
    auto build = [&]() {
        memset(o, 0xcd, sizeof(o));
        memset(c, 0xcd, sizeof(c));
        oracleCtor(o, 0, 0);
        new(c) BoxShape(0, 0);
    };
    auto destroy = [&]() {
        reinterpret_cast<DtorSlot>((*reinterpret_cast<void***>(o))[0])(o, 0);
        reinterpret_cast<DtorSlot>((*reinterpret_cast<void***>(c))[0])(c, 0);
    };
    auto compareHull = [&](const char* what, unsigned index) {
        unsigned ow[117], cw[117];
        hullWords(o, ow);
        hullWords(c, cw);
        result.digest = foldOracle(result.digest, ow, sizeof(ow));
        if(memcmp(ow, cw, sizeof(ow)) != 0) {
            fprintf(stderr, "box hull %s case=%u differs\n", what, index);
            for(unsigned k = 0; k < 117; ++k)
                if(ow[k] != cw[k])
                    fprintf(stderr, "  word%u oracle=%08x candidate=%08x\n", k, ow[k], cw[k]);
            ++result.failures;
        }
        ++result.cases;
    };

    // BOX hull facade slot 4 (phys_fn_000963) returns the requested face record.
    // Check every index directly: the support/rebuild cases below read mFaces
    // internally and would not detect a bad implementation of this getter.
    {
        typedef const BoxFaceRecord* (__thiscall* FaceRecordSlot)(void*, unsigned);
        build();
        FaceRecordSlot oracleFace = reinterpret_cast<FaceRecordSlot>(
            (*reinterpret_cast<void***>(o + 0xe0))[4]);
        FaceRecordSlot candidateFace = reinterpret_cast<FaceRecordSlot>(
            (*reinterpret_cast<void***>(c + 0xe0))[4]);
        for(unsigned index = 0; index < 6; ++index) {
            const BoxFaceRecord* expectedOracle = reinterpret_cast<const BoxFaceRecord*>(
                o + 0x150 + index * 0x24);
            const BoxFaceRecord* expectedCandidate = reinterpret_cast<const BoxFaceRecord*>(
                c + 0x150 + index * 0x24);
            if(oracleFace(o + 0xe0, index) != expectedOracle ||
               candidateFace(c + 0xe0, index) != expectedCandidate) {
                fprintf(stderr, "box hull facade_face case=%u differs\n", index);
                ++result.failures;
            }
            ++result.cases;
        }
        destroy();
    }

    // The remaining fixed BoxHullFacade accessors are cheap public-to-the-
    // object vtable contracts. Rebuild consumes these slots indirectly, so
    // call each one directly to make a bad count or table selection visible.
    {
        typedef unsigned (__thiscall* CountSlot)(void*);
        typedef const NxU32* (__thiscall* TableSlot)(void*);
        build();
        void** oracleTable = *reinterpret_cast<void***>(o + 0xe0);
        void** candidateTable = *reinterpret_cast<void***>(c + 0xe0);
        CountSlot oracleCount = reinterpret_cast<CountSlot>(oracleTable[1]);
        CountSlot candidateCount = reinterpret_cast<CountSlot>(candidateTable[1]);
        if(oracleCount(o + 0xe0) != 8 || candidateCount(c + 0xe0) != 8) {
            fprintf(stderr, "box hull facade_vertex_count differs\n");
            ++result.failures;
        }
        ++result.cases;

        const unsigned slots[3] = {6, 7, 8};
        const char* names[3] = {"edge_table", "face_corner_table", "adjacency_table"};
        for(unsigned i = 0; i < 3; ++i) {
            TableSlot oracleGetter = reinterpret_cast<TableSlot>(oracleTable[slots[i]]);
            TableSlot candidateGetter = reinterpret_cast<TableSlot>(candidateTable[slots[i]]);
            const NxU32* oracleWords = oracleGetter(o + 0xe0);
            const NxU32* candidateWords = candidateGetter(c + 0xe0);
            if(!oracleWords || !candidateWords ||
               memcmp(oracleWords, candidateWords, 24 * sizeof(NxU32)) != 0) {
                fprintf(stderr, "box hull facade_%s differs\n", names[i]);
                ++result.failures;
            }
            ++result.cases;
        }
        destroy();
    }

    const unsigned dimensionBits[][3] = {
        {0x3f800000u, 0x3f800000u, 0x3f800000u},    // 1, 1, 1
        {0x40200000u, 0x40500000u, 0x40980000u},    // 2.5, 3.25, 4.75
        {0x3e000000u, 0x42c80000u, 0x3f400000u},    // 0.125, 100, 0.75
        {0x00000000u, 0x80000000u, 0x00000000u},    // 0, -0, 0
        {0xbf800000u, 0x40000000u, 0xc0400000u},    // -1, 2, -3
        {0x3a83126fu, 0x4be4e1c0u, 0x40e33333u},    // 0.001, 3e7, 7.1
        {0x3dcccccdu, 0x3eaaaaabu, 0x3f7fffffu},    // 0.1, 1/3, 1-ulp
        {0x7f800000u, 0x00000001u, 0x7fc00123u},    // inf, denormal, NaN payload
        {0xffc00456u, 0xff800000u, 0x3f000000u}     // -NaN payload, -inf, 0.5
    };
    const unsigned dimensionCases = sizeof(dimensionBits) / sizeof(dimensionBits[0]);

    // 000973 called directly on each side.
    for(unsigned i = 0; i < dimensionCases; ++i) {
        build();
        memcpy(o + 0xe4, dimensionBits[i], 12);
        memcpy(c + 0xe4, dimensionBits[i], 12);
        oracleRebuild(o);
        reinterpret_cast<BoxShape*>(c)->nxBoxRebuildHull();
        compareHull("rebuild", i);
        destroy();
    }

    // 000973 through tables whose face and vertex counts the harness sets:
    // the loops' zero-count and short arms. Each side's table is its own
    // twelve slots with slots 1 and 3 replaced.
    build();
    void* oracleFake[12];
    void* candidateFake[12];
    void** oracleFacade = *reinterpret_cast<void***>(o + 0xe0);
    void** candidateFacade = *reinterpret_cast<void***>(c + 0xe0);
    memcpy(oracleFake, oracleFacade, sizeof(oracleFake));
    memcpy(candidateFake, candidateFacade, sizeof(candidateFake));
    oracleFake[1] = candidateFake[1] = reinterpret_cast<void*>(&HullFakeSlots::vertexCount);
    oracleFake[3] = candidateFake[3] = reinterpret_cast<void*>(&HullFakeSlots::faceCount);
    void* oracleFakePointer = oracleFake;
    void* candidateFakePointer = candidateFake;
    memcpy(o + 0xe0, &oracleFakePointer, 4);
    memcpy(c + 0xe0, &candidateFakePointer, 4);
    const unsigned vertexCounts[4] = {0, 1, 5, 8};
    for(unsigned faces = 0; faces <= 6; ++faces)
    for(unsigned v = 0; v < 4; ++v) {
        gHullFaceCount = faces;
        gHullVertexCount = vertexCounts[v];
        const unsigned index = faces * 4 + v;
        const unsigned* dims = dimensionBits[1 + index % 6];
        memcpy(o + 0xe4, dims, 12);
        memcpy(c + 0xe4, dims, 12);
        oracleRebuild(o);
        reinterpret_cast<BoxShape*>(c)->nxBoxRebuildHull();
        compareHull("rebuild_counts", index);
    }
    gHullFaceCount = 6;
    gHullVertexCount = 8;
    memcpy(o + 0xe0, &oracleFacade, 4);
    memcpy(c + 0xe0, &candidateFacade, 4);
    destroy();

    // 000981, BOX slot 12, through each side's own BOX table.
    for(unsigned i = 0; i < dimensionCases; ++i) {
        build();
        unsigned char record[0x58] = {};
        const float pose[12] = {0,0,1, 0,1,0, -1,0,0, 1.5f,-2.0f,0.25f};
        memcpy(record + 8, pose, sizeof(pose));
        const unsigned short group = static_cast<unsigned short>(i % 4);
        memcpy(record + 0x3c, &group, 2);
        memcpy(record + 0x4c, dimensionBits[i], 12);
        const bool ro = reinterpret_cast<LoadSlot>((*reinterpret_cast<void***>(o))[12])(o, record);
        const bool rc = reinterpret_cast<LoadSlot>((*reinterpret_cast<void***>(c))[12])(c, record);
        const unsigned char roByte = ro ? 1 : 0;
        result.digest = foldOracle(result.digest, &roByte, 1);
        result.digest = foldOracle(result.digest, o + 0x6c, 48);
        result.digest = foldOracle(result.digest, o + 0xd8, 8);
        if(ro != rc || memcmp(o + 0x6c, c + 0x6c, 48) != 0 ||
           memcmp(o + 0xd8, c + 0xd8, 8) != 0) {
            fprintf(stderr, "box hull load case=%u return/base differs\n", i);
            ++result.failures;
        }
        ++result.cases;
        compareHull("load", i);
        destroy();
    }

    // Facade slots 9 and 10 through each side's own +0xe0 table.
    const float directions[][3] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1},
        {1,1,0}, {1,0,1}, {0,1,1}, {-1,1,0}, {1,-1,-1}, {1,1,1}, {-1,-1,-1},
        {0,0,0}, {-0.0f,0,0}, {0.5f,0.49999997f,0}, {1e-30f,0,0},
        {0.70710677f,0.70710677f,0}, {0.3f,-0.8f,0.52f}, {2.0f,-3.5f,0.125f}
    };
    const unsigned directionCount = sizeof(directions) / sizeof(directions[0]);
    float poses[4][12];
    memset(poses, 0, sizeof(poses));
    poses[1][0] = poses[1][5] = poses[1][10] = 1.0f;                // identity, 4-word rows
    const float rotation[12] = {0,-1,0,9, 1,0,0,9, 0,0,1,9};
    memcpy(poses[2], rotation, sizeof(rotation));
    for(unsigned k = 0; k < 12; ++k) hullRandom(1.5f, &poses[3][k]);
    auto driveSlots = [&](const char* what, unsigned index, const float* direction,
            const float* pose) {
        FaceSlot oracleFace = reinterpret_cast<FaceSlot>((*reinterpret_cast<void***>(o + 0xe0))[9]);
        FaceSlot candidateFace = reinterpret_cast<FaceSlot>((*reinterpret_cast<void***>(c + 0xe0))[9]);
        FeatureSlot oracleFeature = reinterpret_cast<FeatureSlot>((*reinterpret_cast<void***>(o + 0xe0))[10]);
        FeatureSlot candidateFeature = reinterpret_cast<FeatureSlot>((*reinterpret_cast<void***>(c + 0xe0))[10]);
        const unsigned fo = oracleFace(o + 0xe0, direction, pose);
        const unsigned fc = candidateFace(c + 0xe0, direction, pose);
        unsigned outO = 0xcdcdcdcdu, outC = 0xcdcdcdcdu;
        const unsigned eo = oracleFeature(o + 0xe0, direction, pose, &outO);
        const unsigned ec = candidateFeature(c + 0xe0, direction, pose, &outC);
        const unsigned no = oracleFeature(o + 0xe0, direction, pose, 0);
        const unsigned nc = candidateFeature(c + 0xe0, direction, pose, 0);
        const unsigned words[4] = {fo, eo, outO, no};
        result.digest = foldOracle(result.digest, words, sizeof(words));
        if(fo != fc || eo != ec || outO != outC || no != nc) {
            fprintf(stderr, "box hull %s case=%u slot9 %u/%u slot10 %u/%u out %08x/%08x null %u/%u\n",
                what, index, fo, fc, eo, ec, outO, outC, no, nc);
            ++result.failures;
        }
        ++result.cases;
    };
    build();
    memcpy(o + 0xe4, dimensionBits[1], 12);
    memcpy(c + 0xe4, dimensionBits[1], 12);
    oracleRebuild(o);
    reinterpret_cast<BoxShape*>(c)->nxBoxRebuildHull();
    for(unsigned d = 0; d < directionCount; ++d)
    for(unsigned p = 0; p < 5; ++p)
        driveSlots("support", d * 5 + p, directions[d], p ? poses[p - 1] : 0);
    const float nanDirection[3] = {0, 0, 0};
    unsigned nanBits = 0x7fc00001u;
    float withNan[3];
    memcpy(withNan, nanDirection, sizeof(withNan));
    memcpy(&withNan[1], &nanBits, 4);
    driveSlots("support_nan", 0, withNan, 0);
    withNan[0] = 1.0f;
    driveSlots("support_nan", 1, withNan, 0);

    // Crafted normals and poses where the listing's sum grouping and its float
    // spills decide the answer (random data almost never does). A = 1e17f:
    // (A*1 + 4*1) - A is 0 in double where (A - A) + 4 is 4.
    const float bigA = 1e17f;
    auto writeNormals = [&](const float (*normals)[3]) {
        for(unsigned r = 0; r < 6; ++r) {
            memcpy(o + 0x15c + 0x24*r, normals[r], 12);
            memcpy(c + 0x15c + 0x24*r, normals[r], 12);
        }
    };
    const float ones[3] = {1, 1, 1};
    unsigned crafted = 0;
    // Grouping of each face position's sum: face j carries A, 4, -A in the
    // three orders; the other faces project to 2 (above every edge's 2*sqrt(1/2),
    // so slot 10's edge pass cannot mask the face search).
    for(unsigned j = 0; j < 6; ++j)
    for(unsigned unitAxis = 0; unitAxis < 3; ++unitAxis)
    for(unsigned sign = 0; sign < 2; ++sign) {
        float normals[6][3];
        for(unsigned r = 0; r < 6; ++r) { normals[r][0] = 2.0f; normals[r][1] = 0; normals[r][2] = 0; }
        const unsigned first = (unitAxis + 1) % 3, second = (unitAxis + 2) % 3;
        normals[j][unitAxis] = 4.0f;
        normals[j][first] = sign ? -bigA : bigA;
        normals[j][second] = sign ? bigA : -bigA;
        writeNormals(normals);
        driveSlots("crafted_grouping", crafted++, ones, 0);
    }
    // The float spill of a replaced best: faces a < b, a projects to 2 + 2^-29
    // (not a float), b to 2 + 2^-30 -- b wins only against the spilled 2.0f.
    // Face 0's seed stays unrounded, so a = 0 keeps it.
    const unsigned pairs[][2] = {{0,1}, {1,2}, {1,4}, {2,3}, {3,5}, {4,5}, {0,5}};
    for(const auto& pair : pairs) {
        float normals[6][3];
        for(unsigned r = 0; r < 6; ++r) { normals[r][0] = 0.25f; normals[r][1] = 0; normals[r][2] = 0; }
        normals[pair[0]][0] = 2.0f; normals[pair[0]][1] = 1.0f / 536870912.0f;
        normals[pair[1]][0] = 2.0f; normals[pair[1]][2] = 1.0f / 1073741824.0f;
        writeNormals(normals);
        driveSlots("crafted_spill", crafted++, ones, 0);
    }
    // The pose rows' grouping: one row carries A, 4, -A in the three orders.
    {
        oracleRebuild(o);
        reinterpret_cast<BoxShape*>(c)->nxBoxRebuildHull();
        for(unsigned row = 0; row < 3; ++row)
        for(unsigned unitAxis = 0; unitAxis < 3; ++unitAxis) {
            float pose[12] = {0.25f,0,0,0, 0,0.25f,0,0, 0,0,0.25f,0};
            const unsigned first = (unitAxis + 1) % 3, second = (unitAxis + 2) % 3;
            pose[4*row + unitAxis] = 4.0f;
            pose[4*row + first] = bigA;
            pose[4*row + second] = -bigA;
            const float direction[3] = {1, 1, 1};
            driveSlots("crafted_pose", crafted++, direction, pose);
            const float flipped[3] = {-1, -1, -1};
            driveSlots("crafted_pose", crafted++, flipped, pose);
        }
    }

    // The same slots over face normals the harness writes into both records
    // (the sums' grouping shows once the normals are not axes), with every face
    // count through the replaced slot 3.
    const unsigned samples = 96;
    for(unsigned s = 0; s < samples; ++s) {
        float normals[6][3];
        for(unsigned r = 0; r < 6; ++r)
            for(unsigned k = 0; k < 3; ++k)
                hullRandom(1.25f, &normals[r][k]);
        if(s % 8 == 3) memcpy(normals[4], normals[2], 12);   // a tie
        for(unsigned r = 0; r < 6; ++r) {
            memcpy(o + 0x15c + 0x24*r, normals[r], 12);
            memcpy(c + 0x15c + 0x24*r, normals[r], 12);
        }
        float direction[3], pose[12];
        for(unsigned k = 0; k < 3; ++k) hullRandom(2.0f, &direction[k]);
        for(unsigned k = 0; k < 12; ++k) hullRandom(1.0f, &pose[k]);
        const bool fake = s >= samples / 2;
        if(fake) {
            gHullFaceCount = s % 7;
            memcpy(o + 0xe0, &oracleFakePointer, 4);
            memcpy(c + 0xe0, &candidateFakePointer, 4);
            memcpy(oracleFake, oracleFacade, sizeof(oracleFake));
            memcpy(candidateFake, candidateFacade, sizeof(candidateFake));
            oracleFake[3] = candidateFake[3] = reinterpret_cast<void*>(&HullFakeSlots::faceCount);
        }
        driveSlots(fake ? "support_counts" : "support_normals", s, direction,
            s % 3 == 0 ? 0 : pose);
        if(fake) {
            memcpy(o + 0xe0, &oracleFacade, 4);
            memcpy(c + 0xe0, &candidateFacade, 4);
            gHullFaceCount = 6;
        }
    }
    destroy();
    return result;
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
    unsigned boxMassCases = 0, boxMassFailures = 0;
    unsigned boxMassDigest = 2166136261u;
    unsigned char oracleBytes[0x228], candidateBytes[0x228];
    memset(oracleBytes, 0xcd, sizeof(oracleBytes));
    memset(candidateBytes, 0xcd, sizeof(candidateBytes));
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x21870)(oracleBytes, 0, 0);
    new(candidateBytes) BoxShape(0, 0);
    {
        // phys_fn_001271 is installed by ShapeBase construction and must
        // forward the owner and AABB to owner vtable slot 10, not slot 9.
        void* ownerVtable[11] = {};
        ownerVtable[9] = reinterpret_cast<void*>(&ownerNotifySlot9);
        ownerVtable[10] = reinterpret_cast<void*>(&ownerNotifySlot10);
        struct FakeOwner { void** vtable; } owner = { ownerVtable };
        AABB box;
        memset(&box, 0, sizeof(box));
        typedef void (__cdecl* OwnerNotifyAdapter)(void*, AABB*);
        OwnerNotifyAdapter oracleNotify = reinterpret_cast<OwnerNotifyAdapter>(
            *reinterpret_cast<void* const*>(base + 0x128474));
        OwnerNotifyAdapter candidateNotify = gPrunableOwnerNotify;
        OwnerNotifyObservation oracleObservation = {};
        OwnerNotifyObservation candidateObservation = {};
        if(oracleNotify && candidateNotify) {
            memset(&ownerNotifyObservation, 0, sizeof(ownerNotifyObservation));
            oracleNotify(&owner, &box);
            oracleObservation = ownerNotifyObservation;
            memset(&ownerNotifyObservation, 0, sizeof(ownerNotifyObservation));
            candidateNotify(&owner, &box);
            candidateObservation = ownerNotifyObservation;
        }
        const bool matches = oracleNotify && candidateNotify &&
            oracleObservation.slot == 10 && oracleObservation.calls == 1 &&
            oracleObservation.owner == &owner && oracleObservation.box == &box &&
            candidateObservation.slot == oracleObservation.slot &&
            candidateObservation.calls == oracleObservation.calls &&
            candidateObservation.owner == oracleObservation.owner &&
            candidateObservation.box == oracleObservation.box;
        printf("shape vtable owner_notify oracle_slot=%u candidate_slot=%u "
            "oracle_calls=%u candidate_calls=%u owner_forwarded=%u box_forwarded=%u mismatches=%u\n",
            oracleObservation.slot, candidateObservation.slot,
            oracleObservation.calls, candidateObservation.calls,
            candidateObservation.owner == &owner,
            candidateObservation.box == &box, matches ? 0u : 1u);
        if(!matches)
            ++failures;
    }
    void** oracleTable = *reinterpret_cast<void***>(oracleBytes);
    void** candidateTable = *reinterpret_cast<void***>(candidateBytes);
    unsigned baseStubCases = 0, baseStubFailures = 0;
    unsigned baseStubDigest = 2166136261u;
    {
        // Base-shape slots 4, 5 and 7 are inherited stubs. Exercise their exact
        // entry points directly: final shape tables can replace these slots,
        // so derived-table calls do not prove the base implementations.
        typedef bool (__thiscall* BaseSlot4Fn)(void*, void*, float, unsigned);
        typedef bool (__thiscall* BaseSlot7Fn)(void*, unsigned*, const void*);
        typedef void* (__thiscall* BaseSlot5Fn)(void*, void*, float,
            unsigned, unsigned, void*);
        unsigned outputSentinel = 0x6a5b4c3du;
        unsigned oracleOutput = outputSentinel;
        unsigned candidateOutput = outputSentinel;
        void* const rayArgument = &outputSentinel;
        void* const hitArgument = &oracleOutput;
        BaseSlot4Fn oracleSlot4 = reinterpret_cast<BaseSlot4Fn>(
            const_cast<unsigned char*>(base) + 0x24f70);
        const bool oracleSlot4Result = oracleSlot4(oracleBytes, &oracleOutput,
            2.5f, 0x13579bdfu);
        const bool candidateSlot4Result =
            reinterpret_cast<ShapeBase*>(candidateBytes)->nxBaseSlot4(
                &candidateOutput, 2.5f, 0x13579bdfu);
        const unsigned slot4OracleWord = oracleSlot4Result ? 1u : 0u;
        baseStubDigest = foldOracle(baseStubDigest, &slot4OracleWord,
            sizeof(slot4OracleWord));
        baseStubDigest = foldOracle(baseStubDigest, &oracleOutput,
            sizeof(oracleOutput));
        if(oracleSlot4Result || oracleSlot4Result != candidateSlot4Result ||
           oracleOutput != outputSentinel || candidateOutput != oracleOutput) {
            fprintf(stderr, "base shape slot 4 stub differs\n");
            ++baseStubFailures;
        }
        printf("shape vtable base_stub slot4 oracle_false=%u candidate_false=%u "
            "output_preserved=%u\n", !oracleSlot4Result,
            !candidateSlot4Result, oracleOutput == outputSentinel &&
            candidateOutput == oracleOutput);
        ++baseStubCases;

        BaseSlot5Fn oracleSlot5 = reinterpret_cast<BaseSlot5Fn>(
            const_cast<unsigned char*>(base) + 0xb4070);
        unsigned stackBeforeSlot5 = 0, stackAfterOracleSlot5 = 0;
        unsigned stackBeforeCandidateSlot5 = 0, stackAfterCandidateSlot5 = 0;
        __asm mov stackBeforeSlot5, esp
        void* const oracleSlot5Result = oracleSlot5(oracleBytes, rayArgument,
            17.5f, 0x10203040u, 0x50607080u, hitArgument);
        __asm mov stackAfterOracleSlot5, esp
        __asm mov stackBeforeCandidateSlot5, esp
        void* const candidateSlot5Result =
            reinterpret_cast<ShapeBase*>(candidateBytes)->nxBaseSlot5(
                rayArgument, 17.5f, 0x10203040u, 0x50607080u, hitArgument);
        __asm mov stackAfterCandidateSlot5, esp
        const unsigned slot5OracleWord = oracleSlot5Result ? 1u : 0u;
        baseStubDigest = foldOracle(baseStubDigest, &slot5OracleWord,
            sizeof(slot5OracleWord));
        if(stackBeforeSlot5 != stackAfterOracleSlot5 ||
           stackBeforeCandidateSlot5 != stackAfterCandidateSlot5) {
            fprintf(stderr, "base shape slot 5 stack imbalance: oracle %u -> %u, "
                "candidate %u -> %u\n", stackBeforeSlot5, stackAfterOracleSlot5,
                stackBeforeCandidateSlot5, stackAfterCandidateSlot5);
            ++baseStubFailures;
        }
        if(oracleSlot5Result || oracleSlot5Result != candidateSlot5Result) {
            fprintf(stderr, "base shape slot 5 stub differs\n");
            ++baseStubFailures;
        }
        printf("shape vtable base_stub slot5 oracle_null=%u candidate_null=%u "
            "esp_balanced=%u\n", oracleSlot5Result == 0,
            candidateSlot5Result == 0,
            stackBeforeSlot5 == stackAfterOracleSlot5 &&
            stackBeforeCandidateSlot5 == stackAfterCandidateSlot5);
        ++baseStubCases;

        unsigned oracleSweepOutput = 0xcafef00du;
        unsigned candidateSweepOutput = oracleSweepOutput;
        BaseSlot7Fn oracleSlot7 = reinterpret_cast<BaseSlot7Fn>(
            const_cast<unsigned char*>(base) + 0x22dd0);
        const bool oracleSlot7Result = oracleSlot7(oracleBytes,
            &oracleSweepOutput, oracleBytes);
        const bool candidateSlot7Result =
            reinterpret_cast<ShapeBase*>(candidateBytes)->nxBaseSlot7(
                &candidateSweepOutput, candidateBytes);
        const unsigned slot7OracleWord = oracleSlot7Result ? 1u : 0u;
        baseStubDigest = foldOracle(baseStubDigest, &slot7OracleWord,
            sizeof(slot7OracleWord));
        baseStubDigest = foldOracle(baseStubDigest, &oracleSweepOutput,
            sizeof(oracleSweepOutput));
        if(oracleSlot7Result || oracleSlot7Result != candidateSlot7Result ||
           oracleSweepOutput != 0xcafef00du ||
           candidateSweepOutput != oracleSweepOutput) {
            fprintf(stderr, "base shape slot 7 stub differs\n");
            ++baseStubFailures;
        }
        printf("shape vtable base_stub slot7 oracle_false=%u candidate_false=%u "
            "output_preserved=%u\n", !oracleSlot7Result,
            !candidateSlot7Result, oracleSweepOutput == 0xcafef00du &&
            candidateSweepOutput == oracleSweepOutput);
        ++baseStubCases;
        cases += baseStubCases;
    }
    printf("shape vtable base_stub_cases=%u mismatches=%u\n",
        baseStubCases, baseStubFailures);
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
    // BOX slot 4 (phys_fn_000849) through the public shape's mass accumulator:
    // include its local-pose payload and both the unit-density and scaled paths.
    typedef bool (__thiscall* BoxMassSlot)(void*, MassFrame*, float, unsigned);
    const float boxMassDensities[] = {1.0f, 2.0f};
    const float boxMassPoses[][12] = {
        {1,0,0, 0,1,0, 0,0,1, 0,0,0},
        {0,-1,0, 1,0,0, 0,0,1, 1.25f,-0.75f,2.5f}
    };
    const unsigned short boxMassFlags[] = {0, 1};
    for(unsigned shape = 0; shape != 3; ++shape) {
        memcpy(oracleBytes + 0xe4, dimensions[shape], 12);
        memcpy(candidateBytes + 0xe4, dimensions[shape], 12);
        for(unsigned pose = 0; pose != 2; ++pose) {
            memcpy(oracleBytes + 0x6c, boxMassPoses[pose], 48);
            memcpy(candidateBytes + 0x6c, boxMassPoses[pose], 48);
            for(unsigned density = 0; density != 2; ++density)
            for(unsigned flags = 0; flags != 2; ++flags) {
                memcpy(oracleBytes + 0xde, &boxMassFlags[flags], 2);
                memcpy(candidateBytes + 0xde, &boxMassFlags[flags], 2);
                unsigned char oracleFrame[0x34], candidateFrame[0x34];
                memset(oracleFrame, 0xcd, sizeof(oracleFrame));
                memset(candidateFrame, 0xcd, sizeof(candidateFrame));
                const bool oracleMass = reinterpret_cast<BoxMassSlot>(oracleTable[4])(
                    oracleBytes, reinterpret_cast<MassFrame*>(oracleFrame),
                    boxMassDensities[density], 0);
                const bool candidateMass = reinterpret_cast<BoxMassSlot>(candidateTable[4])(
                    candidateBytes, reinterpret_cast<MassFrame*>(candidateFrame),
                    boxMassDensities[density], 0);
                boxMassDigest = foldOracle(boxMassDigest, &oracleMass,
                    sizeof(oracleMass));
                boxMassDigest = foldOracle(boxMassDigest, oracleFrame,
                    sizeof(oracleFrame));
                if(oracleMass != candidateMass ||
                   memcmp(oracleFrame, candidateFrame, sizeof(oracleFrame)) != 0) {
                    fprintf(stderr,
                        "box slot 4 shape=%u pose=%u density=%u flags=%u differs\n",
                        shape, pose, density, flags);
                    ++boxMassFailures;
                }
                ++boxMassCases;
            }
        }
    }
    if(reinterpret_cast<SelfSlot>(oracleTable[14])(oracleBytes) != oracleBytes ||
       reinterpret_cast<SelfSlot>(candidateTable[14])(candidateBytes) != candidateBytes)
        ++failures;
    ++cases;
    typedef void (__thiscall* DtorSlot)(void*, unsigned);
    unsigned oracleBefore = oracleFreeCount;
    reinterpret_cast<DtorSlot>(oracleTable[0])(oracleBytes, 0);
    unsigned oracleFrees = oracleFreeCount - oracleBefore;
    unsigned holderBefore = oracleFreeCount;
    unsigned candidateBefore = candidateAllocator.freeCount;
    reinterpret_cast<DtorSlot>(candidateTable[0])(candidateBytes, 0);
    if(oracleFrees != 1 ||
       candidateFreesSince(candidateAllocator, candidateBefore, holderBefore) != 1)
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
    oracleBefore = oracleFreeCount;
    reinterpret_cast<DtorSlot>(oracleHeapTable[0])(oracleHeap, 1);
    oracleFrees = oracleFreeCount - oracleBefore;
    holderBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    reinterpret_cast<DtorSlot>(candidateHeapTable[0])(candidateHeap, 1);
    if(oracleFrees != 2 ||
       candidateFreesSince(candidateAllocator, candidateBefore, holderBefore) != 2)
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
    oracleBefore = oracleFreeCount;
    reinterpret_cast<DtorSlot>(oracleSphereTable[0])(oracleSphere, 0);
    oracleFrees = oracleFreeCount - oracleBefore;
    holderBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    reinterpret_cast<DtorSlot>(candidateSphereTable[0])(candidateSphere, 0);
    if(oracleFrees != 1 ||
       candidateFreesSince(candidateAllocator, candidateBefore, holderBefore) != 1)
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
    reinterpret_cast<DtorSlot>(oracleHeapSphereTable[0])(oracleSphereHeap, 1);
    oracleFrees = oracleFreeCount - oracleBefore;
    holderBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    reinterpret_cast<DtorSlot>(candidateHeapSphereTable[0])(candidateSphereHeap, 1);
    if(oracleFrees != 2 ||
       candidateFreesSince(candidateAllocator, candidateBefore, holderBefore) != 2)
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
    // Row 000989 returns the al of the BASE apply (0x10021b34); the return
    // is reported on its own line so the registered digest line is unchanged.
    typedef bool (__thiscall* CapsuleLoadReturnSlot)(void*, const void*);
    capsuleLoadOracleReturn = reinterpret_cast<CapsuleLoadReturnSlot>(
        oracleCapsuleTable[12])(oracleCapsule, loadRecord) ? 1u : 0u;
    capsuleLoadCandidateReturn = reinterpret_cast<CapsuleLoadReturnSlot>(
        candidateCapsuleTable[12])(candidateCapsule, loadRecord) ? 1u : 0u;
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
    reinterpret_cast<DtorSlot>(oracleCapsuleTable[0])(oracleCapsule, 0);
    const unsigned oracleStackFrees = oracleFreeCount - oracleBefore;
    holderBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    if(capsuleTableInstalled)
        reinterpret_cast<DtorSlot>(candidateCapsuleTable[0])(candidateCapsule, 0);
    else
        capsule.nxCapsuleScalarDeletingDtor(0);
    oracleDigest = foldOracle(oracleDigest, &oracleStackFrees,
        sizeof(oracleStackFrees));
    if(oracleStackFrees != 1 ||
       candidateFreesSince(candidateAllocator, candidateBefore, holderBefore) !=
           oracleStackFrees)
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
    reinterpret_cast<DtorSlot>(oracleCapsuleHeapTable[0])(oracleCapsuleHeap, 1);
    const unsigned oracleHeapFrees = oracleFreeCount - oracleBefore;
    holderBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    if(capsuleTableInstalled)
        reinterpret_cast<DtorSlot>(candidateCapsuleHeapTable[0])(
            candidateCapsuleHeap, 1);
    else
        reinterpret_cast<CapsuleShape*>(candidateCapsuleHeap)->nxCapsuleScalarDeletingDtor(1);
    oracleDigest = foldOracle(oracleDigest, &oracleHeapFrees,
        sizeof(oracleHeapFrees));
    if(oracleHeapFrees != 2 ||
       candidateFreesSince(candidateAllocator, candidateBefore, holderBefore) !=
           oracleHeapFrees)
        ++failures;
    ++cases;

    unsigned char oraclePlane[0x10c], candidatePlane[0x10c];
    memset(oraclePlane, 0xcd, sizeof(oraclePlane));
    memset(candidatePlane, 0xcd, sizeof(candidatePlane));
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x24ed0)(
        oraclePlane, 0, 0);
    PlaneShape& plane = *new(candidatePlane) PlaneShape(0, 0);
    void** oraclePlaneTable = *reinterpret_cast<void***>(oraclePlane);
    void** candidatePlaneTable = *reinterpret_cast<void***>(candidatePlane);
    HMODULE planeTableOwner = 0;
    const BOOL planeTableMapped = GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(candidatePlaneTable), &planeTableOwner);
    const bool planeTableInstalled = planeTableMapped &&
        planeTableOwner == GetModuleHandleW(0);
    if(!planeTableInstalled) ++failures;
    ++cases;
    if(planeTableInstalled)
    for(unsigned slot = 0; slot < 17; ++slot) {
        HMODULE owner = 0;
        const BOOL ok = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(candidatePlaneTable[slot]), &owner);
        if(!ok || owner != GetModuleHandleW(0)) ++failures;
        ++cases;
    }
    typedef bool (__thiscall* PlaneMassSlot)(void*, void*, float, unsigned);
    const unsigned planeMassSeeds[] = {0u,0xcdcdcdcdu,0x7fc00001u};
    for(unsigned seed : planeMassSeeds) {
        unsigned oracleOut[13], candidateOut[13];
        for(unsigned i = 0; i < 13; ++i)
            oracleOut[i] = candidateOut[i] = seed;
        const bool ro = reinterpret_cast<PlaneMassSlot>(oraclePlaneTable[4])(
            oraclePlane, oracleOut, 2.0f, 0);
        const bool rc = planeTableInstalled
            ? reinterpret_cast<PlaneMassSlot>(candidatePlaneTable[4])(
                candidatePlane, candidateOut, 2.0f, 0)
            : plane.mBase.nxBaseSlot4(candidateOut, 2.0f, 0);
        oracleDigest = foldOracle(oracleDigest, &ro, sizeof(ro));
        oracleDigest = foldOracle(oracleDigest, oracleOut, sizeof(oracleOut));
        if(ro != rc || ro || memcmp(oracleOut, candidateOut,
                sizeof(oracleOut)) != 0)
            ++failures;
        ++cases;
    }
    typedef bool (__thiscall* PlaneSweepSlot)(void*, unsigned*, const void*);
    const unsigned planeSweepSeeds[] = {0u,0x80000000u,0xffffffffu};
    for(unsigned seed : planeSweepSeeds) {
        unsigned oracleOut = seed, candidateOut = seed;
        const void* unread = reinterpret_cast<const void*>(0xdeadbeefu);
        const bool ro = reinterpret_cast<PlaneSweepSlot>(oraclePlaneTable[7])(
            oraclePlane, &oracleOut, unread);
        const bool rc = planeTableInstalled
            ? reinterpret_cast<PlaneSweepSlot>(candidatePlaneTable[7])(
                candidatePlane, &candidateOut, unread)
            : plane.mBase.nxBaseSlot7(&candidateOut, unread);
        oracleDigest = foldOracle(oracleDigest, &ro, sizeof(ro));
        oracleDigest = foldOracle(oracleDigest, &oracleOut, sizeof(oracleOut));
        if(ro != rc || ro || oracleOut != candidateOut || oracleOut != seed)
            ++failures;
        ++cases;
    }
    NxShapeRaycastFn oraclePlaneRaycast =
        reinterpret_cast<NxShapeRaycastFn>(oraclePlaneTable[5]);
    const NxRay planeRays[4] = {
        NxRay(NxVec3(0,3,0), NxVec3(0,-1,0)),
        NxRay(NxVec3(0,-3,0), NxVec3(0,1,0)),
        NxRay(NxVec3(0,0,0), NxVec3(0,-1,0)),
        NxRay(NxVec3(2,5,-1), NxVec3(0,-1,0))
    };
    for(unsigned ray = 0; ray < 4; ++ray)
    for(unsigned limit = 0; limit < 2; ++limit)
    for(unsigned normal = 0; normal < 2; ++normal) {
        NxRaycastHit oracleHit, candidateHit;
        memset(&oracleHit, 0xcd, sizeof(oracleHit));
        memset(&candidateHit, 0xcd, sizeof(candidateHit));
        const unsigned flags = normal ? NX_RAYCAST_NORMAL : 0u;
        const NxCollisionShape* ro = oraclePlaneRaycast(
            reinterpret_cast<const NxCollisionShape*>(oraclePlane),
            &planeRays[ray], limits[limit], 0, flags, &oracleHit);
        const NxCollisionShape* rc = planeTableInstalled
            ? reinterpret_cast<NxShapeRaycastFn>(candidatePlaneTable[5])(
                reinterpret_cast<const NxCollisionShape*>(candidatePlane),
                &planeRays[ray], limits[limit], 0, flags, &candidateHit)
            : NxShapeRaycastPlane(
                reinterpret_cast<const NxCollisionShape*>(candidatePlane),
                nullptr, &planeRays[ray], limits[limit], 0, flags,
                &candidateHit);
        if(ro) oracleHit.shape = reinterpret_cast<NxShape*>(0x12345678u);
        if(rc) candidateHit.shape = reinterpret_cast<NxShape*>(0x12345678u);
        const unsigned oraclePresent = ro != 0;
        oracleDigest = foldOracle(oracleDigest, &oraclePresent,
            sizeof(oraclePresent));
        oracleDigest = foldOracle(oracleDigest, &oracleHit,
            sizeof(oracleHit));
        if(bool(ro) != bool(rc) ||
           memcmp(&oracleHit, &candidateHit, sizeof(oracleHit)) != 0) {
            fprintf(stderr, "plane slot 5 ray=%u limit=%u normal=%u differs\n",
                ray, limit, normal);
            ++failures;
        }
        ++cases;
    }
    unsigned planeSaveA, planeSaveB, planeSaveC;
    memcpy(&planeSaveA, guardA, 4); memcpy(&planeSaveB, guardB, 4);
    memcpy(&planeSaveC, guardC, 4);
    DWORD planeOldProtection, planeIgnoredProtection;
    if(!VirtualProtect(guardC, 4, PAGE_READWRITE, &planeOldProtection)) return 2;
    const unsigned planeZero = 0, planeOne = 0x3f800000u;
    memcpy(guardA, &planeZero, 4); memcpy(guardB, &planeZero, 4);
    const float planeGeometry[2][10] = {
        {0,1,0,0, -1,0,0, 0,0,1},
        {0,0,1,2, 1,0,0, 0,1,0}
    };
    for(unsigned geometry = 0; geometry < 2; ++geometry)
    for(unsigned low = 0; low < 2; ++low)
    for(unsigned enabled = 0; enabled < 2; ++enabled)
    for(unsigned cGuard = 0; cGuard < 2; ++cGuard) {
        memcpy(oraclePlane + 0xe0, planeGeometry[geometry], 40);
        memcpy(candidatePlane + 0xe0, planeGeometry[geometry], 40);
        const unsigned short flags = static_cast<unsigned short>(
            low | (enabled ? 8u : 0u));
        memcpy(oraclePlane + 0xde, &flags, 2);
        memcpy(candidatePlane + 0xde, &flags, 2);
        memcpy(guardC, cGuard ? &planeOne : &planeZero, 4);
        renderCount = lineCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        memset(lineRows, 0xcd, sizeof(lineRows));
        reinterpret_cast<SphereSlot3>(oraclePlaneTable[3])(
            oraclePlane, rendererObject);
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
        if(planeTableInstalled)
            reinterpret_cast<SphereSlot3>(candidatePlaneTable[3])(
                candidatePlane, rendererObject);
        else
            plane.nxPlaneDebugRenderDispatch(rendererObject);
        const unsigned expected = enabled && cGuard ? 1u : 0u;
        if(oraclePoseCount != 4u * expected || oracleLineCount != 0 ||
           renderCount != oraclePoseCount || lineCount != oracleLineCount ||
           memcmp(oraclePoseRows, renderRows, sizeof(renderRows)) != 0 ||
           memcmp(oracleLineRows, lineRows, sizeof(lineRows)) != 0) {
            fprintf(stderr,
                "plane slot 3 geometry=%u low=%u enabled=%u c=%u lines=%u/%u poses=%u/%u differs\n",
                geometry, low, enabled, cGuard, oracleLineCount, lineCount,
                oraclePoseCount, renderCount);
            ++failures;
        }
        ++cases;
    }
    unsigned planeSaveScale;
    memcpy(&planeSaveScale, scale, 4);
    memcpy(scale, &planeOne, 4);
    memcpy(oraclePlane + 0xe0, planeGeometry[1], 40);
    memcpy(candidatePlane + 0xe0, planeGeometry[1], 40);
    const float planePoseRotation[9] = {0,-1,0, 1,0,0, 0,0,1};
    const float planePoseTranslation[3] = {3,4,5};
    memcpy(oraclePlane + 0x0c, planePoseRotation, 36);
    memcpy(candidatePlane + 0x0c, planePoseRotation, 36);
    memcpy(oraclePlane + 0x30, planePoseTranslation, 12);
    memcpy(candidatePlane + 0x30, planePoseTranslation, 12);
    for(unsigned a = 0; a < 2; ++a)
    for(unsigned b = 0; b < 2; ++b)
    for(unsigned c = 0; c < 2; ++c)
    for(unsigned low = 0; low < 2; ++low) {
        memcpy(guardA, a ? &planeOne : &planeZero, 4);
        memcpy(guardB, b ? &planeOne : &planeZero, 4);
        memcpy(guardC, c ? &planeOne : &planeZero, 4);
        const unsigned short flags = static_cast<unsigned short>(8u | low);
        memcpy(oraclePlane + 0xde, &flags, 2);
        memcpy(candidatePlane + 0xde, &flags, 2);
        renderCount = lineCount = 0;
        memset(renderRows, 0xcd, sizeof(renderRows));
        memset(lineRows, 0xcd, sizeof(lineRows));
        reinterpret_cast<SphereSlot3>(oraclePlaneTable[3])(
            oraclePlane, rendererObject);
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
        if(planeTableInstalled)
            reinterpret_cast<SphereSlot3>(candidatePlaneTable[3])(
                candidatePlane, rendererObject);
        else
            plane.nxPlaneDebugRenderDispatch(rendererObject);
        if(oraclePoseCount != 3u*b + 4u*c ||
           oracleLineCount != 3u*a ||
           renderCount != oraclePoseCount || lineCount != oracleLineCount ||
           memcmp(oraclePoseRows, renderRows, sizeof(renderRows)) != 0 ||
           memcmp(oracleLineRows, lineRows, sizeof(lineRows)) != 0) {
            fprintf(stderr,
                "plane shared render a=%u b=%u c=%u low=%u lines=%u/%u poses=%u/%u differs\n",
                a, b, c, low, oracleLineCount, lineCount,
                oraclePoseCount, renderCount);
            ++failures;
        }
        ++cases;
    }
    memcpy(scale, &planeSaveScale, 4);
    memcpy(guardA, &planeSaveA, 4); memcpy(guardB, &planeSaveB, 4);
    memcpy(guardC, &planeSaveC, 4);
    VirtualProtect(guardC, 4, planeOldProtection, &planeIgnoredProtection);
    unsigned char planeRecords[3 * 24];
    for(unsigned i = 0; i < sizeof(planeRecords); i += 4) {
        const unsigned word = 0x55000000u + i;
        memcpy(planeRecords + i, &word, 4);
    }
    unsigned char planeInner[0x20] = {};
    void* planeRecordsPtr = planeRecords;
    memcpy(planeInner + 0x14, &planeRecordsPtr, 4);
    unsigned char oraclePlaneFake[0x10c] = {}, candidatePlaneFake[0x10c] = {};
    memcpy(oraclePlaneFake, &oraclePlaneTable, 4);
    memcpy(candidatePlaneFake, &candidatePlaneTable, 4);
    void* planeInnerPtr = planeInner;
    memcpy(oraclePlaneFake + 0xc4, &planeInnerPtr, 4);
    memcpy(candidatePlaneFake + 0xc4, &planeInnerPtr, 4);
    oraclePlaneFake[0xac] = candidatePlaneFake[0xac] = 2;
    for(unsigned idx = 0; idx < 3; ++idx) {
        const unsigned short index = static_cast<unsigned short>(idx);
        memcpy(oraclePlaneFake + 0xcc, &index, 2);
        memcpy(candidatePlaneFake + 0xcc, &index, 2);
        unsigned oracleOut[8] = {}, candidateOut[8] = {};
        reinterpret_cast<BoundsSlot>(oraclePlaneTable[8])(
            oraclePlaneFake, reinterpret_cast<float*>(oracleOut));
        reinterpret_cast<BoundsSlot>(candidatePlaneTable[8])(
            candidatePlaneFake, reinterpret_cast<float*>(candidateOut));
        oracleDigest = foldOracle(oracleDigest, oracleOut, sizeof(oracleOut));
        if(memcmp(oracleOut, candidateOut, sizeof(oracleOut)) != 0) {
            fprintf(stderr, "plane slot 8 idx=%u differs\n", idx);
            ++failures;
        }
        ++cases;
    }
    for(unsigned slot = 9; slot <= 11; ++slot) {
        float oracleOut[6], candidateOut[6];
        memset(oracleOut, 0xcd, sizeof(oracleOut));
        memset(candidateOut, 0xcd, sizeof(candidateOut));
        reinterpret_cast<BoundsSlot>(oraclePlaneTable[slot])(
            oraclePlane, oracleOut);
        reinterpret_cast<BoundsSlot>(candidatePlaneTable[slot])(
            candidatePlane, candidateOut);
        oracleDigest = foldOracle(oracleDigest, oracleOut, sizeof(oracleOut));
        if(memcmp(oracleOut, candidateOut, sizeof(oracleOut)) != 0) {
            fprintf(stderr, "plane slot %u bounds differ\n", slot);
            ++failures;
        }
        ++cases;
    }
    for(unsigned slot = 14; slot <= 16; ++slot) {
        if(reinterpret_cast<SelfSlot>(oraclePlaneTable[slot])(
               oraclePlane) != oraclePlane ||
           reinterpret_cast<SelfSlot>(candidatePlaneTable[slot])(
               candidatePlane) != candidatePlane)
            ++failures;
        ++cases;
    }
    unsigned char planeDesc[0x5c] = {};
    const float planeDescEquations[4][4] = {
        {1,0,0,2}, {0,1,0,2}, {0,0,-1,2}, {0.6f,0.8f,0,1.25f}
    };
    const unsigned short planeDescGroup = 4;
    memcpy(planeDesc + 0x3c, &planeDescGroup, 2);
    typedef void (__thiscall* PlaneLoadSlot)(void*, const void*);
    for(unsigned eq = 0; eq < 4; ++eq) {
        memcpy(planeDesc + 0x4c, planeDescEquations[eq],
            sizeof(planeDescEquations[eq]));
        reinterpret_cast<PlaneLoadSlot>(oraclePlaneTable[12])(
            oraclePlane, planeDesc);
        reinterpret_cast<PlaneLoadSlot>(candidatePlaneTable[12])(
            candidatePlane, planeDesc);
        oracleDigest = foldOracle(oracleDigest, oraclePlane + 0xe0, 44);
        if(memcmp(oraclePlane + 0xe0, candidatePlane + 0xe0, 44) != 0 ||
           memcmp(oraclePlane + 0x6c, candidatePlane + 0x6c, 48) != 0 ||
           memcmp(oraclePlane + 0xd8, candidatePlane + 0xd8, 8) != 0) {
            fprintf(stderr, "plane slot 12 equation=%u load differs\n", eq);
            ++failures;
        }
        ++cases;
    }
    typedef bool (__thiscall* PlaneSaveSlot)(void*, void*);
    unsigned char oraclePlaneDesc[0x5c], candidatePlaneDesc[0x5c];
    memset(oraclePlaneDesc, 0xcd, sizeof(oraclePlaneDesc));
    memset(candidatePlaneDesc, 0xcd, sizeof(candidatePlaneDesc));
    const bool savedO = reinterpret_cast<PlaneSaveSlot>(oraclePlaneTable[13])(
        oraclePlane, oraclePlaneDesc);
    const bool savedC = reinterpret_cast<PlaneSaveSlot>(candidatePlaneTable[13])(
        candidatePlane, candidatePlaneDesc);
    oracleDigest = foldOracle(oracleDigest, &savedO, sizeof(savedO));
    oracleDigest = foldOracle(oracleDigest, oraclePlaneDesc,
        sizeof(oraclePlaneDesc));
    if(savedO != savedC || memcmp(oraclePlaneDesc, candidatePlaneDesc,
            sizeof(oraclePlaneDesc)) != 0) {
        fprintf(stderr, "plane slot 13 save differs\n");
        ++failures;
    }
    ++cases;
    const bool planeAppliedO = reinterpret_cast<CapsuleApplySlot>(
        oraclePlaneTable[1])(oraclePlane, baseRecord);
    const bool planeAppliedC = reinterpret_cast<CapsuleApplySlot>(
        candidatePlaneTable[1])(candidatePlane, baseRecord);
    oracleDigest = foldOracle(oracleDigest, &planeAppliedO,
        sizeof(planeAppliedO));
    oracleDigest = foldOracle(oracleDigest, oraclePlane + 0x6c, 48);
    if(planeAppliedO != planeAppliedC ||
       memcmp(oraclePlane + 0x6c, candidatePlane + 0x6c, 48) != 0 ||
       memcmp(oraclePlane + 0xd8, candidatePlane + 0xd8, 8) != 0) {
        fprintf(stderr, "plane slot 1 apply differs\n");
        ++failures;
    }
    ++cases;
    unsigned char oraclePlaneBase[0x5c], candidatePlaneBase[0x5c];
    memset(oraclePlaneBase, 0xcd, sizeof(oraclePlaneBase));
    memset(candidatePlaneBase, 0xcd, sizeof(candidatePlaneBase));
    const bool planeBaseSavedO = reinterpret_cast<PlaneSaveSlot>(
        oraclePlaneTable[2])(oraclePlane, oraclePlaneBase);
    const bool planeBaseSavedC = reinterpret_cast<PlaneSaveSlot>(
        candidatePlaneTable[2])(candidatePlane, candidatePlaneBase);
    oracleDigest = foldOracle(oracleDigest, &planeBaseSavedO,
        sizeof(planeBaseSavedO));
    oracleDigest = foldOracle(oracleDigest, oraclePlaneBase,
        sizeof(oraclePlaneBase));
    if(planeBaseSavedO != planeBaseSavedC ||
       memcmp(oraclePlaneBase, candidatePlaneBase,
           sizeof(oraclePlaneBase)) != 0) {
        fprintf(stderr, "plane slot 2 save differs\n");
        ++failures;
    }
    ++cases;
    unsigned char oraclePlaneBeforeOwner[0x10c], candidatePlaneBeforeOwner[0x10c];
    memcpy(oraclePlaneBeforeOwner, oraclePlane,
        sizeof(oraclePlaneBeforeOwner));
    memcpy(candidatePlaneBeforeOwner, candidatePlane,
        sizeof(candidatePlaneBeforeOwner));
    reinterpret_cast<CapsuleOwnerSlot>(oraclePlaneTable[6])(oraclePlane, 1);
    reinterpret_cast<CapsuleOwnerSlot>(candidatePlaneTable[6])(
        candidatePlane, 1);
    const unsigned planeOwnerNoopO =
        memcmp(oraclePlaneBeforeOwner, oraclePlane,
            sizeof(oraclePlaneBeforeOwner)) == 0;
    const unsigned planeOwnerNoopC =
        memcmp(candidatePlaneBeforeOwner, candidatePlane,
            sizeof(candidatePlaneBeforeOwner)) == 0;
    oracleDigest = foldOracle(oracleDigest, &planeOwnerNoopO,
        sizeof(planeOwnerNoopO));
    if(!planeOwnerNoopO || planeOwnerNoopO != planeOwnerNoopC) {
        fprintf(stderr, "plane slot 6 detached owner update differs\n");
        ++failures;
    }
    ++cases;
    oracleBefore = oracleFreeCount;
    reinterpret_cast<DtorSlot>(oraclePlaneTable[0])(oraclePlane, 0);
    const unsigned oraclePlaneStackFrees = oracleFreeCount - oracleBefore;
    holderBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    if(planeTableInstalled)
        reinterpret_cast<DtorSlot>(candidatePlaneTable[0])(candidatePlane, 0);
    else
        plane.nxPlaneScalarDeletingDtor(0);
    oracleDigest = foldOracle(oracleDigest, &oraclePlaneStackFrees,
        sizeof(oraclePlaneStackFrees));
    if(oraclePlaneStackFrees != 1 ||
       candidateFreesSince(candidateAllocator, candidateBefore, holderBefore) !=
           oraclePlaneStackFrees)
        ++failures;
    ++cases;
    unsigned char* oraclePlaneHeap = static_cast<unsigned char*>(malloc(0x10c));
    unsigned char* candidatePlaneHeap = static_cast<unsigned char*>(malloc(0x10c));
    if(!oraclePlaneHeap || !candidatePlaneHeap) return 2;
    memset(oraclePlaneHeap, 0xcd, 0x10c);
    memset(candidatePlaneHeap, 0xcd, 0x10c);
    reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) + 0x24ed0)(
        oraclePlaneHeap, 0, 0);
    new(candidatePlaneHeap) PlaneShape(0, 0);
    void** oraclePlaneHeapTable = *reinterpret_cast<void***>(oraclePlaneHeap);
    void** candidatePlaneHeapTable = *reinterpret_cast<void***>(candidatePlaneHeap);
    oracleBefore = oracleFreeCount;
    reinterpret_cast<DtorSlot>(oraclePlaneHeapTable[0])(oraclePlaneHeap, 1);
    const unsigned oraclePlaneHeapFrees = oracleFreeCount - oracleBefore;
    holderBefore = oracleFreeCount;
    candidateBefore = candidateAllocator.freeCount;
    if(planeTableInstalled)
        reinterpret_cast<DtorSlot>(candidatePlaneHeapTable[0])(
            candidatePlaneHeap, 1);
    else
        reinterpret_cast<PlaneShape*>(candidatePlaneHeap)->nxPlaneScalarDeletingDtor(1);
    oracleDigest = foldOracle(oracleDigest, &oraclePlaneHeapFrees,
        sizeof(oraclePlaneHeapFrees));
    if(oraclePlaneHeapFrees != 2 ||
       candidateFreesSince(candidateAllocator, candidateBefore, holderBefore) !=
           oraclePlaneHeapFrees)
        ++failures;
    ++cases;
    // MESH slot 10 copies its four-word center record, transforms the first
    // three words through the shape pose, and preserves the fourth word.
    typedef void (__thiscall* MeshCenterSlot)(void*, float*);
    for(unsigned sample = 0; sample < 4; ++sample) {
        unsigned char oracleMeshShape[0xe8] = {}, candidateMeshShape[0xe8] = {};
        unsigned char meshRecord[0x100] = {};
        const float source[4] = {
            0.25f + sample, -1.5f * (sample + 1), 2.75f - sample,
            0.125f * (sample + 1)
        };
        memcpy(meshRecord + 0x5c, source, sizeof(source));
        for(unsigned row = 0; row < 3; ++row) {
            for(unsigned col = 0; col < 3; ++col) {
                const float value = row == col ? 1.25f + sample * 0.25f :
                    (row + 1) * (col + 1) * 0.125f;
                memcpy(oracleMeshShape + 0x0c + 12*row + 4*col,
                    &value, 4);
                memcpy(candidateMeshShape + 0x0c + 12*row + 4*col,
                    &value, 4);
            }
            const float translation = (row + 1) * (sample + 1) * 0.75f;
            memcpy(oracleMeshShape + 0x30 + 4*row, &translation, 4);
            memcpy(candidateMeshShape + 0x30 + 4*row, &translation, 4);
        }
        void* meshPointer = meshRecord;
        memcpy(oracleMeshShape + 0xe0, &meshPointer, 4);
        memcpy(candidateMeshShape + 0xe0, &meshPointer, 4);
        float oracleCenter[4] = {}, candidateCenter[4] = {};
        reinterpret_cast<MeshCenterSlot>(const_cast<unsigned char*>(base) +
            0x29190)(oracleMeshShape, oracleCenter);
        reinterpret_cast<MeshShape*>(candidateMeshShape)->nxMeshTransformCenter(
            candidateCenter);
        oracleDigest = foldOracle(oracleDigest, oracleCenter,
            sizeof(oracleCenter));
        if(memcmp(oracleCenter, candidateCenter, sizeof(oracleCenter)) != 0) {
            fprintf(stderr, "mesh slot 10 sample=%u differs\n", sample);
            ++failures;
        }
        ++cases;
    }
    // MESH slot 9's no-tree branch reads six local bound words and applies
    // the shape pose. Keep the mesh record's +0xa0 tree pointer null.
    typedef void (__thiscall* MeshBoundsSlot)(void*, float*);
    for(unsigned sample = 0; sample < 64; ++sample) {
        unsigned char oracleMeshShape[0xe8] = {}, candidateMeshShape[0xe8] = {};
        unsigned char meshRecord[0xb0] = {};
        const float lo[3] = {
            -2.2f - sample * 0.113f, -0.53f * (sample + 1),
            0.77f - sample * 0.161f
        };
        const float hi[3] = {
            3.51f + sample * 0.137f, 1.27f * (sample + 1),
            4.03f + sample * 0.173f
        };
        memcpy(meshRecord + 0x44, lo, sizeof(lo));
        memcpy(meshRecord + 0x50, hi, sizeof(hi));
        for(unsigned row = 0; row < 3; ++row) {
            for(unsigned col = 0; col < 3; ++col) {
                const float value = row == col ? 0.73f + 0.013f * sample :
                    ((row + col + sample) % 2 ? -1.0f : 1.0f) *
                    0.117f * (row + col + 1);
                memcpy(oracleMeshShape + 0x0c + 12*row + 4*col,
                    &value, 4);
                memcpy(candidateMeshShape + 0x0c + 12*row + 4*col,
                    &value, 4);
            }
            const float translation = (row + 1) * (sample + 1) * -0.373f;
            memcpy(oracleMeshShape + 0x30 + 4*row, &translation, 4);
            memcpy(candidateMeshShape + 0x30 + 4*row, &translation, 4);
        }
        void* meshPointer = meshRecord;
        memcpy(oracleMeshShape + 0xe0, &meshPointer, 4);
        memcpy(candidateMeshShape + 0xe0, &meshPointer, 4);
        float oracleBounds[6] = {}, candidateBounds[6] = {};
        reinterpret_cast<MeshBoundsSlot>(const_cast<unsigned char*>(base) +
            0x28ed0)(oracleMeshShape, oracleBounds);
        reinterpret_cast<MeshShape*>(candidateMeshShape)->nxMeshWorldAABBNoTree(
            candidateBounds);
        oracleDigest = foldOracle(oracleDigest, oracleBounds,
            sizeof(oracleBounds));
        if(memcmp(oracleBounds, candidateBounds, sizeof(oracleBounds)) != 0) {
            fprintf(stderr, "mesh slot 9 no-tree sample=%u differs\n", sample);
            if(sample < 2) for(unsigned k = 0; k < 6; ++k) {
                unsigned ow, cw;
                memcpy(&ow, oracleBounds + k, 4);
                memcpy(&cw, candidateBounds + k, 4);
                fprintf(stderr, "  word%u oracle=%08x candidate=%08x\n",
                    k, ow, cw);
            }
            ++failures;
        }
        ++cases;
    }
    // MESH slot 9's tree-backed branch uses a neighbor graph to find the
    // support vertex for each of the six signed pose axes. Every vertex in
    // this tetrahedron neighbors the other three, so the climb is complete.
    const float meshVertices[12] = {
        -1.3f, 0.2f, 0.4f,  2.1f, -0.6f, 0.3f,
        0.5f, 2.3f, -0.7f,  -0.4f, 0.8f, 2.4f
    };
    const unsigned meshCounts[4] = {3, 3, 3, 3};
    const unsigned meshNoNeighbors[4] = {0, 0, 0, 0};
    const unsigned meshOffsets[4] = {0, 3, 6, 9};
    const unsigned meshChainCounts[4] = {1, 2, 2, 1};
    const unsigned meshChainOffsets[4] = {0, 1, 3, 5};
    const unsigned meshChainNeighbors[6] = {1, 0, 2, 1, 3, 2};
    const unsigned meshNeighbors[12] = {
        1, 2, 3,  0, 2, 3,  0, 1, 3,  0, 1, 2
    };
    for(unsigned sample = 0; sample < 84; ++sample) {
        const unsigned mode = sample % 7;
        unsigned char oracleMeshShape[0xe8] = {}, candidateMeshShape[0xe8] = {};
        unsigned char oracleMesh[0xb0] = {}, candidateMesh[0xb0] = {};
        unsigned char oracleTree[0x80] = {}, candidateTree[0x80] = {};
        unsigned char oracleGraph[0x20] = {}, candidateGraph[0x20] = {};
        unsigned char oracleOwner[0x10] = {}, candidateOwner[0x10] = {};
        unsigned char oracleScratch[0x20] = {}, candidateScratch[0x20] = {};
        unsigned oracleVisited[4] = {}, candidateVisited[4] = {};
        for(unsigned side = 0; side < 2; ++side) {
            unsigned char* shape = side ? candidateMeshShape : oracleMeshShape;
            unsigned char* mesh = side ? candidateMesh : oracleMesh;
            unsigned char* tree = side ? candidateTree : oracleTree;
            unsigned char* graph = side ? candidateGraph : oracleGraph;
            unsigned char* owner = side ? candidateOwner : oracleOwner;
            unsigned char* scratch = side ? candidateScratch : oracleScratch;
            unsigned* visited = side ? candidateVisited : oracleVisited;
            void* pointer = owner;
            memcpy(shape + 4, &pointer, 4);
            pointer = mesh;
            memcpy(shape + 0xe0, &pointer, 4);
            pointer = tree;
            memcpy(mesh + 0xa0, &pointer, 4);
            pointer = const_cast<float*>(meshVertices);
            memcpy(tree + 0x10, &pointer, 4);
            pointer = mode == 1 ? nullptr : graph;
            memcpy(tree + 0x64, &pointer, 4);
            for(unsigned k = 0; k < 6; ++k) {
                const unsigned start = (sample + k) % 4;
                memcpy(tree + 0x68 + 4*k, &start, 4);
            }
            pointer = mode == 2 ? nullptr :
                mode == 5 ? const_cast<unsigned*>(meshNoNeighbors) :
                mode == 6 ? const_cast<unsigned*>(meshChainCounts) :
                const_cast<unsigned*>(meshCounts);
            memcpy(graph + 8, &pointer, 4);
            pointer = mode == 3 ? nullptr :
                mode == 6 ? const_cast<unsigned*>(meshChainOffsets) :
                const_cast<unsigned*>(meshOffsets);
            memcpy(graph + 0x0c, &pointer, 4);
            pointer = mode == 4 ? nullptr :
                mode == 6 ? const_cast<unsigned*>(meshChainNeighbors) :
                const_cast<unsigned*>(meshNeighbors);
            memcpy(graph + 0x10, &pointer, 4);
            pointer = scratch;
            memcpy(owner + 4, &pointer, 4);
            const unsigned count = 4;
            memcpy(scratch + 4, &count, 4);
            pointer = visited;
            memcpy(scratch + 8, &pointer, 4);
            if(sample >= 70) {
                const unsigned nearWrap = 0xfffffffcu;
                memcpy(scratch + 0x14, &nearWrap, 4);
                for(unsigned k = 0; k < 4; ++k)
                    visited[k] = 0xdead0000u + k;
            }
            for(unsigned row = 0; row < 3; ++row) {
                for(unsigned col = 0; col < 3; ++col) {
                    const float value = row == col ?
                        0.91f + 0.019f * sample :
                        ((row + col + sample) % 2 ? -1.0f : 1.0f) *
                        0.071f * (row + col + 1);
                    memcpy(shape + 0x0c + 12*row + 4*col, &value, 4);
                }
                const float translation = (row + 1) * (sample + 1) * 0.233f;
                memcpy(shape + 0x30 + 4*row, &translation, 4);
            }
        }
        float oracleBounds[6] = {}, candidateBounds[6] = {};
        reinterpret_cast<MeshBoundsSlot>(const_cast<unsigned char*>(base) +
            0x28ed0)(oracleMeshShape, oracleBounds);
        reinterpret_cast<MeshShape*>(candidateMeshShape)->nxMeshWorldAABB(
            candidateBounds);
        oracleDigest = foldOracle(oracleDigest, oracleBounds,
            sizeof(oracleBounds));
        if(memcmp(oracleBounds, candidateBounds, sizeof(oracleBounds)) != 0 ||
           memcmp(oracleTree + 0x68, candidateTree + 0x68, 24) != 0 ||
           memcmp(oracleVisited, candidateVisited, sizeof(oracleVisited)) != 0 ||
           memcmp(oracleScratch + 0x14, candidateScratch + 0x14, 4) != 0) {
            fprintf(stderr, "mesh slot 9 tree sample=%u differs\n", sample);
            ++failures;
        }
        ++cases;
    }
    // MESH slot 7: no classifier returns the mesh's +0x68 word and false.
    // A classifier with a prepared plane table transforms the input point,
    // selects a plane, and writes the ray/plane distance before returning true.
    typedef bool (__thiscall* MeshSweepSlot)(void*, float*, const float*);
    for(unsigned sample = 0; sample < 24; ++sample) {
        const bool prepared = sample >= 4;
        unsigned char oracleShape[0xe8] = {}, candidateShape[0xe8] = {};
        unsigned char oracleMesh[0xb0] = {}, candidateMesh[0xb0] = {};
        unsigned char oracleTree[0x40] = {}, candidateTree[0x40] = {};
        unsigned char oracleClassifier[0x20] = {}, candidateClassifier[0x20] = {};
        unsigned char oraclePlanes[0xb0] = {}, candidatePlanes[0xb0] = {};
        unsigned char map[54] = {};
        for(unsigned k = 0; k < 54; ++k)
            map[k] = static_cast<unsigned char>(k % 3);
        const unsigned meshWord = 0x3f400000u + sample;
        memcpy(oracleMesh + 0x68, &meshWord, 4);
        memcpy(candidateMesh + 0x68, &meshWord, 4);
        for(unsigned side = 0; side < 2; ++side) {
            unsigned char* shape = side ? candidateShape : oracleShape;
            unsigned char* mesh = side ? candidateMesh : oracleMesh;
            unsigned char* tree = side ? candidateTree : oracleTree;
            unsigned char* classifier = side ? candidateClassifier : oracleClassifier;
            unsigned char* planes = side ? candidatePlanes : oraclePlanes;
            void* pointer = mesh;
            memcpy(shape + 0xe0, &pointer, 4);
            if(prepared) {
                pointer = tree;
                memcpy(mesh + 0xa0, &pointer, 4);
                pointer = classifier;
                memcpy(mesh + 0xac, &pointer, 4);
                const unsigned bins = 1 + sample % 3;
                memcpy(classifier + 4, &bins, 4);
                pointer = map;
                memcpy(classifier + 0x0c, &pointer, 4);
                pointer = planes;
                memcpy(tree + 0x28, &pointer, 4);
                for(unsigned p = 0; p < 3; ++p) {
                    const float normal[3] = {
                        0.13f * (p + 1), -0.21f + 0.04f * p,
                        1.07f - 0.05f * p
                    };
                    const float planeD = -1.37f + 0.19f * p;
                    memcpy(planes + 0x0c + p*0x24, normal,
                        sizeof(normal));
                    memcpy(planes + 0x18 + p*0x24, &planeD, 4);
                }
                const float origin[3] = {0.11f, -0.23f, 0.37f};
                memcpy(tree + 0x18, origin, sizeof(origin));
                for(unsigned row = 0; row < 3; ++row) {
                    for(unsigned col = 0; col < 3; ++col) {
                        const float value = row == col ?
                            0.81f + 0.017f * sample :
                            ((row + col + sample) % 2 ? -1.0f : 1.0f) *
                            0.047f * (row + col + 1);
                        memcpy(shape + 0x0c + 12*row + 4*col, &value, 4);
                    }
                    const float translation =
                        (row + 1) * (sample + 1) * 0.083f;
                    memcpy(shape + 0x30 + 4*row, &translation, 4);
                }
            }
        }
        const float input[3] = {
            1.1f + 0.091f * sample, -0.6f + 0.071f * sample,
            2.3f + 0.047f * sample
        };
        unsigned oracleOut[2] = {0xcdcdcdcdu, 0xa5a5a5a5u};
        unsigned candidateOut[2] = {0xcdcdcdcdu, 0xa5a5a5a5u};
        const bool ro = reinterpret_cast<MeshSweepSlot>(
            const_cast<unsigned char*>(base) + 0x29610)(
            oracleShape, reinterpret_cast<float*>(oracleOut), input);
        const bool rc = reinterpret_cast<MeshShape*>(candidateShape)->
            nxMeshSweepPrepared(reinterpret_cast<float*>(candidateOut), input);
        oracleDigest = foldOracle(oracleDigest, &ro, sizeof(ro));
        oracleDigest = foldOracle(oracleDigest, oracleOut, sizeof(oracleOut));
        if(ro != rc || memcmp(oracleOut, candidateOut,
                sizeof(oracleOut)) != 0) {
            fprintf(stderr, "mesh slot 7 prepared=%u sample=%u differs\n",
                prepared ? 1u : 0u, sample);
            ++failures;
        }
        ++cases;
    }
    // MESH slot 0 must destroy its collision object, decrement the bound
    // mesh refcount, and free the shape itself only when flags bit 0 is set.
    for(unsigned flags = 0; flags <= 1; ++flags) {
        unsigned char* oracleShape = static_cast<unsigned char*>(malloc(0xe8));
        unsigned char* candidateShape = static_cast<unsigned char*>(malloc(0xe8));
        if(!oracleShape || !candidateShape) return 2;
        memset(oracleShape, 0xcd, 0xe8);
        memset(candidateShape, 0xcd, 0xe8);
        reinterpret_cast<BoxCtor>(const_cast<unsigned char*>(base) +
            0x27db0)(oracleShape, 0, 0);
        new(candidateShape) MeshShape(0, 0);
        unsigned char oracleMesh[0x100] = {}, candidateMesh[0x100] = {};
        unsigned refcount = 7;
        memcpy(oracleMesh + 0x74, &refcount, 4);
        memcpy(candidateMesh + 0x74, &refcount, 4);
        void* pointer = oracleMesh;
        memcpy(oracleShape + 0xe0, &pointer, 4);
        pointer = candidateMesh;
        memcpy(candidateShape + 0xe0, &pointer, 4);
        const unsigned oracleBefore = oracleFreeCount;
        reinterpret_cast<DtorSlot>(const_cast<unsigned char*>(base) +
            0x28e80)(oracleShape, flags);
        const unsigned oracleFrees = oracleFreeCount - oracleBefore;
        const unsigned holderBefore = oracleFreeCount;
        const unsigned candidateBefore = candidateAllocator.freeCount;
        reinterpret_cast<MeshShape*>(candidateShape)->nxMeshScalarDeletingDtor(
            flags);
        unsigned oracleRefcount = 0, candidateRefcount = 0;
        memcpy(&oracleRefcount, oracleMesh + 0x74, 4);
        memcpy(&candidateRefcount, candidateMesh + 0x74, 4);
        const unsigned candidateFrees =
            candidateFreesSince(candidateAllocator, candidateBefore, holderBefore);
        oracleDigest = foldOracle(oracleDigest, &oracleFrees,
            sizeof(oracleFrees));
        oracleDigest = foldOracle(oracleDigest, &oracleRefcount,
            sizeof(oracleRefcount));
        if(oracleFrees != 1 + flags || candidateFrees != oracleFrees ||
           oracleRefcount != 6 || candidateRefcount != oracleRefcount) {
            fprintf(stderr, "mesh slot 0 flags=%u frees=%u/%u ref=%u/%u differs\n",
                flags, oracleFrees, candidateFrees,
                oracleRefcount, candidateRefcount);
            ++failures;
        }
        if(flags == 0) {
            free(oracleShape);
            free(candidateShape);
        }
        ++cases;
    }
    // MESH slot 4: low flag bits bypass mass work. Otherwise, a valid
    // nonnegative cache at mesh+0xb0 supplies a 13-word mass frame.
    typedef bool (__thiscall* MeshMassSlot)(void*, MassFrame*, float, unsigned);
    for(unsigned sample = 0; sample < 48; ++sample) {
        unsigned char oracleShape[0xe8] = {}, candidateShape[0xe8] = {};
        unsigned char meshRecord[0xe8] = {};
        const unsigned short flags = sample % 3 == 0 ? 1u :
            sample % 3 == 1 ? 7u : 8u;
        memcpy(oracleShape + 0xde, &flags, 2);
        memcpy(candidateShape + 0xde, &flags, 2);
        void* pointer = meshRecord;
        memcpy(oracleShape + 0xe0, &pointer, 4);
        memcpy(candidateShape + 0xe0, &pointer, 4);
        const float cacheMass = 1.25f + 0.137f * sample;
        memcpy(meshRecord + 0xb0, &cacheMass, 4);
        for(unsigned k = 0; k < 12; ++k) {
            const float value = (k + 1) * 0.071f + sample * 0.019f;
            memcpy(meshRecord + 0xb4 + 4*k, &value, 4);
        }
        for(unsigned side = 0; side < 2; ++side) {
            unsigned char* shape = side ? candidateShape : oracleShape;
            for(unsigned row = 0; row < 3; ++row) {
                for(unsigned col = 0; col < 3; ++col) {
                    const float value = row == col ?
                        1.0f + 0.013f * sample :
                        ((row + col + sample) % 2 ? -1.0f : 1.0f) *
                        0.017f * (row + col + 1);
                    memcpy(shape + 0x6c + 12*row + 4*col, &value, 4);
                }
                const float translation =
                    (row + 1) * (sample + 1) * 0.011f;
                memcpy(shape + 0x90 + 4*row, &translation, 4);
            }
        }
        unsigned oracleFrame[13], candidateFrame[13];
        for(unsigned k = 0; k < 13; ++k) {
            const float value = 0.2f + k * 0.037f;
            memcpy(oracleFrame + k, &value, 4);
            memcpy(candidateFrame + k, &value, 4);
        }
        const bool ro = reinterpret_cast<MeshMassSlot>(
            const_cast<unsigned char*>(base) + 0x28e10)(
            oracleShape, reinterpret_cast<MassFrame*>(oracleFrame), 2.0f, 0);
        const bool rc = reinterpret_cast<MeshShape*>(candidateShape)->
            nxMeshAccumulateMassCached(
                reinterpret_cast<MassFrame*>(candidateFrame), 2.0f, 0);
        oracleDigest = foldOracle(oracleDigest, &ro, sizeof(ro));
        oracleDigest = foldOracle(oracleDigest, oracleFrame,
            sizeof(oracleFrame));
        if(ro != rc || memcmp(oracleFrame, candidateFrame,
                sizeof(oracleFrame)) != 0) {
            fprintf(stderr, "mesh slot 4 cached sample=%u differs\n", sample);
            for(unsigned k = 0; k < 13; ++k)
                if(oracleFrame[k] != candidateFrame[k])
                    fprintf(stderr, "  word%u oracle=%08x candidate=%08x\n",
                        k, oracleFrame[k], candidateFrame[k]);
            ++failures;
        }
        ++cases;
    }
    // Mass-frame rows (scene-raycast Task 4, shape rows): 000833's translate
    // over non-finite and signed-zero offsets and steps, where its
    // x*[0x101041f0] addends show, over both its centered (c == 0) and
    // displaced paths; and 000829's box build over irregular half-extents,
    // where its m32 spills of F and the pairwise sums show. Reported on
    // their own line so the registered digest line above is unchanged.
    unsigned massCases = 0, massFailures = 0, massDigest = 2166136261u;
    {
    typedef void (__thiscall* FrameArgFn)(void*, const void*);
    FrameArgFn oracleTranslate = reinterpret_cast<FrameArgFn>(
        const_cast<unsigned char*>(base) + 0x1c040);
    FrameArgFn oracleBuildBox = reinterpret_cast<FrameArgFn>(
        const_cast<unsigned char*>(base) + 0x1bd00);
    const unsigned specialBits[] = {
        0x7f800000u, 0xff800000u, 0x7fc00000u, 0x80000000u, 0x00000000u,
        0x3fc00000u, 0xc0100000u, 0x3dcccccdu };
    const unsigned specialCount = sizeof(specialBits) / sizeof(specialBits[0]);
    for(unsigned oi = 0; oi < specialCount; ++oi)
    for(unsigned di = 0; di < specialCount; ++di)
    for(unsigned shape = 0; shape < 3; ++shape) {
        float frame[13];
        for(unsigned k = 0; k < 9; ++k)
            frame[k] = 0.5f + 0.25f * static_cast<float>(k);
        unsigned o[3] = { specialBits[oi], specialBits[(oi + shape + 1) % specialCount],
            0x3f000000u };
        unsigned d[3] = { specialBits[di], 0xbf400000u,
            specialBits[(di + 2 * shape) % specialCount] };
        if(shape == 2) {
            // centered path: d = -o, so c lands exactly on zero
            o[2] = 0x3f000000u;
            o[0] = 0x3fc00000u; o[1] = 0xc0100000u;
            d[0] = o[0] ^ 0x80000000u; d[1] = o[1] ^ 0x80000000u;
            d[2] = o[2] ^ 0x80000000u;
            if(di & 1) { o[0] = specialBits[oi]; d[0] = o[0] ^ 0x80000000u; }
        }
        memcpy(frame + 9, o, sizeof(o));
        frame[12] = 2.5f;
        unsigned char oracleFrame[0x34], candidateFrame[0x34];
        memcpy(oracleFrame, frame, sizeof(oracleFrame));
        memcpy(candidateFrame, frame, sizeof(candidateFrame));
        oracleTranslate(oracleFrame, d);
        reinterpret_cast<MassFrame*>(candidateFrame)->nxMassFrameTranslate(d);
        massDigest = foldOracle(massDigest, oracleFrame, sizeof(oracleFrame));
        if(memcmp(oracleFrame, candidateFrame, sizeof(oracleFrame)) != 0) {
            fprintf(stderr, "masstranslate o=%u d=%u shape=%u differs\n", oi, di, shape);
            ++massFailures;
        }
        ++massCases;
    }
    const float boxExtents[][3] = {
        {0.7f, 1.3f, 2.1f}, {0.1f, 3.3f, 0.37f}, {1.0f, 1.0f, 1.0f},
        {0.0f, 2.2f, 5.9f}, {1e-3f, 7.77f, 0.123f}, {4.4f, 0.0f, 0.0f},
        // extents where rounding F and the sums to m32 changes a diagonal
        {5.62f, 6.69f, 7.17f}, {0.31f, 4.22f, 8.49f}, {5.86f, 8.11f, 1.06f} };
    for(unsigned b = 0; b < sizeof(boxExtents) / sizeof(boxExtents[0]); ++b) {
        unsigned char oracleFrame[0x34], candidateFrame[0x34];
        memset(oracleFrame, 0xcd, sizeof(oracleFrame));
        memset(candidateFrame, 0xcd, sizeof(candidateFrame));
        oracleBuildBox(oracleFrame, boxExtents[b]);
        reinterpret_cast<MassFrame*>(candidateFrame)->nxMassFrameBuildBox(boxExtents[b]);
        massDigest = foldOracle(massDigest, oracleFrame, sizeof(oracleFrame));
        if(memcmp(oracleFrame, candidateFrame, sizeof(oracleFrame)) != 0) {
            fprintf(stderr, "massbox b=%u differs\n", b);
            ++massFailures;
        }
        ++massCases;
    }
    }
    // Mass-frame recentering row (000841): negate the old COM offset, then
    // route through the exact translate row. Directly call the oracle entry
    // because the only production caller (actor mass aggregation) is not yet
    // reconstructed; keep the frame's inertia and mass words observable too.
    unsigned centreCases = 0, centreFailures = 0, centreDigest = 2166136261u;
    {
    typedef void (__thiscall* TranslateCentreFn)(void*);
    TranslateCentreFn oracleTranslateCentre = reinterpret_cast<TranslateCentreFn>(
        const_cast<unsigned char*>(base) + 0x1c720);
    const float offsets[][3] = {
        {1.25f, -0.75f, 2.5f}, {-0.0f, 0.0f, -3.0f},
        {-4.4f, 0.1f, -0.123f}, {0.0f, 0.0f, 0.0f} };
    for(unsigned sample = 0; sample < sizeof(offsets) / sizeof(offsets[0]); ++sample) {
        unsigned char oracleFrame[0x34], candidateFrame[0x34];
        for(unsigned k = 0; k < 9; ++k) {
            const float value = 0.31f + 0.27f * static_cast<float>(k + sample);
            memcpy(oracleFrame + 4*k, &value, 4);
            memcpy(candidateFrame + 4*k, &value, 4);
        }
        const float mass = 2.75f + static_cast<float>(sample);
        memcpy(oracleFrame + 0x24, offsets[sample], 12);
        memcpy(candidateFrame + 0x24, offsets[sample], 12);
        memcpy(oracleFrame + 0x30, &mass, 4);
        memcpy(candidateFrame + 0x30, &mass, 4);
        oracleTranslateCentre(oracleFrame);
        reinterpret_cast<MassFrame*>(candidateFrame)->nxMassFrameTranslateToCentre();
        centreDigest = foldOracle(centreDigest, oracleFrame, sizeof(oracleFrame));
        if(memcmp(oracleFrame, candidateFrame, sizeof(oracleFrame)) != 0) {
            fprintf(stderr, "massframe centre sample=%u differs\n", sample);
            ++centreFailures;
        }
        ++centreCases;
    }
    }
    // BOX slot 7 (000951) through its slab test (001730) on rotated,
    // translated boxes, with directions that lie inside the +-2^-23 parallel
    // band on some axes, exactly on it, negative, and non-finite.
    unsigned sweepCases = 0, sweepFailures = 0, sweepDigest = 2166136261u;
    {
    typedef void (__thiscall* SweepBoxCtor)(void*, void*, unsigned);
    typedef bool (__thiscall* SweepFn)(void*, void*, const void*);
    SweepFn oracleSweep = reinterpret_cast<SweepFn>(
        const_cast<unsigned char*>(base) + 0x20b20);
    const float sweepRotations[3][9] = {
        {1,0,0, 0,1,0, 0,0,1},
        {0.36f, 0.48f, -0.8f, -0.8f, 0.6f, 0.0f, 0.48f, 0.64f, 0.6f},
        {0.8660254f, -0.5f, 0.0f, 0.5f, 0.8660254f, 0.0f, 0.0f, 0.0f, 1.0f} };
    const float sweepTranslations[2][3] = { {0,0,0}, {1.25f, -3.5f, 0.75f} };
    const float sweepDims[2][3] = { {0.7f, 1.3f, 2.1f}, {3.0f, 0.25f, 1.0f} };
    const float sweepDirs[][3] = {
        {1,0,0}, {0.3f, -0.9f, 0.2f}, {-2.0f, 1e-8f, 0.5f},
        {1.1920929e-7f, -1.1920929e-7f, 1.0f}, {5e-8f, -5e-8f, 2e-8f},
        {-0.25f, -0.5f, -4.0f}, {0.0f, 0.0f, 0.0f} };
    for(unsigned r = 0; r < 3; ++r)
    for(unsigned t = 0; t < 2; ++t)
    for(unsigned m = 0; m < 2; ++m)
    for(unsigned dd = 0; dd < sizeof(sweepDirs) / sizeof(sweepDirs[0]); ++dd) {
        unsigned char oracleBox[0x228], candidateBox[0x228];
        memset(oracleBox, 0xcd, sizeof(oracleBox));
        memset(candidateBox, 0xcd, sizeof(candidateBox));
        reinterpret_cast<SweepBoxCtor>(const_cast<unsigned char*>(base) + 0x21870)(
            oracleBox, 0, 0);
        BoxShape& candidateShape = *new(candidateBox) BoxShape(0, 0);
        memcpy(oracleBox + 0x0c, sweepRotations[r], 36);
        memcpy(candidateBox + 0x0c, sweepRotations[r], 36);
        memcpy(oracleBox + 0x30, sweepTranslations[t], 12);
        memcpy(candidateBox + 0x30, sweepTranslations[t], 12);
        memcpy(oracleBox + 0xe4, sweepDims[m], 12);
        memcpy(candidateBox + 0xe4, sweepDims[m], 12);
        float oracleOut = 0.5f, candidateOut = 0.5f;
        const bool ro = oracleSweep(oracleBox, &oracleOut, sweepDirs[dd]);
        const bool rc = candidateShape.nxBoxSweep(&candidateOut, sweepDirs[dd]);
        const unsigned oracleWords[2] = { ro ? 1u : 0u, 0u };
        sweepDigest = foldOracle(sweepDigest, oracleWords, 4);
        sweepDigest = foldOracle(sweepDigest, &oracleOut, 4);
        if(ro != rc || memcmp(&oracleOut, &candidateOut, 4) != 0) {
            fprintf(stderr, "boxsweep r=%u t=%u m=%u d=%u differs\n", r, t, m, dd);
            ++sweepFailures;
        }
        ++sweepCases;
    }
    }
    const BoxHullResult hull = runBoxHullCases(base);
    nxSetSdkAllocatorBridge(0);
    printf("shape vtable oracle_digest=%08x cases=%u mismatches=%u\n",
        oracleDigest, cases, failures);
    printf("shape vtable base_stubs oracle_digest=%08x cases=%u mismatches=%u\n",
        baseStubDigest, baseStubCases, baseStubFailures);
    printf("shape vtable boxmass oracle_digest=%08x cases=%u failures=%u\n",
        boxMassDigest, boxMassCases, boxMassFailures);
    printf("shape vtable capsule_load_return oracle=%u candidate=%u\n",
        capsuleLoadOracleReturn, capsuleLoadCandidateReturn);
    printf("shape vtable massframe oracle_digest=%08x cases=%u failures=%u\n",
        massDigest, massCases, massFailures);
    printf("shape vtable massframe centre oracle_digest=%08x cases=%u failures=%u\n",
        centreDigest, centreCases, centreFailures);
    printf("shape vtable boxsweep oracle_digest=%08x cases=%u failures=%u\n",
        sweepDigest, sweepCases, sweepFailures);
    printf("box hull oracle_digest=%08x cases=%u failures=%u\n",
        hull.digest, hull.cases, hull.failures);
    if(capsuleLoadOracleReturn != capsuleLoadCandidateReturn || baseStubFailures ||
       massFailures ||
       centreFailures ||
       boxMassFailures ||
       sweepFailures || hull.failures)
        return 1;
    return failures ? 1 : 0;
}

// ObjectModel.cpp's BOX slot 3 (phys_fn_000945) reads the SDK's live parameter
// array through nxPhysicsSDKParameters, which PhysicsSDK.cpp defines in the DLL.
// This harness does not link PhysicsSDK.cpp and never calls slot 3, so it
// supplies an all-zero array (every visualisation parameter at its default;
// 128 covers NxParameter).
const NxReal* nxPhysicsSDKParameters()
{
    static const NxReal parameters[128] = {0};
    return parameters;
}
