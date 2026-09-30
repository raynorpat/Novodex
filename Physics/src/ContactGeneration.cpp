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
#include "Containers.h"

// phys_fn_002266 reads NX_CONTINUOUS_CD out of the SDK's live parameter array
// through phys_fn_000429, so this unit reaches Phase 2's PhysicsSDK.
#include "PhysicsSDK.h"
#include "IcePrunable.h"

#include "NxIntersectionRayPlane.h"
#include "NxIntersectionRaySphere.h"
#include "NxIntersectionSegmentCapsule.h"
#include "NxPlane.h"
#include "NxSegment.h"
#include "NxBoxDistance.h"
#include "X87Sqrt.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static_assert(offsetof(NxContactSink, orientedTo) == 0x08, "sink orientedTo is at 0x08");
static_assert(offsetof(NxContactSink, contactCount) == 0x10, "sink contactCount is at 0x10");
static_assert(offsetof(NxContactSink, lastObject1) == 0x20, "sink lastObject1 is at 0x20");
static_assert(offsetof(NxContactSink, lastNormal) == 0x28, "sink lastNormal is at 0x28");
static_assert(offsetof(NxContactSink, featurePairValid) == 0x34, "sink featurePairValid is at 0x34");
static_assert(offsetof(NxContactSink, stream) == 0x40, "sink stream data is at 0x40");

extern "C" void nxContactCallContainerResize();		// 004840, Container::Resize(udword)
#pragma comment(linker, "/alternatename:_nxContactCallContainerResize=?Resize@Container@IceCore@@AAE_NI@Z")

// phys_fn_002354 (0x0005b620, 86 B)
void __fastcall NxContactSinkResetState(NxU32* state)
	{
	if(state[11] != 0)
		state[11] = 0;
	const NxU32 oldCount = state[11];
	state[0] = 0;
	if(state[11] == state[10])
		{
		NxU32* container = state + 10;
		__asm
			{
			push 1
			mov ecx, container
			call nxContactCallContainerResize
			}
		}
	NxU32* stream = reinterpret_cast<NxU32*>(static_cast<size_t>(state[12]));
	stream[state[11]] = 0;
	++state[11];
	state[1] = oldCount;
	state[2] = 0;
	state[3] = 0;
	state[4] = 0;
	state[5] = 0;
	state[6] = 0;
	state[7] = 0;
	state[8] = 0;
	state[9] = 0;
	}

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
static void nxReserve(NxContactSink* sink, NxU32 count)
	{
	const bool full = (count == 1)
		? (sink->streamCount == sink->streamCapacity)
		: (sink->streamCount + count > sink->streamCapacity);
	if(full)
		reinterpret_cast<SdkContainer*>(&sink->streamCapacity)->resize(count);
	}

static void nxAppend(NxContactSink* sink, NxU32 word)
	{
	nxReserve(sink, 1);
	sink->stream[sink->streamCount++] = word;
	}

static void nxAppend3(NxContactSink* sink, NxU32 a, NxU32 b, NxU32 c)
	{
	nxReserve(sink, 3);
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

void __fastcall NxEmitContactThiscall(NxContactSink* sink, NxU32,
	void* object1, void* object0, NxU32 separationBits,
	const NxVec3* point, const NxVec3* normal, NxU16 featureId0, NxU16 featureId1)
	{
	NxEmitContact(sink, object1, object0, separationBits, point, normal,
		featureId0, featureId1);
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

// phys_fn_001283 at 0x000257b0 reads the collision shape's type word at +0xd0.
NxU32 __fastcall NxShapeGetType(const NxCollisionShape* shape, void* edxUnused)
	{
	(void) edxUnused;
	return shape->type;
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

// One edge of the quad phys_fn_001739 interpolates over, projected onto the
// query point.
//
// A SEPARATE, NOINLINE FUNCTION, AND THAT IS THE WHOLE POINT OF IT. The oracle
// holds the three edge components and the two deltas in st(1)..st(6) from
// 0x00038b0d to 0x00038b39 and never stores one, and MSVC only leaves a
// `double` in a register while the live set fits the x87 stack. Four spellings
// were measured against the oracle over the same 1,200,000 checks, and all four
// agree on every one of them under 0x027f:
//
//   named `double` locals, inline           88 words differ under 0x0f7f
//   every use written out as a subexpression  20,060  (MSVC re-CSEs them and
//                                             spills the temps it invented)
//   this function, allowed to inline            68  (dy and dz spilled, since
//                                             each is used by both edges)
//   this function, noinline                      0
//
// Every one of those differences is an 8-byte spill slot truncating a 64-bit
// significand to 53. Nothing about the arithmetic changed between the four.
//
// That matters beyond this row: the `double`-liveness signal has been read in
// this program as "a value carried across a branch", and there is no branch
// anywhere in this region. The cause is the spill and a branch is only its
// commonest reason -- register pressure in straight-line code is another, and
// counting x87 depth at branches cannot see it. It also says a divergence in
// that class is not automatically something to pin: this one was removable.
//
// The two callers do NOT share a denominator order: quad[1]'s is
// (z*z + y*y) + x*x at 0x00038b29..0x00038b37 and quad[3]'s is
// (z*z + x*x) + y*y at 0x00038b63..0x00038b71, so `swapped` picks between them
// rather than the caller picking an argument order.
static __declspec(noinline) double nxQuadEdge(const NxVec3* origin, const NxVec3* along,
	NxReal pointY, NxReal pointZ, bool swapped)
	{
	const double ex = (double) along->x - origin->x;
	const double ey = (double) along->y - origin->y;
	const double ez = (double) along->z - origin->z;
	const double numerator = ex * (ez * ((double) pointZ - origin->z)
		+ ey * ((double) pointY - origin->y));
	if(swapped)
		return numerator / ((ez * ez + ex * ex) + ey * ey);
	return numerator / ((ez * ez + ey * ey) + ex * ex);
	}

// phys_fn_001739 at 0x00038a90. See the header for the ABI note.
//
// A containment test in the (y, z) plane followed by an interpolation of x, and
// the two halves do not use the same corners: the test walks all four edges and
// the interpolation reads quad[0], quad[1] and quad[3] only. quad[2] never
// reaches an arithmetic instruction.
//
// Its one branch above x87 depth 0 is the loop's back edge at 0x00038af0, which
// carries the previous corner's y and z -- both `fld dword` of a 32-bit slot the
// loop wrote with an integer move at 0x00038aa9 and 0x00038ab1 -- so nothing
// that survives a branch here is anything but a float. That is what said this
// row should be bit-exact, and it was not until the spill in the tail was taken
// out; see nxQuadEdge above for the measurement and for what it corrects.
double NxBoxBoxQuadDepth(const NxVec3* const* quad, NxReal pointY, NxReal pointZ)
	{
	// The previous corner starts at quad[3] (0x00038a90) and each iteration
	// leaves its own y and z for the next (0x00038ae4, 0x00038aec).
	double previousY = quad[3]->y;
	double previousZ = quad[3]->z;

	for(int index = 0; index < 4; ++index)
		{
		const NxVec3* const corner = quad[index];
		const NxReal cornerY = corner->y;
		const NxReal cornerZ = corner->z;

		const double first = ((double) cornerY - previousY) * ((double) pointZ - previousZ);
		const double second = ((double) cornerZ - previousZ) * ((double) pointY - previousY);
		// `test ah,1` at 0x00038ad9 reads C0 alone, which is set for "less" and
		// for "unordered" alike, so a corner exactly on an edge is outside and a
		// NaN cross product keeps walking. -1.0f is the constant at 0x1010687c.
		if(first - second >= 0.0)
			return -1.0f;

		previousY = cornerY;
		previousZ = cornerZ;
		}

	// 0x00038af4. The point is inside, so x comes from two independent edge
	// projections added to quad[0]'s own x.
	const NxVec3* const origin = quad[0];
	const NxVec3* const alongU = quad[1];
	const NxVec3* const alongV = quad[3];

	// The first quotient is added to quad[0].x and NARROWED through a 32-bit
	// slot at 0x00038b3d; the second is added to that narrowed copy and left
	// wide. So the two halves of the answer are not carried at the same width.
	const NxReal partial = (NxReal) (nxQuadEdge(origin, alongU, pointY, pointZ, false)
		+ origin->x);
	return nxQuadEdge(origin, alongV, pointY, pointZ, true) + (double) partial;
	}

// ---------------------------------------------------------------------------
// phys_fn_001741 at 0x00038ba0 and its continuation phys_fn_001743 at
// 0x00039730 -- the clipping/manifold row of matrix A [BOX][BOX], in five
// stages. See the header for the two register arguments.

// A corner of the incident box in the reference face's frame, with the two
// integer flags the oracle keeps beside it. The oracle's record is five
// consecutive dwords at [esp+0x54 + 20*i] -- x, y, z, `behind`, `emitted` --
// which is what the `+0xc` and `+0x10` reads in the edge loop (0x000393e9,
// 0x000393ff) and the face loop (0x00039745, 0x000397ad) index, and what the
// `lea ecx,[esp + ecx*4 + 0x54]` with `ecx = 5*index` at 0x000393e1 strides.
struct NxBoxBoxCorner
	{
	NxVec3 p;
	int behind;
	int emitted;
	};

// One of the four y/z clip cases of the edge loop -- 0x00039411, 0x000394b3,
// 0x00039545 and 0x000395e7, which are the same seven instructions with the
// axis and the sign changed. `alongY` picks which component is the clip
// parameter; the other one is interpolated and range-checked.
//
// TWO WIDE VALUES CROSS A BRANCH HERE, not one. `t` is the fdivp quotient at
// 0x00039456 and it is live across the |other| <= extent test at 0x00039475;
// the projected x is live across the x >= 0.0f test at 0x0003948a, and it is
// COMPARED while wide (`fcom [0x101041f0]`, no narrowing store before it).
// Neither is ever spilled by the oracle. That is the phys_fn_001690 shape and
// it is why this row was forecast into the codegen-divergence class.
//
// It is in a function of its own for the same reason nxQuadEdge is -- so that
// the quotient and the projection are the only live values while they are being
// formed -- and that turned out NOT to be where this row's divergence was: the
// change moved not one bit. See nxBoxBoxCornerA below for what it was.
static __declspec(noinline) double nxBoxBoxClipAt(NxReal limit, NxReal cAxis, NxReal sAxis,
	NxReal cComponent, NxReal sComponent)
	{
	const double t = ((double) limit - cAxis) / ((double) sAxis - cAxis);
	return ((double) sComponent - cComponent) * t + cComponent;
	}

// The fifth case's numerator is `fld [ecx]; fchs` at 0x00039695, NOT a subtract
// from zero: they differ on a +0.0f endpoint, where the negation gives -0.0 and
// `0 - x` gives +0.0, and on a NaN payload.
static __declspec(noinline) double nxBoxBoxPlaneAt(NxReal cx, NxReal sx,
	NxReal cComponent, NxReal sComponent)
	{
	const double t = (double) (-cx) / ((double) sx - cx);
	return ((double) sComponent - cComponent) * t + cComponent;
	}

// The stage 4 query and its rejection test in one function, because the oracle
// keeps the leaf's answer in st(0) from the `fcom 0.0f` at 0x00039938 to the
// two narrowing stores at 0x0003994c and 0x00039967 and never spills it.
static __declspec(noinline) bool nxBoxBoxFaceDepth(const NxVec3* const* quad,
	NxReal pointY, NxReal pointZ, NxReal* depth)
	{
	const double answer = NxBoxBoxQuadDepth(quad, pointY, pointZ);
	if(!(answer >= 0.0f))
		return false;
	*depth = (NxReal) answer;
	return true;
	}

// One of stage 1's nine dot products, in a function of its own so that the
// running sum is the only live value while it is being formed. The operands are
// passed in the order the oracle multiplies them and the two additions in the
// order it accumulates them; nothing here reorders anything.
static __declspec(noinline) NxReal nxBoxBoxDot(NxReal a0, NxReal b0,
	NxReal a1, NxReal b1, NxReal a2, NxReal b2)
	{
	return (NxReal) (((double) a0 * b0 + (double) a1 * b1) + (double) a2 * b2);
	}

// The reference-frame centre difference, one row at a time. Each call
// recomputes all three deltas rather than receiving them: they are three live
// `double`s the oracle keeps in st(0)..st(2) from 0x00038cca to 0x00038d19, and
// handing them across a call would only move the spill here.
static __declspec(noinline) double nxBoxBoxDelta(const NxReal* pose, const NxReal* q, int row)
	{
	const double dx = (double) q[9] - pose[9];
	const double dy = (double) q[10] - pose[10];
	const double dz = (double) q[11] - pose[11];
	if(row == 0)
		return (dy * pose[1] + dz * pose[2]) + dx * pose[0];
	if(row == 1)
		return (dx * pose[3] + dz * pose[5]) + dy * pose[4];
	return (dz * pose[8] + dy * pose[7]) + dx * pose[6];
	}

// The third row's projection combined with the first half axis's z. The oracle
// keeps that projection in st(0) from 0x00038d19 to 0x00038de2 and uses it for
// nothing but these two, so it is recomputed here rather than kept.
static __declspec(noinline) NxReal nxBoxBoxDelta2(const NxReal* pose, const NxReal* q,
	NxReal ax0z, bool add)
	{
	const double d2 = nxBoxBoxDelta(pose, q, 2);
	return (NxReal) (add ? (double) ax0z + d2 : d2 - ax0z);
	}

// THE CARRIER, FOUND BY READING THE GENERATED CODE RATHER THAN BY GUESSING.
//
// p0, w0, w1 and u0 -- the centre plus the first half axis, and the sum and the
// difference of the other two -- are the four values the oracle keeps in the
// x87 stack across the entire corner construction (0x00038dca, and
// 0x00038de6..0x00038f71). Written as ordinary `double` locals each is used by
// four corners, MSVC materialises all four into 8-byte slots, and the listing
// says so in one shape: `fadd qword ptr [X]; fstp qword ptr [X]`, at exactly
// four sites and nowhere else in this row. Every one truncates a 64-bit
// significand to 53 under 0x0f7f.
//
// So each corner recomputes what it needs from floats and nothing is live
// across a store. FOUR EARLIER ATTEMPTS MOVED EXACTLY NOTHING -- the clip
// quotient, the delta projection, the nine dot products and the stage 4 depth,
// each given its own noinline helper, left the candidate digest byte-identical
// at ad5c57fbd5dd9f5c. That is what said the carrier was none of them, and it
// is why the fifth attempt started from the listing instead.
//
// These are not gratuitous: narrowing the same four values to NxReal instead
// moves 411,952 words, so they really do have to be wide AND unspilled.
static __declspec(noinline) NxReal nxBoxBoxCornerA(NxReal base, NxReal ax1c, NxReal ax2c,
	bool useU, bool add)
	{
	const double v = useU ? (double) ax1c - ax2c : (double) ax2c + ax1c;
	return (NxReal) (add ? (double) base + v : (double) base - v);
	}

static __declspec(noinline) NxReal nxBoxBoxCornerP(NxReal m0, NxReal e0, NxReal d0,
	NxReal ax1x, NxReal ax2x, bool useU, bool add)
	{
	const double p0 = (double) m0 * e0 + d0;
	const double v = useU ? (double) ax1x - ax2x : (double) ax2x + ax1x;
	return (NxReal) (add ? p0 + v : p0 - v);
	}

// The first half axis's x is the only one of the nine the oracle leaves in a
// register (0x00038d24), and `a0` is the only place it reaches that is not
// already inside nxBoxBoxCornerP.
static __declspec(noinline) NxReal nxBoxBoxAxis0(NxReal d0, NxReal m0, NxReal e0)
	{
	return (NxReal) ((double) d0 - (double) m0 * e0);
	}

static bool nxBoxBoxClipCase(const NxVec3* c, const NxVec3* s, NxReal limit,
	NxReal otherLimit, bool alongY, NxVec3* point, NxReal* separation)
	{
	const NxReal cAxis = alongY ? c->y : c->z;
	const NxReal sAxis = alongY ? s->y : s->z;
	const NxReal cOther = alongY ? c->z : c->y;
	const NxReal sOther = alongY ? s->z : s->y;

	// 0x00039424 and 0x00039435. `test ah,5; jp` and `test ah,1; jne` both put
	// the unordered case on the reject side, so an edge with a NaN endpoint
	// never clips.
	if(!(cAxis < limit) || !(sAxis >= limit))
		return false;

	const double other = nxBoxBoxClipAt(limit, cAxis, sAxis, cOther, sOther);
	// 0x00039463: a narrowed copy is stored and the wide one stays in st(0).
	const NxReal otherStored = (NxReal) other;
	// 0x00039467: `fabs` on the WIDE value, then `test ah,0x41; jp`, which
	// rejects |other| > limit and the unordered case alike.
	if(!(fabs(other) <= otherLimit))
		return false;

	const double x = nxBoxBoxClipAt(limit, cAxis, sAxis, c->x, s->x);
	// 0x0003947f: compared against the 0.0f at 0x101041f0 while still wide, and
	// `test ah,1; jne` rejects a negative depth and a NaN.
	if(!(x >= 0.0f))
		return false;

	*separation = (NxReal) x;
	point->x = (NxReal) x;
	if(alongY)
		{
		point->y = limit;
		point->z = otherStored;
		}
	else
		{
		point->y = otherStored;
		point->z = limit;
		}
	return true;
	}

// The fifth case, at 0x00039695: where the edge crosses the reference face's
// own plane. Its quotient is -c.x / (s.x - c.x) and BOTH interpolated
// components are range-checked, so `t` crosses the first branch and the
// interpolated z crosses the second.
static bool nxBoxBoxClipPlane(const NxVec3* c, const NxVec3* s, NxReal extentY,
	NxReal extentZ, NxVec3* point, NxReal* separation)
	{
	const double y = nxBoxBoxPlaneAt(c->x, s->x, c->y, s->y);
	const NxReal yStored = (NxReal) y;
	if(!(fabs(y) <= extentY))
		return false;
	const double z = nxBoxBoxPlaneAt(c->x, s->x, c->z, s->z);
	// 0x000396c9 duplicates z with `fld st(0)` so the wide copy survives the
	// test and is what gets stored.
	if(!(fabs(z) <= extentZ))
		return false;

	// 0x000396df and 0x000396ef store integer zeros: this case's separation and
	// its point's x are +0.0f by construction, not by arithmetic.
	*separation = 0.0f;
	point->x = 0.0f;
	point->y = yStored;
	point->z = (NxReal) z;
	return true;
	}

// Stage 5, one point. The unrolled body at 0x000399b0 and the remainder loop at
// 0x00039b88 do NOT accumulate the same way, and neither do the four copies of
// the unrolled body, so `variant` picks between them: 0..3 are the four unrolled
// copies and 4 is the remainder. X is one association in all five; Y is one in
// the four unrolled copies and another in the remainder; Z has THREE.
//
// So the unrolling is observable, which is the opposite of what the decode
// recorded for this stage. What makes it observable is only the ORDER the three
// products are added in -- every product and every operand is the same.
static void nxBoxBoxToWorld(NxVec3* point, const NxReal* pose, int variant)
	{
	const NxReal px = point->x;
	const NxReal py = point->y;
	const NxReal pz = point->z;

	double wx, wy, wz;
	if(variant == 4)
		{
		wx = ((double) pz * pose[6] + (double) pose[0] * px) + (double) pose[3] * py;
		wy = ((double) pose[1] * px + (double) pose[4] * py) + (double) pz * pose[7];
		wz = ((double) px * pose[2] + (double) pose[5] * py) + (double) pz * pose[8];
		}
	else
		{
		wx = ((double) pose[0] * px + (double) pz * pose[6]) + (double) pose[3] * py;
		wy = ((double) pose[1] * px + (double) pose[7] * pz) + (double) pose[4] * py;
		if(variant == 1 || variant == 3)
			wz = ((double) pose[8] * pz + (double) pose[5] * py) + (double) pose[2] * px;
		else
			wz = ((double) pose[8] * pz + (double) px * pose[2]) + (double) pose[5] * py;
		}

	// 0x000399dc and 0x000399f6 narrow y and z into 32-bit slots and read them
	// back; x is never stored, so the centre is added to a wide x and to two
	// narrowed ones. 0x00039a0b then narrows z a second time before the integer
	// move at 0x00039a13.
	const NxReal yn = (NxReal) wy;
	const NxReal zn = (NxReal) wz;
	const NxReal outX = (NxReal) (wx + pose[9]);
	const NxReal outY = (NxReal) ((double) yn + pose[10]);
	const NxReal outZ = (NxReal) ((double) zn + pose[11]);
	point->x = outX;
	point->y = outY;
	point->z = outZ;
	}

// The twelve edges at 0x10107ad0 and the six faces at 0x10107a70, read out of
// .rdata. The edge loop walks two dwords per iteration and stops when its
// cursor reaches 0x10107b30 (0x000396fc); the face loop walks four and stops
// at 0x10107ad8 (0x0003997d), which is the same table read four at a time
// starting two dwords early.
static const int kNxBoxBoxEdges[12][2] =
	{
	{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
	{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },
	{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
	};

static const int kNxBoxBoxFaces[6][4] =
	{
	{ 0, 1, 3, 2 }, { 1, 5, 7, 3 }, { 5, 4, 6, 7 },
	{ 4, 0, 2, 6 }, { 2, 3, 7, 6 }, { 0, 4, 5, 1 }
	};

int NxBoxBoxClipFace(NxVec3* points, NxReal* separations, const NxReal* pose,
	NxReal extentY, NxReal extentZ, const NxReal* incidentPose,
	const NxReal* incidentExtents)
	{
	// -- Stage 1, 0x00038ba0..0x00038f93 -----------------------------------
	//
	// Nine dot products of the reference pose's rows against the incident
	// pose's rows, each accumulated in its own order and each narrowed by its
	// own `fstp dword` from 0x00038bc8 on. The oracle then `rep movsd`s all
	// nine into a second copy at [esp+0x130] (0x00038cc8) because stage 5 reads
	// them back after the first copy's slots have been reused; one array does
	// for both here.
	const NxReal* const q = incidentPose;
	NxReal m[9];
	m[0] = nxBoxBoxDot(pose[1], q[1], q[2], pose[2], pose[0], q[0]);
	m[1] = nxBoxBoxDot(q[2], pose[5], q[1], pose[4], pose[3], q[0]);
	m[2] = nxBoxBoxDot(pose[7], q[1], q[0], pose[6], pose[8], q[2]);
	m[3] = nxBoxBoxDot(pose[0], q[3], q[5], pose[2], pose[1], q[4]);
	m[4] = nxBoxBoxDot(pose[5], q[5], pose[4], q[4], pose[3], q[3]);
	m[5] = nxBoxBoxDot(q[3], pose[6], pose[8], q[5], pose[7], q[4]);
	m[6] = nxBoxBoxDot(pose[0], q[6], q[8], pose[2], pose[1], q[7]);
	m[7] = nxBoxBoxDot(pose[5], q[8], pose[4], q[7], pose[3], q[6]);
	m[8] = nxBoxBoxDot(q[6], pose[6], pose[8], q[8], pose[7], q[7]);

	// The centre difference, projected onto the reference pose's three rows.
	// 0x00038cca. The third component is the one the oracle never stores -- it
	// stays in st(0) through the whole corner construction below -- which is why
	// it has no local here and lives inside nxBoxBoxDelta2 instead.
	const NxReal d0 = (NxReal) nxBoxBoxDelta(pose, q, 0);
	const NxReal d1 = (NxReal) nxBoxBoxDelta(pose, q, 1);

	// The three scaled half axes. The FIRST component of the first one is the
	// only one the oracle leaves in a register (0x00038d24); the other eight go
	// through 32-bit slots at 0x00038d2f onward. That asymmetry is not an
	// accident of transcription -- it decides the width of half the corners,
	// which is why the x column below goes through nxBoxBoxCornerA/P and the
	// other two do not.
	const NxReal ax0y = (NxReal) ((double) m[1] * incidentExtents[0]);
	const NxReal ax0z = (NxReal) ((double) m[2] * incidentExtents[0]);
	const NxReal ax1x = (NxReal) ((double) m[3] * incidentExtents[1]);
	const NxReal ax1y = (NxReal) ((double) m[4] * incidentExtents[1]);
	const NxReal ax1z = (NxReal) ((double) m[5] * incidentExtents[1]);
	const NxReal ax2x = (NxReal) ((double) m[6] * incidentExtents[2]);
	const NxReal ax2y = (NxReal) ((double) m[7] * incidentExtents[2]);
	const NxReal ax2z = (NxReal) ((double) m[8] * incidentExtents[2]);

	// 0x00038daa. `a` is the centre minus the first half axis and `p` is the
	// centre plus it; `w` is the sum of the other two and `u` their difference,
	// so the eight corners are a/p combined with -w, +u, -u, +w.
	const NxReal e0 = incidentExtents[0];
	const NxReal a0 = nxBoxBoxAxis0(d0, m[0], e0);
	const NxReal a1 = (NxReal) ((double) d1 - ax0y);
	const NxReal a2 = nxBoxBoxDelta2(pose, q, ax0z, false);
	const NxReal p1 = (NxReal) ((double) ax0y + d1);
	const NxReal p2 = nxBoxBoxDelta2(pose, q, ax0z, true);
	const NxReal w2 = (NxReal) ((double) ax2z + ax1z);
	const NxReal u1 = (NxReal) ((double) ax1y - ax2y);
	const NxReal u2 = (NxReal) ((double) ax1z - ax2z);

	NxBoxBoxCorner corner[8];
	corner[0].p.x = nxBoxBoxCornerA(a0, ax1x, ax2x, false, false);
	corner[0].p.y = nxBoxBoxCornerA(a1, ax1y, ax2y, false, false);
	corner[0].p.z = (NxReal) ((double) a2 - w2);
	corner[1].p.x = nxBoxBoxCornerP(m[0], e0, d0, ax1x, ax2x, false, false);
	corner[1].p.y = nxBoxBoxCornerA(p1, ax1y, ax2y, false, false);
	corner[1].p.z = (NxReal) ((double) p2 - w2);
	corner[6].p.x = nxBoxBoxCornerA(a0, ax1x, ax2x, false, true);
	corner[6].p.y = nxBoxBoxCornerA(a1, ax1y, ax2y, false, true);
	corner[6].p.z = (NxReal) ((double) a2 + w2);
	corner[7].p.x = nxBoxBoxCornerP(m[0], e0, d0, ax1x, ax2x, false, true);
	corner[7].p.y = nxBoxBoxCornerA(p1, ax1y, ax2y, false, true);
	corner[7].p.z = (NxReal) ((double) p2 + w2);
	corner[2].p.x = nxBoxBoxCornerA(a0, ax1x, ax2x, true, true);
	corner[2].p.y = (NxReal) ((double) u1 + a1);
	corner[2].p.z = (NxReal) ((double) u2 + a2);
	corner[3].p.x = nxBoxBoxCornerP(m[0], e0, d0, ax1x, ax2x, true, true);
	corner[3].p.y = (NxReal) ((double) u1 + p1);
	corner[3].p.z = (NxReal) ((double) u2 + p2);
	corner[4].p.x = nxBoxBoxCornerA(a0, ax1x, ax2x, true, false);
	corner[4].p.y = (NxReal) ((double) a1 - u1);
	corner[4].p.z = (NxReal) ((double) a2 - u2);
	corner[5].p.x = nxBoxBoxCornerP(m[0], e0, d0, ax1x, ax2x, true, false);
	corner[5].p.y = (NxReal) ((double) p1 - u1);
	corner[5].p.z = (NxReal) ((double) p2 - u2);

	// -- Stage 2, 0x00038f9a..0x000393b7 -----------------------------------
	//
	// Eight identical blocks, unrolled. THIS IS THE KERNEL THAT REJECTS A NaN,
	// and it is the opposite of every other kernel in this component and of the
	// box/box leaf: all three tests are on integer words.
	int count = 0;
	for(int i = 0; i < 8; ++i)
		{
		// `test eax,eax; jns` at 0x00038f3a/0x00038f9a is an integer sign test
		// on the depth's bit pattern, so a NaN with a clear sign bit is behind
		// the face and -0.0f is not.
		if((nxBits(corner[i].p.x) & 0x80000000u) != 0)
			{
			corner[i].behind = 0;
			corner[i].emitted = 0;
			continue;
			}
		corner[i].behind = 1;

		// `and eax,0x7fffffff; cmp eax,esi; ja` at 0x00038fac and 0x00038fbd:
		// an unsigned compare of the masked coordinate against the extent's
		// UNMASKED word, which is an ordering test on the magnitude only
		// because an IEEE float's bit pattern is monotonic for non-negative
		// values. Two consequences: a NaN coordinate is a very large magnitude
		// and is rejected where every fcom in this component lets the unordered
		// case through, and -0.0f compares equal to +0.0f. A third follows from
		// the extent side not being masked: a negative extent admits
		// everything.
		if((nxBits(corner[i].p.y) & 0x7fffffffu) > nxBits(extentY)
			|| (nxBits(corner[i].p.z) & 0x7fffffffu) > nxBits(extentZ))
			{
			corner[i].emitted = 0;
			continue;
			}
		corner[i].emitted = 1;

		// 0x00038fc6: one load of the corner's x serves as both the separation
		// and the point's x, and all three stores are integer moves.
		separations[count] = corner[i].p.x;
		points[count] = corner[i].p;
		++count;
		}

	// -- Stage 3, 0x000393c5..0x000396fc -----------------------------------
	//
	// The edge clip: twelve edges, five cases each.
	for(int e = 0; e < 12; ++e)
		{
		const NxBoxBoxCorner* c = &corner[kNxBoxBoxEdges[e][0]];
		const NxBoxBoxCorner* s = &corner[kNxBoxBoxEdges[e][1]];

		// 0x000393e9: an edge with neither endpoint behind the reference face
		// cannot cross it.
		if(!c->behind && !s->behind)
			continue;

		// 0x000393ff: an edge whose two endpoints were BOTH emitted as corners
		// has nothing left to clip, and drops straight to the fifth case --
		// which then rejects it too, because that case needs one endpoint
		// behind and unemitted.
		if(!(c->emitted && s->emitted))
			{
			// 0x00039417: order the endpoints by y, then clip against +extentY
			// and -extentY. The swap is by value, and it persists into the z
			// pair below and into the fifth case.
			if(c->p.y > s->p.y)
				{
				const NxBoxBoxCorner* swap = c;
				c = s;
				s = swap;
				}
			if(nxBoxBoxClipCase(&c->p, &s->p, extentY, extentZ, true,
					&points[count], &separations[count]))
				++count;
			if(nxBoxBoxClipCase(&c->p, &s->p, (NxReal) -extentY, extentZ, true,
					&points[count], &separations[count]))
				++count;

			// 0x0003954b: the same again ordered by z.
			if(c->p.z > s->p.z)
				{
				const NxBoxBoxCorner* swap = c;
				c = s;
				s = swap;
				}
			if(nxBoxBoxClipCase(&c->p, &s->p, extentZ, extentY, false,
					&points[count], &separations[count]))
				++count;
			if(nxBoxBoxClipCase(&c->p, &s->p, (NxReal) -extentZ, extentY, false,
					&points[count], &separations[count]))
				++count;
			}

		// 0x00039679, transcribed as the four tests it is rather than as the
		// predicate they reduce to: exactly one endpoint behind the face, and
		// that endpoint not already emitted.
		bool crossesPlane;
		if(!c->behind && !s->emitted)
			crossesPlane = true;
		else
			crossesPlane = !s->behind && !c->emitted;
		if(crossesPlane && nxBoxBoxClipPlane(&c->p, &s->p, extentY, extentZ,
				&points[count], &separations[count]))
			++count;
		}

	// -- Stage 4, 0x00039730..0x0003998c, the phys_fn_001743 continuation ---
	//
	// The reference face's own four vertices, queried against each face of the
	// incident box in turn. `mask` is the oracle's [esp+0x14]: one bit per
	// reference-face vertex, and the loop stops as soon as all four are found
	// (`cmp [esp+0x14],0xf; je` at 0x00039730, tested before the first face).
	int mask = 0;
	for(int f = 0; f < 6; ++f)
		{
		if(mask == 0xf)
			break;

		const NxBoxBoxCorner* const v0 = &corner[kNxBoxBoxFaces[f][0]];
		const NxBoxBoxCorner* const v1 = &corner[kNxBoxBoxFaces[f][1]];
		const NxBoxBoxCorner* const v2 = &corner[kNxBoxBoxFaces[f][2]];
		const NxBoxBoxCorner* const v3 = &corner[kNxBoxBoxFaces[f][3]];

		// 0x00039749 onward: every one of the four has to be behind the
		// reference face, and they must not all four already have been emitted.
		if(!v0->behind || !v1->behind || !v2->behind || !v3->behind)
			continue;
		if(v0->emitted && v1->emitted && v2->emitted && v3->emitted)
			continue;

		const NxVec3* quad[4];
		quad[0] = &v0->p;
		quad[1] = &v1->p;
		quad[2] = &v2->p;
		quad[3] = &v3->p;

		// The four calls at 0x000397fb, 0x0003986f, 0x000398d6 and 0x00039933,
		// in that order and with those signs. Each result is rejected by
		// `fcom 0.0f; test ah,1; jne`, so the leaf's -1.0f for a point outside
		// the quad contributes nothing -- and so does a NaN.
		if(!(mask & 1))
			{
			NxReal depth;
			if(nxBoxBoxFaceDepth(quad, (NxReal) -extentY, (NxReal) -extentZ, &depth))
				{
				mask |= 1;
				separations[count] = depth;
				points[count].x = depth;
				points[count].y = (NxReal) -extentY;
				points[count].z = (NxReal) -extentZ;
				++count;
				}
			}
		if(!(mask & 2))
			{
			NxReal depth;
			if(nxBoxBoxFaceDepth(quad, extentY, (NxReal) -extentZ, &depth))
				{
				mask |= 2;
				separations[count] = depth;
				points[count].x = depth;
				points[count].y = extentY;
				points[count].z = (NxReal) -extentZ;
				++count;
				}
			}
		if(!(mask & 4))
			{
			NxReal depth;
			if(nxBoxBoxFaceDepth(quad, (NxReal) -extentY, extentZ, &depth))
				{
				mask |= 4;
				separations[count] = depth;
				points[count].x = depth;
				points[count].y = (NxReal) -extentY;
				points[count].z = extentZ;
				++count;
				}
			}
		if(!(mask & 8))
			{
			NxReal depth;
			if(nxBoxBoxFaceDepth(quad, extentY, extentZ, &depth))
				{
				mask |= 8;
				separations[count] = depth;
				points[count].x = depth;
				points[count].y = extentY;
				points[count].z = extentZ;
				++count;
				}
			}
		}

	// -- Stage 5, 0x0003998c..0x00039c07 -----------------------------------
	//
	// Back to world space through the reference pose transposed, four points at
	// a time and then a remainder loop. `blocks` is the oracle's
	// `lea ecx,[ebx-4]; shr ecx,2; inc ecx` at 0x0003999e.
	int index = 0;
	if(count >= 4)
		{
		const int blocks = ((count - 4) >> 2) + 1;
		for(int b = 0; b < blocks; ++b)
			for(int k = 0; k < 4; ++k)
				nxBoxBoxToWorld(&points[b * 4 + k], pose, k);
		index = blocks * 4;
		}
	for(; index < count; ++index)
		nxBoxBoxToWorld(&points[index], pose, 4);

	// `mov eax,ebx` at 0x00039bfe.
	return count;
	}

// ---------------------------------------------------------------------------
// phys_fn_001745 at 0x00039c10 -- the fifteen-axis separating-axis search of
// matrix A [BOX][BOX], its 24-byte switch table and its warm-start cache. See
// the header for the three register arguments and for what sink+0xe8 is.

// The three memory constants this row reads, each with the address it reads it
// from and the word that is actually there. The fudge is added to every
// |R[i][j]| BEFORE any radius sum, which is what keeps two exactly parallel
// boxes from failing an edge test on a zero-length cross product.
//
//   0x10106880  0x358637bd  1e-6f       the fudge, 0x00039cbf and eight more
//   0x10106858  0x7f7fffff  FLT_MAX     the minimum search's seed, 0x0003a303
//   0x10107b30  0x3f7fbe77  0.999f      the warm-start bias, 0x0003a2fb
static const NxReal kNxBoxAxisFudge = 1e-6f;
static const NxReal kNxBoxAxisSeed = 3.402823466e+38f;
static const NxReal kNxBoxAxisBias = 0.999f;

// One of the six face-axis overlaps: `radiusSum - |projection|`. The whole
// chain is formed in the x87 stack -- the three products, the two adds, the
// box's own extent and the subtraction -- and narrowed once by the `fstp dword`
// that stores it (0x00039d14 and its five siblings), so the sum is never
// rounded to 32 bits on the way.
//
// `fabs`, not `x < 0 ? -x : x`: 0x00039d07 is an `fabs`, which clears the sign
// bit, and the two spellings differ on a -0.0f projection. That is the same
// correction the sphere/box row needed.
static NxReal nxBoxAxisOverlap(NxReal f0, NxReal e0, NxReal f1, NxReal e1,
	NxReal f2, NxReal e2, NxReal own, NxReal projection)
	{
	const double sum = (((double) f0 * e0 + (double) f1 * e1) + (double) f2 * e2) + own;
	return (NxReal) (sum - fabs((double) projection));
	}

// One of the nine edge-axis radius sums. Four terms, left-associated, and the
// four are NOT in the same order at every site: seven of the nine take both of
// box A's terms first and edges 7 and 8 interleave them with box B's
// (0x0003a262 and 0x0003a2b0). Only rounding can tell, which is why they are
// passed positionally rather than by axis.
static NxReal nxBoxAxisRadius(NxReal f0, NxReal e0, NxReal f1, NxReal e1,
	NxReal f2, NxReal e2, NxReal f3, NxReal e3)
	{
	return (NxReal) ((((double) f0 * e0 + (double) f1 * e1) + (double) f2 * e2)
		+ (double) f3 * e3);
	}

// An edge axis's projection of the centre difference: two products and a
// subtract, narrowed by `fstp dword ptr [esp+0x1c]`.
static NxReal nxBoxAxisCross(NxReal a0, NxReal b0, NxReal a1, NxReal b1)
	{
	return (NxReal) ((double) a0 * b0 - (double) a1 * b1);
	}

// The edge test itself, and it is not a float compare. 0x0003a06a is
// `cmp dword ptr [esp+0x18], ecx; jb` on the two stored WORDS, with only the
// projection's word masked (`and ecx,0x7fffffff` at 0x0003a045). Three
// consequences, all of them reproduced rather than smoothed over: an IEEE
// float's bit pattern orders correctly for non-negative values, so this is a
// magnitude compare while both sides are non-negative; a NaN projection masks
// to a very large magnitude and SEPARATES the pair; and a negative radius sum
// has its sign bit set, reads as a huge unsigned word and separates nothing.
static bool nxBoxAxisSeparated(NxReal radius, NxReal projection)
	{
	return nxBits(radius) < (nxBits(projection) & 0x7fffffffu);
	}

int NxBoxBoxSeparatingAxis(NxVec3* points, NxReal* separations,
	const NxReal* extentsA, const NxReal* poseB, unsigned char* cache,
	const NxReal* poseA, NxVec3* normal, const NxReal* extentsB)
	{
	const NxReal* const a = poseA;
	const NxReal* const b = poseB;

	// The centre difference in world space, 0x00039c1e. Narrowed per component.
	const NxReal dx = (NxReal) ((double) b[9] - a[9]);
	const NxReal dy = (NxReal) ((double) b[10] - a[10]);
	const NxReal dz = (NxReal) ((double) b[11] - a[11]);

	// R, nine dot products in nine different accumulation orders, each narrowed
	// by its own `fstp dword` from 0x00039c5c on. nxBoxBoxDot takes the operands
	// in the order the oracle multiplies them and adds them in the order it
	// accumulates them; nothing here reorders anything.
	//
	// The projections of the centre difference onto A's three rows and B's
	// three rows have their own orders too, and the last is the odd one -- x, z,
	// y where the other five are z/y-first (0x00039faf).
	//
	// Rows are interleaved with their tests the way the oracle interleaves them:
	// each face axis is tested before the next row of R is formed, so a
	// separated pair never computes the rest.
	NxReal m[9];
	NxReal f[9];
	NxReal overlap[6];
	NxReal projection[6];

	m[0] = nxBoxBoxDot(a[0], b[0], b[2], a[2], a[1], b[1]);
	m[1] = nxBoxBoxDot(b[4], a[1], b[5], a[2], b[3], a[0]);
	m[2] = nxBoxBoxDot(b[7], a[1], b[8], a[2], b[6], a[0]);
	projection[0] = nxBoxBoxDot(dy, a[1], dx, a[0], dz, a[2]);
	f[0] = (NxReal) (fabs((double) m[0]) + kNxBoxAxisFudge);
	f[1] = (NxReal) (fabs((double) m[1]) + kNxBoxAxisFudge);
	f[2] = (NxReal) (fabs((double) m[2]) + kNxBoxAxisFudge);

	// The face-axis test is an integer sign test on the STORED overlap --
	// `mov eax,0x80000000; test eax,ecx; jne 0x0003acb3` at 0x00039d22 -- so a
	// NaN overlap with a clear sign bit passes it, and -0.0f does not.
	overlap[0] = nxBoxAxisOverlap(f[0], extentsB[0], f[2], extentsB[2],
		f[1], extentsB[1], extentsA[0], projection[0]);
	if((nxBits(overlap[0]) & 0x80000000u) != 0)
		return 0;

	m[3] = nxBoxBoxDot(b[1], a[4], b[2], a[5], b[0], a[3]);
	m[4] = nxBoxBoxDot(b[3], a[3], b[5], a[5], b[4], a[4]);
	m[5] = nxBoxBoxDot(b[6], a[3], b[8], a[5], b[7], a[4]);
	projection[1] = nxBoxBoxDot(dz, a[5], dy, a[4], dx, a[3]);
	f[3] = (NxReal) (fabs((double) m[3]) + kNxBoxAxisFudge);
	f[4] = (NxReal) (fabs((double) m[4]) + kNxBoxAxisFudge);
	f[5] = (NxReal) (fabs((double) m[5]) + kNxBoxAxisFudge);
	overlap[1] = nxBoxAxisOverlap(f[3], extentsB[0], f[5], extentsB[2],
		f[4], extentsB[1], extentsA[1], projection[1]);
	if((nxBits(overlap[1]) & 0x80000000u) != 0)
		return 0;

	m[6] = nxBoxBoxDot(b[2], a[8], a[6], b[0], a[7], b[1]);
	m[7] = nxBoxBoxDot(b[3], a[6], b[5], a[8], b[4], a[7]);
	m[8] = nxBoxBoxDot(b[6], a[6], b[8], a[8], b[7], a[7]);
	projection[2] = nxBoxBoxDot(dz, a[8], dy, a[7], dx, a[6]);
	f[6] = (NxReal) (fabs((double) m[6]) + kNxBoxAxisFudge);
	f[7] = (NxReal) (fabs((double) m[7]) + kNxBoxAxisFudge);
	f[8] = (NxReal) (fabs((double) m[8]) + kNxBoxAxisFudge);
	overlap[2] = nxBoxAxisOverlap(f[6], extentsB[0], f[8], extentsB[2],
		f[7], extentsB[1], extentsA[2], projection[2]);
	if((nxBits(overlap[2]) & 0x80000000u) != 0)
		return 0;

	projection[3] = nxBoxBoxDot(dz, b[2], dy, b[1], dx, b[0]);
	overlap[3] = nxBoxAxisOverlap(f[6], extentsA[2], f[0], extentsA[0],
		f[3], extentsA[1], extentsB[0], projection[3]);
	if((nxBits(overlap[3]) & 0x80000000u) != 0)
		return 0;

	projection[4] = nxBoxBoxDot(dz, b[5], dy, b[4], dx, b[3]);
	overlap[4] = nxBoxAxisOverlap(f[7], extentsA[2], f[1], extentsA[0],
		f[4], extentsA[1], extentsB[1], projection[4]);
	if((nxBits(overlap[4]) & 0x80000000u) != 0)
		return 0;

	projection[5] = nxBoxBoxDot(dx, b[6], dz, b[8], dy, b[7]);
	overlap[5] = nxBoxAxisOverlap(f[8], extentsA[2], f[2], extentsA[0],
		f[5], extentsA[1], extentsB[2], projection[5]);
	if((nxBits(overlap[5]) & 0x80000000u) != 0)
		return 0;

	// -- The warm start, read at 0x0003a016 --------------------------------
	//
	// AND IT DECIDES TWO THINGS. `test al,al; je` then `cmp al,0xff; jne` sends
	// 0 and 0xff into the nine edge-axis tests and everything else STRAIGHT
	// PAST THEM to the bias at 0x0003a2e8. So a pair whose only separating axis
	// is an edge cross product is reported as overlapping whenever the sink
	// happens to be carrying an index from an earlier pair.
	const unsigned char cached = *cache;
	const bool warm = cached != 0 && cached != 0xff;

	if(!warm)
		{
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[3], extentsA[2], f[6], extentsA[1],
				f[2], extentsB[1], f[1], extentsB[2]),
			nxBoxAxisCross(projection[2], m[3], m[6], projection[1])))
			return 0;
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[4], extentsA[2], f[7], extentsA[1],
				f[2], extentsB[0], f[0], extentsB[2]),
			nxBoxAxisCross(projection[2], m[4], m[7], projection[1])))
			return 0;
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[5], extentsA[2], f[8], extentsA[1],
				f[1], extentsB[0], f[0], extentsB[1]),
			nxBoxAxisCross(projection[2], m[5], m[8], projection[1])))
			return 0;
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[6], extentsA[0], f[0], extentsA[2],
				f[5], extentsB[1], f[4], extentsB[2]),
			nxBoxAxisCross(m[6], projection[0], projection[2], m[0])))
			return 0;
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[7], extentsA[0], f[1], extentsA[2],
				f[5], extentsB[0], f[3], extentsB[2]),
			nxBoxAxisCross(m[7], projection[0], projection[2], m[1])))
			return 0;
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[8], extentsA[0], f[2], extentsA[2],
				f[4], extentsB[0], f[3], extentsB[1]),
			nxBoxAxisCross(m[8], projection[0], projection[2], m[2])))
			return 0;
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[3], extentsA[0], f[0], extentsA[1],
				f[8], extentsB[1], f[7], extentsB[2]),
			nxBoxAxisCross(projection[1], m[0], m[3], projection[0])))
			return 0;
		// Edges 7 and 8 accumulate their four terms in a different order from
		// the seven above: A, B, A, B rather than A, A, B, B.
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[4], extentsA[0], f[8], extentsB[0],
				f[1], extentsA[1], f[6], extentsB[2]),
			nxBoxAxisCross(projection[1], m[1], m[4], projection[0])))
			return 0;
		if(nxBoxAxisSeparated(nxBoxAxisRadius(f[5], extentsA[0], f[7], extentsB[0],
				f[2], extentsA[1], f[6], extentsB[1]),
			nxBoxAxisCross(projection[1], m[2], m[5], projection[0])))
			return 0;
		}

	// 0x0003a2e8. `cmp al,0x10; ja` then `test al,al; jbe` admits 1..16 where
	// only six overlaps exist, and `lea eax,[esp + eax*4 + 0x7c]` walks straight
	// past them into the stored projections and, at 16, into the argument area.
	// Nothing can write such a value: this row writes `code + 1`, which is 1..6
	// (0x0003a456), the sink's constructor writes 0xff (0x0001efc8) and
	// phys_fn_001749 writes 0 (0x0003ae5d). So 7..16 is unreachable and is not
	// reproduced.
	if(warm && cached <= 6)
		overlap[cached - 1] = (NxReal) ((double) overlap[cached - 1] * kNxBoxAxisBias);

	// The minimum search, 0x0003a303: six copies of one comparison, seeded from
	// FLT_MAX. `fcomp 0.0f; test ah,1; jne` skips an overlap that is negative
	// OR UNORDERED, which is the only filter a NaN meets here -- every negative
	// one has already returned above -- and `test ah,5; jp` takes the strictly
	// smaller. If all six are NaNs the code stays 0 and the seed is never
	// replaced.
	NxReal best = kNxBoxAxisSeed;
	int code = 0;
	for(int k = 0; k < 6; ++k)
		if(overlap[k] >= 0.0f && overlap[k] < best)
			{
			best = overlap[k];
			code = k;
			}

	// `inc cl; mov byte ptr [eax],cl` at 0x0003a456. Written on every path that
	// reaches the dispatch and on none that returns 0, so a separated pair
	// leaves the previous pair's index in place.
	*cache = (unsigned char) (code + 1);

	// `mov eax,[esp + ecx*4 + 0x98]; and eax,0x80000000` at 0x0003a45e: the
	// STORED projection's word, not a recomputed one.
	const NxU32 sign = nxBits(projection[code]) & 0x80000000u;

	// -- The dispatch, 0x0003a473 ------------------------------------------
	//
	// `jmp dword ptr [ecx*4 + 0x1003acc0]` walks phys_data_000006, whose six
	// slots are 0x0003a47a, 0x0003a5ca, 0x0003a734, 0x0003a89d, 0x0003a9f4 and
	// 0x0003ab42. Arms 0..2 make box A the reference and leave edx holding box
	// B's extents; arms 3..5 make box B the reference and reload edx with box
	// A's, at two addresses for THREE arms -- 0x0003a9de is arm 3's own tail
	// and 0x0003ac9d is the tail arms 4 and 5 share.
	const bool referenceIsA = code < 3;
	const int i = code % 3;
	const NxReal* const refPose = referenceIsA ? poseA : poseB;
	const NxReal* const refExtents = referenceIsA ? extentsA : extentsB;
	const NxReal* const r0 = refPose + 3 * i;
	const NxReal* const r1 = refPose + 3 * ((i + 1) % 3);
	const NxReal* const r2 = refPose + 3 * ((i + 2) % 3);
	const NxReal e0 = refExtents[i];

	// The normal is the winning row copied as three raw words (0x0003a47c) and
	// then negated in place with three `fld; fchs; fstp` when the projection's
	// sign bit is CLEAR (0x0003a4e8 and its five siblings). The copy preserves a
	// signalling NaN and the negation quiets one, so which path ran is visible
	// in the payload as well as in the sign.
	memcpy(&normal->x, &r0[0], 4);
	memcpy(&normal->y, &r0[1], 4);
	memcpy(&normal->z, &r0[2], 4);
	if(sign == 0)
		{
		normal->x = -normal->x;
		normal->y = -normal->y;
		normal->z = -normal->z;
		}

	// WHICH FACE CONSTRUCTION RUNS IS NOT THE SAME TEST ON BOTH HALVES OF THE
	// TABLE. Arms 0..2 take the `centre + e*row` form when the sign bit is
	// clear (`je 0x0003a4e8`) and arms 3..5 take it when the sign bit is SET
	// (`je 0x0003a955` goes to the other one). Every projection is measured
	// along box A's frame, so this is what keeps the reference face the one
	// facing the other box on both halves of the table.
	const bool plus = referenceIsA ? (sign == 0) : (sign != 0);

	// The pose handed to the clip row: the winning row first, then the other
	// two cyclically. Only the first two rows are negated -- the third is a raw
	// copy on both paths (0x0003a55e, 0x0003a591) -- which leaves the
	// determinant's sign alone.
	NxReal face[12];
	for(int k = 0; k < 3; ++k)
		{
		if(plus)
			{
			face[k] = -r0[k];
			face[3 + k] = -r1[k];
			}
		else
			{
			memcpy(&face[k], &r0[k], 4);
			memcpy(&face[3 + k], &r1[k], 4);
			}
		memcpy(&face[6 + k], &r2[k], 4);
		}

	// The face centre, 0x0003a48e and 0x0003a4fe. THE THREE COMPONENTS ARE NOT
	// CARRIED AT THE SAME WIDTH: the x and y terms go through 32-bit slots
	// (`fstp dword ptr [esp+0xc]` and `[esp+0x10]`) and the z term does not --
	// `fmul [ebx+8]` leaves it in st(0) and `fsub st(3)` or `fadd [ebx+0x2c]`
	// consumes it wide.
	const NxReal t0 = (NxReal) ((double) e0 * r0[0]);
	const NxReal t1 = (NxReal) ((double) e0 * r0[1]);
	if(plus)
		{
		face[9] = (NxReal) ((double) t0 + refPose[9]);
		face[10] = (NxReal) ((double) t1 + refPose[10]);
		face[11] = (NxReal) ((double) e0 * r0[2] + refPose[11]);
		}
	else
		{
		face[9] = (NxReal) ((double) refPose[9] - t0);
		face[10] = (NxReal) ((double) refPose[10] - t1);
		face[11] = (NxReal) ((double) refPose[11] - (double) e0 * r0[2]);
		}

	// The two half extents the clip row range-checks against are the reference
	// box's OTHER two, in the same cyclic order as the rows.
	return NxBoxBoxClipFace(points, separations, face,
		refExtents[(i + 1) % 3], refExtents[(i + 2) % 3],
		referenceIsA ? poseB : poseA, referenceIsA ? extentsB : extentsA);
	}

// ---------------------------------------------------------------------------
// phys_fn_001748 at 0x0003ace0 -- the transpose-and-copy shim.

// The 3x3 transposed and the translation straight, twenty-four integer moves
// and nothing else. `out[i*3+j] = in[j*3+i]` is what
// `[eax+0] -> [esp+0x34]`, `[eax+0xc] -> [esp+0x38]`, `[eax+0x18] -> [esp+0x3c]`
// says: the copy's first ROW is the source's first COLUMN.
static void nxBoxBoxTranspose(NxReal* out, const NxReal* in)
	{
	for(int i = 0; i < 3; ++i)
		for(int j = 0; j < 3; ++j)
			memcpy(&out[i * 3 + j], &in[j * 3 + i], 4);
	for(int k = 9; k < 12; ++k)
		memcpy(&out[k], &in[k], 4);
	}

int NxBoxBoxTransposedPair(NxVec3* points, NxReal* separations, NxVec3* normal,
	const NxReal* extentsA, const NxReal* poseA,
	const NxReal* extentsB, const NxReal* poseB, unsigned char* cache)
	{
	// Two 12-dword frames. The oracle puts box A's at [esp+0x34] and box B's at
	// [esp+0x04] and passes the first in ebx and the second as its callee's
	// third stack argument (`lea ebx,[esp+0x48]` at 0x0003adbf and
	// `lea ecx,[esp+8]` at 0x0003ada5).
	NxReal transposedA[12];
	NxReal transposedB[12];
	nxBoxBoxTranspose(transposedA, poseA);
	nxBoxBoxTranspose(transposedB, poseB);

	return NxBoxBoxSeparatingAxis(points, separations, extentsA, transposedB,
		cache, transposedA, normal, extentsB);
	}

// ---------------------------------------------------------------------------
// phys_fn_001749 at 0x0003add0 -- matrix A [BOX][BOX]. See the header.

void __cdecl NxContactBoxBox(const NxCollisionShape* box0,
	const NxCollisionShape* box1, NxContactSink* sink, void* context)
	{
	(void) context;

	// 0x0003ade1..0x0003ae18, the same guard the two sphere entries carry and
	// in the same shape: box0 is tested first and box1 only if box0's
	// owner->[8] is non-null. The pushes name the argument order --
	// `push edi; push esi; push ebp` at 0x0003adfd is
	// (moving = box1, fixed = box0, sink).
	if(!*(void* const*) ((const NxU8*) NxShapeOwner(box0, 0) + 8))
		NxContinuousCdPair(box1, box0, sink);
	else if(!*(void* const*) ((const NxU8*) NxShapeOwner(box1, 0) + 8))
		NxContinuousCdPair(box0, box1, sink);

	// The oracle's arrays are sixteen deep and end flush against its own return
	// address; see the header. Eighty is what phys_fn_001741 can actually
	// produce, so this copy stops where the oracle would start writing over its
	// caller.
	NxVec3 points[80];
	NxReal separations[80];
	NxVec3 normal;

	// `lea eax,[esi+0xe4]` and `lea edx,[esi+0xc]` at 0x0003ae31 and 0x0003ae2d:
	// the extents are Shape+0xe4, which is `geometry[1..3]`, and the pose is
	// Shape+0x0c, the row-major 3x3 followed by the translation.
	const int count = NxBoxBoxTransposedPair(points, separations, &normal,
		&box0->geometry[1], &box0->rotation[0],
		&box1->geometry[1], &box1->rotation[0],
		&sink->separatingAxis);

	// 0x0003ae52..0x0003ae66. A pair that produced nothing writes ZERO over the
	// cached axis on the way out -- so the entry, not the search, is what
	// forgets an axis, and it only forgets it when the search found none.
	if(count == 0)
		{
		sink->separatingAxis = 0;
		return;
		}

	// The orientation swap, 0x0003ae67, and it reads the FIRST shape's owner
	// where plane/box reads the second's. box0 is this entry's `object1`.
	const NxCollisionShape* first = box0;
	const NxCollisionShape* second = box1;
	const bool negated =
		*(void* const*) ((const NxU8*) box0->owner + 8) != sink->orientedTo;
	if(negated)
		{
		first = box1;
		second = box0;
		}

	// No header predicate: written whenever the call emitted (`test eax,eax;
	// jne` at 0x0003ae58 is the only gate). The feature half is always zero
	// because sink->[0x34] is cleared at 0x0003ae82, which also makes the fifth
	// contact word at 0x0003b08e dead.
	sink->featurePairValid = 0;
	nxAppendPairHeader(sink, first->collisionObject, second->collisionObject,
		nxHeaderMaterial(first, second), 0);

	// No normal predicate either: the block always follows. The oracle even
	// clears the cached normal at 0x0003af45 and overwrites it at 0x0003af81,
	// so the comparison the emitter makes is not merely skipped here -- the
	// state it would read is destroyed first.
	NxVec3 emitted;
	if(negated)
		{
		emitted.x = (NxReal) (-(double) normal.x);
		emitted.y = (NxReal) (-(double) normal.y);
		emitted.z = (NxReal) (-(double) normal.z);
		}
	else
		emitted = normal;
	nxAppendNormalBlock(sink, &emitted);

	// The manifold, 0x0003b010..0x0003b0c6. NO CAP -- there is no `cmp eax,6`
	// here, which is what makes this the one entry where the stream growth path
	// could actually be reached. The separation is negated through the x87
	// (`fld dword; fchs; fstp dword` at 0x0003b013) and then masked by
	// nxAppendContactRecord's own `and ebp,0x7fffffff` at 0x0003b064, so the
	// negation changes nothing for a finite value -- but the load quiets a
	// signalling NaN, which a bare mask would not.
	for(int c = 0; c < count; ++c)
		{
		const NxReal flipped = (NxReal) (-(double) separations[c]);
		nxAppendContactRecord(sink, &points[c], nxBits(flipped), 0);
		}
	}

// ---------------------------------------------------------------------------
// phys_fn_001753 (0x0003b260, 2267 B)
// Matrix A [BOX][CAPSULE] (convex-mesh gap Task 2a; units/convex-mesh-gap-
// contract.md, sub-unit G). The capsule's axis is built as the overlap entries
// build it (0x0003b26e..0x0003b2e7), and then three algorithms behind two
// tests:
//
//  * NX_SWEPT_SHAPE (the capsule's flag byte at +0xe8, 0x0003b28b): a ray
//    along the axis through the BOX's vtable slot 5 -- the one indirect call,
//    `call dword ptr [eax+0x14]` at 0x0003b3da with ecx = arg0, the box, and
//    [box] its vtable: phys_fn_000949 (0x00020880) on a box shape -- asked for
//    the normal (`push 4`), and the hit emitted with separation 0.
//  * otherwise a sphere of the capsule's radius at each end through
//    phys_fn_001917, each hit emitted when its normal is within 0.01 of facing
//    along the axis away from the other end; and unless BOTH ends emitted, the
//    axis against the box through phys_fn_001688:
//  * a positive squared distance inside the radius emits one contact between
//    the closest points; exactly zero -- the axis crosses the box -- runs the
//    box/box search (phys_fn_001748) with the capsule as a box of half size
//    (0.666 r, h, 0.666 r) and writes the manifold inline, the way
//    phys_fn_001749 does.
void __cdecl NxContactBoxCapsule(const NxCollisionShape* box,
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

	const NxU8 sweptFlag = *(const NxU8*) &capsule->geometry[2];
	if(sweptFlag & 1)
		{
		// 0x0003b2f1. x is stored with `fst` and the wide difference is what a
		// zero-length axis leaves as the direction (0x0003b38e `fstp`); the
		// length's sum is (x^2 + z^2) + y^2 over the three stored words.
		const double wideDx = (double) segment.p1.x - segment.p0.x;
		const NxReal dx = (NxReal) wideDx;
		const NxReal dy = (NxReal) ((double) segment.p1.y - segment.p0.y);
		const NxReal dz = (NxReal) ((double) segment.p1.z - segment.p0.z);
		const NxReal length = (NxReal) x87FsqrtDot3(dx, dx, dz, dz, dy, dy);

		NxRay ray;
		ray.orig = segment.p0;
		if(length == 0.0f)
			{
			ray.dir.x = (NxReal) wideDx;
			ray.dir.y = dy;
			ray.dir.z = dz;
			}
		else
			{
			// Over the half height's slot, dead on this path.
			const NxReal inverse = (NxReal) (1.0f / (double) length);
			ray.dir.x = (NxReal) ((double) dx * inverse);
			ray.dir.y = (NxReal) ((double) dy * inverse);
			ray.dir.z = (NxReal) ((double) dz * inverse);
			}

		NxRaycastHit hit;
		const NxShapeRaycastFn raycast = (*(const NxShapeRaycastFn* const*) box)[5];
		if(!raycast(box, &ray, length, 0, NX_RAYCAST_NORMAL, &hit))
			return;
		NxEmitContact(sink, capsule->collisionObject, box->collisionObject,
			0, &hit.worldImpact, &hit.worldNormal, 0xffff, 0xffff);
		return;
		}

	// 0x0003b424. The box is flattened into the frame the way phys_fn_001919
	// does it, and the radius is copied over the half height's slot.
	const NxReal radius = capsule->geometry[0];
	NxCollisionBoxData boxData;
	boxData.center[0] = box->translation[0];
	boxData.center[1] = box->translation[1];
	boxData.center[2] = box->translation[2];
	boxData.extents[0] = box->geometry[1];
	boxData.extents[1] = box->geometry[2];
	boxData.extents[2] = box->geometry[3];
	for(int i = 0; i < 9; ++i)
		boxData.rotation[i] = box->rotation[i];

	// In the listing's frame after its prologue pushes (0x0003b42e, 0x0003b460),
	// [esp+0x24] holds a byte per end, [esp+0x4c] whether the axis has been
	// normalised, and the axis itself sits in [esp+0x18..0x20]. The axis is
	// negated at the bottom of every pass (0x0003b5fd), normalised or not, so
	// the second end is tested against the axis pointing back at the first.
	NxU8 emitted[2];
	int axisReady = 0;
	NxReal axis[3] = { 0.0f, 0.0f, 0.0f };
	const NxVec3* ends[2] = { &segment.p0, &segment.p1 };
	for(int end = 0; end < 2; ++end)
		{
		NxCollisionSphereData sphere;
		sphere.center[0] = ends[end]->x;
		sphere.center[1] = ends[end]->y;
		sphere.center[2] = ends[end]->z;
		sphere.radius = radius;

		NxVec3 point;
		NxVec3 normal;
		NxReal separation;
		emitted[end] = NxSphereBoxContactData(&sphere, &boxData, &point, &normal, &separation) ? 1 : 0;
		if(emitted[end])
			{
			if(!axisReady)
				{
				// 0x0003b50c. x and z stay in registers; the length is
				// (x^2 + z^2) + y^2 with y narrowed, and a zero length
				// leaves the three stored differences.
				const double adx = (double) segment.p1.x - segment.p0.x;
				const NxReal ady = (NxReal) ((double) segment.p1.y - segment.p0.y);
				const double adz = (double) segment.p1.z - segment.p0.z;
				axis[1] = ady;
				axis[0] = (NxReal) adx;
				axis[2] = (NxReal) adz;
				const double length = x87FsqrtDot3(adx, adx, adz, adz, ady, ady);
				if(length != 0.0)
					{
					const double inverse = 1.0f / length;
					axis[0] = (NxReal) (adx * inverse);
					axis[1] = (NxReal) ((double) ady * inverse);
					axis[2] = (NxReal) (adz * inverse);
					}
				axisReady = 1;
				}

			// 0x0003b58f: against the DOUBLE -0.01 at 0x10107b58, strictly.
			const double facing = ((double) normal.x * axis[0] + (double) normal.z * axis[2])
				+ (double) normal.y * axis[1];
			if(facing > -0.01)
				NxEmitContact(sink, capsule->collisionObject, box->collisionObject,
					nxBits(separation), &point, &normal, 0xffff, 0xffff);
			else
				emitted[end] = 0;
			}

		const NxReal flippedX = (NxReal) (-(double) axis[0]);
		const NxReal flippedY = (NxReal) (-(double) axis[1]);
		const NxReal flippedZ = (NxReal) (-(double) axis[2]);
		axis[0] = flippedX;
		axis[1] = flippedY;
		axis[2] = flippedZ;
		}

	// 0x0003b63a: both ends emitted, nothing more.
	if(emitted[0] && emitted[1])
		return;

	NxReal parameter;
	NxReal boxPoint[3];
	const NxReal squared = (NxReal) NxSegmentBoxSquareDistance(&segment, boxData.center,
		boxData.extents, boxData.rotation, &parameter, boxPoint);

	if(squared == 0.0f)
		{
		// 0x0003b843: the axis crosses the box. The capsule becomes a box of
		// half size (0.666 r, h, 0.666 r) on its own pose; 0.666f is the float
		// at 0x10107b54 and the product is narrowed into both slots.
		const double scaled = (double) capsule->geometry[0] * 0.666f;
		NxReal pseudoExtents[3];
		pseudoExtents[0] = (NxReal) scaled;
		pseudoExtents[1] = halfHeight;
		pseudoExtents[2] = (NxReal) scaled;

		// Sixteen deep in the oracle (0x0003b889, 0x0003b881: the frame ends at
		// the return address); eighty here, as in NxContactBoxBox.
		NxVec3 points[80];
		NxReal separations[80];
		NxVec3 normal;
		const int count = NxBoxBoxTransposedPair(points, separations, &normal,
			pseudoExtents, &capsule->rotation[0],
			&box->geometry[1], &box->rotation[0], &sink->separatingAxis);

		// 0x0003b8a3: `mov byte ptr [esi], al` -- the count's low byte, zero.
		if(count == 0)
			{
			sink->separatingAxis = 0;
			return;
			}

		// 0x0003b8ae: the orientation is the CAPSULE's owner, the first shape
		// of the box/box search.
		const NxCollisionShape* first = capsule;
		const NxCollisionShape* second = box;
		const bool negated =
			*(void* const*) ((const NxU8*) capsule->owner + 8) != sink->orientedTo;
		if(negated)
			{
			first = box;
			second = capsule;
			}

		sink->featurePairValid = 0;
		nxAppendPairHeader(sink, first->collisionObject, second->collisionObject,
			nxHeaderMaterial(first, second), 0);

		NxVec3 emittedNormal;
		if(negated)
			{
			emittedNormal.x = (NxReal) (-(double) normal.x);
			emittedNormal.y = (NxReal) (-(double) normal.y);
			emittedNormal.z = (NxReal) (-(double) normal.z);
			}
		else
			emittedNormal = normal;
		nxAppendNormalBlock(sink, &emittedNormal);

		for(int c = 0; c < count; ++c)
			{
			const NxReal flipped = (NxReal) (-(double) separations[c]);
			nxAppendContactRecord(sink, &points[c], nxBits(flipped), 0);
			}
		return;
		}

	// 0x0003b695: strictly inside the radius, against the narrowed distance.
	if(!((double) radius * radius > squared))
		return;

	// The closest point on the axis: x narrowed after the multiply and again
	// after the add, y narrowed once, z kept in a register (0x0003b6ac..0x0003b6f4).
	const double sdx = (double) segment.p1.x - segment.p0.x;
	const double sdy = (double) segment.p1.y - segment.p0.y;
	const NxReal sdz = (NxReal) ((double) segment.p1.z - segment.p0.z);
	const NxReal alongX = (NxReal) (sdx * parameter);
	const NxReal onAxisX = (NxReal) ((double) alongX + segment.p0.x);
	const NxReal onAxisY = (NxReal) (sdy * parameter + segment.p0.y);
	const double onAxisZ = (double) sdz * parameter + segment.p0.z;

	// The box's closest point back in world space. Row 0 is summed
	// ((b0 r0 + b2 r2) + b1 r1), rows 1 and 2 ((b2 + b1) + b0); x and y are
	// added to the centre from registers, z narrowed first (0x0003b6f8..0x0003b78e).
	const NxReal* r = boxData.rotation;
	const double worldX = ((double) boxPoint[0] * r[0] + (double) boxPoint[2] * r[2])
		+ (double) boxPoint[1] * r[1];
	const double worldY = ((double) r[5] * boxPoint[2] + (double) r[4] * boxPoint[1])
		+ (double) r[3] * boxPoint[0];
	const NxReal worldZ = (NxReal) (((double) r[8] * boxPoint[2] + (double) r[7] * boxPoint[1])
		+ (double) r[6] * boxPoint[0]);

	NxVec3 point;
	point.x = (NxReal) (worldX + boxData.center[0]);
	point.y = (NxReal) (worldY + boxData.center[1]);
	point.z = (NxReal) ((double) worldZ + boxData.center[2]);

	NxVec3 normal;
	normal.x = (NxReal) ((double) onAxisX - point.x);
	normal.y = (NxReal) ((double) onAxisY - point.y);
	const double normalZ = onAxisZ - point.z;

	const double length = x87FsqrtDot3(normal.x, normal.x, normalZ, normalZ, normal.y, normal.y);
	if(length == 0.0)
		return;
	const double inverse = 1.0f / length;
	normal.x = (NxReal) ((double) normal.x * inverse);
	normal.y = (NxReal) ((double) normal.y * inverse);
	normal.z = (NxReal) (normalZ * inverse);

	// The separation from the narrowed squared distance (0x0003b824).
	const NxReal separation = (NxReal) (x87Fsqrt(squared) - radius);
	NxEmitContact(sink, capsule->collisionObject, box->collisionObject,
		nxBits(separation), &point, &normal, 0xffff, 0xffff);
	}

// ---------------------------------------------------------------------------
// convex-mesh gap Task 2g: prerequisites P-Emit (000875) and P-Plane (001903,
// 001907, 001909 with their continuations 001905 and 001911), from the Capstone
// listing (units/convex-mesh-gap-contract.md). 000875 sits after 000873 in the
// image and the P-Plane rows after 001901 (plane/sphere), both in this file's
// units, so they are written here.
//
// Every row is the listing's instructions, naked (branch targets are labels
// named by their oracle RVA), because each keeps values on the x87 stack across
// narrowing stores and mixes register and stack arguments no C++ declaration
// expresses. They call: the vendored Container::Resize (004840, a private
// member, reached through an /alternatename alias of its decorated name, as
// ConvexHull.cpp reaches the Container's constructor), the CRT's stack probe
// (005695, `_chkstk`: eax the byte count, esp moved by it -- the same contract
// as the oracle's static-CRT copy) and the Foundation export
// NxFindRotationMatrix through its import slot (`call dword ptr
// [__imp__NxFindRotationMatrix]`, the listing's `call dword ptr [0x10104174]`).

extern "C" void _chkstk();								// 005695, the stack probe
extern "C" void* _imp__NxFindRotationMatrix;			// the import slot 0x10104174

// .rdata 0x101041f0 (0.0f), 0x101041ec (1.0f), 0x10107880 / 0x10107888 (the
// doubles 1e-7 and -1e-7).
static const float kContactZero = 0.0f;
static const float kContactOne = 1.0f;
static const double kContactTenth7 = 1e-7;
static const double kContactMinusTenth7 = -1e-7;

// phys_fn_000875 (0x0001d8e0, 915 B)
// The contact emitter with feature words (thiscall on the sink, nine stack
// arguments, `ret 0x24`): the two collision objects, the separation's bits, the
// point, the normal, two 16-bit feature ids and two 32-bit feature words. As
// 000873 it reads each object's shape at +0x08 and, when shape1's owner's +0x08
// differs from the sink's +0x08, swaps the objects, the two ids and the two
// words and negates the normal into a local (`fld; fchs; fstp`: a signalling
// NaN is quieted). The pair header is written when either collision object
// differs from the sink's last pair (+0x20, +0x24): its flag word (+0x34) is 1
// when both ids are real (not 0xffff) or'd with 4 when either shape's +0xde has
// bit 0x20, shifted into bits 16..31 of the header word, whose top byte is the
// material from shape1's owner's holder (+0x240) or, with a null holder,
// shape0's; the two objects, then the header word (whose stream index goes to
// +0x18, the pair counter at +0x14 incremented), and the cached normal cleared.
// The normal block is written when the normal's words differ from the cached
// ones (+0x28..+0x30): three words and a zero count (its index to +0x1c, the
// header's count incremented). Then the contact: the counter at +0x10, the
// point's three words, the separation word with its sign bit replaced by bit
// 31 set when either 32-bit feature word exceeds 0xffff, the count at +0x1c
// incremented; with flag 1 the ids' word (id1 << 16 | id0); with flag 4 either
// the two feature words (when one exceeds 0xffff) or (wb << 16 | wa). Every
// append grows the stream (the Container at +0x38) through 004840 when full:
// by 1, or by 3 before the three-word appends.
__declspec(naked) void __fastcall NxEmitContactFeatures(NxContactSink* /*sink*/, NxU32 /*edx*/,
	void* /*object1*/, void* /*object0*/, NxU32 /*separationBits*/, const NxVec3* /*point*/,
	const NxVec3* /*normal*/, NxU32 /*featureId0*/, NxU32 /*featureId1*/, NxU32 /*featureWord0*/,
	NxU32 /*featureWord1*/)
	{
	__asm
		{
		mov	eax, dword ptr [esp + 4]		// 0x0001d8e0
		sub	esp, 0xc		// 0x0001d8e4
		push	ebx		// 0x0001d8e7
		mov	ebx, dword ptr [eax + 8]		// 0x0001d8e8
		mov	edx, dword ptr [ebx + 4]		// 0x0001d8eb
		mov	eax, dword ptr [edx + 8]		// 0x0001d8ee
		push	ebp		// 0x0001d8f1
		push	esi		// 0x0001d8f2
		mov	esi, ecx		// 0x0001d8f3
		mov	ecx, dword ptr [esp + 0x20]		// 0x0001d8f5
		mov	ebp, dword ptr [ecx + 8]		// 0x0001d8f9
		cmp	eax, dword ptr [esi + 8]		// 0x0001d8fc
		push	edi		// 0x0001d8ff
		je	L1d950		// 0x0001d900
		mov	ecx, dword ptr [esp + 0x40]		// 0x0001d902
		mov	edx, dword ptr [esp + 0x38]		// 0x0001d906
		mov	eax, ebx		// 0x0001d90a
		mov	ebx, ebp		// 0x0001d90c
		mov	ebp, eax		// 0x0001d90e
		mov	eax, dword ptr [esp + 0x3c]		// 0x0001d910
		mov	dword ptr [esp + 0x40], eax		// 0x0001d914
		mov	eax, dword ptr [esp + 0x34]		// 0x0001d918
		mov	dword ptr [esp + 0x38], eax		// 0x0001d91c
		mov	eax, dword ptr [esp + 0x30]		// 0x0001d920
		fld	dword ptr [eax]		// 0x0001d924
		mov	dword ptr [esp + 0x3c], ecx		// 0x0001d926
		fchs		// 0x0001d92a
		mov	dword ptr [esp + 0x34], edx		// 0x0001d92c
		fstp	dword ptr [esp + 0x10]		// 0x0001d930
		fld	dword ptr [eax + 4]		// 0x0001d934
		fchs		// 0x0001d937
		fstp	dword ptr [esp + 0x14]		// 0x0001d939
		fld	dword ptr [eax + 8]		// 0x0001d93d
		lea	eax, [esp + 0x10]		// 0x0001d940
		fchs		// 0x0001d944
		mov	dword ptr [esp + 0x30], eax		// 0x0001d946
		fstp	dword ptr [esp + 0x18]		// 0x0001d94a
		jmp	L1d958		// 0x0001d94e
L1d950:
		mov	eax, dword ptr [esp + 0x30]		// 0x0001d950
		mov	dword ptr [esp + 0x30], eax		// 0x0001d954
L1d958:
		mov	ecx, dword ptr [esi + 0x20]		// 0x0001d958
		cmp	ecx, dword ptr [ebx + 0x9c]		// 0x0001d95b
		jne	L1d972		// 0x0001d961
		mov	edx, dword ptr [esi + 0x24]		// 0x0001d963
		cmp	edx, dword ptr [ebp + 0x9c]		// 0x0001d966
		je	L1da86		// 0x0001d96c
L1d972:
		mov	eax, 0xffff		// 0x0001d972
		cmp	word ptr [esp + 0x34], ax		// 0x0001d977
		je	L1d98c		// 0x0001d97c
		cmp	word ptr [esp + 0x38], ax		// 0x0001d97e
		je	L1d98c		// 0x0001d983
		mov	ecx, 1		// 0x0001d985
		jmp	L1d98e		// 0x0001d98a
L1d98c:
		xor	ecx, ecx		// 0x0001d98c
L1d98e:
		mov	dl, byte ptr [ebx + 0xde]		// 0x0001d98e
		mov	al, 0x20		// 0x0001d994
		_emit	0x84
		_emit	0xd0		// 0x0001d996 test al, dl
		jne	L1d9a6		// 0x0001d998
		test	byte ptr [ebp + 0xde], al		// 0x0001d99a
		jne	L1d9a6		// 0x0001d9a0
		xor	eax, eax		// 0x0001d9a2
		jmp	L1d9ab		// 0x0001d9a4
L1d9a6:
		mov	eax, 4		// 0x0001d9a6
L1d9ab:
		or	eax, ecx		// 0x0001d9ab
		mov	dword ptr [esi + 0x34], eax		// 0x0001d9ad
		shl	eax, 0x10		// 0x0001d9b0
		mov	dword ptr [esp + 0x24], eax		// 0x0001d9b3
		mov	eax, dword ptr [ebx + 0x9c]		// 0x0001d9b7
		mov	dword ptr [esi + 0x20], eax		// 0x0001d9bd
		mov	ecx, dword ptr [ebp + 0x9c]		// 0x0001d9c0
		mov	dword ptr [esi + 0x24], ecx		// 0x0001d9c6
		mov	eax, dword ptr [esi + 0x3c]		// 0x0001d9c9
		mov	ecx, dword ptr [esi + 0x38]		// 0x0001d9cc
		cmp	eax, ecx		// 0x0001d9cf
		mov	edx, dword ptr [ebx + 0x9c]		// 0x0001d9d1
		lea	edi, [esi + 0x38]		// 0x0001d9d7
		mov	dword ptr [esp + 0x20], edx		// 0x0001d9da
		jne	L1d9e9		// 0x0001d9de
		push	1		// 0x0001d9e0
		mov	ecx, edi		// 0x0001d9e2
		call	nxContactCallContainerResize		// 0x0001d9e4
L1d9e9:
		mov	ecx, dword ptr [edi + 4]		// 0x0001d9e9
		mov	eax, dword ptr [esp + 0x20]		// 0x0001d9ec
		mov	edx, dword ptr [edi + 8]		// 0x0001d9f0
		mov	dword ptr [edx + ecx*4], eax		// 0x0001d9f3
		inc	dword ptr [edi + 4]		// 0x0001d9f6
		mov	edx, dword ptr [edi + 4]		// 0x0001d9f9
		cmp	edx, dword ptr [edi]		// 0x0001d9fc
		mov	ecx, dword ptr [ebp + 0x9c]		// 0x0001d9fe
		mov	dword ptr [esp + 0x20], ecx		// 0x0001da04
		jne	L1da13		// 0x0001da08
		push	1		// 0x0001da0a
		mov	ecx, edi		// 0x0001da0c
		call	nxContactCallContainerResize		// 0x0001da0e
L1da13:
		mov	eax, dword ptr [edi + 4]		// 0x0001da13
		mov	edx, dword ptr [esp + 0x20]		// 0x0001da16
		mov	ecx, dword ptr [edi + 8]		// 0x0001da1a
		mov	dword ptr [ecx + eax*4], edx		// 0x0001da1d
		inc	dword ptr [edi + 4]		// 0x0001da20
		mov	eax, dword ptr [ebx + 4]		// 0x0001da23
		mov	eax, dword ptr [eax + 8]		// 0x0001da26
		test	eax, eax		// 0x0001da29
		je	L1da35		// 0x0001da2b
		mov	ebx, dword ptr [eax + 0x240]		// 0x0001da2d
		jmp	L1da41		// 0x0001da33
L1da35:
		mov	ecx, dword ptr [ebp + 4]		// 0x0001da35
		mov	edx, dword ptr [ecx + 8]		// 0x0001da38
		mov	ebx, dword ptr [edx + 0x240]		// 0x0001da3b
L1da41:
		mov	eax, dword ptr [edi + 4]		// 0x0001da41
		cmp	eax, dword ptr [edi]		// 0x0001da44
		mov	ebp, dword ptr [esi + 0x3c]		// 0x0001da46
		jne	L1da54		// 0x0001da49
		push	1		// 0x0001da4b
		mov	ecx, edi		// 0x0001da4d
		call	nxContactCallContainerResize		// 0x0001da4f
L1da54:
		mov	ecx, dword ptr [edi + 4]		// 0x0001da54
		mov	edx, dword ptr [edi + 8]		// 0x0001da57
		mov	eax, dword ptr [esp + 0x24]		// 0x0001da5a
		shl	ebx, 0x18		// 0x0001da5e
		or	ebx, eax		// 0x0001da61
		mov	dword ptr [edx + ecx*4], ebx		// 0x0001da63
		inc	dword ptr [edi + 4]		// 0x0001da66
		mov	ecx, dword ptr [esi + 0x40]		// 0x0001da69
		mov	eax, dword ptr [esi + 0x14]		// 0x0001da6c
		lea	eax, [ecx + eax*4]		// 0x0001da6f
		mov	dword ptr [esi + 0x18], ebp		// 0x0001da72
		inc	dword ptr [eax]		// 0x0001da75
		xor	eax, eax		// 0x0001da77
		mov	dword ptr [esi + 0x30], eax		// 0x0001da79
		mov	dword ptr [esi + 0x2c], eax		// 0x0001da7c
		mov	dword ptr [esi + 0x28], eax		// 0x0001da7f
		mov	eax, dword ptr [esp + 0x30]		// 0x0001da82
L1da86:
		mov	edx, dword ptr [esi + 0x28]		// 0x0001da86
		cmp	edx, dword ptr [eax]		// 0x0001da89
		jne	L1da9d		// 0x0001da8b
		mov	ecx, dword ptr [esi + 0x2c]		// 0x0001da8d
		cmp	ecx, dword ptr [eax + 4]		// 0x0001da90
		jne	L1da9d		// 0x0001da93
		mov	edx, dword ptr [esi + 0x30]		// 0x0001da95
		cmp	edx, dword ptr [eax + 8]		// 0x0001da98
		je	L1db1a		// 0x0001da9b
L1da9d:
		mov	ecx, dword ptr [eax]		// 0x0001da9d
		mov	dword ptr [esi + 0x28], ecx		// 0x0001da9f
		mov	edx, dword ptr [eax + 4]		// 0x0001daa2
		mov	dword ptr [esi + 0x2c], edx		// 0x0001daa5
		mov	ecx, dword ptr [eax + 8]		// 0x0001daa8
		lea	edi, [esi + 0x38]		// 0x0001daab
		mov	dword ptr [esi + 0x30], ecx		// 0x0001daae
		mov	edx, dword ptr [edi + 4]		// 0x0001dab1
		mov	ecx, dword ptr [edi]		// 0x0001dab4
		add	edx, 3		// 0x0001dab6
		cmp	edx, ecx		// 0x0001dab9
		jbe	L1daca		// 0x0001dabb
		push	3		// 0x0001dabd
		mov	ecx, edi		// 0x0001dabf
		call	nxContactCallContainerResize		// 0x0001dac1
		mov	eax, dword ptr [esp + 0x30]		// 0x0001dac6
L1daca:
		mov	ecx, dword ptr [edi + 4]		// 0x0001daca
		mov	edx, dword ptr [edi + 8]		// 0x0001dacd
		lea	ecx, [edx + ecx*4]		// 0x0001dad0
		mov	edx, dword ptr [eax]		// 0x0001dad3
		mov	dword ptr [ecx], edx		// 0x0001dad5
		mov	edx, dword ptr [eax + 4]		// 0x0001dad7
		mov	dword ptr [ecx + 4], edx		// 0x0001dada
		mov	eax, dword ptr [eax + 8]		// 0x0001dadd
		mov	dword ptr [ecx + 8], eax		// 0x0001dae0
		mov	ecx, dword ptr [edi + 4]		// 0x0001dae3
		add	ecx, 3		// 0x0001dae6
		mov	dword ptr [edi + 4], ecx		// 0x0001dae9
		cmp	ecx, dword ptr [edi]		// 0x0001daec
		mov	ebx, dword ptr [esi + 0x3c]		// 0x0001daee
		jne	L1dafc		// 0x0001daf1
		push	1		// 0x0001daf3
		mov	ecx, edi		// 0x0001daf5
		call	nxContactCallContainerResize		// 0x0001daf7
L1dafc:
		mov	edx, dword ptr [edi + 4]		// 0x0001dafc
		mov	eax, dword ptr [edi + 8]		// 0x0001daff
		mov	dword ptr [eax + edx*4], 0		// 0x0001db02
		inc	dword ptr [edi + 4]		// 0x0001db09
		mov	ecx, dword ptr [esi + 0x18]		// 0x0001db0c
		mov	edx, dword ptr [esi + 0x40]		// 0x0001db0f
		lea	eax, [edx + ecx*4]		// 0x0001db12
		mov	dword ptr [esi + 0x1c], ebx		// 0x0001db15
		inc	dword ptr [eax]		// 0x0001db18
L1db1a:
		mov	eax, dword ptr [esp + 0x28]		// 0x0001db1a
		mov	ecx, dword ptr [esp + 0x3c]		// 0x0001db1e
		mov	dword ptr [esp + 0x30], eax		// 0x0001db22
		mov	eax, 0xffff		// 0x0001db26
		cmp	ecx, eax		// 0x0001db2b
		ja	L1db39		// 0x0001db2d
		cmp	dword ptr [esp + 0x40], eax		// 0x0001db2f
		ja	L1db39		// 0x0001db33
		xor	ebp, ebp		// 0x0001db35
		jmp	L1db3e		// 0x0001db37
L1db39:
		mov	ebp, 0x80000000		// 0x0001db39
L1db3e:
		inc	dword ptr [esi + 0x10]		// 0x0001db3e
		mov	ecx, dword ptr [esi + 0x3c]		// 0x0001db41
		mov	eax, dword ptr [esi + 0x38]		// 0x0001db44
		lea	edi, [esi + 0x38]		// 0x0001db47
		add	ecx, 3		// 0x0001db4a
		cmp	ecx, eax		// 0x0001db4d
		jbe	L1db5a		// 0x0001db4f
		push	3		// 0x0001db51
		mov	ecx, edi		// 0x0001db53
		call	nxContactCallContainerResize		// 0x0001db55
L1db5a:
		mov	eax, dword ptr [edi + 4]		// 0x0001db5a
		mov	ecx, dword ptr [edi + 8]		// 0x0001db5d
		mov	edx, dword ptr [esp + 0x2c]		// 0x0001db60
		mov	ebx, dword ptr [esp + 0x30]		// 0x0001db64
		lea	eax, [ecx + eax*4]		// 0x0001db68
		mov	ecx, dword ptr [edx]		// 0x0001db6b
		mov	dword ptr [eax], ecx		// 0x0001db6d
		mov	ecx, dword ptr [edx + 4]		// 0x0001db6f
		mov	dword ptr [eax + 4], ecx		// 0x0001db72
		mov	edx, dword ptr [edx + 8]		// 0x0001db75
		mov	dword ptr [eax + 8], edx		// 0x0001db78
		mov	ecx, dword ptr [edi + 4]		// 0x0001db7b
		add	ecx, 3		// 0x0001db7e
		and	ebx, 0x7fffffff		// 0x0001db81
		mov	eax, ecx		// 0x0001db87
		mov	dword ptr [edi + 4], ecx		// 0x0001db89
		mov	ecx, dword ptr [edi]		// 0x0001db8c
		or	ebx, ebp		// 0x0001db8e
		cmp	eax, ecx		// 0x0001db90
		jne	L1db9d		// 0x0001db92
		push	1		// 0x0001db94
		mov	ecx, edi		// 0x0001db96
		call	nxContactCallContainerResize		// 0x0001db98
L1db9d:
		mov	ecx, dword ptr [edi + 4]		// 0x0001db9d
		mov	edx, dword ptr [edi + 8]		// 0x0001dba0
		mov	dword ptr [edx + ecx*4], ebx		// 0x0001dba3
		inc	dword ptr [edi + 4]		// 0x0001dba6
		mov	ecx, dword ptr [esi + 0x40]		// 0x0001dba9
		mov	eax, dword ptr [esi + 0x1c]		// 0x0001dbac
		lea	eax, [ecx + eax*4]		// 0x0001dbaf
		inc	dword ptr [eax]		// 0x0001dbb2
		test	byte ptr [esi + 0x34], 1		// 0x0001dbb4
		je	L1dbe5		// 0x0001dbb8
		mov	edx, dword ptr [edi + 4]		// 0x0001dbba
		cmp	edx, dword ptr [edi]		// 0x0001dbbd
		jne	L1dbca		// 0x0001dbbf
		push	1		// 0x0001dbc1
		mov	ecx, edi		// 0x0001dbc3
		call	nxContactCallContainerResize		// 0x0001dbc5
L1dbca:
		movzx	eax, word ptr [esp + 0x38]		// 0x0001dbca
		movzx	ecx, word ptr [esp + 0x34]		// 0x0001dbcf
		mov	edx, dword ptr [edi + 4]		// 0x0001dbd4
		shl	eax, 0x10		// 0x0001dbd7
		or	eax, ecx		// 0x0001dbda
		mov	ecx, dword ptr [edi + 8]		// 0x0001dbdc
		mov	dword ptr [ecx + edx*4], eax		// 0x0001dbdf
		inc	dword ptr [edi + 4]		// 0x0001dbe2
L1dbe5:
		test	byte ptr [esi + 0x34], 4		// 0x0001dbe5
		je	L1dc69		// 0x0001dbe9
		test	ebp, ebp		// 0x0001dbeb
		mov	eax, dword ptr [edi]		// 0x0001dbed
		je	L1dc40		// 0x0001dbef
		mov	edx, dword ptr [edi + 4]		// 0x0001dbf1
		cmp	edx, eax		// 0x0001dbf4
		jne	L1dc01		// 0x0001dbf6
		push	1		// 0x0001dbf8
		mov	ecx, edi		// 0x0001dbfa
		call	nxContactCallContainerResize		// 0x0001dbfc
L1dc01:
		mov	eax, dword ptr [edi + 4]		// 0x0001dc01
		mov	ecx, dword ptr [edi + 8]		// 0x0001dc04
		mov	edx, dword ptr [esp + 0x3c]		// 0x0001dc07
		mov	dword ptr [ecx + eax*4], edx		// 0x0001dc0b
		mov	edx, dword ptr [edi + 4]		// 0x0001dc0e
		inc	edx		// 0x0001dc11
		mov	dword ptr [edi + 4], edx		// 0x0001dc12
		mov	ecx, dword ptr [edi]		// 0x0001dc15
		mov	eax, edx		// 0x0001dc17
		cmp	eax, ecx		// 0x0001dc19
		jne	L1dc26		// 0x0001dc1b
		push	1		// 0x0001dc1d
		mov	ecx, edi		// 0x0001dc1f
		call	nxContactCallContainerResize		// 0x0001dc21
L1dc26:
		mov	ecx, dword ptr [edi + 4]		// 0x0001dc26
		mov	edx, dword ptr [edi + 8]		// 0x0001dc29
		mov	eax, dword ptr [esp + 0x40]		// 0x0001dc2c
		mov	dword ptr [edx + ecx*4], eax		// 0x0001dc30
		inc	dword ptr [edi + 4]		// 0x0001dc33
		pop	edi		// 0x0001dc36
		pop	esi		// 0x0001dc37
		pop	ebp		// 0x0001dc38
		pop	ebx		// 0x0001dc39
		add	esp, 0xc		// 0x0001dc3a
		ret	0x24		// 0x0001dc3d
L1dc40:
		mov	ecx, dword ptr [edi + 4]		// 0x0001dc40
		cmp	ecx, eax		// 0x0001dc43
		jne	L1dc50		// 0x0001dc45
		push	1		// 0x0001dc47
		mov	ecx, edi		// 0x0001dc49
		call	nxContactCallContainerResize		// 0x0001dc4b
L1dc50:
		mov	eax, dword ptr [esp + 0x40]		// 0x0001dc50
		mov	ecx, dword ptr [esp + 0x3c]		// 0x0001dc54
		mov	edx, dword ptr [edi + 4]		// 0x0001dc58
		shl	eax, 0x10		// 0x0001dc5b
		or	eax, ecx		// 0x0001dc5e
		mov	ecx, dword ptr [edi + 8]		// 0x0001dc60
		mov	dword ptr [ecx + edx*4], eax		// 0x0001dc63
		inc	dword ptr [edi + 4]		// 0x0001dc66
L1dc69:
		pop	edi		// 0x0001dc69
		pop	esi		// 0x0001dc6a
		pop	ebp		// 0x0001dc6b
		pop	ebx		// 0x0001dc6c
		add	esp, 0xc		// 0x0001dc6d
		ret	0x24		// 0x0001dc70
		}
	}

// phys_fn_001903 (0x00048b30, 45 B)
// phys_fn_001905 (0x00048b60, 111 B)
// Whether a point lies inside a convex polygon in its plane's frame (the row
// and its continuation, the loop, which the contract's list lacked). Register
// arguments: eax the vertex count, ecx the vertices (12-byte stride, x and y
// read); the point's x and y on the stack, cleaned by the caller. Each edge
// from the previous vertex (the last, first) whose ends lie on opposite sides
// of the point's y (`fcomp; test ah, 1`: a NaN y counts as below) is tested for
// the side the point is on ((prev.y - cur.y)(cur.x - x) against (prev.x -
// cur.x)(cur.y - y), `test ah, 0x41; jp`); a crossing on the side that matches
// the edge's direction is counted, and a second one returns 0. Returns the
// count's low bit.
__declspec(naked) NxU32 nxPolygonContainsPoint(float /*x*/, float /*y*/)
	{
	__asm
		{
		push	ebx		// 0x00048b30
		push	ebp		// 0x00048b31
		push	esi		// 0x00048b32
		push	edi		// 0x00048b33
		mov	edi, eax		// 0x00048b34
		lea	eax, [edi + edi*2]		// 0x00048b36
		fld	dword ptr [ecx + eax*4 - 8]		// 0x00048b39
		lea	esi, [ecx + eax*4 - 0xc]		// 0x00048b3d
		fcomp	dword ptr [esp + 0x18]		// 0x00048b41
		fnstsw	ax		// 0x00048b45
		test	ah, 1		// 0x00048b47
		jne	L48b53		// 0x00048b4a
		mov	ebx, 1		// 0x00048b4c
		jmp	L48b55		// 0x00048b51
L48b53:
		xor	ebx, ebx		// 0x00048b53
L48b55:
		xor	ebp, ebp		// 0x00048b55
		test	edi, edi		// 0x00048b57
		je	L48bbe		// 0x00048b59
		jmp	L48b60		// 0x00048b5b
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x00048b5d lea ecx, [ecx]
L48b60:
		fld	dword ptr [ecx + 4]		// 0x00048b60
		dec	edi		// 0x00048b63
		fcomp	dword ptr [esp + 0x18]		// 0x00048b64
		fnstsw	ax		// 0x00048b68
		test	ah, 1		// 0x00048b6a
		jne	L48b76		// 0x00048b6d
		mov	edx, 1		// 0x00048b6f
		jmp	L48b78		// 0x00048b74
L48b76:
		xor	edx, edx		// 0x00048b76
L48b78:
		cmp	ebx, edx		// 0x00048b78
		je	L48bb3		// 0x00048b7a
		fld	dword ptr [esi]		// 0x00048b7c
		fsub	dword ptr [ecx]		// 0x00048b7e
		fld	dword ptr [ecx + 4]		// 0x00048b80
		fsub	dword ptr [esp + 0x18]		// 0x00048b83
		fmulp	st(1), st		// 0x00048b87
		fld	dword ptr [esi + 4]		// 0x00048b89
		fsub	dword ptr [ecx + 4]		// 0x00048b8c
		fld	dword ptr [ecx]		// 0x00048b8f
		fsub	dword ptr [esp + 0x14]		// 0x00048b91
		fmulp	st(1), st		// 0x00048b95
		fcompp		// 0x00048b97
		fnstsw	ax		// 0x00048b99
		test	ah, 0x41		// 0x00048b9b
		jp	L48ba7		// 0x00048b9e
		mov	eax, 1		// 0x00048ba0
		jmp	L48ba9		// 0x00048ba5
L48ba7:
		xor	eax, eax		// 0x00048ba7
L48ba9:
		cmp	eax, edx		// 0x00048ba9
		jne	L48bb3		// 0x00048bab
		cmp	ebp, 1		// 0x00048bad
		je	L48bc8		// 0x00048bb0
		inc	ebp		// 0x00048bb2
L48bb3:
		mov	esi, ecx		// 0x00048bb3
		add	ecx, 0xc		// 0x00048bb5
		test	edi, edi		// 0x00048bb8
		mov	ebx, edx		// 0x00048bba
		jne	L48b60		// 0x00048bbc
L48bbe:
		pop	edi		// 0x00048bbe
		pop	esi		// 0x00048bbf
		mov	eax, ebp		// 0x00048bc0
		pop	ebp		// 0x00048bc2
		and	eax, 1		// 0x00048bc3
		pop	ebx		// 0x00048bc6
		ret		// 0x00048bc7
L48bc8:
		pop	edi		// 0x00048bc8
		pop	esi		// 0x00048bc9
		pop	ebp		// 0x00048bca
		xor	eax, eax		// 0x00048bcb
		pop	ebx		// 0x00048bcd
		ret		// 0x00048bce
		}
	}

// phys_fn_001907 (0x00048bd0, 607 B)
// An edge of one polygon clipped against the plane through an edge of the
// other (register arguments edx, ecx, esi and ebx, five stack arguments
// cleaned by the caller; 0 or 1 in eax). The two ends' plane values are
// multiplied and tested against 0.0f (`test ah, 0x41`: touching, crossing and
// NaN go on, a product above 0.0f returns 0); the edge's direction is
// normalised when its squared length is not 0.0f (`test ah, 0x44; jnp`); the
// two larger axes of the plane normal (compared as words with their sign bits
// cleared) choose the 2D solve for the crossing parameter, which is stored
// through the last argument and must be above 0.0f (`test ah, 5; jnp`); the
// point is written through esi and the result is 1 when its plane value is not
// above 0.0f.
__declspec(naked) NxU32 nxClipEdgeToPolygonPlane()
	{
	__asm
		{
		sub	esp, 0x18		// 0x00048bd0
		fld	dword ptr [edx]		// 0x00048bd3
		push	ebp		// 0x00048bd5
		fmul	dword ptr [ecx]		// 0x00048bd6
		push	edi		// 0x00048bd8
		mov	edi, dword ptr [esp + 0x30]		// 0x00048bd9
		mov	ebp, dword ptr [esp + 0x2c]		// 0x00048bdd
		fstp	dword ptr [esp + 0x30]		// 0x00048be1
		fld	dword ptr [ecx + 4]		// 0x00048be5
		fmul	dword ptr [edi + 4]		// 0x00048be8
		fld	dword ptr [edi + 8]		// 0x00048beb
		fmul	dword ptr [ecx + 8]		// 0x00048bee
		faddp	st(1), st		// 0x00048bf1
		fld	dword ptr [edi]		// 0x00048bf3
		fmul	dword ptr [ecx]		// 0x00048bf5
		faddp	st(1), st		// 0x00048bf7
		fadd	dword ptr [ecx + 0xc]		// 0x00048bf9
		fld	dword ptr [ecx + 4]		// 0x00048bfc
		fmul	dword ptr [edx + 4]		// 0x00048bff
		fld	dword ptr [edx + 8]		// 0x00048c02
		fmul	dword ptr [ecx + 8]		// 0x00048c05
		faddp	st(1), st		// 0x00048c08
		fadd	dword ptr [esp + 0x30]		// 0x00048c0a
		fadd	dword ptr [ecx + 0xc]		// 0x00048c0e
		fmulp	st(1), st		// 0x00048c11
		fcomp	dword ptr kContactZero		// 0x00048c13
		fnstsw	ax		// 0x00048c19
		test	ah, 0x41		// 0x00048c1b
		je	L48cb0		// 0x00048c1e
		fld	dword ptr [edi]		// 0x00048c24
		fsub	dword ptr [edx]		// 0x00048c26
		fld	dword ptr [edi + 4]		// 0x00048c28
		fsub	dword ptr [edx + 4]		// 0x00048c2b
		fstp	dword ptr [esp + 0xc]		// 0x00048c2e
		fld	dword ptr [edi + 8]		// 0x00048c32
		fsub	dword ptr [edx + 8]		// 0x00048c35
		fst	dword ptr [esp + 0x10]		// 0x00048c38
		fmul	dword ptr [esp + 0x10]		// 0x00048c3c
		fld	st(1)		// 0x00048c40
		fmul	st, st(2)		// 0x00048c42
		faddp	st(1), st		// 0x00048c44
		fld	dword ptr [esp + 0xc]		// 0x00048c46
		fmul	dword ptr [esp + 0xc]		// 0x00048c4a
		faddp	st(1), st		// 0x00048c4e
		fld	dword ptr kContactZero		// 0x00048c50
		fld	st(1)		// 0x00048c56
		fucompp		// 0x00048c58
		fnstsw	ax		// 0x00048c5a
		test	ah, 0x44		// 0x00048c5c
		jnp	L48c83		// 0x00048c5f
		fsqrt		// 0x00048c61
		fdivr	dword ptr kContactOne		// 0x00048c63
		fxch	st(1)		// 0x00048c69
		fmul	st, st(1)		// 0x00048c6b
		fxch	st(1)		// 0x00048c6d
		fld	dword ptr [esp + 0xc]		// 0x00048c6f
		fmul	st, st(1)		// 0x00048c73
		fstp	dword ptr [esp + 0xc]		// 0x00048c75
		fld	dword ptr [esp + 0x10]		// 0x00048c79
		fmul	st, st(1)		// 0x00048c7d
		fstp	dword ptr [esp + 0x10]		// 0x00048c7f
L48c83:
		fstp	st(0)		// 0x00048c83
		fld	dword ptr [esp + 0xc]		// 0x00048c85
		fmul	dword ptr [ecx + 4]		// 0x00048c89
		fld	dword ptr [esp + 0x10]		// 0x00048c8c
		fmul	dword ptr [ecx + 8]		// 0x00048c90
		faddp	st(1), st		// 0x00048c93
		fld	st(1)		// 0x00048c95
		fmul	dword ptr [ecx]		// 0x00048c97
		faddp	st(1), st		// 0x00048c99
		fld	dword ptr kContactZero		// 0x00048c9b
		fld	st(1)		// 0x00048ca1
		fucompp		// 0x00048ca3
		fnstsw	ax		// 0x00048ca5
		test	ah, 0x44		// 0x00048ca7
		jp	L48cb8		// 0x00048caa
		fstp	st(0)		// 0x00048cac
L48cae:
		fstp	st(0)		// 0x00048cae
L48cb0:
		pop	edi		// 0x00048cb0
		xor	eax, eax		// 0x00048cb1
		pop	ebp		// 0x00048cb3
		add	esp, 0x18		// 0x00048cb4
		ret		// 0x00048cb7
L48cb8:
		fld	dword ptr [ecx + 4]		// 0x00048cb8
		mov	eax, esi		// 0x00048cbb
		fmul	dword ptr [edx + 4]		// 0x00048cbd
		fld	dword ptr [edx + 8]		// 0x00048cc0
		fmul	dword ptr [ecx + 8]		// 0x00048cc3
		faddp	st(1), st		// 0x00048cc6
		fadd	dword ptr [esp + 0x30]		// 0x00048cc8
		fadd	dword ptr [ecx + 0xc]		// 0x00048ccc
		fdiv	st, st(1)		// 0x00048ccf
		fstp	dword ptr [esp + 0x30]		// 0x00048cd1
		fstp	st(0)		// 0x00048cd5
		fmul	dword ptr [esp + 0x30]		// 0x00048cd7
		fld	dword ptr [esp + 0x30]		// 0x00048cdb
		fmul	dword ptr [esp + 0xc]		// 0x00048cdf
		fld	dword ptr [esp + 0x10]		// 0x00048ce3
		fmul	dword ptr [esp + 0x30]		// 0x00048ce7
		fstp	dword ptr [esp + 0x1c]		// 0x00048ceb
		fld	dword ptr [edx]		// 0x00048cef
		fsub	st, st(2)		// 0x00048cf1
		fstp	dword ptr [esp + 8]		// 0x00048cf3
		fld	dword ptr [edx + 4]		// 0x00048cf7
		fsub	st, st(1)		// 0x00048cfa
		fstp	dword ptr [esp + 0xc]		// 0x00048cfc
		fstp	st(0)		// 0x00048d00
		fstp	st(0)		// 0x00048d02
		fld	dword ptr [edx + 8]		// 0x00048d04
		mov	edx, dword ptr [esp + 8]		// 0x00048d07
		fsub	dword ptr [esp + 0x1c]		// 0x00048d0b
		mov	dword ptr [eax], edx		// 0x00048d0f
		mov	edx, dword ptr [esp + 0xc]		// 0x00048d11
		mov	dword ptr [eax + 4], edx		// 0x00048d15
		fstp	dword ptr [esp + 0x10]		// 0x00048d18
		mov	edx, dword ptr [esp + 0x10]		// 0x00048d1c
		mov	dword ptr [eax + 8], edx		// 0x00048d20
		mov	edx, dword ptr [ecx + 4]		// 0x00048d23
		mov	edi, dword ptr [ecx]		// 0x00048d26
		and	edx, 0x7fffffff		// 0x00048d28
		and	edi, 0x7fffffff		// 0x00048d2e
		xor	eax, eax		// 0x00048d34
		cmp	edx, edi		// 0x00048d36
		jbe	L48d3f		// 0x00048d38
		mov	eax, 1		// 0x00048d3a
L48d3f:
		mov	edx, dword ptr [ecx + eax*4]		// 0x00048d3f
		mov	ecx, dword ptr [ecx + 8]		// 0x00048d42
		and	edx, 0x7fffffff		// 0x00048d45
		and	ecx, 0x7fffffff		// 0x00048d4b
		cmp	ecx, edx		// 0x00048d51
		ja	L48d6a		// 0x00048d53
		test	eax, eax		// 0x00048d55
		mov	ecx, 2		// 0x00048d57
		jne	L48d65		// 0x00048d5c
		mov	eax, 1		// 0x00048d5e
		jmp	L48d71		// 0x00048d63
L48d65:
		cmp	eax, 1		// 0x00048d65
		je	L48d6f		// 0x00048d68
L48d6a:
		mov	ecx, 1		// 0x00048d6a
L48d6f:
		xor	eax, eax		// 0x00048d6f
L48d71:
		fld	dword ptr [esi + ecx*4]		// 0x00048d71
		mov	edx, dword ptr [esp + 0x24]		// 0x00048d74
		fsub	dword ptr [edx + ecx*4]		// 0x00048d78
		fmul	dword ptr [ebx + eax*4]		// 0x00048d7b
		fld	dword ptr [esi + eax*4]		// 0x00048d7e
		fsub	dword ptr [edx + eax*4]		// 0x00048d81
		fmul	dword ptr [ebx + ecx*4]		// 0x00048d84
		fsubp	st(1), st		// 0x00048d87
		fld	dword ptr [ebp + ecx*4]		// 0x00048d89
		fmul	dword ptr [ebx + eax*4]		// 0x00048d8d
		fld	dword ptr [ebp + eax*4]		// 0x00048d90
		fmul	dword ptr [ebx + ecx*4]		// 0x00048d94
		mov	eax, dword ptr [esp + 0x34]		// 0x00048d97
		fsubp	st(1), st		// 0x00048d9b
		fdivp	st(1), st		// 0x00048d9d
		fld	st(0)		// 0x00048d9f
		fstp	dword ptr [eax]		// 0x00048da1
		fcom	dword ptr kContactZero		// 0x00048da3
		fnstsw	ax		// 0x00048da9
		test	ah, 5		// 0x00048dab
		jnp	L48cae		// 0x00048dae
		fld	st(0)		// 0x00048db4
		mov	eax, dword ptr [esp + 0x28]		// 0x00048db6
		fmul	dword ptr [ebp]		// 0x00048dba
		fld	st(1)		// 0x00048dbd
		fmul	dword ptr [ebp + 4]		// 0x00048dbf
		fstp	dword ptr [esp + 0xc]		// 0x00048dc2
		fxch	st(1)		// 0x00048dc6
		fmul	dword ptr [ebp + 8]		// 0x00048dc8
		fstp	dword ptr [esp + 0x10]		// 0x00048dcb
		fsubr	dword ptr [esi]		// 0x00048dcf
		fst	dword ptr [esi]		// 0x00048dd1
		fld	dword ptr [esi + 4]		// 0x00048dd3
		fsub	dword ptr [esp + 0xc]		// 0x00048dd6
		fst	dword ptr [esi + 4]		// 0x00048dda
		fld	dword ptr [esi + 8]		// 0x00048ddd
		fsub	dword ptr [esp + 0x10]		// 0x00048de0
		fst	dword ptr [esi + 8]		// 0x00048de4
		fld	dword ptr [edx + 8]		// 0x00048de7
		fsub	st, st(1)		// 0x00048dea
		fld	dword ptr [eax + 8]		// 0x00048dec
		fsub	st, st(2)		// 0x00048def
		fmulp	st(1), st		// 0x00048df1
		fld	dword ptr [edx + 4]		// 0x00048df3
		fsub	st, st(3)		// 0x00048df6
		fld	dword ptr [eax + 4]		// 0x00048df8
		fsub	st, st(4)		// 0x00048dfb
		fmulp	st(1), st		// 0x00048dfd
		faddp	st(1), st		// 0x00048dff
		fld	dword ptr [edx]		// 0x00048e01
		fsub	st, st(4)		// 0x00048e03
		fld	dword ptr [eax]		// 0x00048e05
		fsub	st, st(5)		// 0x00048e07
		fmulp	st(1), st		// 0x00048e09
		faddp	st(1), st		// 0x00048e0b
		fcomp	dword ptr kContactZero		// 0x00048e0d
		fstp	st(0)		// 0x00048e13
		fnstsw	ax		// 0x00048e15
		fstp	st(0)		// 0x00048e17
		test	ah, 5		// 0x00048e19
		fstp	st(0)		// 0x00048e1c
		jp	L48cb0		// 0x00048e1e
		pop	edi		// 0x00048e24
		mov	eax, 1		// 0x00048e25
		pop	ebp		// 0x00048e2a
		add	esp, 0x18		// 0x00048e2b
		ret		// 0x00048e2e
		}
	}

// phys_fn_001909 (0x00048e30, 733 B)
// phys_fn_001911 (0x00049110, 2,952 B)
// Contacts between two convex polygons (cdecl, 22 arguments; the row and its
// continuation, which the contract's list lacked): the two polygons' counts,
// vertices, references, poses and planes, the contact normal and the frames,
// the two shapes and the sink, and the ids and feature words 000875 is given
// (0, 0, 0xffff, 0xffff, 0, 0 from 001818). The polygon with more vertices
// sizes the `_chkstk` blocks (12 bytes a vertex); the normal is negated into a
// local and NxFindRotationMatrix takes the given axis to (0, 0, 1); each
// polygon's vertices are projected into that frame (four at a time, then the
// rest). A vertex of one polygon is emitted through 000875 (at its world
// position, with its depth) when its plane value against the other polygon
// passes the tests on the doubles -1e-7 and 1e-7 (0x10107888 / 0x10107880) or
// its crossing parameter is not below 0.0f, and 001903 finds it inside the other
// polygon; then every edge pair is clipped by 001907 and emitted. The vertex
// references are not checked against the vertex arrays.
__declspec(naked) void NxConvexPolygonContacts()
	{
	__asm
		{
		push	ebp		// 0x00048e30
		lea	ebp, [esp - 0x20]		// 0x00048e31
		sub	esp, 0xcc		// 0x00048e35
		mov	eax, dword ptr [ebp + 0x50]		// 0x00048e3b
		fld	dword ptr [eax]		// 0x00048e3e
		push	ebx		// 0x00048e40
		fchs		// 0x00048e41
		push	esi		// 0x00048e43
		fstp	dword ptr [ebp - 0x3c]		// 0x00048e44
		push	edi		// 0x00048e47
		fld	dword ptr [eax + 4]		// 0x00048e48
		mov	edi, dword ptr [ebp + 0x3c]		// 0x00048e4b
		fchs		// 0x00048e4e
		fstp	dword ptr [ebp - 0x38]		// 0x00048e50
		fld	dword ptr [eax + 8]		// 0x00048e53
		mov	eax, dword ptr [ebp + 0x28]		// 0x00048e56
		cmp	eax, edi		// 0x00048e59
		fchs		// 0x00048e5b
		fstp	dword ptr [ebp - 0x34]		// 0x00048e5d
		ja	L48e64		// 0x00048e60
		mov	eax, edi		// 0x00048e62
L48e64:
		lea	eax, [eax + eax*2]		// 0x00048e64
		shl	eax, 2		// 0x00048e67
		add	eax, 3		// 0x00048e6a
		and	eax, 0xfffffffc		// 0x00048e6d
		call	_chkstk		// 0x00048e70
		mov	edx, dword ptr [ebp + 0x4c]		// 0x00048e75
		mov	esi, esp		// 0x00048e78
		lea	eax, [ebp - 0x10]		// 0x00048e7a
		push	eax		// 0x00048e7d
		lea	ecx, [ebp - 0x48]		// 0x00048e7e
		push	ecx		// 0x00048e81
		push	edx		// 0x00048e82
		mov	dword ptr [ebp - 0x14], esi		// 0x00048e83
		mov	dword ptr [ebp - 0x48], 0		// 0x00048e86
		mov	dword ptr [ebp - 0x44], 0		// 0x00048e8d
		mov	dword ptr [ebp - 0x40], 0x3f800000		// 0x00048e94
		call	dword ptr _imp__NxFindRotationMatrix		// 0x00048e9b
		add	esp, 0xc		// 0x00048ea1
		xor	ebx, ebx		// 0x00048ea4
		cmp	edi, 4		// 0x00048ea6
		jl	L48fc2		// 0x00048ea9
		mov	edx, dword ptr [ebp + 0x44]		// 0x00048eaf
		add	edi, -4		// 0x00048eb2
		shr	edi, 2		// 0x00048eb5
		add	edx, 8		// 0x00048eb8
		inc	edi		// 0x00048ebb
		lea	eax, [esi + 0x1c]		// 0x00048ebc
		lea	ebx, [edi*4]		// 0x00048ebf
L48ec6:
		mov	ecx, dword ptr [edx - 8]		// 0x00048ec6
		fld	dword ptr [ebp - 0xc]		// 0x00048ec9
		mov	esi, dword ptr [ebp + 0x40]		// 0x00048ecc
		lea	ecx, [ecx + ecx*2]		// 0x00048ecf
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00048ed2
		lea	ecx, [esi + ecx*4]		// 0x00048ed6
		fld	dword ptr [ebp - 8]		// 0x00048ed9
		fmul	dword ptr [ecx + 8]		// 0x00048edc
		faddp	st(1), st		// 0x00048edf
		fld	dword ptr [ebp - 0x10]		// 0x00048ee1
		fmul	dword ptr [ecx]		// 0x00048ee4
		faddp	st(1), st		// 0x00048ee6
		fstp	dword ptr [eax - 0x1c]		// 0x00048ee8
		fld	dword ptr [ebp - 4]		// 0x00048eeb
		fmul	dword ptr [ecx]		// 0x00048eee
		fld	dword ptr [ebp]		// 0x00048ef0
		fmul	dword ptr [ecx + 4]		// 0x00048ef3
		faddp	st(1), st		// 0x00048ef6
		fld	dword ptr [ebp + 4]		// 0x00048ef8
		fmul	dword ptr [ecx + 8]		// 0x00048efb
		faddp	st(1), st		// 0x00048efe
		fstp	dword ptr [eax - 0x18]		// 0x00048f00
		mov	ecx, dword ptr [edx - 4]		// 0x00048f03
		fld	dword ptr [ebp - 0xc]		// 0x00048f06
		lea	ecx, [ecx + ecx*2]		// 0x00048f09
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00048f0c
		lea	ecx, [esi + ecx*4]		// 0x00048f10
		fld	dword ptr [ebp - 8]		// 0x00048f13
		fmul	dword ptr [ecx + 8]		// 0x00048f16
		faddp	st(1), st		// 0x00048f19
		fld	dword ptr [ebp - 0x10]		// 0x00048f1b
		fmul	dword ptr [ecx]		// 0x00048f1e
		faddp	st(1), st		// 0x00048f20
		fstp	dword ptr [eax - 0x10]		// 0x00048f22
		fld	dword ptr [ebp - 4]		// 0x00048f25
		fmul	dword ptr [ecx]		// 0x00048f28
		fld	dword ptr [ebp]		// 0x00048f2a
		fmul	dword ptr [ecx + 4]		// 0x00048f2d
		faddp	st(1), st		// 0x00048f30
		fld	dword ptr [ebp + 4]		// 0x00048f32
		fmul	dword ptr [ecx + 8]		// 0x00048f35
		faddp	st(1), st		// 0x00048f38
		fstp	dword ptr [eax - 0xc]		// 0x00048f3a
		mov	ecx, dword ptr [edx]		// 0x00048f3d
		fld	dword ptr [ebp - 0xc]		// 0x00048f3f
		lea	ecx, [ecx + ecx*2]		// 0x00048f42
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00048f45
		lea	ecx, [esi + ecx*4]		// 0x00048f49
		fld	dword ptr [ebp - 8]		// 0x00048f4c
		fmul	dword ptr [ecx + 8]		// 0x00048f4f
		faddp	st(1), st		// 0x00048f52
		fld	dword ptr [ebp - 0x10]		// 0x00048f54
		fmul	dword ptr [ecx]		// 0x00048f57
		faddp	st(1), st		// 0x00048f59
		fstp	dword ptr [eax - 4]		// 0x00048f5b
		fld	dword ptr [ebp - 4]		// 0x00048f5e
		fmul	dword ptr [ecx]		// 0x00048f61
		fld	dword ptr [ebp]		// 0x00048f63
		fmul	dword ptr [ecx + 4]		// 0x00048f66
		faddp	st(1), st		// 0x00048f69
		fld	dword ptr [ebp + 4]		// 0x00048f6b
		fmul	dword ptr [ecx + 8]		// 0x00048f6e
		faddp	st(1), st		// 0x00048f71
		fstp	dword ptr [eax]		// 0x00048f73
		mov	ecx, dword ptr [edx + 4]		// 0x00048f75
		fld	dword ptr [ebp - 0xc]		// 0x00048f78
		lea	ecx, [ecx + ecx*2]		// 0x00048f7b
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00048f7e
		lea	ecx, [esi + ecx*4]		// 0x00048f82
		fld	dword ptr [ebp - 8]		// 0x00048f85
		fmul	dword ptr [ecx + 8]		// 0x00048f88
		faddp	st(1), st		// 0x00048f8b
		fld	dword ptr [ebp - 0x10]		// 0x00048f8d
		fmul	dword ptr [ecx]		// 0x00048f90
		faddp	st(1), st		// 0x00048f92
		fstp	dword ptr [eax + 8]		// 0x00048f94
		fld	dword ptr [ebp - 4]		// 0x00048f97
		fmul	dword ptr [ecx]		// 0x00048f9a
		fld	dword ptr [ebp]		// 0x00048f9c
		fmul	dword ptr [ecx + 4]		// 0x00048f9f
		faddp	st(1), st		// 0x00048fa2
		fld	dword ptr [ebp + 4]		// 0x00048fa4
		add	edx, 0x10		// 0x00048fa7
		fmul	dword ptr [ecx + 8]		// 0x00048faa
		add	eax, 0x30		// 0x00048fad
		dec	edi		// 0x00048fb0
		faddp	st(1), st		// 0x00048fb1
		fstp	dword ptr [eax - 0x24]		// 0x00048fb3
		jne	L48ec6		// 0x00048fb6
		mov	esi, dword ptr [ebp - 0x14]		// 0x00048fbc
		mov	edi, dword ptr [ebp + 0x3c]		// 0x00048fbf
L48fc2:
		cmp	ebx, edi		// 0x00048fc2
		jae	L49018		// 0x00048fc4
		lea	edx, [ebx + ebx*2]		// 0x00048fc6
		lea	ecx, [esi + edx*4]		// 0x00048fc9
		_emit	0x8d
		_emit	0x64
		_emit	0x24
		_emit	0x00		// 0x00048fcc lea esp, [esp]
L48fd0:
		mov	eax, dword ptr [ebp + 0x44]		// 0x00048fd0
		fld	dword ptr [ebp - 0xc]		// 0x00048fd3
		mov	eax, dword ptr [eax + ebx*4]		// 0x00048fd6
		lea	edx, [eax + eax*2]		// 0x00048fd9
		mov	eax, dword ptr [ebp + 0x40]		// 0x00048fdc
		fmul	dword ptr [eax + edx*4 + 4]		// 0x00048fdf
		lea	eax, [eax + edx*4]		// 0x00048fe3
		fld	dword ptr [ebp - 8]		// 0x00048fe6
		inc	ebx		// 0x00048fe9
		fmul	dword ptr [eax + 8]		// 0x00048fea
		add	ecx, 0xc		// 0x00048fed
		cmp	ebx, edi		// 0x00048ff0
		faddp	st(1), st		// 0x00048ff2
		fld	dword ptr [ebp - 0x10]		// 0x00048ff4
		fmul	dword ptr [eax]		// 0x00048ff7
		faddp	st(1), st		// 0x00048ff9
		fstp	dword ptr [ecx - 0xc]		// 0x00048ffb
		fld	dword ptr [ebp - 4]		// 0x00048ffe
		fmul	dword ptr [eax]		// 0x00049001
		fld	dword ptr [ebp]		// 0x00049003
		fmul	dword ptr [eax + 4]		// 0x00049006
		faddp	st(1), st		// 0x00049009
		fld	dword ptr [ebp + 4]		// 0x0004900b
		fmul	dword ptr [eax + 8]		// 0x0004900e
		faddp	st(1), st		// 0x00049011
		fstp	dword ptr [ecx - 8]		// 0x00049013
		jb	L48fd0		// 0x00049016
L49018:
		mov	ecx, dword ptr [ebp + 0x44]		// 0x00049018
		fld	dword ptr [ebp + 0xc]		// 0x0004901b
		mov	eax, dword ptr [ecx]		// 0x0004901e
		mov	esi, dword ptr [ebp + 0x54]		// 0x00049020
		lea	edx, [eax + eax*2]		// 0x00049023
		mov	eax, dword ptr [ebp + 0x40]		// 0x00049026
		fmul	dword ptr [eax + edx*4 + 4]		// 0x00049029
		lea	eax, [eax + edx*4]		// 0x0004902d
		fld	dword ptr [ebp + 0x10]		// 0x00049030
		mov	ebx, dword ptr [ebp + 0x48]		// 0x00049033
		fmul	dword ptr [eax + 8]		// 0x00049036
		faddp	st(1), st		// 0x00049039
		fld	dword ptr [ebp + 8]		// 0x0004903b
		fmul	dword ptr [eax]		// 0x0004903e
		xor	eax, eax		// 0x00049040
		mov	dword ptr [ebp + 0x54], eax		// 0x00049042
		faddp	st(1), st		// 0x00049045
		fstp	dword ptr [ebp - 0x58]		// 0x00049047
		fld	dword ptr [ebp + 0xc]		// 0x0004904a
		fmul	dword ptr [esi + 4]		// 0x0004904d
		fld	dword ptr [ebp + 0x10]		// 0x00049050
		fmul	dword ptr [esi + 8]		// 0x00049053
		faddp	st(1), st		// 0x00049056
		fld	dword ptr [ebp + 8]		// 0x00049058
		fmul	dword ptr [esi]		// 0x0004905b
		faddp	st(1), st		// 0x0004905d
		fstp	dword ptr [ebp - 0x98]		// 0x0004905f
		fld	dword ptr [ebp + 0x10]		// 0x00049065
		fmul	dword ptr [esi + 0x18]		// 0x00049068
		fld	dword ptr [ebp + 8]		// 0x0004906b
		fmul	dword ptr [esi + 0x10]		// 0x0004906e
		faddp	st(1), st		// 0x00049071
		fld	dword ptr [ebp + 0xc]		// 0x00049073
		fmul	dword ptr [esi + 0x14]		// 0x00049076
		faddp	st(1), st		// 0x00049079
		fstp	dword ptr [ebp - 0x88]		// 0x0004907b
		fld	dword ptr [ebp + 8]		// 0x00049081
		fmul	dword ptr [esi + 0x20]		// 0x00049084
		fld	dword ptr [ebp + 0xc]		// 0x00049087
		fmul	dword ptr [esi + 0x24]		// 0x0004908a
		faddp	st(1), st		// 0x0004908d
		fld	dword ptr [ebp + 0x10]		// 0x0004908f
		fmul	dword ptr [esi + 0x28]		// 0x00049092
		faddp	st(1), st		// 0x00049095
		fstp	dword ptr [ebp - 0x78]		// 0x00049097
		fld	dword ptr [ebp + 0x10]		// 0x0004909a
		fmul	dword ptr [esi + 0x38]		// 0x0004909d
		fld	dword ptr [ebp + 8]		// 0x000490a0
		fmul	dword ptr [esi + 0x30]		// 0x000490a3
		faddp	st(1), st		// 0x000490a6
		fld	dword ptr [ebp + 0xc]		// 0x000490a8
		fmul	dword ptr [esi + 0x34]		// 0x000490ab
		faddp	st(1), st		// 0x000490ae
		fstp	dword ptr [ebp - 0x68]		// 0x000490b0
		fld	dword ptr [ebp - 0x34]		// 0x000490b3
		fmul	dword ptr [ebx + 8]		// 0x000490b6
		fld	dword ptr [ebp - 0x38]		// 0x000490b9
		fmul	dword ptr [ebx + 4]		// 0x000490bc
		faddp	st(1), st		// 0x000490bf
		fld	dword ptr [ebp - 0x3c]		// 0x000490c1
		fmul	dword ptr [ebx]		// 0x000490c4
		faddp	st(1), st		// 0x000490c6
		fstp	dword ptr [ebp - 0x20]		// 0x000490c8
		fld	dword ptr [ebp - 0x34]		// 0x000490cb
		fmul	dword ptr [ebx + 0x18]		// 0x000490ce
		fld	dword ptr [ebp - 0x3c]		// 0x000490d1
		fmul	dword ptr [ebx + 0x10]		// 0x000490d4
		faddp	st(1), st		// 0x000490d7
		fld	dword ptr [ebp - 0x38]		// 0x000490d9
		fmul	dword ptr [ebx + 0x14]		// 0x000490dc
		faddp	st(1), st		// 0x000490df
		fstp	dword ptr [ebp - 0x1c]		// 0x000490e1
		fld	dword ptr [ebp - 0x3c]		// 0x000490e4
		fmul	dword ptr [ebx + 0x20]		// 0x000490e7
		fld	dword ptr [ebp - 0x38]		// 0x000490ea
		fmul	dword ptr [ebx + 0x24]		// 0x000490ed
		faddp	st(1), st		// 0x000490f0
		fld	dword ptr [ebp - 0x34]		// 0x000490f2
		fmul	dword ptr [ebx + 0x28]		// 0x000490f5
		faddp	st(1), st		// 0x000490f8
		fstp	dword ptr [ebp - 0x18]		// 0x000490fa
		mov	ecx, dword ptr [ebp + 0x28]		// 0x000490fd
		test	ecx, ecx		// 0x00049100
		mov	edi, dword ptr [ebp + 0x34]		// 0x00049102
		jbe	L491f1		// 0x00049105
		jmp	L49110		// 0x0004910b
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x0004910d lea ecx, [ecx]
L49110:
		mov	ecx, dword ptr [ebp + 0x30]		// 0x00049110
		fld	dword ptr [ebp - 0x78]		// 0x00049113
		mov	eax, dword ptr [ecx + eax*4]		// 0x00049116
		lea	edx, [eax + eax*2]		// 0x00049119
		mov	eax, dword ptr [ebp + 0x2c]		// 0x0004911c
		fmul	dword ptr [eax + edx*4 + 8]		// 0x0004911f
		lea	ecx, [eax + edx*4]		// 0x00049123
		fld	dword ptr [ebp - 0x88]		// 0x00049126
		mov	dword ptr [ebp + 0x48], ecx		// 0x0004912c
		fmul	dword ptr [ecx + 4]		// 0x0004912f
		faddp	st(1), st		// 0x00049132
		fld	dword ptr [ebp - 0x98]		// 0x00049134
		fmul	dword ptr [ecx]		// 0x0004913a
		faddp	st(1), st		// 0x0004913c
		fadd	dword ptr [ebp - 0x68]		// 0x0004913e
		fcomp	dword ptr [ebp - 0x58]		// 0x00049141
		fnstsw	ax		// 0x00049144
		test	ah, 5		// 0x00049146
		jp	L491df		// 0x00049149
		fld	dword ptr [ecx]		// 0x0004914f
		fmul	dword ptr [esi]		// 0x00049151
		fld	dword ptr [ecx + 4]		// 0x00049153
		fmul	dword ptr [esi + 0x10]		// 0x00049156
		faddp	st(1), st		// 0x00049159
		fld	dword ptr [esi + 0x20]		// 0x0004915b
		fmul	dword ptr [ecx + 8]		// 0x0004915e
		faddp	st(1), st		// 0x00049161
		fadd	dword ptr [esi + 0x30]		// 0x00049163
		fstp	dword ptr [ebp + 0x14]		// 0x00049166
		fld	dword ptr [ecx]		// 0x00049169
		fmul	dword ptr [esi + 4]		// 0x0004916b
		fld	dword ptr [ecx + 4]		// 0x0004916e
		fmul	dword ptr [esi + 0x14]		// 0x00049171
		faddp	st(1), st		// 0x00049174
		fld	dword ptr [esi + 0x24]		// 0x00049176
		fmul	dword ptr [ecx + 8]		// 0x00049179
		faddp	st(1), st		// 0x0004917c
		fadd	dword ptr [esi + 0x34]		// 0x0004917e
		fstp	dword ptr [ebp + 0x18]		// 0x00049181
		fld	dword ptr [ecx + 8]		// 0x00049184
		fmul	dword ptr [esi + 0x28]		// 0x00049187
		fld	dword ptr [ecx + 4]		// 0x0004918a
		fmul	dword ptr [esi + 0x18]		// 0x0004918d
		faddp	st(1), st		// 0x00049190
		fld	dword ptr [ecx]		// 0x00049192
		mov	ecx, dword ptr [ebp + 0x4c]		// 0x00049194
		fmul	dword ptr [esi + 8]		// 0x00049197
		faddp	st(1), st		// 0x0004919a
		fadd	dword ptr [esi + 0x38]		// 0x0004919c
		fstp	dword ptr [ebp + 0x1c]		// 0x0004919f
		fld	dword ptr [ebp - 0x1c]		// 0x000491a2
		fmul	dword ptr [ecx + 4]		// 0x000491a5
		fld	dword ptr [ebp - 0x18]		// 0x000491a8
		fmul	dword ptr [ecx + 8]		// 0x000491ab
		faddp	st(1), st		// 0x000491ae
		fld	dword ptr [ebp - 0x20]		// 0x000491b0
		fmul	dword ptr [ecx]		// 0x000491b3
		faddp	st(1), st		// 0x000491b5
		fld	st(0)		// 0x000491b7
		fld	qword ptr kContactMinusTenth7		// 0x000491b9
		fcomp	st(1)		// 0x000491bf
		fnstsw	ax		// 0x000491c1
		test	ah, 5		// 0x000491c3
		jp	L49921		// 0x000491c6
		fcomp	qword ptr kContactTenth7		// 0x000491cc
		fnstsw	ax		// 0x000491d2
		test	ah, 5		// 0x000491d4
		jp	L49923		// 0x000491d7
		fstp	st(0)		// 0x000491dd
L491df:
		mov	eax, dword ptr [ebp + 0x54]		// 0x000491df
		mov	ecx, dword ptr [ebp + 0x28]		// 0x000491e2
		inc	eax		// 0x000491e5
		cmp	eax, ecx		// 0x000491e6
		mov	dword ptr [ebp + 0x54], eax		// 0x000491e8
		jb	L49110		// 0x000491eb
L491f1:
		mov	ecx, dword ptr [ebp + 0x38]		// 0x000491f1
		lea	edx, [ebp - 0x10]		// 0x000491f4
		push	edx		// 0x000491f7
		lea	eax, [ebp - 0x54]		// 0x000491f8
		push	eax		// 0x000491fb
		push	ecx		// 0x000491fc
		mov	dword ptr [ebp - 0x54], 0		// 0x000491fd
		mov	dword ptr [ebp - 0x50], 0		// 0x00049204
		mov	dword ptr [ebp - 0x4c], 0x3f800000		// 0x0004920b
		call	dword ptr _imp__NxFindRotationMatrix		// 0x00049212
		mov	ecx, dword ptr [ebp + 0x28]		// 0x00049218
		add	esp, 0xc		// 0x0004921b
		xor	edx, edx		// 0x0004921e
		cmp	ecx, 4		// 0x00049220
		jl	L49347		// 0x00049223
		mov	edx, dword ptr [ebp + 0x30]		// 0x00049229
		mov	eax, dword ptr [ebp - 0x14]		// 0x0004922c
		add	ecx, -4		// 0x0004922f
		shr	ecx, 2		// 0x00049232
		add	edx, 8		// 0x00049235
		add	eax, 0x1c		// 0x00049238
		inc	ecx		// 0x0004923b
		mov	dword ptr [ebp + 0x50], ecx		// 0x0004923c
		shl	ecx, 2		// 0x0004923f
		mov	dword ptr [ebp + 0x54], ecx		// 0x00049242
L49245:
		mov	ecx, dword ptr [edx - 8]		// 0x00049245
		fld	dword ptr [ebp - 0xc]		// 0x00049248
		mov	esi, dword ptr [ebp + 0x2c]		// 0x0004924b
		lea	ecx, [ecx + ecx*2]		// 0x0004924e
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00049251
		lea	ecx, [esi + ecx*4]		// 0x00049255
		fld	dword ptr [ebp - 8]		// 0x00049258
		fmul	dword ptr [ecx + 8]		// 0x0004925b
		faddp	st(1), st		// 0x0004925e
		fld	dword ptr [ebp - 0x10]		// 0x00049260
		fmul	dword ptr [ecx]		// 0x00049263
		faddp	st(1), st		// 0x00049265
		fstp	dword ptr [eax - 0x1c]		// 0x00049267
		fld	dword ptr [ebp - 4]		// 0x0004926a
		fmul	dword ptr [ecx]		// 0x0004926d
		fld	dword ptr [ebp]		// 0x0004926f
		fmul	dword ptr [ecx + 4]		// 0x00049272
		faddp	st(1), st		// 0x00049275
		fld	dword ptr [ebp + 4]		// 0x00049277
		fmul	dword ptr [ecx + 8]		// 0x0004927a
		faddp	st(1), st		// 0x0004927d
		fstp	dword ptr [eax - 0x18]		// 0x0004927f
		mov	ecx, dword ptr [edx - 4]		// 0x00049282
		fld	dword ptr [ebp - 0xc]		// 0x00049285
		lea	ecx, [ecx + ecx*2]		// 0x00049288
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x0004928b
		lea	ecx, [esi + ecx*4]		// 0x0004928f
		fld	dword ptr [ebp - 8]		// 0x00049292
		fmul	dword ptr [ecx + 8]		// 0x00049295
		faddp	st(1), st		// 0x00049298
		fld	dword ptr [ebp - 0x10]		// 0x0004929a
		fmul	dword ptr [ecx]		// 0x0004929d
		faddp	st(1), st		// 0x0004929f
		fstp	dword ptr [eax - 0x10]		// 0x000492a1
		fld	dword ptr [ebp - 4]		// 0x000492a4
		fmul	dword ptr [ecx]		// 0x000492a7
		fld	dword ptr [ebp]		// 0x000492a9
		fmul	dword ptr [ecx + 4]		// 0x000492ac
		faddp	st(1), st		// 0x000492af
		fld	dword ptr [ebp + 4]		// 0x000492b1
		fmul	dword ptr [ecx + 8]		// 0x000492b4
		faddp	st(1), st		// 0x000492b7
		fstp	dword ptr [eax - 0xc]		// 0x000492b9
		mov	ecx, dword ptr [edx]		// 0x000492bc
		fld	dword ptr [ebp - 0xc]		// 0x000492be
		lea	ecx, [ecx + ecx*2]		// 0x000492c1
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x000492c4
		lea	ecx, [esi + ecx*4]		// 0x000492c8
		fld	dword ptr [ebp - 8]		// 0x000492cb
		fmul	dword ptr [ecx + 8]		// 0x000492ce
		faddp	st(1), st		// 0x000492d1
		fld	dword ptr [ebp - 0x10]		// 0x000492d3
		fmul	dword ptr [ecx]		// 0x000492d6
		faddp	st(1), st		// 0x000492d8
		fstp	dword ptr [eax - 4]		// 0x000492da
		fld	dword ptr [ebp - 4]		// 0x000492dd
		fmul	dword ptr [ecx]		// 0x000492e0
		fld	dword ptr [ebp]		// 0x000492e2
		fmul	dword ptr [ecx + 4]		// 0x000492e5
		faddp	st(1), st		// 0x000492e8
		fld	dword ptr [ebp + 4]		// 0x000492ea
		fmul	dword ptr [ecx + 8]		// 0x000492ed
		faddp	st(1), st		// 0x000492f0
		fstp	dword ptr [eax]		// 0x000492f2
		mov	ecx, dword ptr [edx + 4]		// 0x000492f4
		fld	dword ptr [ebp - 0xc]		// 0x000492f7
		lea	ecx, [ecx + ecx*2]		// 0x000492fa
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x000492fd
		lea	ecx, [esi + ecx*4]		// 0x00049301
		fld	dword ptr [ebp - 8]		// 0x00049304
		fmul	dword ptr [ecx + 8]		// 0x00049307
		faddp	st(1), st		// 0x0004930a
		fld	dword ptr [ebp - 0x10]		// 0x0004930c
		fmul	dword ptr [ecx]		// 0x0004930f
		faddp	st(1), st		// 0x00049311
		fstp	dword ptr [eax + 8]		// 0x00049313
		fld	dword ptr [ebp - 4]		// 0x00049316
		fmul	dword ptr [ecx]		// 0x00049319
		fld	dword ptr [ebp]		// 0x0004931b
		fmul	dword ptr [ecx + 4]		// 0x0004931e
		faddp	st(1), st		// 0x00049321
		fld	dword ptr [ebp + 4]		// 0x00049323
		add	edx, 0x10		// 0x00049326
		fmul	dword ptr [ecx + 8]		// 0x00049329
		mov	ecx, dword ptr [ebp + 0x50]		// 0x0004932c
		add	eax, 0x30		// 0x0004932f
		dec	ecx		// 0x00049332
		faddp	st(1), st		// 0x00049333
		mov	dword ptr [ebp + 0x50], ecx		// 0x00049335
		fstp	dword ptr [eax - 0x24]		// 0x00049338
		jne	L49245		// 0x0004933b
		mov	ecx, dword ptr [ebp + 0x28]		// 0x00049341
		mov	edx, dword ptr [ebp + 0x54]		// 0x00049344
L49347:
		cmp	edx, ecx		// 0x00049347
		jae	L4939f		// 0x00049349
		mov	ecx, dword ptr [ebp - 0x14]		// 0x0004934b
		lea	eax, [edx + edx*2]		// 0x0004934e
		lea	ecx, [ecx + eax*4]		// 0x00049351
L49354:
		mov	eax, dword ptr [ebp + 0x30]		// 0x00049354
		fld	dword ptr [ebp - 0xc]		// 0x00049357
		mov	eax, dword ptr [eax + edx*4]		// 0x0004935a
		mov	esi, dword ptr [ebp + 0x2c]		// 0x0004935d
		lea	eax, [eax + eax*2]		// 0x00049360
		fmul	dword ptr [esi + eax*4 + 4]		// 0x00049363
		lea	eax, [esi + eax*4]		// 0x00049367
		fld	dword ptr [ebp - 8]		// 0x0004936a
		inc	edx		// 0x0004936d
		fmul	dword ptr [eax + 8]		// 0x0004936e
		add	ecx, 0xc		// 0x00049371
		faddp	st(1), st		// 0x00049374
		fld	dword ptr [ebp - 0x10]		// 0x00049376
		fmul	dword ptr [eax]		// 0x00049379
		faddp	st(1), st		// 0x0004937b
		fstp	dword ptr [ecx - 0xc]		// 0x0004937d
		fld	dword ptr [ebp - 4]		// 0x00049380
		fmul	dword ptr [eax]		// 0x00049383
		fld	dword ptr [ebp]		// 0x00049385
		fmul	dword ptr [eax + 4]		// 0x00049388
		faddp	st(1), st		// 0x0004938b
		fld	dword ptr [ebp + 4]		// 0x0004938d
		fmul	dword ptr [eax + 8]		// 0x00049390
		mov	eax, dword ptr [ebp + 0x28]		// 0x00049393
		cmp	edx, eax		// 0x00049396
		faddp	st(1), st		// 0x00049398
		fstp	dword ptr [ecx - 8]		// 0x0004939a
		jb	L49354		// 0x0004939d
L4939f:
		mov	ecx, dword ptr [ebp + 0x30]		// 0x0004939f
		fld	dword ptr [ebp + 0xc]		// 0x000493a2
		mov	eax, dword ptr [ecx]		// 0x000493a5
		mov	esi, dword ptr [ebp + 0x58]		// 0x000493a7
		lea	edx, [eax + eax*2]		// 0x000493aa
		mov	eax, dword ptr [ebp + 0x2c]		// 0x000493ad
		fmul	dword ptr [eax + edx*4 + 4]		// 0x000493b0
		lea	eax, [eax + edx*4]		// 0x000493b4
		fld	dword ptr [ebp + 0x10]		// 0x000493b7
		fmul	dword ptr [eax + 8]		// 0x000493ba
		faddp	st(1), st		// 0x000493bd
		fld	dword ptr [ebp + 8]		// 0x000493bf
		fmul	dword ptr [eax]		// 0x000493c2
		faddp	st(1), st		// 0x000493c4
		fstp	dword ptr [ebp + 0x50]		// 0x000493c6
		fld	dword ptr [ebp - 0x10]		// 0x000493c9
		fmul	dword ptr [esi]		// 0x000493cc
		fld	dword ptr [ebp - 0xc]		// 0x000493ce
		fmul	dword ptr [esi + 4]		// 0x000493d1
		faddp	st(1), st		// 0x000493d4
		fld	dword ptr [ebp - 8]		// 0x000493d6
		fmul	dword ptr [esi + 8]		// 0x000493d9
		faddp	st(1), st		// 0x000493dc
		fstp	dword ptr [ebp - 0xa0]		// 0x000493de
		fld	dword ptr [ebp]		// 0x000493e4
		fmul	dword ptr [esi + 4]		// 0x000493e7
		fld	dword ptr [ebp + 4]		// 0x000493ea
		fmul	dword ptr [esi + 8]		// 0x000493ed
		faddp	st(1), st		// 0x000493f0
		fld	dword ptr [ebp - 4]		// 0x000493f2
		fmul	dword ptr [esi]		// 0x000493f5
		faddp	st(1), st		// 0x000493f7
		fstp	dword ptr [ebp - 0x9c]		// 0x000493f9
		fld	dword ptr [ebp + 0xc]		// 0x000493ff
		fmul	dword ptr [esi + 4]		// 0x00049402
		fld	dword ptr [ebp + 0x10]		// 0x00049405
		fmul	dword ptr [esi + 8]		// 0x00049408
		faddp	st(1), st		// 0x0004940b
		fld	dword ptr [ebp + 8]		// 0x0004940d
		fmul	dword ptr [esi]		// 0x00049410
		faddp	st(1), st		// 0x00049412
		fstp	dword ptr [ebp - 0x98]		// 0x00049414
		fld	dword ptr [ebp - 0x10]		// 0x0004941a
		fmul	dword ptr [esi + 0x10]		// 0x0004941d
		fld	dword ptr [ebp - 8]		// 0x00049420
		fmul	dword ptr [esi + 0x18]		// 0x00049423
		faddp	st(1), st		// 0x00049426
		fld	dword ptr [ebp - 0xc]		// 0x00049428
		fmul	dword ptr [esi + 0x14]		// 0x0004942b
		faddp	st(1), st		// 0x0004942e
		fstp	dword ptr [ebp - 0x90]		// 0x00049430
		fld	dword ptr [ebp + 4]		// 0x00049436
		fmul	dword ptr [esi + 0x18]		// 0x00049439
		fld	dword ptr [ebp - 4]		// 0x0004943c
		fmul	dword ptr [esi + 0x10]		// 0x0004943f
		faddp	st(1), st		// 0x00049442
		fld	dword ptr [ebp]		// 0x00049444
		fmul	dword ptr [esi + 0x14]		// 0x00049447
		faddp	st(1), st		// 0x0004944a
		fstp	dword ptr [ebp - 0x8c]		// 0x0004944c
		fld	dword ptr [ebp + 0x10]		// 0x00049452
		fmul	dword ptr [esi + 0x18]		// 0x00049455
		fld	dword ptr [ebp + 8]		// 0x00049458
		fmul	dword ptr [esi + 0x10]		// 0x0004945b
		faddp	st(1), st		// 0x0004945e
		fld	dword ptr [ebp + 0xc]		// 0x00049460
		fmul	dword ptr [esi + 0x14]		// 0x00049463
		faddp	st(1), st		// 0x00049466
		fstp	dword ptr [ebp - 0x88]		// 0x00049468
		fld	dword ptr [ebp - 0x10]		// 0x0004946e
		fmul	dword ptr [esi + 0x20]		// 0x00049471
		fld	dword ptr [ebp - 8]		// 0x00049474
		fmul	dword ptr [esi + 0x28]		// 0x00049477
		faddp	st(1), st		// 0x0004947a
		fld	dword ptr [ebp - 0xc]		// 0x0004947c
		fmul	dword ptr [esi + 0x24]		// 0x0004947f
		faddp	st(1), st		// 0x00049482
		fstp	dword ptr [ebp - 0x80]		// 0x00049484
		fld	dword ptr [ebp + 4]		// 0x00049487
		fmul	dword ptr [esi + 0x28]		// 0x0004948a
		fld	dword ptr [ebp - 4]		// 0x0004948d
		fmul	dword ptr [esi + 0x20]		// 0x00049490
		mov	ecx, dword ptr [ebp + 0x3c]		// 0x00049493
		xor	eax, eax		// 0x00049496
		test	ecx, ecx		// 0x00049498
		faddp	st(1), st		// 0x0004949a
		mov	dword ptr [ebp + 0x54], eax		// 0x0004949c
		fld	dword ptr [ebp]		// 0x0004949f
		fmul	dword ptr [esi + 0x24]		// 0x000494a2
		faddp	st(1), st		// 0x000494a5
		fstp	dword ptr [ebp - 0x7c]		// 0x000494a7
		fld	dword ptr [ebp + 0x10]		// 0x000494aa
		fmul	dword ptr [esi + 0x28]		// 0x000494ad
		fld	dword ptr [ebp + 8]		// 0x000494b0
		fmul	dword ptr [esi + 0x20]		// 0x000494b3
		faddp	st(1), st		// 0x000494b6
		fld	dword ptr [ebp + 0xc]		// 0x000494b8
		fmul	dword ptr [esi + 0x24]		// 0x000494bb
		faddp	st(1), st		// 0x000494be
		fstp	dword ptr [ebp - 0x78]		// 0x000494c0
		fld	dword ptr [ebp - 0x10]		// 0x000494c3
		fmul	dword ptr [esi + 0x30]		// 0x000494c6
		fld	dword ptr [ebp - 8]		// 0x000494c9
		fmul	dword ptr [esi + 0x38]		// 0x000494cc
		faddp	st(1), st		// 0x000494cf
		fld	dword ptr [ebp - 0xc]		// 0x000494d1
		fmul	dword ptr [esi + 0x34]		// 0x000494d4
		faddp	st(1), st		// 0x000494d7
		fstp	dword ptr [ebp - 0x70]		// 0x000494d9
		fld	dword ptr [ebp + 4]		// 0x000494dc
		fmul	dword ptr [esi + 0x38]		// 0x000494df
		fld	dword ptr [ebp - 4]		// 0x000494e2
		fmul	dword ptr [esi + 0x30]		// 0x000494e5
		faddp	st(1), st		// 0x000494e8
		fld	dword ptr [ebp]		// 0x000494ea
		fmul	dword ptr [esi + 0x34]		// 0x000494ed
		faddp	st(1), st		// 0x000494f0
		fstp	dword ptr [ebp - 0x6c]		// 0x000494f2
		fld	dword ptr [ebp + 0x10]		// 0x000494f5
		fmul	dword ptr [esi + 0x38]		// 0x000494f8
		fld	dword ptr [ebp + 8]		// 0x000494fb
		fmul	dword ptr [esi + 0x30]		// 0x000494fe
		faddp	st(1), st		// 0x00049501
		fld	dword ptr [ebp + 0xc]		// 0x00049503
		fmul	dword ptr [esi + 0x34]		// 0x00049506
		faddp	st(1), st		// 0x00049509
		fstp	dword ptr [ebp - 0x68]		// 0x0004950b
		jbe	L49659		// 0x0004950e
L49514:
		mov	ecx, dword ptr [ebp + 0x44]		// 0x00049514
		fld	dword ptr [ebp - 0x78]		// 0x00049517
		mov	eax, dword ptr [ecx + eax*4]		// 0x0004951a
		lea	edx, [eax + eax*2]		// 0x0004951d
		mov	eax, dword ptr [ebp + 0x40]		// 0x00049520
		fmul	dword ptr [eax + edx*4 + 8]		// 0x00049523
		lea	ecx, [eax + edx*4]		// 0x00049527
		fld	dword ptr [ebp - 0x88]		// 0x0004952a
		mov	dword ptr [ebp + 0x48], ecx		// 0x00049530
		fmul	dword ptr [ecx + 4]		// 0x00049533
		faddp	st(1), st		// 0x00049536
		fld	dword ptr [ebp - 0x98]		// 0x00049538
		fmul	dword ptr [ecx]		// 0x0004953e
		faddp	st(1), st		// 0x00049540
		fadd	dword ptr [ebp - 0x68]		// 0x00049542
		fst	dword ptr [ebp - 0x18]		// 0x00049545
		fcomp	dword ptr [ebp + 0x50]		// 0x00049548
		fnstsw	ax		// 0x0004954b
		test	ah, 5		// 0x0004954d
		jp	L49647		// 0x00049550
		fld	dword ptr [ebp - 0x80]		// 0x00049556
		mov	eax, dword ptr [ebp + 0x28]		// 0x00049559
		fmul	dword ptr [ecx + 8]		// 0x0004955c
		fld	dword ptr [ebp - 0x90]		// 0x0004955f
		fmul	dword ptr [ecx + 4]		// 0x00049565
		faddp	st(1), st		// 0x00049568
		fld	dword ptr [ebp - 0xa0]		// 0x0004956a
		fmul	dword ptr [ecx]		// 0x00049570
		faddp	st(1), st		// 0x00049572
		fadd	dword ptr [ebp - 0x70]		// 0x00049574
		fstp	dword ptr [ebp - 0x20]		// 0x00049577
		mov	edx, dword ptr [ebp - 0x20]		// 0x0004957a
		fld	dword ptr [ebp - 0x7c]		// 0x0004957d
		fmul	dword ptr [ecx + 8]		// 0x00049580
		fld	dword ptr [ebp - 0x8c]		// 0x00049583
		fmul	dword ptr [ecx + 4]		// 0x00049589
		faddp	st(1), st		// 0x0004958c
		fld	dword ptr [ebp - 0x9c]		// 0x0004958e
		fmul	dword ptr [ecx]		// 0x00049594
		faddp	st(1), st		// 0x00049596
		fadd	dword ptr [ebp - 0x6c]		// 0x00049598
		fstp	dword ptr [ebp - 0x1c]		// 0x0004959b
		mov	ecx, dword ptr [ebp - 0x1c]		// 0x0004959e
		push	ecx		// 0x000495a1
		mov	ecx, dword ptr [ebp - 0x14]		// 0x000495a2
		push	edx		// 0x000495a5
		call	nxPolygonContainsPoint		// 0x000495a6
		add	esp, 8		// 0x000495ab
		test	eax, eax		// 0x000495ae
		je	L49647		// 0x000495b0
		mov	eax, dword ptr [ebp + 0x48]		// 0x000495b6
		fld	dword ptr [ebx + 0x20]		// 0x000495b9
		fmul	dword ptr [eax + 8]		// 0x000495bc
		mov	ecx, dword ptr [ebp + 0x78]		// 0x000495bf
		fld	dword ptr [eax]		// 0x000495c2
		mov	edx, dword ptr [ebp + 0x74]		// 0x000495c4
		fmul	dword ptr [ebx]		// 0x000495c7
		faddp	st(1), st		// 0x000495c9
		fld	dword ptr [ebx + 0x10]		// 0x000495cb
		fmul	dword ptr [eax + 4]		// 0x000495ce
		faddp	st(1), st		// 0x000495d1
		fadd	dword ptr [ebx + 0x30]		// 0x000495d3
		fstp	dword ptr [ebp - 0x54]		// 0x000495d6
		fld	dword ptr [ebx + 0x24]		// 0x000495d9
		fmul	dword ptr [eax + 8]		// 0x000495dc
		fld	dword ptr [ebx + 0x14]		// 0x000495df
		fmul	dword ptr [eax + 4]		// 0x000495e2
		faddp	st(1), st		// 0x000495e5
		fld	dword ptr [ebx + 4]		// 0x000495e7
		fmul	dword ptr [eax]		// 0x000495ea
		faddp	st(1), st		// 0x000495ec
		fadd	dword ptr [ebx + 0x34]		// 0x000495ee
		fstp	dword ptr [ebp - 0x50]		// 0x000495f1
		fld	dword ptr [ebx + 0x28]		// 0x000495f4
		fmul	dword ptr [eax + 8]		// 0x000495f7
		fld	dword ptr [ebx + 8]		// 0x000495fa
		fmul	dword ptr [eax]		// 0x000495fd
		faddp	st(1), st		// 0x000495ff
		fld	dword ptr [ebx + 0x18]		// 0x00049601
		fmul	dword ptr [eax + 4]		// 0x00049604
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00049607
		push	eax		// 0x0004960a
		mov	eax, dword ptr [ebp + 0x70]		// 0x0004960b
		push	ecx		// 0x0004960e
		faddp	st(1), st		// 0x0004960f
		push	edx		// 0x00049611
		push	eax		// 0x00049612
		fadd	dword ptr [ebx + 0x38]		// 0x00049613
		mov	eax, dword ptr [ebp + 0x60]		// 0x00049616
		lea	ecx, [ebp - 0x3c]		// 0x00049619
		push	ecx		// 0x0004961c
		fstp	dword ptr [ebp - 0x4c]		// 0x0004961d
		lea	edx, [ebp - 0x54]		// 0x00049620
		fld	dword ptr [ebp - 0x18]		// 0x00049623
		push	edx		// 0x00049626
		fsub	dword ptr [ebp + 0x50]		// 0x00049627
		mov	edx, dword ptr [ebp + 0x5c]		// 0x0004962a
		push	ecx		// 0x0004962d
		mov	ecx, dword ptr [eax + 0x9c]		// 0x0004962e
		mov	eax, dword ptr [edx + 0x9c]		// 0x00049634
		fstp	dword ptr [esp]		// 0x0004963a
		push	ecx		// 0x0004963d
		mov	ecx, dword ptr [ebp + 0x64]		// 0x0004963e
		push	eax		// 0x00049641
		call	NxEmitContactFeatures		// 0x00049642
L49647:
		mov	eax, dword ptr [ebp + 0x54]		// 0x00049647
		mov	ecx, dword ptr [ebp + 0x3c]		// 0x0004964a
		inc	eax		// 0x0004964d
		cmp	eax, ecx		// 0x0004964e
		mov	dword ptr [ebp + 0x54], eax		// 0x00049650
		jb	L49514		// 0x00049653
L49659:
		lea	eax, [ecx + ecx*2]		// 0x00049659
		shl	eax, 2		// 0x0004965c
		add	eax, 3		// 0x0004965f
		and	eax, 0xfffffffc		// 0x00049662
		call	_chkstk		// 0x00049665
		mov	eax, dword ptr [ebp + 0x3c]		// 0x0004966a
		mov	ebx, esp		// 0x0004966d
		xor	ecx, ecx		// 0x0004966f
		cmp	eax, 4		// 0x00049671
		mov	dword ptr [ebp + 0x50], ebx		// 0x00049674
		jl	L49874		// 0x00049677
		mov	ecx, dword ptr [ebp + 0x44]		// 0x0004967d
		add	eax, -4		// 0x00049680
		shr	eax, 2		// 0x00049683
		add	ecx, 8		// 0x00049686
		inc	eax		// 0x00049689
		mov	dword ptr [ebp + 0x54], eax		// 0x0004968a
		shl	eax, 2		// 0x0004968d
		lea	edx, [ebx + 0x18]		// 0x00049690
		mov	dword ptr [ebp + 0x48], eax		// 0x00049693
L49696:
		mov	eax, dword ptr [ecx - 8]		// 0x00049696
		mov	ebx, dword ptr [ebp + 0x40]		// 0x00049699
		lea	eax, [eax + eax*2]		// 0x0004969c
		fld	dword ptr [ebx + eax*4]		// 0x0004969f
		lea	eax, [ebx + eax*4]		// 0x000496a2
		fmul	dword ptr [esi]		// 0x000496a5
		fld	dword ptr [esi + 0x10]		// 0x000496a7
		fmul	dword ptr [eax + 4]		// 0x000496aa
		faddp	st(1), st		// 0x000496ad
		fld	dword ptr [eax + 8]		// 0x000496af
		fmul	dword ptr [esi + 0x20]		// 0x000496b2
		faddp	st(1), st		// 0x000496b5
		fadd	dword ptr [esi + 0x30]		// 0x000496b7
		fstp	dword ptr [ebp + 0x14]		// 0x000496ba
		mov	ebx, dword ptr [ebp + 0x14]		// 0x000496bd
		fld	dword ptr [esi + 0x14]		// 0x000496c0
		fmul	dword ptr [eax + 4]		// 0x000496c3
		fld	dword ptr [esi + 4]		// 0x000496c6
		fmul	dword ptr [eax]		// 0x000496c9
		faddp	st(1), st		// 0x000496cb
		fld	dword ptr [eax + 8]		// 0x000496cd
		fmul	dword ptr [esi + 0x24]		// 0x000496d0
		faddp	st(1), st		// 0x000496d3
		fadd	dword ptr [esi + 0x34]		// 0x000496d5
		fstp	dword ptr [ebp + 0x18]		// 0x000496d8
		fld	dword ptr [esi + 0x28]		// 0x000496db
		fmul	dword ptr [eax + 8]		// 0x000496de
		fld	dword ptr [esi + 8]		// 0x000496e1
		fmul	dword ptr [eax]		// 0x000496e4
		faddp	st(1), st		// 0x000496e6
		fld	dword ptr [esi + 0x18]		// 0x000496e8
		fmul	dword ptr [eax + 4]		// 0x000496eb
		lea	eax, [edx - 0x18]		// 0x000496ee
		faddp	st(1), st		// 0x000496f1
		fadd	dword ptr [esi + 0x38]		// 0x000496f3
		mov	dword ptr [eax], ebx		// 0x000496f6
		mov	ebx, dword ptr [ebp + 0x18]		// 0x000496f8
		mov	dword ptr [eax + 4], ebx		// 0x000496fb
		fstp	dword ptr [ebp + 0x1c]		// 0x000496fe
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x00049701
		mov	dword ptr [eax + 8], ebx		// 0x00049704
		mov	eax, dword ptr [ecx - 4]		// 0x00049707
		mov	ebx, dword ptr [ebp + 0x40]		// 0x0004970a
		lea	eax, [eax + eax*2]		// 0x0004970d
		fld	dword ptr [ebx + eax*4]		// 0x00049710
		lea	eax, [ebx + eax*4]		// 0x00049713
		fmul	dword ptr [esi]		// 0x00049716
		fld	dword ptr [esi + 0x10]		// 0x00049718
		fmul	dword ptr [eax + 4]		// 0x0004971b
		faddp	st(1), st		// 0x0004971e
		fld	dword ptr [eax + 8]		// 0x00049720
		fmul	dword ptr [esi + 0x20]		// 0x00049723
		faddp	st(1), st		// 0x00049726
		fadd	dword ptr [esi + 0x30]		// 0x00049728
		fstp	dword ptr [ebp + 0x14]		// 0x0004972b
		mov	ebx, dword ptr [ebp + 0x14]		// 0x0004972e
		fld	dword ptr [esi + 0x14]		// 0x00049731
		fmul	dword ptr [eax + 4]		// 0x00049734
		fld	dword ptr [esi + 4]		// 0x00049737
		fmul	dword ptr [eax]		// 0x0004973a
		faddp	st(1), st		// 0x0004973c
		fld	dword ptr [eax + 8]		// 0x0004973e
		fmul	dword ptr [esi + 0x24]		// 0x00049741
		faddp	st(1), st		// 0x00049744
		fadd	dword ptr [esi + 0x34]		// 0x00049746
		fstp	dword ptr [ebp + 0x18]		// 0x00049749
		fld	dword ptr [esi + 0x28]		// 0x0004974c
		fmul	dword ptr [eax + 8]		// 0x0004974f
		fld	dword ptr [esi + 8]		// 0x00049752
		fmul	dword ptr [eax]		// 0x00049755
		faddp	st(1), st		// 0x00049757
		fld	dword ptr [esi + 0x18]		// 0x00049759
		fmul	dword ptr [eax + 4]		// 0x0004975c
		lea	eax, [edx - 0xc]		// 0x0004975f
		faddp	st(1), st		// 0x00049762
		fadd	dword ptr [esi + 0x38]		// 0x00049764
		mov	dword ptr [eax], ebx		// 0x00049767
		mov	ebx, dword ptr [ebp + 0x18]		// 0x00049769
		mov	dword ptr [eax + 4], ebx		// 0x0004976c
		fstp	dword ptr [ebp + 0x1c]		// 0x0004976f
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x00049772
		mov	dword ptr [eax + 8], ebx		// 0x00049775
		mov	eax, dword ptr [ecx]		// 0x00049778
		mov	ebx, dword ptr [ebp + 0x40]		// 0x0004977a
		lea	eax, [eax + eax*2]		// 0x0004977d
		fld	dword ptr [ebx + eax*4]		// 0x00049780
		lea	eax, [ebx + eax*4]		// 0x00049783
		fmul	dword ptr [esi]		// 0x00049786
		fld	dword ptr [esi + 0x10]		// 0x00049788
		fmul	dword ptr [eax + 4]		// 0x0004978b
		faddp	st(1), st		// 0x0004978e
		fld	dword ptr [eax + 8]		// 0x00049790
		fmul	dword ptr [esi + 0x20]		// 0x00049793
		faddp	st(1), st		// 0x00049796
		fadd	dword ptr [esi + 0x30]		// 0x00049798
		fstp	dword ptr [ebp + 0x14]		// 0x0004979b
		mov	ebx, dword ptr [ebp + 0x14]		// 0x0004979e
		fld	dword ptr [esi + 0x14]		// 0x000497a1
		fmul	dword ptr [eax + 4]		// 0x000497a4
		fld	dword ptr [esi + 4]		// 0x000497a7
		fmul	dword ptr [eax]		// 0x000497aa
		faddp	st(1), st		// 0x000497ac
		fld	dword ptr [eax + 8]		// 0x000497ae
		fmul	dword ptr [esi + 0x24]		// 0x000497b1
		faddp	st(1), st		// 0x000497b4
		fadd	dword ptr [esi + 0x34]		// 0x000497b6
		fstp	dword ptr [ebp + 0x18]		// 0x000497b9
		fld	dword ptr [esi + 0x28]		// 0x000497bc
		fmul	dword ptr [eax + 8]		// 0x000497bf
		fld	dword ptr [esi + 8]		// 0x000497c2
		fmul	dword ptr [eax]		// 0x000497c5
		faddp	st(1), st		// 0x000497c7
		fld	dword ptr [esi + 0x18]		// 0x000497c9
		fmul	dword ptr [eax + 4]		// 0x000497cc
		mov	eax, edx		// 0x000497cf
		faddp	st(1), st		// 0x000497d1
		fadd	dword ptr [esi + 0x38]		// 0x000497d3
		mov	dword ptr [eax], ebx		// 0x000497d6
		mov	ebx, dword ptr [ebp + 0x18]		// 0x000497d8
		mov	dword ptr [eax + 4], ebx		// 0x000497db
		fstp	dword ptr [ebp + 0x1c]		// 0x000497de
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x000497e1
		mov	dword ptr [eax + 8], ebx		// 0x000497e4
		mov	eax, dword ptr [ecx + 4]		// 0x000497e7
		mov	ebx, dword ptr [ebp + 0x40]		// 0x000497ea
		lea	eax, [eax + eax*2]		// 0x000497ed
		fld	dword ptr [ebx + eax*4]		// 0x000497f0
		lea	eax, [ebx + eax*4]		// 0x000497f3
		fmul	dword ptr [esi]		// 0x000497f6
		fld	dword ptr [esi + 0x10]		// 0x000497f8
		fmul	dword ptr [eax + 4]		// 0x000497fb
		faddp	st(1), st		// 0x000497fe
		fld	dword ptr [eax + 8]		// 0x00049800
		fmul	dword ptr [esi + 0x20]		// 0x00049803
		faddp	st(1), st		// 0x00049806
		fadd	dword ptr [esi + 0x30]		// 0x00049808
		fstp	dword ptr [ebp + 0x14]		// 0x0004980b
		mov	ebx, dword ptr [ebp + 0x14]		// 0x0004980e
		fld	dword ptr [esi + 0x14]		// 0x00049811
		fmul	dword ptr [eax + 4]		// 0x00049814
		fld	dword ptr [esi + 4]		// 0x00049817
		fmul	dword ptr [eax]		// 0x0004981a
		faddp	st(1), st		// 0x0004981c
		fld	dword ptr [eax + 8]		// 0x0004981e
		fmul	dword ptr [esi + 0x24]		// 0x00049821
		faddp	st(1), st		// 0x00049824
		fadd	dword ptr [esi + 0x34]		// 0x00049826
		fstp	dword ptr [ebp + 0x18]		// 0x00049829
		fld	dword ptr [esi + 0x28]		// 0x0004982c
		fmul	dword ptr [eax + 8]		// 0x0004982f
		fld	dword ptr [esi + 8]		// 0x00049832
		fmul	dword ptr [eax]		// 0x00049835
		faddp	st(1), st		// 0x00049837
		fld	dword ptr [esi + 0x18]		// 0x00049839
		fmul	dword ptr [eax + 4]		// 0x0004983c
		lea	eax, [edx + 0xc]		// 0x0004983f
		faddp	st(1), st		// 0x00049842
		fadd	dword ptr [esi + 0x38]		// 0x00049844
		mov	dword ptr [eax], ebx		// 0x00049847
		mov	ebx, dword ptr [ebp + 0x18]		// 0x00049849
		mov	dword ptr [eax + 4], ebx		// 0x0004984c
		fstp	dword ptr [ebp + 0x1c]		// 0x0004984f
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x00049852
		mov	dword ptr [eax + 8], ebx		// 0x00049855
		mov	eax, dword ptr [ebp + 0x54]		// 0x00049858
		add	ecx, 0x10		// 0x0004985b
		add	edx, 0x30		// 0x0004985e
		dec	eax		// 0x00049861
		mov	dword ptr [ebp + 0x54], eax		// 0x00049862
		jne	L49696		// 0x00049865
		mov	ecx, dword ptr [ebp + 0x48]		// 0x0004986b
		mov	eax, dword ptr [ebp + 0x3c]		// 0x0004986e
		mov	ebx, dword ptr [ebp + 0x50]		// 0x00049871
L49874:
		cmp	ecx, eax		// 0x00049874
		jae	L49903		// 0x00049876
		lea	edx, [ecx + ecx*2]		// 0x0004987c
		lea	edx, [ebx + edx*4]		// 0x0004987f
L49882:
		mov	eax, dword ptr [ebp + 0x44]		// 0x00049882
		mov	eax, dword ptr [eax + ecx*4]		// 0x00049885
		mov	ebx, dword ptr [ebp + 0x40]		// 0x00049888
		lea	eax, [eax + eax*2]		// 0x0004988b
		fld	dword ptr [ebx + eax*4]		// 0x0004988e
		lea	eax, [ebx + eax*4]		// 0x00049891
		fmul	dword ptr [esi]		// 0x00049894
		inc	ecx		// 0x00049896
		fld	dword ptr [esi + 0x10]		// 0x00049897
		fmul	dword ptr [eax + 4]		// 0x0004989a
		faddp	st(1), st		// 0x0004989d
		fld	dword ptr [eax + 8]		// 0x0004989f
		fmul	dword ptr [esi + 0x20]		// 0x000498a2
		faddp	st(1), st		// 0x000498a5
		fadd	dword ptr [esi + 0x30]		// 0x000498a7
		fstp	dword ptr [ebp + 0x14]		// 0x000498aa
		mov	ebx, dword ptr [ebp + 0x14]		// 0x000498ad
		fld	dword ptr [esi + 0x14]		// 0x000498b0
		fmul	dword ptr [eax + 4]		// 0x000498b3
		fld	dword ptr [esi + 4]		// 0x000498b6
		fmul	dword ptr [eax]		// 0x000498b9
		faddp	st(1), st		// 0x000498bb
		fld	dword ptr [eax + 8]		// 0x000498bd
		fmul	dword ptr [esi + 0x24]		// 0x000498c0
		faddp	st(1), st		// 0x000498c3
		fadd	dword ptr [esi + 0x34]		// 0x000498c5
		fstp	dword ptr [ebp + 0x18]		// 0x000498c8
		fld	dword ptr [esi + 0x28]		// 0x000498cb
		fmul	dword ptr [eax + 8]		// 0x000498ce
		fld	dword ptr [esi + 8]		// 0x000498d1
		fmul	dword ptr [eax]		// 0x000498d4
		faddp	st(1), st		// 0x000498d6
		fld	dword ptr [esi + 0x18]		// 0x000498d8
		fmul	dword ptr [eax + 4]		// 0x000498db
		mov	eax, edx		// 0x000498de
		add	edx, 0xc		// 0x000498e0
		faddp	st(1), st		// 0x000498e3
		fadd	dword ptr [esi + 0x38]		// 0x000498e5
		mov	dword ptr [eax], ebx		// 0x000498e8
		mov	ebx, dword ptr [ebp + 0x18]		// 0x000498ea
		mov	dword ptr [eax + 4], ebx		// 0x000498ed
		fstp	dword ptr [ebp + 0x1c]		// 0x000498f0
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x000498f3
		mov	dword ptr [eax + 8], ebx		// 0x000498f6
		mov	eax, dword ptr [ebp + 0x3c]		// 0x000498f9
		cmp	ecx, eax		// 0x000498fc
		jb	L49882		// 0x000498fe
		mov	ebx, dword ptr [ebp + 0x50]		// 0x00049900
L49903:
		test	eax, eax		// 0x00049903
		jbe	L49c88		// 0x00049905
		mov	edx, 1		// 0x0004990b
		lea	esi, [ebx + 4]		// 0x00049910
		mov	dword ptr [ebp + 0x58], edx		// 0x00049913
		mov	dword ptr [ebp + 0x54], esi		// 0x00049916
		mov	dword ptr [ebp - 0x14], eax		// 0x00049919
		jmp	L49a71		// 0x0004991c
L49921:
		fstp	st(0)		// 0x00049921
L49923:
		fld	dword ptr [ebp + 0x1c]		// 0x00049923
		fmul	dword ptr [ecx + 8]		// 0x00049926
		fld	dword ptr [ebp + 0x14]		// 0x00049929
		fmul	dword ptr [ecx]		// 0x0004992c
		faddp	st(1), st		// 0x0004992e
		fld	dword ptr [ebp + 0x18]		// 0x00049930
		fmul	dword ptr [ecx + 4]		// 0x00049933
		faddp	st(1), st		// 0x00049936
		fld	dword ptr [ecx + 0xc]		// 0x00049938
		fchs		// 0x0004993b
		fsubp	st(1), st		// 0x0004993d
		fdiv	st, st(1)		// 0x0004993f
		fstp	dword ptr [ebp + 0x50]		// 0x00049941
		fstp	st(0)		// 0x00049944
		fld	dword ptr [ebp + 0x50]		// 0x00049946
		fcomp	dword ptr kContactZero		// 0x00049949
		fnstsw	ax		// 0x0004994f
		test	ah, 5		// 0x00049951
		jp	L491df		// 0x00049954
		fld	dword ptr [ebp - 0x20]		// 0x0004995a
		mov	eax, dword ptr [ebp + 0x3c]		// 0x0004995d
		fmul	dword ptr [ebp + 0x50]		// 0x00049960
		fld	dword ptr [ebp - 0x1c]		// 0x00049963
		fmul	dword ptr [ebp + 0x50]		// 0x00049966
		fld	dword ptr [ebp - 0x18]		// 0x00049969
		fmul	dword ptr [ebp + 0x50]		// 0x0004996c
		fstp	dword ptr [ebp - 0x24]		// 0x0004996f
		fld	dword ptr [ebp + 0x14]		// 0x00049972
		fsub	st, st(2)		// 0x00049975
		fstp	dword ptr [ebp - 0xac]		// 0x00049977
		fld	dword ptr [ebp + 0x18]		// 0x0004997d
		fsub	st, st(1)		// 0x00049980
		fstp	st(2)		// 0x00049982
		fstp	st(0)		// 0x00049984
		fld	dword ptr [ebp + 0x1c]		// 0x00049986
		fsub	dword ptr [ebp - 0x24]		// 0x00049989
		fld	st(1)		// 0x0004998c
		fmul	dword ptr [ebp - 0xc]		// 0x0004998e
		fld	st(1)		// 0x00049991
		fmul	dword ptr [ebp - 8]		// 0x00049993
		faddp	st(1), st		// 0x00049996
		fld	dword ptr [ebp - 0xac]		// 0x00049998
		fmul	dword ptr [ebp - 0x10]		// 0x0004999e
		faddp	st(1), st		// 0x000499a1
		fstp	dword ptr [ebp - 0x48]		// 0x000499a3
		mov	edx, dword ptr [ebp - 0x48]		// 0x000499a6
		fxch	st(1)		// 0x000499a9
		fmul	dword ptr [ebp]		// 0x000499ab
		fxch	st(1)		// 0x000499ae
		fmul	dword ptr [ebp + 4]		// 0x000499b0
		faddp	st(1), st		// 0x000499b3
		fld	dword ptr [ebp - 4]		// 0x000499b5
		fmul	dword ptr [ebp - 0xac]		// 0x000499b8
		faddp	st(1), st		// 0x000499be
		fstp	dword ptr [ebp - 0x44]		// 0x000499c0
		mov	ecx, dword ptr [ebp - 0x44]		// 0x000499c3
		push	ecx		// 0x000499c6
		mov	ecx, dword ptr [ebp - 0x14]		// 0x000499c7
		push	edx		// 0x000499ca
		call	nxPolygonContainsPoint		// 0x000499cb
		add	esp, 8		// 0x000499d0
		test	eax, eax		// 0x000499d3
		je	L491df		// 0x000499d5
		mov	eax, dword ptr [ebp + 0x48]		// 0x000499db
		fld	dword ptr [edi + 0x20]		// 0x000499de
		fmul	dword ptr [eax + 8]		// 0x000499e1
		mov	ecx, dword ptr [ebp + 0x78]		// 0x000499e4
		fld	dword ptr [edi + 0x10]		// 0x000499e7
		mov	edx, dword ptr [ebp + 0x74]		// 0x000499ea
		fmul	dword ptr [eax + 4]		// 0x000499ed
		faddp	st(1), st		// 0x000499f0
		fld	dword ptr [eax]		// 0x000499f2
		fmul	dword ptr [edi]		// 0x000499f4
		faddp	st(1), st		// 0x000499f6
		fadd	dword ptr [edi + 0x30]		// 0x000499f8
		fstp	dword ptr [ebp - 0x54]		// 0x000499fb
		fld	dword ptr [edi + 0x24]		// 0x000499fe
		fmul	dword ptr [eax + 8]		// 0x00049a01
		fld	dword ptr [edi + 0x14]		// 0x00049a04
		fmul	dword ptr [eax + 4]		// 0x00049a07
		faddp	st(1), st		// 0x00049a0a
		fld	dword ptr [edi + 4]		// 0x00049a0c
		fmul	dword ptr [eax]		// 0x00049a0f
		faddp	st(1), st		// 0x00049a11
		fadd	dword ptr [edi + 0x34]		// 0x00049a13
		fstp	dword ptr [ebp - 0x50]		// 0x00049a16
		fld	dword ptr [edi + 0x28]		// 0x00049a19
		fmul	dword ptr [eax + 8]		// 0x00049a1c
		fld	dword ptr [edi + 0x18]		// 0x00049a1f
		fmul	dword ptr [eax + 4]		// 0x00049a22
		faddp	st(1), st		// 0x00049a25
		fld	dword ptr [edi + 8]		// 0x00049a27
		fmul	dword ptr [eax]		// 0x00049a2a
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00049a2c
		push	eax		// 0x00049a2f
		mov	eax, dword ptr [ebp + 0x70]		// 0x00049a30
		push	ecx		// 0x00049a33
		faddp	st(1), st		// 0x00049a34
		push	edx		// 0x00049a36
		push	eax		// 0x00049a37
		mov	eax, dword ptr [ebp + 0x50]		// 0x00049a38
		fadd	dword ptr [edi + 0x38]		// 0x00049a3b
		lea	ecx, [ebp - 0x3c]		// 0x00049a3e
		push	ecx		// 0x00049a41
		mov	ecx, dword ptr [ebp + 0x60]		// 0x00049a42
		fstp	dword ptr [ebp - 0x4c]		// 0x00049a45
		lea	edx, [ebp - 0x54]		// 0x00049a48
		push	edx		// 0x00049a4b
		mov	edx, dword ptr [ecx + 0x9c]		// 0x00049a4c
		push	eax		// 0x00049a52
		mov	eax, dword ptr [ebp + 0x5c]		// 0x00049a53
		mov	ecx, dword ptr [eax + 0x9c]		// 0x00049a56
		push	edx		// 0x00049a5c
		push	ecx		// 0x00049a5d
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00049a5e
		call	NxEmitContactFeatures		// 0x00049a61
		jmp	L491df		// 0x00049a66
L49a6b:
		mov	eax, dword ptr [ebp + 0x3c]		// 0x00049a6b
		mov	ebx, dword ptr [ebp + 0x50]		// 0x00049a6e
L49a71:
		cmp	edx, eax		// 0x00049a71
		mov	ecx, edx		// 0x00049a73
		jb	L49a79		// 0x00049a75
		xor	ecx, ecx		// 0x00049a77
L49a79:
		lea	ecx, [ecx + ecx*2]		// 0x00049a79
		fld	dword ptr [ebx + ecx*4]		// 0x00049a7c
		lea	ebx, [ebx + ecx*4]		// 0x00049a7f
		fsub	dword ptr [esi - 4]		// 0x00049a82
		mov	ecx, dword ptr [ebp + 0x38]		// 0x00049a85
		mov	dword ptr [ebp - 0x58], ebx		// 0x00049a88
		fstp	dword ptr [ebp - 0x20]		// 0x00049a8b
		fld	dword ptr [ebx + 4]		// 0x00049a8e
		fsub	dword ptr [esi]		// 0x00049a91
		fstp	dword ptr [ebp - 0x1c]		// 0x00049a93
		fld	dword ptr [ebx + 8]		// 0x00049a96
		fsub	dword ptr [esi + 4]		// 0x00049a99
		fstp	dword ptr [ebp - 0x18]		// 0x00049a9c
		fld	dword ptr [ebp - 0x1c]		// 0x00049a9f
		fmul	dword ptr [ecx + 8]		// 0x00049aa2
		fld	dword ptr [ebp - 0x18]		// 0x00049aa5
		fmul	dword ptr [ecx + 4]		// 0x00049aa8
		fsubp	st(1), st		// 0x00049aab
		fstp	dword ptr [ebp - 0x48]		// 0x00049aad
		mov	eax, dword ptr [ebp - 0x48]		// 0x00049ab0
		fld	dword ptr [ebp - 0x18]		// 0x00049ab3
		mov	dword ptr [ebp - 0x30], eax		// 0x00049ab6
		fmul	dword ptr [ecx]		// 0x00049ab9
		fld	dword ptr [ebp - 0x20]		// 0x00049abb
		fmul	dword ptr [ecx + 8]		// 0x00049abe
		fsubp	st(1), st		// 0x00049ac1
		fstp	dword ptr [ebp - 0x44]		// 0x00049ac3
		mov	eax, dword ptr [ebp - 0x44]		// 0x00049ac6
		fld	dword ptr [ebp - 0x20]		// 0x00049ac9
		mov	dword ptr [ebp - 0x2c], eax		// 0x00049acc
		fmul	dword ptr [ecx + 4]		// 0x00049acf
		fld	dword ptr [ebp - 0x1c]		// 0x00049ad2
		fmul	dword ptr [ecx]		// 0x00049ad5
		fsubp	st(1), st		// 0x00049ad7
		fstp	dword ptr [ebp - 0x40]		// 0x00049ad9
		mov	eax, dword ptr [ebp - 0x40]		// 0x00049adc
		mov	dword ptr [ebp - 0x28], eax		// 0x00049adf
		fld	dword ptr [ebp - 0x28]		// 0x00049ae2
		fmul	dword ptr [ebp - 0x28]		// 0x00049ae5
		fld	dword ptr [ebp - 0x2c]		// 0x00049ae8
		fmul	dword ptr [ebp - 0x2c]		// 0x00049aeb
		faddp	st(1), st		// 0x00049aee
		fld	dword ptr [ebp - 0x30]		// 0x00049af0
		fmul	dword ptr [ebp - 0x30]		// 0x00049af3
		faddp	st(1), st		// 0x00049af6
		fld	dword ptr kContactZero		// 0x00049af8
		fld	st(1)		// 0x00049afe
		fucompp		// 0x00049b00
		fnstsw	ax		// 0x00049b02
		test	ah, 0x44		// 0x00049b04
		jnp	L49b29		// 0x00049b07
		fsqrt		// 0x00049b09
		fdivr	dword ptr kContactOne		// 0x00049b0b
		fld	dword ptr [ebp - 0x30]		// 0x00049b11
		fmul	st, st(1)		// 0x00049b14
		fstp	dword ptr [ebp - 0x30]		// 0x00049b16
		fld	dword ptr [ebp - 0x2c]		// 0x00049b19
		fmul	st, st(1)		// 0x00049b1c
		fstp	dword ptr [ebp - 0x2c]		// 0x00049b1e
		fld	dword ptr [ebp - 0x28]		// 0x00049b21
		fmul	st, st(1)		// 0x00049b24
		fstp	dword ptr [ebp - 0x28]		// 0x00049b26
L49b29:
		mov	eax, dword ptr [ebp + 0x28]		// 0x00049b29
		fstp	st(0)		// 0x00049b2c
		test	eax, eax		// 0x00049b2e
		fld	dword ptr [ebp - 0x28]		// 0x00049b30
		fmul	dword ptr [esi + 4]		// 0x00049b33
		fld	dword ptr [ebp - 0x30]		// 0x00049b36
		fmul	dword ptr [esi - 4]		// 0x00049b39
		faddp	st(1), st		// 0x00049b3c
		fld	dword ptr [ebp - 0x2c]		// 0x00049b3e
		fmul	dword ptr [esi]		// 0x00049b41
		faddp	st(1), st		// 0x00049b43
		fchs		// 0x00049b45
		fstp	dword ptr [ebp - 0x24]		// 0x00049b47
		jbe	L49c71		// 0x00049b4a
		mov	edx, dword ptr [ebp + 0x30]		// 0x00049b50
		mov	dword ptr [ebp + 0x44], edx		// 0x00049b53
		mov	edx, dword ptr [ebp + 0x28]		// 0x00049b56
		mov	eax, 1		// 0x00049b59
		mov	dword ptr [ebp + 0x40], eax		// 0x00049b5e
		mov	dword ptr [ebp + 0x48], edx		// 0x00049b61
		jmp	L49b70		// 0x00049b64
L49b66:
		mov	eax, dword ptr [ebp + 0x40]		// 0x00049b66
		mov	ebx, dword ptr [ebp - 0x58]		// 0x00049b69
		mov	ecx, dword ptr [ebp + 0x38]		// 0x00049b6c
		_emit	0x90		// 0x00049b6f nop 
L49b70:
		cmp	eax, dword ptr [ebp + 0x28]		// 0x00049b70
		mov	dword ptr [ebp - 0x5c], eax		// 0x00049b73
		jb	L49b7c		// 0x00049b76
		xor	eax, eax		// 0x00049b78
		jmp	L49b7f		// 0x00049b7a
L49b7c:
		mov	eax, dword ptr [ebp - 0x5c]		// 0x00049b7c
L49b7f:
		lea	edx, [ebp - 0x60]		// 0x00049b7f
		push	edx		// 0x00049b82
		mov	edx, dword ptr [ebp + 0x30]		// 0x00049b83
		mov	eax, dword ptr [edx + eax*4]		// 0x00049b86
		mov	edx, dword ptr [ebp + 0x2c]		// 0x00049b89
		lea	eax, [eax + eax*2]		// 0x00049b8c
		lea	eax, [edx + eax*4]		// 0x00049b8f
		push	eax		// 0x00049b92
		mov	eax, dword ptr [ebp + 0x44]		// 0x00049b93
		mov	eax, dword ptr [eax]		// 0x00049b96
		lea	eax, [eax + eax*2]		// 0x00049b98
		push	ecx		// 0x00049b9b
		lea	edx, [edx + eax*4]		// 0x00049b9c
		lea	eax, [esi - 4]		// 0x00049b9f
		push	ebx		// 0x00049ba2
		push	eax		// 0x00049ba3
		lea	esi, [ebp + 0x14]		// 0x00049ba4
		lea	ecx, [ebp - 0x30]		// 0x00049ba7
		lea	ebx, [ebp - 0x20]		// 0x00049baa
		call	nxClipEdgeToPolygonPlane		// 0x00049bad
		add	esp, 0x14		// 0x00049bb2
		test	eax, eax		// 0x00049bb5
		je	L49c4e		// 0x00049bb7
		fld	dword ptr [ebp + 0x18]		// 0x00049bbd
		mov	ecx, dword ptr [ebp + 0x7c]		// 0x00049bc0
		fmul	dword ptr [edi + 0x14]		// 0x00049bc3
		mov	edx, dword ptr [ebp + 0x78]		// 0x00049bc6
		fld	dword ptr [ebp + 0x1c]		// 0x00049bc9
		mov	eax, dword ptr [ebp + 0x74]		// 0x00049bcc
		fmul	dword ptr [edi + 0x24]		// 0x00049bcf
		push	ecx		// 0x00049bd2
		mov	ecx, dword ptr [ebp + 0x70]		// 0x00049bd3
		push	edx		// 0x00049bd6
		faddp	st(1), st		// 0x00049bd7
		push	eax		// 0x00049bd9
		fld	dword ptr [ebp + 0x14]		// 0x00049bda
		push	ecx		// 0x00049bdd
		fmul	dword ptr [edi + 4]		// 0x00049bde
		lea	edx, [ebp - 0x3c]		// 0x00049be1
		push	edx		// 0x00049be4
		mov	eax, esi		// 0x00049be5
		faddp	st(1), st		// 0x00049be7
		push	eax		// 0x00049be9
		mov	eax, dword ptr [ebp + 0x5c]		// 0x00049bea
		push	ecx		// 0x00049bed
		fadd	dword ptr [edi + 0x34]		// 0x00049bee
		mov	ecx, dword ptr [ebp + 0x60]		// 0x00049bf1
		fld	dword ptr [ebp + 0x18]		// 0x00049bf4
		mov	edx, dword ptr [ecx + 0x9c]		// 0x00049bf7
		fmul	dword ptr [edi + 0x18]		// 0x00049bfd
		mov	ecx, dword ptr [eax + 0x9c]		// 0x00049c00
		fld	dword ptr [ebp + 0x1c]		// 0x00049c06
		fmul	dword ptr [edi + 0x28]		// 0x00049c09
		faddp	st(1), st		// 0x00049c0c
		fld	dword ptr [ebp + 0x14]		// 0x00049c0e
		fmul	dword ptr [edi + 8]		// 0x00049c11
		faddp	st(1), st		// 0x00049c14
		fadd	dword ptr [edi + 0x38]		// 0x00049c16
		fld	dword ptr [ebp + 0x18]		// 0x00049c19
		fmul	dword ptr [edi + 0x10]		// 0x00049c1c
		fld	dword ptr [ebp + 0x1c]		// 0x00049c1f
		fmul	dword ptr [edi + 0x20]		// 0x00049c22
		faddp	st(1), st		// 0x00049c25
		fld	dword ptr [ebp + 0x14]		// 0x00049c27
		fmul	dword ptr [edi]		// 0x00049c2a
		faddp	st(1), st		// 0x00049c2c
		fadd	dword ptr [edi + 0x30]		// 0x00049c2e
		fstp	dword ptr [ebp + 0x14]		// 0x00049c31
		fxch	st(1)		// 0x00049c34
		fstp	dword ptr [ebp + 0x18]		// 0x00049c36
		fstp	dword ptr [ebp + 0x1c]		// 0x00049c39
		fld	dword ptr [ebp - 0x60]		// 0x00049c3c
		fchs		// 0x00049c3f
		fstp	dword ptr [esp]		// 0x00049c41
		push	edx		// 0x00049c44
		push	ecx		// 0x00049c45
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00049c46
		call	NxEmitContactFeatures		// 0x00049c49
L49c4e:
		mov	edx, dword ptr [ebp + 0x40]		// 0x00049c4e
		mov	ecx, dword ptr [ebp + 0x44]		// 0x00049c51
		mov	eax, dword ptr [ebp + 0x48]		// 0x00049c54
		mov	esi, dword ptr [ebp + 0x54]		// 0x00049c57
		inc	edx		// 0x00049c5a
		add	ecx, 4		// 0x00049c5b
		dec	eax		// 0x00049c5e
		mov	dword ptr [ebp + 0x40], edx		// 0x00049c5f
		mov	dword ptr [ebp + 0x44], ecx		// 0x00049c62
		mov	dword ptr [ebp + 0x48], eax		// 0x00049c65
		jne	L49b66		// 0x00049c68
		mov	edx, dword ptr [ebp + 0x58]		// 0x00049c6e
L49c71:
		mov	eax, dword ptr [ebp - 0x14]		// 0x00049c71
		inc	edx		// 0x00049c74
		add	esi, 0xc		// 0x00049c75
		dec	eax		// 0x00049c78
		mov	dword ptr [ebp + 0x58], edx		// 0x00049c79
		mov	dword ptr [ebp + 0x54], esi		// 0x00049c7c
		mov	dword ptr [ebp - 0x14], eax		// 0x00049c7f
		jne	L49a6b		// 0x00049c82
L49c88:
		lea	esp, [ebp - 0xb8]		// 0x00049c88
		pop	edi		// 0x00049c8e
		pop	esi		// 0x00049c8f
		pop	ebx		// 0x00049c90
		add	ebp, 0x20		// 0x00049c91
		mov	esp, ebp		// 0x00049c94
		pop	ebp		// 0x00049c96
		ret		// 0x00049c97
		}
	}

static bool nxCompoundAabbOverlap(const NxReal* a, const NxReal* b)
	{
	return a[0] <= b[3] && b[0] <= a[3] &&
		a[1] <= b[4] && b[1] <= a[4] &&
		a[2] <= b[5] && b[2] <= a[5];
	}

// phys_fn_001793 (0x0003f8b0, 345 B)
// Shared matrix-A compound expander.
static void nxContactCompoundPair(const NxCollisionShape* compound,
	const NxCollisionShape* other, NxContactSink* sink, void* context)
	{
	const NxReal* compoundBounds = NxShapeWorldBounds(compound);
	const NxCollisionShape* const* child =
		*(const NxCollisionShape* const* const*) ((const NxU8*) compound + 0xe0);
	const NxCollisionShape* const* childEnd =
		*(const NxCollisionShape* const* const*) ((const NxU8*) compound + 0xe4);
	const NxU32* groupMasks = nxPhysicsSDKGroupCollisionMasks();
	const void* pairMap = (const NxU8*) context + 0x2c;
	void* matrix = NxGetCollisionDispatchMatrix();
	if(!matrix)
		return;
	for(; child != childEnd; ++child)
		{
		const NxCollisionShape* candidate = *child;
		if(candidate == other)
			continue;
		const NxReal* childBounds = NxShapeWorldBounds(candidate);
		if(nxCompoundAabbOverlap(childBounds, compoundBounds) &&
			NxFilterShapePair(groupMasks, pairMap, candidate, other))
			NxDispatchShapePair(matrix, candidate, other, sink, context);
		}
	}

// phys_fn_001795 (0x0003fa10, 29 B)
// The wrapper puts its compound argument first for the shared expander.
void __cdecl NxContactCompoundShape(const NxCollisionShape* shape,
	const NxCollisionShape* compound, NxContactSink* sink, void* context)
	{
	nxContactCompoundPair(compound, shape, sink, context);
	}

// phys_fn_001797 (0x0003fa30, 77 B)
// phys_fn_001799 (0x0003fa80, 409 B)
// phys_fn_001801 (0x0003fc20, 346 B)
// The child-pair walks occupy the entry and its two continuations.
void __cdecl NxContactCompoundCompound(const NxCollisionShape* compound0,
	const NxCollisionShape* compound1, NxContactSink* sink, void* context)
	{
	const NxCollisionShape* const* children0 =
		*(const NxCollisionShape* const* const*) ((const NxU8*) compound0 + 0xe0);
	const NxCollisionShape* const* end0 =
		*(const NxCollisionShape* const* const*) ((const NxU8*) compound0 + 0xe4);
	const NxCollisionShape* const* children1 =
		*(const NxCollisionShape* const* const*) ((const NxU8*) compound1 + 0xe0);
	const NxCollisionShape* const* end1 =
		*(const NxCollisionShape* const* const*) ((const NxU8*) compound1 + 0xe4);
	const NxU32* groupMasks = nxPhysicsSDKGroupCollisionMasks();
	const void* pairMap = (const NxU8*) context + 0x2c;
	void* matrix = NxGetCollisionDispatchMatrix();
	if(!matrix)
		return;
	if(compound0 == compound1)
		{
		for(const NxCollisionShape* const* a = children0; a != end0; ++a)
			{
			const NxReal* boundsA = NxShapeWorldBounds(*a);
			for(const NxCollisionShape* const* b = a + 1; b != end0; ++b)
				{
				const NxReal* boundsB = NxShapeWorldBounds(*b);
				if(nxCompoundAabbOverlap(boundsA, boundsB) &&
					NxFilterShapePair(groupMasks, pairMap, *a, *b))
					NxDispatchShapePair(matrix, *a, *b, sink, context);
				}
			}
		return;
		}
	for(const NxCollisionShape* const* a = children0; a != end0; ++a)
		{
		const NxReal* boundsA = NxShapeWorldBounds(*a);
		for(const NxCollisionShape* const* b = children1; b != end1; ++b)
			{
			const NxReal* boundsB = NxShapeWorldBounds(*b);
			if(nxCompoundAabbOverlap(boundsA, boundsB) &&
				NxFilterShapePair(groupMasks, pairMap, *a, *b))
				NxDispatchShapePair(matrix, *a, *b, sink, context);
			}
		}
	}
