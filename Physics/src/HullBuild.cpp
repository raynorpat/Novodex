// HullBuild.cpp: the rest of the P-Hull class (ConvexHull, vtable 0x1010769c) -- its face clean-up helpers,
// its release, its scalar deleting destructor, the point-cloud hull builder (virtual slot 1, phys_fn_001476
// to 001492 in the listing's split of one function) and the from-faces initialiser (slot 2, 001494).
// Naked listing transcriptions; the compare script is
// docs/reconstruction/novodex-physics/evidence/phase3-hull-build-listing-compare.py.
//
// Rows (all but the builder and the initialiser are called with the listing's own register conventions;
// the compare script checks the instructions, the harness enters each with the registers it expects):
//   001443 triangle normal of a 16-bit indexed triangle (thiscall, ecx = triangle, `ret 8`)
//   001447 is a point in front of a triangle (thiscall, `ret 8`)
//   001451 drop duplicate faces (esi = the face count's address, cdecl)
//   001453 drop degenerate faces (ebx = vertices, cdecl)
//   001467 orient the faces outward (eax = face count, edx = vertex count, cdecl)
//   001469 the clean-up pass over the three (eax = vertices, cdecl)
//   001471 release every array and the container (esi = the hull)
//   001474 scalar deleting destructor (slot 0)
//   001476-001492 the builder (slot 1, thiscall, `ret 4`, one CONVEXHULLCREATE)
//   001494 the initialiser (slot 2, thiscall, `ret 4`)

#include "NxPhysicsSDK.h"

extern "C" void nxHullCallAllocator();
extern "C" void nxHullCallContainerDtor();
extern "C" void nxHullCallDelete();
extern "C" void nxHullCallAlloca();
extern "C" void nxHullCallVectorConstruct();
extern "C" void nxHullCallReducedCtor();
extern "C" void nxHullCallReduce();
extern "C" void nxHullCallReducedDtor();
extern "C" void nxHullCallComputePolygons();
extern "C" void nxHullCallComputeCentroid();
extern "C" void nxHullIdentityCtor();
#pragma comment(linker, "/alternatename:_nxHullCallAllocator=?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallContainerDtor=??1Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallDelete=??3@YAXPAX@Z")
#pragma comment(linker, "/alternatename:_nxHullCallAlloca=__chkstk")
#pragma comment(linker, "/alternatename:_nxHullCallVectorConstruct=?nxIceVectorConstruct@@YGXPAXIIP6IPAX0@Z@Z")
#pragma comment(linker, "/alternatename:_nxHullCallReducedCtor=??0ReducedVertices@@QAE@PBVPoint@IceMaths@@I@Z")
#pragma comment(linker, "/alternatename:_nxHullCallReduce=?Reduce@ReducedVertices@@QAE_NPAUREDUCEDCLOUD@@@Z")
#pragma comment(linker, "/alternatename:_nxHullCallReducedDtor=??1ReducedVertices@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallComputePolygons=?nxHullComputePolygons@@YI_NPAVConvexHull@@@Z")
#pragma comment(linker, "/alternatename:_nxHullCallComputeCentroid=?nxHullComputeCentroid@@YI_NPBVConvexHull@@IPAVPoint@IceMaths@@@Z")
#pragma comment(linker, "/alternatename:_nxHullIdentityCtor=?nxIceIdentityConstruct@@YIPAXPAX@Z")

void nxHullTriangleNormal();
void nxHullPointInFront();
void nxHullDedupFaces();
void nxHullDropDegenerate();
void nxHullOrientFaces();
void nxHullClean();
void nxHullRelease();
void nxHullScalarDelete();
void nxHullBuild();
void nxHullInit();

// The class's vtable: slot 0 the scalar deleting destructor, slot 1 the builder, slot 2 the initialiser.
void* nxHullVtable[3] = { (void*) &nxHullScalarDelete, (void*) &nxHullBuild, (void*) &nxHullInit };

static const unsigned nxHullK101041ec_4 = 0x3f800000;
static const unsigned nxHullK101041f0_4 = 0x0;
static const unsigned nxHullK101066f8_4 = 0x4f800000;
static const unsigned long long nxHullK10106898_8 = 0x3ff0000000000000ull;
static const unsigned nxHullK10107698_4 = 0x38d1b717;
static const unsigned long long nxHullK101076e0_8 = 0x47efffffe0000000ull;
static const unsigned long long nxHullK101076e8_8 = 0x0ull;
static const unsigned long long nxHullK101076f0_8 = 0xc024000000000000ull;
static const unsigned long long nxHullK101076f8_8 = 0x4054000000000000ull;
static const unsigned long long nxHullK10107700_8 = 0x3fe0000000000000ull;
static const unsigned long long nxHullK10107708_8 = 0x3f00002000400080ull;
static const unsigned long long nxHullK10107710_8 = 0x41f0000000000000ull;
static const unsigned long long nxHullK10107718_8 = 0x3ee4f8b588e368f1ull;

// phys_fn_001443 (0x0002a6e0, 170 B), 58 instructions
__declspec(naked) void nxHullTriangleNormal()
	{
	__asm
		{
		sub esp, 0x18		// 0x0002a6e0
		push esi		// 0x0002a6e3
		mov esi, dword ptr [esp + 0x20]		// 0x0002a6e4
		test esi, esi		// 0x0002a6e8
		je L2a783		// 0x0002a6ea
		movzx eax, word ptr [ecx]		// 0x0002a6f0
		lea eax, [eax + eax*2]		// 0x0002a6f3
		lea edx, [esi + eax*4]		// 0x0002a6f6
		movzx eax, word ptr [ecx + 2]		// 0x0002a6f9
		fld dword ptr [edx]		// 0x0002a6fd
		movzx ecx, word ptr [ecx + 4]		// 0x0002a6ff
		lea eax, [eax + eax*2]		// 0x0002a703
		lea eax, [esi + eax*4]		// 0x0002a706
		lea ecx, [ecx + ecx*2]		// 0x0002a709
		lea ecx, [esi + ecx*4]		// 0x0002a70c
		fsub dword ptr [eax]		// 0x0002a70f
		fld dword ptr [edx + 4]		// 0x0002a711
		fsub dword ptr [eax + 4]		// 0x0002a714
		fld dword ptr [edx + 8]		// 0x0002a717
		mov edx, dword ptr [esp + 0x24]		// 0x0002a71a
		fsub dword ptr [eax + 8]		// 0x0002a71e
		fld dword ptr [ecx]		// 0x0002a721
		fsub dword ptr [eax]		// 0x0002a723
		fstp dword ptr [esp + 4]		// 0x0002a725
		fld dword ptr [ecx + 4]		// 0x0002a729
		fsub dword ptr [eax + 4]		// 0x0002a72c
		fstp dword ptr [esp + 8]		// 0x0002a72f
		fld dword ptr [ecx + 8]		// 0x0002a733
		fsub dword ptr [eax + 8]		// 0x0002a736
		fld dword ptr [esp + 8]		// 0x0002a739
		fmul st(0), st(2)		// 0x0002a73d
		fld st(1)		// 0x0002a73f
		fmul st(0), st(4)		// 0x0002a741
		fsubp st(1), st(0)		// 0x0002a743
		fstp dword ptr [esp + 0x10]		// 0x0002a745
		mov eax, dword ptr [esp + 0x10]		// 0x0002a749
		mov dword ptr [edx], eax		// 0x0002a74d
		fmul st(0), st(3)		// 0x0002a74f
		fld dword ptr [esp + 4]		// 0x0002a751
		fmul st(0), st(2)		// 0x0002a755
		fsubp st(1), st(0)		// 0x0002a757
		fstp dword ptr [esp + 0x14]		// 0x0002a759
		mov ecx, dword ptr [esp + 0x14]		// 0x0002a75d
		mov dword ptr [edx + 4], ecx		// 0x0002a761
		fstp st(0)		// 0x0002a764
		fld dword ptr [esp + 4]		// 0x0002a766
		fmul st(0), st(1)		// 0x0002a76a
		fld dword ptr [esp + 8]		// 0x0002a76c
		fmul st(0), st(3)		// 0x0002a770
		fsubp st(1), st(0)		// 0x0002a772
		fstp dword ptr [esp + 0x18]		// 0x0002a774
		mov eax, dword ptr [esp + 0x18]		// 0x0002a778
		mov dword ptr [edx + 8], eax		// 0x0002a77c
		fstp st(0)		// 0x0002a77f
		fstp st(0)		// 0x0002a781
L2a783:
		pop esi		// 0x0002a783
		add esp, 0x18		// 0x0002a784
		ret 8		// 0x0002a787
		}
	}

// phys_fn_001447 (0x0002a830, 206 B), 74 instructions
__declspec(naked) void nxHullPointInFront()
	{
	__asm
		{
		sub esp, 0x18		// 0x0002a830
		push esi		// 0x0002a833
		mov esi, dword ptr [esp + 0x20]		// 0x0002a834
		test esi, esi		// 0x0002a838
		je L2a8f5		// 0x0002a83a
		movzx eax, word ptr [ecx]		// 0x0002a840
		movzx edx, word ptr [ecx + 2]		// 0x0002a843
		movzx ecx, word ptr [ecx + 4]		// 0x0002a847
		lea eax, [eax + eax*2]		// 0x0002a84b
		lea eax, [esi + eax*4]		// 0x0002a84e
		lea edx, [edx + edx*2]		// 0x0002a851
		lea edx, [esi + edx*4]		// 0x0002a854
		fld dword ptr [eax]		// 0x0002a857
		lea ecx, [ecx + ecx*2]		// 0x0002a859
		lea ecx, [esi + ecx*4]		// 0x0002a85c
		fsub dword ptr [edx]		// 0x0002a85f
		fld dword ptr [eax + 4]		// 0x0002a861
		fsub dword ptr [edx + 4]		// 0x0002a864
		fld dword ptr [eax + 8]		// 0x0002a867
		fsub dword ptr [edx + 8]		// 0x0002a86a
		fld dword ptr [ecx]		// 0x0002a86d
		fsub dword ptr [edx]		// 0x0002a86f
		fstp dword ptr [esp + 4]		// 0x0002a871
		fld dword ptr [ecx + 4]		// 0x0002a875
		fsub dword ptr [edx + 4]		// 0x0002a878
		fstp dword ptr [esp + 8]		// 0x0002a87b
		fld dword ptr [ecx + 8]		// 0x0002a87f
		mov ecx, dword ptr [esp + 0x24]		// 0x0002a882
		fsub dword ptr [edx + 8]		// 0x0002a886
		fld dword ptr [esp + 8]		// 0x0002a889
		fmul st(0), st(2)		// 0x0002a88d
		fld st(1)		// 0x0002a88f
		fmul st(0), st(4)		// 0x0002a891
		fsubp st(1), st(0)		// 0x0002a893
		fstp dword ptr [esp + 0x10]		// 0x0002a895
		fmul st(0), st(3)		// 0x0002a899
		fld dword ptr [esp + 4]		// 0x0002a89b
		fmul st(0), st(2)		// 0x0002a89f
		fsubp st(1), st(0)		// 0x0002a8a1
		fstp dword ptr [esp + 0x14]		// 0x0002a8a3
		fstp st(0)		// 0x0002a8a7
		fld dword ptr [esp + 4]		// 0x0002a8a9
		fmul st(0), st(1)		// 0x0002a8ad
		fld dword ptr [esp + 8]		// 0x0002a8af
		fmul st(0), st(3)		// 0x0002a8b3
		fsubp st(1), st(0)		// 0x0002a8b5
		fstp st(2)		// 0x0002a8b7
		fstp st(0)		// 0x0002a8b9
		fld dword ptr [ecx]		// 0x0002a8bb
		fsub dword ptr [eax]		// 0x0002a8bd
		fld dword ptr [ecx + 4]		// 0x0002a8bf
		fsub dword ptr [eax + 4]		// 0x0002a8c2
		fld dword ptr [ecx + 8]		// 0x0002a8c5
		fsub dword ptr [eax + 8]		// 0x0002a8c8
		fmul st(0), st(3)		// 0x0002a8cb
		fxch st(1)		// 0x0002a8cd
		fmul dword ptr [esp + 0x14]		// 0x0002a8cf
		faddp st(1), st(0)		// 0x0002a8d3
		fxch st(1)		// 0x0002a8d5
		fmul dword ptr [esp + 0x10]		// 0x0002a8d7
		faddp st(1), st(0)		// 0x0002a8db
		fcomp dword ptr [nxHullK101041f0_4]		// 0x0002a8dd
		fnstsw ax		// 0x0002a8e3
		fstp st(0)		// 0x0002a8e5
		test ah, 1		// 0x0002a8e7
		jne L2a8f5		// 0x0002a8ea
		mov al, 1		// 0x0002a8ec
		pop esi		// 0x0002a8ee
		add esp, 0x18		// 0x0002a8ef
		ret 8		// 0x0002a8f2
L2a8f5:
		xor al, al		// 0x0002a8f5
		pop esi		// 0x0002a8f7
		add esp, 0x18		// 0x0002a8f8
		ret 8		// 0x0002a8fb
		}
	}

// phys_fn_001451 (0x0002a9a0, 233 B), 90 instructions
__declspec(naked) void nxHullDedupFaces()
	{
	__asm
		{
		push ebp		// 0x0002a9a0
		mov ebp, esp		// 0x0002a9a1
		sub esp, 0x20		// 0x0002a9a3
		push ebx		// 0x0002a9a6
		push edi		// 0x0002a9a7
		mov edi, dword ptr [esi]		// 0x0002a9a8
		test edi, edi		// 0x0002a9aa
		je L2aa7e		// 0x0002a9ac
		mov eax, dword ptr [ebp + 8]		// 0x0002a9b2
		test eax, eax		// 0x0002a9b5
		je L2aa7e		// 0x0002a9b7
		lea eax, [edi + edi*2]		// 0x0002a9bd
		shl eax, 2		// 0x0002a9c0
		add eax, 3		// 0x0002a9c3
		and eax, 0xfffffffc		// 0x0002a9c6
		call nxHullCallAlloca		// 0x0002a9c9
		xor edx, edx		// 0x0002a9ce
		test edi, edi		// 0x0002a9d0
		mov ebx, esp		// 0x0002a9d2
		jbe L2aa00		// 0x0002a9d4
		mov eax, dword ptr [ebp + 8]		// 0x0002a9d6
		lea ecx, [ebx + 8]		// 0x0002a9d9
		add eax, 4		// 0x0002a9dc
		nop		// 0x0002a9df
L2a9e0:
		movzx edi, word ptr [eax - 4]		// 0x0002a9e0
		mov dword ptr [ecx - 8], edi		// 0x0002a9e4
		movzx edi, word ptr [eax - 2]		// 0x0002a9e7
		mov dword ptr [ecx - 4], edi		// 0x0002a9eb
		movzx edi, word ptr [eax]		// 0x0002a9ee
		mov dword ptr [ecx], edi		// 0x0002a9f1
		mov edi, dword ptr [esi]		// 0x0002a9f3
		inc edx		// 0x0002a9f5
		add eax, 6		// 0x0002a9f6
		add ecx, 0xc		// 0x0002a9f9
		cmp edx, edi		// 0x0002a9fc
		jb L2a9e0		// 0x0002a9fe
L2aa00:
		mov eax, dword ptr [esi]		// 0x0002aa00
		push eax		// 0x0002aa02
		push ebx		// 0x0002aa03
		lea ecx, [ebp - 0x20]		// 0x0002aa04
		call nxHullCallReducedCtor		// 0x0002aa07
		lea ecx, [ebp - 0xc]		// 0x0002aa0c
		push ecx		// 0x0002aa0f
		lea ecx, [ebp - 0x20]		// 0x0002aa10
		call nxHullCallReduce		// 0x0002aa13
		mov eax, dword ptr [ebp - 8]		// 0x0002aa18
		cmp eax, dword ptr [esi]		// 0x0002aa1b
		jae L2aa76		// 0x0002aa1d
		mov cl, byte ptr [ebp + 0xc]		// 0x0002aa1f
		test cl, cl		// 0x0002aa22
		je L2aa63		// 0x0002aa24
		xor edx, edx		// 0x0002aa26
		test eax, eax		// 0x0002aa28
		mov dword ptr [esi], eax		// 0x0002aa2a
		jbe L2aa63		// 0x0002aa2c
		mov ecx, dword ptr [ebp + 8]		// 0x0002aa2e
		mov eax, dword ptr [ebp - 0xc]		// 0x0002aa31
		add ecx, 4		// 0x0002aa34
		add eax, 8		// 0x0002aa37
		lea ebx, [ebx]		// 0x0002aa3a
L2aa40:
		mov di, word ptr [eax - 8]		// 0x0002aa40
		mov word ptr [ecx - 4], di		// 0x0002aa44
		mov di, word ptr [eax - 4]		// 0x0002aa48
		mov word ptr [ecx - 2], di		// 0x0002aa4c
		mov di, word ptr [eax]		// 0x0002aa50
		mov word ptr [ecx], di		// 0x0002aa53
		mov edi, dword ptr [esi]		// 0x0002aa56
		inc edx		// 0x0002aa58
		add eax, 0xc		// 0x0002aa59
		add ecx, 6		// 0x0002aa5c
		cmp edx, edi		// 0x0002aa5f
		jb L2aa40		// 0x0002aa61
L2aa63:
		lea ecx, [ebp - 0x20]		// 0x0002aa63
		call nxHullCallReducedDtor		// 0x0002aa66
		xor al, al		// 0x0002aa6b
		lea esp, [ebp - 0x28]		// 0x0002aa6d
		pop edi		// 0x0002aa70
		pop ebx		// 0x0002aa71
		mov esp, ebp		// 0x0002aa72
		pop ebp		// 0x0002aa74
		ret		// 0x0002aa75
L2aa76:
		lea ecx, [ebp - 0x20]		// 0x0002aa76
		call nxHullCallReducedDtor		// 0x0002aa79
L2aa7e:
		mov al, 1		// 0x0002aa7e
		lea esp, [ebp - 0x28]		// 0x0002aa80
		pop edi		// 0x0002aa83
		pop ebx		// 0x0002aa84
		mov esp, ebp		// 0x0002aa85
		pop ebp		// 0x0002aa87
		ret		// 0x0002aa88
		}
	}

// phys_fn_001453 (0x0002aa90, 584 B), 210 instructions
__declspec(naked) void nxHullDropDegenerate()
	{
	__asm
		{
		sub esp, 0x28		// 0x0002aa90
		push edi		// 0x0002aa93
		mov edi, dword ptr [esp + 0x30]		// 0x0002aa94
		mov eax, dword ptr [edi]		// 0x0002aa98
		test eax, eax		// 0x0002aa9a
		je L2acd1		// 0x0002aa9c
		mov ecx, dword ptr [esp + 0x34]		// 0x0002aaa2
		test ecx, ecx		// 0x0002aaa6
		je L2acd1		// 0x0002aaa8
		test ebx, ebx		// 0x0002aaae
		je L2acd1		// 0x0002aab0
		push ebp		// 0x0002aab6
		push esi		// 0x0002aab7
L2aab8:
		xor esi, esi		// 0x0002aab8
		test eax, eax		// 0x0002aaba
		jbe L2ab04		// 0x0002aabc
		mov ebp, dword ptr [esp + 0x3c]		// 0x0002aabe
L2aac2:
		lea eax, [esp + 0x28]		// 0x0002aac2
		push eax		// 0x0002aac6
		push ebx		// 0x0002aac7
		mov ecx, ebp		// 0x0002aac8
		call nxHullTriangleNormal		// 0x0002aaca
		fld dword ptr [esp + 0x30]		// 0x0002aacf
		fmul dword ptr [esp + 0x30]		// 0x0002aad3
		fld dword ptr [esp + 0x2c]		// 0x0002aad7
		fmul dword ptr [esp + 0x2c]		// 0x0002aadb
		faddp st(1), st(0)		// 0x0002aadf
		fld dword ptr [esp + 0x28]		// 0x0002aae1
		fmul dword ptr [esp + 0x28]		// 0x0002aae5
		faddp st(1), st(0)		// 0x0002aae9
		fsqrt		// 0x0002aaeb
		fcomp dword ptr [nxHullK10107698_4]		// 0x0002aaed
		fnstsw ax		// 0x0002aaf3
		test ah, 5		// 0x0002aaf5
		jnp L2ab0d		// 0x0002aaf8
		mov eax, dword ptr [edi]		// 0x0002aafa
		inc esi		// 0x0002aafc
		add ebp, 6		// 0x0002aafd
		cmp esi, eax		// 0x0002ab00
		jb L2aac2		// 0x0002ab02
L2ab04:
		pop esi		// 0x0002ab04
		pop ebp		// 0x0002ab05
		mov al, 1		// 0x0002ab06
		pop edi		// 0x0002ab08
		add esp, 0x28		// 0x0002ab09
		ret		// 0x0002ab0c
L2ab0d:
		mov al, byte ptr [esp + 0x40]		// 0x0002ab0d
		test al, al		// 0x0002ab11
		je L2acc8		// 0x0002ab13
		mov edx, dword ptr [esp + 0x3c]		// 0x0002ab19
		lea ecx, [esi + esi*2]		// 0x0002ab1d
		mov si, word ptr [edx + ecx*2]		// 0x0002ab20
		mov di, word ptr [edx + ecx*2 + 2]		// 0x0002ab24
		lea edx, [edx + ecx*2]		// 0x0002ab29
		mov bp, word ptr [edx + 4]		// 0x0002ab2c
		movzx eax, si		// 0x0002ab30
		movzx ecx, di		// 0x0002ab33
		lea eax, [eax + eax*2]		// 0x0002ab36
		lea eax, [ebx + eax*4]		// 0x0002ab39
		lea ecx, [ecx + ecx*2]		// 0x0002ab3c
		lea ecx, [ebx + ecx*4]		// 0x0002ab3f
		fld dword ptr [eax]		// 0x0002ab42
		movzx edx, bp		// 0x0002ab44
		fsub dword ptr [ecx]		// 0x0002ab47
		lea edx, [edx + edx*2]		// 0x0002ab49
		lea edx, [ebx + edx*4]		// 0x0002ab4c
		fstp dword ptr [esp + 0x1c]		// 0x0002ab4f
		fld dword ptr [eax + 4]		// 0x0002ab53
		fsub dword ptr [ecx + 4]		// 0x0002ab56
		fstp dword ptr [esp + 0x20]		// 0x0002ab59
		fld dword ptr [eax + 8]		// 0x0002ab5d
		fsub dword ptr [ecx + 8]		// 0x0002ab60
		fstp dword ptr [esp + 0x24]		// 0x0002ab63
		fld dword ptr [eax]		// 0x0002ab67
		fsub dword ptr [edx]		// 0x0002ab69
		fld dword ptr [eax + 4]		// 0x0002ab6b
		fsub dword ptr [edx + 4]		// 0x0002ab6e
		fld dword ptr [eax + 8]		// 0x0002ab71
		fsub dword ptr [edx + 8]		// 0x0002ab74
		fld dword ptr [ecx]		// 0x0002ab77
		fsub dword ptr [edx]		// 0x0002ab79
		fstp dword ptr [esp + 0x10]		// 0x0002ab7b
		fld dword ptr [ecx + 4]		// 0x0002ab7f
		fsub dword ptr [edx + 4]		// 0x0002ab82
		fstp dword ptr [esp + 0x14]		// 0x0002ab85
		fld dword ptr [ecx + 8]		// 0x0002ab89
		xor ecx, ecx		// 0x0002ab8c
		fsub dword ptr [edx + 8]		// 0x0002ab8e
		fstp dword ptr [esp + 0x18]		// 0x0002ab91
		fld st(0)		// 0x0002ab95
		fmul st(0), st(1)		// 0x0002ab97
		fld st(2)		// 0x0002ab99
		fmul st(0), st(3)		// 0x0002ab9b
		faddp st(1), st(0)		// 0x0002ab9d
		fld st(3)		// 0x0002ab9f
		fmul st(0), st(4)		// 0x0002aba1
		faddp st(1), st(0)		// 0x0002aba3
		fsqrt		// 0x0002aba5
		fstp st(3)		// 0x0002aba7
		fstp st(0)		// 0x0002aba9
		fstp st(0)		// 0x0002abab
		fstp dword ptr [esp + 0xc]		// 0x0002abad
		fld dword ptr [esp + 0x18]		// 0x0002abb1
		fmul dword ptr [esp + 0x18]		// 0x0002abb5
		fld dword ptr [esp + 0x14]		// 0x0002abb9
		fmul dword ptr [esp + 0x14]		// 0x0002abbd
		faddp st(1), st(0)		// 0x0002abc1
		fld dword ptr [esp + 0x10]		// 0x0002abc3
		fmul dword ptr [esp + 0x10]		// 0x0002abc7
		faddp st(1), st(0)		// 0x0002abcb
		fsqrt		// 0x0002abcd
		fld dword ptr [esp + 0x24]		// 0x0002abcf
		fmul dword ptr [esp + 0x24]		// 0x0002abd3
		fld dword ptr [esp + 0x20]		// 0x0002abd7
		fmul dword ptr [esp + 0x20]		// 0x0002abdb
		faddp st(1), st(0)		// 0x0002abdf
		fld dword ptr [esp + 0x1c]		// 0x0002abe1
		fmul dword ptr [esp + 0x1c]		// 0x0002abe5
		faddp st(1), st(0)		// 0x0002abe9
		fsqrt		// 0x0002abeb
		fld dword ptr [esp + 0xc]		// 0x0002abed
		fcomp st(1)		// 0x0002abf1
		fnstsw ax		// 0x0002abf3
		test ah, 5		// 0x0002abf5
		jp L2ac05		// 0x0002abf8
		fstp st(0)		// 0x0002abfa
		mov ecx, 1		// 0x0002abfc
		fld dword ptr [esp + 0xc]		// 0x0002ac01
L2ac05:
		fxch st(1)		// 0x0002ac05
		fcomp st(1)		// 0x0002ac07
		fnstsw ax		// 0x0002ac09
		fstp st(0)		// 0x0002ac0b
		test ah, 5		// 0x0002ac0d
		jnp L2ac23		// 0x0002ac10
		test ecx, ecx		// 0x0002ac12
		movzx edx, si		// 0x0002ac14
		jne L2ac1e		// 0x0002ac17
		movzx ecx, di		// 0x0002ac19
		jmp L2ac29		// 0x0002ac1c
L2ac1e:
		cmp ecx, 1		// 0x0002ac1e
		je L2ac26		// 0x0002ac21
L2ac23:
		movzx edx, di		// 0x0002ac23
L2ac26:
		movzx ecx, bp		// 0x0002ac26
L2ac29:
		mov edi, dword ptr [esp + 0x38]		// 0x0002ac29
		mov eax, dword ptr [edi]		// 0x0002ac2d
		xor esi, esi		// 0x0002ac2f
		test eax, eax		// 0x0002ac31
		jbe L2ac6c		// 0x0002ac33
		mov eax, dword ptr [esp + 0x3c]		// 0x0002ac35
		add eax, 4		// 0x0002ac39
		lea esp, [esp]		// 0x0002ac3c
L2ac40:
		movzx ebp, word ptr [eax - 4]		// 0x0002ac40
		cmp ebp, ecx		// 0x0002ac44
		jne L2ac4c		// 0x0002ac46
		mov word ptr [eax - 4], dx		// 0x0002ac48
L2ac4c:
		movzx ebp, word ptr [eax - 2]		// 0x0002ac4c
		cmp ebp, ecx		// 0x0002ac50
		jne L2ac58		// 0x0002ac52
		mov word ptr [eax - 2], dx		// 0x0002ac54
L2ac58:
		movzx ebp, word ptr [eax]		// 0x0002ac58
		cmp ebp, ecx		// 0x0002ac5b
		jne L2ac62		// 0x0002ac5d
		mov word ptr [eax], dx		// 0x0002ac5f
L2ac62:
		mov ebp, dword ptr [edi]		// 0x0002ac62
		inc esi		// 0x0002ac64
		add eax, 6		// 0x0002ac65
		cmp esi, ebp		// 0x0002ac68
		jb L2ac40		// 0x0002ac6a
L2ac6c:
		mov eax, dword ptr [edi]		// 0x0002ac6c
		test eax, eax		// 0x0002ac6e
		jbe L2acbd		// 0x0002ac70
		mov ebp, dword ptr [esp + 0x3c]		// 0x0002ac72
		lea ecx, [ebp + 4]		// 0x0002ac76
		mov dword ptr [esp + 0xc], eax		// 0x0002ac79
		lea ecx, [ecx]		// 0x0002ac7d
L2ac80:
		mov dx, word ptr [ecx - 4]		// 0x0002ac80
		mov ax, word ptr [ecx - 2]		// 0x0002ac84
		cmp dx, ax		// 0x0002ac88
		je L2ac9a		// 0x0002ac8b
		mov si, word ptr [ecx]		// 0x0002ac8d
		cmp ax, si		// 0x0002ac90
		je L2ac9a		// 0x0002ac93
		cmp si, dx		// 0x0002ac95
		jne L2acb4		// 0x0002ac98
L2ac9a:
		mov eax, dword ptr [edi]		// 0x0002ac9a
		dec eax		// 0x0002ac9c
		mov dword ptr [edi], eax		// 0x0002ac9d
		lea eax, [eax + eax*2]		// 0x0002ac9f
		lea edx, [ebp + eax*2]		// 0x0002aca2
		mov eax, dword ptr [edx]		// 0x0002aca6
		mov dword ptr [ecx - 4], eax		// 0x0002aca8
		mov dx, word ptr [edx + 4]		// 0x0002acab
		mov word ptr [ecx], dx		// 0x0002acaf
		jmp L2acb7		// 0x0002acb2
L2acb4:
		add ecx, 6		// 0x0002acb4
L2acb7:
		dec dword ptr [esp + 0xc]		// 0x0002acb7
		jne L2ac80		// 0x0002acbb
L2acbd:
		mov eax, dword ptr [edi]		// 0x0002acbd
		cmp eax, 4		// 0x0002acbf
		ja L2aab8		// 0x0002acc2
L2acc8:
		pop esi		// 0x0002acc8
		pop ebp		// 0x0002acc9
		xor al, al		// 0x0002acca
		pop edi		// 0x0002accc
		add esp, 0x28		// 0x0002accd
		ret		// 0x0002acd0
L2acd1:
		xor al, al		// 0x0002acd1
		pop edi		// 0x0002acd3
		add esp, 0x28		// 0x0002acd4
		ret		// 0x0002acd7
		}
	}

// phys_fn_001467 (0x0002b3b0, 452 B), 144 instructions
__declspec(naked) void nxHullOrientFaces()
	{
	__asm
		{
		sub esp, 0x10		// 0x0002b3b0
		test edx, edx		// 0x0002b3b3
		push ebp		// 0x0002b3b5
		mov ebp, dword ptr [esp + 0x18]		// 0x0002b3b6
		push edi		// 0x0002b3ba
		mov edi, eax		// 0x0002b3bb
		je L2b56c		// 0x0002b3bd
		test ebp, ebp		// 0x0002b3c3
		je L2b56c		// 0x0002b3c5
		test edi, edi		// 0x0002b3cb
		je L2b56c		// 0x0002b3cd
		mov eax, dword ptr [esp + 0x20]		// 0x0002b3d3
		test eax, eax		// 0x0002b3d7
		je L2b56c		// 0x0002b3d9
		test edx, edx		// 0x0002b3df
		push ebx		// 0x0002b3e1
		mov dword ptr [esp + 0xc], edx		// 0x0002b3e2
		fild dword ptr [esp + 0xc]		// 0x0002b3e6
		push esi		// 0x0002b3ea
		jge L2b3f3		// 0x0002b3eb
		fadd dword ptr [nxHullK101066f8_4]		// 0x0002b3ed
L2b3f3:
		fdivr dword ptr [nxHullK101041ec_4]		// 0x0002b3f3
		xor esi, esi		// 0x0002b3f9
		cmp edx, 4		// 0x0002b3fb
		mov dword ptr [esp + 0x18], 0		// 0x0002b3fe
		mov dword ptr [esp + 0x1c], 0		// 0x0002b406
		fstp dword ptr [esp + 0x10]		// 0x0002b40e
		fld dword ptr [nxHullK101041f0_4]		// 0x0002b412
		fst dword ptr [esp + 0x14]		// 0x0002b418
		jl L2b4df		// 0x0002b41c
		lea ecx, [edx - 4]		// 0x0002b422
		shr ecx, 2		// 0x0002b425
		inc ecx		// 0x0002b428
		lea eax, [ebp + 8]		// 0x0002b429
		lea esi, [ecx*4]		// 0x0002b42c
L2b433:
		fld dword ptr [esp + 0x10]		// 0x0002b433
		add eax, 0x30		// 0x0002b437
		dec ecx		// 0x0002b43a
		fmul dword ptr [eax - 0x38]		// 0x0002b43b
		faddp st(1), st(0)		// 0x0002b43e
		fld dword ptr [esp + 0x10]		// 0x0002b440
		fmul dword ptr [eax - 0x34]		// 0x0002b444
		fadd dword ptr [esp + 0x18]		// 0x0002b447
		fld dword ptr [esp + 0x10]		// 0x0002b44b
		fmul dword ptr [eax - 0x30]		// 0x0002b44f
		fadd dword ptr [esp + 0x1c]		// 0x0002b452
		fstp dword ptr [esp + 0x1c]		// 0x0002b456
		fld dword ptr [esp + 0x10]		// 0x0002b45a
		fmul dword ptr [eax - 0x2c]		// 0x0002b45e
		fadd st(0), st(2)		// 0x0002b461
		fstp dword ptr [esp + 0x14]		// 0x0002b463
		fld dword ptr [esp + 0x10]		// 0x0002b467
		fmul dword ptr [eax - 0x28]		// 0x0002b46b
		fadd st(0), st(1)		// 0x0002b46e
		fstp st(2)		// 0x0002b470
		fstp st(0)		// 0x0002b472
		fld dword ptr [esp + 0x10]		// 0x0002b474
		fmul dword ptr [eax - 0x24]		// 0x0002b478
		fadd dword ptr [esp + 0x1c]		// 0x0002b47b
		fld dword ptr [esp + 0x10]		// 0x0002b47f
		fmul dword ptr [eax - 0x20]		// 0x0002b483
		fadd dword ptr [esp + 0x14]		// 0x0002b486
		fstp dword ptr [esp + 0x14]		// 0x0002b48a
		fld dword ptr [esp + 0x10]		// 0x0002b48e
		fmul dword ptr [eax - 0x1c]		// 0x0002b492
		fadd st(0), st(2)		// 0x0002b495
		fstp dword ptr [esp + 0x18]		// 0x0002b497
		fld dword ptr [esp + 0x10]		// 0x0002b49b
		fmul dword ptr [eax - 0x18]		// 0x0002b49f
		fadd st(0), st(1)		// 0x0002b4a2
		fstp dword ptr [esp + 0x1c]		// 0x0002b4a4
		fstp st(0)		// 0x0002b4a8
		fstp st(0)		// 0x0002b4aa
		fld dword ptr [esp + 0x10]		// 0x0002b4ac
		fmul dword ptr [eax - 0x14]		// 0x0002b4b0
		fadd dword ptr [esp + 0x14]		// 0x0002b4b3
		fst dword ptr [esp + 0x14]		// 0x0002b4b7
		fld dword ptr [esp + 0x10]		// 0x0002b4bb
		fmul dword ptr [eax - 0x10]		// 0x0002b4bf
		fadd dword ptr [esp + 0x18]		// 0x0002b4c2
		fstp dword ptr [esp + 0x18]		// 0x0002b4c6
		fld dword ptr [esp + 0x10]		// 0x0002b4ca
		fmul dword ptr [eax - 0xc]		// 0x0002b4ce
		fadd dword ptr [esp + 0x1c]		// 0x0002b4d1
		fstp dword ptr [esp + 0x1c]		// 0x0002b4d5
		jne L2b433		// 0x0002b4d9
L2b4df:
		cmp esi, edx		// 0x0002b4df
		jae L2b523		// 0x0002b4e1
		lea eax, [esi + esi*2]		// 0x0002b4e3
		lea eax, [ebp + eax*4 + 8]		// 0x0002b4e6
		sub edx, esi		// 0x0002b4ea
		lea esp, [esp]		// 0x0002b4ec
L2b4f0:
		fld dword ptr [esp + 0x10]		// 0x0002b4f0
		add eax, 0xc		// 0x0002b4f4
		dec edx		// 0x0002b4f7
		fmul dword ptr [eax - 0x14]		// 0x0002b4f8
		faddp st(1), st(0)		// 0x0002b4fb
		fld dword ptr [esp + 0x10]		// 0x0002b4fd
		fmul dword ptr [eax - 0x10]		// 0x0002b501
		fadd dword ptr [esp + 0x18]		// 0x0002b504
		fstp dword ptr [esp + 0x18]		// 0x0002b508
		fld dword ptr [esp + 0x10]		// 0x0002b50c
		fmul dword ptr [eax - 0xc]		// 0x0002b510
		fadd dword ptr [esp + 0x1c]		// 0x0002b513
		fstp dword ptr [esp + 0x1c]		// 0x0002b517
		jne L2b4f0		// 0x0002b51b
		fstp dword ptr [esp + 0x14]		// 0x0002b51d
		jmp L2b525		// 0x0002b521
L2b523:
		fstp st(0)		// 0x0002b523
L2b525:
		test edi, edi		// 0x0002b525
		mov bl, 1		// 0x0002b527
		jbe L2b562		// 0x0002b529
		mov esi, dword ptr [esp + 0x28]		// 0x0002b52b
		add esi, 2		// 0x0002b52f
L2b532:
		lea ecx, [esp + 0x14]		// 0x0002b532
		push ecx		// 0x0002b536
		push ebp		// 0x0002b537
		lea ecx, [esi - 2]		// 0x0002b538
		call nxHullPointInFront		// 0x0002b53b
		test al, al		// 0x0002b540
		je L2b55c		// 0x0002b542
		mov al, byte ptr [esp + 0x2c]		// 0x0002b544
		test al, al		// 0x0002b548
		je L2b55a		// 0x0002b54a
		mov dx, word ptr [esi + 2]		// 0x0002b54c
		mov ax, word ptr [esi]		// 0x0002b550
		mov word ptr [esi], dx		// 0x0002b553
		mov word ptr [esi + 2], ax		// 0x0002b556
L2b55a:
		xor bl, bl		// 0x0002b55a
L2b55c:
		add esi, 6		// 0x0002b55c
		dec edi		// 0x0002b55f
		jne L2b532		// 0x0002b560
L2b562:
		pop esi		// 0x0002b562
		mov al, bl		// 0x0002b563
		pop ebx		// 0x0002b565
		pop edi		// 0x0002b566
		pop ebp		// 0x0002b567
		add esp, 0x10		// 0x0002b568
		ret		// 0x0002b56b
L2b56c:
		pop edi		// 0x0002b56c
		xor al, al		// 0x0002b56d
		pop ebp		// 0x0002b56f
		add esp, 0x10		// 0x0002b570
		ret		// 0x0002b573
		}
	}

// phys_fn_001469 (0x0002b580, 76 B), 34 instructions
__declspec(naked) void nxHullClean()
	{
	__asm
		{
		push ebx		// 0x0002b580
		push esi		// 0x0002b581
		push 0		// 0x0002b582
		push edi		// 0x0002b584
		lea esi, [esp + 0x14]		// 0x0002b585
		mov ebx, eax		// 0x0002b589
		call nxHullDedupFaces		// 0x0002b58b
		add esp, 8		// 0x0002b590
		test al, al		// 0x0002b593
		jne L2b59c		// 0x0002b595
L2b597:
		pop esi		// 0x0002b597
		xor al, al		// 0x0002b598
		pop ebx		// 0x0002b59a
		ret		// 0x0002b59b
L2b59c:
		mov eax, dword ptr [esp + 0xc]		// 0x0002b59c
		mov edx, dword ptr [esp + 0x10]		// 0x0002b5a0
		push 0		// 0x0002b5a4
		push edi		// 0x0002b5a6
		push ebx		// 0x0002b5a7
		call nxHullOrientFaces		// 0x0002b5a8
		add esp, 0xc		// 0x0002b5ad
		test al, al		// 0x0002b5b0
		je L2b597		// 0x0002b5b2
		push 0		// 0x0002b5b4
		lea eax, [esp + 0x10]		// 0x0002b5b6
		push edi		// 0x0002b5ba
		push eax		// 0x0002b5bb
		call nxHullDropDegenerate		// 0x0002b5bc
		add esp, 0xc		// 0x0002b5c1
		test al, al		// 0x0002b5c4
		pop esi		// 0x0002b5c6
		setne al		// 0x0002b5c7
		pop ebx		// 0x0002b5ca
		ret		// 0x0002b5cb
		}
	}

// phys_fn_001471 (0x0002b5d0, 288 B), 112 instructions
__declspec(naked) void nxHullRelease()
	{
	__asm
		{
		push ebx		// 0x0002b5d0
		push esi		// 0x0002b5d1
		mov esi, ecx		// 0x0002b5d2
		mov ebx, dword ptr [esi + 0x44]		// 0x0002b5d4
		push edi		// 0x0002b5d7
		xor edi, edi		// 0x0002b5d8
		cmp ebx, edi		// 0x0002b5da
		mov dword ptr [esi], offset nxHullVtable		// 0x0002b5dc
		je L2b5f4		// 0x0002b5e2
		call nxHullCallAllocator		// 0x0002b5e4
		mov edx, dword ptr [eax]		// 0x0002b5e9
		push ebx		// 0x0002b5eb
		mov ecx, eax		// 0x0002b5ec
		call dword ptr [edx + 0xc]		// 0x0002b5ee
		mov dword ptr [esi + 0x44], edi		// 0x0002b5f1
L2b5f4:
		cmp dword ptr [esi + 0x48], edi		// 0x0002b5f4
		je L2b60c		// 0x0002b5f7
		call nxHullCallAllocator		// 0x0002b5f9
		mov ecx, dword ptr [esi + 0x48]		// 0x0002b5fe
		mov edx, dword ptr [eax]		// 0x0002b601
		push ecx		// 0x0002b603
		mov ecx, eax		// 0x0002b604
		call dword ptr [edx + 0xc]		// 0x0002b606
		mov dword ptr [esi + 0x48], edi		// 0x0002b609
L2b60c:
		cmp dword ptr [esi + 0x40], edi		// 0x0002b60c
		je L2b624		// 0x0002b60f
		call nxHullCallAllocator		// 0x0002b611
		mov ecx, dword ptr [esi + 0x40]		// 0x0002b616
		mov edx, dword ptr [eax]		// 0x0002b619
		push ecx		// 0x0002b61b
		mov ecx, eax		// 0x0002b61c
		call dword ptr [edx + 0xc]		// 0x0002b61e
		mov dword ptr [esi + 0x40], edi		// 0x0002b621
L2b624:
		mov ebx, dword ptr [esi + 0x3c]		// 0x0002b624
		cmp ebx, edi		// 0x0002b627
		je L2b63e		// 0x0002b629
		call nxHullCallAllocator		// 0x0002b62b
		mov edx, dword ptr [eax]		// 0x0002b630
		add ebx, -4		// 0x0002b632
		push ebx		// 0x0002b635
		mov ecx, eax		// 0x0002b636
		call dword ptr [edx + 0xc]		// 0x0002b638
		mov dword ptr [esi + 0x3c], edi		// 0x0002b63b
L2b63e:
		mov ebx, dword ptr [esi + 0x34]		// 0x0002b63e
		cmp ebx, edi		// 0x0002b641
		je L2b658		// 0x0002b643
		mov ecx, ebx		// 0x0002b645
		call nxHullCallContainerDtor		// 0x0002b647
		push ebx		// 0x0002b64c
		call nxHullCallDelete		// 0x0002b64d
		add esp, 4		// 0x0002b652
		mov dword ptr [esi + 0x34], edi		// 0x0002b655
L2b658:
		cmp dword ptr [esi + 0x30], edi		// 0x0002b658
		je L2b670		// 0x0002b65b
		call nxHullCallAllocator		// 0x0002b65d
		mov ecx, dword ptr [esi + 0x30]		// 0x0002b662
		mov edx, dword ptr [eax]		// 0x0002b665
		push ecx		// 0x0002b667
		mov ecx, eax		// 0x0002b668
		call dword ptr [edx + 0xc]		// 0x0002b66a
		mov dword ptr [esi + 0x30], edi		// 0x0002b66d
L2b670:
		cmp dword ptr [esi + 0x2c], edi		// 0x0002b670
		je L2b688		// 0x0002b673
		call nxHullCallAllocator		// 0x0002b675
		mov ecx, dword ptr [esi + 0x2c]		// 0x0002b67a
		mov edx, dword ptr [eax]		// 0x0002b67d
		push ecx		// 0x0002b67f
		mov ecx, eax		// 0x0002b680
		call dword ptr [edx + 0xc]		// 0x0002b682
		mov dword ptr [esi + 0x2c], edi		// 0x0002b685
L2b688:
		mov ebx, dword ptr [esi + 0x28]		// 0x0002b688
		cmp ebx, edi		// 0x0002b68b
		je L2b6a2		// 0x0002b68d
		call nxHullCallAllocator		// 0x0002b68f
		mov edx, dword ptr [eax]		// 0x0002b694
		add ebx, -4		// 0x0002b696
		push ebx		// 0x0002b699
		mov ecx, eax		// 0x0002b69a
		call dword ptr [edx + 0xc]		// 0x0002b69c
		mov dword ptr [esi + 0x28], edi		// 0x0002b69f
L2b6a2:
		mov ebx, dword ptr [esi + 8]		// 0x0002b6a2
		cmp ebx, edi		// 0x0002b6a5
		je L2b6bc		// 0x0002b6a7
		call nxHullCallAllocator		// 0x0002b6a9
		mov edx, dword ptr [eax]		// 0x0002b6ae
		add ebx, -4		// 0x0002b6b0
		push ebx		// 0x0002b6b3
		mov ecx, eax		// 0x0002b6b4
		call dword ptr [edx + 0xc]		// 0x0002b6b6
		mov dword ptr [esi + 8], edi		// 0x0002b6b9
L2b6bc:
		cmp dword ptr [esi + 0x14], edi		// 0x0002b6bc
		je L2b6d4		// 0x0002b6bf
		call nxHullCallAllocator		// 0x0002b6c1
		mov ecx, dword ptr [esi + 0x14]		// 0x0002b6c6
		mov edx, dword ptr [eax]		// 0x0002b6c9
		push ecx		// 0x0002b6cb
		mov ecx, eax		// 0x0002b6cc
		call dword ptr [edx + 0xc]		// 0x0002b6ce
		mov dword ptr [esi + 0x14], edi		// 0x0002b6d1
L2b6d4:
		cmp dword ptr [esi + 0x10], edi		// 0x0002b6d4
		je L2b6ec		// 0x0002b6d7
		call nxHullCallAllocator		// 0x0002b6d9
		mov ecx, dword ptr [esi + 0x10]		// 0x0002b6de
		mov edx, dword ptr [eax]		// 0x0002b6e1
		push ecx		// 0x0002b6e3
		mov ecx, eax		// 0x0002b6e4
		call dword ptr [edx + 0xc]		// 0x0002b6e6
		mov dword ptr [esi + 0x10], edi		// 0x0002b6e9
L2b6ec:
		pop edi		// 0x0002b6ec
		pop esi		// 0x0002b6ed
		pop ebx		// 0x0002b6ee
		ret		// 0x0002b6ef
		}
	}

// phys_fn_001474 (0x0002b990, 34 B), 13 instructions
__declspec(naked) void nxHullScalarDelete()
	{
	__asm
		{
		push esi		// 0x0002b990
		mov esi, ecx		// 0x0002b991
		call nxHullRelease		// 0x0002b993
		test byte ptr [esp + 8], 1		// 0x0002b998
		je L2b9ac		// 0x0002b99d
		call nxHullCallAllocator		// 0x0002b99f
		mov edx, dword ptr [eax]		// 0x0002b9a4
		push esi		// 0x0002b9a6
		mov ecx, eax		// 0x0002b9a7
		call dword ptr [edx + 0xc]		// 0x0002b9a9
L2b9ac:
		mov eax, esi		// 0x0002b9ac
		pop esi		// 0x0002b9ae
		ret 4		// 0x0002b9af
		}
	}

// phys_fn_001476, phys_fn_001478, phys_fn_001480, phys_fn_001482, phys_fn_001484, phys_fn_001486, phys_fn_001488, phys_fn_001490, phys_fn_001492 (0x0002b9c0, 3418 B), 1204 instructions
__declspec(naked) void nxHullBuild()
	{
	__asm
		{
		push ebp		// 0x0002b9c0
		lea ebp, [esp - 0x74]		// 0x0002b9c1
		sub esp, 0x100		// 0x0002b9c5
		push ebx		// 0x0002b9cb
		push esi		// 0x0002b9cc
		mov esi, dword ptr [ebp + 0x7c]		// 0x0002b9cd
		mov eax, dword ptr [esi]		// 0x0002b9d0
		push edi		// 0x0002b9d2
		mov ebx, ecx		// 0x0002b9d3
		mov ecx, dword ptr [esi + 4]		// 0x0002b9d5
		push eax		// 0x0002b9d8
		push ecx		// 0x0002b9d9
		lea ecx, [ebp - 0x7c]		// 0x0002b9da
		mov dword ptr [ebp + 0x60], ebx		// 0x0002b9dd
		call nxHullCallReducedCtor		// 0x0002b9e0
		push 0		// 0x0002b9e5
		lea ecx, [ebp - 0x7c]		// 0x0002b9e7
		call nxHullCallReduce		// 0x0002b9ea
		test al, al		// 0x0002b9ef
		je L2c453		// 0x0002b9f1
		mov edx, dword ptr [esi]		// 0x0002b9f7
		mov eax, dword ptr [esi + 4]		// 0x0002b9f9
		mov ecx, dword ptr [esi + 8]		// 0x0002b9fc
		mov edi, dword ptr [ebp - 0x74]		// 0x0002b9ff
		mov dword ptr [ebp + 8], eax		// 0x0002ba02
		mov eax, dword ptr [ebp - 0x70]		// 0x0002ba05
		mov dword ptr [ebp + 4], edx		// 0x0002ba08
		mov edx, dword ptr [esi + 0xc]		// 0x0002ba0b
		mov dword ptr [ebp + 0xc], ecx		// 0x0002ba0e
		mov dword ptr [ebp + 0x10], edx		// 0x0002ba11
		mov dword ptr [ebp + 8], eax		// 0x0002ba14
		mov dword ptr [ebp + 0x70], edi		// 0x0002ba17
		mov dword ptr [ebp + 0x28], 0		// 0x0002ba1a
		call nxHullCallAllocator		// 0x0002ba21
		mov edx, dword ptr [eax]		// 0x0002ba26
		lea ecx, [edi + 4]		// 0x0002ba28
		shl ecx, 2		// 0x0002ba2b
		push 1		// 0x0002ba2e
		push ecx		// 0x0002ba30
		mov ecx, eax		// 0x0002ba31
		call dword ptr [edx]		// 0x0002ba33
		mov esi, eax		// 0x0002ba35
		lea eax, [edi + 4]		// 0x0002ba37
		test eax, eax		// 0x0002ba3a
		mov dword ptr [ebp + 0x1c], esi		// 0x0002ba3c
		mov dword ptr [ebp + 0x7c], 0		// 0x0002ba3f
		jle L2ba68		// 0x0002ba46
L2ba48:
		call nxHullCallAllocator		// 0x0002ba48
		mov edx, dword ptr [eax]		// 0x0002ba4d
		push 1		// 0x0002ba4f
		push 0x18		// 0x0002ba51
		mov ecx, eax		// 0x0002ba53
		call dword ptr [edx]		// 0x0002ba55
		mov ecx, dword ptr [ebp + 0x7c]		// 0x0002ba57
		mov dword ptr [esi + ecx*4], eax		// 0x0002ba5a
		inc ecx		// 0x0002ba5d
		lea eax, [edi + 4]		// 0x0002ba5e
		cmp ecx, eax		// 0x0002ba61
		mov dword ptr [ebp + 0x7c], ecx		// 0x0002ba63
		jl L2ba48		// 0x0002ba66
L2ba68:
		xor eax, eax		// 0x0002ba68
		test edi, edi		// 0x0002ba6a
		jle L2ba96		// 0x0002ba6c
		mov ecx, dword ptr [ebp + 8]		// 0x0002ba6e
		add ecx, 8		// 0x0002ba71
L2ba74:
		mov edx, dword ptr [esi + eax*4]		// 0x0002ba74
		fld dword ptr [ecx - 8]		// 0x0002ba77
		fstp qword ptr [edx]		// 0x0002ba7a
		mov edx, dword ptr [esi + eax*4]		// 0x0002ba7c
		fld dword ptr [ecx - 4]		// 0x0002ba7f
		inc eax		// 0x0002ba82
		fstp qword ptr [edx + 8]		// 0x0002ba83
		mov edx, dword ptr [esi + eax*4 - 4]		// 0x0002ba86
		fld dword ptr [ecx]		// 0x0002ba8a
		add ecx, 0xc		// 0x0002ba8c
		cmp eax, edi		// 0x0002ba8f
		fstp qword ptr [edx + 0x10]		// 0x0002ba91
		jl L2ba74		// 0x0002ba94
L2ba96:
		mov eax, dword ptr [esi]		// 0x0002ba96
		fld qword ptr [eax]		// 0x0002ba98
		mov ecx, 1		// 0x0002ba9a
		cmp edi, ecx		// 0x0002ba9f
		fst qword ptr [ebp + 0x4c]		// 0x0002baa1
		fstp qword ptr [ebp + 0x58]		// 0x0002baa4
		fld qword ptr [eax + 8]		// 0x0002baa7
		fst qword ptr [ebp + 0x64]		// 0x0002baaa
		fstp qword ptr [ebp + 0x14]		// 0x0002baad
		fld qword ptr [eax + 0x10]		// 0x0002bab0
		fst qword ptr [ebp + 0x44]		// 0x0002bab3
		fstp qword ptr [ebp + 0x20]		// 0x0002bab6
		jle L2bb36		// 0x0002bab9
		jmp L2bac0		// 0x0002babb
		lea ecx, [ecx]		// 0x0002babd
L2bac0:
		mov edx, dword ptr [esi + ecx*4]		// 0x0002bac0
		fld qword ptr [edx]		// 0x0002bac3
		fld qword ptr [ebp + 0x58]		// 0x0002bac5
		fcomp st(1)		// 0x0002bac8
		fnstsw ax		// 0x0002baca
		test ah, 5		// 0x0002bacc
		jp L2bad4		// 0x0002bacf
		fst qword ptr [ebp + 0x58]		// 0x0002bad1
L2bad4:
		fld qword ptr [ebp + 0x4c]		// 0x0002bad4
		fcomp st(1)		// 0x0002bad7
		fnstsw ax		// 0x0002bad9
		test ah, 0x41		// 0x0002badb
		jne L2bae5		// 0x0002bade
		fstp qword ptr [ebp + 0x4c]		// 0x0002bae0
		jmp L2bae7		// 0x0002bae3
L2bae5:
		fstp st(0)		// 0x0002bae5
L2bae7:
		fld qword ptr [edx + 8]		// 0x0002bae7
		fld qword ptr [ebp + 0x14]		// 0x0002baea
		fcomp st(1)		// 0x0002baed
		fnstsw ax		// 0x0002baef
		test ah, 5		// 0x0002baf1
		jp L2baf9		// 0x0002baf4
		fst qword ptr [ebp + 0x14]		// 0x0002baf6
L2baf9:
		fld qword ptr [ebp + 0x64]		// 0x0002baf9
		fcomp st(1)		// 0x0002bafc
		fnstsw ax		// 0x0002bafe
		test ah, 0x41		// 0x0002bb00
		jne L2bb0a		// 0x0002bb03
		fstp qword ptr [ebp + 0x64]		// 0x0002bb05
		jmp L2bb0c		// 0x0002bb08
L2bb0a:
		fstp st(0)		// 0x0002bb0a
L2bb0c:
		fld qword ptr [edx + 0x10]		// 0x0002bb0c
		fld qword ptr [ebp + 0x20]		// 0x0002bb0f
		fcomp st(1)		// 0x0002bb12
		fnstsw ax		// 0x0002bb14
		test ah, 5		// 0x0002bb16
		jp L2bb1e		// 0x0002bb19
		fst qword ptr [ebp + 0x20]		// 0x0002bb1b
L2bb1e:
		fld qword ptr [ebp + 0x44]		// 0x0002bb1e
		fcomp st(1)		// 0x0002bb21
		fnstsw ax		// 0x0002bb23
		test ah, 0x41		// 0x0002bb25
		jne L2bb2f		// 0x0002bb28
		fstp qword ptr [ebp + 0x44]		// 0x0002bb2a
		jmp L2bb31		// 0x0002bb2d
L2bb2f:
		fstp st(0)		// 0x0002bb2f
L2bb31:
		inc ecx		// 0x0002bb31
		cmp ecx, edi		// 0x0002bb32
		jl L2bac0		// 0x0002bb34
L2bb36:
		fld qword ptr [ebp + 0x58]		// 0x0002bb36
		fsub qword ptr [ebp + 0x4c]		// 0x0002bb39
		fld qword ptr [ebp + 0x14]		// 0x0002bb3c
		fsub qword ptr [ebp + 0x64]		// 0x0002bb3f
		fld qword ptr [ebp + 0x20]		// 0x0002bb42
		fsub qword ptr [ebp + 0x44]		// 0x0002bb45
		fld st(2)		// 0x0002bb48
		fld st(3)		// 0x0002bb4a
		fcomp st(3)		// 0x0002bb4c
		fnstsw ax		// 0x0002bb4e
		test ah, 5		// 0x0002bb50
		jp L2bb59		// 0x0002bb53
		fstp st(0)		// 0x0002bb55
		fld st(1)		// 0x0002bb57
L2bb59:
		fcom st(1)		// 0x0002bb59
		fnstsw ax		// 0x0002bb5b
		test ah, 5		// 0x0002bb5d
		jp L2bb66		// 0x0002bb60
		fstp st(0)		// 0x0002bb62
		fld st(0)		// 0x0002bb64
L2bb66:
		fmul qword ptr [nxHullK10107718_8]		// 0x0002bb66
		xor ecx, ecx		// 0x0002bb6c
		test edi, edi		// 0x0002bb6e
		mov eax, 0x16f		// 0x0002bb70
		jle L2bc38		// 0x0002bb75
		jmp L2bb80		// 0x0002bb7b
		lea ecx, [ecx]		// 0x0002bb7d
L2bb80:
		mov edx, dword ptr [esi + ecx*4]		// 0x0002bb80
		imul eax, eax, 0x7ff8a3ed		// 0x0002bb83
		add eax, 0x2aa01d31		// 0x0002bb89
		mov edi, eax		// 0x0002bb8e
		and edi, 0xffff		// 0x0002bb90
		test edi, edi		// 0x0002bb96
		mov dword ptr [ebp + 0x7c], edi		// 0x0002bb98
		fild dword ptr [ebp + 0x7c]		// 0x0002bb9b
		jge L2bba6		// 0x0002bb9e
		fadd qword ptr [nxHullK10107710_8]		// 0x0002bba0
L2bba6:
		fmul qword ptr [nxHullK10107708_8]		// 0x0002bba6
		imul eax, eax, 0x7ff8a3ed		// 0x0002bbac
		fsubr qword ptr [nxHullK10107700_8]		// 0x0002bbb2
		add eax, 0x2aa01d31		// 0x0002bbb8
		fmul st(0), st(1)		// 0x0002bbbd
		mov edi, eax		// 0x0002bbbf
		and edi, 0xffff		// 0x0002bbc1
		test edi, edi		// 0x0002bbc7
		fadd qword ptr [edx]		// 0x0002bbc9
		mov dword ptr [ebp + 0x7c], edi		// 0x0002bbcb
		fstp qword ptr [edx]		// 0x0002bbce
		mov edx, dword ptr [esi + ecx*4]		// 0x0002bbd0
		fild dword ptr [ebp + 0x7c]		// 0x0002bbd3
		jge L2bbde		// 0x0002bbd6
		fadd qword ptr [nxHullK10107710_8]		// 0x0002bbd8
L2bbde:
		fmul qword ptr [nxHullK10107708_8]		// 0x0002bbde
		imul eax, eax, 0x7ff8a3ed		// 0x0002bbe4
		fsubr qword ptr [nxHullK10107700_8]		// 0x0002bbea
		add eax, 0x2aa01d31		// 0x0002bbf0
		fmul st(0), st(1)		// 0x0002bbf5
		mov edi, eax		// 0x0002bbf7
		and edi, 0xffff		// 0x0002bbf9
		test edi, edi		// 0x0002bbff
		fadd qword ptr [edx + 8]		// 0x0002bc01
		mov dword ptr [ebp + 0x7c], edi		// 0x0002bc04
		fstp qword ptr [edx + 8]		// 0x0002bc07
		mov edx, dword ptr [esi + ecx*4]		// 0x0002bc0a
		fild dword ptr [ebp + 0x7c]		// 0x0002bc0d
		jge L2bc18		// 0x0002bc10
		fadd qword ptr [nxHullK10107710_8]		// 0x0002bc12
L2bc18:
		fmul qword ptr [nxHullK10107708_8]		// 0x0002bc18
		mov edi, dword ptr [ebp + 0x70]		// 0x0002bc1e
		inc ecx		// 0x0002bc21
		cmp ecx, edi		// 0x0002bc22
		fsubr qword ptr [nxHullK10107700_8]		// 0x0002bc24
		fmul st(0), st(1)		// 0x0002bc2a
		fadd qword ptr [edx + 0x10]		// 0x0002bc2c
		fstp qword ptr [edx + 0x10]		// 0x0002bc2f
		jl L2bb80		// 0x0002bc32
L2bc38:
		fstp st(0)		// 0x0002bc38
		xor eax, eax		// 0x0002bc3a
		fld qword ptr [nxHullK101076f8_8]		// 0x0002bc3c
		lea esi, [esi + edi*4]		// 0x0002bc42
		fstp qword ptr [ebp - 0x68]		// 0x0002bc45
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bc48
		fstp qword ptr [ebp - 0x60]		// 0x0002bc4e
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bc51
		fstp qword ptr [ebp - 0x58]		// 0x0002bc57
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bc5a
		fstp qword ptr [ebp - 0x50]		// 0x0002bc60
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bc63
		fstp qword ptr [ebp - 0x48]		// 0x0002bc69
		fld qword ptr [nxHullK101076f8_8]		// 0x0002bc6c
		fstp qword ptr [ebp - 0x40]		// 0x0002bc72
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bc75
		fstp qword ptr [ebp - 0x38]		// 0x0002bc7b
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bc7e
		fstp qword ptr [ebp - 0x30]		// 0x0002bc84
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bc87
		fstp qword ptr [ebp - 0x28]		// 0x0002bc8d
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bc90
		fstp qword ptr [ebp - 0x20]		// 0x0002bc96
		fld qword ptr [nxHullK101076f8_8]		// 0x0002bc99
		fstp qword ptr [ebp - 0x18]		// 0x0002bc9f
		fld qword ptr [nxHullK101076f0_8]		// 0x0002bca2
		fstp qword ptr [ebp - 0x10]		// 0x0002bca8
		jmp L2bcb0		// 0x0002bcab
		lea ecx, [ecx]		// 0x0002bcad
L2bcb0:
		mov ecx, dword ptr [esi]		// 0x0002bcb0
		fld st(2)		// 0x0002bcb2
		fmul qword ptr [ebp + eax*8 - 0x68]		// 0x0002bcb4
		inc eax		// 0x0002bcb8
		add esi, 4		// 0x0002bcb9
		cmp eax, 4		// 0x0002bcbc
		fadd qword ptr [ebp + 0x4c]		// 0x0002bcbf
		fstp qword ptr [ecx]		// 0x0002bcc2
		mov edx, dword ptr [esi - 4]		// 0x0002bcc4
		fld st(1)		// 0x0002bcc7
		fmul qword ptr [ebp + eax*8 - 0x50]		// 0x0002bcc9
		fadd qword ptr [ebp + 0x64]		// 0x0002bccd
		fstp qword ptr [edx + 8]		// 0x0002bcd0
		mov ecx, dword ptr [esi - 4]		// 0x0002bcd3
		fld st(0)		// 0x0002bcd6
		fmul qword ptr [ebp + eax*8 - 0x30]		// 0x0002bcd8
		fadd qword ptr [ebp + 0x44]		// 0x0002bcdc
		fstp qword ptr [ecx + 0x10]		// 0x0002bcdf
		jl L2bcb0		// 0x0002bce2
		fstp st(0)		// 0x0002bce4
		fstp st(0)		// 0x0002bce6
		fstp st(0)		// 0x0002bce8
		call nxHullCallAllocator		// 0x0002bcea
		mov edx, dword ptr [eax]		// 0x0002bcef
		push 1		// 0x0002bcf1
		push 0x70c		// 0x0002bcf3
		mov ecx, eax		// 0x0002bcf8
		call dword ptr [edx]		// 0x0002bcfa
		mov esi, eax		// 0x0002bcfc
		mov dword ptr [ebp + 0x24], esi		// 0x0002bcfe
		call nxHullCallAllocator		// 0x0002bd01
		mov edx, dword ptr [eax]		// 0x0002bd06
		push 1		// 0x0002bd08
		push 0x1524		// 0x0002bd0a
		mov ecx, eax		// 0x0002bd0f
		call dword ptr [edx]		// 0x0002bd11
		mov dword ptr [esi], eax		// 0x0002bd13
		mov eax, 0xc		// 0x0002bd15
		lea ecx, [esi + 4]		// 0x0002bd1a
		lea ecx, [ecx]		// 0x0002bd1d
L2bd20:
		mov edx, dword ptr [esi]		// 0x0002bd20
		add edx, eax		// 0x0002bd22
		mov dword ptr [ecx], edx		// 0x0002bd24
		add eax, 0xc		// 0x0002bd26
		add ecx, 4		// 0x0002bd29
		cmp eax, 0x1524		// 0x0002bd2c
		jl L2bd20		// 0x0002bd31
		lea edi, [edi + edi*2 + 0x12]		// 0x0002bd33
		shl edi, 1		// 0x0002bd37
		call nxHullCallAllocator		// 0x0002bd39
		mov edx, dword ptr [eax]		// 0x0002bd3e
		lea ecx, [edi*4]		// 0x0002bd40
		push 1		// 0x0002bd47
		push ecx		// 0x0002bd49
		mov ecx, eax		// 0x0002bd4a
		call dword ptr [edx]		// 0x0002bd4c
		xor ecx, ecx		// 0x0002bd4e
		test edi, edi		// 0x0002bd50
		mov dword ptr [ebp + 0x48], eax		// 0x0002bd52
		jle L2bd5f		// 0x0002bd55
L2bd57:
		mov dword ptr [eax + ecx*4], ecx		// 0x0002bd57
		inc ecx		// 0x0002bd5a
		cmp ecx, edi		// 0x0002bd5b
		jl L2bd57		// 0x0002bd5d
L2bd5f:
		call nxHullCallAllocator		// 0x0002bd5f
		mov edx, dword ptr [eax]		// 0x0002bd64
		push 1		// 0x0002bd66
		lea ecx, [edi*4]		// 0x0002bd68
		push ecx		// 0x0002bd6f
		mov ecx, eax		// 0x0002bd70
		call dword ptr [edx]		// 0x0002bd72
		mov esi, eax		// 0x0002bd74
		mov dword ptr [ebp + 0x18], esi		// 0x0002bd76
		call nxHullCallAllocator		// 0x0002bd79
		mov edx, dword ptr [eax]		// 0x0002bd7e
		mov ecx, edi		// 0x0002bd80
		shl ecx, 4		// 0x0002bd82
		push 1		// 0x0002bd85
		push ecx		// 0x0002bd87
		mov ecx, eax		// 0x0002bd88
		call dword ptr [edx]		// 0x0002bd8a
		mov dword ptr [esi], eax		// 0x0002bd8c
		mov eax, 1		// 0x0002bd8e
		cmp edi, eax		// 0x0002bd93
		jle L2bdaf		// 0x0002bd95
		mov ecx, 0x10		// 0x0002bd97
		lea esp, [esp]		// 0x0002bd9c
L2bda0:
		mov edx, dword ptr [esi]		// 0x0002bda0
		add edx, ecx		// 0x0002bda2
		mov dword ptr [esi + eax*4], edx		// 0x0002bda4
		inc eax		// 0x0002bda7
		add ecx, 0x10		// 0x0002bda8
		cmp eax, edi		// 0x0002bdab
		jl L2bda0		// 0x0002bdad
L2bdaf:
		mov ecx, dword ptr [esi]		// 0x0002bdaf
		mov eax, dword ptr [ebp + 0x70]		// 0x0002bdb1
		mov dword ptr [ecx], eax		// 0x0002bdb4
		mov ecx, dword ptr [esi]		// 0x0002bdb6
		lea edx, [eax + 1]		// 0x0002bdb8
		mov dword ptr [ecx + 4], edx		// 0x0002bdbb
		mov ecx, dword ptr [esi]		// 0x0002bdbe
		lea edx, [eax + 2]		// 0x0002bdc0
		mov dword ptr [ecx + 8], edx		// 0x0002bdc3
		mov edx, dword ptr [esi]		// 0x0002bdc6
		add eax, 3		// 0x0002bdc8
		mov dword ptr [edx + 0xc], eax		// 0x0002bdcb
		call nxHullCallAllocator		// 0x0002bdce
		mov edx, dword ptr [eax]		// 0x0002bdd3
		push 1		// 0x0002bdd5
		lea ecx, [edi*4]		// 0x0002bdd7
		push ecx		// 0x0002bdde
		mov ecx, eax		// 0x0002bddf
		call dword ptr [edx]		// 0x0002bde1
		mov dword ptr [ebp + 0x6c], eax		// 0x0002bde3
		call nxHullCallAllocator		// 0x0002bde6
		mov edx, dword ptr [eax]		// 0x0002bdeb
		mov ecx, edi		// 0x0002bded
		shl ecx, 5		// 0x0002bdef
		push 1		// 0x0002bdf2
		push ecx		// 0x0002bdf4
		mov ecx, eax		// 0x0002bdf5
		call dword ptr [edx]		// 0x0002bdf7
		cmp edi, 1		// 0x0002bdf9
		mov edx, dword ptr [ebp + 0x6c]		// 0x0002bdfc
		mov dword ptr [edx], eax		// 0x0002bdff
		mov dword ptr [ebp + 0x54], 1		// 0x0002be01
		mov eax, edx		// 0x0002be08
		jle L2be31		// 0x0002be0a
		mov dword ptr [ebp + 0x7c], 0x20		// 0x0002be0c
L2be13:
		mov ecx, dword ptr [ebp + 0x7c]		// 0x0002be13
		mov edx, dword ptr [eax]		// 0x0002be16
		add edx, ecx		// 0x0002be18
		mov ecx, dword ptr [ebp + 0x54]		// 0x0002be1a
		mov dword ptr [eax + ecx*4], edx		// 0x0002be1d
		mov edx, dword ptr [ebp + 0x7c]		// 0x0002be20
		inc ecx		// 0x0002be23
		add edx, 0x20		// 0x0002be24
		cmp ecx, edi		// 0x0002be27
		mov dword ptr [ebp + 0x54], ecx		// 0x0002be29
		mov dword ptr [ebp + 0x7c], edx		// 0x0002be2c
		jl L2be13		// 0x0002be2f
L2be31:
		mov ecx, dword ptr [eax]		// 0x0002be31
		fld qword ptr [nxHullK101076e8_8]		// 0x0002be33
		fstp qword ptr [ecx]		// 0x0002be39
		mov edx, dword ptr [eax]		// 0x0002be3b
		fld qword ptr [nxHullK101076e8_8]		// 0x0002be3d
		mov dword ptr [ebp + 0x54], 0		// 0x0002be43
		fstp qword ptr [edx + 8]		// 0x0002be4a
		mov ecx, dword ptr [eax]		// 0x0002be4d
		fld qword ptr [nxHullK101076e8_8]		// 0x0002be4f
		fstp qword ptr [ecx + 0x10]		// 0x0002be55
		mov edx, dword ptr [eax]		// 0x0002be58
		fld qword ptr [nxHullK101076e0_8]		// 0x0002be5a
		mov ecx, 1		// 0x0002be60
		fstp qword ptr [edx + 0x18]		// 0x0002be65
		mov edx, dword ptr [ebp + 0x70]		// 0x0002be68
		test edx, edx		// 0x0002be6b
		mov dword ptr [ebp + 0x40], ecx		// 0x0002be6d
		mov dword ptr [ebp + 0x2c], ecx		// 0x0002be70
		jle L2c28a		// 0x0002be73
		mov eax, dword ptr [ebp + 0x1c]		// 0x0002be79
		mov dword ptr [ebp + 0x30], eax		// 0x0002be7c
		nop		// 0x0002be7f
L2be80:
		mov ecx, dword ptr [ebp + 0x40]		// 0x0002be80
		xor eax, eax		// 0x0002be83
		or edi, 0xffffffff		// 0x0002be85
		cmp ecx, eax		// 0x0002be88
		mov dword ptr [ebp + 0x7c], edi		// 0x0002be8a
		mov dword ptr [ebp + 0x34], eax		// 0x0002be8d
		mov dword ptr [ebp + 0x68], eax		// 0x0002be90
		jle L2c25f		// 0x0002be93
		lea esp, [esp]		// 0x0002be99
L2bea0:
		mov ecx, dword ptr [esi + edi*4 + 4]		// 0x0002bea0
		mov eax, dword ptr [ecx]		// 0x0002bea4
		inc edi		// 0x0002bea6
		test eax, eax		// 0x0002bea7
		jge L2bebb		// 0x0002bea9
		jmp L2beb0		// 0x0002beab
		lea ecx, [ecx]		// 0x0002bead
L2beb0:
		mov edx, dword ptr [esi + edi*4 + 4]		// 0x0002beb0
		mov eax, dword ptr [edx]		// 0x0002beb4
		inc edi		// 0x0002beb6
		test eax, eax		// 0x0002beb7
		jl L2beb0		// 0x0002beb9
L2bebb:
		mov eax, dword ptr [ebp + 0x6c]		// 0x0002bebb
		mov eax, dword ptr [eax + edi*4]		// 0x0002bebe
		mov ebx, dword ptr [ebp + 0x30]		// 0x0002bec1
		fld qword ptr [eax + 0x18]		// 0x0002bec4
		mov ebx, dword ptr [ebx]		// 0x0002bec7
		xor edx, edx		// 0x0002bec9
		mov ecx, eax		// 0x0002becb
		sub ebx, eax		// 0x0002becd
		nop		// 0x0002becf
L2bed0:
		fld qword ptr [ebx + ecx]		// 0x0002bed0
		fsub qword ptr [ecx]		// 0x0002bed3
		fld st(0)		// 0x0002bed5
		fmul st(0), st(1)		// 0x0002bed7
		fsubp st(2), st(0)		// 0x0002bed9
		fstp st(0)		// 0x0002bedb
		fcom qword ptr [nxHullK101076e8_8]		// 0x0002bedd
		fnstsw ax		// 0x0002bee3
		test ah, 5		// 0x0002bee5
		jnp L2c022		// 0x0002bee8
		inc edx		// 0x0002beee
		add ecx, 8		// 0x0002beef
		cmp edx, 3		// 0x0002bef2
		jl L2bed0		// 0x0002bef5
		mov ebx, dword ptr [ebp + 0x34]		// 0x0002bef7
		fstp st(0)		// 0x0002befa
		mov eax, dword ptr [ebp + 0x2c]		// 0x0002befc
		mov ecx, dword ptr [ebp + 0x48]		// 0x0002beff
		dec ebx		// 0x0002bf02
		dec eax		// 0x0002bf03
		mov dword ptr [ecx + eax*4], edi		// 0x0002bf04
		mov ecx, dword ptr [ebp + 0x24]		// 0x0002bf07
		mov dword ptr [ebp + 0x34], ebx		// 0x0002bf0a
		mov ebx, dword ptr [ebp + 0x7c]		// 0x0002bf0d
		mov dword ptr [ebp + 0x2c], eax		// 0x0002bf10
		xor eax, eax		// 0x0002bf13
		lea edx, [ebp - 8]		// 0x0002bf15
		sub edx, 4		// 0x0002bf18
		lea ecx, [ecx + ebx*4]		// 0x0002bf1b
		mov dword ptr [ebp + 0x5c], eax		// 0x0002bf1e
		mov dword ptr [ebp + 0x50], edx		// 0x0002bf21
		mov dword ptr [ebp + 0x3c], ecx		// 0x0002bf24
		jmp L2bf30		// 0x0002bf27
L2bf29:
		mov edx, dword ptr [ebp + 0x50]		// 0x0002bf29
		lea esp, [esp]		// 0x0002bf2c
L2bf30:
		test eax, eax		// 0x0002bf30
		mov dword ptr [ebp - 8], 0		// 0x0002bf32
		jne L2bf42		// 0x0002bf39
		mov dword ptr [ebp - 8], 1		// 0x0002bf3b
L2bf42:
		mov ecx, 1		// 0x0002bf42
L2bf47:
		mov eax, dword ptr [edx + ecx*4]		// 0x0002bf47
		mov ebx, dword ptr [ebp + 0x5c]		// 0x0002bf4a
		inc eax		// 0x0002bf4d
		cmp eax, ebx		// 0x0002bf4e
		mov dword ptr [ebp + ecx*4 - 8], eax		// 0x0002bf50
		jne L2bf5b		// 0x0002bf54
		inc eax		// 0x0002bf56
		mov dword ptr [ebp + ecx*4 - 8], eax		// 0x0002bf57
L2bf5b:
		inc ecx		// 0x0002bf5b
		cmp ecx, 3		// 0x0002bf5c
		jl L2bf47		// 0x0002bf5f
		mov eax, dword ptr [ebp + 0x7c]		// 0x0002bf61
		cmp eax, 2		// 0x0002bf64
		jle L2bf9c		// 0x0002bf67
		xor ecx, ecx		// 0x0002bf69
		test eax, eax		// 0x0002bf6b
		jl L2bf9c		// 0x0002bf6d
		nop		// 0x0002bf6f
L2bf70:
		xor eax, eax		// 0x0002bf70
L2bf72:
		cmp eax, 0xc		// 0x0002bf72
		jge L2bff5		// 0x0002bf75
		mov edx, dword ptr [ebp + 0x24]		// 0x0002bf77
		mov ebx, dword ptr [ebp + eax - 8]		// 0x0002bf7a
		mov esi, dword ptr [esi + edi*4]		// 0x0002bf7e
		mov esi, dword ptr [esi + ebx*4]		// 0x0002bf81
		mov edx, dword ptr [edx + ecx*4]		// 0x0002bf84
		cmp esi, dword ptr [eax + edx]		// 0x0002bf87
		mov esi, dword ptr [ebp + 0x18]		// 0x0002bf8a
		jne L2bf94		// 0x0002bf8d
		add eax, 4		// 0x0002bf8f
		jmp L2bf72		// 0x0002bf92
L2bf94:
		mov eax, dword ptr [ebp + 0x7c]		// 0x0002bf94
		inc ecx		// 0x0002bf97
		cmp ecx, eax		// 0x0002bf98
		jle L2bf70		// 0x0002bf9a
L2bf9c:
		mov eax, dword ptr [ebp + 0x7c]		// 0x0002bf9c
		mov edx, dword ptr [ebp + 0x3c]		// 0x0002bf9f
		mov ebx, dword ptr [ebp + 0x60]		// 0x0002bfa2
		inc eax		// 0x0002bfa5
		add edx, 4		// 0x0002bfa6
		cmp eax, 0x1c2		// 0x0002bfa9
		mov dword ptr [ebp + 0x7c], eax		// 0x0002bfae
		mov dword ptr [ebp + 0x3c], edx		// 0x0002bfb1
		jg L2c2e0		// 0x0002bfb4
		xor eax, eax		// 0x0002bfba
		lea esp, [esp]		// 0x0002bfbc
L2bfc0:
		mov ecx, dword ptr [ebp + eax - 8]		// 0x0002bfc0
		mov edx, dword ptr [esi + edi*4]		// 0x0002bfc4
		mov ebx, dword ptr [ebp + 0x3c]		// 0x0002bfc7
		mov ebx, dword ptr [ebx]		// 0x0002bfca
		mov ecx, dword ptr [edx + ecx*4]		// 0x0002bfcc
		mov dword ptr [eax + ebx], ecx		// 0x0002bfcf
		add eax, 4		// 0x0002bfd2
		cmp eax, 0xc		// 0x0002bfd5
		jl L2bfc0		// 0x0002bfd8
L2bfda:
		mov eax, dword ptr [ebp + 0x5c]		// 0x0002bfda
		inc eax		// 0x0002bfdd
		cmp eax, 4		// 0x0002bfde
		mov dword ptr [ebp + 0x5c], eax		// 0x0002bfe1
		jl L2bf29		// 0x0002bfe4
		mov edx, dword ptr [esi + edi*4]		// 0x0002bfea
		mov dword ptr [edx], 0xffffffff		// 0x0002bfed
		jmp L2c024		// 0x0002bff3
L2bff5:
		xor eax, eax		// 0x0002bff5
L2bff7:
		mov edx, dword ptr [ebp + 0x3c]		// 0x0002bff7
		mov edx, dword ptr [edx]		// 0x0002bffa
		mov ebx, dword ptr [ebp + 0x24]		// 0x0002bffc
		mov edx, dword ptr [eax + edx]		// 0x0002bfff
		mov ebx, dword ptr [ebx + ecx*4]		// 0x0002c002
		mov dword ptr [eax + ebx], edx		// 0x0002c005
		add eax, 4		// 0x0002c008
		cmp eax, 0xc		// 0x0002c00b
		jl L2bff7		// 0x0002c00e
		mov ecx, dword ptr [ebp + 0x7c]		// 0x0002c010
		mov eax, dword ptr [ebp + 0x3c]		// 0x0002c013
		dec ecx		// 0x0002c016
		sub eax, 4		// 0x0002c017
		mov dword ptr [ebp + 0x7c], ecx		// 0x0002c01a
		mov dword ptr [ebp + 0x3c], eax		// 0x0002c01d
		jmp L2bfda		// 0x0002c020
L2c022:
		fstp st(0)		// 0x0002c022
L2c024:
		mov eax, dword ptr [ebp + 0x68]		// 0x0002c024
		mov ecx, dword ptr [ebp + 0x40]		// 0x0002c027
		inc eax		// 0x0002c02a
		cmp eax, ecx		// 0x0002c02b
		mov dword ptr [ebp + 0x68], eax		// 0x0002c02d
		jl L2bea0		// 0x0002c030
		mov ecx, dword ptr [ebp + 0x7c]		// 0x0002c036
		test ecx, ecx		// 0x0002c039
		mov ebx, dword ptr [ebp + 0x60]		// 0x0002c03b
		jl L2c25f		// 0x0002c03e
		mov eax, dword ptr [ebp + 0x2c]		// 0x0002c044
		mov edx, dword ptr [ebp + 0x48]		// 0x0002c047
		lea edi, [edx + eax*4]		// 0x0002c04a
		mov eax, dword ptr [ebp + 0x24]		// 0x0002c04d
		inc ecx		// 0x0002c050
		mov dword ptr [ebp + 0x5c], eax		// 0x0002c051
		mov dword ptr [ebp + 0x50], ecx		// 0x0002c054
L2c057:
		mov ecx, dword ptr [eax]		// 0x0002c057
		mov edx, dword ptr [ecx]		// 0x0002c059
		cmp edx, dword ptr [ebp + 0x70]		// 0x0002c05b
		jl L2c24c		// 0x0002c05e
		mov ecx, dword ptr [ebp + 0x30]		// 0x0002c064
		mov edx, dword ptr [ecx]		// 0x0002c067
		mov eax, dword ptr [eax]		// 0x0002c069
		mov dword ptr [ebp + 0x68], edx		// 0x0002c06b
		lea ecx, [ebp - 0x50]		// 0x0002c06e
		mov dword ptr [ebp + 0x7c], eax		// 0x0002c071
		mov dword ptr [ebp + 0x3c], 3		// 0x0002c074
		jmp L2c080		// 0x0002c07b
		lea ecx, [ecx]		// 0x0002c07d
L2c080:
		mov edx, dword ptr [ebp + 0x7c]		// 0x0002c080
		fld qword ptr [nxHullK101076e8_8]		// 0x0002c083
		mov edx, dword ptr [edx]		// 0x0002c089
		fstp qword ptr [ecx]		// 0x0002c08b
		mov ebx, dword ptr [ebp + 0x1c]		// 0x0002c08d
		mov edx, dword ptr [ebx + edx*4]		// 0x0002c090
		mov eax, dword ptr [ebp + 0x68]		// 0x0002c093
		mov ebx, eax		// 0x0002c096
		sub edx, ebx		// 0x0002c098
		lea esi, [ecx - 0x18]		// 0x0002c09a
		mov ebx, 3		// 0x0002c09d
L2c0a2:
		fld qword ptr [eax + edx]		// 0x0002c0a2
		add esi, 8		// 0x0002c0a5
		fsub qword ptr [eax]		// 0x0002c0a8
		add eax, 8		// 0x0002c0aa
		dec ebx		// 0x0002c0ad
		fst qword ptr [esi - 8]		// 0x0002c0ae
		fld qword ptr [eax + edx - 8]		// 0x0002c0b1
		fadd qword ptr [eax - 8]		// 0x0002c0b5
		fmul st(0), st(1)		// 0x0002c0b8
		fmul qword ptr [nxHullK10107700_8]		// 0x0002c0ba
		fadd qword ptr [ecx]		// 0x0002c0c0
		fstp qword ptr [ecx]		// 0x0002c0c2
		fstp st(0)		// 0x0002c0c4
		jne L2c0a2		// 0x0002c0c6
		mov esi, dword ptr [ebp + 0x7c]		// 0x0002c0c8
		mov eax, dword ptr [ebp + 0x3c]		// 0x0002c0cb
		add esi, 4		// 0x0002c0ce
		add ecx, 0x20		// 0x0002c0d1
		dec eax		// 0x0002c0d4
		mov dword ptr [ebp + 0x7c], esi		// 0x0002c0d5
		mov dword ptr [ebp + 0x3c], eax		// 0x0002c0d8
		jne L2c080		// 0x0002c0db
		fld qword ptr [ebp - 0x18]		// 0x0002c0dd
		mov eax, dword ptr [edi]		// 0x0002c0e0
		fmul qword ptr [ebp - 0x40]		// 0x0002c0e2
		mov ecx, dword ptr [ebp + 0x6c]		// 0x0002c0e5
		fld qword ptr [ebp - 0x20]		// 0x0002c0e8
		mov edx, dword ptr [ecx + eax*4]		// 0x0002c0eb
		fmul qword ptr [ebp - 0x38]		// 0x0002c0ee
		fsubp st(1), st(0)		// 0x0002c0f1
		fstp qword ptr [ebp + 0x64]		// 0x0002c0f3
		fld qword ptr [ebp - 0x18]		// 0x0002c0f6
		fmul qword ptr [ebp - 0x48]		// 0x0002c0f9
		fld qword ptr [ebp - 0x28]		// 0x0002c0fc
		fmul qword ptr [ebp - 0x38]		// 0x0002c0ff
		fsubp st(1), st(0)		// 0x0002c102
		fld qword ptr [ebp - 0x20]		// 0x0002c104
		fmul qword ptr [ebp - 0x48]		// 0x0002c107
		fld qword ptr [ebp - 0x28]		// 0x0002c10a
		fmul qword ptr [ebp - 0x40]		// 0x0002c10d
		fsubp st(1), st(0)		// 0x0002c110
		fld qword ptr [ebp + 0x64]		// 0x0002c112
		fmul qword ptr [ebp - 0x68]		// 0x0002c115
		fld st(2)		// 0x0002c118
		fmul qword ptr [ebp - 0x60]		// 0x0002c11a
		fsubp st(1), st(0)		// 0x0002c11d
		fld st(1)		// 0x0002c11f
		fmul qword ptr [ebp - 0x58]		// 0x0002c121
		faddp st(1), st(0)		// 0x0002c124
		fld qword ptr [ebp - 0x18]		// 0x0002c126
		fmul qword ptr [ebp - 0x30]		// 0x0002c129
		fld qword ptr [ebp - 0x10]		// 0x0002c12c
		fmul qword ptr [ebp - 0x38]		// 0x0002c12f
		fsubp st(1), st(0)		// 0x0002c132
		fstp qword ptr [ebp + 0x38]		// 0x0002c134
		fld qword ptr [ebp - 0x20]		// 0x0002c137
		fmul qword ptr [ebp - 0x30]		// 0x0002c13a
		fstp qword ptr [ebp - 0x84]		// 0x0002c13d
		fld qword ptr [ebp - 0x10]		// 0x0002c143
		fmul qword ptr [ebp - 0x40]		// 0x0002c146
		fstp qword ptr [ebp - 0x8c]		// 0x0002c149
		fdivr qword ptr [nxHullK10106898_8]		// 0x0002c14f
		fld qword ptr [ebp + 0x64]		// 0x0002c155
		fmul qword ptr [ebp - 0x50]		// 0x0002c158
		fld qword ptr [ebp + 0x38]		// 0x0002c15b
		fmul qword ptr [ebp - 0x60]		// 0x0002c15e
		fsubp st(1), st(0)		// 0x0002c161
		fld qword ptr [ebp - 0x84]		// 0x0002c163
		fsub qword ptr [ebp - 0x8c]		// 0x0002c169
		fmul qword ptr [ebp - 0x58]		// 0x0002c16f
		faddp st(1), st(0)		// 0x0002c172
		fmul st(0), st(1)		// 0x0002c174
		fstp qword ptr [edx]		// 0x0002c176
		mov eax, dword ptr [edi]		// 0x0002c178
		fld qword ptr [ebp - 0x10]		// 0x0002c17a
		mov edx, dword ptr [ecx + eax*4]		// 0x0002c17d
		fmul qword ptr [ebp - 0x48]		// 0x0002c180
		fld qword ptr [ebp - 0x28]		// 0x0002c183
		fmul qword ptr [ebp - 0x30]		// 0x0002c186
		fsubp st(1), st(0)		// 0x0002c189
		fld qword ptr [ebp + 0x38]		// 0x0002c18b
		fmul qword ptr [ebp - 0x68]		// 0x0002c18e
		fxch st(4)		// 0x0002c191
		fmul qword ptr [ebp - 0x50]		// 0x0002c193
		fsubp st(4), st(0)		// 0x0002c196
		fld st(0)		// 0x0002c198
		fmul qword ptr [ebp - 0x58]		// 0x0002c19a
		faddp st(4), st(0)		// 0x0002c19d
		fxch st(3)		// 0x0002c19f
		fmul st(0), st(1)		// 0x0002c1a1
		fstp qword ptr [edx + 8]		// 0x0002c1a3
		fld qword ptr [ebp - 0x8c]		// 0x0002c1a6
		mov eax, dword ptr [edi]		// 0x0002c1ac
		fsub qword ptr [ebp - 0x84]		// 0x0002c1ae
		mov edx, dword ptr [ecx + eax*4]		// 0x0002c1b4
		mov esi, dword ptr [ebp + 0x18]		// 0x0002c1b7
		fmul qword ptr [ebp - 0x68]		// 0x0002c1ba
		fxch st(3)		// 0x0002c1bd
		fmul qword ptr [ebp - 0x60]		// 0x0002c1bf
		fsubp st(3), st(0)		// 0x0002c1c2
		fxch st(1)		// 0x0002c1c4
		fmul qword ptr [ebp - 0x50]		// 0x0002c1c6
		faddp st(2), st(0)		// 0x0002c1c9
		fxch st(1)		// 0x0002c1cb
		fmul st(0), st(1)		// 0x0002c1cd
		fstp qword ptr [edx + 0x10]		// 0x0002c1cf
		mov eax, dword ptr [edi]		// 0x0002c1d2
		mov edx, dword ptr [ecx + eax*4]		// 0x0002c1d4
		fstp st(0)		// 0x0002c1d7
		fld qword ptr [nxHullK101076e8_8]		// 0x0002c1d9
		fstp qword ptr [edx + 0x18]		// 0x0002c1df
		xor edx, edx		// 0x0002c1e2
		xor eax, eax		// 0x0002c1e4
		jmp L2c1f0		// 0x0002c1e6
L2c1e8:
		mov ecx, dword ptr [ebp + 0x6c]		// 0x0002c1e8
		jmp L2c1f0		// 0x0002c1eb
		lea ecx, [ecx]		// 0x0002c1ed
L2c1f0:
		mov ebx, dword ptr [edi]		// 0x0002c1f0
		mov ecx, dword ptr [ecx + ebx*4]		// 0x0002c1f2
		mov ebx, dword ptr [ebp + 0x30]		// 0x0002c1f5
		mov ebx, dword ptr [ebx]		// 0x0002c1f8
		fld qword ptr [ebx + eax]		// 0x0002c1fa
		add eax, 8		// 0x0002c1fd
		fsub qword ptr [ecx + eax - 8]		// 0x0002c200
		add edx, 4		// 0x0002c204
		cmp eax, 0x18		// 0x0002c207
		fld st(0)		// 0x0002c20a
		fmul st(0), st(1)		// 0x0002c20c
		fadd qword ptr [ecx + 0x18]		// 0x0002c20e
		fstp qword ptr [ecx + 0x18]		// 0x0002c211
		mov ecx, dword ptr [ebp + 0x5c]		// 0x0002c214
		mov ecx, dword ptr [ecx]		// 0x0002c217
		mov ebx, dword ptr [edi]		// 0x0002c219
		fstp st(0)		// 0x0002c21b
		mov ecx, dword ptr [edx + ecx - 4]		// 0x0002c21d
		mov ebx, dword ptr [esi + ebx*4]		// 0x0002c221
		mov dword ptr [edx + ebx - 4], ecx		// 0x0002c224
		jl L2c1e8		// 0x0002c228
		mov edx, dword ptr [edi]		// 0x0002c22a
		mov eax, dword ptr [esi + edx*4]		// 0x0002c22c
		mov ecx, dword ptr [ebp + 0x54]		// 0x0002c22f
		mov edx, dword ptr [ebp + 0x2c]		// 0x0002c232
		mov ebx, dword ptr [ebp + 0x60]		// 0x0002c235
		mov dword ptr [eax + 0xc], ecx		// 0x0002c238
		mov eax, dword ptr [ebp + 0x34]		// 0x0002c23b
		inc edx		// 0x0002c23e
		add edi, 4		// 0x0002c23f
		inc eax		// 0x0002c242
		mov dword ptr [ebp + 0x34], eax		// 0x0002c243
		mov eax, dword ptr [ebp + 0x5c]		// 0x0002c246
		mov dword ptr [ebp + 0x2c], edx		// 0x0002c249
L2c24c:
		mov ecx, dword ptr [ebp + 0x50]		// 0x0002c24c
		add eax, 4		// 0x0002c24f
		dec ecx		// 0x0002c252
		mov dword ptr [ebp + 0x5c], eax		// 0x0002c253
		mov dword ptr [ebp + 0x50], ecx		// 0x0002c256
		jne L2c057		// 0x0002c259
L2c25f:
		mov edx, dword ptr [ebp + 0x34]		// 0x0002c25f
		mov eax, dword ptr [ebp + 0x40]		// 0x0002c262
		mov ecx, dword ptr [ebp + 0x70]		// 0x0002c265
		add eax, edx		// 0x0002c268
		mov edx, dword ptr [ebp + 0x30]		// 0x0002c26a
		mov dword ptr [ebp + 0x40], eax		// 0x0002c26d
		mov eax, dword ptr [ebp + 0x54]		// 0x0002c270
		inc eax		// 0x0002c273
		add edx, 4		// 0x0002c274
		cmp eax, ecx		// 0x0002c277
		mov dword ptr [ebp + 0x54], eax		// 0x0002c279
		mov dword ptr [ebp + 0x30], edx		// 0x0002c27c
		jl L2be80		// 0x0002c27f
		mov edx, ecx		// 0x0002c285
		mov ecx, dword ptr [ebp + 0x40]		// 0x0002c287
L2c28a:
		or eax, 0xffffffff		// 0x0002c28a
		test ecx, ecx		// 0x0002c28d
		mov dword ptr [ebx + 4], 0		// 0x0002c28f
		jle L2c2ba		// 0x0002c296
L2c298:
		mov edi, dword ptr [esi + eax*4 + 4]		// 0x0002c298
		inc eax		// 0x0002c29c
		cmp dword ptr [edi], 0		// 0x0002c29d
		jge L2c2ac		// 0x0002c2a0
L2c2a2:
		mov edi, dword ptr [esi + eax*4 + 4]		// 0x0002c2a2
		inc eax		// 0x0002c2a6
		cmp dword ptr [edi], 0		// 0x0002c2a7
		jl L2c2a2		// 0x0002c2aa
L2c2ac:
		mov edi, dword ptr [esi + eax*4]		// 0x0002c2ac
		cmp dword ptr [edi + 4], edx		// 0x0002c2af
		jge L2c2b7		// 0x0002c2b2
		inc dword ptr [ebx + 4]		// 0x0002c2b4
L2c2b7:
		dec ecx		// 0x0002c2b7
		jne L2c298		// 0x0002c2b8
L2c2ba:
		mov edi, dword ptr [ebx + 4]		// 0x0002c2ba
		call nxHullCallAllocator		// 0x0002c2bd
		mov edx, dword ptr [eax]		// 0x0002c2c2
		lea ecx, [edi + edi*2]		// 0x0002c2c4
		lea ecx, [ecx*4 + 4]		// 0x0002c2c7
		push 1		// 0x0002c2ce
		push ecx		// 0x0002c2d0
		mov ecx, eax		// 0x0002c2d1
		call dword ptr [edx]		// 0x0002c2d3
		test eax, eax		// 0x0002c2d5
		je L2c2e6		// 0x0002c2d7
		mov dword ptr [eax], edi		// 0x0002c2d9
		lea edx, [eax + 4]		// 0x0002c2db
		jmp L2c2e8		// 0x0002c2de
L2c2e0:
		mov byte ptr [ebp + 0x7f], 0		// 0x0002c2e0
		jmp L2c355		// 0x0002c2e4
L2c2e6:
		xor edx, edx		// 0x0002c2e6
L2c2e8:
		mov ecx, dword ptr [ebp + 0x40]		// 0x0002c2e8
		or eax, 0xffffffff		// 0x0002c2eb
		test ecx, ecx		// 0x0002c2ee
		mov dword ptr [ebp + 0x28], edx		// 0x0002c2f0
		mov dword ptr [ebx + 4], 0		// 0x0002c2f3
		jle L2c351		// 0x0002c2fa
		mov dword ptr [ebp + 0x7c], ecx		// 0x0002c2fc
		nop		// 0x0002c2ff
L2c300:
		mov ecx, dword ptr [esi + eax*4 + 4]		// 0x0002c300
		mov edi, dword ptr [ecx]		// 0x0002c304
		inc eax		// 0x0002c306
		test edi, edi		// 0x0002c307
		jge L2c31b		// 0x0002c309
		jmp L2c310		// 0x0002c30b
		lea ecx, [ecx]		// 0x0002c30d
L2c310:
		mov ecx, dword ptr [esi + eax*4 + 4]		// 0x0002c310
		mov edi, dword ptr [ecx]		// 0x0002c314
		inc eax		// 0x0002c316
		test edi, edi		// 0x0002c317
		jl L2c310		// 0x0002c319
L2c31b:
		mov ecx, dword ptr [esi + eax*4]		// 0x0002c31b
		mov ecx, dword ptr [ecx + 4]		// 0x0002c31e
		cmp ecx, dword ptr [ebp + 0x70]		// 0x0002c321
		jge L2c34c		// 0x0002c324
		mov ecx, dword ptr [ebx + 4]		// 0x0002c326
		mov edi, dword ptr [esi + eax*4]		// 0x0002c329
		mov edi, dword ptr [edi + 4]		// 0x0002c32c
		lea ecx, [ecx + ecx*2]		// 0x0002c32f
		lea ecx, [edx + ecx*4]		// 0x0002c332
		mov dword ptr [ecx], edi		// 0x0002c335
		mov edi, dword ptr [esi + eax*4]		// 0x0002c337
		mov edi, dword ptr [edi + 8]		// 0x0002c33a
		mov dword ptr [ecx + 4], edi		// 0x0002c33d
		mov edi, dword ptr [esi + eax*4]		// 0x0002c340
		mov edi, dword ptr [edi + 0xc]		// 0x0002c343
		mov dword ptr [ecx + 8], edi		// 0x0002c346
		inc dword ptr [ebx + 4]		// 0x0002c349
L2c34c:
		dec dword ptr [ebp + 0x7c]		// 0x0002c34c
		jne L2c300		// 0x0002c34f
L2c351:
		mov byte ptr [ebp + 0x7f], 1		// 0x0002c351
L2c355:
		mov edx, dword ptr [ebp + 0x6c]		// 0x0002c355
		cmp dword ptr [edx], 0		// 0x0002c358
		je L2c377		// 0x0002c35b
		call nxHullCallAllocator		// 0x0002c35d
		mov edi, dword ptr [ebp + 0x6c]		// 0x0002c362
		mov ecx, dword ptr [edi]		// 0x0002c365
		mov edx, dword ptr [eax]		// 0x0002c367
		push ecx		// 0x0002c369
		mov ecx, eax		// 0x0002c36a
		call dword ptr [edx + 0xc]		// 0x0002c36c
		mov dword ptr [edi], 0		// 0x0002c36f
		jmp L2c37a		// 0x0002c375
L2c377:
		mov edi, dword ptr [ebp + 0x6c]		// 0x0002c377
L2c37a:
		call nxHullCallAllocator		// 0x0002c37a
		mov edx, dword ptr [eax]		// 0x0002c37f
		push edi		// 0x0002c381
		mov ecx, eax		// 0x0002c382
		call dword ptr [edx + 0xc]		// 0x0002c384
		cmp dword ptr [esi], 0		// 0x0002c387
		je L2c3a1		// 0x0002c38a
		call nxHullCallAllocator		// 0x0002c38c
		mov ecx, dword ptr [esi]		// 0x0002c391
		mov edx, dword ptr [eax]		// 0x0002c393
		push ecx		// 0x0002c395
		mov ecx, eax		// 0x0002c396
		call dword ptr [edx + 0xc]		// 0x0002c398
		mov dword ptr [esi], 0		// 0x0002c39b
L2c3a1:
		call nxHullCallAllocator		// 0x0002c3a1
		mov edx, dword ptr [eax]		// 0x0002c3a6
		push esi		// 0x0002c3a8
		mov ecx, eax		// 0x0002c3a9
		call dword ptr [edx + 0xc]		// 0x0002c3ab
		mov esi, dword ptr [ebp + 0x48]		// 0x0002c3ae
		test esi, esi		// 0x0002c3b1
		je L2c3c2		// 0x0002c3b3
		call nxHullCallAllocator		// 0x0002c3b5
		mov edx, dword ptr [eax]		// 0x0002c3ba
		push esi		// 0x0002c3bc
		mov ecx, eax		// 0x0002c3bd
		call dword ptr [edx + 0xc]		// 0x0002c3bf
L2c3c2:
		mov esi, dword ptr [ebp + 0x24]		// 0x0002c3c2
		cmp dword ptr [esi], 0		// 0x0002c3c5
		je L2c3df		// 0x0002c3c8
		call nxHullCallAllocator		// 0x0002c3ca
		mov ecx, dword ptr [esi]		// 0x0002c3cf
		mov edx, dword ptr [eax]		// 0x0002c3d1
		push ecx		// 0x0002c3d3
		mov ecx, eax		// 0x0002c3d4
		call dword ptr [edx + 0xc]		// 0x0002c3d6
		mov dword ptr [esi], 0		// 0x0002c3d9
L2c3df:
		call nxHullCallAllocator		// 0x0002c3df
		mov edx, dword ptr [eax]		// 0x0002c3e4
		push esi		// 0x0002c3e6
		mov ecx, eax		// 0x0002c3e7
		call dword ptr [edx + 0xc]		// 0x0002c3e9
		mov eax, dword ptr [ebp + 0x70]		// 0x0002c3ec
		mov edi, dword ptr [ebp + 0x1c]		// 0x0002c3ef
		add eax, 4		// 0x0002c3f2
		xor esi, esi		// 0x0002c3f5
		test eax, eax		// 0x0002c3f7
		jle L2c428		// 0x0002c3f9
		jmp L2c400		// 0x0002c3fb
		lea ecx, [ecx]		// 0x0002c3fd
L2c400:
		cmp dword ptr [edi + esi*4], 0		// 0x0002c400
		je L2c41d		// 0x0002c404
		call nxHullCallAllocator		// 0x0002c406
		mov ecx, dword ptr [edi + esi*4]		// 0x0002c40b
		mov edx, dword ptr [eax]		// 0x0002c40e
		push ecx		// 0x0002c410
		mov ecx, eax		// 0x0002c411
		call dword ptr [edx + 0xc]		// 0x0002c413
		mov dword ptr [edi + esi*4], 0		// 0x0002c416
L2c41d:
		mov eax, dword ptr [ebp + 0x70]		// 0x0002c41d
		inc esi		// 0x0002c420
		add eax, 4		// 0x0002c421
		cmp esi, eax		// 0x0002c424
		jl L2c400		// 0x0002c426
L2c428:
		call nxHullCallAllocator		// 0x0002c428
		mov edx, dword ptr [eax]		// 0x0002c42d
		push edi		// 0x0002c42f
		mov ecx, eax		// 0x0002c430
		call dword ptr [edx + 0xc]		// 0x0002c432
		mov al, byte ptr [ebp + 0x7f]		// 0x0002c435
		test al, al		// 0x0002c438
		jne L2c46f		// 0x0002c43a
		mov esi, dword ptr [ebp + 0x28]		// 0x0002c43c
		test esi, esi		// 0x0002c43f
		je L2c453		// 0x0002c441
		call nxHullCallAllocator		// 0x0002c443
		mov edx, dword ptr [eax]		// 0x0002c448
		add esi, -4		// 0x0002c44a
		push esi		// 0x0002c44d
		mov ecx, eax		// 0x0002c44e
		call dword ptr [edx + 0xc]		// 0x0002c450
L2c453:
		lea ecx, [ebp - 0x7c]		// 0x0002c453
		call nxHullCallReducedDtor		// 0x0002c456
		xor al, al		// 0x0002c45b
		lea esp, [ebp - 0x98]		// 0x0002c45d
		pop edi		// 0x0002c463
		pop esi		// 0x0002c464
		pop ebx		// 0x0002c465
		add ebp, 0x74		// 0x0002c466
		mov esp, ebp		// 0x0002c469
		pop ebp		// 0x0002c46b
		ret 4		// 0x0002c46c
L2c46f:
		mov edi, dword ptr [ebx + 4]		// 0x0002c46f
		lea esi, [ebx + 4]		// 0x0002c472
		call nxHullCallAllocator		// 0x0002c475
		mov edx, dword ptr [eax]		// 0x0002c47a
		lea ecx, [edi + edi*2]		// 0x0002c47c
		lea ecx, [ecx + ecx + 4]		// 0x0002c47f
		push 0		// 0x0002c483
		push ecx		// 0x0002c485
		mov ecx, eax		// 0x0002c486
		call dword ptr [edx]		// 0x0002c488
		test eax, eax		// 0x0002c48a
		je L2c495		// 0x0002c48c
		mov dword ptr [eax], edi		// 0x0002c48e
		add eax, 4		// 0x0002c490
		jmp L2c497		// 0x0002c493
L2c495:
		xor eax, eax		// 0x0002c495
L2c497:
		test eax, eax		// 0x0002c497
		mov dword ptr [ebx + 8], eax		// 0x0002c499
		je L2c453		// 0x0002c49c
		mov eax, dword ptr [esi]		// 0x0002c49e
		xor edx, edx		// 0x0002c4a0
		test eax, eax		// 0x0002c4a2
		jbe L2c4e3		// 0x0002c4a4
		mov eax, dword ptr [ebp + 0x28]		// 0x0002c4a6
		xor ecx, ecx		// 0x0002c4a9
		add eax, 8		// 0x0002c4ab
		mov edi, edi		// 0x0002c4ae
L2c4b0:
		mov di, word ptr [eax - 8]		// 0x0002c4b0
		mov esi, dword ptr [ebx + 8]		// 0x0002c4b4
		mov word ptr [ecx + esi], di		// 0x0002c4b7
		mov di, word ptr [eax - 4]		// 0x0002c4bb
		mov esi, dword ptr [ebx + 8]		// 0x0002c4bf
		mov word ptr [ecx + esi + 2], di		// 0x0002c4c2
		mov di, word ptr [eax]		// 0x0002c4c7
		mov esi, dword ptr [ebx + 8]		// 0x0002c4ca
		mov word ptr [ecx + esi + 4], di		// 0x0002c4cd
		mov edi, dword ptr [ebx + 4]		// 0x0002c4d2
		lea esi, [ebx + 4]		// 0x0002c4d5
		inc edx		// 0x0002c4d8
		add eax, 0xc		// 0x0002c4d9
		add ecx, 6		// 0x0002c4dc
		cmp edx, edi		// 0x0002c4df
		jb L2c4b0		// 0x0002c4e1
L2c4e3:
		mov edi, dword ptr [ebp + 0x28]		// 0x0002c4e3
		test edi, edi		// 0x0002c4e6
		je L2c4fa		// 0x0002c4e8
		call nxHullCallAllocator		// 0x0002c4ea
		mov edx, dword ptr [eax]		// 0x0002c4ef
		add edi, -4		// 0x0002c4f1
		push edi		// 0x0002c4f4
		mov ecx, eax		// 0x0002c4f5
		call dword ptr [edx + 0xc]		// 0x0002c4f7
L2c4fa:
		mov eax, dword ptr [ebx + 8]		// 0x0002c4fa
		push 1		// 0x0002c4fd
		push eax		// 0x0002c4ff
		call nxHullDedupFaces		// 0x0002c500
		mov al, byte ptr [ebp + 0x10]		// 0x0002c505
		add esp, 8		// 0x0002c508
		test al, al		// 0x0002c50b
		je L2c526		// 0x0002c50d
		mov ecx, dword ptr [ebx + 8]		// 0x0002c50f
		mov edx, dword ptr [ebp + 8]		// 0x0002c512
		mov eax, dword ptr [esi]		// 0x0002c515
		push 1		// 0x0002c517
		push ecx		// 0x0002c519
		push edx		// 0x0002c51a
		mov edx, dword ptr [ebp + 0x70]		// 0x0002c51b
		call nxHullOrientFaces		// 0x0002c51e
		add esp, 0xc		// 0x0002c523
L2c526:
		mov edi, dword ptr [ebp + 0x70]		// 0x0002c526
		mov eax, edi		// 0x0002c529
		add eax, 3		// 0x0002c52b
		and eax, 0xfffffffc		// 0x0002c52e
		call nxHullCallAlloca		// 0x0002c531
		mov edx, esp		// 0x0002c536
		mov ecx, edi		// 0x0002c538
		mov edi, edx		// 0x0002c53a
		mov dword ptr [ebp + 0x68], edx		// 0x0002c53c
		mov edx, ecx		// 0x0002c53f
		shr ecx, 2		// 0x0002c541
		xor eax, eax		// 0x0002c544
		rep stosd		// 0x0002c546
		mov ecx, edx		// 0x0002c548
		and ecx, 3		// 0x0002c54a
		rep stosb		// 0x0002c54d
		mov ecx, dword ptr [esi]		// 0x0002c54f
		xor eax, eax		// 0x0002c551
		cmp ecx, eax		// 0x0002c553
		mov dword ptr [ebx + 0xc], eax		// 0x0002c555
		mov dword ptr [ebp + 0x50], eax		// 0x0002c558
		jbe L2c5bf		// 0x0002c55b
		mov dword ptr [ebp + 0x7c], eax		// 0x0002c55d
		mov edi, esp		// 0x0002c560
L2c562:
		mov ecx, dword ptr [ebp + 0x7c]		// 0x0002c562
		mov eax, dword ptr [ebx + 8]		// 0x0002c565
		mov dx, word ptr [eax + ecx + 2]		// 0x0002c568
		add eax, ecx		// 0x0002c56d
		mov cx, word ptr [eax]		// 0x0002c56f
		mov ax, word ptr [eax + 4]		// 0x0002c572
		movzx ecx, cx		// 0x0002c576
		add ecx, edi		// 0x0002c579
		cmp byte ptr [ecx], 0		// 0x0002c57b
		jne L2c586		// 0x0002c57e
		mov byte ptr [ecx], 1		// 0x0002c580
		inc dword ptr [ebx + 0xc]		// 0x0002c583
L2c586:
		movzx edx, dx		// 0x0002c586
		lea ecx, [edx + edi]		// 0x0002c589
		cmp byte ptr [ecx], 0		// 0x0002c58c
		jne L2c597		// 0x0002c58f
		mov byte ptr [ecx], 1		// 0x0002c591
		inc dword ptr [ebx + 0xc]		// 0x0002c594
L2c597:
		movzx eax, ax		// 0x0002c597
		mov cl, byte ptr [eax + edi]		// 0x0002c59a
		add eax, edi		// 0x0002c59d
		test cl, cl		// 0x0002c59f
		jne L2c5a9		// 0x0002c5a1
		mov byte ptr [eax], 1		// 0x0002c5a3
		inc dword ptr [ebx + 0xc]		// 0x0002c5a6
L2c5a9:
		mov eax, dword ptr [ebp + 0x50]		// 0x0002c5a9
		mov edx, dword ptr [ebp + 0x7c]		// 0x0002c5ac
		mov ecx, dword ptr [esi]		// 0x0002c5af
		inc eax		// 0x0002c5b1
		add edx, 6		// 0x0002c5b2
		cmp eax, ecx		// 0x0002c5b5
		mov dword ptr [ebp + 0x50], eax		// 0x0002c5b7
		mov dword ptr [ebp + 0x7c], edx		// 0x0002c5ba
		jb L2c562		// 0x0002c5bd
L2c5bf:
		mov esi, dword ptr [ebp + 0x70]		// 0x0002c5bf
		lea eax, [esi*4]		// 0x0002c5c2
		add eax, 3		// 0x0002c5c9
		and eax, 0xfffffffc		// 0x0002c5cc
		call nxHullCallAlloca		// 0x0002c5cf
		mov edi, esp		// 0x0002c5d4
		call nxHullCallAllocator		// 0x0002c5d6
		mov ecx, dword ptr [ebx + 0xc]		// 0x0002c5db
		mov edx, dword ptr [eax]		// 0x0002c5de
		lea ecx, [ecx + ecx*2]		// 0x0002c5e0
		shl ecx, 2		// 0x0002c5e3
		push 0		// 0x0002c5e6
		push ecx		// 0x0002c5e8
		mov ecx, eax		// 0x0002c5e9
		call dword ptr [edx]		// 0x0002c5eb
		mov dword ptr [ebx + 0x10], eax		// 0x0002c5ed
		xor eax, eax		// 0x0002c5f0
		cmp esi, eax		// 0x0002c5f2
		mov dword ptr [ebp + 0x7c], eax		// 0x0002c5f4
		jle L2c653		// 0x0002c5f7
		mov edx, dword ptr [ebp + 8]		// 0x0002c5f9
		mov dword ptr [ebp + 0x48], eax		// 0x0002c5fc
		mov dword ptr [ebp + 0x5c], edx		// 0x0002c5ff
		mov ecx, 0xc		// 0x0002c602
L2c607:
		mov edx, dword ptr [ebp + 0x68]		// 0x0002c607
		cmp byte ptr [eax + edx], 0		// 0x0002c60a
		je L2c646		// 0x0002c60e
		mov ecx, dword ptr [ebp + 0x7c]		// 0x0002c610
		mov dword ptr [edi + eax*4], ecx		// 0x0002c613
		mov edx, dword ptr [ebx + 0x10]		// 0x0002c616
		add edx, dword ptr [ebp + 0x48]		// 0x0002c619
		mov ecx, dword ptr [ebp + 0x5c]		// 0x0002c61c
		mov esi, dword ptr [ecx]		// 0x0002c61f
		mov dword ptr [edx], esi		// 0x0002c621
		mov esi, dword ptr [ecx + 4]		// 0x0002c623
		mov dword ptr [edx + 4], esi		// 0x0002c626
		mov ecx, dword ptr [ecx + 8]		// 0x0002c629
		mov esi, dword ptr [ebp + 0x7c]		// 0x0002c62c
		mov dword ptr [edx + 8], ecx		// 0x0002c62f
		mov edx, dword ptr [ebp + 0x48]		// 0x0002c632
		inc esi		// 0x0002c635
		mov ecx, 0xc		// 0x0002c636
		add edx, ecx		// 0x0002c63b
		mov dword ptr [ebp + 0x7c], esi		// 0x0002c63d
		mov esi, dword ptr [ebp + 0x70]		// 0x0002c640
		mov dword ptr [ebp + 0x48], edx		// 0x0002c643
L2c646:
		mov edx, dword ptr [ebp + 0x5c]		// 0x0002c646
		inc eax		// 0x0002c649
		add edx, ecx		// 0x0002c64a
		cmp eax, esi		// 0x0002c64c
		mov dword ptr [ebp + 0x5c], edx		// 0x0002c64e
		jl L2c607		// 0x0002c651
L2c653:
		mov eax, dword ptr [ebx + 4]		// 0x0002c653
		lea ecx, [ebx + 4]		// 0x0002c656
		xor edx, edx		// 0x0002c659
		test eax, eax		// 0x0002c65b
		jbe L2c6a5		// 0x0002c65d
		xor eax, eax		// 0x0002c65f
L2c661:
		mov ecx, dword ptr [ebx + 8]		// 0x0002c661
		movzx esi, word ptr [ecx + eax]		// 0x0002c664
		mov si, word ptr [edi + esi*4]		// 0x0002c668
		add ecx, eax		// 0x0002c66c
		mov word ptr [ecx], si		// 0x0002c66e
		mov ecx, dword ptr [ebx + 8]		// 0x0002c671
		movzx esi, word ptr [eax + ecx + 2]		// 0x0002c674
		mov si, word ptr [edi + esi*4]		// 0x0002c679
		lea ecx, [eax + ecx + 2]		// 0x0002c67d
		mov word ptr [ecx], si		// 0x0002c681
		mov ecx, dword ptr [ebx + 8]		// 0x0002c684
		movzx esi, word ptr [eax + ecx + 4]		// 0x0002c687
		mov si, word ptr [edi + esi*4]		// 0x0002c68c
		lea ecx, [eax + ecx + 4]		// 0x0002c690
		mov word ptr [ecx], si		// 0x0002c694
		mov esi, dword ptr [ebx + 4]		// 0x0002c697
		lea ecx, [ebx + 4]		// 0x0002c69a
		inc edx		// 0x0002c69d
		add eax, 6		// 0x0002c69e
		cmp edx, esi		// 0x0002c6a1
		jb L2c661		// 0x0002c6a3
L2c6a5:
		mov edx, dword ptr [ebx + 8]		// 0x0002c6a5
		mov ebx, dword ptr [ebx + 0x10]		// 0x0002c6a8
		push 1		// 0x0002c6ab
		push edx		// 0x0002c6ad
		push ecx		// 0x0002c6ae
		call nxHullDropDegenerate		// 0x0002c6af
		add esp, 0xc		// 0x0002c6b4
		test al, al		// 0x0002c6b7
		je L2c453		// 0x0002c6b9
		mov al, byte ptr [ebp + 0x11]		// 0x0002c6bf
		test al, al		// 0x0002c6c2
		mov esi, dword ptr [ebp + 0x60]		// 0x0002c6c4
		je L2c6d8		// 0x0002c6c7
		mov ecx, esi		// 0x0002c6c9
		call nxHullCallComputePolygons		// 0x0002c6cb
		test al, al		// 0x0002c6d0
		je L2c453		// 0x0002c6d2
L2c6d8:
		lea eax, [esi + 0x18]		// 0x0002c6d8
		push eax		// 0x0002c6db
		mov ecx, esi		// 0x0002c6dc
		call nxHullCallComputeCentroid		// 0x0002c6de
		mov eax, dword ptr [esi + 0x10]		// 0x0002c6e3
		test eax, eax		// 0x0002c6e6
		je L2c6fc		// 0x0002c6e8
		mov eax, dword ptr [esi + 8]		// 0x0002c6ea
		test eax, eax		// 0x0002c6ed
		je L2c6fc		// 0x0002c6ef
		mov eax, dword ptr [esi + 0x28]		// 0x0002c6f1
		test eax, eax		// 0x0002c6f4
		je L2c6fc		// 0x0002c6f6
		mov bl, 1		// 0x0002c6f8
		jmp L2c6fe		// 0x0002c6fa
L2c6fc:
		xor bl, bl		// 0x0002c6fc
L2c6fe:
		lea ecx, [ebp - 0x7c]		// 0x0002c6fe
		call nxHullCallReducedDtor		// 0x0002c701
		mov al, bl		// 0x0002c706
		lea esp, [ebp - 0x98]		// 0x0002c708
		pop edi		// 0x0002c70e
		pop esi		// 0x0002c70f
		pop ebx		// 0x0002c710
		add ebp, 0x74		// 0x0002c711
		mov esp, ebp		// 0x0002c714
		pop ebp		// 0x0002c716
		ret 4		// 0x0002c717
		}
	}

// phys_fn_001494 (0x0002c720, 459 B), 181 instructions
__declspec(naked) void nxHullInit()
	{
	__asm
		{
		push ebx		// 0x0002c720
		mov ebx, dword ptr [esp + 8]		// 0x0002c721
		mov eax, dword ptr [ebx]		// 0x0002c725
		push ebp		// 0x0002c727
		mov ebp, ecx		// 0x0002c728
		mov dword ptr [ebp + 0xc], eax		// 0x0002c72a
		mov ecx, dword ptr [ebx + 8]		// 0x0002c72d
		mov dword ptr [ebp + 4], ecx		// 0x0002c730
		call nxHullCallAllocator		// 0x0002c733
		mov ecx, dword ptr [ebp + 0xc]		// 0x0002c738
		mov edx, dword ptr [eax]		// 0x0002c73b
		lea ecx, [ecx + ecx*2]		// 0x0002c73d
		shl ecx, 2		// 0x0002c740
		push 0		// 0x0002c743
		push ecx		// 0x0002c745
		mov ecx, eax		// 0x0002c746
		call dword ptr [edx]		// 0x0002c748
		test eax, eax		// 0x0002c74a
		mov dword ptr [ebp + 0x10], eax		// 0x0002c74c
		jne L2c758		// 0x0002c74f
		pop ebp		// 0x0002c751
		xor al, al		// 0x0002c752
		pop ebx		// 0x0002c754
		ret 4		// 0x0002c755
L2c758:
		mov ecx, dword ptr [ebp + 0xc]		// 0x0002c758
		lea ecx, [ecx + ecx*2]		// 0x0002c75b
		shl ecx, 2		// 0x0002c75e
		push esi		// 0x0002c761
		mov esi, dword ptr [ebx + 4]		// 0x0002c762
		mov edx, ecx		// 0x0002c765
		shr ecx, 2		// 0x0002c767
		push edi		// 0x0002c76a
		mov edi, eax		// 0x0002c76b
		rep movsd		// 0x0002c76d
		mov ecx, edx		// 0x0002c76f
		and ecx, 3		// 0x0002c771
		rep movsb		// 0x0002c774
		mov eax, dword ptr [ebx + 0x10]		// 0x0002c776
		test eax, eax		// 0x0002c779
		je L2c7d8		// 0x0002c77b
		mov esi, dword ptr [ebp + 4]		// 0x0002c77d
		call nxHullCallAllocator		// 0x0002c780
		mov edx, dword ptr [eax]		// 0x0002c785
		lea ecx, [esi + esi*2]		// 0x0002c787
		lea ecx, [ecx + ecx + 4]		// 0x0002c78a
		push 0		// 0x0002c78e
		push ecx		// 0x0002c790
		mov ecx, eax		// 0x0002c791
		call dword ptr [edx]		// 0x0002c793
		test eax, eax		// 0x0002c795
		je L2c7b0		// 0x0002c797
		push offset nxHullIdentityCtor		// 0x0002c799
		push esi		// 0x0002c79e
		lea edi, [eax + 4]		// 0x0002c79f
		push 6		// 0x0002c7a2
		push edi		// 0x0002c7a4
		mov dword ptr [eax], esi		// 0x0002c7a5
		call nxHullCallVectorConstruct		// 0x0002c7a7
		mov eax, edi		// 0x0002c7ac
		jmp L2c7b2		// 0x0002c7ae
L2c7b0:
		xor eax, eax		// 0x0002c7b0
L2c7b2:
		test eax, eax		// 0x0002c7b2
		mov dword ptr [ebp + 8], eax		// 0x0002c7b4
		je L2c8e2		// 0x0002c7b7
		mov ecx, dword ptr [ebp + 4]		// 0x0002c7bd
		mov esi, dword ptr [ebx + 0x10]		// 0x0002c7c0
		lea ecx, [ecx + ecx*2]		// 0x0002c7c3
		shl ecx, 1		// 0x0002c7c6
		mov edx, ecx		// 0x0002c7c8
		shr ecx, 2		// 0x0002c7ca
		mov edi, eax		// 0x0002c7cd
		rep movsd		// 0x0002c7cf
		mov ecx, edx		// 0x0002c7d1
		and ecx, 3		// 0x0002c7d3
		rep movsb		// 0x0002c7d6
L2c7d8:
		mov eax, dword ptr [ebx + 0xc]		// 0x0002c7d8
		test eax, eax		// 0x0002c7db
		je L2c86c		// 0x0002c7dd
		mov esi, dword ptr [ebp + 4]		// 0x0002c7e3
		call nxHullCallAllocator		// 0x0002c7e6
		mov edx, dword ptr [eax]		// 0x0002c7eb
		lea ecx, [esi + esi*2]		// 0x0002c7ed
		lea ecx, [ecx + ecx + 4]		// 0x0002c7f0
		push 0		// 0x0002c7f4
		push ecx		// 0x0002c7f6
		mov ecx, eax		// 0x0002c7f7
		call dword ptr [edx]		// 0x0002c7f9
		test eax, eax		// 0x0002c7fb
		je L2c814		// 0x0002c7fd
		push offset nxHullIdentityCtor		// 0x0002c7ff
		push esi		// 0x0002c804
		lea edi, [eax + 4]		// 0x0002c805
		push 6		// 0x0002c808
		push edi		// 0x0002c80a
		mov dword ptr [eax], esi		// 0x0002c80b
		call nxHullCallVectorConstruct		// 0x0002c80d
		jmp L2c816		// 0x0002c812
L2c814:
		xor edi, edi		// 0x0002c814
L2c816:
		test edi, edi		// 0x0002c816
		mov dword ptr [ebp + 8], edi		// 0x0002c818
		je L2c8e2		// 0x0002c81b
		mov eax, dword ptr [ebp + 4]		// 0x0002c821
		xor edx, edx		// 0x0002c824
		test eax, eax		// 0x0002c826
		jbe L2c86c		// 0x0002c828
		xor ecx, ecx		// 0x0002c82a
		xor eax, eax		// 0x0002c82c
		mov edi, edi		// 0x0002c82e
L2c830:
		mov esi, dword ptr [ebx + 0xc]		// 0x0002c830
		mov si, word ptr [eax + esi]		// 0x0002c833
		mov edi, dword ptr [ebp + 8]		// 0x0002c837
		mov word ptr [ecx + edi], si		// 0x0002c83a
		mov esi, dword ptr [ebx + 0xc]		// 0x0002c83e
		mov si, word ptr [eax + esi + 4]		// 0x0002c841
		mov edi, dword ptr [ebp + 8]		// 0x0002c846
		mov word ptr [ecx + edi + 2], si		// 0x0002c849
		mov esi, dword ptr [ebx + 0xc]		// 0x0002c84e
		mov si, word ptr [eax + esi + 8]		// 0x0002c851
		mov edi, dword ptr [ebp + 8]		// 0x0002c856
		mov word ptr [ecx + edi + 4], si		// 0x0002c859
		mov esi, dword ptr [ebp + 4]		// 0x0002c85e
		inc edx		// 0x0002c861
		add eax, 0xc		// 0x0002c862
		add ecx, 6		// 0x0002c865
		cmp edx, esi		// 0x0002c868
		jb L2c830		// 0x0002c86a
L2c86c:
		mov edi, dword ptr [ebp + 8]		// 0x0002c86c
		mov ebx, dword ptr [ebp + 0x10]		// 0x0002c86f
		push 1		// 0x0002c872
		push edi		// 0x0002c874
		lea esi, [ebp + 4]		// 0x0002c875
		call nxHullDedupFaces		// 0x0002c878
		mov eax, dword ptr [esi]		// 0x0002c87d
		mov edx, dword ptr [ebp + 0xc]		// 0x0002c87f
		push 1		// 0x0002c882
		push edi		// 0x0002c884
		push ebx		// 0x0002c885
		call nxHullOrientFaces		// 0x0002c886
		push 1		// 0x0002c88b
		push edi		// 0x0002c88d
		push esi		// 0x0002c88e
		call nxHullDropDegenerate		// 0x0002c88f
		mov edx, dword ptr [ebp + 0xc]		// 0x0002c894
		mov eax, dword ptr [ebp + 4]		// 0x0002c897
		mov edi, dword ptr [ebp + 8]		// 0x0002c89a
		push edx		// 0x0002c89d
		push eax		// 0x0002c89e
		mov eax, dword ptr [ebp + 0x10]		// 0x0002c89f
		call nxHullClean		// 0x0002c8a2
		add esp, 0x28		// 0x0002c8a7
		test al, al		// 0x0002c8aa
		je L2c8e2		// 0x0002c8ac
		mov ecx, ebp		// 0x0002c8ae
		call nxHullCallComputePolygons		// 0x0002c8b0
		test al, al		// 0x0002c8b5
		je L2c8e2		// 0x0002c8b7
		lea ecx, [ebp + 0x18]		// 0x0002c8b9
		push ecx		// 0x0002c8bc
		mov ecx, ebp		// 0x0002c8bd
		call nxHullCallComputeCentroid		// 0x0002c8bf
		mov eax, dword ptr [ebp + 0x10]		// 0x0002c8c4
		test eax, eax		// 0x0002c8c7
		je L2c8e2		// 0x0002c8c9
		mov eax, dword ptr [ebp + 8]		// 0x0002c8cb
		test eax, eax		// 0x0002c8ce
		je L2c8e2		// 0x0002c8d0
		mov eax, dword ptr [ebp + 0x28]		// 0x0002c8d2
		test eax, eax		// 0x0002c8d5
		je L2c8e2		// 0x0002c8d7
		pop edi		// 0x0002c8d9
		pop esi		// 0x0002c8da
		pop ebp		// 0x0002c8db
		mov al, 1		// 0x0002c8dc
		pop ebx		// 0x0002c8de
		ret 4		// 0x0002c8df
L2c8e2:
		pop edi		// 0x0002c8e2
		pop esi		// 0x0002c8e3
		pop ebp		// 0x0002c8e4
		xor al, al		// 0x0002c8e5
		pop ebx		// 0x0002c8e7
		ret 4		// 0x0002c8e8
		}
	}
