/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Sub-unit E of units/convex-mesh-gap-contract.md, the box half: the point/box,
// line/box and segment/box squared distances (convex-mesh gap Task 2a). The
// triangle half (001672, 001692, 001694) is Task 2b's and joins this file.
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
