/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/PointInPlaneJoint.h"
#include "core/NpPointInPlaneJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x10119b7c).
#define NX_POINTINPLANEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\PointInPlaneJoint.cpp"

// Joint-families Task 3e scaffold: every row asserts until its body is
// written. See units/joint-families-contract.md "## PointInPlane".

// phys_fn_004256 (0x000a10a0, 54 B)
// (unimplemented)
void PointInPlaneJoint::saveToDesc(NxPointInPlaneJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004258 (0x000a10e0, 1391 B)
// (unimplemented)
void PointInPlaneJoint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004260 (0x000a1650, 1273 B)
// (unimplemented)
void PointInPlaneJoint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	}

// phys_fn_004262 (0x000a1b50, 84 B)
// (unimplemented)
PointInPlaneJoint::PointInPlaneJoint(const NxPointInPlaneJointDesc& desc)
	: Joint(desc, 2)
	{
	NX_ASSERT(0);
	}

// phys_fn_004264 (0x000a1bb0, 56 B)
// (unimplemented)
PointInPlaneJoint::~PointInPlaneJoint()
	{
	NX_ASSERT(0);
	}

// phys_fn_004266 (0x000a1bf0, 194 B)
// (unimplemented)
void PointInPlaneJoint::loadFromDesc(const NxPointInPlaneJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}
