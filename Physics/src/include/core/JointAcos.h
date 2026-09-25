#ifndef NX_PHYSICS_CORE_JOINTACOS
#define NX_PHYSICS_CORE_JOINTACOS
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The x87 acos the joint rows share: the CRT's _CIacos (0x000f47f0) and the
// inlined NxMath::acos(NxF32) clamp around it. Written for the revolute pilot
// as file-static helpers in core/RevoluteJoint.cpp and moved here by
// joint-families Task 3c, whose spherical rows (004310, 004312) carry the same
// sites (the Global Constraints' rule: acos goes through this reproduction,
// moved to a shared internal header before reuse). Not oracle rows. Include
// only from the x87 (/arch:IA32) joint translation units.

#include "Nxp.h"
#include "NxMath.h"

#include <math.h>

// _CIacos (0x000f47f0), the CRT's x87 acos: argument and result in st(0).
// Its core (0xf4828-0xf4836) is
//     fld1; fadd st,st(1); fld1; fsub st,st(2); fmulp st(1),st; fsqrt;
//     fxch st(1); fpatan
// i.e. atan2(sqrt((1 + x) * (1 - x)), x), computed in that order. Around it
// the CRT saves the control word and, when it is not the default 0x027f,
// runs the core with the precision bits kept, rounding to nearest and all
// exceptions masked ((cw & 0x300) | 0x7f, 0xfa9b5), restoring the saved
// word afterwards (0xfaa70, or 0xfa98e); that is reproduced too. The
// argument passes through `fst qword` (0xf47f3), so it arrives here as a
// double.
// The flag at 0x10128514 is never set, so the exit is always the
// 0xfaa4b path (0xfaa3e is dead): with the default word it returns at
// 0xfaa73; otherwise, when the saved word has PM (bit 5) clear it reloads
// the word at 0xfaa70, and when PM is set and the status word shows PE it
// calls 0xfa957, which stores the result as a qword, runs the CRT
// exception dispatcher (0xffcf5; a masked inexact result passes through)
// and reloads the qword before restoring the word (0xfa98e). Not
// reproduced, and why no value differs under the SDK's control words
// (0x027f, or the in-step word with every exception masked):
// - the 0xfa957 arm only rounds the result to a double, which the helper's
//   own double store below does as well;
// - the |x| >= 1 arms (0xf4855-0xf4878) are unreachable, every call site
//   clamps first (jointAcos);
// - the NaN arm (0xf4881 -> 0xfa9cc) returns the argument quieted, which
//   is also what the core computes for a NaN argument (it propagates
//   through fadd/fsub/fmulp/fsqrt/fpatan unchanged).
// The CRT's acos need not agree with this sequence, which is why it is not
// used. The result is stored as a double (SmoothNormals.cpp's reason: st(0)
// left to the caller is a register the compiler did not put there).
static NX_INLINE double jointCIacos(double x)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	double result;
	NxU16 savedControlWord;
	NxU16 coreControlWord;
	__asm
		{
		fnstcw	savedControlWord
		mov		ax, savedControlWord
		cmp		ax, 0x27f
		je		acosCore
		and		ax, 0x300
		or		ax, 0x7f
		mov		coreControlWord, ax
		fldcw	coreControlWord
	acosCore:
		fld		x
		fld1
		fadd	st(0), st(1)
		fld1
		fsub	st(0), st(2)
		fmulp	st(1), st(0)
		fsqrt
		fxch	st(1)
		fpatan
		fstp	result
		fldcw	savedControlWord
		}
	return result;
#else
	return atan2(sqrt((1.0 + x) * (1.0 - x)), x);
#endif
	}

// The inlined NxMath::acos(NxF32) clamp every acos site carries (revolute
// 004330 0xa8dfe-0xa8e34, 004352 0xa94fd-0xa9530, 004372 0xad015-0xad04b;
// spherical 004310 0xa570a-0xa5740 and 0xa5d86-0xa5dbc, 004312
// 0xa6fd7-0xa700d, whose pi is the unit's own float at 0x10119e10, the same
// value): >= 1 -> 0, <= -1 -> the float pi (0x1011a1b0 in the revolute
// unit), otherwise _CIacos of the float. The result is returned unrounded: 004352 and 004372 keep it on the
// stack; 004330 rounds it where it stores it.
static NX_INLINE double jointAcos(NxReal f)
	{
	if(f >= 1.0f)
		return 0.0f;
	if(f <= -1.0f)
		return NxPiF32;
	return jointCIacos(f);
	}

#endif
