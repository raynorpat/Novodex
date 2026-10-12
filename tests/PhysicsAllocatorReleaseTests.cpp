#include "PhysicsPairLoader.h"

#include <string.h>

typedef void (__thiscall* ReleaseRow)(void*);
typedef void (__fastcall* CandidateReleaseRow)(void*);
typedef void (__fastcall* FreeSlot)(void*, void*, void*);

struct FakeAllocator
	{
	void** vtable;
	};

static void* gFreed[8];
static unsigned gFreeCount;
static FakeAllocator gAllocator;
static void* gAllocatorVtable[6];

static void __fastcall nxRecordFree(void*, void*, void* memory)
	{
	if(gFreeCount < sizeof(gFreed) / sizeof(gFreed[0]))
		gFreed[gFreeCount] = memory;
	++gFreeCount;
	}

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
			{
			preferredBase = static_cast<unsigned>(strtoul(basePos + sizeof(marker) - 1, 0, 16));
			}
		char* symbolPos = strstr(line, symbol);
		if(!symbolPos)
			continue;
		char* addressText = symbolPos + strlen(symbol);
		while(*addressText == ' ' || *addressText == '\t') ++addressText;
		char* addressEnd = 0;
		unsigned preferredAddress = static_cast<unsigned>(strtoul(addressText, &addressEnd, 16));
		if(addressEnd == addressText || !preferredBase || preferredAddress < preferredBase)
			{ fclose(map); return false; }
		*address = reinterpret_cast<unsigned char*>(module) + (preferredAddress - preferredBase);
		found = true;
		break;
		}
	fclose(map);
	return found;
	}

static bool nxRunRow(HMODULE physics, const wchar_t* pairDirectory,
	bool candidate, unsigned rva, const char* symbol, unsigned row)
	{
	void* entry = reinterpret_cast<unsigned char*>(physics) + rva;
	if(candidate && !nxResolveMapAddress(pairDirectory, physics, symbol, &entry))
		{
		fprintf(stderr, "FAIL candidate map symbol missing: %s\n", symbol);
		return false;
		}
	void** allocatorImportSlot = reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(physics) + (candidate ? 0x0011a0b0 : 0x001041bc));
	if(candidate && !nxResolveMapAddress(pairDirectory, physics,
		"__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A", reinterpret_cast<void**>(&allocatorImportSlot)))
		{
		fprintf(stderr, "FAIL candidate allocator import slot missing from map\n");
		return false;
		}
	void** allocatorVariable = *reinterpret_cast<void***>(allocatorImportSlot);
	if(!allocatorVariable)
		{
		fprintf(stderr, "FAIL imported allocator global variable is null\n");
		return false;
		}
	DWORD oldProtect = 0;
	if(!VirtualProtect(allocatorVariable, sizeof(void*), PAGE_READWRITE, &oldProtect))
		return false;
	void* originalAllocator = *allocatorVariable;
	gAllocator.vtable = gAllocatorVtable;
	gAllocatorVtable[5] = reinterpret_cast<void*>(&nxRecordFree);
	*allocatorVariable = &gAllocator;

	unsigned failures = 0;
	const unsigned offsets[] = { 0x0c, 0x14, 0x08 };
	for(unsigned mask = 0; mask < 8; ++mask)
		{
		unsigned char object[0x1c];
		memset(object, 0x5a, sizeof(object));
		for(unsigned i = 0; i < 3; ++i)
			*reinterpret_cast<void**>(object + offsets[i]) = 0;
		void* expected[3] = { 0, 0, 0 };
		for(unsigned i = 0; i < 3; ++i)
			if(mask & (1u << i))
				{
				expected[i] = reinterpret_cast<void*>(0x1000u + row * 0x100u + mask * 0x10u + i);
				*reinterpret_cast<void**>(object + offsets[i]) = expected[i];
				}
		gFreeCount = 0;
		memset(gFreed, 0, sizeof(gFreed));
		if(candidate)
			reinterpret_cast<CandidateReleaseRow>(entry)(object);
		else
			reinterpret_cast<ReleaseRow>(entry)(object);
		unsigned expectedCount = 0;
		bool matched = true;
		for(unsigned i = 0; i < 3; ++i)
			{
			void* actual = *reinterpret_cast<void**>(object + offsets[i]);
			if(expected[i])
				{
				if(actual != 0) matched = false;
				if(expectedCount >= gFreeCount || gFreed[expectedCount] != expected[i]) matched = false;
				++expectedCount;
				}
			else if(actual != 0)
				matched = false;
			}
		if(gFreeCount != expectedCount) matched = false;
		if(!matched) ++failures;
		}

	*allocatorVariable = originalAllocator;
	DWORD ignored = 0;
	VirtualProtect(allocatorVariable, sizeof(void*), oldProtect, &ignored);
	printf("allocrelease row=%06u cases=8 failures=%u\n", row, failures);
	return failures == 0;
	}

static bool nxRunFluidManagerDeletingDestructor(HMODULE physics,
	const wchar_t* pairDirectory, bool candidate)
	{
	void* entry = reinterpret_cast<unsigned char*>(physics) + 0x89fd0;
	if(candidate && !nxResolveMapAddress(pairDirectory, physics,
		"?nxFluidManagerDeletingDestructor@@YIPAXPAX0E@Z", &entry))
		{
		fprintf(stderr, "FAIL candidate fluid manager deleting destructor missing from map\n");
		return false;
		}
	void** allocatorImportSlot = reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(physics) +
			(candidate ? 0x0011a0b0 : 0x001041bc));
	if(candidate && !nxResolveMapAddress(pairDirectory, physics,
		"__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A",
		reinterpret_cast<void**>(&allocatorImportSlot)))
		{
		fprintf(stderr, "FAIL candidate allocator import slot missing from map\n");
		return false;
		}
	void** allocatorVariable = *reinterpret_cast<void***>(allocatorImportSlot);
	if(!allocatorVariable)
		{
		fprintf(stderr, "FAIL imported allocator global variable is null\n");
		return false;
		}
	DWORD oldProtect = 0;
	if(!VirtualProtect(allocatorVariable, sizeof(void*), PAGE_READWRITE, &oldProtect))
		return false;
	void* originalAllocator = *allocatorVariable;
	gAllocator.vtable = gAllocatorVtable;
	gAllocatorVtable[5] = reinterpret_cast<void*>(&nxRecordFree);
	*allocatorVariable = &gAllocator;

	unsigned failures = 0;
	for(unsigned char deleteObject = 0; deleteObject != 2; ++deleteObject)
		{
		unsigned char manager[0x34] = {};
		gFreeCount = 0;
		memset(gFreed, 0, sizeof(gFreed));
		void* result = 0;
		if(candidate)
			{
			typedef void* (__fastcall* CandidateDeletingDestructor)(void*, void*, unsigned char);
			CandidateDeletingDestructor destroy = reinterpret_cast<CandidateDeletingDestructor>(entry);
			result = destroy(manager, 0, deleteObject);
			}
		else
			{
			typedef void* (__thiscall* OracleDeletingDestructor)(void*, unsigned char);
			OracleDeletingDestructor destroy = reinterpret_cast<OracleDeletingDestructor>(entry);
			result = destroy(manager, deleteObject);
			}
		const unsigned expectedFrees = deleteObject & 1 ? 1u : 0u;
		if(result != manager || gFreeCount != expectedFrees ||
			(expectedFrees && gFreed[0] != manager))
			++failures;
		}

	*allocatorVariable = originalAllocator;
	DWORD ignored = 0;
	VirtualProtect(allocatorVariable, sizeof(void*), oldProtect, &ignored);
	printf("fluid manager deleting-destructor cases=2 frees=0,1 pointers=1 mismatches=%u\n",
		failures);
	return failures == 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsAllocatorReleaseTests", pairDirectory, &physics);
	if(status) return status;
	void* ignored = 0;
	const bool candidate = nxResolveMapAddress(pairDirectory, physics,
		"__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A", &ignored);
	bool matched = true;
	matched &= nxRunRow(physics, pairDirectory, candidate, 0x9a500,
		"@nxReleaseAllocatorFields004149@4", 4149);
	matched &= nxRunRow(physics, pairDirectory, candidate, 0x9a560,
		"@nxReleaseAllocatorFields004151@4", 4151);
	matched &= nxRunFluidManagerDeletingDestructor(physics, pairDirectory, candidate);
	if(!matched) return 1;
	return nxReportPairIdentity(pairDirectory);
	}
