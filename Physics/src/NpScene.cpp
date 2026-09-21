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
// The three locks the constructor builds are Phase 3's rows -- phys_fn_0005b6a0,
// phys_fn_0005b7b0, phys_fn_0005b9a0, phys_fn_0005b9d0, phys_fn_0005b870 and
// phys_fn_0005ba70 -- and are REPRODUCTION HOLES here: the constructor reproduces
// the allocation and the field layout, and does not model the lock protocol.
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

#include "Scene.h"
#include "NxActor.h"
#include "NxActorDesc.h"

#include <stdio.h>

// ---------------------------------------------------------------------------
// Reproduction holes: the lock protocol Phase 3 owns.
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

	// The oracle builds the inner lock at +8, then the two 4-byte locks at +0xc and
	// +0x10 through phys_fn_0005b6a0, then the two at +0x14 and +0x18, then the
	// 0x18-byte object at +0x1c linked to them.
	mWriteLock = nxGetSdkAllocator()->malloc(4, NX_MEMORY_PERSISTENT);
	if(mWriteLock)
		mWriteLock = nxLockConstruct(mWriteLock);

	mReadLock = nxGetSdkAllocator()->malloc(4, NX_MEMORY_PERSISTENT);
	if(mReadLock)
		mReadLock = nxLockConstruct(mReadLock);

	mCondition = nxGetSdkAllocator()->malloc(0x18, NX_MEMORY_PERSISTENT);
	if(mCondition)
		mCondition = nxConditionConstruct(mCondition, mLockB, mLockA, 0);
	}

NpScene::~NpScene()
	{
	if(mCondition)
		nxGetSdkAllocator()->free(mCondition);
	if(mReadLock)
		nxGetSdkAllocator()->free(mReadLock);
	if(mWriteLock)
		nxGetSdkAllocator()->free(mWriteLock);
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

// phys_fn_000295's shape: the same lock, then the Scene-side release.
void NpScene::releaseActor(NxActor& actor)
	{
	if(!mWriteLock || !nxLockTryLock(mWriteLock))
		{
		nxSceneDeadlockReport();
		return;
		}

	(void)actor;		// the Scene-side release is a reproduction hole
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
	// phys_fn_0005b6a0 allocates a 32-byte CRITICAL_SECTION block and holds only
	// the pointer, so the lock object itself is one word.
	unsigned* p = static_cast<unsigned*>(memory);
	*p = 0;
	return memory;
	}

static bool nxLockTryLock(void* lock)
	{
	// phys_fn_0005b730 is the tryLock that fails only when another thread holds it.
	// Nothing in this reconstruction is threaded, so it succeeds.
	(void)lock;
	return true;
	}

static bool nxLockUnlock(void* lock)
	{
	// phys_fn_0005b790 returns a literal true except the failing tryLock path.
	(void)lock;
	return true;
	}

static void* nxConditionConstruct(void* memory, void* a, void* b, void* c)
	{
	(void)memory; (void)a; (void)b; (void)c;
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

// (unimplemented) createJoint
NxJoint * NpScene::createJoint(const NxJointDesc &)
	{
	return 0;
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

// (unimplemented) getNbActors
NxU32 NpScene::getNbActors() const
	{
	return 0;
	}

// (unimplemented) getActors
NxActor** NpScene::getActors()
	{
	return 0;
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

// nxSceneDeadlockReport is defined in Scene.cpp, beside the other scene diagnostics.
