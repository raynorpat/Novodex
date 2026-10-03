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
	NxMeshContactReport(NxActor* ground, NxActor* sphere)
		: expectedGround(ground), expectedSphere(sphere), calls(0), events(0) {}
	virtual void onContactNotify(NxContactPair& pair, NxU32 eventFlags)
		{
		if(pair.actors[0] != expectedGround || pair.actors[1] != expectedSphere)
			return;
		++calls;
		events |= eventFlags;
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

	sdk->setActorGroupPairFlags(7, 3, 0);
	scene->releaseActor(*sphere);
	scene->releaseActor(*ground);
	sdk->releaseScene(*scene);
	sdk->releaseTriangleMesh(*mesh);
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
