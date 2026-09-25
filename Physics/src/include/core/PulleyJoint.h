#ifndef NX_PHYSICS_CORE_PULLEYJOINT
#define NX_PHYSICS_CORE_PULLEYJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal PulleyJoint object (NxJointType 7), recovered by
// joint-families Task 3g. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Pulley" (object layout, 0x1e0 bytes, and the 14-slot internal table
// 0x10119840, phys_data_002645).
//
// PulleyJoint derives from Joint (Joint.h) as DistanceJoint does: the base
// part IS a Joint (phys_fn_004141 is called first, 0x9ea6e). It adds the
// descriptor's family fields at +0x16c..+0x190 and the solver state at
// +0x194..+0x1dc, overrides Joint slots 0, 1, 4, 5 and 6, inherits slots 2,
// 3, 7 and 8, and adds its own slots 9-13.

#include "Nxp.h"
#include "core/Joint.h"
#include "NxPulleyJointDesc.h"

#include <cstddef>

struct JointSupportBody;

class PulleyJoint : public Joint
	{
	public:
	//! phys_fn_004222 (0x0009ea60, 81 B). Joint(desc, 0x1000), vptr
	//! 0x10119840, allocates and constructs the 0x1c-byte NpPulleyJoint,
	//! copies desc.userData to it, then copyFamilyFields (phys_fn_004218).
	//! See "## Pulley" "### Construction chain".
	PulleyJoint(const NxPulleyJointDesc& desc);

	//! phys_fn_004224 (0x0009eac0, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~PulleyJoint();

	// --- Joint slots PulleyJoint overrides ---

	//! Slot 0 (+0x00). phys_fn_004219 (0x0009e4e0, 805 B). The impulse
	//! slot: `ret 4`, the argument is never read.
	virtual void row_slot0(NxU32 arg);

	//! Slot 1 (+0x04). phys_fn_004214 (0x0009e3c0, 11 B). mBias = 0.
	virtual void row_slot1();

	//! Slot 4 (+0x10). phys_fn_004221 (0x0009e810, 592 B). Debug
	//! visualization: one addLine (+0x20) from each world anchor to its
	//! pulley point.
	virtual void row_slot4(NxDebugRenderable& renderable);

	//! Slot 6 (+0x18). phys_fn_004228 (0x0009eb90, 1572 B). The solver
	//! slot; the float argument is the step divisor (0x9ed6d).
	virtual void row_slot6(NxReal arg);

	// --- PulleyJoint's own slots, 9-13 (0x10119840) ---

	//! Slot 9 (+0x24). phys_fn_004226 (0x0009eb00, 141 B). loadFromDesc
	//! (string "PulleyJoint::loadFromDesc: desc.isValid() fails!").
	virtual void loadFromDesc(const NxPulleyJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004216 (0x0009e3d0, 155 B). saveToDesc
	//! (string "PulleyJoint::saveToDesc: joint is broken. ...").
	virtual void saveToDesc(NxPulleyJointDesc& desc);

	//! Slots 11-13 (+0x2c, +0x30, +0x34). phys_fn_001391 (folded; not
	//! claimed). Trivial `mov eax,ecx; ret` bodies, as cylindrical's three.
	virtual PulleyJoint* row_slot11() { return this; }
	virtual PulleyJoint* row_slot12() { return this; }
	virtual PulleyJoint* row_slot13() { return this; }

	//! phys_fn_004218 (0x0009e470, 112 B). A separate thiscall row (`ret 4`)
	//! the constructor (0x9eaa5) and loadFromDesc (0x9eb83) call: copies the
	//! ten family words desc+0x6c..+0x90 to +0x16c..+0x190.
	void copyFamilyFields(const NxPulleyJointDesc& desc);

	// --- fields (see "## Pulley" "### Object layouts") ---

	//! +0x16c. NxPulleyJointDesc::pulley (desc+0x6c): the two suspension
	//! points in world space.
	NxVec3				mPulley[2];

	//! +0x184. NxPulleyJointDesc::distance (desc+0x84): the rest length.
	NxReal				mDistance;

	//! +0x188. NxPulleyJointDesc::stiffness (desc+0x88).
	NxReal				mStiffness;

	//! +0x18c. NxPulleyJointDesc::ratio (desc+0x8c).
	NxReal				mRatio;

	//! +0x190. NxPulleyJointDesc::flags (desc+0x90, NX_PJF_IS_RIGID). No
	//! row tests it.
	NxU32				mPulleyFlags;

	//! +0x194 / +0x198. The bodies' +0x204 support records (null without a
	//! body), written by 004228 (0x9ed8f, 0x9eda3) and read by 004219.
	JointSupportBody*	mSupport[2];

	//! +0x19c / +0x1a8. The unit directions from each world point to its
	//! pulley (004228 0x9ee5d-0x9ee87).
	NxVec3				mDirection[2];

	//! +0x1b4 / +0x1c0. lever x direction per body (004228 0x9eecb-0x9ef31).
	NxVec3				mCross[2];

	//! +0x1cc. (distance - (len0 + len1 ratio)) (stiffness / arg), written
	//! by 004228 (0x9ed76), zeroed by 004214, read by 004219.
	NxReal				mBias;

	//! +0x1d0. 1 / the effective mass, or 0 (004228 0x9f19a).
	NxReal				mInverseMass;

	//! +0x1d4. The unrounded inverse times 0.7f (004228 0x9f1a6).
	NxReal				mScaledInverseMass;

	//! +0x1d8 / +0x1dc. The sums of the applied and of the unbiased impulses
	//! (004219 0x9e5b1 / 0x9e58f); 004228 zeroes both.
	NxReal				mAccumulated[2];
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxDistanceJointAttachScene does for the
// distance family. Not an oracle row. Defined in core/NpPulleyJoint.cpp;
// internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxPulleyJointAttachScene(PulleyJoint* internal, void* writeLink, void* readLink);

static_assert(offsetof(PulleyJoint, mPulley) == 0x16c, "the pulley points are at +0x16c (0x9e3f9)");
static_assert(offsetof(PulleyJoint, mDistance) == 0x184, "distance is at +0x184 (0x9e432)");
static_assert(offsetof(PulleyJoint, mStiffness) == 0x188, "stiffness is at +0x188 (0x9e43e)");
static_assert(offsetof(PulleyJoint, mRatio) == 0x18c, "ratio is at +0x18c (0x9e44a)");
static_assert(offsetof(PulleyJoint, mPulleyFlags) == 0x190, "the pulley flags are at +0x190 (0x9e456)");
static_assert(offsetof(PulleyJoint, mSupport) == 0x194, "the support records are at +0x194 (0x9ed8f)");
static_assert(offsetof(PulleyJoint, mDirection) == 0x19c, "the directions are at +0x19c (0x9ee5d)");
static_assert(offsetof(PulleyJoint, mCross) == 0x1b4, "the crosses are at +0x1b4 (0x9eecb)");
static_assert(offsetof(PulleyJoint, mBias) == 0x1cc, "the bias is at +0x1cc (0x9ed76)");
static_assert(offsetof(PulleyJoint, mInverseMass) == 0x1d0, "the inverse mass is at +0x1d0 (0x9f19a)");
static_assert(offsetof(PulleyJoint, mScaledInverseMass) == 0x1d4, "the scaled inverse mass is at +0x1d4 (0x9f1a6)");
static_assert(offsetof(PulleyJoint, mAccumulated) == 0x1d8, "the impulse sums are at +0x1d8 (0x9ef3f)");
static_assert(sizeof(PulleyJoint) == 0x1e0, "PulleyJoint is 0x1e0 bytes in the oracle (push 0x1e0 at 0x144e4)");
static_assert(offsetof(NxPulleyJointDesc, pulley) == 0x6c, "desc.pulley is at +0x6c (0x9e3ff)");
static_assert(offsetof(NxPulleyJointDesc, distance) == 0x84, "desc.distance is at +0x84 (0x9e438)");
static_assert(offsetof(NxPulleyJointDesc, stiffness) == 0x88, "desc.stiffness is at +0x88 (0x9e444)");
static_assert(offsetof(NxPulleyJointDesc, ratio) == 0x8c, "desc.ratio is at +0x8c (0x9e450)");
static_assert(offsetof(NxPulleyJointDesc, flags) == 0x90, "desc.flags is at +0x90 (0x9e45c)");

#endif
