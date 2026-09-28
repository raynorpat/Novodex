#ifndef NX_PHYSICS_BOX_DISTANCE_H
#define NX_PHYSICS_BOX_DISTANCE_H

// The box distance kernels of sub-unit E (units/convex-mesh-gap-contract.md):
// point/box, line/box and segment/box squared distances, Physics/src/Distance.cpp.
// They are Eberly's (Magic Software) point-, line- and segment-to-box distances
// as NovodeX compiled them; the oracle's own listing is what the source follows.
//
// A box is the 60-byte stack structure the callers build, NxCollisionBoxData
// (NarrowPhase.h): centre, extents, then a 3x3 whose COLUMNS are the box axes
// (the kernels read rotation[0], [3], [6] for axis 0). A line is origin then
// direction, six floats.
//
// Every kernel leaves its result in st(0). Two of them return a value that was
// never narrowed (point/box accumulates in a register), and line/box returns a
// float it loaded back, so the return type is `double` and callers that need
// the register untouched call through a thunk.

#include "NarrowPhase.h"

struct NxDistanceLine
	{
	NxReal origin[3];
	NxReal direction[3];
	};

// phys_fn_001670 at 0x00032840. Squared distance from `point` to the box
// (centre, extents, rotation), and the closest point in BOX coordinates when
// `closest` is not null.
double __cdecl NxPointBoxSquareDistance(const NxReal* point, const NxReal* center,
	const NxReal* extents, const NxReal* rotation, NxReal* closest);

// phys_fn_001684 at 0x00033a50 (continuation phys_fn_001686). Squared distance
// from the infinite line to the box. When `lineParam` is not null it receives
// the line parameter and the three box parameters receive the closest point in
// box coordinates; when it is null none of the four is written.
double __cdecl NxLineBoxSquareDistance(const NxDistanceLine* line,
	const NxCollisionBoxData* box, NxReal* lineParam,
	NxReal* boxParam0, NxReal* boxParam1, NxReal* boxParam2);

// phys_fn_001688 at 0x00033d00. Squared distance from the segment p0..p1 to the
// box, the segment parameter (0..1) and the closest point in box coordinates,
// each written only when its pointer is not null.
double __cdecl NxSegmentBoxSquareDistance(const NxSegment* segment,
	const NxReal* center, const NxReal* extents, const NxReal* rotation,
	NxReal* segmentParam, NxReal* boxPoint);

#endif
