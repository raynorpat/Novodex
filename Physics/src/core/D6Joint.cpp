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
#include "core/JointLinearRecords.h"
#include "core/JointAcos.h"
#include "core/JointX87.h"
#include "PhysicsSDK.h"
#include "NxJoint.h"
#include "NxDebugRenderable.h"

#include <new>
#include <cstdio>
#include <cmath>
#include <cfloat>

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

static NX_INLINE double d6Mul(double a, double b)
	{
	return a * b;
	}

static NX_INLINE JointBodyRecord* d6Body(void* body)
	{
	return static_cast<JointBodyRecord*>(body);
	}

// SDK parameters, read through PhysicsSDK::getParameter as the other joint
// units do (NX_VISUALIZATION_SCALE is .data 0x10123b4c, the joint axes
// 0x10123b94/0x10123b98, NX_PENALTY_FORCE 0x10123b18): 0 with no SDK.
static NX_INLINE NxReal d6SdkParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// The solver dump's state in .data: the stream (0x10127194, opened once by
// 004206 and never closed), the record count (0x10127190, reset by 004206 at
// every call) and the records (0x10127198, 0x12c4 bytes up to the next
// anchor: 100 records). 004194/004196 append without a bound check, as the
// oracle does.
static FILE* gD6DumpStream = 0;
static int gD6DumpCount = 0;
static D6JointDumpRecord gD6DumpRecords[100];

static void d6PrintPose(FILE* stream, const char* name, const D6JointPose* pose);
static void d6Dump(const D6JointPose* pose0, const D6JointPose* pose1, const D6JointPose* pose2,
	const NxReal* jwq, NxReal fps);
static void d6QuaternionRateMatrix(NxReal* out, const NxReal* a, const NxReal* b);

// phys_fn_004178 (0x0009b270, 396 B)
// out = this * other, with q = this->q = (x, y, z, w) and P = other.p:
// - t = q * (P, 0): w ((-(x Px) - Py y) - Pz z) and x ((Pz y + w Px) - Py z)
//   stay on the FPU stack, y ((Py w + z Px) - x Pz) and z ((Pz w + Py x) -
//   y Px) are stored (0x9b288-0x9b2e3);
// - r = t * conj(q), with the negated components held on the stack:
//   x ((tx w + ty (-z)) + tw (-x)) - tz (-y), y ((ty w + tw (-y)) +
//   tz (-x)) - tx (-z), z ((tz w + tw (-z)) + tx (-y)) - ty (-x), each stored
//   (0x9b2e7-0x9b33a), then out.p = r + this->p;
// - out.q = q * other.q (Q): x ((Qw x + Qx w) + Qz y) - Qy z, y ((Qy w +
//   Qx z) + Qw y) - Qz x, z ((Qy x + Qz w) + Qw z) - Qx y, w (((Qw w -
//   Qx x) - Qy y) - Qz z) (0x9b362-0x9b3d6).
// Every operand is read before `out` is written (the callers never alias).
void D6JointPose::row004178(D6JointPose& out, const D6JointPose& other) const
	{
	const double nx = -q[0];
	const double ny = -q[1];
	const double nz = -q[2];
	const double w = q[3];
	const NxVec3& P = other.p;
	const double tw = ((-d6Mul(q[0], P.x)) - d6Mul(P.y, q[1])) - d6Mul(P.z, q[2]);
	const double tx = (d6Mul(P.z, q[1]) + d6Mul(q[3], P.x)) - d6Mul(P.y, q[2]);
	const NxReal ty = (NxReal)((d6Mul(P.y, q[3]) + d6Mul(q[2], P.x)) - d6Mul(q[0], P.z));
	const NxReal tz = (NxReal)((d6Mul(P.z, q[3]) + d6Mul(P.y, q[0])) - d6Mul(q[1], P.x));
	const NxReal rx = (NxReal)(((tx * w + d6Mul(ty, nz)) + tw * nx) - d6Mul(tz, ny));
	const NxReal ry = (NxReal)(((d6Mul(ty, w) + tw * ny) + d6Mul(tz, nx)) - tx * nz);
	const NxReal rz = (NxReal)(((d6Mul(tz, w) + tw * nz) + tx * ny) - d6Mul(ty, nx));
	const double px = (double)rx + p.x;
	const double py = (double)ry + p.y;
	const NxReal pz = (NxReal)((double)rz + p.z);
	const NxReal* Q = other.q;
	const double qx = ((d6Mul(Q[3], q[0]) + d6Mul(Q[0], q[3])) + d6Mul(Q[2], q[1])) - d6Mul(Q[1], q[2]);
	const double qy = ((d6Mul(Q[1], q[3]) + d6Mul(Q[0], q[2])) + d6Mul(Q[3], q[1])) - d6Mul(Q[2], q[0]);
	const double qz = ((d6Mul(Q[1], q[0]) + d6Mul(Q[2], q[3])) + d6Mul(Q[3], q[2])) - d6Mul(Q[0], q[1]);
	const NxReal qw = (NxReal)(((d6Mul(Q[3], q[3]) - d6Mul(Q[0], q[0])) - d6Mul(Q[1], q[1])) - d6Mul(Q[2], q[2]));
	out.q[3] = qw;
	out.q[0] = (NxReal)qx;
	out.q[1] = (NxReal)qy;
	out.q[2] = (NxReal)qz;
	out.p.z = pz;
	out.p.x = (NxReal)px;
	out.p.y = (NxReal)py;
	}

// phys_fn_004180 (0x0009b400, 330 B)
// out = the inverse pose: the conjugate quaternion (x, y and z negated, w
// copied) and the position conj(q) * (-p) * q.
// - -p is stored (0x9b403-0x9b421); -x is stored and kept, -y kept twice,
//   -z stored and kept.
// - t = conj(q) * (-p, 0): w ((-(nx npx) - ny npy) - npz nz) and x ((npz ny
//   + w npx) - nz npy) stored, y ((nz npx + w npy) - npz nx) and z ((npz w +
//   nx npy) - ny npx) kept (0x9b43c-0x9b4a6).
// - r = t * q: x ((ty z + tx w) + tw x) - tz y, y ((ty w + tw y) + tz x) -
//   tx z, z ((tx y + tz w) + tw z) - ty x, each stored (0x9b4a8-0x9b51c).
// Returns `out` in eax (004206 uses it as the next call's `this`, 0x9d02a).
D6JointPose* D6JointPose::row004180(D6JointPose& out) const
	{
	const NxReal npx = -p.x;
	const NxReal npy = -p.y;
	const NxReal npz = -p.z;
	const NxReal w = q[3];
	const NxReal nx = -q[0];
	const double ny = -q[1];
	const NxReal nz = -q[2];
	const NxReal tw = (NxReal)(((-d6Mul(nx, npx)) - ny * npy) - d6Mul(npz, nz));
	const NxReal tx = (NxReal)((npz * ny + d6Mul(w, npx)) - d6Mul(nz, npy));
	const double ty = (d6Mul(nz, npx) + d6Mul(w, npy)) - d6Mul(npz, nx);
	const double tz = (d6Mul(npz, w) + d6Mul(nx, npy)) - ny * npx;
	const NxReal rx = (NxReal)(((ty * q[2] + d6Mul(tx, q[3])) + d6Mul(tw, q[0])) - tz * q[1]);
	const NxReal ry = (NxReal)(((ty * q[3] + d6Mul(tw, q[1])) + tz * q[0]) - d6Mul(tx, q[2]));
	const NxReal rz = (NxReal)(((d6Mul(tx, q[1]) + tz * q[3]) + d6Mul(tw, q[2])) - ty * q[0]);
	out.q[3] = w;
	out.q[0] = nx;
	out.q[1] = (NxReal)ny;
	out.q[2] = nz;
	out.p.x = rx;
	out.p.y = ry;
	out.p.z = rz;
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
// fputs("angular\n" or "linear\n") by the +0x2c byte; for a linear record
// the two levers ("ra: %f, %f, %f\n", "rb: %f, %f, %f\n"); then "normal: %f,
// %f, %f\n" and "maxForce: %f,  bias:  %f\n" (maxForce first). Each float is
// widened to a double for fprintf (`fld dword; fstp qword`).
void D6JointDumpRecord::print(FILE* stream) const
	{
	fputs(angular ? "angular\n" : "linear\n", stream);
	if(!angular)
		{
		fprintf(stream, "ra: %f, %f, %f\n", (double)ra.x, (double)ra.y, (double)ra.z);
		fprintf(stream, "rb: %f, %f, %f\n", (double)rb.x, (double)rb.y, (double)rb.z);
		}
	fprintf(stream, "normal: %f, %f, %f\n", (double)normal.x, (double)normal.y, (double)normal.z);
	fprintf(stream, "maxForce: %f,  bias:  %f\n", (double)maxForce, (double)bias);
	}

// phys_fn_004190 (0x0009b6a0, 98 B)
// Two fprintf calls: the position, then the quaternion x, y, z, w. The
// manifest decompile drops the first call's arguments; the listing pushes
// the name and the three position floats (0x9b6aa-0x9b6c6).
static void d6PrintPose(FILE* stream, const char* name, const D6JointPose* pose)
	{
	fprintf(stream, "%s position    (XYZ) : %f, %f, %f\n", name, (double)pose->p.x, (double)pose->p.y,
		(double)pose->p.z);
	fprintf(stream, "%s orientation (XYZW): %f, %f, %f, %f\n", name, (double)pose->q[0], (double)pose->q[1],
		(double)pose->q[2], (double)pose->q[3]);
	}

// phys_fn_004192 (0x0009b710, 328 B)
// The per-step dump 004206 ends with (0x9d4b6), to the stream it opened:
// "fps: %f\n" with the step divisor's inverse, the three poses named "A",
// "B" and "rel", "Angle: %f\n" with twice the acos of rel's w (the inlined
// NxMath::acos clamp: >= 1 -> 0, <= -1 -> the unit's pi 0x10119560,
// otherwise _CIacos, 0x9b773-0x9b7be; the doubling is `fadd st(0),st(0)` on
// the unrounded angle), the four rows of the 4x3 matrix ("JwQ row %d: %f,
// %f, %f\n"), every dumped record (004188), then the separator line with
// no newline.
static void d6Dump(const D6JointPose* pose0, const D6JointPose* pose1, const D6JointPose* pose2,
	const NxReal* jwq, NxReal fps)
	{
	fprintf(gD6DumpStream, "fps: %f\n", (double)fps);
	d6PrintPose(gD6DumpStream, "A", pose0);
	d6PrintPose(gD6DumpStream, "B", pose1);
	d6PrintPose(gD6DumpStream, "rel", pose2);
	const double angle = jointAcos(pose2->q[3]);
	fprintf(gD6DumpStream, "Angle: %f\n", angle + angle);
	for(int row = 0; row < 4; row++)
		{
		const NxReal* r = jwq + 3 * row;
		fprintf(gD6DumpStream, "JwQ row %d: %f, %f, %f\n", row, (double)r[0], (double)r[1], (double)r[2]);
		}
	for(int i = 0; i < gD6DumpCount; i++)
		gD6DumpRecords[i].print(gD6DumpStream);
	fputs("--------------------------------------------", gD6DumpStream);
	}

// phys_fn_004194 (0x0009b860, 514 B)
// Appends a dump record (the levers, the normal, bias, maxForce, linear;
// 0x9b869-0x9b8ce), then takes a constraint record (004093) and fills it:
// the support records of body 0 and body 1 at +0x10/+0x14, the normal at
// +0x00, ra x n at +0x18 and rb x n at +0x24 (the same products and
// subtraction order as jointLinearRecord, stored y, z, x -- here stored
// x, y, z, which does not change a value), kind 1 when `limit` is 0 and
// kind 0 otherwise (`sete`, 0x9b982), bit 9 = (kind is 0 or 2), bit 10 =
// (kind is 3, 2 or 5), bits 5-8 and 11-18 cleared; then the solve tail with
// +0x34 = bias and +0x48 = maxForce (jointSolveRecord: the listing passes the
// bias argument's slot as 004391's dead first output).
void D6Joint::row004194(NxU32 limit, const NxVec3& ra, const NxVec3& rb, const NxVec3& normal, NxReal bias,
	NxReal maxForce)
	{
	D6JointDumpRecord& dump = gD6DumpRecords[gD6DumpCount++];
	dump.ra = ra;
	dump.rb = rb;
	dump.normal = normal;
	dump.bias = bias;
	dump.maxForce = maxForce;
	dump.angular = false;
	JointBodyRecord* const body1 = d6Body(mBody[1]);
	JointSupportBody* const support1 = body1 ? body1->mUnknown204 : 0;
	JointBodyRecord* const body0 = d6Body(mBody[0]);
	JointSupportBody* const support0 = body0 ? body0->mUnknown204 : 0;
	JointSupportRecord* record = row004093();
	record->mBody[0] = support0;
	record->mBody[1] = support1;
	record->mUnknown000 = normal;
	record->mUnknown018.x = (NxReal)(d6Mul(normal.z, ra.y) - d6Mul(ra.z, normal.y));
	record->mUnknown018.y = (NxReal)(d6Mul(ra.z, normal.x) - d6Mul(normal.z, ra.x));
	record->mUnknown018.z = (NxReal)(d6Mul(ra.x, normal.y) - d6Mul(ra.y, normal.x));
	record->mUnknown024.x = (NxReal)(d6Mul(normal.z, rb.y) - d6Mul(normal.y, rb.z));
	record->mUnknown024.y = (NxReal)(d6Mul(normal.x, rb.z) - d6Mul(normal.z, rb.x));
	record->mUnknown024.z = (NxReal)(d6Mul(rb.x, normal.y) - d6Mul(normal.x, rb.y));
	NxU32 flags = record->mFlags;
	flags = flags ^ (((limit == 0 ? 1u : 0u) ^ flags) & 0x1f);
	record->mFlags = flags;
	NxU32 kind = flags & 0x1f;
	const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
	flags = (((bit9 << 9) ^ flags) & 0x200) ^ flags;
	record->mFlags = flags;
	kind = flags & 0x1f;
	const NxU32 bit10 = (kind == 3 || kind == 2 || kind == 5) ? 1 : 0;
	record->mFlags = ((bit10 & 1) << 10) | (flags & 0xfff8021f);
	jointSolveRecord(record, this, bias, maxForce);
	}

// phys_fn_004196 (0x0009ba70, 349 B)
// Appends an angular dump record (zero levers, the axis, bias, maxForce;
// 0x9ba74-0x9bac5), then takes a constraint record (004093): the support
// records of body 0 and body 1, the axis at +0x00, kind 3 when `limit` is 0
// and kind 2 otherwise (`neg; sbb; add 3`, 0x9bb15-0x9bb23), bit 9 = (kind
// is 0 or 2), bit 10 set, bits 5-8 and 11-18 cleared (+0x18/+0x24 are not
// written); then the solve tail with +0x34 = bias and +0x48 = maxForce.
void D6Joint::row004196(NxU32 limit, const NxVec3& axis, NxReal bias, NxReal maxForce)
	{
	D6JointDumpRecord& dump = gD6DumpRecords[gD6DumpCount++];
	dump.ra.x = 0.0f;
	dump.ra.y = 0.0f;
	dump.ra.z = 0.0f;
	dump.rb.x = 0.0f;
	dump.rb.y = 0.0f;
	dump.rb.z = 0.0f;
	dump.normal = axis;
	dump.bias = bias;
	dump.maxForce = maxForce;
	dump.angular = true;
	JointBodyRecord* const body1 = d6Body(mBody[1]);
	JointSupportBody* const support1 = body1 ? body1->mUnknown204 : 0;
	JointBodyRecord* const body0 = d6Body(mBody[0]);
	JointSupportBody* const support0 = body0 ? body0->mUnknown204 : 0;
	JointSupportRecord* record = row004093();
	record->mBody[0] = support0;
	record->mBody[1] = support1;
	record->mUnknown000 = axis;
	NxU32 flags = record->mFlags;
	flags = flags ^ (((limit != 0 ? 2u : 3u) ^ flags) & 0x1f);
	record->mFlags = flags;
	const NxU32 kind = flags & 0x1f;
	const NxU32 bit9 = (kind == 0 || kind == 2) ? 1 : 0;
	record->mFlags = (((bit9 & 1) | 2) << 9) | (flags & 0xfff8041f);
	jointSolveRecord(record, this, bias, maxForce);
	}

// phys_fn_004198 (0x0009bbd0, 1164 B)
// The 4x3 matrix 004206 dumps as "JwQ" (a quaternion-rate matrix), from the
// quaternions a (A0..A3 = x, y, z, w) and b (h = b * 0.5f, each stored).
// Every product the listing stores is rounded where it is stored and read
// back as a float; the ones it keeps are double below (0x9bbd0-0x9c055):
// - D = h3 A3 - ((h0 A0 + h1 A1) + h2 A2), stored, is added last to the
//   diagonal words 0, 4 and 8;
// - row 0: ((float(h0 A0) + h0 A0) - A3 * 0) - h3 * 0,
//   (float(A1 h0) + float(h1 A0)) - (-h2) A3, then - float((-A2) h3);
//   ((float(A2 h0) + float(h2 A0)) - float(h1 A3)) - float(A1 h3);
// - row 1: ((float(h1 A0) + float(A1 h0)) - h2 A3) - float(A2 h3);
//   ((float(h1 A1) + h1 A1) - A3 * 0) - float(h3 * 0);
//   ((float(h1 A2) + float(h2 A1)) - float((-h0) A3)) - float((-A0) h3);
// - row 2: ((float(h2 A0) + float(A2 h0)) - float((-h1) A3)) -
//   float((-A1) h3); ((float(h2 A1) + float(h1 A2)) - h0 A3) - float(A0 h3);
//   ((float(h2 A2) + float(h2 A2)) - float(A3 * 0)) - float(h3 * 0);
// - row 3: (A0 h3 - float(h0 A3)) + float(float(h1 A2) - float(h2 A1)),
//   (float(A1 h3) - float(h1 A3)) + float(float(h2 A0) - float(A2 h0)),
//   (float(A2 h3) - float(h2 A3)) + float(float(A1 h0) - float(h1 A0)).
// Every sum the listing stores is a float, as marked by the groupings.
static void d6QuaternionRateMatrix(NxReal* out, const NxReal* a, const NxReal* b)
	{
	const NxReal A0 = a[0];
	const NxReal A2 = a[2];
	const NxReal A1 = a[1];
	const NxReal A3 = a[3];
	const NxReal h0 = (NxReal)((double)b[0] * 0.5f);
	const NxReal h1 = (NxReal)((double)b[1] * 0.5f);
	const NxReal h2 = (NxReal)((double)b[2] * 0.5f);
	const NxReal h3 = (NxReal)((double)b[3] * 0.5f);
	const NxReal nh2 = -h2;
	const NxReal nh0 = -h0;
	const NxReal nh1 = -h1;
	const NxReal nA2 = -A2;
	const NxReal nA0 = -A0;
	const NxReal nA1 = -A1;

	const double P1 = d6Mul(h2, A2);
	const double P2 = d6Mul(h1, A1);
	const double P3 = d6Mul(h0, A0);
	const double P4 = d6Mul(h3, A3);
	const NxReal D = (NxReal)(P4 - ((P3 + P2) + P1));
	const double Z1 = d6Mul(h3, 0.0f);
	const NxReal Z1f = (NxReal)Z1;
	const NxReal nA2h3 = (NxReal)d6Mul(nA2, h3);
	const NxReal A1h3 = (NxReal)d6Mul(A1, h3);
	const double Z2 = d6Mul(A3, 0.0f);
	const NxReal Z2f = (NxReal)Z2;
	const double nh2A3 = d6Mul(nh2, A3);
	const NxReal h1A3 = (NxReal)d6Mul(h1, A3);
	const NxReal h1A0 = (NxReal)d6Mul(h1, A0);
	const NxReal h2A0 = (NxReal)d6Mul(h2, A0);
	const NxReal P3f = (NxReal)P3;
	const NxReal A1h0 = (NxReal)d6Mul(A1, h0);
	const NxReal A2h0 = (NxReal)d6Mul(A2, h0);

	// Row 0 (0x9bd6b-0x9bde2).
	const NxReal r0a = (NxReal)((double)P3f + P3);
	const double s1 = (double)A1h0 + h1A0;
	const NxReal r0c = (NxReal)((double)A2h0 + h2A0);
	const NxReal r0x = (NxReal)((double)r0a - Z2);
	const NxReal r0y = (NxReal)(s1 - nh2A3);
	const NxReal r0z = (NxReal)((double)r0c - h1A3);
	out[0] = (NxReal)((double)r0x - Z1);
	out[1] = (NxReal)((double)r0y - nA2h3);
	out[2] = (NxReal)((double)r0z - A1h3);
	out[0] = (NxReal)((double)D + out[0]);

	// Row 1 (0x9bde4-0x9bee8).
	const NxReal A2h3 = (NxReal)d6Mul(A2, h3);
	const NxReal nA0h3 = (NxReal)d6Mul(nA0, h3);
	const double h2A3 = d6Mul(h2, A3);
	const NxReal h2A3f = (NxReal)h2A3;
	const NxReal nh0A3 = (NxReal)d6Mul(nh0, A3);
	const NxReal h2A1 = (NxReal)d6Mul(h2, A1);
	const NxReal P2f = (NxReal)P2;
	const NxReal h1A2 = (NxReal)d6Mul(h1, A2);
	const NxReal r1a = (NxReal)((double)h1A0 + A1h0);
	const NxReal r1b = (NxReal)((double)P2f + P2);
	const NxReal r1c = (NxReal)((double)h1A2 + h2A1);
	const double y0 = (double)r1a - h2A3;
	const NxReal r1y = (NxReal)((double)r1b - Z2f);
	const NxReal r1z = (NxReal)((double)r1c - nh0A3);
	out[5] = (NxReal)((double)r1z - nA0h3);
	out[3] = (NxReal)(y0 - A2h3);
	out[4] = (NxReal)((double)r1y - Z1f);
	out[4] = (NxReal)((double)D + out[4]);

	// Row 2 (0x9beeb-0x9bfbf).
	const NxReal nA1h3 = (NxReal)d6Mul(nA1, h3);
	const double A0h3 = d6Mul(A0, h3);
	const NxReal A0h3f = (NxReal)A0h3;
	const NxReal nh1A3 = (NxReal)d6Mul(nh1, A3);
	const double R = d6Mul(h0, A3);
	const NxReal P1f = (NxReal)P1;
	const NxReal r2a = (NxReal)((double)h2A0 + A2h0);
	const NxReal r2b = (NxReal)((double)h2A1 + h1A2);
	const double K = (double)P1f + P1f;
	const NxReal r2x = (NxReal)((double)r2a - nh1A3);
	const NxReal r2y = (NxReal)((double)r2b - R);
	const double K1 = K - Z2f;
	out[8] = (NxReal)(K1 - Z1f);
	out[6] = (NxReal)((double)r2x - nA1h3);
	out[7] = (NxReal)((double)r2y - A0h3f);
	out[8] = (NxReal)((double)D + out[8]);

	// Row 3 (0x9bfc9-0x9c052).
	const NxReal d0 = (NxReal)((double)h1A2 - h2A1);
	const NxReal d1 = (NxReal)((double)h2A0 - A2h0);
	const NxReal d2 = (NxReal)((double)A1h0 - h1A0);
	const NxReal Rf = (NxReal)R;
	const NxReal e0 = (NxReal)(A0h3 - Rf);
	const NxReal e1 = (NxReal)((double)A1h3 - h1A3);
	const double e2 = (double)A2h3 - h2A3f;
	out[11] = (NxReal)(e2 + d2);
	out[9] = (NxReal)((double)e0 + d0);
	out[10] = (NxReal)((double)e1 + d1);
	}

// A body's 3x3 (+0x134) applied to a vector, one sum of three products;
// the argument order is the listing's (each site groups them differently).
static NX_INLINE double d6Sum3(NxReal a0, NxReal b0, NxReal a1, NxReal b1, NxReal a2, NxReal b2)
	{
	return (d6Mul(a0, b0) + d6Mul(a1, b1)) + d6Mul(a2, b2);
	}

// The stale-body refresh the visualization and solver slots open with
// (0x9c075-0x9c0a2, 0x9cbca-0x9cc02): the first body whose stamp (+0x198)
// differs from the cached one is refreshed (row 004097) and the loop stops.
static void d6RefreshFirstStaleBody(Joint& joint)
	{
	for(NxU32 i = 0; i < 2; i++)
		{
		const JointBodyRecord* body = d6Body(joint.mBody[i]);
		if(body && body->mStamp != joint.mBodyStamp[i])
			{
			joint.refreshBodyFrame(i);
			return;
			}
		}
	}

// phys_fn_004200 (0x0009c060, 2098 B)
// Debug visualization, only when +0x2c bit 9 (NX_JF_VISUALIZATION) is set,
// after the stale-body refresh. Each part is gated by its SDK parameter
// being non-zero (`fucompp; test ah,0x44; jnp`: a NaN draws):
// - world axes (0x10123b98): at the point row004123 returns, three lines
//   P - s e_i to P + s e_i, colours 0xff0000, 0xff00 and 0xff, with
//   s = float(NX_VISUALIZATION_SCALE * NX_VISUALIZE_JOINT_WORLD_AXES)
//   (0x9c0d0-0x9c1be); each end component is rounded where it is stored;
// - local axes (0x10123b94): per body, the world anchor, axis, normal and
//   cross carried through the body's +0x134/+0x158 pose (the stored world
//   copies without a body; the anchor's x and y sums and the translation
//   are added on the stack, z is stored first), then six arrows (length
//   1.0f, scale float(NX_VISUALIZE_JOINT_LOCAL_AXES *
//   NX_VISUALIZATION_SCALE)) from each anchor along its normal (0x902020 /
//   0xe05050), cross (0x209020 / 0x50e050) and axis (0x202090 / 0x5050e0),
//   and a line from anchor 0 to anchor 1 (0xffff00).
// The local-axes arm constructs its four two-element NxVec3 arrays through
// the compiler's `eh vector constructor iterator` (phys_fn_000001 with the
// folded NxVec3 constructor 001391, 0x9c1da-0x9c221), as the cylindrical
// row does. The manifest decompile loses the stack arguments of the world
// lines and the arrows; the listing is followed.
void D6Joint::row_slot4(NxDebugRenderable& renderable)
	{
	if(!((mFlags >> 9) & 1))
		return;

	d6RefreshFirstStaleBody(*this);

	if(d6SdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES) != 0.0f)
		{
		NxVec3 point;
		row004123(point);
		const NxReal s = (NxReal)((double)d6SdkParameter(NX_VISUALIZATION_SCALE) *
			d6SdkParameter(NX_VISUALIZE_JOINT_WORLD_AXES));
		NxVec3 lo;
		NxVec3 hi;
		hi.x = (NxReal)((double)point.x + s);
		hi.y = point.y;
		hi.z = point.z;
		lo.x = (NxReal)((double)point.x - s);
		lo.y = point.y;
		lo.z = point.z;
		renderable.addLine(lo, hi, 0xff0000);
		hi.x = point.x;
		hi.y = (NxReal)((double)point.y + s);
		hi.z = point.z;
		lo.x = point.x;
		lo.y = (NxReal)((double)point.y - s);
		lo.z = point.z;
		renderable.addLine(lo, hi, 0xff00);
		hi.x = point.x;
		hi.y = point.y;
		hi.z = (NxReal)((double)point.z + s);
		lo.x = point.x;
		lo.y = point.y;
		lo.z = (NxReal)((double)point.z - s);
		renderable.addLine(lo, hi, 0xff);
		}

	if(d6SdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) != 0.0f)
		{
		NxVec3 anchor[2];
		NxVec3 axis[2];
		NxVec3 normal[2];
		NxVec3 cross[2];

		const JointBodyRecord* body0 = d6Body(mBody[0]);
		if(!body0)
			{
			anchor[0] = mWorldAnchor[0];
			axis[0] = mWorldAxis[0];
			normal[0] = mWorldNormal[0];
			cross[0] = mWorldCross[0];
			}
		else
			{
			// 0x9c2a9-0x9c4e7.
			const NxReal* m = body0->mUnknown134;
			const NxVec3& t = body0->mUnknown158;
			const NxVec3& a = mWorldAnchor[0];
			const double ax = (d6Mul(m[1], a.y) + d6Mul(m[2], a.z)) + d6Mul(a.x, m[0]);
			const NxReal ay = (NxReal)d6Sum3(m[3], a.x, m[4], a.y, m[5], a.z);
			const NxReal az = (NxReal)d6Sum3(m[6], a.x, m[7], a.y, m[8], a.z);
			const double wx = ax + t.x;
			const double wy = (double)ay + t.y;
			anchor[0].z = (NxReal)((double)az + t.z);
			anchor[0].x = (NxReal)wx;
			anchor[0].y = (NxReal)wy;
			const NxVec3& w = mWorldAxis[0];
			const double wAxisY = d6Sum3(m[5], w.z, m[3], w.x, m[4], w.y);
			const double wAxisZ = d6Sum3(m[8], w.z, m[6], w.x, m[7], w.y);
			axis[0].x = (NxReal)d6Sum3(m[2], w.z, m[1], w.y, m[0], w.x);
			axis[0].y = (NxReal)wAxisY;
			axis[0].z = (NxReal)wAxisZ;
			const NxVec3& n = mWorldNormal[0];
			const double nY = d6Sum3(m[4], n.y, m[3], n.x, m[5], n.z);
			const double nZ = d6Sum3(m[7], n.y, m[6], n.x, m[8], n.z);
			normal[0].x = (NxReal)d6Sum3(m[1], n.y, m[2], n.z, m[0], n.x);
			normal[0].y = (NxReal)nY;
			normal[0].z = (NxReal)nZ;
			const NxVec3& c = mWorldCross[0];
			const double cY = d6Sum3(m[5], c.z, m[4], c.y, m[3], c.x);
			const double cZ = d6Sum3(m[8], c.z, m[7], c.y, m[6], c.x);
			cross[0].x = (NxReal)d6Sum3(m[2], c.z, m[1], c.y, m[0], c.x);
			cross[0].y = (NxReal)cY;
			cross[0].z = (NxReal)cZ;
			}

		const JointBodyRecord* body1 = d6Body(mBody[1]);
		if(!body1)
			{
			anchor[1] = mWorldAnchor[1];
			axis[1] = mWorldAxis[1];
			normal[1] = mWorldNormal[1];
			cross[1] = mWorldCross[1];
			}
		else
			{
			// 0x9c56f-0x9c7ad.
			const NxReal* m = body1->mUnknown134;
			const NxVec3& t = body1->mUnknown158;
			const NxVec3& a = mWorldAnchor[1];
			const double ax = d6Sum3(m[2], a.z, m[1], a.y, m[0], a.x);
			const NxReal ay = (NxReal)d6Sum3(m[5], a.z, m[3], a.x, m[4], a.y);
			const NxReal az = (NxReal)d6Sum3(m[8], a.z, m[6], a.x, m[7], a.y);
			const double wx = ax + t.x;
			const double wy = (double)ay + t.y;
			anchor[1].z = (NxReal)((double)az + t.z);
			anchor[1].x = (NxReal)wx;
			anchor[1].y = (NxReal)wy;
			const NxVec3& w = mWorldAxis[1];
			const double wAxisY = d6Sum3(m[5], w.z, m[4], w.y, m[3], w.x);
			const double wAxisZ = d6Sum3(m[8], w.z, m[7], w.y, m[6], w.x);
			axis[1].x = (NxReal)d6Sum3(m[2], w.z, m[1], w.y, m[0], w.x);
			axis[1].y = (NxReal)wAxisY;
			axis[1].z = (NxReal)wAxisZ;
			const NxVec3& n = mWorldNormal[1];
			const double nY = d6Sum3(m[5], n.z, m[4], n.y, m[3], n.x);
			const double nZ = d6Sum3(m[8], n.z, m[7], n.y, m[6], n.x);
			normal[1].x = (NxReal)d6Sum3(m[2], n.z, m[1], n.y, m[0], n.x);
			normal[1].y = (NxReal)nY;
			normal[1].z = (NxReal)nZ;
			const NxVec3& c = mWorldCross[1];
			const double cY = d6Sum3(m[5], c.z, m[3], c.x, m[4], c.y);
			const double cZ = d6Sum3(m[8], c.z, m[6], c.x, m[7], c.y);
			cross[1].x = (NxReal)d6Sum3(m[2], c.z, m[1], c.y, m[0], c.x);
			cross[1].y = (NxReal)cY;
			cross[1].z = (NxReal)cZ;
			}

		const NxReal scale = (NxReal)((double)d6SdkParameter(NX_VISUALIZE_JOINT_LOCAL_AXES) *
			d6SdkParameter(NX_VISUALIZATION_SCALE));
		renderable.addArrow(anchor[0], normal[0], 1.0f, scale, 0x902020);
		renderable.addArrow(anchor[0], cross[0], 1.0f, scale, 0x209020);
		renderable.addArrow(anchor[0], axis[0], 1.0f, scale, 0x202090);
		renderable.addArrow(anchor[1], normal[1], 1.0f, scale, 0xe05050);
		renderable.addArrow(anchor[1], cross[1], 1.0f, scale, 0x50e050);
		renderable.addArrow(anchor[1], axis[1], 1.0f, scale, 0x5050e0);
		renderable.addLine(anchor[0], anchor[1], 0xffff00);
		}
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

static NX_INLINE double d6Twice(double v)
	{
	return v + v;
	}

// A body's pose as 004206/004207 copy it (word moves): its +0x124
// quaternion and +0x158 position, or the identity without a body
// (0x9cc07-0x9cc85, 0x9d8d8-0x9d97e).
static void d6BodyPose(D6JointPose& pose, const JointBodyRecord* body)
	{
	if(body)
		{
		pose.q[0] = body->mCMassOrientation[0];
		pose.q[1] = body->mCMassOrientation[1];
		pose.q[2] = body->mCMassOrientation[2];
		pose.q[3] = body->mCMassOrientation[3];
		pose.p = body->mUnknown158;
		}
	else
		{
		pose.q[0] = 0.0f;
		pose.q[1] = 0.0f;
		pose.q[2] = 0.0f;
		pose.q[3] = 1.0f;
		pose.p.x = 0.0f;
		pose.p.y = 0.0f;
		pose.p.z = 0.0f;
		}
	}

// Body i's joint frame in its own space: the Joint base's world quaternion
// i (+0x12c / +0x13c) and world anchor i (+0x114 / +0x120), word copies.
static void d6Frame(D6JointPose& pose, const Joint& joint, NxU32 i)
	{
	pose.q[0] = joint.mWorldQuat[i][0];
	pose.q[1] = joint.mWorldQuat[i][1];
	pose.q[2] = joint.mWorldQuat[i][2];
	pose.q[3] = joint.mWorldQuat[i][3];
	pose.p = joint.mWorldAnchor[i];
	}

// phys_fn_004206 (0x0009cba0, 3200 B)
// The solver slot (`ret 4`; the float is the step divisor). In order:
// - opens "D6JointDump.txt" ("w") once into the stream at 0x10127194, resets
//   the dump count, refreshes the first stale body;
// - W0 = pose0 * frame0 and W1 = pose1 * frame1 (004178; the bodies' poses
//   and the joint frames, d6BodyPose/d6Frame); ra = W0.p - pose0.p and
//   rb = W1.p - pose1.p, each component stored;
// - M = the 3x3 of W0.q (row-major, each word stored; the listing's
//   groupings: M0 (1 - 2yy) - 2zz, M1 2yx - 2zw, M2 float(2zx) + 2yw, M3
//   2zw + 2yx, M4 (1 - 2xx) - 2zz, M5 2zy - 2wx, M6 float(2zx) - float(2yw),
//   M7 2wx + 2zy, M8 float(1 - 2xx) - 2yy) and N = the first column of W1.q's
//   3x3 (N0 (1 - 2yy) - 2zz, N3 2(yx + zw), N6 2zx - 2yw);
// - rel = W0^-1 * W1 (004180 then 004178) and G = the first column of
//   rel.q's 3x3, with y*y and z*z stored first (G0 (1 - 2yy) - 2zz,
//   G3 2(wz + yx), G6 2zx - 2wy);
// - jwq = the 4x3 matrix of 004198 from W0.q and W1.q; inv = 1.0f / arg;
// - each LOCKED linear motion i: a kind-1 record along M's column i with
//   bias -(inv * rel.p[i]) and maxForce FLT_MAX (004194);
// - when a linear motion is LIMITED: the rows of M scaled by rel.p's
//   LIMITED components summed (x on the stack, y and z stored), its length
//   (fsqrt) minus linearLimit.value; above 0, a kind-0 record along the unit
//   sum with bias -(float(error) * inv) and FLT_MAX;
// - the LOCKED angular motions: with twist free, swing1 locked and swing2
//   free, one record along N x M's column 1 with bias G3 * inv (no sign
//   change); with twist and swing1 free and swing2 locked, along N x M's
//   column 2 with bias G6 * inv; otherwise one record per locked motion
//   along jwq's row 0 (twist), 1 (swing1) or 2 (swing2) with bias
//   -(rel.q[i] * inv); every angular record carries maxTorque;
// - a LIMITED twist: with c the twist cosine, error = (c c x) x - w
//   float((1 - c c) w) on rel.q; above 0, a kind-2 record along
//   2 jwq[0..2] float(c c x) - 2 jwq[9..11] float((1 - c c) w) with bias
//   -(error * inv);
// - both swings LIMITED: the elliptical cone test of the listing
//   (0x9d57d-0x9d7e0; see the body) and, when violated, a kind-2 record;
// - the dump (004192) of W0, W1, rel, jwq and inv.
// Listing over decompile: the supplement decompile reads the linear
// locked bias from the body-0 position slot (Ghidra's frame merges it with
// rel.p, which the listing writes there first, 0x9d02c).
void D6Joint::row_slot6(NxReal arg)
	{
	if(!gD6DumpStream)
		gD6DumpStream = fopen("D6JointDump.txt", "w");
	gD6DumpCount = 0;
	d6RefreshFirstStaleBody(*this);

	D6JointPose pose0;
	D6JointPose pose1;
	D6JointPose frame;
	D6JointPose W0;
	D6JointPose W1;
	d6BodyPose(pose0, d6Body(mBody[0]));
	d6BodyPose(pose1, d6Body(mBody[1]));
	d6Frame(frame, *this, 0);
	pose0.row004178(W0, frame);
	d6Frame(frame, *this, 1);
	pose1.row004178(W1, frame);

	NxVec3 ra;
	NxVec3 rb;
	ra.x = (NxReal)((double)W0.p.x - pose0.p.x);
	ra.y = (NxReal)((double)W0.p.y - pose0.p.y);
	ra.z = (NxReal)((double)W0.p.z - pose0.p.z);
	rb.x = (NxReal)((double)W1.p.x - pose1.p.x);
	rb.y = (NxReal)((double)W1.p.y - pose1.p.y);
	rb.z = (NxReal)((double)W1.p.z - pose1.p.z);

	// M, W0.q's 3x3 (0x9ce7d-0x9cfa4).
	NxReal M[9];
		{
		const NxReal x = W0.q[0];
		const NxReal y = W0.q[1];
		const NxReal z = W0.q[2];
		const NxReal w = W0.q[3];
		const double YY = d6Twice(d6Mul(y, y));
		const double ZZ = d6Twice(d6Mul(z, z));
		M[0] = (NxReal)((1.0f - YY) - ZZ);
		const double XY = d6Twice(d6Mul(y, x));
		const double ZW = d6Twice(d6Mul(z, w));
		M[1] = (NxReal)(XY - ZW);
		const NxReal ZXf = (NxReal)d6Twice(d6Mul(z, x));
		const double YW = d6Twice(d6Mul(y, w));
		const NxReal YWf = (NxReal)YW;
		M[2] = (NxReal)(YW + ZXf);
		M[3] = (NxReal)(ZW + XY);
		const double oneMinusXX = 1.0f - d6Twice(d6Mul(x, x));
		const NxReal oneMinusXXf = (NxReal)oneMinusXX;
		M[4] = (NxReal)(oneMinusXX - ZZ);
		const double ZY = d6Twice(d6Mul(z, y));
		const double WX = d6Twice(d6Mul(w, x));
		M[5] = (NxReal)(ZY - WX);
		M[6] = (NxReal)((double)ZXf - YWf);
		M[7] = (NxReal)(WX + ZY);
		M[8] = (NxReal)((double)oneMinusXXf - YY);
		}

	// N, the first column of W1.q's 3x3 (0x9cfa6-0x9d01e).
	NxReal N0;
	NxReal N3;
	NxReal N6;
		{
		const NxReal x = W1.q[0];
		const NxReal y = W1.q[1];
		const NxReal z = W1.q[2];
		const NxReal w = W1.q[3];
		N0 = (NxReal)((1.0f - d6Twice(d6Mul(y, y))) - d6Twice(d6Mul(z, z)));
		N3 = (NxReal)d6Twice(d6Mul(y, x) + d6Mul(z, w));
		N6 = (NxReal)(d6Twice(d6Mul(z, x)) - d6Twice(d6Mul(y, w)));
		}

	D6JointPose inverse0;
	D6JointPose rel;
	W0.row004180(inverse0)->row004178(rel, W1);

	// G, the first column of rel.q's 3x3 (0x9d031-0x9d0ad).
	const NxReal yyF = (NxReal)d6Mul(rel.q[1], rel.q[1]);
	const NxReal zzF = (NxReal)d6Mul(rel.q[2], rel.q[2]);
	const NxReal G0 = (NxReal)((1.0f - d6Twice(yyF)) - d6Twice(zzF));
	const NxReal G3 = (NxReal)d6Twice(d6Mul(rel.q[3], rel.q[2]) + d6Mul(rel.q[1], rel.q[0]));
	const NxReal G6 = (NxReal)(d6Twice(d6Mul(rel.q[2], rel.q[0])) - d6Twice(d6Mul(rel.q[3], rel.q[1])));

	NxVec3 jwq[4];
	d6QuaternionRateMatrix(&jwq[0].x, W0.q, W1.q);
	const NxReal inv = (NxReal)(1.0f / (double)arg);

	// The LOCKED linear motions (0x9d0d7-0x9d12b).
	const NxReal relP[3] = { rel.p.x, rel.p.y, rel.p.z };
	for(NxU32 i = 0; i < 3; i++)
		{
		if(mMotion[i] == NX_D6JOINT_MOTION_LOCKED)
			{
			NxVec3 axis;
			axis.x = M[i];
			axis.y = M[3 + i];
			axis.z = M[6 + i];
			row004194(0, ra, rb, axis, (NxReal)(-d6Mul(inv, relP[i])), FLT_MAX);
			}
		}

	// The linear limit (0x9d12d-0x9d2b7).
	if(mLinearLimited)
		{
		double sumX = 0.0f;
		NxReal sumY = 0.0f;
		NxReal sumZ = 0.0f;
		if(mMotion[0] == NX_D6JOINT_MOTION_LIMITED)
			{
			sumX = d6Mul(M[0], rel.p.x);
			sumY = (NxReal)d6Mul(M[1], rel.p.x);
			sumZ = (NxReal)d6Mul(M[2], rel.p.x);
			}
		if(mMotion[1] == NX_D6JOINT_MOTION_LIMITED)
			{
			const double tx = d6Mul(M[3], rel.p.y);
			const NxReal ty = (NxReal)d6Mul(M[4], rel.p.y);
			const NxReal tz = (NxReal)d6Mul(M[5], rel.p.y);
			sumX = sumX + tx;
			sumY = (NxReal)((double)ty + sumY);
			sumZ = (NxReal)((double)tz + sumZ);
			}
		if(mMotion[2] == NX_D6JOINT_MOTION_LIMITED)
			{
			const double tx = d6Mul(M[6], rel.p.z);
			const NxReal ty = (NxReal)d6Mul(M[7], rel.p.z);
			const NxReal tz = (NxReal)d6Mul(M[8], rel.p.z);
			sumX = sumX + tx;
			sumY = (NxReal)((double)ty + sumY);
			sumZ = (NxReal)((double)tz + sumZ);
			}
		const double length = jointFsqrtDot3(sumX, sumX, sumY, sumY, sumZ, sumZ);
		const double error = length - mLinearLimit.value;
		const NxReal errorF = (NxReal)error;
		if(error > 0.0f)
			{
			const NxReal inverseLength = (NxReal)(1.0f / length);
			NxVec3 normal;
			normal.x = (NxReal)(sumX * inverseLength);
			normal.y = (NxReal)d6Mul(sumY, inverseLength);
			normal.z = (NxReal)d6Mul(sumZ, inverseLength);
			row004194(1, ra, rb, normal, (NxReal)(-d6Mul(errorF, inv)), FLT_MAX);
			}
		}

	// The LOCKED angular motions (0x9d2b9-0x9d466).
	const NxD6JointMotion twist = mMotion[3];
	const NxD6JointMotion swing1 = mMotion[4];
	const NxD6JointMotion swing2 = mMotion[5];
	if(twist != NX_D6JOINT_MOTION_LOCKED && swing1 == NX_D6JOINT_MOTION_LOCKED &&
		swing2 != NX_D6JOINT_MOTION_LOCKED)
		{
		NxVec3 v;
		v.x = (NxReal)(d6Mul(N3, M[7]) - d6Mul(N6, M[4]));
		v.y = (NxReal)(d6Mul(N6, M[1]) - d6Mul(M[7], N0));
		v.z = (NxReal)(d6Mul(M[4], N0) - d6Mul(N3, M[1]));
		row004196(0, v, (NxReal)d6Mul(G3, inv), mMaxTorque);
		}
	else if(twist != NX_D6JOINT_MOTION_LOCKED && swing1 != NX_D6JOINT_MOTION_LOCKED &&
		swing2 == NX_D6JOINT_MOTION_LOCKED)
		{
		NxVec3 v;
		v.x = (NxReal)(d6Mul(N3, M[8]) - d6Mul(N6, M[5]));
		v.y = (NxReal)(d6Mul(N6, M[2]) - d6Mul(M[8], N0));
		v.z = (NxReal)(d6Mul(M[5], N0) - d6Mul(N3, M[2]));
		row004196(0, v, (NxReal)d6Mul(G6, inv), mMaxTorque);
		}
	else
		{
		if(twist == NX_D6JOINT_MOTION_LOCKED)
			row004196(0, jwq[0], (NxReal)(-d6Mul(rel.q[0], inv)), mMaxTorque);
		if(swing1 == NX_D6JOINT_MOTION_LOCKED)
			row004196(0, jwq[1], (NxReal)(-d6Mul(rel.q[1], inv)), mMaxTorque);
		if(swing2 == NX_D6JOINT_MOTION_LOCKED)
			row004196(0, jwq[2], (NxReal)(-d6Mul(rel.q[2], inv)), mMaxTorque);
		}

	// The twist limit (0x9d46b-0x9d563).
	if(twist == NX_D6JOINT_MOTION_LIMITED)
		{
		const double c = mTwistCosHalf;
		const double cc = c * c;
		const NxReal A = (NxReal)((1.0f - cc) * rel.q[3]);
		const double B = cc * rel.q[0];
		const NxReal Bf = (NxReal)B;
		const double error = B * rel.q[0] - d6Mul(rel.q[3], A);
		if(error > 0.0f)
			{
			const NxReal j9A = (NxReal)(d6Twice(jwq[3].x) * A);
			const double j10A = d6Twice(jwq[3].y) * A;
			const double j11A = d6Twice(jwq[3].z) * A;
			const NxReal j0B = (NxReal)(d6Twice(jwq[0].x) * Bf);
			const double j1B = d6Twice(jwq[0].y) * Bf;
			const NxReal j2B = (NxReal)(d6Twice(jwq[0].z) * Bf);
			NxVec3 v;
			v.x = (NxReal)((double)j0B - j9A);
			v.y = (NxReal)(j1B - j10A);
			v.z = (NxReal)((double)j2B - j11A);
			row004196(1, v, (NxReal)(-(error * inv)), mMaxTorque);
			}
		}

	// The swing cone (0x9d565-0x9d7e9). With s = |(w, x)| and t = |(y, z)|
	// of rel.q (t from the stored squares), k = 2 (1 + s) s, m = 2 s t and
	// a_i = ((c_i + 1) G) / (1 - c_i) for swing1 (G3) and swing2 (G6): when
	// q = G6 a2 + G3 a1 exceeds k k, the record's axis is N x l, where l is
	// the listing's combination of M's columns (below), and the bias is
	// -((float(1 / (1 + s)) t - float(sqrt(1 / q)) m) inv).
	if(swing1 == NX_D6JOINT_MOTION_LIMITED && swing2 == NX_D6JOINT_MOTION_LIMITED)
		{
		const double s = jointFsqrtSum2(d6Mul(rel.q[3], rel.q[3]), d6Mul(rel.q[0], rel.q[0]));
		const NxReal t = (NxReal)jointFsqrtSum2(yyF, zzF);
		const double u = 1.0f + s;
		const double us = u * s;
		const NxReal k = (NxReal)(us + us);
		const NxReal inverseU = (NxReal)(1.0f / u);
		const double st = s * t;
		const double m = st + st;
		const NxReal c1 = mSwing1CosHalf;
		const NxReal a1 = (NxReal)((((double)c1 + 1.0f) * G3) / (1.0f - (double)c1));
		const NxReal c2 = mSwing2CosHalf;
		const NxReal a2 = (NxReal)((((double)c2 + 1.0f) * G6) / (1.0f - (double)c2));
		const double q = d6Mul(G6, a2) + d6Mul(G3, a1);
		if(q > d6Mul(k, k))
			{
			const double iq = 1.0f / q;
			const double sq = jointFsqrt(iq);
			const NxReal sqF = (NxReal)sq;
			const double r = (sq * m) * iq;
			const NxReal e0 = (NxReal)((M[2] * r) * a2);
			const double e1 = (M[5] * r) * a2;
			const NxReal e2 = (NxReal)((double)(NxReal)(M[8] * r) * a2);
			const NxReal f0 = (NxReal)((M[1] * r) * a1);
			const double f1 = (M[4] * r) * a1;
			const double f2 = (double)(NxReal)(M[7] * r) * a1;
			const double h = d6Mul(inverseU, 0.5f) - d6Mul(G0, sqF);
			const double m0h = M[0] * h;
			const NxReal m3h = (NxReal)(M[3] * h);
			const NxReal m6h = (NxReal)(M[6] * h);
			const NxReal im = (NxReal)(1.0f / m);
			const double g0 = m0h * im;
			const double g1 = d6Mul(m3h, im);
			const NxReal g2 = (NxReal)d6Mul(m6h, im);
			const NxReal k0 = (NxReal)(g0 - f0);
			const NxReal k1 = (NxReal)(g1 - f1);
			const NxReal k2 = (NxReal)((double)g2 - f2);
			const NxReal l0 = (NxReal)((double)k0 - e0);
			const double l1 = (double)k1 - e1;
			const NxReal l2 = (NxReal)((double)k2 - e2);
			NxVec3 v;
			v.x = (NxReal)(N6 * l1 - d6Mul(l2, N3));
			v.y = (NxReal)(d6Mul(l2, N0) - d6Mul(N6, l0));
			v.z = (NxReal)(d6Mul(N3, l0) - l1 * N0);
			const NxReal bias = (NxReal)(-((d6Mul(inverseU, t) - sqF * m) * inv));
			row004196(1, v, bias, mMaxTorque);
			}
		}

	d6Dump(&W0, &W1, &rel, &jwq[0].x, inv);
	}

// phys_fn_004207 (0x0009d820, 2394 B)
// Projection (internal slot 8, `ret 4`; the argument is the body record to
// move, compared with mBody[1] only). After the stale-body refresh:
// - W0 = pose0 * frame0, W1 = pose1 * frame1 and rel = W0^-1 * W1 as in
//   004206 (0x9d859-0x9da5f);
// - the correction pose starts as the identity; per linear motion rel.p's
//   component goes to the locked vector (LOCKED; x on the stack, y and z
//   stored), the limited vector (LIMITED) or straight into the correction
//   (FREE). A locked vector longer than projectionDistance is scaled to it
//   (squares summed x, z, y; the compare re-forms the sum), a limited one
//   longer than float(linearLimit.value + projectionDistance) to that, and
//   both are added to the correction's position;
// - the angular correction by the set of LOCKED angular motions (the
//   listing's jump table at 0x9e17c, index twist + 2 swing1 + 4 swing2 - 1):
//   one locked motion keeps the matching quaternion component pair
//   normalised; two locked swings or twist with one swing keep one
//   component and w normalised; all three give the identity;
// - the target: for body 1, W0 * correction * frame1^-1; for any other
//   body, W1 * correction^-1 * frame0^-1 (0x9ddcc-0x9e032). The body's +0x124
//   quaternion and +0x158 position take it, its +0x134 3x3 is rebuilt from
//   the quaternion (a local matrix copied with `rep movsd`), and row 000022
//   is called on the body's +0x19c owner with 1 (deferred: its stub
//   asserts, as in revolute 004356 and spherical 004298).
void D6Joint::row_slot8(void* bodyArgument)
	{
	d6RefreshFirstStaleBody(*this);

	D6JointPose frame0;
	D6JointPose frame1;
	d6Frame(frame0, *this, 0);
	d6Frame(frame1, *this, 1);
	D6JointPose pose;
	D6JointPose W0;
	D6JointPose W1;
	d6BodyPose(pose, d6Body(mBody[0]));
	pose.row004178(W0, frame0);
	d6BodyPose(pose, d6Body(mBody[1]));
	pose.row004178(W1, frame1);
	D6JointPose inverse0;
	D6JointPose rel;
	W0.row004180(inverse0)->row004178(rel, W1);

	D6JointPose correction;
	correction.q[0] = 0.0f;
	correction.q[1] = 0.0f;
	correction.q[2] = 0.0f;
	correction.q[3] = 1.0f;
	correction.p.x = 0.0f;
	correction.p.y = 0.0f;
	correction.p.z = 0.0f;
	NxReal limitedX = 0.0f;
	NxReal limitedY = 0.0f;
	NxReal limitedZ = 0.0f;
	double lockedX = 0.0f;
	NxReal lockedY = 0.0f;
	NxReal lockedZ = 0.0f;

	// 0x9da6a-0x9db45.
	if(mMotion[0] == NX_D6JOINT_MOTION_LOCKED)
		lockedX = rel.p.x;
	else if(mMotion[0] == NX_D6JOINT_MOTION_LIMITED)
		limitedX = rel.p.x;
	else
		correction.p.x = rel.p.x;
	if(mMotion[1] == NX_D6JOINT_MOTION_LOCKED)
		lockedY = rel.p.y;
	else if(mMotion[1] == NX_D6JOINT_MOTION_LIMITED)
		limitedY = rel.p.y;
	else
		correction.p.y = rel.p.y;
	if(mMotion[2] == NX_D6JOINT_MOTION_LOCKED)
		lockedZ = rel.p.z;
	else if(mMotion[2] == NX_D6JOINT_MOTION_LIMITED)
		limitedZ = rel.p.z;
	else
		correction.p.z = rel.p.z;

	// The locked vector (0x9db49-0x9dbc4).
	const double lockedSquared = (lockedX * lockedX + d6Mul(lockedZ, lockedZ)) + d6Mul(lockedY, lockedY);
	const NxReal distance = mProjectionDistance;
	if(lockedSquared > d6Mul(distance, distance))
		{
		const double scale = distance / jointFsqrtDot3(lockedX, lockedX, lockedZ, lockedZ, lockedY, lockedY);
		lockedX = lockedX * scale;
		lockedY = (NxReal)(lockedY * scale);
		lockedZ = (NxReal)(lockedZ * scale);
		}
	correction.p.x = (NxReal)((double)correction.p.x + lockedX);
	correction.p.y = (NxReal)((double)correction.p.y + lockedY);
	correction.p.z = (NxReal)((double)correction.p.z + lockedZ);

	// The limited vector (0x9dbca-0x9dc41).
	if(mLinearLimited)
		{
		const NxReal limit = (NxReal)((double)mLinearLimit.value + mProjectionDistance);
		double lx = limitedX;
		double ly = limitedY;
		double lz = limitedZ;
		const double limitedSquared = (d6Mul(limitedX, limitedX) + d6Mul(limitedZ, limitedZ)) +
			d6Mul(limitedY, limitedY);
		if(limitedSquared > d6Mul(limit, limit))
			{
			const double scale = limit / jointFsqrtDot3(limitedX, limitedX, limitedZ, limitedZ, limitedY, limitedY);
			lx = lx * scale;
			ly = ly * scale;
			lz = lz * scale;
			}
		correction.p.x = (NxReal)((double)correction.p.x + lx);
		correction.p.y = (NxReal)((double)correction.p.y + ly);
		correction.p.z = (NxReal)((double)correction.p.z + lz);
		}

	// The angular correction (0x9dc43-0x9dfeb).
	const NxReal x = rel.q[0];
	const NxReal y = rel.q[1];
	const NxReal z = rel.q[2];
	const NxReal w = rel.q[3];
	const NxU32 locked = (mMotion[3] == NX_D6JOINT_MOTION_LOCKED ? 1 : 0) +
		(mMotion[4] == NX_D6JOINT_MOTION_LOCKED ? 2 : 0) + (mMotion[5] == NX_D6JOINT_MOTION_LOCKED ? 4 : 0);
	switch(locked)
		{
		case 1:
			{
			// Twist locked (0x9dc88-0x9dcf5): (0, -(zx - yw) / s, (yx + zw) / s, s)
			// with s = |(x, w)|.
			const double s = jointFsqrtSum2(d6Mul(x, x), d6Mul(w, w));
			if(s > 0.0f)
				{
				correction.q[0] = 0.0f;
				const double is = 1.0f / s;
				correction.q[1] = (NxReal)(-((d6Mul(z, x) - d6Mul(y, w)) * is));
				correction.q[2] = (NxReal)((d6Mul(y, x) + d6Mul(z, w)) * is);
				correction.q[3] = (NxReal)s;
				}
			break;
			}
		case 2:
			{
			// Swing1 locked (0x9dcfa-0x9ddc6).
			double a = (double)w - y;
			double b = (double)z + x;
			NxReal c = (NxReal)((double)y + w);
			double d = (double)z - x;
			const NxReal n1 = (NxReal)jointFsqrtDot2(b, b, a, a);
			const NxReal n2 = (NxReal)jointFsqrtDot2(d, d, c, c);
			if(n1 != 0.0f && n2 != 0.0f)
				{
				const double i1 = 1.0f / (double)n1;
				a = a * i1;
				b = b * i1;
				const double i2 = 1.0f / (double)n2;
				c = (NxReal)(c * i2);
				d = d * i2;
				}
			correction.q[0] = (NxReal)((b - d) * 0.5f);
			correction.q[1] = (NxReal)(((double)c - a) * 0.5f);
			correction.q[2] = (NxReal)((d + b) * 0.5f);
			correction.q[3] = (NxReal)(((double)c + a) * 0.5f);
			break;
			}
		case 3:
			{
			// Twist and swing1 locked (0x9df79-0x9dfc2): (0, 0, z, w) / |(z, w)|.
			const double s = jointFsqrtSum2(d6Mul(z, z), d6Mul(w, w));
			if(s > 0.0f)
				{
				const double is = 1.0f / s;
				correction.q[0] = 0.0f;
				correction.q[1] = 0.0f;
				correction.q[2] = (NxReal)(z * is);
				correction.q[3] = (NxReal)(is * w);
				}
			break;
			}
		case 4:
			{
			// Swing2 locked (0x9de0f-0x9dedf).
			double a = (double)z + w;
			double b = (double)x - y;
			double c = (double)y + x;
			NxReal d = (NxReal)((double)z - w);
			const NxReal n1 = (NxReal)jointFsqrtDot2(c, c, a, a);
			const NxReal n2 = (NxReal)jointFsqrtDot2(d, d, b, b);
			if(n1 != 0.0f && n2 != 0.0f)
				{
				const double i1 = 1.0f / (double)n1;
				a = a * i1;
				const double i2 = 1.0f / (double)n2;
				const NxReal i2F = (NxReal)i2;
				b = b * i2;
				c = c * i1;
				d = (NxReal)d6Mul(i2F, d);
				}
			correction.q[0] = (NxReal)((c + b) * 0.5f);
			correction.q[1] = (NxReal)((c - b) * 0.5f);
			correction.q[2] = (NxReal)(((double)d + a) * 0.5f);
			correction.q[3] = (NxReal)((a - d) * 0.5f);
			break;
			}
		case 5:
			{
			// Twist and swing2 locked (0x9df27-0x9df74): (0, y, 0, w) / |(y, w)|.
			const double s = jointFsqrtSum2(d6Mul(y, y), d6Mul(w, w));
			if(s > 0.0f)
				{
				const double is = 1.0f / s;
				correction.q[0] = 0.0f;
				correction.q[1] = (NxReal)(y * is);
				correction.q[3] = (NxReal)(is * w);
				correction.q[2] = 0.0f;
				}
			break;
			}
		case 6:
			{
			// Both swings locked (0x9dee8-0x9df74): (x, 0, 0, w) / |(x, w)|.
			const double s = jointFsqrtSum2(d6Mul(x, x), d6Mul(w, w));
			if(s > 0.0f)
				{
				const double is = 1.0f / s;
				correction.q[1] = 0.0f;
				correction.q[0] = (NxReal)(x * is);
				correction.q[3] = (NxReal)(is * w);
				correction.q[2] = 0.0f;
				}
			break;
			}
		case 7:
			correction.q[0] = 0.0f;
			correction.q[1] = 0.0f;
			correction.q[2] = 0.0f;
			correction.q[3] = 1.0f;
			break;
		default:
			break;
		}

	// The target pose (0x9ddcc-0x9e032).
	JointBodyRecord* const body = d6Body(bodyArgument);
	D6JointPose target;
	if(bodyArgument == mBody[1])
		{
		D6JointPose inverse1;
		D6JointPose moved;
		frame1.row004180(inverse1);
		W0.row004178(moved, correction);
		moved.row004178(target, inverse1);
		}
	else
		{
		D6JointPose inverseFrame0;
		D6JointPose inverseCorrection;
		D6JointPose moved;
		frame0.row004180(inverseFrame0);
		correction.row004180(inverseCorrection);
		W1.row004178(moved, inverseCorrection);
		moved.row004178(target, inverseFrame0);
		}

	// The body's pose and 3x3 (0x9e037-0x9e15f); x, y and z stay on the
	// stack, w is read back from a stored copy.
	const NxReal qx = target.q[0];
	const NxReal qy = target.q[1];
	const NxReal qz = target.q[2];
	const NxReal qw = target.q[3];
	body->mUnknown158 = target.p;
	body->mCMassOrientation[3] = qw;
	body->mCMassOrientation[0] = qx;
	body->mCMassOrientation[1] = qy;
	body->mCMassOrientation[2] = qz;
	NxReal R[9];
	const NxReal YYf = (NxReal)d6Twice(d6Mul(qy, qy));
	const double ZZ = d6Twice(d6Mul(qz, qz));
	R[0] = (NxReal)((1.0f - (double)YYf) - ZZ);
	const double XY = d6Twice(d6Mul(qy, qx));
	const double WZ = d6Twice(d6Mul(qw, qz));
	R[1] = (NxReal)(XY - WZ);
	const NxReal WYf = (NxReal)d6Twice(d6Mul(qw, qy));
	const double ZX = d6Twice(d6Mul(qz, qx));
	const NxReal ZXf = (NxReal)ZX;
	R[2] = (NxReal)(ZX + WYf);
	R[3] = (NxReal)(WZ + XY);
	const double oneMinusXX = 1.0f - d6Twice(d6Mul(qx, qx));
	const NxReal oneMinusXXf = (NxReal)oneMinusXX;
	R[4] = (NxReal)(oneMinusXX - ZZ);
	const double ZY = d6Twice(d6Mul(qy, qz));
	const double WX = d6Twice(d6Mul(qw, qx));
	R[5] = (NxReal)(ZY - WX);
	R[6] = (NxReal)((double)ZXf - WYf);
	R[7] = (NxReal)(WX + ZY);
	R[8] = (NxReal)((double)oneMinusXXf - YYf);
	for(NxU32 i = 0; i < 9; i++)
		body->mUnknown134[i] = R[i];
	// Row 000022 is deferred (owner gap <start>..Actor.cpp); its stub asserts.
	reinterpret_cast<Row000022Fixture*>(body->mOwner)->row000022(1);
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
