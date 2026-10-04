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

static unsigned shapeRootType(const NxActor* actor)
	{
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	const unsigned char* shape = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	return shape ? *reinterpret_cast<const unsigned*>(shape + 0xd0) : 0xffffffffu;
	}

static const unsigned char* shapeRoot(const NxActor* actor)
	{
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	return *reinterpret_cast<unsigned char* const*>(body + 0x10);
	}

static const unsigned char* staticPruner(const NxActor* actor)
	{
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	const unsigned char* scene = *reinterpret_cast<unsigned char* const*>(body + 4);
	return *reinterpret_cast<unsigned char* const*>(scene + 0x640);
	}

static void printGroupState(const char* label, const NxActor* actor)
	{
	const unsigned char* group = shapeRoot(actor);
	void* const* childFirst = *reinterpret_cast<void* const* const*>(group + 0xe0);
	void* const* childEnd = *reinterpret_cast<void* const* const*>(group + 0xe4);
	void* const* childCap = *reinterpret_cast<void* const* const*>(group + 0xe8);
	void* const* publicFirst = *reinterpret_cast<void* const* const*>(group + 0xf0);
	void* const* publicEnd = *reinterpret_cast<void* const* const*>(group + 0xf4);
	void* const* publicCap = *reinterpret_cast<void* const* const*>(group + 0xf8);
	const unsigned char* pruner = staticPruner(actor);
	printf("shape_mutation %s=%u.%u.%u.%u.%u.%u\n", label,
		static_cast<unsigned>(childEnd - childFirst),
		static_cast<unsigned>(childCap - childFirst),
		static_cast<unsigned>(publicEnd - publicFirst),
		static_cast<unsigned>(publicCap - publicFirst),
		pruner ? *reinterpret_cast<const unsigned short*>(pruner + 0x10) : 0,
		pruner ? *reinterpret_cast<const unsigned short*>(pruner + 0x12) : 0);
	}

static void printAllocations(const char* label, const NxPageGuardedAllocator& allocator,
	unsigned beforeAlloc, unsigned beforeFree)
	{
	const unsigned added = allocator.allocations() - beforeAlloc;
	const unsigned removed = allocator.frees() - beforeFree;
	printf("shape_mutation %s=%u.%u", label, added, removed);
	for(unsigned i = 0; i < added; ++i)
		printf(".%u", allocator.allocSizeFromEnd(added - 1 - i));
	printf(".f");
	for(unsigned i = 0; i < removed; ++i)
		printf(".%u", allocator.freedSizeFromEnd(removed - 1 - i));
	printf("\n");
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorShapeMutationTests",
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
	first.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&first);
	NxActor* actor = scene->createActor(actorDesc);
	printf("shape_mutation actor=%u\n", actor ? 1u : 0u);
	if(!actor) return nxFail("actor creation failed");
	NxShape* original = actor->getShapes()[0];
	printf("shape_mutation initial=%u.%u.%u\n", actor->getNbShapes(),
		shapeRootType(actor), original ? 1u : 0u);
	NxBoxShapeDesc second;
	second.dimensions = NxVec3(0.5f, 1.0f, 1.5f);
	const unsigned beforeAddAlloc = allocator.allocations();
	const unsigned beforeAddFree = allocator.frees();
	NxShape* added = actor->createShape(second);
	printAllocations("add_memory", allocator, beforeAddAlloc, beforeAddFree);
	const NxU32 countAfterAdd = actor->getNbShapes();
	NxShape** handlesAfterAdd = actor->getShapes();
	printf("shape_mutation added=%u.%u.%u.%u.%u\n", added ? 1u : 0u,
		countAfterAdd, shapeRootType(actor),
		countAfterAdd > 0 && handlesAfterAdd[0] == original ? 1u : 0u,
		countAfterAdd > 1 && handlesAfterAdd[1] == added ? 1u : 0u);
	if(shapeRootType(actor) == 5)
		{
		const unsigned char* group = shapeRoot(actor);
		void* const* childFirst = *reinterpret_cast<void* const* const*>(group + 0xe0);
		void* const* childEnd = *reinterpret_cast<void* const* const*>(group + 0xe4);
		void* const* childCap = *reinterpret_cast<void* const* const*>(group + 0xe8);
		void* const* publicFirst = *reinterpret_cast<void* const* const*>(group + 0xf0);
		void* const* publicEnd = *reinterpret_cast<void* const* const*>(group + 0xf4);
		void* const* publicCap = *reinterpret_cast<void* const* const*>(group + 0xf8);
		printf("shape_mutation group=%u.%u.%u.%u.%u.%u\n",
			static_cast<unsigned>(childEnd - childFirst),
			static_cast<unsigned>(childCap - childFirst),
			static_cast<unsigned>(publicEnd - publicFirst),
			static_cast<unsigned>(publicCap - publicFirst),
			childFirst[0] == *reinterpret_cast<void* const*>(
				reinterpret_cast<const unsigned char*>(original) + 0x18) ? 1u : 0u,
			publicFirst[0] == original ? 1u : 0u);
		}
	if(added)
		{
		const unsigned beforeReleaseAlloc = allocator.allocations();
		const unsigned beforeReleaseFree = allocator.frees();
		actor->releaseShape(*added);
		printAllocations("release_memory", allocator,
			beforeReleaseAlloc, beforeReleaseFree);
		const NxU32 countAfterRelease = actor->getNbShapes();
		NxShape** handlesAfterRelease = actor->getShapes();
		printf("shape_mutation released=%u.%u.%u\n", countAfterRelease,
			shapeRootType(actor), countAfterRelease > 0 &&
			handlesAfterRelease[0] == original ? 1u : 0u);
		if(shapeRootType(actor) == 5)
			{
			const unsigned char* group = shapeRoot(actor);
			void* const* childFirst = *reinterpret_cast<void* const* const*>(group + 0xe0);
			void* const* childEnd = *reinterpret_cast<void* const* const*>(group + 0xe4);
			void* const* publicFirst = *reinterpret_cast<void* const* const*>(group + 0xf0);
			void* const* publicEnd = *reinterpret_cast<void* const* const*>(group + 0xf4);
			printf("shape_mutation released_group=%u.%u.%u.%u\n",
				static_cast<unsigned>(childEnd - childFirst),
				static_cast<unsigned>(publicEnd - publicFirst),
				childFirst[0] == *reinterpret_cast<void* const*>(
					reinterpret_cast<const unsigned char*>(original) + 0x18) ? 1u : 0u,
				publicFirst[0] == original ? 1u : 0u);
			}
		}
	const unsigned beforeActorReleaseAlloc = allocator.allocations();
	const unsigned beforeActorReleaseFree = allocator.frees();
	scene->releaseActor(*actor);
	printAllocations("actor_release_memory", allocator,
		beforeActorReleaseAlloc, beforeActorReleaseFree);
	NxBoxShapeDesc groupFirst;
	NxBoxShapeDesc groupSecond;
	NxBoxShapeDesc groupThird;
	groupFirst.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	groupSecond.dimensions = NxVec3(2.0f, 2.0f, 2.0f);
	groupThird.dimensions = NxVec3(3.0f, 3.0f, 3.0f);
	NxActorDesc groupDesc;
	groupDesc.shapes.pushBack(&groupFirst);
	groupDesc.shapes.pushBack(&groupSecond);
	NxActor* groupActor = scene->createActor(groupDesc);
	if(!groupActor) return nxFail("group actor creation failed");
	NxShape* firstHandle = groupActor->getShapes()[0];
	NxShape* secondHandle = groupActor->getShapes()[1];
	printGroupState("group_before_append", groupActor);
	const unsigned beforeGroupAddAlloc = allocator.allocations();
	const unsigned beforeGroupAddFree = allocator.frees();
	NxShape* thirdHandle = groupActor->createShape(groupThird);
	printAllocations("group_add_memory", allocator,
		beforeGroupAddAlloc, beforeGroupAddFree);
	NxShape** groupHandles = groupActor->getShapes();
	printf("shape_mutation group_added=%u.%u.%u.%u.%u.%u\n",
		thirdHandle ? 1u : 0u, groupActor->getNbShapes(),
		shapeRootType(groupActor),
		groupHandles[0] == firstHandle ? 1u : 0u,
		groupHandles[1] == secondHandle ? 1u : 0u,
		thirdHandle && groupHandles[2] == thirdHandle ? 1u : 0u);
	printGroupState("group_after_append", groupActor);
	if(thirdHandle)
		{
		const unsigned beforeGroupRemoveAlloc = allocator.allocations();
		const unsigned beforeGroupRemoveFree = allocator.frees();
		groupActor->releaseShape(*secondHandle);
		printAllocations("group_remove_memory", allocator,
			beforeGroupRemoveAlloc, beforeGroupRemoveFree);
		groupHandles = groupActor->getShapes();
		printf("shape_mutation group_removed=%u.%u.%u.%u\n",
			groupActor->getNbShapes(), shapeRootType(groupActor),
			groupHandles[0] == firstHandle ? 1u : 0u,
			groupHandles[1] == thirdHandle ? 1u : 0u);
		printGroupState("group_after_remove", groupActor);
		}
	scene->releaseActor(*groupActor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}
