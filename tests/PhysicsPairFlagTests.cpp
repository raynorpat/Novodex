#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxShape.h"
#include "NxSphereShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxUserContactReport.h"
#include "PhysicsActorErrorStream.h"
#include "../Physics/src/include/NpSceneGuard.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned nxPairObjectLabel(const void* object, const NxActor* actor0, const NxActor* actor1)
	{
	if(object == actor0) return 0;
	if(object == actor1) return 1;
	return 255;
	}

struct NxPairFlagHeldSceneWriteLock
	{
	void* link;
	HANDLE ready;
	HANDLE release;
	};

static DWORD WINAPI nxPairFlagHoldSceneWriteLock(void* context)
	{
	NxPairFlagHeldSceneWriteLock* const held =
		static_cast<NxPairFlagHeldSceneWriteLock*>(context);
	nxNpSceneGuardEnter(held->link);
	SetEvent(held->ready);
	WaitForSingleObject(held->release, INFINITE);
	nxNpSceneGuardLeave(held->link);
	return 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsPairFlagTests", pairDirectory, &physics);
	if(status)
		return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		return nxFail("NxCreatePhysicsSDK missing");
	NxActorErrorStream errorStream("pairflag");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, &errorStream);
	if(!sdk)
		return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		return nxFail("scene creation failed");

	NxPlaneShapeDesc plane;
	NxActorDesc groundDesc;
	groundDesc.shapes.pushBack(&plane);
	NxActor* ground = scene->createActor(groundDesc);
	NxSphereShapeDesc leftSphere;
	leftSphere.radius = 0.5f;
	leftSphere.localPose.t = NxVec3(-0.25f, 0.0f, 0.0f);
	NxSphereShapeDesc rightSphere;
	rightSphere.radius = 0.5f;
	rightSphere.localPose.t = NxVec3(0.25f, 0.0f, 0.0f);
	NxBodyDesc body;
	NxActorDesc compoundDesc;
	compoundDesc.body = &body;
	compoundDesc.density = 1.0f;
	compoundDesc.globalPose.t = NxVec3(0.0f, 2.0f, 0.0f);
	compoundDesc.shapes.pushBack(&leftSphere);
	compoundDesc.shapes.pushBack(&rightSphere);
	NxActor* compound = scene->createActor(compoundDesc);
	if(!ground || !compound || compound->getNbShapes() != 2)
		return nxFail("compound actor creation failed");

	scene->setActorPairFlags(*ground, *compound, NX_IGNORE_PAIR);
	const NxU32 flags = scene->getActorPairFlags(*ground, *compound);
	const NxU32 count = scene->getNbPairs();
	NxPairFlag pairs[16] = {};
	const bool array = scene->getPairFlagArray(pairs, count);
	unsigned first = 255, second = 255;
	if(array && count)
		{
		first = nxPairObjectLabel(pairs[0].objects[0], ground, compound);
		second = nxPairObjectLabel(pairs[0].objects[1], ground, compound);
		}
	const unsigned actorPair = array && count ? (pairs[0].isActorPair() ? 1u : 0u) : 0u;
	const NxU32 pairBits = array && count ? pairs[0].flags : 0;
	printf("pairflag compound flags=%08x count=%u array=%u actor_pair=%u objects=%u.%u pair_flags=%08x\n",
		flags, count, array ? 1u : 0u, actorPair, first, second, pairBits);

	NxShape** compoundShapes = compound->getShapes();
	if(!compoundShapes || !compoundShapes[0])
		return nxFail("compound shape handle unavailable");
	errorStream.enabled = true;
	scene->setShapePairFlags(*compoundShapes[0], *compoundShapes[0], NX_IGNORE_PAIR);
	errorStream.enabled = false;
	const NxU32 sameShapeFlags = scene->getShapePairFlags(*compoundShapes[0], *compoundShapes[0]);
	const NxU32 sameShapeCount = scene->getNbPairs();
	NxPairFlag sameShapePairs[16] = {};
	const bool sameShapeArray = scene->getPairFlagArray(sameShapePairs, sameShapeCount);
	unsigned sameShapeEntries = 0;
	for(NxU32 i = 0; sameShapeArray && i < sameShapeCount && i < 16; ++i)
		if(sameShapePairs[i].objects[0] == compoundShapes[0] &&
			sameShapePairs[i].objects[1] == compoundShapes[0] && !sameShapePairs[i].isActorPair())
			++sameShapeEntries;
	printf("pairflag same_shape flags=%08x count=%u array=%u self_entries=%u errors=%u\n",
		sameShapeFlags, sameShapeCount, sameShapeArray ? 1u : 0u, sameShapeEntries, errorStream.reports);

	NxPairFlagHeldSceneWriteLock held = {
		*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(scene) + 0x0c),
		CreateEventA(0, TRUE, FALSE, 0), CreateEventA(0, TRUE, FALSE, 0) };
	if(!held.link || !held.ready || !held.release)
		return nxFail("scene write-lock fixture setup failed");
	HANDLE lockThread = CreateThread(0, 0, nxPairFlagHoldSceneWriteLock, &held, 0, 0);
	if(!lockThread || WaitForSingleObject(held.ready, 5000) != WAIT_OBJECT_0)
		return nxFail("scene write-lock fixture did not acquire the lock");
	const unsigned actorReportsBeforeContention = errorStream.reports;
	errorStream.enabled = true;
	scene->setActorPairFlags(*ground, *compound, NX_NOTIFY_ON_TOUCH);
	errorStream.enabled = false;
	printf("pairflag actor_contended reports=%u flags=%08x\n",
		errorStream.reports - actorReportsBeforeContention,
		scene->getActorPairFlags(*ground, *compound));
	const unsigned reportsBeforeContention = errorStream.reports;
	errorStream.enabled = true;
	scene->setShapePairFlags(*compoundShapes[0], *compoundShapes[1], NX_NOTIFY_ON_TOUCH);
	errorStream.enabled = false;
	SetEvent(held.release);
	WaitForSingleObject(lockThread, INFINITE);
	CloseHandle(lockThread);
	CloseHandle(held.ready);
	CloseHandle(held.release);
	printf("pairflag contended reports=%u flags=%08x\n",
		errorStream.reports - reportsBeforeContention,
		scene->getShapePairFlags(*compoundShapes[0], *compoundShapes[1]));

	// Releasing one side must remove its per-shape records from the scene hash,
	// including both child-shape keys of this compound actor.
	scene->releaseActor(*compound);
	const NxU32 releasedPairCount = scene->getNbPairs();
	NxPairFlag releasedPairs[16] = {};
	const bool releasedPairArray = scene->getPairFlagArray(releasedPairs, releasedPairCount);
	printf("pairflag compound_released count=%u array=%u\n",
		releasedPairCount, releasedPairArray ? 1u : 0u);

	sdk->releaseScene(*scene);
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
