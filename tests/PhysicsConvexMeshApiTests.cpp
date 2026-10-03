#include "PhysicsPairLoader.h"

#include <stdio.h>

#include "NxPhysicsSDK.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
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

// The polygon support path reads the vertex graph pointer at
// TriangleMesh+0xa0 -> hull+0x64. Keep this observation independent of pointer
// addresses so the paired oracle run can tell a constructed graph from an
// unset/sentinel field without making assumptions about allocator locations.
static bool reportConvexVertexGraph(const NxTriangleMesh& mesh, const char* name)
	{
	const unsigned char* wrapper = reinterpret_cast<const unsigned char*>(&mesh);
	NxU32 meshImage = 0;
	memcpy(&meshImage, wrapper + 4, sizeof(meshImage));
	NxU32 hullImage = 0;
	if(meshImage)
		memcpy(&hullImage, reinterpret_cast<const unsigned char*>(meshImage) + 0xa0, sizeof(hullImage));
	NxU32 graphImage = 0;
	if(hullImage)
		memcpy(&graphImage, reinterpret_cast<const unsigned char*>(hullImage) + 0x64, sizeof(graphImage));
	if(!graphImage)
		{
		printf("mesh-graph-%s=none", name);
		return false;
		}
	if(graphImage < 0x10000)
		{
		printf("mesh-graph-%s=sentinel:%u", name, graphImage);
		return false;
		}
	const unsigned char* graph = reinterpret_cast<const unsigned char*>(graphImage);
	NxU32 counts = 0, offsets = 0, neighbours = 0, vertexCount = 0, adjacentCount = 0;
	memcpy(&vertexCount, graph, sizeof(vertexCount));
	memcpy(&adjacentCount, graph + 4, sizeof(adjacentCount));
	memcpy(&counts, graph + 8, sizeof(counts));
	memcpy(&offsets, graph + 0xc, sizeof(offsets));
	memcpy(&neighbours, graph + 0x10, sizeof(neighbours));
	if(vertexCount > 65536 || adjacentCount > 1048576 || !counts || !offsets || !neighbours)
		{
		printf("mesh-graph-%s=malformed", name);
		return false;
		}
	const NxU32 countsHash = hashBytes(reinterpret_cast<const void*>(counts), vertexCount * 4);
	const NxU32 offsetsHash = hashBytes(reinterpret_cast<const void*>(offsets), vertexCount * 4);
	const NxU32 neighboursHash = hashBytes(reinterpret_cast<const void*>(neighbours), adjacentCount * 4);
	printf("mesh-graph-%s=valid/%u/%u/%08x/%08x/%08x", name, vertexCount, adjacentCount,
		countsHash, offsetsHash, neighboursHash);
	return true;
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
	const bool graphBuilt = reportConvexVertexGraph(*mesh, "tetra");
	printf("\n");
	NxBodyDesc bodyDesc;
	NxActorDesc dynamicDesc;
	dynamicDesc.body = &bodyDesc;
	dynamicDesc.density = 1.0f;
	dynamicDesc.shapes.pushBack(&shapeDesc);
	NxActor* dynamicActor = scene->createActor(dynamicDesc);
	NxU32 dynamicShapeCount = dynamicActor ? dynamicActor->getNbShapes() : 0;
	NxU32 dynamicShapeType = dynamicActor && dynamicShapeCount
		? dynamicActor->getShapes()[0]->getType() : NX_SHAPE_COUNT;
	NxU32 dynamic = dynamicActor && dynamicActor->isDynamic() ? 1u : 0u;
	printf("mesh-actor name=dynamic_tetra mesh=1 scene=1 actor=%u shapes=%u type=%u dynamic=%u\n",
		dynamicActor ? 1u : 0u, dynamicShapeCount, dynamicShapeType,
		dynamic);
	if(dynamicActor && dynamicShapeCount)
		{
		NxBounds3 bounds;
		dynamicActor->getShapes()[0]->getWorldBounds(bounds);
		NxU32 words[6];
		memcpy(words, &bounds, sizeof(words));
		printf("mesh-actor-bounds=%08x:%08x:%08x:%08x:%08x:%08x\n",
			words[0], words[1], words[2], words[3], words[4], words[5]);
		}
	if(dynamicActor)
		{
		const NxReal mass = dynamicActor->getMass();
		const NxVec3 center = dynamicActor->getCMassLocalPositionVal();
		const NxVec3 inertia = dynamicActor->getMassSpaceInertiaTensorVal();
		NxU32 bits[7];
		memcpy(bits, &mass, sizeof(mass));
		memcpy(bits + 1, &center, sizeof(center));
		memcpy(bits + 4, &inertia, sizeof(inertia));
		printf("mesh-actor-mass mass=%08x center=%08x:%08x:%08x inertia=%08x:%08x:%08x\n",
			bits[0], bits[1], bits[2], bits[3], bits[4], bits[5], bits[6]);
		}
	scene->simulate(1.0f / 60.0f);
	const bool fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	printf("mesh-actor-sim fetched=%u\n", fetched ? 1u : 0u);
	sdk.releaseScene(*scene);
	sdk.releaseTriangleMesh(*mesh);
	return staticActor != 0 && staticShapeCount == 1 && staticShapeType == NX_SHAPE_MESH
		&& dynamicActor != 0 && dynamicShapeCount == 1 && dynamicShapeType == NX_SHAPE_MESH
		&& dynamic && fetched && graphBuilt;
	}

static bool runMeshGraphCase(NxPhysicsSDK& sdk, const NxTriangleMeshDesc& desc)
	{
	NxTriangleMesh* mesh = sdk.createTriangleMesh(desc);
	if(!mesh)
		{
		printf("mesh-graph-octa=mesh-create-failed\n");
		return false;
		}
	const bool graphBuilt = reportConvexVertexGraph(*mesh, "octa");
	printf("\n");
	sdk.releaseTriangleMesh(*mesh);
	return graphBuilt;
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
	const NxVec3 octaPoints[] = {
		NxVec3(0.0f, 1.0f, 0.0f), NxVec3(0.0f, -1.0f, 0.0f),
		NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 1.0f),
		NxVec3(-1.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, -1.0f)
		};
	const NxU32 octaTriangles[] = {
		0,3,2, 0,4,3, 0,5,4, 0,2,5,
		1,2,3, 1,3,4, 1,4,5, 1,5,2
		};
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
	NxTriangleMeshDesc octa;
	octa.numVertices = sizeof(octaPoints) / sizeof(octaPoints[0]);
	octa.pointStrideBytes = sizeof(NxVec3);
	octa.points = octaPoints;
	octa.numTriangles = sizeof(octaTriangles) / (3 * sizeof(NxU32));
	octa.triangleStrideBytes = sizeof(NxTriangle32);
	octa.triangles = octaTriangles;
	octa.flags = NX_MF_CONVEX;

	bool ok = true;
	ok = runMeshCase(*sdk, "precomputed_tetra_first", precomputed, 1) && ok;
	ok = runMeshCase(*sdk, "precomputed_tetra_flip_normals", flipped, 1) && ok;
	ok = runMeshCase(*sdk, "computed_tetra", computed, 2) && ok;
	ok = runMeshCase(*sdk, "computed_tetra_padded_points", computedPadded, 1) && ok;
	ok = runMeshCase(*sdk, "precomputed_tetra", precomputed, 1) && ok;
	ok = runMeshCase(*sdk, "precomputed_tetra_padded16_materials", padded16, 1) && ok;
	ok = runTriangleMeshActorCase(*sdk, precomputed) && ok;
	ok = runMeshGraphCase(*sdk, octa) && ok;

	sdk->release();
	const int identityStatus = nxReportPairIdentity(pairDirectory);
	if(ok && identityStatus)
		ok = false;
	FreeLibrary(physics);
	return ok ? 0 : nxFail("one or more valid triangle mesh descriptors failed");
	}
