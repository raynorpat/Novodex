#include "PhysicsPairLoader.h"

// The ten Phase 2 exports of the shipped NxPhysics.dll, as assigned in
// docs/reconstruction/novodex-physics/inventory.json. The remaining 31 named
// exports belong to later phases and are not required here.
static const char* const physicsPhase2Exports[] =
	{
	"NxCreatePhysicsSDK",
	"NxFluidAssert",
	"NxFluidDebugAABB",
	"NxFluidDebugArrow",
	"NxFluidDebugLine",
	"NxFluidDebugPoint",
	"NxFluidDebugSphere",
	"NxFluidDebugTriangle",
	"NxFluidFree",
	"NxFluidPAlloc"
	};

static int checkPe32(HMODULE module)
	{
	const BYTE* base = reinterpret_cast<const BYTE*>(module);
	const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
	if(dos->e_magic != IMAGE_DOS_SIGNATURE)
		return nxFail("loaded image has no MZ signature");

	const IMAGE_NT_HEADERS32* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
	if(nt->Signature != IMAGE_NT_SIGNATURE)
		return nxFail("loaded image has no PE signature");
	if(nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return nxFail("loaded image is not PE32 i386");

	printf("pe32 machine=0x%04x magic=0x%04x\n", nt->FileHeader.Machine, nt->OptionalHeader.Magic);
	return 0;
	}

static int checkExports(HMODULE module)
	{
	unsigned found = 0;
	for(size_t i = 0; i < sizeof(physicsPhase2Exports) / sizeof(physicsPhase2Exports[0]); ++i)
		{
		if(GetProcAddress(module, physicsPhase2Exports[i]))
			++found;
		else
			printf("export_missing name=%s\n", physicsPhase2Exports[i]);
		}

	printf("exports expected=%u found=%u\n",
		static_cast<unsigned>(sizeof(physicsPhase2Exports) / sizeof(physicsPhase2Exports[0])), found);
	if(found != sizeof(physicsPhase2Exports) / sizeof(physicsPhase2Exports[0]))
		return nxFail("Phase 2 export missing from NxPhysics.dll");
	return 0;
	}

// Walks the loaded import directory: every imported module must itself be a
// module the audit already accepted, and every imported name must resolve.
static int checkImports(HMODULE module, const wchar_t* pairDirectory)
	{
	const BYTE* base = reinterpret_cast<const BYTE*>(module);
	const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
	const IMAGE_NT_HEADERS32* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
	const IMAGE_DATA_DIRECTORY& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	wchar_t systemDirectory[MAX_PATH];
	if(!GetSystemDirectoryW(systemDirectory, MAX_PATH))
		return nxFail("cannot resolve system directory");

	unsigned dlls = 0;
	unsigned resolved = 0;
	unsigned unresolved = 0;
	if(directory.VirtualAddress)
		{
		const IMAGE_IMPORT_DESCRIPTOR* descriptor =
			reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress);
		for(; descriptor->Name; ++descriptor)
			{
			const char* name = reinterpret_cast<const char*>(base + descriptor->Name);
			HMODULE imported = GetModuleHandleA(name);
			wchar_t importedPath[MAX_PATH];
			if(!imported || !GetModuleFileNameW(imported, importedPath, MAX_PATH))
				{
				printf("import_module_unloaded name=%s\n", name);
				++unresolved;
				continue;
				}
			if(!nxInDirectory(importedPath, pairDirectory) && !nxInDirectory(importedPath, systemDirectory))
				{
				printf("import_module_rejected name=%s path=%S\n", name, importedPath);
				++unresolved;
				continue;
				}
			++dlls;

			if(!descriptor->OriginalFirstThunk)
				{
				printf("import_names_unavailable name=%s\n", name);
				++unresolved;
				continue;
				}
			const IMAGE_THUNK_DATA32* thunk =
				reinterpret_cast<const IMAGE_THUNK_DATA32*>(base + descriptor->OriginalFirstThunk);
			const IMAGE_THUNK_DATA32* bound =
				reinterpret_cast<const IMAGE_THUNK_DATA32*>(base + descriptor->FirstThunk);
			for(; thunk->u1.AddressOfData; ++thunk, ++bound)
				{
				const char* symbol = (thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG32)
					? reinterpret_cast<const char*>(MAKEINTRESOURCEA(IMAGE_ORDINAL32(thunk->u1.Ordinal)))
					: reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base + thunk->u1.AddressOfData)->Name;
				if(bound->u1.Function && GetProcAddress(imported, symbol))
					++resolved;
				else
					{
					printf("import_unresolved module=%s\n", name);
					++unresolved;
					}
				}
			}
		}

	printf("imports dlls=%u resolved=%u unresolved=%u\n", dlls, resolved, unresolved);
	if(unresolved)
		return nxFail("NxPhysics.dll has unresolved imports");
	return 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsExportTests", pairDirectory, &physics);
	if(status)
		return status;

	status = checkPe32(physics);
	if(!status)
		status = checkExports(physics);
	if(!status)
		status = checkImports(physics, pairDirectory);
	if(!status)
		status = nxReportPairIdentity(pairDirectory);

	FreeLibrary(physics);
	return status;
	}
