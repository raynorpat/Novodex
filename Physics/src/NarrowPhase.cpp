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
