/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/PulleyJoint.h"
#include "core/NpPulleyJoint.h"
#include "core/JointSupport.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>

// Joint-families Task 3g scaffold: every row asserts until Step 3 writes it.
// See units/joint-families-contract.md "## Pulley".

// phys_fn_004214 (0x0009e3c0, 11 B)
void PulleyJoint::row_slot1()
	{
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004216 (0x0009e3d0, 155 B)
void PulleyJoint::saveToDesc(NxPulleyJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004218 (0x0009e470, 112 B)
void PulleyJoint::copyFamilyFields(const NxPulleyJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004219 (0x0009e4e0, 805 B)
void PulleyJoint::row_slot0(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004221 (0x0009e810, 592 B)
void PulleyJoint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004222 (0x0009ea60, 81 B)
PulleyJoint::PulleyJoint(const NxPulleyJointDesc& desc)
	: Joint(desc, 0x1000)
	{
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004224 (0x0009eac0, 56 B)
PulleyJoint::~PulleyJoint()
	{
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004226 (0x0009eb00, 141 B)
void PulleyJoint::loadFromDesc(const NxPulleyJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004228 (0x0009eb90, 1572 B)
void PulleyJoint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	// (unimplemented)
	}
