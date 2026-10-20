// HullFlood.cpp: the edge-adjacency traversal that groups a hull's triangles into polygons, and the queue
// it uses. Naked listing transcriptions of phys_fn_001423 (the two owned buffers), 001425 (the coplanarity
// test of one triangle against its neighbours), 001427 + 001429 (the flood from one triangle), 001431 and
// 001433 (the flood driver), 001435 + 001437 (the clustering pass that runs it over every face) and 001655
// (the queue pop). The compare script is
// docs/reconstruction/novodex-physics/evidence/phase3-hull-flood-listing-compare.py.
//
// The object these rows share (the "clusterer", this in ecx): +0 the triangles (three vertex indices each),
// +4 the vertices, +8 the edge topology (+0xc three edge words per triangle, +0x10 an edge record of a count
// and an offset per edge, +0x14 the triangle indices those records list), +0xc and +0x10 two Containers
// (the polygon and the border lists), +0x14 a byte per triangle (set when it is already clustered), +0x18
// the polygon each triangle was put in (-1 for none), +0x1c the polygon being grown, +0x20 how many triangles
// the last flood took, +0x24 the polygon count, +0x28 / +0x30 the buffers 001423 frees, +0x2c the count of
// distinct groups. The flood's stamp (0x10123c74) is a global the listing bumps.

#include "NxPhysicsSDK.h"

extern "C" void nxHullCallContainerCtor();
extern "C" void nxHullCallContainerResize();
extern "C" void nxHullCallContainerDtor();
extern "C" void nxHullCallPlaneSet();
extern "C" void nxHullCallRadixCtor();
extern "C" void nxHullCallRadixSort();
extern "C" void nxHullCallRadixDtor();
extern "C" void nxHullCallAlloca();
extern "C" void nxHullCallNew();
extern "C" void nxHullCallDelete();
#pragma comment(linker, "/alternatename:_nxHullCallContainerCtor=??0Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallContainerResize=?Resize@Container@IceCore@@AAE_NI@Z")
#pragma comment(linker, "/alternatename:_nxHullCallContainerDtor=??1Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallPlaneSet=?Set@Plane@IceMaths@@QAEAAV12@ABVPoint@2@00@Z")
#pragma comment(linker, "/alternatename:_nxHullCallRadixCtor=??0RadixSort@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallRadixSort=?Sort@RadixSort@IceCore@@QAEAAV12@PBIIW4RadixHint@2@@Z")
#pragma comment(linker, "/alternatename:_nxHullCallRadixDtor=??1RadixSort@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxHullCallAlloca=__chkstk")
#pragma comment(linker, "/alternatename:_nxHullCallNew=??2@YAPAXI@Z")
#pragma comment(linker, "/alternatename:_nxHullCallDelete=??3@YAXPAX@Z")

// The constants the listings read: 1.0f, 0.0f, 0.001f and 0.01f, and the stamp counter.
static const unsigned nxHullOne = 0x3f800000;
static const unsigned nxHullZero = 0x00000000;
static const unsigned nxHullEpsilonSmall = 0x3a83126f;
static const unsigned nxHullEpsilon = 0x3c23d70a;
unsigned nxHullStamp = 0;

void nxHullQueuePop();
void nxHullFreeBuffers();
void nxHullCoplanar();
void nxHullFlood();
void nxHullFloodStart();
void nxHullFloodEntry();
void nxHullCluster();

// phys_fn_001655 (0x00032410, 66 B), 23 instructions
__declspec(naked) void nxHullQueuePop()
	{
	__asm
		{
		mov edx, dword ptr [ecx + 4]		// 0x00032410
		test edx, edx		// 0x00032413
		jne L3241c		// 0x00032415
		xor al, al		// 0x00032417
		ret 4		// 0x00032419
L3241c:
		mov eax, dword ptr [ecx + 0x10]		// 0x0003241c
		push edi		// 0x0003241f
		lea edi, [eax + 1]		// 0x00032420
		mov dword ptr [ecx + 0x10], edi		// 0x00032423
		mov edi, dword ptr [ecx + 8]		// 0x00032426
		mov eax, dword ptr [edi + eax*4]		// 0x00032429
		mov edi, dword ptr [esp + 8]		// 0x0003242c
		mov dword ptr [edi], eax		// 0x00032430
		cmp dword ptr [ecx + 0x10], edx		// 0x00032432
		pop edi		// 0x00032435
		jne L3244d		// 0x00032436
		mov eax, dword ptr [ecx + 4]		// 0x00032438
		test eax, eax		// 0x0003243b
		je L32446		// 0x0003243d
		mov dword ptr [ecx + 4], 0		// 0x0003243f
L32446:
		mov dword ptr [ecx + 0x10], 0		// 0x00032446
L3244d:
		mov al, 1		// 0x0003244d
		ret 4		// 0x0003244f
		}
	}

// phys_fn_001423 (0x00029a30, 51 B), 18 instructions
__declspec(naked) void nxHullFreeBuffers()
	{
	__asm
		{
		push esi		// 0x00029a30
		mov esi, ecx		// 0x00029a31
		mov eax, dword ptr [esi + 0x30]		// 0x00029a33
		test eax, eax		// 0x00029a36
		je L29a4a		// 0x00029a38
		push eax		// 0x00029a3a
		call nxHullCallDelete		// 0x00029a3b
		add esp, 4		// 0x00029a40
		mov dword ptr [esi + 0x30], 0		// 0x00029a43
L29a4a:
		mov eax, dword ptr [esi + 0x28]		// 0x00029a4a
		test eax, eax		// 0x00029a4d
		je L29a61		// 0x00029a4f
		push eax		// 0x00029a51
		call nxHullCallDelete		// 0x00029a52
		add esp, 4		// 0x00029a57
		mov dword ptr [esi + 0x28], 0		// 0x00029a5a
L29a61:
		pop esi		// 0x00029a61
		ret		// 0x00029a62
		}
	}

// phys_fn_001425 (0x00029a70, 516 B), 185 instructions
__declspec(naked) void nxHullCoplanar()
	{
	__asm
		{
		sub esp, 0x14		// 0x00029a70
		push ebx		// 0x00029a73
		push ebp		// 0x00029a74
		push esi		// 0x00029a75
		mov esi, ecx		// 0x00029a76
		mov ebx, eax		// 0x00029a78
		mov eax, dword ptr [esi + 0xc]		// 0x00029a7a
		test eax, eax		// 0x00029a7d
		push edi		// 0x00029a7f
		je L29c6a		// 0x00029a80
		mov ecx, dword ptr [esi + 0x10]		// 0x00029a86
		test ecx, ecx		// 0x00029a89
		je L29c6a		// 0x00029a8b
		cmp ebx, -1		// 0x00029a91
		je L29bc7		// 0x00029a94
		mov edx, dword ptr [eax + 4]		// 0x00029a9a
		test edx, edx		// 0x00029a9d
		mov ebp, dword ptr [eax + 8]		// 0x00029a9f
		je L29b34		// 0x00029aa2
L29aa8:
		mov eax, dword ptr [ebp]		// 0x00029aa8
		mov ecx, dword ptr [esi]		// 0x00029aab
		lea eax, [eax + eax*2]		// 0x00029aad
		mov edi, dword ptr [ecx + eax*4 + 4]		// 0x00029ab0
		lea eax, [ecx + eax*4]		// 0x00029ab4
		mov ecx, dword ptr [eax]		// 0x00029ab7
		mov eax, dword ptr [eax + 8]		// 0x00029ab9
		dec edx		// 0x00029abc
		add ebp, 4		// 0x00029abd
		cmp ebx, ecx		// 0x00029ac0
		mov dword ptr [esp + 0x10], edx		// 0x00029ac2
		je L29b2c		// 0x00029ac6
		cmp ebx, edi		// 0x00029ac8
		je L29b2c		// 0x00029aca
		cmp ebx, eax		// 0x00029acc
		je L29b2c		// 0x00029ace
		mov edx, dword ptr [esi + 4]		// 0x00029ad0
		lea eax, [eax + eax*2]		// 0x00029ad3
		lea eax, [edx + eax*4]		// 0x00029ad6
		push eax		// 0x00029ad9
		lea eax, [edi + edi*2]		// 0x00029ada
		lea eax, [edx + eax*4]		// 0x00029add
		lea ecx, [ecx + ecx*2]		// 0x00029ae0
		lea edx, [edx + ecx*4]		// 0x00029ae3
		push eax		// 0x00029ae6
		push edx		// 0x00029ae7
		lea ecx, [esp + 0x20]		// 0x00029ae8
		call nxHullCallPlaneSet		// 0x00029aec
		fld dword ptr [esp + 0x18]		// 0x00029af1
		mov ecx, dword ptr [esi + 4]		// 0x00029af5
		lea eax, [ebx + ebx*2]		// 0x00029af8
		fmul dword ptr [ecx + eax*4 + 4]		// 0x00029afb
		lea eax, [ecx + eax*4]		// 0x00029aff
		fld dword ptr [esp + 0x1c]		// 0x00029b02
		fmul dword ptr [eax + 8]		// 0x00029b06
		faddp st(1), st(0)		// 0x00029b09
		fld dword ptr [esp + 0x14]		// 0x00029b0b
		fmul dword ptr [eax]		// 0x00029b0f
		faddp st(1), st(0)		// 0x00029b11
		fadd dword ptr [esp + 0x20]		// 0x00029b13
		fcomp dword ptr [nxHullEpsilonSmall]		// 0x00029b17
		fnstsw ax		// 0x00029b1d
		test ah, 0x41		// 0x00029b1f
		je L29bbd		// 0x00029b22
		mov edx, dword ptr [esp + 0x10]		// 0x00029b28
L29b2c:
		test edx, edx		// 0x00029b2c
		jne L29aa8		// 0x00029b2e
L29b34:
		mov edi, dword ptr [esp + 0x28]		// 0x00029b34
		mov eax, dword ptr [esi]		// 0x00029b38
		mov ecx, dword ptr [esi + 4]		// 0x00029b3a
		lea edx, [edi + edi*2]		// 0x00029b3d
		lea eax, [eax + edx*4]		// 0x00029b40
		mov edx, dword ptr [eax + 8]		// 0x00029b43
		lea edx, [edx + edx*2]		// 0x00029b46
		lea edx, [ecx + edx*4]		// 0x00029b49
		push edx		// 0x00029b4c
		mov edx, dword ptr [eax + 4]		// 0x00029b4d
		mov eax, dword ptr [eax]		// 0x00029b50
		lea edx, [edx + edx*2]		// 0x00029b52
		lea edx, [ecx + edx*4]		// 0x00029b55
		lea eax, [eax + eax*2]		// 0x00029b58
		lea ecx, [ecx + eax*4]		// 0x00029b5b
		push edx		// 0x00029b5e
		push ecx		// 0x00029b5f
		lea ecx, [esp + 0x20]		// 0x00029b60
		call nxHullCallPlaneSet		// 0x00029b64
		mov eax, dword ptr [esi + 0x10]		// 0x00029b69
		mov ecx, dword ptr [eax + 4]		// 0x00029b6c
		test ecx, ecx		// 0x00029b6f
		mov edx, dword ptr [eax + 8]		// 0x00029b71
		je L29bcb		// 0x00029b74
		mov ebp, dword ptr [esi + 4]		// 0x00029b76
		lea esp, [esp]		// 0x00029b79
L29b80:
		mov eax, dword ptr [edx]		// 0x00029b80
		fld dword ptr [esp + 0x18]		// 0x00029b82
		lea eax, [eax + eax*2]		// 0x00029b86
		fmul dword ptr [ebp + eax*4 + 4]		// 0x00029b89
		lea eax, [ebp + eax*4]		// 0x00029b8d
		fld dword ptr [esp + 0x1c]		// 0x00029b91
		dec ecx		// 0x00029b95
		fmul dword ptr [eax + 8]		// 0x00029b96
		add edx, 4		// 0x00029b99
		faddp st(1), st(0)		// 0x00029b9c
		fld dword ptr [esp + 0x14]		// 0x00029b9e
		fmul dword ptr [eax]		// 0x00029ba2
		faddp st(1), st(0)		// 0x00029ba4
		fadd dword ptr [esp + 0x20]		// 0x00029ba6
		fcomp dword ptr [nxHullEpsilonSmall]		// 0x00029baa
		fnstsw ax		// 0x00029bb0
		test ah, 0x41		// 0x00029bb2
		je L29bbd		// 0x00029bb5
		test ecx, ecx		// 0x00029bb7
		jne L29b80		// 0x00029bb9
		jmp L29bcb		// 0x00029bbb
L29bbd:
		pop edi		// 0x00029bbd
		pop esi		// 0x00029bbe
		pop ebp		// 0x00029bbf
		xor al, al		// 0x00029bc0
		pop ebx		// 0x00029bc2
		add esp, 0x14		// 0x00029bc3
		ret		// 0x00029bc6
L29bc7:
		mov edi, dword ptr [esp + 0x28]		// 0x00029bc7
L29bcb:
		mov ebp, dword ptr [esi + 0xc]		// 0x00029bcb
		mov ecx, dword ptr [ebp + 4]		// 0x00029bce
		cmp ecx, dword ptr [ebp]		// 0x00029bd1
		jne L29bdf		// 0x00029bd4
		push 1		// 0x00029bd6
		mov ecx, ebp		// 0x00029bd8
		call nxHullCallContainerResize		// 0x00029bda
L29bdf:
		mov edx, dword ptr [ebp + 4]		// 0x00029bdf
		mov eax, dword ptr [ebp + 8]		// 0x00029be2
		mov dword ptr [eax + edx*4], edi		// 0x00029be5
		mov ecx, dword ptr [ebp + 4]		// 0x00029be8
		inc ecx		// 0x00029beb
		cmp ebx, -1		// 0x00029bec
		mov dword ptr [ebp + 4], ecx		// 0x00029bef
		jne L29c4b		// 0x00029bf2
		mov ecx, dword ptr [esi]		// 0x00029bf4
		lea ebx, [edi + edi*2]		// 0x00029bf6
		mov edi, dword ptr [esi + 0x10]		// 0x00029bf9
		mov edx, dword ptr [edi + 4]		// 0x00029bfc
		mov eax, dword ptr [edi]		// 0x00029bff
		shl ebx, 2		// 0x00029c01
		cmp edx, eax		// 0x00029c04
		mov ebp, dword ptr [ebx + ecx]		// 0x00029c06
		jne L29c14		// 0x00029c09
		push 1		// 0x00029c0b
		mov ecx, edi		// 0x00029c0d
		call nxHullCallContainerResize		// 0x00029c0f
L29c14:
		mov eax, dword ptr [edi + 4]		// 0x00029c14
		mov ecx, dword ptr [edi + 8]		// 0x00029c17
		mov dword ptr [ecx + eax*4], ebp		// 0x00029c1a
		inc dword ptr [edi + 4]		// 0x00029c1d
		mov edi, dword ptr [esi + 0x10]		// 0x00029c20
		mov eax, dword ptr [edi + 4]		// 0x00029c23
		cmp eax, dword ptr [edi]		// 0x00029c26
		mov edx, dword ptr [esi]		// 0x00029c28
		mov ebp, dword ptr [ebx + edx + 4]		// 0x00029c2a
		jne L29c39		// 0x00029c2e
		push 1		// 0x00029c30
		mov ecx, edi		// 0x00029c32
		call nxHullCallContainerResize		// 0x00029c34
L29c39:
		mov ecx, dword ptr [edi + 4]		// 0x00029c39
		mov edx, dword ptr [edi + 8]		// 0x00029c3c
		mov dword ptr [edx + ecx*4], ebp		// 0x00029c3f
		inc dword ptr [edi + 4]		// 0x00029c42
		mov eax, dword ptr [esi]		// 0x00029c45
		mov ebx, dword ptr [ebx + eax + 8]		// 0x00029c47
L29c4b:
		mov esi, dword ptr [esi + 0x10]		// 0x00029c4b
		mov ecx, dword ptr [esi + 4]		// 0x00029c4e
		cmp ecx, dword ptr [esi]		// 0x00029c51
		jne L29c5e		// 0x00029c53
		mov ecx, esi		// 0x00029c55
		push 1		// 0x00029c57
		call nxHullCallContainerResize		// 0x00029c59
L29c5e:
		mov eax, dword ptr [esi + 8]		// 0x00029c5e
		mov edx, dword ptr [esi + 4]		// 0x00029c61
		mov dword ptr [eax + edx*4], ebx		// 0x00029c64
		inc dword ptr [esi + 4]		// 0x00029c67
L29c6a:
		pop edi		// 0x00029c6a
		pop esi		// 0x00029c6b
		pop ebp		// 0x00029c6c
		mov al, 1		// 0x00029c6d
		pop ebx		// 0x00029c6f
		add esp, 0x14		// 0x00029c70
		ret		// 0x00029c73
		}
	}

// phys_fn_001427, phys_fn_001429 (0x00029c80, 1053 B), 345 instructions
__declspec(naked) void nxHullFlood()
	{
	__asm
		{
		mov edx, dword ptr [nxHullStamp]		// 0x00029c80
		sub esp, 0x10		// 0x00029c86
		push esi		// 0x00029c89
		mov esi, ecx		// 0x00029c8a
		mov eax, dword ptr [esi + 0x18]		// 0x00029c8c
		push edi		// 0x00029c8f
		mov edi, dword ptr [esp + 0x20]		// 0x00029c90
		mov ecx, dword ptr [eax + edi*4]		// 0x00029c94
		inc edx		// 0x00029c97
		cmp ecx, -1		// 0x00029c98
		mov dword ptr [nxHullStamp], edx		// 0x00029c9b
		jne L2a095		// 0x00029ca1
		mov eax, dword ptr [esp + 0x24]		// 0x00029ca7
		push edi		// 0x00029cab
		mov ecx, esi		// 0x00029cac
		call nxHullCoplanar		// 0x00029cae
		add esp, 4		// 0x00029cb3
		test al, al		// 0x00029cb6
		je L2a095		// 0x00029cb8
		mov ecx, dword ptr [esi + 0x18]		// 0x00029cbe
		mov edx, dword ptr [esi + 0x1c]		// 0x00029cc1
		mov dword ptr [ecx + edi*4], edx		// 0x00029cc4
		mov ecx, dword ptr [esi + 8]		// 0x00029cc7
		push ebx		// 0x00029cca
		push ebp		// 0x00029ccb
		mov ebp, dword ptr [esi + 0x20]		// 0x00029ccc
		lea eax, [edi + edi*2]		// 0x00029ccf
		mov edi, dword ptr [esi]		// 0x00029cd2
		inc ebp		// 0x00029cd4
		mov dword ptr [esi + 0x20], ebp		// 0x00029cd5
		mov edx, dword ptr [ecx + 0xc]		// 0x00029cd8
		mov ebp, dword ptr [esp + 0x24]		// 0x00029cdb
		shl eax, 2		// 0x00029cdf
		add edx, eax		// 0x00029ce2
		add eax, edi		// 0x00029ce4
		mov edi, dword ptr [eax]		// 0x00029ce6
		mov dword ptr [esp + 0x14], edi		// 0x00029ce8
		mov edi, dword ptr [eax + 4]		// 0x00029cec
		mov eax, dword ptr [eax + 8]		// 0x00029cef
		mov dword ptr [esp + 0x10], eax		// 0x00029cf2
		mov eax, dword ptr [edx]		// 0x00029cf6
		mov dword ptr [esp + 0x2c], edi		// 0x00029cf8
		mov edi, dword ptr [esi + 0x14]		// 0x00029cfc
		and eax, 0xfffffff		// 0x00029cff
		cmp byte ptr [eax + edi], 0		// 0x00029d04
		mov dword ptr [esp + 0x1c], edx		// 0x00029d08
		jne L29e29		// 0x00029d0c
		mov ecx, dword ptr [ecx + 0x10]		// 0x00029d12
		cmp word ptr [ecx + eax*8 + 2], 0		// 0x00029d15
		lea eax, [ecx + eax*8]		// 0x00029d1b
		mov dword ptr [esp + 0x18], eax		// 0x00029d1e
		mov dword ptr [esp + 0x24], 0		// 0x00029d22
		jbe L29e29		// 0x00029d2a
L29d30:
		mov edx, dword ptr [esp + 0x18]		// 0x00029d30
		mov eax, dword ptr [edx + 4]		// 0x00029d34
		add eax, dword ptr [esp + 0x24]		// 0x00029d37
		mov ecx, dword ptr [esi + 8]		// 0x00029d3b
		mov edx, dword ptr [ecx + 0x14]		// 0x00029d3e
		mov ebx, dword ptr [edx + eax*4]		// 0x00029d41
		cmp ebx, dword ptr [esp + 0x28]		// 0x00029d44
		je L29e0c		// 0x00029d48
		mov ecx, dword ptr [esi]		// 0x00029d4e
		mov edi, dword ptr [esp + 0x14]		// 0x00029d50
		lea eax, [ebx + ebx*2]		// 0x00029d54
		mov edx, dword ptr [ecx + eax*4 + 4]		// 0x00029d57
		lea eax, [ecx + eax*4]		// 0x00029d5b
		mov ecx, dword ptr [eax]		// 0x00029d5e
		cmp ecx, edi		// 0x00029d60
		mov eax, dword ptr [eax + 8]		// 0x00029d62
		jne L29d71		// 0x00029d65
		cmp edx, dword ptr [esp + 0x2c]		// 0x00029d67
		jne L29d71		// 0x00029d6b
		mov edi, eax		// 0x00029d6d
		jmp L29dc4		// 0x00029d6f
L29d71:
		cmp ecx, dword ptr [esp + 0x2c]		// 0x00029d71
		jne L29d81		// 0x00029d75
		cmp edx, dword ptr [esp + 0x14]		// 0x00029d77
		jne L29d81		// 0x00029d7b
		mov edi, eax		// 0x00029d7d
		jmp L29dc4		// 0x00029d7f
L29d81:
		cmp ecx, dword ptr [esp + 0x14]		// 0x00029d81
		jne L29d91		// 0x00029d85
		cmp eax, dword ptr [esp + 0x2c]		// 0x00029d87
		jne L29d91		// 0x00029d8b
		mov edi, edx		// 0x00029d8d
		jmp L29dc4		// 0x00029d8f
L29d91:
		cmp ecx, dword ptr [esp + 0x2c]		// 0x00029d91
		jne L29da1		// 0x00029d95
		cmp eax, dword ptr [esp + 0x14]		// 0x00029d97
		jne L29da1		// 0x00029d9b
		mov edi, edx		// 0x00029d9d
		jmp L29dc4		// 0x00029d9f
L29da1:
		cmp edx, dword ptr [esp + 0x14]		// 0x00029da1
		jne L29db1		// 0x00029da5
		cmp eax, dword ptr [esp + 0x2c]		// 0x00029da7
		jne L29db1		// 0x00029dab
		mov edi, ecx		// 0x00029dad
		jmp L29dc4		// 0x00029daf
L29db1:
		cmp edx, dword ptr [esp + 0x2c]		// 0x00029db1
		jne L29dc1		// 0x00029db5
		cmp eax, dword ptr [esp + 0x14]		// 0x00029db7
		jne L29dc1		// 0x00029dbb
		mov edi, ecx		// 0x00029dbd
		jmp L29dc4		// 0x00029dbf
L29dc1:
		or edi, 0xffffffff		// 0x00029dc1
L29dc4:
		mov edx, dword ptr [esi + 0x18]		// 0x00029dc4
		cmp dword ptr [edx + ebx*4], -1		// 0x00029dc7
		jne L29e0c		// 0x00029dcb
		mov eax, dword ptr [ebp + 4]		// 0x00029dcd
		cmp eax, dword ptr [ebp]		// 0x00029dd0
		jne L29dde		// 0x00029dd3
		push 1		// 0x00029dd5
		mov ecx, ebp		// 0x00029dd7
		call nxHullCallContainerResize		// 0x00029dd9
L29dde:
		mov ecx, dword ptr [ebp + 4]		// 0x00029dde
		mov edx, dword ptr [ebp + 8]		// 0x00029de1
		mov dword ptr [edx + ecx*4], ebx		// 0x00029de4
		mov edx, dword ptr [ebp + 4]		// 0x00029de7
		mov ecx, dword ptr [ebp]		// 0x00029dea
		inc edx		// 0x00029ded
		mov eax, edx		// 0x00029dee
		cmp eax, ecx		// 0x00029df0
		mov dword ptr [ebp + 4], edx		// 0x00029df2
		jne L29e00		// 0x00029df5
		push 1		// 0x00029df7
		mov ecx, ebp		// 0x00029df9
		call nxHullCallContainerResize		// 0x00029dfb
L29e00:
		mov eax, dword ptr [ebp + 4]		// 0x00029e00
		mov ecx, dword ptr [ebp + 8]		// 0x00029e03
		mov dword ptr [ecx + eax*4], edi		// 0x00029e06
		inc dword ptr [ebp + 4]		// 0x00029e09
L29e0c:
		mov edx, dword ptr [esp + 0x18]		// 0x00029e0c
		mov eax, dword ptr [esp + 0x24]		// 0x00029e10
		movzx ecx, word ptr [edx + 2]		// 0x00029e14
		inc eax		// 0x00029e18
		cmp eax, ecx		// 0x00029e19
		mov dword ptr [esp + 0x24], eax		// 0x00029e1b
		jb L29d30		// 0x00029e1f
		mov edx, dword ptr [esp + 0x1c]		// 0x00029e25
L29e29:
		mov edx, dword ptr [edx + 4]		// 0x00029e29
		mov eax, dword ptr [esi + 0x14]		// 0x00029e2c
		and edx, 0xfffffff		// 0x00029e2f
		cmp byte ptr [edx + eax], 0		// 0x00029e35
		jne L29f57		// 0x00029e39
		mov ecx, dword ptr [esi + 8]		// 0x00029e3f
		mov eax, dword ptr [ecx + 0x10]		// 0x00029e42
		cmp word ptr [eax + edx*8 + 2], 0		// 0x00029e45
		lea eax, [eax + edx*8]		// 0x00029e4b
		mov dword ptr [esp + 0x18], eax		// 0x00029e4e
		mov dword ptr [esp + 0x24], 0		// 0x00029e52
		jbe L29f57		// 0x00029e5a
L29e60:
		mov ecx, dword ptr [esp + 0x18]		// 0x00029e60
		mov edx, dword ptr [ecx + 4]		// 0x00029e64
		mov ecx, dword ptr [esp + 0x24]		// 0x00029e67
		mov eax, dword ptr [esi + 8]		// 0x00029e6b
		add edx, ecx		// 0x00029e6e
		mov ecx, dword ptr [eax + 0x14]		// 0x00029e70
		mov ebx, dword ptr [ecx + edx*4]		// 0x00029e73
		cmp ebx, dword ptr [esp + 0x28]		// 0x00029e76
		je L29f3e		// 0x00029e7a
		mov eax, dword ptr [esi]		// 0x00029e80
		mov edi, dword ptr [esp + 0x2c]		// 0x00029e82
		lea edx, [ebx + ebx*2]		// 0x00029e86
		mov ecx, dword ptr [eax + edx*4]		// 0x00029e89
		cmp ecx, edi		// 0x00029e8c
		lea eax, [eax + edx*4]		// 0x00029e8e
		mov edx, dword ptr [eax + 4]		// 0x00029e91
		mov eax, dword ptr [eax + 8]		// 0x00029e94
		jne L29ea3		// 0x00029e97
		cmp edx, dword ptr [esp + 0x10]		// 0x00029e99
		jne L29ea3		// 0x00029e9d
		mov edi, eax		// 0x00029e9f
		jmp L29ef6		// 0x00029ea1
L29ea3:
		cmp ecx, dword ptr [esp + 0x10]		// 0x00029ea3
		jne L29eb3		// 0x00029ea7
		cmp edx, dword ptr [esp + 0x2c]		// 0x00029ea9
		jne L29eb3		// 0x00029ead
		mov edi, eax		// 0x00029eaf
		jmp L29ef6		// 0x00029eb1
L29eb3:
		cmp ecx, dword ptr [esp + 0x2c]		// 0x00029eb3
		jne L29ec3		// 0x00029eb7
		cmp eax, dword ptr [esp + 0x10]		// 0x00029eb9
		jne L29ec3		// 0x00029ebd
		mov edi, edx		// 0x00029ebf
		jmp L29ef6		// 0x00029ec1
L29ec3:
		cmp ecx, dword ptr [esp + 0x10]		// 0x00029ec3
		jne L29ed3		// 0x00029ec7
		cmp eax, dword ptr [esp + 0x2c]		// 0x00029ec9
		jne L29ed3		// 0x00029ecd
		mov edi, edx		// 0x00029ecf
		jmp L29ef6		// 0x00029ed1
L29ed3:
		cmp edx, dword ptr [esp + 0x2c]		// 0x00029ed3
		jne L29ee3		// 0x00029ed7
		cmp eax, dword ptr [esp + 0x10]		// 0x00029ed9
		jne L29ee3		// 0x00029edd
		mov edi, ecx		// 0x00029edf
		jmp L29ef6		// 0x00029ee1
L29ee3:
		cmp edx, dword ptr [esp + 0x10]		// 0x00029ee3
		jne L29ef3		// 0x00029ee7
		cmp eax, dword ptr [esp + 0x2c]		// 0x00029ee9
		jne L29ef3		// 0x00029eed
		mov edi, ecx		// 0x00029eef
		jmp L29ef6		// 0x00029ef1
L29ef3:
		or edi, 0xffffffff		// 0x00029ef3
L29ef6:
		mov ecx, dword ptr [esi + 0x18]		// 0x00029ef6
		cmp dword ptr [ecx + ebx*4], -1		// 0x00029ef9
		jne L29f3e		// 0x00029efd
		mov edx, dword ptr [ebp + 4]		// 0x00029eff
		cmp edx, dword ptr [ebp]		// 0x00029f02
		jne L29f10		// 0x00029f05
		push 1		// 0x00029f07
		mov ecx, ebp		// 0x00029f09
		call nxHullCallContainerResize		// 0x00029f0b
L29f10:
		mov eax, dword ptr [ebp + 4]		// 0x00029f10
		mov ecx, dword ptr [ebp + 8]		// 0x00029f13
		mov dword ptr [ecx + eax*4], ebx		// 0x00029f16
		mov edx, dword ptr [ebp + 4]		// 0x00029f19
		mov ecx, dword ptr [ebp]		// 0x00029f1c
		inc edx		// 0x00029f1f
		mov eax, edx		// 0x00029f20
		cmp eax, ecx		// 0x00029f22
		mov dword ptr [ebp + 4], edx		// 0x00029f24
		jne L29f32		// 0x00029f27
		push 1		// 0x00029f29
		mov ecx, ebp		// 0x00029f2b
		call nxHullCallContainerResize		// 0x00029f2d
L29f32:
		mov edx, dword ptr [ebp + 4]		// 0x00029f32
		mov eax, dword ptr [ebp + 8]		// 0x00029f35
		mov dword ptr [eax + edx*4], edi		// 0x00029f38
		inc dword ptr [ebp + 4]		// 0x00029f3b
L29f3e:
		mov ecx, dword ptr [esp + 0x18]		// 0x00029f3e
		mov eax, dword ptr [esp + 0x24]		// 0x00029f42
		movzx edx, word ptr [ecx + 2]		// 0x00029f46
		inc eax		// 0x00029f4a
		cmp eax, edx		// 0x00029f4b
		mov dword ptr [esp + 0x24], eax		// 0x00029f4d
		jb L29e60		// 0x00029f51
L29f57:
		mov eax, dword ptr [esp + 0x1c]		// 0x00029f57
		mov eax, dword ptr [eax + 8]		// 0x00029f5b
		mov ecx, dword ptr [esi + 0x14]		// 0x00029f5e
		and eax, 0xfffffff		// 0x00029f61
		cmp byte ptr [eax + ecx], 0		// 0x00029f66
		jne L2a093		// 0x00029f6a
		mov edx, dword ptr [esi + 8]		// 0x00029f70
		mov ecx, dword ptr [edx + 0x10]		// 0x00029f73
		cmp word ptr [ecx + eax*8 + 2], 0		// 0x00029f76
		lea eax, [ecx + eax*8]		// 0x00029f7c
		mov dword ptr [esp + 0x24], eax		// 0x00029f7f
		mov dword ptr [esp + 0x2c], 0		// 0x00029f83
		jbe L2a093		// 0x00029f8b
		jmp L29fa0		// 0x00029f91
L29f93:
		mov eax, dword ptr [esp + 0x24]		// 0x00029f93
		jmp L29fa0		// 0x00029f97
		lea esp, [esp]		// 0x00029f99
L29fa0:
		mov edx, dword ptr [eax + 4]		// 0x00029fa0
		mov ecx, dword ptr [esp + 0x2c]		// 0x00029fa3
		mov eax, dword ptr [esi + 8]		// 0x00029fa7
		add edx, ecx		// 0x00029faa
		mov ecx, dword ptr [eax + 0x14]		// 0x00029fac
		mov ebx, dword ptr [ecx + edx*4]		// 0x00029faf
		cmp ebx, dword ptr [esp + 0x28]		// 0x00029fb2
		je L2a07a		// 0x00029fb6
		mov eax, dword ptr [esi]		// 0x00029fbc
		mov edi, dword ptr [esp + 0x14]		// 0x00029fbe
		lea edx, [ebx + ebx*2]		// 0x00029fc2
		mov ecx, dword ptr [eax + edx*4]		// 0x00029fc5
		cmp ecx, edi		// 0x00029fc8
		lea eax, [eax + edx*4]		// 0x00029fca
		mov edx, dword ptr [eax + 4]		// 0x00029fcd
		mov eax, dword ptr [eax + 8]		// 0x00029fd0
		jne L29fdf		// 0x00029fd3
		cmp edx, dword ptr [esp + 0x10]		// 0x00029fd5
		jne L29fdf		// 0x00029fd9
		mov edi, eax		// 0x00029fdb
		jmp L2a032		// 0x00029fdd
L29fdf:
		cmp ecx, dword ptr [esp + 0x10]		// 0x00029fdf
		jne L29fef		// 0x00029fe3
		cmp edx, dword ptr [esp + 0x14]		// 0x00029fe5
		jne L29fef		// 0x00029fe9
		mov edi, eax		// 0x00029feb
		jmp L2a032		// 0x00029fed
L29fef:
		cmp ecx, dword ptr [esp + 0x14]		// 0x00029fef
		jne L29fff		// 0x00029ff3
		cmp eax, dword ptr [esp + 0x10]		// 0x00029ff5
		jne L29fff		// 0x00029ff9
		mov edi, edx		// 0x00029ffb
		jmp L2a032		// 0x00029ffd
L29fff:
		cmp ecx, dword ptr [esp + 0x10]		// 0x00029fff
		jne L2a00f		// 0x0002a003
		cmp eax, dword ptr [esp + 0x14]		// 0x0002a005
		jne L2a00f		// 0x0002a009
		mov edi, edx		// 0x0002a00b
		jmp L2a032		// 0x0002a00d
L2a00f:
		cmp edx, dword ptr [esp + 0x14]		// 0x0002a00f
		jne L2a01f		// 0x0002a013
		cmp eax, dword ptr [esp + 0x10]		// 0x0002a015
		jne L2a01f		// 0x0002a019
		mov edi, ecx		// 0x0002a01b
		jmp L2a032		// 0x0002a01d
L2a01f:
		cmp edx, dword ptr [esp + 0x10]		// 0x0002a01f
		jne L2a02f		// 0x0002a023
		cmp eax, dword ptr [esp + 0x14]		// 0x0002a025
		jne L2a02f		// 0x0002a029
		mov edi, ecx		// 0x0002a02b
		jmp L2a032		// 0x0002a02d
L2a02f:
		or edi, 0xffffffff		// 0x0002a02f
L2a032:
		mov ecx, dword ptr [esi + 0x18]		// 0x0002a032
		cmp dword ptr [ecx + ebx*4], -1		// 0x0002a035
		jne L2a07a		// 0x0002a039
		mov edx, dword ptr [ebp + 4]		// 0x0002a03b
		cmp edx, dword ptr [ebp]		// 0x0002a03e
		jne L2a04c		// 0x0002a041
		push 1		// 0x0002a043
		mov ecx, ebp		// 0x0002a045
		call nxHullCallContainerResize		// 0x0002a047
L2a04c:
		mov eax, dword ptr [ebp + 4]		// 0x0002a04c
		mov ecx, dword ptr [ebp + 8]		// 0x0002a04f
		mov dword ptr [ecx + eax*4], ebx		// 0x0002a052
		mov edx, dword ptr [ebp + 4]		// 0x0002a055
		mov ecx, dword ptr [ebp]		// 0x0002a058
		inc edx		// 0x0002a05b
		mov eax, edx		// 0x0002a05c
		cmp eax, ecx		// 0x0002a05e
		mov dword ptr [ebp + 4], edx		// 0x0002a060
		jne L2a06e		// 0x0002a063
		push 1		// 0x0002a065
		mov ecx, ebp		// 0x0002a067
		call nxHullCallContainerResize		// 0x0002a069
L2a06e:
		mov edx, dword ptr [ebp + 4]		// 0x0002a06e
		mov eax, dword ptr [ebp + 8]		// 0x0002a071
		mov dword ptr [eax + edx*4], edi		// 0x0002a074
		inc dword ptr [ebp + 4]		// 0x0002a077
L2a07a:
		mov ecx, dword ptr [esp + 0x24]		// 0x0002a07a
		mov eax, dword ptr [esp + 0x2c]		// 0x0002a07e
		movzx edx, word ptr [ecx + 2]		// 0x0002a082
		inc eax		// 0x0002a086
		cmp eax, edx		// 0x0002a087
		mov dword ptr [esp + 0x2c], eax		// 0x0002a089
		jb L29f93		// 0x0002a08d
L2a093:
		pop ebp		// 0x0002a093
		pop ebx		// 0x0002a094
L2a095:
		pop edi		// 0x0002a095
		pop esi		// 0x0002a096
		add esp, 0x10		// 0x0002a097
		ret 0xc		// 0x0002a09a
		}
	}

// phys_fn_001431 (0x0002a0a0, 186 B), 55 instructions
__declspec(naked) void nxHullFloodStart()
	{
	__asm
		{
		sub esp, 0x18		// 0x0002a0a0
		push esi		// 0x0002a0a3
		mov esi, ecx		// 0x0002a0a4
		lea ecx, [esp + 8]		// 0x0002a0a6
		call nxHullCallContainerCtor		// 0x0002a0aa
		mov eax, dword ptr [esp + 0xc]		// 0x0002a0af
		cmp eax, dword ptr [esp + 8]		// 0x0002a0b3
		mov dword ptr [esp + 0x18], 0		// 0x0002a0b7
		jne L2a0cc		// 0x0002a0bf
		push 1		// 0x0002a0c1
		lea ecx, [esp + 0xc]		// 0x0002a0c3
		call nxHullCallContainerResize		// 0x0002a0c7
L2a0cc:
		mov eax, dword ptr [esp + 0xc]		// 0x0002a0cc
		mov ecx, dword ptr [esp + 0x20]		// 0x0002a0d0
		mov edx, dword ptr [esp + 0x10]		// 0x0002a0d4
		mov dword ptr [edx + eax*4], ecx		// 0x0002a0d8
		mov eax, dword ptr [esp + 0xc]		// 0x0002a0db
		mov ecx, dword ptr [esp + 8]		// 0x0002a0df
		inc eax		// 0x0002a0e3
		cmp eax, ecx		// 0x0002a0e4
		mov dword ptr [esp + 0xc], eax		// 0x0002a0e6
		jne L2a0f7		// 0x0002a0ea
		push 1		// 0x0002a0ec
		lea ecx, [esp + 0xc]		// 0x0002a0ee
		call nxHullCallContainerResize		// 0x0002a0f2
L2a0f7:
		mov edx, dword ptr [esp + 0xc]		// 0x0002a0f7
		mov ecx, dword ptr [esp + 0x10]		// 0x0002a0fb
		mov dword ptr [ecx + edx*4], 0xffffffff		// 0x0002a0ff
		inc dword ptr [esp + 0xc]		// 0x0002a106
		je L2a14a		// 0x0002a10a
		lea esp, [esp]		// 0x0002a10c
L2a110:
		lea eax, [esp + 4]		// 0x0002a110
		push eax		// 0x0002a114
		lea ecx, [esp + 0xc]		// 0x0002a115
		call nxHullQueuePop		// 0x0002a119
		lea ecx, [esp + 0x20]		// 0x0002a11e
		push ecx		// 0x0002a122
		lea ecx, [esp + 0xc]		// 0x0002a123
		call nxHullQueuePop		// 0x0002a127
		mov edx, dword ptr [esp + 0x20]		// 0x0002a12c
		mov eax, dword ptr [esp + 4]		// 0x0002a130
		push edx		// 0x0002a134
		push eax		// 0x0002a135
		lea ecx, [esp + 0x10]		// 0x0002a136
		push ecx		// 0x0002a13a
		mov ecx, esi		// 0x0002a13b
		call nxHullFlood		// 0x0002a13d
		mov eax, dword ptr [esp + 0xc]		// 0x0002a142
		test eax, eax		// 0x0002a146
		jne L2a110		// 0x0002a148
L2a14a:
		lea ecx, [esp + 8]		// 0x0002a14a
		call nxHullCallContainerDtor		// 0x0002a14e
		pop esi		// 0x0002a153
		add esp, 0x18		// 0x0002a154
		ret 8		// 0x0002a157
		}
	}

// phys_fn_001433 (0x0002a160, 39 B), 13 instructions
__declspec(naked) void nxHullFloodEntry()
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x0002a160
		cmp eax, dword ptr [esp + 8]		// 0x0002a164
		jb L2a16f		// 0x0002a168
		xor al, al		// 0x0002a16a
		ret 8		// 0x0002a16c
L2a16f:
		xor edx, edx		// 0x0002a16f
		push -1		// 0x0002a171
		push eax		// 0x0002a173
		mov dword ptr [ecx + 0x20], edx		// 0x0002a174
		mov dword ptr [nxHullStamp], edx		// 0x0002a177
		call nxHullFloodStart		// 0x0002a17d
		mov al, 1		// 0x0002a182
		ret 8		// 0x0002a184
		}
	}

// phys_fn_001435, phys_fn_001437 (0x0002a190, 1144 B), 421 instructions
__declspec(naked) void nxHullCluster()
	{
	__asm
		{
		push ebp		// 0x0002a190
		mov ebp, esp		// 0x0002a191
		sub esp, 0x60		// 0x0002a193
		mov eax, dword ptr [ebp + 0x18]		// 0x0002a196
		push ebx		// 0x0002a199
		push esi		// 0x0002a19a
		push edi		// 0x0002a19b
		mov edi, dword ptr [eax]		// 0x0002a19c
		test edi, edi		// 0x0002a19e
		mov esi, ecx		// 0x0002a1a0
		je L2a1ee		// 0x0002a1a2
		mov ebx, dword ptr [eax + 4]		// 0x0002a1a4
		test ebx, ebx		// 0x0002a1a7
		mov dword ptr [ebp - 8], ebx		// 0x0002a1a9
		je L2a1ee		// 0x0002a1ac
		mov ecx, dword ptr [eax + 0x10]		// 0x0002a1ae
		test ecx, ecx		// 0x0002a1b1
		mov dword ptr [ebp - 0x1c], ecx		// 0x0002a1b3
		je L2a1ee		// 0x0002a1b6
		mov eax, dword ptr [eax + 0x14]		// 0x0002a1b8
		test eax, eax		// 0x0002a1bb
		mov dword ptr [ebp - 0x20], eax		// 0x0002a1bd
		je L2a1ee		// 0x0002a1c0
		mov eax, edi		// 0x0002a1c2
		add eax, 3		// 0x0002a1c4
		and eax, 0xfffffffc		// 0x0002a1c7
		call nxHullCallAlloca		// 0x0002a1ca
		mov eax, esp		// 0x0002a1cf
		test eax, eax		// 0x0002a1d1
		mov dword ptr [ebp - 0xc], eax		// 0x0002a1d3
		je L2a1ee		// 0x0002a1d6
		mov eax, edi		// 0x0002a1d8
		add eax, 3		// 0x0002a1da
		and eax, 0xfffffffc		// 0x0002a1dd
		call nxHullCallAlloca		// 0x0002a1e0
		mov eax, esp		// 0x0002a1e5
		test eax, eax		// 0x0002a1e7
		mov dword ptr [ebp - 0x28], eax		// 0x0002a1e9
		jne L2a1fc		// 0x0002a1ec
L2a1ee:
		xor al, al		// 0x0002a1ee
		lea esp, [ebp - 0x6c]		// 0x0002a1f0
		pop edi		// 0x0002a1f3
		pop esi		// 0x0002a1f4
		pop ebx		// 0x0002a1f5
		mov esp, ebp		// 0x0002a1f6
		pop ebp		// 0x0002a1f8
		ret 0x14		// 0x0002a1f9
L2a1fc:
		mov ecx, dword ptr [ebp - 0xc]		// 0x0002a1fc
		mov dword ptr [ebp - 0x10], ecx		// 0x0002a1ff
		mov ecx, dword ptr [ebp - 0x1c]		// 0x0002a202
		add ecx, 4		// 0x0002a205
		mov dword ptr [ebp - 0x14], eax		// 0x0002a208
		mov dword ptr [ebp - 0x18], ecx		// 0x0002a20b
		mov dword ptr [ebp - 0x1c], edi		// 0x0002a20e
L2a211:
		movzx eax, word ptr [ecx - 2]		// 0x0002a211
		cmp eax, 1		// 0x0002a215
		mov byte ptr [ebp - 1], 0		// 0x0002a218
		mov byte ptr [ebp - 2], 0		// 0x0002a21c
		je L2a312		// 0x0002a220
		cmp eax, 2		// 0x0002a226
		jne L2a312		// 0x0002a229
		mov edx, dword ptr [ecx]		// 0x0002a22f
		mov eax, dword ptr [ebp - 0x20]		// 0x0002a231
		mov ecx, dword ptr [eax + edx*4]		// 0x0002a234
		mov edi, dword ptr [ebx + 4]		// 0x0002a237
		lea eax, [eax + edx*4]		// 0x0002a23a
		mov edx, dword ptr [eax + 4]		// 0x0002a23d
		lea eax, [ecx + ecx*2]		// 0x0002a240
		mov ecx, dword ptr [ebp + 0xc]		// 0x0002a243
		lea eax, [ecx + eax*4]		// 0x0002a246
		mov ecx, dword ptr [eax]		// 0x0002a249
		mov dword ptr [ebp - 0x24], edx		// 0x0002a24b
		mov edx, dword ptr [ebx]		// 0x0002a24e
		cmp ecx, edx		// 0x0002a250
		mov ebx, dword ptr [eax + 4]		// 0x0002a252
		mov eax, dword ptr [eax + 8]		// 0x0002a255
		jne L2a262		// 0x0002a258
		cmp ebx, edi		// 0x0002a25a
		jne L2a262		// 0x0002a25c
		mov ebx, eax		// 0x0002a25e
		jmp L2a299		// 0x0002a260
L2a262:
		cmp ecx, edi		// 0x0002a262
		jne L2a26e		// 0x0002a264
		cmp ebx, edx		// 0x0002a266
		jne L2a26e		// 0x0002a268
		mov ebx, eax		// 0x0002a26a
		jmp L2a299		// 0x0002a26c
L2a26e:
		cmp ecx, edx		// 0x0002a26e
		jne L2a276		// 0x0002a270
		cmp eax, edi		// 0x0002a272
		je L2a299		// 0x0002a274
L2a276:
		cmp ecx, edi		// 0x0002a276
		jne L2a27e		// 0x0002a278
		cmp eax, edx		// 0x0002a27a
		je L2a299		// 0x0002a27c
L2a27e:
		cmp ebx, edx		// 0x0002a27e
		jne L2a28a		// 0x0002a280
		cmp eax, edi		// 0x0002a282
		jne L2a28a		// 0x0002a284
		mov ebx, ecx		// 0x0002a286
		jmp L2a299		// 0x0002a288
L2a28a:
		cmp ebx, edi		// 0x0002a28a
		jne L2a296		// 0x0002a28c
		cmp eax, edx		// 0x0002a28e
		jne L2a296		// 0x0002a290
		mov ebx, ecx		// 0x0002a292
		jmp L2a299		// 0x0002a294
L2a296:
		or ebx, 0xffffffff		// 0x0002a296
L2a299:
		mov eax, dword ptr [ebp - 0x24]		// 0x0002a299
		mov edi, dword ptr [ebp + 0x14]		// 0x0002a29c
		lea edx, [eax + eax*2]		// 0x0002a29f
		mov eax, dword ptr [ebp + 0xc]		// 0x0002a2a2
		mov ecx, dword ptr [eax + edx*4 + 8]		// 0x0002a2a5
		lea eax, [eax + edx*4]		// 0x0002a2a9
		lea ecx, [ecx + ecx*2]		// 0x0002a2ac
		lea edx, [edi + ecx*4]		// 0x0002a2af
		mov ecx, dword ptr [eax + 4]		// 0x0002a2b2
		mov eax, dword ptr [eax]		// 0x0002a2b5
		lea ecx, [ecx + ecx*2]		// 0x0002a2b7
		push edx		// 0x0002a2ba
		lea edx, [edi + ecx*4]		// 0x0002a2bb
		lea eax, [eax + eax*2]		// 0x0002a2be
		lea ecx, [edi + eax*4]		// 0x0002a2c1
		push edx		// 0x0002a2c4
		push ecx		// 0x0002a2c5
		lea ecx, [ebp - 0x38]		// 0x0002a2c6
		call nxHullCallPlaneSet		// 0x0002a2c9
		fld dword ptr [ebp - 0x30]		// 0x0002a2ce
		lea edx, [ebx + ebx*2]		// 0x0002a2d1
		fmul dword ptr [edi + edx*4 + 8]		// 0x0002a2d4
		lea eax, [edi + edx*4]		// 0x0002a2d8
		fld dword ptr [ebp - 0x34]		// 0x0002a2db
		fmul dword ptr [eax + 4]		// 0x0002a2de
		faddp st(1), st(0)		// 0x0002a2e1
		fld dword ptr [ebp - 0x38]		// 0x0002a2e3
		fmul dword ptr [eax]		// 0x0002a2e6
		faddp st(1), st(0)		// 0x0002a2e8
		fadd dword ptr [ebp - 0x2c]		// 0x0002a2ea
		fcom dword ptr [nxHullEpsilon]		// 0x0002a2ed
		fnstsw ax		// 0x0002a2f3
		test ah, 0x41		// 0x0002a2f5
		jne L2a2fe		// 0x0002a2f8
		mov byte ptr [ebp - 1], 1		// 0x0002a2fa
L2a2fe:
		mov ebx, dword ptr [ebp - 8]		// 0x0002a2fe
		fabs		// 0x0002a301
		fcomp dword ptr [nxHullEpsilon]		// 0x0002a303
		fnstsw ax		// 0x0002a309
		test ah, 0x41		// 0x0002a30b
		jne L2a31a		// 0x0002a30e
		jmp L2a316		// 0x0002a310
L2a312:
		mov byte ptr [ebp - 1], 1		// 0x0002a312
L2a316:
		mov byte ptr [ebp - 2], 1		// 0x0002a316
L2a31a:
		mov eax, dword ptr [ebp - 0x10]		// 0x0002a31a
		mov cl, byte ptr [ebp - 1]		// 0x0002a31d
		mov dl, byte ptr [ebp - 2]		// 0x0002a320
		mov byte ptr [eax], cl		// 0x0002a323
		mov ecx, dword ptr [ebp - 0x18]		// 0x0002a325
		inc eax		// 0x0002a328
		mov dword ptr [ebp - 0x10], eax		// 0x0002a329
		mov eax, dword ptr [ebp - 0x14]		// 0x0002a32c
		mov byte ptr [eax], dl		// 0x0002a32f
		inc eax		// 0x0002a331
		mov dword ptr [ebp - 0x14], eax		// 0x0002a332
		mov eax, dword ptr [ebp - 0x1c]		// 0x0002a335
		add ecx, 8		// 0x0002a338
		add ebx, 8		// 0x0002a33b
		dec eax		// 0x0002a33e
		mov dword ptr [ebp - 0x18], ecx		// 0x0002a33f
		mov dword ptr [ebp - 8], ebx		// 0x0002a342
		mov dword ptr [ebp - 0x1c], eax		// 0x0002a345
		jne L2a211		// 0x0002a348
		mov ebx, dword ptr [ebp + 8]		// 0x0002a34e
		lea edi, [ebx*4]		// 0x0002a351
		push edi		// 0x0002a358
		call nxHullCallNew		// 0x0002a359
		mov edx, eax		// 0x0002a35e
		mov ecx, edi		// 0x0002a360
		mov dword ptr [esi + 0x30], edx		// 0x0002a362
		mov edi, edx		// 0x0002a365
		mov edx, ecx		// 0x0002a367
		shr ecx, 2		// 0x0002a369
		or eax, 0xffffffff		// 0x0002a36c
		rep stosd		// 0x0002a36f
		mov ecx, edx		// 0x0002a371
		mov edx, dword ptr [ebp + 0x14]		// 0x0002a373
		and ecx, 3		// 0x0002a376
		rep stosb		// 0x0002a379
		mov eax, dword ptr [ebp + 0x18]		// 0x0002a37b
		mov ecx, dword ptr [ebp + 0xc]		// 0x0002a37e
		xor edi, edi		// 0x0002a381
		mov dword ptr [esi + 8], eax		// 0x0002a383
		mov eax, dword ptr [ebp - 0x28]		// 0x0002a386
		mov dword ptr [esi], ecx		// 0x0002a389
		mov ecx, dword ptr [esi + 0x30]		// 0x0002a38b
		add esp, 4		// 0x0002a38e
		cmp ebx, edi		// 0x0002a391
		mov dword ptr [esi + 0x2c], edi		// 0x0002a393
		mov dword ptr [esi + 4], edx		// 0x0002a396
		mov dword ptr [esi + 0x14], eax		// 0x0002a399
		mov dword ptr [esi + 0xc], edi		// 0x0002a39c
		mov dword ptr [esi + 0x10], edi		// 0x0002a39f
		mov dword ptr [esi + 0x18], ecx		// 0x0002a3a2
		je L2a3e0		// 0x0002a3a5
L2a3a7:
		mov edx, dword ptr [esi + 0x30]		// 0x0002a3a7
		mov eax, dword ptr [edx]		// 0x0002a3aa
		xor ecx, ecx		// 0x0002a3ac
		cmp eax, -1		// 0x0002a3ae
		je L2a3c1		// 0x0002a3b1
		mov eax, edx		// 0x0002a3b3
L2a3b5:
		mov edx, dword ptr [eax + 4]		// 0x0002a3b5
		add eax, 4		// 0x0002a3b8
		inc ecx		// 0x0002a3bb
		cmp edx, -1		// 0x0002a3bc
		jne L2a3b5		// 0x0002a3bf
L2a3c1:
		mov eax, dword ptr [esi + 0x2c]		// 0x0002a3c1
		push ebx		// 0x0002a3c4
		push ecx		// 0x0002a3c5
		mov ecx, esi		// 0x0002a3c6
		mov dword ptr [esi + 0x1c], eax		// 0x0002a3c8
		call nxHullFloodEntry		// 0x0002a3cb
		mov edx, dword ptr [esi + 0x20]		// 0x0002a3d0
		mov ecx, dword ptr [esi + 0x2c]		// 0x0002a3d3
		add edi, edx		// 0x0002a3d6
		inc ecx		// 0x0002a3d8
		cmp edi, ebx		// 0x0002a3d9
		mov dword ptr [esi + 0x2c], ecx		// 0x0002a3db
		jne L2a3a7		// 0x0002a3de
L2a3e0:
		mov edi, dword ptr [esi + 0x2c]		// 0x0002a3e0
		shl edi, 2		// 0x0002a3e3
		mov eax, edi		// 0x0002a3e6
		add eax, 3		// 0x0002a3e8
		and eax, 0xfffffffc		// 0x0002a3eb
		call nxHullCallAlloca		// 0x0002a3ee
		mov edx, esp		// 0x0002a3f3
		mov ecx, edi		// 0x0002a3f5
		mov edi, edx		// 0x0002a3f7
		mov dword ptr [ebp - 0x14], edx		// 0x0002a3f9
		mov edx, ecx		// 0x0002a3fc
		xor eax, eax		// 0x0002a3fe
		shr ecx, 2		// 0x0002a400
		rep stosd		// 0x0002a403
		mov ecx, edx		// 0x0002a405
		and ecx, 3		// 0x0002a407
		rep stosb		// 0x0002a40a
		lea eax, [ebx*4]		// 0x0002a40c
		add eax, 3		// 0x0002a413
		and eax, 0xfffffffc		// 0x0002a416
		call nxHullCallAlloca		// 0x0002a419
		xor eax, eax		// 0x0002a41e
		cmp ebx, eax		// 0x0002a420
		mov edi, esp		// 0x0002a422
		mov dword ptr [ebp - 0x18], edi		// 0x0002a424
		mov dword ptr [ebp - 0x1c], eax		// 0x0002a427
		jbe L2a493		// 0x0002a42a
		mov dword ptr [ebp - 8], eax		// 0x0002a42c
		nop		// 0x0002a42f
L2a430:
		mov eax, dword ptr [ebp + 0x18]		// 0x0002a430
		mov edx, dword ptr [ebp - 8]		// 0x0002a433
		mov eax, dword ptr [eax + 0xc]		// 0x0002a436
		mov ecx, dword ptr [ebp - 0xc]		// 0x0002a439
		add eax, edx		// 0x0002a43c
		mov edx, dword ptr [eax]		// 0x0002a43e
		and edx, 0xfffffff		// 0x0002a440
		cmp byte ptr [edx + ecx], 0		// 0x0002a446
		mov dword ptr [ebp + 8], 0		// 0x0002a44a
		je L2a45a		// 0x0002a451
		mov dword ptr [ebp + 8], 1		// 0x0002a453
L2a45a:
		mov edx, dword ptr [eax + 4]		// 0x0002a45a
		and edx, 0xfffffff		// 0x0002a45d
		cmp byte ptr [edx + ecx], 0		// 0x0002a463
		mov edx, dword ptr [ebp + 8]		// 0x0002a467
		je L2a46d		// 0x0002a46a
		inc edx		// 0x0002a46c
L2a46d:
		mov eax, dword ptr [eax + 8]		// 0x0002a46d
		and eax, 0xfffffff		// 0x0002a470
		cmp byte ptr [eax + ecx], 0		// 0x0002a475
		je L2a47c		// 0x0002a479
		inc edx		// 0x0002a47b
L2a47c:
		mov eax, dword ptr [ebp - 0x1c]		// 0x0002a47c
		mov dword ptr [edi + eax*4], edx		// 0x0002a47f
		mov edx, dword ptr [ebp - 8]		// 0x0002a482
		inc eax		// 0x0002a485
		add edx, 0xc		// 0x0002a486
		cmp eax, ebx		// 0x0002a489
		mov dword ptr [ebp - 0x1c], eax		// 0x0002a48b
		mov dword ptr [ebp - 8], edx		// 0x0002a48e
		jb L2a430		// 0x0002a491
L2a493:
		xor eax, eax		// 0x0002a493
		test ebx, ebx		// 0x0002a495
		jbe L2a4b7		// 0x0002a497
		lea esp, [esp]		// 0x0002a499
L2a4a0:
		mov ecx, dword ptr [esi + 0x30]		// 0x0002a4a0
		mov edi, dword ptr [edi + eax*4]		// 0x0002a4a3
		mov ecx, dword ptr [ecx + eax*4]		// 0x0002a4a6
		mov edx, dword ptr [ebp - 0x14]		// 0x0002a4a9
		add dword ptr [edx + ecx*4], edi		// 0x0002a4ac
		mov edi, dword ptr [ebp - 0x18]		// 0x0002a4af
		inc eax		// 0x0002a4b2
		cmp eax, ebx		// 0x0002a4b3
		jb L2a4a0		// 0x0002a4b5
L2a4b7:
		xor eax, eax		// 0x0002a4b7
		test ebx, ebx		// 0x0002a4b9
		jbe L2a4d4		// 0x0002a4bb
		lea ecx, [ecx]		// 0x0002a4bd
L2a4c0:
		mov edx, dword ptr [esi + 0x30]		// 0x0002a4c0
		mov ecx, dword ptr [edx + eax*4]		// 0x0002a4c3
		mov edx, dword ptr [ebp - 0x14]		// 0x0002a4c6
		mov ecx, dword ptr [edx + ecx*4]		// 0x0002a4c9
		mov dword ptr [edi + eax*4], ecx		// 0x0002a4cc
		inc eax		// 0x0002a4cf
		cmp eax, ebx		// 0x0002a4d0
		jb L2a4c0		// 0x0002a4d2
L2a4d4:
		lea ecx, [ebp - 0x60]		// 0x0002a4d4
		call nxHullCallRadixCtor		// 0x0002a4d7
		push 1		// 0x0002a4dc
		push ebx		// 0x0002a4de
		push edi		// 0x0002a4df
		lea ecx, [ebp - 0x60]		// 0x0002a4e0
		call nxHullCallRadixSort		// 0x0002a4e3
		mov edx, dword ptr [eax + 4]		// 0x0002a4e8
		lea edi, [ebx*4]		// 0x0002a4eb
		push edi		// 0x0002a4f2
		mov dword ptr [ebp + 8], edx		// 0x0002a4f3
		call nxHullCallNew		// 0x0002a4f6
		mov edx, eax		// 0x0002a4fb
		mov ecx, edi		// 0x0002a4fd
		mov dword ptr [esi + 0x28], edx		// 0x0002a4ff
		mov edi, edx		// 0x0002a502
		mov edx, ecx		// 0x0002a504
		shr ecx, 2		// 0x0002a506
		or eax, 0xffffffff		// 0x0002a509
		rep stosd		// 0x0002a50c
		mov ecx, edx		// 0x0002a50e
		and ecx, 3		// 0x0002a510
		rep stosb		// 0x0002a513
		xor edi, edi		// 0x0002a515
		add esp, 4		// 0x0002a517
		lea ecx, [ebp - 0x48]		// 0x0002a51a
		mov dword ptr [esi + 0x24], edi		// 0x0002a51d
		mov dword ptr [ebp - 0x1c], edi		// 0x0002a520
		call nxHullCallContainerCtor		// 0x0002a523
		lea ecx, [ebp - 0x38]		// 0x0002a528
		call nxHullCallContainerCtor		// 0x0002a52b
		cmp ebx, edi		// 0x0002a530
		mov eax, dword ptr [ebp + 0x18]		// 0x0002a532
		mov ecx, dword ptr [ebp + 0xc]		// 0x0002a535
		mov edx, dword ptr [ebp + 0x14]		// 0x0002a538
		mov dword ptr [esi + 8], eax		// 0x0002a53b
		mov eax, dword ptr [ebp - 0xc]		// 0x0002a53e
		mov dword ptr [esi], ecx		// 0x0002a541
		mov dword ptr [esi + 4], edx		// 0x0002a543
		mov dword ptr [esi + 0x14], eax		// 0x0002a546
		mov eax, dword ptr [esi + 0x28]		// 0x0002a549
		lea ecx, [ebp - 0x48]		// 0x0002a54c
		lea edx, [ebp - 0x38]		// 0x0002a54f
		mov dword ptr [esi + 0xc], ecx		// 0x0002a552
		mov dword ptr [esi + 0x10], edx		// 0x0002a555
		mov dword ptr [esi + 0x18], eax		// 0x0002a558
		je L2a5e2		// 0x0002a55b
		mov ecx, dword ptr [ebp + 8]		// 0x0002a561
		lea edx, [ecx + ebx*4 - 4]		// 0x0002a564
		mov dword ptr [ebp + 0x18], edx		// 0x0002a568
		jmp L2a570		// 0x0002a56b
		lea ecx, [ecx]		// 0x0002a56d
L2a570:
		mov eax, dword ptr [ebp + 0x18]		// 0x0002a570
		mov edx, dword ptr [eax]		// 0x0002a573
		mov eax, dword ptr [esi + 0x28]		// 0x0002a575
		xor ecx, ecx		// 0x0002a578
		cmp dword ptr [eax + edx*4], -1		// 0x0002a57a
		je L2a594		// 0x0002a57e
		mov edx, eax		// 0x0002a580
		mov eax, dword ptr [ebp + 0x18]		// 0x0002a582
L2a585:
		mov edi, dword ptr [eax - 4]		// 0x0002a585
		sub eax, 4		// 0x0002a588
		inc ecx		// 0x0002a58b
		cmp dword ptr [edx + edi*4], -1		// 0x0002a58c
		jne L2a585		// 0x0002a590
		xor edi, edi		// 0x0002a592
L2a594:
		cmp dword ptr [ebp - 0x44], edi		// 0x0002a594
		je L2a59c		// 0x0002a597
		mov dword ptr [ebp - 0x44], edi		// 0x0002a599
L2a59c:
		cmp dword ptr [ebp - 0x34], edi		// 0x0002a59c
		je L2a5a4		// 0x0002a59f
		mov dword ptr [ebp - 0x34], edi		// 0x0002a5a1
L2a5a4:
		mov edx, dword ptr [esi + 0x24]		// 0x0002a5a4
		mov eax, ebx		// 0x0002a5a7
		sub eax, ecx		// 0x0002a5a9
		mov ecx, dword ptr [ebp + 8]		// 0x0002a5ab
		mov dword ptr [esi + 0x1c], edx		// 0x0002a5ae
		mov eax, dword ptr [ecx + eax*4 - 4]		// 0x0002a5b1
		cmp eax, ebx		// 0x0002a5b5
		jae L2a5cc		// 0x0002a5b7
		push -1		// 0x0002a5b9
		push eax		// 0x0002a5bb
		mov ecx, esi		// 0x0002a5bc
		mov dword ptr [esi + 0x20], edi		// 0x0002a5be
		mov dword ptr [nxHullStamp], edi		// 0x0002a5c1
		call nxHullFloodStart		// 0x0002a5c7
L2a5cc:
		mov eax, dword ptr [ebp - 0x1c]		// 0x0002a5cc
		mov ecx, dword ptr [esi + 0x20]		// 0x0002a5cf
		mov edx, dword ptr [esi + 0x24]		// 0x0002a5d2
		add eax, ecx		// 0x0002a5d5
		inc edx		// 0x0002a5d7
		cmp eax, ebx		// 0x0002a5d8
		mov dword ptr [ebp - 0x1c], eax		// 0x0002a5da
		mov dword ptr [esi + 0x24], edx		// 0x0002a5dd
		jne L2a570		// 0x0002a5e0
L2a5e2:
		lea ecx, [ebp - 0x38]		// 0x0002a5e2
		call nxHullCallContainerDtor		// 0x0002a5e5
		lea ecx, [ebp - 0x48]		// 0x0002a5ea
		call nxHullCallContainerDtor		// 0x0002a5ed
		lea ecx, [ebp - 0x60]		// 0x0002a5f2
		call nxHullCallRadixDtor		// 0x0002a5f5
		mov al, 1		// 0x0002a5fa
		lea esp, [ebp - 0x6c]		// 0x0002a5fc
		pop edi		// 0x0002a5ff
		pop esi		// 0x0002a600
		pop ebx		// 0x0002a601
		mov esp, ebp		// 0x0002a602
		pop ebp		// 0x0002a604
		ret 0x14		// 0x0002a605
		}
	}
