#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxMaterial.h"
#include "NxSceneDesc.h"
#include "NxUserAllocator.h"
#include "NxUserOutputStream.h"
#include "../Physics/src/include/NpSceneGuard.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

class RecordingAllocator : public NxUserAllocator
	{
	public:
	RecordingAllocator(): mallocCalls(0), typedMallocCalls(0), reallocCalls(0), freeCalls(0), firstSize(0) {}

	void* mallocDEBUG(size_t size, const char*, int)
		{
		return allocate(size, false);
		}

	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)
		{
		return allocate(size, true);
		}

	void* malloc(size_t size)
		{
		return allocate(size, false);
		}

	void* malloc(size_t size, NxMemoryType)
		{
		return allocate(size, true);
		}

	void* realloc(void* memory, size_t size)
		{
		++reallocCalls;
		return ::realloc(memory, size);
		}

	void free(void* memory)
		{
		++freeCalls;
		::free(memory);
		}

	unsigned mallocCalls;
	unsigned typedMallocCalls;
	unsigned reallocCalls;
	unsigned freeCalls;
	size_t firstSize;

	private:
	void* allocate(size_t size, bool typed)
		{
		++mallocCalls;
		if(typed)
			++typedMallocCalls;
		if(!firstSize)
			firstSize = size;
		return ::malloc(size);
		}
	};

class RecordingOutputStream : public NxUserOutputStream
	{
	public:
	RecordingOutputStream(): errors(0), asserts(0), prints(0), code(NXE_NO_ERROR), line(0)
		{
		message[0] = 0;
		file[0] = 0;
		}

	void reportError(NxErrorCode errorCode, const char* errorMessage, const char* errorFile, int errorLine)
		{
		++errors;
		code = errorCode;
		line = errorLine;
		copy(message, sizeof(message), errorMessage);
		copy(file, sizeof(file), errorFile);
		}

	NxAssertResponse reportAssertViolation(const char*, const char*, int)
		{
		++asserts;
		return NX_AR_CONTINUE;
		}

	void print(const char*)
		{
		++prints;
		}
	void reset()
		{
		errors = 0;
		asserts = 0;
		prints = 0;
		code = NXE_NO_ERROR;
		line = 0;
		message[0] = 0;
		file[0] = 0;
		}

	unsigned errors;
	unsigned asserts;
	unsigned prints;
	NxErrorCode code;
	int line;
	char message[64];
	char file[64];

	private:
	static void copy(char* destination, size_t capacity, const char* source)
		{
		if(!source)
			{
			destination[0] = 0;
			return;
			}
		strncpy_s(destination, capacity, source, _TRUNCATE);
		}
	};

struct NxSdkHeldSceneWriteLock
	{
	void* link;
	HANDLE ready;
	HANDLE release;
	};

static DWORD WINAPI nxSdkHoldSceneWriteLock(void* context)
	{
	NxSdkHeldSceneWriteLock* held = static_cast<NxSdkHeldSceneWriteLock*>(context);
	nxNpSceneGuardEnter(held->link);
	SetEvent(held->ready);
	WaitForSingleObject(held->release, 1500);
	nxNpSceneGuardLeave(held->link);
	return 0;
	}

struct NxSdkProbeSceneWriteLock
	{
	void* link;
	HANDLE completed;
	bool acquired;
	};

static DWORD WINAPI nxSdkProbeSceneWriteLock(void* context)
	{
	NxSdkProbeSceneWriteLock* probe = static_cast<NxSdkProbeSceneWriteLock*>(context);
	probe->acquired = nxNpSceneGuardWriteTry(probe->link);
	if(probe->acquired)
		nxNpSceneGuardLeave(probe->link);
	SetEvent(probe->completed);
	return 0;
	}

static void reportAllocator(const char* step, const RecordingAllocator& allocator)
	{
	printf("%s alloc_calls=%u typed_calls=%u realloc_calls=%u free_calls=%u first_size=%u\n",
		step, allocator.mallocCalls, allocator.typedMallocCalls, allocator.reallocCalls,
		allocator.freeCalls, static_cast<unsigned>(allocator.firstSize));
	}

static void reportStream(const char* step, const RecordingOutputStream& stream)
	{
	printf("%s errors=%u asserts=%u prints=%u code=%d line=%d message=%s file=%s\n",
		step, stream.errors, stream.asserts, stream.prints, static_cast<int>(stream.code),
		stream.line, stream.message, stream.file);
	}

// One create/release cycle for one allocator/output-stream combination. The
// singleton is left released so the next combination starts from the same state.
static int runCombination(CreatePhysicsSDKFn createSDK, const char* name,
	RecordingAllocator* allocator, RecordingOutputStream* stream)
	{
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, allocator, stream);
	printf("step=create_%s sdk=%s\n", name, sdk ? "nonnull" : "null");
	if(!sdk)
		return nxFail("exact version create returned null");

	if(allocator)
		{
		char step[64];
		sprintf_s(step, "step=create_%s.allocator", name);
		reportAllocator(step, *allocator);
		}
	if(stream)
		{
		char step[64];
		sprintf_s(step, "step=create_%s.stream", name);
		reportStream(step, *stream);
		}

	sdk->release();
	printf("step=release_%s\n", name);
	if(allocator)
		{
		char step[64];
		sprintf_s(step, "step=release_%s.allocator", name);
		reportAllocator(step, *allocator);
		}
	return 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSDKTests", pairDirectory, &physics);
	if(status)
		return status;

	CreatePhysicsSDKFn createSDK =
		reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	printf("export=NxCreatePhysicsSDK present=%s\n", createSDK ? "yes" : "no");
	if(!createSDK)
		{
		FreeLibrary(physics);
		return nxFail("NxCreatePhysicsSDK missing; SDK creation unavailable");
		}

	printf("version=0x%08x\n", static_cast<unsigned>(NX_PHYSICS_SDK_VERSION));
	NxPhysicsSDK* wrongVersion = createSDK(NX_PHYSICS_SDK_VERSION ^ 1, 0, 0);
	printf("step=wrong_version sdk=%s\n", wrongVersion ? "nonnull" : "null");
	if(wrongVersion)
		{
		FreeLibrary(physics);
		return nxFail("wrong version accepted");
		}

	RecordingAllocator allocatorOnly;
	RecordingOutputStream streamOnly;
	RecordingAllocator allocatorBoth;
	RecordingOutputStream streamBoth;
	status = runCombination(createSDK, "default_default", 0, 0);
	if(!status)
		status = runCombination(createSDK, "allocator_default", &allocatorOnly, 0);
	if(!status)
		status = runCombination(createSDK, "default_stream", 0, &streamOnly);
	if(!status)
		status = runCombination(createSDK, "allocator_stream", &allocatorBoth, &streamBoth);
	if(status)
		{
		FreeLibrary(physics);
		return status;
		}

	RecordingAllocator allocatorFirst;
	RecordingOutputStream streamFirst;
	RecordingAllocator allocatorSecond;
	RecordingOutputStream streamSecond;
	NxPhysicsSDK* first = createSDK(NX_PHYSICS_SDK_VERSION, &allocatorFirst, &streamFirst);
	printf("step=create sdk=%s\n", first ? "nonnull" : "null");
	if(!first)
		{
		FreeLibrary(physics);
		return nxFail("create returned null");
		}
	reportAllocator("step=create.allocator", allocatorFirst);
	reportStream("step=create.stream", streamFirst);

	NxPhysicsSDK* second = createSDK(NX_PHYSICS_SDK_VERSION, &allocatorSecond, &streamSecond);
	printf("step=second_create sdk=%s identity=%s\n", second ? "nonnull" : "null",
		second == first ? "same" : "different");
	reportAllocator("step=second_create.allocator", allocatorSecond);

	// NX_PENALTY_FORCE is documented as range [0, inf], so the negative value is
	// rejected; both calls stay inside the declared enum.
	bool rejected = first->setParameter(NX_PENALTY_FORCE, -1.0f);
	bool accepted = first->setParameter(NX_PENALTY_FORCE, 0.5f);
	printf("step=parameters out_of_range=%d in_range=%d value=%.6f scenes=%u materials=%u\n",
		rejected ? 1 : 0, accepted ? 1 : 0, first->getParameter(NX_PENALTY_FORCE),
		first->getNbScenes(), first->getNbMaterials());
	NxSceneDesc sceneDesc;
	NxScene* sceneA = first->createScene(sceneDesc);
	NxScene* sceneB = first->createScene(sceneDesc);
	int sceneLookupFailed = !sceneA || !sceneB || first->getScene(0) != sceneA ||
		first->getScene(1) != sceneB;
	printf("step=actor_group_pair_flags_scenes count=%u\n", first->getNbScenes());
	if(first->getNbScenes() != 2)
		return nxFail("actor-group pair-flag scenes were not created");
	const NxActorGroup groupA = 7;
	const NxActorGroup groupB = 3;
	const NxU32 initialPairFlags = first->getActorGroupPairFlags(groupA, groupB);
	first->setActorGroupPairFlags(groupA, groupB, 0x0000000a);
	const NxU32 forwardPairFlags = first->getActorGroupPairFlags(groupA, groupB);
	const NxU32 reversePairFlags = first->getActorGroupPairFlags(groupB, groupA);
	first->setActorGroupPairFlags(groupA, groupB, 0);
	const NxU32 clearedPairFlags = first->getActorGroupPairFlags(groupA, groupB);
	printf("step=actor_group_pair_flags initial=%u forward=%u reverse=%u cleared=%u\n",
		initialPairFlags, forwardPairFlags, reversePairFlags, clearedPairFlags);

	// addMaterial is a write-locked SDK wrapper. Contending on the second live
	// Scene drives its reverse-unlock continuation after Scene A was acquired.
	NxMaterial* sourceMaterial = first->getMaterial(0);
	if(!sourceMaterial)
		return nxFail("default material is missing");
	NxMaterial addedMaterial = *sourceMaterial;
	addedMaterial.dynamicFriction = 0.31f;
	addedMaterial.staticFriction = 0.47f;
	const NxU32 materialsBefore = first->getNbMaterials();
	void* firstSceneWriteLink = *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(sceneA) + 0x0c);
	NxSdkHeldSceneWriteLock held = {
		*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(sceneB) + 0x0c),
		CreateEventW(0, TRUE, FALSE, 0), CreateEventW(0, TRUE, FALSE, 0) };
	if(!firstSceneWriteLink || !held.link || !held.ready || !held.release)
		return nxFail("material Scene-lock fixture setup failed");
	HANDLE holdThread = CreateThread(0, 0, nxSdkHoldSceneWriteLock, &held, 0, 0);
	if(!holdThread || WaitForSingleObject(held.ready, 5000) != WAIT_OBJECT_0)
		return nxFail("material fixture did not acquire the second Scene lock");
	streamFirst.reset();
	const DWORD materialStart = GetTickCount();
	const NxMaterialIndex lockedMaterial = first->addMaterial(addedMaterial);
	const DWORD materialElapsed = GetTickCount() - materialStart;
	SetEvent(held.release);
	const bool holdJoined = WaitForSingleObject(holdThread, 5000) == WAIT_OBJECT_0;
	CloseHandle(holdThread);
	CloseHandle(held.ready);
	CloseHandle(held.release);
	NxSdkProbeSceneWriteLock probe = { firstSceneWriteLink, CreateEventW(0, TRUE, FALSE, 0), false };
	HANDLE probeThread = probe.completed ? CreateThread(0, 0, nxSdkProbeSceneWriteLock, &probe, 0, 0) : 0;
	const bool probeCompleted = probeThread &&
		WaitForSingleObject(probe.completed, 5000) == WAIT_OBJECT_0 &&
		WaitForSingleObject(probeThread, 5000) == WAIT_OBJECT_0;
	if(probeThread) CloseHandle(probeThread);
	if(probe.completed) CloseHandle(probe.completed);
	const bool firstSceneLockUnwound = probeCompleted && probe.acquired;
	if(probeCompleted && !probe.acquired)
		// A no-op-unwind mutant leaked this lock on the calling thread.
		nxNpSceneGuardLeave(firstSceneWriteLink);
	const unsigned materialLockErrors = streamFirst.errors;
	const bool materialRefused = materialLockErrors == 1 &&
		streamFirst.code == NXE_INVALID_OPERATION && streamFirst.line == 173 &&
		strstr(streamFirst.message, "WriteLock is still aquired") != 0;
	const bool materialCountUnchanged = first->getNbMaterials() == materialsBefore;
	const bool materialReturnedPromptly = materialElapsed < 500;
	printf("step=material_lock returned_while_locked=%u index=%u errors=%u code=%u line=%d unwind_first=%u count_unchanged=%u\n",
		materialReturnedPromptly ? 1 : 0, static_cast<unsigned>(lockedMaterial),
		materialLockErrors, static_cast<unsigned>(streamFirst.code), streamFirst.line,
		firstSceneLockUnwound ? 1u : 0u, materialCountUnchanged ? 1u : 0u);
	if(!holdJoined || !materialReturnedPromptly || !materialRefused || !firstSceneLockUnwound ||
		!materialCountUnchanged)
		return nxFail("addMaterial did not refuse the contended Scene write lock");

	streamFirst.reset();
	const NxMaterialIndex unlockedMaterial = first->addMaterial(addedMaterial);
	const NxU32 materialsAfterAdd = first->getNbMaterials();
	printf("step=material_unlocked index=%u count=%u errors=%u\n",
		static_cast<unsigned>(unlockedMaterial), materialsAfterAdd, streamFirst.errors);
	if(unlockedMaterial != materialsBefore || materialsAfterAdd != materialsBefore + 1 || streamFirst.errors)
		return nxFail("addMaterial failed after the Scene locks were released");

	printf("step=scenes_created count=%u both=%d first_lookup=%d second_lookup=%d\n",
		first->getNbScenes(), sceneA && sceneB ? 1 : 0,
		first->getScene(0) == sceneA ? 1 : 0,
		first->getScene(1) == sceneB ? 1 : 0);
	if(sceneA)
		first->releaseScene(*sceneA);
	const int survivorLookup = first->getScene(0) == sceneB ? 1 : 0;
	printf("step=scene_released count=%u survivor_lookup=%d\n", first->getNbScenes(), survivorLookup);
	if(first->getNbScenes() != 1 || !survivorLookup)
		sceneLookupFailed = 1;
	if(sceneB)
		first->releaseScene(*sceneB);
	printf("step=scenes_empty count=%u\n", first->getNbScenes());
	reportAllocator("step=scenes_released.allocator", allocatorFirst);
	reportStream("step=parameters.stream", streamFirst);

	first->release();
	printf("step=release\n");
	reportAllocator("step=release.allocator", allocatorFirst);
	reportStream("step=release.stream", streamFirst);

	NxPhysicsSDK* recreated = createSDK(NX_PHYSICS_SDK_VERSION, &allocatorSecond, &streamSecond);
	printf("step=recreate sdk=%s\n", recreated ? "nonnull" : "null");
	if(!recreated)
		{
		FreeLibrary(physics);
		return nxFail("recreate returned null");
		}
	reportAllocator("step=recreate.allocator", allocatorSecond);
	reportStream("step=recreate.stream", streamSecond);

	// Both pointers are live here. Comparing against the instance released above
	// would read a freed pointer, and the answer would depend on heap reuse.
	NxPhysicsSDK* recreatedAgain = createSDK(NX_PHYSICS_SDK_VERSION, &allocatorSecond, &streamSecond);
	printf("step=recreate_second sdk=%s identity=%s\n", recreatedAgain ? "nonnull" : "null",
		recreatedAgain == recreated ? "same" : "different");

	recreated->release();
	printf("step=recreate_release\n");
	reportAllocator("step=recreate_release.allocator", allocatorSecond);

	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	if(!status && sceneLookupFailed)
		return nxFail("scene lookup did not track the created and surviving public scenes");
	return status;
	}
