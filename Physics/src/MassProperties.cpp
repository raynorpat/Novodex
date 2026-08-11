/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The exported mass, density and inertia-tensor kernels. All twelve are leaves:
// none of them calls anything, and each is a single straight run of x87
// arithmetic in the oracle.
//
// Why the intermediates are `double`. The oracle evaluates each expression in
// the x87 stack and narrows to 32 bits only where it stores one, so a
// transcription into `NxReal` temporaries rounds in places the oracle does not.
// The process the tests run in reports _control87 == 0x0009001f, whose _MCW_PC
// field is _PC_53: every x87 result is rounded to a 53-bit significand, which
// is what `double` is. Writing the register lifetimes as `double` and the
// stores as `NxReal` therefore reproduces the oracle's rounding exactly, and
// does so in the source rather than by depending on a compiler switch.
//
// Measured, not assumed. NxComputeSphereMass(0x40505969, 0x404c999e) returns
// 0x43e70125 from the shipped DLL; evaluated entirely in float32 the same
// expression gives 0x43e70124. The 299-case Phase 3 matrix contains no input
// that separates the two -- it was a randomized differential against the
// shipped exports, over millions of inputs, that found it and that these rows
// are closed against.

#include "Nxp.h"
#include "NxVec3.h"
#include "NxMath.h"
#include "NxInertiaTensor.h"

#include <string.h>

// The constants the oracle multiplies by, read out of .rdata rather than
// written as expressions, because the stored 32-bit value is what participates
// in the arithmetic.
static const NxReal gFourThirdsPi	= 4.1887903f;	// 0x101068d8
static const NxReal gPi				= 3.1415927f;	// 0x101068d0
static const NxReal gOneThirdPi		= 1.0471976f;	// 0x101068dc
static const NxReal gOneTwelfth		= 0.083333336f;	// 0x101068e0
static const NxReal gTwoFifths		= 0.4f;			// 0x101068e4
static const NxReal gTwoThirds		= 0.6666667f;	// 0x101068e8

// 0x0001bad0, 0x0001bb00, 0x0001bb30 and 0x0001bb70 test each extent with an
// integer compare of its raw word against zero -- `cmp dword ptr [eax], 0` and
// `test ecx, ecx` -- and not with a floating-point comparison. Negative zero
// therefore does not count as zero here: an extent of -0.0f is multiplied in
// and flips the sign of the product. That is shipped behaviour, reproduced
// rather than corrected.
static bool isZeroWord(NxReal value)
	{
	NxU32 word;
	memcpy(&word, &value, sizeof(word));
	return word == 0;
	}

// The shared opening of the box and ellipsoid kernels. The accumulator starts
// at 1.0f and the first non-zero extent replaces it rather than multiplying
// into it: `fld 1.0; cmp dword ptr [eax], 0; je +4; fstp st(0); fld [eax]`.
static double extentProduct(const NxVec3& extents)
	{
	double product = 1.0f;
	if(!isZeroWord(extents.x))
		product = extents.x;
	if(!isZeroWord(extents.y))
		product = product * extents.y;
	if(!isZeroWord(extents.z))
		product = product * extents.z;
	return product;
	}

// 0x0001ba90
NxReal NX_CALL_CONV NxComputeSphereMass(NxReal radius, NxReal density)
	{
	const double r = radius;
	return (NxReal) (r * r * r * density * gFourThirdsPi);
	}

// 0x0001bab0. `fdivr`, so the denominator is accumulated first and the divide
// is the last operation.
NxReal NX_CALL_CONV NxComputeSphereDensity(NxReal radius, NxReal mass)
	{
	const double r = radius;
	return (NxReal) (mass / (r * r * r * gFourThirdsPi));
	}

// 0x0001bad0
NxReal NX_CALL_CONV NxComputeBoxMass(const NxVec3& extents, NxReal density)
	{
	return (NxReal) (extentProduct(extents) * density);
	}

// 0x0001bb00
NxReal NX_CALL_CONV NxComputeBoxDensity(const NxVec3& extents, NxReal mass)
	{
	return (NxReal) (mass / extentProduct(extents));
	}

// 0x0001bb30
NxReal NX_CALL_CONV NxComputeEllipsoidMass(const NxVec3& extents, NxReal density)
	{
	return (NxReal) (extentProduct(extents) * density * gFourThirdsPi);
	}

// 0x0001bb70. The oracle folds the constant into the denominator before the
// `fdivr`, so this is mass / (product * k) and not (mass / product) / k.
NxReal NX_CALL_CONV NxComputeEllipsoidDensity(const NxVec3& extents, NxReal mass)
	{
	return (NxReal) (mass / (extentProduct(extents) * gFourThirdsPi));
	}

// 0x0001bbb0. `fld length; fadd st(0), st(0)` -- the length is doubled, so the
// caller's length is a half-height. The doubling happens before either radius
// multiply, which is the oracle's order and is kept.
NxReal NX_CALL_CONV NxComputeCylinderMass(NxReal radius, NxReal length, NxReal density)
	{
	const double l = length;
	return (NxReal) ((l + l) * radius * radius * density * gPi);
	}

// 0x0001bbd0
NxReal NX_CALL_CONV NxComputeCylinderDensity(NxReal radius, NxReal length, NxReal mass)
	{
	const double l = length;
	return (NxReal) (mass / ((l + l) * radius * radius * gPi));
	}

// 0x0001bbf0. `fabs` on the length rather than a doubling: the cone kernels
// take the magnitude of the length where the cylinder kernels double it.
NxReal NX_CALL_CONV NxComputeConeMass(NxReal radius, NxReal length, NxReal density)
	{
	const double l = NxMath::abs(length);
	return (NxReal) (l * radius * radius * density * gOneThirdPi);
	}

// 0x0001bc10
NxReal NX_CALL_CONV NxComputeConeDensity(NxReal radius, NxReal length, NxReal mass)
	{
	const double l = NxMath::abs(length);
	return (NxReal) (mass / (l * radius * radius * gOneThirdPi));
	}

// 0x0001bc30. Two details are load-bearing and neither is visible in the
// arithmetic:
//
//   * the components are written x, y, z, and the pair each one sums is the
//     pair of the other two axes;
//   * `fst dword ptr [esp + 0x10]` at 0x0001bc5e spills x*x through a 32-bit
//     stack slot, and the z component reads it back at 0x0001bc6b while the y
//     component uses the register copy. z therefore sees x*x rounded to single
//     precision and y does not. `squaredX` is that spill.
void NX_CALL_CONV NxComputeBoxInertiaTensor(NxVec3& diagInertia, NxReal mass,
	NxReal xlength, NxReal ylength, NxReal zlength)
	{
	const double x = xlength, y = ylength, z = zlength;
	const double massOverTwelve = mass * (double) gOneTwelfth;

	diagInertia.x = (NxReal) ((z * z + y * y) * massOverTwelve);

	const NxReal squaredX = (NxReal) (x * x);
	diagInertia.y = (NxReal) ((x * x + z * z) * massOverTwelve);
	diagInertia.z = (NxReal) ((squaredX + y * y) * massOverTwelve);
	}

// 0x0001bc80. The oracle stores the unscaled mass*r*r into diagInertia.x
// before it branches (`fld st(0); fstp dword ptr [eax]`) and then overwrites
// it with the scaled value, so a caller that aliases the output can observe
// the intermediate. The scale multiplies the register copy and not the value
// that was just stored, so the first store's rounding does not feed the
// result. y and z are then copied from x after it has been narrowed.
void NX_CALL_CONV NxComputeSphereInertiaTensor(NxVec3& diagInertia, NxReal mass,
	NxReal radius, bool hollow)
	{
	const double massRadiusSquared = (double) mass * radius * radius;

	diagInertia.x = (NxReal) massRadiusSquared;
	if(hollow)
		diagInertia.x = (NxReal) (massRadiusSquared * gTwoThirds);
	else
		diagInertia.x = (NxReal) (massRadiusSquared * gTwoFifths);
	diagInertia.y = diagInertia.x;
	diagInertia.z = diagInertia.x;
	}
