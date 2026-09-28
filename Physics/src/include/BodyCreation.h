#ifndef NX_PHYSICS_BODYCREATION
#define NX_PHYSICS_BODYCREATION
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The dynamic body record's constructor and destructor rows (scene-raycast
// block Task 4, sub-area body-creation; units/scene-raycast-contract.md). The
// record is the 0x260-byte block phys_fn_000026 allocates for every dynamic
// actor: an NxFoundation::Observable at +0 (vtable .rdata 0x10106890, one slot,
// the imported Observable::event), the base sub-object at +0x18 (constructor
// 000801, destructor 000799), then the body fields the rest of the product
// reads by offset (core/Joint.h JointBodyRecord names some of them).
//
// 000722, which 000797 and 000776 call, is core/JointSupport.cpp's
// Row000722Fixture::row000722 (written on main by effector-and-coredump Task 2
// and brought here unchanged).
//
// Both types are views: no object is declared with them and every field is
// reached by offset. Being members gives the rows the listing's thiscall ABI
// (`this` in ecx, the callee pops its stack arguments).
//
// The rows run at API time (NxScene::createActor, NxScene::releaseActor and
// the Scene's release) under the process control word 0x027f, never inside the
// simulation step, so BodyCreation.cpp keeps the default architecture: its
// double arithmetic is the x87's at 53 bits.

#include "Nxp.h"

class NxBodyDesc;
class NxVec3;
class NxQuat;

// this = record + 0x18.
struct DynamicBodyBase
	{
	// Row 000801 (0x1b7a0, ret 0xc): the base sub-object constructor.
	DynamicBodyBase* construct(void* aux, const NxReal* pose, NxU32 id);
	// Row 000799 (0x1b760, plain ret): the base sub-object destructor.
	void destruct();
	};

// this = the record.
struct DynamicBody
	{
	// Row 000797 (0x1b5c0, ret 0xc): the record constructor. `owner` is
	// the actor's 0x50-byte body (+4 the Scene), `pose` 12 floats (a row-major
	// 3x3, then the position), `desc` the body descriptor 000026 prepared.
	DynamicBody* construct(void* owner, const NxReal* pose, const NxBodyDesc* desc);
	// Row 000793 with its continuation 000795 (0x1a350, ret 4).
	void loadFromBodyDesc(const NxBodyDesc* desc);
	// Row 000748 (0x16f80, plain ret; the record in ecx): marks the
	// root's island dirty (+0x1e4 bit 1) when the root has an island object.
	void markIslandDirty();
	// Row 000776 (0x18570, plain ret; tail-jumps to 000799): the record
	// destructor (non-deleting; phys_fn_000030 frees the block).
	void destruct();

	// The setters (scene-raycast Task 4, sub-area setters). NpActor.cpp's
	// helpers nxNpActorAccumulateForce, nxNpActorTransitionKinematic and
	// nxNpActorApplyWorldMassPose forward to them; 000791 is
	// core/JointSupport.cpp's Row000791Fixture::row000791.
	// Row 000782 (0x18730, ret 0x10): add `force` (linear) and `torque`
	// (angular), either may be null, under `mode`; then wake when `wake`.
	void addForce(const NxVec3* force, const NxVec3* torque, NxU32 mode, bool wake);
	// Row 000784 (0x194b0, ret 8): the kinematic target in the +0x118
	// block, either part may be null; then wake.
	void setKinematicTarget(const NxVec3* position, const NxQuat* orientation);
	// Row 000785 with its continuation 000787 (0x19620, ret 4): enter
	// (non-zero) or leave (zero) the kinematic state.
	void setKinematic(NxU32 enable);
	// Row 000789 (0x19d00, plain ret): the actor pose (+0x50, +0x5c and the
	// copies at +0x18, +0x24) from the world mass frame (+0x134, +0x158) and
	// the local one (+0xdc, +0x100), and the world inverse tensor (+0x164).
	void setPoseFromCMass();
	};

#endif
