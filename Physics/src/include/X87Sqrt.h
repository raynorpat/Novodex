#ifndef NX_PHYSICS_X87SQRT
#define NX_PHYSICS_X87SQRT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// x87 square roots for the non-joint /arch:IA32 translation units. Not oracle
// rows.
//
// The same helpers as core/JointX87.h, under non-joint names, plus the two
// forms Geometry.cpp needs that the joints do not (x87FsqrtDot2 and
// x87FsqrtMulSub). core/JointX87.h is left where it is while the joint-family
// tasks still write against it; folding it onto this header is a rename only.
// Read that header for the full argument. In short:
//
//   * `sqrt()` in an /arch:IA32 unit is `call __CIsqrt`, which runs fsqrt
//     under (cw & 0x300) | 0x7f -- round to nearest -- whenever the word is
//     not 0x027f. The oracle has no CRT square root at all (no sqrt import,
//     and the only CRT routines in it that execute fsqrt are _CIasin and
//     _CIacos at 0x000f47f0 and 0x000f48e0); every root it takes is an inline
//     `fsqrt` at the live word. Inside the simulation step that word is 0x0f7f
//     (64-bit precision, round toward zero), and there the two disagree.
//   * The helpers are naked, so they are never inlined and are ordinary cdecl
//     calls that return their double in st(0), unnarrowed. The ContactGeneration
//     and SmoothNormals precedent (`fld; fsqrt; fstp result`) narrows the root
//     to 53 bits on the way out, which the oracle does not do where it keeps
//     the root in a register.
//   * Operands are passed rather than a pre-formed sum. A double argument
//     travels through a qword, so a 64-bit sum formed by the caller would be
//     cut to 53 bits under 0x0f7f before fsqrt saw it. Each helper forms the
//     listing's sum at the live word, in the listing's order. A float operand,
//     or a product of two floats, is exact in a qword; an operand the caller
//     already holds as a `double` variable is narrowed at the pass under 0x0f7f
//     only, the narrowing the `double` convention accepts wherever MSVC spills.
//     A subtraction is passed as the addition of a negated operand (IEEE
//     defines a - b as a + (-b), so the two round identically in every mode),
//     except where a form subtracts itself, as x87FsqrtMulSub does.
//
// The forms:
//   x87Fsqrt(x)                          fsqrt(x)
//   x87FsqrtMulSub(a, b, c)              fsqrt(a b - c)
//   x87FsqrtDot2(a0,b0, a1,b1)           fsqrt(a0 b0 + a1 b1)
//   x87FsqrtDot3(a0,b0, a1,b1, a2,b2)    fsqrt((a0 b0 + a1 b1) + a2 b2)

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

#else

static NX_INLINE double x87Fsqrt(double x)
	{
	return sqrt(x);
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

#endif

#endif
