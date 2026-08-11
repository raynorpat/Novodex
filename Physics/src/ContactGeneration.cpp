/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// Contact generation. Same floating-point model as NarrowPhase.cpp and for the
// same reason: these entries are reached only from the simulation step, where
// the x87 control word is 64-bit precision with round-toward-zero. Built
// /arch:IA32; `double` means the oracle keeps the value in st(n) and `NxReal`
// means it stored it and read it back.

#include "ContactGeneration.h"

// phys_fn_002266 reads NX_CONTINUOUS_CD out of the SDK's live parameter array
// through phys_fn_000429, so this unit reaches Phase 2's PhysicsSDK.
#include "PhysicsSDK.h"

#include "NxIntersectionRayPlane.h"
#include "NxIntersectionRaySphere.h"
#include "NxIntersectionSegmentCapsule.h"
#include "NxPlane.h"
#include "NxSegment.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static_assert(offsetof(NxContactSink, orientedTo) == 0x08, "sink orientedTo is at 0x08");
static_assert(offsetof(NxContactSink, contactCount) == 0x10, "sink contactCount is at 0x10");
static_assert(offsetof(NxContactSink, lastObject1) == 0x20, "sink lastObject1 is at 0x20");
static_assert(offsetof(NxContactSink, lastNormal) == 0x28, "sink lastNormal is at 0x28");
static_assert(offsetof(NxContactSink, featurePairValid) == 0x34, "sink featurePairValid is at 0x34");
static_assert(offsetof(NxContactSink, stream) == 0x40, "sink stream data is at 0x40");

// The stream never reallocates here.
//
// The *policy* -- how much phys_fn_004840 at 0x000b4de0 adds and where it gets
// it -- is that Phase 2 row's business and is not reproduced. What belongs to
// this row is *when* it is called and with what count, and the oracle uses two
// different predicates:
//
//   count == capacity      before every single-word append
//   count + 3 > capacity   before each of the two three-word bursts
//
// eight sites in all: 0x0001d6d2, 0x0001d6ff, 0x0001d744, 0x0001d7c0,
// 0x0001d7f1, 0x0001d838, 0x0001d873 and 0x0001d8a0. Roughly 110 of this row's
// 706 bytes are those tests and the calls under them. An earlier version of
// this file had neither predicate and never read streamCapacity at all, so it
// would have run off the end of a real caller's buffer instead of growing.
//
// The differential pre-sizes the stream, so no reserve ever fails and the
// growth path is unexercised on both sides; a matching stream says nothing
// about it. What the guard buys is that this reconstruction stops rather than
// overruns.
static bool nxReserve(NxContactSink* sink, NxU32 count)
	{
	const bool full = (count == 1)
		? (sink->streamCount == sink->streamCapacity)
		: (sink->streamCount + count > sink->streamCapacity);
	// phys_fn_004840 would grow here. Until that row is reconstructed, refusing
	// the write is the only safe thing.
	return !full;
	}

static void nxAppend(NxContactSink* sink, NxU32 word)
	{
	if(!nxReserve(sink, 1))
		return;
	sink->stream[sink->streamCount++] = word;
	}

static void nxAppend3(NxContactSink* sink, NxU32 a, NxU32 b, NxU32 c)
	{
	if(!nxReserve(sink, 3))
		return;
	sink->stream[sink->streamCount + 0] = a;
	sink->stream[sink->streamCount + 1] = b;
	sink->stream[sink->streamCount + 2] = c;
	sink->streamCount += 3;
	}

// `fsqrt` follows the x87 control word and the CRT's sqrt() does not. Task 3
// measured that directly: with the word at 0x0f7f, sqrt(2.0) is
// 3ff6a09e667f3bcd from the CRT and 3ff6a09e667f3bcc from fsqrt. Every square
// root in this file is one the oracle takes with `fsqrt` from inside the
// simulation step, so the library routine is a real difference and not a
// last-bit nicety -- under round-toward-zero it moved 95 words of
// phys_fn_001377 and 10 of phys_fn_001923, every one of them one ulp and every
// one of them under 0x0f7f only.
//
// The block stores its result rather than leaving it in st(0). Leaving it there
// is the documented MSVC idiom for returning a double out of inline assembly,
// and it was tried first because it avoids an 8-byte round trip -- but it makes
// the caller responsible for popping a register the compiler did not put there,
// and whether it does depends on inlining. That showed up as the *oracle's*
// digest for contact_plane_capsule moving, in a block where only the candidate
// side calls this at all: 3d216c67a6297c4b became 98fe2a9a73c62b4b with every
// branch count and the word count unchanged. An instrument whose oracle half
// moves when the reconstruction is recompiled is not measuring the oracle.
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

static NxU32 nxBits(NxReal value)
	{
	NxU32 word;
	memcpy(&word, &value, 4);
	return word;
	}

// The three stream levels, factored out because there are two implementations
// of them in the oracle and therefore two here.
//
// phys_fn_000873 at 0x0001d610 is the emitter that fifteen inventory rows call.
// phys_fn_001883, matrix A [PLANE][BOX], does not call it: it writes
// `sink->[0x34]`, `+0x20` and `+0x24` itself at 0x00047fdf, 0x00047fec and
// 0x00047ff5 and appends through its own `lea esi,[edi+0x38]`. So the oracle
// carries a second copy, and a reconstruction that transcribed both would carry
// a second copy too -- with nothing forcing the two to stay in step.
//
// HOW A FUTURE READER CHECKS THEY STILL AGREE: by there being nothing left to
// check. Everything the two oracle copies do identically is written once, in the
// three helpers below, and called from both. What phys_fn_001883 duplicates is
// only the *predicates* -- when a header is written and when a normal block is
// -- and those are exactly where the two oracle copies disagree, which is
// recorded on NxContactPlaneBox with the addresses that establish it. If a
// future edit changes an append rule it changes it for both by construction; if
// it changes a predicate, it changes only the row whose predicate it is.
//
// The differential is the second half of that: contact_emit drives
// phys_fn_000873 and contact_plane_box drives the inlined copy, each against its
// own counterpart in the pinned DLL, so a drift that these helpers cannot
// prevent is a drift one of the two blocks fails on.

// The material the pair header carries in its high byte. shape1's owner is
// consulted first and shape0's is the fallback -- 0x0001d726..0x0001d73e in the
// emitter and 0x00048053..0x0004806b in plane/box, which are the same three
// loads in the same order.
static NxU32 nxHeaderMaterial(const NxCollisionShape* shape1, const NxCollisionShape* shape0)
	{
	const NxU8* holder = *(const NxU8* const*) ((const NxU8*) shape1->owner + 8);
	if(!holder)
		holder = *(const NxU8* const*) ((const NxU8*) shape0->owner + 8);
	return *(const NxU32*) (holder + 0x240);
	}

// Level one: the pair header. Three words, and writing it clears the cached
// normal so the block below always follows.
static void nxAppendPairHeader(NxContactSink* sink, void* object1, void* object0,
	NxU32 material, NxU32 packed)
	{
	sink->lastObject1 = object1;
	sink->lastObject0 = object0;

	nxAppend(sink, (NxU32) (size_t) object1);
	nxAppend(sink, (NxU32) (size_t) object0);

	sink->normalCountIndex = sink->streamCount;
	nxAppend(sink, (material << 24) | packed);
	++sink->stream[sink->pairCountIndex];

	sink->lastNormal[0] = 0.0f;
	sink->lastNormal[1] = 0.0f;
	sink->lastNormal[2] = 0.0f;
	}

// Level two: the normal and the count word its contacts increment.
static void nxAppendNormalBlock(NxContactSink* sink, const NxVec3* normal)
	{
	sink->lastNormal[0] = normal->x;
	sink->lastNormal[1] = normal->y;
	sink->lastNormal[2] = normal->z;

	nxAppend3(sink, nxBits(normal->x), nxBits(normal->y), nxBits(normal->z));

	sink->pointCountIndex = sink->streamCount;
	nxAppend(sink, 0);
	++sink->stream[sink->normalCountIndex];
	}

// Level three: the contact itself. Four words, or five where both feature ids
// are real -- and the contact counter is incremented before any of them.
static void nxAppendContactRecord(NxContactSink* sink, const NxVec3* point,
	NxU32 separationBits, NxU32 featureWord)
	{
	++sink->contactCount;

	nxAppend3(sink, nxBits(point->x), nxBits(point->y), nxBits(point->z));
	// The sign bit is masked off, not negated -- `and ebx, 0x7fffffff` at
	// 0x0001d86d in the emitter and 0x000481bf in plane/box. For the negative
	// separations a penetrating contact produces the two are indistinguishable,
	// which is why this reads as a negation.
	nxAppend(sink, separationBits & 0x7fffffffu);
	++sink->stream[sink->pointCountIndex];

	if(sink->featurePairValid & 1)
		nxAppend(sink, featureWord);
	}

// phys_fn_000873 at 0x0001d610.
//
// Three nested levels, each opened by a count word that later appends increment
// in place. The pair header is written only when either shape's collision
// object differs from what the sink last recorded; the normal block only when
// the normal differs from the cached one; the contact record always.
void NxEmitContact(NxContactSink* sink, void* object1, void* object0,
	NxU32 separationBits, const NxVec3* point, const NxVec3* normal,
	NxU16 featureId0, NxU16 featureId1)
	{
	// `shape->[0x9c]->[8]` is the shape and `shape->[4]` its owner: borrowed
	// Phase 5 layout, established at 0x000247e9 and 0x00025543.
	const NxCollisionShape* shape1 = *(const NxCollisionShape* const*) ((const NxU8*) object1 + 8);
	const NxCollisionShape* shape0 = *(const NxCollisionShape* const*) ((const NxU8*) object0 + 8);
	const void* orientation = *(void* const*) ((const NxU8*) shape1->owner + 8);

	NxVec3 negated;
	if(orientation != sink->orientedTo)
		{
		// 0x0001d632..0x0001d66e: the pair is emitted the other way round, the
		// two feature ids are swapped, and every component of the normal is
		// negated into a local the normal argument is then repointed at.
		const NxCollisionShape* swapShape = shape1;
		shape1 = shape0;
		shape0 = swapShape;

		NxU16 swapFeature = featureId0;
		featureId0 = featureId1;
		featureId1 = swapFeature;

		negated.x = (NxReal) (-(double) normal->x);
		negated.y = (NxReal) (-(double) normal->y);
		negated.z = (NxReal) (-(double) normal->z);
		normal = &negated;
		}

	if(sink->lastObject1 != shape1->collisionObject || sink->lastObject0 != shape0->collisionObject)
		{
		// The flag is an integer 1 or 0, and the header word carries it shifted
		// into bits 16..31 alongside the material in bits 24..31.
		sink->featurePairValid = (featureId0 != 0xffff && featureId1 != 0xffff) ? 1u : 0u;
		// The cached normal is cleared by the header, so the block below always
		// writes -- unless the normal is bit-for-bit zero, which is the `w7`
		// state the emitter block measures.
		nxAppendPairHeader(sink, shape1->collisionObject, shape0->collisionObject,
			nxHeaderMaterial(shape1, shape0), sink->featurePairValid << 16);
		}

	// The comparison is on the raw words, not on float equality: the oracle
	// compares with `cmp` at 0x0001d78c, 0x0001d798 and 0x0001d7a0, so two
	// normals that are equal as floats but differ in bits -- +0.0 against -0.0,
	// or two NaNs -- are a change, and two identical NaNs are not.
	if(nxBits(sink->lastNormal[0]) != nxBits(normal->x)
		|| nxBits(sink->lastNormal[1]) != nxBits(normal->y)
		|| nxBits(sink->lastNormal[2]) != nxBits(normal->z))
		nxAppendNormalBlock(sink, normal);

	nxAppendContactRecord(sink, point, separationBits,
		((NxU32) featureId1 << 16) | (NxU32) featureId0);
	}

// phys_fn_001901 at 0x00048a70, matrix A slot [PLANE][SPHERE].
//
// The plane's own normal is handed to the emitter in place -- the entry never
// copies it -- and the contact point is `centre - radius * normal`, a point on
// the sphere's surface. The separation is stored into the caller's second
// argument slot at 0x00048aa5 and read back out of it at 0x00048ad4, which is
// why it arrives at the emitter as raw bits rather than as a register value.
void __cdecl NxContactPlaneSphere(const NxCollisionShape* plane,
	const NxCollisionShape* sphere, NxContactSink* sink, void* context)
	{
	(void) context;
	const NxReal* n = plane->geometry;
	const NxReal* c = sphere->translation;

	double radius = sphere->geometry[0];
	double distance = ((double) c[2] * n[2] + (double) c[1] * n[1]) + (double) c[0] * n[0];
	distance += plane->geometry[3];

	// `fst` at 0x00048aa5, not `fstp`: the narrowed copy is what the emitter is
	// handed, and the comparison at 0x00048aa9 is on the value still in the
	// register. Those are not the same number.
	double separation = distance - radius;
	NxReal separationStored = (NxReal) separation;
	if(!(separation <= 0.0))
		return;

	// The z product is pushed through a 32-bit slot at 0x00048ae5 and read back
	// at 0x00048b08; x and y are subtracted from the register copies.
	double scaledX = radius * n[0];
	double scaledY = radius * n[1];
	NxReal scaledZ = (NxReal) (radius * n[2]);

	NxVec3 point;
	point.x = (NxReal) ((double) c[0] - scaledX);
	point.y = (NxReal) ((double) c[1] - scaledY);
	point.z = (NxReal) ((double) c[2] - (double) scaledZ);

	NxEmitContact(sink, sphere->collisionObject, plane->collisionObject,
		nxBits(separationStored), &point, (const NxVec3*) n, 0xffff, 0xffff);
	}

// phys_fn_001261 at 0x00025350: what a PLANE shape puts in vtable slot 5.
//
// It is a segment/plane raycast whose only callee is NxRayPlaneIntersect at
// 0x00036bb0, a Task 2 export. Three gates in order -- the ray must run into
// the plane, the hit must be ahead of the origin, and it must be within the
// distance limit -- and every one of them lets an unordered comparison through,
// so a NaN anywhere in the ray or the plane reaches the hit record rather than
// being rejected.
//
// `hit.shape` is written from `shape->[0x9c]` (0x000253d8), the collision
// object, not from the shape itself. Whether that object is what the public API
// hands out as an NxShape is a Phase 5 question; this row only records which
// pointer the oracle stores.
const NxCollisionShape* __fastcall NxShapeRaycastPlane(const NxCollisionShape* plane,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit)
	{
	(void) edxUnused;
	(void) unread;
	const NxReal* n = plane->geometry;

	// 0x00025357..0x00025377, accumulated z, y, x and left in st(0).
	const double facing = ((double) n[2] * worldRay->dir.z + (double) n[1] * worldRay->dir.y)
		+ (double) n[0] * worldRay->dir.x;
	// `test ah,1; jne` at 0x00025381 reads C0 alone, which is set for "less"
	// and for "unordered" alike, so a NaN dot product continues.
	if(facing >= 0.0)
		return 0;

	// The oracle passes the address of its own first argument slot as `dist`
	// (0x00025396) -- a store into the caller's frame, dead by then.
	NxReal distance;
	if(!NxRayPlaneIntersect(*worldRay, *(const NxPlane*) n, distance, hit->worldImpact))
		return 0;

	// 0x000253bb and 0x000253cc, both `test ah,0x41` over C3 and C0 but with
	// opposite branches: strictly ahead of the origin, and not past the limit.
	// Both let the unordered case through.
	if(distance <= 0.0f)
		return 0;
	if(distance > maxDistance)
		return 0;

	// Integer moves, not float stores: an fld/fstp pair would quiet a
	// signalling NaN, and these are `mov` at 0x000253d1 and 0x000253ff.
	memcpy(&hit->distance, &distance, 4);
	hit->shape = (NxShape*) plane->collisionObject;
	hit->faceID = 0;
	hit->u = 0.0f;
	hit->v = 0.0f;
	hit->flags = NX_RAYCAST_SHAPE | NX_RAYCAST_IMPACT | NX_RAYCAST_DISTANCE;
	if(hintFlags & NX_RAYCAST_NORMAL)
		{
		memcpy(&hit->worldNormal.x, &n[0], 4);
		memcpy(&hit->worldNormal.y, &n[1], 4);
		memcpy(&hit->worldNormal.z, &n[2], 4);
		hit->flags |= NX_RAYCAST_NORMAL;
		}
	return plane;
	}

// phys_fn_001891 at 0x00048370, matrix A slot [PLANE][CAPSULE].
//
// The byte at `capsule+0xe8` picks between two entirely different algorithms
// (`test al,1` at 0x000483aa, branch at 0x00048400). It is
// NxCapsuleShapeDesc::flags: 0x00021ad0 loads the capsule's geometry from its
// descriptor and copies `desc+0x54` straight into `+0xe8` at 0x00021af9, one
// field after the radius at `desc+0x4c` and the height at `desc+0x50`. The only
// bit that enum defines is NX_SWEPT_SHAPE, and bit 0 is the bit tested.
//
// Both paths build the two endpoints the way phys_fn_001889 does -- the same
// column-1 axis, the same narrowed x half-axis, the same narrowed -axisZ -- with
// one difference: p1.z is narrowed here (0x000483fc) where the overlap test
// leaves it in a register.
void __cdecl NxContactPlaneCapsule(const NxCollisionShape* plane,
	const NxCollisionShape* capsule, NxContactSink* sink, void* context)
	{
	(void) context;
	const NxReal* m = capsule->rotation;
	const NxReal* t = capsule->translation;

	// Stored over the caller's second argument slot at 0x0004839c and read back
	// from it twice. The swept path overwrites that slot with the inverse
	// length at 0x0004847b, which is safe only because it never reads the
	// radius again.
	const NxReal radius = capsule->geometry[0];
	const NxReal halfHeight = capsule->geometry[1];

	const NxReal axisX = (NxReal) ((double) m[1] * halfHeight);
	const double axisY = (double) m[4] * halfHeight;
	const double axisZ = (double) m[7] * halfHeight;
	const NxReal negatedAxisZ = (NxReal) (-axisZ);

	NxVec3 p0, p1;
	p0.x = (NxReal) (-(double) axisX + t[0]);
	p0.y = (NxReal) (-axisY + t[1]);
	p0.z = (NxReal) ((double) negatedAxisZ + t[2]);
	p1.x = (NxReal) ((double) axisX + t[0]);
	p1.y = (NxReal) (axisY + t[1]);
	p1.z = (NxReal) (axisZ + t[2]);

	// `mov al, byte ptr [edi+0xe8]`: the low byte of the flags word, not the
	// float in that union slot.
	const NxU8 sweptFlag = *(const NxU8*) &capsule->geometry[2];
	if(sweptFlag & 1)
		{
		// 0x00048406. A moving sphere: normalise the endpoint difference and
		// ask the partner shape's own vtable slot 5 to raycast the segment.
		const NxReal dx = (NxReal) ((double) p1.x - p0.x);
		const NxReal dy = (NxReal) ((double) p1.y - p0.y);
		const NxReal dz = (NxReal) ((double) p1.z - p0.z);

		// Every squared term reads its 32-bit slot back, so the length is not
		// formed from the register copies.
		const NxReal length = (NxReal) nxSqrt(((double) dz * dz + (double) dy * dy)
			+ (double) dx * dx);

		// `fucompp` against the 0.0f at 0x101041f0 with `jnp`: the equal case
		// skips the normalisation and the *unnormalised* difference is what
		// reaches slot 5. An unordered compare normalises.
		NxVec3 direction;
		if(length == 0.0f)
			{
			// The two slots the zero path leaves alone hold the raw copies made
			// at 0x00048426 and 0x0004843a; the x slot is written from the
			// register copy of the same subtraction, which narrows to `dx`.
			direction.x = dx;
			direction.y = dy;
			direction.z = dz;
			}
		else
			{
			const NxReal inverse = (NxReal) (1.0 / (double) length);
			direction.x = (NxReal) ((double) dx * inverse);
			direction.y = (NxReal) ((double) dy * inverse);
			direction.z = (NxReal) ((double) dz * inverse);
			}

		NxRay ray;
		ray.orig = p0;
		ray.dir = direction;

		// `mov edx,[esi]; call dword ptr [edx+0x14]` at 0x000484e1-0x000484e6.
		// A closure over direct call edges cannot see this, which is why the
		// entry survey undercounted four of the eight matrix A rows.
		NxRaycastHit hit;
		const NxShapeRaycastFn raycast = (*(const NxShapeRaycastFn* const*) plane)[5];
		if(!raycast(plane, &ray, length, 0, 0, &hit))
			return;

		// The separation is an immediate `push 0` at 0x00048513, not a computed
		// value, and the point is the impact point the raycast wrote.
		NxEmitContact(sink, capsule->collisionObject, plane->collisionObject,
			0, &hit.worldImpact, (const NxVec3*) plane->geometry, 0xffff, 0xffff);
		return;
		}

	// 0x00048529. A plane distance per endpoint, and a contact for each that is
	// strictly closer than the radius -- so up to two contacts from one call.
	const NxReal* n = plane->geometry;

	// dist0 stays in st(0) for the comparison, the point and the separation;
	// dist1 is pushed through the caller's first argument slot at 0x00048572
	// and every later use reads the narrowed copy back. The two endpoints are
	// therefore not treated at the same precision.
	const double distance0 = (((double) p0.z * n[2] + (double) p0.y * n[1])
		+ (double) p0.x * n[0]) + n[3];
	const NxReal distance1 = (NxReal) ((((double) p1.y * n[1] + (double) p1.z * n[2])
		+ (double) p1.x * n[0]) + n[3]);

	// `fcom` then `test ah,5` then `jp`: strictly less, and an unordered
	// comparison emits nothing.
	if(distance0 < (double) radius)
		{
		// point = endpoint - distance * normal, which puts the point on the
		// PLANE. plane/sphere scales the normal by the radius instead and puts
		// its point on the sphere; the two entries do not agree.
		const double scaledX = distance0 * n[0];
		const double scaledY = distance0 * n[1];
		const NxReal scaledZ = (NxReal) (distance0 * n[2]);

		NxVec3 point;
		point.x = (NxReal) ((double) p0.x - scaledX);
		point.y = (NxReal) ((double) p0.y - scaledY);
		point.z = (NxReal) ((double) p0.z - (double) scaledZ);

		const NxReal separation = (NxReal) (distance0 - radius);
		NxEmitContact(sink, capsule->collisionObject, plane->collisionObject,
			nxBits(separation), &point, (const NxVec3*) n, 0xffff, 0xffff);
		}

	if((double) distance1 < (double) radius)
		{
		const double scaledX = (double) distance1 * n[0];
		const double scaledY = (double) distance1 * n[1];
		const NxReal scaledZ = (NxReal) ((double) distance1 * n[2]);

		NxVec3 point;
		point.x = (NxReal) ((double) p1.x - scaledX);
		point.y = (NxReal) ((double) p1.y - scaledY);
		point.z = (NxReal) ((double) p1.z - (double) scaledZ);

		const NxReal separation = (NxReal) ((double) distance1 - radius);
		NxEmitContact(sink, capsule->collisionObject, plane->collisionObject,
			nxBits(separation), &point, (const NxVec3*) n, 0xffff, 0xffff);
		}
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

// phys_fn_001923 at 0x0004a4b0, matrix A slot [SPHERE][CAPSULE].
//
// The same NX_SWEPT_SHAPE split as plane/capsule and the same endpoint
// construction, and then four differences worth naming before the code: slot 5
// is called on the *sphere*, it is passed NX_RAYCAST_NORMAL rather than 0, the
// normal handed to the emitter is the raycast's own rather than a shape's
// geometry passed in place, and the flag-clear path is a segment/point distance
// rather than two plane distances -- so it emits one contact where
// plane/capsule emits up to two.
void __cdecl NxContactSphereCapsule(const NxCollisionShape* sphere,
	const NxCollisionShape* capsule, NxContactSink* sink, void* context)
	{
	(void) context;
	const NxReal* m = capsule->rotation;
	const NxReal* t = capsule->translation;
	const NxReal halfHeight = capsule->geometry[1];

	const NxReal axisX = (NxReal) ((double) m[1] * halfHeight);
	const double axisY = (double) m[4] * halfHeight;
	const double axisZ = (double) m[7] * halfHeight;
	const NxReal negatedAxisZ = (NxReal) (-axisZ);

	NxSegment segment;
	segment.p0.x = (NxReal) (-(double) axisX + t[0]);
	segment.p0.y = (NxReal) (-axisY + t[1]);
	segment.p0.z = (NxReal) ((double) negatedAxisZ + t[2]);
	segment.p1.x = (NxReal) ((double) axisX + t[0]);
	segment.p1.y = (NxReal) (axisY + t[1]);
	segment.p1.z = (NxReal) (axisZ + t[2]);

	const NxVec3* centre = (const NxVec3*) sphere->translation;
	const NxU8 sweptFlag = *(const NxU8*) &capsule->geometry[2];
	if(sweptFlag & 1)
		{
		// 0x0004a541, identical to plane/capsule's down to the constants.
		const NxReal dx = (NxReal) ((double) segment.p1.x - segment.p0.x);
		const NxReal dy = (NxReal) ((double) segment.p1.y - segment.p0.y);
		const NxReal dz = (NxReal) ((double) segment.p1.z - segment.p0.z);
		const NxReal length = (NxReal) nxSqrt(((double) dz * dz + (double) dy * dy)
			+ (double) dx * dx);

		NxVec3 direction;
		if(length == 0.0f)
			{
			direction.x = dx;
			direction.y = dy;
			direction.z = dz;
			}
		else
			{
			// Stored over the half height's slot here where plane/capsule puts
			// it over the radius. Both are dead on this path.
			const NxReal inverse = (NxReal) (1.0 / (double) length);
			direction.x = (NxReal) ((double) dx * inverse);
			direction.y = (NxReal) ((double) dy * inverse);
			direction.z = (NxReal) ((double) dz * inverse);
			}

		NxRay ray;
		ray.orig = segment.p0;
		ray.dir = direction;

		// `push 4` at 0x0004a60a, where plane/capsule pushes 0. So this caller
		// does ask for the normal, and reads it back from `hit+0x10` while the
		// point comes from `hit+0x04` -- the NxRaycastHit layout confirmed from
		// a second caller that uses a field the first never asks for.
		NxRaycastHit hit;
		const NxShapeRaycastFn raycast = (*(const NxShapeRaycastFn* const*) sphere)[5];
		if(!raycast(sphere, &ray, length, 0, NX_RAYCAST_NORMAL, &hit))
			return;

		// The capsule's radius is not read anywhere on this path: a swept
		// capsule against a sphere is a zero-radius ray.
		NxEmitContact(sink, capsule->collisionObject, sphere->collisionObject,
			0, &hit.worldImpact, &hit.worldNormal, 0xffff, 0xffff);
		return;
		}

	// 0x0004a668. The squared distance from the sphere's centre to the capsule
	// axis, against the sum of the two radii.
	const NxReal sphereRadius = sphere->geometry[0];
	const NxReal radiusSum = (NxReal) ((double) sphereRadius + capsule->geometry[0]);

	NxReal parameter;
	const NxReal squared = NxComputeSquareDistance(segment, *centre, &parameter);

	// `fcomp` then `test ah,0x41` then `jne`: strictly greater, and an unordered
	// comparison emits nothing -- the same convention the sphere/sphere and
	// sphere/capsule overlap tests use, so two shapes exactly touching produce
	// no contact.
	if(!((double) radiusSum * radiusSum > (double) squared))
		return;

	// The closest point on the axis, at the parameter the distance call
	// returned. x is narrowed after its multiply at 0x0004a6dc and z before it
	// at 0x0004a6d2, and y is not narrowed at all, so the three components are
	// not formed at the same precision.
	const double axisDx = (double) segment.p1.x - segment.p0.x;
	const double axisDy = (double) segment.p1.y - segment.p0.y;
	const NxReal axisDz = (NxReal) ((double) segment.p1.z - segment.p0.z);
	const NxReal alongX = (NxReal) (axisDx * parameter);
	const NxReal closestX = (NxReal) ((double) alongX + segment.p0.x);
	const NxReal closestY = (NxReal) (axisDy * parameter + segment.p0.y);
	// `fst` at 0x0004a716: the narrowed copy is dead -- the contact point
	// overwrites that slot -- and the wide value is what the normal is built
	// from.
	const double closestZ = (double) axisDz * parameter + segment.p0.z;

	NxVec3 normal;
	normal.x = (NxReal) ((double) closestX - centre->x);
	normal.y = (NxReal) ((double) closestY - centre->y);
	const double normalZ = closestZ - centre->z;

	const double length = nxSqrt((normalZ * normalZ + (double) normal.y * normal.y)
		+ (double) normal.x * normal.x);
	// Unlike the swept path, a zero-length normal here emits nothing at all:
	// 0x0004a811 pops twice and returns.
	if(length == 0.0)
		return;

	const double inverse = 1.0 / length;
	normal.x = (NxReal) ((double) normal.x * inverse);
	normal.y = (NxReal) ((double) normal.y * inverse);
	normal.z = (NxReal) (normalZ * inverse);

	// point = centre + sphereRadius * normal, so the point is on the SPHERE --
	// plane/sphere's convention and not plane/capsule's. The scale is the
	// sphere's own radius and not the sum that decided the test.
	const double scaledX = (double) normal.x * sphereRadius;
	const NxReal scaledY = (NxReal) ((double) normal.y * sphereRadius);
	const NxReal scaledZ = (NxReal) ((double) normal.z * sphereRadius);

	NxVec3 point;
	point.x = (NxReal) (scaledX + centre->x);
	point.y = (NxReal) ((double) scaledY + centre->y);
	point.z = (NxReal) ((double) scaledZ + centre->z);

	// The distance is recovered by taking the square root of the *narrowed*
	// squared distance at 0x0004a7f3 rather than kept from anything above.
	const NxReal separation = (NxReal) (nxSqrt((double) squared) - radiusSum);
	NxEmitContact(sink, capsule->collisionObject, sphere->collisionObject,
		nxBits(separation), &point, &normal, 0xffff, 0xffff);
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

// phys_fn_001775 at 0x0003d9d0, matrix A slot [CAPSULE][CAPSULE], 2,463 bytes.
//
// Two unrelated algorithms behind one flag test, like the other three capsule
// entries -- and then the non-swept half is itself two algorithms with a
// fallthrough between them, which none of the others is.
//
// THE SWEPT HALF IS NOT A FUNCTION OF ITS ARGUMENTS. It passes hintFlags = 4 and
// then hands the emitter `hit.worldNormal`, and the callee it dispatches to is
// always phys_fn_001010 -- both shapes are capsules, so there is no
// configuration in which any other slot-5 row runs -- and phys_fn_001010 writes
// no normal at all. The three words the emitter receives as the contact normal
// are whatever the caller's stack left at `esp+0xcc`. This is the second such
// case in the programme after NxSeparatingAxis with fullTest = false, and it is
// the worse of the two: that one reaches a return value and this one reaches a
// contact record. Reproduced, not corrected -- `hit` below is deliberately
// uninitialised and `&hit.worldNormal` is deliberately passed. The differential
// seeds the stack under both calls with one repeated dword so that the read is
// the same on both sides, and measures that the seed really does come out.
//
// The swept branch is the OR of the two flag bytes at 0x0003da3a, but which
// capsule becomes the ray is shape1's flag *alone* at 0x0003dae9 and the
// receiver is the other one. So the predicate and the selector read different
// things and all four flag combinations are distinct inputs.
void __cdecl NxContactCapsuleCapsule(const NxCollisionShape* capsule0,
	const NxCollisionShape* capsule1, NxContactSink* sink, void* context)
	{
	(void) context;

	const NxCollisionShape* const shapes[2] = { capsule0, capsule1 };
	NxSegment segment[2];
	for(int which = 0; which < 2; ++which)
		{
		const NxReal* m = shapes[which]->rotation;
		const NxReal* translation = shapes[which]->translation;
		const NxReal halfHeight = shapes[which]->geometry[1];
		const NxReal axisX = (NxReal) ((double) m[1] * halfHeight);
		const double axisY = (double) m[4] * halfHeight;
		const double axisZ = (double) m[7] * halfHeight;
		const NxReal negatedAxisZ = (NxReal) (-axisZ);
		segment[which].p0.x = (NxReal) (-(double) axisX + translation[0]);
		segment[which].p0.y = (NxReal) (-axisY + translation[1]);
		segment[which].p0.z = (NxReal) ((double) negatedAxisZ + translation[2]);
		segment[which].p1.x = (NxReal) ((double) axisX + translation[0]);
		segment[which].p1.y = (NxReal) (axisY + translation[1]);
		segment[which].p1.z = (NxReal) (axisZ + translation[2]);
		}

	NxU32 flags0, flags1;
	memcpy(&flags0, &capsule0->geometry[2], 4);
	memcpy(&flags1, &capsule1->geometry[2], 4);

	if((flags0 | flags1) & 1)
		{
		// 0x0003dae9. shape1's bit alone picks the ray.
		const unsigned rayIndex = flags1 & 1u;
		const unsigned targetIndex = 1u - rayIndex;
		const NxSegment& ray = segment[rayIndex];

		const double wideX = (double) ray.p1.x - ray.p0.x;
		const NxReal deltaX = (NxReal) wideX;
		const NxReal deltaY = (NxReal) ((double) ray.p1.y - ray.p0.y);
		const NxReal deltaZ = (NxReal) ((double) ray.p1.z - ray.p0.z);
		const NxReal length = (NxReal) nxSqrt(((double) deltaX * deltaX
			+ (double) deltaZ * deltaZ) + (double) deltaY * deltaY);

		NxRay worldRay;
		worldRay.orig = ray.p0;
		// A zero length skips the normalisation but not the call, and what
		// reaches slot 5 is then the raw difference -- with x still wide, since
		// the `fst` at 0x0003db07 kept it in st(0). Same guard as
		// plane/capsule's, same consequence.
		if(length != 0.0f)
			{
			const NxReal inverse = (NxReal) ((double) 1.0f / length);
			worldRay.dir.x = (NxReal) ((double) deltaX * inverse);
			worldRay.dir.y = (NxReal) ((double) deltaY * inverse);
			worldRay.dir.z = (NxReal) ((double) deltaZ * inverse);
			}
		else
			{
			worldRay.dir.x = (NxReal) wideX;
			worldRay.dir.y = deltaY;
			worldRay.dir.z = deltaZ;
			}

		// Deliberately uninitialised; see the comment above the function.
		NxRaycastHit hit;
		const NxCollisionShape* const receiver = shapes[targetIndex];
		const NxShapeRaycastFn slot5 =
			(NxShapeRaycastFn) (*(void***) receiver)[5];
		if(slot5(receiver, &worldRay, length, 0, 4, &hit))
			NxEmitContact(sink, shapes[rayIndex]->collisionObject,
				receiver->collisionObject, 0, &hit.worldImpact, &hit.worldNormal,
				0xffff, 0xffff);
		return;
		}

	// 0x0003dc4e. phys_fn_001690 is a Phase 2 row; see NarrowPhase.cpp.
	NxReal parameter0, parameter1;
	const double squared = NxSegmentSegmentSquareDistance(&segment[0], &segment[1],
		&parameter0, &parameter1);
	const double wideSum = (double) capsule1->geometry[0] + capsule0->geometry[0];
	const NxReal radiusSum = (NxReal) wideSum;
	// Strictly less: two capsules exactly touching produce nothing, and an
	// unordered comparison produces nothing either.
	if(!(squared < wideSum * radiusSum))
		return;

	// dir[k] is the axis direction, normalised unless its length is exactly
	// zero, in which case the raw difference stays. length[k] is kept because
	// the endpoint loop below clips against it.
	NxVec3 direction[2];
	NxReal length[2];
	const double wideX0 = (double) segment[0].p1.x - segment[0].p0.x;
	const NxReal deltaY0 = (NxReal) ((double) segment[0].p1.y - segment[0].p0.y);
	const double wideZ0 = (double) segment[0].p1.z - segment[0].p0.z;
	direction[0].x = (NxReal) wideX0;
	direction[0].y = deltaY0;
	direction[0].z = (NxReal) wideZ0;
	const double wideX1 = (double) segment[1].p1.x - segment[1].p0.x;
	const NxReal deltaX1 = (NxReal) wideX1;
	const NxReal deltaY1 = (NxReal) ((double) segment[1].p1.y - segment[1].p0.y);
	const NxReal deltaZ1 = (NxReal) ((double) segment[1].p1.z - segment[1].p0.z);
	direction[1].x = (NxReal) wideX1;
	direction[1].y = deltaY1;
	direction[1].z = deltaZ1;

	// The two lengths are not formed the same way: the first squares the wide x
	// and z still in registers and the second reads all three back from their
	// 32-bit slots.
	length[0] = (NxReal) nxSqrt((wideX0 * wideX0 + wideZ0 * wideZ0)
		+ (double) deltaY0 * deltaY0);
	if(length[0] != 0.0f)
		{
		const double inverse = (double) 1.0f / length[0];
		direction[0].x = (NxReal) (inverse * wideX0);
		direction[0].y = (NxReal) ((double) deltaY0 * inverse);
		direction[0].z = (NxReal) (wideZ0 * inverse);
		}
	const double wideLength1 = nxSqrt(((double) deltaX1 * deltaX1
		+ (double) deltaZ1 * deltaZ1) + (double) deltaY1 * deltaY1);
	if(wideLength1 != 0.0f)
		{
		// And the second divides by the *wide* length where the first divides by
		// the narrowed one.
		const double inverse = (double) 1.0f / wideLength1;
		direction[1].x = (NxReal) ((double) deltaX1 * inverse);
		direction[1].y = (NxReal) ((double) deltaY1 * inverse);
		direction[1].z = (NxReal) ((double) deltaZ1 * inverse);
		}
	length[1] = (NxReal) wideLength1;

	const double dot = ((double) direction[1].x * direction[0].x
		+ (double) direction[1].z * direction[0].z)
		+ (double) direction[1].y * direction[0].y;

	// 0x00107bf4 is 0.9998f. Strictly greater, so an unordered comparison takes
	// the single-contact path.
	NxU32 emitted = 0;
	if(dot > 0.9998f)
		{
		// The endpoint clip, up to four contacts. 0x00107690 is 0.001f, so the
		// tolerance is a tenth of a percent of the axis length -- and it is a
		// tolerance on the *parameter range*, not on the distance.
		NxReal tolerance[2];
		tolerance[0] = (NxReal) ((double) length[0] * 0.001f);
		tolerance[1] = (NxReal) (wideLength1 * 0.001f);

		NxVec3 point[2];
		for(unsigned index = 0; index < 2; ++index)
			{
			const NxReal lowerBound = (NxReal) (-(double) tolerance[index]);
			const unsigned other = 1u - index;
			const NxSegment& axis = segment[index];
			const NxVec3& along = direction[index];

			for(unsigned end = 0; end < 2; ++end)
				{
				point[index] = end ? segment[other].p1 : segment[other].p0;

				const double projection =
					(((double) point[index].z - axis.p0.z) * along.z
						+ ((double) point[index].y - axis.p0.y) * along.y)
					+ ((double) point[index].x - axis.p0.x) * along.x;
				const NxReal narrowedProjection = (NxReal) projection;
				if(projection < lowerBound)
					continue;
				if((double) length[index] + tolerance[index] < narrowedProjection)
					continue;

				const NxReal scaledY = (NxReal) ((double) narrowedProjection * along.y);
				const NxReal scaledZ = (NxReal) ((double) narrowedProjection * along.z);
				point[other].x = (NxReal) ((double) narrowedProjection * along.x + axis.p0.x);
				point[other].y = (NxReal) ((double) scaledY + axis.p0.y);
				point[other].z = (NxReal) ((double) scaledZ + axis.p0.z);

				const double normalX = (double) point[1].x - point[0].x;
				const NxReal normalY = (NxReal) ((double) point[1].y - point[0].y);
				const double normalZ = (double) point[1].z - point[0].z;
				const double reverseX = (double) point[0].x - point[1].x;
				const double reverseY = (double) point[0].y - point[1].y;
				const double reverseZ = (double) point[0].z - point[1].z;
				const double gap = nxSqrt((reverseZ * reverseZ + reverseY * reverseY)
					+ reverseX * reverseX);
				const NxReal separation = (NxReal) (gap - radiusSum);

				const double normalLength = nxSqrt((normalZ * normalZ
					+ (double) normalY * normalY) + normalX * normalX);
				// A zero-length normal skips the contact here, where the
				// single-contact path below returns outright.
				if(normalLength == 0.0)
					continue;
				const double inverse = (double) 1.0f / normalLength;
				NxVec3 normal;
				normal.x = (NxReal) (inverse * normalX);
				normal.y = (NxReal) ((double) normalY * inverse);
				normal.z = (NxReal) (normalZ * inverse);

				// Strictly penetrating, and an unordered separation emits
				// nothing.
				if(!(separation < 0.0f))
					continue;

				// The radius of the *other* capsule, and the point is always
				// point[1] whichever way round the loop is.
				const double radius = shapes[other]->geometry[0];
				const NxReal scaledNormalY = (NxReal) ((double) normal.y * radius);
				const NxReal scaledNormalZ = (NxReal) ((double) normal.z * radius);
				NxVec3 contact;
				contact.x = (NxReal) ((double) point[1].x - (double) normal.x * radius);
				contact.y = (NxReal) ((double) point[1].y - scaledNormalY);
				contact.z = (NxReal) ((double) point[1].z - scaledNormalZ);

				NxEmitContact(sink, capsule0->collisionObject, capsule1->collisionObject,
					nxBits(separation), &contact, &normal, 0xffff, 0xffff);
				++emitted;
				}
			}
		if(emitted)
			return;
		// And when the clip emitted nothing it falls through, so one call can
		// run both algorithms. 0x0003e162.
		}

	// 0x0003e184, one contact at the closest points.
	const NxReal firstZ = (NxReal) ((double) segment[0].p1.z - segment[0].p0.z);
	const NxReal firstScaledX = (NxReal) (((double) segment[0].p1.x - segment[0].p0.x)
		* parameter0);
	const double firstScaledY = ((double) segment[0].p1.y - segment[0].p0.y) * parameter0;
	const double firstScaledZ = (double) firstZ * parameter0;
	const NxReal closest0X = (NxReal) ((double) firstScaledX + segment[0].p0.x);
	const NxReal closest0Y = (NxReal) (firstScaledY + segment[0].p0.y);
	const double closest0Z = firstScaledZ + segment[0].p0.z;

	const NxReal secondZ = (NxReal) ((double) segment[1].p1.z - segment[1].p0.z);
	const NxReal secondScaledX = (NxReal) (((double) segment[1].p1.x - segment[1].p0.x)
		* parameter1);
	const double secondScaledY = ((double) segment[1].p1.y - segment[1].p0.y) * parameter1;
	const double secondScaledZ = (double) secondZ * parameter1;
	const NxReal closest1X = (NxReal) ((double) secondScaledX + segment[1].p0.x);
	const NxReal closest1Y = (NxReal) (secondScaledY + segment[1].p0.y);
	const double closest1Z = secondScaledZ + segment[1].p0.z;

	NxVec3 normal;
	normal.x = (NxReal) ((double) closest0X - closest1X);
	normal.y = (NxReal) ((double) closest0Y - closest1Y);
	normal.z = (NxReal) (closest0Z - closest1Z);

	const double span = nxSqrt(((double) normal.z * normal.z
		+ (double) normal.y * normal.y) + (double) normal.x * normal.x);
	if(span == 0.0)
		return;
	const double inverse = (double) 1.0f / span;
	normal.x = (NxReal) ((double) normal.x * inverse);
	normal.y = (NxReal) ((double) normal.y * inverse);
	normal.z = (NxReal) ((double) normal.z * inverse);

	// The first capsule's radius, not the sum, and the point is on segment 0.
	const double radius = capsule0->geometry[0];
	const NxReal scaledNormalY = (NxReal) ((double) normal.y * radius);
	const NxReal scaledNormalZ = (NxReal) ((double) normal.z * radius);
	NxVec3 contact;
	contact.x = (NxReal) ((double) closest0X - (double) normal.x * radius);
	contact.y = (NxReal) ((double) closest0Y - scaledNormalY);
	contact.z = (NxReal) (closest0Z - scaledNormalZ);

	// The root is re-taken from the narrowed squared distance rather than kept
	// from anything, exactly as sphere/capsule does.
	const NxReal separation = (NxReal) (nxSqrt((NxReal) squared) - radiusSum);
	NxEmitContact(sink, capsule0->collisionObject, capsule1->collisionObject,
		nxBits(separation), &contact, &normal, 0xffff, 0xffff);
	}

// phys_fn_001883 at 0x00047f20, matrix A slot [PLANE][BOX]. 42 bytes plus 786
// in two continuations the census splits at the entry's internal alignment
// padding and annotates: 0x00047f50 (9) and 0x00047f60 (777), both carrying
// `continuation of the entry at 0x00047f20`. A reader has to sum them.
//
// The one entry in this matrix that does not call phys_fn_000873. It writes the
// sink's own fields at 0x00047fdf, 0x00047fec and 0x00047ff5 and appends through
// `lea esi,[edi+0x38]`, so the oracle carries a second copy of the stream logic.
// Everything the two copies do identically is in the three helpers above and is
// written once; what is duplicated here is only where they disagree, and each
// disagreement carries the address that establishes it:
//
//  * NO HEADER PREDICATE. The emitter compares `shape1->[0x9c]` against
//    `sink->[0x20]` and `shape0->[0x9c]` against `sink->[0x24]` and skips the
//    header when both match (0x0001d67d, 0x0001d688). This writes it whenever
//    the call emits at all -- `test eax,eax; jne` at 0x00047fbc branches on this
//    call's own contact count and on nothing the sink holds. So a second
//    plane/box against the same pair rewrites the header where the emitter would
//    not, and the pair counter is incremented again with it.
//  * NO NORMAL PREDICATE. The emitter compares the normal against the cache
//    (0x0001d78c..0x0001d7a3). This writes the block unconditionally. Since the
//    header it just wrote cleared the cache, the two agree except for a plane
//    whose normal is bit-for-bit zero -- which the emitter would skip and this
//    does not.
//  * THE HEADER'S SECOND HALF IS ALWAYS ZERO. `sink->[0x34]` is set to 0 at
//    0x00047fdf rather than computed, so the feature-valid bit never reaches the
//    packed word and the fifth contact word is unreachable from this entry. The
//    test for it at 0x000481ed is still emitted and is dead.
//  * AT MOST SIX CONTACTS, from a shape with eight corners. `cmp eax,6; jae` at
//    0x00048218 returns as soon as the sixth is emitted, so a box resting
//    corner-on into a plane loses two of its eight. That is a buffer limit
//    inside a kernel rather than in a buffer, and it is the only one this
//    component can reach.
//
// It also stores a flag byte over the caller's third argument slot at
// 0x00047fcf, which is the sink pointer -- safe only because `edi` already holds
// it.
void __cdecl NxContactPlaneBox(const NxCollisionShape* plane,
	const NxCollisionShape* box, NxContactSink* sink, void* context)
	{
	(void) context;

	const NxReal* normalWords = plane->geometry;
	NxU32 emittedCount = 0;

	// signZ innermost: 0x00047f50 seeds it, 0x00048229 steps it, and the two
	// outer counters live in the frame. All three step by 2 from -1 and are
	// passed to phys_fn_000943 as full ints.
	for(int signX = -1; signX <= 1; signX += 2)
		for(int signY = -1; signY <= 1; signY += 2)
			for(int signZ = -1; signZ <= 1; signZ += 2)
				{
				NxVec3 corner;
				NxBoxShapeCorner(box, signX, signY, signZ, &corner);

				// y and z first, then x, then the plane constant -- and the
				// wide value is what the comparison sees while the narrowed
				// copy is what reaches the stream.
				const double wide = ((((double) corner.y * normalWords[1]
					+ (double) corner.z * normalWords[2])
					+ (double) corner.x * normalWords[0]) + normalWords[3]);
				const NxReal distance = (NxReal) wide;
				// `test ah,0x41` + `jp`: greater and unordered both skip, so a
				// corner exactly on the plane is a contact and a NaN is not.
				if(!(wide <= 0.0))
					continue;

				if(emittedCount == 0)
					{
					// The orientation rule, inlined. It reads the *box's*
					// owner, which is the pair's shape1, exactly as the emitter
					// does at 0x0001d61b.
					const NxCollisionShape* first = box;
					const NxCollisionShape* second = plane;
					const bool negated =
						*(void* const*) ((const NxU8*) box->owner + 8) != sink->orientedTo;
					if(negated)
						{
						first = plane;
						second = box;
						}

					sink->featurePairValid = 0;
					nxAppendPairHeader(sink, first->collisionObject,
						second->collisionObject, nxHeaderMaterial(first, second), 0);

					// The plane's own normal, negated component by component
					// through the FPU when the pair is the other way round.
					NxVec3 normal;
					if(negated)
						{
						normal.x = (NxReal) (-(double) normalWords[0]);
						normal.y = (NxReal) (-(double) normalWords[1]);
						normal.z = (NxReal) (-(double) normalWords[2]);
						}
					else
						{
						normal.x = normalWords[0];
						normal.y = normalWords[1];
						normal.z = normalWords[2];
						}
					nxAppendNormalBlock(sink, &normal);
					}

				// The contact point is the box corner itself, not a point on
				// the plane -- a third convention in one matrix column, after
				// plane/sphere putting it on the sphere and plane/capsule on
				// the plane. And the separation is the plane distance, which
				// for a corner below the plane is what the emitter's mask makes
				// indistinguishable from its magnitude.
				nxAppendContactRecord(sink, &corner, nxBits(distance), 0);

				if(++emittedCount >= 6)
					return;
				}
	}

// phys_fn_001281 at 0x000257a0. Four bytes, and its whole body is the load: the
// oracle reaches Shape+0x04 through it in both sphere entries and inline in the
// emitter, which is why the two spellings sit side by side in this file.
const void* __fastcall NxShapeOwner(const NxCollisionShape* shape, void* edxUnused)
	{
	(void) edxUnused;
	return shape->owner;
	}

// phys_fn_002266 at 0x00056650.
//
// `mov ecx,[0x10123c04]` loads the SDK singleton and phys_fn_000429 -- the
// reconstruction's PhysicsSDK::getParameter -- never touches it: 0x0000dc00
// reads its argument off the stack and indexes a file-scope array. The null
// guard below therefore changes no state the oracle can be in, because the only
// thing that fills that array is the SDK constructor, and it writes
// NX_CONTINUOUS_CD's default of 0.0f -- the same value the array holds
// statically before any SDK exists.
//
// The comparison is `fucompp` against the 0.0f at 0x101041f0 with
// `test ah,0x44; jp` at 0x00056667, which is MSVC's `==`: a NaN parameter is NOT
// equal and takes the sweep.
bool __cdecl NxContinuousCdPair(const NxCollisionShape* moving,
	const NxCollisionShape* fixed, NxContactSink* sink)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	const NxReal continuous = sdk ? sdk->getParameter(NX_CONTINUOUS_CD) : 0.0f;
	if(continuous == 0.0f)
		return true;

	// phys_fn_002264 at 0x00055eb0 runs here in the oracle. It is a stop, not an
	// omission: see the header for the three things it reads that nothing in
	// this program establishes. Leaving the sink alone is the only safe thing,
	// which is the same call this file already makes for the stream growth path.
	(void) moving;
	(void) fixed;
	(void) sink;
	return false;
	}

// phys_fn_001933 at 0x0004b860, matrix A slot [SPHERE][SPHERE]. 341 bytes.
//
// The narrowing is per component and is not symmetric. The z separation and the
// radius sum are each stored with `fst` -- 0x0004b8b8 and 0x0004b8e4 -- so the
// square of each is the wide value times the narrowed one, while x and y are
// squared from their slots. Both matter: a - b of two floats is exact in an
// 80-bit register only while their exponents are close, so two spheres whose
// centres or radii differ by many orders of magnitude are a case where the wide
// and the narrow product are different numbers.
//
// It stores the squared distance and the radius sum over the caller's second and
// first argument slots (0x0004b8d4, 0x0004b8e4), which is safe only because edi
// and esi already hold both shapes.
//
// There is no `double` alive across a branch anywhere in it -- every value that
// survives a comparison went through a 32-bit slot on the way -- so this row is
// not in the codegen-divergence class the four capsule rows are.
void __cdecl NxContactSphereSphere(const NxCollisionShape* sphere0,
	const NxCollisionShape* sphere1, NxContactSink* sink, void* context)
	{
	(void) context;

	// 0x0004b86c..0x0004b89b. A null `owner->[8]` on either side sends the pair
	// to the continuous-CD entry with the shape whose owner->[8] is NOT null
	// first. Shape 0 is tested first and shape 1 only if shape 0's is non-null,
	// so a pair with two nulls takes the first branch alone.
	if(!*(void* const*) ((const NxU8*) NxShapeOwner(sphere0, 0) + 8))
		NxContinuousCdPair(sphere1, sphere0, sink);
	else if(!*(void* const*) ((const NxU8*) NxShapeOwner(sphere1, 0) + 8))
		NxContinuousCdPair(sphere0, sphere1, sink);

	const NxReal* c0 = sphere0->translation;
	const NxReal* c1 = sphere1->translation;

	NxVec3 delta;
	delta.x = (NxReal) ((double) c1[0] - c0[0]);
	delta.y = (NxReal) ((double) c1[1] - c0[1]);
	const double wideZ = (double) c1[2] - c0[2];
	delta.z = (NxReal) wideZ;

	// z, then y, then x -- 0x0004b8bc..0x0004b8d2.
	const NxReal distanceSquared = (NxReal) ((wideZ * delta.z
		+ (double) delta.y * delta.y) + (double) delta.x * delta.x);

	const double wideRadius = (double) sphere1->geometry[0] + sphere0->geometry[0];
	const NxReal radiusSum = (NxReal) wideRadius;
	// Strict and ordered: `test ah,0x41; jne` at 0x0004b8f2 leaves on less, on
	// equal and on unordered, so two spheres exactly touching produce nothing
	// and a NaN anywhere in either centre or radius produces nothing. That
	// agrees with phys_fn_001931, the overlap test for the same pair.
	if(!(wideRadius * radiusSum > (double) distanceSquared))
		return;

	// 1e-5f at 0x00107a08, and `jnp` at 0x0004b90a leaves on less and on equal.
	// Two coincident centres have no direction to push along, and this is where
	// that is caught -- the divide below would otherwise be by zero. A NaN
	// cannot reach this test; the comparison above has already left.
	if(!((double) distanceSquared > (double) 1.0e-5f))
		return;

	const double distance = nxSqrt(distanceSquared);
	const double inverse = 1.0 / distance;
	delta.x = (NxReal) ((double) delta.x * inverse);
	delta.y = (NxReal) ((double) delta.y * inverse);
	delta.z = (NxReal) ((double) delta.z * inverse);

	// The contact point is on sphere0's surface, along the normal towards
	// sphere1 -- a fourth convention in this matrix, after the sphere, the plane
	// and the box corner. x keeps the wide product (0x0004b981 adds straight to
	// it) while y and z are read back from their slots.
	const double radius0 = sphere0->geometry[0];
	const double scaledX = (double) delta.x * radius0;
	const NxReal scaledY = (NxReal) ((double) delta.y * radius0);
	const NxReal scaledZ = (NxReal) ((double) delta.z * radius0);

	NxVec3 point;
	point.x = (NxReal) (scaledX + c0[0]);
	point.y = (NxReal) ((double) scaledY + c0[1]);
	point.z = (NxReal) ((double) scaledZ + c0[2]);

	// The wide root minus the narrowed radius sum, from the slot it was stored
	// in at 0x0004b8e4 and read back at 0x0004b9a0.
	const NxReal separation = (NxReal) (distance - radiusSum);

	NxEmitContact(sink, sphere1->collisionObject, sphere0->collisionObject,
		nxBits(separation), &point, &delta, 0xffff, 0xffff);
	}

// phys_fn_001917 at 0x00049f00.
//
// The first two thirds are phys_fn_001913 -- the same transform into box space
// with the wide z on the first row and the narrowed z on the other two, the same
// per-axis clamp, the same flag that is set on the x and y axes only. What is
// new is what happens after.
//
// OUTSIDE (0x0004a02b). The clamped point goes back out through the rotation's
// rows, the vector from it to the sphere centre becomes the normal, and its
// length becomes the separation once the radius is taken off. Two details are
// the oracle's: the third world component is left in a register while the first
// two are pushed through slots, so the normal's z is formed against a wider
// number than the point's z was (`fst` at 0x0004a08b, `fsub st(3)` at
// 0x0004a0a2); and the box centre is added back x-first, y-second and z-third
// with the operands the other way round on y alone (0x0004a10a..0x0004a11f).
//
// INSIDE (0x0004a131). The centre is inside the box, so the contact is taken
// from the shallowest of the three faces: the depths are `extent - |local|` per
// axis, the smallest wins with ties going to z, and the normal is that axis
// signed by which side of the centre the sphere is on. The point is then the
// SPHERE CENTRE copied dword for dword (0x0004a253..0x0004a266) rather than any
// point on the box, and the separation is `-(depth + radius)`.
//
// A NaN reaching the length test returns TRUE, because `test ah,0x41; jne` at
// 0x0004a0d7 continues on unordered. That is the same disagreement with the
// other kernels that phys_fn_001913 has and it is reached the same way.
bool __cdecl NxSphereBoxContactData(const NxCollisionSphereData* sphere,
	const NxCollisionBoxData* box, NxVec3* point, NxVec3* normal,
	NxReal* separation)
	{
	const NxReal* m = box->rotation;
	const NxReal* extents = box->extents;

	const NxReal dx = (NxReal) ((double) sphere->center[0] - box->center[0]);
	const NxReal dy = (NxReal) ((double) sphere->center[1] - box->center[1]);
	const double wideZ = (double) sphere->center[2] - box->center[2];
	const NxReal dz = (NxReal) wideZ;

	const NxReal local0 = (NxReal) ((wideZ * m[6] + (double) dy * m[3]) + (double) dx * m[0]);
	const NxReal local1 = (NxReal) (((double) dz * m[7] + (double) dy * m[4]) + (double) dx * m[1]);
	const NxReal local2 = (NxReal) (((double) dz * m[8] + (double) dy * m[5]) + (double) dx * m[2]);

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

	// The clamped z never reaches a 32-bit slot: it stays in st(0) from
	// 0x00049f88 to 0x0004a02b. Every value it can hold is a float either way,
	// so a spill of this one cannot truncate anything.
	double clamped2;
	bool inside = false;
	if(local2 < -extents[2])
		clamped2 = -extents[2];
	else if(local2 > extents[2])
		clamped2 = extents[2];
	else
		{
		clamped2 = local2;
		// The z axis does not set the flag. Control only reaches this test with
		// z inside its extent, so the flag answers "is the centre inside the
		// box" exactly -- and a NaN z with x and y unclamped answers yes.
		inside = !clamped;
		}

	if(!inside)
		{
		const NxReal world0 = (NxReal) ((clamped2 * m[2] + (double) clamped1 * m[1]) + (double) clamped0 * m[0]);
		const NxReal world1 = (NxReal) ((clamped2 * m[5] + (double) clamped1 * m[4]) + (double) clamped0 * m[3]);
		const double world2 = (clamped2 * m[8] + (double) clamped1 * m[7]) + (double) clamped0 * m[6];

		// y is stored before x -- 0x0004a080 against 0x0004a089 -- and z is the
		// `fst` at 0x0004a08b, so the wide value survives into the normal below.
		point->y = world1;
		point->x = world0;
		point->z = (NxReal) world2;

		const double e0 = (double) dx - world0;
		const double e1 = (double) dy - world1;
		const double e2 = (double) dz - world2;
		normal->x = (NxReal) e0;
		normal->y = (NxReal) e1;
		normal->z = (NxReal) e2;

		const double squared = (e2 * e2 + e1 * e1) + e0 * e0;
		const double radius = sphere->radius;
		if(squared > radius * radius)
			return false;

		const double distance = nxSqrt(squared);
		*separation = (NxReal) distance;
		const double inverse = 1.0 / distance;
		normal->x = (NxReal) (inverse * normal->x);
		normal->y = (NxReal) (inverse * normal->y);
		normal->z = (NxReal) (inverse * normal->z);

		point->x = (NxReal) ((double) box->center[0] + point->x);
		point->y = (NxReal) ((double) point->y + box->center[1]);
		point->z = (NxReal) ((double) box->center[2] + point->z);
		*separation = (NxReal) ((double) *separation - sphere->radius);
		return true;
		}

	// |x| and |y| stay in registers; |z| is narrowed into a slot at 0x0004a145
	// before the depth is taken, so the three depths are not formed alike. The
	// magnitude of a float is a float, so that asymmetry is transcribed rather
	// than measured.
	//
	// `fabs` rather than `x < 0 ? -x : x`, and the difference is not academic:
	// the oracle's `fabs` at 0x0004a137 clears the sign bit, so it turns -0.0
	// into +0.0 where the comparison spelling leaves it negative. With an extent
	// of -0.0 -- which the raw-bit half of the generator can produce -- the two
	// give `-0.0 - -0.0 = +0.0` and `-0.0 - 0.0 = -0.0`, and the sign of that
	// zero reaches the stream. No draw in 3,480,000 hit it; it was found by
	// reading, and is fixed rather than left for a different seed to find.
	const double absolute0 = fabs((double) clamped0);
	const double absolute1 = fabs((double) clamped1);
	const NxReal absolute2 = (NxReal) fabs((double) local2);

	const NxReal depth0 = (NxReal) ((double) extents[0] - absolute0);
	const NxReal depth1 = (NxReal) ((double) extents[1] - absolute1);
	const NxReal depth2 = (NxReal) ((double) extents[2] - (double) absolute2);

	// Two strict ordered comparisons per branch, so a NaN depth is never the
	// smallest and z wins every tie: 0x0004a174, 0x0004a18b and 0x0004a1d0.
	NxReal depth;
	NxVec3 local;
	local.x = 0.0f;
	local.y = 0.0f;
	local.z = 0.0f;
	if(depth1 < depth0)
		{
		if(depth1 < depth2)
			{
			depth = depth1;
			local.y = clamped1 > 0.0f ? 1.0f : -1.0f;
			}
		else
			{
			depth = depth2;
			local.z = local2 > 0.0f ? 1.0f : -1.0f;
			}
		}
	else if(depth0 < depth2)
		{
		depth = depth0;
		local.x = clamped0 > 0.0f ? 1.0f : -1.0f;
		}
	else
		{
		depth = depth2;
		local.z = local2 > 0.0f ? 1.0f : -1.0f;
		}

	*separation = (NxReal) (-(double) depth);
	point->x = sphere->center[0];
	point->y = sphere->center[1];
	point->z = sphere->center[2];

	const NxReal world0 = (NxReal) (((double) local.z * m[2] + (double) local.y * m[1]) + (double) local.x * m[0]);
	const NxReal world1 = (NxReal) (((double) local.z * m[5] + (double) local.y * m[4]) + (double) local.x * m[3]);
	const double world2 = ((double) local.z * m[8] + (double) local.y * m[7]) + (double) local.x * m[6];

	normal->y = world1;
	normal->x = world0;
	normal->z = (NxReal) world2;
	*separation = (NxReal) ((double) *separation - sphere->radius);
	return true;
	}

// phys_fn_001919 at 0x0004a2d0, matrix A slot [SPHERE][BOX]. 259 bytes, of which
// most is the two stack structures -- the same flattening phys_fn_001915 does
// for the overlap test, which is what fixes their field order.
void __cdecl NxContactSphereBox(const NxCollisionShape* sphere,
	const NxCollisionShape* box, NxContactSink* sink, void* context)
	{
	(void) context;

	// 0x0004a2db..0x0004a30e, the same shape as the sphere pair: the sphere is
	// tested first and the box only if the sphere's owner->[8] is non-null.
	if(!*(void* const*) ((const NxU8*) NxShapeOwner(sphere, 0) + 8))
		NxContinuousCdPair(box, sphere, sink);
	else if(!*(void* const*) ((const NxU8*) NxShapeOwner(box, 0) + 8))
		NxContinuousCdPair(sphere, box, sink);

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

	NxVec3 point;
	NxVec3 normal;
	NxReal separation;
	if(!NxSphereBoxContactData(&sphereData, &boxData, &point, &normal, &separation))
		return;

	// The sphere in the `object1` slot and the box in `object0`. See the header.
	NxEmitContact(sink, sphere->collisionObject, box->collisionObject,
		nxBits(separation), &point, &normal, 0xffff, 0xffff);
	}
