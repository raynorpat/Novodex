#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static NxU32 nxCcdFloatBits(NxReal value)
	{
	union { NxReal real; NxU32 bits; } result;
	result.real = value;
	return result.bits;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int opened = nxOpenPair(argc, argv, "NxPhysicsCcdSimulationTests",
		pairDirectory, &physics);
	if(opened)
		return opened;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		return nxFail("NxCreatePhysicsSDK missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk)
		return nxFail("SDK creation failed");
	if(!sdk->setParameter(NX_CONTINUOUS_CD, 1.0f))
		return nxFail("enabling continuous collision detection failed");

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		return nxFail("CCD scene creation failed");
	scene->setTiming(0.1f, 1, NX_TIMESTEP_VARIABLE);

	NxBoxShapeDesc wallShape;
	wallShape.dimensions = NxVec3(0.1f, 2.0f, 2.0f);
	NxActorDesc wallDesc;
	wallDesc.shapes.pushBack(&wallShape);
	NxActor* wall = scene->createActor(wallDesc);

	NxBoxShapeDesc moverShape;
	moverShape.dimensions = NxVec3(0.25f, 0.25f, 0.25f);
	NxBodyDesc moverBody;
	NxActorDesc moverDesc;
	moverDesc.body = &moverBody;
	moverDesc.density = 1.0f;
	moverDesc.globalPose.t = NxVec3(-3.0f, 0.0f, 0.0f);
	moverDesc.shapes.pushBack(&moverShape);
	NxActor* mover = scene->createActor(moverDesc);
	if(!wall || !mover)
		return nxFail("CCD fixture actor creation failed");
	mover->setLinearVelocity(NxVec3(100.0f, 0.0f, 0.0f));

	for(unsigned step = 0; step != 2; ++step)
		{
		scene->simulate(0.1f);
		const bool ready = scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("CCD simulation result was not ready and fetched");
		NxVec3 velocity;
		mover->getLinearVelocity(velocity);
		if(step == 1 && (velocity.x < 0.399f || velocity.x > 0.401f))
			return nxFail("CCD response velocity is outside its expected range");
		printf("ccd step=%u x=%08x vx=%.3f ready=%u fetched=%u\n", step,
			nxCcdFloatBits(mover->getGlobalPosition().x), velocity.x,
			ready ? 1u : 0u, fetched ? 1u : 0u);
		}

	sdk->releaseScene(*scene);
	sdk->setParameter(NX_CONTINUOUS_CD, 0.0f);
	sdk->release();
	const int audited = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return audited;
	}
