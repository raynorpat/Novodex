/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/JointSupport.h"
#include "PhysicsInternal.h"

// Rows phys_fn_004389/004391/004393 are not Joint or RevoluteJoint members: they
// run on JointSupportRecord (see core/JointSupport.h and revolute-contract.md
// "## Row assignment"). phys_fn_000022/000571/000633/000758 are deferred
// stubs only: each is owned by a unit outside the pilot, declared here so a
// pilot row can call through a named function instead of a raw address.
//
// Precision: as in core/Joint.cpp, a value the listing keeps on the x87
// stack is a `double` here and a value it stores is an `NxReal`, with the
// listing's operand grouping kept.

static NX_INLINE double supportMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

// phys_fn_004389 (0x000af2d0, 227 B)
// Bit 10 of mFlags clear: the dot of the record's vectors with body[0]'s
// (+0x00, +0x10) minus the same for body[1], each missing body counting 0.
// Bit 10 set: the record's +0x00 vector dotted with body[0]'s +0x10 minus
// body[1]'s +0x10 (either missing: that side alone, the listing does not
// test body[0] when body[1] is null).
NxF64 JointSupportRecord::row004389() const
	{
	if(mFlags & 0x400)
		{
		const JointSupportBody* body1 = mBody[1];
		if(!body1)
			{
			const JointSupportBody* body0 = mBody[0];
			return (supportMul(body0->mUnknown010.z, mUnknown000.z) + supportMul(body0->mUnknown010.y, mUnknown000.y))
				+ supportMul(body0->mUnknown010.x, mUnknown000.x);
			}
		const JointSupportBody* body0 = mBody[0];
		if(!body0)
			{
			return -((supportMul(body1->mUnknown010.z, mUnknown000.z) + supportMul(body1->mUnknown010.y, mUnknown000.y))
				+ supportMul(body1->mUnknown010.x, mUnknown000.x));
			}
		const double dx = (double)body0->mUnknown010.x - body1->mUnknown010.x;
		const double dy = (double)body0->mUnknown010.y - body1->mUnknown010.y;
		const double dz = (double)body0->mUnknown010.z - body1->mUnknown010.z;
		return (dz * mUnknown000.z + dy * mUnknown000.y) + dx * mUnknown000.x;
		}

	double first;
	const JointSupportBody* body0 = mBody[0];
	if(body0)
		{
		first = ((((supportMul(body0->mUnknown010.z, mUnknown018.z) + supportMul(body0->mUnknown010.y, mUnknown018.y))
			+ supportMul(body0->mUnknown000.z, mUnknown000.z))
			+ supportMul(body0->mUnknown000.y, mUnknown000.y))
			+ supportMul(body0->mUnknown000.x, mUnknown000.x))
			+ supportMul(body0->mUnknown010.x, mUnknown018.x);
		}
	else
		{
		first = 0.0f;
		}
	const JointSupportBody* body1 = mBody[1];
	if(!body1)
		return first;
	const double second = ((((supportMul(body1->mUnknown010.z, mUnknown024.z) + supportMul(body1->mUnknown010.y, mUnknown024.y))
		+ supportMul(body1->mUnknown000.z, mUnknown000.z))
		+ supportMul(body1->mUnknown000.y, mUnknown000.y))
		+ supportMul(mUnknown000.x, body1->mUnknown000.x))
		+ supportMul(mUnknown024.x, body1->mUnknown010.x);
	return first - second;
	}

// phys_fn_004391 (0x000af3c0, 837 B)
// (deferred: solver slots 6/7)
void JointSupportRecord::row004391(NxReal& out0, NxReal& out1)
	{
	(void)out0;
	(void)out1;
	NX_ASSERT(0);
	}

// phys_fn_004393 (0x000af710, 122 B)
// Both tests are `fcomp 0; test ah,1; jne`: the branch is taken only for an
// ordered argument >= 0. phys_fn_004391's first output is never read.
void JointSupportRecord::row004393(NxReal arg0, NxReal arg1)
	{
	NxReal unused;
	NxReal value;
	row004391(unused, value);
	mUnknown03c = value;
	mUnknown040 = value;
	if(arg1 >= 0.0f)
		mUnknown040 = (NxReal)supportMul(value, arg1);
	if(arg0 >= 0.0f)
		{
		const double scale = 1.0 / (supportMul(value, arg0) + 1.0);
		mUnknown040 = (NxReal)(scale * mUnknown040);
		mUnknown03c = (NxReal)(scale * value);
		}
	}

// phys_fn_000022 (0x00001840, 27 B)
// (deferred: owner gap <start>..Actor.cpp)
void row000022()
	{
	NX_ASSERT(0);
	}

// phys_fn_000571 (0x000108e0, 22 B)
// (deferred: owner Scene.cpp, link-insert at Scene+0x620)
void Row000571Fixture::row000571(void* event)
	{
	(void)event;
	NX_ASSERT(0);
	}

// phys_fn_000633 (0x00012660, 370 B)
// (deferred: owner Scene.cpp, joint removal)
void Row000633Fixture::row000633(void* joint)
	{
	(void)joint;
	NX_ASSERT(0);
	}

// phys_fn_000758 (0x00017630, 214 B)
// (deferred: owner gap SceneRaycast..CapsuleShape)
void row000758()
	{
	NX_ASSERT(0);
	}
