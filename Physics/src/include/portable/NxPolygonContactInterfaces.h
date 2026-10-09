#ifndef NX_POLYGON_CONTACT_INTERFACES_H
#define NX_POLYGON_CONTACT_INTERFACES_H
#include "NarrowPhase.h"
#include "NxPlane.h"
struct NxContactSink;
// 001903/001905: eax=count, ecx=vertices; stack x,y, caller cleans8.
NxU32 nxPolygonContainsPoint(NxU32 count, const NxVec3* vertices, float x, float y);
// 001907: edx=edge0 start, ecx=plane, esi=point output, ebx=edge1Direction;
// five caller-cleaned stack arguments: edge1 start/end, polygon0 normal,
// edge0 end, parameter output. Outputs may be written before returning0.
NxU32 nxClipEdgeToPolygonPlane(const NxVec3* start0, const float* plane,
    NxVec3* point, const NxVec3* edge1Direction, const NxVec3* start1,
    const NxVec3* end1, const NxVec3* normal0, const NxVec3* end0, float* parameter);
// 001909/001911: cdecl22; EBP=entryESP-0x24, arg1 atEBP+28.
// Poses are column-major4x4; planes have normal plus signed d. Arrays are borrowed.
// Words17/18 are passed by callers but never read by this complete body.
void NxConvexPolygonContacts(NxU32 count0, const NxVec3* vertices0, const NxU32* refs0,
    const float* pose0, const NxPlane* plane0, NxU32 count1,
    const NxVec3* vertices1, const NxU32* refs1, const float* pose1,
    const NxPlane* plane1, const NxVec3* displacement, const float* relative01,
    const float* relative10, const NxCollisionShape* shape0,
    const NxCollisionShape* shape1, NxContactSink* sink, NxU32 unread0,
    NxU32 unread1, NxU32 featureId0, NxU32 featureId1, NxU32 featureWord0, NxU32 featureWord1);
#endif
