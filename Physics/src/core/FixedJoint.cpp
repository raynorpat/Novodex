/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/FixedJoint.h"
#include "core/NpFixedJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"

#include <new>

// Joint-families Task 3h scaffold: every row asserts until Step 3 writes it.
// See units/joint-families-contract.md "## Fixed".

// phys_fn_004242 (0x000a00e0, 42 B)
void FixedJoint::saveToDesc(NxFixedJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004244 (0x000a0110, 723 B)
void FixedJoint::recordRelativePose(const NxFixedJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004246 (0x000a03f0, 2922 B)
void FixedJoint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004250 (0x000a0f70, 81 B)
FixedJoint::FixedJoint(const NxFixedJointDesc& desc)
	: Joint(desc, 0x200)
	{
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004252 (0x000a0fd0, 56 B)
FixedJoint::~FixedJoint()
	{
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004254 (0x000a1010, 141 B)
void FixedJoint::loadFromDesc(const NxFixedJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}
