#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "PhysicsPairLoader.h"
#include "NxUserAllocator.h"
#include "NxUserOutputStream.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxPMap.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);
typedef bool (NX_CALL_CONV *CreatePMapFn)(
	NxPMap&, const NxTriangleMesh&, NxU32, NxUserOutputStream*);
typedef bool (NX_CALL_CONV *ReleasePMapFn)(NxPMap&);

#ifndef NX_PMAP_COMPUTE_DENSITY
#define NX_PMAP_COMPUTE_DENSITY 32
#endif

static NxU32 nxMeshFloatBits(NxReal value)
	{
	union { NxReal real; NxU32 bits; } word = { value };
	return word.bits;
	}

static unsigned long long nxMeshArrayHash(const NxTriangleMesh& mesh, NxInternalArray array)
	{
	const NxU8* bytes = static_cast<const NxU8*>(mesh.getBase(0, array));
	const NxU32 byteCount = mesh.getCount(0, array) * mesh.getStride(0, array);
	unsigned long long hash = 14695981039346656037ull;
	for(NxU32 i = 0; i < byteCount; ++i)
		{
		hash ^= bytes[i];
		hash *= 1099511628211ull;
		}
	return hash;
	}

static void nxPMapStoreBits(unsigned char* data, unsigned& bitOffset, NxU32 value,
	unsigned bitCount)
	{
	for(unsigned i = bitCount; i != 0; --i, ++bitOffset)
		if((value >> (i - 1)) & 1u)
			data[bitOffset >> 3] |= static_cast<unsigned char>(0x80u >> (bitOffset & 7));
	}

static unsigned long long nxPMapByteHash(const unsigned char* bytes, NxU32 count)
	{
	unsigned long long hash = 14695981039346656037ull;
	for(NxU32 i = 0; i < count; ++i)
		{
		hash ^= bytes[i];
		hash *= 1099511628211ull;
		}
	return hash;
	}

class TriangleMeshApiAllocator : public NxUserAllocator
	{
	public:
	TriangleMeshApiAllocator(): liveAllocations(0) {}
	void* mallocDEBUG(size_t size, const char*, int) override { return allocate(size); }
	void* malloc(size_t size) override { return allocate(size); }
	void* realloc(void* memory, size_t size) override
		{
		void* result = ::realloc(memory, size);
		if(!memory && result)
			{
			++liveAllocations;
			}
		else if(memory && !size && !result)
			{
			--liveAllocations;
			}
		return result;
		}
	void free(void* memory) override
		{
		if(memory)
			{
			--liveAllocations;
			}
		::free(memory);
		}
	long long liveAllocations;
	private:
	void* allocate(size_t size)
		{
		void* memory = ::malloc(size);
		if(memory)
			{
			++liveAllocations;
			}
		return memory;
		}
	};

static TriangleMeshApiAllocator gAllocator;

class TriangleMeshApiOutputStream : public NxUserOutputStream
	{
	public:
	TriangleMeshApiOutputStream(): errors(0), lastCode(NXE_NO_ERROR), lastLine(0)
		{ message[0] = 0; file[0] = 0; }
	void reportError(NxErrorCode code, const char* text, const char* source, int line) override
		{
		++errors;
		lastCode = code;
		lastLine = line;
		copy(message, sizeof(message), text);
		copy(file, sizeof(file), source);
		}
	NxAssertResponse reportAssertViolation(const char*, const char*, int) override
		{ return NX_AR_CONTINUE; }
	void print(const char*) override {}
	void reset()
		{ errors = 0; lastCode = NXE_NO_ERROR; lastLine = 0; message[0] = 0; file[0] = 0; }
	unsigned errors;
	NxErrorCode lastCode;
	int lastLine;
	char message[128];
	char file[96];
	private:
	static void copy(char* destination, size_t capacity, const char* source)
		{
		if(!source) { destination[0] = 0; return; }
		size_t i = 0;
		for(; i + 1 < capacity && source[i]; ++i) destination[i] = source[i];
		destination[i] = 0;
		}
	};

static TriangleMeshApiOutputStream gOutputStream;

static int nxTestPMapComputeFirst(HMODULE physics, NxPhysicsSDK* sdk, NxU32 density)
	{
	CreatePMapFn createPMap = reinterpret_cast<CreatePMapFn>(
		GetProcAddress(physics, "NxCreatePMap"));
	ReleasePMapFn releasePMap = reinterpret_cast<ReleasePMapFn>(
		GetProcAddress(physics, "NxReleasePMap"));
	if(!createPMap || !releasePMap)
		return nxFail("PMap creation or release export is missing");

	const NxVec3 vertices[] = {
		NxVec3(-1.0f, -1.0f, -1.0f), NxVec3(1.3f, -0.8f, -0.9f),
		NxVec3(-0.7f, 1.2f, -0.6f), NxVec3(-0.5f, -0.4f, 1.5f)
		};
	const NxU32 triangles[] = { 0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3 };
	NxTriangleMeshDesc desc;
	desc.setToDefault();
	desc.numVertices = sizeof(vertices) / sizeof(vertices[0]);
	desc.points = vertices;
	desc.pointStrideBytes = sizeof(NxVec3);
	desc.numTriangles = sizeof(triangles) / (3 * sizeof(NxU32));
	desc.triangles = triangles;
	desc.triangleStrideBytes = 3 * sizeof(NxU32);
	NxTriangleMesh* mesh = sdk->createTriangleMesh(desc);
	if(!mesh)
		return nxFail("authored PMap fixture mesh creation failed");
	if(mesh->getCount(0, NX_ARRAY_VERTICES) != 4 ||
		mesh->getCount(0, NX_ARRAY_TRIANGLES) != 4 ||
		mesh->getStride(0, NX_ARRAY_VERTICES) != sizeof(NxVec3) ||
		mesh->getStride(0, NX_ARRAY_TRIANGLES) != 3 * sizeof(NxU32))
		{
		sdk->releaseTriangleMesh(*mesh);
		return nxFail("authored PMap fixture did not cook to its expected four-triangle layout");
		}
	// Pin the raw arrays after the cooker runs so this test measures PMap over
	// the same topology on both DLLs; qhull/cooking order has its own differential.
	memcpy(const_cast<void*>(mesh->getBase(0, NX_ARRAY_VERTICES)), vertices, sizeof(vertices));
	memcpy(const_cast<void*>(mesh->getBase(0, NX_ARRAY_TRIANGLES)), triangles, sizeof(triangles));
	srand(1);
	NxPMap pmap = { 0, 0 };
	const bool computed = createPMap(pmap, *mesh, density, 0);
	const char* dumpPath = getenv("NX_PMAP_DUMP_FILE");
	if(computed && pmap.data && dumpPath && dumpPath[0])
		{
		FILE* dump = fopen(dumpPath, "wb");
		if(dump)
			{
			fwrite(pmap.data, 1, pmap.dataSize, dump);
			fclose(dump);
			}
		}
	const unsigned long long hash = computed ? nxPMapByteHash(
		static_cast<const unsigned char*>(pmap.data), pmap.dataSize) : 0ull;
	printf("pmap_compute density=%u created=%u size=%u hash=%016llx\n",
		density, computed ? 1u : 0u, pmap.dataSize, hash);
	// Density 32 is the normal fixture; separate target builds pin 64 and 80
	// without allowing earlier API calls to advance the DLL's rand() stream.
	const NxU32 expectedSize = density == 32 ? 10444 :
		density == 64 ? 74563 : density == 80 ? 144272 : 0;
	const unsigned long long expectedHash = density == 32 ? 0x9a70de00aaf0edd4ull :
		density == 64 ? 0x2c38820e277e9465ull :
		density == 80 ? 0x1c6814b928a39f10ull : 0ull;
	const bool expected = computed && pmap.data && pmap.dataSize == expectedSize &&
		hash == expectedHash;
	const bool valid = expected && density == 32 &&
		memcmp(pmap.data, "PMAP", 4) == 0 &&
		static_cast<const NxU8*>(pmap.data)[4] == 4 &&
		static_cast<const NxU8*>(pmap.data)[8] == 32 &&
		hash == 0x9a70de00aaf0edd4ull;
	const bool loaded = valid && mesh->loadPMap(pmap);
	const NxU32 roundTripSize = loaded ? mesh->getPMapSize() : 0;
	void* roundTripBytes = roundTripSize ? malloc(roundTripSize) : 0;
	NxPMap roundTrip = { roundTripSize, roundTripBytes };
	const bool exported = loaded && roundTripBytes && mesh->getPMapData(roundTrip);
	const bool roundTripValid = loaded && mesh->hasPMap() && roundTripSize >= 12 && exported;
	free(roundTripBytes);
	if(pmap.data && !releasePMap(pmap))
		{
		sdk->releaseTriangleMesh(*mesh);
		return nxFail("NxReleasePMap failed for computed map");
		}
	if(density != 32)
		{
		sdk->releaseTriangleMesh(*mesh);
		return expected ? 0 : nxFail("alternate-resolution PMap differs from the oracle");
		}
	sdk->releaseTriangleMesh(*mesh);
	return roundTripValid ? 0 : nxFail("computed PMap did not survive load and export");
	}

static int nxTestDisconnectedPMap(HMODULE physics, NxPhysicsSDK* sdk)
	{
	CreatePMapFn createPMap = reinterpret_cast<CreatePMapFn>(
		GetProcAddress(physics, "NxCreatePMap"));
	ReleasePMapFn releasePMap = reinterpret_cast<ReleasePMapFn>(
		GetProcAddress(physics, "NxReleasePMap"));
	if(!createPMap || !releasePMap)
		return nxFail("PMap creation or release export is missing");

	const NxVec3 vertices[] = {
		NxVec3(-1.0f, -1.0f, -1.0f), NxVec3(1.0f, -1.0f, -1.0f),
		NxVec3(-1.0f, 1.0f, -1.0f), NxVec3(-1.0f, -1.0f, 1.0f),
		NxVec3(3.0f, -1.0f, -1.0f), NxVec3(5.0f, -1.0f, -1.0f),
		NxVec3(3.0f, 1.0f, -1.0f), NxVec3(3.0f, -1.0f, 1.0f)
		};
	const NxU32 triangles[] = {
		0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3,
		4, 6, 5, 4, 5, 7, 4, 7, 6, 5, 6, 7
		};
	NxTriangleMeshDesc desc;
	desc.setToDefault();
	desc.numVertices = sizeof(vertices) / sizeof(vertices[0]);
	desc.points = vertices;
	desc.pointStrideBytes = sizeof(NxVec3);
	desc.numTriangles = sizeof(triangles) / (3 * sizeof(NxU32));
	desc.triangles = triangles;
	desc.triangleStrideBytes = 3 * sizeof(NxU32);
	NxTriangleMesh* mesh = sdk->createTriangleMesh(desc);
	if(!mesh)
		return nxFail("disconnected-component PMap fixture mesh creation failed");
	if(mesh->getCount(0, NX_ARRAY_VERTICES) != 8 ||
		mesh->getCount(0, NX_ARRAY_TRIANGLES) != 8)
		{
		sdk->releaseTriangleMesh(*mesh);
		return nxFail("disconnected-component PMap fixture changed topology during cooking");
		}
	// Keep the cooked arrays intact: the PMap ray classifier uses the cached
	// Opcode model built from them during createTriangleMesh().
	srand(1);
	NxPMap pmap = { 0, 0 };
	const bool computed = createPMap(pmap, *mesh, 32, 0);
	const unsigned long long hash = computed ? nxPMapByteHash(
		static_cast<const unsigned char*>(pmap.data), pmap.dataSize) : 0ull;
	printf("pmap_compute topology=disconnected-tetrahedra density=32 created=%u size=%u hash=%016llx\n",
		computed ? 1u : 0u, pmap.dataSize, hash);
	const bool expected = computed && pmap.data && pmap.dataSize == 9522 &&
		hash == 0x847baf05835be7ceull;
	if(pmap.data && !releasePMap(pmap))
		{
		sdk->releaseTriangleMesh(*mesh);
		return nxFail("NxReleasePMap failed for disconnected-component map");
		}
	sdk->releaseTriangleMesh(*mesh);
	return expected ? 0 : nxFail("disconnected-component PMap differs from the oracle");
	}

static unsigned long long nxMeshSavedMaterialsHash(const NxTriangleMeshDesc& desc)
	{
	if(!desc.materialIndices || !desc.materialIndexStride || !desc.numTriangles)
		return 14695981039346656037ull;
	unsigned long long hash = 14695981039346656037ull;
	const NxU8* bytes = static_cast<const NxU8*>(desc.materialIndices);
	for(NxU32 i = 0; i < desc.numTriangles; ++i)
		for(unsigned byte = 0; byte < sizeof(NxU16); ++byte)
			{
			hash ^= bytes[i * desc.materialIndexStride + byte];
			hash *= 1099511628211ull;
			}
	return hash;
	}

static int nxTestDescriptorPath(NxPhysicsSDK* sdk, const char* name,
	const NxTriangleMeshDesc& input, NxU32 expectedVertices, NxU32 expectedTriangles, NxU32 expectedFlags)
	{
	NxTriangleMesh* mesh = sdk->createTriangleMesh(input);
	if(!mesh)
		return nxFail("triangle-mesh descriptor fixture creation failed");
	NxTriangleMeshDesc saved;
	saved.setToDefault();
	const bool savedOk = mesh->saveToDesc(saved);
	const NxU32 vertices = mesh->getCount(0, NX_ARRAY_VERTICES);
	const NxU32 triangles = mesh->getCount(0, NX_ARRAY_TRIANGLES);
	if(!savedOk || vertices != expectedVertices || triangles != expectedTriangles || saved.flags != expectedFlags)
		{
		fprintf(stderr, "triangle_mesh case=%s actual=%u.%u.%08x expected=%u.%u.%08x saved=%u\n",
			name, vertices, triangles, saved.flags, expectedVertices, expectedTriangles, expectedFlags, savedOk ? 1u : 0u);
		sdk->releaseTriangleMesh(*mesh);
		return nxFail("triangle-mesh descriptor fixture changed expected topology");
		}
	printf("triangle_mesh case=%s created=1 vertices=%u triangles=%u vertex_hash=%016llx triangle_hash=%016llx flags=%08x material_hash=%016llx\n",
		name, vertices, triangles, nxMeshArrayHash(*mesh, NX_ARRAY_VERTICES),
		nxMeshArrayHash(*mesh, NX_ARRAY_TRIANGLES), saved.flags,
		nxMeshSavedMaterialsHash(saved));
	sdk->releaseTriangleMesh(*mesh);
	return 0;
	}

static int nxTestDescriptorVariants(NxPhysicsSDK* sdk)
	{
	struct PointWithPadding { NxVec3 point; NxU32 padding; };
	const PointWithPadding points16[] = {
		{ NxVec3(0.0f, 0.0f, 0.0f), 0xaaaaaaaa },
		{ NxVec3(2.0f, 0.0f, 0.0f), 0xbbbbbbbb },
		{ NxVec3(0.0f, 2.0f, 0.0f), 0xcccccccc },
		{ NxVec3(2.0f, 2.0f, 0.0f), 0xdddddddd }
		};
	struct Triangle16WithPadding { NxU16 indices[3]; NxU16 padding; };
	const Triangle16WithPadding triangles16[] = {
		{ { 0, 1, 2 }, 0xaaaa }, { { 2, 1, 3 }, 0xbbbb }
		};
	struct MaterialWithPadding { NxU16 index; NxU16 padding; };
	const MaterialWithPadding materials[] = { { 3, 0xaaaa }, { 7, 0xbbbb } };
	NxTriangleMeshDesc indexed;
	indexed.setToDefault();
	indexed.numVertices = 4;
	indexed.points = points16;
	indexed.pointStrideBytes = sizeof(PointWithPadding);
	indexed.numTriangles = 2;
	indexed.triangles = triangles16;
	indexed.triangleStrideBytes = sizeof(Triangle16WithPadding);
	indexed.flags = NX_MF_16_BIT_INDICES | NX_MF_FLIPNORMALS;
	indexed.materialIndices = materials;
	indexed.materialIndexStride = sizeof(MaterialWithPadding);
	int status = nxTestDescriptorPath(sdk, "descriptor16", indexed, 4, 2, 0);
	if(status)
		return status;

	const NxVec3 unindexedPoints[] = {
		NxVec3(-2.0f, -1.0f, 0.0f), NxVec3(0.0f, -1.0f, 0.0f), NxVec3(-2.0f, 1.0f, 0.0f),
		NxVec3(0.0f, -1.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f), NxVec3(-2.0f, 1.0f, 0.0f)
		};
	NxTriangleMeshDesc unindexed;
	unindexed.setToDefault();
	unindexed.numVertices = 6;
	unindexed.points = unindexedPoints;
	unindexed.pointStrideBytes = sizeof(NxVec3);
	status = nxTestDescriptorPath(sdk, "implicit_indices", unindexed, 4, 2, 0);
	if(status)
		return status;

	const NxVec3 tetraPoints[] = {
		NxVec3(-1.0f, -1.0f, -1.0f), NxVec3(1.0f, -1.0f, -1.0f),
		NxVec3(-1.0f, 1.0f, -1.0f), NxVec3(-1.0f, -1.0f, 1.0f)
		};
	const NxU32 tetraTriangles[] = { 0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3 };
	NxTriangleMeshDesc precomputedConvex;
	precomputedConvex.setToDefault();
	precomputedConvex.numVertices = 4;
	precomputedConvex.points = tetraPoints;
	precomputedConvex.pointStrideBytes = sizeof(NxVec3);
	precomputedConvex.numTriangles = 4;
	precomputedConvex.triangles = tetraTriangles;
	precomputedConvex.triangleStrideBytes = 3 * sizeof(NxU32);
	precomputedConvex.flags = NX_MF_CONVEX;
	return nxTestDescriptorPath(sdk, "precomputed_convex", precomputedConvex, 4, 4, NX_MF_CONVEX);
	}

static int nxTestTriangleMeshReload(NxPhysicsSDK* sdk)
	{
	const NxVec3 plainPoints[] = {
		NxVec3(-1.0f, -1.0f, 0.0f), NxVec3(1.0f, -1.0f, 0.0f),
		NxVec3(-1.0f, 1.0f, 0.0f), NxVec3(1.0f, 1.0f, 0.0f)
		};
	const NxU32 plainIndices[] = { 0, 1, 2, 1, 3, 2 };
	NxTriangleMeshDesc plain;
	plain.setToDefault();
	plain.numVertices = 4;
	plain.points = plainPoints;
	plain.pointStrideBytes = sizeof(NxVec3);
	plain.numTriangles = 2;
	plain.triangles = plainIndices;
	plain.triangleStrideBytes = 3 * sizeof(NxU32);
	NxTriangleMesh* mesh = sdk->createTriangleMesh(plain);
	if(!mesh)
		return nxFail("triangle-mesh reload fixture creation failed");

	const NxVec3 tetraPoints[] = {
		NxVec3(-1.0f, -1.0f, -1.0f), NxVec3(1.0f, -1.0f, -1.0f),
		NxVec3(-1.0f, 1.0f, -1.0f), NxVec3(-1.0f, -1.0f, 1.0f)
		};
	const NxVec3 replacementPoints[] = {
		NxVec3(8.0f, 8.0f, 8.0f), NxVec3(10.0f, 8.0f, 8.0f),
		NxVec3(8.0f, 10.0f, 8.0f), NxVec3(8.0f, 8.0f, 10.0f)
		};
	const NxU32 tetraIndices[] = { 0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3 };
	NxTriangleMeshDesc computed;
	computed.setToDefault();
	computed.numVertices = 4;
	computed.points = tetraPoints;
	computed.pointStrideBytes = sizeof(NxVec3);
	computed.flags = NX_MF_CONVEX | NX_MF_COMPUTE_CONVEX;
	NxTriangleMeshDesc precomputed = computed;
	precomputed.points = replacementPoints;
	precomputed.numTriangles = 4;
	precomputed.triangles = tetraIndices;
	precomputed.triangleStrideBytes = 3 * sizeof(NxU32);
	precomputed.flags = NX_MF_CONVEX;

	bool ok = true;
	const NxTriangleMeshDesc* inputs[] = { &computed, &precomputed, &plain };
	const NxU32 expectedFlags[] = { NX_MF_CONVEX, NX_MF_CONVEX, NX_MF_CONVEX };
	const char* names[] = { "computed_convex", "precomputed_convex", "plain" };
	unsigned long long retainedHullHash = 0;
	for(unsigned i = 0; i < 3; ++i)
		{
		const bool loaded = mesh->loadFromDesc(*inputs[i]);
		NxTriangleMeshDesc saved;
		saved.setToDefault();
		const bool savedOk = mesh->saveToDesc(saved);
		const NxU32 hullVertices = mesh->getCount(0, NX_ARRAY_HULL_VERTICES);
		const NxU32 hullPolygons = mesh->getCount(0, NX_ARRAY_HULL_POLYGONS);
		const unsigned long long hullHash = nxMeshArrayHash(*mesh, NX_ARRAY_HULL_VERTICES);
		if(i == 0)
			retainedHullHash = hullHash;
		printf("triangle_mesh reload step=%s loaded=%u flags=%08x hull=%u.%u arrays=%016llx.%016llx hull_hash=%016llx\n",
			names[i], loaded ? 1u : 0u, saved.flags, hullVertices, hullPolygons,
			nxMeshArrayHash(*mesh, NX_ARRAY_VERTICES), nxMeshArrayHash(*mesh, NX_ARRAY_TRIANGLES), hullHash);
		if(!loaded || !savedOk || saved.flags != expectedFlags[i] ||
			hullVertices < 4 || hullPolygons < 4 ||
			(i != 0 && hullHash != retainedHullHash))
			ok = false;
		}
	sdk->releaseTriangleMesh(*mesh);
	return ok ? 0 : nxFail("triangle-mesh loadFromDesc replacement lifecycle diverged");
	}

static int nxTestTriangleMeshReleaseLifetime(NxPhysicsSDK* sdk)
	{
	const NxVec3 points[] = {
		NxVec3(-1.0f, -1.0f, 0.0f), NxVec3(1.0f, -1.0f, 0.0f),
		NxVec3(-1.0f, 1.0f, 0.0f), NxVec3(1.0f, 1.0f, 0.0f)
		};
	const NxU32 indices[] = { 0, 1, 2, 1, 3, 2 };
	NxTriangleMeshDesc desc;
	desc.setToDefault();
	desc.numVertices = 4;
	desc.points = points;
	desc.pointStrideBytes = sizeof(NxVec3);
	desc.numTriangles = 2;
	desc.triangles = indices;
	desc.triangleStrideBytes = 3 * sizeof(NxU32);
	const long long liveBefore = gAllocator.liveAllocations;
	NxTriangleMesh* mesh = sdk->createTriangleMesh(desc);
	if(!mesh)
		return nxFail("triangle-mesh destructor fixture creation failed");
	sdk->releaseTriangleMesh(*mesh);
	const long long liveAfterRelease = gAllocator.liveAllocations;
	printf("triangle_mesh lifecycle returned_to_baseline=%u\n",
		liveAfterRelease == liveBefore ? 1u : 0u);
	return liveAfterRelease == liveBefore ? 0 :
		nxFail("triangle-mesh release did not return allocator ownership to its starting level");
	}

static int nxTestInvalidDescriptor(NxPhysicsSDK* sdk)
	{
	NxTriangleMeshDesc invalid;
	invalid.setToDefault();
	gOutputStream.reset();
	NxTriangleMesh* mesh = sdk->createTriangleMesh(invalid);
	if(mesh)
		sdk->releaseTriangleMesh(*mesh);
	printf("triangle_mesh invalid_desc rejected=%u errors=%u code=%u line=%d file=%s message=%s\n",
		mesh ? 0u : 1u, gOutputStream.errors, static_cast<unsigned>(gOutputStream.lastCode),
		gOutputStream.lastLine, gOutputStream.file, gOutputStream.message);
	return !mesh && gOutputStream.errors == 1 &&
		gOutputStream.lastCode == NXE_INVALID_PARAMETER && gOutputStream.lastLine == 498 &&
		strcmp(gOutputStream.file, "\\Epic\\Novodex\\SDKs\\Physics\\src\\PhysicsSDK.cpp") == 0 &&
		strcmp(gOutputStream.message, "PhysicsSDK::createTriangleMesh: desc.isValid() is false!") == 0 ?
		0 : nxFail("invalid triangle-mesh descriptor did not report the oracle error");
	}

static float nxApiRange(unsigned& state, float low, float high)
	{
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	const float unit = static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
	return low + (high - low) * unit;
	}

static unsigned long long nxPointCloudHash(const NxVec3* points, NxU32 count)
	{
	const NxU8* bytes = reinterpret_cast<const NxU8*>(points);
	const size_t byteCount = static_cast<size_t>(count) * sizeof(*points);
	unsigned long long hash = 14695981039346656037ull;
	for(size_t i = 0; i < byteCount; ++i)
		{
		hash ^= bytes[i];
		hash *= 1099511628211ull;
		}
	return hash;
	}

static bool nxConvexMeshHasUsableTopology(const NxTriangleMesh& mesh,
	const NxVec3* points, NxU32 pointCount)
	{
	const NxU32 vertexCount = mesh.getCount(0, NX_ARRAY_VERTICES);
	const NxU32 triangleCount = mesh.getCount(0, NX_ARRAY_TRIANGLES);
	if(vertexCount < 4 || vertexCount > 256 || triangleCount < 4 ||
		mesh.getFormat(0, NX_ARRAY_VERTICES) != NX_FORMAT_FLOAT ||
		mesh.getFormat(0, NX_ARRAY_TRIANGLES) != NX_FORMAT_INT ||
		mesh.getStride(0, NX_ARRAY_VERTICES) != sizeof(NxVec3) ||
		mesh.getStride(0, NX_ARRAY_TRIANGLES) != 3 * sizeof(NxU32))
		return false;

	const NxVec3* vertices = static_cast<const NxVec3*>(mesh.getBase(0, NX_ARRAY_VERTICES));
	const NxU32* indices = static_cast<const NxU32*>(mesh.getBase(0, NX_ARRAY_TRIANGLES));
	if(!vertices || !indices)
		return false;

	double minimum[3] = { points[0].x, points[0].y, points[0].z };
	double maximum[3] = { points[0].x, points[0].y, points[0].z };
	for(NxU32 i = 1; i < pointCount; ++i)
		{
		const double value[3] = { points[i].x, points[i].y, points[i].z };
		for(int axis = 0; axis < 3; ++axis)
			{
			if(value[axis] < minimum[axis]) minimum[axis] = value[axis];
			if(value[axis] > maximum[axis]) maximum[axis] = value[axis];
			}
		}
	const double extent = maximum[0] - minimum[0] > maximum[1] - minimum[1] ?
		(maximum[0] - minimum[0] > maximum[2] - minimum[2] ? maximum[0] - minimum[0] : maximum[2] - minimum[2]) :
		(maximum[1] - minimum[1] > maximum[2] - minimum[2] ? maximum[1] - minimum[1] : maximum[2] - minimum[2]);
	const double tolerance = extent * 1.0e-5;
	bool referenced[256] = {};
	for(NxU32 triangle = 0; triangle < triangleCount; ++triangle)
		{
		const NxU32 ia = indices[triangle * 3 + 0];
		const NxU32 ib = indices[triangle * 3 + 1];
		const NxU32 ic = indices[triangle * 3 + 2];
		if(ia >= vertexCount || ib >= vertexCount || ic >= vertexCount)
			return false;
		referenced[ia] = referenced[ib] = referenced[ic] = true;
		const double abx = vertices[ib].x - vertices[ia].x;
		const double aby = vertices[ib].y - vertices[ia].y;
		const double abz = vertices[ib].z - vertices[ia].z;
		const double acx = vertices[ic].x - vertices[ia].x;
		const double acy = vertices[ic].y - vertices[ia].y;
		const double acz = vertices[ic].z - vertices[ia].z;
		const double nx = aby * acz - abz * acy;
		const double ny = abz * acx - abx * acz;
		const double nz = abx * acy - aby * acx;
		if(nx * nx + ny * ny + nz * nz <= tolerance * tolerance * tolerance * tolerance)
			return false;
		}
	for(NxU32 vertex = 0; vertex < vertexCount; ++vertex)
		{
		if(!referenced[vertex] || vertices[vertex].x < minimum[0] - tolerance ||
			vertices[vertex].x > maximum[0] + tolerance ||
			vertices[vertex].y < minimum[1] - tolerance ||
			vertices[vertex].y > maximum[1] + tolerance ||
			vertices[vertex].z < minimum[2] - tolerance ||
			vertices[vertex].z > maximum[2] + tolerance)
			return false;
		}
	return true;
	}

static int nxTestConvexCookingCloud(NxPhysicsSDK* sdk, const char* name,
	const NxVec3* points, NxU32 count)
	{
	NxTriangleMeshDesc desc;
	desc.setToDefault();
	desc.numVertices = count;
	desc.points = points;
	desc.pointStrideBytes = sizeof(NxVec3);
	desc.flags = NX_MF_CONVEX | NX_MF_COMPUTE_CONVEX;
	if(!desc.isValid())
		return nxFail("convex cooking point-cloud descriptor is invalid");

	const long long allocationsBefore = gAllocator.liveAllocations;
	gOutputStream.reset();
	NxTriangleMesh* mesh = sdk->createTriangleMesh(desc);
	if(!mesh)
		{
		printf("triangle_mesh convex_cook case=%s input=%016llx created=0 errors=%u code=%u line=%d\n",
			name, nxPointCloudHash(points, count), gOutputStream.errors,
			static_cast<unsigned>(gOutputStream.lastCode), gOutputStream.lastLine);
		return nxFail("convex cooking input cloud could not be cooked");
		}

	const NxU32 vertices = mesh->getCount(0, NX_ARRAY_VERTICES);
	const NxU32 triangles = mesh->getCount(0, NX_ARRAY_TRIANGLES);
	if(strcmp(name, "five_clusters") == 0)
		{
	// This 400-point input enters the oracle's 256-point reducer. Repeated
	// oracle processes return different reduced point selections and facet order
	// because qhull's facet hash incorporates heap addresses. Separate recorded
	// qhull sets and algorithm paths are covered by NxPhysicsThirdPartyTests; this
	// public route pins successful mesh construction, bounded output geometry,
	// usable triangle topology and release without treating allocator-dependent
	// topology as a deterministic byte contract.
	const bool validTopology = nxConvexMeshHasUsableTopology(*mesh, points, count);
	printf("triangle_mesh convex_cook case=%s input=%016llx created=1 topology=%u\n",
		name, nxPointCloudHash(points, count), validTopology ? 1u : 0u);
	if(!validTopology)
		{
		sdk->releaseTriangleMesh(*mesh);
		return nxFail("reduced convex cooking output has invalid geometry or triangle topology");
		}
		}
	else
		printf("triangle_mesh convex_cook case=%s input=%016llx created=1 vertices=%u triangles=%u arrays=%016llx.%016llx\n",
			name, nxPointCloudHash(points, count), vertices, triangles,
			nxMeshArrayHash(*mesh, NX_ARRAY_VERTICES), nxMeshArrayHash(*mesh, NX_ARRAY_TRIANGLES));
	sdk->releaseTriangleMesh(*mesh);
	if(gAllocator.liveAllocations != allocationsBefore)
		{
		printf("triangle_mesh convex_cook case=%s allocator_before=%lld after=%lld\n",
			name, allocationsBefore, gAllocator.liveAllocations);
		return nxFail("convex cooking point-cloud release leaked user allocations");
		}
	return 0;
	}

// Public-DLL coverage for duplicate welding and the reduced point-cloud path.
static int nxTestConvexCookingPointClouds(NxPhysicsSDK* sdk)
	{
	NxVec3 weldedPair[10];
	for(int i = 0; i < 10; ++i)
		{
		const float delta = (i / 2) * 1e-8f;
		weldedPair[i].set((i & 1) ? 2.0f + delta : -1.0f + delta,
			(i & 1) ? 1.0f - delta : 0.5f,
			(i & 1) ? -3.0f : 4.0f + delta);
		}
	int status = nxTestConvexCookingCloud(sdk, "welded_pair", weldedPair, 10);
	if(status) return status;

	const NxVec3 centers[] = {
		NxVec3(-4.0f, 0.0f, 1.0f), NxVec3(2.0f, -3.0f, 4.0f),
		NxVec3(3.0f, 2.0f, -2.0f), NxVec3(-1.0f, 5.0f, -3.0f),
		NxVec3(0.0f, -2.0f, -5.0f)
		};
	NxVec3 clusters[400];
	unsigned state = 0x4e0c0df1u;
	for(int i = 0; i < 400; ++i)
		{
		const int cluster = i % 5;
		clusters[i].set(centers[cluster].x + nxApiRange(state, -0.01f, 0.01f),
			centers[cluster].y + nxApiRange(state, -0.01f, 0.01f),
			centers[cluster].z + nxApiRange(state, -0.01f, 0.01f));
		}
	return nxTestConvexCookingCloud(sdk, "five_clusters", clusters, 400);
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	NxU32 density = NX_PMAP_COMPUTE_DENSITY;
	const bool isolatedPMap = argc == 3;
	if(argc != 2 && !isolatedPMap)
		return nxFail("usage: NxPhysicsTriangleMeshApiTests <absolute pair directory> [64|80|disconnected32]");
	if(isolatedPMap)
		{
		if(wcscmp(argv[2], L"disconnected32") != 0)
			{
			const unsigned long parsedDensity = wcstoul(argv[2], 0, 10);
			if(parsedDensity != 64 && parsedDensity != 80)
				return nxFail("isolated PMap mode accepts only density 64, density 80, or disconnected32");
			density = static_cast<NxU32>(parsedDensity);
			}
		}
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(2, argv, "NxPhysicsTriangleMeshApiTests",
		pairDirectory, &physics);
	if(status) return status;

	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &gAllocator, &gOutputStream);
	if(!sdk) return nxFail("SDK creation failed");
	const bool disconnectedPMap = isolatedPMap && wcscmp(argv[2], L"disconnected32") == 0;
	status = disconnectedPMap ? nxTestDisconnectedPMap(physics, sdk) :
		nxTestPMapComputeFirst(physics, sdk, density);
	if(status)
		{
		sdk->release();
		return status;
		}
	if(isolatedPMap)
		{
		sdk->release();
		return nxReportPairIdentity(pairDirectory);
		}

	status = nxTestDescriptorVariants(sdk);
	if(status)
		{
		sdk->release();
		return status;
		}
	status = nxTestTriangleMeshReload(sdk);
	if(status)
		{
		sdk->release();
		return status;
		}
	status = nxTestTriangleMeshReleaseLifetime(sdk);
	if(status)
		{
		sdk->release();
		return status;
		}
	status = nxTestInvalidDescriptor(sdk);
	if(status)
		{
		sdk->release();
		return status;
		}
	status = nxTestConvexCookingPointClouds(sdk);
	if(status)
		{
		sdk->release();
		return status;
		}

	const NxVec3 vertices[] = {
		NxVec3(-1.0f, -1.0f, -1.0f), NxVec3(1.0f, -1.0f, -1.0f),
		NxVec3(-1.0f, 1.0f, -1.0f), NxVec3(1.0f, 1.0f, -1.0f),
		NxVec3(-1.0f, -1.0f, 1.0f), NxVec3(1.0f, -1.0f, 1.0f),
		NxVec3(-1.0f, 1.0f, 1.0f), NxVec3(1.0f, 1.0f, 1.0f)
		};
	unsigned char pmapBytes[] = {
		0x50, 0x4d, 0x41, 0x50, 0x04, 0x00, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xc0
		};
	NxPMap pmap = { sizeof(pmapBytes), pmapBytes };
	NxTriangleMeshDesc desc;
	desc.setToDefault();
	desc.numVertices = sizeof(vertices) / sizeof(vertices[0]);
	desc.points = vertices;
	desc.pointStrideBytes = sizeof(NxVec3);
	desc.flags = NX_MF_CONVEX | NX_MF_COMPUTE_CONVEX;
	desc.pmap = &pmap;
	if(!desc.isValid())
		{
		sdk->release();
		return nxFail("convex point-cloud descriptor is invalid");
		}

	NxTriangleMesh* mesh = sdk->createTriangleMesh(desc);
	printf("triangle_mesh created=%u\n", mesh ? 1u : 0u);
	if(!mesh)
		{
		sdk->release();
		return nxFail("public convex triangle-mesh creation returned null");
		}
	const bool pmapFromDesc = mesh->hasPMap();
	printf("triangle_mesh pmap descriptor_has=%u\n", pmapFromDesc ? 1u : 0u);
	if(!pmapFromDesc)
		{
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("triangle-mesh descriptor PMap was not loaded");
		}
	const NxU32 verticesOut = mesh->getCount(0, NX_ARRAY_VERTICES);
	const NxU32 trianglesOut = mesh->getCount(0, NX_ARRAY_TRIANGLES);
	printf("triangle_mesh geometry=%u.%u\n", verticesOut, trianglesOut);
	printf("triangle_mesh arrays vertices=%016llx triangles=%016llx\n",
		nxMeshArrayHash(*mesh, NX_ARRAY_VERTICES),
		nxMeshArrayHash(*mesh, NX_ARRAY_TRIANGLES));
	if(getenv("NX_TRIANGLE_MESH_DUMP"))
		{
		const NxVec3* cookedVertices = static_cast<const NxVec3*>(
			mesh->getBase(0, NX_ARRAY_VERTICES));
		const NxU32* cookedTriangles = static_cast<const NxU32*>(
			mesh->getBase(0, NX_ARRAY_TRIANGLES));
		for(NxU32 i = 0; i < verticesOut; ++i)
			printf("triangle_mesh vertex=%u %08x.%08x.%08x\n", i,
				nxMeshFloatBits(cookedVertices[i].x), nxMeshFloatBits(cookedVertices[i].y),
				nxMeshFloatBits(cookedVertices[i].z));
		for(NxU32 i = 0; i < trianglesOut; ++i)
			printf("triangle_mesh triangle=%u %u.%u.%u\n", i,
				cookedTriangles[i * 3], cookedTriangles[i * 3 + 1], cookedTriangles[i * 3 + 2]);
		}
	// The public mesh wrapper also owns an optional, loadable penetration map.
	// This minimal serialized map exercises TriangleMesh::loadPMap's stream
	// route without depending on the still-unreconstructed map-compute export.
	const bool pmapLoaded = mesh->loadPMap(pmap);
	const bool pmapPresent = mesh->hasPMap();
	printf("triangle_mesh pmap valid_load=%u has=%u\n", pmapLoaded ? 1u : 0u,
		pmapPresent ? 1u : 0u);
	const NxU32 pmapSize = mesh->getPMapSize();
	unsigned char exportedPmapBytes[24] = {};
	NxPMap exportedPmap = { sizeof(exportedPmapBytes), exportedPmapBytes };
	const bool pmapExported = mesh->getPMapData(exportedPmap);
	static const unsigned char expectedPmapBytes[] = {
		0x50, 0x4d, 0x41, 0x50, 0x04, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff,
		0x80, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff
		};
	printf("triangle_mesh pmap export size=%u success=%u bytes=", pmapSize,
		pmapExported ? 1u : 0u);
	for(unsigned i = 0; i < sizeof(exportedPmapBytes); ++i)
		printf("%02x", exportedPmapBytes[i]);
	printf("\n");
	if(pmapSize != sizeof(exportedPmapBytes) || !pmapExported ||
		memcmp(exportedPmapBytes, expectedPmapBytes, sizeof(expectedPmapBytes)) != 0)
		{
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("triangle-mesh PMap serialization disagrees with the oracle");
		}
	NxPMap undersizedExport = { pmapSize - 1, exportedPmapBytes };
	NxPMap oversizedExport = { pmapSize + 1, exportedPmapBytes };
	if(mesh->getPMapData(undersizedExport) || mesh->getPMapData(oversizedExport))
		{
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("PMap export accepted a buffer size different from getPMapSize");
		}

	// A four-cell value group at resolution 32 covers several neighbor steps and
	// an absolute-coordinate escape in the cell-run decoder and Morton serializer.
	// All untouched cells retain the serialized empty marker, while their sign
	// plane is one.
	unsigned char cellRunPmapBytes[12 + 4200] = {};
	static const unsigned char pmapHeader[] = {
		0x50, 0x4d, 0x41, 0x50, 0x04, 0x00, 0x00, 0x00,
		0x20, 0x00, 0x00, 0x00
		};
	memcpy(cellRunPmapBytes, pmapHeader, sizeof(pmapHeader));
	unsigned cellRunBitOffset = 12 * 8;
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 0, 1); // absolute value follows
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 1, 32); // value id
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 4, 32); // four cells in this group
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 19, 5); // (+1,+1,+1) from (-1,-1,-1)
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 1, 5); // x increases by one
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 8, 5); // x decreases as y increases
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 26, 5); // absolute x coordinate follows
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 31, 5); // x = 31, y = 1, z = 0
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 0, 1); // absolute terminator follows
	nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 0xffffffffu, 32);
	for(unsigned cell = 0; cell < 32u * 32u * 32u; ++cell)
		nxPMapStoreBits(cellRunPmapBytes, cellRunBitOffset, 1, 1);
	const NxU32 cellRunInputSize = 12 + (cellRunBitOffset - 12 * 8 + 7) / 8;
	NxPMap cellRunPmap = { cellRunInputSize, cellRunPmapBytes };
	const bool cellRunLoaded = mesh->loadPMap(cellRunPmap);
	const NxU32 cellRunOutputSize = mesh->getPMapSize();
	unsigned char cellRunOutput[8192] = {};
	NxPMap cellRunOutputPmap = { cellRunOutputSize, cellRunOutput };
	const bool cellRunExported = mesh->getPMapData(cellRunOutputPmap);
	printf("triangle_mesh pmap cell_run load=%u export=%u size=%u hash=%016llx\n",
		cellRunLoaded ? 1u : 0u, cellRunExported ? 1u : 0u, cellRunOutputSize,
		cellRunExported ? nxPMapByteHash(cellRunOutput, cellRunOutputSize) : 0ull);
	if(!cellRunLoaded || !cellRunExported || cellRunOutputSize <= pmapSize ||
		cellRunOutputSize != 4124 || cellRunOutputSize > sizeof(cellRunOutput) ||
		nxPMapByteHash(cellRunOutput, cellRunOutputSize) != 0x575faca3773bc417ull)
		{
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("non-empty PMap cell run diverged from the oracle");
		}
	unsigned char badPmapBytes[sizeof(pmapBytes)];
	memcpy(badPmapBytes, pmapBytes, sizeof(pmapBytes));
	badPmapBytes[0] = 0x58;	// Invalid magic; the oracle drops the previous map on this failed reload.
	NxPMap badPmap = { sizeof(badPmapBytes), badPmapBytes };
	const bool badPmapLoaded = mesh->loadPMap(badPmap);
	const bool badPmapPresent = mesh->hasPMap();
	const NxU32 sizeAfterBadLoad = mesh->getPMapSize();
	NxPMap dataAfterBadLoad = { 0, exportedPmapBytes };
	const bool dataAfterBadLoadResult = mesh->getPMapData(dataAfterBadLoad);
	printf("triangle_mesh pmap invalid_load=%u has=%u size=%u export=%u\n",
		badPmapLoaded ? 1u : 0u, badPmapPresent ? 1u : 0u,
		sizeAfterBadLoad, dataAfterBadLoadResult ? 1u : 0u);
	if(!pmapLoaded || !pmapPresent || badPmapLoaded || badPmapPresent ||
		sizeAfterBadLoad != 0 || dataAfterBadLoadResult)
		{
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("triangle-mesh PMap load/reject lifecycle disagrees with the oracle");
		}
	if(verticesOut < 4 || trianglesOut < 4)
		{
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("convex mesh did not expose cooked geometry");
		}
	NxTriangleMeshDesc roundTrip;
	roundTrip.setToDefault();
	if(!mesh->saveToDesc(roundTrip) || roundTrip.numVertices != verticesOut ||
		roundTrip.numTriangles != trianglesOut)
		{
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("cooked mesh descriptor round-trip disagrees with geometry");
		}

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		{
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("scene creation failed before triangle-mesh actor test");
		}
	NxPlaneShapeDesc planeDesc;
	planeDesc.normal = NxVec3(0.0f, 1.0f, 0.0f);
	planeDesc.d = 0.0f;
	NxActorDesc groundDesc;
	groundDesc.shapes.pushBack(&planeDesc);
	NxActor* ground = scene->createActor(groundDesc);
	if(!ground)
		{
		sdk->releaseScene(*scene);
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("ground plane creation failed");
		}
	NxTriangleMeshShapeDesc shapeDesc;
	shapeDesc.meshData = mesh;
	NxBodyDesc bodyDesc;
	NxActorDesc actorDesc;
	actorDesc.body = &bodyDesc;
	actorDesc.density = 1.0f;
	actorDesc.globalPose.t = NxVec3(0.0f, 3.0f, 0.0f);
	actorDesc.shapes.pushBack(&shapeDesc);
	NxActor* actor = scene->createActor(actorDesc);
	printf("triangle_mesh actor=%u shapes=%u\n", actor ? 1u : 0u,
		actor ? actor->getNbShapes() : 0u);
	if(!actor || actor->getNbShapes() != 1)
		{
		sdk->releaseScene(*scene);
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("triangle-mesh actor creation failed");
		}
	// phys_fn_002258 refuses to release a mesh while any shape still owns it.
	// Check before simulation so a broken implementation fails without using a
	// dangling wrapper in subsequent actor work.
	gOutputStream.reset();
	sdk->releaseTriangleMesh(*mesh);
	const bool releaseRefused = gOutputStream.errors == 1 &&
		gOutputStream.lastCode == NXE_INVALID_OPERATION && gOutputStream.lastLine == 173 &&
		strcmp(gOutputStream.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\TriangleMesh.cpp") == 0 &&
		strcmp(gOutputStream.message,
			"TriangleMesh::release: instances of this mesh still exist!") == 0;
	printf("triangle_mesh release_in_use refused=%u errors=%u code=%u line=%d\n",
		releaseRefused ? 1u : 0u, gOutputStream.errors,
		static_cast<unsigned>(gOutputStream.lastCode), gOutputStream.lastLine);
	if(!releaseRefused)
		return nxFail("triangle-mesh release did not refuse a live shape reference");
	gOutputStream.reset();
	NxVec3 inertia = actor->getMassSpaceInertiaTensor();
	printf("triangle_mesh mass=%08x inertia=%08x.%08x.%08x\n",
		nxMeshFloatBits(actor->getMass()), nxMeshFloatBits(inertia.x),
		nxMeshFloatBits(inertia.y), nxMeshFloatBits(inertia.z));
	bool fetched = true;
	for(unsigned step = 0; step < 120 && fetched; ++step)
		{
		scene->simulate(1.0f / 60.0f);
		fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(getenv("NX_TRIANGLE_MESH_TRACE"))
			{
			const NxVec3 tracePosition = actor->getGlobalPosition();
			NxVec3 traceVelocity;
			actor->getLinearVelocity(traceVelocity);
			printf("triangle_mesh trace=%u position=%08x.%08x.%08x velocity=%08x.%08x.%08x\n",
				step + 1, nxMeshFloatBits(tracePosition.x), nxMeshFloatBits(tracePosition.y),
				nxMeshFloatBits(tracePosition.z), nxMeshFloatBits(traceVelocity.x),
				nxMeshFloatBits(traceVelocity.y), nxMeshFloatBits(traceVelocity.z));
			}
		}
	NxVec3 position = actor->getGlobalPosition();
	NxVec3 velocity;
	actor->getLinearVelocity(velocity);
	printf("triangle_mesh settle=%08x.%08x.%08x velocity=%08x.%08x.%08x fetched=%u\n",
		nxMeshFloatBits(position.x), nxMeshFloatBits(position.y), nxMeshFloatBits(position.z),
		nxMeshFloatBits(velocity.x), nxMeshFloatBits(velocity.y), nxMeshFloatBits(velocity.z),
		fetched ? 1u : 0u);
	if(!fetched)
		{
		scene->releaseActor(*actor);
		scene->releaseActor(*ground);
		sdk->releaseScene(*scene);
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("triangle-mesh actor simulation did not fetch");
		}
	// Oracle baseline for this deterministic 120-step convex-mesh/plane case:
	// the 2-unit cube rests at y=0x3f73332a with zero linear velocity.
	// Checking the actual state makes this gate catch missing mesh-plane contact
	// dispatch instead of treating any successful fetch as a passing simulation.
	// Current independently staged oracle and candidate runs match this state
	// exactly, so keep the full float-bit result in the differential contract.
	const bool settledOnPlane = nxMeshFloatBits(position.x) == 0 &&
		nxMeshFloatBits(position.y) == 0x3f73332a &&
		nxMeshFloatBits(position.z) == 0 && nxMeshFloatBits(velocity.x) == 0 &&
		nxMeshFloatBits(velocity.y) == 0 && nxMeshFloatBits(velocity.z) == 0;
	if(!settledOnPlane)
		{
		scene->releaseActor(*actor);
		scene->releaseActor(*ground);
		sdk->releaseScene(*scene);
		sdk->releaseTriangleMesh(*mesh);
		sdk->release();
		return nxFail("convex triangle mesh did not settle on the plane like the oracle");
		}
	scene->releaseActor(*actor);
	scene->releaseActor(*ground);
	sdk->releaseScene(*scene);

	sdk->releaseTriangleMesh(*mesh);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}
