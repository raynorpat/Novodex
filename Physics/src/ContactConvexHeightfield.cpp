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

void nxConvexMeshProject();
bool nxConvexMeshAxis();
bool nxConvexMeshFaceAxesAll();
bool nxConvexMeshInterval();
bool nxConvexMeshTriangleAxis();
void nxConvexMeshEdgeDirections();

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
