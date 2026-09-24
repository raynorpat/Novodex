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
	NxActor* quarterActor = scene->createActor(rotatedDesc);
	printf("actor quarter created=%u\n", quarterActor ? 1u : 0u);
	if(!quarterActor) return nxFail("quarter-turn actor creation failed");
	nxPrintDynamicQuaternion("quarter", quarterActor);
	nxPrintOrientation("quarter", quarterActor->getGlobalOrientationVal());
	nxPrintPublicQuaternion("quarter", quarterActor->getGlobalOrientationQuatVal());
	nxPrintPose("quarter", quarterActor->getGlobalPoseVal());

	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
