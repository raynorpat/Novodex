#ifndef NX_PHYSICS_CONVEXHULL_H
#define NX_PHYSICS_CONVEXHULL_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The convex hull the ConvexHull.cpp rows work on (prerequisite P-Hull of
// units/convex-mesh-gap-contract.md, Task 2f). Convex-mesh gap Task 2e writes
// one of its rows, 001461 (the vertex normals, P-Small), and only the fields
// that row reads are established here; the support-map rows (sub-unit B) read
// the centroid at +0x18 and the polygons at +0x24/+0x28, which Task 2f adds
// with the rest of the class.

#include "IceMeshTools.h"

class ConvexHull
	{
	public:
	bool				ComputeVertexNormals();

	NxU32				mWord00;			// +0x00, not read by 001461
	NxU32				mNbFaces;			// +0x04, 0x0002aedc
	const NxU16*		mFaces;				// +0x08, 16-bit triangles, 0x0002aee3
	NxU32				mNbVerts;			// +0x0c, 0x0002ae84 / 0x0002aeb0
	const IceMaths::Point*	mVerts;			// +0x10, 0x0002aed5
	IceMaths::Point*	mVertexNormals;		// +0x14, 0x0002ae66..0x0002aea4
	};

#endif
