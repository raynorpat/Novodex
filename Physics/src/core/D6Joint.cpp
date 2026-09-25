/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/D6Joint.h"
#include "core/NpD6Joint.h"
#include "core/JointSupport.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"

#include <new>
#include <cstdio>
#include <cmath>

// The oracle's __FILE__ for this unit (every report in it pushes the string
// at 0x101195b0). The image keeps this unit at src\D6Joint.cpp, not under
// core\; see units/joint-families-contract.md "## D6" "### File placement".
#define NX_D6JOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\D6Joint.cpp"

// Joint-families Task 3i. Floating point follows core/FixedJoint.cpp: this
// translation unit is x87 in the oracle and is built /arch:IA32 here; a value
// the listing keeps on the FPU stack is a `double`, a value it stores (fstp
// dword) is an `NxReal`, and the listing's operand grouping and order are
// kept. D6Joint is constructed by Scene::createJoint's D6 case (NxJointType
// 9). See units/joint-families-contract.md "## D6".

// The descriptor offsets 004204 reads (0x9c8e4-0x9cb89).
static_assert(offsetof(NxD6JointDesc, xMotion) == 0x6c, "xMotion at desc+0x6c");
static_assert(offsetof(NxD6JointDesc, linearLimit) == 0x84, "linearLimit at desc+0x84");
static_assert(offsetof(NxD6JointDesc, twistLimit) == 0x90, "twistLimit at desc+0x90");
static_assert(offsetof(NxD6JointDesc, swing1Limit) == 0xa8, "swing1Limit at desc+0xa8");
static_assert(offsetof(NxD6JointDesc, swing2Limit) == 0xb4, "swing2Limit at desc+0xb4");
static_assert(offsetof(NxD6JointDesc, xDrive) == 0xc0, "xDrive at desc+0xc0");
static_assert(offsetof(NxD6JointDesc, useSpherical) == 0x120, "useSpherical at desc+0x120");
static_assert(offsetof(NxD6JointDesc, drivePosition) == 0x124, "drivePosition at desc+0x124");
static_assert(offsetof(NxD6JointDesc, driveOrientation) == 0x130, "driveOrientation at desc+0x130");
static_assert(offsetof(NxD6JointDesc, driveLinearVelocity) == 0x140, "driveLinearVelocity at desc+0x140");
static_assert(offsetof(NxD6JointDesc, driveAngularVelocity) == 0x14c, "driveAngularVelocity at desc+0x14c");
static_assert(offsetof(NxD6JointDesc, projectionDistance) == 0x158, "projectionDistance at desc+0x158");
static_assert(offsetof(NxD6JointDesc, projectionAngle) == 0x15c, "projectionAngle at desc+0x15c");
static_assert(offsetof(NxD6JointDesc, projectionMode) == 0x160, "projectionMode at desc+0x160");

// The x87 fcos 004204 takes of each half limit angle (0x9cafa, 0x9cb14,
// 0x9cb2e). The CRT's cos need not agree with the instruction, so it is used
// as core/SphericalJoint.cpp does. The product (a float times 0.5f) is exact
// in a double, so passing it as one changes nothing.
static double d6Fcos(double x)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	double result;
	__asm
		{
		fld		x
		fcos
		fstp	result
		}
	return result;
#else
	return cos(x);
#endif
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* d6ActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* d6BodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

static void d6PrintPose(FILE* stream, const char* name, const D6JointPose* pose);
static void d6Dump(const D6JointPose* pose0, const D6JointPose* pose1, const D6JointPose* pose2,
	const NxReal* jwq, NxReal fps);
static void d6QuaternionRateMatrix(NxReal* out, const D6JointPose* a, const D6JointPose* b);

// phys_fn_004178 (0x0009b270, 396 B)
void D6JointPose::row004178(D6JointPose& out, const D6JointPose& other) const
	{
	(void)out;
	(void)other;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004180 (0x0009b400, 330 B)
D6JointPose* D6JointPose::row004180(D6JointPose& out) const
	{
	NX_ASSERT(0);
	// (unimplemented)
	return &out;
	}

// phys_fn_004182 (0x0009b550, 57 B)
// A broken joint ((mFlags & 0x18) == 0x10) reports (code 1, line 0xaa)
// through the FoundationSDK instance (`cmp [ecx],0; jne; int3` guards the
// import call, 0x9b559-0x9b578) and returns. Otherwise the row tail-jumps to
// the base part (row 004066, 0x9b584): D6's saveToDesc writes none of the
// descriptor's family fields, so they keep what the caller put there. The
// supplement decompile agrees with the listing.
void D6Joint::saveToDesc(NxD6JointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_D6JOINT_CPP, 0xaa, 0,
			"D6Joint::saveToDesc: Joint is broken. Broken joints can't be saved!");
		return;
		}
	saveToDescBase(desc);
	}

// phys_fn_004184 (0x0009b590, 62 B)
// The guarded store (spherical 004292's shape): a broken joint reports (code
// 1, line 0xb1) through the FoundationSDK instance and returns; otherwise
// the argument goes to the Joint base's projectionMode (+0x44, 0x9b5c8).
void D6Joint::setProjectionMode(NxJointProjectionMode mode)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_D6JOINT_CPP, 0xb1, 0,
			"D6Joint::setProjectionMode: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	mProjectionMode = mode;
	}

// phys_fn_004188 (0x0009b5e0, 181 B)
void D6JointDumpRecord::print(FILE* stream) const
	{
	(void)stream;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004190 (0x0009b6a0, 98 B)
static void d6PrintPose(FILE* stream, const char* name, const D6JointPose* pose)
	{
	(void)stream;
	(void)name;
	(void)pose;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004192 (0x0009b710, 328 B)
static void d6Dump(const D6JointPose* pose0, const D6JointPose* pose1, const D6JointPose* pose2,
	const NxReal* jwq, NxReal fps)
	{
	(void)pose0;
	(void)pose1;
	(void)pose2;
	(void)jwq;
	(void)fps;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004194 (0x0009b860, 514 B)
void D6Joint::row004194(NxU32 limit, const NxVec3& ra, const NxVec3& rb, const NxVec3& normal, NxReal bias,
	NxReal maxForce)
	{
	(void)limit;
	(void)ra;
	(void)rb;
	(void)normal;
	(void)bias;
	(void)maxForce;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004196 (0x0009ba70, 349 B)
void D6Joint::row004196(NxU32 limit, const NxVec3& axis, NxReal bias, NxReal maxForce)
	{
	(void)limit;
	(void)axis;
	(void)bias;
	(void)maxForce;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004198 (0x0009bbd0, 1164 B)
static void d6QuaternionRateMatrix(NxReal* out, const D6JointPose* a, const D6JointPose* b)
	{
	(void)out;
	(void)a;
	(void)b;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004200 (0x0009c060, 2098 B)
void D6Joint::row_slot4(NxDebugRenderable& renderable)
	{
	(void)renderable;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004202 (0x0009c8a0, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x10119570, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`, 0x9c8b4), calls the Joint destructor
// body (row 004095) directly, and frees `this` through the SDK allocator
// (slot +0x14) when the flag's bit 0 is set (Joint::operator delete).
D6Joint::~D6Joint()
	{
	if(mPublicObject)
		delete static_cast<NpD6Joint*>(mPublicObject);
	}

// phys_fn_004204 (0x0009c8e0, 698 B)
// A thiscall row of its own (`ret 4`), called by the constructor (0x9e2a7)
// and loadFromDesc (0x9e3a7); kept out of line so it stays one.
// - Word copies in the listing's order: the six motions (desc+0x6c..+0x80 ->
//   +0x16c..+0x180), linearLimit (+0x84 -> +0x184), swing1Limit (+0xa8 ->
//   +0x190), swing2Limit (+0xb4 -> +0x19c), twistLimit (+0x90 -> +0x1a8),
//   then only the x, y and z drives (+0xc0..+0xef -> +0x1c0..+0x1ef; the
//   swing, twist and spherical drives are not copied), the useSpherical
//   byte, drivePosition, driveOrientation, driveLinearVelocity and
//   driveAngularVelocity (+0x120..+0x157 -> +0x220..+0x257).
// - For each LIMITED angular motion (== 1), the half-angle cosine: twist from
//   twistLimit.high.value (+0x1b4), swing1 from +0x190, swing2 from +0x19c,
//   each `fld; fmul 0.5f; fcos; fstp` (0x9caee-0x9cb30). An unlimited motion
//   leaves its cosine as it was.
// - mAngularLimited = any of twist/swing1/swing2 is LIMITED (0x9cb36-0x9cb48),
//   mLinearLimited = any of x/y/z is LIMITED (0x9cb4e-0x9cb6c).
// - Then projectionMode (desc+0x160 -> the Joint base's +0x44),
//   projectionAngle (+0x15c -> +0x25c) and projectionDistance (+0x158 ->
//   +0x258), in that order.
// The float copies are word moves in the listing (`mov edx,[eax+..]`); the
// decompile shows three of them as float loads.
__declspec(noinline) void D6Joint::row004204(const NxD6JointDesc& desc)
	{
	mMotion[0] = desc.xMotion;
	mMotion[1] = desc.yMotion;
	mMotion[2] = desc.zMotion;
	mMotion[3] = desc.twistMotion;
	mMotion[4] = desc.swing1Motion;
	mMotion[5] = desc.swing2Motion;
	mLinearLimit = desc.linearLimit;
	mSwing1Limit = desc.swing1Limit;
	mSwing2Limit = desc.swing2Limit;
	mTwistLimit = desc.twistLimit;
	mDrive[0] = desc.xDrive;
	mDrive[1] = desc.yDrive;
	mDrive[2] = desc.zDrive;
	mUseSpherical = desc.useSpherical;
	mDrivePosition = desc.drivePosition;
	mDriveOrientation = desc.driveOrientation;
	mDriveLinearVelocity = desc.driveLinearVelocity;
	mDriveAngularVelocity = desc.driveAngularVelocity;
	const NxD6JointMotion twist = mMotion[3];
	if(twist == NX_D6JOINT_MOTION_LIMITED)
		mTwistCosHalf = (NxReal)d6Fcos((double)mTwistLimit.high.value * 0.5f);
	const NxD6JointMotion swing1 = mMotion[4];
	if(swing1 == NX_D6JOINT_MOTION_LIMITED)
		mSwing1CosHalf = (NxReal)d6Fcos((double)mSwing1Limit.value * 0.5f);
	const NxD6JointMotion swing2 = mMotion[5];
	if(swing2 == NX_D6JOINT_MOTION_LIMITED)
		mSwing2CosHalf = (NxReal)d6Fcos((double)mSwing2Limit.value * 0.5f);
	mAngularLimited = twist == NX_D6JOINT_MOTION_LIMITED || swing1 == NX_D6JOINT_MOTION_LIMITED ||
		swing2 == NX_D6JOINT_MOTION_LIMITED;
	mLinearLimited = mMotion[0] == NX_D6JOINT_MOTION_LIMITED || mMotion[1] == NX_D6JOINT_MOTION_LIMITED ||
		mMotion[2] == NX_D6JOINT_MOTION_LIMITED;
	mProjectionMode = desc.projectionMode;
	mProjectionAngle = desc.projectionAngle;
	mProjectionDistance = desc.projectionDistance;
	}

// phys_fn_004206 (0x0009cba0, 3200 B)
void D6Joint::row_slot6(NxReal arg)
	{
	(void)arg;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004207 (0x0009d820, 2394 B)
void D6Joint::row_slot8(void* body)
	{
	(void)body;
	NX_ASSERT(0);
	// (unimplemented)
	}

// phys_fn_004210 (0x0009e1a0, 331 B)
// Joint(desc, 0x4000) runs first (`push 0x4000` at 0x9e1a7: the type bit);
// the compiler then stores the vptr 0x10119570 (0x9e1b4) and runs the
// members' inline constructors: NxJointLimitDesc (value 0, restitution 0,
// hardness 1.0f) for the linear, swing1, swing2 and both twist limits, and
// NxJointDriveDesc (driveType 0, spring 0, damping 0, forceLimit FLT_MAX)
// for the six drives (0x9e1bc-0x9e2a1; NxVec3 and NxQuat construct nothing).
// Then row004204 with the descriptor (0x9e2a7). The public object is
// allocated through the SDK allocator (`push 0; push 0x1c; call [edx+8]`)
// and constructed only when the allocation succeeded, but desc.userData is
// written to it without a null check (0x9e2ce / 0x9e2e1): a failed
// allocation faults there in the oracle, and does here too.
D6Joint::D6Joint(const NxD6JointDesc& desc)
	: Joint(desc, 0x4000)
	{
	row004204(desc);
	void* memory = nxGetSdkAllocator()->malloc(sizeof(NpD6Joint), NX_MEMORY_PERSISTENT);
	NpD6Joint* publicJoint = memory ? new(memory) NpD6Joint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	}

// phys_fn_004212 (0x0009e2f0, 194 B)
// desc.isValid() first (the descriptor's virtual, `call [eax+8]`, 0x9e2fc):
// a failure reports line 0x5f; then a broken joint reports line 0x60, both
// through the FoundationSDK instance (`int3` guard, 0x9e303-0x9e340). The
// actors are re-bound (phys_fn_004107 with suppressAttach false, 0x9e397)
// only when a body differs from the one held; the second body is not looked
// at when the first already differs. Then the base part (004121, 0x9e39f)
// and row004204 (0x9e3a7) with the descriptor. The supplement decompile
// agrees with the listing.
void D6Joint::loadFromDesc(const NxD6JointDesc& desc)
	{
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_D6JOINT_CPP, 0x5f, 0,
			"D6Joint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_D6JOINT_CPP, 0x60, 0,
			"D6Joint::loadFromDesc: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	void* actorImpl0 = d6ActorImpl(desc.actor[0]);
	void* actorImpl1 = d6ActorImpl(desc.actor[1]);
	if(d6BodyOfActorImpl(actorImpl0) != mBody[0] || d6BodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	row004204(desc);
	}
