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

// The hull library phys_fn_002233 calls (qhull-gap Task 4).
#include "QhullHost.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxTriangleMesh.h"

class NxStream;
class NxPMap;
class Adjacencies;
class EdgeList;
class PenetrationMap;

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

The +0x1c word is an owned per-triangle allocation: phys_fn_002079 allocates
16 bytes per triangle there and phys_fn_002067 releases it. Its record contents
are still opaque. The embedded MeshInterface begins at +0x24; its first four
words are initialized by the constructor at 0x000e8fa0. The remaining bytes are
carried as opaque storage.
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
	void*					mTriangleData;		//!< +0x1c, 16 bytes per triangle, allocated by 002079 and released by 002067
	Opcode::BaseModel*		mModel;				//!< +0x20
	Opcode::MeshInterface	mMeshInterface;	//!< +0x24, OPCODE's four-word mesh interface
	void*					mInterfaceAllocation;	//!< +0x34; the TriangleMesh destructor frees it through the Foundation allocator
	};

// InternalTriangleMesh rows recovered from the allocation/teardown paths in
// gap__EdgeList.cpp__to__InternalTriangleMesh.cpp. Kept as explicit rows so
// TriangleMesh construction, load, and destruction can share the exact memory
// ownership rules without giving the measured data structure an invented C++
// vtable or destructor.
void nxInternalMeshInit(InternalTriangleMesh* mesh);                         // 002065
void nxInternalMeshRelease(InternalTriangleMesh* mesh);                      // 002067
void nxInternalMeshAllocateVertices(InternalTriangleMesh* mesh, NxU32 count); // 002069
void nxInternalMeshAllocateTriangles(InternalTriangleMesh* mesh, NxU32 count);// 002071
void nxInternalMeshAllocateMaterials(InternalTriangleMesh* mesh);             // 002073
void nxInternalMeshAllocateFaceRemap(InternalTriangleMesh* mesh);             // 002075
void nxInternalMeshBuildTriangleData(InternalTriangleMesh* mesh);              // 002079
bool nxInternalMeshBuildTopology(InternalTriangleMesh* mesh);                  // 002087
bool nxInternalMeshBuildModel(InternalTriangleMesh* mesh, NxU32 extendAxis, NxReal extendValue,
	const void* deserializeFrom);                                                  // 002083

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
	TriangleMesh();
	~TriangleMesh();
	//! phys_fn_002258 (oracle RVA 0x00055810): refuse to destroy a mesh with
	//! live shape instances; otherwise run the destructor and free this object.
	bool release();

	bool loadFromDesc(const NxTriangleMeshDesc& desc);
	bool buildModel();
	bool computeMassProperties();
	bool saveToDesc(NxTriangleMeshDesc& desc) const;
	NxU32 getCount(NxInternalArray array) const;
	NxInternalFormat getFormat(NxInternalArray array) const;
	const void* getBase(NxInternalArray array) const;
	NxU32 getStride(NxInternalArray array) const;
	NxTriangleMesh* publicHandle() const;
	bool loadPMap(const NxPMap& pmap);
	bool hasPMap() const;
	NxU32 getPMapSize() const;
	bool getPMapData(NxPMap& pmap) const;
	//! phys_fn_002162 (0x000539d0), the whole of the writer. Returns the
	//! literal 1; there is no error path in it (mov al,1 at 0x00053b64).
	bool					save(NxStream& stream) const;

	//! phys_fn_002186 (0x000543d0) and phys_fn_002188 (0x00054460): build the
	//! adjacencies (+0x84) and the edge list (+0x88) of the triangles.
	void					createAdjacencies();
	void					createEdgeList();

	//! +0x00, the vtable slot. Not a C++ vtable; see the class comment.
	void*					mVtableSlot;
	//! +0x04, the polygon interface table (0x101085d4: the constructor stores
	//! it at 0x000554a4 over the abstract table it stored at 0x00055493, the
	//! destructor again at 0x00055581). Its twelve slots take the mesh plus
	//! four as `this` and read the convex mesh at +0xa0; the candidate's slots
	//! are gTriangleMeshPolygonTable (TriangleMeshPolygons.cpp, convex-mesh gap
	//! Task 2g). The candidate constructor installs the table.
	const void* const*		mPolygonTable;
	//! +0x08, the embedded internal mesh -- which reaches exactly to +0x40.
	InternalTriangleMesh	mInternal;
	//! +0x40, the hull-construction flags. Only bit 0 is established.
	NxU32					mHullFlags;
	//! +0x44..+0x5b, local bounds used by mesh-shape AABB queries.
	float					mBounds44[6];
	//! +0x5c..+0x68, additional hull/mass metadata not yet named.
	NxU8					mGap5C[0x10];
	//! +0x6c, NxTriangleMeshDesc::convexEdgeThreshold's image value.
	float					mConvexEdgeThreshold;
	//! +0x70..+0x78, unestablished.
	NxU8					mGap70[4];
	NxU32					mReferenceCount;
	NxU32					mWord78;
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
	//! +0x9c, PenetrationMap, released by the TriangleMesh destructor.
	PenetrationMap*			mPMap;
	//! +0xa0, the convex mesh. Released by the mesh cleanup helper. The polygon
	//! interface reads it as the hull of ConvexHull.h (+0x0c..+0x48) with a
	//! vertex graph at +0x64 (002249).
	void*					mConvexMesh;
	//! +0xa4, +0xa8, and +0xac are opaque objects released through their
	//! virtual deleting-destructor slots by phys_fn_002253's cleanup helper.
	void*					mOwnedSlotA4;
	void*					mOwnedSlotA8;
	void*					mOwnedSlotAC;
	//! +0xb0, lazy mass/inertia cache consumed by MeshShape's mass path.
	float					mCachedMass;
	float					mCachedInertia[9];
	float					mCachedCenter[3];
	//! +0xe4, the separately allocated eight-byte NxTriangleMesh wrapper.
	void*					mPublicObject;
	};

// The measured offsets, pinned so a field added in the wrong place fails here
// rather than in a differential.
static_assert(offsetof(TriangleMesh, mVtableSlot) == 0x00, "the vtable slot is first");
static_assert(offsetof(TriangleMesh, mPolygonTable) == 0x04, "the polygon interface table is at +0x04");
static_assert(offsetof(TriangleMesh, mInternal) == 0x08, "the internal mesh is embedded at +0x08");
static_assert(offsetof(TriangleMesh, mInternal.mVertexCount) == 0x08, "vertex count is internal+0x00");
static_assert(offsetof(TriangleMesh, mInternal.mTriangleCount) == 0x0c, "triangle count is internal+0x04");
static_assert(offsetof(TriangleMesh, mInternal.mVertices) == 0x10, "vertices are internal+0x08");
static_assert(offsetof(TriangleMesh, mInternal.mTriangles) == 0x14, "triangles are internal+0x0c");
static_assert(offsetof(TriangleMesh, mInternal.mMaterialIndices) == 0x18, "material indices are internal+0x10");
static_assert(offsetof(TriangleMesh, mInternal.mFaceRemap) == 0x1c, "face remap is internal+0x14");
static_assert(offsetof(TriangleMesh, mInternal.mVertexNormals) == 0x20, "vertex normals are internal+0x18");
static_assert(offsetof(TriangleMesh, mInternal.mTriangleData) == 0x24, "per-triangle data pointer is internal+0x1c / TriangleMesh+0x24");
static_assert(offsetof(TriangleMesh, mInternal.mModel) == 0x28, "the model is internal+0x20 / TriangleMesh+0x28");
static_assert(offsetof(TriangleMesh, mInternal.mInterfaceAllocation) == 0x3c, "the destructor-owned interface allocation is at +0x3c");
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
static_assert(offsetof(TriangleMesh, mPMap) == 0x9c, "the penetration map is at +0x9c");
static_assert(offsetof(TriangleMesh, mConvexMesh) == 0xa0, "the convex mesh is at +0xa0");
static_assert(offsetof(TriangleMesh, mOwnedSlotA4) == 0xa4, "opaque deleting slot A is at +0xa4");
static_assert(offsetof(TriangleMesh, mOwnedSlotA8) == 0xa8, "opaque deleting slot B is at +0xa8");
static_assert(offsetof(TriangleMesh, mOwnedSlotAC) == 0xac, "opaque deleting slot C is at +0xac");
static_assert(offsetof(TriangleMesh, mPublicObject) == 0xe4, "the public wrapper is at +0xe4");
static_assert(sizeof(TriangleMesh) == 0xe8, "the SDK allocates a 0xe8-byte triangle mesh");

/**
TriangleMesh's first two virtuals, slots 0 and 1 of .rdata:0x00108608, are an
allocator interface -- malloc(size) (phys_fn_002235) and free(p)
(phys_fn_002237) -- and it is the interface the hull library takes as its user
allocator (Physics/src/include/QhullHost.h, HullAllocator). phys_fn_002233, the
hull computation loadFromDesc runs for NX_MF_COMPUTE_CONVEX, hands `this` to the
library as that allocator (`mov [esp+8],ecx` at 0x0005492e).

TriangleMesh above carries its vtable as an opaque word, so the three rows are
written on this base, which is exactly that interface plus the member that uses
it. Making TriangleMesh derive from it (so that slot 2 onward follows) belongs to
the deferred TriangleMesh/ConvexHull unit (units/convex-cooking-contract.md,
"Existing candidate code this replaces"). The name is descriptive; no original
identifier is evidenced.
*/
class TriangleMeshHullAllocator : public HullAllocator
	{
	public:
	//! phys_fn_002235 (0x00054a40), slot 0: the Foundation allocator's
	//! malloc(size, NX_MEMORY_PERSISTENT).
	virtual	void*			malloc(size_t size);
	//! phys_fn_002237 (0x00054a60), slot 1: the Foundation allocator's free,
	//! only for a non-null pointer.
	virtual	void			free(void* memory);

	//! phys_fn_002233 (0x00054920): the convex hull of desc's points, as a
	//! triangle mesh descriptor in `out`; false (and `out` untouched) when
	//! CreateConvexHull fails.
	bool					computeHull(const NxTriangleMeshDesc& desc, NxTriangleMeshDesc& out);
	};
//! The internal mesh's vertex normals (phys_fn_002081, 0x00052240), built on
//! demand (TriangleMeshTopology.cpp, convex-mesh gap Task 2i). Thiscall on the
//! InternalTriangleMesh with no argument; naked, so declared without parameters
//! and called from naked code (001844) or through a register thunk.
void nxMeshComputeVertexNormals();
#endif
