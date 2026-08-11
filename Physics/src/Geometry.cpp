/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The exported ray and segment intersection kernels. All of them are leaves.
//
// Two conventions run through this file and both come from the oracle's x87
// code rather than from taste:
//
//   * a value the oracle keeps in an x87 register is `double` here, and a
//     value it stores to a 32-bit slot is `NxReal`. The process runs with
//     _control87 == 0x0009001f, whose precision control is _PC_53, so an x87
//     register holds exactly what a `double` holds. Transcribing everything as
//     `NxReal` would round in places the oracle does not, and the difference is
//     one ulp on ordinary inputs -- measured, not theoretical.
//   * the file is built /arch:IA32 (see CMakeLists.txt). The `double` typing
//     covers the precision; the code generator is needed for the other half,
//     which is that x87 propagates the NaN with the larger significand where
//     SSE propagates the first source operand.
//
// Several of these kernels narrow one component of a vector result and leave
// the other two at register precision, or store an output twice, or test a
// sign bit as an integer rather than comparing against zero. Those are not
// tidy and they are not mistakes in the transcription: each one is named at
// the site and each is shipped behaviour.

#include "Nxp.h"
#include "NxVec3.h"
#include "NxRay.h"
#include "NxPlane.h"
#include "NxIntersectionRayPlane.h"
#include "NxIntersectionRaySphere.h"
#include "NxIntersectionRayTriangle.h"

#include <math.h>
#include <string.h>

// 0x10107880 and 0x10107888, and they are qwords: the plane kernels compare
// against a *double* 1e-7, so the comparison happens at register precision and
// not after a narrowing to 32 bits.
static const double gPlaneParallelEpsilon = 1e-7;

// 0x10106880 and 0x101079e8, dwords.
static const NxReal gTriangleEpsilon = 1e-6f;

// `test eax, eax; js` on the word that was just stored. This is an integer
// test of the sign bit and not a comparison against zero, so a stored -0.0f
// fails it where `value < 0.0f` would not.
static bool storedSignBitSet(NxReal value)
	{
	NxI32 word;
	memcpy(&word, &value, sizeof(word));
	return word < 0;
	}

// 0x00036bb0. The dot product stays in a register and is compared against the
// double epsilon on both sides; a NaN fails both comparisons and so takes the
// hit path, which is what the C++ short-circuit does too.
bool NX_CALL_CONV NxRayPlaneIntersect(const NxRay& ray, const NxPlane& plane,
	NxReal& dist, NxVec3& pointOnPlane)
	{
	const double denominator = (ray.dir.y * (double) plane.normal.y
		+ ray.dir.z * (double) plane.normal.z) + plane.normal.x * (double) ray.dir.x;

	if(denominator > -gPlaneParallelEpsilon && denominator < gPlaneParallelEpsilon)
		return false;

	const double numerator = ((ray.orig.y * (double) plane.normal.y
		+ ray.orig.x * (double) plane.normal.x) + plane.normal.z * (double) ray.orig.z)
		+ plane.d;
	const double t = -(numerator / denominator);

	dist = (NxReal) t;

	// x keeps the register copy of t*dir.x; y and z go through 32-bit slots at
	// 0x00036c25 and 0x00036c2e before the origin is added.
	const NxReal alongY = (NxReal) (t * ray.dir.y);
	const NxReal alongZ = (NxReal) (t * ray.dir.z);
	pointOnPlane.x = (NxReal) (t * ray.dir.x + ray.orig.x);
	pointOnPlane.y = (NxReal) (alongY + (double) ray.orig.y);
	pointOnPlane.z = (NxReal) (alongZ + (double) ray.orig.z);
	return true;
	}

// 0x00036c60. Returns void: the parallel case writes v1 into pointOnPlane with
// three integer moves and leaves dist untouched, so a caller cannot tell a
// parallel segment from a hit by looking at dist alone.
void NX_CALL_CONV NxSegmentPlaneIntersect(const NxVec3& v1, const NxVec3& v2,
	const NxPlane& plane, NxReal& dist, NxVec3& pointOnPlane)
	{
	// The x component of the direction is never stored, so it stays at
	// register precision through the normalize and into the dot product. y and
	// z are stored at 0x00036c75 and 0x00036c7f and read back from there.
	double directionX = v2.x - (double) v1.x;
	NxReal directionY = v2.y - v1.y;
	const double directionZRegister = v2.z - (double) v1.z;
	NxReal directionZ = (NxReal) directionZRegister;

	// 0x00036c7f is `fst`, not `fstp`: z is stored and then squared against
	// the register copy, so this one term is the extended value times the
	// narrowed one. y is loaded back twice and x never leaves the stack.
	const double length = sqrt((directionZRegister * directionZ
		+ directionY * (double) directionY) + directionX * directionX);

	if(length != 0.0)
		{
		const double inverseLength = 1.0 / length;
		directionX = directionX * inverseLength;
		directionY = (NxReal) (directionY * inverseLength);
		directionZ = (NxReal) (directionZ * inverseLength);
		}

	const double denominator = (directionZ * (double) plane.normal.z
		+ directionY * (double) plane.normal.y) + directionX * plane.normal.x;

	if(denominator > -gPlaneParallelEpsilon && denominator < gPlaneParallelEpsilon)
		{
		// 0x00036d02: three integer moves, and no write to dist.
		pointOnPlane.x = v1.x;
		pointOnPlane.y = v1.y;
		pointOnPlane.z = v1.z;
		return;
		}

	const double numerator = ((v1.z * (double) plane.normal.z
		+ plane.normal.y * (double) v1.y) + v1.x * (double) plane.normal.x)
		+ plane.d;

	const NxReal t = (NxReal) (-(numerator / denominator));
	dist = t;

	// All three products use the narrowed t, but only x still has its
	// direction component at register precision.
	const NxReal alongY = (NxReal) (directionY * (double) t);
	const NxReal alongZ = (NxReal) (directionZ * (double) t);
	pointOnPlane.x = (NxReal) (directionX * t + v1.x);
	pointOnPlane.y = (NxReal) (alongY + (double) v1.y);
	pointOnPlane.z = (NxReal) (alongZ + (double) v1.z);
	}

// 0x00036e80. Both the projection onto the ray and the discriminant are
// narrowed to 32 bits before they are used again, at 0x00036eb1 and
// 0x00036ed9, so the square and the square root see the rounded values.
bool NX_CALL_CONV NxRaySphereIntersect(const NxVec3& origin, const NxVec3& dir,
	const NxVec3& center, NxReal radius, NxVec3* coord)
	{
	const double toCentreX = center.x - (double) origin.x;
	const double toCentreY = center.y - (double) origin.y;
	const double toCentreZ = center.z - (double) origin.z;

	const NxReal alongRay = (NxReal) ((toCentreZ * dir.z + toCentreY * dir.y)
		+ toCentreX * dir.x);

	const NxReal discriminant = (NxReal) (radius * (double) radius
		- (((toCentreZ * toCentreZ + toCentreX * toCentreX) + toCentreY * toCentreY)
			- alongRay * (double) alongRay));

	if(discriminant < 0.0f)
		return false;

	if(coord)
		{
		const double t = alongRay - sqrt((double) discriminant);

		// z is written first, and it is the one component whose product with
		// the direction is not narrowed before the origin is added.
		const NxReal alongX = (NxReal) (t * dir.x);
		const NxReal alongY = (NxReal) (t * dir.y);
		coord->z = (NxReal) (t * dir.z + origin.z);
		coord->x = (NxReal) (alongX + (double) origin.x);
		coord->y = (NxReal) (alongY + (double) origin.y);
		}
	return true;
	}

// 0x00036f50. Moller-Trumbore, with the culled and non-culled paths written
// out separately in the oracle rather than sharing a tail. They are not the
// same code with a different epsilon:
//
//   * the culled path stores the unscaled u and v, tests them, and only then
//     multiplies all three outputs by 1/det -- so a caller that stops the
//     function early sees unscaled barycentrics in u and v;
//   * the non-culled path scales u and v as it computes them and tests
//     against 1.0;
//   * the two paths sum the three terms of v in a different order, at
//     0x000370a9 and 0x000371f5.
//
// The u and v range tests are `test eax, eax; js` on the word just stored,
// which is a sign-bit test and not `< 0`. u's upper test uses the register copy
// (`fcomp dword ptr [esp + 0x40]` at 0x0003705a); it is v's that reads u back
// out of the caller's float (`fadd dword ptr [ecx]` at 0x000370d3), so an
// aliasing caller changes the second test and not the first.
bool NX_CALL_CONV NxRayTriIntersect(const NxVec3& orig, const NxVec3& dir,
	const NxVec3& vert0, const NxVec3& vert1, const NxVec3& vert2,
	float& t, float& u, float& v, bool cull)
	{
	const NxReal edge1X = vert1.x - vert0.x;
	const NxReal edge1Y = vert1.y - vert0.y;
	const NxReal edge1Z = vert1.z - vert0.z;
	const NxReal edge2X = vert2.x - vert0.x;
	const NxReal edge2Y = vert2.y - vert0.y;
	const NxReal edge2Z = vert2.z - vert0.z;

	// pvec = dir x edge2. Its z component is stored with `fst` and not
	// `fstp` at 0x00036fd6, so the determinant below multiplies the register
	// copy while everything after it reads the narrowed one.
	const NxReal pvecX = (NxReal) (edge2Z * (double) dir.y - edge2Y * (double) dir.z);
	const NxReal pvecY = (NxReal) (edge2X * (double) dir.z - edge2Z * (double) dir.x);
	const double pvecZ = edge2Y * (double) dir.x - edge2X * (double) dir.y;
	const NxReal pvecZStored = (NxReal) pvecZ;

	const NxReal det = (NxReal) ((pvecZ * edge1Z + pvecY * (double) edge1Y)
		+ pvecX * (double) edge1X);

	if(cull)
		{
		if(det < gTriangleEpsilon)
			return false;

		const NxReal tvecX = orig.x - vert0.x;
		const NxReal tvecY = orig.y - vert0.y;
		const double tvecZ = orig.z - (double) vert0.z;
		const NxReal tvecZStored = (NxReal) tvecZ;

		const double uRaw = (tvecZ * pvecZStored + tvecY * (double) pvecY)
			+ tvecX * (double) pvecX;
		u = (NxReal) uRaw;
		if(storedSignBitSet(u) || uRaw > det)
			return false;

		const NxReal qvecX = (NxReal) (tvecY * (double) edge1Z - tvecZStored * (double) edge1Y);
		const NxReal qvecY = (NxReal) (tvecZStored * (double) edge1X - edge1Z * (double) tvecX);
		const NxReal qvecZ = (NxReal) (tvecX * (double) edge1Y - tvecY * (double) edge1X);

		const double vRaw = (qvecY * (double) dir.y + qvecZ * (double) dir.z)
			+ qvecX * (double) dir.x;
		v = (NxReal) vRaw;
		if(storedSignBitSet(v) || vRaw + u > det)
			return false;

		const double inverseDet = 1.0 / det;
		t = (NxReal) (((qvecZ * (double) edge2Z + qvecY * (double) edge2Y)
			+ qvecX * (double) edge2X) * inverseDet);
		u = (NxReal) (inverseDet * u);
		v = (NxReal) (inverseDet * v);
		return true;
		}

	if(det > -gTriangleEpsilon && det < gTriangleEpsilon)
		return false;

	// 0x00037152 narrows 1/det before anything uses it, where the culled path
	// keeps it in a register.
	const NxReal inverseDet = (NxReal) (1.0 / det);

	const NxReal tvecX = orig.x - vert0.x;
	const NxReal tvecY = orig.y - vert0.y;
	const double tvecZ = orig.z - (double) vert0.z;
	const NxReal tvecZStored = (NxReal) tvecZ;

	const double uRaw = ((tvecZ * pvecZStored + tvecY * (double) pvecY)
		+ tvecX * (double) pvecX) * inverseDet;
	u = (NxReal) uRaw;
	if(storedSignBitSet(u) || uRaw > 1.0f)
		return false;

	const NxReal qvecX = (NxReal) (tvecY * (double) edge1Z - tvecZStored * (double) edge1Y);
	const NxReal qvecY = (NxReal) (tvecZStored * (double) edge1X - edge1Z * (double) tvecX);
	const NxReal qvecZ = (NxReal) (tvecX * (double) edge1Y - tvecY * (double) edge1X);

	const double vRaw = ((qvecY * (double) dir.y + qvecX * (double) dir.x)
		+ qvecZ * (double) dir.z) * inverseDet;
	v = (NxReal) vRaw;
	if(storedSignBitSet(v) || vRaw + u > 1.0f)
		return false;

	t = (NxReal) (((qvecZ * (double) edge2Z + qvecY * (double) edge2Y)
		+ qvecX * (double) edge2X) * inverseDet);
	return true;
	}
