#include "ContactPairManager.h"
#include "NxPlane.h"
#include "NxMat33.h"
#include "NxUtilities.h"
#include <cmath>
#include <cstring>
#include "NxSdkAllocator.h"
#include "FoundationSDK.h"
#include <new>
#include <limits>

void __fastcall NxEmitContactFeatures(NxContactSink* sink, NxU32,
    void* object1, void* object0, NxU32 separationBits, const NxVec3* point,
    const NxVec3* normal, NxU32 featureId0, NxU32 featureId1,
    NxU32 featureWord0, NxU32 featureWord1)
{
    // Legacy opaque view only: actual receiver is the constructed actor pair.
    NxActorPair* pair = static_cast<NxActorPair*>(static_cast<void*>(sink));
    pair->row000875(static_cast<const NxU8*>(object1), static_cast<const NxU8*>(object0),
        separationBits, &point->x, &normal->x, static_cast<NxU16>(featureId0),
        static_cast<NxU16>(featureId1), featureWord0, featureWord1);
}

NxU32 nxPolygonContainsPoint(NxU32 count, const NxVec3* vertices, float x, float y)
{
    const NxVec3* previous = vertices + count - 1;
    bool previousAbove = previous->y >= y;
    NxU32 crossings = 0;
    for(NxU32 i = 0; i < count; ++i)
    {
        const NxVec3& current = vertices[i];
        const bool above = current.y >= y;
        if(previousAbove != above)
        {
            const double left = (double(previous->y) - current.y) * (double(current.x) - x);
            const double right = (double(previous->x) - current.x) * (double(current.y) - y);
            if((left <= right) == above)
            {
                if(crossings == 1) return 0;
                ++crossings;
            }
        }
        previous = &current;
        previousAbove = above;
    }
    return crossings & 1;
}

NxU32 nxClipEdgeToPolygonPlane(const NxVec3* start0, const float* plane,
    NxVec3* point, const NxVec3* edge1Direction, const NxVec3* start1,
    const NxVec3* end1, const NxVec3* normal0, const NxVec3* end0, float* parameter)
{
    const float startXProduct = float(double(start0->x) * plane[0]);
    const double startValue = ((double(plane[1]) * start0->y + double(start0->z) * plane[2]) + startXProduct) + plane[3];
    const double endValue = ((double(plane[1]) * end0->y + double(end0->z) * plane[2]) + double(end0->x) * plane[0]) + plane[3];
    if(endValue * startValue > 0.0) return 0;

    double dx = double(end0->x) - start0->x;
    float dy = float(double(end0->y) - start0->y);
    const double wideDz = double(end0->z) - start0->z;
    float dz = float(wideDz);
    const double squared = (wideDz * dz + dx * dx) + double(dy) * dy;
    if(squared != 0.0)
    {
        const double inverse = 1.0 / std::sqrt(squared);
        dx *= inverse;
        dy = float(double(dy) * inverse);
        dz = float(double(dz) * inverse);
    }
    const double denominator = (double(dy) * plane[1] + double(dz) * plane[2]) + dx * plane[0];
    if(denominator == 0.0) return 0;
    const float distance = float(startValue / denominator);
    const float alongZ = float(double(dz) * distance);
    point->x = float(double(start0->x) - dx * distance);
    point->y = float(double(start0->y) - double(dy) * distance);
    point->z = float(double(start0->z) - alongZ);

    NxU32 magnitude[3];
    for(unsigned i = 0; i < 3; ++i) { std::memcpy(&magnitude[i], plane + i, 4); magnitude[i] &= 0x7fffffff; }
    unsigned major = magnitude[1] > magnitude[0] ? 1 : 0;
    unsigned first, second;
    if(magnitude[2] > magnitude[major]) first = 0, second = 1;
    else first = major == 0 ? 1 : 0, second = 2;
    const double numerator = (double((*point)[second]) - (*start1)[second]) * (*edge1Direction)[first]
        - (double((*point)[first]) - (*start1)[first]) * (*edge1Direction)[second];
    const double divisor = double((*normal0)[second]) * (*edge1Direction)[first]
        - double((*normal0)[first]) * (*edge1Direction)[second];
    const double crossing = numerator / divisor;
    *parameter = float(crossing);
    if(crossing < 0.0) return 0;
    const float alongY = float(crossing * normal0->y);
    const float alongNormalZ = float(crossing * normal0->z);
    const double px = double(point->x) - crossing * normal0->x;
    const double py = double(point->y) - alongY;
    const double pz = double(point->z) - alongNormalZ;
    point->x = float(px); point->y = float(py); point->z = float(pz);
    const double within = ((double(start1->z) - pz) * (double(end1->z) - pz)
        + (double(start1->y) - py) * (double(end1->y) - py))
        + (double(start1->x) - px) * (double(end1->x) - px);
    return within < 0.0 ? 1 : 0;
}

namespace {
class PolygonScratchVertices
{
public:
    explicit PolygonScratchVertices(NxU32 count) : mData(nullptr), mCount(count)
    {
        if(count > std::numeric_limits<size_t>::max() / sizeof(NxVec3)) return;
        mData = static_cast<NxVec3*>(nxGetSdkAllocator()->malloc(size_t(count) * sizeof(NxVec3), NX_MEMORY_TEMP));
        if(mData) for(NxU32 i = 0; i < count; ++i) new(mData + i) NxVec3;
    }
    ~PolygonScratchVertices()
    {
        if(mData) {
            for(NxU32 i = 0; i < mCount; ++i) mData[i].~NxVec3();
            nxGetSdkAllocator()->free(mData);
        }
    }
    NxVec3* get() const { return mData; }
private:
    PolygonScratchVertices(const PolygonScratchVertices&);
    PolygonScratchVertices& operator=(const PolygonScratchVertices&);
    NxVec3* mData;
    NxU32 mCount;
};

// Every pose call has its own original term order. Translation is added wide
// before the final float store; no common reassociated dot product is valid here.
NxVec3 polygonRelative01Point(const float* m, const NxVec3& v)
{
    // 0x4914f..0x4919f: x=(x+y)+z; y=(x+y)+z; z=(z+y)+x.
    return NxVec3(float(((double(v.x)*m[0]+double(v.y)*m[4])+double(v.z)*m[8])+m[12]),
        float(((double(v.x)*m[1]+double(v.y)*m[5])+double(v.z)*m[9])+m[13]),
        float(((double(v.z)*m[10]+double(v.y)*m[6])+double(v.x)*m[2])+m[14]));
}
NxVec3 polygonEmitFirstPoint(const float* m, const NxVec3& v)
{
    // 0x499de..0x49a45: every coordinate=(z+y)+x.
    return NxVec3(float(((double(v.z)*m[8]+double(v.y)*m[4])+double(v.x)*m[0])+m[12]),
        float(((double(v.z)*m[9]+double(v.y)*m[5])+double(v.x)*m[1])+m[13]),
        float(((double(v.z)*m[10]+double(v.y)*m[6])+double(v.x)*m[2])+m[14]));
}
NxVec3 polygonEmitSecondPoint(const float* m, const NxVec3& v)
{
    // 0x495b9..0x4961d: x=(z+x)+y; y=(z+y)+x; z=(z+x)+y.
    return NxVec3(float(((double(v.z)*m[8]+double(v.x)*m[0])+double(v.y)*m[4])+m[12]),
        float(((double(v.z)*m[9]+double(v.y)*m[5])+double(v.x)*m[1])+m[13]),
        float(((double(v.z)*m[10]+double(v.x)*m[2])+double(v.y)*m[6])+m[14]));
}
NxVec3 polygonRelative10Point(const float* m, const NxVec3& v)
{
    // Unrolled 0x4969f..0x49855 and tail0x4988e..0x498f6 have identical
    // stores: x=(x+y)+z; y=(y+x)+z; z=(z+x)+y.
    return NxVec3(float(((double(v.x)*m[0]+double(v.y)*m[4])+double(v.z)*m[8])+m[12]),
        float(((double(v.y)*m[5]+double(v.x)*m[1])+double(v.z)*m[9])+m[13]),
        float(((double(v.z)*m[10]+double(v.x)*m[2])+double(v.y)*m[6])+m[14]));
}
NxVec3 polygonEmitEdgePoint(const float* m, const NxVec3& v)
{
    // 0x49bbd..0x49c39: every coordinate=(y+z)+x; retained wide y/z
    // values are stored after x, with no intermediate float narrowing.
    return NxVec3(float(((double(v.y)*m[4]+double(v.z)*m[8])+double(v.x)*m[0])+m[12]),
        float(((double(v.y)*m[5]+double(v.z)*m[9])+double(v.x)*m[1])+m[13]),
        float(((double(v.y)*m[6]+double(v.z)*m[10])+double(v.x)*m[2])+m[14]));
}
void polygonCompose(const float* r, const float* m, float (&c)[12])
{
    c[0] = float((double(r[0]) * m[0] + double(r[1]) * m[1]) + double(r[2]) * m[2]);
    c[1] = float((double(r[0]) * m[4] + double(r[2]) * m[6]) + double(r[1]) * m[5]);
    c[2] = float((double(r[0]) * m[8] + double(r[2]) * m[10]) + double(r[1]) * m[9]);
    c[3] = float((double(r[0]) * m[12] + double(r[2]) * m[14]) + double(r[1]) * m[13]);
    c[4] = float((double(r[4]) * m[1] + double(r[5]) * m[2]) + double(r[3]) * m[0]);
    c[5] = float((double(r[5]) * m[6] + double(r[3]) * m[4]) + double(r[4]) * m[5]);
    c[6] = float((double(r[5]) * m[10] + double(r[3]) * m[8]) + double(r[4]) * m[9]);
    c[7] = float((double(r[5]) * m[14] + double(r[3]) * m[12]) + double(r[4]) * m[13]);
    c[8] = float((double(r[8]) * m[2] + double(r[7]) * m[1]) + double(r[6]) * m[0]);
    c[9] = float((double(r[8]) * m[6] + double(r[6]) * m[4]) + double(r[7]) * m[5]);
    c[10] = float((double(r[8]) * m[10] + double(r[6]) * m[8]) + double(r[7]) * m[9]);
    c[11] = float((double(r[8]) * m[14] + double(r[6]) * m[12]) + double(r[7]) * m[13]);
}
double polygonProjection(const float* row, const NxVec3& v)
{
    return ((double(row[2]) * v.z + double(row[1]) * v.y) + double(row[0]) * v.x) + row[3];
}
void polygonProject(NxVec3* output, NxU32 count, const NxVec3* vertices,
    const NxU32* refs, const float* r)
{
    for(NxU32 i = 0; i < count; ++i) {
        const NxVec3& v = vertices[refs[i]];
        output[i].x = float((double(r[1]) * v.y + double(r[2]) * v.z) + double(r[0]) * v.x);
        output[i].y = float((double(r[3]) * v.x + double(r[4]) * v.y) + double(r[5]) * v.z);
    }
}
float polygonAnchor(const float* r, const NxVec3& v)
{
    return float((double(r[7]) * v.y + double(r[8]) * v.z) + double(r[6]) * v.x);
}
NxU32 polygonFloatBits(float value)
{
    NxU32 word; std::memcpy(&word, &value, 4); return word;
}
}

void NxConvexPolygonContacts(NxU32 count0, const NxVec3* vertices0, const NxU32* refs0,
    const float* pose0, const NxPlane* plane0, NxU32 count1,
    const NxVec3* vertices1, const NxU32* refs1, const float* pose1,
    const NxPlane* plane1, const NxVec3* displacement, const float* relative01,
    const float* relative10, const NxCollisionShape* shape0,
    const NxCollisionShape* shape1, NxContactSink* sink, NxU32, NxU32,
    NxU32 featureId0, NxU32 featureId1, NxU32 featureWord0, NxU32 featureWord1)
{
    // Original count/product is12*max(count0,count1), then12*count1. Both are
    // allocated before emission so portable allocation failure cannot leave a
    // partial contact stream. This is an explicit portable failure disposition.
    PolygonScratchVertices projectedStorage(count0 > count1 ? count0 : count1);
    PolygonScratchVertices transformedStorage(count1);
    if(!projectedStorage.get() || !transformedStorage.get()) {
        NxFoundation::FoundationSDK::error(NXE_OUT_OF_MEMORY, __FILE__, __LINE__, nullptr,
            "Convex polygon contact scratch allocation failed");
        return;
    }
    NxVec3* projected = projectedStorage.get();
    NxVec3* transformed = transformedStorage.get();
    const NxVec3 emittedNormal(-displacement->x, -displacement->y, -displacement->z);
    const NxVec3 zAxis(0, 0, 1);
    NxMat33 rotation;
    float r[9], c[12];
    NxFindRotationMatrix(plane1->normal, zAxis, rotation);
    rotation.getRowMajor(r);
    polygonProject(projected, count1, vertices1, refs1, r);
    const float anchor1 = polygonAnchor(r, vertices1[refs1[0]]);
    polygonCompose(r, relative01, c);
    const NxVec3 localDirection(
        float((double(emittedNormal.y) * pose1[1] + double(emittedNormal.z) * pose1[2]) + double(emittedNormal.x) * pose1[0]),
        float((double(emittedNormal.x) * pose1[4] + double(emittedNormal.y) * pose1[5]) + double(emittedNormal.z) * pose1[6]),
        float((double(emittedNormal.x) * pose1[8] + double(emittedNormal.y) * pose1[9]) + double(emittedNormal.z) * pose1[10]));
    const double denominator = (double(localDirection.y) * plane1->normal.y + double(localDirection.z) * plane1->normal.z)
        + double(localDirection.x) * plane1->normal.x;
    for(NxU32 i = 0; i < count0; ++i) {
        const NxVec3& vertex = vertices0[refs0[i]];
        if(!(polygonProjection(c + 8, vertex) < anchor1)) continue;
        const NxVec3 inOther = polygonRelative01Point(relative01, vertex);
        if(denominator > -1e-7 && denominator < 1e-7) continue;
        const float parameter = float(((double(inOther.z) * plane1->normal.z + double(inOther.x) * plane1->normal.x)
            + double(inOther.y) * plane1->normal.y + plane1->d) / denominator);
        if(!(parameter < 0.0f)) continue;
        const float intersectionX = float(double(inOther.x) - double(localDirection.x) * parameter);
        const double intersectionY = double(inOther.y) - double(localDirection.y) * parameter;
        const float alongZ = float(double(localDirection.z) * parameter);
        const double intersectionZ = double(inOther.z) - alongZ;
        const float x = float((intersectionY * r[1] + intersectionZ * r[2]) + double(intersectionX) * r[0]);
        const float y = float((intersectionY * r[4] + intersectionZ * r[5]) + double(intersectionX) * r[3]);
        if(!nxPolygonContainsPoint(count1, projected, x, y)) continue;
        const NxVec3 point = polygonEmitFirstPoint(pose0, vertex);
        NxEmitContactFeatures(sink, 0, shape0->collisionObject, shape1->collisionObject, polygonFloatBits(parameter),
            &point, &emittedNormal, featureId0, featureId1, featureWord0, featureWord1);
    }
    NxFindRotationMatrix(plane0->normal, zAxis, rotation);
    rotation.getRowMajor(r);
    polygonProject(projected, count0, vertices0, refs0, r);
    const float anchor0 = polygonAnchor(r, vertices0[refs0[0]]);
    polygonCompose(r, relative10, c);
    for(NxU32 i = 0; i < count1; ++i) {
        const NxVec3& vertex = vertices1[refs1[i]];
        const double depth = polygonProjection(c + 8, vertex);
        const float storedDepth = float(depth);
        if(!(depth < anchor0)) continue;
        const float x = float(polygonProjection(c, vertex));
        const float y = float(polygonProjection(c + 4, vertex));
        if(!nxPolygonContainsPoint(count0, projected, x, y)) continue;
        const NxVec3 point = polygonEmitSecondPoint(pose1, vertex);
        const float separation = float(double(storedDepth) - anchor0);
        NxEmitContactFeatures(sink, 0, shape0->collisionObject, shape1->collisionObject, polygonFloatBits(separation),
            &point, &emittedNormal, featureId0, featureId1, featureWord0, featureWord1);
    }
    for(NxU32 i = 0; i < count1; ++i) transformed[i] = polygonRelative10Point(relative10, vertices1[refs1[i]]);
    for(NxU32 edge1 = 0; edge1 < count1; ++edge1) {
        const NxVec3& begin1 = transformed[edge1];
        const NxVec3& end1 = transformed[edge1 + 1 < count1 ? edge1 + 1 : 0];
        const NxVec3 edge(float(double(end1.x) - begin1.x), float(double(end1.y) - begin1.y), float(double(end1.z) - begin1.z));
        NxVec3 cross(float(double(edge.y) * plane0->normal.z - double(edge.z) * plane0->normal.y),
            float(double(edge.z) * plane0->normal.x - double(edge.x) * plane0->normal.z),
            float(double(edge.x) * plane0->normal.y - double(edge.y) * plane0->normal.x));
        const double squared = (double(cross.z) * cross.z + double(cross.y) * cross.y) + double(cross.x) * cross.x;
        if(squared != 0.0) {
            const double inverse = 1.0 / std::sqrt(squared);
            cross.x = float(double(cross.x) * inverse); cross.y = float(double(cross.y) * inverse); cross.z = float(double(cross.z) * inverse);
        }
        const float edgePlane[4] = {cross.x, cross.y, cross.z,
            float(-((double(cross.z) * begin1.z + double(cross.x) * begin1.x) + double(cross.y) * begin1.y))};
        for(NxU32 edge0 = 0; edge0 < count0; ++edge0) {
            const NxVec3& begin0 = vertices0[refs0[edge0]];
            const NxVec3& end0 = vertices0[refs0[edge0 + 1 < count0 ? edge0 + 1 : 0]];
            NxVec3 localPoint; float parameter;
            if(!nxClipEdgeToPolygonPlane(&begin0, edgePlane, &localPoint, &edge, &begin1, &end1, &plane0->normal, &end0, &parameter)) continue;
            const NxVec3 point = polygonEmitEdgePoint(pose0, localPoint);
            NxEmitContactFeatures(sink, 0, shape0->collisionObject, shape1->collisionObject, polygonFloatBits(-parameter),
                &point, &emittedNormal, featureId0, featureId1, featureWord0, featureWord1);
        }
    }
}
