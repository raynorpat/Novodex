/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "glm.h"

#include "CollisionMeshAdapter.h"
#include "NxTriangleMesh.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxPhysicsSDK.h"

CollisionMeshAdapter::CollisionMeshAdapter() : triangleMesh(NULL)
	{
	}

bool CollisionMeshAdapter::setTriangleMesh(NxTriangleMesh& mesh)
	{
	triangleMesh = &mesh;
	return true;
	}

void CollisionMeshAdapter::releaseTriangleMesh(NxPhysicsSDK &sdk)
	{
	NX_ASSERT(triangleMesh);
	sdk.releaseTriangleMesh(*triangleMesh);
	triangleMesh = 0;
	}

CollisionMeshAdapter::~CollisionMeshAdapter()
	{
	NX_ASSERT(!triangleMesh);
	//if(triangleMesh)	triangleMesh->release();
	}
