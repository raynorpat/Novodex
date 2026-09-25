/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpPulleyJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3g. The rows below are the prismatic shape
// (core/NpPrismaticJoint.cpp): the NxJoint setters call NpJointShared's
// shared forward* bodies with this unit's __FILE__ and line; the 13 folded
// NxJoint bodies live in core/NpJointShared.cpp (five of them, 004479,
// 004483, 004491, 004497 and 004499, sit inside this unit's address range).
// Every write-locked NxJoint row reports line 0x10; loadFromDesc 0x14 and
// saveToDesc 0x1f. The unit ends with its own constructor / thunk /
// destructor triple (004505-004509). The three rows before it
// (004469-004473) are the D6 family's triple, not pulley's. See
// units/joint-families-contract.md "## Pulley".

// The oracle's __FILE__ for this unit (the string at 0x1011a8fc every
// write-lock report pushes, e.g. 0xb0c83).
#define NX_NPPULLEYJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpPulleyJoint.cpp"

// phys_fn_004475 (0x000b0c60, 84 B)
void NpPulleyJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPPULLEYJOINT_CPP, 0x10, anchor);
	}

// phys_fn_004477 (0x000b0cc0, 84 B)
void NpPulleyJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPPULLEYJOINT_CPP, 0x10, axis);
	}

// phys_fn_004481 (0x000b0d60, 89 B)
void NpPulleyJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPPULLEYJOINT_CPP, 0x10, maxForce, maxTorque);
	}

// phys_fn_004485 (0x000b0df0, 89 B)
void NpPulleyJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPPULLEYJOINT_CPP, 0x10, point, pointIsOnBody2);
	}

// phys_fn_004487 (0x000b0e50, 97 B)
bool NpPulleyJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPPULLEYJOINT_CPP, 0x10, normal, pointInPlane);
	}

// phys_fn_004489 (0x000b0ec0, 74 B)
void NpPulleyJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPPULLEYJOINT_CPP, 0x10);
	}

// phys_fn_004493 (0x000b0f40, 88 B)
void NpPulleyJoint::setName(const char* name)
	{
	forwardSetName(NX_NPPULLEYJOINT_CPP, 0x10, name);
	}

// phys_fn_004495 (0x000b0fa0, 74 B)
void NpPulleyJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPPULLEYJOINT_CPP, 0x10);
	}

// phys_fn_004501 (0x000b1050, 84 B)
// Internal slot 9 (PulleyJoint::loadFromDesc), dispatched through
// mInternal's own vtable as the listing's `[[this+0x18]]+0x24` call
// (0xb1095).
void NpPulleyJoint::loadFromDesc(const NxPulleyJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPPULLEYJOINT_CPP, 0x14, desc);
	}

// phys_fn_004503 (0x000b10b0, 84 B)
// Internal slot 10 (PulleyJoint::saveToDesc, `[vt+0x28]`, 0xb10f5).
void NpPulleyJoint::saveToDesc(NxPulleyJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPPULLEYJOINT_CPP, 0x1f, desc);
	}

// phys_fn_004505 (0x000b1110, 57 B)
// As phys_fn_004753 (NpPrismaticJoint): NxJoint()'s inline constructor zeroes
// userData/appData (0xb111b/0xb111e), the transient NxPulleyJoint table
// (0x1011a878), the secondary table (0x1011a9bc) and the final table
// (0x1011a938) are installed by ordinary base/derived construction in the
// oracle's order, and the shared NpJointShared constructor zeroes the hook
// base's two words (phys_fn_002404) and stores `internal` at +0x18 and +0x08.
NpPulleyJoint::NpPulleyJoint(PulleyJoint* internal)
	: NpJointShared<NxPulleyJoint, PulleyJoint>(internal)
	{
	}

// phys_fn_004507 (0x000b1150, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpPulleyJoint>") the second base (EmbeddedHookBase) needs for this
// shared virtual destructor, emitted once ~NpPulleyJoint() is defined.
// phys_fn_004509 (0x000b1160, 55 B)
NpPulleyJoint::~NpPulleyJoint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the SDK allocator, slot +0x14).
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/PulleyJoint.h. Not an oracle row; see the declaration.
NxJoint* nxPulleyJointAttachScene(PulleyJoint* internal, void* writeLink, void* readLink)
	{
	NpPulleyJoint* np = static_cast<NpPulleyJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
