/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/PulleyJoint.h"
#include "core/NpPulleyJoint.h"
#include "core/JointSupport.h"
#include "X87Sqrt.h"
#include "core/JointLinearRecords.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>

// The oracle's __FILE__ for this unit (every report in it pushes the string
// at 0x10119878).
#define NX_PULLEYJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\PulleyJoint.cpp"

// Joint-families Task 3g. Floating point follows core/DistanceJoint.cpp: this
// translation unit is x87 in the oracle and is built /arch:IA32 here; a value
// the listing keeps on the FPU stack is a `double`, a value it stores (fstp
// dword) is an `NxReal`, and the listing's operand grouping and order are
// kept. PulleyJoint is constructed by Scene::createJoint's pulley case
// (NxJointType 7). No row is shared with another family. See
// units/joint-families-contract.md "## Pulley".

static NX_INLINE double pulleyMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

static NX_INLINE JointBodyRecord* pulleyBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* pulleyActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* pulleyBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// The inlined loop 004221 and 004228 open with (0x9e845-0x9e872,
// 0x9eb98-0x9ebc2; the same loop as distance's 004232/004240): refresh the
// first body whose stamp no longer matches, and only that one.
static void pulleyRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = pulleyBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// One world anchor for 004221 (0x9e877-0x9e949 for body 0, 0x9e94d-0x9ea20
// for body 1), distance's grouping: R * a + t through the body's
// +0x134/+0x158 pose, x summed as ((R2 a.z + R1 a.y) + R0 a.x) and kept
// until t.x is added, y and z stored before t is added; the stored anchor
// without a body.
static void pulleyWorldAnchor(const JointBodyRecord* body, const NxVec3& a, NxVec3& out)
	{
	if(!body)
		{
		out = a;
		return;
		}
	const NxReal* R = body->mUnknown134;
	const NxVec3& t = body->mUnknown158;
	const double x = (pulleyMul(R[2], a.z) + pulleyMul(R[1], a.y)) + pulleyMul(R[0], a.x);
	const NxReal y = (NxReal)((pulleyMul(R[5], a.z) + pulleyMul(R[4], a.y)) + pulleyMul(R[3], a.x));
	const NxReal z = (NxReal)((pulleyMul(R[8], a.z) + pulleyMul(R[7], a.y)) + pulleyMul(R[6], a.x));
	out.x = (NxReal)(x + t.x);
	out.y = (NxReal)((double)y + t.y);
	out.z = (NxReal)((double)z + t.z);
	}

// One support record's part of 004228's effective mass (0x9ef51-0x9f00b for
// record 0, 0x9f01d-0x9f0d7 for record 1): w = I * cross through the
// record's +0x20 3x3, rows ((c.z I2 + c.y I1) + c.x I0), ((c.x I3 + c.z I5) +
// c.y I4) and ((c.x I6 + c.z I8) + c.y I7), and u = direction * the
// record's +0x0c scale, all stored.
static void pulleyMassTerms(const JointSupportBody* support, const NxVec3& cross, const NxVec3& direction,
	NxVec3& w, NxVec3& u)
	{
	const NxReal* I = support->mUnknown020;
	const NxReal m = support->mUnknown00c;
	w.x = (NxReal)((pulleyMul(cross.z, I[2]) + pulleyMul(cross.y, I[1])) + pulleyMul(cross.x, I[0]));
	w.y = (NxReal)((pulleyMul(cross.x, I[3]) + pulleyMul(cross.z, I[5])) + pulleyMul(cross.y, I[4]));
	w.z = (NxReal)((pulleyMul(cross.x, I[6]) + pulleyMul(cross.z, I[8])) + pulleyMul(cross.y, I[7]));
	u.x = (NxReal)pulleyMul(direction.x, m);
	u.y = (NxReal)pulleyMul(direction.y, m);
	u.z = (NxReal)pulleyMul(direction.z, m);
	}

// One support record's update in 004219 (0x9e5d5-0x9e6dd for record 0,
// 0x9e6ef-0x9e7fd for record 1). impulse * direction: x and z stored, y kept;
// impulse * cross stored. Only when the record's +0x0c scale m is non-zero
// (`fucompp; test ah,0x44; jnp`, a NaN counts as non-zero): the linear part
// +0x00 gains (i.x m) (kept), (NxReal)(i.y m) and (NxReal)(i.z m); the
// angular part +0x10 gains I * (impulse * cross), rows ((s.z I2 + s.y I1) +
// s.x I0) and so on, x and y kept, z stored. A record with m == 0 gets
// nothing, the angular part included.
static void pulleyApplyImpulse(JointSupportBody* support, NxReal impulse, const NxVec3& direction,
	const NxVec3& cross)
	{
	const NxReal ix = (NxReal)pulleyMul(impulse, direction.x);
	const double iy = pulleyMul(impulse, direction.y);
	const NxReal iz = (NxReal)pulleyMul(impulse, direction.z);
	NxVec3 s;
	s.x = (NxReal)pulleyMul(cross.x, impulse);
	s.y = (NxReal)pulleyMul(cross.y, impulse);
	s.z = (NxReal)pulleyMul(cross.z, impulse);
	const NxReal m = support->mUnknown00c;
	if(m == 0.0f)
		return;
	const double lx = pulleyMul(ix, m);
	const NxReal ly = (NxReal)(iy * m);
	const NxReal lz = (NxReal)pulleyMul(iz, m);
	support->mUnknown000.x = (NxReal)(lx + support->mUnknown000.x);
	support->mUnknown000.y = (NxReal)((double)ly + support->mUnknown000.y);
	support->mUnknown000.z = (NxReal)((double)lz + support->mUnknown000.z);
	const NxReal* I = support->mUnknown020;
	const double ax = (pulleyMul(s.z, I[2]) + pulleyMul(s.y, I[1])) + pulleyMul(s.x, I[0]);
	const double ay = (pulleyMul(s.z, I[5]) + pulleyMul(s.y, I[4])) + pulleyMul(s.x, I[3]);
	const NxReal az = (NxReal)((pulleyMul(s.z, I[8]) + pulleyMul(s.y, I[7])) + pulleyMul(s.x, I[6]));
	support->mUnknown010.x = (NxReal)(ax + support->mUnknown010.x);
	support->mUnknown010.y = (NxReal)(ay + support->mUnknown010.y);
	support->mUnknown010.z = (NxReal)((double)az + support->mUnknown010.z);
	}

// phys_fn_004214 (0x0009e3c0, 11 B)
// `mov dword ptr [ecx+0x1cc], 0; ret`.
void PulleyJoint::row_slot1()
	{
	mBias = 0.0f;
	}

// phys_fn_004216 (0x0009e3d0, 155 B)
// As distance's 004230: a broken joint reports (code 1, line 0x42) through
// the static FoundationSDK::error (no instance check, no int3 guard before
// the import call, 0x9e3d9-0x9e3e9) and returns. Otherwise the ten family
// words go to desc+0x6c..+0x90 in address order and the row tail-jumps to
// the base part (row 004066, 0x9e466). The supplement decompile agrees with
// the listing.
void PulleyJoint::saveToDesc(NxPulleyJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_PULLEYJOINT_CPP, 0x42, 0,
			"PulleyJoint::saveToDesc: joint is broken. Broken joints can't be saved!");
		return;
		}
	desc.pulley[0] = mPulley[0];
	desc.pulley[1] = mPulley[1];
	desc.distance = mDistance;
	desc.stiffness = mStiffness;
	desc.ratio = mRatio;
	desc.flags = mPulleyFlags;
	saveToDescBase(desc);
	}

// phys_fn_004218 (0x0009e470, 112 B)
// A thiscall row of its own (`ret 4`) in the oracle, called by the
// constructor (0x9eaa5) and loadFromDesc (0x9eb83); kept out of line so it
// stays one. Copies desc+0x6c..+0x90 to +0x16c..+0x190 word by word.
__declspec(noinline) void PulleyJoint::copyFamilyFields(const NxPulleyJointDesc& desc)
	{
	mPulley[0] = desc.pulley[0];
	mPulley[1] = desc.pulley[1];
	mDistance = desc.distance;
	mStiffness = desc.stiffness;
	mRatio = desc.ratio;
	mPulleyFlags = desc.flags;
	}

// phys_fn_004219 (0x0009e4e0, 805 B)
// Slot 0, `ret 4`; the argument is never read. S sums, over the support
// records present, the record's velocity against this row's direction and
// cross: record 0 as ((((w.z c0.z + w.y c0.y) + v.z d0.z) + v.y d0.y) + w.x
// c0.x) + v.x d0.x, record 1 with v.x d1.x before w.x c1.x, S = S0 + S1 (0
// without records). lambda = -S * mInverseMass is kept; mAccumulated[1] +=
// lambda. impulse = lambda - mScaledInverseMass * mBias is stored;
// mAccumulated[0] += impulse. When impulse is not zero (a NaN counts as
// non-zero) each present record takes it along its direction and cross
// (pulleyApplyImpulse); body 1 takes +impulse too, its direction already
// pointing the other way.
// Listing over decompile: the supplement decompile shows every intermediate
// as a float; the listing keeps S, lambda, i.y and the linear and angular x
// and y updates on the FPU stack.
void PulleyJoint::row_slot0(NxU32 arg)
	{
	(void)arg;
	JointSupportBody* support0 = mSupport[0];
	double velocity;
	if(support0)
		{
		const NxVec3& v = support0->mUnknown000;
		const NxVec3& w = support0->mUnknown010;
		velocity = ((((pulleyMul(w.z, mCross[0].z) + pulleyMul(w.y, mCross[0].y)) + pulleyMul(v.z, mDirection[0].z))
			+ pulleyMul(v.y, mDirection[0].y)) + pulleyMul(w.x, mCross[0].x)) + pulleyMul(v.x, mDirection[0].x);
		}
	else
		{
		velocity = 0.0f;
		}
	JointSupportBody* support1 = mSupport[1];
	if(support1)
		{
		const NxVec3& v = support1->mUnknown000;
		const NxVec3& w = support1->mUnknown010;
		const double velocity1 = ((((pulleyMul(w.z, mCross[1].z) + pulleyMul(w.y, mCross[1].y))
			+ pulleyMul(v.z, mDirection[1].z)) + pulleyMul(v.y, mDirection[1].y)) + pulleyMul(v.x, mDirection[1].x))
			+ pulleyMul(w.x, mCross[1].x);
		velocity = velocity + velocity1;
		}

	const double lambda = -velocity * mInverseMass;
	mAccumulated[1] = (NxReal)(lambda + mAccumulated[1]);
	const NxReal impulse = (NxReal)(lambda - pulleyMul(mScaledInverseMass, mBias));
	mAccumulated[0] = (NxReal)((double)impulse + mAccumulated[0]);
	if(impulse == 0.0f)
		return;

	if(support0)
		pulleyApplyImpulse(support0, impulse, mDirection[0], mCross[0]);
	support1 = mSupport[1];
	if(support1)
		pulleyApplyImpulse(support1, impulse, mDirection[1], mCross[1]);
	}

// phys_fn_004221 (0x0009e810, 592 B)
// Debug visualization with distance's gate: SDK parameter 32 (world axes,
// 0x10123b98) or, failing that, 31 (local axes, 0x10123b94) non-zero (a NaN
// counts as non-zero); no NX_JF_VISUALIZATION test and no scale. After the
// stale-body refresh, the two world anchors (pulleyWorldAnchor), then one
// 0xf0f0f0 line from each anchor to its pulley point through addLine (the
// renderable's slot +0x20, 0x9ea3d and 0x9ea55).
// Listing over decompile: the supplement decompile passes the first anchor
// to both calls; the listing passes the second anchor (`lea eax,[esp+0x34]`,
// 0x9ea4e) to the second.
void PulleyJoint::row_slot4(NxDebugRenderable& renderable)
	{
	if(jointLinearSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES) == 0.0f &&
		jointLinearSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) == 0.0f)
		return;

	pulleyRefreshFirstStaleBody(*this);

	NxVec3 anchor0;
	pulleyWorldAnchor(pulleyBody(mBody[0]), mWorldAnchor[0], anchor0);
	NxVec3 anchor1;
	pulleyWorldAnchor(pulleyBody(mBody[1]), mWorldAnchor[1], anchor1);
	renderable.addLine(anchor0, mPulley[0], 0xf0f0f0);
	renderable.addLine(anchor1, mPulley[1], 0xf0f0f0);
	}

// phys_fn_004222 (0x0009ea60, 81 B)
// Joint(desc, 0x1000) runs first (`push 0x1000` at 0x9ea66: the type bit);
// the compiler then stores the vptr 0x10119840 (0x9ea73). The public object
// is allocated through the Foundation allocator (`push 0; push 0x1c; call [edx+8]`)
// and constructed only when the allocation succeeded, but desc.userData is
// written to it without a null check (0x9ea9c-0x9ea9f): a failed allocation
// faults there in the oracle, and does here too. Then copyFamilyFields
// (row 004218, 0x9eaa5). The solver state +0x194..+0x1dc is not
// initialised.
PulleyJoint::PulleyJoint(const NxPulleyJointDesc& desc)
	: Joint(desc, 0x1000)
	{
	void* memory = nxFoundationSDKAllocator->malloc(sizeof(NpPulleyJoint), NX_MEMORY_PERSISTENT);
	NpPulleyJoint* publicJoint = memory ? new(memory) NpPulleyJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	copyFamilyFields(desc);
	}

// phys_fn_004224 (0x0009eac0, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x10119840, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`), calls the Joint destructor body
// (row 004095) directly, and frees `this` through the Foundation allocator (slot
// +0x14) when the flag's bit 0 is set (Joint::operator delete).
PulleyJoint::~PulleyJoint()
	{
	if(mPublicObject)
		delete static_cast<NpPulleyJoint*>(mPublicObject);
	}

// phys_fn_004226 (0x0009eb00, 141 B)
// As distance's 004238: no broken-joint test, only desc.isValid() (the
// descriptor's virtual, `call [eax+8]`, 0x9eb0c), whose failure reports line
// 0x2a through the static error with no instance check (0x9eb13-0x9eb23).
// The actors are re-bound (phys_fn_004107 with suppressAttach false) only
// when a body differs from the one held; the second body is not looked at
// when the first already differs. Then the base part (004121) and
// copyFamilyFields (004218, 0x9eb83), where distance copies inline.
void PulleyJoint::loadFromDesc(const NxPulleyJointDesc& desc)
	{
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_PULLEYJOINT_CPP, 0x2a, 0,
			"PulleyJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	void* actorImpl0 = pulleyActorImpl(desc.actor[0]);
	void* actorImpl1 = pulleyActorImpl(desc.actor[1]);
	if(pulleyBodyOfActorImpl(actorImpl0) != mBody[0] || pulleyBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	copyFamilyFields(desc);
	}

// phys_fn_004228 (0x0009eb90, 1572 B)
// The pulley solver slot (arg = the step divisor). After the stale-body
// refresh, per body i:
// - with a body, the lever R * a[i] is stored into lever[1] (the fixed
//   stack slots [esp+0x4c..0x54], 0x9ec09-0x9ec4d), x ((R2 a.z + R1 a.y) +
//   R0 a.x), y ((R5 a.z + R3 a.x) + R4 a.y), z ((R8 a.z + R6 a.x) + R7 a.y),
//   while the world point reads lever[i] ([esp+edx+0x40], 0x9ec51): for a
//   dynamic body 0 the point and the first cross use lever[0], which this
//   row never writes (the oracle reads that stack slot as it finds it; the
//   reconstruction reads its own uninitialised slot). point = lever + t,
//   stored;
// - without a body, lever[i] = point[i] = the stored anchor;
// - dir[i] = pulley[i] - point[i] (stored), len = fsqrt((x x + z z) + y y)
//   kept while dir is scaled by 1 / len when len is not zero (a NaN counts
//   as non-zero), then stored.
// mBias = (distance - (len0 + len1 ratio)) (stiffness / arg), rounded once.
// mSupport = the bodies' +0x204 records. One record from row 004093 (kind
// 6, bit 5 set, this at +0x30, the support records at +0x10/+0x14, every
// vector and float zero): it carries no direction, slot 0 applies the
// impulse itself. mDirection = dir; mCross[i] = lever[i] x dir[i]; the
// impulse sums are zeroed. K = K0 + K1 over the present records
// (pulleyMassTerms; K1 adds w.x c.x before u.x d.x, K0 after), mInverseMass
// = 1 / K or 0 (a NaN counts as non-zero), stored with `fst`, and
// mScaledInverseMass = the unrounded value * 0.7f (0x10106940).
// Listing over decompile: the manifest decompile names the stack arrays and
// so hides the lever[1] store; it shows every intermediate as a float.
void PulleyJoint::row_slot6(NxReal arg)
	{
	pulleyRefreshFirstStaleBody(*this);

	NxReal length[2];
	NxVec3 lever[2];
	NxVec3 direction[2];
	NxVec3 point[2];
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = pulleyBody(mBody[i]);
		const NxVec3& a = mWorldAnchor[i];
		if(body)
			{
			const NxReal* R = body->mUnknown134;
			const NxVec3& t = body->mUnknown158;
			lever[1].x = (NxReal)((pulleyMul(R[2], a.z) + pulleyMul(R[1], a.y)) + pulleyMul(R[0], a.x));
			lever[1].y = (NxReal)((pulleyMul(R[5], a.z) + pulleyMul(R[3], a.x)) + pulleyMul(R[4], a.y));
			lever[1].z = (NxReal)((pulleyMul(R[8], a.z) + pulleyMul(R[6], a.x)) + pulleyMul(R[7], a.y));
			point[i].x = (NxReal)((double)lever[i].x + t.x);
			point[i].y = (NxReal)((double)lever[i].y + t.y);
			point[i].z = (NxReal)((double)t.z + lever[i].z);
			}
		else
			{
			lever[i] = a;
			point[i] = a;
			}
		direction[i].x = (NxReal)((double)mPulley[i].x - point[i].x);
		direction[i].y = (NxReal)((double)mPulley[i].y - point[i].y);
		direction[i].z = (NxReal)((double)mPulley[i].z - point[i].z);
		const double len = x87FsqrtDot3(direction[i].x, direction[i].x, direction[i].z, direction[i].z,
			direction[i].y, direction[i].y);
		if(len != 0.0f)
			{
			const double inverse = 1.0f / len;
			direction[i].x = (NxReal)(inverse * direction[i].x);
			direction[i].y = (NxReal)(inverse * direction[i].y);
			direction[i].z = (NxReal)(inverse * direction[i].z);
			}
		length[i] = (NxReal)len;
		}

	mBias = (NxReal)(((double)mDistance - ((double)length[0] + pulleyMul(length[1], mRatio)))
		* ((double)mStiffness / arg));

	const JointBodyRecord* body0 = pulleyBody(mBody[0]);
	JointSupportBody* support0 = body0 ? body0->mUnknown204 : 0;
	mSupport[0] = support0;
	const JointBodyRecord* body1 = pulleyBody(mBody[1]);
	JointSupportBody* support1 = body1 ? body1->mUnknown204 : 0;
	mSupport[1] = support1;

	// The record (0x9eda9-0x9ee52): kind 6 and bit 5, bit 9 = (kind is 0 or
	// 2) and bit 10 = (kind is 3, 2 or 5) through xors, everything else
	// zeroed, then bits 6-8 and 11-18 cleared. The kind is known, but the
	// listing tests it, so the tests stay.
	JointSupportRecord* record = row004093();
	NxU32 flags = (record->mFlags & 0xffffffe6) | 0x26;
	record->mUnknown030 = this;
	record->mFlags = flags;
	NxU32 kind = flags & 0x1f;
	const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
	flags = (((bit9 << 9) ^ flags) & 0x200) ^ flags;
	kind = flags & 0x1f;
	record->mFlags = flags;
	const NxU32 bit10 = (kind == 3 || kind == 2 || kind == 5) ? 1 : 0;
	record->mBody[0] = support0;
	record->mBody[1] = support1;
	record->mUnknown000.z = 0.0f;
	record->mUnknown000.y = 0.0f;
	record->mUnknown000.x = 0.0f;
	record->mFlags = (((bit10 << 10) ^ flags) & 0x400) ^ flags;
	record->mUnknown018.z = 0.0f;
	record->mUnknown018.y = 0.0f;
	record->mUnknown018.x = 0.0f;
	record->mUnknown024.z = 0.0f;
	record->mUnknown024.y = 0.0f;
	record->mUnknown024.x = 0.0f;
	record->mUnknown034 = 0.0f;
	record->mUnknown038 = 0.0f;
	record->mUnknown04c = 0;
	record->mUnknown048 = 0.0f;
	record->mUnknown040 = 0.0f;
	record->mUnknown03c = 0.0f;
	record->mFlags &= 0xfff8063f;

	mDirection[0] = direction[0];
	mDirection[1] = direction[1];

	// The crosses (0x9ee8d-0x9ef31), each component a product minus a
	// product in the listing's operand order.
	mCross[0].x = (NxReal)(pulleyMul(lever[0].y, mDirection[0].z) - pulleyMul(lever[0].z, mDirection[0].y));
	mCross[0].y = (NxReal)(pulleyMul(lever[0].z, mDirection[0].x) - pulleyMul(lever[0].x, mDirection[0].z));
	mCross[0].z = (NxReal)(pulleyMul(lever[0].x, mDirection[0].y) - pulleyMul(lever[0].y, mDirection[0].x));
	mCross[1].x = (NxReal)(pulleyMul(lever[1].y, mDirection[1].z) - pulleyMul(lever[1].z, mDirection[1].y));
	mCross[1].y = (NxReal)(pulleyMul(lever[1].z, mDirection[1].x) - pulleyMul(lever[1].x, mDirection[1].z));
	mCross[1].z = (NxReal)(pulleyMul(lever[1].x, mDirection[1].y) - pulleyMul(lever[1].y, mDirection[1].x));

	mAccumulated[0] = 0.0f;
	mAccumulated[1] = 0.0f;

	// The effective mass (0x9ef37-0x9f177).
	NxVec3 w0;
	NxVec3 u0;
	if(mSupport[0])
		pulleyMassTerms(mSupport[0], mCross[0], mDirection[0], w0, u0);
	NxVec3 w1;
	NxVec3 u1;
	if(mSupport[1])
		pulleyMassTerms(mSupport[1], mCross[1], mDirection[1], w1, u1);
	double mass;
	if(mSupport[0])
		mass = ((((pulleyMul(u0.y, mDirection[0].y) + pulleyMul(u0.z, mDirection[0].z)) + pulleyMul(w0.z, mCross[0].z))
			+ pulleyMul(w0.y, mCross[0].y)) + pulleyMul(u0.x, mDirection[0].x)) + pulleyMul(w0.x, mCross[0].x);
	else
		mass = 0.0f;
	if(mSupport[1])
		{
		const double mass1 = ((((pulleyMul(u1.y, mDirection[1].y) + pulleyMul(u1.z, mDirection[1].z))
			+ pulleyMul(w1.z, mCross[1].z)) + pulleyMul(w1.y, mCross[1].y)) + pulleyMul(w1.x, mCross[1].x))
			+ pulleyMul(u1.x, mDirection[1].x);
		mass = mass + mass1;
		}
	double inverse;
	if(mass != 0.0f)
		inverse = 1.0f / mass;
	else
		inverse = 0.0f;
	mInverseMass = (NxReal)inverse;
	mScaledInverseMass = (NxReal)(inverse * 0.7f);
	}
