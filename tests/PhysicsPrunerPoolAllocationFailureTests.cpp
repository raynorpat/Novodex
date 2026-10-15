// The shipped constructor does not recover when one of its four process-pool
// arrays cannot be allocated. Keep that access violation inside a child process
// and require the oracle and candidate to fail at the same allocation site.
#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
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

static int nxRunFailingChild(const wchar_t* pairDirectory, unsigned size, unsigned ordinal)
	{
	wchar_t self[MAX_PATH];
	if(!GetModuleFileNameW(0, self, MAX_PATH))
		return 1;

	wchar_t commandLine[MAX_PATH * 2 + 96];
	if(swprintf_s(commandLine, sizeof(commandLine) / sizeof(commandLine[0]),
		L"\"%s\" \"%s\" --child %u %u", self, pairDirectory, size, ordinal) < 0)
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

	const DWORD waited = WaitForSingleObject(process.hProcess, 30000);
	DWORD exitCode = STILL_ACTIVE;
	if(waited == WAIT_OBJECT_0)
		GetExitCodeProcess(process.hProcess, &exitCode);
	else
		TerminateProcess(process.hProcess, 0xffffffffu);
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return waited == WAIT_OBJECT_0 && exitCode == 0xc0000005u ? 0 : 1;
	}

static int nxRunChild(int argc, wchar_t** argv, HMODULE physics)
	{
	if(argc != 5 || wcscmp(argv[2], L"--child") != 0)
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

	sdkAllocator.failAllocationOfSizeAfter(size, ordinal);
	NxBoxShapeDesc box;
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	NxActor* actor = scene->createActor(actorDesc);
	return actor ? 0 : 8;
	}

int wmain(int argc, wchar_t** argv)
	{
	const bool child = argc == 5 && wcscmp(argv[2], L"--child") == 0;
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
		if(nxRunFailingChild(pairDirectory, probe.size, probe.ordinal))
			++failures;
		}
	printf("pruner pool array oom cases=4 slots=objects.forward.reverse.generations exit=c0000005.c0000005.c0000005.c0000005 mismatches=%u\n",
		failures);
	return failures ? 1 : 0;
	}
