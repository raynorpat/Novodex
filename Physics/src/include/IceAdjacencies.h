#ifndef NX_PHYSICS_ICEADJACENCIES_H
#define NX_PHYSICS_ICEADJACENCIES_H
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The ICE-shaped triangle adjacencies of `\Epic\Novodex\SDKs\Physics\src\
// IceAdjacencies.cpp` (the oracle's own __FILE__, .rdata 0x101077cc), sub-unit
// A of units/convex-mesh-gap-contract.md, written by convex-mesh gap Task 2c.
// Not vendored (OPCODE 1.3's Ice/ has no IceAdjacencies); the layouts are the
// listing's, at the sites cited.

#include "EdgeList.h"

// The per-face link word: bits 0-28 the adjacent face (0x1fffffff = none, the
// 0xffffffff every word starts as), bit 29 the active-edge flag (0x0002e0f0),
// bits 30-31 the adjacent face's edge number (0x0002dcb8). Words 0, 1 and 2 are
// the face's edges (0,1), (0,2) and (1,2) -- AddTriangle's order.
struct AdjTriangle
	{
	NxU32	ATri[3];
	};

// One temporary edge record: the smaller and larger vertex reference and the
// face (0x0002db16..0x0002db39).
struct AdjEdge
	{
	NxU32	Ref0;
	NxU32	Ref1;
	NxU32	FaceNb;
	};

// The create block 001546 reads: +0x00 face count, +0x04 32-bit faces, +0x08
// 16-bit faces, +0x0c vertices (the active-edge pass runs only when non-null),
// +0x10 epsilon (handed on to the EdgeList create block at 0x0002e095).
struct ADJACENCIESCREATE
	{
	NxU32			NbFaces;
	const NxU32*	DFaces;
	const NxU16*	WFaces;
	const IceMaths::Point*	Verts;
	float			Epsilon;
	};

// +0x00 face count, +0x04 faces (`new[]` with a count cookie, persistent).
class Adjacencies
	{
	public:
				Adjacencies();
				~Adjacencies();

	bool		Init(const ADJACENCIESCREATE& create);
	NxU32		ComputeNbBoundaryEdges() const;

	NxU32			mNbFaces;
	AdjTriangle*	mFaces;
	};

// The three rows the image calls with register arguments, written as ordinary
// functions with those registers as parameters (MSVC cannot declare the
// listing's conventions). They are free functions, not members: none of them
// reads the Adjacencies object.

// Row phys_fn_001537 at 0x0002daf0: EAX = &nbEdges, ECX = edges, EDX = face,
// EDI = ref0; stack ref1, ref2, faces.
void nxAdjacenciesAddTriangle(NxU32* nbEdges, AdjEdge* edges, NxU32 face, NxU32 ref0,
	NxU32 ref1, NxU32 ref2, AdjTriangle* faces);

// Row phys_fn_001539 at 0x0002dbc0: ECX = the create block, EDI = first face,
// EAX = second face; stack ref0, ref1, faces.
bool nxAdjacenciesUpdateLink(const ADJACENCIESCREATE* create, NxU32 firstTri, NxU32 secondTri,
	NxU32 ref0, NxU32 ref1, AdjTriangle* faces);

// Row phys_fn_001541 at 0x0002dcf0: EAX = the edge count; stack faces, edges,
// the create block.
bool nxAdjacenciesCreateDatabase(NxU32 nb, AdjTriangle* faces, const AdjEdge* edges,
	const ADJACENCIESCREATE* create);

#endif
