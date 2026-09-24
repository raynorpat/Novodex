#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBoxShapeDesc.h"
#include "NxBoxShape.h"

#include <stdio.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

int wmain(int argc, wchar_t** argv)
{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorNameTests",
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

	static const char descriptorName[] = "actor-descriptor";
	static const char firstName[] = "actor-first";
	static const char secondName[] = "actor-second";
	static const char shapeName[] = "shape-name";
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.name = descriptorName;
	NxActor* actor = scene->createActor(actorDesc);
	printf("actor name_created=%u\n", actor ? 1u : 0u);
	if(!actor) return nxFail("actor creation failed");
	const char* observed = actor->getName();
	printf("actor name_descriptor=%u.%s\n",
		observed == descriptorName, observed ? observed : "<null>");
	NxShape* shape = actor->getShapes()[0];
	printf("actor name_shape_initial=%u\n", shape->getName() == 0);

	const unsigned beforeAlloc = allocator.allocations();
	const unsigned beforeFree = allocator.frees();
	actor->setName(firstName);
	observed = actor->getName();
	printf("actor name_first=%u.%s\n",
		observed == firstName, observed ? observed : "<null>");
	actor->setName(secondName);
	observed = actor->getName();
	printf("actor name_second=%u.%s\n",
		observed == secondName, observed ? observed : "<null>");
	shape->setName(shapeName);
	printf("actor name_shape_separate=%u.%u\n",
		shape->getName() == shapeName, actor->getName() == secondName);
	actor->setName(0);
	printf("actor name_cleared=%u.%u\n",
		actor->getName() == 0, shape->getName() == shapeName);
	printf("actor name_mutation_allocs=%u.%u\n",
		allocator.allocations() - beforeAlloc,
		allocator.frees() - beforeFree);

	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
