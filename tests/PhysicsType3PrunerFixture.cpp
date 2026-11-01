#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "NxSdkAllocator.h"
#include "PhysicsInternal.h"
#include "IcePruner.h"

unsigned nx2iFoundationBegin();
void nx2iFoundationEnd();

class NxType3TestAllocator : public NxUserAllocator
{
public:
	void* mallocDEBUG(size_t size, const char*, int) override { return ::malloc(size); }
	void* malloc(size_t size) override { return ::malloc(size); }
	void* realloc(void* memory, size_t size) override { return ::realloc(memory, size); }
	void free(void* memory) override { ::free(memory); }
};

class NxType3SdkAllocator : public SdkAllocator
{
public:
	explicit NxType3SdkAllocator(NxUserAllocator* value) : allocator(value) {}
	void* malloc(size_t size, NxMemoryType type) override { return allocator->malloc(size, type); }
	void* mallocDEBUG(size_t size, const char* file, int line, const char* name, NxMemoryType type) override
		{ return allocator->mallocDEBUG(size, file, line, name, type); }
	void* realloc(void* memory, size_t size) override { return allocator->realloc(memory, size); }
	void free(void* memory) override { allocator->free(memory); }
	NxUserAllocator* allocator;
};

struct NxType3TestOwner
{
	float worldBounds[6];
	unsigned updateCalls;
};

// Row 004855 is the Scene pruning engine's initial-capacity adapter. Keep its
// oracle differential beside the existing private pruner fixture so both
// sides use independently constructed pruners and the same allocator bridge.
unsigned nxDrivePrunerPoolReserve(unsigned char* base)
{
	if(!nx2iFoundationBegin())
		return 1;
	NxType3TestAllocator allocator;
	NxUserAllocator* const savedFoundationAllocator = nxFoundationSDKAllocator;
	nxFoundationSDKAllocator = &allocator;
	SdkAllocator* const savedCandidateSdk = nxGetSdkAllocator();
	SdkAllocatorBridge candidateBridge;
	candidateBridge.mAllocator = &allocator;
	nxSetSdkAllocatorBridge(&candidateBridge);
	typedef bool (__cdecl* OracleSetAllocatorFn)(void*);
	const OracleSetAllocatorFn oracleSetAllocator =
		reinterpret_cast<OracleSetAllocatorFn>(base + 0x000b4020);
	SdkAllocator* const savedOracleSdk =
		*reinterpret_cast<SdkAllocator**>(base + 0x0012845c);
	NxType3SdkAllocator oracleBridge(&allocator);
	if(!oracleSetAllocator(&oracleBridge))
		return 1;
	typedef void* (__stdcall* OracleFactoryFn)(unsigned);
	const OracleFactoryFn oracleFactory =
		reinterpret_cast<OracleFactoryFn>(base + 0x000b5090);
	typedef unsigned char (__thiscall* OracleReserveFn)(void*, unsigned, unsigned, unsigned);
	const OracleReserveFn oracleReserve =
		reinterpret_cast<OracleReserveFn>(base + 0x000b5140);
	unsigned mismatches = 0;
	for(unsigned mode = 0; mode != 4; ++mode)
		{
		NxU8 oracleEngine[0x2c] = {};
		NxU8 candidateEngine[0x2c] = {};
		Pruner* candidatePruners[4] = {};
		bool setupOkay = true;
		for(unsigned type = 0; type != 4; ++type)
			{
			void* const oraclePruner = oracleFactory(type);
			candidatePruners[type] = nxPruningEngineCreatePruner(type);
			*reinterpret_cast<void**>(oracleEngine + 0x1c + type*4) = oraclePruner;
			*reinterpret_cast<void**>(candidateEngine + 0x1c + type*4) = candidatePruners[type];
			setupOkay = setupOkay && oraclePruner && candidatePruners[type];
			}
		if(!setupOkay)
			{
			++mismatches;
			for(unsigned type = 0; type != 4; ++type)
				{
				void* const oraclePruner = *reinterpret_cast<void**>(oracleEngine + 0x1c + type*4);
				if(oraclePruner)
					{
					void** const table = *reinterpret_cast<void***>(oraclePruner);
					typedef void* (__thiscall* DeleteFn)(void*, unsigned);
					reinterpret_cast<DeleteFn>(table[0])(oraclePruner, 1);
					}
				if(candidatePruners[type]) nxPruningEngineDestroyPruner(candidatePruners[type]);
				}
			continue;
		}
		const unsigned char oracleZero = oracleReserve(&oracleEngine, 0, 0, mode);
		const unsigned char oracleFirst = oracleReserve(&oracleEngine, 8, 16, mode);
		const unsigned char candidateFirst = nxSceneEngineReservePoolsCandidate(
			&candidateEngine, 8, 16, mode);
		const unsigned char oracleSecond = oracleReserve(&oracleEngine, 3, 5, mode);
		const unsigned char candidateSecond = nxSceneEngineReservePoolsCandidate(
			&candidateEngine, 3, 5, mode);
		bool capacitiesMatch = oracleFirst == candidateFirst && oracleSecond == candidateSecond;
		for(unsigned type = 0; type != 4; ++type)
			{
			const unsigned oracleCapacity = *reinterpret_cast<unsigned short*>(
				static_cast<unsigned char*>(*reinterpret_cast<void**>(oracleEngine + 0x1c + type*4)) + 0x12);
			const unsigned candidateCapacity = *reinterpret_cast<unsigned short*>(
				reinterpret_cast<unsigned char*>(candidatePruners[type]) + 0x12);
			const unsigned expected = type == 0 ? (mode == 0 ? 5u : 3u) :
				(type == mode ? 5u : 0u);
			capacitiesMatch = capacitiesMatch && oracleCapacity == candidateCapacity &&
				candidateCapacity == expected;
			}
		if(!capacitiesMatch)
			++mismatches;
		mismatches += !oracleZero;
		for(unsigned type = 0; type != 4; ++type)
			{
			void* const oraclePruner = *reinterpret_cast<void**>(oracleEngine + 0x1c + type*4);
			void** const table = *reinterpret_cast<void***>(oraclePruner);
			typedef void* (__thiscall* DeleteFn)(void*, unsigned);
			reinterpret_cast<DeleteFn>(table[0])(oraclePruner, 1);
			nxPruningEngineDestroyPruner(candidatePruners[type]);
			}
		}
	oracleSetAllocator(savedOracleSdk);
	nxSetSdkAllocatorBridge(savedCandidateSdk);
	nxFoundationSDKAllocator = savedFoundationAllocator;
	nx2iFoundationEnd();
	return mismatches;
}

static void __cdecl nxType3UpdateWorldAabb(void* owner, AABB* bounds)
{
	NxType3TestOwner* const testOwner = static_cast<NxType3TestOwner*>(owner);
	memcpy(bounds, testOwner->worldBounds, sizeof(testOwner->worldBounds));
	++testOwner->updateCalls;
}

// Compare the private factory product and the moved-leaf AABB query with the
// pinned image. The oracle is passed in by the collision harness after its
// SHA-256 check; candidate implementation symbols are linked into that image.
unsigned nxDriveType3PrunerRefit(unsigned char* base)
{
	if(!nx2iFoundationBegin())
		return 1;
	NxType3TestAllocator allocator;
	NxUserAllocator* const savedFoundationAllocator = nxFoundationSDKAllocator;
	nxFoundationSDKAllocator = &allocator;
	SdkAllocator* const savedCandidateSdk = nxGetSdkAllocator();
	SdkAllocatorBridge candidateBridge;
	candidateBridge.mAllocator = &allocator;
	nxSetSdkAllocatorBridge(&candidateBridge);
	typedef bool (__cdecl* OracleSetAllocatorFn)(void*);
	const OracleSetAllocatorFn oracleSetAllocator =
		reinterpret_cast<OracleSetAllocatorFn>(base + 0x000b4020);
	SdkAllocator* const savedOracleSdk =
		*reinterpret_cast<SdkAllocator**>(base + 0x0012845c);
	NxType3SdkAllocator oracleBridge(&allocator);
	if(!oracleSetAllocator(&oracleBridge))
		return 1;
	typedef void* (__cdecl* OracleFactoryFn)(unsigned);
	const OracleFactoryFn oracleFactory =
		reinterpret_cast<OracleFactoryFn>(base + 0x000b5090);
	void* const oraclePruner = oracleFactory(3);
	Pruner* const candidatePruner = nxPruningEngineCreatePruner(3);
	if(!oraclePruner || !candidatePruner)
		return 1;

	NxU8 oracleObjects[3][0x2c] = {};
	Prunable candidateObjects[3];
	NxType3TestOwner oracleOwners[3] = {};
	NxType3TestOwner candidateOwners[3] = {};
	typedef void (__thiscall* OracleCtorFn)(void*);
	const OracleCtorFn oraclePrunableCtor =
		reinterpret_cast<OracleCtorFn>(base + 0x000b54a0);
	for(unsigned i = 0; i != 3; ++i)
		{
		oraclePrunableCtor(oracleObjects[i]);
		*reinterpret_cast<void**>(oracleObjects[i] + 4) = &oracleOwners[i];
		*reinterpret_cast<NxU32*>(oracleObjects[i] + 8) = PRUNABLE_FLAG_WORLD_AABB_VALID;
		*reinterpret_cast<NxU32*>(oracleObjects[i] + 0x24) = 0xffffffffu;
		candidateObjects[i].mOwner = &candidateOwners[i];
		candidateObjects[i].mFlags = PRUNABLE_FLAG_WORLD_AABB_VALID;
		candidateObjects[i].mPrunable24 = 0xffffffffu;
		}
	void (* const savedCandidateOwnerAabb)(void*, AABB*) = gPrunableOwnerWorldAABB;
	void (** const oracleOwnerAabbSlot)(void*, AABB*) =
		reinterpret_cast<void (**)(void*, AABB*)>(base + 0x00128478);
	void (* const savedOracleOwnerAabb)(void*, AABB*) = *oracleOwnerAabbSlot;
	gPrunableOwnerWorldAABB = nxType3UpdateWorldAabb;
	*oracleOwnerAabbSlot = nxType3UpdateWorldAabb;

	typedef bool (__thiscall* AddObjectFn)(void*, void*);
	typedef bool (__thiscall* RemoveObjectFn)(void*, void*);
	void** const oracleVtable = *reinterpret_cast<void***>(oraclePruner);
	const AddObjectFn oracleAdd = reinterpret_cast<AddObjectFn>(oracleVtable[1]);
	const RemoveObjectFn oracleRemove = reinterpret_cast<RemoveObjectFn>(oracleVtable[2]);
	unsigned mismatches = 0;
	for(unsigned i = 0; i != 2; ++i)
		if(!oracleAdd(oraclePruner, oracleObjects[i]) ||
			!candidatePruner->AddObject(&candidateObjects[i]))
			++mismatches;

	// Pool AABBs are {min xyz, max xyz}. Both boxes stay under the original
	// root after the first query builds the tree.
	const float initialBoxes[2][6] = {
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f},
		{10.0f, 0.0f, 0.0f, 11.0f, 1.0f, 1.0f}};
	for(unsigned side = 0; side != 2; ++side)
		{
		void* const pruner = side ? static_cast<void*>(candidatePruner) : oraclePruner;
		float* const poolBoxes = *reinterpret_cast<float**>(
			static_cast<unsigned char*>(pruner) + 0x14);
		if(!poolBoxes)
			++mismatches;
		else
			memcpy(poolBoxes, initialBoxes, sizeof(initialBoxes));
		}

	struct TestOutput
		{
		NxU32 capacity;
		NxU32 count;
		NxU32* entries;
		float growthFactor;
		};
	NxU32 oracleEntries[8] = {}, candidateEntries[8] = {};
	TestOutput oracleOutput = {8, 0, oracleEntries, 2.0f};
	TestOutput candidateOutput = {8, 0, candidateEntries, 2.0f};
	const float query[6] = {3.0f, -1.0f, -1.0f, 6.0f, 2.0f, 2.0f};
	typedef bool (__thiscall* AabbQueryFn)(void*, NxU32, NxU32, NxU32, NxU32);
	const AabbQueryFn oracleQuery = reinterpret_cast<AabbQueryFn>(oracleVtable[7]);
	const bool oracleInitial = oracleQuery(oraclePruner,
		reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(query), 0, 0xffffffffu);
	const bool candidateInitial = candidatePruner->NovodeXPrunerSlot7(
		reinterpret_cast<NxU32>(&candidateOutput), reinterpret_cast<NxU32>(query), 0, 0xffffffffu);
	if(oracleInitial != candidateInitial || oracleOutput.count != 0 || candidateOutput.count != 0)
		++mismatches;
	const float addedBounds[6] = {20.0f, 0.0f, 0.0f, 21.0f, 1.0f, 1.0f};
	memcpy(oracleOwners[2].worldBounds, addedBounds, sizeof(addedBounds));
	memcpy(candidateOwners[2].worldBounds, addedBounds, sizeof(addedBounds));
	*reinterpret_cast<NxU32*>(oracleObjects[2] + 8) = PRUNABLE_FLAG_WORLD_AABB_VALID;
	candidateObjects[2].mFlags = PRUNABLE_FLAG_WORLD_AABB_VALID;
	const bool oracleAddedAfterBuild = oracleAdd(oraclePruner, oracleObjects[2]);
	const bool candidateAddedAfterBuild = candidatePruner->AddObject(&candidateObjects[2]);
	for(unsigned side = 0; side != 2; ++side)
		{
		void* const pruner = side ? static_cast<void*>(candidatePruner) : oraclePruner;
		unsigned char* const object = side ?
			static_cast<unsigned char*>(static_cast<void*>(&candidateObjects[2])) : oracleObjects[2];
		float* const poolBoxes = *reinterpret_cast<float**>(
			static_cast<unsigned char*>(pruner) + 0x14);
		const unsigned handle = *reinterpret_cast<NxU16*>(object + 0x28);
		if(poolBoxes && handle < 3)
			memcpy(poolBoxes + handle * 6, addedBounds, sizeof(addedBounds));
		else
			++mismatches;
		}
	oracleOutput.count = 0;
	candidateOutput.count = 0;
	const bool oracleAddedQuery = oracleQuery(oraclePruner,
		reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(addedBounds),
		0, 0xffffffffu);
	const bool candidateAddedQuery = candidatePruner->NovodeXPrunerSlot7(
		reinterpret_cast<NxU32>(&candidateOutput), reinterpret_cast<NxU32>(addedBounds),
		0, 0xffffffffu);
	const NxU32 oracleAddedHits = oracleOutput.count;
	const NxU32 candidateAddedHits = candidateOutput.count;
	bool oracleAddedFound = false, candidateAddedFound = false;
	for(NxU32 i = 0; i < oracleOutput.count && i < 8; ++i)
		oracleAddedFound = oracleAddedFound || oracleEntries[i] == reinterpret_cast<NxU32>(oracleObjects[2]);
	for(NxU32 i = 0; i < candidateOutput.count && i < 8; ++i)
		candidateAddedFound = candidateAddedFound || candidateEntries[i] == reinterpret_cast<NxU32>(&candidateObjects[2]);
	if(!oracleAddedAfterBuild || !candidateAddedAfterBuild || !oracleAddedQuery ||
		!candidateAddedQuery || oracleOutput.count != 1 || candidateOutput.count != 1 ||
		!oracleAddedFound || !candidateAddedFound)
		++mismatches;

	memcpy(oracleOwners[0].worldBounds, query, sizeof(query));
	memcpy(candidateOwners[0].worldBounds, query, sizeof(query));
	*reinterpret_cast<NxU32*>(oracleObjects[0] + 8) &= ~PRUNABLE_FLAG_WORLD_AABB_VALID;
	candidateObjects[0].mFlags &= ~PRUNABLE_FLAG_WORLD_AABB_VALID;
	typedef bool (__thiscall* UpdateObjectFn)(void*, void*);
	const UpdateObjectFn oracleUpdate = reinterpret_cast<UpdateObjectFn>(oracleVtable[3]);
	const bool oracleUpdated = oracleUpdate(oraclePruner, oracleObjects[0]);
	const bool candidateUpdated = candidatePruner->UpdateObject(&candidateObjects[0]);
	if(!oracleUpdated || !candidateUpdated || oracleOwners[0].updateCalls != 1 ||
		candidateOwners[0].updateCalls != 1 ||
		!(*reinterpret_cast<NxU32*>(oracleObjects[0] + 8) & PRUNABLE_FLAG_WORLD_AABB_VALID) ||
		!(candidateObjects[0].mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
		++mismatches;
	oracleOutput.count = 0;
	candidateOutput.count = 0;
	const bool oracleMoved = oracleQuery(oraclePruner,
		reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(query), 0, 0xffffffffu);
	const bool candidateMoved = candidatePruner->NovodeXPrunerSlot7(
		reinterpret_cast<NxU32>(&candidateOutput), reinterpret_cast<NxU32>(query), 0, 0xffffffffu);
	const NxU32 oracleHits = oracleOutput.count;
	const NxU32 candidateHits = candidateOutput.count;
	bool oracleFound = false, candidateFound = false;
	for(NxU32 i = 0; i < oracleHits && i < 8; ++i)
		oracleFound = oracleFound || oracleEntries[i] == reinterpret_cast<NxU32>(oracleObjects[0]);
	for(NxU32 i = 0; i < candidateHits && i < 8; ++i)
		candidateFound = candidateFound || candidateEntries[i] == reinterpret_cast<NxU32>(&candidateObjects[0]);
	if(!oracleMoved || !candidateMoved || oracleHits != 1 || candidateHits != 1 ||
		!oracleFound || !candidateFound)
		++mismatches;
	const float sphere[4] = {4.5f, 0.5f, 0.5f, 0.4f};
	oracleOutput.count = 0;
	candidateOutput.count = 0;
	typedef bool (__thiscall* SphereQueryFn)(void*, NxU32, NxU32, NxU32, NxU32);
	const SphereQueryFn oracleSphereQuery =
		reinterpret_cast<SphereQueryFn>(oracleVtable[8]);
	const bool oracleSphere = oracleSphereQuery(oraclePruner,
		reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(sphere),
		0, 0xffffffffu);
	const bool candidateSphere = candidatePruner->NovodeXPrunerSlot8(
		reinterpret_cast<NxU32>(&candidateOutput), reinterpret_cast<NxU32>(sphere),
		0, 0xffffffffu);
	const NxU32 oracleSphereHits = oracleOutput.count;
	const NxU32 candidateSphereHits = candidateOutput.count;
	bool oracleSphereFound = false, candidateSphereFound = false;
	for(NxU32 i = 0; i < oracleOutput.count && i < 8; ++i)
		oracleSphereFound = oracleSphereFound || oracleEntries[i] == reinterpret_cast<NxU32>(oracleObjects[0]);
	for(NxU32 i = 0; i < candidateOutput.count && i < 8; ++i)
		candidateSphereFound = candidateSphereFound || candidateEntries[i] == reinterpret_cast<NxU32>(&candidateObjects[0]);
	if(!oracleSphere || !candidateSphere || oracleSphereHits != 1 ||
		candidateSphereHits != 1 || !oracleSphereFound || !candidateSphereFound)
		++mismatches;
	const float planes[8] = {1.0f, 0.0f, 0.0f, -3.0f,
		-1.0f, 0.0f, 0.0f, 6.0f};
	oracleOutput.count = 0;
	candidateOutput.count = 0;
	typedef bool (__thiscall* PlaneQueryFn)(void*, NxU32, NxU32, NxU32, NxU32, NxU32);
	const PlaneQueryFn oraclePlaneQuery =
		reinterpret_cast<PlaneQueryFn>(oracleVtable[5]);
	const bool oraclePlane = oraclePlaneQuery(oraclePruner,
		reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(planes),
		2, 0, 0xffffffffu);
	const bool candidatePlane = candidatePruner->NovodeXPrunerSlot5(
		reinterpret_cast<NxU32>(&candidateOutput), reinterpret_cast<NxU32>(planes),
		2, 0, 0xffffffffu);
	const NxU32 oraclePlaneHits = oracleOutput.count;
	const NxU32 candidatePlaneHits = candidateOutput.count;
	bool oraclePlaneFound = false, candidatePlaneFound = false;
	for(NxU32 i = 0; i < oracleOutput.count && i < 8; ++i)
		oraclePlaneFound = oraclePlaneFound || oracleEntries[i] == reinterpret_cast<NxU32>(oracleObjects[0]);
	for(NxU32 i = 0; i < candidateOutput.count && i < 8; ++i)
		candidatePlaneFound = candidatePlaneFound || candidateEntries[i] == reinterpret_cast<NxU32>(&candidateObjects[0]);
	if(!oraclePlane || !candidatePlane || oraclePlaneHits != 1 ||
		candidatePlaneHits != 1 || !oraclePlaneFound || !candidatePlaneFound)
		++mismatches;
	const Ray worldRay(Point(0.0f, 0.5f, 0.5f), Point(1.0f, 0.0f, 0.0f));
	Container oracleRayHits;
	Container candidateRayHits;
	typedef bool (__thiscall* RaycastFn)(void*, Container&, const Ray&, float, bool, NxU32);
	const RaycastFn oracleRaycast = reinterpret_cast<RaycastFn>(oracleVtable[6]);
	const bool oracleRay = oracleRaycast(oraclePruner, oracleRayHits, worldRay,
		8.0f, true, 0xffffffffu);
	const bool candidateRay = candidatePruner->Raycast(candidateRayHits, worldRay,
		8.0f, true, 0xffffffffu);
	bool oracleRayFound = false, candidateRayFound = false;
	for(NxU32 i = 0; i < oracleRayHits.GetNbEntries(); ++i)
		oracleRayFound = oracleRayFound || oracleRayHits.GetEntry(i) == reinterpret_cast<NxU32>(oracleObjects[0]);
	for(NxU32 i = 0; i < candidateRayHits.GetNbEntries(); ++i)
		candidateRayFound = candidateRayFound || candidateRayHits.GetEntry(i) == reinterpret_cast<NxU32>(&candidateObjects[0]);
	if(!oracleRay || !candidateRay || oracleRayHits.GetNbEntries() != 1 ||
		candidateRayHits.GetNbEntries() != 1 || !oracleRayFound || !candidateRayFound)
		++mismatches;
	const bool oracleRemoved = oracleRemove(oraclePruner, oracleObjects[1]);
	const bool candidateRemoved = candidatePruner->RemoveObject(&candidateObjects[1]);
	const float removedBounds[6] = {10.0f, 0.0f, 0.0f, 11.0f, 1.0f, 1.0f};
	oracleOutput.count = 0;
	candidateOutput.count = 0;
	const bool oracleAfterRemove = oracleQuery(oraclePruner,
		reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(removedBounds),
		0, 0xffffffffu);
	const bool candidateAfterRemove = candidatePruner->NovodeXPrunerSlot7(
		reinterpret_cast<NxU32>(&candidateOutput), reinterpret_cast<NxU32>(removedBounds),
		0, 0xffffffffu);
	if(!oracleRemoved || !candidateRemoved || !oracleAfterRemove || !candidateAfterRemove ||
		oracleOutput.count != 0 || candidateOutput.count != 0 ||
		*reinterpret_cast<NxU16*>(oracleObjects[1] + 0x28) != PRUNABLE_INVALID_HANDLE ||
		candidateObjects[1].mHandle != PRUNABLE_INVALID_HANDLE)
		++mismatches;
	printf("collision type3_pruner refit objects=2 oracle_found=%u candidate_found=%u oracle_hits=%u candidate_hits=%u mismatches=%u\n",
		oracleFound ? 1u : 0u, candidateFound ? 1u : 0u, oracleHits, candidateHits, mismatches);
	printf("collision coverage name=type3_pruner case=tree_refit_moved_leaf\n");
	printf("collision coverage name=type3_pruner case=update_object_refresh_invalid_world_box oracle_calls=%u candidate_calls=%u\n",
		oracleOwners[0].updateCalls, candidateOwners[0].updateCalls);
	printf("collision type3_pruner sphere oracle_found=%u candidate_found=%u oracle_hits=%u candidate_hits=%u mismatches=%u\n",
		oracleSphereFound ? 1u : 0u, candidateSphereFound ? 1u : 0u,
		oracleSphereHits, candidateSphereHits, mismatches);
	printf("collision coverage name=type3_pruner case=sphere_query_after_refit\n");
	printf("collision type3_pruner planes oracle_found=%u candidate_found=%u oracle_hits=%u candidate_hits=%u mismatches=%u\n",
		oraclePlaneFound ? 1u : 0u, candidatePlaneFound ? 1u : 0u,
		oraclePlaneHits, candidatePlaneHits, mismatches);
	printf("collision coverage name=type3_pruner case=plane_query_after_refit\n");
	printf("collision type3_pruner raycast oracle_found=%u candidate_found=%u oracle_hits=%u candidate_hits=%u mismatches=%u\n",
		oracleRayFound ? 1u : 0u, candidateRayFound ? 1u : 0u,
		oracleRayHits.GetNbEntries(), candidateRayHits.GetNbEntries(), mismatches);
	printf("collision coverage name=type3_pruner case=raycast_after_refit\n");
	printf("collision type3_pruner remove oracle_removed=%u candidate_removed=%u oracle_stale_hits=%u candidate_stale_hits=%u mismatches=%u\n",
		oracleRemoved ? 1u : 0u, candidateRemoved ? 1u : 0u,
		oracleOutput.count, candidateOutput.count, mismatches);
	printf("collision coverage name=type3_pruner case=remove_invalidates_tree\n");
	printf("collision type3_pruner add_after_build oracle_found=%u candidate_found=%u oracle_hits=%u candidate_hits=%u mismatches=%u\n",
		oracleAddedFound ? 1u : 0u, candidateAddedFound ? 1u : 0u,
		oracleAddedHits, candidateAddedHits, mismatches);
	printf("collision coverage name=type3_pruner case=add_invalidates_tree\n");

	for(unsigned i = 0; i != 3; ++i)
		{
		if(*reinterpret_cast<NxU16*>(oracleObjects[i] + 0x28) != PRUNABLE_INVALID_HANDLE)
			oracleRemove(oraclePruner, oracleObjects[i]);
		if(candidateObjects[i].mHandle != PRUNABLE_INVALID_HANDLE)
			candidatePruner->RemoveObject(&candidateObjects[i]);
		}
	{
		void** const table = *reinterpret_cast<void***>(oraclePruner);
		typedef void* (__thiscall* DeleteFn)(void*, unsigned);
		reinterpret_cast<DeleteFn>(table[0])(oraclePruner, 1);
	}
	nxPruningEngineDestroyPruner(candidatePruner);
	for(unsigned i = 0; i != 3; ++i)
		{
		void** const table = *reinterpret_cast<void***>(oracleObjects[i]);
		typedef void* (__thiscall* DeleteFn)(void*, unsigned);
		reinterpret_cast<DeleteFn>(table[0])(oracleObjects[i], 0);
		}
	oracleRayHits.Empty();
	candidateRayHits.Empty();
	*oracleOwnerAabbSlot = savedOracleOwnerAabb;
	gPrunableOwnerWorldAABB = savedCandidateOwnerAabb;
	oracleSetAllocator(savedOracleSdk);
	nxSetSdkAllocatorBridge(savedCandidateSdk);
	nxFoundationSDKAllocator = savedFoundationAllocator;
	nx2iFoundationEnd();
	return mismatches;
}

// The type 3 pruner's slots as the pruner support rows have them: phys_fn_005462 (the AABB query, slot 7), 005460
// (the sphere query, slot 8), 005456 (the plane query, slot 5), 005458 (the raycast, slot 6), 005452 (the tree
// refit each of them runs first) and 005454 (the scalar deleting destructor, slot 0). The oracle runs its own
// slots through its table; the candidate's pruner, built the same way, runs the transcribed rows on the same
// objects. Results, hit lists (as object indices) and the allocator's frees are compared.
unsigned __fastcall nxP45454(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45456(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4);
unsigned __fastcall nxP45458(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4);
unsigned __fastcall nxP45460(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP45462(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);

class NxType3CountingAllocator : public NxUserAllocator
{
public:
	unsigned frees;
	NxType3CountingAllocator() : frees(0) {}
	void* mallocDEBUG(size_t size, const char*, int) override { return ::calloc(1, size); }
	void* malloc(size_t size) override { return ::calloc(1, size); }
	void* realloc(void* memory, size_t size) override { return ::realloc(memory, size); }
	void free(void* memory) override { if(memory) ++frees; ::free(memory); }
};

unsigned nxDrivePrunerSupportType3Slots(unsigned char* base)
{
	if(!nx2iFoundationBegin())
		return 1;
	NxType3CountingAllocator allocator;
	NxUserAllocator* const savedFoundationAllocator = nxFoundationSDKAllocator;
	nxFoundationSDKAllocator = &allocator;
	SdkAllocator* const savedCandidateSdk = nxGetSdkAllocator();
	SdkAllocatorBridge candidateBridge;
	candidateBridge.mAllocator = &allocator;
	nxSetSdkAllocatorBridge(&candidateBridge);
	typedef bool (__cdecl* OracleSetAllocatorFn)(void*);
	const OracleSetAllocatorFn oracleSetAllocator = reinterpret_cast<OracleSetAllocatorFn>(base + 0x000b4020);
	SdkAllocator* const savedOracleSdk = *reinterpret_cast<SdkAllocator**>(base + 0x0012845c);
	NxType3SdkAllocator oracleBridge(&allocator);
	if(!oracleSetAllocator(&oracleBridge))
		return 1;
	typedef void* (__stdcall* OracleFactoryFn)(unsigned);
	const OracleFactoryFn oracleFactory = reinterpret_cast<OracleFactoryFn>(base + 0x000b5090);
	void* const oraclePruner = oracleFactory(3);
	Pruner* const candidatePruner = nxPruningEngineCreatePruner(3);
	if(!oraclePruner || !candidatePruner)
		return 1;

	NxU8 oracleObjects[4][0x2c] = {};
	Prunable candidateObjects[4];
	NxType3TestOwner oracleOwners[4] = {};
	NxType3TestOwner candidateOwners[4] = {};
	typedef void (__thiscall* OracleCtorFn)(void*);
	const OracleCtorFn oraclePrunableCtor = reinterpret_cast<OracleCtorFn>(base + 0x000b54a0);
	static const float boxes[4][6] = {
		{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f},
		{10.0f, 0.0f, 0.0f, 11.0f, 1.0f, 1.0f},
		{4.0f, 0.0f, 0.0f, 5.0f, 1.0f, 1.0f},
		{-6.0f, -2.0f, -2.0f, -4.0f, 2.0f, 2.0f}};
	for(unsigned i = 0; i != 4; ++i)
		{
		oraclePrunableCtor(oracleObjects[i]);
		*reinterpret_cast<void**>(oracleObjects[i] + 4) = &oracleOwners[i];
		*reinterpret_cast<NxU32*>(oracleObjects[i] + 8) = PRUNABLE_FLAG_WORLD_AABB_VALID;
		*reinterpret_cast<NxU32*>(oracleObjects[i] + 0x24) = 0xffffffffu;
		candidateObjects[i].mOwner = &candidateOwners[i];
		candidateObjects[i].mFlags = PRUNABLE_FLAG_WORLD_AABB_VALID;
		candidateObjects[i].mPrunable24 = 0xffffffffu;
		memcpy(oracleOwners[i].worldBounds, boxes[i], sizeof(boxes[i]));
		memcpy(candidateOwners[i].worldBounds, boxes[i], sizeof(boxes[i]));
		}
	void (* const savedCandidateOwnerAabb)(void*, AABB*) = gPrunableOwnerWorldAABB;
	void (** const oracleOwnerAabbSlot)(void*, AABB*) = reinterpret_cast<void (**)(void*, AABB*)>(base + 0x00128478);
	void (* const savedOracleOwnerAabb)(void*, AABB*) = *oracleOwnerAabbSlot;
	gPrunableOwnerWorldAABB = nxType3UpdateWorldAabb;
	*oracleOwnerAabbSlot = nxType3UpdateWorldAabb;

	typedef bool (__thiscall* AddObjectFn)(void*, void*);
	void** const oracleVtable = *reinterpret_cast<void***>(oraclePruner);
	const AddObjectFn oracleAdd = reinterpret_cast<AddObjectFn>(oracleVtable[1]);
	unsigned mismatches = 0, checks = 0;
	for(unsigned i = 0; i != 4; ++i)
		if(!oracleAdd(oraclePruner, oracleObjects[i]) || !candidatePruner->AddObject(&candidateObjects[i]))
			++mismatches;
	// the pool boxes, as the fixture before this sets them
	for(unsigned side = 0; side != 2; ++side)
		{
		void* const pruner = side ? static_cast<void*>(candidatePruner) : oraclePruner;
		float* const poolBoxes = *reinterpret_cast<float**>(static_cast<unsigned char*>(pruner) + 0x14);
		unsigned char* const objects = side ? reinterpret_cast<unsigned char*>(candidateObjects) : 0;
		for(unsigned i = 0; i != 4; ++i)
			{
			const unsigned handle = side ? candidateObjects[i].mHandle : *reinterpret_cast<NxU16*>(oracleObjects[i] + 0x28);
			if(poolBoxes && handle < 4) memcpy(poolBoxes + handle * 6, boxes[i], sizeof(boxes[i]));
			else ++mismatches;
			}
		(void) objects;
		}

	struct TestOutput { NxU32 capacity; NxU32 count; NxU32* entries; float growthFactor; };
	// the hits, as indices of the four objects, sorted
	struct Hits { bool result; unsigned count; unsigned indices[16]; };
	const NxU32 oracleBase = reinterpret_cast<NxU32>(oracleObjects[0]);
	const NxU32 candidateBase = reinterpret_cast<NxU32>(&candidateObjects[0]);
	const NxU32 oracleStride = sizeof(oracleObjects[0]);
	const NxU32 candidateStride = sizeof(Prunable);
	struct Fold
		{
		static void indices(Hits& hits, const NxU32* entries, unsigned count, NxU32 base, NxU32 stride)
			{
			hits.count = count;
			for(unsigned i = 0; i < count && i < 16; ++i)
				hits.indices[i] = stride ? (entries[i] - base) / stride : 0xffffu;
			for(unsigned i = 0; i < count && i < 16; ++i)
				for(unsigned j = i + 1; j < count && j < 16; ++j)
					if(hits.indices[j] < hits.indices[i]) { unsigned t = hits.indices[i]; hits.indices[i] = hits.indices[j]; hits.indices[j] = t; }
			}
		};
	#define COMPARE_HITS(NAME, OH, CH) do { ++checks; \
		const bool same = (OH).result == (CH).result && (OH).count == (CH).count && \
			memcmp((OH).indices, (CH).indices, 4 * ((OH).count < 16 ? (OH).count : 16)) == 0; \
		if(!same) { ++mismatches; printf("collision mismatch name=pruner_support_type3 query=%s check=%u oracle_count=%u candidate_count=%u\n", NAME, checks, (OH).count, (CH).count); } } while(0)

	// phys_fn_005462 / slot 7: boxes
	static const float aabbs[5][6] = {
		{3.0f, -1.0f, -1.0f, 6.0f, 2.0f, 2.0f}, {-100.0f, -100.0f, -100.0f, 100.0f, 100.0f, 100.0f},
		{9.5f, 0.5f, 0.5f, 10.5f, 0.6f, 0.6f}, {50.0f, 50.0f, 50.0f, 51.0f, 51.0f, 51.0f}, {-7.0f, -3.0f, -3.0f, -3.0f, 3.0f, 3.0f}};
	for(unsigned q = 0; q != 5; ++q)
		{
		NxU32 oracleEntries[16] = {}, candidateEntries[16] = {};
		TestOutput oracleOutput = {16, 0, oracleEntries, 2.0f};
		TestOutput candidateOutput = {16, 0, candidateEntries, 2.0f};
		typedef bool (__thiscall* AabbQueryFn)(void*, NxU32, NxU32, NxU32, NxU32);
		Hits o = {}, c = {};
		o.result = reinterpret_cast<AabbQueryFn>(oracleVtable[7])(oraclePruner, reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(aabbs[q]), 0, 0xffffffffu);
		c.result = (nxP45462(candidatePruner, 0, reinterpret_cast<unsigned>(&candidateOutput), reinterpret_cast<unsigned>(aabbs[q]), 0, 0xffffffffu) & 0xff) != 0;
		Fold::indices(o, oracleEntries, oracleOutput.count, oracleBase, oracleStride);
		Fold::indices(c, candidateEntries, candidateOutput.count, candidateBase, candidateStride);
		COMPARE_HITS("aabb", o, c);
		}
	// phys_fn_005460 / slot 8: spheres
	static const float spheres[4][4] = {{4.5f, 0.5f, 0.5f, 0.4f}, {0.0f, 0.0f, 0.0f, 100.0f}, {10.5f, 0.5f, 0.5f, 0.1f}, {30.0f, 30.0f, 30.0f, 1.0f}};
	for(unsigned q = 0; q != 4; ++q)
		{
		NxU32 oracleEntries[16] = {}, candidateEntries[16] = {};
		TestOutput oracleOutput = {16, 0, oracleEntries, 2.0f};
		TestOutput candidateOutput = {16, 0, candidateEntries, 2.0f};
		typedef bool (__thiscall* SphereQueryFn)(void*, NxU32, NxU32, NxU32, NxU32);
		Hits o = {}, c = {};
		o.result = reinterpret_cast<SphereQueryFn>(oracleVtable[8])(oraclePruner, reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(spheres[q]), 0, 0xffffffffu);
		c.result = (nxP45460(candidatePruner, 0, reinterpret_cast<unsigned>(&candidateOutput), reinterpret_cast<unsigned>(spheres[q]), 0, 0xffffffffu) & 0xff) != 0;
		Fold::indices(o, oracleEntries, oracleOutput.count, oracleBase, oracleStride);
		Fold::indices(c, candidateEntries, candidateOutput.count, candidateBase, candidateStride);
		COMPARE_HITS("sphere", o, c);
		}
	// phys_fn_005456 / slot 5: plane sets
	static const float planeSets[3][8] = {
		{1.0f, 0.0f, 0.0f, -3.0f, -1.0f, 0.0f, 0.0f, 6.0f},
		{1.0f, 0.0f, 0.0f, 100.0f, -1.0f, 0.0f, 0.0f, 100.0f},
		{0.0f, 1.0f, 0.0f, -0.5f, 0.0f, -1.0f, 0.0f, 0.6f}};
	for(unsigned q = 0; q != 3; ++q)
		{
		NxU32 oracleEntries[16] = {}, candidateEntries[16] = {};
		TestOutput oracleOutput = {16, 0, oracleEntries, 2.0f};
		TestOutput candidateOutput = {16, 0, candidateEntries, 2.0f};
		typedef bool (__thiscall* PlaneQueryFn)(void*, NxU32, NxU32, NxU32, NxU32, NxU32);
		Hits o = {}, c = {};
		o.result = reinterpret_cast<PlaneQueryFn>(oracleVtable[5])(oraclePruner, reinterpret_cast<NxU32>(&oracleOutput), reinterpret_cast<NxU32>(planeSets[q]), 2, 0, 0xffffffffu);
		c.result = (nxP45456(candidatePruner, 0, reinterpret_cast<unsigned>(&candidateOutput), reinterpret_cast<unsigned>(planeSets[q]), 2, 0, 0xffffffffu) & 0xff) != 0;
		Fold::indices(o, oracleEntries, oracleOutput.count, oracleBase, oracleStride);
		Fold::indices(c, candidateEntries, candidateOutput.count, candidateBase, candidateStride);
		COMPARE_HITS("planes", o, c);
		}
	// phys_fn_005458 / slot 6: rays
	struct RayCase { float origin[3]; float direction[3]; float distance; bool first; };
	static const RayCase rays[4] = {
		{{0.0f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 8.0f, true}, {{0.0f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 100.0f, false},
		{{-10.0f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 3.0f, true}, {{0.5f, 10.0f, 0.5f}, {0.0f, -1.0f, 0.0f}, 20.0f, true}};
	for(unsigned q = 0; q != 4; ++q)
		{
		const Ray worldRay(Point(rays[q].origin[0], rays[q].origin[1], rays[q].origin[2]),
			Point(rays[q].direction[0], rays[q].direction[1], rays[q].direction[2]));
		Container oracleRayHits, candidateRayHits;
		typedef bool (__thiscall* RaycastFn)(void*, Container&, const Ray&, float, bool, NxU32);
		Hits o = {}, c = {};
		o.result = reinterpret_cast<RaycastFn>(oracleVtable[6])(oraclePruner, oracleRayHits, worldRay, rays[q].distance, rays[q].first, 0xffffffffu);
		unsigned distanceBits;
		memcpy(&distanceBits, &rays[q].distance, 4);
		c.result = (nxP45458(candidatePruner, 0, reinterpret_cast<unsigned>(&candidateRayHits), reinterpret_cast<unsigned>(&worldRay),
			distanceBits, rays[q].first ? 1u : 0u, 0xffffffffu) & 0xff) != 0;
		NxU32 oracleEntries[16] = {}, candidateEntries[16] = {};
		for(unsigned i = 0; i < oracleRayHits.GetNbEntries() && i < 16; ++i) oracleEntries[i] = oracleRayHits.GetEntry(i);
		for(unsigned i = 0; i < candidateRayHits.GetNbEntries() && i < 16; ++i) candidateEntries[i] = candidateRayHits.GetEntry(i);
		Fold::indices(o, oracleEntries, oracleRayHits.GetNbEntries(), oracleBase, oracleStride);
		Fold::indices(c, candidateEntries, candidateRayHits.GetNbEntries(), candidateBase, candidateStride);
		COMPARE_HITS("ray", o, c);
		oracleRayHits.Empty();
		candidateRayHits.Empty();
		}
	#undef COMPARE_HITS

	// removal, then phys_fn_005454: the scalar deleting destructor (slot 0) with the flag set.
	for(unsigned i = 0; i != 4; ++i)
		{
		typedef bool (__thiscall* RemoveObjectFn)(void*, void*);
		reinterpret_cast<RemoveObjectFn>(oracleVtable[2])(oraclePruner, oracleObjects[i]);
		candidatePruner->RemoveObject(&candidateObjects[i]);
		}
	unsigned freesOracle, freesCandidate;
	{
		allocator.frees = 0;
		typedef void* (__thiscall* DeleteFn)(void*, unsigned);
		reinterpret_cast<DeleteFn>(oracleVtable[0])(oraclePruner, 1);
		freesOracle = allocator.frees;
		allocator.frees = 0;
		nxP45454(candidatePruner, 0, 1);
		freesCandidate = allocator.frees;
	}
	++checks;
	if(freesOracle != freesCandidate) { ++mismatches; printf("collision mismatch name=pruner_support_type3 destructor oracle_frees=%u candidate_frees=%u\n", freesOracle, freesCandidate); }
	for(unsigned i = 0; i != 4; ++i)
		{
		void** const table = *reinterpret_cast<void***>(oracleObjects[i]);
		typedef void* (__thiscall* DeleteFn)(void*, unsigned);
		reinterpret_cast<DeleteFn>(table[0])(oracleObjects[i], 0);
		}
	*oracleOwnerAabbSlot = savedOracleOwnerAabb;
	gPrunableOwnerWorldAABB = savedCandidateOwnerAabb;
	oracleSetAllocator(savedOracleSdk);
	nxSetSdkAllocatorBridge(savedCandidateSdk);
	nxFoundationSDKAllocator = savedFoundationAllocator;
	nx2iFoundationEnd();
	printf("collision name=pruner_support_type3 rows=005454,005456,005458,005460,005462 checks=%u frees=%u mismatches=%u\n", checks, freesOracle, mismatches);
	return mismatches;
}
