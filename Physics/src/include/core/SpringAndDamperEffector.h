#ifndef NX_PHYSICS_CORE_SPRINGANDDAMPEREFFECTOR
#define NX_PHYSICS_CORE_SPRINGANDDAMPEREFFECTOR
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal effector classes, recovered by the effector-and-coredump plan
// (docs/reconstruction/novodex-physics/units/effector-coredump-contract.md,
// "## Effector"). Three levels, each with its own table:
//
//   Effector                 table 0x10117920 [Observable::event, 003938, _purecall]
//   ActorPairEffector        table 0x101178f8 [003928, 003932, 003924, _purecall]
//   SpringAndDamperEffector  table 0x101179e4 [003928, 003977, 003924, 003979]
//
// Effector derives from NxFoundation::Observable (0x14 bytes: vptr, the
// observer array {first, last, memEnd} and its allocator word), whose one
// virtual, event(), is slot 0. Slot 1 is the deleting destructor, slot 2 the
// per-tick entry the Scene's pre-tick loop 000655 calls, slot 3 (from
// ActorPairEffector on) the force application on the two body records.
//
// The two observed body records are `[actorBody+8]`, the 0x260-byte dynamic
// records the candidate builds in nxActorComputeMass (Scene.cpp); their
// Observable part at +0 is what the effector registers with.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "Observable.h"
#include "NxVec3.h"

#include <cstddef>

class NxSceneInternal;
class NpSpringAndDamperEffector;

// The effector allocations and frees go through the Foundation's allocator
// (`[[0x101041bc]]`, nxFoundationSDKAllocator): 000587 allocates (0x68, 0)
// through its slot +8 and every deleting destructor frees through slot +0x14.
// Observable's base NxAllocateable declares only its own operator new
// overloads, which hide the global placement form, so the class declares the
// placement form it is constructed with and the delete that matches it.
class Effector : public NxFoundation::Observable
	{
	public:
	//! phys_fn_003934 (0x0008ee80). The Observable constructor (import
	//! 0x10104190), +0x1c = scene, +0x18 = 0.
	explicit Effector(NxSceneInternal* scene);
	//! phys_fn_003936 (0x0008eeb0): ~Observable (import 0x10104194). Slot 1
	//! is the compiler's scalar deleting destructor, phys_fn_003938.
	virtual ~Effector();
	//! Slot 2: the per-tick entry (pure here, _purecall in 0x10117920).
	virtual void tick() = 0;

	static void* operator new(size_t, void* memory) { return memory; }
	static void operator delete(void*, void*) {}
	static void operator delete(void* memory) { nxFoundationSDKAllocator->free(memory); }

	//! +0x14. No effector row writes this word.
	NxU32						mPad14;
	//! +0x18. Next effector in the Scene's list at +0x5a4 (003934 zeroes
	//! it; 000587 pushes at the head; 000573 and 000575 unlink).
	Effector*					mNext;
	//! +0x1c. The owning Scene (the constructor argument). 003979 reads the
	//! step size at its +0x548.
	NxSceneInternal*			mScene;
	//! +0x20. The public object: 003960 stores the Np wrapper here (0 when
	//! its allocation fails); 000301 and 000331 read it.
	NpSpringAndDamperEffector*	mPublicObject;
	};

class ActorPairEffector : public Effector
	{
	public:
	//! phys_fn_003922 (0x0008ed20). Effector's constructor, then both body
	//! records zeroed.
	explicit ActorPairEffector(NxSceneInternal* scene);
	//! Slot 0, phys_fn_003928 (0x0008edb0): event 0x100 from a body record
	//! going away (the notify in the record teardown) nulls the matching
	//! record pointer.
	virtual void event(NxU32 code, NxFoundation::Observable& sender);
	//! phys_fn_003930 (0x0008ede0); slot 1 is phys_fn_003932, the deleting
	//! destructor.
	virtual ~ActorPairEffector();
	//! Slot 2, phys_fn_003924 (0x0008ed50): slot 3 on the two records.
	virtual void tick();
	//! Slot 3: apply the effect to the two body records (pure here).
	virtual void apply(NxFoundation::Observable* body1, NxFoundation::Observable* body2) = 0;

	//! phys_fn_003926 (0x0008ed60): stop observing the old records, store the
	//! new ones, observe them.
	void setBodyRecords(NxFoundation::Observable* body1, NxFoundation::Observable* body2);

	//! +0x24 / +0x28. The observed body records, null for the world.
	NxFoundation::Observable*	mBody[2];
	};

class SpringAndDamperEffector : public ActorPairEffector
	{
	public:
	//! phys_fn_003960 (0x0008f180). Zeroes the anchors and the spring and
	//! damper words, then allocates (0x18, 0) and constructs the Np wrapper.
	explicit SpringAndDamperEffector(NxSceneInternal* scene);
	//! phys_fn_003977 (0x0008f6c0), the deleting destructor (slot 1): the Np
	//! wrapper's own deleting destructor through its hook member's table,
	//! then ActorPairEffector's destructor body, then the free.
	virtual ~SpringAndDamperEffector();
	//! Slot 3, phys_fn_003979 (0x0008f700).
	virtual void apply(NxFoundation::Observable* body1, NxFoundation::Observable* body2);

	//! phys_fn_003962 (0x0008f200). The arguments are internal actor bodies
	//! (NxActor +0x14); each anchor is stored in its record's frame, or as
	//! given when the body has no record.
	void setBodies(void* actorBody1, const NxVec3& global1, void* actorBody2, const NxVec3& global2);
	//! phys_fn_003964 (0x0008f390). The core dump's reader: the records'
	//! owners (+0x19c) and the anchors in world space. Both records are
	//! dereferenced before any null test, as in the listing.
	void getBodies(void** owner1, NxVec3& global1, void** owner2, NxVec3& global2);
	//! phys_fn_003966 (0x0008f4f0).
	void setLinearSpring(NxReal distCompressSaturate, NxReal distRelaxed, NxReal distStretchSaturate,
		NxReal maxCompressForce, NxReal maxStretchForce);
	//! phys_fn_003968 (0x0008f520).
	void setLinearDamper(NxReal velCompressSaturate, NxReal velStretchSaturate,
		NxReal maxCompressForce, NxReal maxStretchForce);
	//! phys_fn_003970 (0x0008f540). Returns on the x87 stack, unrounded.
	double springForce(NxReal distance);
	//! phys_fn_003972 (0x0008f5e0). Returns on the x87 stack, unrounded.
	double damperForce(NxReal velocity);
	//! phys_fn_003974 (0x0008f660).
	void getLinearSpring(NxReal& distCompressSaturate, NxReal& distRelaxed, NxReal& distStretchSaturate,
		NxReal& maxCompressForce, NxReal& maxStretchForce);
	//! phys_fn_003975 (0x0008f690).
	void getLinearDamper(NxReal& velCompressSaturate, NxReal& velStretchSaturate,
		NxReal& maxCompressForce, NxReal& maxStretchForce);

	//! +0x2c / +0x38. Anchor 1 and 2, each in its record's frame (the
	//! transposed +0x134 3x3 applied to the global point less +0x158), or
	//! the global point when the body has no record.
	NxVec3						mLocalAnchor[2];
	//! +0x44..+0x54. The spring (setLinearSpring's argument order).
	NxReal						mDistCompressSaturate;
	NxReal						mDistRelaxed;
	NxReal						mDistStretchSaturate;
	NxReal						mSpringMaxCompressForce;
	NxReal						mSpringMaxStretchForce;
	//! +0x58..+0x64. The damper (setLinearDamper's argument order).
	NxReal						mVelCompressSaturate;
	NxReal						mVelStretchSaturate;
	NxReal						mDamperMaxCompressForce;
	NxReal						mDamperMaxStretchForce;
	};

static_assert(sizeof(NxFoundation::Observable) == 0x14, "the Observable base is 0x14 bytes");
static_assert(offsetof(Effector, mPad14) == 0x14, "pad word at +0x14");
static_assert(offsetof(Effector, mNext) == 0x18, "scene list link at +0x18");
static_assert(offsetof(Effector, mScene) == 0x1c, "scene at +0x1c");
static_assert(offsetof(Effector, mPublicObject) == 0x20, "public object at +0x20");
static_assert(offsetof(ActorPairEffector, mBody) == 0x24, "body records at +0x24/+0x28");
static_assert(offsetof(SpringAndDamperEffector, mLocalAnchor) == 0x2c, "anchors at +0x2c/+0x38");
static_assert(offsetof(SpringAndDamperEffector, mDistCompressSaturate) == 0x44, "spring at +0x44");
static_assert(offsetof(SpringAndDamperEffector, mVelCompressSaturate) == 0x58, "damper at +0x58");
static_assert(sizeof(SpringAndDamperEffector) == 0x68, "000587 allocates 0x68 bytes");

#endif
