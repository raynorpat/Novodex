#ifndef NX_PHYSICS_NARROW_PHASE_H
#define NX_PHYSICS_NARROW_PHASE_H

// The narrow phase: the shape-pair dispatch matrix and the overlap tests it
// selects.
//
// The oracle keeps two 6x6 matrices of function pointers in one heap object the
// SDK constructor builds (phys_fn_002338 at 0x0005a8e0, 0x124 bytes: a vtable
// pointer, then 36 slots at +0x04 and 36 more at +0x94).
//
// There are two readers, not one. phys_fn_002348 at 0x0005ab80 is the
// dispatcher and reads both halves. phys_fn_002350 at 0x0005ae50 -- Phase 7,
// reached from 0x0001399c, the persistent trigger-pair reconciliation pass --
// reads the +0x94 half only, with the same type swap at 0x0005b28c, the same
// `6 * type0 + type1` at 0x0005b2b7 and the same null skip at 0x0005b2c7.
// Whoever owns trigger enter/stay/leave wants the second one.
//
// The dispatcher orders the pair by shape type, indexes `6 * type0 + type1`
// with `type0 <= type1`, and calls
//
//   +0x04  contact generation, three arguments plus the shared context:
//          (shape0, shape1, contact sink, context), __cdecl
//   +0x94  the boolean overlap test, (shape0, shape1, context), __cdecl,
//          result in al
//
// The two agree on their last argument -- the overlap test's third is the
// contact path's fourth, both the dispatcher's own param_4 -- and what contact
// generation has that the overlap test does not is the *third*, the sink. That
// matters for anyone extending this: an entry declared with two parameters is
// safe under __cdecl for the seven driven here, which read neither, but the
// four mesh entries do read the context.
//
// The second matrix is chosen when either shape carries one of the three
// trigger bits in its flag byte. Only the upper triangle is ever indexed, so
// the lower triangle is left null by construction rather than by omission.
//
// This header declares the +0x94 entries for the four primitive shape types.
// Everything mesh- or compound-shaped, and the whole +0x04 matrix, is still
// unreconstructed; see docs/reconstruction/novodex-physics/evidence for the
// recovered table and what each slot is.

#include "Nxf.h"
#include "NxVec3.h"

// NxShapeType, from the pinned public NxShape.h. NX_SHAPE_COUNT is 6 and that
// is exactly the matrix order.
enum
	{
	NX_COLLISION_SHAPE_TYPES = 6
	};

// The part of the internal Shape object the collision kernels read. The
// offsets are the oracle's, and the layout is deliberately expressed as
// padding rather than as a class: which fields sit in the gaps is Phase 5's to
// say, and nothing here may depend on it.
//
// `geometry` is the 16-byte union at +0xe0:
//   plane    normal in [0..2], d in [3]
//   sphere   radius in [0]
//   capsule  radius in [0], half height in [1]
//   box      extents in [1..3]; [0] is not read by any kernel here
// `owner` and `collisionObject` are BORROWED Phase 5 fields. Phase 3 reads them
// because contact generation cannot emit without them; Phase 5 owns and must
// still close the rows behind them. See the borrowed-layout section of
// evidence/phase3-narrow-phase.md for the address establishing each.
struct NxCollisionShape
	{
	NxU8   vtable[4];
	void*  owner;				// 0x04: borrowed, written at 0x00025543
	NxU8   head[0x0c - 0x08];
	NxReal rotation[9];			// 0x0c: world rotation, row major
	NxReal translation[3];		// 0x30: world position
	NxU8   middle[0x9c - 0x3c];
	void*  collisionObject;		// 0x9c: borrowed, written at 0x00024f0b etc
	NxU8   middle2[0xd0 - 0xa0];
	NxU32  type;				// 0xd0: NxShapeType
	NxU8   tail[0xe0 - 0xd4];
	NxReal geometry[4];			// 0xe0
	};

// The index phys_fn_002348 forms at 0x0005abbb and again at 0x0005abf2, as
// `lea edx,[eax+eax*2]; lea edx,[eax+edx*2]` over the ordered pair.
NX_INLINE NxU32 NxCollisionPairIndex(NxU32 lowType, NxU32 highType)
	{
	return lowType * NX_COLLISION_SHAPE_TYPES + highType;
	}

typedef bool (__cdecl* NxShapeOverlapFn)(const NxCollisionShape*, const NxCollisionShape*);

// The +0x94 matrix entries. Each takes the pair already ordered by ascending
// shape type, which is what makes the lower triangle unnecessary.
bool __cdecl NxOverlapPlaneSphere(const NxCollisionShape* plane, const NxCollisionShape* sphere);
bool __cdecl NxOverlapPlaneBox(const NxCollisionShape* plane, const NxCollisionShape* box);
bool __cdecl NxOverlapPlaneCapsule(const NxCollisionShape* plane, const NxCollisionShape* capsule);
bool __cdecl NxOverlapSphereSphere(const NxCollisionShape* sphere0, const NxCollisionShape* sphere1);
bool __cdecl NxOverlapSphereBox(const NxCollisionShape* sphere, const NxCollisionShape* box);
bool __cdecl NxOverlapSphereCapsule(const NxCollisionShape* sphere, const NxCollisionShape* capsule);
bool __cdecl NxOverlapBoxBox(const NxCollisionShape* box0, const NxCollisionShape* box1);

// The two helpers the entries above share, declared because the differential
// drives them at their own recorded addresses as well as through their callers.

// phys_fn_000943 at 0x00020750, __thiscall on the box shape. Writes the world
// position of the corner picked by the three signs, each of which the oracle
// passes as a full int and converts with `fild`.
void NxBoxShapeCorner(const NxCollisionShape* box, int signX, int signY, int signZ, NxVec3* corner);

// phys_fn_001913 at 0x00049ca0. The oracle's sphere/box entry copies both
// shapes into these two stack structures and calls this; the copies are what
// fixes the field order, so they are reproduced rather than folded away.
struct NxCollisionSphereData
	{
	NxReal center[3];
	NxReal radius;
	};

struct NxCollisionBoxData
	{
	NxReal center[3];
	NxReal extents[3];
	NxReal rotation[9];
	};

bool __cdecl NxOverlapSphereBoxData(const NxCollisionSphereData* sphere, const NxCollisionBoxData* box);

#endif
