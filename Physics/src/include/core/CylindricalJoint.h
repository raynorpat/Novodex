#ifndef NX_PHYSICS_CORE_CYLINDRICALJOINT
#define NX_PHYSICS_CORE_CYLINDRICALJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal CylindricalJoint object (NxJointType 2), recovered by
// joint-families Task 3b. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Cylindrical" (object layout, 0x16c bytes, and the 14-slot internal
// table 0x1011a048, phys_data_002678).
//
// CylindricalJoint derives from Joint (Joint.h) as PrismaticJoint does: the
// base part IS a Joint (phys_fn_004141 is called first, 0xa76ae), and the
// class adds no field (Scene::createJoint allocates 0x16c = sizeof(Joint),
// 0x143f7). It overrides Joint slots 4, 5 and 6, inherits slots 0-3, 7 and 8,
// and adds its own slots 9-13.

#include "Nxp.h"
#include "core/Joint.h"
#include "NxCylindricalJointDesc.h"

#include <cstddef>

class CylindricalJoint : public Joint
	{
	public:
	//! phys_fn_004320 (0x000a76a0, 87 B). Joint(desc, 0x100), vptr
	//! 0x1011a048, allocates and constructs the 0x1c-byte NpCylindricalJoint
	//! and copies desc.userData to it. See "## Cylindrical"
	//! "### Construction chain".
	CylindricalJoint(const NxCylindricalJointDesc& desc);

	//! phys_fn_004322 (0x000a7700, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~CylindricalJoint();

	// --- Joint slots CylindricalJoint overrides ---

	//! Slot 4 (+0x10). phys_fn_004318 (0x000a7240, 1115 B), written once as
	//! Joint::row004318 because the prismatic table names the same body.
	virtual void row_slot4(NxDebugRenderable& renderable);

	//! Slot 6 (+0x18). phys_fn_004326 (0x000a7810, 5377 B). The float
	//! argument is a divisor (0xa832b-0xa8334).
	virtual void row_slot6(NxReal arg);

	// --- CylindricalJoint's own slots, 9-13 (0x1011a048) ---

	//! Slot 9 (+0x24). phys_fn_004324 (0x000a7740, 194 B). loadFromDesc
	//! (strings "CylindricalJoint::loadFromDesc" x2).
	virtual void loadFromDesc(const NxCylindricalJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004316 (0x000a7200, 54 B). saveToDesc (its
	//! string also says "CylindricalJoint::loadFromDesc").
	virtual void saveToDesc(NxCylindricalJointDesc& desc);

	//! Slot 11 (+0x2c). phys_fn_001391 (folded; not claimed). Trivial
	//! `mov eax,ecx; ret` body.
	virtual CylindricalJoint* row_slot11() { return this; }

	//! Slot 12 (+0x30). phys_fn_001391 (folded; not claimed; same target).
	virtual CylindricalJoint* row_slot12() { return this; }

	//! Slot 13 (+0x34). phys_fn_001391 (folded; not claimed; same target).
	//! The table's last word (0x11a07c) before the unit's __FILE__ string;
	//! meaning unknown, declared for the oracle's slot count.
	virtual CylindricalJoint* row_slot13() { return this; }
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxPrismaticJointAttachScene does for the
// prismatic family. Not an oracle row. Defined in core/NpCylindricalJoint.cpp;
// internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxCylindricalJointAttachScene(CylindricalJoint* internal, void* writeLink, void* readLink);

static_assert(sizeof(CylindricalJoint) == 0x16c, "CylindricalJoint is 0x16c bytes in the oracle");

#endif
