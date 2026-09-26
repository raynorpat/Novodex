/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/RevoluteJoint.h"
#include "core/NpRevoluteJoint.h"
#include "core/JointSupport.h"
#include "Scene.h"
#include "core/JointAcos.h"
#include "X87Sqrt.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxMath.h"
#include "NxMat33.h"
#include "NxDebugRenderable.h"
#include "NxUtilities.h"

#include <float.h>
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
// Task 7 wrote the rows under 700 B; Task 8a the solver-slot rows 004360,
// 004362 and 004374; Task 8b the projection row 004356, the visualization
// row 004364 and getAngle (004372). RevoluteJoint is constructed by
// Scene::createJoint's revolute case (Task 10).

// The float the setters raise a body's wake counter to and compare it with:
// 0x3ecccccc (.rdata 0x101053d4 and the immediate stored), as in Joint.cpp.
static const NxReal gRevoluteWakeFloor = 0.39999998f;

static NX_INLINE double revoluteMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

// (a0 * b0 + a1 * b1) + a2 * b2, the listing's usual three-term sum with the
// products and the partial sum on the stack. The argument order is the
// listing's term order at each site.
static NX_INLINE double revoluteSum3(NxReal a0, NxReal b0, NxReal a1, NxReal b1, NxReal a2, NxReal b2)
	{
	return (revoluteMul(a0, b0) + revoluteMul(a1, b1)) + revoluteMul(a2, b2);
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

// The x87 _CIacos reproduction and the inlined NxMath::acos(NxF32) clamp
// moved to core/JointAcos.h (jointCIacos, jointAcos) by joint-families Task
// 3c, which reuses them for the spherical rows.

// The two SDK parameters the solver-slot rows read straight from the live
// parameter array at .data 0x10123b18 (PhysicsSDK.cpp's gParameter):
// element 0 (0x10123b18, NX_PENALTY_FORCE) and element 4 (0x10123b28,
// NX_BOUNCE_TRESHOLD). Read through PhysicsSDK::getParameter as
// ContactGeneration.cpp does. With no SDK instance this returns 0, which
// is what the oracle reads only before the first SDK exists: the static
// array keeps its values after an SDK is released, so the oracle would
// then read the last values. No joint exists without an SDK, so these
// rows never run in that window.
static NxReal revoluteSdkParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// The record header every constraint-record site writes after Joint::row004093
// (row 004093) hands out a record: the two body records, the row vector,
// and the kind in bits 0-4 ((flags & keep) | kind; stored).
static void revoluteRecordHeader(JointSupportRecord* record, JointSupportBody* body0, JointSupportBody* body1,
	const NxVec3& v, NxU32 keep, NxU32 kind)
	{
	record->mBody[0] = body0;
	record->mBody[1] = body1;
	record->mUnknown000 = v;
	record->mFlags = (record->mFlags & keep) | kind;
	}

// The bit update the limit, motor and angular records share (004360
// 0xab065-0xab09a, 004362 0xab29b-0xab2cf): bit 9 = (kind is 0 or 2), bit 10
// set, bits 5-8 and 11-18 cleared. The kind is known at every site, but the
// listing tests it (the supplement decompile drops those arms as
// unreachable), so the test is kept.
static void revoluteRecordBits(JointSupportRecord* record)
	{
	const NxU32 flags = record->mFlags;
	const NxU32 kind = flags & 0x1f;
	const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
	record->mFlags = (((bit9 & 1) | 2) << 9) | (flags & 0xfff8041f);
	}

// The tail every phys_fn_004391 site shares (004360 0xab0a8-0xab0f0, 004362
// 0xab2d5-0xab31e): fill +0x34..+0x4c, solve with 004391 into +0x40, copy
// that to +0x3c, then scale +0x40 by kind: 0 or 2 by SDK parameter 0, 1 or
// 3 by 0.7f (0x3f333333 at 0x10106940). The listing passes a dead stack
// slot as 004391's first output; `unused` stands for it.
static void revoluteSolveRecord(JointSupportRecord* record, Joint* joint, NxReal value034, NxReal value048)
	{
	record->mUnknown034 = value034;
	record->mUnknown038 = 0.0f;
	record->mUnknown044 = 0;
	record->mUnknown04c = 0;
	record->mUnknown048 = value048;
	record->mUnknown030 = joint;
	NxReal unused;
	record->row004391(unused, record->mUnknown040);
	record->mUnknown03c = record->mUnknown040;
	const NxU32 kind = record->mFlags & 0x1f;
	if(kind == 0 || kind == 2)
		record->mUnknown040 = (NxReal)((double)revoluteSdkParameter(NX_PENALTY_FORCE) * record->mUnknown040);
	else if(kind == 1 || kind == 3)
		record->mUnknown040 = (NxReal)((double)record->mUnknown040 * 0.7f);
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

// 004364's limit arc (0xac328-0xac32e) takes fcos and fsin of the same
// unrounded angle (`fld st(0); fcos; fxch st(1); fsin`); the CRT's cos/sin
// need not agree with the instructions, so they are used directly, as in
// revoluteFcos/revoluteFsin.
static void revoluteFcosFsin(double angle, double& cosine, double& sine)
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

// 004372's quaternion-to-rows conversion for body 0's normal (0xac77d-
// 0xac83a): the same instructions as core/Joint.cpp's jointQuatToRows
// (yy2, xz2 and yz2 stored, the rest on the stack). q is x, y, z, w; m is
// row-major.
static void revoluteQuatToRows(const NxReal* q, NxReal* m)
	{
	const double x = q[0], y = q[1], z = q[2], w = q[3];
	const double yy = y * y;
	const NxReal yy2 = (NxReal)(yy + yy);
	const double zz = z * z;
	const double zz2 = zz + zz;
	m[0] = (NxReal)((1.0 - yy2) - zz2);
	const double xy = y * x;
	const double xy2 = xy + xy;
	const double zw = z * w;
	const double zw2 = zw + zw;
	m[1] = (NxReal)(xy2 - zw2);
	const double xz = z * x;
	const NxReal xz2 = (NxReal)(xz + xz);
	const double yw = y * w;
	const double yw2 = yw + yw;
	const NxReal yw2f = (NxReal)yw2;
	m[2] = (NxReal)(yw2 + xz2);
	m[3] = (NxReal)(zw2 + xy2);
	const double xx = x * x;
	const double oneMinusXx2 = 1.0 - (xx + xx);
	const NxReal oneMinusXx2f = (NxReal)oneMinusXx2;
	m[4] = (NxReal)(oneMinusXx2 - zz2);
	const double yz = z * y;
	const NxReal yz2 = (NxReal)(yz + yz);
	const double xw = w * x;
	const double xw2 = xw + xw;
	m[5] = (NxReal)(yz2 - xw2);
	m[6] = (NxReal)((double)xz2 - yw2f);
	m[7] = (NxReal)(xw2 + yz2);
	m[8] = (NxReal)((double)oneMinusXx2f - yy2);
	}

// The same conversion where 004372 already holds three values on the FPU
// stack (body 0's cross vector, 0xaca27-0xacaed, and body 1's normal,
// 0xacd23-0xacdf6; the two runs are the same instructions): zz2, xy2 and
// zw2 are stored as well, so m[0], m[1], m[3] and m[4] use their floats.
static void revoluteQuatToRowsSpilled(const NxReal* q, NxReal* m)
	{
	const double x = q[0], y = q[1], z = q[2], w = q[3];
	const double yy = y * y;
	const NxReal yy2 = (NxReal)(yy + yy);
	const double zz = z * z;
	const NxReal zz2 = (NxReal)(zz + zz);
	m[0] = (NxReal)((1.0 - yy2) - zz2);
	const double xy = y * x;
	const NxReal xy2 = (NxReal)(xy + xy);
	const double zw = z * w;
	const NxReal zw2 = (NxReal)(zw + zw);
	m[1] = (NxReal)((double)xy2 - zw2);
	const double xz = z * x;
	const NxReal xz2 = (NxReal)(xz + xz);
	const double yw = y * w;
	const double yw2 = yw + yw;
	const NxReal yw2f = (NxReal)yw2;
	m[2] = (NxReal)(yw2 + xz2);
	m[3] = (NxReal)((double)zw2 + xy2);
	const double xx = x * x;
	const double oneMinusXx2 = 1.0 - (xx + xx);
	const NxReal oneMinusXx2f = (NxReal)oneMinusXx2;
	m[4] = (NxReal)(oneMinusXx2 - zz2);
	const double yz = z * y;
	const NxReal yz2 = (NxReal)(yz + yz);
	const double xw = w * x;
	const double xw2 = xw + xw;
	m[5] = (NxReal)(yz2 - xw2);
	m[6] = (NxReal)((double)xz2 - yw2f);
	m[7] = (NxReal)(xw2 + yz2);
	m[8] = (NxReal)((double)oneMinusXx2f - yy2);
	}

// phys_fn_004366 (0x000ac540, 162 B)
// Joint(desc, 0x40) runs first; the compiler then stores the vptr 0x1011a1c0
// and the member default constructors write +0x16c..+0x198 (0xac559-0xac5a4:
// limit 0, 0, 1, 0, 0, 1; motor NX_MAX_REAL, 0, 0; spring 0, 0, 0). The public
// object is allocated through the Foundation allocator (`push 0; push 0x1c; call
// [edx+8]`) and constructed only when the allocation succeeded, but
// desc.userData is written to it without a null check (0xac5cc-0xac5cf): a
// failed allocation faults there in the oracle, and does here too.
RevoluteJoint::RevoluteJoint(const NxRevoluteJointDesc& desc)
	: Joint(desc, 0x40)
	{
	void* memory = nxFoundationSDKAllocator->malloc(sizeof(NpRevoluteJoint), NX_MEMORY_PERSISTENT);
	NpRevoluteJoint* publicJoint = memory ? new(memory) NpRevoluteJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	row004332(desc);
	}

// phys_fn_004368 (0x000ac5f0, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x1011a1c0, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`), calls the Joint destructor body
// (row 004095) directly, and frees `this` through the Foundation allocator (slot
// +0x14) when the flag's bit 0 is set (Joint::operator delete).
RevoluteJoint::~RevoluteJoint()
	{
	if(mPublicObject)
		delete static_cast<NpRevoluteJoint*>(mPublicObject);
	}

// 004374's per-body update (0xad310-0xad3a9 for body 0, 0xad437-0xad4d2 for
// body 1; the two are the same instructions): unless the record's +0x0c is
// zero, +0x00 += +0x0c * t and +0x10 += (+0x20 3x3) * c. The first product
// stays on the stack, the other two are stored before the sums; the first
// two rows of the 3x3 product stay on the stack, the third is stored. The
// callers differ only in where t.z comes from (a register for body 0, the
// stored float for body 1), hence the double.
static void revoluteApplyToRecord(JointSupportBody* record, NxReal tx, NxReal ty, double tz, const NxVec3& c)
	{
	if(record->mUnknown00c == 0.0f)
		return;
	const NxReal m = record->mUnknown00c;
	const double lx = revoluteMul(tx, m);
	const NxReal ly = (NxReal)revoluteMul(ty, m);
	const NxReal lz = (NxReal)(tz * m);
	record->mUnknown000.x = (NxReal)(lx + record->mUnknown000.x);
	record->mUnknown000.y = (NxReal)((double)ly + record->mUnknown000.y);
	record->mUnknown000.z = (NxReal)((double)lz + record->mUnknown000.z);
	const NxReal* I = record->mUnknown020;
	const double u0 = (revoluteMul(c.z, I[2]) + revoluteMul(c.y, I[1])) + revoluteMul(c.x, I[0]);
	const double u1 = (revoluteMul(c.z, I[5]) + revoluteMul(c.y, I[4])) + revoluteMul(c.x, I[3]);
	const NxReal u2 = (NxReal)((revoluteMul(c.z, I[8]) + revoluteMul(c.y, I[7])) + revoluteMul(c.x, I[6]));
	record->mUnknown010.x = (NxReal)(u0 + record->mUnknown010.x);
	record->mUnknown010.y = (NxReal)(u1 + record->mUnknown010.y);
	record->mUnknown010.z = (NxReal)((double)u2 + record->mUnknown010.z);
	}

// phys_fn_004374 (0x000ad0b0, 1068 B)
// `ret 4`, but the listing never reads the argument (the manifest decompile
// shows no stack argument at all). v is the velocity error phys_fn_004358
// returns. When maxForce is finite, a = v + mUnknown1f4 is compared with
// mUnknown200: above it the joint breaks (+0x2c bits 3/4 = broken, a break
// event carrying |a| is posted to the Scene through row 000571), a is
// scaled back to length sqrt(mUnknown200), v becomes the scaled a minus the
// old mUnknown1f4, and mUnknown1f4 takes the scaled a; otherwise
// mUnknown1f4 = a. Then t = mUnknown1b8 * -(v + mUnknown1ac) is applied to
// body 0's record with lever mUnknown1dc and -t to body 1's with lever
// mUnknown1e8 (revoluteApplyToRecord).
// Listing over decompile: the decompile shows every intermediate as a
// float; the listing keeps a.z's sum (squared against its stored float),
// the ratio's first product, t.z and several partial sums on the stack
// (0xad109-0xad10d, 0xad185-0xad189, 0xad288-0xad2a2), and -t.z is the
// register value in body 1's first cross component (0xad3c0-0xad3c6) but
// the stored float in its impulse (0xad44a). The event's +4 is not written
// here (000571 writes it).
// Driving this row requires body +0x204 (row004358 and the two record
// updates dereference it), which the candidate's body record never writes.
void RevoluteJoint::row_slot0(NxU32 arg)
	{
	(void)arg;
	if((mFlags & 0x18) == 0x10)
		return;

	NxVec3 v;
	row004358(v);

	if(mMaxForce < NX_MAX_REAL)
		{
		NxVec3 a;
		a.x = (NxReal)((double)v.x + mUnknown1f4.x);
		a.y = (NxReal)((double)v.y + mUnknown1f4.y);
		const double az = (double)v.z + mUnknown1f4.z;
		a.z = (NxReal)az;
		const double lengthSquared = ((az * a.z) + revoluteMul(a.y, a.y)) + revoluteMul(a.x, a.x);
		if(lengthSquared > mUnknown200)
			{
			// fsqrt of the compared sum (0xad136), re-formed from the same
			// operands in the same order (X87Sqrt.h).
			const NxReal length = (NxReal)x87FsqrtDot3(az, a.z, a.y, a.y, a.x, a.x);
			mFlags = (mFlags & ~8u) | 0x10;
			void* memory = nxFoundationSDKAllocator->malloc(sizeof(JointBreakEvent), NX_MEMORY_PERSISTENT);
			JointBreakEvent* event = memory ? new(memory) JointBreakEvent(this, length) : 0;
			// Scene row 000571 (Physics/src/Scene.cpp).
			static_cast<NxSceneInternal*>(mScene)->addJointBreakEvent(event);
			const double ratio = x87Fsqrt(mUnknown200) / length;
			const NxReal ratioF = (NxReal)ratio;
			const double sx = ratio * a.x;
			const double sy = revoluteMul(a.y, ratioF);
			const NxReal sz = (NxReal)revoluteMul(a.z, ratioF);
			const double dx = sx - mUnknown1f4.x;
			const double dy = sy - mUnknown1f4.y;
			v.z = (NxReal)((double)sz - mUnknown1f4.z);
			v.x = (NxReal)dx;
			v.y = (NxReal)dy;
			mUnknown1f4.x = (NxReal)sx;
			mUnknown1f4.y = (NxReal)sy;
			mUnknown1f4.z = sz;
			}
		else
			{
			mUnknown1f4.x = a.x;
			mUnknown1f4.y = a.y;
			mUnknown1f4.z = a.z;
			}
		}

	const double px = (double)v.x + mUnknown1ac.x;
	const double py = (double)v.y + mUnknown1ac.y;
	const NxReal pz = (NxReal)((double)v.z + mUnknown1ac.z);
	const NxReal nx = (NxReal)(px * -1.0f);
	const double ny = py * -1.0f;
	const double nz = (double)pz * -1.0f;

	const NxReal* M = mUnknown1b8;
	const NxReal tx = (NxReal)((nz * M[2] + ny * M[1]) + (double)nx * M[0]);
	const NxReal ty = (NxReal)((nz * M[5] + ny * M[4]) + (double)nx * M[3]);
	const double tz = (nz * M[8] + ny * M[7]) + (double)nx * M[6];

	NxVec3 c;
	const NxVec3& r0 = mUnknown1dc;
	c.x = (NxReal)(tz * r0.y - revoluteMul(ty, r0.z));
	c.y = (NxReal)(revoluteMul(tx, r0.z) - tz * r0.x);
	c.z = (NxReal)(revoluteMul(ty, r0.x) - revoluteMul(tx, r0.y));
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	if(body0)
		revoluteApplyToRecord(body0->mUnknown204, tx, ty, tz, c);

	const NxReal sx = -tx;
	const NxReal sy = -ty;
	const double szRegister = -tz;
	const NxReal sz = (NxReal)szRegister;
	const NxVec3& r1 = mUnknown1e8;
	c.x = (NxReal)(szRegister * r1.y - revoluteMul(sy, r1.z));
	c.y = (NxReal)(revoluteMul(sx, r1.z) - revoluteMul(sz, r1.x));
	c.z = (NxReal)(revoluteMul(sy, r1.x) - revoluteMul(sx, r1.y));
	const JointBodyRecord* body1 = revoluteBody(mBody[1]);
	if(body1)
		revoluteApplyToRecord(body1->mUnknown204, sx, sy, sz, c);
	}

// phys_fn_004328 (0x000a8d20, 21 B)
void RevoluteJoint::row_slot1()
	{
	mUnknown1ac.z = 0.0f;
	mUnknown1ac.y = 0.0f;
	mUnknown1ac.x = 0.0f;
	}

// The step 004364's limit arc advances by: 1/12 as a float (0x3daaaaab,
// .rdata 0x101068e0).
static const NxReal gRevoluteLimitStep = 0.083333336f;

// phys_fn_004364 (0x000ab840, 3326 B)
// Debug visualization, only when +0x2c bit 9 (NX_JF_VISUALIZATION) is set.
// Each part is gated by its SDK parameter being non-zero (the oracle reads
// the live array at 0x10123b18; revoluteSdkParameter here) and scaled by it
// times NX_VISUALIZATION_SCALE (element 13, 0x10123b4c):
// - world axes (element 32): an arrow along row004127's axis at the point
//   row004123 returns, colour 0xffffff;
// - local axes (element 31): per body, the world anchor, axis, normal and
//   cross vectors carried through the body's +0x134/+0x158 pose (as
//   stored without a body); three arrows from each anchor (0x902020,
//   0x209020, 0x202090 for body 0; 0xe05050, 0x50e050, 0x5050e0 for body
//   1), a line between the anchors and one between the axis tips
//   (0xffff00);
// - limits (element 33, only when limit.low < limit.high): 13 arc points
//   from low to high, row004123's point plus scale * (cos * n + sin * c +
//   0 * a) with n, c, a body 0's normal, cross and axis; the two end
//   spokes are 0xff0000, or 0xffd000 when row004352's angle is past that
//   limit, the arc 0xff0000; then an arrow along body 1's normal
//   (0xff00d0).
// The limit arm calls row004127 but never reads its result (0xac04c; the
// stale-body refresh inside still runs).
// Listing over decompile (supplement, no prototype): the decompile loses
// the renderable calls' arguments and colours (the listing pushes them at
// 0xab8c9, 0xabe9d-0xabf50, 0xac404, 0xac41f and 0xac517), shows every
// rotated vector as float with its terms reordered, and misreads the end
// spokes' colour (0xac3ee-0xac404: flag[i / 12] * 0xd000 + 0xff0000). The
// listing keeps the values named double below on the stack.
void RevoluteJoint::row_slot4(NxDebugRenderable& renderable)
	{
	if(!((mFlags >> 9) & 1))
		return;

	revoluteRefreshFirstStaleBody(*this);

	if(revoluteSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES) != 0.0f)
		{
		NxVec3 anchor;
		row004123(anchor);
		NxVec3 axis;
		row004127(axis);
		const NxReal scale = (NxReal)((double)revoluteSdkParameter(NX_VISUALIZATION_SCALE) *
			revoluteSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES));
		renderable.addArrow(anchor, axis, 1.0f, scale, 0xffffff);
		}

	if(revoluteSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) != 0.0f)
		{
		NxVec3 p0, a0, n0, c0;
		const JointBodyRecord* body0 = revoluteBody(mBody[0]);
		if(!body0)
			{
			p0 = mWorldAnchor[0];
			a0 = mWorldAxis[0];
			n0 = mWorldNormal[0];
			c0 = mWorldCross[0];
			}
		else
			{
			// 0xab987-0xabbc5.
			const NxReal* m = body0->mUnknown134;
			const NxVec3& t = body0->mUnknown158;
			const NxVec3& p = mWorldAnchor[0];
			const double px = revoluteSum3(m[1], p.y, m[2], p.z, m[0], p.x);
			const NxReal py = (NxReal)revoluteSum3(m[4], p.y, m[3], p.x, m[5], p.z);
			const NxReal pz = (NxReal)revoluteSum3(m[7], p.y, m[6], p.x, m[8], p.z);
			p0.x = (NxReal)(px + t.x);
			p0.y = (NxReal)((double)py + t.y);
			p0.z = (NxReal)((double)pz + t.z);
			const NxVec3& a = mWorldAxis[0];
			a0.x = (NxReal)revoluteSum3(m[2], a.z, m[1], a.y, a.x, m[0]);
			a0.y = (NxReal)revoluteSum3(m[5], a.z, m[4], a.y, m[3], a.x);
			a0.z = (NxReal)revoluteSum3(m[8], a.z, m[7], a.y, m[6], a.x);
			const NxVec3& n = mWorldNormal[0];
			n0.x = (NxReal)revoluteSum3(m[1], n.y, m[2], n.z, m[0], n.x);
			n0.y = (NxReal)revoluteSum3(m[4], n.y, m[5], n.z, m[3], n.x);
			n0.z = (NxReal)revoluteSum3(m[7], n.y, m[8], n.z, m[6], n.x);
			const NxVec3& c = mWorldCross[0];
			c0.x = (NxReal)revoluteSum3(m[2], c.z, m[1], c.y, c.x, m[0]);
			c0.y = (NxReal)revoluteSum3(m[5], c.z, m[3], c.x, m[4], c.y);
			c0.z = (NxReal)revoluteSum3(m[8], c.z, m[6], c.x, m[7], c.y);
			}

		NxVec3 p1, a1, n1, c1;
		const JointBodyRecord* body1 = revoluteBody(mBody[1]);
		if(!body1)
			{
			p1 = mWorldAnchor[1];
			a1 = mWorldAxis[1];
			n1 = mWorldNormal[1];
			c1 = mWorldCross[1];
			}
		else
			{
			// 0xabc4d-0xabe8b.
			const NxReal* m = body1->mUnknown134;
			const NxVec3& t = body1->mUnknown158;
			const NxVec3& p = mWorldAnchor[1];
			const double px = revoluteSum3(m[2], p.z, m[1], p.y, m[0], p.x);
			const NxReal py = (NxReal)revoluteSum3(m[5], p.z, m[4], p.y, m[3], p.x);
			const NxReal pz = (NxReal)revoluteSum3(m[8], p.z, m[7], p.y, m[6], p.x);
			p1.x = (NxReal)(px + t.x);
			p1.y = (NxReal)((double)py + t.y);
			p1.z = (NxReal)((double)pz + t.z);
			const NxVec3& a = mWorldAxis[1];
			a1.x = (NxReal)revoluteSum3(m[1], a.y, m[2], a.z, m[0], a.x);
			a1.y = (NxReal)revoluteSum3(m[4], a.y, m[3], a.x, m[5], a.z);
			a1.z = (NxReal)revoluteSum3(m[7], a.y, m[6], a.x, m[8], a.z);
			const NxVec3& n = mWorldNormal[1];
			n1.x = (NxReal)revoluteSum3(m[2], n.z, m[1], n.y, n.x, m[0]);
			n1.y = (NxReal)revoluteSum3(m[5], n.z, m[4], n.y, m[3], n.x);
			n1.z = (NxReal)revoluteSum3(m[8], n.z, m[7], n.y, m[6], n.x);
			const NxVec3& c = mWorldCross[1];
			c1.x = (NxReal)revoluteSum3(m[1], c.y, m[2], c.z, m[0], c.x);
			c1.y = (NxReal)revoluteSum3(m[4], c.y, m[3], c.x, m[5], c.z);
			c1.z = (NxReal)revoluteSum3(m[7], c.y, m[6], c.x, m[8], c.z);
			}

		const NxReal scale = (NxReal)((double)revoluteSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) *
			revoluteSdkParameter(NX_VISUALIZATION_SCALE));
		renderable.addArrow(p0, n0, 1.0f, scale, 0x902020);
		renderable.addArrow(p0, c0, 1.0f, scale, 0x209020);
		renderable.addArrow(p0, a0, 1.0f, scale, 0x202090);
		renderable.addArrow(p1, n1, 1.0f, scale, 0xe05050);
		renderable.addArrow(p1, c1, 1.0f, scale, 0x50e050);
		renderable.addArrow(p1, a1, 1.0f, scale, 0x5050e0);
		renderable.addLine(p0, p1, 0xffff00);

		// The axis tips (0xabf64-0xabff9): the z products are stored before
		// their sums, the x and y products are not.
		NxVec3 tip1;
		const NxReal tip1Z = (NxReal)revoluteMul(a1.z, scale);
		tip1.x = (NxReal)(revoluteMul(a1.x, scale) + p1.x);
		tip1.y = (NxReal)(revoluteMul(a1.y, scale) + p1.y);
		tip1.z = (NxReal)((double)tip1Z + p1.z);
		NxVec3 tip0;
		const NxReal tip0Z = (NxReal)revoluteMul(a0.z, scale);
		tip0.x = (NxReal)(revoluteMul(a0.x, scale) + p0.x);
		tip0.y = (NxReal)(revoluteMul(a0.y, scale) + p0.y);
		tip0.z = (NxReal)((double)tip0Z + p0.z);
		renderable.addLine(tip0, tip1, 0xffff00);
		}

	if(mLimit.low.value < mLimit.high.value && revoluteSdkParameter(NX_VISUALIZE_JOINT_LIMITS) != 0.0f)
		{
		const NxReal scale = (NxReal)((double)revoluteSdkParameter(NX_VISUALIZE_JOINT_LIMITS) *
			revoluteSdkParameter(NX_VISUALIZATION_SCALE));
		NxVec3 anchor;
		row004123(anchor);
		NxVec3 unusedAxis;
		row004127(unusedAxis);
		const double angle = row004352();
		// 1 when the angle is past that limit (0xac05f-0xac091).
		NxU32 beyond[2];
		beyond[0] = angle < mLimit.low.value ? 1 : 0;
		beyond[1] = angle > mLimit.high.value ? 1 : 0;

		// Body 0's normal, cross and axis (0xac0a4-0xac25d), all stored.
		NxVec3 n, c, a;
		const JointBodyRecord* body0 = revoluteBody(mBody[0]);
		if(body0)
			{
			const NxReal* m = body0->mUnknown134;
			const NxVec3& wn = mWorldNormal[0];
			n.x = (NxReal)revoluteSum3(m[2], wn.z, m[1], wn.y, wn.x, m[0]);
			n.y = (NxReal)revoluteSum3(m[5], wn.z, m[4], wn.y, m[3], wn.x);
			n.z = (NxReal)revoluteSum3(m[8], wn.z, m[7], wn.y, m[6], wn.x);
			const NxVec3& wc = mWorldCross[0];
			c.x = (NxReal)revoluteSum3(m[1], wc.y, m[2], wc.z, wc.x, m[0]);
			c.y = (NxReal)revoluteSum3(m[4], wc.y, m[3], wc.x, m[5], wc.z);
			c.z = (NxReal)revoluteSum3(m[7], wc.y, m[6], wc.x, m[8], wc.z);
			const NxVec3& wa = mWorldAxis[0];
			a.x = (NxReal)revoluteSum3(m[2], wa.z, m[1], wa.y, wa.x, m[0]);
			a.y = (NxReal)revoluteSum3(m[5], wa.z, m[4], wa.y, m[3], wa.x);
			a.z = (NxReal)revoluteSum3(m[8], wa.z, m[7], wa.y, m[6], wa.x);
			}
		else
			{
			n = mWorldNormal[0];
			c = mWorldCross[0];
			a = mWorldAxis[0];
			}
		// The axis term is multiplied by the 0.0f at 0x101041f0, not dropped
		// (0xac2b7-0xac2e7).
		const NxReal axisY = (NxReal)((double)a.y * 0.0f);
		const NxReal axisZ = (NxReal)((double)a.z * 0.0f);
		const NxReal axisX = (NxReal)((double)a.x * 0.0f);

		// i is unsigned (fild with the 2^32 correction, 0xac2f9-0xac302).
		NxVec3 previous;
		for(NxU32 i = 0; i <= 12; i++)
			{
			const double t = (double)i * gRevoluteLimitStep;
			const double arcAngle = (1.0f - t) * mLimit.low.value + t * mLimit.high.value;
			double cosine;
			double sine;
			revoluteFcosFsin(arcAngle, cosine, sine);
			const NxReal ry = (NxReal)((c.y * sine + n.y * cosine) + axisY);
			const NxReal rz = (NxReal)((c.z * sine + n.z * cosine) + axisZ);
			const double rx = (n.x * cosine + c.x * sine) + axisX;
			const NxReal sy = (NxReal)revoluteMul(ry, scale);
			const NxReal sz = (NxReal)revoluteMul(rz, scale);
			NxVec3 point;
			point.x = (NxReal)(rx * scale + anchor.x);
			point.y = (NxReal)((double)anchor.y + sy);
			point.z = (NxReal)((double)anchor.z + sz);
			if(i == 0 || i == 12)
				renderable.addLine(anchor, point, beyond[i / 12] * 0xd000 + 0xff0000);
			if(i > 0)
				renderable.addLine(previous, point, 0xff0000);
			previous = point;
			}

		// Body 1's normal (0xac48b-0xac50d).
		NxVec3 n1;
		const JointBodyRecord* body1 = revoluteBody(mBody[1]);
		if(!body1)
			{
			n1 = mWorldNormal[1];
			}
		else
			{
			const NxReal* m = body1->mUnknown134;
			const NxVec3& wn = mWorldNormal[1];
			n1.x = (NxReal)revoluteSum3(m[2], wn.z, m[1], wn.y, m[0], wn.x);
			n1.y = (NxReal)revoluteSum3(m[5], wn.z, m[3], wn.x, m[4], wn.y);
			n1.z = (NxReal)revoluteSum3(m[8], wn.z, m[6], wn.x, m[7], wn.y);
			}
		renderable.addArrow(anchor, n1, 1.0f, scale, 0xff00d0);
		}
	}

// 004360's column tail for body 0 (0xaa408-0xaa468 and its two repeats):
// k = mass + w x r, with (w x r).y kept on the stack and w.z arriving as a
// register; k.x stays on the stack, k.y and k.z are stored.
static void revoluteColumnBody0(const NxVec3& r, NxReal wx, NxReal wy, double wz,
	NxReal massX, NxReal massY, NxReal massZ, double& kx, NxReal& ky, NxReal& kz)
	{
	const NxReal vx = (NxReal)(revoluteMul(r.z, wy) - r.y * wz);
	const double vy = wz * r.x - revoluteMul(r.z, wx);
	const NxReal vz = (NxReal)(revoluteMul(r.y, wx) - revoluteMul(r.x, wy));
	kx = (double)vx + massX;
	ky = (NxReal)(vy + massY);
	kz = (NxReal)((double)vz + massZ);
	}

// The same for body 1 (0xaa7a9-0xaa7fb and repeats): here w is stored and
// (w x r).x stays on the stack; all three results are stored.
static void revoluteColumnBody1(const NxVec3& r, NxReal wx, NxReal wy, NxReal wz,
	NxReal massX, NxReal massY, NxReal massZ, NxReal& kx, NxReal& ky, NxReal& kz)
	{
	const double vx = revoluteMul(r.z, wy) - revoluteMul(r.y, wz);
	const NxReal vy = (NxReal)(revoluteMul(r.x, wz) - revoluteMul(r.z, wx));
	const NxReal vz = (NxReal)(revoluteMul(r.y, wx) - revoluteMul(r.x, wy));
	kx = (NxReal)(vx + massX);
	ky = (NxReal)((double)vy + massY);
	kz = (NxReal)((double)vz + massZ);
	}

// phys_fn_004360 (0x000aa060, 4460 B)
// After the stale-body refresh:
// 1. r_i = body i's +0x134 3x3 times mWorldAnchor[i] (or the anchor as is),
//    stored in mUnknown1dc/mUnknown1e8; mUnknown1ac = (r0 + body0 +0x158) -
//    (r1 + body1 +0x158), then scaled by (1 / arg) * SDK parameter 0.
// 2. K = sum over bodies of (m e_j + (J (r x e_j)) x r) for the unit
//    vectors e_j, m = body +0xc0 and J = body +0x164 (3x3). The unit
//    vectors are folded into the code, but their zero components are not:
//    every r * 0 and m * 0 is computed (fmul by the 0.0f at 0x101041f0).
//    Body 1's columns are stored in mUnknown1b8 first and body 0's added.
// 3. mUnknown1b8 = K^-1 by cofactors, or the identity when the float
//    determinant is 0; when it is not, one zeroed record of kind 6 is
//    taken from Joint::row004093.
// 4. With a finite maxForce, mUnknown200 = maxForce^2 (FLT_EPSILON,
//    0x34000000, when the unrounded square is 0) and mUnknown1f4 = 0.
// 5. Two kind-3 records with +0x48 = maxTorque, along n0 = body 0's
//    rotated mWorldNormal[0] and c0 = its rotated mWorldCross[0], each with
//    +0x34 = -(dot(a0 x a1, n) / arg) for the rotated world axes a0, a1.
// Listing over decompile (supplement): the supplement drops the kind tests
// as unreachable (0x100ab07d, 0x100ab142) and shows the intermediates as
// floats; the listing keeps the tests and leaves, per column, the values
// named double below on the stack.
void RevoluteJoint::row_slot6(NxReal arg)
	{
	revoluteRefreshFirstStaleBody(*this);

	NxVec3 r0;
	JointSupportBody* record0;
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	if(body0)
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& a = mWorldAnchor[0];
		record0 = body0->mUnknown204;
		r0.x = (NxReal)((revoluteMul(m[2], a.z) + revoluteMul(m[1], a.y)) + revoluteMul(m[0], a.x));
		r0.y = (NxReal)((revoluteMul(m[5], a.z) + revoluteMul(m[4], a.y)) + revoluteMul(m[3], a.x));
		r0.z = (NxReal)((revoluteMul(m[8], a.z) + revoluteMul(m[7], a.y)) + revoluteMul(m[6], a.x));
		}
	else
		{
		r0 = mWorldAnchor[0];
		record0 = 0;
		}

	NxVec3 r1;
	JointSupportBody* record1;
	const JointBodyRecord* body1 = revoluteBody(mBody[1]);
	if(body1)
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& a = mWorldAnchor[1];
		record1 = body1->mUnknown204;
		r1.x = (NxReal)((revoluteMul(m[2], a.z) + revoluteMul(m[1], a.y)) + revoluteMul(m[0], a.x));
		r1.y = (NxReal)((revoluteMul(m[5], a.z) + revoluteMul(m[4], a.y)) + revoluteMul(m[3], a.x));
		r1.z = (NxReal)((revoluteMul(m[8], a.z) + revoluteMul(m[7], a.y)) + revoluteMul(m[6], a.x));
		}
	else
		{
		r1 = mWorldAnchor[1];
		record1 = 0;
		}

	// The x difference stays on the stack unless body 0 is there, in which
	// case the stored float is reloaded (0xaa24f-0xaa255).
	const double dx = (double)r0.x - r1.x;
	const NxReal dxF = (NxReal)dx;
	NxReal cy = (NxReal)((double)r0.y - r1.y);
	NxReal cz = (NxReal)((double)r0.z - r1.z);
	double cx = dx;
	if(body0)
		{
		cx = (double)dxF + body0->mUnknown158.x;
		cy = (NxReal)((double)cy + body0->mUnknown158.y);
		cz = (NxReal)((double)cz + body0->mUnknown158.z);
		}
	if(body1)
		{
		cx = cx - body1->mUnknown158.x;
		cy = (NxReal)((double)cy - body1->mUnknown158.y);
		cz = (NxReal)((double)cz - body1->mUnknown158.z);
		}

	const NxReal inverse = (NxReal)(1.0f / (double)arg);
	mUnknown1dc = r0;
	mUnknown1e8 = r1;
	mUnknown1ac.y = cy;
	mUnknown1ac.z = cz;
	mUnknown1ac.x = (NxReal)cx;
	const double gain = (double)inverse * revoluteSdkParameter(NX_PENALTY_FORCE);
	mUnknown1ac.x = (NxReal)(gain * mUnknown1ac.x);
	mUnknown1ac.y = (NxReal)(gain * mUnknown1ac.y);
	mUnknown1ac.z = (NxReal)(gain * mUnknown1ac.z);

	// K, row-major; k00..k02, k12 and k22 are held on the stack, the rest
	// are stored (the 3x3 local at esp+0x6c).
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
		double wz;

		// e = (1, 0, 0)
		double ux = (double)ryZero - rzZero;
		double uy = (double)r0.z - rxZero;
		double uz = (double)rxZero - r0.y;
		wx = (NxReal)((uz * J[2] + uy * J[1]) + ux * J[0]);
		wy = (NxReal)((ux * J[3] + uz * J[5]) + uy * J[4]);
		wz = (ux * J[6] + uz * J[8]) + uy * J[7];
		mZero = (NxReal)((double)m * 0.0f);
		revoluteColumnBody0(r0, wx, wy, wz, m, mZero, mZero, k00, k10, k20);

		// e = (0, 1, 0)
		ux = (double)ryZero - r0.z;
		uy = (double)rzZero - rxZero;
		uz = (double)r0.x - ryZero;
		wx = (NxReal)((uz * J[2] + uy * J[1]) + ux * J[0]);
		wy = (NxReal)((uz * J[5] + uy * J[4]) + ux * J[3]);
		wz = (uz * J[8] + uy * J[7]) + ux * J[6];
		mZero = (NxReal)(0.0f * (double)m);
		revoluteColumnBody0(r0, wx, wy, wz, mZero, m, mZero, k01, k11, k21);

		// e = (0, 0, 1); k12 and k22 are stored and reloaded (0xaa670).
		ux = (double)r0.y - rzZero;
		uy = (double)rzZero - r0.x;
		uz = (double)rxZero - ryZero;
		wx = (NxReal)((uz * J[2] + uy * J[1]) + ux * J[0]);
		wy = (NxReal)((uz * J[5] + uy * J[4]) + ux * J[3]);
		wz = (uz * J[8] + uy * J[7]) + ux * J[6];
		mZero = (NxReal)(0.0f * (double)m);
		revoluteColumnBody0(r0, wx, wy, wz, mZero, mZero, m, k02, k1j, k2j);
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
		NxReal uy, uzF, wx, wy, wz, mZero;
		double ux, uz;
		NxReal* K = mUnknown1b8;

		// e = (1, 0, 0)
		ux = (double)ryZero - rzZero;
		uy = (NxReal)((double)r1.z - rxZero);
		uz = (double)rxZero - r1.y;
		uzF = (NxReal)uz;
		wx = (NxReal)((uz * J[2] + (double)uy * J[1]) + ux * J[0]);
		wy = (NxReal)((revoluteMul(uzF, J[5]) + revoluteMul(uy, J[4])) + ux * J[3]);
		wz = (NxReal)((revoluteMul(uzF, J[8]) + revoluteMul(uy, J[7])) + ux * J[6]);
		mZero = (NxReal)((double)m * 0.0f);
		revoluteColumnBody1(r1, wx, wy, wz, m, mZero, mZero, K[0], K[3], K[6]);

		// e = (0, 1, 0)
		ux = (double)ryZero - r1.z;
		uy = (NxReal)((double)rzZero - rxZero);
		uz = (double)r1.x - ryZero;
		uzF = (NxReal)uz;
		wx = (NxReal)((uz * J[2] + (double)uy * J[1]) + ux * J[0]);
		wy = (NxReal)((ux * J[3] + revoluteMul(uzF, J[5])) + revoluteMul(uy, J[4]));
		wz = (NxReal)((ux * J[6] + revoluteMul(uzF, J[8])) + revoluteMul(uy, J[7]));
		mZero = (NxReal)(0.0f * (double)m);
		revoluteColumnBody1(r1, wx, wy, wz, mZero, m, mZero, K[1], K[4], K[7]);

		// e = (0, 0, 1)
		ux = (double)r1.y - rzZero;
		uy = (NxReal)((double)rzZero - r1.x);
		uz = (double)rxZero - ryZero;
		uzF = (NxReal)uz;
		wx = (NxReal)((uz * J[2] + (double)uy * J[1]) + ux * J[0]);
		wy = (NxReal)((revoluteMul(uzF, J[5]) + revoluteMul(uy, J[4])) + ux * J[3]);
		wz = (NxReal)((revoluteMul(uzF, J[8]) + revoluteMul(uy, J[7])) + ux * J[6]);
		mZero = (NxReal)(0.0f * (double)m);
		revoluteColumnBody1(r1, wx, wy, wz, mZero, mZero, m, K[2], K[5], K[8]);

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

	const double c00 = (double)k11 * k22 - k12 * k21;
	const NxReal c01 = (NxReal)(k02 * k21 - k22 * k01);
	const double c02 = k12 * k01 - k02 * k11;
	const NxReal c02F = (NxReal)c02;
	const NxReal determinant = (NxReal)(((c02 * k20) + revoluteMul(c01, k10)) + c00 * k00);
	if(determinant == 0.0f)
		{
		mUnknown1b8[0] = 1.0f;
		mUnknown1b8[1] = 0.0f;
		mUnknown1b8[2] = 0.0f;
		mUnknown1b8[3] = 0.0f;
		mUnknown1b8[4] = 1.0f;
		mUnknown1b8[5] = 0.0f;
		mUnknown1b8[6] = 0.0f;
		mUnknown1b8[7] = 0.0f;
		mUnknown1b8[8] = 1.0f;
		}
	else
		{
		const double inverseDeterminant = 1.0f / (double)determinant;
		const NxReal d = (NxReal)inverseDeterminant;
		mUnknown1b8[0] = (NxReal)(inverseDeterminant * c00);
		mUnknown1b8[1] = (NxReal)revoluteMul(d, c01);
		mUnknown1b8[2] = (NxReal)revoluteMul(d, c02F);
		mUnknown1b8[3] = (NxReal)((k12 * k20 - k22 * k10) * d);
		mUnknown1b8[4] = (NxReal)((k00 * k22 - k02 * k20) * d);
		mUnknown1b8[5] = (NxReal)((k02 * k10 - k00 * k12) * d);
		mUnknown1b8[6] = (NxReal)((revoluteMul(k21, k10) - revoluteMul(k11, k20)) * d);
		mUnknown1b8[7] = (NxReal)((k20 * k01 - k00 * k21) * d);
		mUnknown1b8[8] = (NxReal)((k00 * k11 - k01 * k10) * d);

		// A zeroed record of kind 6 (0xaac53-0xaacf3). Bits 9 and 10 are
		// stored as bit-fields from the kind tests, then bits 6-8 and
		// 11-18 are cleared.
		JointSupportRecord* record = row004093();
		NxU32 flags = (record->mFlags & 0xffffffe6) | 0x26;
		record->mUnknown030 = this;
		record->mFlags = flags;
		NxU32 kind = flags & 0x1f;
		const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
		flags = (flags & ~0x200u) | (bit9 << 9);
		record->mFlags = flags;
		kind = flags & 0x1f;
		const NxU32 bit10 = (kind == 3 || kind == 2 || kind == 5) ? 1 : 0;
		flags = (flags & ~0x400u) | (bit10 << 10);
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
		}

	if(mMaxForce < NX_MAX_REAL)
		{
		const double squared = revoluteMul(mMaxForce, mMaxForce);
		mUnknown200 = (NxReal)squared;
		if(squared == 0.0f)
			mUnknown200 = FLT_EPSILON;
		mUnknown1f4.z = 0.0f;
		mUnknown1f4.y = 0.0f;
		mUnknown1f4.x = 0.0f;
		}

	// Body 0 is reloaded here (0xaad40).
	NxVec3 n0;
	NxVec3 c0;
	double a0x, a0y;
	NxReal a0z;
	body0 = revoluteBody(mBody[0]);
	if(body0)
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& n = mWorldNormal[0];
		n0.x = (NxReal)((revoluteMul(m[1], n.y) + revoluteMul(m[2], n.z)) + revoluteMul(m[0], n.x));
		n0.y = (NxReal)((revoluteMul(m[4], n.y) + revoluteMul(m[5], n.z)) + revoluteMul(m[3], n.x));
		n0.z = (NxReal)((revoluteMul(m[7], n.y) + revoluteMul(m[8], n.z)) + revoluteMul(m[6], n.x));
		const NxVec3& c = mWorldCross[0];
		c0.x = (NxReal)((revoluteMul(m[1], c.y) + revoluteMul(m[2], c.z)) + revoluteMul(c.x, m[0]));
		c0.y = (NxReal)((revoluteMul(m[4], c.y) + revoluteMul(m[3], c.x)) + revoluteMul(m[5], c.z));
		c0.z = (NxReal)((revoluteMul(m[7], c.y) + revoluteMul(m[6], c.x)) + revoluteMul(m[8], c.z));
		const NxVec3& a = mWorldAxis[0];
		a0x = (revoluteMul(m[1], a.y) + revoluteMul(m[2], a.z)) + revoluteMul(m[0], a.x);
		a0y = (revoluteMul(m[4], a.y) + revoluteMul(m[5], a.z)) + revoluteMul(m[3], a.x);
		a0z = (NxReal)((revoluteMul(m[7], a.y) + revoluteMul(m[8], a.z)) + revoluteMul(m[6], a.x));
		}
	else
		{
		n0 = mWorldNormal[0];
		c0 = mWorldCross[0];
		a0x = mWorldAxis[0].x;
		a0y = mWorldAxis[0].y;
		a0z = mWorldAxis[0].z;
		}

	double a1x, a1y, a1z;
	body1 = revoluteBody(mBody[1]);
	if(body1)
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& a = mWorldAxis[1];
		a1x = (revoluteMul(m[1], a.y) + revoluteMul(m[2], a.z)) + revoluteMul(m[0], a.x);
		a1y = (revoluteMul(m[4], a.y) + revoluteMul(m[5], a.z)) + revoluteMul(m[3], a.x);
		a1z = (revoluteMul(m[7], a.y) + revoluteMul(m[8], a.z)) + revoluteMul(m[6], a.x);
		}
	else
		{
		a1x = mWorldAxis[1].x;
		a1y = mWorldAxis[1].y;
		a1z = mWorldAxis[1].z;
		}

	// e = a0 x a1; e.x and e.y are stored, e.z stays on the stack.
	const NxReal ex = (NxReal)(a1z * a0y - a1y * a0z);
	const NxReal ey = (NxReal)(a1x * a0z - a0x * a1z);
	const double ez = a0x * a1y - a1x * a0y;
	const NxReal biasNormal = (NxReal)-((((double)ex * n0.x + ez * n0.z) + revoluteMul(ey, n0.y)) * inverse);
	const NxReal biasCross = (NxReal)-((((double)ex * c0.x + ez * c0.z) + revoluteMul(ey, c0.y)) * inverse);

	JointSupportRecord* record = row004093();
	revoluteRecordHeader(record, record0, record1, n0, 0xffffffe3, 3);
	revoluteRecordBits(record);
	revoluteSolveRecord(record, this, biasNormal, mMaxTorque);

	record = row004093();
	revoluteRecordHeader(record, record0, record1, c0, 0xffffffe3, 3);
	revoluteRecordBits(record);
	revoluteSolveRecord(record, this, biasCross, mMaxTorque);
	}

// phys_fn_004362 (0x000ab1d0, 1644 B)
// Fills constraint records (Joint::row004093) for the limit, then either
// the motor or the spring:
// - Limit (flag bit 0, low < high; else the Joint base's slot-7 body,
//   called directly at 0xab4b4): below the low limit, a kind-2 record along
//   row004127's axis with +0x34 = (angle - low) / arg; above the high
//   limit, the same along -axis with +0x34 = -((angle - high) / arg). When
//   that limit's restitution is non-zero and the record's row004389 value is
//   below SDK parameter 4, +0x38 = -(value * restitution).
// - Motor (flag bit 1, maxForce != 0; returns): freeSpin set -> kind 2
//   along the axis (negated for a negative velTarget), +0x38 =
//   |velTarget|; freeSpin clear -> kind 3, +0x38 = velTarget. Both set
//   +0x48 = maxForce and finish with flags (f & 0xfffa6fff) | 0x26800.
// - Spring (flag bit 2, spring or damper non-zero): a kind-3 record with
//   +0x34 = (angle - targetValue) / arg, then row004393(1 / ((spring * arg
//   + damper) * arg), (spring * arg) / (spring * arg + damper)).
// Listing over decompile: the decompile rounds the angle differences and
// the spring terms to float; the listing keeps the angle, the spring
// error and spring * arg on the stack and stores only the values it
// spills (0xab252, 0xab265, 0xab7ab, 0xab7b9, 0xab7fd). The listing reuses
// the argument's stack slot as row004391's dead first output in the motor
// arms (0xab5a1, 0xab695) and as the spill of 1 / (... * arg) (0xab7b9).
void RevoluteJoint::row_slot7(NxReal arg)
	{
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	JointSupportBody* record0 = body0 ? body0->mUnknown204 : 0;
	const JointBodyRecord* body1 = revoluteBody(mBody[1]);
	JointSupportBody* record1 = body1 ? body1->mUnknown204 : 0;

	if((mRevoluteFlags & NX_RJF_LIMIT_ENABLED) && mLimit.low.value < mLimit.high.value)
		{
		const double angle = row004352();
		if(angle < mLimit.low.value)
			{
			NxReal depth = (NxReal)(angle - mLimit.low.value);
			NxVec3 axis;
			row004127(axis);
			depth = (NxReal)((double)depth / arg);
			JointSupportRecord* record = row004093();
			revoluteRecordHeader(record, record0, record1, axis, 0xffffffe2, 2);
			revoluteRecordBits(record);
			revoluteSolveRecord(record, this, depth, NX_MAX_REAL);
			if(mLimit.low.restitution != 0.0f)
				{
				const double value = record->row004389();
				if(value < revoluteSdkParameter(NX_BOUNCE_TRESHOLD))
					record->mUnknown038 = (NxReal)-(value * mLimit.low.restitution);
				}
			}
		else if(angle > mLimit.high.value)
			{
			NxReal depth = (NxReal)(angle - mLimit.high.value);
			NxVec3 axis;
			row004127(axis);
			axis.x = -axis.x;
			axis.y = -axis.y;
			axis.z = -axis.z;
			depth = (NxReal)-((double)depth / arg);
			JointSupportRecord* record = row004093();
			revoluteRecordHeader(record, record0, record1, axis, 0xffffffe2, 2);
			revoluteRecordBits(record);
			revoluteSolveRecord(record, this, depth, NX_MAX_REAL);
			if(mLimit.high.restitution != 0.0f)
				{
				const double value = record->row004389();
				if(value < revoluteSdkParameter(NX_BOUNCE_TRESHOLD))
					record->mUnknown038 = (NxReal)-(value * mLimit.high.restitution);
				}
			}
		}
	else
		{
		// Joint base slot 7 (row 004135), called directly as 0xab4b4 does.
		Joint::row_slot7(arg);
		}

	if((mRevoluteFlags & NX_RJF_MOTOR_ENABLED) && mMotor.maxForce != 0.0f)
		{
		NxVec3 axis;
		row004127(axis);
		JointSupportRecord* record = row004093();
		if(mMotor.freeSpin)
			{
			if(mMotor.velTarget < 0.0f)
				{
				axis.x = -axis.x;
				axis.y = -axis.y;
				axis.z = -axis.z;
				}
			revoluteRecordHeader(record, record0, record1, axis, 0xffffffe2, 2);
			revoluteRecordBits(record);
			revoluteSolveRecord(record, this, 0.0f, mMotor.maxForce);
			record->mUnknown038 = (NxReal)fabs((double)mMotor.velTarget);
			}
		else
			{
			revoluteRecordHeader(record, record0, record1, axis, 0xffffffe3, 3);
			revoluteRecordBits(record);
			revoluteSolveRecord(record, this, 0.0f, mMotor.maxForce);
			record->mUnknown038 = mMotor.velTarget;
			}
		record->mFlags = (record->mFlags & 0xfffa6fff) | 0x26800;
		return;
		}

	if((mRevoluteFlags & NX_RJF_SPRING_ENABLED) && (mSpring.spring != 0.0f || mSpring.damper != 0.0f))
		{
		JointSupportRecord* record = row004093();
		NxVec3 axis;
		row004127(axis);
		const double angle = row004352();
		const double error = (angle - mSpring.targetValue) / arg;
		const double stiffness = revoluteMul(mSpring.spring, arg);
		const double denominator = stiffness + mSpring.damper;
		revoluteRecordHeader(record, record0, record1, axis, 0xffffffe3, 3);
		const NxReal ratio = (NxReal)(stiffness / denominator);
		const NxReal scale = (NxReal)(1.0f / (denominator * arg));
		// The same bit-9 test as revoluteRecordBits, stored as a bit-field
		// (0xab7cd-0xab7e2), then bit 10 = (kind is 2, 3 or 5) with bits
		// 5-8 and 11-18 cleared (0xab7da-0xab80f).
		NxU32 flags = record->mFlags;
		NxU32 kind = flags & 0x1f;
		const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
		flags = (flags & ~0x200u) | (bit9 << 9);
		record->mFlags = flags;
		kind = flags & 0x1f;
		const NxU32 bit10 = (kind == 3 || kind == 2 || kind == 5) ? 1 : 0;
		record->mUnknown034 = (NxReal)error;
		record->mFlags = ((bit10 & 1) << 10) | (flags & 0xfff8021f);
		record->mUnknown038 = 0.0f;
		record->mUnknown044 = 0;
		record->mUnknown04c = 0;
		record->mUnknown048 = NX_MAX_REAL;
		record->mUnknown030 = this;
		record->row004393(scale, ratio);
		}
	}

// phys_fn_004356 (0x000a9650, 2303 B)
// Projection of the given body record (one of mBody[0]/mBody[1]; `ret 4`,
// no null check). After the stale-body refresh:
// 1. d = row004064(mWorldAnchor[0], mWorldAnchor[1]) (body 0's anchor minus
//    body 1's, through the +0x134/+0x158 poses). When |d|^2 >=
//    projectionDistance^2, d is negated for body 0, scaled by
//    (|d| - projectionDistance) / |d| and added to the body's +0x158.
// 2. With a0/a1 the bodies' world axes (rotated by their +0x134 3x3) and
//    dot = a1 . a0: when dot < cos(projectionAngle), the body's axis ("own")
//    is turned towards the other ("other") so that the two are
//    projectionAngle apart: target = cos * other + sin * n, n the unit
//    component of own orthogonal to other; NxFindRotationMatrix(own,
//    target) (the Foundation export, import slot 0x10104174) gives M; the
//    body's orientation becomes M times its +0x134 3x3, stored as the
//    normalised +0x124 quaternion (the NxQuat-from-matrix sequence, inlined)
//    with row 000758 rebuilding +0x134 from it, and +0x158 is moved so the
//    body's own anchor keeps the world point it had after step 1.
// 3. When either step changed the body, row 000022 is called on the body's
//    +0x19c owner with 1.
// Listing over decompile (supplement): the decompile shows the rotated
// axes, the target and the matrix product as floats with reordered terms,
// and compares dot after rounding it; the listing compares the unrounded
// dot (0xa98f9 `fst`, then `fcomp`) and uses the stored float afterwards,
// and keeps the values named double below on the stack. The quaternion
// switch has a default arm (0xa9eb7) that loads an unset local; it is
// unreachable (the index is 0, 1 or 2) and is not reproduced.
void RevoluteJoint::row_slot8(void* bodyPointer)
	{
	JointBodyRecord* body = revoluteBody(bodyPointer);
	bool projected = false;
	revoluteRefreshFirstStaleBody(*this);

	NxVec3 d;
	row004064(mWorldAnchor[0], mWorldAnchor[1], d);
	const double lengthSquared = revoluteSum3(d.x, d.x, d.z, d.z, d.y, d.y);
	if(lengthSquared >= revoluteMul(mProjectionDistance, mProjectionDistance))
		{
		if(bodyPointer == mBody[0])
			{
			d.x = -d.x;
			d.y = -d.y;
			d.z = -d.z;
			}
		// fsqrt of the compared sum (0xa9710), re-formed as in the test
		// (the negation above does not change a square).
		const double length = x87FsqrtDot3(d.x, d.x, d.z, d.z, d.y, d.y);
		projected = true;
		const double ratio = (length - mProjectionDistance) / length;
		d.x = (NxReal)(d.x * ratio);
		d.y = (NxReal)(d.y * ratio);
		d.z = (NxReal)(d.z * ratio);
		NxVec3& position = body->mUnknown158;
		position.x = (NxReal)((double)d.x + position.x);
		position.y = (NxReal)((double)d.y + position.y);
		position.z = (NxReal)((double)d.z + position.z);
		}

	// The world axes, rotated and stored (0xa977c-0xa98d9).
	NxVec3 a0;
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	if(body0)
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& a = mWorldAxis[0];
		a0.y = (NxReal)revoluteSum3(m[5], a.z, m[3], a.x, m[4], a.y);
		a0.z = (NxReal)revoluteSum3(m[8], a.z, m[6], a.x, m[7], a.y);
		a0.x = (NxReal)revoluteSum3(m[2], a.z, m[1], a.y, m[0], a.x);
		}
	else
		{
		a0 = mWorldAxis[0];
		}
	NxVec3 a1;
	const JointBodyRecord* body1 = revoluteBody(mBody[1]);
	if(body1)
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& a = mWorldAxis[1];
		a1.y = (NxReal)revoluteSum3(m[4], a.y, m[3], a.x, m[5], a.z);
		a1.z = (NxReal)revoluteSum3(m[7], a.y, m[6], a.x, m[8], a.z);
		a1.x = (NxReal)revoluteSum3(m[1], a.y, m[2], a.z, m[0], a.x);
		}
	else
		{
		a1 = mWorldAxis[1];
		}

	const double dot = revoluteSum3(a1.x, a0.x, a1.z, a0.z, a1.y, a0.y);
	const NxReal dotF = (NxReal)dot;
	if(dot < mProjectionAngleCos)
		{
		// The body's own anchor in world space, before the turn
		// (0xa990e-0xa99a3).
		const NxVec3& anchor = (bodyPointer == mBody[1]) ? mWorldAnchor[1] : mWorldAnchor[0];
		const NxReal* m = body->mUnknown134;
		const NxVec3& t = body->mUnknown158;
		const double px = revoluteSum3(m[2], anchor.z, m[1], anchor.y, m[0], anchor.x);
		const double py = revoluteSum3(m[5], anchor.z, m[3], anchor.x, m[4], anchor.y);
		const NxReal pz = (NxReal)revoluteSum3(m[8], anchor.z, m[6], anchor.x, m[7], anchor.y);
		const NxReal worldX = (NxReal)(px + t.x);
		const NxReal worldY = (NxReal)(py + t.y);
		const NxReal worldZ = (NxReal)((double)pz + t.z);

		const bool isBody0 = bodyPointer == mBody[0];
		const NxVec3& own = isBody0 ? a0 : a1;
		const NxVec3& other = isBody0 ? a1 : a0;

		// n = (own - dot * other) / sqrt(1 - dot^2) (0xa99be-0xa9a21).
		const double ox = revoluteMul(dotF, other.x);
		const double oy = revoluteMul(dotF, other.y);
		const NxReal oz = (NxReal)revoluteMul(dotF, other.z);
		const NxReal wx = (NxReal)((double)own.x - ox);
		const double wy = (double)own.y - oy;
		const double wz = (double)own.z - oz;
		const double k = 1.0f / x87FsqrtSum2(1.0f, -revoluteMul(dotF, dotF));
		const NxReal nx = (NxReal)(wx * k);
		const NxReal ny = (NxReal)(wy * k);
		const double nz = wz * k;

		// target = cos * other + sin * n (0xa9a23-0xa9a8e).
		const NxReal sine = mProjectionAngleSin;
		const NxReal snx = (NxReal)revoluteMul(nx, sine);
		const NxReal sny = (NxReal)revoluteMul(ny, sine);
		const double snz = nz * sine;
		const NxReal cosine = mProjectionAngleCos;
		const double cox = revoluteMul(cosine, other.x);
		const NxReal coy = (NxReal)revoluteMul(cosine, other.y);
		const NxReal coz = (NxReal)revoluteMul(cosine, other.z);
		NxVec3 target;
		target.x = (NxReal)(cox + snx);
		target.y = (NxReal)((double)coy + sny);
		target.z = (NxReal)((double)coz + snz);
		NxMat33 turn;
		NxFindRotationMatrix(own, target, turn);

		// R = turn * the body's 3x3, all stored (0xa9a96-0xa9c64).
		NxReal M[9];
		for(int e = 0; e < 9; e++)
			M[e] = turn(e / 3, e % 3);
		NxReal R[9];
		R[0] = (NxReal)revoluteSum3(M[0], m[0], M[1], m[3], M[2], m[6]);
		R[1] = (NxReal)revoluteSum3(M[2], m[7], M[1], m[4], M[0], m[1]);
		R[2] = (NxReal)revoluteSum3(M[1], m[5], M[0], m[2], M[2], m[8]);
		R[3] = (NxReal)revoluteSum3(M[3], m[0], M[4], m[3], M[5], m[6]);
		R[4] = (NxReal)revoluteSum3(M[5], m[7], M[3], m[1], M[4], m[4]);
		R[5] = (NxReal)revoluteSum3(M[3], m[2], M[4], m[5], M[5], m[8]);
		R[6] = (NxReal)revoluteSum3(M[6], m[0], M[7], m[3], M[8], m[6]);
		R[7] = (NxReal)revoluteSum3(M[8], m[7], M[6], m[1], M[7], m[4]);
		R[8] = (NxReal)revoluteSum3(M[6], m[2], M[7], m[5], M[8], m[8]);

		// Move the body so its anchor stays put (0xa9c6d-0xa9cf0).
		const double qx = revoluteSum3(R[1], anchor.y, R[2], anchor.z, R[0], anchor.x);
		const NxReal qy = (NxReal)revoluteSum3(R[3], anchor.x, R[4], anchor.y, R[5], anchor.z);
		const NxReal qz = (NxReal)revoluteSum3(R[6], anchor.x, R[7], anchor.y, R[8], anchor.z);
		NxVec3& position = body->mUnknown158;
		position.z = (NxReal)((double)worldZ - qz);
		position.x = (NxReal)((double)worldX - qx);
		position.y = (NxReal)((double)worldY - qy);

		// R to a quaternion (0xa9cf6-0xa9eb5). The trace's first sum is
		// stored (0xa9cfe `fst`) and reused, rounded, by the index-0 arm.
		const NxReal sum84 = (NxReal)((double)R[8] + R[4]);
		const double trace = ((double)R[8] + R[4]) + R[0];
		double x;
		NxReal y, z, w;
		if(trace >= 0.0f)
			{
			const double root = x87FsqrtSum4(R[8], R[4], R[0], 1.0f);
			w = (NxReal)(0.5f * root);
			const NxReal scale = (NxReal)(0.5f / root);
			x = ((double)R[7] - R[5]) * scale;
			y = (NxReal)(((double)R[2] - R[6]) * scale);
			z = (NxReal)(((double)R[3] - R[1]) * scale);
			}
		else
			{
			NxU32 index = 0;
			if(R[4] > R[0])
				index = 1;
			if(R[8] > R[index * 4])
				index = 2;
			switch(index)
				{
				case 0:
					{
					const double root = x87FsqrtSum3(R[0], -sum84, 1.0f);
					const NxReal rootF = (NxReal)root;
					x = root * 0.5f;
					const double scale = 0.5f / (double)rootF;
					y = (NxReal)(((double)R[3] + R[1]) * scale);
					z = (NxReal)(((double)R[6] + R[2]) * scale);
					w = (NxReal)(((double)R[7] - R[5]) * scale);
					}
					break;
				case 1:
					{
					const double root = x87FsqrtDiag(R[4], R[8], R[0]);
					y = (NxReal)(0.5f * root);
					const NxReal scale = (NxReal)(0.5f / root);
					z = (NxReal)(((double)R[7] + R[5]) * scale);
					x = ((double)R[3] + R[1]) * scale;
					w = (NxReal)(((double)R[2] - R[6]) * scale);
					}
					break;
				default:
					{
					const double root = x87FsqrtDiag(R[8], R[4], R[0]);
					z = (NxReal)(0.5f * root);
					const NxReal scale = (NxReal)(0.5f / root);
					x = ((double)R[6] + R[2]) * scale;
					y = (NxReal)(((double)R[7] + R[5]) * scale);
					w = (NxReal)(((double)R[3] - R[1]) * scale);
					}
					break;
				}
			}

		// Normalise and store as x, y, z, w (0xa9ebb-0xa9f24).
		double qxN = x;
		double qyN = y;
		double qzN = z;
		double qwN = w;
		const double norm = x87FsqrtDot4(z, z, y, y, w, w, x, x);
		if(norm != 0.0f)
			{
			const double inverse = 1.0f / norm;
			qxN = x * inverse;
			qyN = y * inverse;
			qzN = z * inverse;
			qwN = w * inverse;
			}
		body->mCMassOrientation[0] = (NxReal)qxN;
		body->mCMassOrientation[1] = (NxReal)qyN;
		body->mCMassOrientation[2] = (NxReal)qzN;
		body->mCMassOrientation[3] = (NxReal)qwN;
		// Row 000758 (core/JointSupport.cpp) rebuilds +0x134 from the
		// quaternion.
		reinterpret_cast<Row000758Fixture*>(body)->row000758();
		}
	else if(!projected)
		{
		return;
		}

	// Row 000022 (core/JointSupport.cpp); its first callee, 000754, is
	// written in core/JointSupport.cpp (Task 6, 21b275d).
	reinterpret_cast<Row000022Fixture*>(body->mOwner)->row000022(1);
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
// NxMath::acos(NxF32) clamp (0xa8dfe-0xa8e34; _CIacos at 0xa8e34) and
// stored as a float (0xa8e39).
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
	desc.projectionAngle = (NxReal)jointAcos(mProjectionAngleCos);
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
// neither the acos result nor the returned product. The clamp and _CIacos
// (0xa9530) are jointAcos/jointCIacos, which leave the result
// unrounded.
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
	const double angle = jointAcos(cosine);
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
// two vec3s of body i's JointSupportBody; a missing body contributes zero.
// Listing over decompile: the decompile shows every intermediate as a float;
// the listing keeps body 0's first cross component and body 1's first two
// cross components and last two sums on the FPU stack, storing only the
// values it spills (0xa9f88, 0xa9fa0, 0xaa025, 0xaa02d).
void RevoluteJoint::row004358(NxVec3& out) const
	{
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	if(body0)
		{
		const JointSupportBody* record = body0->mUnknown204;
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
		const JointSupportBody* record = body1->mUnknown204;
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
// The same angle as row004352, but each body's frame is its orientation
// quaternion (+0x5c) as rows times its +0xdc 3x3 (the getGlobalAxis
// composition) rather than the +0x134 3x3: body 0's normal and cross
// vectors and body 1's normal are carried through that product (a missing
// body contributes the stored world vector), the cosine of the normals is
// stored as a float and clamped through jointAcos (_CIacos at
// 0xad04b), and the result is negated when body 1's normal points against
// body 0's cross vector. The quaternion is converted afresh for each
// vector: once with the conversion keeping most terms on the stack, twice
// with three values already held there and more terms stored
// (revoluteQuatToRows / revoluteQuatToRowsSpilled).
// Listing over decompile: the decompile shows every product sum in a
// different order from the listing (for example body 0's first matrix
// element as m0 * r0 + m6 * r2 + m3 * r1; the listing adds r1 * m3 and
// r2 * m6 first, 0xac840-0xac862) and all intermediates as floats; the
// listing keeps body 0's first normal component on the stack
// (0xac9a5-0xac9c5), stores its other two and every cross/normal-1
// component, and returns the product unrounded in st(0) (004721 rounds it
// with `fstp dword` at 0xb3357).
NxF64 RevoluteJoint::getAngle() const
	{
	// The oracle row is logically const but refreshes the frame cache.
	revoluteRefreshFirstStaleBody(const_cast<RevoluteJoint&>(*this));

	double n0x;
	NxReal n0y, n0z;
	NxVec3 c0;
	const JointBodyRecord* body0 = revoluteBody(mBody[0]);
	if(!body0)
		{
		n0x = mWorldNormal[0].x;
		n0y = mWorldNormal[0].y;
		n0z = mWorldNormal[0].z;
		c0 = mWorldCross[0];
		}
	else
		{
		const NxReal* m = body0->mMassLocalRot;
		NxReal r[9];
		NxReal n[9];

		// Normal (0xac77d-0xaca20).
		revoluteQuatToRows(body0->mOrientation, r);
		n[0] = (NxReal)revoluteSum3(r[1], m[3], r[2], m[6], r[0], m[0]);
		n[1] = (NxReal)revoluteSum3(r[0], m[1], r[1], m[4], r[2], m[7]);
		n[2] = (NxReal)revoluteSum3(r[1], m[5], r[2], m[8], r[0], m[2]);
		n[3] = (NxReal)revoluteSum3(r[3], m[0], r[4], m[3], r[5], m[6]);
		n[4] = (NxReal)revoluteSum3(r[4], m[4], r[5], m[7], r[3], m[1]);
		n[5] = (NxReal)revoluteSum3(r[4], m[5], r[5], m[8], r[3], m[2]);
		n[6] = (NxReal)revoluteSum3(r[6], m[0], r[7], m[3], r[8], m[6]);
		n[7] = (NxReal)revoluteSum3(r[7], m[4], r[8], m[7], r[6], m[1]);
		n[8] = (NxReal)revoluteSum3(r[7], m[5], r[8], m[8], r[6], m[2]);
		const NxVec3& wn = mWorldNormal[0];
		n0x = revoluteSum3(n[0], wn.x, n[1], wn.y, n[2], wn.z);
		n0y = (NxReal)revoluteSum3(n[3], wn.x, n[4], wn.y, n[5], wn.z);
		n0z = (NxReal)revoluteSum3(n[6], wn.x, n[7], wn.y, n[8], wn.z);

		// Cross (0xaca27-0xacce9).
		revoluteQuatToRowsSpilled(body0->mOrientation, r);
		n[0] = (NxReal)revoluteSum3(r[0], m[0], r[1], m[3], r[2], m[6]);
		n[1] = (NxReal)revoluteSum3(r[1], m[4], r[0], m[1], r[2], m[7]);
		n[2] = (NxReal)revoluteSum3(r[1], m[5], r[0], m[2], r[2], m[8]);
		n[3] = (NxReal)revoluteSum3(r[3], m[0], r[4], m[3], r[5], m[6]);
		n[4] = (NxReal)revoluteSum3(r[3], m[1], r[4], m[4], r[5], m[7]);
		n[5] = (NxReal)revoluteSum3(r[3], m[2], r[4], m[5], r[5], m[8]);
		n[6] = (NxReal)revoluteSum3(r[6], m[0], r[7], m[3], r[8], m[6]);
		n[7] = (NxReal)revoluteSum3(r[6], m[1], r[7], m[4], r[8], m[7]);
		n[8] = (NxReal)revoluteSum3(r[6], m[2], r[7], m[5], r[8], m[8]);
		const NxVec3& wc = mWorldCross[0];
		c0.x = (NxReal)revoluteSum3(n[0], wc.x, n[1], wc.y, n[2], wc.z);
		c0.y = (NxReal)revoluteSum3(n[3], wc.x, n[4], wc.y, n[5], wc.z);
		c0.z = (NxReal)revoluteSum3(n[6], wc.x, n[7], wc.y, n[8], wc.z);
		}

	NxVec3 n1;
	const JointBodyRecord* body1 = revoluteBody(mBody[1]);
	if(!body1)
		{
		n1 = mWorldNormal[1];
		}
	else
		{
		// 0xacd23-0xacfe5.
		const NxReal* m = body1->mMassLocalRot;
		NxReal r[9];
		NxReal n[9];
		revoluteQuatToRowsSpilled(body1->mOrientation, r);
		n[0] = (NxReal)revoluteSum3(r[0], m[0], r[1], m[3], r[2], m[6]);
		n[1] = (NxReal)revoluteSum3(r[0], m[1], r[1], m[4], r[2], m[7]);
		n[2] = (NxReal)revoluteSum3(r[0], m[2], r[1], m[5], r[2], m[8]);
		n[3] = (NxReal)revoluteSum3(r[4], m[3], r[5], m[6], r[3], m[0]);
		n[4] = (NxReal)revoluteSum3(r[3], m[1], r[4], m[4], r[5], m[7]);
		n[5] = (NxReal)revoluteSum3(r[3], m[2], r[4], m[5], r[5], m[8]);
		n[6] = (NxReal)revoluteSum3(r[7], m[3], r[8], m[6], r[6], m[0]);
		n[7] = (NxReal)revoluteSum3(r[6], m[1], r[7], m[4], r[8], m[7]);
		n[8] = (NxReal)revoluteSum3(r[6], m[2], r[7], m[5], r[8], m[8]);
		const NxVec3& wn = mWorldNormal[1];
		n1.x = (NxReal)revoluteSum3(n[1], wn.y, n[2], wn.z, n[0], wn.x);
		n1.y = (NxReal)revoluteSum3(n[4], wn.y, n[5], wn.z, n[3], wn.x);
		n1.z = (NxReal)revoluteSum3(n[7], wn.y, n[8], wn.z, n[6], wn.x);
		}

	// 0xacfec-0xad09a.
	const NxReal cosine = (NxReal)(((double)n1.z * n0z + (double)n1.y * n0y) + (double)n1.x * n0x);
	const double angle = jointAcos(cosine);
	const double side = revoluteSum3(n1.z, c0.z, n1.y, c0.y, n1.x, c0.x);
	if(side < 0.0f)
		return angle * -1.0f;
	return angle * 1.0f;
	}
