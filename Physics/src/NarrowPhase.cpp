/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// The boolean overlap half of the shape-pair dispatch matrix, for the four
// primitive shape types.
//
// The floating-point model here is not the one a consumer calling an exported
// geometry kernel sees, and the difference is measured rather than assumed. A
// consumer call runs at the CRT default 0x027f: 53-bit precision, round to
// nearest. These entries are only ever reached from the simulation step, and
// phys_fn_000659 at 0x00013c40 calls NxSetFPURoundingChop and then
// NxSetFPUPrecision64 on its way in and restores the caller's word with `fldcw`
// on its way out. So the whole narrow phase runs at 64-bit precision with
// round-toward-zero.
//
// That window is wider than the narrow phase, and this matrix is what shows it.
// Three of the Phase 3 exports are inside it as well as outside:
// NxBoxBoxIntersect from the [BOX][BOX] overlap entry, NxBuildSmoothNormals
// from [BOX][MESH] contact generation and NxRayTriIntersect from [MESH][MESH].
// The step reaches the matrix through a function pointer, so no direct call
// edge crosses it and nothing before the matrix was recovered could have shown
// which exports the step reaches.
//
// No C++ type names a 64-bit x87 significand -- MSVC's long double is double --
// so the precision cannot be written down. It does not have to be: the control
// word is process state, and code that keeps a value in an x87 register picks
// up whatever precision is current, exactly as the oracle does. What the source
// must get right is the *shape* of the computation -- which values stay in a
// register and which the oracle pushes through a 32-bit slot -- and that it is
// compiled to x87 at all. `double` here means "the oracle keeps this in st(n)";
// `NxReal` means "the oracle stored it and read it back". Built /arch:IA32,
// which is what makes those two mean anything.

#include "NarrowPhase.h"

#include "NxSegment.h"
#include "NxMat33.h"
#include "NxIntersectionBoxBox.h"

#include <math.h>
#include <stddef.h>

// The four offsets every kernel below reads out of a shape. They are the
// oracle's, so they are asserted rather than commented.
static_assert(offsetof(NxCollisionShape, owner) == 0x04, "shape owner is at 0x04");
static_assert(offsetof(NxCollisionShape, rotation) == 0x0c, "shape rotation is at 0x0c");
static_assert(offsetof(NxCollisionShape, collisionObject) == 0x9c, "shape collision object is at 0x9c");
static_assert(offsetof(NxCollisionShape, translation) == 0x30, "shape translation is at 0x30");
static_assert(offsetof(NxCollisionShape, type) == 0xd0, "shape type is at 0xd0");
static_assert(offsetof(NxCollisionShape, geometry) == 0xe0, "shape geometry union is at 0xe0");

// phys_fn_000943 at 0x00020750.
//
// The signs arrive as full ints and are converted with `fild`, not folded into
// the constant, so the caller is free to pass anything; the plane/box entry
// passes -1 and +1.
void NxBoxShapeCorner(const NxCollisionShape* box, int signX, int signY, int signZ, NxVec3* corner)
	{
	const NxReal* m = box->rotation;
	const NxReal* t = box->translation;

	double fx = (double) signX * box->geometry[1];
	double fy = (double) signY * box->geometry[2];
	double fz = (double) signZ * box->geometry[3];

	// Each row of the product is narrowed to 32 bits before the translation is
	// added: `fstp dword ptr [esp]`, `[esp+4]`, `[esp+8]` at 0x00020788,
	// 0x0002079e and 0x000207b3.
	NxReal rotatedX = (NxReal) ((fz * m[2] + fy * m[1]) + fx * m[0]);
	NxReal rotatedY = (NxReal) ((fz * m[5] + fy * m[4]) + fx * m[3]);
	NxReal rotatedZ = (NxReal) ((fz * m[8] + fy * m[7]) + fx * m[6]);

	corner->x = (NxReal) ((double) rotatedX + t[0]);
	corner->y = (NxReal) ((double) rotatedY + t[1]);
	corner->z = (NxReal) ((double) rotatedZ + t[2]);
	}

// phys_fn_001899 at 0x00048a20, matrix slot [PLANE][SPHERE].
//
// `fcomp` against the 4-byte zero at 0x001041f0 -- which is the padding in
// front of an assert string, reused as a constant -- then `test ah,0x41` with
// `jp` to the false arm, so the unordered case is a miss.
bool __cdecl NxOverlapPlaneSphere(const NxCollisionShape* plane, const NxCollisionShape* sphere)
	{
	const NxReal* n = plane->geometry;
	const NxReal* c = sphere->translation;

	double distance = ((double) c[2] * n[2] + (double) c[1] * n[1]) + (double) c[0] * n[0];
	distance += plane->geometry[3];
	distance -= sphere->geometry[0];
	return distance <= 0.0;
	}

// phys_fn_001881 at 0x00047e90, matrix slot [PLANE][BOX].
//
// Eight corners, each rebuilt from scratch through phys_fn_000943 rather than
// from an incremental walk, and the loop indices really are -1 and +1 stepping
// by 2 -- `or ebp,0xffffffff` then `add ebp,2` against `cmp ebp,1`.
bool __cdecl NxOverlapPlaneBox(const NxCollisionShape* plane, const NxCollisionShape* box)
	{
	const NxReal* n = plane->geometry;

	for(int signX = -1; signX <= 1; signX += 2)
		for(int signY = -1; signY <= 1; signY += 2)
			for(int signZ = -1; signZ <= 1; signZ += 2)
				{
				NxVec3 corner;
				NxBoxShapeCorner(box, signX, signY, signZ, &corner);

				double distance = ((double) corner.x * n[0] + (double) corner.y * n[1]) + (double) corner.z * n[2];
				distance += plane->geometry[3];
				if(distance <= 0.0)
					return true;
				}
	return false;
	}

// phys_fn_001889 at 0x00048270, matrix slot [PLANE][CAPSULE].
//
// Three things here are the oracle's and not a simplification's:
//   * the capsule axis is the second *column* of the rotation -- m[1], m[4],
//     m[7] -- so a capsule points along its own local +Y;
//   * the x component of the half-axis is narrowed to 32 bits at 0x0004829e
//     and read back, while y and z stay in registers, and the z component is
//     then narrowed again on its own at 0x000482bc;
//   * p1.z is never stored. It is still in st(0) when the second plane
//     distance is formed, where every other component came back out of a
//     32-bit slot.
// The two distances are also summed in different orders, which at register
// precision is not observable; it is transcribed because it is what the binary
// does.
bool __cdecl NxOverlapPlaneCapsule(const NxCollisionShape* plane, const NxCollisionShape* capsule)
	{
	const NxReal* m = capsule->rotation;
	const NxReal* t = capsule->translation;
	const NxReal* n = plane->geometry;

	double radius = capsule->geometry[0];
	NxReal halfHeight = capsule->geometry[1];

	NxReal axisX = (NxReal) ((double) m[1] * halfHeight);
	double axisY = (double) m[4] * halfHeight;
	double axisZ = (double) m[7] * halfHeight;
	NxReal negatedAxisZ = (NxReal) (-axisZ);

	NxReal p0x = (NxReal) (-(double) axisX + t[0]);
	NxReal p0y = (NxReal) (-axisY + t[1]);
	NxReal p0z = (NxReal) ((double) negatedAxisZ + t[2]);
	NxReal p1x = (NxReal) ((double) axisX + t[0]);
	NxReal p1y = (NxReal) (axisY + t[1]);
	double p1z = axisZ + t[2];

	double distance0 = ((double) p0z * n[2] + (double) p0y * n[1]) + (double) p0x * n[0];
	distance0 += plane->geometry[3];
	if(distance0 < radius)
		return true;

	double distance1 = ((double) p1y * n[1] + p1z * n[2]) + (double) p1x * n[0];
	distance1 += plane->geometry[3];
	return distance1 < radius;
	}

// phys_fn_001931 at 0x0004b800, matrix slot [SPHERE][SPHERE].
//
// Everything stays in x87 registers, so the squared distance carries the full
// working precision rather than being rounded per term. The test is strict and
// the unordered case is a miss, so two spheres exactly touching do not overlap.
bool __cdecl NxOverlapSphereSphere(const NxCollisionShape* sphere0, const NxCollisionShape* sphere1)
	{
	double dx = (double) sphere1->translation[0] - sphere0->translation[0];
	double dy = (double) sphere1->translation[1] - sphere0->translation[1];
	double dz = (double) sphere1->translation[2] - sphere0->translation[2];
	double radius = (double) sphere0->geometry[0] + sphere1->geometry[0];

	double squared = (dz * dz + dy * dy) + dx * dx;
	return radius * radius > squared;
	}

// phys_fn_001913 at 0x00049ca0.
//
// Closest point on the box to the sphere centre, in box space, then back out.
// Four details are the oracle's:
//   * the separation's z component is stored with `fst` at 0x00049cc6, which
//     narrows the copy and leaves the wide value in st(0). The first row of
//     the transform into box space then uses the wide value and the other two
//     rows read the narrowed one back, so the three rows are not formed from
//     the same z.
//   * every clamp is a strict ordered comparison, so a NaN coordinate is
//     neither below its extent nor above it and is left unclamped.
//   * the "was anything clamped" flag is set on the x and y axes only. The z
//     axis does not need it: control only reaches the flag test when z was
//     inside its extent, so the flag then answers "is the centre inside the
//     box" exactly -- and a NaN z reaching it with x and y unclamped returns
//     true.
//   * the third component of the point back in world space is left in a
//     register while the first two are pushed through 32-bit slots, so the
//     three components of the separation are not formed at the same precision.
// It also writes -extents.z into the caller's first argument slot at
// 0x00049d92, which is dead by then but is a store past the callee's own
// frame.
bool __cdecl NxOverlapSphereBoxData(const NxCollisionSphereData* sphere, const NxCollisionBoxData* box)
	{
	const NxReal* m = box->rotation;
	const NxReal* extents = box->extents;

	NxReal dx = (NxReal) ((double) sphere->center[0] - box->center[0]);
	NxReal dy = (NxReal) ((double) sphere->center[1] - box->center[1]);
	double wideZ = (double) sphere->center[2] - box->center[2];
	NxReal dz = (NxReal) wideZ;

	NxReal local0 = (NxReal) ((wideZ * m[6] + (double) dy * m[3]) + (double) dx * m[0]);
	NxReal local1 = (NxReal) (((double) dz * m[7] + (double) dy * m[4]) + (double) dx * m[1]);
	NxReal local2 = (NxReal) (((double) dz * m[8] + (double) dy * m[5]) + (double) dx * m[2]);

	NxReal clamped0 = local0;
	NxReal clamped1 = local1;
	bool clamped = false;

	if(local0 < -extents[0])
		{
		clamped0 = -extents[0];
		clamped = true;
		}
	else if(local0 > extents[0])
		{
		clamped0 = extents[0];
		clamped = true;
		}

	if(local1 < -extents[1])
		{
		clamped1 = -extents[1];
		clamped = true;
		}
	else if(local1 > extents[1])
		{
		clamped1 = extents[1];
		clamped = true;
		}

	double clamped2;
	if(local2 < -extents[2])
		clamped2 = -extents[2];
	else if(local2 > extents[2])
		clamped2 = extents[2];
	else
		{
		if(!clamped)
			return true;
		clamped2 = local2;
		}

	NxReal world0 = (NxReal) ((clamped2 * m[2] + (double) clamped1 * m[1]) + (double) clamped0 * m[0]);
	NxReal world1 = (NxReal) ((clamped2 * m[5] + (double) clamped1 * m[4]) + (double) clamped0 * m[3]);
	double world2 = (clamped2 * m[8] + (double) clamped1 * m[7]) + (double) clamped0 * m[6];

	double e0 = (double) dx - world0;
	double e1 = (double) dy - world1;
	double e2 = (double) dz - world2;
	double radius = sphere->radius;

	double squared = (e2 * e2 + e1 * e1) + e0 * e0;
	return !(radius * radius < squared);
	}

// phys_fn_001915 at 0x00049e70, matrix slot [SPHERE][BOX]. The copies are the
// oracle's: it flattens both shapes into two stack structures before the call.
bool __cdecl NxOverlapSphereBox(const NxCollisionShape* sphere, const NxCollisionShape* box)
	{
	NxCollisionSphereData sphereData;
	NxCollisionBoxData boxData;

	boxData.center[0] = box->translation[0];
	boxData.center[1] = box->translation[1];
	boxData.center[2] = box->translation[2];
	boxData.extents[0] = box->geometry[1];
	boxData.extents[1] = box->geometry[2];
	boxData.extents[2] = box->geometry[3];
	for(int i = 0; i < 9; ++i)
		boxData.rotation[i] = box->rotation[i];

	sphereData.center[0] = sphere->translation[0];
	sphereData.center[1] = sphere->translation[1];
	sphereData.center[2] = sphere->translation[2];
	sphereData.radius = sphere->geometry[0];

	return NxOverlapSphereBoxData(&sphereData, &boxData);
	}

// phys_fn_001921 at 0x0004a3e0, matrix slot [SPHERE][CAPSULE].
//
// The same half-axis construction as the plane/capsule entry, except that here
// every one of the six components is stored, because they have to be a segment
// the Foundation export can be handed. The sum of the two radii is narrowed to
// 32 bits before it is squared.
bool __cdecl NxOverlapSphereCapsule(const NxCollisionShape* sphere, const NxCollisionShape* capsule)
	{
	const NxReal* m = capsule->rotation;
	const NxReal* t = capsule->translation;

	NxReal halfHeight = capsule->geometry[1];
	NxReal axisX = (NxReal) ((double) m[1] * halfHeight);
	double axisY = (double) m[4] * halfHeight;
	double axisZ = (double) m[7] * halfHeight;
	NxReal negatedAxisZ = (NxReal) (-axisZ);

	NxSegment segment;
	segment.p0.x = (NxReal) (-(double) axisX + t[0]);
	segment.p0.y = (NxReal) (-axisY + t[1]);
	segment.p0.z = (NxReal) ((double) negatedAxisZ + t[2]);
	segment.p1.x = (NxReal) ((double) axisX + t[0]);
	segment.p1.y = (NxReal) (axisY + t[1]);
	segment.p1.z = (NxReal) (axisZ + t[2]);

	NxReal radius = (NxReal) ((double) sphere->geometry[0] + capsule->geometry[0]);

	NxVec3 center(sphere->translation[0], sphere->translation[1], sphere->translation[2]);
	NxReal parameter;
	double squared = NxComputeSquareDistance(segment, center, &parameter);

	return (double) radius * radius > squared;
	}

// phys_fn_001738 at 0x000389d0, matrix slot [BOX][BOX]. A copy of each shape
// into the argument triple the export wants, and `fullTest` always set.
bool __cdecl NxOverlapBoxBox(const NxCollisionShape* box0, const NxCollisionShape* box1)
	{
	NxVec3 center0(box0->translation[0], box0->translation[1], box0->translation[2]);
	NxVec3 extents0(box0->geometry[1], box0->geometry[2], box0->geometry[3]);
	NxMat33 rotation0;
	NxVec3 center1(box1->translation[0], box1->translation[1], box1->translation[2]);
	NxVec3 extents1(box1->geometry[1], box1->geometry[2], box1->geometry[3]);
	NxMat33 rotation1;

	rotation0.setRowMajor(box0->rotation);
	rotation1.setRowMajor(box1->rotation);

	return NxBoxBoxIntersect(extents0, center0, rotation0, extents1, center1, rotation1, true);
	}

// phys_fn_001690 at 0x00033e80, 1,836 bytes. A PHASE 2 row, disclosed rather
// than adopted: the census owns it as phase 2 and Phase 2's closure ledger
// still owns the row. It lives here because it has no translation unit of its
// own -- Phase 2 censused it shared_by_callers in the gap between
// IceAdjacencies.cpp and ContactConvexHeightfield.cpp -- and because the two
// callers in this component are matrix B [CAPSULE][CAPSULE] at 0x0003d890 and
// matrix A [CAPSULE][CAPSULE] at 0x0003d9d0, which calls it at 0x0003dc68.
// This file is on the /arch:IA32 list, which is what the row needs: it is only
// ever reached from inside the simulation step.
//
// The squared distance between two segments, with the closest-point parameter
// of each written back. The shape of it is the region decomposition of
//
//     Q(s,t) = a*s^2 + 2*b*s*t + c*t^2 + 2*d*s + 2*e*t + f
//
// over the unit square, with s and t carried scaled by the determinant until
// the interior case divides them down and every boundary case assigns 0 or 1
// directly. Nine leaves for the non-degenerate case and a second tree for the
// near-parallel one.
//
// Three things a reimplementation would not produce, all of which the
// differential drives:
//
//  * dir -- segment0's direction -- never touches memory. All three components
//    stay in x87 registers from 0x00033e8b to 0x00033f4b, so a, b and d are
//    formed at register precision from wide operands while c and e are formed
//    from the 32-bit copies of the other direction and the offset.
//  * s and t are not computed symmetrically. s at 0x00033fbc multiplies the
//    *wide* e still sitting in st(0) after the fst; t at 0x00033fd5 reloads the
//    narrowed copy. Mirroring the geometry end for end does not mirror the
//    answer.
//  * Four leaves keep the quotient they just divided in st(0) and use it wide
//    for the result while storing the narrowed copy as the parameter
//    (0x0003425c, 0x000343c2, 0x00034575, and 0x00034043 for t); two others
//    store it and read the narrowed value back (0x0003441a, 0x00034507). The
//    same expression is evaluated at two precisions in one function.
//
// It also stores past its own frame twice: t goes into the caller's first
// argument slot at 0x00033fdf and d into the second at 0x00033f43. Both are
// dead by then -- the two segment pointers were loaded at 0x00033e83 -- but
// both are writes into the caller's stack frame.
// One thing this transcription cannot reproduce, measured rather than assumed.
//
// Which x87 NaN comes out of a two-NaN operation is decided by the significand,
// with ties going to the *destination* operand, and MSVC chooses the destination
// for itself: it emitted `fsubr` where the oracle has `fsub` for the very first
// subtraction, and folded `x + (-y)` into `fsub` for the two negated dot
// products, and FSUB leaves a NaN operand's sign where FADD of an already
// negated one carries the flip. Three spellings were measured against the
// oracle -- the literal one below, one negated sum, and an explicit sign-bit
// flip -- and they moved 3,500, 3,500 and 5,956 parameter words respectively on
// non-finite inputs and **zero** on finite ones. No C++ names the destination of
// an x87 instruction, so this is the same kind of limit as the 64-bit
// significand: it is a property of the machine the source cannot state.
//
// The differential therefore compares NaN against NaN as NaN, and compares
// everything else bit for bit; the count of canonicalised words is registered,
// so the day the generator stops producing them the gate says so. What is still
// compared exactly is which leaf ran, whether a parameter is a NaN at all, and
// every finite value.

double __cdecl NxSegmentSegmentSquareDistance(const NxSegment* segment0,
	const NxSegment* segment1, NxReal* parameter0, NxReal* parameter1)
	{
	const double dirX = (double) segment0->p1.x - segment0->p0.x;
	const double dirY = (double) segment0->p1.y - segment0->p0.y;
	const double dirZ = (double) segment0->p1.z - segment0->p0.z;

	const NxReal otherX = (NxReal) ((double) segment1->p1.x - segment1->p0.x);
	const NxReal otherY = (NxReal) ((double) segment1->p1.y - segment1->p0.y);
	const NxReal otherZ = (NxReal) ((double) segment1->p1.z - segment1->p0.z);

	const NxReal offsetX = (NxReal) ((double) segment0->p0.x - segment1->p0.x);
	const NxReal offsetY = (NxReal) ((double) segment0->p0.y - segment1->p0.y);
	const NxReal offsetZ = (NxReal) ((double) segment0->p0.z - segment1->p0.z);

	const NxReal a = (NxReal) ((dirZ * dirZ + dirX * dirX) + dirY * dirY);
	const NxReal b = (NxReal) ((-dirZ * otherZ + -dirY * otherY) + -dirX * otherX);
	const NxReal c = (NxReal) (((double) otherZ * otherZ + (double) otherY * otherY)
		+ (double) otherX * otherX);
	const NxReal d = (NxReal) (((double) offsetZ * dirZ + (double) offsetY * dirY)
		+ (double) offsetX * dirX);
	// f stays in st(0) from 0x00033f67 to the faddp at 0x00034589 or one of the
	// fadd chains that end at 0x0003458b. Every leaf adds into it in place
	// rather than reloading it.
	const double f = ((double) offsetZ * offsetZ + (double) offsetY * offsetY)
		+ (double) offsetX * offsetX;

	// fabs at 0x00033f7b, then fst a narrowed copy and compare the *wide* one
	// against the epsilon at 0x00107938, which is 1e-5f. A NaN determinant
	// takes the near-parallel branch, because test ah,1 after fcomp reads C0
	// and the unordered result sets it.
	const double product = ((double) c * a) - ((double) b * b);
	const double wideDeterminant = product < 0.0 ? -product : product;
	const NxReal determinant = (NxReal) wideDeterminant;

	NxReal s = 0.0f;
	NxReal t = 0.0f;
	double result;

	if(wideDeterminant >= 1.0e-5f)
		{
		// e is computed here and not beside the other five: the near-parallel
		// branch at 0x000343cf skips it entirely and recomputes it in the two
		// of its own leaves that need it.
		const double wideE = (-(double) offsetZ * otherZ + -(double) offsetY * otherY)
			+ -(double) offsetX * otherX;
		const NxReal e = (NxReal) wideE;

		s = (NxReal) ((wideE * b) - ((double) d * c));
		t = (NxReal) (((double) d * b) - ((double) e * a));

		if(s >= 0.0f && s <= determinant)
			{
			if(t >= 0.0f && t <= determinant)
				{
				// 0x0003402d, the interior. The only leaf that divides both
				// parameters down and the only one that evaluates the full
				// quadratic. t is used wide for its first product and narrowed
				// for the two that follow.
				const double inverse = (double) 1.0f / determinant;
				s = (NxReal) ((double) s * inverse);
				const double wideT = inverse * t;
				t = (NxReal) wideT;
				const double half1 = ((wideT * c + (double) s * b) + ((double) e + e)) * t;
				const double half2 = (((double) t * b + (double) s * a) + ((double) d + d)) * s;
				result = f + (half1 + half2);
				}
			else if(t >= 0.0f)
				{
				// 0x00034083: t clamped to 1, s solved along segment0.
				const double edge = (double) d + b;
				t = 1.0f;
				if(edge >= 0.0)
					{
					s = 0.0f;
					result = (f + ((double) e + e)) + c;
					}
				else if(-edge >= a)
					{
					s = 1.0f;
					result = ((f + ((edge + e) + (edge + e))) + c) + a;
					}
				else
					{
					s = (NxReal) -(edge / a);
					result = (f + (edge * s + ((double) e + e))) + c;
					}
				}
			else
				{
				// 0x000340f3: t clamped to 0.
				t = 0.0f;
				if(d >= 0.0f)
					{
					s = 0.0f;
					result = f;
					}
				else if(-(double) d >= a)
					{
					s = 1.0f;
					result = (f + ((double) d + d)) + a;
					}
				else
					{
					// 0x0003425c keeps the quotient wide for the product and
					// stores only the narrowed copy as the parameter.
					const double quotient = -((double) d / a);
					s = (NxReal) quotient;
					result = f + quotient * d;
					}
				}
			}
		else if(s >= 0.0f)
			{
			// 0x0003411c: s past the far end. The fcomp qword at 0x00034120
			// compares t against a *double* zero where every other comparison
			// in this function uses a float one; the value is the same and the
			// operand size is not.
			bool clampS = false;
			if(t >= 0.0f && t <= determinant)
				clampS = true;
			else if(t >= 0.0f)
				{
				// 0x0003417b. The comparison against a uses the wide edge and
				// everything after it reloads the narrowed copy the fst at
				// 0x00034183 left in the determinant's own slot.
				const double wideEdge = (double) d + b;
				const NxReal edge = (NxReal) wideEdge;
				if(-wideEdge <= a)
					{
					t = 1.0f;
					if(edge >= 0.0f)
						{
						s = 0.0f;
						result = (f + ((double) e + e)) + c;
						}
					else
						{
						s = (NxReal) -((double) edge / a);
						result = (f + ((double) edge * s + ((double) e + e))) + c;
						}
					}
				else
					clampS = true;
				}
			else if(-(double) d < a)
				{
				// 0x0003422d
				t = 0.0f;
				if(d >= 0.0f)
					{
					s = 0.0f;
					result = f;
					}
				else
					{
					const double quotient = -((double) d / a);
					s = (NxReal) quotient;
					result = f + quotient * d;
					}
				}
			else
				clampS = true;

			if(clampS)
				{
				// 0x00034140: s clamped to 1, t solved along segment1. The
				// `fld [esp+0xc]` there reads the *narrowed* e, where the s
				// above multiplies the wide one still in st(0).
				const double edge = (double) e + b;
				s = 1.0f;
				if(edge >= 0.0)
					{
					t = 0.0f;
					result = (f + ((double) d + d)) + a;
					}
				else if(-edge >= c)
					{
					t = 1.0f;
					result = ((f + ((edge + d) + (edge + d))) + c) + a;
					}
				else
					{
					t = (NxReal) -(edge / c);
					result = (f + (edge * t + ((double) d + d))) + a;
					}
				}
			}
		else
			{
			// 0x0003428b: s negative.
			bool clampS = false;
			if(t >= 0.0f && t <= determinant)
				clampS = true;
			else if(t >= 0.0f)
				{
				// 0x000342d8. test ah,5 after fcom continues only on a strictly
				// negative edge; zero and unordered both fall into the s = 0
				// leaf below.
				const double edge = (double) d + b;
				if(edge < 0.0)
					{
					t = 1.0f;
					if(-edge >= a)
						{
						s = 1.0f;
						result = ((f + ((edge + e) + (edge + e))) + c) + a;
						}
					else
						{
						s = (NxReal) -(edge / a);
						result = (f + (edge * s + ((double) e + e))) + c;
						}
					}
				else
					clampS = true;
				}
			else if((double) d < 0.0)
				{
				// 0x00034348 into the shared tail at 0x00034365.
				t = 0.0f;
				if(-(double) d >= a)
					{
					s = 1.0f;
					result = (f + ((double) d + d)) + a;
					}
				else
					{
					const double quotient = -((double) d / a);
					s = (NxReal) quotient;
					result = f + quotient * d;
					}
				}
			else
				clampS = true;

			if(clampS)
				{
				// 0x000342af: s clamped to 0.
				s = 0.0f;
				if(e >= 0.0f)
					{
					t = 0.0f;
					result = f;
					}
				else if(-(double) e >= c)
					{
					t = 1.0f;
					result = (f + ((double) e + e)) + c;
					}
				else
					{
					const double quotient = -((double) e / c);
					t = (NxReal) quotient;
					result = f + quotient * e;
					}
				}
			}
		}
	else
		{
		// 0x000343cf, near-parallel. A second tree, and the two leaves that need
		// e recompute it -- one wide at 0x0003442d and one narrowed through the
		// same slot at 0x00034517.
		if(b > 0.0f)
			{
			if(d >= 0.0f)
				{
				s = 0.0f;
				t = 0.0f;
				result = f;
				}
			else if(-(double) d <= a)
				{
				// 0x00034410 stores the quotient and reads the narrowed value
				// back for the product, unlike its three siblings.
				s = (NxReal) -((double) d / a);
				t = 0.0f;
				result = f + (double) s * d;
				}
			else
				{
				// 0x0003442d: e recomputed and used wide.
				const double wideE = (-(double) offsetZ * otherZ + -(double) offsetY * otherY)
					+ -(double) offsetX * otherX;
				const double edge = (double) d + a;
				s = 1.0f;
				if(-edge >= b)
					{
					const double sum = (wideE + d) + b;
					t = 1.0f;
					result = ((f + (sum + sum)) + c) + a;
					}
				else
					{
					t = (NxReal) -(edge / b);
					const double doubled = (wideE + b) + (wideE + b);
					result = (f + ((doubled + (double) t * c) * t + ((double) d + d))) + a;
					}
				}
			}
		else if(-(double) d >= a)
			{
			s = 1.0f;
			t = 0.0f;
			result = (f + ((double) d + d)) + a;
			}
		else if((double) d <= 0.0)
			{
			s = (NxReal) -((double) d / a);
			t = 0.0f;
			result = f + (double) s * d;
			}
		else
			{
			// 0x00034517: e recomputed and narrowed through its own slot.
			const NxReal e = (NxReal) ((-(double) offsetZ * otherZ + -(double) offsetY * otherY)
				+ -(double) offsetX * otherX);
			s = 0.0f;
			if(-(double) b <= d)
				{
				t = 1.0f;
				result = (f + ((double) e + e)) + c;
				}
			else
				{
				const double quotient = -((double) d / b);
				t = (NxReal) quotient;
				result = f + ((quotient * c) + ((double) e + e)) * t;
				}
			}
		}

	// Both output pointers are optional -- test eax,eax at 0x0003458f and
	// 0x0003459c -- and both are written with integer moves, so what the caller
	// sees is exactly the 32-bit slot and not a value that has been through the
	// FPU again.
	if(parameter0)
		*parameter0 = s;
	if(parameter1)
		*parameter1 = t;
	// 0x000345a6, and it is a sign clear rather than a test, which is what
	// makes a negative NaN come back positive.
	return fabs(result);
	}
