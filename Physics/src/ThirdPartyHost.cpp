/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/*
The host side of the two seams the vendored third-party libraries leave through.
See External/README.md.

READ THIS BEFORE TREATING ANY OF IT AS A RECONSTRUCTION.

Only ONE function here corresponds to a row this project has recovered:
opcNovodeXAlloc/opcNovodeXFree reach nxGetSdkAllocator, which IS phys_fn_004803
at 0x000b4000 and was closed in Phase 2. Everything else is a LINKAGE SHIM. In
particular:

  * qhull's four hooks belong to a NovodeX class with a nine-slot vtable at
    .rdata:0x00113614, constructed at 0x0007e370 with a 16,384-byte inline
    arena, held in the global at .data:0x00125080 and written exactly once at
    0x0007ea51. That class is a Task 2b row. Nothing here reconstructs it: the
    printf hook writes nowhere, the error exit aborts, and malloc/free forward
    to the SDK allocator because the shipped object's slot +0x14 has to come
    from somewhere and the arena has not been recovered.

  * opcNovodeXSetIceError corresponds to phys_fn_002160 at 0x000539b0, which
    forwards (2, file, line, 0, message) to the error-stream pointer at
    .data:0x001041b4. That row is not this task's and is not claimed here; the
    shim keeps the signature and the `return false`, which is all the vendored
    tree needs to compile and all this file asserts.

Nothing in this file is registered against any census row and nothing in it may
be cited as closing one.
*/

#include "PhysicsInternal.h"
#include "..\..\External\opcode\novodex\OpcodeNovodeXHost.h"

extern "C" {
#include "..\..\External\qhull\novodex\QhullNovodeXHost.h"
}

//////////////////////////////////////////////////////////////////////////////
// OPCODE

void* opcNovodeXAlloc(size_t size)
{
	// phys_fn_004803 (0x000b4000) -> [vtable+0x00] (size, 0).
	return nxGetSdkAllocator()->malloc(size, NX_MEMORY_PERSISTENT);
}

void opcNovodeXFree(void* memory)
{
	// phys_fn_004803 (0x000b4000) -> [vtable+0x0c] (pointer).
	nxGetSdkAllocator()->free(memory);
}

bool opcNovodeXSetIceError(const char* /*message*/, const char* /*file*/, int /*line*/)
{
	// SHIM. phys_fn_002160 at 0x000539b0 is the row; it is not reconstructed
	// here. What the vendored tree depends on is the `false`.
	return false;
}

//////////////////////////////////////////////////////////////////////////////
// qhull -- all four are shims for the Task 2b class at .data:0x00125080

int qhNovodeXFprintf(FILE* /*stream*/, const char* /*format*/, ...)
{
	return 0;
}

void* qhNovodeXMalloc(size_t size)
{
	return nxGetSdkAllocator()->malloc(size, NX_MEMORY_PERSISTENT);
}

void qhNovodeXFree(void* memory)
{
	nxGetSdkAllocator()->free(memory);
}

void qhNovodeXErrexit(int /*exitcode*/)
{
	// The shipped slot +0x20 does not return to qhull. Neither does this: a
	// hook that returned would let qhull carry on past an error exit, which is
	// a worse lie than stopping.
	abort();
}
