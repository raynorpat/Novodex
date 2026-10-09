#ifndef NX_PHYSICS_ICEMESHTOOLS_H
#define NX_PHYSICS_ICEMESHTOOLS_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Sub-unit D of units/convex-mesh-gap-contract.md, the mesh utilities of
// Physics/src/IceMeshTools.cpp (a file name chosen by the contract: the rows
// carry no string). The valencies (001663, 001665, 001667) are written by
// convex-mesh gap Task 2c; the vertex reduction (001645, 001647, 001659) by
// Task 2d, for MeshBuilder2; the rest of the sub-unit (001639, 001641/001643,
// 001649, 001651, 001653, 001661) by Task 2e.

#include "EdgeList.h"

// The create block 001667 reads (0x00032610): +0x00 vertex count, +0x04 face
// count, +0x08 32-bit faces, +0x0c 16-bit faces, +0x10 a flag byte (build the
// adjacent-vertex lists).
struct VALENCESCREATE
	{
	NxU32			NbVerts;
	NxU32			NbFaces;
	const NxU32*	DFaces;
	const NxU16*	WFaces;
	bool			AdjacentList;
	};

// Per-vertex valence (0x14 bytes; 001663 zeroes all five words).
class Valencies
	{
	public:
				Valencies();
				~Valencies();

	bool		Compute(const VALENCESCREATE& create);

	NxU32		mNbVerts;		// +0x00
	NxU32		mNbAdjVerts;	// +0x04
	NxU32*		mValencies;		// +0x08
	NxU32*		mOffsets;		// +0x0c
	NxU32*		mAdjVerts;		// +0x10
	};

// What 001647 hands back (0x0003185e..0x00031876): +0x00 the reduced vertices,
// +0x04 their count, +0x08 the cross-reference (old vertex -> reduced vertex).
struct REDUCEDCLOUD
	{
	IceMaths::Point*	RVerts;
	NxU32				NbRVerts;
	NxU32*				XRef;
	};

// The vertex reduction (0x14 bytes; 001645 sets +0x00/+0x04 and zeroes the
// rest). The three owned blocks come from the 004803 getter (type 0) and are
// released by 001659 (+0x10, then +0x0c).
class ReducedVertices
	{
	public:
				ReducedVertices(const IceMaths::Point* verts, NxU32 nb_verts);
				~ReducedVertices();

	bool		Reduce(REDUCEDCLOUD* rc);

	NxU32					mNbVerts;		// +0x00
	const IceMaths::Point*	mVerts;			// +0x04
	NxU32					mNbRVerts;		// +0x08
	IceMaths::Point*		mRVerts;		// +0x0c
	NxU32*					mXRef;			// +0x10
	};

// The create block 001651 reads (0x000318d0..0x00031b13): +0x00 vertex count,
// +0x04 vertices (null: false at once), +0x08 face count, +0x0c 32-bit faces,
// +0x10 16-bit faces (neither: the references 0, 1, 2 for every face), +0x14 a
// byte (weight each face normal by the corner angle, phys_fn_002144), +0x18 and
// +0x1c the caller's face-normal and vertex-normal arrays (null: allocated, and
// then owned by the object). 001461 fills one on its stack.
struct MESHNORMALSCREATE
	{
	NxU32					NbVerts;
	const IceMaths::Point*	Verts;
	NxU32					NbFaces;
	const NxU32*			DFaces;
	const NxU16*			WFaces;
	bool					WeightByAngle;
	IceMaths::Point*		FaceNormals;
	IceMaths::Point*		VertexNormals;
	};

// Per-face and per-vertex normals (8 bytes). The constructor is the oracle's
// 0x0002dae0 (phys_fn_001536, zero +0x00 and +0x04), which the linker folded
// with Adjacencies' constructor: the product form is Adjacencies::Adjacencies
// (IceAdjacencies.cpp); here it is inline. The destructor is 001649, which
// releases the two arrays the object allocated through the 004803 getter's slot
// 3; arrays the caller supplied are never stored in it.
class MeshNormals
	{
	public:
				MeshNormals() : mFaceNormals(0), mVertexNormals(0)	{}
				~MeshNormals();

	IceMaths::Point*	mFaceNormals;		// +0x00
	IceMaths::Point*	mVertexNormals;		// +0x04
	};

// 001651, MeshNormals::Compute. The oracle's is thiscall on the object (`ret 4`);
// naked code cannot be a member, so it is __fastcall with the object in ecx, an
// unused edx and the create block on the stack, popped by the callee: the same
// registers and the same `ret 4`.
bool __fastcall nxMeshNormalsCompute(MeshNormals* normals, NxU32 edx, const MESHNORMALSCREATE* create);

// 001639: the function-static identity pair (a 3x3 identity at +0x00, a 4x4
// identity at +0x24; 100 bytes), initialised once behind a guard byte.
const NxF32* nxIceIdentityPoses();

// 001641 (with its continuation 001643): the closed outline of an edge list.
bool nxIceEdgeLoop(IceCore::Container& loop, const IceCore::Container& edges);

// 001653: the two relative poses of two frames (either may be null: identity).
void __cdecl nxIcePosePair(IceMaths::Matrix4x4* relative0, IceMaths::Matrix4x4* relative1,
	const IceMaths::Matrix4x4* pose0, const IceMaths::Matrix4x4* pose1);

// 001661: add a direction to a Container of axes unless it is within 0.9999 of
// one already there (the oracle's is thiscall on the Container, `ret 4`).
#if NX_PHYSICS_USE_X87
bool __fastcall nxIceAddUniqueAxis(IceCore::Container* axes, NxU32 edx, const IceMaths::Point* axis);
#else
bool nxIceAddUniqueAxis(IceCore::Container* axes, const IceMaths::Point* axis);
#endif

// 001657: reverse an array of dwords in place (cdecl: count, array; false when
// either is zero). ConvexHull.cpp's 001472 calls it.
bool nxIceReverseArray(NxU32 count, NxU32* array);

// 002144 (SmoothNormals.cpp): the corner angle, register convention (eax the
// vertex, edx the three indices, esi the vertices; st(0) the result). Called
// from legacy assembly; the portable declaration names ordinary typed inputs.
#include "NxSmoothNormalsAngle.h"

#endif
