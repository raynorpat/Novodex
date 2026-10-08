#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string.h>

#include "NxBitField.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxUserOutputStream.h"
#include "fluids/NxFluidDesc.h"
#include "fluids/NxImplicitMeshDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxBoxShape.h"
#include "NxPlaneShapeDesc.h"
#include "NxFixedJointDesc.h"
#include "NxSpringDesc.h"
#include "NxDistanceJointDesc.h"
#include "NxRevoluteJointDesc.h"
#include "NxD6JointDesc.h"
#include "NxSpringAndDamperEffector.h"
#include "NxSpringAndDamperEffectorDesc.h"
#include "NxJoint.h"
#include "NxUserNotify.h"
#include "NxMaterial.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxBounds3.h"
#include "NxUserContactReport.h"
#include "NxFoundationSDK.h"
#include "NxUserAllocator.h"
#include "../Physics/src/include/NpSceneGuard.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);
typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32, NxUserOutputStream*, NxUserAllocator*);
typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);
static unsigned nxFloatBits(NxReal value);

// Private ABI probe for the three-entry controller vtable recovered from the
// pinned DLL. Keep this test-only: the public NxController declaration remains
// intentionally incomplete and unchanged.
class NxGroupsMask;
class NxControllerProbe
	{
	public:
	virtual ~NxControllerProbe() {}
	virtual void move(const NxVec3&, NxU32 activeGroups, NxReal minDistance,
		NxU32& collisionFlags, NxReal sharpness = 1.0f,
		const NxGroupsMask* groupsMask = 0) = 0;
	virtual const NxVec3& getPosition() const = 0;
	};

static unsigned gNxSimulationFluidDestructorCalls = 0;
static unsigned gNxSimulationFluidDestructorFlags = 0;
static void* gNxSimulationFluidDestructorObject = 0;

static void* __fastcall nxSimulationFluidDeletingDestructor(
	void* fluid, void*, unsigned char deleteObject)
	{
	++gNxSimulationFluidDestructorCalls;
	gNxSimulationFluidDestructorFlags = deleteObject;
	gNxSimulationFluidDestructorObject = fluid;
	return fluid;
	}

class NxSimulationOutputStream : public NxUserOutputStream
	{
	public:
	NxSimulationOutputStream(): errors(0), code(NXE_NO_ERROR), line(0)
		{
		message[0] = 0;
		file[0] = 0;
		}
	void reportError(NxErrorCode errorCode, const char* errorMessage, const char* errorFile, int errorLine)
		{
		++errors;
		code = errorCode;
		line = errorLine;
		copy(message, sizeof(message), errorMessage);
		copy(file, sizeof(file), errorFile);
		}
	NxAssertResponse reportAssertViolation(const char*, const char*, int)	{ return NX_AR_CONTINUE; }
	void print(const char*) {}
	void resetLast()
		{
		code = NXE_NO_ERROR;
		line = 0;
		message[0] = 0;
		file[0] = 0;
		}
	unsigned errors;
	NxErrorCode code;
	int line;
	char message[128];
	char file[96];
	private:
	static void copy(char* destination, size_t capacity, const char* source)
		{
		if(!source)
			{
			destination[0] = 0;
			return;
			}
		strncpy_s(destination, capacity, source, _TRUNCATE);
		}
	};

static void nxPrintSimulationSceneState(NxScene* scene, unsigned selector, const char* phase)
	{
	NxSceneStats* const sceneStats = scene->getSceneStats();
	NxSceneLimits sceneLimits;
	scene->getLimits(sceneLimits);
	printf("simulation scene-stats phase=%s selector=%u present=%u contacts=%d max_contacts=%d actors=%d joints=%d awake=%d asleep=%d static_shapes=%d\n",
		phase, selector, sceneStats != 0, sceneStats ? sceneStats->numContacts : -1,
		sceneStats ? sceneStats->maxContacts : -1, sceneStats ? sceneStats->numActors : -1,
		sceneStats ? sceneStats->numJoints : -1, sceneStats ? sceneStats->numAwake : -1,
		sceneStats ? sceneStats->numAsleep : -1, sceneStats ? sceneStats->numStaticShapes : -1);
	printf("simulation scene-limits phase=%s selector=%u actors=%u bodies=%u static_shapes=%u dynamic_shapes=%u joints=%u\n",
		phase, selector, sceneLimits.maxNbActors, sceneLimits.maxNbBodies,
		sceneLimits.maxNbStaticShapes, sceneLimits.maxNbDynamicShapes,
		sceneLimits.maxNbJoints);
	}

struct NxSimulationHeldSceneWriteLock
	{
	void* link;
	HANDLE ready;
	HANDLE release;
	};

static DWORD WINAPI nxSimulationHoldSceneWriteLock(void* context)
	{
	NxSimulationHeldSceneWriteLock* held = static_cast<NxSimulationHeldSceneWriteLock*>(context);
	nxNpSceneGuardEnter(held->link);
	SetEvent(held->ready);
	WaitForSingleObject(held->release, INFINITE);
	nxNpSceneGuardLeave(held->link);
	return 0;
	}

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

class NxSimulationTriggerLifecycleReport : public NxUserTriggerReport
	{
	public:
	NxShape* expectedTrigger;
	NxShape* expectedOther;
	NxTriggerFlag events[8];
	unsigned calls;
	unsigned badShapes;
	NxSimulationTriggerLifecycleReport(NxShape* trigger, NxShape* other)
		: expectedTrigger(trigger), expectedOther(other), calls(0), badShapes(0) {}
	virtual void onTrigger(NxShape& trigger, NxShape& other, NxTriggerFlag event)
		{
		if(&trigger != expectedTrigger || &other != expectedOther)
			++badShapes;
		if(calls < sizeof(events) / sizeof(events[0]))
			events[calls] = event;
		++calls;
		printf("simulation trigger-lifecycle event=%u trigger=%u other=%u\n",
			static_cast<unsigned>(event), &trigger == expectedTrigger,
			&other == expectedOther);
		}
	};

class NxSimulationTriggerBatchReport : public NxUserTriggerReport
	{
	public:
	enum { BatchCount = 8 };
	NxShape* trigger;
	NxShape* others[BatchCount];
	unsigned eventCounts[BatchCount][3];
	unsigned calls;
	unsigned badShapes;
	NxSimulationTriggerBatchReport(NxShape* triggerShape, NxShape** otherShapes)
		: trigger(triggerShape), calls(0), badShapes(0)
		{
		memset(eventCounts, 0, sizeof(eventCounts));
		for(unsigned index = 0; index != BatchCount; ++index)
			others[index] = otherShapes[index];
		}
	virtual void onTrigger(NxShape& triggerShape, NxShape& otherShape, NxTriggerFlag event)
		{
		unsigned otherIndex = BatchCount;
		for(unsigned index = 0; index != BatchCount; ++index)
			if(&otherShape == others[index])
				otherIndex = index;
		unsigned eventIndex = event == NX_TRIGGER_ON_ENTER ? 0 :
			event == NX_TRIGGER_ON_STAY ? 1 : event == NX_TRIGGER_ON_LEAVE ? 2 : 3;
		if(&triggerShape != trigger || otherIndex == BatchCount || eventIndex == 3)
			++badShapes;
		else
			++eventCounts[otherIndex][eventIndex];
		++calls;
		printf("simulation trigger-batch event=%u other=%u trigger_ok=%u\n",
			static_cast<unsigned>(event), otherIndex,
			&triggerShape == trigger);
		}
	};

class NxSimulationCompoundTriggerReport : public NxUserTriggerReport
	{
	public:
	NxShape* triggers[2];
	NxShape* other;
	unsigned eventCounts[2][3];
	unsigned calls;
	unsigned badShapes;
	NxSimulationCompoundTriggerReport(NxShape* const* triggerShapes, NxShape* otherShape)
		: other(otherShape), calls(0), badShapes(0)
		{
		memset(eventCounts, 0, sizeof(eventCounts));
		triggers[0] = triggerShapes[0];
		triggers[1] = triggerShapes[1];
		}
	virtual void onTrigger(NxShape& trigger, NxShape& otherShape, NxTriggerFlag event)
		{
		unsigned triggerIndex = 2;
		for(unsigned index = 0; index != 2; ++index)
			if(&trigger == triggers[index])
				triggerIndex = index;
		const unsigned eventIndex = event == NX_TRIGGER_ON_ENTER ? 0 :
			event == NX_TRIGGER_ON_STAY ? 1 : event == NX_TRIGGER_ON_LEAVE ? 2 : 3;
		if(triggerIndex == 2 || &otherShape != other || eventIndex == 3)
			++badShapes;
		else
			++eventCounts[triggerIndex][eventIndex];
		++calls;
		printf("simulation trigger-compound event=%u trigger=%u other=%u\n",
			static_cast<unsigned>(event), triggerIndex,
			&otherShape == other);
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

class NxSimulationJointBreakNotify : public NxUserNotify
	{
	public:
	NxSimulationJointBreakNotify(bool releaseJoint)
		: releaseJoint(releaseJoint), expectedJoint(0), calls(0),
		  forceBits(0), jointMatches(false), stateAtCallback(0) {}
	virtual bool onJointBreak(NxReal force, NxJoint& joint)
		{
		++calls;
		forceBits = nxFloatBits(force);
		jointMatches = &joint == expectedJoint;
		stateAtCallback = static_cast<unsigned>(joint.getState());
		return releaseJoint;
		}
	bool releaseJoint;
	NxJoint* expectedJoint;
	unsigned calls;
	unsigned forceBits;
	bool jointMatches;
	unsigned stateAtCallback;
	};

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

// NpScene::mScene is at wrapper+0x24; the mode and pair-list offsets are
// pinned by the oracle's 000544 -> 001973 initialization path.
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

struct NxSimulationContactStreamReport : NxUserContactReport
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

	NxSimulationContactStreamReport()
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

	NxSimulationOutputStream simulationOutput;
	bool deferredContactFailed = false;
	bool controllerCreateFailed = false;
	bool shapeUserDataFailed = false;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, &simulationOutput);
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}
	// Exercise trigger-pair generation, enter/stay/leave reconciliation, and
	// fetch-time callback delivery through the public scene API.
	NxSceneDesc triggerLifecycleSceneDesc;
	triggerLifecycleSceneDesc.setToDefault();
	NxScene* const triggerLifecycleScene = sdk->createScene(triggerLifecycleSceneDesc);
	if(!triggerLifecycleScene)
		return nxFail("trigger lifecycle scene creation failed");
	triggerLifecycleScene->setGravity(NxVec3(0.0f, 0.0f, 0.0f));
	triggerLifecycleScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
	NxBoxShapeDesc lifecycleTriggerShapeDesc;
	lifecycleTriggerShapeDesc.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	lifecycleTriggerShapeDesc.shapeFlags |= NX_TRIGGER_ENABLE;
	NxActorDesc lifecycleTriggerActorDesc;
	lifecycleTriggerActorDesc.shapes.pushBack(&lifecycleTriggerShapeDesc);
	NxActor* const lifecycleTriggerActor =
		triggerLifecycleScene->createActor(lifecycleTriggerActorDesc);
	NxSphereShapeDesc lifecycleOtherShapeDesc;
	lifecycleOtherShapeDesc.radius = 0.5f;
	NxBodyDesc lifecycleOtherBodyDesc;
	NxActorDesc lifecycleOtherActorDesc;
	lifecycleOtherActorDesc.body = &lifecycleOtherBodyDesc;
	lifecycleOtherActorDesc.density = 1.0f;
	lifecycleOtherActorDesc.shapes.pushBack(&lifecycleOtherShapeDesc);
	NxActor* const lifecycleOtherActor =
		triggerLifecycleScene->createActor(lifecycleOtherActorDesc);
	if(!lifecycleTriggerActor || !lifecycleOtherActor)
		return nxFail("trigger lifecycle actors creation failed");
	NxShape* const lifecycleTriggerShape = lifecycleTriggerActor->getShapes()[0];
	NxShape* const lifecycleOtherShape = lifecycleOtherActor->getShapes()[0];
	NxSimulationTriggerLifecycleReport lifecycleTriggerReport(
		lifecycleTriggerShape, lifecycleOtherShape);
	triggerLifecycleScene->setUserTriggerReport(&lifecycleTriggerReport);
	bool lifecycleFetched = true;
	for(unsigned step = 0; step != 3 && lifecycleFetched; ++step)
		{
		lifecycleOtherActor->setGlobalPosition(NxVec3(0.1f * step, 0.0f, 0.0f));
		triggerLifecycleScene->simulate(0.01f);
		lifecycleFetched = triggerLifecycleScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		printf("simulation trigger-lifecycle tick=%u calls=%u\n", step + 1,
			lifecycleTriggerReport.calls);
		}
	lifecycleOtherActor->setGlobalPosition(NxVec3(4.0f, 0.0f, 0.0f));
	if(lifecycleFetched)
		{
		triggerLifecycleScene->simulate(0.01f);
		lifecycleFetched = triggerLifecycleScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	const bool triggerLifecycleExact = lifecycleFetched &&
		lifecycleTriggerReport.calls == 3 && lifecycleTriggerReport.badShapes == 0 &&
		lifecycleTriggerReport.events[0] == NX_TRIGGER_ON_ENTER &&
		lifecycleTriggerReport.events[1] == NX_TRIGGER_ON_STAY &&
		lifecycleTriggerReport.events[2] == NX_TRIGGER_ON_LEAVE;
	printf("simulation trigger-lifecycle summary fetched=%u calls=%u bad_shapes=%u exact=%u\n",
		lifecycleFetched, lifecycleTriggerReport.calls,
		lifecycleTriggerReport.badShapes, triggerLifecycleExact);
	triggerLifecycleScene->releaseActor(*lifecycleOtherActor);
	triggerLifecycleScene->releaseActor(*lifecycleTriggerActor);
	sdk->releaseScene(*triggerLifecycleScene);
	if(!triggerLifecycleExact)
		return nxFail("trigger enter/stay/leave lifecycle did not match the public contract");
	// Several live pairs exercise hash collisions and event-buffer growth in one
	// scene. Each body enters, remains for a fetched step, then leaves.
	NxSceneDesc triggerBatchSceneDesc;
	triggerBatchSceneDesc.setToDefault();
	NxScene* const triggerBatchScene = sdk->createScene(triggerBatchSceneDesc);
	if(!triggerBatchScene)
		return nxFail("trigger batch scene creation failed");
	triggerBatchScene->setGravity(NxVec3(0.0f, 0.0f, 0.0f));
	triggerBatchScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
	NxBoxShapeDesc batchTriggerShapeDesc;
	batchTriggerShapeDesc.dimensions = NxVec3(2.0f, 2.0f, 2.0f);
	batchTriggerShapeDesc.shapeFlags |= NX_TRIGGER_ENABLE;
	batchTriggerShapeDesc.shapeFlags = (batchTriggerShapeDesc.shapeFlags & ~NX_TRIGGER_ENABLE) |
		NX_TRIGGER_ON_STAY;
	NxActorDesc batchTriggerActorDesc;
	batchTriggerActorDesc.shapes.pushBack(&batchTriggerShapeDesc);
	NxActor* const batchTriggerActor = triggerBatchScene->createActor(batchTriggerActorDesc);
	NxActor* batchOtherActors[NxSimulationTriggerBatchReport::BatchCount] = {};
	NxShape* batchOtherShapes[NxSimulationTriggerBatchReport::BatchCount] = {};
	for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
		{
		NxSphereShapeDesc batchOtherShapeDesc;
		batchOtherShapeDesc.radius = 0.07f;
		NxBodyDesc batchOtherBodyDesc;
		NxActorDesc batchOtherActorDesc;
		batchOtherActorDesc.body = &batchOtherBodyDesc;
		batchOtherActorDesc.density = 1.0f;
		batchOtherActorDesc.globalPose.t = NxVec3(-0.7f + 0.2f * index, 0.0f, 0.0f);
		batchOtherActorDesc.shapes.pushBack(&batchOtherShapeDesc);
		batchOtherActors[index] = triggerBatchScene->createActor(batchOtherActorDesc);
		if(!batchOtherActors[index])
			return nxFail("trigger batch dynamic actor creation failed");
		batchOtherShapes[index] = batchOtherActors[index]->getShapes()[0];
		}
	if(!batchTriggerActor)
		return nxFail("trigger batch trigger creation failed");
	NxSimulationTriggerBatchReport triggerBatchReport(
		batchTriggerActor->getShapes()[0], batchOtherShapes);
	triggerBatchScene->setUserTriggerReport(&triggerBatchReport);
	bool triggerBatchFetched = true;
	for(unsigned step = 0; step != 3 && triggerBatchFetched; ++step)
		{
		if(step < 2)
			for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
				batchOtherActors[index]->setGlobalPosition(
					NxVec3(-0.7f + 0.2f * index + 0.05f * step, 0.0f, 0.0f));
		triggerBatchScene->simulate(0.01f);
		triggerBatchFetched = triggerBatchScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		printf("simulation trigger-batch tick=%u calls=%u\n", step + 1,
			triggerBatchReport.calls);
		if(step == 1)
			for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
				batchOtherActors[index]->putToSleep();
		}
	for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
		batchOtherActors[index]->setGlobalPosition(NxVec3(4.0f + index, 0.0f, 0.0f));
	if(triggerBatchFetched)
		{
		triggerBatchScene->simulate(0.01f);
		triggerBatchFetched = triggerBatchScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	bool triggerBatchExact = triggerBatchFetched && triggerBatchReport.calls == 24 &&
		triggerBatchReport.badShapes == 0;
	for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
		for(unsigned event = 0; event != 3; ++event)
			triggerBatchExact = triggerBatchExact &&
				triggerBatchReport.eventCounts[index][event] == 1;
	printf("simulation trigger-batch summary fetched=%u calls=%u bad_shapes=%u exact=%u\n",
		triggerBatchFetched, triggerBatchReport.calls,
		triggerBatchReport.badShapes, triggerBatchExact);
	for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
		triggerBatchScene->releaseActor(*batchOtherActors[index]);
	triggerBatchScene->releaseActor(*batchTriggerActor);
	sdk->releaseScene(*triggerBatchScene);
	if(!triggerBatchExact)
		return nxFail("multi-pair trigger lifecycle did not match the public contract");
	// Two trigger shapes on one actor must retain separate overlap pairs even
	// when both shapes overlap the same dynamic actor.
	NxSceneDesc triggerCompoundSceneDesc;
	triggerCompoundSceneDesc.setToDefault();
	NxScene* const triggerCompoundScene = sdk->createScene(triggerCompoundSceneDesc);
	if(!triggerCompoundScene)
		return nxFail("compound trigger scene creation failed");
	triggerCompoundScene->setGravity(NxVec3(0.0f, 0.0f, 0.0f));
	triggerCompoundScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
	NxBoxShapeDesc compoundTriggerShapeDescs[2];
	for(unsigned index = 0; index != 2; ++index)
		{
		compoundTriggerShapeDescs[index].dimensions = NxVec3(1.0f, 1.0f, 1.0f);
		compoundTriggerShapeDescs[index].shapeFlags |= NX_TRIGGER_ENABLE;
		}
	NxActorDesc compoundTriggerActorDesc;
	compoundTriggerActorDesc.shapes.pushBack(&compoundTriggerShapeDescs[0]);
	compoundTriggerActorDesc.shapes.pushBack(&compoundTriggerShapeDescs[1]);
	NxActor* const compoundTriggerActor =
		triggerCompoundScene->createActor(compoundTriggerActorDesc);
	NxSphereShapeDesc compoundOtherShapeDesc;
	compoundOtherShapeDesc.radius = 0.25f;
	NxBodyDesc compoundOtherBodyDesc;
	NxActorDesc compoundOtherActorDesc;
	compoundOtherActorDesc.body = &compoundOtherBodyDesc;
	compoundOtherActorDesc.density = 1.0f;
	compoundOtherActorDesc.shapes.pushBack(&compoundOtherShapeDesc);
	NxActor* const compoundOtherActor =
		triggerCompoundScene->createActor(compoundOtherActorDesc);
	if(!compoundTriggerActor || !compoundOtherActor)
		return nxFail("compound trigger actor creation failed");
	NxShape* const compoundTriggerShapes[] = {
		compoundTriggerActor->getShapes()[0], compoundTriggerActor->getShapes()[1]
		};
	NxSimulationCompoundTriggerReport compoundTriggerReport(
		compoundTriggerShapes, compoundOtherActor->getShapes()[0]);
	triggerCompoundScene->setUserTriggerReport(&compoundTriggerReport);
	bool triggerCompoundFetched = true;
	for(unsigned step = 0; step != 3 && triggerCompoundFetched; ++step)
		{
		triggerCompoundScene->simulate(0.01f);
		triggerCompoundFetched = triggerCompoundScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	compoundOtherActor->setGlobalPosition(NxVec3(4.0f, 0.0f, 0.0f));
	if(triggerCompoundFetched)
		{
		triggerCompoundScene->simulate(0.01f);
		triggerCompoundFetched =
			triggerCompoundScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	bool triggerCompoundExact = triggerCompoundFetched &&
		compoundTriggerReport.calls == 6 && compoundTriggerReport.badShapes == 0;
	for(unsigned trigger = 0; trigger != 2; ++trigger)
		for(unsigned event = 0; event != 3; ++event)
			triggerCompoundExact = triggerCompoundExact &&
				compoundTriggerReport.eventCounts[trigger][event] == 1;
	printf("simulation trigger-compound summary fetched=%u calls=%u bad_shapes=%u exact=%u\n",
		triggerCompoundFetched, compoundTriggerReport.calls,
		compoundTriggerReport.badShapes, triggerCompoundExact);
	triggerCompoundScene->releaseActor(*compoundOtherActor);
	triggerCompoundScene->releaseActor(*compoundTriggerActor);
	sdk->releaseScene(*triggerCompoundScene);
	if(!triggerCompoundExact)
		return nxFail("compound trigger lifecycle did not match the public contract");
	// A public mesh factory smoke case. The oracle accepts this valid two-face
	// descriptor; keeping it in the simulation corpus ensures a rebuilt mesh can
	// become the next real static-contact fixture rather than remaining an
	// internal-only asset object.
	const NxPoint meshPoints[] = {
		NxPoint(-2.0f, 0.0f, -2.0f), NxPoint(2.0f, 0.0f, -2.0f), NxPoint(-2.0f, 0.0f, 2.0f),
		NxPoint(2.0f, 0.0f, -2.0f), NxPoint(2.0f, 0.0f, 2.0f), NxPoint(-2.0f, 0.0f, 2.0f)
		};
	const NxU32 meshTriangles[] = { 0, 1, 2, 3, 4, 5 };
	NxTriangleMeshDesc meshDesc;
	meshDesc.numVertices = sizeof(meshPoints) / sizeof(meshPoints[0]);
	meshDesc.numTriangles = 2;
	meshDesc.pointStrideBytes = sizeof(NxPoint);
	meshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	meshDesc.points = meshPoints;
	meshDesc.triangles = meshTriangles;
	NxTriangleMesh* const publicMesh = sdk->createTriangleMesh(meshDesc);
	printf("simulation triangle-mesh create=%u\n", publicMesh != 0);
	if(publicMesh)
		{
		const NxU32* const meshIndices = static_cast<const NxU32*>(publicMesh->getBase(0, NX_ARRAY_TRIANGLES));
	const NxPoint* const copiedMeshPoints = static_cast<const NxPoint*>(publicMesh->getBase(0, NX_ARRAY_VERTICES));
	NxTriangleMeshDesc savedMeshDesc;
	const bool savedMesh = publicMesh->saveToDesc(savedMeshDesc);
	printf("simulation triangle-mesh data submeshes=%u vertices=%u triangles=%u vertex_format=%u vertex_stride=%u index_format=%u index_stride=%u first_index=%u last_index=%u first_vertex=%08x.%08x.%08x save=%u saved_counts=%u.%u saved_strides=%u.%u\n",
		publicMesh->getSubmeshCount(), publicMesh->getCount(0, NX_ARRAY_VERTICES),
		publicMesh->getCount(0, NX_ARRAY_TRIANGLES),
		publicMesh->getFormat(0, NX_ARRAY_VERTICES), publicMesh->getStride(0, NX_ARRAY_VERTICES),
		publicMesh->getFormat(0, NX_ARRAY_TRIANGLES), publicMesh->getStride(0, NX_ARRAY_TRIANGLES),
		meshIndices ? meshIndices[0] : 0, meshIndices ? meshIndices[5] : 0,
		copiedMeshPoints ? nxFloatBits(copiedMeshPoints[0].x) : 0,
		copiedMeshPoints ? nxFloatBits(copiedMeshPoints[0].y) : 0,
		copiedMeshPoints ? nxFloatBits(copiedMeshPoints[0].z) : 0,
		savedMesh, savedMeshDesc.numVertices, savedMeshDesc.numTriangles,
		savedMeshDesc.pointStrideBytes, savedMeshDesc.triangleStrideBytes);
		}
	if(publicMesh)
		sdk->releaseTriangleMesh(*publicMesh);
	// 16-bit input indices are part of the public descriptor contract. The
	// triangle-mesh implementation must normalize them into the same public
	// indexed representation while preserving the caller's vertex ordering.
	const NxU16 meshTriangles16[] = { 0, 1, 2, 3, 4, 5 };
	NxTriangleMeshDesc mesh16Desc;
	mesh16Desc.numVertices = sizeof(meshPoints) / sizeof(meshPoints[0]);
	mesh16Desc.numTriangles = 2;
	mesh16Desc.pointStrideBytes = sizeof(NxPoint);
	mesh16Desc.triangleStrideBytes = 3 * sizeof(NxU16);
	mesh16Desc.points = meshPoints;
	mesh16Desc.triangles = meshTriangles16;
	mesh16Desc.flags = NX_MF_16_BIT_INDICES;
	NxTriangleMesh* const publicMesh16 = sdk->createTriangleMesh(mesh16Desc);
	printf("simulation triangle-mesh16 create=%u", publicMesh16 != 0);
	if(publicMesh16)
		{
		const NxU32* const indices16 = static_cast<const NxU32*>(publicMesh16->getBase(0, NX_ARRAY_TRIANGLES));
		printf(" vertices=%u triangles=%u index_format=%u index_stride=%u first_index=%u last_index=%u",
			publicMesh16->getCount(0, NX_ARRAY_VERTICES), publicMesh16->getCount(0, NX_ARRAY_TRIANGLES),
			publicMesh16->getFormat(0, NX_ARRAY_TRIANGLES), publicMesh16->getStride(0, NX_ARRAY_TRIANGLES),
			indices16 ? indices16[0] : 0, indices16 ? indices16[5] : 0);
		sdk->releaseTriangleMesh(*publicMesh16);
		}
	printf("\n");
	// Vehicle wheels discover the created capsule shape through the descriptor's
	// userData pointer, so this public-API case pins that descriptor-to-handle copy.
	static unsigned char capsuleUserDataMarker;
	NxSceneDesc capsuleUserDataSceneDesc;
	capsuleUserDataSceneDesc.setToDefault();
	NxScene* const capsuleUserDataScene = sdk->createScene(capsuleUserDataSceneDesc);
	if(!capsuleUserDataScene)
		return nxFail("capsule userData scene creation failed");
	NxCapsuleShapeDesc capsuleUserDataShape;
	capsuleUserDataShape.radius = 0.25f;
	capsuleUserDataShape.height = 0.5f;
	capsuleUserDataShape.userData = &capsuleUserDataMarker;
	NxActorDesc capsuleUserDataActorDesc;
	capsuleUserDataActorDesc.shapes.pushBack(&capsuleUserDataShape);
	NxActor* const capsuleUserDataActor = capsuleUserDataScene->createActor(capsuleUserDataActorDesc);
	NxShape** const capsuleUserDataShapes = capsuleUserDataActor
		? capsuleUserDataActor->getShapes() : 0;
	NxShape* const capsuleUserDataHandle = capsuleUserDataShapes ? capsuleUserDataShapes[0] : 0;
	const bool capsuleUserDataMatches = capsuleUserDataHandle &&
		capsuleUserDataHandle->userData == &capsuleUserDataMarker;
	printf("simulation capsule-shape userdata actor=%u shapes=%u marker=%u\n",
		capsuleUserDataActor != 0,
		capsuleUserDataActor ? capsuleUserDataActor->getNbShapes() : 0,
		capsuleUserDataMatches);
	shapeUserDataFailed = !capsuleUserDataActor || !capsuleUserDataMatches;
	if(capsuleUserDataActor)
		capsuleUserDataScene->releaseActor(*capsuleUserDataActor);
	sdk->releaseScene(*capsuleUserDataScene);
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
		if(selector == NX_BROADPHASE_QUADRATIC)
			{
			NxTriangleMesh* const sceneMesh = sdk->createTriangleMesh(meshDesc);
			printf("simulation triangle-mesh scene_owner create=%u\n", sceneMesh != 0);
			if(sceneMesh)
				{
				NxTriangleMeshShapeDesc meshShapeDesc;
				meshShapeDesc.meshData = sceneMesh;
				NxActorDesc meshActorDesc;
				meshActorDesc.shapes.pushBack(&meshShapeDesc);
				NxActor* const meshActor = broadPhaseScene->createActor(meshActorDesc);
				printf("simulation triangle-mesh actor=%u shapes=%u\n",
					meshActor != 0, meshActor ? meshActor->getNbShapes() : 0);
				NxShape** const actorShapes = meshActor ? meshActor->getShapes() : 0;
				NxShape* const actorShape = actorShapes ? actorShapes[0] : 0;
				NxTriangleMeshShape* const publicMeshShape =
					actorShape ? actorShape->isTriangleMesh() : 0;
				NxTriangleMesh* const recoveredMesh = publicMeshShape
					? &publicMeshShape->getTriangleMesh() : 0;
				NxBounds3 meshWorldBounds;
				if(actorShape)
					actorShape->getWorldBounds(meshWorldBounds);
				printf("simulation triangle-mesh handle type=%u mesh_same=%u\n",
					actorShape ? actorShape->getType() : NX_SHAPE_COUNT,
					recoveredMesh == sceneMesh);
				printf("simulation triangle-mesh world_bounds=%08x.%08x.%08x.%08x.%08x.%08x\n",
					nxFloatBits(meshWorldBounds.getMin().x), nxFloatBits(meshWorldBounds.getMin().y),
					nxFloatBits(meshWorldBounds.getMin().z), nxFloatBits(meshWorldBounds.getMax().x),
					nxFloatBits(meshWorldBounds.getMax().y), nxFloatBits(meshWorldBounds.getMax().z));
				if(meshActor)
					broadPhaseScene->releaseActor(*meshActor);
				sdk->releaseTriangleMesh(*sceneMesh);
				}
			NxSimulationHeldSceneWriteLock held = {
				*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(broadPhaseScene) + 0x0c),
				CreateEventA(0, TRUE, FALSE, 0), CreateEventA(0, TRUE, FALSE, 0) };
			if(!held.link || !held.ready || !held.release)
				return nxFail("triangle-mesh scene-lock fixture setup failed");
			HANDLE lockThread = CreateThread(0, 0, nxSimulationHoldSceneWriteLock, &held, 0, 0);
			if(!lockThread || WaitForSingleObject(held.ready, 5000) != WAIT_OBJECT_0)
				return nxFail("triangle-mesh scene-lock fixture did not acquire the lock");
			NxTriangleMesh* const lockedMesh = sdk->createTriangleMesh(meshDesc);
			printf("simulation triangle-mesh locked_create=%u\n", lockedMesh != 0);
			SetEvent(held.release);
			WaitForSingleObject(lockThread, INFINITE);
			CloseHandle(lockThread);
			CloseHandle(held.ready);
			CloseHandle(held.release);
			if(lockedMesh)
				sdk->releaseTriangleMesh(*lockedMesh);
			}
		printf("simulation broadphase selector=%u mode=%u\n",
			selector, nxReadSceneBroadPhaseMode(broadPhaseScene));
		nxPrintSimulationSceneState(broadPhaseScene, selector, "created");
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
		nxPrintSimulationSceneState(broadPhaseScene, selector, "active");
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
		if(selector == NX_BROADPHASE_COHERENT)
			{
			nxPrintActorBodyVelocity("broadphase2-contact1", *chainActors[1]);
			nxPrintActorBodyVelocity("broadphase2-contact2", *chainActors[2]);
			}
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
	const unsigned simulateErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	emptyScene->simulate(-0.125f);
	printf("simulation negative-dt error=%u code=%u line=%d file=%s message=%s\n",
		simulationOutput.errors - simulateErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	emptyScene->simulate(0.0f);
	bool zeroDtReady = false;
	for(unsigned wait = 0; wait != 1000 && !zeroDtReady; ++wait)
		{
		zeroDtReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
		if(!zeroDtReady)
			::Sleep(1);
		}
	const bool zeroDtFetched = zeroDtReady
		? emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false) : false;
	printf("simulation zero-dt ready=%u fetched=%u\n",
		zeroDtReady ? 1u : 0u, zeroDtFetched ? 1u : 0u);
	unsigned char* emptyWrapper = reinterpret_cast<unsigned char*>(emptyScene);
	unsigned char* emptyInternal = *reinterpret_cast<unsigned char**>(emptyWrapper + 0x24);
	emptyScene->simulate(0.125f);
	emptyScene->simulate(0.25f);
	const unsigned queuedDtBits = nxFloatBits(
		*reinterpret_cast<NxReal*>(emptyInternal + 0x544));
	const unsigned char pendingFlag = emptyWrapper[0x20];
	bool submittedReady = false;
	for(unsigned wait = 0; wait != 1000 && !submittedReady; ++wait)
		{
		submittedReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
		if(!submittedReady)
			::Sleep(1);
		}
	const bool submittedFetched = submittedReady
		? emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false) : false;
	printf("simulation submit state dt=%08x pending=%u ready=%u fetched=%u finished=%u\n",
		queuedDtBits, static_cast<unsigned>(pendingFlag), submittedReady ? 1u : 0u,
		submittedFetched ? 1u : 0u, static_cast<unsigned>(emptyWrapper[0x20]));
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

	// Reach the reconstructed spring/damper effector through the real scene
	// step. The dynamic body starts beyond the relaxed length from the world
	// anchor; the oracle's pre-solver effector tick must pull it toward the anchor.
	NxSceneDesc effectorStepSceneDesc;
	effectorStepSceneDesc.setToDefault();
	effectorStepSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* const effectorStepScene = sdk->createScene(effectorStepSceneDesc);
	if(!effectorStepScene)
		return nxFail("effector-step scene creation failed");
	effectorStepScene->setTiming(1.0f / 60.0f, 1, NX_TIMESTEP_FIXED);
	NxBoxShapeDesc effectorStepShape;
	effectorStepShape.setToDefault();
	effectorStepShape.dimensions = NxVec3(0.5f, 0.5f, 0.5f);
	NxBodyDesc effectorStepBody1;
	effectorStepBody1.setToDefault();
	NxActorDesc effectorStepActorDesc1;
	effectorStepActorDesc1.setToDefault();
	effectorStepActorDesc1.body = &effectorStepBody1;
	effectorStepActorDesc1.density = 2.0f;
	effectorStepActorDesc1.shapes.pushBack(&effectorStepShape);
	NxActor* const effectorStepActor1 = effectorStepScene->createActor(effectorStepActorDesc1);
	if(!effectorStepActor1)
		return nxFail("effector-step actor creation failed");
	NxSpringAndDamperEffectorDesc effectorStepDesc;
	effectorStepDesc.setToDefault();
	effectorStepDesc.body1 = effectorStepActor1;
	effectorStepDesc.pos2 = NxVec3(2.0f, 0.0f, 0.0f);
	effectorStepDesc.springDistCompressSaturate = 0.5f;
	effectorStepDesc.springDistRelaxed = 1.0f;
	effectorStepDesc.springDistStretchSaturate = 4.0f;
	effectorStepDesc.springMaxCompressForce = 100.0f;
	effectorStepDesc.springMaxStretchForce = 100.0f;
	NxSpringAndDamperEffector* const stepEffector =
		effectorStepScene->createSpringAndDamperEffector(effectorStepDesc);
	if(!stepEffector)
		return nxFail("effector-step creation failed");
	NxReal effectorCompress, effectorRelaxed, effectorStretch, effectorMaxCompress, effectorMaxStretch;
	stepEffector->getLinearSpring(effectorCompress, effectorRelaxed, effectorStretch,
		effectorMaxCompress, effectorMaxStretch);
	const NxVec3 effectorInitialPosition1 = effectorStepActor1->getGlobalPosition();
	printf("simulation effector-step setup count=%u awake=%u pos=%08x spring=%08x.%08x.%08x.%08x.%08x\n",
		effectorStepScene->getNbEffectors(), effectorStepActor1->isSleeping() ? 0u : 1u,
		nxFloatBits(effectorStepActor1->getGlobalPosition().x), nxFloatBits(effectorCompress),
		nxFloatBits(effectorRelaxed), nxFloatBits(effectorStretch),
		nxFloatBits(effectorMaxCompress), nxFloatBits(effectorMaxStretch));
	effectorStepActor1->wakeUp(1.0f);
	effectorStepScene->simulate(1.0f / 60.0f);
	const bool effectorStepReady = effectorStepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool effectorStepFetched = effectorStepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!effectorStepReady || !effectorStepFetched)
		return nxFail("effector-step results were not ready and fetched");
	NxVec3 effectorStepVelocity1;
	effectorStepActor1->getLinearVelocity(effectorStepVelocity1);
	printf("simulation effector-step ready=%u fetched=%u vx=%08x\n",
		effectorStepReady ? 1u : 0u, effectorStepFetched ? 1u : 0u,
		nxFloatBits(effectorStepVelocity1.x));
	effectorStepScene->simulate(1.0f / 60.0f);
	const bool effectorStepReady2 = effectorStepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool effectorStepFetched2 = effectorStepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!effectorStepReady2 || !effectorStepFetched2)
		return nxFail("second effector-step results were not ready and fetched");
	effectorStepActor1->getLinearVelocity(effectorStepVelocity1);
	printf("simulation effector-step second ready=%u fetched=%u vx=%08x\n",
		effectorStepReady2 ? 1u : 0u, effectorStepFetched2 ? 1u : 0u,
		nxFloatBits(effectorStepVelocity1.x));
	effectorStepScene->releaseEffector(*stepEffector);
	sdk->releaseScene(*effectorStepScene);

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
	for(unsigned step = 0; step < 1000; ++step)
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
	printf("simulation stack=settled steps=1000 ready=1 fetched=1\n");
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

	// Exercise both NxUserNotify::onJointBreak return paths. Returning true
	// transfers release to fetchResults; returning false keeps the broken joint
	// detached from its bodies and queryable until the caller releases it.
	for(unsigned releaseJoint = 0; releaseJoint != 2; ++releaseJoint)
		{
		NxSimulationJointBreakNotify notify(releaseJoint != 0);
		NxSceneDesc notifySceneDesc;
		notifySceneDesc.setToDefault();
		notifySceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
		notifySceneDesc.userNotify = &notify;
		NxScene* notifyScene = sdk->createScene(notifySceneDesc);
		if(!notifyScene)
			return nxFail("joint-break notify scene creation failed");
		notifyScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
		NxBodyDesc notifyBody;
		NxActorDesc notifyActorDesc;
		notifyActorDesc.body = &notifyBody;
		notifyActorDesc.density = 1.0f;
		notifyActorDesc.globalPose.t = NxVec3(8.0f + 2.0f * releaseJoint, 2.0f, 0.0f);
		notifyActorDesc.shapes.pushBack(&forceSphere);
		NxActor* notifyActor = notifyScene->createActor(notifyActorDesc);
		if(!notifyActor)
			return nxFail("joint-break notify actor creation failed");
		NxFixedJointDesc notifyJointDesc;
		notifyJointDesc.setToDefault();
		notifyJointDesc.actor[0] = notifyActor;
		notifyJointDesc.actor[1] = 0;
		notifyJointDesc.maxForce = 0.001f;
		setGlobalAnchor(notifyJointDesc, notifyActorDesc.globalPose.t);
		setGlobalAxis(notifyJointDesc, NxVec3(0.0f, 0.0f, 1.0f));
		NxJoint* notifyJoint = notifyScene->createJoint(notifyJointDesc);
		if(!notifyJoint)
			return nxFail("joint-break notify joint creation failed");
		notify.expectedJoint = notifyJoint;
		for(unsigned step = 0; step != 4 && notify.calls == 0; ++step)
			{
			notifyScene->simulate(0.02f);
			const bool ready = notifyScene->checkResults(NX_RIGID_BODY_FINISHED, true);
			const bool fetched = notifyScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
			if(!ready || !fetched)
				return nxFail("joint-break notify result was not ready and fetched");
			char stage[32];
			sprintf_s(stage, "breaknotify%u_%u", releaseJoint, step);
			nxPrintActorState(stage, *notifyActor);
			}
		if(notify.calls != 1)
			return nxFail("joint-break notify callback count was not one");
		const unsigned retainedState = notify.releaseJoint ? 0
			: static_cast<unsigned>(notifyJoint->getState());
		NxActor* brokenActor0 = 0;
		NxActor* brokenActor1 = 0;
		unsigned detachedActors = 0;
		if(!notify.releaseJoint)
			{
			notifyJoint->getActors(&brokenActor0, &brokenActor1);
			detachedActors = !brokenActor0 && !brokenActor1;
			if(!detachedActors)
				return nxFail("retained broken joint actors were not detached");
			}
		printf("simulation break-notify result=%s calls=%u force=%08x joint_match=%u callback_state=%u retained_state=%u detached=%u joints=%u\n",
			notify.releaseJoint ? "release" : "retain", notify.calls, notify.forceBits,
			notify.jointMatches, notify.stateAtCallback, retainedState, detachedActors,
			static_cast<unsigned>(notifyScene->getNbJoints()));
		sdk->releaseScene(*notifyScene);
		}

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

	// Reproduce the D6 swing-limit angular-impulse cancellation case. The
	// actor begins 60 degrees off the world frame; one limited swing axis
	// drives a kind-2 row over four fixed solver steps.
	NxSceneDesc d6SwingSceneDesc;
	d6SwingSceneDesc.setToDefault();
	d6SwingSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* d6SwingScene = sdk->createScene(d6SwingSceneDesc);
	if(!d6SwingScene) return nxFail("D6 swing-limit scene creation failed");
	d6SwingScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc d6SwingBody;
	NxActorDesc d6SwingActorDesc;
	d6SwingActorDesc.body = &d6SwingBody;
	d6SwingActorDesc.density = 1.0f;
	d6SwingActorDesc.globalPose.t = NxVec3(8.0f, 2.0f, 0.0f);
	const NxReal d6SwingStartRotation[9] = {
		0.5f, 0.0f, 0.8660254f,
		0.0f, 1.0f, 0.0f,
		-0.8660254f, 0.0f, 0.5f };
	d6SwingActorDesc.globalPose.M.setRowMajor(d6SwingStartRotation);
	d6SwingActorDesc.shapes.pushBack(&forceSphere);
	NxActor* d6SwingActor = d6SwingScene->createActor(d6SwingActorDesc);
	if(!d6SwingActor) return nxFail("D6 swing-limit actor creation failed");
	NxD6JointDesc d6SwingDesc;
	d6SwingDesc.setToDefault();
	d6SwingDesc.actor[0] = d6SwingActor;
	d6SwingDesc.actor[1] = 0;
	d6SwingDesc.swing1Motion = NX_D6JOINT_MOTION_LIMITED;
	d6SwingDesc.swing1Limit.value = 0.2f;
	d6SwingDesc.projectionDistance = 0.0f;
	d6SwingDesc.projectionAngle = 0.0f;
	d6SwingDesc.projectionMode = NX_JPM_NONE;
	setGlobalAnchor(d6SwingDesc, NxVec3(8.0f, 2.0f, 0.0f));
	if(!d6SwingScene->createJoint(d6SwingDesc))
		return nxFail("D6 swing-limit joint creation failed");
	for(unsigned step = 0; step != 4; ++step)
		{
		d6SwingScene->simulate(0.02f);
		const bool ready = d6SwingScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = d6SwingScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("D6 swing-limit simulation result was not ready and fetched");
		}
	const NxQuat d6SwingOrientation = d6SwingActor->getGlobalOrientationQuatVal();
	const NxVec3 d6SwingAngularVelocity = d6SwingActor->getAngularVelocityVal();
	printf("simulation d6-swing-limit final orientation=%08x.%08x.%08x.%08x angular=%08x.%08x.%08x\n",
		nxFloatBits(d6SwingOrientation.x), nxFloatBits(d6SwingOrientation.y),
		nxFloatBits(d6SwingOrientation.z), nxFloatBits(d6SwingOrientation.w),
		nxFloatBits(d6SwingAngularVelocity.x), nxFloatBits(d6SwingAngularVelocity.y),
		nxFloatBits(d6SwingAngularVelocity.z));
	sdk->releaseScene(*d6SwingScene);

	// Exercise two different joint solver kinds in one connected island: a
	// fixed anchor on the first body and a minimum-distance row to the second.
	NxSceneDesc mixedJointSceneDesc;
	mixedJointSceneDesc.setToDefault();
	mixedJointSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* mixedJointScene = sdk->createScene(mixedJointSceneDesc);
	if(!mixedJointScene) return nxFail("mixed-joint scene creation failed");
	mixedJointScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxSphereShapeDesc mixedJointSphere;
	mixedJointSphere.radius = 0.1f;
	NxBodyDesc mixedJointBody0;
	NxActorDesc mixedJointActorDesc0;
	mixedJointActorDesc0.body = &mixedJointBody0;
	mixedJointActorDesc0.density = 1.0f;
	mixedJointActorDesc0.globalPose.t = NxVec3(16.0f, 6.0f, 0.0f);
	mixedJointActorDesc0.shapes.pushBack(&mixedJointSphere);
	NxActor* const mixedJointActor0 = mixedJointScene->createActor(mixedJointActorDesc0);
	NxBodyDesc mixedJointBody1;
	NxActorDesc mixedJointActorDesc1;
	mixedJointActorDesc1.body = &mixedJointBody1;
	mixedJointActorDesc1.density = 1.0f;
	mixedJointActorDesc1.globalPose.t = NxVec3(18.0f, 6.0f, 0.0f);
	mixedJointActorDesc1.shapes.pushBack(&mixedJointSphere);
	NxActor* const mixedJointActor1 = mixedJointScene->createActor(mixedJointActorDesc1);
	if(!mixedJointActor0 || !mixedJointActor1)
		return nxFail("mixed-joint actor creation failed");
	NxFixedJointDesc mixedFixedDesc;
	mixedFixedDesc.setToDefault();
	mixedFixedDesc.actor[0] = mixedJointActor0;
	mixedFixedDesc.actor[1] = 0;
	setGlobalAnchor(mixedFixedDesc, NxVec3(16.0f, 6.0f, 0.0f));
	setGlobalAxis(mixedFixedDesc, NxVec3(0.0f, 0.0f, 1.0f));
	if(!mixedJointScene->createJoint(mixedFixedDesc))
		return nxFail("mixed-joint fixed constraint creation failed");
	NxDistanceJointDesc mixedDistanceDesc;
	mixedDistanceDesc.setToDefault(false);
	mixedDistanceDesc.actor[0] = mixedJointActor1;
	mixedDistanceDesc.actor[1] = mixedJointActor0;
	setGlobalAnchor(mixedDistanceDesc, NxVec3(16.0f, 6.0f, 0.0f));
	mixedDistanceDesc.minDistance = 1.0f;
	mixedDistanceDesc.maxDistance = 1.0f;
	mixedDistanceDesc.flags = NX_DJF_MIN_DISTANCE_ENABLED | NX_DJF_MAX_DISTANCE_ENABLED;
	if(!mixedJointScene->createJoint(mixedDistanceDesc))
		return nxFail("mixed-joint distance constraint creation failed");
	for(unsigned step = 0; step != 12; ++step)
		{
		mixedJointScene->simulate(0.02f);
		const bool ready = mixedJointScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = mixedJointScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("mixed-joint simulation result was not ready and fetched");
		char stage0[24];
		char stage1[24];
		sprintf_s(stage0, "jointmix%u_0", step);
		sprintf_s(stage1, "jointmix%u_1", step);
		nxPrintActorState(stage0, *mixedJointActor0);
		nxPrintActorState(stage1, *mixedJointActor1);
		}
	printf("simulation solver-interaction fixed-distance steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*mixedJointScene);

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
	const NxU32 generatedPairCount = reportScene->getNbPairs();
	NxPairFlag generatedPairs[64];
	const bool generatedPairArray = reportScene->getPairFlagArray(generatedPairs, generatedPairCount);
	printf("simulation generated-contact summary flags=%08x calls=%u events=%08x ready=%u fetched=%u pairs=%u pair_array=%u",
		reportFlags, generatedContactReport.calls, generatedContactReport.events,
		reportReady, reportFetched, generatedPairCount, generatedPairArray);
	for(NxU32 pairIndex = 0; generatedPairArray && pairIndex < generatedPairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			generatedPairs[pairIndex].isActorPair(), generatedPairs[pairIndex].flags);
	printf("\n");
	reportScene->setActorPairFlags(*reportGround, *reportDynamic,
		NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	const NxU32 notifyConfiguredPairCount = reportScene->getNbPairs();
	NxPairFlag notifyConfiguredPairs[64];
	const bool notifyConfiguredPairArray = reportScene->getPairFlagArray(notifyConfiguredPairs, notifyConfiguredPairCount);
	printf("simulation configured-pair-flags pairs=%u pair_array=%u",
		notifyConfiguredPairCount, notifyConfiguredPairArray);
	for(NxU32 pairIndex = 0; notifyConfiguredPairArray && pairIndex < notifyConfiguredPairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			notifyConfiguredPairs[pairIndex].isActorPair(), notifyConfiguredPairs[pairIndex].flags);
	printf("\n");
	reportScene->setActorPairFlags(*reportGround, *reportDynamic, NX_IGNORE_PAIR);
	const NxU32 configuredPairCount = reportScene->getNbPairs();
	NxPairFlag configuredPairs[64];
	const bool configuredPairArray = reportScene->getPairFlagArray(configuredPairs, configuredPairCount);
	printf("simulation configured-pair-flags count=%u array=%u", configuredPairCount, configuredPairArray);
	for(NxU32 pairIndex = 0; configuredPairArray && pairIndex < configuredPairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			configuredPairs[pairIndex].isActorPair(), configuredPairs[pairIndex].flags);
	printf("\n");
	reportScene->setActorPairFlags(*reportGround, *reportDynamic, NX_NOTIFY_ON_TOUCH);
	const NxU32 recordPairCount = reportScene->getNbPairs();
	NxPairFlag recordPairs[64];
	const bool recordPairArray = reportScene->getPairFlagArray(recordPairs, recordPairCount);
	printf("simulation record-pair-flags count=%u array=%u", recordPairCount, recordPairArray);
	for(NxU32 pairIndex = 0; recordPairArray && pairIndex < recordPairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			recordPairs[pairIndex].isActorPair(), recordPairs[pairIndex].flags);
	printf("\n");
	NxShape** const reportGroundShapes = reportGround->getShapes();
	NxShape** const reportDynamicShapes = reportDynamic->getShapes();
	if(!reportGroundShapes || !reportDynamicShapes || !reportGroundShapes[0] || !reportDynamicShapes[0])
		return nxFail("contact-report shapes unavailable for shape-pair flags");
	NxShape& reportGroundShape = *reportGroundShapes[0];
	NxShape& reportDynamicShape = *reportDynamicShapes[0];
	reportScene->setShapePairFlags(reportGroundShape, reportDynamicShape, NX_IGNORE_PAIR);
	const NxU32 configuredShapePairFlags = reportScene->getShapePairFlags(reportGroundShape, reportDynamicShape);
	const NxU32 configuredShapePairCount = reportScene->getNbPairs();
	NxPairFlag configuredShapePairs[64];
	const bool configuredShapePairArray = reportScene->getPairFlagArray(configuredShapePairs, configuredShapePairCount);
	printf("simulation configured-shape-pair-flags flags=%08x count=%u array=%u",
		configuredShapePairFlags, configuredShapePairCount, configuredShapePairArray);
	for(NxU32 pairIndex = 0; configuredShapePairArray && pairIndex < configuredShapePairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			configuredShapePairs[pairIndex].isActorPair(), configuredShapePairs[pairIndex].flags);
	printf("\n");
	reportScene->setShapePairFlags(reportGroundShape, reportDynamicShape, NX_NOTIFY_ON_TOUCH);
	const NxU32 recordShapePairFlags = reportScene->getShapePairFlags(reportGroundShape, reportDynamicShape);
	printf("simulation record-shape-pair-flags flags=%08x\n", recordShapePairFlags);
	reportScene->setShapePairFlags(reportGroundShape, reportDynamicShape, 0);
	printf("simulation cleared-shape-pair-flags flags=%08x\n",
		reportScene->getShapePairFlags(reportGroundShape, reportDynamicShape));
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

	// fetchResults must retain buffered contact reports while no user callback
	// is installed. Installing the listener before the next fetch must deliver
	// the pending record exactly once. The oracle's callback dispatcher only
	// resets the queue when it has a listener; a second unconditional drain would
	// silently discard this record.
	NxSceneDesc deferredContactSceneDesc;
	deferredContactSceneDesc.setToDefault();
	NxScene* deferredContactScene = sdk->createScene(deferredContactSceneDesc);
	if(!deferredContactScene)
		return nxFail("deferred contact-report scene creation failed");
	unsigned char deferredContactWrapper[0x28];
	memcpy(deferredContactWrapper, deferredContactScene, sizeof(deferredContactWrapper));
	unsigned char* const deferredContactInternal = *reinterpret_cast<unsigned char**>(
		deferredContactWrapper + 0x24);
	unsigned char deferredContactQueue[0x2c];
	memset(deferredContactQueue, 0, sizeof(deferredContactQueue));
	NxActor* const deferredExpectedActor0 = reinterpret_cast<NxActor*>(&publicShapeTokens[0]);
	NxActor* const deferredExpectedActor1 = reinterpret_cast<NxActor*>(&publicShapeTokens[1]);
	*reinterpret_cast<NxActor**>(deferredContactQueue + 0x00) = deferredExpectedActor0;
	*reinterpret_cast<NxActor**>(deferredContactQueue + 0x04) = deferredExpectedActor1;
	*reinterpret_cast<NxU32*>(deferredContactQueue + 0x0c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(deferredContactQueue + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(deferredContactQueue + 0x14) = nxFloatBits(3.0f);
	*reinterpret_cast<NxU32*>(deferredContactQueue + 0x28) = NX_NOTIFY_ON_TOUCH;
	void** const deferredContactBeginField = reinterpret_cast<void**>(
		deferredContactInternal + 0x60c);
	void** const deferredContactEndField = reinterpret_cast<void**>(
		deferredContactInternal + 0x610);
	void** const deferredContactCapacityField = reinterpret_cast<void**>(
		deferredContactInternal + 0x614);
	void* const deferredSavedBegin = *deferredContactBeginField;
	void* const deferredSavedEnd = *deferredContactEndField;
	void* const deferredSavedCapacity = *deferredContactCapacityField;
	*deferredContactBeginField = deferredContactQueue;
	*deferredContactEndField = deferredContactQueue + sizeof(deferredContactQueue);
	*deferredContactCapacityField = deferredContactQueue + sizeof(deferredContactQueue);
	const bool deferredFirstFetch = deferredContactScene->fetchResults(
		static_cast<NxSimulationStatus>(0), false);
	const bool deferredPendingAfterFirstFetch =
		*deferredContactEndField == deferredContactQueue + sizeof(deferredContactQueue);
	NxSimulationContactReport deferredContactReport(
		deferredExpectedActor0, deferredExpectedActor1);
	deferredContactScene->setUserContactReport(&deferredContactReport);
	const bool deferredSecondFetch = deferredContactScene->fetchResults(
		static_cast<NxSimulationStatus>(0), false);
	printf("simulation deferred-contact-report first=%u pending=%u second=%u calls=%u events=%08x\n",
		deferredFirstFetch, deferredPendingAfterFirstFetch, deferredSecondFetch,
		deferredContactReport.calls, deferredContactReport.events);
	*deferredContactBeginField = deferredSavedBegin;
	*deferredContactEndField = deferredSavedEnd;
	*deferredContactCapacityField = deferredSavedCapacity;
	deferredContactFailed = !deferredFirstFetch || !deferredPendingAfterFirstFetch || !deferredSecondFetch
		|| deferredContactReport.calls != 1
		|| deferredContactReport.events != NX_NOTIFY_ON_TOUCH;
	if(deferredContactFailed)
		printf("FAIL deferred contact report was not retained and delivered exactly once\n");
	sdk->releaseScene(*deferredContactScene);

	sdk->releaseScene(*scene);

	{
	// Exercise contact generation, reporting and response through the same
	// public scene step/result path. Keep this in its own scene so the 1,000-step
	// gravity soak above cannot affect the collision fixture.
	NxSimulationContactStreamReport contactReport;
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
	NxSimulationContactStreamReport boxContactReport;
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
	NxSimulationContactStreamReport spherePairReport;
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
	NxSimulationContactStreamReport frictionReport;
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
	}

	// The shipped SDK exports the fluid API while this build disables its
	// implementation. Keep the deterministic unsupported contract covered and
	// release this scene without simulating it: the pinned manager's disabled
	// path leaves fields uninitialised that its later step path reads.
	NxSceneDesc fluidSceneDesc;
	fluidSceneDesc.setToDefault();
	NxScene* const fluidScene = sdk->createScene(fluidSceneDesc);
	if(!fluidScene)
		return nxFail("fluid unsupported scene creation failed");
	const unsigned fluidErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	const NxU32 emptyFluidCount = fluidScene->getNbFluids();
	const bool emptyFluidList = fluidScene->getFluids() != 0;
	NxFluidDesc fluidDesc;
	fluidDesc.setToDefault();
	NxFluid* const createdFluid = fluidScene->createFluid(fluidDesc);
	const NxU32 createdFluidCount = fluidScene->getNbFluids();
	const bool createdFluidList = fluidScene->getFluids() != 0;
	unsigned char* const fluidSceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(fluidScene) + 0x24);
	const bool fluidManagerCreated = *reinterpret_cast<void**>(fluidSceneInternal + 0x61c) != 0;
	printf("simulation fluid unsupported create=%u manager=%u empty=%u.%u created=%u.%u errors=%u code=%u line=%d file=%s message=%s\n",
		createdFluid != 0, fluidManagerCreated ? 1u : 0u,
		emptyFluidCount, emptyFluidList ? 1u : 0u,
		createdFluidCount, createdFluidList ? 1u : 0u,
		simulationOutput.errors - fluidErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	unsigned char* const enabledValidationFluidManager = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(fluidSceneInternal + 0x61c));
	if(!enabledValidationFluidManager)
		return nxFail("fluid descriptor validation manager was not created");
	enabledValidationFluidManager[0x2b] = 1;
	NxFluidDesc invalidFluidDesc;
	invalidFluidDesc.setToDefault();
	invalidFluidDesc.restDensity = 0.0f;
	const unsigned fluidValidationErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	NxFluid* const invalidFluid = fluidScene->createFluid(invalidFluidDesc);
	enabledValidationFluidManager[0x2b] = 0;
	printf("simulation fluid enabled-invalid result=%u errors=%u code=%u line=%d file=%s message=%s\n",
		invalidFluid != 0,
		simulationOutput.errors - fluidValidationErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	unsigned char fluidStorage[0x40] = {};
	unsigned char fluidIdentity = 0;
	*reinterpret_cast<void**>(fluidStorage + 0x14) = &fluidIdentity;
	const unsigned fluidReleaseErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	fluidScene->releaseFluid(*reinterpret_cast<NxFluid*>(fluidStorage));
	const bool fluidManagerReleased = *reinterpret_cast<void**>(fluidSceneInternal + 0x61c) == 0;
	printf("simulation fluid unsupported release manager-cleared=%u errors=%u code=%u line=%d file=%s message=%s\n",
		fluidManagerReleased ? 1u : 0u,
		simulationOutput.errors - fluidReleaseErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	unsigned char fluidCallbackMarker = 0;
	NxUserFluidContactReport* const fluidCallback =
		reinterpret_cast<NxUserFluidContactReport*>(&fluidCallbackMarker);
	const unsigned fluidCallbackErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	fluidScene->setUserFluidContactReport(fluidCallback);
	printf("simulation fluid contact-report-set errors=%u code=%u line=%d file=%s message=%s\n",
		simulationOutput.errors - fluidCallbackErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	const unsigned fluidCallbackGetErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	NxUserFluidContactReport* const returnedFluidCallback = fluidScene->getUserFluidContactReport();
	printf("simulation fluid contact-report-get stored=%u errors=%u code=%u line=%d file=%s message=%s\n",
		returnedFluidCallback == fluidCallback ? 1u : 0u,
		simulationOutput.errors - fluidCallbackGetErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	NxImplicitMeshDesc implicitMeshDesc;
	implicitMeshDesc.setToDefault();
	const unsigned implicitCreateErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	NxImplicitMesh* const createdImplicitMesh = fluidScene->createImplicitMesh(implicitMeshDesc);
	printf("simulation implicit-mesh-create created=%u errors=%u code=%u line=%d file=%s message=%s\n",
		createdImplicitMesh != 0 ? 1u : 0u,
		simulationOutput.errors - implicitCreateErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	unsigned char implicitMeshMarker = 0;
	const unsigned implicitReleaseErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	fluidScene->releaseImplicitMesh(*reinterpret_cast<NxImplicitMesh*>(&implicitMeshMarker));
	printf("simulation implicit-mesh-release errors=%u code=%u line=%d file=%s message=%s\n",
		simulationOutput.errors - implicitReleaseErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	const unsigned implicitCountErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	const NxU32 implicitMeshCount = fluidScene->getNbImplicitMeshes();
	printf("simulation implicit-mesh-count count=%u errors=%u code=%u line=%d file=%s message=%s\n",
		implicitMeshCount,
		simulationOutput.errors - implicitCountErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	const unsigned implicitListErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	NxImplicitMesh** const implicitMeshes = fluidScene->getImplicitMeshes();
	printf("simulation implicit-mesh-list present=%u errors=%u code=%u line=%d file=%s message=%s\n",
		implicitMeshes != 0 ? 1u : 0u,
		simulationOutput.errors - implicitListErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	sdk->releaseScene(*fluidScene);
	// The disabled FluidManager is still stepped by the scene scheduler. Pin
	// its uninitialised extension dirty flag to the false branch on both paired
	// processes so this fixture measures the manager warnings without invoking
	// extension callbacks that the shipped installation does not provide.
	NxScene* const fluidStepScene = sdk->createScene(fluidSceneDesc);
	if(!fluidStepScene)
		return nxFail("fluid manager step scene creation failed");
	fluidStepScene->setTiming(1.0f / 60.0f, 4, NX_TIMESTEP_FIXED);
	if(fluidStepScene->createFluid(fluidDesc) != 0)
		return nxFail("disabled fluid manager unexpectedly created a fluid");
	unsigned char* const fluidStepSceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(fluidStepScene) + 0x24);
	unsigned char* const fluidStepManager = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(fluidStepSceneInternal + 0x61c));
	if(!fluidStepManager)
		return nxFail("fluid manager step fixture has no manager");
	fluidStepManager[0x28] = 0;
	simulationOutput.resetLast();
	const unsigned fluidStepErrorsBefore = simulationOutput.errors;
	bool fluidStepReady = true;
	bool fluidStepFetched = true;
	for(unsigned step = 0; step < 2; ++step)
		{
		fluidStepScene->simulate(1.0f / 60.0f);
		fluidStepReady = fluidStepReady &&
			fluidStepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		fluidStepFetched = fluidStepFetched &&
			fluidStepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	printf("simulation fluid manager-step ready=%u fetched=%u frames=2 errors=%u code=%u line=%d file=%s message=%s\n",
		fluidStepReady ? 1u : 0u, fluidStepFetched ? 1u : 0u,
		simulationOutput.errors - fluidStepErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	sdk->releaseScene(*fluidStepScene);
	// Seed the manager's two parallel fluid arrays with a concrete fake fluid
	// so release exercises its swap-removal and scalar-deleting dispatch, even
	// though the shipped build cannot create a backend fluid itself.
	NxScene* const fluidArrayScene = sdk->createScene(fluidSceneDesc);
	if(!fluidArrayScene)
		return nxFail("fluid array scene creation failed");
	fluidArrayScene->createFluid(fluidDesc);
	unsigned char* const fluidArraySceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(fluidArrayScene) + 0x24);
	unsigned char* const fluidArrayManager = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(fluidArraySceneInternal + 0x61c));
	if(!fluidArrayManager)
		return nxFail("fluid array manager was not installed");
	NxSphereShapeDesc fluidNotifyShape;
	fluidNotifyShape.setToDefault();
	fluidNotifyShape.radius = 0.5f;
	NxActorDesc fluidNotifyActorDesc;
	fluidNotifyActorDesc.setToDefault();
	fluidNotifyActorDesc.shapes.pushBack(&fluidNotifyShape);
	simulationOutput.resetLast();
	const unsigned fluidNotifyCreateErrorsBefore = simulationOutput.errors;
	NxActor* const fluidNotifyActor = fluidArrayScene->createActor(fluidNotifyActorDesc);
	printf("simulation fluid manager actor-created present=%u errors=%u code=%u line=%d file=%s message=%s\n",
		fluidNotifyActor ? 1u : 0u,
		simulationOutput.errors - fluidNotifyCreateErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	if(!fluidNotifyActor)
		return nxFail("fluid manager actor-create notification fixture failed");
	simulationOutput.resetLast();
	const unsigned fluidNotifyReleaseErrorsBefore = simulationOutput.errors;
	fluidArrayScene->releaseActor(*fluidNotifyActor);
	printf("simulation fluid manager actor-released errors=%u code=%u line=%d file=%s message=%s\n",
		simulationOutput.errors - fluidNotifyReleaseErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	void* fluidDeletingVtable[] = {
		reinterpret_cast<void*>(nxSimulationFluidDeletingDestructor)
		};
	unsigned char fluidInternalA[0x20] = {};
	unsigned char fluidInternalB[0x20] = {};
	*reinterpret_cast<void***>(fluidInternalA) = fluidDeletingVtable;
	*reinterpret_cast<void***>(fluidInternalB) = fluidDeletingVtable;
	unsigned fluidPointers[] = {
		reinterpret_cast<unsigned>(fluidInternalA),
		reinterpret_cast<unsigned>(fluidInternalB)
		};
	unsigned secondaryPointers[] = { 0x11111111u, 0x22222222u };
	*reinterpret_cast<unsigned*>(fluidArrayManager + 4) = reinterpret_cast<unsigned>(fluidPointers);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 8) = reinterpret_cast<unsigned>(fluidPointers + 2);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0xc) = reinterpret_cast<unsigned>(fluidPointers + 2);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x14) = reinterpret_cast<unsigned>(secondaryPointers);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x18) = reinterpret_cast<unsigned>(secondaryPointers + 2);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x1c) = reinterpret_cast<unsigned>(secondaryPointers + 2);
	unsigned char publicFluidStorage[0x40] = {};
	*reinterpret_cast<void**>(publicFluidStorage + 0x14) = fluidInternalA;
	gNxSimulationFluidDestructorCalls = 0;
	gNxSimulationFluidDestructorFlags = 0;
	gNxSimulationFluidDestructorObject = 0;
	fluidArrayScene->releaseFluid(*reinterpret_cast<NxFluid*>(publicFluidStorage));
	const unsigned fluidArrayRemaining =
		(*reinterpret_cast<unsigned*>(fluidArrayManager + 8) -
		 *reinterpret_cast<unsigned*>(fluidArrayManager + 4)) >> 2;
	const unsigned secondaryArrayRemaining =
		(*reinterpret_cast<unsigned*>(fluidArrayManager + 0x18) -
		 *reinterpret_cast<unsigned*>(fluidArrayManager + 0x14)) >> 2;
	printf("simulation fluid array-release remaining=%u secondary=%u swapped=%u.%u destructor=%u flags=%u target=%u\n",
		fluidArrayRemaining, secondaryArrayRemaining,
		fluidPointers[0] == reinterpret_cast<unsigned>(fluidInternalB) ? 1u : 0u,
		secondaryPointers[0] == 0x22222222u ? 1u : 0u,
		gNxSimulationFluidDestructorCalls, gNxSimulationFluidDestructorFlags,
		gNxSimulationFluidDestructorObject == fluidInternalA ? 1u : 0u);
	// The two arrays above are stack fixtures rather than allocator-owned SDK
	// storage. Empty their headers before releasing the scene so the manager
	// destructor does not try to free those test buffers.
	*reinterpret_cast<unsigned*>(fluidArrayManager + 4) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 8) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0xc) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x14) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x18) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x1c) = 0;
	sdk->releaseScene(*fluidArrayScene);
	// The manager is an internal C++ object. Exercise its first vtable slot
	// through the pinned scalar-deleting destructor instead of treating the
	// manager as a byte buffer only.
	NxScene* const fluidVtableScene = sdk->createScene(fluidSceneDesc);
	if(!fluidVtableScene)
		return nxFail("fluid vtable scene creation failed");
	fluidVtableScene->createFluid(fluidDesc);
	unsigned char* const fluidVtableSceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(fluidVtableScene) + 0x24);
	void* const fluidManager = *reinterpret_cast<void**>(fluidVtableSceneInternal + 0x61c);
	if(!fluidManager)
		return nxFail("fluid manager was not installed by createFluid");
	unsigned char* const fluidManagerBytes = static_cast<unsigned char*>(fluidManager);
	const bool fluidManagerOwnerMatches =
		*reinterpret_cast<void**>(fluidManagerBytes + 0x24) == fluidVtableSceneInternal;
	const bool fluidManagerArraysEmpty =
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 4) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 8) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0xc) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x14) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x18) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x1c) == 0;
	printf("simulation fluid manager-constructor owner=%u arrays-empty=%u initialized=%u extension=%u available=%u\n",
		fluidManagerOwnerMatches ? 1u : 0u, fluidManagerArraysEmpty ? 1u : 0u,
		fluidManagerBytes[0x29], fluidManagerBytes[0x2a], fluidManagerBytes[0x2b]);
	void** const fluidManagerVtable = fluidManager ? *reinterpret_cast<void***>(fluidManager) : 0;
	if(!fluidManagerVtable || !fluidManagerVtable[0])
		return nxFail("fluid manager vtable destructor missing");
	typedef void* (__thiscall *FluidManagerDeletingDestructorFn)(void*, unsigned char);
	FluidManagerDeletingDestructorFn fluidManagerDeletingDestructor =
		reinterpret_cast<FluidManagerDeletingDestructorFn>(fluidManagerVtable[0]);
	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	CreateFoundationSDKFn createFoundation = foundation ?
		reinterpret_cast<CreateFoundationSDKFn>(GetProcAddress(foundation, "NxCreateFoundationSDK")) : 0;
	NxFoundationSDK* foundationSDK = createFoundation ?
		createFoundation(NX_FOUNDATION_SDK_VERSION, &simulationOutput, 0) : 0;
	if(!foundationSDK)
		return nxFail("Foundation allocator access failed for fluid manager teardown");
	NxUserAllocator* foundationAllocator = &foundationSDK->getAllocator();
	unsigned* managerFluids = static_cast<unsigned*>(
		foundationAllocator->malloc(2 * sizeof(unsigned), NX_MEMORY_PERSISTENT));
	unsigned* managerSecondary = static_cast<unsigned*>(
		foundationAllocator->malloc(sizeof(unsigned), NX_MEMORY_PERSISTENT));
	if(!managerFluids || !managerSecondary)
		return nxFail("fluid manager teardown arrays allocation failed");
	unsigned char managerFluidA[0x20] = {};
	unsigned char managerFluidB[0x20] = {};
	*reinterpret_cast<void***>(managerFluidA) = fluidDeletingVtable;
	*reinterpret_cast<void***>(managerFluidB) = fluidDeletingVtable;
	managerFluids[0] = reinterpret_cast<unsigned>(managerFluidA);
	managerFluids[1] = reinterpret_cast<unsigned>(managerFluidB);
	managerSecondary[0] = 0;
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 4) = reinterpret_cast<unsigned>(managerFluids);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 8) = reinterpret_cast<unsigned>(managerFluids + 2);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 0xc) = reinterpret_cast<unsigned>(managerFluids + 2);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x14) = reinterpret_cast<unsigned>(managerSecondary);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x18) = reinterpret_cast<unsigned>(managerSecondary + 1);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x1c) = reinterpret_cast<unsigned>(managerSecondary + 1);
	gNxSimulationFluidDestructorCalls = 0;
	gNxSimulationFluidDestructorFlags = 0;
	gNxSimulationFluidDestructorObject = 0;
	fluidManagerDeletingDestructor(fluidManager, 0);
	const bool managerArraysCleared =
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 4) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 8) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0xc) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x14) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x18) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x1c) == 0;
	const bool populatedManagerDestroyed = gNxSimulationFluidDestructorCalls == 2 &&
		gNxSimulationFluidDestructorFlags == 1 &&
		gNxSimulationFluidDestructorObject == managerFluidB && managerArraysCleared;
	printf("simulation fluid manager-populated-destructor calls=%u flags=%u target=%u arrays-cleared=%u\n",
		gNxSimulationFluidDestructorCalls, gNxSimulationFluidDestructorFlags,
		gNxSimulationFluidDestructorObject == managerFluidB ? 1u : 0u,
		managerArraysCleared ? 1u : 0u);
	if(!populatedManagerDestroyed)
		return nxFail("fluid manager populated deleting destructor mismatch");
	fluidManagerDeletingDestructor(fluidManager, 1);
	*reinterpret_cast<void**>(fluidVtableSceneInternal + 0x61c) = 0;
	printf("simulation fluid manager-vtable deleting-destructor=1\n");
	sdk->releaseScene(*fluidVtableScene);
	// NxScene::createController forwards to the Scene-owned controller list in
	// the pinned DLL. NxControllerDesc is intentionally incomplete in this SDK
	// header set, so supply its recovered byte layout without changing public
	// headers. A zero type word selects the descriptor path accepted by the
	// oracle; the three dimensions at +0x30 are a half-meter box controller.
	NxSceneDesc controllerSceneDesc;
	controllerSceneDesc.setToDefault();
	NxScene* const controllerScene = sdk->createScene(controllerSceneDesc);
	if(!controllerScene)
		return nxFail("controller scene creation failed");
	const NxU32 controllerSceneActorCountBefore = controllerScene->getNbActors();
	alignas(4) unsigned char controllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(controllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(controllerDescStorage + 0x38) = nxFloatBits(0.5f);
	alignas(4) unsigned char rejectedControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(rejectedControllerDescStorage + 8) = 1;
	NxController* const rejectedController = controllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(rejectedControllerDescStorage));
	printf("simulation controller-create rejected=%u actors=%u\n",
		rejectedController == 0, controllerScene->getNbActors());
	controllerCreateFailed = rejectedController != 0 ||
		controllerScene->getNbActors() != controllerSceneActorCountBefore;
	NxController* const controller = controllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(controllerDescStorage));
	alignas(4) unsigned char secondControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(secondControllerDescStorage + 0x0c) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(secondControllerDescStorage + 0x30) = nxFloatBits(0.25f);
	*reinterpret_cast<NxU32*>(secondControllerDescStorage + 0x34) = nxFloatBits(0.75f);
	*reinterpret_cast<NxU32*>(secondControllerDescStorage + 0x38) = nxFloatBits(0.25f);
	NxController* const secondController = controllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(secondControllerDescStorage));
	const NxU32 controllerSceneActorCountCreated = controllerScene->getNbActors();
	printf("simulation controller-create first=%u second=%u actors-before=%u actors-created=%u\n",
		controller != 0, secondController != 0, controllerSceneActorCountBefore,
		controllerSceneActorCountCreated);
	controllerCreateFailed = controllerCreateFailed || controller == 0 || secondController == 0 ||
		controllerSceneActorCountCreated != controllerSceneActorCountBefore + 2;
	if(controller && secondController)
		{
		const NxVec3& firstPosition =
			reinterpret_cast<NxControllerProbe*>(controller)->getPosition();
		const NxVec3& secondPosition =
			reinterpret_cast<NxControllerProbe*>(secondController)->getPosition();
		printf("simulation controller-position first=%08x.%08x.%08x second=%08x.%08x.%08x\n",
			nxFloatBits(firstPosition.x), nxFloatBits(firstPosition.y), nxFloatBits(firstPosition.z),
			nxFloatBits(secondPosition.x), nxFloatBits(secondPosition.y), nxFloatBits(secondPosition.z));
		controllerCreateFailed = controllerCreateFailed ||
			firstPosition.x != 0.0f || firstPosition.y != 0.0f || firstPosition.z != 0.0f ||
			secondPosition.x != 2.0f || secondPosition.y != 0.0f || secondPosition.z != 0.0f;
		NxU32 collisionFlags = 0xdeadbeef;
		const NxVec3 displacement(0.25f, 0.0f, 0.0f);
		reinterpret_cast<NxControllerProbe*>(controller)->move(
			displacement, 0xffffffff, 0.001f, collisionFlags);
		const NxVec3& movedPosition =
			reinterpret_cast<NxControllerProbe*>(controller)->getPosition();
		printf("simulation controller-move position=%08x.%08x.%08x flags=%08x\n",
			nxFloatBits(movedPosition.x), nxFloatBits(movedPosition.y),
			nxFloatBits(movedPosition.z), collisionFlags);
		controllerCreateFailed = controllerCreateFailed ||
			movedPosition.x != 0.25f || movedPosition.y != 0.0f ||
			movedPosition.z != 0.0f || collisionFlags != 0;
		NxVec3 actorPositionBeforeStep;
		NxActor** const controllerActorsBeforeStep = controllerScene->getActors();
		controllerActorsBeforeStep[controllerSceneActorCountBefore]->getGlobalPosition(actorPositionBeforeStep);
		printf("simulation controller-move actor-before-step=%08x.%08x.%08x\n",
			nxFloatBits(actorPositionBeforeStep.x), nxFloatBits(actorPositionBeforeStep.y),
			nxFloatBits(actorPositionBeforeStep.z));
		controllerCreateFailed = controllerCreateFailed || actorPositionBeforeStep.x != 0.0f ||
			actorPositionBeforeStep.y != 0.0f || actorPositionBeforeStep.z != 0.0f;
		}
	NxActor** const controllerActors = controllerScene->getActors();
	for(NxU32 index = 0; index != 2 && index < controllerSceneActorCountCreated; ++index)
		{
		NxVec3 actorPosition;
		controllerActors[controllerSceneActorCountBefore + index]->getGlobalPosition(actorPosition);
		NxShape** const actorShapes = controllerActors[controllerSceneActorCountBefore + index]->getShapes();
		if(!actorShapes || controllerActors[controllerSceneActorCountBefore + index]->getNbShapes() != 1)
			{
			controllerCreateFailed = true;
			printf("simulation controller-actor index=%u shape-layout-invalid\n", index);
			continue;
			}
		const NxVec3& actorDimensions = static_cast<NxBoxShape*>(actorShapes[0])->getDimensions();
		printf("simulation controller-actor index=%u position=%08x.%08x.%08x dimensions=%08x.%08x.%08x\n",
			index, nxFloatBits(actorPosition.x), nxFloatBits(actorPosition.y),
			nxFloatBits(actorPosition.z), nxFloatBits(actorDimensions.x),
			nxFloatBits(actorDimensions.y), nxFloatBits(actorDimensions.z));
		// The controller advances its exposed position immediately, while the
		// generated kinematic actor still exposes its pre-step global pose.
		const NxReal expectedX = index == 0 ? 0.0f : 2.0f;
		const NxU32 expectedHalfExtent = index == 0 ? 0x3f0ccccd : 0x3e8ccccd;
		const NxU32 expectedHeight = index == 0 ? 0x3f8ccccd : 0x3f533334;
		controllerCreateFailed = controllerCreateFailed ||
			actorPosition.x != expectedX || actorPosition.y != 0.0f || actorPosition.z != 0.0f ||
			nxFloatBits(actorDimensions.x) != expectedHalfExtent ||
			nxFloatBits(actorDimensions.y) != expectedHeight ||
			nxFloatBits(actorDimensions.z) != expectedHalfExtent;
		}
	// Releasing the first item exercises Scene::removeController's non-head walk;
	// releasing the second then checks removal of the remaining head.
	const unsigned controllerReleaseErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	if(controller)
		controllerScene->releaseController(*controller);
	const NxU32 controllerSceneActorCountAfterFirstRelease = controllerScene->getNbActors();
	const bool controllerListHasStaleNext = secondController &&
		*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(secondController) + 0x38) != 0;
	const unsigned controllerReleaseErrorsAfterFirst =
		simulationOutput.errors - controllerReleaseErrorsBefore;
	if(secondController)
		controllerScene->releaseController(*secondController);
	const NxU32 controllerSceneActorCountReleased = controllerScene->getNbActors();
	const unsigned controllerReleaseErrorsAfterSecond =
		simulationOutput.errors - controllerReleaseErrorsBefore;
	printf("simulation controller-release actors-after-first=%u actors-after=%u stale-next-after-first=%u errors-after-first=%u errors-after-second=%u\n",
		controllerSceneActorCountAfterFirstRelease, controllerSceneActorCountReleased,
		controllerListHasStaleNext ? 1u : 0u,
		controllerReleaseErrorsAfterFirst, controllerReleaseErrorsAfterSecond);
	// The pinned DLL leaves the generated actor registered when its controller
	// is released. Preserve and compare that observed behavior rather than
	// imposing a stronger cleanup contract than the oracle has.
	controllerCreateFailed = controllerCreateFailed ||
		controllerSceneActorCountAfterFirstRelease != controllerSceneActorCountCreated ||
		controllerSceneActorCountReleased != controllerSceneActorCountCreated ||
		controllerListHasStaleNext ||
		controllerReleaseErrorsAfterFirst != 0 || controllerReleaseErrorsAfterSecond != 0;
	sdk->releaseScene(*controllerScene);

	// Exercise the collision-aware controller sweep with one static obstacle.
	// The implementation change this test should catch is continuing through a
	// solid wall (or reporting no side collision) when the controller requests a
	// displacement longer than the available clear path.
	NxSceneDesc controllerObstacleSceneDesc;
	controllerObstacleSceneDesc.setToDefault();
	NxScene* const controllerObstacleScene = sdk->createScene(controllerObstacleSceneDesc);
	if(!controllerObstacleScene)
		return nxFail("controller obstacle scene creation failed");
	NxBoxShapeDesc controllerObstacleShape;
	controllerObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 1.0f);
	NxActorDesc controllerObstacleActorDesc;
	controllerObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	controllerObstacleActorDesc.shapes.pushBack(&controllerObstacleShape);
	NxActor* const controllerObstacleActor =
		controllerObstacleScene->createActor(controllerObstacleActorDesc);
	alignas(4) unsigned char obstacleControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(obstacleControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(obstacleControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(obstacleControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const obstacleController = controllerObstacleScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(obstacleControllerDescStorage));
	if(!controllerObstacleActor || !obstacleController)
		return nxFail("controller obstacle fixture setup failed");
	NxU32 obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 obstacleDisplacement(2.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(obstacleController)->move(
		obstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& obstacleControllerPosition =
		reinterpret_cast<NxControllerProbe*>(obstacleController)->getPosition();
	printf("simulation controller-obstacle position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(obstacleControllerPosition.x), nxFloatBits(obstacleControllerPosition.y),
		nxFloatBits(obstacleControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(obstacleControllerPosition.x) != 0x3f000000 ||
		nxFloatBits(obstacleControllerPosition.y) != 0 ||
		nxFloatBits(obstacleControllerPosition.z) != 0 || obstacleCollisionFlags != 4;
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(obstacleController)->move(
		obstacleDisplacement, 0, 0.001f, obstacleCollisionFlags);
	const NxVec3& filteredControllerPosition =
		reinterpret_cast<NxControllerProbe*>(obstacleController)->getPosition();
	printf("simulation controller-obstacle filtered position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(filteredControllerPosition.x), nxFloatBits(filteredControllerPosition.y),
		nxFloatBits(filteredControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(filteredControllerPosition.x) != 0x40200000 ||
		nxFloatBits(filteredControllerPosition.y) != 0 ||
		nxFloatBits(filteredControllerPosition.z) != 0 || obstacleCollisionFlags != 0;
	// A downward component keeps the controller grounded while it approaches a
	// low box. The descriptor enables a half-unit step with +Y as the up axis.
	NxSceneDesc stepControllerSceneDesc;
	stepControllerSceneDesc.setToDefault();
	NxScene* const stepControllerScene = sdk->createScene(stepControllerSceneDesc);
	if(!stepControllerScene)
		return nxFail("step controller scene creation failed");
	NxBoxShapeDesc stepFloorShape;
	stepFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc stepFloorActorDesc;
	stepFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	stepFloorActorDesc.shapes.pushBack(&stepFloorShape);
	NxActor* const stepFloorActor = stepControllerScene->createActor(stepFloorActorDesc);
	NxBoxShapeDesc stepObstacleShape;
	stepObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc stepObstacleActorDesc;
	stepObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	stepObstacleActorDesc.shapes.pushBack(&stepObstacleShape);
	NxActor* const stepObstacleActor = stepControllerScene->createActor(stepObstacleActorDesc);
	alignas(4) unsigned char stepControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x0c) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x20) = 0;
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	NxController* const stepController = stepControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(stepControllerDescStorage));
	if(!stepFloorActor || !stepObstacleActor || !stepController)
		return nxFail("step controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 stepDisplacement(2.0f, -0.5f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(stepController)->move(
		stepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& stepControllerPosition =
		reinterpret_cast<NxControllerProbe*>(stepController)->getPosition();
	printf("simulation controller-obstacle step position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(stepControllerPosition.x), nxFloatBits(stepControllerPosition.y),
		nxFloatBits(stepControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(stepControllerPosition.x) != 0x3f000000 ||
		nxFloatBits(stepControllerPosition.y) != 0x3f000000 ||
		nxFloatBits(stepControllerPosition.z) != 0 || obstacleCollisionFlags != 5;
	stepControllerScene->releaseController(*stepController);
	sdk->releaseScene(*stepControllerScene);
	// Isolate the grounded low-obstacle response from the earlier controller
	// fixture, using a fresh scene with no old controller actor.
	NxSceneDesc stepUpSceneDesc;
	stepUpSceneDesc.setToDefault();
	NxScene* const stepUpScene = sdk->createScene(stepUpSceneDesc);
	if(!stepUpScene)
		return nxFail("step-response controller scene creation failed");
	NxBoxShapeDesc stepUpFloorShape;
	stepUpFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc stepUpFloorActorDesc;
	stepUpFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	stepUpFloorActorDesc.shapes.pushBack(&stepUpFloorShape);
	NxActor* const stepUpFloorActor = stepUpScene->createActor(stepUpFloorActorDesc);
	NxBoxShapeDesc stepUpObstacleShape;
	stepUpObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc stepUpObstacleActorDesc;
	stepUpObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	stepUpObstacleActorDesc.shapes.pushBack(&stepUpObstacleShape);
	NxActor* const stepUpObstacleActor = stepUpScene->createActor(stepUpObstacleActorDesc);
	alignas(4) unsigned char stepUpControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x0c) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x14) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x20) = 0;
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x2c) = 0;
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const stepUpController = stepUpScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(stepUpControllerDescStorage));
	if(!stepUpFloorActor || !stepUpObstacleActor || !stepUpController)
		return nxFail("step-response controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 stepUpDisplacement(2.0f, -0.5f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(stepUpController)->move(
		stepUpDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& stepUpPosition = reinterpret_cast<NxControllerProbe*>(stepUpController)->getPosition();
	printf("simulation controller-obstacle step-offset-disabled position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(stepUpPosition.x), nxFloatBits(stepUpPosition.y),
		nxFloatBits(stepUpPosition.z), obstacleCollisionFlags);
	stepUpScene->releaseController(*stepUpController);
	sdk->releaseScene(*stepUpScene);
	// A controller touching the floor while sweeping into a low wall must
	// classify the zero-time vertical floor contact, not a side hit.
	NxSceneDesc groundedSweepSceneDesc;
	groundedSweepSceneDesc.setToDefault();
	NxScene* const groundedSweepScene = sdk->createScene(groundedSweepSceneDesc);
	if(!groundedSweepScene)
		return nxFail("grounded-sweep controller scene creation failed");
	NxBoxShapeDesc groundedSweepFloorShape;
	groundedSweepFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc groundedSweepFloorActorDesc;
	groundedSweepFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	groundedSweepFloorActorDesc.shapes.pushBack(&groundedSweepFloorShape);
	NxActor* const groundedSweepFloorActor = groundedSweepScene->createActor(groundedSweepFloorActorDesc);
	NxBoxShapeDesc groundedSweepObstacleShape;
	groundedSweepObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc groundedSweepObstacleActorDesc;
	groundedSweepObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	groundedSweepObstacleActorDesc.shapes.pushBack(&groundedSweepObstacleShape);
	NxActor* const groundedSweepObstacleActor = groundedSweepScene->createActor(groundedSweepObstacleActorDesc);
	alignas(4) unsigned char groundedSweepControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x0c) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x10) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x14) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x20) = 0;
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const groundedSweepController = groundedSweepScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(groundedSweepControllerDescStorage));
	if(!groundedSweepFloorActor || !groundedSweepObstacleActor || !groundedSweepController)
		return nxFail("grounded-sweep controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 groundedSweepDisplacement(2.0f, -0.1f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(groundedSweepController)->move(
		groundedSweepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& groundedSweepPosition = reinterpret_cast<NxControllerProbe*>(groundedSweepController)->getPosition();
	printf("simulation controller-obstacle grounded-sweep position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(groundedSweepPosition.x), nxFloatBits(groundedSweepPosition.y),
		nxFloatBits(groundedSweepPosition.z), obstacleCollisionFlags);
	groundedSweepScene->releaseController(*groundedSweepController);
	sdk->releaseScene(*groundedSweepScene);
	// Pin the controller's private up-axis and step-offset state from the public
	// descriptor, then retain a blocked obstacle move as the current behavior.
	NxSceneDesc successfulStepSceneDesc;
	successfulStepSceneDesc.setToDefault();
	NxScene* const successfulStepScene = sdk->createScene(successfulStepSceneDesc);
	if(!successfulStepScene)
		return nxFail("successful-step controller scene creation failed");
	NxBoxShapeDesc successfulStepFloorShape;
	successfulStepFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc successfulStepFloorActorDesc;
	successfulStepFloorActorDesc.globalPose.t = NxVec3(0.0f, -10.5f, 0.0f);
	successfulStepFloorActorDesc.shapes.pushBack(&successfulStepFloorShape);
	NxActor* const successfulStepFloorActor = successfulStepScene->createActor(
		successfulStepFloorActorDesc);
	NxBoxShapeDesc successfulStepObstacleShape;
	successfulStepObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc successfulStepObstacleActorDesc;
	successfulStepObstacleActorDesc.globalPose.t = NxVec3(2.5f, 0.1f, 0.0f);
	successfulStepObstacleActorDesc.shapes.pushBack(&successfulStepObstacleShape);
	NxActor* const successfulStepObstacleActor = successfulStepScene->createActor(
		successfulStepObstacleActorDesc);
	alignas(4) unsigned char successfulStepControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x10) = nxFloatBits(0.55f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const successfulStepController = successfulStepScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(successfulStepControllerDescStorage));
	if(!successfulStepFloorActor || !successfulStepObstacleActor || !successfulStepController)
		return nxFail("successful-step controller fixture setup failed");
	const unsigned char* const successfulStepControllerBytes =
		reinterpret_cast<const unsigned char*>(successfulStepController);
	printf("simulation controller-descriptor up-axis=%08x step-offset=%08x\n",
		*reinterpret_cast<const NxU32*>(successfulStepControllerBytes + 0x14),
		*reinterpret_cast<const NxU32*>(successfulStepControllerBytes + 0x20));
	controllerCreateFailed = controllerCreateFailed ||
		*reinterpret_cast<const NxU32*>(successfulStepControllerBytes + 0x14) != 1 ||
		*reinterpret_cast<const NxU32*>(successfulStepControllerBytes + 0x20) != nxFloatBits(0.5f);
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 successfulStepDisplacement(3.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(successfulStepController)->move(
		successfulStepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& successfulStepPosition =
		reinterpret_cast<NxControllerProbe*>(successfulStepController)->getPosition();
	printf("simulation controller-obstacle step-offset move position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(successfulStepPosition.x), nxFloatBits(successfulStepPosition.y),
		nxFloatBits(successfulStepPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(successfulStepPosition.x) != 0x3fc00000 ||
		nxFloatBits(successfulStepPosition.y) != 0x3f0ccccd ||
		nxFloatBits(successfulStepPosition.z) != 0 || obstacleCollisionFlags != 4;
	successfulStepScene->releaseController(*successfulStepController);
	sdk->releaseScene(*successfulStepScene);
	// A grounded +Y box controller requests horizontal and downward motion
	// toward a low box. The pinned oracle treats this as separate vertical and
	// horizontal probes and blocks at the near side; successful step-up remains
	// a separate, unverified path.
	NxSceneDesc stepOverSceneDesc;
	stepOverSceneDesc.setToDefault();
	NxScene* const stepOverScene = sdk->createScene(stepOverSceneDesc);
	if(!stepOverScene)
		return nxFail("step-over controller scene creation failed");
	NxBoxShapeDesc stepOverFloorShape;
	stepOverFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc stepOverFloorActorDesc;
	stepOverFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	stepOverFloorActorDesc.shapes.pushBack(&stepOverFloorShape);
	NxActor* const stepOverFloorActor = stepOverScene->createActor(stepOverFloorActorDesc);
	NxBoxShapeDesc stepOverObstacleShape;
	stepOverObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc stepOverObstacleActorDesc;
	stepOverObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	stepOverObstacleActorDesc.shapes.pushBack(&stepOverObstacleShape);
	NxActor* const stepOverObstacleActor = stepOverScene->createActor(stepOverObstacleActorDesc);
	alignas(4) unsigned char stepOverControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const stepOverController = stepOverScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(stepOverControllerDescStorage));
	if(!stepOverFloorActor || !stepOverObstacleActor || !stepOverController)
		return nxFail("step-over controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 stepOverDisplacement(3.0f, -0.5f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(stepOverController)->move(
		stepOverDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& stepOverPosition =
		reinterpret_cast<NxControllerProbe*>(stepOverController)->getPosition();
	printf("simulation controller-grounded-step-probe position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(stepOverPosition.x), nxFloatBits(stepOverPosition.y),
		nxFloatBits(stepOverPosition.z), obstacleCollisionFlags);
	stepOverScene->releaseController(*stepOverController);
	// Raise the stored threshold above a unit contact-normal component to pin
	// the high-threshold response after a downward sweep with step probing on.
	// Keep this controller in its own scene so generated controller bodies do
	// not become obstacles for one another in the candidate's broadphase path.
	alignas(4) unsigned char highThresholdControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x2c) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxSceneDesc highThresholdSceneDesc;
	highThresholdSceneDesc.setToDefault();
	NxScene* const highThresholdScene = sdk->createScene(highThresholdSceneDesc);
	if(!highThresholdScene)
		return nxFail("high-threshold controller scene creation failed");
	NxBoxShapeDesc highThresholdFloorShape;
	highThresholdFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc highThresholdFloorActorDesc;
	highThresholdFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	highThresholdFloorActorDesc.shapes.pushBack(&highThresholdFloorShape);
	NxActor* const highThresholdFloor = highThresholdScene->createActor(highThresholdFloorActorDesc);
	NxBoxShapeDesc highThresholdObstacleShape;
	highThresholdObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc highThresholdObstacleActorDesc;
	highThresholdObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	highThresholdObstacleActorDesc.shapes.pushBack(&highThresholdObstacleShape);
	NxActor* const highThresholdObstacle = highThresholdScene->createActor(
		highThresholdObstacleActorDesc);
	NxController* const highThresholdController = highThresholdScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(highThresholdControllerDescStorage));
	if(!highThresholdFloor || !highThresholdObstacle || !highThresholdController)
		return nxFail("high-threshold controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(highThresholdController)->move(
		stepOverDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& highThresholdPosition =
		reinterpret_cast<NxControllerProbe*>(highThresholdController)->getPosition();
	printf("simulation controller-grounded-high-threshold position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(highThresholdPosition.x), nxFloatBits(highThresholdPosition.y),
		nxFloatBits(highThresholdPosition.z), obstacleCollisionFlags);
	highThresholdScene->releaseController(*highThresholdController);
	sdk->releaseScene(*highThresholdScene);
	sdk->releaseScene(*stepOverScene);
	{
	// A controller already resting at floor height receives a tiny downward
	// probe while moving into a low box. The oracle reports a side hit only;
	// it does not add a down-collision flag from the floor contact.
	NxSceneDesc stepUpSceneDesc;
	stepUpSceneDesc.setToDefault();
	NxScene* const stepUpScene = sdk->createScene(stepUpSceneDesc);
	if(!stepUpScene)
		return nxFail("step-up controller scene creation failed");
	NxBoxShapeDesc stepUpFloorShape;
	stepUpFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc stepUpFloorActorDesc;
	stepUpFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	stepUpFloorActorDesc.shapes.pushBack(&stepUpFloorShape);
	NxActor* const stepUpFloorActor = stepUpScene->createActor(stepUpFloorActorDesc);
	NxBoxShapeDesc stepUpObstacleShape;
	stepUpObstacleShape.dimensions = NxVec3(0.5f, 0.05f, 1.0f);
	NxActorDesc stepUpObstacleActorDesc;
	stepUpObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.05f, 0.0f);
	stepUpObstacleActorDesc.shapes.pushBack(&stepUpObstacleShape);
	NxActor* const stepUpObstacleActor = stepUpScene->createActor(stepUpObstacleActorDesc);
	alignas(4) unsigned char stepUpControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x0c) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x10) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x14) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const stepUpController = stepUpScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(stepUpControllerDescStorage));
	if(!stepUpFloorActor || !stepUpObstacleActor || !stepUpController)
		return nxFail("step-up controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxReal shortProbeDown = -0.001f;
	const NxVec3 lowStepDisplacement(3.0f, shortProbeDown, 0.0f);
	reinterpret_cast<NxControllerProbe*>(stepUpController)->move(
		lowStepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& lowStepPosition = reinterpret_cast<NxControllerProbe*>(
		stepUpController)->getPosition();
	printf("simulation controller-grounded-short-step-probe position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(lowStepPosition.x), nxFloatBits(lowStepPosition.y),
		nxFloatBits(lowStepPosition.z), obstacleCollisionFlags);
	stepUpScene->releaseController(*stepUpController);
	sdk->releaseScene(*stepUpScene);
	}
	NxSceneDesc slideSceneDesc;
	slideSceneDesc.setToDefault();
	NxScene* const slideScene = sdk->createScene(slideSceneDesc);
	if(!slideScene)
		return nxFail("sliding controller scene creation failed");
	NxBoxShapeDesc slideObstacleShape;
	slideObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 1.0f);
	NxActorDesc slideObstacleActorDesc;
	slideObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	slideObstacleActorDesc.shapes.pushBack(&slideObstacleShape);
	NxActor* const slideObstacleActor = slideScene->createActor(slideObstacleActorDesc);
	alignas(4) unsigned char slideControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slideControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slideControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(slideControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const slideController = slideScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slideControllerDescStorage));
	if(!slideObstacleActor || !slideController)
		return nxFail("sliding controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 diagonalDisplacement(2.0f, 0.0f, 1.0f);
	reinterpret_cast<NxControllerProbe*>(slideController)->move(
		diagonalDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slideControllerPosition =
		reinterpret_cast<NxControllerProbe*>(slideController)->getPosition();
	printf("simulation controller-obstacle slide position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slideControllerPosition.x), nxFloatBits(slideControllerPosition.y),
		nxFloatBits(slideControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(slideControllerPosition.x) != 0x3f000000 ||
		nxFloatBits(slideControllerPosition.y) != 0 ||
		nxFloatBits(slideControllerPosition.z) != 0x3f800000 || obstacleCollisionFlags != 4;
	slideScene->releaseController(*slideController);
	sdk->releaseScene(*slideScene);
	NxSceneDesc overlapSceneDesc;
	overlapSceneDesc.setToDefault();
	NxScene* const overlapScene = sdk->createScene(overlapSceneDesc);
	if(!overlapScene)
		return nxFail("controller initial-overlap scene creation failed");
	NxBoxShapeDesc overlapObstacleShape;
	overlapObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 1.0f);
	NxActorDesc overlapObstacleActorDesc;
	overlapObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	overlapObstacleActorDesc.shapes.pushBack(&overlapObstacleShape);
	NxActor* const overlapObstacleActor = overlapScene->createActor(overlapObstacleActorDesc);
	alignas(4) unsigned char overlapControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(overlapControllerDescStorage + 0x0c) = nxFloatBits(0.75f);
	*reinterpret_cast<NxU32*>(overlapControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(overlapControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(overlapControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const overlapController = overlapScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(overlapControllerDescStorage));
	if(!overlapObstacleActor || !overlapController)
		return nxFail("controller initial-overlap fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 escapeDisplacement(-0.5f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(overlapController)->move(
		escapeDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& escapedControllerPosition =
		reinterpret_cast<NxControllerProbe*>(overlapController)->getPosition();
	printf("simulation controller-initial-overlap escape position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(escapedControllerPosition.x), nxFloatBits(escapedControllerPosition.y),
		nxFloatBits(escapedControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(escapedControllerPosition.x) != 0x3e800000 ||
		nxFloatBits(escapedControllerPosition.y) != 0 ||
		nxFloatBits(escapedControllerPosition.z) != 0 || obstacleCollisionFlags != 0;
	overlapScene->releaseController(*overlapController);
	NxController* const overlapControllerInward = overlapScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(overlapControllerDescStorage));
	if(!overlapControllerInward)
		return nxFail("controller inward-overlap fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 inwardDisplacement(0.5f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(overlapControllerInward)->move(
		inwardDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& inwardControllerPosition =
		reinterpret_cast<NxControllerProbe*>(overlapControllerInward)->getPosition();
	printf("simulation controller-initial-overlap inward position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(inwardControllerPosition.x), nxFloatBits(inwardControllerPosition.y),
		nxFloatBits(inwardControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(inwardControllerPosition.x) != 0x3f400000 ||
		nxFloatBits(inwardControllerPosition.y) != 0 ||
		nxFloatBits(inwardControllerPosition.z) != 0 || obstacleCollisionFlags != 4;
	overlapScene->releaseController(*overlapControllerInward);
	sdk->releaseScene(*overlapScene);
	// The rotated-box SAT path must handle a controller that starts inside the
	// obstacle and moves out through its expanded bound.
	NxSceneDesc rotatedOverlapSceneDesc;
	rotatedOverlapSceneDesc.setToDefault();
	NxScene* const rotatedOverlapScene = sdk->createScene(rotatedOverlapSceneDesc);
	if(!rotatedOverlapScene)
		return nxFail("rotated initial-overlap scene creation failed");
	NxBoxShapeDesc rotatedOverlapObstacleShape;
	rotatedOverlapObstacleShape.dimensions = NxVec3(1.0f, 0.5f, 0.1f);
	NxActorDesc rotatedOverlapObstacleActorDesc;
	rotatedOverlapObstacleActorDesc.globalPose.M =
		NxMat33(NxQuat(45.0f, NxVec3(0.0f, 1.0f, 0.0f)));
	rotatedOverlapObstacleActorDesc.shapes.pushBack(&rotatedOverlapObstacleShape);
	NxActor* const rotatedOverlapObstacleActor = rotatedOverlapScene->createActor(
		rotatedOverlapObstacleActorDesc);
	alignas(4) unsigned char rotatedOverlapControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(rotatedOverlapControllerDescStorage + 0x30) = nxFloatBits(0.1f);
	*reinterpret_cast<NxU32*>(rotatedOverlapControllerDescStorage + 0x34) = nxFloatBits(0.25f);
	*reinterpret_cast<NxU32*>(rotatedOverlapControllerDescStorage + 0x38) = nxFloatBits(0.1f);
	NxController* const rotatedOverlapController = rotatedOverlapScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(rotatedOverlapControllerDescStorage));
	if(!rotatedOverlapObstacleActor || !rotatedOverlapController)
		return nxFail("rotated initial-overlap controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 rotatedEscapeDisplacement(1.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(rotatedOverlapController)->move(
		rotatedEscapeDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& rotatedEscapedPosition =
		reinterpret_cast<NxControllerProbe*>(rotatedOverlapController)->getPosition();
	printf("simulation controller-rotated-initial-overlap escape position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(rotatedEscapedPosition.x), nxFloatBits(rotatedEscapedPosition.y),
		nxFloatBits(rotatedEscapedPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(rotatedEscapedPosition.x) != 0x3f800000 ||
		nxFloatBits(rotatedEscapedPosition.y) != 0 ||
		nxFloatBits(rotatedEscapedPosition.z) != 0 || obstacleCollisionFlags != 0;
	rotatedOverlapScene->releaseController(*rotatedOverlapController);
	NxController* const rotatedOverlapControllerReverse = rotatedOverlapScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(rotatedOverlapControllerDescStorage));
	if(!rotatedOverlapControllerReverse)
		return nxFail("rotated reverse-overlap controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 rotatedEscapeDisplacementReverse(-1.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(rotatedOverlapControllerReverse)->move(
		rotatedEscapeDisplacementReverse, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& rotatedEscapedPositionReverse =
		reinterpret_cast<NxControllerProbe*>(rotatedOverlapControllerReverse)->getPosition();
	printf("simulation controller-rotated-initial-overlap reverse position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(rotatedEscapedPositionReverse.x), nxFloatBits(rotatedEscapedPositionReverse.y),
		nxFloatBits(rotatedEscapedPositionReverse.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(rotatedEscapedPositionReverse.x) != 0xbf800000 ||
		nxFloatBits(rotatedEscapedPositionReverse.y) != 0 ||
		nxFloatBits(rotatedEscapedPositionReverse.z) != 0 || obstacleCollisionFlags != 0;
	rotatedOverlapScene->releaseController(*rotatedOverlapControllerReverse);
	sdk->releaseScene(*rotatedOverlapScene);
	NxBoxShapeDesc verticalObstacleShape;
	verticalObstacleShape.dimensions = NxVec3(1.0f, 0.5f, 1.0f);
	NxActorDesc verticalObstacleActorDesc;
	verticalObstacleActorDesc.globalPose.t = NxVec3(4.0f, 2.0f, 0.0f);
	verticalObstacleActorDesc.shapes.pushBack(&verticalObstacleShape);
	NxActor* const verticalObstacleActor =
		controllerObstacleScene->createActor(verticalObstacleActorDesc);
	alignas(4) unsigned char verticalControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(verticalControllerDescStorage + 0x0c) = nxFloatBits(4.0f);
	*reinterpret_cast<NxU32*>(verticalControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(verticalControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(verticalControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const verticalController = controllerObstacleScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(verticalControllerDescStorage));
	if(!verticalObstacleActor || !verticalController)
		return nxFail("vertical controller obstacle fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 upwardDisplacement(0.0f, 2.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(verticalController)->move(
		upwardDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& verticalControllerPosition =
		reinterpret_cast<NxControllerProbe*>(verticalController)->getPosition();
	printf("simulation controller-obstacle vertical position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(verticalControllerPosition.x), nxFloatBits(verticalControllerPosition.y),
		nxFloatBits(verticalControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(verticalControllerPosition.x) != 0x40800000 ||
		nxFloatBits(verticalControllerPosition.y) != 0x3f000000 ||
		nxFloatBits(verticalControllerPosition.z) != 0 || obstacleCollisionFlags != 1;
	controllerObstacleScene->releaseController(*verticalController);
	NxBoxShapeDesc triggerObstacleShape;
	triggerObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 1.0f);
	triggerObstacleShape.shapeFlags |= NX_TRIGGER_ENABLE;
	NxActorDesc triggerObstacleActorDesc;
	triggerObstacleActorDesc.globalPose.t = NxVec3(8.5f, 0.0f, 0.0f);
	triggerObstacleActorDesc.shapes.pushBack(&triggerObstacleShape);
	NxActor* const triggerObstacleActor =
		controllerObstacleScene->createActor(triggerObstacleActorDesc);
	alignas(4) unsigned char triggerControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(triggerControllerDescStorage + 0x0c) = nxFloatBits(7.0f);
	*reinterpret_cast<NxU32*>(triggerControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(triggerControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(triggerControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const triggerController = controllerObstacleScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(triggerControllerDescStorage));
	if(!triggerObstacleActor || !triggerController)
		return nxFail("trigger controller obstacle fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(triggerController)->move(
		obstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& triggerControllerPosition =
		reinterpret_cast<NxControllerProbe*>(triggerController)->getPosition();
	printf("simulation controller-obstacle trigger position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(triggerControllerPosition.x), nxFloatBits(triggerControllerPosition.y),
		nxFloatBits(triggerControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(triggerControllerPosition.x) != 0x40f00000 ||
		nxFloatBits(triggerControllerPosition.y) != 0 ||
		nxFloatBits(triggerControllerPosition.z) != 0 || obstacleCollisionFlags != 4;
	controllerObstacleScene->releaseController(*triggerController);
	controllerObstacleScene->releaseController(*obstacleController);
	sdk->releaseScene(*controllerObstacleScene);

	// A rotated thin box has an axis-aligned world bound that contains a large
	// empty corner. Moving along X at an offset through that corner distinguishes
	// a true controller sweep from treating every obstacle as its world AABB.
	NxSceneDesc rotatedControllerSceneDesc;
	rotatedControllerSceneDesc.setToDefault();
	NxScene* const rotatedControllerScene = sdk->createScene(rotatedControllerSceneDesc);
	if(!rotatedControllerScene)
		return nxFail("rotated controller scene creation failed");
	NxBoxShapeDesc rotatedControllerObstacleShape;
	rotatedControllerObstacleShape.dimensions = NxVec3(1.0f, 0.5f, 0.1f);
	NxActorDesc rotatedControllerObstacleActorDesc;
	rotatedControllerObstacleActorDesc.globalPose.M =
		NxMat33(NxQuat(45.0f, NxVec3(0.0f, 1.0f, 0.0f)));
	rotatedControllerObstacleActorDesc.shapes.pushBack(&rotatedControllerObstacleShape);
	NxActor* const rotatedControllerObstacle = rotatedControllerScene->createActor(
		rotatedControllerObstacleActorDesc);
	alignas(4) unsigned char rotatedControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x14) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x30) = nxFloatBits(0.1f);
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x34) = nxFloatBits(0.25f);
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x38) = nxFloatBits(0.1f);
	NxController* const rotatedController = rotatedControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(rotatedControllerDescStorage));
	if(!rotatedControllerObstacle || !rotatedController)
		return nxFail("rotated controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 rotatedObstacleDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(rotatedController)->move(
		rotatedObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& rotatedControllerPosition =
		reinterpret_cast<NxControllerProbe*>(rotatedController)->getPosition();
	printf("simulation controller-obstacle rotated-corner position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(rotatedControllerPosition.x), nxFloatBits(rotatedControllerPosition.y),
		nxFloatBits(rotatedControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(rotatedControllerPosition.x) != 0xbf50704e ||
		nxFloatBits(rotatedControllerPosition.y) != 0 ||
		nxFloatBits(rotatedControllerPosition.z) != 0x3f4ccccd || obstacleCollisionFlags != 4;
	rotatedControllerScene->releaseController(*rotatedController);
	sdk->releaseScene(*rotatedControllerScene);

	// A sphere's expanded world AABB has square corners. This diagonal path
	// clips that empty corner for a box controller and a spherical obstacle.
	NxSceneDesc sphereControllerSceneDesc;
	sphereControllerSceneDesc.setToDefault();
	NxScene* const sphereControllerScene = sdk->createScene(sphereControllerSceneDesc);
	if(!sphereControllerScene)
		return nxFail("sphere controller scene creation failed");
	NxSphereShapeDesc sphereControllerObstacleShape;
	sphereControllerObstacleShape.radius = 0.5f;
	NxActorDesc sphereControllerObstacleActorDesc;
	sphereControllerObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	sphereControllerObstacleActorDesc.shapes.pushBack(&sphereControllerObstacleShape);
	NxActor* const sphereControllerObstacle = sphereControllerScene->createActor(
		sphereControllerObstacleActorDesc);
	alignas(4) unsigned char sphereControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x14) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const sphereController = sphereControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(sphereControllerDescStorage));
	if(!sphereControllerObstacle || !sphereController)
		return nxFail("sphere controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 sphereObstacleDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(sphereController)->move(
		sphereObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& sphereControllerPosition =
		reinterpret_cast<NxControllerProbe*>(sphereController)->getPosition();
	printf("simulation controller-obstacle sphere-corner position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(sphereControllerPosition.x), nxFloatBits(sphereControllerPosition.y),
		nxFloatBits(sphereControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(sphereControllerPosition.x) != 0x40000000 ||
		nxFloatBits(sphereControllerPosition.y) != 0x3f4ccccd ||
		nxFloatBits(sphereControllerPosition.z) != 0x3f4ccccd || obstacleCollisionFlags != 0;
	sphereControllerScene->releaseController(*sphereController);
	sdk->releaseScene(*sphereControllerScene);

	// The mesh bounds cover this controller path, but its triangular surface lies
	// beyond the moving box's rounded-free corner in the YZ plane.
	const NxPoint controllerMeshPoints[] = {
		NxPoint(0.0f, 0.0f, 0.0f), NxPoint(0.0f, 0.0f, 1.0f),
		NxPoint(0.0f, 1.0f, 0.0f), NxPoint(0.0f, 0.5f, 0.5f)};
	const NxU32 controllerMeshIndices[] = {0, 1, 3, 0, 3, 2};
	NxTriangleMeshDesc controllerMeshDesc;
	controllerMeshDesc.numVertices = 4;
	controllerMeshDesc.numTriangles = 2;
	controllerMeshDesc.pointStrideBytes = sizeof(NxPoint);
	controllerMeshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	controllerMeshDesc.points = controllerMeshPoints;
	controllerMeshDesc.triangles = controllerMeshIndices;
	NxTriangleMesh* const controllerMesh = sdk->createTriangleMesh(controllerMeshDesc);
	if(!controllerMesh)
		return nxFail("controller mesh fixture cooking failed");
	NxSceneDesc meshControllerSceneDesc;
	meshControllerSceneDesc.setToDefault();
	NxScene* const meshControllerScene = sdk->createScene(meshControllerSceneDesc);
	if(!meshControllerScene)
		return nxFail("mesh controller scene creation failed");
	NxTriangleMeshShapeDesc meshControllerObstacleShape;
	meshControllerObstacleShape.meshData = controllerMesh;
	NxActorDesc meshControllerObstacleActorDesc;
	meshControllerObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	meshControllerObstacleActorDesc.shapes.pushBack(&meshControllerObstacleShape);
	NxActor* const meshControllerObstacle = meshControllerScene->createActor(
		meshControllerObstacleActorDesc);
	alignas(4) unsigned char meshControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x10) = nxFloatBits(1.1f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x14) = nxFloatBits(1.1f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const meshController = meshControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(meshControllerDescStorage));
	if(!meshControllerObstacle || !meshController)
		return nxFail("mesh controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 meshObstacleDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(meshController)->move(
		meshObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& meshControllerPosition =
		reinterpret_cast<NxControllerProbe*>(meshController)->getPosition();
	printf("simulation controller-obstacle mesh-corner position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(meshControllerPosition.x), nxFloatBits(meshControllerPosition.y),
		nxFloatBits(meshControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(meshControllerPosition.x) != 0x40000000 ||
		nxFloatBits(meshControllerPosition.y) != 0x3f8ccccd ||
		nxFloatBits(meshControllerPosition.z) != 0x3f8ccccd || obstacleCollisionFlags != 0;
	meshControllerScene->releaseController(*meshController);
	sdk->releaseScene(*meshControllerScene);
	NxSceneDesc meshHitSceneDesc;
	meshHitSceneDesc.setToDefault();
	NxScene* const meshHitScene = sdk->createScene(meshHitSceneDesc);
	if(!meshHitScene)
		return nxFail("mesh-hit controller scene creation failed");
	NxActorDesc meshHitObstacleActorDesc;
	meshHitObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	meshHitObstacleActorDesc.shapes.pushBack(&meshControllerObstacleShape);
	NxActor* const meshHitObstacle = meshHitScene->createActor(meshHitObstacleActorDesc);
	alignas(4) unsigned char meshHitControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const meshHitController = meshHitScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(meshHitControllerDescStorage));
	if(!meshHitObstacle || !meshHitController)
		return nxFail("mesh-hit controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(meshHitController)->move(
		meshObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& meshHitControllerPosition =
		reinterpret_cast<NxControllerProbe*>(meshHitController)->getPosition();
	printf("simulation controller-obstacle mesh-hit position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(meshHitControllerPosition.x), nxFloatBits(meshHitControllerPosition.y),
		nxFloatBits(meshHitControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(meshHitControllerPosition.x) != 0x3f800000 ||
		nxFloatBits(meshHitControllerPosition.y) != 0x3e4ccccd ||
		nxFloatBits(meshHitControllerPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 4;
	meshHitScene->releaseController(*meshHitController);
	// Sweep across a rising triangle mesh face to pin the controller's slope
	// normal classification and stop pose.
	const NxPoint controllerSlopePoints[] = {
		NxPoint(0.0f, 0.0f, 0.0f), NxPoint(0.0f, 0.0f, 1.0f),
		NxPoint(1.0f, 0.5f, 0.0f), NxPoint(1.0f, 0.5f, 1.0f)};
	const NxU32 controllerSlopeIndices[] = {0, 1, 2, 1, 3, 2};
	NxTriangleMeshDesc controllerSlopeMeshDesc;
	controllerSlopeMeshDesc.numVertices = 4;
	controllerSlopeMeshDesc.numTriangles = 2;
	controllerSlopeMeshDesc.pointStrideBytes = sizeof(NxPoint);
	controllerSlopeMeshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	controllerSlopeMeshDesc.points = controllerSlopePoints;
	controllerSlopeMeshDesc.triangles = controllerSlopeIndices;
	NxTriangleMesh* const controllerSlopeMesh = sdk->createTriangleMesh(controllerSlopeMeshDesc);
	if(!controllerSlopeMesh)
		return nxFail("controller slope mesh cooking failed");
	NxSceneDesc slopeControllerSceneDesc;
	slopeControllerSceneDesc.setToDefault();
	NxScene* const slopeControllerScene = sdk->createScene(slopeControllerSceneDesc);
	if(!slopeControllerScene)
		return nxFail("slope controller scene creation failed");
	NxTriangleMeshShapeDesc slopeObstacleShape;
	slopeObstacleShape.meshData = controllerSlopeMesh;
	NxActorDesc slopeObstacleActorDesc;
	slopeObstacleActorDesc.shapes.pushBack(&slopeObstacleShape);
	NxActor* const slopeObstacle = slopeControllerScene->createActor(slopeObstacleActorDesc);
	alignas(4) unsigned char slopeControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x10) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const slopeController = slopeControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slopeControllerDescStorage));
	if(!slopeObstacle || !slopeController)
		return nxFail("slope controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 slopeDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(slopeController)->move(
		slopeDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slopeControllerPosition =
		reinterpret_cast<NxControllerProbe*>(slopeController)->getPosition();
	printf("simulation controller-mesh-slope position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slopeControllerPosition.x), nxFloatBits(slopeControllerPosition.y),
		nxFloatBits(slopeControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(slopeControllerPosition.x) != 0x3eccccd0 ||
		nxFloatBits(slopeControllerPosition.y) != 0x3f000000 ||
		nxFloatBits(slopeControllerPosition.z) != 0x3f000000 || obstacleCollisionFlags != 4;
	slopeControllerScene->releaseController(*slopeController);
	sdk->releaseScene(*slopeControllerScene);
	// Drive a high-threshold, step-enabled controller downward onto a sloped
	// triangle face. This gives the resolver a non-axis-aligned contact normal
	// for its conditional correction query.
	NxSceneDesc slopeCorrectionSceneDesc;
	slopeCorrectionSceneDesc.setToDefault();
	NxScene* const slopeCorrectionScene = sdk->createScene(slopeCorrectionSceneDesc);
	if(!slopeCorrectionScene)
		return nxFail("slope-correction controller scene creation failed");
	NxActorDesc slopeCorrectionObstacleDesc;
	slopeCorrectionObstacleDesc.shapes.pushBack(&slopeObstacleShape);
	NxActor* const slopeCorrectionObstacle = slopeCorrectionScene->createActor(
		slopeCorrectionObstacleDesc);
	alignas(4) unsigned char slopeCorrectionControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x0c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x1c) = nxFloatBits(0.95f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x18) = 1;
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const slopeCorrectionController = slopeCorrectionScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slopeCorrectionControllerDescStorage));
	if(!slopeCorrectionObstacle || !slopeCorrectionController)
		return nxFail("slope-correction controller fixture setup failed");
	const NxVec3 slopeCorrectionDisplacement(0.0f, -3.0f, 0.0f);
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(slopeCorrectionController)->move(
		slopeCorrectionDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slopeCorrectionPosition = reinterpret_cast<NxControllerProbe*>(
		slopeCorrectionController)->getPosition();
	printf("simulation controller-mesh-slope-correction position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slopeCorrectionPosition.x), nxFloatBits(slopeCorrectionPosition.y),
		nxFloatBits(slopeCorrectionPosition.z), obstacleCollisionFlags);
	slopeCorrectionScene->releaseController(*slopeCorrectionController);
	// Use an isolated scene and disable only the slope correction threshold to
	// capture the pose after the resolver's original three probes. Controller
	// release retains its generated actor, so scene reuse would contaminate the
	// next sweep with a second controller obstacle.
	NxSceneDesc slopeBaselineSceneDesc;
	slopeBaselineSceneDesc.setToDefault();
	NxScene* const slopeBaselineScene = sdk->createScene(slopeBaselineSceneDesc);
	if(!slopeBaselineScene)
		return nxFail("slope-baseline controller scene creation failed");
	NxActorDesc slopeBaselineObstacleDesc;
	slopeBaselineObstacleDesc.shapes.pushBack(&slopeObstacleShape);
	NxActor* const slopeBaselineObstacle = slopeBaselineScene->createActor(
		slopeBaselineObstacleDesc);
	alignas(4) unsigned char slopeBaselineControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x0c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x1c) = 0;
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x18) = 1;
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const slopeBaselineController = slopeBaselineScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slopeBaselineControllerDescStorage));
	if(!slopeBaselineObstacle || !slopeBaselineController)
		return nxFail("slope-baseline controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(slopeBaselineController)->move(
		slopeCorrectionDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slopeBaselinePosition = reinterpret_cast<NxControllerProbe*>(
		slopeBaselineController)->getPosition();
	printf("simulation controller-mesh-slope-baseline position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slopeBaselinePosition.x), nxFloatBits(slopeBaselinePosition.y),
		nxFloatBits(slopeBaselinePosition.z), obstacleCollisionFlags);
	slopeBaselineScene->releaseController(*slopeBaselineController);
	sdk->releaseScene(*slopeBaselineScene);
	sdk->releaseScene(*slopeCorrectionScene);
	// Keep the descriptor's other axis-like word different so this scene proves
	// which field the resolver actually uses when it decomposes movement.
	NxSceneDesc slopeAxisSceneDesc;
	slopeAxisSceneDesc.setToDefault();
	NxScene* const slopeAxisScene = sdk->createScene(slopeAxisSceneDesc);
	if(!slopeAxisScene)
		return nxFail("slope-axis controller scene creation failed");
	NxActorDesc slopeAxisObstacleDesc;
	slopeAxisObstacleDesc.shapes.pushBack(&slopeObstacleShape);
	NxActor* const slopeAxisObstacle = slopeAxisScene->createActor(slopeAxisObstacleDesc);
	alignas(4) unsigned char slopeAxisControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x0c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x1c) = 1;
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x18) = 0;
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const slopeAxisController = slopeAxisScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slopeAxisControllerDescStorage));
	if(!slopeAxisObstacle || !slopeAxisController)
		return nxFail("slope-axis controller fixture setup failed");
	const NxVec3 slopeAxisDisplacement(0.0f, -3.0f, 0.0f);
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(slopeAxisController)->move(
		slopeAxisDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slopeAxisPosition = reinterpret_cast<NxControllerProbe*>(
		slopeAxisController)->getPosition();
	printf("simulation controller-mesh-slope-descriptor-axis position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slopeAxisPosition.x), nxFloatBits(slopeAxisPosition.y),
		nxFloatBits(slopeAxisPosition.z), obstacleCollisionFlags);
	slopeAxisScene->releaseController(*slopeAxisController);
	sdk->releaseScene(*slopeAxisScene);
	sdk->releaseTriangleMesh(*controllerSlopeMesh);

	// Exercise the complete public path from 16-bit descriptor indices through
	// mesh cooking to the controller's triangle sweep.
	const NxU16 controllerMeshIndices16[] = {0, 1, 3, 0, 3, 2};
	NxTriangleMeshDesc controllerMesh16Desc;
	controllerMesh16Desc.numVertices = 4;
	controllerMesh16Desc.numTriangles = 2;
	controllerMesh16Desc.pointStrideBytes = sizeof(NxPoint);
	controllerMesh16Desc.triangleStrideBytes = 3 * sizeof(NxU16);
	controllerMesh16Desc.points = controllerMeshPoints;
	controllerMesh16Desc.triangles = controllerMeshIndices16;
	controllerMesh16Desc.flags = NX_MF_16_BIT_INDICES;
	NxTriangleMesh* const controllerMesh16 = sdk->createTriangleMesh(controllerMesh16Desc);
	if(!controllerMesh16)
		return nxFail("16-bit controller mesh cooking failed");
	NxSceneDesc mesh16HitSceneDesc;
	mesh16HitSceneDesc.setToDefault();
	NxScene* const mesh16HitScene = sdk->createScene(mesh16HitSceneDesc);
	if(!mesh16HitScene)
		return nxFail("16-bit mesh-hit scene creation failed");
	NxTriangleMeshShapeDesc mesh16ObstacleShape;
	mesh16ObstacleShape.meshData = controllerMesh16;
	NxActorDesc mesh16ObstacleActorDesc;
	mesh16ObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	mesh16ObstacleActorDesc.shapes.pushBack(&mesh16ObstacleShape);
	NxActor* const mesh16Obstacle = mesh16HitScene->createActor(mesh16ObstacleActorDesc);
	alignas(4) unsigned char mesh16ControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const mesh16Controller = mesh16HitScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(mesh16ControllerDescStorage));
	if(!mesh16Obstacle || !mesh16Controller)
		return nxFail("16-bit mesh-hit fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(mesh16Controller)->move(
		meshObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& mesh16ControllerPosition =
		reinterpret_cast<NxControllerProbe*>(mesh16Controller)->getPosition();
	const NxU32 mesh16ControllerIndexFormat = controllerMesh16->getFormat(0, NX_ARRAY_TRIANGLES);
	const NxU32 mesh16ControllerIndexStride = controllerMesh16->getStride(0, NX_ARRAY_TRIANGLES);
	printf("simulation controller-obstacle mesh16-hit source_index_bits=16 index_format=%u index_stride=%u position=%08x.%08x.%08x flags=%08x\n",
		mesh16ControllerIndexFormat, mesh16ControllerIndexStride,
		nxFloatBits(mesh16ControllerPosition.x), nxFloatBits(mesh16ControllerPosition.y),
		nxFloatBits(mesh16ControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed || mesh16ControllerIndexFormat != 4 ||
		mesh16ControllerIndexStride != 3 * sizeof(NxU32) ||
		nxFloatBits(mesh16ControllerPosition.x) != 0x3f800000 ||
		nxFloatBits(mesh16ControllerPosition.y) != 0x3e4ccccd ||
		nxFloatBits(mesh16ControllerPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 4;
	mesh16HitScene->releaseController(*mesh16Controller);
	sdk->releaseScene(*mesh16HitScene);
	sdk->releaseTriangleMesh(*controllerMesh16);
	alignas(4) unsigned char meshOverlapOutDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x0c) = nxFloatBits(1.5f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const meshOverlapOutController = meshHitScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(meshOverlapOutDescStorage));
	if(!meshOverlapOutController)
		return nxFail("mesh-overlap-out controller fixture setup failed");
	NxSceneDesc meshOverlapInSceneDesc;
	meshOverlapInSceneDesc.setToDefault();
	NxScene* const meshOverlapInScene = sdk->createScene(meshOverlapInSceneDesc);
	if(!meshOverlapInScene)
		return nxFail("mesh-overlap-in scene creation failed");
	NxActorDesc meshOverlapInObstacleActorDesc;
	meshOverlapInObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	meshOverlapInObstacleActorDesc.shapes.pushBack(&meshControllerObstacleShape);
	NxActor* const meshOverlapInObstacle = meshOverlapInScene->createActor(
		meshOverlapInObstacleActorDesc);
	alignas(4) unsigned char meshOverlapInDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x0c) = nxFloatBits(1.5f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const meshOverlapInController = meshOverlapInScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(meshOverlapInDescStorage));
	if(!meshOverlapInObstacle || !meshOverlapInController)
		return nxFail("mesh-overlap controller fixture setup failed");
	const NxVec3 meshOverlapOutDisplacement(-0.5f, 0.0f, 0.0f);
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(meshOverlapOutController)->move(
		meshOverlapOutDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& meshOverlapOutPosition =
		reinterpret_cast<NxControllerProbe*>(meshOverlapOutController)->getPosition();
	printf("simulation controller-obstacle mesh-overlap-out position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(meshOverlapOutPosition.x), nxFloatBits(meshOverlapOutPosition.y),
		nxFloatBits(meshOverlapOutPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(meshOverlapOutPosition.x) != 0x3f800000 ||
		nxFloatBits(meshOverlapOutPosition.y) != 0x3e4ccccd ||
		nxFloatBits(meshOverlapOutPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 0;
	const NxVec3 meshOverlapInDisplacement(0.5f, 0.0f, 0.0f);
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(meshOverlapInController)->move(
		meshOverlapInDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& meshOverlapInPosition =
		reinterpret_cast<NxControllerProbe*>(meshOverlapInController)->getPosition();
	printf("simulation controller-obstacle mesh-overlap-in position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(meshOverlapInPosition.x), nxFloatBits(meshOverlapInPosition.y),
		nxFloatBits(meshOverlapInPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(meshOverlapInPosition.x) != 0x40000000 ||
		nxFloatBits(meshOverlapInPosition.y) != 0x3e4ccccd ||
		nxFloatBits(meshOverlapInPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 0;
	meshOverlapInScene->releaseController(*meshOverlapInController);
	sdk->releaseScene(*meshOverlapInScene);
	meshHitScene->releaseController(*meshOverlapOutController);
	sdk->releaseScene(*meshHitScene);
	sdk->releaseTriangleMesh(*controllerMesh);

	// Reverse the established controller mesh winding so the moving controller
	// approaches the back side of the same triangular face.
	const NxU32 backfaceMeshIndices[] = {0, 3, 1, 0, 2, 3};
	NxTriangleMeshDesc backfaceMeshDesc;
	backfaceMeshDesc.numVertices = 4;
	backfaceMeshDesc.numTriangles = 2;
	backfaceMeshDesc.pointStrideBytes = sizeof(NxPoint);
	backfaceMeshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	backfaceMeshDesc.points = controllerMeshPoints;
	backfaceMeshDesc.triangles = backfaceMeshIndices;
	NxTriangleMesh* const backfaceMesh = sdk->createTriangleMesh(backfaceMeshDesc);
	if(!backfaceMesh)
		return nxFail("controller back-face mesh cooking failed");
	NxSceneDesc backfaceControllerSceneDesc;
	backfaceControllerSceneDesc.setToDefault();
	NxScene* const backfaceControllerScene = sdk->createScene(backfaceControllerSceneDesc);
	if(!backfaceControllerScene)
		return nxFail("controller back-face scene creation failed");
	NxTriangleMeshShapeDesc backfaceObstacleShape;
	backfaceObstacleShape.meshData = backfaceMesh;
	NxActorDesc backfaceObstacleActorDesc;
	backfaceObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	backfaceObstacleActorDesc.shapes.pushBack(&backfaceObstacleShape);
	NxActor* const backfaceObstacle = backfaceControllerScene->createActor(
		backfaceObstacleActorDesc);
	alignas(4) unsigned char backfaceControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const backfaceController = backfaceControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(backfaceControllerDescStorage));
	if(!backfaceObstacle || !backfaceController)
		return nxFail("controller back-face fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 backfaceDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(backfaceController)->move(
		backfaceDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& backfaceControllerPosition =
		reinterpret_cast<NxControllerProbe*>(backfaceController)->getPosition();
	printf("simulation controller-obstacle mesh-backface position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(backfaceControllerPosition.x), nxFloatBits(backfaceControllerPosition.y),
		nxFloatBits(backfaceControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(backfaceControllerPosition.x) != 0x40000000 ||
		nxFloatBits(backfaceControllerPosition.y) != 0x3e4ccccd ||
		nxFloatBits(backfaceControllerPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 0;
	backfaceControllerScene->releaseController(*backfaceController);
	sdk->releaseScene(*backfaceControllerScene);
	sdk->releaseTriangleMesh(*backfaceMesh);

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
	if(deferredContactFailed)
		status = 1;
	if(controllerCreateFailed)
		status = 1;
	if(shapeUserDataFailed)
		status = 1;
	FreeLibrary(physics);
	return status;
	}
