#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxMaterial.h"
#include "NxUserContactReport.h"

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

static void nxPrintBoxActorState(const char* stage, NxActor& actor)
	{
	nxPrintActorState(stage, actor);
	const NxQuat q = actor.getGlobalOrientationQuat();
	const NxVec3 w = actor.getAngularVelocity();
	const NxMat34 pose = actor.getGlobalPose();
	NxReal rotation[9];
	pose.M.getRowMajor(rotation);
	NxShape** shapes = actor.getShapes();
	const NxMat34 shapePose = shapes[0]->getGlobalPose();
	NxReal shapeRotation[9];
	shapePose.M.getRowMajor(shapeRotation);
	printf("simulation box-state stage=%s orientation=%08x.%08x.%08x.%08x angular=%08x.%08x.%08x matrix=%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x shape=%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x\n",
		stage, nxFloatBits(q.x), nxFloatBits(q.y), nxFloatBits(q.z), nxFloatBits(q.w),
		nxFloatBits(w.x), nxFloatBits(w.y), nxFloatBits(w.z),
		nxFloatBits(rotation[0]), nxFloatBits(rotation[1]), nxFloatBits(rotation[2]),
		nxFloatBits(rotation[3]), nxFloatBits(rotation[4]), nxFloatBits(rotation[5]),
		nxFloatBits(rotation[6]), nxFloatBits(rotation[7]), nxFloatBits(rotation[8]),
		nxFloatBits(shapePose.t.x), nxFloatBits(shapePose.t.y), nxFloatBits(shapePose.t.z),
		nxFloatBits(shapeRotation[0]), nxFloatBits(shapeRotation[1]), nxFloatBits(shapeRotation[2]),
		nxFloatBits(shapeRotation[3]), nxFloatBits(shapeRotation[4]), nxFloatBits(shapeRotation[5]),
		nxFloatBits(shapeRotation[6]), nxFloatBits(shapeRotation[7]), nxFloatBits(shapeRotation[8]));
	}

static void nxPrintActorMotionState(const char* stage, NxActor& actor)
	{
	const NxVec3& p = actor.getGlobalPosition();
	NxVec3 v;
	NxVec3 w;
	actor.getLinearVelocity(v);
	actor.getAngularVelocity(w);
	printf("simulation motion stage=%s position=%08x.%08x.%08x velocity=%08x.%08x.%08x angular=%08x.%08x.%08x\n",
		stage, nxFloatBits(p.x), nxFloatBits(p.y), nxFloatBits(p.z),
		nxFloatBits(v.x), nxFloatBits(v.y), nxFloatBits(v.z),
		nxFloatBits(w.x), nxFloatBits(w.y), nxFloatBits(w.z));
	}

struct NxSimulationContactReport : NxUserContactReport
	{
	unsigned calls;
	unsigned events;
	unsigned pairs;
	unsigned patches;
	unsigned points;
	unsigned normal[3];
	unsigned point[3];
	unsigned separation;
	unsigned capturedCallbacks;
	unsigned callbackPointCount[8];
	unsigned callbackPoint[8][8][4];

	NxSimulationContactReport()
		: calls(0), events(0), pairs(0), patches(0), points(0), separation(0),
			capturedCallbacks(0)
		{
		memset(normal, 0, sizeof(normal));
		memset(point, 0, sizeof(point));
		memset(callbackPointCount, 0, sizeof(callbackPointCount));
		memset(callbackPoint, 0, sizeof(callbackPoint));
		}

	void onContactNotify(NxContactPair& pair, NxU32 eventFlags)
		{
		const unsigned callback = calls;
		++calls;
		if(callback < 8)
			capturedCallbacks = callback + 1;
		events |= eventFlags;
		NxContactStreamIterator iterator(pair.stream);
		while(iterator.goNextPair())
			{
			++pairs;
			while(iterator.goNextPatch())
				{
				++patches;
				while(iterator.goNextPoint())
					{
					++points;
					if(points == 1)
						{
						const NxVec3& n = iterator.getPatchNormal();
						const NxVec3& p = iterator.getPoint();
						for(unsigned axis = 0; axis < 3; ++axis)
							{
							normal[axis] = nxFloatBits(n[axis]);
							point[axis] = nxFloatBits(p[axis]);
							}
						separation = nxFloatBits(iterator.getSeparation());
						}
					if(callback < 8 && callbackPointCount[callback] < 8)
						{
						const NxVec3& p = iterator.getPoint();
						unsigned* out = callbackPoint[callback][callbackPointCount[callback]++];
						out[0] = nxFloatBits(p.x);
						out[1] = nxFloatBits(p.y);
						out[2] = nxFloatBits(p.z);
						out[3] = nxFloatBits(iterator.getSeparation());
						}
					}
				}
			}
		}
	};

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

	// Exercise contact generation, reporting and response through the same
	// public scene step/result path. Keep this in its own scene so the 1,000-step
	// gravity soak above cannot affect the collision fixture.
	NxSimulationContactReport contactReport;
	NxSceneDesc contactSceneDesc;
	contactSceneDesc.setToDefault();
	contactSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	contactSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	contactSceneDesc.userContactReport = &contactReport;
	NxScene* contactScene = sdk->createScene(contactSceneDesc);
	if(!contactScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("contact scene creation failed");
		}

	NxPlaneShapeDesc ground;
	NxActorDesc groundDesc;
	groundDesc.shapes.pushBack(&ground);
	NxActor* groundActor = contactScene->createActor(groundDesc);
	NxSphereShapeDesc fallingSphere;
	fallingSphere.radius = 0.5f;
	NxBodyDesc fallingBody;
	fallingBody.mass = 1.0f;
	fallingBody.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	NxActorDesc fallingDesc;
	fallingDesc.body = &fallingBody;
	fallingDesc.shapes.pushBack(&fallingSphere);
	fallingDesc.globalPose.t = NxVec3(0.0f, 1.0f, 0.0f);
	NxActor* fallingActor = contactScene->createActor(fallingDesc);
	if(!groundActor || !fallingActor)
		{
		sdk->releaseScene(*contactScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("contact actors creation failed");
		}
	contactScene->setActorPairFlags(*groundActor, *fallingActor,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	for(unsigned step = 0; step < 60; ++step)
		{
		contactScene->simulate(1.0f / 60.0f);
		const bool ready = contactScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = contactScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*contactScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("contact scene result was not ready and fetched");
			}
		char contactStage[16];
		sprintf_s(contactStage, "contact%u", step);
		nxPrintActorState(contactStage, *fallingActor);
		}
	printf("simulation contact callbacks=%u events=%08x pairs=%u patches=%u points=%u normal=%08x.%08x.%08x point=%08x.%08x.%08x separation=%08x\n",
		contactReport.calls, contactReport.events, contactReport.pairs,
		contactReport.patches, contactReport.points,
		contactReport.normal[0], contactReport.normal[1], contactReport.normal[2],
		contactReport.point[0], contactReport.point[1], contactReport.point[2],
		contactReport.separation);
	nxPrintActorState("contact60", *fallingActor);
	sdk->releaseScene(*contactScene);

	// The box-plane path exercises a distinct narrow-phase emitter and the
	// angular contact response. Keep every step in the differential transcript
	// so the first divergence identifies the responsible solver stage.
	NxSimulationContactReport boxContactReport;
	NxSceneDesc boxContactSceneDesc;
	boxContactSceneDesc.setToDefault();
	boxContactSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	boxContactSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	boxContactSceneDesc.userContactReport = &boxContactReport;
	NxScene* boxContactScene = sdk->createScene(boxContactSceneDesc);
	if(!boxContactScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("box contact scene creation failed");
		}
	NxPlaneShapeDesc boxGround;
	NxActorDesc boxGroundDesc;
	boxGroundDesc.shapes.pushBack(&boxGround);
	NxActor* boxGroundActor = boxContactScene->createActor(boxGroundDesc);
	NxBoxShapeDesc fallingBox;
	fallingBox.dimensions = NxVec3(0.5f, 0.5f, 0.5f);
	NxBodyDesc fallingBoxBody;
	fallingBoxBody.mass = 1.0f;
	fallingBoxBody.massSpaceInertia = NxVec3(1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f);
	NxActorDesc fallingBoxDesc;
	fallingBoxDesc.body = &fallingBoxBody;
	fallingBoxDesc.shapes.pushBack(&fallingBox);
	fallingBoxDesc.globalPose.t = NxVec3(0.0f, 1.0f, 0.0f);
	NxActor* fallingBoxActor = boxContactScene->createActor(fallingBoxDesc);
	if(!boxGroundActor || !fallingBoxActor)
		{
		sdk->releaseScene(*boxContactScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("box contact actors creation failed");
		}
	boxContactScene->setActorPairFlags(*boxGroundActor, *fallingBoxActor,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	nxPrintBoxActorState("boxcontact-init", *fallingBoxActor);
	for(unsigned step = 0; step < 60; ++step)
		{
		boxContactScene->simulate(1.0f / 60.0f);
		const bool ready = boxContactScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = boxContactScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*boxContactScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("box contact scene result was not ready and fetched");
			}
		char boxContactStage[20];
		sprintf_s(boxContactStage, "boxcontact%u", step);
		nxPrintBoxActorState(boxContactStage, *fallingBoxActor);
		}
	printf("simulation box-contact callbacks=%u events=%08x pairs=%u patches=%u points=%u normal=%08x.%08x.%08x point=%08x.%08x.%08x separation=%08x\n",
		boxContactReport.calls, boxContactReport.events, boxContactReport.pairs,
		boxContactReport.patches, boxContactReport.points,
		boxContactReport.normal[0], boxContactReport.normal[1], boxContactReport.normal[2],
		boxContactReport.point[0], boxContactReport.point[1], boxContactReport.point[2],
		boxContactReport.separation);
	for(unsigned callback = 0; callback < boxContactReport.capturedCallbacks; ++callback)
		for(unsigned i = 0; i < boxContactReport.callbackPointCount[callback]; ++i)
			printf("simulation box-contact callback=%u point=%u xyzs=%08x.%08x.%08x.%08x\n", callback, i,
				boxContactReport.callbackPoint[callback][i][0], boxContactReport.callbackPoint[callback][i][1],
				boxContactReport.callbackPoint[callback][i][2], boxContactReport.callbackPoint[callback][i][3]);
	nxPrintBoxActorState("box-contact60", *fallingBoxActor);
	sdk->releaseScene(*boxContactScene);

	// A two-dynamic-body impact exercises pair ownership and impulse sharing;
	// the plane fixtures above only cover one movable body against static geometry.
	NxSimulationContactReport spherePairReport;
	NxSceneDesc spherePairSceneDesc;
	spherePairSceneDesc.setToDefault();
	spherePairSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	spherePairSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	spherePairSceneDesc.userContactReport = &spherePairReport;
	NxScene* spherePairScene = sdk->createScene(spherePairSceneDesc);
	if(!spherePairScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("sphere-pair scene creation failed");
		}
	NxSphereShapeDesc pairSphereA;
	NxSphereShapeDesc pairSphereB;
	pairSphereA.radius = 0.5f;
	pairSphereB.radius = 0.5f;
	NxBodyDesc pairBodyA;
	pairBodyA.mass = 1.0f;
	pairBodyA.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	pairBodyA.linearVelocity = NxVec3(1.0f, 0.0f, 0.0f);
	NxActorDesc pairActorDescA;
	pairActorDescA.body = &pairBodyA;
	pairActorDescA.globalPose.t = NxVec3(-1.0f, 0.0f, 0.0f);
	pairActorDescA.shapes.pushBack(&pairSphereA);
	NxBodyDesc pairBodyB;
	pairBodyB.mass = 1.0f;
	pairBodyB.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	pairBodyB.linearVelocity = NxVec3(-1.0f, 0.0f, 0.0f);
	NxActorDesc pairActorDescB;
	pairActorDescB.body = &pairBodyB;
	pairActorDescB.globalPose.t = NxVec3(1.0f, 0.0f, 0.0f);
	pairActorDescB.shapes.pushBack(&pairSphereB);
	NxActor* pairActorA = spherePairScene->createActor(pairActorDescA);
	NxActor* pairActorB = spherePairScene->createActor(pairActorDescB);
	printf("simulation sphere-pair creation valid=%u.%u actors=%u.%u\n",
		pairActorDescA.isValid() ? 1u : 0u, pairActorDescB.isValid() ? 1u : 0u,
		pairActorA ? 1u : 0u, pairActorB ? 1u : 0u);
	if(!pairActorA || !pairActorB)
		{
		sdk->releaseScene(*spherePairScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("sphere-pair actors creation failed");
		}
	spherePairScene->setActorPairFlags(*pairActorA, *pairActorB,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	nxPrintActorState("spherepair-init-a", *pairActorA);
	nxPrintActorState("spherepair-init-b", *pairActorB);
	for(unsigned step = 0; step < 60; ++step)
		{
		spherePairScene->simulate(1.0f / 60.0f);
		const bool ready = spherePairScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = spherePairScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*spherePairScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("sphere-pair scene result was not ready and fetched");
			}
		char pairStage[24];
		sprintf_s(pairStage, "spherepair%u-a", step);
		nxPrintActorState(pairStage, *pairActorA);
		sprintf_s(pairStage, "spherepair%u-b", step);
		nxPrintActorState(pairStage, *pairActorB);
		}
	printf("simulation sphere-pair callbacks=%u events=%08x pairs=%u patches=%u points=%u\n",
		spherePairReport.calls, spherePairReport.events, spherePairReport.pairs,
		spherePairReport.patches, spherePairReport.points);
	nxPrintActorState("spherepair60-a", *pairActorA);
	nxPrintActorState("spherepair60-b", *pairActorB);
	sdk->releaseScene(*spherePairScene);

	// Tangential contact response: a moving sphere settles onto a plane and
	// loses horizontal speed while gaining angular velocity through friction.
	NxMaterial frictionMaterial;
	frictionMaterial.staticFriction = 0.8f;
	frictionMaterial.dynamicFriction = 0.6f;
	const NxMaterialIndex frictionIndex = sdk->addMaterial(frictionMaterial);
	NxSimulationContactReport frictionReport;
	NxSceneDesc frictionSceneDesc;
	frictionSceneDesc.setToDefault();
	frictionSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	frictionSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	frictionSceneDesc.userContactReport = &frictionReport;
	NxScene* frictionScene = sdk->createScene(frictionSceneDesc);
	if(!frictionScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("friction scene creation failed");
		}
	NxPlaneShapeDesc frictionGround;
	frictionGround.materialIndex = frictionIndex;
	NxActorDesc frictionGroundDesc;
	frictionGroundDesc.shapes.pushBack(&frictionGround);
	NxActor* frictionGroundActor = frictionScene->createActor(frictionGroundDesc);
	NxSphereShapeDesc slidingSphere;
	slidingSphere.radius = 0.5f;
	slidingSphere.materialIndex = frictionIndex;
	NxBodyDesc slidingBody;
	slidingBody.mass = 1.0f;
	slidingBody.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	slidingBody.linearVelocity = NxVec3(2.0f, 0.0f, 0.0f);
	NxActorDesc slidingDesc;
	slidingDesc.body = &slidingBody;
	slidingDesc.globalPose.t = NxVec3(0.0f, 0.5f, 0.0f);
	slidingDesc.shapes.pushBack(&slidingSphere);
	NxActor* slidingActor = frictionScene->createActor(slidingDesc);
	printf("simulation friction setup material=%u material_valid=%u ground_valid=%u desc_valid=%u actors=%u.%u\n",
		(unsigned)frictionIndex, frictionMaterial.isValid() ? 1u : 0u,
		frictionGround.isValid() ? 1u : 0u, slidingDesc.isValid() ? 1u : 0u,
		frictionGroundActor ? 1u : 0u, slidingActor ? 1u : 0u);
	if(!frictionGroundActor || !slidingActor)
		{
		sdk->releaseScene(*frictionScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("friction actors creation failed");
		}
	frictionScene->setActorPairFlags(*frictionGroundActor, *slidingActor,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	nxPrintActorMotionState("friction-init", *slidingActor);
	for(unsigned step = 0; step < 60; ++step)
		{
		frictionScene->simulate(1.0f / 60.0f);
		const bool ready = frictionScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = frictionScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*frictionScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("friction scene result was not ready and fetched");
			}
		char frictionStage[24];
		sprintf_s(frictionStage, "friction%u", step);
		nxPrintActorMotionState(frictionStage, *slidingActor);
		}
	printf("simulation friction callbacks=%u events=%08x pairs=%u patches=%u points=%u coefficients=%08x.%08x\n",
		frictionReport.calls, frictionReport.events, frictionReport.pairs,
		frictionReport.patches, frictionReport.points,
		nxFloatBits(frictionMaterial.staticFriction), nxFloatBits(frictionMaterial.dynamicFriction));
	nxPrintActorMotionState("friction60", *slidingActor);
	sdk->releaseScene(*frictionScene);

	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
