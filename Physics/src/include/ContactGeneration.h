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

#endif
