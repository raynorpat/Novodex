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
#include "NxPlaneShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxUserAllocator.h"

typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32,
	NxUserOutputStream*, NxUserAllocator*);
typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

class TrackingAllocator : public NxUserAllocator
	{
	public:
	TrackingAllocator() : mOutstanding(0), mAllocationCount(0) {}

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
		for(unsigned i = mAllocationCount; i > 0; --i)
			if(mPointers[i - 1] == memory)
				{
				mPointers[i - 1] = 0;
				break;
				}
		::free(static_cast<unsigned*>(memory) - 2);
		--mOutstanding;
		}

	unsigned outstanding() const { return mOutstanding; }
	size_t allocationSize(const void* memory) const
		{
		for(unsigned i = mAllocationCount; i > 0; --i)
			if(mPointers[i - 1] == memory)
				return mSizes[i - 1];
		return 0;
		}

	private:
	void* allocate(size_t size)
		{
		unsigned* header = static_cast<unsigned*>(::malloc(size + 8));
		if(!header) return 0;
		header[0] = static_cast<unsigned>(size);
		header[1] = 0x54454152;
		if(mAllocationCount < sizeof(mPointers) / sizeof(mPointers[0]))
			{
			mPointers[mAllocationCount] = header + 2;
			mSizes[mAllocationCount] = size;
			++mAllocationCount;
			}
		++mOutstanding;
		return header + 2;
		}

	unsigned mOutstanding;
	unsigned mAllocationCount;
	void* mPointers[8192];
	size_t mSizes[8192];
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

	if(releaseDelta != -39)
		return nxFail("scene teardown did not release the oracle's 39 scene-owned blocks");

	// Leave a live broadphase pair in the pruning engine, then release the
	// scene before another simulation step can retire it. phys_fn_001953 must
	// advance the scene stamp and delete this stale pair node during teardown.
	NxScene* contactScene = sdk->createScene(sceneDesc);
	if(!contactScene) return nxFail("contact-pair teardown scene creation failed");
	NxPlaneShapeDesc contactPlane;
	NxActorDesc contactStaticDesc;
	contactStaticDesc.shapes.pushBack(&contactPlane);
	NxSphereShapeDesc contactSphere;
	contactSphere.radius = 0.5f;
	NxActorDesc contactDynamicDesc;
	contactDynamicDesc.body = &body;
	contactDynamicDesc.density = 1.0f;
	contactDynamicDesc.globalPose.t = NxVec3(0.0f, 0.5f, 0.0f);
	contactDynamicDesc.shapes.pushBack(&contactSphere);
	NxActor* contactStatic = contactScene->createActor(contactStaticDesc);
	NxActor* contactDynamic = contactScene->createActor(contactDynamicDesc);
	if(!contactStatic || !contactDynamic)
		return nxFail("contact-pair teardown actors failed to create");
	contactScene->simulate(0.125f);
	if(!contactScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
		return nxFail("contact-pair teardown simulation did not fetch");
	unsigned char* contactWrapper = reinterpret_cast<unsigned char*>(contactScene);
	unsigned char* contactInternal = *reinterpret_cast<unsigned char**>(contactWrapper + 0x24);
	unsigned char* pairNode = *reinterpret_cast<unsigned char**>(contactInternal + 0x674);
	unsigned contactPairs = 0;
	while(pairNode)
		{
		++contactPairs;
		pairNode = *reinterpret_cast<unsigned char**>(pairNode + 8);
		}
	if(!contactPairs) return nxFail("contact-pair teardown fixture did not retain a broadphase pair");
	unsigned char* contactEngine = contactInternal + 0x624;
	unsigned char* pairHash = contactEngine + 0x34;
	const size_t bucketBytes = allocator.allocationSize(
		*reinterpret_cast<void**>(pairHash + 0x08));
	const size_t linkBytes = allocator.allocationSize(
		*reinterpret_cast<void**>(pairHash + 0x0c));
	const size_t entryBytes = allocator.allocationSize(
		*reinterpret_cast<void**>(pairHash + 0x14));
	printf("teardown contact_pair hash_storage buckets=%u links=%u entries=%u\n",
		static_cast<unsigned>(bucketBytes), static_cast<unsigned>(linkBytes),
		static_cast<unsigned>(entryBytes));
	sdk->releaseScene(*contactScene);
	const unsigned afterContactScene = allocator.outstanding();
	printf("teardown contact_pair pairs_before=%u outstanding_after=%u\n",
		contactPairs, afterContactScene);

	sdk->release();
	foundationSDK->release();
	if(afterContactScene != 15)
		return nxFail("contact-pair teardown did not return to the oracle's 15 outstanding blocks");
	return nxReportPairIdentity(pairDirectory);
	}
