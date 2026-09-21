// A page-guarded NxUserAllocator, which is the instrument 10n named.
//
// The harness passes a null allocator to NxCreatePhysicsSDK, so the oracle's default
// is used and every allocation is opaque. Passing one instead routes EVERY SDK
// allocation through this class -- the bridge in PhysicsInternal.h forwards to
// whatever the caller supplied -- and each block is placed so its last byte is
// immediately before an inaccessible page. A write past the end then faults AT THE
// WRITE, instead of corrupting a later allocation and surfacing as a crash in
// unrelated code.
//
// This is the instrument that does not move the fault: the blocks are still real
// allocations, and only their boundaries are guarded.
//
// Layout of one block, in a two-page reservation [base, base+0x2000):
//
//     base                     a four-byte size word, so free() can find the base
//     base+4 .. block          padding
//     block .. base+0x1000     the caller's bytes, ENDING at the guard page
//     base+0x1000 .. +0x2000   PAGE_NOACCESS
//
// and the pointer returned is `block`, so the caller's block ends exactly where the
// guard begins.

#include "NxUserAllocator.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>

class NxPageGuardedAllocator : public NxUserAllocator
	{
	public:
	NxPageGuardedAllocator() : mAllocations(0), mFellBack(0) {}

	virtual void* malloc(size_t size)
		{
		return allocate(size, NX_MEMORY_PERSISTENT);
		}

	virtual void* malloc(size_t size, NxMemoryType type)
		{
		return allocate(size, type);
		}

	virtual void* mallocDEBUG(size_t size, const char* fileName, int line)
		{
		(void)fileName; (void)line;
		return allocate(size, NX_MEMORY_PERSISTENT);
		}

	virtual void* mallocDEBUG(size_t size, const char* fileName, int line,
		const char* className, NxMemoryType type)
		{
		(void)fileName; (void)line; (void)className;
		return allocate(size, type);
		}

	virtual void* realloc(void* memory, size_t size)
		{
		if(!memory)
			return allocate(size, NX_MEMORY_PERSISTENT);
		unsigned char* old = static_cast<unsigned char*>(memory);
		const size_t oldSize = sizeOf(old);
		void* fresh = allocate(size, NX_MEMORY_PERSISTENT);
		if(fresh)
			{
			memcpy(fresh, old, oldSize < size ? oldSize : size);
			free(old);
			}
		return fresh;
		}

	virtual void free(void* memory)
		{
		if(!memory)
			return;
		VirtualFree(baseOf(memory), 0, MEM_RELEASE);
		}

	unsigned allocations() const { return mAllocations; }
	unsigned fellBack() const { return mFellBack; }

	private:
	static unsigned char* baseOf(void* memory)
		{
		// The reservation is 8 KB and the block lives in its first page, so the base
		// is the page boundary below the returned pointer.
		return reinterpret_cast<unsigned char*>(
			reinterpret_cast<uintptr_t>(memory) & ~static_cast<uintptr_t>(0xFFF));
		}

	static size_t sizeOf(void* memory)
		{
		return *reinterpret_cast<unsigned*>(baseOf(memory));
		}

	void* allocate(size_t size, NxMemoryType type)
		{
		(void)type;
		const size_t page = 0x1000;
		unsigned char* base = static_cast<unsigned char*>(
			VirtualAlloc(0, page * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		if(!base)
			return 0;

		DWORD old = 0;
		if(!VirtualProtect(base + page, page, PAGE_NOACCESS, &old))
			{
			VirtualFree(base, 0, MEM_RELEASE);
			return 0;
			}

		// Four bytes of header at the base, and the block ends at the guard. Anything
		// that does not fit in one page falls back to the default allocator rather
		// than being silently unguarded.
		const size_t usable = page - 4;
		if(size > usable)
			{
			++mFellBack;
			VirtualFree(base, 0, MEM_RELEASE);
			// A plain allocation, deliberately NOT routed back through this class:
			// recursion here would be worse than an unguarded block, and the scene
			// path allocates at most 0x710 bytes.
			return ::malloc(size);
			}

		*reinterpret_cast<unsigned*>(base) = static_cast<unsigned>(size);
		unsigned char* block = base + page - size;
		++mAllocations;
		return block;
		}

	unsigned mAllocations;
	unsigned mFellBack;
	};