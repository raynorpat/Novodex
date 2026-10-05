/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The Scene's raycasts (the oracle's `\Epic\Novodex\SDKs\Physics\src\
// SceneRaycast.cpp`, 0x00015520-0x00015c16) and the three loop helpers the
// compiler placed just before them (0x00015060-0x000153bf, in the gap after
// Scene.cpp). Scene-raycast block, Task 3; the contract is
// docs/reconstruction/novodex-physics/units/scene-raycast-contract.md.
//
// Every query has the same shape. The ray's direction must be a unit vector
// (|d.d - 1| < 0.0001, a double; otherwise error 1 at this file's line and a
// null result). The result collector at +0x500 (a container whose buffer the
// Scene lends it, phys_fn_000503) is reset, the pruning engine at +0x624 walks
// the pruners the shapes type selects -- bit 0 the static pruner, 0xe the
// dynamic ones -- through their raycast slot (phys_fn_004864), and each pruner
// adds the prunables whose world box the ray reaches. A loop helper then walks
// the collector: a shape is skipped when it has raycasting disabled (+0xde bit
// 0x40) or when its group (+0xd8, 0xffff = none) is not in `groups`; the
// bounds queries intersect the shape's world box (NxRayAABBIntersect), the
// shape queries call its slot 5.
//
// The loop helpers take their arguments in registers in the image (eax, ebx,
// ecx, esi) and are called directly by one row each, never through a table;
// here they are ordinary static functions, a difference of code shape only.
// They are kept out of line, as the image's are.
//
// PRECISION. These rows run at API time, under the control word 0x027f. The
// x87 register lifetimes are written `double`; the square roots go through
// X87Sqrt.h. SSE2 double arithmetic is the x87's at 53 bits, so this file
// keeps the default architecture.

#include "Scene.h"
#include "NxRay.h"
#include "NxShape.h"
#include "NxBoxShape.h"
#include "NxCapsuleShape.h"
#include "NxSphereShape.h"
#include "NxPlaneShape.h"
#include "NxUserRaycastReport.h"
#include "NxUserEntityReport.h"
#include "NxIntersectionSegmentBox.h"
#include "NxBounds3.h"
#include "NxSphere.h"
#include "NxPlane.h"
#include "NxTriangle.h"
#include "NxTriangleMesh.h"
#include "NxTriangleMeshShape.h"
#include "NxActor.h"
#include "Containers.h"
#include "FoundationSDK.h"
#include "X87Sqrt.h"
#include "ObjectModel.h"
#include "opcode/IcePruner.h"

#include <string.h>

// The pruner table slot the engine loop dispatches through (opcode/IcePruner.cpp).
NxSlotMfp5 nxPrunerRaycastSlot();

static const char* const kSceneRaycastFile = "\\Epic\\Novodex\\SDKs\\Physics\\src\\SceneRaycast.cpp";
static const char* const kSceneRaycastBadRay = "NxRay direction not valid: must be unit vector.";

// A shape's slot 5: thiscall (ray, maxDist, groups, hintFlags, hit), `ret
// 0x14`, returning the shape on a hit. The unused edx makes __fastcall the
// same ABI (see ContactGeneration.h).
typedef void* (__fastcall* NxShapeRaycastSlot)(void* shape, void* edxUnused, const NxRay* ray,
	NxReal maxDist, NxU32 groups, NxU32 hintFlags, NxRaycastHit* hit);

static inline void* nxShapeRaycast(unsigned char* shape, const NxRay& ray, NxReal maxDist,
	NxU32 groups, NxU32 hintFlags, NxRaycastHit* hit)
	{
	void** table = *reinterpret_cast<void***>(shape);
	return reinterpret_cast<NxShapeRaycastSlot>(table[5])(shape, 0, &ray, maxDist, groups, hintFlags, hit);
	}

// The collector's entries are prunables; each one's owner (+0x04) is the shape.
static inline unsigned char* nxCollectedShape(const NxU32* entry)
	{
	return static_cast<unsigned char*>(reinterpret_cast<Prunable*>(*entry)->mOwner);
	}

// The filter every loop applies first.
static inline bool nxRaycastSkipsShape(const unsigned char* shape, NxU32 groups)
	{
	if(shape[0xde] & 0x40)
		return true;
	const NxU16 group = *reinterpret_cast<const NxU16*>(shape + 0xd8);
	return group != 0xffff && !(groups & (1u << (group & 31)));
	}

// The shape's world box through its prunable (+0xa4), recomputed first when
// stale (phys_fn_004886); null when the prunable has no handle.
static inline const NxVec3* nxRaycastWorldBox(unsigned char* shape)
	{
	Prunable* prunable = reinterpret_cast<Prunable*>(shape + 0xa4);
	PruningPool& pool = reinterpret_cast<Pruner*>(*reinterpret_cast<void**>(shape + 0xc4))->mPool;
	if(prunable->mHandle == PRUNABLE_INVALID_HANDLE)
		return 0;
	if(!(prunable->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
		prunable->UpdateWorldAABB(&pool.mWorldBoxes[prunable->mHandle]);
	return reinterpret_cast<const NxVec3*>(&pool.mWorldBoxes[prunable->mHandle]);
	}

// The unit-direction test every row starts with (0x00015525-0x0001555a).
static inline bool nxRaycastDirectionIsUnit(const NxRay& ray)
	{
	const double x = ray.dir.x;
	const double y = ray.dir.y;
	const double z = ray.dir.z;
	return fabs(((x * x + y * y) + z * z) - 1.0f) < 0.0001;
	}

// The pruner mask the shapes type selects, and the engine walk
// (0x0001558a-0x000155cd).
static inline SdkContainer& nxRaycastCollect(NxSceneInternal* scene, const NxRay& ray,
	NxShapesType shapesType, NxReal maxDist)
	{
	SdkContainer& collector = scene->at<SdkContainer>(0x500);
	if(collector.mCount)
		collector.mCount = 0;
	NxU32 mask = (shapesType & NX_STATIC_SHAPES) ? 1 : 0;
	if(shapesType & NX_DYNAMIC_SHAPES)
		mask |= 0xe;
	NxU32 maxDistBits;
	memcpy(&maxDistBits, &maxDist, 4);
	nxMaskedFourSlotLoop4864(&scene->at<unsigned char>(0x624), reinterpret_cast<unsigned>(&collector),
		reinterpret_cast<unsigned>(&ray), mask, maxDistBits, 0, 0xffffffff, nxPrunerRaycastSlot());
	return collector;
	}

static inline void nxRaycastBadRay(int line)
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, kSceneRaycastFile, line, 0,
		kSceneRaycastBadRay);
	}

// ---------------------------------------------------------------------------
// The loops.

// phys_fn_000680 (0x00015060, 235 B)
// True at the first shape whose world box the ray enters within maxDist: the
// entry point's distance is the root of NxComputeDistanceSquared (an import,
// its float result widened), compared as `<=` (0x0001512a `test ah,0x41;
// jnp`, so a NaN distance is not a hit).
static __declspec(noinline) bool nxRaycastAnyBoundsLoop(const SdkContainer& collector, const NxRay& ray, NxReal maxDist,
	NxU32 groups)
	{
	const NxU32* entry = collector.mEntries;
	NxU32 count = collector.mCount;
	while(count)
		{
		unsigned char* shape = nxCollectedShape(entry);
		count--;
		entry++;
		if(nxRaycastSkipsShape(shape, groups))
			continue;
		const NxVec3* box = nxRaycastWorldBox(shape);
		NxVec3 point;
		if(!NxRayAABBIntersect(box[0], box[1], ray.orig, ray.dir, point))
			continue;
		if(x87Fsqrt(NxComputeDistanceSquared(ray, point, 0)) <= maxDist)
			return true;
		}
	return false;
	}

// phys_fn_000682 (0x00015150, 399 B)
// Every shape whose world box the ray enters within maxDist is reported with
// its shape, the entry point, face 0, u = v = 0, flags 0x13 and the entry
// point's distance, formed again in registers ((dz dz + dy dy) + dx dx with
// d = origin - point) and rooted (0x00015238-0x000152ba). The normal words are
// not written. A NaN distance is kept (0x00015232 skips only when greater).
// The count of reports is returned; the report ends the walk by returning
// false.
static __declspec(noinline) NxU32 nxRaycastAllBoundsLoop(const SdkContainer& collector, const NxRay& ray, NxReal maxDist,
	NxUserRaycastReport& report, NxU32 groups)
	{
	NxU32 reported = 0;
	const NxU32* entry = collector.mEntries;
	NxU32 count = collector.mCount;
	while(count)
		{
		unsigned char* shape = nxCollectedShape(entry);
		count--;
		entry++;
		if(nxRaycastSkipsShape(shape, groups))
			continue;
		const NxVec3* box = nxRaycastWorldBox(shape);
		NxVec3 point;
		if(!NxRayAABBIntersect(box[0], box[1], ray.orig, ray.dir, point))
			continue;
		if(x87Fsqrt(NxComputeDistanceSquared(ray, point, 0)) > maxDist)
			continue;
		const double dx = double(ray.orig.x) - point.x;
		const double dy = double(ray.orig.y) - point.y;
		const double dz = double(ray.orig.z) - point.z;
		NxRaycastHit hit;
		hit.shape = *reinterpret_cast<NxShape**>(shape + 0x9c);
		hit.worldImpact = point;
		reported++;
		hit.faceID = 0;
		hit.u = 0.0f;
		hit.v = 0.0f;
		hit.flags = NX_RAYCAST_SHAPE | NX_RAYCAST_IMPACT | NX_RAYCAST_DISTANCE;
		hit.distance = float(x87FsqrtDot3(dz, dz, dy, dy, dx, dx));
		if(!report.onHit(hit))
			break;
		}
	return reported;
	}

// phys_fn_000684 (0x000152e0, 29 B)
// phys_fn_000686 (0x00015300, 191 B)
// One function the splitter cut in two (000684 is its entry, 000686 its body).
// Every shape the ray hits (slot 5 with the caller's maxDist, groups and hint
// flags) is reported with its public shape, flags | 0x11 and a distance
// recomputed from the hit's impact words ((dz dz + dy dy) + dx dx with
// d = origin - impact, rooted, 0x0001534e-0x000153a3), whether or not the slot
// declared an impact. Returns the count of reports; the report ends the walk by
// returning false.
static __declspec(noinline) NxU32 nxRaycastAllShapesLoop(const SdkContainer& collector, const NxRay& ray, NxU32 groups,
	NxReal maxDist, NxUserRaycastReport& report, NxU32 hintFlags)
	{
	NxU32 reported = 0;
	const NxU32* entry = collector.mEntries;
	NxU32 count = collector.mCount;
	while(count)
		{
		unsigned char* shape = nxCollectedShape(entry);
		count--;
		entry++;
		if(nxRaycastSkipsShape(shape, groups))
			continue;
		NxRaycastHit hit;
		unsigned char* hitShape = static_cast<unsigned char*>(
			nxShapeRaycast(shape, ray, maxDist, groups, hintFlags, &hit));
		if(!hitShape)
			continue;
		const double dx = double(ray.orig.x) - hit.worldImpact.x;
		const double dy = double(ray.orig.y) - hit.worldImpact.y;
		const double dz = double(ray.orig.z) - hit.worldImpact.z;
		hit.shape = *reinterpret_cast<NxShape**>(hitShape + 0x9c);
		reported++;
		hit.flags |= NX_RAYCAST_SHAPE | NX_RAYCAST_DISTANCE;
		hit.distance = float(x87FsqrtDot3(dz, dz, dy, dy, dx, dx));
		if(!report.onHit(hit))
			break;
		}
	return reported;
	}

// phys_fn_000688 (0x000153c0, 339 B)
// The nearest world box the ray enters within maxDist. The distance test
// skips only a greater rooted distance, as in 000682 (0x000154a1), but the key
// kept is the SQUARED
// distance ((dy dy + dz dz) + dx dx with d = origin - point,
// 0x000154a3-0x000154c5), compared strictly against hit.distance and stored
// squared; raycastClosestBounds roots it at the end. A kept box writes the
// shape, the entry point and flags 0x13.
static __declspec(noinline) void nxRaycastClosestBoundsLoop(const SdkContainer& collector, NxRaycastHit& hit, const NxRay& ray,
	NxReal maxDist, NxU32 groups)
	{
	const NxU32* entry = collector.mEntries;
	NxU32 count = collector.mCount;
	if(!count)
		return;
	do
		{
		unsigned char* shape = nxCollectedShape(entry);
		entry++;
		if(nxRaycastSkipsShape(shape, groups))
			continue;
		const NxVec3* box = nxRaycastWorldBox(shape);
		NxVec3 point;
		if(!NxRayAABBIntersect(box[0], box[1], ray.orig, ray.dir, point))
			continue;
		if(x87Fsqrt(NxComputeDistanceSquared(ray, point, 0)) > maxDist)
			continue;
		const double dx = double(ray.orig.x) - point.x;
		const double dy = double(ray.orig.y) - point.y;
		const double dz = double(ray.orig.z) - point.z;
		const double squared = (dy * dy + dz * dz) + dx * dx;
		if(squared < hit.distance)
			{
			hit.distance = float(squared);
			hit.shape = *reinterpret_cast<NxShape**>(shape + 0x9c);
			hit.worldImpact = point;
			hit.flags = NX_RAYCAST_SHAPE | NX_RAYCAST_IMPACT | NX_RAYCAST_DISTANCE;
			}
		}
	while(--count);
	}

// phys_fn_000698 (0x00015890, 126 B)
// True at the first shape whose slot 5 (hint flags 0) reports a hit.
static __declspec(noinline) bool nxRaycastAnyShapeLoop(const SdkContainer& collector, const NxRay& ray, NxReal maxDist,
	NxU32 groups)
	{
	const NxU32* entry = collector.mEntries;
	NxU32 count = collector.mCount;
	while(count)
		{
		unsigned char* shape = nxCollectedShape(entry);
		count--;
		entry++;
		if(nxRaycastSkipsShape(shape, groups))
			continue;
		NxRaycastHit hit;
		if(nxShapeRaycast(shape, ray, maxDist, groups, 0, &hit))
			return true;
		}
	return false;
	}

// phys_fn_000700 (0x00015910, 29 B)
// phys_fn_000702 (0x00015930, 277 B)
// One function the splitter cut in two (000700 is its entry, 000702 its body).
// Each shape's slot 5 fills a local hit. Its key is the SQUARED distance to
// the impact ((dz dz + dy dy) + dx dx with d = impact - origin) when the slot
// declared an impact, else its raw distance when it declared one, else FLT_MAX
// (0x0001598a-0x000159ca); a key strictly below hit.distance copies the local
// hit's words over the caller's, stores the key as the distance, and sets the
// public shape and flags | 0x11. raycastClosestShape roots the distance at the
// end, whichever kind of key was kept.
static __declspec(noinline) void nxRaycastClosestShapeLoop(const SdkContainer& collector, const NxRay& ray, NxRaycastHit& hit,
	NxReal maxDist, NxU32 groups, NxU32 hintFlags)
	{
	const NxU32* entry = collector.mEntries;
	NxU32 count = collector.mCount;
	if(!count)
		return;
	do
		{
		unsigned char* shape = nxCollectedShape(entry);
		entry++;
		if(nxRaycastSkipsShape(shape, groups))
			continue;
		NxRaycastHit local;
		unsigned char* hitShape = static_cast<unsigned char*>(
			nxShapeRaycast(shape, ray, maxDist, groups, hintFlags, &local));
		if(!hitShape)
			continue;
		const NxU32 flags = local.flags;
		double key = NX_MAX_F32;
		if(flags & NX_RAYCAST_IMPACT)
			{
			const double dx = double(local.worldImpact.x) - ray.orig.x;
			const double dy = double(local.worldImpact.y) - ray.orig.y;
			const double dz = double(local.worldImpact.z) - ray.orig.z;
			key = (dz * dz + dy * dy) + dx * dx;
			}
		else if(flags & NX_RAYCAST_DISTANCE)
			key = local.distance;
		if(key < hit.distance)
			{
			hit.shape = local.shape;
			hit.worldImpact = local.worldImpact;
			hit.worldNormal = local.worldNormal;
			hit.distance = float(key);
			hit.faceID = local.faceID;
			hit.u = local.u;
			hit.flags = flags;
			hit.v = local.v;
			hit.shape = *reinterpret_cast<NxShape**>(hitShape + 0x9c);
			hit.flags = flags | NX_RAYCAST_SHAPE | NX_RAYCAST_DISTANCE;
			}
		}
	while(--count);
	}

// ---------------------------------------------------------------------------
// The rows the NxScene wrappers call.

// phys_fn_000690 (0x00015520, 202 B)
bool NxSceneInternal::raycastAnyBounds(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups,
	NxReal maxDist)
	{
	if(!nxRaycastDirectionIsUnit(worldRay))
		{
		nxRaycastBadRay(0x147);
		return false;
		}
	const SdkContainer& collector = nxRaycastCollect(this, worldRay, shapesType, maxDist);
	return nxRaycastAnyBoundsLoop(collector, worldRay, maxDist, groups);
	}

// phys_fn_000692 (0x000155f0, 206 B)
// hintFlags is not read.
NxU32 NxSceneInternal::raycastAllBounds(const NxRay& worldRay, NxUserRaycastReport& report,
	NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 /*hintFlags*/)
	{
	if(!nxRaycastDirectionIsUnit(worldRay))
		{
		nxRaycastBadRay(0x170);
		return 0;
		}
	const SdkContainer& collector = nxRaycastCollect(this, worldRay, shapesType, maxDist);
	return nxRaycastAllBoundsLoop(collector, worldRay, maxDist, report, groups);
	}

// phys_fn_000694 (0x000156c0, 207 B)
// Always returns 0 (0x00015789 `xor eax,eax`): the loop's count is dropped.
NxU32 NxSceneInternal::raycastAllShapes(const NxRay& worldRay, NxUserRaycastReport& report,
	NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags)
	{
	if(!nxRaycastDirectionIsUnit(worldRay))
		{
		nxRaycastBadRay(0x182);
		return 0;
		}
	const SdkContainer& collector = nxRaycastCollect(this, worldRay, shapesType, maxDist);
	nxRaycastAllShapesLoop(collector, worldRay, groups, maxDist, report, hintFlags);
	return 0;
	}

// phys_fn_000696 (0x00015790, 245 B)
// The hit starts as distance FLT_MAX, no shape, flags 1 (after the direction
// test, before the collector is reset). A kept box leaves the squared
// distance, which is rooted here; the internal shape is returned. hintFlags
// is not read.
void* NxSceneInternal::raycastClosestBounds(const NxRay& worldRay, NxShapesType shapeType,
	NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 /*hintFlags*/)
	{
	if(!nxRaycastDirectionIsUnit(worldRay))
		{
		nxRaycastBadRay(0x19b);
		return 0;
		}
	hit.distance = NX_MAX_F32;
	hit.shape = 0;
	hit.flags = NX_RAYCAST_SHAPE;
	const SdkContainer& collector = nxRaycastCollect(this, worldRay, shapeType, maxDist);
	nxRaycastClosestBoundsLoop(collector, hit, worldRay, maxDist, groups);
	if(!hit.shape)
		return 0;
	hit.distance = float(x87Fsqrt(hit.distance));
	return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(hit.shape) + 8);
	}

// phys_fn_000704 (0x00015a50, 202 B)
bool NxSceneInternal::raycastAnyShape(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups,
	NxReal maxDist)
	{
	if(!nxRaycastDirectionIsUnit(worldRay))
		{
		nxRaycastBadRay(0x159);
		return false;
		}
	const SdkContainer& collector = nxRaycastCollect(this, worldRay, shapesType, maxDist);
	return nxRaycastAnyShapeLoop(collector, worldRay, maxDist, groups);
	}

// phys_fn_000706 (0x00015b20, 249 B)
// As raycastClosestBounds, over the shapes' slot 5.
void* NxSceneInternal::raycastClosestShape(const NxRay& worldRay, NxShapesType shapeType,
	NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags)
	{
	if(!nxRaycastDirectionIsUnit(worldRay))
		{
		nxRaycastBadRay(0x1bb);
		return 0;
		}
	hit.distance = NX_MAX_F32;
	hit.shape = 0;
	hit.flags = NX_RAYCAST_SHAPE;
	const SdkContainer& collector = nxRaycastCollect(this, worldRay, shapeType, maxDist);
	nxRaycastClosestShapeLoop(collector, worldRay, hit, maxDist, groups, hintFlags);
	if(!hit.shape)
		return 0;
	hit.distance = float(x87Fsqrt(hit.distance));
	return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(hit.shape) + 8);
	}

// phys_fn_000674 (0x000148d0). The image sends an AABB query through pruning
// engine slot 8 and returns whether its shared collector is nonempty. The
// reconstructed pruner table does not yet implement that query slot, so walk
// the same selected pruner pools and apply the stored world-AABB overlap test.
// This keeps the public result on the scene's registered prunables and updates
// stale bounds through the prunable's recovered update path.
bool NxSceneInternal::checkOverlapAABB(const NxBounds3& worldBounds, NxShapesType shapesType)
	{
	const NxU32 mask = ((shapesType & NX_STATIC_SHAPES) ? 1u : 0u) |
		((shapesType & NX_DYNAMIC_SHAPES) ? 0xeu : 0u);
	Pruner** const pruners = reinterpret_cast<Pruner**>(bytes() + 0x624 + 0x1c);
	for(NxU32 type = 0; type < 4; type++)
		{
		if(!(mask & (1u << type)) || !pruners[type])
			continue;
		PruningPool& pool = pruners[type]->mPool;
		for(NxU32 i = 0; i < pool.mNbTotal; i++)
			{
			Prunable* const prunable = pool.mObjects[i];
			// Plane prunables use an unbounded world box in the oracle query path.
			// Their cached pool box is not the query representation, so any AABB
			// query that selects the owning pruner reports a possible overlap.
			const unsigned char* const shape = static_cast<const unsigned char*>(prunable->mOwner);
			if(shape && *reinterpret_cast<const NxU32*>(shape + 0xd0) == NX_SHAPE_PLANE)
				return true;
			if(!(prunable->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
				prunable->UpdateWorldAABB(&pool.mWorldBoxes[i]);
			const NxBounds3& shapeBounds = *reinterpret_cast<const NxBounds3*>(&pool.mWorldBoxes[i]);
			if(worldBounds.intersects(shapeBounds))
				return true;
			}
		}
	return false;
	}

static bool nxTriangleSeparatesAABB(const NxVec3& axis, const NxVec3& v0,
	const NxVec3& v1, const NxVec3& v2, const NxVec3& extents)
	{
	if(axis.magnitudeSquared() == 0.0f)
		return false;
	const NxReal p0 = v0.dot(axis);
	const NxReal p1 = v1.dot(axis);
	const NxReal p2 = v2.dot(axis);
	const NxReal minimum = NxMath::min(p0, NxMath::min(p1, p2));
	const NxReal maximum = NxMath::max(p0, NxMath::max(p1, p2));
	const NxReal radius = extents.x * NxMath::abs(axis.x)
		+ extents.y * NxMath::abs(axis.y) + extents.z * NxMath::abs(axis.z);
	return minimum > radius || maximum < -radius;
	}

static bool nxSceneTriangleBoundsOverlap(const NxTriangle& triangle, const NxBounds3& bounds)
	{
	if(bounds.isEmpty())
		return false;
	NxVec3 center;
	NxVec3 dimensions;
	bounds.getCenter(center);
	bounds.getDimensions(dimensions);
	const NxVec3 extents = dimensions * 0.5f;
	const NxVec3 v0 = triangle.verts[0] - center;
	const NxVec3 v1 = triangle.verts[1] - center;
	const NxVec3 v2 = triangle.verts[2] - center;
	const NxVec3 edges[3] = { v1 - v0, v2 - v1, v0 - v2 };
	const NxVec3 boxAxes[3] =
		{ NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f), NxVec3(0.0f, 0.0f, 1.0f) };
	for(unsigned axis = 0; axis < 3; ++axis)
		if(nxTriangleSeparatesAABB(boxAxes[axis], v0, v1, v2, extents))
			return false;
	if(nxTriangleSeparatesAABB(edges[0] ^ edges[1], v0, v1, v2, extents))
		return false;
	for(unsigned edge = 0; edge < 3; ++edge)
		for(unsigned axis = 0; axis < 3; ++axis)
			if(nxTriangleSeparatesAABB(edges[edge] ^ boxAxes[axis], v0, v1, v2, extents))
				return false;
	return true;
	}

// The scene query walks its registered actor/shape set and appends cooked mesh
// triangles in the model's observed right-to-left leaf order.
NxU32 NxSceneInternal::overlapAABBTriangles(const NxBounds3& worldBounds,
	NxArraySDK<NxTriangle>& worldTriangles)
	{
	NxActor** const actors = at<NxActor**>(0x55c);
	NxActor** const actorsEnd = at<NxActor**>(0x560);
	const NxU32 actorCount = actors && actorsEnd
		? static_cast<NxU32>(actorsEnd - actors) : 0;
	for(NxU32 actorIndex = 0; actorIndex < actorCount; ++actorIndex)
		{
		NxActor* const actor = actors[actorIndex];
		const NxU32 shapeCount = actor->getNbShapes();
		NxShape* const* shapes = actor->getShapes();
		for(NxU32 shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex)
			{
			NxShape* const shape = shapes[shapeIndex];
			if(shape->getType() != NX_SHAPE_MESH)
				continue;
			NxTriangleMesh* const mesh = &shape->isTriangleMesh()->getTriangleMesh();
			const NxMat34 pose = shape->getGlobalPose();
			const NxU32 submeshCount = mesh->getSubmeshCount();
			for(NxU32 submesh = 0; submesh < submeshCount; ++submesh)
				{
				const NxU32 vertexCount = mesh->getCount(submesh, NX_ARRAY_VERTICES);
				const NxU32 triangleCount = mesh->getCount(submesh, NX_ARRAY_TRIANGLES);
				const NxU32 vertexStride = mesh->getStride(submesh, NX_ARRAY_VERTICES);
				const NxU32 triangleStride = mesh->getStride(submesh, NX_ARRAY_TRIANGLES);
				const NxU8* const vertexData = static_cast<const NxU8*>(mesh->getBase(submesh, NX_ARRAY_VERTICES));
				const NxU8* const triangleData = static_cast<const NxU8*>(mesh->getBase(submesh, NX_ARRAY_TRIANGLES));
				if(mesh->getFormat(submesh, NX_ARRAY_VERTICES) != NX_FORMAT_FLOAT
					|| !vertexData || !triangleData || vertexStride < sizeof(NxVec3)
					|| triangleStride < 3 * sizeof(NxU16))
					continue;
				const NxInternalFormat indexFormat = mesh->getFormat(submesh, NX_ARRAY_TRIANGLES);
				for(NxU32 triangleIndex = triangleCount; triangleIndex > 0; --triangleIndex)
					{
					const NxU8* const indices = triangleData + (triangleIndex - 1) * triangleStride;
					NxU32 vertexIndices[3];
					if(indexFormat == NX_FORMAT_INT && triangleStride >= 3 * sizeof(NxU32))
						memcpy(vertexIndices, indices, sizeof(vertexIndices));
					else if(indexFormat == NX_FORMAT_SHORT)
						{
						NxU16 shortIndices[3];
						memcpy(shortIndices, indices, sizeof(shortIndices));
						vertexIndices[0] = shortIndices[0];
						vertexIndices[1] = shortIndices[1];
						vertexIndices[2] = shortIndices[2];
						}
					else
						continue;
					if(vertexIndices[0] >= vertexCount || vertexIndices[1] >= vertexCount
						|| vertexIndices[2] >= vertexCount)
						continue;
					NxVec3 localVertices[3];
					for(unsigned vertex = 0; vertex < 3; ++vertex)
						memcpy(&localVertices[vertex], vertexData + vertexIndices[vertex] * vertexStride,
							sizeof(NxVec3));
					const NxTriangle triangle(pose * localVertices[0], pose * localVertices[1], pose * localVertices[2]);
					if(nxSceneTriangleBoundsOverlap(triangle, worldBounds))
						worldTriangles.pushBack(triangle);
				}
				}
			}
		}
	return worldTriangles.size();
	}

// phys_fn_000670 (0x000145f0). Collects the prunables whose cached world boxes
// intersect the query, then reports public shapes through either the caller's
// buffer or the NxUserEntityReport batching contract.
NxU32 NxSceneInternal::overlapAABBShapes(const NxBounds3& worldBounds, NxShapesType shapesType,
	NxU32 maxShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	NxShape* callbackBuffer[64];
	NxShape** const buffer = shapes ? shapes : callbackBuffer;
	const NxU32 capacity = shapes ? maxShapes : (callback ? 64u : 0u);
	if(!capacity)
		return 0;
	const NxU32 mask = ((shapesType & NX_STATIC_SHAPES) ? 1u : 0u) |
		((shapesType & NX_DYNAMIC_SHAPES) ? 0xeu : 0u);
	Pruner** const pruners = reinterpret_cast<Pruner**>(bytes() + 0x624 + 0x1c);
	NxU32 count = 0;
	NxU32 buffered = 0;
	for(NxU32 type = 0; type < 4; type++)
		{
		if(!(mask & (1u << type)) || !pruners[type])
			continue;
		if(type == 0)
			{
			// The static pruner's slot 8 traverses its AABB tree and reports
			// the touched shapes in tree order (phys_fn_005229).
			IceCore::Container treeObjects;
			const NxVec3& nxMin = worldBounds.getMin();
			const NxVec3& nxMax = worldBounds.getMax();
			const Point min(nxMin.x, nxMin.y, nxMin.z);
			const Point max(nxMax.x, nxMax.y, nxMax.z);
			static_cast<StaticPruner*>(pruners[type])->OverlapAABB(treeObjects, min, max, 0xffffffffu);
			const NxU32* const entries = treeObjects.GetEntries();
			for(NxU32 i = 0; i < treeObjects.GetNbEntries(); i++)
				{
				Prunable* const prunable = reinterpret_cast<Prunable*>(entries[i]);
				unsigned char* const shapeBase = static_cast<unsigned char*>(prunable->mOwner);
				if(!shapeBase)
					continue;
				NxShape* const publicShape = *reinterpret_cast<NxShape**>(shapeBase + 0x9c);
				if(!publicShape)
					continue;
				buffer[buffered++] = publicShape;
				count++;
				if(buffered == capacity)
					{
					if(callback && !callback->onEvent(buffered, buffer))
						return count;
					buffered = 0;
					if(!callback)
						return count;
					}
				}
			continue;
			}
		PruningPool& pool = pruners[type]->mPool;
		for(NxU32 i = 0; i < pool.mNbTotal; i++)
			{
			Prunable* const prunable = pool.mObjects[i];
			if(!prunable || prunable->mHandle == PRUNABLE_INVALID_HANDLE)
				continue;
			unsigned char* const shapeBase = static_cast<unsigned char*>(prunable->mOwner);
			if(!shapeBase)
				continue;
			if(*reinterpret_cast<NxU32*>(shapeBase + 0xd0) != NX_SHAPE_PLANE)
				{
				if(!(prunable->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
					prunable->UpdateWorldAABB(&pool.mWorldBoxes[i]);
				const NxBounds3& shapeBounds = *reinterpret_cast<NxBounds3*>(&pool.mWorldBoxes[i]);
				if(!worldBounds.intersects(shapeBounds))
					continue;
				}
			NxShape* const publicShape = *reinterpret_cast<NxShape**>(shapeBase + 0x9c);
			if(!publicShape)
				continue;
			buffer[buffered++] = publicShape;
			count++;
			if(buffered == capacity)
				{
				if(callback && !callback->onEvent(buffered, buffer))
					return count;
				buffered = 0;
				if(!callback)
					return count;
				}
			}
		}
	if(callback && buffered && !callback->onEvent(buffered, buffer))
		return count;
	return count;
	}

// phys_fn_000671 (0x000146e0). Report AABBs which remain inside every plane's
// non-positive half-space, preserving selected-pruner order and batching.
NxU32 NxSceneInternal::cullShapes(NxU32 nbPlanes, const NxPlane* worldPlanes, NxShapesType shapesType,
	NxU32 maxShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	NxShape* callbackBuffer[64];
	NxShape** const buffer = shapes ? shapes : callbackBuffer;
	const NxU32 capacity = shapes ? maxShapes : (callback ? 64u : 0u);
	if(!capacity)
		return 0;
	const NxU32 mask = ((shapesType & NX_STATIC_SHAPES) ? 1u : 0u) |
		((shapesType & NX_DYNAMIC_SHAPES) ? 0xeu : 0u);
	Pruner** const pruners = reinterpret_cast<Pruner**>(bytes() + 0x624 + 0x1c);
	NxU32 count = 0;
	NxU32 buffered = 0;
	for(NxU32 type = 0; type < 4; type++)
		{
		if(!(mask & (1u << type)) || !pruners[type])
			continue;
		PruningPool& pool = pruners[type]->mPool;
		for(NxU32 i = 0; i < pool.mNbTotal; i++)
			{
			Prunable* const prunable = pool.mObjects[i];
			if(!prunable || prunable->mHandle == PRUNABLE_INVALID_HANDLE)
				continue;
			unsigned char* const shapeBase = static_cast<unsigned char*>(prunable->mOwner);
			if(!shapeBase)
				continue;
			bool culled = false;
			if(*reinterpret_cast<NxU32*>(shapeBase + 0xd0) != NX_SHAPE_PLANE)
				{
				if(!(prunable->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
					prunable->UpdateWorldAABB(&pool.mWorldBoxes[i]);
				const NxBounds3& bounds = *reinterpret_cast<NxBounds3*>(&pool.mWorldBoxes[i]);
				for(NxU32 p = 0; p < nbPlanes; p++)
					{
					const NxPlane& plane = worldPlanes[p];
					const NxVec3& normal = plane.normal;
					const NxVec3& mins = bounds.getMin();
					const NxVec3& maxs = bounds.getMax();
					const NxVec3 minimumSupport(
						normal.x < 0.0f ? maxs.x : mins.x,
						normal.y < 0.0f ? maxs.y : mins.y,
						normal.z < 0.0f ? maxs.z : mins.z);
					if(normal.dot(minimumSupport) + plane.d > 0.0f)
						{
						culled = true;
						break;
						}
					}
			}
			if(culled)
				continue;
			NxShape* const publicShape = *reinterpret_cast<NxShape**>(shapeBase + 0x9c);
			if(!publicShape)
				continue;
			buffer[buffered++] = publicShape;
			count++;
			if(buffered == capacity)
				{
				if(callback && !callback->onEvent(buffered, buffer))
					return count;
				buffered = 0;
				if(!callback)
					return count;
				}
			}
		}
	if(callback && buffered && !callback->onEvent(buffered, buffer))
		return count;
	return count;
	}

static bool nxSceneSphereOverlapsShape(const NxSphere& sphere, NxShape* shape)
	{
	if(!shape)
		return false;
	switch(shape->getType())
		{
		case NX_SHAPE_PLANE:
			// Plane prunables represent unbounded geometry in the scene-query
			// broadphase; once selected they are always a candidate.
			return true;
		case NX_SHAPE_SPHERE:
			{
			NxSphereShape* sphereShape = shape->isSphere();
			const NxVec3 delta = sphere.center - sphereShape->getGlobalPosition();
			const NxReal radius = sphere.radius + sphereShape->getRadius();
			return delta.magnitudeSquared() <= radius * radius;
			}
		case NX_SHAPE_BOX:
			{
			NxBoxShape* box = shape->isBox();
			const NxMat34 pose = box->getGlobalPose();
			NxVec3 localCenter;
			pose.multiplyByInverseRT(sphere.center, localCenter);
			const NxVec3 dimensions = box->getDimensions();
			const NxVec3 closest(
				localCenter.x < -dimensions.x ? -dimensions.x : (localCenter.x > dimensions.x ? dimensions.x : localCenter.x),
				localCenter.y < -dimensions.y ? -dimensions.y : (localCenter.y > dimensions.y ? dimensions.y : localCenter.y),
				localCenter.z < -dimensions.z ? -dimensions.z : (localCenter.z > dimensions.z ? dimensions.z : localCenter.z));
			return (localCenter - closest).magnitudeSquared() <= sphere.radius * sphere.radius;
			}
		case NX_SHAPE_CAPSULE:
			{
			NxCapsuleShape* capsule = shape->isCapsule();
			const NxMat34 pose = capsule->getGlobalPose();
			NxVec3 halfAxis;
			pose.M.multiply(NxVec3(0.0f, capsule->getHeight() * 0.5f, 0.0f), halfAxis);
			const NxVec3 center = capsule->getGlobalPosition();
			const NxVec3 a = center - halfAxis;
			const NxVec3 ab = halfAxis * 2.0f;
			const NxVec3 ap = sphere.center - a;
			const NxReal ab2 = ab.magnitudeSquared();
			NxReal t = ab2 > 0.0f ? ap.dot(ab) / ab2 : 0.0f;
			if(t < 0.0f) t = 0.0f;
			else if(t > 1.0f) t = 1.0f;
			const NxVec3 delta = ap - ab * t;
			const NxReal radius = sphere.radius + capsule->getRadius();
			return delta.magnitudeSquared() <= radius * radius;
			}
	case NX_SHAPE_MESH:
		{
			NxBounds3 bounds;
			shape->getWorldBounds(bounds);
			NxVec3 boundsCenter;
			NxVec3 dimensions;
			bounds.getCenter(boundsCenter);
			bounds.getDimensions(dimensions);
			const NxVec3 extents = dimensions * 0.5f;
			const NxVec3& center = sphere.center;
			const NxVec3 closest(
				center.x < boundsCenter.x - extents.x ? boundsCenter.x - extents.x : (center.x > boundsCenter.x + extents.x ? boundsCenter.x + extents.x : center.x),
				center.y < boundsCenter.y - extents.y ? boundsCenter.y - extents.y : (center.y > boundsCenter.y + extents.y ? boundsCenter.y + extents.y : center.y),
				center.z < boundsCenter.z - extents.z ? boundsCenter.z - extents.z : (center.z > boundsCenter.z + extents.z ? boundsCenter.z + extents.z : center.z));
			return (center - closest).magnitudeSquared() <= sphere.radius * sphere.radius;
			}
		default:
			return false;
		}
	}

// phys_fn_000678 (0x00014990). Collects broadphase candidates for a sphere,
// applies the shape overlap kernels, then reports public shapes in pruner order.
NxU32 NxSceneInternal::overlapSphereShapes(const NxSphere& worldSphere, NxShapesType shapesType,
	NxU32 maxShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	NxShape* callbackBuffer[64];
	NxShape** const buffer = shapes ? shapes : callbackBuffer;
	const NxU32 capacity = shapes ? maxShapes : (callback ? 64u : 0u);
	if(!capacity)
		return 0;
	NxBounds3 queryBounds;
	queryBounds.set(worldSphere.center.x - worldSphere.radius, worldSphere.center.y - worldSphere.radius,
		worldSphere.center.z - worldSphere.radius, worldSphere.center.x + worldSphere.radius,
		worldSphere.center.y + worldSphere.radius, worldSphere.center.z + worldSphere.radius);
	const NxU32 mask = ((shapesType & NX_STATIC_SHAPES) ? 1u : 0u) |
		((shapesType & NX_DYNAMIC_SHAPES) ? 0xeu : 0u);
	Pruner** const pruners = reinterpret_cast<Pruner**>(bytes() + 0x624 + 0x1c);
	NxU32 count = 0;
	NxU32 buffered = 0;
	for(NxU32 type = 0; type < 4; type++)
		{
		if(!(mask & (1u << type)) || !pruners[type])
			continue;
		PruningPool& pool = pruners[type]->mPool;
		for(NxU32 i = 0; i < pool.mNbTotal; i++)
			{
			Prunable* const prunable = pool.mObjects[i];
			if(!prunable || prunable->mHandle == PRUNABLE_INVALID_HANDLE)
				continue;
			unsigned char* const shapeBase = static_cast<unsigned char*>(prunable->mOwner);
			if(!shapeBase)
				continue;
			const bool isPlane = *reinterpret_cast<NxU32*>(shapeBase + 0xd0) == NX_SHAPE_PLANE;
			if(!isPlane)
				{
				if(!(prunable->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
					prunable->UpdateWorldAABB(&pool.mWorldBoxes[i]);
				const NxBounds3& shapeBounds = *reinterpret_cast<NxBounds3*>(&pool.mWorldBoxes[i]);
				if(!queryBounds.intersects(shapeBounds))
					continue;
				}
			NxShape* const publicShape = *reinterpret_cast<NxShape**>(shapeBase + 0x9c);
			if(!publicShape || !nxSceneSphereOverlapsShape(worldSphere, publicShape))
				continue;
			buffer[buffered++] = publicShape;
			count++;
			if(buffered == capacity)
				{
				if(callback && !callback->onEvent(buffered, buffer))
					return count;
				buffered = 0;
				if(!callback)
					return count;
				}
			}
		}
	if(callback && buffered && !callback->onEvent(buffered, buffer))
		return count;
	return count;
	}

// phys_fn_000672 (0x000147d0). The oracle gathers AABB candidates through the
// pruning engine, then runs a shape-specific overlap test. This implementation
// follows the same selected pool/type path and applies the recovered primitive
// sphere tests to the candidate shapes.
bool NxSceneInternal::checkOverlapSphere(const NxSphere& worldSphere, NxShapesType shapesType)
	{
	NxBounds3 queryBounds;
	queryBounds.set(worldSphere.center.x - worldSphere.radius, worldSphere.center.y - worldSphere.radius,
		worldSphere.center.z - worldSphere.radius, worldSphere.center.x + worldSphere.radius,
		worldSphere.center.y + worldSphere.radius, worldSphere.center.z + worldSphere.radius);
	const NxU32 mask = ((shapesType & NX_STATIC_SHAPES) ? 1u : 0u) |
		((shapesType & NX_DYNAMIC_SHAPES) ? 0xeu : 0u);
	Pruner** const pruners = reinterpret_cast<Pruner**>(bytes() + 0x624 + 0x1c);
	for(NxU32 type = 0; type < 4; type++)
		{
		if(!(mask & (1u << type)) || !pruners[type])
			continue;
		PruningPool& pool = pruners[type]->mPool;
		for(NxU32 i = 0; i < pool.mNbTotal; i++)
			{
			Prunable* const prunable = pool.mObjects[i];
			const unsigned char* const shapeBase = static_cast<const unsigned char*>(prunable->mOwner);
			NxShape* const shape = shapeBase ? *reinterpret_cast<NxShape* const*>(shapeBase + 0x9c) : 0;
			if(shape && shape->getType() == NX_SHAPE_PLANE)
				return true;
			if(!(prunable->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
				prunable->UpdateWorldAABB(&pool.mWorldBoxes[i]);
			const NxBounds3& shapeBounds = *reinterpret_cast<const NxBounds3*>(&pool.mWorldBoxes[i]);
			if(queryBounds.intersects(shapeBounds) && nxSceneSphereOverlapsShape(worldSphere, shape))
				return true;
			}
		}
	return false;
	}
