#ifndef NX_PHYSICS_CORE_REVOLUTEJOINT
#define NX_PHYSICS_CORE_REVOLUTEJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal RevoluteJoint object the revolute pilot recovers. See
// docs/reconstruction/novodex-physics/units/revolute-contract.md
// "## Object layouts" (RevoluteJoint, 0x204 bytes) and "## Dispatch tables"
// (0x1011a1c0 -- RevoluteJoint internal).
//
// RevoluteJoint derives from Joint (Joint.h) the way the oracle's object
// does: the base part (0x00-0x16b) IS a Joint (phys_fn_004141 is called
// first, at 0xac54c). RevoluteJoint overrides several of Joint's 9 vtable
// slots and adds its own slots 9-16 (0x1011a1c0 has 17 slots total).

#include "Nxp.h"
#include "core/Joint.h"
#include "NxRevoluteJointDesc.h"

#include <cstddef>

class RevoluteJoint : public Joint
	{
	public:
	//! phys_fn_004366 (0x000ac540, 162 B). Installs vptr 0x1011a1c0, calls
	//! Joint(desc, 0x40), initialises +0x16c..+0x198, allocates and
	//! constructs the 0x1c-byte NpRevoluteJoint, and calls phys_fn_004332.
	//! See "## Construction chain".
	RevoluteJoint(const NxRevoluteJointDesc& desc);

	//! phys_fn_004368 (0x000ac5f0, 56 B). Internal slot 5: the scalar
	//! deleting destructor override. Deletes [this+0x48] through its
	//! slot 0 with flag 1, then calls the base destructor body
	//! (phys_fn_004095) directly -- not through the vtable, per
	//! revolute-contract.md's note on phys_fn_004119.
	virtual ~RevoluteJoint();

	// --- Joint slots RevoluteJoint overrides ---

	//! Slot 0 (+0x00). phys_fn_004374 (0x000ad0b0, 1068 B). Reads/writes
	//! the revolute-only +0x1ac..+0x200 fields.
	virtual void row_slot0(NxU32 arg);

	//! Slot 1 (+0x04). phys_fn_004328 (0x000a8d20, 21 B). Zeroes
	//! +0x1ac..+0x1b4.
	virtual void row_slot1();

	//! Slot 4 (+0x10). phys_fn_004364 (0x000ab840, 3326 B).
	virtual void row_slot4(NxU32 arg);

	//! Slot 6 (+0x18). phys_fn_004360 (0x000aa060, 4460 B).
	virtual void row_slot6(NxU32 arg);

	//! Slot 7 (+0x1c). phys_fn_004362 (0x000ab1d0, 1644 B).
	virtual void row_slot7(NxU32 arg);

	//! Slot 8 (+0x20). phys_fn_004356 (0x000a9650, 2303 B).
	virtual void row_slot8(NxU32 arg);

	// --- RevoluteJoint's own slots, 9-16 (0x1011a1c0) ---

	//! Slot 9 (+0x24). phys_fn_004370 (0x000ac630, 202 B). loadFromDesc
	//! (strings "RevoluteJoint::loadFromDesc" x2).
	virtual void loadFromDesc(const NxRevoluteJointDesc& desc);

	//! Slot 10 (+0x28). phys_fn_004330 (0x000a8d40, 281 B). saveToDesc
	//! (string "RevoluteJoint::saveToDesc").
	virtual void saveToDesc(NxRevoluteJointDesc& desc);

	//! Slot 11 (+0x2c). phys_fn_004334 (0x000a8f10, 147 B). setFlags
	//! (string "RevoluteJoint::setFlags").
	virtual void setFlags(NxU32 flags);

	//! Slot 12 (+0x30). phys_fn_004336 (0x000a8fb0, 7 B). Returns +0x1a8;
	//! reached from Np getFlags (phys_fn_004703) via [vt+0x30].
	virtual NxU32 getFlags() const;

	//! Slot 13 (+0x34). phys_fn_004338 (0x000a8fc0, 62 B). setProjectionMode
	//! (string "RevoluteJoint::setProjectionMode").
	virtual void setProjectionMode(NxJointProjectionMode mode);

	//! Slot 14 (+0x38). phys_fn_004186 (D6Joint.cpp, folded; not claimed).
	//! Trivial forward to the Joint base's own projectionMode field;
	//! called by Np getProjectionMode (phys_fn_004707).
	virtual NxJointProjectionMode getProjectionMode() const { return mProjectionMode; }

	//! Slot 15 (+0x3c). phys_fn_001391 (folded; not claimed). Trivial
	//! `mov eax,ecx; ret` body.
	virtual RevoluteJoint* row_slot15() { return this; }

	//! Slot 16 (+0x40). phys_fn_001391 (folded; not claimed; same trivial
	//! target reused for this slot).
	virtual RevoluteJoint* row_slot16() { return this; }

	// --- plain (non-virtual) RevoluteJoint members; called directly by
	//     NpRevoluteJoint, not through the internal vtable ---

	//! phys_fn_004332 (0x000a8e60, 171 B). Revolute part of loadFromDesc:
	//! desc+0x6c..+0x9c -> +0x16c..+0x19c, cos/sin(projectionAngle) ->
	//! +0x1a0/+0x1a4, desc.flags -> +0x1a8, desc.projectionMode -> +0x44.
	//! Called by phys_fn_004366 (0xac5d5) and phys_fn_004370 (0xac6f0).
	void row004332(const NxRevoluteJointDesc& desc);

	//! phys_fn_004340 (0x000a9000, 189 B). String "RevoluteJoint::setLimits";
	//! called by phys_fn_004709.
	void setLimits(const NxJointLimitPairDesc& limits);

	//! phys_fn_004342 (0x000a90c0, 58 B). Called by phys_fn_004711; reads
	//! +0x16c..+0x180, +0x1a8.
	bool getLimits(NxJointLimitPairDesc& limits) const;

	//! phys_fn_004344 (0x000a9100, 171 B). String "RevoluteJoint::setMotor";
	//! tail-jumped from phys_fn_004713.
	void setMotor(const NxMotorDesc& motor);

	//! phys_fn_004346 (0x000a91b0, 42 B). Called by phys_fn_004715; reads
	//! +0x184..+0x18c.
	bool getMotor(NxMotorDesc& motor) const;

	//! phys_fn_004348 (0x000a91e0, 171 B). String "RevoluteJoint::setSpring";
	//! called by phys_fn_004717.
	void setSpring(const NxSpringDesc& spring);

	//! phys_fn_004350 (0x000a9290, 43 B). Called by phys_fn_004719; reads
	//! +0x190..+0x198.
	bool getSpring(NxSpringDesc& spring) const;

	//! phys_fn_004352 (0x000a92c0, 694 B). In the evidenced span; called
	//! by phys_fn_004362/phys_fn_004364.
	void row004352();

	//! phys_fn_004354 (0x000a9580, 197 B). Called by phys_fn_004723 (Np
	//! getVelocity).
	NxReal getVelocity() const;

	//! phys_fn_004358 (0x000a9f50, 269 B). In the evidenced span; called
	//! by phys_fn_004374; reads +0x1dc..+0x1f0.
	void row004358();

	//! phys_fn_004372 (0x000ac700, 2467 B). Called only by phys_fn_004721
	//! (Np getAngle); reads Joint +0x8/+0xcc/+0xe4 frames.
	NxReal getAngle() const;

	// --- fields, in the oracle's byte-offset order (RevoluteJoint's base
	//     part, 0x00-0x16b, IS Joint; the fields below start at +0x16c) ---

	struct LimitLeg
		{
		NxReal value;
		NxReal restitution;
		NxReal hardness;
		};

	struct Motor
		{
		NxReal velTarget;
		NxReal maxForce;
		NxReal freeSpin;
		};

	struct Spring
		{
		NxReal spring;
		NxReal damper;
		NxReal targetValue;
		};

	//! +0x16c. limit.low / limit.high (NxJointLimitPairDesc copy).
	struct
		{
		LimitLeg low;
		LimitLeg high;
		}					mLimit;

	//! +0x184. motor (NxMotorDesc copy).
	Motor				mMotor;

	//! +0x190. spring (NxSpringDesc copy).
	Spring				mSpring;

	//! +0x19c. projectionDistance.
	NxReal				mProjectionDistance;

	//! +0x1a0. cos(projectionAngle).
	NxReal				mProjectionAngleCos;

	//! +0x1a4. sin(projectionAngle).
	NxReal				mProjectionAngleSin;

	//! +0x1a8. flags (NX_RJF_*: bit0 limit, bit1 motor, bit2 spring).
	NxU32				mRevoluteFlags;

	//! +0x1ac. Unknown vec3; zeroed by phys_fn_004328 (slot 1); read by
	//! phys_fn_004360, phys_fn_004374.
	NxVec3				mUnknown1ac;

	//! +0x1b8. Unknown 9 floats (3x3, multiplied as a matrix by
	//! phys_fn_004374 0xad246-0xad29c); read/written by phys_fn_004360,
	//! phys_fn_004374.
	NxReal				mUnknown1b8[9];

	//! +0x1dc. Unknown vec3; phys_fn_004358, phys_fn_004360,
	//! phys_fn_004374.
	NxVec3				mUnknown1dc;

	//! +0x1e8. Unknown vec3; phys_fn_004358, phys_fn_004360,
	//! phys_fn_004374.
	NxVec3				mUnknown1e8;

	//! +0x1f4. Unknown vec3 (written by phys_fn_004374 0xad1d3-0xad1fb);
	//! phys_fn_004360, phys_fn_004374.
	NxVec3				mUnknown1f4;

	//! +0x200. Unknown float (compared with a squared length and
	//! fsqrt'ed by phys_fn_004374 0xad125-0xad17f).
	NxReal				mUnknown200;
	};

static_assert(sizeof(RevoluteJoint) == 0x204, "RevoluteJoint is 0x204 bytes in the oracle");
static_assert(offsetof(RevoluteJoint, mLimit) == 0x16c, "the limit pair is at +0x16c");
static_assert(offsetof(RevoluteJoint, mMotor) == 0x184, "the motor is at +0x184");
static_assert(offsetof(RevoluteJoint, mSpring) == 0x190, "the spring is at +0x190");
static_assert(offsetof(RevoluteJoint, mProjectionDistance) == 0x19c, "projectionDistance is at +0x19c");
static_assert(offsetof(RevoluteJoint, mProjectionAngleCos) == 0x1a0, "cos(angle) is at +0x1a0");
static_assert(offsetof(RevoluteJoint, mProjectionAngleSin) == 0x1a4, "sin(angle) is at +0x1a4");
static_assert(offsetof(RevoluteJoint, mRevoluteFlags) == 0x1a8, "the revolute flags are at +0x1a8");
static_assert(offsetof(RevoluteJoint, mUnknown1ac) == 0x1ac, "the first unknown vec3 is at +0x1ac");
static_assert(offsetof(RevoluteJoint, mUnknown1b8) == 0x1b8, "the unknown 3x3 is at +0x1b8");
static_assert(offsetof(RevoluteJoint, mUnknown1dc) == 0x1dc, "the second unknown vec3 is at +0x1dc");
static_assert(offsetof(RevoluteJoint, mUnknown1e8) == 0x1e8, "the third unknown vec3 is at +0x1e8");
static_assert(offsetof(RevoluteJoint, mUnknown1f4) == 0x1f4, "the fourth unknown vec3 is at +0x1f4");
static_assert(offsetof(RevoluteJoint, mUnknown200) == 0x200, "the trailing unknown float is at +0x200");

#endif
