/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/PrismaticJoint.h"
#include "core/NpPrismaticJoint.h"
#include "core/JointSupport.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxUtilities.h"

#include <math.h>
#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x1011a504).
#define NX_PRISMATICJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\PrismaticJoint.cpp"

// Joint-families Task 3a. Floating point follows core/Joint.cpp and
// core/RevoluteJoint.cpp: this translation unit is x87 in the oracle and is
// built /arch:IA32 here; a value the listing keeps on the FPU stack is a
// `double`, a value it stores (fstp dword) is an `NxReal`, and the listing's
// operand grouping and order are kept. PrismaticJoint is constructed by
// Scene::createJoint's prismatic case (NxJointType 0). See
// units/joint-families-contract.md "## Prismatic".

static NX_INLINE double prismaticMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

static NX_INLINE JointBodyRecord* prismaticBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* prismaticActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* prismaticBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// The inlined loop 004386 opens with (0xad896-0xad8c2; the same loop as
// Joint.cpp's 004125/004129): refresh the first body whose stamp no longer
// matches, and only that one.
static void prismaticRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = prismaticBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// SDK parameter 0 (NX_PENALTY_FORCE), which the record tail reads straight
// from the live parameter array (.data 0x10123b18; 0xae56f). Read through
// PhysicsSDK::getParameter as the revolute rows do (revolute-contract.md
// open issue 8): 0 with no SDK, a window in which no joint exists.
static NxReal prismaticSdkParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// The four linear records of 004386 (0xae418-0xae520 and its three copies):
// the two body records, the tangent t at +0x00, t x r0... written as
// r0 x t at +0x18 and r1 x t at +0x24 (each component one product minus
// another, stored), then kind 1 in bits 0-4 ((flags & 0xffffffe1) | 1),
// bit 9 = (kind is 0 or 2), bit 10 = (kind is 3, 2 or 5), bits 5-8 and
// 11-18 cleared. The kind is known, but the listing tests it (the
// supplement decompile drops those arms as unreachable), so the tests stay.
static void prismaticLinearRecord(JointSupportRecord* record, JointSupportBody* body0, JointSupportBody* body1,
	const NxVec3& t, const NxVec3& r0, const NxVec3& r1)
	{
	record->mBody[0] = body0;
	record->mBody[1] = body1;
	record->mUnknown000 = t;
	record->mUnknown018.y = (NxReal)(prismaticMul(t.x, r0.z) - prismaticMul(t.z, r0.x));
	record->mUnknown018.z = (NxReal)(prismaticMul(t.y, r0.x) - prismaticMul(t.x, r0.y));
	record->mUnknown018.x = (NxReal)(prismaticMul(t.z, r0.y) - prismaticMul(t.y, r0.z));
	record->mUnknown024.y = (NxReal)(prismaticMul(t.x, r1.z) - prismaticMul(t.z, r1.x));
	record->mUnknown024.z = (NxReal)(prismaticMul(t.y, r1.x) - prismaticMul(t.x, r1.y));
	record->mUnknown024.x = (NxReal)(prismaticMul(t.z, r1.y) - prismaticMul(t.y, r1.z));

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

// The three angular records (0xaf04f-0xaf0ba and its two copies): the two
// body records, a unit axis at +0x00 (copied from gJointUnitAxis), kind 3
// ((flags & 0xffffffe3) | 3), then bit 9 = (kind is 0 or 2), bit 10 set,
// bits 5-8 and 11-18 cleared -- revolute's record bits
// (core/RevoluteJoint.cpp revoluteRecordBits). +0x18/+0x24 are not written.
static void prismaticAngularRecord(JointSupportRecord* record, JointSupportBody* body0, JointSupportBody* body1,
	const NxVec3& axis)
	{
	record->mBody[0] = body0;
	record->mBody[1] = body1;
	record->mUnknown000 = axis;
	const NxU32 flags = (record->mFlags & 0xffffffe3) | 3;
	record->mFlags = flags;
	const NxU32 kind = flags & 0x1f;
	const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
	record->mFlags = (((bit9 & 1) | 2) << 9) | (flags & 0xfff8041f);
	}

// The tail every phys_fn_004391 site of 004386 shares (0xae519-0xae577 and
// its six copies; the same instructions as revolute's revoluteSolveRecord):
// fill +0x34..+0x4c, solve with 004391 into +0x40, copy that to +0x3c, then
// scale +0x40 by kind: 0 or 2 by SDK parameter 0, 1 or 3 by 0.7f
// (0x3f333333 at 0x10106940). The listing passes as 004391's first output
// the local that held +0x48 (maxForce or maxTorque), which is dead
// afterwards; `unused` stands for it.
static void prismaticSolveRecord(JointSupportRecord* record, Joint* joint, NxReal value034, NxReal value048)
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
		record->mUnknown040 = (NxReal)((double)prismaticSdkParameter(NX_PENALTY_FORCE) * record->mUnknown040);
	else if(kind == 1 || kind == 3)
		record->mUnknown040 = (NxReal)((double)record->mUnknown040 * 0.7f);
	}

// 004386's joint error for one pair of levers (0xae31a-0xae3a4, and the
// same instructions at 0xae9dc-0xaea48): d = r0 - r1, each component stored;
// with body 0 the stored floats plus body 0's +0x158, without it the
// unrounded x difference and the stored y and z; then minus body 1's +0x158
// when body 1 is there. The three stay on the stack.
static void prismaticError(const NxVec3& r0, const NxVec3& r1, const JointBodyRecord* body0,
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

// The bias of one linear record (0xae3cb-0xae3e5 for t1, 0xae3ec-0xae408 for
// t2): ((t.z * e.z + t.y * e.y) + e.x * t.x) * inverse, stored.
static NxReal prismaticBias(const NxVec3& t, double gx, double gy, double gz, NxReal inverse)
	{
	return (NxReal)((((double)t.z * gz + (double)t.y * gy) + gx * t.x) * inverse);
	}

// phys_fn_004380 (0x000ad6e0, 81 B)
// Joint(desc, 0x80) runs first; the compiler then stores the vptr
// 0x1011a4d0 (0xad6f3). The public object is allocated through the SDK
// allocator (`push 0; push 0x1c; call [edx+8]`) and constructed only when the
// allocation succeeded, but desc.userData is written to it without a null
// check (0xad71c-0xad71f): a failed allocation faults there in the oracle,
// and does here too. 004378 gets the descriptor and ignores it.
PrismaticJoint::PrismaticJoint(const NxPrismaticJointDesc& desc)
	: Joint(desc, 0x80)
	{
	void* memory = nxGetSdkAllocator()->malloc(sizeof(NpPrismaticJoint), NX_MEMORY_PERSISTENT);
	NpPrismaticJoint* publicJoint = memory ? new(memory) NpPrismaticJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	row004378(desc);
	}

// phys_fn_004382 (0x000ad740, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x1011a4d0, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`), calls the Joint destructor body
// (row 004095) directly, and frees `this` through the SDK allocator (slot
// +0x14) when the flag's bit 0 is set (Joint::operator delete).
PrismaticJoint::~PrismaticJoint()
	{
	if(mPublicObject)
		delete static_cast<NpPrismaticJoint*>(mPublicObject);
	}

// Slot 4: the folded row phys_fn_004318 (0x000a7240, core\CylindricalJoint.cpp).
// (deferred: Task 3b writes the cylindrical debug-visualization row once for both families)
// The prismatic table 0x1011a4d0 names the cylindrical unit's body directly
// (slot 4, 0x11a4e0); it reads only Joint base fields. Not claimed here.
void PrismaticJoint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	}

// phys_fn_004386 (0x000ad850, 6772 B)
// The prismatic solver slot. After the stale-body refresh it takes the slide
// direction n (a missing body 0: world axis 0; a missing body 1: world axis 1;
// both: the normalized sum of the two rotated axes), two tangents t1, t2 of n
// from NxNormalToTangents, and levers r0/r1 twice: once from the anchors
// (records 1-2), once from points one axis-length along each body's axis
// (records 3-4). With both bodies present each lever is the body's anchor
// moved along its own axis to the point of the common line nearest a point
// C (C0 = midpoint of the anchors' projections on n, C1 = the same moved by
// one unit along n when the projections are within one unit of each other,
// else the other projection). Each pair gives two kind-1 records along t1
// and t2 with bias (t . error) / arg and +0x48 = maxForce. Then the relative
// rotation e = (conj(q0) q1) * mUnknown16c (sign fixed so e.w >= 0), its
// vector part rotated by body 0's +0x134 3x3, gives three kind-3 records
// along the unit axes with bias -2 e_i / arg and +0x48 = maxTorque.
// Listing over decompile (supplement): the supplement drops the kind tests
// as unreachable (0x100ae4d9, ...), shows every intermediate as a float and
// regroups several sums (the error biases, the second-half lever of body 1,
// the quaternion products); the listing is followed throughout, including
// which values stay on the stack (named double below). With both bodies the
// first half uses C0 for both levers and the second half C1 for both; body
// 1's anchor y is carried unrounded into the first half (0xae05e, 0xae0de,
// 0xae2a9) but read back as the stored float in the second (0xae914).
void PrismaticJoint::row_slot6(NxReal arg)
	{
	JointBodyRecord* body0 = prismaticBody(mBody[0]);
	JointSupportBody* const record0 = body0 ? body0->mUnknown204 : 0;
	JointBodyRecord* body1 = prismaticBody(mBody[1]);
	JointSupportBody* const record1 = body1 ? body1->mUnknown204 : 0;
	prismaticRefreshFirstStaleBody(*this);
	body0 = prismaticBody(mBody[0]);
	body1 = prismaticBody(mBody[1]);

	NxVec3 n;
	NxVec3 t1;
	NxVec3 t2;
	NxVec3 r0;
	NxVec3 r1;

	// Branch A (no body 0): the point one axis-length along body 1's axis.
	NxVec3 pointA;
	// Branch B (no body 1): the same along body 0's axis.
	NxVec3 pointB;
	// Branch C (both): the rotated axes, the world anchors and C1.
	NxVec3 q0;
	NxVec3 q1;
	NxVec3 p0;
	NxVec3 p1;
	NxVec3 c1;

	if(!body0)
		{
		// 0xad8d2-0xadaf3
		n = mWorldAxis[0];
		NxNormalToTangents(n, t1, t2);
		const NxReal* R = body1->mUnknown134;
		const NxVec3& a = mWorldAnchor[1];
		const NxVec3& position = body1->mUnknown158;
		const double row0 = (prismaticMul(R[2], a.z) + prismaticMul(R[1], a.y)) + prismaticMul(a.x, R[0]);
		const double row1 = (prismaticMul(R[5], a.z) + prismaticMul(R[4], a.y)) + prismaticMul(R[3], a.x);
		const NxReal row2 = (NxReal)((prismaticMul(R[8], a.z) + prismaticMul(R[7], a.y)) + prismaticMul(R[6], a.x));
		const NxReal p1x = (NxReal)(row0 + position.x);
		const double p1y = row1 + position.y;
		const double p1z = (double)row2 + position.z;
		const NxVec3& w = mWorldAxis[1];
		const double w0 = (prismaticMul(R[2], w.z) + prismaticMul(R[1], w.y)) + prismaticMul(R[0], w.x);
		const double w1 = (prismaticMul(R[5], w.z) + prismaticMul(R[3], w.x)) + prismaticMul(R[4], w.y);
		const NxReal w2 = (NxReal)((prismaticMul(R[8], w.z) + prismaticMul(R[6], w.x)) + prismaticMul(R[7], w.y));
		pointA.x = (NxReal)(w0 + p1x);
		pointA.y = (NxReal)(w1 + p1y);
		pointA.z = (NxReal)((double)w2 + p1z);

		const NxVec3& a0 = mWorldAnchor[0];
		const NxVec3& n0 = mWorldAxis[0];
		const double dx = (double)p1x - a0.x;
		const double dy = p1y - a0.y;
		const double dz = p1z - a0.z;
		const double s = (dx * n0.x + dz * n0.z) + dy * n0.y;
		const NxReal sx = (NxReal)(s * n0.x);
		const NxReal sy = (NxReal)(s * n0.y);
		const double sz = s * n0.z;
		r0.x = (NxReal)((double)sx + a0.x);
		r0.y = (NxReal)((double)sy + a0.y);
		r0.z = (NxReal)(sz + a0.z);
		r1.x = (NxReal)((double)p1x - position.x);
		r1.y = (NxReal)(p1y - position.y);
		r1.z = (NxReal)(p1z - position.z);
		}
	else if(!body1)
		{
		// 0xadb03-0xadd24
		n = mWorldAxis[1];
		NxNormalToTangents(n, t1, t2);
		const NxReal* R = body0->mUnknown134;
		const NxVec3& a = mWorldAnchor[0];
		const NxVec3& position = body0->mUnknown158;
		const double row0 = (prismaticMul(R[1], a.y) + prismaticMul(R[2], a.z)) + prismaticMul(R[0], a.x);
		const double row1 = (prismaticMul(R[4], a.y) + prismaticMul(R[3], a.x)) + prismaticMul(R[5], a.z);
		const NxReal row2 = (NxReal)((prismaticMul(R[7], a.y) + prismaticMul(R[6], a.x)) + prismaticMul(R[8], a.z));
		const NxReal p0x = (NxReal)(row0 + position.x);
		const double p0y = row1 + position.y;
		const double p0z = (double)row2 + position.z;
		const NxVec3& w = mWorldAxis[0];
		const double w0 = (prismaticMul(R[2], w.z) + prismaticMul(R[1], w.y)) + prismaticMul(w.x, R[0]);
		const double w1 = (prismaticMul(R[5], w.z) + prismaticMul(R[4], w.y)) + prismaticMul(R[3], w.x);
		const NxReal w2 = (NxReal)((prismaticMul(R[8], w.z) + prismaticMul(R[7], w.y)) + prismaticMul(R[6], w.x));
		pointB.x = (NxReal)(w0 + p0x);
		pointB.y = (NxReal)(w1 + p0y);
		pointB.z = (NxReal)((double)w2 + p0z);

		const NxVec3& a1 = mWorldAnchor[1];
		const NxVec3& n1 = mWorldAxis[1];
		const double dx = (double)p0x - a1.x;
		const double dy = p0y - a1.y;
		const double dz = p0z - a1.z;
		const double s = (dx * n1.x + dz * n1.z) + dy * n1.y;
		const NxReal sx = (NxReal)(s * n1.x);
		const NxReal sy = (NxReal)(s * n1.y);
		const double sz = s * n1.z;
		r1.x = (NxReal)((double)sx + a1.x);
		r1.y = (NxReal)((double)sy + a1.y);
		r1.z = (NxReal)(sz + a1.z);
		r0.x = (NxReal)((double)p0x - position.x);
		r0.y = (NxReal)(p0y - position.y);
		r0.z = (NxReal)(p0z - position.z);
		}
	else
		{
		// 0xadd2d-0xae310
		const NxReal* R0 = body0->mUnknown134;
		const NxReal* R1 = body1->mUnknown134;
		const NxVec3& w0 = mWorldAxis[0];
		q0.x = (NxReal)((prismaticMul(R0[1], w0.y) + prismaticMul(R0[2], w0.z)) + prismaticMul(w0.x, R0[0]));
		q0.y = (NxReal)((prismaticMul(R0[4], w0.y) + prismaticMul(R0[3], w0.x)) + prismaticMul(R0[5], w0.z));
		q0.z = (NxReal)((prismaticMul(R0[7], w0.y) + prismaticMul(R0[6], w0.x)) + prismaticMul(R0[8], w0.z));
		const NxVec3& w1 = mWorldAxis[1];
		q1.x = (NxReal)((prismaticMul(R1[1], w1.y) + prismaticMul(R1[2], w1.z)) + prismaticMul(R1[0], w1.x));
		q1.y = (NxReal)((prismaticMul(R1[4], w1.y) + prismaticMul(R1[3], w1.x)) + prismaticMul(R1[5], w1.z));
		q1.z = (NxReal)((prismaticMul(R1[7], w1.y) + prismaticMul(R1[6], w1.x)) + prismaticMul(R1[8], w1.z));

		// n = q0 + q1, normalized unless its length is 0 (0xade35-0xadebc):
		// x and z are normalized from the unrounded sums, y from its float.
		const double sx = (double)q0.x + q1.x;
		const NxReal sy = (NxReal)((double)q0.y + q1.y);
		const double sz = (double)q0.z + q1.z;
		n.x = (NxReal)sx;
		n.y = sy;
		n.z = (NxReal)sz;
		const double length = sqrt((sx * sx + sz * sz) + prismaticMul(sy, sy));
		if(length != 0.0f)
			{
			const double scale = 1.0f / length;
			n.x = (NxReal)(sx * scale);
			n.y = (NxReal)((double)sy * scale);
			n.z = (NxReal)(scale * sz);
			}
		NxNormalToTangents(n, t1, t2);

		const NxVec3& a0 = mWorldAnchor[0];
		const NxVec3& position0 = body0->mUnknown158;
		const double p0Row0 = (prismaticMul(R0[2], a0.z) + prismaticMul(R0[1], a0.y)) + prismaticMul(R0[0], a0.x);
		const double p0Row1 = (prismaticMul(R0[4], a0.y) + prismaticMul(R0[3], a0.x)) + prismaticMul(R0[5], a0.z);
		const NxReal p0Row2 = (NxReal)((prismaticMul(R0[7], a0.y) + prismaticMul(R0[6], a0.x)) + prismaticMul(R0[8], a0.z));
		p0.x = (NxReal)(p0Row0 + position0.x);
		p0.y = (NxReal)(p0Row1 + position0.y);
		p0.z = (NxReal)((double)p0Row2 + position0.z);

		const NxVec3& a1 = mWorldAnchor[1];
		const NxVec3& position1 = body1->mUnknown158;
		const double p1Row0 = (prismaticMul(R1[2], a1.z) + prismaticMul(R1[1], a1.y)) + prismaticMul(a1.x, R1[0]);
		const double p1Row1 = (prismaticMul(R1[3], a1.x) + prismaticMul(R1[5], a1.z)) + prismaticMul(R1[4], a1.y);
		const NxReal p1Row2 = (NxReal)((prismaticMul(R1[6], a1.x) + prismaticMul(R1[8], a1.z)) + prismaticMul(R1[7], a1.y));
		p1.x = (NxReal)(p1Row0 + position1.x);
		const double p1y = p1Row1 + position1.y;
		p1.y = (NxReal)p1y;
		p1.z = (NxReal)((double)p1Row2 + position1.z);

		// The midpoint M; x and y stored, z from the stored sum times 0.5
		// (0x101043cc) left on the stack.
		const NxReal mx = (NxReal)(((double)p1.x + p0.x) * 0.5f);
		const NxReal my = (NxReal)((p1y + p0.y) * 0.5f);
		const NxReal sumZ = (NxReal)((double)p1.z + p0.z);
		const double mz = (double)sumZ * 0.5f;

		// Each anchor's distance along n from M (0xae096-0xae108): body 0's
		// stored, body 1's on the stack; when the two are within one unit
		// (strictly: -1.0f < d0 - d1 < 1.0f, 0x1010687c / 0x101041ec) body
		// 1's becomes d0 + 1.
		const NxReal d0 = (NxReal)((((double)p0.x - mx) * n.x + ((double)p0.z - mz) * n.z) + ((double)p0.y - my) * n.y);
		double d1 = (((double)p1.x - mx) * n.x + ((double)p1.z - mz) * n.z) + (p1y - my) * n.y;
		const double gap = (double)d0 - d1;
		if(-1.0f < gap && gap < 1.0f)
			d1 = (double)d0 + 1.0f;

		NxVec3 c0;
		c0.x = (NxReal)(prismaticMul(n.x, d0) + mx);
		c0.y = (NxReal)(prismaticMul(n.y, d0) + my);
		const NxReal nzd0 = (NxReal)prismaticMul(n.z, d0);
		c0.z = (NxReal)((double)nzd0 + mz);
		const NxReal nxd1 = (NxReal)(n.x * d1);
		const NxReal nyd1 = (NxReal)(n.y * d1);
		c1.x = (NxReal)((double)nxd1 + mx);
		c1.y = (NxReal)((double)nyd1 + my);
		c1.z = (NxReal)(d1 * n.z + mz);

		// Records 1-2 levers (0xae1e6-0xae310): each anchor moved along its
		// own rotated axis to the point nearest C0.
		const double s0 = (((double)c0.x - p0.x) * q0.x + ((double)c0.z - p0.z) * q0.z) + ((double)c0.y - p0.y) * q0.y;
		const double x0x = q0.x * s0 + p0.x;
		const NxReal s0y = (NxReal)(s0 * q0.y);
		const NxReal s0z = (NxReal)(s0 * q0.z);
		const double x0y = (double)s0y + p0.y;
		const NxReal x0z = (NxReal)((double)s0z + p0.z);
		const double s1 = (((double)c0.x - p1.x) * q1.x + ((double)c0.z - p1.z) * q1.z) + ((double)c0.y - p1y) * q1.y;
		const NxReal s1y = (NxReal)(s1 * q1.y);
		const NxReal s1z = (NxReal)(s1 * q1.z);
		const NxReal x1x = (NxReal)(q1.x * s1 + p1.x);
		const NxReal x1y = (NxReal)((double)s1y + p1y);
		const NxReal x1z = (NxReal)((double)s1z + p1.z);
		r0.x = (NxReal)(x0x - position0.x);
		r0.y = (NxReal)(x0y - position0.y);
		r0.z = (NxReal)((double)x0z - position0.z);
		r1.x = (NxReal)((double)x1x - position1.x);
		r1.y = (NxReal)((double)x1y - position1.y);
		r1.z = (NxReal)((double)x1z - position1.z);
		}

	// Records 1 and 2 (0xae31a-0xae6ed).
	double gx, gy, gz;
	prismaticError(r0, r1, body0, body1, gx, gy, gz);
	const NxReal inverse = (NxReal)(1.0f / (double)arg);
	NxReal bias1 = prismaticBias(t1, gx, gy, gz, inverse);
	NxReal bias2 = prismaticBias(t2, gx, gy, gz, inverse);

	JointSupportRecord* record = row004093();
	prismaticLinearRecord(record, record0, record1, t1, r0, r1);
	prismaticSolveRecord(record, this, bias1, mMaxForce);

	record = row004093();
	prismaticLinearRecord(record, record0, record1, t2, r0, r1);
	prismaticSolveRecord(record, this, bias2, mMaxForce);

	// The second pair of levers (0xae6ef-0xae9d8).
	body0 = prismaticBody(mBody[0]);
	body1 = prismaticBody(mBody[1]);
	if(!body0)
		{
		// The s sum is grouped (z, y) + x here, unlike the first half.
		const NxVec3& a0 = mWorldAnchor[0];
		const NxVec3& n0 = mWorldAxis[0];
		const double dx = (double)pointA.x - a0.x;
		const double dy = (double)pointA.y - a0.y;
		const double dz = (double)pointA.z - a0.z;
		const double s = (dz * n0.z + dy * n0.y) + dx * n0.x;
		const NxReal sx = (NxReal)(s * n0.x);
		const NxReal sy = (NxReal)(s * n0.y);
		const double sz = s * n0.z;
		r0.x = (NxReal)((double)sx + a0.x);
		r0.y = (NxReal)((double)sy + a0.y);
		r0.z = (NxReal)(sz + a0.z);
		const NxVec3& position = body1->mUnknown158;
		r1.x = (NxReal)((double)pointA.x - position.x);
		r1.y = (NxReal)((double)pointA.y - position.y);
		r1.z = (NxReal)((double)pointA.z - position.z);
		}
	else if(!body1)
		{
		const NxVec3& a1 = mWorldAnchor[1];
		const NxVec3& n1 = mWorldAxis[1];
		const double dx = (double)pointB.x - a1.x;
		const double dy = (double)pointB.y - a1.y;
		const double dz = (double)pointB.z - a1.z;
		const double s = (dx * n1.x + dz * n1.z) + dy * n1.y;
		const NxReal sx = (NxReal)(s * n1.x);
		const NxReal sy = (NxReal)(s * n1.y);
		const double sz = s * n1.z;
		r1.x = (NxReal)((double)sx + a1.x);
		r1.y = (NxReal)((double)sy + a1.y);
		r1.z = (NxReal)(sz + a1.z);
		const NxVec3& position = body0->mUnknown158;
		r0.x = (NxReal)((double)pointB.x - position.x);
		r0.y = (NxReal)((double)pointB.y - position.y);
		r0.z = (NxReal)((double)pointB.z - position.z);
		}
	else
		{
		// Both levers towards C1 (0xae886-0xae9d8); body 1's sum is grouped
		// (z, y) + x and its anchor y is the stored float.
		const double s0 = (((double)c1.x - p0.x) * q0.x + ((double)c1.z - p0.z) * q0.z) + ((double)c1.y - p0.y) * q0.y;
		const double x0x = q0.x * s0 + p0.x;
		const NxReal s0y = (NxReal)(s0 * q0.y);
		const NxReal s0z = (NxReal)(s0 * q0.z);
		const double x0y = (double)s0y + p0.y;
		const NxReal x0z = (NxReal)((double)s0z + p0.z);
		const double ex = (double)c1.x - p1.x;
		const double s1 = (((double)c1.z - p1.z) * q1.z + ((double)c1.y - p1.y) * q1.y) + q1.x * ex;
		const NxReal s1y = (NxReal)(q1.y * s1);
		const NxReal s1z = (NxReal)(q1.z * s1);
		const NxReal x1x = (NxReal)(p1.x + q1.x * s1);
		const NxReal x1y = (NxReal)((double)s1y + p1.y);
		const NxReal x1z = (NxReal)((double)s1z + p1.z);
		const NxVec3& position0 = body0->mUnknown158;
		const NxVec3& position1 = body1->mUnknown158;
		r0.x = (NxReal)(x0x - position0.x);
		r0.y = (NxReal)(x0y - position0.y);
		r0.z = (NxReal)((double)x0z - position0.z);
		r1.x = (NxReal)((double)x1x - position1.x);
		r1.y = (NxReal)((double)x1y - position1.y);
		r1.z = (NxReal)((double)x1z - position1.z);
		}

	// Records 3 and 4 (0xae9dc-0xaed80): the same as 1 and 2, with the
	// stored 1 / arg.
	prismaticError(r0, r1, body0, body1, gx, gy, gz);
	bias1 = prismaticBias(t1, gx, gy, gz, inverse);
	bias2 = prismaticBias(t2, gx, gy, gz, inverse);

	record = row004093();
	prismaticLinearRecord(record, record0, record1, t1, r0, r1);
	prismaticSolveRecord(record, this, bias1, mMaxForce);

	record = row004093();
	prismaticLinearRecord(record, record0, record1, t2, r0, r1);
	prismaticSolveRecord(record, this, bias2, mMaxForce);

	// The relative rotation (0xaed82-0xaef4c): a = conj(q0) * q1 over the
	// bodies' +0x124 quaternions (identity for a missing body 0; conj(q0) as
	// is for a missing body 1), stored when both are there; then
	// e = a * mUnknown16c, with e.x, e.y and e.w stored and e.z on the stack.
	body0 = prismaticBody(mBody[0]);
	double aw, ax, ay, az;
	if(body0)
		{
		const NxReal* q = body0->mCMassOrientation;
		ax = -(double)q[0];
		ay = -(double)q[1];
		az = -(double)q[2];
		aw = q[3];
		}
	else
		{
		ax = -0.0f;
		ay = -0.0f;
		az = -0.0f;
		aw = 1.0f;
		}
	body1 = prismaticBody(mBody[1]);
	if(body1)
		{
		const NxReal* b = body1->mCMassOrientation;
		const NxReal rw = (NxReal)(((aw * b[3] - ax * b[0]) - ay * b[1]) - az * b[2]);
		const NxReal rx = (NxReal)(((aw * b[0] + ay * b[2]) + ax * b[3]) - az * b[1]);
		const NxReal ry = (NxReal)(((az * b[0] + ay * b[3]) + aw * b[1]) - ax * b[2]);
		const NxReal rz = (NxReal)(((aw * b[2] + ax * b[1]) + az * b[3]) - ay * b[0]);
		aw = rw;
		ax = rx;
		ay = ry;
		az = rz;
		}
	const NxReal* p = mUnknown16c;
	const NxReal ew = (NxReal)(((aw * p[3] - ax * p[0]) - ay * p[1]) - az * p[2]);
	const NxReal exF = (NxReal)(((aw * p[0] + ay * p[2]) + ax * p[3]) - az * p[1]);
	NxReal ey = (NxReal)(((az * p[0] + ay * p[3]) + aw * p[1]) - ax * p[2]);
	double ez = ((aw * p[2] + ax * p[1]) + az * p[3]) - ay * p[0];
	double ex = exF;
	// e.w < 0 (0xaef56 against 0.0f): every vector component times -1.0f.
	if(ew < 0.0f)
		{
		ex = (double)exF * -1.0f;
		ey = (NxReal)((double)ey * -1.0f);
		ez = ez * -1.0f;
		}

	// Rotated by body 0's +0x134 3x3 (0xaef8b-0xaefe9); x and y stored.
	NxReal vx;
	NxReal vy;
	double vz;
	if(body0)
		{
		const NxReal* R = body0->mUnknown134;
		vx = (NxReal)(((double)ey * R[1] + ez * R[2]) + ex * R[0]);
		vy = (NxReal)((ex * R[3] + (double)ey * R[4]) + ez * R[5]);
		vz = (ex * R[6] + (double)ey * R[7]) + ez * R[8];
		}
	else
		{
		vx = (NxReal)ex;
		vy = ey;
		vz = ez;
		}
	const NxReal angular0 = (NxReal)(((double)inverse * vx) * -2.0f);
	const NxReal angular1 = (NxReal)(((double)inverse * vy) * -2.0f);
	const NxReal angular2 = (NxReal)(((double)inverse * vz) * -2.0f);

	// Records 5-7 (0xaf04a-0xaf2b6), one per unit axis, +0x48 = maxTorque.
	record = row004093();
	prismaticAngularRecord(record, record0, record1, gJointUnitAxis[0]);
	prismaticSolveRecord(record, this, angular0, mMaxTorque);

	record = row004093();
	prismaticAngularRecord(record, record0, record1, gJointUnitAxis[1]);
	prismaticSolveRecord(record, this, angular1, mMaxTorque);

	record = row004093();
	prismaticAngularRecord(record, record0, record1, gJointUnitAxis[2]);
	prismaticSolveRecord(record, this, angular2, mMaxTorque);
	}

// phys_fn_004384 (0x000ad780, 202 B)
// desc.isValid() is the descriptor's virtual (`call [edx+8]`, 0xad7be). The
// actors are re-bound (phys_fn_004107 with suppressAttach false) only when a
// body differs from the one held; the second body is not looked at when the
// first already differs. Then the base part (004121) and 004378.
void PrismaticJoint::loadFromDesc(const NxPrismaticJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_PRISMATICJOINT_CPP, 0x39, 0,
			"PrismaticJoint::loadFromDesc: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_PRISMATICJOINT_CPP, 0x3a, 0,
			"PrismaticJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	void* actorImpl0 = prismaticActorImpl(desc.actor[0]);
	void* actorImpl1 = prismaticActorImpl(desc.actor[1]);
	if(prismaticBodyOfActorImpl(actorImpl0) != mBody[0] || prismaticBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	row004378(desc);
	}

// phys_fn_004376 (0x000ad4e0, 54 B)
// A broken joint reports (code 1, line 0x4d) and returns; otherwise the row
// tail-jumps to the base part (row 004066, 0xad511). NxPrismaticJointDesc has
// no field of its own.
void PrismaticJoint::saveToDesc(NxPrismaticJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_PRISMATICJOINT_CPP, 0x4d, 0,
			"PrismaticJoint::saveToDesc: Joint is broken. Broken joints can't be saved!");
		return;
		}
	saveToDescBase(desc);
	}

// phys_fn_004378 (0x000ad520, 443 B)
// mUnknown16c = body 0's +0x124 quaternion (or the identity), conjugated in
// place (three fld/fchs/fstp); with body 1, multiplied on the right by body
// 1's +0x124 quaternion, every product read from the old values and each
// result rounded once where it is stored (y through a stack slot, 0xad653);
// then the vector part is negated again in place (0xad6ab-0xad6cf). The
// descriptor argument is never read (`ret 4`).
void PrismaticJoint::row004378(const NxPrismaticJointDesc& desc)
	{
	(void)desc;
	const JointBodyRecord* body0 = prismaticBody(mBody[0]);
	NxReal* q = mUnknown16c;
	if(body0)
		{
		q[0] = body0->mCMassOrientation[0];
		q[1] = body0->mCMassOrientation[1];
		q[2] = body0->mCMassOrientation[2];
		q[3] = body0->mCMassOrientation[3];
		}
	else
		{
		q[0] = 0.0f;
		q[1] = 0.0f;
		q[2] = 0.0f;
		q[3] = 1.0f;
		}
	q[0] = -q[0];
	q[1] = -q[1];
	q[2] = -q[2];

	const JointBodyRecord* body1 = prismaticBody(mBody[1]);
	if(body1)
		{
		const NxReal* b = body1->mCMassOrientation;
		const double w = ((prismaticMul(b[3], q[3]) - prismaticMul(q[0], b[0])) - prismaticMul(q[1], b[1])) - prismaticMul(q[2], b[2]);
		const double x = ((prismaticMul(q[0], b[3]) + prismaticMul(q[1], b[2])) + prismaticMul(q[3], b[0])) - prismaticMul(q[2], b[1]);
		const NxReal y = (NxReal)(((prismaticMul(b[3], q[1]) + prismaticMul(q[2], b[0])) + prismaticMul(b[1], q[3])) - prismaticMul(q[0], b[2]));
		const NxReal z = (NxReal)(((prismaticMul(q[0], b[1]) + prismaticMul(b[3], q[2])) + prismaticMul(b[2], q[3])) - prismaticMul(q[1], b[0]));
		q[1] = y;
		q[2] = z;
		q[3] = (NxReal)w;
		q[0] = (NxReal)x;
		}

	q[0] = -q[0];
	q[1] = -q[1];
	q[2] = -q[2];
	}
