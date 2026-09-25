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
// readLink()) -- report on a failed write tryLock, call the internal
// RevoluteJoint/Joint method through mInternal (+0x18), unlock with the
// same captured link value. Task 10 wired Scene::createJoint to construct it
// (through RevoluteJoint's constructor, phys_fn_004366).
//
// Joint-families Task 1: the 13 NxJoint bodies the oracle keeps as one
// identical-code-folded copy for all ten families now live once in
// core/NpJointShared.cpp (NpJointShared<NxRevoluteJoint, RevoluteJoint>),
// and the NxJoint setter rows below (slots 2, 4, 9, 11, 13-15, 29, and the
// descriptor pair 31/32) call NpJointShared's shared forward* bodies with
// this unit's __FILE__ and line. See units/joint-families-contract.md.

// The oracle's __FILE__ for this unit (every write-lock report in it pushes
// the string at 0xb2d33/0xb2d93/etc; the four rows with their own source
// line push it again at their own address -- see the bundle entries).
#define NX_NPREVOLUTEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpRevoluteJoint.cpp"

// phys_fn_004725 (0x000b33a0, 57 B)
// NxJoint()'s inline default constructor -- called automatically as part of
// the NxRevoluteJoint primary-base construction -- already zeroes
// userData/appData (0xb33ab/0xb33ae); the transient NxRevoluteJoint vtable
// (0x1011b238) and the two final vtables (0x1011b328 primary, 0x1011b3dc
// secondary) are installed automatically by ordinary C++ base/derived
// construction, in the oracle's exact order, because neither base overloads
// a virtual and NpJointShared is __declspec(novtable). The shared
// NpJointShared constructor zeroes the hook base's two words (reproducing the
// zeroing of phys_fn_002404, 0xb33b7; not claimed -- gap
// Controller.cpp..Fluid.cpp, see revolute-contract.md's reuse table), then
// stores `internal` at +0x18 (0xb33c6) and +0x08 (0xb33c9, the second write;
// NxJoint() zeroed it first).
NpRevoluteJoint::NpRevoluteJoint(RevoluteJoint* internal)
	: NpJointShared<NxRevoluteJoint, RevoluteJoint>(internal)
	{
	}

// phys_fn_004727 (0x000b33e0, 8 B)
// This row is not defined here: it is the compiler-generated adjustor
// thunk ("sub ecx,0xc; jmp <~NpRevoluteJoint>") the second base
// (EmbeddedHookBase) needs for this shared virtual destructor, emitted
// automatically now that ~NpRevoluteJoint() is defined. It has no decompile
// anywhere -- Capstone listing only.
// phys_fn_004729 (0x000b33f0, 55 B)
NpRevoluteJoint::~NpRevoluteJoint()
	{
	// Nothing to do in the body: the base-destruction chain the compiler
	// generates for this multiple-inheritance shape reproduces the rest of
	// this row (phys_fn_004729) automatically -- EmbeddedHookBase's own
	// (empty) destructor resets the secondary vptr to its own table
	// (0x101088b8, the not-claimed phys_fn_002406), then NxJoint's abstract
	// destructor resets the primary vptr to 0x1011a680 -- and the scalar-deleting-
	// destructor wrapper the compiler generates around this body supplies
	// the flags&1 free, through the `operator delete` NpJointShared.h
	// declares (the SDK allocator, matching phys_fn_004729's tail exactly).
	}

// phys_fn_004681 (0x000b2d10, 84 B)
void NpRevoluteJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPREVOLUTEJOINT_CPP, 0xe, anchor);
	}

// phys_fn_004683 (0x000b2d70, 84 B)
void NpRevoluteJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPREVOLUTEJOINT_CPP, 0xe, axis);
	}

// phys_fn_004685 (0x000b2dd0, 89 B)
void NpRevoluteJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPREVOLUTEJOINT_CPP, 0xe, maxForce, maxTorque);
	}

// phys_fn_004687 (0x000b2e30, 89 B)
void NpRevoluteJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPREVOLUTEJOINT_CPP, 0xe, point, pointIsOnBody2);
	}

// phys_fn_004689 (0x000b2e90, 97 B)
bool NpRevoluteJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPREVOLUTEJOINT_CPP, 0xe, normal, pointInPlane);
	}

// phys_fn_004695 (0x000b2fb0, 74 B)
void NpRevoluteJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPREVOLUTEJOINT_CPP, 0xe);
	}

// phys_fn_004691 (0x000b2f00, 74 B)
void NpRevoluteJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPREVOLUTEJOINT_CPP, 0xe);
	}

// phys_fn_004693 (0x000b2f50, 88 B)
void NpRevoluteJoint::setName(const char* name)
	{
	forwardSetName(NX_NPREVOLUTEJOINT_CPP, 0xe, name);
	}

// phys_fn_004697 (0x000b3000, 84 B)
// Slot +0x24 is virtual (RevoluteJoint::loadFromDesc, internal slot 9); the
// plain call below dispatches through mInternal's own vtable exactly as the
// listing's `[[this+0x18]]+0x24]` indirect call does.
void NpRevoluteJoint::loadFromDesc(const NxRevoluteJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPREVOLUTEJOINT_CPP, 0x12, desc);
	}

// phys_fn_004699 (0x000b3060, 84 B)
// Internal slot 10 (RevoluteJoint::saveToDesc), dispatched the same way as
// loadFromDesc above.
void NpRevoluteJoint::saveToDesc(NxRevoluteJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPREVOLUTEJOINT_CPP, 0x1d, desc);
	}

// phys_fn_004709 (0x000b31e0, 84 B)
void NpRevoluteJoint::setLimits(const NxJointLimitPairDesc& limits)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		reportWriteLocked(NX_NPREVOLUTEJOINT_CPP, 0x3f);
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
		reportWriteLocked(NX_NPREVOLUTEJOINT_CPP, 0x57);
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
// returns it after unlocking. This file compiles SSE2, so the NxF64 result
// is narrowed via `cvtsd2ss` rather than the oracle's `fstp dword`;
// identical under the default 0x027f control word the public API runs
// under (same applies to 004723 below).
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
		reportWriteLocked(NX_NPREVOLUTEJOINT_CPP, 0x25);
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
		reportWriteLocked(NX_NPREVOLUTEJOINT_CPP, 0x32);
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
