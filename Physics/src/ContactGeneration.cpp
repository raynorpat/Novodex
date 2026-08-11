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
// The oracle grows it through phys_fn_004840 at 0x000b4de0, a Phase 2 row that
// reaches the SDK allocator. Reproducing the growth policy is that row's
// business, not this one's, and the differential pre-sizes the stream so the
// call is never reached on either side. That is a stated limitation, not an
// oversight: the growth path of this emitter is unexercised, and a reader
// should not take a matching stream as evidence about it.
static void nxAppend(NxContactSink* sink, NxU32 word)
	{
	sink->stream[sink->streamCount++] = word;
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

		nxAppend(sink, nxBits(normal->x));
		nxAppend(sink, nxBits(normal->y));
		nxAppend(sink, nxBits(normal->z));

		sink->pointCountIndex = sink->streamCount;
		nxAppend(sink, 0);
		++sink->stream[sink->normalCountIndex];
		}

	++sink->contactCount;

	nxAppend(sink, nxBits(point->x));
	nxAppend(sink, nxBits(point->y));
	nxAppend(sink, nxBits(point->z));
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
