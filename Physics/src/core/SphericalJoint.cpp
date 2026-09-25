/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/SphericalJoint.h"
#include "core/NpSphericalJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "core/JointAcos.h"
#include "X87Sqrt.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"
#include "NxUtilities.h"

#include <float.h>
#include <math.h>
#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x10119e64).
#define NX_SPHERICALJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\SphericalJoint.cpp"

// Joint-families Task 3c. Floating point follows core/RevoluteJoint.cpp:
// this translation unit is x87 in the oracle and is built /arch:IA32 here; a
// value the listing keeps on the FPU stack is a `double`, a value it stores
// (fstp dword) is an `NxReal`, and the listing's operand grouping is kept
// (sums are written in the listing's term order, grouped as the listing adds
// them). SphericalJoint is constructed by Scene::createJoint's spherical case
// (NxJointType 3). See units/joint-families-contract.md "## Spherical".
//
// The unit includes the five rows of the gap
// core\SphericalJoint.cpp..core\CylindricalJoint.cpp (004306-004314), which
// Task 3b found to be spherical; 004314 is the tail of 004312.

static NX_INLINE double sphericalMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

// (a0 * b0 + a1 * b1) + a2 * b2 with the products and the partial sum on the
// stack; the argument order is the listing's term order at each site.
static NX_INLINE double sphericalSum3(NxReal a0, NxReal b0, NxReal a1, NxReal b1, NxReal a2, NxReal b2)
	{
	return (sphericalMul(a0, b0) + sphericalMul(a1, b1)) + sphericalMul(a2, b2);
	}

static NX_INLINE JointBodyRecord* sphericalBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* sphericalActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* sphericalBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// The inlined loop 004296, 004298, 004306 and 004312 open with (the same loop
// as Joint.cpp's 004125/004129): refresh the first body whose stamp no longer
// matches, and only that one.
static void sphericalRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = sphericalBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// An SDK parameter the oracle reads straight from the live array (.data
// 0x10123b18 + 4 * index): 0 NX_PENALTY_FORCE, 4 NX_BOUNCE_TRESHOLD, 13
// NX_VISUALIZATION_SCALE, 31/32/33 the joint local axes / world axes / limits
// visualization. Read through PhysicsSDK::getParameter as the revolute rows
// do (revolute-contract.md open issue 8).
static NxReal sphericalSdkParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// The x87 transcendental instructions the rows use directly (fcos, fsin,
// fptan, fpatan). The CRT's functions need not agree with the instructions,
// so they are used as Foundation/src/DebugRenderable.cpp and
// core/RevoluteJoint.cpp do. The result is kept as a double.
static double sphericalFcos(double x)
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

static double sphericalFsin(double x)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	double result;
	__asm
		{
		fld		x
		fsin
		fstp	result
		}
	return result;
#else
	return sin(x);
#endif
	}

// `fld st(0); fcos; fxch st(1); fsin` (004312's twist arc, 0xa6b92-0xa6b98;
// the same instructions as revolute 004364's arc).
static void sphericalFcosFsin(double angle, double& cosine, double& sine)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	double c;
	double s;
	__asm
		{
		fld		angle
		fld		st(0)
		fcos
		fxch	st(1)
		fsin
		fstp	s
		fstp	c
		}
	cosine = c;
	sine = s;
#else
	cosine = cos(angle);
	sine = sin(angle);
#endif
	}

// `fptan; fstp st(0)` (004312's swing cone radius, 0xa7012-0xa7014): the
// 1.0 fptan pushes is dropped.
static double sphericalFptan(double x)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	double result;
	__asm
		{
		fld		x
		fptan
		fstp	st(0)
		fstp	result
		}
	return result;
#else
	return tan(x);
#endif
	}

// `fpatan` with y in st(1) and x in st(0) (004306, 0xa4ee9).
static double sphericalFpatan(double y, double x)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	double result;
	__asm
		{
		fld		y
		fld		x
		fpatan
		fstp	result
		}
	return result;
#else
	return atan2(y, x);
#endif
	}

// The record header the 004296/004310 record sites write after
// Joint::row004093 hands out a record: the two body records and the row
// vector.
static void sphericalRecordHeader(JointSupportRecord* record, JointSupportBody* body0, JointSupportBody* body1,
	const NxVec3& v)
	{
	record->mUnknown000 = v;
	record->mBody[0] = body0;
	record->mBody[1] = body1;
	}

// The kind and bit update of the spring records (004310 0xa55bc-0xa5629 and
// its copies): the kind in bits 0-4 ((flags & keep) | kind), bit 9 = (kind is
// 0 or 2) stored through an xor, then bit 10 = (kind is 3, 2 or 5) with bits
// 5-8 and 11-18 cleared. The kind is known, but the listing tests it (the
// decompile drops those arms as unreachable), so the tests stay.
static void sphericalSpringBits(JointSupportRecord* record, NxU32 keep, NxU32 kind)
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

// The limit records' update (004310 0xa59d4-0xa5a11 and its copies; the
// same as revolute's revoluteRecordBits): the kind, then bit 9 = (kind is 0
// or 2), bit 10 set, bits 5-8 and 11-18 cleared.
static void sphericalLimitBits(JointSupportRecord* record, NxU32 keep, NxU32 kind)
	{
	const NxU32 flags = (record->mFlags & keep) | kind;
	record->mFlags = flags;
	const NxU32 current = flags & 0x1f;
	const NxU32 bit9 = (current == 0 || current == 2) ? 1 : 0;
	record->mFlags = (((bit9 & 1) | 2) << 9) | (flags & 0xfff8041f);
	}

// The 004393 tail of the spring records (004296 0xa4019-0xa403a, 004310
// 0xa5625-0xa564e and copies): fill +0x30..+0x4c and hand the two gains to
// row 004393 (JointSupportRecord::row004393).
static void sphericalSpringTail(JointSupportRecord* record, Joint* joint, NxReal value034, NxReal value048,
	NxReal scale, NxReal ratio)
	{
	record->mUnknown034 = value034;
	record->mUnknown038 = 0.0f;
	record->mUnknown044 = 0;
	record->mUnknown04c = 0;
	record->mUnknown048 = value048;
	record->mUnknown030 = joint;
	record->row004393(scale, ratio);
	}

// The 004391 tail of the limit records (004310 0xa5a14-0xa5a75 and its
// copies): the same instructions as jointSolveRecord with +0x48 = NX_MAX_REAL;
// the listing passes the local that held +0x34 as 004391's dead first output.
static void sphericalLimitTail(JointSupportRecord* record, Joint* joint, NxReal value034)
	{
	record->mUnknown034 = value034;
	record->mUnknown038 = 0.0f;
	record->mUnknown044 = 0;
	record->mUnknown04c = 0;
	record->mUnknown048 = NX_MAX_REAL;
	record->mUnknown030 = joint;
	NxReal unused = value034;
	record->row004391(unused, record->mUnknown040);
	record->mUnknown03c = record->mUnknown040;
	const NxU32 kind = record->mFlags & 0x1f;
	if(kind == 0 || kind == 2)
		record->mUnknown040 = (NxReal)((double)sphericalSdkParameter(NX_PENALTY_FORCE) * record->mUnknown040);
	else if(kind == 1 || kind == 3)
		record->mUnknown040 = (NxReal)((double)record->mUnknown040 * 0.7f);
	}

// The restitution step after a limit record (004310 0xa5b5c-0xa5b98 and its
// copies): with a non-zero restitution, when phys_fn_004389's value is below
// SDK parameter 4, +0x38 = -(value * restitution).
static void sphericalRestitution(JointSupportRecord* record, NxReal restitution)
	{
	if(restitution != 0.0f)
		{
		const double value = record->row004389();
		if(value < sphericalSdkParameter(NX_BOUNCE_TRESHOLD))
			record->mUnknown038 = (NxReal)-(value * restitution);
		}
	}

// 004308's per-body update (0xa5187-0xa5220 for body 0, 0xa52ae-0xa5349 for
// body 1; the same instructions as revolute 004374's, see
// revoluteApplyToRecord): unless the record's +0x0c is zero, +0x00 += +0x0c
// * t and +0x10 += (+0x20 3x3) * c.
static void sphericalApplyToRecord(JointSupportBody* record, NxReal tx, NxReal ty, double tz, const NxVec3& c)
	{
	if(record->mUnknown00c == 0.0f)
		return;
	const NxReal m = record->mUnknown00c;
	const double lx = sphericalMul(tx, m);
	const NxReal ly = (NxReal)sphericalMul(ty, m);
	const NxReal lz = (NxReal)(tz * m);
	record->mUnknown000.x = (NxReal)(lx + record->mUnknown000.x);
	record->mUnknown000.y = (NxReal)((double)ly + record->mUnknown000.y);
	record->mUnknown000.z = (NxReal)((double)lz + record->mUnknown000.z);
	const NxReal* I = record->mUnknown020;
	const double u0 = (sphericalMul(c.z, I[2]) + sphericalMul(c.y, I[1])) + sphericalMul(c.x, I[0]);
	const double u1 = (sphericalMul(c.z, I[5]) + sphericalMul(c.y, I[4])) + sphericalMul(c.x, I[3]);
	const NxReal u2 = (NxReal)((sphericalMul(c.z, I[8]) + sphericalMul(c.y, I[7])) + sphericalMul(c.x, I[6]));
	record->mUnknown010.x = (NxReal)(u0 + record->mUnknown010.x);
	record->mUnknown010.y = (NxReal)(u1 + record->mUnknown010.y);
	record->mUnknown010.z = (NxReal)((double)u2 + record->mUnknown010.z);
	}

// phys_fn_004282 (0x000a2bf0, 21 B)
void SphericalJoint::row_slot1()
	{
	mBias.z = 0.0f;
	mBias.y = 0.0f;
	mBias.x = 0.0f;
	}

// phys_fn_004284 (0x000a2c10, 427 B)
// The descriptor blocks are copied dword for dword; fcos of the stored
// swing limit value goes to +0x1cc (0xa2c63-0xa2c73); the swing axis is
// carried into world space through body 0's world frame columns
// (0xa2cdc-0xa2d91): (normal * s.x + axis * s.z) + cross * s.y per
// component, the frame words the listing moves through integer registers
// being the same floats.
void SphericalJoint::row004284(const NxSphericalJointDesc& desc)
	{
	mTwistLimit = desc.twistLimit;
	mSwingLimit = desc.swingLimit;
	mSwingLimitCos = (NxReal)sphericalFcos(mSwingLimit.value);
	mSwingSpring = desc.swingSpring;
	mTwistSpring = desc.twistSpring;
	mJointSpring = desc.jointSpring;
	mSwingAxis = desc.swingAxis;
	const NxVec3& n = mWorldNormal[0];
	const NxVec3& c = mWorldCross[0];
	const NxVec3& a = mWorldAxis[0];
	const NxVec3& s = mSwingAxis;
	mSwingAxisWorld.x = (NxReal)sphericalSum3(n.x, s.x, a.x, s.z, c.x, s.y);
	mSwingAxisWorld.y = (NxReal)sphericalSum3(n.y, s.x, a.y, s.z, c.y, s.y);
	mSwingAxisWorld.z = (NxReal)sphericalSum3(n.z, s.x, a.z, s.z, c.z, s.y);
	mSphericalFlags = desc.flags;
	mProjectionMode = desc.projectionMode;
	mProjectionDistance = desc.projectionDistance;
	}

// phys_fn_004286 (0x000a2dc0, 283 B)
void SphericalJoint::saveToDesc(NxSphericalJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_SPHERICALJOINT_CPP, 0x70, 0,
			"SphericalJoint::saveToDesc: Joint is broken. Broken joints can't be saved!");
		return;
		}
	saveToDescBase(desc);
	desc.twistLimit = mTwistLimit;
	desc.swingLimit = mSwingLimit;
	desc.swingSpring = mSwingSpring;
	desc.twistSpring = mTwistSpring;
	desc.jointSpring = mJointSpring;
	desc.swingAxis = mSwingAxis;
	desc.flags = mSphericalFlags;
	desc.projectionMode = mProjectionMode;
	desc.projectionDistance = mProjectionDistance;
	}

// phys_fn_004288 (0x000a2ee0, 65 B)
// The work arm only stores the argument (0xa2f18); unlike revolute's
// setFlags (004334) there is no wake-counter raise.
void SphericalJoint::setFlags(NxU32 flags)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_SPHERICALJOINT_CPP, 0x83, 0,
			"SphericalJoint::setFlags: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	mSphericalFlags = flags;
	}

// phys_fn_004290 (0x000a2f30, 7 B)
NxU32 SphericalJoint::getFlags() const
	{
	return mSphericalFlags;
	}

// phys_fn_004292 (0x000a2f40, 62 B)
void SphericalJoint::setProjectionMode(NxJointProjectionMode mode)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_SPHERICALJOINT_CPP, 0x8e, 0,
			"SphericalJoint::setProjectionMode: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	mProjectionMode = mode;
	}

// phys_fn_004294 (0x000a2f80, 269 B)
// out = (v0 + w0 x mLever[0]) - (v1 + w1 x mLever[1]), where vi/wi are the
// two vec3s of body i's JointSupportBody; a missing body contributes zero.
// The same instructions as revolute's 004358 over the spherical levers.
// Listing over decompile: the decompile shows every intermediate as a float;
// the listing keeps body 0's first cross component and body 1's first two
// cross components and last two sums on the FPU stack (0xa2fb8, 0xa2fd0,
// 0xa3055, 0xa305d).
void SphericalJoint::row004294(NxVec3& out) const
	{
	const JointBodyRecord* body0 = sphericalBody(mBody[0]);
	if(body0)
		{
		const JointSupportBody* record = body0->mUnknown204;
		const NxVec3& v = record->mUnknown000;
		const NxVec3& w = record->mUnknown010;
		const NxVec3& r = mLever[0];
		const double t0 = sphericalMul(w.y, r.z) - sphericalMul(w.z, r.y);
		const NxReal t1 = (NxReal)(sphericalMul(w.z, r.x) - sphericalMul(w.x, r.z));
		const NxReal t2 = (NxReal)(sphericalMul(w.x, r.y) - sphericalMul(w.y, r.x));
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
	const JointBodyRecord* body1 = sphericalBody(mBody[1]);
	if(body1)
		{
		const JointSupportBody* record = body1->mUnknown204;
		const NxVec3& v = record->mUnknown000;
		const NxVec3& w = record->mUnknown010;
		const NxVec3& r = mLever[1];
		const double u0 = sphericalMul(w.y, r.z) - sphericalMul(w.z, r.y);
		const double u1 = sphericalMul(w.z, r.x) - sphericalMul(w.x, r.z);
		const NxReal u2 = (NxReal)(sphericalMul(w.x, r.y) - sphericalMul(w.y, r.x));
		const NxReal p0 = (NxReal)(u0 + v.x);
		const double p1 = u1 + v.y;
		const double p2 = (double)u2 + v.z;
		out.x = (NxReal)((double)out.x - p0);
		out.y = (NxReal)((double)out.y - p1);
		out.z = (NxReal)((double)out.z - p2);
		}
	}

// 004296's column for body 0 (0xa3406-0xa349b and its two repeats): the
// point constraint's effective-mass column k = m e + w x r for w = J (r x e),
// with (w x r).y kept on the stack; k.x stays on the stack, k.y and k.z are
// stored.
static void sphericalColumnBody0(const NxVec3& r, NxReal wx, NxReal wy, double wz,
	NxReal massX, NxReal massY, NxReal massZ, double& kx, NxReal& ky, NxReal& kz)
	{
	const NxReal vx = (NxReal)(sphericalMul(r.z, wy) - wz * r.y);
	const double vy = wz * r.x - sphericalMul(r.z, wx);
	const NxReal vz = (NxReal)(sphericalMul(wx, r.y) - sphericalMul(wy, r.x));
	kx = (double)vx + massX;
	ky = (NxReal)(vy + massY);
	kz = (NxReal)((double)vz + massZ);
	}

// The same for body 1 (0xa37e3-0xa3835 and its two repeats): here w is
// stored and (w x r).x stays on the stack; all three results are stored.
static void sphericalColumnBody1(const NxVec3& r, NxReal wx, NxReal wy, NxReal wz,
	NxReal massX, NxReal massY, NxReal massZ, NxReal& kx, NxReal& ky, NxReal& kz)
	{
	const double vx = sphericalMul(wy, r.z) - sphericalMul(r.y, wz);
	const NxReal vy = (NxReal)(sphericalMul(wz, r.x) - sphericalMul(wx, r.z));
	const NxReal vz = (NxReal)(sphericalMul(r.y, wx) - sphericalMul(wy, r.x));
	kx = (NxReal)(vx + massX);
	ky = (NxReal)((double)vy + massY);
	kz = (NxReal)((double)vz + massZ);
	}

// phys_fn_004296 (0x000a3090, 5907 B)
// The spherical solver slot (arg is the step divisor). After the stale-body
// refresh:
// 1. The levers r_i = body i's +0x134 3x3 times mWorldAnchor[i] (the anchor
//    itself without the body) and the position error e = (r0 - r1) + body
//    0's +0x158 - body 1's +0x158, each component stored; mLever = r0/r1 and
//    mBias = e * ((1 / arg) * SDK parameter 0).
// 2. K = sum over bodies of (m e_j + (J (r x e_j)) x r) for the unit vectors
//    e_j (m = body +0xc0, J = body +0x164), the zero products of the unit
//    vectors kept (every r * 0 and m * 0 is an fmul by the 0.0f at
//    0x101041f0). Body 1's columns are stored in mInverseMass first and body
//    0's added.
// 3. Determinant non-zero: mInverseMass = K^-1 by cofactors, one zeroed
//    kind-6 record (the joint's point solve, done by slot 0), the per-axis
//    joint-spring gains when flag 0x10 is set, and with a finite maxForce
//    mMaxImpulseSquared = maxForce^2 (FLT_EPSILON when that is 0) and a
//    cleared accumulator. Determinant zero: mInverseMass = identity and three
//    kind-1 linear records along the unit axes, solved through 004393 with
//    the joint spring's gains (flag 0x10) or through 004391 with bias
//    e_j / arg and +0x48 = maxForce.
// Listing over decompile (supplement): the supplement shows the
// intermediates as floats and drops the kind tests as unreachable
// (0x100a3d09, 0x100a3e20, 0x100a3fee, ...); the listing keeps the tests and
// leaves the values named double below on the stack. Unlike revolute's
// 004360 the error's x term adds body 0's +0x158 to the unrounded difference
// (0xa3279), and several sums group differently (r.y and r.z of the levers,
// the second and third inertia rows of every column, and the determinant).
void SphericalJoint::row_slot6(NxReal arg)
	{
	sphericalRefreshFirstStaleBody(*this);

	NxVec3 r0;
	JointSupportBody* record0;
	const JointBodyRecord* body0 = sphericalBody(mBody[0]);
	if(body0)
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& a = mWorldAnchor[0];
		record0 = body0->mUnknown204;
		r0.x = (NxReal)sphericalSum3(m[2], a.z, m[1], a.y, m[0], a.x);
		r0.y = (NxReal)sphericalSum3(m[5], a.z, m[3], a.x, m[4], a.y);
		r0.z = (NxReal)sphericalSum3(m[8], a.z, m[6], a.x, m[7], a.y);
		}
	else
		{
		r0 = mWorldAnchor[0];
		record0 = 0;
		}

	NxVec3 r1;
	JointSupportBody* record1;
	const JointBodyRecord* body1 = sphericalBody(mBody[1]);
	if(body1)
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& a = mWorldAnchor[1];
		record1 = body1->mUnknown204;
		r1.x = (NxReal)sphericalSum3(m[2], a.z, m[1], a.y, m[0], a.x);
		r1.y = (NxReal)sphericalSum3(m[5], a.z, m[3], a.x, m[4], a.y);
		r1.z = (NxReal)sphericalSum3(m[8], a.z, m[6], a.x, m[7], a.y);
		}
	else
		{
		r1 = mWorldAnchor[1];
		record1 = 0;
		}

	// The error (0xa323f-0xa32cd): the x difference stays on the stack and
	// is what body 0's +0x158 is added to; y and z add to their stored
	// floats; every result is stored.
	const double dx = (double)r0.x - r1.x;
	const NxReal dy = (NxReal)((double)r0.y - r1.y);
	const NxReal dz = (NxReal)((double)r0.z - r1.z);
	NxVec3 e;
	e.x = (NxReal)dx;
	e.y = dy;
	e.z = dz;
	if(body0)
		{
		e.x = (NxReal)(dx + body0->mUnknown158.x);
		e.y = (NxReal)((double)dy + body0->mUnknown158.y);
		e.z = (NxReal)((double)dz + body0->mUnknown158.z);
		}
	if(body1)
		{
		e.x = (NxReal)((double)e.x - body1->mUnknown158.x);
		e.y = (NxReal)((double)e.y - body1->mUnknown158.y);
		e.z = (NxReal)((double)e.z - body1->mUnknown158.z);
		}

	const double inverse = 1.0f / (double)arg;
	mLever[0] = r0;
	mLever[1] = r1;
	mBias = e;
	NxReal inverseF = (NxReal)inverse;
	const double gain = inverse * sphericalSdkParameter(NX_PENALTY_FORCE);
	mBias.x = (NxReal)(gain * mBias.x);
	mBias.y = (NxReal)(gain * mBias.y);
	mBias.z = (NxReal)(gain * mBias.z);

	// K, row-major; k00..k02, k12 and k22 are held on the stack (k12 and k22
	// are stored and reloaded after body 0's third column, 0xa36a6), the
	// rest are stored.
	double k00 = 0.0f, k01 = 0.0f, k02 = 0.0f, k12 = 0.0f, k22 = 0.0f;
	NxReal k10 = 0.0f, k11 = 0.0f, k20 = 0.0f, k21 = 0.0f;
	if(body0)
		{
		const NxReal* J = body0->mWorldInverseInertia;
		const NxReal m = body0->mInverseMass;
		const NxReal ryZero = (NxReal)((double)r0.y * 0.0f);
		const NxReal rzZero = (NxReal)((double)r0.z * 0.0f);
		const NxReal rxZero = (NxReal)((double)r0.x * 0.0f);
		NxReal wx, wy, mZero, k1j, k2j;
		double ux, uy, uz, wz;

		// e = (1, 0, 0) (0xa33a0-0xa349b)
		ux = (double)ryZero - rzZero;
		uy = (double)r0.z - rxZero;
		uz = (double)rxZero - r0.y;
		wx = (NxReal)((uz * J[2] + uy * J[1]) + ux * J[0]);
		wy = (NxReal)((uz * J[5] + uy * J[4]) + ux * J[3]);
		wz = (uz * J[8] + uy * J[7]) + ux * J[6];
		mZero = (NxReal)((double)m * 0.0f);
		sphericalColumnBody0(r0, wx, wy, wz, m, mZero, mZero, k00, k10, k20);

		// e = (0, 1, 0) (0xa34a2-0xa35a9)
		ux = (double)ryZero - r0.z;
		uy = (double)rzZero - rxZero;
		uz = (double)r0.x - ryZero;
		wx = (NxReal)((uz * J[2] + uy * J[1]) + ux * J[0]);
		wy = (NxReal)((ux * J[3] + uz * J[5]) + uy * J[4]);
		wz = (ux * J[6] + uz * J[8]) + uy * J[7];
		mZero = (NxReal)(0.0f * (double)m);
		sphericalColumnBody0(r0, wx, wy, wz, mZero, m, mZero, k01, k11, k21);

		// e = (0, 0, 1) (0xa35b7-0xa36ad)
		ux = (double)r0.y - rzZero;
		uy = (double)rzZero - r0.x;
		uz = (double)rxZero - ryZero;
		wx = (NxReal)((uz * J[2] + uy * J[1]) + ux * J[0]);
		wy = (NxReal)((ux * J[3] + uz * J[5]) + uy * J[4]);
		wz = (ux * J[6] + uz * J[8]) + uy * J[7];
		mZero = (NxReal)(0.0f * (double)m);
		sphericalColumnBody0(r0, wx, wy, wz, mZero, mZero, m, k02, k1j, k2j);
		k12 = k1j;
		k22 = k2j;
		}

	if(body1)
		{
		const NxReal* J = body1->mWorldInverseInertia;
		const NxReal m = body1->mInverseMass;
		const NxReal ryZero = (NxReal)((double)r1.y * 0.0f);
		const NxReal rzZero = (NxReal)((double)r1.z * 0.0f);
		const NxReal rxZero = (NxReal)((double)r1.x * 0.0f);
		NxReal uyF, uzF, wx, wy, wz, mZero;
		double ux, uz;
		NxReal* K = mInverseMass;

		// e = (1, 0, 0) (0xa370b-0xa3855)
		ux = (double)ryZero - rzZero;
		uyF = (NxReal)((double)r1.z - rxZero);
		uz = (double)rxZero - r1.y;
		uzF = (NxReal)uz;
		wx = (NxReal)((uz * J[2] + (double)uyF * J[1]) + ux * J[0]);
		wy = (NxReal)((ux * J[3] + sphericalMul(uzF, J[5])) + sphericalMul(uyF, J[4]));
		wz = (NxReal)((ux * J[6] + sphericalMul(uzF, J[8])) + sphericalMul(uyF, J[7]));
		mZero = (NxReal)((double)m * 0.0f);
		sphericalColumnBody1(r1, wx, wy, wz, m, mZero, mZero, K[0], K[3], K[6]);

		// e = (0, 1, 0) (0xa385b-0xa3972)
		ux = (double)ryZero - r1.z;
		uyF = (NxReal)((double)rzZero - rxZero);
		uz = (double)r1.x - ryZero;
		uzF = (NxReal)uz;
		wx = (NxReal)((uz * J[2] + (double)uyF * J[1]) + ux * J[0]);
		wy = (NxReal)((ux * J[3] + sphericalMul(uzF, J[5])) + sphericalMul(uyF, J[4]));
		wz = (NxReal)((ux * J[6] + sphericalMul(uzF, J[8])) + sphericalMul(uyF, J[7]));
		mZero = (NxReal)(0.0f * (double)m);
		sphericalColumnBody1(r1, wx, wy, wz, mZero, m, mZero, K[1], K[4], K[7]);

		// e = (0, 0, 1) (0xa3978-0xa3a8b)
		ux = (double)r1.y - rzZero;
		uyF = (NxReal)((double)rzZero - r1.x);
		uz = (double)rxZero - ryZero;
		uzF = (NxReal)uz;
		wx = (NxReal)((uz * J[2] + (double)uyF * J[1]) + ux * J[0]);
		wy = (NxReal)((sphericalMul(uzF, J[5]) + sphericalMul(uyF, J[4])) + ux * J[3]);
		wz = (NxReal)((sphericalMul(uzF, J[8]) + sphericalMul(uyF, J[7])) + ux * J[6]);
		mZero = (NxReal)(0.0f * (double)m);
		sphericalColumnBody1(r1, wx, wy, wz, mZero, mZero, m, K[2], K[5], K[8]);

		// 0xa3a91-0xa3b09.
		k00 = k00 + K[0];
		k01 = k01 + K[1];
		k02 = k02 + K[2];
		k10 = (NxReal)((double)k10 + K[3]);
		k11 = (NxReal)((double)k11 + K[4]);
		k12 = k12 + K[5];
		k20 = (NxReal)((double)k20 + K[6]);
		k21 = (NxReal)((double)k21 + K[7]);
		k22 = k22 + K[8];
		}

	// Cofactors and the float determinant (0xa3b0f-0xa3b63).
	const double c00 = (double)k11 * k22 - k12 * k21;
	const NxReal c10 = (NxReal)(k02 * k21 - k22 * k01);
	const double c20 = k12 * k01 - k02 * k11;
	const NxReal c20F = (NxReal)c20;
	const NxReal determinant = (NxReal)(((c20 * k20) + c00 * k00) + sphericalMul(c10, k10));
	const NxReal maxForce = mMaxForce;
	if(determinant == 0.0f)
		{
		mInverseMass[0] = 1.0f;
		mInverseMass[1] = 0.0f;
		mInverseMass[2] = 0.0f;
		mInverseMass[3] = 0.0f;
		mInverseMass[4] = 1.0f;
		mInverseMass[5] = 0.0f;
		mInverseMass[6] = 0.0f;
		mInverseMass[7] = 0.0f;
		mInverseMass[8] = 1.0f;

		// The per-axis error over arg, from the unscaled error (0xa3bad-0xa3bf6).
		const NxReal errorX = (NxReal)sphericalMul(e.x, inverseF);
		const NxReal errorY = (NxReal)sphericalMul(e.y, inverseF);
		const NxReal errorZ = (NxReal)sphericalMul(e.z, inverseF);
		if(mSphericalFlags & NX_SJF_JOINT_SPRING_ENABLED)
			{
			// The joint spring's gains (0xa3c00-0xa3c31).
			const double stiffness = (double)arg * mJointSpring.spring;
			const double denominator = stiffness + mJointSpring.damper;
			const NxReal ratio = (NxReal)((mJointSpring.spring / denominator) * arg);
			const NxReal scale = (NxReal)(1.0f / (denominator * arg));

			JointSupportRecord* record = row004093();
			jointLinearRecord(record, record0, record1, gJointUnitAxis[0], r0, r1);
			sphericalSpringTail(record, this, errorX, maxForce, scale, ratio);

			record = row004093();
			jointLinearRecord(record, record0, record1, gJointUnitAxis[1], r0, r1);
			sphericalSpringTail(record, this, errorY, mMaxForce, scale, ratio);

			record = row004093();
			jointLinearRecord(record, record0, record1, gJointUnitAxis[2], r0, r1);
			sphericalSpringTail(record, this, errorZ, mMaxForce, scale, ratio);
			return;
			}

		// 0xa42f0-0xa4796.
		JointSupportRecord* record = row004093();
		jointLinearRecord(record, record0, record1, gJointUnitAxis[0], r0, r1);
		jointSolveRecord(record, this, errorX, maxForce);

		record = row004093();
		jointLinearRecord(record, record0, record1, gJointUnitAxis[1], r0, r1);
		jointSolveRecord(record, this, errorY, mMaxForce);

		record = row004093();
		jointLinearRecord(record, record0, record1, gJointUnitAxis[2], r0, r1);
		jointSolveRecord(record, this, errorZ, mMaxForce);
		return;
		}

	// 0xa3d1f-0xa3e01: the first entry uses the unrounded 1 / determinant,
	// the rest the stored float.
	const double inverseDeterminant = 1.0f / (double)determinant;
	const NxReal d = (NxReal)inverseDeterminant;
	mInverseMass[0] = (NxReal)(inverseDeterminant * c00);
	mInverseMass[1] = (NxReal)sphericalMul(d, c10);
	mInverseMass[2] = (NxReal)sphericalMul(d, c20F);
	mInverseMass[3] = (NxReal)((k12 * k20 - k22 * k10) * d);
	mInverseMass[4] = (NxReal)((k22 * k00 - k02 * k20) * d);
	mInverseMass[5] = (NxReal)((k02 * k10 - k12 * k00) * d);
	mInverseMass[6] = (NxReal)((sphericalMul(k21, k10) - sphericalMul(k11, k20)) * d);
	mInverseMass[7] = (NxReal)((k20 * k01 - k00 * k21) * d);
	mInverseMass[8] = (NxReal)((k00 * k11 - k01 * k10) * d);

	// A zeroed record of kind 6 (0xa3e07-0xa3ea7), revolute 004360's
	// instructions: bits 9 and 10 stored through xors from the kind tests,
	// then bits 6-8 and 11-18 cleared.
	JointSupportRecord* record = row004093();
	NxU32 flags = (record->mFlags & 0xffffffe6) | 0x26;
	record->mUnknown030 = this;
	record->mFlags = flags;
	NxU32 kind = flags & 0x1f;
	const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
	flags = (((bit9 << 9) ^ flags) & 0x200) ^ flags;
	record->mFlags = flags;
	kind = flags & 0x1f;
	const NxU32 bit10 = (kind == 3 || kind == 2 || kind == 5) ? 1 : 0;
	flags = (((bit10 << 10) ^ flags) & 0x400) ^ flags;
	record->mFlags = flags;
	record->mBody[0] = record0;
	record->mBody[1] = record1;
	record->mUnknown000.z = 0.0f;
	record->mUnknown000.y = 0.0f;
	record->mUnknown000.x = 0.0f;
	record->mUnknown018.z = 0.0f;
	record->mUnknown018.y = 0.0f;
	record->mUnknown018.x = 0.0f;
	record->mUnknown024.z = 0.0f;
	record->mUnknown024.y = 0.0f;
	record->mUnknown024.x = 0.0f;
	record->mFlags &= 0xfff8063f;
	record->mUnknown034 = 0.0f;
	record->mUnknown038 = 0.0f;
	record->mUnknown04c = 0;
	record->mUnknown048 = 0.0f;
	record->mUnknown040 = 0.0f;
	record->mUnknown03c = 0.0f;

	if(mSphericalFlags & NX_SJF_JOINT_SPRING_ENABLED)
		{
		// The per-axis spring gains (0xa3eb7-0xa3f70): 1 / (g K^-1[i][i] + 1)
		// with g = 1 / ((spring * arg + damper) * arg), stored, and the bias
		// scaled by gain * ratio (the first product on the stack, the other
		// two stored).
		const double stiffness = (double)arg * mJointSpring.spring;
		const double denominator = stiffness + mJointSpring.damper;
		const NxReal ratio = (NxReal)((mJointSpring.spring / denominator) * arg);
		const double g = 1.0f / (denominator * arg);
		const double gainX = 1.0f / (g * mInverseMass[0] + 1.0f);
		mSpringGain.x = (NxReal)gainX;
		const double biasX = gainX * ratio;
		const double gainY = 1.0f / (g * mInverseMass[4] + 1.0f);
		mSpringGain.y = (NxReal)gainY;
		const NxReal biasY = (NxReal)(gainY * ratio);
		const double gainZ = 1.0f / (g * mInverseMass[8] + 1.0f);
		mSpringGain.z = (NxReal)gainZ;
		const NxReal biasZ = (NxReal)(gainZ * ratio);
		mBias.x = (NxReal)(biasX * mBias.x);
		mBias.y = (NxReal)sphericalMul(biasY, mBias.y);
		mBias.z = (NxReal)sphericalMul(biasZ, mBias.z);
		}

	// 0xa3f76-0xa3fc5 (revolute 004360's instructions).
	if(mMaxForce < NX_MAX_REAL)
		{
		const double squared = sphericalMul(mMaxForce, mMaxForce);
		mMaxImpulseSquared = (NxReal)squared;
		if(squared == 0.0f)
			mMaxImpulseSquared = FLT_EPSILON;
		mAccumulatedImpulse.z = 0.0f;
		mAccumulatedImpulse.y = 0.0f;
		mAccumulatedImpulse.x = 0.0f;
		}
	}

// phys_fn_004298 (0x000a47b0, 317 B)
// Projection of the given body record (one of mBody[0]/mBody[1]; `ret 4`, no
// null check): after the stale-body refresh, d = row004064(mWorldAnchor[0],
// mWorldAnchor[1]) (body 0's anchor minus body 1's through the +0x134/+0x158
// poses). Unless |d|^2 < projectionDistance^2 (or unordered), d is negated
// for body 0, scaled by (|d| - projectionDistance) / |d| and added to the
// body's +0x158, and row 000022 is called on the body's +0x19c owner with 1.
// The first step of revolute's 004356, without the angular part.
void SphericalJoint::row_slot8(void* bodyPointer)
	{
	JointBodyRecord* body = sphericalBody(bodyPointer);
	sphericalRefreshFirstStaleBody(*this);

	NxVec3 d;
	row004064(mWorldAnchor[0], mWorldAnchor[1], d);
	const double lengthSquared = sphericalSum3(d.z, d.z, d.y, d.y, d.x, d.x);
	if(!(lengthSquared >= sphericalMul(mProjectionDistance, mProjectionDistance)))
		return;
	if(bodyPointer == mBody[0])
		{
		d.x = -d.x;
		d.y = -d.y;
		d.z = -d.z;
		}
	// fsqrt of the compared sum (0xa4868), re-formed as in the test (the
	// negation above does not change a square; X87Sqrt.h).
	const double length = x87FsqrtDot3(d.z, d.z, d.y, d.y, d.x, d.x);
	const double ratio = (length - mProjectionDistance) / length;
	d.x = (NxReal)(d.x * ratio);
	d.y = (NxReal)(d.y * ratio);
	d.z = (NxReal)(d.z * ratio);
	NxVec3& position = body->mUnknown158;
	position.z = (NxReal)((double)d.z + position.z);
	position.x = (NxReal)((double)d.x + position.x);
	position.y = (NxReal)((double)d.y + position.y);

	// Row 000022 is deferred (owner gap <start>..Actor.cpp); its stub asserts.
	reinterpret_cast<Row000022Fixture*>(body->mOwner)->row000022(1);
	}

// phys_fn_004300 (0x000a48f0, 194 B)
// Joint(desc, 8) runs first (0xa48f7-0xa48fc); the compiler then stores the
// vptr 0x10119e20 (0xa4901) and the member default constructors write
// +0x16c..+0x1b0 (0xa4909-0xa4974: twist limit 0, 0, 1, 0, 0, 1; swing limit
// 0, 0, 1; the three springs 0, 0, 0). The public object is allocated through
// the SDK allocator (`push 0; push 0x1c; call [edx+8]`) and constructed only
// when the allocation succeeded, but desc.userData is written to it without a
// null check (0xa499c-0xa499f): a failed allocation faults there in the
// oracle, and does here too.
SphericalJoint::SphericalJoint(const NxSphericalJointDesc& desc)
	: Joint(desc, 8)
	{
	void* memory = nxGetSdkAllocator()->malloc(sizeof(NpSphericalJoint), NX_MEMORY_PERSISTENT);
	NpSphericalJoint* publicJoint = memory ? new(memory) NpSphericalJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	row004284(desc);
	}

// phys_fn_004302 (0x000a49c0, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x10119e20, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`, 0xa49d2-0xa49d4), calls the Joint
// destructor body (row 004095) directly, and frees `this` through the SDK
// allocator (slot +0x14) when the flag's bit 0 is set (Joint::operator
// delete).
SphericalJoint::~SphericalJoint()
	{
	if(mPublicObject)
		delete static_cast<NpSphericalJoint*>(mPublicObject);
	}

// phys_fn_004304 (0x000a4a00, 186 B)
// desc.isValid() is the descriptor's virtual (slot 2, `call [eax+8]`), tested
// before the broken check (lines 0x39 and 0x3a; revolute's loadFromDesc tests
// them the other way round). The actors are re-bound (phys_fn_004107 with
// suppressAttach false) only when a body differs from the one held; the
// second body is not looked at when the first already differs.
void SphericalJoint::loadFromDesc(const NxSphericalJointDesc& desc)
	{
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_SPHERICALJOINT_CPP, 0x39, 0,
			"SphericalJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_SPHERICALJOINT_CPP, 0x3a, 0,
			"SphericalJoint::loadFromDesc: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	void* actorImpl0 = sphericalActorImpl(desc.actor[0]);
	void* actorImpl1 = sphericalActorImpl(desc.actor[1]);
	if(sphericalBodyOfActorImpl(actorImpl0) != mBody[0] || sphericalBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	row004284(desc);
	}

// phys_fn_004306 (0x000a4ac0, 1075 B)
// The twist angle, after the stale-body refresh. With a_i body i's world
// axis and n_i its world normal (rotated by the body's +0x134 3x3, as stored
// without the body): coneFactor = dot(a0, a1) + 1 when the dot is negative,
// otherwise 1; the half-way axis h = normalize(a0 + a1) is written out; then
// c = normalize(h x n0) (left as is when its length is 0), b = c x h and the
// result is -fpatan(c . n1, b . n1), left unrounded in st(0).
// Listing over decompile: the decompile shows a0.x, the normals and h.x/h.y
// as floats and the rest on the stack, which matches; it writes h.z from the
// scaled value where the listing stores the unrounded product (0xa4e17),
// and reorders the normal sums. The listing's grouping is kept.
NxF64 SphericalJoint::row004306(NxVec3& halfAxis, NxReal& coneFactor)
	{
	sphericalRefreshFirstStaleBody(*this);

	// Body 0's axis (a0.x stored, a0.y/a0.z on the stack) and normal
	// (stored), 0xa4af7-0xa4c32.
	NxReal a0x;
	double a0y, a0z;
	NxVec3 n0;
	const JointBodyRecord* body0 = sphericalBody(mBody[0]);
	if(!body0)
		{
		a0x = mWorldAxis[0].x;
		a0y = mWorldAxis[0].y;
		a0z = mWorldAxis[0].z;
		n0 = mWorldNormal[0];
		}
	else
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& a = mWorldAxis[0];
		a0x = (NxReal)sphericalSum3(m[1], a.y, m[2], a.z, m[0], a.x);
		a0y = sphericalSum3(m[4], a.y, m[5], a.z, m[3], a.x);
		a0z = sphericalSum3(m[7], a.y, m[8], a.z, m[6], a.x);
		const NxVec3& n = mWorldNormal[0];
		n0.x = (NxReal)sphericalSum3(m[2], n.z, m[1], n.y, m[0], n.x);
		n0.y = (NxReal)sphericalSum3(m[5], n.z, m[3], n.x, m[4], n.y);
		n0.z = (NxReal)sphericalSum3(m[8], n.z, m[6], n.x, m[7], n.y);
		}

	// Body 1's axis (a1.x/a1.y on the stack, a1.z stored) and normal
	// (stored), 0xa4c36-0xa4d72.
	double a1x, a1y;
	NxReal a1z;
	NxVec3 n1;
	const JointBodyRecord* body1 = sphericalBody(mBody[1]);
	if(!body1)
		{
		a1z = mWorldAxis[1].z;
		a1x = mWorldAxis[1].x;
		a1y = mWorldAxis[1].y;
		n1 = mWorldNormal[1];
		}
	else
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& a = mWorldAxis[1];
		a1x = sphericalSum3(m[2], a.z, m[1], a.y, m[0], a.x);
		a1y = sphericalSum3(m[5], a.z, m[4], a.y, m[3], a.x);
		a1z = (NxReal)sphericalSum3(m[8], a.z, m[7], a.y, m[6], a.x);
		const NxVec3& n = mWorldNormal[1];
		n1.x = (NxReal)sphericalSum3(m[2], n.z, m[1], n.y, n.x, m[0]);
		n1.y = (NxReal)sphericalSum3(m[3], n.x, m[5], n.z, m[4], n.y);
		n1.z = (NxReal)sphericalSum3(m[6], n.x, m[8], n.z, m[7], n.y);
		}

	// 0xa4d76-0xa4db3: `fcom 0; test ah,5; jp` takes the 1.0 arm unless the
	// dot is below zero.
	const double dot = ((double)a1z * a0z + a1y * a0y) + a1x * a0x;
	if(dot < 0.0)
		coneFactor = (NxReal)(dot + 1.0f);
	else
		coneFactor = 1.0f;

	// The sum, stored except for z, normalised (0xa4db3-0xa4e17).
	const NxReal sx = (NxReal)(a1x + a0x);
	const NxReal sy = (NxReal)(a1y + a0y);
	const double sz = (double)a1z + a0z;
	halfAxis.y = sy;
	halfAxis.x = sx;
	halfAxis.z = (NxReal)sz;
	const double inverse = 1.0f / x87FsqrtDot3(sz, sz, sy, sy, sx, sx);
	const NxReal hx = (NxReal)(sx * inverse);
	halfAxis.x = hx;
	const NxReal hy = (NxReal)(sy * inverse);
	halfAxis.y = hy;
	const double hz = inverse * sz;
	halfAxis.z = (NxReal)hz;

	// c = h x n0 on the stack, normalised when its length is not 0
	// (0xa4e1a-0xa4e87).
	double cx = (double)n0.z * hy - n0.y * hz;
	double cy = hz * n0.x - sphericalMul(n0.z, hx);
	double cz = sphericalMul(n0.y, hx) - sphericalMul(hy, n0.x);
	const double length = x87FsqrtDot3(cz, cz, cy, cy, cx, cx);
	if(length != 0.0)
		{
		const double scale = 1.0f / length;
		cx = cx * scale;
		cy = cy * scale;
		cz = cz * scale;
		}

	// b = c x h, stored (0xa4e87-0xa4eb6).
	const NxReal bx = (NxReal)(cy * hz - cz * hy);
	const NxReal by = (NxReal)(hx * cz - hz * cx);
	const NxReal bz = (NxReal)(hy * cx - cy * hx);

	const double y = (cz * n1.z + cy * n1.y) + cx * n1.x;
	const double x = (sphericalMul(bz, n1.z) + sphericalMul(by, n1.y)) + sphericalMul(bx, n1.x);
	return -sphericalFpatan(y, x);
	}

// phys_fn_004308 (0x000a4f00, 1107 B)
// `ret 4`, but the listing never reads the argument. revolute 004374's
// instructions over the spherical fields: v is the velocity error that
// row004294 returns, scaled per axis by mSpringGain when the joint
// spring is on (flag 0x10, 0xa4f1f-0xa4f4e; not in revolute). When maxForce
// is finite, a = v + mAccumulatedImpulse is compared with
// mMaxImpulseSquared: above it the joint breaks (+0x2c bits 3/4 = broken, a
// break event carrying |a| is posted to the Scene through row 000571), a is
// scaled back to length sqrt(mMaxImpulseSquared), v becomes the scaled a
// minus the old accumulator and the accumulator takes the scaled a;
// otherwise the accumulator = a. Then t = mInverseMass * -(v + mBias) is
// applied to body 0's record with lever mLever[0] and -t to body 1's with
// lever mLever[1] (sphericalApplyToRecord). The negations are `fchs` where
// revolute multiplies by -1.0f (the same values).
// Listing over decompile: the decompile shows every intermediate as a float;
// the listing keeps a.z's sum, the ratio's first product, t.z and several
// partial sums on the stack, as named double below.
// Driving this row requires body +0x204 (row004294 and the two record
// updates dereference it), which the candidate's body record never writes.
void SphericalJoint::row_slot0(NxU32 arg)
	{
	(void)arg;
	if((mFlags & 0x18) == 0x10)
		return;

	NxVec3 v;
	row004294(v);
	if(mSphericalFlags & NX_SJF_JOINT_SPRING_ENABLED)
		{
		v.x = (NxReal)sphericalMul(v.x, mSpringGain.x);
		v.y = (NxReal)sphericalMul(v.y, mSpringGain.y);
		v.z = (NxReal)sphericalMul(v.z, mSpringGain.z);
		}

	if(mMaxForce < NX_MAX_REAL)
		{
		NxVec3 a;
		a.x = (NxReal)((double)v.x + mAccumulatedImpulse.x);
		a.y = (NxReal)((double)v.y + mAccumulatedImpulse.y);
		const double az = (double)v.z + mAccumulatedImpulse.z;
		a.z = (NxReal)az;
		const double lengthSquared = ((az * a.z) + sphericalMul(a.y, a.y)) + sphericalMul(a.x, a.x);
		if(lengthSquared > mMaxImpulseSquared)
			{
			// fsqrt of the compared sum (0xa4fb9), re-formed from the same
			// operands in the same order (X87Sqrt.h).
			const NxReal length = (NxReal)x87FsqrtDot3(az, a.z, a.y, a.y, a.x, a.x);
			mFlags = (mFlags & ~8u) | 0x10;
			void* memory = nxGetSdkAllocator()->malloc(sizeof(JointBreakEvent), NX_MEMORY_PERSISTENT);
			JointBreakEvent* event = memory ? new(memory) JointBreakEvent(this, length) : 0;
			// Scene row 000571 is deferred (owner Scene.cpp); its stub asserts.
			reinterpret_cast<Row000571Fixture*>(mScene)->row000571(event);
			const double ratio = x87Fsqrt(mMaxImpulseSquared) / length;
			const NxReal ratioF = (NxReal)ratio;
			const double sx = ratio * a.x;
			const double sy = sphericalMul(a.y, ratioF);
			const NxReal sz = (NxReal)sphericalMul(a.z, ratioF);
			const double dx = sx - mAccumulatedImpulse.x;
			const double dy = sy - mAccumulatedImpulse.y;
			v.z = (NxReal)((double)sz - mAccumulatedImpulse.z);
			v.x = (NxReal)dx;
			v.y = (NxReal)dy;
			mAccumulatedImpulse.x = (NxReal)sx;
			mAccumulatedImpulse.y = (NxReal)sy;
			mAccumulatedImpulse.z = sz;
			}
		else
			{
			mAccumulatedImpulse.x = a.x;
			mAccumulatedImpulse.y = a.y;
			mAccumulatedImpulse.z = a.z;
			}
		}

	const double px = (double)v.x + mBias.x;
	const double py = (double)v.y + mBias.y;
	const NxReal pz = (NxReal)((double)v.z + mBias.z);
	const NxReal nx = (NxReal)-px;
	const double ny = -py;
	const double nz = -(double)pz;

	const NxReal* M = mInverseMass;
	const NxReal tx = (NxReal)((nz * M[2] + ny * M[1]) + (double)nx * M[0]);
	const NxReal ty = (NxReal)((nz * M[5] + ny * M[4]) + (double)nx * M[3]);
	const double tz = (nz * M[8] + ny * M[7]) + (double)nx * M[6];

	NxVec3 c;
	const NxVec3& r0 = mLever[0];
	c.x = (NxReal)(tz * r0.y - sphericalMul(ty, r0.z));
	c.y = (NxReal)(sphericalMul(tx, r0.z) - tz * r0.x);
	c.z = (NxReal)(sphericalMul(ty, r0.x) - sphericalMul(tx, r0.y));
	const JointBodyRecord* body0 = sphericalBody(mBody[0]);
	if(body0)
		sphericalApplyToRecord(body0->mUnknown204, tx, ty, tz, c);

	const NxReal sx = -tx;
	const NxReal sy = -ty;
	const double szRegister = -tz;
	const NxReal sz = (NxReal)szRegister;
	const NxVec3& r1 = mLever[1];
	c.x = (NxReal)(szRegister * r1.y - sphericalMul(sy, r1.z));
	c.y = (NxReal)(sphericalMul(sx, r1.z) - sphericalMul(sz, r1.x));
	c.z = (NxReal)(sphericalMul(sy, r1.x) - sphericalMul(sx, r1.y));
	const JointBodyRecord* body1 = sphericalBody(mBody[1]);
	if(body1)
		sphericalApplyToRecord(body1->mUnknown204, sx, sy, sz, c);
	}

// phys_fn_004310 (0x000a5360, 2942 B)
// Fills constraint records (Joint::row004093) for the springs and limits the
// flags enable, then runs the Joint base's slot 7 (row 004135, called
// directly at 0xa5ecf). a1 is body 1's world axis and s the world swing axis
// carried through body 0's +0x134 3x3 (each as stored without its body);
// swingCos = s . a1; the twist angle and cone factor come from 004306.
// - Twist spring (flag 4): a kind-3 record along a1, +0x34 = ((twist -
//   targetValue) / arg) * coneFactor, solved through 004393 with
//   (1 / ((spring * arg + damper) * arg), spring * arg / (spring * arg +
//   damper)), the denominator stored as a float first.
// - Swing spring (flag 8, |a1 x s| > 0.0001): two kind-3 records along the
//   normalised p = a1 x s and q = p x s, the first with +0x34 = -((acos
//   (swingCos) - targetValue) / arg), the second with 0, both solved through
//   004393 with the swing spring's gains.
// - Twist limit (flag 1): low == high -> unless twist == high, a kind-3 lock
//   record along a1 with +0x34 = ((twist - high) / arg) * coneFactor; twist
//   below low -> a kind-2 record along a1 with ((twist - low) / arg) *
//   coneFactor; above high -> along -a1 with ((twist - high) * (-1 / arg)) *
//   coneFactor; the kind-2 records get the limit's restitution.
// - Swing limit (flag 2, swingCos < mSwingLimitCos): a kind-2 record along
//   the normalised a1 x s with +0x34 = (acos(swingCos) - swingLimit.value) *
//   (-1 / arg) and the swing limit's restitution.
// Listing over decompile: the decompile drops the kind tests as unreachable
// (0x100a55e5, 0x100a57ba, ...) and shows the swing-spring gains and the
// cross products' squared lengths as floats; the listing keeps the values
// named double below on the stack (the swing spring's denominator is not
// stored, the twist spring's is; the second cross product's z term squares
// the unrounded value against its stored float, 0xa5866).
void SphericalJoint::row_slot7(NxReal arg)
	{
	const JointBodyRecord* body0 = sphericalBody(mBody[0]);
	JointSupportBody* record0 = body0 ? body0->mUnknown204 : 0;
	const JointBodyRecord* body1 = sphericalBody(mBody[1]);
	JointSupportBody* record1 = body1 ? body1->mUnknown204 : 0;

	NxReal twist = 0.0f;
	NxReal coneFactor = 0.0f;
	NxVec3 halfAxis;
	if(mSphericalFlags & (NX_SJF_TWIST_LIMIT_ENABLED | NX_SJF_TWIST_SPRING_ENABLED))
		twist = (NxReal)row004306(halfAxis, coneFactor);

	const NxU32 flags = mSphericalFlags;
	NxVec3 a1(0.0f, 0.0f, 0.0f);
	if((flags & NX_SJF_SWING_LIMIT_ENABLED) || (flags & 0xd))
		{
		// 0xa53ce-0xa5479.
		body1 = sphericalBody(mBody[1]);
		if(body1)
			{
			const NxReal* m = body1->mUnknown134;
			const NxVec3& a = mWorldAxis[1];
			a1.x = (NxReal)sphericalSum3(m[1], a.y, m[2], a.z, m[0], a.x);
			a1.y = (NxReal)sphericalSum3(m[4], a.y, m[5], a.z, m[3], a.x);
			a1.z = (NxReal)sphericalSum3(m[7], a.y, m[8], a.z, m[6], a.x);
			}
		else
			{
			a1 = mWorldAxis[1];
			}
		}
	NxVec3 s(0.0f, 0.0f, 0.0f);
	NxReal swingCos = 0.0f;
	if((flags & NX_SJF_SWING_LIMIT_ENABLED) || (flags & NX_SJF_SWING_SPRING_ENABLED))
		{
		// 0xa548a-0xa5555.
		body0 = sphericalBody(mBody[0]);
		if(body0)
			{
			const NxReal* m = body0->mUnknown134;
			const NxVec3& w = mSwingAxisWorld;
			s.x = (NxReal)sphericalSum3(m[1], w.y, m[2], w.z, m[0], w.x);
			s.y = (NxReal)sphericalSum3(m[4], w.y, m[5], w.z, m[3], w.x);
			s.z = (NxReal)sphericalSum3(m[7], w.y, m[8], w.z, m[6], w.x);
			}
		else
			{
			s = mSwingAxisWorld;
			}
		swingCos = (NxReal)sphericalSum3(s.x, a1.x, s.z, a1.z, a1.y, s.y);
		}

	if(flags & NX_SJF_TWIST_SPRING_ENABLED)
		{
		// 0xa5562-0xa564e.
		const double stiffness = sphericalMul(mTwistSpring.spring, arg);
		const NxReal denominator = (NxReal)(stiffness + mTwistSpring.damper);
		const NxReal ratio = (NxReal)(stiffness / denominator);
		const NxReal error = (NxReal)((((double)twist - mTwistSpring.targetValue) / arg) * coneFactor);
		JointSupportRecord* record = row004093();
		const NxReal scale = (NxReal)(1.0f / sphericalMul(denominator, arg));
		sphericalRecordHeader(record, record0, record1, a1);
		sphericalSpringBits(record, 0xffffffe3, 3);
		sphericalSpringTail(record, this, error, NX_MAX_REAL, scale, ratio);
		}

	if(mSphericalFlags & NX_SJF_SWING_SPRING_ENABLED)
		{
		// p = a1 x s, stored, normalised when its length is not 0
		// (0xa5660-0xa56f7).
		NxVec3 p;
		p.x = (NxReal)(sphericalMul(a1.z, s.y) - sphericalMul(s.z, a1.y));
		p.y = (NxReal)(sphericalMul(s.z, a1.x) - sphericalMul(a1.z, s.x));
		p.z = (NxReal)(sphericalMul(a1.y, s.x) - sphericalMul(s.y, a1.x));
		const double length = x87FsqrtDot3(p.x, p.x, p.z, p.z, p.y, p.y);
		if(length != 0.0)
			{
			const double inverse = 1.0f / length;
			p.x = (NxReal)(p.x * inverse);
			p.y = (NxReal)(p.y * inverse);
			p.z = (NxReal)(p.z * inverse);
			}
		// `fcomp 0.0001f; test ah,0x41; jne`: only above the threshold.
		if(length > 0.0001f)
			{
			const double angle = jointAcos(swingCos);
			const NxReal error = (NxReal)-((angle - mSwingSpring.targetValue) / arg);
			const double stiffness = sphericalMul(mSwingSpring.spring, arg);
			const double denominator = stiffness + mSwingSpring.damper;
			const NxReal ratio = (NxReal)(stiffness / denominator);
			const NxReal scale = (NxReal)(1.0f / (denominator * arg));
			JointSupportRecord* record = row004093();
			sphericalRecordHeader(record, record0, record1, p);
			sphericalSpringBits(record, 0xffffffe3, 3);
			sphericalSpringTail(record, this, error, NX_MAX_REAL, scale, ratio);

			// q = p x s (0xa5828-0xa58bb): q.z's square multiplies the
			// unrounded value by its stored float.
			NxVec3 q;
			q.x = (NxReal)(sphericalMul(p.z, s.y) - sphericalMul(p.y, s.z));
			q.y = (NxReal)(sphericalMul(s.z, p.x) - sphericalMul(p.z, s.x));
			const double qz = sphericalMul(p.y, s.x) - sphericalMul(s.y, p.x);
			q.z = (NxReal)qz;
			const double qLength = x87FsqrtDot3(qz, q.z, q.y, q.y, q.x, q.x);
			if(qLength != 0.0)
				{
				const double inverse = 1.0f / qLength;
				q.x = (NxReal)(q.x * inverse);
				q.y = (NxReal)(q.y * inverse);
				q.z = (NxReal)(q.z * inverse);
				}
			record = row004093();
			sphericalRecordHeader(record, record0, record1, q);
			sphericalSpringBits(record, 0xffffffe3, 3);
			sphericalSpringTail(record, this, 0.0f, NX_MAX_REAL, scale, ratio);
			}
		}

	if(mSphericalFlags & NX_SJF_TWIST_LIMIT_ENABLED)
		{
		const NxJointLimitDesc& low = mTwistLimit.low;
		const NxJointLimitDesc& high = mTwistLimit.high;
		if(low.value == high.value)
			{
			// 0xa598c-0xa5a77: a lock record unless the twist is exactly there.
			if(high.value != twist)
				{
				const NxReal error = (NxReal)((((double)twist - high.value) / arg) * coneFactor);
				JointSupportRecord* record = row004093();
				sphericalRecordHeader(record, record0, record1, a1);
				sphericalLimitBits(record, 0xffffffe3, 3);
				sphericalLimitTail(record, this, error);
				}
			}
		else if(twist < low.value)
			{
			// 0xa5a8d-0xa5b98.
			const NxReal error = (NxReal)((((double)twist - low.value) / arg) * coneFactor);
			JointSupportRecord* record = row004093();
			sphericalRecordHeader(record, record0, record1, a1);
			sphericalLimitBits(record, 0xffffffe2, 2);
			sphericalLimitTail(record, this, error);
			sphericalRestitution(record, low.restitution);
			}
		else if(high.value < twist)
			{
			// 0xa5bb2-0xa5ccd.
			const NxReal error = (NxReal)((((double)twist - high.value) * (-1.0f / (double)arg)) * coneFactor);
			JointSupportRecord* record = row004093();
			NxVec3 axis;
			axis.x = -a1.x;
			axis.y = -a1.y;
			axis.z = -a1.z;
			sphericalRecordHeader(record, record0, record1, axis);
			sphericalLimitBits(record, 0xffffffe2, 2);
			sphericalLimitTail(record, this, error);
			sphericalRestitution(record, high.restitution);
			}
		}

	if((mSphericalFlags & NX_SJF_SWING_LIMIT_ENABLED) && swingCos < mSwingLimitCos)
		{
		// 0xa5cf3-0xa5ec4: the normalised a1 x s; its z term squares the
		// unrounded value against its stored float (0xa5d31-0xa5d35).
		NxVec3 p;
		p.x = (NxReal)(sphericalMul(a1.z, s.y) - sphericalMul(s.z, a1.y));
		p.y = (NxReal)(sphericalMul(s.z, a1.x) - sphericalMul(a1.z, s.x));
		const double pz = sphericalMul(a1.y, s.x) - sphericalMul(s.y, a1.x);
		p.z = (NxReal)pz;
		const double length = x87FsqrtDot3(pz, p.z, p.y, p.y, p.x, p.x);
		if(length != 0.0)
			{
			const double inverse = 1.0f / length;
			p.x = (NxReal)(p.x * inverse);
			p.y = (NxReal)(p.y * inverse);
			p.z = (NxReal)(p.z * inverse);
			}
		const double angle = jointAcos(swingCos);
		const NxReal error = (NxReal)((angle - mSwingLimit.value) * (-1.0f / (double)arg));
		JointSupportRecord* record = row004093();
		sphericalRecordHeader(record, record0, record1, p);
		sphericalLimitBits(record, 0xffffffe2, 2);
		sphericalLimitTail(record, this, error);
		sphericalRestitution(record, mSwingLimit.restitution);
		}

	// Joint base slot 7 (row 004135), called directly as 0xa5ecf does.
	Joint::row_slot7(arg);
	}

// phys_fn_004314 (0x000a7050, 430 B)
// Not a function: the swing-limit loop and epilogue of phys_fn_004312 below.
// 004312's last instruction jumps to it (`jmp 0x100a7050`, 0xa704b); it
// loops back to itself (0xa71eb) and ends by popping 004312's registers and
// frame (`add esp,0xe8; ret 4`, 0xa71f5). Ghidra's supplement body for
// 004312 covers it (0xa7050-0xa71fe). Written as the second loop below.
// phys_fn_004312 (0x000a5ee0, 4461 B)
// Debug visualization, only when +0x2c bit 9 (NX_JF_VISUALIZATION) is set,
// after the stale-body refresh. Each part is gated by its SDK parameter being
// non-zero and scaled by it times NX_VISUALIZATION_SCALE:
// - world axes (32): three lines through row004123's point, +/- the scale
//   along x (0xff0000), y (0xff00) and z (0xff);
// - local axes (31): per body, the anchor, normal, cross and axis carried
//   through the body's +0x134/+0x158 pose (as stored without the body), in
//   four two-element NxVec3 arrays built through the compiler's `eh vector
//   constructor iterator` (phys_fn_000001 with the folded NxVec3 constructor
//   001391); arrows along body 0's normal/cross/axis (0x902020, 0x209020,
//   0x202090) and body 1's (0xe05050, 0x50e050, 0x5050e0), then a yellow
//   line between the anchors;
// - limits (33): the twist limit (flag 1) as revolute's arc -- 13 points
//   between low and high about body 0's world frame, offset by -0.1 times the
//   world swing axis, the end spokes coloured by whether the twist
//   (phys_fn_004306) is past that limit -- and an arrow at the twist angle
//   (0xff00d0); the swing limit (flag 2) as an arrow along body 1's world
//   axis and a cone of 24 spokes (004314) of radius tan(acos(cos limit))
//   (1000 for |cos| <= 0.01) in the frame (t1, t2, s) of the world swing
//   axis s (NxNormalToTangents), coloured by whether s . a1 is below the
//   limit cosine.
// Listing over decompile (supplement): the decompile shows the frame sums
// as floats with reordered terms; the listing's groupings and the values it
// keeps on the stack (named double below) are followed.
void SphericalJoint::row_slot4(NxDebugRenderable& renderable)
	{
	if(!((mFlags >> 9) & 1))
		return;

	sphericalRefreshFirstStaleBody(*this);

	if(sphericalSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES) != 0.0f)
		{
		// 0xa5f47-0xa603e.
		NxVec3 point;
		row004123(point);
		const NxReal scale = (NxReal)((double)sphericalSdkParameter(NX_VISUALIZATION_SCALE) *
			sphericalSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES));
		NxVec3 p0;
		NxVec3 p1;
		p1.y = point.y;
		p1.x = (NxReal)((double)point.x + scale);
		p0.y = point.y;
		p0.z = point.z;
		p0.x = (NxReal)((double)point.x - scale);
		p1.z = point.z;
		renderable.addLine(p0, p1, 0xff0000);
		p1.y = (NxReal)((double)point.y + scale);
		p1.x = point.x;
		p0.y = (NxReal)((double)point.y - scale);
		p0.x = point.x;
		p0.z = point.z;
		p1.z = point.z;
		renderable.addLine(p0, p1, 0xff00);
		p1.z = (NxReal)((double)point.z + scale);
		p1.x = point.x;
		p0.z = (NxReal)((double)point.z - scale);
		p1.y = point.y;
		p0.y = point.y;
		p0.x = point.x;
		renderable.addLine(p0, p1, 0xff);
		}

	if(sphericalSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) != 0.0f)
		{
		NxVec3 anchor[2];
		NxVec3 axis[2];
		NxVec3 normal[2];
		NxVec3 cross[2];

		const JointBodyRecord* body0 = sphericalBody(mBody[0]);
		if(!body0)
			{
			anchor[0] = mWorldAnchor[0];
			axis[0] = mWorldAxis[0];
			normal[0] = mWorldNormal[0];
			cross[0] = mWorldCross[0];
			}
		else
			{
			// 0xa6146-0xa639f.
			const NxReal* m = body0->mUnknown134;
			const NxVec3& t = body0->mUnknown158;
			const NxVec3& p = mWorldAnchor[0];
			const double px = sphericalSum3(m[1], p.y, m[2], p.z, m[0], p.x);
			const NxReal py = (NxReal)sphericalSum3(m[4], p.y, m[3], p.x, m[5], p.z);
			const NxReal pz = (NxReal)sphericalSum3(m[7], p.y, m[6], p.x, m[8], p.z);
			anchor[0].x = (NxReal)(px + t.x);
			anchor[0].y = (NxReal)((double)py + t.y);
			anchor[0].z = (NxReal)((double)pz + t.z);
			const NxVec3& w = mWorldAxis[0];
			axis[0].x = (NxReal)sphericalSum3(m[2], w.z, m[1], w.y, w.x, m[0]);
			axis[0].y = (NxReal)sphericalSum3(m[5], w.z, m[4], w.y, m[3], w.x);
			axis[0].z = (NxReal)sphericalSum3(m[8], w.z, m[7], w.y, m[6], w.x);
			const NxVec3& n = mWorldNormal[0];
			normal[0].x = (NxReal)sphericalSum3(m[1], n.y, m[2], n.z, m[0], n.x);
			normal[0].y = (NxReal)sphericalSum3(m[4], n.y, m[3], n.x, m[5], n.z);
			normal[0].z = (NxReal)sphericalSum3(m[7], n.y, m[6], n.x, m[8], n.z);
			const NxVec3& c = mWorldCross[0];
			cross[0].x = (NxReal)sphericalSum3(m[2], c.z, m[1], c.y, c.x, m[0]);
			cross[0].y = (NxReal)sphericalSum3(m[5], c.z, m[4], c.y, m[3], c.x);
			cross[0].z = (NxReal)sphericalSum3(m[8], c.z, m[7], c.y, m[6], c.x);
			}

		const JointBodyRecord* body1 = sphericalBody(mBody[1]);
		if(!body1)
			{
			anchor[1] = mWorldAnchor[1];
			axis[1] = mWorldAxis[1];
			normal[1] = mWorldNormal[1];
			cross[1] = mWorldCross[1];
			}
		else
			{
			// 0xa643d-0xa6693.
			const NxReal* m = body1->mUnknown134;
			const NxVec3& t = body1->mUnknown158;
			const NxVec3& p = mWorldAnchor[1];
			const double px = sphericalSum3(m[2], p.z, m[1], p.y, m[0], p.x);
			const NxReal py = (NxReal)sphericalSum3(m[5], p.z, m[3], p.x, m[4], p.y);
			const NxReal pz = (NxReal)sphericalSum3(m[8], p.z, m[6], p.x, m[7], p.y);
			anchor[1].x = (NxReal)(px + t.x);
			anchor[1].y = (NxReal)((double)py + t.y);
			anchor[1].z = (NxReal)((double)pz + t.z);
			const NxVec3& w = mWorldAxis[1];
			axis[1].x = (NxReal)sphericalSum3(m[2], w.z, m[1], w.y, m[0], w.x);
			axis[1].y = (NxReal)sphericalSum3(m[5], w.z, m[4], w.y, m[3], w.x);
			axis[1].z = (NxReal)sphericalSum3(m[8], w.z, m[7], w.y, m[6], w.x);
			const NxVec3& n = mWorldNormal[1];
			normal[1].x = (NxReal)sphericalSum3(m[1], n.y, m[2], n.z, m[0], n.x);
			normal[1].y = (NxReal)sphericalSum3(m[4], n.y, m[5], n.z, m[3], n.x);
			normal[1].z = (NxReal)sphericalSum3(m[7], n.y, m[8], n.z, m[6], n.x);
			const NxVec3& c = mWorldCross[1];
			cross[1].x = (NxReal)sphericalSum3(m[1], c.y, m[2], c.z, m[0], c.x);
			cross[1].y = (NxReal)sphericalSum3(m[3], c.x, m[4], c.y, m[5], c.z);
			cross[1].z = (NxReal)sphericalSum3(m[6], c.x, m[7], c.y, m[8], c.z);
			}

		// 0xa669a-0xa677e.
		const NxReal scale = (NxReal)((double)sphericalSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) *
			sphericalSdkParameter(NX_VISUALIZATION_SCALE));
		renderable.addArrow(anchor[0], normal[0], 1.0f, scale, 0x902020);
		renderable.addArrow(anchor[0], cross[0], 1.0f, scale, 0x209020);
		renderable.addArrow(anchor[0], axis[0], 1.0f, scale, 0x202090);
		renderable.addArrow(anchor[1], normal[1], 1.0f, scale, 0xe05050);
		renderable.addArrow(anchor[1], cross[1], 1.0f, scale, 0x50e050);
		renderable.addArrow(anchor[1], axis[1], 1.0f, scale, 0x5050e0);
		renderable.addLine(anchor[0], anchor[1], 0xffff00);
		}

	if(sphericalSdkParameter(NX_VISUALIZE_JOINT_LIMITS) == 0.0f)
		return;

	const NxReal scale = (NxReal)((double)sphericalSdkParameter(NX_VISUALIZE_JOINT_LIMITS) *
		sphericalSdkParameter(NX_VISUALIZATION_SCALE));

	if(mSphericalFlags & NX_SJF_TWIST_LIMIT_ENABLED)
		{
		NxVec3 center;
		row004123(center);
		NxVec3 halfAxis;
		NxReal coneFactor;
		const double twistValue = row004306(halfAxis, coneFactor);
		const NxReal twist = (NxReal)twistValue;
		// 1 when the twist is past that limit (0xa67d5-0xa6817): the low
		// test compares the unrounded angle, the high test the stored float.
		NxU32 beyond[2];
		beyond[0] = twistValue < mTwistLimit.low.value ? 1 : 0;
		beyond[1] = twist > mTwistLimit.high.value ? 1 : 0;

		// Body 0's world frame, stored as the columns of a 3x3 (normal and
		// cross; the axis stays on the stack), 0xa6817-0xa6a34.
		NxVec3 n, c;
		double ax, ay, az;
		const JointBodyRecord* body0 = sphericalBody(mBody[0]);
		if(body0)
			{
			const NxReal* m = body0->mUnknown134;
			const NxVec3& wn = mWorldNormal[0];
			n.x = (NxReal)sphericalSum3(m[1], wn.y, m[2], wn.z, m[0], wn.x);
			n.y = (NxReal)sphericalSum3(m[4], wn.y, m[3], wn.x, m[5], wn.z);
			n.z = (NxReal)sphericalSum3(m[7], wn.y, m[6], wn.x, m[8], wn.z);
			const NxVec3& wc = mWorldCross[0];
			c.x = (NxReal)sphericalSum3(m[2], wc.z, m[1], wc.y, wc.x, m[0]);
			c.y = (NxReal)sphericalSum3(m[5], wc.z, m[4], wc.y, m[3], wc.x);
			c.z = (NxReal)sphericalSum3(m[8], wc.z, m[7], wc.y, m[6], wc.x);
			const NxVec3& wa = mWorldAxis[0];
			ax = (NxReal)sphericalSum3(m[1], wa.y, m[2], wa.z, wa.x, m[0]);
			ay = (NxReal)sphericalSum3(m[3], wa.x, m[4], wa.y, m[5], wa.z);
			az = (NxReal)sphericalSum3(m[6], wa.x, m[7], wa.y, m[8], wa.z);
			}
		else
			{
			n = mWorldNormal[0];
			ax = mWorldAxis[0].x;
			ay = mWorldAxis[0].y;
			az = mWorldAxis[0].z;
			c = mWorldCross[0];
			}

		// The world swing axis (0xa6a3a-0xa6ad4): x and y on the stack, z
		// stored.
		double sx, sy;
		NxReal sz;
		body0 = sphericalBody(mBody[0]);
		if(body0)
			{
			const NxReal* m = body0->mUnknown134;
			const NxVec3& w = mSwingAxisWorld;
			sx = sphericalSum3(m[1], w.y, m[2], w.z, m[0], w.x);
			sy = sphericalSum3(m[4], w.y, m[3], w.x, m[5], w.z);
			sz = (NxReal)sphericalSum3(m[7], w.y, m[6], w.x, m[8], w.z);
			}
		else
			{
			sz = mSwingAxisWorld.z;
			sx = mSwingAxisWorld.x;
			sy = mSwingAxisWorld.y;
			}

		// The arc's centre moves by -0.1 times the swing axis (the float at
		// 0x1011a028) times the scale (0xa6ad4-0xa6b37).
		const NxReal offsetX = (NxReal)(sx * -0.1f);
		const double offsetY = sy * -0.1f;
		const double offsetZ = (double)sz * -0.1f;
		const NxReal ox = (NxReal)sphericalMul(offsetX, scale);
		const NxReal oy = (NxReal)(offsetY * scale);
		const double oz = offsetZ * scale;
		center.x = (NxReal)((double)ox + center.x);
		center.y = (NxReal)((double)center.y + oy);
		center.z = (NxReal)((double)center.z + oz);

		// The axis column is multiplied by the 0.0f at 0x101041f0, not
		// dropped (0xa6b39-0xa6b58).
		const NxReal axisY = (NxReal)(ay * 0.0f);
		const NxReal axisZ = (NxReal)(az * 0.0f);
		const NxReal axisX = (NxReal)(ax * 0.0f);

		// i is unsigned (fild with the 2^32 correction, 0xa6b66-0xa6b6c);
		// the step is 1/12 as a float (0x3daaaaab at 0x101068e0).
		NxVec3 previous;
		for(NxU32 i = 0; i <= 12; i++)
			{
			const double t = (double)i * 0.083333336f;
			const double arcAngle = (1.0f - t) * mTwistLimit.low.value + t * mTwistLimit.high.value;
			double cosine;
			double sine;
			sphericalFcosFsin(arcAngle, cosine, sine);
			const NxReal ry = (NxReal)((c.y * sine + n.y * cosine) + axisY);
			const NxReal rz = (NxReal)((c.z * sine + n.z * cosine) + axisZ);
			const double rx = (c.x * sine + n.x * cosine) + axisX;
			const NxReal dy = (NxReal)sphericalMul(ry, scale);
			const NxReal dz = (NxReal)sphericalMul(rz, scale);
			NxVec3 point;
			point.x = (NxReal)(rx * scale + center.x);
			point.y = (NxReal)((double)dy + center.y);
			point.z = (NxReal)((double)dz + center.z);
			if(i == 0 || i == 12)
				renderable.addLine(center, point, beyond[i / 12] * 0xd000 + 0xff0000);
			if(i > 0)
				renderable.addLine(previous, point, 0xff0000);
			previous = point;
			}

		// The twist arrow (0xa6cbc-0xa6d4a): cos of the stored twist on the
		// stack, sin stored.
		const double cosine = sphericalFcos(twist);
		const NxReal sine = (NxReal)sphericalFsin(twist);
		NxVec3 direction;
		const double dirY = (sphericalMul(c.y, sine) + n.y * cosine) + axisY;
		const double dirZ = (sphericalMul(c.z, sine) + n.z * cosine) + axisZ;
		direction.x = (NxReal)((sphericalMul(c.x, sine) + n.x * cosine) + axisX);
		direction.y = (NxReal)dirY;
		direction.z = (NxReal)dirZ;
		renderable.addArrow(center, direction, 1.0f, scale, 0xff00d0);
		}

	if(!(mSphericalFlags & NX_SJF_SWING_LIMIT_ENABLED))
		return;

	NxVec3 center;
	row004123(center);

	// Body 1's world axis (0xa6d66-0xa6e13), stored.
	NxVec3 a1;
	const JointBodyRecord* body1 = sphericalBody(mBody[1]);
	if(body1)
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& a = mWorldAxis[1];
		a1.x = (NxReal)sphericalSum3(m[1], a.y, m[2], a.z, a.x, m[0]);
		a1.y = (NxReal)sphericalSum3(m[4], a.y, m[3], a.x, m[5], a.z);
		a1.z = (NxReal)sphericalSum3(m[7], a.y, m[6], a.x, m[8], a.z);
		}
	else
		{
		a1 = mWorldAxis[1];
		}

	// The world swing axis (0xa6e17-0xa6ec3), stored.
	NxVec3 s;
	const JointBodyRecord* body0 = sphericalBody(mBody[0]);
	if(body0)
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& w = mSwingAxisWorld;
		s.x = (NxReal)sphericalSum3(m[2], w.z, m[1], w.y, w.x, m[0]);
		s.y = (NxReal)sphericalSum3(m[5], w.z, m[3], w.x, m[4], w.y);
		s.z = (NxReal)sphericalSum3(m[8], w.z, m[6], w.x, m[7], w.y);
		}
	else
		{
		s = mSwingAxisWorld;
		}

	// The cone's frame: columns t1, t2 (the Foundation import at
	// [0x1010418c]) and s, 0xa6ec7-0xa6f6b.
	NxVec3 t1;
	NxVec3 t2;
	NxNormalToTangents(s, t1, t2);
	const double dot = (sphericalMul(s.z, a1.z) + sphericalMul(s.y, a1.y)) + sphericalMul(s.x, a1.x);
	// `fcomp [+0x1cc]; test ah,5; jp`: 1 only when the dot is below.
	const NxU32 outside = dot < mSwingLimitCos ? 1 : 0;
	renderable.addArrow(center, a1, 1.0f, scale, 0xff00d0);

	// The radius (0xa6f9d-0xa7016): tan(acos(cos limit)), or 1000 (0x447a0000)
	// when the cosine lies within 0.01 of zero or is unordered.
	NxReal radius;
	if(mSwingLimitCos > 0.01f || mSwingLimitCos < -0.01f)
		radius = (NxReal)sphericalFptan(jointAcos(mSwingLimitCos));
	else
		radius = 1000.0f;
	const NxReal sign = mSwingLimitCos > 0.0f ? 1.0f : -1.0f;
	const NxReal signSquared = (NxReal)sphericalMul(sign, sign);

	// The loop is phys_fn_004314 (0xa7050-0xa71eb): 24 spokes, i unsigned
	// (fild with the 2^32 correction), the step 0.27318197f (0x1011a020);
	// cos of the unrounded angle, sin of the stored float.
	NxVec3 previous;
	for(NxU32 i = 0; i <= 0x17; i++)
		{
		const double angle = (double)i * 0.27318197f;
		const NxReal angleF = (NxReal)angle;
		double x = sphericalFcos(angle) * radius;
		double y = sphericalFsin(angleF) * radius;
		double z;
		const NxReal length = (NxReal)x87FsqrtDot3(y, y, x, x, signSquared, 1.0);
		if(length != 0.0f)
			{
			const double inverse = 1.0f / (double)length;
			x = x * inverse;
			y = y * inverse;
			z = inverse * sign;
			}
		else
			{
			z = sign;
			}
		const NxReal wy = (NxReal)((s.y * z + t2.y * y) + t1.y * x);
		const NxReal wz = (NxReal)((s.z * z + t2.z * y) + t1.z * x);
		const double wx = (s.x * z + t2.x * y) + t1.x * x;
		const NxReal dy = (NxReal)sphericalMul(wy, scale);
		const NxReal dz = (NxReal)sphericalMul(wz, scale);
		NxVec3 point;
		point.x = (NxReal)(wx * scale + center.x);
		point.y = (NxReal)((double)center.y + dy);
		point.z = (NxReal)((double)center.z + dz);
		if(i > 0)
			{
			renderable.addLine(previous, point, 0xff0000);
			renderable.addLine(center, point, outside * 0xd000 + 0xff0000);
			}
		previous = point;
		}
	}
