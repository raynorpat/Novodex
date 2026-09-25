#ifndef NX_PHYSICS_CORE_SPHERICALJOINT
#define NX_PHYSICS_CORE_SPHERICALJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal SphericalJoint object (NxJointType 3), recovered by
// joint-families Task 3c. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Spherical" (object layout, 0x23c bytes, and the 17-slot internal
// table 0x10119e20, phys_data_002668).
//
// SphericalJoint derives from Joint (Joint.h) as RevoluteJoint does: the
// base part IS a Joint (phys_fn_004141 is called first, 0xa48fc). It
// overrides Joint slots 0, 1 and 4-8, inherits slots 2 and 3, and adds its
// own slots 9-16 in the revolute table's shape (the flags/projection-mode
// quartet at 11-14), which is why the two Np bodies that read slots 12 and 14
// (phys_fn_004703/004707) serve both families.

#include "Nxp.h"
#include "core/Joint.h"
#include "NxSphericalJointDesc.h"
#include "NxJointLimitPairDesc.h"
#include "NxJointLimitDesc.h"
#include "NxSpringDesc.h"

#include <cstddef>

class SphericalJoint : public Joint
	{
	public:
	//! phys_fn_004300 (0x000a48f0, 194 B). Joint(desc, 8), vptr 0x10119e20,
	//! the limit/spring member defaults, allocates and constructs the
	//! 0x1c-byte NpSphericalJoint, copies desc.userData to it and calls
	//! phys_fn_004284. See "## Spherical" "### Construction chain".
	SphericalJoint(const NxSphericalJointDesc& desc);

	//! phys_fn_004302 (0x000a49c0, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~SphericalJoint();

	// --- Joint slots SphericalJoint overrides ---

	//! Slot 0 (+0x00). phys_fn_004308 (0x000a4f00, 1107 B). The point
	//! solve: revolute phys_fn_004374's shape over the spherical fields.
	//! `ret 4`; the listing never reads the argument.
	virtual void row_slot0(NxU32 arg);

	//! Slot 1 (+0x04). phys_fn_004282 (0x000a2bf0, 21 B). Zeroes
	//! +0x1f0..+0x1f8.
	virtual void row_slot1();

	//! Slot 4 (+0x10). phys_fn_004312 (0x000a5ee0, 4461 B) with its tail
	//! phys_fn_004314 (0x000a7050, 430 B). Debug visualization.
	virtual void row_slot4(NxDebugRenderable& renderable);

	//! Slot 6 (+0x18). phys_fn_004296 (0x000a3090, 5907 B). The float
	//! argument is a divisor (0xa32d1-0xa32db).
	virtual void row_slot6(NxReal arg);

	//! Slot 7 (+0x1c). phys_fn_004310 (0x000a5360, 2942 B). The float
	//! argument is a divisor; ends with Joint base slot 7 (phys_fn_004135).
	virtual void row_slot7(NxReal arg);

	//! Slot 8 (+0x20). phys_fn_004298 (0x000a47b0, 317 B). Projects the
	//! given body (one of mBody[0]/mBody[1]) back to projectionDistance.
	virtual void row_slot8(void* body);

	// --- SphericalJoint's own slots, 9-16 (0x10119e20) ---

	//! Slot 9 (+0x24). phys_fn_004304 (0x000a4a00, 186 B). loadFromDesc
	//! (strings "SphericalJoint::loadFromDesc" x2).
	virtual void loadFromDesc(const NxSphericalJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004286 (0x000a2dc0, 283 B). saveToDesc.
	virtual void saveToDesc(NxSphericalJointDesc& desc);

	//! Slot 11 (+0x2c). phys_fn_004288 (0x000a2ee0, 65 B). setFlags.
	virtual void setFlags(NxU32 flags);

	//! Slot 12 (+0x30). phys_fn_004290 (0x000a2f30, 7 B). Returns +0x1d0;
	//! reached from the Np getFlags body (phys_fn_004703) via [vt+0x30].
	virtual NxU32 getFlags() const;

	//! Slot 13 (+0x34). phys_fn_004292 (0x000a2f40, 62 B).
	//! setProjectionMode.
	virtual void setProjectionMode(NxJointProjectionMode mode);

	//! Slot 14 (+0x38). phys_fn_004186 (D6Joint.cpp, folded; not claimed).
	//! Returns the Joint base's projectionMode; called by the Np
	//! getProjectionMode body (phys_fn_004707).
	virtual NxJointProjectionMode getProjectionMode() const { return mProjectionMode; }

	//! Slot 15 (+0x3c). phys_fn_001391 (folded; not claimed).
	virtual SphericalJoint* row_slot15() { return this; }

	//! Slot 16 (+0x40). phys_fn_001391 (folded; not claimed; same target).
	virtual SphericalJoint* row_slot16() { return this; }

	// --- plain (non-virtual) SphericalJoint members ---

	//! phys_fn_004284 (0x000a2c10, 427 B). Spherical part of loadFromDesc:
	//! desc+0x6c..+0xc8 -> +0x16c..+0x1d4 and +0x44. Called by
	//! phys_fn_004300 (0xa49a5) and phys_fn_004304 (0xa4ab0).
	void row004284(const NxSphericalJointDesc& desc);

	//! phys_fn_004294 (0x000a2f80, 269 B). Called by phys_fn_004308; the
	//! velocity error at the anchor levers (revolute phys_fn_004358's
	//! instructions). `ret 4`: one output vec3.
	void row004294(NxVec3& out) const;

	//! phys_fn_004306 (0x000a4ac0, 1075 B). Called by phys_fn_004310 and
	//! phys_fn_004312. `ret 8`: writes the half-way axis and the cone
	//! factor, and returns the twist angle unrounded in st(0), hence NxF64.
	NxF64 row004306(NxVec3& halfAxis, NxReal& coneFactor);

	// --- fields, in the oracle's byte-offset order (the base part,
	//     0x00-0x16b, IS Joint; the fields below start at +0x16c) ---

	//! +0x16c. twistLimit (NxJointLimitPairDesc copy).
	NxJointLimitPairDesc	mTwistLimit;

	//! +0x184. swingLimit (NxJointLimitDesc copy).
	NxJointLimitDesc		mSwingLimit;

	//! +0x190. twistSpring.
	NxSpringDesc			mTwistSpring;

	//! +0x19c. swingSpring.
	NxSpringDesc			mSwingSpring;

	//! +0x1a8. jointSpring.
	NxSpringDesc			mJointSpring;

	//! +0x1b4. swingAxis (joint space of body 0).
	NxVec3					mSwingAxis;

	//! +0x1c0. The swing axis carried into world space through body 0's
	//! world frame (phys_fn_004284 0xa2cdc-0xa2d91). Name unknown.
	NxVec3					mSwingAxisWorld;

	//! +0x1cc. fcos(swingLimit.value) (phys_fn_004284 0xa2c63-0xa2c73).
	NxReal					mSwingLimitCos;

	//! +0x1d0. flags (NX_SJF_*).
	NxU32					mSphericalFlags;

	//! +0x1d4. projectionDistance.
	NxReal					mProjectionDistance;

	//! +0x1d8 / +0x1e4. The bodies' anchor levers (written by
	//! phys_fn_004296; read by phys_fn_004294, phys_fn_004308).
	NxVec3					mLever[2];

	//! +0x1f0. The velocity bias (phys_fn_004296); zeroed by slot 1.
	NxVec3					mBias;

	//! +0x1fc. Per-axis joint-spring gain (phys_fn_004296, used by
	//! phys_fn_004308 when flag 0x10 is set).
	NxVec3					mSpringGain;

	//! +0x208. Row-major 3x3: the inverse of the point constraint's
	//! effective mass (phys_fn_004296; read by phys_fn_004308).
	NxReal					mInverseMass[9];

	//! +0x22c. Accumulated impulse (phys_fn_004296 zeroes it,
	//! phys_fn_004308 accumulates and clamps it).
	NxVec3					mAccumulatedImpulse;

	//! +0x238. maxForce^2, or 1.1920929e-7f when that is 0 (phys_fn_004296).
	NxReal					mMaxImpulseSquared;
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxCylindricalJointAttachScene does for the
// cylindrical family. Not an oracle row. Defined in
// core/NpSphericalJoint.cpp; internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxSphericalJointAttachScene(SphericalJoint* internal, void* writeLink, void* readLink);

static_assert(sizeof(SphericalJoint) == 0x23c, "SphericalJoint is 0x23c bytes in the oracle");
static_assert(sizeof(NxJointLimitPairDesc) == 0x18 && sizeof(NxJointLimitDesc) == 0xc && sizeof(NxSpringDesc) == 0xc, "the descriptor blocks keep the oracle sizes");
static_assert(offsetof(SphericalJoint, mTwistLimit) == 0x16c, "twistLimit is at +0x16c");
static_assert(offsetof(SphericalJoint, mSwingLimit) == 0x184, "swingLimit is at +0x184");
static_assert(offsetof(SphericalJoint, mTwistSpring) == 0x190, "twistSpring is at +0x190");
static_assert(offsetof(SphericalJoint, mSwingSpring) == 0x19c, "swingSpring is at +0x19c");
static_assert(offsetof(SphericalJoint, mJointSpring) == 0x1a8, "jointSpring is at +0x1a8");
static_assert(offsetof(SphericalJoint, mSwingAxis) == 0x1b4, "swingAxis is at +0x1b4");
static_assert(offsetof(SphericalJoint, mSwingAxisWorld) == 0x1c0, "the world swing axis is at +0x1c0");
static_assert(offsetof(SphericalJoint, mSwingLimitCos) == 0x1cc, "cos(swingLimit) is at +0x1cc");
static_assert(offsetof(SphericalJoint, mSphericalFlags) == 0x1d0, "the spherical flags are at +0x1d0");
static_assert(offsetof(SphericalJoint, mProjectionDistance) == 0x1d4, "projectionDistance is at +0x1d4");
static_assert(offsetof(SphericalJoint, mLever) == 0x1d8, "the levers are at +0x1d8");
static_assert(offsetof(SphericalJoint, mBias) == 0x1f0, "the bias is at +0x1f0");
static_assert(offsetof(SphericalJoint, mSpringGain) == 0x1fc, "the spring gain is at +0x1fc");
static_assert(offsetof(SphericalJoint, mInverseMass) == 0x208, "the inverse mass 3x3 is at +0x208");
static_assert(offsetof(SphericalJoint, mAccumulatedImpulse) == 0x22c, "the accumulated impulse is at +0x22c");
static_assert(offsetof(SphericalJoint, mMaxImpulseSquared) == 0x238, "maxForce^2 is at +0x238");

#endif
