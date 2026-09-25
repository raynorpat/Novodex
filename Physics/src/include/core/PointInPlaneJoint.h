#ifndef NX_PHYSICS_CORE_POINTINPLANEJOINT
#define NX_PHYSICS_CORE_POINTINPLANEJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal PointInPlaneJoint object (NxJointType 5), recovered by
// joint-families Task 3e. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## PointInPlane" (object layout, 0x16c bytes, and the 13-slot internal
// table 0x10119b48, phys_data_002657).
//
// PointInPlaneJoint derives from Joint (Joint.h) as PointOnLineJoint does:
// the base part IS a Joint (phys_fn_004141 is called first, 0xa1b5b), and the
// class adds no field (createJoint allocates 0x16c bytes, sizeof(Joint)). It
// overrides Joint slots 4, 5 and 6, inherits slots 0-3, 7 and 8, and adds its
// own slots 9-12.

#include "Nxp.h"
#include "core/Joint.h"
#include "NxPointInPlaneJointDesc.h"

#include <cstddef>

class PointInPlaneJoint : public Joint
	{
	public:
	//! phys_fn_004262 (0x000a1b50, 84 B). Joint(desc, 2), vptr 0x10119b48,
	//! allocates and constructs the 0x1c-byte NpPointInPlaneJoint and copies
	//! desc.userData to it. See "## PointInPlane" "### Construction chain".
	PointInPlaneJoint(const NxPointInPlaneJointDesc& desc);

	//! phys_fn_004264 (0x000a1bb0, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~PointInPlaneJoint();

	// --- Joint slots PointInPlaneJoint overrides ---

	//! Slot 4 (+0x10). phys_fn_004260 (0x000a1650, 1273 B). Debug
	//! visualization through the renderable's addLine (+0x20).
	virtual void row_slot4(NxDebugRenderable& renderable);

	//! Slot 6 (+0x18). phys_fn_004258 (0x000a10e0, 1391 B). The solver slot;
	//! the float argument is a divisor (0xa14d9).
	virtual void row_slot6(NxReal arg);

	// --- PointInPlaneJoint's own slots, 9-12 (0x10119b48) ---

	//! Slot 9 (+0x24). phys_fn_004266 (0x000a1bf0, 194 B). loadFromDesc
	//! (strings "PointInPlaneJoint::loadFromDesc" x2).
	virtual void loadFromDesc(const NxPointInPlaneJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004256 (0x000a10a0, 54 B). saveToDesc
	//! (string "PointInPlaneJoint::saveToDesc").
	virtual void saveToDesc(NxPointInPlaneJointDesc& desc);

	//! Slots 11 and 12 (+0x2c, +0x30). phys_fn_001391 (folded; not claimed).
	//! Trivial `mov eax,ecx; ret` bodies, as prismatic's slots 11 and 12.
	virtual PointInPlaneJoint* row_slot11() { return this; }
	virtual PointInPlaneJoint* row_slot12() { return this; }
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxPrismaticJointAttachScene does for the
// prismatic family. Not an oracle row. Defined in
// core/NpPointInPlaneJoint.cpp; internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxPointInPlaneJointAttachScene(PointInPlaneJoint* internal, void* writeLink, void* readLink);

static_assert(sizeof(PointInPlaneJoint) == 0x16c, "PointInPlaneJoint is 0x16c bytes in the oracle (push 0x16c at 0x14472)");

#endif
