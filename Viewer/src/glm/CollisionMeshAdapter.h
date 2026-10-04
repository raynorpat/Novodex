/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef COLLISIONMESHADAPTER_H
#define COLLISIONMESHADAPTER_H

#include "MeshReadAdapter.h"
#include "NxTriangleMeshInstance.h"

class NxTriangleMesh;
/**
this gets assigned to GraphicsLib's clModel3::userData for user mesh access.
its purpose is to share mesh data between the graphics lib and collision detection.
*/

class CollisionMeshAdapter : public MeshReadAdapter				
	{
	public:
	CollisionMeshAdapter(GLMmodel & glmModel, NxTriangleMesh & triangleMesh);
	~CollisionMeshAdapter();

	NxTriangleMeshInstanceShape * createTriangleMeshInstance();

	private:
	NxTriangleMesh & triangleMesh;
	};

#endif