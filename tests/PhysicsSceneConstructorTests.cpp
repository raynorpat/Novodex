// Pins the NpScene wrapper's back-link to the NxSceneInternal passed to its
// constructor. The oracle listing stores that argument at wrapper +0x24.
#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "EmbeddedHookBase.h"
#include "NpScene.h"
#include <stdlib.h>
#include <stdio.h>
#include <type_traits>

static_assert(std::is_base_of<EmbeddedHookBase, NpScene>::value,
	"NpScene must model its secondary 12-byte polymorphic subobject");

typedef void* (__thiscall* SceneSecondaryDeletingDtor)(void*, unsigned);

static bool belongsToModule(HMODULE expectedModule, const void* address)
	{
	HMODULE owner = 0;
	return address && GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCWSTR>(address), &owner) && owner == expectedModule;
	}

class SceneDestructorAllocator : public NxUserAllocator
	{
	public:
		SceneDestructorAllocator() : freeCount(0) {}
		void* mallocDEBUG(size_t size, const char*, int) override
			{ return malloc(size); }
		void* malloc(size_t size) override
			{ return ::malloc(size); }
		void* realloc(void* memory, size_t size) override
			{ return ::realloc(memory, size); }
		void free(void* memory) override
			{ if(memory) ++freeCount; ::free(memory); }
		unsigned freeCount;
	};

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSceneConstructorTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	SceneDestructorAllocator allocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");

	const void* internalScene = *reinterpret_cast<void* const*>(
		reinterpret_cast<const unsigned char*>(scene) + 0x24);
	printf("scene constructor internal_link=%u\n", internalScene ? 1u : 0u);
	if(!internalScene)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		status = nxReportPairIdentity(pairDirectory);
		return status ? status : nxFail("scene constructor internal link is null");
		}

	unsigned char* sceneBytes = reinterpret_cast<unsigned char*>(scene);
	void** secondaryVtable = *reinterpret_cast<void***>(sceneBytes + 8);
	void* secondaryDeletingDtor = secondaryVtable ? secondaryVtable[0] : 0;
	if(!belongsToModule(physics, secondaryVtable) ||
		!belongsToModule(physics, secondaryDeletingDtor))
		{
		sdk->releaseScene(*scene);
		sdk->release();
		return nxFail("scene secondary destructor is not installed in NxPhysics");
		}

	void* primaryBefore = *reinterpret_cast<void**>(sceneBytes);
	void* secondaryBefore = *reinterpret_cast<void**>(sceneBytes + 8);
	const unsigned freesBefore = allocator.freeCount;
	void* const returned = reinterpret_cast<SceneSecondaryDeletingDtor>(
		secondaryDeletingDtor)(sceneBytes + 8, 0);
	const unsigned freed = allocator.freeCount - freesBefore;
	const bool adjusted = returned == scene;
	const bool primaryTransition = primaryBefore != *reinterpret_cast<void**>(sceneBytes);
	const bool secondaryTransition = secondaryBefore !=
		*reinterpret_cast<void**>(sceneBytes + 8);
	printf("scene secondary dtor nondeleting root=%u primary_transition=%u "
		"secondary_transition=%u frees=%u\n", adjusted, primaryTransition,
		secondaryTransition, freed);
	if(!adjusted || !primaryTransition || !secondaryTransition || freed == 0)
		return nxFail("scene secondary destructor did not adjust to the complete object");
	NxScene* deletingScene = sdk->createScene(sceneDesc);
	if(!deletingScene) return nxFail("second scene creation failed");
	unsigned char* deletingBytes = reinterpret_cast<unsigned char*>(deletingScene);
	void** deletingVtable = *reinterpret_cast<void***>(deletingBytes + 8);
	void* deletingDtor = deletingVtable ? deletingVtable[0] : 0;
	if(!belongsToModule(physics, deletingDtor))
		return nxFail("second scene secondary destructor is missing");
	const unsigned deletingFreesBefore = allocator.freeCount;
	void* const deletingReturned = reinterpret_cast<SceneSecondaryDeletingDtor>(
		deletingDtor)(deletingBytes + 8, 1);
	const unsigned deletingFrees = allocator.freeCount - deletingFreesBefore;
	const bool deletingAdjusted = deletingReturned == deletingScene;
	printf("scene secondary dtor deleting root=%u frees=%u\n",
		deletingAdjusted, deletingFrees);
	if(!deletingAdjusted || deletingFrees <= freed)
		return nxFail("deleting secondary destructor returned the wrong object");

	// Negative control: the unadjusted EmbeddedHookBase member destructor returns
	// the +8 subobject. The real secondary thunk must return the root instead.
	alignas(4) unsigned char memberOnlyObject[0x28] = {};
	EmbeddedHookBase memberOnlyBase;
	*reinterpret_cast<void***>(memberOnlyObject + 8) =
		*reinterpret_cast<void***>(&memberOnlyBase);
	void** memberOnlyVtable = *reinterpret_cast<void***>(memberOnlyObject + 8);
	void* const memberOnlyReturned = reinterpret_cast<SceneSecondaryDeletingDtor>(
		memberOnlyVtable[0])(memberOnlyObject + 8, 0);
	const bool memberOnlyMutantDetected = memberOnlyReturned != memberOnlyObject;
	printf("scene secondary dtor member-only mutant detected=%u\n",
		memberOnlyMutantDetected);
	if(!memberOnlyMutantDetected)
		return nxFail("member-only scene destructor mutant was not detected");

	return nxReportPairIdentity(pairDirectory);
	}
