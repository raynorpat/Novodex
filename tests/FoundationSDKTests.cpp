#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NxFoundationSDK.h"
#include "NxUserAllocator.h"
#include "NxUserOutputStream.h"

typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32, NxUserOutputStream*, NxUserAllocator*);
typedef bool (__cdecl *FoundationErrorFn)(NxErrorCode, const char*, int, bool*, const char*, ...);

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

static int fail(HMODULE module, const char* message)
	{
	fprintf(stderr, "FAIL %s\n", message);
	FreeLibrary(module);
	return 1;
	}

int main(int argc, char** argv)
	{
	if(argc != 2)
		{
		fprintf(stderr, "usage: NxFoundationSDKTests <NxFoundation.dll>\n");
		return 2;
		}

	HMODULE module = LoadLibraryA(argv[1]);
	if(!module)
		{
		fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
		return 2;
		}

	CreateFoundationSDKFn createSDK = reinterpret_cast<CreateFoundationSDKFn>(GetProcAddress(module, "NxCreateFoundationSDK"));
	FoundationErrorFn reportFoundationError = reinterpret_cast<FoundationErrorFn>(GetProcAddress(module, "?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ"));
	NxUserAllocator** allocatorGlobal = reinterpret_cast<NxUserAllocator**>(GetProcAddress(module, "?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A"));
	if(!createSDK || !reportFoundationError || !allocatorGlobal)
		return fail(module, "required export missing");

	if(createSDK(NX_FOUNDATION_SDK_VERSION ^ 1, 0, 0) != 0)
		return fail(module, "wrong version accepted");
	if(*allocatorGlobal != 0)
		return fail(module, "wrong version changed allocator global");
	printf("wrong_version=null global=null\n");

	RecordingAllocator allocatorA;
	RecordingAllocator allocatorB;
	RecordingOutputStream streamA;
	RecordingOutputStream streamB;
	NxFoundationSDK* sdk = createSDK(NX_FOUNDATION_SDK_VERSION, &streamA, &allocatorA);
	if(!sdk || &sdk->getAllocator() != &allocatorA || *allocatorGlobal != &allocatorA || sdk->getErrorStream() != &streamA)
		return fail(module, "create bindings differ");
	if(allocatorA.firstSize != 56)
		return fail(module, "FoundationSDK allocation size differs");
	printf("create=nonnull alloc_calls=%u typed_calls=%u first_size=%u bindings=allocator,global,stream\n",
		allocatorA.mallocCalls, allocatorA.typedMallocCalls, static_cast<unsigned>(allocatorA.firstSize));

	NxErrorCode initialLast = sdk->getLastError();
	NxErrorCode initialFirst = sdk->getFirstError();
	if(initialLast != NXE_NO_ERROR || initialFirst != NXE_NO_ERROR)
		return fail(module, "initial error state differs");
	printf("initial_errors=%d,%d\n", static_cast<int>(initialLast), static_cast<int>(initialFirst));

	bool ignoreAlways = false;
	bool breakRequested = reportFoundationError(NXE_DB_WARNING, "sdk_gate.cpp", 77, &ignoreAlways, "gate message");
	NxErrorCode last = sdk->getLastError();
	NxErrorCode first = sdk->getFirstError();
	NxErrorCode clearedLast = sdk->getLastError();
	NxErrorCode clearedFirst = sdk->getFirstError();
	if(breakRequested || ignoreAlways || streamA.errors != 1 || streamA.asserts || streamA.prints ||
		streamA.code != NXE_DB_WARNING || streamA.line != 77 || strcmp(streamA.message, "gate message") ||
		strcmp(streamA.file, "sdk_gate.cpp") || last != NXE_DB_WARNING || first != NXE_DB_WARNING ||
		clearedLast != NXE_NO_ERROR || clearedFirst != NXE_NO_ERROR)
		return fail(module, "error/output behavior differs");
	printf("error=return_false callback=206,gate_message,sdk_gate.cpp,77 codes=%d,%d,%d,%d\n",
		static_cast<int>(last), static_cast<int>(first), static_cast<int>(clearedLast), static_cast<int>(clearedFirst));

	NxFoundationSDK* second = createSDK(NX_FOUNDATION_SDK_VERSION, &streamB, &allocatorB);
	if(second != sdk || &second->getAllocator() != &allocatorA || second->getErrorStream() != &streamB || allocatorB.mallocCalls != 0)
		return fail(module, "singleton recreation behavior differs");
	printf("second_create=same allocator=sticky stream=updated second_allocator_calls=0\n");

	sdk->release();
	if(allocatorA.freeCalls != 1 || *allocatorGlobal != &allocatorA)
		return fail(module, "release allocator behavior differs");
	printf("release=free_calls_%u global=sticky\n", allocatorA.freeCalls);

	NxFoundationSDK* recreated = createSDK(NX_FOUNDATION_SDK_VERSION, 0, &allocatorB);
	if(!recreated || &recreated->getAllocator() != &allocatorB || *allocatorGlobal != &allocatorB)
		return fail(module, "post-release recreation differs");
	recreated->release();
	if(allocatorB.mallocCalls != 1 || allocatorB.typedMallocCalls != 1 || allocatorB.freeCalls != 1)
		return fail(module, "recreated allocator activity differs");
	printf("recreate=nonnull alloc_calls=%u typed_calls=%u free_calls=%u\n",
		allocatorB.mallocCalls, allocatorB.typedMallocCalls, allocatorB.freeCalls);

	FreeLibrary(module);
	return 0;
	}
