#ifndef NP_ACTOR_DYNAMIC_MATH_H
#define NP_ACTOR_DYNAMIC_MATH_H

#include "NxMat33.h"
#include "NxQuat.h"
#include "X87Sqrt.h"
#include <math.h>
#include <string.h>

// Dynamic record matrices are row-major. The shipped Win32 build evaluates
// quaternion products in x87 precision, then rounds each matrix element to
// float; its tensor helper rounds the scaled columns before accumulating.
static inline float nxNpActorRoundProduct(float a, float b)
	{
	volatile float result = a * b;
	return result;
	}

static inline void nxNpActorRotationFromQuaternionAt(
	const unsigned char* record, unsigned offset, float* rotation)
	{
	const float* q = reinterpret_cast<const float*>(record + offset);
	const double x = q[0], y = q[1], z = q[2], w = q[3];
	rotation[0] = static_cast<float>(1.0 - 2.0 * y * y - 2.0 * z * z);
	rotation[1] = static_cast<float>(2.0 * x * y - 2.0 * w * z);
	rotation[2] = static_cast<float>(2.0 * x * z + 2.0 * w * y);
	rotation[3] = static_cast<float>(2.0 * x * y + 2.0 * w * z);
	rotation[4] = static_cast<float>(1.0 - 2.0 * x * x - 2.0 * z * z);
	rotation[5] = static_cast<float>(2.0 * y * z - 2.0 * w * x);
	rotation[6] = static_cast<float>(2.0 * x * z - 2.0 * w * y);
	rotation[7] = static_cast<float>(2.0 * y * z + 2.0 * w * x);
	rotation[8] = static_cast<float>(1.0 - 2.0 * x * x - 2.0 * y * y);
	}

static inline void nxNpActorRotationFromQuaternion(
	const unsigned char* record, float* rotation)
	{
	nxNpActorRotationFromQuaternionAt(record, 0x5c, rotation);
	}

static inline void nxNpActorWorldTensor(const float* diagonal,
	const float* rotation, float* world)
	{
	for(unsigned row = 0; row < 3; ++row)
		{
		const float x = nxNpActorRoundProduct(diagonal[0], rotation[row * 3]);
		const float y = nxNpActorRoundProduct(diagonal[1], rotation[row * 3 + 1]);
		const float z = nxNpActorRoundProduct(diagonal[2], rotation[row * 3 + 2]);
		for(unsigned col = row; col < 3; ++col)
			{
			const double xx = static_cast<double>(x) * rotation[col * 3];
			const double zz = static_cast<double>(z) * rotation[col * 3 + 2];
			const double yy = static_cast<double>(y) * rotation[col * 3 + 1];
			world[row * 3 + col] = static_cast<float>(xx + zz + yy);
			world[col * 3 + row] = world[row * 3 + col];
			}
		}
	}

static inline void nxNpActorQuaternionFromMatrix(const float* m, float* q)
	{
	const double trace = static_cast<double>(m[0]) + m[4] + m[8];
	if(trace >= 0.0)
		{
		const double root = sqrt(trace + 1.0);
		const double scale = 0.5 / root;
		q[3] = static_cast<float>(0.5 * root);
		q[0] = static_cast<float>((static_cast<double>(m[7]) - m[5]) * scale);
		q[1] = static_cast<float>((static_cast<double>(m[2]) - m[6]) * scale);
		q[2] = static_cast<float>((static_cast<double>(m[3]) - m[1]) * scale);
		}
	else
		{
		unsigned axis = m[4] > m[0] ? 1u : 0u;
		if(m[8] > m[axis * 3 + axis]) axis = 2;
		const unsigned next = (axis + 1) % 3;
		const unsigned last = (axis + 2) % 3;
		const double root = sqrt(static_cast<double>(m[axis * 3 + axis]) -
			(static_cast<double>(m[next * 3 + next]) + m[last * 3 + last]) + 1.0);
		const double scale = 0.5 / root;
		q[axis] = static_cast<float>(0.5 * root);
		q[next] = static_cast<float>((static_cast<double>(m[axis * 3 + next]) +
			m[next * 3 + axis]) * scale);
		q[last] = static_cast<float>((static_cast<double>(m[axis * 3 + last]) +
			m[last * 3 + axis]) * scale);
		q[3] = static_cast<float>((static_cast<double>(m[last * 3 + next]) -
			m[next * 3 + last]) * scale);
		}
	}

// The body pose constructor's matrix-to-quaternion conversion (phys_fn_000801,
// 0x1b82e-0x1b987), which fills the dynamic record's quaternion at +0x24 and
// copies it to +0x5c when an actor is created (joint-open-items Task 4). It is
// not the public NxMat33::toQuat: the listing keeps every intermediate in an x87
// register (53-bit here, so double), sums the trace as (m11 + m22) + m00 and
// spills (m11 + m22) to a float that the x arm reuses. Each arm computes
// s = sqrt(... + 1), stores 0.5 * s, and multiplies three sums or differences
// by the register reciprocal 0.5 / s.
// It is also phys_fn_000756 (0x17420, 525 B, thiscall on the record: +0x134 to
// +0x124), which setCMassGlobalPose/setCMassGlobalOrientation (000204/000208)
// call: the same instruction sequence with the (m11 + m22) spill at [esp].
// The roots are the listing's inline fsqrt (X87Sqrt.h), each over the operands
// in the listing's order: fadd 1.0f after the trace, `fld b; fadd c; fsubr a;
// fadd 1.0f` in the y and z arms, m00 - float(m11 + m22) + 1.0f in the x arm.
static inline void nxNpActorBodyQuaternionFromMatrix(const float* m, float* q)
	{
	const double yz = static_cast<double>(m[4]) + m[8];
	const float yzSpill = static_cast<float>(yz);		// fst [esp+0xc], 0x1b834
	const double trace = yz + m[0];
	if(trace >= 0.0)		// fcom 0.0; test ah, 1: below or unordered takes the arms
		{
		const double s = x87FsqrtSum4(m[4], m[8], m[0], 1.0);
		q[3] = static_cast<float>(0.5 * s);
		const double r = 0.5 / s;
		q[0] = static_cast<float>((static_cast<double>(m[7]) - m[5]) * r);
		q[1] = static_cast<float>((static_cast<double>(m[2]) - m[6]) * r);
		q[2] = static_cast<float>((static_cast<double>(m[3]) - m[1]) * r);
		return;
		}
	// fcomp with `test ah, 0x41`: the index moves only on a strict greater.
	unsigned axis = m[4] > m[0] ? 1u : 0u;
	if(m[8] > m[axis * 4])
		axis = 2;
	if(axis == 0)
		{
		const double s = x87FsqrtDiffSum(m[0], yzSpill, 1.0);
		q[0] = static_cast<float>(0.5 * s);
		const double r = 0.5 / s;
		q[1] = static_cast<float>((static_cast<double>(m[3]) + m[1]) * r);
		q[2] = static_cast<float>((static_cast<double>(m[6]) + m[2]) * r);
		q[3] = static_cast<float>((static_cast<double>(m[7]) - m[5]) * r);
		}
	else if(axis == 1)
		{
		const double s = x87FsqrtDiag(m[4], m[8], m[0]);		// 0x1001755a
		q[1] = static_cast<float>(0.5 * s);
		const double r = 0.5 / s;
		q[2] = static_cast<float>((static_cast<double>(m[7]) + m[5]) * r);
		q[0] = static_cast<float>((static_cast<double>(m[3]) + m[1]) * r);
		q[3] = static_cast<float>((static_cast<double>(m[2]) - m[6]) * r);
		}
	else
		{
		const double s = x87FsqrtDiag(m[8], m[4], m[0]);		// 0x100174ec
		q[2] = static_cast<float>(0.5 * s);
		const double r = 0.5 / s;
		q[0] = static_cast<float>((static_cast<double>(m[6]) + m[2]) * r);
		q[1] = static_cast<float>((static_cast<double>(m[7]) + m[5]) * r);
		q[3] = static_cast<float>((static_cast<double>(m[3]) - m[1]) * r);
		}
	}

// The pose setters' own matrix-to-quaternion conversion: setGlobalPose
// (phys_fn_000196, 0x8b5c-0x8d07) and setGlobalOrientation (phys_fn_000200,
// 0x9110-, the same instruction sequence). The trace arm matches 000801's
// conversion. The other arms differ in what they spill:
// - the z arm spills s to float and forms 0.5 / float(s);
// - the y and x arms spill the reciprocal 0.5 / s to float before using it.
// In every arm the (m22 + m11) sum is spilled to float, as in 000801.
// phys_fn_000789 (0x19d00) converts its actor rotation with the same sequence
// (0x19fe1-0x1a1a0: spill at [esp+0x10], trace arm, z arm 0x1a093 with
// float(s), y arm 0x1a0ec and x arm 0x1a148 with the reciprocal spilled).
// The roots are the listing's inline fsqrt (X87Sqrt.h) over its operands.
static inline void nxNpActorSetterQuaternionFromMatrix(const float* m, float* q)
	{
	const double zy = static_cast<double>(m[8]) + m[4];
	const float zySpill = static_cast<float>(zy);		// fst [esp+0x34], 0x8b66
	const double trace = zy + m[0];
	if(trace >= 0.0)
		{
		const double s = x87FsqrtSum4(m[8], m[4], m[0], 1.0);
		q[3] = static_cast<float>(0.5 * s);
		const double r = 0.5 / s;
		q[0] = static_cast<float>((static_cast<double>(m[7]) - m[5]) * r);
		q[1] = static_cast<float>((static_cast<double>(m[2]) - m[6]) * r);
		q[2] = static_cast<float>((static_cast<double>(m[3]) - m[1]) * r);
		return;
		}
	unsigned axis = m[4] > m[0] ? 1u : 0u;
	if(m[8] > m[axis * 4])
		axis = 2;
	if(axis == 2)
		{
		// 0x8bf7: fst [esp+0x34] keeps a float copy of s for the reciprocal.
		const double s = x87FsqrtDiag(m[8], m[4], m[0]);
		const float sSpill = static_cast<float>(s);
		q[2] = static_cast<float>(s * 0.5);
		const double r = 0.5 / static_cast<double>(sSpill);
		q[0] = static_cast<float>((static_cast<double>(m[6]) + m[2]) * r);
		q[1] = static_cast<float>((static_cast<double>(m[7]) + m[5]) * r);
		q[3] = static_cast<float>((static_cast<double>(m[3]) - m[1]) * r);
		}
	else if(axis == 1)
		{
		// 0x8c46: the reciprocal is stored to [esp+0x34] and reloaded.
		const double s = x87FsqrtDiag(m[4], m[8], m[0]);
		q[1] = static_cast<float>(0.5 * s);
		const double r = static_cast<float>(0.5 / s);
		q[2] = static_cast<float>((static_cast<double>(m[7]) + m[5]) * r);
		q[0] = static_cast<float>((static_cast<double>(m[3]) + m[1]) * r);
		q[3] = static_cast<float>((static_cast<double>(m[2]) - m[6]) * r);
		}
	else
		{
		// 0x8c98: as the y arm, over m00 - float(m22 + m11).
		const double s = x87FsqrtDiffSum(m[0], zySpill, 1.0);
		q[0] = static_cast<float>(0.5 * s);
		const double r = static_cast<float>(0.5 / s);
		q[1] = static_cast<float>((static_cast<double>(m[3]) + m[1]) * r);
		q[2] = static_cast<float>((static_cast<double>(m[6]) + m[2]) * r);
		q[3] = static_cast<float>((static_cast<double>(m[7]) - m[5]) * r);
		}
	}

// phys_fn_000746 (0x00016e80, 245 B)
// A row of gap:SceneRaycast.cpp..CapsuleShape.cpp that 000768 and 000789 call.
// out = R diag(d) R^T, as the cdecl helper phys_fn_000746 (0x16e80, 245 B)
// forms it (joint-open-items Task 4). Of the nine products d[k] * R[i][k],
// four stay in x87 registers (d0*R00, d0*R20, d1*R21, d2*R22: double here)
// and five are spilled to float; each element then sums three products in
// the listing's order and the symmetric pairs store one value twice.
static inline void nxNpActorWorldTensorRDRt(const float* d, const float* r, float* out)
	{
	const double a = static_cast<double>(d[0]) * r[0];
	const float s4 = static_cast<float>(static_cast<double>(d[0]) * r[3]);
	const double b = static_cast<double>(d[0]) * r[6];
	const float s14 = static_cast<float>(static_cast<double>(r[1]) * d[1]);
	const float s8 = static_cast<float>(static_cast<double>(d[1]) * r[4]);
	const double c = static_cast<double>(d[1]) * r[7];
	const float s10 = static_cast<float>(static_cast<double>(r[2]) * d[2]);
	const float s0 = static_cast<float>(static_cast<double>(r[5]) * d[2]);
	const double dd = static_cast<double>(r[8]) * d[2];
	out[0] = static_cast<float>((static_cast<double>(s14) * r[1] + static_cast<double>(s10) * r[2]) + a * r[0]);
	out[4] = static_cast<float>((static_cast<double>(s0) * r[5] + static_cast<double>(s4) * r[3]) +
		static_cast<double>(s8) * r[4]);
	out[8] = static_cast<float>((dd * r[8] + b * r[6]) + c * r[7]);
	out[3] = out[1] = static_cast<float>((static_cast<double>(s10) * r[5] + a * r[3]) +
		static_cast<double>(s14) * r[4]);
	out[6] = out[2] = static_cast<float>((static_cast<double>(s10) * r[8] + a * r[6]) +
		static_cast<double>(s14) * r[7]);
	out[7] = out[5] = static_cast<float>((static_cast<double>(s0) * r[8] + static_cast<double>(s4) * r[6]) +
		static_cast<double>(s8) * r[7]);
	}

// The rotation matrix, row-major, of a record quaternion (x, y, z, w), as
// every inline expansion in the listing forms it (000768 from +0x24;
// 000218, 000220, 000222 and the joint-descriptor exports 004115/004117 from
// +0x5c: the same instruction sequence, compared with registers and stack
// slots normalised). Each doubled product is `fmul; fadd st(0), st(0)` in an
// x87 register (53-bit here, so double); five are spilled to float.
static inline void nxNpActorComposeRotation(const float* q, float* r)
	{
	const double x = q[0], y = q[1], z = q[2], w = q[3];
	const float yy2 = static_cast<float>(y * y + y * y);
	const double zz2 = z * z + z * z;
	r[0] = static_cast<float>((1.0 - yy2) - zz2);
	const double xy2 = y * x + y * x;
	const double zw2 = z * w + z * w;
	r[1] = static_cast<float>(xy2 - zw2);
	const float xz2 = static_cast<float>(z * x + z * x);
	const double yw2 = y * w + y * w;
	const float yw2Spill = static_cast<float>(yw2);
	r[2] = static_cast<float>(yw2 + xz2);
	r[3] = static_cast<float>(zw2 + xy2);
	const double xx1 = 1.0 - (x * x + x * x);
	const float xx1Spill = static_cast<float>(xx1);
	r[4] = static_cast<float>(xx1 - zz2);
	const float yz2 = static_cast<float>(z * y + z * y);
	const double xw2 = x * w + x * w;
	r[5] = static_cast<float>(static_cast<double>(yz2) - xw2);
	r[6] = static_cast<float>(static_cast<double>(xz2) - yw2Spill);
	r[7] = static_cast<float>(xw2 + yz2);
	r[8] = static_cast<float>(static_cast<double>(xx1Spill) - yy2);
	}

// The mass-frame refresh phys_fn_000768 (0x17f10, 1164 B, thiscall on the
// record, joint-open-items Task 4). From the pose quaternion at +0x24 (w
// last) and position at +0x18 it forms the rotation R with the same x87
// pattern the joint-descriptor exports use (five doubled products spilled to
// float), then writes
//   +0x134 = R * F (F the mass-frame 3x3 at +0xdc), each element summed in
//            the listing's operand order and rounded once;
//   +0x158 = R * p + t (p the mass-frame position at +0x100): x stays in the
//            register until the store, y and z round the product sum first;
//   +0x124 = the quaternion of +0x134, by the conversion 000801 uses;
// and calls the world-tensor helper 0x16e80 for +0x164. The actor creation
// path calls it, and so do the pose and CMass-offset setters, through
// nxNpActorRefreshCMass (NpActor.cpp), as every oracle setter ends in
// `call 0x10017f10`.
static inline void nxNpActorUpdateMassFrame(unsigned char* record)
	{
	float r[9];
	nxNpActorComposeRotation(reinterpret_cast<const float*>(record + 0x24), r);

	// World centre of mass, 0x17fe1-0x181f4.
	const float* t = reinterpret_cast<const float*>(record + 0x18);
	const float* p = reinterpret_cast<const float*>(record + 0x100);
	const double cx = (static_cast<double>(r[0]) * p[0] + static_cast<double>(r[1]) * p[1]) +
		static_cast<double>(r[2]) * p[2];
	const float cy = static_cast<float>((static_cast<double>(r[4]) * p[1] +
		static_cast<double>(r[5]) * p[2]) + static_cast<double>(r[3]) * p[0]);
	const float cz = static_cast<float>((static_cast<double>(r[7]) * p[1] +
		static_cast<double>(r[8]) * p[2]) + static_cast<double>(r[6]) * p[0]);
	const double wx = cx + t[0];
	const double wy = static_cast<double>(t[1]) + cy;
	const float wz = static_cast<float>(static_cast<double>(t[2]) + cz);

	// R * F, 0x1807f-0x181e6; F[k][c] is frame[3 * k + c].
	const float* f = reinterpret_cast<const float*>(record + 0xdc);
	float* m = reinterpret_cast<float*>(record + 0x134);
	#define NX_RF(a, b) (static_cast<double>(r[a]) * f[b])
	m[0] = static_cast<float>((NX_RF(0, 0) + NX_RF(1, 3)) + NX_RF(2, 6));
	m[1] = static_cast<float>((NX_RF(1, 4) + NX_RF(2, 7)) + NX_RF(0, 1));
	m[2] = static_cast<float>((NX_RF(0, 2) + NX_RF(1, 5)) + NX_RF(2, 8));
	m[3] = static_cast<float>((NX_RF(4, 3) + NX_RF(5, 6)) + NX_RF(3, 0));
	m[4] = static_cast<float>((NX_RF(3, 1) + NX_RF(4, 4)) + NX_RF(5, 7));
	m[5] = static_cast<float>((NX_RF(3, 2) + NX_RF(4, 5)) + NX_RF(5, 8));
	m[6] = static_cast<float>((NX_RF(7, 3) + NX_RF(8, 6)) + NX_RF(6, 0));
	m[7] = static_cast<float>((NX_RF(6, 1) + NX_RF(7, 4)) + NX_RF(8, 7));
	m[8] = static_cast<float>((NX_RF(6, 2) + NX_RF(7, 5)) + NX_RF(8, 8));
	#undef NX_RF
	float* centre = reinterpret_cast<float*>(record + 0x158);
	centre[0] = static_cast<float>(wx);
	centre[1] = static_cast<float>(wy);
	centre[2] = wz;

	nxNpActorBodyQuaternionFromMatrix(m, reinterpret_cast<float*>(record + 0x124));
	nxNpActorWorldTensorRDRt(reinterpret_cast<const float*>(record + 0xc4), m,
		reinterpret_cast<float*>(record + 0x164));
	}

// phys_fn_000756 (0x00017420, 525 B)
// A row of gap:SceneRaycast.cpp..CapsuleShape.cpp (thiscall on the record):
// the quaternion of the mass-frame world rotation +0x134 into +0x124, by the
// 000801 conversion (the instruction sequences are the same; checked by
// listing, Task 3 of the NpActor.cpp completion plan).
static inline void nxNpActorUpdateCMassQuaternion(unsigned char* record)
	{
	nxNpActorBodyQuaternionFromMatrix(
		reinterpret_cast<const float*>(record + 0x134),
		reinterpret_cast<float*>(record + 0x124));
	}

#endif
