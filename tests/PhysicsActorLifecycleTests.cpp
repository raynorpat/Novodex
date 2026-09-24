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

static void nxPrintBodyLink(const char* label, const NxActor* actor)
{
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(actor);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(bytes + 0x14);
	const unsigned char* nested = body
		? *reinterpret_cast<unsigned char* const*>(body + 0x08) : 0;
	printf("actor %s body=%u nested=%u\n", label,
		body ? 1u : 0u, nested ? 1u : 0u);
	if(body)
		{
		const unsigned allocSize = *reinterpret_cast<const unsigned*>(
			reinterpret_cast<uintptr_t>(body) & ~static_cast<uintptr_t>(0xfff));
		printf("actor %s body_alloc=%x\n", label, allocSize);
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
				const unsigned poseSize = *reinterpret_cast<const unsigned*>(
					reinterpret_cast<uintptr_t>(pose) & ~static_cast<uintptr_t>(0xfff));
				const void* cached = *reinterpret_cast<void* const*>(pose + 8);
				printf("actor %s pose_alloc=%x cached=%u cached_nested=%u\n", label,
					poseSize, cached ? 1u : 0u, cached == nested ? 1u : 0u);
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

	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
