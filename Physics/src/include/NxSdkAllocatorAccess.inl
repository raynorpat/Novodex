// Shared exact production allocator definitions, included once per program.
// The built-in fallback vtable: phys_fn_004807 malloc, phys_fn_004808 realloc,
// and phys_fn_004810 free belong to Phase 6; phys_fn_004812 mallocDEBUG is the
// Phase 5 null stub. These definitions give phys_fn_004803 an object to install.
void* SdkDefaultAllocator::malloc(size_t size, NxMemoryType)
	{
	return ::malloc(size);
	}

void* SdkDefaultAllocator::mallocDEBUG(size_t, const char*, int, const char*, NxMemoryType)
	{
	return 0;
	}

void* SdkDefaultAllocator::realloc(void* memory, size_t size)
	{
	return ::realloc(memory, size);
	}

void SdkDefaultAllocator::free(void* memory)
	{
	::free(memory);
	}

// .data 0x0012845c and .data 0x00122368.
static SdkAllocator* gSdkAllocator = 0;
static SdkDefaultAllocator gSdkDefaultAllocator;

bool nxSetSdkAllocatorBridge(SdkAllocator* allocator)
	{
	gSdkAllocator = allocator;
	return true;
	}

// phys_fn_004803 (0x000b4000). The install at 0x000b400e is a store, not just a
// return, so the second call reads back what the first one wrote.
SdkAllocator* nxGetSdkAllocator()
	{
	if(!gSdkAllocator)
		gSdkAllocator = &gSdkDefaultAllocator;
	return gSdkAllocator;
	}
