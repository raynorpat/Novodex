#ifndef NX_PHYSICS_NPSCENE
#define NX_PHYSICS_NPSCENE
/*----------------------------------------------------------------------------*\
|
|								Novodex Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The NxScene the user is handed. It owns a back pointer to the internal Scene and
// three locks, and forwards; its locks are what the SDK singletons' scene walks
// take around a forwarded call.
//
// Layout MEASURED from phys_fn_000285 (0x0000c310), the constructor:
//
//   +0x00  vtable            (the oracle installs PTR_FUN_10105a98 last)
//   +0x04  --
//   +0x08  an inner lock object, whose own vtable is PTR_LAB_10105ba4
//   +0x0c  a 4-byte ReadWriteLock, allocated and constructed by phys_fn_0005b6a0
//   +0x10  a 4-byte ReadWriteLock, allocated and constructed by phys_fn_0005b6a0
//   +0x14  a stack-shaped lock, initialised by phys_fn_0005b7b0
//   +0x18  a stack-shaped lock, initialised by phys_fn_0005b7b0
//   +0x1c  a 0x18-byte object from phys_fn_0005b9a0, linked to the two above
//   +0x20  a byte, zeroed
//   +0x24  the internal Scene (the oracle's `param_1`)
//
// The class is 0x28 bytes, which is the size phys_fn_000476 allocates for it.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxScene.h"

class NxSceneInternal;
class NxActorDescBase;
class NxActor;

class NpScene : public NxScene, public NxAllocateable
	{
	public:
	// phys_fn_000285 (0x0000c310).
	NpScene(NxSceneInternal* scene);
	~NpScene();

	// The Scene's internal pointer, which the forwarding slots below use and which
	// getScene reads back out of the Scene at +0x6cc.
	NxSceneInternal* scene() { return mScene; }
	const NxSceneInternal* scene() const { return mScene; }

	// phys_fn_000293 (0x0000c490) reads the write lock, forwards, and releases it.
	// The lock walk itself is Phase 3's; the forward is in NpScene.cpp.
	NxActor* createActor(const NxActorDescBase& desc);
	void releaseActor(NxActor& actor);
	// phys_fn_000295's shape, forwarded to the Scene.
	virtual NxJoint* createJoint(const NxJointDesc& desc);
	virtual void setGravity(const NxVec3&);

	// The remaining virtuals are not reached by a reconstructed path and are
	// declared only so the vtable keeps the oracle's slot order.
	void release();

	// The remaining NxScene virtuals, UNIMPLEMENTED. NpScene must be concrete to
	// be instantiated and NxScene declares 65 pure virtuals; only createActor and
	// releaseActor above are reconstructed. Each body is empty and returns a default.
	// None is claimed as reconstructed and none is gated.
	virtual void getGravity(NxVec3&);
	virtual void releaseJoint(NxJoint &);
	virtual NxSpringAndDamperEffector* createSpringAndDamperEffector(const NxSpringAndDamperEffectorDesc&);
	virtual void releaseEffector(NxEffector&);
	virtual NxController* createController(const NxControllerDesc&);
	virtual void releaseController(NxController&);
	virtual void setActorPairFlags(NxActor&, NxActor&, NxU32 nxContactPairFlag);
	virtual NxU32 getActorPairFlags(NxActor&, NxActor&) const;
	virtual void setShapePairFlags(NxShape&, NxShape&, NxU32 nxContactPairFlag);
	virtual NxU32 getShapePairFlags(NxShape&, NxShape&) const;
	virtual NxU32 getNbPairs() const;
	virtual bool getPairFlagArray(NxPairFlag* userArray, NxU32 numPairs) const;
	virtual NxU32 getNbActors() const;
	virtual NxActor** getActors();
	virtual NxU32 getNbJoints() const;
	virtual void resetJointIterator();
	virtual NxJoint * getNextJoint();
	virtual NxU32 getNbEffectors() const;
	virtual void resetEffectorIterator();
	virtual NxEffector * getNextEffector();
	virtual void flushStream();
	virtual void startRun(NxReal elapsedTime);
	virtual void finishRun();
	virtual void setTiming(NxReal maxTimestep, NxU32 maxIter, NxTimeStepMethod method);
	virtual void getTiming(NxReal & maxTimestep, NxU32 & maxIter, NxTimeStepMethod & method) const;
	virtual void runFor(NxReal elapsedTime, NxReal maxTimestep, NxU32 maxIter, NxTimeStepMethod method);
	virtual void visualize();
	virtual NxSceneStats* getSceneStats();
	virtual void getLimits(NxSceneLimits& limits) const;
	virtual void setUserNotify(NxUserNotify* callback);
	virtual NxUserNotify* getUserNotify() const;
	virtual void setUserTriggerReport(NxUserTriggerReport* callback);
	virtual NxUserTriggerReport* getUserTriggerReport() const;
	virtual void setUserContactReport(NxUserContactReport* callback);
	virtual NxUserContactReport* getUserContactReport() const;
	virtual void setUserFluidContactReport(NxUserFluidContactReport* callback);
	virtual NxUserFluidContactReport* getUserFluidContactReport() const;
	virtual bool raycastAnyBounds(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist) const;
	virtual bool raycastAnyShape(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist) const;
	virtual NxU32 raycastAllBounds(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const;
	virtual NxU32 raycastAllShapes(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const;
	virtual NxShape* raycastClosestBounds(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const;
	virtual NxShape* raycastClosestShape(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit, NxU32 groups, NxReal maxDist, NxU32 hintFlags) const;
	virtual NxU32 overlapSphereShapes(const NxSphere& worldSphere, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback);
	virtual NxU32 overlapAABBShapes(const NxBounds3& worldBounds, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback);
	virtual NxU32 cullShapes(NxU32 nbPlanes, const NxPlane* worldPlanes, NxShapesType shapeType, NxU32 nbShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback);
	virtual bool checkOverlapSphere(const NxSphere& worldSphere, NxShapesType shapeType);
	virtual bool checkOverlapAABB(const NxBounds3& worldBounds, NxShapesType shapeType);
	virtual NxU32 overlapAABBTriangles(const NxBounds3& worldBounds, NxArraySDK<NxTriangle>& worldTriangles);
	virtual NxFluid* createFluid(const NxFluidDesc&);
	virtual void releaseFluid(NxFluid&);
	virtual NxU32 getNbFluids() const;
	virtual NxFluid** getFluids();
	virtual NxImplicitMesh* createImplicitMesh(const NxImplicitMeshDesc&);
	virtual void releaseImplicitMesh(NxImplicitMesh&);
	virtual NxU32 getNbImplicitMeshes() const;
	virtual NxImplicitMesh** getImplicitMeshes();
	virtual bool wait(NxStandardFences, bool block);
	virtual bool isWritable();
	virtual void simulate(NxReal elapsedTime);
	virtual bool checkResults(NxSimulationStatus, bool block );
	virtual bool fetchResults(NxSimulationStatus, bool block );

	private:
	NpScene(const NpScene&);
	NpScene& operator=(const NpScene&);

	unsigned char mLockObject[0x04];		// +0x08
	void* mWriteLock;						// +0x0c
	void* mReadLock;						// +0x10
	unsigned char mLockA[0x04];				// +0x14
	unsigned char mLockB[0x04];				// +0x18
	void* mCondition;						// +0x1c
	unsigned char mFlag;					// +0x20
	NxSceneInternal* mScene;				// +0x24
	};

// The oracle allocates 0x28 bytes for this class (phys_fn_000476's literal). A
// different size means the allocation and the object disagree, and every field
// past the difference is written out of bounds -- which is what a heap overrun
// looks like from a crash in an unrelated helper.
static_assert(sizeof(NpScene) == 0x28, "NpScene is 40 bytes in the oracle");

#endif