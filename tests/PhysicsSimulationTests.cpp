#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxBitField.h"
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
#include "NxPointInPlaneJointDesc.h"
#include "NxD6JointDesc.h"
#include "NxJoint.h"
#include "NxUserNotify.h"
#include "NxMaterial.h"
#include "NxBounds3.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);
typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);

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

static void nxPrintActorRotationState(const char* stage, NxActor& actor)
	{
	const NxQuat orientation = actor.getGlobalOrientationQuat();
	NxVec3 angularVelocity;
	actor.getAngularVelocity(angularVelocity);
	printf("simulation rotation-state=%s orientation=%08x.%08x.%08x.%08x angular=%08x.%08x.%08x\n",
		stage, nxFloatBits(orientation.x), nxFloatBits(orientation.y), nxFloatBits(orientation.z), nxFloatBits(orientation.w),
		nxFloatBits(angularVelocity.x), nxFloatBits(angularVelocity.y), nxFloatBits(angularVelocity.z));
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

class NxBreakReleaseNotify : public NxUserNotify
	{
	public:
	NxBreakReleaseNotify(bool releaseJoint)
		: calls(0), jointMatched(false), breakingForceBits(0), releaseJoint(releaseJoint) {}
	virtual bool onJointBreak(NxReal breakingForce, NxJoint& brokenJoint)
		{
		++calls;
		jointMatched = (&brokenJoint == expectedJoint);
		breakingForceBits = nxFloatBits(breakingForce);
		return releaseJoint;
		}
	NxJoint* expectedJoint;
	unsigned calls;
	bool jointMatched;
	unsigned breakingForceBits;
	bool releaseJoint;
	};

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
	printf("simulation nonblocking=idle ready=%u fetched=%u\n",
		idleReady ? 1u : 0u, idleFetched ? 1u : 0u);
	emptyScene->simulate(0.125f);
	const bool emptyReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool emptyReadyNonblocking = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
	const bool emptyFetchedNonblocking = emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false);
	printf("simulation empty-step ready=%u fetched=%u\n",
		emptyReady ? 1u : 0u, emptyFetchedNonblocking ? 1u : 0u);
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

	// Returning true from onJointBreak transfers release responsibility to the
	// scene. Observe the callback while the joint is valid, then only inspect
	// scene state after fetchResults has completed the automatic release.
	NxSceneDesc notifyBreakSceneDesc;
	notifyBreakSceneDesc.setToDefault();
	notifyBreakSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* notifyBreakScene = sdk->createScene(notifyBreakSceneDesc);
	if(!notifyBreakScene)
		return nxFail("notify break scene creation failed");
	notifyBreakScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc notifyBreakBody;
	NxActorDesc notifyBreakActorDesc;
	notifyBreakActorDesc.body = &notifyBreakBody;
	notifyBreakActorDesc.density = 1.0f;
	notifyBreakActorDesc.globalPose.t = NxVec3(6.0f, 2.0f, 0.0f);
	notifyBreakActorDesc.shapes.pushBack(&forceSphere);
	NxActor* notifyBreakActor = notifyBreakScene->createActor(notifyBreakActorDesc);
	if(!notifyBreakActor)
		return nxFail("notify break actor creation failed");
	NxFixedJointDesc notifyBreakDesc;
	notifyBreakDesc.setToDefault();
	notifyBreakDesc.actor[0] = notifyBreakActor;
	notifyBreakDesc.actor[1] = 0;
	notifyBreakDesc.maxForce = 0.001f;
	setGlobalAnchor(notifyBreakDesc, NxVec3(6.0f, 2.0f, 0.0f));
	setGlobalAxis(notifyBreakDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* notifyBreakJoint = notifyBreakScene->createJoint(notifyBreakDesc);
	if(!notifyBreakJoint)
		return nxFail("notify break joint creation failed");
	NxBreakReleaseNotify breakNotify(true);
	breakNotify.expectedJoint = notifyBreakJoint;
	notifyBreakScene->setUserNotify(&breakNotify);
	printf("simulation break-notify retrieved=%u\n",
		notifyBreakScene->getUserNotify() == &breakNotify);
	for(unsigned step = 0; step != 4; ++step)
		{
		notifyBreakScene->simulate(0.02f);
		const bool ready = notifyBreakScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = notifyBreakScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("notify break simulation result was not ready and fetched");
		char stage[32];
		sprintf_s(stage, "notifyBreak%u", step);
		nxPrintActorState(stage, *notifyBreakActor);
		printf("simulation break-notify step=%u calls=%u matched=%u force=%08x joints=%u\n",
			step, breakNotify.calls, breakNotify.jointMatched,
			breakNotify.breakingForceBits, notifyBreakScene->getNbJoints());
		}
	sdk->releaseScene(*notifyBreakScene);

	// Returning false reports the break but leaves the broken joint available
	// for the application to inspect and release explicitly.
	NxSceneDesc keepBreakSceneDesc;
	keepBreakSceneDesc.setToDefault();
	keepBreakSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* keepBreakScene = sdk->createScene(keepBreakSceneDesc);
	if(!keepBreakScene)
		return nxFail("keep break scene creation failed");
	keepBreakScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc keepBreakBody;
	NxActorDesc keepBreakActorDesc;
	keepBreakActorDesc.body = &keepBreakBody;
	keepBreakActorDesc.density = 1.0f;
	keepBreakActorDesc.globalPose.t = NxVec3(6.0f, 2.0f, 0.0f);
	keepBreakActorDesc.shapes.pushBack(&forceSphere);
	NxActor* keepBreakActor = keepBreakScene->createActor(keepBreakActorDesc);
	if(!keepBreakActor)
		return nxFail("keep break actor creation failed");
	NxFixedJointDesc keepBreakDesc;
	keepBreakDesc.setToDefault();
	keepBreakDesc.actor[0] = keepBreakActor;
	keepBreakDesc.actor[1] = 0;
	keepBreakDesc.maxForce = 0.001f;
	setGlobalAnchor(keepBreakDesc, NxVec3(6.0f, 2.0f, 0.0f));
	setGlobalAxis(keepBreakDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* keepBreakJoint = keepBreakScene->createJoint(keepBreakDesc);
	if(!keepBreakJoint)
		return nxFail("keep break joint creation failed");
	NxBreakReleaseNotify keepBreakNotify(false);
	keepBreakNotify.expectedJoint = keepBreakJoint;
	keepBreakScene->setUserNotify(&keepBreakNotify);
	for(unsigned step = 0; step != 4; ++step)
		{
		keepBreakScene->simulate(0.02f);
		const bool ready = keepBreakScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = keepBreakScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("keep break simulation result was not ready and fetched");
		char stage[32];
		sprintf_s(stage, "keepBreak%u", step);
		nxPrintActorState(stage, *keepBreakActor);
		printf("simulation break-notify-false step=%u calls=%u matched=%u force=%08x joints=%u state=%u\n",
			step, keepBreakNotify.calls, keepBreakNotify.jointMatched,
			keepBreakNotify.breakingForceBits, keepBreakScene->getNbJoints(),
			(unsigned)keepBreakJoint->getState());
		}
	keepBreakScene->releaseJoint(*keepBreakJoint);
	sdk->releaseScene(*keepBreakScene);

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

	// Exercise the kind-2 joint-support dispatch with a point constrained to a
	// plane under gravity; this solver route previously had no public-path case.
	NxSceneDesc planeJointSceneDesc;
	planeJointSceneDesc.setToDefault();
	planeJointSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* planeJointScene = sdk->createScene(planeJointSceneDesc);
	if(!planeJointScene)
		return nxFail("point-in-plane scene creation failed");
	planeJointScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc planeJointBody;
	NxActorDesc planeJointActorDesc;
	planeJointActorDesc.body = &planeJointBody;
	planeJointActorDesc.density = 1.0f;
	planeJointActorDesc.globalPose.t = NxVec3(9.0f, 2.0f, 0.0f);
	planeJointActorDesc.shapes.pushBack(&forceSphere);
	NxActor* planeJointActor = planeJointScene->createActor(planeJointActorDesc);
	if(!planeJointActor)
		return nxFail("point-in-plane actor creation failed");
	NxPointInPlaneJointDesc planeJointDesc;
	planeJointDesc.setToDefault();
	planeJointDesc.actor[0] = planeJointActor;
	planeJointDesc.actor[1] = 0;
	setGlobalAnchor(planeJointDesc, NxVec3(9.0f, 2.0f, 0.0f));
	setGlobalAxis(planeJointDesc, NxVec3(0.0f, 1.0f, 0.0f));
	NxJoint* planeJoint = planeJointScene->createJoint(planeJointDesc);
	if(!planeJoint)
		return nxFail("point-in-plane joint creation failed");
	for(unsigned step = 0; step != 12; ++step)
		{
		planeJointScene->simulate(0.02f);
		const bool ready = planeJointScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = planeJointScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("point-in-plane simulation result was not ready and fetched");
		char stage[32];
		sprintf_s(stage, "pointInPlane%u", step);
		nxPrintActorState(stage, *planeJointActor);
		}
	printf("simulation point-in-plane-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*planeJointScene);

	// Exercise all three D6 linear constraints against an identity-oriented
	// world frame under repeated steps. Rotated-frame multi-axis limits remain
	// a separate unresolved differential case.
	NxSceneDesc d6SceneDesc;
	d6SceneDesc.setToDefault();
	d6SceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* d6Scene = sdk->createScene(d6SceneDesc);
	if(!d6Scene)
		return nxFail("D6 scene creation failed");
	d6Scene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc d6Body;
	NxActorDesc d6ActorDesc;
	d6ActorDesc.body = &d6Body;
	d6ActorDesc.density = 1.0f;
	d6ActorDesc.globalPose.t = NxVec3(0.0f, 2.0f, 0.0f);
	d6ActorDesc.shapes.pushBack(&forceSphere);
	NxActor* d6Actor = d6Scene->createActor(d6ActorDesc);
	if(!d6Actor)
		return nxFail("D6 actor creation failed");
	NxD6JointDesc d6Desc;
	d6Desc.setToDefault();
	d6Desc.actor[0] = d6Actor;
	d6Desc.actor[1] = 0;
	d6Desc.xMotion = NX_D6JOINT_MOTION_LOCKED;
	d6Desc.yMotion = NX_D6JOINT_MOTION_LOCKED;
	d6Desc.zMotion = NX_D6JOINT_MOTION_LOCKED;
	d6Desc.projectionDistance = 0.0f;
	d6Desc.projectionAngle = 0.0f;
	d6Desc.projectionMode = NX_JPM_NONE;
	setGlobalAnchor(d6Desc, NxVec3(0.0f, 2.0f, 0.0f));
	NxJoint* d6Joint = d6Scene->createJoint(d6Desc);
	if(!d6Joint)
		return nxFail("D6 joint creation failed");
	for(unsigned step = 0; step != 12; ++step)
		{
		d6Scene->simulate(0.02f);
		const bool ready = d6Scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = d6Scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("D6 simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "d6Locked%u", step);
		nxPrintActorState(stage, *d6Actor);
		}
	printf("simulation d6-locked-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*d6Scene);


	// Characterize the same locked translation with a rotated D6 world frame.
	// This catches frame-basis errors hidden by the identity-oriented fixture.
	NxSceneDesc d6RotatedSceneDesc;
	d6RotatedSceneDesc.setToDefault();
	d6RotatedSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* d6RotatedScene = sdk->createScene(d6RotatedSceneDesc);
	if(!d6RotatedScene)
		return nxFail("rotated D6 scene creation failed");
	d6RotatedScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc d6RotatedBody;
	NxActorDesc d6RotatedActorDesc;
	d6RotatedActorDesc.body = &d6RotatedBody;
	d6RotatedActorDesc.density = 1.0f;
	d6RotatedActorDesc.globalPose.t = NxVec3(3.0f, 2.0f, 0.0f);
	d6RotatedActorDesc.shapes.pushBack(&forceSphere);
	NxActor* d6RotatedActor = d6RotatedScene->createActor(d6RotatedActorDesc);
	if(!d6RotatedActor)
		return nxFail("rotated D6 actor creation failed");
	NxD6JointDesc d6RotatedDesc;
	d6RotatedDesc.setToDefault();
	d6RotatedDesc.actor[0] = d6RotatedActor;
	d6RotatedDesc.actor[1] = 0;
	d6RotatedDesc.xMotion = NX_D6JOINT_MOTION_LOCKED;
	d6RotatedDesc.yMotion = NX_D6JOINT_MOTION_LOCKED;
	d6RotatedDesc.zMotion = NX_D6JOINT_MOTION_LOCKED;
	d6RotatedDesc.projectionDistance = 0.0f;
	d6RotatedDesc.projectionAngle = 0.0f;
	d6RotatedDesc.projectionMode = NX_JPM_NONE;
	setGlobalAnchor(d6RotatedDesc, NxVec3(3.0f, 2.0f, 0.0f));
	setGlobalAxis(d6RotatedDesc, NxVec3(0.70710678f, 0.70710678f, 0.0f));
	NxJoint* d6RotatedJoint = d6RotatedScene->createJoint(d6RotatedDesc);
	if(!d6RotatedJoint)
		return nxFail("rotated D6 joint creation failed");
	for(unsigned step = 0; step != 12; ++step)
		{
		d6RotatedScene->simulate(0.02f);
		const bool ready = d6RotatedScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = d6RotatedScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("rotated D6 simulation result was not ready and fetched");
		char stage[32];
		sprintf_s(stage, "d6Rotated%u", step);
		nxPrintActorState(stage, *d6RotatedActor);
		}
	printf("simulation d6-rotated-locked-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*d6RotatedScene);

	// Lock all three angular axes while the actor starts with angular velocity.
	// This drives the support solver's angular-only impulse update path.
	NxSceneDesc d6AngularSceneDesc;
	d6AngularSceneDesc.setToDefault();
	d6AngularSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* d6AngularScene = sdk->createScene(d6AngularSceneDesc);
	if(!d6AngularScene)
		return nxFail("angular D6 scene creation failed");
	d6AngularScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc d6AngularBody;
	d6AngularBody.setToDefault();
	d6AngularBody.angularVelocity = NxVec3(0.0f, 0.0f, 2.0f);
	NxActorDesc d6AngularActorDesc;
	d6AngularActorDesc.body = &d6AngularBody;
	d6AngularActorDesc.density = 1.0f;
	d6AngularActorDesc.globalPose.t = NxVec3(5.0f, 2.0f, 0.0f);
	d6AngularActorDesc.shapes.pushBack(&forceSphere);
	NxActor* d6AngularActor = d6AngularScene->createActor(d6AngularActorDesc);
	if(!d6AngularActor)
		return nxFail("angular D6 actor creation failed");
	NxD6JointDesc d6AngularDesc;
	d6AngularDesc.setToDefault();
	d6AngularDesc.actor[0] = d6AngularActor;
	d6AngularDesc.actor[1] = 0;
	d6AngularDesc.xMotion = NX_D6JOINT_MOTION_LOCKED;
	d6AngularDesc.yMotion = NX_D6JOINT_MOTION_LOCKED;
	d6AngularDesc.zMotion = NX_D6JOINT_MOTION_LOCKED;
	d6AngularDesc.twistMotion = NX_D6JOINT_MOTION_LOCKED;
	d6AngularDesc.swing1Motion = NX_D6JOINT_MOTION_LOCKED;
	d6AngularDesc.swing2Motion = NX_D6JOINT_MOTION_LOCKED;
	d6AngularDesc.projectionDistance = 0.0f;
	d6AngularDesc.projectionAngle = 0.0f;
	d6AngularDesc.projectionMode = NX_JPM_NONE;
	setGlobalAnchor(d6AngularDesc, NxVec3(5.0f, 2.0f, 0.0f));
	setGlobalAxis(d6AngularDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* d6AngularJoint = d6AngularScene->createJoint(d6AngularDesc);
	if(!d6AngularJoint)
		return nxFail("angular D6 joint creation failed");
	for(unsigned step = 0; step != 12; ++step)
		{
		d6AngularScene->simulate(0.02f);
		const bool ready = d6AngularScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = d6AngularScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("angular D6 simulation result was not ready and fetched");
		char stage[32];
		sprintf_s(stage, "d6Angular%u", step);
		nxPrintActorRotationState(stage, *d6AngularActor);
		}
	printf("simulation d6-angular-locked-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*d6AngularScene);

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

	sdk->releaseScene(*scene);
	sdk->release();

	// Recreate the SDK several times in one process, stepping and releasing a
	// dynamic actor scene each time. This covers allocator/global teardown and
	// reinitialization beyond the earlier one-shot simulation fixtures.
	for(unsigned cycle = 0; cycle != 3; ++cycle)
		{
		NxPhysicsSDK* cycleSDK = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
		if(!cycleSDK)
			return nxFail("repeated lifecycle SDK creation failed");
		NxSceneDesc cycleSceneDesc;
		cycleSceneDesc.setToDefault();
		cycleSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
		NxScene* cycleScene = cycleSDK->createScene(cycleSceneDesc);
		if(!cycleScene)
			return nxFail("repeated lifecycle scene creation failed");
		cycleScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
		NxBodyDesc cycleBody;
		NxActorDesc cycleActorDesc;
		cycleActorDesc.body = &cycleBody;
		cycleActorDesc.density = 1.0f;
		cycleActorDesc.globalPose.t = NxVec3(0.0f, 2.0f, 0.0f);
		cycleActorDesc.shapes.pushBack(&forceSphere);
		NxActor* cycleActor = cycleScene->createActor(cycleActorDesc);
		if(!cycleActor)
			return nxFail("repeated lifecycle actor creation failed");
		cycleScene->simulate(0.02f);
		const bool cycleReady = cycleScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool cycleFetched = cycleScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!cycleReady || !cycleFetched)
			return nxFail("repeated lifecycle simulation result was not ready and fetched");
		const NxU32 actorsBeforeRelease = cycleScene->getNbActors();
		const NxU32 scenesBeforeRelease = cycleSDK->getNbScenes();
		cycleSDK->releaseScene(*cycleScene);
		const NxU32 scenesAfterRelease = cycleSDK->getNbScenes();
		printf("simulation lifecycle cycle=%u actors=%u scenes=%u.%u ready=1 fetched=1\n",
			cycle, actorsBeforeRelease, scenesBeforeRelease, scenesAfterRelease);
		cycleSDK->release();
		}

	// Release a populated scene with a live joint and both dynamic actors still
	// attached. This covers the teardown path that empty-scene cycles miss.
	NxPhysicsSDK* populatedSDK = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!populatedSDK)
		return nxFail("populated lifecycle SDK creation failed");
	NxSceneDesc populatedSceneDesc;
	populatedSceneDesc.setToDefault();
	populatedSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* populatedScene = populatedSDK->createScene(populatedSceneDesc);
	if(!populatedScene)
		return nxFail("populated lifecycle scene creation failed");
	populatedScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc populatedBody0;
	NxActorDesc populatedActorDesc0;
	populatedActorDesc0.body = &populatedBody0;
	populatedActorDesc0.density = 1.0f;
	populatedActorDesc0.globalPose.t = NxVec3(-0.5f, 2.0f, 0.0f);
	populatedActorDesc0.shapes.pushBack(&forceSphere);
	NxActor* populatedActor0 = populatedScene->createActor(populatedActorDesc0);
	NxBodyDesc populatedBody1;
	NxActorDesc populatedActorDesc1;
	populatedActorDesc1.body = &populatedBody1;
	populatedActorDesc1.density = 1.0f;
	populatedActorDesc1.globalPose.t = NxVec3(0.5f, 2.0f, 0.0f);
	populatedActorDesc1.shapes.pushBack(&forceSphere);
	NxActor* populatedActor1 = populatedScene->createActor(populatedActorDesc1);
	if(!populatedActor0 || !populatedActor1)
		return nxFail("populated lifecycle actor creation failed");
	NxFixedJointDesc populatedJointDesc;
	populatedJointDesc.setToDefault();
	populatedJointDesc.actor[0] = populatedActor0;
	populatedJointDesc.actor[1] = populatedActor1;
	setGlobalAnchor(populatedJointDesc, NxVec3(0.0f, 2.0f, 0.0f));
	NxJoint* populatedJoint = populatedScene->createJoint(populatedJointDesc);
	if(!populatedJoint)
		return nxFail("populated lifecycle joint creation failed");
	populatedScene->simulate(0.02f);
	const bool populatedReady = populatedScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool populatedFetched = populatedScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!populatedReady || !populatedFetched)
		return nxFail("populated lifecycle simulation result was not ready and fetched");
	const NxU32 populatedActors = populatedScene->getNbActors();
	const NxU32 populatedJoints = populatedScene->getNbJoints();
	const NxU32 populatedScenes = populatedSDK->getNbScenes();
	populatedSDK->releaseScene(*populatedScene);
	const NxU32 populatedScenesAfterRelease = populatedSDK->getNbScenes();
	printf("simulation lifecycle populated actors=%u joints=%u scenes=%u.%u ready=1 fetched=1\n",
		populatedActors, populatedJoints, populatedScenes, populatedScenesAfterRelease);
	populatedSDK->release();

	// Create and step two populated scenes under one SDK, then release them in
	// reverse order to exercise scene-list unlinking and shared allocator state.
	NxPhysicsSDK* multiSceneSDK = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!multiSceneSDK)
		return nxFail("multi-scene lifecycle SDK creation failed");
	NxScene* multiScenes[2] = { 0, 0 };
	NxActor* multiActors[2][2] = { { 0, 0 }, { 0, 0 } };
	NxJoint* multiJoints[2] = { 0, 0 };
	for(unsigned sceneIndex = 0; sceneIndex != 2; ++sceneIndex)
		{
		NxSceneDesc multiSceneDesc;
		multiSceneDesc.setToDefault();
		multiSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
		multiScenes[sceneIndex] = multiSceneSDK->createScene(multiSceneDesc);
		if(!multiScenes[sceneIndex])
			return nxFail("multi-scene lifecycle scene creation failed");
		multiScenes[sceneIndex]->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
		NxBodyDesc multiBody0;
		NxActorDesc multiActorDesc0;
		multiActorDesc0.body = &multiBody0;
		multiActorDesc0.density = 1.0f;
		multiActorDesc0.globalPose.t = NxVec3(-0.5f, 2.0f, 0.0f);
		multiActorDesc0.shapes.pushBack(&forceSphere);
		multiActors[sceneIndex][0] = multiScenes[sceneIndex]->createActor(multiActorDesc0);
		NxBodyDesc multiBody1;
		NxActorDesc multiActorDesc1;
		multiActorDesc1.body = &multiBody1;
		multiActorDesc1.density = 1.0f;
		multiActorDesc1.globalPose.t = NxVec3(0.5f, 2.0f, 0.0f);
		multiActorDesc1.shapes.pushBack(&forceSphere);
		multiActors[sceneIndex][1] = multiScenes[sceneIndex]->createActor(multiActorDesc1);
		if(!multiActors[sceneIndex][0] || !multiActors[sceneIndex][1])
			return nxFail("multi-scene lifecycle actor creation failed");
		NxFixedJointDesc multiJointDesc;
		multiJointDesc.setToDefault();
		multiJointDesc.actor[0] = multiActors[sceneIndex][0];
		multiJointDesc.actor[1] = multiActors[sceneIndex][1];
		setGlobalAnchor(multiJointDesc, NxVec3(0.0f, 2.0f, 0.0f));
		multiJoints[sceneIndex] = multiScenes[sceneIndex]->createJoint(multiJointDesc);
		if(!multiJoints[sceneIndex])
			return nxFail("multi-scene lifecycle joint creation failed");
		multiScenes[sceneIndex]->simulate(0.02f);
		const bool multiReady = multiScenes[sceneIndex]->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool multiFetched = multiScenes[sceneIndex]->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!multiReady || !multiFetched)
			return nxFail("multi-scene lifecycle simulation result was not ready and fetched");
		}
	const NxU32 multiActorCount0 = multiScenes[0]->getNbActors();
	const NxU32 multiActorCount1 = multiScenes[1]->getNbActors();
	const NxU32 multiJointCount0 = multiScenes[0]->getNbJoints();
	const NxU32 multiJointCount1 = multiScenes[1]->getNbJoints();
	const NxU32 multiSceneCount = multiSceneSDK->getNbScenes();
	multiSceneSDK->releaseScene(*multiScenes[1]);
	const NxU32 multiSceneCountAfter1 = multiSceneSDK->getNbScenes();
	multiSceneSDK->releaseScene(*multiScenes[0]);
	const NxU32 multiSceneCountAfter0 = multiSceneSDK->getNbScenes();
	printf("simulation lifecycle multi-scene actors=%u.%u joints=%u.%u scenes=%u.%u.%u ready=1 fetched=1\n",
		multiActorCount0, multiActorCount1, multiJointCount0, multiJointCount1,
		multiSceneCount, multiSceneCountAfter1, multiSceneCountAfter0);
	multiSceneSDK->release();

	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
