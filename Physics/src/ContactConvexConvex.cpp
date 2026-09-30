/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Convex/convex contact generation: sub-unit L of
// units/convex-mesh-gap-contract.md (0x0003fd80..0x0004135a; the file name is
// the contract's choice), written by convex-mesh gap Task 2g from the Capstone
// listing. Its entry 001820 is what ContactMeshMesh.cpp's 001876 calls when
// both meshes are convex; 001818 is also called by box/mesh (001758).
//
// The rows work on two convex meshes through the TriangleMesh polygon
// interface (the table at mesh +0x04, TriangleMeshPolygons.cpp): slot 0 the
// centre, 2 the vertices, 3 the polygon count, 4 a polygon, 9 and 10 the
// supporting polygon and face, 11 the extent along an axis. The separating-
// axis search (001816) tries each hull's face normals (001809 through 001807,
// 001805 and 001803) and then the cross products of the two hulls' edge axes
// gathered by 001812 (through the box test 001810) into the two Containers at
// the pair's scratch record +0x4e0 and +0x4f0; 001818 then picks the two
// polygons along the axis and hands them to 001909 (ContactGeneration.cpp),
// which emits through 000875.
//
// Every row is the listing's instructions, naked (branch targets are labels
// named by their oracle RVA), because the float code keeps values on the x87
// stack across narrowing stores and most rows take register arguments with the
// caller cleaning the stack (001803, 001805, 001807, 001810, 001812), which no
// C++ declaration expresses; those are declared without parameters and are
// only called from the naked rows. They call: the polygon interface through
// the objects' tables; 001653 and 001661 (IceMeshTools.cpp); 001281 and 002266
// (ContactGeneration.cpp, NxShapeOwner and NxContinuousCdPair); the vendored
// Prunable::UpdateWorldAABB (004886, through an /alternatename alias of its
// decorated name); and the CRT's stack probe (005695, `_chkstk`).
//
// Established by these listings (the contract records them): the pair's
// context (the fourth matrix-A argument) is a scratch record: +0x04 the
// visited array's count, +0x08 the visited array, +0x14 the stamp (002249
// through 000505), +0x4e0 and +0x4f0 the two edge-axis Containers (001816,
// emptied at 0x00040987 / 0x0004099a); the mesh's +0xa8 is the support map
// slot 11 takes (kind C); 001820 passes the meshes' +0xa4 as 001818's seventh
// and eighth arguments, which 001818 never reads.
//
// x87: on the /arch:IA32 list; /EHs-c- with the other ICE-shaped files.

#include "ContactGeneration.h"
#include "ConvexHull.h"

extern "C" void nxConvexCallUpdateWorldAABB();		// 004886, Prunable::UpdateWorldAABB(AABB*)
#pragma comment(linker, "/alternatename:_nxConvexCallUpdateWorldAABB=?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z")
extern "C" void _chkstk();								// 005695, the stack probe

// .rdata 0x101041f0 (0.0f), 0x101041ec (1.0f), 0x101043cc (0.5f), and the
// double 1e-6 at 0x10107c58 (the unit's own constant, 001816).
static const float kConvexZero = 0.0f;
static const float kConvexOne = 1.0f;
static const float kConvexHalf = 0.5f;
static const double kConvexMicro = 1e-6;

// phys_fn_001803 (0x0003fd80, 152 B)
// One axis against both hulls (register arguments: ecx and esi the two
// interfaces, ebx the scratch record, edi the axis; five stack arguments --
// the two poses and maps and the depth out -- cleaned by the caller). Each
// hull's extent along the axis (slot 11). False when either interval lies
// wholly beyond the other (`test ah, 5; jnp`: a NaN bound separates);
// otherwise, when the depth pointer is given, the smaller of the two overlaps
// (maxA - minB, maxB - minA; the second kept unless the first is below it) is
// stored, and true.
__declspec(naked) bool nxConvexAxisOverlap()
	{
	__asm
		{
		mov	edx, dword ptr [esp + 0x10]		// 0x0003fd80
		mov	eax, dword ptr [ecx]		// 0x0003fd84
		sub	esp, 0x10		// 0x0003fd86
		push	ebp		// 0x0003fd89
		mov	ebp, dword ptr [esp + 0x20]		// 0x0003fd8a
		push	edx		// 0x0003fd8e
		mov	edx, dword ptr [esp + 0x1c]		// 0x0003fd8f
		push	edx		// 0x0003fd93
		push	edi		// 0x0003fd94
		lea	edx, [esp + 0x10]		// 0x0003fd95
		push	edx		// 0x0003fd99
		lea	edx, [esp + 0x20]		// 0x0003fd9a
		push	edx		// 0x0003fd9e
		push	ebx		// 0x0003fd9f
		call	dword ptr [eax + 0x2c]		// 0x0003fda0
		mov	ecx, dword ptr [esp + 0x28]		// 0x0003fda3
		mov	edx, dword ptr [esp + 0x1c]		// 0x0003fda7
		mov	eax, dword ptr [esi]		// 0x0003fdab
		push	ecx		// 0x0003fdad
		push	edx		// 0x0003fdae
		push	edi		// 0x0003fdaf
		lea	ecx, [esp + 0x18]		// 0x0003fdb0
		push	ecx		// 0x0003fdb4
		lea	edx, [esp + 0x18]		// 0x0003fdb5
		push	edx		// 0x0003fdb9
		push	ebx		// 0x0003fdba
		mov	ecx, esi		// 0x0003fdbb
		call	dword ptr [eax + 0x2c]		// 0x0003fdbd
		fld	dword ptr [esp + 4]		// 0x0003fdc0
		fcomp	dword ptr [esp + 8]		// 0x0003fdc4
		fnstsw	ax		// 0x0003fdc8
		test	ah, 5		// 0x0003fdca
		jnp	L3fe11		// 0x0003fdcd
		fld	dword ptr [esp + 0xc]		// 0x0003fdcf
		fcomp	dword ptr [esp + 0x10]		// 0x0003fdd3
		fnstsw	ax		// 0x0003fdd7
		test	ah, 5		// 0x0003fdd9
		jnp	L3fe11		// 0x0003fddc
		test	ebp, ebp		// 0x0003fdde
		je	L3fe0a		// 0x0003fde0
		fld	dword ptr [esp + 4]		// 0x0003fde2
		fsub	dword ptr [esp + 8]		// 0x0003fde6
		fld	dword ptr [esp + 0xc]		// 0x0003fdea
		fsub	dword ptr [esp + 0x10]		// 0x0003fdee
		fstp	dword ptr [esp + 0x10]		// 0x0003fdf2
		fcom	dword ptr [esp + 0x10]		// 0x0003fdf6
		fnstsw	ax		// 0x0003fdfa
		test	ah, 5		// 0x0003fdfc
		jnp	L3fe07		// 0x0003fdff
		fstp	st(0)		// 0x0003fe01
		fld	dword ptr [esp + 0x10]		// 0x0003fe03
L3fe07:
		fstp	dword ptr [ebp]		// 0x0003fe07
L3fe0a:
		mov	al, 1		// 0x0003fe0a
		pop	ebp		// 0x0003fe0c
		add	esp, 0x10		// 0x0003fe0d
		ret		// 0x0003fe10
L3fe11:
		xor	al, al		// 0x0003fe11
		pop	ebp		// 0x0003fe13
		add	esp, 0x10		// 0x0003fe14
		ret		// 0x0003fe17
		}
	}

// phys_fn_001805 (0x0003fe20, 117 B)
// One axis against one hull and a given interval (register arguments: ecx the
// interface, edx and esi as the callers pass them -- the depth out in esi;
// five stack arguments cleaned by the caller): slot 11's extent, the same two
// tests and the smaller overlap as 001803.
__declspec(naked) bool nxConvexAxisSeparation()
	{
	__asm
		{
		mov	eax, dword ptr [ecx]		// 0x0003fe20
		sub	esp, 8		// 0x0003fe22
		push	edx		// 0x0003fe25
		mov	edx, dword ptr [esp + 0x20]		// 0x0003fe26
		push	edx		// 0x0003fe2a
		mov	edx, dword ptr [esp + 0x18]		// 0x0003fe2b
		push	edx		// 0x0003fe2f
		lea	edx, [esp + 0x10]		// 0x0003fe30
		push	edx		// 0x0003fe34
		lea	edx, [esp + 0x10]		// 0x0003fe35
		push	edx		// 0x0003fe39
		mov	edx, dword ptr [esp + 0x20]		// 0x0003fe3a
		push	edx		// 0x0003fe3e
		call	dword ptr [eax + 0x2c]		// 0x0003fe3f
		fld	dword ptr [esp + 0x18]		// 0x0003fe42
		fcomp	dword ptr [esp]		// 0x0003fe46
		fnstsw	ax		// 0x0003fe49
		test	ah, 5		// 0x0003fe4b
		jnp	L3fe8f		// 0x0003fe4e
		fld	dword ptr [esp + 4]		// 0x0003fe50
		fcomp	dword ptr [esp + 0x14]		// 0x0003fe54
		fnstsw	ax		// 0x0003fe58
		test	ah, 5		// 0x0003fe5a
		jnp	L3fe8f		// 0x0003fe5d
		test	esi, esi		// 0x0003fe5f
		je	L3fe89		// 0x0003fe61
		fld	dword ptr [esp + 0x18]		// 0x0003fe63
		fsub	dword ptr [esp]		// 0x0003fe67
		fld	dword ptr [esp + 4]		// 0x0003fe6a
		fsub	dword ptr [esp + 0x14]		// 0x0003fe6e
		fstp	dword ptr [esp + 0x18]		// 0x0003fe72
		fcom	dword ptr [esp + 0x18]		// 0x0003fe76
		fnstsw	ax		// 0x0003fe7a
		test	ah, 5		// 0x0003fe7c
		jnp	L3fe87		// 0x0003fe7f
		fstp	st(0)		// 0x0003fe81
		fld	dword ptr [esp + 0x18]		// 0x0003fe83
L3fe87:
		fstp	dword ptr [esi]		// 0x0003fe87
L3fe89:
		mov	al, 1		// 0x0003fe89
		add	esp, 8		// 0x0003fe8b
		ret		// 0x0003fe8e
L3fe8f:
		xor	al, al		// 0x0003fe8f
		add	esp, 8		// 0x0003fe91
		ret		// 0x0003fe94
		}
	}

// phys_fn_001807 (0x0003fea0, 301 B)
// Every polygon normal of one hull, rotated by its pose, as an axis for 001803
// (register arguments eax and ecx, ten stack arguments cleaned by the caller):
// false as soon as one separates; the smallest depth (`test ah, 5; jp`: equal
// and NaN do not replace it), its axis and index kept through the out
// arguments; true when there is no polygon or none separates.
__declspec(naked) bool nxConvexFaceAxes()
	{
	__asm
		{
		sub	esp, 0x18		// 0x0003fea0
		push	ebx		// 0x0003fea3
		push	ebp		// 0x0003fea4
		mov	ebp, dword ptr [esp + 0x28]		// 0x0003fea5
		push	esi		// 0x0003fea9
		push	edi		// 0x0003feaa
		mov	edi, dword ptr [esp + 0x2c]		// 0x0003feab
		mov	esi, eax		// 0x0003feaf
		mov	eax, dword ptr [edi]		// 0x0003feb1
		mov	ebx, ecx		// 0x0003feb3
		mov	ecx, edi		// 0x0003feb5
		call	dword ptr [eax + 0xc]		// 0x0003feb7
		test	eax, eax		// 0x0003feba
		mov	dword ptr [esp + 0x18], eax		// 0x0003febc
		mov	dword ptr [esp + 0x10], 0		// 0x0003fec0
		ja	L3fed8		// 0x0003fec8
		pop	edi		// 0x0003feca
		pop	esi		// 0x0003fecb
		pop	ebp		// 0x0003fecc
		mov	al, 1		// 0x0003fecd
		pop	ebx		// 0x0003fecf
		add	esp, 0x18		// 0x0003fed0
		ret		// 0x0003fed3
L3fed4:
		mov	edi, dword ptr [esp + 0x2c]		// 0x0003fed4
L3fed8:
		mov	eax, dword ptr [esp + 0x10]		// 0x0003fed8
		mov	edx, dword ptr [edi]		// 0x0003fedc
		push	eax		// 0x0003fede
		mov	ecx, edi		// 0x0003fedf
		call	dword ptr [edx + 0x10]		// 0x0003fee1
		fld	dword ptr [ebp + 0x20]		// 0x0003fee4
		fmul	dword ptr [eax + 0x14]		// 0x0003fee7
		add	eax, 0xc		// 0x0003feea
		fld	dword ptr [ebp + 0x10]		// 0x0003feed
		mov	ecx, dword ptr [esp + 0x3c]		// 0x0003fef0
		fmul	dword ptr [eax + 4]		// 0x0003fef4
		mov	edx, dword ptr [esp + 0x34]		// 0x0003fef7
		push	ecx		// 0x0003fefb
		mov	ecx, dword ptr [esp + 0x3c]		// 0x0003fefc
		faddp	st(1), st		// 0x0003ff00
		push	edx		// 0x0003ff02
		fld	dword ptr [eax]		// 0x0003ff03
		fmul	dword ptr [ebp]		// 0x0003ff05
		faddp	st(1), st		// 0x0003ff08
		fstp	dword ptr [esp + 0x24]		// 0x0003ff0a
		fld	dword ptr [ebp + 0x24]		// 0x0003ff0e
		fmul	dword ptr [eax + 8]		// 0x0003ff11
		fld	dword ptr [eax]		// 0x0003ff14
		fmul	dword ptr [ebp + 4]		// 0x0003ff16
		faddp	st(1), st		// 0x0003ff19
		fld	dword ptr [ebp + 0x14]		// 0x0003ff1b
		fmul	dword ptr [eax + 4]		// 0x0003ff1e
		faddp	st(1), st		// 0x0003ff21
		fstp	dword ptr [esp + 0x28]		// 0x0003ff23
		fld	dword ptr [eax + 4]		// 0x0003ff27
		fmul	dword ptr [ebp + 0x18]		// 0x0003ff2a
		fld	dword ptr [ebp + 8]		// 0x0003ff2d
		fmul	dword ptr [eax]		// 0x0003ff30
		faddp	st(1), st		// 0x0003ff32
		fld	dword ptr [eax + 8]		// 0x0003ff34
		lea	eax, [esp + 0x1c]		// 0x0003ff37
		fmul	dword ptr [ebp + 0x28]		// 0x0003ff3b
		push	eax		// 0x0003ff3e
		push	ecx		// 0x0003ff3f
		mov	ecx, edi		// 0x0003ff40
		faddp	st(1), st		// 0x0003ff42
		push	ebp		// 0x0003ff44
		lea	edi, [esp + 0x30]		// 0x0003ff45
		fstp	dword ptr [esp + 0x38]		// 0x0003ff49
		call	nxConvexAxisOverlap		// 0x0003ff4d
		add	esp, 0x14		// 0x0003ff52
		test	al, al		// 0x0003ff55
		je	L3ffc3		// 0x0003ff57
		fld	dword ptr [esp + 0x14]		// 0x0003ff59
		mov	ecx, dword ptr [esp + 0x40]		// 0x0003ff5d
		fcomp	dword ptr [ecx]		// 0x0003ff61
		fnstsw	ax		// 0x0003ff63
		test	ah, 5		// 0x0003ff65
		jp	L3ffa4		// 0x0003ff68
		mov	edx, dword ptr [esp + 0x14]		// 0x0003ff6a
		mov	eax, dword ptr [esp + 0x44]		// 0x0003ff6e
		mov	dword ptr [ecx], edx		// 0x0003ff72
		mov	ecx, dword ptr [esp + 0x1c]		// 0x0003ff74
		mov	edx, dword ptr [esp + 0x20]		// 0x0003ff78
		mov	dword ptr [eax], ecx		// 0x0003ff7c
		mov	ecx, dword ptr [esp + 0x24]		// 0x0003ff7e
		mov	dword ptr [eax + 4], edx		// 0x0003ff82
		mov	dword ptr [eax + 8], ecx		// 0x0003ff85
		mov	eax, dword ptr [esp + 0x4c]		// 0x0003ff88
		test	eax, eax		// 0x0003ff8c
		je	L3ff96		// 0x0003ff8e
		mov	edx, dword ptr [esp + 0x48]		// 0x0003ff90
		mov	dword ptr [eax], edx		// 0x0003ff94
L3ff96:
		mov	eax, dword ptr [esp + 0x50]		// 0x0003ff96
		test	eax, eax		// 0x0003ff9a
		je	L3ffa4		// 0x0003ff9c
		mov	ecx, dword ptr [esp + 0x10]		// 0x0003ff9e
		mov	dword ptr [eax], ecx		// 0x0003ffa2
L3ffa4:
		mov	eax, dword ptr [esp + 0x10]		// 0x0003ffa4
		mov	ecx, dword ptr [esp + 0x18]		// 0x0003ffa8
		inc	eax		// 0x0003ffac
		cmp	eax, ecx		// 0x0003ffad
		mov	dword ptr [esp + 0x10], eax		// 0x0003ffaf
		jb	L3fed4		// 0x0003ffb3
		pop	edi		// 0x0003ffb9
		pop	esi		// 0x0003ffba
		pop	ebp		// 0x0003ffbb
		mov	al, 1		// 0x0003ffbc
		pop	ebx		// 0x0003ffbe
		add	esp, 0x18		// 0x0003ffbf
		ret		// 0x0003ffc2
L3ffc3:
		pop	edi		// 0x0003ffc3
		pop	esi		// 0x0003ffc4
		pop	ebp		// 0x0003ffc5
		xor	al, al		// 0x0003ffc6
		pop	ebx		// 0x0003ffc8
		add	esp, 0x18		// 0x0003ffc9
		ret		// 0x0003ffcc
		}
	}

// phys_fn_001809 (0x0003ffd0, 432 B)
// The face-normal pass with the separating-distance bookkeeping (cdecl, 13
// arguments): the best index starts at -1; each polygon whose plane has the
// other hull's centre on its positive side (`test ah, 5; jnp`) is recorded and
// tested through 001805; when no polygon qualified, 001807 tests them all and
// the indices 0..n-1 are written out. False as soon as an axis separates.
__declspec(naked) bool nxConvexFaceAxesFirst()
	{
	__asm
		{
		sub	esp, 0x18		// 0x0003ffd0
		mov	eax, dword ptr [esp + 0x40]		// 0x0003ffd3
		push	ebp		// 0x0003ffd7
		push	esi		// 0x0003ffd8
		mov	esi, dword ptr [esp + 0x4c]		// 0x0003ffd9
		push	edi		// 0x0003ffdd
		mov	edi, dword ptr [esp + 0x30]		// 0x0003ffde
		mov	dword ptr [eax], 0xffffffff		// 0x0003ffe2
		mov	edx, dword ptr [edi]		// 0x0003ffe8
		mov	ecx, edi		// 0x0003ffea
		call	dword ptr [edx + 0xc]		// 0x0003ffec
		xor	ebp, ebp		// 0x0003ffef
		test	eax, eax		// 0x0003fff1
		mov	dword ptr [esp + 0x14], eax		// 0x0003fff3
		jbe	L40105		// 0x0003fff7
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x0003fffd lea ecx, [ecx]
L40000:
		mov	eax, dword ptr [edi]		// 0x00040000
		push	ebp		// 0x00040002
		mov	ecx, edi		// 0x00040003
		call	dword ptr [eax + 0x10]		// 0x00040005
		mov	ecx, eax		// 0x00040008
		mov	eax, dword ptr [esp + 0x2c]		// 0x0004000a
		fld	dword ptr [ecx + 0x14]		// 0x0004000e
		fmul	dword ptr [eax + 8]		// 0x00040011
		lea	edi, [ecx + 0xc]		// 0x00040014
		fld	dword ptr [eax + 4]		// 0x00040017
		fmul	dword ptr [edi + 4]		// 0x0004001a
		faddp	st(1), st		// 0x0004001d
		fld	dword ptr [eax]		// 0x0004001f
		fmul	dword ptr [edi]		// 0x00040021
		faddp	st(1), st		// 0x00040023
		fadd	dword ptr [edi + 0xc]		// 0x00040025
		fcomp	dword ptr kConvexZero		// 0x00040028
		fnstsw	ax		// 0x0004002e
		test	ah, 5		// 0x00040030
		jnp	L400f4		// 0x00040033
		mov	edx, dword ptr [esp + 0x58]		// 0x00040039
		push	edx		// 0x0004003d
		mov	edx, dword ptr [esp + 0x2c]		// 0x0004003e
		mov	dword ptr [esi], ebp		// 0x00040042
		mov	eax, dword ptr [ecx + 0x20]		// 0x00040044
		mov	ecx, dword ptr [ecx + 0x1c]		// 0x00040047
		push	eax		// 0x0004004a
		push	ecx		// 0x0004004b
		mov	ecx, dword ptr [esp + 0x44]		// 0x0004004c
		add	esi, 4		// 0x00040050
		push	edi		// 0x00040053
		push	edx		// 0x00040054
		mov	edx, dword ptr [esp + 0x54]		// 0x00040055
		mov	dword ptr [esp + 0x24], esi		// 0x00040059
		lea	esi, [esp + 0x20]		// 0x0004005d
		call	nxConvexAxisSeparation		// 0x00040061
		add	esp, 0x14		// 0x00040066
		test	al, al		// 0x00040069
		je	L40151		// 0x0004006b
		fld	dword ptr [esp + 0xc]		// 0x00040071
		mov	ecx, dword ptr [esp + 0x44]		// 0x00040075
		fcomp	dword ptr [ecx]		// 0x00040079
		fnstsw	ax		// 0x0004007b
		test	ah, 5		// 0x0004007d
		jp	L400f0		// 0x00040080
		fld	dword ptr [ebx + 0x20]		// 0x00040082
		mov	eax, dword ptr [esp + 0xc]		// 0x00040085
		fmul	dword ptr [edi + 8]		// 0x00040089
		fld	dword ptr [ebx + 0x10]		// 0x0004008c
		fmul	dword ptr [edi + 4]		// 0x0004008f
		faddp	st(1), st		// 0x00040092
		fld	dword ptr [edi]		// 0x00040094
		fmul	dword ptr [ebx]		// 0x00040096
		faddp	st(1), st		// 0x00040098
		fstp	dword ptr [esp + 0x18]		// 0x0004009a
		mov	edx, dword ptr [esp + 0x18]		// 0x0004009e
		fld	dword ptr [ebx + 4]		// 0x000400a2
		fmul	dword ptr [edi]		// 0x000400a5
		fld	dword ptr [ebx + 0x24]		// 0x000400a7
		fmul	dword ptr [edi + 8]		// 0x000400aa
		faddp	st(1), st		// 0x000400ad
		fld	dword ptr [ebx + 0x14]		// 0x000400af
		fmul	dword ptr [edi + 4]		// 0x000400b2
		faddp	st(1), st		// 0x000400b5
		fstp	dword ptr [esp + 0x1c]		// 0x000400b7
		fld	dword ptr [ebx + 8]		// 0x000400bb
		fmul	dword ptr [edi]		// 0x000400be
		fld	dword ptr [ebx + 0x28]		// 0x000400c0
		fmul	dword ptr [edi + 8]		// 0x000400c3
		faddp	st(1), st		// 0x000400c6
		fld	dword ptr [ebx + 0x18]		// 0x000400c8
		fmul	dword ptr [edi + 4]		// 0x000400cb
		mov	dword ptr [ecx], eax		// 0x000400ce
		mov	ecx, dword ptr [esp + 0x48]		// 0x000400d0
		mov	eax, dword ptr [esp + 0x1c]		// 0x000400d4
		faddp	st(1), st		// 0x000400d8
		mov	dword ptr [ecx], edx		// 0x000400da
		mov	dword ptr [ecx + 4], eax		// 0x000400dc
		mov	eax, dword ptr [esp + 0x4c]		// 0x000400df
		fstp	dword ptr [esp + 0x20]		// 0x000400e3
		mov	edx, dword ptr [esp + 0x20]		// 0x000400e7
		mov	dword ptr [ecx + 8], edx		// 0x000400eb
		mov	dword ptr [eax], ebp		// 0x000400ee
L400f0:
		mov	esi, dword ptr [esp + 0x10]		// 0x000400f0
L400f4:
		mov	eax, dword ptr [esp + 0x14]		// 0x000400f4
		mov	edi, dword ptr [esp + 0x30]		// 0x000400f8
		inc	ebp		// 0x000400fc
		cmp	ebp, eax		// 0x000400fd
		jb	L40000		// 0x000400ff
L40105:
		mov	ebp, dword ptr [esp + 0x50]		// 0x00040105
		mov	ecx, dword ptr [esp + 0x54]		// 0x00040109
		mov	eax, dword ptr [esp + 0x4c]		// 0x0004010d
		sub	esi, ebp		// 0x00040111
		sar	esi, 2		// 0x00040113
		mov	dword ptr [ecx], esi		// 0x00040116
		cmp	dword ptr [eax], -1		// 0x00040118
		jne	L40177		// 0x0004011b
		mov	edx, dword ptr [esp + 0x48]		// 0x0004011d
		mov	ecx, dword ptr [esp + 0x40]		// 0x00040121
		push	eax		// 0x00040125
		mov	eax, dword ptr [esp + 0x48]		// 0x00040126
		push	0		// 0x0004012a
		push	0		// 0x0004012c
		push	edx		// 0x0004012e
		mov	edx, dword ptr [esp + 0x4c]		// 0x0004012f
		push	eax		// 0x00040133
		mov	eax, dword ptr [esp + 0x48]		// 0x00040134
		push	ecx		// 0x00040138
		mov	ecx, dword ptr [esp + 0x40]		// 0x00040139
		push	edx		// 0x0004013d
		push	eax		// 0x0004013e
		mov	eax, dword ptr [esp + 0x58]		// 0x0004013f
		push	ebx		// 0x00040143
		push	edi		// 0x00040144
		call	nxConvexFaceAxes		// 0x00040145
		add	esp, 0x28		// 0x0004014a
		test	al, al		// 0x0004014d
		jne	L4015a		// 0x0004014f
L40151:
		pop	edi		// 0x00040151
		pop	esi		// 0x00040152
		xor	al, al		// 0x00040153
		pop	ebp		// 0x00040155
		add	esp, 0x18		// 0x00040156
		ret		// 0x00040159
L4015a:
		test	ebp, ebp		// 0x0004015a
		je	L40177		// 0x0004015c
		mov	ecx, dword ptr [esp + 0x14]		// 0x0004015e
		xor	eax, eax		// 0x00040162
		test	ecx, ecx		// 0x00040164
		jbe	L40171		// 0x00040166
L40168:
		mov	dword ptr [ebp + eax*4], eax		// 0x00040168
		inc	eax		// 0x0004016c
		cmp	eax, ecx		// 0x0004016d
		jb	L40168		// 0x0004016f
L40171:
		mov	edx, dword ptr [esp + 0x54]		// 0x00040171
		mov	dword ptr [edx], ecx		// 0x00040175
L40177:
		pop	edi		// 0x00040177
		pop	esi		// 0x00040178
		mov	al, 1		// 0x00040179
		pop	ebp		// 0x0004017b
		add	esp, 0x18		// 0x0004017c
		ret		// 0x0004017f
		}
	}

// phys_fn_001810 (0x00040180, 611 B)
// A box against an edge in the box's frame (register arguments edx, ecx, esi,
// edi, ebx; one stack argument cleaned by the caller): the edge's centre and
// half extent against the box's half extents on the three axes and the three
// cross-product axes, each `fabs` sum compared (`test ah, 5; jnp`: beyond, or
// NaN, returns false).
__declspec(naked) bool nxConvexEdgeBoxOverlap()
	{
	__asm
		{
		sub	esp, 0x34		// 0x00040180
		fld	dword ptr [edx]		// 0x00040183
		push	esi		// 0x00040185
		mov	esi, dword ptr [esp + 0x3c]		// 0x00040186
		fsub	dword ptr [esi]		// 0x0004018a
		fstp	dword ptr [esp + 0x3c]		// 0x0004018c
		fld	dword ptr [edx + 4]		// 0x00040190
		fsub	dword ptr [esi + 4]		// 0x00040193
		fmul	dword ptr [ecx + 0x10]		// 0x00040196
		fld	dword ptr [edx + 8]		// 0x00040199
		fsub	dword ptr [esi + 8]		// 0x0004019c
		fmul	dword ptr [ecx + 0x20]		// 0x0004019f
		faddp	st(1), st		// 0x000401a2
		fld	dword ptr [esp + 0x3c]		// 0x000401a4
		fmul	dword ptr [ecx]		// 0x000401a8
		faddp	st(1), st		// 0x000401aa
		fmul	dword ptr kConvexHalf		// 0x000401ac
		fstp	dword ptr [esp + 8]		// 0x000401b2
		fld	dword ptr [edi]		// 0x000401b6
		fsub	dword ptr [ebx]		// 0x000401b8
		fmul	dword ptr kConvexHalf		// 0x000401ba
		fstp	dword ptr [esp + 0x2c]		// 0x000401c0
		fld	dword ptr [esi]		// 0x000401c4
		fadd	dword ptr [edx]		// 0x000401c6
		fstp	dword ptr [esp + 4]		// 0x000401c8
		fld	dword ptr [esi + 4]		// 0x000401cc
		fadd	dword ptr [edx + 4]		// 0x000401cf
		fmul	dword ptr [ecx + 0x10]		// 0x000401d2
		fld	dword ptr [esi + 8]		// 0x000401d5
		fadd	dword ptr [edx + 8]		// 0x000401d8
		fmul	dword ptr [ecx + 0x20]		// 0x000401db
		faddp	st(1), st		// 0x000401de
		fld	dword ptr [esp + 4]		// 0x000401e0
		fmul	dword ptr [ecx]		// 0x000401e4
		faddp	st(1), st		// 0x000401e6
		fld	dword ptr [ecx + 0x30]		// 0x000401e8
		fadd	st(0), st(0)		// 0x000401eb
		faddp	st(1), st		// 0x000401ed
		fld	dword ptr [ebx]		// 0x000401ef
		fadd	dword ptr [edi]		// 0x000401f1
		fsubp	st(1), st		// 0x000401f3
		fmul	dword ptr kConvexHalf		// 0x000401f5
		fstp	dword ptr [esp + 0x14]		// 0x000401fb
		fld	dword ptr [esp + 8]		// 0x000401ff
		fabs		// 0x00040203
		fstp	dword ptr [esp + 0x20]		// 0x00040205
		fld	dword ptr [esp + 0x14]		// 0x00040209
		fabs		// 0x0004020d
		fld	dword ptr [esp + 0x20]		// 0x0004020f
		fadd	dword ptr [esp + 0x2c]		// 0x00040213
		fcompp		// 0x00040217
		fnstsw	ax		// 0x00040219
		test	ah, 5		// 0x0004021b
		jnp	L403d5		// 0x0004021e
		fld	dword ptr [edx + 4]		// 0x00040224
		fsub	dword ptr [esi + 4]		// 0x00040227
		fmul	dword ptr [ecx + 0x14]		// 0x0004022a
		fld	dword ptr [edx + 8]		// 0x0004022d
		fsub	dword ptr [esi + 8]		// 0x00040230
		fmul	dword ptr [ecx + 0x24]		// 0x00040233
		faddp	st(1), st		// 0x00040236
		fld	dword ptr [esp + 0x3c]		// 0x00040238
		fmul	dword ptr [ecx + 4]		// 0x0004023c
		faddp	st(1), st		// 0x0004023f
		fmul	dword ptr kConvexHalf		// 0x00040241
		fstp	dword ptr [esp + 0xc]		// 0x00040247
		fld	dword ptr [edi + 4]		// 0x0004024b
		fsub	dword ptr [ebx + 4]		// 0x0004024e
		fmul	dword ptr kConvexHalf		// 0x00040251
		fstp	dword ptr [esp + 0x30]		// 0x00040257
		fld	dword ptr [esi + 4]		// 0x0004025b
		fadd	dword ptr [edx + 4]		// 0x0004025e
		fmul	dword ptr [ecx + 0x14]		// 0x00040261
		fld	dword ptr [esi + 8]		// 0x00040264
		fadd	dword ptr [edx + 8]		// 0x00040267
		fmul	dword ptr [ecx + 0x24]		// 0x0004026a
		faddp	st(1), st		// 0x0004026d
		fld	dword ptr [esp + 4]		// 0x0004026f
		fmul	dword ptr [ecx + 4]		// 0x00040273
		faddp	st(1), st		// 0x00040276
		fld	dword ptr [ecx + 0x34]		// 0x00040278
		fadd	st(0), st(0)		// 0x0004027b
		faddp	st(1), st		// 0x0004027d
		fld	dword ptr [edi + 4]		// 0x0004027f
		fadd	dword ptr [ebx + 4]		// 0x00040282
		fsubp	st(1), st		// 0x00040285
		fmul	dword ptr kConvexHalf		// 0x00040287
		fstp	dword ptr [esp + 0x18]		// 0x0004028d
		fld	dword ptr [esp + 0xc]		// 0x00040291
		fabs		// 0x00040295
		fstp	dword ptr [esp + 0x24]		// 0x00040297
		fld	dword ptr [esp + 0x18]		// 0x0004029b
		fabs		// 0x0004029f
		fld	dword ptr [esp + 0x24]		// 0x000402a1
		fadd	dword ptr [esp + 0x30]		// 0x000402a5
		fcompp		// 0x000402a9
		fnstsw	ax		// 0x000402ab
		test	ah, 5		// 0x000402ad
		jnp	L403d5		// 0x000402b0
		fld	dword ptr [edx + 4]		// 0x000402b6
		fsub	dword ptr [esi + 4]		// 0x000402b9
		fmul	dword ptr [ecx + 0x18]		// 0x000402bc
		fld	dword ptr [edx + 8]		// 0x000402bf
		fsub	dword ptr [esi + 8]		// 0x000402c2
		fmul	dword ptr [ecx + 0x28]		// 0x000402c5
		faddp	st(1), st		// 0x000402c8
		fld	dword ptr [esp + 0x3c]		// 0x000402ca
		fmul	dword ptr [ecx + 8]		// 0x000402ce
		faddp	st(1), st		// 0x000402d1
		fmul	dword ptr kConvexHalf		// 0x000402d3
		fstp	dword ptr [esp + 0x10]		// 0x000402d9
		fld	dword ptr [edi + 8]		// 0x000402dd
		fsub	dword ptr [ebx + 8]		// 0x000402e0
		fmul	dword ptr kConvexHalf		// 0x000402e3
		fstp	dword ptr [esp + 0x34]		// 0x000402e9
		fld	dword ptr [esi + 8]		// 0x000402ed
		fadd	dword ptr [edx + 8]		// 0x000402f0
		fmul	dword ptr [ecx + 0x28]		// 0x000402f3
		fld	dword ptr [esi + 4]		// 0x000402f6
		fadd	dword ptr [edx + 4]		// 0x000402f9
		fmul	dword ptr [ecx + 0x18]		// 0x000402fc
		faddp	st(1), st		// 0x000402ff
		fld	dword ptr [esp + 4]		// 0x00040301
		fmul	dword ptr [ecx + 8]		// 0x00040305
		faddp	st(1), st		// 0x00040308
		fld	dword ptr [ecx + 0x38]		// 0x0004030a
		fadd	st(0), st(0)		// 0x0004030d
		faddp	st(1), st		// 0x0004030f
		fld	dword ptr [ebx + 8]		// 0x00040311
		fadd	dword ptr [edi + 8]		// 0x00040314
		fsubp	st(1), st		// 0x00040317
		fmul	dword ptr kConvexHalf		// 0x00040319
		fstp	dword ptr [esp + 0x1c]		// 0x0004031f
		fld	dword ptr [esp + 0x10]		// 0x00040323
		fabs		// 0x00040327
		fstp	dword ptr [esp + 0x28]		// 0x00040329
		fld	dword ptr [esp + 0x1c]		// 0x0004032d
		fabs		// 0x00040331
		fld	dword ptr [esp + 0x28]		// 0x00040333
		fadd	dword ptr [esp + 0x34]		// 0x00040337
		fcompp		// 0x0004033b
		fnstsw	ax		// 0x0004033d
		test	ah, 5		// 0x0004033f
		jnp	L403d5		// 0x00040342
		fld	dword ptr [esp + 0x1c]		// 0x00040348
		fmul	dword ptr [esp + 0xc]		// 0x0004034c
		fld	dword ptr [esp + 0x10]		// 0x00040350
		fmul	dword ptr [esp + 0x18]		// 0x00040354
		fsubp	st(1), st		// 0x00040358
		fabs		// 0x0004035a
		fld	dword ptr [esp + 0x34]		// 0x0004035c
		fmul	dword ptr [esp + 0x24]		// 0x00040360
		fld	dword ptr [esp + 0x28]		// 0x00040364
		fmul	dword ptr [esp + 0x30]		// 0x00040368
		faddp	st(1), st		// 0x0004036c
		fcompp		// 0x0004036e
		fnstsw	ax		// 0x00040370
		test	ah, 5		// 0x00040372
		jnp	L403d5		// 0x00040375
		fld	dword ptr [esp + 0x10]		// 0x00040377
		fmul	dword ptr [esp + 0x14]		// 0x0004037b
		fld	dword ptr [esp + 0x1c]		// 0x0004037f
		fmul	dword ptr [esp + 8]		// 0x00040383
		fsubp	st(1), st		// 0x00040387
		fabs		// 0x00040389
		fld	dword ptr [esp + 0x34]		// 0x0004038b
		fmul	dword ptr [esp + 0x20]		// 0x0004038f
		fld	dword ptr [esp + 0x28]		// 0x00040393
		fmul	dword ptr [esp + 0x2c]		// 0x00040397
		faddp	st(1), st		// 0x0004039b
		fcompp		// 0x0004039d
		fnstsw	ax		// 0x0004039f
		test	ah, 5		// 0x000403a1
		jnp	L403d5		// 0x000403a4
		fld	dword ptr [esp + 0x18]		// 0x000403a6
		fmul	dword ptr [esp + 8]		// 0x000403aa
		fld	dword ptr [esp + 0xc]		// 0x000403ae
		fmul	dword ptr [esp + 0x14]		// 0x000403b2
		fsubp	st(1), st		// 0x000403b6
		fabs		// 0x000403b8
		fld	dword ptr [esp + 0x30]		// 0x000403ba
		fmul	dword ptr [esp + 0x20]		// 0x000403be
		fld	dword ptr [esp + 0x24]		// 0x000403c2
		fmul	dword ptr [esp + 0x2c]		// 0x000403c6
		faddp	st(1), st		// 0x000403ca
		fcompp		// 0x000403cc
		fnstsw	ax		// 0x000403ce
		test	ah, 5		// 0x000403d0
		jp	L403dc		// 0x000403d3
L403d5:
		xor	al, al		// 0x000403d5
		pop	esi		// 0x000403d7
		add	esp, 0x34		// 0x000403d8
		ret		// 0x000403db
L403dc:
		mov	al, 1		// 0x000403dc
		pop	esi		// 0x000403de
		add	esp, 0x34		// 0x000403df
		ret		// 0x000403e2
		}
	}

// phys_fn_001812 (0x000403f0, 125 B)
// phys_fn_001814 (0x00040470, 381 B)
// The edge axes of a hull's polygons near the other (eax the polygon count,
// stack arguments cleaned by the caller; the row and its continuation): for
// each listed polygon, each outline edge (references ordered by `xor` swaps)
// with an end on the positive side of the given plane (`test ah, 0x41`) and
// passing 001810 against the other's box is taken into the pose's frame,
// normalised when its squared length is not 0.0f, and added to the Container
// through 001661.
__declspec(naked) void nxConvexGatherEdgeAxes()
	{
	__asm
		{
		sub	esp, 0x28		// 0x000403f0
		mov	ecx, dword ptr [esp + 0x34]		// 0x000403f3
		push	ebx		// 0x000403f7
		push	ebp		// 0x000403f8
		mov	ebp, dword ptr [esp + 0x40]		// 0x000403f9
		push	esi		// 0x000403fd
		push	edi		// 0x000403fe
		mov	edi, dword ptr [esp + 0x4c]		// 0x000403ff
		mov	esi, eax		// 0x00040403
		mov	eax, dword ptr [ecx]		// 0x00040405
		add	edi, 0xc		// 0x00040407
		call	dword ptr [eax + 8]		// 0x0004040a
		test	esi, esi		// 0x0004040d
		mov	ebx, eax		// 0x0004040f
		mov	dword ptr [esp + 0x18], ebx		// 0x00040411
		je	L405e5		// 0x00040415
		mov	dword ptr [esp + 0x20], esi		// 0x0004041b
		_emit	0x90		// 0x0004041f nop 
L40420:
		mov	esi, dword ptr [esp + 0x40]		// 0x00040420
		mov	eax, dword ptr [esi]		// 0x00040424
		mov	ecx, dword ptr [esp + 0x44]		// 0x00040426
		mov	edx, dword ptr [ecx]		// 0x0004042a
		push	eax		// 0x0004042c
		call	dword ptr [edx + 0x10]		// 0x0004042d
		mov	edx, dword ptr [eax + 4]		// 0x00040430
		mov	eax, dword ptr [eax]		// 0x00040433
		add	esi, 4		// 0x00040435
		test	eax, eax		// 0x00040438
		mov	dword ptr [esp + 0x40], esi		// 0x0004043a
		mov	dword ptr [esp + 0x24], edx		// 0x0004043e
		mov	dword ptr [esp + 0x28], eax		// 0x00040442
		jbe	L405db		// 0x00040446
		mov	ecx, 1		// 0x0004044c
		mov	dword ptr [esp + 0x10], ecx		// 0x00040451
		mov	dword ptr [esp + 0x14], edx		// 0x00040455
		mov	dword ptr [esp + 0x1c], eax		// 0x00040459
		jmp	L40470		// 0x0004045d
L4045f:
		mov	ecx, dword ptr [esp + 0x10]		// 0x0004045f
		mov	edx, dword ptr [esp + 0x24]		// 0x00040463
		mov	eax, dword ptr [esp + 0x28]		// 0x00040467
		jmp	L40470		// 0x0004046b
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x0004046d lea ecx, [ecx]
L40470:
		cmp	ecx, eax		// 0x00040470
		jb	L40476		// 0x00040472
		xor	ecx, ecx		// 0x00040474
L40476:
		mov	eax, dword ptr [esp + 0x14]		// 0x00040476
		mov	eax, dword ptr [eax]		// 0x0004047a
		mov	ecx, dword ptr [edx + ecx*4]		// 0x0004047c
		cmp	eax, ecx		// 0x0004047f
		jbe	L40489		// 0x00040481
		xor	eax, ecx		// 0x00040483
		xor	ecx, eax		// 0x00040485
		xor	eax, ecx		// 0x00040487
L40489:
		lea	edx, [eax + eax*2]		// 0x00040489
		fld	dword ptr [ebx + edx*4 + 8]		// 0x0004048c
		lea	esi, [ebx + edx*4]		// 0x00040490
		mov	edx, dword ptr [esp + 0x50]		// 0x00040493
		fmul	dword ptr [edx + 8]		// 0x00040497
		fld	dword ptr [esi + 4]		// 0x0004049a
		fmul	dword ptr [edx + 4]		// 0x0004049d
		faddp	st(1), st		// 0x000404a0
		fld	dword ptr [esi]		// 0x000404a2
		fmul	dword ptr [edx]		// 0x000404a4
		faddp	st(1), st		// 0x000404a6
		fadd	dword ptr [edx + 0xc]		// 0x000404a8
		fcomp	dword ptr kConvexZero		// 0x000404ab
		fnstsw	ax		// 0x000404b1
		test	ah, 0x41		// 0x000404b3
		jnp	L404e7		// 0x000404b6
		lea	eax, [ecx + ecx*2]		// 0x000404b8
		fld	dword ptr [ebx + eax*4 + 8]		// 0x000404bb
		lea	eax, [ebx + eax*4]		// 0x000404bf
		fmul	dword ptr [edx + 8]		// 0x000404c2
		fld	dword ptr [eax + 4]		// 0x000404c5
		fmul	dword ptr [edx + 4]		// 0x000404c8
		faddp	st(1), st		// 0x000404cb
		fld	dword ptr [eax]		// 0x000404cd
		fmul	dword ptr [edx]		// 0x000404cf
		faddp	st(1), st		// 0x000404d1
		fadd	dword ptr [edx + 0xc]		// 0x000404d3
		fcomp	dword ptr kConvexZero		// 0x000404d6
		fnstsw	ax		// 0x000404dc
		test	ah, 0x41		// 0x000404de
		jp	L405b8		// 0x000404e1
L404e7:
		lea	ecx, [ecx + ecx*2]		// 0x000404e7
		lea	edx, [ebx + ecx*4]		// 0x000404ea
		mov	ebx, dword ptr [esp + 0x4c]		// 0x000404ed
		push	esi		// 0x000404f1
		mov	ecx, ebp		// 0x000404f2
		call	nxConvexEdgeBoxOverlap		// 0x000404f4
		add	esp, 4		// 0x000404f9
		test	al, al		// 0x000404fc
		je	L405b4		// 0x000404fe
		fld	dword ptr [esi]		// 0x00040504
		fsub	dword ptr [edx]		// 0x00040506
		fld	dword ptr [esi + 4]		// 0x00040508
		fsub	dword ptr [edx + 4]		// 0x0004050b
		fld	dword ptr [esi + 8]		// 0x0004050e
		fsub	dword ptr [edx + 8]		// 0x00040511
		fld	st(0)		// 0x00040514
		fmul	dword ptr [ebp + 0x20]		// 0x00040516
		fld	st(2)		// 0x00040519
		fmul	dword ptr [ebp + 0x10]		// 0x0004051b
		faddp	st(1), st		// 0x0004051e
		fld	st(3)		// 0x00040520
		fmul	dword ptr [ebp]		// 0x00040522
		faddp	st(1), st		// 0x00040525
		fstp	dword ptr [esp + 0x2c]		// 0x00040527
		fld	st(0)		// 0x0004052b
		fmul	dword ptr [ebp + 0x24]		// 0x0004052d
		fld	st(3)		// 0x00040530
		fmul	dword ptr [ebp + 4]		// 0x00040532
		faddp	st(1), st		// 0x00040535
		fld	st(2)		// 0x00040537
		fmul	dword ptr [ebp + 0x14]		// 0x00040539
		faddp	st(1), st		// 0x0004053c
		fstp	dword ptr [esp + 0x30]		// 0x0004053e
		fmul	dword ptr [ebp + 0x28]		// 0x00040542
		fxch	st(2)		// 0x00040545
		fmul	dword ptr [ebp + 8]		// 0x00040547
		faddp	st(2), st		// 0x0004054a
		fmul	dword ptr [ebp + 0x18]		// 0x0004054c
		faddp	st(1), st		// 0x0004054f
		fst	dword ptr [esp + 0x34]		// 0x00040551
		fmul	dword ptr [esp + 0x34]		// 0x00040555
		fld	dword ptr [esp + 0x30]		// 0x00040559
		fmul	dword ptr [esp + 0x30]		// 0x0004055d
		faddp	st(1), st		// 0x00040561
		fld	dword ptr [esp + 0x2c]		// 0x00040563
		fmul	dword ptr [esp + 0x2c]		// 0x00040567
		faddp	st(1), st		// 0x0004056b
		fld	dword ptr kConvexZero		// 0x0004056d
		fld	st(1)		// 0x00040573
		fucompp		// 0x00040575
		fnstsw	ax		// 0x00040577
		test	ah, 0x44		// 0x00040579
		jnp	L405a4		// 0x0004057c
		fsqrt		// 0x0004057e
		fdivr	dword ptr kConvexOne		// 0x00040580
		fld	dword ptr [esp + 0x2c]		// 0x00040586
		fmul	st, st(1)		// 0x0004058a
		fstp	dword ptr [esp + 0x2c]		// 0x0004058c
		fld	dword ptr [esp + 0x30]		// 0x00040590
		fmul	st, st(1)		// 0x00040594
		fstp	dword ptr [esp + 0x30]		// 0x00040596
		fld	dword ptr [esp + 0x34]		// 0x0004059a
		fmul	st, st(1)		// 0x0004059e
		fstp	dword ptr [esp + 0x34]		// 0x000405a0
L405a4:
		mov	ecx, dword ptr [esp + 0x3c]		// 0x000405a4
		fstp	st(0)		// 0x000405a8
		lea	edx, [esp + 0x2c]		// 0x000405aa
		push	edx		// 0x000405ae
		call	nxIceAddUniqueAxis		// 0x000405af
L405b4:
		mov	ebx, dword ptr [esp + 0x18]		// 0x000405b4
L405b8:
		mov	edx, dword ptr [esp + 0x10]		// 0x000405b8
		mov	ecx, dword ptr [esp + 0x14]		// 0x000405bc
		mov	eax, dword ptr [esp + 0x1c]		// 0x000405c0
		inc	edx		// 0x000405c4
		add	ecx, 4		// 0x000405c5
		dec	eax		// 0x000405c8
		mov	dword ptr [esp + 0x10], edx		// 0x000405c9
		mov	dword ptr [esp + 0x14], ecx		// 0x000405cd
		mov	dword ptr [esp + 0x1c], eax		// 0x000405d1
		jne	L4045f		// 0x000405d5
L405db:
		dec	dword ptr [esp + 0x20]		// 0x000405db
		jne	L40420		// 0x000405df
L405e5:
		pop	edi		// 0x000405e5
		pop	esi		// 0x000405e6
		pop	ebp		// 0x000405e7
		pop	ebx		// 0x000405e8
		add	esp, 0x28		// 0x000405e9
		ret		// 0x000405ec
		}
	}

// phys_fn_001816 (0x000405f0, 1,490 B)
// The separating-axis search (cdecl, 16 arguments; an ebp frame): both hulls'
// centres in world space (slot 0 through each pose), two `_chkstk` blocks of
// four bytes a polygon (slot 3), the face pass for each hull (001809, the
// depth seeded with FLT_MAX), the best of the two, then both Containers of the
// scratch record emptied and filled with the edge axes (001812), and every
// cross product of an A axis with a B axis whose components are not all within
// 1e-6 (`fabs` against the double at 0x10107c58) normalised (when its squared
// length is not 0.0f) and tested through 001803; a smaller depth replaces the
// best with kind 2. The depth, axis and kind (0, 1, 2) go out; false as soon
// as any axis separates.
__declspec(naked) bool nxConvexSeparatingAxis()
	{
	__asm
		{
		push	ebp		// 0x000405f0
		lea	ebp, [esp - 0x38]		// 0x000405f1
		sub	esp, 0x9c		// 0x000405f5
		push	ebx		// 0x000405fb
		push	esi		// 0x000405fc
		push	edi		// 0x000405fd
		mov	edi, dword ptr [ebp + 0x54]		// 0x000405fe
		mov	eax, dword ptr [edi]		// 0x00040601
		mov	ecx, edi		// 0x00040603
		call	dword ptr [eax]		// 0x00040605
		mov	esi, dword ptr [ebp + 0x64]		// 0x00040607
		fld	dword ptr [esi]		// 0x0004060a
		mov	ecx, dword ptr [ebp + 0x58]		// 0x0004060c
		fmul	dword ptr [eax]		// 0x0004060f
		mov	edx, dword ptr [ecx]		// 0x00040611
		fld	dword ptr [esi + 0x10]		// 0x00040613
		fmul	dword ptr [eax + 4]		// 0x00040616
		faddp	st(1), st		// 0x00040619
		fld	dword ptr [esi + 0x20]		// 0x0004061b
		fmul	dword ptr [eax + 8]		// 0x0004061e
		faddp	st(1), st		// 0x00040621
		fadd	dword ptr [esi + 0x30]		// 0x00040623
		fstp	dword ptr [ebp + 0x2c]		// 0x00040626
		fld	dword ptr [esi + 0x24]		// 0x00040629
		fmul	dword ptr [eax + 8]		// 0x0004062c
		fld	dword ptr [esi + 4]		// 0x0004062f
		fmul	dword ptr [eax]		// 0x00040632
		faddp	st(1), st		// 0x00040634
		fld	dword ptr [esi + 0x14]		// 0x00040636
		fmul	dword ptr [eax + 4]		// 0x00040639
		faddp	st(1), st		// 0x0004063c
		fadd	dword ptr [esi + 0x34]		// 0x0004063e
		fstp	dword ptr [ebp + 0x30]		// 0x00040641
		fld	dword ptr [esi + 0x18]		// 0x00040644
		fmul	dword ptr [eax + 4]		// 0x00040647
		fld	dword ptr [eax + 8]		// 0x0004064a
		fmul	dword ptr [esi + 0x28]		// 0x0004064d
		faddp	st(1), st		// 0x00040650
		fld	dword ptr [esi + 8]		// 0x00040652
		fmul	dword ptr [eax]		// 0x00040655
		faddp	st(1), st		// 0x00040657
		fadd	dword ptr [esi + 0x38]		// 0x00040659
		fstp	dword ptr [ebp + 0x34]		// 0x0004065c
		call	dword ptr [edx]		// 0x0004065f
		mov	ebx, dword ptr [ebp + 0x68]		// 0x00040661
		fld	dword ptr [ebx + 0x10]		// 0x00040664
		mov	ecx, edi		// 0x00040667
		fmul	dword ptr [eax + 4]		// 0x00040669
		fld	dword ptr [ebx]		// 0x0004066c
		fmul	dword ptr [eax]		// 0x0004066e
		faddp	st(1), st		// 0x00040670
		fld	dword ptr [ebx + 0x20]		// 0x00040672
		fmul	dword ptr [eax + 8]		// 0x00040675
		faddp	st(1), st		// 0x00040678
		fadd	dword ptr [ebx + 0x30]		// 0x0004067a
		fstp	dword ptr [ebp + 0x10]		// 0x0004067d
		fld	dword ptr [ebx + 4]		// 0x00040680
		fmul	dword ptr [eax]		// 0x00040683
		fld	dword ptr [eax + 4]		// 0x00040685
		fmul	dword ptr [ebx + 0x14]		// 0x00040688
		faddp	st(1), st		// 0x0004068b
		fld	dword ptr [eax + 8]		// 0x0004068d
		fmul	dword ptr [ebx + 0x24]		// 0x00040690
		faddp	st(1), st		// 0x00040693
		fadd	dword ptr [ebx + 0x34]		// 0x00040695
		fstp	dword ptr [ebp + 0x14]		// 0x00040698
		fld	dword ptr [ebx + 8]		// 0x0004069b
		fmul	dword ptr [eax]		// 0x0004069e
		fld	dword ptr [eax + 8]		// 0x000406a0
		fmul	dword ptr [ebx + 0x28]		// 0x000406a3
		faddp	st(1), st		// 0x000406a6
		fld	dword ptr [ebx + 0x18]		// 0x000406a8
		fmul	dword ptr [eax + 4]		// 0x000406ab
		mov	eax, dword ptr [edi]		// 0x000406ae
		faddp	st(1), st		// 0x000406b0
		fadd	dword ptr [ebx + 0x38]		// 0x000406b2
		fstp	dword ptr [ebp + 0x18]		// 0x000406b5
		call	dword ptr [eax + 0xc]		// 0x000406b8
		shl	eax, 2		// 0x000406bb
		add	eax, 3		// 0x000406be
		and	eax, 0xfffffffc		// 0x000406c1
		call	_chkstk		// 0x000406c4
		mov	ecx, dword ptr [ebp + 0x58]		// 0x000406c9
		mov	dword ptr [ebp + 0x24], esp		// 0x000406cc
		mov	edx, dword ptr [ecx]		// 0x000406cf
		call	dword ptr [edx + 0xc]		// 0x000406d1
		shl	eax, 2		// 0x000406d4
		add	eax, 3		// 0x000406d7
		and	eax, 0xfffffffc		// 0x000406da
		call	_chkstk		// 0x000406dd
		mov	ecx, dword ptr [ebp + 0x24]		// 0x000406e2
		mov	edx, dword ptr [ebp + 0x44]		// 0x000406e5
		mov	dword ptr [ebp + 0x20], esp		// 0x000406e8
		push	ebx		// 0x000406eb
		mov	ebx, dword ptr [ebp + 0x5c]		// 0x000406ec
		lea	eax, [ebp + 4]		// 0x000406ef
		push	eax		// 0x000406f2
		push	ecx		// 0x000406f3
		push	edx		// 0x000406f4
		mov	edx, dword ptr [ebp + 0x70]		// 0x000406f5
		lea	eax, [ebp - 0x18]		// 0x000406f8
		push	eax		// 0x000406fb
		mov	eax, dword ptr [ebp + 0x60]		// 0x000406fc
		lea	ecx, [ebp + 0x1c]		// 0x000406ff
		push	ecx		// 0x00040702
		mov	ecx, dword ptr [ebp + 0x58]		// 0x00040703
		push	edx		// 0x00040706
		mov	edx, dword ptr [ebp + 0x6c]		// 0x00040707
		push	eax		// 0x0004070a
		push	ecx		// 0x0004070b
		mov	ecx, dword ptr [ebp + 0x40]		// 0x0004070c
		push	edx		// 0x0004070f
		push	edi		// 0x00040710
		lea	eax, [ebp + 0x10]		// 0x00040711
		push	eax		// 0x00040714
		push	ecx		// 0x00040715
		mov	dword ptr [ebp + 0x1c], 0x7f7fffff		// 0x00040716
		mov	dword ptr [ebp + 0x28], 0x7f7fffff		// 0x0004071d
		call	nxConvexFaceAxesFirst		// 0x00040724
		add	esp, 0x34		// 0x00040729
		test	al, al		// 0x0004072c
		je	L40bb3		// 0x0004072e
		mov	eax, dword ptr [ebp + 0x20]		// 0x00040734
		mov	ecx, dword ptr [ebp + 0x48]		// 0x00040737
		push	esi		// 0x0004073a
		lea	edx, [ebp + 8]		// 0x0004073b
		push	edx		// 0x0004073e
		push	eax		// 0x0004073f
		push	ecx		// 0x00040740
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00040741
		lea	edx, [ebp - 8]		// 0x00040744
		push	edx		// 0x00040747
		mov	edx, dword ptr [ebp + 0x70]		// 0x00040748
		lea	eax, [ebp + 0x28]		// 0x0004074b
		push	eax		// 0x0004074e
		mov	eax, dword ptr [ebp + 0x58]		// 0x0004074f
		push	ecx		// 0x00040752
		push	ebx		// 0x00040753
		mov	ebx, dword ptr [ebp + 0x60]		// 0x00040754
		push	edi		// 0x00040757
		push	edx		// 0x00040758
		mov	edx, dword ptr [ebp + 0x40]		// 0x00040759
		push	eax		// 0x0004075c
		lea	ecx, [ebp + 0x2c]		// 0x0004075d
		push	ecx		// 0x00040760
		push	edx		// 0x00040761
		call	nxConvexFaceAxesFirst		// 0x00040762
		add	esp, 0x34		// 0x00040767
		test	al, al		// 0x0004076a
		je	L40bb3		// 0x0004076c
		mov	ecx, dword ptr [ebp - 0x18]		// 0x00040772
		mov	eax, dword ptr [ebp + 0x1c]		// 0x00040775
		mov	edx, dword ptr [ebp - 0x14]		// 0x00040778
		mov	dword ptr [ebp + 0x10], ecx		// 0x0004077b
		mov	ecx, dword ptr [ebp + 0x7c]		// 0x0004077e
		test	ecx, ecx		// 0x00040781
		mov	dword ptr [ebp + 0xc], eax		// 0x00040783
		mov	eax, dword ptr [ebp - 0x10]		// 0x00040786
		mov	dword ptr [ebp + 0x14], edx		// 0x00040789
		mov	dword ptr [ebp + 0x18], eax		// 0x0004078c
		je	L40797		// 0x0004078f
		mov	dword ptr [ecx], 0		// 0x00040791
L40797:
		fld	dword ptr [ebp + 0x28]		// 0x00040797
		fcomp	dword ptr [ebp + 0x1c]		// 0x0004079a
		fnstsw	ax		// 0x0004079d
		test	ah, 5		// 0x0004079f
		jp	L407c6		// 0x000407a2
		test	ecx, ecx		// 0x000407a4
		mov	edx, dword ptr [ebp + 0x28]		// 0x000407a6
		mov	eax, dword ptr [ebp - 8]		// 0x000407a9
		mov	dword ptr [ebp + 0xc], edx		// 0x000407ac
		mov	edx, dword ptr [ebp - 4]		// 0x000407af
		mov	dword ptr [ebp + 0x10], eax		// 0x000407b2
		mov	eax, dword ptr [ebp]		// 0x000407b5
		mov	dword ptr [ebp + 0x14], edx		// 0x000407b8
		mov	dword ptr [ebp + 0x18], eax		// 0x000407bb
		je	L407c6		// 0x000407be
		mov	dword ptr [ecx], 1		// 0x000407c0
L407c6:
		mov	eax, dword ptr [ebp + 0x44]		// 0x000407c6
		mov	ecx, dword ptr [eax]		// 0x000407c9
		mov	edx, dword ptr [edi]		// 0x000407cb
		push	ecx		// 0x000407cd
		mov	ecx, edi		// 0x000407ce
		call	dword ptr [edx + 0x10]		// 0x000407d0
		mov	edx, dword ptr [esi]		// 0x000407d3
		mov	ecx, dword ptr [esi + 4]		// 0x000407d5
		mov	dword ptr [ebp - 0x3c], ecx		// 0x000407d8
		mov	ecx, dword ptr [esi + 0x10]		// 0x000407db
		mov	dword ptr [ebp - 0x40], edx		// 0x000407de
		mov	edx, dword ptr [esi + 8]		// 0x000407e1
		mov	dword ptr [ebp - 0x34], ecx		// 0x000407e4
		mov	ecx, dword ptr [esi + 0x18]		// 0x000407e7
		mov	dword ptr [ebp - 0x38], edx		// 0x000407ea
		mov	edx, dword ptr [esi + 0x14]		// 0x000407ed
		mov	dword ptr [ebp - 0x2c], ecx		// 0x000407f0
		mov	ecx, dword ptr [esi + 0x24]		// 0x000407f3
		mov	dword ptr [ebp - 0x30], edx		// 0x000407f6
		mov	edx, dword ptr [esi + 0x20]		// 0x000407f9
		add	eax, 0xc		// 0x000407fc
		mov	dword ptr [ebp - 0x24], ecx		// 0x000407ff
		mov	dword ptr [ebp - 0x28], edx		// 0x00040802
		mov	edx, dword ptr [esi + 0x28]		// 0x00040805
		mov	dword ptr [ebp - 0x20], edx		// 0x00040808
		mov	ecx, 9		// 0x0004080b
		lea	esi, [ebp - 0x40]		// 0x00040810
		lea	edi, [ebp - 0x64]		// 0x00040813
		rep movsd		// 0x00040816
		fld	dword ptr [ebp - 0x58]		// 0x00040818
		fmul	dword ptr [eax + 4]		// 0x0004081b
		fld	dword ptr [ebp - 0x4c]		// 0x0004081e
		fmul	dword ptr [eax + 8]		// 0x00040821
		faddp	st(1), st		// 0x00040824
		fld	dword ptr [ebp - 0x64]		// 0x00040826
		fmul	dword ptr [eax]		// 0x00040829
		faddp	st(1), st		// 0x0004082b
		fstp	dword ptr [ebp + 0x2c]		// 0x0004082d
		mov	ecx, dword ptr [ebp + 0x2c]		// 0x00040830
		fld	dword ptr [ebp - 0x54]		// 0x00040833
		mov	dword ptr [ebp - 0x1c], ecx		// 0x00040836
		fmul	dword ptr [eax + 4]		// 0x00040839
		fld	dword ptr [ebp - 0x48]		// 0x0004083c
		fmul	dword ptr [eax + 8]		// 0x0004083f
		faddp	st(1), st		// 0x00040842
		fld	dword ptr [ebp - 0x60]		// 0x00040844
		fmul	dword ptr [eax]		// 0x00040847
		faddp	st(1), st		// 0x00040849
		fstp	dword ptr [ebp + 0x30]		// 0x0004084b
		mov	edx, dword ptr [ebp + 0x30]		// 0x0004084e
		fld	dword ptr [ebp - 0x50]		// 0x00040851
		mov	dword ptr [ebp - 0x18], edx		// 0x00040854
		fmul	dword ptr [eax + 4]		// 0x00040857
		fld	dword ptr [ebp - 0x44]		// 0x0004085a
		fmul	dword ptr [eax + 8]		// 0x0004085d
		faddp	st(1), st		// 0x00040860
		fld	dword ptr [ebp - 0x5c]		// 0x00040862
		fmul	dword ptr [eax]		// 0x00040865
		faddp	st(1), st		// 0x00040867
		fstp	dword ptr [ebp + 0x34]		// 0x00040869
		mov	ecx, dword ptr [ebp + 0x34]		// 0x0004086c
		mov	dword ptr [ebp - 0x14], ecx		// 0x0004086f
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00040872
		fld	dword ptr [ecx + 0x30]		// 0x00040875
		fld	dword ptr [ecx + 0x34]		// 0x00040878
		fld	dword ptr [ecx + 0x38]		// 0x0004087b
		mov	ecx, dword ptr [ebp + 0x58]		// 0x0004087e
		fld	dword ptr [ebp - 0x1c]		// 0x00040881
		fmul	st, st(3)		// 0x00040884
		fld	dword ptr [ebp - 0x14]		// 0x00040886
		fmul	st, st(2)		// 0x00040889
		faddp	st(1), st		// 0x0004088b
		fld	dword ptr [ebp - 0x18]		// 0x0004088d
		fmul	st, st(3)		// 0x00040890
		faddp	st(1), st		// 0x00040892
		fsubr	dword ptr [eax + 0xc]		// 0x00040894
		fstp	dword ptr [ebp - 0x10]		// 0x00040897
		fstp	st(0)		// 0x0004089a
		fstp	st(0)		// 0x0004089c
		fstp	st(0)		// 0x0004089e
		mov	eax, dword ptr [ebp + 0x48]		// 0x000408a0
		mov	eax, dword ptr [eax]		// 0x000408a3
		mov	edx, dword ptr [ecx]		// 0x000408a5
		push	eax		// 0x000408a7
		call	dword ptr [edx + 0x10]		// 0x000408a8
		mov	edx, dword ptr [ebp + 0x68]		// 0x000408ab
		mov	ecx, dword ptr [edx]		// 0x000408ae
		mov	dword ptr [ebp - 0x64], ecx		// 0x000408b0
		mov	ecx, dword ptr [edx + 4]		// 0x000408b3
		mov	dword ptr [ebp - 0x60], ecx		// 0x000408b6
		mov	ecx, dword ptr [edx + 8]		// 0x000408b9
		mov	dword ptr [ebp - 0x5c], ecx		// 0x000408bc
		mov	ecx, dword ptr [edx + 0x10]		// 0x000408bf
		mov	dword ptr [ebp - 0x58], ecx		// 0x000408c2
		mov	ecx, dword ptr [edx + 0x14]		// 0x000408c5
		mov	dword ptr [ebp - 0x54], ecx		// 0x000408c8
		mov	ecx, dword ptr [edx + 0x18]		// 0x000408cb
		mov	dword ptr [ebp - 0x50], ecx		// 0x000408ce
		mov	ecx, dword ptr [edx + 0x20]		// 0x000408d1
		mov	dword ptr [ebp - 0x4c], ecx		// 0x000408d4
		mov	ecx, dword ptr [edx + 0x24]		// 0x000408d7
		mov	dword ptr [ebp - 0x48], ecx		// 0x000408da
		mov	ecx, dword ptr [edx + 0x28]		// 0x000408dd
		add	eax, 0xc		// 0x000408e0
		mov	dword ptr [ebp - 0x44], ecx		// 0x000408e3
		mov	ecx, 9		// 0x000408e6
		lea	esi, [ebp - 0x64]		// 0x000408eb
		lea	edi, [ebp - 0x40]		// 0x000408ee
		rep movsd		// 0x000408f1
		fld	dword ptr [ebp - 0x40]		// 0x000408f3
		fmul	dword ptr [eax]		// 0x000408f6
		fld	dword ptr [ebp - 0x34]		// 0x000408f8
		fmul	dword ptr [eax + 4]		// 0x000408fb
		faddp	st(1), st		// 0x000408fe
		fld	dword ptr [ebp - 0x28]		// 0x00040900
		fmul	dword ptr [eax + 8]		// 0x00040903
		faddp	st(1), st		// 0x00040906
		mov	ebx, dword ptr [ebp + 0x40]		// 0x00040908
		fstp	dword ptr [ebp + 0x2c]		// 0x0004090b
		mov	ecx, dword ptr [ebp + 0x2c]		// 0x0004090e
		fld	dword ptr [ebp - 0x24]		// 0x00040911
		mov	dword ptr [ebp - 0xc], ecx		// 0x00040914
		fmul	dword ptr [eax + 8]		// 0x00040917
		fld	dword ptr [ebp - 0x3c]		// 0x0004091a
		fmul	dword ptr [eax]		// 0x0004091d
		faddp	st(1), st		// 0x0004091f
		fld	dword ptr [ebp - 0x30]		// 0x00040921
		fmul	dword ptr [eax + 4]		// 0x00040924
		faddp	st(1), st		// 0x00040927
		fstp	dword ptr [ebp + 0x30]		// 0x00040929
		mov	ecx, dword ptr [ebp + 0x30]		// 0x0004092c
		fld	dword ptr [ebp - 0x20]		// 0x0004092f
		mov	dword ptr [ebp - 8], ecx		// 0x00040932
		fmul	dword ptr [eax + 8]		// 0x00040935
		fld	dword ptr [ebp - 0x38]		// 0x00040938
		fmul	dword ptr [eax]		// 0x0004093b
		faddp	st(1), st		// 0x0004093d
		fld	dword ptr [ebp - 0x2c]		// 0x0004093f
		fmul	dword ptr [eax + 4]		// 0x00040942
		faddp	st(1), st		// 0x00040945
		fstp	dword ptr [ebp + 0x34]		// 0x00040947
		mov	ecx, dword ptr [ebp + 0x34]		// 0x0004094a
		fld	dword ptr [edx + 0x30]		// 0x0004094d
		mov	dword ptr [ebp - 4], ecx		// 0x00040950
		fld	dword ptr [edx + 0x34]		// 0x00040953
		fld	dword ptr [edx + 0x38]		// 0x00040956
		fld	dword ptr [ebp - 0xc]		// 0x00040959
		fmul	st, st(3)		// 0x0004095c
		fld	dword ptr [ebp - 4]		// 0x0004095e
		fmul	st, st(2)		// 0x00040961
		faddp	st(1), st		// 0x00040963
		fld	dword ptr [ebp - 8]		// 0x00040965
		fmul	st, st(3)		// 0x00040968
		faddp	st(1), st		// 0x0004096a
		fsubr	dword ptr [eax + 0xc]		// 0x0004096c
		lea	eax, [ebx + 0x4e0]		// 0x0004096f
		fstp	dword ptr [ebp]		// 0x00040975
		fstp	st(0)		// 0x00040978
		fstp	st(0)		// 0x0004097a
		fstp	st(0)		// 0x0004097c
		mov	ecx, dword ptr [eax + 4]		// 0x0004097e
		xor	edi, edi		// 0x00040981
		cmp	ecx, edi		// 0x00040983
		je	L4098a		// 0x00040985
		mov	dword ptr [eax + 4], edi		// 0x00040987
L4098a:
		mov	ecx, dword ptr [ebx + 0x4f4]		// 0x0004098a
		cmp	ecx, edi		// 0x00040990
		lea	esi, [ebx + 0x4f0]		// 0x00040992
		je	L4099d		// 0x00040998
		mov	dword ptr [esi + 4], edi		// 0x0004099a
L4099d:
		mov	ecx, dword ptr [ebp + 0x50]		// 0x0004099d
		lea	edx, [ebp - 0xc]		// 0x000409a0
		push	edx		// 0x000409a3
		mov	edx, dword ptr [ebp + 0x5c]		// 0x000409a4
		push	ecx		// 0x000409a7
		mov	ecx, dword ptr [ebp + 0x54]		// 0x000409a8
		push	edx		// 0x000409ab
		mov	edx, dword ptr [ebp + 0x24]		// 0x000409ac
		push	ecx		// 0x000409af
		push	edx		// 0x000409b0
		push	eax		// 0x000409b1
		mov	eax, dword ptr [ebp + 4]		// 0x000409b2
		call	nxConvexGatherEdgeAxes		// 0x000409b5
		mov	ecx, dword ptr [ebp + 0x4c]		// 0x000409ba
		mov	edx, dword ptr [ebp + 0x60]		// 0x000409bd
		lea	eax, [ebp - 0x1c]		// 0x000409c0
		push	eax		// 0x000409c3
		mov	eax, dword ptr [ebp + 0x58]		// 0x000409c4
		push	ecx		// 0x000409c7
		mov	ecx, dword ptr [ebp + 0x20]		// 0x000409c8
		push	edx		// 0x000409cb
		push	eax		// 0x000409cc
		mov	eax, dword ptr [ebp + 8]		// 0x000409cd
		push	ecx		// 0x000409d0
		push	esi		// 0x000409d1
		call	nxConvexGatherEdgeAxes		// 0x000409d2
		mov	ecx, dword ptr [ebx + 0x4e8]		// 0x000409d7
		mov	eax, 0xaaaaaaab		// 0x000409dd
		mul	dword ptr [ebx + 0x4e4]		// 0x000409e2
		mov	esi, edx		// 0x000409e8
		mov	eax, 0xaaaaaaab		// 0x000409ea
		mul	dword ptr [ebx + 0x4f4]		// 0x000409ef
		mov	eax, edx		// 0x000409f5
		mov	edx, dword ptr [ebx + 0x4f8]		// 0x000409f7
		shr	esi, 1		// 0x000409fd
		shr	eax, 1		// 0x000409ff
		add	esp, 0x30		// 0x00040a01
		cmp	esi, edi		// 0x00040a04
		mov	dword ptr [ebp + 0x1c], esi		// 0x00040a06
		mov	dword ptr [ebp + 4], eax		// 0x00040a09
		mov	dword ptr [ebp + 8], edx		// 0x00040a0c
		mov	dword ptr [ebp + 0x28], edi		// 0x00040a0f
		jbe	L40b80		// 0x00040a12
		lea	edx, [ecx + 4]		// 0x00040a18
		mov	dword ptr [ebp + 0x68], edx		// 0x00040a1b
		_emit	0x8b
		_emit	0xff		// 0x00040a1e mov edi, edi
L40a20:
		cmp	eax, edi		// 0x00040a20
		mov	dword ptr [ebp + 0x24], edi		// 0x00040a22
		jbe	L40b6b		// 0x00040a25
		mov	ecx, dword ptr [ebp + 8]		// 0x00040a2b
		add	ecx, 4		// 0x00040a2e
		mov	dword ptr [ebp + 0x64], ecx		// 0x00040a31
L40a34:
		fld	dword ptr [edx]		// 0x00040a34
		fmul	dword ptr [ecx + 4]		// 0x00040a36
		fld	dword ptr [ecx]		// 0x00040a39
		fmul	dword ptr [edx + 4]		// 0x00040a3b
		fsubp	st(1), st		// 0x00040a3e
		fstp	dword ptr [ebp + 0x2c]		// 0x00040a40
		fld	dword ptr [edx + 4]		// 0x00040a43
		fmul	dword ptr [ecx - 4]		// 0x00040a46
		fld	dword ptr [edx - 4]		// 0x00040a49
		fmul	dword ptr [ecx + 4]		// 0x00040a4c
		fsubp	st(1), st		// 0x00040a4f
		fstp	dword ptr [ebp + 0x30]		// 0x00040a51
		fld	dword ptr [edx - 4]		// 0x00040a54
		fmul	dword ptr [ecx]		// 0x00040a57
		fld	dword ptr [edx]		// 0x00040a59
		fmul	dword ptr [ecx - 4]		// 0x00040a5b
		fsubp	st(1), st		// 0x00040a5e
		fstp	dword ptr [ebp + 0x34]		// 0x00040a60
		fld	dword ptr [ebp + 0x2c]		// 0x00040a63
		fabs		// 0x00040a66
		fcomp	qword ptr kConvexMicro		// 0x00040a68
		fnstsw	ax		// 0x00040a6e
		test	ah, 0x41		// 0x00040a70
		je	L40a9d		// 0x00040a73
		fld	dword ptr [ebp + 0x30]		// 0x00040a75
		fabs		// 0x00040a78
		fcomp	qword ptr kConvexMicro		// 0x00040a7a
		fnstsw	ax		// 0x00040a80
		test	ah, 0x41		// 0x00040a82
		je	L40a9d		// 0x00040a85
		fld	dword ptr [ebp + 0x34]		// 0x00040a87
		fabs		// 0x00040a8a
		fcomp	qword ptr kConvexMicro		// 0x00040a8c
		fnstsw	ax		// 0x00040a92
		test	ah, 0x41		// 0x00040a94
		jne	L40b4e		// 0x00040a97
L40a9d:
		fld	dword ptr [ebp + 0x34]		// 0x00040a9d
		fmul	dword ptr [ebp + 0x34]		// 0x00040aa0
		fld	dword ptr [ebp + 0x30]		// 0x00040aa3
		fmul	dword ptr [ebp + 0x30]		// 0x00040aa6
		faddp	st(1), st		// 0x00040aa9
		fld	dword ptr [ebp + 0x2c]		// 0x00040aab
		fmul	dword ptr [ebp + 0x2c]		// 0x00040aae
		faddp	st(1), st		// 0x00040ab1
		fld	dword ptr kConvexZero		// 0x00040ab3
		fld	st(1)		// 0x00040ab9
		fucompp		// 0x00040abb
		fnstsw	ax		// 0x00040abd
		test	ah, 0x44		// 0x00040abf
		jnp	L40ae4		// 0x00040ac2
		fsqrt		// 0x00040ac4
		fdivr	dword ptr kConvexOne		// 0x00040ac6
		fld	dword ptr [ebp + 0x2c]		// 0x00040acc
		fmul	st, st(1)		// 0x00040acf
		fstp	dword ptr [ebp + 0x2c]		// 0x00040ad1
		fld	dword ptr [ebp + 0x30]		// 0x00040ad4
		fmul	st, st(1)		// 0x00040ad7
		fstp	dword ptr [ebp + 0x30]		// 0x00040ad9
		fld	dword ptr [ebp + 0x34]		// 0x00040adc
		fmul	st, st(1)		// 0x00040adf
		fstp	dword ptr [ebp + 0x34]		// 0x00040ae1
L40ae4:
		mov	eax, dword ptr [ebp + 0x70]		// 0x00040ae4
		fstp	st(0)		// 0x00040ae7
		mov	ecx, dword ptr [ebp + 0x6c]		// 0x00040ae9
		mov	esi, dword ptr [ebp + 0x58]		// 0x00040aec
		push	eax		// 0x00040aef
		mov	eax, dword ptr [ebp + 0x60]		// 0x00040af0
		push	ecx		// 0x00040af3
		mov	ecx, dword ptr [ebp + 0x5c]		// 0x00040af4
		lea	edx, [ebp + 0x20]		// 0x00040af7
		push	edx		// 0x00040afa
		push	eax		// 0x00040afb
		push	ecx		// 0x00040afc
		mov	ecx, dword ptr [ebp + 0x54]		// 0x00040afd
		lea	edi, [ebp + 0x2c]		// 0x00040b00
		call	nxConvexAxisOverlap		// 0x00040b03
		add	esp, 0x14		// 0x00040b08
		test	al, al		// 0x00040b0b
		je	L40bb3		// 0x00040b0d
		fld	dword ptr [ebp + 0x20]		// 0x00040b13
		fcomp	dword ptr [ebp + 0xc]		// 0x00040b16
		fnstsw	ax		// 0x00040b19
		test	ah, 5		// 0x00040b1b
		jp	L40b45		// 0x00040b1e
		mov	eax, dword ptr [ebp + 0x2c]		// 0x00040b20
		mov	edx, dword ptr [ebp + 0x20]		// 0x00040b23
		mov	ecx, dword ptr [ebp + 0x30]		// 0x00040b26
		mov	dword ptr [ebp + 0x10], eax		// 0x00040b29
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00040b2c
		test	eax, eax		// 0x00040b2f
		mov	dword ptr [ebp + 0xc], edx		// 0x00040b31
		mov	edx, dword ptr [ebp + 0x34]		// 0x00040b34
		mov	dword ptr [ebp + 0x14], ecx		// 0x00040b37
		mov	dword ptr [ebp + 0x18], edx		// 0x00040b3a
		je	L40b45		// 0x00040b3d
		mov	dword ptr [eax], 2		// 0x00040b3f
L40b45:
		mov	ebx, dword ptr [ebp + 0x40]		// 0x00040b45
		mov	edx, dword ptr [ebp + 0x68]		// 0x00040b48
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00040b4b
L40b4e:
		mov	esi, dword ptr [ebp + 0x24]		// 0x00040b4e
		mov	eax, dword ptr [ebp + 4]		// 0x00040b51
		inc	esi		// 0x00040b54
		add	ecx, 0xc		// 0x00040b55
		cmp	esi, eax		// 0x00040b58
		mov	dword ptr [ebp + 0x24], esi		// 0x00040b5a
		mov	dword ptr [ebp + 0x64], ecx		// 0x00040b5d
		jb	L40a34		// 0x00040b60
		mov	esi, dword ptr [ebp + 0x1c]		// 0x00040b66
		xor	edi, edi		// 0x00040b69
L40b6b:
		mov	ecx, dword ptr [ebp + 0x28]		// 0x00040b6b
		inc	ecx		// 0x00040b6e
		add	edx, 0xc		// 0x00040b6f
		cmp	ecx, esi		// 0x00040b72
		mov	dword ptr [ebp + 0x28], ecx		// 0x00040b74
		mov	dword ptr [ebp + 0x68], edx		// 0x00040b77
		jb	L40a20		// 0x00040b7a
L40b80:
		mov	eax, dword ptr [ebp + 0x74]		// 0x00040b80
		cmp	eax, edi		// 0x00040b83
		je	L40b8c		// 0x00040b85
		mov	ecx, dword ptr [ebp + 0xc]		// 0x00040b87
		mov	dword ptr [eax], ecx		// 0x00040b8a
L40b8c:
		mov	eax, dword ptr [ebp + 0x78]		// 0x00040b8c
		cmp	eax, edi		// 0x00040b8f
		je	L40ba4		// 0x00040b91
		mov	edx, dword ptr [ebp + 0x10]		// 0x00040b93
		mov	ecx, dword ptr [ebp + 0x14]		// 0x00040b96
		mov	dword ptr [eax], edx		// 0x00040b99
		mov	edx, dword ptr [ebp + 0x18]		// 0x00040b9b
		mov	dword ptr [eax + 4], ecx		// 0x00040b9e
		mov	dword ptr [eax + 8], edx		// 0x00040ba1
L40ba4:
		mov	al, 1		// 0x00040ba4
		lea	esp, [ebp - 0x70]		// 0x00040ba6
		pop	edi		// 0x00040ba9
		pop	esi		// 0x00040baa
		pop	ebx		// 0x00040bab
		add	ebp, 0x38		// 0x00040bac
		mov	esp, ebp		// 0x00040baf
		pop	ebp		// 0x00040bb1
		ret		// 0x00040bb2
L40bb3:
		xor	al, al		// 0x00040bb3
		lea	esp, [ebp - 0x70]		// 0x00040bb5
		pop	edi		// 0x00040bb8
		pop	esi		// 0x00040bb9
		pop	ebx		// 0x00040bba
		add	ebp, 0x38		// 0x00040bbb
		mov	esp, ebp		// 0x00040bbe
		pop	ebp		// 0x00040bc0
		ret		// 0x00040bc1
		}
	}

// phys_fn_001818 (0x00040bd0, 1,478 B)
// Convex/convex contacts (cdecl, 12 arguments: the two shapes, the two
// interfaces, the two support maps, two words it never reads, the two 4x4
// poses, the sink, the scratch record). Both centres in world space; the
// centre-to-centre direction's extents on both hulls (slot 11) return when the
// intervals do not meet; the relative poses (001653); each shape's world box
// from its pruner (+0xc4, +0xcc; null for handle 0xffff; 004886 first unless
// flag 2 of +0xac is set); 001816. On success the axis is turned toward B
// (`test ah, 5; jp`) and scaled by the depth; kind 0 takes A's supporting
// polygon (slot 9) and B's face (slot 10), kind 1 A's face and B's supporting
// polygon on the negated axis, kind 2 both faces; the two polygons' normals are
// taken into world space and the one more nearly along the axis (`fabs`,
// `test ah, 5; jp`) is the reference: 001909 with the two polygons in that
// order.
__declspec(naked) void nxConvexConvexContact()
	{
	__asm
		{
		sub	esp, 0xe4		// 0x00040bd0
		push	ebx		// 0x00040bd6
		push	ebp		// 0x00040bd7
		mov	ebp, dword ptr [esp + 0xf8]		// 0x00040bd8
		mov	eax, dword ptr [ebp]		// 0x00040bdf
		push	esi		// 0x00040be2
		push	edi		// 0x00040be3
		mov	ecx, ebp		// 0x00040be4
		call	dword ptr [eax]		// 0x00040be6
		fld	dword ptr [eax]		// 0x00040be8
		mov	esi, dword ptr [esp + 0x118]		// 0x00040bea
		fmul	dword ptr [esi]		// 0x00040bf1
		mov	ebx, dword ptr [esp + 0x104]		// 0x00040bf3
		fld	dword ptr [esi + 0x10]		// 0x00040bfa
		mov	edx, dword ptr [ebx]		// 0x00040bfd
		fmul	dword ptr [eax + 4]		// 0x00040bff
		mov	ecx, ebx		// 0x00040c02
		faddp	st(1), st		// 0x00040c04
		fld	dword ptr [esi + 0x20]		// 0x00040c06
		fmul	dword ptr [eax + 8]		// 0x00040c09
		faddp	st(1), st		// 0x00040c0c
		fadd	dword ptr [esi + 0x30]		// 0x00040c0e
		fstp	dword ptr [esp + 0x2c]		// 0x00040c11
		fld	dword ptr [esi + 0x14]		// 0x00040c15
		fmul	dword ptr [eax + 4]		// 0x00040c18
		fld	dword ptr [eax]		// 0x00040c1b
		fmul	dword ptr [esi + 4]		// 0x00040c1d
		faddp	st(1), st		// 0x00040c20
		fld	dword ptr [esi + 0x24]		// 0x00040c22
		fmul	dword ptr [eax + 8]		// 0x00040c25
		faddp	st(1), st		// 0x00040c28
		fadd	dword ptr [esi + 0x34]		// 0x00040c2a
		fstp	dword ptr [esp + 0x30]		// 0x00040c2d
		fld	dword ptr [eax]		// 0x00040c31
		fmul	dword ptr [esi + 8]		// 0x00040c33
		fld	dword ptr [eax + 8]		// 0x00040c36
		fmul	dword ptr [esi + 0x28]		// 0x00040c39
		faddp	st(1), st		// 0x00040c3c
		fld	dword ptr [esi + 0x18]		// 0x00040c3e
		fmul	dword ptr [eax + 4]		// 0x00040c41
		faddp	st(1), st		// 0x00040c44
		fadd	dword ptr [esi + 0x38]		// 0x00040c46
		fstp	dword ptr [esp + 0x34]		// 0x00040c49
		call	dword ptr [edx]		// 0x00040c4d
		mov	edi, dword ptr [esp + 0x11c]		// 0x00040c4f
		fld	dword ptr [edi]		// 0x00040c56
		fmul	dword ptr [eax]		// 0x00040c58
		fld	dword ptr [eax + 8]		// 0x00040c5a
		fmul	dword ptr [edi + 0x20]		// 0x00040c5d
		faddp	st(1), st		// 0x00040c60
		fld	dword ptr [edi + 0x10]		// 0x00040c62
		fmul	dword ptr [eax + 4]		// 0x00040c65
		faddp	st(1), st		// 0x00040c68
		fadd	dword ptr [edi + 0x30]		// 0x00040c6a
		fld	dword ptr [edi + 4]		// 0x00040c6d
		fmul	dword ptr [eax]		// 0x00040c70
		fld	dword ptr [eax + 8]		// 0x00040c72
		fmul	dword ptr [edi + 0x24]		// 0x00040c75
		faddp	st(1), st		// 0x00040c78
		fld	dword ptr [eax + 4]		// 0x00040c7a
		fmul	dword ptr [edi + 0x14]		// 0x00040c7d
		faddp	st(1), st		// 0x00040c80
		fadd	dword ptr [edi + 0x34]		// 0x00040c82
		fld	dword ptr [edi + 0x28]		// 0x00040c85
		fmul	dword ptr [eax + 8]		// 0x00040c88
		fld	dword ptr [edi + 0x18]		// 0x00040c8b
		fmul	dword ptr [eax + 4]		// 0x00040c8e
		faddp	st(1), st		// 0x00040c91
		fld	dword ptr [edi + 8]		// 0x00040c93
		fmul	dword ptr [eax]		// 0x00040c96
		mov	eax, dword ptr [ebp]		// 0x00040c98
		faddp	st(1), st		// 0x00040c9b
		fadd	dword ptr [edi + 0x38]		// 0x00040c9d
		fstp	dword ptr [esp + 0x70]		// 0x00040ca0
		fxch	st(1)		// 0x00040ca4
		fsub	dword ptr [esp + 0x2c]		// 0x00040ca6
		fstp	dword ptr [esp + 0x3c]		// 0x00040caa
		fsub	dword ptr [esp + 0x30]		// 0x00040cae
		fstp	dword ptr [esp + 0x40]		// 0x00040cb2
		fld	dword ptr [esp + 0x70]		// 0x00040cb6
		fsub	dword ptr [esp + 0x34]		// 0x00040cba
		fstp	dword ptr [esp + 0x44]		// 0x00040cbe
		mov	ecx, dword ptr [esp + 0x108]		// 0x00040cc2
		push	ecx		// 0x00040cc9
		push	esi		// 0x00040cca
		lea	edx, [esp + 0x44]		// 0x00040ccb
		push	edx		// 0x00040ccf
		lea	ecx, [esp + 0x24]		// 0x00040cd0
		push	ecx		// 0x00040cd4
		mov	ecx, dword ptr [esp + 0x134]		// 0x00040cd5
		lea	edx, [esp + 0x2c]		// 0x00040cdc
		push	edx		// 0x00040ce0
		push	ecx		// 0x00040ce1
		mov	ecx, ebp		// 0x00040ce2
		call	dword ptr [eax + 0x2c]		// 0x00040ce4
		mov	eax, dword ptr [esp + 0x10c]		// 0x00040ce7
		mov	edx, dword ptr [ebx]		// 0x00040cee
		push	eax		// 0x00040cf0
		push	edi		// 0x00040cf1
		lea	ecx, [esp + 0x44]		// 0x00040cf2
		push	ecx		// 0x00040cf6
		lea	eax, [esp + 0x1c]		// 0x00040cf7
		push	eax		// 0x00040cfb
		mov	eax, dword ptr [esp + 0x134]		// 0x00040cfc
		lea	ecx, [esp + 0x24]		// 0x00040d03
		push	ecx		// 0x00040d07
		push	eax		// 0x00040d08
		mov	ecx, ebx		// 0x00040d09
		call	dword ptr [edx + 0x2c]		// 0x00040d0b
		fld	dword ptr [esp + 0x18]		// 0x00040d0e
		fcomp	dword ptr [esp + 0x14]		// 0x00040d12
		fnstsw	ax		// 0x00040d16
		test	ah, 5		// 0x00040d18
		jnp	L4118b		// 0x00040d1b
		fld	dword ptr [esp + 0x10]		// 0x00040d21
		fcomp	dword ptr [esp + 0x1c]		// 0x00040d25
		fnstsw	ax		// 0x00040d29
		test	ah, 5		// 0x00040d2b
		jnp	L4118b		// 0x00040d2e
		push	edi		// 0x00040d34
		push	esi		// 0x00040d35
		lea	ecx, [esp + 0xbc]		// 0x00040d36
		push	ecx		// 0x00040d3d
		lea	edx, [esp + 0x80]		// 0x00040d3e
		push	edx		// 0x00040d45
		call	nxIcePosePair		// 0x00040d46
		mov	eax, dword ptr [esp + 0x10c]		// 0x00040d4b
		mov	dx, word ptr [eax + 0xcc]		// 0x00040d52
		lea	ecx, [eax + 0xa4]		// 0x00040d59
		mov	eax, dword ptr [eax + 0xc4]		// 0x00040d5f
		add	eax, 4		// 0x00040d65
		add	esp, 0x10		// 0x00040d68
		_emit	0x66
		_emit	0x81
		_emit	0xfa
		_emit	0xff
		_emit	0xff		// 0x00040d6b cmp dx, 0xffff
		mov	dword ptr [esp + 0x18], ecx		// 0x00040d70
		mov	dword ptr [esp + 0x1c], eax		// 0x00040d74
		jne	L40d84		// 0x00040d78
		mov	dword ptr [esp + 0x14], 0		// 0x00040d7a
		jmp	L40db5		// 0x00040d82
L40d84:
		test	byte ptr [ecx + 8], 2		// 0x00040d84
		jne	L40da4		// 0x00040d88
		mov	eax, dword ptr [eax + 0x10]		// 0x00040d8a
		movzx	edx, dx		// 0x00040d8d
		lea	edx, [edx + edx*2]		// 0x00040d90
		lea	edx, [eax + edx*8]		// 0x00040d93
		push	edx		// 0x00040d96
		call	nxConvexCallUpdateWorldAABB		// 0x00040d97
		mov	eax, dword ptr [esp + 0x1c]		// 0x00040d9c
		mov	ecx, dword ptr [esp + 0x18]		// 0x00040da0
L40da4:
		movzx	ecx, word ptr [ecx + 0x28]		// 0x00040da4
		mov	edx, dword ptr [eax + 0x10]		// 0x00040da8
		lea	ecx, [ecx + ecx*2]		// 0x00040dab
		lea	eax, [edx + ecx*8]		// 0x00040dae
		mov	dword ptr [esp + 0x14], eax		// 0x00040db1
L40db5:
		mov	eax, dword ptr [esp + 0xf8]		// 0x00040db5
		mov	dx, word ptr [eax + 0xcc]		// 0x00040dbc
		lea	ecx, [eax + 0xa4]		// 0x00040dc3
		mov	eax, dword ptr [eax + 0xc4]		// 0x00040dc9
		add	eax, 4		// 0x00040dcf
		_emit	0x66
		_emit	0x81
		_emit	0xfa
		_emit	0xff
		_emit	0xff		// 0x00040dd2 cmp dx, 0xffff
		mov	dword ptr [esp + 0x1c], ecx		// 0x00040dd7
		mov	dword ptr [esp + 0x18], eax		// 0x00040ddb
		jne	L40de5		// 0x00040ddf
		xor	eax, eax		// 0x00040de1
		jmp	L40e12		// 0x00040de3
L40de5:
		test	byte ptr [ecx + 8], 2		// 0x00040de5
		jne	L40e05		// 0x00040de9
		mov	eax, dword ptr [eax + 0x10]		// 0x00040deb
		movzx	edx, dx		// 0x00040dee
		lea	edx, [edx + edx*2]		// 0x00040df1
		lea	edx, [eax + edx*8]		// 0x00040df4
		push	edx		// 0x00040df7
		call	nxConvexCallUpdateWorldAABB		// 0x00040df8
		mov	ecx, dword ptr [esp + 0x1c]		// 0x00040dfd
		mov	eax, dword ptr [esp + 0x18]		// 0x00040e01
L40e05:
		movzx	ecx, word ptr [ecx + 0x28]		// 0x00040e05
		mov	edx, dword ptr [eax + 0x10]		// 0x00040e09
		lea	ecx, [ecx + ecx*2]		// 0x00040e0c
		lea	eax, [edx + ecx*8]		// 0x00040e0f
L40e12:
		lea	ecx, [esp + 0x1c]		// 0x00040e12
		push	ecx		// 0x00040e16
		lea	edx, [esp + 0x24]		// 0x00040e17
		push	edx		// 0x00040e1b
		mov	edx, dword ptr [esp + 0x114]		// 0x00040e1c
		lea	ecx, [esp + 0x18]		// 0x00040e23
		push	ecx		// 0x00040e27
		mov	ecx, dword ptr [esp + 0x114]		// 0x00040e28
		push	edx		// 0x00040e2f
		push	ecx		// 0x00040e30
		lea	edx, [esp + 0xc8]		// 0x00040e31
		push	edx		// 0x00040e38
		mov	edx, dword ptr [esp + 0x2c]		// 0x00040e39
		lea	ecx, [esp + 0x8c]		// 0x00040e3d
		push	ecx		// 0x00040e44
		push	edi		// 0x00040e45
		push	esi		// 0x00040e46
		push	ebx		// 0x00040e47
		push	ebp		// 0x00040e48
		push	edx		// 0x00040e49
		mov	edx, dword ptr [esp + 0x154]		// 0x00040e4a
		push	eax		// 0x00040e51
		lea	eax, [esp + 0x6c]		// 0x00040e52
		push	eax		// 0x00040e56
		lea	ecx, [esp + 0x50]		// 0x00040e57
		push	ecx		// 0x00040e5b
		push	edx		// 0x00040e5c
		call	nxConvexSeparatingAxis		// 0x00040e5d
		add	esp, 0x40		// 0x00040e62
		test	al, al		// 0x00040e65
		je	L4118b		// 0x00040e67
		fld	dword ptr [esp + 0x20]		// 0x00040e6d
		fld	dword ptr [esp + 0x24]		// 0x00040e71
		fld	dword ptr [esp + 0x28]		// 0x00040e75
		fmul	dword ptr [esp + 0x44]		// 0x00040e79
		fld	st(1)		// 0x00040e7d
		fmul	dword ptr [esp + 0x40]		// 0x00040e7f
		faddp	st(1), st		// 0x00040e83
		fld	st(2)		// 0x00040e85
		fmul	dword ptr [esp + 0x3c]		// 0x00040e87
		faddp	st(1), st		// 0x00040e8b
		fcomp	dword ptr kConvexZero		// 0x00040e8d
		fnstsw	ax		// 0x00040e93
		test	ah, 5		// 0x00040e95
		jp	L40eac		// 0x00040e98
		fxch	st(1)		// 0x00040e9a
		fchs		// 0x00040e9c
		fxch	st(1)		// 0x00040e9e
		fchs		// 0x00040ea0
		fld	dword ptr [esp + 0x28]		// 0x00040ea2
		fchs		// 0x00040ea6
		fstp	dword ptr [esp + 0x28]		// 0x00040ea8
L40eac:
		mov	eax, dword ptr [esp + 0x1c]		// 0x00040eac
		fxch	st(1)		// 0x00040eb0
		test	eax, eax		// 0x00040eb2
		fmul	dword ptr [esp + 0x10]		// 0x00040eb4
		fstp	dword ptr [esp + 0x20]		// 0x00040eb8
		fmul	dword ptr [esp + 0x10]		// 0x00040ebc
		fstp	dword ptr [esp + 0x24]		// 0x00040ec0
		fld	dword ptr [esp + 0x28]		// 0x00040ec4
		fmul	dword ptr [esp + 0x10]		// 0x00040ec8
		fstp	dword ptr [esp + 0x28]		// 0x00040ecc
		jne	L40eee		// 0x00040ed0
		mov	eax, dword ptr [ebp]		// 0x00040ed2
		push	esi		// 0x00040ed5
		lea	ecx, [esp + 0x24]		// 0x00040ed6
		push	ecx		// 0x00040eda
		mov	ecx, ebp		// 0x00040edb
		call	dword ptr [eax + 0x24]		// 0x00040edd
		push	0		// 0x00040ee0
		mov	dword ptr [esp + 0x14], eax		// 0x00040ee2
		push	edi		// 0x00040ee6
		lea	eax, [esp + 0x34]		// 0x00040ee7
		push	eax		// 0x00040eeb
		jmp	L40f5b		// 0x00040eec
L40eee:
		cmp	eax, 1		// 0x00040eee
		jne	L40f34		// 0x00040ef1
		mov	edx, dword ptr [ebp]		// 0x00040ef3
		push	0		// 0x00040ef6
		push	esi		// 0x00040ef8
		lea	eax, [esp + 0x28]		// 0x00040ef9
		push	eax		// 0x00040efd
		mov	ecx, ebp		// 0x00040efe
		call	dword ptr [edx + 0x28]		// 0x00040f00
		fld	dword ptr [esp + 0x20]		// 0x00040f03
		mov	edx, dword ptr [ebx]		// 0x00040f07
		fchs		// 0x00040f09
		fstp	dword ptr [esp + 0x2c]		// 0x00040f0b
		mov	dword ptr [esp + 0x10], eax		// 0x00040f0f
		fld	dword ptr [esp + 0x24]		// 0x00040f13
		push	edi		// 0x00040f17
		fchs		// 0x00040f18
		lea	eax, [esp + 0x30]		// 0x00040f1a
		fstp	dword ptr [esp + 0x34]		// 0x00040f1e
		push	eax		// 0x00040f22
		fld	dword ptr [esp + 0x30]		// 0x00040f23
		mov	ecx, ebx		// 0x00040f27
		fchs		// 0x00040f29
		fstp	dword ptr [esp + 0x3c]		// 0x00040f2b
		call	dword ptr [edx + 0x24]		// 0x00040f2f
		jmp	L40f80		// 0x00040f32
L40f34:
		cmp	eax, 2		// 0x00040f34
		jne	L40f84		// 0x00040f37
		mov	edx, dword ptr [ebp]		// 0x00040f39
		lea	eax, [esp + 0x64]		// 0x00040f3c
		push	eax		// 0x00040f40
		push	esi		// 0x00040f41
		lea	ecx, [esp + 0x28]		// 0x00040f42
		push	ecx		// 0x00040f46
		mov	ecx, ebp		// 0x00040f47
		call	dword ptr [edx + 0x28]		// 0x00040f49
		mov	dword ptr [esp + 0x10], eax		// 0x00040f4c
		lea	eax, [esp + 0x60]		// 0x00040f50
		push	eax		// 0x00040f54
		push	edi		// 0x00040f55
		lea	ecx, [esp + 0x34]		// 0x00040f56
		push	ecx		// 0x00040f5a
L40f5b:
		fld	dword ptr [esp + 0x2c]		// 0x00040f5b
		mov	edx, dword ptr [ebx]		// 0x00040f5f
		fchs		// 0x00040f61
		mov	ecx, ebx		// 0x00040f63
		fstp	dword ptr [esp + 0x38]		// 0x00040f65
		fld	dword ptr [esp + 0x30]		// 0x00040f69
		fchs		// 0x00040f6d
		fstp	dword ptr [esp + 0x3c]		// 0x00040f6f
		fld	dword ptr [esp + 0x34]		// 0x00040f73
		fchs		// 0x00040f77
		fstp	dword ptr [esp + 0x40]		// 0x00040f79
		call	dword ptr [edx + 0x28]		// 0x00040f7d
L40f80:
		mov	dword ptr [esp + 0x14], eax		// 0x00040f80
L40f84:
		mov	eax, dword ptr [esp + 0x10]		// 0x00040f84
		mov	edx, dword ptr [ebp]		// 0x00040f88
		push	eax		// 0x00040f8b
		mov	ecx, ebp		// 0x00040f8c
		call	dword ptr [edx + 0x10]		// 0x00040f8e
		mov	edx, dword ptr [ebx]		// 0x00040f91
		mov	ebp, eax		// 0x00040f93
		mov	eax, dword ptr [esp + 0x14]		// 0x00040f95
		push	eax		// 0x00040f99
		mov	ecx, ebx		// 0x00040f9a
		mov	dword ptr [esp + 0x20], ebp		// 0x00040f9c
		call	dword ptr [edx + 0x10]		// 0x00040fa0
		fld	dword ptr [ebp + 0xc]		// 0x00040fa3
		fmul	dword ptr [esi]		// 0x00040fa6
		mov	edx, dword ptr [eax + 4]		// 0x00040fa8
		fld	dword ptr [esi + 0x10]		// 0x00040fab
		mov	ecx, dword ptr [ebp + 4]		// 0x00040fae
		fmul	dword ptr [ebp + 0x10]		// 0x00040fb1
		lea	ebx, [ebp + 0xc]		// 0x00040fb4
		lea	ebp, [eax + 0xc]		// 0x00040fb7
		mov	dword ptr [esp + 0x10], edx		// 0x00040fba
		faddp	st(1), st		// 0x00040fbe
		mov	edx, dword ptr [esp + 0x1c]		// 0x00040fc0
		fld	dword ptr [esi + 0x20]		// 0x00040fc4
		mov	dword ptr [esp + 0x38], ecx		// 0x00040fc7
		fmul	dword ptr [ebx + 8]		// 0x00040fcb
		faddp	st(1), st		// 0x00040fce
		fstp	dword ptr [esp + 0x48]		// 0x00040fd0
		fld	dword ptr [esi + 0x14]		// 0x00040fd4
		fmul	dword ptr [ebx + 4]		// 0x00040fd7
		fld	dword ptr [esi + 0x24]		// 0x00040fda
		fmul	dword ptr [ebx + 8]		// 0x00040fdd
		faddp	st(1), st		// 0x00040fe0
		fld	dword ptr [esi + 4]		// 0x00040fe2
		fmul	dword ptr [ebx]		// 0x00040fe5
		faddp	st(1), st		// 0x00040fe7
		fstp	dword ptr [esp + 0x4c]		// 0x00040fe9
		fld	dword ptr [ebx + 8]		// 0x00040fed
		fmul	dword ptr [esi + 0x28]		// 0x00040ff0
		fld	dword ptr [ebx]		// 0x00040ff3
		fmul	dword ptr [esi + 8]		// 0x00040ff5
		faddp	st(1), st		// 0x00040ff8
		fld	dword ptr [ebx + 4]		// 0x00040ffa
		fmul	dword ptr [esi + 0x18]		// 0x00040ffd
		faddp	st(1), st		// 0x00041000
		fstp	dword ptr [esp + 0x50]		// 0x00041002
		fld	dword ptr [ebp + 8]		// 0x00041006
		fmul	dword ptr [edi + 0x20]		// 0x00041009
		fld	dword ptr [edi + 0x10]		// 0x0004100c
		fmul	dword ptr [ebp + 4]		// 0x0004100f
		faddp	st(1), st		// 0x00041012
		fld	dword ptr [ebp]		// 0x00041014
		fmul	dword ptr [edi]		// 0x00041017
		faddp	st(1), st		// 0x00041019
		fstp	dword ptr [esp + 0x54]		// 0x0004101b
		fld	dword ptr [ebp + 8]		// 0x0004101f
		fmul	dword ptr [edi + 0x24]		// 0x00041022
		fld	dword ptr [ebp + 4]		// 0x00041025
		fmul	dword ptr [edi + 0x14]		// 0x00041028
		faddp	st(1), st		// 0x0004102b
		fld	dword ptr [ebp]		// 0x0004102d
		fmul	dword ptr [edi + 4]		// 0x00041030
		faddp	st(1), st		// 0x00041033
		fstp	dword ptr [esp + 0x58]		// 0x00041035
		fld	dword ptr [ebp + 8]		// 0x00041039
		fmul	dword ptr [edi + 0x28]		// 0x0004103c
		fld	dword ptr [ebp + 4]		// 0x0004103f
		fmul	dword ptr [edi + 0x18]		// 0x00041042
		faddp	st(1), st		// 0x00041045
		fld	dword ptr [ebp]		// 0x00041047
		fmul	dword ptr [edi + 8]		// 0x0004104a
		faddp	st(1), st		// 0x0004104d
		fstp	dword ptr [esp + 0x5c]		// 0x0004104f
		mov	edx, dword ptr [edx]		// 0x00041053
		fld	dword ptr [esp + 0x50]		// 0x00041055
		mov	eax, dword ptr [eax]		// 0x00041059
		fmul	dword ptr [esp + 0x28]		// 0x0004105b
		mov	dword ptr [esp + 0x14], edx		// 0x0004105f
		fld	dword ptr [esp + 0x4c]		// 0x00041063
		mov	dword ptr [esp + 0x18], eax		// 0x00041067
		fmul	dword ptr [esp + 0x24]		// 0x0004106b
		faddp	st(1), st		// 0x0004106f
		push	0		// 0x00041071
		fld	dword ptr [esp + 0x4c]		// 0x00041073
		push	0		// 0x00041077
		fmul	dword ptr [esp + 0x28]		// 0x00041079
		push	0xffff		// 0x0004107d
		push	0xffff		// 0x00041082
		push	0		// 0x00041087
		faddp	st(1), st		// 0x00041089
		push	0		// 0x0004108b
		fabs		// 0x0004108d
		fld	dword ptr [esp + 0x74]		// 0x0004108f
		fmul	dword ptr [esp + 0x40]		// 0x00041093
		fld	dword ptr [esp + 0x70]		// 0x00041097
		fmul	dword ptr [esp + 0x3c]		// 0x0004109b
		faddp	st(1), st		// 0x0004109f
		fld	dword ptr [esp + 0x6c]		// 0x000410a1
		fmul	dword ptr [esp + 0x38]		// 0x000410a5
		faddp	st(1), st		// 0x000410a9
		fabs		// 0x000410ab
		fxch	st(1)		// 0x000410ad
		fxch	st(1)		// 0x000410af
		fcompp		// 0x000410b1
		fnstsw	ax		// 0x000410b3
		test	ah, 5		// 0x000410b5
		mov	eax, dword ptr [esp + 0x110]		// 0x000410b8
		jp	L4112c		// 0x000410bf
		mov	ecx, dword ptr [esp + 0x138]		// 0x000410c1
		mov	edx, dword ptr [esp + 0x114]		// 0x000410c8
		push	ecx		// 0x000410cf
		push	edx		// 0x000410d0
		push	eax		// 0x000410d1
		lea	ecx, [esp + 0xd8]		// 0x000410d2
		push	ecx		// 0x000410d9
		mov	ecx, dword ptr [esp + 0x38]		// 0x000410da
		lea	edx, [esp + 0x9c]		// 0x000410de
		push	edx		// 0x000410e5
		lea	eax, [esp + 0x74]		// 0x000410e6
		push	eax		// 0x000410ea
		push	ebp		// 0x000410eb
		push	edi		// 0x000410ec
		push	ecx		// 0x000410ed
		mov	ecx, dword ptr [esp + 0x140]		// 0x000410ee
		mov	edx, dword ptr [ecx]		// 0x000410f5
		call	dword ptr [edx + 8]		// 0x000410f7
		mov	ecx, dword ptr [esp + 0x74]		// 0x000410fa
		push	eax		// 0x000410fe
		mov	eax, dword ptr [esp + 0x58]		// 0x000410ff
		push	eax		// 0x00041103
		push	ebx		// 0x00041104
		push	esi		// 0x00041105
		push	ecx		// 0x00041106
		mov	ecx, dword ptr [esp + 0x150]		// 0x00041107
		mov	edx, dword ptr [ecx]		// 0x0004110e
		call	dword ptr [edx + 8]		// 0x00041110
		push	eax		// 0x00041113
		mov	eax, dword ptr [esp + 0x68]		// 0x00041114
		push	eax		// 0x00041118
		call	NxConvexPolygonContacts		// 0x00041119
		add	esp, 0x58		// 0x0004111e
		pop	edi		// 0x00041121
		pop	esi		// 0x00041122
		pop	ebp		// 0x00041123
		pop	ebx		// 0x00041124
		add	esp, 0xe4		// 0x00041125
		ret		// 0x0004112b
L4112c:
		mov	edx, dword ptr [esp + 0x138]		// 0x0004112c
		push	edx		// 0x00041133
		mov	edx, dword ptr [esp + 0x118]		// 0x00041134
		push	eax		// 0x0004113b
		push	edx		// 0x0004113c
		lea	eax, [esp + 0x98]		// 0x0004113d
		push	eax		// 0x00041144
		lea	edx, [esp + 0xdc]		// 0x00041145
		push	edx		// 0x0004114c
		lea	eax, [esp + 0x80]		// 0x0004114d
		push	eax		// 0x00041154
		push	ebx		// 0x00041155
		push	esi		// 0x00041156
		push	ecx		// 0x00041157
		mov	ecx, dword ptr [esp + 0x13c]		// 0x00041158
		mov	edx, dword ptr [ecx]		// 0x0004115f
		call	dword ptr [edx + 8]		// 0x00041161
		mov	ecx, dword ptr [esp + 0x4c]		// 0x00041164
		push	eax		// 0x00041168
		mov	eax, dword ptr [esp + 0x54]		// 0x00041169
		push	eax		// 0x0004116d
		push	ebp		// 0x0004116e
		push	edi		// 0x0004116f
		push	ecx		// 0x00041170
		mov	ecx, dword ptr [esp + 0x154]		// 0x00041171
		mov	edx, dword ptr [ecx]		// 0x00041178
		call	dword ptr [edx + 8]		// 0x0004117a
		push	eax		// 0x0004117d
		mov	eax, dword ptr [esp + 0x6c]		// 0x0004117e
		push	eax		// 0x00041182
		call	NxConvexPolygonContacts		// 0x00041183
		add	esp, 0x58		// 0x00041188
L4118b:
		pop	edi		// 0x0004118b
		pop	esi		// 0x0004118c
		pop	ebp		// 0x0004118d
		pop	ebx		// 0x0004118e
		add	esp, 0xe4		// 0x0004118f
		ret		// 0x00041195
		}
	}

// phys_fn_001820 (0x000411a0, 442 B)
// The convex/convex entry (cdecl, the matrix-A signature). When either shape's
// owner (001281) has a null +0x08, 002266 runs with the other shape first, and
// its result is not read: the contact generation continues either way. The
// two poses become 4x4 matrices on the stack (rows +0x00, +0x10, +0x20 with a
// zero fourth column, the translation at +0x30 and 1.0f), and 001818 is called
// with the meshes' (+0xe0) interfaces (+0x04), maps (+0xa8) and +0xa4 words.
__declspec(naked) void NxContactConvexConvex(const NxCollisionShape* /*shape0*/,
	const NxCollisionShape* /*shape1*/, NxContactSink* /*sink*/, void* /*context*/)
	{
	__asm
		{
		sub	esp, 0x88		// 0x000411a0
		push	ebx		// 0x000411a6
		push	ebp		// 0x000411a7
		push	esi		// 0x000411a8
		push	edi		// 0x000411a9
		mov	edi, dword ptr [esp + 0x9c]		// 0x000411aa
		mov	ecx, edi		// 0x000411b1
		call	NxShapeOwner		// 0x000411b3
		mov	ecx, dword ptr [eax + 8]		// 0x000411b8
		test	ecx, ecx		// 0x000411bb
		mov	ebx, dword ptr [esp + 0xa4]		// 0x000411bd
		mov	esi, dword ptr [esp + 0xa0]		// 0x000411c4
		jne	L411d2		// 0x000411cb
		push	ebx		// 0x000411cd
		push	edi		// 0x000411ce
		push	esi		// 0x000411cf
		jmp	L411e3		// 0x000411d0
L411d2:
		mov	ecx, esi		// 0x000411d2
		call	NxShapeOwner		// 0x000411d4
		mov	ecx, dword ptr [eax + 8]		// 0x000411d9
		test	ecx, ecx		// 0x000411dc
		jne	L411eb		// 0x000411de
		push	ebx		// 0x000411e0
		push	esi		// 0x000411e1
		push	edi		// 0x000411e2
L411e3:
		call	NxContinuousCdPair		// 0x000411e3
		add	esp, 0xc		// 0x000411e8
L411eb:
		mov	eax, dword ptr [edi + 0xe0]		// 0x000411eb
		mov	ebp, dword ptr [eax + 0xa4]		// 0x000411f1
		mov	edx, dword ptr [eax + 0xa8]		// 0x000411f7
		mov	ecx, dword ptr [esi + 0xe0]		// 0x000411fd
		mov	ebx, dword ptr [ecx + 0xa8]		// 0x00041203
		mov	dword ptr [esp + 0x10], ebp		// 0x00041209
		mov	ebp, dword ptr [ecx + 0xa4]		// 0x0004120d
		mov	dword ptr [esp + 0x14], ebp		// 0x00041213
		mov	ebp, dword ptr [edi + 0xc]		// 0x00041217
		mov	dword ptr [esp + 0x58], ebp		// 0x0004121a
		mov	ebp, dword ptr [edi + 0x10]		// 0x0004121e
		mov	dword ptr [esp + 0x68], ebp		// 0x00041221
		mov	ebp, dword ptr [edi + 0x14]		// 0x00041225
		mov	dword ptr [esp + 0x78], ebp		// 0x00041228
		mov	ebp, dword ptr [edi + 0x18]		// 0x0004122c
		mov	dword ptr [esp + 0x5c], ebp		// 0x0004122f
		mov	ebp, dword ptr [edi + 0x1c]		// 0x00041233
		mov	dword ptr [esp + 0x6c], ebp		// 0x00041236
		mov	ebp, dword ptr [edi + 0x20]		// 0x0004123a
		mov	dword ptr [esp + 0x7c], ebp		// 0x0004123d
		mov	ebp, dword ptr [edi + 0x24]		// 0x00041241
		mov	dword ptr [esp + 0x60], ebp		// 0x00041244
		mov	ebp, dword ptr [edi + 0x28]		// 0x00041248
		mov	dword ptr [esp + 0x70], ebp		// 0x0004124b
		mov	ebp, dword ptr [edi + 0x2c]		// 0x0004124f
		mov	dword ptr [esp + 0x80], ebp		// 0x00041252
		mov	ebp, dword ptr [edi + 0x30]		// 0x00041259
		mov	dword ptr [esp + 0x88], ebp		// 0x0004125c
		mov	ebp, dword ptr [edi + 0x34]		// 0x00041263
		mov	dword ptr [esp + 0x8c], ebp		// 0x00041266
		mov	ebp, dword ptr [edi + 0x38]		// 0x0004126d
		mov	dword ptr [esp + 0x90], ebp		// 0x00041270
		mov	ebp, dword ptr [esi + 0xc]		// 0x00041277
		mov	dword ptr [esp + 0x18], ebp		// 0x0004127a
		mov	ebp, dword ptr [esi + 0x10]		// 0x0004127e
		mov	dword ptr [esp + 0x28], ebp		// 0x00041281
		mov	ebp, dword ptr [esi + 0x14]		// 0x00041285
		mov	dword ptr [esp + 0x38], ebp		// 0x00041288
		mov	ebp, dword ptr [esi + 0x18]		// 0x0004128c
		mov	dword ptr [esp + 0x1c], ebp		// 0x0004128f
		mov	ebp, dword ptr [esi + 0x1c]		// 0x00041293
		mov	dword ptr [esp + 0x2c], ebp		// 0x00041296
		mov	ebp, dword ptr [esi + 0x20]		// 0x0004129a
		mov	dword ptr [esp + 0x3c], ebp		// 0x0004129d
		mov	ebp, dword ptr [esi + 0x24]		// 0x000412a1
		mov	dword ptr [esp + 0x20], ebp		// 0x000412a4
		mov	ebp, dword ptr [esi + 0x28]		// 0x000412a8
		mov	dword ptr [esp + 0x30], ebp		// 0x000412ab
		mov	ebp, dword ptr [esi + 0x2c]		// 0x000412af
		mov	dword ptr [esp + 0x40], ebp		// 0x000412b2
		mov	ebp, dword ptr [esi + 0x30]		// 0x000412b6
		mov	dword ptr [esp + 0x48], ebp		// 0x000412b9
		mov	ebp, dword ptr [esi + 0x34]		// 0x000412bd
		mov	dword ptr [esp + 0x4c], ebp		// 0x000412c0
		mov	ebp, dword ptr [esi + 0x38]		// 0x000412c4
		mov	dword ptr [esp + 0x50], ebp		// 0x000412c7
		mov	ebp, dword ptr [esp + 0xa8]		// 0x000412cb
		push	ebp		// 0x000412d2
		mov	ebp, dword ptr [esp + 0xa8]		// 0x000412d3
		push	ebp		// 0x000412da
		lea	ebp, [esp + 0x20]		// 0x000412db
		push	ebp		// 0x000412df
		lea	ebp, [esp + 0x64]		// 0x000412e0
		push	ebp		// 0x000412e4
		mov	ebp, dword ptr [esp + 0x24]		// 0x000412e5
		push	ebp		// 0x000412e9
		mov	ebp, dword ptr [esp + 0x24]		// 0x000412ea
		push	ebp		// 0x000412ee
		push	ebx		// 0x000412ef
		add	ecx, 4		// 0x000412f0
		push	edx		// 0x000412f3
		mov	dword ptr [esp + 0xa4], 0		// 0x000412f4
		mov	dword ptr [esp + 0x94], 0		// 0x000412ff
		mov	dword ptr [esp + 0x84], 0		// 0x0004130a
		mov	dword ptr [esp + 0xb4], 0x3f800000		// 0x00041315
		mov	dword ptr [esp + 0x64], 0		// 0x00041320
		mov	dword ptr [esp + 0x54], 0		// 0x00041328
		mov	dword ptr [esp + 0x44], 0		// 0x00041330
		mov	dword ptr [esp + 0x74], 0x3f800000		// 0x00041338
		push	ecx		// 0x00041340
		add	eax, 4		// 0x00041341
		push	eax		// 0x00041344
		push	esi		// 0x00041345
		push	edi		// 0x00041346
		call	nxConvexConvexContact		// 0x00041347
		add	esp, 0x30		// 0x0004134c
		pop	edi		// 0x0004134f
		pop	esi		// 0x00041350
		pop	ebp		// 0x00041351
		pop	ebx		// 0x00041352
		add	esp, 0x88		// 0x00041353
		ret		// 0x00041359
		}
	}
