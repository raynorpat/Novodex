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
#include "core/JointSupport.h"

#include <cstddef>

class NxDebugRenderable;

// The dynamic-body record Joint::mBody[i] points to -- `[actorImpl+8]`, the
// 0x260-byte record the candidate builds in nxActorComputeMass
// (Physics/src/Scene.cpp). This is a read view only: nothing constructs it
// through this type. Only the fields the Joint rows touch are named; names
// come from what the candidate stores there, the rest are by offset. See
// revolute-contract.md "## Object layouts" (Body fields the Joint rows read).
// Its +0x204 points to a JointSupportBody (core/JointSupport.h): Task 8a
// found that phys_fn_004360/004362 store that pointer in the constraint
// record's body slots, so the two views are one record.
struct JointBodyRecord
	{
	NxU8				mUnknown000[0x4c];
	//! +0x04c. Raised to 0.4f (0x3ecccccc) by phys_fn_004107/004121/004074
	//! when below it and mUnknown114 bit 8 is clear; the candidate stores
	//! the body descriptor's wakeUpCounter here.
	NxReal				mWakeUpCounter;
	//! +0x050. Position (phys_fn_004125).
	NxVec3				mPosition;
	//! +0x05c. Orientation quaternion x, y, z, w (phys_fn_004125/004129).
	NxReal				mOrientation[4];
	NxU8				mUnknown06c[0x78 - 0x6c];
	//! +0x078. The candidate stores the body descriptor's angularVelocity
	//! here (phys_fn_004354 differences it between the two bodies).
	NxVec3				mAngularVelocity;
	NxU8				mUnknown084[0xc0 - 0x84];
	//! +0x0c0. Inverse mass: the candidate writes 1.0f / mass here
	//! (Physics/src/Scene.cpp:1905). phys_fn_004360 scales the unit
	//! vectors by it when it builds the 3x3 at RevoluteJoint +0x1b8.
	NxReal				mInverseMass;
	NxU8				mUnknown0c4[0xdc - 0xc4];
	//! +0x0dc. Row-major 3x3 (the candidate writes massLocalPose.M here).
	NxReal				mMassLocalRot[9];
	//! +0x100. The candidate writes massLocalPose.t here.
	NxVec3				mMassLocalPos;
	//! +0x10c. Flags word (bit 0x80 tested by phys_fn_004133).
	NxU32				mUnknown10c;
	NxU32				mUnknown110;
	//! +0x114. Flags word; bit 8 (0x100) suppresses the 0.4f wake raise.
	NxU32				mUnknown114;
	NxU8				mUnknown118[0x124 - 0x118];
	//! +0x124. Quaternion x, y, z, w of the +0x134 3x3: the candidate's
	//! nxNpActorUpdateCMassQuaternion (Physics/src/include/NpActorDynamicMath.h)
	//! writes it from +0x134; phys_fn_004356 writes it and has row 000758
	//! rebuild +0x134 from it (000758 reads +0x124..+0x130 as x, y, z, w).
	NxReal				mCMassOrientation[4];
	//! +0x134. Row-major 3x3 + vec3 pose (phys_fn_004080, 004127, 004131
	//! transform through it; 004064 reads it).
	NxReal				mUnknown134[9];
	NxVec3				mUnknown158;
	//! +0x164. World inverse inertia, row-major 3x3: the candidate's
	//! nxNpActorUpdateInertiaMatrices (Physics/src/include/NpActorDynamicMath.h,
	//! called from Scene.cpp and NpActor.cpp) writes it from the +0xc4
	//! inverse-inertia diagonal and the +0x134 rotation. phys_fn_004360
	//! multiplies r x e by it.
	NxReal				mWorldInverseInertia[9];
	NxU8				mUnknown188[0x198 - 0x188];
	//! +0x198. Stamp compared with Joint::mBodyStamp[i].
	NxU32				mStamp;
	//! +0x19c. Pointer whose first word is the NxActor* (phys_fn_004066).
	void*				mOwner;
	NxU8				mUnknown1a0[0x204 - 0x1a0];
	//! +0x204. Pointer to a JointSupportBody (phys_fn_004358 reads it;
	//! phys_fn_004374 writes through it; phys_fn_004360/004362 copy it into
	//! JointSupportRecord::mBody). The body constructor phys_fn_000797 stores
	//! 0 (0x1b713); the only other writer is phys_fn_000611 (0x11305), on the
	//! simulation step, which points it at the body's element of the Scene's
	//! +0x5ac array. The candidate has no step, so it stays 0 (Scene.cpp,
	//! nxActorComputeMass). See joint-open-items-contract.md
	//! "## Body record +0x204".
	JointSupportBody*	mUnknown204;
	};

static_assert(offsetof(JointBodyRecord, mWakeUpCounter) == 0x04c, "wake counter at +0x4c");
static_assert(offsetof(JointBodyRecord, mPosition) == 0x050, "position at +0x50");
static_assert(offsetof(JointBodyRecord, mOrientation) == 0x05c, "quaternion at +0x5c");
static_assert(offsetof(JointBodyRecord, mMassLocalRot) == 0x0dc, "3x3 at +0xdc");
static_assert(offsetof(JointBodyRecord, mMassLocalPos) == 0x100, "vec3 at +0x100");
static_assert(offsetof(JointBodyRecord, mUnknown10c) == 0x10c, "flags at +0x10c");
static_assert(offsetof(JointBodyRecord, mUnknown114) == 0x114, "flags at +0x114");
static_assert(offsetof(JointBodyRecord, mCMassOrientation) == 0x124, "quaternion at +0x124");
static_assert(offsetof(JointBodyRecord, mUnknown134) == 0x134, "pose at +0x134");
static_assert(offsetof(JointBodyRecord, mUnknown158) == 0x158, "pose vec3 at +0x158");
static_assert(offsetof(JointBodyRecord, mStamp) == 0x198, "stamp at +0x198");
static_assert(offsetof(JointBodyRecord, mOwner) == 0x19c, "owner at +0x19c");
static_assert(offsetof(JointBodyRecord, mAngularVelocity) == 0x078, "angular velocity at +0x78");
static_assert(offsetof(JointBodyRecord, mUnknown204) == 0x204, "record pointer at +0x204");
static_assert(offsetof(JointBodyRecord, mInverseMass) == 0x0c0, "inverse mass at +0xc0");
static_assert(offsetof(JointBodyRecord, mWorldInverseInertia) == 0x164, "world inverse inertia at +0x164");

// The actor-side record JointBodyRecord::mOwner (+0x19c) points to: the
// 0x50-byte actor body the candidate builds in Physics/src/Scene.cpp (its
// NxActor* at +0, its dynamic body record at +8, the static global pose at
// +0x20). Read view only. phys_fn_004099 and phys_fn_004101 read the actor's
// global pose through it: the rows of the +8 record's +0x5c quaternion and
// its +0x50 position when +8 is set (the listing's inlined quaternion-to-
// matrix, 0x9637e-0x9643e), else the 3x3 at +0x20 and the vec3 at +0x44.
struct JointActorBody
	{
	NxActor*			mActor;				//!< +0x00 (phys_fn_004066 reads it)
	NxU32				mUnknown004;		//!< +0x04
	JointBodyRecord*	mBody;				//!< +0x08; null for a static actor
	NxU8				mUnknown00c[0x20 - 0x0c];
	NxReal				mPoseRotation[9];	//!< +0x20; row-major
	NxVec3				mPosePosition;		//!< +0x44
	};

static_assert(offsetof(JointActorBody, mBody) == 0x08, "body record at +0x08");
static_assert(offsetof(JointActorBody, mPoseRotation) == 0x20, "static pose at +0x20");
static_assert(offsetof(JointActorBody, mPosePosition) == 0x44, "static position at +0x44");
static_assert(sizeof(JointActorBody) == 0x50, "the actor body is 0x50 bytes");

// A limit-plane list node: 0x14 bytes (phys_fn_004143 allocates `push 0x14`),
// linked through +0x10 from Joint::mLimitPlaneHead. The names are the
// public NxJoint::getNextLimitPlane outputs phys_fn_004131 copies them to.
struct JointLimitPlane
	{
	NxVec3				normal;		//!< +0x00
	NxReal				d;			//!< +0x0c
	JointLimitPlane*	next;		//!< +0x10
	};

static_assert(sizeof(JointLimitPlane) == 0x14, "limit-plane nodes are 0x14 bytes");

class Joint;

// The joint break event: 0x10 bytes, vtable 0x101192cc (inside
// phys_data_002614; one slot, the oracle's row 004113, a Joint.cpp row
// outside the pilot). Allocated through the SDK allocator by
// phys_fn_004111 (0x98019-0x98022) and phys_fn_004374 (0xad160-0xad169),
// which store the vptr, the joint at +8 and a float at +0xc, and handed to
// the Scene's phys_fn_000571, which links it through +4 into the list at
// Scene+0x620. See revolute-contract.md "## Dispatch tables".
class JointBreakEvent
	{
	public:
	JointBreakEvent(Joint* joint, NxReal value) : mJoint(joint), mUnknown00c(value) {}

	//! Slot 0: the oracle's row 004113 (fires the user notify, else frees
	//! the event). Out of the pilot's scope -- no pilot path dispatches it --
	//! so the body only asserts; declared so the object carries a vtable.
	virtual void row004113() { NX_ASSERT(0); }

	JointBreakEvent*	mNext;			//!< +0x04; written by phys_fn_000571
	Joint*				mJoint;			//!< +0x08
	NxReal				mUnknown00c;	//!< +0x0c
	};

static_assert(sizeof(JointBreakEvent) == 0x10, "the break event is 0x10 bytes");
static_assert(offsetof(JointBreakEvent, mJoint) == 0x08, "joint at +0x08");
static_assert(offsetof(JointBreakEvent, mUnknown00c) == 0x0c, "float at +0x0c");

class Joint
	{
	public:
	//! phys_fn_004141 (0x00099e60, 464 B). Installs vptr 0x101192d0, stores
	//! typeBit at +0x04, zeroes +0x48/+0x2c/+0x10/+0x34/+0x38/+0x154..+0x15c/
	//! +0x14..+0x1c/+0x20/+0x44, then calls phys_fn_004107 and
	//! phys_fn_004121. See "## Construction chain" step 6.
	Joint(const NxJointDesc& desc, NxU32 typeBit);

	//! The deleting destructors free through the SDK allocator
	//! (`[[0x101041bc]]` slot +0x14): phys_fn_004119 (0x98789) for the base
	//! and phys_fn_004368 (0xac61f) for RevoluteJoint. Declared here so the
	//! compiler-generated deleting destructors do the same.
	static void operator delete(void* p) { nxGetSdkAllocator()->free(p); }

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

	//! Slot 2 (+0x08). phys_fn_004111 (0x00097fd0, 113 B; write): the
	//! break test the solver calls. `ret 8`: the first argument is a
	//! constraint record (the row tests its +0x0c flags word), the second a
	//! float the break event carries at +0xc. Every joint family inherits
	//! this slot unchanged.
	virtual void row004111(const JointSupportRecord* record, NxReal value);

	//! Slot 3 (+0x0c). phys_fn_004087 (0x00095cc0, 87 B; write:
	//! mAccumulated += (numerator / divisor) * v; `ret 0xc`, the middle
	//! argument is a pointer). RevoluteJoint inherits this slot unchanged.
	virtual void row004087(NxReal numerator, const NxVec3& v, NxReal divisor);

	//! Slot 4 (+0x10). Pure in the base (_purecall, phys_fn_005667).
	//! RevoluteJoint overrides this slot with phys_fn_004364. The one
	//! stack argument is an NxDebugRenderable: 004364 calls its slots +0x20
	//! (addLine) and +0x30 (addArrow) with those virtuals' argument shapes.
	virtual void row_slot4(NxDebugRenderable& renderable) = 0;

	//! Slot 5 (+0x14). phys_fn_004095 (0x00095e20, 41 B) is this
	//! destructor's body: reinstalls vptr 0x101192d0. The compiler emits
	//! the scalar deleting destructor around it (phys_fn_004119, not
	//! written here). RevoluteJoint overrides this slot with
	//! phys_fn_004368, which calls this body directly rather than through
	//! the vtable (see revolute-contract.md's note on phys_fn_004119).
	virtual ~Joint();

	//! Slot 6 (+0x18). phys_fn_004133 (0x00099ab0, 134 B; write): the
	//! Joint base's own body for this slot, which every family overrides
	//! (RevoluteJoint with phys_fn_004360). The scene row phys_fn_000728
	//! calls this body directly (0x167ea), not through the table: it
	//! refreshes a stale body, sets the state bits to 1, calls slot 7 and,
	//! unless flag bit 2 is set, slot 6 (the override). The argument is a
	//! float: the override divides by it (0xaa2a7).
	virtual void row_slot6(NxReal arg);

	//! Slot 7 (+0x1c). phys_fn_004135 (0x00099b40, 701 B; write): the
	//! limit-plane constraint record. Inherited by the prismatic,
	//! cylindrical, point-on-line, point-in-plane, distance, pulley, fixed
	//! and D6 tables; RevoluteJoint overrides it with phys_fn_004362, which
	//! passes the argument on unchanged to this body (0xab4ad).
	virtual void row_slot7(NxReal arg);

	//! Slot 8 (+0x20). Joint's own default is the folded, unclaimed
	//! phys_fn_004248 (same trivial target as slot 0; not claimed, inline
	//! here). RevoluteJoint overrides this slot with phys_fn_004356, whose
	//! argument is one of the two body records (compared with mBody[0] and
	//! mBody[1], and written through at +0x124..+0x160).
	virtual void row_slot8(void* body) { (void)body; }

	// --- non-virtual Joint members (write / defer rows assigned to
	//     core/Joint.cpp; not part of either vtable) ---

	//! phys_fn_004064 (0x000957a0, 385 B; write: called by the revolute
	//! and spherical projection slots, 004356/004298). `ret 0xc`, three
	//! pointers: out = body[0] pose * anchor0 - body[1] pose * anchor1 (the
	//! +0x134/+0x158 poses; a missing body leaves its point as is).
	void row004064(const NxVec3& anchor0, const NxVec3& anchor1, NxVec3& out) const;

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

	//! phys_fn_004091 (0x00095d60, 62 B; write: called by phys_fn_004111).
	//! `this` in ecx, plain `ret`: sets bit 5 of the flags word of every
	//! record this joint took from phys_fn_004093 (records mUnknown160[0]
	//! .. + mUnknown160[1] of the Scene's array at +0x5b8).
	void row004091();

	//! phys_fn_004093 (0x00095da0, 116 B; write: every family's solver
	//! slots). No stack arguments, plain `ret`; takes the next 0x50-byte
	//! record of the Scene's array at +0x5b8 (count +0x5bc, capacity
	//! +0x5c0; the deferred Scene row phys_fn_000598 grows it when full)
	//! and counts it in mUnknown160 (first index, count).
	JointSupportRecord* row004093();

	//! phys_fn_004095 -- see the ~Joint() destructor above (slot 5).

	//! phys_fn_004097 (0x00095e50, 1176 B; write; on the transcript path).
	//! Per-body frame refresh for body index `i`.
	void refreshBodyFrame(NxU32 bodyIndex);

	//! phys_fn_004099 (0x000962f0, 1112 B; write). Np slot 2
	//! setGlobalAnchor body (every family's slot-2 row, e.g. 004681).
	void setGlobalAnchor(const NxVec3& anchor);

	//! phys_fn_004101 (0x00096750, 5302 B; write). Np slot 4 setGlobalAxis
	//! body (every family's slot-4 row, e.g. 004683).
	void setGlobalAxis(const NxVec3& axis);

	//! phys_fn_004107 (0x00097d30, 297 B; write; on the transcript path).
	//! Called by phys_fn_004141 and phys_fn_004370. The two pointers are
	//! the actors' internal objects (desc.actor[i] +0x14); the row stores
	//! each one's +8 (the JointBodyRecord) in mBody.
	void row004107(void* actorImpl0, void* actorImpl1, bool suppressAttach);

	//! phys_fn_004109 (0x00097e60, 366 B; write). Np slot 11 setLimitPoint
	//! body (every family's slot-11 row, e.g. 004687).
	void setLimitPoint(const NxVec3& point, bool pointIsOnBody2);

	//! phys_fn_004121 (0x000987a0, 1084 B; write; on the transcript path).
	//! Base part of loadFromDesc: local frame copy, per-body world copy,
	//! maxForce/maxTorque, name binding, jointFlags mapping.
	void loadFromDescBase(const NxJointDesc& desc);

	//! phys_fn_004123 (0x00098be0, 518 B; write: the debug-visualization
	//! slots of the revolute, cylindrical, point-on-line, point-in-plane,
	//! spherical and D6 families). `ret 4`, one output vec3: after the
	//! stale-body refresh, the midpoint of the two world anchors carried
	//! through the bodies' +0x134/+0x158 poses.
	void row004123(NxVec3& out);

	//! phys_fn_004125 (0x00098df0, 1940 B; write; on the transcript path,
	//! folded Np slot 3 getGlobalAnchor).
	void getGlobalAnchor(NxVec3& out) const;

	//! phys_fn_004127 (0x00099590, 235 B; write). Called by
	//! phys_fn_004362/phys_fn_004364. Refreshes a stale body, then writes
	//! mWorldAxis[0] rotated by body[0]'s +0x134 3x3 (or copied, no body).
	void row004127(NxVec3& out) const;

	//! phys_fn_004318 (0x000a7240, 1115 B; joint-families Task 3b). The
	//! debug-visualization body the cylindrical (0x1011a048) and prismatic
	//! (0x1011a4d0) internal tables both name at slot 4. It reads only Joint
	//! base fields, so it is written once here and each family's row_slot4
	//! calls it. Defined in core/CylindricalJoint.cpp (its owning unit).
	void row004318(NxDebugRenderable& renderable);

	//! phys_fn_004129 (0x00099680, 787 B; write; on the transcript path via
	//! folded Np slot 5 getGlobalAxis; also called by phys_fn_004354).
	void getGlobalAxis(NxVec3& out) const;

	//! phys_fn_004131 (0x000999a0, 260 B; write: called by
	//! phys_fn_004145). `ret 0x10`. Writes the plane in world space
	//! (rotated/offset by mSolverBody[1]'s +0x134 pose, or copied) and
	//! returns dot(point, planeNormal) + planeD. The listing returns the
	//! unrounded x87 value in st(0), which the caller compares with 0, so
	//! it is declared NxF64.
	NxF64 row004131(const JointLimitPlane* plane, const NxVec3& point, NxVec3& planeNormal, NxReal& planeD);

	//! phys_fn_004133 default body -- see the slot 6 virtual above.

	//! phys_fn_004135 default body -- see the slot 7 virtual above.

	//! phys_fn_004137 (0x00099e00, 41 B; write). Folded Np slot 6
	//! getGlobalAnchorVal body.
	NxVec3 getGlobalAnchorVal() const;

	//! phys_fn_004139 (0x00099e30, 41 B; write). Folded Np slot 7
	//! getGlobalAxisVal body.
	NxVec3 getGlobalAxisVal() const;

	//! phys_fn_004141 -- see the constructor above.

	//! phys_fn_004143 (0x0009a0d0, 860 B; write). Np slot 13 addLimitPlane
	//! body (every family's slot-13 row, e.g. 004689).
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

	//! +0x014..+0x01c. Limit point, in mSolverBody[0]'s frame:
	//! phys_fn_004080 (getLimitPoint) transforms it by that body's +0x134
	//! pose into worldLimitPoint. Zeroed by phys_fn_004141 (0x99ea2-0x99ea8).
	NxVec3				mLimitPoint;

	//! +0x020. Limit-plane list head (nodes linked through node+0x10).
	JointLimitPlane*	mLimitPlaneHead;

	//! +0x024 / +0x028. Body pair in solver order. phys_fn_004107
	//! 0x97de5-0x97dfa: +0x2c bit 1 set -> (body[0], body[1]); clear ->
	//! (body[1], body[0]).
	void*				mSolverBody[2];

	//! +0x02c. Flags: bit0 in scene; bit1 solver order (clear: mSolverBody =
	//! body[1], body[0]; set: body[0], body[1]); bit2 unknown
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

	// World copy of the +0x4c..+0xcb local block, +0x0cc..+0x14b, same
	// field order (each written by phys_fn_004121 when body[i] is null, or
	// phys_fn_004097(i) otherwise; phys_fn_004372 reads normal/cross).

	//! +0x0cc / +0x0d8. World normal[2].
	NxVec3				mWorldNormal[2];

	//! +0x0e4 / +0x0f0. World cross[2] (mirrors mLocalCross).
	NxVec3				mWorldCross[2];

	//! +0x0fc / +0x108. World axis[2].
	NxVec3				mWorldAxis[2];

	//! +0x114 / +0x120. World anchor[2].
	NxVec3				mWorldAnchor[2];

	//! +0x12c / +0x13c. World frame quaternion[2] (same raw storage
	//! convention as mFrameQuat).
	NxReal				mWorldQuat[2][4];

	//! +0x14c / +0x150. body[i] stamp cache, compared with body+0x198;
	//! -1 forces a refresh.
	NxU32				mBodyStamp[2];

	//! +0x154. Accumulated vec3 (`+= (a/b)*v` by internal slot 3).
	NxVec3				mAccumulated;

	//! +0x160 / +0x164. First index and count of the constraint records
	//! this joint took from phys_fn_004093 (004093 sets them, 004091 walks
	//! them, the scene row phys_fn_000728 resets them to -1 / 0 before it
	//! calls phys_fn_004133). Meaning beyond that unknown.
	NxU32				mUnknown160[2];

	//! +0x168. NxJointType.
	NxJointType			mType;
	};

static_assert(sizeof(Joint) == 0x16c, "Joint is 0x16c bytes in the oracle");
static_assert(offsetof(Joint, mTypeBit) == 0x004, "type bit follows the vptr");
static_assert(offsetof(Joint, mBody) == 0x008, "the body pointers are at +0x08");
static_assert(offsetof(Joint, mNextJoint) == 0x010, "the scene list link is at +0x10");
static_assert(offsetof(Joint, mLimitPoint) == 0x014, "the limit point is at +0x14");
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
static_assert(offsetof(Joint, mWorldNormal) == 0x0cc, "the world normal pair is at +0xcc");
static_assert(offsetof(Joint, mWorldCross) == 0x0e4, "the world cross pair is at +0xe4");
static_assert(offsetof(Joint, mWorldAxis) == 0x0fc, "the world axis pair is at +0xfc");
static_assert(offsetof(Joint, mWorldAnchor) == 0x114, "the world anchor pair is at +0x114");
static_assert(offsetof(Joint, mWorldQuat) == 0x12c, "the world quaternion pair is at +0x12c");
static_assert(offsetof(Joint, mBodyStamp) == 0x14c, "the body stamp cache is at +0x14c");
static_assert(offsetof(Joint, mAccumulated) == 0x154, "the accumulated vec3 is at +0x154");
static_assert(offsetof(Joint, mUnknown160) == 0x160, "the unknown pair is at +0x160");
static_assert(offsetof(Joint, mType) == 0x168, "the joint type is at +0x168");

#endif
