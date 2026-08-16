#ifndef NX_PHYSICS_TRIANGLEMESH
#define NX_PHYSICS_TRIANGLEMESH
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"

class NxStream;

/**
The triangle-mesh stream format's reader, and ONLY the part of it this task
established against the shipped DLL.

phys_fn_002262 at 0x00055cb0, 511 bytes, is slot 17 of the nineteen-slot
TriangleMesh vtable at .rdata:0x00108608; slot 18 at 0x000539d0 is the matching
writer. The nineteen fields are written up in
docs/reconstruction/novodex-physics/evidence/phase4-formats.md, recovered from
both sides.

WHAT IS RECONSTRUCTED HERE IS FIELDS 1 AND 2 AND NOTHING ELSE.

The two tags are read as dwords through NxStream slot +0x0c and compared against
0x4e585354 and 0x4d455348 -- `cmp eax,0x4e585354` at 0x00055cc2 and
`cmp eax,0x4d455348` at 0x00055cd0. Either failure is the same `xor al,al` at
0x00055cd8, so the row reads ONE dword when the first tag is wrong and TWO when
the second is, which is what says where it stopped rather than only that it
refused. Neither reject arm touches `this`: the first store into the object is
`fstp dword ptr [edi+0x6c]` at 0x00055cfa, past both tests.

Fields 3 to 19 are NOT RECONSTRUCTED. The accept arm allocates the vertex and
triangle arrays through the Foundation SDK allocator at 0x101041bc -- null until
an NxPhysicsSDK exists -- and then builds an OPCODE model out of field 19 and a
convex hull out of the flag bits, which is the mesh component's work and not
this task's. Rather than answer for an arm it has not reconstructed, this entry
says so, which is what the third return value below is.
*/
enum NxTriangleMeshHeader
	{
	//! Either tag test failed; the reader returned false.
	NX_TRIANGLE_MESH_HEADER_REJECTED		= 0,
	//! Both tags matched. The remaining seventeen fields are not reconstructed.
	NX_TRIANGLE_MESH_HEADER_NOT_RECONSTRUCTED	= 1
	};

//! phys_fn_002262 (0x00055cb0), fields 1 and 2.
NxTriangleMeshHeader nxTriangleMeshReadHeader(const NxStream& stream);

#endif
