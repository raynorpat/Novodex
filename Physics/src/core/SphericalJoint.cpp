/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/SphericalJoint.h"
#include "core/NpSphericalJoint.h"
#include "core/JointSupport.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x10119e64).
#define NX_SPHERICALJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\SphericalJoint.cpp"

// Joint-families Task 3c scaffold: every row asserts until its body is
// written. See units/joint-families-contract.md "## Spherical".

// phys_fn_004282 (0x000a2bf0, 21 B)
// (unimplemented)
void SphericalJoint::row_slot1()
	{
	NX_ASSERT(0);
	}

// phys_fn_004284 (0x000a2c10, 427 B)
// (unimplemented)
void SphericalJoint::row004284(const NxSphericalJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004286 (0x000a2dc0, 283 B)
// (unimplemented)
void SphericalJoint::saveToDesc(NxSphericalJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004288 (0x000a2ee0, 65 B)
// (unimplemented)
void SphericalJoint::setFlags(NxU32 flags)
	{
	(void)flags;
	NX_ASSERT(0);
	}

// phys_fn_004290 (0x000a2f30, 7 B)
// (unimplemented)
NxU32 SphericalJoint::getFlags() const
	{
	NX_ASSERT(0);
	return 0;
	}

// phys_fn_004292 (0x000a2f40, 62 B)
// (unimplemented)
void SphericalJoint::setProjectionMode(NxJointProjectionMode mode)
	{
	(void)mode;
	NX_ASSERT(0);
	}

// phys_fn_004294 (0x000a2f80, 269 B)
// (unimplemented)
void SphericalJoint::row004294(NxVec3& out) const
	{
	(void)out;
	NX_ASSERT(0);
	}

// phys_fn_004296 (0x000a3090, 5907 B)
// (unimplemented)
void SphericalJoint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004298 (0x000a47b0, 317 B)
// (unimplemented)
void SphericalJoint::row_slot8(void* body)
	{
	(void)body;
	NX_ASSERT(0);
	}

// phys_fn_004300 (0x000a48f0, 194 B)
// (unimplemented)
SphericalJoint::SphericalJoint(const NxSphericalJointDesc& desc)
	: Joint(desc, 8)
	{
	NX_ASSERT(0);
	}

// phys_fn_004302 (0x000a49c0, 56 B)
// (unimplemented)
SphericalJoint::~SphericalJoint()
	{
	NX_ASSERT(0);
	}

// phys_fn_004304 (0x000a4a00, 186 B)
// (unimplemented)
void SphericalJoint::loadFromDesc(const NxSphericalJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004306 (0x000a4ac0, 1075 B)
// (unimplemented)
NxF64 SphericalJoint::row004306(NxVec3& halfAxis, NxReal& coneFactor)
	{
	(void)halfAxis;
	(void)coneFactor;
	NX_ASSERT(0);
	return 0.0;
	}

// phys_fn_004308 (0x000a4f00, 1107 B)
// (unimplemented)
void SphericalJoint::row_slot0(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004310 (0x000a5360, 2942 B)
// (unimplemented)
void SphericalJoint::row_slot7(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004314 (0x000a7050, 430 B)
// Not a function: the swing-limit loop and epilogue of phys_fn_004312 below.
// phys_fn_004312 (0x000a5ee0, 4461 B)
// (unimplemented)
void SphericalJoint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	}
