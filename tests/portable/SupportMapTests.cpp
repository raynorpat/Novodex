#include "FixtureSupport.h"
#include "IceSupportMaps.h"
#include "NxScalarConversions.h"
#include "NxSdkAllocator.h"
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#undef for
#if NX_PHYSICS_USE_X87
#include <float.h>
#define RECEIVER(p) p, 0
using DeleteSlot = IceSupportMap *(__fastcall *)(IceSupportMap *, NxU32, NxU32);
using AllocateSlot = bool(__fastcall *)(IceSupportMap *);
using ComputeSlot = bool(__fastcall *)(IceSupportMap *, NxU32, NxU32, const IceMaths::Point *);
using FinishSlot = void(__fastcall *)(IceSupportMap *);
#else
#define RECEIVER(p) p
using DeleteSlot = NxSupportMapDeleteSlot;
using AllocateSlot = NxSupportMapAllocateSlot;
using ComputeSlot = NxSupportMapComputeSlot;
using FinishSlot = NxSupportMapFinishSlot;
#endif
static_assert(sizeof(IceSupportMap) == 0x18 && offsetof(IceSupportMap, mVertexSource) == 0x14,
              "raw32 support-map receiver");
static_assert(sizeof(ConvexHull) == 0x4c && offsetof(ConvexHull, mPolygons) == 0x28, "actual hull receiver");
static_assert(sizeof(HullPolygon) == 0x24 && sizeof(Valencies) == 0x14, "actual polygon and graph receivers");
static unsigned failures, fixtureId, reportLine;
static void check(bool ok, const char *why)
{
    if (!ok)
    {
        std::fprintf(stderr, "FAIL %s group=%u\n", why, fixtureId);
        ++failures;
    }
}
// Existing documented fixture-host error/allocator boundaries; every physics,
// topology, support, graph and vendor routine is the production implementation.
bool opcNovodeXSetIceError(const char *, const char *, int line)
{
    reportLine = line;
    return false;
}
void *opcNovodeXAlloc(size_t n)
{
    return nxGetSdkAllocator()->malloc(n, NX_MEMORY_PERSISTENT);
}
void opcNovodeXFree(void *p)
{
    nxGetSdkAllocator()->free(p);
}
class GuardAllocator : public SdkAllocator
{
  public:
    std::map<void *, size_t> blocks;
    unsigned calls = 0, frees = 0, failCall = 0;
    std::vector<void *> freed;
    void *malloc(size_t n, NxMemoryType)
    {
        ++calls;
        if (calls == failCall)
            return 0;
        unsigned char *raw = (unsigned char *)std::malloc(n + 32);
        if (!raw)
            return 0;
        std::memset(raw, 0x6a, 16);
        std::memset(raw + 16, 0xcd, n);
        std::memset(raw + 16 + n, 0x7b, 16);
        blocks[raw + 16] = n;
        return raw + 16;
    }
    void *mallocDEBUG(size_t n, const char *, int, const char *, NxMemoryType t)
    {
        return malloc(n, t);
    }
    void *realloc(void *p, size_t n)
    {
        if (!p)
            return malloc(n, NX_MEMORY_PERSISTENT);
        void *q = malloc(n, NX_MEMORY_PERSISTENT);
        if (q)
        {
            std::memcpy(q, p, n < blocks[p] ? n : blocks[p]);
            free(p);
        }
        return q;
    }
    void canaries()
    {
        for (const auto &b : blocks)
        {
            const unsigned char *raw = (unsigned char *)b.first - 16;
            for (unsigned j = 0; j < 16; ++j)
                check(raw[j] == 0x6a && raw[16 + b.second + j] == 0x7b, "allocator canaries");
        }
    }
    void free(void *p)
    {
        if (!p)
            return;
        auto it = blocks.find(p);
        check(it != blocks.end(), "owned allocator block");
        if (it == blocks.end())
            return;
        canaries();
        ++frees;
        freed.push_back(p);
        blocks.erase(it);
        std::free((unsigned char *)p - 16);
    }
};
struct Observation
{
    unsigned kind, id, index, word;
};
static std::vector<Observation> observations;
static void exact(unsigned value)
{
    observations.push_back({0, fixtureId, (unsigned)observations.size(), value});
}
static float f(unsigned word)
{
    float value;
    std::memcpy(&value, &word, 4);
    return value;
}
static unsigned word(float value)
{
    unsigned result;
    std::memcpy(&result, &value, 4);
    return result;
}
static void coordinate(float value)
{
    observations.push_back({1, fixtureId, (unsigned)observations.size(), word(value)});
}
// Literal binary32 words: axes/signs/ties/adjacent-face boundaries, sample
// half-index neighbours, decimal ratios, exact signedzero and nonfinite input.
static const unsigned directions[][3] = {{0x3f800000, 0, 0},
                                         {0xbf800000, 0, 0},
                                         {0, 0x3f800000, 0},
                                         {0, 0xbf800000, 0},
                                         {0, 0, 0x3f800000},
                                         {0, 0, 0xbf800000},
                                         {0x3f800000, 0x3f800000, 0x3f800000},
                                         {0xbf800000, 0x3f800000, 0x3f800000},
                                         {0x3f800000, 0xbf800000, 0xbf800000},
                                         {0x3f800000, 0x3f800001, 0x3f800000},
                                         {0x3f800000, 0x3f7fffff, 0x3f800000},
                                         {0x3f800000, 0x3f800000, 0x3f800001},
                                         {0, 0x3f800000, 0x3f800000},
                                         {0, 0xbf800000, 0x3f800000},
                                         {0x3f800000, 0, 0x3f800000},
                                         {0x3f800000, 0x3effffff, 0},
                                         {0x3f800000, 0x3f000000, 0},
                                         {0x3f800000, 0x3f000001, 0},
                                         {0x3f800000, 0xbeffffff, 0},
                                         {0x3f800000, 0xbf000000, 0},
                                         {0x3f800000, 0xbf000001, 0},
                                         {0x3f800000, 0x3e7fffff, 0x3f3fffff},
                                         {0x3f800000, 0x3e800000, 0x3f400000},
                                         {0x3f800000, 0x3e800001, 0x3f400001},
                                         {0x3e800000, 0xbf000000, 0x3fa00000},
                                         {0xbf800000, 0x40000000, 0xc0400000},
                                         {0x3dcccccd, 0x3e4ccccd, 0x3e99999a},
                                         {0x3f800000, 0x80000000, 0},
                                         {0x80000000, 0xbf800000, 0x80000000},
                                         {0, 0, 0},
                                         {0x80000000, 0, 0},
                                         {0x3f800000, 0x33d6bf94, 0},
                                         {0x3f800000, 0x33d6bf95, 0},
                                         {0x3f800000, 0x33d6bf96, 0},
                                         {0x7f800000, 0x3f800000, 0},
                                         {0xff800000, 0x3f800000, 0},
                                         {0x7fc00001, 0x3f800000, 0},
                                         {0x7f800001, 0x3f800000, 0},
                                         {0xffc00001, 0x3f800000, 0},
                                         {0x7f800000, 0x7f800000, 0},
                                         {0x7f7fffff, 0xff7fffff, 0x3f800000},
                                         {0x00800000, 0x00800000, 0x00800000},
                                         {1, 0, 0}};
static IceMaths::Point direction(unsigned index)
{
    const unsigned *d = directions[index];
    return IceMaths::Point(f(d[0]), f(d[1]), f(d[2]));
}
static const unsigned directionCount = sizeof(directions) / sizeof(directions[0]);
static unsigned tableIndex(const IceSupportMap &m)
{
    const void *const *tables[] = {gIceSupportMapBaseTable, gIceSupportMapHullTable, gIceSupportMapPlaneTable,
                                   gIceSupportMapVertexTable};
    for (unsigned i = 0; i < 4; ++i)
        if (m.mVtable == tables[i])
            return i;
    return 0xdead;
}
static void releaseHull(ConvexHull &h)
{
    if (h.mPolygons)
        nxIceDeleteArray(h.mPolygons);
    if (h.mEdges)
        nxIceDeleteArray(h.mEdges);
    if (h.mPolygonVRefs)
        nxIceFree(h.mPolygonVRefs);
    if (h.mPolygonERefs)
        nxIceFree(h.mPolygonERefs);
    if (h.mEdgeNormals)
        nxIceFree(h.mEdgeNormals);
    if (h.mEdgeToPolygons)
        nxIceFree(h.mEdgeToPolygons);
    if (h.mEdgePolygons)
        nxIceFree(h.mEdgePolygons);
    if (h.mEdgeAxes)
    {
        h.mEdgeAxes->~Container();
        std::free(h.mEdgeAxes);
    }
}
static void construct(IceSupportMap &map, unsigned kind, ConvexHull &hull)
{
    std::memset(&map, 0xcd, sizeof(map));
    if (kind == 0)
        exact(nxSupportMapHullConstruct(RECEIVER(&map), &hull) == &map);
    else if (kind == 1)
        exact(nxSupportMapPlaneConstruct(RECEIVER(&map), &hull) == &map);
    else
        exact(nxSupportMapVertexConstruct(RECEIVER(&map), &hull) == &map);
    exact(tableIndex(map));
    exact(map.mSubdiv);
    exact(map.mNbSamples);
    exact(map.mSamples == 0);
    exact(kind == 2 ? map.mSamples2 == 0 && map.mVertexSource == &hull : map.mHull == &hull);
    if (kind != 2)
        exact((size_t)map.mVertexSource == 0xcdcdcdcd);
}
static void observeMap(const IceSupportMap &m, unsigned kind, GuardAllocator &allocator)
{
    exact(m.mSubdiv);
    exact(m.mNbSamples);
    exact(m.mSamples != 0);
    if (m.mSamples)
    {
        exact((unsigned)allocator.blocks[m.mSamples]);
        for (unsigned i = 0; i < m.mNbSamples; ++i)
            exact(m.mSamples[i]);
    }
    if (kind == 2)
    {
        exact(m.mSamples2 != 0);
        if (m.mSamples2)
        {
            exact((unsigned)allocator.blocks[m.mSamples2]);
            for (unsigned i = 0; i < m.mNbSamples; ++i)
                exact(m.mSamples2[i]);
        }
    }
    allocator.canaries();
}
static void cubeLookup()
{
    const unsigned subdivisions[] = {0, 1, 2, 3, 4, 5, 8, 17, 0x7fffffff, 0x80000000, 0xffffffff};
    for (unsigned d = 0; d < directionCount; ++d)
    {
        IceMaths::Point dir = direction(d);
        float u = f(0xcdcd0001), v = f(0xcdcd0002);
        exact(nxSupportMapCubeFace(&dir, &u, &v));
        coordinate(u);
        coordinate(v);
        const unsigned *literal = directions[d];
        if ((literal[0] & 0x7fffffff) > 0x7f800000)
            check(std::isnan(u) && std::isnan(v), "literal s/qNaN dominant cube coordinate classes");
        if ((literal[0] & 0x7fffffff) == 0x7f800000 && literal[1] == 0x3f800000)
            check(u == 0.0f && v == 0.0f, "infinite dominant reciprocal has zero finite coordinates");
        if ((literal[0] & 0x7fffffff) == 0 && literal[1] == 0 && literal[2] == 0)
        {
            check(std::isnan(u) && std::isnan(v), "zero dominant cube coordinate classes");
            IceSupportMap privateMap;
            std::memset(&privateMap, 0, sizeof(privateMap));
            privateMap.mSubdiv = 5;
            check(nxSupportMapLookup(RECEIVER(&privateMap), &dir) == ((literal[0] >> 31) * 25),
                  "invalid qword low32 zero gives exact zero-direction face index");
        }
        for (unsigned n : subdivisions)
        {
            IceSupportMap m;
            std::memset(&m, 0xcd, sizeof(m));
            m.mSubdiv = n;
            exact(nxSupportMapLookup(RECEIVER(&m), &dir));
        }
        ++fixtureId;
    }
    for (unsigned bits = 0; bits < 8; ++bits)
    {
        IceMaths::Point dir(f(bits & 1 ? 0xbf800000 : 0x3f800000), f(bits & 2 ? 0xbf800000 : 0x3f800000),
                            f(bits & 4 ? 0xbf800000 : 0x3f800000));
        float u, v;
        check(nxSupportMapCubeFace(&dir, &u, &v) == (bits & 1), "x wins all magnitude ties");
    }
}
static void conversions()
{
    // Exact private qword-low32 contract from Task3: no int32 saturation or
    // truncation. Binary64 inputs and independently specified output words.
    static const unsigned long long inputs[] = {0,
                                                0x8000000000000000ULL,
                                                0x3fe0000000000000ULL,
                                                0x3ff8000000000000ULL,
                                                0x4004000000000000ULL,
                                                0xbff8000000000000ULL,
                                                0x3fdfffffffffffffULL,
                                                0x3fe0000000000001ULL,
                                                0x41dfffffffe00000ULL,
                                                0x41e0000000000000ULL,
                                                0x41f0000000080000ULL,
                                                0x41f0000000180000ULL,
                                                0xc1f0000000180000ULL,
                                                0x43dfffffffffffffULL,
                                                0x43e0000000000000ULL,
                                                0xc3e0000000000000ULL,
                                                0xc3e0000000000001ULL,
                                                0x7ff0000000000000ULL,
                                                0xfff0000000000000ULL,
                                                0x7ff8000000000001ULL};
    static const unsigned outputs[] = {0, 0,          0,          2, 2, 0xfffffffe, 0,
                                       1, 0x80000000, 0x80000000, 0, 2, 0xfffffffe, 0xfffffc00,
                                       0, 0,          0,          0, 0, 0};
    for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i)
    {
        double value;
        std::memcpy(&value, &inputs[i], 8);
        check((unsigned)nxScalarFistpLow32(value) == outputs[i],
              "private qword rounding/low32/invalid contract");
    }
}
#include "SupportMapDomainInputs.h"
static const NxU16 cubeFaces[] = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
                                  3, 7, 6, 3, 6, 2, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5};
static const NxU16 tetraFaces[] = {0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3};
static void mapDomain(GuardAllocator &allocator)
{
    const unsigned subdivisions[] = {0, 1, 2, 3, 5, 8};
    for (unsigned tr = 0; tr < 4; ++tr)
        for (unsigned shape = 0; shape < 2; ++shape)
        {
            IceMaths::Point verts[8];
            unsigned nb = shape ? 4 : 8;
            for (unsigned i = 0; i < nb; ++i)
            {
                const unsigned *p = meshWords[tr * 2 + shape][i];
                verts[i].Set(f(p[0]), f(p[1]), f(p[2]));
            }
            ConvexHull hull;
            std::memset(&hull, 0, sizeof(hull));
            hull.mVerts = verts;
            hull.mNbVerts = nb;
            hull.mFaces = shape ? tetraFaces : cubeFaces;
            hull.mNbFaces = shape ? 4 : 12;
            // Real centroid producer populates the value B borrows. Polygon builder
            // computes its own orientation scratch and does not store this field.
            exact(nxHullComputeCentroid(RECEIVER(&hull), &hull.mCentroid));
            {
                IceSupportMap lazy;
                construct(lazy, 1, hull);
                lazy.mNbSamples = 1;
                lazy.mSamples = (NxU8 *)allocator.malloc(1, NX_MEMORY_PERSISTENT);
                IceMaths::Point dir = direction(0);
                exact(nxSupportMapPlaneCompute(RECEIVER(&lazy), 0, &dir));
                exact(lazy.mSamples[0]);
                exact(hull.mNbPolygons);
                nxSupportMapPlaneDelete(RECEIVER(&lazy), 0);
            }
            for (unsigned kind = 0; kind < 3; ++kind)
                for (unsigned n : subdivisions)
                {
                    IceSupportMap map;
                    construct(map, kind, hull);
                    exact(nxSupportMapInit(RECEIVER(&map), n));
                    observeMap(map, kind, allocator);
                    for (unsigned d = 0; d < 29; ++d)
                    {
                        IceMaths::Point dir = direction(d);
                        unsigned sample = nxSupportMapLookup(RECEIVER(&map), &dir);
                        exact(sample);
                        if (sample < map.mNbSamples)
                        {
                            exact(map.mSamples[sample]);
                            if (kind == 2)
                                exact(map.mSamples2[sample]);
                        }
                    }
                    const unsigned freeStart = allocator.frees;
                    NxU8 *first = map.mSamples;
                    NxU8 *second = kind == 2 ? map.mSamples2 : 0;
                    auto destructor = (DeleteSlot)map.mVtable[0];
                    exact(destructor(RECEIVER(&map), 2) == &map);
                    exact(tableIndex(map));
                    exact(map.mSamples == 0);
                    if (kind == 2)
                        exact(map.mSamples2 == 0);
                    exact(allocator.frees - freeStart);
                    if (second && first)
                        check(allocator.freed[freeStart] == second && allocator.freed[freeStart + 1] == first,
                              "C releases second map before first");
                    exact(destructor(RECEIVER(&map), 0) == &map);
                    ++fixtureId;
                }
            // Every direct allocate/compute argument and genuine slot is exercised.
            for (unsigned kind = 0; kind < 3; ++kind)
            {
                IceSupportMap map;
                construct(map, kind, hull);
                map.mNbSamples = 41;
                exact(((AllocateSlot)map.mVtable[1])(&map));
                for (unsigned d = 0; d < 34; ++d)
                {
                    IceMaths::Point dir = direction(d);
                    exact(((ComputeSlot)map.mVtable[2])(RECEIVER(&map), d + 1, &dir));
                }
                exact(map.mSamples[0]);
                exact(map.mSamples[35]);
                if (kind == 2)
                {
                    exact(map.mSamples2[0]);
                    exact(map.mSamples2[35]);
                }
                ((FinishSlot)map.mVtable[3])(&map);
                observeMap(map, kind, allocator);
                // Repeated Init preserves original overwritten-allocation ownership.
                NxU8 *old1 = map.mSamples;
                NxU8 *old2 = kind == 2 ? map.mSamples2 : 0;
                exact(nxSupportMapInit(RECEIVER(&map), 3));
                observeMap(map, kind, allocator);
                check(allocator.blocks.count(old1) == 1 && (!old2 || allocator.blocks.count(old2) == 1),
                      "original Init abandons prior allocation");
                ((DeleteSlot)map.mVtable[0])(RECEIVER(&map), 0);
                allocator.free(old1);
                allocator.free(old2);
                IceSupportMap *owned =
                    (IceSupportMap *)allocator.malloc(sizeof(IceSupportMap), NX_MEMORY_PERSISTENT);
                construct(*owned, kind, hull);
                exact(nxSupportMapInit(RECEIVER(owned), 2));
                exact(((DeleteSlot)owned->mVtable[0])(RECEIVER(owned), 3) == owned);
                exact(allocator.blocks.count(owned) == 0);
                ++fixtureId;
            }
            // A freshly constructed A owns the actual allocation/lazy-build path.
            releaseHull(hull);
            std::memset(&hull, 0, sizeof(hull));
            hull.mVerts = verts;
            hull.mNbVerts = nb;
            hull.mFaces = shape ? tetraFaces : cubeFaces;
            hull.mNbFaces = shape ? 4 : 12;
            nxHullComputeCentroid(RECEIVER(&hull), &hull.mCentroid);
            {
                IceSupportMap lazy;
                construct(lazy, 0, hull);
                exact(nxSupportMapInit(RECEIVER(&lazy), 2));
                observeMap(lazy, 0, allocator);
                exact(hull.mNbPolygons);
                nxSupportMapHullDelete(RECEIVER(&lazy), 0);
            }
            Valencies graph;
            VALENCESCREATE c = {nb, hull.mNbFaces, 0, hull.mFaces, true};
            exact(graph.Compute(c));
            unsigned visited[10] = {0};
            visited[0] = 0x6a6a6a6a;
            visited[nb + 1] = 0x7b7b7b7b;
            unsigned stamp = 1;
            for (unsigned d = 0; d < 29; ++d)
                for (unsigned start = 0; start < nb; ++start)
                {
                    unsigned index = start;
                    IceMaths::Point dir = direction(d);
                    exact(nxHullClimbSupportVertex(&index, &dir, verts, &graph, stamp++, visited + 1));
                    exact(index);
                    check(visited[0] == 0x6a6a6a6a && visited[nb + 1] == 0x7b7b7b7b,
                          "graph scratch reuse canaries");
                }
            releaseHull(hull);
            ++fixtureId;
        }
}
static void failureDomain(GuardAllocator &allocator)
{
    ConvexHull source;
    std::memset(&source, 0, sizeof(source));
    IceSupportMap map;
    std::memset(&map, 0xcd, sizeof(map));
    exact(nxSupportMapBaseConstruct(&map) == &map);
    exact(tableIndex(map));
    exact(map.mSubdiv);
    exact(map.mNbSamples);
    exact((size_t)map.mSamples == 0xcdcdcdcd);
    IceSupportMap beforeNoop = map;
    nxSupportMapNoop(&map);
    check(std::memcmp(&beforeNoop, &map, sizeof(map)) == 0,
          "authoritative slot3 noop has no receiver writes");
    nxSupportMapBaseTable(&map);
    exact(nxSupportMapBaseDelete(RECEIVER(&map), 2) == &map);
    IceSupportMap *base = (IceSupportMap *)allocator.malloc(sizeof(map), NX_MEMORY_PERSISTENT);
    nxSupportMapBaseConstruct(base);
    exact(nxSupportMapBaseDelete(RECEIVER(base), 1) == base);
    exact(allocator.blocks.count(base) == 0);
    // Source count over255 fails before allocation; actual receiver types,
    // never fabricated raw receiver layouts or physics callback tables.
    source.mNbPolygons = source.mNbVerts = 256;
    for (unsigned kind = 0; kind < 3; ++kind)
    {
        construct(map, kind, source);
        unsigned before = allocator.calls;
        exact(nxSupportMapInit(RECEIVER(&map), 2));
        exact(allocator.calls - before);
        exact(map.mSubdiv);
        exact(map.mNbSamples);
        exact(map.mSamples == 0);
        ((DeleteSlot)map.mVtable[0])(RECEIVER(&map), 0);
        ++fixtureId;
    }
    source.mNbPolygons = 1;
    source.mNbVerts = 0;
    for (unsigned kind = 0; kind < 3; ++kind)
    {
        construct(map, kind, source);
        allocator.failCall = allocator.calls + 1;
        exact(nxSupportMapInit(RECEIVER(&map), 3));
        exact(map.mSamples == 0);
        allocator.failCall = 0;
        ((DeleteSlot)map.mVtable[0])(RECEIVER(&map), 0);
        ++fixtureId;
    }
    construct(map, 2, source);
    allocator.failCall = allocator.calls + 2;
    exact(nxSupportMapInit(RECEIVER(&map), 3));
    exact(map.mSamples != 0);
    exact(map.mSamples2 == 0);
    allocator.failCall = 0;
    nxSupportMapVertexRelease(&map);
    exact(map.mSamples == 0);
    exact(tableIndex(map));
    // Empty vertex source and coincident/collinear vertices are supported by C.
    for (unsigned count = 0; count < 10; ++count)
    {
        IceMaths::Point verts[255];
        for (unsigned i = 0; i < 255; ++i)
            verts[i].Set(float(i % 4), 0, 0);
        source.mNbVerts = count == 9 ? 255 : count;
        source.mVerts = verts;
        construct(map, 2, source);
        exact(nxSupportMapInit(RECEIVER(&map), 2));
        observeMap(map, 2, allocator);
        for (unsigned d = 0; d < 29; ++d)
        {
            IceMaths::Point dir = direction(d);
            exact(nxSupportMapVertexCompute(RECEIVER(&map), 7, &dir));
            exact(map.mSamples[7]);
            exact(map.mSamples2[7]);
        }
        nxSupportMapVertexDelete(RECEIVER(&map), 0);
        ++fixtureId;
    }
}
static void vertexCancellationDomain(GuardAllocator &allocator)
{
    // Non-grid ordinary source vertices exercise every unrolled lane/tail,
    // float winner spills, ties and cancellation with both direction signs.
    static const unsigned vertices[][3] = {
        {0x4b800000, 0xcb800000, 0x3dcccccd}, {0x4b800000, 0xcb800000, 0x3e4ccccd},
        {0x4b800000, 0xcb800000, 0xbe99999a}, {0xcb800000, 0x4b800000, 0x3dcccccd},
        {0xcb800000, 0x4b800000, 0x3e4ccccd}, {0xcb800000, 0x4b800000, 0xbe99999a},
        {0x3dcccccd, 0x3e4ccccd, 0x3e99999a}, {0x3dccccce, 0x3e4ccccc, 0x3e99999b},
        {0x80000000, 0, 0x80000000}};
    IceMaths::Point points[9];
    for (unsigned i = 0; i < 9; ++i)
        points[i].Set(f(vertices[i][0]), f(vertices[i][1]), f(vertices[i][2]));
    ConvexHull source;
    std::memset(&source, 0, sizeof(source));
    source.mVerts = points;
    for (unsigned count = 1; count <= 9; ++count)
    {
        source.mNbVerts = count;
        IceSupportMap map;
        construct(map, 2, source);
        map.mNbSamples = 2;
        exact(nxSupportMapVertexAllocate(&map));
        for (unsigned d = 0; d < directionCount; ++d)
        {
            IceMaths::Point dir = direction(d);
            exact(nxSupportMapVertexCompute(RECEIVER(&map), 1, &dir));
            exact(map.mSamples[1]);
            exact(map.mSamples2[1]);
            check(map.mSamples[0] == 0xcd && map.mSamples2[0] == 0xcd,
                  "direct compute immediate sample canaries");
        }
        nxSupportMapVertexDelete(RECEIVER(&map), 0);
        ++fixtureId;
    }
    source.mNbVerts = 9;
    IceSupportMap retained;
    construct(retained, 2, source);
    exact(nxSupportMapInit(RECEIVER(&retained), 2));
    NxU8 *a = retained.mSamples;
    NxU8 *b = retained.mSamples2;
    source.mNbVerts = 256;
    unsigned calls = allocator.calls;
    exact(nxSupportMapInit(RECEIVER(&retained), 3));
    exact(allocator.calls - calls);
    exact(retained.mSamples == a && retained.mSamples2 == b);
    exact(retained.mSubdiv);
    exact(retained.mNbSamples);
    // No array dereference under the new count: old allocations remain24B.
    nxSupportMapVertexDelete(RECEIVER(&retained), 0);
    ++fixtureId;
    ConvexHull empty;
    std::memset(&empty, 0, sizeof(empty));
    for (unsigned n = 0; n < 3; ++n)
    {
        IceSupportMap map;
        construct(map, 1, empty);
        reportLine = 0;
        exact(nxSupportMapInit(RECEIVER(&map), n));
        exact(reportLine);
        observeMap(map, 1, allocator);
        nxSupportMapPlaneDelete(RECEIVER(&map), 0);
        ++fixtureId;
    }
    IceSupportMap noSamples;
    construct(noSamples, 0, empty);
    reportLine = 0;
    exact(nxSupportMapInit(RECEIVER(&noSamples), 0));
    exact(reportLine);
    observeMap(noSamples, 0, allocator);
    nxSupportMapHullDelete(RECEIVER(&noSamples), 0);
    releaseHull(empty);
    ++fixtureId;
}
static void put(FILE *out, unsigned v)
{
    unsigned char b[4] = {(unsigned char)v, (unsigned char)(v >> 8), (unsigned char)(v >> 16),
                          (unsigned char)(v >> 24)};
    std::fwrite(b, 1, 4, out);
}
static unsigned get(const unsigned char *p)
{
    return unsigned(p[0]) | (unsigned(p[1]) << 8) | (unsigned(p[2]) << 16) | (unsigned(p[3]) << 24);
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
#if NX_PHYSICS_USE_X87
    if ((_controlfp(0, 0) & (_MCW_PC | _MCW_RC)) != (_PC_53 | _RC_NEAR))
        return 3;
#endif
    check(nxScalarIsRoundToNearest(), "portable numeric contract requires nearest rounding");
    conversions();
    cubeLookup();
    GuardAllocator allocator;
    nxSetSdkAllocatorBridge(&allocator);
    mapDomain(allocator);
    failureDomain(allocator);
    vertexCancellationDomain(allocator);
    allocator.canaries();
    check(allocator.blocks.empty(), "all observed ownership released");
    nxSetSdkAllocatorBridge(0);
#if NX_PHYSICS_USE_X87
    FILE *out = std::fopen(argv[1], "wb");
    if (!out)
        return 4;
    std::fwrite("NXPF", 1, 4, out);
    put(out, 1);
    put(out, (unsigned)observations.size() * 20);
    put(out, 20);
    for (const auto &o : observations)
    {
        put(out, o.kind);
        put(out, o.id);
        put(out, o.index);
        put(out, 0);
        put(out, o.word);
    }
    std::fclose(out);
#else
    std::vector<unsigned char> bytes;
    std::string error;
    check(nxReadFixture(argv[1], bytes, error), error.c_str());
    check(bytes.size() == observations.size() * 20, "support-map observation count");
    double maxAbsolute = 0, maxRelative = 0;
    unsigned exactDifferences = 0, coordinateDifferences = 0;
    if (bytes.size() == observations.size() * 20)
        for (unsigned i = 0; i < observations.size(); ++i)
        {
            const auto &o = observations[i];
            const unsigned char *p = &bytes[i * 20];
            unsigned expected = get(p + 16);
            check(get(p) == o.kind && get(p + 4) == o.id && get(p + 8) == o.index && get(p + 12) == 0,
                  "fixture observation identity");
            if (o.word == expected)
                continue;
            double a = f(o.word), b = f(expected), difference = std::fabs(a - b),
                   relative = b ? difference / std::fabs(b) : difference;
            if (o.kind)
            {
                ++coordinateDifferences;
                if (difference > maxAbsolute)
                    maxAbsolute = difference;
                if (relative > maxRelative)
                    maxRelative = relative;
            }
            else
                ++exactDifferences;
            // Controller approved exact word equality (absolute0/relative0) for
            // this measured Win32 domain. NaN auxiliary payload equality is current
            // toolchain evidence; semantic classes and integer decisions also have
            // independent assertions above, without a future-platform payload claim.
            if (failures < 20)
                std::fprintf(stderr, "difference row=%u kind=%u actual=%08x reference=%08x\n", i, o.kind,
                             o.word, expected);
            check(false, "approved exact support-map comparison");
        }
    std::printf("support_maps groups=%u words=%zu exact_differences=%u coordinate_differences=%u "
                "coordinate_max_abs=%.17g coordinate_max_rel=%.17g failures=%u\n",
                fixtureId, observations.size(), exactDifferences, coordinateDifferences, maxAbsolute,
                maxRelative, failures);
#endif
    return failures ? 1 : 0;
}
