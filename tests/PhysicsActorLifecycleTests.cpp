// Public-DLL actor probe: distinguish static and dynamic actors through the
// shipped and rebuilt SDKs. The pair loader verifies module identity.
#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include <string.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned nxBits(float value)
{
	unsigned bits = 0;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static void nxPrintPosition(const char* label, const NxVec3& position)
{
	printf("actor %s position=%08x.%08x.%08x\n", label,
		nxBits(position.x), nxBits(position.y), nxBits(position.z));
}

static void nxPrintOrientation(const char* label, const NxMat33& orientation)
{
	float rowMajor[9];
	orientation.getRowMajor(rowMajor);
	printf("actor %s orientation=", label);
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%08x", i ? "." : "", nxBits(rowMajor[i]));
	printf("\n");
}

static void nxPrintPublicQuaternion(const char* label, const NxQuat& quaternion)
{
	printf("actor %s public_quaternion=%08x.%08x.%08x.%08x\n", label,
		nxBits(quaternion.x), nxBits(quaternion.y),
		nxBits(quaternion.z), nxBits(quaternion.w));
}

static void nxPrintPose(const char* label, const NxMat34& pose)
{
	float rowMajor[9];
	pose.M.getRowMajor(rowMajor);
	printf("actor %s pose=", label);
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%08x", i ? "." : "", nxBits(rowMajor[i]));
	printf(".%08x.%08x.%08x\n", nxBits(pose.t.x),
		nxBits(pose.t.y), nxBits(pose.t.z));
}

static void nxPrintDynamicQuaternion(const char* label, const NxActor* actor)
{
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(actor);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(bytes + 0x14);
	const unsigned char* record = body
		? *reinterpret_cast<unsigned char* const*>(body + 8) : 0;
	if(!record) return;
	unsigned words[4];
	memcpy(words, record + 0x5c, sizeof(words));
	printf("actor %s quaternion=%08x.%08x.%08x.%08x\n", label,
		words[0], words[1], words[2], words[3]);
}

static unsigned nxFindPointerOffset(const void* block, unsigned size, const void* target);

static void nxPrintBodyLink(const char* label, const NxActor* actor)
{
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(actor);
	const unsigned actorSize = *reinterpret_cast<const unsigned*>(
		reinterpret_cast<uintptr_t>(bytes) & ~static_cast<uintptr_t>(0xfff));
	printf("actor %s actor_alloc=%x\n", label, actorSize);
	const unsigned char* linked = *reinterpret_cast<unsigned char* const*>(bytes + 0x10);
	printf("actor %s linked=%u\n", label, linked ? 1u : 0u);
	if(linked)
		{
		const unsigned linkedSize = *reinterpret_cast<const unsigned*>(
			reinterpret_cast<uintptr_t>(linked) & ~static_cast<uintptr_t>(0xfff));
		printf("actor %s linked_alloc=%x\n", label, linkedSize);
		const void* child = *reinterpret_cast<void* const*>(linked);
		printf("actor %s linked_child=%u\n", label, child ? 1u : 0u);
		if(child)
			{
			MEMORY_BASIC_INFORMATION memory;
			if(VirtualQuery(child, &memory, sizeof(memory)) == sizeof(memory) &&
				memory.State == MEM_COMMIT && !(memory.Protect & PAGE_NOACCESS))
				{
				const unsigned childSize = *reinterpret_cast<const unsigned*>(
					reinterpret_cast<uintptr_t>(child) & ~static_cast<uintptr_t>(0xfff));
				printf("actor %s linked_child_alloc=%x\n", label, childSize);
				}
			}
		}
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(bytes + 0x14);
	const unsigned char* nested = body
		? *reinterpret_cast<unsigned char* const*>(body + 0x08) : 0;
	printf("actor %s body=%u nested=%u\n", label,
		body ? 1u : 0u, nested ? 1u : 0u);
	printf("actor %s body_is_actor=%u\n", label, body == bytes ? 1u : 0u);
	if(body)
		{
		const unsigned allocSize = *reinterpret_cast<const unsigned*>(
			reinterpret_cast<uintptr_t>(body) & ~static_cast<uintptr_t>(0xfff));
		printf("actor %s body_alloc=%x\n", label, allocSize);
		const void* bodyActor = *reinterpret_cast<void* const*>(body);
		const void* bodyShape = *reinterpret_cast<void* const*>(body + 0x10);
		printf("actor %s body_actor=%u body_shape=%u\n", label,
			bodyActor == actor ? 1u : 0u, bodyShape ? 1u : 0u);
		if(bodyShape)
			{
			const unsigned shapeSize = *reinterpret_cast<const unsigned*>(
				reinterpret_cast<uintptr_t>(bodyShape) & ~static_cast<uintptr_t>(0xfff));
			printf("actor %s body_shape_alloc=%x\n", label, shapeSize);
			printf("actor %s shape_actor_offset=%x\n", label,
				nxFindPointerOffset(bodyShape, shapeSize, actor));
			unsigned helperOffset = 0xffff;
			for(unsigned offset = 0; offset + 4 <= shapeSize; offset += 4)
				{
				const void* candidate = *reinterpret_cast<void* const*>(
					static_cast<const unsigned char*>(bodyShape) + offset);
				MEMORY_BASIC_INFORMATION memory;
				if(!candidate || VirtualQuery(candidate, &memory, sizeof(memory)) != sizeof(memory)
					|| memory.State != MEM_COMMIT || (memory.Protect & PAGE_NOACCESS))
					continue;
				const unsigned size = *reinterpret_cast<const unsigned*>(
					reinterpret_cast<uintptr_t>(candidate) & ~static_cast<uintptr_t>(0xfff));
				if(size == 0x1c && helperOffset == 0xffff) helperOffset = offset;
				}
			printf("actor %s shape_helper_offset=%x\n", label, helperOffset);
			}
		}
	if(nested)
		{
		const unsigned allocSize = *reinterpret_cast<const unsigned*>(
			reinterpret_cast<uintptr_t>(nested) & ~static_cast<uintptr_t>(0xfff));
		printf("actor %s nested_alloc=%x\n", label, allocSize);
		if(allocSize >= 0x1a0)
			{
			const unsigned char* pose = *reinterpret_cast<unsigned char* const*>(nested + 0x19c);
			if(pose)
				{
				printf("actor %s pose_is_body=%u\n", label,
					pose == body ? 1u : 0u);
				const unsigned poseSize = *reinterpret_cast<const unsigned*>(
					reinterpret_cast<uintptr_t>(pose) & ~static_cast<uintptr_t>(0xfff));
				const void* cached = *reinterpret_cast<void* const*>(pose + 8);
				printf("actor %s pose_alloc=%x cached=%u cached_nested=%u\n", label,
					poseSize, cached ? 1u : 0u, cached == nested ? 1u : 0u);
				}
			}
		}
}

static unsigned nxFindPointerOffset(const void* block, unsigned size, const void* target)
{
	const unsigned char* bytes = static_cast<const unsigned char*>(block);
	for(unsigned i = 0; i + 4 <= size; i += 4)
		if(*reinterpret_cast<void* const*>(bytes + i) == target) return i;
	return 0xffff;
}

static void nxPrintObjectArray(const char* label, const NxScene* scene)
{
	const unsigned char* wrapper = reinterpret_cast<const unsigned char*>(scene);
	const unsigned char* internal = *reinterpret_cast<unsigned char* const*>(wrapper + 0x24);
	const void* const* first = *reinterpret_cast<void* const* const*>(internal + 0x56c);
	const void* const* last = *reinterpret_cast<void* const* const*>(internal + 0x570);
	const void* const* end = *reinterpret_cast<void* const* const*>(internal + 0x574);
	printf("scene objects_%s=%u/%u\n", label,
		first ? static_cast<unsigned>(last - first) : 0u,
		first ? static_cast<unsigned>(end - first) : 0u);
}

int wmain(int argc, wchar_t** argv)
{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorLifecycleTests", pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	static NxPageGuardedAllocator allocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");

	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&box);
	staticDesc.globalPose.t = NxVec3(2.0f, -1.0f, 4.0f);
	NxActor* staticActor = scene->createActor(staticDesc);
	printf("actor static created=%u\n", staticActor ? 1u : 0u);
	if(!staticActor) return nxFail("static actor creation failed");
	printf("actor static dynamic=%u\n", staticActor->isDynamic() ? 1u : 0u);
	nxPrintBodyLink("static", staticActor);
	nxPrintPosition("static", staticActor->getGlobalPositionVal());
	nxPrintOrientation("static", staticActor->getGlobalOrientationVal());
	nxPrintPublicQuaternion("static", staticActor->getGlobalOrientationQuatVal());
	nxPrintPose("static", staticActor->getGlobalPoseVal());

	NxBodyDesc body;
	NxActorDesc dynamicDesc;
	dynamicDesc.body = &body;
	dynamicDesc.density = 1.0f;
	dynamicDesc.shapes.pushBack(&box);
	dynamicDesc.globalPose.t = NxVec3(-3.0f, 2.0f, 1.0f);
	NxActor* dynamicActor = scene->createActor(dynamicDesc);
	printf("actor dynamic created=%u\n", dynamicActor ? 1u : 0u);
	if(!dynamicActor) return nxFail("dynamic actor creation failed");
	printf("actor dynamic dynamic=%u\n", dynamicActor->isDynamic() ? 1u : 0u);
	nxPrintBodyLink("dynamic", dynamicActor);
	nxPrintPosition("dynamic", dynamicActor->getGlobalPositionVal());
	nxPrintPose("dynamic", dynamicActor->getGlobalPoseVal());

	NxActorDesc rotatedDesc = dynamicDesc;
	rotatedDesc.globalPose.M.setRow(0, NxVec3(-1.0f, 0.0f, 0.0f));
	rotatedDesc.globalPose.M.setRow(1, NxVec3(0.0f, -1.0f, 0.0f));
	rotatedDesc.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	rotatedDesc.globalPose.t = NxVec3(5.0f, -2.0f, 3.0f);
	NxActor* rotatedActor = scene->createActor(rotatedDesc);
	printf("actor rotated created=%u\n", rotatedActor ? 1u : 0u);
	if(!rotatedActor) return nxFail("rotated actor creation failed");
	nxPrintDynamicQuaternion("rotated", rotatedActor);
	nxPrintOrientation("rotated", rotatedActor->getGlobalOrientationVal());
	nxPrintPublicQuaternion("rotated", rotatedActor->getGlobalOrientationQuatVal());
	nxPrintPose("rotated", rotatedActor->getGlobalPoseVal());

	rotatedDesc.globalPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
	rotatedDesc.globalPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
	nxPrintObjectArray("before_quarter", scene);
	const unsigned allocationsBeforeQuarter = allocator.allocations();
	NxActor* quarterActor = scene->createActor(rotatedDesc);
	printf("actor quarter created=%u\n", quarterActor ? 1u : 0u);
	printf("actor quarter creation_allocs=%u\n",
		allocator.allocations() - allocationsBeforeQuarter);
	nxPrintObjectArray("after_quarter", scene);
	printf("actor quarter creation_sizes=%x.%x.%x.%x.%x.%x\n",
		allocator.allocSizeFromEnd(5), allocator.allocSizeFromEnd(4),
		allocator.allocSizeFromEnd(3), allocator.allocSizeFromEnd(2),
		allocator.allocSizeFromEnd(1), allocator.allocSizeFromEnd(0));
	if(!quarterActor) return nxFail("quarter-turn actor creation failed");
	const unsigned char* quarterBody = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(quarterActor) + 0x14);
	const unsigned char* quarterRecord = quarterBody
		? *reinterpret_cast<unsigned char* const*>(quarterBody + 8) : 0;
	printf("actor quarter creation_roles=");
	for(int i = 5; i >= 0; --i)
		{
			void* block = allocator.allocPointerFromEnd(i);
			printf("%s%x", i == 5 ? "" : ".",
				(block == quarterActor ? 1 : 0) |
				(block == quarterBody ? 2 : 0) |
				(block == quarterRecord ? 4 : 0));
		}
	printf("\n");
	const void* retained = allocator.allocPointerFromEnd(0);
	const void* quarterShape = quarterBody
		? *reinterpret_cast<void* const*>(quarterBody + 0x10) : 0;
	const void* internalScene = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(scene) + 0x24);
	printf("actor quarter retained_refs=%x.%x.%x.%x\n",
		nxFindPointerOffset(quarterBody, 0x50, retained),
		nxFindPointerOffset(quarterRecord, 0x260, retained),
		quarterShape ? nxFindPointerOffset(quarterShape,
			*reinterpret_cast<const unsigned*>(reinterpret_cast<uintptr_t>(quarterShape) & ~static_cast<uintptr_t>(0xfff)),
			retained) : 0xffff,
		nxFindPointerOffset(internalScene, 0x710, retained));
	nxPrintDynamicQuaternion("quarter", quarterActor);
	nxPrintOrientation("quarter", quarterActor->getGlobalOrientationVal());
	nxPrintPublicQuaternion("quarter", quarterActor->getGlobalOrientationQuatVal());
	nxPrintPose("quarter", quarterActor->getGlobalPoseVal());
	const void* staticLink = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(staticActor) + 0x10);
	const void* dynamicLink = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(dynamicActor) + 0x10);
	const void* rotatedLink = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(rotatedActor) + 0x10);
	const void* quarterLink = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(quarterActor) + 0x10);
	printf("actor links_alias=%u.%u.%u\n", staticLink == dynamicLink ? 1u : 0u,
		staticLink == rotatedLink ? 1u : 0u, staticLink == quarterLink ? 1u : 0u);
	unsigned sceneLinkMask = 0;
	for(unsigned i = 0; i < 9; ++i)
		if(*reinterpret_cast<void* const*>(reinterpret_cast<const unsigned char*>(scene) + i*4)
			== staticLink) sceneLinkMask |= 1u << i;
	printf("actor link_scene_mask=%03x\n", sceneLinkMask);
	const void* slotWord = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(staticActor) + 0x0c);
	unsigned slotSceneMask = 0;
	for(unsigned i = 0; i < 9; ++i)
		if(*reinterpret_cast<void* const*>(reinterpret_cast<const unsigned char*>(scene) + i*4)
			== slotWord) slotSceneMask |= 1u << i;
	printf("actor slot_scene_mask=%03x slot_small=%u\n", slotSceneMask,
		reinterpret_cast<uintptr_t>(slotWord) < 1024 ? 1u : 0u);

	printf("scene actors_before=%u\n", scene->getNbActors());
	NxActor** actorsBefore = scene->getActors();
	printf("scene actors_before_list=%u\n", actorsBefore ? 1u : 0u);
	const unsigned freesBefore = allocator.frees();
	void* creationBlocks[6];
	for(unsigned i = 0; i < 6; ++i) creationBlocks[i] = allocator.allocPointerFromEnd(i);
	scene->releaseActor(*quarterActor);
	printf("scene release_frees=%u\n", allocator.frees() - freesBefore);
	printf("scene release_sizes=%x.%x.%x.%x.%x\n",
		allocator.freedSizeFromEnd(4), allocator.freedSizeFromEnd(3),
		allocator.freedSizeFromEnd(2), allocator.freedSizeFromEnd(1),
		allocator.freedSizeFromEnd(0));
	unsigned freedCreationMask = 0;
	for(unsigned i = 0; i < allocator.frees() - freesBefore; ++i)
		for(unsigned j = 0; j < 6; ++j)
			if(allocator.freedPointerFromEnd(i) == creationBlocks[j])
				freedCreationMask |= 1u << j;
	printf("scene release_creation_mask=%02x\n", freedCreationMask);
	printf("scene actors_after=%u\n", scene->getNbActors());
	NxActor** actorsAfter = scene->getActors();
	unsigned quarterStillListed = 0;
	for(NxU32 i = 0; actorsAfter && i < scene->getNbActors(); ++i)
		if(actorsAfter[i] == quarterActor) quarterStillListed = 1;
	printf("scene quarter_still_listed=%u\n", quarterStillListed);

	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
