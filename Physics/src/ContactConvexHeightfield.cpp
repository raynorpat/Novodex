/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Convex against triangle-mesh separating axes: the first half of sub-unit M
// of units/convex-mesh-gap-contract.md (0x00041360..0x000427cf), written by
// convex-mesh gap Task 2h from the Capstone listing. The file name is the one
// the unit's own asserts name (001847 line 583, 001849 line 2594); the contract
// leaves open whether these twelve rows open ContactConvexHeightfield.cpp or
// close the convex/convex file, and puts them here because every caller
// (001844, 001849; Task 2i) is here.
//
// The rows test one convex mesh -- its TriangleMesh polygon interface (the
// table at mesh +0x04, TriangleMeshPolygons.cpp: slot 2 the vertices, 3 the
// polygon count, 4 a polygon, 11 the extent along an axis) and its support map
// (kind C or null) -- against a group of triangles of another TriangleMesh
// (+0x10 the vertices, +0x14 the 32-bit triangles, +0x88 the EdgeList that
// 002188 builds on first use). A group's interval along an axis is its
// vertices' least and greatest projection, each vertex once (001824, the
// scratch record's visited array stamped through 000505). The separating-axis
// search is 001832 (the hull's face normals facing a point: 001830 on their
// precomputed extents, else every face through 001828 / 001826), 001840 (the
// triangle normal through 001833, and the triangles' active edges gathered by
// 001834 into the scratch record's Container at +0x4e0) and 001836 with its
// continuation 001838 (the cross products of the hull's edges near the plane
// with those directions, gathered into +0x4f0 and tested through 001826).
// 001842 hands the chosen polygon and triangle to 001909 (ContactGeneration.cpp),
// which emits through 000875. 001822 casts a ray from inside a hull out
// through its polygons (001708, Geometry.cpp), in the hull's frame when a
// 4x4 pose is given (inverted by the vendored Matrix4x4::Invert, 005197).
//
// Every row is the listing's instructions, naked (branch targets are labels
// named by their oracle RVA), as sub-unit L's are (ContactConvexConvex.cpp):
// the float code keeps values on the x87 stack across narrowing stores, and
// every row takes register arguments that no C++ declaration expresses, so the
// rows are declared without parameters and are only called from naked code
// (their callers, 001844 and 001849, are Task 2i; the harness calls them
// through register thunks). The stack arguments are cleaned by the caller.
//
// x87: on the /arch:IA32 list; /EHs-c- with the other ICE-shaped files.

#include "ContactGeneration.h"
#include "ConvexHull.h"
#include "IceMeshTools.h"
#include "NxGeometryHelpers.h"
#include "TriangleMeshPolygons.h"
#include "TriangleMesh.h"
#include "NxMeshContactHelpers.h"
#include "NxTriangleDistance.h"

// 005197, Matrix4x4::Invert (vendored, thiscall), and 002188,
// TriangleMesh::createEdgeList (TriangleMeshTopology.cpp): reached through
// /alternatename aliases of their decorated names, so the calls stay direct.
extern "C" void nxConvexMeshCallInvert();
#pragma comment(linker, "/alternatename:_nxConvexMeshCallInvert=?Invert@Matrix4x4@IceMaths@@QAEAAV12@XZ")
extern "C" void nxConvexMeshCallCreateEdgeList();
#pragma comment(linker, "/alternatename:_nxConvexMeshCallCreateEdgeList=?createEdgeList@TriangleMesh@@QAEXXZ")
extern "C" void _chkstk();								// 005695, the stack probe

// .rdata 0x101041f0 (0.0f), 0x101041ec (1.0f) and the double 1e-6 at
// 0x10107c58 (shared with 001816).
static const float kConvexMeshZero = 0.0f;
static const float kConvexMeshOne = 1.0f;
static const double kConvexMeshMicro = 1e-6;

// phys_fn_001822 (0x00041360, 717 B)
// A ray from inside a hull out through its polygons (register arguments: ecx
// the origin, eax the direction, ebx the ConvexHull; three stack arguments:
// a 4x4 pose or null, the distance out and the direction out). With a pose,
// a copy is inverted in place (005197) and the origin and direction are taken
// into the hull's frame (the origin with the translation row). The distance
// starts at FLT_MAX (0x7f7fffff); the polygons are built first (001472) when
// the count at +0x24 is 0, and again when the array at +0x28 is null. For each
// polygon, an origin plane value ((o.z n.z + o.y n.y) + o.x n.x + d) greater
// than 0.0f (`test ah, 0x41; je`: NaN is not) returns false at once; otherwise
// the ray is tested against the polygon's fan (001708 on its count, the hull's
// vertices at +0x10 and its references), and a hit nearer than the distance
// (`test ah, 5; jp`) stores the distance and the ray's direction (as the ray on
// the stack holds it after 001708). True when the distance is no longer
// FLT_MAX.
__declspec(naked) bool nxConvexMeshRay()
	{
	__asm
		{
		sub	esp, 0x8c		// 0x00041360
		mov	edx, dword ptr [ecx]		// 0x00041366
		mov	dword ptr [esp], edx		// 0x00041368
		mov	edx, dword ptr [ecx + 4]		// 0x0004136b
		mov	ecx, dword ptr [ecx + 8]		// 0x0004136e
		push	ebp		// 0x00041371
		mov	dword ptr [esp + 8], edx		// 0x00041372
		mov	edx, dword ptr [eax]		// 0x00041376
		push	esi		// 0x00041378
		mov	esi, dword ptr [esp + 0x98]		// 0x00041379
		mov	dword ptr [esp + 0x10], ecx		// 0x00041380
		mov	ecx, dword ptr [eax + 4]		// 0x00041384
		mov	dword ptr [esp + 0x14], edx		// 0x00041387
		mov	edx, dword ptr [eax + 8]		// 0x0004138b
		xor	ebp, ebp		// 0x0004138e
		cmp	esi, ebp		// 0x00041390
		push	edi		// 0x00041392
		mov	dword ptr [esp + 0x1c], ecx		// 0x00041393
		mov	dword ptr [esp + 0x20], edx		// 0x00041397
		je	L414e5		// 0x0004139b
		mov	ecx, 0x10		// 0x000413a1
		lea	edi, [esp + 0x54]		// 0x000413a6
		rep movsd		// 0x000413aa
		lea	ecx, [esp + 0x54]		// 0x000413ac
		call	nxConvexMeshCallInvert		// 0x000413b0
		mov	ecx, dword ptr [esp + 0x58]		// 0x000413b5
		fld	dword ptr [esp + 0x68]		// 0x000413b9
		mov	eax, dword ptr [esp + 0x54]		// 0x000413bd
		fmul	dword ptr [esp + 0x10]		// 0x000413c1
		mov	edx, dword ptr [esp + 0x5c]		// 0x000413c5
		fld	dword ptr [esp + 0x78]		// 0x000413c9
		mov	dword ptr [esp + 0x34], ecx		// 0x000413cd
		fmul	dword ptr [esp + 0x14]		// 0x000413d1
		mov	ecx, dword ptr [esp + 0x68]		// 0x000413d5
		mov	dword ptr [esp + 0x30], eax		// 0x000413d9
		mov	eax, dword ptr [esp + 0x64]		// 0x000413dd
		faddp	st(1), st		// 0x000413e1
		mov	dword ptr [esp + 0x38], edx		// 0x000413e3
		fld	dword ptr [esp + 0x58]		// 0x000413e7
		mov	edx, dword ptr [esp + 0x6c]		// 0x000413eb
		fmul	dword ptr [esp + 0xc]		// 0x000413ef
		mov	dword ptr [esp + 0x40], ecx		// 0x000413f3
		mov	ecx, dword ptr [esp + 0x78]		// 0x000413f7
		mov	dword ptr [esp + 0x3c], eax		// 0x000413fb
		faddp	st(1), st		// 0x000413ff
		mov	eax, dword ptr [esp + 0x74]		// 0x00041401
		mov	dword ptr [esp + 0x44], edx		// 0x00041405
		mov	edx, dword ptr [esp + 0x7c]		// 0x00041409
		fadd	dword ptr [esp + 0x88]		// 0x0004140d
		mov	dword ptr [esp + 0x4c], ecx		// 0x00041414
		fld	dword ptr [esp + 0x6c]		// 0x00041418
		mov	ecx, 9		// 0x0004141c
		fmul	dword ptr [esp + 0x10]		// 0x00041421
		lea	esi, [esp + 0x30]		// 0x00041425
		fld	dword ptr [esp + 0x7c]		// 0x00041429
		lea	edi, [esp + 0x54]		// 0x0004142d
		fmul	dword ptr [esp + 0x14]		// 0x00041431
		mov	dword ptr [esp + 0x48], eax		// 0x00041435
		mov	dword ptr [esp + 0x50], edx		// 0x00041439
		faddp	st(1), st		// 0x0004143d
		fld	dword ptr [esp + 0x5c]		// 0x0004143f
		fmul	dword ptr [esp + 0xc]		// 0x00041443
		faddp	st(1), st		// 0x00041447
		fadd	dword ptr [esp + 0x8c]		// 0x00041449
		fld	dword ptr [esp + 0x64]		// 0x00041450
		fmul	dword ptr [esp + 0x10]		// 0x00041454
		fld	dword ptr [esp + 0x74]		// 0x00041458
		fmul	dword ptr [esp + 0x14]		// 0x0004145c
		faddp	st(1), st		// 0x00041460
		fld	dword ptr [esp + 0x54]		// 0x00041462
		fmul	dword ptr [esp + 0xc]		// 0x00041466
		faddp	st(1), st		// 0x0004146a
		fadd	dword ptr [esp + 0x84]		// 0x0004146c
		rep movsd		// 0x00041473
		fstp	dword ptr [esp + 0xc]		// 0x00041475
		fxch	st(1)		// 0x00041479
		fstp	dword ptr [esp + 0x10]		// 0x0004147b
		fstp	dword ptr [esp + 0x14]		// 0x0004147f
		fld	dword ptr [esp + 0x70]		// 0x00041483
		fmul	dword ptr [esp + 0x20]		// 0x00041487
		fld	dword ptr [esp + 0x64]		// 0x0004148b
		fmul	dword ptr [esp + 0x1c]		// 0x0004148f
		faddp	st(1), st		// 0x00041493
		fld	dword ptr [esp + 0x58]		// 0x00041495
		fmul	dword ptr [esp + 0x18]		// 0x00041499
		faddp	st(1), st		// 0x0004149d
		fld	dword ptr [esp + 0x74]		// 0x0004149f
		fmul	dword ptr [esp + 0x20]		// 0x000414a3
		fld	dword ptr [esp + 0x68]		// 0x000414a7
		fmul	dword ptr [esp + 0x1c]		// 0x000414ab
		faddp	st(1), st		// 0x000414af
		fld	dword ptr [esp + 0x5c]		// 0x000414b1
		fmul	dword ptr [esp + 0x18]		// 0x000414b5
		faddp	st(1), st		// 0x000414b9
		fld	dword ptr [esp + 0x6c]		// 0x000414bb
		fmul	dword ptr [esp + 0x20]		// 0x000414bf
		fld	dword ptr [esp + 0x60]		// 0x000414c3
		fmul	dword ptr [esp + 0x1c]		// 0x000414c7
		faddp	st(1), st		// 0x000414cb
		fld	dword ptr [esp + 0x54]		// 0x000414cd
		fmul	dword ptr [esp + 0x18]		// 0x000414d1
		faddp	st(1), st		// 0x000414d5
		fstp	dword ptr [esp + 0x18]		// 0x000414d7
		fxch	st(1)		// 0x000414db
		fstp	dword ptr [esp + 0x1c]		// 0x000414dd
		fstp	dword ptr [esp + 0x20]		// 0x000414e1
L414e5:
		mov	eax, dword ptr [esp + 0xa0]		// 0x000414e5
		mov	dword ptr [eax], 0x7f7fffff		// 0x000414ec
		cmp	dword ptr [ebx + 0x24], ebp		// 0x000414f2
		jne	L414fe		// 0x000414f5
		mov	ecx, ebx		// 0x000414f7
		call	nxHullComputePolygons		// 0x000414f9
L414fe:
		mov	eax, dword ptr [ebx + 0x24]		// 0x000414fe
		cmp	eax, ebp		// 0x00041501
		mov	dword ptr [esp + 0x94], eax		// 0x00041503
		mov	dword ptr [esp + 0x2c], ebp		// 0x0004150a
		jbe	L41606		// 0x0004150e
		mov	esi, dword ptr [esp + 0x10]		// 0x00041514
		mov	edi, dword ptr [esp + 0x14]		// 0x00041518
		mov	dword ptr [esp + 0x24], ebp		// 0x0004151c
		mov	ebp, dword ptr [esp + 0xc]		// 0x00041520
L41524:
		mov	eax, dword ptr [ebx + 0x28]		// 0x00041524
		test	eax, eax		// 0x00041527
		jne	L41532		// 0x00041529
		mov	ecx, ebx		// 0x0004152b
		call	nxHullComputePolygons		// 0x0004152d
L41532:
		mov	eax, dword ptr [esp + 0x24]		// 0x00041532
		fld	dword ptr [esp + 0x14]		// 0x00041536
		mov	ecx, dword ptr [ebx + 0x28]		// 0x0004153a
		fmul	dword ptr [ecx + eax + 0x14]		// 0x0004153d
		add	ecx, eax		// 0x00041541
		fld	dword ptr [esp + 0x10]		// 0x00041543
		fmul	dword ptr [ecx + 0x10]		// 0x00041547
		faddp	st(1), st		// 0x0004154a
		fld	dword ptr [esp + 0xc]		// 0x0004154c
		fmul	dword ptr [ecx + 0xc]		// 0x00041550
		faddp	st(1), st		// 0x00041553
		fadd	dword ptr [ecx + 0x18]		// 0x00041555
		fcomp	dword ptr kConvexMeshZero		// 0x00041558
		fnstsw	ax		// 0x0004155e
		test	ah, 0x41		// 0x00041560
		je	L41615		// 0x00041563
		mov	edx, dword ptr [esp + 0x18]		// 0x00041569
		mov	eax, dword ptr [esp + 0x1c]		// 0x0004156d
		mov	dword ptr [esp + 0x3c], edx		// 0x00041571
		mov	edx, dword ptr [esp + 0x20]		// 0x00041575
		mov	dword ptr [esp + 0x44], edx		// 0x00041579
		lea	edx, [esp + 0x28]		// 0x0004157d
		push	edx		// 0x00041581
		mov	dword ptr [esp + 0x44], eax		// 0x00041582
		mov	eax, dword ptr [ebx + 0x10]		// 0x00041586
		lea	edx, [esp + 0x34]		// 0x00041589
		push	edx		// 0x0004158d
		mov	dword ptr [esp + 0x38], ebp		// 0x0004158e
		mov	dword ptr [esp + 0x3c], esi		// 0x00041592
		mov	dword ptr [esp + 0x40], edi		// 0x00041596
		mov	edx, dword ptr [ecx + 4]		// 0x0004159a
		push	edx		// 0x0004159d
		push	eax		// 0x0004159e
		mov	eax, dword ptr [ecx]		// 0x0004159f
		push	eax		// 0x000415a1
		call	NxRayInflatedTriangleFan		// 0x000415a2
		add	esp, 0x14		// 0x000415a7
		test	al, al		// 0x000415aa
		je	L415e3		// 0x000415ac
		fld	dword ptr [esp + 0x28]		// 0x000415ae
		mov	ecx, dword ptr [esp + 0xa0]		// 0x000415b2
		fcomp	dword ptr [ecx]		// 0x000415b9
		fnstsw	ax		// 0x000415bb
		test	ah, 5		// 0x000415bd
		jp	L415e3		// 0x000415c0
		mov	edx, dword ptr [esp + 0x28]		// 0x000415c2
		mov	eax, dword ptr [esp + 0xa4]		// 0x000415c6
		mov	dword ptr [ecx], edx		// 0x000415cd
		mov	ecx, dword ptr [esp + 0x3c]		// 0x000415cf
		mov	edx, dword ptr [esp + 0x40]		// 0x000415d3
		mov	dword ptr [eax], ecx		// 0x000415d7
		mov	ecx, dword ptr [esp + 0x44]		// 0x000415d9
		mov	dword ptr [eax + 4], edx		// 0x000415dd
		mov	dword ptr [eax + 8], ecx		// 0x000415e0
L415e3:
		mov	eax, dword ptr [esp + 0x2c]		// 0x000415e3
		mov	edx, dword ptr [esp + 0x24]		// 0x000415e7
		mov	ecx, dword ptr [esp + 0x94]		// 0x000415eb
		inc	eax		// 0x000415f2
		add	edx, 0x24		// 0x000415f3
		cmp	eax, ecx		// 0x000415f6
		mov	dword ptr [esp + 0x2c], eax		// 0x000415f8
		mov	dword ptr [esp + 0x24], edx		// 0x000415fc
		jb	L41524		// 0x00041600
L41606:
		mov	edx, dword ptr [esp + 0xa0]		// 0x00041606
		cmp	dword ptr [edx], 0x7f7fffff		// 0x0004160d
		jne	L41621		// 0x00041613
L41615:
		pop	edi		// 0x00041615
		pop	esi		// 0x00041616
		xor	al, al		// 0x00041617
		pop	ebp		// 0x00041619
		add	esp, 0x8c		// 0x0004161a
		ret		// 0x00041620
L41621:
		pop	edi		// 0x00041621
		pop	esi		// 0x00041622
		mov	al, 1		// 0x00041623
		pop	ebp		// 0x00041625
		add	esp, 0x8c		// 0x00041626
		ret		// 0x0004162c
		}
	}

// phys_fn_001824 (0x00041630, 348 B)
// The interval of a group of triangles along an axis (register arguments: ebx
// the least out, esi the axis, edi the mesh; four stack arguments: the scratch
// record, the greatest out, the triangle count and the triangle indices). The
// least starts at FLT_MAX and the greatest at -FLT_MAX (0xff7fffff); a fresh
// stamp (000505) marks each vertex in the scratch record's array (+0x08) the
// first time one of the triangles (mesh +0x14, three 32-bit indices) reaches
// it, and only then is it projected ((v.z a.z + v.y a.y) + v.x a.x, mesh
// +0x10): stored as the least when below it (`fcom; test ah, 5; jp`: NaN is
// not) and as the greatest when above it (`test ah, 0x41; jne`). The indices
// are not checked.
__declspec(naked) void nxConvexMeshProject()
	{
	__asm
		{
		mov	eax, dword ptr [esp + 8]		// 0x00041630
		mov	ecx, dword ptr [esp + 4]		// 0x00041634
		push	ebp		// 0x00041638
		mov	ebp, dword ptr [esp + 0x10]		// 0x00041639
		mov	dword ptr [ebx], 0x7f7fffff		// 0x0004163d
		mov	dword ptr [eax], 0xff7fffff		// 0x00041643
		call	nxScratchStamp		// 0x00041649
		test	ebp, ebp		// 0x0004164e
		mov	edx, eax		// 0x00041650
		je	L4178a		// 0x00041652
		mov	dword ptr [esp + 0x10], ebp		// 0x00041658
		_emit	0x8d
		_emit	0x64
		_emit	0x24
		_emit	0x00		// 0x0004165c lea esp, [esp]
L41660:
		mov	ebp, dword ptr [esp + 0x14]		// 0x00041660
		mov	eax, dword ptr [ebp]		// 0x00041664
		add	ebp, 4		// 0x00041667
		lea	ecx, [eax + eax*2]		// 0x0004166a
		mov	eax, dword ptr [edi + 0x14]		// 0x0004166d
		mov	dword ptr [esp + 0x14], ebp		// 0x00041670
		mov	ebp, dword ptr [esp + 8]		// 0x00041674
		mov	ebp, dword ptr [ebp + 8]		// 0x00041678
		lea	ecx, [eax + ecx*4]		// 0x0004167b
		mov	eax, dword ptr [ecx]		// 0x0004167e
		lea	eax, [ebp + eax*4]		// 0x00041680
		mov	ebp, dword ptr [eax]		// 0x00041684
		sub	ebp, edx		// 0x00041686
		je	L416cf		// 0x00041688
		mov	dword ptr [eax], edx		// 0x0004168a
		mov	eax, dword ptr [ecx]		// 0x0004168c
		mov	ebp, dword ptr [edi + 0x10]		// 0x0004168e
		lea	eax, [eax + eax*2]		// 0x00041691
		fld	dword ptr [ebp + eax*4 + 8]		// 0x00041694
		lea	eax, [ebp + eax*4]		// 0x00041698
		fmul	dword ptr [esi + 8]		// 0x0004169c
		fld	dword ptr [eax + 4]		// 0x0004169f
		fmul	dword ptr [esi + 4]		// 0x000416a2
		faddp	st(1), st		// 0x000416a5
		fld	dword ptr [eax]		// 0x000416a7
		fmul	dword ptr [esi]		// 0x000416a9
		faddp	st(1), st		// 0x000416ab
		fcom	dword ptr [ebx]		// 0x000416ad
		fnstsw	ax		// 0x000416af
		test	ah, 5		// 0x000416b1
		jp	L416b8		// 0x000416b4
		fst	dword ptr [ebx]		// 0x000416b6
L416b8:
		mov	ebp, dword ptr [esp + 0xc]		// 0x000416b8
		fld	st(0)		// 0x000416bc
		fcomp	dword ptr [ebp]		// 0x000416be
		fnstsw	ax		// 0x000416c1
		test	ah, 0x41		// 0x000416c3
		jne	L416cd		// 0x000416c6
		fstp	dword ptr [ebp]		// 0x000416c8
		jmp	L416cf		// 0x000416cb
L416cd:
		fstp	st(0)		// 0x000416cd
L416cf:
		mov	ebp, dword ptr [esp + 8]		// 0x000416cf
		mov	ebp, dword ptr [ebp + 8]		// 0x000416d3
		mov	eax, dword ptr [ecx + 4]		// 0x000416d6
		lea	eax, [ebp + eax*4]		// 0x000416d9
		mov	ebp, dword ptr [eax]		// 0x000416dd
		sub	ebp, edx		// 0x000416df
		je	L41729		// 0x000416e1
		mov	dword ptr [eax], edx		// 0x000416e3
		mov	eax, dword ptr [ecx + 4]		// 0x000416e5
		mov	ebp, dword ptr [edi + 0x10]		// 0x000416e8
		lea	eax, [eax + eax*2]		// 0x000416eb
		fld	dword ptr [ebp + eax*4 + 8]		// 0x000416ee
		lea	eax, [ebp + eax*4]		// 0x000416f2
		fmul	dword ptr [esi + 8]		// 0x000416f6
		fld	dword ptr [eax + 4]		// 0x000416f9
		fmul	dword ptr [esi + 4]		// 0x000416fc
		faddp	st(1), st		// 0x000416ff
		fld	dword ptr [eax]		// 0x00041701
		fmul	dword ptr [esi]		// 0x00041703
		faddp	st(1), st		// 0x00041705
		fcom	dword ptr [ebx]		// 0x00041707
		fnstsw	ax		// 0x00041709
		test	ah, 5		// 0x0004170b
		jp	L41712		// 0x0004170e
		fst	dword ptr [ebx]		// 0x00041710
L41712:
		mov	ebp, dword ptr [esp + 0xc]		// 0x00041712
		fld	st(0)		// 0x00041716
		fcomp	dword ptr [ebp]		// 0x00041718
		fnstsw	ax		// 0x0004171b
		test	ah, 0x41		// 0x0004171d
		jne	L41727		// 0x00041720
		fstp	dword ptr [ebp]		// 0x00041722
		jmp	L41729		// 0x00041725
L41727:
		fstp	st(0)		// 0x00041727
L41729:
		mov	ebp, dword ptr [esp + 8]		// 0x00041729
		mov	ebp, dword ptr [ebp + 8]		// 0x0004172d
		mov	eax, dword ptr [ecx + 8]		// 0x00041730
		lea	eax, [ebp + eax*4]		// 0x00041733
		mov	ebp, dword ptr [eax]		// 0x00041737
		sub	ebp, edx		// 0x00041739
		je	L41780		// 0x0004173b
		mov	dword ptr [eax], edx		// 0x0004173d
		mov	ecx, dword ptr [ecx + 8]		// 0x0004173f
		mov	eax, dword ptr [edi + 0x10]		// 0x00041742
		lea	ecx, [ecx + ecx*2]		// 0x00041745
		fld	dword ptr [eax + ecx*4 + 8]		// 0x00041748
		lea	eax, [eax + ecx*4]		// 0x0004174c
		fmul	dword ptr [esi + 8]		// 0x0004174f
		fld	dword ptr [eax + 4]		// 0x00041752
		fmul	dword ptr [esi + 4]		// 0x00041755
		faddp	st(1), st		// 0x00041758
		fld	dword ptr [eax]		// 0x0004175a
		fmul	dword ptr [esi]		// 0x0004175c
		faddp	st(1), st		// 0x0004175e
		fcom	dword ptr [ebx]		// 0x00041760
		fnstsw	ax		// 0x00041762
		test	ah, 5		// 0x00041764
		jp	L4176b		// 0x00041767
		fst	dword ptr [ebx]		// 0x00041769
L4176b:
		mov	ecx, dword ptr [esp + 0xc]		// 0x0004176b
		fld	st(0)		// 0x0004176f
		fcomp	dword ptr [ecx]		// 0x00041771
		fnstsw	ax		// 0x00041773
		test	ah, 0x41		// 0x00041775
		jne	L4177e		// 0x00041778
		fstp	dword ptr [ecx]		// 0x0004177a
		jmp	L41780		// 0x0004177c
L4177e:
		fstp	st(0)		// 0x0004177e
L41780:
		dec	dword ptr [esp + 0x10]		// 0x00041780
		jne	L41660		// 0x00041784
L4178a:
		pop	ebp		// 0x0004178a
		ret		// 0x0004178b
		}
	}

// phys_fn_001826 (0x00041790, 166 B)
// One axis: the hull against a group of triangles (register arguments: ecx the
// hull's polygon interface, eax the axis, edx the support map for slot 11; six
// stack arguments: the scratch record, the triangle count and indices, the
// mesh, the hull's pose and the depth out or null). The hull's extent along
// the axis comes from slot 11 and the triangles' from 001824; false when
// either interval lies wholly beyond the other (`test ah, 5; jnp`: a NaN bound
// does not separate); otherwise, when the depth pointer is given, the smaller
// of the two overlaps (the second kept unless the first is below it) is
// stored, and true.
__declspec(naked) bool nxConvexMeshAxis()
	{
	__asm
		{
		sub	esp, 0x10		// 0x00041790
		push	ebx		// 0x00041793
		push	ebp		// 0x00041794
		mov	ebp, dword ptr [esp + 0x30]		// 0x00041795
		push	esi		// 0x00041799
		push	edi		// 0x0004179a
		mov	edi, dword ptr [esp + 0x24]		// 0x0004179b
		push	edx		// 0x0004179f
		mov	edx, dword ptr [esp + 0x38]		// 0x000417a0
		push	edx		// 0x000417a4
		mov	esi, eax		// 0x000417a5
		mov	eax, dword ptr [ecx]		// 0x000417a7
		push	esi		// 0x000417a9
		lea	edx, [esp + 0x1c]		// 0x000417aa
		push	edx		// 0x000417ae
		lea	edx, [esp + 0x2c]		// 0x000417af
		push	edx		// 0x000417b3
		push	edi		// 0x000417b4
		call	dword ptr [eax + 0x2c]		// 0x000417b5
		mov	eax, dword ptr [esp + 0x2c]		// 0x000417b8
		mov	ecx, dword ptr [esp + 0x28]		// 0x000417bc
		push	eax		// 0x000417c0
		push	ecx		// 0x000417c1
		lea	edx, [esp + 0x20]		// 0x000417c2
		push	edx		// 0x000417c6
		push	edi		// 0x000417c7
		mov	edi, dword ptr [esp + 0x40]		// 0x000417c8
		lea	ebx, [esp + 0x24]		// 0x000417cc
		call	nxConvexMeshProject		// 0x000417d0
		fld	dword ptr [esp + 0x20]		// 0x000417d5
		fcomp	dword ptr [esp + 0x24]		// 0x000417d9
		add	esp, 0x10		// 0x000417dd
		fnstsw	ax		// 0x000417e0
		test	ah, 5		// 0x000417e2
		jnp	L4182c		// 0x000417e5
		fld	dword ptr [esp + 0x18]		// 0x000417e7
		fcomp	dword ptr [esp + 0x1c]		// 0x000417eb
		fnstsw	ax		// 0x000417ef
		test	ah, 5		// 0x000417f1
		jnp	L4182c		// 0x000417f4
		test	ebp, ebp		// 0x000417f6
		je	L41822		// 0x000417f8
		fld	dword ptr [esp + 0x10]		// 0x000417fa
		fsub	dword ptr [esp + 0x14]		// 0x000417fe
		fld	dword ptr [esp + 0x18]		// 0x00041802
		fsub	dword ptr [esp + 0x1c]		// 0x00041806
		fstp	dword ptr [esp + 0x1c]		// 0x0004180a
		fcom	dword ptr [esp + 0x1c]		// 0x0004180e
		fnstsw	ax		// 0x00041812
		test	ah, 5		// 0x00041814
		jnp	L4181f		// 0x00041817
		fstp	st(0)		// 0x00041819
		fld	dword ptr [esp + 0x1c]		// 0x0004181b
L4181f:
		fstp	dword ptr [ebp]		// 0x0004181f
L41822:
		pop	edi		// 0x00041822
		pop	esi		// 0x00041823
		pop	ebp		// 0x00041824
		mov	al, 1		// 0x00041825
		pop	ebx		// 0x00041827
		add	esp, 0x10		// 0x00041828
		ret		// 0x0004182b
L4182c:
		pop	edi		// 0x0004182c
		pop	esi		// 0x0004182d
		pop	ebp		// 0x0004182e
		xor	al, al		// 0x0004182f
		pop	ebx		// 0x00041831
		add	esp, 0x10		// 0x00041832
		ret		// 0x00041835
		}
	}

// phys_fn_001828 (0x00041840, 235 B)
// Every face normal of the hull as an axis (register arguments: edi the
// polygon interface, esi the 4x4 that takes the hull's normals into the mesh's
// frame; stack arguments: the scratch record, the support map, the triangle
// count and indices, the mesh, the best depth, the best axis and the best
// index). For each polygon (slots 3 and 4) the normal (polygon +0x0c) is
// rotated by the 4x4's first three rows and tested with 001826; a separating
// axis returns false at once, and a depth below the best (`test ah, 5; jp`)
// stores the depth, the rotated normal and the polygon's index. True when no
// face separates.
__declspec(naked) bool nxConvexMeshFaceAxesAll()
	{
	__asm
		{
		mov	eax, dword ptr [edi]		// 0x00041840
		sub	esp, 0x10		// 0x00041842
		push	ebx		// 0x00041845
		push	ebp		// 0x00041846
		mov	ecx, edi		// 0x00041847
		call	dword ptr [eax + 0xc]		// 0x00041849
		mov	ebp, eax		// 0x0004184c
		xor	ebx, ebx		// 0x0004184e
		test	ebp, ebp		// 0x00041850
		jbe	L4191b		// 0x00041852
L41858:
		mov	edx, dword ptr [edi]		// 0x00041858
		push	ebx		// 0x0004185a
		mov	ecx, edi		// 0x0004185b
		call	dword ptr [edx + 0x10]		// 0x0004185d
		fld	dword ptr [esi + 0x20]		// 0x00041860
		fmul	dword ptr [eax + 0x14]		// 0x00041863
		add	eax, 0xc		// 0x00041866
		fld	dword ptr [eax]		// 0x00041869
		mov	ecx, dword ptr [esp + 0x2c]		// 0x0004186b
		fmul	dword ptr [esi]		// 0x0004186f
		mov	edx, dword ptr [esp + 0x28]		// 0x00041871
		faddp	st(1), st		// 0x00041875
		fld	dword ptr [esi + 0x10]		// 0x00041877
		fmul	dword ptr [eax + 4]		// 0x0004187a
		faddp	st(1), st		// 0x0004187d
		fstp	dword ptr [esp + 0xc]		// 0x0004187f
		fld	dword ptr [esi + 0x24]		// 0x00041883
		fmul	dword ptr [eax + 8]		// 0x00041886
		fld	dword ptr [esi + 0x14]		// 0x00041889
		fmul	dword ptr [eax + 4]		// 0x0004188c
		faddp	st(1), st		// 0x0004188f
		fld	dword ptr [eax]		// 0x00041891
		fmul	dword ptr [esi + 4]		// 0x00041893
		faddp	st(1), st		// 0x00041896
		fstp	dword ptr [esp + 0x10]		// 0x00041898
		fld	dword ptr [eax + 4]		// 0x0004189c
		fmul	dword ptr [esi + 0x18]		// 0x0004189f
		fld	dword ptr [eax + 8]		// 0x000418a2
		fmul	dword ptr [esi + 0x28]		// 0x000418a5
		faddp	st(1), st		// 0x000418a8
		fld	dword ptr [esi + 8]		// 0x000418aa
		fmul	dword ptr [eax]		// 0x000418ad
		lea	eax, [esp + 8]		// 0x000418af
		push	eax		// 0x000418b3
		mov	eax, dword ptr [esp + 0x28]		// 0x000418b4
		push	esi		// 0x000418b8
		faddp	st(1), st		// 0x000418b9
		push	ecx		// 0x000418bb
		mov	ecx, dword ptr [esp + 0x28]		// 0x000418bc
		push	edx		// 0x000418c0
		fstp	dword ptr [esp + 0x24]		// 0x000418c1
		mov	edx, dword ptr [esp + 0x30]		// 0x000418c5
		push	eax		// 0x000418c9
		push	ecx		// 0x000418ca
		mov	ecx, edi		// 0x000418cb
		lea	eax, [esp + 0x24]		// 0x000418cd
		call	nxConvexMeshAxis		// 0x000418d1
		add	esp, 0x18		// 0x000418d6
		test	al, al		// 0x000418d9
		je	L41923		// 0x000418db
		fld	dword ptr [esp + 8]		// 0x000418dd
		mov	ecx, dword ptr [esp + 0x30]		// 0x000418e1
		fcomp	dword ptr [ecx]		// 0x000418e5
		fnstsw	ax		// 0x000418e7
		test	ah, 5		// 0x000418e9
		jp	L41912		// 0x000418ec
		mov	edx, dword ptr [esp + 8]		// 0x000418ee
		mov	eax, dword ptr [esp + 0x34]		// 0x000418f2
		mov	dword ptr [ecx], edx		// 0x000418f6
		mov	ecx, dword ptr [esp + 0xc]		// 0x000418f8
		mov	edx, dword ptr [esp + 0x10]		// 0x000418fc
		mov	dword ptr [eax], ecx		// 0x00041900
		mov	ecx, dword ptr [esp + 0x14]		// 0x00041902
		mov	dword ptr [eax + 4], edx		// 0x00041906
		mov	edx, dword ptr [esp + 0x38]		// 0x00041909
		mov	dword ptr [eax + 8], ecx		// 0x0004190d
		mov	dword ptr [edx], ebx		// 0x00041910
L41912:
		inc	ebx		// 0x00041912
		cmp	ebx, ebp		// 0x00041913
		jb	L41858		// 0x00041915
L4191b:
		pop	ebp		// 0x0004191b
		mov	al, 1		// 0x0004191c
		pop	ebx		// 0x0004191e
		add	esp, 0x10		// 0x0004191f
		ret		// 0x00041922
L41923:
		pop	ebp		// 0x00041923
		xor	al, al		// 0x00041924
		pop	ebx		// 0x00041926
		add	esp, 0x10		// 0x00041927
		ret		// 0x0004192a
		}
	}

// phys_fn_001830 (0x00041930, 123 B)
// One axis on a given hull interval (register arguments: ecx the triangle
// count, eax the triangle indices, esi the axis, edi the mesh; stack
// arguments: the scratch record, the interval's least and greatest, and the
// depth out or null). 001824 gives the triangles' interval; false when the two
// are disjoint (`test ah, 5; jnp`), otherwise the smaller overlap is stored
// when asked for, and true.
__declspec(naked) bool nxConvexMeshInterval()
	{
	__asm
		{
		sub	esp, 8		// 0x00041930
		push	ebx		// 0x00041933
		push	ebp		// 0x00041934
		mov	ebp, dword ptr [esp + 0x20]		// 0x00041935
		push	eax		// 0x00041939
		mov	eax, dword ptr [esp + 0x18]		// 0x0004193a
		push	ecx		// 0x0004193e
		lea	edx, [esp + 0x14]		// 0x0004193f
		push	edx		// 0x00041943
		push	eax		// 0x00041944
		lea	ebx, [esp + 0x18]		// 0x00041945
		call	nxConvexMeshProject		// 0x00041949
		fld	dword ptr [esp + 0x2c]		// 0x0004194e
		fcomp	dword ptr [esp + 0x18]		// 0x00041952
		add	esp, 0x10		// 0x00041956
		fnstsw	ax		// 0x00041959
		test	ah, 5		// 0x0004195b
		jnp	L419a3		// 0x0004195e
		fld	dword ptr [esp + 0xc]		// 0x00041960
		fcomp	dword ptr [esp + 0x18]		// 0x00041964
		fnstsw	ax		// 0x00041968
		test	ah, 5		// 0x0004196a
		jnp	L419a3		// 0x0004196d
		test	ebp, ebp		// 0x0004196f
		je	L4199b		// 0x00041971
		fld	dword ptr [esp + 0x1c]		// 0x00041973
		fsub	dword ptr [esp + 8]		// 0x00041977
		fld	dword ptr [esp + 0xc]		// 0x0004197b
		fsub	dword ptr [esp + 0x18]		// 0x0004197f
		fstp	dword ptr [esp + 0x1c]		// 0x00041983
		fcom	dword ptr [esp + 0x1c]		// 0x00041987
		fnstsw	ax		// 0x0004198b
		test	ah, 5		// 0x0004198d
		jnp	L41998		// 0x00041990
		fstp	st(0)		// 0x00041992
		fld	dword ptr [esp + 0x1c]		// 0x00041994
L41998:
		fstp	dword ptr [ebp]		// 0x00041998
L4199b:
		pop	ebp		// 0x0004199b
		mov	al, 1		// 0x0004199c
		pop	ebx		// 0x0004199e
		add	esp, 8		// 0x0004199f
		ret		// 0x000419a2
L419a3:
		pop	ebp		// 0x000419a3
		xor	al, al		// 0x000419a4
		pop	ebx		// 0x000419a6
		add	esp, 8		// 0x000419a7
		ret		// 0x000419aa
		}
	}

// phys_fn_001832 (0x000419b0, 496 B)
// The hull's face normals that face a point (twelve stack arguments the caller
// cleans: the scratch record, the point in the hull's frame, the polygon
// interface, the support map, the triangle count and indices, the mesh, the
// best depth, the best axis, the best index (set to -1 first), the facing
// polygons out and their count out; ebx the 4x4 taking the hull into the
// mesh's frame). A polygon whose plane value at the point is not below 0.0f
// (NaN included) is listed, its normal rotated by ebx, its extents (polygon
// +0x1c and +0x20) moved by the translation's projection, and tested with
// 001830; a separating axis returns false, a smaller depth is kept. When no
// polygon faced the point (the index still -1), every face is tried with
// 001828 and, when the list pointer is given, every polygon is listed.
__declspec(naked) bool nxConvexMeshFaceAxes()
	{
	__asm
		{
		sub	esp, 0x24		// 0x000419b0
		mov	eax, dword ptr [esp + 0x4c]		// 0x000419b3
		push	ebp		// 0x000419b7
		mov	ebp, dword ptr [esp + 0x54]		// 0x000419b8
		mov	dword ptr [eax], 0xffffffff		// 0x000419bc
		mov	ecx, dword ptr [ebx + 0x30]		// 0x000419c2
		mov	edx, dword ptr [ebx + 0x34]		// 0x000419c5
		mov	eax, dword ptr [ebx + 0x38]		// 0x000419c8
		push	esi		// 0x000419cb
		push	edi		// 0x000419cc
		mov	edi, dword ptr [esp + 0x3c]		// 0x000419cd
		mov	dword ptr [esp + 0x24], ecx		// 0x000419d1
		mov	dword ptr [esp + 0x28], edx		// 0x000419d5
		mov	edx, dword ptr [edi]		// 0x000419d9
		mov	ecx, edi		// 0x000419db
		mov	dword ptr [esp + 0x2c], eax		// 0x000419dd
		call	dword ptr [edx + 0xc]		// 0x000419e1
		xor	esi, esi		// 0x000419e4
		test	eax, eax		// 0x000419e6
		mov	dword ptr [esp + 0x14], eax		// 0x000419e8
		mov	dword ptr [esp + 0xc], esi		// 0x000419ec
		jbe	L41b24		// 0x000419f0
L419f6:
		mov	eax, dword ptr [edi]		// 0x000419f6
		push	esi		// 0x000419f8
		mov	ecx, edi		// 0x000419f9
		call	dword ptr [eax + 0x10]		// 0x000419fb
		mov	ecx, eax		// 0x000419fe
		mov	eax, dword ptr [esp + 0x38]		// 0x00041a00
		fld	dword ptr [ecx + 0x10]		// 0x00041a04
		fmul	dword ptr [eax + 4]		// 0x00041a07
		fld	dword ptr [eax]		// 0x00041a0a
		fmul	dword ptr [ecx + 0xc]		// 0x00041a0c
		faddp	st(1), st		// 0x00041a0f
		fld	dword ptr [eax + 8]		// 0x00041a11
		fmul	dword ptr [ecx + 0x14]		// 0x00041a14
		faddp	st(1), st		// 0x00041a17
		fadd	dword ptr [ecx + 0x18]		// 0x00041a19
		fcomp	dword ptr kConvexMeshZero		// 0x00041a1c
		fnstsw	ax		// 0x00041a22
		test	ah, 5		// 0x00041a24
		jnp	L41b13		// 0x00041a27
		mov	dword ptr [ebp], esi		// 0x00041a2d
		fld	dword ptr [ebx + 0x10]		// 0x00041a30
		fmul	dword ptr [ecx + 0x10]		// 0x00041a33
		mov	eax, dword ptr [esp + 0x34]		// 0x00041a36
		fld	dword ptr [ebx + 0x20]		// 0x00041a3a
		mov	edi, dword ptr [esp + 0x4c]		// 0x00041a3d
		fmul	dword ptr [ecx + 0x14]		// 0x00041a41
		lea	edx, [esp + 0x10]		// 0x00041a44
		push	edx		// 0x00041a48
		sub	esp, 8		// 0x00041a49
		faddp	st(1), st		// 0x00041a4c
		lea	esi, [esp + 0x24]		// 0x00041a4e
		fld	dword ptr [ecx + 0xc]		// 0x00041a52
		add	ebp, 4		// 0x00041a55
		fmul	dword ptr [ebx]		// 0x00041a58
		faddp	st(1), st		// 0x00041a5a
		fstp	dword ptr [esp + 0x24]		// 0x00041a5c
		fld	dword ptr [ebx + 0x14]		// 0x00041a60
		fmul	dword ptr [ecx + 0x10]		// 0x00041a63
		fld	dword ptr [ebx + 0x24]		// 0x00041a66
		fmul	dword ptr [ecx + 0x14]		// 0x00041a69
		faddp	st(1), st		// 0x00041a6c
		fld	dword ptr [ebx + 4]		// 0x00041a6e
		fmul	dword ptr [ecx + 0xc]		// 0x00041a71
		faddp	st(1), st		// 0x00041a74
		fstp	dword ptr [esp + 0x28]		// 0x00041a76
		fld	dword ptr [ebx + 0x18]		// 0x00041a7a
		fmul	dword ptr [ecx + 0x10]		// 0x00041a7d
		fld	dword ptr [ebx + 0x28]		// 0x00041a80
		fmul	dword ptr [ecx + 0x14]		// 0x00041a83
		faddp	st(1), st		// 0x00041a86
		fld	dword ptr [ebx + 8]		// 0x00041a88
		fmul	dword ptr [ecx + 0xc]		// 0x00041a8b
		faddp	st(1), st		// 0x00041a8e
		fst	dword ptr [esp + 0x2c]		// 0x00041a90
		fmul	dword ptr [esp + 0x38]		// 0x00041a94
		fld	dword ptr [esp + 0x28]		// 0x00041a98
		fmul	dword ptr [esp + 0x34]		// 0x00041a9c
		faddp	st(1), st		// 0x00041aa0
		fld	dword ptr [esp + 0x24]		// 0x00041aa2
		fmul	dword ptr [esp + 0x30]		// 0x00041aa6
		faddp	st(1), st		// 0x00041aaa
		fld	st(0)		// 0x00041aac
		fadd	dword ptr [ecx + 0x20]		// 0x00041aae
		fstp	dword ptr [esp + 4]		// 0x00041ab1
		fadd	dword ptr [ecx + 0x1c]		// 0x00041ab5
		mov	ecx, dword ptr [esp + 0x50]		// 0x00041ab8
		fstp	dword ptr [esp]		// 0x00041abc
		push	eax		// 0x00041abf
		mov	eax, dword ptr [esp + 0x58]		// 0x00041ac0
		call	nxConvexMeshInterval		// 0x00041ac4
		add	esp, 0x10		// 0x00041ac9
		test	al, al		// 0x00041acc
		je	L41b97		// 0x00041ace
		fld	dword ptr [esp + 0x10]		// 0x00041ad4
		mov	ecx, dword ptr [esp + 0x50]		// 0x00041ad8
		fcomp	dword ptr [ecx]		// 0x00041adc
		mov	edi, dword ptr [esp + 0x3c]		// 0x00041ade
		mov	esi, dword ptr [esp + 0xc]		// 0x00041ae2
		fnstsw	ax		// 0x00041ae6
		test	ah, 5		// 0x00041ae8
		jp	L41b13		// 0x00041aeb
		mov	edx, dword ptr [esp + 0x10]		// 0x00041aed
		mov	eax, dword ptr [esp + 0x54]		// 0x00041af1
		mov	dword ptr [ecx], edx		// 0x00041af5
		mov	ecx, dword ptr [esp + 0x18]		// 0x00041af7
		mov	edx, dword ptr [esp + 0x1c]		// 0x00041afb
		mov	dword ptr [eax], ecx		// 0x00041aff
		mov	ecx, dword ptr [esp + 0x20]		// 0x00041b01
		mov	dword ptr [eax + 4], edx		// 0x00041b05
		mov	dword ptr [eax + 8], ecx		// 0x00041b08
		mov	eax, dword ptr [esp + 0x58]		// 0x00041b0b
		mov	edx, esi		// 0x00041b0f
		mov	dword ptr [eax], edx		// 0x00041b11
L41b13:
		mov	eax, dword ptr [esp + 0x14]		// 0x00041b13
		inc	esi		// 0x00041b17
		cmp	esi, eax		// 0x00041b18
		mov	dword ptr [esp + 0xc], esi		// 0x00041b1a
		jb	L419f6		// 0x00041b1e
L41b24:
		mov	esi, dword ptr [esp + 0x5c]		// 0x00041b24
		mov	ecx, dword ptr [esp + 0x60]		// 0x00041b28
		mov	eax, dword ptr [esp + 0x58]		// 0x00041b2c
		sub	ebp, esi		// 0x00041b30
		sar	ebp, 2		// 0x00041b32
		mov	dword ptr [ecx], ebp		// 0x00041b35
		cmp	dword ptr [eax], -1		// 0x00041b37
		jne	L41b8e		// 0x00041b3a
		mov	edx, dword ptr [esp + 0x54]		// 0x00041b3c
		mov	ecx, dword ptr [esp + 0x4c]		// 0x00041b40
		push	eax		// 0x00041b44
		mov	eax, dword ptr [esp + 0x54]		// 0x00041b45
		push	edx		// 0x00041b49
		mov	edx, dword ptr [esp + 0x50]		// 0x00041b4a
		push	eax		// 0x00041b4e
		mov	eax, dword ptr [esp + 0x50]		// 0x00041b4f
		push	ecx		// 0x00041b53
		mov	ecx, dword ptr [esp + 0x50]		// 0x00041b54
		push	edx		// 0x00041b58
		mov	edx, dword ptr [esp + 0x48]		// 0x00041b59
		push	eax		// 0x00041b5d
		push	ecx		// 0x00041b5e
		push	edx		// 0x00041b5f
		mov	esi, ebx		// 0x00041b60
		call	nxConvexMeshFaceAxesAll		// 0x00041b62
		mov	ecx, dword ptr [esp + 0x7c]		// 0x00041b67
		add	esp, 0x20		// 0x00041b6b
		test	ecx, ecx		// 0x00041b6e
		je	L41b8e		// 0x00041b70
		mov	edx, dword ptr [esp + 0x14]		// 0x00041b72
		xor	eax, eax		// 0x00041b76
		test	edx, edx		// 0x00041b78
		jbe	L41b88		// 0x00041b7a
		_emit	0x8d
		_emit	0x64
		_emit	0x24
		_emit	0x00		// 0x00041b7c lea esp, [esp]
L41b80:
		mov	dword ptr [ecx + eax*4], eax		// 0x00041b80
		inc	eax		// 0x00041b83
		cmp	eax, edx		// 0x00041b84
		jb	L41b80		// 0x00041b86
L41b88:
		mov	eax, dword ptr [esp + 0x60]		// 0x00041b88
		mov	dword ptr [eax], edx		// 0x00041b8c
L41b8e:
		pop	edi		// 0x00041b8e
		pop	esi		// 0x00041b8f
		mov	al, 1		// 0x00041b90
		pop	ebp		// 0x00041b92
		add	esp, 0x24		// 0x00041b93
		ret		// 0x00041b96
L41b97:
		pop	edi		// 0x00041b97
		pop	esi		// 0x00041b98
		xor	al, al		// 0x00041b99
		pop	ebp		// 0x00041b9b
		add	esp, 0x24		// 0x00041b9c
		ret		// 0x00041b9f
		}
	}

// phys_fn_001833 (0x00041ba0, 112 B)
// One axis (the triangle's normal) against the current best (register
// arguments: ecx the polygon interface, edx the support map, esi the axis, edi
// the best axis out; stack arguments: the scratch record, the triangle count
// and indices, the mesh, the hull's pose, the best depth and an index out or
// null). 001826 tests the axis; false when it separates; a depth below the
// best stores the depth and the axis, and 0 to the index when it is given.
__declspec(naked) bool nxConvexMeshTriangleAxis()
	{
	__asm
		{
		push	ecx		// 0x00041ba0
		push	ebx		// 0x00041ba1
		mov	ebx, dword ptr [esp + 0x24]		// 0x00041ba2
		push	ebp		// 0x00041ba6
		mov	ebp, dword ptr [esp + 0x24]		// 0x00041ba7
		lea	eax, [esp + 8]		// 0x00041bab
		push	eax		// 0x00041baf
		mov	eax, dword ptr [esp + 0x24]		// 0x00041bb0
		push	ecx		// 0x00041bb4
		mov	ecx, dword ptr [esp + 0x24]		// 0x00041bb5
		push	edx		// 0x00041bb9
		mov	edx, dword ptr [esp + 0x1c]		// 0x00041bba
		push	eax		// 0x00041bbe
		push	ecx		// 0x00041bbf
		mov	ecx, dword ptr [esp + 0x28]		// 0x00041bc0
		push	edx		// 0x00041bc4
		mov	edx, dword ptr [esp + 0x30]		// 0x00041bc5
		mov	eax, esi		// 0x00041bc9
		call	nxConvexMeshAxis		// 0x00041bcb
		add	esp, 0x18		// 0x00041bd0
		test	al, al		// 0x00041bd3
		jne	L41bdb		// 0x00041bd5
		pop	ebp		// 0x00041bd7
		pop	ebx		// 0x00041bd8
		pop	ecx		// 0x00041bd9
		ret		// 0x00041bda
L41bdb:
		fld	dword ptr [esp + 8]		// 0x00041bdb
		fcomp	dword ptr [ebp]		// 0x00041bdf
		fnstsw	ax		// 0x00041be2
		test	ah, 5		// 0x00041be4
		jp	L41c0a		// 0x00041be7
		test	ebx, ebx		// 0x00041be9
		mov	eax, dword ptr [esp + 8]		// 0x00041beb
		mov	dword ptr [ebp], eax		// 0x00041bef
		mov	ecx, dword ptr [esi]		// 0x00041bf2
		mov	dword ptr [edi], ecx		// 0x00041bf4
		mov	edx, dword ptr [esi + 4]		// 0x00041bf6
		mov	dword ptr [edi + 4], edx		// 0x00041bf9
		mov	eax, dword ptr [esi + 8]		// 0x00041bfc
		mov	dword ptr [edi + 8], eax		// 0x00041bff
		je	L41c0a		// 0x00041c02
		mov	dword ptr [ebx], 0		// 0x00041c04
L41c0a:
		pop	ebp		// 0x00041c0a
		mov	al, 1		// 0x00041c0b
		pop	ebx		// 0x00041c0d
		pop	ecx		// 0x00041c0e
		ret		// 0x00041c0f
		}
	}

// phys_fn_001834 (0x00041c10, 961 B)
// The triangles' active edges near a hull polygon, as axis directions (register
// arguments: ecx the polygon interface, eax the 4x4 that takes the hull into
// the mesh's frame; stack arguments: the Container the directions go to, the
// triangle count and indices, the mesh and the polygon's index). The mesh's
// EdgeList (+0x88) is built first (002188) when it is null; an `_alloca` of 36
// bytes a triangle is probed (005695) and never used. The polygon's plane
// (slot 4, +0x0c) is taken into the mesh's frame by the 4x4's rows and
// translation. For each triangle edge whose EdgeList word is negative (bit 31,
// the active flag) and with an end not in front of that plane (`test ah,
// 0x41`: at or below 0.0f, or NaN), the edge's direction (its first end minus
// the second) is normalised through an inline `fsqrt` when its squared length
// is not 0.0f and added to the Container by 001661.
__declspec(naked) void nxConvexMeshEdgeDirections()
	{
	__asm
		{
		push	ebp		// 0x00041c10
		lea	ebp, [esp - 0x64]		// 0x00041c11
		sub	esp, 0x84		// 0x00041c15
		push	ebx		// 0x00041c1b
		push	esi		// 0x00041c1c
		push	edi		// 0x00041c1d
		mov	edi, dword ptr [ebp + 0x78]		// 0x00041c1e
		mov	ebx, eax		// 0x00041c21
		mov	eax, dword ptr [edi + 0x88]		// 0x00041c23
		test	eax, eax		// 0x00041c29
		mov	esi, ecx		// 0x00041c2b
		jne	L41c36		// 0x00041c2d
		mov	ecx, edi		// 0x00041c2f
		call	nxConvexMeshCallCreateEdgeList		// 0x00041c31
L41c36:
		mov	eax, dword ptr [edi + 0x88]		// 0x00041c36
		mov	dword ptr [ebp + 0x28], eax		// 0x00041c3c
		mov	eax, dword ptr [ebp + 0x70]		// 0x00041c3f
		lea	eax, [eax + eax*8]		// 0x00041c42
		shl	eax, 2		// 0x00041c45
		add	eax, 3		// 0x00041c48
		and	eax, 0xfffffffc		// 0x00041c4b
		call	_chkstk		// 0x00041c4e
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00041c53
		mov	edx, dword ptr [esi]		// 0x00041c56
		push	eax		// 0x00041c58
		mov	ecx, esi		// 0x00041c59
		call	dword ptr [edx + 0x10]		// 0x00041c5b
		mov	ecx, dword ptr [ebx]		// 0x00041c5e
		mov	edx, dword ptr [ebx + 4]		// 0x00041c60
		mov	dword ptr [ebp + 4], ecx		// 0x00041c63
		mov	ecx, dword ptr [ebx + 8]		// 0x00041c66
		mov	dword ptr [ebp + 0xc], ecx		// 0x00041c69
		mov	ecx, dword ptr [ebx + 0x14]		// 0x00041c6c
		mov	dword ptr [ebp + 8], edx		// 0x00041c6f
		mov	edx, dword ptr [ebx + 0x10]		// 0x00041c72
		mov	dword ptr [ebp + 0x14], ecx		// 0x00041c75
		mov	ecx, dword ptr [ebx + 0x20]		// 0x00041c78
		mov	dword ptr [ebp + 0x10], edx		// 0x00041c7b
		mov	edx, dword ptr [ebx + 0x18]		// 0x00041c7e
		mov	dword ptr [ebp + 0x1c], ecx		// 0x00041c81
		mov	ecx, dword ptr [ebx + 0x28]		// 0x00041c84
		add	eax, 0xc		// 0x00041c87
		mov	dword ptr [ebp + 0x18], edx		// 0x00041c8a
		mov	edx, dword ptr [ebx + 0x24]		// 0x00041c8d
		mov	dword ptr [ebp + 0x24], ecx		// 0x00041c90
		mov	dword ptr [ebp + 0x20], edx		// 0x00041c93
		mov	ecx, 9		// 0x00041c96
		lea	esi, [ebp + 4]		// 0x00041c9b
		lea	edi, [ebp - 0x20]		// 0x00041c9e
		rep movsd		// 0x00041ca1
		fld	dword ptr [ebp - 0x14]		// 0x00041ca3
		fmul	dword ptr [eax + 4]		// 0x00041ca6
		fld	dword ptr [ebp - 0x20]		// 0x00041ca9
		fmul	dword ptr [eax]		// 0x00041cac
		faddp	st(1), st		// 0x00041cae
		fld	dword ptr [ebp - 8]		// 0x00041cb0
		fmul	dword ptr [eax + 8]		// 0x00041cb3
		faddp	st(1), st		// 0x00041cb6
		fstp	dword ptr [ebp + 0x48]		// 0x00041cb8
		mov	edx, dword ptr [ebp + 0x48]		// 0x00041cbb
		fld	dword ptr [ebp - 0x10]		// 0x00041cbe
		mov	dword ptr [ebp + 0x54], edx		// 0x00041cc1
		fmul	dword ptr [eax + 4]		// 0x00041cc4
		fld	dword ptr [ebp - 0x1c]		// 0x00041cc7
		fmul	dword ptr [eax]		// 0x00041cca
		faddp	st(1), st		// 0x00041ccc
		fld	dword ptr [ebp - 4]		// 0x00041cce
		fmul	dword ptr [eax + 8]		// 0x00041cd1
		faddp	st(1), st		// 0x00041cd4
		fstp	dword ptr [ebp + 0x4c]		// 0x00041cd6
		mov	ecx, dword ptr [ebp + 0x4c]		// 0x00041cd9
		fld	dword ptr [ebp - 0xc]		// 0x00041cdc
		mov	dword ptr [ebp + 0x58], ecx		// 0x00041cdf
		fmul	dword ptr [eax + 4]		// 0x00041ce2
		fld	dword ptr [ebp - 0x18]		// 0x00041ce5
		fmul	dword ptr [eax]		// 0x00041ce8
		faddp	st(1), st		// 0x00041cea
		fld	dword ptr [ebp]		// 0x00041cec
		fmul	dword ptr [eax + 8]		// 0x00041cef
		faddp	st(1), st		// 0x00041cf2
		fstp	dword ptr [ebp + 0x50]		// 0x00041cf4
		mov	edx, dword ptr [ebp + 0x50]		// 0x00041cf7
		fld	dword ptr [ebx + 0x30]		// 0x00041cfa
		mov	dword ptr [ebp + 0x5c], edx		// 0x00041cfd
		fld	dword ptr [ebx + 0x34]		// 0x00041d00
		fld	dword ptr [ebx + 0x38]		// 0x00041d03
		fld	dword ptr [ebp + 0x5c]		// 0x00041d06
		fmul	st, st(1)		// 0x00041d09
		fld	dword ptr [ebp + 0x58]		// 0x00041d0b
		fmul	st, st(3)		// 0x00041d0e
		faddp	st(1), st		// 0x00041d10
		fld	dword ptr [ebp + 0x54]		// 0x00041d12
		fmul	st, st(4)		// 0x00041d15
		faddp	st(1), st		// 0x00041d17
		fsubr	dword ptr [eax + 0xc]		// 0x00041d19
		mov	eax, dword ptr [ebp + 0x70]		// 0x00041d1c
		test	eax, eax		// 0x00041d1f
		fstp	dword ptr [ebp + 0x60]		// 0x00041d21
		fstp	st(0)		// 0x00041d24
		fstp	st(0)		// 0x00041d26
		fstp	st(0)		// 0x00041d28
		je	L41fc4		// 0x00041d2a
		mov	dword ptr [ebp + 0x2c], eax		// 0x00041d30
L41d33:
		mov	ecx, dword ptr [ebp + 0x74]		// 0x00041d33
		mov	eax, dword ptr [ecx]		// 0x00041d36
		mov	edx, dword ptr [ebp + 0x78]		// 0x00041d38
		add	ecx, 4		// 0x00041d3b
		mov	dword ptr [ebp + 0x74], ecx		// 0x00041d3e
		lea	ecx, [eax + eax*2]		// 0x00041d41
		mov	eax, dword ptr [edx + 0x14]		// 0x00041d44
		mov	edx, dword ptr [edx + 0x10]		// 0x00041d47
		shl	ecx, 2		// 0x00041d4a
		mov	esi, dword ptr [eax + ecx]		// 0x00041d4d
		add	eax, ecx		// 0x00041d50
		lea	esi, [esi + esi*2]		// 0x00041d52
		lea	ebx, [edx + esi*4]		// 0x00041d55
		mov	esi, dword ptr [eax + 4]		// 0x00041d58
		mov	eax, dword ptr [eax + 8]		// 0x00041d5b
		lea	esi, [esi + esi*2]		// 0x00041d5e
		lea	eax, [eax + eax*2]		// 0x00041d61
		lea	edi, [edx + esi*4]		// 0x00041d64
		lea	edx, [edx + eax*4]		// 0x00041d67
		mov	eax, dword ptr [ebp + 0x28]		// 0x00041d6a
		mov	esi, dword ptr [eax + 0xc]		// 0x00041d6d
		mov	eax, dword ptr [esi + ecx]		// 0x00041d70
		add	esi, ecx		// 0x00041d73
		test	eax, eax		// 0x00041d75
		mov	dword ptr [ebp + 0x70], edx		// 0x00041d77
		jns	L41e38		// 0x00041d7a
		fld	dword ptr [ebp + 0x5c]		// 0x00041d80
		fmul	dword ptr [ebx + 8]		// 0x00041d83
		fld	dword ptr [ebp + 0x58]		// 0x00041d86
		fmul	dword ptr [ebx + 4]		// 0x00041d89
		faddp	st(1), st		// 0x00041d8c
		fld	dword ptr [ebp + 0x54]		// 0x00041d8e
		fmul	dword ptr [ebx]		// 0x00041d91
		faddp	st(1), st		// 0x00041d93
		fadd	dword ptr [ebp + 0x60]		// 0x00041d95
		fcomp	dword ptr kConvexMeshZero		// 0x00041d98
		fnstsw	ax		// 0x00041d9e
		test	ah, 0x41		// 0x00041da0
		jnp	L41dca		// 0x00041da3
		fld	dword ptr [ebp + 0x5c]		// 0x00041da5
		fmul	dword ptr [edi + 8]		// 0x00041da8
		fld	dword ptr [ebp + 0x58]		// 0x00041dab
		fmul	dword ptr [edi + 4]		// 0x00041dae
		faddp	st(1), st		// 0x00041db1
		fld	dword ptr [ebp + 0x54]		// 0x00041db3
		fmul	dword ptr [edi]		// 0x00041db6
		faddp	st(1), st		// 0x00041db8
		fadd	dword ptr [ebp + 0x60]		// 0x00041dba
		fcomp	dword ptr kConvexMeshZero		// 0x00041dbd
		fnstsw	ax		// 0x00041dc3
		test	ah, 0x41		// 0x00041dc5
		jp	L41e38		// 0x00041dc8
L41dca:
		fld	dword ptr [ebx]		// 0x00041dca
		fsub	dword ptr [edi]		// 0x00041dcc
		fstp	dword ptr [ebp + 0x30]		// 0x00041dce
		fld	dword ptr [ebx + 4]		// 0x00041dd1
		fsub	dword ptr [edi + 4]		// 0x00041dd4
		fstp	dword ptr [ebp + 0x34]		// 0x00041dd7
		fld	dword ptr [ebx + 8]		// 0x00041dda
		fsub	dword ptr [edi + 8]		// 0x00041ddd
		fst	dword ptr [ebp + 0x38]		// 0x00041de0
		fmul	dword ptr [ebp + 0x38]		// 0x00041de3
		fld	dword ptr [ebp + 0x34]		// 0x00041de6
		fmul	dword ptr [ebp + 0x34]		// 0x00041de9
		faddp	st(1), st		// 0x00041dec
		fld	dword ptr [ebp + 0x30]		// 0x00041dee
		fmul	dword ptr [ebp + 0x30]		// 0x00041df1
		faddp	st(1), st		// 0x00041df4
		fld	dword ptr kConvexMeshZero		// 0x00041df6
		fld	st(1)		// 0x00041dfc
		fucompp		// 0x00041dfe
		fnstsw	ax		// 0x00041e00
		test	ah, 0x44		// 0x00041e02
		jnp	L41e27		// 0x00041e05
		fsqrt		// 0x00041e07
		fdivr	dword ptr kConvexMeshOne		// 0x00041e09
		fld	dword ptr [ebp + 0x30]		// 0x00041e0f
		fmul	st, st(1)		// 0x00041e12
		fstp	dword ptr [ebp + 0x30]		// 0x00041e14
		fld	dword ptr [ebp + 0x34]		// 0x00041e17
		fmul	st, st(1)		// 0x00041e1a
		fstp	dword ptr [ebp + 0x34]		// 0x00041e1c
		fld	dword ptr [ebp + 0x38]		// 0x00041e1f
		fmul	st, st(1)		// 0x00041e22
		fstp	dword ptr [ebp + 0x38]		// 0x00041e24
L41e27:
		lea	ecx, [ebp + 0x30]		// 0x00041e27
		fstp	st(0)		// 0x00041e2a
		push	ecx		// 0x00041e2c
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00041e2d
		call	nxIceAddUniqueAxis		// 0x00041e30
		mov	edx, dword ptr [ebp + 0x70]		// 0x00041e35
L41e38:
		mov	eax, dword ptr [esi + 4]		// 0x00041e38
		test	eax, eax		// 0x00041e3b
		jns	L41efb		// 0x00041e3d
		fld	dword ptr [ebp + 0x54]		// 0x00041e43
		fmul	dword ptr [edi]		// 0x00041e46
		fld	dword ptr [ebp + 0x5c]		// 0x00041e48
		fmul	dword ptr [edi + 8]		// 0x00041e4b
		faddp	st(1), st		// 0x00041e4e
		fld	dword ptr [ebp + 0x58]		// 0x00041e50
		fmul	dword ptr [edi + 4]		// 0x00041e53
		faddp	st(1), st		// 0x00041e56
		fadd	dword ptr [ebp + 0x60]		// 0x00041e58
		fcomp	dword ptr kConvexMeshZero		// 0x00041e5b
		fnstsw	ax		// 0x00041e61
		test	ah, 0x41		// 0x00041e63
		jnp	L41e8d		// 0x00041e66
		fld	dword ptr [ebp + 0x5c]		// 0x00041e68
		fmul	dword ptr [edx + 8]		// 0x00041e6b
		fld	dword ptr [ebp + 0x58]		// 0x00041e6e
		fmul	dword ptr [edx + 4]		// 0x00041e71
		faddp	st(1), st		// 0x00041e74
		fld	dword ptr [ebp + 0x54]		// 0x00041e76
		fmul	dword ptr [edx]		// 0x00041e79
		faddp	st(1), st		// 0x00041e7b
		fadd	dword ptr [ebp + 0x60]		// 0x00041e7d
		fcomp	dword ptr kConvexMeshZero		// 0x00041e80
		fnstsw	ax		// 0x00041e86
		test	ah, 0x41		// 0x00041e88
		jp	L41efb		// 0x00041e8b
L41e8d:
		fld	dword ptr [edi]		// 0x00041e8d
		fsub	dword ptr [edx]		// 0x00041e8f
		fstp	dword ptr [ebp + 0x3c]		// 0x00041e91
		fld	dword ptr [edi + 4]		// 0x00041e94
		fsub	dword ptr [edx + 4]		// 0x00041e97
		fstp	dword ptr [ebp + 0x40]		// 0x00041e9a
		fld	dword ptr [edi + 8]		// 0x00041e9d
		fsub	dword ptr [edx + 8]		// 0x00041ea0
		fst	dword ptr [ebp + 0x44]		// 0x00041ea3
		fmul	dword ptr [ebp + 0x44]		// 0x00041ea6
		fld	dword ptr [ebp + 0x40]		// 0x00041ea9
		fmul	dword ptr [ebp + 0x40]		// 0x00041eac
		faddp	st(1), st		// 0x00041eaf
		fld	dword ptr [ebp + 0x3c]		// 0x00041eb1
		fmul	dword ptr [ebp + 0x3c]		// 0x00041eb4
		faddp	st(1), st		// 0x00041eb7
		fld	dword ptr kConvexMeshZero		// 0x00041eb9
		fld	st(1)		// 0x00041ebf
		fucompp		// 0x00041ec1
		fnstsw	ax		// 0x00041ec3
		test	ah, 0x44		// 0x00041ec5
		jnp	L41eea		// 0x00041ec8
		fsqrt		// 0x00041eca
		fdivr	dword ptr kConvexMeshOne		// 0x00041ecc
		fld	dword ptr [ebp + 0x3c]		// 0x00041ed2
		fmul	st, st(1)		// 0x00041ed5
		fstp	dword ptr [ebp + 0x3c]		// 0x00041ed7
		fld	dword ptr [ebp + 0x40]		// 0x00041eda
		fmul	st, st(1)		// 0x00041edd
		fstp	dword ptr [ebp + 0x40]		// 0x00041edf
		fld	dword ptr [ebp + 0x44]		// 0x00041ee2
		fmul	st, st(1)		// 0x00041ee5
		fstp	dword ptr [ebp + 0x44]		// 0x00041ee7
L41eea:
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00041eea
		fstp	st(0)		// 0x00041eed
		lea	edx, [ebp + 0x3c]		// 0x00041eef
		push	edx		// 0x00041ef2
		call	nxIceAddUniqueAxis		// 0x00041ef3
		mov	edx, dword ptr [ebp + 0x70]		// 0x00041ef8
L41efb:
		mov	eax, dword ptr [esi + 8]		// 0x00041efb
		test	eax, eax		// 0x00041efe
		jns	L41fbb		// 0x00041f00
		fld	dword ptr [ebp + 0x5c]		// 0x00041f06
		fmul	dword ptr [ebx + 8]		// 0x00041f09
		fld	dword ptr [ebp + 0x58]		// 0x00041f0c
		fmul	dword ptr [ebx + 4]		// 0x00041f0f
		faddp	st(1), st		// 0x00041f12
		fld	dword ptr [ebp + 0x54]		// 0x00041f14
		fmul	dword ptr [ebx]		// 0x00041f17
		faddp	st(1), st		// 0x00041f19
		fadd	dword ptr [ebp + 0x60]		// 0x00041f1b
		fcomp	dword ptr kConvexMeshZero		// 0x00041f1e
		fnstsw	ax		// 0x00041f24
		test	ah, 0x41		// 0x00041f26
		jnp	L41f50		// 0x00041f29
		fld	dword ptr [ebp + 0x5c]		// 0x00041f2b
		fmul	dword ptr [edx + 8]		// 0x00041f2e
		fld	dword ptr [ebp + 0x58]		// 0x00041f31
		fmul	dword ptr [edx + 4]		// 0x00041f34
		faddp	st(1), st		// 0x00041f37
		fld	dword ptr [ebp + 0x54]		// 0x00041f39
		fmul	dword ptr [edx]		// 0x00041f3c
		faddp	st(1), st		// 0x00041f3e
		fadd	dword ptr [ebp + 0x60]		// 0x00041f40
		fcomp	dword ptr kConvexMeshZero		// 0x00041f43
		fnstsw	ax		// 0x00041f49
		test	ah, 0x41		// 0x00041f4b
		jp	L41fbb		// 0x00041f4e
L41f50:
		fld	dword ptr [ebx]		// 0x00041f50
		fsub	dword ptr [edx]		// 0x00041f52
		fstp	dword ptr [ebp + 0x48]		// 0x00041f54
		fld	dword ptr [ebx + 4]		// 0x00041f57
		fsub	dword ptr [edx + 4]		// 0x00041f5a
		fstp	dword ptr [ebp + 0x4c]		// 0x00041f5d
		fld	dword ptr [ebx + 8]		// 0x00041f60
		fsub	dword ptr [edx + 8]		// 0x00041f63
		fst	dword ptr [ebp + 0x50]		// 0x00041f66
		fmul	dword ptr [ebp + 0x50]		// 0x00041f69
		fld	dword ptr [ebp + 0x4c]		// 0x00041f6c
		fmul	dword ptr [ebp + 0x4c]		// 0x00041f6f
		faddp	st(1), st		// 0x00041f72
		fld	dword ptr [ebp + 0x48]		// 0x00041f74
		fmul	dword ptr [ebp + 0x48]		// 0x00041f77
		faddp	st(1), st		// 0x00041f7a
		fld	dword ptr kConvexMeshZero		// 0x00041f7c
		fld	st(1)		// 0x00041f82
		fucompp		// 0x00041f84
		fnstsw	ax		// 0x00041f86
		test	ah, 0x44		// 0x00041f88
		jnp	L41fad		// 0x00041f8b
		fsqrt		// 0x00041f8d
		fdivr	dword ptr kConvexMeshOne		// 0x00041f8f
		fld	dword ptr [ebp + 0x48]		// 0x00041f95
		fmul	st, st(1)		// 0x00041f98
		fstp	dword ptr [ebp + 0x48]		// 0x00041f9a
		fld	dword ptr [ebp + 0x4c]		// 0x00041f9d
		fmul	st, st(1)		// 0x00041fa0
		fstp	dword ptr [ebp + 0x4c]		// 0x00041fa2
		fld	dword ptr [ebp + 0x50]		// 0x00041fa5
		fmul	st, st(1)		// 0x00041fa8
		fstp	dword ptr [ebp + 0x50]		// 0x00041faa
L41fad:
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00041fad
		fstp	st(0)		// 0x00041fb0
		lea	eax, [ebp + 0x48]		// 0x00041fb2
		push	eax		// 0x00041fb5
		call	nxIceAddUniqueAxis		// 0x00041fb6
L41fbb:
		dec	dword ptr [ebp + 0x2c]		// 0x00041fbb
		jne	L41d33		// 0x00041fbe
L41fc4:
		lea	esp, [ebp - 0x2c]		// 0x00041fc4
		pop	edi		// 0x00041fc7
		pop	esi		// 0x00041fc8
		pop	ebx		// 0x00041fc9
		add	ebp, 0x64		// 0x00041fca
		mov	esp, ebp		// 0x00041fcd
		pop	ebp		// 0x00041fcf
		ret		// 0x00041fd0
		}
	}

// phys_fn_001836 (0x00041fe0, 342 B)
// phys_fn_001838 (0x00042140, 790 B)
// The cross axes (the row and its continuation; register arguments: eax the
// 4x4 taking the mesh into the hull's frame, ebx the 4x4 taking the hull into
// the mesh's, edx the triangle's plane; eleven stack arguments: the Container
// of edge directions (the scratch record's +0x4e0), the scratch record, the
// polygon count and indices from 001832, the mesh, the polygon interface, the
// support map, the triangle count and indices, the best axis out and the best
// depth out, which starts at FLT_MAX). The plane is taken into the hull's
// frame; the Container at +0x4f0 is emptied. For each listed polygon (slot 4)
// and each of its edges (the ends ordered by index) with an end not in front
// of the plane (`test ah, 0x41`), the edge (slot 2's vertices) is rotated by
// ebx and crossed with every edge direction; a cross product with a component
// above 1e-6 in magnitude (the double at 0x10107c58) is normalised (inline
// `fsqrt`, not when its squared length is 0.0f) and added to +0x4f0 by 001661.
// Each gathered axis is then tested with 001826: false when one separates, and
// the least depth keeps its axis.
__declspec(naked) bool nxConvexMeshCrossAxes()
	{
	__asm
		{
		sub	esp, 0x94		// 0x00041fe0
		mov	ecx, dword ptr [esp + 0xc0]		// 0x00041fe6
		mov	dword ptr [ecx], 0x7f7fffff		// 0x00041fed
		mov	ecx, dword ptr [eax]		// 0x00041ff3
		mov	dword ptr [esp + 0x4c], ecx		// 0x00041ff5
		mov	ecx, dword ptr [eax + 4]		// 0x00041ff9
		mov	dword ptr [esp + 0x50], ecx		// 0x00041ffc
		mov	ecx, dword ptr [eax + 8]		// 0x00042000
		mov	dword ptr [esp + 0x54], ecx		// 0x00042003
		mov	ecx, dword ptr [eax + 0x10]		// 0x00042007
		mov	dword ptr [esp + 0x58], ecx		// 0x0004200a
		mov	ecx, dword ptr [eax + 0x14]		// 0x0004200e
		mov	dword ptr [esp + 0x5c], ecx		// 0x00042011
		mov	ecx, dword ptr [eax + 0x18]		// 0x00042015
		mov	dword ptr [esp + 0x60], ecx		// 0x00042018
		mov	ecx, dword ptr [eax + 0x20]		// 0x0004201c
		mov	dword ptr [esp + 0x64], ecx		// 0x0004201f
		mov	ecx, dword ptr [eax + 0x24]		// 0x00042023
		mov	dword ptr [esp + 0x68], ecx		// 0x00042026
		mov	ecx, dword ptr [eax + 0x28]		// 0x0004202a
		push	ebp		// 0x0004202d
		push	esi		// 0x0004202e
		mov	dword ptr [esp + 0x74], ecx		// 0x0004202f
		push	edi		// 0x00042033
		mov	ecx, 9		// 0x00042034
		lea	esi, [esp + 0x58]		// 0x00042039
		lea	edi, [esp + 0x7c]		// 0x0004203d
		rep movsd		// 0x00042041
		fld	dword ptr [esp + 0x88]		// 0x00042043
		fmul	dword ptr [edx + 4]		// 0x0004204a
		fld	dword ptr [esp + 0x94]		// 0x0004204d
		fmul	dword ptr [edx + 8]		// 0x00042054
		faddp	st(1), st		// 0x00042057
		fld	dword ptr [esp + 0x7c]		// 0x00042059
		fmul	dword ptr [edx]		// 0x0004205d
		faddp	st(1), st		// 0x0004205f
		mov	esi, dword ptr [esp + 0xb8]		// 0x00042061
		fstp	dword ptr [esp + 0xc]		// 0x00042068
		mov	ecx, dword ptr [esp + 0xc]		// 0x0004206c
		fld	dword ptr [esp + 0x8c]		// 0x00042070
		mov	dword ptr [esp + 0x24], ecx		// 0x00042077
		fmul	dword ptr [edx + 4]		// 0x0004207b
		fld	dword ptr [esp + 0x98]		// 0x0004207e
		fmul	dword ptr [edx + 8]		// 0x00042085
		faddp	st(1), st		// 0x00042088
		fld	dword ptr [esp + 0x80]		// 0x0004208a
		fmul	dword ptr [edx]		// 0x00042091
		faddp	st(1), st		// 0x00042093
		fstp	dword ptr [esp + 0x10]		// 0x00042095
		mov	ecx, dword ptr [esp + 0x10]		// 0x00042099
		fld	dword ptr [esp + 0x90]		// 0x0004209d
		mov	dword ptr [esp + 0x28], ecx		// 0x000420a4
		fmul	dword ptr [edx + 4]		// 0x000420a8
		fld	dword ptr [esp + 0x9c]		// 0x000420ab
		fmul	dword ptr [edx + 8]		// 0x000420b2
		faddp	st(1), st		// 0x000420b5
		fld	dword ptr [esp + 0x84]		// 0x000420b7
		fmul	dword ptr [edx]		// 0x000420be
		faddp	st(1), st		// 0x000420c0
		fstp	dword ptr [esp + 0x14]		// 0x000420c2
		mov	ecx, dword ptr [esp + 0x14]		// 0x000420c6
		fld	dword ptr [eax + 0x30]		// 0x000420ca
		mov	dword ptr [esp + 0x2c], ecx		// 0x000420cd
		fld	dword ptr [eax + 0x34]		// 0x000420d1
		mov	ecx, esi		// 0x000420d4
		fld	dword ptr [eax + 0x38]		// 0x000420d6
		fld	dword ptr [esp + 0x2c]		// 0x000420d9
		fmul	st, st(1)		// 0x000420dd
		fld	dword ptr [esp + 0x28]		// 0x000420df
		fmul	st, st(3)		// 0x000420e3
		faddp	st(1), st		// 0x000420e5
		fld	dword ptr [esp + 0x24]		// 0x000420e7
		fmul	st, st(4)		// 0x000420eb
		faddp	st(1), st		// 0x000420ed
		fsubr	dword ptr [edx + 0xc]		// 0x000420ef
		mov	edx, dword ptr [esi]		// 0x000420f2
		fstp	dword ptr [esp + 0x30]		// 0x000420f4
		fstp	st(0)		// 0x000420f8
		fstp	st(0)		// 0x000420fa
		fstp	st(0)		// 0x000420fc
		call	dword ptr [edx + 8]		// 0x000420fe
		mov	ecx, dword ptr [esp + 0xa8]		// 0x00042101
		mov	edx, dword ptr [ecx + 0x4f4]		// 0x00042108
		add	ecx, 0x4f0		// 0x0004210e
		mov	edi, eax		// 0x00042114
		xor	eax, eax		// 0x00042116
		cmp	edx, eax		// 0x00042118
		mov	dword ptr [esp + 0x18], ecx		// 0x0004211a
		je	L42123		// 0x0004211e
		mov	dword ptr [ecx + 4], eax		// 0x00042120
L42123:
		cmp	dword ptr [esp + 0xac], eax		// 0x00042123
		mov	dword ptr [esp + 0x1c], eax		// 0x0004212a
		jbe	L423aa		// 0x0004212e
		jmp	L42140		// 0x00042134
		_emit	0x8d
		_emit	0xa4
		_emit	0x24
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x00042136 lea esp, [esp]
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x0004213d lea ecx, [ecx]
L42140:
		mov	edx, dword ptr [esp + 0x1c]		// 0x00042140
		mov	ecx, dword ptr [esp + 0xb0]		// 0x00042144
		mov	ecx, dword ptr [ecx + edx*4]		// 0x0004214b
		mov	eax, dword ptr [esi]		// 0x0004214e
		push	ecx		// 0x00042150
		mov	ecx, esi		// 0x00042151
		call	dword ptr [eax + 0x10]		// 0x00042153
		mov	ecx, dword ptr [esp + 0xa4]		// 0x00042156
		mov	ebp, eax		// 0x0004215d
		mov	edx, dword ptr [ebp + 4]		// 0x0004215f
		mov	dword ptr [esp + 0x3c], edx		// 0x00042162
		mov	eax, 0xaaaaaaab		// 0x00042166
		mul	dword ptr [ecx + 4]		// 0x0004216b
		mov	ecx, dword ptr [ecx + 8]		// 0x0004216e
		shr	edx, 1		// 0x00042171
		test	edx, edx		// 0x00042173
		mov	dword ptr [esp + 0x48], ebp		// 0x00042175
		je	L4238e		// 0x00042179
		mov	dword ptr [esp + 0x38], edx		// 0x0004217f
L42183:
		mov	eax, dword ptr [ebp]		// 0x00042183
		mov	esi, ecx		// 0x00042186
		add	ecx, 0xc		// 0x00042188
		test	eax, eax		// 0x0004218b
		mov	dword ptr [esp + 0x44], ecx		// 0x0004218d
		mov	dword ptr [esp + 0x40], eax		// 0x00042191
		jbe	L4237d		// 0x00042195
		mov	ecx, dword ptr [esp + 0x3c]		// 0x0004219b
		mov	ebp, 1		// 0x0004219f
		mov	dword ptr [esp + 0x20], ecx		// 0x000421a4
		mov	dword ptr [esp + 0x34], eax		// 0x000421a8
		jmp	L421b2		// 0x000421ac
L421ae:
		mov	eax, dword ptr [esp + 0x40]		// 0x000421ae
L421b2:
		cmp	ebp, eax		// 0x000421b2
		mov	ecx, ebp		// 0x000421b4
		jb	L421ba		// 0x000421b6
		xor	ecx, ecx		// 0x000421b8
L421ba:
		mov	edx, dword ptr [esp + 0x20]		// 0x000421ba
		mov	eax, dword ptr [edx]		// 0x000421be
		mov	edx, dword ptr [esp + 0x3c]		// 0x000421c0
		mov	ecx, dword ptr [edx + ecx*4]		// 0x000421c4
		cmp	eax, ecx		// 0x000421c7
		jbe	L421d1		// 0x000421c9
		xor	eax, ecx		// 0x000421cb
		xor	ecx, eax		// 0x000421cd
		xor	eax, ecx		// 0x000421cf
L421d1:
		fld	dword ptr [esp + 0x2c]		// 0x000421d1
		lea	eax, [eax + eax*2]		// 0x000421d5
		fmul	dword ptr [edi + eax*4 + 8]		// 0x000421d8
		lea	edx, [edi + eax*4]		// 0x000421dc
		fld	dword ptr [esp + 0x28]		// 0x000421df
		fmul	dword ptr [edx + 4]		// 0x000421e3
		faddp	st(1), st		// 0x000421e6
		fld	dword ptr [esp + 0x24]		// 0x000421e8
		fmul	dword ptr [edx]		// 0x000421ec
		faddp	st(1), st		// 0x000421ee
		fadd	dword ptr [esp + 0x30]		// 0x000421f0
		fcomp	dword ptr kConvexMeshZero		// 0x000421f4
		fnstsw	ax		// 0x000421fa
		test	ah, 0x41		// 0x000421fc
		jnp	L42235		// 0x000421ff
		fld	dword ptr [esp + 0x2c]		// 0x00042201
		lea	eax, [ecx + ecx*2]		// 0x00042205
		fmul	dword ptr [edi + eax*4 + 8]		// 0x00042208
		lea	eax, [edi + eax*4]		// 0x0004220c
		fld	dword ptr [esp + 0x28]		// 0x0004220f
		fmul	dword ptr [eax + 4]		// 0x00042213
		faddp	st(1), st		// 0x00042216
		fld	dword ptr [esp + 0x24]		// 0x00042218
		fmul	dword ptr [eax]		// 0x0004221c
		faddp	st(1), st		// 0x0004221e
		fadd	dword ptr [esp + 0x30]		// 0x00042220
		fcomp	dword ptr kConvexMeshZero		// 0x00042224
		fnstsw	ax		// 0x0004222a
		test	ah, 0x41		// 0x0004222c
		jp	L4235a		// 0x0004222f
L42235:
		fld	dword ptr [edx]		// 0x00042235
		lea	ecx, [ecx + ecx*2]		// 0x00042237
		fsub	dword ptr [edi + ecx*4]		// 0x0004223a
		lea	eax, [edi + ecx*4]		// 0x0004223d
		fld	dword ptr [edx + 4]		// 0x00042240
		fsub	dword ptr [eax + 4]		// 0x00042243
		fld	dword ptr [edx + 8]		// 0x00042246
		fsub	dword ptr [eax + 8]		// 0x00042249
		fld	st(0)		// 0x0004224c
		fmul	dword ptr [ebx + 0x20]		// 0x0004224e
		fld	st(2)		// 0x00042251
		fmul	dword ptr [ebx + 0x10]		// 0x00042253
		faddp	st(1), st		// 0x00042256
		fld	st(3)		// 0x00042258
		fmul	dword ptr [ebx]		// 0x0004225a
		faddp	st(1), st		// 0x0004225c
		fstp	dword ptr [esp + 0x4c]		// 0x0004225e
		fld	st(0)		// 0x00042262
		fmul	dword ptr [ebx + 0x24]		// 0x00042264
		fld	st(2)		// 0x00042267
		fmul	dword ptr [ebx + 0x14]		// 0x00042269
		faddp	st(1), st		// 0x0004226c
		fld	st(3)		// 0x0004226e
		fmul	dword ptr [ebx + 4]		// 0x00042270
		faddp	st(1), st		// 0x00042273
		fstp	dword ptr [esp + 0x50]		// 0x00042275
		fmul	dword ptr [ebx + 0x28]		// 0x00042279
		fxch	st(1)		// 0x0004227c
		fmul	dword ptr [ebx + 0x18]		// 0x0004227e
		faddp	st(1), st		// 0x00042281
		fxch	st(1)		// 0x00042283
		fmul	dword ptr [ebx + 8]		// 0x00042285
		faddp	st(1), st		// 0x00042288
		fld	dword ptr [esp + 0x50]		// 0x0004228a
		fmul	dword ptr [esi + 8]		// 0x0004228e
		fld	st(1)		// 0x00042291
		fmul	dword ptr [esi + 4]		// 0x00042293
		fsubp	st(1), st		// 0x00042296
		fstp	dword ptr [esp + 0xc]		// 0x00042298
		fmul	dword ptr [esi]		// 0x0004229c
		fld	dword ptr [esp + 0x4c]		// 0x0004229e
		fmul	dword ptr [esi + 8]		// 0x000422a2
		fsubp	st(1), st		// 0x000422a5
		fstp	dword ptr [esp + 0x10]		// 0x000422a7
		fld	dword ptr [esp + 0x4c]		// 0x000422ab
		fmul	dword ptr [esi + 4]		// 0x000422af
		fld	dword ptr [esp + 0x50]		// 0x000422b2
		fmul	dword ptr [esi]		// 0x000422b6
		fsubp	st(1), st		// 0x000422b8
		fstp	dword ptr [esp + 0x14]		// 0x000422ba
		fld	dword ptr [esp + 0xc]		// 0x000422be
		fabs		// 0x000422c2
		fcomp	qword ptr kConvexMeshMicro		// 0x000422c4
		fnstsw	ax		// 0x000422ca
		test	ah, 0x41		// 0x000422cc
		je	L422f7		// 0x000422cf
		fld	dword ptr [esp + 0x10]		// 0x000422d1
		fabs		// 0x000422d5
		fcomp	qword ptr kConvexMeshMicro		// 0x000422d7
		fnstsw	ax		// 0x000422dd
		test	ah, 0x41		// 0x000422df
		je	L422f7		// 0x000422e2
		fld	dword ptr [esp + 0x14]		// 0x000422e4
		fabs		// 0x000422e8
		fcomp	qword ptr kConvexMeshMicro		// 0x000422ea
		fnstsw	ax		// 0x000422f0
		test	ah, 0x41		// 0x000422f2
		jne	L4235a		// 0x000422f5
L422f7:
		fld	dword ptr [esp + 0x14]		// 0x000422f7
		fmul	dword ptr [esp + 0x14]		// 0x000422fb
		fld	dword ptr [esp + 0x10]		// 0x000422ff
		fmul	dword ptr [esp + 0x10]		// 0x00042303
		faddp	st(1), st		// 0x00042307
		fld	dword ptr [esp + 0xc]		// 0x00042309
		fmul	dword ptr [esp + 0xc]		// 0x0004230d
		faddp	st(1), st		// 0x00042311
		fld	dword ptr kConvexMeshZero		// 0x00042313
		fld	st(1)		// 0x00042319
		fucompp		// 0x0004231b
		fnstsw	ax		// 0x0004231d
		test	ah, 0x44		// 0x0004231f
		jnp	L4234a		// 0x00042322
		fsqrt		// 0x00042324
		fdivr	dword ptr kConvexMeshOne		// 0x00042326
		fld	dword ptr [esp + 0xc]		// 0x0004232c
		fmul	st, st(1)		// 0x00042330
		fstp	dword ptr [esp + 0xc]		// 0x00042332
		fld	dword ptr [esp + 0x10]		// 0x00042336
		fmul	st, st(1)		// 0x0004233a
		fstp	dword ptr [esp + 0x10]		// 0x0004233c
		fld	dword ptr [esp + 0x14]		// 0x00042340
		fmul	st, st(1)		// 0x00042344
		fstp	dword ptr [esp + 0x14]		// 0x00042346
L4234a:
		mov	ecx, dword ptr [esp + 0x18]		// 0x0004234a
		fstp	st(0)		// 0x0004234e
		lea	edx, [esp + 0xc]		// 0x00042350
		push	edx		// 0x00042354
		call	nxIceAddUniqueAxis		// 0x00042355
L4235a:
		mov	ecx, dword ptr [esp + 0x20]		// 0x0004235a
		mov	eax, dword ptr [esp + 0x34]		// 0x0004235e
		add	ecx, 4		// 0x00042362
		inc	ebp		// 0x00042365
		dec	eax		// 0x00042366
		mov	dword ptr [esp + 0x20], ecx		// 0x00042367
		mov	dword ptr [esp + 0x34], eax		// 0x0004236b
		jne	L421ae		// 0x0004236f
		mov	ebp, dword ptr [esp + 0x48]		// 0x00042375
		mov	ecx, dword ptr [esp + 0x44]		// 0x00042379
L4237d:
		dec	dword ptr [esp + 0x38]		// 0x0004237d
		jne	L42183		// 0x00042381
		mov	esi, dword ptr [esp + 0xb8]		// 0x00042387
L4238e:
		mov	eax, dword ptr [esp + 0x1c]		// 0x0004238e
		mov	ecx, dword ptr [esp + 0xac]		// 0x00042392
		inc	eax		// 0x00042399
		cmp	eax, ecx		// 0x0004239a
		mov	dword ptr [esp + 0x1c], eax		// 0x0004239c
		jb	L42140		// 0x000423a0
		mov	ecx, dword ptr [esp + 0x18]		// 0x000423a6
L423aa:
		mov	edi, dword ptr [ecx + 8]		// 0x000423aa
		mov	eax, 0xaaaaaaab		// 0x000423ad
		mul	dword ptr [ecx + 4]		// 0x000423b2
		mov	ebp, edx		// 0x000423b5
		shr	ebp, 1		// 0x000423b7
		test	ebp, ebp		// 0x000423b9
		je	L4243e		// 0x000423bb
L423c1:
		mov	ecx, dword ptr [esp + 0xb4]		// 0x000423c1
		mov	edx, dword ptr [esp + 0xc4]		// 0x000423c8
		lea	eax, [esp + 0x18]		// 0x000423cf
		push	eax		// 0x000423d3
		mov	eax, dword ptr [esp + 0xc4]		// 0x000423d4
		push	ebx		// 0x000423db
		push	ecx		// 0x000423dc
		mov	ecx, dword ptr [esp + 0xb4]		// 0x000423dd
		push	edx		// 0x000423e4
		mov	edx, dword ptr [esp + 0xcc]		// 0x000423e5
		push	eax		// 0x000423ec
		mov	esi, edi		// 0x000423ed
		push	ecx		// 0x000423ef
		mov	ecx, dword ptr [esp + 0xd0]		// 0x000423f0
		mov	eax, esi		// 0x000423f7
		dec	ebp		// 0x000423f9
		add	edi, 0xc		// 0x000423fa
		call	nxConvexMeshAxis		// 0x000423fd
		add	esp, 0x18		// 0x00042402
		test	al, al		// 0x00042405
		je	L4244a		// 0x00042407
		fld	dword ptr [esp + 0x18]		// 0x00042409
		mov	ecx, dword ptr [esp + 0xcc]		// 0x0004240d
		fcomp	dword ptr [ecx]		// 0x00042414
		fnstsw	ax		// 0x00042416
		test	ah, 5		// 0x00042418
		jp	L4243a		// 0x0004241b
		mov	edx, dword ptr [esp + 0x18]		// 0x0004241d
		mov	eax, dword ptr [esp + 0xc8]		// 0x00042421
		mov	dword ptr [ecx], edx		// 0x00042428
		mov	ecx, dword ptr [esi]		// 0x0004242a
		mov	dword ptr [eax], ecx		// 0x0004242c
		mov	edx, dword ptr [esi + 4]		// 0x0004242e
		mov	dword ptr [eax + 4], edx		// 0x00042431
		mov	ecx, dword ptr [esi + 8]		// 0x00042434
		mov	dword ptr [eax + 8], ecx		// 0x00042437
L4243a:
		test	ebp, ebp		// 0x0004243a
		jne	L423c1		// 0x0004243c
L4243e:
		pop	edi		// 0x0004243e
		pop	esi		// 0x0004243f
		mov	al, 1		// 0x00042440
		pop	ebp		// 0x00042442
		add	esp, 0x94		// 0x00042443
		ret		// 0x00042449
L4244a:
		pop	edi		// 0x0004244a
		pop	esi		// 0x0004244b
		xor	al, al		// 0x0004244c
		pop	ebp		// 0x0004244e
		add	esp, 0x94		// 0x0004244f
		ret		// 0x00042455
		}
	}

// phys_fn_001840 (0x00042460, 247 B)
// A triangle's axes (sixteen stack arguments the caller cleans: the Container
// of edge directions, the polygon index, the best depth so far and its axis,
// the depth out and the index out, the scratch record, the polygon interface,
// the 4x4 taking the hull into the mesh's frame, the support map, the group's
// triangle count and indices, the mesh, a second triangle count and indices,
// and the triangle's normal; ebx the axis out). The triangle's normal is tested
// against the group through 001833 (false when it separates); the smaller of
// its depth and the given one keeps its axis (`test ah, 5; jp`), the second
// list's active edges near the polygon go to the Container (001834), and the
// axis, the depth and the given polygon index are stored. True.
__declspec(naked) bool nxConvexMeshEdgeAxes()
	{
	__asm
		{
		sub	esp, 0x20		// 0x00042460
		mov	edx, dword ptr [esp + 0x50]		// 0x00042463
		push	ebp		// 0x00042467
		mov	ebp, dword ptr [esp + 0x58]		// 0x00042468
		push	esi		// 0x0004246c
		mov	esi, dword ptr [esp + 0x68]		// 0x0004246d
		push	edi		// 0x00042471
		lea	eax, [esp + 0x10]		// 0x00042472
		push	eax		// 0x00042476
		mov	eax, dword ptr [esp + 0x5c]		// 0x00042477
		lea	ecx, [esp + 0x10]		// 0x0004247b
		push	ecx		// 0x0004247f
		mov	ecx, dword ptr [esp + 0x5c]		// 0x00042480
		push	edx		// 0x00042484
		mov	edx, dword ptr [esp + 0x58]		// 0x00042485
		push	eax		// 0x00042489
		mov	eax, dword ptr [esp + 0x58]		// 0x0004248a
		push	ecx		// 0x0004248e
		mov	ecx, dword ptr [esp + 0x64]		// 0x0004248f
		push	edx		// 0x00042493
		push	eax		// 0x00042494
		lea	edi, [esp + 0x3c]		// 0x00042495
		mov	edx, ebp		// 0x00042499
		mov	dword ptr [esp + 0x28], 0x7f7fffff		// 0x0004249b
		call	nxConvexMeshTriangleAxis		// 0x000424a3
		add	esp, 0x1c		// 0x000424a8
		test	al, al		// 0x000424ab
		jne	L424b6		// 0x000424ad
		pop	edi		// 0x000424af
		pop	esi		// 0x000424b0
		pop	ebp		// 0x000424b1
		add	esp, 0x20		// 0x000424b2
		ret		// 0x000424b5
L424b6:
		mov	eax, dword ptr [esp + 0x3c]		// 0x000424b6
		fld	dword ptr [esp + 0xc]		// 0x000424ba
		mov	edx, dword ptr [eax]		// 0x000424be
		fcomp	dword ptr [esp + 0x38]		// 0x000424c0
		mov	ecx, dword ptr [esp + 0x38]		// 0x000424c4
		mov	dword ptr [esp + 0x10], ecx		// 0x000424c8
		mov	ecx, dword ptr [eax + 4]		// 0x000424cc
		mov	dword ptr [esp + 0x14], edx		// 0x000424cf
		mov	edx, dword ptr [eax + 8]		// 0x000424d3
		fnstsw	ax		// 0x000424d6
		mov	dword ptr [esp + 0x18], ecx		// 0x000424d8
		test	ah, 5		// 0x000424dc
		mov	dword ptr [esp + 0x1c], edx		// 0x000424df
		jp	L42505		// 0x000424e3
		mov	eax, dword ptr [esp + 0xc]		// 0x000424e5
		mov	ecx, dword ptr [esp + 0x20]		// 0x000424e9
		mov	edx, dword ptr [esp + 0x24]		// 0x000424ed
		mov	dword ptr [esp + 0x10], eax		// 0x000424f1
		mov	eax, dword ptr [esp + 0x28]		// 0x000424f5
		mov	dword ptr [esp + 0x14], ecx		// 0x000424f9
		mov	dword ptr [esp + 0x18], edx		// 0x000424fd
		mov	dword ptr [esp + 0x1c], eax		// 0x00042501
L42505:
		mov	esi, dword ptr [esp + 0x34]		// 0x00042505
		mov	ecx, dword ptr [esp + 0x68]		// 0x00042509
		mov	edx, dword ptr [esp + 0x64]		// 0x0004250d
		mov	eax, dword ptr [esp + 0x30]		// 0x00042511
		push	esi		// 0x00042515
		push	ebp		// 0x00042516
		push	ecx		// 0x00042517
		mov	ecx, dword ptr [esp + 0x58]		// 0x00042518
		push	edx		// 0x0004251c
		push	eax		// 0x0004251d
		mov	eax, dword ptr [esp + 0x64]		// 0x0004251e
		call	nxConvexMeshEdgeDirections		// 0x00042522
		mov	ecx, dword ptr [esp + 0x28]		// 0x00042527
		mov	edx, dword ptr [esp + 0x2c]		// 0x0004252b
		mov	eax, dword ptr [esp + 0x30]		// 0x0004252f
		mov	dword ptr [ebx], ecx		// 0x00042533
		mov	ecx, dword ptr [esp + 0x24]		// 0x00042535
		mov	dword ptr [ebx + 4], edx		// 0x00042539
		mov	edx, dword ptr [esp + 0x54]		// 0x0004253c
		mov	dword ptr [ebx + 8], eax		// 0x00042540
		mov	eax, dword ptr [esp + 0x58]		// 0x00042543
		add	esp, 0x14		// 0x00042547
		pop	edi		// 0x0004254a
		mov	dword ptr [edx], ecx		// 0x0004254b
		mov	dword ptr [eax], esi		// 0x0004254d
		pop	esi		// 0x0004254f
		mov	al, 1		// 0x00042550
		pop	ebp		// 0x00042552
		add	esp, 0x20		// 0x00042553
		ret		// 0x00042556
		}
	}

// phys_fn_001842 (0x00042560, 623 B)
// The contacts of a hull polygon and a triangle (register arguments: edx the
// polygon index, ebx the hull's world 4x4, esi the mesh's, edi the triangle's
// normal in the mesh's frame; thirteen stack arguments: the contact normal,
// the polygon interface, the triangle's three vertices, its feature id, the
// mesh's shape, the hull's shape, the two relative 4x4s, the sink, 0 and a
// feature word). The polygon (slot 4) and the triangle's normal are rotated
// into the world; the one whose normal lies further along the contact normal
// (|n . N| compared, `test ah, 5; jp`) is the reference: 001909 gets the
// polygon first when the triangle's does not (the triangle as a polygon of
// three references {0, 1, 2} on the stack), the triangle first otherwise, with
// the hull shape's +0xda word and -1 as the other feature word.
__declspec(naked) void nxConvexMeshContacts()
	{
	__asm
		{
		sub	esp, 0x48		// 0x00042560
		mov	ecx, dword ptr [esp + 0x50]		// 0x00042563
		mov	eax, dword ptr [ecx]		// 0x00042567
		push	ebp		// 0x00042569
		mov	ebp, dword ptr [esp + 0x50]		// 0x0004256a
		push	edx		// 0x0004256e
		call	dword ptr [eax + 0x10]		// 0x0004256f
		fld	dword ptr [ebx + 0x10]		// 0x00042572
		mov	edx, eax		// 0x00042575
		fmul	dword ptr [edx + 0x10]		// 0x00042577
		lea	ecx, [edx + 0xc]		// 0x0004257a
		fld	dword ptr [ebx + 0x20]		// 0x0004257d
		mov	eax, dword ptr [esp + 0x58]		// 0x00042580
		fmul	dword ptr [ecx + 8]		// 0x00042584
		mov	eax, dword ptr [eax]		// 0x00042587
		mov	dword ptr [esp + 0x50], edx		// 0x00042589
		faddp	st(1), st		// 0x0004258d
		fld	dword ptr [ebx]		// 0x0004258f
		fmul	dword ptr [ecx]		// 0x00042591
		faddp	st(1), st		// 0x00042593
		fstp	dword ptr [esp + 4]		// 0x00042595
		fld	dword ptr [ebx + 0x14]		// 0x00042599
		fmul	dword ptr [ecx + 4]		// 0x0004259c
		fld	dword ptr [ebx + 4]		// 0x0004259f
		fmul	dword ptr [ecx]		// 0x000425a2
		faddp	st(1), st		// 0x000425a4
		fld	dword ptr [ebx + 0x24]		// 0x000425a6
		fmul	dword ptr [ecx + 8]		// 0x000425a9
		faddp	st(1), st		// 0x000425ac
		fstp	dword ptr [esp + 8]		// 0x000425ae
		fld	dword ptr [ebx + 0x18]		// 0x000425b2
		fmul	dword ptr [ecx + 4]		// 0x000425b5
		fld	dword ptr [ebx + 8]		// 0x000425b8
		fmul	dword ptr [ecx]		// 0x000425bb
		faddp	st(1), st		// 0x000425bd
		fld	dword ptr [ebx + 0x28]		// 0x000425bf
		fmul	dword ptr [ecx + 8]		// 0x000425c2
		mov	dword ptr [esp + 0x28], eax		// 0x000425c5
		mov	eax, dword ptr [esp + 0x58]		// 0x000425c9
		mov	eax, dword ptr [eax + 4]		// 0x000425cd
		faddp	st(1), st		// 0x000425d0
		mov	dword ptr [esp + 0x2c], eax		// 0x000425d2
		mov	eax, dword ptr [esp + 0x58]		// 0x000425d6
		mov	eax, dword ptr [eax + 8]		// 0x000425da
		fstp	dword ptr [esp + 0xc]		// 0x000425dd
		mov	dword ptr [esp + 0x30], eax		// 0x000425e1
		fld	dword ptr [esi + 0x20]		// 0x000425e5
		mov	eax, dword ptr [esp + 0x5c]		// 0x000425e8
		fmul	dword ptr [edi + 8]		// 0x000425ec
		mov	eax, dword ptr [eax]		// 0x000425ef
		fld	dword ptr [esi + 0x10]		// 0x000425f1
		mov	dword ptr [esp + 0x34], eax		// 0x000425f4
		fmul	dword ptr [edi + 4]		// 0x000425f8
		mov	eax, dword ptr [esp + 0x5c]		// 0x000425fb
		mov	eax, dword ptr [eax + 4]		// 0x000425ff
		mov	dword ptr [esp + 0x38], eax		// 0x00042602
		faddp	st(1), st		// 0x00042606
		mov	eax, dword ptr [esp + 0x5c]		// 0x00042608
		fld	dword ptr [edi]		// 0x0004260c
		mov	eax, dword ptr [eax + 8]		// 0x0004260e
		fmul	dword ptr [esi]		// 0x00042611
		mov	dword ptr [esp + 0x3c], eax		// 0x00042613
		faddp	st(1), st		// 0x00042617
		fstp	dword ptr [esp + 0x10]		// 0x00042619
		fld	dword ptr [esi + 4]		// 0x0004261d
		fmul	dword ptr [edi]		// 0x00042620
		fld	dword ptr [esi + 0x24]		// 0x00042622
		fmul	dword ptr [edi + 8]		// 0x00042625
		faddp	st(1), st		// 0x00042628
		fld	dword ptr [esi + 0x14]		// 0x0004262a
		fmul	dword ptr [edi + 4]		// 0x0004262d
		faddp	st(1), st		// 0x00042630
		fstp	dword ptr [esp + 0x14]		// 0x00042632
		fld	dword ptr [esi + 8]		// 0x00042636
		fmul	dword ptr [edi]		// 0x00042639
		fld	dword ptr [esi + 0x28]		// 0x0004263b
		fmul	dword ptr [edi + 8]		// 0x0004263e
		faddp	st(1), st		// 0x00042641
		fld	dword ptr [esi + 0x18]		// 0x00042643
		fmul	dword ptr [edi + 4]		// 0x00042646
		faddp	st(1), st		// 0x00042649
		fstp	dword ptr [esp + 0x18]		// 0x0004264b
		fld	dword ptr [esp + 4]		// 0x0004264f
		mov	eax, dword ptr [esp + 0x60]		// 0x00042653
		fmul	dword ptr [ebp]		// 0x00042657
		mov	eax, dword ptr [eax]		// 0x0004265a
		fld	dword ptr [esp + 0xc]		// 0x0004265c
		mov	dword ptr [esp + 0x40], eax		// 0x00042660
		fmul	dword ptr [ebp + 8]		// 0x00042664
		mov	eax, dword ptr [esp + 0x60]		// 0x00042667
		mov	eax, dword ptr [eax + 4]		// 0x0004266b
		mov	dword ptr [esp + 0x44], eax		// 0x0004266e
		faddp	st(1), st		// 0x00042672
		mov	eax, dword ptr [esp + 0x60]		// 0x00042674
		fld	dword ptr [esp + 8]		// 0x00042678
		mov	eax, dword ptr [eax + 8]		// 0x0004267c
		fmul	dword ptr [ebp + 4]		// 0x0004267f
		mov	dword ptr [esp + 0x48], eax		// 0x00042682
		mov	dword ptr [esp + 0x1c], 0		// 0x00042686
		mov	dword ptr [esp + 0x20], 1		// 0x0004268e
		faddp	st(1), st		// 0x00042696
		mov	dword ptr [esp + 0x24], 2		// 0x00042698
		fabs		// 0x000426a0
		fld	dword ptr [esp + 0x10]		// 0x000426a2
		fmul	dword ptr [ebp]		// 0x000426a6
		fld	dword ptr [esp + 0x18]		// 0x000426a9
		fmul	dword ptr [ebp + 8]		// 0x000426ad
		faddp	st(1), st		// 0x000426b0
		fld	dword ptr [esp + 0x14]		// 0x000426b2
		fmul	dword ptr [ebp + 4]		// 0x000426b6
		mov	ebp, dword ptr [esp + 0x64]		// 0x000426b9
		faddp	st(1), st		// 0x000426bd
		fabs		// 0x000426bf
		fxch	st(1)		// 0x000426c1
		fxch	st(1)		// 0x000426c3
		fcompp		// 0x000426c5
		fnstsw	ax		// 0x000426c7
		test	ah, 5		// 0x000426c9
		mov	eax, dword ptr [esp + 0x6c]		// 0x000426cc
		movzx	eax, word ptr [eax + 0xda]		// 0x000426d0
		jp	L42754		// 0x000426d7
		push	ebp		// 0x000426d9
		mov	ebp, dword ptr [esp + 0x84]		// 0x000426da
		push	-1		// 0x000426e1
		push	ebp		// 0x000426e3
		push	eax		// 0x000426e4
		mov	eax, dword ptr [esp + 0x8c]		// 0x000426e5
		push	eax		// 0x000426ec
		mov	eax, dword ptr [esp + 0x8c]		// 0x000426ed
		push	0		// 0x000426f4
		push	eax		// 0x000426f6
		mov	eax, dword ptr [esp + 0x84]		// 0x000426f7
		push	eax		// 0x000426fe
		mov	eax, dword ptr [esp + 0x8c]		// 0x000426ff
		push	eax		// 0x00042706
		mov	eax, dword ptr [esp + 0x98]		// 0x00042707
		push	eax		// 0x0004270e
		mov	eax, dword ptr [esp + 0x98]		// 0x0004270f
		push	eax		// 0x00042716
		lea	eax, [esp + 0x30]		// 0x00042717
		push	eax		// 0x0004271b
		push	edi		// 0x0004271c
		push	esi		// 0x0004271d
		lea	eax, [esp + 0x54]		// 0x0004271e
		push	eax		// 0x00042722
		lea	eax, [esp + 0x64]		// 0x00042723
		push	eax		// 0x00042727
		push	3		// 0x00042728
		push	ecx		// 0x0004272a
		mov	ecx, dword ptr [edx + 4]		// 0x0004272b
		push	ebx		// 0x0004272e
		push	ecx		// 0x0004272f
		mov	ecx, dword ptr [esp + 0xa4]		// 0x00042730
		mov	edx, dword ptr [ecx]		// 0x00042737
		call	dword ptr [edx + 8]		// 0x00042739
		push	eax		// 0x0004273c
		mov	eax, dword ptr [esp + 0xa4]		// 0x0004273d
		mov	ecx, dword ptr [eax]		// 0x00042744
		push	ecx		// 0x00042746
		call	NxConvexPolygonContacts		// 0x00042747
		add	esp, 0x58		// 0x0004274c
		pop	ebp		// 0x0004274f
		add	esp, 0x48		// 0x00042750
		ret		// 0x00042753
L42754:
		push	-1		// 0x00042754
		push	ebp		// 0x00042756
		push	eax		// 0x00042757
		mov	eax, dword ptr [esp + 0x8c]		// 0x00042758
		push	eax		// 0x0004275f
		mov	eax, dword ptr [esp + 0x8c]		// 0x00042760
		push	0		// 0x00042767
		push	eax		// 0x00042769
		mov	eax, dword ptr [esp + 0x90]		// 0x0004276a
		push	eax		// 0x00042771
		mov	eax, dword ptr [esp + 0x88]		// 0x00042772
		push	eax		// 0x00042779
		mov	eax, dword ptr [esp + 0x88]		// 0x0004277a
		push	eax		// 0x00042781
		mov	eax, dword ptr [esp + 0x94]		// 0x00042782
		push	eax		// 0x00042789
		mov	eax, dword ptr [esp + 0x9c]		// 0x0004278a
		push	eax		// 0x00042791
		lea	eax, [esp + 0x3c]		// 0x00042792
		push	eax		// 0x00042796
		push	ecx		// 0x00042797
		mov	ecx, dword ptr [edx + 4]		// 0x00042798
		push	ebx		// 0x0004279b
		push	ecx		// 0x0004279c
		mov	ecx, dword ptr [esp + 0x90]		// 0x0004279d
		mov	edx, dword ptr [ecx]		// 0x000427a4
		call	dword ptr [edx + 8]		// 0x000427a6
		push	eax		// 0x000427a9
		mov	eax, dword ptr [esp + 0x90]		// 0x000427aa
		mov	ecx, dword ptr [eax]		// 0x000427b1
		push	ecx		// 0x000427b3
		push	edi		// 0x000427b4
		push	esi		// 0x000427b5
		lea	edx, [esp + 0x68]		// 0x000427b6
		push	edx		// 0x000427ba
		lea	eax, [esp + 0x78]		// 0x000427bb
		push	eax		// 0x000427bf
		push	3		// 0x000427c0
		call	NxConvexPolygonContacts		// 0x000427c2
		add	esp, 0x58		// 0x000427c7
		pop	ebp		// 0x000427ca
		add	esp, 0x48		// 0x000427cb
		ret		// 0x000427ce
		}
	}

// ---------------------------------------------------------------------------
// Sub-unit M's second half (convex-mesh gap Task 2i): the callers of the rows
// above and the unit's two entries. 001847 is the convex/height-field entry
// (001876 calls it when one mesh is a height field, +0x7c != 0xff, and the
// other convex): it queries the height field's OPCODE model with the convex
// shape's world box and hands the touched triangles to 001844. 001853 (five
// bytes, a jmp) and 001851 are the convex/triangle-mesh entry (001876 calls
// 001853 when neither mesh is a height field and one is convex): 001851 builds
// the convex mesh's local bounds box in the world and hands it to 001849, which
// queries the mesh's model, groups the touched triangles by the mesh's convex
// parts (+0x94) and flat parts (+0x98) and runs the separating-axis rows above
// per group. The contract had the two entries' roles the other way round
// (Task 2i erratum): the height-field test is 001876's, on +0x7c.
//
// The context is the scene record the matrix-A entries receive (their fourth
// argument): +0x04/+0x08 the visited array and +0x14 the stamp (000505), +0x110
// an OBBCollider (its flags at +0x04, the touched-primitive Container pointer at
// +0x10; OPC_FIRST_CONTACT 1, OPC_TEMPORAL_COHERENCE 2, OPC_CONTACT 4,
// OPC_NO_PRIMITIVE_TESTS 0x10), +0x244 its OBBCache, +0x4e0/+0x4f0 the axis
// Containers the rows above fill. Both entries report a failed query through
// FoundationSDK::error(4, this file, line, 0, "Opcode is not OK.") after the
// `cmp [instance], 0; jne; int3` guard, and return.
//
// Every row is the listing's instructions, naked. They reach the vendored
// OBBCollider::Collide (005067), RadixSort (005157, 005159, 005163, 005177),
// Triangle::Area / Center (005179, 005183), Prunable::UpdateWorldAABB (004886)
// and ConvexHull::ComputeVertexNormals (001461) through /alternatename aliases,
// and the Foundation's instance and error() through their import slots, so
// every call and slot read stays direct as in the listing.
extern "C" void nxConvexMeshCallObbCollide();		// 005067
#pragma comment(linker, "/alternatename:_nxConvexMeshCallObbCollide=?Collide@OBBCollider@Opcode@@QAE_NAAUOBBCache@2@ABVOBB@IceMaths@@ABVModel@2@PBVMatrix4x4@5@3@Z")
extern "C" void nxConvexMeshCallRadixSortCtor();	// 005157
#pragma comment(linker, "/alternatename:_nxConvexMeshCallRadixSortCtor=??0RadixSort@IceCore@@QAE@XZ")
extern "C" void nxConvexMeshCallRadixSortDtor();	// 005159
#pragma comment(linker, "/alternatename:_nxConvexMeshCallRadixSortDtor=??1RadixSort@IceCore@@QAE@XZ")
extern "C" void nxConvexMeshCallRadixSortSort();	// 005163
#pragma comment(linker, "/alternatename:_nxConvexMeshCallRadixSortSort=?Sort@RadixSort@IceCore@@QAEAAV12@PBIIW4RadixHint@2@@Z")
extern "C" void nxConvexMeshCallRadixSortSetRankBuffers();	// 005177
#pragma comment(linker, "/alternatename:_nxConvexMeshCallRadixSortSetRankBuffers=?SetRankBuffers@RadixSort@IceCore@@QAE_NPAI0@Z")
extern "C" void nxConvexMeshCallTriangleArea();		// 005179
#pragma comment(linker, "/alternatename:_nxConvexMeshCallTriangleArea=?Area@Triangle@IceMaths@@QBEMXZ")
extern "C" void nxConvexMeshCallTriangleCenter();	// 005183
#pragma comment(linker, "/alternatename:_nxConvexMeshCallTriangleCenter=?Center@Triangle@IceMaths@@QBEXAAVPoint@2@@Z")
extern "C" void nxConvexMeshCallUpdateWorldAABB();	// 004886
#pragma comment(linker, "/alternatename:_nxConvexMeshCallUpdateWorldAABB=?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z")
extern "C" void nxConvexMeshCallVertexNormals();	// 001461, ConvexHull::ComputeVertexNormals
#pragma comment(linker, "/alternatename:_nxConvexMeshCallVertexNormals=?ComputeVertexNormals@ConvexHull@@QAE_NXZ")
// The import slots 0x101041b0 (FoundationSDK::instance) and 0x101041b4
// (FoundationSDK::error).
extern "C" void* nxConvexMeshFoundationInstanceSlot;
#pragma comment(linker, "/alternatename:_nxConvexMeshFoundationInstanceSlot=__imp_?instance@FoundationSDK@NxFoundation@@0PAV12@A")
extern "C" void* nxConvexMeshFoundationErrorSlot;
#pragma comment(linker, "/alternatename:_nxConvexMeshFoundationErrorSlot=__imp_?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ")

// .rdata 0x101043cc (0.5f) and 0x1010687c (-1.0f); the report's strings
// (0x10107c7c, this unit's __FILE__, and 0x10107bc4, pooled with the other
// three OPCODE-failure reports).
static const float kConvexMeshHalf = 0.5f;
static const float kConvexMeshMinusOne = -1.0f;
static const char kConvexHeightfieldFile[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\ContactConvexHeightfield.cpp";
static const char kConvexMeshOpcodeNotOk[] = "Opcode is not OK.";

void nxConvexHeightfieldContacts();
void nxConvexMeshContact();
void nxContactConvexMeshEntry();

// phys_fn_001844 (0x000427d0, 2170 B)
// phys_fn_001846 (0x00043050, 640 B)
// The convex hull against the height field's touched triangles (cdecl, eight
// arguments: the context, the touched count and indices, the ConvexHull, the
// convex's 4x4 pose, the convex shape, the height-field shape, the sink;
// ebp-framed, alloca; 001846 is its continuation, one extent with the filler
// between them). The hull's vertices go to the world through the pose, and its
// vertex normals (001461 first when +0x14 is null), negated, through the pose's
// rotation, into two alloca arrays; a third alloca holds a byte per hull vertex
// (a contact already emitted), zeroed. The height field's up vector is +-1.0f
// (-1.0f when bit 3 of mesh +0x78 is set) on component (+0x78 & 3), and its two
// other axes are the low bytes of 0x1000201 >> (8 * mesh +0x7c). The mesh's
// vertex normals (internal +0x18, mesh +0x20) are built on first use (002081)
// and its EdgeList (+0x88) by 002188. Per touched triangle (its material from
// +0x18 or 0xffff; its index through the remap +0x1c when that is non-null, for
// the contacts): the world triangle through the height-field shape's pose and
// its plane (001760); each hull vertex not yet emitted whose negated normal
// faces against the plane (dot < 0.0f), which lies on or below it (`test ah, 5;
// jp`: NaN is not) and inside the triangle in the plane of the two other axes
// (two 2D cross terms and a third, their signs combined as bits, `jns`) is
// emitted (000875: the plane distance, the vertex, the plane normal, feature
// words -1 and the triangle) and marked. Then each of the triangle's three
// vertices not yet stamped in the context's visited array casts a ray along the
// up vector out of the hull (001822 with the pose); a hit emits the vertex moved
// by |t| along the ray's direction out, with the mesh's vertex normal through
// the shape's rotation, and stamps the vertex. Then every hull edge (+0x38 /
// +0x3c, 001502 when either is zero) against each of the triangle's active edges
// (EdgeList face word bit 31) along -up (001855): a crossing emits the closest
// point (001692), the normalised cross of the two edges (left as it is when its
// length is exactly zero) turned to face up (`test ah, 0x41; jne`) and -t as the
// separation.
__declspec(naked) void nxConvexHeightfieldContacts()
	{
	__asm
		{
		push	ebp		// 0x000427d0
		lea	ebp, [esp - 0x58]		// 0x000427d1
		sub	esp, 0x1bc		// 0x000427d5
		mov	eax, dword ptr [ebp + 0x6c]		// 0x000427db
		push	ebx		// 0x000427de
		mov	ebx, dword ptr [ebp + 0x78]		// 0x000427df
		push	esi		// 0x000427e2
		mov	esi, dword ptr [ebx + 0xe0]		// 0x000427e3
		push	edi		// 0x000427e9
		mov	edi, dword ptr [eax + 0xc]		// 0x000427ea
		mov	dword ptr [ebp + 0x2c], edi		// 0x000427ed
		lea	edi, [edi + edi*2]		// 0x000427f0
		shl	edi, 2		// 0x000427f3
		mov	eax, edi		// 0x000427f6
		add	eax, 3		// 0x000427f8
		and	eax, 0xfffffffc		// 0x000427fb
		mov	dword ptr [ebp + 0x44], esi		// 0x000427fe
		call	_chkstk		// 0x00042801
		mov	eax, edi		// 0x00042806
		add	eax, 3		// 0x00042808
		and	eax, 0xfffffffc		// 0x0004280b
		mov	dword ptr [ebp - 8], esp		// 0x0004280e
		call	_chkstk		// 0x00042811
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00042816
		mov	eax, dword ptr [ecx + 0x14]		// 0x00042819
		test	eax, eax		// 0x0004281c
		mov	edi, esp		// 0x0004281e
		mov	dword ptr [ebp - 0x54], edi		// 0x00042820
		jne	L4282d		// 0x00042823
		call	nxConvexMeshCallVertexNormals		// 0x00042825
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x0004282a
L4282d:
		mov	eax, dword ptr [ebp + 0x2c]		// 0x0004282d
		test	eax, eax		// 0x00042830
		mov	ecx, dword ptr [ecx + 0x14]		// 0x00042832
		jbe	L429d2		// 0x00042835
		mov	eax, dword ptr [ebp - 8]		// 0x0004283b
		mov	esi, ecx		// 0x0004283e
		sub	esi, edi		// 0x00042840
		mov	dword ptr [ebp + 4], esi		// 0x00042842
		mov	esi, eax		// 0x00042845
		sub	esi, edi		// 0x00042847
		mov	dword ptr [ebp + 0x3c], esi		// 0x00042849
		mov	esi, 0xfffffff8		// 0x0004284c
		lea	edx, [eax + 4]		// 0x00042851
		sub	esi, edi		// 0x00042854
		mov	dword ptr [ebp + 0x54], ecx		// 0x00042856
		sub	ecx, eax		// 0x00042859
		mov	eax, dword ptr [ebp + 0x2c]		// 0x0004285b
		mov	dword ptr [ebp + 0x50], edx		// 0x0004285e
		lea	edx, [edi + 8]		// 0x00042861
		mov	dword ptr [ebp + 0xc], esi		// 0x00042864
		mov	dword ptr [ebp + 0x10], ecx		// 0x00042867
		mov	dword ptr [ebp + 0x38], eax		// 0x0004286a
		jmp	L42872		// 0x0004286d
L4286f:
		mov	esi, dword ptr [ebp + 0xc]		// 0x0004286f
L42872:
		mov	eax, dword ptr [ebp + 0x6c]		// 0x00042872
		lea	ecx, [esi + edx]		// 0x00042875
		mov	esi, dword ptr [eax + 0x10]		// 0x00042878
		mov	eax, dword ptr [ebp + 0x70]		// 0x0004287b
		fld	dword ptr [eax + 0x10]		// 0x0004287e
		add	ecx, esi		// 0x00042881
		fmul	dword ptr [ecx + 4]		// 0x00042883
		lea	esi, [ebp - 0x2c]		// 0x00042886
		fld	dword ptr [eax]		// 0x00042889
		lea	edi, [ebp - 0x128]		// 0x0004288b
		fmul	dword ptr [ecx]		// 0x00042891
		faddp	st(1), st		// 0x00042893
		fld	dword ptr [eax + 0x20]		// 0x00042895
		fmul	dword ptr [ecx + 8]		// 0x00042898
		faddp	st(1), st		// 0x0004289b
		fadd	dword ptr [eax + 0x30]		// 0x0004289d
		fld	dword ptr [eax + 0x24]		// 0x000428a0
		fmul	dword ptr [ecx + 8]		// 0x000428a3
		fld	dword ptr [eax + 4]		// 0x000428a6
		fmul	dword ptr [ecx]		// 0x000428a9
		faddp	st(1), st		// 0x000428ab
		fld	dword ptr [eax + 0x14]		// 0x000428ad
		fmul	dword ptr [ecx + 4]		// 0x000428b0
		faddp	st(1), st		// 0x000428b3
		fadd	dword ptr [eax + 0x34]		// 0x000428b5
		fld	dword ptr [eax + 0x18]		// 0x000428b8
		fmul	dword ptr [ecx + 4]		// 0x000428bb
		fld	dword ptr [ecx + 8]		// 0x000428be
		fmul	dword ptr [eax + 0x28]		// 0x000428c1
		faddp	st(1), st		// 0x000428c4
		fld	dword ptr [eax + 8]		// 0x000428c6
		fmul	dword ptr [ecx]		// 0x000428c9
		faddp	st(1), st		// 0x000428cb
		fadd	dword ptr [eax + 0x38]		// 0x000428cd
		fstp	dword ptr [ebp - 0x90]		// 0x000428d0
		mov	ecx, dword ptr [ebp - 0x90]		// 0x000428d6
		mov	dword ptr [edx], ecx		// 0x000428dc
		fxch	st(1)		// 0x000428de
		fstp	dword ptr [edx - 8]		// 0x000428e0
		fstp	dword ptr [edx - 4]		// 0x000428e3
		mov	ecx, dword ptr [eax]		// 0x000428e6
		mov	dword ptr [ebp - 0x2c], ecx		// 0x000428e8
		mov	ecx, dword ptr [eax + 4]		// 0x000428eb
		mov	dword ptr [ebp - 0x28], ecx		// 0x000428ee
		mov	ecx, dword ptr [eax + 8]		// 0x000428f1
		mov	dword ptr [ebp - 0x24], ecx		// 0x000428f4
		mov	ecx, dword ptr [eax + 0x10]		// 0x000428f7
		mov	dword ptr [ebp - 0x20], ecx		// 0x000428fa
		mov	ecx, dword ptr [eax + 0x14]		// 0x000428fd
		mov	dword ptr [ebp - 0x1c], ecx		// 0x00042900
		mov	ecx, dword ptr [eax + 0x18]		// 0x00042903
		mov	dword ptr [ebp - 0x18], ecx		// 0x00042906
		mov	ecx, dword ptr [eax + 0x20]		// 0x00042909
		mov	dword ptr [ebp - 0x14], ecx		// 0x0004290c
		mov	ecx, dword ptr [eax + 0x24]		// 0x0004290f
		mov	eax, dword ptr [eax + 0x28]		// 0x00042912
		mov	dword ptr [ebp - 0x10], ecx		// 0x00042915
		mov	dword ptr [ebp - 0xc], eax		// 0x00042918
		mov	eax, dword ptr [ebp + 0x50]		// 0x0004291b
		mov	ecx, 9		// 0x0004291e
		rep movsd		// 0x00042923
		mov	ecx, dword ptr [ebp + 0x54]		// 0x00042925
		mov	esi, dword ptr [ebp + 0x10]		// 0x00042928
		fld	dword ptr [ecx]		// 0x0004292b
		fchs		// 0x0004292d
		fld	dword ptr [esi + eax]		// 0x0004292f
		mov	esi, dword ptr [ebp + 4]		// 0x00042932
		fchs		// 0x00042935
		fstp	dword ptr [ebp + 0x24]		// 0x00042937
		fld	dword ptr [esi + edx]		// 0x0004293a
		fchs		// 0x0004293d
		fstp	dword ptr [ebp + 0x28]		// 0x0004293f
		fld	dword ptr [ebp - 0x110]		// 0x00042942
		fmul	dword ptr [ebp + 0x28]		// 0x00042948
		fld	dword ptr [ebp - 0x11c]		// 0x0004294b
		fmul	dword ptr [ebp + 0x24]		// 0x00042951
		faddp	st(1), st		// 0x00042954
		fld	dword ptr [ebp - 0x128]		// 0x00042956
		fmul	st, st(2)		// 0x0004295c
		faddp	st(1), st		// 0x0004295e
		fld	dword ptr [ebp - 0x10c]		// 0x00042960
		fmul	dword ptr [ebp + 0x28]		// 0x00042966
		mov	esi, dword ptr [ebp + 0x3c]		// 0x00042969
		fld	dword ptr [ebp - 0x118]		// 0x0004296c
		add	eax, 0xc		// 0x00042972
		fmul	dword ptr [ebp + 0x24]		// 0x00042975
		mov	dword ptr [ebp + 0x50], eax		// 0x00042978
		add	ecx, 0xc		// 0x0004297b
		add	edx, 0xc		// 0x0004297e
		faddp	st(1), st		// 0x00042981
		mov	dword ptr [ebp + 0x54], ecx		// 0x00042983
		fld	dword ptr [ebp - 0x124]		// 0x00042986
		fmul	st, st(3)		// 0x0004298c
		faddp	st(1), st		// 0x0004298e
		fld	dword ptr [ebp - 0x108]		// 0x00042990
		fmul	dword ptr [ebp + 0x28]		// 0x00042996
		fld	dword ptr [ebp - 0x114]		// 0x00042999
		fmul	dword ptr [ebp + 0x24]		// 0x0004299f
		faddp	st(1), st		// 0x000429a2
		fld	dword ptr [ebp - 0x120]		// 0x000429a4
		fmul	st, st(4)		// 0x000429aa
		faddp	st(1), st		// 0x000429ac
		fstp	dword ptr [ebp - 0x7c]		// 0x000429ae
		fxch	st(1)		// 0x000429b1
		fstp	dword ptr [eax - 0x10]		// 0x000429b3
		fstp	dword ptr [eax - 0xc]		// 0x000429b6
		mov	eax, dword ptr [ebp + 0x38]		// 0x000429b9
		dec	eax		// 0x000429bc
		fstp	st(0)		// 0x000429bd
		mov	dword ptr [ebp + 0x38], eax		// 0x000429bf
		fld	dword ptr [ebp - 0x7c]		// 0x000429c2
		fstp	dword ptr [esi + edx - 0xc]		// 0x000429c5
		jne	L4286f		// 0x000429c9
		mov	esi, dword ptr [ebp + 0x44]		// 0x000429cf
L429d2:
		mov	ecx, dword ptr [ebp + 0x60]		// 0x000429d2
		call	nxScratchStamp		// 0x000429d5
		mov	edi, dword ptr [ebp + 0x2c]		// 0x000429da
		mov	dword ptr [ebp + 0x10], eax		// 0x000429dd
		mov	eax, edi		// 0x000429e0
		add	eax, 3		// 0x000429e2
		and	eax, 0xfffffffc		// 0x000429e5
		call	_chkstk		// 0x000429e8
		mov	edx, esp		// 0x000429ed
		mov	ecx, edi		// 0x000429ef
		mov	edi, edx		// 0x000429f1
		mov	dword ptr [ebp - 0x48], edx		// 0x000429f3
		mov	edx, ecx		// 0x000429f6
		shr	ecx, 2		// 0x000429f8
		xor	eax, eax		// 0x000429fb
		rep stosd		// 0x000429fd
		mov	ecx, edx		// 0x000429ff
		and	ecx, 3		// 0x00042a01
		rep stosb		// 0x00042a04
		mov	eax, dword ptr [esi + 0x78]		// 0x00042a06
		test	al, 8		// 0x00042a09
		mov	dword ptr [ebp - 0x30], 0		// 0x00042a0b
		mov	dword ptr [ebp - 0x34], 0		// 0x00042a12
		mov	dword ptr [ebp - 0x38], 0		// 0x00042a19
		je	L42a2a		// 0x00042a20
		fld	dword ptr kConvexMeshMinusOne		// 0x00042a22
		jmp	L42a30		// 0x00042a28
L42a2a:
		fld	dword ptr kConvexMeshOne		// 0x00042a2a
L42a30:
		mov	ecx, dword ptr [esi + 0x7c]		// 0x00042a30
		and	eax, 3		// 0x00042a33
		fstp	dword ptr [ebp + eax*4 - 0x38]		// 0x00042a36
		shl	ecx, 3		// 0x00042a3a
		mov	eax, 0x1000201		// 0x00042a3d
		shr	eax, cl		// 0x00042a42
		lea	edi, [esi + 8]		// 0x00042a44
		mov	ecx, eax		// 0x00042a47
		movzx	eax, ah		// 0x00042a49
		mov	dword ptr [ebp - 0x58], eax		// 0x00042a4c
		mov	eax, dword ptr [edi + 0x18]		// 0x00042a4f
		and	ecx, 0xff		// 0x00042a52
		test	eax, eax		// 0x00042a58
		mov	dword ptr [ebp - 0x4c], ecx		// 0x00042a5a
		jne	L42a66		// 0x00042a5d
		mov	ecx, edi		// 0x00042a5f
		call	nxMeshComputeVertexNormals		// 0x00042a61
L42a66:
		mov	eax, dword ptr [ebp + 0x64]		// 0x00042a66
		test	eax, eax		// 0x00042a69
		mov	ecx, dword ptr [edi + 0x18]		// 0x00042a6b
		mov	dword ptr [ebp - 0x8c], ecx		// 0x00042a6e
		je	L432c0		// 0x00042a74
		fld	dword ptr [ebp - 0x38]		// 0x00042a7a
		mov	dword ptr [ebp + 0xc], eax		// 0x00042a7d
		fchs		// 0x00042a80
		fstp	dword ptr [ebp + 0x20]		// 0x00042a82
		fld	dword ptr [ebp - 0x34]		// 0x00042a85
		fchs		// 0x00042a88
		fstp	dword ptr [ebp + 0x24]		// 0x00042a8a
		fld	dword ptr [ebp - 0x30]		// 0x00042a8d
		fchs		// 0x00042a90
		fstp	dword ptr [ebp + 0x28]		// 0x00042a92
		jmp	L42aa0		// 0x00042a95
L42a97:
		mov	esi, dword ptr [ebp + 0x44]		// 0x00042a97
		_emit	0x8d
		_emit	0x9b
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x00042a9a lea ebx, [ebx]
L42aa0:
		mov	ecx, dword ptr [ebp + 0x68]		// 0x00042aa0
		mov	eax, dword ptr [ecx]		// 0x00042aa3
		add	ecx, 4		// 0x00042aa5
		mov	dword ptr [ebp + 0x68], ecx		// 0x00042aa8
		mov	ecx, dword ptr [esi + 0x18]		// 0x00042aab
		test	ecx, ecx		// 0x00042aae
		mov	dword ptr [ebp + 0x64], eax		// 0x00042ab0
		je	L42abf		// 0x00042ab3
		mov	dx, word ptr [ecx + eax*2]		// 0x00042ab5
		mov	word ptr [ebp + 0x38], dx		// 0x00042ab9
		jmp	L42ac6		// 0x00042abd
L42abf:
		mov	dword ptr [ebp + 0x38], 0xffff		// 0x00042abf
L42ac6:
		mov	ecx, dword ptr [esi + 0x10]		// 0x00042ac6
		lea	edx, [eax + eax*2]		// 0x00042ac9
		mov	eax, dword ptr [esi + 0x14]		// 0x00042acc
		shl	edx, 2		// 0x00042acf
		mov	esi, dword ptr [eax + edx]		// 0x00042ad2
		add	eax, edx		// 0x00042ad5
		lea	esi, [esi + esi*2]		// 0x00042ad7
		lea	edi, [ecx + esi*4]		// 0x00042ada
		mov	esi, dword ptr [eax + 4]		// 0x00042add
		mov	dword ptr [ebp + 0x3c], eax		// 0x00042ae0
		mov	eax, dword ptr [eax + 8]		// 0x00042ae3
		lea	esi, [esi + esi*2]		// 0x00042ae6
		lea	eax, [eax + eax*2]		// 0x00042ae9
		lea	eax, [ecx + eax*4]		// 0x00042aec
		lea	esi, [ecx + esi*4]		// 0x00042aef
		mov	ecx, dword ptr [ebp + 0x44]		// 0x00042af2
		cmp	dword ptr [ecx + 0x88], 0		// 0x00042af5
		mov	dword ptr [ebp + 0x4c], edx		// 0x00042afc
		mov	dword ptr [ebp + 0x40], eax		// 0x00042aff
		jne	L42b12		// 0x00042b02
		call	nxConvexMeshCallCreateEdgeList		// 0x00042b04
		mov	edx, dword ptr [ebp + 0x4c]		// 0x00042b09
		mov	ecx, dword ptr [ebp + 0x44]		// 0x00042b0c
		mov	eax, dword ptr [ebp + 0x40]		// 0x00042b0f
L42b12:
		fld	dword ptr [edi + 4]		// 0x00042b12
		mov	ecx, dword ptr [ecx + 0x88]		// 0x00042b15
		fmul	dword ptr [ebx + 0x10]		// 0x00042b1b
		mov	ecx, dword ptr [ecx + 0xc]		// 0x00042b1e
		fld	dword ptr [ebx + 0x14]		// 0x00042b21
		add	ecx, edx		// 0x00042b24
		fmul	dword ptr [edi + 8]		// 0x00042b26
		mov	dword ptr [ebp - 0x6c], ecx		// 0x00042b29
		faddp	st(1), st		// 0x00042b2c
		fld	dword ptr [ebx + 0xc]		// 0x00042b2e
		fmul	dword ptr [edi]		// 0x00042b31
		faddp	st(1), st		// 0x00042b33
		fld	dword ptr [ebx + 0x18]		// 0x00042b35
		fmul	dword ptr [edi]		// 0x00042b38
		fld	dword ptr [ebx + 0x20]		// 0x00042b3a
		fmul	dword ptr [edi + 8]		// 0x00042b3d
		faddp	st(1), st		// 0x00042b40
		fld	dword ptr [edi + 4]		// 0x00042b42
		fmul	dword ptr [ebx + 0x1c]		// 0x00042b45
		faddp	st(1), st		// 0x00042b48
		fstp	dword ptr [ebp - 0xe8]		// 0x00042b4a
		fld	dword ptr [ebx + 0x2c]		// 0x00042b50
		fmul	dword ptr [edi + 8]		// 0x00042b53
		fld	dword ptr [edi + 4]		// 0x00042b56
		fmul	dword ptr [ebx + 0x28]		// 0x00042b59
		faddp	st(1), st		// 0x00042b5c
		fld	dword ptr [ebx + 0x24]		// 0x00042b5e
		fmul	dword ptr [edi]		// 0x00042b61
		faddp	st(1), st		// 0x00042b63
		fstp	dword ptr [ebp - 0xe4]		// 0x00042b65
		fadd	dword ptr [ebx + 0x30]		// 0x00042b6b
		fld	dword ptr [ebp - 0xe8]		// 0x00042b6e
		fadd	dword ptr [ebx + 0x34]		// 0x00042b74
		fld	dword ptr [ebp - 0xe4]		// 0x00042b77
		fadd	dword ptr [ebx + 0x38]		// 0x00042b7d
		fstp	dword ptr [ebp - 0x138]		// 0x00042b80
		mov	edx, dword ptr [ebp - 0x138]		// 0x00042b86
		fxch	st(1)		// 0x00042b8c
		mov	dword ptr [ebp - 0x24], edx		// 0x00042b8e
		fstp	dword ptr [ebp - 0x2c]		// 0x00042b91
		fstp	dword ptr [ebp - 0x28]		// 0x00042b94
		fld	dword ptr [ebx + 0xc]		// 0x00042b97
		fmul	dword ptr [esi]		// 0x00042b9a
		fld	dword ptr [ebx + 0x14]		// 0x00042b9c
		fmul	dword ptr [esi + 8]		// 0x00042b9f
		faddp	st(1), st		// 0x00042ba2
		fld	dword ptr [ebx + 0x10]		// 0x00042ba4
		fmul	dword ptr [esi + 4]		// 0x00042ba7
		faddp	st(1), st		// 0x00042baa
		fld	dword ptr [esi + 8]		// 0x00042bac
		fmul	dword ptr [ebx + 0x20]		// 0x00042baf
		fld	dword ptr [esi + 4]		// 0x00042bb2
		fmul	dword ptr [ebx + 0x1c]		// 0x00042bb5
		faddp	st(1), st		// 0x00042bb8
		fld	dword ptr [ebx + 0x18]		// 0x00042bba
		fmul	dword ptr [esi]		// 0x00042bbd
		faddp	st(1), st		// 0x00042bbf
		fstp	dword ptr [ebp - 0xb8]		// 0x00042bc1
		fld	dword ptr [ebx + 0x2c]		// 0x00042bc7
		fmul	dword ptr [esi + 8]		// 0x00042bca
		fld	dword ptr [ebx + 0x28]		// 0x00042bcd
		fmul	dword ptr [esi + 4]		// 0x00042bd0
		faddp	st(1), st		// 0x00042bd3
		fld	dword ptr [esi]		// 0x00042bd5
		fmul	dword ptr [ebx + 0x24]		// 0x00042bd7
		faddp	st(1), st		// 0x00042bda
		fstp	dword ptr [ebp - 0xb4]		// 0x00042bdc
		fadd	dword ptr [ebx + 0x30]		// 0x00042be2
		fld	dword ptr [ebp - 0xb8]		// 0x00042be5
		fadd	dword ptr [ebx + 0x34]		// 0x00042beb
		fld	dword ptr [ebp - 0xb4]		// 0x00042bee
		fadd	dword ptr [ebx + 0x38]		// 0x00042bf4
		fstp	dword ptr [ebp - 0x12c]		// 0x00042bf7
		mov	ecx, dword ptr [ebp - 0x12c]		// 0x00042bfd
		fxch	st(1)		// 0x00042c03
		mov	dword ptr [ebp - 0x18], ecx		// 0x00042c05
		fstp	dword ptr [ebp - 0x20]		// 0x00042c08
		fstp	dword ptr [ebp - 0x1c]		// 0x00042c0b
		fld	dword ptr [ebx + 0xc]		// 0x00042c0e
		fmul	dword ptr [eax]		// 0x00042c11
		fld	dword ptr [ebx + 0x14]		// 0x00042c13
		fmul	dword ptr [eax + 8]		// 0x00042c16
		lea	ecx, [ebp - 0x20]		// 0x00042c19
		faddp	st(1), st		// 0x00042c1c
		fld	dword ptr [ebx + 0x10]		// 0x00042c1e
		fmul	dword ptr [eax + 4]		// 0x00042c21
		faddp	st(1), st		// 0x00042c24
		fld	dword ptr [ebx + 0x20]		// 0x00042c26
		fmul	dword ptr [eax + 8]		// 0x00042c29
		fld	dword ptr [ebx + 0x1c]		// 0x00042c2c
		fmul	dword ptr [eax + 4]		// 0x00042c2f
		faddp	st(1), st		// 0x00042c32
		fld	dword ptr [ebx + 0x18]		// 0x00042c34
		fmul	dword ptr [eax]		// 0x00042c37
		faddp	st(1), st		// 0x00042c39
		fstp	dword ptr [ebp - 0xc4]		// 0x00042c3b
		fld	dword ptr [ebx + 0x2c]		// 0x00042c41
		fmul	dword ptr [eax + 8]		// 0x00042c44
		fld	dword ptr [ebx + 0x28]		// 0x00042c47
		fmul	dword ptr [eax + 4]		// 0x00042c4a
		faddp	st(1), st		// 0x00042c4d
		fld	dword ptr [eax]		// 0x00042c4f
		lea	eax, [ebp - 0x14]		// 0x00042c51
		fmul	dword ptr [ebx + 0x24]		// 0x00042c54
		push	eax		// 0x00042c57
		push	ecx		// 0x00042c58
		lea	ecx, [ebp - 0x68]		// 0x00042c59
		faddp	st(1), st		// 0x00042c5c
		fstp	dword ptr [ebp - 0xc0]		// 0x00042c5e
		fadd	dword ptr [ebx + 0x30]		// 0x00042c64
		fld	dword ptr [ebp - 0xc4]		// 0x00042c67
		fadd	dword ptr [ebx + 0x34]		// 0x00042c6d
		fld	dword ptr [ebp - 0xc0]		// 0x00042c70
		fadd	dword ptr [ebx + 0x38]		// 0x00042c76
		fstp	dword ptr [ebp - 0x144]		// 0x00042c79
		mov	edx, dword ptr [ebp - 0x144]		// 0x00042c7f
		fxch	st(1)		// 0x00042c85
		mov	dword ptr [ebp - 0xc], edx		// 0x00042c87
		fstp	dword ptr [ebp - 0x14]		// 0x00042c8a
		lea	edx, [ebp - 0x2c]		// 0x00042c8d
		push	edx		// 0x00042c90
		fstp	dword ptr [ebp - 0x10]		// 0x00042c91
		call	NxTrianglePlane		// 0x00042c94
		mov	eax, dword ptr [ebp + 0x2c]		// 0x00042c99
		xor	esi, esi		// 0x00042c9c
		test	eax, eax		// 0x00042c9e
		jbe	L42e6e		// 0x00042ca0
		mov	edi, dword ptr [ebp - 0x54]		// 0x00042ca6
		mov	ecx, dword ptr [ebp - 0x58]		// 0x00042ca9
		mov	eax, dword ptr [ebp - 8]		// 0x00042cac
		add	eax, 8		// 0x00042caf
		lea	edx, [edi + ecx*4]		// 0x00042cb2
		mov	ecx, dword ptr [ebp - 0x4c]		// 0x00042cb5
		mov	dword ptr [ebp + 0x48], eax		// 0x00042cb8
		lea	eax, [edi + ecx*4]		// 0x00042cbb
		mov	dword ptr [ebp + 0x54], edx		// 0x00042cbe
		mov	edx, dword ptr [ebp - 8]		// 0x00042cc1
		mov	dword ptr [ebp + 0x50], eax		// 0x00042cc4
		mov	eax, edi		// 0x00042cc7
		sub	eax, edx		// 0x00042cc9
		mov	edx, dword ptr [ebp + 0x48]		// 0x00042ccb
		mov	dword ptr [ebp], eax		// 0x00042cce
L42cd1:
		mov	eax, dword ptr [ebp - 0x48]		// 0x00042cd1
		cmp	byte ptr [esi + eax], 0		// 0x00042cd4
		jne	L42e51		// 0x00042cd8
		fld	dword ptr [ebp - 0x68]		// 0x00042cde
		fmul	dword ptr [edx - 8]		// 0x00042ce1
		fld	dword ptr [ebp - 0x64]		// 0x00042ce4
		fmul	dword ptr [edx - 4]		// 0x00042ce7
		faddp	st(1), st		// 0x00042cea
		fld	dword ptr [ebp - 0x60]		// 0x00042cec
		fmul	dword ptr [edx]		// 0x00042cef
		faddp	st(1), st		// 0x00042cf1
		fcomp	dword ptr kConvexMeshZero		// 0x00042cf3
		fnstsw	ax		// 0x00042cf9
		test	ah, 1		// 0x00042cfb
		je	L42e51		// 0x00042cfe
		mov	eax, dword ptr [ebp]		// 0x00042d04
		fld	dword ptr [ebp - 0x60]		// 0x00042d07
		fmul	dword ptr [eax + edx]		// 0x00042d0a
		fld	dword ptr [ebp - 0x64]		// 0x00042d0d
		fmul	dword ptr [edi + 4]		// 0x00042d10
		faddp	st(1), st		// 0x00042d13
		fld	dword ptr [ebp - 0x68]		// 0x00042d15
		fmul	dword ptr [edi]		// 0x00042d18
		faddp	st(1), st		// 0x00042d1a
		fadd	dword ptr [ebp - 0x5c]		// 0x00042d1c
		fst	dword ptr [ebp - 4]		// 0x00042d1f
		fcomp	dword ptr kConvexMeshZero		// 0x00042d22
		fnstsw	ax		// 0x00042d28
		test	ah, 5		// 0x00042d2a
		jp	L42e51		// 0x00042d2d
		mov	eax, dword ptr [ebp - 0x58]		// 0x00042d33
		fld	dword ptr [ebp + eax*4 - 0x14]		// 0x00042d36
		lea	edx, [ebp + eax*4 - 0x2c]		// 0x00042d3a
		fsub	dword ptr [edx]		// 0x00042d3e
		fld	dword ptr [ebp + ecx*4 - 0x14]		// 0x00042d40
		fsub	dword ptr [ebp + ecx*4 - 0x2c]		// 0x00042d44
		fld	dword ptr [ebp + eax*4 - 0x20]		// 0x00042d48
		mov	eax, dword ptr [ebp + 0x50]		// 0x00042d4c
		fsub	dword ptr [edx]		// 0x00042d4f
		fld	dword ptr [ebp + ecx*4 - 0x20]		// 0x00042d51
		fsub	dword ptr [ebp + ecx*4 - 0x2c]		// 0x00042d55
		fld	st(0)		// 0x00042d59
		fmul	st, st(1)		// 0x00042d5b
		fld	st(2)		// 0x00042d5d
		fmul	st, st(3)		// 0x00042d5f
		faddp	st(1), st		// 0x00042d61
		fstp	dword ptr [ebp + 0x4c]		// 0x00042d63
		fld	st(0)		// 0x00042d66
		fmul	st, st(3)		// 0x00042d68
		fld	st(2)		// 0x00042d6a
		fmul	st, st(5)		// 0x00042d6c
		faddp	st(1), st		// 0x00042d6e
		fstp	dword ptr [ebp + 8]		// 0x00042d70
		fld	st(2)		// 0x00042d73
		fmul	st, st(3)		// 0x00042d75
		fld	st(4)		// 0x00042d77
		fmul	st, st(5)		// 0x00042d79
		faddp	st(1), st		// 0x00042d7b
		fstp	dword ptr [ebp + 0x40]		// 0x00042d7d
		fld	dword ptr [eax]		// 0x00042d80
		mov	eax, dword ptr [ebp + 0x54]		// 0x00042d82
		fsub	dword ptr [ebp + ecx*4 - 0x2c]		// 0x00042d85
		fstp	dword ptr [ebp - 0x50]		// 0x00042d89
		fld	dword ptr [eax]		// 0x00042d8c
		fsub	dword ptr [edx]		// 0x00042d8e
		fstp	dword ptr [ebp + 0x34]		// 0x00042d90
		fld	dword ptr [ebp - 0x50]		// 0x00042d93
		fmul	st, st(1)		// 0x00042d96
		fld	dword ptr [ebp + 0x34]		// 0x00042d98
		fmul	st, st(3)		// 0x00042d9b
		faddp	st(1), st		// 0x00042d9d
		fstp	dword ptr [ebp + 0x30]		// 0x00042d9f
		fstp	st(0)		// 0x00042da2
		fstp	st(0)		// 0x00042da4
		fld	dword ptr [ebp - 0x50]		// 0x00042da6
		fmul	st, st(1)		// 0x00042da9
		fld	dword ptr [ebp + 0x34]		// 0x00042dab
		fmul	st, st(3)		// 0x00042dae
		faddp	st(1), st		// 0x00042db0
		fstp	st(2)		// 0x00042db2
		fstp	st(0)		// 0x00042db4
		fld	dword ptr [ebp + 0x30]		// 0x00042db6
		fmul	dword ptr [ebp + 0x40]		// 0x00042db9
		fld	st(1)		// 0x00042dbc
		fmul	dword ptr [ebp + 8]		// 0x00042dbe
		fsubp	st(1), st		// 0x00042dc1
		fstp	dword ptr [ebp + 0x34]		// 0x00042dc3
		mov	eax, dword ptr [ebp + 0x34]		// 0x00042dc6
		fmul	dword ptr [ebp + 0x4c]		// 0x00042dc9
		fld	dword ptr [ebp + 0x30]		// 0x00042dcc
		fmul	dword ptr [ebp + 8]		// 0x00042dcf
		fsubp	st(1), st		// 0x00042dd2
		fst	dword ptr [ebp + 0x30]		// 0x00042dd4
		fadd	dword ptr [ebp + 0x34]		// 0x00042dd7
		mov	edx, dword ptr [ebp + 0x30]		// 0x00042dda
		fld	dword ptr [ebp + 0x40]		// 0x00042ddd
		or	edx, eax		// 0x00042de0
		fmul	dword ptr [ebp + 0x4c]		// 0x00042de2
		not	edx		// 0x00042de5
		fld	dword ptr [ebp + 8]		// 0x00042de7
		fmul	dword ptr [ebp + 8]		// 0x00042dea
		fsubp	st(1), st		// 0x00042ded
		fsubp	st(1), st		// 0x00042def
		fstp	dword ptr [ebp + 0x4c]		// 0x00042df1
		and	edx, dword ptr [ebp + 0x4c]		// 0x00042df4
		test	edx, edx		// 0x00042df7
		jns	L42e4e		// 0x00042df9
		mov	eax, dword ptr [ebp + 0x44]		// 0x00042dfb
		mov	eax, dword ptr [eax + 0x1c]		// 0x00042dfe
		test	eax, eax		// 0x00042e01
		je	L42e0e		// 0x00042e03
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00042e05
		mov	edx, dword ptr [eax + ecx*4]		// 0x00042e08
		mov	dword ptr [ebp + 0x64], edx		// 0x00042e0b
L42e0e:
		mov	eax, dword ptr [ebp + 0x64]		// 0x00042e0e
		mov	ecx, dword ptr [ebp + 0x38]		// 0x00042e11
		push	eax		// 0x00042e14
		mov	eax, dword ptr [ebp + 0x74]		// 0x00042e15
		xor	edx, edx		// 0x00042e18
		mov	dx, word ptr [eax + 0xda]		// 0x00042e1a
		push	-1		// 0x00042e21
		push	ecx		// 0x00042e23
		lea	ecx, [ebp - 0x68]		// 0x00042e24
		push	edx		// 0x00042e27
		mov	edx, dword ptr [ebp - 4]		// 0x00042e28
		push	ecx		// 0x00042e2b
		mov	ecx, dword ptr [ebx + 0x9c]		// 0x00042e2c
		push	edi		// 0x00042e32
		push	edx		// 0x00042e33
		mov	edx, dword ptr [eax + 0x9c]		// 0x00042e34
		push	ecx		// 0x00042e3a
		mov	ecx, dword ptr [ebp + 0x7c]		// 0x00042e3b
		push	edx		// 0x00042e3e
		call	NxEmitContactFeatures		// 0x00042e3f
		mov	eax, dword ptr [ebp - 0x48]		// 0x00042e44
		mov	ecx, dword ptr [ebp - 0x4c]		// 0x00042e47
		mov	byte ptr [esi + eax], 1		// 0x00042e4a
L42e4e:
		mov	edx, dword ptr [ebp + 0x48]		// 0x00042e4e
L42e51:
		add	dword ptr [ebp + 0x50], 0xc		// 0x00042e51
		add	dword ptr [ebp + 0x54], 0xc		// 0x00042e55
		mov	eax, dword ptr [ebp + 0x2c]		// 0x00042e59
		inc	esi		// 0x00042e5c
		add	edx, 0xc		// 0x00042e5d
		add	edi, 0xc		// 0x00042e60
		cmp	esi, eax		// 0x00042e63
		mov	dword ptr [ebp + 0x48], edx		// 0x00042e65
		jb	L42cd1		// 0x00042e68
L42e6e:
		xor	esi, esi		// 0x00042e6e
		lea	edi, [ebp - 0x2c]		// 0x00042e70
L42e73:
		mov	ecx, dword ptr [ebp + 0x3c]		// 0x00042e73
		mov	eax, dword ptr [ebp + 0x60]		// 0x00042e76
		mov	edx, dword ptr [ecx + esi*4]		// 0x00042e79
		mov	ecx, dword ptr [eax + 8]		// 0x00042e7c
		mov	edx, dword ptr [ecx + edx*4]		// 0x00042e7f
		sub	edx, dword ptr [ebp + 0x10]		// 0x00042e82
		je	L42fca		// 0x00042e85
		mov	edx, dword ptr [ebp + 0x70]		// 0x00042e8b
		mov	ebx, dword ptr [ebp + 0x6c]		// 0x00042e8e
		lea	eax, [ebp - 0x164]		// 0x00042e91
		push	eax		// 0x00042e97
		lea	ecx, [ebp + 4]		// 0x00042e98
		push	ecx		// 0x00042e9b
		push	edx		// 0x00042e9c
		lea	eax, [ebp - 0x38]		// 0x00042e9d
		mov	ecx, edi		// 0x00042ea0
		call	nxConvexMeshRay		// 0x00042ea2
		add	esp, 0xc		// 0x00042ea7
		test	al, al		// 0x00042eaa
		je	L42fc7		// 0x00042eac
		mov	ebx, dword ptr [ebp + 0x3c]		// 0x00042eb2
		mov	eax, dword ptr [ebx + esi*4]		// 0x00042eb5
		mov	ecx, dword ptr [ebp - 0x8c]		// 0x00042eb8
		lea	eax, [eax + eax*2]		// 0x00042ebe
		lea	eax, [ecx + eax*4]		// 0x00042ec1
		mov	ecx, dword ptr [ebp + 0x78]		// 0x00042ec4
		fld	dword ptr [ecx + 0x18]		// 0x00042ec7
		mov	edx, dword ptr [ebp + 0x44]		// 0x00042eca
		fmul	dword ptr [eax]		// 0x00042ecd
		fld	dword ptr [eax + 8]		// 0x00042ecf
		fmul	dword ptr [ecx + 0x20]		// 0x00042ed2
		faddp	st(1), st		// 0x00042ed5
		fld	dword ptr [eax + 4]		// 0x00042ed7
		fmul	dword ptr [ecx + 0x1c]		// 0x00042eda
		faddp	st(1), st		// 0x00042edd
		fld	dword ptr [ecx + 0x2c]		// 0x00042edf
		fmul	dword ptr [eax + 8]		// 0x00042ee2
		fld	dword ptr [ecx + 0x28]		// 0x00042ee5
		fmul	dword ptr [eax + 4]		// 0x00042ee8
		faddp	st(1), st		// 0x00042eeb
		fld	dword ptr [eax]		// 0x00042eed
		fmul	dword ptr [ecx + 0x24]		// 0x00042eef
		faddp	st(1), st		// 0x00042ef2
		fld	dword ptr [eax]		// 0x00042ef4
		fmul	dword ptr [ecx + 0xc]		// 0x00042ef6
		fld	dword ptr [ecx + 0x14]		// 0x00042ef9
		fmul	dword ptr [eax + 8]		// 0x00042efc
		faddp	st(1), st		// 0x00042eff
		fld	dword ptr [eax + 4]		// 0x00042f01
		mov	eax, dword ptr [edx + 0x1c]		// 0x00042f04
		test	eax, eax		// 0x00042f07
		fmul	dword ptr [ecx + 0x10]		// 0x00042f09
		faddp	st(1), st		// 0x00042f0c
		fstp	dword ptr [ebp - 0xf8]		// 0x00042f0e
		fxch	st(1)		// 0x00042f14
		fstp	dword ptr [ebp - 0xf4]		// 0x00042f16
		fstp	dword ptr [ebp - 0xf0]		// 0x00042f1c
		fld	dword ptr [ebp + 4]		// 0x00042f22
		fabs		// 0x00042f25
		fld	dword ptr [ebp - 0x38]		// 0x00042f27
		fmul	st, st(1)		// 0x00042f2a
		fld	dword ptr [ebp - 0x34]		// 0x00042f2c
		fmul	st, st(2)		// 0x00042f2f
		fstp	dword ptr [ebp - 0x100]		// 0x00042f31
		fld	dword ptr [ebp - 0x30]		// 0x00042f37
		fmul	st, st(2)		// 0x00042f3a
		fstp	dword ptr [ebp - 0xfc]		// 0x00042f3c
		fld	dword ptr [edi]		// 0x00042f42
		fsub	st, st(1)		// 0x00042f44
		fstp	dword ptr [ebp - 0xe0]		// 0x00042f46
		fstp	st(0)		// 0x00042f4c
		fstp	st(0)		// 0x00042f4e
		fld	dword ptr [edi + 4]		// 0x00042f50
		fsub	dword ptr [ebp - 0x100]		// 0x00042f53
		fstp	dword ptr [ebp - 0xdc]		// 0x00042f59
		fld	dword ptr [edi + 8]		// 0x00042f5f
		fsub	dword ptr [ebp - 0xfc]		// 0x00042f62
		fstp	dword ptr [ebp - 0xd8]		// 0x00042f68
		je	L42f79		// 0x00042f6e
		mov	edx, dword ptr [ebp + 0x64]		// 0x00042f70
		mov	eax, dword ptr [eax + edx*4]		// 0x00042f73
		mov	dword ptr [ebp + 0x64], eax		// 0x00042f76
L42f79:
		mov	edx, dword ptr [ebp + 0x64]		// 0x00042f79
		mov	eax, dword ptr [ebp + 0x38]		// 0x00042f7c
		push	edx		// 0x00042f7f
		mov	ecx, dword ptr [ecx + 0x9c]		// 0x00042f80
		push	-1		// 0x00042f86
		push	eax		// 0x00042f88
		mov	eax, dword ptr [ebp + 0x74]		// 0x00042f89
		xor	edx, edx		// 0x00042f8c
		mov	dx, word ptr [eax + 0xda]		// 0x00042f8e
		push	edx		// 0x00042f95
		lea	edx, [ebp - 0xf8]		// 0x00042f96
		push	edx		// 0x00042f9c
		lea	edx, [ebp - 0xe0]		// 0x00042f9d
		push	edx		// 0x00042fa3
		mov	edx, dword ptr [ebp + 4]		// 0x00042fa4
		push	edx		// 0x00042fa7
		mov	edx, dword ptr [eax + 0x9c]		// 0x00042fa8
		push	ecx		// 0x00042fae
		mov	ecx, dword ptr [ebp + 0x7c]		// 0x00042faf
		push	edx		// 0x00042fb2
		call	NxEmitContactFeatures		// 0x00042fb3
		mov	ecx, dword ptr [ebp + 0x60]		// 0x00042fb8
		mov	edx, dword ptr [ecx + 8]		// 0x00042fbb
		mov	eax, dword ptr [ebx + esi*4]		// 0x00042fbe
		mov	ecx, dword ptr [ebp + 0x10]		// 0x00042fc1
		mov	dword ptr [edx + eax*4], ecx		// 0x00042fc4
L42fc7:
		mov	ebx, dword ptr [ebp + 0x78]		// 0x00042fc7
L42fca:
		inc	esi		// 0x00042fca
		add	edi, 0xc		// 0x00042fcb
		cmp	esi, 3		// 0x00042fce
		jb	L42e73		// 0x00042fd1
		mov	esi, dword ptr [ebp + 0x6c]		// 0x00042fd7
		mov	eax, dword ptr [esi + 0x38]		// 0x00042fda
		test	eax, eax		// 0x00042fdd
		jne	L42fe8		// 0x00042fdf
		mov	ecx, esi		// 0x00042fe1
		call	nxHullComputeEdges		// 0x00042fe3
L42fe8:
		mov	eax, dword ptr [esi + 0x3c]		// 0x00042fe8
		test	eax, eax		// 0x00042feb
		mov	edi, dword ptr [esi + 0x38]		// 0x00042fed
		mov	dword ptr [ebp], edi		// 0x00042ff0
		jne	L42ffc		// 0x00042ff3
		mov	ecx, esi		// 0x00042ff5
		call	nxHullComputeEdges		// 0x00042ff7
L42ffc:
		mov	esi, dword ptr [esi + 0x3c]		// 0x00042ffc
		xor	edx, edx		// 0x00042fff
		test	edi, edi		// 0x00043001
		mov	dword ptr [ebp - 4], esi		// 0x00043003
		mov	dword ptr [ebp + 0x40], edx		// 0x00043006
		jbe	L432b7		// 0x00043009
		jmp	L43014		// 0x0004300f
L43011:
		mov	esi, dword ptr [ebp - 4]		// 0x00043011
L43014:
		mov	eax, dword ptr [esi + edx*8]		// 0x00043014
		mov	ecx, dword ptr [ebp - 0x54]		// 0x00043017
		lea	eax, [eax + eax*2]		// 0x0004301a
		lea	eax, [ecx + eax*4]		// 0x0004301d
		mov	dword ptr [ebp + 0x54], eax		// 0x00043020
		mov	eax, dword ptr [esi + edx*8 + 4]		// 0x00043023
		lea	edx, [eax + eax*2]		// 0x00043027
		lea	eax, [ecx + edx*4]		// 0x0004302a
		mov	ecx, dword ptr [ebp - 0x6c]		// 0x0004302d
		mov	dword ptr [ebp + 0x50], eax		// 0x00043030
		mov	eax, 1		// 0x00043033
		mov	dword ptr [ebp + 0x3c], eax		// 0x00043038
		lea	edi, [ebp - 0x2c]		// 0x0004303b
		mov	dword ptr [ebp + 0x48], ecx		// 0x0004303e
		mov	dword ptr [ebp + 0x4c], 3		// 0x00043041
		jmp	L43050		// 0x00043048
		_emit	0x8d
		_emit	0x9b
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0004304a lea ebx, [ebx]
L43050:
		mov	edx, dword ptr [ebp + 0x48]		// 0x00043050
		cmp	dword ptr [edx], 0		// 0x00043053
		jns	L43285		// 0x00043056
		cmp	eax, 3		// 0x0004305c
		jne	L43063		// 0x0004305f
		xor	eax, eax		// 0x00043061
L43063:
		fld	dword ptr [ebp + 0x20]		// 0x00043063
		lea	esi, [eax + eax*2]		// 0x00043066
		fchs		// 0x00043069
		lea	eax, [ebp - 0xa4]		// 0x0004306b
		push	eax		// 0x00043071
		fstp	dword ptr [ebp - 0xd4]		// 0x00043072
		fld	dword ptr [ebp + 0x24]		// 0x00043078
		mov	eax, dword ptr [ebp + 0x50]		// 0x0004307b
		lea	ecx, [ebp - 0x88]		// 0x0004307e
		fchs		// 0x00043084
		push	ecx		// 0x00043086
		fstp	dword ptr [ebp - 0xd0]		// 0x00043087
		fld	dword ptr [ebp + 0x28]		// 0x0004308d
		mov	ecx, dword ptr [ebp + 0x54]		// 0x00043090
		lea	esi, [ebp + esi*4 - 0x2c]		// 0x00043093
		fchs		// 0x00043097
		push	esi		// 0x00043099
		fstp	dword ptr [ebp - 0xcc]		// 0x0004309a
		push	edi		// 0x000430a0
		lea	edx, [ebp - 0xd4]		// 0x000430a1
		push	edx		// 0x000430a7
		push	eax		// 0x000430a8
		push	ecx		// 0x000430a9
		call	NxSegmentTriangleEdge		// 0x000430aa
		add	esp, 0x1c		// 0x000430af
		test	eax, eax		// 0x000430b2
		je	L43285		// 0x000430b4
		mov	ecx, dword ptr [ebp + 0x50]		// 0x000430ba
		mov	eax, dword ptr [ebp + 0x54]		// 0x000430bd
		fld	dword ptr [ecx]		// 0x000430c0
		fsub	dword ptr [eax]		// 0x000430c2
		lea	edx, [ebp - 0xb0]		// 0x000430c4
		push	edx		// 0x000430ca
		push	edi		// 0x000430cb
		fstp	dword ptr [ebp - 0x84]		// 0x000430cc
		lea	edx, [ebp - 0x158]		// 0x000430d2
		fld	dword ptr [ecx + 4]		// 0x000430d8
		fsub	dword ptr [eax + 4]		// 0x000430db
		fstp	dword ptr [ebp - 0x80]		// 0x000430de
		fld	dword ptr [ecx + 8]		// 0x000430e1
		lea	ecx, [ebp - 0x84]		// 0x000430e4
		fsub	dword ptr [eax + 8]		// 0x000430ea
		push	ecx		// 0x000430ed
		push	eax		// 0x000430ee
		push	edx		// 0x000430ef
		fstp	dword ptr [ebp - 0x7c]		// 0x000430f0
		lea	eax, [ebp - 0x98]		// 0x000430f3
		fld	dword ptr [esi]		// 0x000430f9
		push	eax		// 0x000430fb
		fsub	dword ptr [edi]		// 0x000430fc
		fstp	dword ptr [ebp - 0xb0]		// 0x000430fe
		fld	dword ptr [esi + 4]		// 0x00043104
		fsub	dword ptr [edi + 4]		// 0x00043107
		fstp	dword ptr [ebp - 0xac]		// 0x0004310a
		fld	dword ptr [esi + 8]		// 0x00043110
		fsub	dword ptr [edi + 8]		// 0x00043113
		fstp	dword ptr [ebp - 0xa8]		// 0x00043116
		call	NxLineLineClosestPoints		// 0x0004311c
		fld	dword ptr [edi]		// 0x00043121
		mov	eax, dword ptr [ebp - 0x90]		// 0x00043123
		fsub	dword ptr [esi]		// 0x00043129
		mov	ecx, dword ptr [ebp - 0x98]		// 0x0004312b
		fld	dword ptr [edi + 4]		// 0x00043131
		mov	dword ptr [ebp - 0x9c], eax		// 0x00043134
		fsub	dword ptr [esi + 4]		// 0x0004313a
		mov	eax, dword ptr [ebp + 0x54]		// 0x0004313d
		fld	dword ptr [edi + 8]		// 0x00043140
		mov	dword ptr [ebp - 0xa4], ecx		// 0x00043143
		fsub	dword ptr [esi + 8]		// 0x00043149
		mov	ecx, dword ptr [ebp + 0x50]		// 0x0004314c
		fld	dword ptr [eax]		// 0x0004314f
		mov	edx, dword ptr [ebp - 0x94]		// 0x00043151
		fsub	dword ptr [ecx]		// 0x00043157
		mov	dword ptr [ebp - 0xa0], edx		// 0x00043159
		add	esp, 0x18		// 0x0004315f
		fstp	dword ptr [ebp - 0x78]		// 0x00043162
		fld	dword ptr [eax + 4]		// 0x00043165
		fsub	dword ptr [ecx + 4]		// 0x00043168
		fstp	dword ptr [ebp - 0x74]		// 0x0004316b
		fld	dword ptr [eax + 8]		// 0x0004316e
		fsub	dword ptr [ecx + 8]		// 0x00043171
		fld	dword ptr [ebp - 0x74]		// 0x00043174
		fmul	st, st(2)		// 0x00043177
		fld	st(1)		// 0x00043179
		fmul	st, st(4)		// 0x0004317b
		fsubp	st(1), st		// 0x0004317d
		fstp	dword ptr [ebp - 0x44]		// 0x0004317f
		mov	ecx, dword ptr [ebp - 0x44]		// 0x00043182
		mov	dword ptr [ebp + 0x14], ecx		// 0x00043185
		fmul	st, st(3)		// 0x00043188
		fxch	st(1)		// 0x0004318a
		fmul	dword ptr [ebp - 0x78]		// 0x0004318c
		fsubp	st(1), st		// 0x0004318f
		fstp	dword ptr [ebp - 0x40]		// 0x00043191
		mov	edx, dword ptr [ebp - 0x40]		// 0x00043194
		mov	dword ptr [ebp + 0x18], edx		// 0x00043197
		fmul	dword ptr [ebp - 0x78]		// 0x0004319a
		fld	dword ptr [ebp - 0x74]		// 0x0004319d
		fmul	st, st(2)		// 0x000431a0
		fsubp	st(1), st		// 0x000431a2
		fstp	st(1)		// 0x000431a4
		fst	dword ptr [ebp + 0x1c]		// 0x000431a6
		fld	dword ptr [ebp - 0x44]		// 0x000431a9
		fmul	dword ptr [ebp - 0x44]		// 0x000431ac
		fld	st(1)		// 0x000431af
		fmul	st, st(2)		// 0x000431b1
		faddp	st(1), st		// 0x000431b3
		fld	dword ptr [ebp - 0x40]		// 0x000431b5
		fmul	dword ptr [ebp - 0x40]		// 0x000431b8
		faddp	st(1), st		// 0x000431bb
		fsqrt		// 0x000431bd
		fld	dword ptr kConvexMeshZero		// 0x000431bf
		fld	st(1)		// 0x000431c5
		fucompp		// 0x000431c7
		fnstsw	ax		// 0x000431c9
		test	ah, 0x44		// 0x000431cb
		jnp	L431ef		// 0x000431ce
		fdivr	dword ptr kConvexMeshOne		// 0x000431d0
		fld	dword ptr [ebp - 0x44]		// 0x000431d6
		fmul	st, st(1)		// 0x000431d9
		fstp	dword ptr [ebp + 0x14]		// 0x000431db
		fld	dword ptr [ebp - 0x40]		// 0x000431de
		fmul	st, st(1)		// 0x000431e1
		fstp	dword ptr [ebp + 0x18]		// 0x000431e3
		fxch	st(1)		// 0x000431e6
		fmul	st, st(1)		// 0x000431e8
		fstp	dword ptr [ebp + 0x1c]		// 0x000431ea
		jmp	L431f1		// 0x000431ed
L431ef:
		fstp	st(0)		// 0x000431ef
L431f1:
		fstp	st(0)		// 0x000431f1
		fld	dword ptr [ebp + 0x20]		// 0x000431f3
		fmul	dword ptr [ebp + 0x14]		// 0x000431f6
		fld	dword ptr [ebp + 0x1c]		// 0x000431f9
		fmul	dword ptr [ebp + 0x28]		// 0x000431fc
		faddp	st(1), st		// 0x000431ff
		fld	dword ptr [ebp + 0x18]		// 0x00043201
		fmul	dword ptr [ebp + 0x24]		// 0x00043204
		faddp	st(1), st		// 0x00043207
		fcomp	dword ptr kConvexMeshZero		// 0x00043209
		fnstsw	ax		// 0x0004320f
		test	ah, 0x41		// 0x00043211
		jne	L4322e		// 0x00043214
		fld	dword ptr [ebp + 0x14]		// 0x00043216
		fchs		// 0x00043219
		fstp	dword ptr [ebp + 0x14]		// 0x0004321b
		fld	dword ptr [ebp + 0x18]		// 0x0004321e
		fchs		// 0x00043221
		fstp	dword ptr [ebp + 0x18]		// 0x00043223
		fld	dword ptr [ebp + 0x1c]		// 0x00043226
		fchs		// 0x00043229
		fstp	dword ptr [ebp + 0x1c]		// 0x0004322b
L4322e:
		mov	eax, dword ptr [ebp + 0x44]		// 0x0004322e
		mov	eax, dword ptr [eax + 0x1c]		// 0x00043231
		test	eax, eax		// 0x00043234
		je	L43241		// 0x00043236
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00043238
		mov	edx, dword ptr [eax + ecx*4]		// 0x0004323b
		mov	dword ptr [ebp + 0x64], edx		// 0x0004323e
L43241:
		mov	eax, dword ptr [ebp + 0x64]		// 0x00043241
		fld	dword ptr [ebp - 0x88]		// 0x00043244
		mov	ecx, dword ptr [ebp + 0x38]		// 0x0004324a
		fchs		// 0x0004324d
		push	eax		// 0x0004324f
		mov	eax, dword ptr [ebp + 0x74]		// 0x00043250
		xor	edx, edx		// 0x00043253
		mov	dx, word ptr [eax + 0xda]		// 0x00043255
		push	-1		// 0x0004325c
		push	ecx		// 0x0004325e
		lea	ecx, [ebp + 0x14]		// 0x0004325f
		push	edx		// 0x00043262
		push	ecx		// 0x00043263
		lea	edx, [ebp - 0xa4]		// 0x00043264
		push	edx		// 0x0004326a
		mov	edx, dword ptr [eax + 0x9c]		// 0x0004326b
		push	ecx		// 0x00043271
		mov	ecx, dword ptr [ebx + 0x9c]		// 0x00043272
		fstp	dword ptr [esp]		// 0x00043278
		push	ecx		// 0x0004327b
		mov	ecx, dword ptr [ebp + 0x7c]		// 0x0004327c
		push	edx		// 0x0004327f
		call	NxEmitContactFeatures		// 0x00043280
L43285:
		mov	eax, dword ptr [ebp + 0x48]		// 0x00043285
		mov	ecx, dword ptr [ebp + 0x4c]		// 0x00043288
		add	eax, 4		// 0x0004328b
		mov	dword ptr [ebp + 0x48], eax		// 0x0004328e
		mov	eax, dword ptr [ebp + 0x3c]		// 0x00043291
		inc	eax		// 0x00043294
		add	edi, 0xc		// 0x00043295
		dec	ecx		// 0x00043298
		mov	dword ptr [ebp + 0x3c], eax		// 0x00043299
		mov	dword ptr [ebp + 0x4c], ecx		// 0x0004329c
		jne	L43050		// 0x0004329f
		mov	edx, dword ptr [ebp + 0x40]		// 0x000432a5
		mov	eax, dword ptr [ebp]		// 0x000432a8
		inc	edx		// 0x000432ab
		cmp	edx, eax		// 0x000432ac
		mov	dword ptr [ebp + 0x40], edx		// 0x000432ae
		jb	L43011		// 0x000432b1
L432b7:
		dec	dword ptr [ebp + 0xc]		// 0x000432b7
		jne	L42a97		// 0x000432ba
L432c0:
		lea	esp, [ebp - 0x170]		// 0x000432c0
		pop	edi		// 0x000432c6
		pop	esi		// 0x000432c7
		pop	ebx		// 0x000432c8
		add	ebp, 0x58		// 0x000432c9
		mov	esp, ebp		// 0x000432cc
		pop	ebp		// 0x000432ce
		ret		// 0x000432cf
		}
	}

// phys_fn_001847 (0x000432d0, 943 B)
// The convex/height-field entry (cdecl: the convex shape, the height-field
// shape, the sink, the context). The collider at context +0x110 gets
// OPC_FIRST_CONTACT and OPC_TEMPORAL_COHERENCE cleared and
// OPC_NO_PRIMITIVE_TESTS set (three stores to its +0x04). Both shapes' poses go
// to 4x4s (the rotation transposed, the translation, 1.0f). The query box is the
// convex shape's world box through its Prunable (+0xa4: handle +0xcc, where
// 0xffff gives a null box that is then read; 004886 first unless bit 2 of the
// flags +0xac is set; the box array at pruner (+0xc4) +0x14, 24 bytes a handle)
// as a centre and extents (x 0.5f, in the listing's order) with an identity
// rotation. OBBCollider::Collide(context +0x244, box, height field +0x28, null,
// the height field's 4x4): false reports "Opcode is not OK." (line 583) and
// returns; with OPC_CONTACT set, the touched Container's count and entries, the
// hull (convex mesh +0xa0) and the convex's 4x4 go to 001844.
__declspec(naked) void __cdecl NxContactConvexHeightfield(const NxCollisionShape* /*convex*/,
	const NxCollisionShape* /*heightfield*/, NxContactSink* /*sink*/, void* /*context*/)
	{
	__asm
		{
		sub	esp, 0xf0		// 0x000432d0
		push	ebx		// 0x000432d6
		mov	ebx, dword ptr [esp + 0x104]		// 0x000432d7
		push	ebp		// 0x000432de
		add	ebx, 0x110		// 0x000432df
		push	esi		// 0x000432e5
		mov	esi, dword ptr [esp + 0x100]		// 0x000432e6
		mov	eax, dword ptr [esi + 0xe0]		// 0x000432ed
		mov	ecx, dword ptr [eax + 0xa0]		// 0x000432f3
		push	edi		// 0x000432f9
		mov	edi, dword ptr [ebx + 4]		// 0x000432fa
		mov	dword ptr [esp + 0x34], ecx		// 0x000432fd
		and	edi, 0xfffffffe		// 0x00043301
		mov	dword ptr [ebx + 4], edi		// 0x00043304
		mov	edx, edi		// 0x00043307
		mov	edi, dword ptr [esp + 0x108]		// 0x00043309
		and	edx, 0xfffffffd		// 0x00043310
		mov	dword ptr [ebx + 4], edx		// 0x00043313
		mov	ecx, edx		// 0x00043316
		or	ecx, 0x10		// 0x00043318
		mov	dword ptr [ebx + 4], ecx		// 0x0004331b
		mov	ecx, dword ptr [esi + 0x14]		// 0x0004331e
		mov	eax, dword ptr [esi + 0x10]		// 0x00043321
		mov	edx, dword ptr [esi + 0xc]		// 0x00043324
		mov	ebp, dword ptr [esi + 0xc4]		// 0x00043327
		mov	dword ptr [esp + 0xe0], ecx		// 0x0004332d
		mov	ecx, dword ptr [esi + 0x20]		// 0x00043334
		mov	dword ptr [esp + 0xd0], eax		// 0x00043337
		mov	eax, dword ptr [esi + 0x1c]		// 0x0004333e
		mov	dword ptr [esp + 0xe4], ecx		// 0x00043341
		mov	ecx, dword ptr [esi + 0x2c]		// 0x00043348
		mov	dword ptr [esp + 0xc0], edx		// 0x0004334b
		mov	edx, dword ptr [esi + 0x18]		// 0x00043352
		mov	dword ptr [esp + 0xd4], eax		// 0x00043355
		mov	eax, dword ptr [esi + 0x28]		// 0x0004335c
		mov	dword ptr [esp + 0xe8], ecx		// 0x0004335f
		mov	ecx, dword ptr [esi + 0x38]		// 0x00043366
		mov	dword ptr [esp + 0xc4], edx		// 0x00043369
		mov	edx, dword ptr [esi + 0x24]		// 0x00043370
		mov	dword ptr [esp + 0xd8], eax		// 0x00043373
		mov	eax, dword ptr [esi + 0x34]		// 0x0004337a
		mov	dword ptr [esp + 0xf8], ecx		// 0x0004337d
		mov	ecx, dword ptr [edi + 0x14]		// 0x00043384
		mov	dword ptr [esp + 0xc8], edx		// 0x00043387
		mov	edx, dword ptr [esi + 0x30]		// 0x0004338e
		mov	dword ptr [esp + 0xf4], eax		// 0x00043391
		mov	eax, dword ptr [edi + 0x10]		// 0x00043398
		mov	dword ptr [esp + 0xa0], ecx		// 0x0004339b
		mov	ecx, dword ptr [edi + 0x20]		// 0x000433a2
		mov	dword ptr [esp + 0xf0], edx		// 0x000433a5
		mov	edx, dword ptr [edi + 0xc]		// 0x000433ac
		mov	dword ptr [esp + 0x90], eax		// 0x000433af
		mov	eax, dword ptr [edi + 0x1c]		// 0x000433b6
		mov	dword ptr [esp + 0xa4], ecx		// 0x000433b9
		mov	ecx, dword ptr [edi + 0x2c]		// 0x000433c0
		mov	dword ptr [esp + 0x80], edx		// 0x000433c3
		mov	edx, dword ptr [edi + 0x18]		// 0x000433ca
		mov	dword ptr [esp + 0x94], eax		// 0x000433cd
		mov	eax, dword ptr [edi + 0x28]		// 0x000433d4
		mov	dword ptr [esp + 0xa8], ecx		// 0x000433d7
		mov	ecx, dword ptr [edi + 0x38]		// 0x000433de
		mov	dword ptr [esp + 0x84], edx		// 0x000433e1
		mov	edx, dword ptr [edi + 0x24]		// 0x000433e8
		mov	dword ptr [esp + 0x98], eax		// 0x000433eb
		mov	eax, dword ptr [edi + 0x34]		// 0x000433f2
		mov	dword ptr [esp + 0xb8], ecx		// 0x000433f5
		lea	ecx, [esi + 0xa4]		// 0x000433fc
		mov	dword ptr [esp + 0x88], edx		// 0x00043402
		mov	edx, dword ptr [edi + 0x30]		// 0x00043409
		mov	dword ptr [esp + 0xb4], eax		// 0x0004340c
		mov	ax, word ptr [ecx + 0x28]		// 0x00043413
		add	ebp, 4		// 0x00043417
		mov	dword ptr [esp + 0xec], 0		// 0x0004341a
		mov	dword ptr [esp + 0xdc], 0		// 0x00043425
		mov	dword ptr [esp + 0xcc], 0		// 0x00043430
		mov	dword ptr [esp + 0xfc], 0x3f800000		// 0x0004343b
		mov	dword ptr [esp + 0xb0], edx		// 0x00043446
		mov	dword ptr [esp + 0xac], 0		// 0x0004344d
		mov	dword ptr [esp + 0x9c], 0		// 0x00043458
		mov	dword ptr [esp + 0x8c], 0		// 0x00043463
		mov	dword ptr [esp + 0xbc], 0x3f800000		// 0x0004346e
		_emit	0x66		// 0x00043479 cmp ax, 0xffff (the listing's encoding, 66 3d ff ff;
		_emit	0x3d		// MSVC's assembler would pick the sign-extended 66 83 f8 ff)
		_emit	0xff
		_emit	0xff
		jne	L43483		// 0x0004347d
		xor	eax, eax		// 0x0004347f
		jmp	L434ab		// 0x00043481
L43483:
		test	byte ptr [ecx + 8], 2		// 0x00043483
		jne	L4349b		// 0x00043487
		movzx	eax, ax		// 0x00043489
		lea	edx, [eax + eax*2]		// 0x0004348c
		mov	eax, dword ptr [ebp + 0x10]		// 0x0004348f
		lea	edx, [eax + edx*8]		// 0x00043492
		push	edx		// 0x00043495
		call	nxConvexMeshCallUpdateWorldAABB		// 0x00043496
L4349b:
		movzx	eax, word ptr [esi + 0xcc]		// 0x0004349b
		mov	ecx, dword ptr [ebp + 0x10]		// 0x000434a2
		lea	eax, [eax + eax*2]		// 0x000434a5
		lea	eax, [ecx + eax*8]		// 0x000434a8
L434ab:
		mov	edx, dword ptr [eax]		// 0x000434ab
		mov	ecx, dword ptr [eax + 4]		// 0x000434ad
		mov	dword ptr [esp + 0x20], ecx		// 0x000434b0
		mov	ecx, dword ptr [eax + 0xc]		// 0x000434b4
		mov	dword ptr [esp + 0x28], ecx		// 0x000434b7
		fld	dword ptr [esp + 0x28]		// 0x000434bb
		mov	dword ptr [esp + 0x1c], edx		// 0x000434bf
		fadd	dword ptr [esp + 0x1c]		// 0x000434c3
		mov	edx, dword ptr [eax + 8]		// 0x000434c7
		mov	dword ptr [esp + 0x24], edx		// 0x000434ca
		mov	edx, dword ptr [eax + 0x10]		// 0x000434ce
		mov	eax, dword ptr [eax + 0x14]		// 0x000434d1
		mov	dword ptr [esp + 0x2c], edx		// 0x000434d4
		fld	dword ptr [esp + 0x2c]		// 0x000434d8
		fadd	dword ptr [esp + 0x20]		// 0x000434dc
		mov	dword ptr [esp + 0x30], eax		// 0x000434e0
		fld	dword ptr [esp + 0x30]		// 0x000434e4
		fadd	dword ptr [esp + 0x24]		// 0x000434e8
		fstp	dword ptr [esp + 0x7c]		// 0x000434ec
		fxch	st(1)		// 0x000434f0
		fmul	dword ptr kConvexMeshHalf		// 0x000434f2
		fstp	dword ptr [esp + 0x10]		// 0x000434f8
		mov	ecx, dword ptr [esp + 0x10]		// 0x000434fc
		mov	dword ptr [esp + 0x38], ecx		// 0x00043500
		fmul	dword ptr kConvexMeshHalf		// 0x00043504
		fstp	dword ptr [esp + 0x14]		// 0x0004350a
		mov	edx, dword ptr [esp + 0x14]		// 0x0004350e
		fld	dword ptr [esp + 0x7c]		// 0x00043512
		mov	dword ptr [esp + 0x3c], edx		// 0x00043516
		fmul	dword ptr kConvexMeshHalf		// 0x0004351a
		fstp	dword ptr [esp + 0x18]		// 0x00043520
		mov	eax, dword ptr [esp + 0x18]		// 0x00043524
		fld	dword ptr [esp + 0x28]		// 0x00043528
		mov	dword ptr [esp + 0x40], eax		// 0x0004352c
		fsub	dword ptr [esp + 0x1c]		// 0x00043530
		fld	dword ptr [esp + 0x2c]		// 0x00043534
		fsub	dword ptr [esp + 0x20]		// 0x00043538
		fld	dword ptr [esp + 0x30]		// 0x0004353c
		fsub	dword ptr [esp + 0x24]		// 0x00043540
		fstp	dword ptr [esp + 0x7c]		// 0x00043544
		fxch	st(1)		// 0x00043548
		fmul	dword ptr kConvexMeshHalf		// 0x0004354a
		fstp	dword ptr [esp + 0x10]		// 0x00043550
		mov	ecx, dword ptr [esp + 0x10]		// 0x00043554
		mov	dword ptr [esp + 0x44], ecx		// 0x00043558
		fmul	dword ptr kConvexMeshHalf		// 0x0004355c
		xor	ecx, ecx		// 0x00043562
		mov	dword ptr [esp + 0x50], ecx		// 0x00043564
		mov	dword ptr [esp + 0x54], ecx		// 0x00043568
		mov	dword ptr [esp + 0x58], ecx		// 0x0004356c
		fstp	dword ptr [esp + 0x14]		// 0x00043570
		fld	dword ptr [esp + 0x7c]		// 0x00043574
		mov	edx, dword ptr [esp + 0x14]		// 0x00043578
		fmul	dword ptr kConvexMeshHalf		// 0x0004357c
		mov	dword ptr [esp + 0x5c], ecx		// 0x00043582
		mov	dword ptr [esp + 0x60], ecx		// 0x00043586
		mov	dword ptr [esp + 0x64], ecx		// 0x0004358a
		mov	dword ptr [esp + 0x68], ecx		// 0x0004358e
		fstp	dword ptr [esp + 0x18]		// 0x00043592
		mov	eax, dword ptr [esp + 0x18]		// 0x00043596
		mov	dword ptr [esp + 0x6c], ecx		// 0x0004359a
		mov	dword ptr [esp + 0x70], ecx		// 0x0004359e
		mov	dword ptr [esp + 0x48], edx		// 0x000435a2
		mov	edx, dword ptr [edi + 0xe0]		// 0x000435a6
		lea	ecx, [esp + 0x80]		// 0x000435ac
		push	ecx		// 0x000435b3
		mov	dword ptr [esp + 0x50], eax		// 0x000435b4
		push	0		// 0x000435b8
		mov	dword ptr [esp + 0x78], 0x3f800000		// 0x000435ba
		mov	dword ptr [esp + 0x68], 0x3f800000		// 0x000435c2
		mov	dword ptr [esp + 0x58], 0x3f800000		// 0x000435ca
		mov	eax, dword ptr [edx + 0x28]		// 0x000435d2
		push	eax		// 0x000435d5
		mov	eax, dword ptr [esp + 0x11c]		// 0x000435d6
		lea	edx, [esp + 0x44]		// 0x000435dd
		push	edx		// 0x000435e1
		add	eax, 0x244		// 0x000435e2
		push	eax		// 0x000435e7
		mov	ecx, ebx		// 0x000435e8
		call	nxConvexMeshCallObbCollide		// 0x000435ea
		test	al, al		// 0x000435ef
		jne	L43626		// 0x000435f1
		mov	ecx, dword ptr nxConvexMeshFoundationInstanceSlot		// 0x000435f3
		cmp	dword ptr [ecx], 0		// 0x000435f9
		jne	L435ff		// 0x000435fc
		_emit	0xcc		// 0x000435fe int3 
L435ff:
		push	offset kConvexMeshOpcodeNotOk		// 0x000435ff
		push	0		// 0x00043604
		push	0x247		// 0x00043606
		push	offset kConvexHeightfieldFile		// 0x0004360b
		push	4		// 0x00043610
		call	dword ptr nxConvexMeshFoundationErrorSlot		// 0x00043612
		add	esp, 0x14		// 0x00043618
		pop	edi		// 0x0004361b
		pop	esi		// 0x0004361c
		pop	ebp		// 0x0004361d
		pop	ebx		// 0x0004361e
		add	esp, 0xf0		// 0x0004361f
		ret		// 0x00043625
L43626:
		mov	edx, dword ptr [esp + 0x110]		// 0x00043626
		test	byte ptr [edx + 0x114], 4		// 0x0004362d
		je	L43674		// 0x00043634
		mov	eax, dword ptr [ebx + 0x10]		// 0x00043636
		test	eax, eax		// 0x00043639
		je	L43642		// 0x0004363b
		mov	ecx, dword ptr [eax + 4]		// 0x0004363d
		jmp	L43644		// 0x00043640
L43642:
		xor	ecx, ecx		// 0x00043642
L43644:
		mov	ebx, dword ptr [ebx + 0x10]		// 0x00043644
		test	ebx, ebx		// 0x00043647
		je	L43650		// 0x00043649
		mov	eax, dword ptr [ebx + 8]		// 0x0004364b
		jmp	L43652		// 0x0004364e
L43650:
		xor	eax, eax		// 0x00043650
L43652:
		mov	ebx, dword ptr [esp + 0x10c]		// 0x00043652
		push	ebx		// 0x00043659
		push	edi		// 0x0004365a
		push	esi		// 0x0004365b
		lea	esi, [esp + 0xcc]		// 0x0004365c
		push	esi		// 0x00043663
		mov	esi, dword ptr [esp + 0x44]		// 0x00043664
		push	esi		// 0x00043668
		push	eax		// 0x00043669
		push	ecx		// 0x0004366a
		push	edx		// 0x0004366b
		call	nxConvexHeightfieldContacts		// 0x0004366c
		add	esp, 0x20		// 0x00043671
L43674:
		pop	edi		// 0x00043674
		pop	esi		// 0x00043675
		pop	ebp		// 0x00043676
		pop	ebx		// 0x00043677
		add	esp, 0xf0		// 0x00043678
		ret		// 0x0004367e
		}
	}

// phys_fn_001849 (0x00043680, 2899 B)
// The convex against a triangle mesh (cdecl, nine arguments: the convex shape,
// the mesh shape, the convex's and the mesh's 4x4 poses, the convex mesh's
// polygon interface, its support map or null, the query box, the sink, the
// context; ebp-framed, alloca). OPC_FIRST_CONTACT, OPC_TEMPORAL_COHERENCE and
// OPC_NO_PRIMITIVE_TESTS cleared; the two relative poses (001653); slot 3 of the
// interface (its result unread); OBBCollider::Collide(context +0x244, box, mesh
// +0x28, null, the mesh's 4x4), a failure reported (line 2594). With contacts:
// the interface's centre (slot 0) in the world and in the mesh's frame; the
// EdgeList built on first use; the touched triangles radix-sorted (unsigned) by
// their convex part (mesh +0x94). Per convex part: its triangles' area-weighted
// centre (Triangle::Area and Center of each, summed in the listing's order, the
// flat parts (+0x98) copied beside them) taken into the convex's frame; the face
// axes (001832); unless they separate, the part's triangles sorted again by flat
// part, and per flat part each triangle whose plane (mesh +0x24, 16 bytes a
// triangle) has the convex's centre above it (`test ah, 0x41`) through the edge
// axes (001840), keeping the least depth; the cross axes (001836) over the part;
// the normal turned against the centres' difference, the supporting polygon
// along it (slot 9 with the convex's pose) and 001842 on each triangle of the
// best flat part (its material +0x18 or 0xffff, its index through the remap
// +0x1c when that is non-null).
__declspec(naked) void nxConvexMeshContact()
	{
	__asm
		{
		push	ebp		// 0x00043680
		lea	ebp, [esp - 0x54]		// 0x00043681
		sub	esp, 0x1fc		// 0x00043685
		mov	eax, dword ptr [ebp + 0x64]		// 0x0004368b
		push	ebx		// 0x0004368e
		mov	ebx, dword ptr [ebp + 0x7c]		// 0x0004368f
		push	esi		// 0x00043692
		mov	esi, dword ptr [ebx + 0x114]		// 0x00043693
		add	ebx, 0x110		// 0x00043699
		and	esi, 0xfffffffe		// 0x0004369f
		mov	edx, esi		// 0x000436a2
		and	edx, 0xfffffffd		// 0x000436a4
		mov	dword ptr [ebx + 4], esi		// 0x000436a7
		mov	esi, dword ptr [ebp + 0x68]		// 0x000436aa
		push	edi		// 0x000436ad
		mov	ecx, edx		// 0x000436ae
		and	ecx, 0xffffffef		// 0x000436b0
		mov	dword ptr [ebx + 4], edx		// 0x000436b3
		push	esi		// 0x000436b6
		mov	dword ptr [ebx + 4], ecx		// 0x000436b7
		push	eax		// 0x000436ba
		lea	ecx, [ebp - 0x154]		// 0x000436bb
		push	ecx		// 0x000436c1
		lea	edx, [ebp - 0x114]		// 0x000436c2
		push	edx		// 0x000436c8
		call	nxIcePosePair		// 0x000436c9
		mov	eax, dword ptr [ebp + 0x60]		// 0x000436ce
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x000436d1
		mov	edi, dword ptr [eax + 0xe0]		// 0x000436d4
		mov	edx, dword ptr [ecx]		// 0x000436da
		add	esp, 0x10		// 0x000436dc
		mov	dword ptr [ebp + 4], edi		// 0x000436df
		call	dword ptr [edx + 0xc]		// 0x000436e2
		mov	eax, dword ptr [edi + 0x28]		// 0x000436e5
		mov	ecx, dword ptr [ebp + 0x7c]		// 0x000436e8
		push	esi		// 0x000436eb
		push	0		// 0x000436ec
		push	eax		// 0x000436ee
		mov	eax, dword ptr [ebp + 0x74]		// 0x000436ef
		add	ecx, 0x244		// 0x000436f2
		push	eax		// 0x000436f8
		push	ecx		// 0x000436f9
		mov	ecx, ebx		// 0x000436fa
		call	nxConvexMeshCallObbCollide		// 0x000436fc
		test	al, al		// 0x00043701
		jne	L4373d		// 0x00043703
		mov	edx, dword ptr nxConvexMeshFoundationInstanceSlot		// 0x00043705
		cmp	dword ptr [edx], 0		// 0x0004370b
		jne	L43711		// 0x0004370e
		_emit	0xcc		// 0x00043710 int3 
L43711:
		push	offset kConvexMeshOpcodeNotOk		// 0x00043711
		push	0		// 0x00043716
		push	0xa22		// 0x00043718
		push	offset kConvexHeightfieldFile		// 0x0004371d
		push	4		// 0x00043722
		call	dword ptr nxConvexMeshFoundationErrorSlot		// 0x00043724
		add	esp, 0x14		// 0x0004372a
		lea	esp, [ebp - 0x1b4]		// 0x0004372d
		pop	edi		// 0x00043733
		pop	esi		// 0x00043734
		pop	ebx		// 0x00043735
		add	ebp, 0x54		// 0x00043736
		mov	esp, ebp		// 0x00043739
		pop	ebp		// 0x0004373b
		ret		// 0x0004373c
L4373d:
		mov	eax, dword ptr [ebp + 0x7c]		// 0x0004373d
		test	byte ptr [eax + 0x114], 4		// 0x00043740
		je	L441c3		// 0x00043747
		mov	ebx, dword ptr [ebx + 0x10]		// 0x0004374d
		test	ebx, ebx		// 0x00043750
		je	L43759		// 0x00043752
		mov	ebx, dword ptr [ebx + 4]		// 0x00043754
		jmp	L4375b		// 0x00043757
L43759:
		xor	ebx, ebx		// 0x00043759
L4375b:
		test	ebx, ebx		// 0x0004375b
		je	L441c3		// 0x0004375d
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00043763
		add	eax, 0x110		// 0x00043766
		mov	eax, dword ptr [eax + 0x10]		// 0x0004376b
		test	eax, eax		// 0x0004376e
		je	L4377a		// 0x00043770
		mov	ecx, dword ptr [eax + 8]		// 0x00043772
		mov	dword ptr [ebp], ecx		// 0x00043775
		jmp	L43781		// 0x00043778
L4377a:
		mov	dword ptr [ebp], 0		// 0x0004377a
L43781:
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00043781
		mov	edx, dword ptr [ecx]		// 0x00043784
		call	dword ptr [edx]		// 0x00043786
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00043788
		fld	dword ptr [ecx + 0x10]		// 0x0004378b
		fmul	dword ptr [eax + 4]		// 0x0004378e
		fld	dword ptr [ecx + 0x20]		// 0x00043791
		fmul	dword ptr [eax + 8]		// 0x00043794
		faddp	st(1), st		// 0x00043797
		fld	dword ptr [ecx]		// 0x00043799
		fmul	dword ptr [eax]		// 0x0004379b
		faddp	st(1), st		// 0x0004379d
		fadd	dword ptr [ecx + 0x30]		// 0x0004379f
		fstp	dword ptr [ebp - 0x64]		// 0x000437a2
		fld	dword ptr [ecx + 0x14]		// 0x000437a5
		fmul	dword ptr [eax + 4]		// 0x000437a8
		fld	dword ptr [ecx + 4]		// 0x000437ab
		fmul	dword ptr [eax]		// 0x000437ae
		faddp	st(1), st		// 0x000437b0
		fld	dword ptr [ecx + 0x24]		// 0x000437b2
		fmul	dword ptr [eax + 8]		// 0x000437b5
		faddp	st(1), st		// 0x000437b8
		fadd	dword ptr [ecx + 0x34]		// 0x000437ba
		fstp	dword ptr [ebp - 0x60]		// 0x000437bd
		fld	dword ptr [ecx + 0x18]		// 0x000437c0
		fmul	dword ptr [eax + 4]		// 0x000437c3
		fld	dword ptr [ecx + 8]		// 0x000437c6
		fmul	dword ptr [eax]		// 0x000437c9
		faddp	st(1), st		// 0x000437cb
		fld	dword ptr [ecx + 0x28]		// 0x000437cd
		fmul	dword ptr [eax + 8]		// 0x000437d0
		faddp	st(1), st		// 0x000437d3
		fadd	dword ptr [ecx + 0x38]		// 0x000437d5
		fstp	dword ptr [ebp - 0x5c]		// 0x000437d8
		fld	dword ptr [ebp - 0xf4]		// 0x000437db
		fmul	dword ptr [eax + 8]		// 0x000437e1
		fld	dword ptr [ebp - 0x104]		// 0x000437e4
		fmul	dword ptr [eax + 4]		// 0x000437ea
		faddp	st(1), st		// 0x000437ed
		fld	dword ptr [ebp - 0x114]		// 0x000437ef
		fmul	dword ptr [eax]		// 0x000437f5
		faddp	st(1), st		// 0x000437f7
		fadd	dword ptr [ebp - 0xe4]		// 0x000437f9
		fstp	dword ptr [ebp - 0x54]		// 0x000437ff
		fld	dword ptr [ebp - 0x100]		// 0x00043802
		fmul	dword ptr [eax + 4]		// 0x00043808
		fld	dword ptr [ebp - 0xf0]		// 0x0004380b
		fmul	dword ptr [eax + 8]		// 0x00043811
		faddp	st(1), st		// 0x00043814
		fld	dword ptr [ebp - 0x110]		// 0x00043816
		fmul	dword ptr [eax]		// 0x0004381c
		faddp	st(1), st		// 0x0004381e
		fadd	dword ptr [ebp - 0xe0]		// 0x00043820
		fstp	dword ptr [ebp - 0x50]		// 0x00043826
		fld	dword ptr [ebp - 0xfc]		// 0x00043829
		fmul	dword ptr [eax + 4]		// 0x0004382f
		fld	dword ptr [ebp - 0xec]		// 0x00043832
		fmul	dword ptr [eax + 8]		// 0x00043838
		faddp	st(1), st		// 0x0004383b
		fld	dword ptr [ebp - 0x10c]		// 0x0004383d
		fmul	dword ptr [eax]		// 0x00043843
		mov	eax, dword ptr [edi + 0x88]		// 0x00043845
		test	eax, eax		// 0x0004384b
		faddp	st(1), st		// 0x0004384d
		fadd	dword ptr [ebp - 0xdc]		// 0x0004384f
		fstp	dword ptr [ebp - 0x4c]		// 0x00043855
		jne	L43861		// 0x00043858
		mov	ecx, edi		// 0x0004385a
		call	nxConvexMeshCallCreateEdgeList		// 0x0004385c
L43861:
		mov	eax, dword ptr [edi + 0x94]		// 0x00043861
		mov	ecx, dword ptr [edi + 0x98]		// 0x00043867
		mov	dword ptr [ebp + 0x28], eax		// 0x0004386d
		lea	eax, [ebx*4]		// 0x00043870
		add	eax, 3		// 0x00043877
		and	eax, 0xfffffffc		// 0x0004387a
		mov	dword ptr [ebp - 0x6c], ecx		// 0x0004387d
		call	_chkstk		// 0x00043880
		test	ebx, ebx		// 0x00043885
		mov	eax, esp		// 0x00043887
		mov	dword ptr [ebp - 4], eax		// 0x00043889
		jbe	L438b7		// 0x0004388c
		mov	ecx, dword ptr [ebp]		// 0x0004388e
		sub	ecx, eax		// 0x00043891
		mov	dword ptr [ebp + 0x1c], ecx		// 0x00043893
		mov	dword ptr [ebp + 0x24], ebx		// 0x00043896
		jmp	L438a0		// 0x00043899
L4389b:
		mov	ecx, dword ptr [ebp + 0x1c]		// 0x0004389b
		_emit	0x8b
		_emit	0xff		// 0x0004389e mov edi, edi
L438a0:
		mov	edx, dword ptr [ecx + eax]		// 0x000438a0
		mov	ecx, dword ptr [ebp + 0x28]		// 0x000438a3
		mov	edx, dword ptr [ecx + edx*4]		// 0x000438a6
		mov	ecx, dword ptr [ebp + 0x24]		// 0x000438a9
		mov	dword ptr [eax], edx		// 0x000438ac
		add	eax, 4		// 0x000438ae
		dec	ecx		// 0x000438b1
		mov	dword ptr [ebp + 0x24], ecx		// 0x000438b2
		jne	L4389b		// 0x000438b5
L438b7:
		lea	eax, [ebx*4]		// 0x000438b7
		add	eax, 3		// 0x000438be
		and	eax, 0xfffffffc		// 0x000438c1
		call	_chkstk		// 0x000438c4
		lea	eax, [ebx*4]		// 0x000438c9
		add	eax, 3		// 0x000438d0
		and	eax, 0xfffffffc		// 0x000438d3
		mov	dword ptr [ebp + 0x28], esp		// 0x000438d6
		call	_chkstk		// 0x000438d9
		lea	ecx, [ebp - 0x16c]		// 0x000438de
		mov	dword ptr [ebp + 0x1c], esp		// 0x000438e4
		call	nxConvexMeshCallRadixSortCtor		// 0x000438e7
		mov	eax, dword ptr [ebp + 0x1c]		// 0x000438ec
		mov	ecx, dword ptr [ebp + 0x28]		// 0x000438ef
		push	eax		// 0x000438f2
		push	ecx		// 0x000438f3
		lea	ecx, [ebp - 0x16c]		// 0x000438f4
		call	nxConvexMeshCallRadixSortSetRankBuffers		// 0x000438fa
		mov	edx, dword ptr [ebp - 4]		// 0x000438ff
		push	1		// 0x00043902
		push	ebx		// 0x00043904
		push	edx		// 0x00043905
		lea	ecx, [ebp - 0x16c]		// 0x00043906
		call	nxConvexMeshCallRadixSortSort		// 0x0004390c
		mov	eax, dword ptr [eax + 4]		// 0x00043911
		mov	edx, dword ptr [eax]		// 0x00043914
		lea	ecx, [eax + ebx*4]		// 0x00043916
		mov	dword ptr [ebp - 0x44], eax		// 0x00043919
		mov	eax, dword ptr [ebp - 4]		// 0x0004391c
		mov	dword ptr [ebp - 0x68], ecx		// 0x0004391f
		mov	ecx, dword ptr [eax + edx*4]		// 0x00043922
		lea	eax, [ebx*4 + 4]		// 0x00043925
		add	eax, 3		// 0x0004392c
		and	eax, 0xfffffffc		// 0x0004392f
		mov	dword ptr [ebp - 0x40], ecx		// 0x00043932
		call	_chkstk		// 0x00043935
		xor	eax, eax		// 0x0004393a
		mov	dword ptr [ebp + 0x4c], eax		// 0x0004393c
		mov	dword ptr [ebp + 0x20], eax		// 0x0004393f
		lea	eax, [ebx + ebx*4]		// 0x00043942
		shl	eax, 2		// 0x00043945
		add	eax, 3		// 0x00043948
		and	eax, 0xfffffffc		// 0x0004394b
		mov	dword ptr [ebp + 0x30], esp		// 0x0004394e
		call	_chkstk		// 0x00043951
		inc	ebx		// 0x00043956
		mov	dword ptr [ebp + 0x24], esp		// 0x00043957
		je	L441b8		// 0x0004395a
		mov	dword ptr [ebp + 0x1c], ebx		// 0x00043960
L43963:
		mov	ecx, dword ptr [ebp - 0x44]		// 0x00043963
		cmp	ecx, dword ptr [ebp - 0x68]		// 0x00043966
		je	L43984		// 0x00043969
		mov	eax, dword ptr [ecx]		// 0x0004396b
		mov	edx, dword ptr [ebp - 4]		// 0x0004396d
		add	ecx, 4		// 0x00043970
		mov	dword ptr [ebp - 0x44], ecx		// 0x00043973
		mov	ecx, dword ptr [edx + eax*4]		// 0x00043976
		mov	edx, dword ptr [ebp]		// 0x00043979
		mov	eax, dword ptr [edx + eax*4]		// 0x0004397c
		mov	dword ptr [ebp + 0x28], eax		// 0x0004397f
		jmp	L4398a		// 0x00043982
L43984:
		or	ecx, 0xffffffff		// 0x00043984
		mov	dword ptr [ebp + 0x28], ecx		// 0x00043987
L4398a:
		cmp	ecx, dword ptr [ebp - 0x40]		// 0x0004398a
		je	L4419f		// 0x0004398d
		mov	ebx, dword ptr [ebp + 0x4c]		// 0x00043993
		lea	eax, [ebx*4]		// 0x00043996
		add	eax, 3		// 0x0004399d
		and	eax, 0xfffffffc		// 0x000439a0
		mov	dword ptr [ebp - 0x40], ecx		// 0x000439a3
		call	_chkstk		// 0x000439a6
		test	ebx, ebx		// 0x000439ab
		mov	eax, esp		// 0x000439ad
		mov	dword ptr [ebp - 0xc], eax		// 0x000439af
		mov	dword ptr [ebp - 8], 0		// 0x000439b2
		mov	dword ptr [ebp - 0x10], 0		// 0x000439b9
		mov	dword ptr [ebp - 0x14], 0		// 0x000439c0
		mov	dword ptr [ebp - 0x18], 0		// 0x000439c7
		jbe	L43af4		// 0x000439ce
		mov	ecx, dword ptr [ebp + 0x30]		// 0x000439d4
		sub	eax, ecx		// 0x000439d7
		mov	dword ptr [ebp + 0x38], ecx		// 0x000439d9
		mov	dword ptr [ebp + 0x3c], eax		// 0x000439dc
		mov	dword ptr [ebp + 0x10], ebx		// 0x000439df
L439e2:
		mov	ecx, dword ptr [ebp + 0x38]		// 0x000439e2
		mov	eax, dword ptr [ecx]		// 0x000439e5
		mov	ecx, dword ptr [edi + 0x14]		// 0x000439e7
		lea	edx, [eax + eax*2]		// 0x000439ea
		mov	ebx, dword ptr [ecx + edx*4]		// 0x000439ed
		lea	ecx, [ecx + edx*4]		// 0x000439f0
		mov	edx, dword ptr [edi + 0x10]		// 0x000439f3
		lea	ebx, [ebx + ebx*2]		// 0x000439f6
		lea	ebx, [edx + ebx*4]		// 0x000439f9
		mov	dword ptr [ebp + 0x18], ebx		// 0x000439fc
		mov	ebx, dword ptr [ecx + 4]		// 0x000439ff
		mov	ecx, dword ptr [ecx + 8]		// 0x00043a02
		lea	ecx, [ecx + ecx*2]		// 0x00043a05
		lea	ebx, [ebx + ebx*2]		// 0x00043a08
		lea	ebx, [edx + ebx*4]		// 0x00043a0b
		lea	edx, [edx + ecx*4]		// 0x00043a0e
		mov	ecx, dword ptr [ebp - 0x6c]		// 0x00043a11
		mov	dword ptr [ebp + 0x34], edx		// 0x00043a14
		mov	edx, dword ptr [ecx + eax*4]		// 0x00043a17
		mov	eax, dword ptr [ebp + 0x3c]		// 0x00043a1a
		mov	ecx, dword ptr [ebp + 0x38]		// 0x00043a1d
		mov	dword ptr [eax + ecx], edx		// 0x00043a20
		mov	eax, dword ptr [ebp + 0x18]		// 0x00043a23
		mov	edx, dword ptr [eax]		// 0x00043a26
		mov	dword ptr [ebp - 0xd4], edx		// 0x00043a28
		mov	ecx, dword ptr [eax + 4]		// 0x00043a2e
		mov	dword ptr [ebp - 0xd0], ecx		// 0x00043a31
		mov	edx, dword ptr [eax + 8]		// 0x00043a37
		mov	dword ptr [ebp - 0xcc], edx		// 0x00043a3a
		mov	eax, dword ptr [ebx]		// 0x00043a40
		mov	dword ptr [ebp - 0xc8], eax		// 0x00043a42
		mov	ecx, dword ptr [ebx + 4]		// 0x00043a48
		mov	eax, dword ptr [ebp + 0x34]		// 0x00043a4b
		mov	dword ptr [ebp - 0xc4], ecx		// 0x00043a4e
		mov	edx, dword ptr [ebx + 8]		// 0x00043a54
		mov	dword ptr [ebp - 0xc0], edx		// 0x00043a57
		mov	ecx, dword ptr [eax]		// 0x00043a5d
		mov	dword ptr [ebp - 0xbc], ecx		// 0x00043a5f
		mov	edx, dword ptr [eax + 4]		// 0x00043a65
		mov	dword ptr [ebp - 0xb8], edx		// 0x00043a68
		mov	eax, dword ptr [eax + 8]		// 0x00043a6e
		lea	ecx, [ebp - 0xd4]		// 0x00043a71
		mov	dword ptr [ebp - 0xb4], eax		// 0x00043a77
		call	nxConvexMeshCallTriangleArea		// 0x00043a7d
		fstp	dword ptr [ebp + 0x50]		// 0x00043a82
		lea	ecx, [ebp - 0xa0]		// 0x00043a85
		push	ecx		// 0x00043a8b
		lea	ecx, [ebp - 0xd4]		// 0x00043a8c
		call	nxConvexMeshCallTriangleCenter		// 0x00043a92
		fld	dword ptr [ebp - 0xa0]		// 0x00043a97
		fmul	dword ptr [ebp + 0x50]		// 0x00043a9d
		mov	ecx, dword ptr [ebp + 0x38]		// 0x00043aa0
		fld	dword ptr [ebp - 0x9c]		// 0x00043aa3
		mov	eax, dword ptr [ebp + 0x10]		// 0x00043aa9
		fmul	dword ptr [ebp + 0x50]		// 0x00043aac
		add	ecx, 4		// 0x00043aaf
		fld	dword ptr [ebp - 0x98]		// 0x00043ab2
		dec	eax		// 0x00043ab8
		fmul	dword ptr [ebp + 0x50]		// 0x00043ab9
		mov	dword ptr [ebp + 0x38], ecx		// 0x00043abc
		mov	dword ptr [ebp + 0x10], eax		// 0x00043abf
		fstp	dword ptr [ebp - 0x17c]		// 0x00043ac2
		fxch	st(1)		// 0x00043ac8
		fadd	dword ptr [ebp - 0x18]		// 0x00043aca
		fstp	dword ptr [ebp - 0x18]		// 0x00043acd
		fadd	dword ptr [ebp - 0x14]		// 0x00043ad0
		fstp	dword ptr [ebp - 0x14]		// 0x00043ad3
		fld	dword ptr [ebp - 0x17c]		// 0x00043ad6
		fadd	dword ptr [ebp - 0x10]		// 0x00043adc
		fstp	dword ptr [ebp - 0x10]		// 0x00043adf
		fld	dword ptr [ebp + 0x50]		// 0x00043ae2
		fadd	dword ptr [ebp - 8]		// 0x00043ae5
		fstp	dword ptr [ebp - 8]		// 0x00043ae8
		jne	L439e2		// 0x00043aeb
		mov	ebx, dword ptr [ebp + 0x4c]		// 0x00043af1
L43af4:
		fld	dword ptr kConvexMeshOne		// 0x00043af4
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00043afa
		fdiv	dword ptr [ebp - 8]		// 0x00043afd
		mov	edx, dword ptr [ecx]		// 0x00043b00
		mov	dword ptr [ebp + 0xc], 0xffffffff		// 0x00043b02
		mov	dword ptr [ebp - 0x34], 0x7f7fffff		// 0x00043b09
		fld	dword ptr [ebp - 0x18]		// 0x00043b10
		fmul	st, st(1)		// 0x00043b13
		fld	dword ptr [ebp - 0x14]		// 0x00043b15
		fmul	st, st(2)		// 0x00043b18
		fld	dword ptr [ebp - 0x10]		// 0x00043b1a
		fmul	st, st(3)		// 0x00043b1d
		fld	st(2)		// 0x00043b1f
		fmul	dword ptr [esi]		// 0x00043b21
		fld	st(1)		// 0x00043b23
		fmul	dword ptr [esi + 0x20]		// 0x00043b25
		faddp	st(1), st		// 0x00043b28
		fld	st(2)		// 0x00043b2a
		fmul	dword ptr [esi + 0x10]		// 0x00043b2c
		faddp	st(1), st		// 0x00043b2f
		fadd	dword ptr [esi + 0x30]		// 0x00043b31
		fld	st(3)		// 0x00043b34
		fmul	dword ptr [esi + 4]		// 0x00043b36
		fld	st(3)		// 0x00043b39
		fmul	dword ptr [esi + 0x14]		// 0x00043b3b
		faddp	st(1), st		// 0x00043b3e
		fld	st(2)		// 0x00043b40
		fmul	dword ptr [esi + 0x24]		// 0x00043b42
		faddp	st(1), st		// 0x00043b45
		fadd	dword ptr [esi + 0x34]		// 0x00043b47
		fld	st(4)		// 0x00043b4a
		fmul	dword ptr [esi + 8]		// 0x00043b4c
		fld	st(4)		// 0x00043b4f
		fmul	dword ptr [esi + 0x18]		// 0x00043b51
		faddp	st(1), st		// 0x00043b54
		fld	st(3)		// 0x00043b56
		fmul	dword ptr [esi + 0x28]		// 0x00043b58
		faddp	st(1), st		// 0x00043b5b
		fadd	dword ptr [esi + 0x38]		// 0x00043b5d
		fstp	dword ptr [ebp - 0x1a0]		// 0x00043b60
		fld	dword ptr [ebp - 0x64]		// 0x00043b66
		fsub	st, st(2)		// 0x00043b69
		fstp	dword ptr [ebp - 0x7c]		// 0x00043b6b
		fld	dword ptr [ebp - 0x60]		// 0x00043b6e
		fsub	st, st(1)		// 0x00043b71
		fstp	dword ptr [ebp - 0x78]		// 0x00043b73
		fstp	st(0)		// 0x00043b76
		fstp	st(0)		// 0x00043b78
		fld	dword ptr [ebp - 0x5c]		// 0x00043b7a
		fsub	dword ptr [ebp - 0x1a0]		// 0x00043b7d
		fstp	dword ptr [ebp - 0x74]		// 0x00043b83
		fld	st(2)		// 0x00043b86
		fmul	dword ptr [ebp - 0x154]		// 0x00043b88
		fld	st(1)		// 0x00043b8e
		fmul	dword ptr [ebp - 0x134]		// 0x00043b90
		faddp	st(1), st		// 0x00043b96
		fld	st(2)		// 0x00043b98
		fmul	dword ptr [ebp - 0x144]		// 0x00043b9a
		faddp	st(1), st		// 0x00043ba0
		fadd	dword ptr [ebp - 0x124]		// 0x00043ba2
		fstp	dword ptr [ebp - 0x94]		// 0x00043ba8
		fld	dword ptr [ebp - 0x130]		// 0x00043bae
		fmul	st, st(1)		// 0x00043bb4
		fld	st(2)		// 0x00043bb6
		fmul	dword ptr [ebp - 0x140]		// 0x00043bb8
		faddp	st(1), st		// 0x00043bbe
		fld	st(3)		// 0x00043bc0
		fmul	dword ptr [ebp - 0x150]		// 0x00043bc2
		faddp	st(1), st		// 0x00043bc8
		fadd	dword ptr [ebp - 0x120]		// 0x00043bca
		fstp	dword ptr [ebp - 0x90]		// 0x00043bd0
		fmul	dword ptr [ebp - 0x12c]		// 0x00043bd6
		fxch	st(1)		// 0x00043bdc
		fmul	dword ptr [ebp - 0x13c]		// 0x00043bde
		faddp	st(1), st		// 0x00043be4
		fxch	st(1)		// 0x00043be6
		fmul	dword ptr [ebp - 0x14c]		// 0x00043be8
		faddp	st(1), st		// 0x00043bee
		fadd	dword ptr [ebp - 0x11c]		// 0x00043bf0
		fstp	dword ptr [ebp - 0x8c]		// 0x00043bf6
		fstp	st(0)		// 0x00043bfc
		call	dword ptr [edx + 0xc]		// 0x00043bfe
		shl	eax, 2		// 0x00043c01
		add	eax, 3		// 0x00043c04
		and	eax, 0xfffffffc		// 0x00043c07
		call	_chkstk		// 0x00043c0a
		mov	eax, esp		// 0x00043c0f
		lea	ecx, [ebp - 0x58]		// 0x00043c11
		push	ecx		// 0x00043c14
		push	eax		// 0x00043c15
		mov	dword ptr [ebp + 0x14], eax		// 0x00043c16
		lea	edx, [ebp + 0xc]		// 0x00043c19
		push	edx		// 0x00043c1c
		mov	edx, dword ptr [ebp + 0x30]		// 0x00043c1d
		lea	eax, [ebp - 0x178]		// 0x00043c20
		push	eax		// 0x00043c26
		mov	eax, dword ptr [ebp + 0x70]		// 0x00043c27
		lea	ecx, [ebp - 0x34]		// 0x00043c2a
		push	ecx		// 0x00043c2d
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00043c2e
		push	edi		// 0x00043c31
		push	edx		// 0x00043c32
		push	ebx		// 0x00043c33
		push	eax		// 0x00043c34
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00043c35
		push	ecx		// 0x00043c38
		lea	edx, [ebp - 0x94]		// 0x00043c39
		push	edx		// 0x00043c3f
		push	eax		// 0x00043c40
		lea	ebx, [ebp - 0x114]		// 0x00043c41
		call	nxConvexMeshFaceAxes		// 0x00043c47
		add	esp, 0x30		// 0x00043c4c
		test	al, al		// 0x00043c4f
		je	L44197		// 0x00043c51
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00043c57
		mov	ecx, dword ptr [eax + 0x4e4]		// 0x00043c5a
		add	eax, 0x4e0		// 0x00043c60
		test	ecx, ecx		// 0x00043c65
		mov	dword ptr [ebp + 0x40], 0		// 0x00043c67
		mov	dword ptr [ebp + 0x44], 0		// 0x00043c6e
		mov	dword ptr [ebp + 0x48], 0		// 0x00043c75
		mov	dword ptr [ebp + 0x2c], 0x7f7fffff		// 0x00043c7c
		mov	dword ptr [ebp + 0x18], 0xffffffff		// 0x00043c83
		je	L43c93		// 0x00043c8a
		mov	dword ptr [eax + 4], 0		// 0x00043c8c
L43c93:
		mov	ebx, dword ptr [ebp + 0x4c]		// 0x00043c93
		shl	ebx, 2		// 0x00043c96
		mov	eax, ebx		// 0x00043c99
		add	eax, 3		// 0x00043c9b
		and	eax, 0xfffffffc		// 0x00043c9e
		call	_chkstk		// 0x00043ca1
		mov	eax, ebx		// 0x00043ca6
		add	eax, 3		// 0x00043ca8
		and	eax, 0xfffffffc		// 0x00043cab
		mov	dword ptr [ebp + 0x34], esp		// 0x00043cae
		call	_chkstk		// 0x00043cb1
		lea	ecx, [ebp - 0x19c]		// 0x00043cb6
		mov	dword ptr [ebp + 0x3c], esp		// 0x00043cbc
		call	nxConvexMeshCallRadixSortCtor		// 0x00043cbf
		mov	ecx, dword ptr [ebp + 0x3c]		// 0x00043cc4
		mov	edx, dword ptr [ebp + 0x34]		// 0x00043cc7
		push	ecx		// 0x00043cca
		push	edx		// 0x00043ccb
		lea	ecx, [ebp - 0x19c]		// 0x00043ccc
		call	nxConvexMeshCallRadixSortSetRankBuffers		// 0x00043cd2
		mov	eax, dword ptr [ebp + 0x4c]		// 0x00043cd7
		mov	ecx, dword ptr [ebp - 0xc]		// 0x00043cda
		push	1		// 0x00043cdd
		push	eax		// 0x00043cdf
		push	ecx		// 0x00043ce0
		lea	ecx, [ebp - 0x19c]		// 0x00043ce1
		call	nxConvexMeshCallRadixSortSort		// 0x00043ce7
		mov	eax, dword ptr [eax + 4]		// 0x00043cec
		mov	edx, dword ptr [eax]		// 0x00043cef
		mov	dword ptr [ebp + 0x3c], eax		// 0x00043cf1
		mov	eax, dword ptr [ebp - 0xc]		// 0x00043cf4
		mov	ecx, dword ptr [eax + edx*4]		// 0x00043cf7
		mov	eax, ebx		// 0x00043cfa
		add	eax, 3		// 0x00043cfc
		and	eax, 0xfffffffc		// 0x00043cff
		mov	dword ptr [ebp + 0x10], ecx		// 0x00043d02
		call	_chkstk		// 0x00043d05
		mov	edx, dword ptr [ebp + 0x24]		// 0x00043d0a
		mov	ebx, dword ptr [ebp + 0x4c]		// 0x00043d0d
		xor	ecx, ecx		// 0x00043d10
		xor	eax, eax		// 0x00043d12
		add	edx, 0x10		// 0x00043d14
		mov	dword ptr [ebp + 0x38], esp		// 0x00043d17
		mov	dword ptr [ebp + 0x50], ecx		// 0x00043d1a
		mov	dword ptr [ebp + 0x34], eax		// 0x00043d1d
		cmp	eax, ebx		// 0x00043d20
L43d22:
		je	L43d3e		// 0x00043d22
		mov	ecx, dword ptr [ebp + 0x3c]		// 0x00043d24
		mov	ecx, dword ptr [ecx + eax*4]		// 0x00043d27
		mov	eax, dword ptr [ebp - 0xc]		// 0x00043d2a
		mov	eax, dword ptr [eax + ecx*4]		// 0x00043d2d
		mov	ebx, dword ptr [ebp + 0x30]		// 0x00043d30
		mov	ecx, dword ptr [ebx + ecx*4]		// 0x00043d33
		mov	dword ptr [ebp + 8], ecx		// 0x00043d36
		mov	ecx, dword ptr [ebp + 0x50]		// 0x00043d39
		jmp	L43d44		// 0x00043d3c
L43d3e:
		or	eax, 0xffffffff		// 0x00043d3e
		mov	dword ptr [ebp + 8], eax		// 0x00043d41
L43d44:
		cmp	eax, dword ptr [ebp + 0x10]		// 0x00043d44
		je	L43ede		// 0x00043d47
		mov	ebx, dword ptr [edi + 0x14]		// 0x00043d4d
		mov	dword ptr [ebp + 0x10], eax		// 0x00043d50
		mov	eax, dword ptr [ebp + 0x38]		// 0x00043d53
		mov	eax, dword ptr [eax]		// 0x00043d56
		lea	ecx, [eax + eax*2]		// 0x00043d58
		mov	ecx, dword ptr [ebx + ecx*4]		// 0x00043d5b
		mov	ebx, dword ptr [edi + 0x10]		// 0x00043d5e
		shl	eax, 4		// 0x00043d61
		lea	ecx, [ecx + ecx*2]		// 0x00043d64
		lea	ecx, [ebx + ecx*4]		// 0x00043d67
		add	eax, dword ptr [edi + 0x24]		// 0x00043d6a
		mov	ebx, dword ptr [ebp + 0x50]		// 0x00043d6d
		mov	dword ptr [edx - 0xc], ebx		// 0x00043d70
		mov	edi, eax		// 0x00043d73
		fld	dword ptr [edi + 8]		// 0x00043d75
		mov	eax, dword ptr [ebp + 0x20]		// 0x00043d78
		fmul	dword ptr [esi + 0x20]		// 0x00043d7b
		inc	eax		// 0x00043d7e
		fld	dword ptr [esi]		// 0x00043d7f
		mov	dword ptr [ebp + 0x20], eax		// 0x00043d81
		fmul	dword ptr [edi]		// 0x00043d84
		add	edx, 0x14		// 0x00043d86
		mov	dword ptr [ebp + 0x50], edx		// 0x00043d89
		faddp	st(1), st		// 0x00043d8c
		fld	dword ptr [esi + 0x10]		// 0x00043d8e
		fmul	dword ptr [edi + 4]		// 0x00043d91
		faddp	st(1), st		// 0x00043d94
		fstp	dword ptr [edx - 0x1c]		// 0x00043d96
		fld	dword ptr [esi + 4]		// 0x00043d99
		fmul	dword ptr [edi]		// 0x00043d9c
		fld	dword ptr [edi + 8]		// 0x00043d9e
		fmul	dword ptr [esi + 0x24]		// 0x00043da1
		faddp	st(1), st		// 0x00043da4
		fld	dword ptr [edi + 4]		// 0x00043da6
		fmul	dword ptr [esi + 0x14]		// 0x00043da9
		faddp	st(1), st		// 0x00043dac
		fstp	dword ptr [edx - 0x18]		// 0x00043dae
		fld	dword ptr [esi + 8]		// 0x00043db1
		fmul	dword ptr [edi]		// 0x00043db4
		fld	dword ptr [edi + 8]		// 0x00043db6
		fmul	dword ptr [esi + 0x28]		// 0x00043db9
		faddp	st(1), st		// 0x00043dbc
		fld	dword ptr [edi + 4]		// 0x00043dbe
		fmul	dword ptr [esi + 0x18]		// 0x00043dc1
		faddp	st(1), st		// 0x00043dc4
		fstp	dword ptr [edx - 0x14]		// 0x00043dc6
		fld	dword ptr [ebp - 0x54]		// 0x00043dc9
		fsub	dword ptr [ecx]		// 0x00043dcc
		fld	dword ptr [ebp - 0x50]		// 0x00043dce
		fsub	dword ptr [ecx + 4]		// 0x00043dd1
		fld	dword ptr [ebp - 0x4c]		// 0x00043dd4
		fsub	dword ptr [ecx + 8]		// 0x00043dd7
		fxch	st(1)		// 0x00043dda
		fmul	dword ptr [edi + 4]		// 0x00043ddc
		fxch	st(2)		// 0x00043ddf
		fmul	dword ptr [edi]		// 0x00043de1
		faddp	st(2), st		// 0x00043de3
		fmul	dword ptr [edi + 8]		// 0x00043de5
		faddp	st(1), st		// 0x00043de8
		fcomp	dword ptr kConvexMeshZero		// 0x00043dea
		fnstsw	ax		// 0x00043df0
		test	ah, 0x41		// 0x00043df2
		jne	L43ed9		// 0x00043df5
		mov	edx, dword ptr [ebp + 0x38]		// 0x00043dfb
		mov	eax, dword ptr [ebp + 4]		// 0x00043dfe
		mov	ecx, dword ptr [ebp + 0x30]		// 0x00043e01
		push	edi		// 0x00043e04
		push	edx		// 0x00043e05
		mov	edx, dword ptr [ebp + 0x4c]		// 0x00043e06
		push	ebx		// 0x00043e09
		push	eax		// 0x00043e0a
		mov	eax, dword ptr [ebp + 0x70]		// 0x00043e0b
		push	ecx		// 0x00043e0e
		push	edx		// 0x00043e0f
		mov	edx, dword ptr [ebp + 0x6c]		// 0x00043e10
		push	eax		// 0x00043e13
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00043e14
		lea	ecx, [ebp - 0x114]		// 0x00043e17
		push	ecx		// 0x00043e1d
		push	edx		// 0x00043e1e
		push	eax		// 0x00043e1f
		lea	ecx, [ebp - 0x70]		// 0x00043e20
		push	ecx		// 0x00043e23
		lea	edx, [ebp - 0x48]		// 0x00043e24
		push	edx		// 0x00043e27
		mov	edx, dword ptr [ebp - 0x34]		// 0x00043e28
		lea	ecx, [ebp - 0x178]		// 0x00043e2b
		push	ecx		// 0x00043e31
		mov	ecx, dword ptr [ebp + 0xc]		// 0x00043e32
		push	edx		// 0x00043e35
		push	ecx		// 0x00043e36
		add	eax, 0x4e0		// 0x00043e37
		push	eax		// 0x00043e3c
		lea	ebx, [ebp - 0x30]		// 0x00043e3d
		call	nxConvexMeshEdgeAxes		// 0x00043e40
		add	esp, 0x40		// 0x00043e45
		test	al, al		// 0x00043e48
		je	L43ed6		// 0x00043e4a
		fld	dword ptr [ebp - 0x48]		// 0x00043e50
		fcomp	dword ptr [ebp + 0x2c]		// 0x00043e53
		fnstsw	ax		// 0x00043e56
		test	ah, 5		// 0x00043e58
		jp	L43ed6		// 0x00043e5b
		fld	dword ptr [ebp - 0x30]		// 0x00043e5d
		mov	edx, dword ptr [ebp - 0x48]		// 0x00043e60
		fmul	dword ptr [esi]		// 0x00043e63
		mov	eax, dword ptr [ebp - 0x70]		// 0x00043e65
		fld	dword ptr [ebp - 0x28]		// 0x00043e68
		mov	ecx, dword ptr [edi]		// 0x00043e6b
		fmul	dword ptr [esi + 0x20]		// 0x00043e6d
		mov	dword ptr [ebp + 0x2c], edx		// 0x00043e70
		mov	edx, dword ptr [edi + 4]		// 0x00043e73
		mov	dword ptr [ebp + 0x18], eax		// 0x00043e76
		faddp	st(1), st		// 0x00043e79
		mov	eax, dword ptr [edi + 8]		// 0x00043e7b
		fld	dword ptr [ebp - 0x2c]		// 0x00043e7e
		mov	dword ptr [ebp - 0xb0], ecx		// 0x00043e81
		fmul	dword ptr [esi + 0x10]		// 0x00043e87
		mov	ecx, dword ptr [edi + 0xc]		// 0x00043e8a
		mov	dword ptr [ebp - 0xac], edx		// 0x00043e8d
		mov	dword ptr [ebp - 0xa8], eax		// 0x00043e93
		faddp	st(1), st		// 0x00043e99
		mov	dword ptr [ebp - 0xa4], ecx		// 0x00043e9b
		fstp	dword ptr [ebp + 0x40]		// 0x00043ea1
		fld	dword ptr [ebp - 0x30]		// 0x00043ea4
		fmul	dword ptr [esi + 4]		// 0x00043ea7
		fld	dword ptr [ebp - 0x2c]		// 0x00043eaa
		fmul	dword ptr [esi + 0x14]		// 0x00043ead
		faddp	st(1), st		// 0x00043eb0
		fld	dword ptr [ebp - 0x28]		// 0x00043eb2
		fmul	dword ptr [esi + 0x24]		// 0x00043eb5
		faddp	st(1), st		// 0x00043eb8
		fstp	dword ptr [ebp + 0x44]		// 0x00043eba
		fld	dword ptr [ebp - 0x30]		// 0x00043ebd
		fmul	dword ptr [esi + 8]		// 0x00043ec0
		fld	dword ptr [ebp - 0x2c]		// 0x00043ec3
		fmul	dword ptr [esi + 0x18]		// 0x00043ec6
		faddp	st(1), st		// 0x00043ec9
		fld	dword ptr [ebp - 0x28]		// 0x00043ecb
		fmul	dword ptr [esi + 0x28]		// 0x00043ece
		faddp	st(1), st		// 0x00043ed1
		fstp	dword ptr [ebp + 0x48]		// 0x00043ed3
L43ed6:
		mov	edx, dword ptr [ebp + 0x50]		// 0x00043ed6
L43ed9:
		mov	edi, dword ptr [ebp + 4]		// 0x00043ed9
		xor	ecx, ecx		// 0x00043edc
L43ede:
		mov	eax, dword ptr [ebp + 8]		// 0x00043ede
		mov	ebx, dword ptr [ebp + 0x38]		// 0x00043ee1
		mov	dword ptr [ebx + ecx*4], eax		// 0x00043ee4
		mov	eax, dword ptr [ebp + 0x34]		// 0x00043ee7
		mov	ebx, dword ptr [ebp + 0x4c]		// 0x00043eea
		inc	ecx		// 0x00043eed
		inc	eax		// 0x00043eee
		cmp	eax, ebx		// 0x00043eef
		mov	dword ptr [ebp + 0x50], ecx		// 0x00043ef1
		mov	dword ptr [ebp + 0x34], eax		// 0x00043ef4
		jbe	L43d22		// 0x00043ef7
		cmp	dword ptr [ebp + 0x18], -1		// 0x00043efd
		je	L4418c		// 0x00043f01
		mov	eax, dword ptr [ebp + 0x30]		// 0x00043f07
		lea	ecx, [ebp - 0x3c]		// 0x00043f0a
		push	ecx		// 0x00043f0d
		lea	edx, [ebp - 0x24]		// 0x00043f0e
		push	edx		// 0x00043f11
		mov	edx, dword ptr [ebp + 0x70]		// 0x00043f12
		push	eax		// 0x00043f15
		mov	eax, dword ptr [ebp + 0x6c]		// 0x00043f16
		mov	ecx, ebx		// 0x00043f19
		push	ecx		// 0x00043f1b
		mov	ecx, dword ptr [ebp + 0x14]		// 0x00043f1c
		push	edx		// 0x00043f1f
		mov	edx, dword ptr [ebp - 0x58]		// 0x00043f20
		push	eax		// 0x00043f23
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00043f24
		push	edi		// 0x00043f27
		push	ecx		// 0x00043f28
		push	edx		// 0x00043f29
		push	eax		// 0x00043f2a
		add	eax, 0x4e0		// 0x00043f2b
		push	eax		// 0x00043f30
		lea	eax, [ebp - 0x154]		// 0x00043f31
		lea	ebx, [ebp - 0x114]		// 0x00043f37
		lea	edx, [ebp - 0xb0]		// 0x00043f3d
		call	nxConvexMeshCrossAxes		// 0x00043f43
		add	esp, 0x2c		// 0x00043f48
		test	al, al		// 0x00043f4b
		je	L4418c		// 0x00043f4d
		fld	dword ptr [ebp - 0x3c]		// 0x00043f53
		fcomp	dword ptr [ebp + 0x2c]		// 0x00043f56
		fnstsw	ax		// 0x00043f59
		test	ah, 5		// 0x00043f5b
		jp	L43fb0		// 0x00043f5e
		fld	dword ptr [ebp - 0x24]		// 0x00043f60
		mov	eax, dword ptr [ebp - 0x3c]		// 0x00043f63
		fmul	dword ptr [esi]		// 0x00043f66
		mov	dword ptr [ebp + 0x2c], eax		// 0x00043f68
		fld	dword ptr [ebp - 0x1c]		// 0x00043f6b
		fmul	dword ptr [esi + 0x20]		// 0x00043f6e
		faddp	st(1), st		// 0x00043f71
		fld	dword ptr [ebp - 0x20]		// 0x00043f73
		fmul	dword ptr [esi + 0x10]		// 0x00043f76
		faddp	st(1), st		// 0x00043f79
		fstp	dword ptr [ebp + 0x40]		// 0x00043f7b
		fld	dword ptr [ebp - 0x24]		// 0x00043f7e
		fmul	dword ptr [esi + 4]		// 0x00043f81
		fld	dword ptr [ebp - 0x20]		// 0x00043f84
		fmul	dword ptr [esi + 0x14]		// 0x00043f87
		faddp	st(1), st		// 0x00043f8a
		fld	dword ptr [ebp - 0x1c]		// 0x00043f8c
		fmul	dword ptr [esi + 0x24]		// 0x00043f8f
		faddp	st(1), st		// 0x00043f92
		fstp	dword ptr [ebp + 0x44]		// 0x00043f94
		fld	dword ptr [ebp - 0x24]		// 0x00043f97
		fmul	dword ptr [esi + 8]		// 0x00043f9a
		fld	dword ptr [ebp - 0x20]		// 0x00043f9d
		fmul	dword ptr [esi + 0x18]		// 0x00043fa0
		faddp	st(1), st		// 0x00043fa3
		fld	dword ptr [ebp - 0x1c]		// 0x00043fa5
		fmul	dword ptr [esi + 0x28]		// 0x00043fa8
		faddp	st(1), st		// 0x00043fab
		fstp	dword ptr [ebp + 0x48]		// 0x00043fad
L43fb0:
		mov	ecx, dword ptr [ebp + 0x20]		// 0x00043fb0
		cmp	ecx, 1		// 0x00043fb3
		mov	ebx, dword ptr [ebp + 0x24]		// 0x00043fb6
		mov	dword ptr [ebx], 0		// 0x00043fb9
		jbe	L43fd3		// 0x00043fbf
		lea	eax, [ebx + 4]		// 0x00043fc1
		dec	ecx		// 0x00043fc4
L43fc5:
		mov	edx, dword ptr [eax - 4]		// 0x00043fc5
		add	edx, dword ptr [eax]		// 0x00043fc8
		add	eax, 0x14		// 0x00043fca
		dec	ecx		// 0x00043fcd
		mov	dword ptr [eax - 4], edx		// 0x00043fce
		jne	L43fc5		// 0x00043fd1
L43fd3:
		fld	dword ptr [ebp + 0x40]		// 0x00043fd3
		fmul	dword ptr [ebp - 0x7c]		// 0x00043fd6
		fld	dword ptr [ebp + 0x48]		// 0x00043fd9
		fmul	dword ptr [ebp - 0x74]		// 0x00043fdc
		faddp	st(1), st		// 0x00043fdf
		fld	dword ptr [ebp + 0x44]		// 0x00043fe1
		fmul	dword ptr [ebp - 0x78]		// 0x00043fe4
		faddp	st(1), st		// 0x00043fe7
		fcomp	dword ptr kConvexMeshZero		// 0x00043fe9
		fnstsw	ax		// 0x00043fef
		test	ah, 5		// 0x00043ff1
		jp	L4400e		// 0x00043ff4
		fld	dword ptr [ebp + 0x40]		// 0x00043ff6
		fchs		// 0x00043ff9
		fstp	dword ptr [ebp + 0x40]		// 0x00043ffb
		fld	dword ptr [ebp + 0x44]		// 0x00043ffe
		fchs		// 0x00044001
		fstp	dword ptr [ebp + 0x44]		// 0x00044003
		fld	dword ptr [ebp + 0x48]		// 0x00044006
		fchs		// 0x00044009
		fstp	dword ptr [ebp + 0x48]		// 0x0004400b
L4400e:
		mov	eax, dword ptr [ebp + 0x20]		// 0x0004400e
		or	ecx, 0xffffffff		// 0x00044011
		xor	edx, edx		// 0x00044014
		test	eax, eax		// 0x00044016
		mov	dword ptr [ebp - 0x38], 0xff7fffff		// 0x00044018
		jbe	L4405a		// 0x0004401f
		add	ebx, 0xc		// 0x00044021
L44024:
		fld	dword ptr [ebp + 0x48]		// 0x00044024
		fmul	dword ptr [ebx + 4]		// 0x00044027
		fld	dword ptr [ebp + 0x40]		// 0x0004402a
		fmul	dword ptr [ebx - 4]		// 0x0004402d
		faddp	st(1), st		// 0x00044030
		fld	dword ptr [ebp + 0x44]		// 0x00044032
		fmul	dword ptr [ebx]		// 0x00044035
		faddp	st(1), st		// 0x00044037
		fcom	dword ptr [ebp - 0x38]		// 0x00044039
		fnstsw	ax		// 0x0004403c
		test	ah, 0x41		// 0x0004403e
		jne	L4404a		// 0x00044041
		fstp	dword ptr [ebp - 0x38]		// 0x00044043
		mov	ecx, edx		// 0x00044046
		jmp	L4404c		// 0x00044048
L4404a:
		fstp	st(0)		// 0x0004404a
L4404c:
		mov	eax, dword ptr [ebp + 0x20]		// 0x0004404c
		inc	edx		// 0x0004404f
		add	ebx, 0x14		// 0x00044050
		cmp	edx, eax		// 0x00044053
		jb	L44024		// 0x00044055
		mov	ebx, dword ptr [ebp + 0x24]		// 0x00044057
L4405a:
		fld	dword ptr [ebp + 0x40]		// 0x0004405a
		lea	eax, [ecx + ecx*4]		// 0x0004405d
		mov	ecx, dword ptr [ebx + eax*4 + 4]		// 0x00044060
		fchs		// 0x00044064
		mov	edx, dword ptr [ebx + eax*4]		// 0x00044066
		fstp	dword ptr [ebp - 0x88]		// 0x00044069
		fld	dword ptr [ebp + 0x44]		// 0x0004406f
		lea	eax, [ebx + eax*4]		// 0x00044072
		mov	eax, dword ptr [ebp + 0x3c]		// 0x00044075
		fchs		// 0x00044078
		lea	ebx, [eax + edx*4]		// 0x0004407a
		fstp	dword ptr [ebp - 0x84]		// 0x0004407d
		mov	eax, dword ptr [ebp + 0x64]		// 0x00044083
		fld	dword ptr [ebp + 0x48]		// 0x00044086
		push	eax		// 0x00044089
		fchs		// 0x0004408a
		mov	dword ptr [ebp + 0x14], ecx		// 0x0004408c
		fstp	dword ptr [ebp - 0x80]		// 0x0004408f
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00044092
		mov	edx, dword ptr [ecx]		// 0x00044095
		lea	eax, [ebp - 0x88]		// 0x00044097
		push	eax		// 0x0004409d
		call	dword ptr [edx + 0x24]		// 0x0004409e
		fld	dword ptr [ebp + 0x40]		// 0x000440a1
		fmul	dword ptr [ebp + 0x2c]		// 0x000440a4
		mov	ecx, dword ptr [edi + 0x1c]		// 0x000440a7
		mov	dword ptr [ebp + 0xc], eax		// 0x000440aa
		mov	eax, dword ptr [ebp + 0x14]		// 0x000440ad
		test	eax, eax		// 0x000440b0
		fstp	dword ptr [ebp + 0x40]		// 0x000440b2
		fld	dword ptr [ebp + 0x44]		// 0x000440b5
		mov	dword ptr [ebp + 0x18], ecx		// 0x000440b8
		fmul	dword ptr [ebp + 0x2c]		// 0x000440bb
		fstp	dword ptr [ebp + 0x44]		// 0x000440be
		fld	dword ptr [ebp + 0x48]		// 0x000440c1
		fmul	dword ptr [ebp + 0x2c]		// 0x000440c4
		fstp	dword ptr [ebp + 0x48]		// 0x000440c7
		je	L4418c		// 0x000440ca
		mov	dword ptr [ebp + 0x3c], eax		// 0x000440d0
		jmp	L440d8		// 0x000440d3
L440d5:
		mov	ebx, dword ptr [ebp + 0x14]		// 0x000440d5
L440d8:
		mov	edx, dword ptr [ebx]		// 0x000440d8
		mov	eax, dword ptr [ebp + 0x30]		// 0x000440da
		mov	ecx, dword ptr [eax + edx*4]		// 0x000440dd
		mov	eax, dword ptr [edi + 0x14]		// 0x000440e0
		add	ebx, 4		// 0x000440e3
		mov	dword ptr [ebp + 0x14], ebx		// 0x000440e6
		lea	edx, [ecx + ecx*2]		// 0x000440e9
		mov	ebx, dword ptr [eax + edx*4]		// 0x000440ec
		lea	eax, [eax + edx*4]		// 0x000440ef
		mov	edx, dword ptr [edi + 0x10]		// 0x000440f2
		lea	ebx, [ebx + ebx*2]		// 0x000440f5
		lea	ebx, [edx + ebx*4]		// 0x000440f8
		mov	dword ptr [ebp + 8], ebx		// 0x000440fb
		mov	ebx, dword ptr [eax + 4]		// 0x000440fe
		mov	eax, dword ptr [eax + 8]		// 0x00044101
		lea	eax, [eax + eax*2]		// 0x00044104
		lea	ebx, [ebx + ebx*2]		// 0x00044107
		lea	ebx, [edx + ebx*4]		// 0x0004410a
		lea	edx, [edx + eax*4]		// 0x0004410d
		mov	dword ptr [ebp + 0x34], edx		// 0x00044110
		mov	edx, dword ptr [edi + 0x24]		// 0x00044113
		mov	eax, ecx		// 0x00044116
		shl	eax, 4		// 0x00044118
		add	eax, edx		// 0x0004411b
		mov	edx, dword ptr [ebp + 0x18]		// 0x0004411d
		test	edx, edx		// 0x00044120
		je	L44129		// 0x00044122
		mov	edx, dword ptr [edx + ecx*4]		// 0x00044124
		jmp	L4412b		// 0x00044127
L44129:
		mov	edx, ecx		// 0x00044129
L4412b:
		mov	edi, dword ptr [edi + 0x18]		// 0x0004412b
		test	edi, edi		// 0x0004412e
		je	L44138		// 0x00044130
		mov	cx, word ptr [edi + ecx*2]		// 0x00044132
		jmp	L4413d		// 0x00044136
L44138:
		mov	ecx, 0xffff		// 0x00044138
L4413d:
		push	ecx		// 0x0004413d
		mov	ecx, dword ptr [ebp + 0x78]		// 0x0004413e
		push	0		// 0x00044141
		push	ecx		// 0x00044143
		lea	ecx, [ebp - 0x154]		// 0x00044144
		push	ecx		// 0x0004414a
		lea	ecx, [ebp - 0x114]		// 0x0004414b
		push	ecx		// 0x00044151
		mov	ecx, dword ptr [ebp + 0x5c]		// 0x00044152
		push	ecx		// 0x00044155
		mov	ecx, dword ptr [ebp + 0x60]		// 0x00044156
		push	ecx		// 0x00044159
		mov	ecx, dword ptr [ebp + 8]		// 0x0004415a
		push	edx		// 0x0004415d
		mov	edx, dword ptr [ebp + 0x34]		// 0x0004415e
		push	edx		// 0x00044161
		mov	edx, dword ptr [ebp + 0x6c]		// 0x00044162
		push	ebx		// 0x00044165
		mov	ebx, dword ptr [ebp + 0x64]		// 0x00044166
		push	ecx		// 0x00044169
		push	edx		// 0x0004416a
		mov	edx, dword ptr [ebp + 0xc]		// 0x0004416b
		lea	ecx, [ebp + 0x40]		// 0x0004416e
		push	ecx		// 0x00044171
		mov	edi, eax		// 0x00044172
		call	nxConvexMeshContacts		// 0x00044174
		mov	eax, dword ptr [ebp + 0x3c]		// 0x00044179
		mov	edi, dword ptr [ebp + 4]		// 0x0004417c
		add	esp, 0x34		// 0x0004417f
		dec	eax		// 0x00044182
		mov	dword ptr [ebp + 0x3c], eax		// 0x00044183
		jne	L440d5		// 0x00044186
L4418c:
		lea	ecx, [ebp - 0x19c]		// 0x0004418c
		call	nxConvexMeshCallRadixSortDtor		// 0x00044192
L44197:
		xor	eax, eax		// 0x00044197
		mov	dword ptr [ebp + 0x20], eax		// 0x00044199
		mov	dword ptr [ebp + 0x4c], eax		// 0x0004419c
L4419f:
		mov	eax, dword ptr [ebp + 0x4c]		// 0x0004419f
		mov	edx, dword ptr [ebp + 0x30]		// 0x000441a2
		mov	ecx, dword ptr [ebp + 0x28]		// 0x000441a5
		mov	dword ptr [edx + eax*4], ecx		// 0x000441a8
		inc	eax		// 0x000441ab
		mov	dword ptr [ebp + 0x4c], eax		// 0x000441ac
		dec	dword ptr [ebp + 0x1c]		// 0x000441af
		jne	L43963		// 0x000441b2
L441b8:
		lea	ecx, [ebp - 0x16c]		// 0x000441b8
		call	nxConvexMeshCallRadixSortDtor		// 0x000441be
L441c3:
		lea	esp, [ebp - 0x1b4]		// 0x000441c3
		pop	edi		// 0x000441c9
		pop	esi		// 0x000441ca
		pop	ebx		// 0x000441cb
		add	ebp, 0x54		// 0x000441cc
		mov	esp, ebp		// 0x000441cf
		pop	ebp		// 0x000441d1
		ret		// 0x000441d2
		}
	}

// phys_fn_001851 (0x000441e0, 790 B)
// The convex/triangle-mesh entry's body (cdecl: the convex shape, the mesh
// shape, the sink, the context). When the mesh shape's owner (001281) has no
// holder at +0x08, 002266 (its result unread). The convex's pose to a 4x4 (four
// rotation words kept on the x87 stack through `fst`), the mesh's to another;
// the convex mesh's local bounds (+0x44..+0x58) as a centre and extents (x 0.5f),
// the centre through the convex's pose and the pose's rotation copied beside
// them, the query box; then 001849 with the convex mesh's polygon interface
// (+0x04; null only for a null mesh, which the row has already read through) and
// its support map (+0xa8).
__declspec(naked) void nxContactConvexMeshEntry()
	{
	__asm
		{
		sub	esp, 0xf8		// 0x000441e0
		push	ebx		// 0x000441e6
		mov	ebx, dword ptr [esp + 0x104]		// 0x000441e7
		push	ebp		// 0x000441ee
		mov	ecx, ebx		// 0x000441ef
		call	NxShapeOwner		// 0x000441f1
		mov	ecx, dword ptr [eax + 8]		// 0x000441f6
		test	ecx, ecx		// 0x000441f9
		mov	ebp, dword ptr [esp + 0x104]		// 0x000441fb
		jne	L44216		// 0x00044202
		mov	eax, dword ptr [esp + 0x10c]		// 0x00044204
		push	eax		// 0x0004420b
		push	ebx		// 0x0004420c
		push	ebp		// 0x0004420d
		call	NxContinuousCdPair		// 0x0004420e
		add	esp, 0xc		// 0x00044213
L44216:
		mov	eax, dword ptr [ebp + 0xe0]		// 0x00044216
		fld	dword ptr [ebp + 0xc]		// 0x0004421c
		mov	edx, dword ptr [eax + 0xa8]		// 0x0004421f
		fst	dword ptr [esp + 0x14]		// 0x00044225
		mov	ecx, dword ptr [ebp + 0x14]		// 0x00044229
		fld	dword ptr [ebp + 0x10]		// 0x0004422c
		mov	dword ptr [esp + 0x34], ecx		// 0x0004422f
		fst	dword ptr [esp + 0x24]		// 0x00044233
		mov	ecx, dword ptr [ebp + 0x1c]		// 0x00044237
		fld	dword ptr [ebp + 0x18]		// 0x0004423a
		mov	dword ptr [esp + 0x28], ecx		// 0x0004423d
		fst	dword ptr [esp + 0x18]		// 0x00044241
		mov	ecx, dword ptr [ebp + 0x20]		// 0x00044245
		fld	dword ptr [ebp + 0x24]		// 0x00044248
		mov	dword ptr [esp + 0x38], ecx		// 0x0004424b
		fst	dword ptr [esp + 0x1c]		// 0x0004424f
		mov	ecx, dword ptr [ebp + 0x28]		// 0x00044253
		mov	dword ptr [esp + 0x2c], ecx		// 0x00044256
		mov	ecx, dword ptr [ebp + 0x2c]		// 0x0004425a
		mov	dword ptr [esp + 0x3c], ecx		// 0x0004425d
		mov	ecx, dword ptr [ebp + 0x30]		// 0x00044261
		mov	dword ptr [esp + 0x44], ecx		// 0x00044264
		mov	ecx, dword ptr [ebp + 0x34]		// 0x00044268
		mov	dword ptr [esp + 0x48], ecx		// 0x0004426b
		mov	ecx, dword ptr [ebp + 0x38]		// 0x0004426f
		mov	dword ptr [esp + 0x4c], ecx		// 0x00044272
		mov	ecx, dword ptr [ebx + 0xc]		// 0x00044276
		mov	dword ptr [esp + 0xc0], ecx		// 0x00044279
		mov	ecx, dword ptr [ebx + 0x10]		// 0x00044280
		mov	dword ptr [esp + 0xd0], ecx		// 0x00044283
		mov	ecx, dword ptr [ebx + 0x14]		// 0x0004428a
		mov	dword ptr [esp + 0xe0], ecx		// 0x0004428d
		mov	ecx, dword ptr [ebx + 0x18]		// 0x00044294
		mov	dword ptr [esp + 0xc4], ecx		// 0x00044297
		mov	ecx, dword ptr [ebx + 0x1c]		// 0x0004429e
		mov	dword ptr [esp + 0xd4], ecx		// 0x000442a1
		mov	ecx, dword ptr [ebx + 0x20]		// 0x000442a8
		mov	dword ptr [esp + 0xe4], ecx		// 0x000442ab
		mov	ecx, dword ptr [ebx + 0x24]		// 0x000442b2
		mov	dword ptr [esp + 0xc8], ecx		// 0x000442b5
		mov	ecx, dword ptr [ebx + 0x28]		// 0x000442bc
		mov	dword ptr [esp + 0xd8], ecx		// 0x000442bf
		mov	ecx, dword ptr [ebx + 0x2c]		// 0x000442c6
		mov	dword ptr [esp + 0xe8], ecx		// 0x000442c9
		mov	ecx, dword ptr [ebx + 0x30]		// 0x000442d0
		mov	dword ptr [esp + 0xf0], ecx		// 0x000442d3
		mov	ecx, dword ptr [ebx + 0x34]		// 0x000442da
		mov	dword ptr [esp + 0xf4], ecx		// 0x000442dd
		mov	ecx, dword ptr [ebx + 0x38]		// 0x000442e4
		mov	dword ptr [esp + 0xf8], ecx		// 0x000442e7
		mov	dword ptr [esp + 0x40], 0		// 0x000442ee
		mov	dword ptr [esp + 0x30], 0		// 0x000442f6
		mov	dword ptr [esp + 0x20], 0		// 0x000442fe
		mov	dword ptr [esp + 0x50], 0x3f800000		// 0x00044306
		mov	dword ptr [esp + 0xec], 0		// 0x0004430e
		mov	dword ptr [esp + 0xdc], 0		// 0x00044319
		mov	dword ptr [esp + 0xcc], 0		// 0x00044324
		mov	dword ptr [esp + 0xfc], 0x3f800000		// 0x0004432f
		fld	dword ptr [eax + 0x44]		// 0x0004433a
		fadd	dword ptr [eax + 0x50]		// 0x0004433d
		push	esi		// 0x00044340
		fld	dword ptr [eax + 0x54]		// 0x00044341
		push	edi		// 0x00044344
		fadd	dword ptr [eax + 0x48]		// 0x00044345
		fld	dword ptr [eax + 0x58]		// 0x00044348
		fadd	dword ptr [eax + 0x4c]		// 0x0004434b
		fstp	dword ptr [esp + 0x64]		// 0x0004434e
		fxch	st(1)		// 0x00044352
		fmul	dword ptr kConvexMeshHalf		// 0x00044354
		fstp	dword ptr [esp + 0x10]		// 0x0004435a
		mov	ecx, dword ptr [esp + 0x10]		// 0x0004435e
		mov	dword ptr [esp + 0x68], ecx		// 0x00044362
		fmul	dword ptr kConvexMeshHalf		// 0x00044366
		fstp	dword ptr [esp + 0x14]		// 0x0004436c
		mov	ecx, dword ptr [esp + 0x14]		// 0x00044370
		fld	dword ptr [esp + 0x64]		// 0x00044374
		mov	dword ptr [esp + 0x6c], ecx		// 0x00044378
		fmul	dword ptr kConvexMeshHalf		// 0x0004437c
		fstp	dword ptr [esp + 0x18]		// 0x00044382
		mov	ecx, dword ptr [esp + 0x18]		// 0x00044386
		mov	dword ptr [esp + 0x70], ecx		// 0x0004438a
		test	eax, eax		// 0x0004438e
		fld	dword ptr [eax + 0x50]		// 0x00044390
		fsub	dword ptr [eax + 0x44]		// 0x00044393
		lea	esi, [esp + 0xa4]		// 0x00044396
		fld	dword ptr [eax + 0x54]		// 0x0004439d
		lea	edi, [esp + 0x80]		// 0x000443a0
		fsub	dword ptr [eax + 0x48]		// 0x000443a7
		fld	dword ptr [eax + 0x58]		// 0x000443aa
		fsub	dword ptr [eax + 0x4c]		// 0x000443ad
		fstp	dword ptr [esp + 0x64]		// 0x000443b0
		fxch	st(1)		// 0x000443b4
		fmul	dword ptr kConvexMeshHalf		// 0x000443b6
		fstp	dword ptr [esp + 0x10]		// 0x000443bc
		mov	ecx, dword ptr [esp + 0x10]		// 0x000443c0
		mov	dword ptr [esp + 0x74], ecx		// 0x000443c4
		fmul	dword ptr kConvexMeshHalf		// 0x000443c8
		fstp	dword ptr [esp + 0x14]		// 0x000443ce
		mov	ecx, dword ptr [esp + 0x14]		// 0x000443d2
		fld	dword ptr [esp + 0x64]		// 0x000443d6
		mov	dword ptr [esp + 0x78], ecx		// 0x000443da
		fmul	dword ptr kConvexMeshHalf		// 0x000443de
		fstp	dword ptr [esp + 0x18]		// 0x000443e4
		mov	ecx, dword ptr [esp + 0x18]		// 0x000443e8
		fld	dword ptr [esp + 0x70]		// 0x000443ec
		mov	dword ptr [esp + 0x7c], ecx		// 0x000443f0
		fmul	dword ptr [esp + 0x40]		// 0x000443f4
		mov	ecx, dword ptr [esp + 0x30]		// 0x000443f8
		fld	dword ptr [esp + 0x6c]		// 0x000443fc
		mov	dword ptr [esp + 0xb4], ecx		// 0x00044400
		fmul	dword ptr [esp + 0x30]		// 0x00044407
		mov	ecx, dword ptr [esp + 0x34]		// 0x0004440b
		mov	dword ptr [esp + 0xb8], ecx		// 0x0004440f
		mov	ecx, dword ptr [esp + 0x3c]		// 0x00044416
		faddp	st(1), st		// 0x0004441a
		mov	dword ptr [esp + 0xbc], ecx		// 0x0004441c
		fld	st(2)		// 0x00044423
		mov	ecx, dword ptr [esp + 0x40]		// 0x00044425
		fmul	dword ptr [esp + 0x68]		// 0x00044429
		mov	dword ptr [esp + 0xc0], ecx		// 0x0004442d
		mov	ecx, dword ptr [esp + 0x44]		// 0x00044434
		mov	dword ptr [esp + 0xc4], ecx		// 0x00044438
		faddp	st(1), st		// 0x0004443f
		mov	ecx, 9		// 0x00044441
		fadd	dword ptr [esp + 0x50]		// 0x00044446
		fld	dword ptr [esp + 0x70]		// 0x0004444a
		fmul	dword ptr [esp + 0x44]		// 0x0004444e
		fld	dword ptr [esp + 0x6c]		// 0x00044452
		fmul	dword ptr [esp + 0x34]		// 0x00044456
		faddp	st(1), st		// 0x0004445a
		fld	st(2)		// 0x0004445c
		fmul	dword ptr [esp + 0x68]		// 0x0004445e
		faddp	st(1), st		// 0x00044462
		fadd	dword ptr [esp + 0x54]		// 0x00044464
		fld	dword ptr [esp + 0x70]		// 0x00044468
		fmul	dword ptr [esp + 0x3c]		// 0x0004446c
		fld	dword ptr [esp + 0x6c]		// 0x00044470
		fmul	st, st(6)		// 0x00044474
		faddp	st(1), st		// 0x00044476
		fld	dword ptr [esp + 0x68]		// 0x00044478
		fmul	st, st(7)		// 0x0004447c
		faddp	st(1), st		// 0x0004447e
		fadd	dword ptr [esp + 0x4c]		// 0x00044480
		fstp	dword ptr [esp + 0x68]		// 0x00044484
		fxch	st(1)		// 0x00044488
		fstp	dword ptr [esp + 0x6c]		// 0x0004448a
		fstp	dword ptr [esp + 0x70]		// 0x0004448e
		fxch	st(3)		// 0x00044492
		fstp	dword ptr [esp + 0xa4]		// 0x00044494
		fstp	dword ptr [esp + 0xa8]		// 0x0004449b
		fxch	st(1)		// 0x000444a2
		fstp	dword ptr [esp + 0xac]		// 0x000444a4
		fstp	dword ptr [esp + 0xb0]		// 0x000444ab
		rep movsd		// 0x000444b2
		pop	edi		// 0x000444b4
		pop	esi		// 0x000444b5
		je	L444bd		// 0x000444b6
		add	eax, 4		// 0x000444b8
		jmp	L444bf		// 0x000444bb
L444bd:
		xor	eax, eax		// 0x000444bd
L444bf:
		mov	ecx, dword ptr [esp + 0x110]		// 0x000444bf
		push	ecx		// 0x000444c6
		mov	ecx, dword ptr [esp + 0x110]		// 0x000444c7
		push	ecx		// 0x000444ce
		lea	ecx, [esp + 0x68]		// 0x000444cf
		push	ecx		// 0x000444d3
		push	edx		// 0x000444d4
		push	eax		// 0x000444d5
		lea	edx, [esp + 0xd4]		// 0x000444d6
		push	edx		// 0x000444dd
		lea	eax, [esp + 0x2c]		// 0x000444de
		push	eax		// 0x000444e2
		push	ebx		// 0x000444e3
		push	ebp		// 0x000444e4
		call	nxConvexMeshContact		// 0x000444e5
		add	esp, 0x24		// 0x000444ea
		pop	ebp		// 0x000444ed
		pop	ebx		// 0x000444ee
		add	esp, 0xf8		// 0x000444ef
		ret		// 0x000444f5
		}
	}

// phys_fn_001853 (0x00044500, 5 B)
// The entry 001876 calls: a jmp to 001851.
__declspec(naked) void __cdecl NxContactConvexMesh(const NxCollisionShape* /*convex*/,
	const NxCollisionShape* /*mesh*/, NxContactSink* /*sink*/, void* /*context*/)
	{
	__asm
		{
		jmp	nxContactConvexMeshEntry		// 0x00044500
		}
	}
