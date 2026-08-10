#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxUserAllocator.h"
#include "NxUserOutputStream.h"
#include "NxUserDebugRenderer.h"
#include "NxDebugRenderable.h"
#include "NxMaterial.h"

// The Phase 2 shared runtime components that are reachable without a scene:
// FluidSupport (the nine NxFluid* exports) and Visualization (the three
// PhysicsSDK rows behind them and the NpPhysicsSDK::visualize wrapper).
//
// Everything the fluid shim draws lands in the NxDebugRenderable the Foundation
// creates on first use, and the only way back out of the Physics API is
// NxPhysicsSDK::visualize, which walks it into an NxUserDebugRenderer. That
// makes one differential cover both components: the draw exports are gated by
// what the renderer is handed, and the visualize path is gated by whether it is
// handed anything at all.
//
// None of the nine exports checks an input and none checks the SDK, so this
// harness never calls one before NxCreatePhysicsSDK has returned: the oracle
// faults if it does, and a differential cannot gate a fault.

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);
typedef void (NX_CALL_CONV *FluidAssertFn)();
typedef void* (NX_CALL_CONV *FluidPAllocFn)(size_t);
typedef void (NX_CALL_CONV *FluidFreeFn)(void*);
typedef void (NX_CALL_CONV *FluidDebugLineFn)(const NxF32*, NxU32);
typedef void (NX_CALL_CONV *FluidDebugTriangleFn)(const NxF32*, NxU32);
typedef void (NX_CALL_CONV *FluidDebugPointFn)(const NxF32*, NxF32, NxU32);
typedef void (NX_CALL_CONV *FluidDebugSphereFn)(const NxF32*, NxF32, NxU32);
typedef void (NX_CALL_CONV *FluidDebugArrowFn)(const NxF32*, const NxF32*, NxF32, NxF32, NxU32);
typedef void (NX_CALL_CONV *FluidDebugAABBFn)(const NxF32*, NxU32);

// Refuses anything above the cap so the allocation-failure case is the same on
// both sides of the differential rather than depending on the heap.
static const size_t nxAllocationCap = 0x10000000;

class RecordingAllocator : public NxUserAllocator
	{
	public:
	RecordingAllocator(): mallocCalls(0), refusedCalls(0), reallocCalls(0), freeCalls(0) {}

	void* mallocDEBUG(size_t size, const char*, int)					{ return allocate(size); }
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)	{ return allocate(size); }
	void* malloc(size_t size)											{ return allocate(size); }
	void* malloc(size_t size, NxMemoryType)								{ return allocate(size); }
	// resize's shrink-to-fit is the only thing in Phase 2 that reaches the
	// allocator's realloc slot, so purgeMaterials is what gates it.
	void* realloc(void* memory, size_t size)							{ ++reallocCalls; return ::realloc(memory, size); }

	void free(void* memory)
		{
		++freeCalls;
		::free(memory);
		}

	unsigned mallocCalls;
	unsigned refusedCalls;
	unsigned reallocCalls;
	unsigned freeCalls;

	private:
	void* allocate(size_t size)
		{
		if(size > nxAllocationCap)
			{
			++refusedCalls;
			return 0;
			}
		++mallocCalls;
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

	NxAssertResponse reportAssertViolation(const char*, const char*, int)	{ ++asserts; return NX_AR_CONTINUE; }
	void print(const char*)	{ ++prints; }

	unsigned errors;
	unsigned asserts;
	unsigned prints;
	NxErrorCode code;
	int line;
	char message[96];
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

static void reportStream(const char* step, const RecordingOutputStream& stream)
	{
	printf("step=%s errors=%u code=%d line=%d file=%s message=%s\n",
		step, stream.errors, static_cast<int>(stream.code), stream.line, stream.file, stream.message);
	}

// Copies out what it is handed, because the renderable is only valid for the
// duration of the callback. The renderable itself is kept so the sphere's
// argument list can be replayed against it.
static const unsigned nxMaxCapturedLines = 512;

class RecordingRenderer : public NxUserDebugRenderer
	{
	public:
	RecordingRenderer() { renderable = 0; reset(); }

	void reset() const
		{
		datas = 0;
		points = 0;
		lines = 0;
		triangles = 0;
		captured = 0;
		}

	void renderData(const NxDebugRenderable& data) const
		{
		++datas;
		points += data.getNbPoints();
		triangles += data.getNbTriangles();
		renderable = const_cast<NxDebugRenderable*>(&data);

		NxU32 count = data.getNbLines();
		const NxDebugLine* source = data.getLines();
		for(NxU32 i = 0; i < count && captured < nxMaxCapturedLines; ++i)
			line[captured++] = source[i];
		lines += count;
		}

	mutable unsigned datas;
	mutable unsigned points;
	mutable unsigned lines;
	mutable unsigned triangles;
	mutable unsigned captured;
	mutable NxDebugRenderable* renderable;
	mutable NxDebugLine line[nxMaxCapturedLines];
	};

// FNV-1a over the raw bytes of the lines added since `from`, so a step whose
// delta is too long to print verbatim is still compared byte for byte.
static unsigned lineDigest(const RecordingRenderer& renderer, unsigned from)
	{
	unsigned hash = 2166136261u;
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(renderer.line + from);
	size_t size = (renderer.captured - from) * sizeof(NxDebugLine);
	for(size_t i = 0; i < size; ++i)
		{
		hash ^= bytes[i];
		hash *= 16777619u;
		}
	return hash;
	}

// Suppresses the per-line dump for a step whose coordinates are the
// Foundation's to gate rather than this component's.
static const unsigned nxNoLineDump = 0xffffffffu;

static void reportVisualize(const char* step, NxPhysicsSDK* sdk, RecordingRenderer& renderer, unsigned printFrom)
	{
	renderer.reset();
	sdk->visualize(renderer);
	printf("step=%s datas=%u points=%u lines=%u triangles=%u digest=0x%08x\n",
		step, renderer.datas, renderer.points, renderer.lines, renderer.triangles,
		lineDigest(renderer, printFrom < renderer.captured ? printFrom : renderer.captured));

	unsigned printed = 0;
	for(unsigned i = printFrom; i < renderer.captured && printed < 4; ++i, ++printed)
		printf("  line=%u p0=%.6f,%.6f,%.6f p1=%.6f,%.6f,%.6f color=0x%08x\n", i,
			renderer.line[i].p0.x, renderer.line[i].p0.y, renderer.line[i].p0.z,
			renderer.line[i].p1.x, renderer.line[i].p1.y, renderer.line[i].p1.z,
			static_cast<unsigned>(renderer.line[i].color));
	}

static void reportAllocator(const char* step, const RecordingAllocator& allocator)
	{
	printf("step=%s alloc_calls=%u refused_calls=%u realloc_calls=%u free_calls=%u\n",
		step, allocator.mallocCalls, allocator.refusedCalls, allocator.reallocCalls, allocator.freeCalls);
	}

int wmain(int argc, wchar_t** argv)
	{
	// Unbuffered, so a fault truncates the transcript at the call that faulted
	// rather than at whatever stdio had last flushed.
	setvbuf(stdout, 0, _IONBF, 0);

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsCoreClusterTests", pairDirectory, &physics);
	if(status)
		return status;

	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	FluidAssertFn fluidAssert = reinterpret_cast<FluidAssertFn>(GetProcAddress(physics, "NxFluidAssert"));
	FluidPAllocFn fluidPAlloc = reinterpret_cast<FluidPAllocFn>(GetProcAddress(physics, "NxFluidPAlloc"));
	FluidFreeFn fluidFree = reinterpret_cast<FluidFreeFn>(GetProcAddress(physics, "NxFluidFree"));
	FluidDebugLineFn debugLine = reinterpret_cast<FluidDebugLineFn>(GetProcAddress(physics, "NxFluidDebugLine"));
	FluidDebugTriangleFn debugTriangle = reinterpret_cast<FluidDebugTriangleFn>(GetProcAddress(physics, "NxFluidDebugTriangle"));
	FluidDebugPointFn debugPoint = reinterpret_cast<FluidDebugPointFn>(GetProcAddress(physics, "NxFluidDebugPoint"));
	FluidDebugSphereFn debugSphere = reinterpret_cast<FluidDebugSphereFn>(GetProcAddress(physics, "NxFluidDebugSphere"));
	FluidDebugArrowFn debugArrow = reinterpret_cast<FluidDebugArrowFn>(GetProcAddress(physics, "NxFluidDebugArrow"));
	FluidDebugAABBFn debugAABB = reinterpret_cast<FluidDebugAABBFn>(GetProcAddress(physics, "NxFluidDebugAABB"));

	printf("resolved create=%d assert=%d palloc=%d free=%d line=%d triangle=%d point=%d sphere=%d arrow=%d aabb=%d\n",
		createSDK ? 1 : 0, fluidAssert ? 1 : 0, fluidPAlloc ? 1 : 0, fluidFree ? 1 : 0,
		debugLine ? 1 : 0, debugTriangle ? 1 : 0, debugPoint ? 1 : 0, debugSphere ? 1 : 0,
		debugArrow ? 1 : 0, debugAABB ? 1 : 0);
	if(!createSDK || !fluidAssert || !fluidPAlloc || !fluidFree || !debugLine || !debugTriangle
		|| !debugPoint || !debugSphere || !debugArrow || !debugAABB)
		{
		FreeLibrary(physics);
		return nxFail("a Phase 2 shared runtime export is missing from NxPhysics.dll");
		}

	// One byte in the oracle. All this can gate is that it returns, leaves the
	// stack to the caller, and needs no SDK.
	fluidAssert();
	printf("step=assert_before_create returned=1\n");

	RecordingAllocator allocator;
	RecordingOutputStream stream;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, &stream);
	printf("step=create sdk=%s\n", sdk ? "nonnull" : "null");
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("create returned null");
		}

	RecordingRenderer renderer;

	// No draw has run, so no renderable exists yet and the walk has nothing to
	// hand the renderer. This is the case that separates "visualize forwards"
	// from "visualize renders": both report zero lines, only one reports zero
	// renderables.
	reportVisualize("visualize_before_any_draw", sdk, renderer, 0);

	static const NxF32 linePoints[6] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
	debugLine(linePoints, 0xff0000ff);
	reportVisualize("line", sdk, renderer, 0);

	static const NxF32 trianglePoints[9] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	debugTriangle(trianglePoints, 0xff00ff00);
	reportVisualize("triangle", sdk, renderer, 1);

	static const NxF32 point[3] = { 10.0f, 20.0f, 30.0f };
	debugPoint(point, 0.5f, 0xffff0000);
	reportVisualize("point", sdk, renderer, 4);

	static const NxF32 bounds[6] = { -1.0f, -2.0f, -3.0f, 4.0f, 5.0f, 6.0f };
	debugAABB(bounds, 0xffffff00);
	reportVisualize("aabb", sdk, renderer, 7);

	static const NxF32 arrowPosition[3] = { 1.0f, 1.0f, 1.0f };
	static const NxF32 arrowDirection[3] = { 0.0f, 1.0f, 0.0f };
	debugArrow(arrowPosition, arrowDirection, 2.0f, 0.25f, 0xff00ffff);
	reportVisualize("arrow", sdk, renderer, 19);

	// The sphere is the one draw whose coordinates this component does not
	// determine: addCircle computes them with the Foundation's x87 sine/cosine
	// and the pinned inline `M * src + t`, and the shipped and the rebuilt
	// NxFoundation.dll disagree there by one ULP on 3 of the 120 points as soon
	// as the rotation is not the identity. That divergence is real and is
	// recorded in the evidence, but it belongs to the Foundation, so gating the
	// sphere on it would gate the wrong component.
	//
	// What FluidSupport owns is the argument list, and that is what is gated
	// here: the same renderable is asked, in the same process and through the
	// same DLL pair, to draw the three circles this reconstruction claims, and
	// the replayed batch must come out byte identical to the shipped one.
	static const NxF32 sphereCenter[3] = { 3.0f, -4.0f, 5.0f };
	unsigned sphereFirst = renderer.captured;
	debugSphere(sphereCenter, 2.0f, 0xffffffff);
	reportVisualize("sphere", sdk, renderer, nxNoLineDump);

	unsigned sphereCount = renderer.captured - sphereFirst;
	unsigned replayFirst = renderer.captured;
	if(!renderer.renderable)
		{
		FreeLibrary(physics);
		return nxFail("no renderable was handed to the renderer");
		}
	NxVec3 center(sphereCenter[0], sphereCenter[1], sphereCenter[2]);
	NxMat34 claimed[3];
	claimed[0].M.setRow(0, NxVec3(1.0f, 0.0f, 0.0f));
	claimed[0].M.setRow(1, NxVec3(0.0f, 1.0f, 0.0f));
	claimed[0].M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	claimed[1].M.setRow(0, NxVec3(0.0f, 0.0f, 1.0f));
	claimed[1].M.setRow(1, NxVec3(0.0f, 1.0f, 0.0f));
	claimed[1].M.setRow(2, NxVec3(1.0f, 0.0f, 0.0f));
	claimed[2].M.setRow(0, NxVec3(1.0f, 0.0f, 0.0f));
	claimed[2].M.setRow(1, NxVec3(0.0f, 0.0f, 1.0f));
	claimed[2].M.setRow(2, NxVec3(0.0f, 1.0f, 0.0f));
	for(int i = 0; i < 3; ++i)
		{
		claimed[i].t = center;
		renderer.renderable->addCircle(40, claimed[i], 0xffffffff, 2.0f, false);
		}
	reportVisualize("sphere_replay", sdk, renderer, nxNoLineDump);

	unsigned replayCount = renderer.captured - replayFirst;
	unsigned mismatches = sphereCount != replayCount ? sphereCount + replayCount : 0;
	for(unsigned i = 0; !mismatches && i < sphereCount; ++i)
		if(memcmp(&renderer.line[sphereFirst + i], &renderer.line[replayFirst + i], sizeof(NxDebugLine)) != 0)
			++mismatches;
	printf("step=sphere_arguments drawn=%u replayed=%u mismatches=%u\n",
		sphereCount, replayCount, mismatches);

	reportAllocator("draws.allocator", allocator);

	// The allocator pair is typed malloc and free, straight through, with no
	// null check on either side.
	void* block = fluidPAlloc(64);
	printf("step=palloc block=%s\n", block ? "nonnull" : "null");
	reportAllocator("palloc.allocator", allocator);
	if(block)
		memset(block, 0, 64);
	fluidFree(block);
	reportAllocator("free.allocator", allocator);

	void* refused = fluidPAlloc(nxAllocationCap + 1);
	printf("step=palloc_refused block=%s\n", refused ? "nonnull" : "null");
	reportAllocator("palloc_refused.allocator", allocator);

	sdk->release();
	printf("step=release\n");
	reportAllocator("release.allocator", allocator);

	// The destructor hands the cached renderable back through
	// releaseDebugRenderable, which takes it by reference and nulls it, so the
	// next SDK starts with no renderable and the next draw creates a fresh one.
	NxPhysicsSDK* second = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, &stream);
	printf("step=recreate sdk=%s\n", second ? "nonnull" : "null");
	if(!second)
		{
		FreeLibrary(physics);
		return nxFail("recreate returned null");
		}
	reportVisualize("visualize_after_recreate", second, renderer, 0);
	debugLine(linePoints, 0xff0000ff);
	reportVisualize("line_after_recreate", second, renderer, 0);

	// ---- Factory-A: collision groups, and the material read/add pair ----
	//
	// Both halves are read back through the public API, so what is gated is the
	// recovered state and not the reconstruction's own bookkeeping: the group
	// mask through getGroupCollisionFlag, the material array through
	// getNbMaterials and getMaterial. The oracle's constructor fills all 32 group
	// words with 0xffffffff, so every pair starts enabled -- the case the old
	// placeholder got exactly backwards.

	printf("step=groups_default self=%d pair=%d high=%d cross=%d\n",
		second->getGroupCollisionFlag(0, 0) ? 1 : 0,
		second->getGroupCollisionFlag(3, 7) ? 1 : 0,
		second->getGroupCollisionFlag(31, 31) ? 1 : 0,
		second->getGroupCollisionFlag(31, 0) ? 1 : 0);

	second->setGroupCollisionFlag(3, 7, false);
	printf("step=groups_disabled forward=%d reverse=%d neighbour=%d unrelated=%d\n",
		second->getGroupCollisionFlag(3, 7) ? 1 : 0,
		second->getGroupCollisionFlag(7, 3) ? 1 : 0,
		second->getGroupCollisionFlag(3, 8) ? 1 : 0,
		second->getGroupCollisionFlag(4, 7) ? 1 : 0);

	second->setGroupCollisionFlag(3, 7, true);
	printf("step=groups_reenabled forward=%d reverse=%d\n",
		second->getGroupCollisionFlag(3, 7) ? 1 : 0,
		second->getGroupCollisionFlag(7, 3) ? 1 : 0);

	// 31 is the last legal group and 32 the first illegal one. The oracle also
	// carries an unreachable 0xffff sentinel test behind the range check, so
	// 0xffff must report the range error and not be waved through.
	second->getGroupCollisionFlag(32, 0);
	reportStream("groups_get_out_of_range", stream);
	printf("step=groups_get_out_of_range value=%d\n", second->getGroupCollisionFlag(32, 0) ? 1 : 0);
	printf("step=groups_get_sentinel value=%d\n", second->getGroupCollisionFlag(0xffff, 0) ? 1 : 0);
	reportStream("groups_get_sentinel", stream);

	second->setGroupCollisionFlag(32, 0, false);
	reportStream("groups_set_out_of_range", stream);
	printf("step=groups_unchanged_after_error pair=%d\n", second->getGroupCollisionFlag(0, 0) ? 1 : 0);

	// The default material is index 0 and the array holds exactly it.
	printf("step=materials_initial count=%u\n", second->getNbMaterials());
	NxMaterial* defaultMaterial = second->getMaterial(0);
	printf("step=material_default nonnull=%d dynamic=%.6f static=%.6f restitution=%.6f flags=0x%08x\n",
		defaultMaterial ? 1 : 0,
		defaultMaterial ? defaultMaterial->dynamicFriction : 0.0f,
		defaultMaterial ? defaultMaterial->staticFriction : 0.0f,
		defaultMaterial ? defaultMaterial->restitution : 0.0f,
		defaultMaterial ? static_cast<unsigned>(defaultMaterial->flags) : 0u);
	printf("step=material_out_of_range is_default=%d\n",
		second->getMaterial(5) == defaultMaterial ? 1 : 0);

	// Nine pushes past a capacity of one force the (1 + size()) * 2 growth twice
	// over, so the readback also gates that the copy preserved every element.
	NxMaterial added;
	added.setToDefault();
	unsigned indices[9];
	for(unsigned i = 0; i < 9; ++i)
		{
		added.dynamicFriction = 0.125f * static_cast<NxReal>(i + 1);
		added.staticFriction = 0.25f * static_cast<NxReal>(i + 1);
		added.restitution = 0.0625f * static_cast<NxReal>(i + 1);
		indices[i] = second->addMaterial(added);
		}
	printf("step=materials_added count=%u indices=%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
		second->getNbMaterials(), indices[0], indices[1], indices[2], indices[3],
		indices[4], indices[5], indices[6], indices[7], indices[8]);
	for(unsigned i = 0; i < 9; ++i)
		{
		NxMaterial* read = second->getMaterial(static_cast<NxMaterialIndex>(indices[i]));
		printf("  material=%u dynamic=%.6f static=%.6f restitution=%.6f is_default=%d\n",
			indices[i], read ? read->dynamicFriction : 0.0f, read ? read->staticFriction : 0.0f,
			read ? read->restitution : 0.0f, read == defaultMaterial ? 1 : 0);
		}
	printf("step=material_default_still_reads dynamic=%.6f\n", second->getMaterial(0)->dynamicFriction);
	reportAllocator("materials.allocator", allocator);

	// Bit 31 of flags marks a slot the oracle refuses to hand back: getMaterial
	// falls through to index 0 rather than returning the slot or an error.
	added.dynamicFriction = 9.0f;
	added.flags |= 0x80000000;
	NxMaterialIndex retired = second->addMaterial(added);
	NxMaterial* retiredRead = second->getMaterial(retired);
	printf("step=material_bit31 index=%u count=%u is_default=%d dynamic=%.6f\n",
		retired, second->getNbMaterials(), retiredRead == second->getMaterial(0) ? 1 : 0,
		retiredRead ? retiredRead->dynamicFriction : 0.0f);
	reportStream("material_bit31", stream);


	// ---- Factory-A part 2: setMaterialAtIndex and purgeMaterials ----
	//
	// The array is 11 long here with index 10 already retired. Every case below
	// is read back through getNbMaterials and getMaterial, so what is gated is
	// the array state and not the call.

	NxMaterial replacement;
	replacement.setToDefault();
	replacement.dynamicFriction = 42.0f;
	second->setMaterialAtIndex(2, &replacement);
	NxMaterial* slot2 = second->getMaterial(2);
	printf("step=set_in_range count=%u dynamic=%.6f is_default=%d\n",
		second->getNbMaterials(), slot2 ? slot2->dynamicFriction : 0.0f,
		slot2 == second->getMaterial(0) ? 1 : 0);

	// A null material retires the slot rather than clearing it, and the count
	// does not move.
	second->setMaterialAtIndex(2, 0);
	printf("step=set_null_retires count=%u is_default=%d\n",
		second->getNbMaterials(), second->getMaterial(2) == second->getMaterial(0) ? 1 : 0);

	// Index 0 is the one slot a null cannot retire.
	second->setMaterialAtIndex(0, 0);
	printf("step=set_null_index0 count=%u flags=0x%08x dynamic=%.6f\n",
		second->getNbMaterials(), static_cast<unsigned>(second->getMaterial(0)->flags),
		second->getMaterial(0)->dynamicFriction);

	// Past the end: grow to the index with the default material template, then
	// append. The template carries bit 31, so every gap slot reads back as the
	// default material and not as itself.
	//
	// The index is 16 and not 12 or 14 for a reason that is the oracle's, not
	// this harness's. NxArray::insert reserves size() + n but its
	// copy(where, where + n, where + n) writes n elements starting AT where + n,
	// so inserting at end() needs size() + 2n. Measured against the shipped DLL
	// from this exact state -- 11 materials, capacity 14 -- index 12 (n = 1)
	// completes, index 13 (n = 2) and index 14 (n = 3) both fault, and index 16
	// (n = 5) completes because it is the first that forces a reserve. The
	// faulting cases corrupt the CRT heap, so no transcript taken from them
	// would mean anything; 16 exercises the same grow, gap fill and append.
	NxMaterial beyond;
	beyond.setToDefault();
	beyond.dynamicFriction = 3.5f;
	beyond.restitution = 0.75f;
	unsigned before = second->getNbMaterials();
	second->setMaterialAtIndex(16, &beyond);
	NxMaterial* placed = second->getMaterial(16);
	printf("step=set_past_end before=%u count=%u placed_dynamic=%.6f placed_restitution=%.6f\n",
		before, second->getNbMaterials(), placed ? placed->dynamicFriction : 0.0f,
		placed ? placed->restitution : 0.0f);
	printf("step=set_past_end_gap gap12_is_default=%d gap15_is_default=%d last_live=%d\n",
		second->getMaterial(12) == second->getMaterial(0) ? 1 : 0,
		second->getMaterial(15) == second->getMaterial(0) ? 1 : 0,
		second->getMaterial(16) == second->getMaterial(0) ? 0 : 1);

	// Past the end with a null material does nothing at all.
	second->setMaterialAtIndex(40, 0);
	printf("step=set_past_end_null count=%u\n", second->getNbMaterials());

	// Exactly at the end is a plain append: the grow is a no-op and the push is
	// the whole effect.
	NxMaterial appended;
	appended.setToDefault();
	appended.staticFriction = 6.25f;
	before = second->getNbMaterials();
	second->setMaterialAtIndex(static_cast<NxMaterialIndex>(before), &appended);
	printf("step=set_at_end before=%u count=%u static=%.6f\n",
		before, second->getNbMaterials(), second->getMaterial(static_cast<NxMaterialIndex>(before))->staticFriction);
	reportAllocator("set_material.allocator", allocator);

	// purgeMaterials keeps index 0 exactly as it is and drops everything else.
	NxMaterial kept;
	kept.setToDefault();
	kept.dynamicFriction = 7.5f;
	kept.restitution = 0.375f;
	second->setMaterialAtIndex(0, &kept);
	second->purgeMaterials();
	NxMaterial* survivor = second->getMaterial(0);
	printf("step=purge count=%u dynamic=%.6f restitution=%.6f flags=0x%08x\n",
		second->getNbMaterials(), survivor ? survivor->dynamicFriction : 0.0f,
		survivor ? survivor->restitution : 0.0f,
		survivor ? static_cast<unsigned>(survivor->flags) : 0u);
	reportAllocator("purge.allocator", allocator);

	// A second purge is a no-op, and the array can be rebuilt afterwards.
	second->purgeMaterials();
	printf("step=purge_again count=%u dynamic=%.6f\n",
		second->getNbMaterials(), second->getMaterial(0)->dynamicFriction);
	printf("step=purge_then_add index=%u count=%u\n",
		second->addMaterial(replacement), second->getNbMaterials());
	reportAllocator("purge_then_add.allocator", allocator);

	// The two fluid group-pair slots are the shipped DLL refusing part of the
	// fluid API it exports. They report a debug warning, not an error.
#if NX_USE_FLUID_API
	second->setFluidGroupPairFlags(1, 2, 3);
	reportStream("fluid_pair_set", stream);
	printf("step=fluid_pair_get value=%u\n", second->getFluidGroupPairFlags(1, 2));
	reportStream("fluid_pair_get", stream);
#endif

	second->release();
	printf("step=recreate_release\n");
	reportAllocator("recreate_release.allocator", allocator);
	printf("stream errors=%u asserts=%u prints=%u\n", stream.errors, stream.asserts, stream.prints);

	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
