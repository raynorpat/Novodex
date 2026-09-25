/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpFixedJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3h. The rows below are the prismatic shape
// (core/NpPrismaticJoint.cpp): the NxJoint setters call NpJointShared's
// shared forward* bodies with this unit's __FILE__ and line; the 13 folded
// NxJoint bodies live in core/NpJointShared.cpp (none of them sits inside
// this unit's address range). Every write-locked NxJoint row reports line
// 0x10; loadFromDesc 0x14 and saveToDesc 0x1f, as in core/NpPulleyJoint.cpp.
// The constructor / thunk / destructor triple (004561-004565) sits after the
// unit's evidenced span, in the gap work_units.json calls
// `gap:core\NpFixedJoint.cpp..core\NpPointInPlaneJoint.cpp`: 004250 is its
// only caller and it installs the fixed tables. See
// units/joint-families-contract.md "## Fixed".

// The oracle's __FILE__ for this unit (the string at 0x1011abbc every
// write-lock report pushes, e.g. 0xb1693).
#define NX_NPFIXEDJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpFixedJoint.cpp"

// phys_fn_004541 (0x000b1670, 84 B)
void NpFixedJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPFIXEDJOINT_CPP, 0x10, anchor);
	}

// phys_fn_004543 (0x000b16d0, 84 B)
void NpFixedJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPFIXEDJOINT_CPP, 0x10, axis);
	}

// phys_fn_004545 (0x000b1730, 89 B)
void NpFixedJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPFIXEDJOINT_CPP, 0x10, maxForce, maxTorque);
	}

// phys_fn_004547 (0x000b1790, 89 B)
void NpFixedJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPFIXEDJOINT_CPP, 0x10, point, pointIsOnBody2);
	}

// phys_fn_004549 (0x000b17f0, 97 B)
bool NpFixedJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPFIXEDJOINT_CPP, 0x10, normal, pointInPlane);
	}

// phys_fn_004551 (0x000b1860, 74 B)
void NpFixedJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPFIXEDJOINT_CPP, 0x10);
	}

// phys_fn_004553 (0x000b18b0, 88 B)
void NpFixedJoint::setName(const char* name)
	{
	forwardSetName(NX_NPFIXEDJOINT_CPP, 0x10, name);
	}

// phys_fn_004555 (0x000b1910, 74 B)
void NpFixedJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPFIXEDJOINT_CPP, 0x10);
	}

// phys_fn_004557 (0x000b1960, 84 B)
// Internal slot 9 (FixedJoint::loadFromDesc), dispatched through
// mInternal's own vtable as the listing's `[[this+0x18]]+0x24` call
// (0xb19a5).
void NpFixedJoint::loadFromDesc(const NxFixedJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPFIXEDJOINT_CPP, 0x14, desc);
	}

// phys_fn_004559 (0x000b19c0, 84 B)
// Internal slot 10 (FixedJoint::saveToDesc, `[vt+0x28]`, 0xb1a05).
void NpFixedJoint::saveToDesc(NxFixedJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPFIXEDJOINT_CPP, 0x1f, desc);
	}

// phys_fn_004561 (0x000b1a20, 57 B)
// As phys_fn_004753 (NpPrismaticJoint): NxJoint()'s inline constructor zeroes
// userData/appData (0xb1a2b/0xb1a2e), the transient NxFixedJoint table
// (0x1011ab38), the secondary table (0x1011ac7c) and the final table
// (0x1011abf8) are installed by ordinary base/derived construction in the
// oracle's order, and the shared NpJointShared constructor zeroes the hook
// base's two words (phys_fn_002404) and stores `internal` at +0x18 and +0x08.
NpFixedJoint::NpFixedJoint(FixedJoint* internal)
	: NpJointShared<NxFixedJoint, FixedJoint>(internal)
	{
	}

// phys_fn_004563 (0x000b1a60, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpFixedJoint>") the second base (EmbeddedHookBase) needs for this
// shared virtual destructor, emitted once ~NpFixedJoint() is defined.
// phys_fn_004565 (0x000b1a70, 55 B)
NpFixedJoint::~NpFixedJoint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the Foundation allocator, slot +0x14).
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/FixedJoint.h. Not an oracle row; see the declaration.
NxJoint* nxFixedJointAttachScene(FixedJoint* internal, void* writeLink, void* readLink)
	{
	NpFixedJoint* np = static_cast<NpFixedJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
