#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxUserAllocator.h"
#include "NxUserOutputStream.h"
#include "fluids/NxFluid.h"
#include "fluids/NxFluidDesc.h"

#ifndef NX_FLUID_MANAGER_FAIL_ORDINAL
#define NX_FLUID_MANAGER_FAIL_ORDINAL 1
#endif
#if NX_FLUID_MANAGER_FAIL_ORDINAL == 1
#define NX_FLUID_EXPECT_INTERNAL_COUNT 2
#define NX_FLUID_EXPECT_PUBLIC_COUNT 2
#elif NX_FLUID_MANAGER_FAIL_ORDINAL == 2
#define NX_FLUID_EXPECT_INTERNAL_COUNT 3
#define NX_FLUID_EXPECT_PUBLIC_COUNT 2
#else
#error Unsupported fluid-manager allocation failure ordinal
#endif

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

class NxFluidGrowthFailureOutput : public NxUserOutputStream
	{
	public:
	void reportError(NxErrorCode, const char*, const char*, int) {}
	NxAssertResponse reportAssertViolation(const char*, const char*, int)
		{ return NX_AR_CONTINUE; }
	void print(const char*) {}
	};

class NxFluidGrowthFailureLicense
	{
	public:
	NxFluidGrowthFailureLicense()
		: mRoot(0), mProduct(0), mRootCreated(false), mOverridden(false), mReady(false)
		{
		mRootPath[0] = 0;
		sprintf_s(mRootPath, sizeof(mRootPath), "Software\\NxFluidArrayOom-%lu-%lu",
			static_cast<unsigned long>(GetCurrentProcessId()),
			static_cast<unsigned long>(GetTickCount()));
		DWORD disposition = 0;
		if(RegCreateKeyExA(HKEY_CURRENT_USER, mRootPath, 0, 0, 0, KEY_ALL_ACCESS,
			0, &mRoot, &disposition) != ERROR_SUCCESS ||
			disposition != REG_CREATED_NEW_KEY)
			return;
		mRootCreated = true;
		if(RegOverridePredefKey(HKEY_LOCAL_MACHINE, mRoot) != ERROR_SUCCESS)
			return;
		mOverridden = true;
		if(RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\NovodexSDK212", 0,
			0, 0, KEY_ALL_ACCESS, 0, &mProduct, &disposition) != ERROR_SUCCESS)
			return;
		const char fluidKey[] = "a-804-:";
		if(RegSetValueExA(mProduct, "FluidKey", 0, REG_SZ,
			reinterpret_cast<const BYTE*>(fluidKey), sizeof(fluidKey)) != ERROR_SUCCESS)
			return;
		HKEY verify = 0;
		char readback[32] = {};
		DWORD type = 0;
		DWORD size = sizeof(readback);
		mReady = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\NovodexSDK212", 0,
			KEY_QUERY_VALUE, &verify) == ERROR_SUCCESS &&
			RegQueryValueExA(verify, "FluidKey", 0, &type,
				reinterpret_cast<BYTE*>(readback), &size) == ERROR_SUCCESS &&
			type == REG_SZ && strcmp(readback, fluidKey) == 0;
		if(verify)
			RegCloseKey(verify);
		}
	~NxFluidGrowthFailureLicense()
		{
		if(mProduct)
			RegCloseKey(mProduct);
		if(mOverridden)
			RegOverridePredefKey(HKEY_LOCAL_MACHINE, 0);
		if(mRoot)
			RegCloseKey(mRoot);
		if(mRootCreated)
			RegDeleteTreeA(HKEY_CURRENT_USER, mRootPath);
		}
	bool ready() const { return mReady; }
	private:
	HKEY mRoot;
	HKEY mProduct;
	bool mRootCreated;
	bool mOverridden;
	bool mReady;
	char mRootPath[128];
	};

class NxFluidGrowthFailureAllocator : public NxUserAllocator
	{
	public:
	NxFluidGrowthFailureAllocator(NxUserAllocator& inner, size_t failSize)
		: mInner(inner), mFailSize(failSize), mFailCount(0), mMatchingCount(0) {}
	void* mallocDEBUG(size_t size, const char* file, int line) override
		{ return fail(size) ? 0 : mInner.mallocDEBUG(size, file, line); }
	void* malloc(size_t size) override
		{ return fail(size) ? 0 : mInner.malloc(size); }
	void* realloc(void* memory, size_t size) override
		{ return mInner.realloc(memory, size); }
	void free(void* memory) override { mInner.free(memory); }
	unsigned failCount() const { return mFailCount; }
	private:
	bool fail(size_t size)
		{
		if(size != mFailSize)
			return false;
		++mMatchingCount;
		if(mMatchingCount != NX_FLUID_MANAGER_FAIL_ORDINAL)
			return false;
		++mFailCount;
		return true;
		}
	NxUserAllocator& mInner;
	size_t mFailSize;
	unsigned mFailCount;
	unsigned mMatchingCount;
	};

class NxFluidSingleFailureAllocator : public NxUserAllocator
	{
	public:
	NxFluidSingleFailureAllocator(NxUserAllocator& inner, size_t failSize)
		: mInner(inner), mFailSize(failSize), mFailCount(0) {}
	void* mallocDEBUG(size_t size, const char* file, int line) override
		{ return fail(size) ? 0 : mInner.mallocDEBUG(size, file, line); }
	void* malloc(size_t size) override
		{ return fail(size) ? 0 : mInner.malloc(size); }
	void* realloc(void* memory, size_t size) override
		{ return mInner.realloc(memory, size); }
	void free(void* memory) override { mInner.free(memory); }
	unsigned failCount() const { return mFailCount; }
	private:
	bool fail(size_t size)
		{
		if(size == mFailSize && !mFailCount)
			{
			++mFailCount;
			return true;
			}
		return false;
		}
	NxUserAllocator& mInner;
	size_t mFailSize;
	unsigned mFailCount;
	};

typedef void* (__cdecl *NxFluidMallocFn)(size_t);
static NxFluidMallocFn gNxFluidSolverOriginalMalloc = 0;
static unsigned gNxFluidSolverMallocFailures = 0;

static void* __cdecl nxFluidSolverFailureMalloc(size_t size)
	{
	if(size == 0x18)
		{
		++gNxFluidSolverMallocFailures;
		return 0;
		}
	return gNxFluidSolverOriginalMalloc(size);
	}

struct NxFluidSolverFailurePatch
	{
	void** importSlot;
	void* originalImport;
	unsigned char* code;
	unsigned char originalCode[5];
	bool oracleCodePatch;
	};

static bool nxInstallFluidSolverAllocationFailure(HMODULE physics,
	NxFluidSolverFailurePatch* patch)
	{
	memset(patch, 0, sizeof(*patch));
	gNxFluidSolverMallocFailures = 0;
	unsigned char* const image = reinterpret_cast<unsigned char*>(physics);
	IMAGE_DOS_HEADER* const dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
	if(dos->e_magic != IMAGE_DOS_SIGNATURE)
		return false;
	IMAGE_NT_HEADERS32* const nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(
		image + dos->e_lfanew);
	if(nt->Signature != IMAGE_NT_SIGNATURE ||
		nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return false;
	const IMAGE_DATA_DIRECTORY imports =
		nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	IMAGE_IMPORT_DESCRIPTOR* descriptor = imports.VirtualAddress
		? reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image + imports.VirtualAddress) : 0;
	for(; descriptor && descriptor->Name; ++descriptor)
		{
		IMAGE_THUNK_DATA32* const names = reinterpret_cast<IMAGE_THUNK_DATA32*>(
			image + (descriptor->OriginalFirstThunk
				? descriptor->OriginalFirstThunk : descriptor->FirstThunk));
		IMAGE_THUNK_DATA32* const slots = reinterpret_cast<IMAGE_THUNK_DATA32*>(
			image + descriptor->FirstThunk);
		for(unsigned index = 0; names[index].u1.AddressOfData; ++index)
			{
			if(IMAGE_SNAP_BY_ORDINAL32(names[index].u1.Ordinal))
				continue;
			IMAGE_IMPORT_BY_NAME* const imported = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
				image + names[index].u1.AddressOfData);
			if(strcmp(reinterpret_cast<const char*>(imported->Name), "malloc") != 0)
				continue;
			DWORD oldProtect = 0;
			void** const slot = reinterpret_cast<void**>(&slots[index].u1.Function);
			if(!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &oldProtect))
				return false;
			patch->importSlot = slot;
			patch->originalImport = *slot;
			gNxFluidSolverOriginalMalloc = reinterpret_cast<NxFluidMallocFn>(*slot);
			*slot = reinterpret_cast<void*>(&nxFluidSolverFailureMalloc);
			DWORD ignored = 0;
			VirtualProtect(slot, sizeof(*slot), oldProtect, &ignored);
			return true;
			}
		}

	// The shipped binary routes this one 0x18-byte process-heap allocation
	// through its local allocator thunk. Replace only that call's return value.
	patch->code = image + 0x85b6c;
	if(patch->code[0] != 0xe8)
		return false;
	LONG displacement = *reinterpret_cast<LONG*>(patch->code + 1);
	if(patch->code + 5 + displacement != image + 0xf48c5)
		return false;
	DWORD oldProtect = 0;
	if(!VirtualProtect(patch->code, sizeof(patch->originalCode), PAGE_EXECUTE_READWRITE,
		&oldProtect))
		return false;
	memcpy(patch->originalCode, patch->code, sizeof(patch->originalCode));
	const unsigned char returnNull[5] = { 0x33, 0xc0, 0x90, 0x90, 0x90 };
	memcpy(patch->code, returnNull, sizeof(returnNull));
	FlushInstructionCache(GetCurrentProcess(), patch->code,
		sizeof(patch->originalCode));
	DWORD ignored = 0;
	VirtualProtect(patch->code, sizeof(patch->originalCode), oldProtect, &ignored);
	patch->oracleCodePatch = true;
	return true;
	}

static void nxRemoveFluidSolverAllocationFailure(NxFluidSolverFailurePatch* patch)
	{
	if(patch->importSlot)
		{
	DWORD oldProtect = 0;
		if(VirtualProtect(patch->importSlot, sizeof(void*), PAGE_READWRITE, &oldProtect))
			{
			*patch->importSlot = patch->originalImport;
			DWORD ignored = 0;
			VirtualProtect(patch->importSlot, sizeof(void*), oldProtect, &ignored);
			}
		}
	if(patch->oracleCodePatch)
		{
		DWORD oldProtect = 0;
		if(VirtualProtect(patch->code, sizeof(patch->originalCode),
			PAGE_EXECUTE_READWRITE, &oldProtect))
			{
			memcpy(patch->code, patch->originalCode, sizeof(patch->originalCode));
			FlushInstructionCache(GetCurrentProcess(), patch->code,
				sizeof(patch->originalCode));
			DWORD ignored = 0;
			VirtualProtect(patch->code, sizeof(patch->originalCode), oldProtect,
				&ignored);
			}
		}
	}

static NxFluid* nxCreateFluidWithSeh(NxScene* scene, const NxFluidDesc* desc,
	unsigned* exceptionCode)
	{
	*exceptionCode = 0;
	__try
		{
		return scene->createFluid(*desc);
		}
	__except(EXCEPTION_EXECUTE_HANDLER)
		{
		*exceptionCode = GetExceptionCode();
		return 0;
		}
	}

static unsigned nxFluidArrayCount(const unsigned char* manager, unsigned offset)
	{
	const unsigned* const header = reinterpret_cast<const unsigned*>(manager + offset);
	if(header[0] == 0 || header[1] < header[0])
		return 0;
	return (header[1] - header[0]) / sizeof(unsigned);
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int openStatus = nxOpenPair(argc, argv,
		"NxPhysicsFluidManagerGrowthFailureTests", pairDirectory, &physics);
	if(openStatus)
		return openStatus;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		return nxFail("NxCreatePhysicsSDK is missing");
	NxFluidGrowthFailureOutput output;
	NxPhysicsSDK* const sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, &output);
	if(!sdk)
		return nxFail("SDK creation failed");
	NxFluidGrowthFailureLicense license;
	if(!license.ready())
		return nxFail("FluidKey override could not be installed");
	wchar_t fluidModelPath[MAX_PATH];
	if(swprintf_s(fluidModelPath, L"%s\\FluidModel.dll", pairDirectory) < 0)
		return nxFail("FluidModel path construction failed");
	HMODULE fluidModel = LoadLibraryExW(fluidModelPath, 0,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!fluidModel)
		return nxFail("staged FluidModel callback fixture failed to load");

	// The manager starts with its static-collision refresh dirty. The shipped
	// constructor continues into that refresh after a failed 0x84-byte fluid
	// allocation and faults when it reads the backend handle through the null
	// fluid. Keep this first-create case isolated from the growth OOM below.
	NxSceneDesc firstFailureSceneDesc;
	firstFailureSceneDesc.setToDefault();
	NxScene* const firstFailureScene = sdk->createScene(firstFailureSceneDesc);
	if(!firstFailureScene)
		return nxFail("first-fluid allocation-failure scene creation failed");
	NxUserAllocator** const firstFailureAllocatorSlot = reinterpret_cast<NxUserAllocator**>(
		GetProcAddress(GetModuleHandleW(L"NxFoundation.dll"),
			"?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A"));
	if(!firstFailureAllocatorSlot || !*firstFailureAllocatorSlot)
		return nxFail("Foundation allocator hook is missing");
	NxFluidDesc firstFailureDesc;
	firstFailureDesc.setToDefault();
	NxUserAllocator* const firstFailureSavedAllocator = *firstFailureAllocatorSlot;
	NxFluidSingleFailureAllocator firstFailureAllocator(*firstFailureSavedAllocator, 0x84);
	*firstFailureAllocatorSlot = &firstFailureAllocator;
	unsigned firstFailureException = 0;
	NxFluid* const firstFailureResult = nxCreateFluidWithSeh(firstFailureScene,
		&firstFailureDesc, &firstFailureException);
	*firstFailureAllocatorSlot = firstFailureSavedAllocator;
	unsigned char* const firstFailureSceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(firstFailureScene) + 0x24);
	unsigned char* const firstFailureManager = firstFailureSceneInternal
		? static_cast<unsigned char*>(*reinterpret_cast<void**>(
			firstFailureSceneInternal + 0x61c)) : 0;
	const bool firstFluidAllocationFailureExact = !firstFailureResult &&
		firstFailureException == EXCEPTION_ACCESS_VIOLATION &&
		firstFailureAllocator.failCount() == 1 && firstFailureManager &&
		firstFailureManager[0x29] == 1;
	printf("fluid first-create oom dirty-refresh caught=%u exception=%08x allocations=%u dirty=%u returned=%u\n",
		firstFailureException == EXCEPTION_ACCESS_VIOLATION ? 1u : 0u,
		firstFailureException, firstFailureAllocator.failCount(),
		firstFailureManager ? firstFailureManager[0x29] : 0,
		firstFailureResult ? 1u : 0u);
	if(!firstFluidAllocationFailureExact)
		return nxFail("first-fluid allocation failure did not match the dirty-refresh oracle path");
	sdk->releaseScene(*firstFailureScene);

	// The shipped constructor tolerates a null solver allocation, but the
	// subsequent fluid initialization writes userData through solver+4. Fail
	// only the 0x18-byte process-heap allocation and observe that fault before
	// the new fluid is published into either manager array.
	NxSceneDesc solverFailureSceneDesc;
	solverFailureSceneDesc.setToDefault();
	NxScene* const solverFailureScene = sdk->createScene(solverFailureSceneDesc);
	if(!solverFailureScene)
		return nxFail("solver-allocation-failure scene creation failed");
	NxFluidDesc solverFailureDesc;
	solverFailureDesc.setToDefault();
	NxFluid* const solverFailureSeed = solverFailureScene->createFluid(
		solverFailureDesc);
	if(!solverFailureSeed)
		return nxFail("solver-allocation-failure seed fluid creation failed");
	unsigned char* const solverFailureSceneInternal =
		*reinterpret_cast<unsigned char**>(
			reinterpret_cast<unsigned char*>(solverFailureScene) + 0x24);
	unsigned char* const solverFailureManager = solverFailureSceneInternal
		? static_cast<unsigned char*>(*reinterpret_cast<void**>(
			solverFailureSceneInternal + 0x61c)) : 0;
	NxFluidSolverFailurePatch solverFailurePatch;
	if(!solverFailureManager ||
		!nxInstallFluidSolverAllocationFailure(physics, &solverFailurePatch))
		return nxFail("solver allocation failure could not be injected");
	unsigned solverFailureException = 0;
	NxFluid* const solverFailureResult = nxCreateFluidWithSeh(
		solverFailureScene, &solverFailureDesc, &solverFailureException);
	nxRemoveFluidSolverAllocationFailure(&solverFailurePatch);
	const unsigned solverFailureAllocations = solverFailurePatch.oracleCodePatch
		? 1 : gNxFluidSolverMallocFailures;
	const unsigned solverFailureInternalCount = nxFluidArrayCount(
		solverFailureManager, 4);
	const unsigned solverFailurePublicCount = nxFluidArrayCount(
		solverFailureManager, 0x14);
	const bool solverFailureExact = !solverFailureResult &&
		solverFailureException == EXCEPTION_ACCESS_VIOLATION &&
		solverFailureAllocations == 1 &&
		solverFailureInternalCount == 1 && solverFailurePublicCount == 1;
	printf("fluid solver-allocation-failure caught=%u exception=%08x allocations=%u internal=%u public=%u returned=%u\n",
		solverFailureException == EXCEPTION_ACCESS_VIOLATION ? 1u : 0u,
		solverFailureException, solverFailureAllocations,
		solverFailureInternalCount, solverFailurePublicCount,
		solverFailureResult ? 1u : 0u);
	if(!solverFailureExact)
		return nxFail("solver allocation failure differs from the oracle initialization path");
	sdk->releaseScene(*solverFailureScene);

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* const scene = sdk->createScene(sceneDesc);
	if(!scene)
		return nxFail("scene creation failed");
	NxFluidDesc fluidDesc;
	fluidDesc.setToDefault();
	NxFluid* const first = scene->createFluid(fluidDesc);
	NxFluid* const second = scene->createFluid(fluidDesc);
	unsigned char* const sceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(scene) + 0x24);
	unsigned char* const manager = sceneInternal
		? static_cast<unsigned char*>(*reinterpret_cast<void**>(sceneInternal + 0x61c))
		: 0;
	if(!first || !second || !manager ||
		nxFluidArrayCount(manager, 4) != 2 || nxFluidArrayCount(manager, 0x14) != 2)
		return nxFail("two-fluid precondition did not establish both capacity-2 arrays");
	// Avoid re-entering the unrelated initial dirty collision-builder path on the
	// failing third insertion; the oracle trace shows the failing 24-byte calls
	// are the manager's parallel fluid-array growth allocations.
	manager[0x29] = 0;

	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	NxUserAllocator** const allocatorSlot = foundation
		? reinterpret_cast<NxUserAllocator**>(GetProcAddress(foundation,
			"?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A")) : 0;
	if(!allocatorSlot || !*allocatorSlot)
		return nxFail("Foundation SDK allocator hook is missing");
	NxUserAllocator* const savedAllocator = *allocatorSlot;
	NxFluidGrowthFailureAllocator failGrowth(*savedAllocator, 24);
	*allocatorSlot = &failGrowth;
	unsigned exceptionCode = 0;
	NxFluid* const third = nxCreateFluidWithSeh(scene, &fluidDesc, &exceptionCode);
	*allocatorSlot = savedAllocator;
	const unsigned internalCount = nxFluidArrayCount(manager, 4);
	const unsigned publicCount = nxFluidArrayCount(manager, 0x14);
	const bool oracleFailureContract = !third &&
		exceptionCode == EXCEPTION_ACCESS_VIOLATION && failGrowth.failCount() == 1 &&
		internalCount == NX_FLUID_EXPECT_INTERNAL_COUNT &&
		publicCount == NX_FLUID_EXPECT_PUBLIC_COUNT;
	printf("fluid manager-array-growth oom ordinal=%u caught=%u exception=%08x allocations=%u internal=%u public=%u returned=%u\n",
		static_cast<unsigned>(NX_FLUID_MANAGER_FAIL_ORDINAL),
		exceptionCode == EXCEPTION_ACCESS_VIOLATION ? 1u : 0u,
		exceptionCode, failGrowth.failCount(), internalCount, publicCount,
		third ? 1u : 0u);
	const int identityStatus = nxReportPairIdentity(pairDirectory);
	if(identityStatus)
		return identityStatus;
	// The oracle faults partway through publication. Deliberately end this
	// isolated process without running scene teardown over partial state.
	return oracleFailureContract ? 0 : nxFail("fluid-array OOM ordering differs from the oracle");
	}
