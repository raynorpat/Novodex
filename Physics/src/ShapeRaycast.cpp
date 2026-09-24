/*----------------------------------------------------------------------------*\
|
|							NovodeX Technology
|
|                             www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ContactGeneration.h"
#include "NxIntersectionRaySphere.h"
#include "NxIntersectionSegmentCapsule.h"

// The original row executes x87 fsqrt. Its result must follow the live
// control word, including the simulation step's 0x0f7f setting.
static double nxSqrt(double value)
	{
	double result;
	__asm
		{
		fld value
		fsqrt
		fstp result
		}
	return result;
	}

// phys_fn_001377 at 0x00027c70: what a SPHERE shape puts in vtable slot 5.
//
// Its only callee is NxRaySphereIntersect at 0x00036e80 (phys_fn_001710), a
// Task 2 export. Read alongside the plane's copy the two are not the same
// function with a different intersector:
//
//   * there is no facing test and no "ahead of the origin" test here, only the
//     distance limit, so a sphere behind the ray origin is whatever
//     NxRaySphereIntersect says it is;
//   * hit.distance is written *before* that limit is tested (`fst` at
//     0x00027cc9, `fcomp` at 0x00027ccc), so a raycast rejected for being too
//     far still leaves the distance in the caller's hit record;
//   * the comparison uses the wide value still in st(0) while the hit gets the
//     narrowed copy, and those are not the same number;
//   * the normal is computed and normalised rather than copied out of the
//     shape, and a zero-length one is left unnormalised with the
//     NX_RAYCAST_NORMAL bit still set (0x00027d94).
const NxCollisionShape* __fastcall NxShapeRaycastSphere(const NxCollisionShape* sphere,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit)
	{
	(void) edxUnused;
	(void) unread;
	const NxVec3* centre = (const NxVec3*) sphere->translation;

	if(!NxRaySphereIntersect(worldRay->orig, worldRay->dir, *centre,
			sphere->geometry[0], &hit->worldImpact))
		return 0;

	// |origin - impact|, from the register copies of the three differences and
	// accumulated z, y, x -- 0x00027ca1..0x00027cc1.
	const double toImpactX = (double) worldRay->orig.x - hit->worldImpact.x;
	const double toImpactY = (double) worldRay->orig.y - hit->worldImpact.y;
	const double toImpactZ = (double) worldRay->orig.z - hit->worldImpact.z;
	const double distance = nxSqrt((toImpactZ * toImpactZ + toImpactY * toImpactY)
		+ toImpactX * toImpactX);

	hit->distance = (NxReal) distance;
	if(distance > (double) maxDistance)
		return 0;

	hit->shape = (NxShape*) sphere->collisionObject;
	hit->faceID = 0;
	hit->u = 0.0f;
	hit->v = 0.0f;
	hit->flags = NX_RAYCAST_SHAPE | NX_RAYCAST_IMPACT | NX_RAYCAST_DISTANCE;
	if(hintFlags & NX_RAYCAST_NORMAL)
		{
		// impact - centre, stored into the hit and read back: the length is
		// formed from the narrowed components at 0x00027d32..0x00027d38 and not
		// from the registers that produced them, and its terms run x, y, z
		// where the distance above runs z, y, x.
		hit->worldNormal.x = (NxReal) ((double) hit->worldImpact.x - centre->x);
		hit->worldNormal.y = (NxReal) ((double) hit->worldImpact.y - centre->y);
		hit->worldNormal.z = (NxReal) ((double) hit->worldImpact.z - centre->z);
		const double length = nxSqrt(((double) hit->worldNormal.x * hit->worldNormal.x
			+ (double) hit->worldNormal.y * hit->worldNormal.y)
			+ (double) hit->worldNormal.z * hit->worldNormal.z);
		// `fucompp` against the 0.0f at 0x101041f0 with `jnp`: equal skips the
		// normalisation, unordered takes it.
		if(length != 0.0)
			{
			const double inverse = 1.0 / length;
			hit->worldNormal.x = (NxReal) (inverse * hit->worldNormal.x);
			hit->worldNormal.y = (NxReal) (inverse * hit->worldNormal.y);
			hit->worldNormal.z = (NxReal) (inverse * hit->worldNormal.z);
			}
		hit->flags |= NX_RAYCAST_NORMAL;
		}
	return sphere;
	}

// phys_fn_001010 at 0x00022480, slot 5 of a CAPSULE shape's vtable
// (0x00106b20 + 0x14). Same ABI note as the two above.
//
// A third convention across three slot-5 rows, and the one that matters: the
// plane's copies the plane geometry into hit.worldNormal under a flag test, the
// sphere's computes and normalises one under the same test, and this one has no
// normal branch at all. `flags` is the literal 0x13 at 0x000225b0 -- SHAPE,
// IMPACT and DISTANCE -- with no `|4` anywhere, and nothing in the 321 bytes
// writes hit+0x10..+0x18 under any hint flags. That is not an oversight to fix
// here; phys_fn_001775 depends on it, in the worst way. See the comment on
// NxContactCapsuleCapsule.
const NxCollisionShape* __fastcall NxShapeRaycastCapsule(const NxCollisionShape* capsule,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit)
	{
	(void) edxUnused;
	(void) unread;
	(void) hintFlags;

	const NxReal* m = capsule->rotation;
	const NxReal* translation = capsule->translation;
	const NxReal halfHeight = capsule->geometry[1];

	// The same column-1 axis, the same narrowing of the x half-axis and the same
	// narrowed -axisZ as the three capsule entries already closed.
	const NxReal axisX = (NxReal) ((double) m[1] * halfHeight);
	const double axisY = (double) m[4] * halfHeight;
	const double axisZ = (double) m[7] * halfHeight;
	const NxReal negatedAxisZ = (NxReal) (-axisZ);

	NxCapsule body;
	body.p0.x = (NxReal) (-(double) axisX + translation[0]);
	body.p0.y = (NxReal) (-axisY + translation[1]);
	body.p0.z = (NxReal) ((double) negatedAxisZ + translation[2]);
	body.p1.x = (NxReal) ((double) axisX + translation[0]);
	body.p1.y = (NxReal) (axisY + translation[1]);
	body.p1.z = (NxReal) (axisZ + translation[2]);
	// An integer move at 0x000224a4/0x000224d8, so the radius arrives exactly as
	// the shape holds it.
	body.radius = capsule->geometry[0];

	NxReal roots[2];
	const NxU32 count = NxRayCapsuleIntersect(worldRay->orig, worldRay->dir, body, roots);
	if(count == 0)
		return 0;

	// Three ways, not two: one root is taken directly and anything else compares
	// the pair and takes the smaller, with the unordered case taking the second.
	double distance;
	if(count == 1)
		distance = roots[0];
	else
		distance = roots[0] < roots[1] ? roots[0] : roots[1];

	// `test ah,0x41` + `jne`: less, equal or unordered all continue.
	if(distance > maxDistance)
		return 0;

	const NxReal scaledY = (NxReal) (distance * worldRay->dir.y);
	const NxReal scaledZ = (NxReal) (distance * worldRay->dir.z);
	hit->worldImpact.x = (NxReal) (distance * worldRay->dir.x + worldRay->orig.x);
	hit->worldImpact.y = (NxReal) ((double) scaledY + worldRay->orig.y);
	hit->worldImpact.z = (NxReal) ((double) scaledZ + worldRay->orig.z);
	hit->distance = (NxReal) distance;
	hit->shape = (NxShape*) capsule->collisionObject;
	hit->faceID = 0;
	hit->u = 0.0f;
	hit->v = 0.0f;
	hit->flags = 0x13;
	// hit->worldNormal is deliberately not written. See above.
	return capsule;
	}
