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
// `double`, a value it stores is an `NxReal`, as in Distance.cpp -- except
// 001760's normal, which is x87 assembly (see there).

#include "NxMeshContactHelpers.h"

// 0x101041ec and 0x101041f0, the two dword constants the normalisation reads.
static const float gPlaneOne = 1.0f;
static const float gPlaneZero = 0.0f;

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
//
// The normal and its normalisation (0x0003c163..0x0003c21b) are written as one
// x87 assembly block, transcribing the listing instruction for instruction:
// the per-site precedent recorded in X87Sqrt.h. In C++ the row's wide values --
// the cross product's z and the length -- cannot survive to the scaling under
// the in-step word 0x0f7f: MSVC keeps z in an 8-byte slot, and the root is a
// call whose operands and result travel through qwords, so both are cut from 64
// to 53 bits before they are reused (51 differing words in the first
// differential, 0 with this block). d (0x0003c21d..0x0003c235) reads only the
// stored normal and stays C++.
__declspec(noinline) NxPlane* __fastcall NxTrianglePlane(NxPlane* plane, void* /*unusedEdx*/,
	const NxVec3* p0, const NxVec3* p1, const NxVec3* p2)
	{
	NxReal nx;		// [esp] in the listing
	NxReal ny;		// [esp+4]
	NxReal e1x;		// [esp+0xc]
	NxReal e1y;		// [esp+0x10]
	__asm
		{
		mov		ecx, plane
		mov		eax, p1
		mov		edx, p0
		fld		dword ptr [eax]
		fsub	dword ptr [edx]
		fld		dword ptr [eax + 4]
		fsub	dword ptr [edx + 4]
		fld		dword ptr [eax + 8]
		mov		eax, p2
		fsub	dword ptr [edx + 8]
		fld		dword ptr [eax]
		fsub	dword ptr [edx]
		fstp	e1x
		fld		dword ptr [eax + 4]
		fsub	dword ptr [edx + 4]
		fstp	e1y
		fld		dword ptr [eax + 8]
		fsub	dword ptr [edx + 8]
		fld		st(0)
		fmul	st, st(3)
		fld		e1y
		fmul	st, st(3)
		fsubp	st(1), st
		fstp	nx
		mov		eax, nx
		fxch	st(1)
		mov		dword ptr [ecx], eax
		fmul	e1x
		fxch	st(1)
		fmul	st, st(3)
		fsubp	st(1), st
		fstp	ny
		mov		eax, ny
		fld		e1y
		mov		dword ptr [ecx + 4], eax
		fmulp	st(2), st
		fmul	e1x
		fsubp	st(1), st
		fst		dword ptr [ecx + 8]
		fld		st(0)
		fmul	st, st(1)
		fld		ny
		fmul	ny
		faddp	st(1), st
		fld		nx
		fmul	nx
		faddp	st(1), st
		fsqrt
		fld		gPlaneZero
		fld		st(1)
		fucompp
		fnstsw	ax
		test	ah, 0x44
		jnp		zeroLength
		fdivr	gPlaneOne
		fld		nx
		fmul	st, st(1)
		fstp	dword ptr [ecx]
		fld		ny
		fmul	st, st(1)
		fstp	dword ptr [ecx + 4]
		fxch	st(1)
		fmul	st, st(1)
		fstp	dword ptr [ecx + 8]
		jmp		scaled
zeroLength:
		fstp	st(0)
scaled:
		fstp	st(0)
		}

	plane->d = (NxReal) -(((double) p0->z * plane->normal.z + (double) p0->x * plane->normal.x)
		+ (double) plane->normal.y * p0->y);
	return plane;
	}
