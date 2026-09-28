/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The convex-hull rows around `ConvexHull.cpp` (prerequisite P-Hull of
// units/convex-mesh-gap-contract.md). Convex-mesh gap Task 2e wrote 001461 (in
// the P-Small list); Task 2f writes the rest of P-Hull from the Capstone
// listing: 001441, 001445, 001449, 001459, 001463, 001465, 001472, 001496 (with
// its continuations 001498 and 001500) and 001502 (with 001504..001512). The
// file name follows 001465's __FILE__ (0x101076ac); the other rows lie in the
// gaps either side of it, so their placement here is a choice (the contract
// records it).
//
// The float rows (001441, 001445, 001459, 001463, 001472, 001496, 001502) are
// the listing's instructions, naked (the precedent of 001712 in Geometry.cpp
// and 001651 in IceMeshTools.cpp), because their values live on the x87 stack
// across stores that narrow, and C++ would reorder, widen or spill them
// differently: the frames, the operands each one loads, the stores that narrow
// and the orders of the terms are the oracle's. 001502 is mostly integer code,
// but it ends in an x87 normalisation and its six allocations and releases are
// interleaved with the sort; naked keeps the whole row one transcription.
// Branch targets are labels named by their oracle RVA; the listing's alignment
// fillers are emitted as their bytes. Calls go to the candidate's rows of the
// same stable IDs: the 004803 getter (nxGetSdkAllocator; slot 0 malloc, slot 3
// free), 000001 and the element constructors below, 001657
// (IceMeshTools.cpp), and the vendored Container (004836, 004846), RadixSort
// (005157, 005163, 005159), Triangle::Area (005179) and Plane::Set (005155).
// MSVC's inline assembler cannot name a member function, so those are reached
// through /alternatename aliases of their decorated names: the call is the same
// direct call to the same function. The integer rows 001449 and 001465 are C++.
//
// x87: on the /arch:IA32 list. Built /EHs-c- with the other ICE-shaped files
// (the listings are frameless; 001465's Adjacencies and Container locals would
// otherwise get an unwind frame).

#include "ConvexHull.h"

#include <malloc.h>
#include <string.h>

// The vendored members the naked rows call, by their decorated names.
extern "C" void nxIceCallContainerCtor();		// 004836, Container::Container()
extern "C" void nxIceCallContainerDtor();		// 004846, Container::~Container()
extern "C" void nxIceCallRadixSortCtor();		// 005157, RadixSort::RadixSort()
extern "C" void nxIceCallRadixSortDtor();		// 005159, RadixSort::~RadixSort()
extern "C" void nxIceCallRadixSortSort();		// 005163, RadixSort::Sort(const udword*, udword, RadixHint)
extern "C" void nxIceCallTriangleArea();		// 005179, Triangle::Area()
extern "C" void nxIceCallPlaneSet();			// 005155, Plane::Set(const Point&, const Point&, const Point&)
#pragma comment(linker, "/alternatename:_nxIceCallContainerCtor=??0Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxIceCallContainerDtor=??1Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxIceCallRadixSortCtor=??0RadixSort@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxIceCallRadixSortDtor=??1RadixSort@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxIceCallRadixSortSort=?Sort@RadixSort@IceCore@@QAEAAV12@PBIIW4RadixHint@2@@Z")
#pragma comment(linker, "/alternatename:_nxIceCallTriangleArea=?Area@Triangle@IceMaths@@QBEMXZ")
#pragma comment(linker, "/alternatename:_nxIceCallPlaneSet=?Set@Plane@IceMaths@@QAEAAV12@ABVPoint@2@00@Z")


// The constants the float rows read: 0.0f (0x101041f0), 1.0f (0x101041ec), 0.5f
// (0x101043cc) and 1/3 (0x101068ec, the bits 0x3eaaaaab).
static const float kIceHullZero = 0.0f;
static const float kIceHullOne = 1.0f;
static const float kIceHullHalf = 0.5f;
static const float kIceHullThird = 1.0f / 3.0f;

// The report 001465 makes when a polygon's outline does not close (002160,
// through the seam): its file and line.
static const char gConvexHullFile[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\ConvexHull.cpp";

// phys_fn_000001 (0x00001000, 48 B)
// MSVC's `vector constructor iterator` as the image carries it (stdcall: the
// array, the element size, the count, the constructor): the constructor is
// called with ecx on each element in turn, nothing when the count is not
// positive. Written for the `new[]` sites of 001472 and 001502, which call it.
__declspec(naked) void __stdcall nxIceVectorConstruct(void* /*array*/, NxU32 /*size*/, NxU32 /*count*/,
	void* (__fastcall* /*ctor*/)(void*))
	{
	__asm
		{
		mov	eax, dword ptr [esp + 0xc]		// 0x00001000
		dec	eax		// 0x00001004
		js	L0102d		// 0x00001005
		push	ebx		// 0x00001007
		mov	ebx, dword ptr [esp + 0x14]		// 0x00001008
		push	ebp		// 0x0000100c
		mov	ebp, dword ptr [esp + 0x10]		// 0x0000100d
		push	esi		// 0x00001011
		mov	esi, dword ptr [esp + 0x10]		// 0x00001012
		push	edi		// 0x00001016
		lea	edi, [eax + 1]		// 0x00001017
		_emit	0x8d
		_emit	0x9b
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0000101a lea ebx, [ebx]
L01020:
		mov	ecx, esi		// 0x00001020
		call	ebx		// 0x00001022
		add	esi, ebp		// 0x00001024
		dec	edi		// 0x00001026
		jne	L01020		// 0x00001027
		pop	edi		// 0x00001029
		pop	esi		// 0x0000102a
		pop	ebp		// 0x0000102b
		pop	ebx		// 0x0000102c
L0102d:
		ret	0x10		// 0x0000102d
		}
	}

// phys_fn_000925 (0x00020440, 13 B)
// The polygon's constructor: +0x00, +0x04 and +0x08 zeroed, this returned.
// (The row is also modelled in ObjectModel.cpp; this is its product form.)
__declspec(naked) void* __fastcall nxHullPolygonConstruct(void* /*polygon*/)
	{
	__asm
		{
		mov	eax, ecx		// 0x00020440
		xor	ecx, ecx		// 0x00020442
		mov	dword ptr [eax], ecx		// 0x00020444
		mov	dword ptr [eax + 4], ecx		// 0x00020446
		mov	dword ptr [eax + 8], ecx		// 0x00020449
		ret		// 0x0002044c
		}
	}

// phys_fn_001391 (0x00027f00, 3 B)
// The identity constructor 001502's edges are built with: this returned.
// (Its product form for the shape tables is in ObjectModel.cpp; this is the one
// the `new[]` site pushes.)
__declspec(naked) void* __fastcall nxIceIdentityConstruct(void* /*object*/)
	{
	__asm
		{
		mov	eax, ecx		// 0x00027f00
		ret		// 0x00027f02
		}
	}

// phys_fn_001439 (0x0002a610, 15 B)
// EdgeDesc's constructor: the two words and the dword zeroed, this returned.
// (Also modelled in ObjectModel.cpp; EdgeList.cpp zeroes its EdgeDescs inline.)
__declspec(naked) void* __fastcall nxEdgeDescConstruct(void* /*desc*/)
	{
	__asm
		{
		mov	eax, ecx		// 0x0002a610
		xor	ecx, ecx		// 0x0002a612
		mov	word ptr [eax], cx		// 0x0002a614
		mov	word ptr [eax + 2], cx		// 0x0002a617
		mov	dword ptr [eax + 4], ecx		// 0x0002a61b
		ret		// 0x0002a61e
		}
	}

// phys_fn_001441 (0x0002a620, 178 B)
// IndexedTriangle::Area over 16-bit references (thiscall on the triangle, `ret
// 4`): 0.0f when the vertices are null; else |(p0 - p2) x (p0 - p1)| * 0.5,
// the first edge kept on the stack, the second's x and y stored, the cross
// product's x and y stored and its z squared wide, the root of
// ((z^2 + y^2) + x^2) scaled by 0.5f. The listing's instructions, naked.
__declspec(naked) float __fastcall nxHullTriangleArea(const NxU16* /*triangle*/, NxU32 /*edx*/,
	const IceMaths::Point* /*verts*/)
	{
	__asm
		{
		sub	esp, 0x18		// 0x0002a620
		push	esi		// 0x0002a623
		mov	esi, dword ptr [esp + 0x20]		// 0x0002a624
		test	esi, esi		// 0x0002a628
		jne	L2a639		// 0x0002a62a
		fld	dword ptr kIceHullZero		// 0x0002a62c
		pop	esi		// 0x0002a632
		add	esp, 0x18		// 0x0002a633
		ret	4		// 0x0002a636
L2a639:
		movzx	eax, word ptr [ecx]		// 0x0002a639
		movzx	edx, word ptr [ecx + 2]		// 0x0002a63c
		movzx	ecx, word ptr [ecx + 4]		// 0x0002a640
		lea	eax, [eax + eax*2]		// 0x0002a644
		lea	eax, [esi + eax*4]		// 0x0002a647
		lea	ecx, [ecx + ecx*2]		// 0x0002a64a
		lea	ecx, [esi + ecx*4]		// 0x0002a64d
		fld	dword ptr [eax]		// 0x0002a650
		lea	edx, [edx + edx*2]		// 0x0002a652
		lea	edx, [esi + edx*4]		// 0x0002a655
		fsub	dword ptr [ecx]		// 0x0002a658
		pop	esi		// 0x0002a65a
		fld	dword ptr [eax + 4]		// 0x0002a65b
		fsub	dword ptr [ecx + 4]		// 0x0002a65e
		fld	dword ptr [eax + 8]		// 0x0002a661
		fsub	dword ptr [ecx + 8]		// 0x0002a664
		fld	dword ptr [eax]		// 0x0002a667
		fsub	dword ptr [edx]		// 0x0002a669
		fstp	dword ptr [esp]		// 0x0002a66b
		fld	dword ptr [eax + 4]		// 0x0002a66e
		fsub	dword ptr [edx + 4]		// 0x0002a671
		fstp	dword ptr [esp + 4]		// 0x0002a674
		fld	dword ptr [eax + 8]		// 0x0002a678
		fsub	dword ptr [edx + 8]		// 0x0002a67b
		fld	dword ptr [esp + 4]		// 0x0002a67e
		fmul	st, st(2)		// 0x0002a682
		fld	st(1)		// 0x0002a684
		fmul	st, st(4)		// 0x0002a686
		fsubp	st(1), st		// 0x0002a688
		fstp	dword ptr [esp + 0xc]		// 0x0002a68a
		fmul	st, st(3)		// 0x0002a68e
		fxch	st(1)		// 0x0002a690
		fmul	dword ptr [esp]		// 0x0002a692
		fsubp	st(1), st		// 0x0002a695
		fstp	dword ptr [esp + 0x10]		// 0x0002a697
		fld	dword ptr [esp]		// 0x0002a69b
		fmul	st, st(1)		// 0x0002a69e
		fld	dword ptr [esp + 4]		// 0x0002a6a0
		fmul	st, st(3)		// 0x0002a6a4
		fsubp	st(1), st		// 0x0002a6a6
		fstp	st(2)		// 0x0002a6a8
		fstp	st(0)		// 0x0002a6aa
		fld	st(0)		// 0x0002a6ac
		fmulp	st(1), st		// 0x0002a6ae
		fld	dword ptr [esp + 0x10]		// 0x0002a6b0
		fmul	dword ptr [esp + 0x10]		// 0x0002a6b4
		faddp	st(1), st		// 0x0002a6b8
		fld	dword ptr [esp + 0xc]		// 0x0002a6ba
		fmul	dword ptr [esp + 0xc]		// 0x0002a6be
		faddp	st(1), st		// 0x0002a6c2
		fsqrt		// 0x0002a6c4
		fmul	dword ptr kIceHullHalf		// 0x0002a6c6
		add	esp, 0x18		// 0x0002a6cc
		ret	4		// 0x0002a6cf
		}
	}

// phys_fn_001445 (0x0002a790, 146 B)
// IndexedTriangle::Center over 16-bit references (thiscall, `ret 8`): nothing
// when the vertices are null; else ((p1 + p0) + p2) * (1/3) per component, the
// z and x sums stored before they are scaled and the results stored through a
// local and copied as integers. The listing's instructions, naked.
__declspec(naked) void __fastcall nxHullTriangleCenter(const NxU16* /*triangle*/, NxU32 /*edx*/,
	const IceMaths::Point* /*verts*/, IceMaths::Point* /*center*/)
	{
	__asm
		{
		sub	esp, 0x18		// 0x0002a790
		push	esi		// 0x0002a793
		mov	esi, dword ptr [esp + 0x20]		// 0x0002a794
		test	esi, esi		// 0x0002a798
		je	L2a81b		// 0x0002a79a
		movzx	edx, word ptr [ecx + 2]		// 0x0002a79c
		movzx	eax, word ptr [ecx]		// 0x0002a7a0
		movzx	ecx, word ptr [ecx + 4]		// 0x0002a7a3
		lea	edx, [edx + edx*2]		// 0x0002a7a7
		lea	edx, [esi + edx*4]		// 0x0002a7aa
		lea	eax, [eax + eax*2]		// 0x0002a7ad
		lea	eax, [esi + eax*4]		// 0x0002a7b0
		fld	dword ptr [edx]		// 0x0002a7b3
		lea	ecx, [ecx + ecx*2]		// 0x0002a7b5
		lea	ecx, [esi + ecx*4]		// 0x0002a7b8
		fadd	dword ptr [eax]		// 0x0002a7bb
		fld	dword ptr [edx + 4]		// 0x0002a7bd
		fadd	dword ptr [eax + 4]		// 0x0002a7c0
		fld	dword ptr [edx + 8]		// 0x0002a7c3
		mov	edx, dword ptr [esp + 0x24]		// 0x0002a7c6
		fadd	dword ptr [eax + 8]		// 0x0002a7ca
		fstp	dword ptr [esp + 0xc]		// 0x0002a7cd
		fxch	st(1)		// 0x0002a7d1
		fadd	dword ptr [ecx]		// 0x0002a7d3
		fstp	dword ptr [esp + 0x10]		// 0x0002a7d5
		fadd	dword ptr [ecx + 4]		// 0x0002a7d9
		fld	dword ptr [esp + 0xc]		// 0x0002a7dc
		fadd	dword ptr [ecx + 8]		// 0x0002a7e0
		fld	dword ptr [esp + 0x10]		// 0x0002a7e3
		fmul	dword ptr kIceHullThird		// 0x0002a7e7
		fstp	dword ptr [esp + 4]		// 0x0002a7ed
		mov	eax, dword ptr [esp + 4]		// 0x0002a7f1
		fxch	st(1)		// 0x0002a7f5
		mov	dword ptr [edx], eax		// 0x0002a7f7
		fmul	dword ptr kIceHullThird		// 0x0002a7f9
		fstp	dword ptr [esp + 8]		// 0x0002a7ff
		mov	ecx, dword ptr [esp + 8]		// 0x0002a803
		mov	dword ptr [edx + 4], ecx		// 0x0002a807
		fmul	dword ptr kIceHullThird		// 0x0002a80a
		fstp	dword ptr [esp + 0xc]		// 0x0002a810
		mov	eax, dword ptr [esp + 0xc]		// 0x0002a814
		mov	dword ptr [edx + 8], eax		// 0x0002a818
L2a81b:
		pop	esi		// 0x0002a81b
		add	esp, 0x18		// 0x0002a81c
		ret	8		// 0x0002a81f
		}
	}

// phys_fn_001449 (0x0002a900, 156 B)
// Gathers one polygon's faces: from `face`, every face reached across an edge
// whose link word has bit 29 (active) clear, depth first, each marked and
// appended to the Container (Add, which resizes by one when full). Edges 0 and
// 1 recurse; edge 2 is followed in the same frame, until it is active or leads
// to a marked face. A marked face on entry adds nothing.
__declspec(noinline) void nxHullGatherFaces(IceCore::Container* faces, const AdjTriangle* adj, NxU32 face,
	NxU8* marks)
	{
	if(marks[face])
		return;
	do
		{
		marks[face] = 1;
		faces->Add(face);
		const AdjTriangle* tri = adj + face;
		if(!(tri->ATri[0] & 0x20000000))
			nxHullGatherFaces(faces, adj, tri->ATri[0] & 0x1fffffff, marks);
		if(!(tri->ATri[1] & 0x20000000))
			nxHullGatherFaces(faces, adj, tri->ATri[1] & 0x1fffffff, marks);
		const NxU32 link = tri->ATri[2];
		if(link & 0x20000000)
			return;
		face = link & 0x1fffffff;
		}
	while(!marks[face]);
	}

// phys_fn_001459 (0x0002ad60, 241 B)
// The hull's centre (thiscall, `ret 4`): false when it has no vertex count or
// no vertices. Otherwise the output is zeroed and, per face, 001441's area
// (stored narrow) weights 001445's centre, added component by component, and
// summed; the output is then scaled by 1 / (sum of areas) -- with no faces,
// 1 / 0 times zero. The listing's instructions, naked.
__declspec(naked) bool __fastcall nxHullComputeCentroid(const ConvexHull* /*hull*/, NxU32 /*edx*/,
	IceMaths::Point* /*center*/)
	{
	__asm
		{
		sub	esp, 0x24		// 0x0002ad60
		push	ebx		// 0x0002ad63
		mov	ebx, ecx		// 0x0002ad64
		mov	ecx, dword ptr [ebx + 0xc]		// 0x0002ad66
		xor	eax, eax		// 0x0002ad69
		cmp	ecx, eax		// 0x0002ad6b
		je	L2ae48		// 0x0002ad6d
		cmp	dword ptr [ebx + 0x10], eax		// 0x0002ad73
		je	L2ae48		// 0x0002ad76
		push	esi		// 0x0002ad7c
		mov	esi, dword ptr [esp + 0x30]		// 0x0002ad7d
		mov	dword ptr [esi + 8], eax		// 0x0002ad81
		mov	dword ptr [esi + 4], eax		// 0x0002ad84
		mov	dword ptr [esi], eax		// 0x0002ad87
		cmp	dword ptr [ebx + 4], eax		// 0x0002ad89
		mov	dword ptr [esp + 0xc], eax		// 0x0002ad8c
		mov	dword ptr [esp + 0x10], eax		// 0x0002ad90
		jbe	L2ae20		// 0x0002ad94
		push	ebp		// 0x0002ad9a
		mov	dword ptr [esp + 0xc], eax		// 0x0002ad9b
		push	edi		// 0x0002ad9f
L2ada0:
		mov	ebp, dword ptr [esp + 0x10]		// 0x0002ada0
		mov	edi, dword ptr [ebx + 8]		// 0x0002ada4
		add	edi, ebp		// 0x0002ada7
		mov	ebp, dword ptr [ebx + 0x10]		// 0x0002ada9
		push	ebp		// 0x0002adac
		mov	ecx, edi		// 0x0002adad
		call	nxHullTriangleArea		// 0x0002adaf
		fstp	dword ptr [esp + 0x38]		// 0x0002adb4
		lea	eax, [esp + 0x1c]		// 0x0002adb8
		push	eax		// 0x0002adbc
		push	ebp		// 0x0002adbd
		mov	ecx, edi		// 0x0002adbe
		call	nxHullTriangleCenter		// 0x0002adc0
		fld	dword ptr [esp + 0x1c]		// 0x0002adc5
		fmul	dword ptr [esp + 0x38]		// 0x0002adc9
		mov	eax, dword ptr [esp + 0x18]		// 0x0002adcd
		fld	dword ptr [esp + 0x20]		// 0x0002add1
		mov	edx, dword ptr [esp + 0x10]		// 0x0002add5
		fmul	dword ptr [esp + 0x38]		// 0x0002add9
		inc	eax		// 0x0002addd
		fld	dword ptr [esp + 0x24]		// 0x0002adde
		add	edx, 6		// 0x0002ade2
		fmul	dword ptr [esp + 0x38]		// 0x0002ade5
		mov	dword ptr [esp + 0x18], eax		// 0x0002ade9
		mov	dword ptr [esp + 0x10], edx		// 0x0002aded
		fstp	dword ptr [esp + 0x30]		// 0x0002adf1
		fxch	st(1)		// 0x0002adf5
		fadd	dword ptr [esi]		// 0x0002adf7
		fstp	dword ptr [esi]		// 0x0002adf9
		fadd	dword ptr [esi + 4]		// 0x0002adfb
		fstp	dword ptr [esi + 4]		// 0x0002adfe
		fld	dword ptr [esp + 0x30]		// 0x0002ae01
		fadd	dword ptr [esi + 8]		// 0x0002ae05
		fstp	dword ptr [esi + 8]		// 0x0002ae08
		mov	ecx, dword ptr [ebx + 4]		// 0x0002ae0b
		cmp	eax, ecx		// 0x0002ae0e
		fld	dword ptr [esp + 0x38]		// 0x0002ae10
		fadd	dword ptr [esp + 0x14]		// 0x0002ae14
		fstp	dword ptr [esp + 0x14]		// 0x0002ae18
		jb	L2ada0		// 0x0002ae1c
		pop	edi		// 0x0002ae1e
		pop	ebp		// 0x0002ae1f
L2ae20:
		fld	dword ptr kIceHullOne		// 0x0002ae20
		mov	al, 1		// 0x0002ae26
		fdiv	dword ptr [esp + 0xc]		// 0x0002ae28
		fld	st(0)		// 0x0002ae2c
		fmul	dword ptr [esi]		// 0x0002ae2e
		fstp	dword ptr [esi]		// 0x0002ae30
		fld	st(0)		// 0x0002ae32
		fmul	dword ptr [esi + 4]		// 0x0002ae34
		fstp	dword ptr [esi + 4]		// 0x0002ae37
		fmul	dword ptr [esi + 8]		// 0x0002ae3a
		fstp	dword ptr [esi + 8]		// 0x0002ae3d
		pop	esi		// 0x0002ae40
		pop	ebx		// 0x0002ae41
		add	esp, 0x24		// 0x0002ae42
		ret	4		// 0x0002ae45
L2ae48:
		xor	al, al		// 0x0002ae48
		pop	ebx		// 0x0002ae4a
		add	esp, 0x24		// 0x0002ae4b
		ret	4		// 0x0002ae4e
		}
	}

// phys_fn_001463 (0x0002af30, 337 B)
// A polygon's plane (cdecl): false when there is no reference, no reference
// array or no vertex array. For each i, the triangle (i, i + n/3, i + 2n/3)
// modulo n (n/3 by the reciprocal multiply) is copied onto the stack and its
// area taken (Triangle::Area, 005179); the first strictly largest wins
// (`test ah, 0x41`: an equal or NaN area does not replace, and the best starts
// at -FLT_MAX). Plane::Set (005155) then builds the plane through that
// triangle. The listing's instructions, naked.
__declspec(naked) bool nxHullPolygonPlane(IceMaths::Plane* /*plane*/, NxU32 /*nbVerts*/,
	const NxU32* /*vrefs*/, const IceMaths::Point* /*verts*/)
	{
	__asm
		{
		sub	esp, 0x30		// 0x0002af30
		push	ebx		// 0x0002af33
		mov	ebx, dword ptr [esp + 0x3c]		// 0x0002af34
		push	ebp		// 0x0002af38
		xor	ebp, ebp		// 0x0002af39
		cmp	ebx, ebp		// 0x0002af3b
		push	esi		// 0x0002af3d
		push	edi		// 0x0002af3e
		je	L2b077		// 0x0002af3f
		mov	edi, dword ptr [esp + 0x4c]		// 0x0002af45
		cmp	edi, ebp		// 0x0002af49
		je	L2b077		// 0x0002af4b
		mov	esi, dword ptr [esp + 0x50]		// 0x0002af51
		cmp	esi, ebp		// 0x0002af55
		je	L2b077		// 0x0002af57
		mov	eax, 0xaaaaaaab		// 0x0002af5d
		mul	ebx		// 0x0002af62
		mov	ecx, edx		// 0x0002af64
		shr	ecx, 1		// 0x0002af66
		cmp	ebx, ebp		// 0x0002af68
		mov	dword ptr [esp + 0x18], ecx		// 0x0002af6a
		mov	dword ptr [esp + 0x14], 0xff7fffff		// 0x0002af6e
		mov	dword ptr [esp + 0x10], ebp		// 0x0002af76
		mov	dword ptr [esp + 0x48], ebp		// 0x0002af7a
		jbe	L2b02d		// 0x0002af7e
L2af84:
		mov	eax, dword ptr [esp + 0x48]		// 0x0002af84
		xor	edx, edx		// 0x0002af88
		add	eax, ecx		// 0x0002af8a
		div	ebx		// 0x0002af8c
		mov	ebp, edx		// 0x0002af8e
		xor	edx, edx		// 0x0002af90
		lea	eax, [ecx + ebp]		// 0x0002af92
		div	ebx		// 0x0002af95
		mov	ebp, dword ptr [edi + ebp*4]		// 0x0002af97
		mov	edx, dword ptr [edi + edx*4]		// 0x0002af9a
		lea	ecx, [edx + edx*2]		// 0x0002af9d
		lea	eax, [esi + ecx*4]		// 0x0002afa0
		lea	edx, [ebp + ebp*2]		// 0x0002afa3
		lea	ecx, [esi + edx*4]		// 0x0002afa7
		mov	edx, dword ptr [esp + 0x48]		// 0x0002afaa
		mov	edx, dword ptr [edi + edx*4]		// 0x0002afae
		lea	edx, [edx + edx*2]		// 0x0002afb1
		mov	ebp, dword ptr [esi + edx*4]		// 0x0002afb4
		lea	edx, [esi + edx*4]		// 0x0002afb7
		mov	dword ptr [esp + 0x1c], ebp		// 0x0002afba
		mov	ebp, dword ptr [edx + 4]		// 0x0002afbe
		mov	dword ptr [esp + 0x20], ebp		// 0x0002afc1
		mov	edx, dword ptr [edx + 8]		// 0x0002afc5
		mov	dword ptr [esp + 0x24], edx		// 0x0002afc8
		mov	edx, dword ptr [ecx]		// 0x0002afcc
		mov	dword ptr [esp + 0x28], edx		// 0x0002afce
		mov	edx, dword ptr [ecx + 4]		// 0x0002afd2
		mov	dword ptr [esp + 0x2c], edx		// 0x0002afd5
		mov	ecx, dword ptr [ecx + 8]		// 0x0002afd9
		mov	dword ptr [esp + 0x30], ecx		// 0x0002afdc
		mov	edx, dword ptr [eax]		// 0x0002afe0
		mov	dword ptr [esp + 0x34], edx		// 0x0002afe2
		mov	ecx, dword ptr [eax + 4]		// 0x0002afe6
		mov	dword ptr [esp + 0x38], ecx		// 0x0002afe9
		mov	edx, dword ptr [eax + 8]		// 0x0002afed
		lea	ecx, [esp + 0x1c]		// 0x0002aff0
		mov	dword ptr [esp + 0x3c], edx		// 0x0002aff4
		call	nxIceCallTriangleArea		// 0x0002aff8
		fcom	dword ptr [esp + 0x14]		// 0x0002affd
		fnstsw	ax		// 0x0002b001
		test	ah, 0x41		// 0x0002b003
		jne	L2b016		// 0x0002b006
		mov	eax, dword ptr [esp + 0x48]		// 0x0002b008
		fstp	dword ptr [esp + 0x14]		// 0x0002b00c
		mov	dword ptr [esp + 0x10], eax		// 0x0002b010
		jmp	L2b018		// 0x0002b014
L2b016:
		fstp	st(0)		// 0x0002b016
L2b018:
		mov	eax, dword ptr [esp + 0x48]		// 0x0002b018
		mov	ecx, dword ptr [esp + 0x18]		// 0x0002b01c
		inc	eax		// 0x0002b020
		cmp	eax, ebx		// 0x0002b021
		mov	dword ptr [esp + 0x48], eax		// 0x0002b023
		jb	L2af84		// 0x0002b027
L2b02d:
		mov	edx, dword ptr [esp + 0x10]		// 0x0002b02d
		lea	eax, [edx + ecx]		// 0x0002b031
		xor	edx, edx		// 0x0002b034
		div	ebx		// 0x0002b036
		mov	ebp, edx		// 0x0002b038
		lea	eax, [ecx + ebp]		// 0x0002b03a
		xor	edx, edx		// 0x0002b03d
		div	ebx		// 0x0002b03f
		mov	ebp, dword ptr [edi + ebp*4]		// 0x0002b041
		mov	edx, dword ptr [edi + edx*4]		// 0x0002b044
		lea	eax, [edx + edx*2]		// 0x0002b047
		lea	ecx, [esi + eax*4]		// 0x0002b04a
		push	ecx		// 0x0002b04d
		mov	ecx, dword ptr [esp + 0x14]		// 0x0002b04e
		mov	edi, dword ptr [edi + ecx*4]		// 0x0002b052
		mov	ecx, dword ptr [esp + 0x48]		// 0x0002b055
		lea	edx, [ebp + ebp*2]		// 0x0002b059
		lea	eax, [esi + edx*4]		// 0x0002b05d
		push	eax		// 0x0002b060
		lea	edx, [edi + edi*2]		// 0x0002b061
		lea	eax, [esi + edx*4]		// 0x0002b064
		push	eax		// 0x0002b067
		call	nxIceCallPlaneSet		// 0x0002b068
		pop	edi		// 0x0002b06d
		pop	esi		// 0x0002b06e
		pop	ebp		// 0x0002b06f
		mov	al, 1		// 0x0002b070
		pop	ebx		// 0x0002b072
		add	esp, 0x30		// 0x0002b073
		ret		// 0x0002b076
L2b077:
		pop	edi		// 0x0002b077
		pop	esi		// 0x0002b078
		pop	ebp		// 0x0002b079
		xor	al, al		// 0x0002b07a
		pop	ebx		// 0x0002b07c
		add	esp, 0x30		// 0x0002b07d
		ret		// 0x0002b080
		}
	}

// phys_fn_001465 (0x0002b090, 791 B)
// The polygons of the hull's 16-bit faces. An Adjacencies over them (001536,
// 001546; epsilon 0.001f, so the active edges come from the vertices through an
// EdgeList) must build and have no boundary edge (001542), else false. A mark
// byte per face is taken from the stack (`_alloca` of the count rounded up to
// four, through __chkstk) and zeroed. Then, repeatedly, from the first unmarked
// face: its polygon's faces are gathered (001449); every active edge of every
// gathered face is added as a vertex pair -- word 0 as (v0, v1), word 1 as
// (v0, v2), word 2 as (v1, v2); the pairs are chained into an outline (001641).
// When the outline does not close, 002160 reports line 318 of ConvexHull.cpp
// with no message and its result is returned (the seam returns false). When the
// outline has entries, their count less one is added to `data`, then that many
// of its references (when the array and the count are both non-zero), and the
// polygon count is incremented; an empty outline adds nothing. When every face
// is marked, true. The Adjacencies is released (001544) on every return.
__declspec(noinline) bool nxHullExtractPolygons(NxU32* nbPolygons, IceCore::Container* data,
	const ConvexHull* hull)
	{
	const NxU32 nbFaces = hull->mNbFaces;
	const NxU16* faces = hull->mFaces;

	ADJACENCIESCREATE create;
	create.NbFaces = nbFaces;
	create.DFaces = 0;
	create.WFaces = faces;
	create.Verts = hull->mVerts;
	create.Epsilon = 0.001f;

	Adjacencies adj;
	if(!adj.Init(create) || adj.ComputeNbBoundaryEdges())
		return false;

	NxU8* marks = (NxU8*) _alloca((nbFaces + 3) & ~3u);
	if(!marks)
		return false;
	memset(marks, 0, nbFaces);
	*nbPolygons = 0;

	for(;;)
		{
		NxU32 first = 0;
		while(first < nbFaces && marks[first])
			first++;
		if(first == nbFaces)
			return true;

		IceCore::Container polygonFaces;
		nxHullGatherFaces(&polygonFaces, adj.mFaces, first, marks);

		IceCore::Container edges;
		for(NxU32 i = 0; i < polygonFaces.GetNbEntries(); i++)
			{
			const NxU32 f = polygonFaces.GetEntries()[i];
			const NxU32 v0 = faces[f * 3 + 0];
			const NxU32 v2 = faces[f * 3 + 2];
			const NxU32 v1 = faces[f * 3 + 1];
			const AdjTriangle& t = adj.mFaces[f];
			if(t.ATri[0] & 0x20000000)
				edges.Add(v0).Add(v1);
			if(t.ATri[1] & 0x20000000)
				edges.Add(v0).Add(v2);
			if(t.ATri[2] & 0x20000000)
				edges.Add(v1).Add(v2);
			}

		IceCore::Container outline;
		if(!nxIceEdgeLoop(outline, edges))
			return opcNovodeXSetIceError(0, gConvexHullFile, 0x13e);

		const NxU32 count = outline.GetNbEntries();
		if(count)
			{
			const NxU32* refs = outline.GetEntries();
			const NxU32 nb = count - 1;
			data->Add(nb);
			if(refs && nb)
				data->Add(refs, nb);
			(*nbPolygons)++;
			}
		}
	}

// phys_fn_001472 (0x0002b6f0, 664 B)
// The polygons (thiscall): the count cleared and the previous reference array
// (+0x2c, slot 3) and polygons (+0x28, a `new[]`, released at the pointer minus
// four) released. 001465 gathers the polygons into a Container on the stack
// (false, when it fails). The polygons are `new[]` through the getter (count
// cookie, 000001 running 000925), the centre is computed (001459; its result
// is not read) and every reference copied into one array of (entries - count)
// dwords (slot 0, type 0). Per polygon: its count and its run, its plane
// (001463; the result is not read) and, when the centre is on the positive
// side (the plane's value at the centre above 0.0f; `test ah, 0x41`, so a NaN
// value does not flip), the run reversed (001657) and the plane negated
// (fchs on the four words: a load and store that quiet a signalling NaN).
// Then per polygon its least and greatest projection over every hull vertex,
// from FLT_MAX and -FLT_MAX: `fst` into the least when strictly below (`test
// ah, 5; jp`), `fstp` into the greatest when strictly above (`test ah, 0x41`);
// a NaN projection replaces neither.
// A failed allocation returns false with what was built left in place. The
// listing's instructions, naked.
__declspec(naked) bool __fastcall nxHullComputePolygons(ConvexHull* /*hull*/)
	{
	__asm
		{
		sub	esp, 0x34		// 0x0002b6f0
		push	ebx		// 0x0002b6f3
		mov	ebx, ecx		// 0x0002b6f4
		mov	eax, dword ptr [ebx + 0x2c]		// 0x0002b6f6
		push	ebp		// 0x0002b6f9
		xor	ebp, ebp		// 0x0002b6fa
		cmp	eax, ebp		// 0x0002b6fc
		push	esi		// 0x0002b6fe
		mov	dword ptr [ebx + 0x24], ebp		// 0x0002b6ff
		je	L2b717		// 0x0002b702
		call	nxGetSdkAllocator		// 0x0002b704
		mov	ecx, dword ptr [ebx + 0x2c]		// 0x0002b709
		mov	edx, dword ptr [eax]		// 0x0002b70c
		push	ecx		// 0x0002b70e
		mov	ecx, eax		// 0x0002b70f
		call	dword ptr [edx + 0xc]		// 0x0002b711
		mov	dword ptr [ebx + 0x2c], ebp		// 0x0002b714
L2b717:
		mov	esi, dword ptr [ebx + 0x28]		// 0x0002b717
		cmp	esi, ebp		// 0x0002b71a
		je	L2b731		// 0x0002b71c
		call	nxGetSdkAllocator		// 0x0002b71e
		mov	edx, dword ptr [eax]		// 0x0002b723
		add	esi, -4		// 0x0002b725
		push	esi		// 0x0002b728
		mov	ecx, eax		// 0x0002b729
		call	dword ptr [edx + 0xc]		// 0x0002b72b
		mov	dword ptr [ebx + 0x28], ebp		// 0x0002b72e
L2b731:
		lea	ecx, [esp + 0x30]		// 0x0002b731
		call	nxIceCallContainerCtor		// 0x0002b735
		push	ebx		// 0x0002b73a
		lea	eax, [esp + 0x34]		// 0x0002b73b
		push	eax		// 0x0002b73f
		lea	ecx, [esp + 0x20]		// 0x0002b740
		push	ecx		// 0x0002b744
		call	nxHullExtractPolygons		// 0x0002b745
		add	esp, 0xc		// 0x0002b74a
		test	al, al		// 0x0002b74d
		jne	L2b763		// 0x0002b74f
		lea	ecx, [esp + 0x30]		// 0x0002b751
		call	nxIceCallContainerDtor		// 0x0002b755
		pop	esi		// 0x0002b75a
		pop	ebp		// 0x0002b75b
		xor	al, al		// 0x0002b75c
		pop	ebx		// 0x0002b75e
		add	esp, 0x34		// 0x0002b75f
		ret		// 0x0002b762
L2b763:
		push	edi		// 0x0002b763
		mov	edi, dword ptr [esp + 0x1c]		// 0x0002b764
		mov	dword ptr [ebx + 0x24], edi		// 0x0002b768
		call	nxGetSdkAllocator		// 0x0002b76b
		mov	edx, dword ptr [eax]		// 0x0002b770
		lea	ecx, [edi + edi*8]		// 0x0002b772
		lea	ecx, [ecx*4 + 4]		// 0x0002b775
		push	ebp		// 0x0002b77c
		push	ecx		// 0x0002b77d
		mov	ecx, eax		// 0x0002b77e
		call	dword ptr [edx]		// 0x0002b780
		cmp	eax, ebp		// 0x0002b782
		je	L2b79b		// 0x0002b784
		push	offset nxHullPolygonConstruct		// 0x0002b786
		push	edi		// 0x0002b78b
		lea	esi, [eax + 4]		// 0x0002b78c
		push	0x24		// 0x0002b78f
		push	esi		// 0x0002b791
		mov	dword ptr [eax], edi		// 0x0002b792
		call	nxIceVectorConstruct		// 0x0002b794
		jmp	L2b79d		// 0x0002b799
L2b79b:
		xor	esi, esi		// 0x0002b79b
L2b79d:
		cmp	esi, ebp		// 0x0002b79d
		mov	dword ptr [ebx + 0x28], esi		// 0x0002b79f
		jne	L2b7b7		// 0x0002b7a2
L2b7a4:
		lea	ecx, [esp + 0x34]		// 0x0002b7a4
		call	nxIceCallContainerDtor		// 0x0002b7a8
		pop	edi		// 0x0002b7ad
		pop	esi		// 0x0002b7ae
		pop	ebp		// 0x0002b7af
		xor	al, al		// 0x0002b7b0
		pop	ebx		// 0x0002b7b2
		add	esp, 0x34		// 0x0002b7b3
		ret		// 0x0002b7b6
L2b7b7:
		lea	edx, [esp + 0x28]		// 0x0002b7b7
		push	edx		// 0x0002b7bb
		mov	ecx, ebx		// 0x0002b7bc
		call	nxHullComputeCentroid		// 0x0002b7be
		call	nxGetSdkAllocator		// 0x0002b7c3
		mov	ecx, dword ptr [esp + 0x38]		// 0x0002b7c8
		mov	edx, dword ptr [eax]		// 0x0002b7cc
		sub	ecx, edi		// 0x0002b7ce
		shl	ecx, 2		// 0x0002b7d0
		push	ebp		// 0x0002b7d3
		push	ecx		// 0x0002b7d4
		mov	ecx, eax		// 0x0002b7d5
		call	dword ptr [edx]		// 0x0002b7d7
		cmp	eax, ebp		// 0x0002b7d9
		mov	dword ptr [ebx + 0x2c], eax		// 0x0002b7db
		je	L2b7a4		// 0x0002b7de
		cmp	edi, ebp		// 0x0002b7e0
		mov	esi, dword ptr [esp + 0x3c]		// 0x0002b7e2
		mov	edx, eax		// 0x0002b7e6
		mov	dword ptr [esp + 0x10], edx		// 0x0002b7e8
		jbe	L2b975		// 0x0002b7ec
		mov	dword ptr [esp + 0x18], edi		// 0x0002b7f2
		jmp	L2b800		// 0x0002b7f6
L2b7f8:
		mov	edx, dword ptr [esp + 0x10]		// 0x0002b7f8
		mov	esi, dword ptr [esp + 0x14]		// 0x0002b7fc
L2b800:
		mov	eax, dword ptr [ebx + 0x28]		// 0x0002b800
		mov	dword ptr [eax + ebp + 4], edx		// 0x0002b803
		mov	eax, dword ptr [esi]		// 0x0002b807
		mov	ecx, dword ptr [ebx + 0x28]		// 0x0002b809
		mov	dword ptr [ecx + ebp], eax		// 0x0002b80c
		lea	ecx, [eax*4]		// 0x0002b80f
		mov	dword ptr [esp + 0x24], ecx		// 0x0002b816
		mov	edi, edx		// 0x0002b81a
		mov	edx, ecx		// 0x0002b81c
		shr	ecx, 2		// 0x0002b81e
		add	esi, 4		// 0x0002b821
		mov	dword ptr [esp + 0x14], esi		// 0x0002b824
		rep movsd		// 0x0002b828
		mov	ecx, edx		// 0x0002b82a
		mov	edx, dword ptr [esp + 0x10]		// 0x0002b82c
		and	ecx, 3		// 0x0002b830
		rep movsb		// 0x0002b833
		mov	ecx, dword ptr [ebx + 0x10]		// 0x0002b835
		push	ecx		// 0x0002b838
		push	edx		// 0x0002b839
		push	eax		// 0x0002b83a
		mov	dword ptr [esp + 0x2c], eax		// 0x0002b83b
		mov	eax, dword ptr [ebx + 0x28]		// 0x0002b83f
		lea	ecx, [eax + ebp + 0xc]		// 0x0002b842
		push	ecx		// 0x0002b846
		call	nxHullPolygonPlane		// 0x0002b847
		mov	edx, dword ptr [ebx + 0x28]		// 0x0002b84c
		fld	dword ptr [esp + 0x3c]		// 0x0002b84f
		lea	eax, [edx + ebp + 0xc]		// 0x0002b853
		add	esp, 0x10		// 0x0002b857
		fmul	dword ptr [eax + 4]		// 0x0002b85a
		fld	dword ptr [esp + 0x30]		// 0x0002b85d
		fmul	dword ptr [eax + 8]		// 0x0002b861
		faddp	st(1), st		// 0x0002b864
		fld	dword ptr [esp + 0x28]		// 0x0002b866
		fmul	dword ptr [eax]		// 0x0002b86a
		faddp	st(1), st		// 0x0002b86c
		fadd	dword ptr [eax + 0xc]		// 0x0002b86e
		fcomp	dword ptr kIceHullZero		// 0x0002b871
		fnstsw	ax		// 0x0002b877
		test	ah, 0x41		// 0x0002b879
		jne	L2b8be		// 0x0002b87c
		mov	eax, dword ptr [esp + 0x10]		// 0x0002b87e
		mov	ecx, dword ptr [esp + 0x20]		// 0x0002b882
		push	eax		// 0x0002b886
		push	ecx		// 0x0002b887
		call	nxIceReverseArray		// 0x0002b888
		mov	edx, dword ptr [ebx + 0x28]		// 0x0002b88d
		fld	dword ptr [edx + ebp + 0xc]		// 0x0002b890
		lea	eax, [edx + ebp + 0xc]		// 0x0002b894
		fchs		// 0x0002b898
		add	esp, 8		// 0x0002b89a
		fstp	dword ptr [eax]		// 0x0002b89d
		fld	dword ptr [eax + 4]		// 0x0002b89f
		fchs		// 0x0002b8a2
		fstp	dword ptr [eax + 4]		// 0x0002b8a4
		fld	dword ptr [eax + 8]		// 0x0002b8a7
		fchs		// 0x0002b8aa
		fstp	dword ptr [eax + 8]		// 0x0002b8ac
		mov	eax, dword ptr [ebx + 0x28]		// 0x0002b8af
		fld	dword ptr [eax + ebp + 0x18]		// 0x0002b8b2
		lea	eax, [eax + ebp + 0x18]		// 0x0002b8b6
		fchs		// 0x0002b8ba
		fstp	dword ptr [eax]		// 0x0002b8bc
L2b8be:
		mov	eax, dword ptr [esp + 0x24]		// 0x0002b8be
		mov	esi, dword ptr [esp + 0x14]		// 0x0002b8c2
		mov	edx, dword ptr [esp + 0x10]		// 0x0002b8c6
		add	esi, eax		// 0x0002b8ca
		add	edx, eax		// 0x0002b8cc
		mov	eax, dword ptr [esp + 0x18]		// 0x0002b8ce
		add	ebp, 0x24		// 0x0002b8d2
		dec	eax		// 0x0002b8d5
		mov	dword ptr [esp + 0x14], esi		// 0x0002b8d6
		mov	dword ptr [esp + 0x10], edx		// 0x0002b8da
		mov	dword ptr [esp + 0x18], eax		// 0x0002b8de
		jne	L2b7f8		// 0x0002b8e2
		mov	edi, dword ptr [esp + 0x1c]		// 0x0002b8e8
		xor	ebp, ebp		// 0x0002b8ec
		cmp	edi, ebp		// 0x0002b8ee
		jbe	L2b975		// 0x0002b8f0
		xor	edx, edx		// 0x0002b8f6
		mov	dword ptr [esp + 0x1c], edi		// 0x0002b8f8
		mov	ebp, 0xff7fffff		// 0x0002b8fc
L2b901:
		mov	ecx, dword ptr [ebx + 0x28]		// 0x0002b901
		mov	eax, dword ptr [ebx + 0xc]		// 0x0002b904
		test	eax, eax		// 0x0002b907
		mov	esi, dword ptr [ebx + 0x10]		// 0x0002b909
		mov	dword ptr [ecx + edx + 0x1c], 0x7f7fffff		// 0x0002b90c
		mov	ecx, dword ptr [ebx + 0x28]		// 0x0002b914
		mov	dword ptr [ecx + edx + 0x20], ebp		// 0x0002b917
		je	L2b967		// 0x0002b91b
		mov	edi, eax		// 0x0002b91d
		nop		// 0x0002b91f
L2b920:
		mov	ecx, dword ptr [ebx + 0x28]		// 0x0002b920
		mov	eax, esi		// 0x0002b923
		fld	dword ptr [eax + 8]		// 0x0002b925
		add	ecx, edx		// 0x0002b928
		fmul	dword ptr [ecx + 0x14]		// 0x0002b92a
		add	esi, 0xc		// 0x0002b92d
		fld	dword ptr [eax + 4]		// 0x0002b930
		fmul	dword ptr [ecx + 0x10]		// 0x0002b933
		faddp	st(1), st		// 0x0002b936
		fld	dword ptr [eax]		// 0x0002b938
		fmul	dword ptr [ecx + 0xc]		// 0x0002b93a
		faddp	st(1), st		// 0x0002b93d
		fcom	dword ptr [ecx + 0x1c]		// 0x0002b93f
		fnstsw	ax		// 0x0002b942
		test	ah, 5		// 0x0002b944
		jp	L2b94c		// 0x0002b947
		fst	dword ptr [ecx + 0x1c]		// 0x0002b949
L2b94c:
		mov	eax, dword ptr [ebx + 0x28]		// 0x0002b94c
		fcom	dword ptr [eax + edx + 0x20]		// 0x0002b94f
		lea	ecx, [eax + edx + 0x20]		// 0x0002b953
		fnstsw	ax		// 0x0002b957
		test	ah, 0x41		// 0x0002b959
		jne	L2b962		// 0x0002b95c
		fstp	dword ptr [ecx]		// 0x0002b95e
		jmp	L2b964		// 0x0002b960
L2b962:
		fstp	st(0)		// 0x0002b962
L2b964:
		dec	edi		// 0x0002b964
		jne	L2b920		// 0x0002b965
L2b967:
		mov	eax, dword ptr [esp + 0x1c]		// 0x0002b967
		add	edx, 0x24		// 0x0002b96b
		dec	eax		// 0x0002b96e
		mov	dword ptr [esp + 0x1c], eax		// 0x0002b96f
		jne	L2b901		// 0x0002b973
L2b975:
		lea	ecx, [esp + 0x34]		// 0x0002b975
		call	nxIceCallContainerDtor		// 0x0002b979
		pop	edi		// 0x0002b97e
		pop	esi		// 0x0002b97f
		pop	ebp		// 0x0002b980
		mov	al, 1		// 0x0002b981
		pop	ebx		// 0x0002b983
		add	esp, 0x34		// 0x0002b984
		ret		// 0x0002b987
		}
	}

// phys_fn_001496 (0x0002c8f0, 296 B)
// phys_fn_001498 (0x0002ca20, 217 B)
// phys_fn_001500 (0x0002cb00, 66 B)
// The polygon furthest along a direction (thiscall, `ret 8`), the row and its
// two continuations (the unrolled loop and its remainder). With a pose (a 4x4:
// rows at +0x00, +0x10, +0x20), the 3x3 is copied out and the direction
// rotated by it, each component ((m0 d.x + m1 d.y) + m2 d.z) stored narrow;
// without one the direction is copied as integers. The polygons are built
// first when the count is zero (001472). The first polygon's dot product seeds
// the best; each next one, (n.z d.z + n.x d.x) + n.y d.y, replaces it when
// strictly greater (`test ah, 0x41`: equal and NaN do not), four at a time
// while at least four remain and then one at a time. The index is returned.
// The listing's instructions, naked.
__declspec(naked) NxU32 __fastcall nxHullSupportPolygon(ConvexHull* /*hull*/, NxU32 /*edx*/,
	const IceMaths::Point* /*dir*/, const float* /*pose*/)
	{
	__asm
		{
		sub	esp, 0x64		// 0x0002c8f0
		mov	eax, dword ptr [esp + 0x6c]		// 0x0002c8f3
		test	eax, eax		// 0x0002c8f7
		push	ebx		// 0x0002c8f9
		push	esi		// 0x0002c8fa
		push	edi		// 0x0002c8fb
		mov	ebx, ecx		// 0x0002c8fc
		mov	dword ptr [esp + 0xc], 0		// 0x0002c8fe
		je	L2c9b7		// 0x0002c906
		mov	ecx, dword ptr [eax]		// 0x0002c90c
		mov	edx, dword ptr [eax + 4]		// 0x0002c90e
		mov	dword ptr [esp + 0x28], ecx		// 0x0002c911
		mov	ecx, dword ptr [eax + 8]		// 0x0002c915
		mov	dword ptr [esp + 0x2c], edx		// 0x0002c918
		mov	edx, dword ptr [eax + 0x10]		// 0x0002c91c
		mov	dword ptr [esp + 0x30], ecx		// 0x0002c91f
		mov	ecx, dword ptr [eax + 0x14]		// 0x0002c923
		mov	dword ptr [esp + 0x34], edx		// 0x0002c926
		mov	edx, dword ptr [eax + 0x18]		// 0x0002c92a
		mov	dword ptr [esp + 0x38], ecx		// 0x0002c92d
		mov	ecx, dword ptr [eax + 0x20]		// 0x0002c931
		mov	dword ptr [esp + 0x3c], edx		// 0x0002c934
		mov	edx, dword ptr [eax + 0x24]		// 0x0002c938
		mov	eax, dword ptr [eax + 0x28]		// 0x0002c93b
		mov	dword ptr [esp + 0x40], ecx		// 0x0002c93e
		mov	dword ptr [esp + 0x48], eax		// 0x0002c942
		mov	eax, dword ptr [esp + 0x74]		// 0x0002c946
		mov	ecx, 9		// 0x0002c94a
		lea	esi, [esp + 0x28]		// 0x0002c94f
		lea	edi, [esp + 0x4c]		// 0x0002c953
		mov	dword ptr [esp + 0x44], edx		// 0x0002c957
		rep movsd		// 0x0002c95b
		fld	dword ptr [esp + 0x4c]		// 0x0002c95d
		fmul	dword ptr [eax]		// 0x0002c961
		fld	dword ptr [esp + 0x50]		// 0x0002c963
		fmul	dword ptr [eax + 4]		// 0x0002c967
		faddp	st(1), st		// 0x0002c96a
		fld	dword ptr [esp + 0x54]		// 0x0002c96c
		fmul	dword ptr [eax + 8]		// 0x0002c970
		faddp	st(1), st		// 0x0002c973
		fstp	dword ptr [esp + 0x1c]		// 0x0002c975
		fld	dword ptr [esp + 0x58]		// 0x0002c979
		fmul	dword ptr [eax]		// 0x0002c97d
		fld	dword ptr [esp + 0x5c]		// 0x0002c97f
		fmul	dword ptr [eax + 4]		// 0x0002c983
		faddp	st(1), st		// 0x0002c986
		fld	dword ptr [esp + 0x60]		// 0x0002c988
		fmul	dword ptr [eax + 8]		// 0x0002c98c
		faddp	st(1), st		// 0x0002c98f
		fstp	dword ptr [esp + 0x20]		// 0x0002c991
		fld	dword ptr [esp + 0x64]		// 0x0002c995
		fmul	dword ptr [eax]		// 0x0002c999
		fld	dword ptr [esp + 0x68]		// 0x0002c99b
		fmul	dword ptr [eax + 4]		// 0x0002c99f
		faddp	st(1), st		// 0x0002c9a2
		fld	dword ptr [esp + 0x6c]		// 0x0002c9a4
		fmul	dword ptr [eax + 8]		// 0x0002c9a8
		lea	eax, [esp + 0x1c]		// 0x0002c9ab
		faddp	st(1), st		// 0x0002c9af
		fstp	dword ptr [esp + 0x24]		// 0x0002c9b1
		jmp	L2c9bb		// 0x0002c9b5
L2c9b7:
		mov	eax, dword ptr [esp + 0x74]		// 0x0002c9b7
L2c9bb:
		mov	ecx, dword ptr [eax]		// 0x0002c9bb
		mov	edx, dword ptr [eax + 4]		// 0x0002c9bd
		mov	eax, dword ptr [eax + 8]		// 0x0002c9c0
		mov	dword ptr [esp + 0x18], eax		// 0x0002c9c3
		mov	eax, dword ptr [ebx + 0x24]		// 0x0002c9c7
		test	eax, eax		// 0x0002c9ca
		mov	dword ptr [esp + 0x10], ecx		// 0x0002c9cc
		mov	dword ptr [esp + 0x14], edx		// 0x0002c9d0
		jne	L2c9dd		// 0x0002c9d4
		mov	ecx, ebx		// 0x0002c9d6
		call	nxHullComputePolygons		// 0x0002c9d8
L2c9dd:
		mov	edi, dword ptr [ebx + 0x24]		// 0x0002c9dd
		fld	dword ptr [esp + 0x18]		// 0x0002c9e0
		mov	ebx, dword ptr [ebx + 0x28]		// 0x0002c9e4
		fmul	dword ptr [ebx + 0x14]		// 0x0002c9e7
		lea	edx, [edi - 1]		// 0x0002c9ea
		fld	dword ptr [esp + 0x14]		// 0x0002c9ed
		xor	esi, esi		// 0x0002c9f1
		cmp	edx, 4		// 0x0002c9f3
		fmul	dword ptr [ebx + 0x10]		// 0x0002c9f6
		mov	ecx, 1		// 0x0002c9f9
		faddp	st(1), st		// 0x0002c9fe
		fld	dword ptr [esp + 0x10]		// 0x0002ca00
		fmul	dword ptr [ebx + 0xc]		// 0x0002ca04
		faddp	st(1), st		// 0x0002ca07
		jl	L2caec		// 0x0002ca09
		push	ebp		// 0x0002ca0f
		lea	ebp, [edi - 3]		// 0x0002ca10
		lea	edx, [ebx + 0x34]		// 0x0002ca13
		jmp	L2ca20		// 0x0002ca16
		_emit	0x8d
		_emit	0xa4
		_emit	0x24
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002ca18 lea esp, [esp]
		nop		// 0x0002ca1f
L2ca20:
		fld	dword ptr [esp + 0x1c]		// 0x0002ca20
		fmul	dword ptr [edx + 4]		// 0x0002ca24
		fld	dword ptr [esp + 0x14]		// 0x0002ca27
		fmul	dword ptr [edx - 4]		// 0x0002ca2b
		faddp	st(1), st		// 0x0002ca2e
		fld	dword ptr [esp + 0x18]		// 0x0002ca30
		fmul	dword ptr [edx]		// 0x0002ca34
		faddp	st(1), st		// 0x0002ca36
		fst	dword ptr [esp + 0x78]		// 0x0002ca38
		fcomp	st(1)		// 0x0002ca3c
		fnstsw	ax		// 0x0002ca3e
		test	ah, 0x41		// 0x0002ca40
		jne	L2ca4d		// 0x0002ca43
		fstp	st(0)		// 0x0002ca45
		mov	esi, ecx		// 0x0002ca47
		fld	dword ptr [esp + 0x78]		// 0x0002ca49
L2ca4d:
		fld	dword ptr [esp + 0x1c]		// 0x0002ca4d
		fmul	dword ptr [edx + 0x28]		// 0x0002ca51
		fld	dword ptr [esp + 0x18]		// 0x0002ca54
		fmul	dword ptr [edx + 0x24]		// 0x0002ca58
		faddp	st(1), st		// 0x0002ca5b
		fld	dword ptr [esp + 0x14]		// 0x0002ca5d
		fmul	dword ptr [edx + 0x20]		// 0x0002ca61
		faddp	st(1), st		// 0x0002ca64
		fst	dword ptr [esp + 0x78]		// 0x0002ca66
		fcomp	st(1)		// 0x0002ca6a
		fnstsw	ax		// 0x0002ca6c
		test	ah, 0x41		// 0x0002ca6e
		jne	L2ca7c		// 0x0002ca71
		fstp	st(0)		// 0x0002ca73
		lea	esi, [ecx + 1]		// 0x0002ca75
		fld	dword ptr [esp + 0x78]		// 0x0002ca78
L2ca7c:
		fld	dword ptr [esp + 0x1c]		// 0x0002ca7c
		fmul	dword ptr [edx + 0x4c]		// 0x0002ca80
		fld	dword ptr [esp + 0x18]		// 0x0002ca83
		fmul	dword ptr [edx + 0x48]		// 0x0002ca87
		faddp	st(1), st		// 0x0002ca8a
		fld	dword ptr [esp + 0x14]		// 0x0002ca8c
		fmul	dword ptr [edx + 0x44]		// 0x0002ca90
		faddp	st(1), st		// 0x0002ca93
		fst	dword ptr [esp + 0x78]		// 0x0002ca95
		fcomp	st(1)		// 0x0002ca99
		fnstsw	ax		// 0x0002ca9b
		test	ah, 0x41		// 0x0002ca9d
		jne	L2caab		// 0x0002caa0
		fstp	st(0)		// 0x0002caa2
		lea	esi, [ecx + 2]		// 0x0002caa4
		fld	dword ptr [esp + 0x78]		// 0x0002caa7
L2caab:
		fld	dword ptr [esp + 0x1c]		// 0x0002caab
		fmul	dword ptr [edx + 0x70]		// 0x0002caaf
		fld	dword ptr [esp + 0x18]		// 0x0002cab2
		fmul	dword ptr [edx + 0x6c]		// 0x0002cab6
		faddp	st(1), st		// 0x0002cab9
		fld	dword ptr [esp + 0x14]		// 0x0002cabb
		fmul	dword ptr [edx + 0x68]		// 0x0002cabf
		faddp	st(1), st		// 0x0002cac2
		fst	dword ptr [esp + 0x78]		// 0x0002cac4
		fcomp	st(1)		// 0x0002cac8
		fnstsw	ax		// 0x0002caca
		test	ah, 0x41		// 0x0002cacc
		jne	L2cada		// 0x0002cacf
		fstp	st(0)		// 0x0002cad1
		lea	esi, [ecx + 3]		// 0x0002cad3
		fld	dword ptr [esp + 0x78]		// 0x0002cad6
L2cada:
		add	ecx, 4		// 0x0002cada
		add	edx, 0x90		// 0x0002cadd
		cmp	ecx, ebp		// 0x0002cae3
		jb	L2ca20		// 0x0002cae5
		pop	ebp		// 0x0002caeb
L2caec:
		cmp	ecx, edi		// 0x0002caec
		jae	L2cb35		// 0x0002caee
		lea	eax, [ecx + ecx*8]		// 0x0002caf0
		lea	edx, [ebx + eax*4 + 0x10]		// 0x0002caf3
		jmp	L2cb00		// 0x0002caf7
		_emit	0x8d
		_emit	0xa4
		_emit	0x24
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002caf9 lea esp, [esp]
L2cb00:
		fld	dword ptr [esp + 0x18]		// 0x0002cb00
		fmul	dword ptr [edx + 4]		// 0x0002cb04
		fld	dword ptr [esp + 0x10]		// 0x0002cb07
		fmul	dword ptr [edx - 4]		// 0x0002cb0b
		faddp	st(1), st		// 0x0002cb0e
		fld	dword ptr [esp + 0x14]		// 0x0002cb10
		fmul	dword ptr [edx]		// 0x0002cb14
		faddp	st(1), st		// 0x0002cb16
		fst	dword ptr [esp + 0x74]		// 0x0002cb18
		fcomp	st(1)		// 0x0002cb1c
		fnstsw	ax		// 0x0002cb1e
		test	ah, 0x41		// 0x0002cb20
		jne	L2cb2d		// 0x0002cb23
		fstp	st(0)		// 0x0002cb25
		mov	esi, ecx		// 0x0002cb27
		fld	dword ptr [esp + 0x74]		// 0x0002cb29
L2cb2d:
		inc	ecx		// 0x0002cb2d
		add	edx, 0x24		// 0x0002cb2e
		cmp	ecx, edi		// 0x0002cb31
		jb	L2cb00		// 0x0002cb33
L2cb35:
		pop	edi		// 0x0002cb35
		fstp	st(0)		// 0x0002cb36
		mov	eax, esi		// 0x0002cb38
		pop	esi		// 0x0002cb3a
		pop	ebx		// 0x0002cb3b
		add	esp, 0x64		// 0x0002cb3c
		ret	8		// 0x0002cb3f
		}
	}

// phys_fn_001502 (0x0002cb50, 298 B)
// phys_fn_001504 (0x0002cc80, 749 B)
// phys_fn_001506 (0x0002cf70, 217 B)
// phys_fn_001508 (0x0002d050, 186 B)
// phys_fn_001510 (0x0002d110, 73 B)
// phys_fn_001512 (0x0002d160, 371 B)
// The edges (thiscall), the row and its five continuations. The polygons are
// built when absent (001472, tested again before each polygon is read). Four
// temporary arrays of (sum of the polygons' counts) dwords (slot 0, type 1):
// per polygon edge (i, i + 1 mod n) its smaller and larger reference, its
// polygon and its position. The pairs are radix-sorted (005157; 005163 twice,
// on the larger and then the smaller reference, signed hint) and each run of
// equal pairs becomes one edge in a `new[]` of 8-byte edges (count cookie,
// 001391) sized for all; every polygon edge gets its edge's number. The edges
// are then copied into an exactly sized `new[]` (+0x3c, the previous released)
// and the oversized one released. A second sort (by polygon, then position)
// gives each polygon's edge numbers in order (+0x30, slot 0 type 0; each
// polygon's +0x08 points at its run). The four temporaries are released. The
// edge-to-polygon table (+0x44, 8-byte EdgeDescs without a cookie, 000001
// running 001439) counts each edge's polygons in its word at +2 and gives run
// offsets; the polygons are written by edge (+0x48) and the offsets formed
// again. Then, when +0x3c, +0x44 or +0x48 is still null, this row calls itself
// (never, as each was tested above). Per edge its normal: the sum of its first
// two polygons' normals, normalised unless its squared length is zero
// (`fucompp; test ah, 0x44; jnp`: a NaN length normalises), stored as integers
// through a local. False when an allocation fails, the sorter released; the
// arrays built so far are left. The listing's instructions, naked.
__declspec(naked) bool __fastcall nxHullComputeEdges(ConvexHull* /*hull*/)
	{
	__asm
		{
		sub	esp, 0x68		// 0x0002cb50
		push	ebx		// 0x0002cb53
		push	ebp		// 0x0002cb54
		mov	ebp, ecx		// 0x0002cb55
		mov	eax, dword ptr [ebp + 0x24]		// 0x0002cb57
		test	eax, eax		// 0x0002cb5a
		push	esi		// 0x0002cb5c
		push	edi		// 0x0002cb5d
		jne	L2cb65		// 0x0002cb5e
		call	nxHullComputePolygons		// 0x0002cb60
L2cb65:
		mov	eax, dword ptr [ebp + 0x24]		// 0x0002cb65
		xor	esi, esi		// 0x0002cb68
		test	eax, eax		// 0x0002cb6a
		mov	dword ptr [esp + 0x24], eax		// 0x0002cb6c
		mov	dword ptr [esp + 0x18], esi		// 0x0002cb70
		jbe	L2cb9e		// 0x0002cb74
		xor	ebx, ebx		// 0x0002cb76
		mov	edi, eax		// 0x0002cb78
		_emit	0x8d
		_emit	0x9b
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002cb7a lea ebx, [ebx]
L2cb80:
		mov	eax, dword ptr [ebp + 0x28]		// 0x0002cb80
		test	eax, eax		// 0x0002cb83
		jne	L2cb8e		// 0x0002cb85
		mov	ecx, ebp		// 0x0002cb87
		call	nxHullComputePolygons		// 0x0002cb89
L2cb8e:
		mov	eax, dword ptr [ebp + 0x28]		// 0x0002cb8e
		add	esi, dword ptr [ebx + eax]		// 0x0002cb91
		add	ebx, 0x24		// 0x0002cb94
		dec	edi		// 0x0002cb97
		jne	L2cb80		// 0x0002cb98
		mov	dword ptr [esp + 0x18], esi		// 0x0002cb9a
L2cb9e:
		call	nxGetSdkAllocator		// 0x0002cb9e
		mov	edx, dword ptr [eax]		// 0x0002cba3
		push	1		// 0x0002cba5
		lea	ebx, [esi*4]		// 0x0002cba7
		push	ebx		// 0x0002cbae
		mov	ecx, eax		// 0x0002cbaf
		call	dword ptr [edx]		// 0x0002cbb1
		test	eax, eax		// 0x0002cbb3
		mov	dword ptr [esp + 0x14], eax		// 0x0002cbb5
		je	L2d0f1		// 0x0002cbb9
		call	nxGetSdkAllocator		// 0x0002cbbf
		mov	edx, dword ptr [eax]		// 0x0002cbc4
		push	1		// 0x0002cbc6
		push	ebx		// 0x0002cbc8
		mov	ecx, eax		// 0x0002cbc9
		call	dword ptr [edx]		// 0x0002cbcb
		test	eax, eax		// 0x0002cbcd
		mov	dword ptr [esp + 0x10], eax		// 0x0002cbcf
		je	L2d0f1		// 0x0002cbd3
		call	nxGetSdkAllocator		// 0x0002cbd9
		mov	edx, dword ptr [eax]		// 0x0002cbde
		push	1		// 0x0002cbe0
		push	ebx		// 0x0002cbe2
		mov	ecx, eax		// 0x0002cbe3
		call	dword ptr [edx]		// 0x0002cbe5
		mov	edi, eax		// 0x0002cbe7
		test	edi, edi		// 0x0002cbe9
		mov	dword ptr [esp + 0x40], edi		// 0x0002cbeb
		je	L2d0f1		// 0x0002cbef
		call	nxGetSdkAllocator		// 0x0002cbf5
		mov	edx, dword ptr [eax]		// 0x0002cbfa
		push	1		// 0x0002cbfc
		push	ebx		// 0x0002cbfe
		mov	ecx, eax		// 0x0002cbff
		call	dword ptr [edx]		// 0x0002cc01
		test	eax, eax		// 0x0002cc03
		mov	dword ptr [esp + 0x3c], eax		// 0x0002cc05
		je	L2d0f1		// 0x0002cc09
		mov	ecx, dword ptr [esp + 0x10]		// 0x0002cc0f
		mov	ebx, dword ptr [esp + 0x14]		// 0x0002cc13
		mov	dword ptr [esp + 0x34], eax		// 0x0002cc17
		mov	eax, dword ptr [esp + 0x24]		// 0x0002cc1b
		test	eax, eax		// 0x0002cc1f
		mov	dword ptr [esp + 0x28], ecx		// 0x0002cc21
		mov	dword ptr [esp + 0x30], edi		// 0x0002cc25
		mov	dword ptr [esp + 0x1c], 0		// 0x0002cc29
		jbe	L2cd02		// 0x0002cc31
		mov	dword ptr [esp + 0x20], 0		// 0x0002cc37
		nop		// 0x0002cc3f
L2cc40:
		mov	eax, dword ptr [ebp + 0x28]		// 0x0002cc40
		test	eax, eax		// 0x0002cc43
		jne	L2cc4e		// 0x0002cc45
		mov	ecx, ebp		// 0x0002cc47
		call	nxHullComputePolygons		// 0x0002cc49
L2cc4e:
		mov	eax, dword ptr [ebp + 0x28]		// 0x0002cc4e
		test	eax, eax		// 0x0002cc51
		mov	edi, dword ptr [esp + 0x20]		// 0x0002cc53
		mov	esi, dword ptr [edi + eax]		// 0x0002cc57
		mov	dword ptr [esp + 0x38], esi		// 0x0002cc5a
		jne	L2cc67		// 0x0002cc5e
		mov	ecx, ebp		// 0x0002cc60
		call	nxHullComputePolygons		// 0x0002cc62
L2cc67:
		mov	edx, dword ptr [ebp + 0x28]		// 0x0002cc67
		mov	eax, dword ptr [edi + edx + 4]		// 0x0002cc6a
		xor	ecx, ecx		// 0x0002cc6e
		test	esi, esi		// 0x0002cc70
		mov	dword ptr [esp + 0x2c], eax		// 0x0002cc72
		jbe	L2ccde		// 0x0002cc76
		jmp	L2cc80		// 0x0002cc78
		_emit	0x8d
		_emit	0x9b
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002cc7a lea ebx, [ebx]
L2cc80:
		mov	edx, dword ptr [esp + 0x2c]		// 0x0002cc80
		mov	esi, dword ptr [edx + ecx*4]		// 0x0002cc84
		lea	edi, [ecx + 1]		// 0x0002cc87
		xor	edx, edx		// 0x0002cc8a
		mov	eax, edi		// 0x0002cc8c
		div	dword ptr [esp + 0x38]		// 0x0002cc8e
		mov	eax, dword ptr [esp + 0x2c]		// 0x0002cc92
		mov	edx, dword ptr [eax + edx*4]		// 0x0002cc96
		cmp	esi, edx		// 0x0002cc99
		jbe	L2cca3		// 0x0002cc9b
		xor	esi, edx		// 0x0002cc9d
		xor	edx, esi		// 0x0002cc9f
		xor	esi, edx		// 0x0002cca1
L2cca3:
		mov	eax, dword ptr [esp + 0x28]		// 0x0002cca3
		add	eax, 4		// 0x0002cca7
		mov	dword ptr [ebx], esi		// 0x0002ccaa
		mov	dword ptr [eax - 4], edx		// 0x0002ccac
		mov	edx, dword ptr [esp + 0x1c]		// 0x0002ccaf
		mov	dword ptr [esp + 0x28], eax		// 0x0002ccb3
		mov	eax, dword ptr [esp + 0x30]		// 0x0002ccb7
		mov	dword ptr [eax], edx		// 0x0002ccbb
		add	eax, 4		// 0x0002ccbd
		mov	dword ptr [esp + 0x30], eax		// 0x0002ccc0
		mov	eax, dword ptr [esp + 0x34]		// 0x0002ccc4
		mov	dword ptr [eax], ecx		// 0x0002ccc8
		add	eax, 4		// 0x0002ccca
		mov	dword ptr [esp + 0x34], eax		// 0x0002cccd
		mov	eax, dword ptr [esp + 0x38]		// 0x0002ccd1
		mov	ecx, edi		// 0x0002ccd5
		add	ebx, 4		// 0x0002ccd7
		cmp	ecx, eax		// 0x0002ccda
		jb	L2cc80		// 0x0002ccdc
L2ccde:
		mov	eax, dword ptr [esp + 0x1c]		// 0x0002ccde
		mov	edx, dword ptr [esp + 0x20]		// 0x0002cce2
		mov	ecx, dword ptr [esp + 0x24]		// 0x0002cce6
		inc	eax		// 0x0002ccea
		add	edx, 0x24		// 0x0002cceb
		cmp	eax, ecx		// 0x0002ccee
		mov	dword ptr [esp + 0x1c], eax		// 0x0002ccf0
		mov	dword ptr [esp + 0x20], edx		// 0x0002ccf4
		jb	L2cc40		// 0x0002ccf8
		mov	esi, dword ptr [esp + 0x18]		// 0x0002ccfe
L2cd02:
		lea	ecx, [esp + 0x60]		// 0x0002cd02
		call	nxIceCallRadixSortCtor		// 0x0002cd06
		mov	eax, dword ptr [esp + 0x14]		// 0x0002cd0b
		mov	ecx, dword ptr [esp + 0x10]		// 0x0002cd0f
		push	0		// 0x0002cd13
		push	esi		// 0x0002cd15
		push	eax		// 0x0002cd16
		push	0		// 0x0002cd17
		push	esi		// 0x0002cd19
		push	ecx		// 0x0002cd1a
		lea	ecx, [esp + 0x78]		// 0x0002cd1b
		call	nxIceCallRadixSortSort		// 0x0002cd1f
		mov	ecx, eax		// 0x0002cd24
		call	nxIceCallRadixSortSort		// 0x0002cd26
		mov	edx, dword ptr [eax + 4]		// 0x0002cd2b
		mov	dword ptr [esp + 0x20], edx		// 0x0002cd2e
		mov	dword ptr [ebp + 0x38], 0		// 0x0002cd32
		call	nxGetSdkAllocator		// 0x0002cd39
		mov	edx, dword ptr [eax]		// 0x0002cd3e
		push	0		// 0x0002cd40
		lea	ecx, [esi*8 + 4]		// 0x0002cd42
		push	ecx		// 0x0002cd49
		mov	ecx, eax		// 0x0002cd4a
		call	dword ptr [edx]		// 0x0002cd4c
		test	eax, eax		// 0x0002cd4e
		je	L2d0e8		// 0x0002cd50
		push	offset nxIceIdentityConstruct		// 0x0002cd56
		push	esi		// 0x0002cd5b
		lea	ebx, [eax + 4]		// 0x0002cd5c
		push	8		// 0x0002cd5f
		push	ebx		// 0x0002cd61
		mov	dword ptr [eax], esi		// 0x0002cd62
		mov	dword ptr [esp + 0x40], ebx		// 0x0002cd64
		call	nxIceVectorConstruct		// 0x0002cd68
		test	ebx, ebx		// 0x0002cd6d
		je	L2d0e8		// 0x0002cd6f
		call	nxGetSdkAllocator		// 0x0002cd75
		mov	edx, dword ptr [eax]		// 0x0002cd7a
		push	1		// 0x0002cd7c
		lea	edi, [esi*4]		// 0x0002cd7e
		push	edi		// 0x0002cd85
		mov	ecx, eax		// 0x0002cd86
		call	dword ptr [edx]		// 0x0002cd88
		test	eax, eax		// 0x0002cd8a
		mov	dword ptr [esp + 0x1c], eax		// 0x0002cd8c
		je	L2d0e8		// 0x0002cd90
		call	nxGetSdkAllocator		// 0x0002cd96
		mov	edx, dword ptr [eax]		// 0x0002cd9b
		push	1		// 0x0002cd9d
		push	edi		// 0x0002cd9f
		mov	ecx, eax		// 0x0002cda0
		call	dword ptr [edx]		// 0x0002cda2
		test	eax, eax		// 0x0002cda4
		mov	dword ptr [esp + 0x2c], eax		// 0x0002cda6
		je	L2d0e8		// 0x0002cdaa
		call	nxGetSdkAllocator		// 0x0002cdb0
		mov	edx, dword ptr [eax]		// 0x0002cdb5
		push	1		// 0x0002cdb7
		push	edi		// 0x0002cdb9
		mov	ecx, eax		// 0x0002cdba
		call	dword ptr [edx]		// 0x0002cdbc
		test	eax, eax		// 0x0002cdbe
		mov	dword ptr [esp + 0x28], eax		// 0x0002cdc0
		je	L2d0e8		// 0x0002cdc4
		or	eax, 0xffffffff		// 0x0002cdca
		test	esi, esi		// 0x0002cdcd
		mov	dword ptr [esp + 0x38], eax		// 0x0002cdcf
		mov	dword ptr [esp + 0x34], eax		// 0x0002cdd3
		jbe	L2ce7e		// 0x0002cdd7
		mov	ecx, dword ptr [esp + 0x1c]		// 0x0002cddd
		mov	eax, dword ptr [esp + 0x2c]		// 0x0002cde1
		mov	edi, dword ptr [esp + 0x20]		// 0x0002cde5
		sub	eax, ecx		// 0x0002cde9
		mov	dword ptr [esp + 0x4c], eax		// 0x0002cdeb
		mov	eax, dword ptr [esp + 0x28]		// 0x0002cdef
		sub	edi, ecx		// 0x0002cdf3
		sub	eax, ecx		// 0x0002cdf5
		mov	dword ptr [esp + 0x44], edi		// 0x0002cdf7
		mov	dword ptr [esp + 0x50], eax		// 0x0002cdfb
		mov	dword ptr [esp + 0x20], esi		// 0x0002cdff
L2ce03:
		mov	eax, dword ptr [edi + ecx]		// 0x0002ce03
		mov	edx, dword ptr [esp + 0x40]		// 0x0002ce06
		mov	esi, dword ptr [edx + eax*4]		// 0x0002ce0a
		mov	edx, dword ptr [esp + 0x3c]		// 0x0002ce0d
		mov	edx, dword ptr [edx + eax*4]		// 0x0002ce11
		mov	ebx, dword ptr [esp + 0x10]		// 0x0002ce14
		mov	dword ptr [esp + 0x48], edx		// 0x0002ce18
		mov	edx, dword ptr [esp + 0x14]		// 0x0002ce1c
		mov	edx, dword ptr [edx + eax*4]		// 0x0002ce20
		mov	eax, dword ptr [ebx + eax*4]		// 0x0002ce23
		cmp	edx, dword ptr [esp + 0x38]		// 0x0002ce26
		jne	L2ce32		// 0x0002ce2a
		cmp	eax, dword ptr [esp + 0x34]		// 0x0002ce2c
		je	L2ce54		// 0x0002ce30
L2ce32:
		mov	edi, dword ptr [ebp + 0x38]		// 0x0002ce32
		mov	ebx, dword ptr [esp + 0x30]		// 0x0002ce35
		mov	dword ptr [ebx + edi*8], edx		// 0x0002ce39
		mov	edi, dword ptr [esp + 0x44]		// 0x0002ce3c
		mov	dword ptr [esp + 0x38], edx		// 0x0002ce40
		mov	edx, dword ptr [ebp + 0x38]		// 0x0002ce44
		mov	dword ptr [ebx + edx*8 + 4], eax		// 0x0002ce47
		mov	dword ptr [esp + 0x34], eax		// 0x0002ce4b
		inc	dword ptr [ebp + 0x38]		// 0x0002ce4f
		jmp	L2ce58		// 0x0002ce52
L2ce54:
		mov	ebx, dword ptr [esp + 0x30]		// 0x0002ce54
L2ce58:
		mov	eax, dword ptr [esp + 0x48]		// 0x0002ce58
		mov	edx, dword ptr [esp + 0x4c]		// 0x0002ce5c
		mov	dword ptr [ecx], esi		// 0x0002ce60
		mov	dword ptr [edx + ecx], eax		// 0x0002ce62
		mov	eax, dword ptr [ebp + 0x38]		// 0x0002ce65
		mov	edx, dword ptr [esp + 0x50]		// 0x0002ce68
		dec	eax		// 0x0002ce6c
		mov	dword ptr [edx + ecx], eax		// 0x0002ce6d
		mov	eax, dword ptr [esp + 0x20]		// 0x0002ce70
		add	ecx, 4		// 0x0002ce74
		dec	eax		// 0x0002ce77
		mov	dword ptr [esp + 0x20], eax		// 0x0002ce78
		jne	L2ce03		// 0x0002ce7c
L2ce7e:
		mov	esi, dword ptr [ebp + 0x3c]		// 0x0002ce7e
		test	esi, esi		// 0x0002ce81
		je	L2ce9c		// 0x0002ce83
		call	nxGetSdkAllocator		// 0x0002ce85
		mov	edx, dword ptr [eax]		// 0x0002ce8a
		add	esi, -4		// 0x0002ce8c
		push	esi		// 0x0002ce8f
		mov	ecx, eax		// 0x0002ce90
		call	dword ptr [edx + 0xc]		// 0x0002ce92
		mov	dword ptr [ebp + 0x3c], 0		// 0x0002ce95
L2ce9c:
		mov	esi, dword ptr [ebp + 0x38]		// 0x0002ce9c
		call	nxGetSdkAllocator		// 0x0002ce9f
		mov	edx, dword ptr [eax]		// 0x0002cea4
		push	0		// 0x0002cea6
		lea	ecx, [esi*8 + 4]		// 0x0002cea8
		push	ecx		// 0x0002ceaf
		mov	ecx, eax		// 0x0002ceb0
		call	dword ptr [edx]		// 0x0002ceb2
		test	eax, eax		// 0x0002ceb4
		je	L2cecd		// 0x0002ceb6
		push	offset nxIceIdentityConstruct		// 0x0002ceb8
		push	esi		// 0x0002cebd
		lea	edi, [eax + 4]		// 0x0002cebe
		push	8		// 0x0002cec1
		push	edi		// 0x0002cec3
		mov	dword ptr [eax], esi		// 0x0002cec4
		call	nxIceVectorConstruct		// 0x0002cec6
		jmp	L2cecf		// 0x0002cecb
L2cecd:
		xor	edi, edi		// 0x0002cecd
L2cecf:
		test	edi, edi		// 0x0002cecf
		mov	dword ptr [ebp + 0x3c], edi		// 0x0002ced1
		je	L2d0e8		// 0x0002ced4
		mov	ecx, dword ptr [ebp + 0x38]		// 0x0002ceda
		shl	ecx, 3		// 0x0002cedd
		mov	edx, ecx		// 0x0002cee0
		shr	ecx, 2		// 0x0002cee2
		mov	esi, ebx		// 0x0002cee5
		rep movsd		// 0x0002cee7
		mov	ecx, edx		// 0x0002cee9
		and	ecx, 3		// 0x0002ceeb
		rep movsb		// 0x0002ceee
		call	nxGetSdkAllocator		// 0x0002cef0
		mov	edx, dword ptr [eax]		// 0x0002cef5
		add	ebx, -4		// 0x0002cef7
		push	ebx		// 0x0002cefa
		mov	ecx, eax		// 0x0002cefb
		call	dword ptr [edx + 0xc]		// 0x0002cefd
		mov	ebx, dword ptr [esp + 0x18]		// 0x0002cf00
		mov	eax, dword ptr [esp + 0x1c]		// 0x0002cf04
		mov	ecx, dword ptr [esp + 0x2c]		// 0x0002cf08
		push	0		// 0x0002cf0c
		push	ebx		// 0x0002cf0e
		push	eax		// 0x0002cf0f
		push	0		// 0x0002cf10
		push	ebx		// 0x0002cf12
		push	ecx		// 0x0002cf13
		lea	ecx, [esp + 0x78]		// 0x0002cf14
		call	nxIceCallRadixSortSort		// 0x0002cf18
		mov	ecx, eax		// 0x0002cf1d
		call	nxIceCallRadixSortSort		// 0x0002cf1f
		mov	esi, dword ptr [eax + 4]		// 0x0002cf24
		mov	eax, dword ptr [ebp + 0x30]		// 0x0002cf27
		test	eax, eax		// 0x0002cf2a
		mov	dword ptr [esp + 0x20], esi		// 0x0002cf2c
		je	L2cf49		// 0x0002cf30
		call	nxGetSdkAllocator		// 0x0002cf32
		mov	ecx, dword ptr [ebp + 0x30]		// 0x0002cf37
		mov	edx, dword ptr [eax]		// 0x0002cf3a
		push	ecx		// 0x0002cf3c
		mov	ecx, eax		// 0x0002cf3d
		call	dword ptr [edx + 0xc]		// 0x0002cf3f
		mov	dword ptr [ebp + 0x30], 0		// 0x0002cf42
L2cf49:
		call	nxGetSdkAllocator		// 0x0002cf49
		mov	edx, dword ptr [eax]		// 0x0002cf4e
		push	0		// 0x0002cf50
		lea	ecx, [ebx*4]		// 0x0002cf52
		push	ecx		// 0x0002cf59
		mov	ecx, eax		// 0x0002cf5a
		call	dword ptr [edx]		// 0x0002cf5c
		mov	dword ptr [ebp + 0x30], eax		// 0x0002cf5e
		xor	eax, eax		// 0x0002cf61
		test	ebx, ebx		// 0x0002cf63
		jbe	L2cf81		// 0x0002cf65
		mov	ecx, dword ptr [esp + 0x28]		// 0x0002cf67
		jmp	L2cf70		// 0x0002cf6b
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x0002cf6d lea ecx, [ecx]
L2cf70:
		mov	edx, dword ptr [esi + eax*4]		// 0x0002cf70
		mov	edi, dword ptr [ebp + 0x30]		// 0x0002cf73
		mov	edx, dword ptr [ecx + edx*4]		// 0x0002cf76
		mov	dword ptr [edi + eax*4], edx		// 0x0002cf79
		inc	eax		// 0x0002cf7c
		cmp	eax, ebx		// 0x0002cf7d
		jb	L2cf70		// 0x0002cf7f
L2cf81:
		mov	esi, dword ptr [esp + 0x24]		// 0x0002cf81
		test	esi, esi		// 0x0002cf85
		mov	edx, dword ptr [ebp + 0x30]		// 0x0002cf87
		jbe	L2cfa4		// 0x0002cf8a
		mov	eax, dword ptr [ebp + 0x28]		// 0x0002cf8c
		xor	ecx, ecx		// 0x0002cf8f
L2cf91:
		mov	dword ptr [eax + ecx + 8], edx		// 0x0002cf91
		mov	eax, dword ptr [ebp + 0x28]		// 0x0002cf95
		mov	edi, dword ptr [eax + ecx]		// 0x0002cf98
		add	ecx, 0x24		// 0x0002cf9b
		dec	esi		// 0x0002cf9e
		lea	edx, [edx + edi*4]		// 0x0002cf9f
		jne	L2cf91		// 0x0002cfa2
L2cfa4:
		call	nxGetSdkAllocator		// 0x0002cfa4
		mov	ecx, dword ptr [esp + 0x3c]		// 0x0002cfa9
		mov	edx, dword ptr [eax]		// 0x0002cfad
		push	ecx		// 0x0002cfaf
		mov	ecx, eax		// 0x0002cfb0
		call	dword ptr [edx + 0xc]		// 0x0002cfb2
		call	nxGetSdkAllocator		// 0x0002cfb5
		mov	ecx, dword ptr [esp + 0x40]		// 0x0002cfba
		mov	edx, dword ptr [eax]		// 0x0002cfbe
		push	ecx		// 0x0002cfc0
		mov	ecx, eax		// 0x0002cfc1
		call	dword ptr [edx + 0xc]		// 0x0002cfc3
		call	nxGetSdkAllocator		// 0x0002cfc6
		mov	ecx, dword ptr [esp + 0x10]		// 0x0002cfcb
		mov	edx, dword ptr [eax]		// 0x0002cfcf
		push	ecx		// 0x0002cfd1
		mov	ecx, eax		// 0x0002cfd2
		call	dword ptr [edx + 0xc]		// 0x0002cfd4
		call	nxGetSdkAllocator		// 0x0002cfd7
		mov	ecx, dword ptr [esp + 0x14]		// 0x0002cfdc
		mov	edx, dword ptr [eax]		// 0x0002cfe0
		push	ecx		// 0x0002cfe2
		mov	ecx, eax		// 0x0002cfe3
		call	dword ptr [edx + 0xc]		// 0x0002cfe5
		mov	esi, dword ptr [ebp + 0x44]		// 0x0002cfe8
		test	esi, esi		// 0x0002cfeb
		je	L2d003		// 0x0002cfed
		call	nxGetSdkAllocator		// 0x0002cfef
		mov	edx, dword ptr [eax]		// 0x0002cff4
		push	esi		// 0x0002cff6
		mov	ecx, eax		// 0x0002cff7
		call	dword ptr [edx + 0xc]		// 0x0002cff9
		mov	dword ptr [ebp + 0x44], 0		// 0x0002cffc
L2d003:
		mov	edi, dword ptr [ebp + 0x38]		// 0x0002d003
		call	nxGetSdkAllocator		// 0x0002d006
		mov	edx, dword ptr [eax]		// 0x0002d00b
		push	0		// 0x0002d00d
		lea	ecx, [edi*8]		// 0x0002d00f
		push	ecx		// 0x0002d016
		mov	ecx, eax		// 0x0002d017
		call	dword ptr [edx]		// 0x0002d019
		mov	esi, eax		// 0x0002d01b
		test	esi, esi		// 0x0002d01d
		je	L2d031		// 0x0002d01f
		push	offset nxEdgeDescConstruct		// 0x0002d021
		push	edi		// 0x0002d026
		push	8		// 0x0002d027
		push	esi		// 0x0002d029
		call	nxIceVectorConstruct		// 0x0002d02a
		jmp	L2d033		// 0x0002d02f
L2d031:
		xor	esi, esi		// 0x0002d031
L2d033:
		test	esi, esi		// 0x0002d033
		mov	dword ptr [ebp + 0x44], esi		// 0x0002d035
		je	L2d0e8		// 0x0002d038
		test	ebx, ebx		// 0x0002d03e
		mov	ecx, dword ptr [ebp + 0x30]		// 0x0002d040
		jbe	L2d064		// 0x0002d043
		mov	edx, ebx		// 0x0002d045
		jmp	L2d050		// 0x0002d047
		_emit	0x8d
		_emit	0xa4
		_emit	0x24
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002d049 lea esp, [esp]
L2d050:
		mov	eax, dword ptr [ecx]		// 0x0002d050
		mov	esi, dword ptr [ebp + 0x44]		// 0x0002d052
		inc	word ptr [esi + eax*8 + 2]		// 0x0002d055
		lea	eax, [esi + eax*8 + 2]		// 0x0002d05a
		add	ecx, 4		// 0x0002d05e
		dec	edx		// 0x0002d061
		jne	L2d050		// 0x0002d062
L2d064:
		mov	ecx, dword ptr [ebp + 0x44]		// 0x0002d064
		mov	dword ptr [ecx + 4], 0		// 0x0002d067
		mov	eax, dword ptr [ebp + 0x38]		// 0x0002d06e
		mov	ecx, 1		// 0x0002d071
		cmp	eax, ecx		// 0x0002d076
		jbe	L2d09b		// 0x0002d078
		_emit	0x8d
		_emit	0x9b
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002d07a lea ebx, [ebx]
L2d080:
		mov	edx, dword ptr [ebp + 0x44]		// 0x0002d080
		mov	esi, dword ptr [edx + ecx*8 - 4]		// 0x0002d083
		lea	eax, [edx + ecx*8]		// 0x0002d087
		movzx	edx, word ptr [eax - 6]		// 0x0002d08a
		add	edx, esi		// 0x0002d08e
		mov	dword ptr [eax + 4], edx		// 0x0002d090
		mov	eax, dword ptr [ebp + 0x38]		// 0x0002d093
		inc	ecx		// 0x0002d096
		cmp	ecx, eax		// 0x0002d097
		jb	L2d080		// 0x0002d099
L2d09b:
		mov	ecx, dword ptr [ebp + 0x44]		// 0x0002d09b
		mov	eax, dword ptr [ebp + 0x38]		// 0x0002d09e
		movzx	esi, word ptr [ecx + eax*8 - 6]		// 0x0002d0a1
		lea	eax, [ecx + eax*8]		// 0x0002d0a6
		mov	ecx, dword ptr [eax - 4]		// 0x0002d0a9
		mov	eax, dword ptr [ebp + 0x48]		// 0x0002d0ac
		add	esi, ecx		// 0x0002d0af
		test	eax, eax		// 0x0002d0b1
		je	L2d0cc		// 0x0002d0b3
		call	nxGetSdkAllocator		// 0x0002d0b5
		mov	ecx, dword ptr [ebp + 0x48]		// 0x0002d0ba
		mov	edx, dword ptr [eax]		// 0x0002d0bd
		push	ecx		// 0x0002d0bf
		mov	ecx, eax		// 0x0002d0c0
		call	dword ptr [edx + 0xc]		// 0x0002d0c2
		mov	dword ptr [ebp + 0x48], 0		// 0x0002d0c5
L2d0cc:
		call	nxGetSdkAllocator		// 0x0002d0cc
		mov	edx, dword ptr [eax]		// 0x0002d0d1
		push	0		// 0x0002d0d3
		lea	ecx, [esi*4]		// 0x0002d0d5
		push	ecx		// 0x0002d0dc
		mov	ecx, eax		// 0x0002d0dd
		call	dword ptr [edx]		// 0x0002d0df
		test	eax, eax		// 0x0002d0e1
		mov	dword ptr [ebp + 0x48], eax		// 0x0002d0e3
		jne	L2d0fb		// 0x0002d0e6
L2d0e8:
		lea	ecx, [esp + 0x60]		// 0x0002d0e8
		call	nxIceCallRadixSortDtor		// 0x0002d0ec
L2d0f1:
		pop	edi		// 0x0002d0f1
		pop	esi		// 0x0002d0f2
		pop	ebp		// 0x0002d0f3
		xor	al, al		// 0x0002d0f4
		pop	ebx		// 0x0002d0f6
		add	esp, 0x68		// 0x0002d0f7
		ret		// 0x0002d0fa
L2d0fb:
		mov	eax, dword ptr [ebp + 0x30]		// 0x0002d0fb
		xor	ecx, ecx		// 0x0002d0fe
		test	ebx, ebx		// 0x0002d100
		jbe	L2d143		// 0x0002d102
		mov	esi, dword ptr [esp + 0x1c]		// 0x0002d104
		jmp	L2d110		// 0x0002d108
		_emit	0x8d
		_emit	0x9b
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002d10a lea ebx, [ebx]
L2d110:
		mov	edi, dword ptr [eax]		// 0x0002d110
		mov	ebx, dword ptr [ebp + 0x44]		// 0x0002d112
		mov	edi, dword ptr [ebx + edi*8 + 4]		// 0x0002d115
		mov	edx, dword ptr [esp + 0x20]		// 0x0002d119
		mov	edx, dword ptr [edx + ecx*4]		// 0x0002d11d
		mov	edx, dword ptr [esi + edx*4]		// 0x0002d120
		mov	ebx, dword ptr [ebp + 0x48]		// 0x0002d123
		mov	dword ptr [ebx + edi*4], edx		// 0x0002d126
		mov	edx, dword ptr [eax]		// 0x0002d129
		mov	edi, dword ptr [ebp + 0x44]		// 0x0002d12b
		lea	edx, [edi + edx*8 + 4]		// 0x0002d12e
		mov	edi, dword ptr [edx]		// 0x0002d132
		inc	edi		// 0x0002d134
		add	eax, 4		// 0x0002d135
		mov	dword ptr [edx], edi		// 0x0002d138
		mov	edx, dword ptr [esp + 0x18]		// 0x0002d13a
		inc	ecx		// 0x0002d13e
		cmp	ecx, edx		// 0x0002d13f
		jb	L2d110		// 0x0002d141
L2d143:
		mov	eax, dword ptr [ebp + 0x44]		// 0x0002d143
		xor	edi, edi		// 0x0002d146
		mov	dword ptr [eax + 4], edi		// 0x0002d148
		mov	eax, dword ptr [ebp + 0x38]		// 0x0002d14b
		mov	ecx, 1		// 0x0002d14e
		cmp	eax, ecx		// 0x0002d153
		jbe	L2d17b		// 0x0002d155
		jmp	L2d160		// 0x0002d157
		_emit	0x8d
		_emit	0xa4
		_emit	0x24
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002d159 lea esp, [esp]
L2d160:
		mov	edx, dword ptr [ebp + 0x44]		// 0x0002d160
		mov	esi, dword ptr [edx + ecx*8 - 4]		// 0x0002d163
		lea	eax, [edx + ecx*8]		// 0x0002d167
		movzx	edx, word ptr [eax - 6]		// 0x0002d16a
		add	edx, esi		// 0x0002d16e
		mov	dword ptr [eax + 4], edx		// 0x0002d170
		mov	eax, dword ptr [ebp + 0x38]		// 0x0002d173
		inc	ecx		// 0x0002d176
		cmp	ecx, eax		// 0x0002d177
		jb	L2d160		// 0x0002d179
L2d17b:
		call	nxGetSdkAllocator		// 0x0002d17b
		mov	ecx, dword ptr [esp + 0x28]		// 0x0002d180
		mov	edx, dword ptr [eax]		// 0x0002d184
		push	ecx		// 0x0002d186
		mov	ecx, eax		// 0x0002d187
		call	dword ptr [edx + 0xc]		// 0x0002d189
		call	nxGetSdkAllocator		// 0x0002d18c
		mov	ecx, dword ptr [esp + 0x2c]		// 0x0002d191
		mov	edx, dword ptr [eax]		// 0x0002d195
		push	ecx		// 0x0002d197
		mov	ecx, eax		// 0x0002d198
		call	dword ptr [edx + 0xc]		// 0x0002d19a
		call	nxGetSdkAllocator		// 0x0002d19d
		mov	ecx, dword ptr [esp + 0x1c]		// 0x0002d1a2
		mov	edx, dword ptr [eax]		// 0x0002d1a6
		push	ecx		// 0x0002d1a8
		mov	ecx, eax		// 0x0002d1a9
		call	dword ptr [edx + 0xc]		// 0x0002d1ab
		cmp	dword ptr [ebp + 0x3c], edi		// 0x0002d1ae
		jne	L2d1ba		// 0x0002d1b1
		mov	ecx, ebp		// 0x0002d1b3
		call	nxHullComputeEdges		// 0x0002d1b5
L2d1ba:
		cmp	dword ptr [ebp + 0x44], edi		// 0x0002d1ba
		jne	L2d1c6		// 0x0002d1bd
		mov	ecx, ebp		// 0x0002d1bf
		call	nxHullComputeEdges		// 0x0002d1c1
L2d1c6:
		cmp	dword ptr [ebp + 0x48], edi		// 0x0002d1c6
		mov	esi, dword ptr [ebp + 0x44]		// 0x0002d1c9
		jne	L2d1d5		// 0x0002d1cc
		mov	ecx, ebp		// 0x0002d1ce
		call	nxHullComputeEdges		// 0x0002d1d0
L2d1d5:
		mov	edx, dword ptr [ebp + 0x48]		// 0x0002d1d5
		mov	dword ptr [esp + 0x50], edx		// 0x0002d1d8
		call	nxGetSdkAllocator		// 0x0002d1dc
		mov	ecx, dword ptr [ebp + 0x38]		// 0x0002d1e1
		mov	edx, dword ptr [eax]		// 0x0002d1e4
		lea	ecx, [ecx + ecx*2]		// 0x0002d1e6
		shl	ecx, 2		// 0x0002d1e9
		push	edi		// 0x0002d1ec
		push	ecx		// 0x0002d1ed
		mov	ecx, eax		// 0x0002d1ee
		call	dword ptr [edx]		// 0x0002d1f0
		mov	dword ptr [ebp + 0x40], eax		// 0x0002d1f2
		mov	eax, dword ptr [ebp + 0x38]		// 0x0002d1f5
		test	eax, eax		// 0x0002d1f8
		jbe	L2d2c0		// 0x0002d1fa
		xor	ebx, ebx		// 0x0002d200
		add	esi, 4		// 0x0002d202
L2d205:
		mov	edx, dword ptr [esi]		// 0x0002d205
		mov	eax, dword ptr [esp + 0x50]		// 0x0002d207
		lea	ecx, [eax + edx*4]		// 0x0002d20b
		mov	eax, dword ptr [ecx + 4]		// 0x0002d20e
		mov	edx, dword ptr [ebp + 0x28]		// 0x0002d211
		mov	ecx, dword ptr [ecx]		// 0x0002d214
		lea	eax, [eax + eax*8]		// 0x0002d216
		fld	dword ptr [edx + eax*4 + 0xc]		// 0x0002d219
		lea	eax, [edx + eax*4 + 0xc]		// 0x0002d21d
		lea	ecx, [ecx + ecx*8]		// 0x0002d221
		fadd	dword ptr [edx + ecx*4 + 0xc]		// 0x0002d224
		lea	ecx, [edx + ecx*4 + 0xc]		// 0x0002d228
		fstp	dword ptr [esp + 0x54]		// 0x0002d22c
		fld	dword ptr [ecx + 4]		// 0x0002d230
		fadd	dword ptr [eax + 4]		// 0x0002d233
		fstp	dword ptr [esp + 0x58]		// 0x0002d236
		fld	dword ptr [ecx + 8]		// 0x0002d23a
		fadd	dword ptr [eax + 8]		// 0x0002d23d
		fst	dword ptr [esp + 0x5c]		// 0x0002d240
		fmul	dword ptr [esp + 0x5c]		// 0x0002d244
		fld	dword ptr [esp + 0x58]		// 0x0002d248
		fmul	dword ptr [esp + 0x58]		// 0x0002d24c
		faddp	st(1), st		// 0x0002d250
		fld	dword ptr [esp + 0x54]		// 0x0002d252
		fmul	dword ptr [esp + 0x54]		// 0x0002d256
		faddp	st(1), st		// 0x0002d25a
		fld	dword ptr kIceHullZero		// 0x0002d25c
		fld	st(1)		// 0x0002d262
		fucompp		// 0x0002d264
		fnstsw	ax		// 0x0002d266
		test	ah, 0x44		// 0x0002d268
		jnp	L2d293		// 0x0002d26b
		fsqrt		// 0x0002d26d
		fdivr	dword ptr kIceHullOne		// 0x0002d26f
		fld	dword ptr [esp + 0x54]		// 0x0002d275
		fmul	st, st(1)		// 0x0002d279
		fstp	dword ptr [esp + 0x54]		// 0x0002d27b
		fld	dword ptr [esp + 0x58]		// 0x0002d27f
		fmul	st, st(1)		// 0x0002d283
		fstp	dword ptr [esp + 0x58]		// 0x0002d285
		fld	dword ptr [esp + 0x5c]		// 0x0002d289
		fmul	st, st(1)		// 0x0002d28d
		fstp	dword ptr [esp + 0x5c]		// 0x0002d28f
L2d293:
		mov	edx, dword ptr [ebp + 0x40]		// 0x0002d293
		fstp	st(0)		// 0x0002d296
		mov	eax, dword ptr [esp + 0x54]		// 0x0002d298
		mov	ecx, dword ptr [esp + 0x58]		// 0x0002d29c
		add	edx, ebx		// 0x0002d2a0
		mov	dword ptr [edx], eax		// 0x0002d2a2
		mov	eax, dword ptr [esp + 0x5c]		// 0x0002d2a4
		mov	dword ptr [edx + 4], ecx		// 0x0002d2a8
		mov	dword ptr [edx + 8], eax		// 0x0002d2ab
		mov	eax, dword ptr [ebp + 0x38]		// 0x0002d2ae
		inc	edi		// 0x0002d2b1
		add	esi, 8		// 0x0002d2b2
		add	ebx, 0xc		// 0x0002d2b5
		cmp	edi, eax		// 0x0002d2b8
		jb	L2d205		// 0x0002d2ba
L2d2c0:
		lea	ecx, [esp + 0x60]		// 0x0002d2c0
		call	nxIceCallRadixSortDtor		// 0x0002d2c4
		pop	edi		// 0x0002d2c9
		pop	esi		// 0x0002d2ca
		pop	ebp		// 0x0002d2cb
		mov	al, 1		// 0x0002d2cc
		pop	ebx		// 0x0002d2ce
		add	esp, 0x68		// 0x0002d2cf
		ret		// 0x0002d2d2
		}
	}

// phys_fn_001461 (0x0002ae60, 194 B)
// The hull's vertex normals, angle-weighted. The previous array (+0x14) is
// released through the 004803 getter's slot 3 and cleared; false when the hull
// has no vertices, or when the new array (slot 0, type 0, nbVerts * 12 bytes,
// stored into +0x14 before it is tested) is null. Otherwise a MeshNormals on
// the stack (its constructor, 001536) computes into that array (001651): the
// create block {nbVerts, vertices, nbFaces, no 32-bit faces, the 16-bit faces,
// weight by angle, face normals allocated by the object, the vertex normals
// given}; the object is released (001649, which frees the face normals) and
// 001651's result returned.
__declspec(noinline) bool ConvexHull::ComputeVertexNormals()
	{
	if(mVertexNormals)
		{
		nxIceFree(mVertexNormals);
		mVertexNormals = 0;
		}
	if(!mNbVerts)
		return false;
	mVertexNormals = (IceMaths::Point*) nxIceAlloc(mNbVerts * 12, NX_MEMORY_PERSISTENT);
	if(!mVertexNormals)
		return false;

	MESHNORMALSCREATE create;
	memset(&create, 0, sizeof(create));
	create.NbVerts = mNbVerts;
	create.Verts = mVerts;
	create.NbFaces = mNbFaces;
	create.DFaces = 0;
	create.WFaces = mFaces;
	create.WeightByAngle = true;
	create.FaceNormals = 0;
	create.VertexNormals = mVertexNormals;

	MeshNormals normals;
	const bool status = nxMeshNormalsCompute(&normals, 0, &create);
	return status;
	}
