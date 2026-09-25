/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpPointOnLineJoint.h"
#include "core/Joint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// Joint-families Task 3d. The rows below are the prismatic shape
// (core/NpPrismaticJoint.cpp): the NxJoint setters call NpJointShared's
// shared forward* bodies with this unit's __FILE__ and line; the 13 folded
// NxJoint bodies live in core/NpJointShared.cpp. Every write-locked NxJoint
// row reports line 0x10; loadFromDesc 0x14 and saveToDesc 0x1f. The
// constructor / thunk / destructor triple (004617-004621) is the
// `gap:core\NpPointOnLineJoint.cpp..core\NpSphericalJoint.cpp` unit, which
// is this unit's tail (see units/joint-families-contract.md "## PointOnLine").

// The oracle's __FILE__ for this unit (the string at 0x1011ae7c every
// write-lock report pushes, e.g. 0xb1f73).
#define NX_NPPOINTONLINEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpPointOnLineJoint.cpp"

// phys_fn_004597 (0x000b1f50, 84 B)
void NpPointOnLineJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	forwardSetGlobalAnchor(NX_NPPOINTONLINEJOINT_CPP, 0x10, anchor);
	}

// phys_fn_004599 (0x000b1fb0, 84 B)
void NpPointOnLineJoint::setGlobalAxis(const NxVec3& axis)
	{
	forwardSetGlobalAxis(NX_NPPOINTONLINEJOINT_CPP, 0x10, axis);
	}

// phys_fn_004601 (0x000b2010, 89 B)
void NpPointOnLineJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	forwardSetBreakable(NX_NPPOINTONLINEJOINT_CPP, 0x10, maxForce, maxTorque);
	}

// phys_fn_004603 (0x000b2070, 89 B)
void NpPointOnLineJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	forwardSetLimitPoint(NX_NPPOINTONLINEJOINT_CPP, 0x10, point, pointIsOnBody2);
	}

// phys_fn_004605 (0x000b20d0, 97 B)
bool NpPointOnLineJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	return forwardAddLimitPlane(NX_NPPOINTONLINEJOINT_CPP, 0x10, normal, pointInPlane);
	}

// phys_fn_004607 (0x000b2140, 74 B)
void NpPointOnLineJoint::resetLimitPlaneIterator()
	{
	forwardResetLimitPlaneIterator(NX_NPPOINTONLINEJOINT_CPP, 0x10);
	}

// phys_fn_004609 (0x000b2190, 88 B)
void NpPointOnLineJoint::setName(const char* name)
	{
	forwardSetName(NX_NPPOINTONLINEJOINT_CPP, 0x10, name);
	}

// phys_fn_004611 (0x000b21f0, 74 B)
void NpPointOnLineJoint::purgeLimitPlanes()
	{
	forwardPurgeLimitPlanes(NX_NPPOINTONLINEJOINT_CPP, 0x10);
	}

// phys_fn_004613 (0x000b2240, 84 B)
// Internal slot 9 (PointOnLineJoint::loadFromDesc), dispatched through
// mInternal's own vtable as the listing's `[[this+0x18]]+0x24` call
// (0xb2285).
void NpPointOnLineJoint::loadFromDesc(const NxPointOnLineJointDesc& desc)
	{
	forwardLoadFromDesc(NX_NPPOINTONLINEJOINT_CPP, 0x14, desc);
	}

// phys_fn_004615 (0x000b22a0, 84 B)
// Internal slot 10 (PointOnLineJoint::saveToDesc, `[vt+0x28]`, 0xb22e5).
void NpPointOnLineJoint::saveToDesc(NxPointOnLineJointDesc& desc)
	{
	forwardSaveToDesc(NX_NPPOINTONLINEJOINT_CPP, 0x1f, desc);
	}

// phys_fn_004617 (0x000b2300, 57 B)
// As phys_fn_004753 (NpPrismaticJoint): NxJoint()'s inline constructor zeroes
// userData/appData (0xb230b/0xb230e), the transient NxPointOnLineJoint table
// (0x1011adf8), the secondary table (0x1011af3c) and the final table
// (0x1011aeb8) are installed by ordinary base/derived construction in the
// oracle's order, and the shared NpJointShared constructor zeroes the hook
// base's two words (phys_fn_002404) and stores `internal` at +0x18 and +0x08.
NpPointOnLineJoint::NpPointOnLineJoint(PointOnLineJoint* internal)
	: NpJointShared<NxPointOnLineJoint, PointOnLineJoint>(internal)
	{
	}

// phys_fn_004619 (0x000b2340, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,0xc; jmp
// <~NpPointOnLineJoint>") the second base (EmbeddedHookBase) needs for this
// shared virtual destructor, emitted once ~NpPointOnLineJoint() is defined.
// phys_fn_004621 (0x000b2350, 55 B)
NpPointOnLineJoint::~NpPointOnLineJoint()
	{
	// Nothing to do in the body: the base-destruction chain reinstalls the
	// hook base's own table (phys_fn_002406) and the abstract NxJoint table
	// 0x1011a680, and the compiler's scalar deleting destructor frees through
	// NpJointShared's operator delete (the SDK allocator, slot +0x14).
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/PointOnLineJoint.h. Not an oracle row; see the declaration.
NxJoint* nxPointOnLineJointAttachScene(PointOnLineJoint* internal, void* writeLink, void* readLink)
	{
	NpPointOnLineJoint* np = static_cast<NpPointOnLineJoint*>(internal->mPublicObject);
	np->mWord04 = reinterpret_cast<NxU32>(writeLink);
	np->mWord08 = reinterpret_cast<NxU32>(readLink);
	return np;
	}
