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

// phys_fn_001010 at 0x00022480, slot 5 of a *capsule* shape's vtable
// (0x00106b20 + 0x14). Same ABI note as the two above.
//
// It writes no normal at all, whatever the hint flags say -- `flags` is the
// literal 0x13 and nothing in the 321 bytes touches hit+0x10..+0x18. That is
// load-bearing for NxContactCapsuleCapsule below.
const NxCollisionShape* __fastcall NxShapeRaycastCapsule(const NxCollisionShape* capsule,
	void* edxUnused, const NxRay* worldRay, NxReal maxDistance, NxU32 unread,
	NxU32 hintFlags, NxRaycastHit* hit);

// phys_fn_001775 at 0x0003d9d0, matrix A slot [CAPSULE][CAPSULE].
//
// Its swept half is NOT a function of its arguments: it passes hintFlags = 4 and
// hands the emitter hit.worldNormal, which the only slot-5 row it can reach
// never writes. See the comment on the definition.
void __cdecl NxContactCapsuleCapsule(const NxCollisionShape* capsule0,
	const NxCollisionShape* capsule1, NxContactSink* sink, void* context);

// phys_fn_001883 at 0x00047f20, matrix A slot [PLANE][BOX] -- 42 bytes plus 786
// in two annotated continuations.
//
// The one entry in this matrix that does not call NxEmitContact: the oracle
// inlines the stream logic into it. The three append levels are shared in the
// .cpp so there is only one copy of what the two agree on; what this row
// duplicates is only the predicates, and each one is a recorded difference.
void __cdecl NxContactPlaneBox(const NxCollisionShape* plane,
	const NxCollisionShape* box, NxContactSink* sink, void* context);

// phys_fn_001281 at 0x000257a0, four bytes: `mov eax,[ecx+4]; ret`. The shape's
// owner, the BORROWED Phase 5 field at Shape+0x04 established at 0x00025543.
// The two sphere entries below reach it through this accessor where the emitter
// reads the same field inline at 0x0001d61b. Same __fastcall-for-__thiscall note
// as the raycast rows above, except that this one takes no stack argument at all.
const void* __fastcall NxShapeOwner(const NxCollisionShape* shape, void* edxUnused);

// phys_fn_002266 at 0x00056650, 57 bytes. NOT an error path.
//
// Both sphere entries call it when one of the two shapes has a null
// `owner->[8]`, passing the shape whose `owner->[8]` is NOT null first. It reads
// NX_CONTINUOUS_CD out of the SDK's live parameter array through
// phys_fn_000429 -- `mov ecx,[0x10123c04]; push 0xb; call 0x0000dc00` at
// 0x00056650..0x00056658 -- and returns true without touching the sink when that
// parameter is 0.0f. Otherwise it tail-calls phys_fn_002264 at 0x00055eb0, the
// continuous-collision sweep.
//
// phys_fn_002264 IS NOT RECONSTRUCTED AND MUST NOT BE GUESSED. It reads a
// SECOND pose at Shape+0x3c..+0x68 (0x00055ebe onward) that no Phase 3 or
// borrowed-layout evidence establishes, reads and writes `sink+0xdc`, `+0xe0`,
// `+0xe4` and `+0xe9` (0x00056196..0x000561d2) where the borrowed sink layout
// stops at 0x44, and dispatches through vtable slot 7 at 0x000562c3 and
// 0x000564b9 -- a slot no constructor scan in this program has resolved. Three
// unestablished things, so this row stops here rather than inventing them.
//
// NX_CONTINUOUS_CD's shipped default is 0.0f -- phys_fn_000472 writes `ebp`,
// which it zeroed at 0x0000e1bb, into the defaults table at 0x0000e56e -- so
// under the SDK as it ships the guard closes and phys_fn_002264 is unreachable.
// The differential measures that rather than assuming it.
bool __cdecl NxContinuousCdPair(const NxCollisionShape* moving,
	const NxCollisionShape* fixed, NxContactSink* sink);

// phys_fn_001933 at 0x0004b860, matrix A slot [SPHERE][SPHERE].
void __cdecl NxContactSphereSphere(const NxCollisionShape* sphere0,
	const NxCollisionShape* sphere1, NxContactSink* sink, void* context);

// phys_fn_001917 at 0x00049f00, 969 bytes: the sphere/box contact geometry.
//
// It is phys_fn_001913 -- the [SPHERE][BOX] overlap test in NarrowPhase.cpp --
// with the answer kept rather than thrown away, plus a whole second algorithm
// for the case that one returns `true` from early: a sphere centre inside the
// box, where there is no closest point to push away from and the contact has to
// come from the shallowest face instead.
//
// It takes the same two flattened structures the overlap test does, which is
// what fixes their field order, and writes three results through pointers.
bool __cdecl NxSphereBoxContactData(const NxCollisionSphereData* sphere,
	const NxCollisionBoxData* box, NxVec3* point, NxVec3* normal,
	NxReal* separation);

// phys_fn_001919 at 0x0004a2d0, matrix A slot [SPHERE][BOX].
//
// It hands the emitter the SPHERE as `object1` and the BOX as `object0` -- that
// is, shape0 in the slot the plane/sphere entry gives shape1 (0x0004a3c7 pushes
// `[ebp+0x9c]` and 0x0004a3c6 pushes `[ebx+0x9c]`). Which side lands in which
// slot decides which owner the header's material is taken from first and which
// collision object the ordering rule compares, so it is a per-entry fact and not
// a convention.
void __cdecl NxContactSphereBox(const NxCollisionShape* sphere,
	const NxCollisionShape* box, NxContactSink* sink, void* context);

// phys_fn_001739 at 0x00038a90, 258 bytes, and the leaf of the box/box subtree:
// it is the only row under matrix A [BOX][BOX] that calls nothing.
//
// Given a quad as four vertex pointers and a point in the (y, z) plane, it
// answers "is the point inside the quad, and if it is, what x does it
// interpolate to". Outside is -1.0f, the constant at 0x1010687c, which its one
// caller rejects with `fcom 0.0f; test ah,1; jne` at 0x00039938.
//
// The oracle is entered with the quad in ecx and the two floats on the stack,
// and the CALLER cleans -- `add esp,8` at 0x0003993e and at the three other
// call sites in phys_fn_001741. That is a compiler-chosen convention for an
// internal function and no C++ declaration expresses it, so this copy takes
// ordinary parameters: the differential calls the candidate by name and only
// the oracle side needs the ABI. The result is left in st(0) and the caller
// compares the register before it narrows it, so the return type is `double`.
double NxBoxBoxQuadDepth(const NxVec3* const* quad, NxReal pointY, NxReal pointZ);

// phys_fn_001741 at 0x00038ba0 with its continuation phys_fn_001743 at
// 0x00039730 -- 4,193 bytes, the clipping/manifold row of matrix A [BOX][BOX].
// It builds the whole contact manifold for one reference face and returns the
// contact count in eax (`mov eax,ebx` at 0x00039bfe).
//
// THE ORACLE TAKES SEVEN INPUTS AND NO C++ DECLARATION EXPRESSES TWO OF THEM.
// Five arrive on the stack and two in registers:
//
//   eax  the incident box's pose, read from 0x00038bb3 on
//   edx  the incident box's three half extents, read at 0x00038d1b,
//        0x00038d40 and 0x00038d75
//
// `edx` is not in the decode's argument table, and it is a register argument in
// two different ways: the last two of phys_fn_001745's six dispatch arms set it
// with `mov edx,edi` immediately before the call (0x0003a9de, 0x0003ac9d) and
// the first four do not set it at all -- it survives untouched from
// phys_fn_001748, which loaded it before calling phys_fn_001745. So four of the
// six arms depend on a register living across a 4,271-byte function.
//
// A pose is twelve floats: a row-major 3x3 followed by the centre. `extentY`
// and `extentZ` are the REFERENCE face's other two half extents, by value.
//
// The differential calls this copy by name and drives the oracle through a
// thunk that loads eax and edx, the way nxCallQuadDepthOracle does for the leaf.
//
// NO CAP. The count can reach 72 -- eight corners, five clip cases on each of
// twelve edges, four face vertices -- and phys_fn_001749's own buffers hold
// sixteen, so the caller's frame is what bounds it. See the note on the
// implementation.
int NxBoxBoxClipFace(NxVec3* points, NxReal* separations, const NxReal* pose,
	NxReal extentY, NxReal extentZ, const NxReal* incidentPose,
	const NxReal* incidentExtents);

#endif
