/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/JointSupport.h"
#include "core/Joint.h"
#include "ContactPairManager.h"
#include "Scene.h"
#include "PhysicsInternal.h"
#include "X87Sqrt.h"

#include <math.h>
#include <string.h>

// Rows phys_fn_004389/004391/004393 are not Joint or RevoluteJoint members: they
// run on JointSupportRecord (see core/JointSupport.h and revolute-contract.md
// "## Row assignment"). The body-record rows below (000022, 000712, 000758,
// 000760, 000778) belong to gap units outside the joint code; they are
// written here because the joint code and the Scene's joint removal reach
// them (joint-open-items Task 2, units/joint-open-items-contract.md
// "## Scene joint rows"). 000015 and 000017, the actor body's shape readers,
// are the core dump's (effector-and-coredump Task 3b, units/effector-
// coredump-contract.md "### Readers and whether the candidate has them"); they
// are members of the body view JointActorBody (core/Joint.h). 000754 is now
// written below (Task 6, 21b275d);
// 004167 remains a deferred stub. 000713 (written by effector-and-coredump
// Task 2) and 000791 (written from the listing in scene-raycast Task 4, the one
// definition since the second merge of main; it calls NpActor.cpp's 000782)
// are the spring-and-damper solver slot's, and 000722 the body constructor's
// (effector-and-coredump Task 2, units/effector-coredump-contract.md
// "### Solver slots" and "### Task 2 record").
//
// Precision: as in core/Joint.cpp, a value the listing keeps on the x87
// stack is a `double` here and a value it stores is an `NxReal`, with the
// listing's operand grouping kept.

// .data 0x10122054 / 0x10122060 / 0x1012206c (see core/JointSupport.h).
NxVec3 gJointUnitAxis[3] =
	{
	NxVec3(1.0f, 0.0f, 0.0f),
	NxVec3(0.0f, 1.0f, 0.0f),
	NxVec3(0.0f, 0.0f, 1.0f)
	};

// .data 0x10123c1c (see core/JointSupport.h).
NxVec3 gJointZeroVector(0.0f, 0.0f, 0.0f);

static NX_INLINE double supportMul(NxReal a, NxReal b)
	{
	return (double)a * (double)b;
	}

static NX_INLINE NxReal supportFloat(NxU32 bits)
	{
	NxReal value;
	memcpy(&value, &bits, sizeof(value));
	return value;
	}

static NX_INLINE NxU32 supportBits(NxReal value)
	{
	NxU32 bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

// phys_fn_004403 (0x000affe0), normal contact row. The listing first forms
// the relative support velocity, scales its target error by the effective
// inverse mass prepared by 004391, accumulates/clamps the nonnegative normal
// impulse, then applies only the change to each body's linear and angular
// support velocities. The patch callback is the final-iteration normal-force
// accumulator used by 000879's friction rows.
static void supportSolveNormal004403(NxReal step, NxU32 pass, JointSupportRecord* record)
	{
	// 004403 keeps the contact relative-velocity dot product on the x87 stack
	// through the target subtraction and effective-mass multiply. Calling
	// 004389 here materializes its NxF64 result as a 64-bit double first; that
	// loses precision before the applied impulse is rounded to NxReal.
	const JointSupportBody* const body0 = record->mBody[0];
	const JointSupportBody* const body1 = record->mBody[1];
	const double lambda = ((double)record->mUnknown038
		- (body0 && !body1
			? (((((double)body0->mUnknown010.z * record->mUnknown018.z
				+ (double)body0->mUnknown010.y * record->mUnknown018.y)
				+ (double)body0->mUnknown000.z * record->mUnknown000.z)
				+ (double)body0->mUnknown000.y * record->mUnknown000.y)
				+ (double)body0->mUnknown000.x * record->mUnknown000.x
				+ (double)body0->mUnknown010.x * record->mUnknown018.x)
			: record->row004389())) * record->mUnknown03c;
	NxReal rawAccumulation = (NxReal)((double)supportFloat(record->mUnknown04c) + lambda);
	record->mUnknown04c = supportBits(rawAccumulation);
	const double bias = (double)record->mUnknown040 * record->mUnknown034;
	NxReal applied = (NxReal)(lambda - bias);
	const NxReal previous = supportFloat(record->mUnknown044);
	const NxReal accumulated = (NxReal)((double)applied + previous);
	if(accumulated <= 0.0f)
		{
		applied = -previous;
		record->mUnknown044 = supportBits(0.0f);
		}
	else
		record->mUnknown044 = supportBits(accumulated);

	auto apply = [&](JointSupportBody* body, const NxVec3& angularJacobian, NxReal sign)
		{
		if(!body)
			return;
		// Keep each body's x87 operation order from 004403: body zero projects
		// before inverse-mass scaling; body one rounds its X projection first.
		const NxReal signedApplied = sign < 0.0f ? -applied : applied;
		const double scaledImpulse = (double)signedApplied * body->mUnknown00c;
		double linearX;
		if(sign < 0.0f)
			{
			const NxReal projectedX = (NxReal)((double)signedApplied
				* record->mUnknown000.x);
			linearX = (double)projectedX * body->mUnknown00c;
			}
		else
			linearX = ((double)signedApplied * record->mUnknown000.x)
				* body->mUnknown00c;
		NxReal linearY;
		NxReal linearZ;
		if(sign < 0.0f)
			{
			linearY = (NxReal)(scaledImpulse * record->mUnknown000.y);
			linearZ = (NxReal)(scaledImpulse * record->mUnknown000.z);
			}
		else
			{
			const NxReal projectedY = (NxReal)((double)signedApplied
				* record->mUnknown000.y);
			const NxReal projectedZ = (NxReal)((double)signedApplied
				* record->mUnknown000.z);
			linearY = (NxReal)((double)projectedY * body->mUnknown00c);
			linearZ = (NxReal)((double)projectedZ * body->mUnknown00c);
			}
		body->mUnknown000.x = (NxReal)((double)body->mUnknown000.x
			+ linearX);
		body->mUnknown000.y = (NxReal)((double)body->mUnknown000.y + linearY);
		body->mUnknown000.z = (NxReal)((double)body->mUnknown000.z + linearZ);

		const NxReal torque[3] = {
			(NxReal)((double)angularJacobian.x * applied * sign),
			(NxReal)((double)angularJacobian.y * applied * sign),
			(NxReal)((double)angularJacobian.z * applied * sign)
		};
		NxReal* const angularVelocity = &body->mUnknown010.x;
		for(unsigned row = 0; row != 3; ++row)
			{
			// phys_fn_004403 accumulates each inertia row from column 2 down
			// to column 0 into a float temporary before adding it to velocity.
			const NxReal delta = (NxReal)(((double)body->mUnknown020[row * 3 + 2] * torque[2]
				+ (double)body->mUnknown020[row * 3 + 1] * torque[1])
				+ (double)body->mUnknown020[row * 3] * torque[0]);
			angularVelocity[row] = (NxReal)((double)angularVelocity[row] + delta);
			}
		};
	apply(record->mBody[0], record->mUnknown018, 1.0f);
	apply(record->mBody[1], record->mUnknown024, -1.0f);
	if(pass == 1 && record->mUnknown030)
		{
		NxFrictionPatch* const patch = static_cast<NxFrictionPatch*>(record->mUnknown030);
		patch->accumulate(supportFloat(record->mUnknown044), record, step);
		}
	}

// phys_fn_004401 (0x000afcc0), the bounded tangent-friction row. The row
// accumulates the unconstrained tangent impulse, keeps it within the static
// limit, and once that limit is crossed clamps to the dynamic limit. +0x48
// and +0x4c already include the patch's normal force (000879/000861).
static void supportSolveFriction004401(NxReal step, NxU32 pass, JointSupportRecord* record)
	{
	if((record->mFlags & 0x20) && (record->mFlags & 0x1f) == 6)
		{
		void* const joint = record->mUnknown030;
		void** const vtable = *reinterpret_cast<void***>(joint);
		typedef void (__thiscall *JointFrictionCallback)(void*, NxReal);
		reinterpret_cast<JointFrictionCallback>(vtable[0])(joint, step);
		return;
		}

	const NxReal previous = supportFloat(record->mUnknown044);
	const double candidate = -((record->row004389() + record->mUnknown034)
		* record->mUnknown03c);
	const NxReal candidateFloat = (NxReal)candidate;
	const double accumulated = (double)candidateFloat + previous;
	NxReal applied;
	if(fabs(accumulated) > record->mUnknown048)
		{
		const double bounded = ((double)supportFloat(record->mUnknown04c) / fabs(accumulated)
			* record->mUnknown048) * accumulated;
		applied = (NxReal)(bounded - previous);
		record->mUnknown044 = supportBits((NxReal)bounded);
		record->mFlags |= 0x40;
		}
	else
		{
		applied = candidateFloat;
		record->mUnknown044 = supportBits((NxReal)accumulated);
		}

	if(applied != 0.0f)
		{
		auto apply = [&](JointSupportBody* body, const NxVec3& angularJacobian, NxReal sign)
			{
			if(!body)
				return;
		const NxReal signedApplied = sign < 0.0f ? -applied : applied;
		const double scaledImpulse = (double)signedApplied * body->mUnknown00c;
		const NxReal linearY = (NxReal)(scaledImpulse * record->mUnknown000.y);
		const NxReal linearZ = (NxReal)(scaledImpulse * record->mUnknown000.z);
			body->mUnknown000.x = (NxReal)((double)body->mUnknown000.x
				+ scaledImpulse * record->mUnknown000.x);
			body->mUnknown000.y = (NxReal)((double)body->mUnknown000.y + linearY);
			body->mUnknown000.z = (NxReal)((double)body->mUnknown000.z + linearZ);

			const NxReal torque[3] = {
				(NxReal)((double)angularJacobian.x * applied * sign),
				(NxReal)((double)angularJacobian.y * applied * sign),
				(NxReal)((double)angularJacobian.z * applied * sign)
			};
			NxReal delta[3];
			for(unsigned row = 0; row != 3; ++row)
				// phys_fn_004401 accumulates its inertia rows from column 2 down
				// to column 0 before storing each angular delta.
				delta[row] = (NxReal)(((double)body->mUnknown020[row * 3 + 2] * torque[2]
					+ (double)body->mUnknown020[row * 3 + 1] * torque[1])
					+ (double)body->mUnknown020[row * 3] * torque[0]);
			body->mUnknown010.x = (NxReal)((double)body->mUnknown010.x + delta[0]);
			body->mUnknown010.y = (NxReal)((double)body->mUnknown010.y + delta[1]);
			body->mUnknown010.z = (NxReal)((double)body->mUnknown010.z + delta[2]);
			};
		apply(record->mBody[0], record->mUnknown018, 1.0f);
		apply(record->mBody[1], record->mUnknown024, -1.0f);
		}

	if(pass == 1 && record->mUnknown030)
		static_cast<NxFrictionPatch*>(record->mUnknown030)->accumulate(
		supportFloat(record->mUnknown044), record, step);
	}

// phys_fn_004395 (0x000af790), shared support-impulse application for the
// bounded joint rows 004397/004399. Bit 10 selects a pure angular Jacobian;
// otherwise both the linear direction and each body's angular Jacobian are
// applied through its prepared inverse-mass/inertia record.

// The oracle calls 004395 as a thiscall on the support record and passes both
// the applied impulse and timestep on the stack (`ret 8`). Keep this as a
// member on a pointer-bit fixture so MSVC emits that ABI even though the
// helper is implemented outside JointSupportRecord's public internal layout.
struct JointSupportApplyFixture
	{
	void applyImpulse004395(NxReal impulse, NxReal step);
	};

#if defined(_MSC_VER)
__declspec(noinline)
#endif
void JointSupportApplyFixture::applyImpulse004395(NxReal impulse, NxReal step)
	{
	(void)step; // 004395 receives the timestep but never reads it.
	JointSupportRecord* const record = reinterpret_cast<JointSupportRecord*>(this);
	const bool angularOnly = (record->mFlags & 0x400) != 0;
	auto apply = [&](JointSupportBody* body, const NxVec3& angularJacobian, NxReal sign)
		{
		if(!body || body->mUnknown00c == 0.0f)
			return;
		const NxReal signedImpulse = sign < 0.0f ? -impulse : impulse;
		NxReal torque[3];
		if(angularOnly)
			{
			torque[0] = (NxReal)((double)record->mUnknown000.x * signedImpulse);
			torque[1] = (NxReal)((double)record->mUnknown000.y * signedImpulse);
			torque[2] = (NxReal)((double)record->mUnknown000.z * signedImpulse);
			}
		else
			{
			const NxReal projected[3] = {
				(NxReal)((double)signedImpulse * record->mUnknown000.x),
				(NxReal)((double)signedImpulse * record->mUnknown000.y),
				(NxReal)((double)signedImpulse * record->mUnknown000.z)
			};
			const NxReal scaledYZ[2] = {
				(NxReal)((double)projected[1] * body->mUnknown00c),
				(NxReal)((double)projected[2] * body->mUnknown00c)
			};
			body->mUnknown000.x = (NxReal)((double)body->mUnknown000.x
				+ (double)projected[0] * body->mUnknown00c);
			body->mUnknown000.y = (NxReal)((double)body->mUnknown000.y + scaledYZ[0]);
			body->mUnknown000.z = (NxReal)((double)body->mUnknown000.z + scaledYZ[1]);
			torque[0] = (NxReal)((double)angularJacobian.x * signedImpulse);
			torque[1] = (NxReal)((double)angularJacobian.y * signedImpulse);
			torque[2] = (NxReal)((double)angularJacobian.z * signedImpulse);
			}
		NxReal delta[3];
		for(unsigned row = 0; row != 3; ++row)
			// The oracle accumulates inertia rows from column 2 down to
			// column 0 in x87 precision before storing each angular delta.
			delta[row] = (NxReal)(((double)body->mUnknown020[row * 3 + 2] * torque[2]
				+ (double)body->mUnknown020[row * 3 + 1] * torque[1])
				+ (double)body->mUnknown020[row * 3] * torque[0]);
		body->mUnknown010.x = (NxReal)((double)body->mUnknown010.x + delta[0]);
		body->mUnknown010.y = (NxReal)((double)body->mUnknown010.y + delta[1]);
		body->mUnknown010.z = (NxReal)((double)body->mUnknown010.z + delta[2]);
		};
	apply(record->mBody[0], record->mUnknown018, 1.0f);
	apply(record->mBody[1], record->mUnknown024, -1.0f);
	}

// phys_fn_004399 (0x000afc10), bounded joint row selected for kind 5.
// It has the contact-friction accumulator behavior but reports the final
// accumulated impulse through the joint's slot 3 callback.
static void supportSolveJoint004399(NxReal step, NxU32 pass, JointSupportRecord* record)
	{
	if(record->mFlags & 0x20)
		{
		if((record->mFlags & 0x1f) == 6)
			{
			void* const joint = record->mUnknown030;
			void** const vtable = *reinterpret_cast<void***>(joint);
			typedef void (__thiscall *JointRowCallback)(void*, NxReal);
			reinterpret_cast<JointRowCallback>(vtable[0])(joint, step);
			}
		return;
		}

	const NxReal previous = supportFloat(record->mUnknown044);
	const NxReal candidate = (NxReal)(-((record->row004389() + record->mUnknown034)
		* record->mUnknown03c));
	const NxReal accumulated = (NxReal)((double)candidate + previous);
	NxReal applied;
	if(fabs(accumulated) > record->mUnknown048)
		{
		const double bounded = ((double)supportFloat(record->mUnknown04c) / fabs(accumulated)
			* record->mUnknown048) * accumulated;
		applied = (NxReal)(bounded - previous);
		record->mUnknown044 = supportBits((NxReal)bounded);
		record->mFlags |= 0x40;
		}
	else
		{
		applied = candidate;
		record->mUnknown044 = supportBits(accumulated);
		}
	if(applied != 0.0f)
		reinterpret_cast<JointSupportApplyFixture*>(record)->applyImpulse004395(applied, step);
	if(pass == 1 && record->mUnknown030)
		{
		void** const vtable = *reinterpret_cast<void***>(record->mUnknown030);
		typedef void (__thiscall *JointAccumulatedForceCallback)(void*, NxReal, const NxVec3&, NxReal);
		reinterpret_cast<JointAccumulatedForceCallback>(vtable[3])(record->mUnknown030,
			supportFloat(record->mUnknown044), record->mUnknown000, step);
		}
	}

// 004397 forms the force sum in one x87 lifetime: row004389's extended result
// is subtracted from the target, multiplied by the effective mass, and added
// to the prior force before the single float store. Keeping this in a helper
// prevents MSVC from spilling the intermediate lambda to a 64-bit local.
static NxReal supportJointForceSum004397(const JointSupportRecord* record, NxReal previousForce)
	{
	const double relativeVelocity = record->row004389();
	return (NxReal)((((double)record->mUnknown038 - relativeVelocity)
		* record->mUnknown03c) + previousForce);
	}

// Re-form lambda only on the unclamped path, where 004397 uses it at x87
// precision rather than the float-rounded accumulated-force sum.
static NxF64 supportJointLambda004397(const JointSupportRecord* record)
	{
	const double relativeVelocity = record->row004389();
	return ((double)record->mUnknown038 - relativeVelocity) * record->mUnknown03c;
	}

// phys_fn_004397 (0x000afae0), bounded joint support row. Kind 6 custom
// callbacks are routed through the joint's slot 0; ordinary rows form the
// relative-velocity impulse, honor unilateral bit 9, and apply the delta.
static void supportSolveJoint004397(NxReal step, NxU32 pass, JointSupportRecord* record)
	{
	const NxU32 kind = record->mFlags & 0x1f;
	if(record->mFlags & 0x20)
		{
		if(kind == 6)
			{
			void* const joint = record->mUnknown030;
			void** const vtable = *reinterpret_cast<void***>(joint);
			typedef void (__thiscall *JointRowCallback)(void*, NxReal);
			reinterpret_cast<JointRowCallback>(vtable[0])(joint, step);
			}
		return;
		}

	const NxReal previousForce = supportFloat(record->mUnknown04c);
	NxReal force = supportJointForceSum004397(record, previousForce);
	bool forceClamped = false;
	if(force < -record->mUnknown048)
		{
		if(record->mUnknown030)
			{
			void** const vtable = *reinterpret_cast<void***>(record->mUnknown030);
			typedef void (__thiscall *JointBreakCallback)(void*, const JointSupportRecord*, NxReal);
			reinterpret_cast<JointBreakCallback>(vtable[2])(record->mUnknown030, record, force);
			}
		force = -record->mUnknown048;
		forceClamped = true;
		}
	else if(force > record->mUnknown048)
		{
		if(record->mUnknown030)
			{
			void** const vtable = *reinterpret_cast<void***>(record->mUnknown030);
			typedef void (__thiscall *JointBreakCallback)(void*, const JointSupportRecord*, NxReal);
			reinterpret_cast<JointBreakCallback>(vtable[2])(record->mUnknown030, record, force);
			}
		force = record->mUnknown048;
		forceClamped = true;
		}
	record->mUnknown04c = supportBits(force);
	const double forceCorrection = (double)record->mUnknown040 * record->mUnknown034;
	NxReal applied = forceClamped
		? (NxReal)(((double)force - previousForce) - forceCorrection)
		: (NxReal)(supportJointLambda004397(record) - forceCorrection);
	const NxReal previous = supportFloat(record->mUnknown044);
	NxReal accumulated = (NxReal)((double)applied + previous);
	if(record->mFlags & 0x200)
		{
		if(accumulated < 0.0f)
			{
			applied = -previous;
			accumulated = 0.0f;
			}
		}
	record->mUnknown044 = supportBits(accumulated);
	if(applied != 0.0f)
		reinterpret_cast<JointSupportApplyFixture*>(record)->applyImpulse004395(applied, step);
	if(pass == 1 && record->mUnknown030)
		{
		void** const vtable = *reinterpret_cast<void***>(record->mUnknown030);
		typedef void (__thiscall *JointAccumulatedForceCallback)(void*, NxReal, const NxVec3&, NxReal);
		reinterpret_cast<JointAccumulatedForceCallback>(vtable[3])(record->mUnknown030,
			supportFloat(record->mUnknown044), record->mUnknown000, step);
		}
	}

// phys_fn_004174/004176 (0x0009b120/0x0009b240), the per-island solver
// wrapper. Dispatch table kinds 1/2/3/6 share 004397, kind 4 is 004401,
// kind 5 is 004399, and kind 0 is 004403.
void nxSolveJointSupportRecords(NxSceneInternal* scene, NxReal step, NxU32 iterations)
	{
	JointSupportRecord* const first = scene->at<JointSupportRecord*>(0x5b8);
	const NxU32 count = scene->at<NxU32>(0x5bc);
	for(NxU32 pass = iterations; pass != 0; --pass)
		for(NxU32 i = 0; i != count; ++i)
			{
			JointSupportRecord* record = first + i;
			const bool body0 = record->mBody[0] && pass <= record->mBody[0]->mUnknown05c;
			const bool body1 = record->mBody[1] && pass <= record->mBody[1]->mUnknown05c;
			if(!body0 && !body1)
				continue;
			const NxU32 kind = record->mFlags & 0x1f;
			if(kind == 0)
				supportSolveNormal004403(step, pass, record);
			else if(kind == 1 || kind == 2 || kind == 3 || kind == 6)
				supportSolveJoint004397(step, pass, record);
			else if(kind == 4)
				supportSolveFriction004401(step, pass, record);
			else if(kind == 5)
				supportSolveJoint004399(step, pass, record);
			}

	JointSupportBody* const bodies = scene->at<JointSupportBody*>(0x5ac);
	const NxU32 bodyCount = scene->at<NxU32>(0x5b0);
	for(NxU32 i = 0; i != bodyCount; ++i)
		{
		bodies[i].mUnknown044 = bodies[i].mUnknown000;
		bodies[i].mUnknown050 = bodies[i].mUnknown010;
		}

	for(NxU32 i = 0; i != count; ++i)
		{
		JointSupportRecord* record = first + i;
		const NxU32 kind = record->mFlags & 0x1f;
		if(kind != 0 || record->mUnknown034 <= 0.0f)
			record->mUnknown034 = 0.0f;
		if(kind == 6)
			{
			// phys_fn_004176 clears the custom row's target and dispatches
			// joint slot 1 before its final slot-0 solve (0x9b1fc-0x9b210).
			void* const joint = record->mUnknown030;
			void** const vtable = *reinterpret_cast<void***>(joint);
			typedef void (__thiscall *JointResetCallback)(void*);
			reinterpret_cast<JointResetCallback>(vtable[1])(joint);
			}
		if(kind == 0)
			supportSolveNormal004403(step, 0xffffffffu, record);
		else if(kind == 1 || kind == 2 || kind == 3 || kind == 6)
			supportSolveJoint004397(step, 0xffffffffu, record);
		else if(kind == 4)
			supportSolveFriction004401(step, 0xffffffffu, record);
		else if(kind == 5)
			supportSolveJoint004399(step, 0xffffffffu, record);
		}
	}

// phys_fn_004389 (0x000af2d0, 227 B)
// Bit 10 of mFlags clear: the dot of the record's vectors with body[0]'s
// (+0x00, +0x10) minus the same for body[1], each missing body counting 0.
// Bit 10 set: the record's +0x00 vector dotted with body[0]'s +0x10 minus
// body[1]'s +0x10 (either missing: that side alone, the listing does not
// test body[0] when body[1] is null).
NxF64 JointSupportRecord::row004389() const
	{
	if(mFlags & 0x400)
		{
		const JointSupportBody* body1 = mBody[1];
		if(!body1)
			{
			const JointSupportBody* body0 = mBody[0];
			return (supportMul(body0->mUnknown010.z, mUnknown000.z) + supportMul(body0->mUnknown010.y, mUnknown000.y))
				+ supportMul(body0->mUnknown010.x, mUnknown000.x);
			}
		const JointSupportBody* body0 = mBody[0];
		if(!body0)
			{
			return -((supportMul(body1->mUnknown010.z, mUnknown000.z) + supportMul(body1->mUnknown010.y, mUnknown000.y))
				+ supportMul(body1->mUnknown010.x, mUnknown000.x));
			}
		const double dx = (double)body0->mUnknown010.x - body1->mUnknown010.x;
		const double dy = (double)body0->mUnknown010.y - body1->mUnknown010.y;
		const double dz = (double)body0->mUnknown010.z - body1->mUnknown010.z;
		return (dz * mUnknown000.z + dy * mUnknown000.y) + dx * mUnknown000.x;
		}

	double first;
	const JointSupportBody* body0 = mBody[0];
	if(body0)
		{
		first = ((((supportMul(body0->mUnknown010.z, mUnknown018.z) + supportMul(body0->mUnknown010.y, mUnknown018.y))
			+ supportMul(body0->mUnknown000.z, mUnknown000.z))
			+ supportMul(body0->mUnknown000.y, mUnknown000.y))
			+ supportMul(body0->mUnknown000.x, mUnknown000.x))
			+ supportMul(body0->mUnknown010.x, mUnknown018.x);
		}
	else
		{
		first = 0.0f;
		}
	const JointSupportBody* body1 = mBody[1];
	if(!body1)
		return first;
	const double second = ((((supportMul(body1->mUnknown010.z, mUnknown024.z) + supportMul(body1->mUnknown010.y, mUnknown024.y))
		+ supportMul(body1->mUnknown000.z, mUnknown000.z))
		+ supportMul(body1->mUnknown000.y, mUnknown000.y))
		+ supportMul(mUnknown000.x, body1->mUnknown000.x))
		+ supportMul(mUnknown024.x, body1->mUnknown010.x);
	return first - second;
	}

// phys_fn_004391 (0x000af3c0, 837 B)
// Bit 10 of mFlags clear: for each body, w = its +0x20 3x3 times the
// record's +0x18 (body 0) or +0x24 (body 1) vector and u = the record's
// +0x00 vector times its +0x0c scale, both stored as floats; out0 = body
// 0's (w . +0x18 + u . +0x00) plus body 1's, 0 without body 0. Bit 10 set:
// e = the sum of both 3x3s times +0x00 (the first component unrounded, the
// others stored), out0 = e . +0x00. Then out1 = 1 / out0, or 0 for 0.
// Listing over decompile: the six-term sums keep the listing's order
// (w.z, w.y, u.x, u.z, u.y, w.x for body 0; w.z, w.y, w.x, u.x, u.z, u.y,
// then + out0 for body 1); in the bit-10 arm e.x is summed unrounded
// across the bodies while e.y and e.z are stored first.
void JointSupportRecord::row004391(NxReal& out0, NxReal& out1)
	{
	const NxReal vx = mUnknown000.x;
	const NxReal vy = mUnknown000.y;
	const NxReal vz = mUnknown000.z;
	if(!((mFlags >> 10) & 1))
		{
		NxVec3 w0, u0, w1, u1;
		const JointSupportBody* body0 = mBody[0];
		if(body0)
			{
			const NxReal* m = body0->mUnknown020;
			const NxVec3& a = mUnknown018;
			const double c0 = (supportMul(a.z, m[2]) + supportMul(a.y, m[1])) + supportMul(a.x, m[0]);
			const double c1 = (supportMul(a.z, m[5]) + supportMul(a.y, m[4])) + supportMul(a.x, m[3]);
			const double c2 = (supportMul(a.z, m[8]) + supportMul(a.y, m[7])) + supportMul(a.x, m[6]);
			w0.z = (NxReal)c2;
			w0.x = (NxReal)c0;
			w0.y = (NxReal)c1;
			const NxReal s = body0->mUnknown00c;
			u0.x = (NxReal)supportMul(vx, s);
			u0.y = (NxReal)supportMul(vy, s);
			u0.z = (NxReal)supportMul(vz, s);
			}
		const JointSupportBody* body1 = mBody[1];
		if(body1)
			{
			const NxReal* m = body1->mUnknown020;
			const NxVec3& b = mUnknown024;
			const double c0 = (supportMul(b.z, m[2]) + supportMul(b.y, m[1])) + supportMul(b.x, m[0]);
			const double c1 = (supportMul(b.z, m[5]) + supportMul(b.y, m[4])) + supportMul(b.x, m[3]);
			const double c2 = (supportMul(b.z, m[8]) + supportMul(b.y, m[7])) + supportMul(b.x, m[6]);
			w1.z = (NxReal)c2;
			w1.x = (NxReal)c0;
			w1.y = (NxReal)c1;
			const NxReal s = body1->mUnknown00c;
			u1.x = (NxReal)supportMul(vx, s);
			u1.y = (NxReal)supportMul(vy, s);
			u1.z = (NxReal)supportMul(vz, s);
			}
		if(body0)
			{
			const NxVec3& a = mUnknown018;
			out0 = (NxReal)(((((supportMul(w0.z, a.z) + supportMul(w0.y, a.y)) + supportMul(u0.x, vx))
				+ supportMul(u0.z, vz)) + supportMul(u0.y, vy)) + supportMul(w0.x, a.x));
			}
		else
			{
			out0 = 0.0f;
			}
		// The listing reloads +0x14 here (0xaf56c).
		if(mBody[1])
			{
			const NxVec3& b = mUnknown024;
			out0 = (NxReal)((((((supportMul(w1.z, b.z) + supportMul(w1.y, b.y)) + supportMul(w1.x, b.x))
				+ supportMul(u1.x, vx)) + supportMul(u1.z, vz)) + supportMul(u1.y, vy)) + out0);
			}
		}
	else
		{
		double e0;
		NxReal e1;
		NxReal e2;
		const JointSupportBody* body0 = mBody[0];
		if(body0)
			{
			const NxReal* m = body0->mUnknown020;
			e0 = (supportMul(vz, m[2]) + supportMul(vy, m[1])) + supportMul(vx, m[0]);
			e1 = (NxReal)((supportMul(vz, m[5]) + supportMul(vy, m[4])) + supportMul(vx, m[3]));
			e2 = (NxReal)((supportMul(vz, m[8]) + supportMul(vy, m[7])) + supportMul(vx, m[6]));
			}
		else
			{
			e0 = 0.0f;
			e2 = 0.0f;
			e1 = 0.0f;
			}
		double sum0;
		double sum1;
		double sum2;
		const JointSupportBody* body1 = mBody[1];
		if(body1)
			{
			const NxReal* m = body1->mUnknown020;
			const double f0 = (supportMul(vz, m[2]) + supportMul(vy, m[1])) + supportMul(vx, m[0]);
			const NxReal f1 = (NxReal)((supportMul(vz, m[5]) + supportMul(vy, m[4])) + supportMul(vx, m[3]));
			const NxReal f2 = (NxReal)((supportMul(vz, m[8]) + supportMul(vy, m[7])) + supportMul(vx, m[6]));
			sum0 = e0 + f0;
			sum1 = (double)f1 + e1;
			sum2 = (double)f2 + e2;
			}
		else
			{
			sum0 = e0;
			sum1 = e1;
			sum2 = e2;
			}
		out0 = (NxReal)((sum0 * vx + sum1 * vy) + sum2 * vz);
		}
	// `fucompp; test ah,0x44; jnp`: only an exact zero (not a NaN) takes
	// the zero arm.
	if(out0 == 0.0f)
		out1 = 0.0f;
	else
		out1 = (NxReal)(1.0f / (double)out0);
	}

// phys_fn_004393 (0x000af710, 122 B)
// Both tests are `fcomp 0; test ah,1; jne`: the branch is taken only for an
// ordered argument >= 0. phys_fn_004391's first output is never read.
void JointSupportRecord::row004393(NxReal arg0, NxReal arg1)
	{
	NxReal unused;
	NxReal value;
	row004391(unused, value);
	mUnknown03c = value;
	mUnknown040 = value;
	if(arg1 >= 0.0f)
		mUnknown040 = (NxReal)supportMul(value, arg1);
	if(arg0 >= 0.0f)
		{
		const double scale = 1.0 / (supportMul(value, arg0) + 1.0);
		mUnknown040 = (NxReal)(scale * mUnknown040);
		mUnknown03c = (NxReal)(scale * value);
		}
	}

// The body record's fields the island rows touch, by byte offset.
static NX_INLINE NxU32& supportWord(void* record, NxU32 offset)
	{
	return *reinterpret_cast<NxU32*>(static_cast<NxU8*>(record) + offset);
	}

static NX_INLINE void*& supportPointer(void* record, NxU32 offset)
	{
	return *reinterpret_cast<void**>(static_cast<NxU8*>(record) + offset);
	}

// .rdata 0x101053d4: the wake floor phys_fn_000760 compares +0x4c against,
// the same 0.39999998f (0x3ecccccc) phys_fn_004107 uses.
static const NxReal gSupportWakeFloor = 0.39999998f;

// phys_fn_000015 (0x000014f0, 41 B)
// A compound (+0xd0 == 5) counts its +0xe0..+0xe4 pointer span (`sar`, a
// signed quotient); any other shape is one; no shape is none.
NxU32 JointActorBody::getNbShapes() const
	{
	const NxU8* shape = static_cast<const NxU8*>(mShape);
	if(!shape)
		return 0;
	if(*reinterpret_cast<const NxU32*>(shape + 0xd0) != 5)
		return 1;
	void* const* first = *reinterpret_cast<void* const* const*>(shape + 0xe0);
	void* const* last = *reinterpret_cast<void* const* const*>(shape + 0xe4);
	return (NxU32)(last - first);
	}

// phys_fn_000017 (0x00001520, 28 B)
// The compound's +0xe0 array, else &mShape (the listing's `lea eax,
// [ecx+0x10]` is the return value); 0 with no shape.
void** JointActorBody::getShapes()
	{
	NxU8* shape = static_cast<NxU8*>(mShape);
	if(!shape)
		return 0;
	if(*reinterpret_cast<NxU32*>(shape + 0xd0) == 5)
		return *reinterpret_cast<void***>(shape + 0xe0);
	return &mShape;
	}

// phys_fn_000022 (0x00001840, 27 B)
// 000754 on the actor body's +0x08 record, then the +0x10 object's slot 6
// with the argument as a tail jump (0x1855), or `ret 4` when +0x10 is null.
void Row000022Fixture::row000022(NxU32 arg)
	{
	reinterpret_cast<Row000754Fixture*>(supportPointer(this, 0x08))->row000754();
	Row000022Target* target = static_cast<Row000022Target*>(supportPointer(this, 0x10));
	if(target)
		target->slot6(arg);
	}

// phys_fn_000754 (0x00017010, 1027 B)
// The body record's pose from its centre-of-mass pose (owner gap
// SceneRaycast..CapsuleShape; written by joint-open-items Task 6, whose
// projection differential showed the oracle writing +0x18..+0x30 here).
// R = C M^T with C the +0x134 3x3 and M the +0xdc mass-local 3x3 (both
// row-major), each element a sum of three products stored as a float
// (0x17013-0x1719b). The position is c - R m, with c = +0x158 and m =
// +0x100: R m's x stays in a register, y and z are stored (0x1719f-0x17209);
// the three differences are rounded once into +0x18..+0x20. The quaternion
// is the unnormalised NxQuat-from-matrix sequence over the stored R, into
// +0x24..+0x30 as x, y, z, w (0x1724a-0x1740c):
// - trace = (R22 + R11) + R00, with R22 + R11 also stored as a float; when
//   trace >= 0 (a NaN takes the other arm), s = sqrt(trace + 1), w = s/2
//   stored, k = 0.5 / s kept, and x, y stored, z kept;
// - otherwise the largest diagonal i (R11 > R00, then R22 > R[i][i], both
//   strict), s = sqrt(1 + R[i][i] - the other two), the i component s/2 and
//   k = 0.5 / s stored (arm 2 keeps s/2 and forms k from the stored s).
// z always reaches +0x2c from the register. Each root is formed inside an
// X87Sqrt.h helper from the stored floats in the listing's order (trace arm
// fld R22, fadd R11, fadd R00, fadd 1; arm 2 fld R11, fadd R00, fsubr R22,
// fadd 1; arm 1 fld R22, fadd R00, fsubr R11, fadd 1; arm 0 fld R00, fsub
// the stored R22 + R11, fadd 1), so no sum is narrowed under 0x0f7f.
void Row000754Fixture::row000754()
	{
	NxU8* record = static_cast<NxU8*>(static_cast<void*>(this));
	const NxReal* C = reinterpret_cast<const NxReal*>(record + 0x134);
	const NxReal* M = reinterpret_cast<const NxReal*>(record + 0xdc);
	const NxReal* c = reinterpret_cast<const NxReal*>(record + 0x158);
	const NxReal* m = reinterpret_cast<const NxReal*>(record + 0x100);
	NxReal* position = reinterpret_cast<NxReal*>(record + 0x18);
	NxReal* quaternion = reinterpret_cast<NxReal*>(record + 0x24);

	NxReal R[9];
	R[0] = (NxReal)((supportMul(C[2], M[2]) + supportMul(C[0], M[0])) + supportMul(C[1], M[1]));
	R[1] = (NxReal)((supportMul(C[2], M[5]) + supportMul(M[3], C[0])) + supportMul(C[1], M[4]));
	R[2] = (NxReal)((supportMul(C[2], M[8]) + supportMul(C[1], M[7])) + supportMul(M[6], C[0]));
	R[3] = (NxReal)((supportMul(M[2], C[5]) + supportMul(M[1], C[4])) + supportMul(C[3], M[0]));
	R[4] = (NxReal)((supportMul(C[3], M[3]) + supportMul(C[5], M[5])) + supportMul(C[4], M[4]));
	R[5] = (NxReal)((supportMul(M[8], C[5]) + supportMul(M[7], C[4])) + supportMul(M[6], C[3]));
	R[6] = (NxReal)((supportMul(M[2], C[8]) + supportMul(M[1], C[7])) + supportMul(C[6], M[0]));
	R[7] = (NxReal)((supportMul(C[6], M[3]) + supportMul(C[8], M[5])) + supportMul(C[7], M[4]));
	R[8] = (NxReal)((supportMul(M[8], C[8]) + supportMul(M[7], C[7])) + supportMul(M[6], C[6]));

	const double rmx = (supportMul(R[2], m[2]) + supportMul(R[1], m[1])) + supportMul(R[0], m[0]);
	const NxReal rmy = (NxReal)((supportMul(R[5], m[2]) + supportMul(R[4], m[1])) + supportMul(R[3], m[0]));
	const NxReal rmz = (NxReal)((supportMul(R[8], m[2]) + supportMul(R[7], m[1])) + supportMul(R[6], m[0]));
	const NxReal px = (NxReal)((double)c[0] - rmx);
	const NxReal py = (NxReal)((double)c[1] - rmy);
	const NxReal pz = (NxReal)((double)c[2] - rmz);
	position[2] = pz;
	position[0] = px;
	position[1] = py;

	const NxReal sum84 = (NxReal)((double)R[8] + R[4]);
	const double trace = ((double)R[8] + R[4]) + R[0];
	NxReal x, y, w;
	double z;
	if(trace >= 0.0f)
		{
		const double s = x87FsqrtSum4(R[8], R[4], R[0], 1.0f);
		w = (NxReal)(0.5f * s);
		const double k = 0.5f / s;
		x = (NxReal)(((double)R[7] - R[5]) * k);
		y = (NxReal)(((double)R[2] - R[6]) * k);
		z = ((double)R[3] - R[1]) * k;
		}
	else
		{
		NxU32 index = 0;
		if(R[4] > R[0])
			index = 1;
		if(R[8] > R[index * 4])
			index = 2;
		if(index == 2)
			{
			const double s = x87FsqrtDiag(R[8], R[4], R[0]);
			const NxReal sF = (NxReal)s;
			z = s * 0.5f;
			const double k = 0.5f / (double)sF;
			x = (NxReal)(((double)R[6] + R[2]) * k);
			y = (NxReal)(((double)R[7] + R[5]) * k);
			w = (NxReal)(((double)R[3] - R[1]) * k);
			}
		else if(index == 1)
			{
			const double s = x87FsqrtDiag(R[4], R[8], R[0]);
			y = (NxReal)(0.5f * s);
			const NxReal k = (NxReal)(0.5f / s);
			z = ((double)R[7] + R[5]) * k;
			x = (NxReal)(((double)R[3] + R[1]) * k);
			w = (NxReal)(((double)R[2] - R[6]) * k);
			}
		else
			{
			const double s = x87FsqrtSum3(R[0], -sum84, 1.0f);
			x = (NxReal)(0.5f * s);
			const NxReal k = (NxReal)(0.5f / s);
			y = (NxReal)(((double)R[3] + R[1]) * k);
			z = ((double)R[6] + R[2]) * k;
			w = (NxReal)(((double)R[7] - R[5]) * k);
			}
		}
	quaternion[2] = (NxReal)z;
	quaternion[0] = x;
	quaternion[1] = y;
	quaternion[3] = w;
	}

// phys_fn_000758 (0x00017630, 214 B)
// The record's +0x124 quaternion (x, y, z, w) to the +0x134 row-major 3x3.
// Every product is formed and doubled on the stack (`fmul; fadd st0,st0`);
// the listing spills 2yy, 2xz, 2yw, 2yz and 1 - 2xx to floats and reuses the
// spilled copies, so those are NxReal here and the rest are double. The
// constant 1 is the float at 0x101041ec.
void Row000758Fixture::row000758()
	{
	NxReal* q = reinterpret_cast<NxReal*>(static_cast<NxU8*>(static_cast<void*>(this)) + 0x124);
	NxReal* m = reinterpret_cast<NxReal*>(static_cast<NxU8*>(static_cast<void*>(this)) + 0x134);
	const double w = q[3];
	const double x = q[0];
	const double y = q[1];
	const double z = q[2];
	const double yy = y * y;
	const NxReal yy2 = (NxReal)(yy + yy);
	const double zz = z * z;
	const double zz2 = zz + zz;
	m[0] = (NxReal)((1.0f - (double)yy2) - zz2);
	const double xy = y * x;
	const double xy2 = xy + xy;
	const double zw = z * w;
	const double zw2 = zw + zw;
	m[1] = (NxReal)(xy2 - zw2);
	const double xz = z * x;
	const NxReal xz2 = (NxReal)(xz + xz);
	const double yw = y * w;
	const double yw2 = yw + yw;
	const NxReal yw2Spill = (NxReal)yw2;
	m[2] = (NxReal)(yw2 + (double)xz2);
	m[3] = (NxReal)(zw2 + xy2);
	const double xx = x * x;
	const double oneMinusXx2 = 1.0f - (xx + xx);
	const NxReal oneMinusXx2Spill = (NxReal)oneMinusXx2;
	m[4] = (NxReal)(oneMinusXx2 - zz2);
	const double yz = z * y;
	const NxReal yz2 = (NxReal)(yz + yz);
	const double xw = w * x;
	const double xw2 = xw + xw;
	m[5] = (NxReal)((double)yz2 - xw2);
	m[6] = (NxReal)((double)xz2 - yw2Spill);
	m[7] = (NxReal)(xw2 + yz2);
	m[8] = (NxReal)((double)oneMinusXx2Spill - yy2);
	}

// phys_fn_000712 (0x00015d30, 32 B)
// The listing stores the recursive result back and reloads +0x1bc for the
// return value.
Row000712Fixture* Row000712Fixture::row000712()
	{
	Row000712Fixture* parent = static_cast<Row000712Fixture*>(supportPointer(this, 0x1bc));
	if(this != parent)
		supportPointer(this, 0x1bc) = parent->row000712();
	return static_cast<Row000712Fixture*>(supportPointer(this, 0x1bc));
	}

// phys_fn_000722 (0x00016130, 127 B)
// The maximum is `fcom [rec+0x4c]; test ah,5; jp`: the running value is
// replaced only when it is ordered below the record's; an unordered or
// greater-or-equal one is kept. All the values are floats loaded exactly,
// so no x87 precision question arises.
// The root is stored back to +0x1bc only when the record is not its own
// root (0x16144-0x16149), then +0x1bc is reloaded. Written on both main
// (effector-and-coredump Task 2) and the NpActor.cpp completion branch
// (Task 5); the two were equivalent and this one is kept at the merge.
__declspec(noinline) void Row000722Fixture::row000722()
	{
	void* parent = supportPointer(this, 0x1bc);
	if(this != parent)
		supportPointer(this, 0x1bc) = static_cast<Row000712Fixture*>(parent)->row000712();
	void* root = supportPointer(this, 0x1bc);
	if(root == this)
		{
		NxReal wake = 0.0f;
		for(void* body = root; body; body = supportPointer(body, 0x1d0))
			{
			const NxReal value = *reinterpret_cast<const NxReal*>(static_cast<NxU8*>(body) + 0x4c);
			if(wake < value)
				wake = value;
			}
		*reinterpret_cast<NxReal*>(static_cast<NxU8*>(root) + 0x1cc) = wake;
		}
	else
		supportWord(root, 0x1cc) = 0x4b7afafa;
	for(NxU32 i = 0; i < 7; i++)
		supportWord(this, 0x1e8 + i * 4) = supportWord(this, 0x1bc + i * 4);
	supportWord(this, 0x25c) = 0;
	supportWord(this, 0x208) = 0;
	}

// phys_fn_000738 (0x00016c00, 21 B)
// `mov eax,[ecx+0x1e4]; test ah,2`: bit 9 of +0x1e4 set returns
// `lea eax,[ecx+0x244]`, clear returns 0. The only caller is 001303 in the
// broadphase's CCD path (001949 <- 001976 <- 000608 <- the step 000655),
// which the product does not run, so nothing calls this yet.
void* Row000738Fixture::row000738()
	{
	if(supportWord(this, 0x1e4) & 0x200)
		return static_cast<NxU8*>(static_cast<void*>(this)) + 0x244;
	return 0;
	}

// phys_fn_000713 (0x00015d50, 32 B)
// 000712's shape on the +0x1e8 chain: the recursive result is stored back
// and +0x1e8 reloaded for the return value.
Row000713Fixture* Row000713Fixture::row000713()
	{
	Row000713Fixture* parent = static_cast<Row000713Fixture*>(supportPointer(this, 0x1e8));
	if(this != parent)
		supportPointer(this, 0x1e8) = parent->row000713();
	return static_cast<Row000713Fixture*>(supportPointer(this, 0x1e8));
	}

// Row 000782 (NpActor.cpp): the body's force/torque accumulator, thiscall on
// the record in the image (`ret 0x10`).
void nxNpActorApplyForce(unsigned char* record, const NxVec3* force,
	const NxVec3* torque, unsigned mode, bool wake);

// phys_fn_000791 (0x0001a2c0, 133 B)
// The lever d = position - centre of mass (+0x158): dx stays in the register
// (0x1a2c7-0x1a2c9), dy and dz are spilled to float (0x1a2d8, 0x1a2e9). The
// torque d x force is formed x, y, z (each `fmul; fmul; fsubp`, 0x1a2ed-0x1a315)
// and stored to a float local; 000782 (NpActor.cpp nxNpActorApplyForce, the
// NpActor.cpp completion's row, kept at the second merge) is called once with
// (force, &torque, mode, wake), the mode passed through unchanged and the
// wake as its low byte (000782 tests `mov al,[esp+0x2c]`, 0x1936f). noinline:
// the image calls it as its own function (from 000054, 000154-000158 and
// 003979).
__declspec(noinline) void Row000791Fixture::row000791(const NxVec3& force, const NxVec3& position, NxU32 word3,
	NxU32 word4)
	{
	const NxReal* centre = reinterpret_cast<const NxReal*>(static_cast<NxU8*>(static_cast<void*>(this)) + 0x158);
	const double dx = (double)position.x - centre[0];
	const NxReal dy = (NxReal)((double)position.y - centre[1]);
	const NxReal dz = (NxReal)((double)position.z - centre[2]);
	NxVec3 torque;
	torque.x = (NxReal)((double)dy * force.z - (double)dz * force.y);
	torque.y = (NxReal)((double)dz * force.x - dx * force.z);
	torque.z = (NxReal)(dx * force.y - (double)dy * force.x);
	nxNpActorApplyForce(reinterpret_cast<NxU8*>(static_cast<void*>(this)), &force, &torque, word3,
		(NxU8)word4 != 0);
	}

// phys_fn_000760 (0x00017710, 168 B)
// Only a record that is its own root frees its island object (+0x1e0,
// through 004167 and the Foundation allocator's slot +0x14). The flags word +0x114
// is read before the stores (0x1774c); bit 8 suppresses the wake raise,
// which is `fcomp [0x101053d4]; test ah,5; jp`: only an ordered +0x4c below
// the floor is raised. noinline: the oracle calls it as its own function
// (0x1870a from 000780, 0x110d6 from 000604), which the compiler would
// otherwise fold into row000778.
__declspec(noinline) void Row000760Fixture::row000760()
	{
	if(supportPointer(this, 0x1bc) == this)
		{
		void* island = supportPointer(this, 0x1e0);
		if(island)
			{
			reinterpret_cast<Row004167Fixture*>(island)->row004167();
			nxFoundationSDKAllocator->free(island);
			supportPointer(this, 0x1e0) = 0;
			}
		}
	const NxU32 bits = supportWord(this, 0x1e4) & ~2u;
	const NxU32 flags = supportWord(this, 0x114);
	supportPointer(this, 0x1bc) = this;
	supportWord(this, 0x1c0) = 0;
	supportWord(this, 0x1d0) = 0;
	supportPointer(this, 0x1d4) = this;
	supportWord(this, 0x1c4) = 0;
	supportWord(this, 0x1c8) = 1;
	supportWord(this, 0x1cc) = 0x4b7afafa;
	supportWord(this, 0x1d8) = 0;
	supportWord(this, 0x1dc) = 0;
	supportWord(this, 0x1e4) = bits;
	if(!(flags & 0x100))
		{
		NxReal& wake = *reinterpret_cast<NxReal*>(static_cast<NxU8*>(static_cast<void*>(this)) + 0x4c);
		if(wake < gSupportWakeFloor)
			supportWord(this, 0x4c) = 0x3ecccccc;
		}
	}

// phys_fn_000778 (0x000185f0, 58 B)
// With its continuation phys_fn_000780 (0x00018630, 243 B), which carries
// the loop and the `ret 8`. The root is refreshed through 000712 on the
// parent (0x185fd) only when the record is not its own root. For each
// island body: every joint on its +0x1d8 list except `joint` is pushed onto
// the array (the 000661 push, inlined at 0x1864c-0x186e6); every link word
// (+0x34) is cleared, `joint`'s included; then the next body (+0x1d0) is
// read before 000760 resets this one.
void Row000778Fixture::row000778(void* joint, void** jointArray)
	{
	void* parent = supportPointer(this, 0x1bc);
	if(this != parent)
		supportPointer(this, 0x1bc) = static_cast<Row000712Fixture*>(parent)->row000712();
	void* body = supportPointer(this, 0x1bc);
	while(body)
		{
		void* linked = supportPointer(body, 0x1d8);
		while(linked)
			{
			if(joint != linked)
				nxJointPointerArrayPush(jointArray, linked);
			void* next = supportPointer(linked, 0x34);
			supportPointer(linked, 0x34) = 0;
			linked = next;
			}
		void* nextBody = supportPointer(body, 0x1d0);
		static_cast<Row000760Fixture*>(body)->row000760();
		body = nextBody;
		}
	}

// phys_fn_004167 (0x0009ad10, 156 B)
// (deferred: owner gap Joint.cpp..D6Joint.cpp)
void Row004167Fixture::row004167()
	{
	NX_ASSERT(0);
	}
