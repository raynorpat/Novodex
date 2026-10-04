/*----------------------------------------------------------------------------*\
|
|                              NovodeX Technology
|
|                              www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "TriangleMesh.h"
#include "PhysicsSDK.h"
#include "FoundationSDK.h"
#include "IceMeshBuilder2.h"
#include "EdgeList.h"

#include <new>
#include <string.h>

static_assert(sizeof(InternalTriangleMesh) == 0x38,
	"InternalTriangleMesh is 0x38 bytes in the measured TriangleMesh layout");

// phys_fn_002065 (0x00051ec0). The first nine words are cleared here; the
// embedded OPCODE MeshInterface constructor at +0x24 clears its first four.
void nxInternalMeshInit(InternalTriangleMesh* mesh)
	{
	memset(mesh, 0, 9 * sizeof(NxU32));
	memset(reinterpret_cast<NxU8*>(mesh) + 0x24, 0, 4 * sizeof(NxU32));
	}

// phys_fn_002067 (0x00051ef0). The order is observable through allocators:
// model, per-triangle data, normals, remap, materials, triangles, vertices.
void nxInternalMeshRelease(InternalTriangleMesh* mesh)
	{
	if(mesh->mModel)
		{
		delete mesh->mModel;
		mesh->mModel = 0;
		}
	if(mesh->mTriangleData)
		{
		nxFoundationSDKAllocator->free(mesh->mTriangleData);
		mesh->mTriangleData = 0;
		}
	if(mesh->mVertexNormals)
		{
		nxFoundationSDKAllocator->free(mesh->mVertexNormals);
		mesh->mVertexNormals = 0;
		}
	if(mesh->mFaceRemap)
		{
		nxFoundationSDKAllocator->free(mesh->mFaceRemap);
		mesh->mFaceRemap = 0;
		}
	if(mesh->mMaterialIndices)
		{
		nxFoundationSDKAllocator->free(mesh->mMaterialIndices);
		mesh->mMaterialIndices = 0;
		}
	if(mesh->mTriangles)
		{
		nxFoundationSDKAllocator->free(mesh->mTriangles);
		mesh->mTriangles = 0;
		}
	if(mesh->mVertices)
		{
		nxFoundationSDKAllocator->free(mesh->mVertices);
		mesh->mVertices = 0;
		}
	}

// phys_fn_002069 (0x00051fa0).
void nxInternalMeshAllocateVertices(InternalTriangleMesh* mesh, NxU32 count)
	{
	mesh->mVertexCount = count;
	mesh->mVertices = nxFoundationSDKAllocator->malloc(count * 0x0c, NX_MEMORY_PERSISTENT);
	}

// phys_fn_002071 (0x00051fd0).
void nxInternalMeshAllocateTriangles(InternalTriangleMesh* mesh, NxU32 count)
	{
	mesh->mTriangleCount = count;
	mesh->mTriangles = nxFoundationSDKAllocator->malloc(count * 0x0c, NX_MEMORY_PERSISTENT);
	}

// phys_fn_002073 (0x00052000). It leaves the destination untouched when the
// triangle count is zero, matching the branch before the allocation call.
void nxInternalMeshAllocateMaterials(InternalTriangleMesh* mesh)
	{
	if(mesh->mTriangleCount)
		mesh->mMaterialIndices = static_cast<NxU16*>(nxFoundationSDKAllocator->malloc(
			mesh->mTriangleCount * sizeof(NxU16), NX_MEMORY_PERSISTENT));
	}

// phys_fn_002075 (0x00052030).
void nxInternalMeshAllocateFaceRemap(InternalTriangleMesh* mesh)
	{
	if(mesh->mTriangleCount)
		mesh->mFaceRemap = static_cast<NxU32*>(nxFoundationSDKAllocator->malloc(
			mesh->mTriangleCount * sizeof(NxU32), NX_MEMORY_PERSISTENT));
	}

// phys_fn_002087 (0x000523c0). Rebuild the public descriptor's raw arrays
// through Ice's MeshBuilder2, then split vertices shared by non-manifold edges.
bool nxInternalMeshBuildTopology(InternalTriangleMesh* mesh)
	{
	MeshBuilder2 builder;
	MBCREATE create;
	memset(&create, 0, sizeof(create));
	create.NbVerts = mesh->mVertexCount;
	create.NbFaces = mesh->mTriangleCount;
	create.Verts = static_cast<const IceMaths::Point*>(mesh->mVertices);
	create.KillZeroAreaFaces = true;
	create.IndexedGeo = true;
	create.IndexedUVW = true;
	create.IndexedColors = true;
	create.RelativeIndices = true;
	create.WeightNormalWithAngles = true;
	if(!builder.Init(create))
		return false;

	const NxU32* input = static_cast<const NxU32*>(mesh->mTriangles);
	for(NxU32 i = 0; i < mesh->mTriangleCount; ++i)
		{
		MBFACEINFO face;
		memset(&face, 0, sizeof(face));
		face.Index = i;
		face.MaterialID = 0xffffffffu;
		face.SmoothingGroups = 1;
		face.VRefs = input + i * 3;
		if(!builder.AddFace(face))
			return false;
		}

	MBRESULT result;
	memset(&result, 0, sizeof(result));
	if(!builder.Build(result))
		return false;

	if(mesh->mTriangles)
		{
		nxFoundationSDKAllocator->free(mesh->mTriangles);
			mesh->mTriangles = 0;
		}
	if(mesh->mVertices)
		{
		nxFoundationSDKAllocator->free(mesh->mVertices);
			mesh->mVertices = 0;
		}
	if(mesh->mFaceRemap)
		{
		nxFoundationSDKAllocator->free(mesh->mFaceRemap);
			mesh->mFaceRemap = 0;
		}

	if(result.FaceRemap && result.NbFaces)
		{
		mesh->mFaceRemap = static_cast<NxU32*>(nxFoundationSDKAllocator->malloc(
			result.NbFaces * sizeof(NxU32), NX_MEMORY_PERSISTENT));
		if(!mesh->mFaceRemap)
			return false;
		memcpy(mesh->mFaceRemap, result.FaceRemap, result.NbFaces * sizeof(NxU32));
		}
	if(mesh->mMaterialIndices && mesh->mFaceRemap)
		{
		NxU16* remapped = static_cast<NxU16*>(nxFoundationSDKAllocator->malloc(
			result.NbFaces * sizeof(NxU16), NX_MEMORY_PERSISTENT));
		if(!remapped)
			return false;
		for(NxU32 i = 0; i < result.NbFaces; ++i)
			remapped[i] = mesh->mMaterialIndices[mesh->mFaceRemap[i]];
		nxFoundationSDKAllocator->free(mesh->mMaterialIndices);
		mesh->mMaterialIndices = remapped;
		}

	mesh->mVertexCount = result.NbOutVerts;
	mesh->mTriangleCount = result.NbFaces;
	mesh->mVertices = nxFoundationSDKAllocator->malloc(mesh->mVertexCount * 12, NX_MEMORY_PERSISTENT);
	mesh->mTriangles = nxFoundationSDKAllocator->malloc(mesh->mTriangleCount * 12, NX_MEMORY_PERSISTENT);
	if((mesh->mVertexCount && !mesh->mVertices) || (mesh->mTriangleCount && !mesh->mTriangles))
		return false;
	NxU8* vertices = static_cast<NxU8*>(mesh->mVertices);
	for(NxU32 i = 0; i < mesh->mVertexCount; ++i)
		memcpy(vertices + i * 12, result.Verts + result.VRefs[i] * 3, 12);
	memcpy(mesh->mTriangles, result.Topology, mesh->mTriangleCount * 12);

	// 002087 builds an EdgeList and duplicates the endpoints of edges shared by
	// more than two faces. Ordinary manifold meshes (including convex hulls)
	// retain the MeshBuilder2 output byte for byte.
	EdgeList edges;
	EDGELISTCREATE edgeCreate;
	memset(&edgeCreate, 0, sizeof(edgeCreate));
	edgeCreate.NbFaces = mesh->mTriangleCount;
	edgeCreate.DFaces = static_cast<const NxU32*>(mesh->mTriangles);
	// The listing requests the edge-to-face tables only; Verts is null, so the
	// active-edge geometric pass is not entered.
	edgeCreate.VerticesToEdges = true;
	edgeCreate.Epsilon = 1.0e-5f;
	if(!edges.Init(edgeCreate))
		return false;

	NxU32* triangles = static_cast<NxU32*>(mesh->mTriangles);
	const NxU8* points = static_cast<const NxU8*>(mesh->mVertices);
	IceCore::Container splitPoints;
	NxU32 newVertexCount = mesh->mVertexCount;
	NxU32 perturbation = 1;
	NxU32 ordinal = 0;
	for(NxU32 edge = 0; edge < edges.mNbEdges; ++edge)
		{
		const EdgeDesc& desc = edges.mEdgeToTriangles[edge];
		if(desc.Count <= 2)
			continue;
		const EdgeData& endpoints = edges.mEdges[edge];
		NxU32 firstIndex = 0;
		NxU32 secondIndex = 0;
		for(NxU32 extra = 2; extra < desc.Count; ++extra)
			{
			const NxU32 face = edges.mFacesByEdges[desc.Offset + extra];
			if(((extra - 2) & 1) == 0)
				{
				firstIndex = newVertexCount++;
				secondIndex = newVertexCount++;
				++ordinal;
				if(ordinal == 8)
					{
					++perturbation;
					ordinal = 1;
					}
				NxU32 first[3];
				NxU32 second[3];
				memcpy(first, points + endpoints.Ref0 * 12, sizeof(first));
				memcpy(second, points + endpoints.Ref1 * 12, sizeof(second));
				if(ordinal & 1) first[0] ^= perturbation;
				if(ordinal & 2) first[1] ^= perturbation;
				if(ordinal & 4) first[2] ^= perturbation;
				if(ordinal & 1) second[0] ^= perturbation;
				if(ordinal & 2) second[1] ^= perturbation;
				if(ordinal & 4) second[2] ^= perturbation;
				nxIceContainerAddPoint(&splitPoints, 0, first);
				nxIceContainerAddPoint(&splitPoints, 0, second);
				}
			for(NxU32 endpoint = 0; endpoint < 2; ++endpoint)
				{
				const NxU32 oldVertex = endpoint ? endpoints.Ref1 : endpoints.Ref0;
				const NxU32 newIndex = endpoint ? secondIndex : firstIndex;
				for(NxU32 corner = 0; corner < 3; ++corner)
					if(triangles[face * 3 + corner] == oldVertex)
						triangles[face * 3 + corner] = newIndex;
				}
		}
		}
	if(newVertexCount != mesh->mVertexCount)
		{
		NxU8* expanded = static_cast<NxU8*>(nxFoundationSDKAllocator->malloc(
			newVertexCount * 12, NX_MEMORY_PERSISTENT));
		if(!expanded)
			return false;
		memcpy(expanded, points, mesh->mVertexCount * 12);
		memcpy(expanded + mesh->mVertexCount * 12, splitPoints.GetEntries(),
			(newVertexCount - mesh->mVertexCount) * 12);
		nxFoundationSDKAllocator->free(mesh->mVertices);
		mesh->mVertices = expanded;
		mesh->mVertexCount = newVertexCount;
		}
	return true;
	}

// phys_fn_002079 (0x000521b0). Replaces the old record array if present, then
// emits one oriented plane for every 32-bit indexed triangle.
void nxInternalMeshBuildTriangleData(InternalTriangleMesh* mesh)
	{
	if(mesh->mTriangleData)
		{
		nxFoundationSDKAllocator->free(mesh->mTriangleData);
		mesh->mTriangleData = 0;
		}
	mesh->mTriangleData = nxFoundationSDKAllocator->malloc(
			mesh->mTriangleCount << 4, NX_MEMORY_PERSISTENT);

	IceMaths::Plane* planes = static_cast<IceMaths::Plane*>(mesh->mTriangleData);
	const NxU32* triangles = static_cast<const NxU32*>(mesh->mTriangles);
	const IceMaths::Point* vertices = static_cast<const IceMaths::Point*>(mesh->mVertices);
	for(NxU32 i = 0; i < mesh->mTriangleCount; ++i)
		{
		const NxU32* indices = triangles + i * 3;
		planes[i].Set(vertices[indices[0]], vertices[indices[1]], vertices[indices[2]]);
		}
	}

// phys_fn_002083 (0x00052280). The image allocates Model through the host
// SDK allocator, while the arrays it references remain Foundation-owned.
bool nxInternalMeshBuildModel(InternalTriangleMesh* mesh, NxU32 extendAxis,
	NxReal extendValue, const void* deserializeFrom)
	{
	if(mesh->mModel)
		{
		delete mesh->mModel;
		mesh->mModel = 0;
		}

	Opcode::MeshInterface* interface = &mesh->mMeshInterface;
	interface->SetNbVertices(mesh->mVertexCount);
	interface->SetNbTriangles(mesh->mTriangleCount);
	interface->SetPointers(
		static_cast<const IceMaths::IndexedTriangle*>(mesh->mTriangles),
		static_cast<const IceMaths::Point*>(mesh->mVertices));

	Opcode::OPCODECREATE create;
	create.mIMesh = interface;
	create.mDeserializeFrom = deserializeFrom;
	create.mSettings.mLimit = 1;
	create.mSettings.mRules = 0x22;
	create.mNoLeaf = true;
	if(PhysicsSDK::instance)
		create.mQuantized = PhysicsSDK::instance->getParameter(NX_CONTINUOUS_CD) != 1.0f;
	// The image reads phys_data_003482 at RVA 0x00123b3c into mKeepOriginal.
	// The pinned oracle initializes these bytes to zero and the census records no
	// write references, so OPCODECREATE's false default matches the observed value.
	create.mCanRemap = false;
	if(extendAxis != 0xff)
		{
		create.mSettings.mNovodeXExtendAxis = static_cast<NxI32>(extendAxis);
		create.mSettings.mNovodeXExtendValue = extendValue;
		}

	void* memory = nxGetSdkAllocator()->malloc(sizeof(Opcode::Model), NX_MEMORY_PERSISTENT);
	mesh->mModel = memory ? ::new(memory) Opcode::Model() : 0;
	const bool built = mesh->mModel && mesh->mModel->Build(create);
	if(!built)
		NxFoundation::FoundationSDK::error(static_cast<NxErrorCode>(4),
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\InternalTriangleMesh.cpp",
			0x1c1, 0, "Opcode is not OK.");
	return built;
	}
