#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <cstddef>
#include "Nxp.h"
#include "NxUserAllocator.h"
#include "ConvexHull.h"
#include "IceMeshBuilder2.h"
#include "NxInternalTriangleMesh.h"
#include "NxSmoothNormals.h"
#include "FixtureSupport.h"
#include "MeshNormalsBudgets.h"
#undef for
#if NX_PHYSICS_USE_X87
#include <float.h>
#endif

static_assert(sizeof(MeshNormals) == 8, "actual raw32 normal receiver");
static_assert(sizeof(MESHNORMALSCREATE) == 32, "actual raw32 normal create block");
static_assert(sizeof(MeshBuilder2) == 0x124, "actual raw32 builder receiver");
static_assert(offsetof(InternalTriangleMesh, mVertexNormals) == 0x18,
    "actual internal normal owner");
static int failures;
static void check(bool ok, const char* why)
{
    if(!ok)
    {
        std::fprintf(stderr, "FAIL %s\n", why);
        ++failures;
    }
}
// Explicit host boundary, matching the topology/hull gates.
bool opcNovodeXSetIceError(const char*, const char*, int) { return false; }
void* opcNovodeXAlloc(size_t n)
{
    return nxGetSdkAllocator()->malloc(n, NX_MEMORY_PERSISTENT);
}
void opcNovodeXFree(void* p) { nxGetSdkAllocator()->free(p); }

class GuardAllocator : public SdkAllocator
{
public:
    std::map<void*, size_t> blocks;
    unsigned failAfter = 0;
    void* malloc(size_t n, NxMemoryType)
    {
        if(failAfter && --failAfter == 0)
            return 0;
        unsigned char* raw = static_cast<unsigned char*>(std::malloc(n + 32));
        if(!raw)
            return 0;
        std::memset(raw, 0x6a, 16);
        std::memset(raw + 16, 0xcd, n);
        std::memset(raw + 16 + n, 0x7b, 16);
        blocks[raw + 16] = n;
        return raw + 16;
    }
    void* mallocDEBUG(size_t n, const char*, int, const char*, NxMemoryType type)
    {
        return malloc(n, type);
    }
    void* realloc(void* p, size_t n)
    {
        if(!p)
            return malloc(n, NX_MEMORY_PERSISTENT);
        void* q = malloc(n, NX_MEMORY_PERSISTENT);
        if(q)
        {
            std::memcpy(q, p, n < blocks[p] ? n : blocks[p]);
            free(p);
        }
        return q;
    }
    void free(void* p)
    {
        if(!p)
            return;
        auto found = blocks.find(p);
        check(found != blocks.end(), "actual allocator owned block");
        if(found == blocks.end())
            return;
        unsigned char* raw = static_cast<unsigned char*>(p) - 16;
        for(unsigned j = 0; j < 16; ++j)
            check(raw[j] == 0x6a && raw[16 + found->second + j] == 0x7b,
                "allocator output canaries");
        blocks.erase(found);
        std::free(raw);
    }
};
class FoundationGuard : public NxUserAllocator
{
public:
    GuardAllocator& allocator;
    explicit FoundationGuard(GuardAllocator& a) : allocator(a) {}
    void* mallocDEBUG(size_t n, const char*, int) { return malloc(n); }
    void* malloc(size_t n) { return allocator.malloc(n, NX_MEMORY_PERSISTENT); }
    void* malloc(size_t n, NxMemoryType t) { return allocator.malloc(n, t); }
    void* realloc(void* p, size_t n) { return allocator.realloc(p, n); }
    void free(void* p) { allocator.free(p); }
};
struct Observation { unsigned kind, group, index, word; };
static std::vector<Observation> observations;
static unsigned group;
static double unitResidual;
static float value(unsigned word)
{
    float f;
    std::memcpy(&f, &word, 4);
    return f;
}
static void exact(unsigned word)
{
    observations.push_back({0, group, unsigned(observations.size()), word});
}
static void number(unsigned kind, float f)
{
    unsigned word;
    std::memcpy(&word, &f, 4);
    observations.push_back({kind, group, unsigned(observations.size()), word});
}
static void normal(const IceMaths::Point& p)
{
    number(1, p.x); number(1, p.y); number(1, p.z);
    const double square = (double(p.x) * p.x + double(p.y) * p.y) + double(p.z) * p.z;
    if(square)
    {
        const double residual = std::fabs(std::sqrt(square) - 1);
        if(residual > unitResidual)
            unitResidual = residual;
    }
}
static bool compute(MeshNormals& receiver, const MESHNORMALSCREATE& create)
{
#if NX_PHYSICS_USE_X87
    return nxMeshNormalsCompute(&receiver, 0, &create);
#else
    return nxMeshNormalsCompute(&receiver, &create);
#endif
}
static void internalNormals(InternalTriangleMesh& mesh)
{
#if NX_PHYSICS_USE_X87
    InternalTriangleMesh* receiver = &mesh;
    __asm
    {
        mov ecx, receiver
        call nxMeshComputeVertexNormals
    }
#else
    nxMeshComputeVertexNormals(&mesh);
#endif
}
static float cornerAngle(NxU32 vertex, const NxU32* refs, const IceMaths::Point* verts)
{
#if NX_PHYSICS_USE_X87
    float angle;
    __asm
    {
        mov eax, vertex
        mov edx, refs
        mov esi, verts
        call nxSmoothNormalsAngleAtVertex
        fstp angle
    }
    return angle;
#else
    return nxSmoothNormalsAngleAtVertex(vertex, refs,
        reinterpret_cast<const NxVec3*>(verts));
#endif
}
struct GuardPoints
{
    unsigned before;
    IceMaths::Point points[4];
    unsigned after;
    GuardPoints() : before(0x12345678), after(0x98765432)
    {
        std::memset(reinterpret_cast<unsigned char*>(points), 0xcd, sizeof(points));
    }
    void observeWords() const
    {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(points);
        for(unsigned i = 0; i < sizeof(points); i += 4)
        {
            unsigned word;
            std::memcpy(&word, bytes + i, 4);
            exact(word);
        }
    }
    void observe() const
    {
        exact(before); exact(after);
        check(before == 0x12345678 && after == 0x98765432, "borrowed output canaries");
    }
};
// Literal finite vertex words. Flat, positive/negative crease, affine/nonuniform
// scale/shear/translation, coincident and collinear surfaces; domain[-3,6].
static const unsigned meshes[][12] = {
    {0,0,0, 0x3f800000,0,0, 0,0x3f800000,0, 0x3f800000,0x3f800000,0},
    {0,0,0, 0x3f800000,0,0, 0,0x3f800000,0, 0x3f800000,0x3f800000,0x3f800000},
    {0,0,0, 0x3f800000,0,0, 0,0x3f800000,0, 0x3f800000,0x3f800000,0xbf800000},
    {0x3f800000,0xc0000000,0x40400000, 0x40400000,0xc0000000,0x40400000,
     0x3f800000,0xbfc00000,0x40400000, 0x40400000,0xbfc00000,0x40900000},
    {0x3e800000,0xbf000000,0x3fa00000, 0x3fa00000,0xbf000000,0x3fc66666,
     0x3eb33333,0x3f000000,0x3fa00000, 0x3faccccd,0x3f333333,0x400ccccd},
    {0,0,0, 0,0,0, 0,0x3f800000,0, 0x3f800000,0x3f800000,0},
    {0,0,0, 0x3f800000,0,0, 0x40000000,0,0, 0x40400000,0,0},
    {0x80000000,0,0, 0x3f800000,0x80000000,0, 0,0x3f800000,0x80000000,
     0x3f800000,0x3f800000,0x3dcccccd}
};
static void meshDomain(GuardAllocator& allocator)
{
    for(unsigned shape = 0; shape < sizeof(meshes)/sizeof(meshes[0]); ++shape)
    {
        IceMaths::Point verts[4];
        std::memcpy(reinterpret_cast<unsigned char*>(verts), meshes[shape], sizeof(verts));
        for(unsigned winding = 0; winding < 2; ++winding)
        {
            NxU32 refs[6] = {0,1,2, 1,3,2};
            if(winding)
            {
                std::swap(refs[1], refs[2]);
                std::swap(refs[4], refs[5]);
            }
            for(unsigned corner = 0; corner < 4; ++corner)
                number(3, cornerAngle(corner, refs, verts));
            NxU16 words[6];
            for(unsigned i = 0; i < 6; ++i) words[i] = NxU16(refs[i]);
            for(unsigned indexKind = 0; indexKind < 4; ++indexKind)
            {
                for(unsigned weighted = 0; weighted < 2; ++weighted)
                {
                    for(unsigned outputs = 0; outputs < 4; ++outputs)
                    {
                        MeshNormals receiver;
                        GuardPoints faces, normals;
                        MESHNORMALSCREATE create = {4, verts, 2,
                            indexKind == 0 || indexKind == 3 ? refs : 0,
                            indexKind == 1 || indexKind == 3 ? words : 0,
                            weighted != 0, outputs & 1 ? faces.points : 0,
                            outputs & 2 ? normals.points : 0};
                        for(unsigned repeat = 0; repeat < 2; ++repeat)
                        {
                            // Reuse the receiver's actual retained cache instead of
                            // abandoning old allocations on the second call.
                            if(repeat)
                            {
                                if(receiver.mFaceNormals) create.FaceNormals = receiver.mFaceNormals;
                                if(receiver.mVertexNormals) create.VertexNormals = receiver.mVertexNormals;
                            }
                            exact(compute(receiver, create));
                            exact(receiver.mFaceNormals != 0);
                            exact(receiver.mVertexNormals != 0);
                            const IceMaths::Point* f = create.FaceNormals ? create.FaceNormals : receiver.mFaceNormals;
                            const IceMaths::Point* v = create.VertexNormals ? create.VertexNormals : receiver.mVertexNormals;
                            for(unsigned i = 0; i < 2; ++i) normal(f[i]);
                            for(unsigned i = 0; i < 4; ++i) normal(v[i]);
                            faces.observe(); normals.observe();
                        }
                        ++group;
                    }
                }
                GuardPoints smooth;
                exact(NxBuildSmoothNormals(2, 4, reinterpret_cast<const NxVec3*>(verts),
                    indexKind == 0 || indexKind == 3 ? refs : 0,
                    indexKind == 1 || indexKind == 3 ? words : 0,
                    reinterpret_cast<NxVec3*>(smooth.points), winding != 0));
                for(const auto& p : smooth.points) normal(p);
                smooth.observe();
                ++group;
            }
            ConvexHull hull;
            std::memset(reinterpret_cast<unsigned char*>(&hull), 0, sizeof(hull));
            hull.mNbVerts = 4; hull.mVerts = verts; hull.mNbFaces = 2; hull.mFaces = words;
            for(unsigned repeat = 0; repeat < 2; ++repeat)
            {
                exact(hull.ComputeVertexNormals());
                for(unsigned i = 0; i < 4; ++i) normal(hull.mVertexNormals[i]);
            }
            nxIceFree(hull.mVertexNormals);
            InternalTriangleMesh mesh;
            std::memset(reinterpret_cast<unsigned char*>(&mesh), 0, sizeof(mesh));
            mesh.mVertexCount = 4; mesh.mTriangleCount = 2;
            mesh.mVertices = verts; mesh.mTriangles = refs;
            internalNormals(mesh);
            exact(mesh.mVertexNormals != 0);
            for(unsigned i = 0; i < 4; ++i)
                normal(static_cast<IceMaths::Point*>(mesh.mVertexNormals)[i]);
            allocator.free(mesh.mVertexNormals);
            ++group;
        }
    }
}
static void container(const IceCore::Container& values, unsigned kind)
{
    exact(values.GetNbEntries());
    for(unsigned i = 0; i < values.GetNbEntries(); ++i)
    {
        if(kind) number(kind, value(values.GetEntries()[i]));
        else exact(values.GetEntries()[i]);
    }
}
static void builderDomain()
{
    for(unsigned shape = 0; shape < sizeof(meshes)/sizeof(meshes[0]); ++shape)
    {
        IceMaths::Point verts[4];
        std::memcpy(reinterpret_cast<unsigned char*>(verts), meshes[shape], sizeof(verts));
        for(unsigned options = 0; options < 32; ++options)
        {
            for(unsigned reuse = 0; reuse < 2; ++reuse)
            {
                MeshBuilder2 builder;
                MBCREATE create = {};
                create.NbVerts = 4; create.NbFaces = 2; create.Verts = verts;
                create.NbTVerts = 4; create.TVerts = verts;
                create.NbCVerts = 4; create.CVerts = verts;
                create.KillZeroAreaFaces = false;
                create.ComputeVNormals = true; create.ComputeFNormals = (options & 1) != 0;
                create.ComputeNormInfo = (options & 2) != 0;
                create.WeightNormalWithAngles = (options & 4) != 0;
                create.IndexedGeo = create.IndexedUVW = create.IndexedColors = (options & 8) != 0;
                create.UseW = create.RelativeIndices = (options & 16) != 0;
                exact(builder.Init(create));
                NxU32 refs[2][3] = {{0,1,2},{1,3,2}};
                for(unsigned face = 0; face < 2; ++face)
                {
                    MBFACEINFO info = {face, face, (options & 16) ? face + 1 : 1,
                        refs[face], refs[face], refs[face], (options & 8) != 0};
                    exact(builder.AddFace(info));
                }
                MBRESULT result = {};
                exact(builder.Build(result));
                exact(result.NbFaces); exact(result.NbSubmeshes); exact(result.NbOutVerts);
                exact(result.NbVerts); exact(result.NbNormInfo); exact(result.UseW);
                container(builder.mTopology, 0); container(builder.mFacesPerRun, 0);
                container(builder.mVRefs, 0); container(builder.mTRefs, 0); container(builder.mCRefs, 0);
                container(builder.mVerts, 2); container(builder.mTVerts, 2); container(builder.mCVerts, 2);
                container(builder.mNormals, 1); container(builder.mFaceNormals, 1);
                container(builder.mNormInfo, 0); container(builder.mRuns, 0); container(builder.mMaterials, 0);
                exact(result.FaceRemap != 0);
                if(result.FaceRemap) for(unsigned i = 0; i < 2; ++i) exact(result.FaceRemap[i]);
            }
            ++group;
        }
        // Actual zero-area admission decision, including repeated references.
        MeshBuilder2 builder;
        MBCREATE create = {};
        create.NbVerts = 4; create.NbFaces = 2; create.Verts = verts;
        create.KillZeroAreaFaces = true;
        exact(builder.Init(create));
        NxU32 refs[3] = {0,1,2};
        MBFACEINFO info = {0,0,1,refs,0,0,false};
        exact(builder.AddFace(info)); exact(builder.mNbFaces);
        refs[1] = 0;
        exact(builder.AddFace(info)); exact(builder.mNbFaces);
        ++group;
    }
}
static void poseDomain()
{
    const float poses[][16] = {
        {1,0,0,0, 0,1,0,0, 0,0,1,0, 1,-2,3,1},
        {0,1,0,0, -1,0,0,0, 0,0,1,0, -3,2,1,1},
        {0.6f,0.8f,0,0, -0.8f,0.6f,0,0, 0,0,1,0, 0.1f,-0.2f,0.3f,1}
    };
    for(unsigned a = 0; a < 4; ++a)
    {
        for(unsigned b = 0; b < 4; ++b)
        {
            IceMaths::Matrix4x4 p0, p1;
            if(a) std::memcpy(reinterpret_cast<unsigned char*>(&p0), poses[a-1], sizeof(p0));
            if(b) std::memcpy(reinterpret_cast<unsigned char*>(&p1), poses[b-1], sizeof(p1));
            for(unsigned outputs = 0; outputs < 4; ++outputs)
            {
                struct GuardMatrix { unsigned before; IceMaths::Matrix4x4 matrix; unsigned after; };
                GuardMatrix r0, r1;
                std::memset(reinterpret_cast<unsigned char*>(&r0), 0xcd, sizeof(r0)); std::memset(reinterpret_cast<unsigned char*>(&r1), 0xcd, sizeof(r1));
                nxIcePosePair(outputs & 1 ? &r0.matrix : 0, outputs & 2 ? &r1.matrix : 0,
                    a ? &p0 : 0, b ? &p1 : 0);
                for(unsigned out = 0; out < 2; ++out)
                {
                    const GuardMatrix& r = out ? r1 : r0;
                    exact(r.before); exact(r.after);
                    check(r.before == 0xcdcdcdcd && r.after == 0xcdcdcdcd, "relative pose canaries");
                    for(unsigned row = 0; row < 4; ++row)
                    {
                        for(unsigned col = 0; col < 4; ++col)
                        {
                            if(!(outputs & (1u << out)))
                            {
                                unsigned word; std::memcpy(&word, &r.matrix.m[row][col], 4); exact(word);
                            }
                            else number(row == 3 && col != 3 ? 2 : 1, r.matrix.m[row][col]);
                        }
                    }
                }
                ++group;
            }
        }
    }
}
static void failuresDomain(GuardAllocator& allocator)
{
    IceMaths::Point verts[4] = {{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
    NxU32 refs[3] = {0,1,2};
    GuardPoints faces, vertices;
    MESHNORMALSCREATE create = {4, 0, 1, refs, 0, true, faces.points, vertices.points};
    MeshNormals receiver;
    exact(compute(receiver, create)); faces.observe(); vertices.observe();
    faces.observeWords(); vertices.observeWords();
    create.Verts = verts;
    create.NbFaces = 0;
    exact(compute(receiver, create));
    for(const auto& p : vertices.points) normal(p);
    faces.observeWords();
    create.NbFaces = 1; create.NbVerts = 0;
    exact(compute(receiver, create)); normal(faces.points[0]);
    // NbVerts0 skips normalization after still accumulating into the borrowed
    // outputs. These are dimensionless weighted sums, not valid unit normals.
    for(const auto& p : vertices.points)
    {
        number(1, p.x); number(1, p.y); number(1, p.z);
    }
    create.NbVerts = 4; create.FaceNormals = create.VertexNormals = 0;
    allocator.failAfter = 1;
    exact(compute(receiver, create));
    exact(receiver.mFaceNormals == 0 && receiver.mVertexNormals == 0);
    allocator.failAfter = 2;
    exact(compute(receiver, create));
    exact(receiver.mFaceNormals == 0 && receiver.mVertexNormals == 0);
    // Historical second-allocation failure abandons first allocation; observe
    // this exact ownership outcome before releasing it for fixture bookkeeping.
    exact(unsigned(allocator.blocks.size()));
    while(!allocator.blocks.empty()) allocator.free(allocator.blocks.begin()->first);
    ConvexHull hull;
    std::memset(reinterpret_cast<unsigned char*>(&hull), 0, sizeof(hull));
    exact(hull.ComputeVertexNormals());
    hull.mNbVerts = 4; hull.mVerts = verts;
    allocator.failAfter = 1;
    exact(hull.ComputeVertexNormals()); exact(hull.mVertexNormals == 0);
    GuardPoints smooth;
    exact(NxBuildSmoothNormals(0,4,reinterpret_cast<NxVec3*>(verts),refs,0,
        reinterpret_cast<NxVec3*>(smooth.points),false));
    exact(NxBuildSmoothNormals(1,0,reinterpret_cast<NxVec3*>(verts),refs,0,
        reinterpret_cast<NxVec3*>(smooth.points),false));
    smooth.observe(); smooth.observeWords();
    // Supported allocation failure stores null before the smooth-normal call.
    InternalTriangleMesh mesh;
    std::memset(reinterpret_cast<unsigned char*>(&mesh), 0, sizeof(mesh));
    mesh.mVertexCount = 4; mesh.mTriangleCount = 1;
    mesh.mVertices = verts; mesh.mTriangles = refs;
    allocator.failAfter = 1;
    internalNormals(mesh);
    exact(mesh.mVertexNormals == 0);
    // Re-Init retains counters. Capture that outcome without reading newly
    // allocated/uninitialized face records in the unsupported second Build.
    MeshBuilder2 builder;
    MBCREATE mb = {};
    mb.NbVerts = 4; mb.NbFaces = 1; mb.Verts = verts;
    exact(builder.Init(mb));
    MBFACEINFO face = {0,0,1,refs,0,0,false};
    exact(builder.AddFace(face));
    exact(builder.Init(mb));
    exact(builder.mNbFaces); exact(builder.mNbRefs);
    exact(builder.AddFace(face));
    MBRESULT result;
    MeshBuilder2 empty;
    exact(empty.Build(result));
    ++group;
}
static void put(FILE* out, unsigned v)
{
    unsigned char b[4] = {static_cast<unsigned char>(v), static_cast<unsigned char>(v >> 8), static_cast<unsigned char>(v >> 16), static_cast<unsigned char>(v >> 24)};
    std::fwrite(b, 1, 4, out);
}
static unsigned get(const unsigned char* p)
{
    return unsigned(p[0]) | (unsigned(p[1]) << 8) | (unsigned(p[2]) << 16) | (unsigned(p[3]) << 24);
}
int main(int argc, char** argv)
{
    if(argc != 2) return 2;
#if NX_PHYSICS_USE_X87
    if((_controlfp(0,0) & (_MCW_PC|_MCW_RC)) != (_PC_53|_RC_NEAR)) return 3;
#endif
    GuardAllocator allocator;
    FoundationGuard foundation(allocator);
    nxSetSdkAllocatorBridge(&allocator);
    nxFoundationSDKAllocator = &foundation;
    meshDomain(allocator);
    builderDomain();
    const unsigned poseObservationStart = unsigned(observations.size());
    poseDomain();
    failuresDomain(allocator);
    check(allocator.blocks.empty(), "all retained allocator outputs released");
    nxFoundationSDKAllocator = 0;
    nxSetSdkAllocatorBridge(0);
#if NX_PHYSICS_USE_X87
    FILE* out = std::fopen(argv[1], "wb");
    if(!out) return 4;
    std::fwrite("NXPF", 1, 4, out); put(out, 1); put(out, unsigned(observations.size()) * 20); put(out, 20);
    for(const auto& o : observations)
    {
        put(out, o.kind); put(out, o.group); put(out, o.index); put(out, 0); put(out, o.word);
    }
    std::fclose(out);
#else
    std::vector<unsigned char> bytes;
    std::string error;
    const bool loaded = nxReadFixture(argv[1], bytes, error);
    check(loaded, error.c_str());
    check(bytes.size() == observations.size() * 20, "actual mesh semantic count");
    double maxima[4] = {}, relative[4] = {};
    unsigned differences[4] = {};
    double normalAngularDifference = 0;
    unsigned normalComponent = 0;
    if(bytes.size() == observations.size() * 20)
    {
        for(unsigned i = 0; i < observations.size(); ++i)
        {
            const Observation& o = observations[i];
            const unsigned char* p = &bytes[i * 20];
            check(get(p) == o.kind && get(p+4) == o.group && get(p+8) == o.index && get(p+12) == 0,
                "actual semantic identity");
            const unsigned reference = get(p+16);
            if(o.kind != 1) normalComponent = 0;
            if(i < poseObservationStart && o.kind == 1 && (normalComponent++ % 3) == 0 && i + 2 < poseObservationStart
                && observations[i+1].kind == 1 && observations[i+2].kind == 1)
            {
                // Only complete normal triples from actual outputs, not poses.
                const double ax = value(o.word), ay = value(observations[i+1].word), az = value(observations[i+2].word);
                const double bx = value(reference), by = value(get(p+36)), bz = value(get(p+56));
                const double x = ay*bz-az*by, y = az*bx-ax*bz, z = ax*by-ay*bx;
                const double angle = std::atan2(std::sqrt((x*x+y*y)+z*z), (ax*bx+ay*by)+az*bz);
                if(angle > normalAngularDifference) normalAngularDifference = angle;
            }
            const double delta = o.kind ? std::fabs(double(value(o.word)) - value(reference)) : 0;
            if(delta > maxima[o.kind]) maxima[o.kind] = delta;
            if(o.kind && value(reference) != 0)
            {
                const double r = delta / std::fabs(value(reference));
                if(r > relative[o.kind]) relative[o.kind] = r;
            }
            if(o.word != reference) ++differences[o.kind];
            const bool accepted = o.kind == 0 || o.kind == 3 || value(reference) == 0
                ? o.word == reference
                : nxWithinBudget(value(o.word), value(reference),
                    o.kind == 2 ? nxMeshLengthAbsoluteBudget : nxMeshNormalAbsoluteBudget,
                    nxMeshRelativeBudget);
            if(!accepted)
            {
                std::fprintf(stderr, "row=%u kind=%u group=%u actual=%08x reference=%08x\n",
                    i, o.kind, o.group, o.word, reference);
                ++failures;
            }
        }
    }
    check(normalAngularDifference <= nxMeshNormalAngularBudget, "approved normal angular difference");
    check(unitResidual <= nxMeshUnitResidualBudget, "approved valid unit residual");
    std::printf("MeshNormals groups=%u observations=%zu normal_maxabs=%.17g length_maxabs=%.17g normal_maxrel=%.17g length_maxrel=%.17g normal_diffs=%u length_diffs=%u exact_diffs=%u unit_residual=%.17g angle_maxabs=%.17g angle_maxrel=%.17g angular_difference=%.17g failures=%d\n",
        group, observations.size(), maxima[1], maxima[2], relative[1], relative[2],
        differences[1], differences[2], differences[0], unitResidual, maxima[3], relative[3], normalAngularDifference, failures);
#endif
    return failures ? 1 : 0;
}
