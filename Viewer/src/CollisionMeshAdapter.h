/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef COLLISIONMESHADAPTER_H
#define COLLISIONMESHADAPTER_H

#include "NxPhysics.h"

/**
this gets assigned to the submesh's userData for user mesh access.
its purpose is to share mesh data between the graphics lib and collision detection.
it is persistent for the lifetime of the triangleMesh.  
*/
class CollisionMeshAdapter //: public MeshReadAdapter				
	{
	public:

	CollisionMeshAdapter();
	~CollisionMeshAdapter();

	bool setTriangleMesh(NxTriangleMesh&);
	void releaseTriangleMesh(NxPhysicsSDK &);
	NX_INLINE	NxTriangleMesh * getTriangleMesh()	{ return triangleMesh;	}

	private:
	NxTriangleMesh * triangleMesh;
	};

#endif