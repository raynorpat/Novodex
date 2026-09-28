/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The convex-hull rows around `ConvexHull.cpp` (prerequisite P-Hull of
// units/convex-mesh-gap-contract.md). Convex-mesh gap Task 2e writes 001461 (in
// the P-Small list) from the Capstone listing; Task 2f adds the rest. The file
// name follows 001465's __FILE__; 001461 lies in the gap before it, so its
// placement here is a choice (the contract records it).
//
// Integer code apart from what it calls (001651, IceMeshTools.cpp); built
// /EHs-c- with the other ICE-shaped files (the listing is frameless).

#include "ConvexHull.h"

#include <string.h>

// phys_fn_001461 (0x0002ae60, 194 B)
// The hull's vertex normals, angle-weighted. The previous array (+0x14) is
// released through the 004803 getter's slot 3 and cleared; false when the hull
// has no vertices, or when the new array (slot 0, type 0, nbVerts * 12 bytes,
// stored into +0x14 before it is tested) is null. Otherwise a MeshNormals on
// the stack (its constructor, 001536) computes into that array (001651): the
// create block {nbVerts, vertices, nbFaces, no 32-bit faces, the 16-bit faces,
// weight by angle, face normals allocated by the object, the vertex normals
// given}; the object is released (001649, which frees the face normals) and
// 001651's result returned.
__declspec(noinline) bool ConvexHull::ComputeVertexNormals()
	{
	if(mVertexNormals)
		{
		nxIceFree(mVertexNormals);
		mVertexNormals = 0;
		}
	if(!mNbVerts)
		return false;
	mVertexNormals = (IceMaths::Point*) nxIceAlloc(mNbVerts * 12, NX_MEMORY_PERSISTENT);
	if(!mVertexNormals)
		return false;

	MESHNORMALSCREATE create;
	memset(&create, 0, sizeof(create));
	create.NbVerts = mNbVerts;
	create.Verts = mVerts;
	create.NbFaces = mNbFaces;
	create.DFaces = 0;
	create.WFaces = mFaces;
	create.WeightByAngle = true;
	create.FaceNormals = 0;
	create.VertexNormals = mVertexNormals;

	MeshNormals normals;
	const bool status = nxMeshNormalsCompute(&normals, 0, &create);
	return status;
	}
