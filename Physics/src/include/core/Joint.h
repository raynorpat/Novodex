#ifndef NX_PHYSICS_CORE_JOINT
#define NX_PHYSICS_CORE_JOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal joint base object, recovered by the revolute pilot (Task 4/5).
// Not related to the public NxJoint hierarchy: this is a wholly separate,
// internal-only polymorphic base with its own 9-slot vtable
// (0x101192d0, phys_data_002614), installed by phys_fn_004141 (the
// constructor) and phys_fn_004095/phys_fn_004119 (destructor bodies). See
// docs/reconstruction/novodex-physics/units/revolute-contract.md
// "## Object layouts" (the Joint table) and "## Dispatch tables"
// (0x101192d0 -- Joint base).
//
// Size: 0x16c bytes (the first RevoluteJoint field sits at +0x16c; see
// phys_fn_004366 0xac559 and phys_fn_004141 which writes up to +0x168).
// Every established field below is commented with its offset and the row
// that establishes it; fields marked "unknown" are declared by offset only
// (mUnknownNNN), per the contract's naming convention.
//
// Joint's own virtuals are declared here in the internal vtable's slot
// order (0-8); RevoluteJoint (RevoluteJoint.h) both overrides several of
// them and adds its own slots 9-16.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxJointDesc.h"
#include "NxVec3.h"

#include <cstddef>

class Joint
	{
	public:
	//! phys_fn_004141 (0x00099e60, 464 B). Installs vptr 0x101192d0, stores
	//! typeBit at +0x04, zeroes +0x48/+0x2c/+0x10/+0x34/+0x38/+0x154..+0x15c/
	//! +0x14..+0x1c/+0x20/+0x44, then calls phys_fn_004107 and
	//! phys_fn_004121. See "## Construction chain" step 6.
	Joint(const NxJointDesc& desc, NxU32 typeBit);

	// --- internal vtable, slot order 0-8 (0x101192d0 / phys_data_002614) ---

	//! Slot 0 (+0x00). Joint's own default is the folded, unclaimed
	//! phys_fn_004248 (FixedJoint.cpp; "ret 4", empty body -- not claimed;
	//! written inline here per the contract's `reuse` table).
	//! RevoluteJoint overrides this slot with phys_fn_004374.
	virtual void row_slot0(NxU32 arg) { (void)arg; }

	//! Slot 1 (+0x04). Joint's own default is the folded, unclaimed
	//! phys_fn_001583 ("ret", empty body -- not claimed; inline here).
	//! RevoluteJoint overrides this slot with phys_fn_004328.
	virtual void row_slot1() {}

	//! Slot 2 (+0x08). phys_fn_004111 (0x00097fd0, 113 B; deferred: break
	//! test from the solver, needs phys_fn_000571/004091 and the break
	//! event). RevoluteJoint inherits this slot unchanged.
	virtual void row004111(NxU32 a, NxU32 b);

	//! Slot 3 (+0x0c). phys_fn_004087 (0x00095cc0, 87 B; write: accumulates
	//! into +0x154). RevoluteJoint inherits this slot unchanged.
	virtual void row004087(NxU32 a, NxU32 b, NxU32 c);

	//! Slot 4 (+0x10). Pure in the base (_purecall, phys_fn_005667).
	//! RevoluteJoint overrides this slot with phys_fn_004364.
	virtual void row_slot4(NxU32 arg) = 0;

	//! Slot 5 (+0x14). phys_fn_004095 (0x00095e20, 41 B) is this
	//! destructor's body: reinstalls vptr 0x101192d0. The compiler emits
	//! the scalar deleting destructor around it (phys_fn_004119, not
	//! written here). RevoluteJoint overrides this slot with
	//! phys_fn_004368, which calls this body directly rather than through
	//! the vtable (see revolute-contract.md's note on phys_fn_004119).
	virtual ~Joint();

	//! Slot 6 (+0x18). phys_fn_004133 (0x00099ab0, 134 B; deferred: the
	//! Joint base's own default for this slot). RevoluteJoint overrides
	//! this slot with phys_fn_004360.
	virtual void row_slot6(NxU32 arg);

	//! Slot 7 (+0x1c). phys_fn_004135 (0x00099b40, 701 B; deferred: the
	//! Joint base's own default for this slot). RevoluteJoint overrides
	//! this slot with phys_fn_004362.
	virtual void row_slot7(NxU32 arg);

	//! Slot 8 (+0x20). Joint's own default is the folded, unclaimed
	//! phys_fn_004248 (same trivial target as slot 0; not claimed, inline
	//! here). RevoluteJoint overrides this slot with phys_fn_004356.
	virtual void row_slot8(NxU32 arg) { (void)arg; }

	// --- non-virtual Joint members (write / defer rows assigned to
	//     core/Joint.cpp; not part of either vtable) ---

	//! phys_fn_004064 (0x000957a0, 385 B; deferred: internal slot 8 caller
	//! only, reached from scene code outside the pilot). Transforms two
	//! points through body[0]/body[1].
	void row004064(NxVec3& out1, NxVec3& out2, const NxVec3& in1, const NxVec3& in2);

	//! phys_fn_004066 (0x00095930, 266 B; write). Base part of
	//! saveToDesc, called by every joint's saveToDesc row (incl.
	//! phys_fn_004330). Writes NxJointDesc fields from Joint +0x3c..+0xa8 /
	//! +0x2c / +0x48.
	void saveToDescBase(NxJointDesc& desc) const;

	//! phys_fn_004070 (0x00095a80, 7 B; write). Returns +0x168, the
	//! NxJointType phys_fn_004141 stores.
	NxJointType getType() const;

	//! phys_fn_004074 (0x00095ab0, 216 B; write). Np slot 9 setBreakable
	//! (phys_fn_004685) body.
	void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! phys_fn_004076 (0x00095b90, 21 B; write). Folded Np slot 10
	//! getBreakable body.
	void getBreakable(NxReal& maxForce, NxReal& maxTorque) const;

	//! phys_fn_004078 (0x00095bb0, 10 B; write; on the transcript path via
	//! folded Np getState). Reads the (>>3)&3 state bits at +0x2c.
	NxJointState getState() const;

	//! phys_fn_004080 (0x00095bc0, 208 B; write). Folded Np slot 12
	//! getLimitPoint body.
	bool getLimitPoint(NxVec3& worldLimitPoint) const;

	//! phys_fn_004081 (0x00095c90, 9 B; write). Np slot 15
	//! resetLimitPlaneIterator (phys_fn_004691) body.
	void resetLimitPlaneIterator();

	//! phys_fn_004083 (0x00095ca0, 14 B; write). Folded Np slot 16
	//! hasMoreLimitPlanes body.
	bool hasMoreLimitPlanes() const;

	//! phys_fn_004089 (0x00095d20, 58 B; write). Np slot 14
	//! purgeLimitPlanes (phys_fn_004695); also the tail of phys_fn_004095.
	void purgeLimitPlanes();

	//! phys_fn_004093 (0x00095da0, 116 B; deferred: solver slots 6/7,
	//! also needs Scene row phys_fn_000598 absent from the candidate).
	//! Called by phys_fn_004360/phys_fn_004362.
	void row004093(NxU32 arg);

	//! phys_fn_004095 -- see the ~Joint() destructor above (slot 5).

	//! phys_fn_004097 (0x00095e50, 1176 B; write; on the transcript path).
	//! Per-body frame refresh for body index `i`.
	void refreshBodyFrame(NxU32 bodyIndex);

	//! phys_fn_004099 (0x000962f0, 1112 B; deferred: NxJoint::setGlobalAnchor,
	//! not reached by the joint test). Np slot 2 setGlobalAnchor
	//! (phys_fn_004681) body.
	void setGlobalAnchor(const NxVec3& anchor);

	//! phys_fn_004101 (0x00096750, 5302 B; deferred: NxJoint::setGlobalAxis,
	//! not reached by the joint test). Np slot 4 setGlobalAxis
	//! (phys_fn_004683) body.
	void setGlobalAxis(const NxVec3& axis);

	//! phys_fn_004107 (0x00097d30, 297 B; write; on the transcript path).
	//! Called by phys_fn_004141 and phys_fn_004370.
	void row004107(void* body0, void* body1, bool suppressAttach);

	//! phys_fn_004109 (0x00097e60, 366 B; deferred: NxJoint::setLimitPoint,
	//! not reached by the joint test). Np slot 11 setLimitPoint
	//! (phys_fn_004687) body.
	void setLimitPoint(const NxVec3& point, bool pointIsOnBody2);

	//! phys_fn_004121 (0x000987a0, 1084 B; write; on the transcript path).
	//! Base part of loadFromDesc: local frame copy, per-body world copy,
	//! maxForce/maxTorque, name binding, jointFlags mapping.
	void loadFromDescBase(const NxJointDesc& desc);

	//! phys_fn_004123 (0x00098be0, 518 B; deferred: internal slot 4,
	//! solver). Called by phys_fn_004364.
	void row004123(NxU32 arg);

	//! phys_fn_004125 (0x00098df0, 1940 B; write; on the transcript path,
	//! folded Np slot 3 getGlobalAnchor).
	void getGlobalAnchor(NxVec3& out) const;

	//! phys_fn_004127 (0x00099590, 235 B; write). Called by
	//! phys_fn_004362/phys_fn_004364.
	void row004127(NxU32 arg);

	//! phys_fn_004129 (0x00099680, 787 B; write; on the transcript path via
	//! folded Np slot 5 getGlobalAxis; also called by phys_fn_004354).
	void getGlobalAxis(NxVec3& out) const;

	//! phys_fn_004131 (0x000999a0, 260 B; deferred: called only by
	//! phys_fn_004145).
	void row004131(NxU32 arg);

	//! phys_fn_004133 default body -- see the slot 6 virtual above.

	//! phys_fn_004135 default body -- see the slot 7 virtual above.

	//! phys_fn_004137 (0x00099e00, 41 B; write). Folded Np slot 6
	//! getGlobalAnchorVal body.
	NxVec3 getGlobalAnchorVal() const;

	//! phys_fn_004139 (0x00099e30, 41 B; write). Folded Np slot 7
	//! getGlobalAxisVal body.
	NxVec3 getGlobalAxisVal() const;

	//! phys_fn_004141 -- see the constructor above.

	//! phys_fn_004143 (0x0009a0d0, 860 B; deferred: NxJoint::addLimitPlane,
	//! not reached by the joint test). Np slot 13 addLimitPlane
	//! (phys_fn_004689) body.
	bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004145 (0x0009a430, 174 B; write). Folded Np slot 17
	//! getNextLimitPlane body.
	bool getNextLimitPlane(NxVec3& planeNormal, NxReal& planeD);

	// --- fields, in the oracle's byte-offset order (Joint is 0x16c bytes,
	//     including the compiler-inserted vptr at +0x000) ---

	//! +0x004. One bit per joint type (0x40 = revolute); phys_fn_004141's
	//! second argument. Readers outside the constructor: unknown.
	NxU32				mTypeBit;

	//! +0x008 / +0x00c. `[actorImpl+8]` for desc.actor[i]'s +0x14; 0 =
	//! world. phys_fn_004107 0x97dc7/0x97dea.
	void*				mBody[2];

	//! +0x010. Next joint in the Scene's list (head at Scene+0x59c).
	void*				mNextJoint;

	//! +0x014..+0x01c. Unknown; zeroed by phys_fn_004141 (0x99ea2-0x99ea8).
	NxU32				mUnknown014[3];

	//! +0x020. Limit-plane list head (nodes linked through node+0x10).
	void*				mLimitPlaneHead;

	//! +0x024 / +0x028. Body pair in solver order (swapped when +0x2c
	//! bit 1 is set). phys_fn_004107 0x97def-0x97dfa.
	void*				mSolverBody[2];

	//! +0x02c. Flags: bit0 in scene; bit1 swap order; bit2 unknown
	//! (phys_fn_004133 0x99b22); bits3-4 = NxJointState (0x10 = broken);
	//! bit8/bit9 = NX_JF_COLLISION_ENABLED / NX_JF_VISUALIZATION.
	NxU32				mFlags;

	//! +0x030. Owning Scene*.
	void*				mScene;

	//! +0x034 / +0x038. Unknown; zeroed by phys_fn_004141 (0x99e8a, 0x99e8d).
	NxU32				mUnknown034[2];

	//! +0x03c. maxForce.
	NxReal				mMaxForce;

	//! +0x040. maxTorque.
	NxReal				mMaxTorque;

	//! +0x044. projectionMode (NxJointProjectionMode).
	NxJointProjectionMode	mProjectionMode;

	//! +0x048. The public object (NpRevoluteJoint* for this class).
	void*				mPublicObject;

	//! +0x04c / +0x058. localNormal[2].
	NxVec3				mLocalNormal[2];

	//! +0x064 / +0x070. localNormal[i] x localAxis[i] (name unknown).
	NxVec3				mLocalCross[2];

	//! +0x07c / +0x088. localAxis[2].
	NxVec3				mLocalAxis[2];

	//! +0x094 / +0x0a0. localAnchor[2].
	NxVec3				mLocalAnchor[2];

	//! +0x0ac / +0x0bc. Frame quaternion[2] (x, y, z stored negated, w
	//! last -- raw storage, not NxQuat, because the storage order here is
	//! established by the listing, not left to NxQuat's own convention).
	NxReal				mFrameQuat[2][4];

	//! +0x0cc..+0x14b. World copy of the +0x4c..+0xcb block, same order:
	//! normal[2] (+0xcc), cross[2] (+0xe4), axis[2] (+0xfc),
	//! anchor[2] (+0x114), quat[2] (+0x12c).
	unsigned char		mWorldCopy[0x80];

	//! +0x14c / +0x150. body[i] stamp cache, compared with body+0x198;
	//! -1 forces a refresh.
	NxU32				mBodyStamp[2];

	//! +0x154. Accumulated vec3 (`+= (a/b)*v` by internal slot 3).
	NxVec3				mAccumulated;

	//! +0x160 / +0x164. Unknown; no row read in this task touches them.
	NxU32				mUnknown160[2];

	//! +0x168. NxJointType.
	NxJointType			mType;
	};

static_assert(sizeof(Joint) == 0x16c, "Joint is 0x16c bytes in the oracle");
static_assert(offsetof(Joint, mTypeBit) == 0x004, "type bit follows the vptr");
static_assert(offsetof(Joint, mBody) == 0x008, "the body pointers are at +0x08");
static_assert(offsetof(Joint, mNextJoint) == 0x010, "the scene list link is at +0x10");
static_assert(offsetof(Joint, mUnknown014) == 0x014, "the unknown triple is at +0x14");
static_assert(offsetof(Joint, mLimitPlaneHead) == 0x020, "the limit-plane head is at +0x20");
static_assert(offsetof(Joint, mSolverBody) == 0x024, "the solver body pair is at +0x24");
static_assert(offsetof(Joint, mFlags) == 0x02c, "the flags word is at +0x2c");
static_assert(offsetof(Joint, mScene) == 0x030, "the owning scene is at +0x30");
static_assert(offsetof(Joint, mUnknown034) == 0x034, "the unknown pair is at +0x34");
static_assert(offsetof(Joint, mMaxForce) == 0x03c, "maxForce is at +0x3c");
static_assert(offsetof(Joint, mMaxTorque) == 0x040, "maxTorque is at +0x40");
static_assert(offsetof(Joint, mProjectionMode) == 0x044, "projectionMode is at +0x44");
static_assert(offsetof(Joint, mPublicObject) == 0x048, "the public object pointer is at +0x48");
static_assert(offsetof(Joint, mLocalNormal) == 0x04c, "localNormal[2] is at +0x4c");
static_assert(offsetof(Joint, mLocalCross) == 0x064, "the cross products are at +0x64");
static_assert(offsetof(Joint, mLocalAxis) == 0x07c, "localAxis[2] is at +0x7c");
static_assert(offsetof(Joint, mLocalAnchor) == 0x094, "localAnchor[2] is at +0x94");
static_assert(offsetof(Joint, mFrameQuat) == 0x0ac, "the frame quaternions are at +0xac");
static_assert(offsetof(Joint, mWorldCopy) == 0x0cc, "the world copy block is at +0xcc");
static_assert(offsetof(Joint, mBodyStamp) == 0x14c, "the body stamp cache is at +0x14c");
static_assert(offsetof(Joint, mAccumulated) == 0x154, "the accumulated vec3 is at +0x154");
static_assert(offsetof(Joint, mUnknown160) == 0x160, "the unknown pair is at +0x160");
static_assert(offsetof(Joint, mType) == 0x168, "the joint type is at +0x168");

#endif
