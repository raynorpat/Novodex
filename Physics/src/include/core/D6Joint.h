#ifndef NX_PHYSICS_CORE_D6JOINT
#define NX_PHYSICS_CORE_D6JOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal D6Joint object (NxJointType 9), recovered by joint-families
// Task 3i. See docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## D6" (object layout, 0x270 bytes, and the 16-slot internal table
// 0x10119570, phys_data_002623).
//
// D6Joint derives from Joint (Joint.h) as the other families do: the base
// part IS a Joint (phys_fn_004141 is called first, 0x9e1af). It adds the
// descriptor's family fields at +0x16c..+0x25f, the three cosines of the
// half limit angles and two "is limited" flags, overrides Joint slots 4, 5, 6
// and 8, inherits slots 0-3 and 7, and adds its own slots 9-15.
//
// The image's __FILE__ for this unit is "...\Physics\src\D6Joint.cpp" (not
// under core\); the file lives in core/ as the pilot's Joint.cpp does. See
// the contract's "### File placement".

#include "Nxp.h"
#include "core/Joint.h"
// NxD6JointDesc.h uses NxBitField32 (NxJointDriveDesc::driveType) without
// including its header.
#include "NxBitField.h"
#include "NxD6JointDesc.h"
#include "NxQuat.h"

#include <cstdio>
#include <cstddef>

// A rigid pose as the D6 rows keep it on the stack: the quaternion x, y, z, w
// then the position (7 floats, 0x1c bytes). 004206 builds three of them
// (body 0's +0x124/+0x158, body 1's, and each local frame from the Joint
// base's frame quaternion and anchor) and passes them to 004178/004180.
struct D6JointPose
	{
	NxReal				q[4];	//!< +0x00: x, y, z, w
	NxVec3				p;		//!< +0x10

	//! phys_fn_004178 (0x0009b270, 396 B). Thiscall on the left pose,
	//! `ret 8`: out = this * other (out.q = this.q * other.q, out.p =
	//! this.q rotates other.p, plus this.p). Every product reads the
	//! operands, so out may not alias them. Returns `out` in eax (0x9b3cd);
	//! 004207 chains the result (0x9e030) and reads through it (0x9e037).
	D6JointPose*		row004178(D6JointPose& out, const D6JointPose& other) const;

	//! phys_fn_004180 (0x0009b400, 330 B). Thiscall, `ret 4`: out = the
	//! inverse of this pose (conjugate quaternion, the position rotated by it
	//! and negated). Returns `out` in eax (004206 passes it straight on as
	//! the next call's `this`, 0x9d02a).
	D6JointPose*		row004180(D6JointPose& out) const;
	};

static_assert(offsetof(D6JointPose, p) == 0x10, "the pose position follows the quaternion");
static_assert(sizeof(D6JointPose) == 0x1c, "a pose is seven floats");

// One record of the solver dump (0x30 bytes): 004194/004196 append one per
// constraint they build to the .data array at 0x10127198 (count 0x10127190),
// and 004192 prints them through 004188.
struct D6JointDumpRecord
	{
	NxVec3				ra;			//!< +0x00 (zero for an angular record)
	NxVec3				rb;			//!< +0x0c (zero for an angular record)
	NxVec3				normal;		//!< +0x18
	NxReal				bias;		//!< +0x24
	NxReal				maxForce;	//!< +0x28
	bool				angular;	//!< +0x2c; 004194 stores 0, 004196 stores 1

	//! phys_fn_004188 (0x0009b5e0, 181 B). Thiscall on the record, `ret 4`:
	//! prints it to the stream.
	void				print(FILE* stream) const;
	};

static_assert(offsetof(D6JointDumpRecord, normal) == 0x18, "the dumped normal is at +0x18");
static_assert(offsetof(D6JointDumpRecord, bias) == 0x24, "the dumped bias is at +0x24");
static_assert(offsetof(D6JointDumpRecord, angular) == 0x2c, "the angular flag is at +0x2c");
static_assert(sizeof(D6JointDumpRecord) == 0x30, "dump records are 0x30 bytes");

class D6Joint : public Joint
	{
	public:
	//! phys_fn_004210 (0x0009e1a0, 331 B). Joint(desc, 0x4000), vptr
	//! 0x10119570, the members' inline constructors, row004204, then
	//! allocates and constructs the 0x1c-byte NpD6Joint and copies
	//! desc.userData to it. See "## D6" "### Construction chain".
	D6Joint(const NxD6JointDesc& desc);

	//! phys_fn_004202 (0x0009c8a0, 56 B). Internal slot 5: the scalar
	//! deleting destructor. Deletes [this+0x48] through its slot 0 with flag
	//! 1, then calls the base destructor body (phys_fn_004095) directly.
	virtual ~D6Joint();

	// --- Joint slots D6Joint overrides ---

	//! Slot 4 (+0x10). phys_fn_004200 (0x0009c060, 2098 B). Debug
	//! visualization.
	virtual void row_slot4(NxDebugRenderable& renderable);

	//! Slot 6 (+0x18). phys_fn_004206 (0x0009cba0, 3200 B). The solver slot;
	//! the float argument is the step divisor (`fdiv [esp+0x15c]`, 0x9d0c5).
	virtual void row_slot6(NxReal arg);

	//! Slot 8 (+0x20). phys_fn_004207 (0x0009d820, 2394 B). Projection: the
	//! argument is one of the two body records.
	virtual void row_slot8(void* body);

	// --- D6Joint's own slots, 9-15 (0x10119570) ---

	//! Slot 9 (+0x24). phys_fn_004212 (0x0009e2f0, 194 B). loadFromDesc
	//! (strings "D6Joint::loadFromDesc: ...", lines 0x5f/0x60).
	virtual void loadFromDesc(const NxD6JointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004182 (0x0009b550, 57 B). saveToDesc
	//! (line 0xaa); saves the base fields only.
	virtual void saveToDesc(NxD6JointDesc& desc);

	//! Slot 11 (+0x2c). phys_fn_004184 (0x0009b590, 62 B).
	//! setProjectionMode (line 0xb1).
	virtual void setProjectionMode(NxJointProjectionMode mode);

	//! Slot 12 (+0x30). phys_fn_004186 (folded; not claimed, as in
	//! RevoluteJoint.h/SphericalJoint.h). Returns the Joint base's
	//! projectionMode.
	virtual NxJointProjectionMode getProjectionMode() const { return mProjectionMode; }

	//! Slots 13-15 (+0x34..+0x3c). phys_fn_001391 (folded; not claimed).
	virtual D6Joint* row_slot13() { return this; }
	virtual D6Joint* row_slot14() { return this; }
	virtual D6Joint* row_slot15() { return this; }

	// --- plain (non-virtual) D6Joint members ---

	//! The four drive setters the public rows 004461-004467 call directly
	//! (`mov ecx,[esi+0x18]; call 0x100a0f60`, 0xb0a93): the folded empty
	//! body phys_fn_004248 (`ret 4`; not claimed). The drive state is not
	//! stored.
	void setDrivePosition(const NxVec3& position) { (void)position; }
	void setDriveOrientation(const NxQuat& orientation) { (void)orientation; }
	void setDriveLinearVelocity(const NxVec3& linVel) { (void)linVel; }
	void setDriveAngularVelocity(const NxVec3& angVel) { (void)angVel; }

	//! phys_fn_004204 (0x0009c8e0, 698 B). Thiscall, `ret 4`: copies the
	//! descriptor's family fields, forms the half-angle cosines of the
	//! limited angular motions and the two "limited" flags, and stores the
	//! projection mode/distance/angle. Called by the constructor (0x9e2a7)
	//! and loadFromDesc (0x9e3a7).
	void row004204(const NxD6JointDesc& desc);

	//! phys_fn_004194 (0x0009b860, 514 B). Thiscall, `ret 0x18`: appends a
	//! dump record and builds one linear constraint record (kind 1 when
	//! `limit` is 0, else 0) with the levers ra/rb and the normal.
	void row004194(NxU32 limit, const NxVec3& ra, const NxVec3& rb, const NxVec3& normal, NxReal bias,
		NxReal maxForce);

	//! phys_fn_004196 (0x0009ba70, 349 B). Thiscall, `ret 0x10`: appends a
	//! dump record and builds one angular constraint record (kind 3 when
	//! `limit` is 0, else 2) along the axis.
	void row004196(NxU32 limit, const NxVec3& axis, NxReal bias, NxReal maxForce);

	// --- fields (see "## D6" "### Object layouts") ---

	//! +0x16c. x, y, z, twist, swing1, swing2 (desc+0x6c..+0x80).
	NxD6JointMotion		mMotion[6];
	//! +0x184. desc.linearLimit (desc+0x84).
	NxJointLimitDesc	mLinearLimit;
	//! +0x190. desc.swing1Limit (desc+0xa8).
	NxJointLimitDesc	mSwing1Limit;
	//! +0x19c. desc.swing2Limit (desc+0xb4).
	NxJointLimitDesc	mSwing2Limit;
	//! +0x1a8. desc.twistLimit (desc+0x90).
	NxJointLimitPairDesc	mTwistLimit;
	//! +0x1c0. x, y, z, swing, twist, spherical drives; row004204 copies the
	//! first three (desc+0xc0..+0xef) only.
	NxJointDriveDesc	mDrive[6];
	//! +0x220. desc.useSpherical.
	bool				mUseSpherical;
	//! +0x224. desc.drivePosition.
	NxVec3				mDrivePosition;
	//! +0x230. desc.driveOrientation.
	NxQuat				mDriveOrientation;
	//! +0x240. desc.driveLinearVelocity.
	NxVec3				mDriveLinearVelocity;
	//! +0x24c. desc.driveAngularVelocity.
	NxVec3				mDriveAngularVelocity;
	//! +0x258. desc.projectionDistance.
	NxReal				mProjectionDistance;
	//! +0x25c. desc.projectionAngle.
	NxReal				mProjectionAngle;
	//! +0x260 / +0x264 / +0x268. cos(0.5 * swing1Limit.value),
	//! cos(0.5 * swing2Limit.value), cos(0.5 * twistLimit.high.value);
	//! written only for a LIMITED motion.
	NxReal				mSwing1CosHalf;
	NxReal				mSwing2CosHalf;
	NxReal				mTwistCosHalf;
	//! +0x26c. Any of twist/swing1/swing2 is LIMITED.
	bool				mAngularLimited;
	//! +0x26d. Any of x/y/z is LIMITED.
	bool				mLinearLimited;
	};

// Attaches the public object Scene::createJoint reads at internal +0x48 to
// the scene (0x14509-0x14521), as nxFixedJointAttachScene does for the fixed
// family. Not an oracle row. Defined in core/NpD6Joint.cpp;
// internal->mPublicObject must be non-null.
class NxJoint;
NxJoint* nxD6JointAttachScene(D6Joint* internal, void* writeLink, void* readLink);

static_assert(offsetof(D6Joint, mMotion) == 0x16c, "the motions are at +0x16c (0x9c8e7)");
static_assert(offsetof(D6Joint, mLinearLimit) == 0x184, "the linear limit is at +0x184 (0x9e1bc)");
static_assert(offsetof(D6Joint, mSwing1Limit) == 0x190, "the swing1 limit is at +0x190 (0x9e1d9)");
static_assert(offsetof(D6Joint, mSwing2Limit) == 0x19c, "the swing2 limit is at +0x19c (0x9e1eb)");
static_assert(offsetof(D6Joint, mTwistLimit) == 0x1a8, "the twist limit is at +0x1a8 (0x9e1fd)");
static_assert(offsetof(D6Joint, mDrive) == 0x1c0, "the drives are at +0x1c0 (0x9e21b)");
static_assert(offsetof(D6Joint, mUseSpherical) == 0x220, "useSpherical is at +0x220");
static_assert(offsetof(D6Joint, mDrivePosition) == 0x224, "the drive position is at +0x224");
static_assert(offsetof(D6Joint, mDriveOrientation) == 0x230, "the drive orientation is at +0x230");
static_assert(offsetof(D6Joint, mDriveLinearVelocity) == 0x240, "the drive linear velocity is at +0x240");
static_assert(offsetof(D6Joint, mDriveAngularVelocity) == 0x24c, "the drive angular velocity is at +0x24c");
static_assert(offsetof(D6Joint, mProjectionDistance) == 0x258, "the projection distance is at +0x258");
static_assert(offsetof(D6Joint, mProjectionAngle) == 0x25c, "the projection angle is at +0x25c");
static_assert(offsetof(D6Joint, mSwing1CosHalf) == 0x260, "the swing1 cosine is at +0x260");
static_assert(offsetof(D6Joint, mSwing2CosHalf) == 0x264, "the swing2 cosine is at +0x264");
static_assert(offsetof(D6Joint, mTwistCosHalf) == 0x268, "the twist cosine is at +0x268");
static_assert(offsetof(D6Joint, mAngularLimited) == 0x26c, "the angular flag is at +0x26c");
static_assert(offsetof(D6Joint, mLinearLimited) == 0x26d, "the linear flag is at +0x26d");
static_assert(sizeof(D6Joint) == 0x270, "D6Joint is 0x270 bytes in the oracle (push 0x270 at 0x14560)");

#endif
