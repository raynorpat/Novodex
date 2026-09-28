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

  * opcNovodeXSetIceError corresponds to phys_fn_002160 at 0x000539b0, which
    forwards (2, file, line, 0, message) to the error-stream pointer at
    .data:0x001041b4. That row is not this task's and is not claimed here; the
    shim keeps the signature and the `return false`, which is all the vendored
    tree needs to compile and all this file asserts.

qhull's nine hooks were shims here until qhull-gap Task 4a. They now forward
to the host object published at .data:0x00125080 and live with it in
QhullHost.cpp (units/convex-cooking-contract.md).

Nothing in this file is registered against any census row and nothing in it may
be cited as closing one.
*/

#include "PhysicsInternal.h"
#include "..\..\External\opcode\novodex\OpcodeNovodeXHost.h"

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
