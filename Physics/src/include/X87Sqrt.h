#ifndef NX_PHYSICS_X87SQRT
#define NX_PHYSICS_X87SQRT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The x87 square roots the /arch:IA32 translation units share -- the joint
// rows in core/ and Geometry.cpp. Not oracle rows. Include only from x87
// (/arch:IA32) translation units.
//
// One set of helpers. The joint files had their own copy (core/JointX87.h,
// jointFsqrt*) with the same instruction bodies as this header's; it was
// folded in here by joint-open-items Task 1 as a rename only (the bodies were
// checked byte-identical by disassembly before and after the fold).
//
// Every square root in the joint rows is an inline `fsqrt` in the oracle
// (47 sites across 004097 004101 004121 004143 004240 004298 004306 004308
// 004310 004314 004326 004356 004374 004386; none calls the CRT), and so is
// every root in the Geometry.cpp rows. The oracle has no CRT square root at
// all (no sqrt import; the only CRT routines in it that execute fsqrt are
// _CIasin and _CIacos at 0x000f47f0 and 0x000f48e0).
//
//   * `sqrt()` in an /arch:IA32 unit is `call __CIsqrt`, and __CIsqrt does
//     not follow the control word: when it is not 0x027f it runs fsqrt under
//     (cw & 0x300) | 0x7f, i.e. rounding to nearest. The solver slots run
//     inside the simulation step's word 0x0f7f (64-bit precision, round
//     toward zero), so the oracle rounds a root toward zero where the CRT
//     rounds it to nearest. That is the "no CRT math where the oracle uses
//     x87 instructions" rule, and these helpers replace every such call.
//   * Why naked functions. The precedents (ContactGeneration.cpp,
//     SmoothNormals.cpp, ShapeRaycast.cpp `nxSqrt`) take a double, fsqrt it
//     and store the result to a double before returning; leaving st(0)
//     behind in an ordinary inlinable function broke an oracle-side digest
//     once (ContactGeneration.cpp's note). A naked function is never inlined,
//     so it is an ordinary cdecl call whose double result comes back in st(0)
//     by the ABI: the caller pops what it was told it would get, and the
//     result is not narrowed to 53 bits on the way out. Under 0x027f nothing
//     differs from the precedents' form (a register is already 53-bit there);
//     under the in-step 0x0f7f the root keeps its 64-bit value, as the
//     oracle's does, until the reconstruction's own code stores it.
//   * Why the sums are formed inside. A double argument reaches the helper
//     through a qword, and under 0x0f7f the listing's 64-bit sums (of
//     squares, in the lengths) would be cut to 53 bits there -- a
//     perturbation of 2^-53 relative, far larger than the 2^-64 the rounding
//     mode moves. So every site whose listing adds its operands on the FPU
//     stack and feeds that sum to fsqrt passes the operands instead, and the
//     helper adds (and multiplies) them in the listing's order at the live
//     control word. A factor or addend that is a float, a float product or a
//     constant is exact in a qword; one the reconstruction already holds as a
//     `double` variable (an unrounded sum the listing keeps in a register) is
//     narrowed at the qword pass under 0x0f7f only -- the same narrowing the
//     reconstruction's `double` convention accepts wherever MSVC spills such
//     a value. A subtraction is passed as the addition of a negated operand:
//     IEEE defines a - b as a + (-b), so the two round identically in every
//     mode -- except where a form subtracts itself, as x87FsqrtMulSub and
//     x87FsqrtDiag do.
//
// The forms:
//   x87Fsqrt(x)                     fsqrt(x)
//   x87FsqrtSum2(a, b)              fsqrt(a + b)
//   x87FsqrtSum3(a, b, c)           fsqrt((a + b) + c)
//   x87FsqrtSum4(a, b, c, d)        fsqrt(((a + b) + c) + d)
//   x87FsqrtDiffSum(a, b, c)        fsqrt((a - b) + c)
//   x87FsqrtDiag(a, b, c)           fsqrt((a - (b + c)) + 1)
//   x87FsqrtMulSub(a, b, c)         fsqrt(a b - c)
//   x87FsqrtDot2(a0,b0, a1,b1)                 fsqrt(a0 b0 + a1 b1)
//   x87FsqrtDot3(a0,b0, a1,b1, a2,b2)          fsqrt((a0 b0 + a1 b1) + a2 b2)
//   x87FsqrtDot4(a0,b0, a1,b1, a2,b2, a3,b3)   fsqrt(((a0 b0 + a1 b1) + a2 b2) + a3 b3)
// x87FsqrtDiag is the quaternion-from-matrix diagonal arm
// (`fld b; fadd c; fsubr a; fadd 1.0f; fsqrt`, e.g. 004097 0x960d1-0x960df).
// x87FsqrtDot2 serves Geometry.cpp and D6 004207's swing-lock arms, which
// take the root of two squared differences the listing keeps on the stack
// (0x9dd1e-0x9dd28, 0x9de33-0x9de3d). x87FsqrtMulSub is Geometry.cpp's only.

#include "Nxp.h"

#include <math.h>

#if defined(_MSC_VER) && defined(_M_IX86)

// Arguments start at [esp + 4] (the return address is at [esp]), one qword
// each.

static __declspec(naked) double __cdecl x87Fsqrt(double /*x*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fsqrt
		ret
		}
	}

static __declspec(naked) double __cdecl x87FsqrtSum2(double /*a*/, double /*b*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fadd	qword ptr [esp + 12]
		fsqrt
		ret
		}
	}

static __declspec(naked) double __cdecl x87FsqrtSum3(double /*a*/, double /*b*/, double /*c*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fadd	qword ptr [esp + 12]
		fadd	qword ptr [esp + 20]
		fsqrt
		ret
		}
	}

// fsqrt((a - b) + c): `fld a; fsub b; fadd c`. Passing -b to x87FsqrtSum3
// rounds identically, but fchs flips a NaN's sign, and when b is the NaN the
// x87 keeps, the root then differs in its sign bit (NpActor final review I1).
static __declspec(naked) double __cdecl x87FsqrtDiffSum(double /*a*/, double /*b*/, double /*c*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fsub	qword ptr [esp + 12]
		fadd	qword ptr [esp + 20]
		fsqrt
		ret
		}
	}

static __declspec(naked) double __cdecl x87FsqrtSum4(double /*a*/, double /*b*/, double /*c*/, double /*d*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fadd	qword ptr [esp + 12]
		fadd	qword ptr [esp + 20]
		fadd	qword ptr [esp + 28]
		fsqrt
		ret
		}
	}

static __declspec(naked) double __cdecl x87FsqrtDiag(double /*a*/, double /*b*/, double /*c*/)
	{
	__asm
		{
		fld		qword ptr [esp + 12]
		fadd	qword ptr [esp + 20]
		fsubr	qword ptr [esp + 4]
		fld1
		faddp	st(1), st(0)
		fsqrt
		ret
		}
	}

static __declspec(naked) double __cdecl x87FsqrtMulSub(double /*a*/, double /*b*/, double /*c*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fmul	qword ptr [esp + 12]
		fsub	qword ptr [esp + 20]
		fsqrt
		ret
		}
	}

static __declspec(naked) double __cdecl x87FsqrtDot2(double /*a0*/, double /*b0*/, double /*a1*/, double /*b1*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fmul	qword ptr [esp + 12]
		fld		qword ptr [esp + 20]
		fmul	qword ptr [esp + 28]
		faddp	st(1), st(0)
		fsqrt
		ret
		}
	}

static __declspec(naked) double __cdecl x87FsqrtDot3(double /*a0*/, double /*b0*/, double /*a1*/, double /*b1*/,
	double /*a2*/, double /*b2*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fmul	qword ptr [esp + 12]
		fld		qword ptr [esp + 20]
		fmul	qword ptr [esp + 28]
		faddp	st(1), st(0)
		fld		qword ptr [esp + 36]
		fmul	qword ptr [esp + 44]
		faddp	st(1), st(0)
		fsqrt
		ret
		}
	}

static __declspec(naked) double __cdecl x87FsqrtDot4(double /*a0*/, double /*b0*/, double /*a1*/, double /*b1*/,
	double /*a2*/, double /*b2*/, double /*a3*/, double /*b3*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fmul	qword ptr [esp + 12]
		fld		qword ptr [esp + 20]
		fmul	qword ptr [esp + 28]
		faddp	st(1), st(0)
		fld		qword ptr [esp + 36]
		fmul	qword ptr [esp + 44]
		faddp	st(1), st(0)
		fld		qword ptr [esp + 52]
		fmul	qword ptr [esp + 60]
		faddp	st(1), st(0)
		fsqrt
		ret
		}
	}

#else

static NX_INLINE double x87Fsqrt(double x)
	{
	return sqrt(x);
	}

static NX_INLINE double x87FsqrtSum2(double a, double b)
	{
	return sqrt(a + b);
	}

static NX_INLINE double x87FsqrtSum3(double a, double b, double c)
	{
	return sqrt((a + b) + c);
	}

static NX_INLINE double x87FsqrtDiffSum(double a, double b, double c)
	{
	return sqrt((a - b) + c);
	}

static NX_INLINE double x87FsqrtSum4(double a, double b, double c, double d)
	{
	return sqrt(((a + b) + c) + d);
	}

static NX_INLINE double x87FsqrtDiag(double a, double b, double c)
	{
	return sqrt((a - (b + c)) + 1.0);
	}

static NX_INLINE double x87FsqrtMulSub(double a, double b, double c)
	{
	return sqrt(a * b - c);
	}

static NX_INLINE double x87FsqrtDot2(double a0, double b0, double a1, double b1)
	{
	return sqrt(a0 * b0 + a1 * b1);
	}

static NX_INLINE double x87FsqrtDot3(double a0, double b0, double a1, double b1, double a2, double b2)
	{
	return sqrt((a0 * b0 + a1 * b1) + a2 * b2);
	}

static NX_INLINE double x87FsqrtDot4(double a0, double b0, double a1, double b1, double a2, double b2,
	double a3, double b3)
	{
	return sqrt(((a0 * b0 + a1 * b1) + a2 * b2) + a3 * b3);
	}

#endif

#endif
