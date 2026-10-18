#include "PhysicsPairLoader.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char kOracleSha256[] =
	"4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c";
static const unsigned kSortRva = 0x00054140;

struct EdgeRecord
	{
	uint32_t key0;
	uint32_t key1;
	uint32_t faceAndEdge;
	uint32_t adjacent;
	};

typedef void (__cdecl *SortEdgeRecordsFn)(void*, int, int);

static bool nxResolveMapAddress(const wchar_t* pairDirectory, HMODULE module,
	const char* symbol, void** address)
	{
	wchar_t mapPath[MAX_PATH];
	if(swprintf_s(mapPath, L"%s\\NxPhysics.map", pairDirectory) < 0)
		return false;
	FILE* map = 0;
	if(_wfopen_s(&map, mapPath, L"r") || !map)
		return false;
	unsigned preferredBase = 0;
	char line[2048];
	bool found = false;
	while(fgets(line, sizeof(line), map))
		{
		static const char marker[] = "Preferred load address is ";
		char* basePos = strstr(line, marker);
		if(basePos)
			preferredBase = static_cast<unsigned>(strtoul(basePos + sizeof(marker) - 1, 0, 16));
		char* symbolPos = strstr(line, symbol);
		if(!symbolPos)
			continue;
		char* addressText = symbolPos + strlen(symbol);
		while(*addressText == ' ' || *addressText == '\t') ++addressText;
		char* addressEnd = 0;
		const unsigned preferredAddress = static_cast<unsigned>(strtoul(addressText, &addressEnd, 16));
		if(addressEnd == addressText || !preferredBase || preferredAddress < preferredBase)
			break;
		*address = reinterpret_cast<unsigned char*>(module) + (preferredAddress - preferredBase);
		found = true;
		break;
		}
	fclose(map);
	return found;
	}

static uint32_t nxDigest(const EdgeRecord* records, unsigned count)
	{
	uint32_t digest = 2166136261u;
	const uint32_t* words = reinterpret_cast<const uint32_t*>(records);
	for(unsigned i = 0; i < count * 4; ++i)
		digest = (digest ^ words[i]) * 16777619u;
	return digest;
	}

static bool nxRunCase(SortEdgeRecordsFn sort, unsigned caseIndex,
	const EdgeRecord* seed, unsigned count, int low, int high, uint32_t* digest)
	{
	EdgeRecord actual[18];
	if(count > sizeof(actual) / sizeof(actual[0]))
		return false;
	memset(actual, 0xA5, sizeof(actual));
	memcpy(actual + 1, seed, count * sizeof(EdgeRecord));
	EdgeRecord before[18];
	memcpy(before, actual, sizeof(actual));
	sort(actual + 1, low, high);
	// Canary records on both sides must survive. Fields outside the selected
	// half-open test region must also remain byte-identical.
	if(memcmp(actual, before, sizeof(EdgeRecord)) ||
		memcmp(actual + count + 1, before + count + 1, sizeof(EdgeRecord)))
		return false;
	for(unsigned i = 1; i <= count; ++i)
		if((static_cast<int>(i - 1) < low || static_cast<int>(i - 1) > high) &&
			memcmp(actual + i, before + i, sizeof(EdgeRecord)))
			return false;
	*digest = nxDigest(actual, count + 2) ^ (caseIndex * 0x9e3779b9u);
	return true;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int openStatus = nxOpenPair(argc, argv, "NxPhysicsTriangleMeshSortTests", pairDirectory, &physics);
	if(openStatus)
		return openStatus;

	wchar_t physicsPath[MAX_PATH];
	char loadedHash[65];
	if(!GetModuleFileNameW(physics, physicsPath, MAX_PATH) || !nxSha256(physicsPath, loadedHash))
		return nxFail("cannot identify the loaded Physics DLL");

	void* sortAddress = 0;
	if(strcmp(loadedHash, kOracleSha256) == 0)
		sortAddress = reinterpret_cast<unsigned char*>(physics) + kSortRva;
	else if(!nxResolveMapAddress(pairDirectory, physics, "nxTriangleMeshSortEdgeRecords", &sortAddress))
		{
		fprintf(stderr, "FAIL candidate map lacks nxTriangleMeshSortEdgeRecords (loaded=%s)\n", loadedHash);
		return 1;
		}
	SortEdgeRecordsFn sort = reinterpret_cast<SortEdgeRecordsFn>(sortAddress);

	const EdgeRecord full[] = {
		{ 8, 5, 0x80000004u, 0xffffffffu }, { 2, 9, 0x00000001u, 0xffffffffu },
		{ 8, 5, 0x40000007u, 0xffffffffu }, { 2, 3, 0x00000002u, 0xffffffffu },
		{ 0xffffffffu, 0, 0x80000008u, 0xffffffffu }, { 2, 9, 0x40000003u, 0xffffffffu },
		{ 8, 4, 0x00000005u, 0xffffffffu }, { 2, 3, 0x80000006u, 0xffffffffu },
		{ 0, 0xffffffffu, 0x40000009u, 0xffffffffu }, { 8, 5, 0x0000000au, 0xffffffffu },
		{ 2, 9, 0x8000000bu, 0xffffffffu }, { 8, 4, 0x4000000cu, 0xffffffffu }
		};
	const EdgeRecord partial[] = {
		{ 0xdeadbeefu, 17, 0x01020304u, 0 }, { 4, 4, 0x11111111u, 1 },
		{ 3, 9, 0x22222222u, 2 }, { 1, 2, 0x33333333u, 3 },
		{ 3, 8, 0x44444444u, 4 }, { 1, 8, 0x55555555u, 5 },
		{ 3, 9, 0x66666666u, 6 }, { 0, 5, 0x77777777u, 7 },
		{ 0xbeefcafeu, 19, 0x08070605u, 8 }
		};
	const EdgeRecord single[] = { { 17, 9, 0xdeadbeefu, 0x12345678u } };
	const EdgeRecord empty[] = {
		{ 5, 5, 0xa0a0a0a0u, 0 }, { 4, 4, 0xb1b1b1b1u, 1 },
		{ 3, 3, 0xc2c2c2c2u, 2 }, { 2, 2, 0xd3d3d3d3u, 3 }
		};
	uint32_t digests[4] = {};
	bool ok = true;
	ok = nxRunCase(sort, 0, full, sizeof(full) / sizeof(full[0]), 0, 11, &digests[0]) && ok;
	ok = nxRunCase(sort, 1, partial, sizeof(partial) / sizeof(partial[0]), 1, 7, &digests[1]) && ok;
	ok = nxRunCase(sort, 2, single, sizeof(single) / sizeof(single[0]), 0, 0, &digests[2]) && ok;
	ok = nxRunCase(sort, 3, empty, sizeof(empty) / sizeof(empty[0]), 2, 1, &digests[3]) && ok;
	uint32_t digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(digests) / sizeof(digests[0]); ++i)
		digest = (digest ^ digests[i]) * 16777619u;
	printf("triangle_mesh_sort row=002180/002182 cases=4 digest=%08x mismatches=%u\n",
		digest, ok ? 0u : 1u);
	return ok ? 0 : nxFail("sorted edge records differ from the pinned oracle behavior");
	}
