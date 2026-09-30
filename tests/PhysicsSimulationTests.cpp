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

	NxVec3 observedGravity;
	scene->getGravity(observedGravity);
	printf("simulation gravity=initial %08x.%08x.%08x\n",
		nxFloatBits(observedGravity.x), nxFloatBits(observedGravity.y), nxFloatBits(observedGravity.z));
	const NxVec3 changedGravity(0.0f, -20.0f, 0.0f);
	scene->setGravity(changedGravity);
	scene->getGravity(observedGravity);
	printf("simulation gravity=changed %08x.%08x.%08x\n",
		nxFloatBits(observedGravity.x), nxFloatBits(observedGravity.y), nxFloatBits(observedGravity.z));
	scene->setGravity(sceneDesc.gravity);

	NxReal maxTimestep = 0.0f;
	NxU32 maxIter = 0;
	NxTimeStepMethod method = NX_TIMESTEP_VARIABLE;
	scene->getTiming(maxTimestep, maxIter, method);
	printf("simulation timing=initial %08x.%u.%u\n",
		nxFloatBits(maxTimestep), maxIter, static_cast<unsigned>(method));
	scene->setTiming(0.125f, 4, NX_TIMESTEP_VARIABLE);
	scene->getTiming(maxTimestep, maxIter, method);
	printf("simulation timing=changed %08x.%u.%u\n",
		nxFloatBits(maxTimestep), maxIter, static_cast<unsigned>(method));
	scene->setTiming(sceneDesc.maxTimestep, sceneDesc.maxIter, sceneDesc.timeStepMethod);
	printf("simulation writable=initial %u\n", scene->isWritable() ? 1u : 0u);

	// Pin the worker/event completion path independently of body integration.
	NxScene* emptyScene = sdk->createScene(sceneDesc);
	if(!emptyScene)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("empty worker scene creation failed");
		}
	emptyScene->simulate(0.125f);
	const bool emptyReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool emptyFetched = emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	printf("simulation empty-step ready=%u fetched=%u\n",
		emptyReady ? 1u : 0u, emptyFetched ? 1u : 0u);
	sdk->releaseScene(*emptyScene);

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
	for(unsigned step = 8; step < 1000; ++step)
		{
		scene->simulate(0.125f);
		const bool ready = scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("simulation soak result was not ready and fetched");
		}
	printf("simulation soak steps=1000 ready=1 fetched=1\n");
	nxPrintActorState("soak1000", *actor);

	scene->startRun(0.01f);
	scene->finishRun();
	printf("simulation legacy=start-finish returned\n");
	scene->runFor(0.01f, 0.1f, 1, NX_TIMESTEP_VARIABLE);
	printf("simulation legacy=runFor returned\n");

	sdk->releaseScene(*scene);
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
