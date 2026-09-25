/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/Joint.h"
#include "core/JointSupport.h"
#include "Scene.h"
#include "X87Sqrt.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxUtilities.h"

#include <math.h>
#include <new>

// The oracle's __FILE__ for this unit (see revolute-contract.md "## Row
// assignment": the pilot writes it as core/Joint.cpp, the oracle does not).
#define NX_JOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\Joint.cpp"

// Why the intermediates are `double`. These rows are x87 in the oracle: each
// expression is evaluated on the FPU stack and narrowed to 32 bits only where
// the listing stores it (fstp dword). Outside the simulation step the control
// word is the CRT default 0x027f (_PC_53), so a register lifetime is a double
// and a spill is a float; see Physics/src/MassProperties.cpp for the measured
// argument. The code below types every value the listing keeps on the stack
// as `double` and every value it stores as `NxReal`, and keeps the listing's
// operand grouping; a product of two floats is exact in a double, so only the
// grouping of the sums matters. Rows are also reached from inside the step
// (the solver slots call phys_fn_004097), whose control word is 64-bit
// round-toward-zero; CMakeLists.txt builds this translation unit /arch:IA32 so
// the code follows whichever control word is live, as the oracle's does.

// .data 0x10127180: the limit-plane iterator that phys_fn_004081 sets,
// that phys_fn_004083 tests, phys_fn_004145 advances and 004089 clears.
// Physics/src/ObjectModel.cpp keeps its own twin of the same oracle word,
// gNxGlobalFlag4491, for the parameterised Np-accessor models; the two are
// deliberately separate (the models never touch a product Joint).
static JointLimitPlane* gLimitPlaneIterator = 0;

// The float the setters raise a body's wake counter to and compare it with:
// 0x3ecccccc (.rdata 0x101053d4 and the immediate stored), one ulp below 0.4f.
static const NxReal gJointWakeFloor = 0.39999998f;

static NX_INLINE double jointMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

static NX_INLINE JointBodyRecord* jointBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8). Both
// are read by offset: the oracle's actor classes are outside the pilot.
static NX_INLINE void* jointActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* jointBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// Inlined at every site that takes it (004074, 004107 x2, 004121): raise the
// body's wake counter to the floor unless its +0x114 bit 8 is set. The
// compare is `fcomp; test ah,5; jp` -- store only when strictly less.
static void jointRaiseWakeCounter(void* bodyPointer)
	{
	JointBodyRecord* body = jointBody(bodyPointer);
	if(body && !(body->mUnknown114 & 0x100) && body->mWakeUpCounter < gJointWakeFloor)
		body->mWakeUpCounter = gJointWakeFloor;
	}

// The loop 004125, 004127, 004129, 004131 and 004145 open with: refresh the
// first body whose stamp no longer matches, and only that one (the listing
// leaves the loop after the call).
static void jointRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = jointBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// The inlined quaternion-to-rows conversion 004125 (twice) and 004129 share;
// the three sites evaluate it identically (only their spill slots differ).
// q is x, y, z, w; m is row-major.
static void jointQuatToRows(const NxReal* q, NxReal* m)
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

// (a0 * b0 + a1 * b1) + a2 * b2 with the products exact and the sums at
// double precision: the three-term row sums below, written in the order
// the listing adds them.
static NX_INLINE double jointDot3(NxReal a0, NxReal b0, NxReal a1, NxReal b1, NxReal a2, NxReal b2)
	{
	return (jointMul(a0, b0) + jointMul(a1, b1)) + jointMul(a2, b2);
	}

// The actor's global pose as 004099 and 004101 inline it for a body (see
// JointActorBody in core/Joint.h): the rows of the dynamic record's +0x5c
// quaternion (the same inlined conversion as jointQuatToRows; all eight
// copies in the two rows match it instruction for instruction) and its
// +0x50 position, or the static pose at +0x20/+0x44.
static void jointActorPose(const JointBodyRecord* body, NxReal* m, NxVec3& t)
	{
	const JointActorBody* actor = static_cast<const JointActorBody*>(body->mOwner);
	const JointBodyRecord* record = actor->mBody;
	if(record)
		{
		jointQuatToRows(record->mOrientation, m);
		t = record->mPosition;
		}
	else
		{
		for(NxU32 i = 0; i < 9; i++)
			m[i] = actor->mPoseRotation[i];
		t = actor->mPosePosition;
		}
	}

// World point -> body frame, as 004099 inlines it for both bodies
// (0x96468-0x9650e, 0x9666a-0x96709): out = transpose(m) * (p - t). The x
// and z differences are stored as floats; y stays on the stack.
static void jointLocalizePoint(const NxReal* m, const NxVec3& t, const NxVec3& p, NxVec3& out)
	{
	const NxReal dx = (NxReal)((double)p.x - t.x);
	const double dy = (double)p.y - t.y;
	const NxReal dz = (NxReal)((double)p.z - t.z);
	const double x = (jointMul(m[6], dz) + m[3] * dy) + jointMul(m[0], dx);
	const double y = (jointMul(m[7], dz) + m[4] * dy) + jointMul(m[1], dx);
	const double z = (jointMul(m[8], dz) + m[5] * dy) + jointMul(m[2], dx);
	out.z = (NxReal)z;
	out.x = (NxReal)x;
	out.y = (NxReal)y;
	}

// Rows 6-8 of the frame matrix 004101 builds: axis x normal, each
// component stored as a float.
static void jointFrameCross(const NxVec3& a, const NxVec3& n, NxReal* c)
	{
	c[0] = (NxReal)(jointMul(a.y, n.z) - jointMul(a.z, n.y));
	c[1] = (NxReal)(jointMul(a.z, n.x) - jointMul(a.x, n.z));
	c[2] = (NxReal)(jointMul(a.x, n.y) - jointMul(a.y, n.x));
	}

// The inlined NxQuat-from-matrix of phys_fn_004101's four sites, over the
// row-major m (rows: axis, normal, axis x normal). The four sites differ
// only in which pair sum of the diagonal they store as a float and reuse:
//   JOINT_PAIR_08_WIDE   (body 0, both arms): p = m0 + m8 stored; trace =
//                        the unrounded p + m4; case 1 uses the stored p.
//   JOINT_PAIR_08_STORED (body 1 with a body): trace = m4 + the stored p;
//                        case 1 uses the stored p.
//   JOINT_PAIR_48_STORED (body 1 without a body): q = m8 + m4 stored;
//                        trace = m0 + q; case 0 uses q, case 1 the
//                        unrounded m8 + m0.
// Unlike 004121's copy, the non-negative arm divides 0.5 by the float-
// stored root (`fst; ...; fdiv dword`). Writes x, y, z as floats and
// returns w unrounded; the callers negate x, y, z where they store them.
enum JointPairForm
	{
	JOINT_PAIR_08_WIDE,
	JOINT_PAIR_08_STORED,
	JOINT_PAIR_48_STORED
	};

static double jointRowsToQuat(const NxReal* m, JointPairForm form, NxReal* q)
	{
	double trace;
	NxReal pair;
	if(form == JOINT_PAIR_48_STORED)
		{
		pair = (NxReal)((double)m[8] + m[4]);
		trace = (double)m[0] + pair;
		}
	else
		{
		const double wide = (double)m[0] + m[8];
		pair = (NxReal)wide;
		trace = form == JOINT_PAIR_08_WIDE ? wide + m[4] : (double)m[4] + pair;
		}
	if(trace >= 0.0)
		{
		// trace + 1 re-formed from the same operands in the same order.
		double s;
		if(form == JOINT_PAIR_08_WIDE)
			s = x87FsqrtSum4(m[0], m[8], m[4], 1.0);
		else if(form == JOINT_PAIR_08_STORED)
			s = x87FsqrtSum3(m[4], pair, 1.0);
		else
			s = x87FsqrtSum3(m[0], pair, 1.0);
		const NxReal sf = (NxReal)s;
		const double w = s * 0.5f;
		const double r = 0.5f / (double)sf;
		q[0] = (NxReal)(((double)m[7] - m[5]) * r);
		q[1] = (NxReal)(((double)m[2] - m[6]) * r);
		q[2] = (NxReal)(((double)m[3] - m[1]) * r);
		return w;
		}
	NxU32 k = 0;
	if(m[4] > m[0])
		k = 1;
	if(m[8] > m[k * 4])
		k = 2;
	if(k == 0)
		{
		const double s = form == JOINT_PAIR_48_STORED ? x87FsqrtSum3(m[0], -pair, 1.0)
			: x87FsqrtDiag(m[0], m[4], m[8]);
		q[0] = (NxReal)(0.5f * s);
		const double r = 0.5f / s;
		q[1] = (NxReal)(((double)m[3] + m[1]) * r);
		q[2] = (NxReal)(((double)m[6] + m[2]) * r);
		return ((double)m[7] - m[5]) * r;
		}
	if(k == 1)
		{
		const double s = form == JOINT_PAIR_48_STORED ? x87FsqrtDiag(m[4], m[8], m[0])
			: x87FsqrtSum3(m[4], -pair, 1.0);
		q[1] = (NxReal)(0.5f * s);
		const double r = 0.5f / s;
		q[2] = (NxReal)(((double)m[7] + m[5]) * r);
		q[0] = (NxReal)(((double)m[3] + m[1]) * r);
		return ((double)m[2] - m[6]) * r;
		}
	const double s = x87FsqrtDiag(m[8], m[0], m[4]);
	q[2] = (NxReal)(0.5f * s);
	const double r = 0.5f / s;
	q[0] = (NxReal)(((double)m[6] + m[2]) * r);
	q[1] = (NxReal)(((double)m[7] + m[5]) * r);
	return ((double)m[3] - m[1]) * r;
	}

// The frame quaternion 004101 stores: x, y, z negated, w last.
static void jointStoreFrameQuat(const NxVec3& axis, const NxVec3& normal, JointPairForm form, NxReal* target)
	{
	NxReal m[9];
	m[0] = axis.x;
	m[1] = axis.y;
	m[2] = axis.z;
	m[3] = normal.x;
	m[4] = normal.y;
	m[5] = normal.z;
	jointFrameCross(axis, normal, m + 6);
	NxReal q[3];
	const double w = jointRowsToQuat(m, form, q);
	target[2] = -q[2];
	target[0] = -q[0];
	target[1] = -q[1];
	target[3] = (NxReal)w;
	}

// The SDK parameters 004135 reads straight from the live parameter array
// (.data 0x10123b18, PhysicsSDK.cpp's gParameter): element 0 (0x10123b18,
// NX_PENALTY_FORCE) and element 1 (0x10123b1c, NX_MIN_SEPARATION_FOR_
// PENALTY). Read through PhysicsSDK::getParameter as core/RevoluteJoint.cpp
// does (see its revoluteSdkParameter for the no-SDK case).
static NxReal jointSdkParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// phys_fn_004141 (0x00099e60, 464 B)
// The typeBit -> NxJointType map below was read from the oracle's byte table
// at 0x9a048 and jump table 0x9a030 (2->5, 4->4, 8->3, 0x40->1, 0x80->0), not
// inferred. An unknown typeBit (and 0x4000's fall-through) still reaches the
// loadFromDesc call; only the store is skipped for unknown bits.
Joint::Joint(const NxJointDesc& desc, NxU32 typeBit)
	{
	mTypeBit = typeBit;
	mPublicObject = 0;
	mFlags = 0;
	mFlags &= ~0x18u;
	mNextJoint = 0;
	mUnknown034[0] = 0;
	mUnknown034[1] = 0;
	mAccumulated.z = 0.0f;
	mAccumulated.y = 0.0f;
	mAccumulated.x = 0.0f;
	mLimitPoint.z = 0.0f;
	mLimitPoint.y = 0.0f;
	mLimitPoint.x = 0.0f;
	mLimitPlaneHead = 0;
	mProjectionMode = (NxJointProjectionMode)0;

	row004107(jointActorImpl(desc.actor[0]), jointActorImpl(desc.actor[1]), true);

	switch(typeBit)
		{
		case 0x2:		mType = NX_JOINT_POINT_IN_PLANE;	break;
		case 0x4:		mType = NX_JOINT_POINT_ON_LINE;		break;
		case 0x8:		mType = NX_JOINT_SPHERICAL;			break;
		case 0x40:		mType = NX_JOINT_REVOLUTE;			break;
		case 0x80:		mType = NX_JOINT_PRISMATIC;			break;
		case 0x100:		mType = NX_JOINT_CYLINDRICAL;		break;
		case 0x200:		mType = NX_JOINT_FIXED;				break;
		case 0x1000:	mType = NX_JOINT_PULLEY;			break;
		case 0x2000:	mType = NX_JOINT_DISTANCE;			break;
		case 0x4000:	mType = NX_JOINT_D6;				break;
		default:											break;
		}
	loadFromDescBase(desc);
	}

// phys_fn_004095 (0x00095e20, 41 B)
// The compiler reinstalls the base vptr 0x101192d0 on entry, as the listing
// does at 0x95e26. The listing ends with a tail jump to phys_fn_004089.
Joint::~Joint()
	{
	nxSetSdkPointerBinding(this, 0);
	if(mScene)
		// Scene::removeJoint (phys_fn_000633, Physics/src/Scene.cpp).
		static_cast<NxSceneInternal*>(mScene)->removeJoint(this);
	purgeLimitPlanes();
	}

// phys_fn_004111 (0x00097fd0, 113 B)
// Only the supplement decompile exists; it shows the second argument as
// the return address (`unaff_retaddr`). The listing reads it from
// [esp+0xc] after one push (0x98015), i.e. the second stack argument, and
// purges 8 bytes. Unless the record's bits 11-18 are 0x4d or the joint is
// already broken: mark broken ((flags & ~8) | 0x10), flag its records
// (004091), and post a break event carrying the second argument through
// the Scene's phys_fn_000571 (Scene.cpp) -- a null event when the
// allocation fails, as 0x98032 does.
void Joint::row004111(const JointSupportRecord* record, NxReal value)
	{
	if((NxU8)(record->mFlags >> 11) == 0x4d)
		return;
	if((mFlags & 0x18) == 0x10)
		return;
	mFlags = (mFlags & ~8u) | 0x10u;
	row004091();
	void* memory = nxGetSdkAllocator()->malloc(sizeof(JointBreakEvent), NX_MEMORY_PERSISTENT);
	JointBreakEvent* event = memory ? new(memory) JointBreakEvent(this, value) : 0;
	static_cast<NxSceneInternal*>(mScene)->addJointBreakEvent(event);
	}

// phys_fn_004087 (0x00095cc0, 87 B)
void Joint::row004087(NxReal numerator, const NxVec3& v, NxReal divisor)
	{
	const double scale = (double)numerator / divisor;
	const double x = scale * v.x;
	const NxReal y = (NxReal)(scale * v.y);
	const NxReal z = (NxReal)(scale * v.z);
	mAccumulated.x = (NxReal)(x + mAccumulated.x);
	mAccumulated.y = (NxReal)((double)y + mAccumulated.y);
	mAccumulated.z = (NxReal)((double)z + mAccumulated.z);
	}

// phys_fn_004133 (0x00099ab0, 134 B)
// Runs only when some body exists whose +0x10c bit 7 is clear. The two
// calls go through the table (`call [edx+0x1c]`, `call [edx+0x18]`): slot
// 6 here reaches the family's override, never this body, which only the
// scene row phys_fn_000728 calls directly.
void Joint::row_slot6(NxReal arg)
	{
	const JointBodyRecord* body0 = jointBody(mBody[0]);
	if(!body0 || (body0->mUnknown10c & 0x80))
		{
		const JointBodyRecord* body1 = jointBody(mBody[1]);
		if(!body1 || (body1->mUnknown10c & 0x80))
			return;
		}
	jointRefreshFirstStaleBody(*this);
	mFlags = (mFlags & ~0x10u) | 8u;
	row_slot7(arg);
	if(!((mFlags >> 2) & 1))
		row_slot6(arg);
	}

// phys_fn_004135 (0x00099b40, 701 B)
// Clears mAccumulated, refreshes a stale body, then finds the first limit
// plane the world limit point lies behind (row004131 < 0) and fills one
// constraint record for it: the plane normal, the two bodies' +0x204
// records in solver order, n x r for each (r = the limit point relative
// to that body's +0x158; the point itself without a body), +0x34 =
// (distance - SDK parameter 1) / arg, +0x48 = FLT_MAX, kind 0, then
// row004391 and the kind scale 004360/004362 also apply.
// Listing over decompile: the decompile drops the kind tests of the flag
// update (0x99d43, 0x99d48, 0x99d7c) as unreachable; the listing keeps
// them and so does this body. The listing passes the argument's own stack
// slot as row004391's dead first output (0x99d9c), as here.
void Joint::row_slot7(NxReal arg)
	{
	mAccumulated.z = 0.0f;
	mAccumulated.y = 0.0f;
	mAccumulated.x = 0.0f;
	jointRefreshFirstStaleBody(*this);
	const JointLimitPlane* plane = mLimitPlaneHead;
	NxVec3 limitPoint;
	getLimitPoint(limitPoint);
	if(!plane)
		return;
	NxVec3 normal;
	NxReal planeD;
	double distance;
	for(;;)
		{
		distance = row004131(plane, limitPoint, normal, planeD);
		if(distance < 0.0)
			break;
		plane = plane->next;
		if(!plane)
			return;
		}

	const JointBodyRecord* solver0 = jointBody(mSolverBody[0]);
	NxVec3 r0 = limitPoint;
	if(solver0)
		{
		r0.x = (NxReal)((double)limitPoint.x - solver0->mUnknown158.x);
		r0.y = (NxReal)((double)limitPoint.y - solver0->mUnknown158.y);
		r0.z = (NxReal)((double)limitPoint.z - solver0->mUnknown158.z);
		}
	const JointBodyRecord* solver1 = jointBody(mSolverBody[1]);
	NxVec3 r1 = limitPoint;
	if(solver1)
		{
		r1.x = (NxReal)((double)limitPoint.x - solver1->mUnknown158.x);
		r1.y = (NxReal)((double)limitPoint.y - solver1->mUnknown158.y);
		r1.z = (NxReal)((double)limitPoint.z - solver1->mUnknown158.z);
		}
	arg = (NxReal)((distance - jointSdkParameter(NX_MIN_SEPARATION_FOR_PENALTY)) / arg);
	JointSupportBody* support1 = solver1 ? solver1->mUnknown204 : 0;
	JointSupportBody* support0 = solver0 ? solver0->mUnknown204 : 0;

	JointSupportRecord* record = row004093();
	record->mBody[0] = support0;
	record->mUnknown000 = normal;
	record->mBody[1] = support1;
	record->mUnknown018.x = (NxReal)(jointMul(normal.z, r0.y) - jointMul(normal.y, r0.z));
	record->mUnknown018.y = (NxReal)(jointMul(normal.x, r0.z) - jointMul(normal.z, r0.x));
	record->mUnknown018.z = (NxReal)(jointMul(normal.y, r0.x) - jointMul(normal.x, r0.y));
	record->mUnknown024.x = (NxReal)(jointMul(normal.z, r1.y) - jointMul(normal.y, r1.z));
	record->mUnknown024.y = (NxReal)(jointMul(r1.z, normal.x) - jointMul(normal.z, r1.x));
	record->mUnknown024.z = (NxReal)(jointMul(normal.y, r1.x) - jointMul(normal.x, r1.y));

	// Kind 0; bit 9 = (kind is 0 or 2); bit 10 = (kind is 2, 3 or 5);
	// bits 5-8 and 11-18 cleared.
	NxU32 flags = record->mFlags & 0xffffffe0;
	record->mFlags = flags;
	NxU32 kind = flags & 0x1f;
	const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
	flags = (((bit9 << 9) ^ flags) & 0x200) ^ flags;
	record->mFlags = flags;
	kind = flags & 0x1f;
	const NxU32 bit10 = (kind == 3 || kind == 2 || kind == 5) ? 1 : 0;
	record->mUnknown030 = this;
	record->mFlags = ((bit10 & 1) << 10) | (flags & 0xfff8021f);
	record->mUnknown034 = arg;
	record->mUnknown038 = 0.0f;
	record->mUnknown044 = 0;
	record->mUnknown04c = 0;
	record->mUnknown048 = NX_MAX_REAL;
	record->row004391(arg, record->mUnknown040);
	record->mUnknown03c = record->mUnknown040;
	kind = record->mFlags & 0x1f;
	if(kind == 0 || kind == 2)
		record->mUnknown040 = (NxReal)((double)jointSdkParameter(NX_PENALTY_FORCE) * record->mUnknown040);
	else if(kind == 1 || kind == 3)
		record->mUnknown040 = (NxReal)((double)record->mUnknown040 * 0.7f);
	}

// phys_fn_004064 (0x000957a0, 385 B)
// Body 0's transform keeps x on the stack and stores y and z before adding
// the +0x158 translation; body 1's keeps x and y and stores z, and stores
// its translated x before the subtraction (0x958c9). The listing groups the
// two blocks' sums differently; both are kept.
void Joint::row004064(const NxVec3& anchor0, const NxVec3& anchor1, NxVec3& out) const
	{
	const JointBodyRecord* body0 = jointBody(mBody[0]);
	if(body0)
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& t = body0->mUnknown158;
		const NxVec3& p = anchor0;
		const double x = jointDot3(m[1], p.y, m[2], p.z, p.x, m[0]);
		const NxReal y = (NxReal)jointDot3(m[4], p.y, m[5], p.z, m[3], p.x);
		const NxReal z = (NxReal)jointDot3(m[7], p.y, m[8], p.z, m[6], p.x);
		out.x = (NxReal)(x + t.x);
		out.y = (NxReal)((double)y + t.y);
		out.z = (NxReal)((double)z + t.z);
		}
	else
		{
		out = anchor0;
		}
	const JointBodyRecord* body1 = jointBody(mBody[1]);
	if(body1)
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& t = body1->mUnknown158;
		const NxVec3& p = anchor1;
		const double x = jointDot3(m[2], p.z, m[1], p.y, m[0], p.x);
		const double y = jointDot3(m[3], p.x, m[5], p.z, m[4], p.y);
		const NxReal z = (NxReal)jointDot3(m[6], p.x, m[8], p.z, m[7], p.y);
		const NxReal wx = (NxReal)(x + t.x);
		const double wy = y + t.y;
		const double wz = (double)z + t.z;
		out.x = (NxReal)((double)out.x - wx);
		out.y = (NxReal)((double)out.y - wy);
		out.z = (NxReal)((double)out.z - wz);
		}
	else
		{
		out.x = (NxReal)((double)out.x - anchor1.x);
		out.y = (NxReal)((double)out.y - anchor1.y);
		out.z = (NxReal)((double)out.z - anchor1.z);
		}
	}

// phys_fn_004066 (0x00095930, 266 B)
void Joint::saveToDescBase(NxJointDesc& desc) const
	{
	const JointBodyRecord* body0 = jointBody(mBody[0]);
	desc.actor[0] = body0 ? *static_cast<NxActor**>(body0->mOwner) : 0;
	const JointBodyRecord* body1 = jointBody(mBody[1]);
	desc.actor[1] = body1 ? *static_cast<NxActor**>(body1->mOwner) : 0;
	desc.localNormal[0] = mLocalNormal[0];
	desc.localAxis[0] = mLocalAxis[0];
	desc.localAnchor[0] = mLocalAnchor[0];
	desc.localNormal[1] = mLocalNormal[1];
	desc.localAxis[1] = mLocalAxis[1];
	desc.localAnchor[1] = mLocalAnchor[1];
	desc.maxForce = mMaxForce;
	desc.maxTorque = mMaxTorque;
	// No null check on the public object (0x959f9).
	desc.userData = static_cast<NxJoint*>(mPublicObject)->userData;
	if(mFlags & 0x100)
		desc.jointFlags |= NX_JF_COLLISION_ENABLED;
	else
		desc.jointFlags &= ~(NxU32)NX_JF_COLLISION_ENABLED;
	if(mFlags & 0x200)
		desc.jointFlags |= NX_JF_VISUALIZATION;
	else
		desc.jointFlags &= ~(NxU32)NX_JF_VISUALIZATION;
	}

// phys_fn_004070 (0x00095a80, 7 B)
NxJointType Joint::getType() const
	{
	return mType;
	}

// phys_fn_004074 (0x00095ab0, 216 B)
// The broken report goes through getInstance() (the listing tests the
// instance word and executes int3 when it is null) with code 1 at line 0x207;
// the two range reports (maxForce, then maxTorque, each `< 0`) share line
// 0x20a and the maxForce message, as the listing does.
void Joint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_JOINT_CPP, 0x207, 0,
			"Joint::setBreakable: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	if(maxForce < 0.0f || maxTorque < 0.0f)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_JOINT_CPP, 0x20a, 0,
			"Joint::setBreakable: maxForce should be nonnegative!");
		return;
		}
	mMaxForce = maxForce;
	mMaxTorque = maxTorque;
	jointRaiseWakeCounter(mBody[0]);
	jointRaiseWakeCounter(mBody[1]);
	}

// phys_fn_004076 (0x00095b90, 21 B)
void Joint::getBreakable(NxReal& maxForce, NxReal& maxTorque) const
	{
	maxForce = mMaxForce;
	maxTorque = mMaxTorque;
	}

// phys_fn_004078 (0x00095bb0, 10 B)
NxJointState Joint::getState() const
	{
	return (NxJointState)((mFlags >> 3) & 3);
	}

// phys_fn_004080 (0x00095bc0, 208 B)
// Returns true when +0x2c bit 1 is clear (`shr 1; not al; and 1`).
bool Joint::getLimitPoint(NxVec3& worldLimitPoint) const
	{
	const JointBodyRecord* body = jointBody(mSolverBody[0]);
	if(body)
		{
		const NxReal* m = body->mUnknown134;
		const NxVec3& p = mLimitPoint;
		const double x = (jointMul(m[2], p.z) + jointMul(m[1], p.y)) + jointMul(m[0], p.x);
		const NxReal y = (NxReal)((jointMul(m[5], p.z) + jointMul(m[3], p.x)) + jointMul(m[4], p.y));
		const NxReal z = (NxReal)((jointMul(m[8], p.z) + jointMul(m[6], p.x)) + jointMul(m[7], p.y));
		worldLimitPoint.x = (NxReal)(x + body->mUnknown158.x);
		worldLimitPoint.y = (NxReal)((double)y + body->mUnknown158.y);
		worldLimitPoint.z = (NxReal)((double)z + body->mUnknown158.z);
		}
	else
		{
		worldLimitPoint = mLimitPoint;
		}
	return (mFlags & 2) == 0;
	}

// phys_fn_004081 (0x00095c90, 9 B)
void Joint::resetLimitPlaneIterator()
	{
	gLimitPlaneIterator = mLimitPlaneHead;
	}

// phys_fn_004083 (0x00095ca0, 14 B)
bool Joint::hasMoreLimitPlanes() const
	{
	return gLimitPlaneIterator != 0;
	}

// phys_fn_004089 (0x00095d20, 58 B)
// Frees through the SDK allocator (`[[0x101041bc]]` slot +0x14).
void Joint::purgeLimitPlanes()
	{
	while(mLimitPlaneHead)
		{
		JointLimitPlane* plane = mLimitPlaneHead;
		mLimitPlaneHead = plane->next;
		nxGetSdkAllocator()->free(plane);
		}
	gLimitPlaneIterator = 0;
	}

// phys_fn_004091 (0x00095d60, 62 B)
// The listing reloads the Scene's array pointer on every pass (0x95d80).
void Joint::row004091()
	{
	const NxU32 first = mUnknown160[0];
	const NxU32 end = mUnknown160[1] + first;
	for(NxU32 i = first; i < end; i++)
		{
		JointSupportRecord* records = *reinterpret_cast<JointSupportRecord**>(static_cast<NxU8*>(mScene) + 0x5b8);
		records[i].mFlags |= 0x20;
		}
	}

// phys_fn_004093 (0x00095da0, 116 B)
// The Scene's record array: pointer +0x5b8, count +0x5bc, capacity +0x5c0.
// A full array is grown by the Scene row phys_fn_000598 (Scene.cpp). The
// listing reloads mScene for the returned address.
JointSupportRecord* Joint::row004093()
	{
	NxU8* scene = static_cast<NxU8*>(mScene);
	NxU32& count = *reinterpret_cast<NxU32*>(scene + 0x5bc);
	if(count == *reinterpret_cast<NxU32*>(scene + 0x5c0))
		static_cast<NxSceneInternal*>(mScene)->growJointRecords();
	const NxU32 index = count;
	count = index + 1;
	if(mUnknown160[1] == 0)
		{
		mUnknown160[0] = index;
		mUnknown160[1] = 1;
		}
	else
		{
		mUnknown160[1]++;
		}
	return *reinterpret_cast<JointSupportRecord**>(static_cast<NxU8*>(mScene) + 0x5b8) + index;
	}

// phys_fn_004097 (0x00095e50, 1176 B)
// Listing over decompile: the decompile regroups the third sum of each
// rotated vector ((b2*x + b5*y) + b8*z) and shows the anchor difference's z
// as one float; the listing groups ((b8*z + b5*y) + b2*x), uses the unrounded
// z difference for x and the stored float for y and z, and in the negative-
// trace cases uses a float-stored reciprocal (cases 0 and 1) or a
// reciprocal of the float-stored root (case 2).
void Joint::refreshBodyFrame(NxU32 bodyIndex)
	{
	const NxU32 i = bodyIndex;
	const JointBodyRecord* body = jointBody(mBody[i]);
	const NxReal* b = body->mMassLocalRot;

	// World normal, cross and axis: the transpose of the row-major 3x3.
	{
	const NxVec3& v = mLocalNormal[i];
	const double z = (jointMul(b[8], v.z) + jointMul(b[5], v.y)) + jointMul(v.x, b[2]);
	const double x = (jointMul(b[6], v.z) + jointMul(b[3], v.y)) + jointMul(v.x, b[0]);
	const double y = (jointMul(b[7], v.z) + jointMul(b[4], v.y)) + jointMul(b[1], v.x);
	mWorldNormal[i].z = (NxReal)z;
	mWorldNormal[i].x = (NxReal)x;
	mWorldNormal[i].y = (NxReal)y;
	}
	{
	const NxVec3& v = mLocalCross[i];
	const double x = (jointMul(b[6], v.z) + jointMul(b[3], v.y)) + jointMul(v.x, b[0]);
	const double y = (jointMul(b[7], v.z) + jointMul(b[1], v.x)) + jointMul(b[4], v.y);
	const double z = (jointMul(b[8], v.z) + jointMul(b[2], v.x)) + jointMul(b[5], v.y);
	mWorldCross[i].z = (NxReal)z;
	mWorldCross[i].x = (NxReal)x;
	mWorldCross[i].y = (NxReal)y;
	}
	{
	const NxVec3& v = mLocalAxis[i];
	const double x = (jointMul(b[6], v.z) + jointMul(b[3], v.y)) + jointMul(v.x, b[0]);
	const double y = (jointMul(b[7], v.z) + jointMul(b[4], v.y)) + jointMul(b[1], v.x);
	const double z = (jointMul(b[8], v.z) + jointMul(b[5], v.y)) + jointMul(b[2], v.x);
	mWorldAxis[i].z = (NxReal)z;
	mWorldAxis[i].x = (NxReal)x;
	mWorldAxis[i].y = (NxReal)y;
	}

	// World anchor: the transpose applied to (localAnchor - massLocalPos).
	{
	const NxVec3& v = mLocalAnchor[i];
	const NxVec3& t = body->mMassLocalPos;
	const double dx = (double)v.x - t.x;
	const NxReal dy = (NxReal)((double)v.y - t.y);
	const double dz = (double)v.z - t.z;
	const NxReal dzf = (NxReal)dz;
	const double x = (dz * b[6] + jointMul(dy, b[3])) + dx * b[0];
	const double y = (jointMul(dzf, b[7]) + jointMul(dy, b[4])) + dx * b[1];
	const double z = (jointMul(dzf, b[8]) + jointMul(dy, b[5])) + dx * b[2];
	mWorldAnchor[i].z = (NxReal)z;
	mWorldAnchor[i].x = (NxReal)x;
	mWorldAnchor[i].y = (NxReal)y;
	}

	// The 3x3's quaternion (inlined NxQuat-from-matrix), conjugated.
	NxReal qx, qy, qz, qw;
	const NxReal m48 = (NxReal)((double)b[4] + b[8]);
	const double trace = ((double)b[4] + b[8]) + b[0];
	if(trace >= 0.0)
		{
		const double s = x87FsqrtSum4(b[4], b[8], b[0], 1.0);
		qw = (NxReal)(0.5 * s);
		const double r = 0.5 / s;
		qx = (NxReal)(((double)b[7] - b[5]) * r);
		qy = (NxReal)(((double)b[2] - b[6]) * r);
		qz = (NxReal)(((double)b[3] - b[1]) * r);
		}
	else
		{
		NxU32 k = 0;
		if(b[4] > b[0])
			k = 1;
		if(b[8] > b[k * 4])
			k = 2;
		if(k == 0)
			{
			const double s = x87FsqrtSum3(b[0], -m48, 1.0);
			qx = (NxReal)(0.5 * s);
			const NxReal r = (NxReal)(0.5 / s);
			qy = (NxReal)(((double)b[3] + b[1]) * r);
			qz = (NxReal)(((double)b[6] + b[2]) * r);
			qw = (NxReal)(((double)b[7] - b[5]) * r);
			}
		else if(k == 1)
			{
			const double s = x87FsqrtDiag(b[4], b[0], b[8]);
			qy = (NxReal)(0.5 * s);
			const NxReal r = (NxReal)(0.5 / s);
			qz = (NxReal)(((double)b[7] + b[5]) * r);
			qx = (NxReal)(((double)b[3] + b[1]) * r);
			qw = (NxReal)(((double)b[2] - b[6]) * r);
			}
		else
			{
			const double s = x87FsqrtDiag(b[8], b[0], b[4]);
			const NxReal sf = (NxReal)s;
			const double r = 0.5 / sf;
			qx = (NxReal)(((double)b[6] + b[2]) * r);
			qy = (NxReal)(((double)b[7] + b[5]) * r);
			qw = (NxReal)(((double)b[3] - b[1]) * r);
			qz = (NxReal)(0.5 * s);
			}
		}
	const NxReal cx = -qx;
	const NxReal cy = -qy;
	const NxReal cz = -qz;
	const NxReal cw = qw;

	// World frame quaternion = conjugate * local frame quaternion.
	const NxReal* f = mFrameQuat[i];
	const double wx = ((jointMul(cw, f[0]) + jointMul(cy, f[2])) + jointMul(cx, f[3])) - jointMul(cz, f[1]);
	const NxReal wy = (NxReal)(((jointMul(cz, f[0]) + jointMul(cw, f[1])) + jointMul(cy, f[3])) - jointMul(cx, f[2]));
	const NxReal wz = (NxReal)(((jointMul(cx, f[1]) + jointMul(cz, f[3])) + jointMul(cw, f[2])) - jointMul(cy, f[0]));
	const NxReal ww = (NxReal)(((jointMul(cw, f[3]) - jointMul(cx, f[0])) - jointMul(cy, f[1])) - jointMul(cz, f[2]));
	mWorldQuat[i][1] = wy;
	mWorldQuat[i][2] = wz;
	mWorldQuat[i][3] = ww;
	mWorldQuat[i][0] = (NxReal)wx;

	mBodyStamp[i] = jointBody(mBody[i])->mStamp;
	}

// phys_fn_004099 (0x000962f0, 1112 B)
// For each body: none -> the world anchor is the argument; else the local
// anchor is the argument in the actor's frame (jointLocalizePoint over the
// actor's global pose), the frame is refreshed (004097) and the body's
// wake counter raised (no null test: the body is known). The broken report
// tests the Foundation instance and executes int3 without one, like
// 004074 (code 1, line 0xf0).
// Listing over decompile: the decompile shows the three differences as
// floats; the listing keeps the y difference on the stack (0x9648e-0x96495).
void Joint::setGlobalAnchor(const NxVec3& anchor)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_JOINT_CPP, 0xf0, 0,
			"Joint::setGlobalAnchor: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	for(NxU32 i = 0; i < 2; i++)
		{
		if(!mBody[i])
			{
			mWorldAnchor[i] = anchor;
			continue;
			}
		NxReal m[9];
		NxVec3 t;
		jointActorPose(jointBody(mBody[i]), m, t);
		jointLocalizePoint(m, t, anchor, mLocalAnchor[i]);
		refreshBodyFrame(i);
		jointRaiseWakeCounter(mBody[i]);
		}
	}

// phys_fn_004101 (0x00096750, 5302 B)
// Normalizes the argument (unless its length is 0), takes two tangents
// from NxNormalToTangents (the Foundation import at [0x1010418c]; t1 is
// its second output, t2 its third) and, for each body:
// - no body: world axis = the axis, world normal = t2, world cross = t1,
//   and the world frame quaternion from the rows (axis, t2, axis x t2);
// - a body: local axis, normal and cross = the axis, t2 and t1 in the
//   actor's frame (the actor's global pose re-read for each of the three,
//   as the listing does; one read here, the pose cannot change between
//   them), the frame quaternion from the stored local axis and normal, then
//   the frame refresh (004097) and the wake raise.
// Listing over decompile: the per-body transforms group their sums
// differently for body 0 and body 1 (jointDot3 argument order below), and
// the four quaternion sites differ in which diagonal pair sum they store
// as a float and reuse (JointPairForm). The decompile shows every
// intermediate as a float and drops those differences.
void Joint::setGlobalAxis(const NxVec3& axis)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_JOINT_CPP, 0x13c, 0,
			"Joint::setGlobalAxis: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	NxVec3 a = axis;
	const double length = x87FsqrtDot3(a.y, a.y, a.x, a.x, a.z, a.z);
	if(length != 0.0)
		{
		const double scale = 1.0f / length;
		a.x = (NxReal)(a.x * scale);
		a.y = (NxReal)(a.y * scale);
		a.z = (NxReal)(scale * a.z);
		}
	NxVec3 t1;
	NxVec3 t2;
	NxNormalToTangents(a, t1, t2);

	if(!mBody[0])
		{
		mWorldAxis[0] = a;
		mWorldNormal[0] = t2;
		mWorldCross[0] = t1;
		jointStoreFrameQuat(a, t2, JOINT_PAIR_08_WIDE, mWorldQuat[0]);
		}
	else
		{
		NxReal m[9];
		NxVec3 t;
		jointActorPose(jointBody(mBody[0]), m, t);
		{
		const double x = jointDot3(m[0], a.x, m[6], a.z, a.y, m[3]);
		const double y = jointDot3(m[1], a.x, m[7], a.z, a.y, m[4]);
		const double z = jointDot3(m[2], a.x, m[8], a.z, a.y, m[5]);
		mLocalAxis[0].z = (NxReal)z;
		mLocalAxis[0].x = (NxReal)x;
		mLocalAxis[0].y = (NxReal)y;
		}
		{
		const double x = jointDot3(t2.z, m[6], t2.y, m[3], t2.x, m[0]);
		const double y = jointDot3(m[7], t2.z, m[4], t2.y, t2.x, m[1]);
		const double z = jointDot3(m[8], t2.z, m[5], t2.y, t2.x, m[2]);
		mLocalNormal[0].z = (NxReal)z;
		mLocalNormal[0].x = (NxReal)x;
		mLocalNormal[0].y = (NxReal)y;
		}
		{
		const double x = jointDot3(t1.x, m[0], t1.z, m[6], t1.y, m[3]);
		const double y = jointDot3(t1.x, m[1], m[7], t1.z, m[4], t1.y);
		const double z = jointDot3(t1.x, m[2], m[8], t1.z, m[5], t1.y);
		mLocalCross[0].z = (NxReal)z;
		mLocalCross[0].x = (NxReal)x;
		mLocalCross[0].y = (NxReal)y;
		}
		jointStoreFrameQuat(mLocalAxis[0], mLocalNormal[0], JOINT_PAIR_08_WIDE, mFrameQuat[0]);
		refreshBodyFrame(0);
		jointRaiseWakeCounter(mBody[0]);
		}

	if(!mBody[1])
		{
		mWorldAxis[1] = a;
		mWorldNormal[1] = t2;
		mWorldCross[1] = t1;
		jointStoreFrameQuat(a, t2, JOINT_PAIR_48_STORED, mWorldQuat[1]);
		}
	else
		{
		NxReal m[9];
		NxVec3 t;
		jointActorPose(jointBody(mBody[1]), m, t);
		{
		const double x = jointDot3(a.y, m[3], m[6], a.z, m[0], a.x);
		const double y = jointDot3(a.y, m[4], m[1], a.x, m[7], a.z);
		const double z = jointDot3(a.y, m[5], m[2], a.x, m[8], a.z);
		mLocalAxis[1].z = (NxReal)z;
		mLocalAxis[1].x = (NxReal)x;
		mLocalAxis[1].y = (NxReal)y;
		}
		{
		const double x = jointDot3(m[0], t2.x, m[6], t2.z, m[3], t2.y);
		const double y = jointDot3(m[1], t2.x, m[7], t2.z, m[4], t2.y);
		const double z = jointDot3(m[2], t2.x, m[8], t2.z, m[5], t2.y);
		mLocalNormal[1].z = (NxReal)z;
		mLocalNormal[1].x = (NxReal)x;
		mLocalNormal[1].y = (NxReal)y;
		}
		{
		const double x = jointDot3(m[0], t1.x, m[6], t1.z, m[3], t1.y);
		const double y = jointDot3(m[1], t1.x, m[7], t1.z, m[4], t1.y);
		const double z = jointDot3(m[2], t1.x, m[8], t1.z, m[5], t1.y);
		mLocalCross[1].z = (NxReal)z;
		mLocalCross[1].x = (NxReal)x;
		mLocalCross[1].y = (NxReal)y;
		}
		jointStoreFrameQuat(mLocalAxis[1], mLocalNormal[1], JOINT_PAIR_08_STORED, mFrameQuat[1]);
		refreshBodyFrame(1);
		jointRaiseWakeCounter(mBody[1]);
		}
	}

// phys_fn_004107 (0x00097d30, 297 B)
// With suppressAttach false the row detaches first: it wakes and clears the
// bodies, removes the joint from its scene (phys_fn_000633) and
// marks it broken ((flags & ~8) | 0x10); afterwards it re-registers
// through Scene::addJoint (phys_fn_000661). The constructor passes true and
// neither happens.
void Joint::row004107(void* actorImpl0, void* actorImpl1, bool suppressAttach)
	{
	if(!suppressAttach)
		{
		jointRaiseWakeCounter(mBody[0]);
		jointRaiseWakeCounter(mBody[1]);
		mBody[0] = 0;
		mBody[1] = 0;
		if(mScene)
			// Scene::removeJoint (phys_fn_000633), which clears mScene when
			// it unlinks the joint, so the re-registration below is skipped
			// for a joint it removed.
			static_cast<NxSceneInternal*>(mScene)->removeJoint(this);
		mFlags = (mFlags & ~8u) | 0x10u;
		}
	void* body0 = jointBodyOfActorImpl(actorImpl0);
	mBody[0] = body0;
	void* body1 = jointBodyOfActorImpl(actorImpl1);
	mBodyStamp[0] = 0xffffffff;
	mBodyStamp[1] = 0xffffffff;
	mBody[1] = body1;
	if(mFlags & 2)
		{
		mSolverBody[0] = body0;
		mSolverBody[1] = body1;
		}
	else
		{
		mSolverBody[0] = body1;
		mSolverBody[1] = body0;
		}
	jointRaiseWakeCounter(body0);
	jointRaiseWakeCounter(mBody[1]);
	// The listing reads mScene before the flag; the order only matters to an
	// object whose mScene is not yet written (the constructor's call), so the
	// flag is tested first here.
	if(!suppressAttach && mScene)
		static_cast<NxSceneInternal*>(mScene)->addJoint(this);
	}

// phys_fn_004109 (0x00097e60, 366 B)
// The report is the imported error call alone (no instance test), code 1,
// line 0x285. pointIsOnBody2 clears flag bit 1 and puts body 1 first in
// solver order; otherwise bit 1 is set and body 0 comes first. The point
// is stored in the first solver body's +0x134/+0x158 frame (transpose),
// then the limit planes are purged (004089) and both wake counters raised.
// Listing over decompile: the decompile rounds the z difference to float
// everywhere; the listing uses the unrounded one for y (0x97eed `fst`).
void Joint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_JOINT_CPP, 0x285, 0,
			"Joint::setLimitPoint: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	void* first;
	if(pointIsOnBody2)
		{
		mFlags &= ~2u;
		first = mBody[1];
		}
	else
		{
		mFlags |= 2u;
		first = mBody[0];
		}
	mSolverBody[0] = first;
	mSolverBody[1] = pointIsOnBody2 ? mBody[0] : mBody[1];
	const JointBodyRecord* body = jointBody(first);
	if(body)
		{
		const NxReal* m = body->mUnknown134;
		const NxVec3& t = body->mUnknown158;
		const double dx = (double)point.x - t.x;
		const NxReal dy = (NxReal)((double)point.y - t.y);
		const double dz = (double)point.z - t.z;
		const NxReal dzf = (NxReal)dz;
		const double y = (dz * m[7] + jointMul(dy, m[4])) + dx * m[1];
		const double z = (jointMul(dzf, m[8]) + jointMul(dy, m[5])) + dx * m[2];
		const double x = (jointMul(dzf, m[6]) + jointMul(dy, m[3])) + dx * m[0];
		mLimitPoint.x = (NxReal)x;
		mLimitPoint.y = (NxReal)y;
		mLimitPoint.z = (NxReal)z;
		}
	else
		{
		mLimitPoint = point;
		}
	purgeLimitPlanes();
	jointRaiseWakeCounter(mBody[0]);
	jointRaiseWakeCounter(mBody[1]);
	}

// phys_fn_004121 (0x000987a0, 1084 B)
// Listing over decompile: the negative-trace case 0 subtracts the float-
// stored (m4 + m8) (0x988aa `fst [esp+0x14]`), which the decompile shows as
// the unrounded sum.
void Joint::loadFromDescBase(const NxJointDesc& desc)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		mLocalNormal[i] = desc.localNormal[i];
		mLocalAxis[i] = desc.localAxis[i];
		const NxVec3& n = mLocalNormal[i];
		const NxVec3& a = mLocalAxis[i];
		{
		const double y = jointMul(a.x, n.z) - jointMul(a.z, n.x);
		const double z = jointMul(a.y, n.x) - jointMul(n.y, a.x);
		const double x = jointMul(n.y, a.z) - jointMul(a.y, n.z);
		mLocalCross[i].x = (NxReal)x;
		mLocalCross[i].y = (NxReal)y;
		mLocalCross[i].z = (NxReal)z;
		}
		mLocalAnchor[i] = desc.localAnchor[i];

		// Rows: axis, normal, axis x normal (inlined NxQuat-from-matrix).
		NxReal m[9];
		m[0] = a.x;
		m[1] = a.y;
		m[2] = a.z;
		m[3] = n.x;
		m[4] = n.y;
		m[5] = n.z;
		m[6] = (NxReal)(jointMul(a.y, n.z) - jointMul(n.y, a.z));
		m[7] = (NxReal)(jointMul(a.z, n.x) - jointMul(a.x, n.z));
		m[8] = (NxReal)(jointMul(n.y, a.x) - jointMul(a.y, n.x));

		NxReal qx, qy, qz, qw;
		const NxReal m48 = (NxReal)((double)m[4] + m[8]);
		const double trace = ((double)m[4] + m[8]) + m[0];
		if(trace >= 0.0)
			{
			const double s = x87FsqrtSum4(m[4], m[8], m[0], 1.0);
			qw = (NxReal)(0.5 * s);
			const double r = 0.5 / s;
			qx = (NxReal)(((double)m[7] - m[5]) * r);
			qy = (NxReal)(((double)m[2] - m[6]) * r);
			qz = (NxReal)(((double)m[3] - m[1]) * r);
			}
		else
			{
			NxU32 k = 0;
			if(m[4] > m[0])
				k = 1;
			if(m[8] > m[k * 4])
				k = 2;
			if(k == 0)
				{
				const double s = x87FsqrtSum3(m[0], -m48, 1.0);
				qx = (NxReal)(0.5 * s);
				const double r = 0.5 / s;
				qy = (NxReal)(((double)m[3] + m[1]) * r);
				qz = (NxReal)(((double)m[2] + m[6]) * r);
				qw = (NxReal)(((double)m[7] - m[5]) * r);
				}
			else if(k == 1)
				{
				const double s = x87FsqrtDiag(m[4], m[8], m[0]);
				qy = (NxReal)(0.5 * s);
				const double r = 0.5 / s;
				qz = (NxReal)(((double)m[5] + m[7]) * r);
				qx = (NxReal)(((double)m[3] + m[1]) * r);
				qw = (NxReal)(((double)m[2] - m[6]) * r);
				}
			else
				{
				const double s = x87FsqrtDiag(m[8], m[4], m[0]);
				qz = (NxReal)(0.5 * s);
				const double r = 0.5 / s;
				qx = (NxReal)(((double)m[2] + m[6]) * r);
				qy = (NxReal)(((double)m[5] + m[7]) * r);
				qw = (NxReal)(((double)m[3] - m[1]) * r);
				}
			}
		mFrameQuat[i][3] = qw;
		mFrameQuat[i][2] = -qz;
		mFrameQuat[i][0] = -qx;
		mFrameQuat[i][1] = -qy;

		if(mBody[i] == 0)
			{
			mWorldNormal[i] = mLocalNormal[i];
			mWorldAxis[i] = mLocalAxis[i];
			mWorldCross[i] = mLocalCross[i];
			mWorldAnchor[i] = mLocalAnchor[i];
			mWorldQuat[i][0] = mFrameQuat[i][0];
			mWorldQuat[i][1] = mFrameQuat[i][1];
			mWorldQuat[i][2] = mFrameQuat[i][2];
			mWorldQuat[i][3] = mFrameQuat[i][3];
			}
		else
			{
			refreshBodyFrame(i);
			}
		}

	mMaxForce = desc.maxForce;
	mMaxTorque = desc.maxTorque;
	// Written only when the public object exists; the constructor's call runs
	// before phys_fn_004366 stores it, so there it is skipped.
	if(mPublicObject)
		static_cast<NxJoint*>(mPublicObject)->userData = desc.userData;
	nxSetSdkPointerBinding(this, const_cast<char*>(desc.name));
	if(desc.jointFlags & NX_JF_COLLISION_ENABLED)
		mFlags |= 0x100;
	else
		mFlags &= ~0x100u;
	if(desc.jointFlags & NX_JF_VISUALIZATION)
		mFlags |= 0x200;
	else
		mFlags &= ~0x200u;
	jointRaiseWakeCounter(mBody[0]);
	jointRaiseWakeCounter(mBody[1]);
	}

// phys_fn_004123 (0x00098be0, 518 B)
// Like getGlobalAnchor (004125) but through the bodies' +0x134/+0x158
// poses instead of the actor pose: out = (pose0 * worldAnchor[0] + pose1 *
// worldAnchor[1]) * 0.5, a missing body contributing its world anchor
// as is. Body 0 keeps x on the stack and stores y and z; body 1 keeps x
// and y and stores z, and stores its translated x before the sum.
void Joint::row004123(NxVec3& out)
	{
	jointRefreshFirstStaleBody(*this);
	const JointBodyRecord* body0 = jointBody(mBody[0]);
	if(!body0)
		{
		out = mWorldAnchor[0];
		}
	else
		{
		const NxReal* m = body0->mUnknown134;
		const NxVec3& t = body0->mUnknown158;
		const NxVec3& a = mWorldAnchor[0];
		const double x = jointDot3(m[1], a.y, m[2], a.z, m[0], a.x);
		const NxReal y = (NxReal)jointDot3(m[4], a.y, m[3], a.x, m[5], a.z);
		const NxReal z = (NxReal)jointDot3(m[7], a.y, m[6], a.x, m[8], a.z);
		out.x = (NxReal)(x + t.x);
		out.y = (NxReal)((double)y + t.y);
		out.z = (NxReal)((double)z + t.z);
		}
	const JointBodyRecord* body1 = jointBody(mBody[1]);
	double sumZ;
	if(!body1)
		{
		out.x = (NxReal)((double)mWorldAnchor[1].x + out.x);
		out.y = (NxReal)((double)mWorldAnchor[1].y + out.y);
		sumZ = mWorldAnchor[1].z;
		}
	else
		{
		const NxReal* m = body1->mUnknown134;
		const NxVec3& t = body1->mUnknown158;
		const NxVec3& b = mWorldAnchor[1];
		const double x = jointDot3(m[2], b.z, m[1], b.y, m[0], b.x);
		const double y = jointDot3(m[5], b.z, m[4], b.y, m[3], b.x);
		const NxReal z = (NxReal)jointDot3(m[8], b.z, m[7], b.y, m[6], b.x);
		const NxReal wx = (NxReal)(x + t.x);
		const double wy = y + t.y;
		sumZ = (double)z + t.z;
		out.x = (NxReal)((double)wx + out.x);
		out.y = (NxReal)(wy + out.y);
		}
	out.z = (NxReal)(sumZ + out.z);
	out.x = (NxReal)((double)out.x * 0.5f);
	out.y = (NxReal)((double)out.y * 0.5f);
	out.z = (NxReal)((double)out.z * 0.5f);
	}

// phys_fn_004125 (0x00098df0, 1940 B)
// The midpoint of the anchor as each body carries it: body pose (quaternion
// +0x5c, position +0x50) composed with the 3x3/vec3 at +0xdc/+0x100, applied
// to mWorldAnchor[i]; a missing body contributes mWorldAnchor[i] itself.
// The two body blocks are the same inlined code with different register
// allocation, so their sum groupings and spills differ; both are kept.
void Joint::getGlobalAnchor(NxVec3& out) const
	{
	// The oracle row is logically const but refreshes the frame cache.
	jointRefreshFirstStaleBody(const_cast<Joint&>(*this));

	const JointBodyRecord* body0 = jointBody(mBody[0]);
	if(!body0)
		{
		out = mWorldAnchor[0];
		}
	else
		{
		NxReal r[9];
		jointQuatToRows(body0->mOrientation, r);
		const NxReal* m = body0->mMassLocalRot;
		const NxVec3& t = body0->mMassLocalPos;
		const NxVec3& p = body0->mPosition;
		const double tx = (jointMul(r[1], t.y) + jointMul(r[2], t.z)) + jointMul(r[0], t.x);
		const NxReal ty = (NxReal)((jointMul(r[3], t.x) + jointMul(r[4], t.y)) + jointMul(r[5], t.z));
		const NxReal tz = (NxReal)((jointMul(r[6], t.x) + jointMul(r[7], t.y)) + jointMul(r[8], t.z));
		const NxReal px = (NxReal)((double)p.x + tx);
		const double py = (double)p.y + ty;
		const NxReal pz = (NxReal)((double)p.z + tz);
		NxReal n[9];
		n[0] = (NxReal)((jointMul(r[1], m[3]) + jointMul(r[2], m[6])) + jointMul(r[0], m[0]));
		n[1] = (NxReal)((jointMul(r[0], m[1]) + jointMul(r[2], m[7])) + jointMul(r[1], m[4]));
		n[2] = (NxReal)((jointMul(r[2], m[8]) + jointMul(r[1], m[5])) + jointMul(r[0], m[2]));
		n[3] = (NxReal)((jointMul(r[5], m[6]) + jointMul(r[3], m[0])) + jointMul(r[4], m[3]));
		n[4] = (NxReal)((jointMul(r[5], m[7]) + jointMul(r[3], m[1])) + jointMul(r[4], m[4]));
		n[5] = (NxReal)((jointMul(r[5], m[8]) + jointMul(r[3], m[2])) + jointMul(r[4], m[5]));
		n[6] = (NxReal)((jointMul(r[6], m[0]) + jointMul(r[7], m[3])) + jointMul(r[8], m[6]));
		n[7] = (NxReal)((jointMul(r[8], m[7]) + jointMul(r[6], m[1])) + jointMul(r[7], m[4]));
		n[8] = (NxReal)((jointMul(r[6], m[2]) + jointMul(r[7], m[5])) + jointMul(r[8], m[8]));
		const NxVec3& a = mWorldAnchor[0];
		const double ux = (jointMul(n[0], a.x) + jointMul(n[1], a.y)) + jointMul(n[2], a.z);
		const NxReal uy = (NxReal)((jointMul(n[3], a.x) + jointMul(n[4], a.y)) + jointMul(n[5], a.z));
		const NxReal uz = (NxReal)((jointMul(n[6], a.x) + jointMul(n[7], a.y)) + jointMul(n[8], a.z));
		out.x = (NxReal)(ux + px);
		out.y = (NxReal)(py + uy);
		out.z = (NxReal)((double)uz + pz);
		}

	const JointBodyRecord* body1 = jointBody(mBody[1]);
	double sumZ;
	if(!body1)
		{
		out.x = (NxReal)((double)mWorldAnchor[1].x + out.x);
		out.y = (NxReal)((double)mWorldAnchor[1].y + out.y);
		sumZ = mWorldAnchor[1].z;
		}
	else
		{
		NxReal r[9];
		jointQuatToRows(body1->mOrientation, r);
		const NxReal* m = body1->mMassLocalRot;
		const NxVec3& t = body1->mMassLocalPos;
		const NxVec3& p = body1->mPosition;
		const double tx = (jointMul(r[1], t.y) + jointMul(r[2], t.z)) + jointMul(r[0], t.x);
		const NxReal ty = (NxReal)((jointMul(r[3], t.x) + jointMul(r[4], t.y)) + jointMul(r[5], t.z));
		const NxReal tz = (NxReal)((jointMul(r[6], t.x) + jointMul(r[7], t.y)) + jointMul(r[8], t.z));
		const NxReal px = (NxReal)((double)p.x + tx);
		const double py = (double)p.y + ty;
		const double pz = (double)p.z + tz;
		NxReal n[9];
		n[0] = (NxReal)((jointMul(r[1], m[3]) + jointMul(r[2], m[6])) + jointMul(r[0], m[0]));
		n[1] = (NxReal)((jointMul(r[1], m[4]) + jointMul(r[0], m[1])) + jointMul(r[2], m[7]));
		n[2] = (NxReal)((jointMul(r[2], m[8]) + jointMul(r[1], m[5])) + jointMul(r[0], m[2]));
		n[3] = (NxReal)((jointMul(r[3], m[0]) + jointMul(r[4], m[3])) + jointMul(r[5], m[6]));
		n[4] = (NxReal)((jointMul(r[3], m[1]) + jointMul(r[4], m[4])) + jointMul(r[5], m[7]));
		n[5] = (NxReal)((jointMul(r[5], m[8]) + jointMul(r[3], m[2])) + jointMul(r[4], m[5]));
		n[6] = (NxReal)((jointMul(r[6], m[0]) + jointMul(r[7], m[3])) + jointMul(r[8], m[6]));
		n[7] = (NxReal)((jointMul(r[6], m[1]) + jointMul(r[7], m[4])) + jointMul(r[8], m[7]));
		n[8] = (NxReal)((jointMul(r[8], m[8]) + jointMul(r[6], m[2])) + jointMul(r[7], m[5]));
		const NxVec3& a = mWorldAnchor[1];
		const double ux = (jointMul(n[1], a.y) + jointMul(n[2], a.z)) + jointMul(n[0], a.x);
		const double uy = (jointMul(n[3], a.x) + jointMul(n[4], a.y)) + jointMul(n[5], a.z);
		const NxReal uz = (NxReal)((jointMul(n[6], a.x) + jointMul(n[7], a.y)) + jointMul(n[8], a.z));
		const NxReal vx = (NxReal)(ux + px);
		const NxReal vy = (NxReal)(uy + py);
		sumZ = (double)uz + pz;
		out.x = (NxReal)((double)vx + out.x);
		out.y = (NxReal)((double)vy + out.y);
		}
	out.z = (NxReal)(sumZ + out.z);
	out.x = (NxReal)((double)out.x * 0.5f);
	out.y = (NxReal)((double)out.y * 0.5f);
	out.z = (NxReal)((double)out.z * 0.5f);
	}

// phys_fn_004127 (0x00099590, 235 B)
void Joint::row004127(NxVec3& out) const
	{
	jointRefreshFirstStaleBody(const_cast<Joint&>(*this));
	const JointBodyRecord* body = jointBody(mBody[0]);
	if(!body)
		{
		out = mWorldAxis[0];
		return;
		}
	const NxReal* m = body->mUnknown134;
	const NxVec3& a = mWorldAxis[0];
	const double y = (jointMul(m[3], a.x) + jointMul(m[5], a.z)) + jointMul(m[4], a.y);
	const double z = (jointMul(m[6], a.x) + jointMul(m[8], a.z)) + jointMul(m[7], a.y);
	const double x = (jointMul(m[2], a.z) + jointMul(m[1], a.y)) + jointMul(a.x, m[0]);
	out.x = (NxReal)x;
	out.y = (NxReal)y;
	out.z = (NxReal)z;
	}

// phys_fn_004129 (0x00099680, 787 B)
void Joint::getGlobalAxis(NxVec3& out) const
	{
	// The oracle row is logically const but refreshes the frame cache.
	jointRefreshFirstStaleBody(const_cast<Joint&>(*this));

	const JointBodyRecord* body = jointBody(mBody[0]);
	if(!body)
		{
		out = mWorldAxis[0];
		return;
		}
	NxReal r[9];
	jointQuatToRows(body->mOrientation, r);
	const NxReal* m = body->mMassLocalRot;
	NxReal n[9];
	n[0] = (NxReal)((jointMul(r[1], m[3]) + jointMul(r[2], m[6])) + jointMul(r[0], m[0]));
	n[1] = (NxReal)((jointMul(r[1], m[4]) + jointMul(r[2], m[7])) + jointMul(r[0], m[1]));
	n[2] = (NxReal)((jointMul(r[1], m[5]) + jointMul(r[2], m[8])) + jointMul(r[0], m[2]));
	n[3] = (NxReal)((jointMul(r[3], m[0]) + jointMul(r[4], m[3])) + jointMul(r[5], m[6]));
	n[4] = (NxReal)((jointMul(r[4], m[4]) + jointMul(r[5], m[7])) + jointMul(r[3], m[1]));
	n[5] = (NxReal)((jointMul(r[4], m[5]) + jointMul(r[5], m[8])) + jointMul(r[3], m[2]));
	n[6] = (NxReal)((jointMul(r[6], m[0]) + jointMul(r[7], m[3])) + jointMul(r[8], m[6]));
	n[7] = (NxReal)((jointMul(r[7], m[4]) + jointMul(r[8], m[7])) + jointMul(r[6], m[1]));
	n[8] = (NxReal)((jointMul(r[7], m[5]) + jointMul(r[8], m[8])) + jointMul(r[6], m[2]));
	const NxVec3& a = mWorldAxis[0];
	const double x = (jointMul(n[0], a.x) + jointMul(n[1], a.y)) + jointMul(n[2], a.z);
	const double y = (jointMul(n[3], a.x) + jointMul(n[4], a.y)) + jointMul(n[5], a.z);
	const double z = (jointMul(n[6], a.x) + jointMul(n[7], a.y)) + jointMul(n[8], a.z);
	out.x = (NxReal)x;
	out.y = (NxReal)y;
	out.z = (NxReal)z;
	}

// phys_fn_004131 (0x000999a0, 260 B)
NxF64 Joint::row004131(const JointLimitPlane* plane, const NxVec3& point, NxVec3& planeNormal, NxReal& planeD)
	{
	jointRefreshFirstStaleBody(*this);
	const JointBodyRecord* body = jointBody(mSolverBody[1]);
	if(body)
		{
		const NxReal* m = body->mUnknown134;
		const NxVec3& n = plane->normal;
		const double y = (jointMul(m[4], n.y) + jointMul(m[3], n.x)) + jointMul(m[5], n.z);
		const double z = (jointMul(m[7], n.y) + jointMul(m[6], n.x)) + jointMul(m[8], n.z);
		const double x = (jointMul(m[1], n.y) + jointMul(m[2], n.z)) + jointMul(n.x, m[0]);
		planeNormal.x = (NxReal)x;
		planeNormal.y = (NxReal)y;
		planeNormal.z = (NxReal)z;
		const NxVec3& t = body->mUnknown158;
		planeD = (NxReal)((double)plane->d - ((z * t.z + y * t.y) + jointMul(planeNormal.x, t.x)));
		}
	else
		{
		planeNormal = plane->normal;
		planeD = plane->d;
		}
	return ((jointMul(point.z, planeNormal.z) + jointMul(point.y, planeNormal.y)) + jointMul(point.x, planeNormal.x)) + planeD;
	}

// phys_fn_004137 (0x00099e00, 41 B)
NxVec3 Joint::getGlobalAnchorVal() const
	{
	NxVec3 anchor;
	getGlobalAnchor(anchor);
	return anchor;
	}

// phys_fn_004139 (0x00099e30, 41 B)
NxVec3 Joint::getGlobalAxisVal() const
	{
	NxVec3 axis;
	getGlobalAxis(axis);
	return axis;
	}

// phys_fn_004143 (0x0009a0d0, 860 B)
// The report is the imported error call alone (code 1, line 0x2c1). The
// node is allocated before the stale-body refresh and without a null test
// (0x9a114). Normal and point go into the second solver body's frame
// (transpose of +0x134, point less +0x158), the normal is normalized
// (unless its length is 0), d = -(p . n); the world limit point (the
// first solver body's pose applied to mLimitPoint, inlined with its own
// grouping, not a call of 004080) is then tested against the new plane
// through 004131 (the argument's stack slot takes the dead planeD):
// behind it (< 0) the node is freed and the row returns false; otherwise
// it is linked at the head and both wake counters raised.
// Listing over decompile: the local point's x uses the unrounded z
// difference (0x9a24c `fst`), and d is ((-(y * n.y)) - z * n.z) - x * n.x
// with -(y * n.y) formed as a multiply by -1.0f (0x9a2c5).
bool Joint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_JOINT_CPP, 0x2c1, 0,
			"Joint::addLimitPlane: Joint is broken. Broken joints can't be manipulated!");
		return false;
		}
	JointLimitPlane* plane = static_cast<JointLimitPlane*>(nxGetSdkAllocator()->malloc(sizeof(JointLimitPlane), NX_MEMORY_PERSISTENT));
	jointRefreshFirstStaleBody(*this);

	const JointBodyRecord* body = jointBody(mSolverBody[1]);
	if(body)
		{
		const NxReal* m = body->mUnknown134;
		const double y = jointDot3(m[1], normal.x, m[4], normal.y, m[7], normal.z);
		const double z = jointDot3(m[2], normal.x, m[5], normal.y, m[8], normal.z);
		const double x = jointDot3(m[3], normal.y, m[6], normal.z, m[0], normal.x);
		plane->normal.x = (NxReal)x;
		plane->normal.y = (NxReal)y;
		plane->normal.z = (NxReal)z;
		}
	else
		{
		plane->normal = normal;
		}
	{
	const NxVec3& n = plane->normal;
	const double length = x87FsqrtDot3(n.x, n.x, n.y, n.y, n.z, n.z);
	if(length != 0.0)
		{
		const double scale = 1.0f / length;
		plane->normal.x = (NxReal)(scale * plane->normal.x);
		plane->normal.y = (NxReal)(scale * plane->normal.y);
		plane->normal.z = (NxReal)(scale * plane->normal.z);
		}
	}
	double px;
	double py;
	double pz;
	if(body)
		{
		const NxReal* m = body->mUnknown134;
		const NxVec3& t = body->mUnknown158;
		const NxReal dx = (NxReal)((double)pointInPlane.x - t.x);
		const NxReal dy = (NxReal)((double)pointInPlane.y - t.y);
		const double dz = (double)pointInPlane.z - t.z;
		const NxReal dzf = (NxReal)dz;
		px = (dz * m[6] + jointMul(dy, m[3])) + jointMul(dx, m[0]);
		py = (jointMul(dzf, m[7]) + jointMul(dy, m[4])) + jointMul(dx, m[1]);
		pz = (jointMul(dzf, m[8]) + jointMul(dy, m[5])) + jointMul(dx, m[2]);
		}
	else
		{
		px = pointInPlane.x;
		py = pointInPlane.y;
		pz = pointInPlane.z;
		}
	plane->d = (NxReal)((((py * plane->normal.y) * -1.0f) - pz * plane->normal.z) - px * plane->normal.x);

	NxVec3 limitPoint;
	const JointBodyRecord* first = jointBody(mSolverBody[0]);
	if(first)
		{
		const NxReal* m = first->mUnknown134;
		const NxVec3& t = first->mUnknown158;
		const NxVec3& p = mLimitPoint;
		const double x = jointDot3(m[1], p.y, m[2], p.z, p.x, m[0]);
		const NxReal y = (NxReal)jointDot3(m[4], p.y, m[3], p.x, m[5], p.z);
		const NxReal z = (NxReal)jointDot3(m[7], p.y, m[6], p.x, m[8], p.z);
		limitPoint.z = (NxReal)((double)z + t.z);
		limitPoint.x = (NxReal)(x + t.x);
		limitPoint.y = (NxReal)((double)y + t.y);
		}
	else
		{
		limitPoint = mLimitPoint;
		}
	NxVec3 planeNormal;
	NxReal planeD;
	if(row004131(plane, limitPoint, planeNormal, planeD) < 0.0)
		{
		nxGetSdkAllocator()->free(plane);
		return false;
		}
	plane->next = mLimitPlaneHead;
	mLimitPlaneHead = plane;
	jointRaiseWakeCounter(mBody[0]);
	jointRaiseWakeCounter(mBody[1]);
	return true;
	}

// phys_fn_004145 (0x0009a430, 174 B)
// The report here is the imported error call alone (no instance test, code
// 2, line 0x30b), unlike 004074's.
bool Joint::getNextLimitPlane(NxVec3& planeNormal, NxReal& planeD)
	{
	jointRefreshFirstStaleBody(*this);
	if(!gLimitPlaneIterator)
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_OPERATION, NX_JOINT_CPP, 0x30b, 0,
			"Joint::getNextLimitPlane: you didn't call resetLimitPlaneIterator first!");
		return false;
		}
	NxVec3 limitPoint;
	getLimitPoint(limitPoint);
	const bool inFront = row004131(gLimitPlaneIterator, limitPoint, planeNormal, planeD) > 0.0;
	gLimitPlaneIterator = gLimitPlaneIterator->next;
	return inFront;
	}
