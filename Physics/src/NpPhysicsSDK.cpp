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

// The two fluid group-pair report sites, from the immediates at 0x0000c182 and
// 0x0000c1b2. Like every other line number in this reconstruction they belong to
// the build tree the DLL was compiled from, so __LINE__ cannot produce them.
static const int gSetFluidGroupPairFlagsWarningLine = 257;
static const int gGetFluidGroupPairFlagsWarningLine = 265;

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
	// phys_fn_000240 -> phys_fn_000450. Twenty-two bytes with no lock walk: it
	// forwards, then reads the NxScene wrapper out of Scene at +0x6cc and
	// returns it, unguarded -- 0x0000b7cd dereferences whatever getScene handed
	// back, so an out of range index faults. Reconstructing that needs the Scene
	// layout, which Phase 3 owns, so this slot is blocked, not open.
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

// phys_fn_000248 and phys_fn_000250. The mutating slot walks the scenes taking
// each writer lock at +0xc with tryLock (0x0000b9c8 calls phys_fn_002364) and
// unwinds with the deadlock report below if one is held; the const slot takes
// each reader lock at +0x10 outright (0x0000bab5 calls phys_fn_002362) and has
// no failure path at all. Both walks are Phase 3's and run zero iterations here.

void NpPhysicsSDK::setGroupCollisionFlag(NxCollisionGroup group1, NxCollisionGroup group2, bool enable)
	{
	mSdk->setGroupCollisionFlag(group1, group2, enable);
	}

bool NpPhysicsSDK::getGroupCollisionFlag(NxCollisionGroup group1, NxCollisionGroup group2) const
	{
	return mSdk->getGroupCollisionFlag(group1, group2);
	}

void NpPhysicsSDK::setActorGroupPairFlags(NxActorGroup, NxActorGroup, NxU32)
	{
	// phys_fn_000269 -> phys_fn_000433 -> phys_fn_004155, which the census
	// places in Phase 6. Blocked on the pair-keyed hash at .data 0x00123c28.
	}

NxU32 NpPhysicsSDK::getActorGroupPairFlags(NxActorGroup, NxActorGroup) const
	{
	// phys_fn_000271 -> phys_fn_000435 -> phys_fn_004153, Phase 6. Blocked on
	// the same hash.
	return 0;
	}

#if NX_USE_FLUID_API
// phys_fn_000273 and phys_fn_000275. The shipped DLL exposes the fluid API and
// refuses this part of it: both report and return, and neither touches the SDK.
// The error code pushed at 0x0000c18c and 0x0000c1bc is 0xce, NXE_DB_WARNING,
// not an error code, and the lines are the immediates at 0x0000c182 and
// 0x0000c1b2.
void NpPhysicsSDK::setFluidGroupPairFlags(NxActorGroup, NxFluidGroup, NxU32)
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING, NX_NP_PHYSICS_SDK_CPP,
		gSetFluidGroupPairFlagsWarningLine, 0, "NxFluid::setFluidGroupPairFlags(): Feature not available!");
	}

NxU32 NpPhysicsSDK::getFluidGroupPairFlags(NxActorGroup, NxFluidGroup) const
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING, NX_NP_PHYSICS_SDK_CPP,
		gGetFluidGroupPairFlagsWarningLine, 0, "NxFluid::getFluidGroupPairFlags(): Feature not available!");
	return 0;
	}
#endif

// phys_fn_000254, with phys_fn_000256 as the outlined unwind MSVC produced for
// its deadlock path. Slot 17 is a writer slot: 0x0000bbb4 tries each scene's
// lock at +0xc and reports NpPhysicsSDK.cpp:173 if one is held.
NxMaterialIndex NpPhysicsSDK::addMaterial(const NxMaterial& material)
	{
	return mSdk->addMaterial(material);
	}

void NpPhysicsSDK::setMaterialAtIndex(NxMaterialIndex, const NxMaterial*)
	{
	// phys_fn_000258 -> phys_fn_000484.
	}

// phys_fn_000260, a const slot, so the reader lock at +0x10 and no failure path.
NxMaterial* NpPhysicsSDK::getMaterial(NxMaterialIndex index)
	{
	return mSdk->getMaterial(index);
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
