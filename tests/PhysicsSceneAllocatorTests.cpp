// Which allocator owns the Scene, actor and shape blocks.
//
// The oracle allocates the Scene (0x710), the actor (0x50/0x18), the shape
// (0x228 and its 0x1c handle), the dynamic record (0x260), the group (0x110),
// NpScene's lock links and the Scene's 0xa8 auxiliary manager through the
// imported nxFoundationSDKAllocator ([0x101041bc], slot +8, free slot +0x14),
// and only OPCODE's pool, pruners and pending-shape array through
// phys_fn_004803, the SDK allocator bridge. A harness that hands one allocator
// to NxCreatePhysicsSDK cannot tell the two apart: with no Foundation yet, both
// end up at the same user allocator.
//
// This harness separates them. The Foundation is created first with counting
// allocator A, so nxFoundationSDKAllocator is A; NxCreatePhysicsSDK then gets
// counting allocator B, which only the 004803 bridge reaches. Every block is
// filled with 0xCD and tagged with its owner, so each phase reports how many
// blocks, of what sizes, each allocator saw, and `cross_frees` counts blocks
// freed through the allocator that did not allocate them.

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
#include <string.h>

typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32, NxUserOutputStream*, NxUserAllocator*);
typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned gCrossFrees = 0;

// Header in front of every block: {size, owner tag}. Eight bytes keeps the
// caller's pointer 8-aligned, as the CRT's is.
class NxCountingAllocator : public NxUserAllocator
	{
	public:
	enum { HISTORY = 256 };
	explicit NxCountingAllocator(unsigned tag) : mTag(tag), mAllocations(0), mFrees(0),
		mFailSize(0), mFailHits(0)
		{ memset(mAllocSizes, 0, sizeof(mAllocSizes)); memset(mFreedSizes, 0, sizeof(mFreedSizes)); }

	void failNextAllocationOfSize(unsigned size) { mFailSize = size; }
	unsigned failHits() const { return mFailHits; }

	virtual void* malloc(size_t size) { return allocate(size); }
	virtual void* malloc(size_t size, NxMemoryType) { return allocate(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int) { return allocate(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)
		{ return allocate(size); }

	virtual void* realloc(void* memory, size_t size)
		{
		void* fresh = allocate(size);
		if(memory && fresh)
			{
			const size_t old = header(memory)[0];
			memcpy(fresh, memory, old < size ? old : size);
			free(memory);
			}
		return fresh;
		}

	virtual void free(void* memory)
		{
		if(!memory)
			return;
		unsigned* h = header(memory);
		if(h[1] != mTag)
			++gCrossFrees;
		mFreedSizes[mFrees % HISTORY] = h[0];
		++mFrees;
		::free(h);
		}

	unsigned allocations() const { return mAllocations; }
	unsigned frees() const { return mFrees; }
	unsigned allocSize(unsigned index) const { return mAllocSizes[index % HISTORY]; }
	unsigned freedSize(unsigned index) const { return mFreedSizes[index % HISTORY]; }

	private:
	static unsigned* header(void* memory) { return static_cast<unsigned*>(memory) - 2; }

	void* allocate(size_t size)
		{
		if(mFailSize == size)
			{
			mFailSize = 0;
			++mFailHits;
			return 0;
			}
		unsigned* h = static_cast<unsigned*>(::malloc(size + 8));
		if(!h)
			return 0;
		h[0] = static_cast<unsigned>(size);
		h[1] = mTag;
		memset(h + 2, 0xcd, size);
		mAllocSizes[mAllocations % HISTORY] = static_cast<unsigned>(size);
		++mAllocations;
		return h + 2;
		}

	unsigned mTag;
	unsigned mAllocations;
	unsigned mFrees;
	unsigned mFailSize;
	unsigned mFailHits;
	unsigned mAllocSizes[HISTORY];
	unsigned mFreedSizes[HISTORY];
	};

static NxCountingAllocator gFoundationAllocator(0x46u);	// 'F', allocator A
static NxCountingAllocator gSdkAllocator(0x42u);		// 'B', allocator B

struct NxPhaseMark
	{
	unsigned aAllocs, aFrees, bAllocs, bFrees, cross;
	};

static NxPhaseMark nxMark()
	{
	NxPhaseMark mark = { gFoundationAllocator.allocations(), gFoundationAllocator.frees(),
		gSdkAllocator.allocations(), gSdkAllocator.frees(), gCrossFrees };
	return mark;
	}

static void nxPrintSizes(const char* phase, const char* label,
	const NxCountingAllocator& allocator, unsigned from, unsigned to, bool freed)
	{
	printf("alloc phase=%s %s=", phase, label);
	if(from == to)
		printf("none");
	for(unsigned i = from; i < to; ++i)
		printf("%s%x", i == from ? "" : ".",
			freed ? allocator.freedSize(i) : allocator.allocSize(i));
	printf("\n");
	}

static void nxReport(const char* phase, const NxPhaseMark& before)
	{
	const NxPhaseMark after = nxMark();
	printf("alloc phase=%s a_allocs=%u a_frees=%u b_allocs=%u b_frees=%u cross_frees=%u\n",
		phase, after.aAllocs - before.aAllocs, after.aFrees - before.aFrees,
		after.bAllocs - before.bAllocs, after.bFrees - before.bFrees,
		after.cross - before.cross);
	nxPrintSizes(phase, "a_alloc_sizes", gFoundationAllocator, before.aAllocs, after.aAllocs, false);
	nxPrintSizes(phase, "a_free_sizes", gFoundationAllocator, before.aFrees, after.aFrees, true);
	nxPrintSizes(phase, "b_alloc_sizes", gSdkAllocator, before.bAllocs, after.bAllocs, false);
	nxPrintSizes(phase, "b_free_sizes", gSdkAllocator, before.bFrees, after.bFrees, true);
	}

static unsigned nxReadWord(const unsigned char* bytes, unsigned offset)
	{
	unsigned value;
	memcpy(&value, bytes + offset, sizeof(value));
	return value;
	}

static int nxCheckSceneVisitedBufferGrowth(HMODULE physics, NxScene* publicScene)
	{
	wchar_t path[MAX_PATH];
	char hash[65];
	if(!GetModuleFileNameW(physics, path, MAX_PATH) || !nxSha256(path, hash))
		return nxFail("cannot identify Physics DLL for the direct 000503 call");
	static const char kOracleHash[] = "4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c";
	const unsigned entryRva = strcmp(hash, kOracleHash) == 0 ? 0x100a0 : 0x6c610;
	typedef void (__thiscall *OracleUpdateActorCountFn)(void*, unsigned);
	typedef void (__cdecl *CandidateUpdateActorCountFn)(void*, unsigned);
	const bool oracle = strcmp(hash, kOracleHash) == 0;
	OracleUpdateActorCountFn oracleUpdate = oracle
		? reinterpret_cast<OracleUpdateActorCountFn>(reinterpret_cast<unsigned char*>(physics) + entryRva) : 0;
	CandidateUpdateActorCountFn candidateUpdate = oracle ? 0
		: reinterpret_cast<CandidateUpdateActorCountFn>(reinterpret_cast<unsigned char*>(physics) + entryRva);
	unsigned char* wrapper = reinterpret_cast<unsigned char*>(publicScene);
	unsigned char* scene = *reinterpret_cast<unsigned char**>(wrapper + 0x24);
	if(!scene)
		return nxFail("public Scene has no internal Scene for row 000503");

	const unsigned initialCapacity = nxReadWord(scene, 4);
	const unsigned initialVisits = nxReadWord(scene, 8);
	const unsigned initialShared = nxReadWord(scene, 12);
	const unsigned initialStamp = nxReadWord(scene, 20);
	const unsigned noOpAllocs = gFoundationAllocator.allocations();
	const unsigned noOpFrees = gFoundationAllocator.frees();
	if(oracle) oracleUpdate(scene, initialCapacity); else candidateUpdate(scene, initialCapacity);
	if(gFoundationAllocator.allocations() != noOpAllocs
		|| gFoundationAllocator.frees() != noOpFrees
		|| nxReadWord(scene, 8) != initialVisits
		|| nxReadWord(scene, 12) != initialShared
		|| nxReadWord(scene, 20) != initialStamp)
		return nxFail("row 000503 equal-capacity call changed owned buffers or stamp");

	const unsigned request = initialCapacity + 1;
	const unsigned expectedCapacity = (request + 0x100u) & ~0xffu;
	const unsigned growthAllocs = gFoundationAllocator.allocations();
	const unsigned growthFrees = gFoundationAllocator.frees();
	if(oracle) oracleUpdate(scene, request); else candidateUpdate(scene, request);
	const unsigned capacity = nxReadWord(scene, 4);
	const unsigned visits = nxReadWord(scene, 8);
	const unsigned shared = nxReadWord(scene, 12);
	if(capacity != expectedCapacity || nxReadWord(scene, 20) != capacity
		|| gFoundationAllocator.allocations() - growthAllocs != 2
		|| gFoundationAllocator.frees() - growthFrees != (initialCapacity ? 2u : 0u))
		return nxFail("row 000503 growth capacity, stamp, or allocation order differs");
	for(unsigned i = 0; i < capacity; ++i)
		if(nxReadWord(reinterpret_cast<const unsigned char*>(visits), i * 4) != 0)
			return nxFail("row 000503 did not clear the full visited array");

	const unsigned containerOffsets[] = {0x50, 0x500, 0x510};
	for(unsigned i = 0; i < sizeof(containerOffsets) / sizeof(containerOffsets[0]); ++i)
		{
		const unsigned offset = containerOffsets[i];
		if(nxReadWord(scene, offset) != capacity || nxReadWord(scene, offset + 4) != 0
			|| nxReadWord(scene, offset + 8) != shared
			|| nxReadWord(scene, offset + 12) != 0xbf800000u)
			return nxFail("row 000503 did not rebind an embedded Scene container");
		}

	unsigned pruners = 0;
	for(unsigned i = 0; i < 4; ++i)
		{
		const unsigned pruner = nxReadWord(scene, 0x640 + i * 4);
		if(!pruner)
			continue;
		++pruners;
		if(i != 0)
			continue;
		const unsigned char* object = reinterpret_cast<const unsigned char*>(pruner);
		const unsigned capMatch = nxReadWord(object, 0x40) == capacity;
		const unsigned countMatch = nxReadWord(object, 0x44) == 0;
		const unsigned pointerMatch = nxReadWord(object, 0x48) == shared;
		const unsigned factorMatch = nxReadWord(object, 0x4c) == 0xbf800000u;
		const bool matches = capMatch && countMatch && pointerMatch && factorMatch;
		if(!matches)
			return nxFail("row 000503 did not rebind the original static Pruner buffer");
		}
	if(pruners < 2 || !nxReadWord(scene, 0x648))
		return nxFail("row 000503 fixture did not create live static and dynamic Pruners");

	const unsigned stableAllocs = gFoundationAllocator.allocations();
	const unsigned stableFrees = gFoundationAllocator.frees();
	if(oracle) oracleUpdate(scene, capacity); else candidateUpdate(scene, capacity);
	if(gFoundationAllocator.allocations() != stableAllocs
		|| gFoundationAllocator.frees() != stableFrees
		|| nxReadWord(scene, 8) != visits || nxReadWord(scene, 12) != shared
		|| nxReadWord(scene, 4) != capacity || nxReadWord(scene, 20) != capacity)
		return nxFail("row 000503 post-growth no-op changed the Scene state");

	printf("scene_visited_growth match=1 initial=%u grown=%u allocs=2 frees=%u containers=3 pruners=%u zeroed=%u\n",
		initialCapacity, capacity, initialCapacity ? 2u : 0u, pruners, capacity);
	return 0;
	}

int wmain(int argc, wchar_t** argv)
{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSceneAllocatorTests", pairDirectory, &physics);
	if(status) return status;

	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	if(!foundation) return nxFail("NxFoundation.dll is not loaded");
	CreateFoundationSDKFn createFoundation = reinterpret_cast<CreateFoundationSDKFn>(
		GetProcAddress(foundation, "NxCreateFoundationSDK"));
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createFoundation) return nxFail("NxCreateFoundationSDK is missing");
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");

	// Foundation first, with A: a later NxCreateFoundationSDK from inside
	// NxCreatePhysicsSDK finds the instance and leaves the allocator alone.
	NxPhaseMark mark = nxMark();
	if(!createFoundation(NX_FOUNDATION_SDK_VERSION, 0, &gFoundationAllocator))
		return nxFail("Foundation creation failed");
	nxReport("foundation_create", mark);

	mark = nxMark();
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &gSdkAllocator, 0);
	if(!sdk) return nxFail("SDK creation failed");
	nxReport("sdk_create", mark);

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	gFoundationAllocator.failNextAllocationOfSize(0x710);
	mark = nxMark();
	NxScene* failedScene = sdk->createScene(sceneDesc);
	const bool sceneAllocationFailureExact = failedScene == 0
		&& sdk->getNbScenes() == 0
		&& gFoundationAllocator.failHits() == 1;
	printf("alloc phase=scene_create_oom status=%s scenes=%u fail_hits=%u\n",
		failedScene ? "created" : "null", sdk->getNbScenes(), gFoundationAllocator.failHits());
	nxReport("scene_create_oom", mark);
	if(!sceneAllocationFailureExact)
		return nxFail("Scene allocation failure changed public result or left an SDK Scene");
	const unsigned sdkFailHitsBeforePool = gSdkAllocator.failHits();
	gSdkAllocator.failNextAllocationOfSize(0x1c); // NxPrunerProcessPool owner
	mark = nxMark();
	NxScene* sceneWithoutPrunerPool = sdk->createScene(sceneDesc);
	NxBoxShapeDesc poolFailureBox;
	NxActorDesc poolFailureActorDesc;
	poolFailureActorDesc.shapes.pushBack(&poolFailureBox);
	NxActor* poolFailureActor = sceneWithoutPrunerPool
		? sceneWithoutPrunerPool->createActor(poolFailureActorDesc) : 0;
	const unsigned scenesAfterPoolFailure = sdk->getNbScenes();
	const bool prunerPoolAllocationFailed = sceneWithoutPrunerPool != 0 && poolFailureActor != 0
		&& scenesAfterPoolFailure == 1
		&& gSdkAllocator.failHits() == sdkFailHitsBeforePool + 1;
	nxReport("pruner_pool_owner_oom", mark);
	if(sceneWithoutPrunerPool)
		sdk->releaseScene(*sceneWithoutPrunerPool);
	const bool prunerPoolFailureCleanup = sdk->getNbScenes() == 0;
	printf("alloc phase=pruner_pool_owner_oom status=%s scenes=%u fail_hits=%u after_release_scenes=%u\n",
		prunerPoolAllocationFailed ? "created" : "failed",
		scenesAfterPoolFailure, gSdkAllocator.failHits() - sdkFailHitsBeforePool, sdk->getNbScenes());
	if(!prunerPoolAllocationFailed || !prunerPoolFailureCleanup)
		return nxFail("Pruner process-pool owner allocation failure changed the public Scene lifecycle");
	mark = nxMark();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	nxReport("scene_create", mark);

	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&box);
	staticDesc.globalPose.t = NxVec3(2.0f, -1.0f, 4.0f);
	mark = nxMark();
	NxActor* staticActor = scene->createActor(staticDesc);
	if(!staticActor) return nxFail("static actor creation failed");
	nxReport("static_create", mark);

	NxBodyDesc body;
	NxActorDesc dynamicDesc;
	dynamicDesc.body = &body;
	dynamicDesc.density = 1.0f;
	dynamicDesc.shapes.pushBack(&box);
	dynamicDesc.globalPose.t = NxVec3(-3.0f, 2.0f, 1.0f);
	mark = nxMark();
	NxActor* dynamicActor = scene->createActor(dynamicDesc);
	if(!dynamicActor) return nxFail("dynamic actor creation failed");
	nxReport("dynamic_create", mark);

	NxBoxShapeDesc secondBox;
	secondBox.dimensions = NxVec3(2.0f, 1.0f, 1.0f);
	NxActorDesc multiDesc = dynamicDesc;
	multiDesc.shapes.pushBack(&secondBox);
	multiDesc.globalPose.t = NxVec3(7.0f, 1.0f, -2.0f);
	mark = nxMark();
	NxActor* multiActor = scene->createActor(multiDesc);
	if(!multiActor) return nxFail("two-box actor creation failed");
	nxReport("multi_create", mark);
	if(nxCheckSceneVisitedBufferGrowth(physics, scene))
		return 1;

	mark = nxMark();
	scene->releaseActor(*multiActor);
	nxReport("multi_release", mark);

	mark = nxMark();
	scene->releaseActor(*dynamicActor);
	nxReport("dynamic_release", mark);

	mark = nxMark();
	scene->releaseActor(*staticActor);
	nxReport("static_release", mark);

	mark = nxMark();
	sdk->releaseScene(*scene);
	nxReport("scene_release", mark);

	mark = nxMark();
	sdk->release();
	nxReport("sdk_release", mark);

	printf("alloc total a_allocs=%u a_frees=%u b_allocs=%u b_frees=%u cross_frees=%u\n",
		gFoundationAllocator.allocations(), gFoundationAllocator.frees(),
		gSdkAllocator.allocations(), gSdkAllocator.frees(), gCrossFrees);
	return nxReportPairIdentity(pairDirectory);
}
