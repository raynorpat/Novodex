#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBoxShapeDesc.h"
#include "NxShape.h"

#include <stdio.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorReleaseShapeProbeTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	static NxPageGuardedAllocator allocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	NxBoxShapeDesc first;
	NxBoxShapeDesc second;
	first.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	second.dimensions = NxVec3(2.0f, 3.0f, 4.0f);
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&first);
	actorDesc.shapes.pushBack(&second);
	NxActor* actor = scene->createActor(actorDesc);
	if(!actor) return nxFail("two-shape actor creation failed");
	NxShape* const firstHandle = actor->getShapes()[0];
	NxShape* const secondHandle = actor->getShapes()[1];
	actor->releaseShape(*secondHandle);
	NxShape** const remaining = actor->getShapes();
	const unsigned count = actor->getNbShapes();
	printf("release_shape count=%u remaining_first=%u remaining_released=%u\n",
		count, count == 1 && remaining[0] == firstHandle,
		count == 1 && remaining[0] == secondHandle);
	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}
