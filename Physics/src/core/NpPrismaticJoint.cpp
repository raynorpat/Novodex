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

// The oracle's __FILE__ for this unit (0x1011b453 and the other pushes).
#define NX_NPPRISMATICJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\NpPrismaticJoint.cpp"

// phys_fn_004753 (0x000b3810, 57 B)
NpPrismaticJoint::NpPrismaticJoint(PrismaticJoint* internal)
	: NpJointShared<NxPrismaticJoint, PrismaticJoint>(internal)
	{
	// (unimplemented)
	NX_ASSERT(0);
	}

// phys_fn_004755 (0x000b3850, 8 B)
// phys_fn_004757 (0x000b3860, 55 B)
NpPrismaticJoint::~NpPrismaticJoint()
	{
	// (unimplemented)
	NX_ASSERT(0);
	}

// phys_fn_004731 (0x000b3430, 84 B)
void NpPrismaticJoint::setGlobalAnchor(const NxVec3& anchor)
	{
	// (unimplemented)
	(void)anchor;
	NX_ASSERT(0);
	}

// phys_fn_004733 (0x000b3490, 84 B)
void NpPrismaticJoint::setGlobalAxis(const NxVec3& axis)
	{
	// (unimplemented)
	(void)axis;
	NX_ASSERT(0);
	}

// phys_fn_004735 (0x000b34f0, 89 B)
void NpPrismaticJoint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	// (unimplemented)
	(void)maxForce;
	(void)maxTorque;
	NX_ASSERT(0);
	}

// phys_fn_004737 (0x000b3550, 89 B)
void NpPrismaticJoint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	// (unimplemented)
	(void)point;
	(void)pointIsOnBody2;
	NX_ASSERT(0);
	}

// phys_fn_004739 (0x000b35b0, 97 B)
bool NpPrismaticJoint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	// (unimplemented)
	(void)normal;
	(void)pointInPlane;
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004741 (0x000b3620, 74 B)
void NpPrismaticJoint::resetLimitPlaneIterator()
	{
	// (unimplemented)
	NX_ASSERT(0);
	}

// phys_fn_004745 (0x000b36a0, 88 B)
void NpPrismaticJoint::setName(const char* name)
	{
	// (unimplemented)
	(void)name;
	NX_ASSERT(0);
	}

// phys_fn_004747 (0x000b3700, 74 B)
void NpPrismaticJoint::purgeLimitPlanes()
	{
	// (unimplemented)
	NX_ASSERT(0);
	}

// phys_fn_004749 (0x000b3750, 84 B)
void NpPrismaticJoint::loadFromDesc(const NxPrismaticJointDesc& desc)
	{
	// (unimplemented)
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004751 (0x000b37b0, 84 B)
void NpPrismaticJoint::saveToDesc(NxPrismaticJointDesc& desc)
	{
	// (unimplemented)
	(void)desc;
	NX_ASSERT(0);
	}

// Scene::createJoint's link copy (0x14509-0x14521), declared in
// core/PrismaticJoint.h. Not an oracle row; see the declaration.
NxJoint* nxPrismaticJointAttachScene(PrismaticJoint* internal, void* writeLink, void* readLink)
	{
	(void)internal;
	(void)writeLink;
	(void)readLink;
	NX_ASSERT(0);
	return 0;
	}
