// The shipped process pool does not recover when one of its constructor or
// growth arrays cannot be allocated. Keep those access violations inside child
// processes and require the oracle and candidate to fail at matching sites.
#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxUserAllocator.h"
#include <stdlib.h>
#include <wchar.h>

typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32, NxUserOutputStream*, NxUserAllocator*);
typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

class NxFailingAllocator : public NxUserAllocator
	{
	public:
	NxFailingAllocator() : mFailSize(0), mFailOrdinal(0) {}
	void failAllocationOfSizeAfter(unsigned size, unsigned ordinal)
		{ mFailSize = size; mFailOrdinal = ordinal; }

	virtual void* malloc(size_t size) { return allocate(size); }
	virtual void* malloc(size_t size, NxMemoryType) { return allocate(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int) { return allocate(size); }
	virtual void* realloc(void* memory, size_t size) { return ::realloc(memory, size); }
	virtual void free(void* memory) { ::free(memory); }

	private:
	void* allocate(size_t size)
		{
		if(size == mFailSize && mFailOrdinal)
			{
			if(--mFailOrdinal == 0)
				return 0;
			}
		return ::malloc(size);
		}

	unsigned mFailSize;
	unsigned mFailOrdinal;
	};

struct NxPoolAllocationProbe
	{
	const char* slot;
	unsigned size;
	unsigned ordinal;
	};

static const NxPoolAllocationProbe kProbes[] =
	{
		{ "objects", 8, 1 },
		{ "forward", 4, 1 },
		{ "reverse", 4, 2 },
		{ "generations", 4, 3 }
	};

static int nxRunFailingChild(const wchar_t* pairDirectory, const wchar_t* mode,
	unsigned size, unsigned ordinal)
	{
	wchar_t self[MAX_PATH];
	if(!GetModuleFileNameW(0, self, MAX_PATH))
		return 1;

	wchar_t commandLine[MAX_PATH * 2 + 96];
	if(swprintf_s(commandLine, sizeof(commandLine) / sizeof(commandLine[0]),
		L"\"%s\" \"%s\" %s %u %u", self, pairDirectory, mode, size, ordinal) < 0)
		return 1;

	SECURITY_ATTRIBUTES security = { sizeof(security), 0, TRUE };
	HANDLE sinkInput = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		&security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	HANDLE sinkOutput = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		&security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(sinkInput == INVALID_HANDLE_VALUE || sinkOutput == INVALID_HANDLE_VALUE)
		{
		if(sinkInput != INVALID_HANDLE_VALUE) CloseHandle(sinkInput);
		if(sinkOutput != INVALID_HANDLE_VALUE) CloseHandle(sinkOutput);
		return 1;
		}

	STARTUPINFOW startup = {};
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;
	startup.hStdInput = sinkInput;
	startup.hStdOutput = sinkOutput;
	startup.hStdError = sinkOutput;
	PROCESS_INFORMATION process = {};
	const BOOL started = CreateProcessW(self, commandLine, 0, 0, TRUE,
		CREATE_NO_WINDOW, 0, 0, &startup, &process);
	CloseHandle(sinkInput);
	CloseHandle(sinkOutput);
	if(!started)
		return 1;

	// Full phase gates run many oracle/candidate pairs before this fixture;
	// allow the spawned process to finish under that load before treating it
	// as a hung allocation probe.
	const DWORD waited = WaitForSingleObject(process.hProcess, 60000);
	DWORD exitCode = STILL_ACTIVE;
	if(waited == WAIT_OBJECT_0)
		GetExitCodeProcess(process.hProcess, &exitCode);
	else
		TerminateProcess(process.hProcess, 0xffffffffu);
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	if(waited == WAIT_OBJECT_0)
		printf("child allocation mode=%ls size=%u ordinal=%u exit=%08lx\n",
			mode, size, ordinal, exitCode);
	return waited == WAIT_OBJECT_0 && exitCode == 0xc0000005u ? 0 : 1;
	}

static int nxRunChild(int argc, wchar_t** argv, HMODULE physics)
	{
	const bool growth = argc == 5 && wcscmp(argv[2], L"--growth-child") == 0;
	if(argc != 5 || (wcscmp(argv[2], L"--child") != 0 && !growth))
		return 2;
	const unsigned size = static_cast<unsigned>(wcstoul(argv[3], 0, 10));
	const unsigned ordinal = static_cast<unsigned>(wcstoul(argv[4], 0, 10));
	if(!size || !ordinal)
		return 2;

	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	if(!foundation)
		return 3;
	CreateFoundationSDKFn createFoundation = reinterpret_cast<CreateFoundationSDKFn>(
		GetProcAddress(foundation, "NxCreateFoundationSDK"));
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createFoundation || !createSDK)
		return 4;

	NxFailingAllocator foundationAllocator;
	NxFailingAllocator sdkAllocator;
	if(!createFoundation(NX_FOUNDATION_SDK_VERSION, 0, &foundationAllocator))
		return 5;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &sdkAllocator, 0);
	if(!sdk)
		return 6;
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		return 7;
	if(growth)
		{
		NxBoxShapeDesc box;
		NxActorDesc actorDesc;
		actorDesc.shapes.pushBack(&box);
		if(!scene->createActor(actorDesc))
			return 8;
		NxBodyDesc body;
		NxActorDesc dynamicDesc;
		dynamicDesc.body = &body;
		dynamicDesc.density = 1.0f;
		dynamicDesc.shapes.pushBack(&box);
		if(!scene->createActor(dynamicDesc)) return 8;
		scene = sdk->createScene(sceneDesc);
		if(!scene) return 8;
		actorDesc = dynamicDesc;
		}

	sdkAllocator.failAllocationOfSizeAfter(size, ordinal);
	NxBoxShapeDesc box;
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	NxActor* actor = scene->createActor(actorDesc);
	return actor ? 0 : 9;
	}

int wmain(int argc, wchar_t** argv)
	{
	const bool child = argc == 5 && (wcscmp(argv[2], L"--child") == 0
		|| wcscmp(argv[2], L"--growth-child") == 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int status = nxOpenPair(child ? 2 : argc, argv,
		"NxPhysicsPrunerPoolAllocationFailureTests", pairDirectory, &physics);
	if(status)
		return status;
	if(child)
		return nxRunChild(argc, argv, physics);

	if(argc != 2)
		return 2;
	int failures = 0;
	for(unsigned i = 0; i < sizeof(kProbes) / sizeof(kProbes[0]); ++i)
		{
		const NxPoolAllocationProbe& probe = kProbes[i];
		if(nxRunFailingChild(pairDirectory, L"--child", probe.size, probe.ordinal))
			++failures;
		}
	printf("pruner pool array oom cases=4 slots=objects.forward.reverse.generations exit=c0000005.c0000005.c0000005.c0000005 mismatches=%u\n",
		failures);
	const NxPoolAllocationProbe growthProbes[] =
		{
		{ "objects", 16, 1 },
		{ "forward", 8, 1 },
		{ "reverse", 8, 2 },
		{ "generations", 8, 3 }
		};
	unsigned growthFailures = 0;
	for(unsigned i = 0; i < sizeof(growthProbes) / sizeof(growthProbes[0]); ++i)
		{
		const NxPoolAllocationProbe& probe = growthProbes[i];
		if(nxRunFailingChild(pairDirectory, L"--growth-child", probe.size, probe.ordinal))
			++growthFailures;
		}
	printf("pruner pool growth array oom cases=4 slots=objects.forward.reverse.generations exit=c0000005.c0000005.c0000005.c0000005 mismatches=%u\n",
		growthFailures);
	return failures || growthFailures ? 1 : 0;
	}
