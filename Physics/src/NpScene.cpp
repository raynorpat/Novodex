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

// (unimplemented) getGravity
void NpScene::getGravity(NxVec3&)
	{
	
	}

// (unimplemented) releaseJoint
void NpScene::releaseJoint(NxJoint &)
	{
	
	}

// (unimplemented) createSpringAndDamperEffector
NxSpringAndDamperEffector* NpScene::createSpringAndDamperEffector(const NxSpringAndDamperEffectorDesc&)
	{
	return 0;
	}

// (unimplemented) releaseEffector
void NpScene::releaseEffector(NxEffector&)
	{
	
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

// (unimplemented) getNbJoints
NxU32 NpScene::getNbJoints() const
	{
	return 0;
	}

// (unimplemented) resetJointIterator
void NpScene::resetJointIterator()
	{
	
	}

// (unimplemented) getNextJoint
NxJoint * NpScene::getNextJoint()
	{
	return 0;
	}

// (unimplemented) getNbEffectors
NxU32 NpScene::getNbEffectors() const
	{
	return 0;
	}

// (unimplemented) resetEffectorIterator
void NpScene::resetEffectorIterator()
	{
	
	}

// (unimplemented) getNextEffector
NxEffector * NpScene::getNextEffector()
	{
	return 0;
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

// (unimplemented) visualize
void NpScene::visualize()
	{
	
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

// (unimplemented) raycastAnyBounds
bool NpScene::raycastAnyBounds(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist) const
	{
	return 0;
	}

// (unimplemented) raycastAnyShape
bool NpScene::raycastAnyShape(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist) const
	{
	return 0;
	}

// (unimplemented) raycastAllBounds
NxU32 NpScene::raycastAllBounds(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	return 0;
	}

// (unimplemented) raycastAllShapes
NxU32 NpScene::raycastAllShapes(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	return 0;
	}

// (unimplemented) raycastClosestBounds
NxShape* NpScene::raycastClosestBounds(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	return 0;
	}

// (unimplemented) raycastClosestShape
NxShape* NpScene::raycastClosestShape(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const
	{
	return 0;
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

// (unimplemented) setGravity
void NpScene::setGravity(const NxVec3&)
	{
	
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
