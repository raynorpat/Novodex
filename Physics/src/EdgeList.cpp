/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// EdgeList.cpp, prerequisite P-EdgeList of units/convex-mesh-gap-contract.md
// (the name is the oracle's own: __FILE__ at .rdata 0x101080cc, pushed by
// 002054 and 002061). Written by convex-mesh gap Task 2c from the Capstone
// listing, 0x00051060..0x00051eb0. The shape is ICE's EdgeList (Pierre
// Terdiman), which is not vendored: OPCODE 1.3's Ice/ has no EdgeList, so the
// listing is the only source. What the calls reach IS vendored and is called
// here as the oracle calls it: RadixSort (005157/005163/005159),
// Plane::Set (005155) and Triangle::Normal (005181).
//
// x87: on the /arch:IA32 list. 002061's two float sections -- the side of the
// plane (0x0005189f..0x000518d4) and the angle between the two normals
// (0x000519c1..0x00051a3a) -- are x87 assembly blocks transcribed from the
// listing: the angle keeps |n0 x n1| on the FPU stack into fpatan, which C++
// cannot express (no fpatan, and a double would travel through a qword).
//
// Reports go through the SetIceError seam (002160, OpcodeNovodeXHost.h) with the
// oracle's file string, line and message; each report's `false` is the row's
// return value, as in the listing (the report's `xor al, al` is what the row
// returns).

#include "EdgeList.h"

#include <string.h>
#if !NX_PHYSICS_USE_X87
#include <cmath>
#endif

// .rdata 0x101080cc.
static const char gEdgeListFile[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\EdgeList.cpp";

// 0x101041f0 and 0x10106954, the two dword constants 002061 compares with.
static const float gEdgeListZero = 0.0f;
static const float gEdgeListActiveAngle = 0.1f;

// phys_fn_002052 (0x00051060, 19 B)
// Zeroes +0x00, +0x04, +0x0c, +0x10 and +0x14; +0x08 is left as it was.
EdgeList::EdgeList()
	{
	mNbEdges = 0;
	mEdges = 0;
	mEdgeFaces = 0;
	mEdgeToTriangles = 0;
	mFacesByEdges = 0;
	}

// phys_fn_002060 (0x000515d0, 112 B)
// Releases +0x14, +0x10, +0x04 and +0x0c in that order, each only when non-null
// and each cleared afterwards; the last two are `new[]` blocks with a count
// cookie and are released at the pointer minus four.
EdgeList::~EdgeList()
	{
	if(mFacesByEdges)
		{
		nxIceFree(mFacesByEdges);
		mFacesByEdges = 0;
		}
	if(mEdgeToTriangles)
		{
		nxIceFree(mEdgeToTriangles);
		mEdgeToTriangles = 0;
		}
	if(mEdges)
		{
		nxIceDeleteArray(mEdges);
		mEdges = 0;
		}
	if(mEdgeFaces)
		{
		nxIceDeleteArray(mEdgeFaces);
		mEdgeFaces = 0;
		}
	}

// phys_fn_002054 (0x00051080, 554 B)
// With its continuation phys_fn_002056 (0x000512b0, 313 B). The faces-to-edges
// table: every face's three edges as sorted vertex pairs, radix sorted on the
// larger then the smaller reference, one EdgeData per run of equal pairs and
// each face's link set to its run's index. A second call is a no-op once
// mEdgeFaces exists (0x000510ab). Every failing allocation returns false and
// releases nothing (0x00051365): the listing leaks what it has taken so far,
// and the one taken after the sorter exists releases only the sorter.
bool EdgeList::CreateFacesToEdges(NxU32 nb_faces, const NxU32* dfaces, const NxU16* wfaces)
	{
	if(!nb_faces || (!dfaces && !wfaces))
		return opcNovodeXSetIceError("EdgeList::CreateFacesToEdges: null parameter!", gEdgeListFile, 0x72);

	if(mEdgeFaces)
		return true;

	// new EdgeTriangle[nb_faces], persistent (0x000510be..0x000510f2).
	mEdgeFaces = (EdgeTriangle*) nxIceNewArray(nb_faces, sizeof(EdgeTriangle), NX_MEMORY_PERSISTENT);
	if(!mEdgeFaces)
		return false;

	// The two reference lists, temporary, no cookie (0x000510fb, 0x00051117).
	NxU32* VRefs0 = (NxU32*) nxIceAlloc(nb_faces * 12, NX_MEMORY_TEMP);
	if(!VRefs0)
		return false;
	NxU32* VRefs1 = (NxU32*) nxIceAlloc(nb_faces * 12, NX_MEMORY_TEMP);
	if(!VRefs1)
		return false;

	// new EdgeData[nb_faces * 3], temporary, with a cookie (0x0005113a).
	const NxU32 nbRefs = nb_faces * 3;
	EdgeData* Buffer = (EdgeData*) nxIceNewArray(nbRefs, sizeof(EdgeData), NX_MEMORY_TEMP);
	if(!Buffer)
		return false;

	// 0x00051182..0x00051250: each face's edges (0,1), (1,2), (2,0), smaller
	// reference into VRefs0. Neither face array: the references 0, 1, 2.
	for(NxU32 i = 0; i < nb_faces; i++)
		{
		NxU32 Ref0, Ref1, Ref2;
		if(dfaces)
			{
			Ref0 = dfaces[i * 3 + 0];
			Ref1 = dfaces[i * 3 + 1];
			Ref2 = dfaces[i * 3 + 2];
			}
		else if(wfaces)
			{
			Ref0 = wfaces[i * 3 + 0];
			Ref1 = wfaces[i * 3 + 1];
			Ref2 = wfaces[i * 3 + 2];
			}
		else
			{
			Ref0 = 0;
			Ref1 = 1;
			Ref2 = 2;
			}

		if(Ref0 < Ref1)	{ VRefs0[i * 3 + 0] = Ref0; VRefs1[i * 3 + 0] = Ref1; }
		else			{ VRefs0[i * 3 + 0] = Ref1; VRefs1[i * 3 + 0] = Ref0; }

		if(Ref1 < Ref2)	{ VRefs0[i * 3 + 1] = Ref1; VRefs1[i * 3 + 1] = Ref2; }
		else			{ VRefs0[i * 3 + 1] = Ref2; VRefs1[i * 3 + 1] = Ref1; }

		if(Ref2 < Ref0)	{ VRefs0[i * 3 + 2] = Ref2; VRefs1[i * 3 + 2] = Ref0; }
		else			{ VRefs0[i * 3 + 2] = Ref0; VRefs1[i * 3 + 2] = Ref2; }
		}

	// 0x00051262..0x00051283: the larger references first, then the smaller,
	// both with the default hint (0, signed).
	IceCore::RadixSort Sorter;
	const NxU32* Sorted = Sorter.Sort(VRefs1, nbRefs).Sort(VRefs0, nbRefs).GetRanks();

	mNbFaces = nb_faces;
	mNbEdges = 0;

	// 0x000512b0..0x0005131d. Both "last" references start at 0xffffffff.
	NxU32 LastRef0 = 0xffffffff;
	NxU32 LastRef1 = 0xffffffff;
	for(NxU32 i = 0; i < nbRefs; i++)
		{
		const NxU32 SortedIndex = Sorted[i];
		const NxU32 Face = SortedIndex / 3;
		const NxU32 Edge = SortedIndex % 3;
		const NxU32 Ref0 = VRefs0[SortedIndex];
		const NxU32 Ref1 = VRefs1[SortedIndex];
		if(Ref0 != LastRef0 || Ref1 != LastRef1)
			{
			Buffer[mNbEdges].Ref0 = Ref0;
			Buffer[mNbEdges].Ref1 = Ref1;
			mNbEdges++;
			}
		mEdgeFaces[Face].mLink[Edge] = mNbEdges - 1;
		LastRef1 = Ref1;
		LastRef0 = Ref0;
		}

	// new EdgeData[mNbEdges], persistent (0x00051325); on failure only the
	// sorter is released (0x0005135c).
	mEdges = (EdgeData*) nxIceNewArray(mNbEdges, sizeof(EdgeData), NX_MEMORY_PERSISTENT);
	if(!mEdges)
		return false;

	memcpy(mEdges, Buffer, mNbEdges * sizeof(EdgeData));

	// 0x0005138a..0x000513b5: the buffer (cookie), then VRefs1, then VRefs0.
	nxIceDeleteArray(Buffer);
	nxIceFree(VRefs1);
	nxIceFree(VRefs0);
	return true;
	}

// phys_fn_002058 (0x000513f0, 467 B)
// The edges-to-faces table on top of the faces-to-edges one: per edge the number
// of faces sharing it and the offset of their run in mFacesByEdges. The
// EdgeDesc array has no cookie; its constructor (0x1002a610) zeroes the three
// fields. The offsets are advanced while the faces are written and then formed
// again (0x0005158a), so they end as run starts.
bool EdgeList::CreateEdgesToFaces(NxU32 nb_faces, const NxU32* dfaces, const NxU16* wfaces)
	{
	if(!CreateFacesToEdges(nb_faces, dfaces, wfaces))
		return false;

	const NxU32 nbEdges = mNbEdges;
	EdgeDesc* descs = (EdgeDesc*) nxIceAlloc(nbEdges * sizeof(EdgeDesc), NX_MEMORY_PERSISTENT);
	if(descs)
		{
		for(NxU32 i = 0; i < nbEdges; i++)
			{
			descs[i].Flags = 0;
			descs[i].Count = 0;
			descs[i].Offset = 0;
			}
		}
	mEdgeToTriangles = descs;
	if(!mEdgeToTriangles)
		return false;

	// 0x0005144f..0x00051493: count the faces on each edge.
	for(NxU32 i = 0; i < nb_faces; i++)
		{
		mEdgeToTriangles[mEdgeFaces[i].mLink[0]].Count++;
		mEdgeToTriangles[mEdgeFaces[i].mLink[1]].Count++;
		mEdgeToTriangles[mEdgeFaces[i].mLink[2]].Count++;
		}

	// 0x00051495..0x000514c9: run starts.
	mEdgeToTriangles[0].Offset = 0;
	for(NxU32 i = 1; i < mNbEdges; i++)
		mEdgeToTriangles[i].Offset = mEdgeToTriangles[i - 1].Count + mEdgeToTriangles[i - 1].Offset;

	// 0x000514cb..0x000514f5: the last run's end sizes mFacesByEdges.
	const NxU32 LastOffset = mEdgeToTriangles[mNbEdges - 1].Count + mEdgeToTriangles[mNbEdges - 1].Offset;
	mFacesByEdges = (NxU32*) nxIceAlloc(LastOffset * 4, NX_MEMORY_PERSISTENT);
	if(!mFacesByEdges)
		return false;

	// 0x00051500..0x00051588.
	for(NxU32 i = 0; i < nb_faces; i++)
		{
		mFacesByEdges[mEdgeToTriangles[mEdgeFaces[i].mLink[0]].Offset++] = i;
		mFacesByEdges[mEdgeToTriangles[mEdgeFaces[i].mLink[1]].Offset++] = i;
		mFacesByEdges[mEdgeToTriangles[mEdgeFaces[i].mLink[2]].Offset++] = i;
		}

	// 0x0005158a..0x000515b8.
	mEdgeToTriangles[0].Offset = 0;
	for(NxU32 i = 1; i < mNbEdges; i++)
		mEdgeToTriangles[i].Offset = mEdgeToTriangles[i - 1].Count + mEdgeToTriangles[i - 1].Offset;

	return true;
	}

// phys_fn_002061 (0x00051640, 1933 B)
// Active edges and vertices. An edge is active when one face uses it, or when
// two do, the vertex of the first face opposite the edge lies strictly below
// the second face's plane (a convex edge) and the normals are more than 0.1
// radians apart; any other count is inactive. The marks are copied into the
// links (bit 31) and the descriptors (bit 0); then every vertex of an active
// edge is marked and copied into the links (bit 30, link j for vertex j). The
// epsilon argument is never read: the angle is compared with the constant 0.1f
// at 0x10106954. The vertex marks are sized by the largest reference over
// nb_faces faces plus one; the link loops run over mNbFaces.
bool EdgeList::ComputeActiveEdges(NxU32 nb_faces, const NxU32* dfaces, const NxU16* wfaces,
	const IceMaths::Point* verts, float /*epsilon*/)
	{
	if(!dfaces && !wfaces)
		return opcNovodeXSetIceError("EdgeList::ComputeActiveEdges: null parameter!", gEdgeListFile, 0x10a);
	if(!verts)
		return opcNovodeXSetIceError("EdgeList::ComputeActiveEdges: null parameter!", gEdgeListFile, 0x10b);

	NxU32 NbEdges = mNbEdges;
	if(!NbEdges)
		return opcNovodeXSetIceError("ActiveEdges::ComputeConvexEdges: no edges in edge list!", gEdgeListFile, 0x10e);
	const EdgeData* Edges = mEdges;
	if(!Edges)
		return opcNovodeXSetIceError("ActiveEdges::ComputeConvexEdges: no edge data in edge list!", gEdgeListFile, 0x111);
	const EdgeDesc* ED = mEdgeToTriangles;
	if(!ED)
		return opcNovodeXSetIceError("ActiveEdges::ComputeConvexEdges: no edge-to-triangle in edge list!", gEdgeListFile, 0x114);
	const NxU32* FBE = mFacesByEdges;
	if(!FBE)
		return opcNovodeXSetIceError("ActiveEdges::ComputeConvexEdges: no faces-by-edges in edge list!", gEdgeListFile, 0x117);

	// One byte per edge, temporary (0x00051741).
	bool* ActiveEdges = (bool*) nxIceAlloc(NbEdges, NX_MEMORY_TEMP);
	if(!ActiveEdges)
		return false;

	// The face references survive from one edge to the next in the listing
	// (spilled at 0x00051807 and reloaded at 0x00051a41); only a call with neither
	// face array could observe that, and the checks above exclude it.
	NxU32 VRef00 = 0, VRef01 = 0, VRef02 = 0;
	NxU32 VRef10 = 0, VRef11 = 0, VRef12 = 0;

	bool* CurrentMark = ActiveEdges;
	do
		{
		const NxU32 Count = ED->Count;
		bool Active = false;
		if(Count == 1)
			{
			Active = true;
			}
		else if(Count == 2)
			{
			const NxU32 FaceIndex0 = FBE[ED->Offset + 0] * 3;
			const NxU32 FaceIndex1 = FBE[ED->Offset + 1] * 3;
			if(dfaces)
				{
				VRef00 = dfaces[FaceIndex0 + 0];
				VRef01 = dfaces[FaceIndex0 + 1];
				VRef02 = dfaces[FaceIndex0 + 2];
				VRef10 = dfaces[FaceIndex1 + 0];
				VRef11 = dfaces[FaceIndex1 + 1];
				VRef12 = dfaces[FaceIndex1 + 2];
				}
			else if(wfaces)
				{
				VRef00 = wfaces[FaceIndex0 + 0];
				VRef01 = wfaces[FaceIndex0 + 1];
				VRef02 = wfaces[FaceIndex0 + 2];
				VRef10 = wfaces[FaceIndex1 + 0];
				VRef11 = wfaces[FaceIndex1 + 1];
				VRef12 = wfaces[FaceIndex1 + 2];
				}

			// 0x0005181b..0x00051868: the first face's vertex off the edge, or
			// 0xffffffff when the edge is not one of its sides.
			const NxU32 Ref0 = Edges->Ref0;
			const NxU32 Ref1 = Edges->Ref1;
			NxU32 Op;
			if(VRef00 == Ref0 && VRef01 == Ref1)		Op = VRef02;
			else if(VRef00 == Ref1 && VRef01 == Ref0)	Op = VRef02;
			else if(VRef00 == Ref0 && VRef02 == Ref1)	Op = VRef01;
			else if(VRef00 == Ref1 && VRef02 == Ref0)	Op = VRef01;
			else if(VRef01 == Ref0 && VRef02 == Ref1)	Op = VRef00;
			else if(VRef01 == Ref1 && VRef02 == Ref0)	Op = VRef00;
			else										Op = 0xffffffff;

			// Plane(verts[VRef10], verts[VRef11], verts[VRef12]) at [esp+0x60]:
			// the inline constructor calling Plane::Set (005155, 0x0005189a). A
			// plain block, so that no object with a declared destructor is
			// constructed here.
			float PL[4];
			((IceMaths::Plane*) PL)->Set(verts[VRef10], verts[VRef11], verts[VRef12]);

			// 0x0005189f..0x000518d4: the distance of verts[Op] to that plane,
			// (n.y p.y + n.z p.z) + n.x p.x + d, compared below zero.
			const float* opposite = &verts[Op].x;
			bool below;
#if NX_PHYSICS_USE_X87
			__asm
				{
				lea		ecx, PL
				mov		eax, opposite
				fld		dword ptr [ecx + 4]
				fmul	dword ptr [eax + 4]
				fld		dword ptr [ecx + 8]
				fmul	dword ptr [eax + 8]
				faddp	st(1), st
				fld		dword ptr [ecx]
				fmul	dword ptr [eax]
				faddp	st(1), st
				fadd	dword ptr [ecx + 0xc]
				fcomp	gEdgeListZero
				fnstsw	ax
				test	ah, 5
				setnp	below
				}
#else
			const double distance=((double(PL[1])*opposite[1]+double(PL[2])*opposite[2])+double(PL[0])*opposite[0])+PL[3];
			below=distance<double(gEdgeListZero);
#endif

			if(below)
				{
				// The two triangles, copied to the stack (0x000518da..0x000519a4),
				// and their normals through Triangle::Normal (005181): the first
				// into [esp+0x18], the second into [esp+0x3c].
				float T0[9], T1[9];
				memcpy(&T0[0], &verts[VRef00], 12);
				memcpy(&T0[3], &verts[VRef01], 12);
				memcpy(&T0[6], &verts[VRef02], 12);
				memcpy(&T1[0], &verts[VRef10], 12);
				memcpy(&T1[3], &verts[VRef11], 12);
				memcpy(&T1[6], &verts[VRef12], 12);
				float N0[3], N1[3];
				((const IceMaths::Triangle*) T0)->Normal(*(IceMaths::Point*) N0);
				((const IceMaths::Triangle*) T1)->Normal(*(IceMaths::Point*) N1);

				// 0x000519c1..0x00051a3a: atan2(|N0 x N1|, N0 . N1), the cross
				// product's length ((x^2 + z^2) + y^2) square-rooted and kept on
				// the stack, the dot product ((x + z) + y), then |angle| > 0.1f.
				bool wide;
#if NX_PHYSICS_USE_X87
				__asm
					{
					lea		ecx, N0
					lea		edx, N1
					fld		dword ptr [edx + 8]
					fmul	dword ptr [ecx + 4]
					fld		dword ptr [ecx + 8]
					fmul	dword ptr [edx + 4]
					fsubp	st(1), st
					fld		dword ptr [ecx + 8]
					fmul	dword ptr [edx]
					fld		dword ptr [edx + 8]
					fmul	dword ptr [ecx]
					fsubp	st(1), st
					fld		dword ptr [edx + 4]
					fmul	dword ptr [ecx]
					fld		dword ptr [ecx + 4]
					fmul	dword ptr [edx]
					fsubp	st(1), st
					fld		st(2)
					fmulp	st(3), st
					fld		st(0)
					fmul	st, st(1)
					faddp	st(3), st
					fld		st(1)
					fmul	st, st(2)
					faddp	st(3), st
					fxch	st(2)
					fsqrt
					fstp	st(2)
					fstp	st(0)
					fld		dword ptr [edx]
					fmul	dword ptr [ecx]
					fld		dword ptr [edx + 8]
					fmul	dword ptr [ecx + 8]
					faddp	st(1), st
					fld		dword ptr [edx + 4]
					fmul	dword ptr [ecx + 4]
					faddp	st(1), st
					fpatan
					fabs
					fcomp	gEdgeListActiveAngle
					fnstsw	ax
					test	ah, 0x41
					sete	wide
					}
#else
				const double crossX=double(N1[2])*N0[1]-double(N0[2])*N1[1];
				const double crossY=double(N0[2])*N1[0]-double(N1[2])*N0[0];
				const double crossZ=double(N1[1])*N0[0]-double(N0[1])*N1[0];
				const double length=std::sqrt((crossX*crossX+crossZ*crossZ)+crossY*crossY);
				const double dot=(double(N1[0])*N0[0]+double(N1[2])*N0[2])+double(N1[1])*N0[1];
				wide=std::fabs(std::atan2(length,dot))>double(gEdgeListActiveAngle);
#endif
				if(wide)
					Active = true;
				}
			}

		*CurrentMark++ = Active;
		ED++;
		Edges++;
		}
	while(--NbEdges);

	// 0x00051a81..0x00051afb: into the links, bit 31, unless already set.
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		EdgeTriangle& ET = mEdgeFaces[i];
		for(NxU32 j = 0; j < 3; j++)
			{
			const NxU32 Link = ET.mLink[j];
			if(!(Link & 0x80000000) && ActiveEdges[Link & 0x0fffffff])
				ET.mLink[j] = Link | 0x80000000;
			}
		}

	// 0x00051afd..0x00051b1c: into the descriptors, bit 0 of the flags.
	for(NxU32 i = 0; i < mNbEdges; i++)
		{
		if(ActiveEdges[i])
			mEdgeToTriangles[i].Flags |= 1;
		}

	nxIceFree(ActiveEdges);

	// 0x00051b2b..0x00051bac: the largest reference over nb_faces faces.
	NxU32 MaxIndex = 0;
	for(NxU32 i = 0; i < nb_faces; i++)
		{
		NxU32 v0, v1, v2;
		if(dfaces)
			{
			v0 = dfaces[i * 3 + 0];
			v1 = dfaces[i * 3 + 1];
			v2 = dfaces[i * 3 + 2];
			}
		else
			{
			v0 = wfaces[i * 3 + 0];
			v1 = wfaces[i * 3 + 1];
			v2 = wfaces[i * 3 + 2];
			}
		if(v0 > MaxIndex)	MaxIndex = v0;
		if(v1 > MaxIndex)	MaxIndex = v1;
		if(v2 > MaxIndex)	MaxIndex = v2;
		}
	const NxU32 NbVerts = MaxIndex + 1;

	// One byte per vertex, temporary, zeroed (0x00051bb3..0x00051be8).
	bool* ActiveVerts = (bool*) nxIceAlloc(NbVerts, NX_MEMORY_TEMP);
	if(!ActiveVerts)
		return false;
	memset(ActiveVerts, 0, NbVerts);

	// 0x00051bea..0x00051cb4: both ends of every active edge.
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		NxU32 v0, v1, v2;
		if(dfaces)
			{
			v0 = dfaces[i * 3 + 0];
			v1 = dfaces[i * 3 + 1];
			v2 = dfaces[i * 3 + 2];
			}
		else
			{
			v0 = wfaces[i * 3 + 0];
			v1 = wfaces[i * 3 + 1];
			v2 = wfaces[i * 3 + 2];
			}
		const EdgeTriangle& ET = mEdgeFaces[i];
		if(ET.mLink[0] & 0x80000000)	{ ActiveVerts[v1] = true; ActiveVerts[v0] = true; }
		if(ET.mLink[1] & 0x80000000)	{ ActiveVerts[v2] = true; ActiveVerts[v1] = true; }
		if(ET.mLink[2] & 0x80000000)	{ ActiveVerts[v2] = true; ActiveVerts[v0] = true; }
		}

	// 0x00051cc0..0x00051dab: link j gets bit 30 when vertex j is active.
	for(NxU32 i = 0; i < mNbFaces; i++)
		{
		NxU32 v[3];
		if(dfaces)
			{
			v[0] = dfaces[i * 3 + 0];
			v[1] = dfaces[i * 3 + 1];
			v[2] = dfaces[i * 3 + 2];
			}
		else
			{
			v[0] = wfaces[i * 3 + 0];
			v[1] = wfaces[i * 3 + 1];
			v[2] = wfaces[i * 3 + 2];
			}
		EdgeTriangle& ET = mEdgeFaces[i];
		for(NxU32 j = 0; j < 3; j++)
			{
			const NxU32 Link = ET.mLink[j];
			if(!(Link & 0x40000000) && ActiveVerts[v[j]])
				ET.mLink[j] = Link | 0x40000000;
			}
		}

	nxIceFree(ActiveVerts);
	return true;
	}

// phys_fn_002063 (0x00051dd0, 225 B)
// With vertices both tables are built and the active edges computed, whatever
// the two flags say (0x00051de0, 0x00051e46); afterwards the flags alone decide
// what is kept: without FacesToEdges the links are released, without
// VerticesToEdges the descriptors and the faces-by-edges array.
bool EdgeList::Init(const EDGELISTCREATE& create)
	{
	bool FacesToEdges = true;
	bool VerticesToEdges = true;
	if(!create.Verts)
		{
		FacesToEdges = create.FacesToEdges;
		VerticesToEdges = create.VerticesToEdges;
		}

	if(FacesToEdges)
		{
		if(!CreateFacesToEdges(create.NbFaces, create.DFaces, create.WFaces))
			return false;
		}
	if(VerticesToEdges)
		{
		if(!CreateEdgesToFaces(create.NbFaces, create.DFaces, create.WFaces))
			return false;
		}
	if(create.Verts)
		{
		if(!ComputeActiveEdges(create.NbFaces, create.DFaces, create.WFaces, create.Verts, create.Epsilon))
			return false;
		}

	if(!create.FacesToEdges)
		{
		if(mEdgeFaces)
			{
			nxIceDeleteArray(mEdgeFaces);
			mEdgeFaces = 0;
			}
		}
	if(!create.VerticesToEdges)
		{
		if(mEdgeToTriangles)
			{
			nxIceFree(mEdgeToTriangles);
			mEdgeToTriangles = 0;
			}
		if(mFacesByEdges)
			{
			nxIceFree(mFacesByEdges);
			mFacesByEdges = 0;
			}
		}
	return true;
	}
