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
//
// Added for the body step (BodyStep.cpp, scene-raycast Task 4): the same
// reasoning applied to the other x87 transcendental instructions the step
// rows execute inline, and to the CRT intrinsic they call:
//   x87FsqrtQuotDot3(n, a0,b0, a1,b1, a2,b2)  fsqrt(n / ((a0 b0 + a1 b1) + a2 b2))
//   x87FsinHalfOverNorm3(dt, h, x, y, z)   fsin((dt L) h) / L, L = fsqrt((x x + y y) + z z)
//   x87FcosHalfNorm3(dt, h, x, y, z)       fcos((dt L) h)
//   x87RateOverRoot(a, r, w)        (a r + a r) / fsqrt(1 - w w)
//   x87CIacos(x)                    the oracle CRT's _CIacos (see its note)
//   x87AcosRateOverRoot(w, r)       x87RateOverRoot(x87CIacos(w), r, w), the
//                                   arc cosine kept in st(0)
//
// Where these helpers are not enough. A helper takes its operands through qwords
// and returns through st(0), so a row whose listing keeps a WIDE value both as an
// operand of the root and for use after it (the value is squared into the root,
// then scaled by 1/root) cannot be reproduced under 0x0f7f through them: the
// operand is cut to 53 bits on the way in, and the value itself is held in an
// 8-byte slot across the call. The per-site precedent (convex-mesh gap Task 2b)
// is to write that row's span -- the wide value, the root and its reuse -- as one
// x87 `__asm` block transcribed instruction for instruction from the listing,
// keeping the rest of the row in C++: phys_fn_001760 in ContactBoxMeshICE.cpp
// (0x0003c163..0x0003c21b, 0 differing words where the C++ form had 51). It is a
// per-row judgement of proportion, not a rule: phys_fn_001855 has the same shape
// twice (about 131 instructions) and keeps the C++ form with its measured
// divergence pinned.
//
// Two more uses of the same device, not about square roots (convex-mesh gap harness
// hardening): phys_fn_001694's interior leaf in Distance.cpp, whose wide s MSVC
// spilled to an 8-byte slot (2 words under 0x0f7f on the Task 2b review's draws,
// 0 since), and phys_fn_001712 (NxRayTriIntersect) in Geometry.cpp, written whole
// and naked because which operands its listing loads (quieting a signalling NaN)
// and which it uses from memory decides the NaN it propagates.

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

// fsqrt(n / ((a0 b0 + a1 b1) + a2 b2)): the angular-velocity clamp of the
// body step (000726 0x16757-0x16789: `fdivr [max]; fsqrt` over the squared
// length the listing keeps on the stack). The divisor is summed inside.
static __declspec(naked) double __cdecl x87FsqrtQuotDot3(double /*n*/, double /*a0*/, double /*b0*/,
	double /*a1*/, double /*b1*/, double /*a2*/, double /*b2*/)
	{
	__asm
		{
		fld		qword ptr [esp + 12]
		fmul	qword ptr [esp + 20]
		fld		qword ptr [esp + 28]
		fmul	qword ptr [esp + 36]
		faddp	st(1), st(0)
		fld		qword ptr [esp + 44]
		fmul	qword ptr [esp + 52]
		faddp	st(1), st(0)
		fdivr	qword ptr [esp + 4]
		fsqrt
		ret
		}
	}

// The body step's quaternion integrator (000736 0x16a60-0x16adc): with
// L = fsqrt((x x + y y) + z z) and a = (dt L) half, x87FsinHalfOverNorm3
// returns fsin(a) / L and x87FcosHalfNorm3 returns fcos(a). L and a stay in
// registers in the listing (`fld dt; fmul st(1); fmul [0.5f]; fld st(0);
// fsin; fdiv st(2)` ... `fcos`), so each helper forms them itself rather than
// take them through a qword; the oracle has no CRT sin or cos, only the
// instructions.
static __declspec(naked) double __cdecl x87FsinHalfOverNorm3(double /*dt*/, double /*half*/,
	double /*x*/, double /*y*/, double /*z*/)
	{
	__asm
		{
		fld		qword ptr [esp + 20]
		fmul	qword ptr [esp + 20]
		fld		qword ptr [esp + 28]
		fmul	qword ptr [esp + 28]
		faddp	st(1), st(0)
		fld		qword ptr [esp + 36]
		fmul	qword ptr [esp + 36]
		faddp	st(1), st(0)
		fsqrt
		fld		qword ptr [esp + 4]
		fmul	st(0), st(1)
		fmul	qword ptr [esp + 12]
		fsin
		fdiv	st(0), st(1)
		fstp	st(1)
		ret
		}
	}

static __declspec(naked) double __cdecl x87FcosHalfNorm3(double /*dt*/, double /*half*/,
	double /*x*/, double /*y*/, double /*z*/)
	{
	__asm
		{
		fld		qword ptr [esp + 20]
		fmul	qword ptr [esp + 20]
		fld		qword ptr [esp + 28]
		fmul	qword ptr [esp + 28]
		faddp	st(1), st(0)
		fld		qword ptr [esp + 36]
		fmul	qword ptr [esp + 36]
		faddp	st(1), st(0)
		fsqrt
		fmul	qword ptr [esp + 4]
		fmul	qword ptr [esp + 12]
		fcos
		ret
		}
	}

// (angle invDt + angle invDt) / fsqrt(1 - w w): the kinematic angular
// velocity of the body step (000726 0x164b4-0x164ce: `fmul invDt; fadd
// st(0),st(0)`, then `fld w; fmul w; fsubr [1.0f]; fsqrt; fdivp`). The
// dividend stays in a register across the root in the listing, so it is
// formed here. x87RateOverRoot takes the angle (0 or pi, exact in a qword);
// x87AcosRateOverRoot takes it from x87CIacos(w) (below), the _CIacos
// result the listing keeps in st(0).
static const double gX87One = 1.0;

static __declspec(naked) double __cdecl x87RateOverRoot(double /*angle*/, double /*invDt*/, double /*w*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fmul	qword ptr [esp + 12]
		fadd	st(0), st(0)
		fld		qword ptr [esp + 20]
		fmul	qword ptr [esp + 20]
		fsubr	qword ptr [gX87One]
		fsqrt
		fdivp	st(1), st(0)
		ret
		}
	}

// The x87 indefinite (a quiet NaN) x87CIacos returns for a domain error.
static const unsigned __int64 gX87AcosIndefinite = 0xfff8000000000000ULL;

// The arc cosine as the oracle's CRT intrinsic _CIacos (phys_fn_005697,
// 0x000f47f0, with its body at 0x000f480d and the exits 0x000faa3e /
// 0x000faa4b) computes it; the body step 000726 calls it (0x164af). Not a
// row claim: it stands in for the statically linked CRT routine, which the
// reconstruction's CRT does not share (a UCRT acos follows neither the
// control-word handling nor the fpatan form). What it reproduces:
//   * the control word: saved; when it is not 0x027f, the routine runs
//     under (cw & 0x300) | 0x7f -- the caller's precision, rounding to
//     nearest, all exceptions masked (0x0fa9b5) -- and restores the saved
//     word on the way out (0x0faa46 / 0x0faa70);
//   * |x| < 1 (exponent field below 0x3ff00000): fpatan(sqrt((1 + x)(1 - x)),
//     x), in the listing's order (0x0f4828-0x0f4836);
//   * |x| == 1: 0 or pi (fldz / fldpi, 0x0f4868-0x0f4876); a NaN comes back
//     as itself; |x| > 1 or an infinity gives the indefinite.
// Not reproduced: the error reporting through _87except (0x0fa957), which
// under a word with the precision exception masked the fast path enters
// whenever the status word's PE flag is set, and which for a domain error
// sets errno; it changes no returned value.
static __declspec(naked) double __cdecl x87CIacos(double /*x*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		mov		eax, dword ptr [esp + 8]
		and		eax, 0x7ff00000
		cmp		eax, 0x7ff00000
		je		acosSpecial
		push	edx
		fnstcw	word ptr [esp]
		cmp		word ptr [esp], 0x27f
		je		acosWord
		mov		edx, dword ptr [esp]
		and		edx, 0x300
		or		edx, 0x7f
		mov		word ptr [esp + 2], dx
		fldcw	word ptr [esp + 2]
	acosWord:
		cmp		eax, 0x3ff00000
		jae		acosEdge
		fld1
		fadd	st(0), st(1)
		fld1
		fsub	st(0), st(2)
		fmulp	st(1), st(0)
		fsqrt
		fxch	st(1)
		fpatan
		jmp		acosDone
	acosEdge:
		ja		acosDomain
		mov		eax, dword ptr [esp + 12]
		mov		ecx, eax
		and		eax, 0xfffff
		or		eax, dword ptr [esp + 8]
		jne		acosDomain
		fstp	st(0)
		test	ecx, 0x80000000
		je		acosZero
		fldpi
		jmp		acosDone
	acosZero:
		fldz
		jmp		acosDone
	acosDomain:
		fstp	st(0)
		fld		qword ptr [gX87AcosIndefinite]
	acosDone:
		cmp		word ptr [esp], 0x27f
		je		acosPop
		fldcw	word ptr [esp]
	acosPop:
		pop		edx
		ret
	acosSpecial:
		mov		eax, dword ptr [esp + 8]
		test	eax, 0xfffff
		jne		acosNaN
		cmp		dword ptr [esp + 4], 0
		jne		acosNaN
		fstp	st(0)
		fld		qword ptr [gX87AcosIndefinite]
	acosNaN:
		ret
		}
	}

static __declspec(naked) double __cdecl x87AcosRateOverRoot(double /*w*/, double /*invDt*/)
	{
	__asm
		{
		push	dword ptr [esp + 8]
		push	dword ptr [esp + 8]
		call	x87CIacos
		add		esp, 8
		fmul	qword ptr [esp + 12]
		fadd	st(0), st(0)
		fld		qword ptr [esp + 4]
		fmul	qword ptr [esp + 4]
		fsubr	qword ptr [gX87One]
		fsqrt
		fdivp	st(1), st(0)
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

static NX_INLINE double x87FsqrtQuotDot3(double n, double a0, double b0, double a1, double b1, double a2, double b2)
	{
	return sqrt(n / ((a0 * b0 + a1 * b1) + a2 * b2));
	}

static NX_INLINE double x87FsinHalfOverNorm3(double dt, double half, double x, double y, double z)
	{
	const double length = sqrt((x * x + y * y) + z * z);
	return sin((dt * length) * half) / length;
	}

static NX_INLINE double x87FcosHalfNorm3(double dt, double half, double x, double y, double z)
	{
	const double length = sqrt((x * x + y * y) + z * z);
	return cos((dt * length) * half);
	}

static NX_INLINE double x87RateOverRoot(double angle, double invDt, double w)
	{
	return (angle * invDt + angle * invDt) / sqrt(1.0 - w * w);
	}

static NX_INLINE double x87CIacos(double x)
	{
	return acos(x);
	}

static NX_INLINE double x87AcosRateOverRoot(double w, double invDt)
	{
	return x87RateOverRoot(acos(w), invDt, w);
	}

#endif

#endif
