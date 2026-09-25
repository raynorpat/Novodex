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
// "## Row assignment"). The body-record rows below (000022, 000712, 000758,
// 000760, 000778) belong to gap units outside the joint code; they are
// written here because the joint code and the Scene's joint removal reach
// them (joint-open-items Task 2, units/joint-open-items-contract.md
// "## Scene joint rows"). 000754 and 004167 remain deferred stubs.
//
// Precision: as in core/Joint.cpp, a value the listing keeps on the x87
// stack is a `double` here and a value it stores is an `NxReal`, with the
// listing's operand grouping kept.

// .data 0x10122054 / 0x10122060 / 0x1012206c (see core/JointSupport.h).
NxVec3 gJointUnitAxis[3] =
	{
	NxVec3(1.0f, 0.0f, 0.0f),
	NxVec3(0.0f, 1.0f, 0.0f),
	NxVec3(0.0f, 0.0f, 1.0f)
	};

// .data 0x10123c1c (see core/JointSupport.h).
NxVec3 gJointZeroVector(0.0f, 0.0f, 0.0f);

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

// The body record's fields the island rows touch, by byte offset.
static NX_INLINE NxU32& supportWord(void* record, NxU32 offset)
	{
	return *reinterpret_cast<NxU32*>(static_cast<NxU8*>(record) + offset);
	}

static NX_INLINE void*& supportPointer(void* record, NxU32 offset)
	{
	return *reinterpret_cast<void**>(static_cast<NxU8*>(record) + offset);
	}

// .rdata 0x101053d4: the wake floor phys_fn_000760 compares +0x4c against,
// the same 0.39999998f (0x3ecccccc) phys_fn_004107 uses.
static const NxReal gSupportWakeFloor = 0.39999998f;

// phys_fn_000022 (0x00001840, 27 B)
// 000754 on the actor body's +0x08 record, then the +0x10 object's slot 6
// with the argument as a tail jump (0x1855), or `ret 4` when +0x10 is null.
void Row000022Fixture::row000022(NxU32 arg)
	{
	reinterpret_cast<Row000754Fixture*>(supportPointer(this, 0x08))->row000754();
	Row000022Target* target = static_cast<Row000022Target*>(supportPointer(this, 0x10));
	if(target)
		target->slot6(arg);
	}

// phys_fn_000754 (0x00017010, 1027 B)
// (deferred: owner gap SceneRaycast..CapsuleShape)
void Row000754Fixture::row000754()
	{
	NX_ASSERT(0);
	}

// phys_fn_000758 (0x00017630, 214 B)
// The record's +0x124 quaternion (x, y, z, w) to the +0x134 row-major 3x3.
// Every product is formed and doubled on the stack (`fmul; fadd st0,st0`);
// the listing spills 2yy, 2xz, 2yw, 2yz and 1 - 2xx to floats and reuses the
// spilled copies, so those are NxReal here and the rest are double. The
// constant 1 is the float at 0x101041ec.
void Row000758Fixture::row000758()
	{
	NxReal* q = reinterpret_cast<NxReal*>(static_cast<NxU8*>(static_cast<void*>(this)) + 0x124);
	NxReal* m = reinterpret_cast<NxReal*>(static_cast<NxU8*>(static_cast<void*>(this)) + 0x134);
	const double w = q[3];
	const double x = q[0];
	const double y = q[1];
	const double z = q[2];
	const double yy = y * y;
	const NxReal yy2 = (NxReal)(yy + yy);
	const double zz = z * z;
	const double zz2 = zz + zz;
	m[0] = (NxReal)((1.0f - (double)yy2) - zz2);
	const double xy = y * x;
	const double xy2 = xy + xy;
	const double zw = z * w;
	const double zw2 = zw + zw;
	m[1] = (NxReal)(xy2 - zw2);
	const double xz = z * x;
	const NxReal xz2 = (NxReal)(xz + xz);
	const double yw = y * w;
	const double yw2 = yw + yw;
	const NxReal yw2Spill = (NxReal)yw2;
	m[2] = (NxReal)(yw2 + (double)xz2);
	m[3] = (NxReal)(zw2 + xy2);
	const double xx = x * x;
	const double oneMinusXx2 = 1.0f - (xx + xx);
	const NxReal oneMinusXx2Spill = (NxReal)oneMinusXx2;
	m[4] = (NxReal)(oneMinusXx2 - zz2);
	const double yz = z * y;
	const NxReal yz2 = (NxReal)(yz + yz);
	const double xw = w * x;
	const double xw2 = xw + xw;
	m[5] = (NxReal)((double)yz2 - xw2);
	m[6] = (NxReal)((double)xz2 - yw2Spill);
	m[7] = (NxReal)(xw2 + yz2);
	m[8] = (NxReal)((double)oneMinusXx2Spill - yy2);
	}

// phys_fn_000712 (0x00015d30, 32 B)
// The listing stores the recursive result back and reloads +0x1bc for the
// return value.
Row000712Fixture* Row000712Fixture::row000712()
	{
	Row000712Fixture* parent = static_cast<Row000712Fixture*>(supportPointer(this, 0x1bc));
	if(this != parent)
		supportPointer(this, 0x1bc) = parent->row000712();
	return static_cast<Row000712Fixture*>(supportPointer(this, 0x1bc));
	}

// phys_fn_000760 (0x00017710, 168 B)
// Only a record that is its own root frees its island object (+0x1e0,
// through 004167 and the SDK allocator's slot +0x14). The flags word +0x114
// is read before the stores (0x1774c); bit 8 suppresses the wake raise,
// which is `fcomp [0x101053d4]; test ah,5; jp`: only an ordered +0x4c below
// the floor is raised.
void Row000760Fixture::row000760()
	{
	if(supportPointer(this, 0x1bc) == this)
		{
		void* island = supportPointer(this, 0x1e0);
		if(island)
			{
			reinterpret_cast<Row004167Fixture*>(island)->row004167();
			nxGetSdkAllocator()->free(island);
			supportPointer(this, 0x1e0) = 0;
			}
		}
	const NxU32 bits = supportWord(this, 0x1e4) & ~2u;
	const NxU32 flags = supportWord(this, 0x114);
	supportPointer(this, 0x1bc) = this;
	supportWord(this, 0x1c0) = 0;
	supportWord(this, 0x1d0) = 0;
	supportPointer(this, 0x1d4) = this;
	supportWord(this, 0x1c4) = 0;
	supportWord(this, 0x1c8) = 1;
	supportWord(this, 0x1cc) = 0x4b7afafa;
	supportWord(this, 0x1d8) = 0;
	supportWord(this, 0x1dc) = 0;
	supportWord(this, 0x1e4) = bits;
	if(!(flags & 0x100))
		{
		NxReal& wake = *reinterpret_cast<NxReal*>(static_cast<NxU8*>(static_cast<void*>(this)) + 0x4c);
		if(wake < gSupportWakeFloor)
			supportWord(this, 0x4c) = 0x3ecccccc;
		}
	}

// phys_fn_000778 (0x000185f0, 58 B)
// With its continuation phys_fn_000780 (0x00018630, 243 B), which carries
// the loop and the `ret 8`. The root is refreshed through 000712 on the
// parent (0x185fd) only when the record is not its own root. For each
// island body: every joint on its +0x1d8 list except `joint` is pushed onto
// the array (the 000661 push, inlined at 0x1864c-0x186e6); every link word
// (+0x34) is cleared, `joint`'s included; then the next body (+0x1d0) is
// read before 000760 resets this one.
void Row000778Fixture::row000778(void* joint, void** jointArray)
	{
	void* parent = supportPointer(this, 0x1bc);
	if(this != parent)
		supportPointer(this, 0x1bc) = static_cast<Row000712Fixture*>(parent)->row000712();
	void* body = supportPointer(this, 0x1bc);
	while(body)
		{
		void* linked = supportPointer(body, 0x1d8);
		while(linked)
			{
			if(joint != linked)
				nxJointPointerArrayPush(jointArray, linked);
			void* next = supportPointer(linked, 0x34);
			supportPointer(linked, 0x34) = 0;
			linked = next;
			}
		void* nextBody = supportPointer(body, 0x1d0);
		static_cast<Row000760Fixture*>(body)->row000760();
		body = nextBody;
		}
	}

// phys_fn_004167 (0x0009ad10, 156 B)
// (deferred: owner gap Joint.cpp..D6Joint.cpp)
void Row004167Fixture::row004167()
	{
	NX_ASSERT(0);
	}
