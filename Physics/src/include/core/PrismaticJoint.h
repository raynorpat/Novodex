#ifndef NX_PHYSICS_CORE_PRISMATICJOINT
#define NX_PHYSICS_CORE_PRISMATICJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal PrismaticJoint object (NxJointType 0), recovered by
// joint-families Task 3a. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Prismatic" (object layout, 0x17c bytes, and the 13-slot internal table
// 0x1011a4d0, phys_data_002694).
//
// PrismaticJoint derives from Joint (Joint.h) as RevoluteJoint does: the base
// part (0x00-0x16b) IS a Joint (phys_fn_004141 is called first, 0xad6ee). It
// overrides Joint slots 4, 5 and 6, inherits slots 0-3, 7 and 8, and adds its
// own slots 9-12.

#include "Nxp.h"
#include "core/Joint.h"
#include "NxPrismaticJointDesc.h"

#include <cstddef>

class PrismaticJoint : public Joint
	{
	public:
	//! phys_fn_004380 (0x000ad6e0, 81 B). Joint(desc, 0x80), vptr
	//! 0x1011a4d0, allocates and constructs the 0x1c-byte NpPrismaticJoint,
	//! copies desc.userData to it, then phys_fn_004378. See "## Prismatic"
	//! "### Construction chain".
	PrismaticJoint(const NxPrismaticJointDesc& desc);

	//! phys_fn_004382 (0x000ad740, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~PrismaticJoint();

	// --- Joint slots PrismaticJoint overrides ---

	//! Slot 4 (+0x10). The folded debug-visualization row phys_fn_004318
	//! (core\CylindricalJoint.cpp; reads only Joint base fields), written
	//! once as Joint::row004318 by Task 3b; this override calls it.
	virtual void row_slot4(NxDebugRenderable& renderable);

	//! Slot 6 (+0x18). phys_fn_004386 (0x000ad850, 6772 B). The float
	//! argument is a divisor (0xae3aa-0xae3b3).
	virtual void row_slot6(NxReal arg);

	// --- PrismaticJoint's own slots, 9-12 (0x1011a4d0) ---

	//! Slot 9 (+0x24). phys_fn_004384 (0x000ad780, 202 B). loadFromDesc
	//! (strings "PrismaticJoint::loadFromDesc" x2).
	virtual void loadFromDesc(const NxPrismaticJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004376 (0x000ad4e0, 54 B). saveToDesc
	//! (string "PrismaticJoint::saveToDesc").
	virtual void saveToDesc(NxPrismaticJointDesc& desc);

	//! Slot 11 (+0x2c). phys_fn_001391 (folded; not claimed). Trivial
	//! `mov eax,ecx; ret` body.
	virtual PrismaticJoint* row_slot11() { return this; }

	//! Slot 12 (+0x30). phys_fn_001391 (folded; not claimed; same target).
	virtual PrismaticJoint* row_slot12() { return this; }

	// --- plain (non-virtual) members ---

	//! phys_fn_004378 (0x000ad520, 443 B). Called by phys_fn_004380
	//! (0xad725) and phys_fn_004384 (0xad840) with the descriptor, which it
	//! never reads (`ret 4`). Writes mUnknown16c = conj(conj(q0) * q1) for
	//! the bodies' +0x124 quaternions (identity for a missing body 0).
	void row004378(const NxPrismaticJointDesc& desc);

	// --- fields (the Joint base part is 0x00-0x16b) ---

	//! +0x16c. Quaternion x, y, z, w (name unknown): written only by
	//! phys_fn_004378, read only by phys_fn_004386.
	NxReal				mUnknown16c[4];
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxRevoluteJointAttachScene does for the
// revolute family. Not an oracle row. Defined in core/NpPrismaticJoint.cpp;
// internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxPrismaticJointAttachScene(PrismaticJoint* internal, void* writeLink, void* readLink);

static_assert(sizeof(PrismaticJoint) == 0x17c, "PrismaticJoint is 0x17c bytes in the oracle");
static_assert(offsetof(PrismaticJoint, mUnknown16c) == 0x16c, "the quaternion is at +0x16c");

#endif
