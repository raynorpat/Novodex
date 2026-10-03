#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxFixedJointDesc.h"
#include "NxSpringDesc.h"
#include "NxDistanceJointDesc.h"
#include "NxRevoluteJointDesc.h"
#include "NxJoint.h"
#include "NxMaterial.h"
#include "NxBounds3.h"
#include "NxUserContactReport.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);
typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);
static unsigned nxFloatBits(NxReal value);

class NxSimulationTriggerReport : public NxUserTriggerReport
	{
	public:
	NxShape* expectedTrigger;
	NxShape* expectedOther;
	unsigned calls;
	unsigned status;
	NxSimulationTriggerReport(NxShape* trigger, NxShape* other)
		: expectedTrigger(trigger), expectedOther(other), calls(0), status(0) {}
	virtual void onTrigger(NxShape& trigger, NxShape& other, NxTriggerFlag event)
		{
		++calls;
		status = static_cast<unsigned>(event);
		printf("simulation fetch-trigger trigger=%u other=%u status=%u\n",
			&trigger == expectedTrigger, &other == expectedOther, status);
		}
	};

class NxSimulationContactReport : public NxUserContactReport
	{
	public:
	NxActor* expectedActor0;
	NxActor* expectedActor1;
	unsigned calls;
	unsigned events;
	NxSimulationContactReport(NxActor* actor0, NxActor* actor1)
		: expectedActor0(actor0), expectedActor1(actor1), calls(0), events(0) {}
	virtual void onContactNotify(NxContactPair& pair, NxU32 eventFlags)
		{
		++calls;
		events = eventFlags;
		printf("simulation fetch-contact actor0=%u actor1=%u events=%08x force=%08x.%08x.%08x\n",
			pair.actors[0] == expectedActor0, pair.actors[1] == expectedActor1,
			eventFlags, nxFloatBits(pair.sumNormalForce.x),
			nxFloatBits(pair.sumNormalForce.y), nxFloatBits(pair.sumNormalForce.z));
		}
	};

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

static void nxPrintActorBodyVelocity(const char* stage, NxActor& actor)
	{
	unsigned char* const wrapper = reinterpret_cast<unsigned char*>(&actor);
	unsigned char* const body = *reinterpret_cast<unsigned char**>(wrapper + 0x14);
	unsigned char* const record = *reinterpret_cast<unsigned char**>(body + 8);
	const NxReal* const velocity = reinterpret_cast<const NxReal*>(record + 0x34);
	printf("simulation body-state=%s velocity=%08x.%08x.%08x\n", stage,
		nxFloatBits(velocity[0]), nxFloatBits(velocity[1]), nxFloatBits(velocity[2]));
	}

// NpScene::mScene is the internal Scene pointer at wrapper offset +0x24;
// the pruning-engine mode is at internal Scene+0x654. Both offsets are pinned
// by the oracle's 000544 -> 001973 initialization path.
static unsigned nxReadSceneBroadPhaseMode(NxScene* scene)
	{
	unsigned char* const wrapper = reinterpret_cast<unsigned char*>(scene);
	unsigned char* const internal = *reinterpret_cast<unsigned char**>(wrapper + 0x24);
	return *reinterpret_cast<unsigned*>(internal + 0x654);
	}

static unsigned nxReadSceneBroadPhasePairCount(NxScene* scene)
	{
	unsigned char* const wrapper = reinterpret_cast<unsigned char*>(scene);
	unsigned char* const internal = *reinterpret_cast<unsigned char**>(wrapper + 0x24);
	unsigned char* node = *reinterpret_cast<unsigned char**>(internal + 0x674);
	unsigned count = 0;
	while(node)
		{
			++count;
			node = *reinterpret_cast<unsigned char**>(node + 8);
		}
	return count;
	}

static void nxPrintBroadPhasePairOrder(NxScene* scene, unsigned selector,
	NxActor** actors, unsigned actorCount)
	{
	unsigned char* const wrapper = reinterpret_cast<unsigned char*>(scene);
	unsigned char* const internal = *reinterpret_cast<unsigned char**>(wrapper + 0x24);
	unsigned char* node = *reinterpret_cast<unsigned char**>(internal + 0x674);
	printf("simulation broadphase order selector=%u pairs=", selector);
	bool first = true;
	while(node)
		{
			unsigned char* const pair = node + 0x14;
			unsigned char* owner[2] = {
				*reinterpret_cast<unsigned char**>(pair),
				*reinterpret_cast<unsigned char**>(pair + 4) };
			unsigned char* body[2] = {
				owner[0] ? *reinterpret_cast<unsigned char**>(owner[0] + 8) : 0,
				owner[1] ? *reinterpret_cast<unsigned char**>(owner[1] + 8) : 0 };
			if(body[0] && body[1])
				{
				const NxReal x0 = *reinterpret_cast<NxReal*>(body[0] + 0x158);
				const NxReal x1 = *reinterpret_cast<NxReal*>(body[1] + 0x158);
				unsigned a = actorCount;
				unsigned b = actorCount;
				for(unsigned actorIndex = 0; actorIndex != actorCount; ++actorIndex)
					{
					const NxReal actorX = actors[actorIndex]->getGlobalPosition().x;
					if(actorX == x0) a = actorIndex;
					if(actorX == x1) b = actorIndex;
					}
				printf("%s%u-%u", first ? "" : ",", a, b);
				first = false;
				}
			node = *reinterpret_cast<unsigned char**>(node + 8);
		}
	printf("\n");
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
	JointDescSetGlobalAnchorFn setGlobalAnchor = reinterpret_cast<JointDescSetGlobalAnchorFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAnchor"));
	JointDescSetGlobalAxisFn setGlobalAxis = reinterpret_cast<JointDescSetGlobalAxisFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAxis"));
	if(!setGlobalAnchor || !setGlobalAxis)
		return nxFail("joint descriptor helpers are missing");
	for(unsigned selector = NX_BROADPHASE_QUADRATIC;
		selector <= NX_BROADPHASE_COHERENT; ++selector)
		{
		NxSceneDesc broadPhaseDesc;
		broadPhaseDesc.setToDefault();
		broadPhaseDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
		NxScene* broadPhaseScene = sdk->createScene(broadPhaseDesc);
		if(!broadPhaseScene)
			return nxFail("broadphase selector scene creation failed");
		printf("simulation broadphase selector=%u mode=%u\n",
			selector, nxReadSceneBroadPhaseMode(broadPhaseScene));
		broadPhaseScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
		NxSphereShapeDesc separatedSphere;
		separatedSphere.radius = 0.5f;
		NxBodyDesc separatedBody0;
		NxActorDesc separatedActor0;
		separatedActor0.body = &separatedBody0;
		separatedActor0.density = 1.0f;
		separatedActor0.globalPose.t = NxVec3(-10.0f, 0.0f, 0.0f);
		separatedActor0.shapes.pushBack(&separatedSphere);
		NxBodyDesc separatedBody1;
		NxActorDesc separatedActor1;
		separatedActor1.body = &separatedBody1;
		separatedActor1.density = 1.0f;
		separatedActor1.globalPose.t = NxVec3(10.0f, 0.0f, 0.0f);
		separatedActor1.shapes.pushBack(&separatedSphere);
		if(!broadPhaseScene->createActor(separatedActor0)
			|| !broadPhaseScene->createActor(separatedActor1))
			return nxFail("separated-pair broadphase actors failed");
		broadPhaseScene->simulate(0.01f);
		if(!broadPhaseScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !broadPhaseScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("separated-pair broadphase step failed");
		printf("simulation broadphase separated selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(broadPhaseScene));
		NxSphereShapeDesc staticSphere;
		staticSphere.radius = 1.0f;
		NxActorDesc staticActor0;
		staticActor0.globalPose.t = NxVec3(100.0f, 0.0f, 0.0f);
		staticActor0.shapes.pushBack(&staticSphere);
		NxActorDesc staticActor1;
		staticActor1.globalPose.t = NxVec3(100.0f, 0.0f, 0.0f);
		staticActor1.shapes.pushBack(&staticSphere);
		if(!broadPhaseScene->createActor(staticActor0)
			|| !broadPhaseScene->createActor(staticActor1))
			return nxFail("static-static broadphase actors failed");
		broadPhaseScene->simulate(0.01f);
		if(!broadPhaseScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !broadPhaseScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("static-static broadphase step failed");
		printf("simulation broadphase static-static selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(broadPhaseScene));
		NxSphereShapeDesc overlappingSphere;
		overlappingSphere.radius = 0.5f;
		NxBodyDesc overlapBody0;
		NxActorDesc overlapActor0;
		overlapActor0.body = &overlapBody0;
		overlapActor0.density = 1.0f;
		overlapActor0.globalPose.t = NxVec3(200.0f, 0.0f, 0.0f);
		overlapActor0.shapes.pushBack(&overlappingSphere);
		NxBodyDesc overlapBody1;
		NxActorDesc overlapActor1;
		overlapActor1.body = &overlapBody1;
		overlapActor1.density = 1.0f;
		overlapActor1.globalPose.t = NxVec3(200.0f, 0.0f, 0.0f);
		overlapActor1.shapes.pushBack(&overlappingSphere);
		NxActorDesc overlapStatic;
		overlapStatic.globalPose.t = NxVec3(400.0f, 0.0f, 0.0f);
		overlapStatic.shapes.pushBack(&overlappingSphere);
		NxBodyDesc overlapDynamicBody;
		NxActorDesc overlapDynamic;
		overlapDynamic.body = &overlapDynamicBody;
		overlapDynamic.density = 1.0f;
		overlapDynamic.globalPose.t = NxVec3(400.0f, 0.0f, 0.0f);
		overlapDynamic.shapes.pushBack(&overlappingSphere);
		if(!broadPhaseScene->createActor(overlapActor0)
			|| !broadPhaseScene->createActor(overlapActor1)
			|| !broadPhaseScene->createActor(overlapStatic)
			|| !broadPhaseScene->createActor(overlapDynamic))
			return nxFail("overlapping broadphase actors failed");
		broadPhaseScene->simulate(0.01f);
		if(!broadPhaseScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !broadPhaseScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("overlapping broadphase step failed");
		printf("simulation broadphase overlapping selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(broadPhaseScene));
		sdk->releaseScene(*broadPhaseScene);
		NxSceneDesc chainDesc;
		chainDesc.setToDefault();
		chainDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
		chainDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
		NxScene* chainScene = sdk->createScene(chainDesc);
		if(!chainScene)
			return nxFail("overlapping broadphase chain scene creation failed");
		chainScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
		NxSphereShapeDesc chainSphere;
		chainSphere.radius = 0.5f;
		NxBodyDesc chainBody0;
		NxActorDesc chainActor0;
		chainActor0.body = &chainBody0;
		chainActor0.density = 1.0f;
		chainActor0.globalPose.t = NxVec3(6.0f, 0.0f, 0.0f);
		chainActor0.shapes.pushBack(&chainSphere);
		NxBodyDesc chainBody1;
		NxActorDesc chainActor1;
		chainActor1.body = &chainBody1;
		chainActor1.density = 1.0f;
		chainActor1.globalPose.t = NxVec3(6.75f, 0.0f, 0.0f);
		chainActor1.shapes.pushBack(&chainSphere);
		NxBodyDesc chainBody2;
		NxActorDesc chainActor2;
		chainActor2.body = &chainBody2;
		chainActor2.density = 1.0f;
		chainActor2.globalPose.t = NxVec3(7.5f, 0.0f, 0.0f);
		chainActor2.shapes.pushBack(&chainSphere);
		NxActor* chainActors[3] = {
			chainScene->createActor(chainActor0),
			chainScene->createActor(chainActor1),
			chainScene->createActor(chainActor2) };
		if(!chainActors[0] || !chainActors[1] || !chainActors[2])
			return nxFail("overlapping broadphase chain actors failed");
		chainScene->simulate(0.01f);
		if(!chainScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !chainScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("overlapping broadphase chain step failed");
		for(unsigned chainIndex = 0; chainIndex != 3; ++chainIndex)
			{
			char stage[32];
			sprintf(stage, "broadphase%u-chain%u", selector, chainIndex);
			nxPrintActorState(stage, *chainActors[chainIndex]);
			}
		nxPrintBroadPhasePairOrder(chainScene, selector, chainActors, 3);
		if(selector == NX_BROADPHASE_COHERENT)
			{
			chainActors[0]->setGlobalPosition(NxVec3(100.0f, 0.0f, 0.0f));
			chainScene->simulate(0.01f);
			if(!chainScene->checkResults(NX_RIGID_BODY_FINISHED, true)
				|| !chainScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
				return nxFail("coherent broadphase separation step failed");
			nxPrintActorBodyVelocity("broadphase2-separated1", *chainActors[1]);
			nxPrintActorBodyVelocity("broadphase2-separated2", *chainActors[2]);
			printf("simulation broadphase moved-apart selector=%u pairs=%u\n",
				selector, nxReadSceneBroadPhasePairCount(chainScene));
			chainActors[0]->setGlobalPosition(NxVec3(6.0f, 0.0f, 0.0f));
			chainScene->simulate(0.01f);
			if(!chainScene->checkResults(NX_RIGID_BODY_FINISHED, true)
				|| !chainScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
				return nxFail("coherent broadphase rejoin step failed");
			nxPrintActorBodyVelocity("broadphase2-rejoin1", *chainActors[1]);
			nxPrintActorBodyVelocity("broadphase2-rejoin2", *chainActors[2]);
			printf("simulation broadphase moved-together selector=%u pairs=%u\n",
				selector, nxReadSceneBroadPhasePairCount(chainScene));
			}
		sdk->releaseScene(*chainScene);
		NxSceneDesc denseDesc;
		denseDesc.setToDefault();
		denseDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
		denseDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
		NxScene* denseScene = sdk->createScene(denseDesc);
		if(!denseScene)
			return nxFail("dense broadphase scene creation failed");
		denseScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
		NxSphereShapeDesc denseSphere;
		denseSphere.radius = 0.5f;
		NxBodyDesc denseBodies[4];
		NxActorDesc denseDescs[4];
		NxActor* denseActors[4] = { 0, 0, 0, 0 };
		for(unsigned denseIndex = 0; denseIndex != 4; ++denseIndex)
			{
			denseDescs[denseIndex].body = &denseBodies[denseIndex];
			denseDescs[denseIndex].density = 1.0f;
			denseDescs[denseIndex].globalPose.t = NxVec3(20.0f + denseIndex * 0.25f, 0.0f, 0.0f);
			denseDescs[denseIndex].shapes.pushBack(&denseSphere);
			denseActors[denseIndex] = denseScene->createActor(denseDescs[denseIndex]);
			if(!denseActors[denseIndex])
				return nxFail("dense broadphase actor creation failed");
			}
		denseScene->simulate(0.01f);
		if(!denseScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !denseScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("dense broadphase step failed");
		printf("simulation broadphase dense selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(denseScene));
		nxPrintBroadPhasePairOrder(denseScene, selector, denseActors, 4);
		sdk->releaseScene(*denseScene);
		NxBounds3 boundedBounds;
		boundedBounds.set(-5.0f, -5.0f, -5.0f, 5.0f, 5.0f, 5.0f);
		NxSceneDesc boundedDesc;
		boundedDesc.setToDefault();
		boundedDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
		boundedDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
		boundedDesc.maxBounds = &boundedBounds;
		NxScene* boundedScene = sdk->createScene(boundedDesc);
		if(!boundedScene)
			return nxFail("bounded broadphase scene creation failed");
		boundedScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
		NxSphereShapeDesc boundedSphere;
		boundedSphere.radius = 0.5f;
		NxBodyDesc boundedBodies[4];
		NxActorDesc boundedDescs[4];
		const NxReal boundedX[4] = { 0.0f, 0.25f, 10.0f, 10.25f };
		for(unsigned boundedIndex = 0; boundedIndex != 4; ++boundedIndex)
			{
			boundedDescs[boundedIndex].body = &boundedBodies[boundedIndex];
			boundedDescs[boundedIndex].density = 1.0f;
			boundedDescs[boundedIndex].globalPose.t = NxVec3(boundedX[boundedIndex], 0.0f, 0.0f);
			boundedDescs[boundedIndex].shapes.pushBack(&boundedSphere);
			if(!boundedScene->createActor(boundedDescs[boundedIndex]))
				return nxFail("bounded broadphase actor creation failed");
			}
		boundedScene->simulate(0.01f);
		if(!boundedScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !boundedScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("bounded broadphase step failed");
		printf("simulation broadphase bounded selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(boundedScene));
		sdk->releaseScene(*boundedScene);
		if(selector == NX_BROADPHASE_FULL)
			{
			NxSceneDesc planeDesc;
			planeDesc.setToDefault();
			planeDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
			planeDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
			NxScene* planeScene = sdk->createScene(planeDesc);
			if(!planeScene)
				return nxFail("full-pruner plane scene creation failed");
			planeScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
			NxPlaneShapeDesc planeShape;
			planeShape.normal = NxVec3(0.0f, 1.0f, 0.0f);
			NxActorDesc staticPlane;
			staticPlane.shapes.pushBack(&planeShape);
			NxSphereShapeDesc planeSphere;
			planeSphere.radius = 0.5f;
			NxBodyDesc planeBody;
			NxActorDesc dynamicSphere;
			dynamicSphere.body = &planeBody;
			dynamicSphere.density = 1.0f;
			dynamicSphere.globalPose.t = NxVec3(0.0f, 0.25f, 0.0f);
			dynamicSphere.shapes.pushBack(&planeSphere);
			if(!planeScene->createActor(staticPlane)
				|| !planeScene->createActor(dynamicSphere))
				return nxFail("full-pruner plane actors failed");
			planeScene->simulate(0.01f);
			if(!planeScene->checkResults(NX_RIGID_BODY_FINISHED, true)
				|| !planeScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
				return nxFail("full-pruner plane step failed");
			printf("simulation broadphase plane selector=%u pairs=%u\n",
				selector, nxReadSceneBroadPhasePairCount(planeScene));
			sdk->releaseScene(*planeScene);
			}
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
	const bool idleReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
	const bool idleFetched = emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false);
	const bool idleFence = emptyScene->wait(NX_FENCE_RUN_FINISHED, false);
	printf("simulation nonblocking=idle ready=%u fetched=%u\n",
		idleReady ? 1u : 0u, idleFetched ? 1u : 0u);
	printf("simulation fence=idle ready=%u\n", idleFence ? 1u : 0u);
	emptyScene->simulate(0.125f);
	const bool submittedFence = emptyScene->wait(NX_FENCE_RUN_FINISHED, true);
	const bool emptyReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool emptyReadyNonblocking = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
	const bool emptyFetchedNonblocking = emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false);
	printf("simulation empty-step ready=%u fetched=%u\n",
		emptyReady ? 1u : 0u, emptyFetchedNonblocking ? 1u : 0u);
	printf("simulation fence=submitted ready=%u\n", submittedFence ? 1u : 0u);
	printf("simulation nonblocking=finished ready=%u fetched=%u\n",
		emptyReadyNonblocking ? 1u : 0u, emptyFetchedNonblocking ? 1u : 0u);
	sdk->releaseScene(*emptyScene);

	// Exercise conventional force accumulation through the real scene step,
	// without contact or gravity contributing to the velocity change.
	NxSceneDesc forceSceneDesc;
	forceSceneDesc.setToDefault();
	forceSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* forceScene = sdk->createScene(forceSceneDesc);
	if(!forceScene)
		return nxFail("force scene creation failed");
	forceScene->setTiming(0.125f, 1, NX_TIMESTEP_VARIABLE);
	NxSphereShapeDesc forceSphere;
	forceSphere.radius = 0.5f;
	NxBodyDesc forceBody;
	NxActorDesc forceActorDesc;
	forceActorDesc.body = &forceBody;
	forceActorDesc.density = 1.0f;
	forceActorDesc.shapes.pushBack(&forceSphere);
	NxActor* forceActor = forceScene->createActor(forceActorDesc);
	if(!forceActor)
		return nxFail("force actor creation failed");
	forceActor->addForce(NxVec3(2.0f, 0.0f, 0.0f), NX_FORCE);
	forceScene->simulate(0.125f);
	const bool forceReady = forceScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool forceFetched = forceScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!forceReady || !forceFetched)
		return nxFail("force simulation result was not ready and fetched");
	nxPrintActorState("force1", *forceActor);
	sdk->releaseScene(*forceScene);

	// Exercise the kinematic target branch of BodyStep through the public actor
	// move and scene simulate APIs.
	NxSceneDesc kinematicSceneDesc;
	kinematicSceneDesc.setToDefault();
	kinematicSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* kinematicScene = sdk->createScene(kinematicSceneDesc);
	if(!kinematicScene)
		return nxFail("kinematic scene creation failed");
	kinematicScene->setTiming(0.125f, 1, NX_TIMESTEP_VARIABLE);
	NxBodyDesc kinematicBody;
	kinematicBody.flags |= NX_BF_KINEMATIC;
	NxActorDesc kinematicActorDesc;
	kinematicActorDesc.body = &kinematicBody;
	kinematicActorDesc.density = 1.0f;
	kinematicActorDesc.shapes.pushBack(&forceSphere);
	NxActor* kinematicActor = kinematicScene->createActor(kinematicActorDesc);
	if(!kinematicActor)
		return nxFail("kinematic actor creation failed");
	kinematicActor->moveGlobalPosition(NxVec3(1.0f, 0.0f, 0.5f));
	kinematicScene->simulate(0.125f);
	const bool kinematicReady = kinematicScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool kinematicFetched = kinematicScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!kinematicReady || !kinematicFetched)
		return nxFail("kinematic simulation result was not ready and fetched");
	nxPrintActorState("kinematic1", *kinematicActor);
	printf("simulation kinematic flag=%u\n",
		kinematicActor->readBodyFlag(NX_BF_KINEMATIC) ? 1u : 0u);
	sdk->releaseScene(*kinematicScene);

	// Exercise automatic wake-counter expiry and the public wake transition on
	// an isolated, stationary body.
	NxSceneDesc sleepSceneDesc;
	sleepSceneDesc.setToDefault();
	sleepSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* sleepScene = sdk->createScene(sleepSceneDesc);
	if(!sleepScene)
		return nxFail("sleep scene creation failed");
	sleepScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc sleepBody;
	NxActorDesc sleepActorDesc;
	sleepActorDesc.body = &sleepBody;
	sleepActorDesc.density = 1.0f;
	sleepActorDesc.shapes.pushBack(&forceSphere);
	NxActor* sleepActor = sleepScene->createActor(sleepActorDesc);
	if(!sleepActor)
		return nxFail("sleep actor creation failed");
	for(unsigned step = 0; step < 30; ++step)
		{
		sleepScene->simulate(0.02f);
		const bool ready = sleepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = sleepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("sleep simulation result was not ready and fetched");
		}
	printf("simulation sleep=automatic sleeping=%u\n", sleepActor->isSleeping() ? 1u : 0u);
	sleepActor->wakeUp(0.5f);
	printf("simulation sleep=woken sleeping=%u\n", sleepActor->isSleeping() ? 1u : 0u);
	sdk->releaseScene(*sleepScene);

	// Exercise a three-body contact island settling through the public solver
	// path, beyond the single impact pair.
	NxSceneDesc stackSceneDesc;
	stackSceneDesc.setToDefault();
	stackSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* stackScene = sdk->createScene(stackSceneDesc);
	if(!stackScene)
		return nxFail("stack scene creation failed");
	stackScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxPlaneShapeDesc stackPlane;
	NxActorDesc stackGroundDesc;
	stackGroundDesc.shapes.pushBack(&stackPlane);
	if(!stackScene->createActor(stackGroundDesc))
		return nxFail("stack ground creation failed");
	NxActor* stackActors[3] = {0};
	for(unsigned index = 0; index < 3; ++index)
		{
		NxBodyDesc stackBody;
		NxActorDesc stackActorDesc;
		stackActorDesc.body = &stackBody;
		stackActorDesc.density = 1.0f;
		stackActorDesc.globalPose.t = NxVec3(0.0f, 0.5f + float(index), 0.0f);
		stackActorDesc.shapes.pushBack(&forceSphere);
		stackActors[index] = stackScene->createActor(stackActorDesc);
		if(!stackActors[index])
			return nxFail("stack sphere creation failed");
		}
	for(unsigned step = 0; step < 120; ++step)
		{
		stackScene->simulate(0.02f);
		const bool ready = stackScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = stackScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("stack simulation result was not ready and fetched");
		for(unsigned index = 0; index < 3; ++index)
			{
			char stage[16];
			sprintf_s(stage, "stack%u_%u", step, index);
			nxPrintActorState(stage, *stackActors[index]);
			}
		}
	printf("simulation stack=settled steps=120 ready=1 fetched=1\n");
	for(unsigned index = 0; index < 3; ++index)
		{
		char stage[16];
		sprintf_s(stage, "stack%u", index);
		nxPrintActorState(stage, *stackActors[index]);
		}
	sdk->releaseScene(*stackScene);

	// Exercise fixed-joint angular and linear support rows through the island solver.
	NxSceneDesc fixedSceneDesc;
	fixedSceneDesc.setToDefault();
	fixedSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* fixedScene = sdk->createScene(fixedSceneDesc);
	if(!fixedScene)
		return nxFail("fixed-joint scene creation failed");
	fixedScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc fixedBody;
	NxActorDesc fixedActorDesc;
	fixedActorDesc.body = &fixedBody;
	fixedActorDesc.density = 1.0f;
	fixedActorDesc.globalPose.t = NxVec3(4.0f, 2.0f, 0.0f);
	fixedActorDesc.shapes.pushBack(&forceSphere);
	NxActor* fixedActor = fixedScene->createActor(fixedActorDesc);
	if(!fixedActor)
		return nxFail("fixed-joint actor creation failed");
	NxFixedJointDesc fixedDesc;
	fixedDesc.setToDefault();
	fixedDesc.actor[0] = fixedActor;
	fixedDesc.actor[1] = 0;
	setGlobalAnchor(fixedDesc, NxVec3(4.0f, 2.0f, 0.0f));
	setGlobalAxis(fixedDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* fixedJoint = fixedScene->createJoint(fixedDesc);
	if(!fixedJoint)
		return nxFail("fixed-joint creation failed");
	for(unsigned step = 0; step < 12; ++step)
		{
		fixedScene->simulate(0.02f);
		const bool ready = fixedScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = fixedScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("fixed-joint simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "fixed%u", step);
		nxPrintActorState(stage, *fixedActor);
		}
	printf("simulation fixed-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*fixedScene);

	// Keep the low-force fixed-joint break response in the registered
	// differential corpus; the historical four-step probe differed by six
	// output lines after the joint broke.
	NxSceneDesc breakSceneDesc;
	breakSceneDesc.setToDefault();
	breakSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* breakScene = sdk->createScene(breakSceneDesc);
	if(!breakScene)
		return nxFail("break-joint scene creation failed");
	breakScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc breakBody;
	NxActorDesc breakActorDesc;
	breakActorDesc.body = &breakBody;
	breakActorDesc.density = 1.0f;
	breakActorDesc.globalPose.t = NxVec3(6.0f, 2.0f, 0.0f);
	breakActorDesc.shapes.pushBack(&forceSphere);
	NxActor* breakActor = breakScene->createActor(breakActorDesc);
	if(!breakActor)
		return nxFail("break-joint actor creation failed");
	NxFixedJointDesc breakDesc;
	breakDesc.setToDefault();
	breakDesc.actor[0] = breakActor;
	breakDesc.actor[1] = 0;
	breakDesc.maxForce = 0.001f;
	setGlobalAnchor(breakDesc, NxVec3(6.0f, 2.0f, 0.0f));
	setGlobalAxis(breakDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* breakJoint = breakScene->createJoint(breakDesc);
	if(!breakJoint)
		return nxFail("break-joint creation failed");
	for(unsigned step = 0; step != 4; ++step)
		{
		breakScene->simulate(0.02f);
		const bool ready = breakScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = breakScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("break-joint simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "break%u", step);
		nxPrintActorState(stage, *breakActor);
		printf("simulation break-state step=%u state=%u\n", step, (unsigned)breakJoint->getState());
		}
	printf("simulation break-joint steps=4 ready=1 fetched=1\n");
	sdk->releaseScene(*breakScene);

	// Retain the revolute pendulum differential that exposed a small but
	// repeatable linear-velocity residual after the second solver step.
	NxSceneDesc revoluteSceneDesc;
	revoluteSceneDesc.setToDefault();
	revoluteSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* revoluteScene = sdk->createScene(revoluteSceneDesc);
	if(!revoluteScene)
		return nxFail("revolute-joint scene creation failed");
	revoluteScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc revoluteBody;
	NxActorDesc revoluteActorDesc;
	revoluteActorDesc.body = &revoluteBody;
	revoluteActorDesc.density = 1.0f;
	revoluteActorDesc.globalPose.t = NxVec3(7.0f, 2.0f, 0.0f);
	revoluteActorDesc.shapes.pushBack(&forceSphere);
	NxActor* revoluteActor = revoluteScene->createActor(revoluteActorDesc);
	if(!revoluteActor)
		return nxFail("revolute-joint actor creation failed");
	NxRevoluteJointDesc revoluteDesc;
	revoluteDesc.setToDefault();
	revoluteDesc.actor[0] = revoluteActor;
	revoluteDesc.actor[1] = 0;
	setGlobalAnchor(revoluteDesc, NxVec3(7.0f, 2.0f, 0.0f));
	setGlobalAxis(revoluteDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* revoluteJoint = revoluteScene->createJoint(revoluteDesc);
	if(!revoluteJoint)
		return nxFail("revolute-joint creation failed");
	for(unsigned step = 0; step < 12; ++step)
		{
		revoluteScene->simulate(0.02f);
		const bool ready = revoluteScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = revoluteScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("revolute-joint simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "revolute%u", step);
		nxPrintActorState(stage, *revoluteActor);
		}
	printf("simulation revolute-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*revoluteScene);


	// Regression coverage for kind-1 distance-row settling across repeated steps.
	NxSceneDesc distanceSceneDesc;
	distanceSceneDesc.setToDefault();
	distanceSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* distanceScene = sdk->createScene(distanceSceneDesc);
	if(!distanceScene) return nxFail("distance-joint scene creation failed");
	distanceScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc distanceBody;
	NxActorDesc distanceActorDesc;
	distanceActorDesc.body = &distanceBody;
	distanceActorDesc.density = 1.0f;
	distanceActorDesc.globalPose.t = NxVec3(4.0f, 2.0f, 0.0f);
	distanceActorDesc.shapes.pushBack(&forceSphere);
	NxActor* distanceActor = distanceScene->createActor(distanceActorDesc);
	if(!distanceActor) return nxFail("distance-joint actor creation failed");
	NxDistanceJointDesc distanceDesc;
	distanceDesc.setToDefault(false);
	distanceDesc.actor[0] = distanceActor;
	distanceDesc.actor[1] = 0;
	setGlobalAnchor(distanceDesc, NxVec3(4.0f, 2.0f, 0.0f));
	distanceDesc.minDistance = 0.0f;
	distanceDesc.maxDistance = 0.0f;
	distanceDesc.flags = NX_DJF_MIN_DISTANCE_ENABLED | NX_DJF_MAX_DISTANCE_ENABLED;
	NxJoint* distanceJoint = distanceScene->createJoint(distanceDesc);
	if(!distanceJoint) return nxFail("distance-joint creation failed");
	for(unsigned step = 0; step < 12; ++step)
		{
		distanceScene->simulate(0.02f);
		const bool ready = distanceScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = distanceScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched) return nxFail("distance-joint result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "distance%u", step);
		nxPrintActorState(stage, *distanceActor);
		}
	printf("simulation distance-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*distanceScene);

	NxPlaneShapeDesc groundPlane;
	groundPlane.normal = NxVec3(0.0f, 1.0f, 0.0f);
	groundPlane.d = 0.0f;
	NxActorDesc groundDesc;
	groundDesc.shapes.pushBack(&groundPlane);
	NxActor* ground = scene->createActor(groundDesc);
	if(!ground)
		return nxFail("static ground plane creation failed");
	printf("simulation ground=created\n");

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

	// Exercise the kind-4 friction rows against explicit, nonzero material
	// coefficients while a sphere lands and slides on a static plane.
	NxMaterial frictionMaterial;
	frictionMaterial.staticFriction = 0.8f;
	frictionMaterial.dynamicFriction = 0.8f;
	const NxMaterialIndex frictionMaterialIndex = sdk->addMaterial(frictionMaterial);
	NxSceneDesc frictionSceneDesc;
	frictionSceneDesc.setToDefault();
	frictionSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	frictionSceneDesc.maxTimestep = 0.125f;
	frictionSceneDesc.maxIter = 1;
	NxScene* frictionScene = sdk->createScene(frictionSceneDesc);
	if(!frictionScene)
		return nxFail("friction scene creation failed");
	NxPlaneShapeDesc frictionPlane;
	frictionPlane.materialIndex = frictionMaterialIndex;
	NxActorDesc frictionGroundDesc;
	frictionGroundDesc.shapes.pushBack(&frictionPlane);
	if(!frictionScene->createActor(frictionGroundDesc))
		return nxFail("friction ground creation failed");
	NxSphereShapeDesc frictionSphere;
	frictionSphere.radius = 0.5f;
	frictionSphere.materialIndex = frictionMaterialIndex;
	NxBodyDesc sliderBody;
	NxActorDesc sliderDesc;
	sliderDesc.body = &sliderBody;
	sliderDesc.density = 1.0f;
	sliderDesc.globalPose.t = NxVec3(0.0f, 0.5f, 0.0f);
	sliderDesc.shapes.pushBack(&frictionSphere);
	NxActor* slider = frictionScene->createActor(sliderDesc);
	if(!slider)
		return nxFail("friction slider creation failed");
	slider->setLinearVelocity(NxVec3(2.0f, 0.0f, 0.0f));
	printf("simulation friction=created\n");
	for(unsigned step = 0; step < 8; ++step)
		{
		frictionScene->simulate(0.125f);
		const bool ready = frictionScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = frictionScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("friction simulation result was not ready and fetched");
		char stage[16];
		sprintf_s(stage, "friction%u", step);
		nxPrintActorState(stage, *slider);
		}
	sdk->releaseScene(*frictionScene);

	scene->startRun(0.01f);
	scene->finishRun();
	printf("simulation legacy=start-finish returned\n");
	scene->runFor(0.01f, 0.1f, 1, NX_TIMESTEP_VARIABLE);
	printf("simulation legacy=runFor returned\n");

	// Exercise the contact solver with two dynamic bodies and nonzero tangential
	// velocity. This reaches both the coupled impulse rows and friction path,
	// beyond the single-body static-ground settling case above.
	NxSceneDesc pairSceneDesc;
	pairSceneDesc.setToDefault();
	pairSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* pairScene = sdk->createScene(pairSceneDesc);
	if(!pairScene)
		return nxFail("dynamic-pair scene creation failed");
	pairScene->setTiming(0.025f, 8, NX_TIMESTEP_VARIABLE);
	NxSphereShapeDesc pairShape;
	pairShape.radius = 0.5f;
	NxBodyDesc movingBody;
	NxActorDesc movingDesc;
	movingDesc.body = &movingBody;
	movingDesc.density = 1.0f;
	movingDesc.globalPose.t = NxVec3(-2.0f, 0.0f, 0.0f);
	movingDesc.shapes.pushBack(&pairShape);
	NxActor* movingActor = pairScene->createActor(movingDesc);
	NxBodyDesc targetBody;
	NxActorDesc targetDesc;
	targetDesc.body = &targetBody;
	targetDesc.density = 1.0f;
	targetDesc.globalPose.t = NxVec3(0.0f, 0.0f, 0.0f);
	targetDesc.shapes.pushBack(&pairShape);
	NxActor* targetActor = pairScene->createActor(targetDesc);
	if(!movingActor || !targetActor)
		return nxFail("dynamic-pair actor creation failed");
	movingActor->setLinearVelocity(NxVec3(4.0f, 0.0f, 1.0f));
	printf("simulation pair=created\n");
	for(unsigned step = 0; step < 40; ++step)
		{
		pairScene->simulate(0.025f);
		const bool ready = pairScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = pairScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("dynamic-pair simulation result was not ready and fetched");
		if(step >= 8 && step <= 22)
			{
			char stage[16];
			NxActor& actor0 = *movingActor;
			NxActor& actor1 = *targetActor;
			NxVec3 velocity0;
			NxVec3 velocity1;
			actor0.getLinearVelocity(velocity0);
			actor1.getLinearVelocity(velocity1);
			const NxVec3& position0 = actor0.getGlobalPosition();
			const NxVec3& position1 = actor1.getGlobalPosition();
			sprintf_s(stage, "pair%u", step);
			printf("simulation stage=%s p0=%08x.%08x.%08x v0=%08x.%08x.%08x "
				"p1=%08x.%08x.%08x v1=%08x.%08x.%08x\n", stage,
				nxFloatBits(position0.x), nxFloatBits(position0.y), nxFloatBits(position0.z),
				nxFloatBits(velocity0.x), nxFloatBits(velocity0.y), nxFloatBits(velocity0.z),
				nxFloatBits(position1.x), nxFloatBits(position1.y), nxFloatBits(position1.z),
				nxFloatBits(velocity1.x), nxFloatBits(velocity1.y), nxFloatBits(velocity1.z));
			}
		}
	printf("simulation pair steps=40 ready=1 fetched=1\n");
	sdk->releaseScene(*pairScene);

	// Exercise contact-report generation through the public pair-flag API and
	// the real overlap/contact path (unlike the queue-injection dispatch fixture
	// below). The first frame must report the new touch at fetchResults.
	NxSceneDesc reportSceneDesc;
	reportSceneDesc.setToDefault();
	reportSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* reportScene = sdk->createScene(reportSceneDesc);
	if(!reportScene)
		return nxFail("contact-report scene creation failed");
	NxPlaneShapeDesc reportPlane;
	NxActorDesc reportGroundDesc;
	reportGroundDesc.shapes.pushBack(&reportPlane);
	NxActor* reportGround = reportScene->createActor(reportGroundDesc);
	NxSphereShapeDesc reportSphere;
	reportSphere.radius = 0.5f;
	NxBodyDesc reportBody;
	NxActorDesc reportDynamicDesc;
	reportDynamicDesc.body = &reportBody;
	reportDynamicDesc.density = 1.0f;
	reportDynamicDesc.globalPose.t = NxVec3(0.0f, 0.5f, 0.0f);
	reportDynamicDesc.shapes.pushBack(&reportSphere);
	NxActor* reportDynamic = reportScene->createActor(reportDynamicDesc);
	if(!reportGround || !reportDynamic)
		return nxFail("contact-report actors creation failed");
	NxSimulationContactReport generatedContactReport(reportGround, reportDynamic);
	reportScene->setUserContactReport(&generatedContactReport);
	reportGround->setGroup(7);
	reportDynamic->setGroup(3);
	sdk->setActorGroupPairFlags(7, 3, NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	const NxU32 reportFlags = reportScene->getActorPairFlags(*reportGround, *reportDynamic);
	reportScene->simulate(0.125f);
	const bool reportReady = reportScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool reportFetched = reportScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!reportReady || !reportFetched)
		return nxFail("contact-report simulation result was not ready and fetched");
	printf("simulation generated-contact summary flags=%08x calls=%u events=%08x ready=%u fetched=%u\n",
		reportFlags, generatedContactReport.calls, generatedContactReport.events,
		reportReady, reportFetched);
	sdk->setActorGroupPairFlags(7, 3, 0);
	sdk->releaseScene(*reportScene);

	// Exercise report timing for a dynamic compound that makes contact through
	// two independently pruned sphere children.
	NxSceneDesc compoundReportSceneDesc;
	compoundReportSceneDesc.setToDefault();
	compoundReportSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* compoundReportScene = sdk->createScene(compoundReportSceneDesc);
	if(!compoundReportScene)
		return nxFail("compound contact-report scene creation failed");
	compoundReportScene->setTiming(1.0f / 60.0f, 8, NX_TIMESTEP_FIXED);
	NxActorDesc compoundGroundDesc;
	compoundGroundDesc.shapes.pushBack(&reportPlane);
	NxActor* compoundGround = compoundReportScene->createActor(compoundGroundDesc);
	NxSphereShapeDesc compoundSphere0;
	compoundSphere0.radius = 0.5f;
	compoundSphere0.localPose.t = NxVec3(-0.25f, 0.0f, 0.0f);
	NxSphereShapeDesc compoundSphere1;
	compoundSphere1.radius = 0.5f;
	compoundSphere1.localPose.t = NxVec3(0.25f, 0.0f, 0.0f);
	NxBodyDesc compoundReportBody;
	NxActorDesc compoundReportActorDesc;
	compoundReportActorDesc.body = &compoundReportBody;
	compoundReportActorDesc.density = 1.0f;
	// An elevated, rotated compound reaches an off-center contact and exercises
	// the child-pose refresh and angular solver state in the differential.
	compoundReportActorDesc.globalPose.t = NxVec3(0.0f, 0.7f, 0.0f);
	const NxReal compoundStartRotation[9] = {
		0.8660254f, -0.5f, 0.0f, 0.5f, 0.8660254f, 0.0f, 0.0f, 0.0f, 1.0f };
	compoundReportActorDesc.globalPose.M.setRowMajor(compoundStartRotation);
	compoundReportActorDesc.shapes.pushBack(&compoundSphere0);
	compoundReportActorDesc.shapes.pushBack(&compoundSphere1);
	NxActor* compoundReportActor = compoundReportScene->createActor(compoundReportActorDesc);
	if(!compoundGround || !compoundReportActor)
		return nxFail("compound contact-report actors creation failed");
	unsigned char* const compoundBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(compoundReportActor) + 0x14);
	unsigned char* const compoundRoot = *reinterpret_cast<unsigned char**>(compoundBody + 0x10);
	const NxVec3 actorStart = compoundReportActor->getGlobalPositionVal();
	printf("simulation compound-root-pose actor=%08x.%08x.%08x root=%08x.%08x.%08x\n",
		nxFloatBits(actorStart.x), nxFloatBits(actorStart.y), nxFloatBits(actorStart.z),
		nxFloatBits(*reinterpret_cast<float*>(compoundRoot + 0x30)),
		nxFloatBits(*reinterpret_cast<float*>(compoundRoot + 0x34)),
		nxFloatBits(*reinterpret_cast<float*>(compoundRoot + 0x38)));
	NxSimulationContactReport compoundReport(compoundGround, compoundReportActor);
	compoundReportScene->setUserContactReport(&compoundReport);
	compoundGround->setGroup(7);
	compoundReportActor->setGroup(3);
	sdk->setActorGroupPairFlags(7, 3, NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	for(unsigned step = 0; step < 16; ++step)
		{
		compoundReportScene->simulate(1.0f / 60.0f);
		const bool ready = compoundReportScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = compoundReportScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("compound contact-report result was not ready and fetched");
		const NxVec3 compoundPosition = compoundReportActor->getGlobalPositionVal();
		const NxVec3 compoundVelocity = compoundReportActor->getLinearVelocityVal();
		const NxVec3 compoundAngularVelocity = compoundReportActor->getAngularVelocityVal();
		const NxQuat compoundOrientation = compoundReportActor->getGlobalOrientationQuatVal();
		printf("simulation compound-state step=%u position=%08x.%08x.%08x velocity=%08x.%08x.%08x angular=%08x.%08x.%08x orientation=%08x.%08x.%08x.%08x\n",
			step, nxFloatBits(compoundPosition.x), nxFloatBits(compoundPosition.y),
			nxFloatBits(compoundPosition.z), nxFloatBits(compoundVelocity.x),
			nxFloatBits(compoundVelocity.y), nxFloatBits(compoundVelocity.z),
			nxFloatBits(compoundAngularVelocity.x), nxFloatBits(compoundAngularVelocity.y),
			nxFloatBits(compoundAngularVelocity.z), nxFloatBits(compoundOrientation.x),
			nxFloatBits(compoundOrientation.y), nxFloatBits(compoundOrientation.z),
			nxFloatBits(compoundOrientation.w));
		printf("simulation compound-generated-contact step=%u calls=%u events=%08x\n",
			step, compoundReport.calls, compoundReport.events);
		}
	printf("simulation compound-generated-contact summary calls=%u events=%08x\n",
		compoundReport.calls, compoundReport.events);
	sdk->setActorGroupPairFlags(7, 3, 0);
	sdk->releaseScene(*compoundReportScene);

	// Seed the oracle-shaped fetch callback queues after a completed empty step.
	// This isolates the 000640 callback dispatch/list-reset contract from pair
	// generation, which is covered by the collision/contact reconstruction.
	NxSceneDesc callbackSceneDesc;
	callbackSceneDesc.setToDefault();
	NxScene* callbackScene = sdk->createScene(callbackSceneDesc);
	if(!callbackScene)
		return nxFail("fetch-callback scene creation failed");
	unsigned char callbackWrapper[0x28];
	memcpy(callbackWrapper, callbackScene, sizeof(callbackWrapper));
	unsigned char* const callbackInternal = *reinterpret_cast<unsigned char**>(callbackWrapper + 0x24);
	unsigned char internalShapes[2][0xa0];
	unsigned char publicShapeTokens[2];
	memset(internalShapes, 0, sizeof(internalShapes));
	memset(publicShapeTokens, 0, sizeof(publicShapeTokens));
	NxShape* const expectedTrigger = reinterpret_cast<NxShape*>(&publicShapeTokens[0]);
	NxShape* const expectedOther = reinterpret_cast<NxShape*>(&publicShapeTokens[1]);
	*reinterpret_cast<NxShape**>(internalShapes[0] + 0x9c) = expectedTrigger;
	*reinterpret_cast<NxShape**>(internalShapes[1] + 0x9c) = expectedOther;
	NxSimulationTriggerReport triggerReport(expectedTrigger, expectedOther);
	NxSimulationContactReport contactReport(reinterpret_cast<NxActor*>(&publicShapeTokens[0]),
		reinterpret_cast<NxActor*>(&publicShapeTokens[1]));
	callbackScene->setUserTriggerReport(&triggerReport);
	callbackScene->setUserContactReport(&contactReport);
	const bool triggerGetter = callbackScene->getUserTriggerReport() == &triggerReport;
	const bool contactGetter = callbackScene->getUserContactReport() == &contactReport;
	unsigned char triggerQueue[0x0c];
	*reinterpret_cast<void**>(triggerQueue + 0x00) = internalShapes[0];
	*reinterpret_cast<void**>(triggerQueue + 0x04) = internalShapes[1];
	*reinterpret_cast<NxU32*>(triggerQueue + 0x08) = NX_TRIGGER_ON_ENTER;
	unsigned char contactQueue[0x2c];
	memset(contactQueue, 0, sizeof(contactQueue));
	NxContactPair* const contactPair = reinterpret_cast<NxContactPair*>(contactQueue);
	contactPair->actors[0] = contactReport.expectedActor0;
	contactPair->actors[1] = contactReport.expectedActor1;
	contactPair->sumNormalForce = NxVec3(1.0f, 2.0f, 3.0f);
	*reinterpret_cast<NxU32*>(contactQueue + 0x28) = NX_NOTIFY_ON_TOUCH;
	void** const triggerBeginField = reinterpret_cast<void**>(callbackInternal + 0x5fc);
	void** const triggerEndField = reinterpret_cast<void**>(callbackInternal + 0x600);
	void** const triggerCapacityField = reinterpret_cast<void**>(callbackInternal + 0x604);
	void** const contactBeginField = reinterpret_cast<void**>(callbackInternal + 0x60c);
	void** const contactEndField = reinterpret_cast<void**>(callbackInternal + 0x610);
	void** const contactCapacityField = reinterpret_cast<void**>(callbackInternal + 0x614);
	void* const savedTriggerBegin = *triggerBeginField;
	void* const savedTriggerEnd = *triggerEndField;
	void* const savedTriggerCapacity = *triggerCapacityField;
	void* const savedContactBegin = *contactBeginField;
	void* const savedContactEnd = *contactEndField;
	void* const savedContactCapacity = *contactCapacityField;
	*triggerBeginField = triggerQueue;
	*triggerEndField = triggerQueue + sizeof(triggerQueue);
	*triggerCapacityField = triggerQueue + sizeof(triggerQueue);
	*contactBeginField = contactQueue;
	*contactEndField = contactQueue + sizeof(contactQueue);
	*contactCapacityField = contactQueue + sizeof(contactQueue);
	callbackScene->simulate(0.01f);
	const bool callbackReady = callbackScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool callbackFetched = callbackScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	*triggerBeginField = savedTriggerBegin;
	*triggerEndField = savedTriggerEnd;
	*triggerCapacityField = savedTriggerCapacity;
	*contactBeginField = savedContactBegin;
	*contactEndField = savedContactEnd;
	*contactCapacityField = savedContactCapacity;
	if(!callbackReady || !callbackFetched)
		return nxFail("fetch-callback simulation result was not ready and fetched");
	printf("simulation fetch-callback summary ready=%u fetched=%u trigger_get=%u trigger_calls=%u contact_get=%u contact_calls=%u\n",
		callbackReady, callbackFetched, triggerGetter, triggerReport.calls,
		contactGetter, contactReport.calls);
	sdk->releaseScene(*callbackScene);

	sdk->releaseScene(*scene);
	sdk->release();
	for(unsigned sdkCycle = 0; sdkCycle != 2; ++sdkCycle)
		{
		NxPhysicsSDK* const cycleSdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
		if(!cycleSdk)
			return nxFail("repeated lifecycle SDK creation failed");
		for(unsigned sceneCycle = 0; sceneCycle != 2; ++sceneCycle)
			{
			NxSceneDesc cycleSceneDesc;
			cycleSceneDesc.setToDefault();
			cycleSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
			NxScene* const cycleScene = cycleSdk->createScene(cycleSceneDesc);
			if(!cycleScene)
				return nxFail("repeated lifecycle scene creation failed");
			cycleScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
			NxSphereShapeDesc cycleSphere;
			cycleSphere.radius = 0.25f;
			NxBodyDesc cycleBody;
			NxActorDesc cycleActorDesc;
			cycleActorDesc.body = &cycleBody;
			cycleActorDesc.density = 1.0f;
			cycleActorDesc.globalPose.t = NxVec3((NxReal)(sdkCycle * 4 + sceneCycle), 1.0f, 0.0f);
			cycleActorDesc.shapes.pushBack(&cycleSphere);
			NxActor* const cycleActor = cycleScene->createActor(cycleActorDesc);
			if(!cycleActor)
				return nxFail("repeated lifecycle actor creation failed");
			for(unsigned step = 0; step != 4; ++step)
				{
				cycleScene->simulate(0.02f);
				const bool ready = cycleScene->checkResults(NX_RIGID_BODY_FINISHED, true);
				const bool fetched = cycleScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
				if(!ready || !fetched)
					return nxFail("repeated lifecycle simulation result was not ready and fetched");
				}
			const NxVec3& position = cycleActor->getGlobalPosition();
			NxVec3 velocity;
			cycleActor->getLinearVelocity(velocity);
			printf("simulation lifecycle state sdk=%u scene=%u p=%08x.%08x.%08x v=%08x.%08x.%08x\n",
				sdkCycle, sceneCycle,
				nxFloatBits(position.x), nxFloatBits(position.y), nxFloatBits(position.z),
				nxFloatBits(velocity.x), nxFloatBits(velocity.y), nxFloatBits(velocity.z));
			cycleSdk->releaseScene(*cycleScene);
			printf("simulation lifecycle scene_released sdk=%u scene=%u\n", sdkCycle, sceneCycle);
			}
		cycleSdk->release();
		printf("simulation lifecycle sdk_released cycle=%u\n", sdkCycle);
		}
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
