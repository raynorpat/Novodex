#ifndef NX_PHYSICS_CORE_JOINTLINEARRECORDS
#define NX_PHYSICS_CORE_JOINTLINEARRECORDS
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Code the prismatic (phys_fn_004386) and cylindrical (phys_fn_004326) solver
// slots share instruction for instruction: the kind-1 linear constraint
// record, the solve tail every record site ends with, and the joint error of
// one pair of levers. Hoisted out of core/PrismaticJoint.cpp by joint-families
// Task 3b; see units/joint-families-contract.md "## Cylindrical"
// ("### Dependency closure"). Not oracle rows: the oracle inlines them at
// every site. Include only from the x87 (/arch:IA32) joint translation units,
// whose floating-point conventions these bodies follow: a value the listing
// keeps on the FPU stack is a `double`, a value it stores is an `NxReal`.

#include "Nxp.h"
#include "core/Joint.h"
#include "core/JointSupport.h"
#include "PhysicsSDK.h"

static NX_INLINE double jointLinearMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

// SDK parameter 0 (NX_PENALTY_FORCE), which the record tail reads straight
// from the live parameter array (.data 0x10123b18; 0xae56f, 0xa84ef). Read
// through PhysicsSDK::getParameter as the revolute rows do
// (revolute-contract.md open issue 8): 0 with no SDK, a window in which no
// joint exists.
static NX_INLINE NxReal jointLinearSdkParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// One linear record (prismatic 0xae418-0xae520, cylindrical 0xa83a3-0xa849a,
// and their copies): the two body records, the tangent t at +0x00, then
// r0 x t... written as the components y, z, x of t x r at +0x18 (r0) and
// +0x24 (r1), each one product minus another, stored; then kind 1 in bits
// 0-4 ((flags & 0xffffffe1) | 1), bit 9 = (kind is 0 or 2), bit 10 = (kind is
// 3, 2 or 5), bits 5-8 and 11-18 cleared. The kind is known, but the listing
// tests it (the supplement decompiles drop those arms as unreachable), so the
// tests stay.
static NX_INLINE void jointLinearRecord(JointSupportRecord* record, JointSupportBody* body0, JointSupportBody* body1,
	const NxVec3& t, const NxVec3& r0, const NxVec3& r1)
	{
	record->mBody[0] = body0;
	record->mBody[1] = body1;
	record->mUnknown000 = t;
	record->mUnknown018.y = (NxReal)(jointLinearMul(t.x, r0.z) - jointLinearMul(t.z, r0.x));
	record->mUnknown018.z = (NxReal)(jointLinearMul(t.y, r0.x) - jointLinearMul(t.x, r0.y));
	record->mUnknown018.x = (NxReal)(jointLinearMul(t.z, r0.y) - jointLinearMul(t.y, r0.z));
	record->mUnknown024.y = (NxReal)(jointLinearMul(t.x, r1.z) - jointLinearMul(t.z, r1.x));
	record->mUnknown024.z = (NxReal)(jointLinearMul(t.y, r1.x) - jointLinearMul(t.x, r1.y));
	record->mUnknown024.x = (NxReal)(jointLinearMul(t.z, r1.y) - jointLinearMul(t.y, r1.z));

	NxU32 flags = (record->mFlags & 0xffffffe1) | 1;
	record->mFlags = flags;
	NxU32 kind = flags & 0x1f;
	const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
	flags = (((bit9 << 9) ^ flags) & 0x200) ^ flags;
	record->mFlags = flags;
	kind = flags & 0x1f;
	const NxU32 bit10 = (kind == 3 || kind == 2 || kind == 5) ? 1 : 0;
	record->mFlags = ((bit10 & 1) << 10) | (flags & 0xfff8021f);
	}

// The tail every phys_fn_004391 site shares (prismatic 0xae519-0xae577,
// cylindrical 0xa84a3-0xa84f7, and their copies; the same instructions as
// revolute's revoluteSolveRecord): fill +0x34..+0x4c, solve with 004391 into
// +0x40, copy that to +0x3c, then scale +0x40 by kind: 0 or 2 by SDK
// parameter 0, 1 or 3 by 0.7f (0x3f333333 at 0x10106940). The listing passes
// as 004391's first output the local that held +0x48 (maxForce or
// maxTorque), which is dead afterwards; `unused` stands for it.
static NX_INLINE void jointSolveRecord(JointSupportRecord* record, Joint* joint, NxReal value034, NxReal value048)
	{
	record->mUnknown048 = value048;
	record->mUnknown034 = value034;
	record->mUnknown038 = 0.0f;
	record->mUnknown044 = 0;
	record->mUnknown04c = 0;
	record->mUnknown030 = joint;
	NxReal unused = value048;
	record->row004391(unused, record->mUnknown040);
	record->mUnknown03c = record->mUnknown040;
	const NxU32 kind = record->mFlags & 0x1f;
	if(kind == 0 || kind == 2)
		record->mUnknown040 = (NxReal)((double)jointLinearSdkParameter(NX_PENALTY_FORCE) * record->mUnknown040);
	else if(kind == 1 || kind == 3)
		record->mUnknown040 = (NxReal)((double)record->mUnknown040 * 0.7f);
	}

// The joint error for one pair of levers (prismatic 0xae31a-0xae3a4 and
// 0xae9dc-0xaea48, cylindrical 0xa829b-0xa832b and 0xa8954-0xa89c6):
// d = r0 - r1, each component stored; with body 0 the stored floats plus
// body 0's +0x158, without it the unrounded x difference and the stored y
// and z; then minus body 1's +0x158 when body 1 is there. The three stay on
// the stack.
static NX_INLINE void jointLinearError(const NxVec3& r0, const NxVec3& r1, const JointBodyRecord* body0,
	const JointBodyRecord* body1, double& gx, double& gy, double& gz)
	{
	const double dx = (double)r0.x - r1.x;
	const NxReal dxF = (NxReal)dx;
	const NxReal dyF = (NxReal)((double)r0.y - r1.y);
	const NxReal dzF = (NxReal)((double)r0.z - r1.z);
	if(body0)
		{
		gx = (double)dxF + body0->mUnknown158.x;
		gy = (double)dyF + body0->mUnknown158.y;
		gz = (double)dzF + body0->mUnknown158.z;
		}
	else
		{
		gx = dx;
		gy = dyF;
		gz = dzF;
		}
	if(body1)
		{
		gx = gx - body1->mUnknown158.x;
		gy = gy - body1->mUnknown158.y;
		gz = gz - body1->mUnknown158.z;
		}
	}

#endif
