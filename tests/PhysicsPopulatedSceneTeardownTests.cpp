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

class TeardownTriggerReport : public NxUserTriggerReport
	{
	public:
	TeardownTriggerReport() : calls(0) {}
	virtual void onTrigger(NxShape&, NxShape&, NxTriggerFlag)
		{
		++calls;
		}
	unsigned calls;
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
	// Releasing one dynamic actor returns its shape, actor, and dynamic-record
	// IDs to three Scene-owned LIFO arrays. Keep the Scene until its deleting
	// destructor so all three backing allocations are live at teardown.
	NxScene* recycledIdScene = sdk->createScene(sceneDesc);
	if(!recycledIdScene) return nxFail("recycled-ID teardown scene creation failed");
	NxActor* recycledIdActor = recycledIdScene->createActor(dynamicDesc);
	if(!recycledIdActor) return nxFail("recycled-ID teardown actor creation failed");
	unsigned char* recycledIdInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(recycledIdScene) + 0x24);
	recycledIdScene->releaseActor(*recycledIdActor);
	void* recycledShapeIds = *reinterpret_cast<void**>(recycledIdInternal + 0x6e8);
	void* recycledActorIds = *reinterpret_cast<void**>(recycledIdInternal + 0x6d4);
	void* recycledBodyIds = *reinterpret_cast<void**>(recycledIdInternal + 0x6fc);
	const size_t recycledShapeBytes = allocator.allocationSize(recycledShapeIds);
	const size_t recycledActorBytes = allocator.allocationSize(recycledActorIds);
	const size_t recycledBodyBytes = allocator.allocationSize(recycledBodyIds);
	if(!recycledShapeBytes || !recycledActorBytes || !recycledBodyBytes)
		return nxFail("released actor did not retain all three recycled-ID arrays");
	sdk->releaseScene(*recycledIdScene);
	const bool recycledShapeFreed = allocator.allocationSize(recycledShapeIds) == 0;
	const bool recycledActorFreed = allocator.allocationSize(recycledActorIds) == 0;
	const bool recycledBodyFreed = allocator.allocationSize(recycledBodyIds) == 0;
	printf("teardown recycled_id_buffers shape_bytes=%u shape_freed=%u "
		"actor_bytes=%u actor_freed=%u body_bytes=%u body_freed=%u\n",
		static_cast<unsigned>(recycledShapeBytes), recycledShapeFreed ? 1u : 0u,
		static_cast<unsigned>(recycledActorBytes), recycledActorFreed ? 1u : 0u,
		static_cast<unsigned>(recycledBodyBytes), recycledBodyFreed ? 1u : 0u);
	if(!recycledShapeFreed || !recycledActorFreed || !recycledBodyFreed)
		return nxFail("scene teardown did not free its recycled-ID arrays");
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
	void* contactRootBuffer = *reinterpret_cast<void**>(contactInternal + 0x57c);
	const size_t contactRootBytes = allocator.allocationSize(contactRootBuffer);
	const unsigned beforeContactRelease = allocator.outstanding();
	sdk->releaseScene(*contactScene);
	const unsigned afterContactScene = allocator.outstanding();
	const int contactReleaseDelta = static_cast<int>(afterContactScene) -
		static_cast<int>(beforeContactRelease);
	printf("teardown contact_pair pairs_before=%u release_delta=%d root_bytes=%u root_freed=%u\n",
		contactPairs, contactReleaseDelta, static_cast<unsigned>(contactRootBytes),
		allocator.allocationSize(contactRootBuffer) == 0);

	if(contactReleaseDelta != -54)
		return nxFail("contact-pair teardown did not release the oracle's 54 scene-owned blocks");

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
	const unsigned beforeOwnerRelease = allocator.outstanding();
	sdk->releaseScene(*ownerScene);
	const unsigned afterOwnerScene = allocator.outstanding();
	const int ownerReleaseDelta = static_cast<int>(afterOwnerScene) -
		static_cast<int>(beforeOwnerRelease);
	printf("teardown pruner_owner static_calls=%u dynamic_calls=%u flags=%u/%u selected=%u release_delta=%d\n",
		gPrunerOwnerCalls[0], gPrunerOwnerCalls[1],
		gPrunerOwnerFlags[0], gPrunerOwnerFlags[1], selectedPruner, ownerReleaseDelta);
	if(gPrunerOwnerCalls[0] != 1 || gPrunerOwnerCalls[1] != 1 ||
		gPrunerOwnerFlags[0] != 1 || gPrunerOwnerFlags[1] != 1)
		return nxFail("pruner-owner teardown did not dispatch both deleting destructors");
	if(ownerReleaseDelta != -33)
		return nxFail("pruner-owner teardown did not release 33 scene-owned allocations");

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
	const int controllerReleaseDelta = static_cast<int>(controllerBlocksAfter) -
		static_cast<int>(controllerBlocksBefore);
	printf("teardown controller_owner actors_before=%u release_delta=%d\n",
		controllerActorsBefore, controllerReleaseDelta);
	if(controllerActorsBefore != 1)
		return nxFail("controller-owner teardown did not retain its generated actor");
	if(controllerReleaseDelta != -34)
		return nxFail("controller-owner teardown did not release 34 scene-owned allocations");

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
	unsigned char* reportInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(reportScene) + 0x24);
	void* bufferedReportStorage = reportInternal
		? *reinterpret_cast<void**>(reportInternal + 0x60c) : 0;
	const size_t bufferedReportBytes = allocator.allocationSize(bufferedReportStorage);
	if(!bufferedReportStorage || !bufferedReportBytes)
		return nxFail("contact-report teardown did not retain the buffered report allocation");
	sdk->releaseScene(*reportScene);
	const bool bufferedReportFreed =
		allocator.allocationSize(bufferedReportStorage) == 0;
	printf("teardown contact_report before_release=%u after_release=%u events=%08x\n",
		reportCallbacksBeforeRelease, teardownReport.calls, teardownReport.events);
	printf("teardown contact_report_buffer bytes=%u freed=%u\n",
		static_cast<unsigned>(bufferedReportBytes), bufferedReportFreed ? 1u : 0u);
	if(teardownReport.calls == reportCallbacksBeforeRelease)
		return nxFail("scene teardown did not deliver its pending contact report");
	if(!bufferedReportFreed)
		return nxFail("scene teardown did not free its buffered contact-report allocation");

	// Generate a queued trigger event and an active simulation root, then release
	// the Scene while both backing vectors remain allocated at +0x5fc and +0x57c.
	TeardownTriggerReport teardownTriggerReport;
	NxSceneDesc triggerSceneDesc;
	triggerSceneDesc.setToDefault();
	triggerSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	triggerSceneDesc.userTriggerReport = &teardownTriggerReport;
	NxScene* triggerScene = sdk->createScene(triggerSceneDesc);
	if(!triggerScene) return nxFail("trigger teardown scene creation failed");
	NxBoxShapeDesc triggerBox;
	triggerBox.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	triggerBox.shapeFlags = NX_TRIGGER_ON_ENTER | NX_TRIGGER_ON_STAY | NX_TRIGGER_ON_LEAVE;
	NxActorDesc triggerActorDesc;
	triggerActorDesc.shapes.pushBack(&triggerBox);
	if(!triggerScene->createActor(triggerActorDesc))
		return nxFail("trigger teardown actor creation failed");
	NxSphereShapeDesc triggerOtherShape;
	triggerOtherShape.radius = 0.25f;
	NxActorDesc triggerOtherDesc;
	triggerOtherDesc.body = &body;
	triggerOtherDesc.density = 1.0f;
	triggerOtherDesc.globalPose.t = NxVec3(-2.0f, 0.0f, 0.0f);
	triggerOtherDesc.shapes.pushBack(&triggerOtherShape);
	NxActor* triggerOther = triggerScene->createActor(triggerOtherDesc);
	if(!triggerOther) return nxFail("trigger teardown dynamic actor creation failed");
	triggerOther->setLinearVelocity(NxVec3(8.0f, 0.0f, 0.0f));
	for(unsigned step = 0; step != 10; ++step)
		{
		triggerScene->simulate(0.05f);
		if(!triggerScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("trigger teardown simulation did not fetch");
		}
	unsigned char* triggerInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(triggerScene) + 0x24);
	void* triggerBuffer = triggerInternal ? *reinterpret_cast<void**>(triggerInternal + 0x5fc) : 0;
	void* activeRootBuffer = triggerInternal ? *reinterpret_cast<void**>(triggerInternal + 0x57c) : 0;
	const size_t triggerBufferBytes = allocator.allocationSize(triggerBuffer);
	const size_t activeRootBufferBytes = allocator.allocationSize(activeRootBuffer);
	if(!triggerBuffer || !triggerBufferBytes || !activeRootBuffer || !activeRootBufferBytes)
		return nxFail("trigger teardown did not retain both Scene buffers");
	sdk->releaseScene(*triggerScene);
	const bool triggerBufferFreed = allocator.allocationSize(triggerBuffer) == 0;
	const bool activeRootBufferFreed = allocator.allocationSize(activeRootBuffer) == 0;
	printf("teardown trigger_buffer bytes=%u freed=%u callbacks=%u\n",
		static_cast<unsigned>(triggerBufferBytes), triggerBufferFreed ? 1u : 0u,
		teardownTriggerReport.calls);
	printf("teardown active_root_buffer bytes=%u freed=%u\n",
		static_cast<unsigned>(activeRootBufferBytes), activeRootBufferFreed ? 1u : 0u);
	if(!teardownTriggerReport.calls)
		return nxFail("trigger teardown fixture did not deliver a trigger callback");
	if(!triggerBufferFreed || !activeRootBufferFreed)
		return nxFail("scene teardown did not free its trigger and active-root buffers");


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

	// Coherent broad-phase queries lazily allocate the SweepAndPrune cache at
	// Scene+0x650. phys_fn_001974 destroys that cache and returns its storage
	// through the SDK allocator during the Scene deleting-destructor chain.
	NxSceneDesc coherentSceneDesc;
	coherentSceneDesc.setToDefault();
	coherentSceneDesc.broadPhase = NX_BROADPHASE_COHERENT;
	NxScene* coherentScene = sdk->createScene(coherentSceneDesc);
	if(!coherentScene) return nxFail("coherent teardown scene creation failed");
	NxActor* coherentStatic = coherentScene->createActor(contactStaticDesc);
	NxActor* coherentDynamic = coherentScene->createActor(contactDynamicDesc);
	if(!coherentStatic || !coherentDynamic)
		return nxFail("coherent teardown actors failed to create");
	coherentScene->simulate(0.125f);
	if(!coherentScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
		return nxFail("coherent teardown simulation did not fetch");
	unsigned char* coherentWrapper = reinterpret_cast<unsigned char*>(coherentScene);
	unsigned char* coherentInternal = *reinterpret_cast<unsigned char**>(coherentWrapper + 0x24);
	void* coherentCache = *reinterpret_cast<void**>(coherentInternal + 0x650);
	const size_t coherentCacheBytes = allocator.allocationSize(coherentCache);
	if(!coherentCache || !coherentCacheBytes)
		return nxFail("coherent teardown fixture did not retain a tracked cache");
	sdk->releaseScene(*coherentScene);
	const size_t coherentCacheBytesAfterRelease = allocator.allocationSize(coherentCache);
	printf("teardown coherent_cache bytes=%u freed=%u\n",
		static_cast<unsigned>(coherentCacheBytes), coherentCacheBytesAfterRelease ? 0u : 1u);
	if(coherentCacheBytesAfterRelease != 0)
		return nxFail("coherent cache teardown did not free the retained SweepAndPrune object");

	sdk->release();
	foundationSDK->release();
	return nxReportPairIdentity(pairDirectory);
	}
