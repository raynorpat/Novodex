/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpSpringAndDamperEffector.h"
#include "FoundationSDK.h"

// The public spring-and-damper effector (effector-and-coredump Task 2; see
// core/NpSpringAndDamperEffector.h and units/effector-coredump-contract.md
// "## Effector"). Rows 003940-003958. The setters tryLock the scene write
// link (+0x0c) and report a failure with this unit's __FILE__ (.rdata
// 0x1011796c) and their own line; the getters lock the read link (+0x10).

#define NX_NPSPRINGANDDAMPEREFFECTOR_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpSpringAndDamperEffector.cpp"

// The failed-tryLock report: code 2, this unit's file, the row's line, the
// shared message (.rdata 0x10104760). The listing's instance test with int3
// before it is the error call's own guard.
static void nxEffectorReportWriteLocked(int line)
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_NPSPRINGANDDAMPEREFFECTOR_CPP,
		line, 0, "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
	}

// The internal actor body of a public actor (NxActor +0x14), or 0.
static NX_INLINE void* nxEffectorActorBody(NxActor* actor)
	{
	return actor ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(actor) + 0x14) : 0;
	}

// phys_fn_003940 (0x0008eef0, 121 B)
// Line 0x1b. The write link is loaded after the lock for the unlock; body 2's
// internal body is read before body 1's, as the listing does.
void NpSpringAndDamperEffector::setBodies(NxActor* body1, const NxVec3& global1, NxActor* body2,
	const NxVec3& global2)
	{
	if(!nxNpSceneGuardWriteTry(writeLink()))
		{
		nxEffectorReportWriteLocked(0x1b);
		return;
		}
	void* link = writeLink();
	void* actorBody2 = nxEffectorActorBody(body2);
	void* actorBody1 = nxEffectorActorBody(body1);
	mInternal->setBodies(actorBody1, global1, actorBody2, global2);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003942 (0x0008ef70, 104 B)
// Line 0x22.
void NpSpringAndDamperEffector::setLinearSpring(NxReal distCompressSaturate, NxReal distRelaxed,
	NxReal distStretchSaturate, NxReal maxCompressForce, NxReal maxStretchForce)
	{
	if(!nxNpSceneGuardWriteTry(writeLink()))
		{
		nxEffectorReportWriteLocked(0x22);
		return;
		}
	void* link = writeLink();
	mInternal->setLinearSpring(distCompressSaturate, distRelaxed, distStretchSaturate, maxCompressForce,
		maxStretchForce);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003944 (0x0008efe0, 99 B)
// Line 0x29.
void NpSpringAndDamperEffector::setLinearDamper(NxReal velCompressSaturate, NxReal velStretchSaturate,
	NxReal maxCompressForce, NxReal maxStretchForce)
	{
	if(!nxNpSceneGuardWriteTry(writeLink()))
		{
		nxEffectorReportWriteLocked(0x29);
		return;
		}
	void* link = writeLink();
	mInternal->setLinearDamper(velCompressSaturate, velStretchSaturate, maxCompressForce, maxStretchForce);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003946 (0x0008f050, 59 B)
void NpSpringAndDamperEffector::getLinearSpring(NxReal& distCompressSaturate, NxReal& distRelaxed,
	NxReal& distStretchSaturate, NxReal& maxCompressForce, NxReal& maxStretchForce)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	mInternal->getLinearSpring(distCompressSaturate, distRelaxed, distStretchSaturate, maxCompressForce,
		maxStretchForce);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003948 (0x0008f090, 54 B)
void NpSpringAndDamperEffector::getLinearDamper(NxReal& velCompressSaturate, NxReal& velStretchSaturate,
	NxReal& maxCompressForce, NxReal& maxStretchForce)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	mInternal->getLinearDamper(velCompressSaturate, velStretchSaturate, maxCompressForce, maxStretchForce);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_003950 (0x0008f0d0, 26 B)
// The read lock and its release bracket nothing; returns this. The oracle
// points slots 0, 6 and 7 at this one body.
NxSpringAndDamperEffector* NpSpringAndDamperEffector::isSpringAndDamperEffector()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	nxNpSceneGuardLeave(link);
	return this;
	}

// Slot 6: 003950's body again (0x1011794c +0x18).
NxSpringAndDamperEffector* NpSpringAndDamperEffector::isSpringAndDamperEffectorSlot6()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	nxNpSceneGuardLeave(link);
	return this;
	}

// Slot 7: 003950's body again (0x1011794c +0x1c).
NxSpringAndDamperEffector* NpSpringAndDamperEffector::isSpringAndDamperEffectorSlot7()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	nxNpSceneGuardLeave(link);
	return this;
	}

// phys_fn_003952 (0x0008f0f0, 4 B)
SpringAndDamperEffector* NpSpringAndDamperEffector::getInternal()
	{
	return mInternal;
	}

// phys_fn_003954 (0x0008f100, 8 B)
// Not defined here: the compiler-generated adjustor thunk ("sub ecx,8; jmp
// <deleting destructor>") the hook base's table needs for the virtual
// destructor, emitted once ~NpSpringAndDamperEffector() is defined.

// phys_fn_003956 (0x0008f110, 49 B)
// Nothing in the body: the table stores (0x1011794c, then the hook's own
// 0x10117948 and its base destructor phys_fn_002406) are the base-destruction
// chain, and the compiler's scalar deleting destructor frees through the
// class operator delete (the Foundation allocator, slot +0x14).
NpSpringAndDamperEffector::~NpSpringAndDamperEffector()
	{
	}

// phys_fn_003958 (0x0008f150, 46 B)
// NxEffector's userData is not written (the listing stores only the tables,
// the hook's two words through 002404, and +0x14). The transient table
// 0x101179ac (six _purecall) is the abstract NxSpringAndDamperEffector's.
NpSpringAndDamperEffector::NpSpringAndDamperEffector(SpringAndDamperEffector* internal)
	{
	mWord04 = 0;
	mWord08 = 0;
	mInternal = internal;
	}
