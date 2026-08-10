#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxUserAllocator.h"
#include "NxUserOutputStream.h"
#include "NxUserDebugRenderer.h"
#include "NxDebugRenderable.h"

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
	RecordingAllocator(): mallocCalls(0), refusedCalls(0), freeCalls(0) {}

	void* mallocDEBUG(size_t size, const char*, int)					{ return allocate(size); }
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)	{ return allocate(size); }
	void* malloc(size_t size)											{ return allocate(size); }
	void* malloc(size_t size, NxMemoryType)								{ return allocate(size); }
	void* realloc(void* memory, size_t size)							{ return ::realloc(memory, size); }

	void free(void* memory)
		{
		++freeCalls;
		::free(memory);
		}

	unsigned mallocCalls;
	unsigned refusedCalls;
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

class SilentOutputStream : public NxUserOutputStream
	{
	public:
	SilentOutputStream(): errors(0), asserts(0), prints(0) {}

	void reportError(NxErrorCode, const char*, const char*, int)	{ ++errors; }
	NxAssertResponse reportAssertViolation(const char*, const char*, int)	{ ++asserts; return NX_AR_CONTINUE; }
	void print(const char*)	{ ++prints; }

	unsigned errors;
	unsigned asserts;
	unsigned prints;
	};

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
	printf("step=%s alloc_calls=%u refused_calls=%u free_calls=%u\n",
		step, allocator.mallocCalls, allocator.refusedCalls, allocator.freeCalls);
	}

int wmain(int argc, wchar_t** argv)
	{
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
	SilentOutputStream stream;
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

	second->release();
	printf("step=recreate_release\n");
	reportAllocator("recreate_release.allocator", allocator);
	printf("stream errors=%u asserts=%u prints=%u\n", stream.errors, stream.asserts, stream.prints);

	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
