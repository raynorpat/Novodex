/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/FixedJoint.h"
#include "core/NpFixedJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"

#include <new>

// The oracle's __FILE__ for this unit (every report in it pushes the string
// at 0x10119a84).
#define NX_FIXEDJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\FixedJoint.cpp"

// Joint-families Task 3h. Floating point follows core/PulleyJoint.cpp: this
// translation unit is x87 in the oracle and is built /arch:IA32 here; a value
// the listing keeps on the FPU stack is a `double`, a value it stores (fstp
// dword) is an `NxReal`, and the listing's operand grouping and order are
// kept. FixedJoint is constructed by Scene::createJoint's fixed case
// (NxJointType 8). No row is shared with another family; the table's slots 0,
// 4 and 8 are the folded empty body 004248 (see core/FixedJoint.h). See
// units/joint-families-contract.md "## Fixed".

static NX_INLINE double fixedMul(double a, double b)
	{
	return a * b;
	}

static NX_INLINE JointBodyRecord* fixedBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* fixedActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* fixedBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// The three angular records of 004246 (0xa0d01-0xa0d68 and its two copies):
// the two support records, a unit axis at +0x00 (copied from gJointUnitAxis),
// kind 3 ((flags & 0xffffffe3) | 3), then bit 9 = (kind is 0 or 2), bit 10
// set, bits 5-8 and 11-18 cleared -- prismatic's angular record
// (core/PrismaticJoint.cpp prismaticAngularRecord), instruction for
// instruction. +0x18/+0x24 are not written.
static void fixedAngularRecord(JointSupportRecord* record, JointSupportBody* body0, JointSupportBody* body1,
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

// phys_fn_004242 (0x000a00e0, 42 B)
// As pulley's 004216 without family words: a broken joint reports (code 1,
// line 0x42) through the static FoundationSDK::error (no instance check, no
// int3 guard before the import call, 0xa00e9-0xa00f9) and returns. Otherwise
// the row tail-jumps to the base part (row 004066, 0xa0105).
// NxFixedJointDesc has no field of its own. The supplement decompile agrees
// with the listing.
void FixedJoint::saveToDesc(NxFixedJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_FIXEDJOINT_CPP, 0x42, 0,
			"FixedJoint::saveToDesc: joint is broken. Broken joints can't be saved!");
		return;
		}
	saveToDescBase(desc);
	}

// phys_fn_004244 (0x000a0110, 723 B)
// A thiscall row of its own (`ret 4`) in the oracle, called by the
// constructor (0xa0fb5) and loadFromDesc (0xa1093); kept out of line so it
// stays one. The descriptor argument is never read.
// - d = body 1's +0x158 position minus body 0's. Without body 0 it is body
//   1's position, read with no null check (createJoint needs one dynamic
//   actor, so the two are never both missing); without body 1 it is minus
//   body 0's. d.x stays on the FPU stack, d.y and d.z are stored
//   (0xa014b/0xa0171, 0xa0185).
// - With body 0 (R its +0x134 3x3), d goes into body 0's frame through the
//   transpose: y ((d.z R7 + d.y R4) + d.x R1) and z ((d.z R8 + d.y R5) +
//   d.x R2) through stack slots, x ((d.z R6 + d.y R3) + d.x R0) stored last
//   (0xa0199-0xa0211). Without body 0, d is stored as it is.
// - mRelativeRotation = body 0's +0x124 quaternion (or (0,0,0,1)), its x, y
//   and z negated in place (fld/fchs/fstp). With body 1 (X, Y, Z, W its
//   +0x124 quaternion) it is multiplied on the right by that one, every
//   product read from the old values: w ((wW - xX) - Yy) - Zz and
//   x ((xW + Zy) + wX) - Yz kept, y ((Wy + Yw) + zX) - xZ through a stack
//   slot, z ((Wz + xY) + Zw) - yX stored; the stores go y, z, w, x
//   (0xa0397-0xa03ad). Then x, y and z are negated again in place.
// Prismatic's 004378 builds the same quaternion with other sum groupings.
// Listing over decompile: the decompile shows d.x as a float in the
// two-body case; the listing keeps the unrounded difference.
__declspec(noinline) void FixedJoint::recordRelativePose(const NxFixedJointDesc& desc)
	{
	(void)desc;
	const JointBodyRecord* const body0 = fixedBody(mBody[0]);
	const JointBodyRecord* const body1 = fixedBody(mBody[1]);

	double dx;
	NxReal dy;
	NxReal dz;
	if(!body0)
		{
		dx = body1->mUnknown158.x;
		dy = body1->mUnknown158.y;
		dz = body1->mUnknown158.z;
		}
	else if(!body1)
		{
		dx = -body0->mUnknown158.x;
		dy = -body0->mUnknown158.y;
		dz = -body0->mUnknown158.z;
		}
	else
		{
		dx = (double)body1->mUnknown158.x - body0->mUnknown158.x;
		dy = (NxReal)((double)body1->mUnknown158.y - body0->mUnknown158.y);
		dz = (NxReal)((double)body1->mUnknown158.z - body0->mUnknown158.z);
		}

	if(body0)
		{
		const NxReal* R = body0->mUnknown134;
		const NxReal y = (NxReal)((fixedMul(dz, R[7]) + fixedMul(dy, R[4])) + fixedMul(dx, R[1]));
		const NxReal z = (NxReal)((fixedMul(dz, R[8]) + fixedMul(dy, R[5])) + fixedMul(dx, R[2]));
		mRelativePosition.x = (NxReal)((fixedMul(dz, R[6]) + fixedMul(dy, R[3])) + fixedMul(dx, R[0]));
		mRelativePosition.y = y;
		mRelativePosition.z = z;
		}
	else
		{
		mRelativePosition.x = (NxReal)dx;
		mRelativePosition.y = dy;
		mRelativePosition.z = dz;
		}

	NxReal* q = mRelativeRotation;
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

	if(body1)
		{
		const NxReal* b = body1->mCMassOrientation;
		const double w = ((fixedMul(q[3], b[3]) - fixedMul(q[0], b[0])) - fixedMul(b[1], q[1])) - fixedMul(b[2], q[2]);
		const double x = ((fixedMul(q[0], b[3]) + fixedMul(b[2], q[1])) + fixedMul(q[3], b[0])) - fixedMul(b[1], q[2]);
		const NxReal y = (NxReal)(((fixedMul(b[3], q[1]) + fixedMul(b[1], q[3])) + fixedMul(q[2], b[0])) - fixedMul(q[0], b[2]));
		const NxReal z = (NxReal)(((fixedMul(b[3], q[2]) + fixedMul(q[0], b[1])) + fixedMul(b[2], q[3])) - fixedMul(q[1], b[0]));
		q[1] = y;
		q[2] = z;
		q[3] = (NxReal)w;
		q[0] = (NxReal)x;
		}

	q[0] = -q[0];
	q[1] = -q[1];
	q[2] = -q[2];
	}

// phys_fn_004246 (0x000a03f0, 2922 B)
// The fixed solver slot (arg = the step divisor). Unlike every other family's
// solver slot it does not open with the stale-body refresh. s0/s1 are the
// bodies' +0x204 support records (null without a body).
// - r = R0 * mRelativePosition (x ((R2 p.z + R1 p.y) + R0 p.x),
//   y ((R3 p.x + R5 p.z) + R4 p.y), z ((R6 p.x + R8 p.z) + R7 p.y), all
//   stored; 0xa042f-0xa04af), or mRelativePosition without body 0.
// - The linear error: e = r - gJointZeroVector (0xa04d5-0xa050b; e.x is
//   stored with `fst` and kept, e.y and e.z stored). With body 0, x = the
//   stored e.x + t0.x kept, y and z (e + t0) stored; with body 1, x - t1.x
//   kept, y and z (- t1) stored. Then inv = 1.0f / arg (`fdiv`, then `fst`
//   into the argument slot, 0xa055f-0xa0572): the x error is the unrounded
//   inverse times the kept x, y and z use the stored inverse; all stored.
// - Three kind-1 linear records along gJointUnitAxis[0..2]: jointLinearRecord
//   with r0 = r and r1 = gJointZeroVector (the listing forms the same
//   products and subtracts in the same order, storing +0x18/+0x24 x, y, z
//   where the helper stores y, z, x) and jointSolveRecord with maxForce.
// - The angular error: P = conj(q0) q1 (the bodies' +0x124 quaternions; the
//   identity for a missing body 0, conj(q0) alone for a missing body 1), each
//   component stored when body 1 is there (0xa0ab3-0xa0b57). Then
//   E = P * mRelativeRotation: w ((Pw Qw - Px Qx) - Py Qy) - Pz Qz,
//   x ((Py Qz + Px Qw) + Pw Qx) - Pz Qy and y ((Py Qw + Pw Qy) + Pz Qx) - Px Qz
//   stored, z ((Pw Qz + Px Qy) + Pz Qw) - Py Qx kept (0xa0b6b-0xa0c15). When
//   E.w < 0 (`fcomp 0.0f; test ah,5; jp`: a NaN is not less), x, y and z are
//   multiplied by -1.0f (0x1010687c); y is stored again.
// - With body 0, V = R0 * E: x ((E.y R1 + E.z R2) + E.x R0) and
//   y ((E.x R3 + E.y R4) + E.z R5) stored, z ((E.x R6 + E.y R7) + E.z R8)
//   kept (0xa0c54-0xa0cb2); without it E.x and E.y are stored and E.z kept.
// - The angular errors are ((stored inv) V) * -2.0f (0x10108748), each stored
//   (0xa0cc2-0xa0cfb), and three kind-3 angular records follow along
//   gJointUnitAxis[0..2] with maxTorque (the argument slot is reused for it,
//   0xa0cef).
// The listing passes 004391's write-only first output as a stack slot that
// is dead afterwards (maxForce's copy, then the y and z linear errors, then
// the argument slot holding maxTorque); jointSolveRecord passes a copy of
// the +0x48 value, which is equally dead. Listing over decompile: the
// decompile drops the kind tests of the record flags as unreachable (the
// helpers keep them) and shows the kept x values and E.z as floats.
void FixedJoint::row_slot6(NxReal arg)
	{
	JointBodyRecord* const body0 = fixedBody(mBody[0]);
	JointSupportBody* const record0 = body0 ? body0->mUnknown204 : 0;
	JointBodyRecord* const body1 = fixedBody(mBody[1]);
	JointSupportBody* const record1 = body1 ? body1->mUnknown204 : 0;

	// r (0xa0423-0xa04cf).
	NxVec3 r;
	if(body0)
		{
		const NxReal* R = body0->mUnknown134;
		const NxVec3& p = mRelativePosition;
		r.x = (NxReal)((fixedMul(R[2], p.z) + fixedMul(R[1], p.y)) + fixedMul(R[0], p.x));
		r.y = (NxReal)((fixedMul(R[3], p.x) + fixedMul(R[5], p.z)) + fixedMul(R[4], p.y));
		r.z = (NxReal)((fixedMul(R[6], p.x) + fixedMul(R[8], p.z)) + fixedMul(R[7], p.y));
		}
	else
		r = mRelativePosition;

	// The linear error (0xa04d3-0xa0592).
	const NxVec3& zero = gJointZeroVector;
	const double ex = (double)r.x - zero.x;
	const NxReal exStored = (NxReal)ex;
	const NxReal eyStored = (NxReal)((double)r.y - zero.y);
	const NxReal ezStored = (NxReal)((double)r.z - zero.z);
	double gx;
	NxReal gy;
	NxReal gz;
	if(body0)
		{
		gx = (double)exStored + body0->mUnknown158.x;
		gy = (NxReal)((double)eyStored + body0->mUnknown158.y);
		gz = (NxReal)((double)ezStored + body0->mUnknown158.z);
		}
	else
		{
		gx = ex;
		gy = eyStored;
		gz = ezStored;
		}
	if(body1)
		{
		gx = gx - body1->mUnknown158.x;
		gy = (NxReal)((double)gy - body1->mUnknown158.y);
		gz = (NxReal)((double)gz - body1->mUnknown158.z);
		}
	const double inverse = 1.0f / (double)arg;
	const NxReal inverseStored = (NxReal)inverse;
	const NxReal linearX = (NxReal)(inverse * gx);
	const NxReal linearY = (NxReal)fixedMul(inverseStored, gy);
	const NxReal linearZ = (NxReal)fixedMul(inverseStored, gz);

	// The three linear records (0xa0596-0xa0a49).
	JointSupportRecord* record = row004093();
	jointLinearRecord(record, record0, record1, gJointUnitAxis[0], r, zero);
	jointSolveRecord(record, this, linearX, mMaxForce);

	record = row004093();
	jointLinearRecord(record, record0, record1, gJointUnitAxis[1], r, zero);
	jointSolveRecord(record, this, linearY, mMaxForce);

	record = row004093();
	jointLinearRecord(record, record0, record1, gJointUnitAxis[2], r, zero);
	jointSolveRecord(record, this, linearZ, mMaxForce);

	// P = conj(q0) q1 (0xa0a4b-0xa0b67). a, b, c, w stand for the listing's
	// four stack registers.
	const JointBodyRecord* const pose0 = fixedBody(mBody[0]);
	double w;
	double a;
	double b;
	double c;
	if(pose0)
		{
		w = pose0->mCMassOrientation[3];
		a = -pose0->mCMassOrientation[0];
		b = -pose0->mCMassOrientation[1];
		c = -pose0->mCMassOrientation[2];
		}
	else
		{
		const NxReal zeroPart = 0.0f;
		w = 1.0f;
		a = -zeroPart;
		b = -zeroPart;
		c = -zeroPart;
		}
	const JointBodyRecord* const pose1 = fixedBody(mBody[1]);
	if(pose1)
		{
		const NxReal* Q1 = pose1->mCMassOrientation;
		const NxReal pw = (NxReal)(((fixedMul(w, Q1[3]) - fixedMul(a, Q1[0])) - fixedMul(b, Q1[1])) - fixedMul(c, Q1[2]));
		const NxReal px = (NxReal)(((fixedMul(w, Q1[0]) + fixedMul(b, Q1[2])) + fixedMul(a, Q1[3])) - fixedMul(c, Q1[1]));
		const NxReal py = (NxReal)(((fixedMul(c, Q1[0]) + fixedMul(b, Q1[3])) + fixedMul(w, Q1[1])) - fixedMul(a, Q1[2]));
		const NxReal pz = (NxReal)(((fixedMul(w, Q1[2]) + fixedMul(a, Q1[1])) + fixedMul(c, Q1[3])) - fixedMul(b, Q1[0]));
		w = pw;
		a = px;
		b = py;
		c = pz;
		}

	// E = P * mRelativeRotation (0xa0b6b-0xa0c15).
	const NxReal* Q = mRelativeRotation;
	const NxReal ew = (NxReal)(((fixedMul(w, Q[3]) - fixedMul(a, Q[0])) - fixedMul(b, Q[1])) - fixedMul(c, Q[2]));
	const NxReal exRot = (NxReal)(((fixedMul(b, Q[2]) + fixedMul(a, Q[3])) + fixedMul(w, Q[0])) - fixedMul(c, Q[1]));
	NxReal eyRot = (NxReal)(((fixedMul(b, Q[3]) + fixedMul(w, Q[1])) + fixedMul(c, Q[0])) - fixedMul(a, Q[2]));
	double ezRot = ((fixedMul(w, Q[2]) + fixedMul(a, Q[1])) + fixedMul(c, Q[3])) - fixedMul(b, Q[0]);
	double exKept = exRot;
	if(ew < 0.0f)
		{
		exKept = fixedMul(exRot, -1.0f);
		eyRot = (NxReal)fixedMul(eyRot, -1.0f);
		ezRot = fixedMul(ezRot, -1.0f);
		}

	// V (0xa0c50-0xa0cbe).
	NxReal vx;
	NxReal vy;
	double vz;
	if(body0)
		{
		const NxReal* R = body0->mUnknown134;
		vx = (NxReal)((fixedMul(eyRot, R[1]) + fixedMul(ezRot, R[2])) + fixedMul(exKept, R[0]));
		vy = (NxReal)((fixedMul(exKept, R[3]) + fixedMul(eyRot, R[4])) + fixedMul(ezRot, R[5]));
		vz = (fixedMul(exKept, R[6]) + fixedMul(eyRot, R[7])) + fixedMul(ezRot, R[8]);
		}
	else
		{
		vx = (NxReal)exKept;
		vy = eyRot;
		vz = ezRot;
		}

	// The angular errors (0xa0cc2-0xa0cff).
	const NxReal angularX = (NxReal)fixedMul(fixedMul(inverseStored, vx), -2.0f);
	const NxReal angularY = (NxReal)fixedMul(fixedMul(inverseStored, vy), -2.0f);
	const NxReal angularZ = (NxReal)fixedMul(fixedMul(inverseStored, vz), -2.0f);

	// The three angular records (0xa0d01-0xa0f57).
	record = row004093();
	fixedAngularRecord(record, record0, record1, gJointUnitAxis[0]);
	jointSolveRecord(record, this, angularX, mMaxTorque);

	record = row004093();
	fixedAngularRecord(record, record0, record1, gJointUnitAxis[1]);
	jointSolveRecord(record, this, angularY, mMaxTorque);

	record = row004093();
	fixedAngularRecord(record, record0, record1, gJointUnitAxis[2]);
	jointSolveRecord(record, this, angularZ, mMaxTorque);
	}

// phys_fn_004250 (0x000a0f70, 81 B)
// Joint(desc, 0x200) runs first (`push 0x200` at 0xa0f76: the type bit); the
// compiler then stores the vptr 0x10119a50 (0xa0f83). The public object is
// allocated through the SDK allocator (`push 0; push 0x1c; call [edx+8]`) and
// constructed only when the allocation succeeded, but desc.userData is written
// to it without a null check (0xa0fac-0xa0faf): a failed allocation faults
// there in the oracle, and does here too. Then recordRelativePose (row 004244,
// 0xa0fb5) with the descriptor.
FixedJoint::FixedJoint(const NxFixedJointDesc& desc)
	: Joint(desc, 0x200)
	{
	void* memory = nxGetSdkAllocator()->malloc(sizeof(NpFixedJoint), NX_MEMORY_PERSISTENT);
	NpFixedJoint* publicJoint = memory ? new(memory) NpFixedJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	recordRelativePose(desc);
	}

// phys_fn_004252 (0x000a0fd0, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x10119a50, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`), calls the Joint destructor body
// (row 004095) directly, and frees `this` through the SDK allocator (slot
// +0x14) when the flag's bit 0 is set (Joint::operator delete).
FixedJoint::~FixedJoint()
	{
	if(mPublicObject)
		delete static_cast<NpFixedJoint*>(mPublicObject);
	}

// phys_fn_004254 (0x000a1010, 141 B)
// As pulley's 004226: no broken-joint test, only desc.isValid() (the
// descriptor's virtual, `call [eax+8]`, 0xa101c), whose failure reports line
// 0x2a through the static error with no instance check (0xa1023-0xa1033).
// The actors are re-bound (phys_fn_004107 with suppressAttach false) only
// when a body differs from the one held; the second body is not looked at
// when the first already differs. Then the base part (004121) and
// recordRelativePose (004244, 0xa1093) with the descriptor. The supplement
// decompile agrees with the listing.
void FixedJoint::loadFromDesc(const NxFixedJointDesc& desc)
	{
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_FIXEDJOINT_CPP, 0x2a, 0,
			"FixedJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	void* actorImpl0 = fixedActorImpl(desc.actor[0]);
	void* actorImpl1 = fixedActorImpl(desc.actor[1]);
	if(fixedBodyOfActorImpl(actorImpl0) != mBody[0] || fixedBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	recordRelativePose(desc);
	}
