#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "FoundationSDK.h"
#include "NxSdkAllocator.h"
#include "FixtureSupport.h"
#include "TriangleMesh.h"
#include "TriangleMeshPolygons.h"
#include "PhysicsSDK.h"
#include "TriangleMeshConvexData.h"
#include "NxPolygonScratch.h"
#include "PMap.h"
#include "NxPMap.h"
#if NX_PHYSICS_USE_X87
#include <float.h>
#endif

using NxTriangleMeshPrivate::TriangleMeshConvexData;

using namespace Opcode;
// Literal final vertex words shared with the accepted support-map domain.
#include "SupportMapDomainInputs.h"

static unsigned failures, group;
struct Observation
{
    unsigned kind, group, index, reserved, word;
};
static std::vector<Observation> observations;
static std::vector<unsigned> quantities;
static void check(bool condition, const char *message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL group=%u %s\n", group, message);
        ++failures;
    }
}
static void exact(unsigned word)
{
    observations.push_back({0, group, unsigned(observations.size()), 0, word});
    quantities.push_back(0);
}
static void numeric(float value, unsigned kind = 1, unsigned quantity = 1)
{
    unsigned word;
    std::memcpy(&word, &value, 4);
    observations.push_back({kind, group, unsigned(observations.size()), 0, word});
    quantities.push_back(quantity);
}
static float value(unsigned word)
{
    float result;
    std::memcpy(&result, &word, 4);
    return result;
}
class GuardAllocator : public NxUserAllocator
{
  public:
    std::map<void *, size_t> blocks;
    unsigned allocations = 0, releases = 0, failAt = 0;
    void *mallocDEBUG(size_t size, const char *, int) override
    {
        return malloc(size);
    }
    void *malloc(size_t size) override
    {
        if (++allocations == failAt)
            return nullptr;
        unsigned char *block = static_cast<unsigned char *>(std::malloc(size + 32));
        if (!block)
            std::abort();
        std::memset(block, 0x6a, 16);
        std::memset(block + 16, 0xcd, size);
        std::memset(block + 16 + size, 0x7b, 16);
        blocks[block + 16] = size;
        return block + 16;
    }
    void canaries() const
    {
        for (const auto &block : blocks)
        {
            const unsigned char *bytes = static_cast<const unsigned char *>(block.first) - 16;
            for (unsigned i = 0; i < 16; ++i)
                if (bytes[i] != 0x6a || bytes[16 + block.second + i] != 0x7b)
                {
                    std::fprintf(
                        stderr, "CORRUPT size=%zu offset=%u prefix=%x suffix=%x allocations=%u releases=%u\n",
                        block.second, i, bytes[i], bytes[16 + block.second + i], allocations, releases);
                    std::abort();
                }
        }
    }
    void free(void *pointer) override
    {
        if (!pointer)
            return;
        auto found = blocks.find(pointer);
        check(found != blocks.end(), "release owns actual block");
        if (found == blocks.end())
            std::abort();
        canaries();
        blocks.erase(found);
        ++releases;
        std::free(static_cast<unsigned char *>(pointer) - 16);
    }
    void *realloc(void *pointer, size_t size) override
    {
        if (!pointer)
            return malloc(size);
        auto found = blocks.find(pointer);
        if (found == blocks.end())
            std::abort();
        void *next = malloc(size);
        if (!next)
            return nullptr;
        std::memcpy(next, pointer, std::min(size, found->second));
        free(pointer);
        return next;
    }
};
// A user-supplied instance of the genuine four-slot host interface. Allocation
// reaches the canonical allocator installed by the real Foundation factory.
class FoundationHostAllocator : public SdkAllocator
{
  public:
    void *malloc(size_t size, NxMemoryType type) override
    {
        return nxFoundationSDKAllocator->malloc(size, type);
    }
    void *mallocDEBUG(size_t size, const char *file, int line, const char *name, NxMemoryType type) override
    {
        return nxFoundationSDKAllocator->mallocDEBUG(size, file, line, name, type);
    }
    void *realloc(void *pointer, size_t size) override
    {
        return nxFoundationSDKAllocator->realloc(pointer, size);
    }
    void free(void *pointer) override
    {
        nxFoundationSDKAllocator->free(pointer);
    }
};

#if NX_PHYSICS_USE_X87
#define SLOT0(mesh, slot, fn)                                                                                \
    reinterpret_cast<decltype(&fn)>(const_cast<void *>(mesh.mPolygonTable[slot]))(&mesh.mPolygonTable)
#define SLOT(mesh, slot, fn, ...)                                                                            \
    reinterpret_cast<decltype(&fn)>(const_cast<void *>(mesh.mPolygonTable[slot]))(&mesh.mPolygonTable, 0,    \
                                                                                  __VA_ARGS__)
#else
#define SLOT0(mesh, slot, fn)                                                                                \
    reinterpret_cast<decltype(&fn)>(const_cast<void *>(mesh.mPolygonTable[slot]))(&mesh)
#define SLOT(mesh, slot, fn, ...)                                                                            \
    reinterpret_cast<decltype(&fn)>(const_cast<void *>(mesh.mPolygonTable[slot]))(&mesh, __VA_ARGS__)
#endif
static const unsigned directionWords[][3] = {{0x3f800000, 0, 0},
                                             {0xbf800000, 0, 0},
                                             {0, 0x3f800000, 0},
                                             {0, 0, 0x3f800000},
                                             {0x3f800000, 0x3f800000, 0x3f800000},
                                             {0x3f800000, 0x3f800001, 0x3f800000},
                                             {0x3dcccccd, 0x3e4ccccd, 0x3e99999a},
                                             {0xbf800000, 0x40000000, 0xc0400000},
                                             {0, 0, 0},
                                             {0x80000000, 0, 0},
                                             {0x3f800000, 0x80000000, 0}};
static const float poses[][16] = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
                                  {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 2, -3, 1.25f, 1},
                                  {0.6f, 0.8f, 0, 0, -0.8f, 0.6f, 0, 0, 0, 0, 1, 0, -0.1f, 0.2f, -0.3f, 1}};
static void cleanupOrphans(TriangleMeshConvexData *hull)
{
    // Existing true owner Destroy omits these lazily generated allocations.
    // Observe ownership first; the fixture explicitly reclaims the orphans.
    if (hull->mEdgeAxes)
    {
        delete hull->mEdgeAxes;
        hull->mEdgeAxes = nullptr;
    }
    if (hull->mEdges)
    {
        nxIceFree(reinterpret_cast<NxU32 *>(hull->mEdges) - 1);
        hull->mEdges = nullptr;
    }
    if (hull->mEdgeNormals)
    {
        nxIceFree(hull->mEdgeNormals);
        hull->mEdgeNormals = nullptr;
    }
    if (hull->mEdgeToPolygons)
    {
        nxIceFree(hull->mEdgeToPolygons);
        hull->mEdgeToPolygons = nullptr;
    }
    if (hull->mEdgePolygons)
    {
        nxIceFree(hull->mEdgePolygons);
        hull->mEdgePolygons = nullptr;
    }
    if (hull->mPolygonERefs)
    {
        nxIceFree(hull->mPolygonERefs);
        hull->mPolygonERefs = nullptr;
    }
    hull->mNbEdges = 0;
}
static void clearPolygonCache(TriangleMeshConvexData *hull)
{
    nxIceFree(hull->mPolygonVRefs);
    nxIceFree(reinterpret_cast<NxU32 *>(hull->mPolygons) - 1);
    hull->mPolygonVRefs = nullptr;
    hull->mPolygons = nullptr;
    hull->mNbPolygons = 0;
}
struct OrphanArrays
{
    IceCore::Container *axes;
    HullEdge *edges;
    IceMaths::Point *normals;
    EdgeDesc *descriptors;
    NxU32 *polygons;
    NxU32 *refs;
};
static void destroyObservedOrphans(const OrphanArrays &arrays, GuardAllocator &allocator)
{
    check(allocator.blocks.count(reinterpret_cast<NxU32 *>(arrays.edges) - 1) == 1,
          "actual dtor leaves edge cookie orphan");
    check(allocator.blocks.count(arrays.normals) == 1 && allocator.blocks.count(arrays.descriptors) == 1 &&
              allocator.blocks.count(arrays.polygons) == 1 && allocator.blocks.count(arrays.refs) == 1,
          "actual dtor leaves lazy edge array orphans");
    delete arrays.axes;
    nxIceFree(reinterpret_cast<NxU32 *>(arrays.edges) - 1);
    nxIceFree(arrays.normals);
    nxIceFree(arrays.descriptors);
    nxIceFree(arrays.polygons);
    nxIceFree(arrays.refs);
}
static OrphanArrays polygonDomain(TriangleMesh &mesh)
{
    TriangleMeshConvexData *hull = static_cast<TriangleMeshConvexData *>(mesh.mConvexMesh);
    check(hull && hull->mVertexGraph && hull->mVertexGraph->mNbVerts == hull->mNbVerts, "genuine graph+64");
    clearPolygonCache(hull);
    for (unsigned repeat = 0; repeat < 2; ++repeat)
    {
        exact(SLOT0(mesh, 1, nxMeshHullVertexCount));
        check(SLOT0(mesh, 2, nxMeshHullVertices) == hull->mVerts, "slot2 borrowed owner vertices");
        const IceMaths::Point *center = SLOT0(mesh, 0, nxMeshHullCentre);
        check(center == &hull->mCentroid, "slot0 actual center");
        const unsigned count = SLOT0(mesh, 3, nxMeshHullPolygonCount);
        exact(count);
        for (unsigned i = 0; i < count; ++i)
        {
            const HullPolygon *poly = SLOT(mesh, 4, nxMeshHullPolygon, i);
            check(poly == hull->mPolygons + i, "slot4 polygon index");
            exact(poly->mNbVerts);
            for (unsigned j = 0; j < poly->mNbVerts; ++j)
                exact(poly->mVRefs[j]);
            numeric(poly->mPlane.n.x, 2, 2);
            numeric(poly->mPlane.n.y, 2, 2);
            numeric(poly->mPlane.n.z, 2, 2);
            numeric(poly->mPlane.d, 1, 3);
            numeric(poly->mMin, 1, 4);
            numeric(poly->mMax, 1, 4);
        }
        for (unsigned i = 0; i < 3; ++i)
            numeric((*center)[i]);
        check(SLOT0(mesh, 5, nxMeshHullEdgeAxes) == hull->mEdgeAxes, "slot5 lazy axes");
        check(SLOT0(mesh, 6, nxMeshHullEdges) == hull->mEdges, "slot6 lazy edges");
        check(SLOT0(mesh, 7, nxMeshHullEdgeToPolygons) == hull->mEdgeToPolygons, "slot7 lazy descriptors");
        check(SLOT0(mesh, 8, nxMeshHullEdgePolygons) == hull->mEdgePolygons, "slot8 lazy polygon refs");
        exact(hull->mNbEdges);
        for (unsigned i = 0; i < hull->mNbEdges; ++i)
        {
            exact(hull->mEdges[i].mRef0);
            exact(hull->mEdges[i].mRef1);
            exact(hull->mEdgeToPolygons[i].Count);
            exact(hull->mEdgeToPolygons[i].Offset);
            for (unsigned j = 0; j < hull->mEdgeToPolygons[i].Count; ++j)
                exact(hull->mEdgePolygons[hull->mEdgeToPolygons[i].Offset + j]);
        }
        exact(hull->mEdgeAxes->GetNbEntries());
        for (unsigned i = 0; i < hull->mEdgeAxes->GetNbEntries(); ++i)
            numeric(value(hull->mEdgeAxes->GetEntries()[i]), 2, 5);
    }
    // Each lazy table branch starts from the true owner's absent cache, then
    // reaches the same populated arrays. Inputs/domain remain unchanged.
    const unsigned polygonCount = hull->mNbPolygons, edgeCount = hull->mNbEdges;
    clearPolygonCache(hull);
    check(SLOT(mesh, 4, nxMeshHullPolygon, 0) == hull->mPolygons, "slot4 independently lazy");
    check(hull->mNbPolygons == polygonCount, "slot4 rebuilt count");
    for (unsigned slot = 5; slot <= 8; ++slot)
    {
        cleanupOrphans(hull);
        if (slot == 5)
            check(SLOT0(mesh, 5, nxMeshHullEdgeAxes) == hull->mEdgeAxes, "slot5 independently lazy");
        if (slot == 6)
            check(SLOT0(mesh, 6, nxMeshHullEdges) == hull->mEdges, "slot6 independently lazy");
        if (slot == 7)
            check(SLOT0(mesh, 7, nxMeshHullEdgeToPolygons) == hull->mEdgeToPolygons,
                  "slot7 independently lazy");
        if (slot == 8)
            check(SLOT0(mesh, 8, nxMeshHullEdgePolygons) == hull->mEdgePolygons, "slot8 independently lazy");
        if (slot != 5)
            check(hull->mNbEdges == edgeCount, "lazy edge cache count");
    }
    SLOT0(mesh, 5, nxMeshHullEdgeAxes);
    IceSupportMap map;
#if NX_PHYSICS_USE_X87
    nxSupportMapVertexConstruct(&map, 0, hull);
    check(nxSupportMapInit(&map, 0, 4), "actual kindC map producer");
#else
    nxSupportMapVertexConstruct(&map, hull);
    check(nxSupportMapInit(&map, 4), "actual kindC map producer");
#endif
    unsigned visited[258];
    std::fill(visited, visited + 258, 0x5a5a5a5a);
    NxPolygonScratch scratch = {0x13579bdf, hull->mNbVerts, visited + 1, 0x2468ace0, 0xa5a5a5a5, 0};
    for (unsigned pose = 0; pose < 3; ++pose)
        for (const auto &words : directionWords)
        {
            ++group;
            IceMaths::Point dir(value(words[0]), value(words[1]), value(words[2]));
            exact(SLOT(mesh, 9, nxMeshHullSupportPolygon, &dir, poses[pose]));
            exact(SLOT(mesh, 9, nxMeshHullSupportPolygon, &dir, nullptr));
            unsigned kind = 0xbad;
            exact(SLOT(mesh, 10, nxMeshHullSupportFace, &dir, poses[pose], &kind));
            exact(kind);
            exact(SLOT(mesh, 10, nxMeshHullSupportFace, &dir, nullptr, nullptr));
            for (unsigned path = 0; path < 2; ++path)
            {
                float bounds[4] = {value(0x6a123456), -99, 99, value(0x7b123456)};
                const unsigned oldStamp = scratch.mStamp;
                SLOT(mesh, 11, nxMeshHullProject, &scratch, bounds + 1, bounds + 2, &dir, poses[pose],
                     path ? &map : nullptr);
                numeric(bounds[1], 3, 6);
                numeric(bounds[2], 3, 6);
                exact(scratch.mStamp);
                check(bounds[1] <= bounds[2], "ordered projection");
                check(bounds[0] == value(0x6a123456) && bounds[3] == value(0x7b123456),
                      "projection output canaries");
                check(scratch.mStamp == oldStamp + (path ? 0 : 2), "graph stamps and map scratch reuse");
                check(scratch.mOpaque00 == 0x13579bdf && scratch.mOpaque0C == 0x2468ace0 &&
                          scratch.mOpaque10 == 0xa5a5a5a5,
                      "opaque scratch unchanged");
                check(visited[0] == 0x5a5a5a5a && visited[hull->mNbVerts + 1] == 0x5a5a5a5a,
                      "visited canaries");
            }
        }
#if NX_PHYSICS_USE_X87
    nxSupportMapVertexDelete(&map, 0, 0);
#else
    nxSupportMapVertexDelete(&map, 0);
#endif
    scratch.mStamp = 0xffffffff;
    exact(nxScratchStamp(&scratch));
    for (unsigned i = 0; i < hull->mNbVerts; ++i)
    {
        exact(visited[i + 1]);
        check(visited[i + 1] == 0, "stamp wrap clears");
    }
    scratch.mStamp = 0xffffffff;
    scratch.mVisited = nullptr;
    exact(nxScratchStamp(&scratch));
    check(scratch.mStamp == hull->mNbVerts, "stamp null visited wrap restart");
    const IceMaths::Point axis(1, 0, 0);
    float fallback[2] = {-99, 99};
    const unsigned stampBefore = scratch.mStamp;
    SLOT(mesh, 11, nxMeshHullProject, &scratch, fallback, fallback + 1, &axis, poses[0], nullptr);
    check(fallback[0] == hull->mVerts[0].x && fallback[1] == hull->mVerts[0].x,
          "failed null-visited climb preserves vertex0");
    check(scratch.mStamp == stampBefore + 2, "failed climbs still advance stamps");
    scratch.mVisited = visited + 1;
    scratch.mStamp = 0xffffffff;
    SLOT(mesh, 11, nxMeshHullProject, &scratch, fallback, fallback + 1, &axis, poses[0], nullptr);
    check(scratch.mStamp == hull->mNbVerts + 1, "projection consumes wrapped and next stamp");
    return {hull->mEdgeAxes,       hull->mEdges,        hull->mEdgeNormals,
            hull->mEdgeToPolygons, hull->mEdgePolygons, hull->mPolygonERefs};
}
static void exactBytes(const unsigned char *bytes, unsigned count)
{
    for (unsigned i = 0; i < count; i += 4)
    {
        unsigned packed = 0;
        for (unsigned j = 0; j < 4 && i + j < count; ++j)
            packed |= unsigned(bytes[i + j]) << (j * 8);
        exact(packed);
    }
}
static void meshDomain(GuardAllocator &allocator)
{
    check(PhysicsSDK::instance == nullptr, "actual singleton null default branch");
    const NxU32 cube[][3] = {{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7}, {0, 1, 5}, {0, 5, 4},
                             {1, 2, 6}, {1, 6, 5}, {2, 3, 7}, {2, 7, 6}, {3, 0, 4}, {3, 4, 7}};
    const NxU32 tetra[][3] = {{0, 2, 1}, {0, 1, 3}, {0, 3, 2}, {1, 2, 3}};
    for (unsigned shape = 0; shape < 8; ++shape)
        for (unsigned winding = 0; winding < 2; ++winding)
        {
            ++group;
            const unsigned vertexCount = (shape & 1) ? 4 : 8, faceCount = (shape & 1) ? 4 : 12;
            NxVec3 vertices[8];
            std::memcpy(vertices, meshWords[shape], sizeof(vertices));
            NxU32 triangles[12][3];
            std::memcpy(triangles, (shape & 1) ? tetra : cube, faceCount * 12);
            if (winding)
                for (unsigned i = 0; i < faceCount; ++i)
                    std::swap(triangles[i][1], triangles[i][2]);
            OrphanArrays orphans;
            {
                TriangleMesh mesh;
                check(mesh.publicHandle() != nullptr, "genuine wrapper constructed");
                exact(mesh.mPolygonTable == gTriangleMeshPolygonTable);
                NxTriangleMeshDesc desc;
                desc.setToDefault();
                desc.numVertices = vertexCount;
                desc.numTriangles = faceCount;
                desc.pointStrideBytes = 12;
                desc.triangleStrideBytes = 12;
                desc.points = vertices;
                desc.triangles = triangles;
                desc.flags = NX_MF_CONVEX;
                check(mesh.publicHandle()->loadFromDesc(desc), "true owner model graph build");
                exact(mesh.mInternal.mVertexCount);
                exact(mesh.mInternal.mTriangleCount);
                for (unsigned i = 0; i < 6; ++i)
                    numeric(mesh.mBounds44[i]);
                exact(mesh.publicHandle()->getSubmeshCount());
                NxTriangleMeshDesc saved;
                check(mesh.publicHandle()->saveToDesc(saved), "wrapper descriptor borrow");
                exact(saved.numVertices);
                exact(saved.numTriangles);
                exact(saved.flags);
                for (unsigned i = 0; i < 7; ++i)
                {
                    auto array = static_cast<NxInternalArray>(i);
                    exact(mesh.publicHandle()->getCount(0, array));
                    exact(mesh.publicHandle()->getFormat(0, array));
                    exact(mesh.publicHandle()->getStride(0, array));
                    exact(mesh.publicHandle()->getBase(0, array) != nullptr);
                    exact(mesh.publicHandle()->getCount(1, array));
                    exact(mesh.publicHandle()->getBase(1, array) == nullptr);
                }
                orphans = polygonDomain(mesh);
                check(nxInternalMeshBuildModel(&mesh.mInternal, 0xff, 0.125f, nullptr),
                      "actual null SDK model default producer");
                exact(mesh.mInternal.mModel->IsQuantized());
                const unsigned densities[] = {32, 64, 80};
                for (unsigned density : densities)
                {
                    if (density != 32 && (shape != 0 || winding != 0))
                        continue;
                    ++group;
                    NxPMap pmap;
                    std::srand(123 + winding);
                    check(NxCreatePMap(pmap, *mesh.publicHandle(), density, nullptr),
                          "genuine PMap real model seeded rays");
                    exact(pmap.dataSize);
                    const unsigned char *bytes = static_cast<const unsigned char *>(pmap.data);
                    exactBytes(bytes, pmap.dataSize);
                    check(mesh.publicHandle()->loadPMap(pmap), "true wrapper PMap load");
                    exact(mesh.mPMap->getResolution());
                    exact(mesh.mPMap->getCellCount());
                    for (unsigned i = 0; i < mesh.mPMap->getCellCount(); ++i)
                        exact(mesh.mPMap->getGrid()[i]);
                    exact(mesh.publicHandle()->hasPMap());
                    exact(mesh.publicHandle()->getPMapDensity());
                    NxPMap output;
                    output.dataSize = mesh.publicHandle()->getPMapSize();
                    std::vector<unsigned char> data(output.dataSize + 2, 0x6b);
                    output.data = data.data() + 1;
                    check(mesh.publicHandle()->getPMapData(output), "real wrapper serialization");
                    check(data.front() == 0x6b && data.back() == 0x6b, "serialized output canaries");
                    exact(output.dataSize);
                    exactBytes(data.data() + 1, output.dataSize);
                    --output.dataSize;
                    check(!mesh.publicHandle()->getPMapData(output), "exact size required");
                    const unsigned retainedSize = pmap.dataSize;
                    check(NxReleasePMap(pmap) && !pmap.data && pmap.dataSize == retainedSize,
                          "CRT release leaves size");
                    check(NxReleasePMap(pmap), "repeated release supported");
                }
                allocator.canaries();
            }
            destroyObservedOrphans(orphans, allocator);
            check(allocator.blocks.size() == 1,
                  "true destructor releases model graph wrapper PMap and arrays");
        }
    ++group;
    {
        TriangleMesh empty;
        NxTriangleMeshDesc invalid;
        invalid.setToDefault();
        exact(empty.publicHandle()->loadFromDesc(invalid));
        exact(empty.publicHandle()->hasPMap());
        exact(empty.publicHandle()->getPMapSize());
        NxPMap noMap;
        exact(empty.publicHandle()->getPMapData(noMap));
        PenetrationMap map;
        MemoryStream stream(64, nullptr);
        exact(map.create(nullptr, 32, "unused", &stream, false, nullptr));
        exact(map.create(&empty, 32, nullptr, &stream, false, nullptr));
    }
    allocator.failAt = allocator.allocations + 1;
    {
        TriangleMesh failed;
        check(failed.publicHandle() == nullptr, "checked wrapper allocation failure");
        exact(!failed.publicHandle());
    }
    allocator.failAt = 0;
}

static void put(FILE *file, unsigned word)
{
    std::fwrite(&word, 4, 1, file);
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
#if NX_PHYSICS_USE_X87
    check((_controlfp(0, 0) & (_MCW_PC | _MCW_RC)) == (_PC_53 | _RC_NEAR), "original capture nearest53");
#endif
    GuardAllocator allocator;
    FoundationHostAllocator host;
    NxFoundationSDK *foundation = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, nullptr, &allocator);
    check(foundation && nxFoundationSDKAllocator == &allocator,
          "genuine Foundation factory installs allocator");
    nxSetSdkAllocatorBridge(&host);
    meshDomain(allocator);
    allocator.canaries();
    check(allocator.blocks.size() == 1, "only genuine Foundation remains after vendor owners die");
    foundation->release();
    nxSetSdkAllocatorBridge(nullptr);
    check(allocator.blocks.empty(), "genuine Foundation releases final allocation");
#if NX_PHYSICS_USE_X87
    FILE *file = std::fopen(argv[1], "wb");
    if (!file)
        return 3;
    std::fwrite("NXPF", 1, 4, file);
    put(file, 1);
    put(file, unsigned(observations.size()) * 20);
    put(file, 20);
    for (const auto &row : observations)
    {
        put(file, row.kind);
        put(file, row.group);
        put(file, row.index);
        put(file, row.reserved);
        put(file, row.word);
    }
    std::fclose(file);
#else
    std::vector<unsigned char> bytes;
    std::string error;
    check(nxReadFixture(argv[1], bytes, error), error.c_str());
    check(bytes.size() == observations.size() * 20, "actual mesh fixture observation count");
    unsigned discreteDifferences = 0, numericDifferences = 0, numericViolations = 0;
    const double absoluteBudget[7] = {0, 0, 1.0 / (1u << 29), 1.0 / (1u << 22), 1.0 / (1u << 25), 0, 0};
    const double relativeBudget[7] = {0, 0, 1e-7, 1.2e-7, 1e-7, 0, 0};
    double maxAbsolute[7] = {}, maxRelative[7] = {};
    double maxNormalAngle = 0, maxNormalVectorError = 0, maxNormalUnitError = 0;
    double minimum[7] = {}, maximum[7] = {};
    unsigned differing[7] = {}, totals[7] = {};
    std::map<unsigned, std::vector<Observation>> referenceGroups, actualGroups;
    for (unsigned i = 0; i < bytes.size() / 20; ++i)
    {
        Observation row;
        std::memcpy(&row, &bytes[i * 20], 20);
        check(row.kind < 4 && row.index == i && !row.reserved, "reference fixture identity");
        referenceGroups[row.group].push_back(row);
    }
    for (const auto &row : observations)
    {
        actualGroups[row.group].push_back(row);
        const unsigned quantity = quantities[row.index];
        if (quantity)
        {
            const double number = value(row.word);
            if (!totals[quantity])
                minimum[quantity] = maximum[quantity] = number;
            minimum[quantity] = std::min(minimum[quantity], number);
            maximum[quantity] = std::max(maximum[quantity], number);
            ++totals[quantity];
        }
    }
    check(referenceGroups.size() == actualGroups.size(), "fixture group count");
    for (const auto &entry : actualGroups)
    {
        const auto &actualRows = entry.second;
        const auto &referenceRows = referenceGroups[entry.first];
        if (actualRows.size() != referenceRows.size())
        {
            ++discreteDifferences;
            if (discreteDifferences < 20)
                std::fprintf(stderr, "group count group=%u actual=%zu reference=%zu\n", entry.first,
                             actualRows.size(), referenceRows.size());
        }
        for (unsigned i = 0; i < std::min(actualRows.size(), referenceRows.size()); ++i)
        {
            const auto &actual = actualRows[i];
            const auto &reference = referenceRows[i];
            if (actual.kind != reference.kind)
            {
                ++discreteDifferences;
                continue;
            }
            const unsigned quantity = quantities[actual.index];
            if (quantity == 2 && i + 2 < actualRows.size() &&
                (i == 0 || quantities[actualRows[i - 1].index] != 2))
            {
                double a[3], b[3], deltaSquare = 0, aSquare = 0, bSquare = 0, dot = 0;
                for (unsigned component = 0; component < 3; ++component)
                {
                    a[component] = value(actualRows[i + component].word);
                    b[component] = value(referenceRows[i + component].word);
                    deltaSquare += (a[component] - b[component]) * (a[component] - b[component]);
                    aSquare += a[component] * a[component];
                    bSquare += b[component] * b[component];
                }
                check(aSquare > 0 && bSquare > 0, "closed-domain planes have nonzero normals");
                for (unsigned component = 0; component < 3; ++component)
                {
                    a[component] /= std::sqrt(aSquare);
                    b[component] /= std::sqrt(bSquare);
                    dot += a[component] * b[component];
                }
                const double cross[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
                                         a[0] * b[1] - a[1] * b[0]};
                const double angle = std::atan2(
                    std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]), dot);
                maxNormalAngle = std::max(maxNormalAngle, angle);
                maxNormalVectorError = std::max(maxNormalVectorError, std::sqrt(deltaSquare));
                maxNormalUnitError = std::max(maxNormalUnitError, std::fabs(std::sqrt(aSquare) - 1));
            }
            if (actual.word == reference.word)
                continue;
            if (!actual.kind)
            {
                ++discreteDifferences;
                if (discreteDifferences < 20)
                    std::fprintf(stderr, "discrete row=%u local=%u group=%u actual=%x reference=%x\n",
                                 actual.index, i, actual.group, actual.word, reference.word);
            }
            else
            {
                ++numericDifferences;
                double a = value(actual.word), b = value(reference.word), difference = std::fabs(a - b);
                const unsigned quantity = quantities[actual.index];
                ++differing[quantity];
                maxAbsolute[quantity] = std::max(maxAbsolute[quantity], difference);
                if (b)
                    maxRelative[quantity] = std::max(maxRelative[quantity], difference / std::fabs(b));
                const bool accepted = a != 0 && b != 0 && std::signbit(a) == std::signbit(b) &&
                                      difference <= absoluteBudget[quantity] &&
                                      difference / std::fabs(b) <= relativeBudget[quantity];
                if (!accepted)
                {
                    ++numericViolations;
                    if (numericViolations < 20)
                        std::fprintf(
                            stderr,
                            "numeric budget violation row=%u group=%u quantity=%u actual=%x reference=%x\n",
                            actual.index, actual.group, quantity, actual.word, reference.word);
                }
            }
        }
    }
    std::printf("discrete_differences=%u numeric_differences=%u numeric_violations=%u\n", discreteDifferences,
                numericDifferences, numericViolations);
    for (unsigned quantity = 1; quantity < 7; ++quantity)
        std::printf("quantity=%u count=%u differences=%u min=%.17g max=%.17g max_abs=%.17g max_rel=%.17g\n",
                    quantity, totals[quantity], differing[quantity], minimum[quantity], maximum[quantity],
                    maxAbsolute[quantity], maxRelative[quantity]);
    std::printf("normal max_angle_radians=%.17g max_vector_error=%.17g max_unit_error=%.17g\n",
                maxNormalAngle, maxNormalVectorError, maxNormalUnitError);
    check(maxNormalVectorError <= 1.0 / (1u << 29), "approved plane normal vector delta");
    check(maxNormalAngle <= 2e-9, "approved normalized plane normal angle radians");
    check(maxNormalUnitError <= 1e-7, "approved actual plane normal unit error");
    check(!discreteDifferences && !numericViolations, "approved per-quantity mesh observations");
#endif
    std::printf("triangle_mesh groups=%u observations=%zu failures=%u\n", group, observations.size(),
                failures);
    return failures ? 1 : 0;
}
