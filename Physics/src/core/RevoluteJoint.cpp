/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/RevoluteJoint.h"
#include "core/NpRevoluteJoint.h"
#include "NxJoint.h"
#include "NxMath.h"

#include <math.h>
#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x1011a204).
#define NX_REVOLUTEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\RevoluteJoint.cpp"

// Floating point follows core/Joint.cpp: this translation unit is x87 in the
// oracle and is built /arch:IA32 here; a value the listing keeps on the FPU
// stack is a `double`, a value it stores (fstp dword) is an `NxReal`, and the
// listing's sum grouping is kept.
//
// Task 7 wrote the rows under 700 B. The six large rows (004356, 004360,
// 004362, 004364, 004372, 004374) are Task 8's and stay stubs until then.
// Nothing constructs RevoluteJoint until Task 10 wires Scene::createJoint.

// The float the setters raise a body's wake counter to and compare it with:
// 0x3ecccccc (.rdata 0x101053d4 and the immediate stored), as in Joint.cpp.
static const NxReal gRevoluteWakeFloor = 0.39999998f;

static NX_INLINE double revoluteMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

static NX_INLINE JointBodyRecord* revoluteBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does (the actor classes are outside the pilot).
static NX_INLINE void* revoluteActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* revoluteBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// Inlined in every setter that has it (004334, 004340, 004344, 004348): raise
// the body's wake counter to the floor unless its +0x114 bit 8 is set.
// `fcomp; test ah,5; jp` -- store only when strictly less.
static void revoluteRaiseWakeCounter(void* bodyPointer)
	{
	JointBodyRecord* body = revoluteBody(bodyPointer);
	if(body && !(body->mUnknown114 & 0x100) && body->mWakeUpCounter < gRevoluteWakeFloor)
		body->mWakeUpCounter = gRevoluteWakeFloor;
	}

// The loop 004352 and 004354 open with (the same inlined loop as Joint.cpp's
// 004125/004129): refresh the first body whose stamp no longer matches, and
// only that one.
static void revoluteRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = revoluteBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// The inlined NxMath::acos(NxF32) clamp (>= 1 -> 0, <= -1 -> the float pi at
// 0x1011a1b0, otherwise _CIacos of the float), kept at double precision:
// 004352 neither rounds the result nor the product it returns.
static NX_INLINE double revoluteAcos(NxReal f)
	{
	if(f >= 1.0f)
		return 0.0f;
	if(f <= -1.0f)
		return NxPiF32;
	return acos((double)f);
	}

// 004332 evaluates cos/sin of the float angle with the x87 fcos/fsin
// instructions (0xa8edb, 0xa8eeb) and stores each result as a float. The
// CRT's cos/sin need not agree with fcos/fsin, so the instructions are used
// directly, as Foundation/src/DebugRenderable.cpp does.
static NxReal revoluteFcos(NxReal angle)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	NxReal result;
	__asm
		{
		fld angle
		fcos
		fstp result
		}
	return result;
#else
	return (NxReal)cos((double)angle);
#endif
	}

static NxReal revoluteFsin(NxReal angle)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	NxReal result;
	__asm
		{
		fld angle
		fsin
		fstp result
		}
	return result;
#else
	return (NxReal)sin((double)angle);
#endif
	}

// phys_fn_004366 (0x000ac540, 162 B)
// Joint(desc, 0x40) runs first; the compiler then stores the vptr 0x1011a1c0
// and the member default constructors write +0x16c..+0x198 (0xac559-0xac5a4:
// limit 0, 0, 1, 0, 0, 1; motor NX_MAX_REAL, 0, 0; spring 0, 0, 0). The public
// object is allocated through the SDK allocator (`push 0; push 0x1c; call
// [edx+8]`) and constructed only when the allocation succeeded, but
// desc.userData is written to it without a null check (0xac5cc-0xac5cf): a
// failed allocation faults there in the oracle, and does here too.
RevoluteJoint::RevoluteJoint(const NxRevoluteJointDesc& desc)
	: Joint(desc, 0x40)
	{
	void* memory = nxGetSdkAllocator()->malloc(sizeof(NpRevoluteJoint), NX_MEMORY_PERSISTENT);
	NpRevoluteJoint* publicJoint = memory ? new(memory) NpRevoluteJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	row004332(desc);
	}

// phys_fn_004368 (0x000ac5f0, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x1011a1c0, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`), calls the Joint destructor body
// phys_fn_004095 directly, and frees `this` through the SDK allocator (slot
// +0x14) when the flag's bit 0 is set (Joint::operator delete).
RevoluteJoint::~RevoluteJoint()
	{
	if(mPublicObject)
		delete static_cast<NpRevoluteJoint*>(mPublicObject);
	}

// phys_fn_004374 (0x000ad0b0, 1068 B)
// (unimplemented)
void RevoluteJoint::row_slot0(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004328 (0x000a8d20, 21 B)
void RevoluteJoint::row_slot1()
	{
	mUnknown1ac.z = 0.0f;
	mUnknown1ac.y = 0.0f;
	mUnknown1ac.x = 0.0f;
	}

// phys_fn_004364 (0x000ab840, 3326 B)
// (unimplemented)
void RevoluteJoint::row_slot4(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004360 (0x000aa060, 4460 B)
// (unimplemented)
void RevoluteJoint::row_slot6(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004362 (0x000ab1d0, 1644 B)
// (unimplemented)
void RevoluteJoint::row_slot7(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004356 (0x000a9650, 2303 B)
// (unimplemented)
void RevoluteJoint::row_slot8(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004370 (0x000ac630, 202 B)
// desc.isValid() is the descriptor's virtual (slot 2, `call [edx+8]`). The
// actors are re-bound (phys_fn_004107 with suppressAttach false) only when a
// body differs from the one held; the second body is not looked at when the
// first already differs.
void RevoluteJoint::loadFromDesc(const NxRevoluteJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_REVOLUTEJOINT_CPP, 0x70, 0,
			"RevoluteJoint::loadFromDesc: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_REVOLUTEJOINT_CPP, 0x71, 0,
			"RevoluteJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	void* actorImpl0 = revoluteActorImpl(desc.actor[0]);
	void* actorImpl1 = revoluteActorImpl(desc.actor[1]);
	if(revoluteBodyOfActorImpl(actorImpl0) != mBody[0] || revoluteBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	row004332(desc);
	}

// phys_fn_004330 (0x000a8d40, 281 B)
// projectionAngle is recovered from the stored cosine through the inlined
// NxMath::acos(NxF32) clamp (0xa8dfe-0xa8e34) and stored as a float.
void RevoluteJoint::saveToDesc(NxRevoluteJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_REVOLUTEJOINT_CPP, 0x84, 0,
			"RevoluteJoint::saveToDesc: Joint is broken. Broken joints can't be saved!");
		return;
		}
	saveToDescBase(desc);
	desc.limit = mLimit;
	desc.motor = mMotor;
	desc.spring = mSpring;
	desc.projectionDistance = mProjectionDistance;
	desc.projectionAngle = NxMath::acos(mProjectionAngleCos);
	desc.flags = mRevoluteFlags;
	desc.projectionMode = mProjectionMode;
	}

// phys_fn_004334 (0x000a8f10, 147 B)
// Listing: the work arm stores the argument at +0x1a8 (0xa8f48) and then
// raises both bodies' wake counters. The Round 141 drive that saw +0x1a8
// stay zero (evidence/phase5-object-model.md 3z158) was a harness artifact;
// 3z172 supersedes it and its drive stores the argument as the listing says.
void RevoluteJoint::setFlags(NxU32 flags)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_REVOLUTEJOINT_CPP, 0x9d, 0,
			"RevoluteJoint::setFlags: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	mRevoluteFlags = flags;
	revoluteRaiseWakeCounter(mBody[0]);
	revoluteRaiseWakeCounter(mBody[1]);
	}

// phys_fn_004336 (0x000a8fb0, 7 B)
NxU32 RevoluteJoint::getFlags() const
	{
	return mRevoluteFlags;
	}

// phys_fn_004338 (0x000a8fc0, 62 B)
void RevoluteJoint::setProjectionMode(NxJointProjectionMode mode)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_REVOLUTEJOINT_CPP, 0xae, 0,
			"RevoluteJoint::setProjectionMode: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	mProjectionMode = mode;
	}

// phys_fn_004332 (0x000a8e60, 171 B)
void RevoluteJoint::row004332(const NxRevoluteJointDesc& desc)
	{
	mLimit = desc.limit;
	mMotor = desc.motor;
	mSpring = desc.spring;
	mProjectionDistance = desc.projectionDistance;
	mProjectionAngleCos = revoluteFcos(desc.projectionAngle);
	mProjectionAngleSin = revoluteFsin(desc.projectionAngle);
	mRevoluteFlags = desc.flags;
	mProjectionMode = desc.projectionMode;
	}

// phys_fn_004340 (0x000a9000, 189 B)
void RevoluteJoint::setLimits(const NxJointLimitPairDesc& limits)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_REVOLUTEJOINT_CPP, 0xb9, 0,
			"RevoluteJoint::setLimits: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	mLimit = limits;
	mRevoluteFlags |= NX_RJF_LIMIT_ENABLED;
	revoluteRaiseWakeCounter(mBody[0]);
	revoluteRaiseWakeCounter(mBody[1]);
	}

// phys_fn_004342 (0x000a90c0, 58 B)
bool RevoluteJoint::getLimits(NxJointLimitPairDesc& limits) const
	{
	limits = mLimit;
	return (mRevoluteFlags & 1) != 0;
	}

// phys_fn_004344 (0x000a9100, 171 B)
void RevoluteJoint::setMotor(const NxMotorDesc& motor)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_REVOLUTEJOINT_CPP, 0xcc, 0,
			"RevoluteJoint::setMotor: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	mMotor = motor;
	mRevoluteFlags |= NX_RJF_MOTOR_ENABLED;
	revoluteRaiseWakeCounter(mBody[0]);
	revoluteRaiseWakeCounter(mBody[1]);
	}

// phys_fn_004346 (0x000a91b0, 42 B)
bool RevoluteJoint::getMotor(NxMotorDesc& motor) const
	{
	motor = mMotor;
	return ((mRevoluteFlags >> 1) & 1) != 0;
	}

// phys_fn_004348 (0x000a91e0, 171 B)
void RevoluteJoint::setSpring(const NxSpringDesc& spring)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_REVOLUTEJOINT_CPP, 0xe0, 0,
			"RevoluteJoint::setSpring: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	mSpring = spring;
	mRevoluteFlags |= NX_RJF_SPRING_ENABLED;
	revoluteRaiseWakeCounter(mBody[0]);
	revoluteRaiseWakeCounter(mBody[1]);
	}

// phys_fn_004350 (0x000a9290, 43 B)
bool RevoluteJoint::getSpring(NxSpringDesc& spring) const
	{
	spring = mSpring;
	return ((mRevoluteFlags >> 2) & 1) != 0;
	}

// phys_fn_004352 (0x000a92c0, 694 B)
// The angle between the two bodies' world normals (mWorldNormal[i] rotated by
// body i's +0x134 3x3, or taken as is with no body), signed by the dot of
// body 1's normal with body 0's rotated world cross vector.
// Listing over decompile: the decompile shows body 0's rotated normal and the
// sign test in float; the listing keeps that normal on the FPU stack
// (0xa9332-0xa93a8) and the sign sum unrounded (0xa9535-0xa9551), stores
// only the cosine as a float (0xa94f4) before the acos clamp, and rounds
// neither the acos result nor the returned product. The contract's
// NxMath::acos(NxF32) reuse for this site would round that result, so the
// clamp is inlined at double precision here (revoluteAcos).
NxF64 RevoluteJoint::row004352()
	{
	revoluteRefreshFirstStaleBody(*this);

	double n0x, n0y, n0z;
	NxReal cx, cy, cz;
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	if(!body0)
		{
		n0x = mWorldNormal[0].x;
		n0y = mWorldNormal[0].y;
		n0z = mWorldNormal[0].z;
		cx = mWorldCross[0].x;
		cy = mWorldCross[0].y;
		cz = mWorldCross[0].z;
		}
	else
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& n = mWorldNormal[0];
		n0x = (revoluteMul(m[1], n.y) + revoluteMul(m[2], n.z)) + revoluteMul(m[0], n.x);
		n0y = (revoluteMul(m[4], n.y) + revoluteMul(m[3], n.x)) + revoluteMul(m[5], n.z);
		n0z = (revoluteMul(m[7], n.y) + revoluteMul(m[6], n.x)) + revoluteMul(m[8], n.z);
		const NxVec3& c = mWorldCross[0];
		cx = (NxReal)((revoluteMul(m[2], c.z) + revoluteMul(m[1], c.y)) + revoluteMul(c.x, m[0]));
		cy = (NxReal)((revoluteMul(m[3], c.x) + revoluteMul(m[5], c.z)) + revoluteMul(m[4], c.y));
		cz = (NxReal)((revoluteMul(m[6], c.x) + revoluteMul(m[8], c.z)) + revoluteMul(m[7], c.y));
		}

	NxReal n1x, n1y, n1z;
	const JointBodyRecord* body1 = revoluteBody(mBody[1]);
	if(!body1)
		{
		n1x = mWorldNormal[1].x;
		n1y = mWorldNormal[1].y;
		n1z = mWorldNormal[1].z;
		}
	else
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& n = mWorldNormal[1];
		n1x = (NxReal)((revoluteMul(m[1], n.y) + revoluteMul(m[2], n.z)) + revoluteMul(n.x, m[0]));
		n1y = (NxReal)((revoluteMul(m[3], n.x) + revoluteMul(m[4], n.y)) + revoluteMul(m[5], n.z));
		n1z = (NxReal)((revoluteMul(m[6], n.x) + revoluteMul(m[7], n.y)) + revoluteMul(m[8], n.z));
		}

	const NxReal cosine = (NxReal)(((double)n1z * n0z + (double)n1y * n0y) + (double)n1x * n0x);
	const double angle = revoluteAcos(cosine);
	const double side = (revoluteMul(n1z, cz) + revoluteMul(n1y, cy)) + revoluteMul(n1x, cx);
	if(side < 0.0f)
		return angle * -1.0f;
	return angle * 1.0f;
	}

// phys_fn_004354 (0x000a9580, 197 B)
// The relative angular velocity (body 0's minus body 1's, stored as floats)
// projected on the global axis (phys_fn_004129); returned unrounded.
NxF64 RevoluteJoint::getVelocity() const
	{
	// The oracle row is logically const but refreshes the frame cache.
	revoluteRefreshFirstStaleBody(const_cast<RevoluteJoint&>(*this));

	NxVec3 w;
	w.x = 0.0f;
	w.y = 0.0f;
	w.z = 0.0f;
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	if(body0)
		w = body0->mAngularVelocity;
	const JointBodyRecord* body1 = revoluteBody(mBody[1]);
	if(body1)
		{
		w.x = (NxReal)((double)w.x - body1->mAngularVelocity.x);
		w.y = (NxReal)((double)w.y - body1->mAngularVelocity.y);
		w.z = (NxReal)((double)w.z - body1->mAngularVelocity.z);
		}
	NxVec3 axis;
	getGlobalAxis(axis);
	return (revoluteMul(axis.z, w.z) + revoluteMul(axis.y, w.y)) + revoluteMul(axis.x, w.x);
	}

// phys_fn_004358 (0x000a9f50, 269 B)
// out = (v0 + w0 x mUnknown1dc) - (v1 + w1 x mUnknown1e8), where vi/wi are the
// two vec3s of body i's JointBodyRecord204; a missing body contributes zero.
// Listing over decompile: the decompile shows every intermediate as a float;
// the listing keeps body 0's first cross component and body 1's first two
// cross components and last two sums on the FPU stack, storing only the
// values it spills (0xa9f88, 0xa9fa0, 0xaa025, 0xaa02d).
void RevoluteJoint::row004358(NxVec3& out) const
	{
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	if(body0)
		{
		const JointBodyRecord204* record = body0->mUnknown204;
		const NxVec3& v = record->mUnknown000;
		const NxVec3& w = record->mUnknown010;
		const NxVec3& r = mUnknown1dc;
		const double t0 = revoluteMul(w.y, r.z) - revoluteMul(w.z, r.y);
		const NxReal t1 = (NxReal)(revoluteMul(w.z, r.x) - revoluteMul(w.x, r.z));
		const NxReal t2 = (NxReal)(revoluteMul(w.x, r.y) - revoluteMul(w.y, r.x));
		out.x = (NxReal)(t0 + v.x);
		out.y = (NxReal)((double)t1 + v.y);
		out.z = (NxReal)((double)t2 + v.z);
		}
	else
		{
		out.z = 0.0f;
		out.y = 0.0f;
		out.x = 0.0f;
		}
	const JointBodyRecord* body1 = revoluteBody(mBody[1]);
	if(body1)
		{
		const JointBodyRecord204* record = body1->mUnknown204;
		const NxVec3& v = record->mUnknown000;
		const NxVec3& w = record->mUnknown010;
		const NxVec3& r = mUnknown1e8;
		const double u0 = revoluteMul(w.y, r.z) - revoluteMul(w.z, r.y);
		const double u1 = revoluteMul(w.z, r.x) - revoluteMul(w.x, r.z);
		const NxReal u2 = (NxReal)(revoluteMul(w.x, r.y) - revoluteMul(w.y, r.x));
		const NxReal p0 = (NxReal)(u0 + v.x);
		const double p1 = u1 + v.y;
		const double p2 = (double)u2 + v.z;
		out.x = (NxReal)((double)out.x - p0);
		out.y = (NxReal)((double)out.y - p1);
		out.z = (NxReal)((double)out.z - p2);
		}
	}

// phys_fn_004372 (0x000ac700, 2467 B)
// (unimplemented)
NxReal RevoluteJoint::getAngle() const
	{
	NX_ASSERT(0);
	return 0.0f;
	}
