/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "NpPhysicsSDK.h"
#include "PhysicsSDK.h"

#include <stddef.h>

// 0x0000ea05 allocates 0xc bytes for this object, phys_fn_000226 stores the SDK
// pointer at +4 and constructs the lock at +8.
static_assert(sizeof(NpPhysicsSDK) == 0xc, "NpPhysicsSDK is 12 bytes in the oracle");
static_assert(offsetof(NpPhysicsSDK, mSdk) == 4, "the SDK pointer follows the vtable pointer");
static_assert(offsetof(NpPhysicsSDK, mLock) == 8, "the lock follows the SDK pointer");

// Around every forwarded call the oracle walks mSdk's scenes and takes each
// scene's writer lock (or, for the const queries, its reader lock), unwinding
// and reporting NXE_INVALID_OPERATION with
//   "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a
//    deadlock!"
// if one cannot be taken. Reaching those locks means going through NpScene,
// which Phase 3 owns, so the loops are not reconstructed here. They run zero
// iterations for every state this component can reach, because the only entry
// point that adds a scene is createScene.

NpPhysicsSDK::NpPhysicsSDK(PhysicsSDK* sdk)
	{
	mSdk = sdk;
	}

NpPhysicsSDK::~NpPhysicsSDK()
	{
	}

void NpPhysicsSDK::release()
	{
	mSdk->release();
	}

bool NpPhysicsSDK::setParameter(NxParameter paramEnum, NxReal paramValue)
	{
	return mSdk->setParameter(paramEnum, paramValue);
	}

NxReal NpPhysicsSDK::getParameter(NxParameter paramEnum) const
	{
	return mSdk->getParameter(paramEnum);
	}

NxU32 NpPhysicsSDK::getNbScenes() const
	{
	return mSdk->getNbScenes();
	}

NxU32 NpPhysicsSDK::getNbMaterials()
	{
	return mSdk->getNbMaterials();
	}

// phys_fn_000252. Unlike every other wrapper in this vtable it takes each
// scene's writer lock outright (0x0000bb34 calls phys_fn_002362, not
// phys_fn_002364) and so references neither the deadlock string nor an error
// site. The two scene walks around the forwarded call are Phase 3's and run
// zero iterations for every state this component can reach.
void NpPhysicsSDK::visualize(const NxUserDebugRenderer& renderer)
	{
	mSdk->visualize(renderer);
	}

// Everything below stands in for an oracle row this component does not own. The
// stable IDs are the wrapper row and the SDK-side row it forwards to.

NxScene* NpPhysicsSDK::createScene(const NxSceneDesc&)
	{
	// phys_fn_000234 -> phys_fn_000476; needs Scene, Phase 3.
	return 0;
	}

void NpPhysicsSDK::releaseScene(NxScene&)
	{
	// phys_fn_000236 -> phys_fn_000468; needs Scene, Phase 3.
	}

NxScene* NpPhysicsSDK::getScene(NxU32)
	{
	// phys_fn_000240 -> phys_fn_000450; needs Scene, Phase 3.
	return 0;
	}

NxTriangleMesh* NpPhysicsSDK::createTriangleMesh(const NxTriangleMeshDesc&)
	{
	// phys_fn_000242 -> phys_fn_000478; needs TriangleMesh, Phase 4.
	return 0;
	}

void NpPhysicsSDK::releaseTriangleMesh(NxTriangleMesh&)
	{
	// phys_fn_000244 -> phys_fn_000470; needs TriangleMesh, Phase 4.
	}

void NpPhysicsSDK::setGroupCollisionFlag(NxCollisionGroup, NxCollisionGroup, bool)
	{
	// phys_fn_000248 -> phys_fn_000452.
	}

bool NpPhysicsSDK::getGroupCollisionFlag(NxCollisionGroup, NxCollisionGroup) const
	{
	// phys_fn_000250 -> phys_fn_000431.
	return false;
	}

void NpPhysicsSDK::setActorGroupPairFlags(NxActorGroup, NxActorGroup, NxU32)
	{
	// phys_fn_000269 -> phys_fn_000433.
	}

NxU32 NpPhysicsSDK::getActorGroupPairFlags(NxActorGroup, NxActorGroup) const
	{
	// phys_fn_000271 -> phys_fn_000435.
	return 0;
	}

#if NX_USE_FLUID_API
void NpPhysicsSDK::setFluidGroupPairFlags(NxActorGroup, NxFluidGroup, NxU32)
	{
	// phys_fn_000273.
	}

NxU32 NpPhysicsSDK::getFluidGroupPairFlags(NxActorGroup, NxFluidGroup) const
	{
	// phys_fn_000275.
	return 0;
	}
#endif

NxMaterialIndex NpPhysicsSDK::addMaterial(const NxMaterial&)
	{
	// phys_fn_000254 -> phys_fn_000482.
	return 0;
	}

void NpPhysicsSDK::setMaterialAtIndex(NxMaterialIndex, const NxMaterial*)
	{
	// phys_fn_000258 -> phys_fn_000484.
	}

NxMaterial* NpPhysicsSDK::getMaterial(NxMaterialIndex)
	{
	// phys_fn_000260 -> phys_fn_000456.
	return 0;
	}

void NpPhysicsSDK::purgeMaterials()
	{
	// phys_fn_000263 -> phys_fn_000486.
	}

bool NpPhysicsSDK::coreDump(const char*, bool, const char*)
	{
	// phys_fn_000267 -> phys_fn_004062.
	return false;
	}

bool NpPhysicsSDK::setPerformanceInspector(NxPerformanceInspector*)
	{
	// phys_fn_000277 -> phys_fn_000446.
	return false;
	}
