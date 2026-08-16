#ifndef NX_PHYSICS_PMAP
#define NX_PHYSICS_PMAP
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"
#include "MemoryStream.h"

class NxUserOutputStream;
namespace IceCore { class Container; }

/**
The penetration map: a cubic occupancy grid a triangle mesh carries so that
deep-penetration queries have somewhere to look. The file format and every
offset below are written up in
docs/reconstruction/novodex-physics/evidence/phase4-formats.md.

The rows, all Phase 4, all in the pmap component 0x0004cae0-0x00051080:

	phys_fn_002045	0x000505f0	 69	PenetrationMap::PenetrationMap
	phys_fn_001986	0x0004cb20	115	the bit-spreading table the sort uses
	phys_fn_001984	0x0004cae0	 57	PenetrationMap::~PenetrationMap
	phys_fn_002047	0x00050640	2340	PenetrationMap::Create
	phys_fn_002033	0x0004ff80	398	setup from the resolution and the AABB
	phys_fn_002035	0x00050110	439	the payload bit stream
	phys_fn_002008	0x0004dba0	909	the cell-run decoder
	phys_fn_002037	0x000502d0	 42	the corner pass and the Morton reorder
	phys_fn_002039	0x00050300	 10	  "  (the census splits it four ways on
	phys_fn_002041	0x00050310	429	  "   alignment padding; it is one
	phys_fn_002043	0x000504c0	296	  "   function)
	phys_fn_002051	0x00051040	 32	NxReleasePMap

Layout, 0x78 bytes, from `push 0x78` at 0x00053d35 in TriangleMesh::loadPMap.
Every offset carries the instruction that writes it.

	+0x00	vptr			0x10107f40, one slot, installed at 0x000505f3
	+0x04	spread table	256 dwords, allocated at 0x0004cb28
	+0x08	min[3]			seeded 0x7f7fffff at 0x000505fe, copied 0x0004ffbf
	+0x14	max[3]			seeded 0xff7fffff at 0x0005060c, copied 0x0004ffd3
	+0x20	centre[3]		(min+max)*0.5f, 0x0004ffe8 and 0x00050006
	+0x2c	halfExtents[3]	(max-min)*0.5f, 0x00050027 and 0x0005003f
	+0x38	extents[3]		 max-min, 0x00050060..0x00050082
	+0x44	cellsPerUnit[3]	(n-1)/extent, 0x00050085..0x0005009d
	+0x50	unitsPerCell[3]	extent/(n-1), 0x000500a0..0x000500c6
	+0x5c	resolution n	0x0004ff99
	+0x60	n*n				0x0004ff9c
	+0x64	(float)(n-1)	0x0004ffab
	+0x68	1/(n-1)			0x0004ffba
	+0x6c	n*n*n			0x000500ca
	+0x70	grid			malloc'd at 0x000500cd, stored 0x000500ef
	+0x74	mesh			0x0005073e

THE GRID IS CRT HEAP, NOT THE SDK ALLOCATOR. 0x000500cd calls 0x000f48c0 and the
destructor calls 0x000f48bb -- malloc and free -- while the allocator singleton
at 0x101041bc is never consulted on this path. The same is true of the spread
table and of the NxPMap buffer NxCreatePMap hands out.

WHAT IS NOT RECONSTRUCTED, and must not be guessed:

  * The COMPUTE arm of Create, 0x00050768-0x00050f02. It rasterises the mesh
    through an OPCODE tree it builds on the spot and is roughly two thousand of
    this row's 2,340 bytes. It needs a real InternalTriangleMesh, which is the
    mesh component's and not this task's.
  * The filename arm of the payload loader, 0x00050123-0x0005014f, and the arm
    at 0x0005017b that builds its own stream when the caller passes none. Both
    reach serialization rows this task does not cover.
  * The 5-bit cell-walk codes in the cell-run decoder. The 32-way jump table at
    0x0004dc75 moves three cursor globals and the encoding is unestablished; the
    decoder below reproduces the code-width selection, the count read and the
    zero-count exit, which is what the recorded fixtures drive, and refuses
    rather than inventing the walk.
  * NxCreatePMap (phys_fn_002049, 0x00050f70). Its whole body is the COMPUTE
    arm above.
*/
class PenetrationMap
	{
	public:
	// phys_fn_002045 (0x000505f0). Returns `this` in the image; that is the
	// constructor calling convention, not a member of the class's interface.
						PenetrationMap();
	// phys_fn_001984 (0x0004cae0). Virtual: the one slot of 0x10107f40.
	virtual				~PenetrationMap();

	/**
	phys_fn_002047 (0x00050640), the LOAD arm only.

	`mesh` is the InternalTriangleMesh. On this arm the only thing read out of
	it is the six floats at +0x44 -- `lea ecx,[edi+0x44]` at 0x0005072d -- and
	the pointer is stored at +0x74 whether or not the load succeeds.
	*/
	bool				create(const void* mesh, NxU32 resolution, const char* filename,
							MemoryStream* stream, bool load, NxUserOutputStream* outputStream);

	NxU32				getResolution()	const	{ return mResolution;	}
	NxU32				getCellCount()	const	{ return mCellCount;	}
	const NxU32*		getGrid()		const	{ return mGrid;			}

	private:
	// phys_fn_001986 (0x0004cb20).
	void				buildSpreadTable();
	// phys_fn_002033 (0x0004ff80).
	bool				setup(NxU32 resolution, const NxF32* bounds);
	// phys_fn_002035 (0x00050110), the arm that reads from a supplied stream.
	bool				loadPayload(MemoryStream& stream);
	// phys_fn_002008 (0x0004dba0). Returns the number of cell indices appended.
	static NxU32		decodeCellRun(MemoryStream& stream, IceCore::Container& cells, NxU32 resolution);
	// phys_fn_002037/002039/002041/002043 (0x000502d0..0x000505e8).
	bool				finish();

	NxU32*				mSpread;
	NxF32				mMin[3];
	NxF32				mMax[3];
	NxF32				mCentre[3];
	NxF32				mHalfExtents[3];
	NxF32				mExtents[3];
	NxF32				mCellsPerUnit[3];
	NxF32				mUnitsPerCell[3];
	NxU32				mResolution;
	NxU32				mResolutionSquared;
	NxF32				mLastIndex;
	NxF32				mInvLastIndex;
	NxU32				mCellCount;
	NxU32*				mGrid;
	const void*			mMesh;
	};

#endif
