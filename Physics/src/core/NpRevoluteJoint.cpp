/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpRevoluteJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Task 9 (Phase 6). The 25 claimed rows below are lock-bracketed forwarders:
// lock the link VALUE held at np+0x10 (write) or np+0x14 (read) -- not the
// address of that field, but the pointer the field stores (writeLink()/
// readLink() below) -- report on a failed write tryLock, call the internal
// RevoluteJoint/Joint method through mInternal (+0x18), unlock with the
// same captured link value. The 13 pure virtuals NpRevoluteJoint inherits
// from NxJoint/NxRevoluteJoint but does not claim -- their code is an
// identical-code-folded copy living in another Np*Joint.cpp unit -- are
// implemented here as the same forward, with the "Shared NpJoint body"
// comment form revolute-contract.md specifies instead of a stable ID.
// Task 10 wired Scene::createJoint to construct it (through
// RevoluteJoint's constructor, phys_fn_004366).

// The oracle's __FILE__ for this unit (every write-lock report in it pushes
// the string at 0xb2d33/0xb2d93/etc; the four rows with their own source
// line push it again at their own address -- see the bundle entries).
#define NX_NPREVOLUTEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpRevoluteJoint.cpp"

// The write-lock failure message, shared by every macro-generated setter row
// (0xb2d2a and the same string repeated at every other setGuard call site).
static const char* const gNpRevoluteJointLockMsg =
	"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!";

// phys_fn_004725 (0x000b33a0, 57 B)
NpRevoluteJoint::NpRevoluteJoint(RevoluteJoint* internal)
	{
	// NxJoint()'s inline default constructor -- called automatically as
	// part of the NxRevoluteJoint primary-base construction -- already
	// zeroes userData/appData (0xb33ab/0xb33ae); the transient
	// NxRevoluteJoint vtable (0x1011b238) and the two final vtables
	// (0x1011b328 primary, 0x1011b3dc secondary) are installed
	// automatically by ordinary C++ base/derived construction, in the
	// oracle's exact order, because neither base overloads a virtual. Only
	// the hook base's two words need an explicit zero here: EmbeddedHookBase
	// has no constructor of its own, so this reproduces phys_fn_002404's
	// zeroing (0xb33b7; not claimed -- gap Controller.cpp..Fluid.cpp, see
	// revolute-contract.md's reuse table).
	mWord04 = 0;
	mWord08 = 0;
	mInternal = internal;			// 0xb33c6
	appData = internal;			// 0xb33c9 (second write; NxJoint() zeroed it first)
	}

// phys_fn_004729 (0x000b33f0, 55 B)
// The row below (phys_fn_004727) is not defined here: it is the
// compiler-generated adjustor thunk ("sub ecx,0xc; jmp <~NpRevoluteJoint>")
// the second base (EmbeddedHookBase) needs for this shared virtual
// destructor, emitted automatically now that ~NpRevoluteJoint() is defined.
// It has no decompile anywhere -- Capstone listing only.
// phys_fn_004727 (0x000b33e0, 8 B)
NpRevoluteJoint::~NpRevoluteJoint()
	{
	// Nothing to do in the body: the base-destruction chain the compiler
	// generates for this multiple-inheritance shape reproduces the rest of
	// this row (phys_fn_004729) automatically -- EmbeddedHookBase's own
	// (empty) destructor resets the secondary vptr to its own table
	// (0x101088b8, the not-claimed phys_fn_002406), then NxJoint's abstract
	// destructor resets the primary vptr to 0x1011a680 -- and the scalar-deleting-
	// destructor wrapper the compiler generates around this body supplies
	// the flags&1 free, through the `operator delete` NpRevoluteJoint.h
	// declares (the SDK allocator, matching phys_fn_004729's tail exactly).
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b1600
// (phys_fn_004539).
void NpRevoluteJoint::getActors(NxActor** actor1, NxActor** actor2)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	JointBodyRecord* body0 = static_cast<JointBodyRecord*>(mInternal->mBody[0]);
	*actor1 = body0 ? *reinterpret_cast<NxActor**>(body0->mOwner) : 0;
	JointBodyRecord* body1 = static_cast<JointBodyRecord*>(mInternal->mBody[1]);
	if(body1)
		{
		*actor2 = *reinterpret_cast<NxActor**>(body1->mOwner);
		nxNpSceneGuardLeave(link);
		return;
		}
	*actor2 = 0;
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004681 (0x000b2d10, 84 B)
void NpRevoluteJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0xe, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->setGlobalAnchor(anchor);
	nxNpSceneGuardLeave(link);
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0670
// (phys_fn_004437).
void NpRevoluteJoint::getGlobalAnchor(NxVec3& out) const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	mInternal->getGlobalAnchor(out);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004683 (0x000b2d70, 84 B)
void NpRevoluteJoint::setGlobalAxis(const NxVec3& axis)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0xe, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->setGlobalAxis(axis);
	nxNpSceneGuardLeave(link);
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0700
// (phys_fn_004441).
void NpRevoluteJoint::getGlobalAxis(NxVec3& out) const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	mInternal->getGlobalAxis(out);
	nxNpSceneGuardLeave(link);
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0ff0
// (phys_fn_004497).
NxVec3 NpRevoluteJoint::getGlobalAnchorVal() const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxVec3 out = mInternal->getGlobalAnchorVal();
	nxNpSceneGuardLeave(link);
	return out;
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b1020
// (phys_fn_004499).
NxVec3 NpRevoluteJoint::getGlobalAxisVal() const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxVec3 out = mInternal->getGlobalAxisVal();
	nxNpSceneGuardLeave(link);
	return out;
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0dc0
// (phys_fn_004483). On the transcript path.
NxJointState NpRevoluteJoint::getState()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxJointState state = mInternal->getState();
	nxNpSceneGuardLeave(link);
	return state;
	}

// phys_fn_004685 (0x000b2dd0, 89 B)
void NpRevoluteJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0xe, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->setBreakable(maxForce, maxTorque);
	nxNpSceneGuardLeave(link);
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b1bd0
// (phys_fn_004573).
void NpRevoluteJoint::getBreakable(NxReal& maxForce, NxReal& maxTorque)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	mInternal->getBreakable(maxForce, maxTorque);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004687 (0x000b2e30, 89 B)
void NpRevoluteJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0xe, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->setLimitPoint(point, pointIsOnBody2);
	nxNpSceneGuardLeave(link);
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b1c60
// (phys_fn_004577).
bool NpRevoluteJoint::getLimitPoint(NxVec3& worldLimitPoint)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->getLimitPoint(worldLimitPoint);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_004689 (0x000b2e90, 97 B)
bool NpRevoluteJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0xe, 0, gNpRevoluteJointLockMsg);
		return false;
		}
	bool result = mInternal->addLimitPlane(normal, pointInPlane);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_004695 (0x000b2fb0, 74 B)
void NpRevoluteJoint::purgeLimitPlanes()
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0xe, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->purgeLimitPlanes();
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004691 (0x000b2f00, 74 B)
void NpRevoluteJoint::resetLimitPlaneIterator()
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0xe, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->resetLimitPlaneIterator();
	nxNpSceneGuardLeave(link);
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0f10
// (phys_fn_004491).
bool NpRevoluteJoint::hasMoreLimitPlanes()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->hasMoreLimitPlanes();
	nxNpSceneGuardLeave(link);
	return result;
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b25d0
// (phys_fn_004635).
bool NpRevoluteJoint::getNextLimitPlane(NxVec3& planeNormal, NxReal& planeD)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->getNextLimitPlane(planeNormal, planeD);
	nxNpSceneGuardLeave(link);
	return result;
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0730
// (phys_fn_004443).
NxJointType NpRevoluteJoint::getType() const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxJointType type = mInternal->getType();
	nxNpSceneGuardLeave(link);
	return type;
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0d20
// (phys_fn_004479). `(uint)this & (param_1 != getType()) - 1`: returns
// `this` when the argument equals getType(), else 0 -- written here as the
// equivalent conditional.
void* NpRevoluteJoint::is(NxJointType type) const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxJointType actual = mInternal->getType();
	nxNpSceneGuardLeave(link);
	return actual == type ? const_cast<NpRevoluteJoint*>(this) : 0;
	}

// phys_fn_004693 (0x000b2f50, 88 B)
void NpRevoluteJoint::setName(const char* name)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0xe, 0, gNpRevoluteJointLockMsg);
		return;
		}
	nxSetSdkPointerBinding(mInternal, const_cast<char*>(name));
	nxNpSceneGuardLeave(link);
	}

// Shared NpJoint body; the oracle keeps one folded copy at 0x000b3670
// (phys_fn_004743).
const char* NpRevoluteJoint::getName() const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	const char* name = static_cast<const char*>(nxGetSdkPointerBinding(mInternal));
	nxNpSceneGuardLeave(link);
	return name;
	}

// phys_fn_004697 (0x000b3000, 84 B)
// Slot +0x24 is virtual (RevoluteJoint::loadFromDesc, internal slot 9); the
// plain call below dispatches through mInternal's own vtable exactly as the
// listing's `[[this+0x18]]+0x24]` indirect call does.
void NpRevoluteJoint::loadFromDesc(const NxRevoluteJointDesc& desc)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0x12, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->loadFromDesc(desc);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004699 (0x000b3060, 84 B)
// Internal slot 10 (RevoluteJoint::saveToDesc), dispatched the same way as
// loadFromDesc above.
void NpRevoluteJoint::saveToDesc(NxRevoluteJointDesc& desc)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0x1d, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->saveToDesc(desc);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004709 (0x000b31e0, 84 B)
void NpRevoluteJoint::setLimits(const NxJointLimitPairDesc& limits)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0x3f, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->setLimits(limits);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004711 (0x000b3240, 45 B)
// Locked copy-and-flag accessor shape (ObjectModel.cpp's nxLockedCopyAndFlag);
// returns RevoluteJoint::getLimits' bit-0 result.
bool NpRevoluteJoint::getLimits(NxJointLimitPairDesc& limits)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->getLimits(limits);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_004713 (0x000b3270, 8 B)
// No lock: `mov ecx,[ecx+0x18]; jmp` RevoluteJoint::setMotor.
void NpRevoluteJoint::setMotor(const NxMotorDesc& motor)
	{
	mInternal->setMotor(motor);
	}

// phys_fn_004715 (0x000b3280, 45 B)
// Same shape as getLimits above.
bool NpRevoluteJoint::getMotor(NxMotorDesc& motor)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->getMotor(motor);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_004717 (0x000b32b0, 84 B)
void NpRevoluteJoint::setSpring(const NxSpringDesc& spring)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0x57, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->setSpring(spring);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004719 (0x000b3310, 45 B)
// Same shape as getLimits/getMotor above.
bool NpRevoluteJoint::getSpring(NxSpringDesc& spring)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->getSpring(spring);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_004721 (0x000b3340, 42 B)
// RevoluteJoint::getAngle (phys_fn_004372) returns its result unrounded in
// st(0); this row rounds it to float with its own `fstp dword` BEFORE
// unlocking (0xb3357), then reloads it after the unlock (0xb3362) -- the
// reload has no observable effect since the value is already fixed at float
// precision, so the C++ below computes the float result under the lock and
// returns it after unlocking.
NxReal NpRevoluteJoint::getAngle()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxReal angle = static_cast<NxReal>(mInternal->getAngle());
	nxNpSceneGuardLeave(link);
	return angle;
	}

// phys_fn_004723 (0x000b3370, 42 B)
// Same shape as getAngle above, over RevoluteJoint::getVelocity
// (phys_fn_004354).
NxReal NpRevoluteJoint::getVelocity()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxReal velocity = static_cast<NxReal>(mInternal->getVelocity());
	nxNpSceneGuardLeave(link);
	return velocity;
	}

// phys_fn_004701 (0x000b30c0, 84 B)
// Internal slot 11 (RevoluteJoint::setFlags), dispatched through mInternal's
// own vtable.
void NpRevoluteJoint::setFlags(NxU32 flags)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0x25, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->setFlags(flags);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004703 (0x000b3120, 36 B)
// Lock-bracketed vtable call with no arguments (ObjectModel.cpp's
// nxLockedVtCallNoArg): read lock, internal slot 12 (RevoluteJoint::
// getFlags), unlock. Also installed by the folded NpSphericalJoint ctor
// (phys_fn_004649).
NxU32 NpRevoluteJoint::getFlags()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxU32 flags = mInternal->getFlags();
	nxNpSceneGuardLeave(link);
	return flags;
	}

// phys_fn_004705 (0x000b3150, 84 B)
// Internal slot 13 (RevoluteJoint::setProjectionMode), dispatched through
// mInternal's own vtable.
void NpRevoluteJoint::setProjectionMode(NxJointProjectionMode projectionMode)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPREVOLUTEJOINT_CPP, 0x32, 0, gNpRevoluteJointLockMsg);
		return;
		}
	mInternal->setProjectionMode(projectionMode);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004707 (0x000b31b0, 36 B)
// Same nxLockedVtCallNoArg shape as getFlags above, over internal slot 14
// (RevoluteJoint::getProjectionMode, folded phys_fn_004186).
NxJointProjectionMode NpRevoluteJoint::getProjectionMode()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxJointProjectionMode mode = mInternal->getProjectionMode();
	nxNpSceneGuardLeave(link);
	return mode;
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/RevoluteJoint.h. Not an oracle row; see the declaration.
NxJoint* nxRevoluteJointAttachScene(RevoluteJoint* internal, void* writeLink, void* readLink)
	{
	NpRevoluteJoint* np = static_cast<NpRevoluteJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
