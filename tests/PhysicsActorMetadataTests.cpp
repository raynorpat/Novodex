#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBoxShapeDesc.h"

#include <stdio.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

int wmain(int argc, wchar_t** argv)
{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorMetadataTests",
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

	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.group = 7;
	actorDesc.flags = NX_AF_DISABLE_RESPONSE;
	actorDesc.userData = reinterpret_cast<void*>(0x12345678u);
	NxActor* actor = scene->createActor(actorDesc);
	printf("actor metadata_created=%u\n", actor ? 1u : 0u);
	if(!actor) return nxFail("actor creation failed");
	printf("actor metadata_user_data=%u\n", actor->userData == actorDesc.userData ? 1u : 0u);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	printf("actor metadata_descriptor=%u.%u.%x.%x\n",
		static_cast<unsigned>(actor->getGroup()),
		actor->readActorFlag(NX_AF_DISABLE_RESPONSE) ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(body + 0x14),
		*reinterpret_cast<const unsigned short*>(body + 0x1c));
	printf("actor metadata_collision_default=%u\n",
		actor->readActorFlag(NX_AF_DISABLE_COLLISION) ? 1u : 0u);
	const unsigned beforeAlloc = allocator.allocations();
	const unsigned beforeFree = allocator.frees();
	actor->setGroup(9);
	actor->raiseActorFlag(NX_AF_DISABLE_COLLISION);
	printf("actor metadata_raised=%u.%u.%u.%x\n",
		static_cast<unsigned>(actor->getGroup()),
		actor->readActorFlag(NX_AF_DISABLE_COLLISION) ? 1u : 0u,
		actor->readActorFlag(NX_AF_DISABLE_RESPONSE) ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(body + 0x14));
	actor->clearActorFlag(NX_AF_DISABLE_RESPONSE);
	printf("actor metadata_cleared=%u.%u.%x\n",
		actor->readActorFlag(NX_AF_DISABLE_COLLISION) ? 1u : 0u,
		actor->readActorFlag(NX_AF_DISABLE_RESPONSE) ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(body + 0x14));
	printf("actor metadata_mutation_allocs=%u.%u\n",
		allocator.allocations() - beforeAlloc, allocator.frees() - beforeFree);
	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
