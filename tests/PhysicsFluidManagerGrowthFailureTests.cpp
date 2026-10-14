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
