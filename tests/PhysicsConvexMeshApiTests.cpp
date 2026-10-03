#include "PhysicsPairLoader.h"

#include <stdio.h>

#include "NxPhysicsSDK.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxTriangleMeshShape.h"
#include "NxTriangleMeshShapeDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static NxU32 hashBytes(const void* data, NxU32 size)
	{
	const NxU8* bytes = static_cast<const NxU8*>(data);
	NxU32 hash = 2166136261u;
	for(NxU32 i = 0; i < size; ++i)
		{
		hash ^= bytes[i];
		hash *= 16777619u;
		}
	return hash;
	}

static void reportArray(const char* name, const NxTriangleMesh& mesh, NxInternalArray array)
	{
	const NxU32 count = mesh.getCount(0, array);
	const NxU32 stride = mesh.getStride(0, array);
	const NxInternalFormat format = mesh.getFormat(0, array);
	const void* base = mesh.getBase(0, array);
	const NxU32 hash = base && stride ? hashBytes(base, count * stride) : 0;
	printf(" %s=%u/%u/%u/%08x", name, count, (NxU32)format, stride, hash);
	}

static void reportMeshGeometry(const NxTriangleMesh& mesh)
	{
	const NxU32 triangleCount = mesh.getCount(0, NX_ARRAY_TRIANGLES);
	const NxU32* triangles = static_cast<const NxU32*>(mesh.getBase(0, NX_ARRAY_TRIANGLES));
	printf(" triangle_data=");
	for(NxU32 i = 0; triangles && i < triangleCount * 3; ++i)
		printf("%s%u", i ? "," : "", triangles[i]);
	const NxU32 vertexCount = mesh.getCount(0, NX_ARRAY_VERTICES);
	const NxVec3* vertices = static_cast<const NxVec3*>(mesh.getBase(0, NX_ARRAY_VERTICES));
	printf(" vertex_bits=");
	for(NxU32 i = 0; vertices && i < vertexCount; ++i)
		{
		NxU32 xyz[3];
		memcpy(xyz, &vertices[i], sizeof(xyz));
		printf("%s%08x:%08x:%08x", i ? "," : "", xyz[0], xyz[1], xyz[2]);
		}
	}

static bool runMeshCase(NxPhysicsSDK& sdk, const char* name, const NxTriangleMeshDesc& desc,
	NxU32 repeats)
	{
	for(NxU32 repeat = 0; repeat < repeats; ++repeat)
		{
		NxTriangleMesh* mesh = sdk.createTriangleMesh(desc);
		if(!mesh)
			{
			printf("mesh-case name=%s repeat=%u created=0\n", name, repeat);
			return false;
			}
		printf("mesh-case name=%s repeat=%u flags=%08x created=1 submeshes=%u",
			name, repeat, desc.flags, mesh->getSubmeshCount());
		reportArray("triangles", *mesh, NX_ARRAY_TRIANGLES);
		reportArray("vertices", *mesh, NX_ARRAY_VERTICES);
		reportArray("normals", *mesh, NX_ARRAY_NORMALS);
		reportArray("hull_vertices", *mesh, NX_ARRAY_HULL_VERTICES);
		reportArray("hull_polygons", *mesh, NX_ARRAY_HULL_POLYGONS);
		reportMeshGeometry(*mesh);
		NxTriangleMeshDesc saved;
		const bool savedOk = mesh->saveToDesc(saved);
		printf(" save=%u/%u/%u/%u/%u/%08x/%08x\n", savedOk ? 1u : 0u,
			saved.numVertices, saved.numTriangles, saved.pointStrideBytes, saved.triangleStrideBytes,
			saved.points && saved.numVertices ? hashBytes(saved.points, saved.numVertices * saved.pointStrideBytes) : 0,
			saved.triangles && saved.numTriangles ? hashBytes(saved.triangles, saved.numTriangles * saved.triangleStrideBytes) : 0);
		sdk.releaseTriangleMesh(*mesh);
		}
	return true;
	}

static bool runTriangleMeshActorCase(NxPhysicsSDK& sdk, const NxTriangleMeshDesc& meshDesc)
	{
	NxTriangleMesh* mesh = sdk.createTriangleMesh(meshDesc);
	if(!mesh)
		{
		printf("mesh-actor name=static_tetra mesh=0 actor=0\n");
		return false;
		}
	NxSceneDesc sceneDesc;
	NxScene* scene = sdk.createScene(sceneDesc);
	if(!scene)
		{
		sdk.releaseTriangleMesh(*mesh);
		printf("mesh-actor name=static_tetra mesh=1 scene=0 actor=0\n");
		return false;
		}
	NxTriangleMeshShapeDesc shapeDesc;
	shapeDesc.meshData = mesh;
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&shapeDesc);
	NxActor* staticActor = scene->createActor(staticDesc);
	NxU32 staticShapeCount = staticActor ? staticActor->getNbShapes() : 0;
	NxU32 staticShapeType = staticActor && staticShapeCount
		? staticActor->getShapes()[0]->getType() : NX_SHAPE_COUNT;
	printf("mesh-actor name=static_tetra mesh=1 scene=1 actor=%u shapes=%u type=%u\n",
		staticActor ? 1u : 0u, staticShapeCount, staticShapeType);
	sdk.releaseScene(*scene);
	sdk.releaseTriangleMesh(*mesh);
	return staticActor != 0 && staticShapeCount == 1 && staticShapeType == NX_SHAPE_MESH;
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	if(argc != 2)
		return nxFail("usage: NxPhysicsConvexMeshTests <pair directory>");

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsConvexMeshTests", pairDirectory, &physics);
	if(status)
		return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		{
		FreeLibrary(physics);
		return nxFail("NxCreatePhysicsSDK missing");
		}

	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}

	const NxVec3 tetraPoints[] = {
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f),
		NxVec3(0.0f, 1.0f, 0.0f), NxVec3(0.0f, 0.0f, 1.0f)
		};
	const NxU32 tetraTriangles[] = {
		0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3
		};
	const NxU16 tetraTriangles16[] = {
		0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3
		};
	const NxU16 tetraMaterials[] = { 2, 5, 7, 11 };
	struct PaddedPoint { NxVec3 point; NxReal padding; };
	const PaddedPoint paddedPoints[] = {
		{ NxVec3(0.0f, 0.0f, 0.0f), 101.0f }, { NxVec3(1.0f, 0.0f, 0.0f), 102.0f },
		{ NxVec3(0.0f, 1.0f, 0.0f), 103.0f }, { NxVec3(0.0f, 0.0f, 1.0f), 104.0f }
		};
	struct PaddedTriangle { NxU32 index[3]; NxU32 padding; };
	const PaddedTriangle paddedTriangles[] = {
		{ { 0, 2, 1 }, 0xaaaaaaaau }, { { 0, 1, 3 }, 0xbbbbbbbbu },
		{ { 0, 3, 2 }, 0xccccccccu }, { { 1, 2, 3 }, 0xddddddddu }
		};
	const NxU8 paddedTriangles16[][8] = {
		{ 0,0, 2,0, 1,0, 0xaa,0xaa }, { 0,0, 1,0, 3,0, 0xbb,0xbb },
		{ 0,0, 3,0, 2,0, 0xcc,0xcc }, { 1,0, 2,0, 3,0, 0xdd,0xdd }
		};

	NxTriangleMeshDesc computed;
	computed.numVertices = 4;
	computed.pointStrideBytes = sizeof(NxVec3);
	computed.points = tetraPoints;
	computed.flags = NX_MF_CONVEX | NX_MF_COMPUTE_CONVEX;

	NxTriangleMeshDesc computedPadded = computed;
	computedPadded.pointStrideBytes = sizeof(PaddedPoint);
	computedPadded.points = paddedPoints;

	NxTriangleMeshDesc precomputed = computed;
	precomputed.numTriangles = 4;
	precomputed.triangleStrideBytes = sizeof(NxTriangle32);
	precomputed.triangles = tetraTriangles;
	precomputed.flags = NX_MF_CONVEX;

	NxTriangleMeshDesc padded16 = precomputed;
	padded16.pointStrideBytes = sizeof(PaddedPoint);
	padded16.points = paddedPoints;
	padded16.triangleStrideBytes = sizeof(paddedTriangles16[0]);
	padded16.triangles = paddedTriangles16;
	padded16.flags = NX_MF_CONVEX | NX_MF_16_BIT_INDICES;
	padded16.materialIndexStride = sizeof(NxU16);
	padded16.materialIndices = tetraMaterials;

	NxTriangleMeshDesc flipped = precomputed;
	flipped.flags |= NX_MF_FLIPNORMALS;

	bool ok = true;
	ok = runMeshCase(*sdk, "precomputed_tetra_first", precomputed, 1) && ok;
	ok = runMeshCase(*sdk, "precomputed_tetra_flip_normals", flipped, 1) && ok;
	ok = runMeshCase(*sdk, "computed_tetra", computed, 2) && ok;
	ok = runMeshCase(*sdk, "computed_tetra_padded_points", computedPadded, 1) && ok;
	ok = runMeshCase(*sdk, "precomputed_tetra", precomputed, 1) && ok;
	ok = runMeshCase(*sdk, "precomputed_tetra_padded16_materials", padded16, 1) && ok;
	ok = runTriangleMeshActorCase(*sdk, precomputed) && ok;

	sdk->release();
	const int identityStatus = nxReportPairIdentity(pairDirectory);
	if(ok && identityStatus)
		ok = false;
	FreeLibrary(physics);
	return ok ? 0 : nxFail("one or more valid triangle mesh descriptors failed");
	}
