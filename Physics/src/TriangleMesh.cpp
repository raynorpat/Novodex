/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "TriangleMesh.h"
#include "TriangleMeshPolygons.h"

#include "NxStream.h"

#include <float.h>
#include <string.h>

TriangleMesh::TriangleMesh()
	{
	memset(this, 0, sizeof(*this));
	mVtableSlot = 0;
	mPolygonTable = gTriangleMeshPolygonTable;
	mBounds[0] = mBounds[1] = mBounds[2] = FLT_MAX;
	mBounds[3] = mBounds[4] = mBounds[5] = -FLT_MAX;
	mConvexEdgeThreshold = 0.001f;
	mHeightFieldVerticalAxis = NX_NOT_HEIGHTFIELD;
	mPublicMesh = NX_NEW(NxTriangleMeshAdapter)(this);
	}

bool TriangleMesh::loadFromDesc(const NxTriangleMeshDesc& desc)
	{
	if(!desc.isValid() || (desc.flags & (NX_MF_CONVEX | NX_MF_COMPUTE_CONVEX)))
		return false;

	// The ordinary indexed 32-bit route (002260) owns compact copies of both
	// caller arrays. The model-building continuation is deliberately kept
	// separate until the mesh interface and OPCODE tree are reconstructed.
	const bool indices16 = (desc.flags & NX_MF_16_BIT_INDICES) != 0;
	const NxU32 inputTriangleBytes = indices16 ? 3 * sizeof(NxU16) : sizeof(NxTriangle32);
	if(!desc.triangles || desc.triangleStrideBytes < inputTriangleBytes)
		return false;

	NxUserAllocator* const allocator = nxFoundationSDKAllocator;
	void* vertices = allocator->malloc(static_cast<size_t>(desc.numVertices) * sizeof(NxPoint), NX_MEMORY_PERSISTENT);
	void* triangles = allocator->malloc(static_cast<size_t>(desc.numTriangles) * sizeof(NxTriangle32), NX_MEMORY_PERSISTENT);
	if(!vertices || !triangles)
		{
		if(vertices) allocator->free(vertices);
		if(triangles) allocator->free(triangles);
		return false;
		}

	NxU8* const vertexOut = static_cast<NxU8*>(vertices);
	const NxU8* const vertexIn = static_cast<const NxU8*>(desc.points);
	NxU32* const remap = static_cast<NxU32*>(allocator->malloc(
		static_cast<size_t>(desc.numVertices) * sizeof(NxU32), NX_MEMORY_TEMP));
	if(!remap)
		{
		allocator->free(vertices);
		allocator->free(triangles);
		return false;
		}
	NxU32 uniqueVertices = 0;
	float bounds[6] = { FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for(NxU32 i = 0; i < desc.numVertices; ++i)
		{
		NxPoint point;
		memcpy(&point, vertexIn + i * desc.pointStrideBytes, sizeof(point));
		if(point.x < bounds[0]) bounds[0] = point.x;
		if(point.y < bounds[1]) bounds[1] = point.y;
		if(point.z < bounds[2]) bounds[2] = point.z;
		if(point.x > bounds[3]) bounds[3] = point.x;
		if(point.y > bounds[4]) bounds[4] = point.y;
		if(point.z > bounds[5]) bounds[5] = point.z;
		NxU32 existing = 0;
		for(; existing < uniqueVertices; ++existing)
			if(memcmp(vertexOut + existing * sizeof(NxPoint), &point, sizeof(point)) == 0)
				break;
		if(existing == uniqueVertices)
			{
			memcpy(vertexOut + uniqueVertices * sizeof(NxPoint), &point, sizeof(point));
				++uniqueVertices;
			}
		remap[i] = existing;
		}

	NxU8* const triangleOut = static_cast<NxU8*>(triangles);
	const NxU8* const triangleIn = static_cast<const NxU8*>(desc.triangles);
	for(NxU32 i = 0; i < desc.numTriangles; ++i)
		{
		NxTriangle32 triangle;
		if(indices16)
			{
			NxU16 inputTriangle[3];
			memcpy(inputTriangle, triangleIn + i * desc.triangleStrideBytes, sizeof(inputTriangle));
			for(NxU32 corner = 0; corner < 3; ++corner)
				triangle.v[corner] = inputTriangle[corner];
			}
		else
			memcpy(&triangle, triangleIn + i * desc.triangleStrideBytes, sizeof(triangle));
		for(NxU32 corner = 0; corner < 3; ++corner)
			{
			if(triangle.v[corner] >= desc.numVertices)
				{
				allocator->free(remap);
				allocator->free(vertices);
				allocator->free(triangles);
				return false;
				}
			triangle.v[corner] = remap[triangle.v[corner]];
			}
		memcpy(triangleOut + i * sizeof(NxTriangle32), &triangle, sizeof(triangle));
		}
	allocator->free(remap);

	if(mInternal.mVertices) allocator->free(mInternal.mVertices);
	if(mInternal.mTriangles) allocator->free(mInternal.mTriangles);
	mInternal.mVertexCount = uniqueVertices;
	mInternal.mTriangleCount = desc.numTriangles;
	mInternal.mVertices = vertices;
	mInternal.mTriangles = triangles;
	mInternal.mMaterialIndices = 0;
	mInternal.mFaceRemap = 0;
	mInternal.mVertexNormals = 0;
	mInternal.mModel = 0;
	memcpy(mBounds, bounds, sizeof(mBounds));
	mConvexEdgeThreshold = desc.convexEdgeThreshold;
	mHeightFieldVerticalAxis = desc.heightFieldVerticalAxis;
	mHeightFieldVerticalExtent = desc.heightFieldVerticalExtent;
	return true;
	}

void TriangleMesh::release()
	{
	if(mPublicMesh)
		{
		NxTriangleMeshAdapter* publicMesh = mPublicMesh;
		mPublicMesh = 0;
		NX_DELETE_SINGLE(publicMesh);
		}
	NxUserAllocator* const allocator = nxFoundationSDKAllocator;
	if(mInternal.mVertices) allocator->free(mInternal.mVertices);
	if(mInternal.mTriangles) allocator->free(mInternal.mTriangles);
	if(mInternal.mMaterialIndices) allocator->free(mInternal.mMaterialIndices);
	if(mInternal.mFaceRemap) allocator->free(mInternal.mFaceRemap);
	if(mInternal.mVertexNormals) allocator->free(mInternal.mVertexNormals);
	}

bool NxTriangleMeshAdapter::loadFromDesc(const NxTriangleMeshDesc& desc)
	{ return mMesh->loadFromDesc(desc); }

bool NxTriangleMeshAdapter::saveToDesc(NxTriangleMeshDesc& desc) const
	{
	desc.setToDefault();
	desc.numVertices = mMesh->mInternal.mVertexCount;
	desc.numTriangles = mMesh->mInternal.mTriangleCount;
	desc.pointStrideBytes = sizeof(NxPoint);
	desc.triangleStrideBytes = sizeof(NxTriangle32);
	desc.points = mMesh->mInternal.mVertices;
	desc.triangles = mMesh->mInternal.mTriangles;
	desc.convexEdgeThreshold = mMesh->mConvexEdgeThreshold;
	desc.heightFieldVerticalAxis = static_cast<NxHeightFieldAxis>(mMesh->mHeightFieldVerticalAxis);
	desc.heightFieldVerticalExtent = mMesh->mHeightFieldVerticalExtent;
	return true;
	}

NxU32 NxTriangleMeshAdapter::getSubmeshCount() const { return mMesh->mInternal.mTriangleCount; }

NxU32 NxTriangleMeshAdapter::getCount(NxSubmeshIndex submesh, NxInternalArray array) const
	{
	if(submesh != 0) return 0;
	switch(array)
		{
		case NX_ARRAY_TRIANGLES: return mMesh->mInternal.mTriangleCount;
		case NX_ARRAY_VERTICES: return mMesh->mInternal.mVertexCount;
		case NX_ARRAY_NORMALS: return mMesh->mInternal.mVertexNormals ? mMesh->mInternal.mVertexCount : 0;
		default: return 0;
		}
	}

NxInternalFormat NxTriangleMeshAdapter::getFormat(NxSubmeshIndex submesh, NxInternalArray array) const
	{
	if(submesh != 0) return NX_FORMAT_NODATA;
	if(array == NX_ARRAY_TRIANGLES) return NX_FORMAT_INT;
	if(array == NX_ARRAY_VERTICES || array == NX_ARRAY_NORMALS) return NX_FORMAT_FLOAT;
	return NX_FORMAT_NODATA;
	}

const void* NxTriangleMeshAdapter::getBase(NxSubmeshIndex submesh, NxInternalArray array) const
	{
	if(submesh != 0) return 0;
	switch(array)
		{
		case NX_ARRAY_TRIANGLES: return mMesh->mInternal.mTriangles;
		case NX_ARRAY_VERTICES: return mMesh->mInternal.mVertices;
		case NX_ARRAY_NORMALS: return mMesh->mInternal.mVertexNormals;
		default: return 0;
		}
	}

NxU32 NxTriangleMeshAdapter::getStride(NxSubmeshIndex submesh, NxInternalArray array) const
	{
	if(submesh != 0) return 0;
	if(array == NX_ARRAY_TRIANGLES || array == NX_ARRAY_VERTICES || array == NX_ARRAY_NORMALS)
		return 12;
	return 0;
	}

bool NxTriangleMeshAdapter::loadPMap(const NxPMap&) { return false; }
bool NxTriangleMeshAdapter::hasPMap() const { return false; }
NxU32 NxTriangleMeshAdapter::getPMapSize() const { return 0; }
bool NxTriangleMeshAdapter::getPMapData(NxPMap&) const { return false; }
NxU32 NxTriangleMeshAdapter::getPMapDensity() const { return 0; }

// The two tags are read and written as DWORDS, so on the little-endian target
// the bytes on disc are 54 53 58 4e and 48 53 45 4d. Written most significant
// byte first the two constants spell NXST and MESH; in file order they spell
// TSXN and HSEM. Which spelling the source used is not established and the
// writer does not settle it -- it pushes the same two 32-bit values (0x53a3d's
// siblings at 0x000539de and 0x000539ea). What IS established is the value each
// comparison requires and each store emits, so that is what is written here.
static const NxU32 kTriangleMeshTag0 = 0x4e585354;	// cmp at 0x00055cc2, push at 0x000539de
static const NxU32 kTriangleMeshTag1 = 0x4d455348;	// cmp at 0x00055cd0, push at 0x000539ea

NxTriangleMeshHeader nxTriangleMeshReadHeader(const NxStream& stream)
	{
	// Two separate reads with the first test between them. A reader that read
	// both tags before testing either would consume two dwords for a bad first
	// tag, and the asset differential measures exactly that.
	if(stream.readDword() != kTriangleMeshTag0)
		return NX_TRIANGLE_MESH_HEADER_REJECTED;
	if(stream.readDword() != kTriangleMeshTag1)
		return NX_TRIANGLE_MESH_HEADER_REJECTED;

	return NX_TRIANGLE_MESH_HEADER_NOT_RECONSTRUCTED;
	}

// ---------------------------------------------------------------------------
// phys_fn_002162 (0x000539d0), slot 18 of the TriangleMesh vtable: the mesh
// stream's writer. Field for field with nxTriangleMeshReadHeader's table.

// phys_data_003609 at .data 0x00124120. The writer reads this global for field
// 3 (`mov ecx,[0x10124120]` at 0x000539f4); its address occurs exactly once in
// the whole file, nothing writes it, and .data's raw bytes end at 0x00124000,
// so it is zero-filled at load and the field is written as 0. What the field
// MEANS is unestablished -- its position is where a version would sit, which is
// not evidence that it is one. It is modelled as this translation unit's global
// so that the statement "program-lifetime, zero in this image" has a carrier.
static NxU32 gTriangleMeshSerializationGlobal = 0;

bool TriangleMesh::save(NxStream& stream) const
	{
	stream.storeDword(kTriangleMeshTag0);					// call [eax+0x24] 0x000539e5
	stream.storeDword(kTriangleMeshTag1);					// 0x000539f1
	stream.storeDword(gTriangleMeshSerializationGlobal);	// mov ecx,[0x10124120] 0x000539f4, store 0x000539ff

	// The flags word has four bits and no more, measured on both sides: the
	// writer zeroes eax at 0x00053a05 and only these four conditions touch it.
	NxU32 flags = 0;
	if(mInternal.mMaterialIndices)							// test [edi+0x18] 0x00053a07
		flags |= 1;											// mov eax,1 0x00053a0b
	if(mInternal.mFaceRemap)								// test [edi+0x1c] 0x00053a13
		flags |= 2;											// or eax,2 0x00053a17
	if(mConvexMesh)											// test [edi+0xa0] 0x00053a22
		flags |= 4;											// or eax,4 0x00053a24
	if(mHullFlags & 1)										// test byte ptr [edi+0x40],1 0x00053a27
		flags |= 8;											// or eax,8 0x00053a2d
	stream.storeDword(flags);								// 0x00053a35

	stream.storeFloat(mConvexEdgeThreshold);				// push [edi+0x6c] 0x00053a3d, call +0x28 0x00053a40
	stream.storeDword(mHeightFieldVerticalAxis);			// push [edi+0x7c] 0x00053a48, call +0x24 0x00053a4b
	stream.storeFloat(mHeightFieldVerticalExtent);			// push [edi+0x80] 0x00053a56, call +0x28 0x00053a59

	stream.storeDword(mInternal.mVertexCount);				// push [edi+0x08] 0x00053a61
	stream.storeDword(mInternal.mTriangleCount);			// push [edi+0x0c] 0x00053a6c

	// Both arrays are unconditional and always count*12: `lea eax,[eax+eax*2];
	// shl eax,2` at 0x00053a7a and 0x00053a8f. There is no 16-bit triangle
	// variant anywhere in this format.
	stream.storeBuffer(mInternal.mVertices, mInternal.mVertexCount * 12);	// 0x00053a84
	stream.storeBuffer(mInternal.mTriangles, mInternal.mTriangleCount * 12);	// 0x00053a99

	if(mInternal.mMaterialIndices)							// test 0x00053a9f
		stream.storeBuffer(mInternal.mMaterialIndices, mInternal.mTriangleCount * 2);	// shl ecx,1 0x00053aa8, store 0x00053aae
	if(mInternal.mFaceRemap)								// test 0x00053ab4
		stream.storeBuffer(mInternal.mFaceRemap, mInternal.mTriangleCount * 4);			// shl ecx,2 0x00053abd, store 0x00053ac4

	stream.storeDword(mPresenceFlagA);						// push [edi+0x8c] 0x00053acf
	stream.storeDword(mPresenceFlagB);						// push [edi+0x90] 0x00053ade
	if(mArrayA)												// test 0x00053ae9
		stream.storeBuffer(mArrayA, mInternal.mTriangleCount * 4);	// 0x00053af9
	if(mArrayB)												// 0x00053b02
		stream.storeBuffer(mArrayB, mInternal.mTriangleCount * 4);	// 0x00053b12

	// Fields 18 and 19: the acceleration blob. A growable MemoryStream is built
	// with the literal initial size 0x1000 (push 0x1000 0x00053b17, ctor
	// 0x000b3ce0 at 0x00053b20), the model at internal+0x20 saves into it
	// through BaseModel slot 5 (call [edx+0x14] at 0x00053b2f), its length is
	// taken (phys_fn_004768 through 0x00053b36), stored (0x00053b42), the
	// collapsed buffer fetched (phys_fn_004795 through 0x00053b4e) and stored
	// (call +0x30 at 0x00053b56). The destructor runs at 0x00053b5d on scope
	// exit. The image dereferences the model's vtable without testing it, and
	// so does this: a mesh without a model is not a state either side defines.
	MemoryStream growable(0x1000, 0);
	mInternal.mModel->Save(&growable);
	NxU32 length = growable.getLength();
	stream.storeDword(length);
	const void* data = growable.collapse(0);
	stream.storeBuffer(data, length);

	// mov al,1 / ret 4: no error path.
	return true;
	}

// ---------------------------------------------------------------------------
// qhull-gap Task 4e: the hull computation and TriangleMesh's allocator slots
// (units/convex-cooking-contract.md, "Row assignment" and "Object layouts").
// The Foundation allocator is `[[0x101041bc]]`, nxFoundationSDKAllocator.

// phys_fn_002233 (0x00054920, 277 B)
// thiscall, `ret 8`. The library object (E-0x40), the descriptor (E-0x38) and
// the result (E-0x1c) are locals; the result is not initialised before the
// call. On success the whole input descriptor is copied first (rep movsd of
// 13 dwords, 0x00054992), then the counts, strides, arrays and flags are
// overwritten. NX_MF_16_BIT_INDICES is NOT cleared although the triangles
// written are 32-bit with stride 12 (the listing's `and [ebx+0x18],~8` only).
// ReleaseResult runs on both paths (0x00054a26); its return is not read.
bool TriangleMeshHullAllocator::computeHull(const NxTriangleMeshDesc& desc, NxTriangleMeshDesc& out)
	{
	HullLibrary library;
	library.mAllocator = this;										// mov [esp+8],ecx 0x0005492e

	HullDesc hullDesc;
	hullDesc.mFlags = QF_WELD | QF_NORMALIZE | QF_REDUCE | QF_TRIANGLES | QF_REVERSE_ORDER
		| QF_WRITE_FAIL_OBJ;										// 0xb7, 0x0005494d
	hullDesc.mVcount = desc.numVertices;							// 0x0005493b
	hullDesc.mVertices = static_cast<const NxReal*>(desc.points);	// 0x00054936
	hullDesc.mVertexStride = desc.pointStrideBytes;					// 0x00054955
	hullDesc.mNormalEpsilon = 0.00001f;								// 0x3727c5ac, 0x00054959
	hullDesc.mMaxVertices = 0x100;									// 0x00054961
	hullDesc.mUnknown18 = 0.8f;										// 0x3f4ccccd, 0x00054969
	library.mPolygonizer = 0;										// 0x00054971

	HullResult result;
	bool built = false;												// xor ebx,ebx 0x00054946
	if(library.CreateConvexHull(hullDesc, result) == QE_OK)			// 0x00054975
		{
		out = desc;													// rep movsd, 13 dwords, 0x00054992
		out.numVertices = result.mNumOutputVertices;				// 0x0005499b
		out.numTriangles = result.mNumTriangles;					// 0x00054998
		out.pointStrideBytes = 12;									// 0x000549a2
		out.triangleStrideBytes = 12;								// 0x000549a5

		void* points = nxFoundationSDKAllocator->malloc(result.mNumOutputVertices * 12, NX_MEMORY_TEMP);	// 0x000549bb
		out.points = points;										// 0x000549cc
		memcpy(points, result.mOutputVertices, out.numVertices * 12);	// 0x000549d4-0x000549db

		void* triangles = nxFoundationSDKAllocator->malloc(result.mNumTriangles * 12, NX_MEMORY_TEMP);	// 0x000549f3
		out.triangles = triangles;									// 0x00054a08
		memcpy(triangles, result.mIndices, out.numTriangles * 12);	// 0x00054a0d-0x00054a14

		out.flags &= ~NX_MF_COMPUTE_CONVEX;							// and [ebx+0x18],0xfffffff7 0x00054a16
		built = true;												// mov bl,1 0x00054a1a
		}
	library.ReleaseResult(result);									// 0x00054a26
	return built;
	}

// phys_fn_002235 (0x00054a40, 22 B)
// Slot 0: `push 0; push size; call [edx+8]` on [[0x101041bc]], `ret 4`.
void* TriangleMeshHullAllocator::malloc(size_t size)
	{
	return nxFoundationSDKAllocator->malloc(size, NX_MEMORY_PERSISTENT);
	}

// phys_fn_002237 (0x00054a60, 28 B)
// Slot 1: a null pointer returns (`ret 4` at 0x00054a79); otherwise a tail
// `jmp [edx+0x14]` into the Foundation allocator's free.
void TriangleMeshHullAllocator::free(void* memory)
	{
	if(memory)
		nxFoundationSDKAllocator->free(memory);
	}
