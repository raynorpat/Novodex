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

CollisionMeshAdapter::CollisionMeshAdapter(GLMmodel & gm, NxTriangleMesh & tm) : triangleMesh(tm), MeshReadAdapter(gm)
	{
	triangleMesh.setTriangleData(*this);
	}

CollisionMeshAdapter::~CollisionMeshAdapter()
	{
	triangleMesh.release();
	}

NxTriangleMeshInstanceShape * CollisionMeshAdapter::createTriangleMeshInstance()
	{
	return triangleMesh.createTriangleMeshInstance();
	}


