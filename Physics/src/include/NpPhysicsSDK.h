#ifndef NX_PHYSICS_NPPHYSICSSDK
#define NX_PHYSICS_NPPHYSICSSDK
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"
#include "NxPhysicsSDK.h"

class PhysicsSDK;

/**
The NxPhysicsSDK the user is handed. It owns nothing but a back pointer to the
SDK-side singleton and a lock, and forwards every call to it; the wrapper's real
job is to take each live scene's lock around the forwarded call so a call made
while a scene is being simulated cannot race it.

The vtable at .rdata 0x00105860 has exactly the 24 slots NxPhysicsSDK declares
with NX_USE_FLUID_API on, which is one of the three independent confirmations
that the shipped DLL was built with the fluid API enabled.

Layout is measured from phys_fn_000226 (the constructor, 0x0000b5d0): 0x0000ea05
allocates 0xc bytes, 0x0000b5d0 writes the vtable pointer at +0, the lock is
constructed at +8 and the SDK pointer is stored at +4.

The lifecycle, visualization, collision-group, material read/add and fluid
group-pair slots are reconstructed. The rest forward to SDK-side methods this
component does not own; each one names the oracle rows it stands in for, and
none of them returns a value the oracle would not -- the two placeholders that
did, getGroupCollisionFlag and addMaterial, are now the real thing.

The remaining placeholders are blocked rather than unwritten. getScene,
releaseScene, createScene, createTriangleMesh and releaseTriangleMesh all need
the Scene or TriangleMesh layout that Phases 3 and 4 own; setActorGroupPairFlags
and getActorGroupPairFlags need phys_fn_004155 and phys_fn_004153, which the
census places in Phase 6; coreDump needs seventeen rows across Phases 3, 4 and 6.
*/
class NpPhysicsSDK : public NxPhysicsSDK, public NxAllocateable
	{
	public:
	NpPhysicsSDK(PhysicsSDK* sdk);
	~NpPhysicsSDK();

	// Reconstructed: phys_fn_000228, 000230, 000232, 000238 and 000261, plus the
	// slots below that have to stay in declaration order: phys_fn_000248,
	// 000250, 000252, 000254, 000258, 000260, 000263, 000273, 000275 and 000277.
	// gates/phase2-closure.json is the authority on which of these are proved
	// and how; this list only says which have bodies.
	void release();
	bool setParameter(NxParameter paramEnum, NxReal paramValue);
	NxReal getParameter(NxParameter paramEnum) const;
	NxU32 getNbScenes() const;
	NxU32 getNbMaterials();

	// Placeholders. Each definition names what blocks it, and every one of them
	// is deferred in gates/phase2-closure.json rather than counted as closed.
	NxScene* createScene(const NxSceneDesc& desc);
	void releaseScene(NxScene& scene);
	NxScene* getScene(NxU32 index);
	NxTriangleMesh* createTriangleMesh(const NxTriangleMeshDesc& desc);
	void releaseTriangleMesh(NxTriangleMesh& mesh);
	void setGroupCollisionFlag(NxCollisionGroup group1, NxCollisionGroup group2, bool enable);
	bool getGroupCollisionFlag(NxCollisionGroup group1, NxCollisionGroup group2) const;
	void setActorGroupPairFlags(NxActorGroup group1, NxActorGroup group2, NxU32 flags);
	NxU32 getActorGroupPairFlags(NxActorGroup group1, NxActorGroup group2) const;
#if NX_USE_FLUID_API
	void setFluidGroupPairFlags(NxActorGroup actorGroup, NxFluidGroup fluidGroup, NxU32 flags);
	NxU32 getFluidGroupPairFlags(NxActorGroup actorGroup, NxFluidGroup fluidGroup) const;
#endif
	void visualize(const NxUserDebugRenderer& renderer);	// reconstructed
	NxMaterialIndex addMaterial(const NxMaterial& material);
	void setMaterialAtIndex(NxMaterialIndex index, const NxMaterial* material);
	NxMaterial* getMaterial(NxMaterialIndex index);
	void purgeMaterials();
	bool coreDump(const char* fname, bool binary, const char* addendum);
	bool setPerformanceInspector(NxPerformanceInspector* npi);

	// SDK-internal state, left accessible so the measured offsets can be asserted.
	PhysicsSDK* mSdk;
	ReadWriteLock mLock;
	};

#endif
