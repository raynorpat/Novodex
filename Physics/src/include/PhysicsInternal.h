#ifndef NX_PHYSICS_PHYSICSINTERNAL
#define NX_PHYSICS_PHYSICSINTERNAL
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// SDK-internal types shared by the DLL lifecycle sources. Every size and offset
// asserted here was measured from the shipped Win32 Release NxPhysics.dll; the
// stable IDs in the comments are the rows in
// docs/reconstruction/novodex-physics/inventory.json and the measurements are
// written up in docs/reconstruction/novodex-physics/evidence/phase2-sdk.md.

#include "Nxp.h"
#include "NxAllocateable.h"
#include "NxUserAllocator.h"

// The SDK reports errors through the Foundation's SDK-side error entry point,
// exactly as the oracle does: it imports ?error@FoundationSDK@NxFoundation@@ and
// ?instance@FoundationSDK@NxFoundation@@ from NxFoundation.dll.
#include "FoundationSDK.h"

// The shipped DLL reports the build tree its sources were compiled in, because
// the error calls pass __FILE__. Reproducing the reported file name means
// reproducing that string; the reconstruction's own path would not match.
// Measured at .rdata 0x0010613c and 0x001058c0.
#define NX_PHYSICS_SDK_CPP		"\\Epic\\Novodex\\SDKs\\Physics\\src\\PhysicsSDK.cpp"
#define NX_NP_PHYSICS_SDK_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpPhysicsSDK.cpp"

class Scene;			// Phase 3 owns the internal scene.
class TriangleMesh;		// Phase 4 owns the internal triangle mesh.

/**
Recursive writer/reader lock. phys_fn_002358 (0x0005b6a0) allocates a 32 byte
block through the SDK allocator and holds only that pointer, so the lock object
itself is one word; phys_fn_002360 (0x0005b6d0) deletes the critical section and
frees the block.

The block is a CRITICAL_SECTION followed by the interlocked owner flag written by
phys_fn_002362/002364/002366 at +0x18 and the owning thread id they cache at
+0x1c. The three entry points are reached exclusively through NpScene, which Phase 3
owns, so no Phase 2 differential can observe them: every caller reaches them from
a loop bounded by getNbScenes(), which is zero while createScene is blocked. They
are gated by NxPhysicsInternalTests, a static-proof gate, and their dynamic
behaviour has never been compared against the oracle.
*/
class ReadWriteLock
	{
	public:
	ReadWriteLock();
	~ReadWriteLock();

	// phys_fn_002362 (0x0005b700), phys_fn_002364 (0x0005b730) and
	// phys_fn_002366 (0x0005b790). All three return a literal true except the
	// one tryLock path that fails. Reentrant on the owning thread; tryLock fails
	// only when another thread holds the lock.
	bool lock();
	bool tryLock();
	bool unlock();

	private:
	ReadWriteLock(const ReadWriteLock&);
	ReadWriteLock& operator=(const ReadWriteLock&);

	void* mData;
	};

/**
The 292 byte table the SDK constructor allocates (phys_fn_002338, 0x0005a8e0)
and its destructor deletes (phys_fn_002340, 0x0005aa60). The one entry vtable at
.rdata 0x00106384 holds only the scalar deleting destructor, so the class has a
virtual destructor and no other virtual.

The 72 words after the vtable pointer are code addresses. They form two
consecutive 36 word blocks, and in each block the written entries are exactly
(0,1..5) (1,1..5) (2,2..5) (3,3..5) (4,4..5) (5,5) of a 6x6 index -- the upper
triangle of a symmetric shape pair matrix. Reading that as "six shape types"
is an inference; the offsets and the write pattern are measured. Every slot
value points into Phase 3 collision code, so the slots stay null here and the
table's only Phase 2 role is its allocation and release.
*/
class ShapePairFunctionTable : public NxAllocateable
	{
	public:
	ShapePairFunctionTable();
	virtual ~ShapePairFunctionTable();

	void* mFunction[2][6][6];
	};

/**
The SDK-side allocator interface. Two objects in the image implement it and their
vtables agree slot for slot: the bridge at .data 0x001220e8 with vtable .rdata
0x00106304, and the built-in default at .data 0x00122368 with vtable .rdata
0x0011b580. Four slots, no destructor slot.
*/
class SdkAllocator
	{
	public:
	virtual void* malloc(size_t size, NxMemoryType type) = 0;
	virtual void* mallocDEBUG(size_t size, const char* fileName, int line, const char* className, NxMemoryType type) = 0;
	virtual void* realloc(void* memory, size_t size) = 0;
	virtual void free(void* memory) = 0;
	};

/**
The global allocator adapter at .data 0x001220e8 that NxCreatePhysicsSDK hands
to phys_fn_004805 when the caller supplies an allocator. The object's second word
is the NxUserAllocator the caller passed.

phys_fn_000460 and phys_fn_000466 normalise the memory type to 0 or 1 before
forwarding; the pass-through pair does not.
*/
class SdkAllocatorBridge : public SdkAllocator
	{
	public:
	void* malloc(size_t size, NxMemoryType type);
	void* mallocDEBUG(size_t size, const char* fileName, int line, const char* className, NxMemoryType type);
	void* realloc(void* memory, size_t size);
	void free(void* memory);

	NxUserAllocator* mAllocator;
	};

/**
The built-in fallback at .data 0x00122368, statically initialised in the image
with the vptr 0x0011b580. phys_fn_004803 installs it the first time the SDK
allocator is asked for and nothing has been registered.

Its four slot bodies are NOT Phase 2 rows -- phys_fn_004807 (0x000b4030),
phys_fn_004812 (0x000b4070), phys_fn_004808 (0x000b4040) and phys_fn_004810
(0x000b4060) are Phase 6, 3, 6 and 6 -- and each forwards to the statically
linked CRT (malloc at 0x000f4722, realloc at 0x000f788e, free at 0x000f4734).
They are written here only so that the accessor has the object it installs;
their phases own the bodies.
*/
class SdkDefaultAllocator : public SdkAllocator
	{
	public:
	void* malloc(size_t size, NxMemoryType type);
	void* mallocDEBUG(size_t size, const char* fileName, int line, const char* className, NxMemoryType type);
	void* realloc(void* memory, size_t size);
	void free(void* memory);
	};

// phys_fn_004805 (0x000b4020): stores the adapter in a file scope pointer and
// returns true.
bool nxSetSdkAllocatorBridge(SdkAllocator* allocator);

// phys_fn_004803 (0x000b4000): hands back the registered allocator, installing
// the built-in default the first time if nothing has been registered.
SdkAllocator* nxGetSdkAllocator();

/**
The pointer binding table at .data 0x00123c0c: an NxArraySDK of eight byte
(key, value) pairs, created on first use and destroyed the moment it empties.
phys_fn_000454 reads it, phys_fn_000480 writes it, phys_fn_000474 is the array's
scalar deleting destructor, and the PhysicsSDK destructor releases whatever is
left.

What the binding means is not recoverable from Phase 2: all 25 writers and all
three readers outside this pair are Phase 3, 5, 6 and 7 rows, and none of them is
reachable from any Phase 2 entry point.

File placement is a choice, not a measurement. phys_fn_000454 sits inside
PhysicsSDK.cpp's span while phys_fn_000480 and phys_fn_000474 sit in the gap
above it, which names no unit; they are here so NxPhysicsInternalTests can link
and gate them, and that is the only reason.
*/
struct SdkPointerPair
	{
	void* key;
	void* value;
	};

// phys_fn_000454 (0x0000df90)
void* nxGetSdkPointerBinding(void* key);
// phys_fn_000480 (0x0000edc0), with phys_fn_000474 (0x0000ea30) as the array's
// destructor.
bool nxSetSdkPointerBinding(void* key, void* value);

#endif
