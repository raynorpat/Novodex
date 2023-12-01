#ifndef NX_PHYSICS_NXRAYCAST
#define NX_PHYSICS_NXRAYCAST

#include "Nxp.h"
class NxShape;

enum NxShapesType
{
	NX_STATIC_SHAPES	= (1<<0),								//!< Hits static shapes
	NX_DYNAMIC_SHAPES	= (1<<1),								//!< Hits dynamic shapes
	NX_ALL_SHAPES		= NX_STATIC_SHAPES|NX_DYNAMIC_SHAPES	//!< Hits both static & dynamic shapes
};

/**
The user needs to pass an instance of this class to several of the ray casting routines in
NxScene. Its onHit method will be called for each shape that the ray intersects.
*/
class NxUserRaycastReport
	{
	public:

	/**
	This method is called for each shape hit by the raycast. worldImpact is the world space
	impact point, and distance is the distance between the ray origin and the impact point
	along the ray. If onHit returns true, it may be called again with the next shape that
	was stabbed. If it returns false, no further shapes are returned, and the raycast is concluded.
	*/
	virtual bool onHit(NxShape&, const NxVec3* worldImpact=NULL, const NxF32* distance=NULL) = 0;
	};

#endif
