/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// ContactBoxMeshICE.cpp, sub-unit I of units/convex-mesh-gap-contract.md (the
// name is the oracle's own: 001772's __FILE__, line 1706). Only 001760 is
// written so far (convex-mesh gap Task 2b), ahead of the box/mesh rows (Task
// 2j), because 001844 (Task 2i) and 001762 call it; it is also called by
// 001770, 001779, 001865 and 001929.
//
// x87: on the /arch:IA32 list. A value the listing keeps on the FPU stack is a
// `double`, a value it stores is an `NxReal`, as in Distance.cpp.

#include "NxMeshContactHelpers.h"
#include "X87Sqrt.h"

// phys_fn_001760 (0x0003c160, 222 B)
// NxPlane::set(p0, p1, p2) as the oracle compiled it out of line. The first edge
// stays on the FPU stack; of the second, x and y are stored and z is kept. The
// cross product's x and y are narrowed into a local and copied to the plane,
// its z is stored with `fst` (0x0003c1ce) and its wide value squared for the
// length, so the length is ((z^2 wide + y^2) + x^2) over two narrowed
// components and one wide one. The normalisation is skipped only for a length
// equal to zero (`fucompp; test ah, 0x44; jnp`): a NaN length normalises. x
// and y are scaled from the narrowed copies and z from the wide one. d is
// -((p0.z n.z + p0.x n.x) + n.y p0.y) over the stored normal.
__declspec(noinline) NxPlane* __fastcall NxTrianglePlane(NxPlane* plane, void* /*unusedEdx*/,
	const NxVec3* p0, const NxVec3* p1, const NxVec3* p2)
	{
	const double e0x = (double) p1->x - p0->x;
	const double e0y = (double) p1->y - p0->y;
	const double e0z = (double) p1->z - p0->z;
	const NxReal e1x = (NxReal) ((double) p2->x - p0->x);
	const NxReal e1y = (NxReal) ((double) p2->y - p0->y);
	const double e1z = (double) p2->z - p0->z;

	const NxReal nx = (NxReal) (e1z * e0y - (double) e1y * e0z);
	plane->normal.x = nx;
	const NxReal ny = (NxReal) (e0z * e1x - e1z * e0x);
	plane->normal.y = ny;
	const double nz = e0x * e1y - e0y * e1x;
	plane->normal.z = (NxReal) nz;

	const double length = x87FsqrtDot3(nz, nz, ny, ny, nx, nx);
	if(length != 0.0)
		{
		const double inverse = 1.0f / length;
		plane->normal.x = (NxReal) (nx * inverse);
		plane->normal.y = (NxReal) (ny * inverse);
		plane->normal.z = (NxReal) (nz * inverse);
		}

	plane->d = (NxReal) -(((double) p0->z * plane->normal.z + (double) p0->x * plane->normal.x)
		+ (double) plane->normal.y * p0->y);
	return plane;
	}
