/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// PrunerSupportRows.cpp: Phase 4's remaining rows at 0x000b3d20 to 0x000f3ee4 (and their Phase 6 and 7 neighbours in the
// same clusters): the penetration map's file read and write and the memory stream's float and block readers
// (004790 to 004801, with 004776/004778 as helpers), the box-pruning sorters' lifetimes and the complete and bipartite
// pruning (004820 to 004828), the pruner process pool's registration (004830 to 004834), the pruners' creation (004852)
// and owner adapters (004870, 004872), the static, bounded and type-3 pruners' remaining slots and destructors (005231,
// 005262, 005263, 005279, 005448 to 005479, 005539), the pruning pool's file save and load (005586 to 005646), the
// tree walks (005566, 005568) and the OBB rows (005380, 005386, 005404), with two more atexit thunks (006332,
// 006334). Naked listing transcriptions; the oracle's statics are the candidate's by name (nxPCStatics, widened to the
// oracle's 32 words, the sorters, the pool singleton, the owner adapters), the oracle's tables are the candidate's
// (aliases), and vendored OPCODE code is reached at its vendored symbols. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase4-pruner-support-rows-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxP4Call0AABBCollider();
extern "C" void nxP4Call0DynamicPruner();
extern "C" void nxP4Call0NxPrunerProcessPool();
extern "C" void nxP4Call0RadixSort();
extern "C" void nxP4Call0SdkContainer();
extern "C" void nxP4Call0SphereCollider();
extern "C" void nxP4Call0StaticPruner();
extern "C" void nxP4Call1AABBCollider();
extern "C" void nxP4Call1AABBTree();
extern "C" void nxP4Call1Pruner();
extern "C" void nxP4Call1RadixSort();
extern "C" void nxP4Call1SphereCollider();
extern "C" void nxP4Call2();
extern "C" void nxP4CallAdd();
extern "C" void nxP4CallAppendBlock();
extern "C" void nxP4CallAtexit();
extern "C" void nxP4CallBuildTree();
extern "C" void nxP4CallCollide();
extern "C" void nxP4CallCollideX();
extern "C" void nxP4CallCreateTree();
extern "C" void nxP4CallEmpty();
extern "C" void nxP4CallFclose();
extern "C" void nxP4CallFlushBits();
extern "C" void nxP4CallFopen();
extern "C" void nxP4CallFree();
extern "C" void nxP4CallFseek();
extern "C" void nxP4CallFtell();
extern "C" void nxP4CallFwrite();
extern "C" void nxP4CallInitBlock();
extern "C" void nxP4CallNxGetSdkAllocator();
extern "C" void nxP4CallNxOpcodeReleasePool();
extern "C" void nxP4CallNxPCBoundedCtor();
extern "C" void nxP4CallNxPCBuildTree();
extern "C" void nxP4CallNxPCOctreeRelease();
extern "C" void nxP4CallNxPCStaticDtorBody();
extern "C" void nxP4CallNxPCStaticPlanes();
extern "C" void nxP4CallNxPOIsInside();
extern "C" void nxP4CallNxPQAppendNode();
extern "C" void nxP4CallNxPQAppendSubtree();
extern "C" void nxP4CallNxPQSphereAABB();
extern "C" void nxP4CallRaycast();
extern "C" void nxP4CallReadDword();
extern "C" void nxP4CallRefit2();
extern "C" void nxP4CallRemove();
extern "C" void nxP4CallReportTouched();
extern "C" void nxP4CallResize();
extern "C" void nxP4CallSort();
extern "C" void nxP4CallStoreDword();
extern "C" void nxP4CallUpdateWorldAABB();
#pragma comment(linker, "/alternatename:_nxP4Call0AABBCollider=??0AABBCollider@Opcode@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call0DynamicPruner=??0DynamicPruner@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call0NxPrunerProcessPool=??0NxPrunerProcessPool@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call0RadixSort=??0RadixSort@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call0SdkContainer=??0SdkContainer@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call0SphereCollider=??0SphereCollider@Opcode@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call0StaticPruner=??0StaticPruner@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call1AABBCollider=??1AABBCollider@Opcode@@UAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call1AABBTree=??1AABBTree@Opcode@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call1Pruner=??1Pruner@@UAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call1RadixSort=??1RadixSort@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call1SphereCollider=??1SphereCollider@Opcode@@UAE@XZ")
#pragma comment(linker, "/alternatename:_nxP4Call2=??2@YAPAXI@Z")
#pragma comment(linker, "/alternatename:_nxP4CallAdd=?add@NxPrunerProcessPool@@QAEIPAUNxPrunerRegistration@@@Z")
#pragma comment(linker, "/alternatename:_nxP4CallAppendBlock=?appendBlock@MemoryStream@@AAE_NPAUMemoryStreamBlock@@I@Z")
#pragma comment(linker, "/alternatename:_nxP4CallAtexit=_atexit")
#pragma comment(linker, "/alternatename:_nxP4CallBuildTree=?BuildTree@StaticPruner@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxP4CallCollide=?Collide@SphereCollider@Opcode@@QAE_NAAUSphereCache@2@ABVSphere@IceMaths@@PBVAABBTree@2@@Z")
#pragma comment(linker, "/alternatename:_nxP4CallCollideX=?Collide@AABBCollider@Opcode@@QAE_NAAUAABBCache@2@ABVCollisionAABB@2@PBVAABBTree@2@@Z")
#pragma comment(linker, "/alternatename:_nxP4CallCreateTree=?CreateTree@BaseModel@Opcode@@IAE_N_N0@Z")
#pragma comment(linker, "/alternatename:_nxP4CallEmpty=?empty@SdkContainer@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxP4CallFclose=_fclose")
#pragma comment(linker, "/alternatename:_nxP4CallFlushBits=?flushBits@MemoryStream@@QAEPAV1@XZ")
#pragma comment(linker, "/alternatename:_nxP4CallFopen=_fopen")
#pragma comment(linker, "/alternatename:_nxP4CallFree=_free")
#pragma comment(linker, "/alternatename:_nxP4CallFseek=_fseek")
#pragma comment(linker, "/alternatename:_nxP4CallFtell=_ftell")
#pragma comment(linker, "/alternatename:_nxP4CallFwrite=_fwrite")
#pragma comment(linker, "/alternatename:_nxP4CallInitBlock=?initBlock@MemoryStream@@AAE_NIPBXII@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxGetSdkAllocator=?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ")
#pragma comment(linker, "/alternatename:_nxP4CallNxOpcodeReleasePool=?nxOpcodeReleasePool@@YAXXZ")
#pragma comment(linker, "/alternatename:_nxP4CallNxPCBoundedCtor=?nxPCBoundedCtor@@YIPAXPAX@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxPCBuildTree=?nxPCBuildTree@@YI_NPAX@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxPCOctreeRelease=?nxPCOctreeRelease@@YIXPAX@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxPCStaticDtorBody=?nxPCStaticDtorBody@@YIXPAX@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxPCStaticPlanes=?nxPCStaticPlanes@@YI_NPAX0IIIII@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxPOIsInside=?nxPOIsInside@@YI_NPBXPAX0@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxPQAppendNode=?nxPQAppendNode@@YIXPAX00@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxPQAppendSubtree=?nxPQAppendSubtree@@YAXIIPAX0@Z")
#pragma comment(linker, "/alternatename:_nxP4CallNxPQSphereAABB=?nxPQSphereAABB@@YA_NPBMM00@Z")
#pragma comment(linker, "/alternatename:_nxP4CallRaycast=?Raycast@StaticPruner@@UAE_NAAVContainer@IceCore@@ABVRay@IceMaths@@M_NI@Z")
#pragma comment(linker, "/alternatename:_nxP4CallReadDword=?readDword@MemoryStream@@QAEIXZ")
#pragma comment(linker, "/alternatename:_nxP4CallRefit2=?Refit2@AABBTree@Opcode@@QAE_NPAVAABBTreeBuilder@2@@Z")
#pragma comment(linker, "/alternatename:_nxP4CallRemove=?remove@NxPrunerProcessPool@@QAEXI@Z")
#pragma comment(linker, "/alternatename:_nxP4CallReportTouched=?ReportTouched@StaticPruner@@QAEXAAVContainer@IceCore@@I@Z")
#pragma comment(linker, "/alternatename:_nxP4CallResize=?resize@SdkContainer@@QAE_NI@Z")
#pragma comment(linker, "/alternatename:_nxP4CallSort=?Sort@RadixSort@IceCore@@QAEAAV12@PBMI@Z")
#pragma comment(linker, "/alternatename:_nxP4CallStoreDword=?storeDword@MemoryStream@@QAEPAV1@I@Z")
#pragma comment(linker, "/alternatename:_nxP4CallUpdateWorldAABB=?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z")
extern "C" void* nxSdkInstanceSlot;
#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")
extern "C" unsigned gNxApiReentry;
extern "C" const char* gNxApiReentryMessage;
extern "C" void* nxKnCompleteSorter;
#pragma comment(linker, "/alternatename:_nxKnCompleteSorter=?gCompleteSorter@@3PAVRadixSort@IceCore@@A")
extern "C" void* nxKnBipartiteSorter0;
#pragma comment(linker, "/alternatename:_nxKnBipartiteSorter0=?gBipartiteSorter0@@3PAVRadixSort@IceCore@@A")
extern "C" void* nxKnBipartiteSorter1;
#pragma comment(linker, "/alternatename:_nxKnBipartiteSorter1=?gBipartiteSorter1@@3PAVRadixSort@IceCore@@A")
extern "C" void* nxKnPrunerPool;
#pragma comment(linker, "/alternatename:_nxKnPrunerPool=?gNxPrunerProcessPool@@3PAVNxPrunerProcessPool@@A")
extern "C" void* nxKnOwnerQuery;
#pragma comment(linker, "/alternatename:_nxKnOwnerQuery=?gPrunableOwnerQuery@@3P6AIPAX@ZA")
extern "C" void* nxKnOwnerNotify;
#pragma comment(linker, "/alternatename:_nxKnOwnerNotify=?gPrunableOwnerNotify@@3P6AXPAXPAVAABB@IceMaths@@@ZA")
extern "C" void* nxKnOwnerWorldAABB;
#pragma comment(linker, "/alternatename:_nxKnOwnerWorldAABB=?gPrunableOwnerWorldAABB@@3P6AXPAXPAVAABB@IceMaths@@@ZA")
extern "C" void* nxKnAdapterNotify;
#pragma comment(linker, "/alternatename:_nxKnAdapterNotify=?gPrunableAdapterNotify@@3P6AXPAVPrunable@@PAVAABB@IceMaths@@@ZA")
extern unsigned nxPCStatics[32];
extern "C" void* nxKnVtStatic;
#pragma comment(linker, "/alternatename:_nxKnVtStatic=??_7StaticPruner@@6B@")
extern "C" void* nxKnVtBounded;
#pragma comment(linker, "/alternatename:_nxKnVtBounded=??_7BoundedDynamicPruner@@6B@")
extern "C" void* nxKnVtPruner;
#pragma comment(linker, "/alternatename:_nxKnVtPruner=??_7Pruner@@6B@")
extern "C" void* nxKnVtDynamic;
#pragma comment(linker, "/alternatename:_nxKnVtDynamic=??_7DynamicPruner@@6B@")
extern "C" void* nxKnVtType3;
#pragma comment(linker, "/alternatename:_nxKnVtType3=??_7Type3StaticPruner@@6B@")
extern "C" void* nxKnVtSphereCollider;
#pragma comment(linker, "/alternatename:_nxKnVtSphereCollider=??_7SphereCollider@Opcode@@6B@")
extern "C" void* nxKnVtAABBsBuilder;
#pragma comment(linker, "/alternatename:_nxKnVtAABBsBuilder=??_7AABBTreeOfAABBsBuilder@Opcode@@6B@")

unsigned __fastcall nxP44776(void* self, void* edxUnused);
unsigned __fastcall nxP44778(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP44790(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP44793(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP44799(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxP44801(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __cdecl nxP44820();
unsigned __fastcall nxP44824(void* self, void* edxUnused);
unsigned __cdecl nxP44828();
unsigned __fastcall nxP44830(void* self, void* edxUnused);
unsigned __fastcall nxP44832(void* self, void* edxUnused);
unsigned __cdecl nxP44834();
unsigned __fastcall nxP44852(void* self, void* edxUnused, unsigned a0);
unsigned __cdecl nxP44870();
unsigned __cdecl nxP44872();
unsigned __fastcall nxP45229(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP45231(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP45262(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP45263(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45279(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxP45380(void* self, void* edxUnused);
unsigned __fastcall nxP45386(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45404(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45448(void* self, void* edxUnused);
unsigned __fastcall nxP45452(void* self, void* edxUnused);
unsigned __fastcall nxP45454(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45456(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4);
unsigned __fastcall nxP45458(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4);
unsigned __fastcall nxP45460(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP45462(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP45466(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4);
unsigned __fastcall nxP45470(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP45471(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP45473(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45477(void* self, void* edxUnused);
unsigned __fastcall nxP45479(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45539(void* self, void* edxUnused, unsigned a0);
unsigned __cdecl nxP45566();
unsigned __fastcall nxP45568(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2);
unsigned __fastcall nxP45586(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45588(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45606(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45608(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45624(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45642(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP45644(void* self, void* edxUnused, unsigned a0);
unsigned __cdecl nxP4ThunkF();
unsigned __cdecl nxP4ThunkG();
// @@PROTOS-END@@

static const unsigned nxP4K101043cc_4 = 0x3f000000;
static const char nxP4ModeRb[] = "rb";
static const char nxP4ModeWb[] = "wb";

// helper: phys_fn_004776 (record unchanged) (0x000b3ae0, 24 B), 9 instructions
__declspec(naked) unsigned __fastcall nxP44776(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [ecx]		// 0x000b3ae0
		mov byte ptr [ecx + 0x18], 0		// 0x000b3ae2
		mov edx, dword ptr [eax]		// 0x000b3ae6
		add edx, dword ptr [eax + 4]		// 0x000b3ae8
		mov dword ptr [ecx + 0x10], edx		// 0x000b3aeb
		add dword ptr [eax + 4], 4		// 0x000b3aee
		mov eax, dword ptr [ecx + 0x10]		// 0x000b3af2
		fld dword ptr [eax]		// 0x000b3af5
		ret		// 0x000b3af7
		}
	}

// helper: phys_fn_004778 (record unchanged) (0x000b3b00, 36 B), 14 instructions
__declspec(naked) unsigned __fastcall nxP44778(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [ecx]		// 0x000b3b00
		push esi		// 0x000b3b02
		mov byte ptr [ecx + 0x18], 0		// 0x000b3b03
		mov esi, dword ptr [eax + 4]		// 0x000b3b07
		mov edx, dword ptr [eax]		// 0x000b3b0a
		add edx, esi		// 0x000b3b0c
		mov dword ptr [ecx + 0x10], edx		// 0x000b3b0e
		mov esi, dword ptr [eax + 4]		// 0x000b3b11
		mov edx, dword ptr [esp + 8]		// 0x000b3b14
		add esi, edx		// 0x000b3b18
		mov dword ptr [eax + 4], esi		// 0x000b3b1a
		mov eax, dword ptr [ecx + 0x10]		// 0x000b3b1d
		pop esi		// 0x000b3b20
		ret 4		// 0x000b3b21
		}
	}

// phys_fn_004790 (0x000b3d20, 144 B), 58 instructions
__declspec(naked) unsigned __fastcall nxP44790(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x000b3d20
		push ebx		// 0x000b3d24
		xor ebx, ebx		// 0x000b3d25
		cmp eax, ebx		// 0x000b3d27
		push ebp		// 0x000b3d29
		push esi		// 0x000b3d2a
		mov esi, ecx		// 0x000b3d2b
		push edi		// 0x000b3d2d
		mov dword ptr [esi + 8], ebx		// 0x000b3d2e
		mov dword ptr [esi + 0xc], ebx		// 0x000b3d31
		mov word ptr [esi + 0x14], bx		// 0x000b3d34
		mov word ptr [esi + 0x16], bx		// 0x000b3d38
		mov byte ptr [esi + 0x18], bl		// 0x000b3d3c
		mov byte ptr [esi + 0x19], bl		// 0x000b3d3f
		je Lb3d76		// 0x000b3d42
		push offset nxP4ModeRb		// 0x000b3d44
		push eax		// 0x000b3d49
		call nxP4CallFopen		// 0x000b3d4a
		mov edi, eax		// 0x000b3d4f
		add esp, 8		// 0x000b3d51
		cmp edi, ebx		// 0x000b3d54
		je Lb3d76		// 0x000b3d56
		push 2		// 0x000b3d58
		push ebx		// 0x000b3d5a
		push edi		// 0x000b3d5b
		call nxP4CallFseek		// 0x000b3d5c
		push edi		// 0x000b3d61
		call nxP4CallFtell		// 0x000b3d62
		push edi		// 0x000b3d67
		mov ebp, eax		// 0x000b3d68
		call nxP4CallFclose		// 0x000b3d6a
		add esp, 0x14		// 0x000b3d6f
		cmp ebp, ebx		// 0x000b3d72
		jne Lb3d7b		// 0x000b3d74
Lb3d76:
		mov ebp, 0x1000		// 0x000b3d76
Lb3d7b:
		mov eax, dword ptr [esp + 0x14]		// 0x000b3d7b
		push offset nxP4ModeRb		// 0x000b3d7f
		push eax		// 0x000b3d84
		call nxP4CallFopen		// 0x000b3d85
		mov edi, eax		// 0x000b3d8a
		add esp, 8		// 0x000b3d8c
		cmp edi, ebx		// 0x000b3d8f
		je Lb3da7		// 0x000b3d91
		push ebx		// 0x000b3d93
		push edi		// 0x000b3d94
		push ebx		// 0x000b3d95
		push ebp		// 0x000b3d96
		mov ecx, esi		// 0x000b3d97
		call nxP4CallInitBlock		// 0x000b3d99
		push edi		// 0x000b3d9e
		call nxP4CallFclose		// 0x000b3d9f
		add esp, 4		// 0x000b3da4
Lb3da7:
		pop edi		// 0x000b3da7
		mov eax, esi		// 0x000b3da8
		pop esi		// 0x000b3daa
		pop ebp		// 0x000b3dab
		pop ebx		// 0x000b3dac
		ret 4		// 0x000b3dad
		}
	}

// phys_fn_004793 (0x000b3dc0, 102 B), 48 instructions
__declspec(naked) unsigned __fastcall nxP44793(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ebx		// 0x000b3dc0
		push esi		// 0x000b3dc1
		push edi		// 0x000b3dc2
		mov esi, ecx		// 0x000b3dc3
		call nxP4CallFlushBits		// 0x000b3dc5
		mov esi, dword ptr [esi + 4]		// 0x000b3dca
		mov eax, dword ptr [esi + 0xc]		// 0x000b3dcd
		test eax, eax		// 0x000b3dd0
		mov ebx, dword ptr [esp + 0x10]		// 0x000b3dd2
		je Lb3dfc		// 0x000b3dd6
Lb3dd8:
		mov edi, dword ptr [esi + 4]		// 0x000b3dd8
		test edi, edi		// 0x000b3ddb
		je Lb3df2		// 0x000b3ddd
		mov eax, dword ptr [esi]		// 0x000b3ddf
		push ebx		// 0x000b3de1
		push edi		// 0x000b3de2
		push 1		// 0x000b3de3
		push eax		// 0x000b3de5
		call nxP4CallFwrite		// 0x000b3de6
		add esp, 0x10		// 0x000b3deb
		cmp eax, edi		// 0x000b3dee
		jne Lb3e16		// 0x000b3df0
Lb3df2:
		mov esi, dword ptr [esi + 0xc]		// 0x000b3df2
		mov eax, dword ptr [esi + 0xc]		// 0x000b3df5
		test eax, eax		// 0x000b3df8
		jne Lb3dd8		// 0x000b3dfa
Lb3dfc:
		mov edi, dword ptr [esi + 4]		// 0x000b3dfc
		test edi, edi		// 0x000b3dff
		je Lb3e1e		// 0x000b3e01
		mov ecx, dword ptr [esi]		// 0x000b3e03
		push ebx		// 0x000b3e05
		push edi		// 0x000b3e06
		push 1		// 0x000b3e07
		push ecx		// 0x000b3e09
		call nxP4CallFwrite		// 0x000b3e0a
		add esp, 0x10		// 0x000b3e0f
		cmp eax, edi		// 0x000b3e12
		je Lb3e1e		// 0x000b3e14
Lb3e16:
		pop edi		// 0x000b3e16
		pop esi		// 0x000b3e17
		xor al, al		// 0x000b3e18
		pop ebx		// 0x000b3e1a
		ret 4		// 0x000b3e1b
Lb3e1e:
		pop edi		// 0x000b3e1e
		pop esi		// 0x000b3e1f
		mov al, 1		// 0x000b3e20
		pop ebx		// 0x000b3e22
		ret 4		// 0x000b3e23
		}
	}

// phys_fn_004799 (0x000b3f50, 93 B), 42 instructions
__declspec(naked) unsigned __fastcall nxP44799(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		push esi		// 0x000b3f50
		push edi		// 0x000b3f51
		mov edi, dword ptr [esp + 0xc]		// 0x000b3f52
		test edi, edi		// 0x000b3f56
		mov esi, ecx		// 0x000b3f58
		je Lb3fa6		// 0x000b3f5a
		call nxP4CallFlushBits		// 0x000b3f5c
		mov eax, dword ptr [esp + 0x10]		// 0x000b3f61
		test eax, eax		// 0x000b3f65
		je Lb3fa6		// 0x000b3f67
		push ebx		// 0x000b3f69
		push ebp		// 0x000b3f6a
		mov ebp, eax		// 0x000b3f6b
		lea ecx, [ecx]		// 0x000b3f6d
Lb3f70:
		mov bl, byte ptr [edi]		// 0x000b3f70
		mov ecx, esi		// 0x000b3f72
		call nxP4CallFlushBits		// 0x000b3f74
		mov edx, dword ptr [eax]		// 0x000b3f79
		mov ecx, dword ptr [edx + 4]		// 0x000b3f7b
		inc ecx		// 0x000b3f7e
		cmp ecx, dword ptr [edx + 8]		// 0x000b3f7f
		jbe Lb3f8e		// 0x000b3f82
		push 0		// 0x000b3f84
		push edx		// 0x000b3f86
		mov ecx, eax		// 0x000b3f87
		call nxP4CallAppendBlock		// 0x000b3f89
Lb3f8e:
		mov eax, dword ptr [esi]		// 0x000b3f8e
		mov edx, dword ptr [eax + 4]		// 0x000b3f90
		add edx, dword ptr [eax]		// 0x000b3f93
		mov dword ptr [esi + 0x10], edx		// 0x000b3f95
		inc dword ptr [eax + 4]		// 0x000b3f98
		mov eax, dword ptr [esi + 0x10]		// 0x000b3f9b
		inc edi		// 0x000b3f9e
		dec ebp		// 0x000b3f9f
		mov byte ptr [eax], bl		// 0x000b3fa0
		jne Lb3f70		// 0x000b3fa2
		pop ebp		// 0x000b3fa4
		pop ebx		// 0x000b3fa5
Lb3fa6:
		pop edi		// 0x000b3fa6
		mov eax, esi		// 0x000b3fa7
		pop esi		// 0x000b3fa9
		ret 8		// 0x000b3faa
		}
	}

// phys_fn_004801 (0x000b3fb0, 79 B), 35 instructions
__declspec(naked) unsigned __fastcall nxP44801(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [esp + 8]		// 0x000b3fb0
		test eax, eax		// 0x000b3fb4
		push esi		// 0x000b3fb6
		push edi		// 0x000b3fb7
		mov edi, ecx		// 0x000b3fb8
		je Lb3fc4		// 0x000b3fba
		push eax		// 0x000b3fbc
		mov eax, dword ptr [esp + 0x10]		// 0x000b3fbd
		push eax		// 0x000b3fc1
		jmp Lb3fce		// 0x000b3fc2
Lb3fc4:
		mov ecx, dword ptr [esp + 0xc]		// 0x000b3fc4
		push offset nxP4ModeWb		// 0x000b3fc8
		push ecx		// 0x000b3fcd
Lb3fce:
		call nxP4CallFopen		// 0x000b3fce
		mov esi, eax		// 0x000b3fd3
		add esp, 8		// 0x000b3fd5
		test esi, esi		// 0x000b3fd8
		jne Lb3fe3		// 0x000b3fda
		pop edi		// 0x000b3fdc
		xor al, al		// 0x000b3fdd
		pop esi		// 0x000b3fdf
		ret 8		// 0x000b3fe0
Lb3fe3:
		push ebx		// 0x000b3fe3
		push esi		// 0x000b3fe4
		mov ecx, edi		// 0x000b3fe5
		call nxP44793		// 0x000b3fe7
		push esi		// 0x000b3fec
		mov bl, al		// 0x000b3fed
		call nxP4CallFclose		// 0x000b3fef
		add esp, 4		// 0x000b3ff4
		mov al, bl		// 0x000b3ff7
		pop ebx		// 0x000b3ff9
		pop edi		// 0x000b3ffa
		pop esi		// 0x000b3ffb
		ret 8		// 0x000b3ffc
		}
	}

// phys_fn_004820 and phys_fn_004822 (one function) (0x000b4790, 595 B), 202 instructions
__declspec(naked) unsigned __cdecl nxP44820()
	{
	__asm
		{
		sub esp, 0x20		// 0x000b4790
		push esi		// 0x000b4793
		mov esi, dword ptr [esp + 0x28]		// 0x000b4794
		test esi, esi		// 0x000b4798
		push edi		// 0x000b479a
		je Lb49db		// 0x000b479b
		mov edi, dword ptr [esp + 0x30]		// 0x000b47a1
		test edi, edi		// 0x000b47a5
		je Lb49db		// 0x000b47a7
		mov eax, dword ptr [esp + 0x38]		// 0x000b47ad
		mov ecx, dword ptr [eax + 4]		// 0x000b47b1
		mov edx, dword ptr [eax + 8]		// 0x000b47b4
		push ebx		// 0x000b47b7
		mov ebx, dword ptr [eax]		// 0x000b47b8
		lea eax, [esi*4 + 4]		// 0x000b47ba
		push ebp		// 0x000b47c1
		push eax		// 0x000b47c2
		mov dword ptr [esp + 0x14], ebx		// 0x000b47c3
		mov dword ptr [esp + 0x20], ecx		// 0x000b47c7
		mov dword ptr [esp + 0x24], edx		// 0x000b47cb
		call nxP4Call2		// 0x000b47cf
		add esp, 4		// 0x000b47d4
		xor edx, edx		// 0x000b47d7
		cmp esi, 4		// 0x000b47d9
		mov ebp, eax		// 0x000b47dc
		jl Lb4818		// 0x000b47de
		lea eax, [edi + ebx*4 + 0x30]		// 0x000b47e0
		lea edi, [esi - 4]		// 0x000b47e4
		shr edi, 2		// 0x000b47e7
		inc edi		// 0x000b47ea
		lea ecx, [ebp + 8]		// 0x000b47eb
		lea edx, [edi*4]		// 0x000b47ee
Lb47f5:
		mov ebx, dword ptr [eax - 0x30]		// 0x000b47f5
		mov dword ptr [ecx - 8], ebx		// 0x000b47f8
		mov ebx, dword ptr [eax - 0x18]		// 0x000b47fb
		mov dword ptr [ecx - 4], ebx		// 0x000b47fe
		mov ebx, dword ptr [eax]		// 0x000b4801
		mov dword ptr [ecx], ebx		// 0x000b4803
		mov ebx, dword ptr [eax + 0x18]		// 0x000b4805
		mov dword ptr [ecx + 4], ebx		// 0x000b4808
		add eax, 0x60		// 0x000b480b
		add ecx, 0x10		// 0x000b480e
		dec edi		// 0x000b4811
		jne Lb47f5		// 0x000b4812
		mov edi, dword ptr [esp + 0x38]		// 0x000b4814
Lb4818:
		cmp edx, esi		// 0x000b4818
		jae Lb483e		// 0x000b481a
		mov eax, dword ptr [esp + 0x10]		// 0x000b481c
		lea ecx, [edx + edx*2]		// 0x000b4820
		lea ecx, [eax + ecx*2]		// 0x000b4823
		lea eax, [edi + ecx*4]		// 0x000b4826
		lea esp, [esp]		// 0x000b4829
Lb4830:
		mov ecx, dword ptr [eax]		// 0x000b4830
		mov dword ptr [ebp + edx*4], ecx		// 0x000b4832
		inc edx		// 0x000b4836
		add eax, 0x18		// 0x000b4837
		cmp edx, esi		// 0x000b483a
		jb Lb4830		// 0x000b483c
Lb483e:
		mov eax, dword ptr [nxKnCompleteSorter]		// 0x000b483e
		mov dword ptr [ebp + esi*4], 0x7f7fffff		// 0x000b4843
		inc esi		// 0x000b484b
		test eax, eax		// 0x000b484c
		jne Lb4873		// 0x000b484e
		call nxP4CallNxGetSdkAllocator		// 0x000b4850
		mov edx, dword ptr [eax]		// 0x000b4855
		push 0		// 0x000b4857
		push 0x18		// 0x000b4859
		mov ecx, eax		// 0x000b485b
		call dword ptr [edx]		// 0x000b485d
		test eax, eax		// 0x000b485f
		je Lb486c		// 0x000b4861
		mov ecx, eax		// 0x000b4863
		call nxP4Call0RadixSort		// 0x000b4865
		jmp Lb486e		// 0x000b486a
Lb486c:
		xor eax, eax		// 0x000b486c
Lb486e:
		mov dword ptr [nxKnCompleteSorter], eax		// 0x000b486e
Lb4873:
		push esi		// 0x000b4873
		push ebp		// 0x000b4874
		mov ecx, eax		// 0x000b4875
		call nxP4CallSort		// 0x000b4877
		mov eax, dword ptr [eax + 4]		// 0x000b487c
		lea esi, [eax + esi*4]		// 0x000b487f
		cmp eax, esi		// 0x000b4882
		mov dword ptr [esp + 0x18], esi		// 0x000b4884
		mov ecx, eax		// 0x000b4888
		jae Lb49c4		// 0x000b488a
		mov edi, dword ptr [esp + 0x3c]		// 0x000b4890
		jmp Lb48a0		// 0x000b4894
Lb4896:
		mov esi, dword ptr [esp + 0x18]		// 0x000b4896
		mov eax, dword ptr [esp + 0x14]		// 0x000b489a
		mov edi, edi		// 0x000b489e
Lb48a0:
		cmp eax, esi		// 0x000b48a0
		jae Lb49c4		// 0x000b48a2
		mov edx, dword ptr [eax]		// 0x000b48a8
		add eax, 4		// 0x000b48aa
		mov dword ptr [esp + 0x24], edx		// 0x000b48ad
		mov dword ptr [esp + 0x14], eax		// 0x000b48b1
Lb48b5:
		mov eax, dword ptr [ecx]		// 0x000b48b5
		fld dword ptr [ebp + eax*4]		// 0x000b48b7
		add ecx, 4		// 0x000b48bb
		fcomp dword ptr [ebp + edx*4]		// 0x000b48be
		fnstsw ax		// 0x000b48c2
		test ah, 5		// 0x000b48c4
		jnp Lb48b5		// 0x000b48c7
		cmp ecx, esi		// 0x000b48c9
		mov dword ptr [esp + 0x2c], ecx		// 0x000b48cb
		jae Lb49c4		// 0x000b48cf
		mov ebx, dword ptr [esp + 0x10]		// 0x000b48d5
		mov esi, dword ptr [ecx]		// 0x000b48d9
		fld dword ptr [ebp + esi*4]		// 0x000b48db
		lea eax, [edx + edx*2]		// 0x000b48df
		lea eax, [ebx + eax*2]		// 0x000b48e2
		mov ebx, dword ptr [esp + 0x38]		// 0x000b48e5
		fcomp dword ptr [ebx + eax*4 + 0xc]		// 0x000b48e9
		lea eax, [ebx + eax*4 + 0xc]		// 0x000b48ed
		mov dword ptr [esp + 0x28], eax		// 0x000b48f1
		mov dword ptr [esp + 0x34], ecx		// 0x000b48f5
		fnstsw ax		// 0x000b48f9
		test ah, 0x41		// 0x000b48fb
		jp Lb4896		// 0x000b48fe
		lea ecx, [edx + edx*2]		// 0x000b4900
		lea ebx, [ebx + ecx*8]		// 0x000b4903
		jmp Lb4910		// 0x000b4906
		lea esp, [esp]		// 0x000b4908
		nop		// 0x000b490f
Lb4910:
		add dword ptr [esp + 0x34], 4		// 0x000b4910
		mov eax, dword ptr [esp + 0x38]		// 0x000b4915
		lea edx, [esi + esi*2]		// 0x000b4919
		lea ecx, [eax + edx*8]		// 0x000b491c
		mov edx, dword ptr [esp + 0x1c]		// 0x000b491f
		fld dword ptr [ebx + edx*4 + 0xc]		// 0x000b4923
		fcomp dword ptr [ecx + edx*4]		// 0x000b4927
		fnstsw ax		// 0x000b492a
		test ah, 5		// 0x000b492c
		jnp Lb49a0		// 0x000b492f
		fld dword ptr [ecx + edx*4 + 0xc]		// 0x000b4931
		fcomp dword ptr [ebx + edx*4]		// 0x000b4935
		fnstsw ax		// 0x000b4938
		test ah, 5		// 0x000b493a
		jnp Lb49a0		// 0x000b493d
		mov edx, dword ptr [esp + 0x20]		// 0x000b493f
		fld dword ptr [ebx + edx*4 + 0xc]		// 0x000b4943
		fcomp dword ptr [ecx + edx*4]		// 0x000b4947
		fnstsw ax		// 0x000b494a
		test ah, 5		// 0x000b494c
		jnp Lb49a0		// 0x000b494f
		fld dword ptr [ecx + edx*4 + 0xc]		// 0x000b4951
		fcomp dword ptr [ebx + edx*4]		// 0x000b4955
		fnstsw ax		// 0x000b4958
		test ah, 5		// 0x000b495a
		jnp Lb49a0		// 0x000b495d
		mov ecx, dword ptr [edi + 4]		// 0x000b495f
		cmp ecx, dword ptr [edi]		// 0x000b4962
		jne Lb496f		// 0x000b4964
		push 1		// 0x000b4966
		mov ecx, edi		// 0x000b4968
		call nxP4CallResize		// 0x000b496a
Lb496f:
		mov edx, dword ptr [edi + 4]		// 0x000b496f
		mov eax, dword ptr [edi + 8]		// 0x000b4972
		mov ecx, dword ptr [esp + 0x24]		// 0x000b4975
		mov dword ptr [eax + edx*4], ecx		// 0x000b4979
		mov edx, dword ptr [edi + 4]		// 0x000b497c
		mov ecx, dword ptr [edi]		// 0x000b497f
		inc edx		// 0x000b4981
		mov eax, edx		// 0x000b4982
		cmp eax, ecx		// 0x000b4984
		mov dword ptr [edi + 4], edx		// 0x000b4986
		jne Lb4994		// 0x000b4989
		push 1		// 0x000b498b
		mov ecx, edi		// 0x000b498d
		call nxP4CallResize		// 0x000b498f
Lb4994:
		mov edx, dword ptr [edi + 4]		// 0x000b4994
		mov eax, dword ptr [edi + 8]		// 0x000b4997
		mov dword ptr [eax + edx*4], esi		// 0x000b499a
		inc dword ptr [edi + 4]		// 0x000b499d
Lb49a0:
		mov ecx, dword ptr [esp + 0x34]		// 0x000b49a0
		mov esi, dword ptr [ecx]		// 0x000b49a4
		fld dword ptr [ebp + esi*4]		// 0x000b49a6
		mov edx, dword ptr [esp + 0x28]		// 0x000b49aa
		fcomp dword ptr [edx]		// 0x000b49ae
		fnstsw ax		// 0x000b49b0
		test ah, 0x41		// 0x000b49b2
		jnp Lb4910		// 0x000b49b5
		mov ecx, dword ptr [esp + 0x2c]		// 0x000b49bb
		jmp Lb4896		// 0x000b49bf
Lb49c4:
		test ebp, ebp		// 0x000b49c4
		je Lb49d1		// 0x000b49c6
		push ebp		// 0x000b49c8
		call nxP4CallFree		// 0x000b49c9
		add esp, 4		// 0x000b49ce
Lb49d1:
		pop ebp		// 0x000b49d1
		pop ebx		// 0x000b49d2
		pop edi		// 0x000b49d3
		mov al, 1		// 0x000b49d4
		pop esi		// 0x000b49d6
		add esp, 0x20		// 0x000b49d7
		ret		// 0x000b49da
Lb49db:
		pop edi		// 0x000b49db
		xor al, al		// 0x000b49dc
		pop esi		// 0x000b49de
		add esp, 0x20		// 0x000b49df
		ret		// 0x000b49e2
		}
	}

// phys_fn_004824 (0x000b49f0, 299 B), 106 instructions
__declspec(naked) unsigned __fastcall nxP44824(void* self, void* edxUnused)
	{
	__asm
		{
		push ecx		// 0x000b49f0
		mov eax, dword ptr [esp + 8]		// 0x000b49f1
		test eax, eax		// 0x000b49f5
		push ebx		// 0x000b49f7
		je Lb4b16		// 0x000b49f8
		mov ebx, dword ptr [esp + 0x10]		// 0x000b49fe
		test ebx, ebx		// 0x000b4a02
		je Lb4b16		// 0x000b4a04
		mov ecx, dword ptr [esp + 0x14]		// 0x000b4a0a
		test ecx, ecx		// 0x000b4a0e
		je Lb4b16		// 0x000b4a10
		mov edx, dword ptr [esp + 0x18]		// 0x000b4a16
		test edx, edx		// 0x000b4a1a
		je Lb4b16		// 0x000b4a1c
		test eax, eax		// 0x000b4a22
		mov dword ptr [esp + 4], 0		// 0x000b4a24
		jbe Lb4b11		// 0x000b4a2c
		push ebp		// 0x000b4a32
		push esi		// 0x000b4a33
		mov esi, dword ptr [esp + 0x24]		// 0x000b4a34
		push edi		// 0x000b4a38
		lea esp, [esp]		// 0x000b4a39
Lb4a40:
		xor ebp, ebp		// 0x000b4a40
		test ecx, ecx		// 0x000b4a42
		jbe Lb4afa		// 0x000b4a44
		mov edi, dword ptr [esp + 0x24]		// 0x000b4a4a
		add edi, 4		// 0x000b4a4e
Lb4a51:
		fld dword ptr [ebx + 0xc]		// 0x000b4a51
		fcomp dword ptr [edi - 4]		// 0x000b4a54
		fnstsw ax		// 0x000b4a57
		test ah, 5		// 0x000b4a59
		jnp Lb4aea		// 0x000b4a5c
		fld dword ptr [edi + 8]		// 0x000b4a62
		fcomp dword ptr [ebx]		// 0x000b4a65
		fnstsw ax		// 0x000b4a67
		test ah, 5		// 0x000b4a69
		jnp Lb4aea		// 0x000b4a6c
		fld dword ptr [ebx + 0x10]		// 0x000b4a6e
		fcomp dword ptr [edi]		// 0x000b4a71
		fnstsw ax		// 0x000b4a73
		test ah, 5		// 0x000b4a75
		jnp Lb4aea		// 0x000b4a78
		fld dword ptr [edi + 0xc]		// 0x000b4a7a
		fcomp dword ptr [ebx + 4]		// 0x000b4a7d
		fnstsw ax		// 0x000b4a80
		test ah, 5		// 0x000b4a82
		jnp Lb4aea		// 0x000b4a85
		fld dword ptr [ebx + 0x14]		// 0x000b4a87
		fcomp dword ptr [edi + 4]		// 0x000b4a8a
		fnstsw ax		// 0x000b4a8d
		test ah, 5		// 0x000b4a8f
		jnp Lb4aea		// 0x000b4a92
		fld dword ptr [edi + 0x10]		// 0x000b4a94
		fcomp dword ptr [ebx + 8]		// 0x000b4a97
		fnstsw ax		// 0x000b4a9a
		test ah, 5		// 0x000b4a9c
		jnp Lb4aea		// 0x000b4a9f
		mov eax, dword ptr [esi + 4]		// 0x000b4aa1
		cmp eax, dword ptr [esi]		// 0x000b4aa4
		jne Lb4ab1		// 0x000b4aa6
		push 1		// 0x000b4aa8
		mov ecx, esi		// 0x000b4aaa
		call nxP4CallResize		// 0x000b4aac
Lb4ab1:
		mov ecx, dword ptr [esi + 4]		// 0x000b4ab1
		mov edx, dword ptr [esi + 8]		// 0x000b4ab4
		mov eax, dword ptr [esp + 0x10]		// 0x000b4ab7
		mov dword ptr [edx + ecx*4], eax		// 0x000b4abb
		mov edx, dword ptr [esi + 4]		// 0x000b4abe
		mov ecx, dword ptr [esi]		// 0x000b4ac1
		inc edx		// 0x000b4ac3
		mov eax, edx		// 0x000b4ac4
		cmp eax, ecx		// 0x000b4ac6
		mov dword ptr [esi + 4], edx		// 0x000b4ac8
		jne Lb4ad6		// 0x000b4acb
		push 1		// 0x000b4acd
		mov ecx, esi		// 0x000b4acf
		call nxP4CallResize		// 0x000b4ad1
Lb4ad6:
		mov ecx, dword ptr [esi + 4]		// 0x000b4ad6
		mov edx, dword ptr [esi + 8]		// 0x000b4ad9
		mov dword ptr [edx + ecx*4], ebp		// 0x000b4adc
		mov eax, dword ptr [esi + 4]		// 0x000b4adf
		mov ecx, dword ptr [esp + 0x20]		// 0x000b4ae2
		inc eax		// 0x000b4ae6
		mov dword ptr [esi + 4], eax		// 0x000b4ae7
Lb4aea:
		inc ebp		// 0x000b4aea
		add edi, 0x18		// 0x000b4aeb
		cmp ebp, ecx		// 0x000b4aee
		jb Lb4a51		// 0x000b4af0
		mov eax, dword ptr [esp + 0x18]		// 0x000b4af6
Lb4afa:
		mov edx, dword ptr [esp + 0x10]		// 0x000b4afa
		inc edx		// 0x000b4afe
		add ebx, 0x18		// 0x000b4aff
		cmp edx, eax		// 0x000b4b02
		mov dword ptr [esp + 0x10], edx		// 0x000b4b04
		jb Lb4a40		// 0x000b4b08
		pop edi		// 0x000b4b0e
		pop esi		// 0x000b4b0f
		pop ebp		// 0x000b4b10
Lb4b11:
		mov al, 1		// 0x000b4b11
		pop ebx		// 0x000b4b13
		pop ecx		// 0x000b4b14
		ret		// 0x000b4b15
Lb4b16:
		xor al, al		// 0x000b4b16
		pop ebx		// 0x000b4b18
		pop ecx		// 0x000b4b19
		ret		// 0x000b4b1a
		}
	}

// phys_fn_004828 (0x000b4c40, 123 B), 36 instructions
__declspec(naked) unsigned __cdecl nxP44828()
	{
	__asm
		{
		mov ecx, dword ptr [nxKnBipartiteSorter1]		// 0x000b4c40
		test ecx, ecx		// 0x000b4c46
		push esi		// 0x000b4c48
		je Lb4c69		// 0x000b4c49
		mov esi, ecx		// 0x000b4c4b
		call nxP4Call1RadixSort		// 0x000b4c4d
		call nxP4CallNxGetSdkAllocator		// 0x000b4c52
		mov edx, dword ptr [eax]		// 0x000b4c57
		push esi		// 0x000b4c59
		mov ecx, eax		// 0x000b4c5a
		call dword ptr [edx + 0xc]		// 0x000b4c5c
		mov dword ptr [nxKnBipartiteSorter1], 0		// 0x000b4c5f
Lb4c69:
		mov ecx, dword ptr [nxKnBipartiteSorter0]		// 0x000b4c69
		test ecx, ecx		// 0x000b4c6f
		je Lb4c91		// 0x000b4c71
		mov esi, ecx		// 0x000b4c73
		call nxP4Call1RadixSort		// 0x000b4c75
		call nxP4CallNxGetSdkAllocator		// 0x000b4c7a
		mov edx, dword ptr [eax]		// 0x000b4c7f
		push esi		// 0x000b4c81
		mov ecx, eax		// 0x000b4c82
		call dword ptr [edx + 0xc]		// 0x000b4c84
		mov dword ptr [nxKnBipartiteSorter0], 0		// 0x000b4c87
Lb4c91:
		mov ecx, dword ptr [nxKnCompleteSorter]		// 0x000b4c91
		test ecx, ecx		// 0x000b4c97
		je Lb4cb9		// 0x000b4c99
		mov esi, ecx		// 0x000b4c9b
		call nxP4Call1RadixSort		// 0x000b4c9d
		call nxP4CallNxGetSdkAllocator		// 0x000b4ca2
		mov edx, dword ptr [eax]		// 0x000b4ca7
		push esi		// 0x000b4ca9
		mov ecx, eax		// 0x000b4caa
		call dword ptr [edx + 0xc]		// 0x000b4cac
		mov dword ptr [nxKnCompleteSorter], 0		// 0x000b4caf
Lb4cb9:
		pop esi		// 0x000b4cb9
		ret		// 0x000b4cba
		}
	}

// phys_fn_004830 (0x000b4cc0, 82 B), 30 instructions
__declspec(naked) unsigned __fastcall nxP44830(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [nxKnPrunerPool]		// 0x000b4cc0
		test eax, eax		// 0x000b4cc5
		push esi		// 0x000b4cc7
		mov esi, ecx		// 0x000b4cc8
		mov dword ptr [esi + 4], 0		// 0x000b4cca
		jne Lb4cf6		// 0x000b4cd1
		call nxP4CallNxGetSdkAllocator		// 0x000b4cd3
		mov edx, dword ptr [eax]		// 0x000b4cd8
		push 0		// 0x000b4cda
		push 0x1c		// 0x000b4cdc
		mov ecx, eax		// 0x000b4cde
		call dword ptr [edx]		// 0x000b4ce0
		test eax, eax		// 0x000b4ce2
		je Lb4d04		// 0x000b4ce4
		mov ecx, eax		// 0x000b4ce6
		call nxP4Call0NxPrunerProcessPool		// 0x000b4ce8
		test eax, eax		// 0x000b4ced
		mov dword ptr [nxKnPrunerPool], eax		// 0x000b4cef
		je Lb4d00		// 0x000b4cf4
Lb4cf6:
		push esi		// 0x000b4cf6
		mov ecx, eax		// 0x000b4cf7
		call nxP4CallAdd		// 0x000b4cf9
		mov dword ptr [esi], eax		// 0x000b4cfe
Lb4d00:
		mov eax, esi		// 0x000b4d00
		pop esi		// 0x000b4d02
		ret		// 0x000b4d03
Lb4d04:
		mov eax, esi		// 0x000b4d04
		mov dword ptr [nxKnPrunerPool], 0		// 0x000b4d06
		pop esi		// 0x000b4d10
		ret		// 0x000b4d11
		}
	}

// phys_fn_004832 (0x000b4d20, 21 B), 8 instructions
__declspec(naked) unsigned __fastcall nxP44832(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, ecx		// 0x000b4d20
		mov ecx, dword ptr [nxKnPrunerPool]		// 0x000b4d22
		test ecx, ecx		// 0x000b4d28
		je Lb4d34		// 0x000b4d2a
		mov eax, dword ptr [eax]		// 0x000b4d2c
		push eax		// 0x000b4d2e
		call nxP4CallRemove		// 0x000b4d2f
Lb4d34:
		ret		// 0x000b4d34
		}
	}

// phys_fn_004834 (0x000b4d40, 43 B), 14 instructions
__declspec(naked) unsigned __cdecl nxP44834()
	{
	__asm
		{
		mov ecx, dword ptr [nxKnPrunerPool]		// 0x000b4d40
		test ecx, ecx		// 0x000b4d46
		je Lb4d6a		// 0x000b4d48
		push esi		// 0x000b4d4a
		mov esi, ecx		// 0x000b4d4b
		call nxP4CallNxOpcodeReleasePool		// 0x000b4d4d
		call nxP4CallNxGetSdkAllocator		// 0x000b4d52
		mov edx, dword ptr [eax]		// 0x000b4d57
		push esi		// 0x000b4d59
		mov ecx, eax		// 0x000b4d5a
		call dword ptr [edx + 0xc]		// 0x000b4d5c
		mov dword ptr [nxKnPrunerPool], 0		// 0x000b4d5f
		pop esi		// 0x000b4d69
Lb4d6a:
		ret		// 0x000b4d6a
		}
	}

// phys_fn_004852 (0x000b5090, 147 B), 50 instructions
__declspec(naked) unsigned __fastcall nxP44852(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x000b5090
		cmp eax, 3		// 0x000b5094
		ja Lb511e		// 0x000b5097
		cmp eax, 0		// jump table 0x000b5124[0]
		je Lb50de
		cmp eax, 1		// jump table 0x000b5124[1]
		je Lb50c1
		cmp eax, 2		// jump table 0x000b5124[2]
		je Lb50a4
		jmp Lb50fe		// jump table 0x000b5124[3]		// 0x000b509d
Lb50a4:
		call nxP4CallNxGetSdkAllocator		// 0x000b50a4
		mov edx, dword ptr [eax]		// 0x000b50a9
		push 0		// 0x000b50ab
		push 0x3c		// 0x000b50ad
		mov ecx, eax		// 0x000b50af
		call dword ptr [edx]		// 0x000b50b1
		test eax, eax		// 0x000b50b3
		je Lb511e		// 0x000b50b5
		mov ecx, eax		// 0x000b50b7
		call nxP4Call0DynamicPruner		// 0x000b50b9
		ret 4		// 0x000b50be
Lb50c1:
		call nxP4CallNxGetSdkAllocator		// 0x000b50c1
		mov edx, dword ptr [eax]		// 0x000b50c6
		push 0		// 0x000b50c8
		push 0x40		// 0x000b50ca
		mov ecx, eax		// 0x000b50cc
		call dword ptr [edx]		// 0x000b50ce
		test eax, eax		// 0x000b50d0
		je Lb511e		// 0x000b50d2
		mov ecx, eax		// 0x000b50d4
		call nxP4CallNxPCBoundedCtor		// 0x000b50d6
		ret 4		// 0x000b50db
Lb50de:
		call nxP4CallNxGetSdkAllocator		// 0x000b50de
		mov edx, dword ptr [eax]		// 0x000b50e3
		push 0		// 0x000b50e5
		push 0x90		// 0x000b50e7
		mov ecx, eax		// 0x000b50ec
		call dword ptr [edx]		// 0x000b50ee
		test eax, eax		// 0x000b50f0
		je Lb511e		// 0x000b50f2
		mov ecx, eax		// 0x000b50f4
		call nxP4Call0StaticPruner		// 0x000b50f6
		ret 4		// 0x000b50fb
Lb50fe:
		call nxP4CallNxGetSdkAllocator		// 0x000b50fe
		mov edx, dword ptr [eax]		// 0x000b5103
		push 0		// 0x000b5105
		push 0xa0		// 0x000b5107
		mov ecx, eax		// 0x000b510c
		call dword ptr [edx]		// 0x000b510e
		test eax, eax		// 0x000b5110
		je Lb511e		// 0x000b5112
		mov ecx, eax		// 0x000b5114
		call nxP45448		// 0x000b5116
		ret 4		// 0x000b511b
Lb511e:
		xor eax, eax		// 0x000b511e
		ret 4		// 0x000b5120
		}
	}

// phys_fn_004870 (0x000b5460, 25 B), 9 instructions
__declspec(naked) unsigned __cdecl nxP44870()
	{
	__asm
		{
		mov eax, dword ptr [nxKnOwnerQuery]		// 0x000b5460
		test eax, eax		// 0x000b5465
		je Lb5476		// 0x000b5467
		mov ecx, dword ptr [esp + 4]		// 0x000b5469
		mov edx, dword ptr [ecx + 4]		// 0x000b546d
		mov dword ptr [esp + 4], edx		// 0x000b5470
		jmp eax		// 0x000b5474
Lb5476:
		xor eax, eax		// 0x000b5476
		ret		// 0x000b5478
		}
	}

// phys_fn_004872 (0x000b5480, 23 B), 8 instructions
__declspec(naked) unsigned __cdecl nxP44872()
	{
	__asm
		{
		mov eax, dword ptr [nxKnOwnerNotify]		// 0x000b5480
		test eax, eax		// 0x000b5485
		je Lb5496		// 0x000b5487
		mov edx, dword ptr [esp + 4]		// 0x000b5489
		mov ecx, dword ptr [edx + 4]		// 0x000b548d
		mov dword ptr [esp + 4], ecx		// 0x000b5490
		jmp eax		// 0x000b5494
Lb5496:
		ret		// 0x000b5496
		}
	}

// helper: phys_fn_005229 (record unchanged) (0x000e5500, 133 B), 47 instructions
__declspec(naked) unsigned __fastcall nxP45229(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		sub esp, 0x44		// 0x000e5500
		push esi		// 0x000e5503
		mov esi, ecx		// 0x000e5504
		mov eax, dword ptr [esi + 0x3c]		// 0x000e5506
		test eax, eax		// 0x000e5509
		jne Le5522		// 0x000e550b
		call nxP4CallBuildTree		// 0x000e550d
		mov eax, dword ptr [esi + 0x3c]		// 0x000e5512
		test eax, eax		// 0x000e5515
		jne Le5522		// 0x000e5517
		xor al, al		// 0x000e5519
		pop esi		// 0x000e551b
		add esp, 0x44		// 0x000e551c
		ret 0x10		// 0x000e551f
Le5522:
		lea ecx, [esp + 4]		// 0x000e5522
		call nxP4Call0SphereCollider		// 0x000e5526
		mov al, byte ptr [esp + 0x54]		// 0x000e552b
		test al, al		// 0x000e552f
		mov eax, dword ptr [esp + 8]		// 0x000e5531
		je Le553c		// 0x000e5535
		or eax, 1		// 0x000e5537
		jmp Le553f		// 0x000e553a
Le553c:
		and eax, 0xfffffffe		// 0x000e553c
Le553f:
		mov ecx, dword ptr [esp + 0x50]		// 0x000e553f
		mov dword ptr [esp + 8], eax		// 0x000e5543
		mov edx, eax		// 0x000e5547
		mov eax, dword ptr [esi + 0x3c]		// 0x000e5549
		and edx, 0xfffffffd		// 0x000e554c
		push eax		// 0x000e554f
		mov dword ptr [esp + 0xc], edx		// 0x000e5550
		push ecx		// 0x000e5554
		lea edx, [esi + 0x50]		// 0x000e5555
		push edx		// 0x000e5558
		lea ecx, [esp + 0x10]		// 0x000e5559
		call nxP4CallCollide		// 0x000e555d
		mov eax, dword ptr [esp + 0x58]		// 0x000e5562
		mov ecx, dword ptr [esp + 0x4c]		// 0x000e5566
		push eax		// 0x000e556a
		push ecx		// 0x000e556b
		mov ecx, esi		// 0x000e556c
		call nxP4CallReportTouched		// 0x000e556e
		lea ecx, [esp + 4]		// 0x000e5573
		call nxP4Call1SphereCollider		// 0x000e5577
		mov al, 1		// 0x000e557c
		pop esi		// 0x000e557e
		add esp, 0x44		// 0x000e557f
		ret 0x10		// 0x000e5582
		}
	}

// phys_fn_005231 (0x000e5590, 320 B), 90 instructions
__declspec(naked) unsigned __fastcall nxP45231(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		sub esp, 0xb8		// 0x000e5590
		push esi		// 0x000e5596
		mov esi, ecx		// 0x000e5597
		mov eax, dword ptr [esi + 0x3c]		// 0x000e5599
		test eax, eax		// 0x000e559c
		jne Le55b8		// 0x000e559e
		call nxP4CallBuildTree		// 0x000e55a0
		mov eax, dword ptr [esi + 0x3c]		// 0x000e55a5
		test eax, eax		// 0x000e55a8
		jne Le55b8		// 0x000e55aa
		xor al, al		// 0x000e55ac
		pop esi		// 0x000e55ae
		add esp, 0xb8		// 0x000e55af
		ret 0x10		// 0x000e55b5
Le55b8:
		lea ecx, [esp + 0x34]		// 0x000e55b8
		call nxP4Call0AABBCollider		// 0x000e55bc
		mov al, byte ptr [esp + 0xc8]		// 0x000e55c1
		test al, al		// 0x000e55c8
		mov eax, dword ptr [esp + 0x38]		// 0x000e55ca
		je Le55d5		// 0x000e55ce
		or eax, 1		// 0x000e55d0
		jmp Le55d8		// 0x000e55d3
Le55d5:
		and eax, 0xfffffffe		// 0x000e55d5
Le55d8:
		mov dword ptr [esp + 0x38], eax		// 0x000e55d8
		mov ecx, eax		// 0x000e55dc
		mov eax, dword ptr [esp + 0xc4]		// 0x000e55de
		fld dword ptr [eax]		// 0x000e55e5
		and ecx, 0xfffffffd		// 0x000e55e7
		fadd dword ptr [eax + 0xc]		// 0x000e55ea
		mov dword ptr [esp + 0x38], ecx		// 0x000e55ed
		fld dword ptr [eax + 0x10]		// 0x000e55f1
		fadd dword ptr [eax + 4]		// 0x000e55f4
		fld dword ptr [eax + 0x14]		// 0x000e55f7
		fadd dword ptr [eax + 8]		// 0x000e55fa
		fstp dword ptr [esp + 0x18]		// 0x000e55fd
		fxch st(1)		// 0x000e5601
		fmul dword ptr [nxP4K101043cc_4]		// 0x000e5603
		fstp dword ptr [esp + 4]		// 0x000e5609
		mov ecx, dword ptr [esp + 4]		// 0x000e560d
		mov dword ptr [esp + 0x1c], ecx		// 0x000e5611
		fmul dword ptr [nxP4K101043cc_4]		// 0x000e5615
		fstp dword ptr [esp + 8]		// 0x000e561b
		mov edx, dword ptr [esp + 8]		// 0x000e561f
		fld dword ptr [esp + 0x18]		// 0x000e5623
		mov dword ptr [esp + 0x20], edx		// 0x000e5627
		fmul dword ptr [nxP4K101043cc_4]		// 0x000e562b
		fstp dword ptr [esp + 0xc]		// 0x000e5631
		mov ecx, dword ptr [esp + 0xc]		// 0x000e5635
		fld dword ptr [eax + 0xc]		// 0x000e5639
		mov dword ptr [esp + 0x24], ecx		// 0x000e563c
		fsub dword ptr [eax]		// 0x000e5640
		fld dword ptr [eax + 0x10]		// 0x000e5642
		fsub dword ptr [eax + 4]		// 0x000e5645
		fld dword ptr [eax + 0x14]		// 0x000e5648
		fsub dword ptr [eax + 8]		// 0x000e564b
		fstp dword ptr [esp + 0x18]		// 0x000e564e
		fxch st(1)		// 0x000e5652
		fmul dword ptr [nxP4K101043cc_4]		// 0x000e5654
		fstp dword ptr [esp + 4]		// 0x000e565a
		mov edx, dword ptr [esp + 4]		// 0x000e565e
		mov dword ptr [esp + 0x28], edx		// 0x000e5662
		fmul dword ptr [nxP4K101043cc_4]		// 0x000e5666
		mov edx, dword ptr [esi + 0x3c]		// 0x000e566c
		push edx		// 0x000e566f
		fstp dword ptr [esp + 0xc]		// 0x000e5670
		mov eax, dword ptr [esp + 0xc]		// 0x000e5674
		fld dword ptr [esp + 0x1c]		// 0x000e5678
		mov dword ptr [esp + 0x30], eax		// 0x000e567c
		fmul dword ptr [nxP4K101043cc_4]		// 0x000e5680
		lea eax, [esp + 0x20]		// 0x000e5686
		push eax		// 0x000e568a
		fstp dword ptr [esp + 0x14]		// 0x000e568b
		mov ecx, dword ptr [esp + 0x14]		// 0x000e568f
		mov dword ptr [esp + 0x38], ecx		// 0x000e5693
		lea ecx, [esi + 0x6c]		// 0x000e5697
		push ecx		// 0x000e569a
		lea ecx, [esp + 0x40]		// 0x000e569b
		call nxP4CallCollideX		// 0x000e569f
		mov edx, dword ptr [esp + 0xcc]		// 0x000e56a4
		mov eax, dword ptr [esp + 0xc0]		// 0x000e56ab
		push edx		// 0x000e56b2
		push eax		// 0x000e56b3
		mov ecx, esi		// 0x000e56b4
		call nxP4CallReportTouched		// 0x000e56b6
		lea ecx, [esp + 0x34]		// 0x000e56bb
		call nxP4Call1AABBCollider		// 0x000e56bf
		mov al, 1		// 0x000e56c4
		pop esi		// 0x000e56c6
		add esp, 0xb8		// 0x000e56c7
		ret 0x10		// 0x000e56cd
		}
	}

// phys_fn_005262 (0x000e6230, 512 B), 169 instructions
__declspec(naked) unsigned __fastcall nxP45262(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		push ecx		// 0x000e6230
		push ebx		// 0x000e6231
		mov ebx, ecx		// 0x000e6232
		mov eax, dword ptr [ebx + 0x3c]		// 0x000e6234
		push esi		// 0x000e6237
		xor esi, esi		// 0x000e6238
		cmp eax, esi		// 0x000e623a
		jne Le6250		// 0x000e623c
		call nxP4CallNxPCBuildTree		// 0x000e623e
		cmp dword ptr [ebx + 0x3c], esi		// 0x000e6243
		jne Le6250		// 0x000e6246
		pop esi		// 0x000e6248
		xor al, al		// 0x000e6249
		pop ebx		// 0x000e624b
		pop ecx		// 0x000e624c
		ret 0x10		// 0x000e624d
Le6250:
		test byte ptr [nxPCStatics + 124], 1		// 0x000e6250
		jne Le627f		// 0x000e6257
		mov edx, dword ptr [nxPCStatics + 124]		// 0x000e6259
		or edx, 1		// 0x000e625f
		mov ecx, offset nxPCStatics + 108		// 0x000e6262
		mov dword ptr [nxPCStatics + 124], edx		// 0x000e6267
		call nxP4Call0SdkContainer		// 0x000e626d
		push offset nxP4ThunkG		// 0x000e6272
		call nxP4CallAtexit		// 0x000e6277
		add esp, 4		// 0x000e627c
Le627f:
		test byte ptr [nxPCStatics + 124], 2		// 0x000e627f
		jne Le62ae		// 0x000e6286
		mov edx, dword ptr [nxPCStatics + 124]		// 0x000e6288
		or edx, 2		// 0x000e628e
		mov ecx, offset nxPCStatics + 92		// 0x000e6291
		mov dword ptr [nxPCStatics + 124], edx		// 0x000e6296
		call nxP4Call0SdkContainer		// 0x000e629c
		push offset nxP4ThunkF		// 0x000e62a1
		call nxP4CallAtexit		// 0x000e62a6
		add esp, 4		// 0x000e62ab
Le62ae:
		cmp dword ptr [nxPCStatics + 112], esi		// 0x000e62ae
		je Le62bc		// 0x000e62b4
		mov dword ptr [nxPCStatics + 112], esi		// 0x000e62b6
Le62bc:
		cmp dword ptr [nxPCStatics + 96], esi		// 0x000e62bc
		je Le62ca		// 0x000e62c2
		mov dword ptr [nxPCStatics + 96], esi		// 0x000e62c4
Le62ca:
		mov ecx, dword ptr [ebx + 0x3c]		// 0x000e62ca
		push ebp		// 0x000e62cd
		mov ebp, dword ptr [esp + 0x18]		// 0x000e62ce
		push edi		// 0x000e62d2
		push offset nxPCStatics + 92		// 0x000e62d3
		push offset nxPCStatics + 108		// 0x000e62d8
		push ebp		// 0x000e62dd
		call nxP45568		// 0x000e62de
		mov eax, dword ptr [nxPCStatics + 112]		// 0x000e62e3
		mov edi, dword ptr [esp + 0x18]		// 0x000e62e8
		xor ecx, ecx		// 0x000e62ec
		cmp eax, esi		// 0x000e62ee
		mov dword ptr [esp + 0x10], eax		// 0x000e62f0
		mov dword ptr [esp + 0x1c], ecx		// 0x000e62f4
		jbe Le63d7		// 0x000e62f8
		mov edi, edi		// 0x000e62fe
Le6300:
		mov eax, dword ptr [nxPCStatics + 116]		// 0x000e6300
		mov ecx, dword ptr [eax + ecx*4]		// 0x000e6305
		mov esi, dword ptr [ecx + 4]		// 0x000e6308
		mov edx, dword ptr [esp + 0x24]		// 0x000e630b
		test dword ptr [esi + 0x24], edx		// 0x000e630f
		je Le63c2		// 0x000e6312
		mov ax, word ptr [esi + 0x28]		// 0x000e6318
		cmp ax, 0xffff		// 0x000e631c
		jne Le6326		// 0x000e6320
		xor ecx, ecx		// 0x000e6322
		jmp Le634d		// 0x000e6324
Le6326:
		test byte ptr [esi + 8], 2		// 0x000e6326
		jne Le6340		// 0x000e632a
		mov ecx, dword ptr [ebx + 0x14]		// 0x000e632c
		movzx eax, ax		// 0x000e632f
		lea eax, [eax + eax*2]		// 0x000e6332
		lea edx, [ecx + eax*8]		// 0x000e6335
		push edx		// 0x000e6338
		mov ecx, esi		// 0x000e6339
		call nxP4CallUpdateWorldAABB		// 0x000e633b
Le6340:
		movzx eax, word ptr [esi + 0x28]		// 0x000e6340
		mov ecx, dword ptr [ebx + 0x14]		// 0x000e6344
		lea eax, [eax + eax*2]		// 0x000e6347
		lea ecx, [ecx + eax*8]		// 0x000e634a
Le634d:
		fld dword ptr [ebp + 0xc]		// 0x000e634d
		fcomp dword ptr [ecx]		// 0x000e6350
		fnstsw ax		// 0x000e6352
		test ah, 5		// 0x000e6354
		jnp Le63c2		// 0x000e6357
		fld dword ptr [ecx + 0xc]		// 0x000e6359
		fcomp dword ptr [ebp]		// 0x000e635c
		fnstsw ax		// 0x000e635f
		test ah, 5		// 0x000e6361
		jnp Le63c2		// 0x000e6364
		fld dword ptr [ebp + 0x10]		// 0x000e6366
		fcomp dword ptr [ecx + 4]		// 0x000e6369
		fnstsw ax		// 0x000e636c
		test ah, 5		// 0x000e636e
		jnp Le63c2		// 0x000e6371
		fld dword ptr [ecx + 0x10]		// 0x000e6373
		fcomp dword ptr [ebp + 4]		// 0x000e6376
		fnstsw ax		// 0x000e6379
		test ah, 5		// 0x000e637b
		jnp Le63c2		// 0x000e637e
		fld dword ptr [ebp + 0x14]		// 0x000e6380
		fcomp dword ptr [ecx + 8]		// 0x000e6383
		fnstsw ax		// 0x000e6386
		test ah, 5		// 0x000e6388
		jnp Le63c2		// 0x000e638b
		fld dword ptr [ecx + 0x14]		// 0x000e638d
		fcomp dword ptr [ebp + 8]		// 0x000e6390
		fnstsw ax		// 0x000e6393
		test ah, 5		// 0x000e6395
		jnp Le63c2		// 0x000e6398
		mov edx, dword ptr [edi + 4]		// 0x000e639a
		cmp edx, dword ptr [edi]		// 0x000e639d
		jne Le63aa		// 0x000e639f
		push 1		// 0x000e63a1
		mov ecx, edi		// 0x000e63a3
		call nxP4CallResize		// 0x000e63a5
Le63aa:
		mov eax, dword ptr [edi + 4]		// 0x000e63aa
		mov ecx, dword ptr [edi + 8]		// 0x000e63ad
		mov dword ptr [ecx + eax*4], esi		// 0x000e63b0
		mov ecx, dword ptr [edi + 4]		// 0x000e63b3
		mov al, byte ptr [esp + 0x20]		// 0x000e63b6
		inc ecx		// 0x000e63ba
		test al, al		// 0x000e63bb
		mov dword ptr [edi + 4], ecx		// 0x000e63bd
		jne Le6426		// 0x000e63c0
Le63c2:
		mov ecx, dword ptr [esp + 0x1c]		// 0x000e63c2
		mov eax, dword ptr [esp + 0x10]		// 0x000e63c6
		inc ecx		// 0x000e63ca
		cmp ecx, eax		// 0x000e63cb
		mov dword ptr [esp + 0x1c], ecx		// 0x000e63cd
		jb Le6300		// 0x000e63d1
Le63d7:
		mov eax, dword ptr [nxPCStatics + 96]		// 0x000e63d7
		xor ebp, ebp		// 0x000e63dc
		test eax, eax		// 0x000e63de
		mov ebx, eax		// 0x000e63e0
		jbe Le6426		// 0x000e63e2
Le63e4:
		mov edx, dword ptr [nxPCStatics + 100]		// 0x000e63e4
		mov eax, dword ptr [edx + ebp*4]		// 0x000e63ea
		mov esi, dword ptr [eax + 4]		// 0x000e63ed
		mov ecx, dword ptr [esp + 0x24]		// 0x000e63f0
		test dword ptr [esi + 0x24], ecx		// 0x000e63f4
		je Le6421		// 0x000e63f7
		mov edx, dword ptr [edi + 4]		// 0x000e63f9
		cmp edx, dword ptr [edi]		// 0x000e63fc
		jne Le6409		// 0x000e63fe
		push 1		// 0x000e6400
		mov ecx, edi		// 0x000e6402
		call nxP4CallResize		// 0x000e6404
Le6409:
		mov eax, dword ptr [edi + 4]		// 0x000e6409
		mov ecx, dword ptr [edi + 8]		// 0x000e640c
		mov dword ptr [ecx + eax*4], esi		// 0x000e640f
		mov ecx, dword ptr [edi + 4]		// 0x000e6412
		mov al, byte ptr [esp + 0x20]		// 0x000e6415
		inc ecx		// 0x000e6419
		test al, al		// 0x000e641a
		mov dword ptr [edi + 4], ecx		// 0x000e641c
		jne Le6426		// 0x000e641f
Le6421:
		inc ebp		// 0x000e6421
		cmp ebp, ebx		// 0x000e6422
		jb Le63e4		// 0x000e6424
Le6426:
		pop edi		// 0x000e6426
		pop ebp		// 0x000e6427
		pop esi		// 0x000e6428
		mov al, 1		// 0x000e6429
		pop ebx		// 0x000e642b
		pop ecx		// 0x000e642c
		ret 0x10		// 0x000e642d
		}
	}

// phys_fn_005263 (0x000e6430, 78 B), 28 instructions
__declspec(naked) unsigned __fastcall nxP45263(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000e6430
		mov esi, ecx		// 0x000e6431
		push edi		// 0x000e6433
		mov edi, dword ptr [esi + 0x3c]		// 0x000e6434
		test edi, edi		// 0x000e6437
		mov dword ptr [esi], offset nxKnVtBounded		// 0x000e6439
		je Le645c		// 0x000e643f
		mov ecx, edi		// 0x000e6441
		call nxP4CallNxPCOctreeRelease		// 0x000e6443
		call nxP4CallNxGetSdkAllocator		// 0x000e6448
		mov edx, dword ptr [eax]		// 0x000e644d
		push edi		// 0x000e644f
		mov ecx, eax		// 0x000e6450
		call dword ptr [edx + 0xc]		// 0x000e6452
		mov dword ptr [esi + 0x3c], 0		// 0x000e6455
Le645c:
		mov ecx, esi		// 0x000e645c
		call nxP4Call1Pruner		// 0x000e645e
		test byte ptr [esp + 0xc], 1		// 0x000e6463
		je Le6477		// 0x000e6468
		call nxP4CallNxGetSdkAllocator		// 0x000e646a
		mov edx, dword ptr [eax]		// 0x000e646f
		push esi		// 0x000e6471
		mov ecx, eax		// 0x000e6472
		call dword ptr [edx + 0xc]		// 0x000e6474
Le6477:
		pop edi		// 0x000e6477
		mov eax, esi		// 0x000e6478
		pop esi		// 0x000e647a
		ret 4		// 0x000e647b
		}
	}

// phys_fn_005279 (0x000e6b50, 186 B), 63 instructions
__declspec(naked) unsigned __fastcall nxP45279(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x000e6b50
		sub esp, 0x30		// 0x000e6b54
		test eax, eax		// 0x000e6b57
		push esi		// 0x000e6b59
		mov esi, ecx		// 0x000e6b5a
		je Le6c03		// 0x000e6b5c
		mov eax, dword ptr [esi + 0x18]		// 0x000e6b62
		push edi		// 0x000e6b65
		xor edi, edi		// 0x000e6b66
		test eax, eax		// 0x000e6b68
		jbe Le6c02		// 0x000e6b6a
		push ebx		// 0x000e6b70
		push ebp		// 0x000e6b71
		mov ebp, dword ptr [esp + 0x48]		// 0x000e6b72
		xor ebx, ebx		// 0x000e6b76
Le6b78:
		mov eax, dword ptr [esi + 0x1c]		// 0x000e6b78
		mov ecx, dword ptr [eax + ebx]		// 0x000e6b7b
		mov edx, dword ptr [ecx]		// 0x000e6b7e
		mov ecx, dword ptr [eax + ebx + 4]		// 0x000e6b80
		add eax, ebx		// 0x000e6b84
		mov dword ptr [esp + 0x10], edx		// 0x000e6b86
		mov edx, dword ptr [ecx]		// 0x000e6b8a
		mov ecx, dword ptr [eax + 8]		// 0x000e6b8c
		mov dword ptr [esp + 0x14], edx		// 0x000e6b8f
		mov edx, dword ptr [ecx]		// 0x000e6b93
		mov ecx, dword ptr [eax + 0xc]		// 0x000e6b95
		mov dword ptr [esp + 0x18], edx		// 0x000e6b98
		mov edx, dword ptr [ecx]		// 0x000e6b9c
		mov ecx, dword ptr [eax + 0x10]		// 0x000e6b9e
		mov eax, dword ptr [eax + 0x14]		// 0x000e6ba1
		mov dword ptr [esp + 0x1c], edx		// 0x000e6ba4
		mov edx, dword ptr [ecx]		// 0x000e6ba8
		mov ecx, dword ptr [eax]		// 0x000e6baa
		mov eax, dword ptr [esp + 0x14]		// 0x000e6bac
		mov dword ptr [esp + 0x20], edx		// 0x000e6bb0
		mov edx, dword ptr [esp + 0x10]		// 0x000e6bb4
		mov dword ptr [esp + 0x28], edx		// 0x000e6bb8
		mov edx, dword ptr [esp + 0x1c]		// 0x000e6bbc
		mov dword ptr [esp + 0x24], ecx		// 0x000e6bc0
		mov ecx, dword ptr [esp + 0x18]		// 0x000e6bc4
		mov dword ptr [esp + 0x34], edx		// 0x000e6bc8
		mov dword ptr [esp + 0x2c], eax		// 0x000e6bcc
		mov eax, dword ptr [esp + 0x20]		// 0x000e6bd0
		mov dword ptr [esp + 0x30], ecx		// 0x000e6bd4
		mov ecx, dword ptr [esp + 0x24]		// 0x000e6bd8
		lea edx, [esp + 0x28]		// 0x000e6bdc
		push ebp		// 0x000e6be0
		push edx		// 0x000e6be1
		mov dword ptr [esp + 0x40], eax		// 0x000e6be2
		mov dword ptr [esp + 0x44], ecx		// 0x000e6be6
		call dword ptr [esp + 0x4c]		// 0x000e6bea
		mov eax, dword ptr [esi + 0x18]		// 0x000e6bee
		add esp, 8		// 0x000e6bf1
		inc edi		// 0x000e6bf4
		add ebx, 0x18		// 0x000e6bf5
		cmp edi, eax		// 0x000e6bf8
		jb Le6b78		// 0x000e6bfa
		pop ebp		// 0x000e6c00
		pop ebx		// 0x000e6c01
Le6c02:
		pop edi		// 0x000e6c02
Le6c03:
		pop esi		// 0x000e6c03
		add esp, 0x30		// 0x000e6c04
		ret 8		// 0x000e6c07
		}
	}

// phys_fn_005380 (0x000e9420, 19 B), 8 instructions
__declspec(naked) unsigned __fastcall nxP45380(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [ecx + 0x10]		// 0x000e9420
		add eax, 0x20		// 0x000e9423
		je Le9430		// 0x000e9426
		mov ecx, dword ptr [ecx + 0x10]		// 0x000e9428
		mov edx, dword ptr [ecx]		// 0x000e942b
		jmp dword ptr [edx + 0x10]		// 0x000e942d
Le9430:
		xor eax, eax		// 0x000e9430
		ret		// 0x000e9432
		}
	}

// phys_fn_005386 (0x000e94c0, 133 B), 52 instructions
__declspec(naked) unsigned __fastcall nxP45386(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000e94c0
		mov esi, ecx		// 0x000e94c1
		push edi		// 0x000e94c3
		mov edi, dword ptr [esi + 0xc]		// 0x000e94c4
		test edi, edi		// 0x000e94c7
		je Le94e6		// 0x000e94c9
		mov ecx, edi		// 0x000e94cb
		call nxP4Call1AABBTree		// 0x000e94cd
		call nxP4CallNxGetSdkAllocator		// 0x000e94d2
		mov edx, dword ptr [eax]		// 0x000e94d7
		push edi		// 0x000e94d9
		mov ecx, eax		// 0x000e94da
		call dword ptr [edx + 0xc]		// 0x000e94dc
		mov dword ptr [esi + 0xc], 0		// 0x000e94df
Le94e6:
		mov ecx, dword ptr [esi + 0x10]		// 0x000e94e6
		test ecx, ecx		// 0x000e94e9
		je Le94fa		// 0x000e94eb
		mov eax, dword ptr [ecx]		// 0x000e94ed
		push 1		// 0x000e94ef
		call dword ptr [eax]		// 0x000e94f1
		mov dword ptr [esi + 0x10], 0		// 0x000e94f3
Le94fa:
		mov edi, dword ptr [esp + 0xc]		// 0x000e94fa
		mov ecx, edi		// 0x000e94fe
		call nxP4CallReadDword		// 0x000e9500
		test al, 4		// 0x000e9505
		mov dword ptr [esi + 8], eax		// 0x000e9507
		je Le9513		// 0x000e950a
		pop edi		// 0x000e950c
		mov al, 1		// 0x000e950d
		pop esi		// 0x000e950f
		ret 4		// 0x000e9510
Le9513:
		xor ecx, ecx		// 0x000e9513
		mov cl, byte ptr [esi + 8]		// 0x000e9515
		shr eax, 1		// 0x000e9518
		and eax, 0xffffff01		// 0x000e951a
		and ecx, 0xffffff01		// 0x000e951f
		push ecx		// 0x000e9525
		push eax		// 0x000e9526
		mov ecx, esi		// 0x000e9527
		call nxP4CallCreateTree		// 0x000e9529
		test al, al		// 0x000e952e
		jne Le9537		// 0x000e9530
		pop edi		// 0x000e9532
		pop esi		// 0x000e9533
		ret 4		// 0x000e9534
Le9537:
		mov ecx, dword ptr [esi + 0x10]		// 0x000e9537
		mov edx, dword ptr [ecx]		// 0x000e953a
		push edi		// 0x000e953c
		call dword ptr [edx + 0x18]		// 0x000e953d
		pop edi		// 0x000e9540
		pop esi		// 0x000e9541
		ret 4		// 0x000e9542
		}
	}

// phys_fn_005404 (0x000e9b20, 94 B), 32 instructions
__declspec(naked) unsigned __fastcall nxP45404(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ecx		// 0x000e9b20
		fld dword ptr [ecx + 0x20]		// 0x000e9b21
		mov eax, dword ptr [ecx + 4]		// 0x000e9b24
		fld dword ptr [ecx + 0x1c]		// 0x000e9b27
		mov dword ptr [esp], eax		// 0x000e9b2a
		fld dword ptr [ecx + 0x18]		// 0x000e9b2d
		mov eax, dword ptr [esp + 8]		// 0x000e9b30
		fld dword ptr [ecx + 0x14]		// 0x000e9b34
		mov edx, dword ptr [esp]		// 0x000e9b37
		fld dword ptr [ecx + 0x10]		// 0x000e9b3a
		fld dword ptr [ecx + 0xc]		// 0x000e9b3d
		fld dword ptr [ecx + 8]		// 0x000e9b40
		mov ecx, dword ptr [ecx]		// 0x000e9b43
		fstp dword ptr [eax + 8]		// 0x000e9b45
		mov dword ptr [eax], ecx		// 0x000e9b48
		xor ecx, ecx		// 0x000e9b4a
		fstp dword ptr [eax + 0x10]		// 0x000e9b4c
		mov dword ptr [eax + 4], edx		// 0x000e9b4f
		mov dword ptr [eax + 0xc], ecx		// 0x000e9b52
		fstp dword ptr [eax + 0x14]		// 0x000e9b55
		mov dword ptr [eax + 0x1c], ecx		// 0x000e9b58
		mov dword ptr [eax + 0x2c], ecx		// 0x000e9b5b
		fstp dword ptr [eax + 0x18]		// 0x000e9b5e
		mov dword ptr [eax + 0x30], ecx		// 0x000e9b61
		mov dword ptr [eax + 0x34], ecx		// 0x000e9b64
		fstp dword ptr [eax + 0x20]		// 0x000e9b67
		mov dword ptr [eax + 0x38], ecx		// 0x000e9b6a
		mov dword ptr [eax + 0x3c], 0x3f800000		// 0x000e9b6d
		fstp dword ptr [eax + 0x24]		// 0x000e9b74
		fstp dword ptr [eax + 0x28]		// 0x000e9b77
		pop ecx		// 0x000e9b7a
		ret 4		// 0x000e9b7b
		}
	}

// phys_fn_005448 (0x000ef670, 29 B), 9 instructions
__declspec(naked) unsigned __fastcall nxP45448(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x000ef670
		mov esi, ecx		// 0x000ef671
		call nxP4Call0StaticPruner		// 0x000ef673
		lea ecx, [esi + 0x90]		// 0x000ef678
		mov dword ptr [esi], offset nxKnVtType3		// 0x000ef67e
		call nxP4Call0SdkContainer		// 0x000ef684
		mov eax, esi		// 0x000ef689
		pop esi		// 0x000ef68b
		ret		// 0x000ef68c
		}
	}

// phys_fn_005452 (0x000ef6d0, 117 B), 30 instructions
__declspec(naked) unsigned __fastcall nxP45452(void* self, void* edxUnused)
	{
	__asm
		{
		sub esp, 0x48		// 0x000ef6d0
		push esi		// 0x000ef6d3
		mov esi, ecx		// 0x000ef6d4
		mov eax, dword ptr [esi + 8]		// 0x000ef6d6
		add eax, dword ptr [esi + 4]		// 0x000ef6d9
		je Lef740		// 0x000ef6dc
		mov dword ptr [esp + 0x1c], eax		// 0x000ef6de
		mov eax, dword ptr [esi + 0x14]		// 0x000ef6e2
		xor ecx, ecx		// 0x000ef6e5
		mov dword ptr [esp + 0x48], eax		// 0x000ef6e7
		cmp dword ptr [esi + 0x3c], ecx		// 0x000ef6eb
		mov dword ptr [esp + 0x10], 0		// 0x000ef6ee
		mov dword ptr [esp + 0x14], 0xffffffff		// 0x000ef6f6
		mov dword ptr [esp + 0x18], 0		// 0x000ef6fe
		mov dword ptr [esp + 0x20], ecx		// 0x000ef706
		mov dword ptr [esp + 0x40], ecx		// 0x000ef70a
		mov dword ptr [esp + 0x44], ecx		// 0x000ef70e
		mov dword ptr [esp + 4], offset nxKnVtAABBsBuilder		// 0x000ef712
		mov dword ptr [esp + 0xc], 2		// 0x000ef71a
		mov dword ptr [esp + 8], 1		// 0x000ef722
		jne Lef733		// 0x000ef72a
		mov ecx, esi		// 0x000ef72c
		call nxP4CallBuildTree		// 0x000ef72e
Lef733:
		lea ecx, [esp + 4]		// 0x000ef733
		push ecx		// 0x000ef737
		mov ecx, dword ptr [esi + 0x3c]		// 0x000ef738
		call nxP4CallRefit2		// 0x000ef73b
Lef740:
		pop esi		// 0x000ef740
		add esp, 0x48		// 0x000ef741
		ret		// 0x000ef744
		}
	}

// phys_fn_005454 (0x000ef750, 53 B), 17 instructions
__declspec(naked) unsigned __fastcall nxP45454(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000ef750
		mov esi, ecx		// 0x000ef751
		lea ecx, [esi + 0x90]		// 0x000ef753
		mov dword ptr [esi], offset nxKnVtType3		// 0x000ef759
		call nxP4CallEmpty		// 0x000ef75f
		mov ecx, esi		// 0x000ef764
		call nxP4CallNxPCStaticDtorBody		// 0x000ef766
		test byte ptr [esp + 8], 1		// 0x000ef76b
		je Lef77f		// 0x000ef770
		call nxP4CallNxGetSdkAllocator		// 0x000ef772
		mov edx, dword ptr [eax]		// 0x000ef777
		push esi		// 0x000ef779
		mov ecx, eax		// 0x000ef77a
		call dword ptr [edx + 0xc]		// 0x000ef77c
Lef77f:
		mov eax, esi		// 0x000ef77f
		pop esi		// 0x000ef781
		ret 4		// 0x000ef782
		}
	}

// phys_fn_005456 (0x000ef790, 44 B), 17 instructions
__declspec(naked) unsigned __fastcall nxP45456(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4)
	{
	__asm
		{
		push esi		// 0x000ef790
		mov esi, ecx		// 0x000ef791
		call nxP45452		// 0x000ef793
		mov eax, dword ptr [esp + 0x18]		// 0x000ef798
		mov ecx, dword ptr [esp + 0x14]		// 0x000ef79c
		mov edx, dword ptr [esp + 0x10]		// 0x000ef7a0
		push eax		// 0x000ef7a4
		mov eax, dword ptr [esp + 0x10]		// 0x000ef7a5
		push ecx		// 0x000ef7a9
		mov ecx, dword ptr [esp + 0x10]		// 0x000ef7aa
		push edx		// 0x000ef7ae
		push eax		// 0x000ef7af
		push ecx		// 0x000ef7b0
		mov ecx, esi		// 0x000ef7b1
		call nxP4CallNxPCStaticPlanes		// 0x000ef7b3
		pop esi		// 0x000ef7b8
		ret 0x14		// 0x000ef7b9
		}
	}

// phys_fn_005458 (0x000ef7c0, 44 B), 17 instructions
__declspec(naked) unsigned __fastcall nxP45458(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4)
	{
	__asm
		{
		push esi		// 0x000ef7c0
		mov esi, ecx		// 0x000ef7c1
		call nxP45452		// 0x000ef7c3
		mov eax, dword ptr [esp + 0x18]		// 0x000ef7c8
		mov ecx, dword ptr [esp + 0x14]		// 0x000ef7cc
		mov edx, dword ptr [esp + 0x10]		// 0x000ef7d0
		push eax		// 0x000ef7d4
		mov eax, dword ptr [esp + 0x10]		// 0x000ef7d5
		push ecx		// 0x000ef7d9
		mov ecx, dword ptr [esp + 0x10]		// 0x000ef7da
		push edx		// 0x000ef7de
		push eax		// 0x000ef7df
		push ecx		// 0x000ef7e0
		mov ecx, esi		// 0x000ef7e1
		call nxP4CallRaycast		// 0x000ef7e3
		pop esi		// 0x000ef7e8
		ret 0x14		// 0x000ef7e9
		}
	}

// phys_fn_005460 (0x000ef7f0, 39 B), 15 instructions
__declspec(naked) unsigned __fastcall nxP45460(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		push esi		// 0x000ef7f0
		mov esi, ecx		// 0x000ef7f1
		call nxP45452		// 0x000ef7f3
		mov eax, dword ptr [esp + 0x14]		// 0x000ef7f8
		mov ecx, dword ptr [esp + 0x10]		// 0x000ef7fc
		mov edx, dword ptr [esp + 0xc]		// 0x000ef800
		push eax		// 0x000ef804
		mov eax, dword ptr [esp + 0xc]		// 0x000ef805
		push ecx		// 0x000ef809
		push edx		// 0x000ef80a
		push eax		// 0x000ef80b
		mov ecx, esi		// 0x000ef80c
		call nxP45229		// 0x000ef80e
		pop esi		// 0x000ef813
		ret 0x10		// 0x000ef814
		}
	}

// phys_fn_005462 (0x000ef820, 39 B), 15 instructions
__declspec(naked) unsigned __fastcall nxP45462(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		push esi		// 0x000ef820
		mov esi, ecx		// 0x000ef821
		call nxP45452		// 0x000ef823
		mov eax, dword ptr [esp + 0x14]		// 0x000ef828
		mov ecx, dword ptr [esp + 0x10]		// 0x000ef82c
		mov edx, dword ptr [esp + 0xc]		// 0x000ef830
		push eax		// 0x000ef834
		mov eax, dword ptr [esp + 0xc]		// 0x000ef835
		push ecx		// 0x000ef839
		push edx		// 0x000ef83a
		push eax		// 0x000ef83b
		mov ecx, esi		// 0x000ef83c
		call nxP45231		// 0x000ef83e
		pop esi		// 0x000ef843
		ret 0x10		// 0x000ef844
		}
	}

// phys_fn_005466 (0x000ef870, 536 B), 180 instructions
__declspec(naked) unsigned __fastcall nxP45466(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4)
	{
	__asm
		{
		sub esp, 0x3c		// 0x000ef870
		mov eax, dword ptr [esp + 0x4c]		// 0x000ef873
		push ebx		// 0x000ef877
		mov ebx, ecx		// 0x000ef878
		mov ecx, dword ptr [esp + 0x4c]		// 0x000ef87a
		push esi		// 0x000ef87e
		mov edx, eax		// 0x000ef87f
		and eax, 2		// 0x000ef881
		mov esi, 1		// 0x000ef884
		shl esi, cl		// 0x000ef889
		mov ecx, dword ptr [ebx + 0x18]		// 0x000ef88b
		and edx, 1		// 0x000ef88e
		mov dword ptr [esp + 0x10], eax		// 0x000ef891
		mov eax, dword ptr [ebx + 8]		// 0x000ef895
		push edi		// 0x000ef898
		mov edi, dword ptr [ebx + 4]		// 0x000ef899
		dec esi		// 0x000ef89c
		add eax, edi		// 0x000ef89d
		mov dword ptr [esp + 0xc], ebx		// 0x000ef89f
		mov dword ptr [esp + 0x10], edx		// 0x000ef8a3
		je Lefa7d		// 0x000ef8a7
		mov edi, dword ptr [esp + 0x4c]		// 0x000ef8ad
		push ebp		// 0x000ef8b1
		mov dword ptr [esp + 0x5c], eax		// 0x000ef8b2
		jmp Lef8c0		// 0x000ef8b6
Lef8b8:
		mov edx, dword ptr [esp + 0x14]		// 0x000ef8b8
		mov ecx, dword ptr [esp + 0x58]		// 0x000ef8bc
Lef8c0:
		mov ebp, dword ptr [ecx]		// 0x000ef8c0
		add ecx, 4		// 0x000ef8c2
		test edx, edx		// 0x000ef8c5
		mov dword ptr [esp + 0x58], ecx		// 0x000ef8c7
		jne Lef8d7		// 0x000ef8cb
		test byte ptr [ebp + 8], 1		// 0x000ef8cd
		jne Lefa72		// 0x000ef8d1
Lef8d7:
		mov ax, word ptr [ebp + 0x28]		// 0x000ef8d7
		cmp ax, 0xffff		// 0x000ef8db
		jne Lef8e5		// 0x000ef8df
		xor ecx, ecx		// 0x000ef8e1
		jmp Lef90c		// 0x000ef8e3
Lef8e5:
		test byte ptr [ebp + 8], 2		// 0x000ef8e5
		jne Lef8ff		// 0x000ef8e9
		mov ecx, dword ptr [ebx + 0x14]		// 0x000ef8eb
		movzx eax, ax		// 0x000ef8ee
		lea eax, [eax + eax*2]		// 0x000ef8f1
		lea edx, [ecx + eax*8]		// 0x000ef8f4
		push edx		// 0x000ef8f7
		mov ecx, ebp		// 0x000ef8f8
		call nxP4CallUpdateWorldAABB		// 0x000ef8fa
Lef8ff:
		movzx eax, word ptr [ebp + 0x28]		// 0x000ef8ff
		mov ecx, dword ptr [ebx + 0x14]		// 0x000ef903
		lea eax, [eax + eax*2]		// 0x000ef906
		lea ecx, [ecx + eax*8]		// 0x000ef909
Lef90c:
		fld dword ptr [ecx]		// 0x000ef90c
		lea eax, [ecx + 0xc]		// 0x000ef90e
		fadd dword ptr [eax]		// 0x000ef911
		xor ebx, ebx		// 0x000ef913
		fld dword ptr [eax + 4]		// 0x000ef915
		fadd dword ptr [ecx + 4]		// 0x000ef918
		fld dword ptr [eax + 8]		// 0x000ef91b
		fadd dword ptr [ecx + 8]		// 0x000ef91e
		fstp dword ptr [esp + 0x48]		// 0x000ef921
		fxch st(1)		// 0x000ef925
		fmul dword ptr [nxP4K101043cc_4]		// 0x000ef927
		fstp dword ptr [esp + 0x34]		// 0x000ef92d
		mov edx, dword ptr [esp + 0x34]		// 0x000ef931
		mov dword ptr [esp + 0x28], edx		// 0x000ef935
		fmul dword ptr [nxP4K101043cc_4]		// 0x000ef939
		fstp dword ptr [esp + 0x38]		// 0x000ef93f
		mov ecx, dword ptr [esp + 0x38]		// 0x000ef943
		fld dword ptr [esp + 0x48]		// 0x000ef947
		mov dword ptr [esp + 0x2c], ecx		// 0x000ef94b
		fmul dword ptr [nxP4K101043cc_4]		// 0x000ef94f
		mov ecx, dword ptr [eax]		// 0x000ef955
		mov dword ptr [esp + 0x1c], ecx		// 0x000ef957
		mov ecx, 1		// 0x000ef95b
		cmp esi, ecx		// 0x000ef960
		fstp dword ptr [esp + 0x3c]		// 0x000ef962
		fld dword ptr [esp + 0x1c]		// 0x000ef966
		mov edx, dword ptr [esp + 0x3c]		// 0x000ef96a
		fsub dword ptr [esp + 0x28]		// 0x000ef96e
		mov dword ptr [esp + 0x30], edx		// 0x000ef972
		mov edx, dword ptr [eax + 4]		// 0x000ef976
		mov eax, dword ptr [eax + 8]		// 0x000ef979
		fstp dword ptr [esp + 0x1c]		// 0x000ef97c
		mov dword ptr [esp + 0x20], edx		// 0x000ef980
		fld dword ptr [esp + 0x20]		// 0x000ef984
		mov dword ptr [esp + 0x24], eax		// 0x000ef988
		fsub dword ptr [esp + 0x2c]		// 0x000ef98c
		fstp dword ptr [esp + 0x20]		// 0x000ef990
		fld dword ptr [esp + 0x24]		// 0x000ef994
		fsub dword ptr [esp + 0x30]		// 0x000ef998
		fstp dword ptr [esp + 0x24]		// 0x000ef99c
		jb Lefa1a		// 0x000ef9a0
		mov edx, dword ptr [esp + 0x54]		// 0x000ef9a2
		add edx, 4		// 0x000ef9a6
		lea esp, [esp]		// 0x000ef9a9
Lef9b0:
		test esi, ecx		// 0x000ef9b0
		je Lefa11		// 0x000ef9b2
		fld dword ptr [edx]		// 0x000ef9b4
		fabs		// 0x000ef9b6
		fmul dword ptr [esp + 0x20]		// 0x000ef9b8
		fld dword ptr [edx + 4]		// 0x000ef9bc
		fabs		// 0x000ef9bf
		fxch st(1)		// 0x000ef9c1
		fxch st(1)		// 0x000ef9c3
		fmul dword ptr [esp + 0x24]		// 0x000ef9c5
		faddp st(1), st(0)		// 0x000ef9c9
		fld dword ptr [edx - 4]		// 0x000ef9cb
		fabs		// 0x000ef9ce
		fst dword ptr [esp + 0x50]		// 0x000ef9d0
		fmul dword ptr [esp + 0x1c]		// 0x000ef9d4
		faddp st(1), st(0)		// 0x000ef9d8
		fld dword ptr [esp + 0x2c]		// 0x000ef9da
		fmul dword ptr [edx]		// 0x000ef9de
		fld dword ptr [esp + 0x30]		// 0x000ef9e0
		fmul dword ptr [edx + 4]		// 0x000ef9e4
		faddp st(1), st(0)		// 0x000ef9e7
		fld dword ptr [esp + 0x28]		// 0x000ef9e9
		fmul dword ptr [edx - 4]		// 0x000ef9ed
		faddp st(1), st(0)		// 0x000ef9f0
		fadd dword ptr [edx + 8]		// 0x000ef9f2
		fld st(1)		// 0x000ef9f5
		fcomp st(1)		// 0x000ef9f7
		fnstsw ax		// 0x000ef9f9
		test ah, 5		// 0x000ef9fb
		jnp Lefa6a		// 0x000ef9fe
		fxch st(1)		// 0x000efa00
		fchs		// 0x000efa02
		fcomp st(1)		// 0x000efa04
		fnstsw ax		// 0x000efa06
		fstp st(0)		// 0x000efa08
		test ah, 5		// 0x000efa0a
		jp Lefa11		// 0x000efa0d
		or ebx, ecx		// 0x000efa0f
Lefa11:
		add ecx, ecx		// 0x000efa11
		add edx, 0x10		// 0x000efa13
		cmp ecx, esi		// 0x000efa16
		jbe Lef9b0		// 0x000efa18
Lefa1a:
		mov eax, dword ptr [esp + 0x18]		// 0x000efa1a
		test eax, eax		// 0x000efa1e
		mov eax, dword ptr [edi]		// 0x000efa20
		je Lefa4c		// 0x000efa22
		mov ecx, dword ptr [edi + 4]		// 0x000efa24
		test ebx, ebx		// 0x000efa27
		setne bl		// 0x000efa29
		cmp ecx, eax		// 0x000efa2c
		jne Lefa39		// 0x000efa2e
		push 1		// 0x000efa30
		mov ecx, edi		// 0x000efa32
		call nxP4CallResize		// 0x000efa34
Lefa39:
		mov eax, dword ptr [edi + 4]		// 0x000efa39
		mov ecx, dword ptr [edi + 8]		// 0x000efa3c
		movzx edx, bl		// 0x000efa3f
		or edx, ebp		// 0x000efa42
		mov dword ptr [ecx + eax*4], edx		// 0x000efa44
		inc dword ptr [edi + 4]		// 0x000efa47
		jmp Lefa6e		// 0x000efa4a
Lefa4c:
		mov edx, dword ptr [edi + 4]		// 0x000efa4c
		cmp edx, eax		// 0x000efa4f
		jne Lefa5c		// 0x000efa51
		push 1		// 0x000efa53
		mov ecx, edi		// 0x000efa55
		call nxP4CallResize		// 0x000efa57
Lefa5c:
		mov eax, dword ptr [edi + 4]		// 0x000efa5c
		mov ecx, dword ptr [edi + 8]		// 0x000efa5f
		mov dword ptr [ecx + eax*4], ebp		// 0x000efa62
		inc dword ptr [edi + 4]		// 0x000efa65
		jmp Lefa6e		// 0x000efa68
Lefa6a:
		fstp st(0)		// 0x000efa6a
		fstp st(0)		// 0x000efa6c
Lefa6e:
		mov ebx, dword ptr [esp + 0x10]		// 0x000efa6e
Lefa72:
		dec dword ptr [esp + 0x5c]		// 0x000efa72
		jne Lef8b8		// 0x000efa76
		pop ebp		// 0x000efa7c
Lefa7d:
		pop edi		// 0x000efa7d
		pop esi		// 0x000efa7e
		mov al, 1		// 0x000efa7f
		pop ebx		// 0x000efa81
		add esp, 0x3c		// 0x000efa82
		ret 0x14		// 0x000efa85
		}
	}

// phys_fn_005470 (0x000efc90, 208 B), 77 instructions
__declspec(naked) unsigned __fastcall nxP45470(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		push ecx		// 0x000efc90
		push ebx		// 0x000efc91
		push ebp		// 0x000efc92
		mov ebp, ecx		// 0x000efc93
		mov eax, dword ptr [ebp + 8]		// 0x000efc95
		add eax, dword ptr [ebp + 4]		// 0x000efc98
		mov ebx, dword ptr [ebp + 0x18]		// 0x000efc9b
		push esi		// 0x000efc9e
		push edi		// 0x000efc9f
		je Lefd56		// 0x000efca0
		mov edi, dword ptr [esp + 0x18]		// 0x000efca6
		lea ebx, [ebx]		// 0x000efcaa
Lefcb0:
		mov esi, dword ptr [ebx]		// 0x000efcb0
		mov ecx, dword ptr [esp + 0x24]		// 0x000efcb2
		mov edx, dword ptr [esi + 0x24]		// 0x000efcb6
		dec eax		// 0x000efcb9
		test ecx, edx		// 0x000efcba
		mov dword ptr [esp + 0x18], eax		// 0x000efcbc
		je Lefd4b		// 0x000efcc0
		mov ax, word ptr [esi + 0x28]		// 0x000efcc6
		cmp ax, 0xffff		// 0x000efcca
		jne Lefcd4		// 0x000efcce
		xor eax, eax		// 0x000efcd0
		jmp Lefcfb		// 0x000efcd2
Lefcd4:
		test byte ptr [esi + 8], 2		// 0x000efcd4
		jne Lefcee		// 0x000efcd8
		movzx eax, ax		// 0x000efcda
		lea edx, [eax + eax*2]		// 0x000efcdd
		mov eax, dword ptr [ebp + 0x14]		// 0x000efce0
		lea ecx, [eax + edx*8]		// 0x000efce3
		push ecx		// 0x000efce6
		mov ecx, esi		// 0x000efce7
		call nxP4CallUpdateWorldAABB		// 0x000efce9
Lefcee:
		movzx eax, word ptr [esi + 0x28]		// 0x000efcee
		lea edx, [eax + eax*2]		// 0x000efcf2
		mov eax, dword ptr [ebp + 0x14]		// 0x000efcf5
		lea eax, [eax + edx*8]		// 0x000efcf8
Lefcfb:
		mov ecx, dword ptr [esp + 0x1c]		// 0x000efcfb
		mov edx, dword ptr [ecx + 0xc]		// 0x000efcff
		mov dword ptr [esp + 0x10], edx		// 0x000efd02
		lea edx, [eax + 0xc]		// 0x000efd06
		push edx		// 0x000efd09
		push eax		// 0x000efd0a
		mov eax, dword ptr [esp + 0x18]		// 0x000efd0b
		push eax		// 0x000efd0f
		push ecx		// 0x000efd10
		call nxP4CallNxPQSphereAABB		// 0x000efd11
		add esp, 0x10		// 0x000efd16
		test al, al		// 0x000efd19
		je Lefd47		// 0x000efd1b
		mov ecx, dword ptr [edi + 4]		// 0x000efd1d
		cmp ecx, dword ptr [edi]		// 0x000efd20
		mov esi, dword ptr [ebx]		// 0x000efd22
		jne Lefd2f		// 0x000efd24
		push 1		// 0x000efd26
		mov ecx, edi		// 0x000efd28
		call nxP4CallResize		// 0x000efd2a
Lefd2f:
		mov edx, dword ptr [edi + 4]		// 0x000efd2f
		mov eax, dword ptr [edi + 8]		// 0x000efd32
		mov dword ptr [eax + edx*4], esi		// 0x000efd35
		mov ecx, dword ptr [edi + 4]		// 0x000efd38
		mov al, byte ptr [esp + 0x20]		// 0x000efd3b
		inc ecx		// 0x000efd3f
		test al, al		// 0x000efd40
		mov dword ptr [edi + 4], ecx		// 0x000efd42
		jne Lefd56		// 0x000efd45
Lefd47:
		mov eax, dword ptr [esp + 0x18]		// 0x000efd47
Lefd4b:
		add ebx, 4		// 0x000efd4b
		test eax, eax		// 0x000efd4e
		jne Lefcb0		// 0x000efd50
Lefd56:
		pop edi		// 0x000efd56
		pop esi		// 0x000efd57
		pop ebp		// 0x000efd58
		mov al, 1		// 0x000efd59
		pop ebx		// 0x000efd5b
		pop ecx		// 0x000efd5c
		ret 0x10		// 0x000efd5d
		}
	}

// phys_fn_005471 (0x000efd60, 273 B), 100 instructions
__declspec(naked) unsigned __fastcall nxP45471(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		push ecx		// 0x000efd60
		push ebx		// 0x000efd61
		mov ebx, ecx		// 0x000efd62
		mov eax, dword ptr [ebx + 0x18]		// 0x000efd64
		mov edx, dword ptr [ebx + 8]		// 0x000efd67
		push ebp		// 0x000efd6a
		mov dword ptr [esp + 8], eax		// 0x000efd6b
		add edx, dword ptr [ebx + 4]		// 0x000efd6f
		push esi		// 0x000efd72
		push edi		// 0x000efd73
		je Lefe67		// 0x000efd74
		mov ebp, dword ptr [esp + 0x1c]		// 0x000efd7a
		mov edi, dword ptr [esp + 0x18]		// 0x000efd7e
Lefd82:
		mov ecx, dword ptr [esp + 0x10]		// 0x000efd82
		mov esi, dword ptr [ecx]		// 0x000efd86
		mov eax, dword ptr [esp + 0x24]		// 0x000efd88
		mov ecx, dword ptr [esi + 0x24]		// 0x000efd8c
		dec edx		// 0x000efd8f
		test eax, ecx		// 0x000efd90
		mov dword ptr [esp + 0x1c], edx		// 0x000efd92
		je Lefe54		// 0x000efd96
		mov ax, word ptr [esi + 0x28]		// 0x000efd9c
		cmp ax, 0xffff		// 0x000efda0
		jne Lefdaa		// 0x000efda4
		xor ecx, ecx		// 0x000efda6
		jmp Lefdd5		// 0x000efda8
Lefdaa:
		test byte ptr [esi + 8], 2		// 0x000efdaa
		jne Lefdc8		// 0x000efdae
		mov edx, dword ptr [ebx + 0x14]		// 0x000efdb0
		movzx eax, ax		// 0x000efdb3
		lea ecx, [eax + eax*2]		// 0x000efdb6
		lea eax, [edx + ecx*8]		// 0x000efdb9
		push eax		// 0x000efdbc
		mov ecx, esi		// 0x000efdbd
		call nxP4CallUpdateWorldAABB		// 0x000efdbf
		mov edx, dword ptr [esp + 0x1c]		// 0x000efdc4
Lefdc8:
		movzx eax, word ptr [esi + 0x28]		// 0x000efdc8
		lea ecx, [eax + eax*2]		// 0x000efdcc
		mov eax, dword ptr [ebx + 0x14]		// 0x000efdcf
		lea ecx, [eax + ecx*8]		// 0x000efdd2
Lefdd5:
		fld dword ptr [ebp + 0xc]		// 0x000efdd5
		fcomp dword ptr [ecx]		// 0x000efdd8
		fnstsw ax		// 0x000efdda
		test ah, 5		// 0x000efddc
		jnp Lefe54		// 0x000efddf
		fld dword ptr [ecx + 0xc]		// 0x000efde1
		fcomp dword ptr [ebp]		// 0x000efde4
		fnstsw ax		// 0x000efde7
		test ah, 5		// 0x000efde9
		jnp Lefe54		// 0x000efdec
		fld dword ptr [ebp + 0x10]		// 0x000efdee
		fcomp dword ptr [ecx + 4]		// 0x000efdf1
		fnstsw ax		// 0x000efdf4
		test ah, 5		// 0x000efdf6
		jnp Lefe54		// 0x000efdf9
		fld dword ptr [ecx + 0x10]		// 0x000efdfb
		fcomp dword ptr [ebp + 4]		// 0x000efdfe
		fnstsw ax		// 0x000efe01
		test ah, 5		// 0x000efe03
		jnp Lefe54		// 0x000efe06
		fld dword ptr [ebp + 0x14]		// 0x000efe08
		fcomp dword ptr [ecx + 8]		// 0x000efe0b
		fnstsw ax		// 0x000efe0e
		test ah, 5		// 0x000efe10
		jnp Lefe54		// 0x000efe13
		fld dword ptr [ecx + 0x14]		// 0x000efe15
		fcomp dword ptr [ebp + 8]		// 0x000efe18
		fnstsw ax		// 0x000efe1b
		test ah, 5		// 0x000efe1d
		jnp Lefe54		// 0x000efe20
		mov ecx, dword ptr [esp + 0x10]		// 0x000efe22
		mov esi, dword ptr [ecx]		// 0x000efe26
		mov eax, dword ptr [edi + 4]		// 0x000efe28
		cmp eax, dword ptr [edi]		// 0x000efe2b
		jne Lefe3c		// 0x000efe2d
		push 1		// 0x000efe2f
		mov ecx, edi		// 0x000efe31
		call nxP4CallResize		// 0x000efe33
		mov edx, dword ptr [esp + 0x1c]		// 0x000efe38
Lefe3c:
		mov ecx, dword ptr [edi + 4]		// 0x000efe3c
		mov eax, dword ptr [edi + 8]		// 0x000efe3f
		mov dword ptr [eax + ecx*4], esi		// 0x000efe42
		mov ecx, dword ptr [edi + 4]		// 0x000efe45
		mov al, byte ptr [esp + 0x20]		// 0x000efe48
		inc ecx		// 0x000efe4c
		test al, al		// 0x000efe4d
		mov dword ptr [edi + 4], ecx		// 0x000efe4f
		jne Lefe67		// 0x000efe52
Lefe54:
		mov ecx, dword ptr [esp + 0x10]		// 0x000efe54
		add ecx, 4		// 0x000efe58
		test edx, edx		// 0x000efe5b
		mov dword ptr [esp + 0x10], ecx		// 0x000efe5d
		jne Lefd82		// 0x000efe61
Lefe67:
		pop edi		// 0x000efe67
		pop esi		// 0x000efe68
		pop ebp		// 0x000efe69
		mov al, 1		// 0x000efe6a
		pop ebx		// 0x000efe6c
		pop ecx		// 0x000efe6d
		ret 0x10		// 0x000efe6e
		}
	}

// phys_fn_005473 (0x000efe80, 40 B), 14 instructions
__declspec(naked) unsigned __fastcall nxP45473(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000efe80
		mov esi, ecx		// 0x000efe81
		mov dword ptr [esi], offset nxKnVtDynamic		// 0x000efe83
		call nxP4Call1Pruner		// 0x000efe89
		test byte ptr [esp + 8], 1		// 0x000efe8e
		je Lefea2		// 0x000efe93
		call nxP4CallNxGetSdkAllocator		// 0x000efe95
		mov edx, dword ptr [eax]		// 0x000efe9a
		push esi		// 0x000efe9c
		mov ecx, eax		// 0x000efe9d
		call dword ptr [edx + 0xc]		// 0x000efe9f
Lefea2:
		mov eax, esi		// 0x000efea2
		pop esi		// 0x000efea4
		ret 4		// 0x000efea5
		}
	}

// helper: phys_fn_005477 (record unchanged) (0x000efed0, 65 B), 24 instructions
__declspec(naked) unsigned __fastcall nxP45477(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x000efed0
		mov esi, ecx		// 0x000efed1
		mov eax, dword ptr [esi + 0x10]		// 0x000efed3
		test eax, eax		// 0x000efed6
		je Lefef1		// 0x000efed8
		call nxP4CallNxGetSdkAllocator		// 0x000efeda
		mov ecx, dword ptr [esi + 0x10]		// 0x000efedf
		mov edx, dword ptr [eax]		// 0x000efee2
		push ecx		// 0x000efee4
		mov ecx, eax		// 0x000efee5
		call dword ptr [edx + 0xc]		// 0x000efee7
		mov dword ptr [esi + 0x10], 0		// 0x000efeea
Lefef1:
		mov eax, dword ptr [esi + 0x14]		// 0x000efef1
		test eax, eax		// 0x000efef4
		je Leff0f		// 0x000efef6
		call nxP4CallNxGetSdkAllocator		// 0x000efef8
		mov ecx, dword ptr [esi + 0x14]		// 0x000efefd
		mov edx, dword ptr [eax]		// 0x000eff00
		push ecx		// 0x000eff02
		mov ecx, eax		// 0x000eff03
		call dword ptr [edx + 0xc]		// 0x000eff05
		mov dword ptr [esi + 0x14], 0		// 0x000eff08
Leff0f:
		pop esi		// 0x000eff0f
		ret		// 0x000eff10
		}
	}

// phys_fn_005479 (0x000eff20, 148 B), 56 instructions
__declspec(naked) unsigned __fastcall nxP45479(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000eff20
		push edi		// 0x000eff21
		mov edi, dword ptr [esp + 0xc]		// 0x000eff22
		test edi, edi		// 0x000eff26
		mov esi, ecx		// 0x000eff28
		jne Leff33		// 0x000eff2a
Leff2c:
		pop edi		// 0x000eff2c
		xor al, al		// 0x000eff2d
		pop esi		// 0x000eff2f
		ret 4		// 0x000eff30
Leff33:
		mov eax, dword ptr [esi + 0x10]		// 0x000eff33
		test eax, eax		// 0x000eff36
		je Leff51		// 0x000eff38
		call nxP4CallNxGetSdkAllocator		// 0x000eff3a
		mov ecx, dword ptr [esi + 0x10]		// 0x000eff3f
		mov edx, dword ptr [eax]		// 0x000eff42
		push ecx		// 0x000eff44
		mov ecx, eax		// 0x000eff45
		call dword ptr [edx + 0xc]		// 0x000eff47
		mov dword ptr [esi + 0x10], 0		// 0x000eff4a
Leff51:
		mov eax, dword ptr [esi + 0x14]		// 0x000eff51
		test eax, eax		// 0x000eff54
		je Leff6f		// 0x000eff56
		call nxP4CallNxGetSdkAllocator		// 0x000eff58
		mov ecx, dword ptr [esi + 0x14]		// 0x000eff5d
		mov edx, dword ptr [eax]		// 0x000eff60
		push ecx		// 0x000eff62
		mov ecx, eax		// 0x000eff63
		call dword ptr [edx + 0xc]		// 0x000eff65
		mov dword ptr [esi + 0x14], 0		// 0x000eff68
Leff6f:
		mov word ptr [esi + 0xc], di		// 0x000eff6f
		mov word ptr [esi + 0xe], di		// 0x000eff73
		call nxP4CallNxGetSdkAllocator		// 0x000eff77
		mov edx, dword ptr [eax]		// 0x000eff7c
		lea ecx, [edi + edi*2]		// 0x000eff7e
		shl ecx, 3		// 0x000eff81
		push 0		// 0x000eff84
		push ecx		// 0x000eff86
		mov ecx, eax		// 0x000eff87
		call dword ptr [edx]		// 0x000eff89
		test eax, eax		// 0x000eff8b
		mov dword ptr [esi + 0x10], eax		// 0x000eff8d
		je Leff2c		// 0x000eff90
		call nxP4CallNxGetSdkAllocator		// 0x000eff92
		mov edx, dword ptr [eax]		// 0x000eff97
		push 0		// 0x000eff99
		lea ecx, [edi*4]		// 0x000eff9b
		push ecx		// 0x000effa2
		mov ecx, eax		// 0x000effa3
		call dword ptr [edx]		// 0x000effa5
		mov dword ptr [esi + 0x14], eax		// 0x000effa7
		test eax, eax		// 0x000effaa
		pop edi		// 0x000effac
		setne al		// 0x000effad
		pop esi		// 0x000effb0
		ret 4		// 0x000effb1
		}
	}

// phys_fn_005539 (0x000f15c0, 51 B), 17 instructions
__declspec(naked) unsigned __fastcall nxP45539(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000f15c0
		mov esi, ecx		// 0x000f15c1
		lea ecx, [esi + 0x34]		// 0x000f15c3
		mov dword ptr [esi], offset nxKnVtPruner		// 0x000f15c6
		call nxP44832		// 0x000f15cc
		lea ecx, [esi + 4]		// 0x000f15d1
		call nxP45477		// 0x000f15d4
		test byte ptr [esp + 8], 1		// 0x000f15d9
		je Lf15ed		// 0x000f15de
		call nxP4CallNxGetSdkAllocator		// 0x000f15e0
		mov edx, dword ptr [eax]		// 0x000f15e5
		push esi		// 0x000f15e7
		mov ecx, eax		// 0x000f15e8
		call dword ptr [edx + 0xc]		// 0x000f15ea
Lf15ed:
		mov eax, esi		// 0x000f15ed
		pop esi		// 0x000f15ef
		ret 4		// 0x000f15f0
		}
	}

// phys_fn_005566 (0x000f2090, 199 B), 80 instructions
__declspec(naked) unsigned __cdecl nxP45566()
	{
	__asm
		{
		push ebx		// 0x000f2090
		mov ebx, dword ptr [esp + 8]		// 0x000f2091
		push edi		// 0x000f2095
		mov edi, dword ptr [esp + 0x10]		// 0x000f2096
		cmp ebx, dword ptr [edi]		// 0x000f209a
		jae Lf2154		// 0x000f209c
		mov ecx, dword ptr [edi + 4]		// 0x000f20a2
		push esi		// 0x000f20a5
		mov esi, ebx		// 0x000f20a6
		shl esi, 5		// 0x000f20a8
		mov eax, dword ptr [esi + ecx + 0x18]		// 0x000f20ab
		add esi, ecx		// 0x000f20af
		test eax, eax		// 0x000f20b1
		je Lf2153		// 0x000f20b3
		mov ecx, dword ptr [edi + 0x10]		// 0x000f20b9
		fld dword ptr [ecx + 0xc]		// 0x000f20bc
		fcomp dword ptr [esi]		// 0x000f20bf
		fnstsw ax		// 0x000f20c1
		test ah, 5		// 0x000f20c3
		jnp Lf2153		// 0x000f20c6
		fld dword ptr [esi + 0xc]		// 0x000f20cc
		fcomp dword ptr [ecx]		// 0x000f20cf
		fnstsw ax		// 0x000f20d1
		test ah, 5		// 0x000f20d3
		jnp Lf2153		// 0x000f20d6
		fld dword ptr [ecx + 0x10]		// 0x000f20d8
		fcomp dword ptr [esi + 4]		// 0x000f20db
		fnstsw ax		// 0x000f20de
		test ah, 5		// 0x000f20e0
		jnp Lf2153		// 0x000f20e3
		fld dword ptr [esi + 0x10]		// 0x000f20e5
		fcomp dword ptr [ecx + 4]		// 0x000f20e8
		fnstsw ax		// 0x000f20eb
		test ah, 5		// 0x000f20ed
		jnp Lf2153		// 0x000f20f0
		fld dword ptr [ecx + 0x14]		// 0x000f20f2
		fcomp dword ptr [esi + 8]		// 0x000f20f5
		fnstsw ax		// 0x000f20f8
		test ah, 5		// 0x000f20fa
		jnp Lf2153		// 0x000f20fd
		fld dword ptr [esi + 0x14]		// 0x000f20ff
		fcomp dword ptr [ecx + 8]		// 0x000f2102
		fnstsw ax		// 0x000f2105
		test ah, 5		// 0x000f2107
		jnp Lf2153		// 0x000f210a
		push ecx		// 0x000f210c
		mov ecx, esi		// 0x000f210d
		call nxP4CallNxPOIsInside		// 0x000f210f
		test al, al		// 0x000f2114
		je Lf2130		// 0x000f2116
		mov eax, dword ptr [edi + 0xc]		// 0x000f2118
		mov ecx, dword ptr [edi + 4]		// 0x000f211b
		mov edx, dword ptr [edi]		// 0x000f211e
		push eax		// 0x000f2120
		push ecx		// 0x000f2121
		push edx		// 0x000f2122
		push ebx		// 0x000f2123
		call nxP4CallNxPQAppendSubtree		// 0x000f2124
		add esp, 0x10		// 0x000f2129
		pop esi		// 0x000f212c
		pop edi		// 0x000f212d
		pop ebx		// 0x000f212e
		ret		// 0x000f212f
Lf2130:
		mov eax, dword ptr [edi + 8]		// 0x000f2130
		push eax		// 0x000f2133
		mov ecx, esi		// 0x000f2134
		call nxP4CallNxPQAppendNode		// 0x000f2136
		mov esi, 1		// 0x000f213b
Lf2140:
		lea ecx, [esi + ebx*8]		// 0x000f2140
		push edi		// 0x000f2143
		push ecx		// 0x000f2144
		call nxP45566		// 0x000f2145
		add esp, 8		// 0x000f214a
		inc esi		// 0x000f214d
		cmp esi, 8		// 0x000f214e
		jbe Lf2140		// 0x000f2151
Lf2153:
		pop esi		// 0x000f2153
Lf2154:
		pop edi		// 0x000f2154
		pop ebx		// 0x000f2155
		ret		// 0x000f2156
		}
	}

// phys_fn_005568 (0x000f2160, 57 B), 17 instructions
__declspec(naked) unsigned __fastcall nxP45568(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2)
	{
	__asm
		{
		sub esp, 0x14		// 0x000f2160
		mov eax, dword ptr [ecx + 0x48]		// 0x000f2163
		mov edx, dword ptr [esp + 0x1c]		// 0x000f2166
		mov ecx, dword ptr [ecx + 0x4c]		// 0x000f216a
		mov dword ptr [esp + 8], edx		// 0x000f216d
		lea edx, [esp]		// 0x000f2171
		mov dword ptr [esp], eax		// 0x000f2174
		mov eax, dword ptr [esp + 0x20]		// 0x000f2177
		mov dword ptr [esp + 4], ecx		// 0x000f217b
		mov ecx, dword ptr [esp + 0x18]		// 0x000f217f
		push edx		// 0x000f2183
		push 0		// 0x000f2184
		mov dword ptr [esp + 0x14], eax		// 0x000f2186
		mov dword ptr [esp + 0x18], ecx		// 0x000f218a
		call nxP45566		// 0x000f218e
		add esp, 0x1c		// 0x000f2193
		ret 0xc		// 0x000f2196
		}
	}

// phys_fn_005586 (0x000f2580, 43 B), 16 instructions
__declspec(naked) unsigned __fastcall nxP45586(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [ecx + 4]		// 0x000f2580
		mov ecx, dword ptr [ecx + 8]		// 0x000f2583
		mov edx, eax		// 0x000f2586
		imul edx, edx, 0x1c		// 0x000f2588
		push edx		// 0x000f258b
		push ecx		// 0x000f258c
		push ecx		// 0x000f258d
		mov ecx, dword ptr [esp + 0x10]		// 0x000f258e
		push eax		// 0x000f2592
		call nxP4CallStoreDword		// 0x000f2593
		mov ecx, eax		// 0x000f2598
		call nxP4CallStoreDword		// 0x000f259a
		mov ecx, eax		// 0x000f259f
		call nxP44799		// 0x000f25a1
		mov al, 1		// 0x000f25a6
		ret 4		// 0x000f25a8
		}
	}

// phys_fn_005588 and phys_fn_005590 (one function) (0x000f25b0, 212 B), 86 instructions
__declspec(naked) unsigned __fastcall nxP45588(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ebx		// 0x000f25b0
		push esi		// 0x000f25b1
		push edi		// 0x000f25b2
		mov edi, dword ptr [esp + 0x10]		// 0x000f25b3
		mov ebx, ecx		// 0x000f25b7
		mov ecx, edi		// 0x000f25b9
		call nxP4CallReadDword		// 0x000f25bb
		mov esi, dword ptr [ebx + 8]		// 0x000f25c0
		mov dword ptr [ebx + 4], eax		// 0x000f25c3
		imul eax, eax, 0x1c		// 0x000f25c6
		test esi, esi		// 0x000f25c9
		mov dword ptr [esp + 0x10], eax		// 0x000f25cb
		je Lf25e8		// 0x000f25cf
		call nxP4CallNxGetSdkAllocator		// 0x000f25d1
		mov edx, dword ptr [eax]		// 0x000f25d6
		add esi, -4		// 0x000f25d8
		push esi		// 0x000f25db
		mov ecx, eax		// 0x000f25dc
		call dword ptr [edx + 0xc]		// 0x000f25de
		mov dword ptr [ebx + 8], 0		// 0x000f25e1
Lf25e8:
		mov esi, dword ptr [ebx + 4]		// 0x000f25e8
		call nxP4CallNxGetSdkAllocator		// 0x000f25eb
		mov edx, dword ptr [eax]		// 0x000f25f0
		mov ecx, esi		// 0x000f25f2
		imul ecx, ecx, 0x1c		// 0x000f25f4
		add ecx, 4		// 0x000f25f7
		push 0		// 0x000f25fa
		push ecx		// 0x000f25fc
		mov ecx, eax		// 0x000f25fd
		call dword ptr [edx]		// 0x000f25ff
		test eax, eax		// 0x000f2601
		je Lf260c		// 0x000f2603
		mov dword ptr [eax], esi		// 0x000f2605
		add eax, 4		// 0x000f2607
		jmp Lf260e		// 0x000f260a
Lf260c:
		xor eax, eax		// 0x000f260c
Lf260e:
		test eax, eax		// 0x000f260e
		mov dword ptr [ebx + 8], eax		// 0x000f2610
		jne Lf261d		// 0x000f2613
		pop edi		// 0x000f2615
		pop esi		// 0x000f2616
		xor al, al		// 0x000f2617
		pop ebx		// 0x000f2619
		ret 4		// 0x000f261a
Lf261d:
		push ebp		// 0x000f261d
		mov ecx, edi		// 0x000f261e
		call nxP4CallReadDword		// 0x000f2620
		mov ebp, dword ptr [ebx + 8]		// 0x000f2625
		mov esi, dword ptr [esp + 0x14]		// 0x000f2628
		push esi		// 0x000f262c
		mov ecx, edi		// 0x000f262d
		sub ebp, eax		// 0x000f262f
		call nxP44778		// 0x000f2631
		mov edi, dword ptr [ebx + 8]		// 0x000f2636
		mov ecx, esi		// 0x000f2639
		mov edx, ecx		// 0x000f263b
		shr ecx, 2		// 0x000f263d
		mov esi, eax		// 0x000f2640
		rep movsd		// 0x000f2642
		mov ecx, edx		// 0x000f2644
		and ecx, 3		// 0x000f2646
		rep movsb		// 0x000f2649
		mov eax, dword ptr [ebx + 4]		// 0x000f264b
		xor ecx, ecx		// 0x000f264e
		test eax, eax		// 0x000f2650
		jbe Lf267b		// 0x000f2652
		xor edx, edx		// 0x000f2654
		jmp Lf2660		// 0x000f2656
		lea esp, [esp]		// 0x000f2658
		nop		// 0x000f265f
Lf2660:
		mov eax, dword ptr [ebx + 8]		// 0x000f2660
		test byte ptr [edx + eax + 0x18], 1		// 0x000f2663
		lea eax, [edx + eax + 0x18]		// 0x000f2668
		jne Lf2670		// 0x000f266c
		add dword ptr [eax], ebp		// 0x000f266e
Lf2670:
		mov eax, dword ptr [ebx + 4]		// 0x000f2670
		inc ecx		// 0x000f2673
		add edx, 0x1c		// 0x000f2674
		cmp ecx, eax		// 0x000f2677
		jb Lf2660		// 0x000f2679
Lf267b:
		pop ebp		// 0x000f267b
		pop edi		// 0x000f267c
		pop esi		// 0x000f267d
		mov al, 1		// 0x000f267e
		pop ebx		// 0x000f2680
		ret 4		// 0x000f2681
		}
	}

// phys_fn_005606 (0x000f2ec0, 43 B), 16 instructions
__declspec(naked) unsigned __fastcall nxP45606(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [ecx + 4]		// 0x000f2ec0
		mov ecx, dword ptr [ecx + 8]		// 0x000f2ec3
		mov edx, eax		// 0x000f2ec6
		shl edx, 5		// 0x000f2ec8
		push edx		// 0x000f2ecb
		push ecx		// 0x000f2ecc
		push ecx		// 0x000f2ecd
		mov ecx, dword ptr [esp + 0x10]		// 0x000f2ece
		push eax		// 0x000f2ed2
		call nxP4CallStoreDword		// 0x000f2ed3
		mov ecx, eax		// 0x000f2ed8
		call nxP4CallStoreDword		// 0x000f2eda
		mov ecx, eax		// 0x000f2edf
		call nxP44799		// 0x000f2ee1
		mov al, 1		// 0x000f2ee6
		ret 4		// 0x000f2ee8
		}
	}

// phys_fn_005608 and phys_fn_005610 (one function) (0x000f2ef0, 228 B), 91 instructions
__declspec(naked) unsigned __fastcall nxP45608(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ebx		// 0x000f2ef0
		push esi		// 0x000f2ef1
		push edi		// 0x000f2ef2
		mov edi, dword ptr [esp + 0x10]		// 0x000f2ef3
		mov ebx, ecx		// 0x000f2ef7
		mov ecx, edi		// 0x000f2ef9
		call nxP4CallReadDword		// 0x000f2efb
		mov esi, dword ptr [ebx + 8]		// 0x000f2f00
		mov dword ptr [ebx + 4], eax		// 0x000f2f03
		shl eax, 5		// 0x000f2f06
		test esi, esi		// 0x000f2f09
		mov dword ptr [esp + 0x10], eax		// 0x000f2f0b
		je Lf2f28		// 0x000f2f0f
		call nxP4CallNxGetSdkAllocator		// 0x000f2f11
		mov edx, dword ptr [eax]		// 0x000f2f16
		add esi, -4		// 0x000f2f18
		push esi		// 0x000f2f1b
		mov ecx, eax		// 0x000f2f1c
		call dword ptr [edx + 0xc]		// 0x000f2f1e
		mov dword ptr [ebx + 8], 0		// 0x000f2f21
Lf2f28:
		mov esi, dword ptr [ebx + 4]		// 0x000f2f28
		call nxP4CallNxGetSdkAllocator		// 0x000f2f2b
		mov edx, dword ptr [eax]		// 0x000f2f30
		mov ecx, esi		// 0x000f2f32
		shl ecx, 5		// 0x000f2f34
		add ecx, 4		// 0x000f2f37
		push 0		// 0x000f2f3a
		push ecx		// 0x000f2f3c
		mov ecx, eax		// 0x000f2f3d
		call dword ptr [edx]		// 0x000f2f3f
		test eax, eax		// 0x000f2f41
		je Lf2f4c		// 0x000f2f43
		mov dword ptr [eax], esi		// 0x000f2f45
		add eax, 4		// 0x000f2f47
		jmp Lf2f4e		// 0x000f2f4a
Lf2f4c:
		xor eax, eax		// 0x000f2f4c
Lf2f4e:
		test eax, eax		// 0x000f2f4e
		mov dword ptr [ebx + 8], eax		// 0x000f2f50
		jne Lf2f5d		// 0x000f2f53
		pop edi		// 0x000f2f55
		pop esi		// 0x000f2f56
		xor al, al		// 0x000f2f57
		pop ebx		// 0x000f2f59
		ret 4		// 0x000f2f5a
Lf2f5d:
		push ebp		// 0x000f2f5d
		mov ecx, edi		// 0x000f2f5e
		call nxP4CallReadDword		// 0x000f2f60
		mov ebp, dword ptr [ebx + 8]		// 0x000f2f65
		mov esi, dword ptr [esp + 0x14]		// 0x000f2f68
		push esi		// 0x000f2f6c
		mov ecx, edi		// 0x000f2f6d
		sub ebp, eax		// 0x000f2f6f
		call nxP44778		// 0x000f2f71
		mov edi, dword ptr [ebx + 8]		// 0x000f2f76
		mov ecx, esi		// 0x000f2f79
		mov edx, ecx		// 0x000f2f7b
		shr ecx, 2		// 0x000f2f7d
		mov esi, eax		// 0x000f2f80
		rep movsd		// 0x000f2f82
		mov ecx, edx		// 0x000f2f84
		and ecx, 3		// 0x000f2f86
		rep movsb		// 0x000f2f89
		mov eax, dword ptr [ebx + 4]		// 0x000f2f8b
		xor edx, edx		// 0x000f2f8e
		test eax, eax		// 0x000f2f90
		jbe Lf2fcb		// 0x000f2f92
		xor ecx, ecx		// 0x000f2f94
		jmp Lf2fa0		// 0x000f2f96
		lea esp, [esp]		// 0x000f2f98
		nop		// 0x000f2f9f
Lf2fa0:
		mov eax, dword ptr [ebx + 8]		// 0x000f2fa0
		test byte ptr [ecx + eax + 0x18], 1		// 0x000f2fa3
		lea eax, [ecx + eax + 0x18]		// 0x000f2fa8
		jne Lf2fb0		// 0x000f2fac
		add dword ptr [eax], ebp		// 0x000f2fae
Lf2fb0:
		mov eax, dword ptr [ebx + 8]		// 0x000f2fb0
		test byte ptr [ecx + eax + 0x1c], 1		// 0x000f2fb3
		lea eax, [ecx + eax + 0x1c]		// 0x000f2fb8
		jne Lf2fc0		// 0x000f2fbc
		add dword ptr [eax], ebp		// 0x000f2fbe
Lf2fc0:
		mov eax, dword ptr [ebx + 4]		// 0x000f2fc0
		inc edx		// 0x000f2fc3
		add ecx, 0x20		// 0x000f2fc4
		cmp edx, eax		// 0x000f2fc7
		jb Lf2fa0		// 0x000f2fc9
Lf2fcb:
		pop ebp		// 0x000f2fcb
		pop edi		// 0x000f2fcc
		pop esi		// 0x000f2fcd
		mov al, 1		// 0x000f2fce
		pop ebx		// 0x000f2fd0
		ret 4		// 0x000f2fd1
		}
	}

// phys_fn_005624 (0x000f3630, 117 B), 46 instructions
__declspec(naked) unsigned __fastcall nxP45624(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000f3630
		mov esi, ecx		// 0x000f3631
		mov eax, dword ptr [esi + 4]		// 0x000f3633
		mov ecx, dword ptr [esi + 8]		// 0x000f3636
		push edi		// 0x000f3639
		mov edi, dword ptr [esp + 0xc]		// 0x000f363a
		mov edx, eax		// 0x000f363e
		shl edx, 4		// 0x000f3640
		push edx		// 0x000f3643
		push ecx		// 0x000f3644
		push ecx		// 0x000f3645
		push eax		// 0x000f3646
		mov ecx, edi		// 0x000f3647
		call nxP4CallStoreDword		// 0x000f3649
		mov ecx, eax		// 0x000f364e
		call nxP4CallStoreDword		// 0x000f3650
		mov ecx, eax		// 0x000f3655
		call nxP44799		// 0x000f3657
		mov eax, dword ptr [esi + 0x14]		// 0x000f365c
		mov ecx, dword ptr [esi + 0x10]		// 0x000f365f
		mov edx, dword ptr [esi + 0xc]		// 0x000f3662
		push eax		// 0x000f3665
		push ecx		// 0x000f3666
		push edx		// 0x000f3667
		mov ecx, edi		// 0x000f3668
		call nxP4CallStoreDword		// 0x000f366a
		mov ecx, eax		// 0x000f366f
		call nxP4CallStoreDword		// 0x000f3671
		mov ecx, eax		// 0x000f3676
		call nxP4CallStoreDword		// 0x000f3678
		mov eax, dword ptr [esi + 0x20]		// 0x000f367d
		mov ecx, dword ptr [esi + 0x1c]		// 0x000f3680
		mov edx, dword ptr [esi + 0x18]		// 0x000f3683
		push eax		// 0x000f3686
		push ecx		// 0x000f3687
		push edx		// 0x000f3688
		mov ecx, edi		// 0x000f3689
		call nxP4CallStoreDword		// 0x000f368b
		mov ecx, eax		// 0x000f3690
		call nxP4CallStoreDword		// 0x000f3692
		mov ecx, eax		// 0x000f3697
		call nxP4CallStoreDword		// 0x000f3699
		pop edi		// 0x000f369e
		mov al, 1		// 0x000f369f
		pop esi		// 0x000f36a1
		ret 4		// 0x000f36a2
		}
	}

// phys_fn_005642 (0x000f3d40, 118 B), 46 instructions
__declspec(naked) unsigned __fastcall nxP45642(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000f3d40
		mov esi, ecx		// 0x000f3d41
		mov eax, dword ptr [esi + 4]		// 0x000f3d43
		mov ecx, dword ptr [esi + 8]		// 0x000f3d46
		push edi		// 0x000f3d49
		mov edi, dword ptr [esp + 0xc]		// 0x000f3d4a
		lea edx, [eax + eax*4]		// 0x000f3d4e
		shl edx, 2		// 0x000f3d51
		push edx		// 0x000f3d54
		push ecx		// 0x000f3d55
		push ecx		// 0x000f3d56
		push eax		// 0x000f3d57
		mov ecx, edi		// 0x000f3d58
		call nxP4CallStoreDword		// 0x000f3d5a
		mov ecx, eax		// 0x000f3d5f
		call nxP4CallStoreDword		// 0x000f3d61
		mov ecx, eax		// 0x000f3d66
		call nxP44799		// 0x000f3d68
		mov eax, dword ptr [esi + 0x14]		// 0x000f3d6d
		mov ecx, dword ptr [esi + 0x10]		// 0x000f3d70
		mov edx, dword ptr [esi + 0xc]		// 0x000f3d73
		push eax		// 0x000f3d76
		push ecx		// 0x000f3d77
		push edx		// 0x000f3d78
		mov ecx, edi		// 0x000f3d79
		call nxP4CallStoreDword		// 0x000f3d7b
		mov ecx, eax		// 0x000f3d80
		call nxP4CallStoreDword		// 0x000f3d82
		mov ecx, eax		// 0x000f3d87
		call nxP4CallStoreDword		// 0x000f3d89
		mov eax, dword ptr [esi + 0x20]		// 0x000f3d8e
		mov ecx, dword ptr [esi + 0x1c]		// 0x000f3d91
		mov edx, dword ptr [esi + 0x18]		// 0x000f3d94
		push eax		// 0x000f3d97
		push ecx		// 0x000f3d98
		push edx		// 0x000f3d99
		mov ecx, edi		// 0x000f3d9a
		call nxP4CallStoreDword		// 0x000f3d9c
		mov ecx, eax		// 0x000f3da1
		call nxP4CallStoreDword		// 0x000f3da3
		mov ecx, eax		// 0x000f3da8
		call nxP4CallStoreDword		// 0x000f3daa
		pop edi		// 0x000f3daf
		mov al, 1		// 0x000f3db0
		pop esi		// 0x000f3db2
		ret 4		// 0x000f3db3
		}
	}

// phys_fn_005644 and phys_fn_005646 (one function) (0x000f3dc0, 292 B), 109 instructions
__declspec(naked) unsigned __fastcall nxP45644(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ebx		// 0x000f3dc0
		push ebp		// 0x000f3dc1
		push esi		// 0x000f3dc2
		mov esi, dword ptr [esp + 0x10]		// 0x000f3dc3
		mov ebx, ecx		// 0x000f3dc7
		push edi		// 0x000f3dc9
		mov ecx, esi		// 0x000f3dca
		call nxP4CallReadDword		// 0x000f3dcc
		mov ebp, dword ptr [ebx + 8]		// 0x000f3dd1
		lea edi, [eax + eax*4]		// 0x000f3dd4
		shl edi, 2		// 0x000f3dd7
		test ebp, ebp		// 0x000f3dda
		mov dword ptr [ebx + 4], eax		// 0x000f3ddc
		je Lf3df8		// 0x000f3ddf
		call nxP4CallNxGetSdkAllocator		// 0x000f3de1
		mov edx, dword ptr [eax]		// 0x000f3de6
		add ebp, -4		// 0x000f3de8
		push ebp		// 0x000f3deb
		mov ecx, eax		// 0x000f3dec
		call dword ptr [edx + 0xc]		// 0x000f3dee
		mov dword ptr [ebx + 8], 0		// 0x000f3df1
Lf3df8:
		mov ebp, dword ptr [ebx + 4]		// 0x000f3df8
		call nxP4CallNxGetSdkAllocator		// 0x000f3dfb
		mov edx, dword ptr [eax]		// 0x000f3e00
		lea ecx, [ebp + ebp*4]		// 0x000f3e02
		lea ecx, [ecx*4 + 4]		// 0x000f3e06
		push 0		// 0x000f3e0d
		push ecx		// 0x000f3e0f
		mov ecx, eax		// 0x000f3e10
		call dword ptr [edx]		// 0x000f3e12
		test eax, eax		// 0x000f3e14
		je Lf3e1f		// 0x000f3e16
		mov dword ptr [eax], ebp		// 0x000f3e18
		add eax, 4		// 0x000f3e1a
		jmp Lf3e21		// 0x000f3e1d
Lf3e1f:
		xor eax, eax		// 0x000f3e1f
Lf3e21:
		test eax, eax		// 0x000f3e21
		mov dword ptr [ebx + 8], eax		// 0x000f3e23
		jne Lf3e31		// 0x000f3e26
		pop edi		// 0x000f3e28
		pop esi		// 0x000f3e29
		pop ebp		// 0x000f3e2a
		xor al, al		// 0x000f3e2b
		pop ebx		// 0x000f3e2d
		ret 4		// 0x000f3e2e
Lf3e31:
		mov ecx, esi		// 0x000f3e31
		call nxP4CallReadDword		// 0x000f3e33
		mov ebp, dword ptr [ebx + 8]		// 0x000f3e38
		push edi		// 0x000f3e3b
		mov ecx, esi		// 0x000f3e3c
		sub ebp, eax		// 0x000f3e3e
		call nxP44778		// 0x000f3e40
		mov ecx, edi		// 0x000f3e45
		mov edi, dword ptr [ebx + 8]		// 0x000f3e47
		mov edx, ecx		// 0x000f3e4a
		shr ecx, 2		// 0x000f3e4c
		mov esi, eax		// 0x000f3e4f
		rep movsd		// 0x000f3e51
		mov ecx, edx		// 0x000f3e53
		and ecx, 3		// 0x000f3e55
		rep movsb		// 0x000f3e58
		mov eax, dword ptr [ebx + 4]		// 0x000f3e5a
		xor edx, edx		// 0x000f3e5d
		test eax, eax		// 0x000f3e5f
		jbe Lf3e9b		// 0x000f3e61
		xor ecx, ecx		// 0x000f3e63
		jmp Lf3e70		// 0x000f3e65
		lea esp, [esp]		// 0x000f3e67
		mov edi, edi		// 0x000f3e6e
Lf3e70:
		mov eax, dword ptr [ebx + 8]		// 0x000f3e70
		test byte ptr [ecx + eax + 0xc], 1		// 0x000f3e73
		lea eax, [ecx + eax + 0xc]		// 0x000f3e78
		jne Lf3e80		// 0x000f3e7c
		add dword ptr [eax], ebp		// 0x000f3e7e
Lf3e80:
		mov eax, dword ptr [ebx + 8]		// 0x000f3e80
		test byte ptr [ecx + eax + 0x10], 1		// 0x000f3e83
		lea eax, [ecx + eax + 0x10]		// 0x000f3e88
		jne Lf3e90		// 0x000f3e8c
		add dword ptr [eax], ebp		// 0x000f3e8e
Lf3e90:
		mov eax, dword ptr [ebx + 4]		// 0x000f3e90
		inc edx		// 0x000f3e93
		add ecx, 0x14		// 0x000f3e94
		cmp edx, eax		// 0x000f3e97
		jb Lf3e70		// 0x000f3e99
Lf3e9b:
		mov esi, dword ptr [esp + 0x14]		// 0x000f3e9b
		mov ecx, esi		// 0x000f3e9f
		call nxP44776		// 0x000f3ea1
		fstp dword ptr [ebx + 0xc]		// 0x000f3ea6
		mov ecx, esi		// 0x000f3ea9
		call nxP44776		// 0x000f3eab
		fstp dword ptr [ebx + 0x10]		// 0x000f3eb0
		mov ecx, esi		// 0x000f3eb3
		call nxP44776		// 0x000f3eb5
		fstp dword ptr [ebx + 0x14]		// 0x000f3eba
		mov ecx, esi		// 0x000f3ebd
		call nxP44776		// 0x000f3ebf
		fstp dword ptr [ebx + 0x18]		// 0x000f3ec4
		mov ecx, esi		// 0x000f3ec7
		call nxP44776		// 0x000f3ec9
		fstp dword ptr [ebx + 0x1c]		// 0x000f3ece
		mov ecx, esi		// 0x000f3ed1
		call nxP44776		// 0x000f3ed3
		fstp dword ptr [ebx + 0x20]		// 0x000f3ed8
		pop edi		// 0x000f3edb
		pop esi		// 0x000f3edc
		pop ebp		// 0x000f3edd
		mov al, 1		// 0x000f3ede
		pop ebx		// 0x000f3ee0
		ret 4		// 0x000f3ee1
		}
	}

// helper: phys_fn_006332 (record unchanged) (0x001030a0, 10 B), 2 instructions
__declspec(naked) unsigned __cdecl nxP4ThunkF()
	{
	__asm
		{
		mov ecx, offset nxPCStatics + 92		// 0x001030a0
		jmp nxP4CallEmpty		// 0x001030a5
		}
	}

// helper: phys_fn_006334 (record unchanged) (0x001030b0, 10 B), 2 instructions
__declspec(naked) unsigned __cdecl nxP4ThunkG()
	{
	__asm
		{
		mov ecx, offset nxPCStatics + 108		// 0x001030b0
		jmp nxP4CallEmpty		// 0x001030b5
		}
	}
// @@ROWS-END@@
