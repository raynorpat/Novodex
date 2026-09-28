/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Sub-unit E of units/convex-mesh-gap-contract.md, the box half: the point/box,
// line/box and segment/box squared distances (convex-mesh gap Task 2a). The
// triangle half (001672, 001692, 001694) follows them (Task 2b).
//
// These are Eberly's (Magic Software) DistVec3Box3, DistLin3Box3 and
// DistSeg3Box3 as NovodeX built them, and that correspondence is only a map:
// the listing is what is transcribed, and it differs from the published source
// in three kinds of place.
//
//  * Association. The oracle was built with the sums re-associated, so
//    `a + b + c + d` in the published source is a different tree in nearly
//    every leaf here, and different leaves of the same expression use
//    different trees (Face's four copies of the far-edge squared distance are
//    four orders). Each sum below is written in the tree its own listing
//    forms, with the listing address.
//  * Quotients. `-fDelta / fLSqr` is `fld -1.0; fdivr; fmul delta`, a
//    reciprocal and a product, everywhere in Face. The reciprocal is kept.
//  * One published typo reproduced: Case00's third clamp writes the low bound
//    of axis i2 into pnt[i1] (0x0003393a `fstp [edx+esi*4]`), as the Magic
//    Software source it came from does.
//
// x87: the file is on the /arch:IA32 list. A value the listing keeps on the
// FPU stack is a `double` here and a value it stores (`fstp dword`) is an
// `NxReal`; a product of two floats is exact in a double, so only sums,
// differences and quotients are affected by the choice, and under the CRT
// word 0x027f a register is a double. Under the step's 0x0f7f a `double` MSVC
// spills is cut to 53 bits where the oracle keeps 64: the register-lifetime
// class recorded for phys_fn_001690. It showed here once, in Face written as
// one function (16 words of segment_box under 0x0f7f, edges grazed so the
// squared distance cancels to ~1e-14); Face's leaves are therefore small
// functions of their own (see there), and the differential
// (tests/PhysicsCollisionTests.cpp) now measures no difference under either
// word.
//
// The five line/box helpers are register-convention functions in the oracle
// (0x00032f10, 0x000335f0, 0x000336c0, 0x000338a0, 0x00033970: indices and
// pointers in edi/esi/ebx/ecx/edx, the rest on the stack). No C++ can name
// those registers; the helpers here take the same values as ordinary
// parameters, and are kept out of line (`noinline`) so each row keeps an
// address of its own for the trace.

#include "NxBoxDistance.h"
#include "NxTriangleDistance.h"

#include <math.h>

#include <string.h>

// ---------------------------------------------------------------------------

// phys_fn_001670 (0x00032840, 403 B)
// Point to box. The offset from the centre stays on the FPU stack
// (0x0003284b..0x0003285c) and the three box-frame coordinates are stored,
// each as ((z term + y term) + x term). The squared distance is a register
// throughout: the x arm REPLACES the 0.0 it loaded at 0x000328e2 with delta^2
// (0x00032902 `fstp st(0)` then 0x00032928 `fmulp`) rather than adding to it,
// and y and z add with `faddp st(2)`. It is returned unnarrowed. The x test
// compares against -extent narrowed into the caller's first argument slot
// (0x000328ec); the negation is exact, so that store changes nothing but is a
// write past this row's frame. z's clamped value is kept in a register and
// stored last (0x000329bd).
__declspec(noinline) double __cdecl NxPointBoxSquareDistance(const NxReal* point, const NxReal* center,
	const NxReal* extents, const NxReal* rotation, NxReal* closest)
	{
	const double dx = (double) point[0] - center[0];
	const double dy = (double) point[1] - center[1];
	const double dz = (double) point[2] - center[2];

	NxReal local0 = (NxReal) (((double) rotation[6] * dz + (double) rotation[3] * dy) + dx * rotation[0]);
	NxReal local1 = (NxReal) (((double) rotation[7] * dz + (double) rotation[4] * dy) + (double) rotation[1] * dx);
	const NxReal local2 = (NxReal) (((double) rotation[8] * dz + (double) rotation[5] * dy) + (double) rotation[2] * dx);

	double squared;
	if(local0 < -extents[0])
		{
		const double delta = (double) local0 + extents[0];
		local0 = -extents[0];
		squared = delta * delta;
		}
	else if(local0 > extents[0])
		{
		const double delta = (double) local0 - extents[0];
		local0 = extents[0];
		squared = delta * delta;
		}
	else
		squared = 0.0;

	if(local1 < -extents[1])
		{
		const double delta = (double) local1 + extents[1];
		squared = squared + delta * delta;
		local1 = -extents[1];
		}
	else if(local1 > extents[1])
		{
		const double delta = (double) local1 - extents[1];
		local1 = extents[1];
		squared = squared + delta * delta;
		}

	NxReal clamped2 = local2;
	if(local2 < -extents[2])
		{
		const double delta = (double) local2 + extents[2];
		squared = squared + delta * delta;
		clamped2 = -extents[2];
		}
	else if(local2 > extents[2])
		{
		const double delta = (double) local2 - extents[2];
		squared = squared + delta * delta;
		clamped2 = extents[2];
		}

	if(closest)
		{
		closest[2] = clamped2;
		closest[0] = local0;
		closest[1] = local1;
		}
	return squared;
	}

// ---------------------------------------------------------------------------
// The line/box helpers. `pnt` and `dir` are the line's origin and direction in
// the box frame after the reflections, `sqr` is the running squared distance,
// which lives in memory in the oracle (the caller's first argument slot,
// [esp+0x44] of 0x00033a50) so every addition to it is `fadd dword; fstp
// dword`.

// Adds one clamp axis's contribution. The three copies of this block in the
// oracle (Case0's i2, Case00's i1, Case000's three) compute `pnt + e` and
// `pnt - e` from the loaded coordinate and square it in a register.
static inline void nxClampAxis(NxReal* pnt, const NxReal* extents, int i, NxReal* sqr)
	{
	if(pnt[i] < -extents[i])
		{
		const double delta = (double) pnt[i] + extents[i];
		*sqr = (NxReal) (delta * delta + *sqr);
		pnt[i] = -extents[i];
		}
	else if(pnt[i] > extents[i])
		{
		const double delta = (double) pnt[i] - extents[i];
		*sqr = (NxReal) (delta * delta + *sqr);
		pnt[i] = extents[i];
		}
	}

// Face's leaves. The oracle's Face is one function of 1,751 bytes whose FPU
// stack holds every intermediate of the leaf it is in (tmp, t, tmp2, delta and
// the reciprocal) without a spill. Written as one C++ function, MSVC spills
// several of those `double`s to 8-byte slots across the branches, which is
// invisible under 0x027f and cuts 64-bit registers to 53 bits under 0x0f7f --
// and here, where the edge and corner leaves end in a sum that cancels to
// almost nothing when the line grazes an edge, that moved the last bits of the
// result. So each leaf is its own small function, entered only with values
// the listing has already narrowed (floats) and forming its wide values
// locally; the one wide value a leaf is entered with in the oracle, the
// single-slab arms' `tmp` (still in st(0) at 0x00033004 and 0x000330f1), is
// recomputed inside the arm's function from the same float operands, in the
// same order, rather than passed. None of this changes what is computed.
//
// The near-edge leaves are written once for two entries each in the oracle:
// 0x0003328d is jumped to from both the single-slab arm (wide tmp) and the
// corner arm (tmp reloaded narrowed from [esp+4], 0x00033289), and 0x0003341c
// repeats 0x000330f7 with the same trees (its squared distance forms
// (P1^2 + M0^2) where the other forms (M0^2 + P1^2), the same sum). The
// far-edge leaves' squared-distance trees differ between the two copies, so
// they are written out at each site.

// Near edge i1 (0x0003328d..0x00033324).
static __forceinline void nxFaceNearEdge1(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	double tmp, NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	const double t = tmp / lSqr;
	const double tmp2 = (double) ppE[i1] - t;
	const double delta = ((double) ppE[i2] * dir[i2] + tmp2 * dir[i1]) + (double) dir[i0] * pmE[i0];
	const NxReal param = (NxReal) ((-1.0f / ((double) dir[i1] * dir[i1] + lSqr)) * delta);
	*sqr = (NxReal) (((((double) pmE[i0] * pmE[i0] + (double) ppE[i2] * ppE[i2])
		+ (double) param * delta) + tmp2 * tmp2) + *sqr);
	if(lineParam)
		{
		*lineParam = param;
		pnt[i0] = extents[i0];
		pnt[i1] = (NxReal) (t - extents[i1]);
		pnt[i2] = -extents[i2];
		}
	}

// Near edge i2 (0x000330f7..0x00033190, 0x0003341c..0x000334b9).
static __forceinline void nxFaceNearEdge2(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	double tmp, NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	const double t = tmp / lSqr;
	const double tmp2 = (double) ppE[i2] - t;
	const double delta = ((double) dir[i1] * ppE[i1] + tmp2 * dir[i2]) + (double) dir[i0] * pmE[i0];
	const NxReal param = (NxReal) ((-1.0f / ((double) dir[i2] * dir[i2] + lSqr)) * delta);
	*sqr = (NxReal) (((((double) pmE[i0] * pmE[i0] + (double) ppE[i1] * ppE[i1])
		+ (double) param * delta) + tmp2 * tmp2) + *sqr);
	if(lineParam)
		{
		*lineParam = param;
		pnt[i0] = extents[i0];
		pnt[i1] = -extents[i1];
		pnt[i2] = (NxReal) (t - extents[i2]);
		}
	}

// The corner arms' near-edge entries: tmp arrives narrowed, so a float
// parameter carries it exactly.
static __declspec(noinline) void nxFaceNearEdge1Narrow(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal tmp, NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	nxFaceNearEdge1(i0, i1, i2, pnt, dir, extents, pmE, ppE, tmp, lSqr, lineParam, sqr);
	}

static __declspec(noinline) void nxFaceNearEdge2Narrow(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal tmp, NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	nxFaceNearEdge2(i0, i1, i2, pnt, dir, extents, pmE, ppE, tmp, lSqr, lineParam, sqr);
	}

// 0x0003300a: past edge i1, single slab.
static __declspec(noinline) void nxFaceFarEdge1(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	const double delta = ((double) ppE[i2] * dir[i2] + (double) dir[i0] * pmE[i0])
		+ (double) pmE[i1] * dir[i1];
	const NxReal param = (NxReal) ((-1.0f / ((double) dir[i1] * dir[i1] + lSqr)) * delta);
	*sqr = (NxReal) (((((double) pmE[i1] * pmE[i1] + (double) pmE[i0] * pmE[i0])
		+ (double) ppE[i2] * ppE[i2]) + (double) param * delta) + *sqr);
	if(lineParam)
		{
		*lineParam = param;
		pnt[i0] = extents[i0];
		pnt[i1] = extents[i1];
		pnt[i2] = -extents[i2];
		}
	}

// 0x00033191: past edge i2, single slab.
static __declspec(noinline) void nxFaceFarEdge2(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	const double delta = ((double) dir[i1] * ppE[i1] + (double) dir[i0] * pmE[i0])
		+ (double) dir[i2] * pmE[i2];
	const NxReal param = (NxReal) ((-1.0f / ((double) dir[i2] * dir[i2] + lSqr)) * delta);
	*sqr = (NxReal) (((((double) pmE[i2] * pmE[i2] + (double) pmE[i0] * pmE[i0])
		+ (double) ppE[i1] * ppE[i1]) + (double) param * delta) + *sqr);
	if(lineParam)
		{
		*lineParam = param;
		pnt[i0] = extents[i0];
		pnt[i1] = -extents[i1];
		pnt[i2] = extents[i2];
		}
	}

// 0x00033325: past edge i1, under the corner test.
static __declspec(noinline) void nxFaceCornerFarEdge1(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	const double delta = ((double) ppE[i2] * dir[i2] + (double) dir[i0] * pmE[i0])
		+ (double) pmE[i1] * dir[i1];
	const NxReal param = (NxReal) ((-1.0f / ((double) dir[i1] * dir[i1] + lSqr)) * delta);
	*sqr = (NxReal) (((((double) ppE[i2] * ppE[i2] + (double) pmE[i1] * pmE[i1])
		+ (double) pmE[i0] * pmE[i0]) + (double) param * delta) + *sqr);
	if(lineParam)
		{
		*lineParam = param;
		pnt[i0] = extents[i0];
		pnt[i1] = extents[i1];
		pnt[i2] = -extents[i2];
		}
	}

// 0x000334ba: past edge i2, under the corner test.
static __declspec(noinline) void nxFaceCornerFarEdge2(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	const double delta = ((double) dir[i1] * ppE[i1] + (double) dir[i0] * pmE[i0])
		+ (double) dir[i2] * pmE[i2];
	const NxReal param = (NxReal) ((-1.0f / ((double) dir[i2] * dir[i2] + lSqr)) * delta);
	*sqr = (NxReal) (((((double) ppE[i1] * ppE[i1] + (double) pmE[i2] * pmE[i2])
		+ (double) pmE[i0] * pmE[i0]) + (double) param * delta) + *sqr);
	if(lineParam)
		{
		*lineParam = param;
		pnt[i0] = extents[i0];
		pnt[i1] = -extents[i1];
		pnt[i2] = extents[i2];
		}
	}

// 0x0003354e: the (v[i1], v[i2]) corner, over the second lSqr.
static __declspec(noinline) void nxFaceCorner(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal lSqr, NxReal* lineParam, NxReal* sqr)
	{
	const double delta = ((double) ppE[i2] * dir[i2] + (double) dir[i1] * ppE[i1])
		+ (double) dir[i0] * pmE[i0];
	const NxReal param = (NxReal) ((-1.0f / ((double) dir[i2] * dir[i2] + lSqr)) * delta);
	*sqr = (NxReal) (((((double) ppE[i1] * ppE[i1] + (double) ppE[i2] * ppE[i2])
		+ (double) pmE[i0] * pmE[i0]) + (double) param * delta) + *sqr);
	if(lineParam)
		{
		*lineParam = param;
		pnt[i0] = extents[i0];
		pnt[i1] = -extents[i1];
		pnt[i2] = -extents[i2];
		}
	}

// 0x00032fba: v[i1] >= -e[i1], v[i2] < -e[i2]. tmp stays in st(0) through the
// compare and into the near-edge leaf (0x00033004 `jnp 0x1003328d`).
static __declspec(noinline) void nxFaceSlab1(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal* lineParam, NxReal* sqr)
	{
	const NxReal lSqr = (NxReal) ((double) dir[i2] * dir[i2] + (double) dir[i0] * dir[i0]);
	const double tmp = (double) lSqr * ppE[i1]
		- ((double) ppE[i2] * dir[i2] + (double) dir[i0] * pmE[i0]) * dir[i1];
	const double twice = (double) lSqr * extents[i1];
	if(tmp <= twice + twice)
		nxFaceNearEdge1(i0, i1, i2, pnt, dir, extents, pmE, ppE, tmp, lSqr, lineParam, sqr);
	else
		nxFaceFarEdge1(i0, i1, i2, pnt, dir, extents, pmE, ppE, lSqr, lineParam, sqr);
	}

// 0x000330aa: v[i1] < -e[i1], v[i2] >= -e[i2] (0x000330f1 `jp`).
static __declspec(noinline) void nxFaceSlab2(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal* lineParam, NxReal* sqr)
	{
	const NxReal lSqr = (NxReal) ((double) dir[i1] * dir[i1] + (double) dir[i0] * dir[i0]);
	const double tmp = (double) ppE[i2] * lSqr
		- ((double) dir[i1] * ppE[i1] + (double) dir[i0] * pmE[i0]) * dir[i2];
	const double twice = (double) lSqr * extents[i2];
	if(tmp <= twice + twice)
		nxFaceNearEdge2(i0, i1, i2, pnt, dir, extents, pmE, ppE, tmp, lSqr, lineParam, sqr);
	else
		nxFaceFarEdge2(i0, i1, i2, pnt, dir, extents, pmE, ppE, lSqr, lineParam, sqr);
	}

// 0x00033227: v[i1] < -e[i1], v[i2] < -e[i2]. Each tmp is stored narrowed
// (0x00033258, 0x000333eb `fst [esp+4]`), tested against zero wide, and every
// later read of it is the narrowed copy.
static __declspec(noinline) void nxFaceCornerArm(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, const NxReal* ppE,
	NxReal* lineParam, NxReal* sqr)
	{
		{
		const NxReal lSqr = (NxReal) ((double) dir[i2] * dir[i2] + (double) dir[i0] * dir[i0]);
		const double tmp = (double) lSqr * ppE[i1]
			- ((double) ppE[i2] * dir[i2] + (double) dir[i0] * pmE[i0]) * dir[i1];
		const NxReal tmpN = (NxReal) tmp;
		if(tmp >= 0.0)
			{
			const double twice = (double) lSqr * extents[i1];
			if(twice + twice >= tmpN)
				nxFaceNearEdge1Narrow(i0, i1, i2, pnt, dir, extents, pmE, ppE, tmpN, lSqr, lineParam, sqr);
			else
				nxFaceCornerFarEdge1(i0, i1, i2, pnt, dir, extents, pmE, ppE, lSqr, lineParam, sqr);
			return;
			}
		}

	// 0x000333b7.
	const NxReal lSqr = (NxReal) ((double) dir[i1] * dir[i1] + (double) dir[i0] * dir[i0]);
	const double tmp = (double) ppE[i2] * lSqr
		- ((double) dir[i1] * ppE[i1] + (double) dir[i0] * pmE[i0]) * dir[i2];
	const NxReal tmpN = (NxReal) tmp;
	if(tmp >= 0.0)
		{
		const double twice = (double) lSqr * extents[i2];
		if(twice + twice >= tmpN)
			nxFaceNearEdge2Narrow(i0, i1, i2, pnt, dir, extents, pmE, ppE, tmpN, lSqr, lineParam, sqr);
		else
			nxFaceCornerFarEdge2(i0, i1, i2, pnt, dir, extents, pmE, ppE, lSqr, lineParam, sqr);
		return;
		}
	nxFaceCorner(i0, i1, i2, pnt, dir, extents, pmE, ppE, lSqr, lineParam, sqr);
	}

// phys_fn_001674 (0x00032f10, 1751 B)
// Face: the line enters the box region through the face x[i0] = e[i0]. The
// two ppE words are stored (0x00032f27, 0x00032f36; ppE[i2] a second time into
// the caller's first argument slot at 0x00032f32, which is where every later
// read of it comes from), lSqr is stored over the second argument slot, and
// the four products of each opening test are compared unrounded. The arms
// are the functions above.
static __declspec(noinline) void nxLineBoxFace(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, const NxReal* pmE, NxReal* lineParam,
	NxReal* sqr)
	{
	NxReal ppE[3];
	ppE[i1] = (NxReal) ((double) extents[i1] + pnt[i1]);
	ppE[i2] = (NxReal) ((double) extents[i2] + pnt[i2]);

	if((double) dir[i0] * ppE[i1] >= (double) dir[i1] * pmE[i0])
		{
		if((double) dir[i0] * ppE[i2] >= (double) dir[i2] * pmE[i0])
			{
			// 0x00032f6a: the line meets the face, distance 0.
			if(lineParam)
				{
				pnt[i0] = extents[i0];
				const double inverse = 1.0f / (double) dir[i0];
				pnt[i1] = (NxReal) ((double) pnt[i1] - ((double) dir[i1] * pmE[i0]) * inverse);
				pnt[i2] = (NxReal) ((double) pnt[i2] - ((double) dir[i2] * pmE[i0]) * inverse);
				*lineParam = (NxReal) -(inverse * pmE[i0]);
				}
			return;
			}
		nxFaceSlab1(i0, i1, i2, pnt, dir, extents, pmE, ppE, lineParam, sqr);
		return;
		}

	if((double) dir[i0] * ppE[i2] >= (double) dir[i2] * pmE[i0])
		nxFaceSlab2(i0, i1, i2, pnt, dir, extents, pmE, ppE, lineParam, sqr);
	else
		nxFaceCornerArm(i0, i1, i2, pnt, dir, extents, pmE, ppE, lineParam, sqr);
	}

// phys_fn_001676 (0x000335f0, 194 B)
// CaseNoZeros: all three direction components positive. pmE is stored; each
// test compares two unrounded products (0x00033631, 0x00033647, 0x00033679).
static __declspec(noinline) void nxLineBoxCaseNoZeros(NxReal* pnt, const NxReal* dir,
	const NxReal* extents, NxReal* lineParam, NxReal* sqr)
	{
	NxReal pmE[3];
	pmE[0] = (NxReal) ((double) pnt[0] - extents[0]);
	pmE[1] = (NxReal) ((double) pnt[1] - extents[1]);
	pmE[2] = (NxReal) ((double) pnt[2] - extents[2]);

	if((double) pmE[1] * dir[0] <= (double) pmE[0] * dir[1])
		{
		if((double) pmE[2] * dir[0] <= (double) pmE[0] * dir[2])
			nxLineBoxFace(0, 1, 2, pnt, dir, extents, pmE, lineParam, sqr);
		else
			nxLineBoxFace(2, 0, 1, pnt, dir, extents, pmE, lineParam, sqr);
		}
	else
		{
		if((double) pmE[2] * dir[1] <= (double) pmE[1] * dir[2])
			nxLineBoxFace(1, 2, 0, pnt, dir, extents, pmE, lineParam, sqr);
		else
			nxLineBoxFace(2, 0, 1, pnt, dir, extents, pmE, lineParam, sqr);
		}
	}

// phys_fn_001678 (0x000336c0, 471 B)
// Case0: dir[i2] is zero. pmE0, pmE1 and both products are stored before the
// test; ppE is stored with `fst` and used wide in delta, which is itself
// stored with `fst` and tested wide, and every later read of either is the
// narrowed copy. The two delta >= 0 arms share their parameter tail
// (0x000337fe), which reads [esp+8] and [esp+0x10]: pmE0 and ppE1 in the first
// arm, ppE0 (written over pmE0) and pmE1 in the second.
static __declspec(noinline) void nxLineBoxCase0(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, NxReal* lineParam, NxReal* sqr)
	{
	NxReal slot8 = (NxReal) ((double) pnt[i0] - extents[i0]);		// pmE0
	NxReal slot10 = (NxReal) ((double) pnt[i1] - extents[i1]);		// pmE1
	const NxReal prod0 = (NxReal) ((double) slot8 * dir[i1]);
	const NxReal prod1 = (NxReal) ((double) slot10 * dir[i0]);

	if(prod0 >= prod1)
		{
		pnt[i0] = extents[i0];
		const double ppE1 = (double) pnt[i1] + extents[i1];
		slot10 = (NxReal) ppE1;
		const double delta = (double) prod0 - ppE1 * dir[i0];
		const NxReal deltaN = (NxReal) delta;
		if(delta >= 0.0)
			{
			const double inverse = 1.0f / ((double) dir[i1] * dir[i1] + (double) dir[i0] * dir[i0]);
			*sqr = (NxReal) (((double) deltaN * inverse) * deltaN + *sqr);
			if(lineParam)
				{
				pnt[i1] = -extents[i1];
				*lineParam = (NxReal) -(((double) slot8 * dir[i0] + (double) slot10 * dir[i1]) * inverse);
				}
			}
		else if(lineParam)
			{
			const double inverse = 1.0f / (double) dir[i0];
			pnt[i1] = (NxReal) ((double) pnt[i1] - (double) prod0 * inverse);
			*lineParam = (NxReal) -(inverse * slot8);
			}
		}
	else
		{
		pnt[i1] = extents[i1];
		const double ppE0 = (double) pnt[i0] + extents[i0];
		slot8 = (NxReal) ppE0;
		const double delta = (double) prod1 - ppE0 * dir[i1];
		const NxReal deltaN = (NxReal) delta;
		if(delta >= 0.0)
			{
			const double inverse = 1.0f / ((double) dir[i1] * dir[i1] + (double) dir[i0] * dir[i0]);
			*sqr = (NxReal) (((double) deltaN * inverse) * deltaN + *sqr);
			if(lineParam)
				{
				pnt[i0] = -extents[i0];
				*lineParam = (NxReal) -(((double) slot8 * dir[i0] + (double) slot10 * dir[i1]) * inverse);
				}
			}
		else if(lineParam)
			{
			const double inverse = 1.0f / (double) dir[i1];
			pnt[i0] = (NxReal) ((double) pnt[i0] - (double) prod1 * inverse);
			*lineParam = (NxReal) -(inverse * slot10);
			}
		}

	nxClampAxis(pnt, extents, i2, sqr);
	}

// phys_fn_001680 (0x000338a0, 195 B)
// Case00: two direction components are zero. The parameter is one quotient,
// stored (0x000338c4). The low arm of the i2 clamp stores -e[i2] into
// pnt[i1] (0x0003393a `fstp [edx+esi*4]`, esi = i1), leaving pnt[i2]
// unclamped: the Magic Software typo, compiled in. Its high arm is correct.
static __declspec(noinline) void nxLineBoxCase00(int i0, int i1, int i2, NxReal* pnt,
	const NxReal* dir, const NxReal* extents, NxReal* lineParam, NxReal* sqr)
	{
	if(lineParam)
		*lineParam = (NxReal) (((double) extents[i0] - pnt[i0]) / dir[i0]);
	pnt[i0] = extents[i0];

	nxClampAxis(pnt, extents, i1, sqr);

	if(pnt[i2] < -extents[i2])
		{
		const double delta = (double) pnt[i2] + extents[i2];
		*sqr = (NxReal) (delta * delta + *sqr);
		pnt[i1] = -extents[i2];
		}
	else if(pnt[i2] > extents[i2])
		{
		const double delta = (double) pnt[i2] - extents[i2];
		*sqr = (NxReal) (delta * delta + *sqr);
		pnt[i2] = extents[i2];
		}
	}

// phys_fn_001682 (0x00033970, 214 B)
// Case000: a zero direction, a point: the three clamps.
static __declspec(noinline) void nxLineBoxCase000(NxReal* pnt, const NxReal* extents, NxReal* sqr)
	{
	nxClampAxis(pnt, extents, 0, sqr);
	nxClampAxis(pnt, extents, 1, sqr);
	nxClampAxis(pnt, extents, 2, sqr);
	}

// phys_fn_001684 (0x00033a50, 237 B)
// With its continuation phys_fn_001686 (0x00033b40, 445 B): the reflection
// loop, the dispatch on the signs and the parameter write-back.
// The box frame: column 0 of the rotation is loaded onto the FPU stack
// (0x00033a5e..0x00033a6b) and the other six words are copied to the frame,
// the offset from the centre is three registers, and every coordinate is
// ((z term + y term) + x term). The squared distance is a float in the
// caller's first argument slot (0x00033b7a) and the reflection flags are three
// bytes in its second (0x00033b55), both writes past this row's frame. The
// write-back is guarded by `lineParam` (0x00033cb2 `test ebp,ebp`), not by
// the box parameters, and the result is the float loaded back (0x00033cf1).
__declspec(noinline) double __cdecl NxLineBoxSquareDistance(const NxDistanceLine* line,
	const NxCollisionBoxData* box, NxReal* lineParam,
	NxReal* boxParam0, NxReal* boxParam1, NxReal* boxParam2)
	{
	const NxReal* rotation = box->rotation;
	const NxReal* direction = line->direction;

	const double dx = (double) line->origin[0] - box->center[0];
	const double dy = (double) line->origin[1] - box->center[1];
	const double dz = (double) line->origin[2] - box->center[2];

	NxReal pnt[3];
	NxReal dir[3];
	pnt[0] = (NxReal) ((dz * rotation[6] + dy * rotation[3]) + dx * rotation[0]);
	pnt[1] = (NxReal) (((double) rotation[7] * dz + dy * rotation[4]) + dx * rotation[1]);
	pnt[2] = (NxReal) ((dz * rotation[8] + dy * rotation[5]) + dx * rotation[2]);
	dir[0] = (NxReal) (((double) rotation[6] * direction[2] + (double) rotation[3] * direction[1])
		+ (double) rotation[0] * direction[0]);
	dir[1] = (NxReal) (((double) rotation[7] * direction[2] + (double) rotation[4] * direction[1])
		+ (double) rotation[1] * direction[0]);
	dir[2] = (NxReal) (((double) rotation[8] * direction[2] + (double) rotation[5] * direction[1])
		+ (double) rotation[2] * direction[0]);

	bool reflect[3];
	for(int i = 0; i < 3; ++i)
		{
		if(dir[i] < 0.0f)
			{
			reflect[i] = true;
			pnt[i] = -pnt[i];
			dir[i] = -dir[i];
			}
		else
			reflect[i] = false;
		}

	NxReal squared = 0.0f;
	const NxReal* extents = box->extents;

	if(dir[0] > 0.0f)
		{
		if(dir[1] > 0.0f)
			{
			if(dir[2] > 0.0f)
				nxLineBoxCaseNoZeros(pnt, dir, extents, lineParam, &squared);
			else
				nxLineBoxCase0(0, 1, 2, pnt, dir, extents, lineParam, &squared);
			}
		else
			{
			if(dir[2] > 0.0f)
				nxLineBoxCase0(0, 2, 1, pnt, dir, extents, lineParam, &squared);
			else
				nxLineBoxCase00(0, 1, 2, pnt, dir, extents, lineParam, &squared);
			}
		}
	else
		{
		if(dir[1] > 0.0f)
			{
			if(dir[2] > 0.0f)
				nxLineBoxCase0(1, 2, 0, pnt, dir, extents, lineParam, &squared);
			else
				nxLineBoxCase00(1, 0, 2, pnt, dir, extents, lineParam, &squared);
			}
		else
			{
			if(dir[2] > 0.0f)
				nxLineBoxCase00(2, 0, 1, pnt, dir, extents, lineParam, &squared);
			else
				{
				nxLineBoxCase000(pnt, extents, &squared);
				if(lineParam)
					*lineParam = 0.0f;
				}
			}
		}

	if(lineParam)
		{
		NxReal x = pnt[0];
		NxReal y = pnt[1];
		NxReal z = pnt[2];
		if(reflect[0])
			x = -x;
		if(reflect[1])
			y = -y;
		if(reflect[2])
			z = -z;
		*boxParam0 = x;
		*boxParam1 = y;
		*boxParam2 = z;
		}
	return squared;
	}

// phys_fn_001688 (0x00033d00, 378 B)
// Segment to box: the box is copied into the frame (centre, extents, then the
// nine rotation words by `rep movsd`), the line is p0 and the stored p1 - p0,
// and line/box is called with its four outputs in the caller's argument slots
// and one local (0x00033d70..0x00033daf). Its result stays in st(0) and is
// returned as it is when the parameter is on [0, 1]; past either end it is
// discarded and point/box's unnarrowed result is returned instead, called with
// the ORIGINAL rotation pointer and the caller's closest-point pointer.
__declspec(noinline) double __cdecl NxSegmentBoxSquareDistance(const NxSegment* segment,
	const NxReal* center, const NxReal* extents, const NxReal* rotation,
	NxReal* segmentParam, NxReal* boxPoint)
	{
	NxCollisionBoxData box;
	memcpy(box.center, center, sizeof(box.center));
	memcpy(box.extents, extents, sizeof(box.extents));
	memcpy(box.rotation, rotation, sizeof(box.rotation));

	NxDistanceLine line;
	line.origin[0] = segment->p0.x;
	line.origin[1] = segment->p0.y;
	line.origin[2] = segment->p0.z;
	line.direction[0] = (NxReal) ((double) segment->p1.x - segment->p0.x);
	line.direction[1] = (NxReal) ((double) segment->p1.y - segment->p0.y);
	line.direction[2] = (NxReal) ((double) segment->p1.z - segment->p0.z);

	NxReal t;
	NxReal b0;
	NxReal b1;
	NxReal b2;
	const double squared = NxLineBoxSquareDistance(&line, &box, &t, &b0, &b1, &b2);

	if(t >= 0.0f)
		{
		if(t <= 1.0f)
			{
			if(segmentParam)
				*segmentParam = t;
			if(boxPoint)
				{
				boxPoint[0] = b0;
				boxPoint[1] = b1;
				boxPoint[2] = b2;
				}
			return squared;
			}
		if(segmentParam)
			*segmentParam = 1.0f;
		return NxPointBoxSquareDistance(&segment->p1.x, center, extents, rotation, boxPoint);
		}
	if(segmentParam)
		*segmentParam = 0.0f;
	return NxPointBoxSquareDistance(&segment->p0.x, center, extents, rotation, boxPoint);
	}

// ---------------------------------------------------------------------------
// The triangle half of sub-unit E (convex-mesh gap Task 2b): point/triangle,
// line/line and segment/triangle. Eberly's DistVec3Tri3 and DistSeg3Tri3 as
// NovodeX built them; the same caveat as above applies -- the listing is what
// is transcribed, association and all.

// Point/triangle's leaves. The oracle's 001672 is one function whose FPU
// stack holds the first edge through the setup and then c (the squared offset)
// and t through every branch to the end. Written as one C++ function, MSVC
// spilled the edge, c and t to 8-byte slots, which cut them to 53 bits under
// 0x0f7f (the class Face's leaves above avoid). So point/triangle is written as
// the listing's leaves, each its own small function entered only with the
// values the listing narrows (NxPointTriangleTerms) and forming its wide ones
// locally from them: c from the three stored offset components, t from b0,
// b1, a00 and a01, a numerator from its float operands, all in the listing's
// order, so each is the same value the oracle's register holds. Each leaf ends
// the row (the parameters written, the result |squared| left in st(0)), so no
// wide result crosses a join either.

// Everything 0x000329e0..0x00032b0c stores: the five coefficients, the
// determinant and s, and the offset v0 - p.
struct NxPointTriangleTerms
	{
	NxReal a00;
	NxReal a01;
	NxReal a11;
	NxReal b0;
	NxReal b1;
	NxReal det;
	NxReal s;
	NxReal dx;
	NxReal dy;
	NxReal dz;
	NxReal e1x;
	NxReal e1y;
	NxReal e1z;
	};

// The first edge's three products (0x000329eb..0x00032a9e): a00 = (x^2 + z^2)
// + y^2, a01 = (e1.x x + e1.z z) + e1.y y and b0 = (d.x x + d.z z) + d.y y,
// with the first edge on the FPU stack throughout. A function of its own, entered
// only with stored floats: written inside the whole setup, MSVC held the edge in
// three 8-byte slots (cutting it to 53 bits under 0x0f7f) and read a01 and b0
// from those copies.
static __declspec(noinline) void nxPointTriangleFirstEdge(const NxReal* v0, const NxReal* v1,
	NxPointTriangleTerms* k)
	{
	const double e0x = (double) v1[0] - v0[0];
	const double e0y = (double) v1[1] - v0[1];
	const double e0z = (double) v1[2] - v0[2];
	k->a00 = (NxReal) ((e0x * e0x + e0z * e0z) + e0y * e0y);
	k->a01 = (NxReal) (((double) k->e1x * e0x + (double) k->e1z * e0z) + (double) k->e1y * e0y);
	k->b0 = (NxReal) (((double) k->dx * e0x + (double) k->dz * e0z) + (double) k->dy * e0y);
	}

// 0x000329e0..0x00032b0c. The first edge is on the FPU stack; the second edge
// and the offset are stored. a00 is (x^2 + z^2) + y^2 of the first edge.
static __declspec(noinline) void nxPointTriangleTerms(const NxReal* point, const NxReal* v0,
	const NxReal* v1, const NxReal* v2, NxPointTriangleTerms* k)
	{
	k->e1x = (NxReal) ((double) v2[0] - v0[0]);
	k->e1y = (NxReal) ((double) v2[1] - v0[1]);
	k->e1z = (NxReal) ((double) v2[2] - v0[2]);
	k->dx = (NxReal) ((double) v0[0] - point[0]);
	k->dy = (NxReal) ((double) v0[1] - point[1]);
	k->dz = (NxReal) ((double) v0[2] - point[2]);
	nxPointTriangleFirstEdge(v0, v1, k);

	const NxReal e1x = k->e1x;
	const NxReal e1y = k->e1y;
	const NxReal e1z = k->e1z;
	k->a11 = (NxReal) (((double) e1z * e1z + (double) e1y * e1y) + (double) e1x * e1x);
	k->b1 = (NxReal) (((double) k->dz * e1z + (double) k->dy * e1y) + (double) k->dx * e1x);
	k->det = (NxReal) fabs((double) k->a11 * k->a00 - (double) k->a01 * k->a01);
	k->s = (NxReal) ((double) k->b1 * k->a01 - (double) k->b0 * k->a11);
	}

// c, the squared offset (0x00032ac8..0x00032ae2): (z^2 + y^2) + x^2.
static __forceinline double nxPointTriangleC(const NxPointTriangleTerms* k)
	{
	return ((double) k->dz * k->dz + (double) k->dy * k->dy) + (double) k->dx * k->dx;
	}

// t before any division (0x00032b10..0x00032b20): b0 a01 - b1 a00.
static __forceinline double nxPointTriangleT(const NxPointTriangleTerms* k)
	{
	return (double) k->b0 * k->a01 - (double) k->b1 * k->a00;
	}

// The denominator of the three quotient leaves: (a00 - (a01 + a01)) + a11.
static __forceinline double nxPointTriangleDenominator(const NxPointTriangleTerms* k)
	{
	return ((double) k->a00 - ((double) k->a01 + k->a01)) + k->a11;
	}

// 0x00032eec: the parameters, then |squared|.
static __forceinline double nxPointTriangleFinish(NxReal* sParam, NxReal* tParam,
	NxReal s, NxReal t, double squared)
	{
	if(sParam)
		*sParam = s;
	if(tParam)
		*tParam = t;
	return fabs(squared);
	}

// Vertex 1 (0x00032cc9): s = 1, t = 0, ((b0 + b0) + c) + a00.
static __declspec(noinline) double nxPointTriangleVertex1(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	return nxPointTriangleFinish(sParam, tParam, 1.0f, 0.0f,
		(((double) k->b0 + k->b0) + nxPointTriangleC(k)) + k->a00);
	}

// Vertex 2 (0x00032d0c, 0x00032d9e, 0x00032e5c): s = 0, t = 1,
// ((b1 + b1) + c) + a11.
static __declspec(noinline) double nxPointTriangleVertex2(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	return nxPointTriangleFinish(sParam, tParam, 0.0f, 1.0f,
		(((double) k->b1 + k->b1) + nxPointTriangleC(k)) + k->a11);
	}

// Vertex 0 (0x00032bb9, 0x00032c01 with 0x00032c1e, 0x00032d2d, 0x00032e1c):
// both parameters 0 and the squared distance c.
static __declspec(noinline) double nxPointTriangleVertex0(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	return nxPointTriangleFinish(sParam, tParam, 0.0f, 0.0f, nxPointTriangleC(k));
	}

// Edge 0's quotient (0x00032e2d): s = -(b0 / a00), kept wide for s b0 + c.
static __declspec(noinline) double nxPointTriangleEdge0Quotient(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const double s = -((double) k->b0 / k->a00);
	return nxPointTriangleFinish(sParam, tParam, (NxReal) s, 0.0f, s * k->b0 + nxPointTriangleC(k));
	}

// Edge 1's quotient (0x00032d3a/0x00032d3e): t = -(b1 / a11), kept wide.
static __declspec(noinline) double nxPointTriangleEdge1Quotient(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const double t = -((double) k->b1 / k->a11);
	return nxPointTriangleFinish(sParam, tParam, 0.0f, (NxReal) t, t * k->b1 + nxPointTriangleC(k));
	}

// Edge 0, t = 0 (0x00032b72, from regions 4 and 5): past the far vertex when
// -b0 >= a00.
static __declspec(noinline) double nxPointTriangleEdge0(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	if(-k->b0 >= k->a00)
		return nxPointTriangleVertex1(k, sParam, tParam);
	return nxPointTriangleEdge0Quotient(k, sParam, tParam);
	}

// Edge 1, s = 0 (0x00032ba0, regions 3 and 4).
static __declspec(noinline) double nxPointTriangleEdge1(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	if(k->b1 >= 0.0f)
		return nxPointTriangleVertex0(k, sParam, tParam);
	if(-k->b1 >= k->a11)
		return nxPointTriangleVertex2(k, sParam, tParam);
	return nxPointTriangleEdge1Quotient(k, sParam, tParam);
	}

// The interior sum (0x00032eb1): the first t is `first`, every later read of
// t and s is the narrowed copy; ((X + Y) + c) with
// X = t ((first a11 + s a01) + (b1 + b1)) and Y = s ((t a01 + s a00) + (b0 + b0)).
static __forceinline double nxPointTriangleInterior(const NxPointTriangleTerms* k,
	NxReal s, NxReal t, double first)
	{
	return ((first * k->a11 + (double) s * k->a01) + ((double) k->b1 + k->b1)) * t
		+ (((double) t * k->a01 + (double) s * k->a00) + ((double) k->b0 + k->b0)) * s
		+ nxPointTriangleC(k);
	}

// The interior entered from a quotient s (0x00032e9b..0x00032ead): t = 1 - s
// stored with `fst` and its wide value used first.
static __declspec(noinline) double nxPointTriangleInteriorFromS(const NxPointTriangleTerms* k,
	NxReal s, NxReal* sParam, NxReal* tParam)
	{
	const double first = 1.0f - (double) s;
	const NxReal t = (NxReal) first;
	return nxPointTriangleFinish(sParam, tParam, s, t, nxPointTriangleInterior(k, s, t, first));
	}

// The interior entered with t reloaded (0x00032c79, 0x00032dd8).
static __declspec(noinline) double nxPointTriangleInteriorNarrow(const NxPointTriangleTerms* k,
	NxReal s, NxReal t, NxReal* sParam, NxReal* tParam)
	{
	return nxPointTriangleFinish(sParam, tParam, s, t, nxPointTriangleInterior(k, s, t, t));
	}

// Region 0 (0x00032c2b): the determinant-zero interior returns FLT_MAX
// (0x10106858) with both parameters 0; otherwise s and t are scaled by the
// wide reciprocal and narrowed.
static __declspec(noinline) double nxPointTriangleRegion0(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	if(k->det == 0.0f)
		return nxPointTriangleFinish(sParam, tParam, 0.0f, 0.0f, 3.402823466e+38f);
	const double inverse = 1.0f / (double) k->det;
	const NxReal s = (NxReal) ((double) k->s * inverse);
	const NxReal t = (NxReal) (inverse * nxPointTriangleT(k));
	return nxPointTriangleInteriorNarrow(k, s, t, sParam, tParam);
	}

// Region 2's quotient (0x00032cab..0x00032cc5): numer = (b1 + a11) - tmp0.
static __declspec(noinline) double nxPointTriangleRegion2Quotient(const NxPointTriangleTerms* k,
	NxReal tmp0, NxReal* sParam, NxReal* tParam)
	{
	const double numer = ((double) k->b1 + k->a11) - tmp0;
	const double denom = nxPointTriangleDenominator(k);
	if(numer >= denom)
		return nxPointTriangleVertex1(k, sParam, tParam);
	return nxPointTriangleInteriorFromS(k, (NxReal) (numer / denom), sParam, tParam);
	}

// Region 2 (0x00032c8b): tmp0 = b0 + a01 narrowed into the t slot, tmp1 =
// b1 + a11 wide.
static __declspec(noinline) double nxPointTriangleRegion2(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const NxReal tmp0 = (NxReal) ((double) k->b0 + k->a01);
	const double tmp1 = (double) k->b1 + k->a11;
	if(tmp1 > tmp0)
		return nxPointTriangleRegion2Quotient(k, tmp0, sParam, tParam);
	if(tmp1 <= 0.0)
		return nxPointTriangleVertex2(k, sParam, tParam);
	if(k->b1 >= 0.0f)
		return nxPointTriangleVertex0(k, sParam, tParam);
	return nxPointTriangleEdge1Quotient(k, sParam, tParam);
	}

// Region 6's quotient (0x00032d80..0x00032dd8): t = numer / denom narrowed,
// s = 1 - t narrowed, and the interior entered with t reloaded.
static __declspec(noinline) double nxPointTriangleRegion6Quotient(const NxPointTriangleTerms* k,
	NxReal tmp0, NxReal* sParam, NxReal* tParam)
	{
	const double numer = ((double) k->b0 + k->a00) - tmp0;
	const double denom = nxPointTriangleDenominator(k);
	if(numer >= denom)
		return nxPointTriangleVertex2(k, sParam, tParam);
	const NxReal t = (NxReal) (numer / denom);
	const NxReal s = (NxReal) (1.0f - (double) t);
	return nxPointTriangleInteriorNarrow(k, s, t, sParam, tParam);
	}

// Region 6 (0x00032d65): tmp0 = b1 + a01 narrowed, tmp1 = b0 + a00 wide.
static __declspec(noinline) double nxPointTriangleRegion6(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const NxReal tmp0 = (NxReal) ((double) k->b1 + k->a01);
	const double tmp1 = (double) k->b0 + k->a00;
	if(tmp1 > tmp0)
		return nxPointTriangleRegion6Quotient(k, tmp0, sParam, tParam);
	if(tmp1 <= 0.0)
		return nxPointTriangleVertex1(k, sParam, tParam);
	if(k->b0 >= 0.0f)
		return nxPointTriangleVertex0(k, sParam, tParam);
	return nxPointTriangleEdge0Quotient(k, sParam, tParam);
	}

// Region 1 (0x00032e40): numer = ((b1 + a11) - a01) - b0, stored narrowed
// into the t slot and tested wide against 0; the division reads the narrowed
// copy.
static __declspec(noinline) double nxPointTriangleRegion1(const NxPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const double numerWide = (((double) k->b1 + k->a11) - k->a01) - k->b0;
	const NxReal numer = (NxReal) numerWide;
	if(numerWide <= 0.0)
		return nxPointTriangleVertex2(k, sParam, tParam);
	const double denom = nxPointTriangleDenominator(k);
	if(numer >= denom)
		return nxPointTriangleVertex1(k, sParam, tParam);
	return nxPointTriangleInteriorFromS(k, (NxReal) (numer / denom), sParam, tParam);
	}

// The region decision (0x00032b10..0x00032b68, 0x00032bf2, 0x00032c82,
// 0x00032d50): s + t against the determinant, then the signs of s, t and b0.
// Returns Eberly's region number.
static __declspec(noinline) int nxPointTriangleRegion(const NxPointTriangleTerms* k)
	{
	const double t = nxPointTriangleT(k);
	if((double) k->s + t <= k->det)
		{
		if(k->s < 0.0f)
			return (t < 0.0 && k->b0 < 0.0f) ? 4 : 3;
		return t < 0.0 ? 5 : 0;
		}
	if(k->s < 0.0f)
		return 2;
	return t < 0.0 ? 6 : 1;
	}

// phys_fn_001672 (0x000329e0, 1326 B)
// Point to triangle (origin v0, edges v1 - v0 and v2 - v0), with the two edge
// parameters written through optional pointers. The first edge stays on the
// FPU stack (0x000329eb..0x000329fc) and is squared and dotted from there; the
// second edge and the offset v0 - p are stored. a00, a01, a11, b0, b1, the
// determinant and s are narrowed -- b0, the determinant and s into the
// caller's first three argument slots, b1 into the fourth -- while c (the
// squared offset, 0x00032ac8..0x00032ae2) and t stay in registers; c is added
// last in every leaf. The quotient leaves keep the parameter they just divided
// in st(0) (`fst` at 0x00032d43, 0x00032e33, 0x00032ead) and use it wide; the
// interior leaf (0x00032eb1) reads its first t either wide or reloaded, by
// entry. The determinant-zero interior returns FLT_MAX (0x10106858) with both
// parameters 0. Every comparison keeps the listing's NaN side: a NaN takes the
// branch the `test ah` pattern gives it (for example b1 NaN at 0x00032bb7 is
// "b1 < 0", the quotient). Regions 3 and 4 share the edge-1 leaf and regions 4
// and 5 the edge-0 leaf, as the listing's jumps do.
__declspec(noinline) double __cdecl NxPointTriangleSquareDistance(const NxReal* point,
	const NxReal* v0, const NxReal* v1, const NxReal* v2, NxReal* sParam, NxReal* tParam)
	{
	NxPointTriangleTerms k;
	nxPointTriangleTerms(point, v0, v1, v2, &k);
	switch(nxPointTriangleRegion(&k))
		{
		case 0:
			return nxPointTriangleRegion0(&k, sParam, tParam);
		case 1:
			return nxPointTriangleRegion1(&k, sParam, tParam);
		case 2:
			return nxPointTriangleRegion2(&k, sParam, tParam);
		case 3:
			return nxPointTriangleEdge1(&k, sParam, tParam);
		case 4:
			return nxPointTriangleEdge0(&k, sParam, tParam);
		case 5:
			if(k.b0 >= 0.0f)
				return nxPointTriangleVertex0(&k, sParam, tParam);
			return nxPointTriangleEdge0(&k, sParam, tParam);
		default:
			return nxPointTriangleRegion6(&k, sParam, tParam);
		}
	}

// Clamp to [0, 1] as 0x00034681..0x000346c7 (and its three copies) do it: past
// 1 is 1, and anything not on [0, 1] otherwise -- below 0, or a NaN -- is 0.
// The kept value is the register itself, not a narrowed copy.
static __forceinline double nxClampUnit(double x)
	{
	if(x < 0.0)
		return 0.0f;
	if(x > 1.0)
		return 1.0f;
	if(x >= 0.0 && x <= 1.0)
		return x;
	return 0.0f;
	}

// phys_fn_001692 (0x000345b0, 684 B)
// Closest points of two lines given as origin and direction: s on the first
// from the 2x2 system, clamped to [0, 1] and kept in a register; t from s,
// narrowed into the caller's c slot (0x000346d5), and when t leaves [0, 1] --
// tested wide against 0 but narrowed against 1 -- it is pinned and s is
// recomputed from the pinned end. The offset origin1 - origin0 never touches
// memory. The two points are formed differently: the first narrows all three
// products s * dir0 before adding the origin, the second keeps t * dir1.x in
// st(0) (0x00034821) and narrows only y and z.
__declspec(noinline) void __cdecl NxLineLineClosestPoints(NxReal* point0, NxReal* point1,
	const NxReal* origin0, const NxReal* dir0, const NxReal* origin1, const NxReal* dir1)
	{
	const double offX = (double) origin1[0] - origin0[0];
	const double offY = (double) origin1[1] - origin0[1];
	const double offZ = (double) origin1[2] - origin0[2];

	const NxReal a = (NxReal) (((double) dir0[2] * dir0[2] + (double) dir0[0] * dir0[0])
		+ (double) dir0[1] * dir0[1]);
	const NxReal c = (NxReal) (((double) dir1[0] * dir1[0] + (double) dir1[1] * dir1[1])
		+ (double) dir1[2] * dir1[2]);
	const NxReal b = (NxReal) (((double) dir0[2] * dir1[2] + (double) dir0[1] * dir1[1])
		+ (double) dir0[0] * dir1[0]);
	const NxReal e = (NxReal) ((offZ * dir0[2] + offY * dir0[1]) + offX * dir0[0]);
	const NxReal f = (NxReal) ((offX * dir1[0] + offZ * dir1[2]) + offY * dir1[1]);

	double s = nxClampUnit(((double) e * c - (double) f * b) / ((double) c * a - (double) b * b));

	const double tWide = ((double) b * s - f) / c;
	NxReal t = (NxReal) tWide;
	if(tWide < 0.0)
		{
		t = 0.0f;
		s = nxClampUnit((double) e / a);
		}
	else if(t > 1.0f)
		{
		t = 1.0f;
		s = nxClampUnit(((double) e + b) / a);
		}
	else if(!(t >= 0.0f && t <= 1.0f))
		{
		t = 0.0f;
		s = nxClampUnit((double) e / a);
		}

	const NxReal p0x = (NxReal) (s * dir0[0]);
	const NxReal p0y = (NxReal) (s * dir0[1]);
	const NxReal p0z = (NxReal) (s * dir0[2]);
	point0[0] = (NxReal) ((double) p0x + origin0[0]);
	point0[1] = (NxReal) ((double) p0y + origin0[1]);
	point0[2] = (NxReal) ((double) p0z + origin0[2]);

	const double p1x = (double) t * dir1[0];
	const NxReal p1y = (NxReal) ((double) t * dir1[1]);
	const NxReal p1z = (NxReal) ((double) t * dir1[2]);
	point1[0] = (NxReal) (p1x + origin1[0]);
	point1[1] = (NxReal) ((double) p1y + origin1[1]);
	point1[2] = (NxReal) ((double) p1z + origin1[2]);
	}

// ---------------------------------------------------------------------------
// Segment/triangle (phys_fn_001694) and its pieces. Eberly's DistSeg3Tri3:
// the segment p0 + r (p1 - p0) against the triangle v0 + s e0 + t e1 (e0 =
// v1 - v0, e1 = v2 - v0). When the 3x3 system is not singular its solution
// (r, s, t) picks one of 19 regions (r below 0, on [0, 1] or past 1, times the
// seven regions of the triangle's plane); each region but the interior one
// takes the smallest of a few boundary distances: segment against an edge
// (phys_fn_001690), or an end of the segment against the triangle
// (phys_fn_001672). When it is singular (|det| < 1e-5, 0x10107958) the segment
// is parallel to the plane and all five boundary distances are taken.
//
// What the listing fixes and a reading of Eberly would not:
//  * The best squared distance is a float: every candidate the callee leaves
//    wide in st(0) is compared wide against the narrowed best and narrowed when
//    it wins (`fcom dword; test ah, 5; jp` -- a NaN never wins), and the row
//    returns |best| loaded back from its float slot, 0x000360ce. Even the
//    interior's closed form is stored (0x00035524) and reloaded.
//  * The edge segments are (origin, origin + edge) with each end narrowed, not
//    Eberly's (origin, direction). The third edge is (v1, v1 + (e1 - e0)) with
//    z's difference narrowed first and x's and y's kept wide (0x00034f0f..
//    0x00034f46 and its seven copies: eight sites). The far end of the segment is re-formed
//    as p0 + (p1 - p0), narrowed, not read as p1.
//  * s stays wide in st(0) from 0x00034b50 through the region tests, and on the
//    r in [0, 1] path on into the interior's closed form; r and t are narrowed
//    before they are tested.
//  * A NaN goes the way `test ah` sends it: r NaN is "r >= 0" and then "r > 1",
//    a NaN s + t is "s + t > 1", a NaN s or t is ">= 0", and a NaN determinant
//    is parallel.
//
// Everything wide that the listing keeps across a branch is formed from stored
// floats alone (s from the cofactors and right-hand sides, 0x00034b1a..
// 0x00034b50), so it is recomputed where it is used rather than kept, the
// same device as point/triangle's leaves above.

// Everything the listing stores in the setup (0x00034860..0x00034b8d).
struct NxSegmentTriangleTerms
	{
	NxReal e0[3];
	NxReal e1[3];
	NxReal dir[3];
	NxReal diff[3];
	NxReal a00;
	NxReal a01;
	NxReal a02;
	NxReal a11;
	NxReal a12;
	NxReal a22;
	NxReal b0;
	NxReal b1;
	NxReal b2;
	NxReal cof01;
	NxReal cof02;
	NxReal cof12;
	NxReal rhs0;
	NxReal rhs1;
	NxReal rhs2;
	NxReal r;
	NxReal s;
	NxReal t;
	};

// cof00 as the listing forms it and keeps it in st(1) (0x00034a2d..0x00034a43).
static __forceinline double nxSegmentTriangleCof00(const NxSegmentTriangleTerms* k)
	{
	return (double) k->a22 * k->a11 - (double) k->a12 * k->a12;
	}

// 0x00034a45..0x00034a8e: cof01 and cof02 narrowed (cof02 with `fst`, its wide
// value going on into the determinant), and the determinant returned in st(0).
static __declspec(noinline) double nxSegmentTriangleDeterminant(NxSegmentTriangleTerms* k)
	{
	const double cof00 = nxSegmentTriangleCof00(k);
	k->cof01 = (NxReal) ((double) k->a12 * k->a02 - (double) k->a22 * k->a01);
	const double cof02 = (double) k->a12 * k->a01 - (double) k->a11 * k->a02;
	k->cof02 = (NxReal) cof02;
	return (cof02 * k->a02 + (double) k->cof01 * k->a01) + cof00 * k->a00;
	}

// 0x00034b1a..0x00034b8d: s and t from the stored cofactors and right-hand
// sides, each narrowed (s with `fst`, kept wide in the listing; see
// nxSegmentTriangleS).
static __declspec(noinline) void nxSegmentTriangleST(NxSegmentTriangleTerms* k)
	{
	const NxReal rhs0 = k->rhs0;
	const NxReal rhs1 = k->rhs1;
	const NxReal rhs2 = k->rhs2;
	k->s = (NxReal) ((((double) k->a22 * k->a00 - (double) k->a02 * k->a02) * rhs1
		+ (double) rhs2 * k->cof12) + (double) rhs0 * k->cof01);
	k->t = (NxReal) ((((double) k->a11 * k->a00 - (double) k->a01 * k->a01) * rhs2
		+ (double) rhs1 * k->cof12) + (double) rhs0 * k->cof02);
	}

// 0x00034a90..0x00034b18: the tolerance test on |det| (a NaN is singular),
// cof12, the reciprocal and the three right-hand sides -- the third kept wide
// for r -- and r over the wide cof00. Returns false when singular.
static __declspec(noinline) bool nxSegmentTriangleSolve(NxSegmentTriangleTerms* k)
	{
	const double det = nxSegmentTriangleDeterminant(k);
	if(!(fabs(det) >= 1e-5f))
		return false;
	k->cof12 = (NxReal) ((double) k->a02 * k->a01 - (double) k->a12 * k->a00);
	const double inverse = 1.0f / det;
	k->rhs0 = (NxReal) -((double) k->b0 * inverse);
	k->rhs1 = (NxReal) -((double) k->b1 * inverse);
	const double rhs2 = -(inverse * k->b2);
	k->rhs2 = (NxReal) rhs2;
	k->r = (NxReal) ((rhs2 * k->cof02 + (double) k->rhs1 * k->cof01)
		+ (double) k->rhs0 * nxSegmentTriangleCof00(k));
	nxSegmentTriangleST(k);
	return true;
	}

// 0x00034860..0x00034a9f: the edges, the segment's direction and the offset
// v0 - p0, the six coefficients and three right-hand sides, and the
// determinant over the cofactors, cof00 and cof02 wide. Returns true when it
// is not singular, having gone on to 0x00034b8d: cof12, the reciprocal, the
// three right-hand sides (the third kept wide for r) and r, s, t.
static __declspec(noinline) bool nxSegmentTriangleTerms(const NxSegment* segment,
	const NxReal* v0, const NxReal* v1, const NxReal* v2, NxSegmentTriangleTerms* k)
	{
	for(int i = 0; i < 3; ++i)
		{
		k->e0[i] = (NxReal) ((double) v1[i] - v0[i]);
		k->e1[i] = (NxReal) ((double) v2[i] - v0[i]);
		}
	k->dir[0] = (NxReal) ((double) segment->p1.x - segment->p0.x);
	k->dir[1] = (NxReal) ((double) segment->p1.y - segment->p0.y);
	k->dir[2] = (NxReal) ((double) segment->p1.z - segment->p0.z);
	k->diff[0] = (NxReal) ((double) v0[0] - segment->p0.x);
	k->diff[1] = (NxReal) ((double) v0[1] - segment->p0.y);
	k->diff[2] = (NxReal) ((double) v0[2] - segment->p0.z);

	const NxReal* e0 = k->e0;
	const NxReal* e1 = k->e1;
	const NxReal* d = k->dir;
	const NxReal* q = k->diff;
	k->a00 = (NxReal) (((double) d[2] * d[2] + (double) d[1] * d[1]) + (double) d[0] * d[0]);
	k->a01 = (NxReal) -(((double) e0[2] * d[2] + (double) e0[1] * d[1]) + (double) d[0] * e0[0]);
	k->a02 = (NxReal) -(((double) d[2] * e1[2] + (double) d[1] * e1[1]) + (double) d[0] * e1[0]);
	k->a11 = (NxReal) (((double) e0[2] * e0[2] + (double) e0[1] * e0[1]) + (double) e0[0] * e0[0]);
	k->a12 = (NxReal) (((double) e0[2] * e1[2] + (double) e0[1] * e1[1]) + (double) e1[0] * e0[0]);
	k->a22 = (NxReal) (((double) e1[2] * e1[2] + (double) e1[1] * e1[1]) + (double) e1[0] * e1[0]);
	k->b0 = (NxReal) -(((double) q[2] * d[2] + (double) q[1] * d[1]) + (double) q[0] * d[0]);
	k->b1 = (NxReal) (((double) e0[2] * q[2] + (double) e0[1] * q[1]) + (double) q[0] * e0[0]);
	k->b2 = (NxReal) (((double) q[2] * e1[2] + (double) q[1] * e1[1]) + (double) q[0] * e1[0]);

	return nxSegmentTriangleSolve(k);
	}

// s as the listing keeps it in st(0) (0x00034b1a..0x00034b50): from the stored
// cofactors and right-hand sides only.
static __forceinline double nxSegmentTriangleS(const NxSegmentTriangleTerms* k)
	{
	return (((double) k->a22 * k->a00 - (double) k->a02 * k->a02) * k->rhs1
		+ (double) k->rhs2 * k->cof12) + (double) k->rhs0 * k->cof01;
	}

// The regions, numbered as in Eberly with r's third as a suffix: 0..6 for r
// below 0 (his "m"), 10..16 for r on [0, 1], 20..26 for r past 1 ("p").
static __declspec(noinline) int nxSegmentTriangleRegion(const NxSegmentTriangleTerms* k)
	{
	const double s = nxSegmentTriangleS(k);
	int third;
	if(k->r < 0.0f)
		third = 0;
	else if(k->r <= 1.0f)
		third = 10;
	else
		third = 20;
	if((double) k->t + s <= 1.0)
		{
		if(s < 0.0)
			return third + (k->t < 0.0f ? 4 : 3);
		return third + (k->t < 0.0f ? 5 : 0);
		}
	if(s < 0.0)
		return third + 2;
	return third + (k->t < 0.0f ? 6 : 1);
	}

// The interior (0x00035483..0x00035524), in the listing's order: t's term, then
// s's, then r's -- each ((t a + s a + r a) + 2 b) times its own parameter, with s
// wide throughout -- then the offset's squared length added z, y, x.
static __declspec(noinline) NxReal nxSegmentTriangleInterior(const NxSegmentTriangleTerms* k)
	{
	const NxReal r = k->r;
	const NxReal t = k->t;
	const double s = nxSegmentTriangleS(k);
	const double tTerm = ((((double) t * k->a22 + s * k->a12) + (double) r * k->a02)
		+ ((double) k->b2 + k->b2)) * t;
	const double sTerm = ((((double) t * k->a12 + s * k->a11) + (double) r * k->a01)
		+ ((double) k->b1 + k->b1)) * s;
	const double rTerm = ((((double) t * k->a02 + s * k->a01) + (double) r * k->a00)
		+ ((double) k->b0 + k->b0)) * r;
	return (NxReal) (((((tTerm + sTerm) + rTerm) + (double) k->diff[2] * k->diff[2])
		+ (double) k->diff[1] * k->diff[1]) + (double) k->diff[0] * k->diff[0]);
	}

// (origin, origin + edge), the far end narrowed per component.
static __forceinline void nxEdgeSegment(NxSegment* out, const NxReal* origin, const NxReal* edge)
	{
	out->p0.x = origin[0];
	out->p0.y = origin[1];
	out->p0.z = origin[2];
	out->p1.x = (NxReal) ((double) edge[0] + origin[0]);
	out->p1.y = (NxReal) ((double) edge[1] + origin[1]);
	out->p1.z = (NxReal) ((double) edge[2] + origin[2]);
	}

// (v1, v1 + (e1 - e0)): z's difference narrowed first, x's and y's wide.
static __forceinline void nxThirdEdgeSegment(NxSegment* out, const NxReal* v1,
	const NxSegmentTriangleTerms* k)
	{
	const NxReal dz = (NxReal) ((double) k->e1[2] - k->e0[2]);
	out->p0.x = v1[0];
	out->p0.y = v1[1];
	out->p0.z = v1[2];
	out->p1.x = (NxReal) (((double) k->e1[0] - k->e0[0]) + v1[0]);
	out->p1.y = (NxReal) (((double) k->e1[1] - k->e0[1]) + v1[1]);
	out->p1.z = (NxReal) ((double) dz + v1[2]);
	}

// The running minimum: the float best and its three parameters.
struct NxSegmentTriangleBest
	{
	NxReal squared;
	NxReal r;
	NxReal s;
	NxReal t;
	};

// The segment against edge 0 (v0, v0 + e0): r and s, t = 0.
static void nxSegTriEdge0(NxSegmentTriangleBest* best, bool first, const NxSegment* segment,
	const NxReal* v0, const NxSegmentTriangleTerms* k)
	{
	NxSegment edge;
	nxEdgeSegment(&edge, v0, k->e0);
	if(first)
		{
		best->squared = (NxReal) NxSegmentSegmentSquareDistance(segment, &edge, &best->r, &best->s);
		best->t = 0.0f;
		return;
		}
	NxReal r;
	NxReal s;
	const double squared = NxSegmentSegmentSquareDistance(segment, &edge, &r, &s);
	if(squared < best->squared)
		{
		best->squared = (NxReal) squared;
		best->r = r;
		best->s = s;
		best->t = 0.0f;
		}
	}

// The segment against edge 1 (v0, v0 + e1): r and t, s = 0.
static void nxSegTriEdge1(NxSegmentTriangleBest* best, bool first, const NxSegment* segment,
	const NxReal* v0, const NxSegmentTriangleTerms* k)
	{
	NxSegment edge;
	nxEdgeSegment(&edge, v0, k->e1);
	if(first)
		{
		best->squared = (NxReal) NxSegmentSegmentSquareDistance(segment, &edge, &best->r, &best->t);
		best->s = 0.0f;
		return;
		}
	NxReal r;
	NxReal t;
	const double squared = NxSegmentSegmentSquareDistance(segment, &edge, &r, &t);
	if(squared < best->squared)
		{
		best->squared = (NxReal) squared;
		best->r = r;
		best->s = 0.0f;
		best->t = t;
		}
	}

// The segment against the third edge (v1, v1 + (e1 - e0)): r and t, s = 1 - t.
static void nxSegTriEdge2(NxSegmentTriangleBest* best, bool first, const NxSegment* segment,
	const NxReal* v1, const NxSegmentTriangleTerms* k)
	{
	NxSegment edge;
	nxThirdEdgeSegment(&edge, v1, k);
	if(first)
		{
		best->squared = (NxReal) NxSegmentSegmentSquareDistance(segment, &edge, &best->r, &best->t);
		best->s = (NxReal) (1.0f - (double) best->t);
		return;
		}
	NxReal r;
	NxReal t;
	const double squared = NxSegmentSegmentSquareDistance(segment, &edge, &r, &t);
	const NxReal s = (NxReal) (1.0f - (double) t);
	if(squared < best->squared)
		{
		best->squared = (NxReal) squared;
		best->r = r;
		best->s = s;
		best->t = t;
		}
	}

// An end of the segment against the triangle: s and t, r = 0 (the start) or
// 1 (p0 + (p1 - p0), narrowed).
static void nxSegTriEnd(NxSegmentTriangleBest* best, bool first, bool far, const NxSegment* segment,
	const NxReal* v0, const NxReal* v1, const NxReal* v2, const NxSegmentTriangleTerms* k)
	{
	NxReal end[3];
	const NxReal* point = &segment->p0.x;
	if(far)
		{
		end[0] = (NxReal) ((double) k->dir[0] + segment->p0.x);
		end[1] = (NxReal) ((double) k->dir[1] + segment->p0.y);
		end[2] = (NxReal) ((double) k->dir[2] + segment->p0.z);
		point = end;
		}
	if(first)
		{
		best->squared = (NxReal) NxPointTriangleSquareDistance(point, v0, v1, v2, &best->s, &best->t);
		best->r = far ? 1.0f : 0.0f;
		return;
		}
	NxReal s;
	NxReal t;
	const double squared = NxPointTriangleSquareDistance(point, v0, v1, v2, &s, &t);
	if(squared < best->squared)
		{
		best->squared = (NxReal) squared;
		best->r = far ? 1.0f : 0.0f;
		best->s = s;
		best->t = t;
		}
	}

// phys_fn_001694 (0x00034860, 6266 B)
// Segment to triangle, with the segment parameter and the two triangle
// parameters written through optional pointers (0x00036097..0x000360cc: r, then
// s from st(0), then t) and |best| returned. The regions and the boundary
// distances each takes, in the listing's order (the parameters of the first
// written straight into r, s, t; each later one replacing them only when its
// wide distance is below the narrowed best):
//
//   r < 0:   0 end0;  1 edge2, end0;  2 edge1, edge2, end0;  3 edge1, end0;
//            4 edge1, edge0, end0;  5 edge0, end0;  6 edge0, edge2, end0
//   r 0..1:  0 the interior's closed form;  1 edge2;  2 edge1, edge2;
//            3 edge1;  4 edge1, edge0;  5 edge0;  6 edge0, edge2
//   r > 1:   as r < 0 with end1 (r = 1) for end0
//   parallel: edge0, edge1, edge2, end0, end1
//
// (edge0 = (v0, v0 + e0) giving r, s and t = 0; edge1 = (v0, v0 + e1) giving
// r, t and s = 0; edge2 = the third edge giving r, t and s = 1 - t; endN = that
// end against the triangle through phys_fn_001672 giving s, t.)
__declspec(noinline) double __cdecl NxSegmentTriangleSquareDistance(const NxSegment* segment,
	const NxReal* v0, const NxReal* v1, const NxReal* v2,
	NxReal* segmentParam, NxReal* sParam, NxReal* tParam)
	{
	NxSegmentTriangleTerms k;
	NxSegmentTriangleBest best;
	if(!nxSegmentTriangleTerms(segment, v0, v1, v2, &k))
		{
		// 0x00035dd7: parallel.
		nxSegTriEdge0(&best, true, segment, v0, &k);
		nxSegTriEdge1(&best, false, segment, v0, &k);
		nxSegTriEdge2(&best, false, segment, v1, &k);
		nxSegTriEnd(&best, false, false, segment, v0, v1, v2, &k);
		nxSegTriEnd(&best, false, true, segment, v0, v1, v2, &k);
		}
	else
		{
		const int region = nxSegmentTriangleRegion(&k);
		const int triangleRegion = region % 10;
		const bool end = region < 10 || region >= 20;
		const bool far = region >= 20;
		if(region == 10)
			{
			best.squared = nxSegmentTriangleInterior(&k);
			best.r = k.r;
			best.s = k.s;
			best.t = k.t;
			}
		else if(end && triangleRegion == 0)
			nxSegTriEnd(&best, true, far, segment, v0, v1, v2, &k);
		else
			{
			switch(triangleRegion)
				{
				case 1:
					nxSegTriEdge2(&best, true, segment, v1, &k);
					break;
				case 2:
					nxSegTriEdge1(&best, true, segment, v0, &k);
					nxSegTriEdge2(&best, false, segment, v1, &k);
					break;
				case 3:
					nxSegTriEdge1(&best, true, segment, v0, &k);
					break;
				case 4:
					nxSegTriEdge1(&best, true, segment, v0, &k);
					nxSegTriEdge0(&best, false, segment, v0, &k);
					break;
				case 5:
					nxSegTriEdge0(&best, true, segment, v0, &k);
					break;
				default:
					nxSegTriEdge0(&best, true, segment, v0, &k);
					nxSegTriEdge2(&best, false, segment, v1, &k);
					break;
				}
			if(end)
				nxSegTriEnd(&best, false, far, segment, v0, v1, v2, &k);
			}
		}

	if(segmentParam)
		*segmentParam = best.r;
	if(sParam)
		*sParam = best.s;
	if(tParam)
		*tParam = best.t;
	return fabs((double) best.squared);
	}
