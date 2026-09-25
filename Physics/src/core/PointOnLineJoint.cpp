/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/PointOnLineJoint.h"
#include "core/NpPointOnLineJoint.h"
#include "core/JointSupport.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x10119ce4).
#define NX_POINTONLINEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\PointOnLineJoint.cpp"

// Joint-families Task 3d scaffold: every row asserts until its body is
// written. See units/joint-families-contract.md "## PointOnLine".

// phys_fn_004268 (0x000a1cc0, 54 B)
// (unimplemented)
void PointOnLineJoint::saveToDesc(NxPointOnLineJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004270 (0x000a1d00, 5 B)
// (unimplemented)
PointOnLineJoint* PointOnLineJoint::row_slot11()
	{
	NX_ASSERT(0);
	return 0;
	}

// phys_fn_004272 (0x000a1d10, 2073 B)
// (unimplemented)
void PointOnLineJoint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004274 (0x000a2530, 1345 B)
// (unimplemented)
void PointOnLineJoint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	}

// phys_fn_004276 (0x000a2a80, 84 B)
// (unimplemented)
PointOnLineJoint::PointOnLineJoint(const NxPointOnLineJointDesc& desc)
	: Joint(desc, 4)
	{
	NX_ASSERT(0);
	}

// phys_fn_004278 (0x000a2ae0, 56 B)
// (unimplemented)
PointOnLineJoint::~PointOnLineJoint()
	{
	NX_ASSERT(0);
	}

// phys_fn_004280 (0x000a2b20, 194 B)
// (unimplemented)
void PointOnLineJoint::loadFromDesc(const NxPointOnLineJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}
