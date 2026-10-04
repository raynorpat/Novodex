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
#include "NxSphereShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxUserContactReport.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned nxPairObjectLabel(const void* object, const NxActor* actor0, const NxActor* actor1)
	{
	if(object == actor0) return 0;
	if(object == actor1) return 1;
	return 255;
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
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
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

	sdk->releaseScene(*scene);
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
