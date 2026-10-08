#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxPlaneShape.h"
#include "NxPlaneShapeDesc.h"
#include "NxBounds3.h"
#include <stdio.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

static bool nxCheckPlane(NxActor* actor, const NxVec3& normal, NxReal distance)
	{
	if(!actor || actor->isDynamic() || actor->getNbShapes() != 1)
		return false;
	NxShape** const shapes = actor->getShapes();
	if(!shapes || !shapes[0])
		return false;
	NxPlaneShape* const plane = shapes[0]->isPlane();
	if(!plane)
		return false;
	NxPlaneShapeDesc desc;
	if(!plane->saveToDesc(desc))
		return false;
	return desc.normal.x == normal.x && desc.normal.y == normal.y
		&& desc.normal.z == normal.z && desc.d == distance;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSceneBoundsPlanesTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk) return nxFail("SDK creation failed");

	NxBounds3 bounds;
	bounds.set(-10.0f, -20.0f, -30.0f, 11.0f, 22.0f, 33.0f);
	NxSceneDesc desc;
	desc.setToDefault();
	desc.maxBounds = &bounds;
	desc.boundsPlanes = true;
	NxScene* scene = sdk->createScene(desc);
	if(!scene)
		{
		sdk->release();
		return nxFail("bounded scene creation failed");
		}

	const NxU32 actorCount = scene->getNbActors();
	printf("scene bounds_planes actors=%u\n", actorCount);
	if(actorCount != 6)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		return nxFail("boundsPlanes did not create six boundary actors");
		}

	NxActor* const* actors = scene->getActors();
	if(!actors)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		return nxFail("boundsPlanes actor array is null");
		}
	const NxVec3 normals[6] = {
		NxVec3(-1.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f),
		NxVec3(0.0f, -1.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f),
		NxVec3(0.0f, 0.0f, -1.0f), NxVec3(0.0f, 0.0f, 1.0f) };
	const NxReal distances[6] = { -11.0f, -10.0f, -22.0f, -20.0f, -33.0f, -30.0f };
	for(NxU32 i = 0; i < actorCount; ++i)
		if(!nxCheckPlane(actors[i], normals[i], distances[i]))
			{
			sdk->releaseScene(*scene);
			sdk->release();
			return nxFail("boundsPlanes plane actor differs from its AABB face");
			}
	printf("scene bounds_planes verified=6 faces=xmax,xmin,ymax,ymin,zmax,zmin\n");

	sdk->releaseScene(*scene);

	desc.setToDefault();
	desc.groundPlane = true;
	scene = sdk->createScene(desc);
	if(!scene || scene->getNbActors() != 1)
		{
		if(scene) sdk->releaseScene(*scene);
		sdk->release();
		return nxFail("groundPlane did not create one default plane actor");
		}
	actors = scene->getActors();
	if(!actors || !nxCheckPlane(actors[0], NxVec3(0.0f, 1.0f, 0.0f), 0.0f))
		{
		sdk->releaseScene(*scene);
		sdk->release();
		return nxFail("groundPlane differs from the default y=0 plane");
		}
	printf("scene ground_plane verified=1 y=0\n");
	sdk->releaseScene(*scene);

	desc.setToDefault();
	desc.maxBounds = &bounds;
	desc.boundsPlanes = true;
	desc.groundPlane = true;
	scene = sdk->createScene(desc);
	if(!scene || scene->getNbActors() != 7)
		{
		if(scene) sdk->releaseScene(*scene);
		sdk->release();
		return nxFail("combined groundPlane and boundsPlanes did not create seven actors");
		}
	actors = scene->getActors();
	if(!actors || !nxCheckPlane(actors[0], NxVec3(0.0f, 1.0f, 0.0f), 0.0f))
		{
		sdk->releaseScene(*scene);
		sdk->release();
		return nxFail("combined scene ground plane is not first");
		}
	for(NxU32 i = 0; i < 6; ++i)
		if(!nxCheckPlane(actors[i + 1], normals[i], distances[i]))
			{
			sdk->releaseScene(*scene);
			sdk->release();
			return nxFail("combined scene bounds planes do not follow ground plane");
			}
	printf("scene combined_planes verified=7 ground_then_bounds=1\n");
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}
