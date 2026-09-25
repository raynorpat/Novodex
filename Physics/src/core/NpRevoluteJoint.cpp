/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpRevoluteJoint.h"

// Scaffold only (Phase 6 Task 5). The 25 claimed rows below get their
// stable-ID stubs; Task 9 replaces them. The 13 pure virtuals NpRevoluteJoint
// inherits from NxJoint/NxRevoluteJoint but does not claim (their code is a
// folded copy living in another Np*Joint.cpp unit -- see
// revolute-contract.md "## Task split") still need a definition here so the
// class's vtable can link; Task 9 replaces those bodies with the forwarding
// call and the "Shared NpJoint body" comment form the contract specifies.
// Nothing constructs NpRevoluteJoint until Task 10, so none of this is
// reachable yet.

// phys_fn_004725 (0x000b33a0, 57 B)
// (unimplemented)
NpRevoluteJoint::NpRevoluteJoint(RevoluteJoint* internal)
	{
	(void)internal;
	NX_ASSERT(0);
	}

// phys_fn_004729 (0x000b33f0, 55 B)
// (unimplemented)
NpRevoluteJoint::~NpRevoluteJoint()
	{
	NX_ASSERT(0);
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b1600 (core\NpDistanceJoint.cpp). Task 9 implements the forward.
// (unimplemented)
void NpRevoluteJoint::getActors(NxActor** actor1, NxActor** actor2)
	{
	(void)actor1;
	(void)actor2;
	NX_ASSERT(0);
	}

// phys_fn_004681 (0x000b2d10, 84 B)
// (unimplemented)
void NpRevoluteJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	(void)anchor;
	NX_ASSERT(0);
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0670 (core\NpD6Joint.cpp). Task 9 implements the forward.
// (unimplemented)
void NpRevoluteJoint::getGlobalAnchor(NxVec3& out) const
	{
	(void)out;
	NX_ASSERT(0);
	}

// phys_fn_004683 (0x000b2d70, 84 B)
// (unimplemented)
void NpRevoluteJoint::setGlobalAxis(const NxVec3& axis)
	{
	(void)axis;
	NX_ASSERT(0);
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0700 (core\NpD6Joint.cpp). Task 9 implements the forward.
// (unimplemented)
void NpRevoluteJoint::getGlobalAxis(NxVec3& out) const
	{
	(void)out;
	NX_ASSERT(0);
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0d30 (core\NpPulleyJoint.cpp). Task 9 implements the forward.
// (unimplemented)
NxVec3 NpRevoluteJoint::getGlobalAnchorVal() const
	{
	NX_ASSERT(0);
	return NxVec3();
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0d60 (core\NpPulleyJoint.cpp). Task 9 implements the forward.
// (unimplemented)
NxVec3 NpRevoluteJoint::getGlobalAxisVal() const
	{
	NX_ASSERT(0);
	return NxVec3();
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0dc0 (core\NpPulleyJoint.cpp). Task 9 implements the forward.
// (unimplemented)
NxJointState NpRevoluteJoint::getState()
	{
	NX_ASSERT(0);
	return NX_JS_UNBOUND;
	}

// phys_fn_004685 (0x000b2dd0, 89 B)
// (unimplemented)
void NpRevoluteJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	(void)maxForce;
	(void)maxTorque;
	NX_ASSERT(0);
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0c40 (core\NpPointInPlaneJoint.cpp). Task 9 implements the forward.
// (unimplemented)
void NpRevoluteJoint::getBreakable(NxReal& maxForce, NxReal& maxTorque)
	{
	(void)maxForce;
	(void)maxTorque;
	NX_ASSERT(0);
	}

// phys_fn_004687 (0x000b2e30, 89 B)
// (unimplemented)
void NpRevoluteJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	(void)point;
	(void)pointIsOnBody2;
	NX_ASSERT(0);
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0c70 (core\NpPointInPlaneJoint.cpp). Task 9 implements the forward.
// (unimplemented)
bool NpRevoluteJoint::getLimitPoint(NxVec3& worldLimitPoint)
	{
	(void)worldLimitPoint;
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004689 (0x000b2e90, 97 B)
// (unimplemented)
bool NpRevoluteJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	(void)normal;
	(void)pointInPlane;
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004695 (0x000b2fb0, 74 B)
// (unimplemented)
void NpRevoluteJoint::purgeLimitPlanes()
	{
	NX_ASSERT(0);
	}

// phys_fn_004691 (0x000b2f00, 74 B)
// (unimplemented)
void NpRevoluteJoint::resetLimitPlaneIterator()
	{
	NX_ASSERT(0);
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0de0 (core\NpPulleyJoint.cpp). Task 9 implements the forward.
// (unimplemented)
bool NpRevoluteJoint::hasMoreLimitPlanes()
	{
	NX_ASSERT(0);
	return false;
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b1d10 (core\NpSphericalJoint.cpp). Task 9 implements the forward.
// (unimplemented)
bool NpRevoluteJoint::getNextLimitPlane(NxVec3& planeNormal, NxReal& planeD)
	{
	(void)planeNormal;
	(void)planeD;
	NX_ASSERT(0);
	return false;
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b06f0 (core\NpD6Joint.cpp). Task 9 implements the forward.
// (unimplemented)
NxJointType NpRevoluteJoint::getType() const
	{
	NX_ASSERT(0);
	return NX_JOINT_REVOLUTE;
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b0d90 (core\NpPulleyJoint.cpp). Task 9 implements the forward.
// (unimplemented)
void* NpRevoluteJoint::is(NxJointType) const
	{
	NX_ASSERT(0);
	return 0;
	}

// phys_fn_004693 (0x000b2f50, 88 B)
// (unimplemented)
void NpRevoluteJoint::setName(const char* name)
	{
	(void)name;
	NX_ASSERT(0);
	}

// Unclaimed folded NpJoint body; the oracle keeps one folded copy at
// 0x000b3540 (core\NpPrismaticJoint.cpp). Task 9 implements the forward.
// (unimplemented)
const char* NpRevoluteJoint::getName() const
	{
	NX_ASSERT(0);
	return 0;
	}

// phys_fn_004697 (0x000b3000, 84 B)
// (unimplemented)
void NpRevoluteJoint::loadFromDesc(const NxRevoluteJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004699 (0x000b3060, 84 B)
// (unimplemented)
void NpRevoluteJoint::saveToDesc(NxRevoluteJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004709 (0x000b31e0, 84 B)
// (unimplemented)
void NpRevoluteJoint::setLimits(const NxJointLimitPairDesc& limits)
	{
	(void)limits;
	NX_ASSERT(0);
	}

// phys_fn_004711 (0x000b3240, 45 B)
// (unimplemented)
bool NpRevoluteJoint::getLimits(NxJointLimitPairDesc& limits)
	{
	(void)limits;
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004713 (0x000b3270, 8 B)
// (unimplemented)
void NpRevoluteJoint::setMotor(const NxMotorDesc& motor)
	{
	(void)motor;
	NX_ASSERT(0);
	}

// phys_fn_004715 (0x000b3280, 45 B)
// (unimplemented)
bool NpRevoluteJoint::getMotor(NxMotorDesc& motor)
	{
	(void)motor;
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004717 (0x000b32b0, 84 B)
// (unimplemented)
void NpRevoluteJoint::setSpring(const NxSpringDesc& spring)
	{
	(void)spring;
	NX_ASSERT(0);
	}

// phys_fn_004719 (0x000b3310, 45 B)
// (unimplemented)
bool NpRevoluteJoint::getSpring(NxSpringDesc& spring)
	{
	(void)spring;
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004721 (0x000b3340, 42 B)
// (unimplemented)
NxReal NpRevoluteJoint::getAngle()
	{
	NX_ASSERT(0);
	return 0.0f;
	}

// phys_fn_004723 (0x000b3370, 42 B)
// (unimplemented)
NxReal NpRevoluteJoint::getVelocity()
	{
	NX_ASSERT(0);
	return 0.0f;
	}

// phys_fn_004701 (0x000b30c0, 84 B)
// (unimplemented)
void NpRevoluteJoint::setFlags(NxU32 flags)
	{
	(void)flags;
	NX_ASSERT(0);
	}

// phys_fn_004703 (0x000b3120, 36 B)
// (unimplemented)
NxU32 NpRevoluteJoint::getFlags()
	{
	NX_ASSERT(0);
	return 0;
	}

// phys_fn_004705 (0x000b3150, 84 B)
// (unimplemented)
void NpRevoluteJoint::setProjectionMode(NxJointProjectionMode projectionMode)
	{
	(void)projectionMode;
	NX_ASSERT(0);
	}

// phys_fn_004707 (0x000b31b0, 36 B)
// (unimplemented)
NxJointProjectionMode NpRevoluteJoint::getProjectionMode()
	{
	NX_ASSERT(0);
	return NX_JPM_NONE;
	}

// phys_fn_004727 (0x000b33e0, 8 B; no decompile, Capstone listing only) is
// not defined here: it is the compiler-generated adjustor thunk for
// EmbeddedHookBase's virtual destructor, emitted automatically because
// ~NpRevoluteJoint() above is defined.
