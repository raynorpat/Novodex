#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NxUserAllocator.h"
#include "SceneAuxiliary.h"
#include "PhysicsPairLoader.h"

static const unsigned kSceneAuxiliaryBlockRva = 0x000116b0;
static const char* const kAllocatorExport = "?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A";
static const char* const kCandidateSymbol = "?destroy@NxSceneAuxiliaryArrayBlock@@QAEXXZ";

class SceneAuxBlockTestAllocator : public NxUserAllocator
	{
	public:
	SceneAuxBlockTestAllocator() : allocations(0), freeCount(0), freeOrderCount(0), nextTag(0) {}
	void* mallocDEBUG(size_t size, const char*, int) { return malloc(size); }
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType) { return malloc(size); }
	void* malloc(size_t size) override
		{
		void* memory = ::malloc(size);
		if(memory && allocations < 32)
			{
			allocation[allocations] = memory;
			tag[allocations] = nextTag;
			++allocations;
			}
		return memory;
		}
	void* realloc(void* memory, size_t size) override { return ::realloc(memory, size); }
	void free(void* memory) override
		{
		if(!memory)
			return;
		for(unsigned i = 0; i < allocations; ++i)
			if(allocation[i] == memory)
				{
				if(freeOrderCount < 32)
					freeOrder[freeOrderCount++] = tag[i];
				allocation[i] = 0;
				break;
				}
		++freeCount;
		::free(memory);
		}
	void resetFreeTrace() { freeCount = 0; freeOrderCount = 0; }
	void* allocation[32] = {};
	unsigned tag[32] = {};
	unsigned freeOrder[32] = {};
	unsigned allocations;
	unsigned freeCount;
	unsigned freeOrderCount;
	unsigned nextTag;
	};

struct AuxCase
	{
	unsigned presentMask;
	unsigned seed;
	};

static unsigned nxFold(unsigned state, unsigned value)
	{
	for(unsigned i = 0; i != 4; ++i)
		{
		state ^= (value >> (i * 8)) & 0xff;
		state *= 16777619u;
		}
	return state;
	}

static unsigned nxDigestBytes(unsigned digest, const unsigned char* bytes, unsigned size)
	{
	for(unsigned i = 0; i != size; ++i)
		{
		digest ^= bytes[i];
		digest *= 16777619u;
		}
	return digest;
	}

static bool nxMapFunctionRva(const wchar_t* executablePath, unsigned* rva)
	{
	wchar_t directory[MAX_PATH];
	if(wcscpy_s(directory, executablePath) != 0)
		return false;
	wchar_t* slash = wcsrchr(directory, L'\\');
	if(!slash)
		return false;
	slash[1] = 0;
	wchar_t mapPath[MAX_PATH];
	if(swprintf_s(mapPath, L"%sNxPhysics.map", directory) < 0)
		return false;
	FILE* map = 0;
	if(_wfopen_s(&map, mapPath, L"rb") != 0 || !map)
		return false;
	char line[4096];
	bool found = false;
	while(fgets(line, sizeof(line), map))
		{
		char* symbol = strstr(line, kCandidateSymbol);
		if(!symbol)
			continue;
		unsigned address = 0;
		if(sscanf_s(symbol + strlen(kCandidateSymbol), "%x", &address) == 1
			&& address >= 0x10000000u)
			{
			*rva = address - 0x10000000u;
			found = true;
			break;
			}
		}
	fclose(map);
	return found;
	}

static void nxSeedBlock(unsigned char* block, const AuxCase& test,
	SceneAuxBlockTestAllocator& allocator)
	{
	memset(block, 0xcd, 0x40);
	for(unsigned slot = 0; slot != 4; ++slot)
		{
		const unsigned offset = 0x30 - slot * 0x10;
		unsigned* words = reinterpret_cast<unsigned*>(block + offset);
		allocator.nextTag = slot;
		words[0] = (test.presentMask & (1u << slot))
			? reinterpret_cast<unsigned>(allocator.malloc(8)) : 0;
		words[1] = test.seed ^ (0x62410000u + slot);
		words[2] = test.seed + (0x62420000u + slot);
		}
	}

static bool nxCase(const AuxCase& test, void (__thiscall* oracle)(void*),
	void (__thiscall* candidate)(void*), SceneAuxBlockTestAllocator& allocator,
	unsigned& oracleDigest, unsigned& candidateDigest, unsigned& totalOracleFrees,
	unsigned& totalCandidateFrees, unsigned& mismatches)
	{
	alignas(4) unsigned char oracleBlock[0x40];
	alignas(4) unsigned char candidateBlock[0x40];
	allocator.allocations = 0;
	nxSeedBlock(oracleBlock, test, allocator);
	nxSeedBlock(candidateBlock, test, allocator);
	allocator.resetFreeTrace();
	unsigned oracleStackBefore = 0, oracleStackAfter = 0;
	__asm mov oracleStackBefore, esp
	oracle(oracleBlock);
	__asm mov oracleStackAfter, esp
	const unsigned oracleFrees = allocator.freeCount;
	const unsigned oracleOrderCount = allocator.freeOrderCount;
	unsigned oracleOrder[32];
	memcpy(oracleOrder, allocator.freeOrder, sizeof(oracleOrder));
	totalOracleFrees += oracleFrees;
	oracleDigest = nxFold(oracleDigest, test.presentMask);
	oracleDigest = nxFold(oracleDigest, test.seed);
	oracleDigest = nxDigestBytes(oracleDigest, oracleBlock, sizeof(oracleBlock));
	oracleDigest = nxFold(oracleDigest, oracleFrees);
	oracleDigest = nxFold(oracleDigest, oracleStackAfter - oracleStackBefore);
	allocator.resetFreeTrace();
	unsigned candidateStackBefore = 0, candidateStackAfter = 0;
	__asm mov candidateStackBefore, esp
	candidate(candidateBlock);
	__asm mov candidateStackAfter, esp
	const unsigned candidateFrees = allocator.freeCount;
	totalCandidateFrees += candidateFrees;
	candidateDigest = nxFold(candidateDigest, test.presentMask);
	candidateDigest = nxFold(candidateDigest, test.seed);
	candidateDigest = nxDigestBytes(candidateDigest, candidateBlock, sizeof(candidateBlock));
	candidateDigest = nxFold(candidateDigest, candidateFrees);
	candidateDigest = nxFold(candidateDigest, candidateStackAfter - candidateStackBefore);
	if(memcmp(oracleBlock, candidateBlock, sizeof(oracleBlock)) != 0)
		{ ++mismatches; fprintf(stderr, "case %08x byte mismatch\n", test.presentMask); }
	if(oracleFrees != candidateFrees || oracleOrderCount != allocator.freeOrderCount
		|| memcmp(oracleOrder, allocator.freeOrder, oracleOrderCount * sizeof(unsigned)) != 0)
		{
		++mismatches;
		fprintf(stderr, "case %08x frees=%u/%u order-count=%u/%u order=", test.presentMask,
			oracleFrees, candidateFrees, oracleOrderCount, allocator.freeOrderCount);
		for(unsigned i = 0; i < oracleOrderCount; ++i)
			fprintf(stderr, "%u/%u ", oracleOrder[i], allocator.freeOrder[i]);
		fprintf(stderr, "\n");
		}
	if(oracleStackAfter != oracleStackBefore || candidateStackAfter != candidateStackBefore)
		{ ++mismatches; fprintf(stderr, "case %08x stack=%08x/%08x %08x/%08x\n",
			test.presentMask, oracleStackBefore, oracleStackAfter,
			candidateStackBefore, candidateStackAfter); }
	return oracleStackAfter == oracleStackBefore && candidateStackAfter == candidateStackBefore;
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc != 3)
		{
		fprintf(stderr, "usage: NxPhysicsSceneAuxiliaryBlockTests <oracle directory> <NxPhysics.dll sha256>\n");
		return 2;
		}
	wchar_t oraclePath[MAX_PATH];
	if(swprintf_s(oraclePath, L"%s\\NxPhysics.dll", argv[1]) < 0)
		return nxFail("cannot form the pinned oracle path");
	if(!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32)
		|| !AddDllDirectory(argv[1]))
		return nxFail("cannot restrict DLL loading to the oracle and system directories");
	HMODULE oracleModule = LoadLibraryExW(oraclePath, 0,
		LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!oracleModule)
		return nxFail("cannot load pinned oracle NxPhysics.dll");
	wchar_t loadedPath[MAX_PATH];
	char loadedHash[65], candidateHash[65], expectedHash[65];
	if(!GetModuleFileNameW(oracleModule, loadedPath, MAX_PATH)
		|| !nxSha256(loadedPath, loadedHash)
		|| wcstombs_s(0, expectedHash, sizeof(expectedHash), argv[2], _TRUNCATE) != 0
		|| strcmp(loadedHash, expectedHash) != 0)
		return nxFail("loaded NxPhysics.dll is not the pinned oracle");
	wchar_t executablePath[MAX_PATH];
	if(!GetModuleFileNameW(0, executablePath, MAX_PATH))
		return nxFail("cannot resolve the test executable directory");
	wchar_t executableDirectory[MAX_PATH];
	if(wcscpy_s(executableDirectory, executablePath) != 0)
		return nxFail("cannot form candidate DLL path");
	wchar_t* slash = wcsrchr(executableDirectory, L'\\');
	if(!slash)
		return nxFail("test executable path has no directory");
	slash[1] = 0;
	wchar_t candidatePath[MAX_PATH], builtPath[MAX_PATH];
	if(swprintf_s(builtPath, L"%sNxPhysics.dll", executableDirectory) < 0
		|| swprintf_s(candidatePath, L"%sNxPhysicsCandidate.dll", executableDirectory) < 0)
		return nxFail("cannot form candidate DLL paths");
	if(!nxSha256(builtPath, loadedHash) || !nxSha256(candidatePath, candidateHash)
		|| strcmp(loadedHash, candidateHash) != 0)
		return nxFail("candidate alias does not match the built NxPhysics.dll");
	if(!AddDllDirectory(executableDirectory))
		return nxFail("cannot add the candidate directory to the isolated load search");
	HMODULE candidateModule = LoadLibraryExW(candidatePath, 0,
		LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!candidateModule)
		return nxFail("cannot load the built candidate NxPhysics.dll");
	unsigned candidateRva = 0;
	if(!nxMapFunctionRva(executablePath, &candidateRva))
		return nxFail("candidate linker map does not contain phys_fn_000624 implementation");
	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	NxUserAllocator** allocatorSlot = foundation
		? reinterpret_cast<NxUserAllocator**>(GetProcAddress(foundation, kAllocatorExport)) : 0;
	if(!allocatorSlot)
		return nxFail("cannot resolve the shared Foundation allocator pointer");
	NxUserAllocator* savedAllocator = *allocatorSlot;
	SceneAuxBlockTestAllocator testAllocator;
	*allocatorSlot = &testAllocator;
	void (__thiscall* oracle)(void*) = reinterpret_cast<void (__thiscall*)(void*)>(
		reinterpret_cast<unsigned char*>(oracleModule) + kSceneAuxiliaryBlockRva);
	void (__thiscall* candidate)(void*) = reinterpret_cast<void (__thiscall*)(void*)>(
		reinterpret_cast<unsigned char*>(candidateModule) + candidateRva);
	const AuxCase cases[] = {
		{0x0fu, 0x00062401u},
		{0x0au, 0x00062402u},
		{0x00u, 0x00062403u}
		};
	unsigned oracleDigest = 2166136261u, candidateDigest = 2166136261u;
	unsigned totalOracleFrees = 0, totalCandidateFrees = 0, mismatches = 0;
	for(unsigned i = 0; i != sizeof(cases) / sizeof(cases[0]); ++i)
		nxCase(cases[i], oracle, candidate, testAllocator, oracleDigest,
			candidateDigest, totalOracleFrees, totalCandidateFrees, mismatches);
	*allocatorSlot = savedAllocator;
	printf("layout coverage row=phys_fn_000624 cases=%u oracle_frees=%u candidate_frees=%u stack=0/0 mismatches=%u candidate_digest=%08x\n",
		static_cast<unsigned>(sizeof(cases) / sizeof(cases[0])), totalOracleFrees,
		totalCandidateFrees, mismatches, candidateDigest);
	printf("layout oracle digest=%08x\n", oracleDigest);
	FreeLibrary(candidateModule);
	FreeLibrary(oracleModule);
	return mismatches ? nxFail("oracle and candidate auxiliary-block cleanup differ") : 0;
	}
