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
	unsigned unexpectedCalls;
	unsigned events;
	unsigned patchCount;
	unsigned firstPatchPoints;
	unsigned pointCount;
	unsigned firstPoint[3];
	unsigned firstNormal[3];
	unsigned secondNormal[3];
	unsigned firstSeparation;
	NxMeshContactReport(NxActor* ground, NxActor* sphere)
		: expectedGround(ground), expectedSphere(sphere), calls(0), unexpectedCalls(0), events(0), patchCount(0),
			firstPatchPoints(0), pointCount(0),
			firstPoint(), firstNormal(), secondNormal(), firstSeparation(0) {}
	virtual void onContactNotify(NxContactPair& pair, NxU32 eventFlags)
		{
		if(pair.actors[0] != expectedGround || pair.actors[1] != expectedSphere)
			{
			++unexpectedCalls;
			return;
			}
		++calls;
		events |= eventFlags;
		NxContactStreamIterator iterator(pair.stream);
		while(iterator.goNextPair())
			while(iterator.goNextPatch())
				{
				++patchCount;
				if(patchCount == 1)
					firstPatchPoints = iterator.getNumPoints();
				const NxVec3 normal = iterator.getPatchNormal();
				if(patchCount == 2)
					{
					secondNormal[0] = nxFloatBits(normal.x);
					secondNormal[1] = nxFloatBits(normal.y);
					secondNormal[2] = nxFloatBits(normal.z);
					}
				while(iterator.goNextPoint())
					{
					const NxVec3 point = iterator.getPoint();
					if(pointCount == 0)
						{
						firstPoint[0] = nxFloatBits(point.x);
						firstPoint[1] = nxFloatBits(point.y);
						firstPoint[2] = nxFloatBits(point.z);
						firstNormal[0] = nxFloatBits(normal.x);
						firstNormal[1] = nxFloatBits(normal.y);
						firstNormal[2] = nxFloatBits(normal.z);
						firstSeparation = nxFloatBits(iterator.getSeparation());
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
	printf("simulation mesh-contact calls=%u events=%08x patches=%u firstPatchPoints=%u points=%u normal=%08x.%08x.%08x secondNormal=%08x.%08x.%08x separation=%08x y=%08x vy=%08x\n",
		report.calls, report.events, report.patchCount, report.firstPatchPoints, report.pointCount,
		report.firstNormal[0], report.firstNormal[1], report.firstNormal[2],
		report.secondNormal[0], report.secondNormal[1], report.secondNormal[2],
		report.firstSeparation, nxFloatBits(position.y), nxFloatBits(velocity.y));

	// The indexed triangles wind their front faces upward. The shipped sphere/
	// mesh row rejects an overlapping sphere below an ordinary mesh.
	sphere->setGlobalPosition(NxVec3(0.0f, -0.25f, 0.0f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	report.calls = 0;
	report.events = 0;
	report.patchCount = 0;
	report.firstPatchPoints = 0;
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
	printf("simulation mesh-edge steps=%u calls=%u events=%08x points=%u point=%08x.%08x.%08x normal=%08x.%08x.%08x separation=%08x y=%08x vy=%08x\n",
		edgeSteps, report.calls, report.events, report.pointCount,
		report.firstPoint[0], report.firstPoint[1], report.firstPoint[2],
		report.firstNormal[0], report.firstNormal[1], report.firstNormal[2],
		report.firstSeparation, nxFloatBits(position.y), nxFloatBits(velocity.y));

	// The sphere center lies beyond the mesh corner, so the nearest feature is
	// the single shared vertex rather than an interior face or long edge.
	sphere->setGlobalPosition(NxVec3(2.25f, 1.5f, 2.25f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	report.calls = 0;
	report.events = 0;
	report.patchCount = 0;
	report.firstPatchPoints = 0;
	report.pointCount = 0;
	unsigned vertexSteps = 0;
	for(; vertexSteps < 60; ++vertexSteps)
		{
		scene->simulate(1.0f / 60.0f);
		if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("vertex mesh-contact simulation results failed");
		if(report.calls != 0)
			{
			++vertexSteps;
			break;
			}
		}
	sphere->getGlobalPosition(position);
	sphere->getLinearVelocity(velocity);
	printf("simulation mesh-vertex steps=%u calls=%u events=%08x patches=%u points=%u point=%08x.%08x.%08x normal=%08x.%08x.%08x separation=%08x y=%08x vy=%08x\n",
		vertexSteps, report.calls, report.events, report.patchCount, report.pointCount,
		report.firstPoint[0], report.firstPoint[1], report.firstPoint[2],
		report.firstNormal[0], report.firstNormal[1], report.firstNormal[2],
		report.firstSeparation, nxFloatBits(position.y), nxFloatBits(velocity.y));
	sphere->setGlobalPosition(NxVec3(-30.0f, 20.0f, 20.0f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));

	// Rotate the mesh 90 degrees and translate it. With gravity disabled, an
	// overlapping sphere on the transformed front face exposes point-space errors.
	NxMat34 rotatedMeshPose;
	rotatedMeshPose.id();
	rotatedMeshPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
	rotatedMeshPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
	rotatedMeshPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	rotatedMeshPose.t = NxVec3(-10.0f, 5.0f, 0.0f);
	ground->setGlobalPose(rotatedMeshPose);
	scene->setGravity(NxVec3(0.0f, 0.0f, 0.0f));
	sphere->setGlobalPosition(NxVec3(-10.25f, 5.0f, 0.0f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	report.calls = 0;
	report.events = 0;
	report.patchCount = 0;
	report.firstPatchPoints = 0;
	report.pointCount = 0;
	scene->simulate(1.0f / 60.0f);
	if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
		|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
		return nxFail("transformed mesh-contact simulation results failed");
	printf("simulation mesh-transformed calls=%u events=%08x points=%u point=%08x.%08x.%08x\n",
		report.calls, report.events, report.pointCount,
		report.firstPoint[0], report.firstPoint[1], report.firstPoint[2]);
	sphere->setGlobalPosition(NxVec3(-30.0f, 20.0f, 20.0f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	rotatedMeshPose.id();
	rotatedMeshPose.t = NxVec3(0.0f, 0.0f, 0.0f);
	ground->setGlobalPose(rotatedMeshPose);
	scene->setGravity(NxVec3(0.0f, -9.81f, 0.0f));

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

	// Heightfields extend collision below their surface by the configured
	// vertical extent. This distinguishes the heightfield branch from the
	// ordinary-mesh front-face rejection above.
	NxTriangleMeshDesc heightfieldDesc = meshDesc;
	heightfieldDesc.heightFieldVerticalAxis = NX_Y;
	heightfieldDesc.heightFieldVerticalExtent = -100.0f;
	NxTriangleMesh* const heightfieldMesh = sdk->createTriangleMesh(heightfieldDesc);
	if(!heightfieldMesh)
		return nxFail("heightfield mesh creation failed");
	NxTriangleMeshShapeDesc heightfieldShape;
	heightfieldShape.meshData = heightfieldMesh;
	NxActorDesc heightfieldActorDesc;
	heightfieldActorDesc.shapes.pushBack(&heightfieldShape);
	NxActor* const heightfieldActor = scene->createActor(heightfieldActorDesc);
	if(!heightfieldActor)
		return nxFail("heightfield actor creation failed");
	heightfieldActor->setGroup(7);
	NxMeshContactReport heightfieldReport(sphere, heightfieldActor);
	scene->setUserContactReport(&heightfieldReport);
	scene->setGravity(NxVec3(0.0f, 0.0f, 0.0f));
	sphere->setGlobalPosition(NxVec3(0.0f, -0.25f, 0.0f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	scene->simulate(1.0f / 60.0f);
	if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
		|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
		return nxFail("heightfield mesh-contact simulation results failed");
	sphere->getGlobalPosition(position);
	sphere->getLinearVelocity(velocity);
	printf("simulation mesh-heightfield calls=%u unexpected=%u events=%08x patches=%u points=%u point=%08x.%08x.%08x normal=%08x.%08x.%08x separation=%08x y=%08x vy=%08x\n",
		heightfieldReport.calls, heightfieldReport.unexpectedCalls, heightfieldReport.events,
		heightfieldReport.patchCount, heightfieldReport.pointCount,
		heightfieldReport.firstPoint[0], heightfieldReport.firstPoint[1], heightfieldReport.firstPoint[2],
		heightfieldReport.firstNormal[0], heightfieldReport.firstNormal[1], heightfieldReport.firstNormal[2],
		heightfieldReport.firstSeparation, nxFloatBits(position.y), nxFloatBits(velocity.y));

	// A non-coplanar heightfield makes the smooth-sphere flag observable: the
	// normal at the shared edge blends the two face normals instead of selecting
	// only the contacted face normal.
	const NxPoint smoothVertices[] = {
		NxPoint(-2.0f, 0.0f, -2.0f), NxPoint(2.0f, 0.0f, -2.0f),
		NxPoint(-2.0f, 0.0f, 2.0f), NxPoint(2.0f, 2.0f, 2.0f)
		};
	const NxU32 smoothTriangles[] = { 0, 2, 1, 1, 2, 3 };
	NxTriangleMeshDesc smoothHeightfieldDesc;
	smoothHeightfieldDesc.numVertices = sizeof(smoothVertices) / sizeof(smoothVertices[0]);
	smoothHeightfieldDesc.numTriangles = 2;
	smoothHeightfieldDesc.pointStrideBytes = sizeof(NxPoint);
	smoothHeightfieldDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	smoothHeightfieldDesc.points = smoothVertices;
	smoothHeightfieldDesc.triangles = smoothTriangles;
	smoothHeightfieldDesc.heightFieldVerticalAxis = NX_Y;
	smoothHeightfieldDesc.heightFieldVerticalExtent = -100.0f;
	NxTriangleMesh* const smoothHeightfieldMesh = sdk->createTriangleMesh(smoothHeightfieldDesc);
	if(!smoothHeightfieldMesh)
		return nxFail("smooth heightfield mesh creation failed");
	NxTriangleMeshShapeDesc smoothHeightfieldShape;
	smoothHeightfieldShape.meshData = smoothHeightfieldMesh;
	smoothHeightfieldShape.meshFlags = NX_MESH_SMOOTH_SPHERE_COLLISIONS;
	NxActorDesc smoothHeightfieldActorDesc;
	smoothHeightfieldActorDesc.shapes.pushBack(&smoothHeightfieldShape);
	NxActor* const smoothHeightfieldActor = scene->createActor(smoothHeightfieldActorDesc);
	if(!smoothHeightfieldActor)
		return nxFail("smooth heightfield actor creation failed");
	smoothHeightfieldActor->setGroup(7);
	NxMeshContactReport smoothHeightfieldReport(sphere, smoothHeightfieldActor);
	scene->setUserContactReport(&smoothHeightfieldReport);
	sphere->setGlobalPosition(NxVec3(0.5f, 0.7f, 0.5f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	scene->simulate(1.0f / 60.0f);
	if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
		|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
		return nxFail("smooth heightfield mesh-contact simulation results failed");
	sphere->getGlobalPosition(position);
	sphere->getLinearVelocity(velocity);
	printf("simulation mesh-heightfield-smooth calls=%u unexpected=%u events=%08x patches=%u points=%u point=%08x.%08x.%08x normal=%08x.%08x.%08x separation=%08x\n",
		smoothHeightfieldReport.calls, smoothHeightfieldReport.unexpectedCalls,
		smoothHeightfieldReport.events, smoothHeightfieldReport.patchCount,
		smoothHeightfieldReport.pointCount, smoothHeightfieldReport.firstPoint[0],
		smoothHeightfieldReport.firstPoint[1], smoothHeightfieldReport.firstPoint[2],
		smoothHeightfieldReport.firstNormal[0], smoothHeightfieldReport.firstNormal[1],
		smoothHeightfieldReport.firstNormal[2], smoothHeightfieldReport.firstSeparation);
	printf("simulation mesh-heightfield-smooth-state position=%08x.%08x.%08x velocity=%08x.%08x.%08x\n",
		nxFloatBits(position.x), nxFloatBits(position.y), nxFloatBits(position.z),
		nxFloatBits(velocity.x), nxFloatBits(velocity.y), nxFloatBits(velocity.z));

	// Move the same smooth heightfield through an exact quarter-turn and
	// translation to exercise local-to-world normal and contact-point handling.
	NxMat34 smoothPose;
	smoothPose.id();
	smoothPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
	smoothPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
	smoothPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	smoothPose.t = NxVec3(2.0f, 1.0f, -1.0f);
	smoothHeightfieldActor->setGlobalPose(smoothPose);
	sphere->setGlobalPosition(NxVec3(1.3f, 1.5f, -0.5f));
	sphere->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
	smoothHeightfieldReport.calls = 0;
	smoothHeightfieldReport.unexpectedCalls = 0;
	smoothHeightfieldReport.events = 0;
	smoothHeightfieldReport.patchCount = 0;
	smoothHeightfieldReport.firstPatchPoints = 0;
	smoothHeightfieldReport.pointCount = 0;
	scene->simulate(1.0f / 60.0f);
	if(!scene->checkResults(NX_RIGID_BODY_FINISHED, true)
		|| !scene->fetchResults(NX_RIGID_BODY_FINISHED, true))
		return nxFail("transformed smooth heightfield simulation results failed");
	sphere->getGlobalPosition(position);
	sphere->getLinearVelocity(velocity);
	printf("simulation mesh-heightfield-smooth-transformed calls=%u unexpected=%u events=%08x patches=%u points=%u point=%08x.%08x.%08x normal=%08x.%08x.%08x separation=%08x\n",
		smoothHeightfieldReport.calls, smoothHeightfieldReport.unexpectedCalls,
		smoothHeightfieldReport.events, smoothHeightfieldReport.patchCount,
		smoothHeightfieldReport.pointCount, smoothHeightfieldReport.firstPoint[0],
		smoothHeightfieldReport.firstPoint[1], smoothHeightfieldReport.firstPoint[2],
		smoothHeightfieldReport.firstNormal[0], smoothHeightfieldReport.firstNormal[1],
		smoothHeightfieldReport.firstNormal[2], smoothHeightfieldReport.firstSeparation);
	printf("simulation mesh-heightfield-smooth-transformed-state position=%08x.%08x.%08x velocity=%08x.%08x.%08x\n",
		nxFloatBits(position.x), nxFloatBits(position.y), nxFloatBits(position.z),
		nxFloatBits(velocity.x), nxFloatBits(velocity.y), nxFloatBits(velocity.z));
	fflush(stdout);

	sdk->setActorGroupPairFlags(7, 3, 0);
	scene->releaseActor(*smoothHeightfieldActor);
	sdk->releaseTriangleMesh(*smoothHeightfieldMesh);
	scene->releaseActor(*heightfieldActor);
	sdk->releaseTriangleMesh(*heightfieldMesh);
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
