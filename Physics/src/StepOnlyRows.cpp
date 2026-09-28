// Rows of the scene-raycast block that the oracle reaches only from inside
// the simulation step, where the step has loaded the x87 control word 0x0f7f
// (64-bit precision, round toward zero). This file is on the /arch:IA32 list
// (CMakeLists.txt) so that their arithmetic runs on the x87 and honours that
// word, as the listing's does. At API time (0x027f, 53-bit, round to
// nearest) the x87 code gives the same bits as the SSE2 double arithmetic of
// the file they came from (ObjectModel.cpp), so moving them changes nothing
// the existing differentials observe.
//
// The rows here have no product caller while NpScene::simulate is a stub:
//   000867 -- called by the ActorPair constructor 000893 (step: 000911 ->
//             000901 -> 000893), not reproduced;
//   000951 -- BOX table slot 7, called only by the continuous-collision
//             sweep 002264 (0x100562c3, 0x100564b9), not reproduced.

#include "ObjectModel.h"

#include <math.h>
#include <string.h>

// phys_fn_000867 (0x0001d260, 103 B)
// The kind-selected accumulator, 0x1001d260-0x1001d2c4. The row is
// __thiscall `ret 0xc` on the accumulator (ecx) with (a, desc, b); this is
// a free function taking it first, like the row's other product callers
// expect (the only oracle caller, 000893, calls it directly). v = a / b stays
// unrounded on the x87 stack (fdiv at 0x1001d26b) and is used for every
// product. Kinds other than 4 and 5 (desc+0xc & 0x1f) add v to +0x64. Kinds 4
// and 5 add v*desc[0] and v*desc[1] from the stack but spill v*desc[2] to
// m32 first (fstp [esp+8] at 0x1001d299); bit 6 of desc+0xc then sets the
// byte at +0x75.
void nxAccumulateByKind0867(void* self, float a, void* desc, float b)
	{
	const double v = static_cast<double>(a) / b;
	unsigned char* d = reinterpret_cast<unsigned char*>(desc);
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned kind;
	memcpy(&kind, d + 0xc, 4);
	kind &= 0x1fu;
	if(kind != 4u && kind != 5u)
		{
		float t;
		memcpy(&t, p + 0x64, 4);
		t = static_cast<float>(v + t);					// fadd [ecx+0x64]
		memcpy(p + 0x64, &t, 4);
		return;
		}
	float c[3];
	memcpy(c, d, sizeof(c));
	const double p0 = v * c[0];							// fld st(0); fmul [eax]
	const double p1 = v * c[1];							// fld st(1); fmul [eax+4]
	const float p2 = static_cast<float>(v * c[2]);		// fmul [eax+8]; fstp m32
	float t[3];
	memcpy(t, p + 0x58, sizeof(t));
	t[0] = static_cast<float>(p0 + t[0]);				// 0x1001d29d
	t[1] = static_cast<float>(p1 + t[1]);				// 0x1001d2a3
	t[2] = static_cast<float>(static_cast<double>(p2) + t[2]);	// 0x1001d2ad
	memcpy(p + 0x58, t, sizeof(t));
	unsigned flags;
	memcpy(&flags, d + 0xc, 4);
	if(((flags >> 6) & 1u) != 0u)
		p[0x75] = 1;
	}

namespace
	{
	// The .rdata words the slab test compares against: [0x10107a10] = -2^-23
	// and [0x10107a0c] = +2^-23 (FLT_EPSILON).
	const float gSlabNegEpsilon = -1.1920928955078125e-7f;
	const float gSlabEpsilon = 1.1920928955078125e-7f;
	const float gSlabOne = 1.0f;						// [0x101041ec]
	const float gSweepMinusOne = -1.0f;					// [0x1010687c]

	// Row 001730 (0x00038050, cdecl, not claimed): the segment slab test
	// 000951 calls, written from its listing 0x10038050-0x100381b7 (the
	// loop body at 0x10038090 lies past the row's inventory size). Returns
	// the entering face (axis, or axis+3 when the slab was swapped) or -1.
	// *tnear starts at -FLT_MAX and *tfar at FLT_MAX. An axis whose
	// direction lies strictly inside (-eps, eps) only rejects an origin
	// outside its slab (strict compares; NaN passes). Otherwise
	// inv = 1/dir (m32), t1 = (min - o) * inv stays unrounded, t2 =
	// (max - o) * inv is spilled to m32; when t1 > t2 they swap (t1 rounded
	// to m32). A near value greater than *tnear replaces it and records the
	// face; a far value less than *tfar replaces it. The test fails when
	// *tnear > *tfar or *tfar < eps, inside the loop and again after it.
	// `noinline`: 000951 calls it as a function (0x10020ce7).
	__declspec(noinline) int nxSegmentSlabs(const float* minimum, const float* maximum,
		const float* origin, const float* dir, float* tnear, float* tfar)
		{
		const unsigned negMax = 0xff7fffffu, posMax = 0x7f7fffffu;
		memcpy(tnear, &negMax, 4);
		memcpy(tfar, &posMax, 4);
		int face = -1;
		for(int i = 0; i < 3; ++i)
			{
			const float di = dir[i];
			if(di > gSlabNegEpsilon && di < gSlabEpsilon)
				{
				if(origin[i] < minimum[i])				// 0x100380ae
					return -1;
				if(origin[i] > maximum[i])				// 0x100380bb
					return -1;
				continue;
				}
			const float inv = static_cast<float>(
				static_cast<double>(gSlabOne) / di);		// fdiv; fstp m32
			double nearT = (static_cast<double>(minimum[i]) - origin[i]) * inv;
			float farT = static_cast<float>(
				(static_cast<double>(maximum[i]) - origin[i]) * inv);
			int candidate = i;
			if(nearT > farT)							// fcom at 0x100380fd
				{
				const float swapped = static_cast<float>(nearT);
				nearT = farT;
				farT = swapped;
				candidate = i + 3;
				}
			if(nearT > *tnear)							// 0x1003811f
				{
				*tnear = static_cast<float>(nearT);
				face = candidate;
				}
			if(farT < *tfar)							// 0x10038138
				*tfar = farT;
			if(*tnear > *tfar)							// 0x1003814f
				return -1;
			if(*tfar < gSlabEpsilon)					// 0x10038162
				return -1;
			}
		if(*tnear > *tfar)								// 0x10038182
			return -1;
		if(*tfar < gSlabEpsilon)						// 0x10038199
			return -1;
		return face;
		}
	}

// phys_fn_000951 (0x00020b20, 507 B)
// BOX table slot 7, __thiscall `ret 8` (0x10020b20-0x10020d18): out (arg1),
// direction (arg2). The box's world translation t is carried into its own
// frame twice and summed: R^T(-t) (the negation by the -1.0 literal; x
// spilled to m32, y and z in registers; rows 1 and 2 spilled, row 0 kept)
// plus R^T t (row 0 kept, rows 1 and 2 spilled); origin = their sums, each
// spilled to m32. The direction goes to the box frame as R^T d (m32 each).
// 001730 then runs the slab test against [-dims, dims]; -1 returns false,
// otherwise *out = |tfar| and true. Each dot product sums its terms in the
// order the listing loads them.
bool BoxShape::nxBoxSweep(void* out, const float* direction) const
	{
	const float* R = reinterpret_cast<const float*>(mBase.mPose0C.mRotation);	// +0x0c
	const float* t = mBase.mPose0C.mTranslation;								// +0x30
	float maximum[3], minimum[3];
	maximum[0] = mHull.mDims04[0];							// [esp+0x58..0x60]
	maximum[1] = mHull.mDims04[1];
	maximum[2] = mHull.mDims04[2];
	minimum[0] = -mHull.mDims04[0];							// fchs, [esp+0x64..0x6c]
	minimum[1] = -mHull.mDims04[1];
	minimum[2] = -mHull.mDims04[2];

	const double minusOne = gSweepMinusOne;
	const float ntx = static_cast<float>(static_cast<double>(t[0]) * minusOne);	// [esp+8]
	const double nty = static_cast<double>(t[1]) * minusOne;
	const double ntz = static_cast<double>(t[2]) * minusOne;
	const double r00 = R[0], r01 = R[1], r02 = R[2];
	const double r10 = R[3], r11 = R[4], r12 = R[5];
	const double r20 = R[6], r21 = R[7], r22 = R[8];

	const float l1 = static_cast<float>((r21 * ntz + nty * r11) + r01 * ntx);	// [esp+0x18]
	const float l2 = static_cast<float>((r22 * ntz + nty * r12) + r02 * ntx);	// [esp+0x14]
	const double l0 = (ntz * r20 + nty * r10) + static_cast<double>(ntx) * r00;

	const double m0 = (r20 * t[2] + r10 * t[1]) + r00 * t[0];
	const float m1 = static_cast<float>((r01 * t[0] + r21 * t[2]) + r11 * t[1]);	// [esp+0xc]
	const float m2 = static_cast<float>((r02 * t[0] + r22 * t[2]) + r12 * t[1]);	// [esp+0x10]

	float origin[3];
	origin[0] = static_cast<float>(l0 + m0);				// [esp+0x4c]
	origin[1] = static_cast<float>(static_cast<double>(m1) + l1);
	origin[2] = static_cast<float>(static_cast<double>(m2) + l2);

	const double d0 = direction[0], d1 = direction[1], d2 = direction[2];
	float localDir[3];
	localDir[0] = static_cast<float>((r00 * d0 + r20 * d2) + r10 * d1);	// [esp+8]
	localDir[1] = static_cast<float>((r01 * d0 + r21 * d2) + r11 * d1);
	localDir[2] = static_cast<float>((r02 * d0 + r22 * d2) + r12 * d1);

	float tnear, tfar;										// [esp+0x18], [esp+0x14]
	if(nxSegmentSlabs(minimum, maximum, origin, localDir, &tnear, &tfar) == -1)
		return false;										// 0x10020d10
	*static_cast<float*>(out) = fabsf(tfar);				// fld; fabs; fstp
	return true;
	}
