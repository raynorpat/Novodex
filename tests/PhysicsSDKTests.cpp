#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxUserAllocator.h"
#include "NxUserOutputStream.h"

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
	return status;
	}
