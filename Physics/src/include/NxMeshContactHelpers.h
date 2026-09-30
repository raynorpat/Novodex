#ifndef NX_PHYSICS_MESH_CONTACT_HELPERS_H
#define NX_PHYSICS_MESH_CONTACT_HELPERS_H

// Two leaf helpers the mesh contact entries share, written ahead of their
// callers by convex-mesh gap Task 2b (units/convex-mesh-gap-contract.md):
// the triangle plane of sub-unit I (Physics/src/ContactBoxMeshICE.cpp) and the
// segment/triangle-edge test of sub-unit N (Physics/src/ContactMeshHeightfield.cpp).

#include "Nxp.h"
#include "NxVec3.h"
#include "NxPlane.h"

// Row phys_fn_001760 at 0x0003c160. __thiscall on the plane in the oracle (`ret
// 0xc`); MSVC rejects __thiscall on a free function (C3865), and __fastcall with
// an unused second register argument is the same contract (ecx = plane, three
// stack arguments, callee pops). The plane through p0, p1, p2 with the normal
// (p1 - p0) x (p2 - p0), normalised unless its length is zero. Returns plane.
NxPlane* __fastcall NxTrianglePlane(NxPlane* plane, void* unusedEdx,
	const NxVec3* p0, const NxVec3* p1, const NxVec3* p2);

// Row phys_fn_001855 at 0x00044510. The segment s0..s1 against the plane through the
// edge e0..e1 that contains `axis`: false if the segment is on one side of it or
// parallel to it; otherwise the crossing is written to `hit`, the distance along
// `axis` from the edge's line to it to `t` (in the triangle's dominant plane),
// and `hit` moved back along `axis` by that distance; true if the moved point
// lies between e0 and e1. `t` and `hit` are written on some false returns too.
// The result is 0 or 1 in the whole of eax, as the listing returns it (Task 2i).
NxU32 __cdecl NxSegmentTriangleEdge(const NxReal* e0, const NxReal* e1, const NxReal* axis,
	const NxReal* s0, const NxReal* s1, NxReal* t, NxReal* hit);

#endif
