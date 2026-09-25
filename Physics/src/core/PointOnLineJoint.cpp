/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/PointOnLineJoint.h"
#include "core/NpPointOnLineJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x10119ce4).
#define NX_POINTONLINEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\PointOnLineJoint.cpp"

// Joint-families Task 3d. Floating point follows core/PrismaticJoint.cpp and
// core/CylindricalJoint.cpp: this translation unit is x87 in the oracle and is
// built /arch:IA32 here; a value the listing keeps on the FPU stack is a
// `double`, a value it stores (fstp dword) is an `NxReal`, and the listing's
// operand grouping and order are kept. PointOnLineJoint is constructed by
// Scene::createJoint's point-on-line case (NxJointType 4). See
// units/joint-families-contract.md "## PointOnLine".

static NX_INLINE double pointOnLineMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

static NX_INLINE JointBodyRecord* pointOnLineBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* pointOnLineActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* pointOnLineBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// The inlined loop 004272 and 004274 open with (0xa1d4b-0xa1d72,
// 0xa2544-0xa2572; the same loop as Joint.cpp's 004125/004129): refresh the
// first body whose stamp no longer matches, and only that one.
static void pointOnLineRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = pointOnLineBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// A three-line cross at `centre` (004274, 0xa25bc-0xa2697 and its two
// copies): per axis the line from centre - scale to centre + scale along that
// axis, the other two components copied, minus end first, through addLine
// (the renderable's slot +0x20).
static void pointOnLineCross(NxDebugRenderable& renderable, const NxVec3& centre, NxReal scale,
	NxU32 colorX, NxU32 colorY, NxU32 colorZ)
	{
	NxVec3 plus;
	NxVec3 minus;

	plus.x = (NxReal)((double)centre.x + scale);
	plus.y = centre.y;
	plus.z = centre.z;
	minus.x = (NxReal)((double)centre.x - scale);
	minus.y = centre.y;
	minus.z = centre.z;
	renderable.addLine(minus, plus, colorX);

	plus.x = centre.x;
	plus.y = (NxReal)((double)centre.y + scale);
	plus.z = centre.z;
	minus.x = centre.x;
	minus.y = (NxReal)((double)centre.y - scale);
	minus.z = centre.z;
	renderable.addLine(minus, plus, colorY);

	plus.x = centre.x;
	plus.y = centre.y;
	plus.z = (NxReal)((double)centre.z + scale);
	minus.x = centre.x;
	minus.y = centre.y;
	minus.z = (NxReal)((double)centre.z - scale);
	renderable.addLine(minus, plus, colorZ);
	}

// The white line along the world axis (0xa269a-0xa2719 and 0xa283b-0xa28ba):
// the axis is scaled in place, then the line runs from point - axis to
// point + axis (each end component summed as (axis + point) or
// (point - axis), stored).
static void pointOnLineAxisLine(NxDebugRenderable& renderable, const NxVec3& point, NxVec3& axis, NxReal scale)
	{
	axis.x = (NxReal)((double)axis.x * scale);
	axis.y = (NxReal)((double)axis.y * scale);
	axis.z = (NxReal)((double)axis.z * scale);
	NxVec3 plus;
	plus.x = (NxReal)((double)axis.x + point.x);
	plus.y = (NxReal)((double)axis.y + point.y);
	plus.z = (NxReal)((double)axis.z + point.z);
	NxVec3 minus;
	minus.x = (NxReal)((double)point.x - axis.x);
	minus.y = (NxReal)((double)point.y - axis.y);
	minus.z = (NxReal)((double)point.z - axis.z);
	renderable.addLine(minus, plus, 0xffffff);
	}

// phys_fn_004268 (0x000a1cc0, 54 B)
// A broken joint reports (code 1, line 0x41) and returns; otherwise the row
// tail-jumps to the base part (row 004066, 0xa1cf1). NxPointOnLineJointDesc
// has no field of its own. The supplement decompile agrees with the listing.
void PointOnLineJoint::saveToDesc(NxPointOnLineJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_POINTONLINEJOINT_CPP, 0x41, 0,
			"PointOnLineJoint::saveToDesc: Joint is broken. Broken joints can't be saved!");
		return;
		}
	saveToDescBase(desc);
	}

// phys_fn_004270 (0x000a1d00, 5 B)
// `mov eax,[ecx]; jmp [eax+0x2c]`: a tail call through slot 11 of this
// object's own table, which is this row, so a call never returns. Nothing in
// the image calls internal slot 11 of a point-on-line joint. The virtual
// self-call below compiles to the same two instructions.
PointOnLineJoint* PointOnLineJoint::row_slot11()
	{
	return row_slot11();
	}

// phys_fn_004272 (0x000a1d10, 2073 B)
// The point-on-line solver slot. It reads the bodies' support records (body
// +0x204) before the stale-body refresh. With R0/R1 the bodies' +0x134 3x3s
// and t0/t1 their +0x158 positions: n = R0 * worldNormal[0], c = R0 *
// worldCross[0] (copies without body 0), p0 = R0 * worldAnchor[0] + t0, r1 =
// R1 * worldAnchor[1] (stored; the copy without body 1) and p1 = r1 + t1.
// With d = p1 - p0, s = n . d (stored) and t = c . d (kept), the point of the
// line through p0 along the joint axis nearest p1 is x = (p1 - n s) - c t;
// body 0's lever is r0 = x - t0 (x itself without body 0). The lever pair's
// error e = jointLinearError(r0, r1) (0xa218a-0xa21fa, the same
// instructions). Two kind-1 linear records follow, along n then along c, with
// bias (e . n) / arg and (e . c) / arg and +0x48 = maxForce.
// Listing over decompile: the decompile regroups every dot product and shows
// p1.x, d, t, x.x and e.x as floats; the listing keeps them on the FPU stack
// (named double below) and groups as written here. It also drops the kind
// tests of the record flags as unreachable (jointLinearRecord keeps them).
// jointLinearRecord forms the same products, subtracted in the same order, as
// the listing's +0x18/+0x24 stores (0xa2252-0xa22f1, 0xa23bb-0xa245b); the
// listing stores them x, y, z where the helper stores y, z, x. The first
// record passes the local that held its bias as 004391's write-only first
// output (0xa235f), the second the local that held maxForce (0xa24cc);
// jointSolveRecord passes a copy of maxForce, which is equally dead.
void PointOnLineJoint::row_slot6(NxReal arg)
	{
	JointBodyRecord* body0 = pointOnLineBody(mBody[0]);
	JointSupportBody* const record0 = body0 ? body0->mUnknown204 : 0;
	JointBodyRecord* body1 = pointOnLineBody(mBody[1]);
	JointSupportBody* const record1 = body1 ? body1->mUnknown204 : 0;
	pointOnLineRefreshFirstStaleBody(*this);
	body0 = pointOnLineBody(mBody[0]);

	// n, c and p0 (0xa1d77-0xa1f9b).
	NxVec3 n;
	NxVec3 c;
	NxVec3 p0;
	if(body0)
		{
		const NxReal* R = body0->mUnknown134;
		const NxVec3& wn = mWorldNormal[0];
		n.x = (NxReal)((pointOnLineMul(R[2], wn.z) + pointOnLineMul(R[1], wn.y)) + pointOnLineMul(wn.x, R[0]));
		n.y = (NxReal)((pointOnLineMul(R[3], wn.x) + pointOnLineMul(R[5], wn.z)) + pointOnLineMul(R[4], wn.y));
		n.z = (NxReal)((pointOnLineMul(R[6], wn.x) + pointOnLineMul(R[8], wn.z)) + pointOnLineMul(R[7], wn.y));
		const NxVec3& wc = mWorldCross[0];
		c.x = (NxReal)((pointOnLineMul(R[2], wc.z) + pointOnLineMul(R[1], wc.y)) + pointOnLineMul(R[0], wc.x));
		c.y = (NxReal)((pointOnLineMul(R[5], wc.z) + pointOnLineMul(R[3], wc.x)) + pointOnLineMul(R[4], wc.y));
		c.z = (NxReal)((pointOnLineMul(R[8], wc.z) + pointOnLineMul(R[6], wc.x)) + pointOnLineMul(R[7], wc.y));

		const NxVec3& a = mWorldAnchor[0];
		const NxVec3& t0 = body0->mUnknown158;
		const double ax = (pointOnLineMul(R[2], a.z) + pointOnLineMul(R[1], a.y)) + pointOnLineMul(a.x, R[0]);
		const NxReal ay = (NxReal)((pointOnLineMul(R[3], a.x) + pointOnLineMul(R[5], a.z)) + pointOnLineMul(R[4], a.y));
		const NxReal az = (NxReal)((pointOnLineMul(R[6], a.x) + pointOnLineMul(R[8], a.z)) + pointOnLineMul(R[7], a.y));
		p0.x = (NxReal)(ax + t0.x);
		p0.y = (NxReal)((double)ay + t0.y);
		p0.z = (NxReal)((double)az + t0.z);
		}
	else
		{
		n = mWorldNormal[0];
		c = mWorldCross[0];
		p0 = mWorldAnchor[0];
		}

	// r1 and p1 (0xa1f9f-0xa2080). p1.x stays on the stack; p1.y and p1.z are
	// stored.
	body1 = pointOnLineBody(mBody[1]);
	NxVec3 r1;
	double p1x;
	NxReal p1y;
	NxReal p1z;
	if(body1)
		{
		const NxReal* R = body1->mUnknown134;
		const NxVec3& a = mWorldAnchor[1];
		const NxVec3& t1 = body1->mUnknown158;
		r1.x = (NxReal)((pointOnLineMul(R[1], a.y) + pointOnLineMul(R[2], a.z)) + pointOnLineMul(R[0], a.x));
		r1.y = (NxReal)((pointOnLineMul(R[4], a.y) + pointOnLineMul(R[3], a.x)) + pointOnLineMul(R[5], a.z));
		r1.z = (NxReal)((pointOnLineMul(R[7], a.y) + pointOnLineMul(R[6], a.x)) + pointOnLineMul(R[8], a.z));
		p1x = (double)r1.x + t1.x;
		p1y = (NxReal)((double)r1.y + t1.y);
		p1z = (NxReal)((double)r1.z + t1.z);
		}
	else
		{
		r1 = mWorldAnchor[1];
		p1x = r1.x;
		p1y = r1.y;
		p1z = r1.z;
		}

	// d = p1 - p0, s = n . d (stored), t = c . d (kept) (0xa2084-0xa20ca).
	const double dx = p1x - p0.x;
	const double dy = (double)p1y - p0.y;
	const double dz = (double)p1z - p0.z;
	const NxReal s = (NxReal)(((double)n.z * dz + dx * n.x) + dy * n.y);
	const double t = (dx * c.x + dz * c.z) + dy * c.y;

	// c t (stored), then x = (p1 - n s) - c t; r0 = x - t0 (0xa20cc-0xa2194).
	const NxReal ctx = (NxReal)((double)c.x * t);
	const NxReal cty = (NxReal)((double)c.y * t);
	const NxReal ctz = (NxReal)((double)c.z * t);
	const double nsx = pointOnLineMul(n.x, s);
	const NxReal nsy = (NxReal)pointOnLineMul(n.y, s);
	const NxReal nsz = (NxReal)pointOnLineMul(n.z, s);
	const NxReal ux = (NxReal)(p1x - nsx);
	const NxReal uy = (NxReal)((double)p1y - nsy);
	const NxReal uz = (NxReal)((double)p1z - nsz);
	const double xx = (double)ux - ctx;
	const NxReal xy = (NxReal)((double)uy - cty);
	const NxReal xz = (NxReal)((double)uz - ctz);
	NxVec3 r0;
	if(body0)
		{
		const NxVec3& t0 = body0->mUnknown158;
		r0.x = (NxReal)(xx - t0.x);
		r0.y = (NxReal)((double)xy - t0.y);
		r0.z = (NxReal)((double)xz - t0.z);
		}
	else
		{
		r0.x = (NxReal)xx;
		r0.y = xy;
		r0.z = xz;
		}

	// e and the two biases (0xa218a-0xa224b); 1 / arg stays on the stack.
	double gx, gy, gz;
	jointLinearError(r0, r1, body0, body1, gx, gy, gz);
	const NxReal maxForce = mMaxForce;
	const double inverse = 1.0f / (double)arg;
	const NxReal bias1 = (NxReal)((((gz * n.z) + gx * n.x) + gy * n.y) * inverse);
	const NxReal bias2 = (NxReal)((((gx * c.x) + gz * c.z) + gy * c.y) * inverse);

	// Record 1, along n (0xa224d-0xa23ab).
	JointSupportRecord* record = row004093();
	jointLinearRecord(record, record0, record1, n, r0, r1);
	jointSolveRecord(record, this, bias1, maxForce);

	// Record 2, along c (0xa23ad-0xa251e); maxForce is read again (0xa23ad).
	record = row004093();
	jointLinearRecord(record, record0, record1, c, r0, r1);
	jointSolveRecord(record, this, bias2, mMaxForce);
	}

// phys_fn_004274 (0x000a2530, 1345 B)
// Debug visualization, only when +0x2c bit 9 (NX_JF_VISUALIZATION) is set,
// after the stale-body refresh. Each part is gated by its SDK parameter being
// non-zero and scaled by it times NX_VISUALIZATION_SCALE (element 13,
// 0x10123b4c):
// - world axes (element 32, 0x10123b98): a three-line cross at the point
//   row004123 returns (the anchors' midpoint) in 0xff0000, 0xff00 and 0xff,
//   then the line along row004127's axis through it, scaled, in 0xffffff;
// - local axes (element 31, 0x10123b94): the same four lines, then the same
//   cross at body 1's world anchor (carried through body 1's +0x134/+0x158
//   pose; the stored anchor without body 1) in 0xcf0000, 0xcf00 and 0xcf.
// Every line goes through addLine (+0x20), minus end first. The two scales
// multiply in the listing's operand order (13 * 32, then 31 * 13).
// Listing over decompile: the decompile loses the argument order and most
// arguments of every call.
void PointOnLineJoint::row_slot4(NxDebugRenderable& renderable)
	{
	if(!((mFlags >> 9) & 1))
		return;

	pointOnLineRefreshFirstStaleBody(*this);

	if(jointLinearSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES) != 0.0f)
		{
		const NxReal scale = (NxReal)((double)jointLinearSdkParameter(NX_VISUALIZATION_SCALE) *
			jointLinearSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES));
		NxVec3 point;
		row004123(point);
		NxVec3 axis;
		row004127(axis);
		pointOnLineCross(renderable, point, scale, 0xff0000, 0xff00, 0xff);
		pointOnLineAxisLine(renderable, point, axis, scale);
		}

	if(jointLinearSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) != 0.0f)
		{
		const NxReal scale = (NxReal)((double)jointLinearSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) *
			jointLinearSdkParameter(NX_VISUALIZATION_SCALE));
		NxVec3 point;
		row004123(point);
		NxVec3 axis;
		row004127(axis);
		pointOnLineCross(renderable, point, scale, 0xff0000, 0xff00, 0xff);
		pointOnLineAxisLine(renderable, point, axis, scale);

		// Body 1's world anchor (0xa28bd-0xa298f).
		NxVec3 anchor;
		const JointBodyRecord* body1 = pointOnLineBody(mBody[1]);
		if(!body1)
			anchor = mWorldAnchor[1];
		else
			{
			const NxReal* R = body1->mUnknown134;
			const NxVec3& a = mWorldAnchor[1];
			const NxVec3& t1 = body1->mUnknown158;
			const double ax = (pointOnLineMul(R[1], a.y) + pointOnLineMul(R[2], a.z)) + pointOnLineMul(a.x, R[0]);
			const NxReal ay = (NxReal)((pointOnLineMul(R[4], a.y) + pointOnLineMul(R[5], a.z)) + pointOnLineMul(R[3], a.x));
			const NxReal az = (NxReal)((pointOnLineMul(R[7], a.y) + pointOnLineMul(R[8], a.z)) + pointOnLineMul(R[6], a.x));
			anchor.x = (NxReal)(ax + t1.x);
			anchor.y = (NxReal)((double)ay + t1.y);
			anchor.z = (NxReal)((double)az + t1.z);
			}
		pointOnLineCross(renderable, anchor, scale, 0xcf0000, 0xcf00, 0xcf);
		}
	}

// phys_fn_004276 (0x000a2a80, 84 B)
// Joint(desc, 4) runs first; the compiler then stores the vptr 0x10119cb0
// (0xa2a90). The public object is allocated through the Foundation allocator (`push
// 0; push 0x1c; call [edx+8]`) and constructed only when the allocation
// succeeded, but desc.userData is written to it without a null check
// (0xa2ab5-0xa2ab8, 0xa2ac7-0xa2aca): a failed allocation faults there in the
// oracle, and does here too. Nothing else follows.
PointOnLineJoint::PointOnLineJoint(const NxPointOnLineJointDesc& desc)
	: Joint(desc, 4)
	{
	void* memory = nxFoundationSDKAllocator->malloc(sizeof(NpPointOnLineJoint), NX_MEMORY_PERSISTENT);
	NpPointOnLineJoint* publicJoint = memory ? new(memory) NpPointOnLineJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	}

// phys_fn_004278 (0x000a2ae0, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x10119cb0, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`), calls the Joint destructor body
// (row 004095) directly, and frees `this` through the Foundation allocator (slot
// +0x14) when the flag's bit 0 is set (Joint::operator delete).
PointOnLineJoint::~PointOnLineJoint()
	{
	if(mPublicObject)
		delete static_cast<NpPointOnLineJoint*>(mPublicObject);
	}

// phys_fn_004280 (0x000a2b20, 194 B)
// desc.isValid() is the descriptor's virtual (`call [edx+8]`, 0xa2b5e). The
// actors are re-bound (phys_fn_004107 with suppressAttach false) only when a
// body differs from the one held; the second body is not looked at when the
// first already differs. Then the base part (004121). The prismatic row
// 004384 without its 004378 call.
void PointOnLineJoint::loadFromDesc(const NxPointOnLineJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_POINTONLINEJOINT_CPP, 0x2d, 0,
			"PointOnLineJoint::loadFromDesc: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_POINTONLINEJOINT_CPP, 0x2e, 0,
			"PointOnLineJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	void* actorImpl0 = pointOnLineActorImpl(desc.actor[0]);
	void* actorImpl1 = pointOnLineActorImpl(desc.actor[1]);
	if(pointOnLineBodyOfActorImpl(actorImpl0) != mBody[0] || pointOnLineBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	}
