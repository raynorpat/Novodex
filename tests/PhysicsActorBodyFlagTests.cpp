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
	// NpActor.cpp completion Task 2 (contract H1): the dirty mark's list
	// growth. The manager's dirty list (+0x50/+0x54/+0x58) is swapped for a
	// null one and eight fresh actors' flag words are cleared, so each
	// setLinearDamping appends one id: the null list grows to 2 entries, then
	// to 2n+2 = 6 and 14, each time through the SDK allocator (the new block's
	// size is printed) and freeing only a non-null old block. Then the
	// kinematic transition (000785) on a clean id: its 0x10000 mark grows the
	// null list before the 0x20-byte target block is allocated, and the
	// disable arm frees that block after its marks. The harness frees the
	// blocks the SDK grew and puts the saved list, flags and indices back.
	NxActor* grow[8];
	unsigned growIds[8];
	unsigned growFlags[8];
	unsigned growIndex[8];
	for(unsigned i = 0; i < 8; ++i)
		{
		grow[i] = scene->createActor(actorDesc);
		if(!grow[i]) return nxFail("growth actor creation failed");
		const unsigned char* growRecord = *reinterpret_cast<unsigned char* const*>(
			*reinterpret_cast<unsigned char* const*>(
				reinterpret_cast<const unsigned char*>(grow[i]) + 0x14) + 8);
		growIds[i] = *reinterpret_cast<const unsigned*>(growRecord + 0x11c);
		growFlags[i] = mutableFlags[growIds[i]];
		growIndex[i] = activeIndex[growIds[i]];
		}
	unsigned** listBegin = reinterpret_cast<unsigned**>(const_cast<unsigned char*>(aux) + 0x50);
	unsigned** listCapacity = reinterpret_cast<unsigned**>(const_cast<unsigned char*>(aux) + 0x58);
	unsigned* savedBegin = *listBegin;
	unsigned* savedEnd = *activeCursor;
	unsigned* savedCapacity = *listCapacity;
	printf("actor grow_start=%u.%u.%u\n",
		static_cast<unsigned>(savedEnd - savedBegin),
		static_cast<unsigned>(savedCapacity - savedBegin),
		growIds[7] - growIds[0]);
	for(unsigned i = 0; i < 8; ++i) mutableFlags[growIds[i]] = 0;
	*listBegin = 0;
	*activeCursor = 0;
	*listCapacity = 0;
	for(unsigned i = 0; i < 8; ++i)
		{
		const unsigned allocs = allocator.allocations();
		const unsigned frees = allocator.frees();
		grow[i]->setLinearDamping(0.25f);
		const unsigned grew = allocator.allocations() - allocs;
		const unsigned freed = allocator.frees() - frees;
		printf("actor grow_%u=%u.%u.%x.%u.%u.%u.%x.%x.%u\n", i,
			static_cast<unsigned>(*activeCursor - *listBegin),
			static_cast<unsigned>(*listCapacity - *listBegin),
			mutableFlags[growIds[i]], activeIndex[growIds[i]],
			grew, freed, grew ? allocator.allocSizeFromEnd(0) : 0u,
			freed ? allocator.freedSizeFromEnd(0) : 0u,
			(*activeCursor)[-1] == growIds[i] ? 1u : 0u);
		}
	printf("actor grow_list=");
	for(unsigned i = 0; i < 8; ++i)
		printf("%s%u", i ? "." : "", (*listBegin)[i] - growIds[0]);
	printf("\n");
	allocator.free(*listBegin);
	*listBegin = 0;
	*activeCursor = 0;
	*listCapacity = 0;
	mutableFlags[growIds[0]] = 0;
	const unsigned char* kinematicRecord = *reinterpret_cast<unsigned char* const*>(
		*reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(grow[0]) + 0x14) + 8);
	const unsigned kinematicAllocs = allocator.allocations();
	grow[0]->raiseBodyFlag(NX_BF_KINEMATIC);
	const unsigned kinematicGrew = allocator.allocations() - kinematicAllocs;
	printf("actor grow_kinematic=%u.%x.%x.%u.%u.%x.%x.%x.%x.%x.%x\n", kinematicGrew,
		kinematicGrew > 1 ? allocator.allocSizeFromEnd(1) : 0u,
		kinematicGrew ? allocator.allocSizeFromEnd(0) : 0u,
		static_cast<unsigned>(*activeCursor - *listBegin),
		static_cast<unsigned>(*listCapacity - *listBegin),
		mutableFlags[growIds[0]],
		*reinterpret_cast<const unsigned*>(kinematicRecord + 0x10c),
		*reinterpret_cast<const unsigned*>(kinematicRecord + 0xc0),
		*reinterpret_cast<const unsigned*>(kinematicRecord + 0xc4),
		*reinterpret_cast<const unsigned*>(kinematicRecord + 0xcc),
		*reinterpret_cast<const unsigned* const*>(kinematicRecord + 0x118)
			? *reinterpret_cast<const unsigned*>(
				*reinterpret_cast<const unsigned char* const*>(kinematicRecord + 0x118) + 0xc)
			: 0xdeadu);
	const unsigned dynamicAllocs = allocator.allocations();
	const unsigned dynamicFrees = allocator.frees();
	grow[0]->clearBodyFlag(NX_BF_KINEMATIC);
	const unsigned dynamicFreed = allocator.frees() - dynamicFrees;
	printf("actor grow_dynamic=%u.%u.%x.%u.%x.%x.%x.%x.%x.%u\n",
		allocator.allocations() - dynamicAllocs, dynamicFreed,
		dynamicFreed ? allocator.freedSizeFromEnd(0) : 0u,
		static_cast<unsigned>(*activeCursor - *listBegin),
		mutableFlags[growIds[0]],
		*reinterpret_cast<const unsigned*>(kinematicRecord + 0x10c),
		*reinterpret_cast<const unsigned*>(kinematicRecord + 0xc0),
		*reinterpret_cast<const unsigned*>(kinematicRecord + 0xc4),
		*reinterpret_cast<const unsigned*>(kinematicRecord + 0xcc),
		*reinterpret_cast<const unsigned* const*>(kinematicRecord + 0x118) ? 1u : 0u);
	allocator.free(*listBegin);
	*listBegin = savedBegin;
	*activeCursor = savedEnd;
	*listCapacity = savedCapacity;
	for(unsigned i = 0; i < 8; ++i)
		{
		mutableFlags[growIds[i]] = growFlags[i];
		activeIndex[growIds[i]] = growIndex[i];
		}
	for(unsigned i = 0; i < 8; ++i)
		scene->releaseActor(*grow[i]);
	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
