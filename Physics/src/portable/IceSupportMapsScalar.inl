// Ordinary translations of001550..001589. Preserve binary32 stores, each
// continuation's term order and qword-low32 integer decisions. Native receiver
// layout migration remains separate; this file consumes the actual types.
#include "NxScalarConversions.h"
#include <cmath>
#include <cstring>
extern "C" int __cdecl _purecall(void);
const void *const gIceSupportMapBaseTable[4] = {(const void *)&nxSupportMapBaseDelete,
                                                (const void *)&_purecall, (const void *)&_purecall,
                                                (const void *)&nxSupportMapNoop};
const void *const gIceSupportMapHullTable[4] = {
    (const void *)&nxSupportMapHullDelete, (const void *)&nxSupportMapHullAllocate,
    (const void *)&nxSupportMapHullCompute, (const void *)&nxSupportMapNoop};
const void *const gIceSupportMapPlaneTable[4] = {
    (const void *)&nxSupportMapPlaneDelete, (const void *)&nxSupportMapHullAllocate,
    (const void *)&nxSupportMapPlaneCompute, (const void *)&nxSupportMapNoop};
const void *const gIceSupportMapVertexTable[4] = {
    (const void *)&nxSupportMapVertexDelete, (const void *)&nxSupportMapVertexAllocate,
    (const void *)&nxSupportMapVertexCompute, (const void *)&nxSupportMapNoop};
static void nxSupportMapFree(void *memory)
{
    nxGetSdkAllocator()->free(memory);
}
NxU32 nxSupportMapCubeFace(const IceMaths::Point *direction, float *u, float *v)
{
    NxU32 words[3];
    std::memcpy(words, direction, sizeof(words));
    NxU32 axis = 0;
    if ((words[1] & 0x7fffffff) > (words[0] & 0x7fffffff))
        axis = 1;
    if ((words[2] & 0x7fffffff) > (words[axis] & 0x7fffffff))
        axis = 2;
    const float components[3] = {direction->x, direction->y, direction->z};
    const double reciprocal = 1.0 / std::fabs(double(components[axis]));
    const NxU32 packed = 0x01000201 >> (axis * 8);
    *u = float(reciprocal * double(components[packed & 0xff]));
    *v = float(reciprocal * double(components[(packed >> 8) & 0xff]));
    return axis * 2 | (words[axis] >> 31);
}
void *nxSupportMapBaseConstruct(IceSupportMap *map)
{
    map->mVtable = gIceSupportMapBaseTable;
    map->mSubdiv = 0;
    map->mNbSamples = 0;
    return map;
}
void nxSupportMapBaseTable(IceSupportMap *map)
{
    map->mVtable = gIceSupportMapBaseTable;
}
NxU32 nxSupportMapLookup(const IceSupportMap *map, const IceMaths::Point *direction)
{
    float u, v;
    const NxU32 face = nxSupportMapCubeFace(direction, &u, &v);
    const NxU32 n = map->mSubdiv;
    const double half = double(NxU32(n - 1)) * 0.5;
    const float scaledU = float((double(u) + 1.0) * half); // 2e268 fstp dword
    const double scaledV = (double(v) + 1.0) * half;       // retained until2e286 fistp
    NxU32 indexU = NxU32(nxScalarFistpLow32(double(scaledU)));
    NxU32 indexV = NxU32(nxScalarFistpLow32(scaledV));
    // Both reloads use fild signed dword plus2^32 when negative, i.e. unsigned.
    // NaN comparisons are false; every increment/multiply wraps modulo2^32.
    if (double(scaledU) - double(indexU) > 0.5)
        ++indexU;
    if (scaledV - double(indexV) > 0.5)
        ++indexV;
    return (face * n + indexU) * n + indexV;
}
bool nxSupportMapInit(IceSupportMap *map, NxU32 subdiv)
{
    map->mSubdiv = subdiv;
    map->mNbSamples = 6 * subdiv * subdiv;
    if (!((NxSupportMapAllocateSlot)map->mVtable[1])(map))
        return false;
    const float half = float(double(NxU32(subdiv - 1)) * 0.5); // 2e33f store
    for (NxU32 face = 0; face < 6; ++face)
    {
        for (NxU32 j = 0; j < subdiv; ++j)
        {
            for (NxU32 i = 0; i < subdiv; ++i)
            {
                const double spacing = 1.0 / double(half);
                const float a = float(1.0 - double(i) * spacing);
                const float b = float(1.0 - double(j) * spacing);
                const float dominant = (face & 1) ? 1.0f : -1.0f;
                IceMaths::Point direction;
                if (face < 2)
                    direction.Set(dominant, a, b);
                else if (face < 4)
                    direction.Set(b, dominant, a);
                else
                    direction.Set(a, b, dominant);
                const double square =
                    (double(direction.z) * direction.z + double(direction.y) * direction.y) +
                    double(direction.x) * direction.x;
                if (square != 0.0)
                {
                    const double reciprocal = 1.0 / std::sqrt(square);
                    direction.x = float(double(direction.x) * reciprocal);
                    direction.y = float(double(direction.y) * reciprocal);
                    direction.z = float(double(direction.z) * reciprocal);
                }
                const NxU32 sample = face * subdiv * subdiv + j + i * subdiv;
                if (!((NxSupportMapComputeSlot)map->mVtable[2])(map, sample, &direction))
                    return false;
            }
        }
    }
    ((NxSupportMapFinishSlot)map->mVtable[3])(map);
    return true;
}
IceSupportMap *nxSupportMapBaseDelete(IceSupportMap *map, NxU32 flags)
{
    nxSupportMapBaseTable(map);
    if (flags & 1)
        nxSupportMapFree(map);
    return map;
}
IceSupportMap *nxSupportMapHullConstruct(IceSupportMap *map, ConvexHull *hull)
{
    nxSupportMapBaseConstruct(map);
    map->mHull = hull;
    map->mVtable = gIceSupportMapHullTable;
    map->mSamples = 0;
    return map;
}
bool nxSupportMapHullAllocate(IceSupportMap *map)
{
    ConvexHull *hull = map->mHull;
    if (!hull->mNbPolygons)
        nxHullComputePolygons(hull);
    if (hull->mNbPolygons > 255)
        return false;
    map->mSamples = (NxU8 *)nxGetSdkAllocator()->malloc(map->mNbSamples, NX_MEMORY_PERSISTENT);
    return map->mSamples != 0;
}
bool nxSupportMapHullCompute(IceSupportMap *map, NxU32 sample, const IceMaths::Point *direction)
{
    map->mSamples[sample] = NxU8(nxHullSupportPolygon(map->mHull, direction, 0));
    return true;
}
IceSupportMap *nxSupportMapPlaneConstruct(IceSupportMap *map, ConvexHull *hull)
{
    nxSupportMapBaseConstruct(map);
    map->mHull = hull;
    map->mVtable = gIceSupportMapPlaneTable;
    map->mSamples = 0;
    return map;
}
bool nxSupportMapPlaneCompute(IceSupportMap *map, NxU32 sample, const IceMaths::Point *direction)
{
    ConvexHull *hull = map->mHull;
    const IceMaths::Point center = hull->mCentroid, copiedDirection = *direction;
    float best = FLT_MAX;
    NxU32 index = 0xffffffff;
    if (!hull->mNbPolygons)
        nxHullComputePolygons(hull);
    const NxU32 count = hull->mNbPolygons;
    for (NxU32 i = 0; i < count; ++i)
    {
        hull = map->mHull;
        if (!hull->mPolygons)
            nxHullComputePolygons(hull);
        const IceMaths::Plane &plane = hull->mPolygons[i].mPlane;
        const double facing = (double(plane.n.y) * direction->y + double(plane.n.z) * direction->z) +
                              double(direction->x) * plane.n.x;
        if (!(facing >= 0.0))
            continue;
        const double denominator =
            (double(copiedDirection.x) * plane.n.x + double(copiedDirection.z) * plane.n.z) +
            double(copiedDirection.y) * plane.n.y;
        if (denominator > -1e-7 && denominator < 1e-7)
            continue;
        const double distance =
            -(((double(center.z) * plane.n.z + double(center.x) * plane.n.x) + double(center.y) * plane.n.y) +
              plane.d) /
            denominator;
        if (distance < double(best))
        {
            best = float(distance);
            index = i;
        }
    }
    map->mSamples[sample] = NxU8(index);
    return true;
}
IceSupportMap *nxSupportMapVertexConstruct(IceSupportMap *map, const ConvexHull *source)
{
    nxSupportMapBaseConstruct(map);
    map->mSamples = 0;
    map->mSamples2 = 0;
    map->mVertexSource = source;
    map->mVtable = gIceSupportMapVertexTable;
    return map;
}
void nxSupportMapVertexRelease(IceSupportMap *map)
{
    map->mVtable = gIceSupportMapVertexTable;
    if (map->mSamples2)
    {
        nxSupportMapFree(map->mSamples2);
        map->mSamples2 = 0;
    }
    if (map->mSamples)
    {
        nxSupportMapFree(map->mSamples);
        map->mSamples = 0;
    }
    nxSupportMapBaseTable(map);
}
bool nxSupportMapVertexAllocate(IceSupportMap *map)
{
    if (map->mVertexSource->mNbVerts > 255)
        return false;
    map->mSamples = (NxU8 *)nxGetSdkAllocator()->malloc(map->mNbSamples, NX_MEMORY_PERSISTENT);
    if (!map->mSamples)
        return false;
    map->mSamples2 = (NxU8 *)nxGetSdkAllocator()->malloc(map->mNbSamples, NX_MEMORY_PERSISTENT);
    return map->mSamples2 != 0;
}
bool nxSupportMapVertexCompute(IceSupportMap *map, NxU32 sample, const IceMaths::Point *direction)
{
    const ConvexHull *source = map->mVertexSource;
    float minimum = FLT_MAX, negativeMaximum = FLT_MAX;
    NxU32 least = 0, greatest = 0, i = 0;
    const NxU32 count = source->mNbVerts;
    // Original four-way unroll has three different addition orders. Winning
    // minima and negative maxima narrow; comparison uses the unspilled score.
    if (!(count & 0x80000000) && count >= 4)
    {
        do
        {
            for (NxU32 lane = 0; lane < 4; ++lane)
            {
                const IceMaths::Point &p = source->mVerts[i + lane];
                double projection;
                if (lane == 0)
                    projection = (double(p.y) * direction->y + double(p.x) * direction->x) +
                                 double(p.z) * direction->z;
                else if (lane == 1)
                    projection = (double(p.x) * direction->x + double(p.z) * direction->z) +
                                 double(p.y) * direction->y;
                else
                    projection = (double(p.y) * direction->y + double(p.z) * direction->z) +
                                 double(p.x) * direction->x;
                if (projection < double(minimum))
                {
                    minimum = float(projection);
                    least = i + lane;
                }
                const double negative = -projection;
                if (negative < double(negativeMaximum))
                {
                    negativeMaximum = float(negative);
                    greatest = i + lane;
                }
            }
            i += 4;
        } while (i < count - 3);
    }
    for (; i < count; ++i)
    {
        const IceMaths::Point &p = source->mVerts[i];
        const double projection =
            (double(p.x) * direction->x + double(p.z) * direction->z) + double(direction->y) * p.y;
        if (projection < double(minimum))
        {
            minimum = float(projection);
            least = i;
        }
        const double negative = -projection;
        if (negative < double(negativeMaximum))
        {
            negativeMaximum = float(negative);
            greatest = i;
        }
    }
    map->mSamples[sample] = NxU8(least);
    map->mSamples2[sample] = NxU8(greatest);
    return true;
}
void nxSupportMapNoop(IceSupportMap *)
{
}
IceSupportMap *nxSupportMapHullDelete(IceSupportMap *map, NxU32 flags)
{
    map->mVtable = gIceSupportMapHullTable;
    if (map->mSamples)
    {
        nxSupportMapFree(map->mSamples);
        map->mSamples = 0;
    }
    nxSupportMapBaseTable(map);
    if (flags & 1)
        nxSupportMapFree(map);
    return map;
}
IceSupportMap *nxSupportMapPlaneDelete(IceSupportMap *map, NxU32 flags)
{
    map->mVtable = gIceSupportMapPlaneTable;
    if (map->mSamples)
    {
        nxSupportMapFree(map->mSamples);
        map->mSamples = 0;
    }
    nxSupportMapBaseTable(map);
    if (flags & 1)
        nxSupportMapFree(map);
    return map;
}
IceSupportMap *nxSupportMapVertexDelete(IceSupportMap *map, NxU32 flags)
{
    nxSupportMapVertexRelease(map);
    if (flags & 1)
        nxSupportMapFree(map);
    return map;
}
