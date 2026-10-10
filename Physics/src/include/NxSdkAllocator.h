#ifndef NX_PHYSICS_SDK_ALLOCATOR_H
#define NX_PHYSICS_SDK_ALLOCATOR_H
#include "NxUserAllocator.h"

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
// returns true. The census places this row in Phase 6, not Phase 2 -- it is
// written here because NxCreatePhysicsSDK calls it and phys_fn_004803 reads what
// it stores, and Phase 6 owns the row. Its parameter widened from
// SdkAllocatorBridge* to SdkAllocator* when the built-in default gained the same
// interface; the oracle stores a bare pointer either way.
bool nxSetSdkAllocatorBridge(SdkAllocator* allocator);

// phys_fn_004803 (0x000b4000): hands back the registered allocator, installing
// the built-in default the first time if nothing has been registered.
SdkAllocator* nxGetSdkAllocator();

#endif
