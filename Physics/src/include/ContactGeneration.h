#ifndef NX_PHYSICS_CONTACT_GENERATION_H
#define NX_PHYSICS_CONTACT_GENERATION_H

// Contact generation: the `+0x04` half of the shape-pair dispatch matrix, and
// the emitter every entry in it funnels through.
//
// The dispatcher hands each entry `(shape0, shape1, sink, context)` with the
// pair already ordered by ascending shape type. Entries write nothing
// themselves; they compute a separation, a point and a normal and hand those to
// phys_fn_000873, which owns the stream format.

#include "NarrowPhase.h"
#include "NxRay.h"
#include "NxUserRaycastReport.h"

// BORROWED Phase 7 layout. Recovered from phys_fn_000873 with the address for
// every offset; see the borrowed-layout section of
// evidence/phase3-narrow-phase.md. Phase 7 owns and must still close the rows
// behind this, and should report any disagreement with this struct as a finding
// rather than silently preferring one version.
//
// The object is built and reset by two rows outside Phase 3: phys_fn_002356 at
// 0x0005b680 (Phase 7) constructs the stream sub-object at `+0x38` through
// 0x000b4d70 and then calls phys_fn_002354 at 0x0005b620 (Phase 2) to reset it.
// 0x0005b620 operates on `sink + 0x10`, so every offset inside that function is
// 0x10 lower than the ones here: the sink is an outer object with a 0x10
// prefix, and `orientedTo` at `+0x08` lives in that prefix and survives a reset.
//
// Measured, not inferred. Calling the pinned oracle's 0x0005b620 on
// `&sink + 0x10` over a 0xcd-poisoned sink zeroes `+0x10`..`+0x43`, leaves the
// prefix at 0xcdcdcdcd, preserves the stream array header and reserves one
// stream word whose index it records -- which is field for field what the
// harness's nxResetWorld produces.
//
// An earlier version of this comment said 0x0005b620 was not the initialiser.
// That came from a reference scan that found two of its three call sites; the
// third, 0x0005b68d, passes `this` directly. What is still NOT established is
// what `orientedTo` points at or what else lives in the 0x10 prefix. The 0x44
// minimum is now explained as a 0x10 prefix plus a 0x34 sub-object, and 0x48
// remains unestablished.
struct NxContactSink
	{
	NxU8   head[8];
	void*  orientedTo;			// 0x08: compared against owner->[8], 0x0001d62c
	NxU8   gap0[0x10 - 0x0c];
	NxU32  contactCount;		// 0x10: incremented per contact, 0x0001d827
	NxU32  pairCountIndex;		// 0x14: stream index of the pair counter
	NxU32  normalCountIndex;	// 0x18: stream index of the normal counter
	NxU32  pointCountIndex;		// 0x1c: stream index of the contact counter
	void*  lastObject1;			// 0x20: 0x0001d67d / 0x0001d6c0
	void*  lastObject0;			// 0x24: 0x0001d688 / 0x0001d6c9
	NxReal lastNormal[3];		// 0x28: the cached normal, 0x0001d78c..0x0001d7b6
	NxU32  featurePairValid;	// 0x34: 0x0001d6b0, tested 0x0001d897
	NxU32  streamCapacity;		// 0x38
	NxU32  streamCount;			// 0x3c
	NxU32* stream;				// 0x40
	};

// phys_fn_000873 at 0x0001d610. __thiscall on the sink, seven stack arguments,
// `ret 0x1c`.
void NxEmitContact(NxContactSink* sink, void* object1, void* object0,
	NxU32 separationBits, const NxVec3* point, const NxVec3* normal,
	NxU16 featureId0, NxU16 featureId1);

// phys_fn_001901 at 0x00048a70, matrix A slot [PLANE][SPHERE].
void __cdecl NxContactPlaneSphere(const NxCollisionShape* plane,
	const NxCollisionShape* sphere, NxContactSink* sink, void* context);

// phys_fn_001261 at 0x00025350, slot 5 of the *plane* shape's vtable
// (0x00107430 + 0x14). __thiscall on the shape, five stack arguments,
// `ret 0x14`, and it returns `this` rather than a boolean -- `mov eax,ebx` at
// 0x00025415 against `xor eax,eax` at the two failure exits.
//
// The third argument is never read: the 203 bytes touch the argument slots at
// `+0x04` (the ray), `+0x08` (the distance limit), `+0x10` (the hint flags) and
// `+0x14` (the hit), and nothing reads `+0x0c`. It is left unnamed rather than
// guessed at.
// The slot as the oracle holds it. Every shape type has one; the plane's and the
// sphere's are reconstructed here.
typedef const NxCollisionShape* (__thiscall* NxShapeRaycastFn)(const NxCollisionShape*,
	const NxRay*, NxReal, NxU32, NxU32, NxRaycastHit*);

// MSVC rejects __thiscall on anything that is not a member function (C3865),
// and this row has no class to be a member of -- Phase 5 owns the shape. The
// declaration below is the same ABI written the way the compiler will accept:
// __fastcall passes the first two integer arguments in ecx and edx and leaves
// the rest on the stack for the callee to pop, so an unused second parameter
// makes it byte-for-byte __thiscall with `ret 0x14`. It goes into a vtable slot
// and is called back through NxShapeRaycastFn.
const NxCollisionShape* __fastcall NxShapeRaycastPlane(const NxCollisionShape* plane,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit);

// phys_fn_001377 at 0x00027c70, slot 5 of the *sphere* shape's vtable
// (0x00107528 + 0x14). Same ABI note as above.
//
// It is not the plane's function written twice. It has no facing test and no
// "ahead of the origin" test -- only the distance limit -- and it computes and
// normalises its own hit normal where the plane's copies the plane's geometry.
const NxCollisionShape* __fastcall NxShapeRaycastSphere(const NxCollisionShape* sphere,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit);

// phys_fn_001891 at 0x00048370, matrix A slot [PLANE][CAPSULE].
void __cdecl NxContactPlaneCapsule(const NxCollisionShape* plane,
	const NxCollisionShape* capsule, NxContactSink* sink, void* context);

// phys_fn_001923 at 0x0004a4b0, matrix A slot [SPHERE][CAPSULE].
void __cdecl NxContactSphereCapsule(const NxCollisionShape* sphere,
	const NxCollisionShape* capsule, NxContactSink* sink, void* context);

#endif
