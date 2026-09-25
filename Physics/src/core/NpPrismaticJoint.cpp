/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpPrismaticJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3a. The rows below are the revolute pilot's Np shape
// (core/NpRevoluteJoint.cpp): the NxJoint setters call NpJointShared's shared
// forward* bodies with this unit's __FILE__ and line; the 13 folded NxJoint
// bodies live in core/NpJointShared.cpp. Every write-locked NxJoint row
// reports line 0xf; loadFromDesc 0x13 and saveToDesc 0x1e.

// The oracle's __FILE__ for this unit (the string at 0x1011b47c every
// write-lock report pushes, e.g. 0xb3453).
#define NX_NPPRISMATICJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpPrismaticJoint.cpp"

// phys_fn_004753 (0x000b3810, 57 B)
// As phys_fn_004725 (NpRevoluteJoint): NxJoint()'s inline constructor zeroes
// userData/appData (0xb3818/0xb381b), the transient NxPrismaticJoint table
// (0x1011b3f8), the secondary table (0x1011b53c) and the final table
// (0x1011b4b8) are installed by ordinary base/derived construction in the
// oracle's order, and the shared NpJointShared constructor zeroes the hook
// base's two words (phys_fn_002404) and stores `internal` at +0x18 and +0x08.
NpPrismaticJoint::NpPrismaticJoint(PrismaticJoint* internal)
	: NpJointShared<NxPrismaticJoint, PrismaticJoint>(internal)
	{
	}

// phys_fn_004755 (0x000b3850, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpPrismaticJoint>") the second base (EmbeddedHookBase) needs for this
// shared virtual destructor, emitted once ~NpPrismaticJoint() is defined.
// phys_fn_004757 (0x000b3860, 55 B)
NpPrismaticJoint::~NpPrismaticJoint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the Foundation allocator, slot +0x14).
	}

// phys_fn_004731 (0x000b3430, 84 B)
void NpPrismaticJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPPRISMATICJOINT_CPP, 0xf, anchor);
	}

// phys_fn_004733 (0x000b3490, 84 B)
void NpPrismaticJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPPRISMATICJOINT_CPP, 0xf, axis);
	}

// phys_fn_004735 (0x000b34f0, 89 B)
void NpPrismaticJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPPRISMATICJOINT_CPP, 0xf, maxForce, maxTorque);
	}

// phys_fn_004737 (0x000b3550, 89 B)
void NpPrismaticJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPPRISMATICJOINT_CPP, 0xf, point, pointIsOnBody2);
	}

// phys_fn_004739 (0x000b35b0, 97 B)
bool NpPrismaticJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPPRISMATICJOINT_CPP, 0xf, normal, pointInPlane);
	}

// phys_fn_004741 (0x000b3620, 74 B)
void NpPrismaticJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPPRISMATICJOINT_CPP, 0xf);
	}

// phys_fn_004745 (0x000b36a0, 88 B)
void NpPrismaticJoint::setName(const char* name)
	{
	forwardSetName(NX_NPPRISMATICJOINT_CPP, 0xf, name);
	}

// phys_fn_004747 (0x000b3700, 74 B)
void NpPrismaticJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPPRISMATICJOINT_CPP, 0xf);
	}

// phys_fn_004749 (0x000b3750, 84 B)
// Internal slot 9 (PrismaticJoint::loadFromDesc), dispatched through
// mInternal's own vtable as the listing's `[[this+0x18]]+0x24` call.
void NpPrismaticJoint::loadFromDesc(const NxPrismaticJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPPRISMATICJOINT_CPP, 0x13, desc);
	}

// phys_fn_004751 (0x000b37b0, 84 B)
// Internal slot 10 (PrismaticJoint::saveToDesc, `[vt+0x28]`).
void NpPrismaticJoint::saveToDesc(NxPrismaticJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPPRISMATICJOINT_CPP, 0x1e, desc);
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/PrismaticJoint.h. Not an oracle row; see the declaration.
NxJoint* nxPrismaticJointAttachScene(PrismaticJoint* internal, void* writeLink, void* readLink)
	{
	NpPrismaticJoint* np = static_cast<NpPrismaticJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
