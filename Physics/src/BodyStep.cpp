/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The per-body step math and the continuous-collision (CCD) pose rows of the
// gap unit SceneRaycast.cpp..CapsuleShape.cpp (scene-raycast Task 4, island
// sub-area): 000710 000726 000732 000734 000736 000770 (body math) and 000740
// 000772 000774 (CCD; 000738, the reader of 000740's box, is
// Row000738Fixture in core/JointSupport.cpp). Declarations and callers in
// Physics/src/include/BodyStep.h.
//
// Record fields, by offset (the listing's use): +0x34 linear and +0x40
// angular velocity; +0x4c wake counter; +0x88 gravity, +0x94 accumulated
// angular term, +0xa0 force, +0xac torque; +0xb8 / +0xbc linear / angular
// damping; +0xc4 inverse inertia diagonal; +0xd0 / +0xd4 sleep thresholds;
// +0xd8 maximum squared angular velocity; +0x10c body flags (bit 0 disable
// gravity, bits 1-6 frozen axes, bit 7 kinematic); +0x118 kinematic target;
// +0x124 quaternion, +0x134 3x3 and +0x158 centre of mass (the world mass
// frame); +0x164 world inverse inertia; +0x19c the actor body; +0x1a0 /
// +0x1ac saved velocities; +0x1e4 flags (bit 5 saved velocities valid, bit 9
// swept box valid); +0x20c..+0x238 the CCD start pose; +0x23c / +0x240 the
// earliest time of impact and a count; +0x244..+0x258 the swept box; +0x25c
// the contact-pair count. The actor body's +4 is the internal Scene, whose
// +0x548 / +0x54c / +0x550 / +0x554 / +0x558 these rows read.
//
// Precision: every row here runs inside the simulation step (control word
// 0x0f7f; 000772/000774 only from the CCD sweep 002264, which the step
// reaches too). As in core/JointSupport.cpp, a value the listing keeps on
// the x87 stack is a `double`, a value it stores is an `NxReal`, the
// listing's operand grouping is kept, and fsqrt / fsin / fcos / the CRT's
// _CIacos go through the X87Sqrt.h helpers.

#include "BodyStep.h"
#include "core/JointSupport.h"
#include "NpActorDynamicMath.h"
#include "PhysicsSDK.h"
#include "PhysicsInternal.h"
#include "X87Sqrt.h"

#include <math.h>

static NX_INLINE NxU8* stepBytes(void* record)
	{
	return static_cast<NxU8*>(record);
	}

static NX_INLINE NxU32& stepWord(void* record, NxU32 offset)
	{
	return *reinterpret_cast<NxU32*>(stepBytes(record) + offset);
	}

static NX_INLINE void*& stepPointer(void* record, NxU32 offset)
	{
	return *reinterpret_cast<void**>(stepBytes(record) + offset);
	}

static NX_INLINE NxReal& stepReal(void* record, NxU32 offset)
	{
	return *reinterpret_cast<NxReal*>(stepBytes(record) + offset);
	}

// The internal Scene of the record's actor body: `mov eax,[rec+0x19c]; mov
// ecx,[eax+4]`.
static NX_INLINE void* stepScene(void* record)
	{
	return stepPointer(stepPointer(record, 0x19c), 4);
	}

// .rdata constants the rows read from memory.
static const NxReal gStepZero = 0.0f;				// 0x101041f0
static const NxReal gStepOne = 1.0f;				// 0x101041ec
static const NxReal gStepHalf = 0.5f;				// 0x101043cc
static const NxReal gStepEpsilon = 1e-6f;			// 0x10106880 (0x358637bd)
static const NxReal gStepMinusOne = -1.0f;			// 0x1010687c
static const NxReal gStepPi = 3.14159274f;			// 0x10106870 (0x40490fdb)

// .data 0x10123c00: a float zero in the image that 000726 reads (0x16552)
// and no instruction writes (no other reference in the listing; the image's
// relocations name it only there). While it is 0 a dynamic body's integrated
// impulse is not divided by its contact-pair count. Kept as data so the read
// is not folded away.
NxReal gBodyStepPairDivide = 0.0f;

// phys_fn_000710 (0x00015cb0, 122 B)
// Byte +0x10c bit 0 clear: +0x88..+0x90 = *gravity (dword moves); set: 0.
// Then the nine words +0x94..+0xb4 are zeroed.
void Row000710Fixture::row000710(const NxVec3* gravity)
	{
	if(!(*(stepBytes(this) + 0x10c) & 1))
		{
		const NxU32* from = reinterpret_cast<const NxU32*>(gravity);
		stepWord(this, 0x88) = from[0];
		stepWord(this, 0x8c) = from[1];
		stepWord(this, 0x90) = from[2];
		}
	else
		{
		stepWord(this, 0x88) = 0;
		stepWord(this, 0x8c) = 0;
		stepWord(this, 0x90) = 0;
		}
	for(NxU32 offset = 0x94; offset <= 0xb4; offset += 4)
		stepWord(this, offset) = 0;
	}

// phys_fn_000726 (0x00016260, 1371 B)
// Kinematic (byte +0x10c bit 7), 0x16274-0x1653e. S = Scene+0x554, T = the
// target record +0x118:
// - T+0xc bit 0: v = (T - c) S invDt with c = +0x158; the x and y
//   differences stay in registers and each is scaled by S and stored, the z
//   difference is stored first and its S product kept (0x16293-0x162ef);
// - T+0xc bit 1: r = t q* with q = +0x124 (x, y, z negated, w as is: -x
//   stored, -y and -z kept) and t = T+0x10..+0x1c, each component a
//   four-term sum in the listing's order, stored (0x16302-0x163b6); r is
//   negated when r.w is ordered below 0; normalised by 1 / |r| unless |r| is
//   exactly 0 (a NaN is normalised); then, only when |r.w - 1| is ordered
//   above 1e-6f: angle = 0 for r.w >= 1, pi for r.w <= -1, else _CIacos(r.w)
//   (005697); k = (angle invDt + angle invDt) / sqrt(1 - r.w r.w); x and y of
//   r k stored, z kept; w = those times S (0x16479-0x1650c);
// - then, when Scene+0x550 == Scene+0x558 + 1, T+0xc = 0.
// Dynamic, 0x16541-0x167b8: scale = 1 / (float)(unsigned)+0x25c when that is
// above 1 and the .data float 0x10123c00 is not 0, else 1 (kept);
// v += scale ((force S) + (gravity dt)) and w += (torque S) + (the +0x94
// term dt) with the listing's spills (below); then the damping: a linear
// factor dt +0xb8 ordered below 1 scales v by 1 - it, else v = 0; the angular
// factor dt +0xbc is stored first (over the dt argument) and does the same for
// w; finally |w|^2 = (x x + y y) + z z ordered above +0xd8 scales w by
// sqrt(+0xd8 / |w|^2).
void Row000726Fixture::row000726(NxReal dt, NxReal invDt)
	{
	if(*(stepBytes(this) + 0x10c) & 0x80)
		{
		void* scene = stepScene(this);
		const NxReal S = stepReal(scene, 0x554);
		const NxU8* target = static_cast<const NxU8*>(stepPointer(this, 0x118));
		if(*(target + 0xc) & 1)
			{
			const NxReal* goal = reinterpret_cast<const NxReal*>(target);
			const double dx = (double)goal[0] - stepReal(this, 0x158);
			const double dy = (double)goal[1] - stepReal(this, 0x15c);
			const NxReal dz = (NxReal)((double)goal[2] - stepReal(this, 0x160));
			const NxReal sx = (NxReal)(dx * S);
			const NxReal sy = (NxReal)(dy * S);
			const double sz = (double)dz * S;
			const double vx = (double)sx * invDt;
			const double vy = (double)sy * invDt;
			stepReal(this, 0x3c) = (NxReal)(sz * invDt);
			stepReal(this, 0x34) = (NxReal)vx;
			stepReal(this, 0x38) = (NxReal)vy;
			}
		target = static_cast<const NxU8*>(stepPointer(this, 0x118));
		if(*(target + 0xc) & 2)
			{
			const NxReal* t = reinterpret_cast<const NxReal*>(target + 0x10);
			const NxReal a = -stepReal(this, 0x124);
			const double b = -(double)stepReal(this, 0x128);
			const double c = -(double)stepReal(this, 0x12c);
			const NxReal w = stepReal(this, 0x130);
			NxReal rw = (NxReal)((((double)w * t[3] - (double)a * t[0]) - b * t[1]) - c * t[2]);
			NxReal rx = (NxReal)((((c * t[1] + (double)w * t[0]) + (double)a * t[3])) - b * t[2]);
			NxReal ry = (NxReal)((((double)a * t[2] + (double)w * t[1]) + b * t[3]) - c * t[0]);
			NxReal rz = (NxReal)((((double)w * t[2] + c * t[3]) + b * t[0]) - (double)a * t[1]);
			if(rw < gStepZero)
				{
				rx = -rx;
				ry = -ry;
				rz = -rz;
				rw = -rw;
				}
			const double length = x87FsqrtDot4(rx, rx, ry, ry, rz, rz, rw, rw);
			if(length != gStepZero)
				{
				const double inverse = gStepOne / length;
				rx = (NxReal)(rx * inverse);
				ry = (NxReal)(ry * inverse);
				rz = (NxReal)(rz * inverse);
				rw = (NxReal)(inverse * rw);
				}
			if(fabs((double)rw - gStepOne) > gStepEpsilon)
				{
				// k = (angle invDt + angle invDt) / sqrt(1 - rw rw), formed in
				// one helper so the dividend (and the arc cosine) stays in a
				// register across the root, as in the listing.
				double k;
				if(rw >= gStepOne)
					k = x87RateOverRoot(gStepZero, invDt, rw);
				else if(rw <= gStepMinusOne)
					k = x87RateOverRoot(gStepPi, invDt, rw);
				else
					k = x87AcosRateOverRoot(rw, invDt);
				const NxReal kx = (NxReal)(rx * k);
				const NxReal ky = (NxReal)(ry * k);
				const double kz = k * rz;
				const double wx = (double)kx * S;
				const double wy = (double)ky * S;
				stepReal(this, 0x48) = (NxReal)(kz * S);
				stepReal(this, 0x40) = (NxReal)wx;
				stepReal(this, 0x44) = (NxReal)wy;
				}
			}
		scene = stepScene(this);
		if(stepWord(scene, 0x550) == stepWord(scene, 0x558) + 1)
			stepWord(stepPointer(this, 0x118), 0xc) = 0;
		return;
		}

	const NxU32 pairs = stepWord(this, 0x25c);
	double scale;
	if(pairs > 1 && !(gBodyStepPairDivide == gStepZero))
		scale = gStepOne / (double)pairs;
	else
		scale = gStepOne;
	const NxReal S = stepReal(stepScene(this), 0x554);

	// Linear, 0x16587-0x1662d.
	const double gx = (double)dt * stepReal(this, 0x88);
	const double gy = (double)dt * stepReal(this, 0x8c);
	const NxReal gz = (NxReal)((double)dt * stepReal(this, 0x90));
	const double fx = (double)S * stepReal(this, 0xa0);
	const NxReal fy = (NxReal)((double)S * stepReal(this, 0xa4));
	const NxReal fz = (NxReal)((double)S * stepReal(this, 0xa8));
	const NxReal sumX = (NxReal)(fx + gx);
	const double sumY = (double)fy + gy;
	const NxReal sumZ = (NxReal)((double)fz + gz);
	const NxReal ix = (NxReal)(sumX * scale);
	const NxReal iy = (NxReal)(sumY * scale);
	const double iz = scale * sumZ;
	const double vx = (double)ix + stepReal(this, 0x34);
	const double vy = (double)iy + stepReal(this, 0x38);
	stepReal(this, 0x3c) = (NxReal)(iz + stepReal(this, 0x3c));
	stepReal(this, 0x34) = (NxReal)vx;
	stepReal(this, 0x38) = (NxReal)vy;

	// Angular, 0x16630-0x166b7.
	const double ax = (double)dt * stepReal(this, 0x94);
	const NxReal ay = (NxReal)((double)dt * stepReal(this, 0x98));
	const NxReal az = (NxReal)((double)dt * stepReal(this, 0x9c));
	const double tx = (double)S * stepReal(this, 0xac);
	const double ty = (double)S * stepReal(this, 0xb0);
	const NxReal tz = (NxReal)((double)S * stepReal(this, 0xb4));
	const NxReal ux = (NxReal)(tx + stepReal(this, 0x40));
	const NxReal uy = (NxReal)(ty + stepReal(this, 0x44));
	const NxReal uz = (NxReal)((double)tz + stepReal(this, 0x48));
	const double wx = ax + ux;
	const double wy = (double)uy + ay;
	stepReal(this, 0x48) = (NxReal)((double)uz + az);
	stepReal(this, 0x40) = (NxReal)wx;
	stepReal(this, 0x44) = (NxReal)wy;

	// Damping, 0x166ba-0x16754. `fcom 1.0f; test ah,5; jp`: only a factor
	// ordered below 1 damps; 1 or more, or a NaN, zeroes.
	const double linearDamping = (double)dt * stepReal(this, 0xb8);
	const NxReal angularDamping = (NxReal)((double)dt * stepReal(this, 0xbc));
	if(linearDamping < gStepOne)
		{
		const double keep = gStepOne - linearDamping;
		const double dvx = keep * stepReal(this, 0x34);
		const double dvy = keep * stepReal(this, 0x38);
		stepReal(this, 0x3c) = (NxReal)(keep * stepReal(this, 0x3c));
		stepReal(this, 0x34) = (NxReal)dvx;
		stepReal(this, 0x38) = (NxReal)dvy;
		}
	else
		{
		stepWord(this, 0x34) = 0;
		stepWord(this, 0x38) = 0;
		stepWord(this, 0x3c) = 0;
		}
	if(angularDamping < gStepOne)
		{
		const double keep = (double)gStepOne - angularDamping;
		const double dwx = keep * stepReal(this, 0x40);
		const double dwy = keep * stepReal(this, 0x44);
		const NxReal dwz = (NxReal)(keep * stepReal(this, 0x48));
		stepReal(this, 0x40) = (NxReal)dwx;
		stepReal(this, 0x44) = (NxReal)dwy;
		stepReal(this, 0x48) = dwz;
		}
	else
		{
		stepWord(this, 0x40) = 0;
		stepWord(this, 0x44) = 0;
		stepWord(this, 0x48) = 0;
		}

	// The clamp, 0x16757-0x167a8: `fcom [+0xd8]; test ah,0x41; jne` clamps
	// only a |w|^2 ordered above the maximum.
	const NxReal* w = &stepReal(this, 0x40);
	const double w2 = ((double)w[0] * w[0] + (double)w[1] * w[1]) + (double)w[2] * w[2];
	if(w2 > stepReal(this, 0xd8))
		{
		const double k = x87FsqrtQuotDot3(stepReal(this, 0xd8), w[0], w[0], w[1], w[1], w[2], w[2]);
		const double kx = k * stepReal(this, 0x40);
		const double ky = k * stepReal(this, 0x44);
		stepReal(this, 0x48) = (NxReal)(k * stepReal(this, 0x48));
		stepReal(this, 0x40) = (NxReal)kx;
		stepReal(this, 0x44) = (NxReal)ky;
		}
	}

// phys_fn_000732 (0x00016860, 399 B)
// 000714(dt), then on the +0x4c bit pattern (an integer test: -0.0f counts
// as awake):
// - 0: the saved +0x1a0..+0x1b4 and the linear +0x34..+0x3c are zeroed;
// - else: +0x34..+0x48 are saved to +0x1a0..+0x1b4 unless +0x1e4 bit 5 is set
//   on a body that is not kinematic; +0x1e4 bit 5 is cleared; with any of the
//   +0x10c frozen bits 1-6, the velocities are copied out (five words, and the
//   angular z through the FPU), each frozen axis zeroes its saved word and its
//   copy (bit 6 reloads 0.0f from 0x101041f0), and the copy is written back.
// Finally a kinematic body (+0x10c bit 7, re-read) gets +0x34..+0x48 = 0.
void Row000732Fixture::row000732(NxReal dt, NxReal /*unused*/)
	{
	reinterpret_cast<Row000714Fixture*>(this)->row000714(dt);
	if(stepWord(this, 0x4c) == 0)
		{
		stepWord(this, 0x1a8) = 0;
		stepWord(this, 0x1a4) = 0;
		stepWord(this, 0x1a0) = 0;
		stepWord(this, 0x1b4) = 0;
		stepWord(this, 0x1b0) = 0;
		stepWord(this, 0x1ac) = 0;
		stepWord(this, 0x34) = 0;
		stepWord(this, 0x38) = 0;
		stepWord(this, 0x3c) = 0;
		}
	else
		{
		if(!(*(stepBytes(this) + 0x1e4) & 0x20) || (*(stepBytes(this) + 0x10c) & 0x80))
			{
			for(NxU32 offset = 0; offset < 0x18; offset += 4)
				stepWord(this, 0x1a0 + offset) = stepWord(this, 0x34 + offset);
			}
		stepWord(this, 0x1e4) &= ~0x20u;
		const NxU32 flags = stepWord(this, 0x10c);
		if(flags & 0x7e)
			{
			NxReal wz = stepReal(this, 0x48);
			NxU32 copy[5];
			copy[0] = stepWord(this, 0x34);
			copy[1] = stepWord(this, 0x38);
			copy[2] = stepWord(this, 0x3c);
			copy[3] = stepWord(this, 0x40);
			copy[4] = stepWord(this, 0x44);
			if(flags & 0x02)
				{
				stepWord(this, 0x1a0) = 0;
				copy[0] = 0;
				}
			if(flags & 0x04)
				{
				stepWord(this, 0x1a4) = 0;
				copy[1] = 0;
				}
			if(flags & 0x08)
				{
				stepWord(this, 0x1a8) = 0;
				copy[2] = 0;
				}
			if(flags & 0x10)
				{
				stepWord(this, 0x1ac) = 0;
				copy[3] = 0;
				}
			if(flags & 0x20)
				{
				stepWord(this, 0x1b0) = 0;
				copy[4] = 0;
				}
			if(flags & 0x40)
				{
				stepWord(this, 0x1b4) = 0;
				wz = gStepZero;
				}
			stepWord(this, 0x34) = copy[0];
			stepWord(this, 0x38) = copy[1];
			stepWord(this, 0x3c) = copy[2];
			stepReal(this, 0x48) = wz;
			stepWord(this, 0x40) = copy[3];
			stepWord(this, 0x44) = copy[4];
			}
		}
	if(*(stepBytes(this) + 0x10c) & 0x80)
		{
		stepWord(this, 0x34) = 0;
		stepWord(this, 0x38) = 0;
		stepWord(this, 0x3c) = 0;
		stepWord(this, 0x40) = 0;
		stepWord(this, 0x44) = 0;
		stepWord(this, 0x48) = 0;
		}
	}

// phys_fn_000734 (0x000169f0, 101 B)
// +0x158 += dt * +0x1a0: the x product stays in a register, the y and z
// products are spilled to floats; the z sum is spilled and stored by `mov`.
__declspec(noinline) void Row000734Fixture::row000734(NxReal dt)
	{
	const double px = (double)dt * stepReal(this, 0x1a0);
	const NxReal py = (NxReal)((double)dt * stepReal(this, 0x1a4));
	const NxReal pz = (NxReal)((double)dt * stepReal(this, 0x1a8));
	const double cx = px + stepReal(this, 0x158);
	const double cy = (double)py + stepReal(this, 0x15c);
	stepReal(this, 0x160) = (NxReal)((double)pz + stepReal(this, 0x160));
	stepReal(this, 0x158) = (NxReal)cx;
	stepReal(this, 0x15c) = (NxReal)cy;
	}

// phys_fn_000736 (0x00016a60, 406 B)
// L^2 = (x x + y y) + z z of the saved angular velocity +0x1ac; exactly 0
// (`fucompp; test ah,0x44; jnp`) returns 0. Otherwise a = (dt L) 0.5f,
// k = fsin(a) / L and c = fcos(a) stay in registers (L and a are formed
// inside the two X87Sqrt.h helpers, so neither crosses a call), k w is spilled to
// floats, and q = (k w, c) q with each component a four-term sum in the
// listing's order, from the original q (the stores are interleaved after
// the last reads, 0x16b57-0x16b76); x, y, z are rounded to floats and w is
// stored and reloaded. |q| = sqrt(((w w + z z) + y y) + x x); exactly 0
// returns 1 unnormalised, otherwise q *= 1 / |q| (w as `inverse * w`) and 1.
__declspec(noinline) NxU32 Row000736Fixture::row000736(NxReal* q, NxReal dt)
	{
	const NxReal* w = &stepReal(this, 0x1ac);
	const double l2 = ((double)w[0] * w[0] + (double)w[1] * w[1]) + (double)w[2] * w[2];
	if(l2 == gStepZero)
		return 0;
	const double k = x87FsinHalfOverNorm3(dt, gStepHalf, w[0], w[1], w[2]);
	const NxReal kx = (NxReal)(k * w[0]);
	const NxReal ky = (NxReal)(k * w[1]);
	const NxReal kz = (NxReal)(k * w[2]);
	const double c = x87FcosHalfNorm3(dt, gStepHalf, w[0], w[1], w[2]);
	const double qx = q[0];
	const double qy = q[1];
	const double qz = q[2];
	const double qw = q[3];
	const NxReal nx = (NxReal)(((c * qx + (double)ky * qz) + (double)kx * qw) - (double)kz * qy);
	const NxReal ny = (NxReal)(((c * qy + (double)kz * qx) + (double)ky * qw) - (double)kx * qz);
	const NxReal nz = (NxReal)((((double)kx * qy + c * qz) + (double)kz * qw) - (double)ky * qx);
	q[3] = (NxReal)(((c * qw - (double)kx * qx) - (double)ky * qy) - (double)kz * qz);
	q[0] = nx;
	q[1] = ny;
	q[2] = nz;
	const NxReal nw = q[3];
	const double norm = x87FsqrtDot4(nw, nw, nz, nz, ny, ny, nx, nx);
	if(norm == gStepZero)
		return 1;
	const double inverse = gStepOne / norm;
	q[0] = (NxReal)(nx * inverse);
	q[1] = (NxReal)(ny * inverse);
	q[2] = (NxReal)(nz * inverse);
	q[3] = (NxReal)(inverse * q[3]);
	return 1;
	}

// phys_fn_000740 (0x00016c20, 423 B)
// +0x1e4 |= 0x200 (bit 9: the box is valid). With no object at the actor
// body's +0x10 the bit is cleared again (from the value just written) and
// nothing else happens. Otherwise: d = dt * +0x1a0 (three floats), then the
// object's slot 10 fills a sphere (centre, radius); R = |centre - c| +
// radius with c = +0x158 (the differences and the root in registers); the box
// is min = c - R (+0x244..+0x24c), max = c + R (+0x250..+0x258), and per
// axis d is added to max when it is ordered above 0 (`fcomp 0; test
// ah,0x41; jne`), else to min.
__declspec(noinline) void Row000740Fixture::row000740(NxReal dt)
	{
	const NxU32 bits = stepWord(this, 0x1e4) | 0x200;
	stepWord(this, 0x1e4) = bits;
	Row000740Target* object = static_cast<Row000740Target*>(stepPointer(stepPointer(this, 0x19c), 0x10));
	if(!object)
		{
		stepWord(this, 0x1e4) = bits & ~0x200u;
		return;
		}
	NxReal sweep[3];
	sweep[0] = (NxReal)((double)dt * stepReal(this, 0x1a0));
	sweep[1] = (NxReal)((double)dt * stepReal(this, 0x1a4));
	sweep[2] = (NxReal)((double)dt * stepReal(this, 0x1a8));
	NxReal sphere[4];
	object->slot10(sphere);
	const double dx = (double)sphere[0] - stepReal(this, 0x158);
	const double dy = (double)sphere[1] - stepReal(this, 0x15c);
	const double dz = (double)sphere[2] - stepReal(this, 0x160);
	const double radius = x87FsqrtDot3(dz, dz, dy, dy, dx, dx) + sphere[3];
	const double minX = (double)stepReal(this, 0x158) - radius;
	const double minY = (double)stepReal(this, 0x15c) - radius;
	stepReal(this, 0x24c) = (NxReal)((double)stepReal(this, 0x160) - radius);
	stepReal(this, 0x244) = (NxReal)minX;
	stepReal(this, 0x248) = (NxReal)minY;
	const double maxX = radius + stepReal(this, 0x158);
	const double maxY = radius + stepReal(this, 0x15c);
	stepReal(this, 0x258) = (NxReal)(radius + stepReal(this, 0x160));
	stepReal(this, 0x250) = (NxReal)maxX;
	stepReal(this, 0x254) = (NxReal)maxY;
	for(NxU32 axis = 0; axis < 3; ++axis)
		{
		if(sweep[axis] > gStepZero)
			stepReal(this, 0x250 + axis * 4) = (NxReal)((double)sweep[axis] + stepReal(this, 0x250 + axis * 4));
		else
			stepReal(this, 0x244 + axis * 4) = (NxReal)((double)sweep[axis] + stepReal(this, 0x244 + axis * 4));
		}
	}

// phys_fn_000770 (0x000183a0, 205 B)
// +0x23c = FLT_MAX (0x7f7fffff), +0x240 = 0; the 3x3 +0x134 (rep movsd, nine
// words) and +0x158 to +0x20c..+0x238. When the +0x4c bit pattern is not 0
// (an integer test): 000734(dt), 000736(+0x124, dt), 000758, the world
// inverse inertia 000746(+0xc4, +0x134, +0x164) (cdecl), and when the SDK's
// NX_CONTINUOUS_CD (000429 on [0x10123c04], `fucompp` against 0: a NaN
// counts as set) is not 0, 000740(dt) and return. Otherwise +0x1e4 bit 9 is
// cleared.
void Row000770Fixture::row000770(NxReal dt, NxReal /*unused*/)
	{
	stepWord(this, 0x23c) = 0x7f7fffff;
	stepWord(this, 0x240) = 0;
	for(NxU32 offset = 0; offset < 0x24; offset += 4)
		stepWord(this, 0x20c + offset) = stepWord(this, 0x134 + offset);
	stepWord(this, 0x230) = stepWord(this, 0x158);
	stepWord(this, 0x234) = stepWord(this, 0x15c);
	stepWord(this, 0x238) = stepWord(this, 0x160);
	if(stepWord(this, 0x4c) != 0)
		{
		reinterpret_cast<Row000734Fixture*>(this)->row000734(dt);
		reinterpret_cast<Row000736Fixture*>(this)->row000736(&stepReal(this, 0x124), dt);
		reinterpret_cast<Row000758Fixture*>(this)->row000758();
		nxNpActorWorldTensorRDRt(&stepReal(this, 0xc4), &stepReal(this, 0x134), &stepReal(this, 0x164));
		if(PhysicsSDK::instance->getParameter(NX_CONTINUOUS_CD) != gStepZero)
			{
			reinterpret_cast<Row000740Fixture*>(this)->row000740(dt);
			return;
			}
		}
	stepWord(this, 0x1e4) &= ~0x200u;
	}

// phys_fn_000772 (0x00018470, 207 B)
// Only when toi is ordered below +0x23c (`fcomp; test ah,5; jp`): +0x23c =
// toi, +++0x240, +0x158 and the 3x3 +0x134 restored from +0x230 / +0x20c,
// the quaternion +0x124 recomputed from the 3x3 (000756), then with
// t = (float)(toi * Scene+0x548) (spilled over the argument): 000734(t),
// 000736(+0x124, t), 000758, 000746(+0xc4, +0x134, +0x164); returns 1.
// Otherwise 0.
__declspec(noinline) bool Row000772Fixture::row000772(NxReal toi)
	{
	if(!(toi < stepReal(this, 0x23c)))
		return false;
	stepReal(this, 0x23c) = toi;
	++stepWord(this, 0x240);
	stepWord(this, 0x158) = stepWord(this, 0x230);
	stepWord(this, 0x15c) = stepWord(this, 0x234);
	stepWord(this, 0x160) = stepWord(this, 0x238);
	for(NxU32 offset = 0; offset < 0x24; offset += 4)
		stepWord(this, 0x134 + offset) = stepWord(this, 0x20c + offset);
	nxNpActorUpdateCMassQuaternion(stepBytes(this));
	const NxReal t = (NxReal)((double)toi * stepReal(stepScene(this), 0x548));
	reinterpret_cast<Row000734Fixture*>(this)->row000734(t);
	reinterpret_cast<Row000736Fixture*>(this)->row000736(&stepReal(this, 0x124), t);
	reinterpret_cast<Row000758Fixture*>(this)->row000758();
	nxNpActorWorldTensorRDRt(&stepReal(this, 0xc4), &stepReal(this, 0x134), &stepReal(this, 0x164));
	return true;
	}

// phys_fn_000774 (0x00018540, 34 B)
// 000772(toi); when it returns true, 000022 on the actor body (+0x19c) with
// 0 (the pose notification).
void Row000774Fixture::row000774(NxReal toi)
	{
	if(reinterpret_cast<Row000772Fixture*>(this)->row000772(toi))
		static_cast<Row000022Fixture*>(stepPointer(this, 0x19c))->row000022(0);
	}
