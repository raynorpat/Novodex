/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "NpPhysicsSDK.h"
#include "PhysicsSDK.h"
#include "NpScene.h"
#include "TriangleMesh.h"

#include <stddef.h>
#include <stdio.h>
#include <new>
#include "Scene.h"
#include "NpSceneGuard.h"
#include "FoundationSDK.h"

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
static const int gActorGroupPairFlagsWriteLockErrorLine = 236;
static const int gCreateTriangleMeshWriteLockErrorLine = 111;
static const int gReleaseTriangleMeshWriteLockErrorLine = 127;
static void* nxSdkSceneWriteLink(PhysicsSDK* sdk, NxU32 index);

// SDK-wide mutators take every live scene's write lock in scene-array order,
// unwind acquired locks in reverse on contention, and release them in forward
// order after dispatch. Triangle-mesh creation and release use this same guard.

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

// phys_fn_000234 (0x0000b770): forwards to the SDK-side row and returns the
// public wrapper built inside the internal Scene constructor.
NxScene* NpPhysicsSDK::createScene(const NxSceneDesc& desc)
	{
	NxSceneInternal* scene = mSdk->createScene(desc);
	return scene ? static_cast<NxScene*>(scene->publicScene()) : 0;
	}

void NpPhysicsSDK::releaseScene(NxScene& scene)
	{
	mSdk->releaseScene(static_cast<NpScene&>(scene).scene());
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


// phys_fn_000242 (0x0000b7e0): lock every scene before creating the internal
// mesh, then return its public wrapper at TriangleMesh+0xe4.
NxTriangleMesh* NpPhysicsSDK::createTriangleMesh(const NxTriangleMeshDesc& desc)
	{
	NxU32 locked = 0;
	for(; locked < mSdk->getNbScenes(); ++locked)
		if(!nxNpSceneGuardWriteTry(nxSdkSceneWriteLink(mSdk, locked)))
			{
			while(locked)
				nxNpSceneGuardLeave(nxSdkSceneWriteLink(mSdk, --locked));
			NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
				NX_NP_PHYSICS_SDK_CPP, gCreateTriangleMeshWriteLockErrorLine, 0,
				"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
			return 0;
			}
	NxTriangleMesh* mesh = mSdk->createTriangleMesh(desc);
	for(NxU32 index = 0; index < locked; ++index)
		nxNpSceneGuardLeave(nxSdkSceneWriteLink(mSdk, index));
	return mesh;
	}

// phys_fn_000244 (0x0000b8c0): same lock/unwind protocol around release.
void NpPhysicsSDK::releaseTriangleMesh(NxTriangleMesh& mesh)
	{
	NxU32 locked = 0;
	for(; locked < mSdk->getNbScenes(); ++locked)
		if(!nxNpSceneGuardWriteTry(nxSdkSceneWriteLink(mSdk, locked)))
			{
			while(locked)
				nxNpSceneGuardLeave(nxSdkSceneWriteLink(mSdk, --locked));
			NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
				NX_NP_PHYSICS_SDK_CPP, gReleaseTriangleMeshWriteLockErrorLine, 0,
				"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
			return;
			}
	NxTriangleMeshAdapter& adapter = static_cast<NxTriangleMeshAdapter&>(mesh);
	mSdk->releaseTriangleMesh(adapter.mMesh);
	for(NxU32 index = 0; index < locked; ++index)
		nxNpSceneGuardLeave(nxSdkSceneWriteLink(mSdk, index));
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

// phys_fn_000269: lock scenes and dispatch actor-group pair-flag updates.
void NpPhysicsSDK::setActorGroupPairFlags(NxActorGroup group1, NxActorGroup group2, NxU32 flags)
	{
	NxU32 locked = 0;
	for(; locked < mSdk->getNbScenes(); ++locked)
		{
		NxSceneInternal* scene = reinterpret_cast<NxSceneInternal*>(mSdk->getScene(locked));
		NpScene* wrapper = static_cast<NpScene*>(scene->publicScene());
		if(!nxNpSceneGuardWriteTry(wrapper->writeLink()))
			{
			while(locked)
				{
				--locked;
				NxSceneInternal* previous = reinterpret_cast<NxSceneInternal*>(mSdk->getScene(locked));
				NpScene* previousWrapper = static_cast<NpScene*>(previous->publicScene());
				nxNpSceneGuardLeave(previousWrapper->writeLink());
				}
			NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
				NX_NP_PHYSICS_SDK_CPP, gActorGroupPairFlagsWriteLockErrorLine, 0,
				"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
			return;
			}
		}
	mSdk->setActorGroupPairFlags(group1, group2, flags);
	for(NxU32 index = 0; index < locked; ++index)
		{
		NxSceneInternal* scene = reinterpret_cast<NxSceneInternal*>(mSdk->getScene(index));
		NpScene* wrapper = static_cast<NpScene*>(scene->publicScene());
		nxNpSceneGuardLeave(wrapper->writeLink());
		}
	}

// phys_fn_000271: lock scenes and dispatch actor-group pair-flag queries.
NxU32 NpPhysicsSDK::getActorGroupPairFlags(NxActorGroup group1, NxActorGroup group2) const
	{
	NxU32 locked = 0;
	for(; locked < mSdk->getNbScenes(); ++locked)
		{
		NxSceneInternal* scene = reinterpret_cast<NxSceneInternal*>(mSdk->getScene(locked));
		unsigned char* wrapper = reinterpret_cast<unsigned char*>(scene->publicScene());
		void* readLink = *reinterpret_cast<void**>(wrapper + 0x10);
		nxNpSceneGuardEnter(readLink);
		}
	const NxU32 flags = mSdk->getActorGroupPairFlags(group1, group2);
	for(NxU32 index = 0; index < locked; ++index)
		{
		NxSceneInternal* scene = reinterpret_cast<NxSceneInternal*>(mSdk->getScene(index));
		unsigned char* wrapper = reinterpret_cast<unsigned char*>(scene->publicScene());
		void* readLink = *reinterpret_cast<void**>(wrapper + 0x10);
		nxNpSceneGuardLeave(readLink);
		}
	return flags;
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

// phys_fn_000258, a writer slot; its deadlock report is NpPhysicsSDK.cpp:182,
// from the immediate at 0x0000bd2f.
void NpPhysicsSDK::setMaterialAtIndex(NxMaterialIndex index, const NxMaterial* material)
	{
	mSdk->setMaterialAtIndex(index, material);
	}

// phys_fn_000260, a const slot, so the reader lock at +0x10 and no failure path.
NxMaterial* NpPhysicsSDK::getMaterial(NxMaterialIndex index)
	{
	return mSdk->getMaterial(index);
	}

// phys_fn_000263 with phys_fn_000265 as its outlined unwind, which reports
// NpPhysicsSDK.cpp:211 from the immediate at 0x0000beff.
void NpPhysicsSDK::purgeMaterials()
	{
	mSdk->purgeMaterials();
	}

// The scene write link 000267 tries: 000450(i) -> [+0x6cc] (the NxScene
// wrapper) -> [+0xc].
static void* nxSdkSceneWriteLink(PhysicsSDK* sdk, NxU32 index)
	{
	NxSceneInternal* scene = reinterpret_cast<NxSceneInternal*>(sdk->getScene(index));
	return static_cast<NpScene*>(scene->publicScene())->writeLink();
	}

// phys_fn_000267 (0x0000bf20, 221 B)
// A writer slot. It tries every scene's write link in order (002364); on the
// first failure it releases the links already taken in reverse (002366),
// reports code 2 at NpPhysicsSDK.cpp:225 (the immediate 0xe1 at 0x0000bfe1)
// with the deadlock message and returns false. Otherwise it calls the core
// dump (phys_fn_004062, `this` = mSdk), keeps its result in bl, releases every
// link in order and returns the result -- 004062 always returns false.
bool NpPhysicsSDK::coreDump(const char* fname, bool binary, const char* addendum)
	{
	NxU32 i = 0;
	for(; i < mSdk->getNbScenes(); i++)
		{
		if(!nxNpSceneGuardWriteTry(nxSdkSceneWriteLink(mSdk, i)))
			{
			while(i)
				{
				i--;
				nxNpSceneGuardLeave(nxSdkSceneWriteLink(mSdk, i));
				}
			NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_NP_PHYSICS_SDK_CPP,
				0xe1, 0, "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
			return false;
			}
		}
	const bool result = mSdk->coreDump(fname, binary, addendum);
	for(i = 0; i < mSdk->getNbScenes(); i++)
		nxNpSceneGuardLeave(nxSdkSceneWriteLink(mSdk, i));
	return result;
	}

// phys_fn_000277 with phys_fn_000279 as its outlined unwind, which reports
// NpPhysicsSDK.cpp:277 from the immediate at 0x0000c28f and returns false at
// 0x0000c2a5. A writer slot, so tryLock at 0x0000c1f4.
bool NpPhysicsSDK::setPerformanceInspector(NxPerformanceInspector* inspector)
	{
	return mSdk->setPerformanceInspector(inspector);
	}
