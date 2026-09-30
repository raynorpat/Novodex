#ifndef NX_PHYSICS_TRIANGLE_DISTANCE_H
#define NX_PHYSICS_TRIANGLE_DISTANCE_H

// The triangle distance kernels of sub-unit E (units/convex-mesh-gap-contract.md),
// Physics/src/Distance.cpp: point/triangle, line/line closest points and
// segment/triangle. Eberly's (Magic Software) DistVec3Tri3 and DistSeg3Tri3 as
// NovodeX compiled them; the oracle's own listing is what the source follows.
//
// A triangle is three vertex pointers (origin v0, edges v1 - v0 and v2 - v0);
// the two triangle parameters are the weights of those edges. Every kernel
// that returns a squared distance leaves it in st(0) unnarrowed, so the return
// type is `double`.

#include "NarrowPhase.h"

// Row phys_fn_001672 at 0x000329e0. Squared distance from `point` to the triangle,
// with the two edge parameters written when their pointers are not null.
double __cdecl NxPointTriangleSquareDistance(const NxReal* point,
	const NxReal* v0, const NxReal* v1, const NxReal* v2, NxReal* sParam, NxReal* tParam);

// Row phys_fn_001692 at 0x000345b0. The closest points of the lines
// origin0 + s dir0 and origin1 + t dir1, with s and t clamped to [0, 1];
// always writes both points.
void __cdecl NxLineLineClosestPoints(NxReal* point0, NxReal* point1,
	const NxReal* origin0, const NxReal* dir0, const NxReal* origin1, const NxReal* dir1);

// Row phys_fn_001694 at 0x00034860. Squared distance from the segment to the
// triangle, with the segment parameter and the two triangle parameters written
// when their pointers are not null.
double __cdecl NxSegmentTriangleSquareDistance(const NxSegment* segment,
	const NxReal* v0, const NxReal* v1, const NxReal* v2,
	NxReal* segmentParam, NxReal* sParam, NxReal* tParam);

#endif
