/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// IceMeshTools.cpp, sub-unit D of units/convex-mesh-gap-contract.md (a file
// name chosen by the contract; the rows carry no string and have no .rdata
// block). Convex-mesh gap Task 2c writes the valencies from the Capstone
// listing, 0x00032590..0x0003280f: ICE's Valencies shape, not vendored, built
// on EdgeList (Physics/src/EdgeList.cpp). Every allocation goes through the
// 004803 getter (nxIceAlloc, EdgeList.h), persistent, without a cookie.
//
// x87: on the /arch:IA32 list, for the float rows sub-unit D will add; the
// valencies and the vertex reduction are integer code.
//
// Task 2d adds the vertex reduction (0x00031680..0x0003188c and 0x000324a0),
// which MeshBuilder2 (IceMeshBuilder2.cpp) calls: ICE's reduced-vertex shape,
// vertices radix-sorted on the bits of x, then y, then z (RadixSort 005157,
// 005163 with the unsigned hint, 005159), kept once per run of equal bits.
//
// Task 2e adds the rest of sub-unit D: the function-static identity pair
// (001639), the edge-list outline (001641/001643), the mesh normals (001651, and
// the product form of their release, 001649), the relative poses of two frames
// (001653) and the unique-axis Container (001661). The float rows 001651,
// 001653 and 001661 are the listing's instructions, naked (the precedent of
// 001712 in Geometry.cpp; 002144 in SmoothNormals.cpp is written the same
// way): their frames, the operands each one loads or uses from memory, and the
// stores that narrow are the oracle's. Branch targets are labels named by their
// oracle RVA, and the two alignment `lea`s the listing carries are emitted as
// its bytes. Calls go to the candidate's rows of the same stable IDs: 002144,
// the 004803 getter (nxGetSdkAllocator; slot 0 malloc, slot 3 free), the
// vendored IceMaths::InvertPRMatrix (005191) and 001591 (IceMeshBuilder2.cpp).

#include "IceMeshBuilder2.h"

#include <string.h>

// The three constants the float rows read: 0.0f (0x101041f0), 1.0f
// (0x101041ec) and 0.9999f (0x101078cc, the bits 0x3f7ff972).
static const float kIceMeshToolsZero = 0.0f;
static const float kIceMeshToolsOne = 1.0f;
static const float kIceMeshToolsAxisLimit = 0.9999f;

// phys_fn_001639 (0x000313e0, 157 B)
// A function-static pair of identities, built once: the guard byte 0x10123c78
// tested, then the 4x4 at 0x10123ca0 zeroed (rep stosd of 16 dwords) and the
// nine dwords 0x10123c7c..0x10123c9c, the guard set, and 1.0f stored on the
// diagonals (0x10123cdc, cc8, cb4, ca0, c9c, c8c, c7c, in that order). The block
// returned is 0x10123c7c: a 3x3 identity, then the 4x4 identity at +0x24. There
// is no destructor to register (no _atexit) and no second guard bit.
static bool gIceIdentityPosesBuilt = false;	// 0x10123c78
static NxF32 gIceIdentityPoses[25];			// 0x10123c7c

__declspec(noinline) const NxF32* nxIceIdentityPoses()
	{
	if(!gIceIdentityPosesBuilt)
		{
		NxF32* p = gIceIdentityPoses;
		for(int i = 9; i < 25; i++)
			p[i] = 0.0f;
		for(int i = 0; i < 9; i++)
			p[i] = 0.0f;
		gIceIdentityPosesBuilt = true;
		p[24] = 1.0f;
		p[19] = 1.0f;
		p[14] = 1.0f;
		p[9] = 1.0f;
		p[8] = 1.0f;
		p[4] = 1.0f;
		p[0] = 1.0f;
		}
	return gIceIdentityPoses;
	}

// phys_fn_001641 (0x00031480, 61 B)
// phys_fn_001643 (0x000314c0, 433 B)
// The closed outline of a set of edges (pairs of vertex references). On a copy
// of the edges (the Container copy constructor, 004844), every pair that occurs
// twice, in either orientation, is removed, both copies, and the search starts
// again from the first pair (0x00031495..0x0003152e). Each removal is
// Container::DeleteIndex (the last entry moved into the hole): pair j's second
// and first, then pair i's second and first. Then the first remaining pair
// seeds the outline (both references added, the pair removed the same way),
// and the pair that continues it from its last reference is found, added and
// removed, until none is left (true) or none continues it (false). The copy is
// released on both exits (~Container, 004846).
//
// Listing quirks, reproduced: the first pair is read without testing the count
// (0x00031537), so a copy left empty reads the stale words of its array and
// counts down from zero; an edge present three times keeps one copy.
__declspec(noinline) bool nxIceEdgeLoop(IceCore::Container& loop, const IceCore::Container& edges)
	{
	IceCore::Container Copy(edges);

	for(;;)
		{
		const NxU32 NbPairs = Copy.GetNbEntries() >> 1;
		const NxU32* Pairs = Copy.GetEntries();
		NxU32 i = 0;
		NxU32 j = 0;
		bool Found = false;
		while(i < NbPairs)
			{
			const NxU32 Ref0 = Pairs[i * 2 + 0];
			const NxU32 Ref1 = Pairs[i * 2 + 1];
			for(j = i + 1; j < NbPairs; j++)
				{
				if(Pairs[j * 2 + 0] == Ref0 && Pairs[j * 2 + 1] == Ref1)
					break;
				if(Pairs[j * 2 + 1] == Ref0 && Pairs[j * 2 + 0] == Ref1)
					break;
				}
			if(j < NbPairs)
				{
				Found = true;
				break;
				}
			i++;
			}
		if(!Found)
			break;
		Copy.DeleteIndex(j * 2 + 1);
		Copy.DeleteIndex(j * 2 + 0);
		Copy.DeleteIndex(i * 2 + 1);
		Copy.DeleteIndex(i * 2 + 0);
		}

	const NxU32* Pairs = Copy.GetEntries();
	const NxU32 First = Pairs[0];
	NxU32 Current = Pairs[1];
	loop.Add(First);
	loop.Add(Current);
	Copy.DeleteIndex(1);
	Copy.DeleteIndex(0);

	while(Copy.GetNbEntries() >> 1)
		{
		const NxU32 NbPairs = Copy.GetNbEntries() >> 1;
		const NxU32* Remaining = Copy.GetEntries();
		NxU32 j = 0;
		for(;;)
			{
			if(j >= NbPairs)
				return false;
			const NxU32 Ref0 = Remaining[j * 2 + 0];
			const NxU32 Ref1 = Remaining[j * 2 + 1];
			if(Ref0 == Current)
				{
				loop.Add(Ref1);
				Current = Ref1;
				break;
				}
			if(Ref1 == Current)
				{
				loop.Add(Ref0);
				Current = Ref0;
				break;
				}
			j++;
			}
		Copy.DeleteIndex(j * 2 + 1);
		Copy.DeleteIndex(j * 2 + 0);
		}
	return true;
	}

// phys_fn_001645 (0x00031680, 29 B)
// +0x04 = verts, +0x00 = nb_verts (the arguments in that order, `ret 8`), and
// +0x08, +0x0c, +0x10 zeroed.
ReducedVertices::ReducedVertices(const IceMaths::Point* verts, NxU32 nb_verts)
	{
	mNbRVerts = 0;
	mRVerts = 0;
	mXRef = 0;
	mVerts = verts;
	mNbVerts = nb_verts;
	}

// phys_fn_001659 (0x000324a0, 65 B)
// Releases +0x10, then +0x0c, each through the 004803 getter's slot 3 when
// non-null, each cleared.
ReducedVertices::~ReducedVertices()
	{
	if(mXRef)
		{
		nxIceFree(mXRef);
		mXRef = 0;
		}
	if(mRVerts)
		{
		nxIceFree(mRVerts);
		mRVerts = 0;
		}
	}

// phys_fn_001647 (0x000316a0, 492 B)
// ReducedVertices::Reduce. The two previous outputs are released first, as
// the destructor releases them (0x000316a7..0x000316d8). The cross-reference is
// persistent (type 0) and checked; the key buffer temporary (type 1), checked,
// and released after the third sort (0x000317b9); the reduced array
// persistent and NOT checked (0x000317dc..0x000317f1). A vertex is kept when
// any of its three words differs from the previous sorted vertex's -- the
// first is compared with three 0xffffffff words on the stack (0x000317c6), so
// a first vertex of those bits is not kept and its cross-reference is
// 0xffffffff. The comparison is on bits (`cmp` of dwords), not on values.
bool ReducedVertices::Reduce(REDUCEDCLOUD* rc)
	{
	if(mXRef)
		{
		nxIceFree(mXRef);
		mXRef = 0;
		}
	if(mRVerts)
		{
		nxIceFree(mRVerts);
		mRVerts = 0;
		}

	mXRef = (NxU32*) nxIceAlloc(mNbVerts * 4, NX_MEMORY_PERSISTENT);
	if(!mXRef)
		return false;

	NxU32* Keys = (NxU32*) nxIceAlloc(mNbVerts * 4, NX_MEMORY_TEMP);
	if(!Keys)
		return false;

	const NxU32* Words = (const NxU32*) mVerts;
	for(NxU32 i = 0; i < mNbVerts; i++)
		Keys[i] = Words[i * 3 + 0];
	IceCore::RadixSort Radix;
	Radix.Sort(Keys, mNbVerts, IceCore::RADIX_UNSIGNED);
	for(NxU32 i = 0; i < mNbVerts; i++)
		Keys[i] = Words[i * 3 + 1];
	Radix.Sort(Keys, mNbVerts, IceCore::RADIX_UNSIGNED);
	for(NxU32 i = 0; i < mNbVerts; i++)
		Keys[i] = Words[i * 3 + 2];
	const NxU32* Sorted = Radix.Sort(Keys, mNbVerts, IceCore::RADIX_UNSIGNED).GetRanks();
	nxIceFree(Keys);

	mNbRVerts = 0;
	const NxU32 Junk[3] = { 0xffffffff, 0xffffffff, 0xffffffff };
	const NxU32* Previous = Junk;
	mRVerts = (IceMaths::Point*) nxIceAlloc(mNbVerts * 12, NX_MEMORY_PERSISTENT);
	NxU32* Reduced = (NxU32*) mRVerts;
	for(NxU32 NbToGo = mNbVerts; NbToGo; NbToGo--)
		{
		const NxU32 Index = *Sorted++;
		const NxU32* Current = Words + Index * 3;
		if(Current[0] != Previous[0] || Current[1] != Previous[1] || Current[2] != Previous[2])
			{
			Reduced[mNbRVerts * 3 + 0] = Current[0];
			Reduced[mNbRVerts * 3 + 1] = Current[1];
			Reduced[mNbRVerts * 3 + 2] = Current[2];
			mNbRVerts++;
			}
		Previous = Current;
		mXRef[Index] = mNbRVerts - 1;
		}

	if(rc)
		{
		rc->XRef = mXRef;
		rc->NbRVerts = mNbRVerts;
		rc->RVerts = mRVerts;
		}
	return true;
	}


// phys_fn_001649 (0x00031890, 61 B)
// The release of MeshNormals: +0x00, then +0x04, each through the 004803
// getter's slot 3 when non-null, each cleared. (The ObjectModel.cpp model of
// this row stands; this is its product form.)
MeshNormals::~MeshNormals()
	{
	if(mFaceNormals)
		{
		nxIceFree(mFaceNormals);
		mFaceNormals = 0;
		}
	if(mVertexNormals)
		{
		nxIceFree(mVertexNormals);
		mVertexNormals = 0;
		}
	}

// phys_fn_001651 (0x000318d0, 1240 B)
// MeshNormals::Compute. False without vertices. The face-normal and
// vertex-normal arrays are the create block's or are allocated through the
// 004803 getter's slot 0 (type 0, nb * 12 bytes, not zeroed), each checked; a
// failed vertex-normal allocation returns false with the face normals it
// allocated neither stored nor released. Each array is stored into the object
// only when it was allocated. Then:
//   * per face, its references (32-bit, else 16-bit, else 0, 1, 2) and the face
//     normal (p0 - p2) x (p1 - p2), each component a difference of two products
//     stored as a float, then normalised by 1 / sqrt((x*x + y*y) + z*z) of the
//     stored floats, skipped when the sum compares equal to 0.0f (fucompp,
//     `test ah, 0x44`, jnp: a NaN sum is normalised);
//   * the vertex normals zeroed (rep stosd / rep stosb of nb * 12 bytes);
//   * per face, the face normal added to its three vertices' normals. With the
//     byte at +0x14 each is weighted by the corner angle (phys_fn_002144: eax the
//     vertex, edx the references stored as {r0, r2, r1}, esi the vertices), the
//     corners taken in the order r0, r2, r1, the x product added from its
//     register and y and z narrowed first; without it, the plain sums;
//   * every vertex normal normalised as the face normals were.
// True. The listing's instructions, naked (see the top of the file).
__declspec(naked) bool __fastcall nxMeshNormalsCompute(MeshNormals* /*normals*/, NxU32 /*edx*/,
	const MESHNORMALSCREATE* /*create*/)
	{
	__asm
		{
		sub	esp, 0x40		// 0x000318d0
		push	ebp		// 0x000318d3
		mov	ebp, dword ptr [esp + 0x48]		// 0x000318d4
		mov	eax, dword ptr [ebp + 4]		// 0x000318d8
		test	eax, eax		// 0x000318db
		push	edi		// 0x000318dd
		mov	edi, ecx		// 0x000318de
		jne	L318ec		// 0x000318e0
		pop	edi		// 0x000318e2
		xor	al, al		// 0x000318e3
		pop	ebp		// 0x000318e5
		add	esp, 0x40		// 0x000318e6
		ret	4		// 0x000318e9
L318ec:
		mov	eax, dword ptr [ebp + 0x18]		// 0x000318ec
		test	eax, eax		// 0x000318ef
		push	esi		// 0x000318f1
		je	L318fc		// 0x000318f2
		mov	esi, eax		// 0x000318f4
		mov	dword ptr [esp + 0xc], esi		// 0x000318f6
		jmp	L31919		// 0x000318fa
L318fc:
		call	nxGetSdkAllocator		// 0x000318fc
		mov	ecx, dword ptr [ebp + 8]		// 0x00031901
		mov	edx, dword ptr [eax]		// 0x00031904
		lea	ecx, [ecx + ecx*2]		// 0x00031906
		shl	ecx, 2		// 0x00031909
		push	0		// 0x0003190c
		push	ecx		// 0x0003190e
		mov	ecx, eax		// 0x0003190f
		call	dword ptr [edx]		// 0x00031911
		mov	dword ptr [esp + 0xc], eax		// 0x00031913
		mov	esi, eax		// 0x00031917
L31919:
		test	esi, esi		// 0x00031919
		jne	L31928		// 0x0003191b
		pop	esi		// 0x0003191d
		pop	edi		// 0x0003191e
		xor	al, al		// 0x0003191f
		pop	ebp		// 0x00031921
		add	esp, 0x40		// 0x00031922
		ret	4		// 0x00031925
L31928:
		mov	eax, dword ptr [ebp + 0x1c]		// 0x00031928
		test	eax, eax		// 0x0003192b
		push	ebx		// 0x0003192d
		je	L31938		// 0x0003192e
		mov	ebx, eax		// 0x00031930
		mov	dword ptr [esp + 0x54], ebx		// 0x00031932
		jmp	L31955		// 0x00031936
L31938:
		call	nxGetSdkAllocator		// 0x00031938
		mov	ecx, dword ptr [ebp]		// 0x0003193d
		mov	edx, dword ptr [eax]		// 0x00031940
		lea	ecx, [ecx + ecx*2]		// 0x00031942
		shl	ecx, 2		// 0x00031945
		push	0		// 0x00031948
		push	ecx		// 0x0003194a
		mov	ecx, eax		// 0x0003194b
		call	dword ptr [edx]		// 0x0003194d
		mov	dword ptr [esp + 0x54], eax		// 0x0003194f
		mov	ebx, eax		// 0x00031953
L31955:
		test	ebx, ebx		// 0x00031955
		jne	L31965		// 0x00031957
		pop	ebx		// 0x00031959
		pop	esi		// 0x0003195a
		pop	edi		// 0x0003195b
		xor	al, al		// 0x0003195c
		pop	ebp		// 0x0003195e
		add	esp, 0x40		// 0x0003195f
		ret	4		// 0x00031962
L31965:
		mov	eax, dword ptr [ebp + 0x18]		// 0x00031965
		test	eax, eax		// 0x00031968
		jne	L3196e		// 0x0003196a
		mov	dword ptr [edi], esi		// 0x0003196c
L3196e:
		mov	eax, dword ptr [ebp + 0x1c]		// 0x0003196e
		test	eax, eax		// 0x00031971
		jne	L31978		// 0x00031973
		mov	dword ptr [edi + 4], ebx		// 0x00031975
L31978:
		mov	eax, dword ptr [ebp + 8]		// 0x00031978
		test	eax, eax		// 0x0003197b
		mov	edi, dword ptr [ebp + 4]		// 0x0003197d
		mov	dword ptr [esp + 0x18], 0		// 0x00031980
		jbe	L31af8		// 0x00031988
		mov	eax, dword ptr [esp + 0x10]		// 0x0003198e
		lea	ecx, [esi + 8]		// 0x00031992
		xor	ebx, ebx		// 0x00031995
		mov	esi, 0xfffffff8		// 0x00031997
		sub	esi, eax		// 0x0003199c
		mov	dword ptr [esp + 0x1c], ebx		// 0x0003199e
		mov	dword ptr [esp + 0x14], esi		// 0x000319a2
		jmp	L319b0		// 0x000319a6
L319a8:
		mov	esi, dword ptr [esp + 0x14]		// 0x000319a8
		_emit	0x8d
		_emit	0x64
		_emit	0x24
		_emit	0x00		// 0x000319ac lea esp, [esp]
L319b0:
		mov	eax, dword ptr [ebp + 0xc]		// 0x000319b0
		test	eax, eax		// 0x000319b3
		je	L319bf		// 0x000319b5
		lea	edx, [eax + esi]		// 0x000319b7
		mov	edx, dword ptr [edx + ecx]		// 0x000319ba
		jmp	L319ce		// 0x000319bd
L319bf:
		mov	edx, dword ptr [ebp + 0x10]		// 0x000319bf
		test	edx, edx		// 0x000319c2
		je	L319cc		// 0x000319c4
		movzx	edx, word ptr [edx + ebx]		// 0x000319c6
		jmp	L319ce		// 0x000319ca
L319cc:
		xor	edx, edx		// 0x000319cc
L319ce:
		test	eax, eax		// 0x000319ce
		je	L319da		// 0x000319d0
		add	esi, eax		// 0x000319d2
		mov	esi, dword ptr [esi + ecx + 4]		// 0x000319d4
		jmp	L319ed		// 0x000319d8
L319da:
		mov	esi, dword ptr [ebp + 0x10]		// 0x000319da
		test	esi, esi		// 0x000319dd
		je	L319e8		// 0x000319df
		movzx	esi, word ptr [esi + ebx + 2]		// 0x000319e1
		jmp	L319ed		// 0x000319e6
L319e8:
		mov	esi, 1		// 0x000319e8
L319ed:
		test	eax, eax		// 0x000319ed
		je	L31a01		// 0x000319ef
		mov	ebx, dword ptr [esp + 0x14]		// 0x000319f1
		add	eax, ebx		// 0x000319f5
		mov	eax, dword ptr [eax + ecx + 8]		// 0x000319f7
		mov	ebx, dword ptr [esp + 0x1c]		// 0x000319fb
		jmp	L31a14		// 0x000319ff
L31a01:
		mov	eax, dword ptr [ebp + 0x10]		// 0x00031a01
		test	eax, eax		// 0x00031a04
		je	L31a0f		// 0x00031a06
		movzx	eax, word ptr [eax + ebx + 4]		// 0x00031a08
		jmp	L31a14		// 0x00031a0d
L31a0f:
		mov	eax, 2		// 0x00031a0f
L31a14:
		lea	edx, [edx + edx*2]		// 0x00031a14
		fld	dword ptr [edi + edx*4 + 8]		// 0x00031a17
		lea	edx, [edi + edx*4]		// 0x00031a1b
		lea	eax, [eax + eax*2]		// 0x00031a1e
		fsub	dword ptr [edi + eax*4 + 8]		// 0x00031a21
		lea	eax, [edi + eax*4]		// 0x00031a25
		lea	esi, [esi + esi*2]		// 0x00031a28
		fld	dword ptr [edi + esi*4 + 4]		// 0x00031a2b
		lea	esi, [edi + esi*4]		// 0x00031a2f
		fsub	dword ptr [eax + 4]		// 0x00031a32
		fmulp	st(1), st		// 0x00031a35
		fld	dword ptr [esi + 8]		// 0x00031a37
		fsub	dword ptr [eax + 8]		// 0x00031a3a
		fld	dword ptr [edx + 4]		// 0x00031a3d
		fsub	dword ptr [eax + 4]		// 0x00031a40
		fmulp	st(1), st		// 0x00031a43
		fsubp	st(1), st		// 0x00031a45
		fstp	dword ptr [ecx - 8]		// 0x00031a47
		fld	dword ptr [esi + 8]		// 0x00031a4a
		fsub	dword ptr [eax + 8]		// 0x00031a4d
		fld	dword ptr [edx]		// 0x00031a50
		fsub	dword ptr [eax]		// 0x00031a52
		fmulp	st(1), st		// 0x00031a54
		fld	dword ptr [esi]		// 0x00031a56
		fsub	dword ptr [eax]		// 0x00031a58
		fld	dword ptr [edx + 8]		// 0x00031a5a
		fsub	dword ptr [eax + 8]		// 0x00031a5d
		fmulp	st(1), st		// 0x00031a60
		fsubp	st(1), st		// 0x00031a62
		fstp	dword ptr [ecx - 4]		// 0x00031a64
		fld	dword ptr [esi]		// 0x00031a67
		fsub	dword ptr [eax]		// 0x00031a69
		fld	dword ptr [edx + 4]		// 0x00031a6b
		fsub	dword ptr [eax + 4]		// 0x00031a6e
		fmulp	st(1), st		// 0x00031a71
		fld	dword ptr [edx]		// 0x00031a73
		fsub	dword ptr [eax]		// 0x00031a75
		fld	dword ptr [esi + 4]		// 0x00031a77
		fsub	dword ptr [eax + 4]		// 0x00031a7a
		fmulp	st(1), st		// 0x00031a7d
		fsubp	st(1), st		// 0x00031a7f
		fstp	dword ptr [ecx]		// 0x00031a81
		fld	dword ptr [ecx]		// 0x00031a83
		fld	dword ptr [ecx - 4]		// 0x00031a85
		fld	dword ptr [ecx - 8]		// 0x00031a88
		fld	st(0)		// 0x00031a8b
		fmul	st, st(1)		// 0x00031a8d
		fld	st(2)		// 0x00031a8f
		fmul	st, st(3)		// 0x00031a91
		faddp	st(1), st		// 0x00031a93
		fld	st(3)		// 0x00031a95
		fmul	st, st(4)		// 0x00031a97
		faddp	st(1), st		// 0x00031a99
		fstp	st(3)		// 0x00031a9b
		fstp	st(0)		// 0x00031a9d
		fstp	st(0)		// 0x00031a9f
		fld	dword ptr kIceMeshToolsZero		// 0x00031aa1
		fld	st(1)		// 0x00031aa7
		fucompp		// 0x00031aa9
		fnstsw	ax		// 0x00031aab
		test	ah, 0x44		// 0x00031aad
		jnp	L31ad0		// 0x00031ab0
		fsqrt		// 0x00031ab2
		fdivr	dword ptr kIceMeshToolsOne		// 0x00031ab4
		fld	st(0)		// 0x00031aba
		fmul	dword ptr [ecx - 8]		// 0x00031abc
		fstp	dword ptr [ecx - 8]		// 0x00031abf
		fld	st(0)		// 0x00031ac2
		fmul	dword ptr [ecx - 4]		// 0x00031ac4
		fstp	dword ptr [ecx - 4]		// 0x00031ac7
		fmul	dword ptr [ecx]		// 0x00031aca
		fstp	dword ptr [ecx]		// 0x00031acc
		jmp	L31ad2		// 0x00031ace
L31ad0:
		fstp	st(0)		// 0x00031ad0
L31ad2:
		mov	eax, dword ptr [esp + 0x18]		// 0x00031ad2
		mov	edx, dword ptr [ebp + 8]		// 0x00031ad6
		inc	eax		// 0x00031ad9
		add	ebx, 6		// 0x00031ada
		add	ecx, 0xc		// 0x00031add
		cmp	eax, edx		// 0x00031ae0
		mov	dword ptr [esp + 0x18], eax		// 0x00031ae2
		mov	dword ptr [esp + 0x1c], ebx		// 0x00031ae6
		jb	L319a8		// 0x00031aea
		mov	ebx, dword ptr [esp + 0x54]		// 0x00031af0
		mov	esi, dword ptr [esp + 0x10]		// 0x00031af4
L31af8:
		mov	eax, dword ptr [ebp]		// 0x00031af8
		lea	ecx, [eax + eax*2]		// 0x00031afb
		shl	ecx, 2		// 0x00031afe
		mov	edx, ecx		// 0x00031b01
		shr	ecx, 2		// 0x00031b03
		xor	eax, eax		// 0x00031b06
		mov	edi, ebx		// 0x00031b08
		rep stosd		// 0x00031b0a
		mov	ecx, edx		// 0x00031b0c
		and	ecx, 3		// 0x00031b0e
		rep stosb		// 0x00031b11
		mov	ecx, dword ptr [ebp + 8]		// 0x00031b13
		xor	eax, eax		// 0x00031b16
		cmp	ecx, eax		// 0x00031b18
		mov	dword ptr [esp + 0x1c], eax		// 0x00031b1a
		jbe	L31d36		// 0x00031b1e
		mov	ebx, 0xfffffff8		// 0x00031b24
		sub	ebx, esi		// 0x00031b29
		mov	dword ptr [esp + 0x10], eax		// 0x00031b2b
		lea	edi, [esi + 8]		// 0x00031b2f
		mov	dword ptr [esp + 0x14], ebx		// 0x00031b32
		jmp	L31b40		// 0x00031b36
L31b38:
		mov	ebx, dword ptr [esp + 0x14]		// 0x00031b38
		_emit	0x8d
		_emit	0x64
		_emit	0x24
		_emit	0x00		// 0x00031b3c lea esp, [esp]
L31b40:
		mov	ecx, dword ptr [ebp + 0xc]		// 0x00031b40
		test	ecx, ecx		// 0x00031b43
		je	L31b53		// 0x00031b45
		lea	eax, [ecx + edi]		// 0x00031b47
		mov	eax, dword ptr [eax + ebx]		// 0x00031b4a
		mov	dword ptr [esp + 0x20], eax		// 0x00031b4d
		jmp	L31b74		// 0x00031b51
L31b53:
		mov	eax, dword ptr [ebp + 0x10]		// 0x00031b53
		test	eax, eax		// 0x00031b56
		je	L31b68		// 0x00031b58
		mov	edx, dword ptr [esp + 0x10]		// 0x00031b5a
		movzx	eax, word ptr [eax + edx]		// 0x00031b5e
		mov	dword ptr [esp + 0x20], eax		// 0x00031b62
		jmp	L31b70		// 0x00031b66
L31b68:
		mov	dword ptr [esp + 0x20], 0		// 0x00031b68
L31b70:
		mov	eax, dword ptr [esp + 0x20]		// 0x00031b70
L31b74:
		test	ecx, ecx		// 0x00031b74
		je	L31b85		// 0x00031b76
		lea	edx, [ecx + edi]		// 0x00031b78
		mov	edx, dword ptr [edx + ebx + 4]		// 0x00031b7b
		mov	dword ptr [esp + 0x28], edx		// 0x00031b7f
		jmp	L31ba3		// 0x00031b83
L31b85:
		mov	edx, dword ptr [ebp + 0x10]		// 0x00031b85
		test	edx, edx		// 0x00031b88
		je	L31b9b		// 0x00031b8a
		mov	esi, dword ptr [esp + 0x10]		// 0x00031b8c
		movzx	edx, word ptr [edx + esi + 2]		// 0x00031b90
		mov	dword ptr [esp + 0x28], edx		// 0x00031b95
		jmp	L31ba3		// 0x00031b99
L31b9b:
		mov	dword ptr [esp + 0x28], 1		// 0x00031b9b
L31ba3:
		test	ecx, ecx		// 0x00031ba3
		je	L31baf		// 0x00031ba5
		add	ecx, edi		// 0x00031ba7
		mov	ebx, dword ptr [ecx + ebx + 8]		// 0x00031ba9
		jmp	L31bc6		// 0x00031bad
L31baf:
		mov	ecx, dword ptr [ebp + 0x10]		// 0x00031baf
		test	ecx, ecx		// 0x00031bb2
		je	L31bc1		// 0x00031bb4
		mov	edx, dword ptr [esp + 0x10]		// 0x00031bb6
		movzx	ebx, word ptr [ecx + edx + 4]		// 0x00031bba
		jmp	L31bc6		// 0x00031bbf
L31bc1:
		mov	ebx, 2		// 0x00031bc1
L31bc6:
		mov	cl, byte ptr [ebp + 0x14]		// 0x00031bc6
		test	cl, cl		// 0x00031bc9
		mov	dword ptr [esp + 0x24], ebx		// 0x00031bcb
		je	L31cac		// 0x00031bcf
		mov	esi, dword ptr [ebp + 4]		// 0x00031bd5
		lea	edx, [esp + 0x20]		// 0x00031bd8
		call	nxSmoothNormalsAngleAtVertex		// 0x00031bdc
		fld	st(0)		// 0x00031be1
		fmul	dword ptr [edi - 8]		// 0x00031be3
		mov	eax, dword ptr [esp + 0x20]		// 0x00031be6
		mov	ecx, dword ptr [esp + 0x54]		// 0x00031bea
		fld	st(1)		// 0x00031bee
		fmul	dword ptr [edi - 4]		// 0x00031bf0
		lea	eax, [eax + eax*2]		// 0x00031bf3
		lea	eax, [ecx + eax*4]		// 0x00031bf6
		lea	edx, [esp + 0x20]		// 0x00031bf9
		fstp	dword ptr [esp + 0x30]		// 0x00031bfd
		fxch	st(1)		// 0x00031c01
		fmul	dword ptr [edi]		// 0x00031c03
		fstp	dword ptr [esp + 0x34]		// 0x00031c05
		fadd	dword ptr [eax]		// 0x00031c09
		fstp	dword ptr [eax]		// 0x00031c0b
		fld	dword ptr [esp + 0x30]		// 0x00031c0d
		fadd	dword ptr [eax + 4]		// 0x00031c11
		fstp	dword ptr [eax + 4]		// 0x00031c14
		fld	dword ptr [esp + 0x34]		// 0x00031c17
		fadd	dword ptr [eax + 8]		// 0x00031c1b
		fstp	dword ptr [eax + 8]		// 0x00031c1e
		mov	esi, dword ptr [ebp + 4]		// 0x00031c21
		mov	eax, ebx		// 0x00031c24
		call	nxSmoothNormalsAngleAtVertex		// 0x00031c26
		fld	st(0)		// 0x00031c2b
		fmul	dword ptr [edi - 8]		// 0x00031c2d
		mov	eax, dword ptr [esp + 0x54]		// 0x00031c30
		fld	st(1)		// 0x00031c34
		lea	edx, [ebx + ebx*2]		// 0x00031c36
		fmul	dword ptr [edi - 4]		// 0x00031c39
		mov	ebx, dword ptr [esp + 0x28]		// 0x00031c3c
		lea	eax, [eax + edx*4]		// 0x00031c40
		lea	edx, [esp + 0x20]		// 0x00031c43
		fstp	dword ptr [esp + 0x3c]		// 0x00031c47
		fxch	st(1)		// 0x00031c4b
		fmul	dword ptr [edi]		// 0x00031c4d
		fstp	dword ptr [esp + 0x40]		// 0x00031c4f
		fadd	dword ptr [eax]		// 0x00031c53
		fstp	dword ptr [eax]		// 0x00031c55
		fld	dword ptr [esp + 0x3c]		// 0x00031c57
		fadd	dword ptr [eax + 4]		// 0x00031c5b
		fstp	dword ptr [eax + 4]		// 0x00031c5e
		fld	dword ptr [esp + 0x40]		// 0x00031c61
		fadd	dword ptr [eax + 8]		// 0x00031c65
		fstp	dword ptr [eax + 8]		// 0x00031c68
		mov	esi, dword ptr [ebp + 4]		// 0x00031c6b
		mov	eax, ebx		// 0x00031c6e
		call	nxSmoothNormalsAngleAtVertex		// 0x00031c70
		fld	st(0)		// 0x00031c75
		fmul	dword ptr [edi - 8]		// 0x00031c77
		mov	edx, dword ptr [esp + 0x54]		// 0x00031c7a
		fld	st(1)		// 0x00031c7e
		lea	ecx, [ebx + ebx*2]		// 0x00031c80
		fmul	dword ptr [edi - 4]		// 0x00031c83
		lea	eax, [edx + ecx*4]		// 0x00031c86
		fstp	dword ptr [esp + 0x48]		// 0x00031c89
		fxch	st(1)		// 0x00031c8d
		fmul	dword ptr [edi]		// 0x00031c8f
		fstp	dword ptr [esp + 0x4c]		// 0x00031c91
		fadd	dword ptr [eax]		// 0x00031c95
		fstp	dword ptr [eax]		// 0x00031c97
		fld	dword ptr [esp + 0x48]		// 0x00031c99
		fadd	dword ptr [eax + 4]		// 0x00031c9d
		fstp	dword ptr [eax + 4]		// 0x00031ca0
		fld	dword ptr [esp + 0x4c]		// 0x00031ca3
		fadd	dword ptr [eax + 8]		// 0x00031ca7
		jmp	L31d0d		// 0x00031caa
L31cac:
		mov	ecx, dword ptr [esp + 0x54]		// 0x00031cac
		lea	eax, [eax + eax*2]		// 0x00031cb0
		fld	dword ptr [ecx + eax*4]		// 0x00031cb3
		lea	eax, [ecx + eax*4]		// 0x00031cb6
		fadd	dword ptr [edi - 8]		// 0x00031cb9
		lea	edx, [ebx + ebx*2]		// 0x00031cbc
		fstp	dword ptr [eax]		// 0x00031cbf
		fld	dword ptr [edi - 4]		// 0x00031cc1
		fadd	dword ptr [eax + 4]		// 0x00031cc4
		fstp	dword ptr [eax + 4]		// 0x00031cc7
		fld	dword ptr [edi]		// 0x00031cca
		fadd	dword ptr [eax + 8]		// 0x00031ccc
		fstp	dword ptr [eax + 8]		// 0x00031ccf
		lea	eax, [ecx + edx*4]		// 0x00031cd2
		fld	dword ptr [edi - 8]		// 0x00031cd5
		fadd	dword ptr [eax]		// 0x00031cd8
		fstp	dword ptr [eax]		// 0x00031cda
		fld	dword ptr [edi - 4]		// 0x00031cdc
		fadd	dword ptr [eax + 4]		// 0x00031cdf
		fstp	dword ptr [eax + 4]		// 0x00031ce2
		fld	dword ptr [edi]		// 0x00031ce5
		fadd	dword ptr [eax + 8]		// 0x00031ce7
		fstp	dword ptr [eax + 8]		// 0x00031cea
		mov	eax, dword ptr [esp + 0x28]		// 0x00031ced
		lea	eax, [eax + eax*2]		// 0x00031cf1
		fld	dword ptr [ecx + eax*4]		// 0x00031cf4
		lea	eax, [ecx + eax*4]		// 0x00031cf7
		fadd	dword ptr [edi - 8]		// 0x00031cfa
		fstp	dword ptr [eax]		// 0x00031cfd
		fld	dword ptr [edi - 4]		// 0x00031cff
		fadd	dword ptr [eax + 4]		// 0x00031d02
		fstp	dword ptr [eax + 4]		// 0x00031d05
		fld	dword ptr [eax + 8]		// 0x00031d08
		fadd	dword ptr [edi]		// 0x00031d0b
L31d0d:
		mov	esi, dword ptr [esp + 0x10]		// 0x00031d0d
		fstp	dword ptr [eax + 8]		// 0x00031d11
		mov	eax, dword ptr [esp + 0x1c]		// 0x00031d14
		mov	ecx, dword ptr [ebp + 8]		// 0x00031d18
		inc	eax		// 0x00031d1b
		add	esi, 6		// 0x00031d1c
		add	edi, 0xc		// 0x00031d1f
		cmp	eax, ecx		// 0x00031d22
		mov	dword ptr [esp + 0x1c], eax		// 0x00031d24
		mov	dword ptr [esp + 0x10], esi		// 0x00031d28
		jb	L31b38		// 0x00031d2c
		mov	ebx, dword ptr [esp + 0x54]		// 0x00031d32
L31d36:
		mov	eax, dword ptr [ebp]		// 0x00031d36
		xor	edx, edx		// 0x00031d39
		test	eax, eax		// 0x00031d3b
		jbe	L31d9c		// 0x00031d3d
		lea	ecx, [ebx + 8]		// 0x00031d3f
L31d42:
		fld	dword ptr [ecx]		// 0x00031d42
		fld	dword ptr [ecx - 4]		// 0x00031d44
		fld	dword ptr [ecx - 8]		// 0x00031d47
		fld	st(0)		// 0x00031d4a
		fmul	st, st(1)		// 0x00031d4c
		fld	st(2)		// 0x00031d4e
		fmul	st, st(3)		// 0x00031d50
		faddp	st(1), st		// 0x00031d52
		fld	st(3)		// 0x00031d54
		fmul	st, st(4)		// 0x00031d56
		faddp	st(1), st		// 0x00031d58
		fstp	st(3)		// 0x00031d5a
		fstp	st(0)		// 0x00031d5c
		fstp	st(0)		// 0x00031d5e
		fld	dword ptr kIceMeshToolsZero		// 0x00031d60
		fld	st(1)		// 0x00031d66
		fucompp		// 0x00031d68
		fnstsw	ax		// 0x00031d6a
		test	ah, 0x44		// 0x00031d6c
		jnp	L31d8f		// 0x00031d6f
		fsqrt		// 0x00031d71
		fdivr	dword ptr kIceMeshToolsOne		// 0x00031d73
		fld	st(0)		// 0x00031d79
		fmul	dword ptr [ecx - 8]		// 0x00031d7b
		fstp	dword ptr [ecx - 8]		// 0x00031d7e
		fld	st(0)		// 0x00031d81
		fmul	dword ptr [ecx - 4]		// 0x00031d83
		fstp	dword ptr [ecx - 4]		// 0x00031d86
		fmul	dword ptr [ecx]		// 0x00031d89
		fstp	dword ptr [ecx]		// 0x00031d8b
		jmp	L31d91		// 0x00031d8d
L31d8f:
		fstp	st(0)		// 0x00031d8f
L31d91:
		mov	eax, dword ptr [ebp]		// 0x00031d91
		inc	edx		// 0x00031d94
		add	ecx, 0xc		// 0x00031d95
		cmp	edx, eax		// 0x00031d98
		jb	L31d42		// 0x00031d9a
L31d9c:
		pop	ebx		// 0x00031d9c
		pop	esi		// 0x00031d9d
		pop	edi		// 0x00031d9e
		mov	al, 1		// 0x00031d9f
		pop	ebp		// 0x00031da1
		add	esp, 0x40		// 0x00031da2
		ret	4		// 0x00031da5
		}
	}

// phys_fn_001653 (0x00031db0, 1618 B)
// The relative poses of two frames, as 4x4 matrices. M0 = pose0 ? pose0^-1 :
// identity and M1 = pose1 ? pose1^-1 : identity (the vendored
// IceMaths::InvertPRMatrix, 005191, which inverts a rotation-translation
// matrix). Then, when relative0 is given, relative0 = pose0 * M1 (M1 when pose0
// is null), and when relative1 is given, relative1 = pose1 * M0 (M0 when pose1
// is null). Each product element is a four-term sum whose order the listing
// sets per element (the 2003 compiler reassociated them), formed in a register
// and stored as a float into a 16-float local that is then copied out (rep
// movsd). The listing also zeroes one local dword it never reads (0x00031dc2).
// The listing's instructions, naked (see the top of the file).
__declspec(naked) void __cdecl nxIcePosePair(IceMaths::Matrix4x4* /*relative0*/,
	IceMaths::Matrix4x4* /*relative1*/, const IceMaths::Matrix4x4* /*pose0*/,
	const IceMaths::Matrix4x4* /*pose1*/)
	{
	__asm
		{
		sub	esp, 0xc4		// 0x00031db0
		push	ebx		// 0x00031db6
		push	esi		// 0x00031db7
		mov	esi, dword ptr [esp + 0xd8]		// 0x00031db8
		test	esi, esi		// 0x00031dbf
		push	edi		// 0x00031dc1
		mov	dword ptr [esp + 0xcc], 0		// 0x00031dc2
		je	L31ddf		// 0x00031dcd
		lea	eax, [esp + 0x4c]		// 0x00031dcf
		push	esi		// 0x00031dd3
		push	eax		// 0x00031dd4
		call	IceMaths::InvertPRMatrix		// 0x00031dd5
		add	esp, 8		// 0x00031dda
		jmp	L31e0f		// 0x00031ddd
L31ddf:
		mov	ecx, 0x10		// 0x00031ddf
		xor	eax, eax		// 0x00031de4
		lea	edi, [esp + 0x4c]		// 0x00031de6
		rep stosd		// 0x00031dea
		mov	dword ptr [esp + 0x88], 0x3f800000		// 0x00031dec
		mov	dword ptr [esp + 0x74], 0x3f800000		// 0x00031df7
		mov	dword ptr [esp + 0x60], 0x3f800000		// 0x00031dff
		mov	dword ptr [esp + 0x4c], 0x3f800000		// 0x00031e07
L31e0f:
		mov	ebx, dword ptr [esp + 0xe0]		// 0x00031e0f
		test	ebx, ebx		// 0x00031e16
		je	L31e2a		// 0x00031e18
		lea	ecx, [esp + 0xc]		// 0x00031e1a
		push	ebx		// 0x00031e1e
		push	ecx		// 0x00031e1f
		call	IceMaths::InvertPRMatrix		// 0x00031e20
		add	esp, 8		// 0x00031e25
		jmp	L31e57		// 0x00031e28
L31e2a:
		mov	ecx, 0x10		// 0x00031e2a
		xor	eax, eax		// 0x00031e2f
		lea	edi, [esp + 0xc]		// 0x00031e31
		rep stosd		// 0x00031e35
		mov	dword ptr [esp + 0x48], 0x3f800000		// 0x00031e37
		mov	dword ptr [esp + 0x34], 0x3f800000		// 0x00031e3f
		mov	dword ptr [esp + 0x20], 0x3f800000		// 0x00031e47
		mov	dword ptr [esp + 0xc], 0x3f800000		// 0x00031e4f
L31e57:
		mov	edi, dword ptr [esp + 0xd4]		// 0x00031e57
		test	edi, edi		// 0x00031e5e
		je	L3210e		// 0x00031e60
		test	esi, esi		// 0x00031e66
		je	L32103		// 0x00031e68
		fld	dword ptr [esp + 0x1c]		// 0x00031e6e
		fmul	dword ptr [esi + 4]		// 0x00031e72
		fld	dword ptr [esp + 0x2c]		// 0x00031e75
		fmul	dword ptr [esi + 8]		// 0x00031e79
		faddp	st(1), st		// 0x00031e7c
		fld	dword ptr [esp + 0x3c]		// 0x00031e7e
		fmul	dword ptr [esi + 0xc]		// 0x00031e82
		faddp	st(1), st		// 0x00031e85
		fld	dword ptr [esp + 0xc]		// 0x00031e87
		fmul	dword ptr [esi]		// 0x00031e8b
		faddp	st(1), st		// 0x00031e8d
		fstp	dword ptr [esp + 0x8c]		// 0x00031e8f
		fld	dword ptr [esp + 0x10]		// 0x00031e96
		fmul	dword ptr [esi]		// 0x00031e9a
		fld	dword ptr [esp + 0x30]		// 0x00031e9c
		fmul	dword ptr [esi + 8]		// 0x00031ea0
		faddp	st(1), st		// 0x00031ea3
		fld	dword ptr [esp + 0x40]		// 0x00031ea5
		fmul	dword ptr [esi + 0xc]		// 0x00031ea9
		faddp	st(1), st		// 0x00031eac
		fld	dword ptr [esp + 0x20]		// 0x00031eae
		fmul	dword ptr [esi + 4]		// 0x00031eb2
		faddp	st(1), st		// 0x00031eb5
		fstp	dword ptr [esp + 0x90]		// 0x00031eb7
		fld	dword ptr [esp + 0x14]		// 0x00031ebe
		fmul	dword ptr [esi]		// 0x00031ec2
		fld	dword ptr [esp + 0x24]		// 0x00031ec4
		fmul	dword ptr [esi + 4]		// 0x00031ec8
		faddp	st(1), st		// 0x00031ecb
		fld	dword ptr [esp + 0x44]		// 0x00031ecd
		fmul	dword ptr [esi + 0xc]		// 0x00031ed1
		faddp	st(1), st		// 0x00031ed4
		fld	dword ptr [esp + 0x34]		// 0x00031ed6
		fmul	dword ptr [esi + 8]		// 0x00031eda
		faddp	st(1), st		// 0x00031edd
		fstp	dword ptr [esp + 0x94]		// 0x00031edf
		fld	dword ptr [esp + 0x18]		// 0x00031ee6
		fmul	dword ptr [esi]		// 0x00031eea
		fld	dword ptr [esp + 0x28]		// 0x00031eec
		fmul	dword ptr [esi + 4]		// 0x00031ef0
		faddp	st(1), st		// 0x00031ef3
		fld	dword ptr [esp + 0x38]		// 0x00031ef5
		fmul	dword ptr [esi + 8]		// 0x00031ef9
		faddp	st(1), st		// 0x00031efc
		fld	dword ptr [esp + 0x48]		// 0x00031efe
		fmul	dword ptr [esi + 0xc]		// 0x00031f02
		faddp	st(1), st		// 0x00031f05
		fstp	dword ptr [esp + 0x98]		// 0x00031f07
		fld	dword ptr [esp + 0x3c]		// 0x00031f0e
		fmul	dword ptr [esi + 0x1c]		// 0x00031f12
		fld	dword ptr [esp + 0x1c]		// 0x00031f15
		fmul	dword ptr [esi + 0x14]		// 0x00031f19
		faddp	st(1), st		// 0x00031f1c
		fld	dword ptr [esp + 0x2c]		// 0x00031f1e
		fmul	dword ptr [esi + 0x18]		// 0x00031f22
		faddp	st(1), st		// 0x00031f25
		fld	dword ptr [esp + 0xc]		// 0x00031f27
		fmul	dword ptr [esi + 0x10]		// 0x00031f2b
		faddp	st(1), st		// 0x00031f2e
		fstp	dword ptr [esp + 0x9c]		// 0x00031f30
		fld	dword ptr [esp + 0x20]		// 0x00031f37
		fmul	dword ptr [esi + 0x14]		// 0x00031f3b
		fld	dword ptr [esp + 0x40]		// 0x00031f3e
		fmul	dword ptr [esi + 0x1c]		// 0x00031f42
		faddp	st(1), st		// 0x00031f45
		fld	dword ptr [esp + 0x10]		// 0x00031f47
		fmul	dword ptr [esi + 0x10]		// 0x00031f4b
		faddp	st(1), st		// 0x00031f4e
		fld	dword ptr [esp + 0x30]		// 0x00031f50
		fmul	dword ptr [esi + 0x18]		// 0x00031f54
		faddp	st(1), st		// 0x00031f57
		fstp	dword ptr [esp + 0xa0]		// 0x00031f59
		fld	dword ptr [esp + 0x34]		// 0x00031f60
		fmul	dword ptr [esi + 0x18]		// 0x00031f64
		fld	dword ptr [esp + 0x44]		// 0x00031f67
		fmul	dword ptr [esi + 0x1c]		// 0x00031f6b
		faddp	st(1), st		// 0x00031f6e
		fld	dword ptr [esp + 0x14]		// 0x00031f70
		fmul	dword ptr [esi + 0x10]		// 0x00031f74
		faddp	st(1), st		// 0x00031f77
		fld	dword ptr [esp + 0x24]		// 0x00031f79
		fmul	dword ptr [esi + 0x14]		// 0x00031f7d
		faddp	st(1), st		// 0x00031f80
		fstp	dword ptr [esp + 0xa4]		// 0x00031f82
		fld	dword ptr [esp + 0x18]		// 0x00031f89
		fmul	dword ptr [esi + 0x10]		// 0x00031f8d
		fld	dword ptr [esp + 0x28]		// 0x00031f90
		fmul	dword ptr [esi + 0x14]		// 0x00031f94
		faddp	st(1), st		// 0x00031f97
		fld	dword ptr [esp + 0x38]		// 0x00031f99
		fmul	dword ptr [esi + 0x18]		// 0x00031f9d
		faddp	st(1), st		// 0x00031fa0
		fld	dword ptr [esp + 0x48]		// 0x00031fa2
		fmul	dword ptr [esi + 0x1c]		// 0x00031fa6
		faddp	st(1), st		// 0x00031fa9
		fstp	dword ptr [esp + 0xa8]		// 0x00031fab
		fld	dword ptr [esp + 0x3c]		// 0x00031fb2
		fmul	dword ptr [esi + 0x2c]		// 0x00031fb6
		fld	dword ptr [esp + 0x1c]		// 0x00031fb9
		fmul	dword ptr [esi + 0x24]		// 0x00031fbd
		faddp	st(1), st		// 0x00031fc0
		fld	dword ptr [esp + 0x2c]		// 0x00031fc2
		fmul	dword ptr [esi + 0x28]		// 0x00031fc6
		faddp	st(1), st		// 0x00031fc9
		fld	dword ptr [esp + 0xc]		// 0x00031fcb
		fmul	dword ptr [esi + 0x20]		// 0x00031fcf
		faddp	st(1), st		// 0x00031fd2
		fstp	dword ptr [esp + 0xac]		// 0x00031fd4
		fld	dword ptr [esp + 0x20]		// 0x00031fdb
		fmul	dword ptr [esi + 0x24]		// 0x00031fdf
		fld	dword ptr [esp + 0x40]		// 0x00031fe2
		fmul	dword ptr [esi + 0x2c]		// 0x00031fe6
		faddp	st(1), st		// 0x00031fe9
		fld	dword ptr [esp + 0x10]		// 0x00031feb
		fmul	dword ptr [esi + 0x20]		// 0x00031fef
		faddp	st(1), st		// 0x00031ff2
		fld	dword ptr [esp + 0x30]		// 0x00031ff4
		fmul	dword ptr [esi + 0x28]		// 0x00031ff8
		faddp	st(1), st		// 0x00031ffb
		fstp	dword ptr [esp + 0xb0]		// 0x00031ffd
		fld	dword ptr [esp + 0x34]		// 0x00032004
		fmul	dword ptr [esi + 0x28]		// 0x00032008
		fld	dword ptr [esp + 0x44]		// 0x0003200b
		fmul	dword ptr [esi + 0x2c]		// 0x0003200f
		faddp	st(1), st		// 0x00032012
		fld	dword ptr [esp + 0x14]		// 0x00032014
		fmul	dword ptr [esi + 0x20]		// 0x00032018
		faddp	st(1), st		// 0x0003201b
		fld	dword ptr [esp + 0x24]		// 0x0003201d
		fmul	dword ptr [esi + 0x24]		// 0x00032021
		faddp	st(1), st		// 0x00032024
		fstp	dword ptr [esp + 0xb4]		// 0x00032026
		fld	dword ptr [esp + 0x18]		// 0x0003202d
		fmul	dword ptr [esi + 0x20]		// 0x00032031
		fld	dword ptr [esp + 0x28]		// 0x00032034
		fmul	dword ptr [esi + 0x24]		// 0x00032038
		faddp	st(1), st		// 0x0003203b
		fld	dword ptr [esp + 0x38]		// 0x0003203d
		fmul	dword ptr [esi + 0x28]		// 0x00032041
		faddp	st(1), st		// 0x00032044
		fld	dword ptr [esp + 0x48]		// 0x00032046
		fmul	dword ptr [esi + 0x2c]		// 0x0003204a
		faddp	st(1), st		// 0x0003204d
		fstp	dword ptr [esp + 0xb8]		// 0x0003204f
		fld	dword ptr [esp + 0x3c]		// 0x00032056
		fmul	dword ptr [esi + 0x3c]		// 0x0003205a
		fld	dword ptr [esp + 0x1c]		// 0x0003205d
		fmul	dword ptr [esi + 0x34]		// 0x00032061
		faddp	st(1), st		// 0x00032064
		fld	dword ptr [esp + 0x2c]		// 0x00032066
		fmul	dword ptr [esi + 0x38]		// 0x0003206a
		faddp	st(1), st		// 0x0003206d
		fld	dword ptr [esp + 0xc]		// 0x0003206f
		fmul	dword ptr [esi + 0x30]		// 0x00032073
		faddp	st(1), st		// 0x00032076
		fstp	dword ptr [esp + 0xbc]		// 0x00032078
		fld	dword ptr [esp + 0x20]		// 0x0003207f
		fmul	dword ptr [esi + 0x34]		// 0x00032083
		fld	dword ptr [esp + 0x40]		// 0x00032086
		fmul	dword ptr [esi + 0x3c]		// 0x0003208a
		faddp	st(1), st		// 0x0003208d
		fld	dword ptr [esp + 0x10]		// 0x0003208f
		fmul	dword ptr [esi + 0x30]		// 0x00032093
		faddp	st(1), st		// 0x00032096
		fld	dword ptr [esp + 0x30]		// 0x00032098
		fmul	dword ptr [esi + 0x38]		// 0x0003209c
		faddp	st(1), st		// 0x0003209f
		fstp	dword ptr [esp + 0xc0]		// 0x000320a1
		fld	dword ptr [esp + 0x34]		// 0x000320a8
		fmul	dword ptr [esi + 0x38]		// 0x000320ac
		fld	dword ptr [esp + 0x44]		// 0x000320af
		fmul	dword ptr [esi + 0x3c]		// 0x000320b3
		faddp	st(1), st		// 0x000320b6
		fld	dword ptr [esp + 0x14]		// 0x000320b8
		fmul	dword ptr [esi + 0x30]		// 0x000320bc
		faddp	st(1), st		// 0x000320bf
		fld	dword ptr [esp + 0x24]		// 0x000320c1
		fmul	dword ptr [esi + 0x34]		// 0x000320c5
		faddp	st(1), st		// 0x000320c8
		fstp	dword ptr [esp + 0xc4]		// 0x000320ca
		fld	dword ptr [esp + 0x18]		// 0x000320d1
		fmul	dword ptr [esi + 0x30]		// 0x000320d5
		fld	dword ptr [esp + 0x28]		// 0x000320d8
		fmul	dword ptr [esi + 0x34]		// 0x000320dc
		faddp	st(1), st		// 0x000320df
		fld	dword ptr [esp + 0x38]		// 0x000320e1
		fmul	dword ptr [esi + 0x38]		// 0x000320e5
		faddp	st(1), st		// 0x000320e8
		fld	dword ptr [esp + 0x48]		// 0x000320ea
		fmul	dword ptr [esi + 0x3c]		// 0x000320ee
		lea	esi, [esp + 0x8c]		// 0x000320f1
		faddp	st(1), st		// 0x000320f8
		fstp	dword ptr [esp + 0xc8]		// 0x000320fa
		jmp	L32107		// 0x00032101
L32103:
		lea	esi, [esp + 0xc]		// 0x00032103
L32107:
		mov	ecx, 0x10		// 0x00032107
		rep movsd		// 0x0003210c
L3210e:
		mov	edi, dword ptr [esp + 0xd8]		// 0x0003210e
		test	edi, edi		// 0x00032115
		je	L323f8		// 0x00032117
		test	ebx, ebx		// 0x0003211d
		je	L323ed		// 0x0003211f
		fld	dword ptr [esp + 0x5c]		// 0x00032125
		fmul	dword ptr [ebx + 4]		// 0x00032129
		fld	dword ptr [esp + 0x6c]		// 0x0003212c
		fmul	dword ptr [ebx + 8]		// 0x00032130
		faddp	st(1), st		// 0x00032133
		fld	dword ptr [esp + 0x7c]		// 0x00032135
		fmul	dword ptr [ebx + 0xc]		// 0x00032139
		faddp	st(1), st		// 0x0003213c
		fld	dword ptr [esp + 0x4c]		// 0x0003213e
		fmul	dword ptr [ebx]		// 0x00032142
		faddp	st(1), st		// 0x00032144
		fstp	dword ptr [esp + 0x8c]		// 0x00032146
		fld	dword ptr [esp + 0x70]		// 0x0003214d
		fmul	dword ptr [ebx + 8]		// 0x00032151
		fld	dword ptr [esp + 0x60]		// 0x00032154
		fmul	dword ptr [ebx + 4]		// 0x00032158
		faddp	st(1), st		// 0x0003215b
		fld	dword ptr [esp + 0x50]		// 0x0003215d
		fmul	dword ptr [ebx]		// 0x00032161
		faddp	st(1), st		// 0x00032163
		fld	dword ptr [esp + 0x80]		// 0x00032165
		fmul	dword ptr [ebx + 0xc]		// 0x0003216c
		faddp	st(1), st		// 0x0003216f
		fstp	dword ptr [esp + 0x90]		// 0x00032171
		fld	dword ptr [esp + 0x64]		// 0x00032178
		fmul	dword ptr [ebx + 4]		// 0x0003217c
		fld	dword ptr [esp + 0x54]		// 0x0003217f
		fmul	dword ptr [ebx]		// 0x00032183
		faddp	st(1), st		// 0x00032185
		fld	dword ptr [esp + 0x74]		// 0x00032187
		fmul	dword ptr [ebx + 8]		// 0x0003218b
		faddp	st(1), st		// 0x0003218e
		fld	dword ptr [esp + 0x84]		// 0x00032190
		fmul	dword ptr [ebx + 0xc]		// 0x00032197
		faddp	st(1), st		// 0x0003219a
		fstp	dword ptr [esp + 0x94]		// 0x0003219c
		fld	dword ptr [esp + 0x68]		// 0x000321a3
		fmul	dword ptr [ebx + 4]		// 0x000321a7
		fld	dword ptr [esp + 0x78]		// 0x000321aa
		fmul	dword ptr [ebx + 8]		// 0x000321ae
		faddp	st(1), st		// 0x000321b1
		fld	dword ptr [esp + 0x58]		// 0x000321b3
		fmul	dword ptr [ebx]		// 0x000321b7
		faddp	st(1), st		// 0x000321b9
		fld	dword ptr [esp + 0x88]		// 0x000321bb
		fmul	dword ptr [ebx + 0xc]		// 0x000321c2
		faddp	st(1), st		// 0x000321c5
		fstp	dword ptr [esp + 0x98]		// 0x000321c7
		fld	dword ptr [esp + 0x7c]		// 0x000321ce
		fmul	dword ptr [ebx + 0x1c]		// 0x000321d2
		fld	dword ptr [esp + 0x4c]		// 0x000321d5
		fmul	dword ptr [ebx + 0x10]		// 0x000321d9
		faddp	st(1), st		// 0x000321dc
		fld	dword ptr [esp + 0x5c]		// 0x000321de
		fmul	dword ptr [ebx + 0x14]		// 0x000321e2
		faddp	st(1), st		// 0x000321e5
		fld	dword ptr [esp + 0x6c]		// 0x000321e7
		fmul	dword ptr [ebx + 0x18]		// 0x000321eb
		faddp	st(1), st		// 0x000321ee
		fstp	dword ptr [esp + 0x9c]		// 0x000321f0
		fld	dword ptr [esp + 0x80]		// 0x000321f7
		fmul	dword ptr [ebx + 0x1c]		// 0x000321fe
		fld	dword ptr [esp + 0x70]		// 0x00032201
		fmul	dword ptr [ebx + 0x18]		// 0x00032205
		faddp	st(1), st		// 0x00032208
		fld	dword ptr [esp + 0x50]		// 0x0003220a
		fmul	dword ptr [ebx + 0x10]		// 0x0003220e
		faddp	st(1), st		// 0x00032211
		fld	dword ptr [esp + 0x60]		// 0x00032213
		fmul	dword ptr [ebx + 0x14]		// 0x00032217
		faddp	st(1), st		// 0x0003221a
		fstp	dword ptr [esp + 0xa0]		// 0x0003221c
		fld	dword ptr [esp + 0x84]		// 0x00032223
		fmul	dword ptr [ebx + 0x1c]		// 0x0003222a
		fld	dword ptr [esp + 0x64]		// 0x0003222d
		fmul	dword ptr [ebx + 0x14]		// 0x00032231
		faddp	st(1), st		// 0x00032234
		fld	dword ptr [esp + 0x54]		// 0x00032236
		fmul	dword ptr [ebx + 0x10]		// 0x0003223a
		faddp	st(1), st		// 0x0003223d
		fld	dword ptr [esp + 0x74]		// 0x0003223f
		fmul	dword ptr [ebx + 0x18]		// 0x00032243
		faddp	st(1), st		// 0x00032246
		fstp	dword ptr [esp + 0xa4]		// 0x00032248
		fld	dword ptr [esp + 0x68]		// 0x0003224f
		fmul	dword ptr [ebx + 0x14]		// 0x00032253
		fld	dword ptr [esp + 0x78]		// 0x00032256
		fmul	dword ptr [ebx + 0x18]		// 0x0003225a
		faddp	st(1), st		// 0x0003225d
		fld	dword ptr [esp + 0x88]		// 0x0003225f
		fmul	dword ptr [ebx + 0x1c]		// 0x00032266
		faddp	st(1), st		// 0x00032269
		fld	dword ptr [esp + 0x58]		// 0x0003226b
		fmul	dword ptr [ebx + 0x10]		// 0x0003226f
		faddp	st(1), st		// 0x00032272
		fstp	dword ptr [esp + 0xa8]		// 0x00032274
		fld	dword ptr [esp + 0x7c]		// 0x0003227b
		fmul	dword ptr [ebx + 0x2c]		// 0x0003227f
		fld	dword ptr [esp + 0x4c]		// 0x00032282
		fmul	dword ptr [ebx + 0x20]		// 0x00032286
		faddp	st(1), st		// 0x00032289
		fld	dword ptr [esp + 0x5c]		// 0x0003228b
		fmul	dword ptr [ebx + 0x24]		// 0x0003228f
		faddp	st(1), st		// 0x00032292
		fld	dword ptr [esp + 0x6c]		// 0x00032294
		fmul	dword ptr [ebx + 0x28]		// 0x00032298
		faddp	st(1), st		// 0x0003229b
		fstp	dword ptr [esp + 0xac]		// 0x0003229d
		fld	dword ptr [esp + 0x80]		// 0x000322a4
		fmul	dword ptr [ebx + 0x2c]		// 0x000322ab
		fld	dword ptr [esp + 0x70]		// 0x000322ae
		fmul	dword ptr [ebx + 0x28]		// 0x000322b2
		faddp	st(1), st		// 0x000322b5
		fld	dword ptr [esp + 0x50]		// 0x000322b7
		fmul	dword ptr [ebx + 0x20]		// 0x000322bb
		faddp	st(1), st		// 0x000322be
		fld	dword ptr [esp + 0x60]		// 0x000322c0
		fmul	dword ptr [ebx + 0x24]		// 0x000322c4
		faddp	st(1), st		// 0x000322c7
		fstp	dword ptr [esp + 0xb0]		// 0x000322c9
		fld	dword ptr [esp + 0x84]		// 0x000322d0
		fmul	dword ptr [ebx + 0x2c]		// 0x000322d7
		fld	dword ptr [esp + 0x64]		// 0x000322da
		fmul	dword ptr [ebx + 0x24]		// 0x000322de
		faddp	st(1), st		// 0x000322e1
		fld	dword ptr [esp + 0x54]		// 0x000322e3
		fmul	dword ptr [ebx + 0x20]		// 0x000322e7
		faddp	st(1), st		// 0x000322ea
		fld	dword ptr [esp + 0x74]		// 0x000322ec
		fmul	dword ptr [ebx + 0x28]		// 0x000322f0
		faddp	st(1), st		// 0x000322f3
		fstp	dword ptr [esp + 0xb4]		// 0x000322f5
		fld	dword ptr [esp + 0x68]		// 0x000322fc
		fmul	dword ptr [ebx + 0x24]		// 0x00032300
		fld	dword ptr [esp + 0x78]		// 0x00032303
		fmul	dword ptr [ebx + 0x28]		// 0x00032307
		faddp	st(1), st		// 0x0003230a
		fld	dword ptr [esp + 0x88]		// 0x0003230c
		fmul	dword ptr [ebx + 0x2c]		// 0x00032313
		faddp	st(1), st		// 0x00032316
		fld	dword ptr [esp + 0x58]		// 0x00032318
		fmul	dword ptr [ebx + 0x20]		// 0x0003231c
		faddp	st(1), st		// 0x0003231f
		fstp	dword ptr [esp + 0xb8]		// 0x00032321
		fld	dword ptr [esp + 0x7c]		// 0x00032328
		fmul	dword ptr [ebx + 0x3c]		// 0x0003232c
		fld	dword ptr [esp + 0x4c]		// 0x0003232f
		fmul	dword ptr [ebx + 0x30]		// 0x00032333
		faddp	st(1), st		// 0x00032336
		fld	dword ptr [esp + 0x5c]		// 0x00032338
		fmul	dword ptr [ebx + 0x34]		// 0x0003233c
		faddp	st(1), st		// 0x0003233f
		fld	dword ptr [esp + 0x6c]		// 0x00032341
		fmul	dword ptr [ebx + 0x38]		// 0x00032345
		faddp	st(1), st		// 0x00032348
		fstp	dword ptr [esp + 0xbc]		// 0x0003234a
		fld	dword ptr [esp + 0x80]		// 0x00032351
		fmul	dword ptr [ebx + 0x3c]		// 0x00032358
		fld	dword ptr [esp + 0x70]		// 0x0003235b
		fmul	dword ptr [ebx + 0x38]		// 0x0003235f
		faddp	st(1), st		// 0x00032362
		fld	dword ptr [esp + 0x50]		// 0x00032364
		fmul	dword ptr [ebx + 0x30]		// 0x00032368
		lea	esi, [esp + 0x8c]		// 0x0003236b
		mov	ecx, 0x10		// 0x00032372
		faddp	st(1), st		// 0x00032377
		fld	dword ptr [esp + 0x60]		// 0x00032379
		fmul	dword ptr [ebx + 0x34]		// 0x0003237d
		faddp	st(1), st		// 0x00032380
		fstp	dword ptr [esp + 0xc0]		// 0x00032382
		fld	dword ptr [esp + 0x84]		// 0x00032389
		fmul	dword ptr [ebx + 0x3c]		// 0x00032390
		fld	dword ptr [esp + 0x64]		// 0x00032393
		fmul	dword ptr [ebx + 0x34]		// 0x00032397
		faddp	st(1), st		// 0x0003239a
		fld	dword ptr [esp + 0x54]		// 0x0003239c
		fmul	dword ptr [ebx + 0x30]		// 0x000323a0
		faddp	st(1), st		// 0x000323a3
		fld	dword ptr [esp + 0x74]		// 0x000323a5
		fmul	dword ptr [ebx + 0x38]		// 0x000323a9
		faddp	st(1), st		// 0x000323ac
		fstp	dword ptr [esp + 0xc4]		// 0x000323ae
		fld	dword ptr [esp + 0x68]		// 0x000323b5
		fmul	dword ptr [ebx + 0x34]		// 0x000323b9
		fld	dword ptr [esp + 0x78]		// 0x000323bc
		fmul	dword ptr [ebx + 0x38]		// 0x000323c0
		faddp	st(1), st		// 0x000323c3
		fld	dword ptr [esp + 0x88]		// 0x000323c5
		fmul	dword ptr [ebx + 0x3c]		// 0x000323cc
		faddp	st(1), st		// 0x000323cf
		fld	dword ptr [esp + 0x58]		// 0x000323d1
		fmul	dword ptr [ebx + 0x30]		// 0x000323d5
		faddp	st(1), st		// 0x000323d8
		fstp	dword ptr [esp + 0xc8]		// 0x000323da
		rep movsd		// 0x000323e1
		pop	edi		// 0x000323e3
		pop	esi		// 0x000323e4
		pop	ebx		// 0x000323e5
		add	esp, 0xc4		// 0x000323e6
		ret		// 0x000323ec
L323ed:
		lea	esi, [esp + 0x4c]		// 0x000323ed
		mov	ecx, 0x10		// 0x000323f1
		rep movsd		// 0x000323f6
L323f8:
		pop	edi		// 0x000323f8
		pop	esi		// 0x000323f9
		pop	ebx		// 0x000323fa
		add	esp, 0xc4		// 0x000323fb
		ret		// 0x00032401
		}
	}

// phys_fn_001657 (0x00032460, 60 B)
// Reverses an array of dwords in place (cdecl: count, array): false when either
// is zero; else count/2 swaps from both ends, and true. 001472 (ConvexHull.cpp)
// calls it to turn a polygon's references round with its plane. (The row is
// also modelled in ObjectModel.cpp; this is its product form, convex-mesh gap
// Task 2f.)
__declspec(noinline) bool nxIceReverseArray(NxU32 count, NxU32* array)
	{
	if(!count || !array)
		return false;
	const NxU32 half = count >> 1;
	NxU32* last = array + count - 1;
	for(NxU32 i = 0; i < half; i++)
		{
		const NxU32 tmp = *last;
		*last = array[i];
		array[i] = tmp;
		last--;
		}
	return true;
	}

// phys_fn_001661 (0x000324f0, 155 B)
// Adds a direction to a Container of axes (three floats per axis) unless it is
// nearly parallel to one already there. The direction is copied; when the sign
// bit of its x word is set (`cmp dword ptr [eax], 0; jns`, so -0 and negative
// NaNs too) all three components are negated with fld / fchs / fstp (the load
// quiets a signalling NaN). For each stored axis, |(z*sz + y*sy) + x*sx| is
// compared with 0.9999f and the direction is rejected (false) when it is
// greater (`test ah, 0x41; je`: a NaN never rejects). Otherwise it is appended
// through 001591 (the three words copied as integers) and true is returned.
// The oracle's is thiscall on the Container (`ret 4`). Here it is __fastcall
// with the Container in ecx, an unused edx and the direction on the stack,
// popped by the callee, because the vendored Container header is not changed.
// The listing's instructions, naked (see the top of the file).
__declspec(naked) bool __fastcall nxIceAddUniqueAxis(IceCore::Container* /*axes*/, NxU32 /*edx*/,
	const IceMaths::Point* /*axis*/)
	{
	__asm
		{
		sub	esp, 0xc		// 0x000324f0
		mov	eax, dword ptr [esp + 0x10]		// 0x000324f3
		mov	edx, dword ptr [eax]		// 0x000324f7
		mov	dword ptr [esp], edx		// 0x000324f9
		mov	edx, dword ptr [eax + 4]		// 0x000324fc
		mov	dword ptr [esp + 4], edx		// 0x000324ff
		mov	edx, dword ptr [eax + 8]		// 0x00032503
		mov	dword ptr [esp + 8], edx		// 0x00032506
		cmp	dword ptr [eax], 0		// 0x0003250a
		jns	L3252b		// 0x0003250d
		fld	dword ptr [esp]		// 0x0003250f
		fchs		// 0x00032512
		fstp	dword ptr [esp]		// 0x00032514
		fld	dword ptr [esp + 4]		// 0x00032517
		fchs		// 0x0003251b
		fstp	dword ptr [esp + 4]		// 0x0003251d
		fld	dword ptr [esp + 8]		// 0x00032521
		fchs		// 0x00032525
		fstp	dword ptr [esp + 8]		// 0x00032527
L3252b:
		mov	eax, 0xaaaaaaab		// 0x0003252b
		mul	dword ptr [ecx + 4]		// 0x00032530
		shr	edx, 1		// 0x00032533
		test	edx, edx		// 0x00032535
		push	esi		// 0x00032537
		mov	esi, dword ptr [ecx + 8]		// 0x00032538
		je	L3256f		// 0x0003253b
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x0003253d lea ecx, [ecx]
L32540:
		fld	dword ptr [esp + 0xc]		// 0x00032540
		dec	edx		// 0x00032544
		fmul	dword ptr [esi + 8]		// 0x00032545
		fld	dword ptr [esp + 8]		// 0x00032548
		fmul	dword ptr [esi + 4]		// 0x0003254c
		faddp	st(1), st		// 0x0003254f
		fld	dword ptr [esp + 4]		// 0x00032551
		fmul	dword ptr [esi]		// 0x00032555
		faddp	st(1), st		// 0x00032557
		fabs		// 0x00032559
		fcomp	dword ptr kIceMeshToolsAxisLimit		// 0x0003255b
		fnstsw	ax		// 0x00032561
		test	ah, 0x41		// 0x00032563
		je	L32582		// 0x00032566
		add	esi, 0xc		// 0x00032568
		test	edx, edx		// 0x0003256b
		jne	L32540		// 0x0003256d
L3256f:
		lea	eax, [esp + 4]		// 0x0003256f
		push	eax		// 0x00032573
		call	nxIceContainerAddPoint		// 0x00032574
		mov	al, 1		// 0x00032579
		pop	esi		// 0x0003257b
		add	esp, 0xc		// 0x0003257c
		ret	4		// 0x0003257f
L32582:
		xor	al, al		// 0x00032582
		pop	esi		// 0x00032584
		add	esp, 0xc		// 0x00032585
		ret	4		// 0x00032588
		}
	}

// phys_fn_001663 (0x00032590, 19 B)
Valencies::Valencies()
	{
	mNbVerts = 0;
	mNbAdjVerts = 0;
	mValencies = 0;
	mOffsets = 0;
	mAdjVerts = 0;
	}

// phys_fn_001665 (0x000325b0, 95 B)
// Releases +0x08, +0x0c and +0x10 in that order, each only when non-null and
// each cleared afterwards.
Valencies::~Valencies()
	{
	if(mValencies)
		{
		nxIceFree(mValencies);
		mValencies = 0;
		}
	if(mOffsets)
		{
		nxIceFree(mOffsets);
		mOffsets = 0;
		}
	if(mAdjVerts)
		{
		nxIceFree(mAdjVerts);
		mAdjVerts = 0;
		}
	}

// phys_fn_001667 (0x00032610, 512 B)
// Valencies::Compute. The valences are counted over the edges of an EdgeList
// built with FacesToEdges only (no vertices; epsilon 0.001f, 0x3a83126f, which
// nothing reads); with AdjacentList the offsets are their prefix sums, the
// adjacent vertices are written through the offsets (advancing them), and the
// offsets are formed again. Any failure after the EdgeList exists releases it
// (0x0003275f); a failed valence array returns before it is built. The vertex
// count is trusted: the references must be below it.
bool Valencies::Compute(const VALENCESCREATE& create)
	{
	mNbVerts = create.NbVerts;
	mValencies = (NxU32*) nxIceAlloc(mNbVerts * 4, NX_MEMORY_PERSISTENT);
	if(!mValencies)
		return false;
	memset(mValencies, 0, mNbVerts * 4);

	EdgeList EL;
	EDGELISTCREATE ELC;
	ELC.NbFaces = create.NbFaces;
	ELC.DFaces = create.DFaces;
	ELC.WFaces = create.WFaces;
	ELC.FacesToEdges = true;
	ELC.VerticesToEdges = false;
	ELC.Verts = 0;
	ELC.Epsilon = 0.001f;
	if(!EL.Init(ELC))
		return false;

	// 0x000326a4..0x000326da.
	for(NxU32 i = 0; i < EL.mNbEdges; i++)
		{
		mValencies[EL.mEdges[i].Ref0]++;
		mValencies[EL.mEdges[i].Ref1]++;
		}

	if(create.AdjacentList)
		{
		mOffsets = (NxU32*) nxIceAlloc(mNbVerts * 4, NX_MEMORY_PERSISTENT);
		if(!mOffsets)
			return false;

		// 0x00032701..0x0003272c.
		mOffsets[0] = 0;
		for(NxU32 i = 1; i < mNbVerts; i++)
			mOffsets[i] = mValencies[i - 1] + mOffsets[i - 1];

		// 0x0003272e..0x0003275d: the last vertex's run end.
		mNbAdjVerts = mValencies[mNbVerts - 1] + mOffsets[mNbVerts - 1];
		mAdjVerts = (NxU32*) nxIceAlloc(mNbAdjVerts * 4, NX_MEMORY_PERSISTENT);
		if(!mAdjVerts)
			return false;

		// 0x00032773..0x000327c4.
		for(NxU32 i = 0; i < EL.mNbEdges; i++)
			{
			const NxU32 Ref0 = EL.mEdges[i].Ref0;
			const NxU32 Ref1 = EL.mEdges[i].Ref1;
			mAdjVerts[mOffsets[Ref0]++] = Ref1;
			mAdjVerts[mOffsets[Ref1]++] = Ref0;
			}

		// 0x000327c6..0x000327fa.
		mOffsets[0] = 0;
		for(NxU32 i = 1; i < mNbVerts; i++)
			mOffsets[i] = mValencies[i - 1] + mOffsets[i - 1];
		}
	return true;
	}
