#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxUserOutputStream.h"
#include "fluids/NxFluid.h"
#include "fluids/NxFluidDesc.h"

#ifndef NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE
#define NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE 0
#endif
#ifndef NX_FLUID_GET_PARTICLES_NULL_FAILURE
#define NX_FLUID_GET_PARTICLES_NULL_FAILURE 0
#endif
#ifndef NX_FLUID_ZERO_INITIAL_PARTICLES
#define NX_FLUID_ZERO_INITIAL_PARTICLES 0
#endif
#ifndef NX_FLUID_INITIAL_PARTICLE_CAPACITY_CLIP
#define NX_FLUID_INITIAL_PARTICLE_CAPACITY_CLIP 0
#endif
#ifndef NX_FLUID_ADD_PARTICLES_TEST
#define NX_FLUID_ADD_PARTICLES_TEST 0
#endif
#ifndef NX_FLUID_ADD_PARTICLES_GROW_FAILURE
#define NX_FLUID_ADD_PARTICLES_GROW_FAILURE 0
#endif
#ifndef NX_FLUID_ADD_PARTICLES_APPEND_TEST
#define NX_FLUID_ADD_PARTICLES_APPEND_TEST 0
#endif
#ifndef NX_FLUID_CELL_ROUNDING_TEST
#define NX_FLUID_CELL_ROUNDING_TEST 0
#endif
#ifndef NX_FLUID_NULL_COUNT_TEST
#define NX_FLUID_NULL_COUNT_TEST 0
#endif
#ifndef NX_FLUID_CELL_SIZE_ZERO_TEST
#define NX_FLUID_CELL_SIZE_ZERO_TEST 0
#endif
#ifndef NX_FLUID_CELL_RECIPROCAL_TEST
#define NX_FLUID_CELL_RECIPROCAL_TEST 0
#endif
#if NX_FLUID_ADD_PARTICLES_GROW_FAILURE && !NX_FLUID_ADD_PARTICLES_TEST
#undef NX_FLUID_ADD_PARTICLES_TEST
#define NX_FLUID_ADD_PARTICLES_TEST 1
#endif
#if NX_FLUID_ADD_PARTICLES_APPEND_TEST && !NX_FLUID_ADD_PARTICLES_TEST
#undef NX_FLUID_ADD_PARTICLES_TEST
#define NX_FLUID_ADD_PARTICLES_TEST 1
#endif

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

class NxFluidInitialParticleOutput : public NxUserOutputStream
	{
	public:
	void reportError(NxErrorCode code, const char* message, const char*, int line)
		{
		printf("fluid initial-particles error code=%u line=%d message=%s\n",
			static_cast<unsigned>(code), line, message ? message : "null");
		}
	NxAssertResponse reportAssertViolation(const char*, const char*, int)
		{
		return NX_AR_CONTINUE;
		}
	void print(const char*) {}
	};

class NxFluidInitialParticleLicense
	{
	public:
	NxFluidInitialParticleLicense()
		: mRoot(0), mProduct(0), mRootCreated(false), mOverridden(false), mReady(false)
		{
		mRootPath[0] = 0;
		sprintf_s(mRootPath, sizeof(mRootPath), "Software\\NxFluidInitialParticles-%lu-%lu",
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
		char fluidKey[32] = {};
		sprintf_s(fluidKey, sizeof(fluidKey), "a-804-:");
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
	~NxFluidInitialParticleLicense()
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

static unsigned nxFluidInitialParticleBits(float value)
	{
	unsigned bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
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

static void nxAddFluidParticlesWithSeh(NxFluid* fluid, const NxParticleData* data,
	unsigned* exceptionCode)
	{
	*exceptionCode = 0;
	__try
		{
		fluid->addParticles(*data);
		}
	__except(EXCEPTION_EXECUTE_HANDLER)
		{
		*exceptionCode = GetExceptionCode();
		}
	}

static unsigned nxFluidInitialParticleArrayCount(const unsigned char* manager,
	unsigned offset)
	{
	const unsigned* const header = reinterpret_cast<const unsigned*>(manager + offset);
	return header[0] && header[1] >= header[0]
		? (header[1] - header[0]) / sizeof(unsigned) : 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	#if NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE
	const char* const testName = "NxPhysicsFluidParticleArrayFailureTests";
	#elif NX_FLUID_GET_PARTICLES_NULL_FAILURE
	const char* const testName = "NxPhysicsFluidGetParticlesNullTests";
	#elif NX_FLUID_ZERO_INITIAL_PARTICLES
	const char* const testName = "NxPhysicsFluidZeroInitialParticlesTests";
	#elif NX_FLUID_INITIAL_PARTICLE_CAPACITY_CLIP
	const char* const testName = "NxPhysicsFluidInitialParticleCapacityClipTests";
	#elif NX_FLUID_NULL_COUNT_TEST
	const char* const testName = "NxPhysicsFluidNullParticleCountTests";
	#elif NX_FLUID_CELL_SIZE_ZERO_TEST
	const char* const testName = "NxPhysicsFluidCellSizeZeroTests";
	#elif NX_FLUID_CELL_RECIPROCAL_TEST
	const char* const testName = "NxPhysicsFluidCellReciprocalTests";
	#elif NX_FLUID_ADD_PARTICLES_TEST
	const char* const testName = "NxPhysicsFluidAddParticlesTests";
	#elif NX_FLUID_CELL_ROUNDING_TEST
	const char* const testName = "NxPhysicsFluidCellRoundingTests";
	#else
	const char* const testName = "NxPhysicsFluidInitialParticlesTests";
	#endif
	int status = nxOpenPair(argc, argv,
		testName, pairDirectory, &physics);
	if(status)
		return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		return nxFail("NxCreatePhysicsSDK is missing");
	NxFluidInitialParticleOutput output;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, &output);
	if(!sdk)
		return nxFail("SDK creation failed");
	wchar_t fluidModelPath[MAX_PATH];
	if(swprintf_s(fluidModelPath, L"%s\\FluidModel.dll", pairDirectory) < 0)
		{
		sdk->release();
		return nxFail("fluid model fixture path construction failed");
		}
	HMODULE fluidModel = LoadLibraryExW(fluidModelPath, 0,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!fluidModel)
		{
		sdk->release();
	return nxFail("staged fluid model callback fixture failed to load");
		}
	#if NX_FLUID_CELL_SIZE_ZERO_TEST
	typedef void (__cdecl *SetFluidCellSizeZeroFn)(unsigned);
	SetFluidCellSizeZeroFn setFluidCellSizeZero =
		reinterpret_cast<SetFluidCellSizeZeroFn>(GetProcAddress(fluidModel,
			"NxSetFluidCellSizeZero"));
	if(!setFluidCellSizeZero)
		{
		sdk->release();
		return nxFail("cell-size-zero fixture control is missing");
		}
	typedef void (__cdecl *SetFluidParticleCellPoisonFn)(unsigned);
	SetFluidParticleCellPoisonFn setFluidParticleCellPoison =
		reinterpret_cast<SetFluidParticleCellPoisonFn>(GetProcAddress(fluidModel,
			"NxSetFluidParticleCellPoison"));
	if(!setFluidParticleCellPoison)
		{
		sdk->release();
		return nxFail("particle cell poison control is missing");
		}
	setFluidCellSizeZero(1);
	setFluidParticleCellPoison(1);
	#elif NX_FLUID_CELL_RECIPROCAL_TEST
	typedef void (__cdecl *SetFluidCellSizeFn)(float);
	SetFluidCellSizeFn setFluidCellSize =
		reinterpret_cast<SetFluidCellSizeFn>(GetProcAddress(fluidModel,
			"NxSetFluidCellSize"));
	if(!setFluidCellSize)
		{
		sdk->release();
		return nxFail("custom cell-size fixture control is missing");
		}
	setFluidCellSize(21.575782775878906f);
	#endif
	typedef void (__cdecl *SetFluidPostStepParticleFn)(unsigned,
		const unsigned*, unsigned);
	SetFluidPostStepParticleFn setFluidPostStepParticle =
		reinterpret_cast<SetFluidPostStepParticleFn>(GetProcAddress(fluidModel,
			"NxSetFluidPostStepParticle"));
	if(!setFluidPostStepParticle)
		{
		sdk->release();
		return nxFail("fluid particle stream fixture control is missing");
		}
	const unsigned emptyParticleWords[23] = {};
	setFluidPostStepParticle(0, emptyParticleWords, 0);
	#if NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE
	typedef void (__cdecl *SetParticleArrayAllocFailureFn)(unsigned);
	typedef unsigned (__cdecl *GetParticleArrayAllocCallsFn)();
	SetParticleArrayAllocFailureFn setParticleArrayAllocFailure =
		reinterpret_cast<SetParticleArrayAllocFailureFn>(GetProcAddress(fluidModel,
			"NxSetFluidParticleArrayAllocFailure"));
	GetParticleArrayAllocCallsFn getParticleArrayAllocCalls =
		reinterpret_cast<GetParticleArrayAllocCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidParticleArrayAllocCalls"));
	if(!setParticleArrayAllocFailure || !getParticleArrayAllocCalls)
		{
		sdk->release();
		return nxFail("fluid particle-array allocation controls are missing");
		}
	setParticleArrayAllocFailure(1);
	#elif NX_FLUID_GET_PARTICLES_NULL_FAILURE
	typedef void (__cdecl *SetFluidGetParticlesNullFn)(unsigned);
	typedef unsigned (__cdecl *GetFluidGetParticlesCallsFn)();
	typedef unsigned (__cdecl *GetParticleArrayAllocCallsFn)();
	SetFluidGetParticlesNullFn setFluidGetParticlesNull =
		reinterpret_cast<SetFluidGetParticlesNullFn>(GetProcAddress(fluidModel,
			"NxSetFluidGetParticlesNull"));
	GetFluidGetParticlesCallsFn getFluidGetParticlesCalls =
		reinterpret_cast<GetFluidGetParticlesCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidGetParticlesCalls"));
	GetParticleArrayAllocCallsFn getParticleArrayAllocCalls =
		reinterpret_cast<GetParticleArrayAllocCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidParticleArrayAllocCalls"));
	if(!setFluidGetParticlesNull || !getFluidGetParticlesCalls ||
		!getParticleArrayAllocCalls)
		{
		sdk->release();
		return nxFail("FluidGetParticles-null controls are missing");
		}
	setFluidGetParticlesNull(1);
	#elif NX_FLUID_INITIAL_PARTICLE_CAPACITY_CLIP
	typedef void (__cdecl *SetFluidMaxParticlesFn)(unsigned);
	typedef unsigned (__cdecl *GetFluidGetParticlesCallsFn)();
	typedef unsigned (__cdecl *GetParticleArrayAllocCallsFn)();
	SetFluidMaxParticlesFn setFluidMaxParticles =
		reinterpret_cast<SetFluidMaxParticlesFn>(GetProcAddress(fluidModel,
			"NxSetFluidMaxParticles"));
	GetFluidGetParticlesCallsFn getFluidGetParticlesCalls =
		reinterpret_cast<GetFluidGetParticlesCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidGetParticlesCalls"));
	GetParticleArrayAllocCallsFn getParticleArrayAllocCalls =
		reinterpret_cast<GetParticleArrayAllocCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidParticleArrayAllocCalls"));
	if(!setFluidMaxParticles || !getFluidGetParticlesCalls ||
		!getParticleArrayAllocCalls)
		{
		sdk->release();
		return nxFail("particle-capacity controls are missing");
		}
	setFluidMaxParticles(1);
	#elif NX_FLUID_ADD_PARTICLES_TEST
	typedef unsigned (__cdecl *GetFluidCallbackCallsFn)();
	typedef unsigned (__cdecl *GetParticleArrayCallsFn)();
	GetFluidCallbackCallsFn getFluidGetParticlesCalls =
		reinterpret_cast<GetFluidCallbackCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidGetParticlesCalls"));
	GetParticleArrayCallsFn getParticleArrayAllocCalls =
		reinterpret_cast<GetParticleArrayCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidParticleArrayAllocCalls"));
	GetParticleArrayCallsFn getParticleArrayGrowCalls =
		reinterpret_cast<GetParticleArrayCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidParticleArrayGrowCalls"));
	#if NX_FLUID_ADD_PARTICLES_GROW_FAILURE
	typedef void (__cdecl *SetParticleArrayGrowFailureFn)(unsigned);
	SetParticleArrayGrowFailureFn setParticleArrayGrowFailure =
		reinterpret_cast<SetParticleArrayGrowFailureFn>(GetProcAddress(fluidModel,
			"NxSetFluidParticleArrayGrowFailure"));
	if(!setParticleArrayGrowFailure)
		{
		sdk->release();
		return nxFail("ParticleArrayGrow failure control is missing");
		}
	#else
	void (__cdecl *setParticleArrayGrowFailure)(unsigned) = 0;
	#endif
	if(!getFluidGetParticlesCalls || !getParticleArrayAllocCalls ||
		!getParticleArrayGrowCalls)
		{
		sdk->release();
		return nxFail("addParticles callback counters are missing");
		}
	#elif NX_FLUID_ZERO_INITIAL_PARTICLES
	typedef unsigned (__cdecl *GetFluidGetParticlesCallsFn)();
	typedef unsigned (__cdecl *GetParticleArrayAllocCallsFn)();
	GetFluidGetParticlesCallsFn getFluidGetParticlesCalls =
		reinterpret_cast<GetFluidGetParticlesCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidGetParticlesCalls"));
	GetParticleArrayAllocCallsFn getParticleArrayAllocCalls =
		reinterpret_cast<GetParticleArrayAllocCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidParticleArrayAllocCalls"));
	if(!getFluidGetParticlesCalls || !getParticleArrayAllocCalls)
		{
		sdk->release();
		return nxFail("zero-particle callback counters are missing");
		}
	#endif
	NxFluidInitialParticleLicense license;
	if(!license.ready())
		{
		sdk->release();
		return nxFail("fluid license override could not be installed");
		}
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		{
		sdk->release();
		return nxFail("fluid scene creation failed");
		}

	NxU32 particleCount = NX_FLUID_ZERO_INITIAL_PARTICLES ||
		(NX_FLUID_ADD_PARTICLES_TEST && !NX_FLUID_ADD_PARTICLES_APPEND_TEST) ? 0 : 2;
	NxF32 positions[8] = { 1.0f, -2.5f, 3.5f, 91.0f,
		-1.5f, 2.0f, 4.0f, 92.0f };
	#if NX_FLUID_CELL_ROUNDING_TEST
	positions[0] = 1.375f;
	positions[4] = -1.375f;
	#endif
	#if NX_FLUID_CELL_RECIPROCAL_TEST
	positions[0] = positions[1] = positions[2] = 1531.880615234375f;
	positions[4] = positions[5] = positions[6] = 1531.880615234375f;
	#endif
	NxF32 velocities[8] = { -4.5f, 5.25f, -6.75f, 93.0f,
		0.125f, -0.25f, 0.5f, 94.0f };
	NxF32 lifetimes[4] = { 8.5f, 95.0f, 9.25f, 96.0f };
	NxFluidDesc desc;
	desc.setToDefault();
	desc.initialParticleData.numParticlesPtr = &particleCount;
	desc.initialParticleData.bufferPos = positions;
	desc.initialParticleData.bufferVel = velocities;
	desc.initialParticleData.bufferLife = lifetimes;
	desc.initialParticleData.bufferPosByteStride = 4 * sizeof(NxF32);
	desc.initialParticleData.bufferVelByteStride = 4 * sizeof(NxF32);
	desc.initialParticleData.bufferLifeByteStride = 2 * sizeof(NxF32);
	const bool descValid = desc.isValid();
	unsigned exceptionCode = 0;
	#if NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE || NX_FLUID_GET_PARTICLES_NULL_FAILURE || NX_FLUID_ZERO_INITIAL_PARTICLES || NX_FLUID_INITIAL_PARTICLE_CAPACITY_CLIP
	NxFluid* fluid = nxCreateFluidWithSeh(scene, &desc, &exceptionCode);
	#else
	NxFluid* fluid = scene->createFluid(desc);
	#endif
	unsigned char* internal = fluid ? static_cast<unsigned char*>(
		*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(fluid) + 0x14)) : 0;
	unsigned char* array = internal ? static_cast<unsigned char*>(
		*reinterpret_cast<void**>(internal + 0x78)) : 0;
	unsigned char* records = array ? static_cast<unsigned char*>(
		*reinterpret_cast<void**>(array)) : 0;
	#if NX_FLUID_ADD_PARTICLES_TEST
	particleCount = 2;
	unsigned addParticlesException = 0;
	#if NX_FLUID_ADD_PARTICLES_GROW_FAILURE
	setParticleArrayGrowFailure(1);
	#endif
	#if NX_FLUID_ADD_PARTICLES_APPEND_TEST
	const unsigned existingParticleWords[46] = {};
	setFluidPostStepParticle(0, existingParticleWords, 2);
	#endif
	#if NX_FLUID_NULL_COUNT_TEST
	NxParticleData noCountData = desc.initialParticleData;
	noCountData.numParticlesPtr = 0;
	if(fluid)
		nxAddFluidParticlesWithSeh(fluid, &noCountData,
			&addParticlesException);
	#else
	if(fluid)
		nxAddFluidParticlesWithSeh(fluid, &desc.initialParticleData,
			&addParticlesException);
	#endif
	internal = fluid ? static_cast<unsigned char*>(
		*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(fluid) + 0x14)) : 0;
	array = internal ? static_cast<unsigned char*>(
		*reinterpret_cast<void**>(internal + 0x78)) : 0;
	records = array ? static_cast<unsigned char*>(
		*reinterpret_cast<void**>(array)) : 0;
	#endif
	#if NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE || NX_FLUID_GET_PARTICLES_NULL_FAILURE || NX_FLUID_ZERO_INITIAL_PARTICLES || NX_FLUID_INITIAL_PARTICLE_CAPACITY_CLIP || NX_FLUID_ADD_PARTICLES_TEST
	unsigned char* sceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(scene) + 0x24);
	unsigned char* manager = sceneInternal ? static_cast<unsigned char*>(
		*reinterpret_cast<void**>(sceneInternal + 0x61c)) : 0;
	const unsigned internalCount = manager
		? nxFluidInitialParticleArrayCount(manager, 4) : 0;
	const unsigned publicCount = manager
		? nxFluidInitialParticleArrayCount(manager, 0x14) : 0;
	#if NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE
	const bool failureContract = !fluid &&
		exceptionCode == EXCEPTION_ACCESS_VIOLATION && manager &&
		internalCount == 0 && publicCount == 0 &&
		getParticleArrayAllocCalls() == 1;
	printf("fluid initial-particles oom caught=%u exception=%08x returned=%u array=%u internal=%u public=%u allocs=%u\n",
		exceptionCode == EXCEPTION_ACCESS_VIOLATION ? 1u : 0u,
		exceptionCode, fluid ? 1u : 0u, array ? 1u : 0u,
		internalCount, publicCount, getParticleArrayAllocCalls());
	// The oracle faults in ParticleArray record initialization before either
	// manager array is updated; do not tear down partial constructor state.
	#elif NX_FLUID_GET_PARTICLES_NULL_FAILURE
	const bool failureContract = !fluid &&
		exceptionCode == EXCEPTION_ACCESS_VIOLATION && manager &&
		internalCount == 0 && publicCount == 0 &&
		getFluidGetParticlesCalls() == 1 && getParticleArrayAllocCalls() == 0;
	printf("fluid initial-particles get-particles-null caught=%u exception=%08x returned=%u internal=%u public=%u callbacks=%u array-allocs=%u\n",
		exceptionCode == EXCEPTION_ACCESS_VIOLATION ? 1u : 0u,
		exceptionCode, fluid ? 1u : 0u, internalCount, publicCount,
		getFluidGetParticlesCalls(), getParticleArrayAllocCalls());
	#endif
	#if NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE || NX_FLUID_GET_PARTICLES_NULL_FAILURE
	if(!descValid || !failureContract)
		return nxFail("initial-particle failure behavior differs from the oracle");
	return nxReportPairIdentity(pairDirectory);
	#elif NX_FLUID_ZERO_INITIAL_PARTICLES
	const unsigned particleArrayCount = array
		? *reinterpret_cast<unsigned*>(array + 4) : ~0u;
	const bool zeroParticleContract = fluid && array && particleArrayCount == 0 &&
		manager && internalCount == 1 && publicCount == 1 &&
		getFluidGetParticlesCalls() == 1 &&
		getParticleArrayAllocCalls() == 1;
	printf("fluid initial-particles zero returned=%u array=%u count=%u internal=%u public=%u callbacks=%u array-allocs=%u\n",
		fluid ? 1u : 0u, array ? 1u : 0u, particleArrayCount,
		internalCount, publicCount, getFluidGetParticlesCalls(),
		getParticleArrayAllocCalls());
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!descValid || !zeroParticleContract)
		return nxFail("zero-count initial-particle behavior differs from the oracle");
	return nxReportPairIdentity(pairDirectory);
	#elif NX_FLUID_ADD_PARTICLES_TEST
	#if NX_FLUID_NULL_COUNT_TEST
	const unsigned particleArrayCount = array
		? *reinterpret_cast<unsigned*>(array + 4) : ~0u;
	const bool nullCountContract = fluid && !addParticlesException &&
		particleArrayCount == 0 && getFluidGetParticlesCalls() == 1 &&
		getParticleArrayAllocCalls() == 1 && getParticleArrayGrowCalls() == 0;
	printf("fluid add-particles null-count no-op=%u array-count=%u callbacks=%u allocations=%u grows=%u\n",
		nullCountContract ? 1u : 0u, particleArrayCount,
		getFluidGetParticlesCalls(), getParticleArrayAllocCalls(),
		getParticleArrayGrowCalls());
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!descValid || !nullCountContract)
		return nxFail("null particle count did not leave the ParticleArray unchanged");
	return nxReportPairIdentity(pairDirectory);
	#elif NX_FLUID_ADD_PARTICLES_GROW_FAILURE
	const unsigned particleArrayCount = array
		? *reinterpret_cast<unsigned*>(array + 4) : ~0u;
	const bool growFailureContract = fluid && addParticlesException ==
		EXCEPTION_ACCESS_VIOLATION && array && particleArrayCount == 0 &&
		manager && internalCount == 1 && publicCount == 1 &&
		getFluidGetParticlesCalls() == 2 &&
		getParticleArrayAllocCalls() == 1 &&
		getParticleArrayGrowCalls() == 1;
	printf("fluid add-particles grow-failure caught=%u exception=%08x count=%u "
		"callbacks=%u allocations=%u grows=%u manager=%u/%u\n",
		addParticlesException == EXCEPTION_ACCESS_VIOLATION ? 1u : 0u,
		addParticlesException, particleArrayCount,
		getFluidGetParticlesCalls(), getParticleArrayAllocCalls(),
		getParticleArrayGrowCalls(), internalCount, publicCount);
	// The oracle's access violation leaves its API reentry flag set. Process
	// exit is the faithful cleanup boundary; a teardown call would test a
	// separate reentry path and add unrelated output to this failure probe.
	if(!descValid || !growFailureContract)
		return nxFail("ParticleArrayGrow failure behavior differs from the oracle");
	return nxReportPairIdentity(pairDirectory);
	#elif NX_FLUID_ADD_PARTICLES_APPEND_TEST
	const unsigned appendedCount = array
		? *reinterpret_cast<unsigned*>(array + 4) : ~0u;
	const unsigned char* const appended = records ? records + 184 : 0;
	const bool originalPreserved = records &&
		nxFluidInitialParticleBits(*reinterpret_cast<const float*>(records + 4)) ==
			0x3f800000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<const float*>(records + 96)) ==
			0xbfc00000u;
	const bool appendedPositionsMatch = appended &&
		nxFluidInitialParticleBits(*reinterpret_cast<const float*>(appended + 4)) ==
			0x3f800000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<const float*>(appended + 8)) ==
			0xc0200000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<const float*>(appended + 12)) ==
			0x40600000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<const float*>(appended + 96)) ==
			0xbfc00000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<const float*>(appended + 100)) ==
			0x40000000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<const float*>(appended + 104)) ==
			0x40800000u;
	const bool appendContract = fluid && !addParticlesException &&
		appendedCount == 2 && originalPreserved && appendedPositionsMatch &&
		manager && internalCount == 1 && publicCount == 1 &&
		getFluidGetParticlesCalls() == 2 &&
		getParticleArrayAllocCalls() == 1 &&
		getParticleArrayGrowCalls() == 1;
	printf("fluid add-particles append caught=%u array-count=%u original-preserved=%u "
		"appended-positions=%u callbacks=%u allocations=%u grows=%u\n",
		addParticlesException ? 1u : 0u, appendedCount,
		originalPreserved ? 1u : 0u, appendedPositionsMatch ? 1u : 0u,
		getFluidGetParticlesCalls(), getParticleArrayAllocCalls(),
		getParticleArrayGrowCalls());
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!descValid || !appendContract)
		return nxFail("NxFluid::addParticles append behavior differs from the oracle");
	return nxReportPairIdentity(pairDirectory);
	#else
	const unsigned addedCount = array
		? *reinterpret_cast<unsigned*>(array + 4) : ~0u;
	const bool addedPositionsMatch = records &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 4)) ==
			0x3f800000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 8)) ==
			0xc0200000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 12)) ==
			0x40600000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 96)) ==
			0xbfc00000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 100)) ==
			0x40000000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 104)) ==
			0x40800000u;
	const bool addParticlesContract = fluid && !addParticlesException &&
		addedCount == 2 && addedPositionsMatch &&
		manager && internalCount == 1 && publicCount == 1 &&
		getFluidGetParticlesCalls() == 2 &&
		getParticleArrayAllocCalls() == 1 &&
		getParticleArrayGrowCalls() == 1;
	printf("fluid add-particles caught=%u array-count=%u positions=%u callbacks=%u allocations=%u grows=%u\n",
		addParticlesException ? 1u : 0u, addedCount,
		addedPositionsMatch ? 1u : 0u, getFluidGetParticlesCalls(),
		getParticleArrayAllocCalls(), getParticleArrayGrowCalls());
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!descValid || !addParticlesContract)
		return nxFail("NxFluid::addParticles differs from the oracle");
	return nxReportPairIdentity(pairDirectory);
	#endif
	#elif NX_FLUID_INITIAL_PARTICLE_CAPACITY_CLIP
	const unsigned clippedCount = array
		? *reinterpret_cast<unsigned*>(array + 4) : ~0u;
	const bool clippedPositionMatch = records &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 4)) ==
			0x3f800000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 8)) ==
			0xc0200000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 12)) ==
			0x40600000u;
	const bool capacityClipContract = fluid && clippedCount == 1 &&
		clippedPositionMatch && getFluidGetParticlesCalls() == 1 &&
		getParticleArrayAllocCalls() == 1;
	printf("fluid initial-particles capacity-clip count=%u first-position=%u callbacks=%u array-allocs=%u\n",
		clippedCount == 1 ? 1u : 0u, clippedPositionMatch ? 1u : 0u,
		getFluidGetParticlesCalls(), getParticleArrayAllocCalls());
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!descValid || !capacityClipContract)
		return nxFail("initial-particle capacity clipping differs from the oracle");
	return nxReportPairIdentity(pairDirectory);
	#endif
	#elif NX_FLUID_CELL_ROUNDING_TEST
	const bool countMatch = array &&
		*reinterpret_cast<unsigned*>(array + 4) == 2;
	const bool cellsMatch = records &&
		*reinterpret_cast<short*>(records + 64) == 2 &&
		*reinterpret_cast<short*>(records + 156) == -3;
	printf("fluid initial-particle cell-rounding count=%u cells=%u x=%d,%d\n",
		countMatch ? 2u : 0u, cellsMatch ? 1u : 0u,
		records ? *reinterpret_cast<short*>(records + 64) : 0,
		records ? *reinterpret_cast<short*>(records + 156) : 0);
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!descValid || !countMatch || !cellsMatch)
		return nxFail("particle cell quantization differs from the oracle");
	return nxReportPairIdentity(pairDirectory);
	#elif NX_FLUID_CELL_SIZE_ZERO_TEST
	const bool countMatch = array &&
		*reinterpret_cast<unsigned*>(array + 4) == 2;
	const int x0 = records ? *reinterpret_cast<short*>(records + 64) : 999;
	const int y0 = records ? *reinterpret_cast<short*>(records + 66) : 999;
	const int z0 = records ? *reinterpret_cast<short*>(records + 68) : 999;
	const int x1 = records ? *reinterpret_cast<short*>(records + 156) : 999;
	const int y1 = records ? *reinterpret_cast<short*>(records + 158) : 999;
	const int z1 = records ? *reinterpret_cast<short*>(records + 160) : 999;
	const bool cellsMatch = records && x0 == 0 && y0 == 0 && z0 == 0 &&
		x1 == 0 && y1 == 0 && z1 == 0;
	printf("fluid initial-particles zero-cell-size count=%u cells=%d,%d,%d:%d,%d,%d\n",
		countMatch && cellsMatch ? 2u : 0u, x0, y0, z0, x1, y1, z1);
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!descValid || !countMatch || !cellsMatch)
		return nxFail("zero-cell-size particle array was not populated");
	return nxReportPairIdentity(pairDirectory);
	#elif NX_FLUID_CELL_RECIPROCAL_TEST
	const bool countMatch = array &&
		*reinterpret_cast<unsigned*>(array + 4) == 2;
	const int x = records ? *reinterpret_cast<short*>(records + 64) : 999;
	const int y = records ? *reinterpret_cast<short*>(records + 66) : 999;
	const int z = records ? *reinterpret_cast<short*>(records + 68) : 999;
	const bool cellsMatch = records && x == 70 && y == 70 && z == 70;
	printf("fluid initial-particles reciprocal-cell-scale count=%u cells=%d,%d,%d\n",
		countMatch && cellsMatch ? 2u : 0u, x, y, z);
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!descValid || !countMatch || !cellsMatch)
		return nxFail("reciprocal cell-size scaling differs from the oracle");
	return nxReportPairIdentity(pairDirectory);
	#else
	const bool countMatch = array &&
		*reinterpret_cast<unsigned*>(array + 4) == 2;
	const bool positionsMatch = records &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 4)) == 0x3f800000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 8)) == 0xc0200000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 12)) == 0x40600000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 96)) == 0xbfc00000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 100)) == 0x40000000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 104)) == 0x40800000u;
	const bool velocitiesMatch = records &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 16)) == 0xc0900000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 20)) == 0x40a80000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 24)) == 0xc0d80000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 108)) == 0x3e000000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 112)) == 0xbe800000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 116)) == 0x3f000000u;
	const bool lifetimesMatch = records &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 88)) == 0x41080000u &&
		nxFluidInitialParticleBits(*reinterpret_cast<float*>(records + 180)) == 0x41140000u;
	const bool cellsMatch = records &&
		*reinterpret_cast<short*>(records + 64) == 2 &&
		*reinterpret_cast<short*>(records + 66) == -5 &&
		*reinterpret_cast<short*>(records + 68) == 7 &&
		*reinterpret_cast<short*>(records + 70) == -1 &&
		*reinterpret_cast<short*>(records + 72) == -1 &&
		*reinterpret_cast<short*>(records + 156) == -3 &&
		*reinterpret_cast<short*>(records + 158) == 4 &&
		*reinterpret_cast<short*>(records + 160) == 8;
	printf("fluid initial-particles count=%u positions=%u velocities=%u lifetimes=%u cells=%u strides=%u\n",
		countMatch ? 2u : 0u,
		positionsMatch ? 1u : 0u, velocitiesMatch ? 1u : 0u,
		lifetimesMatch ? 1u : 0u, cellsMatch ? 1u : 0u,
		fluid && countMatch && positionsMatch && velocitiesMatch &&
			lifetimesMatch && cellsMatch ? 1u : 0u);
	if(fluid)
		scene->releaseFluid(*fluid);
	sdk->releaseScene(*scene);
	sdk->release();
	if(!fluid || !countMatch || !positionsMatch || !velocitiesMatch ||
		!lifetimesMatch || !cellsMatch)
		return nxFail("initial ParticleData channels were not copied into the internal ParticleArray");
	return nxReportPairIdentity(pairDirectory);
	#endif
	}
