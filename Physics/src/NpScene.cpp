/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The NxScene wrapper. Its layout is measured from phys_fn_000285 (0x0000c310);
// see NpScene.h for the offsets.
//
// The Scene lock links follow the oracle's two-allocation shape. The
// CRITICAL_SECTION lifecycle and guard protocol are connected to the actor
// virtuals; the condition-object behavior remains a reproduction hole.
//
// The forwarding slots are the point of this class. phys_fn_000293 (0x0000c490)
// is the shape of all of them, and it is transcribed:
//
//     uVar3 = tryLock(scene->mWriteLock);          // +0xc
//     if (uVar3) {
//         piVar1 = scene->mWriteLock;
//         piVar5 = FUN_10011730(param_1);           // the Scene-side call
//         if (piVar5 != 0) { iVar4 = *piVar5; unlock(piVar1); return iVar4; }
//         unlock(piVar1);
//         return 0;
//     }
//     error(2, "NpScene.cpp", 0x69,
//           "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to
//            avoid a deadlock!");
//     return 0;

#include "NpScene.h"
#include "NpSceneGuard.h"
#include <stdio.h>
#include <string.h>

#include "Scene.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxJointDesc.h"
#include "NxJoint.h"
#include "FoundationSDK.h"
#include "core/Joint.h"
#include "core/SpringAndDamperEffector.h"
#include "core/NpSpringAndDamperEffector.h"

// ---------------------------------------------------------------------------
// Lock helpers and the remaining condition-object reproduction hole.
// ---------------------------------------------------------------------------
static void* nxLockConstruct(void* memory);
static bool nxLockTryLock(void* lock);
static bool nxLockUnlock(void* lock);
static void* nxConditionConstruct(void* memory, void* a, void* b, void* c);
// The deadlock report, defined in Scene.cpp.
void nxSceneDeadlockReport();

NpScene::NpScene(NxSceneInternal* scene)
	{
	mWriteLock = 0;
	mReadLock = 0;
	mCondition = 0;
	mFlag = 0;
	mScene = scene;

	// The inner lock at +8, then the two locks at +0xc and +0x10 through
	// phys_fn_0005b6a0, then the two at +0x14 and +0x18, then the 0x18-byte object
	// at +0x1c linked to them.
	//
	// Each field points to a 4-byte link, whose word points to a 0x20-byte lock
	// block. The actor wrapper copies these link pointers into +0x0c/+0x10.
	// Allocating only the four-byte link without its inner block was the old
	// constructor overrun; allocating only the block made the public alias's
	// allocation size wrong.
	static const NxU32 kNpSceneLockBlock = 0x20;

	mWriteLock = nxGetSdkAllocator()->malloc(4, NX_MEMORY_PERSISTENT);
	if(mWriteLock)
		{
		void* block = nxGetSdkAllocator()->malloc(kNpSceneLockBlock, NX_MEMORY_PERSISTENT);
		*static_cast<void**>(mWriteLock) = block ? nxLockConstruct(block) : 0;
		}

	mReadLock = nxGetSdkAllocator()->malloc(4, NX_MEMORY_PERSISTENT);
	if(mReadLock)
		{
		void* block = nxGetSdkAllocator()->malloc(kNpSceneLockBlock, NX_MEMORY_PERSISTENT);
		*static_cast<void**>(mReadLock) = block ? nxLockConstruct(block) : 0;
		}

	mCondition = nxGetSdkAllocator()->malloc(0x18, NX_MEMORY_PERSISTENT);
	if(mCondition)
		mCondition = nxConditionConstruct(mCondition, mLockB, mLockA, 0);
	}

NpScene::~NpScene()
	{
	if(mCondition)
		{
		nxGetSdkAllocator()->free(*reinterpret_cast<void**>(
			static_cast<unsigned char*>(mCondition) + 4));
		nxGetSdkAllocator()->free(mCondition);
		}
	if(mReadLock)
		{
		if(*static_cast<void**>(mReadLock))
			::DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(
				*static_cast<void**>(mReadLock)));
		nxGetSdkAllocator()->free(*static_cast<void**>(mReadLock));
		nxGetSdkAllocator()->free(mReadLock);
		}
	if(mWriteLock)
		{
		if(*static_cast<void**>(mWriteLock))
			::DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(
				*static_cast<void**>(mWriteLock)));
		nxGetSdkAllocator()->free(*static_cast<void**>(mWriteLock));
		nxGetSdkAllocator()->free(mWriteLock);
		}
	}

// phys_fn_000293 (0x0000c490): the forwarding shape every slot in this class has.
// The oracle's `this + 0xc` is mWriteLock, and its `this + 0x24` is mScene.
NxActor* NpScene::createActor(const NxActorDescBase& desc)
	{
	if(!mWriteLock || !nxLockTryLock(mWriteLock))
		{
		nxSceneDeadlockReport();
		return 0;
		}

	NxActor* actor = mScene->createActor(desc);

	nxLockUnlock(mWriteLock);
	return actor;
	}

// phys_fn_000295 at 0x0000c500: the same lock, then Scene::releaseActor.
void NpScene::releaseActor(NxActor& actor)
	{
	if(!mWriteLock || !nxLockTryLock(mWriteLock))
		{
		nxSceneDeadlockReport();
		return;
		}

	void* body = *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(&actor) + 0x14);
	if(body)
		mScene->releaseActor(body);
	nxLockUnlock(mWriteLock);
	}

void NpScene::release()
	{
	// The oracle's NpScene release forwards to the Scene's release, which is a
	// Phase 3 row and not reconstructed.
	}

// ---------------------------------------------------------------------------
// Reproduction holes.
// ---------------------------------------------------------------------------

static void* nxLockConstruct(void* memory)
	{
	// phys_fn_0005b6a0 zeroes the writer flag at +0x18 and initializes
	// the CRITICAL_SECTION occupying the first 0x18 bytes.
	memset(memory, 0, 0x20);
	::InitializeCriticalSection(static_cast<CRITICAL_SECTION*>(memory));
	return memory;
	}

static bool nxLockTryLock(void* lock)
	{
	return nxNpSceneGuardWriteTry(lock);
	}

static bool nxLockUnlock(void* lock)
	{
	nxNpSceneGuardLeave(lock);
	return true;
	}

static void* nxConditionConstruct(void* memory, void* a, void* b, void* c)
	{
	(void)a; (void)b; (void)c;
	memset(memory, 0, 0x18);
	void* state = nxGetSdkAllocator()->malloc(0x14, NX_MEMORY_PERSISTENT);
	if(state) memset(state, 0, 0x14);
	*reinterpret_cast<void**>(static_cast<unsigned char*>(memory) + 4) = state;
	return memory;
	}

// ---------------------------------------------------------------------------
// The remaining NxScene virtuals, UNIMPLEMENTED.
//
// NpScene must be concrete to be instantiated, and NxScene declares 65 pure
// virtuals. Only createActor and releaseActor above are reconstructed; every
// definition below is an empty body returning a default. None is claimed as
// reconstructed and none is gated.
// ---------------------------------------------------------------------------


// phys_fn_000297 (0x0000c460, 39 B): read-lock the Scene gravity copy.
void NpScene::getGravity(NxVec3& gravity)
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	mScene->getGravity(gravity);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000299 (0x0000c5d0, 87 B, phase 7): NxScene::releaseJoint. The
// write lock at +0xc through phys_fn_002364; on failure the Foundation
// instance test with int3 and the deadlock report (code 2, NpScene.cpp line
// 0x7f). Otherwise Scene::releaseJoint (phys_fn_000653) on the joint's
// appData word (+0x08, the internal Joint), then phys_fn_002366 on the link
// value loaded before the call.
void NpScene::releaseJoint(NxJoint& joint)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x7f, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->releaseJoint(static_cast<Joint*>(joint.appData));
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000301 (0x0000c630, 140 B, phase 7):
// NxScene::createSpringAndDamperEffector. The write lock at +0xc through
// phys_fn_002364; on failure the deadlock report (code 2, NpScene.cpp line
// 0x87) and 0. Otherwise Scene::createSpringAndDamperEffector
// (phys_fn_000587). A null internal effector returns 0. An internal effector
// with a public object gets the scene's read link (+0x10) at np+0x10 and its
// write link (+0xc) at np+0xc and the public object is returned; one whose
// public object is null (its allocation failed) is released again through
// Scene::releaseEffector (phys_fn_000594) and 0 is returned. The unlock is
// on the link value loaded before the call.
NxSpringAndDamperEffector* NpScene::createSpringAndDamperEffector(const NxSpringAndDamperEffectorDesc& desc)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x87, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return 0;
		}
	void* link = mWriteLock;
	SpringAndDamperEffector* effector = mScene->createSpringAndDamperEffector(desc);
	if(effector)
		{
		NpSpringAndDamperEffector* np = effector->mPublicObject;
		if(np)
			{
			np->mWord08 = reinterpret_cast<NxU32>(mReadLock);
			np->mWord04 = reinterpret_cast<NxU32>(mWriteLock);
			nxNpSceneGuardLeave(link);
			return np;
			}
		mScene->releaseEffector(effector);
		}
	nxNpSceneGuardLeave(link);
	return 0;
	}

// phys_fn_000303 (0x0000c6c0, 92 B, phase 7): NxScene::releaseEffector. The
// write lock at +0xc; on failure the deadlock report (code 2, NpScene.cpp
// line 0x9a). Otherwise the internal effector at the public object's +0x14
// (phys_fn_003952, called on the argument as the spring-and-damper
// wrapper, the only effector type) goes to Scene::releaseEffector
// (phys_fn_000594), then the unlock on the link value loaded before it.
void NpScene::releaseEffector(NxEffector& effector)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x9a, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->releaseEffector(static_cast<NpSpringAndDamperEffector&>(effector).getInternal());
	nxNpSceneGuardLeave(link);
	}

// (unimplemented) createController
NxController* NpScene::createController(const NxControllerDesc&)
	{
	return 0;
	}

// (unimplemented) releaseController
void NpScene::releaseController(NxController&)
	{
	
	}

// (unimplemented) setActorPairFlags
void NpScene::setActorPairFlags(NxActor&, NxActor&, NxU32 nxContactPairFlag)
	{
	
	}

// (unimplemented) getActorPairFlags
NxU32 NpScene::getActorPairFlags(NxActor&, NxActor&) const
	{
	return 0;
	}

// (unimplemented) setShapePairFlags
void NpScene::setShapePairFlags(NxShape&, NxShape&, NxU32 nxContactPairFlag)
	{
	
	}

// (unimplemented) getShapePairFlags
NxU32 NpScene::getShapePairFlags(NxShape&, NxShape&) const
	{
	return 0;
	}

// (unimplemented) getNbPairs
NxU32 NpScene::getNbPairs() const
	{
	return 0;
	}

// (unimplemented) getPairFlagArray
bool NpScene::getPairFlagArray(NxPairFlag* userArray, NxU32 numPairs) const
	{
	return 0;
	}

// The Scene actor array begins at internal +0x55c. The wrapper forwards the
// count as the distance from its first pointer to its last pointer.
NxU32 NpScene::getNbActors() const
	{
	const NxActor* const* first = mScene->at<NxActor**>(0x55c);
	const NxActor* const* last = mScene->at<NxActor**>(0x560);
	return first ? static_cast<NxU32>(last - first) : 0;
	}

// The public pointer is the Scene's existing contiguous actor array.
NxActor** NpScene::getActors()
	{
	return mScene->at<NxActor**>(0x55c);
	}

// phys_fn_000321 (0x0000c910, 36 B, phase 7): NxScene::getNbJoints. The
// read lock at +0x10 (phys_fn_002362 / phys_fn_002366, the link value loaded
// once) around Scene::getNbJoints (phys_fn_000559).
NxU32 NpScene::getNbJoints() const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 count = mScene->getNbJoints();
	nxNpSceneGuardLeave(link);
	return count;
	}

// phys_fn_000323 (0x0000c940, 31 B, phase 7): NxScene::resetJointIterator,
// the same lock around Scene::resetJointIterator (phys_fn_000563); the
// unlock is a tail jump.
void NpScene::resetJointIterator()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	mScene->resetJointIterator();
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000325 (0x0000c960, 55 B, phase 7): NxScene::getNextJoint, the
// same lock around Scene::getNextJoint (phys_fn_000567); a joint is returned
// as its public object ([internal+0x48], 0xc97a), the end as 0.
NxJoint * NpScene::getNextJoint()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	Joint* joint = mScene->getNextJoint();
	NxJoint* result = joint ? static_cast<NxJoint*>(joint->mPublicObject) : 0;
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000327 (0x0000c9a0, 36 B, phase 7): NxScene::getNbEffectors. The
// read lock at +0x10 (phys_fn_002362 / phys_fn_002366, the link value loaded
// once) around Scene::getNbEffectors (phys_fn_000561).
NxU32 NpScene::getNbEffectors() const
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	const NxU32 count = mScene->getNbEffectors();
	nxNpSceneGuardLeave(link);
	return count;
	}

// phys_fn_000329 (0x0000c9d0, 31 B, phase 7): NxScene::resetEffectorIterator,
// the same lock around Scene::resetEffectorIterator (phys_fn_000565); the
// unlock is a tail jump.
void NpScene::resetEffectorIterator()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	mScene->resetEffectorIterator();
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000331 (0x0000c9f0, 55 B, phase 7): NxScene::getNextEffector, the
// same lock around Scene::getNextEffector (phys_fn_000569); an effector is
// returned as its public object ([internal+0x20], 0xca0a), the end as 0.
NxEffector * NpScene::getNextEffector()
	{
	void* link = mReadLock;
	nxNpSceneGuardEnter(link);
	Effector* effector = mScene->getNextEffector();
	NxEffector* result = effector ? effector->mPublicObject : 0;
	nxNpSceneGuardLeave(link);
	return result;
	}

// (unimplemented) flushStream
void NpScene::flushStream()
	{
	
	}

// (unimplemented) startRun
void NpScene::startRun(NxReal elapsedTime)
	{
	
	}

// (unimplemented) finishRun
void NpScene::finishRun()
	{
	
	}

// (unimplemented) setTiming
void NpScene::setTiming(NxReal maxTimestep, NxU32 maxIter, NxTimeStepMethod method)
	{
	
	}

// (unimplemented) getTiming
void NpScene::getTiming(NxReal & maxTimestep, NxU32 & maxIter, NxTimeStepMethod & method) const
	{
	
	}

// (unimplemented) runFor
void NpScene::runFor(NxReal elapsedTime, NxReal maxTimestep, NxU32 maxIter, NxTimeStepMethod method)
	{
	
	}

// phys_fn_000344 (0x0000cc10, 77 B)
// NxScene::visualize (slot 31 of the table at .rdata:0x10105a98). The write
// lock at +0xc (phys_fn_002364); on failure the deadlock report (code 2, line
// 0x13c) and a plain return, no unlock. Otherwise the link is kept, the Scene
// row on +0x24 runs (phys_fn_000657), and the unlock (phys_fn_002366, a tail
// jump in the image) is on the kept link. Scene-raycast block Task 4.
void NpScene::visualize()
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x13c, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->visualize();
	nxNpSceneGuardLeave(link);
	}

// (unimplemented) getSceneStats
NxSceneStats* NpScene::getSceneStats()
	{
	return 0;
	}

// (unimplemented) getLimits
void NpScene::getLimits(NxSceneLimits& limits) const
	{
	
	}

// (unimplemented) setUserNotify
void NpScene::setUserNotify(NxUserNotify* callback)
	{
	
	}

// (unimplemented) getUserNotify
NxUserNotify* NpScene::getUserNotify() const
	{
	return 0;
	}

// (unimplemented) setUserTriggerReport
void NpScene::setUserTriggerReport(NxUserTriggerReport* callback)
	{
	
	}

// (unimplemented) getUserTriggerReport
NxUserTriggerReport* NpScene::getUserTriggerReport() const
	{
	return 0;
	}

// (unimplemented) setUserContactReport
void NpScene::setUserContactReport(NxUserContactReport* callback)
	{
	
	}

// (unimplemented) getUserContactReport
NxUserContactReport* NpScene::getUserContactReport() const
	{
	return 0;
	}

// (unimplemented) setUserFluidContactReport
void NpScene::setUserFluidContactReport(NxUserFluidContactReport* callback)
	{
	
	}

// (unimplemented) getUserFluidContactReport
NxUserFluidContactReport* NpScene::getUserFluidContactReport() const
	{
	return 0;
	}

// The six NxScene raycasts (slots 42-47 of the NpScene table at
// .rdata:0x10105a98). Each takes the write lock at +0xc (phys_fn_002364); on
// failure it reports the deadlock (code 2, this file's line) and returns a
// null result without unlocking. Otherwise the lock link is kept, maxDist must
// be greater than zero (the fcomp against 0.0f, 0x1000cf11: a NaN fails),
// else error 1 on the next line; the Scene row on +0x24 runs, then the unlock
// (phys_fn_002366) on the kept link. Scene-raycast block Task 3.
static const char* const kNpSceneFile = "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp";
static const char* const kNpSceneDeadlock =
	"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!";

static inline void nxNpSceneRaycastError(NxErrorCode code, int line, const char* message)
	{
	NxFoundation::FoundationSDK::getInstance().error(code, kNpSceneFile, line, 0, message);
	}

// phys_fn_000366 (0x0000ced0, 181 B)
bool NpScene::raycastAnyBounds(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x18b, kNpSceneDeadlock);
		return false;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x18c,
			"Scene::raycastAnyBounds: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return false;
		}
	const bool result = mScene->raycastAnyBounds(worldRay, shapesType, groups, maxDist);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000368 (0x0000cf90, 181 B)
bool NpScene::raycastAnyShape(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x197, kNpSceneDeadlock);
		return false;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x198,
			"Scene::raycastAnyShape: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return false;
		}
	const bool result = mScene->raycastAnyShape(worldRay, shapesType, groups, maxDist);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000370 (0x0000d050, 189 B)
NxU32 NpScene::raycastAllBounds(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x1a2, kNpSceneDeadlock);
		return 0;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x1a3,
			"Scene::raycastAllBounds: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return 0;
		}
	const NxU32 result = mScene->raycastAllBounds(worldRay, report, shapesType, groups, maxDist, hintFlags);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000372 (0x0000d110, 189 B)
NxU32 NpScene::raycastAllShapes(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x1ad, kNpSceneDeadlock);
		return 0;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x1ae,
			"Scene::raycastAllShapes: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return 0;
		}
	const NxU32 result = mScene->raycastAllShapes(worldRay, report, shapesType, groups, maxDist, hintFlags);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000374 (0x0000d1d0, 210 B)
// maxDist is checked, then the Scene row gets FLT_MAX and hint flags
// 0xffffffff in their place (0x1000d263, 0x1000d265). A hit returns the
// shape's public object (+0x9c).
NxShape* NpScene::raycastClosestBounds(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 /*hintFlags*/) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x1b9, kNpSceneDeadlock);
		return 0;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x1ba,
			"Scene::raycastClosestBounds: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return 0;
		}
	void* shape = mScene->raycastClosestBounds(worldRay, shapeType, hit, groups, NX_MAX_F32, 0xffffffff);
	NxShape* result = shape
		? *reinterpret_cast<NxShape**>(static_cast<unsigned char*>(shape) + 0x9c)
		: 0;
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_000376 (0x0000d2b0, 213 B)
NxShape* NpScene::raycastClosestShape(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		nxNpSceneRaycastError(NXE_INVALID_OPERATION, 0x1c6, kNpSceneDeadlock);
		return 0;
		}
	void* link = mWriteLock;
	if(!(maxDist > 0.0f))
		{
		nxNpSceneRaycastError(NXE_INVALID_PARAMETER, 0x1c7,
			"Scene::raycastClosestShape: The maximum distance must be greater than zero!");
		nxNpSceneGuardLeave(link);
		return 0;
		}
	void* shape = mScene->raycastClosestShape(worldRay, shapeType, hit, groups, maxDist, hintFlags);
	NxShape* result = shape
		? *reinterpret_cast<NxShape**>(static_cast<unsigned char*>(shape) + 0x9c)
		: 0;
	nxNpSceneGuardLeave(link);
	return result;
	}

// (unimplemented) overlapSphereShapes
NxU32 NpScene::overlapSphereShapes(const NxSphere& worldSphere, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	return 0;
	}

// (unimplemented) overlapAABBShapes
NxU32 NpScene::overlapAABBShapes(const NxBounds3& worldBounds, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	return 0;
	}

// (unimplemented) cullShapes
NxU32 NpScene::cullShapes(NxU32 nbPlanes, const NxPlane* worldPlanes, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback)
	{
	return 0;
	}

// (unimplemented) checkOverlapSphere
bool NpScene::checkOverlapSphere(const NxSphere& worldSphere, NxShapesType shapeType)
	{
	return 0;
	}

// (unimplemented) checkOverlapAABB
bool NpScene::checkOverlapAABB(const NxBounds3& worldBounds, NxShapesType shapeType)
	{
	return 0;
	}

// (unimplemented) overlapAABBTriangles
NxU32 NpScene::overlapAABBTriangles(const NxBounds3& worldBounds, NxArraySDK<NxTriangle>& worldTriangles)
	{
	return 0;
	}

// (unimplemented) createFluid
NxFluid* NpScene::createFluid(const NxFluidDesc&)
	{
	return 0;
	}

// (unimplemented) releaseFluid
void NpScene::releaseFluid(NxFluid&)
	{
	
	}

// (unimplemented) getNbFluids
NxU32 NpScene::getNbFluids() const
	{
	return 0;
	}

// (unimplemented) getFluids
NxFluid** NpScene::getFluids()
	{
	return 0;
	}

// (unimplemented) createImplicitMesh
NxImplicitMesh* NpScene::createImplicitMesh(const NxImplicitMeshDesc&)
	{
	return 0;
	}

// (unimplemented) releaseImplicitMesh
void NpScene::releaseImplicitMesh(NxImplicitMesh&)
	{
	
	}

// (unimplemented) getNbImplicitMeshes
NxU32 NpScene::getNbImplicitMeshes() const
	{
	return 0;
	}

// (unimplemented) getImplicitMeshes
NxImplicitMesh** NpScene::getImplicitMeshes()
	{
	return 0;
	}

// (unimplemented) wait
bool NpScene::wait(NxStandardFences, bool block)
	{
	return 0;
	}

// (unimplemented) isWritable
bool NpScene::isWritable()
	{
	return 0;
	}

// (unimplemented) simulate
void NpScene::simulate(NxReal elapsedTime)
	{
	
	}

// (unimplemented) checkResults
bool NpScene::checkResults(NxSimulationStatus, bool block )
	{
	return 0;
	}

// (unimplemented) fetchResults
bool NpScene::fetchResults(NxSimulationStatus, bool block )
	{
	return 0;
	}


// phys_fn_000287 (0x0000c400, 84 B): try the write lock, forward the three
// gravity words to Scene+0x520, then release the same lock link.
void NpScene::setGravity(const NxVec3& gravity)
	{
	if(!nxNpSceneGuardWriteTry(mWriteLock))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpScene.cpp", 0x5b, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	void* link = mWriteLock;
	mScene->setGravity(gravity);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_000295's shape: the write lock, the forward, the release.
NxJoint* NpScene::createJoint(const NxJointDesc& desc)
	{
	if(!mWriteLock || !nxLockTryLock(mWriteLock))
		{
		nxSceneDeadlockReport();
		return 0;
		}

	NxJoint* joint = mScene->createJoint(desc);

	nxLockUnlock(mWriteLock);
	return joint;
	}

// nxSceneDeadlockReport is defined in Scene.cpp, beside the other scene diagnostics.
