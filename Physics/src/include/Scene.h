#ifndef NX_PHYSICS_SCENE
#define NX_PHYSICS_SCENE
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal Scene. Phase 3/7 owns it; it is reconstructed here because every
// actor-dependent row in the programme is blocked on it.
//
// The layout below is MEASURED from the shipped Win32 Release NxPhysics.dll, from
// the two rows that build the object:
//
//   phys_fn_000476 (0x0000ea80, PhysicsSDK::createScene)
//       allocates 0x710 bytes through nxFoundationSDKAllocator, then constructs and
//       initialises it; that 0x710 is where the size comes from.
//   phys_fn_000647 (0x00012c10)  the constructor -- a straight-line initialiser
//       that writes every field in increasing offset order.
//   phys_fn_000651 (0x00013070)  the descriptor-driven initialiser.
//
// The offsets are the ones those rows actually write. Fields that appear in one
// and not the other are marked with the row that writes them. See
// evidence/phase6-joints.md 8t onward for the write-by-write derivation.
//
// This is deliberately an offset-addressed object rather than a class with named
// members: the oracle reads and writes it through raw pointer arithmetic, and a
// named-member layout would silently reorder fields and change the behaviour under
// test. The accessors below exist so the code reads, without pretending the field
// names are recovered -- only their offsets are.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxUserRaycastReport.h"

template<class T> class NxUserEntityReport;
class NxShape;
class NxPlane;

class Scene;
class NxSceneDesc;
class NxSceneStats;
class NxSceneLimits;
class NxActor;
class NxActorDescBase;
class NxController;
class NxControllerDesc;
class NxJointDesc;
class NxJoint;
class Joint;
class JointBreakEvent;
class NxRay;
class NxBounds3;
class NxTriangle;
template<class T> class NxArraySDK;
class NxSphere;
class NxDebugRenderable;
class NxFluid;
class NxFluidDesc;
class Effector;
class SpringAndDamperEffector;
class NxSpringAndDamperEffectorDesc;
struct NxPairFlag;

/**
The 0x710-byte scene object.

Only the members the reconstruction needs to reach are named; everything else is
reached through `at<T>(offset)` so that no offset is lost behind a guess. The
fields that are known by more than their offset are documented individually.
*/
class NxSceneInternal
	{
	public:
	// phys_fn_000647 (0x00012c10). Constructs in place. The oracle's prototype
	// takes the block and returns it, because the caller allocates.
	NxSceneInternal();

	// phys_fn_000651 (0x00013070). Applies a descriptor. Returns true on success;
	// on false the caller destroys the object it just built.
	bool initialise(const NxSceneDesc& desc);

	// phys_fn_000626 (0x00011730). The actor factory. createScene reaches it too,
	// because the ground plane is made by calling this.
	NxActor* createActor(const NxActorDescBase& desc);
	void releaseActor(void* body);
	// phys_fn_000305/000307 (0x0000c720/0x0000c730): controller factory and
	// release forwarded by NpScene. Controller storage remains private because
	// the public SDK exposes only an incomplete NxController declaration.
	NxController* createController(const NxControllerDesc& desc);
	void releaseController(NxController& controller);

	// phys_fn_000665 (0x000142c0). The joint factory. Needs at least one of the
	// two actors dynamic, read through each actor's +0x14 body.
	NxJoint* createJoint(const NxJointDesc& desc);
	// phys_fn_000645 (0x00012b80). Lazily creates the disabled-fluid manager,
	// then asks it to create the requested fluid (which reports unavailable).
	NxFluid* createFluid(const NxFluidDesc& desc);
	// phys_fn_000622 (0x00011620). Remove a fluid from the manager and destroy
	// the now-empty disabled manager; the argument is the NpFluid internal ptr.
	void releaseFluid(void* fluidInternal);

	// The joint rows (units/joint-open-items-contract.md "## Scene joint rows").
	// The Scene keeps its joints three ways: a list through Joint +0x10 headed
	// at +0x59c (joints with +0x2c bit 0 set; a second list at +0x5a0 holds
	// joints without it), a {begin, end, capacity} pointer array at
	// +0x58c/+0x590/+0x594, and the count at +0x6c8 that getNbJoints reads;
	// +0x6bc is the enumeration cursor.
	// phys_fn_000661 (0x00013e00). Scene::addJoint.
	void addJoint(Joint* joint);
	// phys_fn_000633 (0x00012660). Scene::removeJoint.
	void removeJoint(Joint* joint);
	// phys_fn_000557 (0x00010840). Pushes a joint on the +0x5a0 list.
	void pushJointWithoutBodies(Joint* joint);
	// phys_fn_000653 (0x00013760). Scene::releaseJoint.
	void releaseJoint(Joint* joint);
	// phys_fn_000598 (0x00010f50). Grows the 0x50-byte record array at +0x5b8.
	void growJointRecords();
	// phys_fn_000571 (0x000108e0). Links a break event into the list at +0x620.
	void addJointBreakEvent(JointBreakEvent* event);
	// phys_fn_000577 (0x000109c0). Dispatches and frees queued break events.
	void processJointBreakEvents();
	// phys_fn_000640 (0x00012a90). Flushes trigger callbacks, break events,
	// then buffered contact callbacks in oracle order.
	void processSimulationCallbacks();
	// phys_fn_000559 (0x00010860), phys_fn_000563 (0x00010880) and
	// phys_fn_000567 (0x000108a0).
	NxU32 getNbJoints() const;
	// phys_fn_000617 (0x00011440) and phys_fn_000621 (0x000115b0).
	NxSceneStats* getSceneStats();
	void getLimits(NxSceneLimits& limits) const;
	void resetJointIterator();
	Joint* getNextJoint();

	// The scene raycasts (SceneRaycast.cpp, scene-raycast block Task 3), the
	// rows the NxScene wrappers call on +0x24. The two closest queries return
	// the internal shape (the hit shape's +0x08); the wrappers return its
	// public shape (+0x9c).
	bool raycastAnyBounds(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist);
	NxU32 raycastAllBounds(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType,
		NxU32 groups, NxReal maxDist, NxU32 hintFlags);
	NxU32 raycastAllShapes(const NxRay& worldRay, NxUserRaycastReport& report, NxShapesType shapesType,
		NxU32 groups, NxReal maxDist, NxU32 hintFlags);
	void* raycastClosestBounds(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit,
		NxU32 groups, NxReal maxDist, NxU32 hintFlags);
	bool raycastAnyShape(const NxRay& worldRay, NxShapesType shapesType, NxU32 groups, NxReal maxDist);
	void* raycastClosestShape(const NxRay& worldRay, NxShapesType shapeType, NxRaycastHit& hit,
		NxU32 groups, NxReal maxDist, NxU32 hintFlags);
	// Broadphase AABB query used by NxScene::checkOverlapAABB.
	bool checkOverlapAABB(const NxBounds3& worldBounds, NxShapesType shapeType);
	// Sphere query used by NxScene::checkOverlapSphere.
	bool checkOverlapSphere(const NxSphere& worldSphere, NxShapesType shapeType);
	// phys_fn_000676 (0x00014930), the Scene-side AABB triangle collection.
	NxU32 overlapAABBTriangles(const NxBounds3& worldBounds, NxArraySDK<NxTriangle>& worldTriangles);
	// phys_fn_000670 (0x000145f0). AABB overlap collection for public NxScene.
	NxU32 overlapAABBShapes(const NxBounds3& worldBounds, NxShapesType shapeType,
		NxU32 maxShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback);
	// phys_fn_000678 (0x00014990). Sphere overlap collection for public NxScene.
	NxU32 overlapSphereShapes(const NxSphere& worldSphere, NxShapesType shapeType,
		NxU32 maxShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback);
	// phys_fn_000671 (0x000146e0). Plane-volume culling for public NxScene.
	NxU32 cullShapes(NxU32 nbPlanes, const NxPlane* worldPlanes, NxShapesType shapeType,
		NxU32 maxShapes, NxShape** shapes, NxUserEntityReport<NxShape*>* callback);

	// Debug visualisation (scene-raycast block Task 4; SceneVisualize.h has
	// the chain). phys_fn_000657 (0x000139c0) fills the renderable at +0x6b8,
	// which phys_fn_000579 (0x00010a10) creates through the Foundation.
	void visualize();
	NxDebugRenderable* getDebugRenderable();

	// The effector rows (units/effector-coredump-contract.md "### Scene and
	// NpScene rows"). The Scene keeps its effectors in a list through
	// Effector +0x18 headed at +0x5a4, with the count at +0x6c4 and the
	// enumeration cursor at +0x6c0.
	// phys_fn_000587 (0x00010c90). Scene::createSpringAndDamperEffector.
	SpringAndDamperEffector* createSpringAndDamperEffector(const NxSpringAndDamperEffectorDesc& desc);
	// phys_fn_000594 (0x00010e80). Scene::releaseEffector.
	void releaseEffector(Effector* effector);
	// phys_fn_000573 (0x00010900). Scene::removeEffector.
	void removeEffector(Effector* effector);
	// phys_fn_000575 (0x00010970). Releases every effector (Scene teardown).
	void releaseEffectors();
	// phys_fn_000561 (0x00010870), phys_fn_000565 (0x00010890) and
	// phys_fn_000569 (0x000108c0).
	NxU32 getNbEffectors() const;
	void resetEffectorIterator();
	Effector* getNextEffector();

	// The core dump's scene readers (units/effector-coredump-contract.md
	// "### Readers and whether the candidate has them"; effector-and-coredump
	// Task 3b).
	// phys_fn_000509 (0x00010200). The gravity at +0x520..+0x528.
	void getGravity(NxVec3& gravity) const;
	// phys_fn_000507 (0x000101d0), the internal gravity setter.
	void setGravity(const NxVec3& gravity);
	// phys_fn_000538/000539 (0x000106f0/0x00010720), timing accessors.
	void setTiming(NxReal maxTimestep, NxU32 maxIter, NxU32 method);
	void getTiming(NxReal& maxTimestep, NxU32& maxIter, NxU32& method) const;
	// phys_fn_000659's fixed/variable timestep scheduler. Called by NpScene's
	// worker after simulate() stores the elapsed time at +0x544.
	void simulateFrame();
	// phys_fn_000610 (0x00011210): integrate bodies reached from the active
	// sleep-group roots at +0x57c/+0x580.
	__declspec(noinline) void row000610();
	// phys_fn_000611 (0x00011260), including continuation block 000613:
	// prepare each active island's support records, solve them, then copy back.
	__declspec(noinline) void row000611();
	// phys_fn_000619: fetch-side body gravity refresh and pose snapshot.
	void finishSimulation();
	// phys_fn_000523 (0x00010400). The pair-flag count at +0x3c.
	NxU32 getNbPairs() const;
	// phys_fn_000590 (0x00010dc0). Rejects a same-shape pair before touching
	// the pair hash, then stores or clears the exact shape-pair flags.
	void setShapePairFlags(void* shape0, void* shape1, NxU32 flags);
	// phys_fn_000525 (0x00010410), with its continuation phys_fn_000527. The
	// pair flags, walked out of the hash at +0x624. Deferred (see Scene.cpp).
	bool getPairFlagArray(NxPairFlag* userArray, NxU32 numPairs) const;

	// The raw object. `at` is the only sanctioned way to reach a field whose name
	// is not recovered.
	unsigned char* bytes() { return mBytes; }
	const unsigned char* bytes() const { return mBytes; }

	template <class T> T& at(NxU32 offset)
		{ return *reinterpret_cast<T*>(mBytes + offset); }
	template <class T> const T& at(NxU32 offset) const
		{ return *reinterpret_cast<const T*>(mBytes + offset); }

	// The measured fields, by the constant the oracle writes them with.
	static const NxU32 SIZE = 0x710;

	// Four fields the constructor writes last, from the register that holds the
	// address of the SdkContainer at +0x50 (0x00012c47 `lea ebx, [esi + 0x50]`,
	// stored at 0x00012f80..0x00012f92): dwords 0x2a, 0x3d, 0x91 and 0xc1.
	static const NxU32 SELF_0 = 0x2a * 4;
	static const NxU32 SELF_1 = 0x3d * 4;
	static const NxU32 SELF_2 = 0x91 * 4;
	static const NxU32 SELF_3 = 0xc1 * 4;

	// The vtable the constructor installs, at .rdata 0x001066f4. The oracle's
	// phys_fn_000647 writes it as its first action (`*param_1 = &PTR_FUN_101066f4`).
	// Only the first slot is modelled -- the scalar deleting destructor reached
	// by failed creation and by public SDK scene release.
	// The public NxScene wrapper, kept at +0x6cc. getScene reads it back and
	// createActor copies it into each actor. Measured in createScene's decompilation.
	void setPublicScene(void* wrapper) { at<void*>(0x6cc) = wrapper; }
	void* publicScene() const { return at<void*>(0x6cc); }

	static void* vtable();
	// The scalar deleting destructor at the vtable's slot 0.
	void scalarDeletingDestructor(int flags);

	private:
	unsigned char mBytes[SIZE];
	};

// The oracle allocates 0x710 bytes for this object (phys_fn_000476's literal) and
// the highest field the reconstruction writes is +0x70c. A different size means the
// allocation and the object disagree, and a write past the end lands in whatever the
// allocator put next -- which is what the round-4 crash in an unrelated CRT helper
// looks like from a distance.
static_assert(sizeof(NxSceneInternal) == NxSceneInternal::SIZE,
              "the Scene object is 0x710 bytes in the oracle");

#endif
