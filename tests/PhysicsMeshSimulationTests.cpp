#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxUserContactReport.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned nxFloatBits(NxReal value)
	{
	unsigned bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

class NxMeshContactReport : public NxUserContactReport
	{
	public:
	NxActor* expectedGround;
	NxActor* expectedSphere;
	unsigned calls;
	unsigned events;
	unsigned pointCount;
	unsigned firstPoint[3];
	NxMeshContactReport(NxActor* ground, NxActor* sphere)
		: expectedGround(ground), expectedSphere(sphere), calls(0), events(0), pointCount(0), firstPoint() {}
	virtual void onContactNotify(NxContactPair& pair, NxU32 eventFlags)
		{
		if(pair.actors[0] != expectedGround || pair.actors[1] != expectedSphere)
			return;
		++calls;
		events |= eventFlags;
		NxContactStreamIterator iterator(pair.stream);
		while(iterator.goNextPair())
			while(iterator.goNextPatch())
				{
				while(iterator.goNextPoint())
					{
					const NxVec3 point = iterator.getPoint();
					if(pointCount == 0)
						{
						firstPoint[0] = nxFloatBits(point.x);
						firstPoint[1] = nxFloatBits(point.y);
						firstPoint[2] = nxFloatBits(point.z);
						}
					++pointCount;
					}
				}
		}
	};

class NxMeshTriggerReport : public NxUserTriggerReport
	{
	public:
	NxShape* expectedTrigger;
	NxShape* expectedOther;
	unsigned calls;
	unsigned lastEvent;
	unsigned unexpectedCalls;
	NxMeshTriggerReport() : expectedTrigger(0), expectedOther(0), calls(0), lastEvent(0), unexpectedCalls(0) {}
	virtual void onTrigger(NxShape& trigger, NxShape& other, NxTriggerFlag event)
		{
		if(&trigger != expectedTrigger || &other != expectedOther)
			{
			++unexpectedCalls;
			return;
			}
		++calls;
		lastEvent = static_cast<unsigned>(event);
		}
	};

int wmain(int argc, wchar_t** argv)
	{
	if(argc != 2)
		return nxFail("usage: NxPhysicsMeshSimulationTests <pair directory>");
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsMeshSimulationTests", pairDirectory, &physics);
	if(status)
		return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		return nxFail("NxCreatePhysicsSDK missing");
	NxPhysicsSDK* const sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk)
		return nxFail("SDK creation failed");

	const NxPoint vertices[] = {
		NxPoint(-2.0f, 0.0f, -2.0f), NxPoint(2.0f, 0.0f, -2.0f), NxPoint(-2.0f, 0.0f, 2.0f),
		NxPoint(2.0f, 0.0f, -2.0f), NxPoint(2.0f, 0.0f, 2.0f), NxPoint(-2.0f, 0.0f, 2.0f)
		};
	const NxU32 triangles[] = { 0, 2, 1, 3, 5, 4 };
	NxTriangleMeshDesc meshDesc;
	meshDesc.numVertices = sizeof(vertices) / sizeof(vertices[0]);
	meshDesc.numTriangles = 2;
	meshDesc.pointStrideBytes = sizeof(NxPoint);
	meshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	meshDesc.points = vertices;
	meshDesc.triangles = triangles;
	NxTriangleMesh* const mesh = sdk->createTriangleMesh(meshDesc);
	if(!mesh)
		return nxFail("triangle-mesh creation failed");

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxMeshTriggerReport triggerReport;
	sceneDesc.userTriggerReport = &triggerReport;
	NxScene* const scene = sdk->createScene(sceneDesc);
	if(!scene)
		return nxFail("mesh-contact scene creation failed");
	scene->setTiming(1.0f / 60.0f, 8, NX_TIMESTEP_FIXED);
	NxTriangleMeshShapeDesc meshShape;
	meshShape.meshData = mesh;
	NxActorDesc groundDesc;
	groundDesc.shapes.pushBack(&meshShape);
	NxActor* const ground = scene->createActor(groundDesc);
	NxSphereShapeDesc sphereShape;
	sphereShape.radius = 0.5f;
	NxBodyDesc bodyDesc;
	NxActorDesc sphereDesc;
	sphereDesc.body = &bodyDesc;
	sphereDesc.density = 1.0f;
	sphereDesc.globalPose.t = NxVec3(0.0f, 1.5f, 0.0f);
	sphereDesc.shapes.pushBack(&sphereShape);
	NxActor* const sphere = scene->createActor(sphereDesc);
	if(!ground || !sphere)
		return nxFail("mesh-contact actor creation failed");

	NxMeshContactReport report(ground, sphere);
	scene->setUserContactReport(&report);
	ground->setGroup(7);
	sphere->setGroup(3);
	sdk->setActorGroupPairFlags(7, 3, NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	for(unsigned step = 0; step < 30; ++step)
		{
		scene->simulate(1.0f / 60.0f);
		if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("mesh-contact simulation results failed");
		}
	NxVec3 position;
	NxVec3 velocity;
	sphere->getGlobalPosition(position);
	sphere->getLinearVelocity(velocity);
	printf("simulation mesh-contact calls=%u events=%08x y=%08x vy=%08x\n",
		report.calls, report.events, nxFloatBits(position.y), nxFloatBits(velocity.y));

	// The indexed triangles wind their front faces upward. The shipped sphere/
	// mesh row rejects an overlapping sphere below an ordinary mesh.
	sphere->setGlobalPosition(NxVec3(0.0f, -0.25f, 0.0f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	report.calls = 0;
	report.events = 0;
	report.pointCount = 0;
	for(unsigned step = 0; step < 30; ++step)
		{
		scene->simulate(1.0f / 60.0f);
		if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("backface mesh-contact simulation results failed");
		}
	sphere->getGlobalPosition(position);
	sphere->getLinearVelocity(velocity);
	printf("simulation mesh-backface calls=%u events=%08x y=%08x vy=%08x\n",
		report.calls, report.events, nxFloatBits(position.y), nxFloatBits(velocity.y));

	// A sphere whose center remains outside the mesh footprint exercises the
	// boundary-edge contact branch rather than only the interior face path.
	sphere->setGlobalPosition(NxVec3(2.25f, 1.5f, 0.0f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	report.calls = 0;
	report.events = 0;
	unsigned edgeSteps = 0;
	for(; edgeSteps < 60; ++edgeSteps)
		{
		scene->simulate(1.0f / 60.0f);
		if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("edge mesh-contact simulation results failed");
		if(report.calls != 0)
			{
			++edgeSteps;
			break;
			}
		}
	sphere->getGlobalPosition(position);
	sphere->getLinearVelocity(velocity);
	printf("simulation mesh-edge steps=%u calls=%u events=%08x points=%u point=%08x.%08x.%08x\n",
		edgeSteps, report.calls, report.events, report.pointCount,
		report.firstPoint[0], report.firstPoint[1], report.firstPoint[2]);
	sphere->setGlobalPosition(NxVec3(-30.0f, 20.0f, 20.0f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));

	// Matrix-B sphere/mesh overlap drives a trigger sphere placed across the
	// mesh's front face. The oracle should emit one enter event.
	NxTriangleMeshShapeDesc triggerMeshShape;
	triggerMeshShape.meshData = mesh;
	triggerMeshShape.shapeFlags = NX_TRIGGER_ON_ENTER;
	NxActorDesc triggerDesc;
	triggerDesc.globalPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
	triggerDesc.globalPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
	triggerDesc.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	triggerDesc.globalPose.t = NxVec3(3.0f, 0.0f, 0.0f);
	triggerDesc.shapes.pushBack(&triggerMeshShape);
	NxActor* const triggerActor = scene->createActor(triggerDesc);
	NxSphereShapeDesc meshOtherShape;
	meshOtherShape.radius = 0.5f;
	NxBodyDesc meshOtherBody;
	NxActorDesc meshTriggerOtherDesc;
	meshTriggerOtherDesc.body = &meshOtherBody;
	meshTriggerOtherDesc.density = 1.0f;
	meshTriggerOtherDesc.globalPose.t = NxVec3(2.75f, 0.0f, 0.0f);
	meshTriggerOtherDesc.shapes.pushBack(&meshOtherShape);
	NxActor* const meshTriggerOther = scene->createActor(meshTriggerOtherDesc);
	if(!triggerActor || !meshTriggerOther)
		return nxFail("mesh trigger actor creation failed");
	triggerReport.expectedTrigger = triggerActor->getShapes()[0];
	triggerReport.expectedOther = meshTriggerOther->getShapes()[0];

	// This sphere's AABB overlaps the trigger mesh's bounds, but its center and
	// radius miss the single triangle. The overlap row must not emit a callback.
	const NxPoint missVertices[] = {
		NxPoint(-2.0f, 0.0f, -2.0f), NxPoint(-1.0f, 0.0f, -2.0f), NxPoint(-2.0f, 0.0f, -1.0f),
		NxPoint(2.0f, 0.0f, 2.0f), NxPoint(1.0f, 0.0f, 2.0f), NxPoint(2.0f, 0.0f, 1.0f)
		};
	const NxU32 missTriangles[] = { 0, 2, 1, 3, 5, 4 };
	NxTriangleMeshDesc missMeshDesc;
	missMeshDesc.numVertices = sizeof(missVertices) / sizeof(missVertices[0]);
	missMeshDesc.numTriangles = 2;
	missMeshDesc.pointStrideBytes = sizeof(NxPoint);
	missMeshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	missMeshDesc.points = missVertices;
	missMeshDesc.triangles = missTriangles;
	NxTriangleMesh* const missMesh = sdk->createTriangleMesh(missMeshDesc);
	if(!missMesh)
		return nxFail("miss triangle-mesh creation failed");
	NxTriangleMeshShapeDesc missTriggerShape;
	missTriggerShape.meshData = missMesh;
	missTriggerShape.shapeFlags = NX_TRIGGER_ON_ENTER;
	NxActorDesc missTriggerDesc;
	missTriggerDesc.globalPose.t = NxVec3(-10.0f, 0.0f, 0.0f);
	missTriggerDesc.shapes.pushBack(&missTriggerShape);
	NxActor* const missTriggerActor = scene->createActor(missTriggerDesc);
	NxSphereShapeDesc missSphereShape;
	missSphereShape.radius = 0.5f;
	NxBodyDesc missSphereBody;
	NxActorDesc missSphereDesc;
	missSphereDesc.body = &missSphereBody;
	missSphereDesc.density = 1.0f;
	missSphereDesc.globalPose.t = NxVec3(-7.6f, 0.25f, 2.4f);
	missSphereDesc.shapes.pushBack(&missSphereShape);
	NxActor* const missSphereActor = scene->createActor(missSphereDesc);
	if(!missTriggerActor || !missSphereActor)
		return nxFail("miss mesh-trigger actor creation failed");
	scene->simulate(1.0f / 60.0f);
	if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
		|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
		return nxFail("mesh trigger simulation results failed");
	printf("simulation mesh-trigger calls=%u event=%u unexpected=%u\n",
		triggerReport.calls, triggerReport.lastEvent, triggerReport.unexpectedCalls);

	sdk->setActorGroupPairFlags(7, 3, 0);
	scene->releaseActor(*missSphereActor);
	scene->releaseActor(*missTriggerActor);
	sdk->releaseTriangleMesh(*missMesh);
	scene->releaseActor(*meshTriggerOther);
	scene->releaseActor(*triggerActor);
	scene->releaseActor(*sphere);
	scene->releaseActor(*ground);
	sdk->releaseScene(*scene);
	sdk->releaseTriangleMesh(*mesh);
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
