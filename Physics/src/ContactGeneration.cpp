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

#include "NxIntersectionRayPlane.h"
#include "NxPlane.h"

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

static NxU32 nxBits(NxReal value)
	{
	NxU32 word;
	memcpy(&word, &value, 4);
	return word;
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
		const NxU32 packed = sink->featurePairValid << 16;

		sink->lastObject1 = shape1->collisionObject;
		sink->lastObject0 = shape0->collisionObject;

		nxAppend(sink, (NxU32) (size_t) shape1->collisionObject);
		nxAppend(sink, (NxU32) (size_t) shape0->collisionObject);

		// The material comes from shape1's owner when `owner->[8]` is non-null
		// and from shape0's otherwise -- 0x0001d726..0x0001d73e. `+0x240` is a
		// borrowed Phase 5 offset.
		const NxU8* holder = *(const NxU8* const*) ((const NxU8*) shape1->owner + 8);
		if(!holder)
			holder = *(const NxU8* const*) ((const NxU8*) shape0->owner + 8);
		const NxU32 material = *(const NxU32*) (holder + 0x240);

		sink->normalCountIndex = sink->streamCount;
		nxAppend(sink, (material << 24) | packed);
		++sink->stream[sink->pairCountIndex];

		// The cached normal is cleared, so the block below always writes.
		sink->lastNormal[0] = 0.0f;
		sink->lastNormal[1] = 0.0f;
		sink->lastNormal[2] = 0.0f;
		}

	// The comparison is on the raw words, not on float equality: the oracle
	// compares with `cmp` at 0x0001d78c, 0x0001d798 and 0x0001d7a0, so two
	// normals that are equal as floats but differ in bits -- +0.0 against -0.0,
	// or two NaNs -- are a change, and two identical NaNs are not.
	if(nxBits(sink->lastNormal[0]) != nxBits(normal->x)
		|| nxBits(sink->lastNormal[1]) != nxBits(normal->y)
		|| nxBits(sink->lastNormal[2]) != nxBits(normal->z))
		{
		sink->lastNormal[0] = normal->x;
		sink->lastNormal[1] = normal->y;
		sink->lastNormal[2] = normal->z;

		nxAppend3(sink, nxBits(normal->x), nxBits(normal->y), nxBits(normal->z));

		sink->pointCountIndex = sink->streamCount;
		nxAppend(sink, 0);
		++sink->stream[sink->normalCountIndex];
		}

	++sink->contactCount;

	nxAppend3(sink, nxBits(point->x), nxBits(point->y), nxBits(point->z));
	// The sign bit is masked off, not negated -- `and ebx, 0x7fffffff` at
	// 0x0001d86d. For the negative separations a penetrating contact produces
	// the two are indistinguishable, which is why this reads as a negation.
	nxAppend(sink, separationBits & 0x7fffffffu);
	++sink->stream[sink->pointCountIndex];

	if(sink->featurePairValid & 1)
		nxAppend(sink, ((NxU32) featureId1 << 16) | (NxU32) featureId0);
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
		const NxReal length = (NxReal) sqrt(((double) dz * dz + (double) dy * dy)
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
