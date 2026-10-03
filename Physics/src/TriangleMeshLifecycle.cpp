/*----------------------------------------------------------------------------*\
|
|                              NovodeX Technology
|
\*----------------------------------------------------------------------------*/
#include "TriangleMesh.h"
#include "ConvexHull.h"
#include "TriangleMeshPolygons.h"
#include "PhysicsSDK.h"
#include "FoundationSDK.h"

#include <string.h>
#include <new>

static void __fastcall nxEnsureInternalVertexNormals(InternalTriangleMesh* mesh)
	{
	if(!mesh->mVertexNormals)
		{
		__asm
			{
			mov ecx, mesh
			call nxMeshComputeVertexNormals
			}
		}
	}

// phys_fn_002251 initializes the measured fields and allocates the separate
// eight-byte public wrapper stored at TriangleMesh+0xe4.
TriangleMesh::TriangleMesh() : mPublicMesh(0)
	{
	mVtableSlot = 0;
	mPolygonTable = gTriangleMeshPolygonTable;
	nxInternalMeshInit(&mInternal);
	mHullFlags = 0;
	memset(mGap44, 0, sizeof(mGap44));
	mConvexEdgeThreshold = 0.001f;
	memset(mGap70, 0, sizeof(mGap70));
	mHeightFieldVerticalAxis = 0xff;
	mHeightFieldVerticalExtent = 0.0f;
	mAdjacencies = 0;
	mEdgeList = 0;
	mPresenceFlagA = 0;
	mPresenceFlagB = 0;
	mArrayA = 0;
	mArrayB = 0;
	mWord9C = 0;
	mConvexMesh = 0;
	memset(mGapA4, 0, sizeof(mGapA4));
	void* memory = nxFoundationSDKAllocator->malloc(sizeof(NpTriangleMesh), NX_MEMORY_PERSISTENT);
	if(memory)
		mPublicMesh = new(memory) NpTriangleMesh(this);
	}

TriangleMesh::~TriangleMesh()
	{
	releaseContents();
	if(mPublicMesh)
		{
		mPublicMesh->~NpTriangleMesh();
		nxFoundationSDKAllocator->free(mPublicMesh);
		mPublicMesh = 0;
		}
	}

void TriangleMesh::releaseContents()
	{
	if(mConvexMesh)
		{
		ConvexHull* hull = static_cast<ConvexHull*>(mConvexMesh);
		if(hull->mVertexNormals)
			nxGetSdkAllocator()->free(hull->mVertexNormals);
		if(hull->mFaces)
			nxGetSdkAllocator()->free(const_cast<NxU16*>(hull->mFaces));
		nxGetSdkAllocator()->free(hull);
		mConvexMesh = 0;
		}
	nxInternalMeshRelease(&mInternal);
	}

bool TriangleMesh::loadFromDesc(const NxTriangleMeshDesc& desc)
	{
	if(!desc.isValid())
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\TriangleMesh.cpp", 0xba, 0,
			"TriangleMesh::loadFromDesc: desc.isValid() failed!");
		return false;
		}
	NxTriangleMeshDesc working = desc;
	mConvexEdgeThreshold = desc.convexEdgeThreshold;
	mHeightFieldVerticalAxis = desc.heightFieldVerticalAxis;
	mHeightFieldVerticalExtent = desc.heightFieldVerticalExtent;
	NxTriangleMeshDesc cooked;
	bool temporaryCookedArrays = false;
	if(desc.flags & NX_MF_COMPUTE_CONVEX)
		{
		TriangleMeshHullAllocator hullAllocator;
		cooked.setToDefault();
		if(!hullAllocator.computeHull(desc, cooked))
			return false;
		working = cooked;
		temporaryCookedArrays = true;
		}

	// Row 002256 copies strided input into owned 12-byte arrays. The public
	// convex path's cooked descriptor already has twelve-byte input strides.
	nxInternalMeshRelease(&mInternal);
	nxInternalMeshAllocateVertices(&mInternal, working.numVertices);
	nxInternalMeshAllocateTriangles(&mInternal, working.numTriangles);
	if(!mInternal.mVertices || !mInternal.mTriangles)
		{
		if(temporaryCookedArrays)
			{
			nxFoundationSDKAllocator->free(const_cast<void*>(cooked.points));
			nxFoundationSDKAllocator->free(const_cast<void*>(cooked.triangles));
			}
		return false;
		}
	NxU8* outputVertices = static_cast<NxU8*>(mInternal.mVertices);
	const NxU8* inputVertices = static_cast<const NxU8*>(working.points);
	for(NxU32 i = 0; i < working.numVertices; ++i)
		memcpy(outputVertices + i * 12, inputVertices + i * working.pointStrideBytes, 12);
	NxU32* outputTriangles = static_cast<NxU32*>(mInternal.mTriangles);
	const NxU8* inputTriangles = static_cast<const NxU8*>(working.triangles);
	for(NxU32 i = 0; i < working.numTriangles; ++i)
		{
		const NxU8* triangle = inputTriangles + i * working.triangleStrideBytes;
		NxU32 indices[3];
		if(working.flags & NX_MF_16_BIT_INDICES)
			{
			const NxU16* source = reinterpret_cast<const NxU16*>(triangle);
			indices[0] = source[0]; indices[1] = source[1]; indices[2] = source[2];
			}
		else
			memcpy(indices, triangle, sizeof(indices));
		if(working.flags & NX_MF_FLIPNORMALS)
			{
			const NxU32 swap = indices[1]; indices[1] = indices[2]; indices[2] = swap;
			}
		memcpy(outputTriangles + i * 3, indices, sizeof(indices));
		}
	if(working.materialIndices)
		{
		nxInternalMeshAllocateMaterials(&mInternal);
		for(NxU32 i = 0; i < working.numTriangles; ++i)
			memcpy(mInternal.mMaterialIndices + i,
				static_cast<const NxU8*>(working.materialIndices) + i * working.materialIndexStride,
				sizeof(NxU16));
		}
	if(!nxInternalMeshBuildTopology(&mInternal))
		{
		if(temporaryCookedArrays)
			{
			nxFoundationSDKAllocator->free(const_cast<void*>(cooked.points));
			nxFoundationSDKAllocator->free(const_cast<void*>(cooked.triangles));
			}
		return false;
		}
	nxInternalMeshBuildTriangleData(&mInternal);
	if(temporaryCookedArrays)
		{
		nxFoundationSDKAllocator->free(const_cast<void*>(cooked.points));
		nxFoundationSDKAllocator->free(const_cast<void*>(cooked.triangles));
		}
	if(!nxInternalMeshBuildModel(&mInternal, mHeightFieldVerticalAxis,
		mHeightFieldVerticalExtent, 0))
		return false;

	if((working.flags & NX_MF_CONVEX) && !mConvexMesh)
		{
		void* memory = nxGetSdkAllocator()->malloc(0x98, NX_MEMORY_PERSISTENT);
		if(!memory)
			return false;
		ConvexHull* hull = new(memory) ConvexHull();
		memset(hull, 0, sizeof(*hull));
		hull->mNbFaces = mInternal.mTriangleCount;
		hull->mNbVerts = mInternal.mVertexCount;
		hull->mVerts = static_cast<const IceMaths::Point*>(mInternal.mVertices);
		NxU16* faces = static_cast<NxU16*>(nxGetSdkAllocator()->malloc(
			hull->mNbFaces * 3 * sizeof(NxU16), NX_MEMORY_PERSISTENT));
		if(!faces)
			{
			nxGetSdkAllocator()->free(hull);
			return false;
			}
		const NxU32* indices = static_cast<const NxU32*>(mInternal.mTriangles);
		for(NxU32 i = 0; i < hull->mNbFaces * 3; ++i)
			{
			const NxU32 corner = i % 3;
			const NxU32 faceIndex = i / 3;
			NxU32 sourceCorner = corner;
			if((working.flags & NX_MF_FLIPNORMALS) && corner == 1)
				sourceCorner = 2;
			else if((working.flags & NX_MF_FLIPNORMALS) && corner == 2)
				sourceCorner = 1;
			faces[i] = static_cast<NxU16>(indices[faceIndex * 3 + sourceCorner]);
			}
		hull->mFaces = faces;
		mConvexMesh = hull;
		hull->ComputeVertexNormals();
		}
	return true;
	}

// phys_fn_002140: the public wrapper stores the internal mesh pointer at +4.
NpTriangleMesh::NpTriangleMesh(TriangleMesh* mesh) : mMesh(mesh) {}
NpTriangleMesh::~NpTriangleMesh() {}
bool NpTriangleMesh::loadFromDesc(const NxTriangleMeshDesc& desc) { return mMesh->loadFromDesc(desc); }
bool NpTriangleMesh::saveToDesc(NxTriangleMeshDesc& desc) const
	{
	desc.setToDefault();
	desc.numVertices = mMesh->mInternal.mVertexCount;
	desc.numTriangles = mMesh->mInternal.mTriangleCount;
	desc.pointStrideBytes = 12;
	desc.triangleStrideBytes = 12;
	desc.points = mMesh->mInternal.mVertices;
	desc.triangles = mMesh->mInternal.mTriangles;
	return true;
	}
// The shipped wrapper's slot forwards to the mesh's triangle count despite
// the public method name; the tetrahedron's four cooked faces therefore yield
// four here.
NxU32 NpTriangleMesh::getSubmeshCount() const { return mMesh->mInternal.mTriangleCount; }
NxU32 NpTriangleMesh::getCount(NxSubmeshIndex submesh, NxInternalArray array) const
	{
	if(submesh) return 0;
	if(array == NX_ARRAY_TRIANGLES) return mMesh->mInternal.mTriangleCount;
	if(array == NX_ARRAY_VERTICES) return mMesh->mInternal.mVertexCount;
	ConvexHull* hull = static_cast<ConvexHull*>(mMesh->mConvexMesh);
	if(!hull) return 0;
	if(array == NX_ARRAY_NORMALS) return mMesh->mInternal.mVertexCount;
	if(array == NX_ARRAY_HULL_VERTICES) return hull->mNbVerts;
	if(array == NX_ARRAY_HULL_POLYGONS) return nxHullComputePolygons(hull) ? hull->mNbPolygons : 0;
	return 0;
	}
NxInternalFormat NpTriangleMesh::getFormat(NxSubmeshIndex submesh, NxInternalArray array) const
	{
	if(submesh) return NX_FORMAT_NODATA;
	if(array == NX_ARRAY_TRIANGLES) return NX_FORMAT_INT;
	if(array == NX_ARRAY_VERTICES || array == NX_ARRAY_HULL_VERTICES)
		return NX_FORMAT_FLOAT;
	if(array == NX_ARRAY_NORMALS && getCount(submesh, array)) return NX_FORMAT_FLOAT;
	if(array == NX_ARRAY_HULL_POLYGONS && getCount(submesh, array)) return NX_FORMAT_INT;
	return NX_FORMAT_NODATA;
	}
const void* NpTriangleMesh::getBase(NxSubmeshIndex submesh, NxInternalArray array) const
	{
	if(submesh) return 0;
	if(array == NX_ARRAY_TRIANGLES) return mMesh->mInternal.mTriangles;
	if(array == NX_ARRAY_VERTICES) return mMesh->mInternal.mVertices;
	ConvexHull* hull = static_cast<ConvexHull*>(mMesh->mConvexMesh);
	if(!hull) return 0;
	if(array == NX_ARRAY_NORMALS)
		{
		nxEnsureInternalVertexNormals(&mMesh->mInternal);
		return mMesh->mInternal.mVertexNormals;
		}
	if(array == NX_ARRAY_HULL_VERTICES) return hull->mVerts;
	if(array == NX_ARRAY_HULL_POLYGONS && nxHullComputePolygons(hull)) return hull->mPolygons;
	return 0;
	}
NxU32 NpTriangleMesh::getStride(NxSubmeshIndex submesh, NxInternalArray array) const
	{
	if(submesh) return 0;
	if(array == NX_ARRAY_TRIANGLES || array == NX_ARRAY_VERTICES || array == NX_ARRAY_NORMALS ||
		array == NX_ARRAY_HULL_VERTICES) return 12;
	return 0;
	}
bool NpTriangleMesh::loadPMap(const NxPMap&) { return false; }
bool NpTriangleMesh::hasPMap() const { return false; }
NxU32 NpTriangleMesh::getPMapSize() const { return 0; }
bool NpTriangleMesh::getPMapData(NxPMap&) const { return false; }
NxU32 NpTriangleMesh::getPMapDensity() const { return 0; }
