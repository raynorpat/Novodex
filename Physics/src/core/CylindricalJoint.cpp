/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/CylindricalJoint.h"
#include "core/NpCylindricalJoint.h"
#include "core/JointSupport.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"
#include "NxUtilities.h"

#include <math.h>
#include <new>

// Joint-families Task 3b scaffold. See units/joint-families-contract.md
// "## Cylindrical".

// phys_fn_004320 (0x000a76a0, 87 B)
// (unimplemented)
CylindricalJoint::CylindricalJoint(const NxCylindricalJointDesc& desc)
	: Joint(desc, 0x100)
	{
	NX_ASSERT(0);
	}

// phys_fn_004322 (0x000a7700, 56 B)
// (unimplemented)
CylindricalJoint::~CylindricalJoint()
	{
	NX_ASSERT(0);
	}

// Slot 4: phys_fn_004318, written as Joint::row004318 below.
void CylindricalJoint::row_slot4(NxDebugRenderable& renderable)
	{
	row004318(renderable);
	}

// phys_fn_004326 (0x000a7810, 5377 B)
// (unimplemented)
void CylindricalJoint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004324 (0x000a7740, 194 B)
// (unimplemented)
void CylindricalJoint::loadFromDesc(const NxCylindricalJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004316 (0x000a7200, 54 B)
// (unimplemented)
void CylindricalJoint::saveToDesc(NxCylindricalJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004318 (0x000a7240, 1115 B)
// (unimplemented)
void Joint::row004318(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	}
