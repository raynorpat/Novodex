/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// A STATIC-PROOF gate, not a differential.
//
// The eight rows checked here -- ReadWriteLock's three entry points, the four
// SdkContainer rows and the SDK allocator accessor -- cannot be reached by any
// input sequence through the public API, so no oracle-versus-candidate
// transcript can exist for them. This links the candidate's own translation
// units and checks them against expectations derived from the disassembly, each
// one carrying the RVA it came from. It is strictly weaker than the transcript
// differentials: it proves the reconstruction does what the disassembly was read
// to say, not that the oracle does the same thing.
//
// The precedent is NxFoundationCustomArrayTests, which the Foundation programme
// used for the same reason.

#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <new>

#include "PhysicsInternal.h"
#include "Containers.h"
#include "Scene.h"
#include "core/JointSupport.h"
#include "TriangleMesh.h"
#include "NxFoundationSDK.h"
#include "NxUserAllocator.h"

static int gChecks = 0;

// Handed to NxCreateFoundationSDK so nxFoundationSDKAllocator is this object.
// The pointer binding table is created and destroyed through it, and its
// destruction has no other observable consequence, so counting is the only way
// to gate it.
class FoundationCounter : public NxUserAllocator
	{
	public:
	FoundationCounter(): mallocs(0), frees(0), lastSize(0) {}

	void* mallocDEBUG(size_t size, const char*, int)								{ ++mallocs; lastSize = size; return ::malloc(size); }
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)	{ ++mallocs; lastSize = size; return ::malloc(size); }
	void* malloc(size_t size)													{ ++mallocs; lastSize = size; return ::malloc(size); }
	void* malloc(size_t size, NxMemoryType)										{ ++mallocs; lastSize = size; return ::malloc(size); }
	void* realloc(void* memory, size_t size)									{ return ::realloc(memory, size); }
	void free(void* memory)														{ ++frees; ::free(memory); }

	unsigned mallocs;
	unsigned frees;
	size_t lastSize;
	};

static FoundationCounter gFoundationCounter;

static unsigned gTriangleMeshDeletingObjectDestructions = 0;

class TriangleMeshDeletingTestObject
	{
	public:
	virtual ~TriangleMeshDeletingTestObject() { ++gTriangleMeshDeletingObjectDestructions; }
	static void operator delete(void* memory)
		{ nxGetSdkAllocator()->free(memory); }
	};

static int fail(const char* message)
	{
	fprintf(stderr, "FAIL %s\n", message);
	return 1;
	}

static bool check(bool condition, const char* what)
	{
	++gChecks;
	if(!condition)
		printf("check_failed %s\n", what);
	return condition;
	}

// Counts what the container asks of the SDK allocator, so the free that
// phys_fn_004846 skips for a non-owned buffer is observable, and refuses
// anything above the cap so the allocation-failure path is reachable.
class CountingAllocator : public SdkAllocator
	{
	public:
	CountingAllocator(): mallocs(0), frees(0), lastSize(0), cap(0x7fffffff) {}

	void* malloc(size_t size, NxMemoryType)
		{
		lastSize = size;
		if(size > cap)
			return 0;
		++mallocs;
		return ::malloc(size);
		}
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType type)
		{
		return malloc(size, type);
		}
	void* realloc(void* memory, size_t size)	{ return ::realloc(memory, size); }
	void free(void* memory)						{ ++frees; ::free(memory); }

	unsigned mallocs;
	unsigned frees;
	size_t lastSize;
	size_t cap;
	};

// ---------------------------------------------------------------- accessor

static int testAllocatorAccessor()
	{
	// phys_fn_004803 at 0x000b4000: nothing has been registered, so the first
	// call must install the built-in default rather than hand back null, and
	// 0x000b400e is a store, so the second call must read back the same object.
	SdkAllocator* first = nxGetSdkAllocator();
	SdkAllocator* second = nxGetSdkAllocator();
	if(!check(first != 0, "accessor installs a default when nothing is registered"))
		return fail("accessor returned null");
	// The store at 0x000b400e is measured, but it has no observable consequence:
	// an implementation that returned the default without recording it would be
	// indistinguishable, because every later call returns the same object either
	// way. This checks the return value agrees across calls and claims no more.
	if(!check(first == second, "repeated calls return the same allocator"))
		return fail("accessor disagreed with itself");

	// The default really is an allocator, not a placeholder.
	void* block = first->malloc(16, NX_MEMORY_PERSISTENT);
	if(!check(block != 0, "the built-in default allocates"))
		return fail("built-in default returned null");
	first->free(block);

	// phys_fn_004805 stores whatever it is handed, and the accessor hands that
	// back unchanged.
	CountingAllocator registered;
	nxSetSdkAllocatorBridge(&registered);
	if(!check(nxGetSdkAllocator() == &registered, "a registered allocator is returned unchanged"))
		return fail("registered allocator not returned");

	// Registering null puts the accessor back in the state where it installs
	// the default: the test is on the pointer being null, not on a flag.
	nxSetSdkAllocatorBridge(0);
	if(!check(nxGetSdkAllocator() == first, "clearing the registration reinstalls the default"))
		return fail("default not reinstalled");
	return 0;
	}

// -------------------------------------------------------------------- lock

struct ContenderArgs
	{
	ReadWriteLock* lock;
	bool tryLockResult;
	};

static DWORD WINAPI contend(LPVOID parameter)
	{
	ContenderArgs* args = static_cast<ContenderArgs*>(parameter);
	args->tryLockResult = args->lock->tryLock();
	if(args->tryLockResult)
		args->lock->unlock();
	return 0;
	}

// A contender can legitimately block: tryLock only refuses when the flag is
// already claimed by another thread, and once it gets past the flag it enters
// the critical section, which can still be held. So the wait is bounded and a
// timeout is a distinct third answer rather than a hang.
enum ContenderOutcome { CONTENDER_REFUSED, CONTENDER_TOOK, CONTENDER_BLOCKED };

static const char* outcomeName(ContenderOutcome outcome)
	{
	return outcome == CONTENDER_TOOK ? "took"
		: outcome == CONTENDER_REFUSED ? "refused" : "blocked";
	}

static ContenderOutcome tryLockOnAnotherThread(ReadWriteLock& lock)
	{
	// The contender writes its answer into these after the wait may have given
	// up on it, so they outlive this frame, and a thread still running is not
	// closed out from under itself. On a timeout both are deliberately leaked:
	// the process is about to fail anyway, and reclaiming them would mean
	// waiting for a thread that is by definition not going to finish.
	static ContenderArgs args;
	args.lock = &lock;
	args.tryLockResult = false;

	HANDLE thread = CreateThread(0, 0, contend, &args, 0, 0);
	if(!thread)
		return CONTENDER_BLOCKED;
	if(WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0)
		return CONTENDER_BLOCKED;

	CloseHandle(thread);
	return args.tryLockResult ? CONTENDER_TOOK : CONTENDER_REFUSED;
	}

// Named rather than folded into a bool, so "blocked" reads as itself in the
// transcript instead of as "refused".
static bool checkOutcome(ContenderOutcome actual, ContenderOutcome expected, const char* what)
	{
	++gChecks;
	if(actual == expected)
		return true;
	printf("check_failed %s: contender %s, expected %s\n",
		what, outcomeName(actual), outcomeName(expected));
	return false;
	}

static int testLock()
	{
	ReadWriteLock lock;

	// 0x0005b745 claims the flag, so tryLock on an idle lock succeeds.
	if(!check(lock.tryLock(), "tryLock succeeds on an idle lock"))
		return fail("tryLock on idle lock");

	// 0x0005b755 compares the owner recorded at +0x1c against this thread, so
	// the owning thread may take it again; 0x0005b75c is the only false return.
	if(!check(lock.tryLock(), "tryLock is reentrant on the owning thread"))
		return fail("tryLock reentrancy");
	if(!checkOutcome(tryLockOnAnotherThread(lock), CONTENDER_REFUSED,
			"tryLock is refused for another thread while the flag is claimed"))
		return fail("tryLock cross-thread");

	// Two unlocks for two acquires. The flag is cleared by the first, but the
	// critical section is recursive and is only released by the second, so a
	// contender between them would clear the flag test and then block on the
	// section -- measured, and deliberately not probed here, because probing it
	// would hang rather than report.
	if(!check(lock.unlock(), "unlock returns the literal true at 0x0005b7ac"))
		return fail("unlock return");
	if(!check(lock.unlock(), "the second unlock returns true"))
		return fail("second unlock return");
	if(!checkOutcome(tryLockOnAnotherThread(lock), CONTENDER_TOOK,
			"another thread can take it once it is fully released"))
		return fail("release did not publish");

	// 0x0005b727 returns true unconditionally and 0x0005b790 checks nothing.
	if(!check(lock.lock(), "lock returns the literal true at 0x0005b727"))
		return fail("lock return");
	if(!check(lock.lock(), "lock is reentrant on the owning thread"))
		return fail("lock reentrancy");
	if(!checkOutcome(tryLockOnAnotherThread(lock), CONTENDER_REFUSED,
			"lock claims the flag against other threads too"))
		return fail("lock cross-thread");
	if(!check(lock.unlock() && lock.unlock(), "both unlocks report success"))
		return fail("lock unlock pair");
	if(!checkOutcome(tryLockOnAnotherThread(lock), CONTENDER_TOOK, "lock releases fully"))
		return fail("lock release");
	return 0;
	}

// --------------------------------------------------------------- container

static int testContainer()
	{
	CountingAllocator allocator;
	nxSetSdkAllocatorBridge(&allocator);

	SdkContainer container;
	if(!check(container.mCapacity == 0 && container.mCount == 0 && container.mEntries == 0
			&& container.mGrowthFactor == 2.0f, "phys_fn_004836 leaves {0, 0, 0, 2.0f}"))
		return fail("constructor");

	// 0x000b4e1b: an empty container grows to a literal 2, not to needed.
	if(!check(container.resize(1), "the first resize succeeds"))
		return fail("first resize");
	if(!check(container.mCapacity == 2 && container.mEntries != 0 && allocator.mallocs == 1
			&& allocator.lastSize == 2 * sizeof(NxU32), "an empty container grows to 2 entries"))
		return fail("empty growth");

	// capacity * 2.0f = 4, which already exceeds count + needed.
	container.mEntries[0] = 0x11223344;
	container.mEntries[1] = 0x55667788;
	container.mCount = 2;
	if(!check(container.resize(1), "the second resize succeeds"))
		return fail("second resize");
	if(!check(container.mCapacity == 4 && allocator.lastSize == 4 * sizeof(NxU32)
			&& allocator.mallocs == 2 && allocator.frees == 1, "capacity * factor wins over count + needed"))
		return fail("factor growth");
	if(!check(container.mEntries[0] == 0x11223344 && container.mEntries[1] == 0x55667788,
			"the live entries survive the move"))
		return fail("copy on grow");

	// count + needed wins when the factor does not reach it: 4 * 2.0f = 8 < 2 + 20.
	if(!check(container.resize(20), "the clamped resize succeeds"))
		return fail("clamped resize");
	if(!check(container.mCapacity == 22 && allocator.lastSize == 22 * sizeof(NxU32),
			"count + needed wins when the factor does not reach it"))
		return fail("clamp");
	if(!check(container.mEntries[0] == 0x11223344, "the entries survive the clamped move"))
		return fail("copy on clamped grow");

	// 0x000b4df4: a factor that is not greater than zero is refused before
	// anything is touched.
	unsigned mallocsBefore = allocator.mallocs;
	NxU32* entriesBefore = container.mEntries;
	NxU32 capacityBefore = container.mCapacity;
	container.mGrowthFactor = 0.0f;
	if(!check(!container.resize(1), "a zero growth factor is refused"))
		return fail("zero factor accepted");
	if(!check(allocator.mallocs == mallocsBefore && container.mEntries == entriesBefore
			&& container.mCapacity == capacityBefore, "the refusal changes nothing"))
		return fail("zero factor side effect");
	container.mGrowthFactor = 2.0f;

	// 0x000b4e2b writes the capacity before the allocation, so a failure leaves
	// the capacity grown and the entries untouched. That asymmetry is measured,
	// not tidied.
	allocator.cap = 4;
	entriesBefore = container.mEntries;
	if(!check(!container.resize(1), "a failed allocation is reported"))
		return fail("allocation failure not reported");
	if(!check(container.mCapacity == 44 && container.mEntries == entriesBefore,
			"a failed allocation still leaves the capacity grown"))
		return fail("failure ordering");
	allocator.cap = 0x7fffffff;

	// 0x000b4f81 and 0x000b4f87 clear the capacity and count outside the branch.
	unsigned freesBefore = allocator.frees;
	container.empty();
	if(!check(allocator.frees == freesBefore + 1 && container.mEntries == 0
			&& container.mCapacity == 0 && container.mCount == 0, "empty releases an owned buffer"))
		return fail("empty owned");

	// An external buffer is adopted with a factor of -1.0f, and the marker is
	// what stops the release.
	// Heap allocated, not a stack array: an implementation that wrongly released
	// it must fail a check rather than corrupt the heap and die before printing.
	NxU32* external = static_cast<NxU32*>(::malloc(3 * sizeof(NxU32)));
	external[0] = 7;
	external[1] = 8;
	external[2] = 9;
	container.resize(2);
	freesBefore = allocator.frees;
	container.setExternalBuffer(3, external);
	if(!check(allocator.frees == freesBefore + 1, "adopting a buffer releases the owned one"))
		return fail("adopt release");
	if(!check(container.mCapacity == 3 && container.mCount == 0 && container.mEntries == external
			&& container.mGrowthFactor == -1.0f, "phys_fn_004847 installs the buffer and the marker"))
		return fail("adopt state");

	freesBefore = allocator.frees;
	container.empty();
	if(!check(allocator.frees == freesBefore, "empty does not release a buffer it does not own"))
		return fail("empty external");
	if(!check(container.mEntries == external && container.mCapacity == 0 && container.mCount == 0,
			"empty still clears the capacity and count of a non-owned buffer"))
		return fail("empty external state");

	// The marker also stops the next resize, because -1.0f is not greater than
	// zero: an adopted buffer can never grow.
	if(!check(!container.resize(1), "an adopted buffer cannot grow"))
		return fail("adopted growth");

	::free(external);
	nxSetSdkAllocatorBridge(0);
	return 0;
	}

// -------------------------------------------------------- pointer bindings

static int testPointerBindings()
	{
	int a = 0, b = 0, c = 0;

	// phys_fn_000454 with no table: a miss and an unset key are the same answer.
	if(!check(nxGetSdkPointerBinding(&a) == 0, "a lookup with no table misses"))
		return fail("lookup with no table");

	// phys_fn_000480: a null key is the only rejection, and it is refused before
	// the table is even consulted.
	if(!check(!nxSetSdkPointerBinding(0, &b), "a null key is refused"))
		return fail("null key accepted");

	// Removing from a table that does not exist succeeds without creating one.
	if(!check(nxSetSdkPointerBinding(&a, 0), "removing with no table succeeds"))
		return fail("remove with no table");
	if(!check(nxGetSdkPointerBinding(&a) == 0, "and creates nothing"))
		return fail("remove created a table");

	if(!check(nxSetSdkPointerBinding(&a, &b) && nxSetSdkPointerBinding(&b, &c),
			"two bindings are stored"))
		return fail("store");
	if(!check(nxGetSdkPointerBinding(&a) == &b && nxGetSdkPointerBinding(&b) == &c,
			"both read back"))
		return fail("readback");
	if(!check(nxGetSdkPointerBinding(&c) == 0, "an unbound key still misses"))
		return fail("unbound key");

	// A second store against the same key updates in place rather than appending.
	if(!check(nxSetSdkPointerBinding(&a, &c) && nxGetSdkPointerBinding(&a) == &c,
			"rebinding a key updates it"))
		return fail("rebind");

	// 0x0000ee1e replaces the removed slot with the last one, so the survivor
	// must still be found afterwards whichever order they were stored in.
	unsigned freesBefore = gFoundationCounter.frees;
	if(!check(nxSetSdkPointerBinding(&a, 0), "removing a binding succeeds"))
		return fail("remove");
	if(!check(gFoundationCounter.frees == freesBefore,
			"removing one of two bindings does not release the table"))
		return fail("early table release");
	if(!check(nxGetSdkPointerBinding(&a) == 0 && nxGetSdkPointerBinding(&b) == &c,
			"the removed one is gone and the survivor is intact"))
		return fail("replaceWithLast");

	// Removing the last binding destroys the table; the next lookup must take
	// the no-table path rather than read a freed one.
	freesBefore = gFoundationCounter.frees;
	if(!check(nxSetSdkPointerBinding(&b, 0), "removing the last binding succeeds"))
		return fail("remove last");
	// phys_fn_000474 frees the entries and then the header, so two frees, and
	// 0x0000ef26 clears the pointer. An implementation that left an empty table
	// standing would free nothing here.
	if(!check(gFoundationCounter.frees == freesBefore + 2,
			"emptying the table releases its entries and the table itself"))
		return fail("table not released");
	if(!check(nxGetSdkPointerBinding(&b) == 0, "the emptied table is gone"))
		return fail("empty table");

	// The table is rebuilt on demand after being destroyed, which also proves
	// the pointer was cleared rather than left dangling.
	unsigned mallocsBefore = gFoundationCounter.mallocs;
	if(!check(nxSetSdkPointerBinding(&a, &b) && nxGetSdkPointerBinding(&a) == &b,
			"the table is rebuilt after being emptied"))
		return fail("rebuild");
	if(!check(gFoundationCounter.mallocs > mallocsBefore, "rebuilding allocates a new table"))
		return fail("rebuild allocated nothing");
	nxSetSdkPointerBinding(&a, 0);
	return 0;
	}

// Static proof for InternalTriangleMesh rows 002065/002067/002069/002071/
// 002073/002075. These rows are private and not independently reachable from
// the public API until the TriangleMesh factory is closed; check their measured
// offsets, allocation sizes, no-op zero-count branches, and teardown ownership.
static int testInternalTriangleMeshRows()
	{
	union AlignedMeshStorage
		{
		NxU32 words[0x38 / sizeof(NxU32)];
		InternalTriangleMesh mesh;
		AlignedMeshStorage() {}
		~AlignedMeshStorage() {}
		} storage;
	InternalTriangleMesh* mesh = ::new (&storage.mesh) InternalTriangleMesh;
	memset(mesh, 0xa5, sizeof(*mesh));
	nxInternalMeshInit(mesh);
	const NxU8* raw = reinterpret_cast<const NxU8*>(mesh);
	for(unsigned offset = 0; offset <= 0x20; offset += 4)
		if(!check(*reinterpret_cast<const NxU32*>(raw + offset) == 0,
			"002065 clears the nine measured leading words"))
			return fail("internal mesh leading initialization");
	for(unsigned offset = 0x24; offset <= 0x30; offset += 4)
		if(!check(*reinterpret_cast<const NxU32*>(raw + offset) == 0,
			"002065 clears the MeshInterface constructor's four measured words"))
			return fail("internal mesh interface initialization");
	if(!check(*reinterpret_cast<const NxU32*>(raw + 0x34) == 0xa5a5a5a5,
		"002065 leaves the unmeasured final interface word untouched"))
		return fail("internal mesh interface boundary");

	nxInternalMeshAllocateMaterials(mesh);
	nxInternalMeshAllocateFaceRemap(mesh);
	if(!check(mesh->mMaterialIndices == 0 && mesh->mFaceRemap == 0,
		"002073 and 002075 do not allocate for zero triangles"))
		return fail("internal mesh zero-count allocation");

	nxInternalMeshAllocateVertices(mesh, 3);
	if(!check(mesh->mVertexCount == 3 && mesh->mVertices != 0 &&
		gFoundationCounter.lastSize == 36,
		"002069 stores count and allocates twelve bytes per vertex"))
		return fail("internal mesh vertex allocation");
	nxInternalMeshAllocateTriangles(mesh, 2);
	if(!check(mesh->mTriangleCount == 2 && mesh->mTriangles != 0 &&
		gFoundationCounter.lastSize == 24,
		"002071 stores count and allocates twelve bytes per triangle"))
		return fail("internal mesh triangle allocation");
	const NxVec3 vertices[3] = {
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f)
		};
	const NxU32 triangles[6] = { 0, 1, 2, 0, 2, 1 };
	memcpy(mesh->mVertices, vertices, sizeof(vertices));
	memcpy(mesh->mTriangles, triangles, sizeof(triangles));
	nxInternalMeshBuildTriangleData(mesh);
	if(!check(mesh->mTriangleData != 0 && gFoundationCounter.lastSize == 32,
		"002079 allocates sixteen bytes per triangle"))
		return fail("internal mesh triangle-data allocation");
	const NxReal* planes = static_cast<const NxReal*>(mesh->mTriangleData);
	if(!check(planes[0] == 0.0f && planes[1] == 0.0f && planes[2] == 1.0f && planes[3] == 0.0f &&
		planes[4] == 0.0f && planes[5] == 0.0f && planes[6] == -1.0f && planes[7] == 0.0f,
		"002079 writes normalized oriented planes in triangle order"))
		return fail("internal mesh triangle-data planes");
	nxInternalMeshAllocateMaterials(&storage.mesh);
	if(!check(storage.mesh.mMaterialIndices != 0 && gFoundationCounter.lastSize == 4,
		"002073 allocates two bytes per triangle"))
		return fail("internal mesh material allocation");
	nxInternalMeshAllocateFaceRemap(&storage.mesh);
	if(!check(storage.mesh.mFaceRemap != 0 && gFoundationCounter.lastSize == 8,
		"002075 allocates four bytes per triangle"))
		return fail("internal mesh face-remap allocation");
	mesh->mVertexNormals = gFoundationCounter.malloc(36, NX_MEMORY_PERSISTENT);
	if(!check(mesh->mVertexNormals != 0,
		"test installs the measured optional vertex-normal array"))
		return fail("internal mesh optional array allocation");

	const unsigned freesBefore = gFoundationCounter.frees;
	nxInternalMeshRelease(mesh);
	if(!check(gFoundationCounter.frees == freesBefore + 6,
		"002067 releases all six Foundation-owned arrays in its empty-model case"))
		return fail("internal mesh release count");
	if(!check(mesh->mVertices == 0 && mesh->mTriangles == 0 &&
		mesh->mMaterialIndices == 0 && mesh->mFaceRemap == 0 &&
		mesh->mVertexNormals == 0 && mesh->mTriangleData == 0 &&
		mesh->mModel == 0,
		"002067 clears each released pointer"))
		return fail("internal mesh release pointers");
	if(!check(mesh->mVertexCount == 3 && mesh->mTriangleCount == 2,
		"002067 leaves the measured counts unchanged"))
		return fail("internal mesh release counts");
	mesh->~InternalTriangleMesh();
	return 0;
	}

// Static proof for the one-triangle model path in 002083. A single triangle
// makes Model::Build take its measured OPC_SINGLE_NODE branch without creating
// the larger AABB tree, while still proving the embedded MeshInterface and
// Model ownership path.
static int testInternalTriangleMeshModel()
	{
	InternalTriangleMesh mesh;
	nxInternalMeshInit(&mesh);
	nxInternalMeshAllocateVertices(&mesh, 3);
	nxInternalMeshAllocateTriangles(&mesh, 1);
	const NxVec3 vertices[3] = {
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f)
		};
	const NxU32 triangles[3] = { 0, 1, 2 };
	memcpy(mesh.mVertices, vertices, sizeof(vertices));
	memcpy(mesh.mTriangles, triangles, sizeof(triangles));
	const bool built = nxInternalMeshBuildModel(&mesh, 0xffffffffu, 0.0f, 0);
	if(!check(built && mesh.mModel != 0,
		"002083 installs an OPCODE model for a valid single-triangle mesh"))
		return fail("internal mesh model build");
	nxInternalMeshRelease(&mesh);
	return 0;
	}

// phys_fn_000604 walks Scene+[0x56c, 0x570) and applies 000760 to every
// surviving body record before the joint lists are destroyed. Exercise all
// three records, including 000760's bit-8 wake-counter suppression branch.
static int testSceneBodyRecordTeardownReset()
	{
	NxSceneInternal scene;
	alignas(16) unsigned char recordsMemory[3][0x260] = {};
	void* records[3] = { recordsMemory[0], recordsMemory[1], recordsMemory[2] };
	const NxU32 wakeWords[3] = { 0x3dcccccd, 0x3e4ccccd, 0x3e800000 };
	for(unsigned index = 0; index != 3; ++index)
		{
		unsigned char* record = recordsMemory[index];
		*reinterpret_cast<void**>(record + 0x1bc) = record;
		*reinterpret_cast<NxU32*>(record + 0x4c) = wakeWords[index];
		}
	*reinterpret_cast<NxU32*>(recordsMemory[2] + 0x114) = 0x100;
	scene.at<void**>(0x56c) = records;
	scene.at<void**>(0x570) = records + 3;

	nxSceneResetBodyRecords(&scene);
	const NxU32 actualWake[3] = {
		*reinterpret_cast<NxU32*>(recordsMemory[0] + 0x4c),
		*reinterpret_cast<NxU32*>(recordsMemory[1] + 0x4c),
		*reinterpret_cast<NxU32*>(recordsMemory[2] + 0x4c)
		};
	printf("scene body_record_cleanup records=3 wake=%08x/%08x/%08x\n",
		actualWake[0], actualWake[1], actualWake[2]);
	if(!check(actualWake[0] == 0x3ecccccc && actualWake[1] == 0x3ecccccc,
		"000604 applies 000760 to each unsuppressed remaining body record"))
		return fail("Scene body-record teardown did not reset wake counters");
	if(!check(actualWake[2] == wakeWords[2],
		"000604 preserves the 000760 bit-8 wake-counter suppression"))
		return fail("Scene body-record teardown ignored its suppression flag");
	if(!check(*reinterpret_cast<void**>(recordsMemory[0] + 0x1bc) == recordsMemory[0] &&
		*reinterpret_cast<void**>(recordsMemory[1] + 0x1bc) == recordsMemory[1] &&
		*reinterpret_cast<void**>(recordsMemory[2] + 0x1bc) == recordsMemory[2],
		"000604 leaves each record as its own island root"))
		return fail("Scene body-record teardown left an island root linked");

	scene.at<void**>(0x56c) = 0;
	scene.at<void**>(0x570) = 0;
	scene.scalarDeletingDestructor(0);
	return 0;
	}

// The TriangleMesh destructor's cleanup helper at 0x00054a80 owns more than
// the embedded mesh arrays: two Foundation arrays, three polymorphic deleting
// slots, the EdgeList and Adjacencies caches, and the allocation at +0x3c.
// Build every one through the same allocators the oracle uses, then verify the
// destructor releases the full ownership set.
static int testTriangleMeshDestructorOwnership()
	{
	CountingAllocator sdkAllocator;
	nxSetSdkAllocatorBridge(&sdkAllocator);
	const unsigned foundationMallocsBefore = gFoundationCounter.mallocs;
	const unsigned foundationFreesBefore = gFoundationCounter.frees;
	const unsigned sdkMallocsBefore = sdkAllocator.mallocs;
	const unsigned sdkFreesBefore = sdkAllocator.frees;
	const unsigned deletingDestructionsBefore = gTriangleMeshDeletingObjectDestructions;

	void* meshMemory = gFoundationCounter.malloc(sizeof(TriangleMesh), NX_MEMORY_PERSISTENT);
	TriangleMesh* mesh = meshMemory ? new(meshMemory) TriangleMesh : 0;
	if(!check(mesh != 0, "destructor fixture allocates a TriangleMesh"))
		return fail("TriangleMesh destructor fixture allocation");

	nxInternalMeshAllocateVertices(&mesh->mInternal, 4);
	nxInternalMeshAllocateTriangles(&mesh->mInternal, 4);
	const NxVec3 vertices[4] = {
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f),
		NxVec3(0.0f, 1.0f, 0.0f), NxVec3(0.0f, 0.0f, 1.0f)
		};
	const NxU32 triangles[12] = { 0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3 };
	memcpy(mesh->mInternal.mVertices, vertices, sizeof(vertices));
	memcpy(mesh->mInternal.mTriangles, triangles, sizeof(triangles));
	nxInternalMeshBuildTriangleData(&mesh->mInternal);
	mesh->createAdjacencies();
	mesh->createEdgeList();
	if(!check(mesh->mInternal.mTriangleData && mesh->mAdjacencies && mesh->mEdgeList,
		"destructor fixture builds triangle data and both topology caches"))
		return fail("TriangleMesh destructor fixture topology construction");

	mesh->mArrayA = static_cast<NxU32*>(gFoundationCounter.malloc(4 * sizeof(NxU32), NX_MEMORY_PERSISTENT));
	mesh->mArrayB = static_cast<NxU32*>(gFoundationCounter.malloc(4 * sizeof(NxU32), NX_MEMORY_PERSISTENT));
	mesh->mInternal.mInterfaceAllocation =
		gFoundationCounter.malloc(16, NX_MEMORY_PERSISTENT);
	for(unsigned offset = 0xa4; offset <= 0xac; offset += 4)
		{
		void* memory = sdkAllocator.malloc(sizeof(TriangleMeshDeletingTestObject), NX_MEMORY_PERSISTENT);
		if(!check(memory != 0, "destructor fixture allocates a deleting-slot object"))
			return fail("TriangleMesh destructor deleting-slot allocation");
		new(memory) TriangleMeshDeletingTestObject;
		*reinterpret_cast<void**>(reinterpret_cast<NxU8*>(mesh) + offset) = memory;
		}
	if(!check(mesh->mArrayA && mesh->mArrayB && mesh->mInternal.mInterfaceAllocation,
		"destructor fixture installs both optional arrays and the +0x3c allocation"))
		return fail("TriangleMesh destructor fixture auxiliary storage");

	mesh->~TriangleMesh();
	gFoundationCounter.free(meshMemory);
	nxSetSdkAllocatorBridge(0);

	const unsigned foundationMallocs = gFoundationCounter.mallocs - foundationMallocsBefore;
	const unsigned foundationFrees = gFoundationCounter.frees - foundationFreesBefore;
	const unsigned sdkMallocs = sdkAllocator.mallocs - sdkMallocsBefore;
	const unsigned sdkFrees = sdkAllocator.frees - sdkFreesBefore;
	const unsigned deletingDestructions = gTriangleMeshDeletingObjectDestructions - deletingDestructionsBefore;
	printf("triangle_mesh destructor foundation=%u.%u sdk=%u.%u deleting=%u\n",
		foundationMallocs, foundationFrees, sdkMallocs, sdkFrees, deletingDestructions);
	if(!check(foundationFrees == foundationMallocs,
		"TriangleMesh destructor releases every Foundation-owned allocation"))
		return fail("TriangleMesh destructor leaked Foundation-owned storage");
	if(!check(sdkFrees == sdkMallocs && deletingDestructions == 3,
		"TriangleMesh destructor releases both caches and all deleting slots"))
		return fail("TriangleMesh destructor leaked SDK-owned storage");
	return 0;
	}

int main()
	{
	// ReadWriteLock allocates its block through nxFoundationSDKAllocator, which
	// only the Foundation SDK installs.
	NxFoundationSDK* foundation = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, 0, &gFoundationCounter);
	if(!foundation)
		return fail("NxCreateFoundationSDK returned null");

	int status = testAllocatorAccessor();
	if(!status)
		status = testLock();
	if(!status)
		status = testContainer();
	if(!status)
		status = testPointerBindings();
	if(!status)
		status = testInternalTriangleMeshRows();
	if(!status)
		status = testInternalTriangleMeshModel();
	if(!status)
		status = testSceneBodyRecordTeardownReset();
	if(!status)
		status = testTriangleMeshDestructorOwnership();

	printf("static_proof checks=%d status=%s\n", gChecks, status ? "fail" : "pass");
	foundation->release();
	return status;
	}
