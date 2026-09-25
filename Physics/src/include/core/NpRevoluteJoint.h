#ifndef NX_PHYSICS_CORE_NPREVOLUTEJOINT
#define NX_PHYSICS_CORE_NPREVOLUTEJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public revolute joint object the revolute pilot recovers: 0x1c bytes,
// vtable 0x1011b328 (phys_data_002727, 45 slots). See
// docs/reconstruction/novodex-physics/units/revolute-contract.md
// "## Object layouts" (NpRevoluteJoint) and "## Dispatch tables"
// (0x1011b328 -- NpRevoluteJoint primary).
//
// Unlike NpJointObject/NpJointVtable (NpJoint.h), which stand in for every
// OTHER joint type with an offset-addressed byte array plus a detached
// vtable class, NpRevoluteJoint is real C++ multiple inheritance: the
// oracle's shape is the public NxRevoluteJoint (0xc bytes: vptr, userData,
// appData) as the primary base, then a 12-byte secondary base with one
// virtual at +0xc (the "hook base", same shape as ObjectModel.h's
// EmbeddedHookBase -- its class name is unknown, and it is used here as an
// actual base rather than an embedded member, which is what makes
// phys_fn_004727 the compiler-generated adjustor thunk for the second
// base's destructor). Declaring it this way makes the compiler emit
// exactly the two vtables (0x1011b328 primary / 0x1011b3dc secondary) the
// oracle installs, in the oracle's slot order, because neither NxJoint nor
// NxRevoluteJoint overloads any virtual (MSVC keeps declaration order).

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxRevoluteJoint.h"
#include "ObjectModel.h"
#include "core/RevoluteJoint.h"

#include <cstddef>

class NpRevoluteJoint : public NxRevoluteJoint, public EmbeddedHookBase
	{
	public:
	//! phys_fn_004725 (0x000b33a0, 57 B). Zeroes userData/appData (inlined
	//! NxJoint()), installs the transient NxRevoluteJoint vtable
	//! (0x1011b238), constructs the hook base (phys_fn_002404, reused),
	//! installs the secondary vtable (0x1011b3dc), stores `internal` at
	//! +0x18 and +0x08, then installs the final vtable (0x1011b328).
	explicit NpRevoluteJoint(RevoluteJoint* internal);

	//! phys_fn_004729 (0x000b33f0, 55 B). Slot 0: reinstalls
	//! 0x1011b328/0x1011b3dc then the abstract NxJoint table 0x1011a680
	//! before releasing (compiler-generated scalar deleting destructor
	//! wraps this body with the operator-delete-if-flagged step).
	virtual ~NpRevoluteJoint();

	//! phys_fn_004727 (0x000b33e0, 8 B; no decompile, Capstone listing
	//! only). Not declared here: it is the compiler-generated adjustor
	//! thunk ("sub ecx,0xc; jmp <~NpRevoluteJoint>") the second base
	//! (EmbeddedHookBase) needs for the shared virtual destructor, emitted
	//! automatically once ~NpRevoluteJoint() above is defined.

	// --- NxJoint pure virtuals, in NxJoint.h declaration order ---

	//! Slot 1. Unclaimed: shared NpJoint body, folded at 0x000b1600
	//! (core\NpDistanceJoint.cpp). Task 9 implements the forward to
	//! `*[[j+8]+0x19c]` / `*[[j+0xc]+0x19c]` under the read lock.
	virtual void getActors(NxActor** actor1, NxActor** actor2);

	//! phys_fn_004681 (0x000b2d10, 84 B). Slot 2: write lock, forwards to
	//! Joint::setGlobalAnchor (phys_fn_004099).
	virtual void setGlobalAnchor(const NxVec3&);

	//! Slot 3. Unclaimed: shared NpJoint body, folded at 0x000b0670
	//! (core\NpD6Joint.cpp). Task 9 implements the forward to
	//! Joint::getGlobalAnchor (phys_fn_004125) under the read lock.
	virtual void getGlobalAnchor(NxVec3&) const;

	//! phys_fn_004683 (0x000b2d70, 84 B). Slot 4: write lock, forwards to
	//! Joint::setGlobalAxis (phys_fn_004101).
	virtual void setGlobalAxis(const NxVec3&);

	//! Slot 5. Unclaimed: shared NpJoint body, folded at 0x000b0700
	//! (core\NpD6Joint.cpp). Task 9 implements the forward to
	//! Joint::getGlobalAxis (phys_fn_004129) under the read lock.
	virtual void getGlobalAxis(NxVec3&) const;

	//! Slot 6. Unclaimed: shared NpJoint body, folded at 0x000b0d30
	//! (core\NpPulleyJoint.cpp). Task 9 implements the forward to
	//! Joint::getGlobalAnchorVal (phys_fn_004137).
	virtual NxVec3 getGlobalAnchorVal() const;

	//! Slot 7. Unclaimed: shared NpJoint body, folded at 0x000b0d60
	//! (core\NpPulleyJoint.cpp). Task 9 implements the forward to
	//! Joint::getGlobalAxisVal (phys_fn_004139).
	virtual NxVec3 getGlobalAxisVal() const;

	//! Slot 8. Unclaimed: shared NpJoint body, folded at 0x000b0dc0
	//! (core\NpPulleyJoint.cpp; on the transcript path). Task 9
	//! implements the forward to Joint::getState (phys_fn_004078) under
	//! the read lock.
	virtual NxJointState getState();

	//! phys_fn_004685 (0x000b2dd0, 89 B). Slot 9: write lock, forwards to
	//! Joint::setBreakable (phys_fn_004074).
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);

	//! Slot 10. Unclaimed: shared NpJoint body, folded at 0x000b0c40
	//! (core\NpPointInPlaneJoint.cpp). Task 9 implements the forward to
	//! Joint::getBreakable (phys_fn_004076).
	virtual void getBreakable(NxReal& maxForce, NxReal& maxTorque);

	//! phys_fn_004687 (0x000b2e30, 89 B). Slot 11: write lock, forwards to
	//! Joint::setLimitPoint (phys_fn_004109).
	virtual void setLimitPoint(const NxVec3& point, bool pointIsOnBody2 = true);

	//! Slot 12. Unclaimed: shared NpJoint body, folded at 0x000b0c70
	//! (core\NpPointInPlaneJoint.cpp). Task 9 implements the forward to
	//! Joint::getLimitPoint (phys_fn_004080).
	virtual bool getLimitPoint(NxVec3& worldLimitPoint);

	//! phys_fn_004689 (0x000b2e90, 97 B). Slot 13: write lock, forwards to
	//! Joint::addLimitPlane (phys_fn_004143).
	virtual bool addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane);

	//! phys_fn_004695 (0x000b2fb0, 74 B). Slot 14: write lock, forwards to
	//! Joint::purgeLimitPlanes (phys_fn_004089), tail-jumps to unlock.
	virtual void purgeLimitPlanes();

	//! phys_fn_004691 (0x000b2f00, 74 B). Slot 15: write lock, forwards to
	//! Joint::resetLimitPlaneIterator (phys_fn_004081), tail-jumps to
	//! unlock.
	virtual void resetLimitPlaneIterator();

	//! Slot 16. Unclaimed: shared NpJoint body, folded at 0x000b0de0
	//! (core\NpPulleyJoint.cpp). Task 9 implements the forward to
	//! Joint::hasMoreLimitPlanes (phys_fn_004083).
	virtual bool hasMoreLimitPlanes();

	//! Slot 17. Unclaimed: shared NpJoint body, folded at 0x000b1d10
	//! (core\NpSphericalJoint.cpp). Task 9 implements the forward to
	//! Joint::getNextLimitPlane (phys_fn_004145).
	virtual bool getNextLimitPlane(NxVec3& planeNormal, NxReal& planeD);

	//! Slot 18. Unclaimed: shared NpJoint body, folded at 0x000b06f0
	//! (core\NpD6Joint.cpp). Task 9 implements the forward to
	//! Joint::getType (phys_fn_004070).
	virtual NxJointType getType() const;

	//! Slot 19. Unclaimed: shared NpJoint body, folded at 0x000b0d90
	//! (core\NpPulleyJoint.cpp). Task 9 implements: returns `this` if the
	//! argument equals getType(), else 0.
	virtual void* is(NxJointType) const;

	//! Slots 20-28 (isRevoluteJoint..isPulleyJoint): compiler-generated
	//! from the inline bodies in NxJoint.h; not overridden here.

	//! phys_fn_004693 (0x000b2f50, 88 B). Slot 29: write lock, forwards to
	//! nxSetSdkPointerBinding([np+0x18], name).
	virtual void setName(const char*);

	//! Slot 30. Unclaimed: shared NpJoint body, folded at 0x000b3540
	//! (core\NpPrismaticJoint.cpp). Task 9 implements the forward to
	//! nxGetSdkPointerBinding([np+0x18]).
	virtual const char* getName() const;

	// --- NxRevoluteJoint pure virtuals, in NxRevoluteJoint.h order ---

	//! phys_fn_004697 (0x000b3000, 84 B). Slot 31: write lock (line 0x12),
	//! forwards to internal slot 9 (RevoluteJoint::loadFromDesc,
	//! phys_fn_004370).
	virtual void loadFromDesc(const NxRevoluteJointDesc&);

	//! phys_fn_004699 (0x000b3060, 84 B). Slot 32: write lock (line 0x1d),
	//! forwards to internal slot 10 (RevoluteJoint::saveToDesc,
	//! phys_fn_004330).
	virtual void saveToDesc(NxRevoluteJointDesc&);

	//! phys_fn_004709 (0x000b31e0, 84 B). Slot 33: write lock, forwards to
	//! RevoluteJoint::setLimits (phys_fn_004340).
	virtual void setLimits(const NxJointLimitPairDesc&);

	//! phys_fn_004711 (0x000b3240, 45 B). Slot 34: read lock, forwards to
	//! RevoluteJoint::getLimits (phys_fn_004342), returns its `al`.
	virtual bool getLimits(NxJointLimitPairDesc&);

	//! phys_fn_004713 (0x000b3270, 8 B). Slot 35: no lock;
	//! `mov ecx,[ecx+0x18]; jmp` RevoluteJoint::setMotor (phys_fn_004344).
	virtual void setMotor(const NxMotorDesc&);

	//! phys_fn_004715 (0x000b3280, 45 B). Slot 36: read lock, forwards to
	//! RevoluteJoint::getMotor (phys_fn_004346).
	virtual bool getMotor(NxMotorDesc&);

	//! phys_fn_004717 (0x000b32b0, 84 B). Slot 37: write lock, forwards to
	//! RevoluteJoint::setSpring (phys_fn_004348).
	virtual void setSpring(const NxSpringDesc&);

	//! phys_fn_004719 (0x000b3310, 45 B). Slot 38: read lock, forwards to
	//! RevoluteJoint::getSpring (phys_fn_004350).
	virtual bool getSpring(NxSpringDesc&);

	//! phys_fn_004721 (0x000b3340, 42 B). Slot 39: read lock, forwards to
	//! RevoluteJoint::getAngle (phys_fn_004372); float returned in st(0).
	virtual NxReal getAngle();

	//! phys_fn_004723 (0x000b3370, 42 B). Slot 40: read lock, forwards to
	//! RevoluteJoint::getVelocity (phys_fn_004354); float returned in
	//! st(0).
	virtual NxReal getVelocity();

	//! phys_fn_004701 (0x000b30c0, 84 B). Slot 41: write lock (line 0x25),
	//! forwards to internal slot 11 (RevoluteJoint::setFlags,
	//! phys_fn_004334).
	virtual void setFlags(NxU32 flags);

	//! phys_fn_004703 (0x000b3120, 36 B). Slot 42: read lock, forwards to
	//! internal slot 12 (RevoluteJoint::getFlags, phys_fn_004336); also
	//! installed by the NpSphericalJoint ctor (folded).
	virtual NxU32 getFlags();

	//! phys_fn_004705 (0x000b3150, 84 B). Slot 43: write lock (line 0x32),
	//! forwards to internal slot 13 (RevoluteJoint::setProjectionMode,
	//! phys_fn_004338).
	virtual void setProjectionMode(NxJointProjectionMode projectionMode);

	//! phys_fn_004707 (0x000b31b0, 36 B). Slot 44: read lock, forwards to
	//! internal slot 14 (RevoluteJoint::getProjectionMode, phys_fn_004186
	//! folded).
	virtual NxJointProjectionMode getProjectionMode();

	// --- fields, in the oracle's byte-offset order (0x1c bytes total:
	//     0xc primary base + 0xc secondary base + 0x4 own member) ---

	//! +0x10 / +0x14 (EmbeddedHookBase::mWord04 / mWord08 here). Scene
	//! write-lock link (tryLock phys_fn_002364 / unlock phys_fn_002366 in
	//! every setter) and scene read-lock link (lock phys_fn_002362 /
	//! unlock phys_fn_002366 in every getter) -- the same meaning as
	//! NpScene::mWriteLock (+0x0c) / mReadLock (+0x10).

	//! +0x18. The internal RevoluteJoint* every accessor forwards to
	//! (also duplicated into NxJoint::appData at +0x08 by the
	//! constructor).
	RevoluteJoint*		mInternal;
	};

static_assert(sizeof(NxJoint) == 0xc, "NxJoint is vptr+userData+appData, twelve bytes");
static_assert(sizeof(NxRevoluteJoint) == sizeof(NxJoint), "NxRevoluteJoint adds no data members");
static_assert(sizeof(EmbeddedHookBase) == 0xc, "the hook base is a vptr plus two words");
static_assert(sizeof(NpRevoluteJoint) == 0x1c, "NpRevoluteJoint is 0x1c bytes in the oracle");
static_assert(offsetof(NpRevoluteJoint, mInternal) == 0x18, "the internal pointer is at +0x18");

#endif
