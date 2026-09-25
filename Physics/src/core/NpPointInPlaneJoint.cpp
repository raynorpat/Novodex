/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpPointInPlaneJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3e. The rows below are the prismatic shape
// (core/NpPrismaticJoint.cpp): the NxJoint setters call NpJointShared's
// shared forward* bodies with this unit's __FILE__ and line; the 13 folded
// NxJoint bodies live in core/NpJointShared.cpp (two of them, 004573 and
// 004577, sit inside this unit's address range). Every write-locked NxJoint
// row reports line 0xf; loadFromDesc 0x13 and saveToDesc 0x1e. The unit ends
// with its own constructor / thunk / destructor triple (004591-004595); the
// gap before it (004561-004565) is the fixed family's (see
// units/joint-families-contract.md "## PointInPlane").

// The oracle's __FILE__ for this unit (the string at 0x1011ad1c every
// write-lock report pushes, e.g. 0xb1ad3).
#define NX_NPPOINTINPLANEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpPointInPlaneJoint.cpp"

// phys_fn_004567 (0x000b1ab0, 84 B)
void NpPointInPlaneJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPPOINTINPLANEJOINT_CPP, 0xf, anchor);
	}

// phys_fn_004569 (0x000b1b10, 84 B)
void NpPointInPlaneJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPPOINTINPLANEJOINT_CPP, 0xf, axis);
	}

// phys_fn_004571 (0x000b1b70, 89 B)
void NpPointInPlaneJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPPOINTINPLANEJOINT_CPP, 0xf, maxForce, maxTorque);
	}

// phys_fn_004575 (0x000b1c00, 89 B)
void NpPointInPlaneJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPPOINTINPLANEJOINT_CPP, 0xf, point, pointIsOnBody2);
	}

// phys_fn_004579 (0x000b1c90, 97 B)
bool NpPointInPlaneJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPPOINTINPLANEJOINT_CPP, 0xf, normal, pointInPlane);
	}

// phys_fn_004581 (0x000b1d00, 74 B)
void NpPointInPlaneJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPPOINTINPLANEJOINT_CPP, 0xf);
	}

// phys_fn_004583 (0x000b1d50, 88 B)
void NpPointInPlaneJoint::setName(const char* name)
	{
	forwardSetName(NX_NPPOINTINPLANEJOINT_CPP, 0xf, name);
	}

// phys_fn_004585 (0x000b1db0, 74 B)
void NpPointInPlaneJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPPOINTINPLANEJOINT_CPP, 0xf);
	}

// phys_fn_004587 (0x000b1e00, 84 B)
// Internal slot 9 (PointInPlaneJoint::loadFromDesc), dispatched through
// mInternal's own vtable as the listing's `[[this+0x18]]+0x24` call
// (0xb1e45).
void NpPointInPlaneJoint::loadFromDesc(const NxPointInPlaneJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPPOINTINPLANEJOINT_CPP, 0x13, desc);
	}

// phys_fn_004589 (0x000b1e60, 84 B)
// Internal slot 10 (PointInPlaneJoint::saveToDesc, `[vt+0x28]`, 0xb1ea5).
void NpPointInPlaneJoint::saveToDesc(NxPointInPlaneJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPPOINTINPLANEJOINT_CPP, 0x1e, desc);
	}

// phys_fn_004591 (0x000b1ec0, 57 B)
// As phys_fn_004753 (NpPrismaticJoint): NxJoint()'s inline constructor zeroes
// userData/appData (0xb1ecb/0xb1ece), the transient NxPointInPlaneJoint table
// (0x1011ac98), the secondary table (0x1011addc) and the final table
// (0x1011ad58) are installed by ordinary base/derived construction in the
// oracle's order, and the shared NpJointShared constructor zeroes the hook
// base's two words (phys_fn_002404) and stores `internal` at +0x18 and +0x08.
NpPointInPlaneJoint::NpPointInPlaneJoint(PointInPlaneJoint* internal)
	: NpJointShared<NxPointInPlaneJoint, PointInPlaneJoint>(internal)
	{
	}

// phys_fn_004593 (0x000b1f00, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpPointInPlaneJoint>") the second base (EmbeddedHookBase) needs for this
// shared virtual destructor, emitted once ~NpPointInPlaneJoint() is defined.
// phys_fn_004595 (0x000b1f10, 55 B)
NpPointInPlaneJoint::~NpPointInPlaneJoint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the SDK allocator, slot +0x14).
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/PointInPlaneJoint.h. Not an oracle row; see the declaration.
NxJoint* nxPointInPlaneJointAttachScene(PointInPlaneJoint* internal, void* writeLink, void* readLink)
	{
	NpPointInPlaneJoint* np = static_cast<NpPointInPlaneJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
