#ifndef NX_PHYSICS_TRIANGLEMESH
#define NX_PHYSICS_TRIANGLEMESH
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"
#include "MemoryStream.h"

// The model pointer is typed as the vendored BaseModel, whose seven-slot
// vtable -- dtor pair, GetUsedBytes, Refit, and the three NovodeX additions
// ending in Save -- NxPhysicsThirdPartyTests asserts against the image.
#include "Opcode.h"

class NxStream;
class Adjacencies;
class EdgeList;

/**
The triangle-mesh stream format's reader and writer, and ONLY the parts of them
this project has established against the shipped DLL.

The reader is phys_fn_002262 at 0x00055cb0, 511 bytes, slot 17 of the
nineteen-slot TriangleMesh vtable at .rdata:0x00108608; the writer is
phys_fn_002162 at 0x000539d0, 413 bytes, slot 18 immediately beside it. The
nineteen fields are written up in
docs/reconstruction/novodex-physics/evidence/phase4-formats.md, recovered from
both sides.

WHAT THE READER RECONSTRUCTS HERE IS FIELDS 1 AND 2 AND NOTHING ELSE.

The two tags are read as dwords through NxStream slot +0x0c and compared against
0x4e585354 and 0x4d455348 -- `cmp eax,0x4e585354` at 0x00055cc2 and
`cmp eax,0x4d455348` at 0x00055cd0. Either failure is the same `xor al,al` at
0x00055cd8, so the row reads ONE dword when the first tag is wrong and TWO when
the second is, which is what says where it stopped rather than only that it
refused. Neither reject arm touches `this`: the first store into the object is
`fstp dword ptr [edi+0x6c]` at 0x00055cfa, past both tests.

Fields 3 to 19 are NOT RECONSTRUCTED. The accept arm allocates the vertex and
triangle arrays through the Foundation SDK allocator at 0x101041bc -- null until
an NxPhysicsSDK exists -- and then builds an OPCODE model out of field 19 and a
convex hull out of the flag bits. Rather than answer for an arm it has not
reconstructed, this entry says so, which is what the second return value below
is.
*/
enum NxTriangleMeshHeader
	{
	//! Either tag test failed; the reader returned false.
	NX_TRIANGLE_MESH_HEADER_REJECTED		= 0,
	//! Both tags matched. The remaining seventeen fields are not reconstructed.
	NX_TRIANGLE_MESH_HEADER_NOT_RECONSTRUCTED	= 1
	};

//! phys_fn_002262 (0x00055cb0), fields 1 and 2.
NxTriangleMeshHeader nxTriangleMeshReadHeader(const NxStream& stream);

/**
The InternalTriangleMesh layout, from the allocation sites
(evidence/phase4-formats.md, "The InternalTriangleMesh layout"):

	+0x00 vertex count        store 0x00051fa7
	+0x04 triangle count      store 0x00051fd7
	+0x08 vertices            allocate 0x00051fbc, store 0x00051fbf
	+0x0c triangles           allocate 0x00051fed, store 0x00051ff0
	+0x10 material indices    allocate 0x0005201b, store 0x0005201e
	+0x14 face remap          allocate 0x0005204c, store 0x0005204f
	+0x18 vertex normals      allocate 0x00052257, store 0x00052269
	+0x20 the OPCODE model    released and cleared 0x00052296, installed 0x00052360
	+0x24 the embedded MeshInterface region

The writer proves +0x08/+0x0c/+0x10/+0x14 from the second side: phys_fn_002162
reads them at TriangleMesh+0x08/+0x0c/+0x10/+0x14/+0x18/+0x1c because the
internal mesh is embedded at TriangleMesh+0x08 (`lea ebp,[edi+8]` at
0x00055d18).

The region from +0x24 onward belongs to the MeshInterface and whatever follows
it; nothing the writer or the reconstructed reader touches reaches past
internal+0x20, so it is carried as opaque bytes here and named rather than
invented.
*/
struct InternalTriangleMesh
	{
	NxU32					mVertexCount;		//!< +0x00
	NxU32					mTriangleCount;		//!< +0x04
	void*					mVertices;			//!< +0x08, 12 bytes each
	void*					mTriangles;			//!< +0x0c, 12 bytes each, 32-bit indices
	NxU16*					mMaterialIndices;	//!< +0x10, 2 bytes each, optional
	NxU32*					mFaceRemap;			//!< +0x14, 4 bytes each, optional
	void*					mVertexNormals;		//!< +0x18, 12 bytes each
	NxU32					mWord1C;			//!< +0x1c, unestablished; the allocation-site table jumps from +0x18 to +0x20
	Opcode::BaseModel*		mModel;				//!< +0x20
	NxU8					mInterfaceRegion[0x14];	//!< +0x24, the MeshInterface region, unestablished
	};

/**
The TriangleMesh layout at the offsets the two stream rows touch. The class is
polymorphic in the image -- slot 17 of the nineteen-slot vtable at .rdata
0x00108608 is the reader and slot 18 the writer -- but this reconstruction
drives the rows directly, so the vtable pointer is carried as an opaque word
rather than reproduced as a C++ vtable that would not match the oracle's
nineteen slots.

Every offset below is a measurement, not a guess; each carries the address that
established it.

	+0x00 vtable              .rdata:0x00108608, nineteen slots
	+0x08 internal mesh       `lea ebp,[edi+8]` 0x00055d18
	+0x40 hull construction   `test byte ptr [edi+0x40],1` 0x00053a27; bit 0 is
	                          which phys_fn_002164 construction ran (set at
	                          0x00053bd2, cleared at 0x00053b97/0x00053b9c)
	+0x6c convex threshold    ctor writes 0.001f at 0x000554d4; readFloat store
	                          0x00055cfa; writer push 0x00053a3d
	+0x7c height-field axis   0xff from setToDefault at 0x000540e9; store 0x00055d04
	+0x80 height-field extent 0 from setToDefault at 0x000540f0; fstp 0x00055d0e
	+0x8c presence flag A     readDword store 0x00055da5; writer push 0x00053acf
	+0x90 presence flag B     readDword store 0x00055db2; writer push 0x00053ade
	+0x94 array A             allocate 0x00055dd5; writer test 0x00053ae9
	+0x98 array B             allocate 0x00055e0a; writer test 0x00053b02
	+0xa0 the convex mesh     phys_fn_002164 releases slot 0 at 0x00053b84 and
	                          installs at 0x00053bc6; writer test 0x00053a22
*/
class TriangleMesh
	{
	public:
	//! phys_fn_002162 (0x000539d0), the whole of the writer. Returns the
	//! literal 1; there is no error path in it (mov al,1 at 0x00053b64).
	bool					save(NxStream& stream) const;

	//! phys_fn_002186 (0x000543d0) and phys_fn_002188 (0x00054460): build the
	//! adjacencies (+0x84) and the edge list (+0x88) of the triangles.
	void					createAdjacencies();
	void					createEdgeList();

	//! +0x00, the vtable slot. Not a C++ vtable; see the class comment.
	void*					mVtableSlot;
	//! +0x04, unestablished.
	NxU32					mWord04;
	//! +0x08, the embedded internal mesh -- which reaches exactly to +0x40.
	InternalTriangleMesh	mInternal;
	//! +0x40, the hull-construction flags. Only bit 0 is established.
	NxU32					mHullFlags;
	//! +0x44..+0x68, unestablished.
	NxU8					mGap44[0x28];
	//! +0x6c, NxTriangleMeshDesc::convexEdgeThreshold's image value.
	float					mConvexEdgeThreshold;
	//! +0x70..+0x78, unestablished.
	NxU8					mGap70[0x0c];
	//! +0x7c, heightFieldVerticalAxis; 0xff is NX_NOT_HEIGHTFIELD.
	NxU32					mHeightFieldVerticalAxis;
	//! +0x80, heightFieldVerticalExtent.
	float					mHeightFieldVerticalExtent;
	//! +0x84, the adjacencies, built on demand by phys_fn_002186
	//! (createAdjacencies, TriangleMeshTopology.cpp). The mesh/height-field
	//! pass 001859 builds them when the word is 0 and stores 1 when that fails
	//! (0x00044b8e..0x00044bb1), so 1 means "could not be built".
	Adjacencies*			mAdjacencies;
	//! +0x88, the edge list, built on demand by phys_fn_002188
	//! (createEdgeList, TriangleMeshTopology.cpp) when 001834 finds it null
	//! (0x00041c23..0x00041c31).
	EdgeList*				mEdgeList;
	//! +0x8c, presence flag A for the array at +0x94.
	NxU32					mPresenceFlagA;
	//! +0x90, presence flag B for the array at +0x98.
	NxU32					mPresenceFlagB;
	//! +0x94, triangleCount dwords when flag A is non-zero.
	NxU32*					mArrayA;
	//! +0x98, triangleCount dwords when flag B is non-zero.
	NxU32*					mArrayB;
	//! +0x9c, unestablished; the next measured store is the hull at +0xa0.
	NxU32					mWord9C;
	//! +0xa0, the convex mesh. Released through its slot 0 by
	//! phys_fn_002164; no type is established beyond that.
	void*					mConvexMesh;
	};

// The measured offsets, pinned so a field added in the wrong place fails here
// rather than in a differential.
static_assert(offsetof(TriangleMesh, mVtableSlot) == 0x00, "the vtable slot is first");
static_assert(offsetof(TriangleMesh, mInternal) == 0x08, "the internal mesh is embedded at +0x08");
static_assert(offsetof(TriangleMesh, mInternal.mVertexCount) == 0x08, "vertex count is internal+0x00");
static_assert(offsetof(TriangleMesh, mInternal.mTriangleCount) == 0x0c, "triangle count is internal+0x04");
static_assert(offsetof(TriangleMesh, mInternal.mVertices) == 0x10, "vertices are internal+0x08");
static_assert(offsetof(TriangleMesh, mInternal.mTriangles) == 0x14, "triangles are internal+0x0c");
static_assert(offsetof(TriangleMesh, mInternal.mMaterialIndices) == 0x18, "material indices are internal+0x10");
static_assert(offsetof(TriangleMesh, mInternal.mFaceRemap) == 0x1c, "face remap is internal+0x14");
static_assert(offsetof(TriangleMesh, mInternal.mVertexNormals) == 0x20, "vertex normals are internal+0x18");
static_assert(offsetof(TriangleMesh, mInternal.mModel) == 0x28, "the model is internal+0x20 / TriangleMesh+0x28");
static_assert(offsetof(TriangleMesh, mHullFlags) == 0x40, "the hull flags are at +0x40");
static_assert(offsetof(TriangleMesh, mConvexEdgeThreshold) == 0x6c, "the threshold is at +0x6c");
static_assert(offsetof(TriangleMesh, mHeightFieldVerticalAxis) == 0x7c, "the height-field axis is at +0x7c");
static_assert(offsetof(TriangleMesh, mHeightFieldVerticalExtent) == 0x80, "the height-field extent is at +0x80");
static_assert(offsetof(TriangleMesh, mAdjacencies) == 0x84, "the adjacencies are at +0x84");
static_assert(offsetof(TriangleMesh, mEdgeList) == 0x88, "the edge list is at +0x88");
static_assert(offsetof(TriangleMesh, mPresenceFlagA) == 0x8c, "presence flag A is at +0x8c");
static_assert(offsetof(TriangleMesh, mPresenceFlagB) == 0x90, "presence flag B is at +0x90");
static_assert(offsetof(TriangleMesh, mArrayA) == 0x94, "array A is at +0x94");
static_assert(offsetof(TriangleMesh, mArrayB) == 0x98, "array B is at +0x98");
static_assert(offsetof(TriangleMesh, mConvexMesh) == 0xa0, "the convex mesh is at +0xa0");
#endif
