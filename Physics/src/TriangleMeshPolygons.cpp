/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The TriangleMesh polygon interface, prerequisite P-Mesh of
// units/convex-mesh-gap-contract.md, written by convex-mesh gap Task 2g from the
// Capstone listing. The TriangleMesh constructor stores a second table pointer
// at +0x04 (0x101085d4, stored at 0x000554a4 and again by the destructor at
// 0x00055581): twelve thiscall slots whose `this` is the mesh plus four, and
// every one of them reads [this + 0x9c], the object at the mesh's +0xa0 (the
// convex hull, ConvexHull.h). The convex rows call these through the table
// (001803..001818: slots 0, 2, 3, 4, 9, 10 and 11).
//
// In the image the slots sit in TriangleMesh's own span (0x00054800..0x0005491a
// and 0x000552c0); they are written in a file of their own, as Task 2e did for
// 002186 and 002188, so that the asset harness, which links TriangleMesh.cpp,
// does not have to link the hull rows they reach. The table is written as the
// array of function pointers the image holds (gTriangleMeshPolygonTable), in
// the image's order; a naked function cannot be a member or a virtual slot, so
// every slot is __fastcall with the object in ecx and an unused edx, which is
// thiscall's convention for the callee.
//
// Slots 0..2 (002211, 002213, 002215) are the census's reconstructed getters;
// they get product forms here so the table is complete (their states are
// unchanged). Slots 3..8 build the hull's arrays lazily (001472 when +0x24 or
// +0x28 is zero, 001514 for +0x34, 001502 for +0x3c, +0x44, +0x48) and return
// them; 9 and 10 are tail jumps to 001496 and 001516 on the hull; 11 (002249) is
// the hull's extent along a direction. 002249's first argument is the pair's
// shared scratch record (the dispatcher's context: +0x04 the visited array's
// count, +0x08 the visited array, +0x14 the stamp), whose stamp it advances
// through 000505, written here as a product form (its state is unchanged).
//
// Every row is the listing's instructions, naked (branch targets are labels
// named by their oracle RVA). x87: on the /arch:IA32 list; /EHs-c- with the
// other ICE-shaped files.

#include "NxPhysicsBackend.h"
#if !NX_PHYSICS_USE_X87
#include "TriangleMesh.h"
#endif
#include "TriangleMeshPolygons.h"

#if NX_PHYSICS_USE_X87
// phys_fn_000505 (0x00010190, 50 B)
// The scratch record's next stamp (thiscall, no argument): +0x14 incremented;
// when it wraps to zero the visited array (+0x08, when non-null) is cleared
// over +0x04 dwords (`rep stosd` and `rep stosb` over the byte count) and the
// stamp restarts at the count. Returns the stamp. A product form (the row is
// the census's reconstructed "growable buffer reset"; its state is unchanged).
__declspec(naked) NxU32 __fastcall nxScratchStamp(void* /*scratch*/)
	{
	__asm
		{
		mov	edx, ecx		// 0x00010190
		inc	dword ptr [edx + 0x14]		// 0x00010192
		jne	L101be		// 0x00010195
		push	edi		// 0x00010197
		mov	edi, dword ptr [edx + 8]		// 0x00010198
		test	edi, edi		// 0x0001019b
		je	L101b7		// 0x0001019d
		mov	ecx, dword ptr [edx + 4]		// 0x0001019f
		shl	ecx, 2		// 0x000101a2
		push	esi		// 0x000101a5
		mov	esi, ecx		// 0x000101a6
		shr	ecx, 2		// 0x000101a8
		xor	eax, eax		// 0x000101ab
		rep stosd		// 0x000101ad
		mov	ecx, esi		// 0x000101af
		and	ecx, 3		// 0x000101b1
		rep stosb		// 0x000101b4
		pop	esi		// 0x000101b6
L101b7:
		mov	eax, dword ptr [edx + 4]		// 0x000101b7
		mov	dword ptr [edx + 0x14], eax		// 0x000101ba
		pop	edi		// 0x000101bd
L101be:
		mov	eax, dword ptr [edx + 0x14]		// 0x000101be
		ret		// 0x000101c1
		}
	}

// phys_fn_002211 (0x00054800, 10 B)
// Slot 0: the hull's centre, +0x18 (a product form; state unchanged).
__declspec(naked) const IceMaths::Point* __fastcall nxMeshHullCentre(const void* /*iface*/)
	{
	__asm
		{
		mov	eax, dword ptr [ecx + 0x9c]		// 0x00054800
		add	eax, 0x18		// 0x00054806
		ret		// 0x00054809
		}
	}

// phys_fn_002213 (0x00054810, 10 B)
// Slot 1: the hull's vertex count, +0x0c (a product form; state unchanged).
__declspec(naked) NxU32 __fastcall nxMeshHullVertexCount(const void* /*iface*/)
	{
	__asm
		{
		mov	eax, dword ptr [ecx + 0x9c]		// 0x00054810
		mov	eax, dword ptr [eax + 0xc]		// 0x00054816
		ret		// 0x00054819
		}
	}

// phys_fn_002215 (0x00054820, 10 B)
// Slot 2: the hull's vertices, +0x10 (a product form; state unchanged).
__declspec(naked) const IceMaths::Point* __fastcall nxMeshHullVertices(const void* /*iface*/)
	{
	__asm
		{
		mov	eax, dword ptr [ecx + 0x9c]		// 0x00054820
		mov	eax, dword ptr [eax + 0x10]		// 0x00054826
		ret		// 0x00054829
		}
	}

// phys_fn_002221 (0x00054850, 26 B)
// Slot 3: the polygon count, +0x24, after 001472 when it is zero.
__declspec(naked) NxU32 __fastcall nxMeshHullPolygonCount(const void* /*iface*/)
	{
	__asm
		{
		push	esi		// 0x00054850
		mov	esi, dword ptr [ecx + 0x9c]		// 0x00054851
		mov	eax, dword ptr [esi + 0x24]		// 0x00054857
		test	eax, eax		// 0x0005485a
		jne	L54865		// 0x0005485c
		mov	ecx, esi		// 0x0005485e
		call	nxHullComputePolygons		// 0x00054860
L54865:
		mov	eax, dword ptr [esi + 0x24]		// 0x00054865
		pop	esi		// 0x00054868
		ret		// 0x00054869
		}
	}

// phys_fn_002223 (0x00054870, 38 B)
// Slot 4 (`ret 4`): polygon i, +0x28 + 0x24 * i, after 001472 when the array
// is null. The index is not checked.
__declspec(naked) const HullPolygon* __fastcall nxMeshHullPolygon(const void* /*iface*/, NxU32 /*edx*/,
	NxU32 /*index*/)
	{
	__asm
		{
		push	esi		// 0x00054870
		mov	esi, dword ptr [ecx + 0x9c]		// 0x00054871
		mov	eax, dword ptr [esi + 0x28]		// 0x00054877
		test	eax, eax		// 0x0005487a
		jne	L54885		// 0x0005487c
		mov	ecx, esi		// 0x0005487e
		call	nxHullComputePolygons		// 0x00054880
L54885:
		mov	eax, dword ptr [esp + 8]		// 0x00054885
		mov	ecx, dword ptr [esi + 0x28]		// 0x00054889
		lea	eax, [eax + eax*8]		// 0x0005488c
		lea	eax, [ecx + eax*4]		// 0x0005488f
		pop	esi		// 0x00054892
		ret	4		// 0x00054893
		}
	}

// phys_fn_002225 (0x000548a0, 26 B)
// Slot 5: the edge axes, +0x34, after 001514 when null.
__declspec(naked) IceCore::Container* __fastcall nxMeshHullEdgeAxes(const void* /*iface*/)
	{
	__asm
		{
		push	esi		// 0x000548a0
		mov	esi, dword ptr [ecx + 0x9c]		// 0x000548a1
		mov	eax, dword ptr [esi + 0x34]		// 0x000548a7
		test	eax, eax		// 0x000548aa
		jne	L548b5		// 0x000548ac
		mov	ecx, esi		// 0x000548ae
		call	nxHullComputeEdgeAxes		// 0x000548b0
L548b5:
		mov	eax, dword ptr [esi + 0x34]		// 0x000548b5
		pop	esi		// 0x000548b8
		ret		// 0x000548b9
		}
	}

// phys_fn_002227 (0x000548c0, 26 B)
// Slot 6: the edges, +0x3c, after 001502 when null.
__declspec(naked) const HullEdge* __fastcall nxMeshHullEdges(const void* /*iface*/)
	{
	__asm
		{
		push	esi		// 0x000548c0
		mov	esi, dword ptr [ecx + 0x9c]		// 0x000548c1
		mov	eax, dword ptr [esi + 0x3c]		// 0x000548c7
		test	eax, eax		// 0x000548ca
		jne	L548d5		// 0x000548cc
		mov	ecx, esi		// 0x000548ce
		call	nxHullComputeEdges		// 0x000548d0
L548d5:
		mov	eax, dword ptr [esi + 0x3c]		// 0x000548d5
		pop	esi		// 0x000548d8
		ret		// 0x000548d9
		}
	}

// phys_fn_002229 (0x000548e0, 26 B)
// Slot 7: the edge-to-polygon descriptors, +0x44, after 001502 when null.
__declspec(naked) const EdgeDesc* __fastcall nxMeshHullEdgeToPolygons(const void* /*iface*/)
	{
	__asm
		{
		push	esi		// 0x000548e0
		mov	esi, dword ptr [ecx + 0x9c]		// 0x000548e1
		mov	eax, dword ptr [esi + 0x44]		// 0x000548e7
		test	eax, eax		// 0x000548ea
		jne	L548f5		// 0x000548ec
		mov	ecx, esi		// 0x000548ee
		call	nxHullComputeEdges		// 0x000548f0
L548f5:
		mov	eax, dword ptr [esi + 0x44]		// 0x000548f5
		pop	esi		// 0x000548f8
		ret		// 0x000548f9
		}
	}

// phys_fn_002231 (0x00054900, 26 B)
// Slot 8: the polygons by edge, +0x48, after 001502 when null.
__declspec(naked) const NxU32* __fastcall nxMeshHullEdgePolygons(const void* /*iface*/)
	{
	__asm
		{
		push	esi		// 0x00054900
		mov	esi, dword ptr [ecx + 0x9c]		// 0x00054901
		mov	eax, dword ptr [esi + 0x48]		// 0x00054907
		test	eax, eax		// 0x0005490a
		jne	L54915		// 0x0005490c
		mov	ecx, esi		// 0x0005490e
		call	nxHullComputeEdges		// 0x00054910
L54915:
		mov	eax, dword ptr [esi + 0x48]		// 0x00054915
		pop	esi		// 0x00054918
		ret		// 0x00054919
		}
	}

// phys_fn_002217 (0x00054830, 11 B)
// Slot 9 (`ret 8`, through the tail): the hull, then a jump to 001496 (the
// polygon furthest along the direction, the direction rotated by the pose).
__declspec(naked) NxU32 __fastcall nxMeshHullSupportPolygon(const void* /*iface*/, NxU32 /*edx*/,
	const IceMaths::Point* /*dir*/, const float* /*pose*/)
	{
	__asm
		{
		mov	ecx, dword ptr [ecx + 0x9c]		// 0x00054830
		jmp	nxHullSupportPolygon		// 0x00054836
		}
	}

// phys_fn_002219 (0x00054840, 11 B)
// Slot 10 (`ret 0xc`, through the tail): the hull, then a jump to 001516 (the
// supporting face, and in *kind whether it came through an edge).
__declspec(naked) NxU32 __fastcall nxMeshHullSupportFace(const void* /*iface*/, NxU32 /*edx*/,
	const IceMaths::Point* /*dir*/, const float* /*pose*/, NxU32* /*kind*/)
	{
	__asm
		{
		mov	ecx, dword ptr [ecx + 0x9c]		// 0x00054840
		jmp	nxHullSupportFace		// 0x00054846
		}
	}

// phys_fn_002249 (0x000552c0, 459 B)
// Slot 11 (`ret 0x18`): the hull's extent along a direction in world space.
// Arguments: the scratch record, the least and greatest projections out, the
// direction, the pose (a 4x4: rows at +0x00, +0x10, +0x20, translation at
// +0x30) and a support map (kind C, IceSupportMaps.h) or null. The direction is
// taken into the hull's frame as its dot products with the pose's first three
// rows ((r.y d.y + r.z d.z) + r.x d.x, stored narrow). With a map, its lookup
// (001556) gives a sample whose two bytes (+0x0c, +0x10) are the least and
// greatest vertex; without one, the graph at hull +0x64 is climbed twice with
// 001530 (from vertex 0, with a fresh stamp from 000505 each time and the
// scratch record's visited array), along the local direction and along its
// negation (`fchs` on the stored words); a failed climb leaves vertex 0. The
// translation's projection (t.x d.x + t.z d.z) + t.y d.y is kept on the stack
// and added to each vertex's local projection ((v.z l.z + v.y l.y) + v.x l.x):
// the least is stored, then the greatest, and when the greatest is below the
// least (`fcom; test ah, 5; jp`: equal and NaN do not) the two are swapped. The vertex
// indices are not checked against the count.
__declspec(naked) void __fastcall nxMeshHullProject(const void* /*iface*/, NxU32 /*edx*/, void* /*scratch*/,
	float* /*least*/, float* /*greatest*/, const IceMaths::Point* /*dir*/, const float* /*pose*/,
	const IceSupportMap* /*map*/)
	{
	__asm
		{
		sub	esp, 0x20		// 0x000552c0
		push	ebx		// 0x000552c3
		push	ebp		// 0x000552c4
		push	esi		// 0x000552c5
		mov	esi, dword ptr [esp + 0x3c]		// 0x000552c6
		push	edi		// 0x000552ca
		mov	edi, dword ptr [esp + 0x44]		// 0x000552cb
		fld	dword ptr [edi + 4]		// 0x000552cf
		mov	ebp, ecx		// 0x000552d2
		fmul	dword ptr [esi + 4]		// 0x000552d4
		mov	eax, dword ptr [ebp + 0x9c]		// 0x000552d7
		fld	dword ptr [edi + 8]		// 0x000552dd
		mov	ecx, dword ptr [esp + 0x48]		// 0x000552e0
		fmul	dword ptr [esi + 8]		// 0x000552e4
		faddp	st(1), st		// 0x000552e7
		fld	dword ptr [esi]		// 0x000552e9
		fmul	dword ptr [edi]		// 0x000552eb
		faddp	st(1), st		// 0x000552ed
		fstp	dword ptr [esp + 0x18]		// 0x000552ef
		fld	dword ptr [edi + 0x14]		// 0x000552f3
		fmul	dword ptr [esi + 4]		// 0x000552f6
		fld	dword ptr [edi + 0x18]		// 0x000552f9
		fmul	dword ptr [esi + 8]		// 0x000552fc
		faddp	st(1), st		// 0x000552ff
		fld	dword ptr [edi + 0x10]		// 0x00055301
		fmul	dword ptr [esi]		// 0x00055304
		faddp	st(1), st		// 0x00055306
		fstp	dword ptr [esp + 0x1c]		// 0x00055308
		fld	dword ptr [edi + 0x24]		// 0x0005530c
		fmul	dword ptr [esi + 4]		// 0x0005530f
		fld	dword ptr [edi + 0x28]		// 0x00055312
		fmul	dword ptr [esi + 8]		// 0x00055315
		faddp	st(1), st		// 0x00055318
		fld	dword ptr [edi + 0x20]		// 0x0005531a
		fmul	dword ptr [esi]		// 0x0005531d
		faddp	st(1), st		// 0x0005531f
		fstp	dword ptr [esp + 0x20]		// 0x00055321
		mov	ebx, dword ptr [eax + 0x10]		// 0x00055325
		xor	eax, eax		// 0x00055328
		cmp	ecx, eax		// 0x0005532a
		mov	dword ptr [esp + 0x14], ebx		// 0x0005532c
		mov	dword ptr [esp + 0x40], eax		// 0x00055330
		mov	dword ptr [esp + 0x44], eax		// 0x00055334
		je	L55363		// 0x00055338
		lea	edx, [esp + 0x18]		// 0x0005533a
		push	edx		// 0x0005533e
		call	nxSupportMapLookup		// 0x0005533f
		mov	edx, dword ptr [esp + 0x48]		// 0x00055344
		mov	ecx, dword ptr [edx + 0xc]		// 0x00055348
		movzx	ecx, byte ptr [ecx + eax]		// 0x0005534b
		mov	edx, dword ptr [edx + 0x10]		// 0x0005534f
		mov	dword ptr [esp + 0x40], ecx		// 0x00055352
		movzx	edx, byte ptr [edx + eax]		// 0x00055356
		mov	dword ptr [esp + 0x44], edx		// 0x0005535a
		jmp	L55401		// 0x0005535e
L55363:
		mov	eax, dword ptr [ebp + 0x9c]		// 0x00055363
		mov	ecx, dword ptr [eax + 0x64]		// 0x00055369
		mov	edx, dword ptr [eax + 0x10]		// 0x0005536c
		mov	ebx, dword ptr [esp + 0x34]		// 0x0005536f
		mov	eax, dword ptr [ebx + 8]		// 0x00055373
		mov	dword ptr [esp + 0x48], ecx		// 0x00055376
		push	eax		// 0x0005537a
		mov	ecx, ebx		// 0x0005537b
		mov	dword ptr [esp + 0x14], edx		// 0x0005537d
		call	nxScratchStamp		// 0x00055381
		mov	ecx, dword ptr [esp + 0x4c]		// 0x00055386
		mov	edx, dword ptr [esp + 0x14]		// 0x0005538a
		push	eax		// 0x0005538e
		push	ecx		// 0x0005538f
		push	edx		// 0x00055390
		lea	eax, [esp + 0x28]		// 0x00055391
		push	eax		// 0x00055395
		lea	ecx, [esp + 0x54]		// 0x00055396
		push	ecx		// 0x0005539a
		call	nxHullClimbSupportVertex		// 0x0005539b
		fld	dword ptr [esp + 0x30]		// 0x000553a0
		mov	eax, dword ptr [ebp + 0x9c]		// 0x000553a4
		fchs		// 0x000553aa
		mov	edx, dword ptr [eax + 0x10]		// 0x000553ac
		mov	ebp, dword ptr [eax + 0x64]		// 0x000553af
		fstp	dword ptr [esp + 0x3c]		// 0x000553b2
		fld	dword ptr [esp + 0x34]		// 0x000553b6
		mov	eax, dword ptr [ebx + 8]		// 0x000553ba
		fchs		// 0x000553bd
		add	esp, 0x18		// 0x000553bf
		fstp	dword ptr [esp + 0x28]		// 0x000553c2
		push	eax		// 0x000553c6
		fld	dword ptr [esp + 0x24]		// 0x000553c7
		mov	ecx, ebx		// 0x000553cb
		fchs		// 0x000553cd
		mov	dword ptr [esp + 0x4c], edx		// 0x000553cf
		fstp	dword ptr [esp + 0x30]		// 0x000553d3
		call	nxScratchStamp		// 0x000553d7
		mov	ecx, dword ptr [esp + 0x4c]		// 0x000553dc
		push	eax		// 0x000553e0
		push	ebp		// 0x000553e1
		push	ecx		// 0x000553e2
		lea	edx, [esp + 0x34]		// 0x000553e3
		push	edx		// 0x000553e7
		lea	eax, [esp + 0x58]		// 0x000553e8
		push	eax		// 0x000553ec
		call	nxHullClimbSupportVertex		// 0x000553ed
		mov	ecx, dword ptr [esp + 0x58]		// 0x000553f2
		mov	edx, dword ptr [esp + 0x5c]		// 0x000553f6
		mov	ebx, dword ptr [esp + 0x2c]		// 0x000553fa
		add	esp, 0x18		// 0x000553fe
L55401:
		fld	dword ptr [edi + 0x30]		// 0x00055401
		lea	ecx, [ecx + ecx*2]		// 0x00055404
		fld	dword ptr [edi + 0x34]		// 0x00055407
		lea	eax, [ebx + ecx*4]		// 0x0005540a
		fld	dword ptr [edi + 0x38]		// 0x0005540d
		mov	ecx, dword ptr [esp + 0x38]		// 0x00055410
		fxch	st(2)		// 0x00055414
		lea	edx, [edx + edx*2]		// 0x00055416
		fmul	dword ptr [esi]		// 0x00055419
		pop	edi		// 0x0005541b
		fxch	st(2)		// 0x0005541c
		fmul	dword ptr [esi + 8]		// 0x0005541e
		faddp	st(2), st		// 0x00055421
		fmul	dword ptr [esi + 4]		// 0x00055423
		pop	esi		// 0x00055426
		pop	ebp		// 0x00055427
		faddp	st(1), st		// 0x00055428
		fld	dword ptr [esp + 0x14]		// 0x0005542a
		fmul	dword ptr [eax + 8]		// 0x0005542e
		fld	dword ptr [esp + 0x10]		// 0x00055431
		fmul	dword ptr [eax + 4]		// 0x00055435
		faddp	st(1), st		// 0x00055438
		fld	dword ptr [esp + 0xc]		// 0x0005543a
		fmul	dword ptr [eax]		// 0x0005543e
		lea	eax, [ebx + edx*4]		// 0x00055440
		mov	edx, dword ptr [esp + 0x30]		// 0x00055443
		pop	ebx		// 0x00055447
		faddp	st(1), st		// 0x00055448
		fadd	st, st(1)		// 0x0005544a
		fstp	dword ptr [ecx]		// 0x0005544c
		fld	dword ptr [esp + 0x10]		// 0x0005544e
		fmul	dword ptr [eax + 8]		// 0x00055452
		fld	dword ptr [esp + 0xc]		// 0x00055455
		fmul	dword ptr [eax + 4]		// 0x00055459
		faddp	st(1), st		// 0x0005545c
		fld	dword ptr [esp + 8]		// 0x0005545e
		fmul	dword ptr [eax]		// 0x00055462
		faddp	st(1), st		// 0x00055464
		faddp	st(1), st		// 0x00055466
		fld	st(0)		// 0x00055468
		fstp	dword ptr [edx]		// 0x0005546a
		fcom	dword ptr [ecx]		// 0x0005546c
		fnstsw	ax		// 0x0005546e
		test	ah, 5		// 0x00055470
		jp	L55483		// 0x00055473
		fld	dword ptr [ecx]		// 0x00055475
		fxch	st(1)		// 0x00055477
		fstp	dword ptr [ecx]		// 0x00055479
		fstp	dword ptr [edx]		// 0x0005547b
		add	esp, 0x20		// 0x0005547d
		ret	0x18		// 0x00055480
L55483:
		fstp	st(0)		// 0x00055483
		add	esp, 0x20		// 0x00055485
		ret	0x18		// 0x00055488
		}
	}

#else
#include "portable/TriangleMeshPolygonsScalar.inl"
#endif

// The table at 0x101085d4, in its order.
const void* const gTriangleMeshPolygonTable[12] =
	{
	(const void*) &nxMeshHullCentre,			// 0 002211
	(const void*) &nxMeshHullVertexCount,		// 1 002213
	(const void*) &nxMeshHullVertices,			// 2 002215
	(const void*) &nxMeshHullPolygonCount,		// 3 002221
	(const void*) &nxMeshHullPolygon,			// 4 002223
	(const void*) &nxMeshHullEdgeAxes,			// 5 002225
	(const void*) &nxMeshHullEdges,				// 6 002227
	(const void*) &nxMeshHullEdgeToPolygons,	// 7 002229
	(const void*) &nxMeshHullEdgePolygons,		// 8 002231
	(const void*) &nxMeshHullSupportPolygon,	// 9 002217
	(const void*) &nxMeshHullSupportFace,		// 10 002219
	(const void*) &nxMeshHullProject			// 11 002249
	};
