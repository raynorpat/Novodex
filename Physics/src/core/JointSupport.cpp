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
// "## Row assignment"). phys_fn_000022/000571/000598/000633/000758 are
// deferred stubs only: each is owned by a unit outside the joint code,
// declared here so a joint row can call through a named function instead
// of a raw address (units/joint-families-contract.md "## Shared rows").
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
// Bit 10 of mFlags clear: for each body, w = its +0x20 3x3 times the
// record's +0x18 (body 0) or +0x24 (body 1) vector and u = the record's
// +0x00 vector times its +0x0c scale, both stored as floats; out0 = body
// 0's (w . +0x18 + u . +0x00) plus body 1's, 0 without body 0. Bit 10 set:
// e = the sum of both 3x3s times +0x00 (the first component unrounded, the
// others stored), out0 = e . +0x00. Then out1 = 1 / out0, or 0 for 0.
// Listing over decompile: the six-term sums keep the listing's order
// (w.z, w.y, u.x, u.z, u.y, w.x for body 0; w.z, w.y, w.x, u.x, u.z, u.y,
// then + out0 for body 1); in the bit-10 arm e.x is summed unrounded
// across the bodies while e.y and e.z are stored first.
void JointSupportRecord::row004391(NxReal& out0, NxReal& out1)
	{
	const NxReal vx = mUnknown000.x;
	const NxReal vy = mUnknown000.y;
	const NxReal vz = mUnknown000.z;
	if(!((mFlags >> 10) & 1))
		{
		NxVec3 w0, u0, w1, u1;
		const JointSupportBody* body0 = mBody[0];
		if(body0)
			{
			const NxReal* m = body0->mUnknown020;
			const NxVec3& a = mUnknown018;
			const double c0 = (supportMul(a.z, m[2]) + supportMul(a.y, m[1])) + supportMul(a.x, m[0]);
			const double c1 = (supportMul(a.z, m[5]) + supportMul(a.y, m[4])) + supportMul(a.x, m[3]);
			const double c2 = (supportMul(a.z, m[8]) + supportMul(a.y, m[7])) + supportMul(a.x, m[6]);
			w0.z = (NxReal)c2;
			w0.x = (NxReal)c0;
			w0.y = (NxReal)c1;
			const NxReal s = body0->mUnknown00c;
			u0.x = (NxReal)supportMul(vx, s);
			u0.y = (NxReal)supportMul(vy, s);
			u0.z = (NxReal)supportMul(vz, s);
			}
		const JointSupportBody* body1 = mBody[1];
		if(body1)
			{
			const NxReal* m = body1->mUnknown020;
			const NxVec3& b = mUnknown024;
			const double c0 = (supportMul(b.z, m[2]) + supportMul(b.y, m[1])) + supportMul(b.x, m[0]);
			const double c1 = (supportMul(b.z, m[5]) + supportMul(b.y, m[4])) + supportMul(b.x, m[3]);
			const double c2 = (supportMul(b.z, m[8]) + supportMul(b.y, m[7])) + supportMul(b.x, m[6]);
			w1.z = (NxReal)c2;
			w1.x = (NxReal)c0;
			w1.y = (NxReal)c1;
			const NxReal s = body1->mUnknown00c;
			u1.x = (NxReal)supportMul(vx, s);
			u1.y = (NxReal)supportMul(vy, s);
			u1.z = (NxReal)supportMul(vz, s);
			}
		if(body0)
			{
			const NxVec3& a = mUnknown018;
			out0 = (NxReal)(((((supportMul(w0.z, a.z) + supportMul(w0.y, a.y)) + supportMul(u0.x, vx))
				+ supportMul(u0.z, vz)) + supportMul(u0.y, vy)) + supportMul(w0.x, a.x));
			}
		else
			{
			out0 = 0.0f;
			}
		// The listing reloads +0x14 here (0xaf56c).
		if(mBody[1])
			{
			const NxVec3& b = mUnknown024;
			out0 = (NxReal)((((((supportMul(w1.z, b.z) + supportMul(w1.y, b.y)) + supportMul(w1.x, b.x))
				+ supportMul(u1.x, vx)) + supportMul(u1.z, vz)) + supportMul(u1.y, vy)) + out0);
			}
		}
	else
		{
		double e0;
		NxReal e1;
		NxReal e2;
		const JointSupportBody* body0 = mBody[0];
		if(body0)
			{
			const NxReal* m = body0->mUnknown020;
			e0 = (supportMul(vz, m[2]) + supportMul(vy, m[1])) + supportMul(vx, m[0]);
			e1 = (NxReal)((supportMul(vz, m[5]) + supportMul(vy, m[4])) + supportMul(vx, m[3]));
			e2 = (NxReal)((supportMul(vz, m[8]) + supportMul(vy, m[7])) + supportMul(vx, m[6]));
			}
		else
			{
			e0 = 0.0f;
			e2 = 0.0f;
			e1 = 0.0f;
			}
		double sum0;
		double sum1;
		double sum2;
		const JointSupportBody* body1 = mBody[1];
		if(body1)
			{
			const NxReal* m = body1->mUnknown020;
			const double f0 = (supportMul(vz, m[2]) + supportMul(vy, m[1])) + supportMul(vx, m[0]);
			const NxReal f1 = (NxReal)((supportMul(vz, m[5]) + supportMul(vy, m[4])) + supportMul(vx, m[3]));
			const NxReal f2 = (NxReal)((supportMul(vz, m[8]) + supportMul(vy, m[7])) + supportMul(vx, m[6]));
			sum0 = e0 + f0;
			sum1 = (double)f1 + e1;
			sum2 = (double)f2 + e2;
			}
		else
			{
			sum0 = e0;
			sum1 = e1;
			sum2 = e2;
			}
		out0 = (NxReal)((sum0 * vx + sum1 * vy) + sum2 * vz);
		}
	// `fucompp; test ah,0x44; jnp`: only an exact zero (not a NaN) takes
	// the zero arm.
	if(out0 == 0.0f)
		out1 = 0.0f;
	else
		out1 = (NxReal)(1.0f / (double)out0);
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
void Row000022Fixture::row000022(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_000571 (0x000108e0, 22 B)
// (deferred: owner Scene.cpp, link-insert at Scene+0x620)
void Row000571Fixture::row000571(void* event)
	{
	(void)event;
	NX_ASSERT(0);
	}

// phys_fn_000598 (0x00010f50, 138 B)
// (deferred: owner Scene.cpp, grows the constraint-record array at Scene+0x5b8)
void Row000598Fixture::row000598()
	{
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
void Row000758Fixture::row000758()
	{
	NX_ASSERT(0);
	}
