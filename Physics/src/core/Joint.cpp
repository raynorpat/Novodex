/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/Joint.h"
#include "core/JointSupport.h"
#include "NxJoint.h"

#include <math.h>

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
// grouping of the sums matters. Rows reached from inside the step (the solver
// slots call phys_fn_004097) would need /arch:IA32 to follow its 64-bit
// round-toward-zero control word; this translation unit is SSE2 and does not.

// phys_fn_000661 Scene::addJoint (thiscall on the Scene, `ret 4`). Reused as
// the candidate's no-op hole; defined in Physics/src/Scene.cpp.
void nxSceneAddJoint(void* scene, void* joint);

// .data 0x10127180: the limit-plane iterator phys_fn_004081 sets,
// phys_fn_004083 tests, phys_fn_004145 advances and phys_fn_004089 clears.
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
		// phys_fn_000633 is deferred (Scene joint removal, reached only on
		// release); its stub asserts.
		reinterpret_cast<Row000633Fixture*>(mScene)->row000633(this);
	purgeLimitPlanes();
	}

// phys_fn_004111 (0x00097fd0, 113 B)
// (deferred: break test from the solver; needs phys_fn_000571, 004091 and the break event)
void Joint::row004111(NxU32 a, NxU32 b)
	{
	(void)a;
	(void)b;
	NX_ASSERT(0);
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
// (deferred: Joint base slot 6 default, overridden by phys_fn_004360 in RevoluteJoint)
void Joint::row_slot6(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004135 (0x00099b40, 701 B)
// (deferred: internal slot 7 (solver); overridden by phys_fn_004362 in RevoluteJoint)
void Joint::row_slot7(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004064 (0x000957a0, 385 B)
// (deferred: internal slot 8 (phys_fn_004356), called only from scene code outside the pilot)
void Joint::row004064(NxVec3& out1, NxVec3& out2, const NxVec3& in1, const NxVec3& in2)
	{
	(void)out1;
	(void)out2;
	(void)in1;
	(void)in2;
	NX_ASSERT(0);
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

// phys_fn_004093 (0x00095da0, 116 B)
// (deferred: internal slots 6/7 (solver); also needs Scene row phys_fn_000598, absent from the candidate)
void Joint::row004093(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
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
		const double s = sqrt(trace + 1.0);
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
			const double s = sqrt(((double)b[0] - m48) + 1.0);
			qx = (NxReal)(0.5 * s);
			const NxReal r = (NxReal)(0.5 / s);
			qy = (NxReal)(((double)b[3] + b[1]) * r);
			qz = (NxReal)(((double)b[6] + b[2]) * r);
			qw = (NxReal)(((double)b[7] - b[5]) * r);
			}
		else if(k == 1)
			{
			const double s = sqrt(((double)b[4] - ((double)b[0] + b[8])) + 1.0);
			qy = (NxReal)(0.5 * s);
			const NxReal r = (NxReal)(0.5 / s);
			qz = (NxReal)(((double)b[7] + b[5]) * r);
			qx = (NxReal)(((double)b[3] + b[1]) * r);
			qw = (NxReal)(((double)b[2] - b[6]) * r);
			}
		else
			{
			const double s = sqrt(((double)b[8] - ((double)b[0] + b[4])) + 1.0);
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
// (deferred: NxJoint::setGlobalAnchor -- the joint test never calls it)
void Joint::setGlobalAnchor(const NxVec3& anchor)
	{
	(void)anchor;
	NX_ASSERT(0);
	}

// phys_fn_004101 (0x00096750, 5302 B)
// (deferred: NxJoint::setGlobalAxis -- not called)
void Joint::setGlobalAxis(const NxVec3& axis)
	{
	(void)axis;
	NX_ASSERT(0);
	}

// phys_fn_004107 (0x00097d30, 297 B)
// With suppressAttach false the row detaches first: it wakes and clears the
// bodies, removes the joint from its scene (deferred phys_fn_000633) and
// marks it broken ((flags & ~8) | 0x10); afterwards it re-registers through
// phys_fn_000661. The constructor passes true and neither happens.
void Joint::row004107(void* actorImpl0, void* actorImpl1, bool suppressAttach)
	{
	if(!suppressAttach)
		{
		jointRaiseWakeCounter(mBody[0]);
		jointRaiseWakeCounter(mBody[1]);
		mBody[0] = 0;
		mBody[1] = 0;
		if(mScene)
			// phys_fn_000633 is deferred (reached only when phys_fn_004370
			// re-binds actors); its stub asserts.
			reinterpret_cast<Row000633Fixture*>(mScene)->row000633(this);
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
		nxSceneAddJoint(mScene, this);
	}

// phys_fn_004109 (0x00097e60, 366 B)
// (deferred: NxJoint::setLimitPoint -- not called)
void Joint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	(void)point;
	(void)pointIsOnBody2;
	NX_ASSERT(0);
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
			const double s = sqrt(trace + 1.0);
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
				const double s = sqrt(((double)m[0] - m48) + 1.0);
				qx = (NxReal)(0.5 * s);
				const double r = 0.5 / s;
				qy = (NxReal)(((double)m[3] + m[1]) * r);
				qz = (NxReal)(((double)m[2] + m[6]) * r);
				qw = (NxReal)(((double)m[7] - m[5]) * r);
				}
			else if(k == 1)
				{
				const double s = sqrt(((double)m[4] - ((double)m[8] + m[0])) + 1.0);
				qy = (NxReal)(0.5 * s);
				const double r = 0.5 / s;
				qz = (NxReal)(((double)m[5] + m[7]) * r);
				qx = (NxReal)(((double)m[3] + m[1]) * r);
				qw = (NxReal)(((double)m[2] - m[6]) * r);
				}
			else
				{
				const double s = sqrt(((double)m[8] - ((double)m[4] + m[0])) + 1.0);
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
// (deferred: internal slot 4 (solver), called by phys_fn_004364)
void Joint::row004123(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
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
// (deferred: NxJoint::addLimitPlane -- not called)
bool Joint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	(void)normal;
	(void)pointInPlane;
	NX_ASSERT(0);
	return false;
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
