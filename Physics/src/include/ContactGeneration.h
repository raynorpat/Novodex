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
// What is NOT established here, and is therefore not written down: what
// constructs this object, what `orientedTo` points at, and whether the fields
// the emitter never touches exist at all. `0x0005b620` is not its initialiser --
// both of that function's call sites pass `this + 0x10`.
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
