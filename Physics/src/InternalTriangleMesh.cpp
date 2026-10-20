/*----------------------------------------------------------------------------*\
|
|                              NovodeX Technology
|
|                              www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "TriangleMesh.h"
#include "PhysicsSDK.h"
#include "FoundationSDK.h"
#include "IceMeshBuilder2.h"
#include "EdgeList.h"

#include <new>
#include <string.h>

static_assert(sizeof(InternalTriangleMesh) == 0x38,
	"InternalTriangleMesh is 0x38 bytes in the measured TriangleMesh layout");

// phys_fn_002065 (0x00051ec0). The first nine words are cleared here; the
// embedded OPCODE MeshInterface constructor at +0x24 clears its first four.
void nxInternalMeshInit(InternalTriangleMesh* mesh)
	{
	memset(mesh, 0, 9 * sizeof(NxU32));
	memset(reinterpret_cast<NxU8*>(mesh) + 0x24, 0, 4 * sizeof(NxU32));
	}

// Listing transcriptions of phys_fn_002067 (0x00051ef0, the release of an InternalTriangleMesh's arrays),
// phys_fn_002085 (0x000523a0, the destructor body: the release, then the embedded MeshInterface's no-op) and
// phys_fn_002087, 002089, 002091 and 002093 (0x000523c0, 2,460 bytes: the inventory splits one function at its jump
// targets): the topology rebuild through MeshBuilder2, the new arrays from the Foundation allocator, and the split of
// vertices shared by edges of more than two faces. The allocator is the Foundation's own exported global (the
// candidate's import slot, loaded as in the listing). The callers keep the cdecl entries after the rows. Compare
// script: docs/reconstruction/novodex-physics/evidence/phase4-mesh-topology-listing-compare.py.
extern "C" void nxMTCallNoop();
extern "C" void nxMTCallBuilderCtor();
extern "C" void nxMTCallBuilderInit();
extern "C" void nxMTCallBuilderAddFace();
extern "C" void nxMTCallBuilderBuild();
extern "C" void nxMTCallBuilderDtor();
extern "C" void nxMTCallEdgeListCtor();
extern "C" void nxMTCallEdgeListInit();
extern "C" void nxMTCallEdgeListDtor();
extern "C" void nxMTCallContainerCtor();
extern "C" void nxMTCallContainerDtor();
extern "C" void nxMTCallAddPoint();
extern "C" void nxMTCallReplaceVertex();
extern "C" void* nxMTAllocSlot;
#pragma comment(linker, "/alternatename:_nxMTCallNoop=?nxSupportMapNoop@@YIXPAUIceSupportMap@@@Z")
#pragma comment(linker, "/alternatename:_nxMTCallBuilderCtor=??0MeshBuilder2@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxMTCallBuilderInit=?Init@MeshBuilder2@@QAE_NABUMBCREATE@@@Z")
#pragma comment(linker, "/alternatename:_nxMTCallBuilderAddFace=?AddFace@MeshBuilder2@@QAE_NABUMBFACEINFO@@@Z")
#pragma comment(linker, "/alternatename:_nxMTCallBuilderBuild=?Build@MeshBuilder2@@QAE_NAAUMBRESULT@@@Z")
#pragma comment(linker, "/alternatename:_nxMTCallBuilderDtor=??1MeshBuilder2@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxMTCallEdgeListCtor=??0EdgeList@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxMTCallEdgeListInit=?Init@EdgeList@@QAE_NABUEDGELISTCREATE@@@Z")
#pragma comment(linker, "/alternatename:_nxMTCallEdgeListDtor=??1EdgeList@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxMTCallContainerCtor=??0Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxMTCallContainerDtor=??1Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxMTCallAddPoint=?nxIceContainerAddPoint@@YIAAVContainer@IceCore@@PAV12@IPBI@Z")
#pragma comment(linker, "/alternatename:_nxMTCallReplaceVertex=?ReplaceVertex@IndexedTriangle@IceMaths@@QAE_NII@Z")
#pragma comment(linker, "/alternatename:_nxMTAllocSlot=__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A")

// phys_fn_002067 (0x00051ef0, 169 B), 68 instructions
__declspec(naked) void __fastcall nxMTReleaseRow(void* mesh)
	{
	__asm
		{
		push esi		// 0x00051ef0
		mov esi, ecx		// 0x00051ef1
		mov ecx, dword ptr [esi + 0x20]		// 0x00051ef3
		push edi		// 0x00051ef6
		xor edi, edi		// 0x00051ef7
		cmp ecx, edi		// 0x00051ef9
		je L51f06		// 0x00051efb
		mov eax, dword ptr [ecx]		// 0x00051efd
		push 1		// 0x00051eff
		call dword ptr [eax]		// 0x00051f01
		mov dword ptr [esi + 0x20], edi		// 0x00051f03
L51f06:
		mov eax, dword ptr [esi + 0x1c]		// 0x00051f06
		cmp eax, edi		// 0x00051f09
		je L51f1e		// 0x00051f0b
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00051f0d
		mov ecx, dword ptr [ecx]		// 0x00051f13
		mov edx, dword ptr [ecx]		// 0x00051f15
		push eax		// 0x00051f17
		call dword ptr [edx + 0x14]		// 0x00051f18
		mov dword ptr [esi + 0x1c], edi		// 0x00051f1b
L51f1e:
		mov eax, dword ptr [esi + 0x18]		// 0x00051f1e
		cmp eax, edi		// 0x00051f21
		je L51f36		// 0x00051f23
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00051f25
		mov ecx, dword ptr [ecx]		// 0x00051f2b
		mov edx, dword ptr [ecx]		// 0x00051f2d
		push eax		// 0x00051f2f
		call dword ptr [edx + 0x14]		// 0x00051f30
		mov dword ptr [esi + 0x18], edi		// 0x00051f33
L51f36:
		mov eax, dword ptr [esi + 0x14]		// 0x00051f36
		cmp eax, edi		// 0x00051f39
		je L51f4e		// 0x00051f3b
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00051f3d
		mov ecx, dword ptr [ecx]		// 0x00051f43
		mov edx, dword ptr [ecx]		// 0x00051f45
		push eax		// 0x00051f47
		call dword ptr [edx + 0x14]		// 0x00051f48
		mov dword ptr [esi + 0x14], edi		// 0x00051f4b
L51f4e:
		mov eax, dword ptr [esi + 0x10]		// 0x00051f4e
		cmp eax, edi		// 0x00051f51
		je L51f66		// 0x00051f53
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00051f55
		mov ecx, dword ptr [ecx]		// 0x00051f5b
		mov edx, dword ptr [ecx]		// 0x00051f5d
		push eax		// 0x00051f5f
		call dword ptr [edx + 0x14]		// 0x00051f60
		mov dword ptr [esi + 0x10], edi		// 0x00051f63
L51f66:
		mov eax, dword ptr [esi + 0xc]		// 0x00051f66
		cmp eax, edi		// 0x00051f69
		je L51f7e		// 0x00051f6b
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00051f6d
		mov ecx, dword ptr [ecx]		// 0x00051f73
		mov edx, dword ptr [ecx]		// 0x00051f75
		push eax		// 0x00051f77
		call dword ptr [edx + 0x14]		// 0x00051f78
		mov dword ptr [esi + 0xc], edi		// 0x00051f7b
L51f7e:
		mov eax, dword ptr [esi + 8]		// 0x00051f7e
		cmp eax, edi		// 0x00051f81
		je L51f96		// 0x00051f83
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00051f85
		mov ecx, dword ptr [ecx]		// 0x00051f8b
		mov edx, dword ptr [ecx]		// 0x00051f8d
		push eax		// 0x00051f8f
		call dword ptr [edx + 0x14]		// 0x00051f90
		mov dword ptr [esi + 8], edi		// 0x00051f93
L51f96:
		pop edi		// 0x00051f96
		pop esi		// 0x00051f97
		ret		// 0x00051f98
		}
	}

// phys_fn_002085 (0x000523a0, 17 B), 6 instructions
__declspec(naked) void __fastcall nxInternalMeshDestruct(void* mesh)
	{
	__asm
		{
		push esi		// 0x000523a0
		mov esi, ecx		// 0x000523a1
		call nxMTReleaseRow		// 0x000523a3
		lea ecx, [esi + 0x24]		// 0x000523a8
		pop esi		// 0x000523ab
		jmp nxMTCallNoop		// 0x000523ac
		}
	}

// phys_fn_002087+002089+002091+002093 (0x000523c0, 1692 B), 485 instructions
__declspec(naked) bool __fastcall nxMTBuildTopologyRow(void* mesh)
	{
	__asm
		{
		sub esp, 0x244		// 0x000523c0
		xor eax, eax		// 0x000523c6
		mov dword ptr [esp + 0x34], eax		// 0x000523c8
		mov dword ptr [esp + 0x38], eax		// 0x000523cc
		mov dword ptr [esp + 0x3c], eax		// 0x000523d0
		mov dword ptr [esp + 0x40], eax		// 0x000523d4
		mov dword ptr [esp + 0x44], eax		// 0x000523d8
		mov dword ptr [esp + 0x48], eax		// 0x000523dc
		mov dword ptr [esp + 0x4c], eax		// 0x000523e0
		mov dword ptr [esp + 0x50], eax		// 0x000523e4
		push ebx		// 0x000523e8
		mov dword ptr [esp + 0x58], eax		// 0x000523e9
		xor ebx, ebx		// 0x000523ed
		push ebp		// 0x000523ef
		mov ebp, ecx		// 0x000523f0
		mov ecx, dword ptr [ebp]		// 0x000523f2
		mov edx, dword ptr [ebp + 4]		// 0x000523f5
		mov dword ptr [esp + 0x60], eax		// 0x000523f8
		mov eax, dword ptr [ebp + 8]		// 0x000523fc
		mov dword ptr [esp + 0x3c], ecx		// 0x000523ff
		lea ecx, [esp + 0x128]		// 0x00052403
		mov dword ptr [esp + 0x40], edx		// 0x0005240a
		mov dword ptr [esp + 0x44], ebx		// 0x0005240e
		mov dword ptr [esp + 0x48], ebx		// 0x00052412
		mov dword ptr [esp + 0x4c], eax		// 0x00052416
		mov dword ptr [esp + 0x50], ebx		// 0x0005241a
		mov dword ptr [esp + 0x54], ebx		// 0x0005241e
		mov byte ptr [esp + 0x58], 1		// 0x00052422
		mov byte ptr [esp + 0x59], bl		// 0x00052427
		mov byte ptr [esp + 0x5a], bl		// 0x0005242b
		mov byte ptr [esp + 0x5b], bl		// 0x0005242f
		mov byte ptr [esp + 0x5c], bl		// 0x00052433
		mov byte ptr [esp + 0x5d], 1		// 0x00052437
		mov byte ptr [esp + 0x5e], 1		// 0x0005243c
		mov byte ptr [esp + 0x5f], 1		// 0x00052441
		mov byte ptr [esp + 0x60], 1		// 0x00052446
		mov byte ptr [esp + 0x61], bl		// 0x0005244b
		mov byte ptr [esp + 0x62], 1		// 0x0005244f
		mov byte ptr [esp + 0x63], bl		// 0x00052454
		call nxMTCallBuilderCtor		// 0x00052458
		lea ecx, [esp + 0x3c]		// 0x0005245d
		push ecx		// 0x00052461
		lea ecx, [esp + 0x12c]		// 0x00052462
		call nxMTCallBuilderInit		// 0x00052469
		test al, al		// 0x0005246e
		jne L52489		// 0x00052470
		lea ecx, [esp + 0x128]		// 0x00052472
		call nxMTCallBuilderDtor		// 0x00052479
		pop ebp		// 0x0005247e
		xor al, al		// 0x0005247f
		pop ebx		// 0x00052481
		add esp, 0x244		// 0x00052482
		ret		// 0x00052488
L52489:
		mov eax, dword ptr [ebp + 4]		// 0x00052489
		push esi		// 0x0005248c
		xor esi, esi		// 0x0005248d
		cmp eax, ebx		// 0x0005248f
		push edi		// 0x00052491
		jbe L524fe		// 0x00052492
		xor edi, edi		// 0x00052494
		jmp L524a0		// 0x00052496
		lea esp, [esp]		// 0x00052498
		nop		// 0x0005249f
L524a0:
		mov edx, dword ptr [ebp + 0xc]		// 0x000524a0
		lea eax, [esp + 0x114]		// 0x000524a3
		add edx, edi		// 0x000524aa
		push eax		// 0x000524ac
		lea ecx, [esp + 0x134]		// 0x000524ad
		mov dword ptr [esp + 0x118], esi		// 0x000524b4
		mov dword ptr [esp + 0x11c], 0xffffffff		// 0x000524bb
		mov dword ptr [esp + 0x120], 1		// 0x000524c6
		mov dword ptr [esp + 0x124], edx		// 0x000524d1
		mov dword ptr [esp + 0x128], ebx		// 0x000524d8
		mov dword ptr [esp + 0x12c], ebx		// 0x000524df
		mov byte ptr [esp + 0x130], 0		// 0x000524e6
		call nxMTCallBuilderAddFace		// 0x000524ee
		mov eax, dword ptr [ebp + 4]		// 0x000524f3
		inc esi		// 0x000524f6
		add edi, 0xc		// 0x000524f7
		cmp esi, eax		// 0x000524fa
		jb L524a0		// 0x000524fc
L524fe:
		xor ecx, ecx		// 0x000524fe
		mov dword ptr [esp + 0xb4], ecx		// 0x00052500
		mov dword ptr [esp + 0xb8], ecx		// 0x00052507
		mov dword ptr [esp + 0xbc], ecx		// 0x0005250e
		mov dword ptr [esp + 0xc0], ecx		// 0x00052515
		mov dword ptr [esp + 0xc4], ecx		// 0x0005251c
		mov dword ptr [esp + 0xc8], ecx		// 0x00052523
		mov dword ptr [esp + 0xcc], ecx		// 0x0005252a
		mov dword ptr [esp + 0xd0], ecx		// 0x00052531
		xor eax, eax		// 0x00052538
		mov ecx, 0xe		// 0x0005253a
		lea edi, [esp + 0xd4]		// 0x0005253f
		rep stosd		// 0x00052546
		lea edx, [esp + 0xb4]		// 0x00052548
		push edx		// 0x0005254f
		lea ecx, [esp + 0x134]		// 0x00052550
		mov dword ptr [esp + 0x110], ebx		// 0x00052557
		mov dword ptr [esp + 0x114], ebx		// 0x0005255e
		call nxMTCallBuilderBuild		// 0x00052565
		test al, al		// 0x0005256a
		je L527c3		// 0x0005256c
		mov eax, dword ptr [ebp + 0xc]		// 0x00052572
		cmp eax, ebx		// 0x00052575
		je L5258a		// 0x00052577
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00052579
		mov ecx, dword ptr [ecx]		// 0x0005257f
		mov edx, dword ptr [ecx]		// 0x00052581
		push eax		// 0x00052583
		call dword ptr [edx + 0x14]		// 0x00052584
		mov dword ptr [ebp + 0xc], ebx		// 0x00052587
L5258a:
		mov eax, dword ptr [ebp + 8]		// 0x0005258a
		cmp eax, ebx		// 0x0005258d
		je L525a2		// 0x0005258f
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00052591
		mov ecx, dword ptr [ecx]		// 0x00052597
		mov edx, dword ptr [ecx]		// 0x00052599
		push eax		// 0x0005259b
		call dword ptr [edx + 0x14]		// 0x0005259c
		mov dword ptr [ebp + 8], ebx		// 0x0005259f
L525a2:
		mov eax, dword ptr [ebp + 0x14]		// 0x000525a2
		cmp eax, ebx		// 0x000525a5
		je L525ba		// 0x000525a7
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x000525a9
		mov ecx, dword ptr [ecx]		// 0x000525af
		mov edx, dword ptr [ecx]		// 0x000525b1
		push eax		// 0x000525b3
		call dword ptr [edx + 0x14]		// 0x000525b4
		mov dword ptr [ebp + 0x14], ebx		// 0x000525b7
L525ba:
		mov ecx, dword ptr [esp + 0xd0]		// 0x000525ba
		cmp ecx, ebx		// 0x000525c1
		mov edx, dword ptr [esp + 0xb4]		// 0x000525c3
		je L5260b		// 0x000525ca
		xor eax, eax		// 0x000525cc
		cmp edx, ebx		// 0x000525ce
		jbe L5260b		// 0x000525d0
L525d2:
		cmp dword ptr [ecx + eax*4], eax		// 0x000525d2
		jne L525de		// 0x000525d5
		inc eax		// 0x000525d7
		cmp eax, edx		// 0x000525d8
		jb L525d2		// 0x000525da
		jmp L5260b		// 0x000525dc
L525de:
		mov eax, dword ptr [nxMTAllocSlot]		// 0x000525de
		mov ecx, dword ptr [eax]		// 0x000525e3
		mov eax, dword ptr [ecx]		// 0x000525e5
		push ebx		// 0x000525e7
		shl edx, 2		// 0x000525e8
		push edx		// 0x000525eb
		call dword ptr [eax + 8]		// 0x000525ec
		mov ecx, dword ptr [esp + 0xb4]		// 0x000525ef
		mov esi, dword ptr [esp + 0xd0]		// 0x000525f6
		mov edi, eax		// 0x000525fd
		mov dword ptr [ebp + 0x14], eax		// 0x000525ff
		rep movsd		// 0x00052602
		mov edx, dword ptr [esp + 0xb4]		// 0x00052604
L5260b:
		cmp dword ptr [ebp + 0x10], ebx		// 0x0005260b
		je L5266c		// 0x0005260e
		cmp dword ptr [ebp + 0x14], ebx		// 0x00052610
		je L5266c		// 0x00052613
		mov eax, dword ptr [nxMTAllocSlot]		// 0x00052615
		mov ecx, dword ptr [eax]		// 0x0005261a
		mov eax, dword ptr [ecx]		// 0x0005261c
		push ebx		// 0x0005261e
		add edx, edx		// 0x0005261f
		push edx		// 0x00052621
		call dword ptr [eax + 8]		// 0x00052622
		mov ecx, dword ptr [esp + 0xb4]		// 0x00052625
		mov esi, eax		// 0x0005262c
		xor eax, eax		// 0x0005262e
		cmp ecx, ebx		// 0x00052630
		jbe L52651		// 0x00052632
L52634:
		mov ecx, dword ptr [ebp + 0x14]		// 0x00052634
		mov edx, dword ptr [ecx + eax*4]		// 0x00052637
		mov ecx, dword ptr [ebp + 0x10]		// 0x0005263a
		mov dx, word ptr [ecx + edx*2]		// 0x0005263d
		mov word ptr [esi + eax*2], dx		// 0x00052641
		mov ecx, dword ptr [esp + 0xb4]		// 0x00052645
		inc eax		// 0x0005264c
		cmp eax, ecx		// 0x0005264d
		jb L52634		// 0x0005264f
L52651:
		mov eax, dword ptr [ebp + 0x10]		// 0x00052651
		cmp eax, ebx		// 0x00052654
		je L52669		// 0x00052656
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00052658
		mov ecx, dword ptr [ecx]		// 0x0005265e
		mov edx, dword ptr [ecx]		// 0x00052660
		push eax		// 0x00052662
		call dword ptr [edx + 0x14]		// 0x00052663
		mov dword ptr [ebp + 0x10], ebx		// 0x00052666
L52669:
		mov dword ptr [ebp + 0x10], esi		// 0x00052669
L5266c:
		mov eax, dword ptr [esp + 0xe0]		// 0x0005266c
		mov dword ptr [ebp], eax		// 0x00052673
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00052676
		mov ecx, dword ptr [ecx]		// 0x0005267c
		mov edx, dword ptr [ecx]		// 0x0005267e
		lea eax, [eax + eax*2]		// 0x00052680
		push ebx		// 0x00052683
		shl eax, 2		// 0x00052684
		push eax		// 0x00052687
		call dword ptr [edx + 8]		// 0x00052688
		mov dword ptr [ebp + 8], eax		// 0x0005268b
		mov eax, dword ptr [esp + 0xb4]		// 0x0005268e
		mov dword ptr [ebp + 4], eax		// 0x00052695
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x00052698
		mov ecx, dword ptr [ecx]		// 0x0005269e
		mov edx, dword ptr [ecx]		// 0x000526a0
		lea eax, [eax + eax*2]		// 0x000526a2
		push ebx		// 0x000526a5
		shl eax, 2		// 0x000526a6
		push eax		// 0x000526a9
		call dword ptr [edx + 8]		// 0x000526aa
		mov dword ptr [ebp + 0xc], eax		// 0x000526ad
		mov eax, dword ptr [ebp]		// 0x000526b0
		xor edx, edx		// 0x000526b3
		cmp eax, ebx		// 0x000526b5
		jbe L5270f		// 0x000526b7
		xor ecx, ecx		// 0x000526b9
		jmp L526c0		// 0x000526bb
		lea ecx, [ecx]		// 0x000526bd
L526c0:
		mov eax, dword ptr [esp + 0xe4]		// 0x000526c0
		mov eax, dword ptr [eax + edx*4]		// 0x000526c7
		mov edi, dword ptr [esp + 0xf0]		// 0x000526ca
		mov esi, dword ptr [ebp + 8]		// 0x000526d1
		lea eax, [eax + eax*2]		// 0x000526d4
		shl eax, 2		// 0x000526d7
		mov edi, dword ptr [eax + edi]		// 0x000526da
		mov dword ptr [ecx + esi], edi		// 0x000526dd
		mov edi, dword ptr [esp + 0xf0]		// 0x000526e0
		mov edi, dword ptr [eax + edi + 4]		// 0x000526e7
		mov esi, dword ptr [ebp + 8]		// 0x000526eb
		mov dword ptr [esi + ecx + 4], edi		// 0x000526ee
		mov edi, dword ptr [esp + 0xf0]		// 0x000526f2
		mov eax, dword ptr [eax + edi + 8]		// 0x000526f9
		mov esi, dword ptr [ebp + 8]		// 0x000526fd
		mov dword ptr [esi + ecx + 8], eax		// 0x00052700
		mov eax, dword ptr [ebp]		// 0x00052704
		inc edx		// 0x00052707
		add ecx, 0xc		// 0x00052708
		cmp edx, eax		// 0x0005270b
		jb L526c0		// 0x0005270d
L5270f:
		mov eax, dword ptr [ebp + 4]		// 0x0005270f
		xor ecx, ecx		// 0x00052712
		cmp eax, ebx		// 0x00052714
		jbe L5275f		// 0x00052716
		xor eax, eax		// 0x00052718
		lea ebx, [ebx]		// 0x0005271a
L52720:
		mov esi, dword ptr [esp + 0xc0]		// 0x00052720
		mov esi, dword ptr [eax + esi]		// 0x00052727
		mov edx, dword ptr [ebp + 0xc]		// 0x0005272a
		mov dword ptr [eax + edx], esi		// 0x0005272d
		mov esi, dword ptr [esp + 0xc0]		// 0x00052730
		mov esi, dword ptr [eax + esi + 4]		// 0x00052737
		mov edx, dword ptr [ebp + 0xc]		// 0x0005273b
		mov dword ptr [edx + eax + 4], esi		// 0x0005273e
		mov esi, dword ptr [esp + 0xc0]		// 0x00052742
		mov edx, dword ptr [ebp + 0xc]		// 0x00052749
		mov esi, dword ptr [eax + esi + 8]		// 0x0005274c
		mov dword ptr [edx + eax + 8], esi		// 0x00052750
		mov edx, dword ptr [ebp + 4]		// 0x00052754
		inc ecx		// 0x00052757
		add eax, 0xc		// 0x00052758
		cmp ecx, edx		// 0x0005275b
		jb L52720		// 0x0005275d
L5275f:
		mov ecx, dword ptr [ebp + 0xc]		// 0x0005275f
		mov eax, dword ptr [ebp + 4]		// 0x00052762
		mov dword ptr [esp + 0x90], ecx		// 0x00052765
		lea ecx, [esp + 0x74]		// 0x0005276c
		mov dword ptr [esp + 0x94], ebx		// 0x00052770
		mov byte ptr [esp + 0x98], 0		// 0x00052777
		mov dword ptr [esp + 0x9c], ebx		// 0x0005277f
		mov dword ptr [esp + 0xa0], 0x3a83126f		// 0x00052786
		mov dword ptr [esp + 0x8c], eax		// 0x00052791
		mov byte ptr [esp + 0x99], 1		// 0x00052798
		call nxMTCallEdgeListCtor		// 0x000527a0
		lea edx, [esp + 0x8c]		// 0x000527a5
		push edx		// 0x000527ac
		lea ecx, [esp + 0x78]		// 0x000527ad
		call nxMTCallEdgeListInit		// 0x000527b1
		test al, al		// 0x000527b6
		jne L527dc		// 0x000527b8
		lea ecx, [esp + 0x74]		// 0x000527ba
		call nxMTCallEdgeListDtor		// 0x000527be
L527c3:
		lea ecx, [esp + 0x130]		// 0x000527c3
		call nxMTCallBuilderDtor		// 0x000527ca
		pop edi		// 0x000527cf
		pop esi		// 0x000527d0
		pop ebp		// 0x000527d1
		xor al, al		// 0x000527d2
		pop ebx		// 0x000527d4
		add esp, 0x244		// 0x000527d5
		ret		// 0x000527db
L527dc:
		lea ecx, [esp + 0xa4]		// 0x000527dc
		call nxMTCallContainerCtor		// 0x000527e3
		mov eax, dword ptr [ebp]		// 0x000527e8
		xor esi, esi		// 0x000527eb
		cmp eax, ebx		// 0x000527ed
		jbe L52810		// 0x000527ef
		xor edi, edi		// 0x000527f1
L527f3:
		mov eax, dword ptr [ebp + 8]		// 0x000527f3
		add eax, edi		// 0x000527f6
		push eax		// 0x000527f8
		lea ecx, [esp + 0xa8]		// 0x000527f9
		call nxMTCallAddPoint		// 0x00052800
		mov eax, dword ptr [ebp]		// 0x00052805
		inc esi		// 0x00052808
		add edi, 0xc		// 0x00052809
		cmp esi, eax		// 0x0005280c
		jb L527f3		// 0x0005280e
L52810:
		mov eax, dword ptr [esp + 0x74]		// 0x00052810
		mov esi, dword ptr [ebp]		// 0x00052814
		xor edx, edx		// 0x00052817
		cmp eax, ebx		// 0x00052819
		mov dword ptr [esp + 0x18], esi		// 0x0005281b
		mov dword ptr [esp + 0x3c], edx		// 0x0005281f
		jbe L529da		// 0x00052823
		mov eax, dword ptr [esp + 0x78]		// 0x00052829
		lea ecx, [ecx]		// 0x0005282d
L52830:
		mov esi, dword ptr [esp + 0x84]		// 0x00052830
		movzx ecx, word ptr [esi + edx*8 + 2]		// 0x00052837
		cmp ecx, 2		// 0x0005283c
		jbe L529c5		// 0x0005283f
		mov esi, dword ptr [esi + edx*8 + 4]		// 0x00052845
		mov edi, dword ptr [eax + edx*8]		// 0x00052849
		mov dword ptr [esp + 0x14], esi		// 0x0005284c
		mov esi, dword ptr [eax + edx*8 + 4]		// 0x00052850
		add ecx, -2		// 0x00052854
		mov dword ptr [esp + 0x6c], esi		// 0x00052857
		mov dword ptr [esp + 0x10], ebx		// 0x0005285b
		mov esi, 1		// 0x0005285f
		xor ebx, ebx		// 0x00052864
		test ecx, ecx		// 0x00052866
		mov dword ptr [esp + 0x70], ecx		// 0x00052868
		mov dword ptr [esp + 0x40], esi		// 0x0005286c
		jbe L529c3		// 0x00052870
		mov ecx, dword ptr [esp + 0x14]		// 0x00052876
		lea edx, [ecx*4]		// 0x0005287a
		mov dword ptr [esp + 0x14], edx		// 0x00052881
		jmp L52890		// 0x00052885
L52887:
		mov esi, dword ptr [esp + 0x40]		// 0x00052887
		jmp L52890		// 0x0005288b
		lea ecx, [ecx]		// 0x0005288d
L52890:
		test bl, 1		// 0x00052890
		jne L5296e		// 0x00052893
		mov eax, dword ptr [esp + 0x10]		// 0x00052899
		inc eax		// 0x0005289d
		cmp eax, 8		// 0x0005289e
		mov dword ptr [esp + 0x10], eax		// 0x000528a1
		jne L528b4		// 0x000528a5
		inc esi		// 0x000528a7
		mov dword ptr [esp + 0x10], 1		// 0x000528a8
		mov dword ptr [esp + 0x40], esi		// 0x000528b0
L528b4:
		mov ecx, dword ptr [ebp + 8]		// 0x000528b4
		lea eax, [edi + edi*2]		// 0x000528b7
		mov edx, dword ptr [ecx + eax*4]		// 0x000528ba
		lea eax, [ecx + eax*4]		// 0x000528bd
		mov dword ptr [esp + 0x24], edx		// 0x000528c0
		mov edx, dword ptr [eax + 4]		// 0x000528c4
		mov eax, dword ptr [eax + 8]		// 0x000528c7
		mov dword ptr [esp + 0x28], edx		// 0x000528ca
		mov edx, dword ptr [esp + 0x10]		// 0x000528ce
		and edx, 1		// 0x000528d2
		mov dword ptr [esp + 0x2c], eax		// 0x000528d5
		je L528df		// 0x000528d9
		xor dword ptr [esp + 0x24], esi		// 0x000528db
L528df:
		mov eax, dword ptr [esp + 0x10]		// 0x000528df
		and eax, 2		// 0x000528e3
		mov dword ptr [esp + 0x1c], eax		// 0x000528e6
		je L528f0		// 0x000528ea
		xor dword ptr [esp + 0x28], esi		// 0x000528ec
L528f0:
		mov eax, dword ptr [esp + 0x10]		// 0x000528f0
		and eax, 4		// 0x000528f4
		mov dword ptr [esp + 0x20], eax		// 0x000528f7
		je L52901		// 0x000528fb
		xor dword ptr [esp + 0x2c], esi		// 0x000528fd
L52901:
		test edx, edx		// 0x00052901
		mov eax, dword ptr [esp + 0x6c]		// 0x00052903
		lea eax, [eax + eax*2]		// 0x00052907
		lea eax, [ecx + eax*4]		// 0x0005290a
		mov ecx, dword ptr [eax]		// 0x0005290d
		mov dword ptr [esp + 0x30], ecx		// 0x0005290f
		mov ecx, dword ptr [eax + 4]		// 0x00052913
		mov eax, dword ptr [eax + 8]		// 0x00052916
		mov dword ptr [esp + 0x34], ecx		// 0x00052919
		mov dword ptr [esp + 0x38], eax		// 0x0005291d
		je L52927		// 0x00052921
		xor dword ptr [esp + 0x30], esi		// 0x00052923
L52927:
		mov eax, dword ptr [esp + 0x1c]		// 0x00052927
		test eax, eax		// 0x0005292b
		je L52933		// 0x0005292d
		xor dword ptr [esp + 0x34], esi		// 0x0005292f
L52933:
		mov eax, dword ptr [esp + 0x20]		// 0x00052933
		test eax, eax		// 0x00052937
		je L5293f		// 0x00052939
		xor dword ptr [esp + 0x38], esi		// 0x0005293b
L5293f:
		lea ecx, [esp + 0x30]		// 0x0005293f
		push ecx		// 0x00052943
		lea edx, [esp + 0x28]		// 0x00052944
		push edx		// 0x00052948
		lea ecx, [esp + 0xac]		// 0x00052949
		call nxMTCallAddPoint		// 0x00052950
		mov ecx, eax		// 0x00052955
		call nxMTCallAddPoint		// 0x00052957
		mov eax, dword ptr [esp + 0x18]		// 0x0005295c
		mov dword ptr [esp + 0x20], eax		// 0x00052960
		inc eax		// 0x00052964
		mov dword ptr [esp + 0x1c], eax		// 0x00052965
		inc eax		// 0x00052969
		mov dword ptr [esp + 0x18], eax		// 0x0005296a
L5296e:
		mov eax, dword ptr [esp + 0x88]		// 0x0005296e
		mov ecx, dword ptr [esp + 0x14]		// 0x00052975
		mov eax, dword ptr [eax + ecx]		// 0x00052979
		mov ecx, dword ptr [esp + 0x20]		// 0x0005297c
		lea edx, [eax + eax*2]		// 0x00052980
		mov eax, dword ptr [ebp + 0xc]		// 0x00052983
		push ecx		// 0x00052986
		lea esi, [eax + edx*4]		// 0x00052987
		push edi		// 0x0005298a
		mov ecx, esi		// 0x0005298b
		call nxMTCallReplaceVertex		// 0x0005298d
		mov edx, dword ptr [esp + 0x1c]		// 0x00052992
		mov eax, dword ptr [esp + 0x6c]		// 0x00052996
		push edx		// 0x0005299a
		push eax		// 0x0005299b
		mov ecx, esi		// 0x0005299c
		call nxMTCallReplaceVertex		// 0x0005299e
		mov ecx, dword ptr [esp + 0x14]		// 0x000529a3
		mov eax, dword ptr [esp + 0x70]		// 0x000529a7
		inc ebx		// 0x000529ab
		add ecx, 4		// 0x000529ac
		cmp ebx, eax		// 0x000529af
		mov dword ptr [esp + 0x14], ecx		// 0x000529b1
		jb L52887		// 0x000529b5
		mov eax, dword ptr [esp + 0x78]		// 0x000529bb
		mov edx, dword ptr [esp + 0x3c]		// 0x000529bf
L529c3:
		xor ebx, ebx		// 0x000529c3
L529c5:
		mov ecx, dword ptr [esp + 0x74]		// 0x000529c5
		inc edx		// 0x000529c9
		cmp edx, ecx		// 0x000529ca
		mov dword ptr [esp + 0x3c], edx		// 0x000529cc
		jb L52830		// 0x000529d0
		mov esi, dword ptr [esp + 0x18]		// 0x000529d6
L529da:
		cmp dword ptr [ebp], esi		// 0x000529da
		je L52a2e		// 0x000529dd
		mov eax, dword ptr [ebp + 8]		// 0x000529df
		cmp eax, ebx		// 0x000529e2
		je L529f7		// 0x000529e4
		mov ecx, dword ptr [nxMTAllocSlot]		// 0x000529e6
		mov ecx, dword ptr [ecx]		// 0x000529ec
		mov edx, dword ptr [ecx]		// 0x000529ee
		push eax		// 0x000529f0
		call dword ptr [edx + 0x14]		// 0x000529f1
		mov dword ptr [ebp + 8], ebx		// 0x000529f4
L529f7:
		mov eax, dword ptr [nxMTAllocSlot]		// 0x000529f7
		mov ecx, dword ptr [eax]		// 0x000529fc
		mov edx, dword ptr [ecx]		// 0x000529fe
		lea esi, [esi + esi*2]		// 0x00052a00
		push ebx		// 0x00052a03
		shl esi, 2		// 0x00052a04
		push esi		// 0x00052a07
		call dword ptr [edx + 8]		// 0x00052a08
		mov ecx, esi		// 0x00052a0b
		mov esi, dword ptr [esp + 0xac]		// 0x00052a0d
		mov dword ptr [ebp + 8], eax		// 0x00052a14
		mov edi, eax		// 0x00052a17
		mov eax, ecx		// 0x00052a19
		shr ecx, 2		// 0x00052a1b
		rep movsd		// 0x00052a1e
		mov ecx, eax		// 0x00052a20
		and ecx, 3		// 0x00052a22
		rep movsb		// 0x00052a25
		mov ecx, dword ptr [esp + 0x18]		// 0x00052a27
		mov dword ptr [ebp], ecx		// 0x00052a2b
L52a2e:
		lea ecx, [esp + 0xa4]		// 0x00052a2e
		call nxMTCallContainerDtor		// 0x00052a35
		lea ecx, [esp + 0x74]		// 0x00052a3a
		call nxMTCallEdgeListDtor		// 0x00052a3e
		lea ecx, [esp + 0x130]		// 0x00052a43
		call nxMTCallBuilderDtor		// 0x00052a4a
		pop edi		// 0x00052a4f
		pop esi		// 0x00052a50
		pop ebp		// 0x00052a51
		mov al, 1		// 0x00052a52
		pop ebx		// 0x00052a54
		add esp, 0x244		// 0x00052a55
		ret		// 0x00052a5b
		}
	}

// The cdecl entries the callers use: phys_fn_002067 and phys_fn_002087 (with the rows split from it).
void nxInternalMeshRelease(InternalTriangleMesh* mesh)
	{
	nxMTReleaseRow(mesh);
	}

bool nxInternalMeshBuildTopology(InternalTriangleMesh* mesh)
	{
	return nxMTBuildTopologyRow(mesh);
	}

// phys_fn_002069 (0x00051fa0).
void nxInternalMeshAllocateVertices(InternalTriangleMesh* mesh, NxU32 count)
	{
	mesh->mVertexCount = count;
	mesh->mVertices = nxFoundationSDKAllocator->malloc(count * 0x0c, NX_MEMORY_PERSISTENT);
	}

// phys_fn_002071 (0x00051fd0).
void nxInternalMeshAllocateTriangles(InternalTriangleMesh* mesh, NxU32 count)
	{
	mesh->mTriangleCount = count;
	mesh->mTriangles = nxFoundationSDKAllocator->malloc(count * 0x0c, NX_MEMORY_PERSISTENT);
	}

// phys_fn_002073 (0x00052000). It leaves the destination untouched when the
// triangle count is zero, matching the branch before the allocation call.
void nxInternalMeshAllocateMaterials(InternalTriangleMesh* mesh)
	{
	if(mesh->mTriangleCount)
		mesh->mMaterialIndices = static_cast<NxU16*>(nxFoundationSDKAllocator->malloc(
			mesh->mTriangleCount * sizeof(NxU16), NX_MEMORY_PERSISTENT));
	}

// phys_fn_002075 (0x00052030).
void nxInternalMeshAllocateFaceRemap(InternalTriangleMesh* mesh)
	{
	if(mesh->mTriangleCount)
		mesh->mFaceRemap = static_cast<NxU32*>(nxFoundationSDKAllocator->malloc(
			mesh->mTriangleCount * sizeof(NxU32), NX_MEMORY_PERSISTENT));
	}

// phys_fn_002079 (0x000521b0). Replaces the old record array if present, then
// emits one oriented plane for every 32-bit indexed triangle.
void nxInternalMeshBuildTriangleData(InternalTriangleMesh* mesh)
	{
	if(mesh->mTriangleData)
		{
		nxFoundationSDKAllocator->free(mesh->mTriangleData);
		mesh->mTriangleData = 0;
		}
	mesh->mTriangleData = nxFoundationSDKAllocator->malloc(
			mesh->mTriangleCount << 4, NX_MEMORY_PERSISTENT);

	IceMaths::Plane* planes = static_cast<IceMaths::Plane*>(mesh->mTriangleData);
	const NxU32* triangles = static_cast<const NxU32*>(mesh->mTriangles);
	const IceMaths::Point* vertices = static_cast<const IceMaths::Point*>(mesh->mVertices);
	for(NxU32 i = 0; i < mesh->mTriangleCount; ++i)
		{
		const NxU32* indices = triangles + i * 3;
		planes[i].Set(vertices[indices[0]], vertices[indices[1]], vertices[indices[2]]);
		}
	}

// phys_fn_002083 (0x00052280). The image allocates Model through the host
// SDK allocator, while the arrays it references remain Foundation-owned.
bool nxInternalMeshBuildModel(InternalTriangleMesh* mesh, NxU32 extendAxis,
	NxReal extendValue, const void* deserializeFrom)
	{
	if(mesh->mModel)
		{
		delete mesh->mModel;
		mesh->mModel = 0;
		}

	Opcode::MeshInterface* interface = &mesh->mMeshInterface;
	interface->SetNbVertices(mesh->mVertexCount);
	interface->SetNbTriangles(mesh->mTriangleCount);
	interface->SetPointers(
		static_cast<const IceMaths::IndexedTriangle*>(mesh->mTriangles),
		static_cast<const IceMaths::Point*>(mesh->mVertices));

	Opcode::OPCODECREATE create;
	create.mIMesh = interface;
	create.mDeserializeFrom = deserializeFrom;
	create.mSettings.mLimit = 1;
	create.mSettings.mRules = 0x22;
	create.mNoLeaf = true;
	if(PhysicsSDK::instance)
		create.mQuantized = PhysicsSDK::instance->getParameter(NX_CONTINUOUS_CD) != 1.0f;
	// The image reads phys_data_003482 at RVA 0x00123b3c into mKeepOriginal.
	// The pinned oracle initializes these bytes to zero and the census records no
	// write references, so OPCODECREATE's false default matches the observed value.
	create.mCanRemap = false;
	if(extendAxis != 0xff)
		{
		create.mSettings.mNovodeXExtendAxis = static_cast<NxI32>(extendAxis);
		create.mSettings.mNovodeXExtendValue = extendValue;
		}

	void* memory = nxGetSdkAllocator()->malloc(sizeof(Opcode::Model), NX_MEMORY_PERSISTENT);
	mesh->mModel = memory ? ::new(memory) Opcode::Model() : 0;
	const bool built = mesh->mModel && mesh->mModel->Build(create);
	if(!built)
		NxFoundation::FoundationSDK::error(static_cast<NxErrorCode>(4),
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\InternalTriangleMesh.cpp",
			0x1c1, 0, "Opcode is not OK.");
	return built;
	}
