/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/DistanceJoint.h"
#include "core/NpDistanceJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <math.h>
#include <new>

// The oracle's __FILE__ for this unit (every report in it pushes the string
// at 0x1011997c).
#define NX_DISTANCEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\DistanceJoint.cpp"

// Joint-families Task 3f. Floating point follows core/PointInPlaneJoint.cpp:
// this translation unit is x87 in the oracle and is built /arch:IA32 here; a
// value the listing keeps on the FPU stack is a `double`, a value it stores
// (fstp dword) is an `NxReal`, and the listing's operand grouping and order
// are kept. DistanceJoint is constructed by Scene::createJoint's distance
// case (NxJointType 6). No row is shared with another family. See
// units/joint-families-contract.md "## Distance".

static NX_INLINE double distanceMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

static NX_INLINE JointBodyRecord* distanceBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* distanceActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* distanceBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// The inlined loop 004232 and 004240 open with (0x9f264-0x9f297,
// 0x9f629-0x9f659; the same loop as Joint.cpp's 004125/004129): refresh the
// first body whose stamp no longer matches, and only that one.
static void distanceRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = distanceBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// One world anchor for 004232 (0x9f2a2-0x9f36a for body 0, 0x9f379-0x9f441
// for body 1): R * a + t through the body's +0x134/+0x158 pose, x summed as
// ((R2 a.z + R1 a.y) + R0 a.x) and kept until t.x is added, y and z stored
// before t is added; the stored anchor without a body.
static void distanceWorldAnchor(const JointBodyRecord* body, const NxVec3& a, NxVec3& out)
	{
	if(!body)
		{
		out = a;
		return;
		}
	const NxReal* R = body->mUnknown134;
	const NxVec3& t = body->mUnknown158;
	const double x = (distanceMul(R[2], a.z) + distanceMul(R[1], a.y)) + distanceMul(R[0], a.x);
	const NxReal y = (NxReal)((distanceMul(R[5], a.z) + distanceMul(R[4], a.y)) + distanceMul(R[3], a.x));
	const NxReal z = (NxReal)((distanceMul(R[8], a.z) + distanceMul(R[7], a.y)) + distanceMul(R[6], a.x));
	out.x = (NxReal)(x + t.x);
	out.y = (NxReal)((double)y + t.y);
	out.z = (NxReal)((double)z + t.z);
	}

// One body's part of 004240 (0x9f659-0x9f751 for body 0, 0x9f753-0x9f850
// for body 1): the support record (+0x204), the lever r = R * a (stored; x
// summed as ((R1 a.y + R2 a.z) + R0 a.x)) and the world point p = r + t,
// whose x stays on the FPU stack while y and z are stored. Without a body r
// and p are the stored anchor and the record is null.
static void distanceLever(const JointBodyRecord* body, const NxVec3& a, JointSupportBody*& record, NxVec3& r,
	double& px, NxReal& py, NxReal& pz)
	{
	if(!body)
		{
		r = a;
		px = a.x;
		py = a.y;
		pz = a.z;
		record = 0;
		return;
		}
	record = body->mUnknown204;
	const NxReal* R = body->mUnknown134;
	const NxVec3& t = body->mUnknown158;
	r.x = (NxReal)((distanceMul(R[1], a.y) + distanceMul(R[2], a.z)) + distanceMul(R[0], a.x));
	r.y = (NxReal)((distanceMul(R[4], a.y) + distanceMul(R[5], a.z)) + distanceMul(R[3], a.x));
	r.z = (NxReal)((distanceMul(R[7], a.y) + distanceMul(R[8], a.z)) + distanceMul(R[6], a.x));
	px = (double)r.x + t.x;
	py = (NxReal)((double)r.y + t.y);
	pz = (NxReal)((double)r.z + t.z);
	}

// The record vectors every 004240 arm writes (0x9f95e-0x9f9f7 and its three
// copies): the direction t at +0x00, the support records at +0x10/+0x14, and
// the lever crosses at +0x18 (r0) and +0x24 (r1), each one product minus
// another in the listing's operand order, stored x, y, z.
static void distanceRecord(JointSupportRecord* record, JointSupportBody* body0, JointSupportBody* body1,
	const NxVec3& t, const NxVec3& r0, const NxVec3& r1)
	{
	record->mBody[1] = body1;
	record->mUnknown000 = t;
	record->mBody[0] = body0;
	record->mUnknown018.x = (NxReal)(distanceMul(r0.y, t.z) - distanceMul(r0.z, t.y));
	record->mUnknown018.y = (NxReal)(distanceMul(r0.z, t.x) - distanceMul(t.z, r0.x));
	record->mUnknown018.z = (NxReal)(distanceMul(t.y, r0.x) - distanceMul(r0.y, t.x));
	record->mUnknown024.x = (NxReal)(distanceMul(r1.y, t.z) - distanceMul(r1.z, t.y));
	record->mUnknown024.y = (NxReal)(distanceMul(r1.z, t.x) - distanceMul(r1.x, t.z));
	record->mUnknown024.z = (NxReal)(distanceMul(r1.x, t.y) - distanceMul(r1.y, t.x));
	}

// The kind and bit update after the vectors (kind 1: 0x9f9fa-0x9fa5a and
// 0x9fb38-0x9fb94; kind 0: 0x9fccf-0x9fd0c with 0x9ff4f-0x9ff86, and
// 0xa0032-0xa006a with 0x9fe05-0x9fe3b): the kind in bits 0-4
// ((flags & keep) | kind), bit 9 = (kind is 0 or 2) stored through an xor,
// then bit 10 = (kind is 3, 2 or 5) with bits 5-8 and 11-18 cleared. The kind
// is known, but the listing tests it (the decompile drops those arms as
// unreachable), so the tests stay. Kind 1 is jointLinearRecord's sequence.
static void distanceRecordBits(JointSupportRecord* record, NxU32 keep, NxU32 kind)
	{
	NxU32 flags = (record->mFlags & keep) | kind;
	record->mFlags = flags;
	NxU32 current = flags & 0x1f;
	const NxU32 bit9 = (current == 0 || current == 2) ? 1 : 0;
	flags = (((bit9 << 9) ^ flags) & 0x200) ^ flags;
	record->mFlags = flags;
	current = flags & 0x1f;
	const NxU32 bit10 = (current == 3 || current == 2 || current == 5) ? 1 : 0;
	record->mFlags = ((bit10 & 1) << 10) | (flags & 0xfff8021f);
	}

// The spring tail (0x9fa56-0x9fa7f, 0x9fd08-0x9fd31): fill +0x30..+0x4c
// and hand the two spring gains to row 004393 (JointSupportRecord::row004393).
static void distanceSpringTail(JointSupportRecord* record, Joint* joint, NxReal error, NxReal maxForce,
	NxReal scale, NxReal ratio)
	{
	record->mUnknown034 = error;
	record->mUnknown048 = maxForce;
	record->mUnknown038 = 0.0f;
	record->mUnknown044 = 0;
	record->mUnknown04c = 0;
	record->mUnknown030 = joint;
	record->row004393(scale, ratio);
	}

// phys_fn_004230 (0x0009f1c0, 109 B)
// A broken joint reports (code 1, line 0x42) and returns. Unlike the other
// families' saveToDesc rows the report has no FoundationSDK instance check
// (no int3 guard before the import call, 0x9f1c9-0x9f1d9): it is the static
// FoundationSDK::error. Otherwise the four family fields go to desc+0x6c..
// +0x80 (the spring as three words) and the row tail-jumps to the base part
// (row 004066, 0x9f228). The supplement decompile agrees with the listing.
void DistanceJoint::saveToDesc(NxDistanceJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_DISTANCEJOINT_CPP, 0x42, 0,
			"DistanceJoint::saveToDesc: joint is broken. Broken joints can't be saved!");
		return;
		}
	desc.maxDistance = mMaxDistance;
	desc.minDistance = mMinDistance;
	desc.spring = mSpring;
	desc.flags = mDistanceFlags;
	saveToDescBase(desc);
	}

// phys_fn_004232 (0x0009f230, 564 B)
// Debug visualization. Gated only by SDK parameter 32 (world axes,
// 0x10123b98) or, failing that, 31 (local axes, 0x10123b94) being non-zero
// (`fucompp; test ah,0x44; jp`: a NaN counts as non-zero); unlike the other
// families' slot 4 there is no NX_JF_VISUALIZATION (+0x2c bit 9) test and no
// scale. After the stale-body refresh, one line between the two world
// anchors in 0xf0f0f0 through addLine (the renderable's slot +0x20,
// 0x9f45a). Listing over decompile: the supplement decompile sums the
// anchors in index order; the listing groups them as distanceWorldAnchor
// does.
void DistanceJoint::row_slot4(NxDebugRenderable& renderable)
	{
	if(jointLinearSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES) == 0.0f &&
		jointLinearSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) == 0.0f)
		return;

	distanceRefreshFirstStaleBody(*this);

	NxVec3 anchor0;
	distanceWorldAnchor(distanceBody(mBody[0]), mWorldAnchor[0], anchor0);
	NxVec3 anchor1;
	distanceWorldAnchor(distanceBody(mBody[1]), mWorldAnchor[1], anchor1);
	renderable.addLine(anchor0, anchor1, 0xf0f0f0);
	}

// phys_fn_004234 (0x0009f470, 162 B)
// Joint(desc, 0x2000) runs first (`push 0x2000` at 0x9f476: the type bit);
// the compiler then stores the vptr 0x10119948 (0x9f483) and runs the spring
// member's inline NxSpringDesc(), which zeroes +0x174..+0x17c
// (0x9f489-0x9f49d). The public object is allocated through the SDK
// allocator (`push 0; push 0x1c; call [edx+8]`) and constructed only when
// the allocation succeeded, but desc.userData is written to it without a
// null check (0x9f4ca-0x9f4cd): a failed allocation faults there in the
// oracle, and does here too. Then the four family fields are copied from
// the descriptor (0x9f4d0-0x9f506).
DistanceJoint::DistanceJoint(const NxDistanceJointDesc& desc)
	: Joint(desc, 0x2000)
	{
	void* memory = nxGetSdkAllocator()->malloc(sizeof(NpDistanceJoint), NX_MEMORY_PERSISTENT);
	NpDistanceJoint* publicJoint = memory ? new(memory) NpDistanceJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	mMaxDistance = desc.maxDistance;
	mMinDistance = desc.minDistance;
	mSpring = desc.spring;
	mDistanceFlags = desc.flags;
	}

// phys_fn_004236 (0x0009f520, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x10119948, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`), calls the Joint destructor body
// (row 004095) directly, and frees `this` through the SDK allocator (slot
// +0x14) when the flag's bit 0 is set (Joint::operator delete).
DistanceJoint::~DistanceJoint()
	{
	if(mPublicObject)
		delete static_cast<NpDistanceJoint*>(mPublicObject);
	}

// phys_fn_004238 (0x0009f560, 188 B)
// Unlike the other families' loadFromDesc rows there is no broken-joint
// test: only desc.isValid(), the descriptor's virtual (`call [eax+8]`,
// 0x9f56c), whose failure reports line 0x2a with no FoundationSDK instance
// check (the static error, 0x9f573-0x9f583). The actors are re-bound
// (phys_fn_004107 with suppressAttach false) only when a body differs from
// the one held; the second body is not looked at when the first already
// differs. Then the base part (004121) and the four family fields
// (0x9f5e0-0x9f612).
void DistanceJoint::loadFromDesc(const NxDistanceJointDesc& desc)
	{
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_DISTANCEJOINT_CPP, 0x2a, 0,
			"DistanceJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	void* actorImpl0 = distanceActorImpl(desc.actor[0]);
	void* actorImpl1 = distanceActorImpl(desc.actor[1]);
	if(distanceBodyOfActorImpl(actorImpl0) != mBody[0] || distanceBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	mMaxDistance = desc.maxDistance;
	mMinDistance = desc.minDistance;
	mSpring = desc.spring;
	mDistanceFlags = desc.flags;
	}

// phys_fn_004240 (0x0009f620, 2747 B)
// The distance solver slot. After the stale-body refresh, per body the
// lever r = R * worldAnchor, the world point p = r + t and the support
// record (distanceLever). d = p0 - p1: x and y stored, z stored while its
// unrounded copy multiplies the stored value (`fst; fmul`, 0x9f870); dist =
// sqrt((dz' dz + dy dy) + dx dx) is stored, and when it is not zero (a NaN
// counts as non-zero, `test ah,0x44; jnp`) d is scaled by 1 / dist (kept)
// and stored again. invArg = 1 / arg is stored (0x9f8cf-0x9f8d9).
// - The rigid arm: maxDistance == minDistance (ordered; `fucompp; jp`) and
//   both limit flags set. e = (dist - max) invArg; one record along d, kind 1.
// - Otherwise the max arm: flag bit 0 and dist > max (ordered; `fcomp; test
//   ah,0x41; jne`). e = (max - dist) invArg; one record along -d (each
//   component negated and stored), kind 0.
// - Only when the max arm is not taken, the min arm: flag bit 1 and
//   dist < min (`fcomp; test ah,5; jp`). e = (dist - min) invArg; one record
//   along d, kind 0. Neither arm: return with no record.
// In every arm maxForce (+0x3c) is read before row 004093 hands out the
// record. With flag bit 2 (the spring) q = arg * spring + damper (kept),
// ratio = (spring / q) * arg and scale = 1 / (q * arg) are stored, and the
// record goes to row 004393 (distanceSpringTail); without it the
// jointSolveRecord tail (0xa006d-0xa00cf) runs, whose first 004391 output is
// the argument slot holding maxForce (0xa008b), which is dead afterwards.
// Listing over decompile: the decompile shows every intermediate as a float
// and drops the kind tests as unreachable (0x9fa16, 0x9fb54, 0x9fcf5, 0xa0048,
// 0xa0051, 0xa0058); the listing keeps p0.x, p1.x, dz', 1 / dist and q on the
// FPU stack and tests the kinds.
void DistanceJoint::row_slot6(NxReal arg)
	{
	distanceRefreshFirstStaleBody(*this);

	JointSupportBody* record0;
	NxVec3 r0;
	double p0x;
	NxReal p0y;
	NxReal p0z;
	distanceLever(distanceBody(mBody[0]), mWorldAnchor[0], record0, r0, p0x, p0y, p0z);

	JointSupportBody* record1;
	NxVec3 r1;
	double p1x;
	NxReal p1y;
	NxReal p1z;
	distanceLever(distanceBody(mBody[1]), mWorldAnchor[1], record1, r1, p1x, p1y, p1z);

	// d, its length and the unit direction (0x9f854-0x9f8cd).
	NxVec3 d;
	d.x = (NxReal)(p0x - p1x);
	d.y = (NxReal)((double)p0y - p1y);
	const double dzUnrounded = (double)p0z - p1z;
	d.z = (NxReal)dzUnrounded;
	const NxReal dist = (NxReal)sqrt((dzUnrounded * d.z + distanceMul(d.y, d.y)) + distanceMul(d.x, d.x));
	if(dist != 0.0f)
		{
		const double inverse = 1.0f / (double)dist;
		d.x = (NxReal)((double)d.x * inverse);
		d.y = (NxReal)((double)d.y * inverse);
		d.z = (NxReal)((double)d.z * inverse);
		}
	const NxReal invArg = (NxReal)(1.0f / (double)arg);

	if(mMinDistance == mMaxDistance && (mDistanceFlags & 3) == 3)
		{
		// The rigid arm (0x9f90a-0x9fb97).
		const NxReal error = (NxReal)(((double)dist - mMaxDistance) * invArg);
		if(mDistanceFlags & NX_DJF_SPRING_ENABLED)
			{
			const double q = (double)arg * mSpring.spring + mSpring.damper;
			const NxReal maxForce = mMaxForce;
			const NxReal ratio = (NxReal)(((double)mSpring.spring / q) * arg);
			const NxReal scale = (NxReal)(1.0f / (q * arg));
			JointSupportRecord* record = row004093();
			distanceRecord(record, record0, record1, d, r0, r1);
			distanceRecordBits(record, 0xffffffe1, 1);
			distanceSpringTail(record, this, error, maxForce, scale, ratio);
			return;
			}
		const NxReal maxForce = mMaxForce;
		JointSupportRecord* record = row004093();
		distanceRecord(record, record0, record1, d, r0, r1);
		distanceRecordBits(record, 0xffffffe1, 1);
		jointSolveRecord(record, this, error, maxForce);
		return;
		}

	const NxU32 flags = mDistanceFlags;
	if((flags & NX_DJF_MAX_DISTANCE_ENABLED) && dist > mMaxDistance)
		{
		// The max arm (0x9fbc0-0x9fe3b), along -d.
		const NxReal error = (NxReal)(((double)mMaxDistance - dist) * invArg);
		const NxReal maxForce = mMaxForce;
		if(flags & NX_DJF_SPRING_ENABLED)
			{
			const double q = (double)arg * mSpring.spring + mSpring.damper;
			NxVec3 m;
			m.x = -d.x;
			m.y = -d.y;
			m.z = -d.z;
			const NxReal ratio = (NxReal)(((double)mSpring.spring / q) * arg);
			const NxReal scale = (NxReal)(1.0f / (q * arg));
			JointSupportRecord* record = row004093();
			distanceRecord(record, record0, record1, m, r0, r1);
			distanceRecordBits(record, 0xffffffe0, 0);
			distanceSpringTail(record, this, error, maxForce, scale, ratio);
			return;
			}
		NxVec3 m;
		m.x = -d.x;
		m.y = -d.y;
		m.z = -d.z;
		JointSupportRecord* record = row004093();
		distanceRecord(record, record0, record1, m, r0, r1);
		distanceRecordBits(record, 0xffffffe0, 0);
		jointSolveRecord(record, this, error, maxForce);
		return;
		}

	if(!((flags & NX_DJF_MIN_DISTANCE_ENABLED) && dist < mMinDistance))
		return;

	// The min arm (0x9fe5e-0xa0032), along d.
	const NxReal error = (NxReal)(((double)dist - mMinDistance) * invArg);
	const NxReal maxForce = mMaxForce;
	if(flags & NX_DJF_SPRING_ENABLED)
		{
		const double q = (double)arg * mSpring.spring + mSpring.damper;
		const NxReal ratio = (NxReal)(((double)mSpring.spring / q) * arg);
		const NxReal scale = (NxReal)(1.0f / (q * arg));
		JointSupportRecord* record = row004093();
		distanceRecord(record, record0, record1, d, r0, r1);
		distanceRecordBits(record, 0xffffffe0, 0);
		distanceSpringTail(record, this, error, maxForce, scale, ratio);
		return;
		}
	JointSupportRecord* record = row004093();
	distanceRecord(record, record0, record1, d, r0, r1);
	distanceRecordBits(record, 0xffffffe0, 0);
	jointSolveRecord(record, this, error, maxForce);
	}
