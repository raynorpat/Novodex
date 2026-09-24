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
#include "NxBoxShape.h"
#include <stdlib.h>
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

static void nxPrintBroadphase(const char* label, const NxScene* scene)
{
	const unsigned char* wrapper = reinterpret_cast<const unsigned char*>(scene);
	const unsigned char* internal = *reinterpret_cast<unsigned char* const*>(wrapper + 0x24);
	const unsigned char* index = *reinterpret_cast<unsigned char* const*>(internal + 0x648);
	if(!index)
		{ printf("actor multi broadphase_%s=0\n", label); return; }
	const unsigned size = *reinterpret_cast<const unsigned*>(
		reinterpret_cast<uintptr_t>(index) & ~static_cast<uintptr_t>(0xfff));
	const unsigned short count = *reinterpret_cast<const unsigned short*>(index + 0x10);
	const unsigned short capacity = *reinterpret_cast<const unsigned short*>(index + 0x12);
	const void* entries = *reinterpret_cast<void* const*>(index + 0x14);
	const void* references = *reinterpret_cast<void* const*>(index + 0x18);
	printf("actor multi broadphase_%s=%x.%u/%u.%u.%u\n", label,
		size, count, capacity, entries ? 1u : 0u, references ? 1u : 0u);
}

static void nxPrintSceneArray6e8(const char* label, const NxScene* scene)
{
	const unsigned char* wrapper = reinterpret_cast<const unsigned char*>(scene);
	const unsigned char* internal = *reinterpret_cast<unsigned char* const*>(wrapper + 0x24);
	const void* const* first = *reinterpret_cast<void* const* const*>(internal + 0x6e8);
	const void* const* last = *reinterpret_cast<void* const* const*>(internal + 0x6ec);
	const void* const* end = *reinterpret_cast<void* const* const*>(internal + 0x6f0);
	printf("actor multi scene_array6e8_%s=%u/%u\n", label,
		first ? static_cast<unsigned>(last - first) : 0u,
		first ? static_cast<unsigned>(end - first) : 0u);
	if(first)
		{
		printf("actor multi scene_array6e8_values_%s=", label);
		for(const void* const* it = first; it != last && it - first < 6; ++it)
			printf("%s%x", it == first ? "" : ".",
				static_cast<unsigned>(reinterpret_cast<uintptr_t>(*it)));
		printf("\n");
		}
}

static void nxPrintSceneArray6d4(const char* label, const NxScene* scene)
{
	const unsigned char* wrapper = reinterpret_cast<const unsigned char*>(scene);
	const unsigned char* internal = *reinterpret_cast<unsigned char* const*>(wrapper + 0x24);
	const unsigned* first = *reinterpret_cast<unsigned* const*>(internal + 0x6d4);
	const unsigned* last = *reinterpret_cast<unsigned* const*>(internal + 0x6d8);
	const unsigned* end = *reinterpret_cast<unsigned* const*>(internal + 0x6dc);
	printf("actor nonlast actor_ids_%s=%u/%u", label,
		first ? static_cast<unsigned>(last - first) : 0u,
		first ? static_cast<unsigned>(end - first) : 0u);
	for(const unsigned* it = first; it && it != last && it - first < 6; ++it)
		printf(".%x", *it);
	printf("\n");
}

static void nxPrintShapeIndex(const char* label, const NxActor* actor)
{
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(actor);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(bytes + 0x14);
	const unsigned char* shape = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	printf("actor multi shape_index_%s=%x.%x\n", label,
		*reinterpret_cast<const unsigned*>(shape + 0xd4),
		*reinterpret_cast<const unsigned short*>(shape + 0xd8));
}

static void nxPrintAuxArrays(const char* label, const NxScene* scene)
{
	const unsigned char* wrapper = reinterpret_cast<const unsigned char*>(scene);
	const unsigned char* internal = *reinterpret_cast<unsigned char* const*>(wrapper + 0x24);
	const unsigned char* aux = *reinterpret_cast<unsigned char* const*>(internal + 0x48);
	printf("actor multi aux_arrays_%s=", label);
	for(unsigned offset = 0x40; offset <= 0x80; offset += 0x10)
		{
		const void* const* first = aux
			? *reinterpret_cast<void* const* const*>(aux + offset) : 0;
		const void* const* last = aux
			? *reinterpret_cast<void* const* const*>(aux + offset + 4) : 0;
		const void* const* end = aux
			? *reinterpret_cast<void* const* const*>(aux + offset + 8) : 0;
		printf("%s%u/%u", offset == 0x40 ? "" : ".",
			first ? static_cast<unsigned>(last - first) : 0u,
			first ? static_cast<unsigned>(end - first) : 0u);
		}
	printf("\n");
}

static void nxPrintAuxIndexSamples(const char* label, const NxScene* scene)
{
	const unsigned char* wrapper = reinterpret_cast<const unsigned char*>(scene);
	const unsigned char* internal = *reinterpret_cast<unsigned char* const*>(wrapper + 0x24);
	const unsigned char* aux = *reinterpret_cast<unsigned char* const*>(internal + 0x48);
	printf("actor multi aux_indices_%s=", label);
	const unsigned offsets[4] = {0x40u, 0x50u, 0x60u, 0x70u};
	for(unsigned i = 0; i < 4; ++i)
		{
		const unsigned* first = *reinterpret_cast<unsigned* const*>(aux + offsets[i]);
		printf("%s%x,%x,%x,%x", i ? ":" : "", first ? first[0] : 0,
			first ? first[1] : 0, first ? first[2] : 0, first ? first[3] : 0);
		}
	printf("\n");
}

static void nxPrintAuxStaticCounts(const char* label, const NxScene* scene)
{
	const unsigned char* wrapper = reinterpret_cast<const unsigned char*>(scene);
	const unsigned char* internal = *reinterpret_cast<unsigned char* const*>(wrapper + 0x24);
	const unsigned char* aux = *reinterpret_cast<unsigned char* const*>(internal + 0x48);
	const unsigned offsets[5] = {0u, 0x10u, 0x20u, 0x30u, 0x90u};
	printf("actor static aux_counts_%s=", label);
	for(unsigned i = 0; i < 5; ++i)
		{
		const unsigned* first = *reinterpret_cast<unsigned* const*>(aux + offsets[i]);
		const unsigned* last = *reinterpret_cast<unsigned* const*>(aux + offsets[i] + 4);
		const unsigned* end = *reinterpret_cast<unsigned* const*>(aux + offsets[i] + 8);
		printf("%s%u/%u", i ? "." : "",
			first ? static_cast<unsigned>(last - first) : 0u,
			first ? static_cast<unsigned>(end - first) : 0u);
		}
	printf("\n");
	for(unsigned j = 0; j < 5; ++j)
		{
		const unsigned* first = *reinterpret_cast<unsigned* const*>(aux + offsets[j]);
		printf("actor static aux_sample_%s_%x=", label, offsets[j]);
		for(unsigned i = 0; i < 6; ++i)
			printf("%s%x", i ? "." : "", first
				? (j == 4 ? (first[i] ? 1u : 0u) : first[i]) : 0u);
		printf("\n");
		}
}

static void nxProbePublicShapes(const char* label, const NxActor* actor)
{
	const NxU32 count = actor->getNbShapes();
	NxShape** shapes = actor->getShapes();
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	const unsigned char* shapeObject = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	const unsigned kind = shapeObject ? *reinterpret_cast<const unsigned*>(shapeObject + 0xd0) : 0;
	const unsigned char* const* children = shapeObject && kind == 5
		? *reinterpret_cast<unsigned char* const* const*>(shapeObject + 0xe0) : 0;
	printf("actor shape_kind_%s=%u\n", label, kind);
	const void* actorLink = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x10);
	printf("actor shape_link_%s=%u.%u\n", label, actorLink ? 1u : 0u,
		shapes == actorLink ? 1u : 0u);
	NxShape** expectedArray = children
		? *reinterpret_cast<NxShape***>(const_cast<unsigned char*>(shapeObject + 0xf0))
		: reinterpret_cast<NxShape**>(const_cast<unsigned char*>(shapeObject + 0x9c));
	printf("actor shape_public_%s=%u.%u.%u\n", label, count, shapes ? 1u : 0u,
		shapes == expectedArray ? 1u : 0u);
	if(shapes && count <= 3)
		for(NxU32 i = 0; i < count; ++i)
			{
			const uintptr_t page = reinterpret_cast<uintptr_t>(shapes[i]) & ~static_cast<uintptr_t>(0xfff);
			const unsigned size = *reinterpret_cast<const unsigned*>(page);
			printf("actor shape_public_item_%s_%u=%u.%u.%u\n", label, i,
				shapes[i] ? 1u : 0u,
				shapes[i] == reinterpret_cast<const NxShape*>(shapeObject) ? 1u : 0u,
				children && shapes[i] == reinterpret_cast<const NxShape*>(children[i]) ? 1u : 0u);
			const unsigned char* expectedShape = children ? children[i] : shapeObject;
			printf("actor shape_public_handle_%s_%u=%x.%u.%u.%u\n", label, i,
				size,
				*reinterpret_cast<void* const*>(shapes[i]) ? 1u : 0u,
				*reinterpret_cast<const unsigned*>(reinterpret_cast<const unsigned char*>(shapes[i]) + 4),
				*reinterpret_cast<unsigned char* const*>(reinterpret_cast<const unsigned char*>(shapes[i]) + 8)
					== expectedShape ? 1u : 0u);
			const unsigned char* handle = reinterpret_cast<const unsigned char*>(shapes[i]);
			printf("actor shape_handle_links_%s_%u=%u.%u.%u.%u.%u\n", label, i,
				*reinterpret_cast<unsigned char* const*>(handle + 8) == expectedShape ? 1u : 0u,
				*reinterpret_cast<unsigned char* const*>(handle + 0xc) == expectedShape ? 1u : 0u,
				*reinterpret_cast<unsigned char* const*>(handle + 0x10) == expectedShape ? 1u : 0u,
				*reinterpret_cast<unsigned char* const*>(handle + 0x14) == expectedShape ? 1u : 0u,
				*reinterpret_cast<unsigned char* const*>(handle + 0x18) == expectedShape ? 1u : 0u);
			if(*reinterpret_cast<void* const*>(shapes[i]))
				{
				NxBoxShape* boxShape = shapes[i]->isBox();
				printf("actor shape_methods_%s_%u=%u.%u.%u\n", label, i,
					static_cast<unsigned>(shapes[i]->getType()),
					&shapes[i]->getActor() == actor ? 1u : 0u,
					boxShape == shapes[i] ? 1u : 0u);
				printf("actor shape_is_sphere_%s_%u=%u\n", label, i,
					shapes[i]->isSphere() ? 1u : 0u);
				if(boxShape)
					{
					const NxVec3& dimensions = boxShape->getDimensions();
					printf("actor shape_dimensions_alias_%s_%u=%u\n", label, i,
						&dimensions == reinterpret_cast<const NxVec3*>(expectedShape + 0xe4)
							? 1u : 0u);
					printf("actor shape_dimensions_%s_%u=%08x.%08x.%08x\n", label, i,
						*reinterpret_cast<const unsigned*>(&dimensions.x),
						*reinterpret_cast<const unsigned*>(&dimensions.y),
						*reinterpret_cast<const unsigned*>(&dimensions.z));
					}
				}
			}
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
	const unsigned beforeSceneCreateAlloc = allocator.allocations();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	#ifdef NX_PHYSICS_EMPTY_SCENE_ONLY
	const bool emptySceneOnly = true;
	#else
	const bool emptySceneOnly = getenv("NX_PHYSICS_PROBE_EMPTY_SCENE_RELEASE") != 0;
	#endif
	if(emptySceneOnly)
		{
		printf("actor empty_scene_allocs=%u\n", allocator.allocations() - beforeSceneCreateAlloc);
		printf("actor empty_scene_alloc_sizes=");
		for(unsigned i = 0; i < allocator.allocations() - beforeSceneCreateAlloc; ++i)
			printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
				allocator.allocations() - beforeSceneCreateAlloc - 1 - i));
		printf("\n");
		const unsigned beforeSceneFree = allocator.frees();
		sdk->releaseScene(*scene);
		printf("actor empty_scene_frees=%u\n", allocator.frees() - beforeSceneFree);
		printf("actor empty_scene_sizes=");
		for(unsigned i = 0; i < allocator.frees() - beforeSceneFree; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - beforeSceneFree - 1 - i));
		printf("\n");
		const unsigned beforeSdkFree = allocator.frees();
		sdk->release();
		printf("actor empty_sdk_frees=%u\n", allocator.frees() - beforeSdkFree);
		printf("actor empty_sdk_sizes=");
		for(unsigned i = 0; i < allocator.frees() - beforeSdkFree; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - beforeSdkFree - 1 - i));
		printf("\n");
		return nxReportPairIdentity(pairDirectory);
		}
	#ifdef NX_PHYSICS_DYNAMIC_FIRST_ONLY
		{
		NxBoxShapeDesc firstBox;
		firstBox.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
		NxBodyDesc firstBody;
		NxActorDesc firstDesc;
		firstDesc.body = &firstBody;
		firstDesc.density = 1.0f;
		firstDesc.shapes.pushBack(&firstBox);
		const unsigned beforeAllocations = allocator.allocations();
		const unsigned beforeFrees = allocator.frees();
		NxActor* firstActor = scene->createActor(firstDesc);
		printf("actor dynamic_first created=%u allocs=%u frees=%u\n",
			firstActor ? 1u : 0u, allocator.allocations() - beforeAllocations,
			allocator.frees() - beforeFrees);
		printf("actor dynamic_first sizes=");
		for(unsigned i = 0; i < allocator.allocations() - beforeAllocations; ++i)
			printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
				allocator.allocations() - beforeAllocations - 1 - i));
		printf("\n");
		printf("actor dynamic_first free_sizes=");
		for(unsigned i = 0; i < allocator.frees() - beforeFrees; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - beforeFrees - 1 - i));
		printf("\n");
		const unsigned char* internal = *reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(scene) + 0x24);
		printf("actor dynamic_first scene=%u.%u.%u.%u.%u\n",
			*reinterpret_cast<const unsigned*>(internal + 4),
			*reinterpret_cast<void* const*>(internal + 8) ? 1u : 0u,
			*reinterpret_cast<void* const*>(internal + 0xc) ? 1u : 0u,
			*reinterpret_cast<void* const*>(internal + 0x640) ? 1u : 0u,
			*reinterpret_cast<void* const*>(internal + 0x648) ? 1u : 0u);
		const unsigned char* manager = *reinterpret_cast<unsigned char* const*>(
			internal + 0x648);
		const unsigned char* firstActorBody = firstActor
			? *reinterpret_cast<unsigned char* const*>(
				reinterpret_cast<const unsigned char*>(firstActor) + 0x14) : 0;
		const unsigned char* shape = firstActorBody
			? *reinterpret_cast<unsigned char* const*>(firstActorBody + 0x10) : 0;
		printf("actor dynamic_first prune=%u/%u.%u.%u.%u\n",
			manager ? *reinterpret_cast<const unsigned short*>(manager + 0x10) : 0u,
			manager ? *reinterpret_cast<const unsigned short*>(manager + 0x12) : 0u,
			manager && shape && manager == *reinterpret_cast<unsigned char* const*>(
				shape + 0xc4) ? 1u : 0u,
			*reinterpret_cast<void* const*>(internal + 0x6a4) ? 1u : 0u,
			*reinterpret_cast<const unsigned*>(internal + 0x14));
		if(!firstActor) return nxFail("dynamic-first actor creation failed");
		sdk->releaseScene(*scene);
		sdk->release();
		return nxReportPairIdentity(pairDirectory);
		}
	#endif
	if(getenv("NX_PHYSICS_PROBE_MULTI"))
		{ nxPrintBroadphase("initial", scene); nxPrintSceneArray6e8("initial", scene); }
	nxPrintAuxArrays("initial", scene);

	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&box);
	staticDesc.globalPose.t = NxVec3(2.0f, -1.0f, 4.0f);
	const unsigned beforeStaticAllocations = allocator.allocations();
	const unsigned beforeStaticFrees = allocator.frees();
	NxActor* staticActor = scene->createActor(staticDesc);
	printf("actor static init_aux_prefix=");
	if(allocator.allocations() - beforeStaticAllocations < 11)
		printf("insufficient");
	else
		for(unsigned i = 0; i < 11; ++i)
			printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
				allocator.allocations() - beforeStaticAllocations - 1 - i));
	printf("\n");
	printf("actor static init_allocs=%u\n", allocator.allocations() - beforeStaticAllocations);
	printf("actor static init_sizes=");
	for(unsigned i = 0; i < allocator.allocations() - beforeStaticAllocations; ++i)
		printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
			allocator.allocations() - beforeStaticAllocations - 1 - i));
	printf("\n");
	printf("actor static init_frees=%u\n", allocator.frees() - beforeStaticFrees);
	printf("actor static init_free_sizes=");
	for(unsigned i = 0; i < allocator.frees() - beforeStaticFrees; ++i)
		printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
			allocator.frees() - beforeStaticFrees - 1 - i));
	printf("\n");
	const unsigned char* staticInternal = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(scene) + 0x24);
	const unsigned char* staticBody = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(staticActor) + 0x14);
	const unsigned char* staticShape = *reinterpret_cast<unsigned char* const*>(
		staticBody + 0x10);
	const unsigned char* staticPruner = *reinterpret_cast<unsigned char* const*>(
		staticInternal + 0x640);
	const void* const* staticReferences = staticPruner
		? *reinterpret_cast<void* const* const*>(staticPruner + 0x18) : 0;
	printf("actor static prune_initial=%u.%u.%u.%u.%u.%u/%u.%u.%u.%u.%u\n",
		*reinterpret_cast<const unsigned*>(staticInternal + 4),
		*reinterpret_cast<void* const*>(staticInternal + 8) ? 1u : 0u,
		*reinterpret_cast<void* const*>(staticInternal + 0xc) ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(staticInternal + 0x14),
		staticPruner && staticPruner == *reinterpret_cast<unsigned char* const*>(
			staticShape + 0xc4) ? 1u : 0u,
		staticPruner ? *reinterpret_cast<const unsigned short*>(staticPruner + 0x10) : 0u,
		staticPruner ? *reinterpret_cast<const unsigned short*>(staticPruner + 0x12) : 0u,
		staticPruner ? *reinterpret_cast<const unsigned*>(staticPruner + 8) : 0u,
		staticPruner ? *reinterpret_cast<const unsigned*>(staticPruner + 0x38) : 0u,
		staticPruner ? *reinterpret_cast<const unsigned*>(staticPruner + 0x40) : 0u,
		staticReferences && staticReferences[0] == staticShape + 0xa4 ? 1u : 0u);
	if(getenv("NX_PHYSICS_PROBE_MULTI"))
		{
		nxPrintBroadphase("first", scene);
		nxPrintSceneArray6e8("first", scene);
		}
	nxPrintAuxArrays("static", scene);
	nxPrintAuxStaticCounts("static", scene);
	printf("actor static created=%u\n", staticActor ? 1u : 0u);
	if(!staticActor) return nxFail("static actor creation failed");
	nxProbePublicShapes("static", staticActor);
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
	const unsigned beforeDynamicAllocations = allocator.allocations();
	const unsigned beforeDynamicFrees = allocator.frees();
	NxActor* dynamicActor = scene->createActor(dynamicDesc);
		{
		printf("actor multi first_dynamic_allocs=%u\n",
			allocator.allocations() - beforeDynamicAllocations);
		printf("actor multi first_dynamic_sizes=");
		for(unsigned i = 0; i < allocator.allocations() - beforeDynamicAllocations; ++i)
			printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
				allocator.allocations() - beforeDynamicAllocations - 1 - i));
		printf("\n");
		printf("actor multi first_dynamic_frees=%u\n", allocator.frees() - beforeDynamicFrees);
		printf("actor multi first_dynamic_free_sizes=");
		for(unsigned i = 0; i < allocator.frees() - beforeDynamicFrees; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - beforeDynamicFrees - 1 - i));
		printf("\n");
		nxPrintBroadphase("dynamic", scene);
		nxPrintSceneArray6e8("dynamic", scene);
		nxPrintAuxArrays("dynamic", scene);
		nxPrintAuxIndexSamples("dynamic", scene);
		nxPrintAuxStaticCounts("dynamic", scene);
		}
	printf("actor dynamic created=%u\n", dynamicActor ? 1u : 0u);
	if(!dynamicActor) return nxFail("dynamic actor creation failed");
	nxProbePublicShapes("dynamic", dynamicActor);
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
	if(getenv("NX_PHYSICS_PROBE_MULTI")) nxPrintBroadphase("rotated", scene);
	nxPrintAuxArrays("rotated", scene);
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
	if(getenv("NX_PHYSICS_PROBE_MULTI")) nxPrintBroadphase("quarter", scene);
	nxPrintAuxArrays("quarter", scene);
	nxPrintAuxIndexSamples("quarter", scene);
	if(getenv("NX_PHYSICS_PROBE_MULTI")) nxPrintShapeIndex("quarter", quarterActor);
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
	if(getenv("NX_PHYSICS_PROBE_MULTI")) nxPrintSceneArray6e8("quarter_released", scene);
	nxPrintAuxArrays("quarter_released", scene);
	nxPrintAuxIndexSamples("quarter_released", scene);
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

	const unsigned freesBeforeStatic = allocator.frees();
	if(getenv("NX_PHYSICS_PROBE_MULTI")) nxPrintShapeIndex("static", staticActor);
	scene->releaseActor(*staticActor);
	const unsigned char* releasedPruner = *reinterpret_cast<unsigned char* const*>(
		staticInternal + 0x640);
	const void* const* releasedReferences = releasedPruner
		? *reinterpret_cast<void* const* const*>(releasedPruner + 0x18) : 0;
	printf("actor static prune_released=%u.%u.%u.%u.%u.%u\n",
		releasedPruner ? 1u : 0u,
		releasedPruner ? *reinterpret_cast<const unsigned short*>(releasedPruner + 0x10) : 0u,
		releasedReferences && releasedReferences[0] ? 1u : 0u,
		releasedPruner ? *reinterpret_cast<const unsigned*>(releasedPruner + 8) : 0u,
		releasedPruner ? *reinterpret_cast<const unsigned*>(releasedPruner + 0x38) : 0u,
		releasedPruner ? *reinterpret_cast<const unsigned*>(releasedPruner + 0x40) : 0u);
	printf("scene static_release_frees=%u\n", allocator.frees() - freesBeforeStatic);
	printf("scene static_release_sizes=%x.%x.%x.%x\n",
		allocator.freedSizeFromEnd(3), allocator.freedSizeFromEnd(2),
		allocator.freedSizeFromEnd(1), allocator.freedSizeFromEnd(0));
	printf("scene actors_after_static=%u\n", scene->getNbActors());
	NxActor** afterStatic = scene->getActors();
	unsigned staticStillListed = 0;
	for(NxU32 i = 0; afterStatic && i < scene->getNbActors(); ++i)
		if(afterStatic[i] == staticActor) staticStillListed = 1;
	printf("scene static_still_listed=%u\n", staticStillListed);

	// The two-box lifecycle and the following non-last actor release are both
	// part of the normal staged-pair differential.
		{
		nxPrintBroadphase("before", scene);
		NxBoxShapeDesc secondBox;
		secondBox.dimensions = NxVec3(2.0f, 1.0f, 1.0f);
		NxActorDesc multiDesc = dynamicDesc;
		multiDesc.shapes.pushBack(&secondBox);
		multiDesc.globalPose.t = NxVec3(7.0f, 1.0f, -2.0f);
		const unsigned beforeAllocations = allocator.allocations();
		const unsigned beforeCreationFrees = allocator.frees();
		NxActor* multiActor = scene->createActor(multiDesc);
		nxPrintAuxArrays("multi", scene);
		nxPrintAuxStaticCounts("multi", scene);
		if(!multiActor) return nxFail("multi-shape actor creation failed");
		nxProbePublicShapes("multi", multiActor);
		printf("actor multi creation_allocs=%u\n", allocator.allocations() - beforeAllocations);
		nxPrintBroadphase("after", scene);
		printf("actor multi creation_frees=%u\n", allocator.frees() - beforeCreationFrees);
		printf("actor multi creation_free_sizes=");
		for(unsigned i = 0; i < allocator.frees() - beforeCreationFrees; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - beforeCreationFrees - 1 - i));
		printf("\n");
		printf("actor multi creation_sizes=");
		for(unsigned i = 0; i < allocator.allocations() - beforeAllocations; ++i)
			printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
				allocator.allocations() - beforeAllocations - 1 - i));
		printf("\n");
		const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(multiActor) + 0x14);
		const unsigned char* group = *reinterpret_cast<unsigned char* const*>(body + 0x10);
		const unsigned char* sceneInternal = *reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(scene) + 0x24);
		const unsigned char* broadphase = *reinterpret_cast<unsigned char* const*>(sceneInternal + 0x648);
		const void* const* broadphaseReferences = broadphase
			? *reinterpret_cast<void* const* const*>(broadphase + 0x18) : 0;
		if(broadphaseReferences && group)
			{
			const unsigned char* const* children =
				*reinterpret_cast<unsigned char* const* const*>(group + 0xe0);
			const unsigned char* dynamicBody = *reinterpret_cast<unsigned char* const*>(
				reinterpret_cast<const unsigned char*>(dynamicActor) + 0x14);
			const unsigned char* rotatedBody = *reinterpret_cast<unsigned char* const*>(
				reinterpret_cast<const unsigned char*>(rotatedActor) + 0x14);
			const unsigned char* dynamicShape = *reinterpret_cast<unsigned char* const*>(dynamicBody + 0x10);
			const unsigned char* rotatedShape = *reinterpret_cast<unsigned char* const*>(rotatedBody + 0x10);
			for(unsigned index = 0; index < 5; ++index)
				{
				printf("actor multi broadphase_ref_%u=%u.%u.%u.%u.%u\n", index,
					broadphaseReferences[index] == dynamicShape + 0xa4 ? 1u : 0u,
					broadphaseReferences[index] == rotatedShape + 0xa4 ? 1u : 0u,
					broadphaseReferences[index] == children[0] + 0xa4 ? 1u : 0u,
					broadphaseReferences[index] == children[1] + 0xa4 ? 1u : 0u,
					broadphaseReferences[index] == group + 0xa4 ? 1u : 0u);
				}
			}
		const unsigned groupSize = group ? *reinterpret_cast<const unsigned*>(
			reinterpret_cast<uintptr_t>(group) & ~static_cast<uintptr_t>(0xfff)) : 0;
		printf("actor multi group_size=%x\n", groupSize);
		if(groupSize == 0x110)
			{
			printf("actor multi group_index=%x.%x\n",
				*reinterpret_cast<const unsigned*>(group + 0xd4),
				*reinterpret_cast<const unsigned short*>(group + 0xd8));
			const unsigned char* const* groupChildren =
				*reinterpret_cast<unsigned char* const* const*>(group + 0xe0);
			for(unsigned childIndex = 0; childIndex < 2; ++childIndex)
				printf("actor multi child_index_%u=%x.%x\n", childIndex,
					*reinterpret_cast<const unsigned*>(groupChildren[childIndex] + 0xd4),
					*reinterpret_cast<const unsigned short*>(groupChildren[childIndex] + 0xd8));
			const void* groupOwner = *reinterpret_cast<void* const*>(group + 4);
			printf("actor multi group_owner=%u owner_is_body=%u owner_is_actor=%u\n",
				groupOwner ? 1u : 0u, groupOwner == body ? 1u : 0u,
				groupOwner == multiActor ? 1u : 0u);
			const unsigned char* internalScene = *reinterpret_cast<unsigned char* const*>(body + 4);
			const unsigned char* publicInternal = *reinterpret_cast<unsigned char* const*>(
				reinterpret_cast<const unsigned char*>(scene) + 0x24);
			printf("actor multi body_scene_public=%u body_scene_internal=%u\n",
				internalScene == reinterpret_cast<const unsigned char*>(scene) ? 1u : 0u,
				internalScene == publicInternal ? 1u : 0u);
			const unsigned char* manager = internalScene
				? *reinterpret_cast<unsigned char* const*>(internalScene + 0x48) : 0;
			printf("actor multi scene_manager=%u\n", manager ? 1u : 0u);
			if(manager)
				{
				const uintptr_t managerAddress = reinterpret_cast<uintptr_t>(manager);
				const uintptr_t page = managerAddress & ~static_cast<uintptr_t>(0xfff);
				const unsigned guardedSize = *reinterpret_cast<const unsigned*>(page);
				printf("actor multi manager_guarded_size=%x\n",
					guardedSize <= 0xffc && managerAddress + guardedSize == page + 0x1000
						? guardedSize : 0u);
				}
			const unsigned arrayOffsets[2] = {0xe0u, 0xf0u};
			for(unsigned arrayIndex = 0; arrayIndex < 2; ++arrayIndex)
				{
				const unsigned base = arrayOffsets[arrayIndex];
				const void* const* first = *reinterpret_cast<void* const* const*>(group + base);
				const void* const* last = *reinterpret_cast<void* const* const*>(group + base + 4);
				const void* const* end = *reinterpret_cast<void* const* const*>(group + base + 8);
				printf("actor multi group_array_%x=%u/%u\n", base,
					first ? static_cast<unsigned>(last - first) : 0u,
					first ? static_cast<unsigned>(end - first) : 0u);
				}
			}
		const unsigned beforeReleaseAllocations = allocator.allocations();
		const unsigned beforeFrees = allocator.frees();
		nxPrintSceneArray6e8("before_release", scene);
		scene->releaseActor(*multiActor);
		nxPrintSceneArray6e8("after_release", scene);
		nxPrintBroadphase("released", scene);
		nxPrintAuxStaticCounts("multi_released", scene);
		printf("actor multi release_allocs=%u\n",
			allocator.allocations() - beforeReleaseAllocations);
		printf("actor multi release_frees=%u\n", allocator.frees() - beforeFrees);
		printf("actor multi release_sizes=");
		for(unsigned i = 0; i < allocator.frees() - beforeFrees; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - beforeFrees - 1 - i));
		printf("\n");
		}
		{
		nxPrintAuxArrays("before_nonlast", scene);
		nxPrintAuxIndexSamples("before_nonlast", scene);
		nxPrintBroadphase("before_nonlast", scene);
		nxPrintSceneArray6e8("before_nonlast", scene);
		nxPrintSceneArray6d4("before", scene);
		const unsigned char* nonlastBody = *reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(dynamicActor) + 0x14);
		printf("actor nonlast body_c=%x\n", *reinterpret_cast<const unsigned*>(nonlastBody + 0xc));
		const unsigned allocationsBeforeNonlast = allocator.allocations();
		const unsigned freesBeforeNonlast = allocator.frees();
		scene->releaseActor(*dynamicActor);
		nxPrintAuxArrays("after_nonlast", scene);
		nxPrintAuxStaticCounts("nonlast", scene);
		nxPrintAuxIndexSamples("after_nonlast", scene);
		nxPrintBroadphase("after_nonlast", scene);
		nxPrintSceneArray6e8("after_nonlast", scene);
		nxPrintSceneArray6d4("after", scene);
		printf("actor nonlast release_allocs=%u\n", allocator.allocations() - allocationsBeforeNonlast);
		printf("actor nonlast release_alloc_sizes=");
		for(unsigned i = 0; i < allocator.allocations() - allocationsBeforeNonlast; ++i)
			printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
				allocator.allocations() - allocationsBeforeNonlast - 1 - i));
		printf("\n");
		printf("actor nonlast release_frees=%u\n", allocator.frees() - freesBeforeNonlast);
		printf("actor nonlast release_sizes=");
		for(unsigned i = 0; i < allocator.frees() - freesBeforeNonlast; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - freesBeforeNonlast - 1 - i));
		printf("\n");
		printf("actor nonlast actors=%u\n", scene->getNbActors());
		}
		{
		const unsigned beforeReuseAllocations = allocator.allocations();
		NxActor* reusedActor = scene->createActor(dynamicDesc);
		printf("actor reuse created=%u\n", reusedActor ? 1u : 0u);
		printf("actor reuse creation_allocs=%u\n", allocator.allocations() - beforeReuseAllocations);
		printf("actor reuse creation_sizes=");
		for(unsigned i = 0; i < allocator.allocations() - beforeReuseAllocations; ++i)
			printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
				allocator.allocations() - beforeReuseAllocations - 1 - i));
		printf("\n");
		nxPrintAuxArrays("reuse", scene);
		nxPrintAuxStaticCounts("reuse", scene);
		nxPrintAuxIndexSamples("reuse", scene);
		nxPrintSceneArray6d4("reuse", scene);
		nxPrintSceneArray6e8("reuse", scene);
		nxPrintBroadphase("reuse", scene);
		if(reusedActor)
			{
			const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
				reinterpret_cast<const unsigned char*>(reusedActor) + 0x14);
			printf("actor reuse body_c=%x\n", *reinterpret_cast<const unsigned*>(body + 0xc));
			nxPrintShapeIndex("reuse", reusedActor);
			NxShape* shape = reusedActor->getShapes()[0];
			const unsigned char* internalShape = *reinterpret_cast<unsigned char* const*>(
				reinterpret_cast<const unsigned char*>(shape) + 0x18);
			printf("actor box defaults=%u.%u.%x.%x\n",
				static_cast<unsigned>(shape->getGroup()),
				static_cast<unsigned>(shape->getMaterial()),
				*reinterpret_cast<const unsigned*>(internalShape + 0xc8),
				*reinterpret_cast<const unsigned*>(internalShape + 0xd8));
			shape->setGroup(5);
			shape->setMaterial(1);
			printf("actor box mutated=%u.%u.%x.%x\n",
				static_cast<unsigned>(shape->getGroup()),
				static_cast<unsigned>(shape->getMaterial()),
				*reinterpret_cast<const unsigned*>(internalShape + 0xc8),
				*reinterpret_cast<const unsigned*>(internalShape + 0xd8));
			printf("actor box flags_default=%x.%x.%x\n",
				shape->getFlag(NX_SF_VISUALIZATION),
				shape->getFlag(NX_SF_FEATURE_INDICES),
				*reinterpret_cast<const NxU16*>(internalShape + 0xde));
			shape->setFlag(NX_SF_FEATURE_INDICES, true);
			shape->setFlag(NX_SF_VISUALIZATION, false);
			printf("actor box flags_changed=%x.%x.%x\n",
				shape->getFlag(NX_SF_VISUALIZATION),
				shape->getFlag(NX_SF_FEATURE_INDICES),
				*reinterpret_cast<const NxU16*>(internalShape + 0xde));
			shape->setFlag(NX_SF_FEATURE_INDICES, false);
			printf("actor box flags_cleared=%x.%x.%x\n",
				shape->getFlag(NX_SF_VISUALIZATION),
				shape->getFlag(NX_SF_FEATURE_INDICES),
				*reinterpret_cast<const NxU16*>(internalShape + 0xde));
			static const char firstName[] = "box-one";
			static const char secondName[] = "box-two";
			printf("actor box name_default=%u\n", shape->getName() == 0);
			const unsigned beforeNameAlloc = allocator.allocations();
			const unsigned beforeNameFree = allocator.frees();
			shape->setName(firstName);
			printf("actor box name_first=%u.%s\n",
				shape->getName() == firstName, shape->getName());
			shape->setName(secondName);
			printf("actor box name_second=%u.%s\n",
				shape->getName() == secondName, shape->getName());
			shape->setName(0);
			printf("actor box name_cleared=%u\n", shape->getName() == 0);
			printf("actor box name_allocs=%u.%u\n",
				allocator.allocations() - beforeNameAlloc,
				allocator.frees() - beforeNameFree);
			printf("actor box name_alloc_sizes=");
			for(unsigned i = 0; i < allocator.allocations() - beforeNameAlloc; ++i)
				printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
					allocator.allocations() - beforeNameAlloc - 1 - i));
			printf("\nactor box name_free_sizes=");
			for(unsigned i = 0; i < allocator.frees() - beforeNameFree; ++i)
				printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
					allocator.frees() - beforeNameFree - 1 - i));
			printf("\n");
			}
		}

	// A second creation proves descriptor values reach the internal shape before
	// any public setter runs.
	{
		box.group = 7;
		box.materialIndex = 2;
		box.shapeFlags = NX_SF_FEATURE_INDICES;
		static const char descriptorName[] = "box-descriptor";
		box.name = descriptorName;
		const unsigned beforeNamedAlloc = allocator.allocations();
		NxActor* descriptorActor = scene->createActor(dynamicDesc);
		printf("actor box descriptor_created=%u\n", descriptorActor ? 1u : 0u);
		if(descriptorActor)
		{
			NxShape* shape = descriptorActor->getShapes()[0];
			const unsigned char* internalShape = *reinterpret_cast<unsigned char* const*>(
				reinterpret_cast<const unsigned char*>(shape) + 0x18);
			printf("actor box descriptor=%u.%u.%x.%x\n",
				static_cast<unsigned>(shape->getGroup()),
				static_cast<unsigned>(shape->getMaterial()),
				*reinterpret_cast<const unsigned*>(internalShape + 0xc8),
				*reinterpret_cast<const unsigned*>(internalShape + 0xd8));
			printf("actor box descriptor_flags=%x.%x.%x\n",
				shape->getFlag(NX_SF_VISUALIZATION),
				shape->getFlag(NX_SF_FEATURE_INDICES),
				*reinterpret_cast<const NxU16*>(internalShape + 0xde));
			printf("actor box descriptor_name=%u.%s\n",
				shape->getName() == descriptorName, shape->getName());
			printf("actor box descriptor_allocs=%u\n",
				allocator.allocations() - beforeNamedAlloc);
			printf("actor box descriptor_alloc_sizes=");
			for(unsigned i = 0; i < allocator.allocations() - beforeNamedAlloc; ++i)
				printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
					allocator.allocations() - beforeNamedAlloc - 1 - i));
			printf("\n");
			const unsigned beforeNamedFree = allocator.frees();
			scene->releaseActor(*descriptorActor);
			printf("actor box descriptor_release_frees=%u\n",
				allocator.frees() - beforeNamedFree);
			printf("actor box descriptor_release_sizes=");
			for(unsigned i = 0; i < allocator.frees() - beforeNamedFree; ++i)
				printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
					allocator.frees() - beforeNamedFree - 1 - i));
			printf("\n");
			}
		}

	const unsigned liveActors = scene->getNbActors();
	NxActor* liveActorPointers[16] = {};
	NxActor** liveActorArray = scene->getActors();
	for(unsigned i = 0; i < liveActors && i < 16; ++i)
		liveActorPointers[i] = liveActorArray[i];
	const unsigned beforeSceneReleaseAllocs = allocator.allocations();
	const unsigned beforeSceneReleaseFrees = allocator.frees();
	sdk->releaseScene(*scene);
	unsigned freedLiveActors = 0;
	for(unsigned i = 0; i < liveActors && i < 16; ++i)
		for(unsigned j = 0; j < allocator.frees() - beforeSceneReleaseFrees; ++j)
			if(allocator.freedPointerFromEnd(
				allocator.frees() - beforeSceneReleaseFrees - 1 - j) == liveActorPointers[i])
				{
				++freedLiveActors;
				break;
				}
	printf("actor scene_live_actor_frees=%u.%u\n", liveActors, freedLiveActors);
	if(getenv("NX_PHYSICS_PROBE_SCENE_TEARDOWN"))
		{
		printf("actor scene_teardown_allocs=%u\n",
			allocator.allocations() - beforeSceneReleaseAllocs);
		printf("actor scene_teardown_alloc_sizes=");
		for(unsigned i = 0; i < allocator.allocations() - beforeSceneReleaseAllocs; ++i)
			printf("%s%x", i ? "." : "", allocator.allocSizeFromEnd(
				allocator.allocations() - beforeSceneReleaseAllocs - 1 - i));
		printf("\n");
		printf("actor scene_teardown_frees=%u\n",
			allocator.frees() - beforeSceneReleaseFrees);
		printf("actor scene_teardown_sizes=");
		for(unsigned i = 0; i < allocator.frees() - beforeSceneReleaseFrees; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - beforeSceneReleaseFrees - 1 - i));
		printf("\n");
		}
	const unsigned beforeSdkReleaseFrees = allocator.frees();
	sdk->release();
	if(getenv("NX_PHYSICS_PROBE_SDK_TEARDOWN"))
		{
		printf("actor sdk_teardown_frees=%u\n",
			allocator.frees() - beforeSdkReleaseFrees);
		printf("actor sdk_teardown_sizes=");
		for(unsigned i = 0; i < allocator.frees() - beforeSdkReleaseFrees; ++i)
			printf("%s%x", i ? "." : "", allocator.freedSizeFromEnd(
				allocator.frees() - beforeSdkReleaseFrees - 1 - i));
		printf("\n");
		}
	return nxReportPairIdentity(pairDirectory);
}
