// Isolates the controller correction query where a triangle face crosses the
// swept controller footprint without a controller corner ray hitting it.
#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxTriangleMeshShapeDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

class NxGroupsMask;
class NxControllerProbe
	{
	public:
	virtual ~NxControllerProbe() {}
	virtual void move(const NxVec3&, NxU32, NxReal, NxU32&, NxReal = 1.0f,
		const NxGroupsMask* = 0) = 0;
	virtual const NxVec3& getPosition() const = 0;
	};

static unsigned nxFloatBits(NxReal value)
	{
	unsigned bits = 0;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsControllerSweepFaceTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk) return nxFail("SDK creation failed");

	// The small rising face is wholly inside the controller footprint. A
	// downward sweep can cross its interior while all eight controller corners
	// remain outside the triangle's projected area.
	const NxPoint points[] = {
		NxPoint(0.35f, 0.4f, 0.35f), NxPoint(0.35f, 0.4f, 0.65f),
		NxPoint(0.65f, 0.6f, 0.35f), NxPoint(0.65f, 0.6f, 0.65f)};
	const NxU32 triangles[] = {0, 1, 2, 1, 3, 2};
	NxTriangleMeshDesc meshDesc;
	meshDesc.numVertices = 4;
	meshDesc.numTriangles = 2;
	meshDesc.pointStrideBytes = sizeof(NxPoint);
	meshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	meshDesc.points = points;
	meshDesc.triangles = triangles;
	NxTriangleMesh* mesh = sdk->createTriangleMesh(meshDesc);
	if(!mesh) return nxFail("controller face mesh cooking failed");

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("controller face scene creation failed");
	NxTriangleMeshShapeDesc shape;
	shape.meshData = mesh;
	NxActorDesc obstacleDesc;
	obstacleDesc.shapes.pushBack(&shape);
	NxActor* obstacle = scene->createActor(obstacleDesc);

	alignas(4) unsigned char controllerStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(controllerStorage + 0x0c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x18) = 1;
	*reinterpret_cast<NxU32*>(controllerStorage + 0x1c) = nxFloatBits(0.95f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(controllerStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x38) = nxFloatBits(0.5f);
	NxController* controller = scene->createController(
		*reinterpret_cast<const NxControllerDesc*>(controllerStorage));
	if(!obstacle || !controller)
		return nxFail("controller face fixture setup failed");
	const NxU32 actorsAfterControllerCreate = scene->getNbActors();
	printf("controller correction_face actors=%u\n", actorsAfterControllerCreate);
	if(actorsAfterControllerCreate != 2)
		return nxFail("box controller actor was not accepted");
	NxU32 collisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(controller)->move(
		NxVec3(0.0f, -3.0f, 0.0f), 0xffffffff, 0.001f, collisionFlags);
	const NxVec3& position = reinterpret_cast<NxControllerProbe*>(controller)->getPosition();
	printf("controller correction_face position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(position.x), nxFloatBits(position.y), nxFloatBits(position.z),
		collisionFlags);

	scene->releaseController(*controller);
	sdk->releaseScene(*scene);
	sdk->releaseTriangleMesh(*mesh);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}
