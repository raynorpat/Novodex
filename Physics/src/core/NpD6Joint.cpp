/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpD6Joint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3i. The rows below are the prismatic shape
// (core/NpPrismaticJoint.cpp): the NxJoint setters call NpJointShared's
// shared forward* bodies with this unit's __FILE__ and line; the 13 folded
// NxJoint bodies live in core/NpJointShared.cpp (004437, 004441 and 004443
// sit inside this unit's address range but are NpJointShared's). Every
// write-locked NxJoint row reports line 0x11; loadFromDesc 0x15 and
// saveToDesc 0x20; the four drive setters 0x28, 0x2f, 0x36 and 0x3d. The
// constructor / thunk / destructor triple (004469-004473) sits after the
// unit's evidenced span, in the gap work_units.json calls
// `gap:core\NpD6Joint.cpp..core\NpPulleyJoint.cpp`: 004210 is its only
// caller and it installs the D6 tables. See units/joint-families-contract.md
// "## D6".

// The oracle's __FILE__ for this unit (the string at 0x1011a794 every
// write-lock report pushes, e.g. 0xb0a73).
#define NX_NPD6JOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpD6Joint.cpp"

// phys_fn_004435 (0x000b0610, 84 B)
void NpD6Joint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPD6JOINT_CPP, 0x11, anchor);
	}

// phys_fn_004439 (0x000b06a0, 84 B)
void NpD6Joint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPD6JOINT_CPP, 0x11, axis);
	}

// phys_fn_004445 (0x000b0760, 89 B)
void NpD6Joint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPD6JOINT_CPP, 0x11, maxForce, maxTorque);
	}

// phys_fn_004447 (0x000b07c0, 89 B)
void NpD6Joint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPD6JOINT_CPP, 0x11, point, pointIsOnBody2);
	}

// phys_fn_004449 (0x000b0820, 97 B)
bool NpD6Joint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPD6JOINT_CPP, 0x11, normal, pointInPlane);
	}

// phys_fn_004451 (0x000b0890, 74 B)
void NpD6Joint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPD6JOINT_CPP, 0x11);
	}

// phys_fn_004453 (0x000b08e0, 88 B)
void NpD6Joint::setName(const char* name)
	{
	forwardSetName(NX_NPD6JOINT_CPP, 0x11, name);
	}

// phys_fn_004455 (0x000b0940, 74 B)
void NpD6Joint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPD6JOINT_CPP, 0x11);
	}

// phys_fn_004457 (0x000b0990, 84 B)
// Internal slot 9 (D6Joint::loadFromDesc), dispatched through mInternal's
// own vtable as the listing's `[[this+0x18]]+0x24` call (0xb09d5).
void NpD6Joint::loadFromDesc(const NxD6JointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPD6JOINT_CPP, 0x15, desc);
	}

// phys_fn_004459 (0x000b09f0, 84 B)
// Internal slot 10 (D6Joint::saveToDesc, `[vt+0x28]`, 0xb0a35).
void NpD6Joint::saveToDesc(NxD6JointDesc& desc)
	{
	forwardSaveToDesc(NX_NPD6JOINT_CPP, 0x20, desc);
	}

// phys_fn_004461 (0x000b0a50, 84 B)
// Write lock (line 0x28), then a direct call of the internal setter with the
// argument (`push ecx; mov ecx,[esi+0x18]; call 0x100a0f60`, 0xb0a8f-0xb0a93):
// the folded empty body 004248, so nothing is stored. The candidate inlines
// the empty D6Joint::setDrivePosition; the lock and unlock remain.
void NpD6Joint::setDrivePosition(const NxVec3& position)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		reportWriteLocked(NX_NPD6JOINT_CPP, 0x28);
		return;
		}
	mInternal->setDrivePosition(position);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004463 (0x000b0ab0, 84 B)
// As 004461 with line 0x2f (0xb0ab0-0xb0b01).
void NpD6Joint::setDriveOrientation(const NxQuat& orientation)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		reportWriteLocked(NX_NPD6JOINT_CPP, 0x2f);
		return;
		}
	mInternal->setDriveOrientation(orientation);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004465 (0x000b0b10, 84 B)
// As 004461 with line 0x36 (0xb0b10-0xb0b61).
void NpD6Joint::setDriveLinearVelocity(const NxVec3& linVel)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		reportWriteLocked(NX_NPD6JOINT_CPP, 0x36);
		return;
		}
	mInternal->setDriveLinearVelocity(linVel);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004467 (0x000b0b70, 84 B)
// As 004461 with line 0x3d (0xb0b70-0xb0bc1).
void NpD6Joint::setDriveAngularVelocity(const NxVec3& angVel)
	{
	void* link = writeLink();
	if(!nxNpSceneGuardWriteTry(link))
		{
		reportWriteLocked(NX_NPD6JOINT_CPP, 0x3d);
		return;
		}
	mInternal->setDriveAngularVelocity(angVel);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004469 (0x000b0bd0, 57 B)
// As phys_fn_004753 (NpPrismaticJoint): NxJoint()'s inline constructor zeroes
// userData/appData (0xb0bdb/0xb0bde), the transient NxD6Joint table
// (0x1011a700), the secondary table (0x1011a85c) and the final table
// (0x1011a7c8) are installed by ordinary base/derived construction in the
// oracle's order, and the shared NpJointShared constructor zeroes the hook
// base's two words (phys_fn_002404) and stores `internal` at +0x18 and +0x08.
NpD6Joint::NpD6Joint(D6Joint* internal)
	: NpJointShared<NxD6Joint, D6Joint>(internal)
	{
	}

// phys_fn_004471 (0x000b0c10, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpD6Joint>") the second base (EmbeddedHookBase) needs for this shared
// virtual destructor, emitted once ~NpD6Joint() is defined.
// phys_fn_004473 (0x000b0c20, 55 B)
NpD6Joint::~NpD6Joint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the Foundation allocator, slot +0x14).
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/D6Joint.h. Not an oracle row; see the declaration.
NxJoint* nxD6JointAttachScene(D6Joint* internal, void* writeLink, void* readLink)
	{
	NpD6Joint* np = static_cast<NpD6Joint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
