#ifndef NX_PHYSICS_CORE_FIXEDJOINT
#define NX_PHYSICS_CORE_FIXEDJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal FixedJoint object (NxJointType 8), recovered by
// joint-families Task 3h. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Fixed" (object layout, 0x188 bytes, and the 13-slot internal table
// 0x10119a50, phys_data_002653).
//
// FixedJoint derives from Joint (Joint.h) as the other families do: the base
// part IS a Joint (phys_fn_004141 is called first, 0xa0f7e). It adds the
// bodies' relative pose at +0x16c..+0x184, overrides Joint slots 4 (with the
// folded no-op), 5 and 6, inherits slots 0-3, 7 and 8, and adds its own slots
// 9-12.

#include "Nxp.h"
#include "core/Joint.h"
#include "NxFixedJointDesc.h"

#include <cstddef>

class FixedJoint : public Joint
	{
	public:
	//! phys_fn_004250 (0x000a0f70, 81 B). Joint(desc, 0x200), vptr
	//! 0x10119a50, allocates and constructs the 0x1c-byte NpFixedJoint,
	//! copies desc.userData to it, then recordRelativePose (phys_fn_004244).
	//! See "## Fixed" "### Construction chain".
	FixedJoint(const NxFixedJointDesc& desc);

	//! phys_fn_004252 (0x000a0fd0, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~FixedJoint();

	// --- Joint slots FixedJoint overrides ---

	//! Slot 4 (+0x10). The folded empty body phys_fn_004248 ("ret 4"; not
	//! claimed, inline here as Joint's slots 0 and 8 are): the fixed joint
	//! has no debug visualization. Joint declares the slot pure.
	virtual void row_slot4(NxDebugRenderable& renderable) { (void)renderable; }

	//! Slot 6 (+0x18). phys_fn_004246 (0x000a03f0, 2922 B). The solver
	//! slot; the float argument is the step divisor (0xa0568).
	virtual void row_slot6(NxReal arg);

	// --- FixedJoint's own slots, 9-12 (0x10119a50) ---

	//! Slot 9 (+0x24). phys_fn_004254 (0x000a1010, 141 B). loadFromDesc
	//! (string "FixedJoint::loadFromDesc: desc.isValid() fails!").
	virtual void loadFromDesc(const NxFixedJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004242 (0x000a00e0, 42 B). saveToDesc
	//! (string "FixedJoint::saveToDesc: joint is broken. ...").
	virtual void saveToDesc(NxFixedJointDesc& desc);

	//! Slots 11 and 12 (+0x2c, +0x30). phys_fn_001391 (folded; not
	//! claimed). Trivial `mov eax,ecx; ret` bodies, as prismatic's two.
	virtual FixedJoint* row_slot11() { return this; }
	virtual FixedJoint* row_slot12() { return this; }

	//! phys_fn_004244 (0x000a0110, 723 B). A separate thiscall row (`ret 4`)
	//! the constructor (0xa0fb5) and loadFromDesc (0xa1093) call: records
	//! body 1's position and orientation relative to body 0 in
	//! mRelativePosition and mRelativeRotation. The descriptor is never read.
	void recordRelativePose(const NxFixedJointDesc& desc);

	// --- fields (see "## Fixed" "### Object layouts") ---

	//! +0x16c. Body 1's +0x158 position minus body 0's, taken into body 0's
	//! frame through the transpose of its +0x134 3x3 (004244).
	NxVec3				mRelativePosition;

	//! +0x178. x, y, z, w: conj(q0) * q1 of the bodies' +0x124 quaternions,
	//! then its vector part negated (004244). Raw storage, as
	//! Joint::mFrameQuat.
	NxReal				mRelativeRotation[4];
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxPulleyJointAttachScene does for the
// pulley family. Not an oracle row. Defined in core/NpFixedJoint.cpp;
// internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxFixedJointAttachScene(FixedJoint* internal, void* writeLink, void* readLink);

static_assert(offsetof(FixedJoint, mRelativePosition) == 0x16c, "the relative position is at +0x16c (0xa0211)");
static_assert(offsetof(FixedJoint, mRelativeRotation) == 0x178, "the relative rotation is at +0x178 (0xa0230)");
static_assert(sizeof(FixedJoint) == 0x188, "FixedJoint is 0x188 bytes in the oracle (push 0x188 at 0x14498)");

#endif
