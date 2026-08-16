/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "TriangleMesh.h"

#include "NxStream.h"

// The two tags are read as DWORDS, so on the little-endian target the bytes on
// disc are 54 53 58 4e and 48 53 45 4d. Written most significant byte first the
// two constants spell NXST and MESH; in file order they spell TSXN and HSEM.
// Which spelling the source used is not established and the writer at 0x000539d0
// does not settle it -- it pushes the same two 32-bit values. What IS
// established is the value each comparison requires, so that is what is written
// here.
static const NxU32 kTriangleMeshTag0 = 0x4e585354;	// cmp at 0x00055cc2
static const NxU32 kTriangleMeshTag1 = 0x4d455348;	// cmp at 0x00055cd0

NxTriangleMeshHeader nxTriangleMeshReadHeader(const NxStream& stream)
	{
	// Two separate reads with the first test between them. A reader that read
	// both tags before testing either would consume two dwords for a bad first
	// tag, and the asset differential measures exactly that.
	if(stream.readDword() != kTriangleMeshTag0)
		return NX_TRIANGLE_MESH_HEADER_REJECTED;
	if(stream.readDword() != kTriangleMeshTag1)
		return NX_TRIANGLE_MESH_HEADER_REJECTED;

	return NX_TRIANGLE_MESH_HEADER_NOT_RECONSTRUCTED;
	}
