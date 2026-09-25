/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/PointInPlaneJoint.h"
#include "core/NpPointInPlaneJoint.h"
#include "core/JointSupport.h"
#include "core/JointLinearRecords.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>

// The oracle's __FILE__ for this unit (every assert report in it pushes the
// string at 0x10119b7c).
#define NX_POINTINPLANEJOINT_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\PointInPlaneJoint.cpp"

// Joint-families Task 3e. Floating point follows core/PointOnLineJoint.cpp:
// this translation unit is x87 in the oracle and is built /arch:IA32 here; a
// value the listing keeps on the FPU stack is a `double`, a value it stores
// (fstp dword) is an `NxReal`, and the listing's operand grouping and order
// are kept. PointInPlaneJoint is constructed by Scene::createJoint's
// point-in-plane case (NxJointType 5). No row is shared with point-on-line
// (every row here has its own address and table entry); where a row repeats
// a point-on-line row's instructions the comment says so. See
// units/joint-families-contract.md "## PointInPlane".

static NX_INLINE double pointInPlaneMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

static NX_INLINE JointBodyRecord* pointInPlaneBody(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// The actor's internal object (NxActor +0x14) and its body record (+8), read
// by offset as core/Joint.cpp does.
static NX_INLINE void* pointInPlaneActorImpl(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

static NX_INLINE void* pointInPlaneBodyOfActorImpl(void* actorImpl)
	{
	return actorImpl ? *reinterpret_cast<void**>(static_cast<NxU8*>(actorImpl) + 8) : 0;
	}

// The inlined loop 004258 and 004260 open with (0xa10eb-0xa1112,
// 0xa1664-0xa1692; the same loop as Joint.cpp's 004125/004129): refresh the
// first body whose stamp no longer matches, and only that one.
static void pointInPlaneRefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = pointInPlaneBody(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// A three-line cross at `centre` (004260, 0xa16dc-0xa17b7 and its two
// copies; point-on-line 004274's instructions): per axis the line from
// centre - scale to centre + scale along that axis, the other two components
// copied, minus end first, through addLine (the renderable's slot +0x20).
static void pointInPlaneCross(NxDebugRenderable& renderable, const NxVec3& centre, NxReal scale,
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

// The axis is scaled in place before the white line (0xa17ba-0xa17ed and
// 0xa1939-0xa196a).
static void pointInPlaneScaleAxis(NxVec3& axis, NxReal scale)
	{
	axis.x = (NxReal)((double)axis.x * scale);
	axis.y = (NxReal)((double)axis.y * scale);
	axis.z = (NxReal)((double)axis.z * scale);
	}

// phys_fn_004256 (0x000a10a0, 54 B)
// A broken joint reports (code 1, line 0x3e) and returns; otherwise the row
// tail-jumps to the base part (row 004066, 0xa10d1). NxPointInPlaneJointDesc
// has no field of its own. Point-on-line 004268 with this unit's file, line
// and message. The supplement decompile agrees with the listing.
void PointInPlaneJoint::saveToDesc(NxPointInPlaneJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_POINTINPLANEJOINT_CPP, 0x3e, 0,
			"PointInPlaneJoint::saveToDesc: Joint is broken. Broken joints can't be saved!");
		return;
		}
	saveToDescBase(desc);
	}

// phys_fn_004258 (0x000a10e0, 1391 B)
// The point-in-plane solver slot. The stale-body refresh comes first; the
// bodies' support records (body +0x204) are read after it, inside the anchor
// blocks (0xa11d3, 0xa12b6). With R0/R1 the bodies' +0x134 3x3s and t0/t1
// their +0x158 positions: n = R0 * worldAxis[0] (the plane normal; the copy
// without body 0), p0 = R0 * worldAnchor[0] + t0, r1 = R1 * worldAnchor[1]
// (stored; the copy without body 1) and p1 = r1 + t1. With d = p1 - p0 and
// s = n . d (kept), the projection of p1 on the plane through p0 is
// x = p1 - n s; body 0's lever is r0 = x - t0 (x itself without body 0). The
// lever pair's error e = jointLinearError(r0, r1) (0xa1444-0xa14b4, the same
// instructions). One kind-1 linear record follows, along n, with bias
// (e . n) / arg and +0x48 = maxForce.
// Listing over decompile: the decompile regroups the sums of n, p0 and r1 and
// every dot product, and shows p1.x, d, s, n.x s and e as floats; the listing
// keeps them on the FPU stack (named double below) and groups as written
// here. It also drops the kind tests of the record flags as unreachable
// (jointLinearRecord keeps them). The bias is an fdiv by the argument, stored
// into the argument's own slot (0xa14d9-0xa14dd), not a multiply by 1 / arg
// as point-on-line 004272 does. jointLinearRecord forms the same products,
// subtracted in the same order, as the listing's +0x18/+0x24 stores
// (0xa14e6-0xa157f); the listing stores them x, y, z where the helper stores
// y, z, x. The listing passes the argument slot that holds the bias as
// 004391's write-only first output (0xa15fd); jointSolveRecord passes a copy
// of maxForce, which is equally dead. maxForce is read before 004093
// (0xa14bc).
void PointInPlaneJoint::row_slot6(NxReal arg)
	{
	pointInPlaneRefreshFirstStaleBody(*this);
	JointBodyRecord* const body0 = pointInPlaneBody(mBody[0]);

	// n (0xa1117-0xa11c1).
	NxVec3 n;
	if(body0)
		{
		const NxReal* R = body0->mUnknown134;
		const NxVec3& a = mWorldAxis[0];
		n.x = (NxReal)((pointInPlaneMul(R[2], a.z) + pointInPlaneMul(R[1], a.y)) + pointInPlaneMul(R[0], a.x));
		n.y = (NxReal)((pointInPlaneMul(R[5], a.z) + pointInPlaneMul(R[3], a.x)) + pointInPlaneMul(R[4], a.y));
		n.z = (NxReal)((pointInPlaneMul(R[8], a.z) + pointInPlaneMul(R[6], a.x)) + pointInPlaneMul(R[7], a.y));
		}
	else
		n = mWorldAxis[0];

	// p0 and body 0's support record (0xa11c5-0xa12a3).
	JointSupportBody* record0;
	NxVec3 p0;
	if(body0)
		{
		record0 = body0->mUnknown204;
		const NxReal* R = body0->mUnknown134;
		const NxVec3& a = mWorldAnchor[0];
		const NxVec3& t0 = body0->mUnknown158;
		const double ax = (pointInPlaneMul(R[2], a.z) + pointInPlaneMul(R[1], a.y)) + pointInPlaneMul(R[0], a.x);
		const NxReal ay = (NxReal)((pointInPlaneMul(R[5], a.z) + pointInPlaneMul(R[3], a.x)) + pointInPlaneMul(R[4], a.y));
		const NxReal az = (NxReal)((pointInPlaneMul(R[8], a.z) + pointInPlaneMul(R[6], a.x)) + pointInPlaneMul(R[7], a.y));
		p0.x = (NxReal)(ax + t0.x);
		p0.y = (NxReal)((double)ay + t0.y);
		p0.z = (NxReal)((double)az + t0.z);
		}
	else
		{
		p0 = mWorldAnchor[0];
		record0 = 0;
		}

	// r1, p1 and body 1's support record (0xa12a5-0xa1390). p1.x stays on
	// the stack; p1.y and p1.z are stored.
	JointBodyRecord* const body1 = pointInPlaneBody(mBody[1]);
	JointSupportBody* record1;
	NxVec3 r1;
	double p1x;
	NxReal p1y;
	NxReal p1z;
	if(body1)
		{
		record1 = body1->mUnknown204;
		const NxReal* R = body1->mUnknown134;
		const NxVec3& a = mWorldAnchor[1];
		const NxVec3& t1 = body1->mUnknown158;
		r1.x = (NxReal)((pointInPlaneMul(R[2], a.z) + pointInPlaneMul(R[1], a.y)) + pointInPlaneMul(R[0], a.x));
		r1.y = (NxReal)((pointInPlaneMul(R[5], a.z) + pointInPlaneMul(R[4], a.y)) + pointInPlaneMul(R[3], a.x));
		r1.z = (NxReal)((pointInPlaneMul(R[8], a.z) + pointInPlaneMul(R[7], a.y)) + pointInPlaneMul(R[6], a.x));
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
		record1 = 0;
		}

	// d = p1 - p0 and s = n . d, both kept (0xa1392-0xa13c0).
	const double dx = p1x - p0.x;
	const double dy = (double)p1y - p0.y;
	const double dz = (double)p1z - p0.z;
	const double s = (dy * n.y + dz * n.z) + dx * n.x;

	// x = p1 - n s: n.x s kept, n.y s and n.z s stored (0xa13c2-0xa13f8).
	const double nsx = (double)n.x * s;
	const NxReal nsy = (NxReal)((double)n.y * s);
	const NxReal nsz = (NxReal)((double)n.z * s);
	const NxReal xx = (NxReal)(p1x - nsx);
	const NxReal xy = (NxReal)((double)p1y - nsy);
	const NxReal xz = (NxReal)((double)p1z - nsz);

	// r0 = x - t0 (0xa13fc-0xa1440).
	NxVec3 r0;
	if(body0)
		{
		const NxVec3& t0 = body0->mUnknown158;
		r0.x = (NxReal)((double)xx - t0.x);
		r0.y = (NxReal)((double)xy - t0.y);
		r0.z = (NxReal)((double)xz - t0.z);
		}
	else
		{
		r0.x = xx;
		r0.y = xy;
		r0.z = xz;
		}

	// e and the bias (0xa1444-0xa14dd).
	double gx, gy, gz;
	jointLinearError(r0, r1, body0, body1, gx, gy, gz);
	const NxReal maxForce = mMaxForce;
	const NxReal bias = (NxReal)(((gy * n.y + gz * n.z) + gx * n.x) / (double)arg);

	// The record, along n (0xa14e1-0xa164c).
	JointSupportRecord* record = row004093();
	jointLinearRecord(record, record0, record1, n, r0, r1);
	jointSolveRecord(record, this, bias, maxForce);
	}

// phys_fn_004260 (0x000a1650, 1273 B)
// Debug visualization, only when +0x2c bit 9 (NX_JF_VISUALIZATION) is set,
// after the stale-body refresh. Point-on-line 004274's instructions with two
// differences:
// - the white line along the world axis starts at the point itself: the axis
//   is scaled in place, then addLine(P, A + P, 0xffffff); no P - A is formed
//   (0xa17ba-0xa1815, 0xa1939-0xa1992). The world-axes block sums the end as
//   A + P in every component; the local-axes block sums its x as P.x + A.x
//   (0xa196e-0xa1972) and y, z as A + P;
// - body 1's world anchor Q groups its sums differently (0xa19bb-0xa1a35).
// Each part is gated by its SDK parameter being non-zero and scaled by it
// times NX_VISUALIZATION_SCALE (element 13, 0x10123b4c):
// - world axes (element 32, 0x10123b98): a three-line cross at the point
//   row004123 returns (the anchors' midpoint) in 0xff0000, 0xff00 and 0xff,
//   then the white line along row004127's axis;
// - local axes (element 31, 0x10123b94): the same four lines, then the same
//   cross at body 1's world anchor (carried through body 1's +0x134/+0x158
//   pose; the stored anchor without body 1) in 0xcf0000, 0xcf00 and 0xcf.
// Every line goes through addLine (+0x20). The two scales multiply in the
// listing's operand order (13 * 32, then 31 * 13).
// Listing over decompile: the supplement decompile loses the argument order
// and most arguments of every call.
void PointInPlaneJoint::row_slot4(NxDebugRenderable& renderable)
	{
	if(!((mFlags >> 9) & 1))
		return;

	pointInPlaneRefreshFirstStaleBody(*this);

	if(jointLinearSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES) != 0.0f)
		{
		const NxReal scale = (NxReal)((double)jointLinearSdkParameter(NX_VISUALIZATION_SCALE) *
			jointLinearSdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES));
		NxVec3 point;
		row004123(point);
		NxVec3 axis;
		row004127(axis);
		pointInPlaneCross(renderable, point, scale, 0xff0000, 0xff00, 0xff);

		// The white line (0xa17ba-0xa1815): A + P in every component.
		pointInPlaneScaleAxis(axis, scale);
		NxVec3 end;
		end.x = (NxReal)((double)axis.x + point.x);
		end.y = (NxReal)((double)axis.y + point.y);
		end.z = (NxReal)((double)axis.z + point.z);
		renderable.addLine(point, end, 0xffffff);
		}

	if(jointLinearSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) != 0.0f)
		{
		const NxReal scale = (NxReal)((double)jointLinearSdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) *
			jointLinearSdkParameter(NX_VISUALIZATION_SCALE));
		NxVec3 point;
		row004123(point);
		NxVec3 axis;
		row004127(axis);
		pointInPlaneCross(renderable, point, scale, 0xff0000, 0xff00, 0xff);

		// The white line (0xa1939-0xa1992): P.x + A.x, then A + P for y and z.
		pointInPlaneScaleAxis(axis, scale);
		NxVec3 end;
		end.x = (NxReal)((double)point.x + axis.x);
		end.y = (NxReal)((double)axis.y + point.y);
		end.z = (NxReal)((double)axis.z + point.z);
		renderable.addLine(point, end, 0xffffff);

		// Body 1's world anchor (0xa1995-0xa1a67).
		NxVec3 anchor;
		const JointBodyRecord* body1 = pointInPlaneBody(mBody[1]);
		if(!body1)
			anchor = mWorldAnchor[1];
		else
			{
			const NxReal* R = body1->mUnknown134;
			const NxVec3& a = mWorldAnchor[1];
			const NxVec3& t1 = body1->mUnknown158;
			const double ax = (pointInPlaneMul(R[1], a.y) + pointInPlaneMul(R[2], a.z)) + pointInPlaneMul(R[0], a.x);
			const NxReal ay = (NxReal)((pointInPlaneMul(R[4], a.y) + pointInPlaneMul(R[3], a.x)) + pointInPlaneMul(R[5], a.z));
			const NxReal az = (NxReal)((pointInPlaneMul(R[7], a.y) + pointInPlaneMul(R[6], a.x)) + pointInPlaneMul(R[8], a.z));
			anchor.x = (NxReal)(ax + t1.x);
			anchor.y = (NxReal)((double)ay + t1.y);
			anchor.z = (NxReal)((double)az + t1.z);
			}
		pointInPlaneCross(renderable, anchor, scale, 0xcf0000, 0xcf00, 0xcf);
		}
	}

// phys_fn_004262 (0x000a1b50, 84 B)
// Joint(desc, 2) runs first (`push 2` at 0xa1b56: the type bit); the
// compiler then stores the vptr 0x10119b48 (0xa1b60). The public object is
// allocated through the SDK allocator (`push 0; push 0x1c; call [edx+8]`)
// and constructed only when the allocation succeeded, but desc.userData is
// written to it without a null check (0xa1b85-0xa1b88, 0xa1b97-0xa1b9a): a
// failed allocation faults there in the oracle, and does here too. Nothing
// else follows. Point-on-line 004276 with this family's type bit, vptr and
// Np constructor.
PointInPlaneJoint::PointInPlaneJoint(const NxPointInPlaneJointDesc& desc)
	: Joint(desc, 2)
	{
	void* memory = nxGetSdkAllocator()->malloc(sizeof(NpPointInPlaneJoint), NX_MEMORY_PERSISTENT);
	NpPointInPlaneJoint* publicJoint = memory ? new(memory) NpPointInPlaneJoint(this) : 0;
	mPublicObject = publicJoint;
	static_cast<NxJoint*>(publicJoint)->userData = desc.userData;
	}

// phys_fn_004264 (0x000a1bb0, 56 B)
// The listing is the compiler's scalar deleting destructor around this body:
// it reinstalls the vptr 0x10119b48, deletes the public object through its
// slot 0 with 1 (`push 1; call [eax]`), calls the Joint destructor body
// (row 004095) directly, and frees `this` through the SDK allocator (slot
// +0x14) when the flag's bit 0 is set (Joint::operator delete).
PointInPlaneJoint::~PointInPlaneJoint()
	{
	if(mPublicObject)
		delete static_cast<NpPointInPlaneJoint*>(mPublicObject);
	}

// phys_fn_004266 (0x000a1bf0, 194 B)
// desc.isValid() is the descriptor's virtual (`call [edx+8]`, 0xa1c2e). The
// actors are re-bound (phys_fn_004107 with suppressAttach false) only when a
// body differs from the one held; the second body is not looked at when the
// first already differs. Then the base part (004121). Point-on-line 004280
// with this unit's file, lines (0x2a, 0x2b) and messages.
void PointInPlaneJoint::loadFromDesc(const NxPointInPlaneJointDesc& desc)
	{
	if((mFlags & 0x18) == 0x10)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_POINTINPLANEJOINT_CPP, 0x2a, 0,
			"PointInPlaneJoint::loadFromDesc: Joint is broken. Broken joints can't be manipulated!");
		return;
		}
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_POINTINPLANEJOINT_CPP, 0x2b, 0,
			"PointInPlaneJoint::loadFromDesc: desc.isValid() fails!");
		return;
		}
	void* actorImpl0 = pointInPlaneActorImpl(desc.actor[0]);
	void* actorImpl1 = pointInPlaneActorImpl(desc.actor[1]);
	if(pointInPlaneBodyOfActorImpl(actorImpl0) != mBody[0] || pointInPlaneBodyOfActorImpl(actorImpl1) != mBody[1])
		row004107(actorImpl0, actorImpl1, false);
	loadFromDescBase(desc);
	}
