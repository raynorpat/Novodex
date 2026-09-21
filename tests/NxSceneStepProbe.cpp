// A minimal probe that exercises the reconstructed scene path in isolation.
//
// Seven rounds established that the harness's crash is a layout-sensitive heap write
// that traces, disabled code and canaries all mask. Application Verifier needs
// administrator rights, which this session does not have. So this probe takes the
// remaining approach: exercise one step at a time and see which step first faults,
// with no instrumentation that changes the allocation order between steps.
//
// The steps, each gated by an argv flag so a run can stop at any of them:
//
//   sdk      create the SDK
//   scene    create a scene
//   actor1   create one actor
//   actor2   create a second actor
//   joint    create a revolute joint between them
//   release  release the scene and the SDK
//
// A run that reaches step N and faults at N+1 names the step. Nothing here prints
// between the steps, so the allocation order is the same in every run.

#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxRevoluteJointDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static NxActor* nxMakeActor(NxScene& scene, float x)
	{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	NxBodyDesc body;
	NxActorDesc desc;
	desc.body = &body;
	desc.density = 1.0f;
	desc.shapes.pushBack(&box);
	desc.globalPose.t = NxVec3(x, 0.0f, 0.0f);
	return scene.createActor(desc);
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc != 4)
		{
		fprintf(stderr, "usage: %s <pair directory> <sha256> <step>\\n",
			"NxSceneStepProbe");
		return 2;
		}

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc - 2, argv, "NxSceneStepProbe", pairDirectory, &physics);
	if(status)
		return status;

	const char* step = 0;
	{
	const wchar_t* w = argv[3];
	static char buffer[32];
	size_t i = 0;
	for(; w[i] && i < sizeof(buffer) - 1; ++i)
		buffer[i] = static_cast<char>(w[i]);
	buffer[i] = 0;
	step = buffer;
	}

	CreatePhysicsSDKFn createSDK =
		reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		{
		FreeLibrary(physics);
		return nxFail("NxCreatePhysicsSDK missing");
		}

	// The step name is printed once, at the start, so a run that faults produces no
	// output after it and the last printed step is the one before the fault.
	printf("probe step=%s starting\\n", step);
	fflush(stdout);

	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}
	if(strcmp(step, "sdk") == 0)
		return nxReportPairIdentity(pairDirectory);

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("scene creation failed");
		}
	if(strcmp(step, "scene") == 0)
		return nxReportPairIdentity(pairDirectory);

	NxActor* a = nxMakeActor(*scene, 0.0f);
	if(!a)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("actor 1 creation failed");
		}
	if(strcmp(step, "actor1") == 0)
		return nxReportPairIdentity(pairDirectory);

	NxActor* b = nxMakeActor(*scene, 4.0f);
	if(!b)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("actor 2 creation failed");
		}
	if(strcmp(step, "actor2") == 0)
		return nxReportPairIdentity(pairDirectory);

	NxRevoluteJointDesc jointDesc;
	jointDesc.setToDefault();
	jointDesc.actor[0] = a;
	jointDesc.actor[1] = b;
	NxJoint* joint = scene->createJoint(jointDesc);
	if(strcmp(step, "joint") == 0)
		return nxReportPairIdentity(pairDirectory);
	if(!joint)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("joint creation failed");
		}

	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}