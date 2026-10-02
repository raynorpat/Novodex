#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxUserContactReport.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned nxFloatBits(NxReal value)
	{
	unsigned bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

class NxTriggerSimulationReport : public NxUserTriggerReport
	{
	public:
	NxShape* expectedTrigger;
	NxShape* expectedOther;
	unsigned calls;
	unsigned lastEvent;
	NxTriggerSimulationReport() : expectedTrigger(0), expectedOther(0), calls(0), lastEvent(0) {}
	virtual void onTrigger(NxShape& trigger, NxShape& other, NxTriggerFlag event)
		{
		++calls;
		lastEvent = static_cast<unsigned>(event);
		printf("trigger callback trigger=%u other=%u event=%u\n",
			&trigger == expectedTrigger, &other == expectedOther, lastEvent);
		}
	};

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsTriggerSimulationTests", pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk) return nxFail("SDK creation failed");

	NxTriggerSimulationReport report;
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.broadPhase = NX_BROADPHASE_QUADRATIC;
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	sceneDesc.userTriggerReport = &report;
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("trigger scene creation failed");

	NxBoxShapeDesc triggerBox;
	triggerBox.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	triggerBox.shapeFlags = NX_TRIGGER_ON_ENTER | NX_TRIGGER_ON_STAY | NX_TRIGGER_ON_LEAVE;
	NxActorDesc triggerActorDesc;
	triggerActorDesc.shapes.pushBack(&triggerBox);
	NxActor* triggerActor = scene->createActor(triggerActorDesc);
	if(!triggerActor) return nxFail("trigger actor creation failed");

	NxSphereShapeDesc otherSphere;
	otherSphere.radius = 0.25f;
	NxBodyDesc otherBody;
	NxActorDesc otherDesc;
	otherDesc.body = &otherBody;
	otherDesc.density = 1.0f;
	otherDesc.globalPose.t = NxVec3(-2.0f, 0.0f, 0.0f);
	otherDesc.shapes.pushBack(&otherSphere);
	NxActor* otherActor = scene->createActor(otherDesc);
	if(!otherActor) return nxFail("trigger other actor creation failed");
	report.expectedTrigger = triggerActor->getShapes()[0];
	report.expectedOther = otherActor->getShapes()[0];
	otherActor->setLinearVelocity(NxVec3(8.0f, 0.0f, 0.0f));

	for(unsigned step = 0; step != 10; ++step)
		{
		scene->simulate(0.05f);
		const bool ready = scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched) return nxFail("trigger simulation step failed");
		printf("trigger step=%u pos=%08x calls=%u last=%u ready=%u fetched=%u\n",
			step, nxFloatBits(otherActor->getGlobalPosition().x), report.calls,
			report.lastEvent, ready, fetched);
		}

	sdk->releaseScene(*scene);
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
