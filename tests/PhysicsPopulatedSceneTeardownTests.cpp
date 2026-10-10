#include "PhysicsPairLoader.h"

#include <stdlib.h>
#include <string.h>
#include <new>

#include "NxFoundationSDK.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxFixedJointDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxUserContactReport.h"
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

static unsigned gPrunerOwnerCalls[2] = {0, 0};
static unsigned gPrunerOwnerFlags[2] = {0, 0};

class PrunerOwnerProbe
	{
	public:
	explicit PrunerOwnerProbe(unsigned index) : mIndex(index) {}
	virtual ~PrunerOwnerProbe()
		{
		if(mIndex < 2) ++gPrunerOwnerCalls[mIndex];
		}
	static void operator delete(void* memory)
		{
		const unsigned index = *reinterpret_cast<unsigned*>(
			static_cast<unsigned char*>(memory) + sizeof(void*));
		if(index < 2) ++gPrunerOwnerFlags[index];
		}
	unsigned mIndex;
	};

class TeardownContactReport : public NxUserContactReport
	{
	public:
	TeardownContactReport() : calls(0), events(0) {}
	virtual void onContactNotify(NxContactPair&, NxU32 eventFlags)
		{
		++calls;
		events |= eventFlags;
		printf("teardown contact_report callback=%u events=%08x\n",
			calls, eventFlags);
		}
	unsigned calls;
	unsigned events;
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

	if(afterContactScene != 15)
		return nxFail("contact-pair teardown did not return to the oracle's 15 outstanding blocks");

	// Seed one static and one selected-dynamic pruner section with synthetic
	// owner objects after releasing real actors. Their deleting-destructor
	// callback records phys_fn_001963's dispatch without running shape teardown
	// a second time or relying on private shape-owner side effects.
	NxScene* ownerScene = sdk->createScene(sceneDesc);
	if(!ownerScene) return nxFail("pruner-owner teardown scene creation failed");
	NxActor* ownerStatic = ownerScene->createActor(staticDesc);
	NxActor* ownerDynamic = ownerScene->createActor(dynamicDesc);
	if(!ownerStatic || !ownerDynamic)
		return nxFail("pruner-owner static/dynamic actor creation failed");
	ownerScene->releaseActor(*ownerStatic);
	ownerScene->releaseActor(*ownerDynamic);
	unsigned char* ownerWrapper = reinterpret_cast<unsigned char*>(ownerScene);
	unsigned char* ownerInternal = *reinterpret_cast<unsigned char**>(ownerWrapper + 0x24);
	unsigned char* ownerEngine = ownerInternal + 0x624;
	unsigned char* staticPruner = *reinterpret_cast<unsigned char**>(ownerEngine + 0x1c);
	unsigned char* dynamicPruner = *reinterpret_cast<unsigned char**>(ownerEngine + 0x24);
	if(!staticPruner) return nxFail("pruner-owner fixture did not create a static pruner");
	if(!dynamicPruner) return nxFail("pruner-owner fixture did not create a dynamic pruner");
	unsigned char* ownerPruners[2] = {staticPruner, dynamicPruner};
	unsigned char fakePrunables[2][0x2c];
	unsigned char ownerStorage[2][sizeof(PrunerOwnerProbe)];
	memset(fakePrunables, 0, sizeof(fakePrunables));
	for(unsigned i = 0; i < 2; ++i)
		{
		PrunerOwnerProbe* owner = new(ownerStorage[i]) PrunerOwnerProbe(i);
		*reinterpret_cast<void**>(fakePrunables[i] + 4) = owner;
		unsigned char* const pruner = ownerPruners[i];
		unsigned* const sections = reinterpret_cast<unsigned*>(pruner + 4);
		void** const objects = *reinterpret_cast<void***>(pruner + 0x18);
		if(!objects || sections[0] || sections[1] || sections[2] ||
			*reinterpret_cast<unsigned short*>(pruner + 0x10) != 0)
			return nxFail("pruner-owner pool was not empty with retained storage");
		objects[0] = fakePrunables[i];
		sections[1] = i == 0 ? 1 : 0;
		sections[2] = i == 1 ? 1 : 0;
		*reinterpret_cast<unsigned short*>(pruner + 0x10) = 1;
		*reinterpret_cast<unsigned short*>(pruner + 0x12) = 4;
		}
	const unsigned selectedPruner = *reinterpret_cast<unsigned*>(ownerEngine + 0x70);
	if(selectedPruner != 2)
		return nxFail("pruner-owner fixture selected dynamic pruner type changed");
	sdk->releaseScene(*ownerScene);
	const unsigned afterOwnerScene = allocator.outstanding();
	printf("teardown pruner_owner static_calls=%u dynamic_calls=%u flags=%u/%u selected=%u outstanding_after=%u\n",
		gPrunerOwnerCalls[0], gPrunerOwnerCalls[1],
		gPrunerOwnerFlags[0], gPrunerOwnerFlags[1], selectedPruner, afterOwnerScene);
	if(gPrunerOwnerCalls[0] != 1 || gPrunerOwnerCalls[1] != 1 ||
		gPrunerOwnerFlags[0] != 1 || gPrunerOwnerFlags[1] != 1)
		return nxFail("pruner-owner teardown did not dispatch both deleting destructors");
	if(afterOwnerScene != afterContactScene)
		return nxFail("pruner-owner teardown did not release scene allocations");

	// Destroy a Scene while its controller and generated kinematic actor are
	// still attached. This exercises the public controller-list teardown path
	// that explicit releaseController calls do not cover.
	NxScene* controllerScene = sdk->createScene(sceneDesc);
	if(!controllerScene)
		return nxFail("controller-owner teardown scene creation failed");
	alignas(4) unsigned char controllerDescStorage[0x80] = {};
	*reinterpret_cast<unsigned*>(controllerDescStorage + 0x30) = 0x3f000000;
	*reinterpret_cast<unsigned*>(controllerDescStorage + 0x34) = 0x3f800000;
	*reinterpret_cast<unsigned*>(controllerDescStorage + 0x38) = 0x3f000000;
	NxController* controller = controllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(controllerDescStorage));
	if(!controller)
		return nxFail("controller-owner teardown controller creation failed");
	const unsigned controllerActorsBefore = controllerScene->getNbActors();
	const unsigned controllerBlocksBefore = allocator.outstanding();
	sdk->releaseScene(*controllerScene);
	const unsigned controllerBlocksAfter = allocator.outstanding();
	printf("teardown controller_owner actors_before=%u outstanding_before=%u outstanding_after=%u delta=%d\n",
		controllerActorsBefore, controllerBlocksBefore, controllerBlocksAfter,
		static_cast<int>(controllerBlocksAfter) - static_cast<int>(controllerBlocksBefore));
	if(controllerActorsBefore != 1)
		return nxFail("controller-owner teardown did not retain its generated actor");

	// Keep a user callback installed while a touching pair is destroyed with
	// the Scene. The oracle fires its outstanding report records from the
	// Scene deleting destructor before it tears down the actors.
	TeardownContactReport teardownReport;
	NxSceneDesc reportSceneDesc;
	reportSceneDesc.setToDefault();
	reportSceneDesc.userContactReport = &teardownReport;
	NxScene* reportScene = sdk->createScene(reportSceneDesc);
	if(!reportScene) return nxFail("contact-report teardown scene creation failed");
	NxActor* reportStatic = reportScene->createActor(contactStaticDesc);
	NxActor* reportDynamic = reportScene->createActor(contactDynamicDesc);
	if(!reportStatic || !reportDynamic)
		return nxFail("contact-report teardown actors failed to create");
	reportScene->setActorPairFlags(*reportStatic, *reportDynamic,
		NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH | NX_NOTIFY_ON_END_TOUCH);
	reportScene->simulate(0.125f);
	if(!reportScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
		return nxFail("contact-report teardown simulation did not fetch");
	const unsigned reportCallbacksBeforeRelease = teardownReport.calls;
	sdk->releaseScene(*reportScene);
	printf("teardown contact_report before_release=%u after_release=%u events=%08x\n",
		reportCallbacksBeforeRelease, teardownReport.calls, teardownReport.events);
	if(teardownReport.calls == reportCallbacksBeforeRelease)
		return nxFail("scene teardown did not deliver its pending contact report");

	// Keep a registered joint alive until Scene destruction. phys_fn_000606
	// walks both joint lists and deletes any joints still registered after the
	// actor teardown path.
	NxScene* jointScene = sdk->createScene(sceneDesc);
	if(!jointScene) return nxFail("joint-list teardown scene creation failed");
	NxActor* jointActor = jointScene->createActor(dynamicDesc);
	if(!jointActor) return nxFail("joint-list teardown dynamic actor failed");
	NxFixedJointDesc fixedDesc;
	fixedDesc.setToDefault();
	fixedDesc.actor[0] = jointActor;
	NxJoint* retainedJoint = jointScene->createJoint(fixedDesc);
	if(!retainedJoint) return nxFail("joint-list teardown fixed joint failed");
	const unsigned jointBlocksBefore = allocator.outstanding();
	sdk->releaseScene(*jointScene);
	const unsigned jointBlocksAfter = allocator.outstanding();
	printf("teardown joint_owner joint_created=1 release_delta=%d\n",
		static_cast<int>(jointBlocksAfter) - static_cast<int>(jointBlocksBefore));

	// Preserve the body record while the public actor follows its normal teardown
	// path. Clearing the pose's record link makes nxActorDestroy remove the actor
	// and its root while leaving the record in [Scene+0x56c, Scene+0x570), where
	// phys_fn_000602 must still destroy and free it.
	NxScene* retainedBodyScene = sdk->createScene(sceneDesc);
	if(!retainedBodyScene)
		return nxFail("retained-body-record scene creation failed");
	NxActor* retainedBodyActor = retainedBodyScene->createActor(dynamicDesc);
	if(!retainedBodyActor)
		return nxFail("retained-body-record actor creation failed");
	unsigned char* retainedBodyWrapper =
		reinterpret_cast<unsigned char*>(retainedBodyScene);
	unsigned char* retainedBodyInternal =
		*reinterpret_cast<unsigned char**>(retainedBodyWrapper + 0x24);
	void** actorBegin = *reinterpret_cast<void***>(retainedBodyInternal + 0x55c);
	void** actorEnd = *reinterpret_cast<void***>(retainedBodyInternal + 0x560);
	void** bodyRecords = *reinterpret_cast<void***>(retainedBodyInternal + 0x56c);
	void** bodyRecordEnd = *reinterpret_cast<void***>(retainedBodyInternal + 0x570);
	unsigned char* retainedBodyPose =
		*reinterpret_cast<unsigned char**>(
			reinterpret_cast<unsigned char*>(retainedBodyActor) + 0x14);
	unsigned char* retainedBodyRecord = retainedBodyPose
		? *reinterpret_cast<unsigned char**>(retainedBodyPose + 8) : 0;
	if(!actorBegin || !actorEnd || actorEnd - actorBegin != 1 ||
		!bodyRecords || !bodyRecordEnd || bodyRecordEnd - bodyRecords != 1 ||
		bodyRecords[0] != retainedBodyRecord ||
		!allocator.allocationSize(retainedBodyRecord))
		return nxFail("retained-body-record fixture did not match the oracle layout");
	const size_t retainedBodyRecordBytes =
		allocator.allocationSize(retainedBodyRecord);
	*reinterpret_cast<unsigned char**>(retainedBodyPose + 8) = 0;
	sdk->releaseScene(*retainedBodyScene);
	const size_t retainedBodyRecordAfter =
		allocator.allocationSize(retainedBodyRecord);
	printf("teardown retained_body_record bytes=%u freed=%u\n",
		static_cast<unsigned>(retainedBodyRecordBytes),
		retainedBodyRecordAfter == 0 ? 1u : 0u);
	if(retainedBodyRecordAfter != 0)
		return nxFail("Scene teardown retained its orphaned dynamic body record");

	sdk->release();
	foundationSDK->release();
	return nxReportPairIdentity(pairDirectory);
	}
