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

#include "PhysicsInternal.h"
#include "Containers.h"
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
	FoundationCounter(): mallocs(0), frees(0) {}

	void* mallocDEBUG(size_t size, const char*, int)								{ ++mallocs; return ::malloc(size); }
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)	{ ++mallocs; return ::malloc(size); }
	void* malloc(size_t size)													{ ++mallocs; return ::malloc(size); }
	void* malloc(size_t size, NxMemoryType)										{ ++mallocs; return ::malloc(size); }
	void* realloc(void* memory, size_t size)									{ return ::realloc(memory, size); }
	void free(void* memory)														{ ++frees; ::free(memory); }

	unsigned mallocs;
	unsigned frees;
	};

static FoundationCounter gFoundationCounter;

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

	printf("static_proof checks=%d status=%s\n", gChecks, status ? "fail" : "pass");
	foundation->release();
	return status;
	}
