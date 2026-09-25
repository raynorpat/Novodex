#ifndef NX_PHYSICS_CORE_POINTONLINEJOINT
#define NX_PHYSICS_CORE_POINTONLINEJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal PointOnLineJoint object (NxJointType 4), recovered by
// joint-families Task 3d. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## PointOnLine" (object layout, 0x16c bytes, and the 13-slot internal
// table 0x10119cb0, phys_data_002662).
//
// PointOnLineJoint derives from Joint (Joint.h) as PrismaticJoint does: the
// base part IS a Joint (phys_fn_004141 is called first, 0xa2a8b), and the
// class adds no field (createJoint allocates 0x16c bytes, sizeof(Joint)). It
// overrides Joint slots 4, 5 and 6, inherits slots 0-3, 7 and 8, and adds its
// own slots 9-12.

#include "Nxp.h"
#include "core/Joint.h"
#include "NxPointOnLineJointDesc.h"

#include <cstddef>

class PointOnLineJoint : public Joint
	{
	public:
	//! phys_fn_004276 (0x000a2a80, 84 B). Joint(desc, 4), vptr 0x10119cb0,
	//! allocates and constructs the 0x1c-byte NpPointOnLineJoint and copies
	//! desc.userData to it. See "## PointOnLine" "### Construction chain".
	PointOnLineJoint(const NxPointOnLineJointDesc& desc);

	//! phys_fn_004278 (0x000a2ae0, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~PointOnLineJoint();

	// --- Joint slots PointOnLineJoint overrides ---

	//! Slot 4 (+0x10). phys_fn_004274 (0x000a2530, 1345 B). Debug
	//! visualization through the renderable's addLine (+0x20).
	virtual void row_slot4(NxDebugRenderable& renderable);

	//! Slot 6 (+0x18). phys_fn_004272 (0x000a1d10, 2073 B). The solver slot;
	//! the float argument is a divisor (0xa2200-0xa2209).
	virtual void row_slot6(NxReal arg);

	// --- PointOnLineJoint's own slots, 9-12 (0x10119cb0) ---

	//! Slot 9 (+0x24). phys_fn_004280 (0x000a2b20, 194 B). loadFromDesc
	//! (strings "PointOnLineJoint::loadFromDesc" x2).
	virtual void loadFromDesc(const NxPointOnLineJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004268 (0x000a1cc0, 54 B). saveToDesc
	//! (string "PointOnLineJoint::saveToDesc").
	virtual void saveToDesc(NxPointOnLineJointDesc& desc);

	//! Slot 11 (+0x2c). phys_fn_004270 (0x000a1d00, 5 B). `mov eax,[ecx];
	//! jmp [eax+0x2c]`: calls slot 11 of `this`, i.e. itself. Nothing calls
	//! it. The return type is unknown; declared as the neighbouring slot's.
	virtual PointOnLineJoint* row_slot11();

	//! Slot 12 (+0x30). phys_fn_001391 (folded; not claimed). Trivial
	//! `mov eax,ecx; ret` body.
	virtual PointOnLineJoint* row_slot12() { return this; }
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxPrismaticJointAttachScene does for the
// prismatic family. Not an oracle row. Defined in core/NpPointOnLineJoint.cpp;
// internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxPointOnLineJointAttachScene(PointOnLineJoint* internal, void* writeLink, void* readLink);

static_assert(sizeof(PointOnLineJoint) == 0x16c, "PointOnLineJoint is 0x16c bytes in the oracle (push 0x16c at 0x14449)");

#endif
