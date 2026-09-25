/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/DistanceJoint.h"
#include "core/NpDistanceJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>

// The oracle's __FILE__ for this unit (every report in it pushes the string
// at 0x1011997c).
#define NX_DISTANCEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\DistanceJoint.cpp"

// Joint-families Task 3f scaffold: every row asserts until its body is
// written. See units/joint-families-contract.md "## Distance".

// phys_fn_004230 (0x0009f1c0, 109 B)
// (unimplemented)
void DistanceJoint::saveToDesc(NxDistanceJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004232 (0x0009f230, 564 B)
// (unimplemented)
void DistanceJoint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	}

// phys_fn_004234 (0x0009f470, 162 B)
// (unimplemented)
DistanceJoint::DistanceJoint(const NxDistanceJointDesc& desc)
	: Joint(desc, 0x2000)
	{
	NX_ASSERT(0);
	}

// phys_fn_004236 (0x0009f520, 56 B)
// (unimplemented)
DistanceJoint::~DistanceJoint()
	{
	NX_ASSERT(0);
	}

// phys_fn_004238 (0x0009f560, 188 B)
// (unimplemented)
void DistanceJoint::loadFromDesc(const NxDistanceJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004240 (0x0009f620, 2747 B)
// (unimplemented)
void DistanceJoint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}
