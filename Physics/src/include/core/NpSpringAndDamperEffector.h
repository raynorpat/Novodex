#ifndef NX_PHYSICS_CORE_NPSPRINGANDDAMPEREFFECTOR
#define NX_PHYSICS_CORE_NPSPRINGANDDAMPEREFFECTOR
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The public NxSpringAndDamperEffector wrapper, 0x18 bytes
// (units/effector-coredump-contract.md "## Effector", "### Layouts"):
//
//   +0x00  primary vptr (table 0x1011794c, 8 slots)
//   +0x04  NxEffector::userData (the constructor does not write it)
//   +0x08  the 12-byte hook base (EmbeddedHookBase: vptr 0x10117948, whose
//          one slot is the adjustor thunk 003954 to the deleting destructor)
//   +0x0c  hook word 1 = the scene write-lock link (NpScene+0xc)
//   +0x10  hook word 2 = the scene read-lock link (NpScene+0x10)
//   +0x14  the internal SpringAndDamperEffector*
//
// The lock links sit one word lower than the joints' (+0x10/+0x14) because
// NxEffector has no appData. Setters tryLock the write link, getters lock
// the read link; both are passed BY VALUE to the nxNpSceneGuard* helpers.
//
// Primary table: [0] 003950 isSpringAndDamperEffector, [1] 003940 setBodies,
// [2] 003942 setLinearSpring, [3] 003946 getLinearSpring, [4] 003944
// setLinearDamper, [5] 003948 getLinearDamper, [6] 003950, [7] 003950. The
// public headers declare six virtuals; slots 6 and 7 are two more Np-level
// virtuals folded onto 003950's body (read lock, unlock, return this). The
// destructor is virtual only through the hook base, so it is not in the
// primary table.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxSpringAndDamperEffector.h"
#include "ObjectModel.h"
#include "NpSceneGuard.h"
#include "core/SpringAndDamperEffector.h"

#include <cstddef>

class NpSpringAndDamperEffector : public NxSpringAndDamperEffector, public EmbeddedHookBase
	{
	public:
	//! phys_fn_003958 (0x0008f150).
	explicit NpSpringAndDamperEffector(SpringAndDamperEffector* internal);
	//! phys_fn_003956 (0x0008f110), the scalar deleting destructor reached
	//! through the hook base's table (phys_fn_003954 is its adjustor thunk).
	virtual ~NpSpringAndDamperEffector();

	//! The free of the deleting destructor: the Foundation allocator's slot
	//! +0x14 (0x8f12e-0x8f138).
	static void operator delete(void* memory) { nxFoundationSDKAllocator->free(memory); }

	virtual NxSpringAndDamperEffector* isSpringAndDamperEffector();	// slot 0
	virtual void setBodies(NxActor* body1, const NxVec3& global1, NxActor* body2, const NxVec3& global2);
	virtual void setLinearSpring(NxReal distCompressSaturate, NxReal distRelaxed, NxReal distStretchSaturate,
		NxReal maxCompressForce, NxReal maxStretchForce);
	virtual void getLinearSpring(NxReal& distCompressSaturate, NxReal& distRelaxed, NxReal& distStretchSaturate,
		NxReal& maxCompressForce, NxReal& maxStretchForce);
	virtual void setLinearDamper(NxReal velCompressSaturate, NxReal velStretchSaturate,
		NxReal maxCompressForce, NxReal maxStretchForce);
	virtual void getLinearDamper(NxReal& velCompressSaturate, NxReal& velStretchSaturate,
		NxReal& maxCompressForce, NxReal& maxStretchForce);
	//! Slots 6 and 7: two Np-level virtuals the oracle folds onto slot 0's
	//! body (0x1011794c +0x18/+0x1c both hold 0x1008f0d0).
	virtual NxSpringAndDamperEffector* isSpringAndDamperEffectorSlot6();
	virtual NxSpringAndDamperEffector* isSpringAndDamperEffectorSlot7();

	//! phys_fn_003952 (0x0008f0f0): the internal effector at +0x14.
	SpringAndDamperEffector* getInternal();

	//! +0x14.
	SpringAndDamperEffector*	mInternal;

	private:
	void*			writeLink() const { return reinterpret_cast<void*>(mWord04); }
	void*			readLink() const { return reinterpret_cast<void*>(mWord08); }
	};

static_assert(sizeof(NpSpringAndDamperEffector) == 0x18, "003960 allocates 0x18 bytes");
static_assert(offsetof(NpSpringAndDamperEffector, userData) == 0x04, "NxEffector::userData is at +0x04");
static_assert(offsetof(NpSpringAndDamperEffector, mWord04) == 0x0c, "the scene write-lock link is at +0x0c");
static_assert(offsetof(NpSpringAndDamperEffector, mWord08) == 0x10, "the scene read-lock link is at +0x10");
static_assert(offsetof(NpSpringAndDamperEffector, mInternal) == 0x14, "the internal pointer is at +0x14");

#endif
