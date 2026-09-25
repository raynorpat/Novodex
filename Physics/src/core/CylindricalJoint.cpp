/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/CylindricalJoint.h"
#include "core/NpCylindricalJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "X87Sqrt.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"
#include "NxUtilities.h"

#include <math.h>
#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x1011a080).
#define NX_CYLINDRICALJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\CylindricalJoint.cpp"

// Joint-families Task 3b. Floating point follows core/PrismaticJoint.cpp:
// this translation unit is x87 in the oracle and is built /arch:IA32 here; a
// value the listing keeps on the FPU stack is a `double`, a value it stores
// (fstp dword) is an `NxReal`, and the listing's operand grouping is kept
// (three-term sums are written in the listing's term order, grouped as the
// listing adds them). CylindricalJoint is constructed by Scene::createJoint's
// cylindrical case (NxJointType 2). See units/joint-families-contract.md
// "## Cylindrical".

// (a0 * b0 + a1 * b1) + a2 * b2 with the products and the partial sum on the
// stack; the argument order is the listing's term order at each site.
static NX_INLINE double cylindricalSum3(NxReal a0, NxReal b0, NxReal a1, NxReal b1, NxReal a2, NxReal b2)
	{
	return (jointLinearMul(a0, b0) + jointLinearMul(a1, b1)) + jointLinearMul(a2, b2);
	}

static NX_INLINE JointBodyRecord* cylindricalBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* cylindricalActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* cylindricalBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// The inlined loop 004318 and 004326 open with (0xa7255-0xa7282,
// 0xa785a-0xa7882; the same loop as Joint.cpp's 004125/004129): refresh the
// first body whose stamp no longer matches, and only that one.
static void cylindricalRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = cylindricalBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// The bias of one linear record in 004326 (0xa8345-0xa835f for t1,
// 0xa8366-0xa837c for t2, and the same at 0xa89c6-0xa8a06):
// ((e.x * t.x + t.z * e.z) + t.y * e.y) * inverse, stored. Prismatic's
// grouping is different ((t.z e.z + t.y e.y) + e.x t.x).
static NxReal cylindricalBias(const NxVec3& t, double gx, double gy, double gz, NxReal inverse)
	{
	return (NxReal)(((gx * t.x + (double)t.z * gz) + (double)t.y * gy) * inverse);
	}

// phys_fn_004320 (0x000a76a0, 87 B)
// Joint(desc, 0x100) runs first (0xa76a6-0xa76ae); the compiler then stores
// the vptr 0x1011a048 (0xa76b3). The public object is allocated through the Foundation
// allocator (`push 0; push 0x1c; call [edx+8]`) and constructed only
// when the allocation succeeded, but desc.userData is written to it without
// a null check (the null arm stores 0 at +0x48 and then writes [0+4],
// 0xa76e5-0xa76ed): a failed allocation faults there in the oracle, and does
// here too. The class has no field of its own, so nothing follows.
CylindricalJoint::CylindricalJoint(const NxCylindricalJointDesc& desc)
	: Joint(desc, 0x100)
	{
	void* memory = nxFoundationSDKAllocator->malloc(sizeof(NpCylindricalJoint), NX_MEMORY_PERSISTENT);
	NpCylindricalJoint* publicJoint = memory ? new(memory) NpCylindricalJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	}

// phys_fn_004322 (0x000a7700, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x1011a048, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`, 0xa7712-0xa7714), calls the Joint
// destructor body (row 004095) directly, and frees `this` through the SDK
// allocator (slot +0x14) when the flag's bit 0 is set (Joint::operator
// delete).
CylindricalJoint::~CylindricalJoint()
	{
	if(mPublicObject)
		delete static_cast<NpCylindricalJoint*>(mPublicObject);
	}

// Slot 4: phys_fn_004318, written once below as Joint::row004318 because the
// prismatic table (0x1011a4d0 slot 4) names the same body. The oracle's table
// points at the body directly; this override only forwards to it.
void CylindricalJoint::row_slot4(NxDebugRenderable& renderable)
	{
	row004318(renderable);
	}

// phys_fn_004326 (0x000a7810, 5377 B)
// The cylindrical solver slot: the first four records of the prismatic one
// (phys_fn_004386) and nothing after them. After the stale-body refresh it
// takes the slide direction n (a missing body 0: world axis 0; a missing
// body 1: world axis 1; both: the normalized sum of the two rotated axes),
// two tangents t1, t2 of n from NxNormalToTangents, and levers r0/r1 twice:
// once from the anchors (records 1-2), once from points one axis length along
// each body's axis (records 3-4). With both bodies each lever is the body's
// anchor moved along its own axis to the point of the common line nearest a
// point C (C0 = the midpoint of the anchors' projections on n; C1 = C0 moved
// one unit along n when the projections are within one unit of each other,
// else the other projection). Each pair gives two kind-1 records along t1 and
// t2 with bias (t . error) / arg and +0x48 = maxForce. No angular record:
// the joint turns freely about its axis.
// Listing over decompile (supplement): the supplement drops the kind tests as
// unreachable (0x100a8458, 0x100a85cf, 0x100a8ae2, 0x100a8c59), shows every
// intermediate as a float and regroups most sums; the listing is followed
// throughout, including which values stay on the stack (named double below).
// The three-term groupings are the cylindrical listing's own and differ from
// prismatic's at these sites: the no-body-0 anchor rows 1 and 2 (0xa78f6,
// 0xa791e), the no-body-0 projection sums of both halves (0xa7a1f,
// 0xa8698), the no-body-1 projection sum of the second half (0xa8765), the
// length of n (0xa7e11-0xa7e25), body 0's anchor rows 1 and 2 and body 1's
// anchor rows 1 and 2 with both bodies (0xa7ea3, 0xa7ecb, 0xa7f67, 0xa7f8f),
// both distances along n (0xa804d-0xa805f, 0xa8085-0xa8097), both first-half
// lever sums (0xa8188-0xa819a, 0xa81ed-0xa81ff), body 0's second-half lever
// sum (0xa8830-0xa8842) and the biases (above). With both bodies the first
// half carries body 1's anchor y unrounded (0xa7fc7, 0xa8074, 0xa81e0) and
// the second half reads it back as the stored float (0xa8894).
void CylindricalJoint::row_slot6(NxReal arg)
	{
	JointBodyRecord* body0 = cylindricalBody(mBody[0]);
	JointSupportBody* const record0 = body0 ? body0->mUnknown204 : 0;
	JointBodyRecord* body1 = cylindricalBody(mBody[1]);
	JointSupportBody* const record1 = body1 ? body1->mUnknown204 : 0;
	cylindricalRefreshFirstStaleBody(*this);
	body0 = cylindricalBody(mBody[0]);

	NxVec3 n;
	NxVec3 t1;
	NxVec3 t2;
	NxVec3 r0;
	NxVec3 r1;

	// No body 0: the point one axis length along body 1's axis.
	NxVec3 pointA;
	// No body 1: the same along body 0's axis.
	NxVec3 pointB;
	// Both: the rotated axes, the world anchors and C1.
	NxVec3 q0;
	NxVec3 q1;
	NxVec3 p0;
	NxVec3 p1;
	NxVec3 c1;

	if(!body0)
		{
		// 0xa7892-0xa7aaa
		n = mWorldAxis[0];
		NxNormalToTangents(n, t1, t2);
		body1 = cylindricalBody(mBody[1]);
		const NxReal* R = body1->mUnknown134;
		const NxVec3& a = mWorldAnchor[1];
		const NxVec3& position = body1->mUnknown158;
		const double row0 = cylindricalSum3(R[2], a.z, R[1], a.y, a.x, R[0]);
		const double row1 = cylindricalSum3(R[3], a.x, R[5], a.z, R[4], a.y);
		const NxReal row2 = (NxReal)cylindricalSum3(R[6], a.x, R[8], a.z, R[7], a.y);
		const NxReal p1x = (NxReal)(row0 + position.x);
		const double p1y = row1 + position.y;
		const double p1z = (double)row2 + position.z;
		const NxVec3& w = mWorldAxis[1];
		const double w0 = cylindricalSum3(R[2], w.z, R[1], w.y, R[0], w.x);
		const double w1 = cylindricalSum3(R[5], w.z, R[3], w.x, R[4], w.y);
		const NxReal w2 = (NxReal)cylindricalSum3(R[8], w.z, R[6], w.x, R[7], w.y);
		pointA.x = (NxReal)(w0 + p1x);
		pointA.y = (NxReal)(w1 + p1y);
		pointA.z = (NxReal)((double)w2 + p1z);

		const NxVec3& a0 = mWorldAnchor[0];
		const NxVec3& n0 = mWorldAxis[0];
		const double dx = (double)p1x - a0.x;
		const double dy = p1y - a0.y;
		const double dz = p1z - a0.z;
		const double s = (dz * n0.z + dy * n0.y) + dx * n0.x;
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
	else if(!(body1 = cylindricalBody(mBody[1])))
		{
		// 0xa7aba-0xa7cd6
		n = mWorldAxis[1];
		NxNormalToTangents(n, t1, t2);
		const NxReal* R = body0->mUnknown134;
		const NxVec3& a = mWorldAnchor[0];
		const NxVec3& position = body0->mUnknown158;
		const double row0 = cylindricalSum3(R[1], a.y, R[2], a.z, R[0], a.x);
		const double row1 = cylindricalSum3(R[4], a.y, R[3], a.x, R[5], a.z);
		const NxReal row2 = (NxReal)cylindricalSum3(R[7], a.y, R[6], a.x, R[8], a.z);
		const NxReal p0x = (NxReal)(row0 + position.x);
		const double p0y = row1 + position.y;
		const double p0z = (double)row2 + position.z;
		const NxVec3& w = mWorldAxis[0];
		const double w0 = cylindricalSum3(R[2], w.z, R[1], w.y, w.x, R[0]);
		const double w1 = cylindricalSum3(R[5], w.z, R[4], w.y, R[3], w.x);
		const NxReal w2 = (NxReal)cylindricalSum3(R[8], w.z, R[7], w.y, R[6], w.x);
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
		// 0xa7cdb-0xa8297
		const NxReal* R0 = body0->mUnknown134;
		const NxReal* R1 = body1->mUnknown134;
		const NxVec3& w0 = mWorldAxis[0];
		q0.x = (NxReal)cylindricalSum3(R0[1], w0.y, R0[2], w0.z, w0.x, R0[0]);
		q0.y = (NxReal)cylindricalSum3(R0[4], w0.y, R0[3], w0.x, R0[5], w0.z);
		q0.z = (NxReal)cylindricalSum3(R0[7], w0.y, R0[6], w0.x, R0[8], w0.z);
		const NxVec3& w1 = mWorldAxis[1];
		q1.x = (NxReal)cylindricalSum3(R1[1], w1.y, R1[2], w1.z, R1[0], w1.x);
		q1.y = (NxReal)cylindricalSum3(R1[4], w1.y, R1[3], w1.x, R1[5], w1.z);
		q1.z = (NxReal)cylindricalSum3(R1[7], w1.y, R1[6], w1.x, R1[8], w1.z);

		// n = q0 + q1, normalized unless its length is 0 (0xa7de3-0xa7e5c):
		// x and z are normalized from the unrounded sums, y from its float;
		// the length sums z, y, then x.
		const double sx = (double)q0.x + q1.x;
		const NxReal sy = (NxReal)((double)q0.y + q1.y);
		const double sz = (double)q0.z + q1.z;
		n.x = (NxReal)sx;
		n.y = sy;
		n.z = (NxReal)sz;
		const double length = x87FsqrtDot3(sz, sz, sy, sy, sx, sx);
		if(length != 0.0f)
			{
			const double scale = 1.0f / length;
			n.x = (NxReal)(sx * scale);
			n.y = (NxReal)((double)sy * scale);
			n.z = (NxReal)(scale * sz);
			}
		NxNormalToTangents(n, t1, t2);

		// The world anchors (0xa7e75-0xa7fe7), all three components stored.
		const NxVec3& a0 = mWorldAnchor[0];
		const NxVec3& position0 = body0->mUnknown158;
		const double p0Row0 = cylindricalSum3(R0[2], a0.z, R0[1], a0.y, R0[0], a0.x);
		const double p0Row1 = cylindricalSum3(R0[5], a0.z, R0[4], a0.y, R0[3], a0.x);
		const NxReal p0Row2 = (NxReal)cylindricalSum3(R0[8], a0.z, R0[7], a0.y, R0[6], a0.x);
		p0.x = (NxReal)(p0Row0 + position0.x);
		p0.y = (NxReal)(p0Row1 + position0.y);
		p0.z = (NxReal)((double)p0Row2 + position0.z);

		const NxVec3& a1 = mWorldAnchor[1];
		const NxVec3& position1 = body1->mUnknown158;
		const double p1Row0 = cylindricalSum3(R1[1], a1.y, R1[2], a1.z, a1.x, R1[0]);
		const double p1Row1 = cylindricalSum3(R1[4], a1.y, R1[3], a1.x, R1[5], a1.z);
		const NxReal p1Row2 = (NxReal)cylindricalSum3(R1[7], a1.y, R1[6], a1.x, R1[8], a1.z);
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

		// Each anchor's distance along n from M (0xa802f-0xa809b): body 0's
		// stored, body 1's on the stack; when the two are within one unit
		// (strictly: -1.0f < d0 - d1 < 1.0f, 0x1010687c / 0x101041ec) body
		// 1's becomes d0 + 1.
		const NxReal d0 = (NxReal)((((double)p0.x - mx) * n.x + ((double)p0.y - my) * n.y) + ((double)p0.z - mz) * n.z);
		double d1 = (((double)p1.x - mx) * n.x + (p1y - my) * n.y) + ((double)p1.z - mz) * n.z;
		const double gap = (double)d0 - d1;
		if(-1.0f < gap && gap < 1.0f)
			d1 = (double)d0 + 1.0f;

		NxVec3 c0;
		c0.x = (NxReal)(jointLinearMul(n.x, d0) + mx);
		c0.y = (NxReal)(jointLinearMul(n.y, d0) + my);
		const NxReal nzd0 = (NxReal)jointLinearMul(n.z, d0);
		c0.z = (NxReal)((double)nzd0 + mz);
		const NxReal nxd1 = (NxReal)(n.x * d1);
		const NxReal nyd1 = (NxReal)(n.y * d1);
		c1.x = (NxReal)((double)nxd1 + mx);
		c1.y = (NxReal)((double)nyd1 + my);
		c1.z = (NxReal)(d1 * n.z + mz);

		// Records 1-2 levers (0xa8167-0xa8297): each anchor moved along its
		// own rotated axis to the point nearest C0; both sums z, y, then x.
		const double s0 = (((double)c0.z - p0.z) * q0.z + ((double)c0.y - p0.y) * q0.y) + ((double)c0.x - p0.x) * q0.x;
		const double x0x = q0.x * s0 + p0.x;
		const NxReal s0y = (NxReal)(s0 * q0.y);
		const NxReal s0z = (NxReal)(s0 * q0.z);
		const double x0y = (double)s0y + p0.y;
		const NxReal x0z = (NxReal)((double)s0z + p0.z);
		const double s1 = (((double)c0.z - p1.z) * q1.z + ((double)c0.y - p1y) * q1.y) + ((double)c0.x - p1.x) * q1.x;
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

	// Records 1 and 2 (0xa829b-0xa866d).
	body0 = cylindricalBody(mBody[0]);
	body1 = cylindricalBody(mBody[1]);
	double gx, gy, gz;
	jointLinearError(r0, r1, body0, body1, gx, gy, gz);
	const NxReal inverse = (NxReal)(1.0f / (double)arg);
	NxReal bias1 = cylindricalBias(t1, gx, gy, gz, inverse);
	NxReal bias2 = cylindricalBias(t2, gx, gy, gz, inverse);

	JointSupportRecord* record = row004093();
	jointLinearRecord(record, record0, record1, t1, r0, r1);
	jointSolveRecord(record, this, bias1, mMaxForce);

	record = row004093();
	jointLinearRecord(record, record0, record1, t2, r0, r1);
	jointSolveRecord(record, this, bias2, mMaxForce);

	// The second pair of levers (0xa866f-0xa8950).
	body0 = cylindricalBody(mBody[0]);
	if(!body0)
		{
		// The s sum is grouped (x, z) + y here, unlike the first half.
		body1 = cylindricalBody(mBody[1]);
		const NxVec3& a0 = mWorldAnchor[0];
		const NxVec3& n0 = mWorldAxis[0];
		const double dx = (double)pointA.x - a0.x;
		const double dy = (double)pointA.y - a0.y;
		const double dz = (double)pointA.z - a0.z;
		const double s = (dx * n0.x + dz * n0.z) + dy * n0.y;
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
	else if(!(body1 = cylindricalBody(mBody[1])))
		{
		// The s sum is grouped (z, y) + x here, unlike the first half.
		const NxVec3& a1 = mWorldAnchor[1];
		const NxVec3& n1 = mWorldAxis[1];
		const double dx = (double)pointB.x - a1.x;
		const double dy = (double)pointB.y - a1.y;
		const double dz = (double)pointB.z - a1.z;
		const double s = (dz * n1.z + dy * n1.y) + dx * n1.x;
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
		// Both levers towards C1 (0xa8806-0xa8950); both sums z, y, then x,
		// over the stored anchors (body 1's y is the stored float here).
		const double s0 = (((double)c1.z - p0.z) * q0.z + ((double)c1.y - p0.y) * q0.y) + ((double)c1.x - p0.x) * q0.x;
		const double x0x = q0.x * s0 + p0.x;
		const NxReal s0y = (NxReal)(s0 * q0.y);
		const NxReal s0z = (NxReal)(s0 * q0.z);
		const double x0y = (double)s0y + p0.y;
		const NxReal x0z = (NxReal)((double)s0z + p0.z);
		const double s1 = (((double)c1.z - p1.z) * q1.z + ((double)c1.y - p1.y) * q1.y) + ((double)c1.x - p1.x) * q1.x;
		const NxReal s1y = (NxReal)(s1 * q1.y);
		const NxReal s1z = (NxReal)(s1 * q1.z);
		const NxReal x1x = (NxReal)(q1.x * s1 + p1.x);
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

	// Records 3 and 4 (0xa8954-0xa8d05): the same as 1 and 2, with the stored
	// 1 / arg.
	jointLinearError(r0, r1, body0, body1, gx, gy, gz);
	bias1 = cylindricalBias(t1, gx, gy, gz, inverse);
	bias2 = cylindricalBias(t2, gx, gy, gz, inverse);

	record = row004093();
	jointLinearRecord(record, record0, record1, t1, r0, r1);
	jointSolveRecord(record, this, bias1, mMaxForce);

	record = row004093();
	jointLinearRecord(record, record0, record1, t2, r0, r1);
	jointSolveRecord(record, this, bias2, mMaxForce);
	}

// phys_fn_004324 (0x000a7740, 194 B)
// desc.isValid() is the descriptor's virtual (`call [edx+8]`, 0xa777e). The
// actors are re-bound (phys_fn_004107 with suppressAttach false) only when a
// body differs from the one held; the second body is not looked at when the
// first already differs. Then the base part (004121); unlike prismatic's
// 004384 nothing follows.
void CylindricalJoint::loadFromDesc(const NxCylindricalJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_CYLINDRICALJOINT_CPP, 0x26, 0,
			"CylindricalJoint::loadFromDesc: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_CYLINDRICALJOINT_CPP, 0x27, 0,
			"CylindricalJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	void* actorImpl0 = cylindricalActorImpl(desc.actor[0]);
	void* actorImpl1 = cylindricalActorImpl(desc.actor[1]);
	if(cylindricalBodyOfActorImpl(actorImpl0) != mBody[0] || cylindricalBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	}

// phys_fn_004316 (0x000a7200, 54 B)
// A broken joint reports (code 1, line 0x3b) and returns; otherwise the row
// tail-jumps to the base part (row 004066, 0xa7231). The report text is the
// oracle's own (0x1011a0c0): it names loadFromDesc although this is
// saveToDesc. NxCylindricalJointDesc has no field of its own.
void CylindricalJoint::saveToDesc(NxCylindricalJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_CYLINDRICALJOINT_CPP, 0x3b, 0,
			"CylindricalJoint::loadFromDesc: Joint is broken. Broken joints can't be saved!");
		return;
		}
	saveToDescBase(desc);
	}

// phys_fn_004318 (0x000a7240, 1115 B)
// Debug visualization, only when +0x2c bit 9 (NX_JF_VISUALIZATION) is set;
// the body both the cylindrical and the prismatic internal tables name at
// slot 4 (it reads only Joint base fields). Each part is gated by its SDK
// parameter being non-zero and scaled by it times NX_VISUALIZATION_SCALE
// (element 13, 0x10123b4c):
// - world axes (element 32, 0x10123b98): an arrow along row004127's axis at
//   the point row004123 returns, colour 0xffffff (as revolute 004364);
// - local axes (element 31, 0x10123b94): per body, the world anchor and axis
//   carried through the body's +0x134/+0x158 pose (as stored without a
//   body); an arrow along each axis from its anchor, 0x202090 for body 0 and
//   0x5050e0 for body 1.
// The local-axes arm constructs four two-element NxVec3 arrays through the
// compiler's `eh vector constructor iterator` (phys_fn_000001 with the folded
// NxVec3 constructor 001391, 0xa72ff-0xa7346) and uses two of them; the other
// two are declared here as the oracle's source evidently declared them.
// Listing over decompile: the decompile passes the second arrow
// (&fStack_50, auStack_68); the listing pushes body 1's anchor and axis
// (0xa7683-0xa768c). The listing keeps the values named double below on the
// stack.
void Joint::row004318(NxDebugRenderable& renderable)
	{
	if(!((mFlags >> 9) & 1))
		return;

	cylindricalRefreshFirstStaleBody(*this);

	if(jointLinearSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES) != 0.0f)
		{
		NxVec3 anchor;
		row004123(anchor);
		NxVec3 axis;
		row004127(axis);
		const NxReal scale = (NxReal)((double)jointLinearSdkParameter(NX_VISUALIZATION_SCALE) *
			jointLinearSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES));
		renderable.addArrow(anchor, axis, 1.0f, scale, 0xffffff);
		}

	if(jointLinearSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) != 0.0f)
		{
		NxVec3 anchor[2];
		NxVec3 axis[2];
		NxVec3 unused0[2];
		NxVec3 unused1[2];
		(void)unused0;
		(void)unused1;

		const JointBodyRecord* body0 = cylindricalBody(mBody[0]);
		if(!body0)
			{
			anchor[0] = mWorldAnchor[0];
			axis[0] = mWorldAxis[0];
			}
		else
			{
			// 0xa7392-0xa74c4.
			const NxReal* m = body0->mUnknown134;
			const NxVec3& t = body0->mUnknown158;
			const NxVec3& p = mWorldAnchor[0];
			const double px = cylindricalSum3(m[1], p.y, m[2], p.z, m[0], p.x);
			const NxReal py = (NxReal)cylindricalSum3(m[4], p.y, m[3], p.x, m[5], p.z);
			const NxReal pz = (NxReal)cylindricalSum3(m[7], p.y, m[6], p.x, m[8], p.z);
			anchor[0].x = (NxReal)(px + t.x);
			anchor[0].y = (NxReal)((double)py + t.y);
			anchor[0].z = (NxReal)((double)pz + t.z);
			const NxVec3& w = mWorldAxis[0];
			axis[0].y = (NxReal)cylindricalSum3(m[3], w.x, m[5], w.z, m[4], w.y);
			axis[0].z = (NxReal)cylindricalSum3(m[6], w.x, m[8], w.z, m[7], w.y);
			axis[0].x = (NxReal)cylindricalSum3(m[2], w.z, m[1], w.y, m[0], w.x);
			}

		const JointBodyRecord* body1 = cylindricalBody(mBody[1]);
		if(!body1)
			{
			anchor[1] = mWorldAnchor[1];
			axis[1] = mWorldAxis[1];
			}
		else
			{
			// 0xa7510-0xa7642.
			const NxReal* m = body1->mUnknown134;
			const NxVec3& t = body1->mUnknown158;
			const NxVec3& p = mWorldAnchor[1];
			const double px = cylindricalSum3(m[2], p.z, m[1], p.y, m[0], p.x);
			const NxReal py = (NxReal)cylindricalSum3(m[5], p.z, m[4], p.y, m[3], p.x);
			const NxReal pz = (NxReal)cylindricalSum3(m[8], p.z, m[7], p.y, m[6], p.x);
			anchor[1].x = (NxReal)(px + t.x);
			anchor[1].y = (NxReal)((double)py + t.y);
			anchor[1].z = (NxReal)((double)pz + t.z);
			const NxVec3& w = mWorldAxis[1];
			axis[1].y = (NxReal)cylindricalSum3(m[5], w.z, m[3], w.x, m[4], w.y);
			axis[1].z = (NxReal)cylindricalSum3(m[8], w.z, m[6], w.x, m[7], w.y);
			axis[1].x = (NxReal)cylindricalSum3(m[2], w.z, m[1], w.y, m[0], w.x);
			}

		const NxReal scale = (NxReal)((double)jointLinearSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) *
			jointLinearSdkParameter(NX_VISUALIZATION_SCALE));
		renderable.addArrow(anchor[0], axis[0], 1.0f, scale, 0x202090);
		renderable.addArrow(anchor[1], axis[1], 1.0f, scale, 0x5050e0);
		}
	}
