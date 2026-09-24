#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"

#include <stdio.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

int wmain(int argc, wchar_t** argv)
{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorBodyFlagTests",
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
	NxBodyDesc bodyDesc;
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.body = &bodyDesc;
	actorDesc.density = 1.0f;
	NxActor* actor = scene->createActor(actorDesc);
	printf("actor bodyflag_created=%u\n", actor ? 1u : 0u);
	if(!actor) return nxFail("actor creation failed");
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	const unsigned char* record = *reinterpret_cast<unsigned char* const*>(body + 8);
	const unsigned char* sceneInternal = *reinterpret_cast<unsigned char* const*>(body + 4);
	const unsigned char* aux = *reinterpret_cast<unsigned char* const*>(sceneInternal + 0x48);
	const unsigned char* recordAux = *reinterpret_cast<unsigned char* const*>(record + 0x120);
	const unsigned id = *reinterpret_cast<const unsigned*>(record + 0x11c);
	const unsigned* recordFlags = *reinterpret_cast<unsigned* const*>(aux + 0x40);
	printf("actor bodyflag_manager=%u.%u.%x\n", recordAux == aux ? 1u : 0u,
		id, recordFlags[id]);
	const void* readContext = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x10);
	const void* writeContext = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x0c);
	printf("actor bodyflag_locklinks=%u.%u.%u\n", readContext ? 1u : 0u,
		writeContext ? 1u : 0u,
		readContext && *reinterpret_cast<void* const*>(readContext) ? 1u : 0u);
	printf("actor bodyflag_descriptor=%u.%u.%x\n",
		actor->readBodyFlag(NX_BF_VISUALIZATION) ? 1u : 0u,
		actor->readBodyFlag(NX_BF_DISABLE_GRAVITY) ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(record + 0x10c));
	const unsigned beforeAlloc = allocator.allocations();
	const unsigned beforeFree = allocator.frees();
	actor->raiseBodyFlag(NX_BF_DISABLE_GRAVITY);
	printf("actor bodyflag_manager_raised=%x\n", recordFlags[id]);
	printf("actor bodyflag_raised=%u.%u.%x\n",
		actor->readBodyFlag(NX_BF_DISABLE_GRAVITY) ? 1u : 0u,
		actor->readBodyFlag(NX_BF_VISUALIZATION) ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(record + 0x10c));
	actor->clearBodyFlag(NX_BF_VISUALIZATION);
	printf("actor bodyflag_manager_cleared=%x\n", recordFlags[id]);
	printf("actor bodyflag_cleared=%u.%u.%x\n",
		actor->readBodyFlag(NX_BF_DISABLE_GRAVITY) ? 1u : 0u,
		actor->readBodyFlag(NX_BF_VISUALIZATION) ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(record + 0x10c));
	printf("actor bodyflag_mutation_allocs=%u.%u\n",
		allocator.allocations() - beforeAlloc, allocator.frees() - beforeFree);
	unsigned* mutableFlags = const_cast<unsigned*>(recordFlags);
	unsigned** activeCursor = reinterpret_cast<unsigned**>(
		const_cast<unsigned char*>(aux) + 0x54);
	unsigned* activeBegin = *reinterpret_cast<unsigned* const*>(aux + 0x50);
	unsigned* priorCursor = *activeCursor;
	unsigned* activeIndex = *reinterpret_cast<unsigned* const*>(aux + 0x60);
	const unsigned priorIndex = activeIndex[id];
	mutableFlags[id] = 0;
	actor->raiseBodyFlag(NX_BF_DISABLE_GRAVITY);
	printf("actor bodyflag_requeued=%x.%u.%u.%u\n", mutableFlags[id],
		static_cast<unsigned>(*activeCursor - activeBegin),
		*(*activeCursor - 1), activeIndex[id]);
	mutableFlags[id] = 0xffffffffu;
	*activeCursor = priorCursor;
	activeIndex[id] = priorIndex;
	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
