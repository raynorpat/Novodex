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
#include "NxUserAllocator.h"
#include "PhysicsSDK.h"
#include "NxBoxDistance.h"
#include "IcePrunable.h"

#include <math.h>
#include <stddef.h>

static void nxAppendTriggerPair(void* context, const NxCollisionShape* shape0,
	const NxCollisionShape* shape1);

static void* gNxCollisionDispatchMatrix = 0;

void NxSetCollisionDispatchMatrix(void* matrix)
	{
	gNxCollisionDispatchMatrix = matrix;
	}

void* NxGetCollisionDispatchMatrix()
	{
	return gNxCollisionDispatchMatrix ? gNxCollisionDispatchMatrix : nxPhysicsSDKShapePairTable();
	}

// phys_fn_002348 (0x0005ab80, 719 B)
// The matrix object has its contact slots at +0x04 and overlap slots at +0x94.
// The oracle orders by the shape tags before using 6 * low + high; either of
// the low three bits in either flag byte selects overlap testing.
void __cdecl NxDispatchShapePair(void* matrix,
	const NxCollisionShape* shape0, const NxCollisionShape* shape1,
	void* contactSink, void* context)
	{
	if(shape0->type > shape1->type)
		{
		const NxCollisionShape* swap = shape0;
		shape0 = shape1;
		shape1 = swap;
		}
	const NxU32 index = NxCollisionPairIndex(shape0->type, shape1->type);
	const NxU8 flags0 = *((const NxU8*) shape0 + 0xde);
	const NxU8 flags1 = *((const NxU8*) shape1 + 0xde);
	if((flags0 & 7) || (flags1 & 7))
		{
		NxShapeOverlapFn* table = (NxShapeOverlapFn*) ((NxU8*) matrix + 0x94);
		NxShapeOverlapFn overlap = table[index];
		typedef bool (__cdecl* NxShapeOverlapContextFn)(const NxCollisionShape*,
			const NxCollisionShape*, void*);
		if(!overlap || !((NxShapeOverlapContextFn) overlap)(shape0, shape1, context))
			return;

		// The dispatcher expands the untriggered compound against the triggered
		// shape. A plain overlap pair is appended as-is; compound children are
		// independently looked up and successful pairs are appended in order.
		const NxCollisionShape* compound = (flags1 & 7) ? shape0 : shape1;
		const NxCollisionShape* other = (flags1 & 7) ? shape1 : shape0;
		if(compound->type != 5)
			{
			nxAppendTriggerPair(context, other, compound);
			return;
			}
		const NxCollisionShape* const* child =
			*(const NxCollisionShape* const* const*) ((const NxU8*) compound + 0xe0);
		const NxCollisionShape* const* childEnd =
			*(const NxCollisionShape* const* const*) ((const NxU8*) compound + 0xe4);
		for(; child != childEnd; ++child)
			{
			const NxCollisionShape* low = other;
			const NxCollisionShape* high = *child;
			if(low->type > high->type)
				{
				const NxCollisionShape* swap = low;
				low = high;
				high = swap;
				}
			NxShapeOverlapFn childOverlap = table[NxCollisionPairIndex(low->type, high->type)];
			if(childOverlap && ((NxShapeOverlapContextFn) childOverlap)(low, high, context))
				nxAppendTriggerPair(context, low, high);
			}
		return;
		}

	typedef void (__cdecl* NxShapeContactFn)(const NxCollisionShape*,
		const NxCollisionShape*, void*, void*);
	NxShapeContactFn* table = (NxShapeContactFn*) ((NxU8*) matrix + 0x04);
	NxShapeContactFn contact = table[index];
	if(contact)
		contact(shape0, shape1, contactSink, context);
	}

// The four offsets every kernel below reads out of a shape. They are the
// oracle's, so they are asserted rather than commented.
static_assert(offsetof(NxCollisionShape, owner) == 0x04, "shape owner is at 0x04");
static_assert(offsetof(NxCollisionShape, rotation) == 0x0c, "shape rotation is at 0x0c");
static_assert(offsetof(NxCollisionShape, collisionObject) == 0x9c, "shape collision object is at 0x9c");
static_assert(offsetof(NxCollisionShape, translation) == 0x30, "shape translation is at 0x30");
static_assert(offsetof(NxCollisionShape, type) == 0xd0, "shape type is at 0xd0");
static_assert(offsetof(NxCollisionShape, geometry) == 0xe0, "shape geometry union is at 0xe0");

// phys_fn_004153 (0x0009a570, 156 B)
// The scene's pair-key hash uses an ascending 16-bit key pair, a 32-bit avalanche hash, bucket mask at +0x04,
// bucket heads at +0x08, links at +0x0c and eight-byte records at +0x14.
// phys_fn_004153: sorted-key lookup in the sparse pair map.
void* NxFindCollisionPairRecord(const void* pairMap, NxU16 owner0, NxU16 owner1)
	{
	const NxU8* map = (const NxU8*) pairMap;
	if(!*(const NxU32*) (map + 8))
		return 0;
	if(owner1 < owner0)
		{
		const NxU16 swap = owner0;
		owner0 = owner1;
		owner1 = swap;
		}
	NxU32 hash = ((NxU32) owner1 << 16) | owner0;
	hash += ~(hash << 15);
	hash = ((NxU32) ((NxI32) hash >> 10) ^ hash) * 9;
	hash ^= (NxU32) ((NxI32) hash >> 6);
	hash += ~(hash << 11);
	const NxU32 bucket = ((NxU32) ((NxI32) hash >> 16) ^ hash) & *(const NxU32*) (map + 4);
	NxI32 index = ((const NxI32*) *(void* const*) (map + 8))[bucket];
	if(index == -1)
		return 0;
	const NxU8* entries = (const NxU8*) *(void* const*) (map + 0x14);
	const NxI32* links = (const NxI32*) *(void* const*) (map + 0x0c);
	while(index != -1)
		{
		const NxU8* entry = entries + index * 8;
		if(*(const NxU16*) entry == owner0 && *(const NxU16*) (entry + 2) == owner1)
			return (void*) entry;
		index = links[index];
		}
	return 0;
	}

// phys_fn_004157 (0x0009a920, 476 B)
// Remove a sorted owner pair from the scene map, preserving its bucket chains
// while compacting the final live record into the released slot.
bool NxRemoveCollisionPairRecord(void* pairMap, NxU16 owner0, NxU16 owner1)
	{
	NxU8* map = (NxU8*) pairMap;
	NxI32* buckets = *(NxI32**) (map + 8);
	if(!buckets)
		return false;
	if(owner1 < owner0)
		{
		const NxU16 swap = owner0;
		owner0 = owner1;
		owner1 = swap;
		}
	NxU32 hash = ((NxU32) owner1 << 16) | owner0;
	hash += ~(hash << 15);
	hash = (((NxU32) ((NxI32) hash >> 10) ^ hash) * 9);
	hash ^= (NxU32) ((NxI32) hash >> 6);
	hash += ~(hash << 11);
	const NxU32 bucket = (((NxU32) ((NxI32) hash >> 16) ^ hash) & *(NxU32*) (map + 4));
	NxI32* links = *(NxI32**) (map + 0x0c);
	NxU32* count = (NxU32*) (map + 0x10);
	NxU8* entries = (NxU8*) *(void**) (map + 0x14);
	NxI32 index = buckets[bucket];
	if(index == -1)
		return false;
	const NxI32 bucketHead = index;
	while(index != -1)
		{
		NxU8* entry = entries + index * 8;
		if(*(NxU16*) entry == owner0 && *(NxU16*) (entry + 2) == owner1)
			break;
		index = links[index];
		}
	if(index == -1)
		return false;
	const NxU32 freeHeadOffset = 0x18;
	const NxU32 oldFreeHead = *(NxU32*) (map + freeHeadOffset);
	const NxU32 newCount = *count - 1;
	if(index == bucketHead)
		buckets[bucket] = links[index];
	else
		{
		NxI32 previous = bucketHead;
		while(links[previous] != index)
			previous = links[previous];
		links[previous] = links[index];
		}
	*(NxU32*) (entries + index * 8 + 4) = oldFreeHead;
	*(NxU16*) (entries + index * 8) = 0xffff;
	*(NxU16*) (entries + index * 8 + 2) = 0xffff;
	*(NxU32*) (map + freeHeadOffset) = (NxU32) index;
	if(index == newCount)
		{
		*(NxU32*) (map + freeHeadOffset) = oldFreeHead;
		*count = newCount;
		return true;
		}
	const NxU32 last = newCount;
	NxU8* lastEntry = entries + last * 8;
	NxU16 moved0 = *(NxU16*) lastEntry;
	NxU16 moved1 = *(NxU16*) (lastEntry + 2);
	if(moved1 < moved0)
		{
		const NxU16 swap = moved0;
		moved0 = moved1;
		moved1 = swap;
		}
	NxU32 movedHash = ((NxU32) moved1 << 16) | moved0;
	movedHash += ~(movedHash << 15);
	movedHash = (((NxU32) ((NxI32) movedHash >> 10) ^ movedHash) * 9);
	movedHash ^= (NxU32) ((NxI32) movedHash >> 6);
	movedHash += ~(movedHash << 11);
	const NxU32 movedBucket = (((NxU32) ((NxI32) movedHash >> 16) ^ movedHash) & *(NxU32*) (map + 4));
	NxI32* predecessor = &buckets[movedBucket];
	while(*predecessor != (NxI32) last)
		predecessor = &links[*predecessor];
	*predecessor = links[last];
	memcpy(entries + index * 8, lastEntry, 8);
	links[index] = buckets[movedBucket];
	buckets[movedBucket] = index;
	*(NxU32*) (map + freeHeadOffset) = oldFreeHead;
	*count = newCount;
	return true;
	}

// phys_fn_000529 (0x00010570, 134 B)
// Filter flags, symmetric collision-group masks and the scene's owner-pair record in that order.
bool NxFilterShapePair(const NxU32* groupMasks, const void* pairMap,
	const NxCollisionShape* shape0, const NxCollisionShape* shape1)
	{
	if((*((const NxU8*) shape0 + 0xde) & 0x10) || (*((const NxU8*) shape1 + 0xde) & 0x10))
		return false;
	const NxU16 group0 = *(const NxU16*) ((const NxU8*) shape0 + 0xd8);
	const NxU16 group1 = *(const NxU16*) ((const NxU8*) shape1 + 0xd8);
	if(group0 != 0xffff && group1 != 0xffff &&
		!(groupMasks[group0] & (1u << (group1 & 0x1f))))
		return false;
	void* record = NxFindCollisionPairRecord(pairMap,
		*(const NxU16*) ((const NxU8*) shape0 + 0xd4),
		*(const NxU16*) ((const NxU8*) shape1 + 0xd4));
	return !record || ((~*(const NxU32*) ((const NxU8*) record + 4) & 1) != 0);
	}

static void nxAppendTriggerPair(void* context, const NxCollisionShape* shape0,
	const NxCollisionShape* shape1);

struct NxTriggerPairArray
	{
	NxCollisionShape** begin;
	NxCollisionShape** end;
	NxCollisionShape** capacity;
	};

static void nxAppendTriggerPair(void* context, const NxCollisionShape* shape0,
	const NxCollisionShape* shape1)
	{
	// Scene+0x5d8 points to the second embedded pair-list header at +0x5ec.
	// phys_fn_002350 dereferences this pointer before reading begin/end/capacity.
	NxTriggerPairArray* array = *(NxTriggerPairArray**) ((NxU8*) context + 0x5d8);
	if(array->capacity <= array->end)
		{
		const NxU32 count = array->begin
			? (NxU32) ((NxU8*) array->end - (NxU8*) array->begin) / 8 : 0;
		const NxU32 held = array->begin
			? (NxU32) ((NxU8*) array->capacity - (NxU8*) array->begin) / 8 : 0;
		const NxU32 wanted = count * 2 + 2;
		if(held < wanted)
			{
			NxCollisionShape** block = (NxCollisionShape**) nxFoundationSDKAllocator->malloc(
				(size_t) wanted * 8, NX_MEMORY_PERSISTENT);
			for(NxU32 i = 0; i < count * 2; ++i)
				block[i] = array->begin[i];
			if(array->begin)
				nxFoundationSDKAllocator->free(array->begin);
			array->begin = block;
			array->end = block + count * 2;
			array->capacity = block + wanted * 2;
			}
		}
	*array->end++ = (NxCollisionShape*) shape0;
	*array->end++ = (NxCollisionShape*) shape1;
	}

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

#include "NxSegmentSegmentDistance.inl"

// ---------------------------------------------------------------------------
// convex-mesh gap Task 2a: the matrix B entries that reach the box distance
// kernels of Distance.cpp (units/convex-mesh-gap-contract.md, sub-units G, J
// and K), and matrix B [CAPSULE][CAPSULE].

// The capsule's axis segment, the construction phys_fn_001921 above writes out
// in place. 0x0003b0e7..0x0003b175 (001751), 0x0003d898..0x0003d932 and
// 0x0003d936..0x0003d996 (001774, once per capsule) and 0x0003f39d..0x0003f427
// (001785) are the same instructions over different frame slots: the x
// component of the half axis is narrowed and read back, y and z stay in
// registers, and -z is narrowed before it is added.
static __forceinline void nxCapsuleSegment(const NxCollisionShape* capsule, NxSegment* segment)
	{
	const NxReal* m = capsule->rotation;
	const NxReal* t = capsule->translation;

	const NxReal halfHeight = capsule->geometry[1];
	const NxReal axisX = (NxReal) ((double) m[1] * halfHeight);
	const double axisY = (double) m[4] * halfHeight;
	const double axisZ = (double) m[7] * halfHeight;
	const NxReal negatedAxisZ = (NxReal) (-axisZ);

	segment->p0.x = (NxReal) (-(double) axisX + t[0]);
	segment->p0.y = (NxReal) (-axisY + t[1]);
	segment->p0.z = (NxReal) ((double) negatedAxisZ + t[2]);
	segment->p1.x = (NxReal) ((double) axisX + t[0]);
	segment->p1.y = (NxReal) (axisY + t[1]);
	segment->p1.z = (NxReal) (axisZ + t[2]);
	}

// The world box of a shape's pruning handle: the Prunable at Shape+0xa4
// (Physics/src/opcode/IcePrunable.h), i.e. Prunable::GetUpdatedWorldAABB
// inlined with its UpdateWorldAABB left as a call -- 0x0003f413..0x0003f45f in
// 001785, 0x0003f5b8..0x0003f5fd in 001789, 0x0003f718..0x0003f765 in 001791,
// each `call 0x100b55b0` (phys_fn_004886). The pruner is loaded before the
// handle is tested. An invalid handle gives a null box, which all three
// entries then dereference, as the oracle does.
const NxReal* NxShapeWorldBounds(const NxCollisionShape* shape)
	{
	Prunable* prunable = (Prunable*) ((NxU8*) shape + 0xa4);
	Pruner* pruner = prunable->mPruner;
	if(prunable->mHandle == PRUNABLE_INVALID_HANDLE)
		return 0;
	if(!(prunable->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
		prunable->UpdateWorldAABB(&pruner->mPool.mWorldBoxes[prunable->mHandle]);
	return (const NxReal*) &pruner->mPool.mWorldBoxes[prunable->mHandle];
	}

// The box the three compound entries build from those bounds (min at +0,
// max at +0xc): centre and half size, each of x and y narrowed before the
// halving and z halved in the register it was formed in, with an identity
// rotation written as nine immediates. 001785 and 001789 form each centre sum
// as max + min, 001791 as min + max; the sum is the same.
static __forceinline void nxBoundsBox(const NxReal* bounds, NxCollisionBoxData* box)
	{
	const NxReal sumX = (NxReal) ((double) bounds[3] + bounds[0]);
	const NxReal sumY = (NxReal) ((double) bounds[4] + bounds[1]);
	const double sumZ = (double) bounds[5] + bounds[2];
	box->center[0] = (NxReal) ((double) sumX * 0.5f);
	box->center[1] = (NxReal) ((double) sumY * 0.5f);
	box->center[2] = (NxReal) (sumZ * 0.5f);

	const NxReal sizeX = (NxReal) ((double) bounds[3] - bounds[0]);
	const NxReal sizeY = (NxReal) ((double) bounds[4] - bounds[1]);
	const double sizeZ = (double) bounds[5] - bounds[2];
	box->extents[0] = (NxReal) ((double) sizeX * 0.5f);
	box->extents[1] = (NxReal) ((double) sizeY * 0.5f);
	box->extents[2] = (NxReal) (sizeZ * 0.5f);

	for(int i = 0; i < 9; ++i)
		box->rotation[i] = (i % 4 == 0) ? 1.0f : 0.0f;
	}

// phys_fn_001751 (0x0003b0e0, 370 B)
// Matrix B [BOX][CAPSULE]. A sphere of the capsule's radius at each end of its
// axis against the box through phys_fn_001913, and only when neither overlaps
// the axis segment against the box through phys_fn_001688 (null parameter
// and point), against the squared radius. The radius is narrowed into two
// slots (0x0003b0f3 and 0x0003b180); the squared distance is compared as the
// register the kernel returned (0x0003b23d `fcompp`), strictly.
bool __cdecl NxOverlapBoxCapsule(const NxCollisionShape* box, const NxCollisionShape* capsule)
	{
	const NxReal radius = capsule->geometry[0];

	NxSegment segment;
	nxCapsuleSegment(capsule, &segment);

	NxCollisionBoxData boxData;
	boxData.center[0] = box->translation[0];
	boxData.center[1] = box->translation[1];
	boxData.center[2] = box->translation[2];
	boxData.extents[0] = box->geometry[1];
	boxData.extents[1] = box->geometry[2];
	boxData.extents[2] = box->geometry[3];
	for(int i = 0; i < 9; ++i)
		boxData.rotation[i] = box->rotation[i];

	NxCollisionSphereData sphere;
	sphere.center[0] = segment.p0.x;
	sphere.center[1] = segment.p0.y;
	sphere.center[2] = segment.p0.z;
	sphere.radius = radius;
	if(NxOverlapSphereBoxData(&sphere, &boxData))
		return true;

	sphere.center[0] = segment.p1.x;
	sphere.center[1] = segment.p1.y;
	sphere.center[2] = segment.p1.z;
	sphere.radius = radius;
	if(NxOverlapSphereBoxData(&sphere, &boxData))
		return true;

	const double squared = NxSegmentBoxSquareDistance(&segment, boxData.center,
		boxData.extents, boxData.rotation, 0, 0);
	return (double) radius * radius > squared;
	}

// phys_fn_001774 (0x0003d890, 320 B)
// Matrix B [CAPSULE][CAPSULE]. The two axis segments through phys_fn_001690,
// whose parameters go to the caller's two argument slots, against the sum of
// the radii squared -- and unlike [SPHERE][CAPSULE] the sum is NOT narrowed:
// `fld [esi+0xe0]; fadd [edi+0xe0]; fld st(0); fmul st(1)` at 0x0003d9a0 keeps
// it on the stack. Strict, and an unordered compare is false (0x0003d9bf
// `test ah,5; jp`).
bool __cdecl NxOverlapCapsuleCapsule(const NxCollisionShape* capsule0, const NxCollisionShape* capsule1)
	{
	NxSegment segment0;
	NxSegment segment1;
	nxCapsuleSegment(capsule0, &segment0);
	nxCapsuleSegment(capsule1, &segment1);

	NxReal parameter0;
	NxReal parameter1;
	const double squared = NxSegmentSegmentSquareDistance(&segment0, &segment1,
		&parameter0, &parameter1);
	const double radiusSum = (double) capsule0->geometry[0] + capsule1->geometry[0];
	return squared < radiusSum * radiusSum;
	}

// phys_fn_001785 (0x0003f390, 471 B)
// Matrix B [CAPSULE][COMPOUND]. Not a walk over children: the capsule's axis
// against ONE box, the compound shape's own world bounds, through segment/box
// (phys_fn_001688) with null outputs, against the squared radius (copied as a
// word at 0x0003f3a6 and squared from that copy at 0x0003f545).
bool __cdecl NxOverlapCapsuleCompound(const NxCollisionShape* capsule, const NxCollisionShape* compound)
	{
	const NxReal radius = capsule->geometry[0];

	NxSegment segment;
	nxCapsuleSegment(capsule, &segment);

	NxCollisionBoxData boxData;
	nxBoundsBox(NxShapeWorldBounds(compound), &boxData);

	const double squared = NxSegmentBoxSquareDistance(&segment, boxData.center,
		boxData.extents, boxData.rotation, 0, 0);
	return (double) radius * radius > squared;
	}

// phys_fn_001789 (0x0003f5b0, 334 B)
// Matrix B [SPHERE][COMPOUND]. The sphere against the compound's world bounds
// through phys_fn_001913.
bool __cdecl NxOverlapSphereCompound(const NxCollisionShape* sphere, const NxCollisionShape* compound)
	{
	NxCollisionBoxData boxData;
	nxBoundsBox(NxShapeWorldBounds(compound), &boxData);

	NxCollisionSphereData sphereData;
	sphereData.center[0] = sphere->translation[0];
	sphereData.center[1] = sphere->translation[1];
	sphereData.center[2] = sphere->translation[2];
	sphereData.radius = sphere->geometry[0];

	return NxOverlapSphereBoxData(&sphereData, &boxData);
	}

// phys_fn_001791 (0x0003f700, 418 B)
// Matrix B [BOX][COMPOUND]. False at once unless the box carries one of the
// three low bits of its flag word at Shape+0xde (0x0003f70b `test byte ptr
// [ebx+0xde], 7`), and otherwise the compound's world bounds against the box
// through NxBoxBoxIntersect (phys_fn_001702, 0x00036690) with the BOUNDS as
// the first box, the identity as its rotation, and `fullTest` set.
bool __cdecl NxOverlapBoxCompound(const NxCollisionShape* box, const NxCollisionShape* compound)
	{
	if(!(*((const NxU8*) box + 0xde) & 7))
		return false;

	NxCollisionBoxData boundsData;
	nxBoundsBox(NxShapeWorldBounds(compound), &boundsData);

	NxVec3 extents0(boundsData.extents[0], boundsData.extents[1], boundsData.extents[2]);
	NxVec3 center0(boundsData.center[0], boundsData.center[1], boundsData.center[2]);
	NxMat33 rotation0;
	rotation0.setRowMajor(boundsData.rotation);
	NxVec3 extents1(box->geometry[1], box->geometry[2], box->geometry[3]);
	NxVec3 center1(box->translation[0], box->translation[1], box->translation[2]);
	NxMat33 rotation1;
	rotation1.setRowMajor(box->rotation);

	return NxBoxBoxIntersect(extents0, center0, rotation0, extents1, center1, rotation1, true);
	}
