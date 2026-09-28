/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/SpringAndDamperEffector.h"
#include "core/NpSpringAndDamperEffector.h"
#include "core/JointSupport.h"
#include "Scene.h"
#include "X87Sqrt.h"

#include <new>

// The internal effector rows (effector-and-coredump Task 2): Effector
// 003934-003938, ActorPairEffector 003922-003932 and SpringAndDamperEffector
// 003960-003979 with the core dump's reader 003964. The oracle keeps no
// __FILE__ for them. See units/effector-coredump-contract.md "## Effector".
//
// The rows the listing calls as functions (003922, 003926, 003930, 003934,
// 003936, 003970, 003972) are noinline, so that a breakpoint on the row sees
// it run, as the Scene joint rows' addJoint and 000760 are.
//
// Precision: as in core/Joint.cpp, a value the listing keeps on the x87
// stack is a `double` here and a value it stores is an `NxReal`, with the
// listing's operand grouping kept. This file is on the /arch:IA32 list: the
// solver slot 003979 runs inside the simulation step's control word.
//
// The body records are the 0x260-byte dynamic records (`[actorBody+8]`),
// read by offset: +0x34 linear velocity, +0x40 angular velocity, +0x134
// the row-major 3x3 and +0x158 the world centre of mass (the pose the
// joint rows read through JointBodyRecord), +0x19c the owning actor body,
// +0x1f8 the float 003979 tests on the chain root (000713).

static NX_INLINE const NxReal* effectorRecordFloats(const void* record, NxU32 offset)
	{
	return reinterpret_cast<const NxReal*>(static_cast<const NxU8*>(record) + offset);
	}

static NX_INLINE void* effectorRecordPointer(const void* record, NxU32 offset)
	{
	return *reinterpret_cast<void* const*>(static_cast<const NxU8*>(record) + offset);
	}

// ---------------------------------------------------------------------------
// ActorPairEffector

// phys_fn_003922 (0x0008ed20, 33 B)
__declspec(noinline) ActorPairEffector::ActorPairEffector(NxSceneInternal* scene)
	: Effector(scene)
	{
	mBody[0] = 0;
	mBody[1] = 0;
	}

// phys_fn_003924 (0x0008ed50, 14 B)
// Slot 2: slot 3 through the object's own table with (+0x24, +0x28).
void ActorPairEffector::tick()
	{
	apply(mBody[0], mBody[1]);
	}

// phys_fn_003926 (0x0008ed60, 72 B)
// removeObserver (import 0x10104158) on each old non-null record, both new
// records stored, then addObserver (import 0x1010415c) on each new non-null
// record. The records' last-observer event (removeObserver's
// OE_LAST_OBSERVER_REMOVED) goes to their own table's slot 0.
__declspec(noinline) void ActorPairEffector::setBodyRecords(NxFoundation::Observable* body1, NxFoundation::Observable* body2)
	{
	if(mBody[0])
		mBody[0]->removeObserver(*this);
	if(mBody[1])
		mBody[1]->removeObserver(*this);
	mBody[0] = body1;
	mBody[1] = body2;
	if(mBody[0])
		mBody[0]->addObserver(*this);
	if(mBody[1])
		mBody[1]->addObserver(*this);
	}

// phys_fn_003928 (0x0008edb0, 39 B)
// Slot 0 (Observable::event). Event 0x100 is the body record's teardown
// notify (000030 at 0x1d82, 000122 at 0x3af6): the record that sent it is
// no longer observed. Any other sender than +0x24 is taken to be +0x28.
void ActorPairEffector::event(NxU32 code, NxFoundation::Observable& sender)
	{
	if(code != 0x100)
		return;
	if(mBody[0] == &sender)
		mBody[0] = 0;
	else
		mBody[1] = 0;
	}

// phys_fn_003930 (0x0008ede0, 59 B)
// Stops observing both records and zeroes them; Effector's destructor body
// (003936, a tail jump in the listing) follows.
__declspec(noinline) ActorPairEffector::~ActorPairEffector()
	{
	if(mBody[0])
		mBody[0]->removeObserver(*this);
	if(mBody[1])
		mBody[1]->removeObserver(*this);
	mBody[0] = 0;
	mBody[1] = 0;
	}

// phys_fn_003932 (0x0008ee20, 84 B)
// Not defined here: ActorPairEffector's scalar deleting destructor, which
// the compiler emits from the virtual destructor above (003930's body, then
// 003936, then the class operator delete when flags bit 0 is set). It is in
// 0x101178f8 only, the table of an abstract class, so nothing calls it.

// ---------------------------------------------------------------------------
// Effector

// phys_fn_003934 (0x0008ee80, 35 B)
// The Observable base constructor (import 0x10104190) zeroes the observer
// array; +0x14 and +0x20 are left as allocated.
__declspec(noinline) Effector::Effector(NxSceneInternal* scene)
	{
	mScene = scene;
	mNext = 0;
	}

// phys_fn_003936 (0x0008eeb0, 12 B)
// Nothing in the body: the table store and ~Observable (import 0x10104194,
// a tail jump in the listing) are the base-destruction chain.
__declspec(noinline) Effector::~Effector()
	{
	}

// phys_fn_003938 (0x0008eec0, 41 B)
// Not defined here: Effector's scalar deleting destructor (003936's body
// inlined, then the Foundation allocator's free when flags bit 0 is set),
// which the compiler emits from the virtual destructor above; it sits in
// 0x10117920, the abstract base's table, and nothing calls it.

// ---------------------------------------------------------------------------
// SpringAndDamperEffector

// phys_fn_003960 (0x0008f180, 115 B)
// Zeroes +0x44..+0x64, then +0x34, +0x30, +0x2c, +0x40, +0x3c, +0x38, in
// the listing's order, then allocates the Np wrapper through the Foundation
// allocator's slot +8 with (0x18, 0) and constructs it (003958); +0x20 is
// the wrapper, or 0 when the allocation fails.
SpringAndDamperEffector::SpringAndDamperEffector(NxSceneInternal* scene)
	: ActorPairEffector(scene)
	{
	mDistCompressSaturate = 0;
	mDistRelaxed = 0;
	mDistStretchSaturate = 0;
	mSpringMaxCompressForce = 0;
	mSpringMaxStretchForce = 0;
	mVelCompressSaturate = 0;
	mVelStretchSaturate = 0;
	mDamperMaxCompressForce = 0;
	mDamperMaxStretchForce = 0;
	mLocalAnchor[0].z = 0;
	mLocalAnchor[0].y = 0;
	mLocalAnchor[0].x = 0;
	mLocalAnchor[1].z = 0;
	mLocalAnchor[1].y = 0;
	mLocalAnchor[1].x = 0;
	void* memory = nxFoundationSDKAllocator->malloc(0x18, NX_MEMORY_PERSISTENT);
	if(memory)
		mPublicObject = new(memory) NpSpringAndDamperEffector(this);
	else
		mPublicObject = 0;
	}

// phys_fn_003962 (0x0008f200, 399 B)
// Each body's record (`[actorBody+8]`, 0 for no body) goes to 003926; then
// each anchor: with a record, the global point less the record's +0x158,
// through the transposed +0x134 3x3; without one, the point as given. The
// listing keeps the z difference on the stack for the y row and uses the
// stored float everywhere else (0x8f259 `fst`), as Joint::setLimitPoint.
void SpringAndDamperEffector::setBodies(void* actorBody1, const NxVec3& global1, void* actorBody2,
	const NxVec3& global2)
	{
	NxFoundation::Observable* record1 = actorBody1
		? static_cast<NxFoundation::Observable*>(effectorRecordPointer(actorBody1, 8)) : 0;
	NxFoundation::Observable* record2 = actorBody2
		? static_cast<NxFoundation::Observable*>(effectorRecordPointer(actorBody2, 8)) : 0;
	setBodyRecords(record1, record2);

	const void* records[2] = { record1, record2 };
	const NxVec3* globals[2] = { &global1, &global2 };
	for(unsigned i = 0; i < 2; i++)
		{
		const NxVec3& point = *globals[i];
		if(records[i])
			{
			const NxReal* m = effectorRecordFloats(records[i], 0x134);
			const NxReal* c = effectorRecordFloats(records[i], 0x158);
			const double dx = (double)point.x - c[0];
			const NxReal dy = (NxReal)((double)point.y - c[1]);
			const double dz = (double)point.z - c[2];
			const NxReal dzf = (NxReal)dz;
			const double y = (dz * m[7] + dx * m[1]) + (double)dy * m[4];
			const double z = ((double)dzf * m[8] + dx * m[2]) + (double)dy * m[5];
			const double x = ((double)dzf * m[6] + (double)dy * m[3]) + dx * m[0];
			mLocalAnchor[i].x = (NxReal)x;
			mLocalAnchor[i].y = (NxReal)y;
			mLocalAnchor[i].z = (NxReal)z;
			}
		else
			{
			mLocalAnchor[i].x = point.x;
			mLocalAnchor[i].y = point.y;
			mLocalAnchor[i].z = point.z;
			}
		}
	}

// phys_fn_003964 (0x0008f390, 343 B)
// Both owners are read before either record is tested (0x8f390-0x8f3ac):
// an effector with a world end faults here, as the oracle's does. Each
// anchor with a record goes back to world space through the +0x134 3x3 and
// +0x158: the x row stays on the stack, the y and z rows are stored first
// (0x8f3fa, 0x8f41d). The two bodies group their products differently.
void SpringAndDamperEffector::getBodies(void** owner1, NxVec3& global1, void** owner2, NxVec3& global2)
	{
	*owner1 = effectorRecordPointer(mBody[0], 0x19c);
	*owner2 = effectorRecordPointer(mBody[1], 0x19c);
	if(mBody[0])
		{
		const NxReal* m = effectorRecordFloats(mBody[0], 0x134);
		const NxReal* c = effectorRecordFloats(mBody[0], 0x158);
		const NxVec3& a = mLocalAnchor[0];
		const double x = ((double)m[1] * a.y + (double)m[2] * a.z) + (double)m[0] * a.x;
		const NxReal y = (NxReal)(((double)m[4] * a.y + (double)m[3] * a.x) + (double)m[5] * a.z);
		const NxReal z = (NxReal)(((double)m[7] * a.y + (double)m[6] * a.x) + (double)m[8] * a.z);
		global1.x = (NxReal)(x + c[0]);
		global1.y = (NxReal)((double)y + c[1]);
		global1.z = (NxReal)((double)z + c[2]);
		}
	if(mBody[1])
		{
		const NxReal* m = effectorRecordFloats(mBody[1], 0x134);
		const NxReal* c = effectorRecordFloats(mBody[1], 0x158);
		const NxVec3& b = mLocalAnchor[1];
		const double x = ((double)m[2] * b.z + (double)m[1] * b.y) + (double)b.x * m[0];
		const NxReal y = (NxReal)(((double)m[5] * b.z + (double)m[4] * b.y) + (double)m[3] * b.x);
		const NxReal z = (NxReal)(((double)m[8] * b.z + (double)m[7] * b.y) + (double)m[6] * b.x);
		global2.x = (NxReal)(x + c[0]);
		global2.y = (NxReal)((double)y + c[1]);
		global2.z = (NxReal)((double)z + c[2]);
		}
	}

// phys_fn_003966 (0x0008f4f0, 38 B)
void SpringAndDamperEffector::setLinearSpring(NxReal distCompressSaturate, NxReal distRelaxed,
	NxReal distStretchSaturate, NxReal maxCompressForce, NxReal maxStretchForce)
	{
	mDistCompressSaturate = distCompressSaturate;
	mDistRelaxed = distRelaxed;
	mDistStretchSaturate = distStretchSaturate;
	mSpringMaxCompressForce = maxCompressForce;
	mSpringMaxStretchForce = maxStretchForce;
	}

// phys_fn_003968 (0x0008f520, 31 B)
void SpringAndDamperEffector::setLinearDamper(NxReal velCompressSaturate, NxReal velStretchSaturate,
	NxReal maxCompressForce, NxReal maxStretchForce)
	{
	mVelCompressSaturate = velCompressSaturate;
	mVelStretchSaturate = velStretchSaturate;
	mDamperMaxCompressForce = maxCompressForce;
	mDamperMaxStretchForce = maxStretchForce;
	}

// phys_fn_003970 (0x0008f540, 150 B)
// The spring force for a distance, on the x87 stack. Stretched (distance >
// relaxed): 0 unless maxStretchForce > 0; below distStretchSaturate the
// ramp -(maxStretch / (stretchSat - relaxed)) * (distance - relaxed), from
// there (or unordered) -maxStretch. Otherwise: 0 unless maxCompressForce >
// 0; above distCompressSaturate (distance*maxC)/D - (maxC*relaxed)/D with
// D = compressSat - relaxed, else maxCompress. The zero is .rdata
// 0x101041f0 (0.0f).
__declspec(noinline) double SpringAndDamperEffector::springForce(NxReal distance)
	{
	if(distance > mDistRelaxed)
		{
		if(!(mSpringMaxStretchForce > 0.0f))
			return 0.0f;
		if(distance < mDistStretchSaturate)
			return -((double)mSpringMaxStretchForce / ((double)mDistStretchSaturate - mDistRelaxed))
				* ((double)distance - mDistRelaxed);
		return -(double)mSpringMaxStretchForce;
		}
	if(!(mSpringMaxCompressForce > 0.0f))
		return 0.0f;
	if(distance > mDistCompressSaturate)
		{
		const double range = (double)mDistCompressSaturate - mDistRelaxed;
		return ((double)distance * mSpringMaxCompressForce) / range
			- ((double)mSpringMaxCompressForce * mDistRelaxed) / range;
		}
	return mSpringMaxCompressForce;
	}

// phys_fn_003972 (0x0008f5e0, 125 B)
// The damper force for a relative velocity, on the x87 stack. Positive
// velocity: 0 unless maxStretchForce > 0; below velStretchSaturate
// -(maxStretch / stretchSat) * v, from there (or unordered) -maxStretch.
// Otherwise: 0 unless maxCompressForce > 0; above velCompressSaturate
// (maxCompress / compressSat) * v, else maxCompress.
__declspec(noinline) double SpringAndDamperEffector::damperForce(NxReal velocity)
	{
	if(velocity > 0.0f)
		{
		if(!(mDamperMaxStretchForce > 0.0f))
			return 0.0f;
		if(velocity < mVelStretchSaturate)
			return -((double)mDamperMaxStretchForce / mVelStretchSaturate) * velocity;
		return -(double)mDamperMaxStretchForce;
		}
	if(!(mDamperMaxCompressForce > 0.0f))
		return 0.0f;
	if(velocity > mVelCompressSaturate)
		return ((double)mDamperMaxCompressForce / mVelCompressSaturate) * velocity;
	return mDamperMaxCompressForce;
	}

// phys_fn_003974 (0x0008f660, 48 B)
void SpringAndDamperEffector::getLinearSpring(NxReal& distCompressSaturate, NxReal& distRelaxed,
	NxReal& distStretchSaturate, NxReal& maxCompressForce, NxReal& maxStretchForce)
	{
	distCompressSaturate = mDistCompressSaturate;
	distRelaxed = mDistRelaxed;
	distStretchSaturate = mDistStretchSaturate;
	maxCompressForce = mSpringMaxCompressForce;
	maxStretchForce = mSpringMaxStretchForce;
	}

// phys_fn_003975 (0x0008f690, 39 B)
void SpringAndDamperEffector::getLinearDamper(NxReal& velCompressSaturate, NxReal& velStretchSaturate,
	NxReal& maxCompressForce, NxReal& maxStretchForce)
	{
	velCompressSaturate = mVelCompressSaturate;
	velStretchSaturate = mVelStretchSaturate;
	maxCompressForce = mDamperMaxCompressForce;
	maxStretchForce = mDamperMaxStretchForce;
	}

// phys_fn_003977 (0x0008f6c0, 59 B)
// The deleting destructor (slot 1) is the compiler's, from this body: the
// Np wrapper's deleting destructor with 1 through its hook member's table
// (`lea ecx,[eax+8]; call [[ecx]]`, 0x8f6d0), then ActorPairEffector's
// destructor body 003930 and 003936, then the class operator delete.
SpringAndDamperEffector::~SpringAndDamperEffector()
	{
	if(mPublicObject)
		delete mPublicObject;
	}

// Adds the listing's world anchor and point velocity for one body with a
// record (003979 0x8f711-0x8f813 for body 1, 0x8f850-0x8f95e for body 2).
// The rows of the 3x3 apply to the local anchor with the listing's
// grouping (the two bodies differ); x is kept on the stack while y and z
// are stored, the world point is stored and the arm from the centre of
// mass rebuilt from it (x from the stored float, y from the unrounded sum,
// z kept on the stack), and the velocity is v + w x r with the x and y
// components of w x r stored and z kept.
struct EffectorBodyState
	{
	NxVec3	point;
	NxReal	velocity[3];
	};

static void effectorBodyState1(const void* record, const NxVec3& a, EffectorBodyState& out)
	{
	const NxReal* m = effectorRecordFloats(record, 0x134);
	const NxReal* c = effectorRecordFloats(record, 0x158);
	const NxReal* v = effectorRecordFloats(record, 0x34);
	const NxReal* w = effectorRecordFloats(record, 0x40);
	const double x = ((double)m[1] * a.y + (double)m[2] * a.z) + (double)m[0] * a.x;
	const double y = ((double)m[3] * a.x + (double)m[4] * a.y) + (double)m[5] * a.z;
	const NxReal z = (NxReal)(((double)m[6] * a.x + (double)m[7] * a.y) + (double)m[8] * a.z);
	const NxReal px = (NxReal)(x + c[0]);
	const double py = y + c[1];
	const double pz = (double)z + c[2];
	out.point.x = px;
	out.point.y = (NxReal)py;
	out.point.z = (NxReal)pz;
	const NxReal rx = (NxReal)((double)px - c[0]);
	const NxReal ry = (NxReal)(py - c[1]);
	const double rz = pz - c[2];
	const NxReal cx = (NxReal)(rz * w[1] - (double)ry * w[2]);
	const NxReal cy = (NxReal)((double)rx * w[2] - rz * w[0]);
	const double cz = (double)ry * w[0] - (double)rx * w[1];
	out.velocity[0] = (NxReal)((double)cx + v[0]);
	out.velocity[1] = (NxReal)((double)cy + v[1]);
	out.velocity[2] = (NxReal)(cz + v[2]);
	}

static void effectorBodyState2(const void* record, const NxVec3& b, EffectorBodyState& out)
	{
	const NxReal* m = effectorRecordFloats(record, 0x134);
	const NxReal* c = effectorRecordFloats(record, 0x158);
	const NxReal* v = effectorRecordFloats(record, 0x34);
	const NxReal* w = effectorRecordFloats(record, 0x40);
	const double x = ((double)m[2] * b.z + (double)m[1] * b.y) + (double)b.x * m[0];
	const double y = ((double)m[5] * b.z + (double)m[3] * b.x) + (double)m[4] * b.y;
	const NxReal z = (NxReal)(((double)m[8] * b.z + (double)m[6] * b.x) + (double)m[7] * b.y);
	const NxReal px = (NxReal)(x + c[0]);
	const double py = y + c[1];
	const double pz = (double)z + c[2];
	out.point.x = px;
	out.point.y = (NxReal)py;
	out.point.z = (NxReal)pz;
	const NxReal rx = (NxReal)((double)px - c[0]);
	const NxReal ry = (NxReal)(py - c[1]);
	const double rz = pz - c[2];
	const NxReal cx = (NxReal)(rz * w[1] - (double)ry * w[2]);
	const NxReal cy = (NxReal)((double)rx * w[2] - rz * w[0]);
	const double cz = (double)ry * w[0] - (double)rx * w[1];
	out.velocity[0] = (NxReal)((double)cx + v[0]);
	out.velocity[1] = (NxReal)((double)cy + v[1]);
	out.velocity[2] = (NxReal)(cz + v[2]);
	}

// phys_fn_003979 (0x0008f700, 1019 B)
// Slot 3, reached only from the simulation step's pre-tick loop (000655,
// through slot 2). Each end's world point and velocity (the anchor as
// given and zero velocity for a null record); the separation D = p2 - p1
// (x and y stored, z stored with the unrounded value kept); its length by
// an inline fsqrt of ((dz*Dz + Dy*Dy) + Dx*Dx), stored as a float; the
// relative velocity (x, y stored; z kept) projected on the UNnormalised D
// and stored as 003972's argument; the damper force stored as a float,
// the spring force (003970) of the length added, the sum divided by the
// length; F = D * that, each component stored. Then for each non-null
// record whose chain root (000713) has a non-zero +0x1f8 (or a NaN, the
// listing's `jnp`): 000791 with F scaled by -step (body 1, the step size
// being the Scene's +0x548 float) or +step (body 2), the end's world
// point, 1 and 0. 000791 is a deferred stub; with both roots' +0x1f8 zero
// the row has no side effect beyond 000713's path compression.
void SpringAndDamperEffector::apply(NxFoundation::Observable* body1, NxFoundation::Observable* body2)
	{
	EffectorBodyState end1;
	if(body1)
		effectorBodyState1(body1, mLocalAnchor[0], end1);
	else
		{
		end1.point = mLocalAnchor[0];
		end1.velocity[0] = 0.0f;
		end1.velocity[1] = 0.0f;
		end1.velocity[2] = 0.0f;
		}
	EffectorBodyState end2;
	if(body2)
		effectorBodyState2(body2, mLocalAnchor[1], end2);
	else
		{
		end2.point = mLocalAnchor[1];
		end2.velocity[0] = 0.0f;
		end2.velocity[1] = 0.0f;
		end2.velocity[2] = 0.0f;
		}

	const NxReal dx = (NxReal)((double)end2.point.x - end1.point.x);
	const NxReal dy = (NxReal)((double)end2.point.y - end1.point.y);
	const double dzr = (double)end2.point.z - end1.point.z;
	const NxReal dz = (NxReal)dzr;
	const NxReal distance = (NxReal)x87FsqrtDot3(dzr, dz, dy, dy, dx, dx);
	const NxReal dvx = (NxReal)((double)end2.velocity[0] - end1.velocity[0]);
	const NxReal dvy = (NxReal)((double)end2.velocity[1] - end1.velocity[1]);
	const double dvz = (double)end2.velocity[2] - end1.velocity[2];
	const NxReal relative = (NxReal)((dvz * dz + (double)dvy * dy) + (double)dvx * dx);
	const NxReal damper = (NxReal)damperForce(relative);
	const double scale = (springForce(distance) + damper) / distance;
	const NxReal step = *effectorRecordFloats(mScene, 0x548);
	NxVec3 force((NxReal)(dx * scale), (NxReal)(dy * scale), (NxReal)(dz * scale));

	if(body1)
		{
		const Row000713Fixture* root = reinterpret_cast<Row000713Fixture*>(body1)->row000713();
		if(*effectorRecordFloats(root, 0x1f8) != 0.0f)
			{
			const double negated = -(double)step;
			const NxVec3 applied((NxReal)(force.x * negated), (NxReal)(force.y * negated),
				(NxReal)(force.z * negated));
			reinterpret_cast<Row000791Fixture*>(body1)->row000791(applied, end1.point, 1, 0);
			}
		}
	if(body2)
		{
		const Row000713Fixture* root = reinterpret_cast<Row000713Fixture*>(body2)->row000713();
		if(*effectorRecordFloats(root, 0x1f8) != 0.0f)
			{
			const NxVec3 applied((NxReal)((double)force.x * step), (NxReal)((double)force.y * step),
				(NxReal)((double)force.z * step));
			reinterpret_cast<Row000791Fixture*>(body2)->row000791(applied, end2.point, 1, 0);
			}
		}
	}
