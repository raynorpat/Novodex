/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "TriangleMesh.h"
#include "PMap.h"

#include "NxStream.h"
#include "NxPMap.h"
#include "NxTriangleMesh.h"
#include "TriangleMeshPolygons.h"
#include "OPC_Model.h"
#include "NxVolumeIntegration.h"
#include "ConvexHull.h"
#include "IceMeshTools.h"

#include <new>
#include <float.h>
#include <string.h>
#include <new>

#define NX_TRIANGLE_MESH_CPP "\\Epic\\Novodex\\SDKs\\Physics\\src\\TriangleMesh.cpp"

static const NxI32 kTriangleMeshInvalidPMapLine = 0x30b;
static const NxI32 kTriangleMeshPMapCreateFailedLine = 0x318;
static const char* const kTriangleMeshInvalidPMapMessage = "TriangleMesh::loadPMap: invalid pmap data!";
static const char* const kTriangleMeshPMapCreateFailedMessage = "TriangleMesh::loadPMap: pmap creation failed!";

// The two tags are read and written as DWORDS, so on the little-endian target
// the bytes on disc are 54 53 58 4e and 48 53 45 4d. Written most significant
// byte first the two constants spell NXST and MESH; in file order they spell
// TSXN and HSEM. Which spelling the source used is not established and the
// writer does not settle it -- it pushes the same two 32-bit values (0x53a3d's
// siblings at 0x000539de and 0x000539ea). What IS established is the value each
// comparison requires and each store emits, so that is what is written here.
static const NxU32 kTriangleMeshTag0 = 0x4e585354;	// cmp at 0x00055cc2, push at 0x000539de
static const NxU32 kTriangleMeshTag1 = 0x4d455348;	// cmp at 0x00055cd0, push at 0x000539ea

namespace
	{
	struct TriangleMeshConvexData : ConvexHull
		{
	float			mBounds[6];		// +0x4c, min xyz then max xyz (001411)
	Valencies*		mVertexGraph;		// +0x64 (001411, 002249)
	NxU32			mObjectWords[12];	// +0x68..+0x97 (001524)
		};

	static_assert(sizeof(ConvexHull) == 0x4c, "the convex hull base has the measured 0x4c layout");
	static_assert(offsetof(TriangleMeshConvexData, mVertexGraph) == 0x64,
		"the convex vertex graph is at +0x64");
	static_assert(sizeof(TriangleMeshConvexData) == 0x98, "the convex mesh allocation is 0x98 bytes");

	static void nxTriangleMeshDestroyConvexData(TriangleMeshConvexData* hull)
		{
	if(!hull)
		return;
	if(hull->mVertexGraph)
		{
		hull->mVertexGraph->~Valencies();
		nxFoundationSDKAllocator->free(hull->mVertexGraph);
		hull->mVertexGraph = 0;
		}
	if(hull->mVertexNormals)
		nxFoundationSDKAllocator->free(hull->mVertexNormals);
	if(hull->mPolygonVRefs)
		nxFoundationSDKAllocator->free(hull->mPolygonVRefs);
	if(hull->mPolygons)
		nxFoundationSDKAllocator->free(reinterpret_cast<NxU32*>(hull->mPolygons) - 1);
	if(hull->mFaces)
		nxFoundationSDKAllocator->free(reinterpret_cast<NxU8*>(const_cast<NxU16*>(hull->mFaces)) - sizeof(NxU32));
	if(hull->mVerts)
		nxFoundationSDKAllocator->free(const_cast<IceMaths::Point*>(hull->mVerts));
		nxFoundationSDKAllocator->free(hull);
		}

	static bool nxTriangleMeshBuildConvexData(TriangleMeshConvexData** result,
		const InternalTriangleMesh& mesh)
		{
	*result = 0;
	if(!mesh.mVertexCount || !mesh.mTriangleCount || !mesh.mVertices || !mesh.mTriangles)
		return false;
	TriangleMeshConvexData* hull = static_cast<TriangleMeshConvexData*>(
		nxFoundationSDKAllocator->malloc(sizeof(TriangleMeshConvexData), NX_MEMORY_PERSISTENT));
	if(!hull)
		return false;
	memset(hull, 0, sizeof(*hull));
	hull->mNbFaces = mesh.mTriangleCount;
	hull->mNbVerts = mesh.mVertexCount;

	IceMaths::Point* vertices = static_cast<IceMaths::Point*>(
		nxFoundationSDKAllocator->malloc(mesh.mVertexCount * sizeof(IceMaths::Point), NX_MEMORY_PERSISTENT));
	if(!vertices)
		{
		nxTriangleMeshDestroyConvexData(hull);
		return false;
		}
	memcpy(vertices, mesh.mVertices, mesh.mVertexCount * sizeof(IceMaths::Point));
	hull->mVerts = vertices;

	NxU32* faceAllocation = static_cast<NxU32*>(nxFoundationSDKAllocator->malloc(
		mesh.mTriangleCount * sizeof(NxU16) * 3 + sizeof(NxU32), NX_MEMORY_PERSISTENT));
	if(!faceAllocation)
		{
		nxTriangleMeshDestroyConvexData(hull);
		return false;
		}
	faceAllocation[0] = mesh.mTriangleCount;
	NxU16* faces = reinterpret_cast<NxU16*>(faceAllocation + 1);
	const NxU32* triangles = static_cast<const NxU32*>(mesh.mTriangles);
	for(NxU32 i = 0; i < mesh.mTriangleCount * 3; ++i)
		faces[i] = static_cast<NxU16>(triangles[i]);
	hull->mFaces = faces;

	// 001494 makes the closed hull consistently outward before 001472 groups
	// coplanar triangles. 001467 compares each triangle plane with the vertex
	// cloud's centre and reverses inward faces.
	IceMaths::Point center(0.0f, 0.0f, 0.0f);
	for(NxU32 i = 0; i < hull->mNbVerts; ++i)
		center += hull->mVerts[i];
	center *= 1.0f / hull->mNbVerts;
	for(NxU32 i = 0; i < hull->mNbFaces; ++i)
		{
		NxU16* face = faces + i * 3;
		const IceMaths::Point& v0 = hull->mVerts[face[0]];
		const IceMaths::Point& v1 = hull->mVerts[face[1]];
		const IceMaths::Point& v2 = hull->mVerts[face[2]];
		const IceMaths::Point edge0 = v1 - v0;
		const IceMaths::Point edge1 = v2 - v0;
		const IceMaths::Point normal = edge0 ^ edge1;
		if((normal | (center - v0)) > 0.0f)
			{
			const NxU16 swap = face[1];
			face[1] = face[2];
			face[2] = swap;
			}
		}

	if(!nxHullComputePolygons(hull) || !hull->ComputeVertexNormals())
		{
		nxTriangleMeshDestroyConvexData(hull);
		return false;
		}

	void* graphMemory = nxFoundationSDKAllocator->malloc(sizeof(Valencies), NX_MEMORY_PERSISTENT);
	if(!graphMemory)
		{
		nxTriangleMeshDestroyConvexData(hull);
		return false;
		}
	hull->mVertexGraph = new(graphMemory) Valencies;
	VALENCESCREATE graphDesc;
	graphDesc.NbVerts = hull->mNbVerts;
	graphDesc.NbFaces = hull->mNbFaces;
	graphDesc.DFaces = 0;
	graphDesc.WFaces = hull->mFaces;
	graphDesc.AdjacentList = true;
	if(!hull->mVertexGraph->Compute(graphDesc))
		{
		nxTriangleMeshDestroyConvexData(hull);
		return false;
		}

	for(unsigned axis = 0; axis < 3; ++axis)
		{
		hull->mBounds[axis] = FLT_MAX;
		hull->mBounds[axis + 3] = -FLT_MAX;
		}
	for(NxU32 i = 0; i < hull->mNbVerts; ++i)
		for(unsigned axis = 0; axis < 3; ++axis)
			{
			const float value = hull->mVerts[i][axis];
			if(value < hull->mBounds[axis]) hull->mBounds[axis] = value;
			if(value > hull->mBounds[axis + 3]) hull->mBounds[axis + 3] = value;
			}
	*result = hull;
	return true;
		}

	class NxTriangleMeshWrapper : public NxTriangleMesh
		{
		public:
		explicit NxTriangleMeshWrapper(TriangleMesh* mesh) : mMesh(mesh) {}
		virtual ~NxTriangleMeshWrapper() {}
		bool loadFromDesc(const NxTriangleMeshDesc& desc) override
			{ return mMesh->loadFromDesc(desc); }
		bool saveToDesc(NxTriangleMeshDesc& desc) const override
			{ return mMesh->saveToDesc(desc); }
		NxU32 getSubmeshCount() const override
			{
			const NxU32 hullPolygons = mMesh->getCount(NX_ARRAY_HULL_POLYGONS);
			return hullPolygons ? hullPolygons : mMesh->getCount(NX_ARRAY_TRIANGLES);
			}
		NxU32 getCount(NxSubmeshIndex submesh, NxInternalArray array) const override
			{ return submesh == 0 ? mMesh->getCount(array) : 0; }
		NxInternalFormat getFormat(NxSubmeshIndex submesh, NxInternalArray array) const override
			{ return submesh == 0 ? mMesh->getFormat(array) : NX_FORMAT_NODATA; }
		const void* getBase(NxSubmeshIndex submesh, NxInternalArray array) const override
			{ return submesh == 0 ? mMesh->getBase(array) : 0; }
		NxU32 getStride(NxSubmeshIndex submesh, NxInternalArray array) const override
			{ return submesh == 0 ? mMesh->getStride(array) : 0; }
		bool loadPMap(const NxPMap& pmap) override { return mMesh->loadPMap(pmap); }
		bool hasPMap() const override { return mMesh->hasPMap(); }
		NxU32 getPMapSize() const override { return mMesh->getPMapSize(); }
		bool getPMapData(NxPMap& pmap) const override { return mMesh->getPMapData(pmap); }
		NxU32 getPMapDensity() const override { return 0; }
		private:
		TriangleMesh* mMesh;
		};

	static void nxTriangleMeshFree(void*& pointer)
		{
		if(pointer)
			{
			nxFoundationSDKAllocator->free(pointer);
			pointer = 0;
			}
		}

	template <typename T>
	static void nxTriangleMeshFreeTyped(T*& pointer)
		{
		if(pointer)
			{
			nxFoundationSDKAllocator->free(pointer);
			pointer = 0;
			}
		}

	static void* nxTriangleMeshCopy(const void* source, NxU32 count, NxU32 sourceStride,
		NxU32 elementSize)
		{
		if(!count || !source || sourceStride < elementSize)
			return 0;
		void* result = nxFoundationSDKAllocator->malloc(count * elementSize,
			NX_MEMORY_PERSISTENT);
		if(!result)
			return 0;
		const NxU8* in = static_cast<const NxU8*>(source);
		NxU8* out = static_cast<NxU8*>(result);
		for(NxU32 i = 0; i < count; ++i)
			memcpy(out + i * elementSize, in + i * sourceStride, elementSize);
		return result;
		}
	}

// phys_fn_002251 (oracle RVA 0x00055490): initialize the TriangleMesh object
// and create its public NxTriangleMesh wrapper.
TriangleMesh::TriangleMesh()
	{
	memset(this, 0, sizeof(*this));
	static const NxU32 vtableToken = 0x00108608;
	mVtableSlot = const_cast<NxU32*>(&vtableToken);
	mPolygonTable = gTriangleMeshPolygonTable;
	mConvexEdgeThreshold = 0.001f;
	mHeightFieldVerticalAxis = NX_NOT_HEIGHTFIELD;
	mCachedMass = -1.0f;
	NxTriangleMeshWrapper* wrapper = 0;
	void* memory = nxFoundationSDKAllocator->malloc(sizeof(NxTriangleMeshWrapper),
		NX_MEMORY_PERSISTENT);
	if(memory)
		wrapper = new(memory) NxTriangleMeshWrapper(this);
	mPublicObject = wrapper;
	}

// phys_fn_002253 (oracle RVA 0x00055570): release the public wrapper and all
// TriangleMesh-owned PMap, convex, model, and array storage.
TriangleMesh::~TriangleMesh()
	{
	nxTriangleMeshDestroyConvexData(static_cast<TriangleMeshConvexData*>(mConvexMesh));
	mConvexMesh = 0;
	if(mPMap)
		{
		mPMap->~PenetrationMap();
		nxFoundationSDKAllocator->free(mPMap);
		mPMap = 0;
		}
	nxTriangleMeshFree(mInternal.mVertices);
	nxTriangleMeshFree(mInternal.mTriangles);
	nxTriangleMeshFreeTyped(mInternal.mMaterialIndices);
	nxTriangleMeshFreeTyped(mInternal.mFaceRemap);
	nxTriangleMeshFree(mInternal.mVertexNormals);
	if(mInternal.mModel)
		{
		delete mInternal.mModel;
		mInternal.mModel = 0;
		}
	if(mPublicObject)
		{
		NxTriangleMeshWrapper* wrapper = static_cast<NxTriangleMeshWrapper*>(mPublicObject);
		wrapper->~NxTriangleMeshWrapper();
		nxFoundationSDKAllocator->free(wrapper);
		mPublicObject = 0;
		}
	}

bool TriangleMesh::loadPMap(const NxPMap& pmap)
	{
	if(!pmap.data || !pmap.dataSize)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
			NX_TRIANGLE_MESH_CPP, kTriangleMeshInvalidPMapLine, 0, "%s", kTriangleMeshInvalidPMapMessage);
		return false;
		}

	if(mPMap)
		{
		mPMap->~PenetrationMap();
		nxFoundationSDKAllocator->free(mPMap);
		mPMap = 0;
		}
	if(!nxFoundationSDKAllocator)
		return false;
	void* memory = nxFoundationSDKAllocator->malloc(sizeof(PenetrationMap), NX_MEMORY_PERSISTENT);
	if(!memory)
		return false;
	PenetrationMap* penetrationMap = new(memory) PenetrationMap;
	MemoryStream stream(pmap.dataSize, pmap.data);
	stream.seek(0);
	if(!penetrationMap->create(this, 0, 0, &stream, true, 0))
		{
		penetrationMap->~PenetrationMap();
		nxFoundationSDKAllocator->free(penetrationMap);
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
			NX_TRIANGLE_MESH_CPP, kTriangleMeshPMapCreateFailedLine, 0, "%s", kTriangleMeshPMapCreateFailedMessage);
		return false;
		}
	mPMap = penetrationMap;
	return true;
	}

bool TriangleMesh::hasPMap() const
	{
	return mPMap != 0;
	}

NxU32 TriangleMesh::getPMapSize() const
	{
	if(!mPMap)
		return 0;
	MemoryStream stream(0x1000, 0);
	if(!mPMap->serialize(stream))
		return 0;
	return stream.getLength();
	}

bool TriangleMesh::getPMapData(NxPMap& pmap) const
	{
	if(!mPMap || !pmap.data)
		return false;
	MemoryStream stream(0x1000, 0);
	if(!mPMap->serialize(stream))
		return false;
	const NxU32 size = stream.getLength();
	if(pmap.dataSize != size)
		return false;
	stream.collapse(pmap.data);
	return true;
	}

NxTriangleMesh* TriangleMesh::publicHandle() const
	{
	return static_cast<NxTriangleMesh*>(mPublicObject);
	}

bool TriangleMesh::buildModel()
	{
	if(mInternal.mModel)
		{
		delete mInternal.mModel;
		mInternal.mModel = 0;
		}
	mInternal.mMeshInterface.SetNbTriangles(mInternal.mTriangleCount);
	mInternal.mMeshInterface.SetNbVertices(mInternal.mVertexCount);
	if(!mInternal.mTriangles || !mInternal.mVertices ||
		!mInternal.mMeshInterface.SetPointers(
			reinterpret_cast<const IndexedTriangle*>(mInternal.mTriangles),
			reinterpret_cast<const Point*>(mInternal.mVertices)))
		return false;
	Opcode::Model* model = new Opcode::Model;
	if(!model) return false;
	Opcode::OPCODECREATE create;
	create.mIMesh = &mInternal.mMeshInterface;
	if(!model->Build(create))
		{
		delete model;
		return false;
		}
	mInternal.mModel = model;
	return true;
	}

bool TriangleMesh::computeMassProperties()
	{
	NxTriangleMeshDesc desc;
	desc.setToDefault();
	desc.numVertices = mInternal.mVertexCount;
	desc.numTriangles = mInternal.mTriangleCount;
	desc.pointStrideBytes = sizeof(NxVec3);
	desc.triangleStrideBytes = sizeof(NxTriangle32);
	desc.points = mInternal.mVertices;
	desc.triangles = mInternal.mTriangles;
	NxIntegrals integrals;
	if(!NxComputeVolumeIntegrals(desc, 1.0f, integrals))
		return false;
	const NxReal sign = integrals.mass < 0.0 ? -1.0f : 1.0f;
	mCachedMass = static_cast<float>(integrals.mass * sign);
	for(unsigned i = 0; i < 3; ++i)
		{
		mCachedCenter[i] = integrals.COM[i];
		for(unsigned j = 0; j < 3; ++j)
			mCachedInertia[i * 3 + j] = static_cast<float>(integrals.COMInertiaTensor[i][j] * sign);
		}
	return true;
	}

bool TriangleMesh::loadFromDesc(const NxTriangleMeshDesc& source)
	{
	if(!source.isValid() || !nxFoundationSDKAllocator)
		return false;

	NxTriangleMeshDesc cooked;
	cooked = source;
	void* cookedPoints = 0;
	void* cookedTriangles = 0;
	if(source.flags & NX_MF_COMPUTE_CONVEX)
		{
		TriangleMeshHullAllocator allocator;
		if(!allocator.computeHull(source, cooked))
			return false;
		cookedPoints = const_cast<void*>(cooked.points);
		cookedTriangles = const_cast<void*>(cooked.triangles);
		}

	const NxU32 pointStride = cooked.pointStrideBytes;
	const NxU32 triangleStride = cooked.triangleStrideBytes;
	void* vertices = nxTriangleMeshCopy(cooked.points, cooked.numVertices, pointStride, sizeof(NxVec3));
	const NxU32 triangleCount = cooked.numTriangles ? cooked.numTriangles : cooked.numVertices / 3;
	void* triangles = nxFoundationSDKAllocator->malloc(
		triangleCount * sizeof(NxTriangle32), NX_MEMORY_PERSISTENT);
	if(triangles && cooked.triangles)
		{
		NxU8* dst = static_cast<NxU8*>(triangles);
		const NxU8* src = static_cast<const NxU8*>(cooked.triangles);
		const bool indices16 = (cooked.flags & NX_MF_16_BIT_INDICES) != 0;
		for(NxU32 i = 0; i < triangleCount; ++i)
			{
			NxTriangle32& outTriangle = reinterpret_cast<NxTriangle32*>(dst)[i];
			if(indices16)
				{
				const NxU16* in = reinterpret_cast<const NxU16*>(src + i * triangleStride);
				outTriangle.v[0] = in[0];
				outTriangle.v[1] = in[(cooked.flags & NX_MF_FLIPNORMALS) ? 2 : 1];
				outTriangle.v[2] = in[(cooked.flags & NX_MF_FLIPNORMALS) ? 1 : 2];
				}
			else
				{
				memcpy(outTriangle.v, src + i * triangleStride, sizeof(outTriangle.v));
				if(cooked.flags & NX_MF_FLIPNORMALS)
					{
					const NxU32 second = outTriangle.v[1];
					outTriangle.v[1] = outTriangle.v[2];
					outTriangle.v[2] = second;
					}
				}
			}
		}
	else if(triangles)
		{
		NxTriangle32* output = static_cast<NxTriangle32*>(triangles);
		for(NxU32 i = 0; i < triangleCount; ++i)
			{
		output[i].v[0] = i * 3;
		output[i].v[1] = i * 3 + 1;
		output[i].v[2] = i * 3 + 2;
		}
		}
	if(!vertices || !triangles)
		{
		nxTriangleMeshFree(vertices);
		nxTriangleMeshFree(triangles);
		nxTriangleMeshFree(cookedPoints);
		nxTriangleMeshFree(cookedTriangles);
		return false;
		}

	nxTriangleMeshFree(mInternal.mVertices);
	nxTriangleMeshFree(mInternal.mTriangles);
	nxTriangleMeshFreeTyped(mInternal.mMaterialIndices);
	nxTriangleMeshFreeTyped(mInternal.mFaceRemap);
	nxTriangleMeshFree(mInternal.mVertexNormals);
	mInternal.mVertexCount = cooked.numVertices;
	mInternal.mTriangleCount = triangleCount;
	mInternal.mVertices = vertices;
	mInternal.mTriangles = triangles;
	mInternal.mMaterialIndices = 0;
	mInternal.mFaceRemap = 0;
	mInternal.mVertexNormals = 0;
	const NxVec3* cookedVertices = static_cast<const NxVec3*>(mInternal.mVertices);
	for(unsigned axis = 0; axis < 3; ++axis)
		{
		mBounds44[axis] = FLT_MAX;
		mBounds44[axis + 3] = -FLT_MAX;
		}
	for(NxU32 i = 0; i < mInternal.mVertexCount; ++i)
		for(unsigned axis = 0; axis < 3; ++axis)
			{
		const float coordinate = cookedVertices[i][axis];
		if(coordinate < mBounds44[axis]) mBounds44[axis] = coordinate;
		if(coordinate > mBounds44[axis + 3]) mBounds44[axis + 3] = coordinate;
		}
	mConvexEdgeThreshold = source.convexEdgeThreshold;
	mHeightFieldVerticalAxis = source.heightFieldVerticalAxis;
	mHeightFieldVerticalExtent = source.heightFieldVerticalExtent;
	mHullFlags = cooked.flags & ~NX_MF_16_BIT_INDICES;
	mCachedMass = -1.0f;
	if(source.materialIndices && source.numTriangles)
		mInternal.mMaterialIndices = static_cast<NxU16*>(nxTriangleMeshCopy(source.materialIndices,
			source.numTriangles, source.materialIndexStride, sizeof(NxU16)));
	nxTriangleMeshFree(cookedPoints);
	nxTriangleMeshFree(cookedTriangles);
	// phys_fn_002260 routes the converted descriptor through 002256 before
	// building the OPCODE model. 002256 runs 002087 (MeshBuilder2 topology),
	// which canonicalizes face order, removes zero-area faces, welds indexed
	// geometry and produces the source-face remap used for material indices.
	if(!nxInternalMeshBuildTopology(&mInternal) || !buildModel())
		return false;
	if(source.pmap && !loadPMap(*source.pmap))
		return false;
	nxTriangleMeshDestroyConvexData(static_cast<TriangleMeshConvexData*>(mConvexMesh));
	mConvexMesh = 0;
	TriangleMeshConvexData* convexData = 0;
	if((mHullFlags & NX_MF_CONVEX) && !nxTriangleMeshBuildConvexData(&convexData, mInternal))
		return false;
	mConvexMesh = convexData;
	return true;
	}

bool TriangleMesh::saveToDesc(NxTriangleMeshDesc& desc) const
	{
	if(!mInternal.mVertices || !mInternal.mTriangles)
		return false;
	desc.numVertices = mInternal.mVertexCount;
	desc.numTriangles = mInternal.mTriangleCount;
	desc.pointStrideBytes = sizeof(NxVec3);
	desc.triangleStrideBytes = sizeof(NxTriangle32);
	desc.points = mInternal.mVertices;
	desc.triangles = mInternal.mTriangles;
	desc.flags = mConvexMesh ? NX_MF_CONVEX : 0;			// 002166 writes only the computed-convex bit.
	desc.materialIndexStride = mInternal.mMaterialIndices ? sizeof(NxU16) : 0;
	desc.materialIndices = mInternal.mMaterialIndices;
	desc.heightFieldVerticalAxis = static_cast<NxHeightFieldAxis>(mHeightFieldVerticalAxis);
	desc.heightFieldVerticalExtent = mHeightFieldVerticalExtent;
	desc.convexEdgeThreshold = mConvexEdgeThreshold;
	return true;
	}

NxU32 TriangleMesh::getCount(NxInternalArray array) const
	{
	switch(array)
		{
		case NX_ARRAY_VERTICES: return mInternal.mVertexCount;
		case NX_ARRAY_TRIANGLES: return mInternal.mTriangleCount;
		case NX_ARRAY_NORMALS: return mInternal.mVertexCount;
		case NX_ARRAY_HULL_VERTICES:
			return mConvexMesh ? static_cast<const TriangleMeshConvexData*>(mConvexMesh)->mNbVerts : 0;
		case NX_ARRAY_HULL_POLYGONS:
			return mConvexMesh ? static_cast<const TriangleMeshConvexData*>(mConvexMesh)->mNbPolygons : 0;
		default: return 0;
		}
	}

NxInternalFormat TriangleMesh::getFormat(NxInternalArray array) const
	{
	if(array == NX_ARRAY_VERTICES) return NX_FORMAT_FLOAT;
	if(array == NX_ARRAY_TRIANGLES) return NX_FORMAT_INT;
	if(array == NX_ARRAY_NORMALS) return NX_FORMAT_FLOAT;
	if(array == NX_ARRAY_HULL_VERTICES) return NX_FORMAT_FLOAT;
	if(array == NX_ARRAY_HULL_POLYGONS) return NX_FORMAT_INT;
	return NX_FORMAT_NODATA;
	}

const void* TriangleMesh::getBase(NxInternalArray array) const
	{
	if(array == NX_ARRAY_VERTICES) return mInternal.mVertices;
	if(array == NX_ARRAY_TRIANGLES) return mInternal.mTriangles;
	if(array == NX_ARRAY_HULL_VERTICES && mConvexMesh)
		return static_cast<const TriangleMeshConvexData*>(mConvexMesh)->mVerts;
	if(array == NX_ARRAY_NORMALS)
		{
		if(!mInternal.mVertexNormals)
			{
			InternalTriangleMesh* internal = const_cast<InternalTriangleMesh*>(&mInternal);
			__asm
				{
				mov ecx, internal
				call nxMeshComputeVertexNormals
				}
			}
		return mInternal.mVertexNormals;
		}
	return 0;
	}

NxU32 TriangleMesh::getStride(NxInternalArray array) const
	{
	if(array == NX_ARRAY_VERTICES || array == NX_ARRAY_NORMALS) return sizeof(NxVec3);
	if(array == NX_ARRAY_HULL_VERTICES) return sizeof(NxVec3);
	if(array == NX_ARRAY_TRIANGLES) return sizeof(NxTriangle32);
	return 0;
	}

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
