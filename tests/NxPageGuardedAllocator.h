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
	enum { HISTORY = 128 };
	NxPageGuardedAllocator() : mAllocations(0), mFellBack(0), mFrees(0)
		{ memset(mFreedSizes, 0, sizeof(mFreedSizes)); memset(mAllocSizes, 0, sizeof(mAllocSizes));
		  memset(mAllocPtrs, 0, sizeof(mAllocPtrs)); memset(mFreedPtrs, 0, sizeof(mFreedPtrs));
		  memset(mFallbackPtrs, 0, sizeof(mFallbackPtrs)); memset(mFallbackSizes, 0, sizeof(mFallbackSizes)); }

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
		size_t oldSize;
		if(!fallbackSize(old, &oldSize))
			oldSize = sizeOf(old);
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
		for(unsigned i = 0; i < HISTORY; ++i)
			if(mFallbackPtrs[i] == memory)
				{
				const unsigned releasedSize = static_cast<unsigned>(mFallbackSizes[i]);
				::free(memory);
				mFallbackPtrs[i] = 0;
				mFallbackSizes[i] = 0;
				mFreedSizes[mFrees % HISTORY] = releasedSize;
				mFreedPtrs[mFrees % HISTORY] = memory;
				++mFrees;
				return;
			}
		const unsigned releasedSize = static_cast<unsigned>(sizeOf(memory));
		if(VirtualFree(baseOf(memory), 0, MEM_RELEASE))
			{
			mFreedSizes[mFrees % HISTORY] = releasedSize;
			mFreedPtrs[mFrees % HISTORY] = memory;
			++mFrees;
			}
		}

	unsigned allocations() const { return mAllocations; }
	unsigned fellBack() const { return mFellBack; }
	unsigned frees() const { return mFrees; }
	unsigned freedSizeFromEnd(unsigned n) const
		{ return n < mFrees && n < HISTORY ? mFreedSizes[(mFrees - 1 - n) % HISTORY] : 0; }
	unsigned allocSizeFromEnd(unsigned n) const
		{ return n < mAllocations && n < HISTORY ? mAllocSizes[(mAllocations - 1 - n) % HISTORY] : 0; }
	void* allocPointerFromEnd(unsigned n) const
		{ return n < mAllocations && n < HISTORY ? mAllocPtrs[(mAllocations - 1 - n) % HISTORY] : 0; }
	void* freedPointerFromEnd(unsigned n) const
		{ return n < mFrees && n < HISTORY ? mFreedPtrs[(mFrees - 1 - n) % HISTORY] : 0; }

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

	bool fallbackSize(void* memory, size_t* size) const
		{
		for(unsigned i = 0; i < HISTORY; ++i)
			if(mFallbackPtrs[i] == memory)
				{
				*size = mFallbackSizes[i];
				return true;
				}
		return false;
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
#ifdef NX_PAGE_GUARDED_FILL
			void* plain = ::malloc(size);
#else
			void* plain = ::malloc(size);
#endif
			if(!plain)
				return 0;
			for(unsigned i = 0; i < HISTORY; ++i)
				if(!mFallbackPtrs[i])
					{
					mFallbackPtrs[i] = plain;
					mFallbackSizes[i] = size;
#ifdef NX_PAGE_GUARDED_FILL
					memset(plain, 0xcd, size);
#endif
					return plain;
					}
			::free(plain);
			return 0;
			}

		*reinterpret_cast<unsigned*>(base) = static_cast<unsigned>(size);
		unsigned char* block = base + page - size;
#ifdef NX_PAGE_GUARDED_FILL
		// Fresh pages are zeroed, which hides a constructor that leaves a field to
		// the allocation. A target built with NX_PAGE_GUARDED_FILL gets every
		// block filled with 0xcd instead, like a debug heap's fresh memory.
		memset(block, 0xcd, size);
#endif
		mAllocSizes[mAllocations % HISTORY] = static_cast<unsigned>(size);
		mAllocPtrs[mAllocations % HISTORY] = block;
		++mAllocations;
		return block;
		}

	unsigned mAllocations;
	unsigned mFellBack;
	unsigned mFrees;
	unsigned mFreedSizes[HISTORY];
	unsigned mAllocSizes[HISTORY];
	void* mAllocPtrs[HISTORY];
	void* mFreedPtrs[HISTORY];
	// Oversized blocks use the CRT heap; keep their sizes so realloc/free never
	// interpret those pointers as page-aligned VirtualAlloc blocks.
	void* mFallbackPtrs[HISTORY];
	size_t mFallbackSizes[HISTORY];
	};
