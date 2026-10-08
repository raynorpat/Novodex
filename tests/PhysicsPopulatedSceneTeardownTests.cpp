#include "PhysicsPairLoader.h"

#include <stdlib.h>
#include <string.h>

#include "NxFoundationSDK.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxUserAllocator.h"

typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32,
	NxUserOutputStream*, NxUserAllocator*);
typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

class TrackingAllocator : public NxUserAllocator
	{
	public:
	TrackingAllocator() : mOutstanding(0) {}

	virtual void* malloc(size_t size) { return allocate(size); }
	virtual void* malloc(size_t size, NxMemoryType) { return allocate(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int) { return allocate(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)
		{ return allocate(size); }
	virtual void* realloc(void* memory, size_t size)
		{
		if(!memory) return allocate(size);
		unsigned* oldHeader = static_cast<unsigned*>(memory) - 2;
		void* replacement = allocate(size);
		if(replacement)
			{
			memcpy(replacement, memory, oldHeader[0] < size ? oldHeader[0] : size);
			free(memory);
			}
		return replacement;
		}
	virtual void free(void* memory)
		{
		if(!memory) return;
		::free(static_cast<unsigned*>(memory) - 2);
		--mOutstanding;
		}

	unsigned outstanding() const { return mOutstanding; }

	private:
	void* allocate(size_t size)
		{
		unsigned* header = static_cast<unsigned*>(::malloc(size + 8));
		if(!header) return 0;
		header[0] = static_cast<unsigned>(size);
		header[1] = 0x54454152;
		++mOutstanding;
		return header + 2;
		}

	unsigned mOutstanding;
	};

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsPopulatedSceneTeardownTests",
		pairDirectory, &physics);
	if(status) return status;
	HMODULE foundationModule = GetModuleHandleW(L"NxFoundation.dll");
	if(!foundationModule) return nxFail("NxFoundation.dll is not loaded");
	CreateFoundationSDKFn createFoundation = reinterpret_cast<CreateFoundationSDKFn>(
		GetProcAddress(foundationModule, "NxCreateFoundationSDK"));
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createFoundation || !createSDK) return nxFail("required SDK export is missing");

	TrackingAllocator allocator;
	NxFoundationSDK* foundationSDK = createFoundation(
		NX_FOUNDATION_SDK_VERSION, 0, &allocator);
	if(!foundationSDK) return nxFail("Foundation creation failed");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk) return nxFail("Physics SDK creation failed");

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&box);
	NxBodyDesc body;
	NxActorDesc dynamicDesc;
	dynamicDesc.body = &body;
	dynamicDesc.density = 1.0f;
	dynamicDesc.shapes.pushBack(&box);
	if(!scene->createActor(staticDesc) || !scene->createActor(dynamicDesc))
		return nxFail("static-first populated actor fixture failed");

	const unsigned beforeScene = allocator.outstanding();
	sdk->releaseScene(*scene);
	const unsigned afterScene = allocator.outstanding();
	const int releaseDelta = static_cast<int>(afterScene) - static_cast<int>(beforeScene);
	printf("teardown static_first outstanding_before=%u outstanding_after=%u delta=%d\n",
		beforeScene, afterScene, releaseDelta);

	sdk->release();
	foundationSDK->release();
	if(releaseDelta != -39)
		return nxFail("scene teardown did not release the oracle's 39 scene-owned blocks");
	return nxReportPairIdentity(pairDirectory);
	}
