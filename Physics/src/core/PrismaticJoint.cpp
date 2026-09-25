/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/PrismaticJoint.h"
#include "core/NpPrismaticJoint.h"
#include "core/JointSupport.h"

#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x1011a504).
#define NX_PRISMATICJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\PrismaticJoint.cpp"

// phys_fn_004380 (0x000ad6e0, 81 B)
PrismaticJoint::PrismaticJoint(const NxPrismaticJointDesc& desc)
	: Joint(desc, 0x80)
	{
	// (unimplemented)
	NX_ASSERT(0);
	}

// phys_fn_004382 (0x000ad740, 56 B)
PrismaticJoint::~PrismaticJoint()
	{
	// (unimplemented)
	NX_ASSERT(0);
	}

// Slot 4: the folded row phys_fn_004318 (0x000a7240, core\CylindricalJoint.cpp).
// (deferred: Task 3b writes the cylindrical debug-visualization row once for both families)
void PrismaticJoint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	}

// phys_fn_004386 (0x000ad850, 6772 B)
void PrismaticJoint::row_slot6(NxReal arg)
	{
	// (unimplemented)
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004384 (0x000ad780, 202 B)
void PrismaticJoint::loadFromDesc(const NxPrismaticJointDesc& desc)
	{
	// (unimplemented)
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004376 (0x000ad4e0, 54 B)
void PrismaticJoint::saveToDesc(NxPrismaticJointDesc& desc)
	{
	// (unimplemented)
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004378 (0x000ad520, 443 B)
void PrismaticJoint::row004378(const NxPrismaticJointDesc& desc)
	{
	// (unimplemented)
	(void)desc;
	NX_ASSERT(0);
	}
