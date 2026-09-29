/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// ContactMeshHeightfield.cpp, sub-unit N of units/convex-mesh-gap-contract.md
// (the name is the oracle's own: 001865's __FILE__, line 328). Only 001855 is
// written so far (convex-mesh gap Task 2b), ahead of the mesh/height-field rows
// (Task 2l), because 001762 (Task 2j) and 001844 (Task 2i) call it as well as
// 001859. The .rdata blocks put 001855/001857 in an unnamed unit of their own
// before this file's; placing them here is the contract's choice (open item 5).
//
// x87: on the /arch:IA32 list. A value the listing keeps on the FPU stack is a
// `double`, a value it stores is an `NxReal`, as in Distance.cpp.

#include "NxMeshContactHelpers.h"
#include "X87Sqrt.h"

#include <math.h>

// phys_fn_001855 (0x00044510, 837 B)
// The segment s0..s1 against the plane through the triangle edge e0..e1 that
// contains `axis` (the triangle's normal at the callers): the plane normal is
// (e1 - e0) x axis, formed with x and z in registers and y stored, then
// normalised unless its length is exactly zero (`fucompp; test ah, 0x44; jnp`,
// so a NaN length normalises; x and z scaled wide, y from the stored copy).
// The segment's two signed distances are formed in registers, the start's x
// term narrowed into the caller's second argument slot (0x000445fa); their
// product > 0 is a miss. The segment's direction keeps x in a register and
// stores y and z (z squared as wide * narrowed, 0x0004465d); a zero
// denominator is a miss. The crossing, the line parameter narrowed at
// 0x000446f8, is written to `hit` with x from the wide product. Then, in the
// plane of the normal's two smallest components (the dominant axis is chosen on
// the absolute values, 0x00044738..0x00044798, ties to the later axis), t is
// the distance along `axis` from the edge's line; it is stored before it is
// tested, so a negative t is a miss that still writes it. `hit` is moved back
// by t * axis, x wide, each component stored with `fst` and kept, and the hit
// is on the edge when (e1 - hit) . (e0 - hit) < 0 over those wide values,
// summed z, x, y.
//
// Under the in-step word 0x0f7f this row does not reproduce every last bit in
// C++. What is narrowed are the wide intermediates themselves -- the normal's x
// and z and the direction's x and z, which the listing keeps in st(n) -- at two
// points: as the operands of each square root, which reach the X87Sqrt.h helper
// through qwords (64 bits cut to 53), and in their reuse after the root, where
// MSVC reloads them from the same 8-byte slots to scale them by 1/root (and dirX
// again for the denominator and the crossing). The oracle's `fsqrt` is inline and
// keeps all of them. The differential measures the effect and pins it (215 words
// with mixed-exponent draws); written as leaves that recompute the wide values
// after the call, the count rose, because MSVC folds the recomputation into the
// spilled copy. The remedy X87Sqrt.h records for 001760 -- the span as one x87
// assembly block -- would be two blocks of about 131 instructions here, and is
// not judged proportionate.
//
// The result is 0 or 1 in the whole of eax, as the listing returns it (`xor eax,
// eax` at 0x000446d7 and 0x00044798, `mov eax, 1` at 0x0004477d and 0x0004484b):
// 001844's naked caller tests eax, not al (`test eax, eax` at 0x000430b2; a
// bool left the upper bytes undefined, convex-mesh gap Task 2i).
__declspec(noinline) NxU32 __cdecl NxSegmentTriangleEdge(const NxReal* e0, const NxReal* e1,
	const NxReal* axis, const NxReal* s0, const NxReal* s1, NxReal* t, NxReal* hit)
	{
	NxReal edge[3];
	edge[0] = (NxReal) ((double) e1[0] - e0[0]);
	edge[1] = (NxReal) ((double) e1[1] - e0[1]);
	edge[2] = (NxReal) ((double) e1[2] - e0[2]);

	const double nxWide = (double) edge[1] * axis[2] - (double) edge[2] * axis[1];
	const NxReal ny = (NxReal) ((double) edge[2] * axis[0] - (double) edge[0] * axis[2]);
	const double nzWide = (double) edge[0] * axis[1] - (double) edge[1] * axis[0];
	NxReal normal[3];
	normal[1] = ny;
	normal[0] = (NxReal) nxWide;
	normal[2] = (NxReal) nzWide;
	const double normalLength = x87FsqrtDot3(nzWide, nzWide, ny, ny, nxWide, nxWide);
	if(normalLength != 0.0)
		{
		const double inverse = 1.0f / normalLength;
		normal[0] = (NxReal) (nxWide * inverse);
		normal[1] = (NxReal) (ny * inverse);
		normal[2] = (NxReal) (nzWide * inverse);
		}

	const NxReal d = (NxReal) (((double) normal[0] * e0[0] + (double) normal[2] * e0[2])
		+ (double) normal[1] * e0[1]);
	const NxReal startX = (NxReal) ((double) normal[0] * s0[0]);
	const double distance1 = (((double) normal[2] * s1[2] + (double) normal[1] * s1[1])
		+ (double) normal[0] * s1[0]) - d;
	const double distance0 = (((double) normal[2] * s0[2] + (double) normal[1] * s0[1])
		+ startX) - d;
	if(distance1 * distance0 > 0.0)
		return 0;

	double dirX = (double) s1[0] - s0[0];
	NxReal dirY = (NxReal) ((double) s1[1] - s0[1]);
	const double dirZWide = (double) s1[2] - s0[2];
	NxReal dirZ = (NxReal) dirZWide;
	const double dirLength = x87FsqrtDot3(dirZWide, dirZ, dirY, dirY, dirX, dirX);
	if(dirLength != 0.0)
		{
		const double inverse = 1.0f / dirLength;
		dirX = dirX * inverse;
		dirY = (NxReal) (dirY * inverse);
		dirZ = (NxReal) (dirZ * inverse);
		}
	const double denominator = ((double) dirZ * normal[2] + (double) dirY * normal[1])
		+ dirX * normal[0];
	if(denominator == 0.0)
		return 0;

	const NxReal along = (NxReal) (((double) d - (((double) normal[2] * s0[2]
		+ (double) normal[1] * s0[1]) + startX)) / denominator);
	const double stepX = dirX * along;
	const NxReal stepY = (NxReal) ((double) dirY * along);
	const NxReal stepZ = (NxReal) ((double) dirZ * along);
	hit[0] = (NxReal) (stepX + s0[0]);
	hit[1] = (NxReal) ((double) stepY + s0[1]);
	hit[2] = (NxReal) ((double) stepZ + s0[2]);

	NxReal magnitude[3];
	magnitude[0] = (NxReal) fabs(normal[0]);
	magnitude[1] = (NxReal) fabs(normal[1]);
	magnitude[2] = (NxReal) fabs(normal[2]);
	const int major = magnitude[0] > magnitude[1] ? 0 : 1;
	int i0;
	int i1;
	if(magnitude[major] < magnitude[2])
		{
		i0 = 0;
		i1 = 1;
		}
	else if(major == 0)
		{
		i0 = 1;
		i1 = 2;
		}
	else
		{
		i0 = 0;
		i1 = 2;
		}

	const double q = (((double) hit[i1] - e0[i1]) * edge[i0] - ((double) hit[i0] - e0[i0]) * edge[i1])
		/ ((double) axis[i1] * edge[i0] - (double) axis[i0] * edge[i1]);
	*t = (NxReal) q;
	if(q < 0.0)
		return 0;

	const double backX = q * axis[0];
	const NxReal backY = (NxReal) (q * axis[1]);
	const NxReal backZ = (NxReal) (q * axis[2]);
	const double px = (double) hit[0] - backX;
	hit[0] = (NxReal) px;
	const double py = (double) hit[1] - backY;
	hit[1] = (NxReal) py;
	const double pz = (double) hit[2] - backZ;
	hit[2] = (NxReal) pz;

	const double inside = (((double) e1[2] - pz) * ((double) e0[2] - pz)
		+ ((double) e0[0] - px) * ((double) e1[0] - px))
		+ ((double) e1[1] - py) * ((double) e0[1] - py);
	return inside < 0.0 ? 1u : 0u;
	}
