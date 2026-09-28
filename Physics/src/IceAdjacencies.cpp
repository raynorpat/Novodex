/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// IceAdjacencies.cpp, sub-unit A of units/convex-mesh-gap-contract.md (the name
// is the oracle's own: __FILE__ at .rdata 0x101077cc, pushed by 001539 and
// 001541). Written by convex-mesh gap Task 2c from the Capstone listing,
// 0x0002daf0..0x0002e158. The shape is ICE's Adjacencies (Pierre Terdiman),
// which is not vendored; what it calls is: RadixSort (005157/005163/005159)
// and IndexedTriangle::FindEdge (005189), called here as the oracle calls them.
//
// Integer code only (the active-edge pass is EdgeList's), so the file is not on
// the /arch:IA32 list.
//
// Every allocation goes through the 004803 getter (nxIceAlloc, EdgeList.h);
// reports go through the SetIceError seam (002160) with the oracle's file
// string, line and message, and the report's `false` is what the row returns.

#include "IceAdjacencies.h"

#include <malloc.h>

// .rdata 0x101077cc.
static const char gIceAdjacenciesFile[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\IceAdjacencies.cpp";

// phys_fn_001537 (0x0002daf0, 196 B)
// Adjacencies::AddTriangle with its three AddEdge calls inlined: the face's
// three link words start as 0xffffffff, and the edges (0,1), (0,2), (1,2) are
// appended as {smaller, larger, face}.
__declspec(noinline) void nxAdjacenciesAddTriangle(NxU32* nbEdges, AdjEdge* edges, NxU32 face,
	NxU32 ref0, NxU32 ref1, NxU32 ref2, AdjTriangle* faces)
	{
	faces[face].ATri[0] = 0xffffffff;
	faces[face].ATri[1] = 0xffffffff;
	faces[face].ATri[2] = 0xffffffff;

	if(ref0 < ref1)	{ edges[*nbEdges].Ref0 = ref0; edges[*nbEdges].Ref1 = ref1; }
	else			{ edges[*nbEdges].Ref0 = ref1; edges[*nbEdges].Ref1 = ref0; }
	edges[*nbEdges].FaceNb = face;
	(*nbEdges)++;

	if(ref0 < ref2)	{ edges[*nbEdges].Ref0 = ref0; edges[*nbEdges].Ref1 = ref2; }
	else			{ edges[*nbEdges].Ref0 = ref2; edges[*nbEdges].Ref1 = ref0; }
	edges[*nbEdges].FaceNb = face;
	(*nbEdges)++;

	if(ref1 < ref2)	{ edges[*nbEdges].Ref0 = ref1; edges[*nbEdges].Ref1 = ref2; }
	else			{ edges[*nbEdges].Ref0 = ref2; edges[*nbEdges].Ref1 = ref1; }
	edges[*nbEdges].FaceNb = face;
	(*nbEdges)++;
	}

// phys_fn_001539 (0x0002dbc0, 293 B)
// Adjacencies::UpdateLink. Both faces are read from the create block: from
// DFaces when set, then from WFaces when set -- two tests, not an else
// (0x0002dc0a), so WFaces wins when both are given; with neither the two
// IndexedTriangle locals are never written. FindEdge (005189) on each gives the
// edge number; 0xff reports at line 266 / 267 and returns the report's false.
// Each face's link for the edge becomes (other face's edge << 30) | other face.
__declspec(noinline) bool nxAdjacenciesUpdateLink(const ADJACENCIESCREATE* create, NxU32 firstTri,
	NxU32 secondTri, NxU32 ref0, NxU32 ref1, AdjTriangle* faces)
	{
	NxU32 Tri0[3];		// [esp+0xc] in the listing
	NxU32 Tri1[3];		// [esp+0x18]
	if(create->DFaces)
		{
		Tri0[0] = create->DFaces[firstTri * 3 + 0];
		Tri0[1] = create->DFaces[firstTri * 3 + 1];
		Tri0[2] = create->DFaces[firstTri * 3 + 2];
		Tri1[0] = create->DFaces[secondTri * 3 + 0];
		Tri1[1] = create->DFaces[secondTri * 3 + 1];
		Tri1[2] = create->DFaces[secondTri * 3 + 2];
		}
	if(create->WFaces)
		{
		Tri0[0] = create->WFaces[firstTri * 3 + 0];
		Tri0[1] = create->WFaces[firstTri * 3 + 1];
		Tri0[2] = create->WFaces[firstTri * 3 + 2];
		Tri1[0] = create->WFaces[secondTri * 3 + 0];
		Tri1[1] = create->WFaces[secondTri * 3 + 1];
		Tri1[2] = create->WFaces[secondTri * 3 + 2];
		}

	const NxU8 edge0 = ((const IceMaths::IndexedTriangle*) Tri0)->FindEdge(ref0, ref1);
	if(edge0 == 0xff)
		return opcNovodeXSetIceError("Adjacencies::UpdateLink: invalid edge reference in first triangle",
			gIceAdjacenciesFile, 266);

	const NxU8 edge1 = ((const IceMaths::IndexedTriangle*) Tri1)->FindEdge(ref0, ref1);
	if(edge1 == 0xff)
		return opcNovodeXSetIceError("Adjacencies::UpdateLink: invalid edge reference in second triangle",
			gIceAdjacenciesFile, 267);

	faces[firstTri].ATri[edge0] = ((NxU32) edge1 << 30) | secondTri;
	faces[secondTri].ATri[edge1] = ((NxU32) edge0 << 30) | firstTri;
	return true;
	}

// phys_fn_001541 (0x0002dcf0, 368 B)
// Adjacencies::CreateDatabase. A RadixSort on the stack sorts the edge records
// on Ref0 and then on Ref1 (keys copied into an alloca'd array, 0x0002dd10;
// the default hint), and the sorted records are read in runs of equal
// (Ref0, Ref1): a run of two links its two faces through UpdateLink, a run of
// three is a non-manifold mesh (reported at line 321, the report's false
// returned). The first record is read before the loop to seed the run and read
// again as the loop's first. The last run is linked after the loop (0x0002dd86),
// and its UpdateLink result is the row's; a failing UpdateLink inside the loop
// returns false (0x0002de4c).
__declspec(noinline) bool nxAdjacenciesCreateDatabase(NxU32 nb, AdjTriangle* faces,
	const AdjEdge* edges, const ADJACENCIESCREATE* create)
	{
	IceCore::RadixSort Core;

	NxU32* FaceNb = (NxU32*) _alloca(nb * 4);
	if(!FaceNb)
		return false;

	for(NxU32 i = 0; i < nb; i++)
		FaceNb[i] = edges[i].Ref0;
	Core.Sort(FaceNb, nb);
	for(NxU32 i = 0; i < nb; i++)
		FaceNb[i] = edges[i].Ref1;
	Core.Sort(FaceNb, nb);

	const NxU32* Sorted = Core.GetRanks();

	NxU32 LastRef0 = edges[Sorted[0]].Ref0;
	NxU32 LastRef1 = edges[Sorted[0]].Ref1;
	NxU32 Count = 0;
	NxU32 TmpBuffer[3];

	while(nb--)
		{
		const AdjEdge& e = edges[*Sorted++];
		const NxU32 Ref0 = e.Ref0;
		const NxU32 Face = e.FaceNb;
		const NxU32 Ref1 = e.Ref1;

		if(Ref0 == LastRef0 && Ref1 == LastRef1)
			{
			TmpBuffer[Count++] = Face;
			if(Count == 3)
				return opcNovodeXSetIceError("Adjacencies::CreateDatabase: can't work on non-manifold meshes.",
					gIceAdjacenciesFile, 321);
			}
		else
			{
			if(Count == 2)
				{
				if(!nxAdjacenciesUpdateLink(create, TmpBuffer[0], TmpBuffer[1], LastRef0, LastRef1, faces))
					return false;
				}
			TmpBuffer[0] = Face;
			Count = 1;
			LastRef0 = Ref0;
			LastRef1 = Ref1;
			}
		}

	bool Status = true;
	if(Count == 2)
		Status = nxAdjacenciesUpdateLink(create, TmpBuffer[0], TmpBuffer[1], LastRef0, LastRef1, faces);
	return Status;
	}

// phys_fn_001542 (0x0002de60, 91 B)
// The number of link words whose face field is 0x1fffffff (no neighbour); zero
// without faces.
NxU32 Adjacencies::ComputeNbBoundaryEdges() const
	{
	NxU32 Nb = 0;
	if(!mFaces || !mNbFaces)
		return Nb;

	const AdjTriangle* CurTri = mFaces;
	for(NxU32 i = mNbFaces; i != 0; i--)
		{
		NxU32 Count = 0;
		if((CurTri->ATri[0] & 0x1fffffff) == 0x1fffffff)	Count = 1;
		if((CurTri->ATri[1] & 0x1fffffff) == 0x1fffffff)	Count++;
		if((CurTri->ATri[2] & 0x1fffffff) == 0x1fffffff)	Count++;
		Nb += Count;
		CurTri++;
		}
	return Nb;
	}

// phys_fn_001544 (0x0002dec0, 37 B)
// Releases the faces (`new[]`, released at the pointer minus four) and clears
// the pointer. (The row is also modelled in ObjectModel.cpp, whose layout test
// keeps closing it; this is its product form.)
Adjacencies::~Adjacencies()
	{
	if(mFaces)
		{
		nxIceDeleteArray(mFaces);
		mFaces = 0;
		}
	}

// phys_fn_001546 (0x0002def0, 490 B)
// With its continuation phys_fn_001548 (0x0002e0e0, 122 B). Adjacencies::Init:
// the faces (persistent) and 3 * NbFaces temporary edge records (both `new[]`
// with a count cookie), one AddTriangle per face from DFaces, else WFaces, else
// the references 0, 1, 2; CreateDatabase; the edge records released. Then, when
// the database succeeded and vertices are given, an EdgeList on the stack built
// with FacesToEdges set and VerticesToEdges clear (and so, with vertices, the
// active edges) copies each edge's active bit into bit 29 of the link word:
// EdgeList link 0 (edge 0,1) into word 0, link 2 (edge 2,0) into word 1, link 1
// (edge 1,2) into word 2. The EdgeList's own failure is not the row's: it
// returns the database's result either way.
bool Adjacencies::Init(const ADJACENCIESCREATE& create)
	{
	if(!create.NbFaces)
		return false;

	mNbFaces = create.NbFaces;
	mFaces = (AdjTriangle*) nxIceNewArray(mNbFaces, sizeof(AdjTriangle), NX_MEMORY_PERSISTENT);
	if(!mFaces)
		return false;

	AdjEdge* Edges = (AdjEdge*) nxIceNewArray(mNbFaces * 3, sizeof(AdjEdge), NX_MEMORY_TEMP);
	if(!Edges)
		return false;

	NxU32 NbEdges = 0;
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		const NxU32 Ref0 = create.DFaces ? create.DFaces[i * 3 + 0] : create.WFaces ? create.WFaces[i * 3 + 0] : 0;
		const NxU32 Ref1 = create.DFaces ? create.DFaces[i * 3 + 1] : create.WFaces ? create.WFaces[i * 3 + 1] : 1;
		const NxU32 Ref2 = create.DFaces ? create.DFaces[i * 3 + 2] : create.WFaces ? create.WFaces[i * 3 + 2] : 2;
		nxAdjacenciesAddTriangle(&NbEdges, Edges, i, Ref0, Ref1, Ref2, mFaces);
		}

	const bool Status = nxAdjacenciesCreateDatabase(NbEdges, mFaces, Edges, &create);

	nxIceDeleteArray(Edges);

	if(!Status || !create.Verts)
		return Status;

	EDGELISTCREATE ELC;
	ELC.NbFaces = create.NbFaces;
	ELC.DFaces = create.DFaces;
	ELC.WFaces = create.WFaces;
	ELC.FacesToEdges = true;
	ELC.VerticesToEdges = false;
	ELC.Verts = create.Verts;
	ELC.Epsilon = create.Epsilon;

	EdgeList EL;
	if(EL.Init(ELC))
		{
		for(NxU32 i = 0; i < mNbFaces; i++)
			{
			const EdgeTriangle& ET = EL.mEdgeFaces[i];
			if(ET.mLink[0] & 0x80000000)	mFaces[i].ATri[0] |= 0x20000000;
			else							mFaces[i].ATri[0] &= ~0x20000000;
			if(ET.mLink[2] & 0x80000000)	mFaces[i].ATri[1] |= 0x20000000;
			else							mFaces[i].ATri[1] &= ~0x20000000;
			if(ET.mLink[1] & 0x80000000)	mFaces[i].ATri[2] |= 0x20000000;
			else							mFaces[i].ATri[2] &= ~0x20000000;
			}
		}
	return Status;
	}
