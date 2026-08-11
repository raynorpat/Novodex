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
#include "NxIntersectionSegmentBox.h"
#include "NxIntersectionBoxBox.h"
#include "NxIntersectionSegmentCapsule.h"
#include "NxIntersectionSweptSpheres.h"

#include <float.h>
#include <math.h>
#include <string.h>

// 0x10107880 and 0x10107888, and they are qwords: the plane kernels compare
// against a *double* 1e-7, so the comparison happens at register precision and
// not after a narrowing to 32 bits.
static const double gPlaneParallelEpsilon = 1e-7;

// 0x10106880 and 0x101079e8, dwords.
static const NxReal gTriangleEpsilon = 1e-6f;

// 0x10107a08, a dword. The ray/box slab tests widen this *float* into an x87
// register and never narrow the sum, so the comparison is against the float's
// exact value promoted to double -- not against the double literal 1e-5, which
// is a different number.
static const NxReal gRayBoxEpsilon = 1e-5f;

// 0x10107a30 and 0x10107a0c, both dwords. The first is 1.0f - FLT_EPSILON and
// decides when the ray counts as running along the capsule axis; the second
// is FLT_EPSILON and rejects a degenerate transformed direction.
static const NxReal gCapsuleAxialThreshold = 0.99999988f;
static const NxReal gCapsuleDegenerateEpsilon = 1.1920928955078125e-07f;

// `test eax, eax` on the raw word of a direction component. Zero skips the
// division; -0.0f does not, because its word is 0x80000000 and not zero. A
// reconstruction written as `dir[i] != 0.0f` divides in one fewer case than the
// oracle and leaves MaxT at -1.0f where the oracle puts an infinity there.
static bool storedWordIsZero(NxReal value)
	{
	NxU32 word;
	memcpy(&word, &value, sizeof(word));
	return word == 0;
	}

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

// 0x00037c80 and 0x00037e70. The Woo candidate-plane ray/box kernel, twice.
//
// The inventory sizes these two at 317 bytes each, which is the first chunk
// only: both continue past a three-byte alignment pad into a 16-byte-aligned
// second loop, and both out-line the z axis's upper branch. The real extents
// are 0x00037c80..0x00037e64 and 0x00037e70..0x00038047.
//
// Three narrowings decide the result and none of them is where a naive
// transcription would put it. Each candidate distance is
// `round32(round53(plane - origin) / dir)`, a genuine triple rounding rather
// than a float expression. The two epsilon comparands are never narrowed, so
// the slab test compares a double against a float. And the intersection
// coordinate is stored and then compared *as the stored float*, at 0x00037dd1
// and 0x00037dd5, even though the register copy is still live.
//
// The first phase is unrolled three times and, unusually for this file, the
// three copies really are identical: same order, same narrowing point.
static bool rayAABBCandidatePlanes(const NxVec3& min, const NxVec3& max,
	const NxVec3& origin, const NxVec3& dir, NxVec3& coord, NxReal* maxT, NxU32& whichPlane)
	{
	const NxReal* minimum = &min.x;
	const NxReal* maximum = &max.x;
	const NxReal* from = &origin.x;
	const NxReal* along = &dir.x;
	NxReal* out = &coord.x;

	// Stored in the order 2, 1, 0 at 0x00037ca4..0x00037cb4.
	maxT[2] = -1.0f;
	maxT[1] = -1.0f;
	maxT[0] = -1.0f;

	bool inside = true;
	for(NxU32 axis = 0; axis < 3; ++axis)
		{
		// A NaN on either side fails both comparisons and the axis counts as
		// inside, which is what the two `jp`/`jne` branches do.
		if(from[axis] < minimum[axis])
			{
			out[axis] = minimum[axis];
			inside = false;
			if(!storedWordIsZero(along[axis]))
				maxT[axis] = (NxReal) ((minimum[axis] - (double) from[axis]) / (double) along[axis]);
			}
		else if(from[axis] > maximum[axis])
			{
			out[axis] = maximum[axis];
			inside = false;
			if(!storedWordIsZero(along[axis]))
				maxT[axis] = (NxReal) ((maximum[axis] - (double) from[axis]) / (double) along[axis]);
			}
		}

	if(inside)
		return true;

	// Strictly greater, so a NaN never takes the plane: 0x00037d7f and
	// 0x00037d9a are both `jne` on `test ah, 0x41`.
	whichPlane = 0;
	if(maxT[1] > maxT[0])
		whichPlane = 1;
	if(maxT[2] > maxT[whichPlane])
		whichPlane = 2;
	return false;
	}

// The second loop, shared by both exports. Returns false on the two slab
// rejections. `coord[axis]` is written at 0x00037dd5 *before* the tests that
// can reject it, so a rejected call leaves the offending component behind.
static bool rayAABBSlabs(const NxVec3& min, const NxVec3& max, const NxVec3& origin,
	const NxVec3& dir, NxVec3& coord, const NxReal* maxT, NxU32 whichPlane)
	{
	const NxReal* minimum = &min.x;
	const NxReal* maximum = &max.x;
	const NxReal* from = &origin.x;
	const NxReal* along = &dir.x;
	NxReal* out = &coord.x;

	for(NxU32 axis = 0; axis < 3; ++axis)
		{
		if(axis == whichPlane)
			continue;

		// fmul is exact here -- two 24-bit significands land in 48 bits -- and
		// the add is the rounding. The sum is then narrowed once and both
		// comparisons read the narrowed word.
		const NxReal point = (NxReal) (along[axis] * (double) maxT[whichPlane] + (double) from[axis]);
		out[axis] = point;

		// The oracle compares the widened epsilon-adjusted bound against the
		// stored point in this order, not the point against the bound. For
		// ordered values the two agree; both let a NaN through.
		if(minimum[axis] - (double) gRayBoxEpsilon > (double) point)
			return false;
		if(maximum[axis] + (double) gRayBoxEpsilon < (double) point)
			return false;
		}
	return true;
	}

// 0x00037c80. Returns the entry point in `coord`, and copies the origin into it
// when the origin is already inside the box.
bool NX_CALL_CONV NxRayAABBIntersect(const NxVec3& min, const NxVec3& max,
	const NxVec3& origin, const NxVec3& dir, NxVec3& coord)
	{
	NxReal maxT[3];
	NxU32 whichPlane = 0;

	if(rayAABBCandidatePlanes(min, max, origin, dir, coord, maxT, whichPlane))
		{
		// 0x00037e41..0x00037e4f, three integer moves in x, y, z order.
		coord.x = origin.x;
		coord.y = origin.y;
		coord.z = origin.z;
		return true;
		}

	// 0x00037da8 tests the sign bit of the stored word rather than comparing
	// against zero, so a candidate distance that flushed to -0.0f on the way
	// into its 32-bit slot is rejected here where `< 0.0f` would accept it.
	if(storedSignBitSet(maxT[whichPlane]))
		return false;

	return rayAABBSlabs(min, max, origin, dir, coord, maxT, whichPlane);
	}

// 0x00037e70. The same kernel, reporting which plane was entered and the
// distance to it.
//
// Two things differ from NxRayAABBIntersect and both are shipped behaviour. An
// origin inside the box returns 0 -- the same value as a clean miss -- and
// writes neither output, where NxRayAABBIntersect returns true and fills
// `coord` with the origin. And the return is the plane index plus one, so a
// caller gets 1, 2 or 3 for a hit on x, y or z and cannot distinguish the
// near face from the far one.
NxU32 NX_CALL_CONV NxRayAABBIntersect2(const NxVec3& min, const NxVec3& max,
	const NxVec3& origin, const NxVec3& dir, NxVec3& coord, NxReal& t)
	{
	NxReal maxT[3];
	NxU32 whichPlane = 0;

	if(rayAABBCandidatePlanes(min, max, origin, dir, coord, maxT, whichPlane))
		return 0;

	if(storedSignBitSet(maxT[whichPlane]))
		return 0;

	if(!rayAABBSlabs(min, max, origin, dir, coord, maxT, whichPlane))
		return 0;

	// 0x00038008, a raw dword move after the callee-saved registers are
	// restored. `coord[whichPlane]` is the candidate plane and is not
	// recomputed from t, so the two are consistent only for the other two axes.
	t = maxT[whichPlane];
	return whichPlane + 1;
	}

// 0x00036690. The fifteen-axis separating axis test for two oriented boxes.
//
// NxMat33 holds its nine floats row-major as its only member, so reading it as
// a flat array is what the oracle does: it indexes `rotation0` at 4*i and pairs
// offsets i, i+3, i+6 with the same triple of `rotation1`, which is the column
// of one dotted with the column of the other -- R = rotation0^T * rotation1.
//
// The translation is the trap. `center1 - center0` is never stored: all three
// components live in x87 registers from 0x0003671f to 0x0003679b and feed the
// three dot products at register precision. Writing `NxVec3 t = center1 -
// center0;` rounds three values the oracle does not round, and every one of the
// fifteen axes then differs in the last bit.
//
// Two more asymmetries are transcribed rather than tidied. T[0] associates its
// terms (x + z) + y where T[1] and T[2] associate (z + y) + x. And the first
// face loop sums its radius in z, x, y order where the second -- structurally
// its mirror -- sums in z, y, x. Both are real one-ulp differences.
//
// There is no epsilon anywhere in this function. A degenerate cross product
// gets no guard, so nearly-parallel boxes can report separated on float noise.
// That is shipped and is deliberately not fixed here.
bool NX_CALL_CONV NxBoxBoxIntersect(const NxVec3& extents0, const NxVec3& center0,
	const NxMat33& rotation0, const NxVec3& extents1, const NxVec3& center1,
	const NxMat33& rotation1, bool fullTest)
	{
	const NxReal* a = (const NxReal*) &rotation0;
	const NxReal* b = (const NxReal*) &rotation1;
	const NxReal* e0 = &extents0.x;
	const NxReal* e1 = &extents1.x;

	const double tx = center1.x - (double) center0.x;
	const double ty = center1.y - (double) center0.y;
	const double tz = center1.z - (double) center0.z;

	NxReal t[3];
	t[0] = (NxReal) ((tx * a[0] + tz * a[6]) + ty * a[3]);
	t[1] = (NxReal) ((a[7] * tz + a[4] * ty) + a[1] * tx);
	t[2] = (NxReal) ((a[8] * tz + a[5] * ty) + a[2] * tx);

	NxReal r[3][3];
	for(NxU32 j = 0; j < 3; ++j)
		{
		r[j][0] = (NxReal) ((b[6] * (double) a[j + 6] + b[3] * (double) a[j + 3]) + b[0] * (double) a[j]);
		r[j][1] = (NxReal) ((b[7] * (double) a[j + 6] + b[4] * (double) a[j + 3]) + b[1] * (double) a[j]);
		r[j][2] = (NxReal) ((b[8] * (double) a[j + 6] + b[5] * (double) a[j + 3]) + b[2] * (double) a[j]);
		}

	// 0x00036834. Every rejection is `radius < separation` strictly, so exact
	// contact counts as intersecting and an unordered comparison passes. A NaN
	// anywhere therefore makes this function return true, on every path.
	for(NxU32 i = 0; i < 3; ++i)
		{
		const double separation = fabs((double) t[i]);
		const double radius = ((fabs((double) r[i][2]) * e1[2]
			+ fabs((double) r[i][0]) * e1[0]) + fabs((double) r[i][1]) * e1[1]) + e0[i];
		if(radius < separation)
			return false;
		}

	// 0x00036884. The mirror of the loop above, except that the separation is a
	// live double dot product rather than the absolute value of a stored float,
	// and the radius sums z, y, x rather than z, x, y.
	for(NxU32 k = 0; k < 3; ++k)
		{
		const double separation = fabs((t[2] * (double) r[2][k] + t[1] * (double) r[1][k])
			+ t[0] * (double) r[0][k]);
		const double radius = ((fabs((double) r[2][k]) * e0[2]
			+ fabs((double) r[1][k]) * e0[1]) + fabs((double) r[0][k]) * e0[0]) + e1[k];
		if(radius < separation)
			return false;
		}

	// 0x000368e6, tested once. Clearing it skips the nine cross axes entirely,
	// and with them the whole absolute-value matrix below.
	if(!fullTest)
		return true;

	const NxReal aR00 = (NxReal) fabs((double) r[0][0]);
	const NxReal aR01 = (NxReal) fabs((double) r[0][1]);
	const NxReal aR02 = (NxReal) fabs((double) r[0][2]);
	const NxReal aR10 = (NxReal) fabs((double) r[1][0]);
	const NxReal aR11 = (NxReal) fabs((double) r[1][1]);
	const NxReal aR12 = (NxReal) fabs((double) r[1][2]);
	const NxReal aR20 = (NxReal) fabs((double) r[2][0]);
	const NxReal aR21 = (NxReal) fabs((double) r[2][1]);
	const NxReal aR22 = (NxReal) fabs((double) r[2][2]);

	// The nine cross axes, in the order A0xB0 .. A2xB2. The grouping of each
	// radius is the oracle's and not an algebraic rearrangement of it.
	if((aR20 * (double) e0[1] + aR10 * (double) e0[2])
		+ (aR02 * (double) e1[1] + aR01 * (double) e1[2])
		< fabs(t[2] * (double) r[1][0] - t[1] * (double) r[2][0]))
		return false;
	if((aR02 * (double) e1[0] + aR00 * (double) e1[2])
		+ (aR21 * (double) e0[1] + aR11 * (double) e0[2])
		< fabs(t[2] * (double) r[1][1] - t[1] * (double) r[2][1]))
		return false;
	if((aR22 * (double) e0[1] + aR12 * (double) e0[2])
		+ (aR01 * (double) e1[0] + aR00 * (double) e1[1])
		< fabs(t[2] * (double) r[1][2] - t[1] * (double) r[2][2]))
		return false;
	if((aR20 * (double) e0[0] + aR00 * (double) e0[2])
		+ (aR12 * (double) e1[1] + aR11 * (double) e1[2])
		< fabs(t[0] * (double) r[2][0] - t[2] * (double) r[0][0]))
		return false;
	if((aR12 * (double) e1[0] + aR10 * (double) e1[2])
		+ (aR21 * (double) e0[0] + aR01 * (double) e0[2])
		< fabs(t[0] * (double) r[2][1] - t[2] * (double) r[0][1]))
		return false;
	if((aR22 * (double) e0[0] + aR02 * (double) e0[2])
		+ (aR11 * (double) e1[0] + aR10 * (double) e1[1])
		< fabs(t[0] * (double) r[2][2] - t[2] * (double) r[0][2]))
		return false;
	if((aR22 * (double) e1[1] + aR21 * (double) e1[2])
		+ (aR00 * (double) e0[1] + aR10 * (double) e0[0])
		< fabs(t[1] * (double) r[0][0] - t[0] * (double) r[1][0]))
		return false;
	if((aR22 * (double) e1[0] + aR20 * (double) e1[2])
		+ (aR01 * (double) e0[1] + aR11 * (double) e0[0])
		< fabs(t[1] * (double) r[0][1] - t[0] * (double) r[1][1]))
		return false;
	if((aR02 * (double) e0[1] + aR12 * (double) e0[0])
		+ (aR21 * (double) e1[0] + aR20 * (double) e1[1])
		< fabs(t[1] * (double) r[0][2] - t[0] * (double) r[1][2]))
		return false;

	return true;
	}

// 0x00037ab0. Segment against an axis-aligned box by the separating axis
// theorem: three box axes and three cross products of the segment direction
// with them.
//
// The half-sums are fused. The oracle forms `((p0 + p1) - (min + max)) * 0.5`
// as one 53-bit expression and narrows once, rather than taking two midpoints
// and subtracting them, so a reconstruction that computes the box centre
// separately is a rounding out on every axis. That is the highest-risk detail
// here, and this export writes nothing at all, so the case matrix has no
// resolution on it: it was closed by the randomized differential.
//
// The z axis adds its box bounds in the opposite order to x and y -- `max + min`
// at 0x00037b9d rather than `min + max`. That is commutative for every ordered
// input and is transcribed anyway, because it is not commutative for the x87
// choice between two NaN payloads.
bool NX_CALL_CONV NxSegmentAABBIntersect(const NxVec3& p0, const NxVec3& p1,
	const NxVec3& min, const NxVec3& max)
	{
	const NxReal dx = (NxReal) ((p1.x - (double) p0.x) * 0.5f);
	const NxReal ex = (NxReal) ((max.x - (double) min.x) * 0.5f);
	const NxReal mx = (NxReal) (((p0.x + (double) p1.x) - (min.x + (double) max.x)) * 0.5f);
	const NxReal adx = (NxReal) fabs((double) dx);
	if((double) adx + (double) ex < fabs((double) mx))
		return false;

	const NxReal dy = (NxReal) ((p1.y - (double) p0.y) * 0.5f);
	const NxReal ey = (NxReal) ((max.y - (double) min.y) * 0.5f);
	const NxReal my = (NxReal) (((p0.y + (double) p1.y) - (min.y + (double) max.y)) * 0.5f);
	const NxReal ady = (NxReal) fabs((double) dy);
	if((double) ady + (double) ey < fabs((double) my))
		return false;

	const NxReal dz = (NxReal) ((p1.z - (double) p0.z) * 0.5f);
	const NxReal ez = (NxReal) ((max.z - (double) min.z) * 0.5f);
	const NxReal mz = (NxReal) (((p0.z + (double) p1.z) - (max.z + (double) min.z)) * 0.5f);
	const NxReal adz = (NxReal) fabs((double) dz);
	if((double) adz + (double) ez < fabs((double) mz))
		return false;

	// The three cross axes. Every comparison is `radius < separation` strictly
	// and ordered, so a NaN passes: an all-NaN call returns true.
	if(ez * (double) ady + adz * (double) ey < fabs(mz * (double) dy - dz * (double) my))
		return false;
	if(ez * (double) adx + adz * (double) ex < fabs(dz * (double) mx - mz * (double) dx))
		return false;
	if(ey * (double) adx + ady * (double) ex < fabs(my * (double) dx - dy * (double) mx))
		return false;
	return true;
	}

// 0x00037260. Woo's candidate-plane box kernel, extended to a segment by
// classifying both endpoints into six-bit quadrant masks and rejecting when
// they share a set bit.
//
// Two things drive the reconstruction. The candidate plane is written into the
// caller's `intercept` and then *read back out of it* to form the numerator at
// 0x000373aa, so the function is sensitive to a caller that aliases `intercept`
// with any of its inputs; the store and the reload are both reproduced rather
// than kept in a register. And each block writes all three components before it
// tests any of them, so a `false` return that reached a block leaves a point on
// a rejected plane behind in the caller's vector.
//
// Only the trivial `q1 & q2` rejection leaves `intercept` untouched.
bool NX_CALL_CONV NxSegmentBoxIntersect(const NxVec3& p1, const NxVec3& p2,
	const NxVec3& bbox_min, const NxVec3& bbox_max, NxVec3& intercept)
	{
	// The upper test comes first, so on an inverted box a coordinate that is
	// both above max and below min is classified as above max. A NaN satisfies
	// neither and is classified as inside.
	NxU32 q1 = 0;
	if(p1.x > bbox_max.x)      q1 = 1;
	else if(p1.x < bbox_min.x) q1 = 2;
	if(p1.y > bbox_max.y)      q1 |= 4;
	else if(p1.y < bbox_min.y) q1 |= 8;
	if(p1.z > bbox_max.z)      q1 |= 0x20;
	else if(p1.z < bbox_min.z) q1 |= 0x10;

	if(q1 == 0)
		{
		intercept.x = p1.x;
		intercept.y = p1.y;
		intercept.z = p1.z;
		return true;
		}

	NxU32 q2 = 0;
	if(p2.x > bbox_max.x)      q2 = 1;
	else if(p2.x < bbox_min.x) q2 = 2;
	if(p2.y > bbox_max.y)      q2 |= 4;
	else if(p2.y < bbox_min.y) q2 |= 8;
	if(p2.z > bbox_max.z)      q2 |= 0x20;
	else if(p2.z < bbox_min.z) q2 |= 0x10;

	if(q2 == 0)
		{
		intercept.x = p2.x;
		intercept.y = p2.y;
		intercept.z = p2.z;
		return true;
		}

	// 0x00037385, the only false return that writes nothing.
	if(q1 & q2)
		return false;

	// The reciprocal is formed once per block and both other axes multiply by
	// it, associated ((delta * inverse) * numerator) and not delta * (numerator
	// / direction). There is no guard on the division.
	if(q1 & 3)
		{
		intercept.x = (q1 & 1) ? bbox_max.x : bbox_min.x;
		const double direction = p2.x - (double) p1.x;
		const double numerator = intercept.x - (double) p1.x;
		const double inverse = 1.0f / direction;
		intercept.y = (NxReal) (((p2.y - (double) p1.y) * inverse) * numerator + p1.y);
		const NxReal along = (NxReal) (((p2.z - (double) p1.z) * inverse) * numerator + p1.z);
		intercept.z = along;
		// A NaN fails every one of these and falls through to the next block,
		// unlike the classification above where a NaN counts as inside.
		if(intercept.y <= bbox_max.y && intercept.y >= bbox_min.y
			&& along <= bbox_max.z && along >= bbox_min.z)
			return true;
		}

	if(q1 & 0xc)
		{
		intercept.y = (q1 & 4) ? bbox_max.y : bbox_min.y;
		const double direction = p2.y - (double) p1.y;
		const double numerator = intercept.y - (double) p1.y;
		const double inverse = 1.0f / direction;
		intercept.x = (NxReal) (((p2.x - (double) p1.x) * inverse) * numerator + p1.x);
		const NxReal along = (NxReal) (((p2.z - (double) p1.z) * inverse) * numerator + p1.z);
		intercept.z = along;
		if(intercept.x <= bbox_max.x && intercept.x >= bbox_min.x
			&& along <= bbox_max.z && along >= bbox_min.z)
			return true;
		}

	if(q1 & 0x30)
		{
		intercept.z = (q1 & 0x20) ? bbox_max.z : bbox_min.z;
		const double direction = p2.z - (double) p1.z;
		const double numerator = intercept.z - (double) p1.z;
		const double inverse = 1.0f / direction;
		intercept.x = (NxReal) (((p2.x - (double) p1.x) * inverse) * numerator + p1.x);
		const NxReal along = (NxReal) (((p2.y - (double) p1.y) * inverse) * numerator + p1.y);
		intercept.y = along;
		if(intercept.x <= bbox_max.x && intercept.x >= bbox_min.x
			&& along <= bbox_max.y && along >= bbox_min.y)
			return true;
		}

	return false;
	}

// 0x00037820 and 0x00037550. Ray and segment against an oriented box.
//
// Both return a bare bool and write nothing, so the 299-case matrix cannot
// distinguish a correct implementation from the oracle for either of them by
// construction. Everything below rests on the disassembly and on the randomized
// differential; the matrix only confirms.
//
// NxMat33 is row-major, so pairing offsets 0, 3, 6 with x, y, z is the
// transpose -- world into box space. The oracle evaluates that product as
// `((m31*z + m21*y) + m11*x)`, which is the reverse of the order the public
// header's own `multiplyByTranspose` is written in. The binary is what is
// reproduced here.
//
// Two asymmetries carry real precision consequences and neither is tidied.
// The transformed origin and direction are never narrowed -- they stay at
// register precision through the slab test -- while their absolute values are.
// And `C.z` is stored with a non-popping `fst` at 0x000379f0, so the first
// cross test consumes the 53-bit register copy while the second and third
// reload the 32-bit slot. Declaring that one value uniformly either way is
// wrong on a large share of inputs.
bool NX_CALL_CONV NxRayOBBIntersect(const NxRay& ray, const NxVec3& center,
	const NxVec3& extents, const NxMat33& rot)
	{
	const NxReal* m = (const NxReal*) &rot;

	const NxReal tx = (NxReal) (ray.orig.x - (double) center.x);
	const NxReal ty = (NxReal) (ray.orig.y - (double) center.y);
	const NxReal tz = (NxReal) (ray.orig.z - (double) center.z);

	// Axis 0 finishes its origin transform value-first where axes 1 and 2
	// finish matrix-first. That is only observable in the choice between two
	// NaN payloads, and it is transcribed for exactly that reason.
	const double dirX = ((m[6] * (double) ray.dir.z) + (m[3] * (double) ray.dir.y))
		+ (m[0] * (double) ray.dir.x);
	const NxReal absDirX = (NxReal) fabs(dirX);
	const double originX = ((m[6] * (double) tz) + (m[3] * (double) ty)) + (tx * (double) m[0]);
	// The product is formed in a register, so it does not flush to zero the way
	// a float product would, and the test is a value comparison: a product of
	// -0.0 counts as non-negative and rejects.
	if(fabs(originX) > extents.x && originX * dirX >= 0.0)
		return false;

	const double dirY = ((m[7] * (double) ray.dir.z) + (m[4] * (double) ray.dir.y))
		+ (m[1] * (double) ray.dir.x);
	const NxReal absDirY = (NxReal) fabs(dirY);
	const double originY = ((m[7] * (double) tz) + (m[4] * (double) ty)) + (m[1] * (double) tx);
	if(fabs(originY) > extents.y && originY * dirY >= 0.0)
		return false;

	const double dirZ = ((m[8] * (double) ray.dir.z) + (m[5] * (double) ray.dir.y))
		+ (m[2] * (double) ray.dir.x);
	const NxReal absDirZ = (NxReal) fabs(dirZ);
	const double originZ = ((m[8] * (double) tz) + (m[5] * (double) ty)) + (m[2] * (double) tx);
	if(fabs(originZ) > extents.z && originZ * dirZ >= 0.0)
		return false;

	const NxReal crossX = (NxReal) (tz * (double) ray.dir.y - ty * (double) ray.dir.z);
	const NxReal crossY = (NxReal) (tx * (double) ray.dir.z - tz * (double) ray.dir.x);
	const double crossZRegister = ty * (double) ray.dir.x - tx * (double) ray.dir.y;
	const NxReal crossZ = (NxReal) crossZRegister;

	if(fabs(((crossZRegister * m[6]) + (crossY * (double) m[3])) + (crossX * (double) m[0]))
		> (absDirZ * (double) extents.y) + (absDirY * (double) extents.z))
		return false;
	if(fabs(((crossZ * (double) m[7]) + (crossY * (double) m[4])) + (crossX * (double) m[1]))
		> (absDirZ * (double) extents.x) + (absDirX * (double) extents.z))
		return false;
	// 0x00037a92 drops the exchange and compares the other way round. Same
	// meaning for ordered values, and a NaN passes either way.
	if((absDirX * (double) extents.y) + (absDirY * (double) extents.x)
		< fabs(((crossZ * (double) m[8]) + (crossY * (double) m[5])) + (crossX * (double) m[2])))
		return false;
	return true;
	}

// 0x00037550. The same cross-axis core as NxRayOBBIntersect, with the segment's
// half-vector standing in for the ray direction and a plain interval test on
// the three box axes instead of the ray's slab cull.
//
// The midpoint is built three different ways and that is the thing to get
// right: x narrows the half before subtracting the centre, y keeps it at
// register precision, and z narrows the *sum* first and then halves it. One
// vector, three narrowing patterns. Nothing a hand-authored case would find.
bool NX_CALL_CONV NxSegmentOBBIntersect(const NxVec3& p0, const NxVec3& p1,
	const NxVec3& center, const NxVec3& extents, const NxMat33& rot)
	{
	const NxReal* m = (const NxReal*) &rot;

	const double deltaX = p1.x - (double) p0.x;
	const double deltaY = p1.y - (double) p0.y;
	const NxReal deltaZ = (NxReal) (p1.z - (double) p0.z);
	const NxReal extentX = (NxReal) (deltaX * 0.5f);
	const NxReal extentY = (NxReal) (deltaY * 0.5f);
	const NxReal extentZ = (NxReal) (deltaZ * 0.5f);

	// The three sums also differ in operand order -- p0+p1, then p1+p0, then
	// p0+p1 -- which again only shows in NaN payload selection.
	const double sumX = p0.x + (double) p1.x;
	const double sumY = p1.y + (double) p0.y;
	const NxReal sumZ = (NxReal) (p0.z + (double) p1.z);
	const NxReal halfX = (NxReal) (sumX * 0.5f);
	const double halfY = sumY * 0.5f;
	const double halfZ = sumZ * 0.5f;
	const NxReal midX = (NxReal) (halfX - (double) center.x);
	const NxReal midY = (NxReal) (halfY - center.y);
	const NxReal midZ = (NxReal) (halfZ - center.z);

	const NxReal radiusX = (NxReal) fabs(((m[6] * (double) extentZ) + (m[3] * (double) extentY))
		+ (extentX * (double) m[0]));
	const double centreX = fabs(((m[6] * (double) midZ) + (m[3] * (double) midY))
		+ (midX * (double) m[0]));
	// The radius sum is a 53-bit add of two floats, not a float add, so an
	// extent far smaller than the radius still moves the boundary.
	if(centreX > (double) radiusX + extents.x)
		return false;

	const NxReal radiusY = (NxReal) fabs(((m[7] * (double) extentZ) + (m[4] * (double) extentY))
		+ (m[1] * (double) extentX));
	const double centreY = fabs(((m[7] * (double) midZ) + (m[4] * (double) midY))
		+ (m[1] * (double) midX));
	if(centreY > (double) radiusY + extents.y)
		return false;

	const NxReal radiusZ = (NxReal) fabs(((m[8] * (double) extentZ) + (m[5] * (double) extentY))
		+ (m[2] * (double) extentX));
	const double centreZ = fabs(((m[8] * (double) midZ) + (m[5] * (double) midY))
		+ (m[2] * (double) midX));
	if((double) radiusZ + extents.z < centreZ)
		return false;

	const NxReal crossX = (NxReal) (midZ * (double) extentY - midY * (double) extentZ);
	const NxReal crossY = (NxReal) (extentZ * (double) midX - midZ * (double) extentX);
	const double crossZRegister = midY * (double) extentX - extentY * (double) midX;
	const NxReal crossZ = (NxReal) crossZRegister;

	if(fabs(((crossZRegister * m[6]) + (crossY * (double) m[3])) + (crossX * (double) m[0]))
		> (radiusZ * (double) extents.y) + (radiusY * (double) extents.z))
		return false;
	if(fabs(((crossZ * (double) m[7]) + (crossY * (double) m[4])) + (crossX * (double) m[1]))
		> (radiusZ * (double) extents.x) + (radiusX * (double) extents.z))
		return false;
	if((radiusX * (double) extents.y) + (radiusY * (double) extents.x)
		< fabs(((crossZ * (double) m[8]) + (crossY * (double) m[5])) + (crossX * (double) m[2])))
		return false;
	return true;
	}

// 0x000381c0. Ray against a capsule: an infinite-cylinder body test followed by
// a sphere test at each end cap, with a separate closed form for a ray that
// runs along the axis.
//
// The capsule is two points then a radius, seven consecutive floats, and the
// oracle indexes it that way.
//
// This kernel ships a real defect and it is load-bearing. The companion vector
// that completes the local frame is built by one of two branches, and the one
// taken when |n.y| > |n.x| divides by the *squared* sub-length where the other
// divides by the sub-length: there is an fsqrt at 0x0003826d and none at
// 0x00038291. The frame is therefore not orthonormal on that path, which about
// half of all inputs take, and every radius comparison downstream is scaled
// wrong. It is reproduced, not corrected.
//
// A capsule whose two points coincide takes the other branch, forms 1/sqrt(0),
// and turns everything downstream into NaN, so a capsule that is geometrically
// a sphere never reports an intersection.
NxU32 NX_CALL_CONV NxRayCapsuleIntersect(const NxVec3& origin, const NxVec3& dir,
	const NxCapsule& capsule, NxReal t[2])
	{
	const NxReal* c = (const NxReal*) &capsule;

	const NxReal axisX = (NxReal) (c[3] - (double) c[0]);
	const NxReal axisY = (NxReal) (c[4] - (double) c[1]);
	const NxReal axisZ = (NxReal) (c[5] - (double) c[2]);

	// If the axis has no length the three registers keep the raw components.
	double nx = axisX, ny = axisY, nz = axisZ;
	const NxReal length = (NxReal) sqrt((axisZ * (double) axisZ + axisY * (double) axisY)
		+ axisX * (double) axisX);
	if(length != 0.0f)
		{
		const NxReal inverseLength = (NxReal) (1.0f / (double) length);
		nx = axisX * (double) inverseLength;
		ny = axisY * (double) inverseLength;
		nz = axisZ * (double) inverseLength;
		}

	// Ordered less-or-equal takes the square-rooted branch; greater, and any
	// unordered comparison, takes the one that is missing its square root.
	double qx, qy, qz;
	if(fabs(ny) <= fabs(nx))
		{
		const double inverse = 1.0f / sqrt(nz * nz + nx * nx);
		const NxReal narrowed = (NxReal) inverse;
		qx = -(inverse * nz);
		qy = 0.0f;
		qz = narrowed * nx;
		}
	else
		{
		const double inverse = 1.0f / (nz * nz + ny * ny);
		const NxReal narrowed = (NxReal) inverse;
		qx = 0.0f;
		qy = nz * (double) narrowed;
		qz = -((double) narrowed * ny);
		}

	const NxReal rx = (NxReal) (qz * ny - qy * nz);
	const NxReal ry = (NxReal) (qx * nz - qz * nx);
	const NxReal rz = (NxReal) (qy * nx - qx * ny);

	// The direction dot products sum x, z, y; the origin ones below sum z, y, x.
	NxReal kdq = (NxReal) ((qx * dir.x + qz * dir.z) + qy * dir.y);
	NxReal kdr = (NxReal) ((rx * (double) dir.x + rz * (double) dir.z) + ry * (double) dir.y);
	NxReal kdn = (NxReal) ((nx * dir.x + nz * dir.z) + ny * dir.y);

	const NxReal dirLength = (NxReal) sqrt((kdr * (double) kdr + kdq * (double) kdq)
		+ kdn * (double) kdn);
	if(dirLength != 0.0f)
		{
		const double inverse = 1.0f / (double) dirLength;
		kdq = (NxReal) (kdq * inverse);
		kdr = (NxReal) (kdr * inverse);
		kdn = (NxReal) (inverse * kdn);
		}

	// Formed a second time at 0x0003838e, narrowed this time, and outside the
	// guard above, so a zero direction length reaches the outputs as an
	// infinity rather than being rejected.
	const NxReal outputScale = (NxReal) (1.0f / (double) dirLength);

	const NxReal wx = (NxReal) (origin.x - (double) c[0]);
	const NxReal wy = (NxReal) (origin.y - (double) c[1]);
	const double wzRegister = origin.z - (double) c[2];
	const NxReal wz = (NxReal) wzRegister;

	// 0x000383ae is `fst`: the q product uses the register copy of w.z and the
	// other two reload the narrowed one.
	const NxReal kwq = (NxReal) ((wzRegister * qz + wy * qy) + wx * qx);
	const NxReal kwr = (NxReal) ((wz * (double) rz + wy * (double) ry) + wx * (double) rx);
	const NxReal kwn = (NxReal) ((wz * nz + wy * ny) + wx * nx);

	const NxReal radiusSquared = (NxReal) (c[6] * (double) c[6]);

	// Both dispatch tests let an unordered comparison through to the cylinder.
	if(!(fabs((double) kdn) >= gCapsuleAxialThreshold)
		&& !((double) dirLength < gCapsuleDegenerateEpsilon))
		{
		const NxReal a = (NxReal) (kdr * (double) kdr + kdq * (double) kdq);
		const NxReal b = (NxReal) (kdr * (double) kwr + kdq * (double) kwq);
		const NxReal cc = (NxReal) ((kwr * (double) kwr + kwq * (double) kwq) - radiusSquared);

		// The sign is tested on the register value and the square root taken of
		// the narrowed one, so a discriminant that underflows on the way into
		// its slot passes the first test and fails the second.
		const double discriminantRegister = b * (double) b - cc * (double) a;
		const NxReal discriminant = (NxReal) discriminantRegister;
		if(discriminantRegister < 0.0)
			return 0;

		NxU32 count = 0;
		if(discriminant > 0.0f)
			{
			const double root = sqrt((double) discriminant);
			const double inverseA = 1.0f / (double) a;
			// The two roots are written differently -- (-b) - root against
			// root - b -- and the first range test reads the register value
			// where the second reads the narrowed one.
			const double firstRegister = ((-(double) b) - root) * inverseA;
			const NxReal first = (NxReal) firstRegister;
			const double firstAxial = firstRegister * (double) kdn + kwn;
			if(firstAxial >= 0.0 && firstAxial <= (double) length)
				{
				t[count] = (NxReal) (first * (double) outputScale);
				++count;
				}
			const NxReal second = (NxReal) ((root - b) * inverseA);
			const double secondAxial = second * (double) kdn + kwn;
			if(secondAxial >= 0.0 && secondAxial <= (double) length)
				{
				t[count] = (NxReal) (second * (double) outputScale);
				++count;
				}
			if(count == 2)
				return 2;
			}
		else
			{
			// A different expression from the two-root form: -(b/a) rather than
			// (-b) * (1/a). The tangent case also returns without ever
			// consulting the end caps.
			const double tangentRegister = -((double) b / (double) a);
			const NxReal tangent = (NxReal) tangentRegister;
			const double axial = tangentRegister * (double) kdn + kwn;
			if(axial >= 0.0 && axial <= (double) length)
				{
				t[0] = (NxReal) (tangent * (double) outputScale);
				return 1;
				}
			}

		// The cap spheres. The coefficients are patched incrementally rather
		// than recomputed and the leading coefficient is one, so there is no
		// division here at all.
		const NxReal b1 = (NxReal) (kwn * (double) kdn + b);
		const NxReal c1 = (NxReal) (kwn * (double) kwn + cc);
		const double discriminant1 = b1 * (double) b1 - c1;
		if(discriminant1 > 0.0)
			{
			const double root = sqrt(discriminant1);
			const double nearRoot = (-(double) b1) - root;
			if(nearRoot * (double) kdn + kwn <= 0.0)
				{
				t[count] = (NxReal) (nearRoot * (double) outputScale);
				if(++count == 2)
					return 2;
				}
			const double farRoot = root - b1;
			if(farRoot * (double) kdn + kwn <= 0.0)
				{
				t[count] = (NxReal) (farRoot * (double) outputScale);
				if(++count == 2)
					return 2;
				}
			}
		else if(discriminant1 == 0.0)
			{
			const double tangent = -(double) b1;
			if(tangent * (double) kdn + kwn <= 0.0)
				{
				t[count] = (NxReal) (tangent * (double) outputScale);
				if(++count == 2)
					return 2;
				}
			}

		// 0x0003867f is `fst`, so the square below is the register value times
		// the narrowed one.
		const double b2Register = (double) b1 - (double) length * kdn;
		const NxReal b2 = (NxReal) b2Register;
		const double discriminant2 = b2Register * (double) b2
			- (((double) length - (kwn + (double) kwn)) * length + c1);
		if(discriminant2 > 0.0)
			{
			const double root = sqrt(discriminant2);
			const double nearRoot = (-(double) b2) - root;
			if(nearRoot * (double) kdn + kwn >= (double) length)
				{
				t[count] = (NxReal) (nearRoot * (double) outputScale);
				if(++count == 2)
					return 2;
				}
			const double farRoot = root - b2;
			if(farRoot * (double) kdn + kwn >= (double) length)
				{
				t[count] = (NxReal) (farRoot * (double) outputScale);
				if(++count == 2)
					return 2;
				}
			}
		else if(discriminant2 == 0.0)
			{
			const double tangent = -(double) b2;
			if(tangent * (double) kdn + kwn >= (double) length)
				{
				t[count] = (NxReal) (tangent * (double) outputScale);
				if(++count == 2)
					return 2;
				}
			}

		return count;
		}

	// 0x00038729. The axial case recomputes its own dot product from the raw,
	// unnormalized axis rather than reusing kdn, and the two branches write
	// their roots in opposite orders: the first is descending.
	const double axisDot = (axisX * (double) dir.x + axisZ * (double) dir.z)
		+ axisY * (double) dir.y;
	const NxReal half = (NxReal) (((double) radiusSquared - kwq * (double) kwq)
		- kwr * (double) kwr);
	if(axisDot < 0.0 && half >= 0.0f)
		{
		const double root = sqrt((double) half);
		t[0] = (NxReal) ((kwn + root) * (double) outputScale);
		t[1] = (NxReal) (-((((double) length - kwn) + root) * (double) outputScale));
		return 2;
		}
	if(axisDot > 0.0 && half >= 0.0f)
		{
		const double root = sqrt((double) half);
		t[0] = (NxReal) (-((kwn + root) * (double) outputScale));
		t[1] = (NxReal) ((((double) length - kwn) + root) * (double) outputScale);
		return 2;
		}
	return 0;
	}

// 0x00038810. Two spheres swept along constant velocities, as a quadratic in
// time over the unit step.
//
// The oracle forms each end point as `centre + velocity` and then subtracts the
// centre again to recover the velocity, rather than using the velocity it was
// given. That is not an algebraic identity in floating point and it is
// reproduced literally. The three components are not treated alike either: x
// and y of the second sphere keep register precision where z does not, and the
// recovered relative velocity narrows x and y but not z.
//
// It also has a reachable defect. Two stationary, separated spheres return
// true: the leading coefficient is zero, the reciprocal is an infinity, both
// roots become NaN, and every remaining comparison falls the permissive way.
bool NX_CALL_CONV NxSweptSpheresIntersect(const NxSphere& sphere0, const NxVec3& velocity0,
	const NxSphere& sphere1, const NxVec3& velocity1)
	{
	const NxReal* s0 = (const NxReal*) &sphere0;
	const NxReal* s1 = (const NxReal*) &sphere1;

	const double end1X = s1[0] + (double) velocity1.x;
	const double end1Y = velocity1.y + (double) s1[1];
	const NxReal end1Z = (NxReal) (velocity1.z + (double) s1[2]);

	const double end0X = s0[0] + (double) velocity0.x;
	const double end0Y = velocity0.y + (double) s0[1];
	const NxReal end0Z = (NxReal) (velocity0.z + (double) s0[2]);

	const NxReal moved0X = (NxReal) (end0X - (double) s0[0]);
	const NxReal moved0Y = (NxReal) (end0Y - (double) s0[1]);
	const NxReal moved0Z = (NxReal) (end0Z - (double) s0[2]);

	const NxReal moved1X = (NxReal) (end1X - (double) s1[0]);
	const double moved1Y = end1Y - (double) s1[1];
	const double moved1Z = end1Z - (double) s1[2];

	const NxReal deltaX = (NxReal) (s1[0] - (double) s0[0]);
	const NxReal deltaY = (NxReal) (s1[1] - (double) s0[1]);
	const NxReal deltaZ = (NxReal) (s1[2] - (double) s0[2]);

	const NxReal relativeX = (NxReal) (moved1X - (double) moved0X);
	const NxReal relativeY = (NxReal) (moved1Y - (double) moved0Y);
	const double relativeZ = moved1Z - (double) moved0Z;

	const double radiusSum = s0[3] + (double) s1[3];

	const NxReal a = (NxReal) ((relativeZ * relativeZ + relativeY * (double) relativeY)
		+ relativeX * (double) relativeX);
	const double halfB = (relativeZ * (double) deltaZ + relativeY * (double) deltaY)
		+ relativeX * (double) deltaX;
	const NxReal b = (NxReal) (halfB + halfB);
	const NxReal distanceSquared = (NxReal) ((deltaZ * (double) deltaZ + deltaY * (double) deltaY)
		+ deltaX * (double) deltaX);

	const double radiusSumSquared = radiusSum * radiusSum;

	// Already touching. The comparison is against a 53-bit square of a 53-bit
	// sum, so the boundary is not where a float implementation puts it, and an
	// unordered comparison continues to the quadratic rather than returning.
	if((double) distanceSquared <= radiusSumSquared)
		return true;

	const double c = (double) distanceSquared - radiusSumSquared;
	const double discriminant = b * (double) b - ((c * (double) a) * 4.0f);
	if(!(discriminant >= 0.0f))
		return false;

	const double root = sqrt(discriminant);
	// A reciprocal and a multiply, not a division by 2a.
	const double inverse = 1.0f / (a + (double) a);
	const NxReal first = (NxReal) ((root - b) * inverse);
	const double second = ((-(double) b) - root) * inverse;

	// The larger root is a narrowed float on one path and a register double on
	// the other, so the test below has different resolution depending on which
	// root won.
	NxReal earliest;
	double latest;
	if((double) first > second)
		{
		earliest = (NxReal) second;
		latest = (double) first;
		}
	else
		{
		earliest = first;
		latest = second;
		}

	if(latest < 0.0f)
		return false;
	if((double) earliest > 1.0f)
		return false;
	return true;
	}

// 0x000360e0. The same fifteen axes as NxBoxBoxIntersect, but reporting which
// one separated rather than whether any did.
//
// The inventory sizes this at 269 bytes, which is the prologue only: the body
// continues past a three-byte alignment pad into an aligned loop at 0x000361f0
// and runs to the two returns at 0x0003667b and 0x00036686. Reconstructing this
// export therefore closes three inventory rows, not one -- phys_fn_001696,
// phys_fn_001698 and phys_fn_001700.
//
// It does not early out. Every axis it is going to compute is computed, the
// signed overlaps are stored, and the *largest* is returned -- so a wrong axis
// ordering is visible in the return value where NxBoxBoxIntersect's bool hides
// it. Ties keep the lower index and a NaN overlap can never win, so a wholly
// NaN transform reports an overlap.
//
// It is also numerically distinct from NxBoxBoxIntersect despite computing the
// same quantities: T[0] and the third column of the matrix product associate
// their terms differently in the two functions. They cannot share code.
NxSepAxis NX_CALL_CONV NxSeparatingAxis(const NxVec3& extents0, const NxVec3& center0,
	const NxMat33& rotation0, const NxVec3& extents1, const NxVec3& center1,
	const NxMat33& rotation1, bool fullTest)
	{
	const NxReal* a = (const NxReal*) &rotation0;
	const NxReal* b = (const NxReal*) &rotation1;
	const NxReal* e0 = &extents0.x;
	const NxReal* e1 = &extents1.x;

	// Never narrowed, exactly as in NxBoxBoxIntersect.
	const double dx = center1.x - (double) center0.x;
	const double dy = center1.y - (double) center0.y;
	const double dz = center1.z - (double) center0.z;

	NxReal t[3];
	t[0] = (NxReal) ((a[6] * dz + a[3] * dy) + a[0] * dx);
	t[1] = (NxReal) ((a[7] * dz + a[4] * dy) + a[1] * dx);
	t[2] = (NxReal) ((a[8] * dz + a[5] * dy) + a[2] * dx);

	// The third column sums x, z, y where the first two sum z, y, x.
	NxReal m[3][3];
	for(NxU32 k = 0; k < 3; ++k)
		{
		m[k][0] = (NxReal) ((b[6] * (double) a[k + 6] + b[3] * (double) a[k + 3]) + b[0] * (double) a[k]);
		m[k][1] = (NxReal) ((b[7] * (double) a[k + 6] + b[4] * (double) a[k + 3]) + b[1] * (double) a[k]);
		m[k][2] = (NxReal) ((b[2] * (double) a[k] + b[8] * (double) a[k + 6]) + b[5] * (double) a[k + 3]);
		}

	// fabs of a stored float is exactly a float, so these nine carry no
	// precision question even though four of them stay in registers.
	NxReal absM[3][3];
	for(NxU32 row = 0; row < 3; ++row)
		for(NxU32 col = 0; col < 3; ++col)
			absM[row][col] = (NxReal) fabs((double) m[row][col]);

	// The signed overlap on each axis. Note the grouping: the box-0 extent is
	// subtracted from the projection *before* the box-1 radius is subtracted,
	// rather than the two radii being summed first.
	NxReal d[15];
	d[0] = (NxReal) ((fabs((double) t[0]) - e0[0])
		- ((absM[0][2] * (double) e1[2] + absM[0][0] * (double) e1[0]) + absM[0][1] * (double) e1[1]));
	d[1] = (NxReal) ((fabs((double) t[1]) - e0[1])
		- ((absM[1][2] * (double) e1[2] + absM[1][1] * (double) e1[1]) + absM[1][0] * (double) e1[0]));
	d[2] = (NxReal) ((fabs((double) t[2]) - e0[2])
		- ((absM[2][2] * (double) e1[2] + absM[2][1] * (double) e1[1]) + absM[2][0] * (double) e1[0]));

	d[3] = (NxReal) ((fabs((t[2] * (double) m[2][0] + t[1] * (double) m[1][0]) + t[0] * (double) m[0][0])
		- ((absM[1][0] * (double) e0[1] + absM[0][0] * (double) e0[0]) + absM[2][0] * (double) e0[2])) - e1[0]);
	d[4] = (NxReal) ((fabs((t[2] * (double) m[2][1] + t[1] * (double) m[1][1]) + t[0] * (double) m[0][1])
		- ((absM[1][1] * (double) e0[1] + absM[0][1] * (double) e0[0]) + absM[2][1] * (double) e0[2])) - e1[1]);
	d[5] = (NxReal) ((fabs((t[2] * (double) m[2][2] + t[1] * (double) m[1][2]) + t[0] * (double) m[0][2])
		- ((absM[1][2] * (double) e0[1] + absM[0][2] * (double) e0[0]) + absM[2][2] * (double) e0[2])) - e1[2]);

	if(fullTest)
		{
		d[6] = (NxReal) ((fabs(t[2] * (double) m[1][0] - t[1] * (double) m[2][0])
			- (absM[2][0] * (double) e0[1] + absM[1][0] * (double) e0[2]))
			- (absM[0][1] * (double) e1[2] + absM[0][2] * (double) e1[1]));
		d[7] = (NxReal) ((fabs(t[2] * (double) m[1][1] - t[1] * (double) m[2][1])
			- (absM[2][1] * (double) e0[1] + absM[1][1] * (double) e0[2]))
			- (absM[0][0] * (double) e1[2] + absM[0][2] * (double) e1[0]));
		d[8] = (NxReal) ((fabs(t[2] * (double) m[1][2] - t[1] * (double) m[2][2])
			- (absM[2][2] * (double) e0[1] + absM[1][2] * (double) e0[2]))
			- (absM[0][0] * (double) e1[1] + absM[0][1] * (double) e1[0]));
		d[9] = (NxReal) ((fabs(t[0] * (double) m[2][0] - t[2] * (double) m[0][0])
			- (absM[2][0] * (double) e0[0] + absM[0][0] * (double) e0[2]))
			- (absM[1][2] * (double) e1[1] + absM[1][1] * (double) e1[2]));
		d[10] = (NxReal) ((fabs(t[0] * (double) m[2][1] - t[2] * (double) m[0][1])
			- (absM[2][1] * (double) e0[0] + absM[0][1] * (double) e0[2]))
			- (absM[1][2] * (double) e1[0] + absM[1][0] * (double) e1[2]));
		d[11] = (NxReal) ((fabs(t[0] * (double) m[2][2] - t[2] * (double) m[0][2])
			- (absM[2][2] * (double) e0[0] + absM[0][2] * (double) e0[2]))
			- (absM[1][1] * (double) e1[0] + absM[1][0] * (double) e1[1]));
		d[12] = (NxReal) ((fabs(t[1] * (double) m[0][0] - t[0] * (double) m[1][0])
			- (absM[0][0] * (double) e0[1] + absM[1][0] * (double) e0[0]))
			- (absM[2][2] * (double) e1[1] + absM[2][1] * (double) e1[2]));
		d[13] = (NxReal) ((fabs(t[1] * (double) m[0][1] - t[0] * (double) m[1][1])
			- (absM[0][1] * (double) e0[1] + absM[1][1] * (double) e0[0]))
			- (absM[2][2] * (double) e1[0] + absM[2][0] * (double) e1[2]));
		d[14] = (NxReal) ((fabs(t[1] * (double) m[0][2] - t[0] * (double) m[1][2])
			- (absM[0][2] * (double) e0[1] + absM[1][2] * (double) e0[0]))
			- (absM[2][1] * (double) e1[0] + absM[2][0] * (double) e1[1]));
		}
	else
		{
		// THE ORACLE READS UNINITIALISED STACK HERE, and the scan below still
		// covers all fifteen slots. `fullTest` false skips the nine stores at
		// 0x000363d3 but does not shorten the selection loop at 0x00036600.
		//
		// Six of the nine slots are genuinely irreproducible: d[9]..d[14] sit
		// at esp+0x8c..0xa0 and hold whatever the caller last left there, so on
		// that path the oracle is not a function of its arguments. They are
		// seeded with -FLT_MAX here, which is the only choice that is a
		// function of the arguments and the only one that cannot change the
		// answer. Where the residue happened to exceed the real maximum the
		// oracle returns an axis this cannot return. Recorded, not fixed.
		//
		// The other three are deterministic and are reproduced: d[6]..d[8] land
		// on the prologue's spill of rotation1's third column at
		// esp+0x80..0x88, written at 0x00036155, 0x00036168 and 0x00036178.
		// That is what makes NxSeparatingAxis.05 return 9 -- rotation1[8] is
		// 1.0f there and beats every real overlap in the case.
		d[6] = b[2];
		d[7] = b[5];
		d[8] = b[8];
		d[9] = -FLT_MAX;
		d[10] = -FLT_MAX;
		d[11] = -FLT_MAX;
		d[12] = -FLT_MAX;
		d[13] = -FLT_MAX;
		d[14] = -FLT_MAX;
		}

	// Strictly greater, so a NaN never becomes the maximum and a tie keeps the
	// lower index.
	double best = -FLT_MAX;
	NxI32 index = -1;
	for(NxU32 i = 0; i < 15; ++i)
		if(best < d[i])
			{
			best = d[i];
			index = (NxI32) i;
			}

	// Equality counts as separated, so boxes exactly touching report an axis.
	if(best < 0.0f)
		return NX_SEP_AXIS_OVERLAP;
	return (NxSepAxis) (index + 1);
	}
