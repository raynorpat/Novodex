#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxSphereShapeDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned nxFloatBits(NxReal value)
	{
	unsigned bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

static void nxPrintActorState(const char* stage, NxActor& actor)
	{
	const NxVec3& p = actor.getGlobalPosition();
	NxVec3 v;
	actor.getLinearVelocity(v);
	printf("simulation stage=%s position=%08x.%08x.%08x velocity=%08x.%08x.%08x\n",
		stage, nxFloatBits(p.x), nxFloatBits(p.y), nxFloatBits(p.z),
		nxFloatBits(v.x), nxFloatBits(v.y), nxFloatBits(v.z));
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc != 2)
		return nxFail("usage: NxPhysicsSimulationTests <pair directory>");

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSimulationTests", pairDirectory, &physics);
	if(status)
		return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		{
		FreeLibrary(physics);
		return nxFail("NxCreatePhysicsSDK missing");
		}

	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("scene creation failed");
		}

	NxSphereShapeDesc sphere;
	sphere.radius = 0.5f;
	NxBodyDesc body;
	NxActorDesc actorDesc;
	actorDesc.body = &body;
	actorDesc.density = 1.0f;
	actorDesc.globalPose.t = NxVec3(0.0f, 10.0f, 0.0f);
	actorDesc.shapes.pushBack(&sphere);
	NxActor* actor = scene->createActor(actorDesc);
	if(!actor)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("dynamic sphere creation failed");
		}

	nxPrintActorState("initial", *actor);
	for(unsigned step = 0; step < 8; ++step)
		{
		const float dt = 0.125f;
		scene->simulate(dt);
		bool ready = scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		bool fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		printf("simulation step=%u ready=%u fetched=%u\n", step, ready ? 1u : 0u, fetched ? 1u : 0u);
		char stage[16];
		sprintf_s(stage, "step%u", step);
		nxPrintActorState(stage, *actor);
		}

	sdk->releaseScene(*scene);
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
