#ifndef NX_PHYSICS_CORE_DISTANCEJOINT
#define NX_PHYSICS_CORE_DISTANCEJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal DistanceJoint object (NxJointType 6), recovered by
// joint-families Task 3f. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Distance" (object layout, 0x184 bytes, and the 13-slot internal table
// 0x10119948, phys_data_002649).
//
// DistanceJoint derives from Joint (Joint.h) as PointInPlaneJoint does: the
// base part IS a Joint (phys_fn_004141 is called first, 0x9f47e). It adds the
// descriptor's four family fields at +0x16c..+0x180, overrides Joint slots
// 4, 5 and 6, inherits slots 0-3, 7 and 8, and adds its own slots 9-12.

#include "Nxp.h"
#include "core/Joint.h"
#include "NxSpringDesc.h"			// NxDistanceJointDesc.h uses NxSpringDesc without including it
#include "NxDistanceJointDesc.h"

#include <cstddef>

class DistanceJoint : public Joint
	{
	public:
	//! phys_fn_004234 (0x0009f470, 162 B). Joint(desc, 0x2000), vptr
	//! 0x10119948, the spring member's inline NxSpringDesc() (zeroes
	//! +0x174..+0x17c), allocates and constructs the 0x1c-byte
	//! NpDistanceJoint, copies desc.userData to it, then copies the four
	//! family fields. See "## Distance" "### Construction chain".
	DistanceJoint(const NxDistanceJointDesc& desc);

	//! phys_fn_004236 (0x0009f520, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~DistanceJoint();

	// --- Joint slots DistanceJoint overrides ---

	//! Slot 4 (+0x10). phys_fn_004232 (0x0009f230, 564 B). Debug
	//! visualization: one addLine (+0x20) between the two world anchors.
	virtual void row_slot4(NxDebugRenderable& renderable);

	//! Slot 6 (+0x18). phys_fn_004240 (0x0009f620, 2747 B). The solver
	//! slot; the float argument is the step divisor (0x9f8cf).
	virtual void row_slot6(NxReal arg);

	// --- DistanceJoint's own slots, 9-12 (0x10119948) ---

	//! Slot 9 (+0x24). phys_fn_004238 (0x0009f560, 188 B). loadFromDesc
	//! (string "DistanceJoint::loadFromDesc: desc.isValid() fails!").
	virtual void loadFromDesc(const NxDistanceJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004230 (0x0009f1c0, 109 B). saveToDesc
	//! (string "DistanceJoint::saveToDesc: joint is broken. ...").
	virtual void saveToDesc(NxDistanceJointDesc& desc);

	//! Slots 11 and 12 (+0x2c, +0x30). phys_fn_001391 (folded; not claimed).
	//! Trivial `mov eax,ecx; ret` bodies, as prismatic's slots 11 and 12.
	virtual DistanceJoint* row_slot11() { return this; }
	virtual DistanceJoint* row_slot12() { return this; }

	// --- fields (see "## Distance" "### Object layouts") ---

	//! +0x16c. NxDistanceJointDesc::maxDistance (desc+0x6c).
	NxReal				mMaxDistance;

	//! +0x170. NxDistanceJointDesc::minDistance (desc+0x70).
	NxReal				mMinDistance;

	//! +0x174. NxDistanceJointDesc::spring (desc+0x74): spring +0x174,
	//! damper +0x178, targetValue +0x17c. Its inline default constructor is
	//! the zeroing at 0x9f489-0x9f49d. 004240 reads spring and damper only.
	NxSpringDesc		mSpring;

	//! +0x180. NxDistanceJointDesc::flags (desc+0x80): NX_DJF_MAX_DISTANCE_ENABLED
	//! (bit 0), NX_DJF_MIN_DISTANCE_ENABLED (bit 1), NX_DJF_SPRING_ENABLED
	//! (bit 2), tested by 004240.
	NxU32				mDistanceFlags;
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxPrismaticJointAttachScene does for the
// prismatic family. Not an oracle row. Defined in core/NpDistanceJoint.cpp;
// internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxDistanceJointAttachScene(DistanceJoint* internal, void* writeLink, void* readLink);

static_assert(offsetof(DistanceJoint, mMaxDistance) == 0x16c, "maxDistance is at +0x16c (0x9f4d3)");
static_assert(offsetof(DistanceJoint, mMinDistance) == 0x170, "minDistance is at +0x170 (0x9f4dc)");
static_assert(offsetof(DistanceJoint, mSpring) == 0x174, "the spring is at +0x174 (0x9f4e7)");
static_assert(offsetof(DistanceJoint, mDistanceFlags) == 0x180, "the distance flags are at +0x180 (0x9f506)");
static_assert(sizeof(DistanceJoint) == 0x184, "DistanceJoint is 0x184 bytes in the oracle (push 0x184 at 0x144be)");
static_assert(offsetof(NxDistanceJointDesc, maxDistance) == 0x6c, "desc.maxDistance is at +0x6c (0x9f4d0)");
static_assert(offsetof(NxDistanceJointDesc, minDistance) == 0x70, "desc.minDistance is at +0x70 (0x9f4d9)");
static_assert(offsetof(NxDistanceJointDesc, spring) == 0x74, "desc.spring is at +0x74 (0x9f4e2)");
static_assert(offsetof(NxDistanceJointDesc, flags) == 0x80, "desc.flags is at +0x80 (0x9f4ff)");

#endif
