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
#include "NxUserRaycastReport.h"
#include "NxIntersectionSegmentBox.h"
#include "Containers.h"
#include "FoundationSDK.h"
#include "X87Sqrt.h"
#include "ObjectModel.h"

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
