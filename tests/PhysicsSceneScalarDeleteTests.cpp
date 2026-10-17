#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxFoundationSDK.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxUserAllocator.h"

typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32,
	NxUserOutputStream*, NxUserAllocator*);
typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

class SceneDeleteAllocator : public NxUserAllocator
	{
	public:
	SceneDeleteAllocator() : target(0), targetFreed(false) {}
	virtual void* malloc(size_t size) { return allocate(size); }
	virtual void* malloc(size_t size, NxMemoryType) { return allocate(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int)
		{ return allocate(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)
		{ return allocate(size); }
	virtual void* realloc(void* memory, size_t size)
		{
		if(!memory) return allocate(size);
		unsigned* oldHeader = static_cast<unsigned*>(memory) - 2;
		void* const replacement = allocate(size);
		if(replacement)
			{
			memcpy(replacement, memory, oldHeader[0] < size ?
				oldHeader[0] : size);
			free(memory);
			}
		return replacement;
		}
	virtual void free(void* memory)
		{
		if(!memory) return;
		if(memory == target) targetFreed = true;
		::free(static_cast<unsigned*>(memory) - 2);
		}
	size_t sizeOf(const void* memory) const
		{ return memory ? *(static_cast<const unsigned*>(memory) - 2) : 0; }
	void* target;
	bool targetFreed;
	private:
	static void* allocate(size_t size)
		{
		unsigned* const header = static_cast<unsigned*>(::malloc(size + 8));
		if(!header) return 0;
		header[0] = static_cast<unsigned>(size);
		header[1] = 0x54454152;
		return header + 2;
		}
	};

typedef void* (__thiscall *SceneScalarDeletingDestructorFn)(void*, unsigned);

static void* nxResolveCandidateThunk(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	wchar_t path[MAX_PATH];
	if(swprintf_s(path, L"%s\\NxPhysics.map", pairDirectory) < 0)
		return 0;
	FILE* map = 0;
	if(_wfopen_s(&map, path, L"rb") != 0 || !map)
		return 0;
	unsigned preferredBase = 0;
	void* result = 0;
	char line[2048];
	while(fgets(line, sizeof(line), map))
		{
		const char* baseText = strstr(line, "Preferred load address is ");
		if(baseText)
			{
			baseText += strlen("Preferred load address is ");
			preferredBase = static_cast<unsigned>(strtoul(baseText, 0, 16));
			continue;
			}
		const char* symbol = strstr(line,
			"scalarDeletingDestructor@NxSceneInternal");
		if(!symbol) continue;
		const char* addressText = symbol;
		while(*addressText && *addressText != ' ' && *addressText != '\t')
			++addressText;
		while(*addressText == ' ' || *addressText == '\t') ++addressText;
		char* end = 0;
		const unsigned address = static_cast<unsigned>(strtoul(addressText,
			&end, 16));
		if(end != addressText && preferredBase && address >= preferredBase)
			result = reinterpret_cast<unsigned char*>(physics) +
				(address - preferredBase);
		break;
		}
	fclose(map);
	return result;
	}

static void* nxCallThunk(void* entry, void* scene, unsigned flags,
	bool* stackBalanced)
	{
	unsigned before = 0, after = 0;
	void* result = 0;
	__asm
		{
		mov before, esp
		mov ecx, scene
		push flags
		call dword ptr [entry]
		mov result, eax
		mov after, esp
		}
	*stackBalanced = before == after;
	return result;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int status = nxOpenPair(argc, argv, "NxPhysicsSceneScalarDeleteTests",
		pairDirectory, &physics);
	if(status) return status;
	HMODULE foundationModule = GetModuleHandleW(L"NxFoundation.dll");
	if(!foundationModule) return nxFail("NxFoundation.dll is not loaded");
	CreateFoundationSDKFn createFoundation = reinterpret_cast<CreateFoundationSDKFn>(
		GetProcAddress(foundationModule, "NxCreateFoundationSDK"));
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createFoundation || !createSDK)
		return nxFail("required SDK export is missing");

	SceneDeleteAllocator allocator;
	NxFoundationSDK* const foundation = createFoundation(
		NX_FOUNDATION_SDK_VERSION, 0, &allocator);
	if(!foundation) return nxFail("Foundation creation failed");
	NxPhysicsSDK* const sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk) return nxFail("Physics SDK creation failed");

	wchar_t mapPath[MAX_PATH];
	if(swprintf_s(mapPath, L"%s\\NxPhysics.map", pairDirectory) < 0)
		return nxFail("candidate map path could not be formed");
	const bool candidate = GetFileAttributesW(mapPath) != INVALID_FILE_ATTRIBUTES;
	void* entry = reinterpret_cast<unsigned char*>(physics) + 0x000145c0u;
	if(candidate) entry = nxResolveCandidateThunk(physics, pairDirectory);
	if(!entry) return nxFail("Scene scalar deleting thunk entry is unresolved");

	NxSceneDesc desc;
	desc.setToDefault();
	for(unsigned flags = 0; flags != 2; ++flags)
		{
	NxScene* const scene = sdk->createScene(desc);
	if(!scene) return nxFail("Scene creation failed");
	void* const internal = *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(scene) + 0x24);
	const size_t bytes = allocator.sizeOf(internal);
	if(!internal || !bytes)
		return nxFail("Scene internal allocation precondition failed");
	allocator.target = internal;
	allocator.targetFreed = false;
	bool stackBalanced = false;
	void* const result = nxCallThunk(entry, internal, flags, &stackBalanced);
	const bool freed = allocator.targetFreed;
	if(flags == 0)
		printf("scene_scalar_delete flag=0 bytes=%u preserved=%u result_self=%u stack=%u\n",
			static_cast<unsigned>(bytes), freed ? 0u : 1u,
			result == internal ? 1u : 0u, stackBalanced ? 1u : 0u);
	else
		printf("scene_scalar_delete flag=1 bytes=%u freed=%u result_self=%u stack=%u\n",
			static_cast<unsigned>(bytes), freed ? 1u : 0u,
			result == internal ? 1u : 0u, stackBalanced ? 1u : 0u);
	if(!stackBalanced || result != internal || (flags == 0 ? freed : !freed))
		return nxFail("Scene scalar deleting thunk row contract mismatch");
	if(flags == 0) allocator.free(internal);
	}
	return nxReportPairIdentity(pairDirectory);
	}
