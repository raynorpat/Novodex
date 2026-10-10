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
#include "Opcode.h"
#if NX_PHYSICS_USE_X87
#include <float.h>
#endif

using namespace Opcode;
// Actual inline producer, including its existing typed eight-value contract.
#include "OPC_RayAABBOverlap.h"
#include "OPC_RayTriOverlap.h"
#include "SupportMapDomainInputs.h"

static unsigned failures, group;
struct Observation
{
    unsigned kind, group, index, reserved, word;
};
static std::vector<Observation> observations;
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
}
static void numeric(float value, unsigned kind = 1)
{
    unsigned word;
    std::memcpy(&word, &value, 4);
    observations.push_back({kind, group, unsigned(observations.size()), 0, word});
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
                check(bytes[i] == 0x6a && bytes[16 + block.second + i] == 0x7b, "owned allocator canaries");
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

static void crossDomain()
{
    // Equality and adjacent binary32 radius; all eight arguments independently
    // alter one comparison, then cancellation, negative sign, nonfinite values.
    const float equality[8] = {3, 2, 1, 1, 2, 1, 3, 1};
    for (unsigned argument = 0; argument < 8; ++argument)
        for (unsigned side = 0; side < 3; ++side)
        {
            ++group;
            float inputs[8];
            std::memcpy(inputs, equality, sizeof(inputs));
            unsigned word;
            std::memcpy(&word, &inputs[argument], 4);
            inputs[argument] = value(word + side - 1);
            const bool separated = nxRayAABBCrossSeparated(inputs[0], inputs[1], inputs[2], inputs[3],
                                                           inputs[4], inputs[5], inputs[6], inputs[7]) != 0;
            exact(separated);
            const double left = std::fabs(double(inputs[0]) * inputs[1] - double(inputs[2]) * inputs[3]);
            const double right = double(inputs[4]) * inputs[5] + double(inputs[6]) * inputs[7];
            check(separated == (left > right), "eight-value cross-axis typed contract and equality");
        }
    const unsigned probes[][8] = {
        {0x4b800001, 0x3f800001, 0x4b800000, 0x3f800000, 0x40000000, 0x3f800000, 0, 0},
        {0xcb800001, 0x3f800001, 0xcb800000, 0x3f800000, 0x40000000, 0x3f800000, 0, 0},
        {0x80000000, 0x3f800000, 0, 0x3f800000, 0, 0, 0, 0},
        {0x7f800000, 0x3f800000, 0, 0, 0x7f800000, 0x3f800000, 0, 0},
        {0x7fc00001, 0x3f800000, 0, 0, 0, 0, 0, 0},
    };
    for (const auto &probe : probes)
    {
        ++group;
        float p[8];
        for (unsigned i = 0; i < 8; ++i)
            p[i] = value(probe[i]);
        exact(nxRayAABBCrossSeparated(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
    }
}
class RayProbe : public RayCollider
{
  public:
    bool triangle(const Ray &ray, const Point *vertices, bool culling, float tolerance)
    {
        mOrigin = ray.mOrig;
        mDir = ray.mDir;
        mCulling = culling;
        mNovodeXSetting88 = tolerance;
        std::memset(&mStabbedFace, 0xcd, sizeof(mStabbedFace));
        return RayTriOverlap(vertices[0], vertices[1], vertices[2]) != 0;
    }
    const CollisionFace &face() const
    {
        return mStabbedFace;
    }
    bool box(const Ray &ray, const Point &center, const Point &extents)
    {
        mOrigin = ray.mOrig;
        mDir = ray.mDir;
        mFDir = Point(std::fabs(mDir.x), std::fabs(mDir.y), std::fabs(mDir.z));
        return RayAABBOverlap(center, extents) != 0;
    }
};
static void triangleDomain()
{
    Point vertices[] = {Point(0, 0, 0), Point(1, 0, 0), Point(0, 1, 0)};
    const unsigned coordinateWords[] = {0,          0x80000000, 0x3e800000, 0x3f000000,
                                        0x3f7fffff, 0x3f800000, 0x3f800001, 0xb3d6bf95};
    const unsigned directionWords[][3] = {
        {0, 0, 0xbf800000}, {0, 0, 0x3f800000}, {0x3f13cd3a, 0x3f13cd3a, 0xbf13cd3a},
        {0, 0, 0},          {0x3f800000, 0, 0}, {0, 0, 0xb58637bd},
        {0, 0, 0xb58637be},
    };
    RayProbe probe;
    for (unsigned winding = 0; winding < 2; ++winding)
    {
        if (winding)
            std::swap(vertices[1], vertices[2]);
        for (unsigned cull = 0; cull < 2; ++cull)
            for (unsigned tolerance = 0; tolerance < 2; ++tolerance)
                for (unsigned x : coordinateWords)
                    for (unsigned y : coordinateWords)
                        for (const auto &words : directionWords)
                        {
                            ++group;
                            Point direction;
                            std::memcpy(&direction, words, sizeof(direction));
                            Ray ray(Point(value(x), value(y), 1), direction);
                            const bool hit =
                                probe.triangle(ray, vertices, cull != 0, tolerance ? 1e-7f : 0.0f);
                            exact(hit);
                            if (hit)
                            {
                                numeric(probe.face().mDistance, 2);
                                numeric(probe.face().mU, 3);
                                numeric(probe.face().mV, 3);
                            }
                            exact(probe.box(ray, Point(.5f, .5f, 0), Point(.5f, .5f, 0)));
                        }
    }
}
static void volume(const CollisionAABB &box)
{
    for (unsigned axis = 0; axis < 3; ++axis)
    {
        numeric(box.mCenter[axis]);
        numeric(box.mExtents[axis]);
        check(box.mExtents[axis] >= 0, "actual node has nonnegative extent");
    }
}
static void volume(const QuantizedAABB &box)
{
    for (unsigned axis = 0; axis < 3; ++axis)
    {
        exact(unsigned(short(box.mCenter[axis])));
        exact(box.mExtents[axis]);
    }
}
template <class Node> static void implicitNodes(const Node *nodes, unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
    {
        volume(nodes[i].mAABB);
        exact(nodes[i].IsLeaf());
        if (nodes[i].IsLeaf())
            exact(nodes[i].GetPrimitive());
        else
            exact(unsigned(nodes[i].GetPos() - nodes));
    }
}
template <class Node> static void noLeafNodes(const Node *nodes, unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
    {
        volume(nodes[i].mAABB);
        exact(nodes[i].HasPosLeaf() != 0);
        exact(nodes[i].HasPosLeaf() ? nodes[i].GetPosPrimitive() : unsigned(nodes[i].GetPos() - nodes));
        exact(nodes[i].HasNegLeaf() != 0);
        exact(nodes[i].HasNegLeaf() ? nodes[i].GetNegPrimitive() : unsigned(nodes[i].GetNeg() - nodes));
    }
}
struct WalkState
{
    unsigned token = 0x12345678, count = 0;
    bool recurse = true;
};
static bool optimizedWalk(const void *current, void *user)
{
    WalkState &state = *static_cast<WalkState *>(user);
    check(current && state.token == 0x12345678, "actual optimized walk callback arguments");
    ++state.count;
    return state.recurse;
}
static bool sourceWalk(const AABBTreeNode *current, udword depth, void *user)
{
    WalkState &state = *static_cast<WalkState *>(user);
    check(current && depth < 32 && state.token == 0x12345678, "actual source walk current/depth/context");
    ++state.count;
    exact(current->GetNbPrimitives());
    for (unsigned i = 0; i < current->GetNbPrimitives(); ++i)
        exact(current->GetPrimitives()[i]);
    for (unsigned axis = 0; axis < 3; ++axis)
    {
        numeric(current->GetAABB()->GetMin(axis));
        numeric(current->GetAABB()->GetMax(axis));
    }
    return state.recurse;
}
static void observeModel(const Model &model, unsigned triangles)
{
    exact(model.HasLeafNodes());
    exact(model.IsQuantized());
    exact(model.HasSingleNode());
    if (model.HasSingleNode())
        return;
    const AABBOptimizedTree *tree = model.GetTree();
    exact(tree->GetNbNodes());
    exact(tree->GetUsedBytes());
    exact(model.GetUsedBytes());
    check(tree->GetNbNodes() == (model.HasLeafNodes() ? triangles * 2 - 1 : triangles - 1),
          "real optimized node count");
    WalkState state;
    exact(tree->Walk(optimizedWalk, &state));
    exact(state.count);
    check(state.count == tree->GetNbNodes(), "optimized walk visits real topology");
    state.count = 0;
    state.recurse = false;
    exact(tree->Walk(optimizedWalk, &state));
    exact(state.count);
    check(state.count == 1, "optimized callback false stops descendants");
    if (model.IsQuantized())
    {
        const Point *center;
        const Point *extents;
        if (model.HasLeafNodes())
        {
            const auto *typed = static_cast<const AABBQuantizedTree *>(tree);
            implicitNodes(typed->GetNodes(), tree->GetNbNodes());
            center = &typed->mCenterCoeff;
            extents = &typed->mExtentsCoeff;
        }
        else
        {
            const auto *typed = static_cast<const AABBQuantizedNoLeafTree *>(tree);
            noLeafNodes(typed->GetNodes(), tree->GetNbNodes());
            center = &typed->mCenterCoeff;
            extents = &typed->mExtentsCoeff;
        }
        for (unsigned axis = 0; axis < 3; ++axis)
        {
            numeric((*center)[axis]);
            numeric((*extents)[axis]);
        }
    }
    else if (model.HasLeafNodes())
        implicitNodes(static_cast<const AABBCollisionTree *>(tree)->GetNodes(), tree->GetNbNodes());
    else
        noLeafNodes(static_cast<const AABBNoLeafTree *>(tree)->GetNodes(), tree->GetNbNodes());
    if (model.GetSourceTree())
    {
        WalkState source;
        const AABBTree *original = model.GetSourceTree();
        exact(original->Walk(sourceWalk, &source));
        exact(source.count);
        exact(original->ComputeDepth());
        exact(original->IsComplete());
        for (unsigned i = 0; i < triangles; ++i)
            exact(original->GetIndices()[i]);
    }
}
static void rays(Model &model)
{
    // World units for origins/distances; directions are literal dimensionless
    // unit axes. Adjacent face/edge/corner rays, signed zero, miss and interior.
    const unsigned originWords[][3] = {
        {0xbf800000, 0x3f000000, 0x3f000000}, {0xbf800000, 0, 0},
        {0xbf800000, 0x3f800000, 0x3f800000}, {0xbf800000, 0x3f800001, 0x3f000000},
        {0xbf800000, 0x3f7fffff, 0x3f000000}, {0x40000000, 0x3f000000, 0x3f000000},
        {0x3f000000, 0x3f000000, 0x3f000000}, {0xbf800000, 0x80000000, 0x3f000000},
        {0x3e800000, 0xbf800000, 0x3e800000}, {0x3e800000, 0x3e800000, 0x40000000},
        {0xc0400000, 0x40000000, 0xc0200000}, {0xbf800000, 0x40100000, 0xc0800000}};
    RayCollider collider;
    CollisionFaces destination;
    collider.SetDestination(&destination);
    for (unsigned mode = 0; mode < 6; ++mode)
    {
        collider.SetCulling((mode & 1) != 0);
        collider.SetFirstContact(mode == 4);
        collider.SetClosestHit(mode == 5);
        collider.SetTemporalCoherence(false);
        collider.SetMaxDist(mode == 2 ? 1.0f : mode == 3 ? value(0x3f800001) : MAX_FLOAT);
        check(!collider.ValidateSettings(), "supported actual ray settings");
        for (unsigned r = 0; r < sizeof(originWords) / sizeof(originWords[0]); ++r)
            for (unsigned axis = 0; axis < 3; ++axis)
                for (unsigned sign = 0; sign < 2; ++sign)
                {
                    ++group;
                    Point origin;
                    std::memcpy(&origin, originWords[r], sizeof(origin));
                    Point direction(0, 0, 0);
                    direction[axis] = sign ? -1.0f : 1.0f;
                    const Ray ray(origin, direction);
                    struct Cache
                    {
                        unsigned before = 0x24681357;
                        udword face = INVALID_ID;
                        unsigned after = 0x76543210;
                    } cache;
                    exact(collider.Collide(ray, model, nullptr, &cache.face));
                    exact(collider.GetContactStatus());
                    exact(collider.GetNbIntersections());
                    exact(collider.GetNbRayBVTests());
                    exact(collider.GetNbRayPrimTests());
                    exact(cache.face);
                    exact(destination.GetNbFaces());
                    check(cache.before == 0x24681357 && cache.after == 0x76543210,
                          "borrowed face-cache canaries");
                    std::vector<CollisionFace> hits;
                    if (destination.GetNbFaces())
                        hits.assign(destination.GetFaces(),
                                    destination.GetFaces() + destination.GetNbFaces());
                    std::sort(hits.begin(), hits.end(), [](const CollisionFace &a, const CollisionFace &b) {
                        return a.mFaceID < b.mFaceID;
                    });
                    for (const auto &hit : hits)
                    {
                        exact(hit.mFaceID);
                        numeric(hit.mDistance, 2);
                        numeric(hit.mU, 3);
                        numeric(hit.mV, 3);
                        check(hit.mFaceID < model.GetMeshInterface()->GetNbTriangles(),
                              "real hit index bound");
                        check(hit.mDistance >= 0, "real nonnegative ray distance");
                    }
                    if (mode == 4)
                    {
                        collider.SetTemporalCoherence(true);
                        exact(collider.Collide(ray, model, nullptr, &cache.face));
                        exact(cache.face);
                        exact(collider.GetNbIntersections());
                        exact(collider.GetNbRayBVTests());
                        collider.SetTemporalCoherence(false);
                    }
                }
    }
    // Exercise all world-matrix arguments using exact rigid translation.
    Matrix4x4 world;
    world.Identity();
    world.SetTrans(1, -2, 3);
    collider.SetCulling(false);
    collider.SetClosestHit(false);
    collider.SetFirstContact(false);
    collider.SetMaxDist();
    const Ray local(Point(-1, .25f, .25f), Point(1, 0, 0));
    collider.Collide(local, model);
    const unsigned localHits = collider.GetNbIntersections();
    const Ray translated(Point(0, -1.75f, 3.25f), Point(1, 0, 0));
    ++group;
    exact(collider.Collide(translated, model, &world, nullptr));
    exact(collider.GetNbIntersections());
    check(collider.GetNbIntersections() == localHits, "rigid translation ray metamorphic parity");
}
static void modelDomain(GuardAllocator &allocator)
{
    const unsigned cubeRefs[] = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
                                 1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7};
    const unsigned tetraRefs[] = {0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3};
    for (unsigned shape = 0; shape < 8; ++shape)
        for (unsigned winding = 0; winding < 2; ++winding)
        {
            const unsigned triangleCount = (shape & 1) ? 4 : 12;
            const unsigned vertexCount = (shape & 1) ? 4 : 8;
            Point vertices[8];
            std::memcpy(vertices, meshWords[shape], sizeof(vertices));
            IndexedTriangle triangles[12];
            std::memcpy(triangles, (shape & 1) ? tetraRefs : cubeRefs,
                        triangleCount * sizeof(IndexedTriangle));
            if (winding)
                for (unsigned i = 0; i < triangleCount; ++i)
                    std::swap(triangles[i].mVRef[1], triangles[i].mVRef[2]);
            MeshInterface mesh;
            mesh.SetNbVertices(vertexCount);
            mesh.SetNbTriangles(triangleCount);
            check(mesh.SetPointers(triangles, vertices), "real mesh pointers accepted");
            for (unsigned variant = 0; variant < 4; ++variant)
            {
                ++group;
                const size_t before = allocator.blocks.size();
                {
                    OPCODECREATE create;
                    create.mIMesh = &mesh;
                    create.mNoLeaf = (variant & 1) != 0;
                    create.mQuantized = (variant & 2) != 0;
                    create.mKeepOriginal = true;
                    Model model;
                    exact(model.Build(create));
                    check(model.GetTree() && model.GetSourceTree(), "genuine model owns both trees");
                    observeModel(model, triangleCount);
                    rays(model);
                    // Real reuse releases previous source/tree and builds another
                    // variant through the same live Model and borrowed mesh.
                    create.mNoLeaf = !create.mNoLeaf;
                    create.mKeepOriginal = false;
                    ++group;
                    exact(model.Build(create));
                    observeModel(model, triangleCount);
                    if (!create.mQuantized)
                    {
                        vertices[0].x += .125f;
                        exact(model.Refit());
                        observeModel(model, triangleCount);
                        vertices[0].x -= .125f;
                    }
                    allocator.canaries();
                }
                check(allocator.blocks.size() == before, "real Model destructor releases owned trees");
                exact(unsigned(allocator.blocks.size() - before));
            }
        }
    ++group;
    MeshInterface empty;
    OPCODECREATE create;
    Model model;
    exact(model.Build(create));
    create.mIMesh = &empty;
    exact(model.Build(create));
    check(!model.GetTree(), "invalid empty mesh does not allocate tree");
    Point vertices[] = {Point(0, 0, 0), Point(1, 0, 0), Point(0, 1, 0)};
    IndexedTriangle face;
    face.mVRef[0] = 0;
    face.mVRef[1] = 1;
    face.mVRef[2] = 2;
    empty.SetNbVertices(3);
    empty.SetNbTriangles(1);
    empty.SetPointers(&face, vertices);
    exact(model.Build(create));
    exact(model.HasSingleNode());
    rays(model);
    // Supported first generic-tree allocation failure. Subsequent checked and
    // unchecked allocation sites need source-specific ownership classification.
    IndexedTriangle twins[2] = {face, face};
    empty.SetNbTriangles(2);
    empty.SetPointers(twins, vertices);
    ++group;
    allocator.failAt = allocator.allocations + 1;
    exact(model.Build(create));
    allocator.failAt = 0;
    check(!model.GetTree() && !model.GetSourceTree(), "first generic allocation failure leaves no trees");
    exact(model.Build(create));
    observeModel(model, 2);
    // Geometric degeneracy is warned about but explicitly accepted by Model.
    twins[1].mVRef[2] = 1;
    ++group;
    exact(model.Build(create));
    observeModel(model, 2);
    create.mSettings.mLimit = 2;
    exact(model.Build(create));
}
static void builderDomain(GuardAllocator &allocator)
{
    const unsigned rules[] = {SPLIT_LARGEST_AXIS, SPLIT_SPLATTER_POINTS | SPLIT_GEOM_CENTER, SPLIT_BEST_AXIS,
                              SPLIT_BALANCED, SPLIT_FIFTY};
    Point vertices[] = {Point(-2, 1, 0), Point(-1, 2, .25f), Point(1, -1, 2), Point(3, 4, -2),
                        Point(3, 4, -2)};
    AABB boxes[5];
    for (unsigned i = 0; i < 5; ++i)
        boxes[i].SetCenterExtents(vertices[i], Point(.125f, .25f, .5f));
    for (unsigned kind = 0; kind < 2; ++kind)
        for (unsigned rule : rules)
        {
            ++group;
            const size_t before = allocator.blocks.size();
            {
                AABBTreeOfVerticesBuilder points;
                AABBTreeOfAABBsBuilder bounds;
                points.mVertexArray = vertices;
                bounds.mAABBArray = boxes;
                AABBTreeBuilder *builder = kind ? static_cast<AABBTreeBuilder *>(&bounds) : &points;
                builder->mNbPrimitives = 5;
                builder->mSettings.mRules = rule;
                builder->mSettings.mNovodeXExtendAxis = 2;
                builder->mSettings.mNovodeXExtendValue = 3;
                builder->mSettings.mNovodeXInflate = .125f;
                AABBTree tree;
                exact(tree.Build(builder));
                exact(tree.GetNbNodes());
                exact(tree.IsComplete());
                WalkState state;
                exact(tree.Walk(sourceWalk, &state));
                exact(state.count);
                check(state.count == 9, "all builder rules construct actual complete topology");
                RayCollider collider;
                Container indices;
                exact(collider.Collide(Ray(Point(-4, 1, 0), Point(1, 0, 0)), &tree, indices));
                exact(indices.GetNbEntries());
                for (unsigned i = 0; i < indices.GetNbEntries(); ++i)
                    exact(indices.GetEntries()[i]);
                vertices[0].x = -1.5f;
                exact(tree.Refit(builder));
                exact(tree.Refit2(builder));
                vertices[0].x = -2;
                allocator.canaries();
            }
            check(allocator.blocks.size() == before, "actual direct tree releases its pool/indices");
        }
    ++group;
    AABBTree empty;
    AABBTreeOfVerticesBuilder builder;
    exact(empty.Build(nullptr));
    exact(empty.Build(&builder));
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
    crossDomain();
    triangleDomain();
    modelDomain(allocator);
    builderDomain(allocator);
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
    check(bytes.size() == observations.size() * 20, "actual model fixture observation count");
    unsigned discreteDifferences = 0, numericDifferences = 0;
    double maxAbsolute[4] = {}, maxRelative[4] = {};
    std::map<unsigned, std::vector<Observation>> referenceGroups, actualGroups;
    for (unsigned i = 0; i < bytes.size() / 20; ++i)
    {
        Observation row;
        std::memcpy(&row, &bytes[i * 20], 20);
        check(row.kind < 4 && row.index == i && !row.reserved, "reference fixture identity");
        referenceGroups[row.group].push_back(row);
    }
    for (const auto &row : observations)
        actualGroups[row.group].push_back(row);
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
                maxAbsolute[actual.kind] = std::max(maxAbsolute[actual.kind], difference);
                if (b)
                    maxRelative[actual.kind] = std::max(maxRelative[actual.kind], difference / std::fabs(b));
                if (numericDifferences < 20)
                    std::fprintf(stderr, "numeric row=%u local=%u group=%u kind=%u actual=%x reference=%x\n",
                                 actual.index, i, actual.group, actual.kind, actual.word, reference.word);
            }
        }
    }
    std::printf("discrete_differences=%u numeric_differences=%u\n", discreteDifferences, numericDifferences);
    for (unsigned kind = 1; kind < 4; ++kind)
        std::printf("kind=%u max_abs=%.17g max_rel=%.17g\n", kind, maxAbsolute[kind], maxRelative[kind]);
    check(!discreteDifferences && !numericDifferences, "exact model/ray observations");
#endif
    std::printf("opcode_model groups=%u observations=%zu failures=%u\n", group, observations.size(),
                failures);
    return failures ? 1 : 0;
}
