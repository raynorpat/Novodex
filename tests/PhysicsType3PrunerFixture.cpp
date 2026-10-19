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
	printf("collision pruner_pool_reserve row=phys_fn_004855 cases=4 modes=0-3 capacities=matched mismatches=%u\n",
		mismatches);
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
