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

	NxU8 oracleObjects[2][0x2c] = {};
	Prunable candidateObjects[2];
	typedef void (__thiscall* OracleCtorFn)(void*);
	const OracleCtorFn oraclePrunableCtor =
		reinterpret_cast<OracleCtorFn>(base + 0x000b54a0);
	for(unsigned i = 0; i != 2; ++i)
		{
		oraclePrunableCtor(oracleObjects[i]);
		*reinterpret_cast<NxU32*>(oracleObjects[i] + 8) = PRUNABLE_FLAG_WORLD_AABB_VALID;
		*reinterpret_cast<NxU32*>(oracleObjects[i] + 0x24) = 0xffffffffu;
		candidateObjects[i].mFlags = PRUNABLE_FLAG_WORLD_AABB_VALID;
		candidateObjects[i].mPrunable24 = 0xffffffffu;
		}

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

	for(unsigned side = 0; side != 2; ++side)
		{
		void* const pruner = side ? static_cast<void*>(candidatePruner) : oraclePruner;
		unsigned char* const object = side ?
			static_cast<unsigned char*>(static_cast<void*>(&candidateObjects[0])) : oracleObjects[0];
		float* const poolBoxes = *reinterpret_cast<float**>(
			static_cast<unsigned char*>(pruner) + 0x14);
		const unsigned handle = *reinterpret_cast<NxU16*>(object + 0x28);
		if(poolBoxes && handle < 2)
			memcpy(poolBoxes + handle * 6, query, sizeof(query));
		else
			++mismatches;
		}
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
	printf("collision type3_pruner refit objects=2 oracle_found=%u candidate_found=%u oracle_hits=%u candidate_hits=%u mismatches=%u\n",
		oracleFound ? 1u : 0u, candidateFound ? 1u : 0u, oracleHits, candidateHits, mismatches);
	printf("collision coverage name=type3_pruner case=tree_refit_moved_leaf\n");

	for(unsigned i = 0; i != 2; ++i)
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
	for(unsigned i = 0; i != 2; ++i)
		{
		void** const table = *reinterpret_cast<void***>(oracleObjects[i]);
		typedef void* (__thiscall* DeleteFn)(void*, unsigned);
		reinterpret_cast<DeleteFn>(table[0])(oracleObjects[i], 0);
		}
	oracleSetAllocator(savedOracleSdk);
	nxSetSdkAllocatorBridge(savedCandidateSdk);
	nxFoundationSDKAllocator = savedFoundationAllocator;
	nx2iFoundationEnd();
	return mismatches;
}
