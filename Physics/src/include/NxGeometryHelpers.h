#ifndef NX_PHYSICS_GEOMETRY_HELPERS_H
#define NX_PHYSICS_GEOMETRY_HELPERS_H

// The private helpers of sub-unit F (units/convex-mesh-gap-contract.md),
// written in Physics/src/Geometry.cpp beside the exported ray kernels they use.

#include "Nxp.h"
#include "NxVec3.h"
#include "NxRay.h"

// Row phys_fn_001708 at 0x00036d90. Ray against the triangle fan
// (indices[0], indices[i], indices[i + 1]), each triangle inflated by 0.02;
// true on the first hit, with its ray parameter in *t (direction need not be
// unit length; negative parameters are retained). count2 returns false without
// reading any pointer or writing *t. For count>=3, indices covers count fan
// entries, vertices covers each referenced index, and ray and t are valid;
// a miss leaves *t untouched.
// count<2 retains the historical unchecked unsigned loop and is unsupported.
// No culling, closest-hit search, clamping or constant-border inflation occurs.
bool __cdecl NxRayInflatedTriangleFan(NxU32 count, const NxVec3* vertices, const NxU32* indices,
	const NxRay* ray, NxReal* t);

// Row phys_fn_001730 at 0x00038050 (continuation phys_fn_001732). Slab test of a ray
// against the box (boxMin, boxMax); the entry face (0..2 min planes, 3..5 max
// planes) or -1.
int __cdecl NxRayAABBSlab(const NxReal* boxMin, const NxReal* boxMax, const NxReal* origin,
	const NxReal* dir, NxReal* tNear, NxReal* tFar);

#endif
