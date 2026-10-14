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

	NxU32 particleCount = NX_FLUID_ZERO_INITIAL_PARTICLES ? 0 : 2;
	NxF32 positions[8] = { 1.0f, -2.5f, 3.5f, 91.0f,
		-1.5f, 2.0f, 4.0f, 92.0f };
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
	#if NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE || NX_FLUID_GET_PARTICLES_NULL_FAILURE || NX_FLUID_ZERO_INITIAL_PARTICLES
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
	#if NX_FLUID_PARTICLE_ARRAY_ALLOC_FAILURE || NX_FLUID_GET_PARTICLES_NULL_FAILURE || NX_FLUID_ZERO_INITIAL_PARTICLES
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
	#else
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
	#endif
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
