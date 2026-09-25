/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/D6Joint.h"
#include "core/NpD6Joint.h"
#include "core/JointSupport.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"

#include <new>
#include <cstdio>

// Joint-families Task 3i scaffold: every row asserts until Step 3 writes it.
// See units/joint-families-contract.md "## D6".

static void d6PrintPose(FILE* stream, const char* name, const D6JointPose* pose);
static void d6Dump(const D6JointPose* pose0, const D6JointPose* pose1, const D6JointPose* pose2,
	const NxReal* jwq, NxReal fps);
static void d6QuaternionRateMatrix(NxReal* out, const D6JointPose* a, const D6JointPose* b);

// phys_fn_004178 (0x0009b270, 396 B)
void D6JointPose::row004178(D6JointPose& out, const D6JointPose& other) const
	{
	(void)out;
	(void)other;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004180 (0x0009b400, 330 B)
D6JointPose* D6JointPose::row004180(D6JointPose& out) const
	{
	NX_ASSERT(0);
	// (unimplemented)
	return &out;
	}

// phys_fn_004182 (0x0009b550, 57 B)
void D6Joint::saveToDesc(NxD6JointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004184 (0x0009b590, 62 B)
void D6Joint::setProjectionMode(NxJointProjectionMode mode)
	{
	(void)mode;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004188 (0x0009b5e0, 181 B)
void D6JointDumpRecord::print(FILE* stream) const
	{
	(void)stream;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004190 (0x0009b6a0, 98 B)
static void d6PrintPose(FILE* stream, const char* name, const D6JointPose* pose)
	{
	(void)stream;
	(void)name;
	(void)pose;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004192 (0x0009b710, 328 B)
static void d6Dump(const D6JointPose* pose0, const D6JointPose* pose1, const D6JointPose* pose2,
	const NxReal* jwq, NxReal fps)
	{
	(void)pose0;
	(void)pose1;
	(void)pose2;
	(void)jwq;
	(void)fps;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004194 (0x0009b860, 514 B)
void D6Joint::row004194(NxU32 limit, const NxVec3& ra, const NxVec3& rb, const NxVec3& normal, NxReal bias,
	NxReal maxForce)
	{
	(void)limit;
	(void)ra;
	(void)rb;
	(void)normal;
	(void)bias;
	(void)maxForce;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004196 (0x0009ba70, 349 B)
void D6Joint::row004196(NxU32 limit, const NxVec3& axis, NxReal bias, NxReal maxForce)
	{
	(void)limit;
	(void)axis;
	(void)bias;
	(void)maxForce;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004198 (0x0009bbd0, 1164 B)
static void d6QuaternionRateMatrix(NxReal* out, const D6JointPose* a, const D6JointPose* b)
	{
	(void)out;
	(void)a;
	(void)b;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004200 (0x0009c060, 2098 B)
void D6Joint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004202 (0x0009c8a0, 56 B)
D6Joint::~D6Joint()
	{
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004204 (0x0009c8e0, 698 B)
void D6Joint::row004204(const NxD6JointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004206 (0x0009cba0, 3200 B)
void D6Joint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004207 (0x0009d820, 2394 B)
void D6Joint::row_slot8(void* body)
	{
	(void)body;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004210 (0x0009e1a0, 331 B)
D6Joint::D6Joint(const NxD6JointDesc& desc)
	: Joint(desc, 0x4000)
	{
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004212 (0x0009e2f0, 194 B)
void D6Joint::loadFromDesc(const NxD6JointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	// (unimplemented)
	}
