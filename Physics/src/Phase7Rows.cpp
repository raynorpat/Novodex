/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// Phase7Rows.cpp: Phase 7's remaining rows: the continuous-collision sweep geometry at 0x00056690 to 0x0005a16a
// (phys_fn_002268 to 002316: the box, capsule and triangle sweep tests, the sweep cache with its table, the per-shape
// sweep dispatch), the character controllers' creation (002332, 002336), the step row 002350 and 002400. Naked listing
// transcriptions. The oracle's .data is the candidate's by name where the candidate has it (the identity poses, the ICE
// box tables through their accessors, the SDK and scene rows) and new statics here where it does not (the sweep cache,
// the axis table). The controller classes' tables (the oracle's 0x10108740, 0x1010878c..0x101087ac and
// 0x10108818..0x10108830) are built here entry for entry, from rows transcribed as helpers (their records unchanged)
// and the CRT's _purecall. Compare script: docs/reconstruction/novodex-physics/evidence/phase7-rows-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxP7Call0AABBCollider();
extern "C" void nxP7Call0SdkContainer();
extern "C" void nxP7Call1AABBCollider();
extern "C" void nxP7CallAdd();
extern "C" void nxP7CallChkstk();
extern "C" void nxP7CallCollide();
extern "C" void nxP7CallComputePoints();
extern "C" void nxP7CallComputePointsX();
extern "C" void nxP7CallComputeWorldEdgeNormal();
extern "C" void nxP7CallCreateEdgeList();
extern "C" void nxP7CallEmpty();
extern "C" void nxP7CallFpclass();
extern "C" void nxP7CallFree();
extern "C" void nxP7CallGetEdgeNormals();
extern "C" void nxP7CallGetEdges();
extern "C" void nxP7CallGetEdgesX();
extern "C" void nxP7CallGetVertexNormals();
extern "C" void nxP7CallInflate();
extern "C" void nxP7CallInitialise();
extern "C" void nxP7CallInvertPRMatrix();
extern "C" void nxP7CallLock();
extern "C" void nxP7CallMalloc();
extern "C" void nxP7CallNormal();
extern "C" void nxP7CallNxFillShapeDescriptor();
extern "C" void nxP7CallNxIceIdentityPoses();
extern "C" void nxP7CallNxP45404();
extern "C" void nxP7CallNxPOSetCenterExtents();
extern "C" void nxP7CallNxRayAABBSlab();
extern "C" void nxP7CallOverlapAABBShapes();
extern "C" void nxP7CallResize();
extern "C" void nxP7CallSimulateFrame();
extern "C" void nxP7CallUnlock();
extern "C" void* nxP7ImpImpExitThread4;
extern "C" void* nxP7ImpImpNxComputeBoxPoints;
extern "C" void* nxP7ImpImpNxGetBoxTriangles;
extern "C" void* nxP7ImpImpResetEvent4;
extern "C" void* nxP7ImpImpSetEvent4;
extern "C" void* nxP7ImpImpWaitForSingleObject8;
extern "C" void* nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll;
#pragma comment(linker, "/alternatename:_nxP7Call0AABBCollider=??0AABBCollider@Opcode@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP7Call0SdkContainer=??0SdkContainer@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxP7Call1AABBCollider=??1AABBCollider@Opcode@@UAE@XZ")
#pragma comment(linker, "/alternatename:_nxP7CallAdd=?Add@AABB@IceMaths@@QAEAAV12@ABV12@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallChkstk=__chkstk")
#pragma comment(linker, "/alternatename:_nxP7CallCollide=?Collide@AABBCollider@Opcode@@QAE_NAAUAABBCache@2@ABVCollisionAABB@2@ABVModel@2@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallComputePoints=?ComputePoints@OBB@IceMaths@@QBE_NPAVPoint@2@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallComputePointsX=?ComputePoints@AABB@IceMaths@@QBE_NPAVPoint@2@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallComputeWorldEdgeNormal=?ComputeWorldEdgeNormal@OBB@IceMaths@@QBEXIAAVPoint@2@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallCreateEdgeList=?createEdgeList@TriangleMesh@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxP7CallEmpty=?empty@SdkContainer@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxP7CallFpclass=__fpclass")
#pragma comment(linker, "/alternatename:_nxP7CallFree=_free")
#pragma comment(linker, "/alternatename:_nxP7CallGetEdgeNormals=?GetEdgeNormals@AABB@IceMaths@@QBEPBVPoint@2@XZ")
#pragma comment(linker, "/alternatename:_nxP7CallGetEdges=?GetEdges@OBB@IceMaths@@QBEPBIXZ")
#pragma comment(linker, "/alternatename:_nxP7CallGetEdgesX=?GetEdges@AABB@IceMaths@@QBEPBIXZ")
#pragma comment(linker, "/alternatename:_nxP7CallGetVertexNormals=?GetVertexNormals@AABB@IceMaths@@QBEPBVPoint@2@XZ")
#pragma comment(linker, "/alternatename:_nxP7CallInflate=?Inflate@Triangle@IceMaths@@QAEXM_N@Z")
#pragma comment(linker, "/alternatename:_nxP7CallInitialise=?initialise@NxSceneInternal@@QAE_NABVNxSceneDesc@@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallInvertPRMatrix=?InvertPRMatrix@IceMaths@@YAXAAVMatrix4x4@1@ABV21@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallLock=?lock@ReadWriteLock@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxP7CallMalloc=_malloc")
#pragma comment(linker, "/alternatename:_nxP7CallNormal=?Normal@Triangle@IceMaths@@QBEXAAVPoint@2@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallNxFillShapeDescriptor=?nxFillShapeDescriptor@BoxShape@@QBEXPAI@Z")
#pragma comment(linker, "/alternatename:_nxP7CallNxIceIdentityPoses=?nxIceIdentityPoses@@YAPBMXZ")
#pragma comment(linker, "/alternatename:_nxP7CallNxP45404=?nxP45404@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxP7CallNxPOSetCenterExtents=?nxPOSetCenterExtents@@YIXPAX0PBX1@Z")
#pragma comment(linker, "/alternatename:_nxP7CallNxRayAABBSlab=?NxRayAABBSlab@@YAHPBM000PAM1@Z")
#pragma comment(linker, "/alternatename:_nxP7CallOverlapAABBShapes=?overlapAABBShapes@NxSceneInternal@@QAEIABVNxBounds3@@W4NxShapesType@@IPAPAVNxShape@@PAV?$NxUserEntityReport@PAVNxShape@@@@@Z")
#pragma comment(linker, "/alternatename:_nxP7CallResize=?resize@SdkContainer@@QAE_NI@Z")
#pragma comment(linker, "/alternatename:_nxP7CallSimulateFrame=?simulateFrame@NxSceneInternal@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxP7CallUnlock=?unlock@ReadWriteLock@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxP7ImpImpExitThread4=__imp__ExitThread@4")
#pragma comment(linker, "/alternatename:_nxP7ImpImpNxComputeBoxPoints=__imp__NxComputeBoxPoints")
#pragma comment(linker, "/alternatename:_nxP7ImpImpNxGetBoxTriangles=__imp__NxGetBoxTriangles")
#pragma comment(linker, "/alternatename:_nxP7ImpImpResetEvent4=__imp__ResetEvent@4")
#pragma comment(linker, "/alternatename:_nxP7ImpImpSetEvent4=__imp__SetEvent@4")
#pragma comment(linker, "/alternatename:_nxP7ImpImpWaitForSingleObject8=__imp__WaitForSingleObject@8")
#pragma comment(linker, "/alternatename:_nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll=__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A")
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
// .data 0x1012466c and 0x10124670: a guard byte and the sweep cache 002300 to 002308 keep.
static unsigned char nxP7SweepGuard; static unsigned nxP7SweepCache;
// .data 0x101222f8: the six signed axes (written once by the image, read here as a table).
static float nxP7Axes[18] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f };
extern "C" void* nxP7Tab108740[2]; extern "C" void* nxP7TabR1[8]; extern "C" void* nxP7TabR2[6];
extern "C" void* nxP7IdentityCtor;
#pragma comment(linker, "/alternatename:_nxP7IdentityCtor=_nxIceIdentityConstruct")
extern "C" void nxP7PureCall();
#pragma comment(linker, "/alternatename:_nxP7PureCall=__purecall")
extern "C" void* nxP7VtShapeDesc;
#pragma comment(linker, "/alternatename:_nxP7VtShapeDesc=??_7NxShapeDesc@@6B@")

unsigned __stdcall nxP70001(unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxP71281(void* self, void* edxUnused);
unsigned __fastcall nxP71957(void* self, void* edxUnused);
unsigned __fastcall nxP71960(void* self, void* edxUnused);
unsigned __fastcall nxP72268(void* self, void* edxUnused);
unsigned __fastcall nxP72274(void* self, void* edxUnused);
unsigned __fastcall nxP72276(void* self, void* edxUnused);
unsigned __fastcall nxP72278(void* self, void* edxUnused);
unsigned __fastcall nxP72280(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __cdecl nxP72282();
unsigned __fastcall nxP72288(void* self, void* edxUnused);
unsigned __cdecl nxP72290();
unsigned __fastcall nxP72296(void* self, void* edxUnused);
unsigned __fastcall nxP72298(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5, unsigned a6, unsigned a7, unsigned a8, unsigned a9);
unsigned __fastcall nxP72300(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5, unsigned a6, unsigned a7, unsigned a8, unsigned a9, unsigned a10, unsigned a11, unsigned a12, unsigned a13, unsigned a14, unsigned a15);
unsigned __fastcall nxP72304(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5, unsigned a6, unsigned a7, unsigned a8, unsigned a9, unsigned a10);
unsigned __cdecl nxP72306();
unsigned __fastcall nxP72308(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __cdecl nxP72310();
unsigned __fastcall nxP72312(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP72314(void* self, void* edxUnused);
unsigned __fastcall nxP72316(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxP72322(void* self, void* edxUnused);
unsigned __fastcall nxP72324(void* self, void* edxUnused);
unsigned __fastcall nxP72326(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP72328(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP72332(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxP72334(void* self, void* edxUnused);
unsigned __fastcall nxP72336(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP72350(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP72371(void* self, void* edxUnused);
unsigned __fastcall nxP72373(void* self, void* edxUnused);
unsigned __fastcall nxP72375(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP72387(void* self, void* edxUnused);
unsigned __fastcall nxP72389(void* self, void* edxUnused);
unsigned __fastcall nxP72400(void* self, void* edxUnused);
unsigned __fastcall nxP74031(void* self, void* edxUnused, unsigned a0);
// @@PROTOS-END@@
// The controller classes' tables, entry for entry as the oracle has them (0x10108740; 0x1010878c..0x101087ac, which
// the oracle's 0x10108798, 0x1010879c and 0x101087a8 point into; 0x10108818..0x10108830, with 0x10108824 inside it).
extern "C" void* nxP7Tab108740[2] = { (void*) &nxP72298, (void*) &nxP72280 };
extern "C" void* nxP7TabR1[8] = { (void*) &nxP72312, (void*) &nxP7PureCall, (void*) &nxP7PureCall, (void*) &nxP72326, (void*) &nxP72312, (void*) &nxP7PureCall, (void*) &nxP7PureCall, (void*) &nxP72322 };
extern "C" void* nxP7TabR2[6] = { (void*) &nxP74031, (void*) &nxP72314, (void*) &nxP72324, (void*) &nxP72328, (void*) &nxP72308, (void*) &nxP72334 };


static const unsigned nxP7K101041ec_4 = 0x3f800000;
static const unsigned nxP7K101041f0_4 = 0x0;
static const unsigned nxP7K101043cc_4 = 0x3f000000;
static const unsigned nxP7K10106880_4 = 0x358637bd;
static const unsigned nxP7K10106954_4 = 0x3dcccccd;
static const unsigned nxP7K101079e8_4 = 0xb58637bd;
static const unsigned nxP7K10107a0c_4 = 0x34000000;
static const unsigned nxP7K10107a10_4 = 0xb4000000;
static const unsigned nxP7K10108738_4 = 0x3c23d70a;
static const unsigned nxP7K1010874c_4 = 0x3f8ccccd;

// helper: phys_fn_000001 (record unchanged) (0x00001000, 48 B), 22 instructions
__declspec(naked) unsigned __stdcall nxP70001(unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		mov eax, dword ptr [esp + 0xc]		// 0x00001000
		dec eax		// 0x00001004
		js L102d		// 0x00001005
		push ebx		// 0x00001007
		mov ebx, dword ptr [esp + 0x14]		// 0x00001008
		push ebp		// 0x0000100c
		mov ebp, dword ptr [esp + 0x10]		// 0x0000100d
		push esi		// 0x00001011
		mov esi, dword ptr [esp + 0x10]		// 0x00001012
		push edi		// 0x00001016
		lea edi, [eax + 1]		// 0x00001017
		lea ebx, [ebx]		// 0x0000101a
L1020:
		mov ecx, esi		// 0x00001020
		call ebx		// 0x00001022
		add esi, ebp		// 0x00001024
		dec edi		// 0x00001026
		jne L1020		// 0x00001027
		pop edi		// 0x00001029
		pop esi		// 0x0000102a
		pop ebp		// 0x0000102b
		pop ebx		// 0x0000102c
L102d:
		ret 0x10		// 0x0000102d
		}
	}

// helper: phys_fn_001281 (record unchanged) (0x000257a0, 4 B), 2 instructions
__declspec(naked) unsigned __fastcall nxP71281(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [ecx + 4]		// 0x000257a0
		ret		// 0x000257a3
		}
	}

// helper: phys_fn_001957 (record unchanged) (0x0004be80, 16 B), 7 instructions
__declspec(naked) unsigned __fastcall nxP71957(void* self, void* edxUnused)
	{
	__asm
		{
		mov ecx, dword ptr [ecx + 0x1c]		// 0x0004be80
		xor eax, eax		// 0x0004be83
		test ecx, ecx		// 0x0004be85
		je L4be8f		// 0x0004be87
		mov eax, dword ptr [ecx + 0xc]		// 0x0004be89
		add eax, dword ptr [ecx + 8]		// 0x0004be8c
L4be8f:
		ret		// 0x0004be8f
		}
	}

// helper: phys_fn_001960 (record unchanged) (0x0004bec0, 20 B), 8 instructions
__declspec(naked) unsigned __fastcall nxP71960(void* self, void* edxUnused)
	{
	__asm
		{
		mov edx, dword ptr [ecx + 0x70]		// 0x0004bec0
		mov ecx, dword ptr [ecx + edx*4 + 0x1c]		// 0x0004bec3
		xor eax, eax		// 0x0004bec7
		test ecx, ecx		// 0x0004bec9
		je L4bed3		// 0x0004becb
		mov eax, dword ptr [ecx + 0xc]		// 0x0004becd
		add eax, dword ptr [ecx + 8]		// 0x0004bed0
L4bed3:
		ret		// 0x0004bed3
		}
	}

// phys_fn_002268 and phys_fn_002270 (one function) (0x00056690, 1086 B), 374 instructions
__declspec(naked) unsigned __fastcall nxP72268(void* self, void* edxUnused)
	{
	__asm
		{
		sub esp, 0x50		// 0x00056690
		fld dword ptr [eax]		// 0x00056693
		push ebx		// 0x00056695
		mov ebx, dword ptr [esp + 0x5c]		// 0x00056696
		fsub dword ptr [ebx]		// 0x0005669a
		push ebp		// 0x0005669c
		fld dword ptr [eax + 4]		// 0x0005669d
		push esi		// 0x000566a0
		fsub dword ptr [ebx + 4]		// 0x000566a1
		mov esi, dword ptr [esp + 0x60]		// 0x000566a4
		fld dword ptr [eax + 8]		// 0x000566a8
		lea eax, [ecx + 4]		// 0x000566ab
		fsub dword ptr [ebx + 8]		// 0x000566ae
		mov ebx, 3		// 0x000566b1
		fld st(0)		// 0x000566b6
		fmul dword ptr [ecx + 8]		// 0x000566b8
		fld st(3)		// 0x000566bb
		fmul dword ptr [ecx]		// 0x000566bd
		faddp st(1), st(0)		// 0x000566bf
		fld st(2)		// 0x000566c1
		fmul dword ptr [eax]		// 0x000566c3
		faddp st(1), st(0)		// 0x000566c5
		fstp dword ptr [esp + 0x2c]		// 0x000566c7
		fld st(0)		// 0x000566cb
		fmul dword ptr [ecx + 0x14]		// 0x000566cd
		fld st(2)		// 0x000566d0
		fmul dword ptr [ecx + 0x10]		// 0x000566d2
		faddp st(1), st(0)		// 0x000566d5
		fld st(3)		// 0x000566d7
		fmul dword ptr [ecx + 0xc]		// 0x000566d9
		faddp st(1), st(0)		// 0x000566dc
		fstp dword ptr [esp + 0x30]		// 0x000566de
		fmul dword ptr [ecx + 0x20]		// 0x000566e2
		fxch st(1)		// 0x000566e5
		fmul dword ptr [ecx + 0x1c]		// 0x000566e7
		faddp st(1), st(0)		// 0x000566ea
		fxch st(1)		// 0x000566ec
		fmul dword ptr [ecx + 0x18]		// 0x000566ee
		lea ecx, [esp + 0x3c]		// 0x000566f1
		faddp st(1), st(0)		// 0x000566f5
		fstp dword ptr [esp + 0x34]		// 0x000566f7
		jmp L56700		// 0x000566fb
		lea ecx, [ecx]		// 0x000566fd
L56700:
		fld dword ptr [eax + 4]		// 0x00056700
		add eax, 0xc		// 0x00056703
		fmul dword ptr [edx + 8]		// 0x00056706
		add ecx, 0xc		// 0x00056709
		dec ebx		// 0x0005670c
		fld dword ptr [edx + 4]		// 0x0005670d
		fmul dword ptr [eax - 0xc]		// 0x00056710
		faddp st(1), st(0)		// 0x00056713
		fld dword ptr [eax - 0x10]		// 0x00056715
		fmul dword ptr [edx]		// 0x00056718
		faddp st(1), st(0)		// 0x0005671a
		fstp dword ptr [ecx - 0x10]		// 0x0005671c
		fld dword ptr [edx + 0x14]		// 0x0005671f
		fmul dword ptr [eax - 8]		// 0x00056722
		fld dword ptr [edx + 0x10]		// 0x00056725
		fmul dword ptr [eax - 0xc]		// 0x00056728
		faddp st(1), st(0)		// 0x0005672b
		fld dword ptr [edx + 0xc]		// 0x0005672d
		fmul dword ptr [eax - 0x10]		// 0x00056730
		faddp st(1), st(0)		// 0x00056733
		fstp dword ptr [ecx - 0xc]		// 0x00056735
		fld dword ptr [edx + 0x20]		// 0x00056738
		fmul dword ptr [eax - 8]		// 0x0005673b
		fld dword ptr [edx + 0x1c]		// 0x0005673e
		fmul dword ptr [eax - 0xc]		// 0x00056741
		faddp st(1), st(0)		// 0x00056744
		fld dword ptr [edx + 0x18]		// 0x00056746
		fmul dword ptr [eax - 0x10]		// 0x00056749
		faddp st(1), st(0)		// 0x0005674c
		fstp dword ptr [ecx - 8]		// 0x0005674e
		jne L56700		// 0x00056751
		lea ebp, [esp + 0x2c]		// 0x00056753
		xor ebx, ebx		// 0x00056757
		mov edx, esi		// 0x00056759
		lea ecx, [esp + 0x3c]		// 0x0005675b
		sub ebp, esi		// 0x0005675f
L56761:
		fld dword ptr [edx + ebp]		// 0x00056761
		fabs		// 0x00056764
		fld dword ptr [ecx + 4]		// 0x00056766
		fabs		// 0x00056769
		fxch st(1)		// 0x0005676b
		fxch st(1)		// 0x0005676d
		fmul dword ptr [edi + 8]		// 0x0005676f
		fld dword ptr [ecx - 4]		// 0x00056772
		fabs		// 0x00056775
		fst dword ptr [esp + 0x64]		// 0x00056777
		fmul dword ptr [edi]		// 0x0005677b
		faddp st(1), st(0)		// 0x0005677d
		fld dword ptr [ecx]		// 0x0005677f
		fabs		// 0x00056781
		fst dword ptr [esp + 0x64]		// 0x00056783
		fmul dword ptr [edi + 4]		// 0x00056787
		faddp st(1), st(0)		// 0x0005678a
		fadd dword ptr [edx]		// 0x0005678c
		fcompp		// 0x0005678e
		fnstsw ax		// 0x00056790
		test ah, 5		// 0x00056792
		jnp L56abc		// 0x00056795
		inc ebx		// 0x0005679b
		add ecx, 0xc		// 0x0005679c
		add edx, 4		// 0x0005679f
		cmp ebx, 3		// 0x000567a2
		jb L56761		// 0x000567a5
		mov edx, edi		// 0x000567a7
		lea eax, [esp + 0x44]		// 0x000567a9
		xor ecx, ecx		// 0x000567ad
		sub edx, eax		// 0x000567af
L567b1:
		fld dword ptr [esp + 0x2c]		// 0x000567b1
		lea eax, [edx + ecx*4]		// 0x000567b5
		fmul dword ptr [esp + ecx*4 + 0x38]		// 0x000567b8
		fld dword ptr [esp + 0x30]		// 0x000567bc
		fmul dword ptr [esp + ecx*4 + 0x44]		// 0x000567c0
		faddp st(1), st(0)		// 0x000567c4
		fld dword ptr [esp + 0x34]		// 0x000567c6
		fmul dword ptr [esp + ecx*4 + 0x50]		// 0x000567ca
		faddp st(1), st(0)		// 0x000567ce
		fabs		// 0x000567d0
		fld dword ptr [esp + ecx*4 + 0x50]		// 0x000567d2
		fabs		// 0x000567d6
		fxch st(1)		// 0x000567d8
		fxch st(1)		// 0x000567da
		fmul dword ptr [esi + 8]		// 0x000567dc
		fld dword ptr [esp + ecx*4 + 0x44]		// 0x000567df
		fabs		// 0x000567e3
		fst dword ptr [esp + 0x64]		// 0x000567e5
		fmul dword ptr [esi + 4]		// 0x000567e9
		faddp st(1), st(0)		// 0x000567ec
		fld dword ptr [esp + ecx*4 + 0x38]		// 0x000567ee
		fabs		// 0x000567f2
		fst dword ptr [esp + 0x64]		// 0x000567f4
		fmul dword ptr [esi]		// 0x000567f8
		faddp st(1), st(0)		// 0x000567fa
		fadd dword ptr [esp + eax + 0x44]		// 0x000567fc
		fcompp		// 0x00056800
		fnstsw ax		// 0x00056802
		test ah, 5		// 0x00056804
		jnp L56abc		// 0x00056807
		inc ecx		// 0x0005680d
		cmp ecx, 3		// 0x0005680e
		jb L567b1		// 0x00056811
		mov al, byte ptr [esp + 0x68]		// 0x00056813
		test al, al		// 0x00056817
		je L56ac5		// 0x00056819
		fld dword ptr [esp + 0x44]		// 0x0005681f
		fabs		// 0x00056823
		fstp dword ptr [esp + 0x64]		// 0x00056825
		fld dword ptr [esp + 0x50]		// 0x00056829
		fabs		// 0x0005682d
		fstp dword ptr [esp + 0x28]		// 0x0005682f
		fld dword ptr [esp + 0x3c]		// 0x00056833
		fabs		// 0x00056837
		fstp dword ptr [esp + 0x14]		// 0x00056839
		fld dword ptr [esp + 0x40]		// 0x0005683d
		fabs		// 0x00056841
		fstp dword ptr [esp + 0x20]		// 0x00056843
		fld dword ptr [esp + 0x34]		// 0x00056847
		fmul dword ptr [esp + 0x44]		// 0x0005684b
		fld dword ptr [esp + 0x30]		// 0x0005684f
		fmul dword ptr [esp + 0x50]		// 0x00056853
		fsubp st(1), st(0)		// 0x00056857
		fabs		// 0x00056859
		fld dword ptr [esp + 0x14]		// 0x0005685b
		fmul dword ptr [edi + 8]		// 0x0005685f
		fld dword ptr [esp + 0x20]		// 0x00056862
		fmul dword ptr [edi + 4]		// 0x00056866
		faddp st(1), st(0)		// 0x00056869
		fld dword ptr [esp + 0x28]		// 0x0005686b
		fmul dword ptr [esi + 4]		// 0x0005686f
		fld dword ptr [esp + 0x64]		// 0x00056872
		fmul dword ptr [esi + 8]		// 0x00056876
		faddp st(1), st(0)		// 0x00056879
		faddp st(1), st(0)		// 0x0005687b
		fcompp		// 0x0005687d
		fnstsw ax		// 0x0005687f
		test ah, 5		// 0x00056881
		jnp L56abc		// 0x00056884
		fld dword ptr [esp + 0x48]		// 0x0005688a
		fabs		// 0x0005688e
		fstp dword ptr [esp + 0x10]		// 0x00056890
		fld dword ptr [esp + 0x54]		// 0x00056894
		fabs		// 0x00056898
		fstp dword ptr [esp + 0x24]		// 0x0005689a
		fld dword ptr [esp + 0x38]		// 0x0005689e
		fabs		// 0x000568a2
		fstp dword ptr [esp + 0xc]		// 0x000568a4
		fld dword ptr [esp + 0x34]		// 0x000568a8
		fmul dword ptr [esp + 0x48]		// 0x000568ac
		fld dword ptr [esp + 0x30]		// 0x000568b0
		fmul dword ptr [esp + 0x54]		// 0x000568b4
		fsubp st(1), st(0)		// 0x000568b8
		fabs		// 0x000568ba
		fld dword ptr [esp + 0xc]		// 0x000568bc
		fmul dword ptr [edi + 8]		// 0x000568c0
		fld dword ptr [esp + 0x20]		// 0x000568c3
		fmul dword ptr [edi]		// 0x000568c7
		faddp st(1), st(0)		// 0x000568c9
		fld dword ptr [esp + 0x24]		// 0x000568cb
		fmul dword ptr [esi + 4]		// 0x000568cf
		fld dword ptr [esp + 0x10]		// 0x000568d2
		fmul dword ptr [esi + 8]		// 0x000568d6
		faddp st(1), st(0)		// 0x000568d9
		faddp st(1), st(0)		// 0x000568db
		fcompp		// 0x000568dd
		fnstsw ax		// 0x000568df
		test ah, 5		// 0x000568e1
		jnp L56abc		// 0x000568e4
		fld dword ptr [esp + 0x4c]		// 0x000568ea
		fabs		// 0x000568ee
		fstp dword ptr [esp + 0x1c]		// 0x000568f0
		fld dword ptr [esp + 0x58]		// 0x000568f4
		fabs		// 0x000568f8
		fstp dword ptr [esp + 0x18]		// 0x000568fa
		fld dword ptr [esp + 0x34]		// 0x000568fe
		fmul dword ptr [esp + 0x4c]		// 0x00056902
		fld dword ptr [esp + 0x30]		// 0x00056906
		fmul dword ptr [esp + 0x58]		// 0x0005690a
		fsubp st(1), st(0)		// 0x0005690e
		fabs		// 0x00056910
		fld dword ptr [esp + 0x18]		// 0x00056912
		fmul dword ptr [esi + 4]		// 0x00056916
		fld dword ptr [esp + 0x1c]		// 0x00056919
		fmul dword ptr [esi + 8]		// 0x0005691d
		faddp st(1), st(0)		// 0x00056920
		fld dword ptr [esp + 0xc]		// 0x00056922
		fmul dword ptr [edi + 4]		// 0x00056926
		fld dword ptr [esp + 0x14]		// 0x00056929
		fmul dword ptr [edi]		// 0x0005692d
		faddp st(1), st(0)		// 0x0005692f
		faddp st(1), st(0)		// 0x00056931
		fcompp		// 0x00056933
		fnstsw ax		// 0x00056935
		test ah, 5		// 0x00056937
		jnp L56abc		// 0x0005693a
		fld dword ptr [esp + 0x2c]		// 0x00056940
		fmul dword ptr [esp + 0x50]		// 0x00056944
		fld dword ptr [esp + 0x34]		// 0x00056948
		fmul dword ptr [esp + 0x38]		// 0x0005694c
		fsubp st(1), st(0)		// 0x00056950
		fabs		// 0x00056952
		fld dword ptr [esp + 0x28]		// 0x00056954
		fmul dword ptr [esi]		// 0x00056958
		fld dword ptr [esp + 0xc]		// 0x0005695a
		fmul dword ptr [esi + 8]		// 0x0005695e
		faddp st(1), st(0)		// 0x00056961
		fld dword ptr [esp + 0x10]		// 0x00056963
		fmul dword ptr [edi + 8]		// 0x00056967
		fld dword ptr [esp + 0x1c]		// 0x0005696a
		fmul dword ptr [edi + 4]		// 0x0005696e
		faddp st(1), st(0)		// 0x00056971
		faddp st(1), st(0)		// 0x00056973
		fcompp		// 0x00056975
		fnstsw ax		// 0x00056977
		test ah, 5		// 0x00056979
		jnp L56abc		// 0x0005697c
		fld dword ptr [esp + 0x2c]		// 0x00056982
		fmul dword ptr [esp + 0x54]		// 0x00056986
		fld dword ptr [esp + 0x34]		// 0x0005698a
		fmul dword ptr [esp + 0x3c]		// 0x0005698e
		fsubp st(1), st(0)		// 0x00056992
		fabs		// 0x00056994
		fld dword ptr [esp + 0x24]		// 0x00056996
		fmul dword ptr [esi]		// 0x0005699a
		fld dword ptr [esp + 0x14]		// 0x0005699c
		fmul dword ptr [esi + 8]		// 0x000569a0
		faddp st(1), st(0)		// 0x000569a3
		fld dword ptr [esp + 0x64]		// 0x000569a5
		fmul dword ptr [edi + 8]		// 0x000569a9
		fld dword ptr [esp + 0x1c]		// 0x000569ac
		fmul dword ptr [edi]		// 0x000569b0
		faddp st(1), st(0)		// 0x000569b2
		faddp st(1), st(0)		// 0x000569b4
		fcompp		// 0x000569b6
		fnstsw ax		// 0x000569b8
		test ah, 5		// 0x000569ba
		jnp L56abc		// 0x000569bd
		fld dword ptr [esp + 0x2c]		// 0x000569c3
		fmul dword ptr [esp + 0x58]		// 0x000569c7
		fld dword ptr [esp + 0x34]		// 0x000569cb
		fmul dword ptr [esp + 0x40]		// 0x000569cf
		fsubp st(1), st(0)		// 0x000569d3
		fabs		// 0x000569d5
		fld dword ptr [esp + 0x18]		// 0x000569d7
		fmul dword ptr [esi]		// 0x000569db
		fld dword ptr [esp + 0x20]		// 0x000569dd
		fmul dword ptr [esi + 8]		// 0x000569e1
		faddp st(1), st(0)		// 0x000569e4
		fld dword ptr [esp + 0x10]		// 0x000569e6
		fmul dword ptr [edi]		// 0x000569ea
		fld dword ptr [esp + 0x64]		// 0x000569ec
		fmul dword ptr [edi + 4]		// 0x000569f0
		faddp st(1), st(0)		// 0x000569f3
		faddp st(1), st(0)		// 0x000569f5
		fcompp		// 0x000569f7
		fnstsw ax		// 0x000569f9
		test ah, 5		// 0x000569fb
		jnp L56abc		// 0x000569fe
		fld dword ptr [esp + 0x30]		// 0x00056a04
		fmul dword ptr [esp + 0x38]		// 0x00056a08
		fld dword ptr [esp + 0x2c]		// 0x00056a0c
		fmul dword ptr [esp + 0x44]		// 0x00056a10
		fsubp st(1), st(0)		// 0x00056a14
		fabs		// 0x00056a16
		fld dword ptr [esp + 0x24]		// 0x00056a18
		fmul dword ptr [edi + 8]		// 0x00056a1c
		fld dword ptr [esp + 0x18]		// 0x00056a1f
		fmul dword ptr [edi + 4]		// 0x00056a23
		faddp st(1), st(0)		// 0x00056a26
		fld dword ptr [esp + 0x64]		// 0x00056a28
		fmul dword ptr [esi]		// 0x00056a2c
		fld dword ptr [esp + 0xc]		// 0x00056a2e
		fmul dword ptr [esi + 4]		// 0x00056a32
		faddp st(1), st(0)		// 0x00056a35
		faddp st(1), st(0)		// 0x00056a37
		fcompp		// 0x00056a39
		fnstsw ax		// 0x00056a3b
		test ah, 5		// 0x00056a3d
		jnp L56abc		// 0x00056a40
		fld dword ptr [esp + 0x30]		// 0x00056a42
		fmul dword ptr [esp + 0x3c]		// 0x00056a46
		fld dword ptr [esp + 0x2c]		// 0x00056a4a
		fmul dword ptr [esp + 0x48]		// 0x00056a4e
		fsubp st(1), st(0)		// 0x00056a52
		fabs		// 0x00056a54
		fld dword ptr [esp + 0x10]		// 0x00056a56
		fmul dword ptr [esi]		// 0x00056a5a
		fld dword ptr [esp + 0x14]		// 0x00056a5c
		fmul dword ptr [esi + 4]		// 0x00056a60
		faddp st(1), st(0)		// 0x00056a63
		fld dword ptr [esp + 0x28]		// 0x00056a65
		fmul dword ptr [edi + 8]		// 0x00056a69
		fld dword ptr [esp + 0x18]		// 0x00056a6c
		fmul dword ptr [edi]		// 0x00056a70
		faddp st(1), st(0)		// 0x00056a72
		faddp st(1), st(0)		// 0x00056a74
		fcompp		// 0x00056a76
		fnstsw ax		// 0x00056a78
		test ah, 5		// 0x00056a7a
		jnp L56abc		// 0x00056a7d
		fld dword ptr [esp + 0x30]		// 0x00056a7f
		fmul dword ptr [esp + 0x40]		// 0x00056a83
		fld dword ptr [esp + 0x2c]		// 0x00056a87
		fmul dword ptr [esp + 0x4c]		// 0x00056a8b
		fsubp st(1), st(0)		// 0x00056a8f
		fabs		// 0x00056a91
		fld dword ptr [esp + 0x1c]		// 0x00056a93
		fmul dword ptr [esi]		// 0x00056a97
		fld dword ptr [esp + 0x20]		// 0x00056a99
		fmul dword ptr [esi + 4]		// 0x00056a9d
		faddp st(1), st(0)		// 0x00056aa0
		fld dword ptr [esp + 0x24]		// 0x00056aa2
		fmul dword ptr [edi]		// 0x00056aa6
		fld dword ptr [esp + 0x28]		// 0x00056aa8
		fmul dword ptr [edi + 4]		// 0x00056aac
		faddp st(1), st(0)		// 0x00056aaf
		faddp st(1), st(0)		// 0x00056ab1
		fcompp		// 0x00056ab3
		fnstsw ax		// 0x00056ab5
		test ah, 5		// 0x00056ab7
		jp L56ac5		// 0x00056aba
L56abc:
		pop esi		// 0x00056abc
		pop ebp		// 0x00056abd
		xor al, al		// 0x00056abe
		pop ebx		// 0x00056ac0
		add esp, 0x50		// 0x00056ac1
		ret		// 0x00056ac4
L56ac5:
		pop esi		// 0x00056ac5
		pop ebp		// 0x00056ac6
		mov al, 1		// 0x00056ac7
		pop ebx		// 0x00056ac9
		add esp, 0x50		// 0x00056aca
		ret		// 0x00056acd
		}
	}

// phys_fn_002274 (0x00056b10, 300 B), 100 instructions
__declspec(naked) unsigned __fastcall nxP72274(void* self, void* edxUnused)
	{
	__asm
		{
L56b10:
		fld dword ptr [ecx]		// 0x00056b10
		fcomp dword ptr [nxP7K10107a10_4]		// 0x00056b12
		fnstsw ax		// 0x00056b18
		test ah, 0x41		// 0x00056b1a
		jne L56b57		// 0x00056b1d
		fld dword ptr [ecx]		// 0x00056b1f
		fcomp dword ptr [nxP7K10107a0c_4]		// 0x00056b21
		fnstsw ax		// 0x00056b27
		test ah, 5		// 0x00056b29
		jp L56b57		// 0x00056b2c
		fld dword ptr [edx + ecx]		// 0x00056b2e
		fcomp dword ptr [edi + ecx]		// 0x00056b31
		fnstsw ax		// 0x00056b34
		test ah, 5		// 0x00056b36
		jnp L56b4c		// 0x00056b39
		fld dword ptr [edx + ecx]		// 0x00056b3b
		fcomp dword ptr [ebx + ecx]		// 0x00056b3e
		fnstsw ax		// 0x00056b41
		test ah, 0x41		// 0x00056b43
		jne L56bf9		// 0x00056b46
L56b4c:
		pop edi		// 0x00056b4c
		pop esi		// 0x00056b4d
		pop ebp		// 0x00056b4e
		or eax, 0xffffffff		// 0x00056b4f
		pop ebx		// 0x00056b52
		add esp, 0xc		// 0x00056b53
		ret		// 0x00056b56
L56b57:
		fld dword ptr [nxP7K101041ec_4]		// 0x00056b57
		mov ebp, esi		// 0x00056b5d
		fdiv dword ptr [ecx]		// 0x00056b5f
		fstp dword ptr [esp + 0x10]		// 0x00056b61
		fld dword ptr [edi + ecx]		// 0x00056b65
		fsub dword ptr [edx + ecx]		// 0x00056b68
		fmul dword ptr [esp + 0x10]		// 0x00056b6b
		fld dword ptr [ebx + ecx]		// 0x00056b6f
		fsub dword ptr [edx + ecx]		// 0x00056b72
		fmul dword ptr [esp + 0x10]		// 0x00056b75
		fstp dword ptr [esp + 0x10]		// 0x00056b79
		fcom dword ptr [esp + 0x10]		// 0x00056b7d
		fnstsw ax		// 0x00056b81
		test ah, 0x41		// 0x00056b83
		jne L56b9b		// 0x00056b86
		fstp dword ptr [esp + 0x18]		// 0x00056b88
		lea ebp, [esi + 3]		// 0x00056b8c
		mov eax, dword ptr [esp + 0x18]		// 0x00056b8f
		fld dword ptr [esp + 0x10]		// 0x00056b93
		mov dword ptr [esp + 0x10], eax		// 0x00056b97
L56b9b:
		mov eax, dword ptr [esp + 0x2c]		// 0x00056b9b
		fld st(0)		// 0x00056b9f
		fcomp dword ptr [eax]		// 0x00056ba1
		fnstsw ax		// 0x00056ba3
		test ah, 0x41		// 0x00056ba5
		jne L56bb6		// 0x00056ba8
		mov eax, dword ptr [esp + 0x2c]		// 0x00056baa
		fstp dword ptr [eax]		// 0x00056bae
		mov dword ptr [esp + 0x14], ebp		// 0x00056bb0
		jmp L56bb8		// 0x00056bb4
L56bb6:
		fstp st(0)		// 0x00056bb6
L56bb8:
		fld dword ptr [esp + 0x10]		// 0x00056bb8
		mov ebp, dword ptr [esp + 0x30]		// 0x00056bbc
		fcomp dword ptr [ebp]		// 0x00056bc0
		fnstsw ax		// 0x00056bc3
		test ah, 5		// 0x00056bc5
		jp L56bd1		// 0x00056bc8
		mov eax, dword ptr [esp + 0x10]		// 0x00056bca
		mov dword ptr [ebp], eax		// 0x00056bce
L56bd1:
		mov eax, dword ptr [esp + 0x2c]		// 0x00056bd1
		fld dword ptr [eax]		// 0x00056bd5
		fcomp dword ptr [ebp]		// 0x00056bd7
		fnstsw ax		// 0x00056bda
		test ah, 0x41		// 0x00056bdc
		je L56b4c		// 0x00056bdf
		fld dword ptr [ebp]		// 0x00056be5
		fcomp dword ptr [nxP7K10107a0c_4]		// 0x00056be8
		fnstsw ax		// 0x00056bee
		test ah, 5		// 0x00056bf0
		jnp L56b4c		// 0x00056bf3
L56bf9:
		inc esi		// 0x00056bf9
		add ecx, 4		// 0x00056bfa
		cmp esi, 3		// 0x00056bfd
		jl L56b10		// 0x00056c00
		mov ecx, dword ptr [esp + 0x2c]		// 0x00056c06
		fld dword ptr [ecx]		// 0x00056c0a
		mov ecx, dword ptr [esp + 0x30]		// 0x00056c0c
		fcomp dword ptr [ecx]		// 0x00056c10
		fnstsw ax		// 0x00056c12
		test ah, 0x41		// 0x00056c14
		je L56b4c		// 0x00056c17
		fld dword ptr [ecx]		// 0x00056c1d
		fcomp dword ptr [nxP7K10107a0c_4]		// 0x00056c1f
		fnstsw ax		// 0x00056c25
		test ah, 5		// 0x00056c27
		jnp L56b4c		// 0x00056c2a
		mov eax, dword ptr [esp + 0x14]		// 0x00056c30
		pop edi		// 0x00056c34
		pop esi		// 0x00056c35
		pop ebp		// 0x00056c36
		pop ebx		// 0x00056c37
		add esp, 0xc		// 0x00056c38
		ret		// 0x00056c3b
		}
	}

// phys_fn_002276 (0x00056c40, 855 B), 277 instructions
__declspec(naked) unsigned __fastcall nxP72276(void* self, void* edxUnused)
	{
	__asm
		{
		sub esp, 0x38		// 0x00056c40
		mov eax, dword ptr [esp + 0x3c]		// 0x00056c43
		fld dword ptr [eax]		// 0x00056c47
		push ebx		// 0x00056c49
		fsub dword ptr [esi]		// 0x00056c4a
		push ebp		// 0x00056c4c
		fstp dword ptr [esp + 0x18]		// 0x00056c4d
		fld dword ptr [eax + 4]		// 0x00056c51
		fsub dword ptr [esi + 4]		// 0x00056c54
		fstp dword ptr [esp + 0x1c]		// 0x00056c57
		fld dword ptr [eax + 8]		// 0x00056c5b
		fsub dword ptr [esi + 8]		// 0x00056c5e
		fstp dword ptr [esp + 0x20]		// 0x00056c61
		fld dword ptr [esp + 0x1c]		// 0x00056c65
		fmul dword ptr [ecx + 8]		// 0x00056c69
		fld dword ptr [esp + 0x20]		// 0x00056c6c
		fmul dword ptr [ecx + 4]		// 0x00056c70
		fsubp st(1), st(0)		// 0x00056c73
		fstp dword ptr [esp + 0xc]		// 0x00056c75
		mov ebp, dword ptr [esp + 0xc]		// 0x00056c79
		fld dword ptr [esp + 0x20]		// 0x00056c7d
		mov dword ptr [esp + 0x30], ebp		// 0x00056c81
		fmul dword ptr [ecx]		// 0x00056c85
		fld dword ptr [esp + 0x18]		// 0x00056c87
		fmul dword ptr [ecx + 8]		// 0x00056c8b
		fsubp st(1), st(0)		// 0x00056c8e
		fstp dword ptr [esp + 0x10]		// 0x00056c90
		mov ebx, dword ptr [esp + 0x10]		// 0x00056c94
		fld dword ptr [esp + 0x18]		// 0x00056c98
		mov dword ptr [esp + 0x34], ebx		// 0x00056c9c
		fmul dword ptr [ecx + 4]		// 0x00056ca0
		fld dword ptr [esp + 0x1c]		// 0x00056ca3
		fmul dword ptr [ecx]		// 0x00056ca7
		fsubp st(1), st(0)		// 0x00056ca9
		fstp dword ptr [esp + 0x14]		// 0x00056cab
		mov eax, dword ptr [esp + 0x14]		// 0x00056caf
		mov dword ptr [esp + 0x38], eax		// 0x00056cb3
		fld dword ptr [esp + 0x38]		// 0x00056cb7
		fmul dword ptr [esp + 0x38]		// 0x00056cbb
		fld dword ptr [esp + 0x34]		// 0x00056cbf
		fmul dword ptr [esp + 0x34]		// 0x00056cc3
		faddp st(1), st(0)		// 0x00056cc7
		fld dword ptr [esp + 0x30]		// 0x00056cc9
		fmul dword ptr [esp + 0x30]		// 0x00056ccd
		faddp st(1), st(0)		// 0x00056cd1
		fld dword ptr [nxP7K101041f0_4]		// 0x00056cd3
		fld st(1)		// 0x00056cd9
		fucompp		// 0x00056cdb
		fnstsw ax		// 0x00056cdd
		test ah, 0x44		// 0x00056cdf
		jnp L56d12		// 0x00056ce2
		fsqrt		// 0x00056ce4
		fdivr dword ptr [nxP7K101041ec_4]		// 0x00056ce6
		fld dword ptr [esp + 0x30]		// 0x00056cec
		fmul st(0), st(1)		// 0x00056cf0
		fstp dword ptr [esp + 0x30]		// 0x00056cf2
		mov ebp, dword ptr [esp + 0x30]		// 0x00056cf6
		fld dword ptr [esp + 0x34]		// 0x00056cfa
		fmul st(0), st(1)		// 0x00056cfe
		fstp dword ptr [esp + 0x34]		// 0x00056d00
		mov ebx, dword ptr [esp + 0x34]		// 0x00056d04
		fld dword ptr [esp + 0x38]		// 0x00056d08
		fmul st(0), st(1)		// 0x00056d0c
		fstp dword ptr [esp + 0x38]		// 0x00056d0e
L56d12:
		fstp st(0)		// 0x00056d12
		mov eax, dword ptr [esp + 0x48]		// 0x00056d14
		fld dword ptr [esp + 0x30]		// 0x00056d18
		fmul dword ptr [esi]		// 0x00056d1c
		fld dword ptr [esp + 0x34]		// 0x00056d1e
		fmul dword ptr [esi + 4]		// 0x00056d22
		faddp st(1), st(0)		// 0x00056d25
		fld dword ptr [esp + 0x38]		// 0x00056d27
		fmul dword ptr [esi + 8]		// 0x00056d2b
		faddp st(1), st(0)		// 0x00056d2e
		fchs		// 0x00056d30
		fstp dword ptr [esp + 0x3c]		// 0x00056d32
		fld dword ptr [esp + 0x30]		// 0x00056d36
		fmul dword ptr [edx]		// 0x00056d3a
		fstp dword ptr [esp + 8]		// 0x00056d3c
		fld dword ptr [esp + 0x38]		// 0x00056d40
		fmul dword ptr [eax + 8]		// 0x00056d44
		fld dword ptr [esp + 0x34]		// 0x00056d47
		fmul dword ptr [eax + 4]		// 0x00056d4b
		faddp st(1), st(0)		// 0x00056d4e
		fld dword ptr [esp + 0x30]		// 0x00056d50
		fmul dword ptr [eax]		// 0x00056d54
		faddp st(1), st(0)		// 0x00056d56
		fadd dword ptr [esp + 0x3c]		// 0x00056d58
		fld dword ptr [esp + 0x38]		// 0x00056d5c
		fmul dword ptr [edx + 8]		// 0x00056d60
		fld dword ptr [esp + 0x34]		// 0x00056d63
		fmul dword ptr [edx + 4]		// 0x00056d67
		faddp st(1), st(0)		// 0x00056d6a
		fadd dword ptr [esp + 8]		// 0x00056d6c
		fadd dword ptr [esp + 0x3c]		// 0x00056d70
		fmulp st(1), st(0)		// 0x00056d74
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00056d76
		fnstsw ax		// 0x00056d7c
		test ah, 0x41		// 0x00056d7e
		je L56e1b		// 0x00056d81
		mov eax, dword ptr [esp + 0x48]		// 0x00056d87
		fld dword ptr [eax]		// 0x00056d8b
		fsub dword ptr [edx]		// 0x00056d8d
		fld dword ptr [eax + 4]		// 0x00056d8f
		fsub dword ptr [edx + 4]		// 0x00056d92
		fstp dword ptr [esp + 0x10]		// 0x00056d95
		fld dword ptr [eax + 8]		// 0x00056d99
		fsub dword ptr [edx + 8]		// 0x00056d9c
		fst dword ptr [esp + 0x14]		// 0x00056d9f
		fmul dword ptr [esp + 0x14]		// 0x00056da3
		fld dword ptr [esp + 0x10]		// 0x00056da7
		fmul dword ptr [esp + 0x10]		// 0x00056dab
		faddp st(1), st(0)		// 0x00056daf
		fld st(1)		// 0x00056db1
		fmul st(0), st(2)		// 0x00056db3
		faddp st(1), st(0)		// 0x00056db5
		fld dword ptr [nxP7K101041f0_4]		// 0x00056db7
		fld st(1)		// 0x00056dbd
		fucompp		// 0x00056dbf
		fnstsw ax		// 0x00056dc1
		test ah, 0x44		// 0x00056dc3
		jnp L56dea		// 0x00056dc6
		fsqrt		// 0x00056dc8
		fdivr dword ptr [nxP7K101041ec_4]		// 0x00056dca
		fxch st(1)		// 0x00056dd0
		fmul st(0), st(1)		// 0x00056dd2
		fxch st(1)		// 0x00056dd4
		fld dword ptr [esp + 0x10]		// 0x00056dd6
		fmul st(0), st(1)		// 0x00056dda
		fstp dword ptr [esp + 0x10]		// 0x00056ddc
		fld dword ptr [esp + 0x14]		// 0x00056de0
		fmul st(0), st(1)		// 0x00056de4
		fstp dword ptr [esp + 0x14]		// 0x00056de6
L56dea:
		fstp st(0)		// 0x00056dea
		fld dword ptr [esp + 0x14]		// 0x00056dec
		fmul dword ptr [esp + 0x38]		// 0x00056df0
		fld dword ptr [esp + 0x10]		// 0x00056df4
		fmul dword ptr [esp + 0x34]		// 0x00056df8
		faddp st(1), st(0)		// 0x00056dfc
		fld st(1)		// 0x00056dfe
		fmul dword ptr [esp + 0x30]		// 0x00056e00
		faddp st(1), st(0)		// 0x00056e04
		fld dword ptr [nxP7K101041f0_4]		// 0x00056e06
		fld st(1)		// 0x00056e0c
		fucompp		// 0x00056e0e
		fnstsw ax		// 0x00056e10
		test ah, 0x44		// 0x00056e12
		jp L56e23		// 0x00056e15
		fstp st(0)		// 0x00056e17
L56e19:
		fstp st(0)		// 0x00056e19
L56e1b:
		pop ebp		// 0x00056e1b
		xor eax, eax		// 0x00056e1c
		pop ebx		// 0x00056e1e
		add esp, 0x38		// 0x00056e1f
		ret		// 0x00056e22
L56e23:
		fld dword ptr [esp + 0x38]		// 0x00056e23
		and ebx, 0x7fffffff		// 0x00056e27
		fmul dword ptr [edx + 8]		// 0x00056e2d
		and ebp, 0x7fffffff		// 0x00056e30
		fld dword ptr [esp + 0x34]		// 0x00056e36
		fmul dword ptr [edx + 4]		// 0x00056e3a
		faddp st(1), st(0)		// 0x00056e3d
		fadd dword ptr [esp + 8]		// 0x00056e3f
		fadd dword ptr [esp + 0x3c]		// 0x00056e43
		fdiv st(0), st(1)		// 0x00056e47
		fstp dword ptr [esp + 0x48]		// 0x00056e49
		fstp st(0)		// 0x00056e4d
		fmul dword ptr [esp + 0x48]		// 0x00056e4f
		fld dword ptr [esp + 0x10]		// 0x00056e53
		fmul dword ptr [esp + 0x48]		// 0x00056e57
		fld dword ptr [esp + 0x14]		// 0x00056e5b
		fmul dword ptr [esp + 0x48]		// 0x00056e5f
		fstp dword ptr [esp + 0x2c]		// 0x00056e63
		fld dword ptr [edx]		// 0x00056e67
		fsub st(0), st(2)		// 0x00056e69
		fstp dword ptr [esp + 0xc]		// 0x00056e6b
		mov eax, dword ptr [esp + 0xc]		// 0x00056e6f
		fld dword ptr [edx + 4]		// 0x00056e73
		fsub st(0), st(1)		// 0x00056e76
		fstp dword ptr [esp + 0x10]		// 0x00056e78
		fstp st(0)		// 0x00056e7c
		fstp st(0)		// 0x00056e7e
		fld dword ptr [edx + 8]		// 0x00056e80
		mov edx, edi		// 0x00056e83
		fsub dword ptr [esp + 0x2c]		// 0x00056e85
		mov dword ptr [edx], eax		// 0x00056e89
		mov eax, dword ptr [esp + 0x10]		// 0x00056e8b
		mov dword ptr [edx + 4], eax		// 0x00056e8f
		fstp dword ptr [esp + 0x14]		// 0x00056e92
		mov eax, dword ptr [esp + 0x14]		// 0x00056e96
		mov dword ptr [edx + 8], eax		// 0x00056e9a
		xor eax, eax		// 0x00056e9d
		cmp ebx, ebp		// 0x00056e9f
		jbe L56ea8		// 0x00056ea1
		mov eax, 1		// 0x00056ea3
L56ea8:
		mov ebx, dword ptr [esp + eax*4 + 0x30]		// 0x00056ea8
		mov edx, dword ptr [esp + 0x38]		// 0x00056eac
		and ebx, 0x7fffffff		// 0x00056eb0
		and edx, 0x7fffffff		// 0x00056eb6
		cmp edx, ebx		// 0x00056ebc
		ja L56ed5		// 0x00056ebe
		test eax, eax		// 0x00056ec0
		mov edx, 2		// 0x00056ec2
		jne L56ed0		// 0x00056ec7
		mov eax, 1		// 0x00056ec9
		jmp L56edc		// 0x00056ece
L56ed0:
		cmp eax, 1		// 0x00056ed0
		je L56eda		// 0x00056ed3
L56ed5:
		mov edx, 1		// 0x00056ed5
L56eda:
		xor eax, eax		// 0x00056eda
L56edc:
		fld dword ptr [edi + edx*4]		// 0x00056edc
		lea ebx, [esp + edx*4 + 0x18]		// 0x00056edf
		fsub dword ptr [esi + edx*4]		// 0x00056ee3
		fmul dword ptr [esp + eax*4 + 0x18]		// 0x00056ee6
		fld dword ptr [edi + eax*4]		// 0x00056eea
		fsub dword ptr [esi + eax*4]		// 0x00056eed
		fmul dword ptr [ebx]		// 0x00056ef0
		fsubp st(1), st(0)		// 0x00056ef2
		fld dword ptr [ecx + edx*4]		// 0x00056ef4
		mov edx, dword ptr [esp + 0x4c]		// 0x00056ef7
		fmul dword ptr [esp + eax*4 + 0x18]		// 0x00056efb
		fld dword ptr [ecx + eax*4]		// 0x00056eff
		fmul dword ptr [ebx]		// 0x00056f02
		fsubp st(1), st(0)		// 0x00056f04
		fdivp st(1), st(0)		// 0x00056f06
		fld st(0)		// 0x00056f08
		fstp dword ptr [edx]		// 0x00056f0a
		fcom dword ptr [nxP7K101041f0_4]		// 0x00056f0c
		fnstsw ax		// 0x00056f12
		test ah, 5		// 0x00056f14
		jnp L56e19		// 0x00056f17
		fld st(0)		// 0x00056f1d
		mov eax, dword ptr [esp + 0x44]		// 0x00056f1f
		fmul dword ptr [ecx]		// 0x00056f23
		fld st(1)		// 0x00056f25
		fmul dword ptr [ecx + 4]		// 0x00056f27
		fstp dword ptr [esp + 0x1c]		// 0x00056f2a
		fxch st(1)		// 0x00056f2e
		fmul dword ptr [ecx + 8]		// 0x00056f30
		fstp dword ptr [esp + 0x20]		// 0x00056f33
		fsubr dword ptr [edi]		// 0x00056f37
		fst dword ptr [edi]		// 0x00056f39
		fld dword ptr [edi + 4]		// 0x00056f3b
		fsub dword ptr [esp + 0x1c]		// 0x00056f3e
		fst dword ptr [edi + 4]		// 0x00056f42
		fld dword ptr [edi + 8]		// 0x00056f45
		fsub dword ptr [esp + 0x20]		// 0x00056f48
		fst dword ptr [edi + 8]		// 0x00056f4c
		fld dword ptr [eax + 8]		// 0x00056f4f
		fsub st(0), st(1)		// 0x00056f52
		fld dword ptr [esi + 8]		// 0x00056f54
		fsub st(0), st(2)		// 0x00056f57
		fmulp st(1), st(0)		// 0x00056f59
		fld dword ptr [esi]		// 0x00056f5b
		fsub st(0), st(4)		// 0x00056f5d
		fld dword ptr [eax]		// 0x00056f5f
		fsub st(0), st(5)		// 0x00056f61
		fmulp st(1), st(0)		// 0x00056f63
		faddp st(1), st(0)		// 0x00056f65
		fld dword ptr [eax + 4]		// 0x00056f67
		fsub st(0), st(3)		// 0x00056f6a
		fld dword ptr [esi + 4]		// 0x00056f6c
		fsub st(0), st(4)		// 0x00056f6f
		fmulp st(1), st(0)		// 0x00056f71
		faddp st(1), st(0)		// 0x00056f73
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00056f75
		fstp st(0)		// 0x00056f7b
		fnstsw ax		// 0x00056f7d
		fstp st(0)		// 0x00056f7f
		test ah, 5		// 0x00056f81
		fstp st(0)		// 0x00056f84
		jp L56e1b		// 0x00056f86
		pop ebp		// 0x00056f8c
		mov eax, 1		// 0x00056f8d
		pop ebx		// 0x00056f92
		add esp, 0x38		// 0x00056f93
		ret		// 0x00056f96
		}
	}

// phys_fn_002278 (0x00056fa0, 754 B), 232 instructions
__declspec(naked) unsigned __fastcall nxP72278(void* self, void* edxUnused)
	{
	__asm
		{
		sub esp, 0x30		// 0x00056fa0
		fld dword ptr [esi]		// 0x00056fa3
		mov edx, dword ptr [esp + 0x38]		// 0x00056fa5
		fsub dword ptr [ecx]		// 0x00056fa9
		push ebp		// 0x00056fab
		mov ebp, dword ptr [esp + 0x44]		// 0x00056fac
		push edi		// 0x00056fb0
		fstp dword ptr [esp + 8]		// 0x00056fb1
		mov edi, dword ptr [esp + 0x3c]		// 0x00056fb5
		fld dword ptr [esi + 4]		// 0x00056fb9
		fsub dword ptr [ecx + 4]		// 0x00056fbc
		fstp dword ptr [esp + 0xc]		// 0x00056fbf
		fld dword ptr [esi + 8]		// 0x00056fc3
		fsub dword ptr [ecx + 8]		// 0x00056fc6
		fstp dword ptr [esp + 0x10]		// 0x00056fc9
		fld dword ptr [eax]		// 0x00056fcd
		fsub dword ptr [ecx]		// 0x00056fcf
		fstp dword ptr [esp + 0x14]		// 0x00056fd1
		fld dword ptr [eax + 4]		// 0x00056fd5
		fsub dword ptr [ecx + 4]		// 0x00056fd8
		fstp dword ptr [esp + 0x18]		// 0x00056fdb
		fld dword ptr [eax + 8]		// 0x00056fdf
		mov al, byte ptr [esp + 0x4c]		// 0x00056fe2
		test al, al		// 0x00056fe6
		fsub dword ptr [ecx + 8]		// 0x00056fe8
		fst dword ptr [esp + 0x1c]		// 0x00056feb
		fmul dword ptr [edx + 4]		// 0x00056fef
		fld dword ptr [esp + 0x18]		// 0x00056ff2
		fmul dword ptr [edx + 8]		// 0x00056ff6
		fsubp st(1), st(0)		// 0x00056ff9
		fstp dword ptr [esp + 0x2c]		// 0x00056ffb
		fld dword ptr [esp + 0x14]		// 0x00056fff
		fmul dword ptr [edx + 8]		// 0x00057003
		fld dword ptr [esp + 0x1c]		// 0x00057006
		fmul dword ptr [edx]		// 0x0005700a
		fsubp st(1), st(0)		// 0x0005700c
		fstp dword ptr [esp + 0x30]		// 0x0005700e
		fld dword ptr [esp + 0x18]		// 0x00057012
		fmul dword ptr [edx]		// 0x00057016
		fld dword ptr [esp + 0x14]		// 0x00057018
		fmul dword ptr [edx + 4]		// 0x0005701c
		fsubp st(1), st(0)		// 0x0005701f
		fst dword ptr [esp + 0x34]		// 0x00057021
		fmul dword ptr [esp + 0x10]		// 0x00057025
		fld dword ptr [esp + 0x30]		// 0x00057029
		fmul dword ptr [esp + 0xc]		// 0x0005702d
		faddp st(1), st(0)		// 0x00057031
		fld dword ptr [esp + 0x2c]		// 0x00057033
		fmul dword ptr [esp + 8]		// 0x00057037
		faddp st(1), st(0)		// 0x0005703b
		fstp dword ptr [esp + 0x40]		// 0x0005703d
		fld dword ptr [esp + 0x40]		// 0x00057041
		je L57162		// 0x00057045
		fcomp dword ptr [nxP7K10106880_4]		// 0x0005704b
		fnstsw ax		// 0x00057051
		test ah, 5		// 0x00057053
		jnp L5728a		// 0x00057056
		fld dword ptr [edi]		// 0x0005705c
		fsub dword ptr [ecx]		// 0x0005705e
		fstp dword ptr [esp + 0x20]		// 0x00057060
		fld dword ptr [edi + 4]		// 0x00057064
		fsub dword ptr [ecx + 4]		// 0x00057067
		fstp dword ptr [esp + 0x24]		// 0x0005706a
		fld dword ptr [edi + 8]		// 0x0005706e
		fsub dword ptr [ecx + 8]		// 0x00057071
		fst dword ptr [esp + 0x28]		// 0x00057074
		fmul dword ptr [esp + 0x34]		// 0x00057078
		fld dword ptr [esp + 0x24]		// 0x0005707c
		fmul dword ptr [esp + 0x30]		// 0x00057080
		faddp st(1), st(0)		// 0x00057084
		fld dword ptr [esp + 0x20]		// 0x00057086
		fmul dword ptr [esp + 0x2c]		// 0x0005708a
		faddp st(1), st(0)		// 0x0005708e
		fst dword ptr [ebx]		// 0x00057090
		mov eax, dword ptr [ebx]		// 0x00057092
		test eax, eax		// 0x00057094
		js L57288		// 0x00057096
		fcomp dword ptr [esp + 0x40]		// 0x0005709c
		fnstsw ax		// 0x000570a0
		test ah, 0x41		// 0x000570a2
		je L5728a		// 0x000570a5
		fld dword ptr [esp + 0x24]		// 0x000570ab
		fmul dword ptr [esp + 0x10]		// 0x000570af
		fld dword ptr [esp + 0x28]		// 0x000570b3
		fmul dword ptr [esp + 0xc]		// 0x000570b7
		fsubp st(1), st(0)		// 0x000570bb
		fstp dword ptr [esp + 0x2c]		// 0x000570bd
		fld dword ptr [esp + 0x28]		// 0x000570c1
		fmul dword ptr [esp + 8]		// 0x000570c5
		fld dword ptr [esp + 0x20]		// 0x000570c9
		fmul dword ptr [esp + 0x10]		// 0x000570cd
		fsubp st(1), st(0)		// 0x000570d1
		fstp dword ptr [esp + 0x30]		// 0x000570d3
		fld dword ptr [esp + 0x20]		// 0x000570d7
		fmul dword ptr [esp + 0xc]		// 0x000570db
		fld dword ptr [esp + 0x24]		// 0x000570df
		fmul dword ptr [esp + 8]		// 0x000570e3
		fsubp st(1), st(0)		// 0x000570e7
		fst dword ptr [esp + 0x34]		// 0x000570e9
		fmul dword ptr [edx + 8]		// 0x000570ed
		fld dword ptr [esp + 0x30]		// 0x000570f0
		fmul dword ptr [edx + 4]		// 0x000570f4
		faddp st(1), st(0)		// 0x000570f7
		fld dword ptr [esp + 0x2c]		// 0x000570f9
		fmul dword ptr [edx]		// 0x000570fd
		faddp st(1), st(0)		// 0x000570ff
		fst dword ptr [ebp]		// 0x00057101
		mov eax, dword ptr [ebp]		// 0x00057104
		test eax, eax		// 0x00057107
		js L57288		// 0x00057109
		fadd dword ptr [ebx]		// 0x0005710f
		fcomp dword ptr [esp + 0x40]		// 0x00057111
		fnstsw ax		// 0x00057115
		test ah, 0x41		// 0x00057117
		je L5728a		// 0x0005711a
		fld dword ptr [nxP7K101041ec_4]		// 0x00057120
		mov eax, dword ptr [esp + 0x44]		// 0x00057126
		fdiv dword ptr [esp + 0x40]		// 0x0005712a
		pop edi		// 0x0005712e
		fld dword ptr [esp + 0x30]		// 0x0005712f
		fmul dword ptr [esp + 0x18]		// 0x00057133
		fld dword ptr [esp + 0x2c]		// 0x00057137
		fmul dword ptr [esp + 0x14]		// 0x0005713b
		faddp st(1), st(0)		// 0x0005713f
		fld dword ptr [esp + 0x28]		// 0x00057141
		fmul dword ptr [esp + 0x10]		// 0x00057145
		faddp st(1), st(0)		// 0x00057149
		fmul st(0), st(1)		// 0x0005714b
		fstp dword ptr [eax]		// 0x0005714d
		mov al, 1		// 0x0005714f
		fld st(0)		// 0x00057151
		fmul dword ptr [ebx]		// 0x00057153
		fstp dword ptr [ebx]		// 0x00057155
		fmul dword ptr [ebp]		// 0x00057157
		fstp dword ptr [ebp]		// 0x0005715a
		pop ebp		// 0x0005715d
		add esp, 0x30		// 0x0005715e
		ret		// 0x00057161
L57162:
		fcomp dword ptr [nxP7K101079e8_4]		// 0x00057162
		fnstsw ax		// 0x00057168
		test ah, 0x41		// 0x0005716a
		jne L57184		// 0x0005716d
		fld dword ptr [esp + 0x40]		// 0x0005716f
		fcomp dword ptr [nxP7K10106880_4]		// 0x00057173
		fnstsw ax		// 0x00057179
		test ah, 5		// 0x0005717b
		jnp L5728a		// 0x0005717e
L57184:
		fld dword ptr [nxP7K101041ec_4]		// 0x00057184
		fdiv dword ptr [esp + 0x40]		// 0x0005718a
		fstp dword ptr [esp + 0x4c]		// 0x0005718e
		fld dword ptr [edi]		// 0x00057192
		fsub dword ptr [ecx]		// 0x00057194
		fstp dword ptr [esp + 0x20]		// 0x00057196
		fld dword ptr [edi + 4]		// 0x0005719a
		fsub dword ptr [ecx + 4]		// 0x0005719d
		fstp dword ptr [esp + 0x24]		// 0x000571a0
		fld dword ptr [edi + 8]		// 0x000571a4
		fsub dword ptr [ecx + 8]		// 0x000571a7
		fst dword ptr [esp + 0x28]		// 0x000571aa
		fmul dword ptr [esp + 0x34]		// 0x000571ae
		fld dword ptr [esp + 0x24]		// 0x000571b2
		fmul dword ptr [esp + 0x30]		// 0x000571b6
		faddp st(1), st(0)		// 0x000571ba
		fld dword ptr [esp + 0x20]		// 0x000571bc
		fmul dword ptr [esp + 0x2c]		// 0x000571c0
		faddp st(1), st(0)		// 0x000571c4
		fmul dword ptr [esp + 0x4c]		// 0x000571c6
		fst dword ptr [ebx]		// 0x000571ca
		mov eax, dword ptr [ebx]		// 0x000571cc
		test eax, eax		// 0x000571ce
		js L57288		// 0x000571d0
		fcomp dword ptr [nxP7K101041ec_4]		// 0x000571d6
		fnstsw ax		// 0x000571dc
		test ah, 0x41		// 0x000571de
		je L5728a		// 0x000571e1
		fld dword ptr [esp + 0x24]		// 0x000571e7
		fmul dword ptr [esp + 0x10]		// 0x000571eb
		fld dword ptr [esp + 0x28]		// 0x000571ef
		fmul dword ptr [esp + 0xc]		// 0x000571f3
		fsubp st(1), st(0)		// 0x000571f7
		fstp dword ptr [esp + 0x2c]		// 0x000571f9
		fld dword ptr [esp + 0x28]		// 0x000571fd
		fmul dword ptr [esp + 8]		// 0x00057201
		fld dword ptr [esp + 0x20]		// 0x00057205
		fmul dword ptr [esp + 0x10]		// 0x00057209
		fsubp st(1), st(0)		// 0x0005720d
		fstp dword ptr [esp + 0x30]		// 0x0005720f
		fld dword ptr [esp + 0x20]		// 0x00057213
		fmul dword ptr [esp + 0xc]		// 0x00057217
		fld dword ptr [esp + 0x24]		// 0x0005721b
		fmul dword ptr [esp + 8]		// 0x0005721f
		fsubp st(1), st(0)		// 0x00057223
		fst dword ptr [esp + 0x34]		// 0x00057225
		fmul dword ptr [edx + 8]		// 0x00057229
		fld dword ptr [esp + 0x30]		// 0x0005722c
		fmul dword ptr [edx + 4]		// 0x00057230
		faddp st(1), st(0)		// 0x00057233
		fld dword ptr [esp + 0x2c]		// 0x00057235
		fmul dword ptr [edx]		// 0x00057239
		faddp st(1), st(0)		// 0x0005723b
		fmul dword ptr [esp + 0x4c]		// 0x0005723d
		fst dword ptr [ebp]		// 0x00057241
		mov eax, dword ptr [ebp]		// 0x00057244
		test eax, eax		// 0x00057247
		js L57288		// 0x00057249
		fadd dword ptr [ebx]		// 0x0005724b
		fcomp dword ptr [nxP7K101041ec_4]		// 0x0005724d
		fnstsw ax		// 0x00057253
		test ah, 0x41		// 0x00057255
		je L5728a		// 0x00057258
		fld dword ptr [esp + 0x34]		// 0x0005725a
		mov ecx, dword ptr [esp + 0x44]		// 0x0005725e
		fmul dword ptr [esp + 0x1c]		// 0x00057262
		pop edi		// 0x00057266
		fld dword ptr [esp + 0x2c]		// 0x00057267
		mov al, 1		// 0x0005726b
		fmul dword ptr [esp + 0x14]		// 0x0005726d
		pop ebp		// 0x00057271
		faddp st(1), st(0)		// 0x00057272
		fld dword ptr [esp + 0x24]		// 0x00057274
		fmul dword ptr [esp + 0xc]		// 0x00057278
		faddp st(1), st(0)		// 0x0005727c
		fmul dword ptr [esp + 0x44]		// 0x0005727e
		fstp dword ptr [ecx]		// 0x00057282
		add esp, 0x30		// 0x00057284
		ret		// 0x00057287
L57288:
		fstp st(0)		// 0x00057288
L5728a:
		pop edi		// 0x0005728a
		xor al, al		// 0x0005728b
		pop ebp		// 0x0005728d
		add esp, 0x30		// 0x0005728e
		ret		// 0x00057291
		}
	}

// phys_fn_002280 (0x000572a0, 139 B), 45 instructions
__declspec(naked) unsigned __fastcall nxP72280(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		sub esp, 0x30		// 0x000572a0
		mov eax, dword ptr [nxP7SweepCache]		// 0x000572a3
		fld dword ptr [eax + 0xc]		// 0x000572a8
		push esi		// 0x000572ab
		fld dword ptr [eax + 0xc]		// 0x000572ac
		mov eax, dword ptr [eax + 0xc]		// 0x000572af
		fxch st(1)		// 0x000572b2
		mov dword ptr [esp + 0x18], eax		// 0x000572b4
		fadd dword ptr [ecx + 0x10]		// 0x000572b8
		mov eax, dword ptr [esp + 0x3c]		// 0x000572bb
		lea esi, [ecx + 4]		// 0x000572bf
		lea edx, [esp + 0x10]		// 0x000572c2
		fstp dword ptr [esp + 4]		// 0x000572c6
		fadd dword ptr [ecx + 0x14]		// 0x000572ca
		fstp dword ptr [esp + 8]		// 0x000572cd
		fld dword ptr [esp + 0x18]		// 0x000572d1
		fadd dword ptr [ecx + 0x18]		// 0x000572d5
		lea ecx, [esp + 4]		// 0x000572d8
		push ecx		// 0x000572dc
		push edx		// 0x000572dd
		fstp dword ptr [esp + 0x14]		// 0x000572de
		lea ecx, [esp + 0x24]		// 0x000572e2
		fld dword ptr [eax]		// 0x000572e6
		fadd dword ptr [esi]		// 0x000572e8
		fstp dword ptr [esp + 0x18]		// 0x000572ea
		fld dword ptr [esi + 4]		// 0x000572ee
		fadd dword ptr [eax + 4]		// 0x000572f1
		fstp dword ptr [esp + 0x1c]		// 0x000572f4
		fld dword ptr [esi + 8]		// 0x000572f8
		fadd dword ptr [eax + 8]		// 0x000572fb
		fstp dword ptr [esp + 0x20]		// 0x000572fe
		call nxP7CallNxPOSetCenterExtents		// 0x00057302
		lea eax, [esp + 4]		// 0x00057307
		push eax		// 0x0005730b
		push esi		// 0x0005730c
		mov esi, dword ptr [esp + 0x40]		// 0x0005730d
		mov ecx, esi		// 0x00057311
		call nxP7CallNxPOSetCenterExtents		// 0x00057313
		lea ecx, [esp + 0x1c]		// 0x00057318
		push ecx		// 0x0005731c
		mov ecx, esi		// 0x0005731d
		call nxP7CallAdd		// 0x0005731f
		pop esi		// 0x00057324
		add esp, 0x30		// 0x00057325
		ret 8		// 0x00057328
		}
	}

// phys_fn_002282 and phys_fn_002284 and phys_fn_002286 (one function) (0x00057330, 2355 B), 657 instructions
__declspec(naked) unsigned __cdecl nxP72282()
	{
	__asm
		{
		mov ecx, dword ptr [esp + 4]		// 0x00057330
		mov eax, dword ptr [esp + 8]		// 0x00057334
		sub esp, 0x330		// 0x00057338
		push ebp		// 0x0005733e
		push esi		// 0x0005733f
		push edi		// 0x00057340
		push 1		// 0x00057341
		lea ebp, [ecx + 0xc]		// 0x00057343
		push ecx		// 0x00057346
		lea esi, [ecx + 0x18]		// 0x00057347
		lea edx, [eax + 0x18]		// 0x0005734a
		lea edi, [eax + 0xc]		// 0x0005734d
		push ebp		// 0x00057350
		mov ecx, esi		// 0x00057351
		call nxP72268		// 0x00057353
		add esp, 0xc		// 0x00057358
		test al, al		// 0x0005735b
		je L57379		// 0x0005735d
		mov eax, dword ptr [esp + 0x348]		// 0x0005735f
		pop edi		// 0x00057366
		pop esi		// 0x00057367
		mov dword ptr [eax + 0x18], 0		// 0x00057368
		mov al, 1		// 0x0005736f
		pop ebp		// 0x00057371
		add esp, 0x330		// 0x00057372
		ret		// 0x00057378
L57379:
		lea ecx, [esp + 0x1bc]		// 0x00057379
		push ecx		// 0x00057380
		mov ecx, dword ptr [esp + 0x344]		// 0x00057381
		call nxP7CallComputePoints		// 0x00057388
		mov ecx, dword ptr [esp + 0x344]		// 0x0005738d
		lea edx, [esp + 0x15c]		// 0x00057394
		push edx		// 0x0005739b
		call nxP7CallComputePoints		// 0x0005739c
		fld dword ptr [ebp]		// 0x000573a1
		fchs		// 0x000573a4
		mov eax, dword ptr [ebp]		// 0x000573a6
		fstp dword ptr [esp + 0x24]		// 0x000573a9
		mov ecx, dword ptr [ebp + 4]		// 0x000573ad
		fld dword ptr [ebp + 4]		// 0x000573b0
		mov edx, dword ptr [ebp + 8]		// 0x000573b3
		fchs		// 0x000573b6
		mov dword ptr [esp + 0x58], eax		// 0x000573b8
		fstp dword ptr [esp + 0x28]		// 0x000573bc
		lea eax, [esp + 0x11c]		// 0x000573c0
		fld dword ptr [ebp + 8]		// 0x000573c7
		mov dword ptr [esp + 0x5c], ecx		// 0x000573ca
		fchs		// 0x000573ce
		push eax		// 0x000573d0
		fstp dword ptr [esp + 0x30]		// 0x000573d1
		mov ecx, esi		// 0x000573d5
		fld dword ptr [esi + 8]		// 0x000573d7
		mov dword ptr [esp + 0x14], 0x7f7fffff		// 0x000573da
		fmul dword ptr [ebx + 8]		// 0x000573e2
		mov dword ptr [esp + 0x58], 0xffffffff		// 0x000573e5
		fld dword ptr [esi + 4]		// 0x000573ed
		mov dword ptr [esp + 0x64], edx		// 0x000573f0
		fmul dword ptr [ebx + 4]		// 0x000573f4
		faddp st(1), st(0)		// 0x000573f7
		fld dword ptr [esi]		// 0x000573f9
		fmul dword ptr [ebx]		// 0x000573fb
		faddp st(1), st(0)		// 0x000573fd
		fstp dword ptr [esp + 0x18]		// 0x000573ff
		fld dword ptr [esi + 0x14]		// 0x00057403
		fmul dword ptr [ebx + 8]		// 0x00057406
		fld dword ptr [esi + 0x10]		// 0x00057409
		fmul dword ptr [ebx + 4]		// 0x0005740c
		faddp st(1), st(0)		// 0x0005740f
		fld dword ptr [esi + 0xc]		// 0x00057411
		fmul dword ptr [ebx]		// 0x00057414
		faddp st(1), st(0)		// 0x00057416
		fstp dword ptr [esp + 0x1c]		// 0x00057418
		fld dword ptr [esi + 0x20]		// 0x0005741c
		fmul dword ptr [ebx + 8]		// 0x0005741f
		fld dword ptr [esi + 0x1c]		// 0x00057422
		fmul dword ptr [ebx + 4]		// 0x00057425
		faddp st(1), st(0)		// 0x00057428
		fld dword ptr [esi + 0x18]		// 0x0005742a
		fmul dword ptr [ebx]		// 0x0005742d
		faddp st(1), st(0)		// 0x0005742f
		fstp dword ptr [esp + 0x20]		// 0x00057431
		call nxP7CallNxP45404		// 0x00057435
		mov ebp, dword ptr [esp + 0x340]		// 0x0005743a
		mov edx, dword ptr [ebp + 4]		// 0x00057441
		mov eax, dword ptr [ebp + 8]		// 0x00057444
		mov ecx, 0x10		// 0x00057447
		lea esi, [esp + 0x11c]		// 0x0005744c
		lea edi, [esp + 0xd0]		// 0x00057453
		rep movsd		// 0x0005745a
		mov ecx, dword ptr [ebp]		// 0x0005745c
		mov dword ptr [esp + 0x100], ecx		// 0x0005745f
		lea ecx, [esp + 0xd0]		// 0x00057466
		mov dword ptr [esp + 0x104], edx		// 0x0005746d
		push ecx		// 0x00057474
		lea edx, [esp + 0x94]		// 0x00057475
		push edx		// 0x0005747c
		mov dword ptr [esp + 0x110], eax		// 0x0005747d
		call nxP7CallInvertPRMatrix		// 0x00057484
		add esp, 8		// 0x00057489
		fld dword ptr [esp + 0x14]		// 0x0005748c
		fchs		// 0x00057490
		fstp dword ptr [esp + 0x30]		// 0x00057492
		fld dword ptr [esp + 0x18]		// 0x00057496
		fchs		// 0x0005749a
		fstp dword ptr [esp + 0x34]		// 0x0005749c
		fld dword ptr [esp + 0x1c]		// 0x000574a0
		fchs		// 0x000574a4
		fstp dword ptr [esp + 0x38]		// 0x000574a6
		lea esi, [esp + 0x160]		// 0x000574aa
		mov edi, 8		// 0x000574b1
		jmp L574c0		// 0x000574b6
		lea esp, [esp]		// 0x000574b8
		nop		// 0x000574bf
L574c0:
		fld dword ptr [esp + 0x90]		// 0x000574c0
		lea eax, [esp + 0x7c]		// 0x000574c7
		fmul dword ptr [esi - 4]		// 0x000574cb
		push eax		// 0x000574ce
		fld dword ptr [esp + 0xb4]		// 0x000574cf
		lea ecx, [esp + 0x10]		// 0x000574d6
		fmul dword ptr [esi + 4]		// 0x000574da
		push ecx		// 0x000574dd
		lea edx, [esp + 0x6c]		// 0x000574de
		push edx		// 0x000574e2
		faddp st(1), st(0)		// 0x000574e3
		lea eax, [esp + 0x64]		// 0x000574e5
		fld dword ptr [esp + 0xac]		// 0x000574e9
		push eax		// 0x000574f0
		fmul dword ptr [esi]		// 0x000574f1
		lea ecx, [esp + 0x34]		// 0x000574f3
		push ecx		// 0x000574f7
		lea ecx, [esp + 0x44]		// 0x000574f8
		faddp st(1), st(0)		// 0x000574fc
		fadd dword ptr [esp + 0xd4]		// 0x000574fe
		fstp dword ptr [esp + 0x78]		// 0x00057505
		fld dword ptr [esp + 0xa8]		// 0x00057509
		fmul dword ptr [esi - 4]		// 0x00057510
		fld dword ptr [esp + 0xc8]		// 0x00057513
		fmul dword ptr [esi + 4]		// 0x0005751a
		faddp st(1), st(0)		// 0x0005751d
		fld dword ptr [esp + 0xb8]		// 0x0005751f
		fmul dword ptr [esi]		// 0x00057526
		faddp st(1), st(0)		// 0x00057528
		fadd dword ptr [esp + 0xd8]		// 0x0005752a
		fstp dword ptr [esp + 0x7c]		// 0x00057531
		fld dword ptr [esp + 0xac]		// 0x00057535
		fmul dword ptr [esi - 4]		// 0x0005753c
		fld dword ptr [esp + 0xcc]		// 0x0005753f
		fmul dword ptr [esi + 4]		// 0x00057546
		faddp st(1), st(0)		// 0x00057549
		fld dword ptr [esp + 0xbc]		// 0x0005754b
		fmul dword ptr [esi]		// 0x00057552
		faddp st(1), st(0)		// 0x00057554
		fadd dword ptr [esp + 0xdc]		// 0x00057556
		fstp dword ptr [esp + 0x80]		// 0x0005755d
		call nxP7CallNxRayAABBSlab		// 0x00057564
		mov ecx, eax		// 0x00057569
		add esp, 0x14		// 0x0005756b
		cmp ecx, -1		// 0x0005756e
		je L575c7		// 0x00057571
		fld dword ptr [esp + 0xc]		// 0x00057573
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00057577
		fnstsw ax		// 0x0005757d
		test ah, 5		// 0x0005757f
		jnp L575c7		// 0x00057582
		fld dword ptr [esp + 0xc]		// 0x00057584
		fcomp dword ptr [esp + 0x10]		// 0x00057588
		fnstsw ax		// 0x0005758c
		test ah, 5		// 0x0005758e
		jp L575c7		// 0x00057591
		mov edx, dword ptr [esp + 0xc]		// 0x00057593
		lea eax, [ecx + ecx*2]		// 0x00057597
		mov dword ptr [esp + 0x10], edx		// 0x0005759a
		mov edx, dword ptr [esp + 0x348]		// 0x0005759e
		lea ecx, [eax*4 + nxP7Axes]		// 0x000575a5
		mov eax, dword ptr [ecx]		// 0x000575ac
		add edx, 0xc		// 0x000575ae
		mov dword ptr [edx], eax		// 0x000575b1
		mov eax, dword ptr [ecx + 4]		// 0x000575b3
		mov dword ptr [edx + 4], eax		// 0x000575b6
		mov ecx, dword ptr [ecx + 8]		// 0x000575b9
		mov dword ptr [edx + 8], ecx		// 0x000575bc
		mov dword ptr [esp + 0x54], 0		// 0x000575bf
L575c7:
		add esi, 0xc		// 0x000575c7
		dec edi		// 0x000575ca
		jne L574c0		// 0x000575cb
		mov ecx, dword ptr [esp + 0x344]		// 0x000575d1
		fld dword ptr [ecx + 0xc]		// 0x000575d8
		mov edx, dword ptr [ecx + 0xc]		// 0x000575db
		fchs		// 0x000575de
		mov eax, dword ptr [ecx + 0x10]		// 0x000575e0
		fstp dword ptr [esp + 0x14]		// 0x000575e3
		add ecx, 0x18		// 0x000575e7
		fld dword ptr [ecx - 8]		// 0x000575ea
		mov dword ptr [esp + 0x58], edx		// 0x000575ed
		fchs		// 0x000575f1
		mov edx, dword ptr [ecx - 4]		// 0x000575f3
		fstp dword ptr [esp + 0x18]		// 0x000575f6
		mov dword ptr [esp + 0x5c], eax		// 0x000575fa
		fld dword ptr [ecx - 4]		// 0x000575fe
		lea eax, [esp + 0x11c]		// 0x00057601
		fchs		// 0x00057608
		push eax		// 0x0005760a
		fstp dword ptr [esp + 0x20]		// 0x0005760b
		mov dword ptr [esp + 0x64], edx		// 0x0005760f
		fld dword ptr [ecx + 8]		// 0x00057613
		fmul dword ptr [ebx + 8]		// 0x00057616
		fld dword ptr [ecx + 4]		// 0x00057619
		fmul dword ptr [ebx + 4]		// 0x0005761c
		faddp st(1), st(0)		// 0x0005761f
		fld dword ptr [ecx]		// 0x00057621
		fmul dword ptr [ebx]		// 0x00057623
		faddp st(1), st(0)		// 0x00057625
		fstp dword ptr [esp + 0x28]		// 0x00057627
		fld dword ptr [ecx + 0x14]		// 0x0005762b
		fmul dword ptr [ebx + 8]		// 0x0005762e
		fld dword ptr [ecx + 0x10]		// 0x00057631
		fmul dword ptr [ebx + 4]		// 0x00057634
		faddp st(1), st(0)		// 0x00057637
		fld dword ptr [ecx + 0xc]		// 0x00057639
		fmul dword ptr [ebx]		// 0x0005763c
		faddp st(1), st(0)		// 0x0005763e
		fstp dword ptr [esp + 0x2c]		// 0x00057640
		fld dword ptr [ecx + 0x20]		// 0x00057644
		fmul dword ptr [ebx + 8]		// 0x00057647
		fld dword ptr [ecx + 0x1c]		// 0x0005764a
		fmul dword ptr [ebx + 4]		// 0x0005764d
		faddp st(1), st(0)		// 0x00057650
		fld dword ptr [ecx + 0x18]		// 0x00057652
		fmul dword ptr [ebx]		// 0x00057655
		faddp st(1), st(0)		// 0x00057657
		fstp dword ptr [esp + 0x30]		// 0x00057659
		call nxP7CallNxP45404		// 0x0005765d
		mov ecx, 0x10		// 0x00057662
		lea esi, [esp + 0x11c]		// 0x00057667
		lea edi, [esp + 0xd0]		// 0x0005766e
		rep movsd		// 0x00057675
		mov edi, dword ptr [esp + 0x344]		// 0x00057677
		mov ecx, dword ptr [edi]		// 0x0005767e
		mov edx, dword ptr [edi + 4]		// 0x00057680
		mov eax, dword ptr [edi + 8]		// 0x00057683
		mov dword ptr [esp + 0x100], ecx		// 0x00057686
		lea ecx, [esp + 0xd0]		// 0x0005768d
		mov dword ptr [esp + 0x104], edx		// 0x00057694
		push ecx		// 0x0005769b
		lea edx, [esp + 0x94]		// 0x0005769c
		push edx		// 0x000576a3
		mov dword ptr [esp + 0x110], eax		// 0x000576a4
		call nxP7CallInvertPRMatrix		// 0x000576ab
		add esp, 8		// 0x000576b0
		lea esi, [esp + 0x1c0]		// 0x000576b3
		mov dword ptr [esp + 0x20], 8		// 0x000576ba
L576c2:
		fld dword ptr [esp + 0xa0]		// 0x000576c2
		lea eax, [esp + 0x7c]		// 0x000576c9
		fmul dword ptr [esi]		// 0x000576cd
		push eax		// 0x000576cf
		fld dword ptr [esp + 0x94]		// 0x000576d0
		lea ecx, [esp + 0x10]		// 0x000576d7
		fmul dword ptr [esi - 4]		// 0x000576db
		push ecx		// 0x000576de
		lea edx, [esp + 0x6c]		// 0x000576df
		push edx		// 0x000576e3
		faddp st(1), st(0)		// 0x000576e4
		lea eax, [esp + 0x64]		// 0x000576e6
		fld dword ptr [esp + 0xbc]		// 0x000576ea
		push eax		// 0x000576f1
		fmul dword ptr [esi + 4]		// 0x000576f2
		lea ecx, [esp + 0x24]		// 0x000576f5
		push ecx		// 0x000576f9
		lea ecx, [esp + 0x38]		// 0x000576fa
		faddp st(1), st(0)		// 0x000576fe
		fadd dword ptr [esp + 0xd4]		// 0x00057700
		fstp dword ptr [esp + 0x78]		// 0x00057707
		fld dword ptr [esp + 0xb8]		// 0x0005770b
		fmul dword ptr [esi]		// 0x00057712
		fld dword ptr [esp + 0xa8]		// 0x00057714
		fmul dword ptr [esi - 4]		// 0x0005771b
		faddp st(1), st(0)		// 0x0005771e
		fld dword ptr [esp + 0xc8]		// 0x00057720
		fmul dword ptr [esi + 4]		// 0x00057727
		faddp st(1), st(0)		// 0x0005772a
		fadd dword ptr [esp + 0xd8]		// 0x0005772c
		fstp dword ptr [esp + 0x7c]		// 0x00057733
		fld dword ptr [esp + 0xbc]		// 0x00057737
		fmul dword ptr [esi]		// 0x0005773e
		fld dword ptr [esp + 0xac]		// 0x00057740
		fmul dword ptr [esi - 4]		// 0x00057747
		faddp st(1), st(0)		// 0x0005774a
		fld dword ptr [esp + 0xcc]		// 0x0005774c
		fmul dword ptr [esi + 4]		// 0x00057753
		faddp st(1), st(0)		// 0x00057756
		fadd dword ptr [esp + 0xdc]		// 0x00057758
		fstp dword ptr [esp + 0x80]		// 0x0005775f
		call nxP7CallNxRayAABBSlab		// 0x00057766
		mov ecx, eax		// 0x0005776b
		add esp, 0x14		// 0x0005776d
		cmp ecx, -1		// 0x00057770
		je L577c9		// 0x00057773
		fld dword ptr [esp + 0xc]		// 0x00057775
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00057779
		fnstsw ax		// 0x0005777f
		test ah, 5		// 0x00057781
		jnp L577c9		// 0x00057784
		fld dword ptr [esp + 0xc]		// 0x00057786
		fcomp dword ptr [esp + 0x10]		// 0x0005778a
		fnstsw ax		// 0x0005778e
		test ah, 5		// 0x00057790
		jp L577c9		// 0x00057793
		mov edx, dword ptr [esp + 0xc]		// 0x00057795
		lea eax, [ecx + ecx*2]		// 0x00057799
		mov dword ptr [esp + 0x10], edx		// 0x0005779c
		mov edx, dword ptr [esp + 0x348]		// 0x000577a0
		lea ecx, [eax*4 + nxP7Axes]		// 0x000577a7
		mov eax, dword ptr [ecx]		// 0x000577ae
		add edx, 0xc		// 0x000577b0
		mov dword ptr [edx], eax		// 0x000577b3
		mov eax, dword ptr [ecx + 4]		// 0x000577b5
		mov dword ptr [edx + 4], eax		// 0x000577b8
		mov ecx, dword ptr [ecx + 8]		// 0x000577bb
		mov dword ptr [edx + 8], ecx		// 0x000577be
		mov dword ptr [esp + 0x54], 1		// 0x000577c1
L577c9:
		mov eax, dword ptr [esp + 0x20]		// 0x000577c9
		add esi, 0xc		// 0x000577cd
		dec eax		// 0x000577d0
		mov dword ptr [esp + 0x20], eax		// 0x000577d1
		jne L576c2		// 0x000577d5
		mov ecx, ebp		// 0x000577db
		call nxP7CallGetEdges		// 0x000577dd
		mov ebp, eax		// 0x000577e2
		mov ecx, edi		// 0x000577e4
		mov dword ptr [esp + 0x7c], ebp		// 0x000577e6
		call nxP7CallGetEdges		// 0x000577ea
		lea edx, [esp + 0x21c]		// 0x000577ef
		mov dword ptr [esp + 0x8c], eax		// 0x000577f6
		xor esi, esi		// 0x000577fd
		mov dword ptr [esp + 0xc], edx		// 0x000577ff
L57803:
		mov eax, dword ptr [esp + 0xc]		// 0x00057803
		mov ecx, dword ptr [esp + 0x340]		// 0x00057807
		push eax		// 0x0005780e
		push esi		// 0x0005780f
		call nxP7CallComputeWorldEdgeNormal		// 0x00057810
		mov ecx, dword ptr [esp + 0xc]		// 0x00057815
		inc esi		// 0x00057819
		add ecx, 0xc		// 0x0005781a
		cmp esi, 0xc		// 0x0005781d
		mov dword ptr [esp + 0xc], ecx		// 0x00057820
		jb L57803		// 0x00057824
		lea ecx, [esp + 0x2ac]		// 0x00057826
		xor esi, esi		// 0x0005782d
		mov dword ptr [esp + 0xc], ecx		// 0x0005782f
L57833:
		mov edx, dword ptr [esp + 0xc]		// 0x00057833
		push edx		// 0x00057837
		push esi		// 0x00057838
		mov ecx, edi		// 0x00057839
		call nxP7CallComputeWorldEdgeNormal		// 0x0005783b
		mov ecx, dword ptr [esp + 0xc]		// 0x00057840
		inc esi		// 0x00057844
		add ecx, 0xc		// 0x00057845
		cmp esi, 0xc		// 0x00057848
		mov dword ptr [esp + 0xc], ecx		// 0x0005784b
		jb L57833		// 0x0005784f
		xor ecx, ecx		// 0x00057851
		lea edx, [esp + 0x220]		// 0x00057853
		mov dword ptr [esp + 0x80], ecx		// 0x0005785a
		mov dword ptr [esp + 0x88], edx		// 0x00057861
L57868:
		fld dword ptr [edx - 4]		// 0x00057868
		fmul dword ptr [ebx]		// 0x0005786b
		fld dword ptr [edx + 4]		// 0x0005786d
		fmul dword ptr [ebx + 8]		// 0x00057870
		faddp st(1), st(0)		// 0x00057873
		fld dword ptr [edx]		// 0x00057875
		fmul dword ptr [ebx + 4]		// 0x00057877
		faddp st(1), st(0)		// 0x0005787a
		fcomp dword ptr [nxP7K101041f0_4]		// 0x0005787c
		fnstsw ax		// 0x00057882
		test ah, 1		// 0x00057884
		jne L57c1b		// 0x00057887
		mov eax, dword ptr [ebp + ecx*8]		// 0x0005788d
		lea eax, [eax + eax*2]		// 0x00057891
		fld dword ptr [esp + eax*4 + 0x1bc]		// 0x00057894
		lea eax, [esp + eax*4 + 0x1bc]		// 0x0005789b
		fld dword ptr [eax + 4]		// 0x000578a2
		mov eax, dword ptr [eax + 8]		// 0x000578a5
		mov dword ptr [esp + 0x1c], eax		// 0x000578a8
		mov eax, dword ptr [ebp + ecx*8 + 4]		// 0x000578ac
		lea eax, [eax + eax*2]		// 0x000578b0
		mov ecx, dword ptr [esp + eax*4 + 0x1bc]		// 0x000578b3
		mov edx, dword ptr [esp + eax*4 + 0x1c0]		// 0x000578ba
		lea eax, [esp + eax*4 + 0x1bc]		// 0x000578c1
		mov eax, dword ptr [eax + 8]		// 0x000578c8
		mov dword ptr [esp + 0x48], ecx		// 0x000578cb
		fld dword ptr [esp + 0x48]		// 0x000578cf
		fsub st(0), st(2)		// 0x000578d3
		mov dword ptr [esp + 0x4c], edx		// 0x000578d5
		fld dword ptr [esp + 0x4c]		// 0x000578d9
		mov dword ptr [esp + 0x50], eax		// 0x000578dd
		fsub st(0), st(2)		// 0x000578e1
		lea eax, [esp + 0x2b0]		// 0x000578e3
		fld dword ptr [esp + 0x50]		// 0x000578ea
		xor ebp, ebp		// 0x000578ee
		fsub dword ptr [esp + 0x1c]		// 0x000578f0
		mov dword ptr [esp + 0x84], eax		// 0x000578f4
		fst dword ptr [esp + 0x78]		// 0x000578fb
		fmul dword ptr [esp + 0x78]		// 0x000578ff
		fld st(1)		// 0x00057903
		fmul st(0), st(2)		// 0x00057905
		faddp st(1), st(0)		// 0x00057907
		fld st(2)		// 0x00057909
		fmul st(0), st(3)		// 0x0005790b
		faddp st(1), st(0)		// 0x0005790d
		fsqrt		// 0x0005790f
		fdivr dword ptr [nxP7K10108738_4]		// 0x00057911
		fstp dword ptr [esp + 0xc]		// 0x00057917
		fxch st(1)		// 0x0005791b
		fmul dword ptr [esp + 0xc]		// 0x0005791d
		fstp dword ptr [esp + 0x70]		// 0x00057921
		fmul dword ptr [esp + 0xc]		// 0x00057925
		fstp dword ptr [esp + 0x74]		// 0x00057929
		fld dword ptr [esp + 0x78]		// 0x0005792d
		fmul dword ptr [esp + 0xc]		// 0x00057931
		fstp dword ptr [esp + 0x78]		// 0x00057935
		fxch st(1)		// 0x00057939
		fsub dword ptr [esp + 0x70]		// 0x0005793b
		fstp dword ptr [esp + 0x14]		// 0x0005793f
		fsub dword ptr [esp + 0x74]		// 0x00057943
		fstp dword ptr [esp + 0x18]		// 0x00057947
		fld dword ptr [esp + 0x1c]		// 0x0005794b
		fsub dword ptr [esp + 0x78]		// 0x0005794f
		fstp dword ptr [esp + 0x1c]		// 0x00057953
		fld dword ptr [esp + 0x70]		// 0x00057957
		fadd dword ptr [esp + 0x48]		// 0x0005795b
		fstp dword ptr [esp + 0x48]		// 0x0005795f
		fld dword ptr [esp + 0x74]		// 0x00057963
		fadd dword ptr [esp + 0x4c]		// 0x00057967
		fstp dword ptr [esp + 0x4c]		// 0x0005796b
		fld dword ptr [esp + 0x78]		// 0x0005796f
		fadd dword ptr [esp + 0x50]		// 0x00057973
		fstp dword ptr [esp + 0x50]		// 0x00057977
		jmp L57980		// 0x0005797b
		lea ecx, [ecx]		// 0x0005797d
L57980:
		fld dword ptr [eax - 4]		// 0x00057980
		fmul dword ptr [ebx]		// 0x00057983
		fld dword ptr [eax + 4]		// 0x00057985
		fmul dword ptr [ebx + 8]		// 0x00057988
		faddp st(1), st(0)		// 0x0005798b
		fld dword ptr [eax]		// 0x0005798d
		fmul dword ptr [ebx + 4]		// 0x0005798f
		faddp st(1), st(0)		// 0x00057992
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00057994
		fnstsw ax		// 0x0005799a
		test ah, 1		// 0x0005799c
		je L57bee		// 0x0005799f
		mov ecx, dword ptr [esp + 0x8c]		// 0x000579a5
		mov eax, dword ptr [ecx + ebp*8]		// 0x000579ac
		lea eax, [eax + eax*2]		// 0x000579af
		fld dword ptr [esp + eax*4 + 0x15c]		// 0x000579b2
		mov edx, dword ptr [esp + eax*4 + 0x164]		// 0x000579b9
		fld dword ptr [esp + eax*4 + 0x160]		// 0x000579c0
		lea eax, [esp + eax*4 + 0x15c]		// 0x000579c7
		mov eax, dword ptr [ecx + ebp*8 + 4]		// 0x000579ce
		lea eax, [eax + eax*2]		// 0x000579d2
		mov ecx, dword ptr [esp + eax*4 + 0x15c]		// 0x000579d5
		lea eax, [esp + eax*4 + 0x15c]		// 0x000579dc
		mov dword ptr [esp + 0x2c], edx		// 0x000579e3
		mov edx, dword ptr [eax + 4]		// 0x000579e7
		mov eax, dword ptr [eax + 8]		// 0x000579ea
		mov dword ptr [esp + 0x3c], ecx		// 0x000579ed
		fld dword ptr [esp + 0x3c]		// 0x000579f1
		fsub st(0), st(2)		// 0x000579f5
		mov dword ptr [esp + 0x40], edx		// 0x000579f7
		fld dword ptr [esp + 0x40]		// 0x000579fb
		mov dword ptr [esp + 0x44], eax		// 0x000579ff
		fsub st(0), st(2)		// 0x00057a03
		lea ecx, [esp + 0x20]		// 0x00057a05
		fld dword ptr [esp + 0x44]		// 0x00057a09
		push ecx		// 0x00057a0d
		fsub dword ptr [esp + 0x30]		// 0x00057a0e
		lea edx, [esp + 0x40]		// 0x00057a12
		push edx		// 0x00057a16
		lea eax, [esp + 0x50]		// 0x00057a17
		fst dword ptr [esp + 0x40]		// 0x00057a1b
		push eax		// 0x00057a1f
		fmul dword ptr [esp + 0x44]		// 0x00057a20
		lea edi, [esp + 0x11c]		// 0x00057a24
		fld st(1)		// 0x00057a2b
		lea edx, [esp + 0x30]		// 0x00057a2d
		fmul st(0), st(2)		// 0x00057a31
		mov ecx, ebx		// 0x00057a33
		lea esi, [esp + 0x20]		// 0x00057a35
		faddp st(1), st(0)		// 0x00057a39
		fld st(2)		// 0x00057a3b
		fmul st(0), st(3)		// 0x00057a3d
		faddp st(1), st(0)		// 0x00057a3f
		fsqrt		// 0x00057a41
		fdivr dword ptr [nxP7K10108738_4]		// 0x00057a43
		fstp dword ptr [esp + 0x18]		// 0x00057a49
		fxch st(1)		// 0x00057a4d
		fmul dword ptr [esp + 0x18]		// 0x00057a4f
		fstp dword ptr [esp + 0x3c]		// 0x00057a53
		fmul dword ptr [esp + 0x18]		// 0x00057a57
		fstp dword ptr [esp + 0x40]		// 0x00057a5b
		fld dword ptr [esp + 0x44]		// 0x00057a5f
		fmul dword ptr [esp + 0x18]		// 0x00057a63
		fstp dword ptr [esp + 0x44]		// 0x00057a67
		fxch st(1)		// 0x00057a6b
		fsub dword ptr [esp + 0x3c]		// 0x00057a6d
		fstp dword ptr [esp + 0x30]		// 0x00057a71
		fsub dword ptr [esp + 0x40]		// 0x00057a75
		fstp dword ptr [esp + 0x34]		// 0x00057a79
		fld dword ptr [esp + 0x38]		// 0x00057a7d
		fsub dword ptr [esp + 0x44]		// 0x00057a81
		fstp dword ptr [esp + 0x38]		// 0x00057a85
		fld dword ptr [esp + 0x3c]		// 0x00057a89
		fadd dword ptr [esp + 0x48]		// 0x00057a8d
		fstp dword ptr [esp + 0x48]		// 0x00057a91
		fld dword ptr [esp + 0x40]		// 0x00057a95
		fadd dword ptr [esp + 0x4c]		// 0x00057a99
		fstp dword ptr [esp + 0x4c]		// 0x00057a9d
		fld dword ptr [esp + 0x44]		// 0x00057aa1
		fadd dword ptr [esp + 0x50]		// 0x00057aa5
		fstp dword ptr [esp + 0x50]		// 0x00057aa9
		call nxP72276		// 0x00057aad
		add esp, 0xc		// 0x00057ab2
		test eax, eax		// 0x00057ab5
		je L57bee		// 0x00057ab7
		fld dword ptr [esp + 0x20]		// 0x00057abd
		fcomp dword ptr [esp + 0x10]		// 0x00057ac1
		fnstsw ax		// 0x00057ac5
		test ah, 5		// 0x00057ac7
		jp L57bee		// 0x00057aca
		fld dword ptr [esp + 0x24]		// 0x00057ad0
		mov ecx, dword ptr [esp + 0x348]		// 0x00057ad4
		fsub dword ptr [esp + 0x3c]		// 0x00057adb
		add ecx, 0xc		// 0x00057adf
		fld dword ptr [esp + 0x28]		// 0x00057ae2
		mov edx, ecx		// 0x00057ae6
		fsub dword ptr [esp + 0x40]		// 0x00057ae8
		fld dword ptr [esp + 0x2c]		// 0x00057aec
		fsub dword ptr [esp + 0x44]		// 0x00057af0
		fld dword ptr [esp + 0x14]		// 0x00057af4
		fsub dword ptr [esp + 0x48]		// 0x00057af8
		fstp dword ptr [esp + 0x58]		// 0x00057afc
		fld dword ptr [esp + 0x18]		// 0x00057b00
		fsub dword ptr [esp + 0x4c]		// 0x00057b04
		fstp dword ptr [esp + 0x5c]		// 0x00057b08
		fld dword ptr [esp + 0x1c]		// 0x00057b0c
		fsub dword ptr [esp + 0x50]		// 0x00057b10
		fld dword ptr [esp + 0x5c]		// 0x00057b14
		fmul st(0), st(2)		// 0x00057b18
		fld st(1)		// 0x00057b1a
		fmul st(0), st(4)		// 0x00057b1c
		fsubp st(1), st(0)		// 0x00057b1e
		fstp dword ptr [esp + 0x64]		// 0x00057b20
		mov eax, dword ptr [esp + 0x64]		// 0x00057b24
		mov dword ptr [edx], eax		// 0x00057b28
		fmul st(0), st(3)		// 0x00057b2a
		fxch st(1)		// 0x00057b2c
		fmul dword ptr [esp + 0x58]		// 0x00057b2e
		fsubp st(1), st(0)		// 0x00057b32
		fstp dword ptr [esp + 0x68]		// 0x00057b34
		mov eax, dword ptr [esp + 0x68]		// 0x00057b38
		mov dword ptr [edx + 4], eax		// 0x00057b3c
		fmul dword ptr [esp + 0x58]		// 0x00057b3f
		fld dword ptr [esp + 0x5c]		// 0x00057b43
		fmul st(0), st(2)		// 0x00057b47
		fsubp st(1), st(0)		// 0x00057b49
		fstp dword ptr [esp + 0x6c]		// 0x00057b4b
		mov eax, dword ptr [esp + 0x6c]		// 0x00057b4f
		mov dword ptr [edx + 8], eax		// 0x00057b53
		fstp st(0)		// 0x00057b56
		fld dword ptr [ecx + 8]		// 0x00057b58
		fld dword ptr [ecx + 4]		// 0x00057b5b
		fld dword ptr [ecx]		// 0x00057b5e
		fld st(0)		// 0x00057b60
		fmul st(0), st(1)		// 0x00057b62
		fld st(2)		// 0x00057b64
		fmul st(0), st(3)		// 0x00057b66
		faddp st(1), st(0)		// 0x00057b68
		fld st(3)		// 0x00057b6a
		fmul st(0), st(4)		// 0x00057b6c
		faddp st(1), st(0)		// 0x00057b6e
		fstp st(3)		// 0x00057b70
		fstp st(0)		// 0x00057b72
		fstp st(0)		// 0x00057b74
		fld dword ptr [nxP7K101041f0_4]		// 0x00057b76
		fld st(1)		// 0x00057b7c
		fucompp		// 0x00057b7e
		fnstsw ax		// 0x00057b80
		test ah, 0x44		// 0x00057b82
		jnp L57ba5		// 0x00057b85
		fsqrt		// 0x00057b87
		fdivr dword ptr [nxP7K101041ec_4]		// 0x00057b89
		fld st(0)		// 0x00057b8f
		fmul dword ptr [ecx]		// 0x00057b91
		fstp dword ptr [ecx]		// 0x00057b93
		fld st(0)		// 0x00057b95
		fmul dword ptr [ecx + 4]		// 0x00057b97
		fstp dword ptr [ecx + 4]		// 0x00057b9a
		fmul dword ptr [ecx + 8]		// 0x00057b9d
		fstp dword ptr [ecx + 8]		// 0x00057ba0
		jmp L57ba7		// 0x00057ba3
L57ba5:
		fstp st(0)		// 0x00057ba5
L57ba7:
		fld dword ptr [ecx + 8]		// 0x00057ba7
		fmul dword ptr [ebx + 8]		// 0x00057baa
		fld dword ptr [ecx + 4]		// 0x00057bad
		fmul dword ptr [ebx + 4]		// 0x00057bb0
		faddp st(1), st(0)		// 0x00057bb3
		fld dword ptr [ebx]		// 0x00057bb5
		fmul dword ptr [ecx]		// 0x00057bb7
		faddp st(1), st(0)		// 0x00057bb9
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00057bbb
		fnstsw ax		// 0x00057bc1
		test ah, 0x41		// 0x00057bc3
		jne L57bde		// 0x00057bc6
		fld dword ptr [ecx]		// 0x00057bc8
		fchs		// 0x00057bca
		fstp dword ptr [ecx]		// 0x00057bcc
		fld dword ptr [ecx + 4]		// 0x00057bce
		fchs		// 0x00057bd1
		fstp dword ptr [ecx + 4]		// 0x00057bd3
		fld dword ptr [ecx + 8]		// 0x00057bd6
		fchs		// 0x00057bd9
		fstp dword ptr [ecx + 8]		// 0x00057bdb
L57bde:
		mov ecx, dword ptr [esp + 0x20]		// 0x00057bde
		mov dword ptr [esp + 0x54], 2		// 0x00057be2
		mov dword ptr [esp + 0x10], ecx		// 0x00057bea
L57bee:
		mov eax, dword ptr [esp + 0x84]		// 0x00057bee
		inc ebp		// 0x00057bf5
		add eax, 0xc		// 0x00057bf6
		cmp ebp, 0xc		// 0x00057bf9
		mov dword ptr [esp + 0x84], eax		// 0x00057bfc
		jb L57980		// 0x00057c03
		mov ebp, dword ptr [esp + 0x7c]		// 0x00057c09
		mov ecx, dword ptr [esp + 0x80]		// 0x00057c0d
		mov edx, dword ptr [esp + 0x88]		// 0x00057c14
L57c1b:
		inc ecx		// 0x00057c1b
		add edx, 0xc		// 0x00057c1c
		cmp ecx, 0xc		// 0x00057c1f
		mov dword ptr [esp + 0x80], ecx		// 0x00057c22
		mov dword ptr [esp + 0x88], edx		// 0x00057c29
		jb L57868		// 0x00057c30
		cmp dword ptr [esp + 0x54], -1		// 0x00057c36
		jne L57c49		// 0x00057c3b
		pop edi		// 0x00057c3d
		pop esi		// 0x00057c3e
		xor al, al		// 0x00057c3f
		pop ebp		// 0x00057c41
		add esp, 0x330		// 0x00057c42
		ret		// 0x00057c48
L57c49:
		mov eax, dword ptr [esp + 0x348]		// 0x00057c49
		mov edx, dword ptr [esp + 0x10]		// 0x00057c50
		pop edi		// 0x00057c54
		pop esi		// 0x00057c55
		mov dword ptr [eax + 0x18], edx		// 0x00057c56
		mov al, 1		// 0x00057c59
		pop ebp		// 0x00057c5b
		add esp, 0x330		// 0x00057c5c
		ret		// 0x00057c62
		}
	}

// phys_fn_002288 (0x00057c70, 133 B), 45 instructions
__declspec(naked) unsigned __fastcall nxP72288(void* self, void* edxUnused)
	{
	__asm
		{
		mov edx, dword ptr [eax + 0x1c]		// 0x00057c70
		sub esp, 0x34		// 0x00057c73
		push esi		// 0x00057c76
		mov esi, dword ptr [esp + 0x3c]		// 0x00057c77
		mov esi, dword ptr [esi + 8]		// 0x00057c7b
		mov edx, dword ptr [esi + edx*4]		// 0x00057c7e
		test edx, edx		// 0x00057c81
		pop esi		// 0x00057c83
		je L57c8e		// 0x00057c84
		mov edx, dword ptr [edx + 0x9c]		// 0x00057c86
		jmp L57c90		// 0x00057c8c
L57c8e:
		xor edx, edx		// 0x00057c8e
L57c90:
		mov dword ptr [esp + 4], edx		// 0x00057c90
		mov edx, dword ptr [eax]		// 0x00057c94
		mov dword ptr [esp + 8], edx		// 0x00057c96
		mov edx, dword ptr [eax + 4]		// 0x00057c9a
		mov dword ptr [esp + 0xc], edx		// 0x00057c9d
		mov edx, dword ptr [eax + 8]		// 0x00057ca1
		mov dword ptr [esp + 0x10], edx		// 0x00057ca4
		mov edx, dword ptr [eax + 0xc]		// 0x00057ca8
		mov dword ptr [esp + 0x14], edx		// 0x00057cab
		mov edx, dword ptr [eax + 0x10]		// 0x00057caf
		mov eax, dword ptr [eax + 0x14]		// 0x00057cb2
		mov dword ptr [esp + 0x18], edx		// 0x00057cb5
		mov edx, dword ptr [ecx]		// 0x00057cb9
		mov dword ptr [esp + 0x1c], eax		// 0x00057cbb
		mov eax, dword ptr [ecx + 4]		// 0x00057cbf
		mov ecx, dword ptr [ecx + 8]		// 0x00057cc2
		mov dword ptr [esp + 0x24], edx		// 0x00057cc5
		mov edx, dword ptr [esp + 0x3c]		// 0x00057cc9
		mov dword ptr [esp + 0x28], eax		// 0x00057ccd
		mov eax, dword ptr [nxP7SweepCache]		// 0x00057cd1
		mov dword ptr [esp + 0x2c], ecx		// 0x00057cd6
		mov dword ptr [esp + 0x30], edx		// 0x00057cda
		mov ecx, dword ptr [eax + 0x14]		// 0x00057cde
		test ecx, ecx		// 0x00057ce1
		je L57cf1		// 0x00057ce3
		mov eax, ecx		// 0x00057ce5
		mov edx, dword ptr [eax]		// 0x00057ce7
		lea ecx, [esp]		// 0x00057ce9
		push ecx		// 0x00057cec
		mov ecx, eax		// 0x00057ced
		call dword ptr [edx]		// 0x00057cef
L57cf1:
		add esp, 0x34		// 0x00057cf1
		ret		// 0x00057cf4
		}
	}

// phys_fn_002290 and phys_fn_002292 and phys_fn_002294 (one function) (0x00057d00, 1756 B), 504 instructions
__declspec(naked) unsigned __cdecl nxP72290()
	{
	__asm
		{
		sub esp, 0xa0		// 0x00057d00
		push ebx		// 0x00057d06
		push ebp		// 0x00057d07
		mov ebp, dword ptr [esp + 0xc0]		// 0x00057d08
		push esi		// 0x00057d0f
		mov esi, dword ptr [esp + 0xb0]		// 0x00057d10
		lea eax, [esp + 0x50]		// 0x00057d17
		push eax		// 0x00057d1b
		mov ecx, esi		// 0x00057d1c
		call nxP7CallNormal		// 0x00057d1e
		mov ecx, esi		// 0x00057d23
		mov edx, dword ptr [ecx]		// 0x00057d25
		mov eax, dword ptr [ecx + 4]		// 0x00057d27
		mov ecx, dword ptr [ecx + 8]		// 0x00057d2a
		mov dword ptr [esp + 0x88], edx		// 0x00057d2d
		lea edx, [esi + 0xc]		// 0x00057d34
		mov dword ptr [esp + 0x8c], eax		// 0x00057d37
		mov eax, dword ptr [edx]		// 0x00057d3e
		mov dword ptr [esp + 0x90], ecx		// 0x00057d40
		mov ecx, dword ptr [edx + 4]		// 0x00057d47
		mov edx, dword ptr [edx + 8]		// 0x00057d4a
		add esi, 0x18		// 0x00057d4d
		mov dword ptr [esp + 0x94], eax		// 0x00057d50
		mov eax, dword ptr [esi]		// 0x00057d57
		mov dword ptr [esp + 0x98], ecx		// 0x00057d59
		mov ecx, dword ptr [esi + 4]		// 0x00057d60
		mov dword ptr [esp + 0x9c], edx		// 0x00057d63
		mov edx, dword ptr [esi + 8]		// 0x00057d6a
		push 0		// 0x00057d6d
		mov dword ptr [esp + 0xa8], ecx		// 0x00057d6f
		push 0x3ca3d70a		// 0x00057d76
		lea ecx, [esp + 0x90]		// 0x00057d7b
		mov dword ptr [esp + 0xa8], eax		// 0x00057d82
		mov dword ptr [esp + 0xb0], edx		// 0x00057d89
		call nxP7CallInflate		// 0x00057d90
		mov ebx, dword ptr [esp + 0xb8]		// 0x00057d95
		shr ebx, 3		// 0x00057d9c
		not bl		// 0x00057d9f
		and bl, 1		// 0x00057da1
		mov byte ptr [esp + 0x18], bl		// 0x00057da4
		je L57dd4		// 0x00057da8
		fld dword ptr [esp + 0x54]		// 0x00057daa
		fmul dword ptr [ebp + 4]		// 0x00057dae
		fld dword ptr [esp + 0x58]		// 0x00057db1
		fmul dword ptr [ebp + 8]		// 0x00057db5
		faddp st(1), st(0)		// 0x00057db8
		fld dword ptr [esp + 0x50]		// 0x00057dba
		fmul dword ptr [ebp]		// 0x00057dbe
		faddp st(1), st(0)		// 0x00057dc1
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00057dc3
		fnstsw ax		// 0x00057dc9
		test ah, 0x41		// 0x00057dcb
		je L583b6		// 0x00057dce
L57dd4:
		mov eax, dword ptr [esp + 0xc8]		// 0x00057dd4
		mov ecx, dword ptr [eax + 0x18]		// 0x00057ddb
		mov dword ptr [esp + 0x14], ecx		// 0x00057dde
		mov ecx, dword ptr [esp + 0xbc]		// 0x00057de2
		push edi		// 0x00057de9
		mov dword ptr [esp + 0x24], 0xffffffff		// 0x00057dea
		call nxP7CallGetVertexNormals		// 0x00057df2
		mov ecx, dword ptr [esp + 0xc4]		// 0x00057df7
		mov edi, ecx		// 0x00057dfe
		lea edx, [eax + 8]		// 0x00057e00
		sub ecx, eax		// 0x00057e03
		mov dword ptr [esp + 0x10], edx		// 0x00057e05
		mov dword ptr [esp + 0x40], ecx		// 0x00057e09
		mov dword ptr [esp + 0x20], 8		// 0x00057e0d
		jmp L57e20		// 0x00057e15
L57e17:
		mov bl, byte ptr [esp + 0x1c]		// 0x00057e17
		jmp L57e20		// 0x00057e1b
		lea ecx, [ecx]		// 0x00057e1d
L57e20:
		test bl, bl		// 0x00057e20
		je L57e4d		// 0x00057e22
		fld dword ptr [esp + 0x58]		// 0x00057e24
		fmul dword ptr [edx - 4]		// 0x00057e28
		fld dword ptr [esp + 0x54]		// 0x00057e2b
		fmul dword ptr [edx - 8]		// 0x00057e2f
		faddp st(1), st(0)		// 0x00057e32
		fld dword ptr [esp + 0x5c]		// 0x00057e34
		fmul dword ptr [edx]		// 0x00057e38
		faddp st(1), st(0)		// 0x00057e3a
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00057e3c
		fnstsw ax		// 0x00057e42
		test ah, 1		// 0x00057e44
		je L57f10		// 0x00057e47
L57e4d:
		mov edx, dword ptr [esp + 0x1c]		// 0x00057e4d
		push edx		// 0x00057e51
		lea eax, [esp + 0x54]		// 0x00057e52
		push eax		// 0x00057e56
		lea ecx, [esp + 0x1c]		// 0x00057e57
		push ecx		// 0x00057e5b
		push ebp		// 0x00057e5c
		push edi		// 0x00057e5d
		lea ebx, [esp + 0x80]		// 0x00057e5e
		lea eax, [esp + 0xb8]		// 0x00057e65
		lea esi, [esp + 0xac]		// 0x00057e6c
		lea ecx, [esp + 0xa0]		// 0x00057e73
		call nxP72278		// 0x00057e7a
		add esp, 0x14		// 0x00057e7f
		test al, al		// 0x00057e82
		je L57f10		// 0x00057e84
		fld dword ptr [esp + 0x14]		// 0x00057e8a
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00057e8e
		fnstsw ax		// 0x00057e94
		test ah, 5		// 0x00057e96
		jnp L57f10		// 0x00057e99
		fld dword ptr [esp + 0x14]		// 0x00057e9b
		fcomp dword ptr [esp + 0x18]		// 0x00057e9f
		fnstsw ax		// 0x00057ea3
		test ah, 1		// 0x00057ea5
		je L57f10		// 0x00057ea8
		fld dword ptr [esp + 0x14]		// 0x00057eaa
		mov edx, dword ptr [esp + 0x14]		// 0x00057eae
		fmul dword ptr [ebp]		// 0x00057eb2
		mov eax, dword ptr [esp + 0x40]		// 0x00057eb5
		fld dword ptr [esp + 0x14]		// 0x00057eb9
		mov ecx, dword ptr [esp + 0x10]		// 0x00057ebd
		fmul dword ptr [ebp + 4]		// 0x00057ec1
		mov dword ptr [esp + 0x18], edx		// 0x00057ec4
		fld dword ptr [esp + 0x14]		// 0x00057ec8
		mov edx, dword ptr [esp + 0xcc]		// 0x00057ecc
		fmul dword ptr [ebp + 8]		// 0x00057ed3
		mov dword ptr [esp + 0x24], 0		// 0x00057ed6
		fstp dword ptr [esp + 0x7c]		// 0x00057ede
		fxch st(1)		// 0x00057ee2
		fadd dword ptr [edi]		// 0x00057ee4
		fstp dword ptr [esp + 0x28]		// 0x00057ee6
		fadd dword ptr [edi + 4]		// 0x00057eea
		fstp dword ptr [esp + 0x2c]		// 0x00057eed
		fld dword ptr [esp + 0x7c]		// 0x00057ef1
		fadd dword ptr [eax + ecx]		// 0x00057ef5
		mov eax, dword ptr [esp + 0x28]		// 0x00057ef8
		mov ecx, dword ptr [esp + 0x2c]		// 0x00057efc
		mov dword ptr [edx], eax		// 0x00057f00
		fstp dword ptr [esp + 0x30]		// 0x00057f02
		mov eax, dword ptr [esp + 0x30]		// 0x00057f06
		mov dword ptr [edx + 4], ecx		// 0x00057f0a
		mov dword ptr [edx + 8], eax		// 0x00057f0d
L57f10:
		mov edx, dword ptr [esp + 0x10]		// 0x00057f10
		mov eax, dword ptr [esp + 0x20]		// 0x00057f14
		add edx, 0xc		// 0x00057f18
		add edi, 0xc		// 0x00057f1b
		dec eax		// 0x00057f1e
		mov dword ptr [esp + 0x10], edx		// 0x00057f1f
		mov dword ptr [esp + 0x20], eax		// 0x00057f23
		jne L57e17		// 0x00057f27
		mov eax, dword ptr [esp + 0x24]		// 0x00057f2d
		test eax, eax		// 0x00057f31
		jne L57f53		// 0x00057f33
		mov ecx, dword ptr [esp + 0xcc]		// 0x00057f35
		mov edx, dword ptr [esp + 0x54]		// 0x00057f3c
		mov eax, dword ptr [esp + 0x58]		// 0x00057f40
		add ecx, 0xc		// 0x00057f44
		mov dword ptr [ecx], edx		// 0x00057f47
		mov edx, dword ptr [esp + 0x5c]		// 0x00057f49
		mov dword ptr [ecx + 4], eax		// 0x00057f4d
		mov dword ptr [ecx + 8], edx		// 0x00057f50
L57f53:
		mov edi, dword ptr [esp + 0xc0]		// 0x00057f53
		mov esi, dword ptr [esp + 0xb4]		// 0x00057f5a
		add edi, 0xc		// 0x00057f61
		mov ebx, 3		// 0x00057f64
		lea esp, [esp]		// 0x00057f69
L57f70:
		fld dword ptr [ebp]		// 0x00057f70
		mov edx, dword ptr [esp + 0xc0]		// 0x00057f73
		fchs		// 0x00057f7a
		lea eax, [esp + 0x50]		// 0x00057f7c
		fstp dword ptr [esp + 0x60]		// 0x00057f80
		push eax		// 0x00057f84
		fld dword ptr [ebp + 4]		// 0x00057f85
		lea ecx, [esp + 0x14]		// 0x00057f88
		push ecx		// 0x00057f8c
		fchs		// 0x00057f8d
		fstp dword ptr [esp + 0x6c]		// 0x00057f8f
		push esi		// 0x00057f93
		fld dword ptr [ebp + 8]		// 0x00057f94
		push edi		// 0x00057f97
		fchs		// 0x00057f98
		push edx		// 0x00057f9a
		lea ecx, [esp + 0x74]		// 0x00057f9b
		fstp dword ptr [esp + 0x7c]		// 0x00057f9f
		call nxP7CallNxRayAABBSlab		// 0x00057fa3
		mov ecx, eax		// 0x00057fa8
		add esp, 0x14		// 0x00057faa
		cmp ecx, -1		// 0x00057fad
		je L5801f		// 0x00057fb0
		fld dword ptr [esp + 0x10]		// 0x00057fb2
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00057fb6
		fnstsw ax		// 0x00057fbc
		test ah, 5		// 0x00057fbe
		jnp L5801f		// 0x00057fc1
		fld dword ptr [esp + 0x10]		// 0x00057fc3
		fcomp dword ptr [esp + 0x18]		// 0x00057fc7
		fnstsw ax		// 0x00057fcb
		test ah, 5		// 0x00057fcd
		jp L5801f		// 0x00057fd0
		mov eax, dword ptr [esp + 0x10]		// 0x00057fd2
		mov dword ptr [esp + 0x18], eax		// 0x00057fd6
		mov eax, dword ptr [esp + 0xcc]		// 0x00057fda
		lea ecx, [ecx + ecx*2]		// 0x00057fe1
		lea edx, [ecx*4 + nxP7Axes]		// 0x00057fe4
		mov ecx, dword ptr [edx]		// 0x00057feb
		add eax, 0xc		// 0x00057fed
		mov dword ptr [eax], ecx		// 0x00057ff0
		mov ecx, dword ptr [edx + 4]		// 0x00057ff2
		mov dword ptr [eax + 4], ecx		// 0x00057ff5
		mov edx, dword ptr [edx + 8]		// 0x00057ff8
		mov ecx, dword ptr [esp + 0xcc]		// 0x00057ffb
		mov dword ptr [eax + 8], edx		// 0x00058002
		mov eax, esi		// 0x00058005
		mov edx, dword ptr [eax]		// 0x00058007
		mov dword ptr [ecx], edx		// 0x00058009
		mov edx, dword ptr [eax + 4]		// 0x0005800b
		mov dword ptr [ecx + 4], edx		// 0x0005800e
		mov eax, dword ptr [eax + 8]		// 0x00058011
		mov dword ptr [esp + 0x24], 1		// 0x00058014
		mov dword ptr [ecx + 8], eax		// 0x0005801c
L5801f:
		add esi, 0xc		// 0x0005801f
		dec ebx		// 0x00058022
		jne L57f70		// 0x00058023
		mov edi, dword ptr [esp + 0xc0]		// 0x00058029
		mov ecx, edi		// 0x00058030
		call nxP7CallGetEdgeNormals		// 0x00058032
		mov ecx, edi		// 0x00058037
		mov esi, eax		// 0x00058039
		call nxP7CallGetEdgesX		// 0x0005803b
		add esi, 4		// 0x00058040
		mov edx, eax		// 0x00058043
		mov dword ptr [esp + 0x70], esi		// 0x00058045
		mov dword ptr [esp + 0x40], 0xc		// 0x00058049
L58051:
		mov eax, dword ptr [edx]		// 0x00058051
		mov ecx, dword ptr [esp + 0xc4]		// 0x00058053
		lea eax, [eax + eax*2]		// 0x0005805a
		fld dword ptr [ecx + eax*4]		// 0x0005805d
		lea eax, [ecx + eax*4]		// 0x00058060
		fld dword ptr [eax + 4]		// 0x00058063
		mov eax, dword ptr [eax + 8]		// 0x00058066
		mov dword ptr [esp + 0x30], eax		// 0x00058069
		mov eax, dword ptr [edx + 4]		// 0x0005806d
		add edx, 4		// 0x00058070
		lea eax, [eax + eax*2]		// 0x00058073
		lea eax, [ecx + eax*4]		// 0x00058076
		mov ecx, dword ptr [eax]		// 0x00058079
		mov dword ptr [esp + 0x34], ecx		// 0x0005807b
		fld dword ptr [esp + 0x34]		// 0x0005807f
		fsub st(0), st(2)		// 0x00058083
		mov ecx, dword ptr [eax + 4]		// 0x00058085
		mov eax, dword ptr [eax + 8]		// 0x00058088
		mov dword ptr [esp + 0x38], ecx		// 0x0005808b
		fld dword ptr [esp + 0x38]		// 0x0005808f
		fsub st(0), st(2)		// 0x00058093
		mov dword ptr [esp + 0x3c], eax		// 0x00058095
		fld dword ptr [esp + 0x3c]		// 0x00058099
		add edx, 4		// 0x0005809d
		fsub dword ptr [esp + 0x30]		// 0x000580a0
		mov dword ptr [esp + 0x50], edx		// 0x000580a4
		fst dword ptr [esp + 0x4c]		// 0x000580a8
		fmul dword ptr [esp + 0x4c]		// 0x000580ac
		fld st(1)		// 0x000580b0
		fmul st(0), st(2)		// 0x000580b2
		faddp st(1), st(0)		// 0x000580b4
		fld st(2)		// 0x000580b6
		fmul st(0), st(3)		// 0x000580b8
		faddp st(1), st(0)		// 0x000580ba
		fsqrt		// 0x000580bc
		fdivr dword ptr [nxP7K10108738_4]		// 0x000580be
		fstp dword ptr [esp + 0x10]		// 0x000580c4
		fxch st(1)		// 0x000580c8
		fmul dword ptr [esp + 0x10]		// 0x000580ca
		fstp dword ptr [esp + 0x44]		// 0x000580ce
		fmul dword ptr [esp + 0x10]		// 0x000580d2
		fstp dword ptr [esp + 0x48]		// 0x000580d6
		fld dword ptr [esp + 0x4c]		// 0x000580da
		fmul dword ptr [esp + 0x10]		// 0x000580de
		fstp dword ptr [esp + 0x4c]		// 0x000580e2
		fxch st(1)		// 0x000580e6
		fsub dword ptr [esp + 0x44]		// 0x000580e8
		fstp dword ptr [esp + 0x28]		// 0x000580ec
		fsub dword ptr [esp + 0x48]		// 0x000580f0
		fstp dword ptr [esp + 0x2c]		// 0x000580f4
		fld dword ptr [esp + 0x30]		// 0x000580f8
		fsub dword ptr [esp + 0x4c]		// 0x000580fc
		fstp dword ptr [esp + 0x30]		// 0x00058100
		fld dword ptr [esp + 0x44]		// 0x00058104
		fadd dword ptr [esp + 0x34]		// 0x00058108
		fstp dword ptr [esp + 0x34]		// 0x0005810c
		fld dword ptr [esp + 0x48]		// 0x00058110
		fadd dword ptr [esp + 0x38]		// 0x00058114
		fstp dword ptr [esp + 0x38]		// 0x00058118
		fld dword ptr [esp + 0x4c]		// 0x0005811c
		fadd dword ptr [esp + 0x3c]		// 0x00058120
		fstp dword ptr [esp + 0x3c]		// 0x00058124
		fld dword ptr [esi - 4]		// 0x00058128
		fmul dword ptr [ebp]		// 0x0005812b
		fld dword ptr [esi + 4]		// 0x0005812e
		fmul dword ptr [ebp + 8]		// 0x00058131
		faddp st(1), st(0)		// 0x00058134
		fld dword ptr [esi]		// 0x00058136
		fmul dword ptr [ebp + 4]		// 0x00058138
		faddp st(1), st(0)		// 0x0005813b
		fcomp dword ptr [nxP7K101041f0_4]		// 0x0005813d
		fnstsw ax		// 0x00058143
		test ah, 1		// 0x00058145
		jne L58398		// 0x00058148
		mov ebx, dword ptr [esp + 0xb4]		// 0x0005814e
		mov esi, dword ptr [esp + 0xb8]		// 0x00058155
		xor ecx, ecx		// 0x0005815c
		mov eax, ebx		// 0x0005815e
		lea edx, [esi + 4]		// 0x00058160
		sub eax, esi		// 0x00058163
		mov dword ptr [esp + 0x1c], ecx		// 0x00058165
		mov dword ptr [esp + 0x14], ebx		// 0x00058169
		mov dword ptr [esp + 0x10], edx		// 0x0005816d
		mov dword ptr [esp + 0x6c], eax		// 0x00058171
		jmp L58190		// 0x00058175
L58177:
		mov ebx, dword ptr [esp + 0xb4]		// 0x00058177
		mov esi, dword ptr [esp + 0xb8]		// 0x0005817e
		jmp L58190		// 0x00058185
		lea esp, [esp]		// 0x00058187
		mov edi, edi		// 0x0005818e
L58190:
		mov edi, dword ptr [esp + 0xbc]		// 0x00058190
		mov eax, 1		// 0x00058197
		shl eax, cl		// 0x0005819c
		test edi, eax		// 0x0005819e
		je L58368		// 0x000581a0
		test esi, esi		// 0x000581a6
		je L581d0		// 0x000581a8
		fld dword ptr [edx - 4]		// 0x000581aa
		fmul dword ptr [ebp]		// 0x000581ad
		fld dword ptr [edx + 4]		// 0x000581b0
		fmul dword ptr [ebp + 8]		// 0x000581b3
		faddp st(1), st(0)		// 0x000581b6
		fld dword ptr [edx]		// 0x000581b8
		fmul dword ptr [ebp + 4]		// 0x000581ba
		faddp st(1), st(0)		// 0x000581bd
		fcomp dword ptr [nxP7K101041f0_4]		// 0x000581bf
		fnstsw ax		// 0x000581c5
		test ah, 1		// 0x000581c7
		je L58368		// 0x000581ca
L581d0:
		lea eax, [ecx + 1]		// 0x000581d0
		cmp eax, 3		// 0x000581d3
		jne L581da		// 0x000581d6
		xor eax, eax		// 0x000581d8
L581da:
		lea ecx, [eax + eax*2]		// 0x000581da
		lea edx, [esp + 0x20]		// 0x000581dd
		push edx		// 0x000581e1
		mov edx, dword ptr [esp + 0x18]		// 0x000581e2
		lea ebx, [ebx + ecx*4]		// 0x000581e6
		lea eax, [esp + 0x38]		// 0x000581e9
		push ebx		// 0x000581ed
		push eax		// 0x000581ee
		lea edi, [esp + 0x80]		// 0x000581ef
		mov ecx, ebp		// 0x000581f6
		lea esi, [esp + 0x34]		// 0x000581f8
		call nxP72276		// 0x000581fc
		add esp, 0xc		// 0x00058201
		test eax, eax		// 0x00058204
		je L58368		// 0x00058206
		fld dword ptr [esp + 0x20]		// 0x0005820c
		fcomp dword ptr [esp + 0x18]		// 0x00058210
		fnstsw ax		// 0x00058214
		test ah, 5		// 0x00058216
		jp L58368		// 0x00058219
		mov eax, dword ptr [esp + 0x14]		// 0x0005821f
		fld dword ptr [eax]		// 0x00058223
		mov ecx, dword ptr [esp + 0x6c]		// 0x00058225
		fsub dword ptr [ebx]		// 0x00058229
		mov edx, dword ptr [esp + 0x10]		// 0x0005822b
		fld dword ptr [ecx + edx]		// 0x0005822f
		mov edx, dword ptr [esp + 0xcc]		// 0x00058232
		fsub dword ptr [ebx + 4]		// 0x00058239
		lea ecx, [edx + 0xc]		// 0x0005823c
		fld dword ptr [eax + 8]		// 0x0005823f
		mov eax, ecx		// 0x00058242
		fsub dword ptr [ebx + 8]		// 0x00058244
		fld dword ptr [esp + 0x28]		// 0x00058247
		fsub dword ptr [esp + 0x34]		// 0x0005824b
		fstp dword ptr [esp + 0x80]		// 0x0005824f
		fld dword ptr [esp + 0x2c]		// 0x00058256
		fsub dword ptr [esp + 0x38]		// 0x0005825a
		fstp dword ptr [esp + 0x84]		// 0x0005825e
		fld dword ptr [esp + 0x30]		// 0x00058265
		fsub dword ptr [esp + 0x3c]		// 0x00058269
		fld dword ptr [esp + 0x84]		// 0x0005826d
		fmul st(0), st(2)		// 0x00058274
		fld st(1)		// 0x00058276
		fmul st(0), st(4)		// 0x00058278
		fsubp st(1), st(0)		// 0x0005827a
		fstp dword ptr [esp + 0x60]		// 0x0005827c
		mov esi, dword ptr [esp + 0x60]		// 0x00058280
		mov dword ptr [eax], esi		// 0x00058284
		fmul st(0), st(3)		// 0x00058286
		fxch st(1)		// 0x00058288
		fmul dword ptr [esp + 0x80]		// 0x0005828a
		fsubp st(1), st(0)		// 0x00058291
		fstp dword ptr [esp + 0x64]		// 0x00058293
		mov esi, dword ptr [esp + 0x64]		// 0x00058297
		mov dword ptr [eax + 4], esi		// 0x0005829b
		fmul dword ptr [esp + 0x80]		// 0x0005829e
		fld dword ptr [esp + 0x84]		// 0x000582a5
		fmul st(0), st(2)		// 0x000582ac
		fsubp st(1), st(0)		// 0x000582ae
		fstp dword ptr [esp + 0x68]		// 0x000582b0
		mov esi, dword ptr [esp + 0x68]		// 0x000582b4
		mov dword ptr [eax + 8], esi		// 0x000582b8
		fstp st(0)		// 0x000582bb
		fld dword ptr [ecx + 8]		// 0x000582bd
		fld dword ptr [ecx + 4]		// 0x000582c0
		fld dword ptr [ecx]		// 0x000582c3
		fld st(0)		// 0x000582c5
		fmul st(0), st(1)		// 0x000582c7
		fld st(2)		// 0x000582c9
		fmul st(0), st(3)		// 0x000582cb
		faddp st(1), st(0)		// 0x000582cd
		fld st(3)		// 0x000582cf
		fmul st(0), st(4)		// 0x000582d1
		faddp st(1), st(0)		// 0x000582d3
		fstp st(3)		// 0x000582d5
		fstp st(0)		// 0x000582d7
		fstp st(0)		// 0x000582d9
		fld dword ptr [nxP7K101041f0_4]		// 0x000582db
		fld st(1)		// 0x000582e1
		fucompp		// 0x000582e3
		fnstsw ax		// 0x000582e5
		test ah, 0x44		// 0x000582e7
		jnp L5830a		// 0x000582ea
		fsqrt		// 0x000582ec
		fdivr dword ptr [nxP7K101041ec_4]		// 0x000582ee
		fld st(0)		// 0x000582f4
		fmul dword ptr [ecx]		// 0x000582f6
		fstp dword ptr [ecx]		// 0x000582f8
		fld st(0)		// 0x000582fa
		fmul dword ptr [ecx + 4]		// 0x000582fc
		fstp dword ptr [ecx + 4]		// 0x000582ff
		fmul dword ptr [ecx + 8]		// 0x00058302
		fstp dword ptr [ecx + 8]		// 0x00058305
		jmp L5830c		// 0x00058308
L5830a:
		fstp st(0)		// 0x0005830a
L5830c:
		fld dword ptr [ecx + 8]		// 0x0005830c
		fmul dword ptr [ebp + 8]		// 0x0005830f
		fld dword ptr [ecx + 4]		// 0x00058312
		fmul dword ptr [ebp + 4]		// 0x00058315
		faddp st(1), st(0)		// 0x00058318
		fld dword ptr [ebp]		// 0x0005831a
		fmul dword ptr [ecx]		// 0x0005831d
		faddp st(1), st(0)		// 0x0005831f
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00058321
		fnstsw ax		// 0x00058327
		test ah, 0x41		// 0x00058329
		jne L58344		// 0x0005832c
		fld dword ptr [ecx]		// 0x0005832e
		fchs		// 0x00058330
		fstp dword ptr [ecx]		// 0x00058332
		fld dword ptr [ecx + 4]		// 0x00058334
		fchs		// 0x00058337
		fstp dword ptr [ecx + 4]		// 0x00058339
		fld dword ptr [ecx + 8]		// 0x0005833c
		fchs		// 0x0005833f
		fstp dword ptr [ecx + 8]		// 0x00058341
L58344:
		mov ecx, dword ptr [esp + 0x20]		// 0x00058344
		mov eax, dword ptr [esp + 0x74]		// 0x00058348
		mov dword ptr [edx], eax		// 0x0005834c
		mov eax, dword ptr [esp + 0x7c]		// 0x0005834e
		mov dword ptr [esp + 0x18], ecx		// 0x00058352
		mov ecx, dword ptr [esp + 0x78]		// 0x00058356
		mov dword ptr [edx + 4], ecx		// 0x0005835a
		mov dword ptr [esp + 0x24], 2		// 0x0005835d
		mov dword ptr [edx + 8], eax		// 0x00058365
L58368:
		mov ecx, dword ptr [esp + 0x1c]		// 0x00058368
		mov edx, dword ptr [esp + 0x10]		// 0x0005836c
		mov esi, dword ptr [esp + 0x14]		// 0x00058370
		inc ecx		// 0x00058374
		add edx, 0xc		// 0x00058375
		add esi, 0xc		// 0x00058378
		cmp ecx, 3		// 0x0005837b
		mov dword ptr [esp + 0x1c], ecx		// 0x0005837e
		mov dword ptr [esp + 0x10], edx		// 0x00058382
		mov dword ptr [esp + 0x14], esi		// 0x00058386
		jb L58177		// 0x0005838a
		mov esi, dword ptr [esp + 0x70]		// 0x00058390
		mov edx, dword ptr [esp + 0x50]		// 0x00058394
L58398:
		mov eax, dword ptr [esp + 0x40]		// 0x00058398
		add esi, 0xc		// 0x0005839c
		dec eax		// 0x0005839f
		mov dword ptr [esp + 0x70], esi		// 0x000583a0
		mov dword ptr [esp + 0x40], eax		// 0x000583a4
		jne L58051		// 0x000583a8
		cmp dword ptr [esp + 0x24], -1		// 0x000583ae
		pop edi		// 0x000583b3
		jne L583c2		// 0x000583b4
L583b6:
		pop esi		// 0x000583b6
		pop ebp		// 0x000583b7
		xor al, al		// 0x000583b8
		pop ebx		// 0x000583ba
		add esp, 0xa0		// 0x000583bb
		ret		// 0x000583c1
L583c2:
		mov ecx, dword ptr [esp + 0x14]		// 0x000583c2
		mov edx, dword ptr [esp + 0xc8]		// 0x000583c6
		pop esi		// 0x000583cd
		pop ebp		// 0x000583ce
		mov dword ptr [edx + 0x18], ecx		// 0x000583cf
		mov al, 1		// 0x000583d2
		pop ebx		// 0x000583d4
		add esp, 0xa0		// 0x000583d5
		ret		// 0x000583db
		}
	}

// phys_fn_002296 (0x000583e0, 1092 B), 276 instructions
__declspec(naked) unsigned __fastcall nxP72296(void* self, void* edxUnused)
	{
	__asm
		{
		sub esp, 0x104		// 0x000583e0
		push ebx		// 0x000583e6
		push ebp		// 0x000583e7
		mov ebp, dword ptr [esp + 0x130]		// 0x000583e8
		push esi		// 0x000583ef
		push edi		// 0x000583f0
		mov edi, eax		// 0x000583f1
		mov eax, 0x38e38e39		// 0x000583f3
		mov dword ptr [ebp + 0x1c], 0xffffffff		// 0x000583f8
		mul dword ptr [edi + 4]		// 0x000583ff
		mov esi, edx		// 0x00058402
		shr esi, 1		// 0x00058404
		test esi, esi		// 0x00058406
		mov byte ptr [esp + 0x13], 0		// 0x00058408
		je L58524		// 0x0005840d
		test ecx, ecx		// 0x00058413
		mov eax, dword ptr [esp + 0x118]		// 0x00058415
		mov edi, dword ptr [edi + 8]		// 0x0005841c
		mov ebx, dword ptr [eax + 8]		// 0x0005841f
		je L5842d		// 0x00058422
		mov ecx, dword ptr [ecx + 8]		// 0x00058424
		mov dword ptr [esp + 0x14], ecx		// 0x00058427
		jmp L58435		// 0x0005842b
L5842d:
		mov dword ptr [esp + 0x14], 0		// 0x0005842d
L58435:
		mov edx, dword ptr [esp + 0x130]		// 0x00058435
		mov eax, dword ptr [esp + 0x12c]		// 0x0005843c
		push edx		// 0x00058443
		push eax		// 0x00058444
		lea ecx, [esp + 0x98]		// 0x00058445
		call nxP7CallNxPOSetCenterExtents		// 0x0005844c
		lea ecx, [esp + 0xa8]		// 0x00058451
		push ecx		// 0x00058458
		lea ecx, [esp + 0x94]		// 0x00058459
		call nxP7CallComputePointsX		// 0x00058460
		test esi, esi		// 0x00058465
		mov dword ptr [esp + 0x18], 0		// 0x00058467
		jbe L58524		// 0x0005846f
L58475:
		mov edx, dword ptr [ebp + 0x18]		// 0x00058475
		mov ecx, dword ptr [ebx]		// 0x00058478
		mov dword ptr [esp + 0x34], edx		// 0x0005847a
		lea edx, [esp + 0x1c]		// 0x0005847e
		push edx		// 0x00058482
		mov edx, dword ptr [esp + 0x138]		// 0x00058483
		push edx		// 0x0005848a
		lea edx, [esp + 0xb0]		// 0x0005848b
		push edx		// 0x00058492
		lea edx, [esp + 0x9c]		// 0x00058493
		push edx		// 0x0005849a
		push ecx		// 0x0005849b
		mov ecx, dword ptr [esp + 0x28]		// 0x0005849c
		mov eax, edi		// 0x000584a0
		push ecx		// 0x000584a2
		push eax		// 0x000584a3
		add edi, 0x24		// 0x000584a4
		add ebx, 4		// 0x000584a7
		call nxP72290		// 0x000584aa
		add esp, 0x1c		// 0x000584af
		test al, al		// 0x000584b2
		je L58504		// 0x000584b4
		fld dword ptr [esp + 0x34]		// 0x000584b6
		fcomp dword ptr [ebp + 0x18]		// 0x000584ba
		fnstsw ax		// 0x000584bd
		test ah, 5		// 0x000584bf
		jp L58504		// 0x000584c2
		mov edx, dword ptr [esp + 0x34]		// 0x000584c4
		mov ecx, dword ptr [esp + 0x28]		// 0x000584c8
		mov dword ptr [ebp + 0x18], edx		// 0x000584cc
		mov edx, dword ptr [esp + 0x2c]		// 0x000584cf
		lea eax, [ebp + 0xc]		// 0x000584d3
		mov dword ptr [eax], ecx		// 0x000584d6
		mov ecx, dword ptr [esp + 0x30]		// 0x000584d8
		mov dword ptr [eax + 4], edx		// 0x000584dc
		mov dword ptr [eax + 8], ecx		// 0x000584df
		mov eax, dword ptr [esp + 0x1c]		// 0x000584e2
		mov ecx, dword ptr [esp + 0x20]		// 0x000584e6
		mov edx, ebp		// 0x000584ea
		mov dword ptr [edx], eax		// 0x000584ec
		mov eax, dword ptr [esp + 0x24]		// 0x000584ee
		mov dword ptr [edx + 4], ecx		// 0x000584f2
		mov ecx, dword ptr [esp + 0x18]		// 0x000584f5
		mov dword ptr [edx + 8], eax		// 0x000584f9
		mov byte ptr [esp + 0x13], 1		// 0x000584fc
		mov dword ptr [ebp + 0x1c], ecx		// 0x00058501
L58504:
		mov eax, dword ptr [esp + 0x14]		// 0x00058504
		test eax, eax		// 0x00058508
		je L58513		// 0x0005850a
		add eax, 0x24		// 0x0005850c
		mov dword ptr [esp + 0x14], eax		// 0x0005850f
L58513:
		mov eax, dword ptr [esp + 0x18]		// 0x00058513
		inc eax		// 0x00058517
		cmp eax, esi		// 0x00058518
		mov dword ptr [esp + 0x18], eax		// 0x0005851a
		jb L58475		// 0x0005851e
L58524:
		mov eax, dword ptr [esp + 0x11c]		// 0x00058524
		test eax, eax		// 0x0005852b
		je L58724		// 0x0005852d
		mov ebx, dword ptr [esp + 0x120]		// 0x00058533
		test ebx, ebx		// 0x0005853a
		je L58724		// 0x0005853c
		call nxP7CallNxIceIdentityPoses		// 0x00058542
		mov ecx, dword ptr [esp + 0x12c]		// 0x00058547
		mov edx, dword ptr [ecx]		// 0x0005854e
		mov dword ptr [esp + 0xa8], edx		// 0x00058550
		mov edx, dword ptr [ecx + 4]		// 0x00058557
		mov ecx, dword ptr [ecx + 8]		// 0x0005855a
		mov dword ptr [esp + 0xac], edx		// 0x0005855d
		mov dword ptr [esp + 0xb0], ecx		// 0x00058564
		mov ecx, dword ptr [esp + 0x130]		// 0x0005856b
		mov edx, dword ptr [ecx]		// 0x00058572
		mov dword ptr [esp + 0xb4], edx		// 0x00058574
		mov edx, dword ptr [ecx + 4]		// 0x0005857b
		mov ecx, dword ptr [ecx + 8]		// 0x0005857e
		mov dword ptr [esp + 0xb8], edx		// 0x00058581
		xor edx, edx		// 0x00058588
		mov dword ptr [esp + 0xbc], ecx		// 0x0005858a
		mov ecx, 9		// 0x00058591
		mov esi, eax		// 0x00058596
		lea edi, [esp + 0xc0]		// 0x00058598
		rep movsd		// 0x0005859f
		mov edi, dword ptr [esp + 0x11c]		// 0x000585a1
		mov dword ptr [esp + 0x6c], edx		// 0x000585a8
		mov dword ptr [esp + 0x70], edx		// 0x000585ac
		mov dword ptr [esp + 0x74], edx		// 0x000585b0
		mov dword ptr [esp + 0x78], edx		// 0x000585b4
		mov dword ptr [esp + 0x7c], edx		// 0x000585b8
		mov dword ptr [esp + 0x80], edx		// 0x000585bc
		mov dword ptr [esp + 0x84], edx		// 0x000585c3
		mov dword ptr [esp + 0x88], edx		// 0x000585ca
		mov dword ptr [esp + 0x8c], edx		// 0x000585d1
		mov dword ptr [esp + 0x8c], 0x3f800000		// 0x000585d8
		mov dword ptr [esp + 0x7c], 0x3f800000		// 0x000585e3
		mov dword ptr [esp + 0x6c], 0x3f800000		// 0x000585eb
		lea esi, [ebx + 0x14]		// 0x000585f3
		jmp L58600		// 0x000585f6
L585f8:
		mov ebx, dword ptr [esp + 0x120]		// 0x000585f8
		nop		// 0x000585ff
L58600:
		fld dword ptr [ebx]		// 0x00058600
		fadd dword ptr [esi - 8]		// 0x00058602
		fld dword ptr [esi - 4]		// 0x00058605
		fadd dword ptr [esi - 0x10]		// 0x00058608
		fld dword ptr [esi - 0xc]		// 0x0005860b
		fadd dword ptr [esi]		// 0x0005860e
		fstp dword ptr [esp + 0x110]		// 0x00058610
		fxch st(1)		// 0x00058617
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058619
		fstp dword ptr [esp + 0x48]		// 0x0005861f
		mov eax, dword ptr [esp + 0x48]		// 0x00058623
		mov dword ptr [esp + 0x54], eax		// 0x00058627
		fmul dword ptr [nxP7K101043cc_4]		// 0x0005862b
		fstp dword ptr [esp + 0x4c]		// 0x00058631
		mov ecx, dword ptr [esp + 0x4c]		// 0x00058635
		fld dword ptr [esp + 0x110]		// 0x00058639
		mov dword ptr [esp + 0x58], ecx		// 0x00058640
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058644
		fstp dword ptr [esp + 0x50]		// 0x0005864a
		mov edx, dword ptr [esp + 0x50]		// 0x0005864e
		fld dword ptr [esi - 8]		// 0x00058652
		mov dword ptr [esp + 0x5c], edx		// 0x00058655
		fsub dword ptr [ebx]		// 0x00058659
		mov ebx, dword ptr [esp + 0x134]		// 0x0005865b
		fld dword ptr [esi - 4]		// 0x00058662
		fsub dword ptr [esi - 0x10]		// 0x00058665
		fld dword ptr [esi]		// 0x00058668
		fsub dword ptr [esi - 0xc]		// 0x0005866a
		fstp dword ptr [esp + 0x98]		// 0x0005866d
		fxch st(1)		// 0x00058674
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058676
		fstp dword ptr [esp + 0x3c]		// 0x0005867c
		mov eax, dword ptr [esp + 0x3c]		// 0x00058680
		mov dword ptr [esp + 0x60], eax		// 0x00058684
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058688
		lea eax, [esp + 0x1c]		// 0x0005868e
		push eax		// 0x00058692
		fstp dword ptr [esp + 0x44]		// 0x00058693
		mov ecx, dword ptr [esp + 0x44]		// 0x00058697
		fld dword ptr [esp + 0x9c]		// 0x0005869b
		mov dword ptr [esp + 0x68], ecx		// 0x000586a2
		fmul dword ptr [nxP7K101043cc_4]		// 0x000586a6
		lea ecx, [esp + 0x58]		// 0x000586ac
		push ecx		// 0x000586b0
		fstp dword ptr [esp + 0x4c]		// 0x000586b1
		mov edx, dword ptr [esp + 0x4c]		// 0x000586b5
		mov dword ptr [esp + 0x70], edx		// 0x000586b9
		lea edx, [esp + 0xb0]		// 0x000586bd
		push edx		// 0x000586c4
		call nxP72282		// 0x000586c5
		add esp, 0xc		// 0x000586ca
		test al, al		// 0x000586cd
		je L58709		// 0x000586cf
		fld dword ptr [esp + 0x34]		// 0x000586d1
		fcomp dword ptr [ebp + 0x18]		// 0x000586d5
		fnstsw ax		// 0x000586d8
		test ah, 5		// 0x000586da
		jp L58709		// 0x000586dd
		mov eax, dword ptr [esp + 0x34]		// 0x000586df
		mov edx, dword ptr [esp + 0x28]		// 0x000586e3
		lea ecx, [ebp + 0xc]		// 0x000586e7
		mov dword ptr [ecx], edx		// 0x000586ea
		mov edx, dword ptr [esp + 0x30]		// 0x000586ec
		mov dword ptr [ebp + 0x18], eax		// 0x000586f0
		mov eax, dword ptr [esp + 0x2c]		// 0x000586f3
		mov dword ptr [ecx + 4], eax		// 0x000586f7
		mov dword ptr [ecx + 8], edx		// 0x000586fa
		mov byte ptr [esp + 0x13], 1		// 0x000586fd
		mov dword ptr [ebp + 0x1c], 0xffffffff		// 0x00058702
L58709:
		mov edx, dword ptr [esp + 0x120]		// 0x00058709
		add edx, 0x18		// 0x00058710
		add esi, 0x18		// 0x00058713
		dec edi		// 0x00058716
		mov dword ptr [esp + 0x120], edx		// 0x00058717
		jne L585f8		// 0x0005871e
L58724:
		mov ebx, dword ptr [esp + 0x124]		// 0x00058724
		test ebx, ebx		// 0x0005872b
		je L58815		// 0x0005872d
		mov eax, dword ptr [esp + 0x128]		// 0x00058733
		test eax, eax		// 0x0005873a
		je L58815		// 0x0005873c
		call nxP7CallNxIceIdentityPoses		// 0x00058742
		mov ecx, dword ptr [esp + 0x12c]		// 0x00058747
		mov edx, dword ptr [ecx]		// 0x0005874e
		mov dword ptr [esp + 0xa8], edx		// 0x00058750
		mov edx, dword ptr [ecx + 4]		// 0x00058757
		mov ecx, dword ptr [ecx + 8]		// 0x0005875a
		mov dword ptr [esp + 0xb0], ecx		// 0x0005875d
		mov ecx, dword ptr [esp + 0x130]		// 0x00058764
		mov dword ptr [esp + 0xac], edx		// 0x0005876b
		mov edx, dword ptr [ecx]		// 0x00058772
		mov dword ptr [esp + 0xb4], edx		// 0x00058774
		mov edx, dword ptr [ecx + 4]		// 0x0005877b
		mov ecx, dword ptr [ecx + 8]		// 0x0005877e
		mov dword ptr [esp + 0xbc], ecx		// 0x00058781
		mov ecx, 9		// 0x00058788
		mov esi, eax		// 0x0005878d
		lea edi, [esp + 0xc0]		// 0x0005878f
		mov dword ptr [esp + 0xb8], edx		// 0x00058796
		rep movsd		// 0x0005879d
		mov esi, ebx		// 0x0005879f
L587a1:
		mov eax, dword ptr [esp + 0x128]		// 0x000587a1
		mov ebx, dword ptr [esp + 0x134]		// 0x000587a8
		lea edx, [esp + 0x1c]		// 0x000587af
		push edx		// 0x000587b3
		push eax		// 0x000587b4
		lea ecx, [esp + 0xb0]		// 0x000587b5
		push ecx		// 0x000587bc
		call nxP72282		// 0x000587bd
		add esp, 0xc		// 0x000587c2
		test al, al		// 0x000587c5
		je L58801		// 0x000587c7
		fld dword ptr [esp + 0x34]		// 0x000587c9
		fcomp dword ptr [ebp + 0x18]		// 0x000587cd
		fnstsw ax		// 0x000587d0
		test ah, 5		// 0x000587d2
		jp L58801		// 0x000587d5
		mov edx, dword ptr [esp + 0x34]		// 0x000587d7
		mov ecx, dword ptr [esp + 0x28]		// 0x000587db
		lea eax, [ebp + 0xc]		// 0x000587df
		mov dword ptr [eax], ecx		// 0x000587e2
		mov ecx, dword ptr [esp + 0x30]		// 0x000587e4
		mov dword ptr [ebp + 0x18], edx		// 0x000587e8
		mov edx, dword ptr [esp + 0x2c]		// 0x000587eb
		mov dword ptr [eax + 4], edx		// 0x000587ef
		mov dword ptr [eax + 8], ecx		// 0x000587f2
		mov byte ptr [esp + 0x13], 1		// 0x000587f5
		mov dword ptr [ebp + 0x1c], 0xffffffff		// 0x000587fa
L58801:
		mov ecx, dword ptr [esp + 0x128]		// 0x00058801
		add ecx, 0x3c		// 0x00058808
		dec esi		// 0x0005880b
		mov dword ptr [esp + 0x128], ecx		// 0x0005880c
		jne L587a1		// 0x00058813
L58815:
		mov al, byte ptr [esp + 0x13]		// 0x00058815
		pop edi		// 0x00058819
		pop esi		// 0x0005881a
		pop ebp		// 0x0005881b
		pop ebx		// 0x0005881c
		add esp, 0x104		// 0x0005881d
		ret		// 0x00058823
		}
	}

// phys_fn_002298 (0x00058830, 63 B), 23 instructions
__declspec(naked) unsigned __fastcall nxP72298(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5, unsigned a6, unsigned a7, unsigned a8, unsigned a9)
	{
	__asm
		{
		mov eax, dword ptr [esp + 0x28]		// 0x00058830
		mov edx, dword ptr [esp + 0x24]		// 0x00058834
		push eax		// 0x00058838
		mov eax, dword ptr [esp + 0x24]		// 0x00058839
		push edx		// 0x0005883d
		mov edx, dword ptr [esp + 0x20]		// 0x0005883e
		add ecx, 0x10		// 0x00058842
		push ecx		// 0x00058845
		mov ecx, dword ptr [esp + 0x28]		// 0x00058846
		push eax		// 0x0005884a
		mov eax, dword ptr [esp + 0x24]		// 0x0005884b
		push ecx		// 0x0005884f
		mov ecx, dword ptr [esp + 0x24]		// 0x00058850
		push edx		// 0x00058854
		mov edx, dword ptr [esp + 0x24]		// 0x00058855
		push eax		// 0x00058859
		mov eax, dword ptr [esp + 0x20]		// 0x0005885a
		push ecx		// 0x0005885e
		mov ecx, dword ptr [esp + 0x28]		// 0x0005885f
		push edx		// 0x00058863
		call nxP72296		// 0x00058864
		add esp, 0x24		// 0x00058869
		ret 0x28		// 0x0005886c
		}
	}

// phys_fn_002300 and phys_fn_002302 (one function) (0x00058870, 1573 B), 423 instructions
__declspec(naked) unsigned __fastcall nxP72300(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5, unsigned a6, unsigned a7, unsigned a8, unsigned a9, unsigned a10, unsigned a11, unsigned a12, unsigned a13, unsigned a14, unsigned a15)
	{
	__asm
		{
		sub esp, 0xe0		// 0x00058870
		push ebx		// 0x00058876
		mov ebx, dword ptr [esp + 0x10c]		// 0x00058877
		push ebp		// 0x0005887e
		lea ebp, [ebx + 4]		// 0x0005887f
		push edi		// 0x00058882
		mov edi, ecx		// 0x00058883
		mov byte ptr [edi + 0x40], 0		// 0x00058885
		mov ecx, dword ptr [ebp + 4]		// 0x00058889
		mov eax, dword ptr [ebp]		// 0x0005888c
		mov edx, dword ptr [ebp + 8]		// 0x0005888f
		mov dword ptr [esp + 0x24], ecx		// 0x00058892
		mov ecx, dword ptr [esp + 0x118]		// 0x00058896
		fld dword ptr [ecx]		// 0x0005889d
		mov dword ptr [esp + 0x20], eax		// 0x0005889f
		fadd dword ptr [ebp]		// 0x000588a3
		mov eax, dword ptr [esp + 0x124]		// 0x000588a6
		test eax, eax		// 0x000588ad
		mov dword ptr [esp + 0x8c], edi		// 0x000588af
		fstp dword ptr [esp + 0x10]		// 0x000588b6
		mov byte ptr [esp + 0xf], 0		// 0x000588ba
		fld dword ptr [ebp + 4]		// 0x000588bf
		mov dword ptr [esp + 0x28], edx		// 0x000588c2
		fadd dword ptr [ecx + 4]		// 0x000588c6
		mov dword ptr [esp + 0x50], 0		// 0x000588c9
		fstp dword ptr [esp + 0x14]		// 0x000588d1
		fld dword ptr [ebp + 8]		// 0x000588d5
		fadd dword ptr [ecx + 8]		// 0x000588d8
		fstp dword ptr [esp + 0x18]		// 0x000588db
		je L58e5f		// 0x000588df
		mov edx, dword ptr [nxP7SweepCache]		// 0x000588e5
		push esi		// 0x000588eb
		jmp L58900		// 0x000588ec
L588ee:
		mov ecx, dword ptr [esp + 0x11c]		// 0x000588ee
		jmp L58900		// 0x000588f5
		lea esp, [esp]		// 0x000588f7
		mov edi, edi		// 0x000588fe
L58900:
		fld dword ptr [esp + 0x14]		// 0x00058900
		mov eax, dword ptr [esp + 0x128]		// 0x00058904
		fsub dword ptr [esp + 0x24]		// 0x0005890b
		dec eax		// 0x0005890f
		fld dword ptr [esp + 0x18]		// 0x00058910
		mov dword ptr [esp + 0x128], eax		// 0x00058914
		fsub dword ptr [esp + 0x28]		// 0x0005891b
		fstp dword ptr [esp + 0x34]		// 0x0005891f
		fld dword ptr [esp + 0x1c]		// 0x00058923
		fsub dword ptr [esp + 0x2c]		// 0x00058927
		fld st(0)		// 0x0005892b
		fmul st(0), st(1)		// 0x0005892d
		fld dword ptr [esp + 0x34]		// 0x0005892f
		fmul dword ptr [esp + 0x34]		// 0x00058933
		faddp st(1), st(0)		// 0x00058937
		fld st(2)		// 0x00058939
		fmul st(0), st(3)		// 0x0005893b
		faddp st(1), st(0)		// 0x0005893d
		fsqrt		// 0x0005893f
		fst dword ptr [esp + 0x20]		// 0x00058941
		fcomp dword ptr [esp + 0x130]		// 0x00058945
		fnstsw ax		// 0x0005894c
		test ah, 5		// 0x0005894e
		jnp L58e5a		// 0x00058951
		fld dword ptr [nxP7K101041ec_4]		// 0x00058957
		fdiv dword ptr [esp + 0x20]		// 0x0005895d
		fxch st(2)		// 0x00058961
		fmul st(0), st(2)		// 0x00058963
		fstp dword ptr [esp + 0x30]		// 0x00058965
		fld dword ptr [esp + 0x34]		// 0x00058969
		fmul st(0), st(2)		// 0x0005896d
		fstp dword ptr [esp + 0x34]		// 0x0005896f
		fmul st(0), st(1)		// 0x00058973
		fstp dword ptr [esp + 0x38]		// 0x00058975
		fstp st(0)		// 0x00058979
		fld dword ptr [esp + 0x38]		// 0x0005897b
		fmul dword ptr [ecx + 8]		// 0x0005897f
		fld dword ptr [esp + 0x34]		// 0x00058982
		fmul dword ptr [ecx + 4]		// 0x00058986
		faddp st(1), st(0)		// 0x00058989
		fld dword ptr [esp + 0x30]		// 0x0005898b
		fmul dword ptr [ecx]		// 0x0005898f
		faddp st(1), st(0)		// 0x00058991
		fcomp dword ptr [nxP7K101041f0_4]		// 0x00058993
		fnstsw ax		// 0x00058999
		test ah, 0x41		// 0x0005899b
		jnp L58e5e		// 0x0005899e
		fld dword ptr [esp + 0x20]		// 0x000589a4
		mov esi, dword ptr [esp + 0xf8]		// 0x000589a8
		fadd dword ptr [edx + 0xc]		// 0x000589af
		mov eax, dword ptr [ebx]		// 0x000589b2
		lea ecx, [esp + 0x64]		// 0x000589b4
		push ecx		// 0x000589b8
		lea edx, [esp + 0x34]		// 0x000589b9
		fstp dword ptr [esp + 0x80]		// 0x000589bd
		push edx		// 0x000589c4
		mov edx, dword ptr [esp + 0x11c]		// 0x000589c5
		lea ecx, [esp + 0x2c]		// 0x000589cc
		push ecx		// 0x000589d0
		mov ecx, dword ptr [esp + 0x11c]		// 0x000589d1
		push edx		// 0x000589d8
		mov edx, dword ptr [esp + 0x11c]		// 0x000589d9
		push ecx		// 0x000589e0
		mov ecx, dword ptr [esp + 0x11c]		// 0x000589e1
		push edx		// 0x000589e8
		mov edx, dword ptr [esp + 0x118]		// 0x000589e9
		push ecx		// 0x000589f0
		mov ecx, dword ptr [esp + 0x118]		// 0x000589f1
		push edx		// 0x000589f8
		push ecx		// 0x000589f9
		push esi		// 0x000589fa
		mov ecx, ebx		// 0x000589fb
		mov byte ptr [esp + 0x3b], 1		// 0x000589fd
		call dword ptr [eax]		// 0x00058a02
		test al, al		// 0x00058a04
		je L58e40		// 0x00058a06
		mov eax, dword ptr [esp + 0x80]		// 0x00058a0c
		cmp eax, -1		// 0x00058a13
		je L58a53		// 0x00058a16
		lea edx, [eax + eax*8]		// 0x00058a18
		mov byte ptr [edi + 0x40], 1		// 0x00058a1b
		mov eax, dword ptr [esi + 8]		// 0x00058a1f
		lea esi, [eax + edx*4]		// 0x00058a22
		mov edx, dword ptr [esp + 0x104]		// 0x00058a25
		add edi, 0x44		// 0x00058a2c
		mov ecx, 9		// 0x00058a2f
		rep movsd		// 0x00058a34
		mov ecx, dword ptr [esp + 0x20]		// 0x00058a36
		push ecx		// 0x00058a3a
		push edx		// 0x00058a3b
		lea ecx, [esp + 0x38]		// 0x00058a3c
		lea eax, [esp + 0x6c]		// 0x00058a40
		call nxP72288		// 0x00058a44
		mov edi, dword ptr [esp + 0x98]		// 0x00058a49
		add esp, 8		// 0x00058a50
L58a53:
		mov edx, dword ptr [nxP7SweepCache]		// 0x00058a53
		fld dword ptr [esp + 0x7c]		// 0x00058a59
		mov eax, dword ptr [esp + 0x54]		// 0x00058a5d
		fcomp dword ptr [edx + 0xc]		// 0x00058a61
		inc eax		// 0x00058a64
		mov dword ptr [esp + 0x54], eax		// 0x00058a65
		fnstsw ax		// 0x00058a69
		test ah, 0x41		// 0x00058a6b
		jne L58abf		// 0x00058a6e
		fld dword ptr [esp + 0x7c]		// 0x00058a70
		fsub dword ptr [edx + 0xc]		// 0x00058a74
		fld dword ptr [esp + 0x30]		// 0x00058a77
		fmul st(0), st(1)		// 0x00058a7b
		fld dword ptr [esp + 0x34]		// 0x00058a7d
		fmul st(0), st(2)		// 0x00058a81
		fstp dword ptr [esp + 0xb8]		// 0x00058a83
		fld dword ptr [esp + 0x38]		// 0x00058a8a
		fmul st(0), st(2)		// 0x00058a8e
		fstp dword ptr [esp + 0xbc]		// 0x00058a90
		fadd dword ptr [esp + 0x24]		// 0x00058a97
		fstp dword ptr [esp + 0x24]		// 0x00058a9b
		fstp st(0)		// 0x00058a9f
		fld dword ptr [esp + 0xb8]		// 0x00058aa1
		fadd dword ptr [esp + 0x28]		// 0x00058aa8
		fstp dword ptr [esp + 0x28]		// 0x00058aac
		fld dword ptr [esp + 0xbc]		// 0x00058ab0
		fadd dword ptr [esp + 0x2c]		// 0x00058ab7
		fstp dword ptr [esp + 0x2c]		// 0x00058abb
L58abf:
		fld dword ptr [esp + 0x70]		// 0x00058abf
		mov eax, dword ptr [esp + 0x124]		// 0x00058ac3
		fadd st(0), st(0)		// 0x00058aca
		mov ecx, dword ptr [esp + 0x120]		// 0x00058acc
		fld dword ptr [esp + 0x74]		// 0x00058ad3
		mov dword ptr [esp + 0xb0], eax		// 0x00058ad7
		fadd st(0), st(0)		// 0x00058ade
		mov dword ptr [esp + 0x94], ecx		// 0x00058ae0
		fld dword ptr [esp + 0x78]		// 0x00058ae7
		fadd st(0), st(0)		// 0x00058aeb
		fstp dword ptr [esp + 0xe0]		// 0x00058aed
		fld dword ptr [esp + 0x78]		// 0x00058af4
		fmul dword ptr [esp + 0x38]		// 0x00058af8
		fld dword ptr [esp + 0x74]		// 0x00058afc
		fmul dword ptr [esp + 0x34]		// 0x00058b00
		faddp st(1), st(0)		// 0x00058b04
		fld dword ptr [esp + 0x70]		// 0x00058b06
		fmul dword ptr [esp + 0x30]		// 0x00058b0a
		faddp st(1), st(0)		// 0x00058b0e
		fstp dword ptr [esp + 0x20]		// 0x00058b10
		fxch st(1)		// 0x00058b14
		fmul dword ptr [esp + 0x20]		// 0x00058b16
		fstp dword ptr [esp + 0xe4]		// 0x00058b1a
		fmul dword ptr [esp + 0x20]		// 0x00058b21
		fld dword ptr [esp + 0xe0]		// 0x00058b25
		fmul dword ptr [esp + 0x20]		// 0x00058b2c
		fld dword ptr [esp + 0x30]		// 0x00058b30
		fsub dword ptr [esp + 0xe4]		// 0x00058b34
		fstp dword ptr [esp + 0xa4]		// 0x00058b3b
		mov eax, dword ptr [esp + 0xa4]		// 0x00058b42
		fld dword ptr [esp + 0x34]		// 0x00058b49
		mov dword ptr [esp + 0x58], eax		// 0x00058b4d
		fsub st(0), st(2)		// 0x00058b51
		fstp dword ptr [esp + 0xa8]		// 0x00058b53
		mov ecx, dword ptr [esp + 0xa8]		// 0x00058b5a
		fld dword ptr [esp + 0x38]		// 0x00058b61
		mov dword ptr [esp + 0x5c], ecx		// 0x00058b65
		fsub st(0), st(1)		// 0x00058b69
		fstp dword ptr [esp + 0xac]		// 0x00058b6b
		mov eax, dword ptr [esp + 0xac]		// 0x00058b72
		mov dword ptr [esp + 0x60], eax		// 0x00058b79
		fstp st(0)		// 0x00058b7d
		fstp st(0)		// 0x00058b7f
		fld dword ptr [esp + 0x58]		// 0x00058b81
		fld dword ptr [esp + 0x5c]		// 0x00058b85
		fld dword ptr [esp + 0x60]		// 0x00058b89
		fmul dword ptr [esp + 0x60]		// 0x00058b8d
		fld st(1)		// 0x00058b91
		fmul st(0), st(2)		// 0x00058b93
		faddp st(1), st(0)		// 0x00058b95
		fld st(2)		// 0x00058b97
		fmul st(0), st(3)		// 0x00058b99
		faddp st(1), st(0)		// 0x00058b9b
		fld dword ptr [nxP7K101041f0_4]		// 0x00058b9d
		fld st(1)		// 0x00058ba3
		fucompp		// 0x00058ba5
		fnstsw ax		// 0x00058ba7
		test ah, 0x44		// 0x00058ba9
		jnp L58bca		// 0x00058bac
		fsqrt		// 0x00058bae
		fdivr dword ptr [nxP7K101041ec_4]		// 0x00058bb0
		fld st(0)		// 0x00058bb6
		fmulp st(3), st(0)		// 0x00058bb8
		fxch st(1)		// 0x00058bba
		fmul st(0), st(1)		// 0x00058bbc
		fxch st(1)		// 0x00058bbe
		fld dword ptr [esp + 0x60]		// 0x00058bc0
		fmul st(0), st(1)		// 0x00058bc4
		fstp dword ptr [esp + 0x60]		// 0x00058bc6
L58bca:
		fstp st(0)		// 0x00058bca
		fld dword ptr [esp + 0x74]		// 0x00058bcc
		fmul st(0), st(1)		// 0x00058bd0
		fld dword ptr [esp + 0x60]		// 0x00058bd2
		fmul dword ptr [esp + 0x78]		// 0x00058bd6
		faddp st(1), st(0)		// 0x00058bda
		fld dword ptr [esp + 0x70]		// 0x00058bdc
		fmul st(0), st(3)		// 0x00058be0
		faddp st(1), st(0)		// 0x00058be2
		fld dword ptr [esp + 0x70]		// 0x00058be4
		fmul st(0), st(1)		// 0x00058be8
		fstp dword ptr [esp + 0x84]		// 0x00058bea
		mov ecx, dword ptr [esp + 0x84]		// 0x00058bf1
		fld dword ptr [esp + 0x74]		// 0x00058bf8
		mov dword ptr [esp + 0x3c], ecx		// 0x00058bfc
		fmul st(0), st(1)		// 0x00058c00
		fstp dword ptr [esp + 0x88]		// 0x00058c02
		mov eax, dword ptr [esp + 0x88]		// 0x00058c09
		fld dword ptr [esp + 0x78]		// 0x00058c10
		mov dword ptr [esp + 0x40], eax		// 0x00058c14
		fmul st(0), st(1)		// 0x00058c18
		fstp dword ptr [esp + 0x8c]		// 0x00058c1a
		mov ecx, dword ptr [esp + 0x8c]		// 0x00058c21
		mov dword ptr [esp + 0x44], ecx		// 0x00058c28
		fstp st(0)		// 0x00058c2c
		fxch st(1)		// 0x00058c2e
		fsub dword ptr [esp + 0x3c]		// 0x00058c30
		fstp dword ptr [esp + 0x98]		// 0x00058c34
		mov eax, dword ptr [esp + 0x98]		// 0x00058c3b
		mov dword ptr [esp + 0x48], eax		// 0x00058c42
		fsub dword ptr [esp + 0x40]		// 0x00058c46
		fstp dword ptr [esp + 0x9c]		// 0x00058c4a
		mov ecx, dword ptr [esp + 0x9c]		// 0x00058c51
		fld dword ptr [esp + 0x60]		// 0x00058c58
		mov dword ptr [esp + 0x4c], ecx		// 0x00058c5c
		fsub dword ptr [esp + 0x44]		// 0x00058c60
		fstp dword ptr [esp + 0xa0]		// 0x00058c64
		mov eax, dword ptr [esp + 0xa0]		// 0x00058c6b
		mov dword ptr [esp + 0x50], eax		// 0x00058c72
		mov al, byte ptr [nxP7SweepGuard]		// 0x00058c76
		test al, al		// 0x00058c7b
		je L58d2d		// 0x00058c7d
		fld dword ptr [esp + 0x40]		// 0x00058c83
		fmul dword ptr [esp + 0x40]		// 0x00058c87
		fld dword ptr [esp + 0x44]		// 0x00058c8b
		fmul dword ptr [esp + 0x44]		// 0x00058c8f
		faddp st(1), st(0)		// 0x00058c93
		fld dword ptr [esp + 0x3c]		// 0x00058c95
		fmul dword ptr [esp + 0x3c]		// 0x00058c99
		faddp st(1), st(0)		// 0x00058c9d
		fld dword ptr [nxP7K101041f0_4]		// 0x00058c9f
		fld st(1)		// 0x00058ca5
		fucompp		// 0x00058ca7
		fnstsw ax		// 0x00058ca9
		test ah, 0x44		// 0x00058cab
		jnp L58cd6		// 0x00058cae
		fsqrt		// 0x00058cb0
		fdivr dword ptr [nxP7K101041ec_4]		// 0x00058cb2
		fld dword ptr [esp + 0x3c]		// 0x00058cb8
		fmul st(0), st(1)		// 0x00058cbc
		fstp dword ptr [esp + 0x3c]		// 0x00058cbe
		fld dword ptr [esp + 0x40]		// 0x00058cc2
		fmul st(0), st(1)		// 0x00058cc6
		fstp dword ptr [esp + 0x40]		// 0x00058cc8
		fld dword ptr [esp + 0x44]		// 0x00058ccc
		fmul st(0), st(1)		// 0x00058cd0
		fstp dword ptr [esp + 0x44]		// 0x00058cd2
L58cd6:
		fstp st(0)		// 0x00058cd6
		fld dword ptr [esp + 0x50]		// 0x00058cd8
		fmul dword ptr [esp + 0x50]		// 0x00058cdc
		fld dword ptr [esp + 0x4c]		// 0x00058ce0
		fmul dword ptr [esp + 0x4c]		// 0x00058ce4
		faddp st(1), st(0)		// 0x00058ce8
		fld dword ptr [esp + 0x48]		// 0x00058cea
		fmul dword ptr [esp + 0x48]		// 0x00058cee
		faddp st(1), st(0)		// 0x00058cf2
		fld dword ptr [nxP7K101041f0_4]		// 0x00058cf4
		fld st(1)		// 0x00058cfa
		fucompp		// 0x00058cfc
		fnstsw ax		// 0x00058cfe
		test ah, 0x44		// 0x00058d00
		jnp L58d2b		// 0x00058d03
		fsqrt		// 0x00058d05
		fdivr dword ptr [nxP7K101041ec_4]		// 0x00058d07
		fld dword ptr [esp + 0x48]		// 0x00058d0d
		fmul st(0), st(1)		// 0x00058d11
		fstp dword ptr [esp + 0x48]		// 0x00058d13
		fld dword ptr [esp + 0x4c]		// 0x00058d17
		fmul st(0), st(1)		// 0x00058d1b
		fstp dword ptr [esp + 0x4c]		// 0x00058d1d
		fld dword ptr [esp + 0x50]		// 0x00058d21
		fmul st(0), st(1)		// 0x00058d25
		fstp dword ptr [esp + 0x50]		// 0x00058d27
L58d2b:
		fstp st(0)		// 0x00058d2b
L58d2d:
		fld dword ptr [esp + 0x14]		// 0x00058d2d
		mov eax, dword ptr [esp + 0x28]		// 0x00058d31
		fsub dword ptr [esp + 0x24]		// 0x00058d35
		mov ecx, dword ptr [esp + 0x24]		// 0x00058d39
		fld dword ptr [esp + 0x1c]		// 0x00058d3d
		mov dword ptr [esp + 0x14], ecx		// 0x00058d41
		fsub dword ptr [esp + 0x2c]		// 0x00058d45
		mov ecx, dword ptr [esp + 0x2c]		// 0x00058d49
		fld dword ptr [esp + 0x18]		// 0x00058d4d
		mov dword ptr [esp + 0x18], eax		// 0x00058d51
		fsub dword ptr [esp + 0x28]		// 0x00058d55
		mov eax, dword ptr [esp + 0x94]		// 0x00058d59
		test eax, eax		// 0x00058d60
		mov dword ptr [esp + 0x1c], ecx		// 0x00058d62
		fld st(0)		// 0x00058d66
		fmul st(0), st(1)		// 0x00058d68
		fld st(2)		// 0x00058d6a
		fmul st(0), st(3)		// 0x00058d6c
		faddp st(1), st(0)		// 0x00058d6e
		fld st(3)		// 0x00058d70
		fmul st(0), st(4)		// 0x00058d72
		faddp st(1), st(0)		// 0x00058d74
		fsqrt		// 0x00058d76
		fstp st(3)		// 0x00058d78
		fstp st(0)		// 0x00058d7a
		fstp st(0)		// 0x00058d7c
		je L58dd1		// 0x00058d7e
		fld dword ptr [esp + 0x120]		// 0x00058d80
		fmul st(0), st(1)		// 0x00058d87
		fld dword ptr [esp + 0x3c]		// 0x00058d89
		fmul st(0), st(1)		// 0x00058d8d
		fld dword ptr [esp + 0x40]		// 0x00058d8f
		fmul st(0), st(2)		// 0x00058d93
		fstp dword ptr [esp + 0xc4]		// 0x00058d95
		fld dword ptr [esp + 0x44]		// 0x00058d9c
		fmul st(0), st(2)		// 0x00058da0
		fstp dword ptr [esp + 0xc8]		// 0x00058da2
		fadd dword ptr [esp + 0x14]		// 0x00058da9
		fstp dword ptr [esp + 0x14]		// 0x00058dad
		fstp st(0)		// 0x00058db1
		fld dword ptr [esp + 0xc4]		// 0x00058db3
		fadd dword ptr [esp + 0x18]		// 0x00058dba
		fstp dword ptr [esp + 0x18]		// 0x00058dbe
		fld dword ptr [esp + 0xc8]		// 0x00058dc2
		fadd dword ptr [esp + 0x1c]		// 0x00058dc9
		fstp dword ptr [esp + 0x1c]		// 0x00058dcd
L58dd1:
		mov eax, dword ptr [esp + 0xb0]		// 0x00058dd1
		test eax, eax		// 0x00058dd8
		je L58e2d		// 0x00058dda
		fmul dword ptr [esp + 0x124]		// 0x00058ddc
		fld dword ptr [esp + 0x48]		// 0x00058de3
		fmul st(0), st(1)		// 0x00058de7
		fld dword ptr [esp + 0x4c]		// 0x00058de9
		fmul st(0), st(2)		// 0x00058ded
		fstp dword ptr [esp + 0xd0]		// 0x00058def
		fld dword ptr [esp + 0x50]		// 0x00058df6
		fmul st(0), st(2)		// 0x00058dfa
		fstp dword ptr [esp + 0xd4]		// 0x00058dfc
		fadd dword ptr [esp + 0x14]		// 0x00058e03
		fstp dword ptr [esp + 0x14]		// 0x00058e07
		fstp st(0)		// 0x00058e0b
		fld dword ptr [esp + 0xd0]		// 0x00058e0d
		fadd dword ptr [esp + 0x18]		// 0x00058e14
		fstp dword ptr [esp + 0x18]		// 0x00058e18
		fld dword ptr [esp + 0xd4]		// 0x00058e1c
		fadd dword ptr [esp + 0x1c]		// 0x00058e23
		fstp dword ptr [esp + 0x1c]		// 0x00058e27
		jmp L58e2f		// 0x00058e2b
L58e2d:
		fstp st(0)		// 0x00058e2d
L58e2f:
		mov eax, dword ptr [esp + 0x128]		// 0x00058e2f
		test eax, eax		// 0x00058e36
		jne L588ee		// 0x00058e38
		jmp L58e5e		// 0x00058e3e
L58e40:
		mov edx, dword ptr [esp + 0x14]		// 0x00058e40
		mov eax, dword ptr [esp + 0x18]		// 0x00058e44
		mov ecx, dword ptr [esp + 0x1c]		// 0x00058e48
		mov dword ptr [esp + 0x24], edx		// 0x00058e4c
		mov dword ptr [esp + 0x28], eax		// 0x00058e50
		mov dword ptr [esp + 0x2c], ecx		// 0x00058e54
		jmp L58e5e		// 0x00058e58
L58e5a:
		fstp st(0)		// 0x00058e5a
		fstp st(0)		// 0x00058e5c
L58e5e:
		pop esi		// 0x00058e5e
L58e5f:
		mov eax, dword ptr [esp + 0x128]		// 0x00058e5f
		test eax, eax		// 0x00058e66
		je L58e70		// 0x00058e68
		mov edx, dword ptr [esp + 0x50]		// 0x00058e6a
		mov dword ptr [eax], edx		// 0x00058e6e
L58e70:
		mov eax, dword ptr [esp + 0x20]		// 0x00058e70
		mov ecx, dword ptr [esp + 0x24]		// 0x00058e74
		mov edx, dword ptr [esp + 0x28]		// 0x00058e78
		mov dword ptr [ebp], eax		// 0x00058e7c
		mov al, byte ptr [esp + 0xf]		// 0x00058e7f
		mov dword ptr [ebp + 4], ecx		// 0x00058e83
		pop edi		// 0x00058e86
		mov dword ptr [ebp + 8], edx		// 0x00058e87
		pop ebp		// 0x00058e8a
		pop ebx		// 0x00058e8b
		add esp, 0xe0		// 0x00058e8c
		ret 0x40		// 0x00058e92
		}
	}

// phys_fn_002304 (0x00058ea0, 1140 B), 424 instructions
__declspec(naked) unsigned __fastcall nxP72304(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5, unsigned a6, unsigned a7, unsigned a8, unsigned a9, unsigned a10)
	{
	__asm
		{
		push ebp		// 0x00058ea0
		mov ebp, esp		// 0x00058ea1
		sub esp, 0x54		// 0x00058ea3
		mov edx, dword ptr [ebp + 0x18]		// 0x00058ea6
		push ebx		// 0x00058ea9
		push esi		// 0x00058eaa
		push edi		// 0x00058eab
		mov ebx, ecx		// 0x00058eac
		mov ecx, dword ptr [ebp + 0x14]		// 0x00058eae
		mov eax, dword ptr [ecx]		// 0x00058eb1
		push edx		// 0x00058eb3
		lea edx, [ebp - 0x54]		// 0x00058eb4
		push edx		// 0x00058eb7
		mov dword ptr [ebp - 4], ebx		// 0x00058eb8
		call dword ptr [eax + 4]		// 0x00058ebb
		fld dword ptr [ebp - 0x48]		// 0x00058ebe
		fadd dword ptr [ebp - 0x54]		// 0x00058ec1
		fld dword ptr [ebp - 0x44]		// 0x00058ec4
		fadd dword ptr [ebp - 0x50]		// 0x00058ec7
		fld dword ptr [ebp - 0x40]		// 0x00058eca
		fadd dword ptr [ebp - 0x4c]		// 0x00058ecd
		fstp dword ptr [ebp - 0x24]		// 0x00058ed0
		fxch st(1)		// 0x00058ed3
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058ed5
		fstp dword ptr [ebp - 0x14]		// 0x00058edb
		mov eax, dword ptr [ebp - 0x14]		// 0x00058ede
		mov dword ptr [ebp - 0x2c], eax		// 0x00058ee1
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058ee4
		fstp dword ptr [ebp - 0x10]		// 0x00058eea
		mov ecx, dword ptr [ebp - 0x10]		// 0x00058eed
		fld dword ptr [ebp - 0x24]		// 0x00058ef0
		mov dword ptr [ebp - 0x28], ecx		// 0x00058ef3
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058ef6
		fstp dword ptr [ebp - 0xc]		// 0x00058efc
		mov edx, dword ptr [ebp - 0xc]		// 0x00058eff
		fld dword ptr [ebp - 0x48]		// 0x00058f02
		mov dword ptr [ebp - 0x24], edx		// 0x00058f05
		fsub dword ptr [ebp - 0x54]		// 0x00058f08
		fld dword ptr [ebp - 0x44]		// 0x00058f0b
		fsub dword ptr [ebp - 0x50]		// 0x00058f0e
		fld dword ptr [ebp - 0x40]		// 0x00058f11
		fsub dword ptr [ebp - 0x4c]		// 0x00058f14
		fstp dword ptr [ebp - 0x18]		// 0x00058f17
		fxch st(1)		// 0x00058f1a
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058f1c
		fstp dword ptr [ebp - 0x14]		// 0x00058f22
		mov eax, dword ptr [ebp - 0x14]		// 0x00058f25
		mov dword ptr [ebp - 0x20], eax		// 0x00058f28
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058f2b
		lea eax, [ebp - 0x14]		// 0x00058f31
		push eax		// 0x00058f34
		fstp dword ptr [ebp - 0x10]		// 0x00058f35
		mov ecx, dword ptr [ebp - 0x10]		// 0x00058f38
		fld dword ptr [ebp - 0x18]		// 0x00058f3b
		mov dword ptr [ebp - 0x1c], ecx		// 0x00058f3e
		fmul dword ptr [nxP7K101043cc_4]		// 0x00058f41
		lea ecx, [ebp - 0x2c]		// 0x00058f47
		push ecx		// 0x00058f4a
		lea ecx, [ebp - 0x54]		// 0x00058f4b
		fstp dword ptr [ebp - 0xc]		// 0x00058f4e
		mov edx, dword ptr [ebp - 0xc]		// 0x00058f51
		fld dword ptr [ebp - 0x20]		// 0x00058f54
		mov dword ptr [ebp - 0x18], edx		// 0x00058f57
		fmul dword ptr [nxP7K1010874c_4]		// 0x00058f5a
		fstp dword ptr [ebp - 0x14]		// 0x00058f60
		fld dword ptr [ebp - 0x1c]		// 0x00058f63
		fmul dword ptr [nxP7K1010874c_4]		// 0x00058f66
		fstp dword ptr [ebp - 0x10]		// 0x00058f6c
		fld dword ptr [ebp - 0x18]		// 0x00058f6f
		fmul dword ptr [nxP7K1010874c_4]		// 0x00058f72
		fstp dword ptr [ebp - 0xc]		// 0x00058f78
		call nxP7CallNxPOSetCenterExtents		// 0x00058f7b
		mov ecx, dword ptr [ebx + 4]		// 0x00058f80
		xor eax, eax		// 0x00058f83
		cmp ecx, eax		// 0x00058f85
		je L58f8c		// 0x00058f87
		mov dword ptr [ebx + 4], eax		// 0x00058f89
L58f8c:
		cmp dword ptr [ebx + 0x14], eax		// 0x00058f8c
		je L58f94		// 0x00058f8f
		mov dword ptr [ebx + 0x14], eax		// 0x00058f91
L58f94:
		mov ecx, dword ptr [ebx + 0x24]		// 0x00058f94
		cmp ecx, eax		// 0x00058f97
		lea esi, [ebx + 0x20]		// 0x00058f99
		je L58fa1		// 0x00058f9c
		mov dword ptr [esi + 4], eax		// 0x00058f9e
L58fa1:
		mov ecx, dword ptr [ebx + 0x34]		// 0x00058fa1
		cmp ecx, eax		// 0x00058fa4
		lea edi, [ebx + 0x30]		// 0x00058fa6
		je L58fae		// 0x00058fa9
		mov dword ptr [edi + 4], eax		// 0x00058fab
L58fae:
		lea ecx, [ebp - 0x3c]		// 0x00058fae
		call nxP7Call0SdkContainer		// 0x00058fb1
		mov edx, dword ptr [ebp + 0x2c]		// 0x00058fb6
		push edx		// 0x00058fb9
		mov edx, dword ptr [ebp + 8]		// 0x00058fba
		lea eax, [ebp - 0x3c]		// 0x00058fbd
		push eax		// 0x00058fc0
		push edi		// 0x00058fc1
		push esi		// 0x00058fc2
		push 0		// 0x00058fc3
		push ebx		// 0x00058fc5
		lea ecx, [ebp - 0x54]		// 0x00058fc6
		push ecx		// 0x00058fc9
		push edx		// 0x00058fca
		call nxP72310		// 0x00058fcb
		mov edi, dword ptr [ebp + 0xc]		// 0x00058fd0
		lea eax, [edi + edi*2]		// 0x00058fd3
		shl eax, 3		// 0x00058fd6
		add eax, 3		// 0x00058fd9
		add esp, 0x20		// 0x00058fdc
		and eax, 0xfffffffc		// 0x00058fdf
		call nxP7CallChkstk		// 0x00058fe2
		mov edx, esp		// 0x00058fe7
		xor esi, esi		// 0x00058fe9
		xor eax, eax		// 0x00058feb
		cmp edi, 4		// 0x00058fed
		mov dword ptr [ebp + 0xc], edx		// 0x00058ff0
		jl L5920c		// 0x00058ff3
		mov ecx, dword ptr [ebp + 0x10]		// 0x00058ff9
		lea eax, [edi - 4]		// 0x00058ffc
		shr eax, 2		// 0x00058fff
		add ecx, 0x1c		// 0x00059002
		inc eax		// 0x00059005
		mov dword ptr [ebp + 0x2c], eax		// 0x00059006
		shl eax, 2		// 0x00059009
		mov dword ptr [ebp - 8], eax		// 0x0005900c
		nop		// 0x0005900f
L59010:
		fld dword ptr [ebp - 0x48]		// 0x00059010
		fcomp dword ptr [ecx - 0x1c]		// 0x00059013
		fnstsw ax		// 0x00059016
		test ah, 5		// 0x00059018
		jnp L5908a		// 0x0005901b
		fld dword ptr [ecx - 0x10]		// 0x0005901d
		fcomp dword ptr [ebp - 0x54]		// 0x00059020
		fnstsw ax		// 0x00059023
		test ah, 5		// 0x00059025
		jnp L5908a		// 0x00059028
		fld dword ptr [ebp - 0x44]		// 0x0005902a
		fcomp dword ptr [ecx - 0x18]		// 0x0005902d
		fnstsw ax		// 0x00059030
		test ah, 5		// 0x00059032
		jnp L5908a		// 0x00059035
		fld dword ptr [ecx - 0xc]		// 0x00059037
		fcomp dword ptr [ebp - 0x50]		// 0x0005903a
		fnstsw ax		// 0x0005903d
		test ah, 5		// 0x0005903f
		jnp L5908a		// 0x00059042
		fld dword ptr [ebp - 0x40]		// 0x00059044
		fcomp dword ptr [ecx - 0x14]		// 0x00059047
		fnstsw ax		// 0x0005904a
		test ah, 5		// 0x0005904c
		jnp L5908a		// 0x0005904f
		fld dword ptr [ecx - 8]		// 0x00059051
		fcomp dword ptr [ebp - 0x4c]		// 0x00059054
		fnstsw ax		// 0x00059057
		test ah, 5		// 0x00059059
		jnp L5908a		// 0x0005905c
		mov ebx, dword ptr [ecx - 0x1c]		// 0x0005905e
		mov eax, edx		// 0x00059061
		mov dword ptr [eax], ebx		// 0x00059063
		mov ebx, dword ptr [ecx - 0x18]		// 0x00059065
		mov dword ptr [eax + 4], ebx		// 0x00059068
		mov ebx, dword ptr [ecx - 0x14]		// 0x0005906b
		mov dword ptr [eax + 8], ebx		// 0x0005906e
		mov ebx, dword ptr [ecx - 0x10]		// 0x00059071
		mov dword ptr [eax + 0xc], ebx		// 0x00059074
		mov ebx, dword ptr [ecx - 0xc]		// 0x00059077
		mov dword ptr [eax + 0x10], ebx		// 0x0005907a
		mov ebx, dword ptr [ecx - 8]		// 0x0005907d
		mov dword ptr [eax + 0x14], ebx		// 0x00059080
		mov ebx, dword ptr [ebp - 4]		// 0x00059083
		inc esi		// 0x00059086
		add edx, 0x18		// 0x00059087
L5908a:
		fld dword ptr [ebp - 0x48]		// 0x0005908a
		fcomp dword ptr [ecx - 4]		// 0x0005908d
		fnstsw ax		// 0x00059090
		test ah, 5		// 0x00059092
		jnp L59102		// 0x00059095
		fld dword ptr [ecx + 8]		// 0x00059097
		fcomp dword ptr [ebp - 0x54]		// 0x0005909a
		fnstsw ax		// 0x0005909d
		test ah, 5		// 0x0005909f
		jnp L59102		// 0x000590a2
		fld dword ptr [ebp - 0x44]		// 0x000590a4
		fcomp dword ptr [ecx]		// 0x000590a7
		fnstsw ax		// 0x000590a9
		test ah, 5		// 0x000590ab
		jnp L59102		// 0x000590ae
		fld dword ptr [ecx + 0xc]		// 0x000590b0
		fcomp dword ptr [ebp - 0x50]		// 0x000590b3
		fnstsw ax		// 0x000590b6
		test ah, 5		// 0x000590b8
		jnp L59102		// 0x000590bb
		fld dword ptr [ebp - 0x40]		// 0x000590bd
		fcomp dword ptr [ecx + 4]		// 0x000590c0
		fnstsw ax		// 0x000590c3
		test ah, 5		// 0x000590c5
		jnp L59102		// 0x000590c8
		fld dword ptr [ecx + 0x10]		// 0x000590ca
		fcomp dword ptr [ebp - 0x4c]		// 0x000590cd
		fnstsw ax		// 0x000590d0
		test ah, 5		// 0x000590d2
		jnp L59102		// 0x000590d5
		mov ebx, dword ptr [ecx - 4]		// 0x000590d7
		mov eax, edx		// 0x000590da
		mov dword ptr [eax], ebx		// 0x000590dc
		mov ebx, dword ptr [ecx]		// 0x000590de
		mov dword ptr [eax + 4], ebx		// 0x000590e0
		mov ebx, dword ptr [ecx + 4]		// 0x000590e3
		mov dword ptr [eax + 8], ebx		// 0x000590e6
		mov ebx, dword ptr [ecx + 8]		// 0x000590e9
		mov dword ptr [eax + 0xc], ebx		// 0x000590ec
		mov ebx, dword ptr [ecx + 0xc]		// 0x000590ef
		mov dword ptr [eax + 0x10], ebx		// 0x000590f2
		mov ebx, dword ptr [ecx + 0x10]		// 0x000590f5
		mov dword ptr [eax + 0x14], ebx		// 0x000590f8
		mov ebx, dword ptr [ebp - 4]		// 0x000590fb
		inc esi		// 0x000590fe
		add edx, 0x18		// 0x000590ff
L59102:
		fld dword ptr [ebp - 0x48]		// 0x00059102
		fcomp dword ptr [ecx + 0x14]		// 0x00059105
		fnstsw ax		// 0x00059108
		test ah, 5		// 0x0005910a
		jnp L5917c		// 0x0005910d
		fld dword ptr [ecx + 0x20]		// 0x0005910f
		fcomp dword ptr [ebp - 0x54]		// 0x00059112
		fnstsw ax		// 0x00059115
		test ah, 5		// 0x00059117
		jnp L5917c		// 0x0005911a
		fld dword ptr [ebp - 0x44]		// 0x0005911c
		fcomp dword ptr [ecx + 0x18]		// 0x0005911f
		fnstsw ax		// 0x00059122
		test ah, 5		// 0x00059124
		jnp L5917c		// 0x00059127
		fld dword ptr [ecx + 0x24]		// 0x00059129
		fcomp dword ptr [ebp - 0x50]		// 0x0005912c
		fnstsw ax		// 0x0005912f
		test ah, 5		// 0x00059131
		jnp L5917c		// 0x00059134
		fld dword ptr [ebp - 0x40]		// 0x00059136
		fcomp dword ptr [ecx + 0x1c]		// 0x00059139
		fnstsw ax		// 0x0005913c
		test ah, 5		// 0x0005913e
		jnp L5917c		// 0x00059141
		fld dword ptr [ecx + 0x28]		// 0x00059143
		fcomp dword ptr [ebp - 0x4c]		// 0x00059146
		fnstsw ax		// 0x00059149
		test ah, 5		// 0x0005914b
		jnp L5917c		// 0x0005914e
		mov ebx, dword ptr [ecx + 0x14]		// 0x00059150
		mov eax, edx		// 0x00059153
		mov dword ptr [eax], ebx		// 0x00059155
		mov ebx, dword ptr [ecx + 0x18]		// 0x00059157
		mov dword ptr [eax + 4], ebx		// 0x0005915a
		mov ebx, dword ptr [ecx + 0x1c]		// 0x0005915d
		mov dword ptr [eax + 8], ebx		// 0x00059160
		mov ebx, dword ptr [ecx + 0x20]		// 0x00059163
		mov dword ptr [eax + 0xc], ebx		// 0x00059166
		mov ebx, dword ptr [ecx + 0x24]		// 0x00059169
		mov dword ptr [eax + 0x10], ebx		// 0x0005916c
		mov ebx, dword ptr [ecx + 0x28]		// 0x0005916f
		mov dword ptr [eax + 0x14], ebx		// 0x00059172
		mov ebx, dword ptr [ebp - 4]		// 0x00059175
		inc esi		// 0x00059178
		add edx, 0x18		// 0x00059179
L5917c:
		fld dword ptr [ebp - 0x48]		// 0x0005917c
		fcomp dword ptr [ecx + 0x2c]		// 0x0005917f
		fnstsw ax		// 0x00059182
		test ah, 5		// 0x00059184
		jnp L591f6		// 0x00059187
		fld dword ptr [ecx + 0x38]		// 0x00059189
		fcomp dword ptr [ebp - 0x54]		// 0x0005918c
		fnstsw ax		// 0x0005918f
		test ah, 5		// 0x00059191
		jnp L591f6		// 0x00059194
		fld dword ptr [ebp - 0x44]		// 0x00059196
		fcomp dword ptr [ecx + 0x30]		// 0x00059199
		fnstsw ax		// 0x0005919c
		test ah, 5		// 0x0005919e
		jnp L591f6		// 0x000591a1
		fld dword ptr [ecx + 0x3c]		// 0x000591a3
		fcomp dword ptr [ebp - 0x50]		// 0x000591a6
		fnstsw ax		// 0x000591a9
		test ah, 5		// 0x000591ab
		jnp L591f6		// 0x000591ae
		fld dword ptr [ebp - 0x40]		// 0x000591b0
		fcomp dword ptr [ecx + 0x34]		// 0x000591b3
		fnstsw ax		// 0x000591b6
		test ah, 5		// 0x000591b8
		jnp L591f6		// 0x000591bb
		fld dword ptr [ecx + 0x40]		// 0x000591bd
		fcomp dword ptr [ebp - 0x4c]		// 0x000591c0
		fnstsw ax		// 0x000591c3
		test ah, 5		// 0x000591c5
		jnp L591f6		// 0x000591c8
		mov ebx, dword ptr [ecx + 0x2c]		// 0x000591ca
		mov eax, edx		// 0x000591cd
		mov dword ptr [eax], ebx		// 0x000591cf
		mov ebx, dword ptr [ecx + 0x30]		// 0x000591d1
		mov dword ptr [eax + 4], ebx		// 0x000591d4
		mov ebx, dword ptr [ecx + 0x34]		// 0x000591d7
		mov dword ptr [eax + 8], ebx		// 0x000591da
		mov ebx, dword ptr [ecx + 0x38]		// 0x000591dd
		mov dword ptr [eax + 0xc], ebx		// 0x000591e0
		mov ebx, dword ptr [ecx + 0x3c]		// 0x000591e3
		mov dword ptr [eax + 0x10], ebx		// 0x000591e6
		mov ebx, dword ptr [ecx + 0x40]		// 0x000591e9
		mov dword ptr [eax + 0x14], ebx		// 0x000591ec
		mov ebx, dword ptr [ebp - 4]		// 0x000591ef
		inc esi		// 0x000591f2
		add edx, 0x18		// 0x000591f3
L591f6:
		mov eax, dword ptr [ebp + 0x2c]		// 0x000591f6
		add ecx, 0x60		// 0x000591f9
		dec eax		// 0x000591fc
		mov dword ptr [ebp + 0x2c], eax		// 0x000591fd
		jne L59010		// 0x00059200
		mov edx, dword ptr [ebp + 0xc]		// 0x00059206
		mov eax, dword ptr [ebp - 8]		// 0x00059209
L5920c:
		cmp eax, edi		// 0x0005920c
		jae L592b5		// 0x0005920e
		lea ecx, [esi + esi*2]		// 0x00059214
		lea edx, [edx + ecx*8]		// 0x00059217
		mov dword ptr [ebp + 0x2c], edx		// 0x0005921a
		mov edx, dword ptr [ebp + 0x10]		// 0x0005921d
		lea ecx, [eax + eax*2]		// 0x00059220
		sub edi, eax		// 0x00059223
		lea ecx, [edx + ecx*8 + 4]		// 0x00059225
		mov edx, dword ptr [ebp + 0x2c]		// 0x00059229
		mov dword ptr [ebp + 0x10], edi		// 0x0005922c
		nop		// 0x0005922f
L59230:
		fld dword ptr [ebp - 0x48]		// 0x00059230
		fcomp dword ptr [ecx - 4]		// 0x00059233
		fnstsw ax		// 0x00059236
		test ah, 5		// 0x00059238
		jnp L592a5		// 0x0005923b
		fld dword ptr [ecx + 8]		// 0x0005923d
		fcomp dword ptr [ebp - 0x54]		// 0x00059240
		fnstsw ax		// 0x00059243
		test ah, 5		// 0x00059245
		jnp L592a5		// 0x00059248
		fld dword ptr [ebp - 0x44]		// 0x0005924a
		fcomp dword ptr [ecx]		// 0x0005924d
		fnstsw ax		// 0x0005924f
		test ah, 5		// 0x00059251
		jnp L592a5		// 0x00059254
		fld dword ptr [ecx + 0xc]		// 0x00059256
		fcomp dword ptr [ebp - 0x50]		// 0x00059259
		fnstsw ax		// 0x0005925c
		test ah, 5		// 0x0005925e
		jnp L592a5		// 0x00059261
		fld dword ptr [ebp - 0x40]		// 0x00059263
		fcomp dword ptr [ecx + 4]		// 0x00059266
		fnstsw ax		// 0x00059269
		test ah, 5		// 0x0005926b
		jnp L592a5		// 0x0005926e
		fld dword ptr [ecx + 0x10]		// 0x00059270
		fcomp dword ptr [ebp - 0x4c]		// 0x00059273
		fnstsw ax		// 0x00059276
		test ah, 5		// 0x00059278
		jnp L592a5		// 0x0005927b
		mov edi, dword ptr [ecx - 4]		// 0x0005927d
		mov eax, edx		// 0x00059280
		mov dword ptr [eax], edi		// 0x00059282
		mov edi, dword ptr [ecx]		// 0x00059284
		mov dword ptr [eax + 4], edi		// 0x00059286
		mov edi, dword ptr [ecx + 4]		// 0x00059289
		mov dword ptr [eax + 8], edi		// 0x0005928c
		mov edi, dword ptr [ecx + 8]		// 0x0005928f
		mov dword ptr [eax + 0xc], edi		// 0x00059292
		mov edi, dword ptr [ecx + 0xc]		// 0x00059295
		mov dword ptr [eax + 0x10], edi		// 0x00059298
		mov edi, dword ptr [ecx + 0x10]		// 0x0005929b
		inc esi		// 0x0005929e
		mov dword ptr [eax + 0x14], edi		// 0x0005929f
		add edx, 0x18		// 0x000592a2
L592a5:
		mov eax, dword ptr [ebp + 0x10]		// 0x000592a5
		add ecx, 0x18		// 0x000592a8
		dec eax		// 0x000592ab
		mov dword ptr [ebp + 0x10], eax		// 0x000592ac
		jne L59230		// 0x000592af
L592b5:
		mov ecx, dword ptr [ebp + 0x30]		// 0x000592b5
		mov edx, dword ptr [ebp + 0x28]		// 0x000592b8
		mov eax, dword ptr [ebp + 0x24]		// 0x000592bb
		push ecx		// 0x000592be
		mov ecx, dword ptr [ebp + 0x20]		// 0x000592bf
		push edx		// 0x000592c2
		mov edx, dword ptr [ebp + 0x1c]		// 0x000592c3
		push eax		// 0x000592c6
		mov eax, dword ptr [ebp + 0x18]		// 0x000592c7
		push ecx		// 0x000592ca
		mov ecx, dword ptr [ebp + 0x14]		// 0x000592cb
		push edx		// 0x000592ce
		mov edx, dword ptr [ebp - 0x34]		// 0x000592cf
		push eax		// 0x000592d2
		push ecx		// 0x000592d3
		push edx		// 0x000592d4
		mov ecx, dword ptr [ebp + 8]		// 0x000592d5
		mov eax, 0x88888889		// 0x000592d8
		mul dword ptr [ebp - 0x38]		// 0x000592dd
		mov eax, dword ptr [ebp + 0xc]		// 0x000592e0
		shr edx, 3		// 0x000592e3
		push edx		// 0x000592e6
		push eax		// 0x000592e7
		push esi		// 0x000592e8
		lea eax, [ebx + 0x30]		// 0x000592e9
		push eax		// 0x000592ec
		lea eax, [ebx + 0x20]		// 0x000592ed
		push eax		// 0x000592f0
		push 0		// 0x000592f1
		push ebx		// 0x000592f3
		push ecx		// 0x000592f4
		mov ecx, ebx		// 0x000592f5
		call nxP72300		// 0x000592f7
		lea ecx, [ebp - 0x3c]		// 0x000592fc
		mov bl, al		// 0x000592ff
		call nxP7CallEmpty		// 0x00059301
		mov al, bl		// 0x00059306
		lea esp, [ebp - 0x60]		// 0x00059308
		pop edi		// 0x0005930b
		pop esi		// 0x0005930c
		pop ebx		// 0x0005930d
		mov esp, ebp		// 0x0005930e
		pop ebp		// 0x00059310
		ret 0x2c		// 0x00059311
		}
	}

// phys_fn_002306 (0x00059320, 1001 B), 253 instructions
__declspec(naked) unsigned __cdecl nxP72306()
	{
	__asm
		{
		sub esp, 0xd8		// 0x00059320
		mov ecx, dword ptr [nxP7SweepCache]		// 0x00059326
		mov eax, dword ptr [ecx + 0x10]		// 0x0005932c
		push ebx		// 0x0005932f
		push ebp		// 0x00059330
		mov dword ptr [esp + 0x14], eax		// 0x00059331
		mov eax, dword ptr [ecx + 4]		// 0x00059335
		push esi		// 0x00059338
		mov esi, dword ptr [esp + 0xf0]		// 0x00059339
		shl eax, 2		// 0x00059340
		push edi		// 0x00059343
		mov edi, dword ptr [esp + 0xf0]		// 0x00059344
		fld dword ptr [eax + edi + 4]		// 0x0005934b
		xor ebx, ebx		// 0x0005934f
		fstp dword ptr [esp + 0x18]		// 0x00059351
		mov dword ptr [esp + 0x14], ebx		// 0x00059355
		fld dword ptr [eax + esi]		// 0x00059359
		fcomp dword ptr [nxP7K101041f0_4]		// 0x0005935c
		fnstsw ax		// 0x00059362
		test ah, 0x41		// 0x00059364
		jne L5936d		// 0x00059367
		mov dword ptr [esp + 0x1c], ebx		// 0x00059369
L5936d:
		mov edx, dword ptr [esi]		// 0x0005936d
		fld dword ptr [esp + 0x1c]		// 0x0005936f
		mov eax, dword ptr [ecx + 4]		// 0x00059373
		mov dword ptr [esp + 0x38], edx		// 0x00059376
		mov edx, dword ptr [esi + 4]		// 0x0005937a
		mov dword ptr [esp + 0x3c], edx		// 0x0005937d
		mov edx, dword ptr [esi + 8]		// 0x00059381
		shl eax, 2		// 0x00059384
		mov dword ptr [esp + 0x2c], 0		// 0x00059387
		mov dword ptr [esp + 0x30], 0		// 0x0005938f
		mov dword ptr [esp + 0x34], 0		// 0x00059397
		fstp dword ptr [esp + eax + 0x2c]		// 0x0005939f
		mov dword ptr [esp + 0x40], edx		// 0x000593a3
		mov dword ptr [esp + eax + 0x38], ebx		// 0x000593a7
		mov eax, dword ptr [ecx + 4]		// 0x000593ab
		shl eax, 2		// 0x000593ae
		mov dword ptr [esp + 0x20], 0		// 0x000593b1
		mov dword ptr [esp + 0x24], 0		// 0x000593b9
		mov dword ptr [esp + 0x28], 0		// 0x000593c1
		mov ecx, dword ptr [eax + esi]		// 0x000593c9
		mov dword ptr [esp + eax + 0x20], ecx		// 0x000593cc
		lea ecx, [esp + 0x80]		// 0x000593d0
		call nxP7Call0SdkContainer		// 0x000593d7
		lea ecx, [esp + 0x90]		// 0x000593dc
		call nxP7Call0SdkContainer		// 0x000593e3
		lea ecx, [esp + 0xa0]		// 0x000593e8
		call nxP7Call0SdkContainer		// 0x000593ef
		lea ecx, [esp + 0xb0]		// 0x000593f4
		call nxP7Call0SdkContainer		// 0x000593fb
		mov ebp, dword ptr [esp + 0x104]		// 0x00059400
		mov ecx, dword ptr [esp + 0xfc]		// 0x00059407
		push ebp		// 0x0005940e
		mov dword ptr [esp + 0x14], ebx		// 0x0005940f
		mov ebx, dword ptr [esp + 0x104]		// 0x00059413
		push ebx		// 0x0005941a
		lea edx, [esp + 0x18]		// 0x0005941b
		push edx		// 0x0005941f
		mov edx, dword ptr [esp + 0x104]		// 0x00059420
		push 0xa		// 0x00059427
		push 0x3f800000		// 0x00059429
		push 0		// 0x0005942e
		lea eax, [esp + 0x44]		// 0x00059430
		push eax		// 0x00059434
		mov eax, dword ptr [esp + 0x108]		// 0x00059435
		push edi		// 0x0005943c
		push ecx		// 0x0005943d
		push edx		// 0x0005943e
		push eax		// 0x0005943f
		lea ecx, [esp + 0xac]		// 0x00059440
		call nxP72304		// 0x00059447
		test al, al		// 0x0005944c
		je L59460		// 0x0005944e
		mov eax, dword ptr [esp + 0x10]		// 0x00059450
		test eax, eax		// 0x00059454
		je L59460		// 0x00059456
		mov dword ptr [esp + 0x14], 2		// 0x00059458
L59460:
		mov eax, dword ptr [esp + 0xfc]		// 0x00059460
		push ebp		// 0x00059467
		push ebx		// 0x00059468
		lea ecx, [esp + 0x18]		// 0x00059469
		push ecx		// 0x0005946d
		mov ecx, dword ptr [esp + 0x104]		// 0x0005946e
		push 0xa		// 0x00059475
		push 0x3f800000		// 0x00059477
		push 0		// 0x0005947c
		lea edx, [esp + 0x50]		// 0x0005947e
		push edx		// 0x00059482
		mov edx, dword ptr [esp + 0x108]		// 0x00059483
		push edi		// 0x0005948a
		push eax		// 0x0005948b
		push ecx		// 0x0005948c
		push edx		// 0x0005948d
		lea ecx, [esp + 0xac]		// 0x0005948e
		mov dword ptr [esp + 0x3c], 0		// 0x00059495
		call nxP72304		// 0x0005949d
		test al, al		// 0x000594a2
		je L594b3		// 0x000594a4
		mov eax, dword ptr [esp + 0x10]		// 0x000594a6
		test eax, eax		// 0x000594aa
		je L594b3		// 0x000594ac
		or dword ptr [esp + 0x14], 1		// 0x000594ae
L594b3:
		mov eax, dword ptr [nxP7SweepCache]		// 0x000594b3
		mov eax, dword ptr [eax + 4]		// 0x000594b8
		fld dword ptr [esp + eax*4 + 0x20]		// 0x000594bb
		push ebp		// 0x000594bf
		fsub dword ptr [esp + 0x20]		// 0x000594c0
		mov ebp, dword ptr [esp + 0x100]		// 0x000594c4
		push ebx		// 0x000594cb
		lea ecx, [esp + 0x18]		// 0x000594cc
		push ecx		// 0x000594d0
		fstp dword ptr [esp + eax*4 + 0x2c]		// 0x000594d1
		mov ecx, dword ptr [esp + 0xf8]		// 0x000594d5
		push 1		// 0x000594dc
		push 0x3f800000		// 0x000594de
		lea eax, [esp + eax*4 + 0x34]		// 0x000594e3
		mov eax, dword ptr [esp + 0x10c]		// 0x000594e7
		push 0		// 0x000594ee
		lea edx, [esp + 0x38]		// 0x000594f0
		push edx		// 0x000594f4
		push edi		// 0x000594f5
		push ebp		// 0x000594f6
		push eax		// 0x000594f7
		push ecx		// 0x000594f8
		lea ecx, [esp + 0xac]		// 0x000594f9
		mov dword ptr [esp + 0x3c], 0		// 0x00059500
		mov byte ptr [esp + 0xec], 0		// 0x00059508
		call nxP72304		// 0x00059510
		test al, al		// 0x00059515
		je L59526		// 0x00059517
		mov eax, dword ptr [esp + 0x10]		// 0x00059519
		test eax, eax		// 0x0005951d
		je L59526		// 0x0005951f
		or dword ptr [esp + 0x14], 4		// 0x00059521
L59526:
		mov eax, dword ptr [nxP7SweepCache]		// 0x00059526
		mov cl, byte ptr [eax + 0x34]		// 0x0005952b
		test cl, cl		// 0x0005952e
		je L596c1		// 0x00059530
		mov cl, byte ptr [esp + 0xc0]		// 0x00059536
		test cl, cl		// 0x0005953d
		je L596c1		// 0x0005953f
		mov edx, dword ptr [eax + 4]		// 0x00059545
		fld dword ptr [esi + edx*4]		// 0x00059548
		fcomp dword ptr [nxP7K101041f0_4]		// 0x0005954b
		fnstsw ax		// 0x00059551
		test ah, 5		// 0x00059553
		jp L596c1		// 0x00059556
		mov eax, dword ptr [esp + 0xc4]		// 0x0005955c
		mov ecx, dword ptr [esp + 0xc8]		// 0x00059563
		mov edx, dword ptr [esp + 0xcc]		// 0x0005956a
		mov dword ptr [esp + 0x50], eax		// 0x00059571
		mov eax, dword ptr [esp + 0xd0]		// 0x00059575
		mov dword ptr [esp + 0x54], ecx		// 0x0005957c
		mov ecx, dword ptr [esp + 0xd4]		// 0x00059580
		mov dword ptr [esp + 0x5c], eax		// 0x00059587
		mov eax, dword ptr [esp + 0xdc]		// 0x0005958b
		mov dword ptr [esp + 0x58], edx		// 0x00059592
		mov edx, dword ptr [esp + 0xd8]		// 0x00059596
		mov dword ptr [esp + 0x60], ecx		// 0x0005959d
		mov ecx, dword ptr [esp + 0xe0]		// 0x000595a1
		mov dword ptr [esp + 0x68], eax		// 0x000595a8
		mov dword ptr [esp + 0x64], edx		// 0x000595ac
		mov edx, dword ptr [esp + 0xe4]		// 0x000595b0
		lea eax, [esp + 0x74]		// 0x000595b7
		mov dword ptr [esp + 0x6c], ecx		// 0x000595bb
		push eax		// 0x000595bf
		lea ecx, [esp + 0x54]		// 0x000595c0
		mov dword ptr [esp + 0x74], edx		// 0x000595c4
		call nxP7CallNormal		// 0x000595c8
		mov edx, dword ptr [nxP7SweepCache]		// 0x000595cd
		mov ecx, dword ptr [edx + 4]		// 0x000595d3
		shl ecx, 2		// 0x000595d6
		fld dword ptr [esp + ecx + 0x74]		// 0x000595d9
		fcomp dword ptr [nxP7K101041f0_4]		// 0x000595dd
		fnstsw ax		// 0x000595e3
		test ah, 1		// 0x000595e5
		jne L596c1		// 0x000595e8
		fld dword ptr [esp + ecx + 0x74]		// 0x000595ee
		fcomp dword ptr [edx + 8]		// 0x000595f2
		fnstsw ax		// 0x000595f5
		test ah, 5		// 0x000595f7
		jp L596c1		// 0x000595fa
		fld dword ptr [ecx + edi + 4]		// 0x00059600
		mov byte ptr [nxP7SweepGuard], 1		// 0x00059604
		fcomp dword ptr [esp + 0x18]		// 0x0005960b
		fnstsw ax		// 0x0005960f
		test ah, 0x41		// 0x00059611
		jne L59620		// 0x00059614
		fld dword ptr [ecx + edi + 4]		// 0x00059616
		fsub dword ptr [esp + 0x18]		// 0x0005961a
		jmp L59626		// 0x0005961e
L59620:
		fld dword ptr [nxP7K101041f0_4]		// 0x00059620
L59626:
		fld dword ptr [ecx + esi]		// 0x00059626
		mov dword ptr [esp + 0x10], 0		// 0x00059629
		fabs		// 0x00059631
		faddp st(1), st(0)		// 0x00059633
		fcom dword ptr [esp + 0x104]		// 0x00059635
		fnstsw ax		// 0x0005963c
		test ah, 5		// 0x0005963e
		jp L59651		// 0x00059641
		fld dword ptr [nxP7K10106954_4]		// 0x00059643
		fmul st(0), st(1)		// 0x00059649
		fstp dword ptr [esp + 0x18]		// 0x0005964b
		jmp L5965c		// 0x0005964f
L59651:
		mov ecx, dword ptr [esp + 0x104]		// 0x00059651
		mov dword ptr [esp + 0x18], ecx		// 0x00059658
L5965c:
		mov edx, dword ptr [esp + 0x18]		// 0x0005965c
		fchs		// 0x00059660
		push edx		// 0x00059662
		fstp dword ptr [esp + 0x4c]		// 0x00059663
		mov edx, dword ptr [esp + 0xfc]		// 0x00059667
		push ebx		// 0x0005966e
		lea eax, [esp + 0x18]		// 0x0005966f
		push eax		// 0x00059673
		mov eax, dword ptr [esp + 0xf8]		// 0x00059674
		push 0xa		// 0x0005967b
		push 0x3f800000		// 0x0005967d
		push 0		// 0x00059682
		lea ecx, [esp + 0x5c]		// 0x00059684
		push ecx		// 0x00059688
		push edi		// 0x00059689
		push ebp		// 0x0005968a
		push edx		// 0x0005968b
		push eax		// 0x0005968c
		lea ecx, [esp + 0xac]		// 0x0005968d
		mov dword ptr [esp + 0x70], 0		// 0x00059694
		mov dword ptr [esp + 0x78], 0		// 0x0005969c
		call nxP72304		// 0x000596a4
		test al, al		// 0x000596a9
		je L596ba		// 0x000596ab
		mov eax, dword ptr [esp + 0x10]		// 0x000596ad
		test eax, eax		// 0x000596b1
		je L596ba		// 0x000596b3
		and dword ptr [esp + 0x14], 0xfffffffb		// 0x000596b5
L596ba:
		mov byte ptr [nxP7SweepGuard], 0		// 0x000596ba
L596c1:
		mov ecx, dword ptr [esp + 0x108]		// 0x000596c1
		mov edx, dword ptr [esp + 0x14]		// 0x000596c8
		mov dword ptr [ecx], edx		// 0x000596cc
		lea ecx, [esp + 0xb0]		// 0x000596ce
		call nxP7CallEmpty		// 0x000596d5
		lea ecx, [esp + 0xa0]		// 0x000596da
		call nxP7CallEmpty		// 0x000596e1
		lea ecx, [esp + 0x90]		// 0x000596e6
		call nxP7CallEmpty		// 0x000596ed
		lea ecx, [esp + 0x80]		// 0x000596f2
		call nxP7CallEmpty		// 0x000596f9
		pop edi		// 0x000596fe
		pop esi		// 0x000596ff
		pop ebp		// 0x00059700
		pop ebx		// 0x00059701
		add esp, 0xd8		// 0x00059702
		ret		// 0x00059708
		}
	}

// phys_fn_002308 (0x00059710, 166 B), 56 instructions
__declspec(naked) unsigned __fastcall nxP72308(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		sub esp, 0x1c		// 0x00059710
		push esi		// 0x00059713
		mov esi, ecx		// 0x00059714
		push edi		// 0x00059716
		lea edi, [esi + 0x28]		// 0x00059717
		mov eax, edi		// 0x0005971a
		mov ecx, dword ptr [eax]		// 0x0005971c
		mov edx, dword ptr [eax + 4]		// 0x0005971e
		mov eax, dword ptr [eax + 8]		// 0x00059721
		mov dword ptr [esp + 0xc], ecx		// 0x00059724
		mov dword ptr [esp + 0x10], edx		// 0x00059728
		lea ecx, [esi + 0x40]		// 0x0005972c
		mov edx, dword ptr [ecx]		// 0x0005972f
		mov dword ptr [esp + 0x14], eax		// 0x00059731
		mov eax, dword ptr [ecx + 4]		// 0x00059735
		mov ecx, dword ptr [ecx + 8]		// 0x00059738
		mov dword ptr [esp + 0x18], edx		// 0x0005973b
		mov dword ptr [esp + 0x1c], eax		// 0x0005973f
		mov eax, dword ptr [esp + 0x34]		// 0x00059743
		push eax		// 0x00059747
		mov eax, dword ptr [esp + 0x2c]		// 0x00059748
		mov dword ptr [esp + 0x24], ecx		// 0x0005974c
		mov ecx, dword ptr [esp + 0x34]		// 0x00059750
		push ecx		// 0x00059754
		lea edx, [esi + 8]		// 0x00059755
		mov dword ptr [nxP7SweepCache], edx		// 0x00059758
		mov edx, dword ptr [esp + 0x34]		// 0x0005975e
		push edx		// 0x00059762
		mov edx, dword ptr [esi + 0x34]		// 0x00059763
		push 0		// 0x00059766
		push 0		// 0x00059768
		push eax		// 0x0005976a
		lea ecx, [esp + 0x20]		// 0x0005976b
		push ecx		// 0x0005976f
		push edx		// 0x00059770
		mov dword ptr [esp + 0x28], offset nxP7Tab108740		// 0x00059771
		call nxP72306		// 0x00059779
		mov eax, dword ptr [esp + 0x2c]		// 0x0005977e
		mov ecx, dword ptr [esp + 0x30]		// 0x00059782
		mov edx, dword ptr [esp + 0x34]		// 0x00059786
		mov dword ptr [edi], eax		// 0x0005978a
		mov dword ptr [edi + 4], ecx		// 0x0005978c
		mov dword ptr [edi + 8], edx		// 0x0005978f
		mov eax, dword ptr [esi + 0x24]		// 0x00059792
		add esp, 0x20		// 0x00059795
		test eax, eax		// 0x00059798
		mov dword ptr [nxP7SweepCache], 0		// 0x0005979a
		je L597ae		// 0x000597a4
		mov ecx, eax		// 0x000597a6
		mov eax, dword ptr [ecx]		// 0x000597a8
		push edi		// 0x000597aa
		call dword ptr [eax + 0x2c]		// 0x000597ab
L597ae:
		pop edi		// 0x000597ae
		pop esi		// 0x000597af
		add esp, 0x1c		// 0x000597b0
		ret 0x10		// 0x000597b3
		}
	}

// phys_fn_002310 (0x000597c0, 2239 B), 698 instructions
__declspec(naked) unsigned __cdecl nxP72310()
	{
	__asm
		{
		push ebp		// 0x000597c0
		lea ebp, [esp - 0x58]		// 0x000597c1
		sub esp, 0x2cc		// 0x000597c5
		push ebx		// 0x000597cb
		push esi		// 0x000597cc
		push edi		// 0x000597cd
		mov edi, dword ptr [ebp + 0x60]		// 0x000597ce
		add edi, 0x624		// 0x000597d1
		mov ecx, edi		// 0x000597d7
		call nxP71960		// 0x000597d9
		mov ecx, edi		// 0x000597de
		mov esi, eax		// 0x000597e0
		call nxP71957		// 0x000597e2
		add esi, eax		// 0x000597e7
		lea eax, [esi*4]		// 0x000597e9
		add eax, 3		// 0x000597f0
		and eax, 0xfffffffc		// 0x000597f3
		call nxP7CallChkstk		// 0x000597f6
		mov edi, dword ptr [ebp + 0x64]		// 0x000597fb
		mov ecx, dword ptr [ebp + 0x60]		// 0x000597fe
		mov ebx, esp		// 0x00059801
		push 0		// 0x00059803
		push ebx		// 0x00059805
		push esi		// 0x00059806
		push 3		// 0x00059807
		push edi		// 0x00059809
		call nxP7CallOverlapAABBShapes		// 0x0005980a
		mov esi, eax		// 0x0005980f
		test esi, esi		// 0x00059811
		jne L59827		// 0x00059813
		xor al, al		// 0x00059815
		lea esp, [ebp - 0x280]		// 0x00059817
		pop edi		// 0x0005981d
		pop esi		// 0x0005981e
		pop ebx		// 0x0005981f
		add ebp, 0x58		// 0x00059820
		mov esp, ebp		// 0x00059823
		pop ebp		// 0x00059825
		ret		// 0x00059826
L59827:
		lea ecx, [ebp - 0x238]		// 0x00059827
		call nxP7Call0AABBCollider		// 0x0005982d
		xor eax, eax		// 0x00059832
		lea ecx, [ebp - 0x120]		// 0x00059834
		mov dword ptr [ebp - 0xb8], eax		// 0x0005983a
		mov dword ptr [ebp - 0xb4], eax		// 0x00059840
		mov dword ptr [ebp - 0x98], 0x3f8ccccd		// 0x00059846
		mov dword ptr [ebp - 0xa8], eax		// 0x00059850
		mov dword ptr [ebp - 0xac], eax		// 0x00059856
		mov dword ptr [ebp - 0xb0], eax		// 0x0005985c
		mov dword ptr [ebp - 0x9c], eax		// 0x00059862
		mov dword ptr [ebp - 0xa0], eax		// 0x00059868
		mov dword ptr [ebp - 0xa4], eax		// 0x0005986e
		call nxP7Call0SdkContainer		// 0x00059874
		lea eax, [ebp - 0x120]		// 0x00059879
		mov dword ptr [ebp - 0xb8], eax		// 0x0005987f
		mov eax, dword ptr [ebp - 0x234]		// 0x00059885
		and eax, 0xfffffffc		// 0x0005988b
		mov ecx, ebx		// 0x0005988e
		mov ebx, dword ptr [ebp + 0x74]		// 0x00059890
		mov dword ptr [ebp - 0x234], eax		// 0x00059893
		mov dword ptr [ebp + 0x14], esi		// 0x00059899
		jmp L598a1		// 0x0005989c
L5989e:
		mov ecx, dword ptr [ebp - 4]		// 0x0005989e
L598a1:
		mov eax, dword ptr [ecx]		// 0x000598a1
		mov esi, dword ptr [eax + 8]		// 0x000598a3
		mov al, byte ptr [esi + 0xde]		// 0x000598a6
		add ecx, 4		// 0x000598ac
		test al, 0x10		// 0x000598af
		mov dword ptr [ebp - 4], ecx		// 0x000598b1
		jne L5a04e		// 0x000598b4
		mov cx, word ptr [esi + 0xd8]		// 0x000598ba
		cmp cx, 0xffff		// 0x000598c1
		je L598da		// 0x000598c6
		mov eax, dword ptr [ebp + 0x7c]		// 0x000598c8
		mov edx, 1		// 0x000598cb
		shl edx, cl		// 0x000598d0
		test edx, eax		// 0x000598d2
		je L5a04e		// 0x000598d4
L598da:
		mov ecx, esi		// 0x000598da
		call nxP71281		// 0x000598dc
		mov eax, dword ptr [eax + 8]		// 0x000598e1
		test eax, eax		// 0x000598e4
		je L598f6		// 0x000598e6
		mov cl, byte ptr [eax + 0x10c]		// 0x000598e8
		test cl, cl		// 0x000598ee
		js L5a04e		// 0x000598f0
L598f6:
		mov eax, dword ptr [esi + 0xd0]		// 0x000598f6
		xor ecx, ecx		// 0x000598fc
		cmp eax, 2		// 0x000598fe
		setne cl		// 0x00059901
		dec ecx		// 0x00059904
		and ecx, esi		// 0x00059905
		je L59a5a		// 0x00059907
		lea eax, [ebp - 0x274]		// 0x0005990d
		push eax		// 0x00059913
		call nxP7CallNxFillShapeDescriptor		// 0x00059914
		push offset nxP7IdentityCtor		// 0x00059919
		push 8		// 0x0005991e
		push 0xc		// 0x00059920
		lea ecx, [ebp - 0x1b0]		// 0x00059922
		push ecx		// 0x00059928
		call nxP70001		// 0x00059929
		lea edx, [ebp - 0x1b0]		// 0x0005992e
		push edx		// 0x00059934
		lea eax, [ebp - 0x274]		// 0x00059935
		push eax		// 0x0005993b
		call dword ptr [nxP7ImpImpNxComputeBoxPoints]		// 0x0005993c
		mov edi, dword ptr [ebp + 0x68]		// 0x00059942
		mov ecx, dword ptr [edi + 4]		// 0x00059945
		mov eax, dword ptr [edi]		// 0x00059948
		add ecx, 0x6c		// 0x0005994a
		add esp, 8		// 0x0005994d
		cmp ecx, eax		// 0x00059950
		jbe L5995d		// 0x00059952
		push 0x6c		// 0x00059954
		mov ecx, edi		// 0x00059956
		call nxP7CallResize		// 0x00059958
L5995d:
		mov eax, dword ptr [edi + 4]		// 0x0005995d
		mov edx, dword ptr [edi + 8]		// 0x00059960
		lea ecx, [edx + eax*4]		// 0x00059963
		add eax, 0x6c		// 0x00059966
		mov dword ptr [ebp + 0x60], ecx		// 0x00059969
		mov dword ptr [edi + 4], eax		// 0x0005996c
		call dword ptr [nxP7ImpImpNxGetBoxTriangles]		// 0x0005996f
		mov edx, dword ptr [ebp + 0x60]		// 0x00059975
		add edx, 0x18		// 0x00059978
		mov edi, eax		// 0x0005997b
		mov dword ptr [ebp + 0x60], edx		// 0x0005997d
		mov dword ptr [ebp + 0x54], 0xc		// 0x00059980
L59987:
		mov eax, dword ptr [edi]		// 0x00059987
		mov edx, dword ptr [ebp + 0x60]		// 0x00059989
		lea eax, [eax + eax*2]		// 0x0005998c
		lea ecx, [ebp + eax*4 - 0x1b0]		// 0x0005998f
		mov eax, dword ptr [ecx]		// 0x00059996
		add edx, -0x18		// 0x00059998
		mov dword ptr [edx], eax		// 0x0005999b
		mov eax, dword ptr [ecx + 4]		// 0x0005999d
		mov dword ptr [edx + 4], eax		// 0x000599a0
		mov ecx, dword ptr [ecx + 8]		// 0x000599a3
		mov dword ptr [edx + 8], ecx		// 0x000599a6
		mov eax, dword ptr [edi + 4]		// 0x000599a9
		mov ecx, dword ptr [ebp + 0x60]		// 0x000599ac
		lea edx, [eax + eax*2]		// 0x000599af
		add edi, 4		// 0x000599b2
		lea eax, [ebp + edx*4 - 0x1b0]		// 0x000599b5
		mov edx, dword ptr [eax]		// 0x000599bc
		add ecx, -0xc		// 0x000599be
		mov dword ptr [ecx], edx		// 0x000599c1
		mov edx, dword ptr [eax + 4]		// 0x000599c3
		mov dword ptr [ecx + 4], edx		// 0x000599c6
		mov eax, dword ptr [eax + 8]		// 0x000599c9
		mov dword ptr [ecx + 8], eax		// 0x000599cc
		mov eax, dword ptr [edi + 4]		// 0x000599cf
		add edi, 4		// 0x000599d2
		lea ecx, [eax + eax*2]		// 0x000599d5
		mov eax, dword ptr [ebp + 0x60]		// 0x000599d8
		lea edx, [ebp + ecx*4 - 0x1b0]		// 0x000599db
		mov ecx, dword ptr [edx]		// 0x000599e2
		mov dword ptr [eax], ecx		// 0x000599e4
		mov ecx, dword ptr [edx + 4]		// 0x000599e6
		mov dword ptr [eax + 4], ecx		// 0x000599e9
		mov edx, dword ptr [edx + 8]		// 0x000599ec
		mov dword ptr [eax + 8], edx		// 0x000599ef
		mov eax, dword ptr [ebx + 4]		// 0x000599f2
		mov ecx, dword ptr [ebx]		// 0x000599f5
		add edi, 4		// 0x000599f7
		cmp eax, ecx		// 0x000599fa
		jne L59a07		// 0x000599fc
		push 1		// 0x000599fe
		mov ecx, ebx		// 0x00059a00
		call nxP7CallResize		// 0x00059a02
L59a07:
		mov ecx, dword ptr [ebx + 4]		// 0x00059a07
		mov edx, dword ptr [ebx + 8]		// 0x00059a0a
		mov dword ptr [edx + ecx*4], esi		// 0x00059a0d
		mov eax, dword ptr [ebx + 4]		// 0x00059a10
		mov ecx, dword ptr [ebp + 0x70]		// 0x00059a13
		inc eax		// 0x00059a16
		mov dword ptr [ebx + 4], eax		// 0x00059a17
		mov eax, dword ptr [ecx + 4]		// 0x00059a1a
		cmp eax, dword ptr [ecx]		// 0x00059a1d
		jne L59a2b		// 0x00059a1f
		push 1		// 0x00059a21
		call nxP7CallResize		// 0x00059a23
		mov ecx, dword ptr [ebp + 0x70]		// 0x00059a28
L59a2b:
		mov edx, dword ptr [ecx + 4]		// 0x00059a2b
		mov eax, dword ptr [ecx + 8]		// 0x00059a2e
		mov dword ptr [eax + edx*4], 7		// 0x00059a31
		mov edx, dword ptr [ecx + 4]		// 0x00059a38
		mov eax, dword ptr [ebp + 0x54]		// 0x00059a3b
		inc edx		// 0x00059a3e
		mov dword ptr [ecx + 4], edx		// 0x00059a3f
		mov ecx, dword ptr [ebp + 0x60]		// 0x00059a42
		add ecx, 0x24		// 0x00059a45
		dec eax		// 0x00059a48
		mov dword ptr [ebp + 0x60], ecx		// 0x00059a49
		mov dword ptr [ebp + 0x54], eax		// 0x00059a4c
		jne L59987		// 0x00059a4f
		jmp L5a04b		// 0x00059a55
L59a5a:
		xor ecx, ecx		// 0x00059a5a
		cmp eax, 4		// 0x00059a5c
		setne cl		// 0x00059a5f
		dec ecx		// 0x00059a62
		and ecx, esi		// 0x00059a63
		mov eax, ecx		// 0x00059a65
		je L5a04e		// 0x00059a67
		mov eax, dword ptr [eax + 0xe0]		// 0x00059a6d
		mov dword ptr [ebp + 0x54], eax		// 0x00059a73
		mov eax, dword ptr [eax + 0x28]		// 0x00059a76
		test eax, eax		// 0x00059a79
		mov dword ptr [ebp + 0x60], eax		// 0x00059a7b
		je L5a04e		// 0x00059a7e
		mov edx, dword ptr [esi + 0xc]		// 0x00059a84
		mov dword ptr [ebp - 0x110], edx		// 0x00059a87
		mov eax, dword ptr [esi + 0x10]		// 0x00059a8d
		mov dword ptr [ebp - 0x100], eax		// 0x00059a90
		mov ecx, dword ptr [esi + 0x14]		// 0x00059a96
		mov dword ptr [ebp - 0xf0], ecx		// 0x00059a99
		mov edx, dword ptr [esi + 0x18]		// 0x00059a9f
		mov dword ptr [ebp - 0x10c], edx		// 0x00059aa2
		mov eax, dword ptr [esi + 0x1c]		// 0x00059aa8
		mov dword ptr [ebp - 0xfc], eax		// 0x00059aab
		mov ecx, dword ptr [esi + 0x20]		// 0x00059ab1
		mov dword ptr [ebp - 0xec], ecx		// 0x00059ab4
		mov edx, dword ptr [esi + 0x24]		// 0x00059aba
		mov dword ptr [ebp - 0x108], edx		// 0x00059abd
		mov eax, dword ptr [esi + 0x28]		// 0x00059ac3
		mov dword ptr [ebp - 0xf8], eax		// 0x00059ac6
		mov ecx, dword ptr [esi + 0x2c]		// 0x00059acc
		mov dword ptr [ebp - 0xe8], ecx		// 0x00059acf
		fld dword ptr [esi + 0x38]		// 0x00059ad5
		fld dword ptr [esi + 0x34]		// 0x00059ad8
		mov edx, dword ptr [esi + 0x30]		// 0x00059adb
		lea eax, [ebp - 0x110]		// 0x00059ade
		fstp dword ptr [ebp - 0xdc]		// 0x00059ae4
		push eax		// 0x00059aea
		lea ecx, [ebp - 0x64]		// 0x00059aeb
		fstp dword ptr [ebp - 0xd8]		// 0x00059aee
		push ecx		// 0x00059af4
		mov dword ptr [ebp - 0xe0], edx		// 0x00059af5
		mov dword ptr [ebp - 0xe4], 0		// 0x00059afb
		mov dword ptr [ebp - 0xf4], 0		// 0x00059b05
		mov dword ptr [ebp - 0x104], 0		// 0x00059b0f
		mov dword ptr [ebp - 0xd4], 0x3f800000		// 0x00059b19
		call nxP7CallInvertPRMatrix		// 0x00059b23
		fld dword ptr [edi + 0xc]		// 0x00059b28
		fsub dword ptr [edi]		// 0x00059b2b
		add esp, 8		// 0x00059b2d
		fld dword ptr [ebp - 0x64]		// 0x00059b30
		fmul st(0), st(1)		// 0x00059b33
		fstp dword ptr [ebp + 0x30]		// 0x00059b35
		fld dword ptr [ebp - 0x60]		// 0x00059b38
		fmul st(0), st(1)		// 0x00059b3b
		fstp dword ptr [ebp + 0x34]		// 0x00059b3d
		fld dword ptr [ebp - 0x5c]		// 0x00059b40
		fmul st(0), st(1)		// 0x00059b43
		fstp dword ptr [ebp + 0x38]		// 0x00059b45
		fstp st(0)		// 0x00059b48
		fld dword ptr [edi + 0x10]		// 0x00059b4a
		fsub dword ptr [edi + 4]		// 0x00059b4d
		fld dword ptr [ebp - 0x54]		// 0x00059b50
		fmul st(0), st(1)		// 0x00059b53
		fstp dword ptr [ebp + 0x24]		// 0x00059b55
		fld dword ptr [ebp - 0x50]		// 0x00059b58
		fmul st(0), st(1)		// 0x00059b5b
		fstp dword ptr [ebp + 0x28]		// 0x00059b5d
		fld dword ptr [ebp - 0x4c]		// 0x00059b60
		fmul st(0), st(1)		// 0x00059b63
		fstp dword ptr [ebp + 0x2c]		// 0x00059b65
		fstp st(0)		// 0x00059b68
		fld dword ptr [edi + 0x14]		// 0x00059b6a
		fsub dword ptr [edi + 8]		// 0x00059b6d
		fld dword ptr [ebp - 0x44]		// 0x00059b70
		fmul st(0), st(1)		// 0x00059b73
		fstp dword ptr [ebp + 0x18]		// 0x00059b75
		fld dword ptr [ebp - 0x40]		// 0x00059b78
		fmul st(0), st(1)		// 0x00059b7b
		fstp dword ptr [ebp + 0x1c]		// 0x00059b7d
		fld dword ptr [ebp - 0x3c]		// 0x00059b80
		fmul st(0), st(1)		// 0x00059b83
		fstp dword ptr [ebp + 0x20]		// 0x00059b85
		fstp st(0)		// 0x00059b88
		fld dword ptr [ebp - 0x64]		// 0x00059b8a
		fmul dword ptr [edi]		// 0x00059b8d
		fld dword ptr [ebp - 0x54]		// 0x00059b8f
		fmul dword ptr [edi + 4]		// 0x00059b92
		faddp st(1), st(0)		// 0x00059b95
		fld dword ptr [ebp - 0x44]		// 0x00059b97
		fmul dword ptr [edi + 8]		// 0x00059b9a
		faddp st(1), st(0)		// 0x00059b9d
		fadd dword ptr [ebp - 0x34]		// 0x00059b9f
		fstp dword ptr [ebp - 0x1c]		// 0x00059ba2
		mov eax, dword ptr [ebp - 0x1c]		// 0x00059ba5
		fld dword ptr [ebp - 0x60]		// 0x00059ba8
		mov dword ptr [ebp + 0x48], eax		// 0x00059bab
		fmul dword ptr [edi]		// 0x00059bae
		mov dword ptr [ebp + 0x3c], eax		// 0x00059bb0
		fld dword ptr [ebp - 0x50]		// 0x00059bb3
		mov eax, 0x80000000		// 0x00059bb6
		fmul dword ptr [edi + 4]		// 0x00059bbb
		faddp st(1), st(0)		// 0x00059bbe
		fld dword ptr [ebp - 0x40]		// 0x00059bc0
		fmul dword ptr [edi + 8]		// 0x00059bc3
		faddp st(1), st(0)		// 0x00059bc6
		fadd dword ptr [ebp - 0x30]		// 0x00059bc8
		fstp dword ptr [ebp - 0x18]		// 0x00059bcb
		mov ecx, dword ptr [ebp - 0x18]		// 0x00059bce
		fld dword ptr [ebp - 0x5c]		// 0x00059bd1
		mov dword ptr [ebp + 0x4c], ecx		// 0x00059bd4
		fmul dword ptr [edi]		// 0x00059bd7
		mov dword ptr [ebp + 0x40], ecx		// 0x00059bd9
		fld dword ptr [ebp - 0x4c]		// 0x00059bdc
		mov ecx, dword ptr [ebp + 0x30]		// 0x00059bdf
		test eax, ecx		// 0x00059be2
		fmul dword ptr [edi + 4]		// 0x00059be4
		faddp st(1), st(0)		// 0x00059be7
		fld dword ptr [ebp - 0x3c]		// 0x00059be9
		fmul dword ptr [edi + 8]		// 0x00059bec
		faddp st(1), st(0)		// 0x00059bef
		fadd dword ptr [ebp - 0x2c]		// 0x00059bf1
		fstp dword ptr [ebp - 0x14]		// 0x00059bf4
		mov edx, dword ptr [ebp - 0x14]		// 0x00059bf7
		mov dword ptr [ebp + 0x50], edx		// 0x00059bfa
		mov dword ptr [ebp + 0x44], edx		// 0x00059bfd
		je L59c0d		// 0x00059c00
		fld dword ptr [ebp + 0x3c]		// 0x00059c02
		fadd dword ptr [ebp + 0x30]		// 0x00059c05
		fstp dword ptr [ebp + 0x3c]		// 0x00059c08
		jmp L59c16		// 0x00059c0b
L59c0d:
		fld dword ptr [ebp + 0x48]		// 0x00059c0d
		fadd dword ptr [ebp + 0x30]		// 0x00059c10
		fstp dword ptr [ebp + 0x48]		// 0x00059c13
L59c16:
		test dword ptr [ebp + 0x34], eax		// 0x00059c16
		je L59c26		// 0x00059c19
		fld dword ptr [ebp + 0x40]		// 0x00059c1b
		fadd dword ptr [ebp + 0x34]		// 0x00059c1e
		fstp dword ptr [ebp + 0x40]		// 0x00059c21
		jmp L59c2f		// 0x00059c24
L59c26:
		fld dword ptr [ebp + 0x4c]		// 0x00059c26
		fadd dword ptr [ebp + 0x34]		// 0x00059c29
		fstp dword ptr [ebp + 0x4c]		// 0x00059c2c
L59c2f:
		test dword ptr [ebp + 0x38], eax		// 0x00059c2f
		je L59c42		// 0x00059c32
		fld dword ptr [ebp + 0x44]		// 0x00059c34
		fadd dword ptr [ebp + 0x38]		// 0x00059c37
		fstp dword ptr [ebp + 0x44]		// 0x00059c3a
		fld dword ptr [ebp + 0x50]		// 0x00059c3d
		jmp L59c48		// 0x00059c40
L59c42:
		fld dword ptr [ebp + 0x50]		// 0x00059c42
		fadd dword ptr [ebp + 0x38]		// 0x00059c45
L59c48:
		test dword ptr [ebp + 0x24], eax		// 0x00059c48
		je L59c58		// 0x00059c4b
		fld dword ptr [ebp + 0x3c]		// 0x00059c4d
		fadd dword ptr [ebp + 0x24]		// 0x00059c50
		fstp dword ptr [ebp + 0x3c]		// 0x00059c53
		jmp L59c61		// 0x00059c56
L59c58:
		fld dword ptr [ebp + 0x48]		// 0x00059c58
		fadd dword ptr [ebp + 0x24]		// 0x00059c5b
		fstp dword ptr [ebp + 0x48]		// 0x00059c5e
L59c61:
		test dword ptr [ebp + 0x28], eax		// 0x00059c61
		je L59c71		// 0x00059c64
		fld dword ptr [ebp + 0x40]		// 0x00059c66
		fadd dword ptr [ebp + 0x28]		// 0x00059c69
		fstp dword ptr [ebp + 0x40]		// 0x00059c6c
		jmp L59c7a		// 0x00059c6f
L59c71:
		fld dword ptr [ebp + 0x4c]		// 0x00059c71
		fadd dword ptr [ebp + 0x28]		// 0x00059c74
		fstp dword ptr [ebp + 0x4c]		// 0x00059c77
L59c7a:
		test dword ptr [ebp + 0x2c], eax		// 0x00059c7a
		je L59c87		// 0x00059c7d
		fld dword ptr [ebp + 0x44]		// 0x00059c7f
		fadd dword ptr [ebp + 0x2c]		// 0x00059c82
		jmp L59c8d		// 0x00059c85
L59c87:
		fadd dword ptr [ebp + 0x2c]		// 0x00059c87
		fld dword ptr [ebp + 0x44]		// 0x00059c8a
L59c8d:
		test dword ptr [ebp + 0x18], eax		// 0x00059c8d
		je L59ca0		// 0x00059c90
		fld dword ptr [ebp + 0x3c]		// 0x00059c92
		fadd dword ptr [ebp + 0x18]		// 0x00059c95
		fstp dword ptr [ebp + 0x3c]		// 0x00059c98
		fld dword ptr [ebp + 0x48]		// 0x00059c9b
		jmp L59ca6		// 0x00059c9e
L59ca0:
		fld dword ptr [ebp + 0x48]		// 0x00059ca0
		fadd dword ptr [ebp + 0x18]		// 0x00059ca3
L59ca6:
		test dword ptr [ebp + 0x1c], eax		// 0x00059ca6
		je L59cb9		// 0x00059ca9
		fld dword ptr [ebp + 0x40]		// 0x00059cab
		fadd dword ptr [ebp + 0x1c]		// 0x00059cae
		fstp dword ptr [ebp + 0x40]		// 0x00059cb1
		fld dword ptr [ebp + 0x4c]		// 0x00059cb4
		jmp L59cbf		// 0x00059cb7
L59cb9:
		fld dword ptr [ebp + 0x4c]		// 0x00059cb9
		fadd dword ptr [ebp + 0x1c]		// 0x00059cbc
L59cbf:
		test dword ptr [ebp + 0x20], eax		// 0x00059cbf
		je L59ccd		// 0x00059cc2
		fxch st(2)		// 0x00059cc4
		fadd dword ptr [ebp + 0x20]		// 0x00059cc6
		fxch st(2)		// 0x00059cc9
		jmp L59cd4		// 0x00059ccb
L59ccd:
		fxch st(3)		// 0x00059ccd
		fadd dword ptr [ebp + 0x20]		// 0x00059ccf
		fxch st(3)		// 0x00059cd2
L59cd4:
		fld st(1)		// 0x00059cd4
		fadd dword ptr [ebp + 0x3c]		// 0x00059cd6
		fld st(1)		// 0x00059cd9
		fadd dword ptr [ebp + 0x40]		// 0x00059cdb
		fld st(5)		// 0x00059cde
		fadd st(0), st(5)		// 0x00059ce0
		fstp dword ptr [ebp - 0x148]		// 0x00059ce2
		fxch st(1)		// 0x00059ce8
		fmul dword ptr [nxP7K101043cc_4]		// 0x00059cea
		fstp dword ptr [ebp]		// 0x00059cf0
		mov edx, dword ptr [ebp]		// 0x00059cf3
		mov dword ptr [ebp - 0xd0], edx		// 0x00059cf6
		fmul dword ptr [nxP7K101043cc_4]		// 0x00059cfc
		fstp dword ptr [ebp + 4]		// 0x00059d02
		mov eax, dword ptr [ebp + 4]		// 0x00059d05
		fld dword ptr [ebp - 0x148]		// 0x00059d08
		mov dword ptr [ebp - 0xcc], eax		// 0x00059d0e
		fmul dword ptr [nxP7K101043cc_4]		// 0x00059d14
		fstp dword ptr [ebp + 8]		// 0x00059d1a
		mov ecx, dword ptr [ebp + 8]		// 0x00059d1d
		fxch st(1)		// 0x00059d20
		mov dword ptr [ebp - 0xc8], ecx		// 0x00059d22
		fsub dword ptr [ebp + 0x3c]		// 0x00059d28
		fstp dword ptr [ebp - 0x7c]		// 0x00059d2b
		fsub dword ptr [ebp + 0x40]		// 0x00059d2e
		fstp dword ptr [ebp - 0x78]		// 0x00059d31
		fsubp st(1), st(0)		// 0x00059d34
		fld dword ptr [ebp - 0x7c]		// 0x00059d36
		fmul dword ptr [nxP7K101043cc_4]		// 0x00059d39
		fstp dword ptr [ebp - 0x10]		// 0x00059d3f
		mov edx, dword ptr [ebp - 0x10]		// 0x00059d42
		fld dword ptr [ebp - 0x78]		// 0x00059d45
		mov dword ptr [ebp - 0xc4], edx		// 0x00059d48
		fmul dword ptr [nxP7K101043cc_4]		// 0x00059d4e
		mov edx, dword ptr [ebp + 0x60]		// 0x00059d54
		push edx		// 0x00059d57
		fstp dword ptr [ebp - 0xc]		// 0x00059d58
		mov eax, dword ptr [ebp - 0xc]		// 0x00059d5b
		mov dword ptr [ebp - 0xc0], eax		// 0x00059d5e
		fmul dword ptr [nxP7K101043cc_4]		// 0x00059d64
		lea eax, [ebp - 0xd0]		// 0x00059d6a
		push eax		// 0x00059d70
		fstp dword ptr [ebp - 8]		// 0x00059d71
		mov ecx, dword ptr [ebp - 8]		// 0x00059d74
		mov dword ptr [ebp - 0xbc], ecx		// 0x00059d77
		lea ecx, [ebp - 0xb8]		// 0x00059d7d
		push ecx		// 0x00059d83
		lea ecx, [ebp - 0x238]		// 0x00059d84
		call nxP7CallCollide		// 0x00059d8a
		mov ecx, dword ptr [ebp - 0x228]		// 0x00059d8f
		test ecx, ecx		// 0x00059d95
		je L59da1		// 0x00059d97
		mov eax, dword ptr [ecx + 4]		// 0x00059d99
		mov dword ptr [ebp + 0x60], eax		// 0x00059d9c
		jmp L59dab		// 0x00059d9f
L59da1:
		mov dword ptr [ebp + 0x60], 0		// 0x00059da1
		mov eax, dword ptr [ebp + 0x60]		// 0x00059da8
L59dab:
		test eax, eax		// 0x00059dab
		je L5a04e		// 0x00059dad
		test ecx, ecx		// 0x00059db3
		je L59dbc		// 0x00059db5
		mov ebx, dword ptr [ecx + 8]		// 0x00059db7
		jmp L59dbe		// 0x00059dba
L59dbc:
		xor ebx, ebx		// 0x00059dbc
L59dbe:
		mov edi, dword ptr [ebp + 0x54]		// 0x00059dbe
		mov ecx, dword ptr [edi + 0x88]		// 0x00059dc1
		test ecx, ecx		// 0x00059dc7
		jne L59dd5		// 0x00059dc9
		mov ecx, edi		// 0x00059dcb
		call nxP7CallCreateEdgeList		// 0x00059dcd
		mov eax, dword ptr [ebp + 0x60]		// 0x00059dd2
L59dd5:
		mov edx, dword ptr [edi + 0x88]		// 0x00059dd5
		lea edi, [eax + eax*8]		// 0x00059ddb
		mov eax, dword ptr [ebp + 0x68]		// 0x00059dde
		mov ecx, dword ptr [eax + 4]		// 0x00059de1
		shl edi, 2		// 0x00059de4
		shr edi, 2		// 0x00059de7
		mov dword ptr [ebp - 0x20], edx		// 0x00059dea
		mov edx, dword ptr [eax]		// 0x00059ded
		add ecx, edi		// 0x00059def
		cmp ecx, edx		// 0x00059df1
		jbe L59e00		// 0x00059df3
		push edi		// 0x00059df5
		mov ecx, eax		// 0x00059df6
		call nxP7CallResize		// 0x00059df8
		mov eax, dword ptr [ebp + 0x68]		// 0x00059dfd
L59e00:
		mov edx, dword ptr [eax + 4]		// 0x00059e00
		mov ecx, dword ptr [eax + 8]		// 0x00059e03
		add edi, edx		// 0x00059e06
		lea ecx, [ecx + edx*4]		// 0x00059e08
		mov edx, dword ptr [ebp + 0x60]		// 0x00059e0b
		mov dword ptr [eax + 4], edi		// 0x00059e0e
		mov dword ptr [ebp + 0x10], edx		// 0x00059e11
		jmp L59e20		// 0x00059e14
L59e16:
		mov ecx, dword ptr [ebp - 0x24]		// 0x00059e16
		mov ebx, dword ptr [ebp + 0xc]		// 0x00059e19
		lea esp, [esp]		// 0x00059e1c
L59e20:
		mov edi, dword ptr [ebx]		// 0x00059e20
		add ebx, 4		// 0x00059e22
		mov eax, ecx		// 0x00059e25
		add ecx, 0x24		// 0x00059e27
		mov dword ptr [ebp + 0xc], ebx		// 0x00059e2a
		mov ebx, dword ptr [ebp + 0x54]		// 0x00059e2d
		mov edx, dword ptr [ebx + 0x14]		// 0x00059e30
		mov ebx, dword ptr [ebx + 0x10]		// 0x00059e33
		mov dword ptr [ebp - 0x24], ecx		// 0x00059e36
		lea edi, [edi + edi*2]		// 0x00059e39
		shl edi, 2		// 0x00059e3c
		mov ecx, dword ptr [edx + edi]		// 0x00059e3f
		add edx, edi		// 0x00059e42
		lea ecx, [ecx + ecx*2]		// 0x00059e44
		fld dword ptr [ebx + ecx*4]		// 0x00059e47
		lea ecx, [ebx + ecx*4]		// 0x00059e4a
		fmul dword ptr [esi + 0xc]		// 0x00059e4d
		mov ebx, dword ptr [ebp + 0x54]		// 0x00059e50
		fld dword ptr [ecx + 8]		// 0x00059e53
		fmul dword ptr [esi + 0x14]		// 0x00059e56
		faddp st(1), st(0)		// 0x00059e59
		fld dword ptr [ecx + 4]		// 0x00059e5b
		fmul dword ptr [esi + 0x10]		// 0x00059e5e
		faddp st(1), st(0)		// 0x00059e61
		fld dword ptr [ecx + 8]		// 0x00059e63
		fmul dword ptr [esi + 0x20]		// 0x00059e66
		fld dword ptr [ecx + 4]		// 0x00059e69
		fmul dword ptr [esi + 0x1c]		// 0x00059e6c
		faddp st(1), st(0)		// 0x00059e6f
		fld dword ptr [ecx]		// 0x00059e71
		fmul dword ptr [esi + 0x18]		// 0x00059e73
		faddp st(1), st(0)		// 0x00059e76
		fstp dword ptr [ebp - 0x6c]		// 0x00059e78
		fld dword ptr [ecx + 8]		// 0x00059e7b
		fmul dword ptr [esi + 0x2c]		// 0x00059e7e
		fld dword ptr [ecx + 4]		// 0x00059e81
		fmul dword ptr [esi + 0x28]		// 0x00059e84
		faddp st(1), st(0)		// 0x00059e87
		fld dword ptr [ecx]		// 0x00059e89
		fmul dword ptr [esi + 0x24]		// 0x00059e8b
		faddp st(1), st(0)		// 0x00059e8e
		fstp dword ptr [ebp - 0x68]		// 0x00059e90
		fadd dword ptr [esi + 0x30]		// 0x00059e93
		fld dword ptr [ebp - 0x6c]		// 0x00059e96
		fadd dword ptr [esi + 0x34]		// 0x00059e99
		fld dword ptr [ebp - 0x68]		// 0x00059e9c
		fadd dword ptr [esi + 0x38]		// 0x00059e9f
		fstp dword ptr [ebp - 0x13c]		// 0x00059ea2
		mov ecx, dword ptr [ebp - 0x13c]		// 0x00059ea8
		mov dword ptr [eax + 8], ecx		// 0x00059eae
		fxch st(1)		// 0x00059eb1
		fstp dword ptr [eax]		// 0x00059eb3
		fstp dword ptr [eax + 4]		// 0x00059eb5
		mov ecx, dword ptr [edx + 4]		// 0x00059eb8
		mov ebx, dword ptr [ebx + 0x10]		// 0x00059ebb
		lea ecx, [ecx + ecx*2]		// 0x00059ebe
		fld dword ptr [ebx + ecx*4 + 8]		// 0x00059ec1
		lea ecx, [ebx + ecx*4]		// 0x00059ec5
		fmul dword ptr [esi + 0x14]		// 0x00059ec8
		fld dword ptr [ecx]		// 0x00059ecb
		fmul dword ptr [esi + 0xc]		// 0x00059ecd
		faddp st(1), st(0)		// 0x00059ed0
		fld dword ptr [ecx + 4]		// 0x00059ed2
		fmul dword ptr [esi + 0x10]		// 0x00059ed5
		faddp st(1), st(0)		// 0x00059ed8
		fld dword ptr [ecx]		// 0x00059eda
		fmul dword ptr [esi + 0x18]		// 0x00059edc
		fld dword ptr [ecx + 8]		// 0x00059edf
		fmul dword ptr [esi + 0x20]		// 0x00059ee2
		faddp st(1), st(0)		// 0x00059ee5
		fld dword ptr [ecx + 4]		// 0x00059ee7
		fmul dword ptr [esi + 0x1c]		// 0x00059eea
		faddp st(1), st(0)		// 0x00059eed
		fstp dword ptr [ebp - 0x90]		// 0x00059eef
		fld dword ptr [ecx + 8]		// 0x00059ef5
		fmul dword ptr [esi + 0x2c]		// 0x00059ef8
		fld dword ptr [ecx + 4]		// 0x00059efb
		fmul dword ptr [esi + 0x28]		// 0x00059efe
		faddp st(1), st(0)		// 0x00059f01
		fld dword ptr [esi + 0x24]		// 0x00059f03
		fmul dword ptr [ecx]		// 0x00059f06
		faddp st(1), st(0)		// 0x00059f08
		mov ebx, dword ptr [ebp + 0x74]		// 0x00059f0a
		fstp dword ptr [ebp - 0x8c]		// 0x00059f0d
		fadd dword ptr [esi + 0x30]		// 0x00059f13
		fld dword ptr [ebp - 0x90]		// 0x00059f16
		fadd dword ptr [esi + 0x34]		// 0x00059f1c
		fld dword ptr [ebp - 0x8c]		// 0x00059f1f
		fadd dword ptr [esi + 0x38]		// 0x00059f25
		fstp dword ptr [ebp - 0x124]		// 0x00059f28
		mov ecx, dword ptr [ebp - 0x124]		// 0x00059f2e
		mov dword ptr [eax + 0x14], ecx		// 0x00059f34
		fxch st(1)		// 0x00059f37
		fstp dword ptr [eax + 0xc]		// 0x00059f39
		mov ecx, dword ptr [ebp + 0x54]		// 0x00059f3c
		fstp dword ptr [eax + 0x10]		// 0x00059f3f
		mov edx, dword ptr [edx + 8]		// 0x00059f42
		mov ecx, dword ptr [ecx + 0x10]		// 0x00059f45
		lea edx, [edx + edx*2]		// 0x00059f48
		fld dword ptr [ecx + edx*4]		// 0x00059f4b
		lea ecx, [ecx + edx*4]		// 0x00059f4e
		fmul dword ptr [esi + 0xc]		// 0x00059f51
		fld dword ptr [esi + 0x14]		// 0x00059f54
		fmul dword ptr [ecx + 8]		// 0x00059f57
		faddp st(1), st(0)		// 0x00059f5a
		fld dword ptr [esi + 0x10]		// 0x00059f5c
		fmul dword ptr [ecx + 4]		// 0x00059f5f
		faddp st(1), st(0)		// 0x00059f62
		fld dword ptr [ecx]		// 0x00059f64
		fmul dword ptr [esi + 0x18]		// 0x00059f66
		fld dword ptr [ecx + 8]		// 0x00059f69
		fmul dword ptr [esi + 0x20]		// 0x00059f6c
		faddp st(1), st(0)		// 0x00059f6f
		fld dword ptr [ecx + 4]		// 0x00059f71
		fmul dword ptr [esi + 0x1c]		// 0x00059f74
		faddp st(1), st(0)		// 0x00059f77
		fstp dword ptr [ebp - 0x84]		// 0x00059f79
		fld dword ptr [esi + 0x2c]		// 0x00059f7f
		fmul dword ptr [ecx + 8]		// 0x00059f82
		fld dword ptr [esi + 0x28]		// 0x00059f85
		fmul dword ptr [ecx + 4]		// 0x00059f88
		faddp st(1), st(0)		// 0x00059f8b
		fld dword ptr [ecx]		// 0x00059f8d
		fmul dword ptr [esi + 0x24]		// 0x00059f8f
		faddp st(1), st(0)		// 0x00059f92
		fstp dword ptr [ebp - 0x80]		// 0x00059f94
		fadd dword ptr [esi + 0x30]		// 0x00059f97
		fld dword ptr [ebp - 0x84]		// 0x00059f9a
		fadd dword ptr [esi + 0x34]		// 0x00059fa0
		fld dword ptr [ebp - 0x80]		// 0x00059fa3
		fadd dword ptr [esi + 0x38]		// 0x00059fa6
		fstp dword ptr [ebp - 0x130]		// 0x00059fa9
		mov edx, dword ptr [ebp - 0x130]		// 0x00059faf
		fxch st(1)		// 0x00059fb5
		mov dword ptr [eax + 0x20], edx		// 0x00059fb7
		fstp dword ptr [eax + 0x18]		// 0x00059fba
		fstp dword ptr [eax + 0x1c]		// 0x00059fbd
		mov eax, dword ptr [ebx + 4]		// 0x00059fc0
		cmp eax, dword ptr [ebx]		// 0x00059fc3
		jne L59fd0		// 0x00059fc5
		push 1		// 0x00059fc7
		mov ecx, ebx		// 0x00059fc9
		call nxP7CallResize		// 0x00059fcb
L59fd0:
		mov ecx, dword ptr [ebx + 4]		// 0x00059fd0
		mov edx, dword ptr [ebx + 8]		// 0x00059fd3
		mov eax, dword ptr [ebp - 0x20]		// 0x00059fd6
		mov dword ptr [edx + ecx*4], esi		// 0x00059fd9
		mov edx, dword ptr [ebx + 4]		// 0x00059fdc
		inc edx		// 0x00059fdf
		test eax, eax		// 0x00059fe0
		mov dword ptr [ebx + 4], edx		// 0x00059fe2
		mov dword ptr [ebp + 0x60], 0		// 0x00059fe5
		je L5a018		// 0x00059fec
		mov eax, dword ptr [eax + 0xc]		// 0x00059fee
		mov edx, dword ptr [eax + edi]		// 0x00059ff1
		add eax, edi		// 0x00059ff4
		mov ecx, 0x80000000		// 0x00059ff6
		test ecx, edx		// 0x00059ffb
		je L5a006		// 0x00059ffd
		mov dword ptr [ebp + 0x60], 1		// 0x00059fff
L5a006:
		test dword ptr [eax + 4], ecx		// 0x0005a006
		je L5a00f		// 0x0005a009
		or dword ptr [ebp + 0x60], 2		// 0x0005a00b
L5a00f:
		test dword ptr [eax + 8], ecx		// 0x0005a00f
		je L5a018		// 0x0005a012
		or dword ptr [ebp + 0x60], 4		// 0x0005a014
L5a018:
		mov edi, dword ptr [ebp + 0x70]		// 0x0005a018
		mov eax, dword ptr [edi + 4]		// 0x0005a01b
		cmp eax, dword ptr [edi]		// 0x0005a01e
		jne L5a02b		// 0x0005a020
		push 1		// 0x0005a022
		mov ecx, edi		// 0x0005a024
		call nxP7CallResize		// 0x0005a026
L5a02b:
		mov ecx, dword ptr [edi + 4]		// 0x0005a02b
		mov eax, dword ptr [ebp + 0x60]		// 0x0005a02e
		mov edx, dword ptr [edi + 8]		// 0x0005a031
		mov dword ptr [edx + ecx*4], eax		// 0x0005a034
		mov ecx, dword ptr [edi + 4]		// 0x0005a037
		mov eax, dword ptr [ebp + 0x10]		// 0x0005a03a
		inc ecx		// 0x0005a03d
		dec eax		// 0x0005a03e
		mov dword ptr [edi + 4], ecx		// 0x0005a03f
		mov dword ptr [ebp + 0x10], eax		// 0x0005a042
		jne L59e16		// 0x0005a045
L5a04b:
		mov edi, dword ptr [ebp + 0x64]		// 0x0005a04b
L5a04e:
		dec dword ptr [ebp + 0x14]		// 0x0005a04e
		jne L5989e		// 0x0005a051
		lea ecx, [ebp - 0x120]		// 0x0005a057
		call nxP7CallEmpty		// 0x0005a05d
		lea ecx, [ebp - 0x238]		// 0x0005a062
		call nxP7Call1AABBCollider		// 0x0005a068
		mov al, 1		// 0x0005a06d
		lea esp, [ebp - 0x280]		// 0x0005a06f
		pop edi		// 0x0005a075
		pop esi		// 0x0005a076
		pop ebx		// 0x0005a077
		add ebp, 0x58		// 0x0005a078
		mov esp, ebp		// 0x0005a07b
		pop ebp		// 0x0005a07d
		ret		// 0x0005a07e
		}
	}

// helper: phys_fn_002312 (record unchanged) (0x0005a080, 31 B), 11 instructions
__declspec(naked) unsigned __fastcall nxP72312(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		test byte ptr [esp + 4], 1		// 0x0005a080
		push esi		// 0x0005a085
		mov esi, ecx		// 0x0005a086
		mov dword ptr [esi], offset nxP7TabR1		// 0x0005a088
		je L5a099		// 0x0005a08e
		push esi		// 0x0005a090
		call nxP7CallFree		// 0x0005a091
		add esp, 4		// 0x0005a096
L5a099:
		mov eax, esi		// 0x0005a099
		pop esi		// 0x0005a09b
		ret 4		// 0x0005a09c
		}
	}

// helper: phys_fn_002314 (record unchanged) (0x0005a0a0, 74 B), 23 instructions
__declspec(naked) unsigned __fastcall nxP72314(void* self, void* edxUnused)
	{
	__asm
		{
		xor eax, eax		// 0x0005a0a0
		mov dword ptr [ecx + 0xc], eax		// 0x0005a0a2
		mov dword ptr [ecx + 0x10], eax		// 0x0005a0a5
		mov dword ptr [ecx + 0x14], eax		// 0x0005a0a8
		mov dword ptr [ecx + 0x1c], eax		// 0x0005a0ab
		mov dword ptr [ecx + 0x20], eax		// 0x0005a0ae
		mov dword ptr [ecx + 0x24], eax		// 0x0005a0b1
		mov edx, 0x3f800000		// 0x0005a0b4
		mov dword ptr [ecx + 8], edx		// 0x0005a0b9
		mov dword ptr [ecx + 0x18], edx		// 0x0005a0bc
		mov dword ptr [ecx + 0x28], edx		// 0x0005a0bf
		mov dword ptr [ecx + 0x34], eax		// 0x0005a0c2
		mov dword ptr [ecx + 0x30], eax		// 0x0005a0c5
		mov dword ptr [ecx + 0x2c], eax		// 0x0005a0c8
		mov dword ptr [ecx + 0x38], 8		// 0x0005a0cb
		mov word ptr [ecx + 0x3c], ax		// 0x0005a0d2
		mov word ptr [ecx + 0x3e], ax		// 0x0005a0d6
		mov dword ptr [ecx + 0x40], eax		// 0x0005a0da
		mov dword ptr [ecx + 0x44], eax		// 0x0005a0dd
		mov dword ptr [ecx + 0x54], eax		// 0x0005a0e0
		mov dword ptr [ecx + 0x50], eax		// 0x0005a0e3
		mov dword ptr [ecx + 0x4c], eax		// 0x0005a0e6
		ret		// 0x0005a0e9
		}
	}

// phys_fn_002316 (0x0005a0f0, 122 B), 41 instructions
__declspec(naked) unsigned __fastcall nxP72316(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [esp + 8]		// 0x0005a0f0
		fld dword ptr [nxP7K101041f0_4]		// 0x0005a0f4
		mov dword ptr [ecx + 0x2c], eax		// 0x0005a0fa
		mov eax, dword ptr [esp + 4]		// 0x0005a0fd
		mov dword ptr [ecx], offset nxP7TabR1 + 12		// 0x0005a101
		push esi		// 0x0005a107
		xor edx, edx		// 0x0005a108
		mov dword ptr [ecx + 0x30], edx		// 0x0005a10a
		mov esi, dword ptr [eax + 0x18]		// 0x0005a10d
		mov dword ptr [ecx + 4], esi		// 0x0005a110
		mov esi, dword ptr [eax + 0x1c]		// 0x0005a113
		mov dword ptr [ecx + 8], esi		// 0x0005a116
		mov esi, dword ptr [eax + 0x20]		// 0x0005a119
		mov dword ptr [ecx + 0xc], esi		// 0x0005a11c
		mov esi, dword ptr [eax + 0x24]		// 0x0005a11f
		mov dword ptr [ecx + 0x10], esi		// 0x0005a122
		mov esi, dword ptr [eax + 0x28]		// 0x0005a125
		mov dword ptr [ecx + 0x14], esi		// 0x0005a128
		mov esi, dword ptr [eax + 0x2c]		// 0x0005a12b
		mov dword ptr [ecx + 0x18], esi		// 0x0005a12e
		mov dword ptr [ecx + 0x1c], edx		// 0x0005a131
		mov esi, dword ptr [eax + 0xc]		// 0x0005a134
		mov dword ptr [ecx + 0x20], esi		// 0x0005a137
		mov esi, dword ptr [eax + 0x10]		// 0x0005a13a
		mov dword ptr [ecx + 0x24], esi		// 0x0005a13d
		mov esi, dword ptr [eax + 0x14]		// 0x0005a140
		mov dword ptr [ecx + 0x28], esi		// 0x0005a143
		fld dword ptr [eax + 0x1c]		// 0x0005a146
		fucompp		// 0x0005a149
		pop esi		// 0x0005a14b
		fnstsw ax		// 0x0005a14c
		test ah, 0x44		// 0x0005a14e
		jnp L5a160		// 0x0005a151
		mov eax, 1		// 0x0005a153
		mov byte ptr [ecx + 0x34], al		// 0x0005a158
		mov eax, ecx		// 0x0005a15b
		ret 8		// 0x0005a15d
L5a160:
		xor eax, eax		// 0x0005a160
		mov byte ptr [ecx + 0x34], al		// 0x0005a162
		mov eax, ecx		// 0x0005a165
		ret 8		// 0x0005a167
		}
	}

// helper: phys_fn_002322 (record unchanged) (0x0005a230, 8 B), 2 instructions
__declspec(naked) unsigned __fastcall nxP72322(void* self, void* edxUnused)
	{
	__asm
		{
		sub ecx, 8		// 0x0005a230
		jmp nxP72328		// 0x0005a233
		}
	}

// helper: phys_fn_002324 (record unchanged) (0x0005a240, 500 B), 135 instructions
__declspec(naked) unsigned __fastcall nxP72324(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x0005a240
		mov esi, ecx		// 0x0005a241
		fld dword ptr [esi + 0x4c]		// 0x0005a243
		sub esp, 8		// 0x0005a246
		fstp qword ptr [esp]		// 0x0005a249
		call nxP7CallFpclass		// 0x0005a24c
		add esp, 8		// 0x0005a251
		test eax, 0x207		// 0x0005a254
		jne L5a425		// 0x0005a259
		fld dword ptr [esi + 0x50]		// 0x0005a25f
		sub esp, 8		// 0x0005a262
		fstp qword ptr [esp]		// 0x0005a265
		call nxP7CallFpclass		// 0x0005a268
		add esp, 8		// 0x0005a26d
		test eax, 0x207		// 0x0005a270
		jne L5a425		// 0x0005a275
		fld dword ptr [esi + 0x54]		// 0x0005a27b
		sub esp, 8		// 0x0005a27e
		fstp qword ptr [esp]		// 0x0005a281
		call nxP7CallFpclass		// 0x0005a284
		add esp, 8		// 0x0005a289
		test eax, 0x207		// 0x0005a28c
		jne L5a425		// 0x0005a291
		fld dword ptr [esi + 0x4c]		// 0x0005a297
		fcomp dword ptr [nxP7K101041f0_4]		// 0x0005a29a
		fnstsw ax		// 0x0005a2a0
		test ah, 5		// 0x0005a2a2
		jnp L5a425		// 0x0005a2a5
		fld dword ptr [esi + 0x50]		// 0x0005a2ab
		fcomp dword ptr [nxP7K101041f0_4]		// 0x0005a2ae
		fnstsw ax		// 0x0005a2b4
		test ah, 5		// 0x0005a2b6
		jnp L5a425		// 0x0005a2b9
		fld dword ptr [esi + 0x54]		// 0x0005a2bf
		fcomp dword ptr [nxP7K101041f0_4]		// 0x0005a2c2
		fnstsw ax		// 0x0005a2c8
		test ah, 5		// 0x0005a2ca
		jnp L5a425		// 0x0005a2cd
		fld dword ptr [esi + 8]		// 0x0005a2d3
		sub esp, 8		// 0x0005a2d6
		fstp qword ptr [esp]		// 0x0005a2d9
		call nxP7CallFpclass		// 0x0005a2dc
		add esp, 8		// 0x0005a2e1
		test eax, 0x207		// 0x0005a2e4
		jne L5a425		// 0x0005a2e9
		fld dword ptr [esi + 0xc]		// 0x0005a2ef
		sub esp, 8		// 0x0005a2f2
		fstp qword ptr [esp]		// 0x0005a2f5
		call nxP7CallFpclass		// 0x0005a2f8
		add esp, 8		// 0x0005a2fd
		test eax, 0x207		// 0x0005a300
		jne L5a425		// 0x0005a305
		fld dword ptr [esi + 0x10]		// 0x0005a30b
		sub esp, 8		// 0x0005a30e
		fstp qword ptr [esp]		// 0x0005a311
		call nxP7CallFpclass		// 0x0005a314
		add esp, 8		// 0x0005a319
		test eax, 0x207		// 0x0005a31c
		jne L5a425		// 0x0005a321
		fld dword ptr [esi + 0x14]		// 0x0005a327
		sub esp, 8		// 0x0005a32a
		fstp qword ptr [esp]		// 0x0005a32d
		call nxP7CallFpclass		// 0x0005a330
		add esp, 8		// 0x0005a335
		test eax, 0x207		// 0x0005a338
		jne L5a425		// 0x0005a33d
		fld dword ptr [esi + 0x18]		// 0x0005a343
		sub esp, 8		// 0x0005a346
		fstp qword ptr [esp]		// 0x0005a349
		call nxP7CallFpclass		// 0x0005a34c
		add esp, 8		// 0x0005a351
		test eax, 0x207		// 0x0005a354
		jne L5a425		// 0x0005a359
		fld dword ptr [esi + 0x1c]		// 0x0005a35f
		sub esp, 8		// 0x0005a362
		fstp qword ptr [esp]		// 0x0005a365
		call nxP7CallFpclass		// 0x0005a368
		add esp, 8		// 0x0005a36d
		test eax, 0x207		// 0x0005a370
		jne L5a425		// 0x0005a375
		fld dword ptr [esi + 0x20]		// 0x0005a37b
		sub esp, 8		// 0x0005a37e
		fstp qword ptr [esp]		// 0x0005a381
		call nxP7CallFpclass		// 0x0005a384
		add esp, 8		// 0x0005a389
		test eax, 0x207		// 0x0005a38c
		jne L5a425		// 0x0005a391
		fld dword ptr [esi + 0x24]		// 0x0005a397
		sub esp, 8		// 0x0005a39a
		fstp qword ptr [esp]		// 0x0005a39d
		call nxP7CallFpclass		// 0x0005a3a0
		add esp, 8		// 0x0005a3a5
		test eax, 0x207		// 0x0005a3a8
		jne L5a425		// 0x0005a3ad
		fld dword ptr [esi + 0x28]		// 0x0005a3af
		sub esp, 8		// 0x0005a3b2
		fstp qword ptr [esp]		// 0x0005a3b5
		call nxP7CallFpclass		// 0x0005a3b8
		add esp, 8		// 0x0005a3bd
		test eax, 0x207		// 0x0005a3c0
		jne L5a425		// 0x0005a3c5
		fld dword ptr [esi + 0x2c]		// 0x0005a3c7
		sub esp, 8		// 0x0005a3ca
		fstp qword ptr [esp]		// 0x0005a3cd
		call nxP7CallFpclass		// 0x0005a3d0
		add esp, 8		// 0x0005a3d5
		test eax, 0x207		// 0x0005a3d8
		jne L5a425		// 0x0005a3dd
		fld dword ptr [esi + 0x30]		// 0x0005a3df
		sub esp, 8		// 0x0005a3e2
		fstp qword ptr [esp]		// 0x0005a3e5
		call nxP7CallFpclass		// 0x0005a3e8
		add esp, 8		// 0x0005a3ed
		test eax, 0x207		// 0x0005a3f0
		jne L5a425		// 0x0005a3f5
		fld dword ptr [esi + 0x34]		// 0x0005a3f7
		sub esp, 8		// 0x0005a3fa
		fstp qword ptr [esp]		// 0x0005a3fd
		call nxP7CallFpclass		// 0x0005a400
		add esp, 8		// 0x0005a405
		test eax, 0x207		// 0x0005a408
		jne L5a425		// 0x0005a40d
		cmp word ptr [esi + 0x3c], 0x20		// 0x0005a40f
		jae L5a425		// 0x0005a414
		test dword ptr [esi + 0x38], 0xffff0000		// 0x0005a416
		jne L5a425		// 0x0005a41d
		cmp dword ptr [esi + 4], 6		// 0x0005a41f
		jl L5a429		// 0x0005a423
L5a425:
		xor al, al		// 0x0005a425
		pop esi		// 0x0005a427
		ret		// 0x0005a428
L5a429:
		cmp word ptr [esi + 0x3e], 0xffff		// 0x0005a429
		pop esi		// 0x0005a42f
		setne al		// 0x0005a430
		ret		// 0x0005a433
		}
	}

// helper: phys_fn_002326 (record unchanged) (0x0005a440, 35 B), 13 instructions
__declspec(naked) unsigned __fastcall nxP72326(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		test byte ptr [esp + 4], 1		// 0x0005a440
		push esi		// 0x0005a445
		mov esi, ecx		// 0x0005a446
		mov dword ptr [esi], offset nxP7TabR1 + 12		// 0x0005a448
		je L5a45d		// 0x0005a44e
		mov eax, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005a450
		mov ecx, dword ptr [eax]		// 0x0005a455
		mov edx, dword ptr [ecx]		// 0x0005a457
		push esi		// 0x0005a459
		call dword ptr [edx + 0x14]		// 0x0005a45a
L5a45d:
		mov eax, esi		// 0x0005a45d
		pop esi		// 0x0005a45f
		ret 4		// 0x0005a460
		}
	}

// helper: phys_fn_002328 (record unchanged) (0x0005a470, 49 B), 15 instructions
__declspec(naked) unsigned __fastcall nxP72328(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		test byte ptr [esp + 4], 1		// 0x0005a470
		push esi		// 0x0005a475
		mov esi, ecx		// 0x0005a476
		mov dword ptr [esi + 8], offset nxP7TabR1 + 28		// 0x0005a478
		mov dword ptr [esi + 8], offset nxP7TabR1 + 12		// 0x0005a47f
		mov dword ptr [esi], offset nxP7TabR1		// 0x0005a486
		je L5a49b		// 0x0005a48c
		mov eax, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005a48e
		mov ecx, dword ptr [eax]		// 0x0005a493
		mov edx, dword ptr [ecx]		// 0x0005a495
		push esi		// 0x0005a497
		call dword ptr [edx + 0x14]		// 0x0005a498
L5a49b:
		mov eax, esi		// 0x0005a49b
		pop esi		// 0x0005a49d
		ret 4		// 0x0005a49e
		}
	}

// phys_fn_002332 (0x0005a520, 846 B), 172 instructions
__declspec(naked) unsigned __fastcall nxP72332(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		sub esp, 0x138		// 0x0005a520
		mov eax, dword ptr [esp + 0x140]		// 0x0005a526
		push ebx		// 0x0005a52d
		mov ebx, dword ptr [esp + 0x140]		// 0x0005a52e
		push esi		// 0x0005a535
		push edi		// 0x0005a536
		mov esi, ecx		// 0x0005a537
		push eax		// 0x0005a539
		xor edi, edi		// 0x0005a53a
		lea ecx, [esi + 8]		// 0x0005a53c
		push ebx		// 0x0005a53f
		mov dword ptr [esi + 4], edi		// 0x0005a540
		mov dword ptr [esi], offset nxP7TabR1 + 16		// 0x0005a543
		call nxP72316		// 0x0005a549
		mov dword ptr [esi], offset nxP7TabR2 + 12		// 0x0005a54e
		mov dword ptr [ecx], offset nxP7TabR1 + 28		// 0x0005a554
		mov dword ptr [esi + 4], ecx		// 0x0005a55a
		mov ecx, dword ptr [ebx + 0x30]		// 0x0005a55d
		mov dword ptr [esi + 0x40], ecx		// 0x0005a560
		mov edx, dword ptr [ebx + 0x34]		// 0x0005a563
		mov dword ptr [esi + 0x44], edx		// 0x0005a566
		mov eax, dword ptr [ebx + 0x38]		// 0x0005a569
		mov dword ptr [esi + 0x48], eax		// 0x0005a56c
		fld dword ptr [esi + 0x40]		// 0x0005a56f
		fmul dword ptr [nxP7K1010874c_4]		// 0x0005a572
		mov dword ptr [esp + 0xc0], 0x3f800000		// 0x0005a578
		fld dword ptr [esi + 0x44]		// 0x0005a583
		mov dword ptr [esp + 0xc4], edi		// 0x0005a586
		fmul dword ptr [nxP7K1010874c_4]		// 0x0005a58d
		mov dword ptr [esp + 0xc8], edi		// 0x0005a593
		fld dword ptr [esi + 0x48]		// 0x0005a59a
		mov dword ptr [esp + 0xcc], edi		// 0x0005a59d
		fmul dword ptr [nxP7K1010874c_4]		// 0x0005a5a4
		mov dword ptr [esp + 0xd0], 0x3f800000		// 0x0005a5aa
		mov dword ptr [esp + 0xd4], edi		// 0x0005a5b5
		mov dword ptr [esp + 0xd8], edi		// 0x0005a5bc
		fstp dword ptr [esp + 0x140]		// 0x0005a5c3
		mov dword ptr [esp + 0xdc], edi		// 0x0005a5ca
		fxch st(1)		// 0x0005a5d1
		mov dword ptr [esp + 0xe0], 0x3f800000		// 0x0005a5d3
		fstp dword ptr [esp + 0xb4]		// 0x0005a5de
		mov dword ptr [esp + 0xec], edi		// 0x0005a5e5
		mov dword ptr [esp + 0xe8], edi		// 0x0005a5ec
		mov dword ptr [esp + 0xe4], edi		// 0x0005a5f3
		mov dword ptr [esp + 0xf8], edi		// 0x0005a5fa
		mov dword ptr [esp + 0xf4], edi		// 0x0005a601
		mov dword ptr [esp + 0xf0], edi		// 0x0005a608
		mov dword ptr [esp + 0x108], edi		// 0x0005a60f
		mov dword ptr [esp + 0x104], edi		// 0x0005a616
		mov dword ptr [esp + 0x100], edi		// 0x0005a61d
		mov dword ptr [esp + 0x114], edi		// 0x0005a624
		mov dword ptr [esp + 0x110], edi		// 0x0005a62b
		mov dword ptr [esp + 0x10c], edi		// 0x0005a632
		mov dword ptr [esp + 0x118], 0x3ecccccc		// 0x0005a639
		mov dword ptr [esp + 0xfc], edi		// 0x0005a644
		mov dword ptr [esp + 0x11c], edi		// 0x0005a64b
		mov dword ptr [esp + 0x120], 0x3d4ccccd		// 0x0005a652
		mov dword ptr [esp + 0x124], 0xbf800000		// 0x0005a65d
		mov dword ptr [esp + 0x12c], 0xbf800000		// 0x0005a668
		mov dword ptr [esp + 0x130], 0xbf800000		// 0x0005a673
		mov dword ptr [esp + 0x134], 4		// 0x0005a67e
		mov dword ptr [esp + 0x128], 0x80		// 0x0005a689
		mov dword ptr [esp + 0x6c], 2		// 0x0005a694
		mov dword ptr [esp + 0xb0], edi		// 0x0005a69c
		mov dword ptr [esp + 0x68], offset nxP7TabR2		// 0x0005a6a3
		mov dword ptr [esp + 0x70], 0x3f800000		// 0x0005a6ab
		mov dword ptr [esp + 0x74], edi		// 0x0005a6b3
		mov dword ptr [esp + 0x78], edi		// 0x0005a6b7
		mov dword ptr [esp + 0x7c], edi		// 0x0005a6bb
		mov dword ptr [esp + 0x80], 0x3f800000		// 0x0005a6bf
		mov dword ptr [esp + 0x84], edi		// 0x0005a6ca
		mov dword ptr [esp + 0x88], edi		// 0x0005a6d1
		mov dword ptr [esp + 0x8c], edi		// 0x0005a6d8
		mov dword ptr [esp + 0x90], 0x3f800000		// 0x0005a6df
		mov dword ptr [esp + 0x9c], edi		// 0x0005a6ea
		mov dword ptr [esp + 0x98], edi		// 0x0005a6f1
		mov dword ptr [esp + 0x94], edi		// 0x0005a6f8
		mov dword ptr [esp + 0xa0], 8		// 0x0005a6ff
		mov word ptr [esp + 0xa4], di		// 0x0005a70a
		mov word ptr [esp + 0xa6], di		// 0x0005a712
		mov dword ptr [esp + 0xa8], edi		// 0x0005a71a
		mov dword ptr [esp + 0xac], edi		// 0x0005a721
		mov ecx, dword ptr [esp + 0x140]		// 0x0005a728
		fstp dword ptr [esp + 0xb8]		// 0x0005a72f
		push 8		// 0x0005a736
		mov dword ptr [esp + 0xc0], ecx		// 0x0005a738
		mov dword ptr [esp + 0x5c], edi		// 0x0005a73f
		mov dword ptr [esp + 0x64], edi		// 0x0005a743
		mov dword ptr [esp + 0x40], edi		// 0x0005a747
		mov dword ptr [esp + 0x44], edi		// 0x0005a74b
		mov dword ptr [esp + 0x10], 0x3f800000		// 0x0005a74f
		mov dword ptr [esp + 0x14], edi		// 0x0005a757
		mov dword ptr [esp + 0x18], edi		// 0x0005a75b
		mov dword ptr [esp + 0x1c], edi		// 0x0005a75f
		mov dword ptr [esp + 0x20], 0x3f800000		// 0x0005a763
		mov dword ptr [esp + 0x24], edi		// 0x0005a76b
		mov dword ptr [esp + 0x28], edi		// 0x0005a76f
		mov dword ptr [esp + 0x2c], edi		// 0x0005a773
		mov dword ptr [esp + 0x30], 0x3f800000		// 0x0005a777
		mov dword ptr [esp + 0x3c], edi		// 0x0005a77f
		mov dword ptr [esp + 0x38], edi		// 0x0005a783
		mov dword ptr [esp + 0x34], edi		// 0x0005a787
		mov dword ptr [esp + 0x48], edi		// 0x0005a78b
		mov dword ptr [esp + 0x50], edi		// 0x0005a78f
		mov dword ptr [esp + 0x54], edi		// 0x0005a793
		mov word ptr [esp + 0x4c], di		// 0x0005a797
		mov dword ptr [esp + 0x60], edi		// 0x0005a79c
		mov dword ptr [esp + 0x58], 1		// 0x0005a7a0
		call nxP7CallMalloc		// 0x0005a7a8
		mov ecx, dword ptr [esp + 0x5c]		// 0x0005a7ad
		mov ebx, eax		// 0x0005a7b1
		mov eax, dword ptr [esp + 0x60]		// 0x0005a7b3
		add esp, 4		// 0x0005a7b7
		cmp ecx, eax		// 0x0005a7ba
		mov edx, ebx		// 0x0005a7bc
		je L5a7d8		// 0x0005a7be
		push ebp		// 0x0005a7c0
L5a7c1:
		mov ebp, dword ptr [ecx]		// 0x0005a7c1
		mov dword ptr [edx], ebp		// 0x0005a7c3
		add ecx, 4		// 0x0005a7c5
		add edx, 4		// 0x0005a7c8
		cmp ecx, eax		// 0x0005a7cb
		jne L5a7c1		// 0x0005a7cd
		mov eax, dword ptr [esp + 0x60]		// 0x0005a7cf
		mov ecx, dword ptr [esp + 0x5c]		// 0x0005a7d3
		pop ebp		// 0x0005a7d7
L5a7d8:
		cmp ecx, edi		// 0x0005a7d8
		je L5a7ed		// 0x0005a7da
		push ecx		// 0x0005a7dc
		call nxP7CallFree		// 0x0005a7dd
		mov eax, dword ptr [esp + 0x60]		// 0x0005a7e2
		mov ecx, dword ptr [esp + 0x5c]		// 0x0005a7e6
		add esp, 4		// 0x0005a7ea
L5a7ed:
		sub eax, ecx		// 0x0005a7ed
		sar eax, 2		// 0x0005a7ef
		lea eax, [ebx + eax*4]		// 0x0005a7f2
		lea edx, [ebx + 8]		// 0x0005a7f5
		mov dword ptr [esp + 0x5c], eax		// 0x0005a7f8
		mov dword ptr [esp + 0x60], edx		// 0x0005a7fc
		mov dword ptr [esp + 0x58], ebx		// 0x0005a800
		lea ecx, [esp + 0x68]		// 0x0005a804
		mov dword ptr [eax], ecx		// 0x0005a808
		mov eax, dword ptr [esi + 0x28]		// 0x0005a80a
		mov ebx, dword ptr [esp + 0x5c]		// 0x0005a80d
		mov ecx, dword ptr [esi + 0x2c]		// 0x0005a811
		lea edx, [esp + 0xc0]		// 0x0005a814
		mov dword ptr [esp + 0x30], eax		// 0x0005a81b
		mov dword ptr [esp + 0x3c], edx		// 0x0005a81f
		mov edx, dword ptr [esi + 0x30]		// 0x0005a823
		lea eax, [esp + 0xc]		// 0x0005a826
		mov dword ptr [esp + 0x34], ecx		// 0x0005a82a
		mov ecx, dword ptr [esi + 0x34]		// 0x0005a82e
		add ebx, 4		// 0x0005a831
		push eax		// 0x0005a834
		mov dword ptr [esp + 0x60], ebx		// 0x0005a835
		mov dword ptr [esp + 0x44], 0x41200000		// 0x0005a839
		mov dword ptr [esp + 0x3c], edx		// 0x0005a841
		call nxP7CallInitialise		// 0x0005a845
		mov ecx, dword ptr [eax]		// 0x0005a84a
		mov eax, dword ptr [esp + 0x58]		// 0x0005a84c
		cmp eax, edi		// 0x0005a850
		mov dword ptr [esi + 0x24], ecx		// 0x0005a852
		je L5a860		// 0x0005a855
		push eax		// 0x0005a857
		call nxP7CallFree		// 0x0005a858
		add esp, 4		// 0x0005a85d
L5a860:
		pop edi		// 0x0005a860
		mov eax, esi		// 0x0005a861
		pop esi		// 0x0005a863
		pop ebx		// 0x0005a864
		add esp, 0x138		// 0x0005a865
		ret 8		// 0x0005a86b
		}
	}

// helper: phys_fn_002334 (record unchanged) (0x0005a870, 4 B), 2 instructions
__declspec(naked) unsigned __fastcall nxP72334(void* self, void* edxUnused)
	{
	__asm
		{
		lea eax, [ecx + 0x28]		// 0x0005a870
		ret		// 0x0005a873
		}
	}

// phys_fn_002336 (0x0005a880, 83 B), 34 instructions
__declspec(naked) unsigned __fastcall nxP72336(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x0005a880
		push edi		// 0x0005a881
		mov edi, dword ptr [esp + 0xc]		// 0x0005a882
		mov esi, ecx		// 0x0005a886
		mov ecx, dword ptr [edi + 8]		// 0x0005a888
		xor eax, eax		// 0x0005a88b
		test ecx, ecx		// 0x0005a88d
		jne L5a8ce		// 0x0005a88f
		mov eax, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005a891
		mov ecx, dword ptr [eax]		// 0x0005a896
		mov edx, dword ptr [ecx]		// 0x0005a898
		push 0		// 0x0005a89a
		push 0x4c		// 0x0005a89c
		call dword ptr [edx + 8]		// 0x0005a89e
		test eax, eax		// 0x0005a8a1
		je L5a8b7		// 0x0005a8a3
		push esi		// 0x0005a8a5
		push edi		// 0x0005a8a6
		mov ecx, eax		// 0x0005a8a7
		call nxP72332		// 0x0005a8a9
		test eax, eax		// 0x0005a8ae
		je L5a8b9		// 0x0005a8b0
		lea ecx, [eax + 8]		// 0x0005a8b2
		jmp L5a8bb		// 0x0005a8b5
L5a8b7:
		xor eax, eax		// 0x0005a8b7
L5a8b9:
		xor ecx, ecx		// 0x0005a8b9
L5a8bb:
		test ecx, ecx		// 0x0005a8bb
		je L5a8ce		// 0x0005a8bd
		mov edx, dword ptr [esi + 0x5a8]		// 0x0005a8bf
		mov dword ptr [ecx + 0x30], edx		// 0x0005a8c5
		mov dword ptr [esi + 0x5a8], ecx		// 0x0005a8c8
L5a8ce:
		pop edi		// 0x0005a8ce
		pop esi		// 0x0005a8cf
		ret 4		// 0x0005a8d0
		}
	}

// phys_fn_002350 (0x0005ae50, 1982 B), 647 instructions
__declspec(naked) unsigned __fastcall nxP72350(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ebp		// 0x0005ae50
		mov ebp, esp		// 0x0005ae51
		sub esp, 0x38		// 0x0005ae53
		push ebx		// 0x0005ae56
		mov dword ptr [ebp - 0x2c], ecx		// 0x0005ae57
		push esi		// 0x0005ae5a
		mov esi, dword ptr [ebp + 8]		// 0x0005ae5b
		mov eax, dword ptr [esi + 0x5d4]		// 0x0005ae5e
		mov ebx, dword ptr [eax]		// 0x0005ae64
		mov ecx, dword ptr [eax + 4]		// 0x0005ae66
		sub ecx, ebx		// 0x0005ae69
		sar ecx, 3		// 0x0005ae6b
		mov eax, ebx		// 0x0005ae6e
		mov dword ptr [ebp - 4], eax		// 0x0005ae70
		mov eax, ecx		// 0x0005ae73
		shr eax, 1		// 0x0005ae75
		or eax, ecx		// 0x0005ae77
		mov dword ptr [ebp - 0x24], ecx		// 0x0005ae79
		mov ecx, eax		// 0x0005ae7c
		shr ecx, 2		// 0x0005ae7e
		or eax, ecx		// 0x0005ae81
		mov edx, eax		// 0x0005ae83
		shr edx, 4		// 0x0005ae85
		or eax, edx		// 0x0005ae88
		mov ecx, eax		// 0x0005ae8a
		shr ecx, 8		// 0x0005ae8c
		or eax, ecx		// 0x0005ae8f
		mov ebx, eax		// 0x0005ae91
		shr ebx, 0x10		// 0x0005ae93
		or ebx, eax		// 0x0005ae96
		inc ebx		// 0x0005ae98
		lea eax, [ebx*4]		// 0x0005ae99
		add eax, 3		// 0x0005aea0
		push edi		// 0x0005aea3
		and eax, 0xfffffffc		// 0x0005aea4
		mov dword ptr [ebp - 0x1c], ebx		// 0x0005aea7
		call nxP7CallChkstk		// 0x0005aeaa
		mov dword ptr [ebp + 8], esp		// 0x0005aeaf
		mov dword ptr [ebp - 0x20], 0xffffffff		// 0x0005aeb2
		push eax		// 0x0005aeb9
		push ecx		// 0x0005aeba
		push edi		// 0x0005aebb
		mov edi, dword ptr [ebp + 8]		// 0x0005aebc
		mov ecx, dword ptr [ebp - 0x1c]		// 0x0005aebf
		mov eax, dword ptr [ebp - 0x20]		// 0x0005aec2
		rep stosd		// 0x0005aec5
		pop edi		// 0x0005aec7
		pop ecx		// 0x0005aec8
		pop eax		// 0x0005aec9
		mov edi, dword ptr [ebp - 0x24]		// 0x0005aeca
		lea eax, [edi*4]		// 0x0005aecd
		add eax, 3		// 0x0005aed4
		and eax, 0xfffffffc		// 0x0005aed7
		call nxP7CallChkstk		// 0x0005aeda
		dec ebx		// 0x0005aedf
		xor edx, edx		// 0x0005aee0
		test edi, edi		// 0x0005aee2
		mov dword ptr [ebp - 0x14], esp		// 0x0005aee4
		mov dword ptr [ebp - 0x10], ebx		// 0x0005aee7
		jbe L5af57		// 0x0005aeea
		lea esp, [esp]		// 0x0005aeec
L5aef0:
		mov ecx, dword ptr [ebp - 4]		// 0x0005aef0
		mov eax, dword ptr [ecx + edx*8 + 4]		// 0x0005aef3
		mov ecx, dword ptr [ecx + edx*8]		// 0x0005aef7
		mov ecx, dword ptr [ecx + 0xd4]		// 0x0005aefa
		mov eax, dword ptr [eax + 0xd4]		// 0x0005af00
		and ecx, 0xffff		// 0x0005af06
		shl eax, 0x10		// 0x0005af0c
		or eax, ecx		// 0x0005af0f
		mov ecx, eax		// 0x0005af11
		shl ecx, 0xf		// 0x0005af13
		not ecx		// 0x0005af16
		add eax, ecx		// 0x0005af18
		mov ebx, dword ptr [ebp - 0x10]		// 0x0005af1a
		mov ecx, eax		// 0x0005af1d
		sar ecx, 0xa		// 0x0005af1f
		xor ecx, eax		// 0x0005af22
		lea eax, [ecx + ecx*8]		// 0x0005af24
		mov ecx, eax		// 0x0005af27
		sar ecx, 6		// 0x0005af29
		xor eax, ecx		// 0x0005af2c
		mov ecx, eax		// 0x0005af2e
		shl ecx, 0xb		// 0x0005af30
		not ecx		// 0x0005af33
		add eax, ecx		// 0x0005af35
		mov ecx, eax		// 0x0005af37
		sar ecx, 0x10		// 0x0005af39
		xor ecx, eax		// 0x0005af3c
		mov eax, dword ptr [ebp + 8]		// 0x0005af3e
		and ecx, ebx		// 0x0005af41
		mov eax, dword ptr [eax + ecx*4]		// 0x0005af43
		mov ebx, dword ptr [ebp - 0x14]		// 0x0005af46
		mov dword ptr [ebx + edx*4], eax		// 0x0005af49
		mov eax, dword ptr [ebp + 8]		// 0x0005af4c
		mov dword ptr [eax + ecx*4], edx		// 0x0005af4f
		inc edx		// 0x0005af52
		cmp edx, edi		// 0x0005af53
		jb L5aef0		// 0x0005af55
L5af57:
		mov eax, dword ptr [esi + 0x5d8]		// 0x0005af57
		mov edx, dword ptr [eax]		// 0x0005af5d
		mov ebx, dword ptr [eax + 4]		// 0x0005af5f
		mov eax, edi		// 0x0005af62
		add eax, 3		// 0x0005af64
		sub ebx, edx		// 0x0005af67
		mov ecx, edx		// 0x0005af69
		and eax, 0xfffffffc		// 0x0005af6b
		sar ebx, 3		// 0x0005af6e
		mov dword ptr [ebp - 8], ecx		// 0x0005af71
		call nxP7CallChkstk		// 0x0005af74
		mov edx, esp		// 0x0005af79
		mov ecx, edi		// 0x0005af7b
		mov edi, edx		// 0x0005af7d
		mov dword ptr [ebp - 0xc], edx		// 0x0005af7f
		mov edx, ecx		// 0x0005af82
		shr ecx, 2		// 0x0005af84
		xor eax, eax		// 0x0005af87
		rep stosd		// 0x0005af89
		mov ecx, edx		// 0x0005af8b
		and ecx, 3		// 0x0005af8d
		test ebx, ebx		// 0x0005af90
		rep stosb		// 0x0005af92
		je L5b265		// 0x0005af94
		mov dword ptr [ebp - 0x20], ebx		// 0x0005af9a
		lea ecx, [ecx]		// 0x0005af9d
L5afa0:
		mov ebx, dword ptr [ebp - 8]		// 0x0005afa0
		mov edx, dword ptr [ebx + 4]		// 0x0005afa3
		mov edi, dword ptr [ebx]		// 0x0005afa6
		mov eax, dword ptr [edi + 0xd4]		// 0x0005afa8
		mov ecx, dword ptr [edx + 0xd4]		// 0x0005afae
		shl ecx, 0x10		// 0x0005afb4
		and eax, 0xffff		// 0x0005afb7
		or eax, ecx		// 0x0005afbc
		mov ecx, eax		// 0x0005afbe
		shl ecx, 0xf		// 0x0005afc0
		not ecx		// 0x0005afc3
		add eax, ecx		// 0x0005afc5
		mov ecx, eax		// 0x0005afc7
		sar ecx, 0xa		// 0x0005afc9
		xor ecx, eax		// 0x0005afcc
		lea eax, [ecx + ecx*8]		// 0x0005afce
		mov ecx, eax		// 0x0005afd1
		sar ecx, 6		// 0x0005afd3
		xor eax, ecx		// 0x0005afd6
		mov ecx, eax		// 0x0005afd8
		shl ecx, 0xb		// 0x0005afda
		not ecx		// 0x0005afdd
		add eax, ecx		// 0x0005afdf
		mov ecx, eax		// 0x0005afe1
		sar ecx, 0x10		// 0x0005afe3
		xor ecx, eax		// 0x0005afe6
		and ecx, dword ptr [ebp - 0x10]		// 0x0005afe8
		mov eax, dword ptr [ebp + 8]		// 0x0005afeb
		mov eax, dword ptr [eax + ecx*4]		// 0x0005afee
		cmp eax, -1		// 0x0005aff1
		mov dword ptr [ebp - 0x18], edx		// 0x0005aff4
		mov dword ptr [ebp - 0x1c], edi		// 0x0005aff7
		je L5b024		// 0x0005affa
		lea esp, [esp]		// 0x0005affc
L5b000:
		mov ecx, dword ptr [ebp - 4]		// 0x0005b000
		cmp dword ptr [ecx + eax*8], edi		// 0x0005b003
		jne L5b00e		// 0x0005b006
		cmp dword ptr [ecx + eax*8 + 4], edx		// 0x0005b008
		je L5b01b		// 0x0005b00c
L5b00e:
		mov ecx, dword ptr [ebp - 0x14]		// 0x0005b00e
		mov eax, dword ptr [ecx + eax*4]		// 0x0005b011
		cmp eax, -1		// 0x0005b014
		jne L5b000		// 0x0005b017
		jmp L5b024		// 0x0005b019
L5b01b:
		cmp eax, -1		// 0x0005b01b
		jne L5b12e		// 0x0005b01e
L5b024:
		mov ebx, dword ptr [esi + 0x604]		// 0x0005b024
		mov eax, dword ptr [esi + 0x600]		// 0x0005b02a
		cmp ebx, eax		// 0x0005b030
		ja L5b11a		// 0x0005b032
		mov ecx, dword ptr [esi + 0x5fc]		// 0x0005b038
		sub eax, ecx		// 0x0005b03e
		mov edx, eax		// 0x0005b040
		mov eax, 0x2aaaaaab		// 0x0005b042
		imul edx		// 0x0005b047
		sar edx, 1		// 0x0005b049
		mov eax, edx		// 0x0005b04b
		shr eax, 0x1f		// 0x0005b04d
		add eax, edx		// 0x0005b050
		test ecx, ecx		// 0x0005b052
		lea edi, [eax + eax + 2]		// 0x0005b054
		jne L5b05e		// 0x0005b058
		xor eax, eax		// 0x0005b05a
		jmp L5b070		// 0x0005b05c
L5b05e:
		sub ebx, ecx		// 0x0005b05e
		mov eax, 0x2aaaaaab		// 0x0005b060
		imul ebx		// 0x0005b065
		sar edx, 1		// 0x0005b067
		mov eax, edx		// 0x0005b069
		shr eax, 0x1f		// 0x0005b06b
		add eax, edx		// 0x0005b06e
L5b070:
		cmp eax, edi		// 0x0005b070
		jae L5b114		// 0x0005b072
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b078
		mov ecx, dword ptr [ecx]		// 0x0005b07e
		mov edx, dword ptr [ecx]		// 0x0005b080
		lea edi, [edi + edi*2]		// 0x0005b082
		push 0		// 0x0005b085
		shl edi, 2		// 0x0005b087
		push edi		// 0x0005b08a
		call dword ptr [edx + 8]		// 0x0005b08b
		mov ecx, dword ptr [esi + 0x600]		// 0x0005b08e
		mov ebx, eax		// 0x0005b094
		mov eax, dword ptr [esi + 0x5fc]		// 0x0005b096
		cmp eax, ecx		// 0x0005b09c
		mov dword ptr [ebp - 0x28], ecx		// 0x0005b09e
		je L5b0c3		// 0x0005b0a1
		lea ecx, [ebx + 8]		// 0x0005b0a3
L5b0a6:
		mov edx, dword ptr [eax]		// 0x0005b0a6
		mov dword ptr [ecx - 8], edx		// 0x0005b0a8
		mov edx, dword ptr [eax + 4]		// 0x0005b0ab
		mov dword ptr [ecx - 4], edx		// 0x0005b0ae
		mov edx, dword ptr [eax + 8]		// 0x0005b0b1
		mov dword ptr [ecx], edx		// 0x0005b0b4
		mov edx, dword ptr [ebp - 0x28]		// 0x0005b0b6
		add eax, 0xc		// 0x0005b0b9
		add ecx, 0xc		// 0x0005b0bc
		cmp eax, edx		// 0x0005b0bf
		jne L5b0a6		// 0x0005b0c1
L5b0c3:
		mov eax, dword ptr [esi + 0x5fc]		// 0x0005b0c3
		test eax, eax		// 0x0005b0c9
		je L5b0db		// 0x0005b0cb
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b0cd
		mov ecx, dword ptr [ecx]		// 0x0005b0d3
		mov edx, dword ptr [ecx]		// 0x0005b0d5
		push eax		// 0x0005b0d7
		call dword ptr [edx + 0x14]		// 0x0005b0d8
L5b0db:
		mov edx, dword ptr [esi + 0x5fc]		// 0x0005b0db
		mov ecx, dword ptr [esi + 0x600]		// 0x0005b0e1
		lea eax, [ebx + edi]		// 0x0005b0e7
		mov dword ptr [esi + 0x604], eax		// 0x0005b0ea
		sub ecx, edx		// 0x0005b0f0
		mov eax, 0x2aaaaaab		// 0x0005b0f2
		imul ecx		// 0x0005b0f7
		sar edx, 1		// 0x0005b0f9
		mov eax, edx		// 0x0005b0fb
		shr eax, 0x1f		// 0x0005b0fd
		add eax, edx		// 0x0005b100
		lea ecx, [eax + eax*2]		// 0x0005b102
		lea edx, [ebx + ecx*4]		// 0x0005b105
		mov dword ptr [esi + 0x600], edx		// 0x0005b108
		mov dword ptr [esi + 0x5fc], ebx		// 0x0005b10e
L5b114:
		mov edx, dword ptr [ebp - 0x18]		// 0x0005b114
		mov edi, dword ptr [ebp - 0x1c]		// 0x0005b117
L5b11a:
		mov eax, dword ptr [esi + 0x600]		// 0x0005b11a
		mov dword ptr [eax], edi		// 0x0005b120
		mov dword ptr [eax + 8], 1		// 0x0005b122
		jmp L5b245		// 0x0005b129
L5b12e:
		mov ecx, dword ptr [ebp - 0xc]		// 0x0005b12e
		mov byte ptr [eax + ecx], 1		// 0x0005b131
		mov eax, dword ptr [ebx + 4]		// 0x0005b135
		mov ecx, dword ptr [esi + 0x604]		// 0x0005b138
		mov edx, dword ptr [ebx]		// 0x0005b13e
		mov dword ptr [ebp - 0x34], eax		// 0x0005b140
		mov eax, dword ptr [esi + 0x600]		// 0x0005b143
		cmp ecx, eax		// 0x0005b149
		mov dword ptr [ebp - 0x38], edx		// 0x0005b14b
		ja L5b230		// 0x0005b14e
		mov edi, dword ptr [esi + 0x5fc]		// 0x0005b154
		sub eax, edi		// 0x0005b15a
		mov edx, eax		// 0x0005b15c
		mov eax, 0x2aaaaaab		// 0x0005b15e
		imul edx		// 0x0005b163
		sar edx, 1		// 0x0005b165
		mov eax, edx		// 0x0005b167
		shr eax, 0x1f		// 0x0005b169
		add eax, edx		// 0x0005b16c
		test edi, edi		// 0x0005b16e
		lea ebx, [eax + eax + 2]		// 0x0005b170
		jne L5b17a		// 0x0005b174
		xor eax, eax		// 0x0005b176
		jmp L5b18c		// 0x0005b178
L5b17a:
		sub ecx, edi		// 0x0005b17a
		mov eax, 0x2aaaaaab		// 0x0005b17c
		imul ecx		// 0x0005b181
		sar edx, 1		// 0x0005b183
		mov eax, edx		// 0x0005b185
		shr eax, 0x1f		// 0x0005b187
		add eax, edx		// 0x0005b18a
L5b18c:
		cmp eax, ebx		// 0x0005b18c
		jae L5b230		// 0x0005b18e
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b194
		mov ecx, dword ptr [ecx]		// 0x0005b19a
		mov edx, dword ptr [ecx]		// 0x0005b19c
		lea edi, [ebx + ebx*2]		// 0x0005b19e
		push 0		// 0x0005b1a1
		shl edi, 2		// 0x0005b1a3
		push edi		// 0x0005b1a6
		call dword ptr [edx + 8]		// 0x0005b1a7
		mov ecx, dword ptr [esi + 0x600]		// 0x0005b1aa
		mov ebx, eax		// 0x0005b1b0
		mov eax, dword ptr [esi + 0x5fc]		// 0x0005b1b2
		cmp eax, ecx		// 0x0005b1b8
		mov dword ptr [ebp - 0x28], ecx		// 0x0005b1ba
		je L5b1df		// 0x0005b1bd
		lea ecx, [ebx + 8]		// 0x0005b1bf
L5b1c2:
		mov edx, dword ptr [eax]		// 0x0005b1c2
		mov dword ptr [ecx - 8], edx		// 0x0005b1c4
		mov edx, dword ptr [eax + 4]		// 0x0005b1c7
		mov dword ptr [ecx - 4], edx		// 0x0005b1ca
		mov edx, dword ptr [eax + 8]		// 0x0005b1cd
		mov dword ptr [ecx], edx		// 0x0005b1d0
		mov edx, dword ptr [ebp - 0x28]		// 0x0005b1d2
		add eax, 0xc		// 0x0005b1d5
		add ecx, 0xc		// 0x0005b1d8
		cmp eax, edx		// 0x0005b1db
		jne L5b1c2		// 0x0005b1dd
L5b1df:
		mov eax, dword ptr [esi + 0x5fc]		// 0x0005b1df
		test eax, eax		// 0x0005b1e5
		je L5b1f7		// 0x0005b1e7
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b1e9
		mov ecx, dword ptr [ecx]		// 0x0005b1ef
		mov edx, dword ptr [ecx]		// 0x0005b1f1
		push eax		// 0x0005b1f3
		call dword ptr [edx + 0x14]		// 0x0005b1f4
L5b1f7:
		mov edx, dword ptr [esi + 0x5fc]		// 0x0005b1f7
		mov ecx, dword ptr [esi + 0x600]		// 0x0005b1fd
		lea eax, [ebx + edi]		// 0x0005b203
		mov dword ptr [esi + 0x604], eax		// 0x0005b206
		sub ecx, edx		// 0x0005b20c
		mov eax, 0x2aaaaaab		// 0x0005b20e
		imul ecx		// 0x0005b213
		sar edx, 1		// 0x0005b215
		mov eax, edx		// 0x0005b217
		shr eax, 0x1f		// 0x0005b219
		add eax, edx		// 0x0005b21c
		lea ecx, [eax + eax*2]		// 0x0005b21e
		lea edx, [ebx + ecx*4]		// 0x0005b221
		mov dword ptr [esi + 0x600], edx		// 0x0005b224
		mov dword ptr [esi + 0x5fc], ebx		// 0x0005b22a
L5b230:
		mov eax, dword ptr [esi + 0x600]		// 0x0005b230
		mov ecx, dword ptr [ebp - 0x38]		// 0x0005b236
		mov edx, dword ptr [ebp - 0x34]		// 0x0005b239
		mov dword ptr [eax], ecx		// 0x0005b23c
		mov dword ptr [eax + 8], 4		// 0x0005b23e
L5b245:
		mov ecx, dword ptr [ebp - 8]		// 0x0005b245
		mov dword ptr [eax + 4], edx		// 0x0005b248
		add dword ptr [esi + 0x600], 0xc		// 0x0005b24b
		mov eax, dword ptr [ebp - 0x20]		// 0x0005b252
		add ecx, 8		// 0x0005b255
		dec eax		// 0x0005b258
		mov dword ptr [ebp - 8], ecx		// 0x0005b259
		mov dword ptr [ebp - 0x20], eax		// 0x0005b25c
		jne L5afa0		// 0x0005b25f
L5b265:
		mov eax, dword ptr [ebp - 0x24]		// 0x0005b265
		test eax, eax		// 0x0005b268
		je L5b5e3		// 0x0005b26a
		mov dword ptr [ebp - 0x24], eax		// 0x0005b270
L5b273:
		mov eax, dword ptr [ebp - 0xc]		// 0x0005b273
		mov cl, byte ptr [eax]		// 0x0005b276
		inc eax		// 0x0005b278
		test cl, cl		// 0x0005b279
		mov dword ptr [ebp - 0xc], eax		// 0x0005b27b
		jne L5b5cd		// 0x0005b27e
		mov ecx, dword ptr [ebp - 4]		// 0x0005b284
		mov edi, dword ptr [ecx]		// 0x0005b287
		mov ebx, dword ptr [ecx + 4]		// 0x0005b289
		mov edx, dword ptr [edi + 0xd0]		// 0x0005b28c
		cmp edx, dword ptr [ebx + 0xd0]		// 0x0005b292
		mov dword ptr [ebp - 8], edi		// 0x0005b298
		mov dword ptr [ebp + 8], ebx		// 0x0005b29b
		jle L5b2ab		// 0x0005b29e
		mov dword ptr [ebp + 8], edi		// 0x0005b2a0
		mov dword ptr [ebp - 8], ebx		// 0x0005b2a3
		mov edi, ebx		// 0x0005b2a6
		mov ebx, dword ptr [ebp + 8]		// 0x0005b2a8
L5b2ab:
		mov eax, dword ptr [edi + 0xd0]		// 0x0005b2ab
		mov edx, dword ptr [ebx + 0xd0]		// 0x0005b2b1
		lea eax, [eax + eax*2]		// 0x0005b2b7
		lea eax, [edx + eax*2]		// 0x0005b2ba
		mov edx, dword ptr [ebp - 0x2c]		// 0x0005b2bd
		mov eax, dword ptr [edx + eax*4 + 0x94]		// 0x0005b2c0
		test eax, eax		// 0x0005b2c7
		je L5b4ac		// 0x0005b2c9
		push esi		// 0x0005b2cf
		push ebx		// 0x0005b2d0
		push edi		// 0x0005b2d1
		call eax		// 0x0005b2d2
		add esp, 0xc		// 0x0005b2d4
		test al, al		// 0x0005b2d7
		je L5b4a9		// 0x0005b2d9
		test byte ptr [ebx + 0xde], 7		// 0x0005b2df
		je L5b2f0		// 0x0005b2e6
		mov dword ptr [ebp - 8], ebx		// 0x0005b2e8
		mov dword ptr [ebp + 8], edi		// 0x0005b2eb
		mov ebx, edi		// 0x0005b2ee
L5b2f0:
		mov edi, dword ptr [esi + 0x5d8]		// 0x0005b2f0
		mov edx, dword ptr [edi + 8]		// 0x0005b2f6
		mov eax, dword ptr [edi + 4]		// 0x0005b2f9
		cmp edx, eax		// 0x0005b2fc
		ja L5b38d		// 0x0005b2fe
		mov ecx, dword ptr [edi]		// 0x0005b304
		sub eax, ecx		// 0x0005b306
		sar eax, 3		// 0x0005b308
		test ecx, ecx		// 0x0005b30b
		lea eax, [eax + eax + 2]		// 0x0005b30d
		jne L5b317		// 0x0005b311
		xor edx, edx		// 0x0005b313
		jmp L5b31c		// 0x0005b315
L5b317:
		sub edx, ecx		// 0x0005b317
		sar edx, 3		// 0x0005b319
L5b31c:
		cmp edx, eax		// 0x0005b31c
		jae L5b38d		// 0x0005b31e
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b320
		mov ecx, dword ptr [ecx]		// 0x0005b326
		mov edx, dword ptr [ecx]		// 0x0005b328
		shl eax, 3		// 0x0005b32a
		push 0		// 0x0005b32d
		push eax		// 0x0005b32f
		mov dword ptr [ebp - 0x20], eax		// 0x0005b330
		call dword ptr [edx + 8]		// 0x0005b333
		mov edx, dword ptr [edi + 4]		// 0x0005b336
		mov ebx, eax		// 0x0005b339
		mov eax, dword ptr [edi]		// 0x0005b33b
		cmp eax, edx		// 0x0005b33d
		mov dword ptr [ebp - 0x28], ebx		// 0x0005b33f
		mov ecx, ebx		// 0x0005b342
		je L5b35d		// 0x0005b344
L5b346:
		mov ebx, dword ptr [eax]		// 0x0005b346
		mov dword ptr [ecx], ebx		// 0x0005b348
		mov ebx, dword ptr [eax + 4]		// 0x0005b34a
		mov dword ptr [ecx + 4], ebx		// 0x0005b34d
		add eax, 8		// 0x0005b350
		add ecx, 8		// 0x0005b353
		cmp eax, edx		// 0x0005b356
		jne L5b346		// 0x0005b358
		mov ebx, dword ptr [ebp - 0x28]		// 0x0005b35a
L5b35d:
		mov eax, dword ptr [edi]		// 0x0005b35d
		test eax, eax		// 0x0005b35f
		je L5b371		// 0x0005b361
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b363
		mov ecx, dword ptr [ecx]		// 0x0005b369
		mov edx, dword ptr [ecx]		// 0x0005b36b
		push eax		// 0x0005b36d
		call dword ptr [edx + 0x14]		// 0x0005b36e
L5b371:
		mov eax, dword ptr [ebp - 0x20]		// 0x0005b371
		mov edx, dword ptr [edi + 4]		// 0x0005b374
		lea ecx, [ebx + eax]		// 0x0005b377
		mov dword ptr [edi + 8], ecx		// 0x0005b37a
		sub edx, dword ptr [edi]		// 0x0005b37d
		sar edx, 3		// 0x0005b37f
		lea eax, [ebx + edx*8]		// 0x0005b382
		mov dword ptr [edi], ebx		// 0x0005b385
		mov ebx, dword ptr [ebp + 8]		// 0x0005b387
		mov dword ptr [edi + 4], eax		// 0x0005b38a
L5b38d:
		mov ecx, dword ptr [edi + 4]		// 0x0005b38d
		mov edx, dword ptr [ebp - 8]		// 0x0005b390
		mov dword ptr [ecx], edx		// 0x0005b393
		mov dword ptr [ecx + 4], ebx		// 0x0005b395
		add dword ptr [edi + 4], 8		// 0x0005b398
		mov edi, dword ptr [esi + 0x604]		// 0x0005b39c
		mov eax, dword ptr [esi + 0x600]		// 0x0005b3a2
		cmp edi, eax		// 0x0005b3a8
		ja L5b491		// 0x0005b3aa
		mov ecx, dword ptr [esi + 0x5fc]		// 0x0005b3b0
		sub eax, ecx		// 0x0005b3b6
		mov edx, eax		// 0x0005b3b8
		mov eax, 0x2aaaaaab		// 0x0005b3ba
		imul edx		// 0x0005b3bf
		sar edx, 1		// 0x0005b3c1
		mov eax, edx		// 0x0005b3c3
		shr eax, 0x1f		// 0x0005b3c5
		add eax, edx		// 0x0005b3c8
		test ecx, ecx		// 0x0005b3ca
		lea ebx, [eax + eax + 2]		// 0x0005b3cc
		jne L5b3d6		// 0x0005b3d0
		xor eax, eax		// 0x0005b3d2
		jmp L5b3e8		// 0x0005b3d4
L5b3d6:
		sub edi, ecx		// 0x0005b3d6
		mov eax, 0x2aaaaaab		// 0x0005b3d8
		imul edi		// 0x0005b3dd
		sar edx, 1		// 0x0005b3df
		mov eax, edx		// 0x0005b3e1
		shr eax, 0x1f		// 0x0005b3e3
		add eax, edx		// 0x0005b3e6
L5b3e8:
		cmp eax, ebx		// 0x0005b3e8
		jae L5b48e		// 0x0005b3ea
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b3f0
		mov ecx, dword ptr [ecx]		// 0x0005b3f6
		mov edx, dword ptr [ecx]		// 0x0005b3f8
		lea edi, [ebx + ebx*2]		// 0x0005b3fa
		push 0		// 0x0005b3fd
		shl edi, 2		// 0x0005b3ff
		push edi		// 0x0005b402
		call dword ptr [edx + 8]		// 0x0005b403
		mov ecx, dword ptr [esi + 0x600]		// 0x0005b406
		mov ebx, eax		// 0x0005b40c
		mov eax, dword ptr [esi + 0x5fc]		// 0x0005b40e
		cmp eax, ecx		// 0x0005b414
		mov dword ptr [ebp - 0x28], ecx		// 0x0005b416
		je L5b43d		// 0x0005b419
		lea ecx, [ebx + 8]		// 0x0005b41b
		mov edi, edi		// 0x0005b41e
L5b420:
		mov edx, dword ptr [eax]		// 0x0005b420
		mov dword ptr [ecx - 8], edx		// 0x0005b422
		mov edx, dword ptr [eax + 4]		// 0x0005b425
		mov dword ptr [ecx - 4], edx		// 0x0005b428
		mov edx, dword ptr [eax + 8]		// 0x0005b42b
		mov dword ptr [ecx], edx		// 0x0005b42e
		mov edx, dword ptr [ebp - 0x28]		// 0x0005b430
		add eax, 0xc		// 0x0005b433
		add ecx, 0xc		// 0x0005b436
		cmp eax, edx		// 0x0005b439
		jne L5b420		// 0x0005b43b
L5b43d:
		mov eax, dword ptr [esi + 0x5fc]		// 0x0005b43d
		test eax, eax		// 0x0005b443
		je L5b455		// 0x0005b445
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b447
		mov ecx, dword ptr [ecx]		// 0x0005b44d
		mov edx, dword ptr [ecx]		// 0x0005b44f
		push eax		// 0x0005b451
		call dword ptr [edx + 0x14]		// 0x0005b452
L5b455:
		mov edx, dword ptr [esi + 0x5fc]		// 0x0005b455
		mov ecx, dword ptr [esi + 0x600]		// 0x0005b45b
		lea eax, [ebx + edi]		// 0x0005b461
		mov dword ptr [esi + 0x604], eax		// 0x0005b464
		sub ecx, edx		// 0x0005b46a
		mov eax, 0x2aaaaaab		// 0x0005b46c
		imul ecx		// 0x0005b471
		sar edx, 1		// 0x0005b473
		mov eax, edx		// 0x0005b475
		shr eax, 0x1f		// 0x0005b477
		add eax, edx		// 0x0005b47a
		lea ecx, [eax + eax*2]		// 0x0005b47c
		lea edx, [ebx + ecx*4]		// 0x0005b47f
		mov dword ptr [esi + 0x600], edx		// 0x0005b482
		mov dword ptr [esi + 0x5fc], ebx		// 0x0005b488
L5b48e:
		mov ebx, dword ptr [ebp + 8]		// 0x0005b48e
L5b491:
		mov eax, dword ptr [esi + 0x600]		// 0x0005b491
		mov ecx, dword ptr [ebp - 8]		// 0x0005b497
		mov dword ptr [eax + 4], ebx		// 0x0005b49a
		mov dword ptr [eax + 8], 4		// 0x0005b49d
		jmp L5b5c4		// 0x0005b4a4
L5b4a9:
		mov ecx, dword ptr [ebp - 4]		// 0x0005b4a9
L5b4ac:
		mov eax, dword ptr [ecx + 4]		// 0x0005b4ac
		mov edi, dword ptr [esi + 0x604]		// 0x0005b4af
		mov edx, dword ptr [ecx]		// 0x0005b4b5
		mov dword ptr [ebp - 0x34], eax		// 0x0005b4b7
		mov eax, dword ptr [esi + 0x600]		// 0x0005b4ba
		cmp edi, eax		// 0x0005b4c0
		mov dword ptr [ebp - 0x38], edx		// 0x0005b4c2
		ja L5b5ae		// 0x0005b4c5
		mov ecx, dword ptr [esi + 0x5fc]		// 0x0005b4cb
		sub eax, ecx		// 0x0005b4d1
		mov edx, eax		// 0x0005b4d3
		mov eax, 0x2aaaaaab		// 0x0005b4d5
		imul edx		// 0x0005b4da
		sar edx, 1		// 0x0005b4dc
		mov eax, edx		// 0x0005b4de
		shr eax, 0x1f		// 0x0005b4e0
		add eax, edx		// 0x0005b4e3
		test ecx, ecx		// 0x0005b4e5
		lea ebx, [eax + eax + 2]		// 0x0005b4e7
		jne L5b4f1		// 0x0005b4eb
		xor eax, eax		// 0x0005b4ed
		jmp L5b503		// 0x0005b4ef
L5b4f1:
		sub edi, ecx		// 0x0005b4f1
		mov eax, 0x2aaaaaab		// 0x0005b4f3
		imul edi		// 0x0005b4f8
		sar edx, 1		// 0x0005b4fa
		mov eax, edx		// 0x0005b4fc
		shr eax, 0x1f		// 0x0005b4fe
		add eax, edx		// 0x0005b501
L5b503:
		cmp eax, ebx		// 0x0005b503
		jae L5b5ae		// 0x0005b505
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b50b
		mov ecx, dword ptr [ecx]		// 0x0005b511
		mov edx, dword ptr [ecx]		// 0x0005b513
		lea edi, [ebx + ebx*2]		// 0x0005b515
		push 0		// 0x0005b518
		shl edi, 2		// 0x0005b51a
		push edi		// 0x0005b51d
		call dword ptr [edx + 8]		// 0x0005b51e
		mov ecx, dword ptr [esi + 0x600]		// 0x0005b521
		mov ebx, eax		// 0x0005b527
		mov eax, dword ptr [esi + 0x5fc]		// 0x0005b529
		cmp eax, ecx		// 0x0005b52f
		mov dword ptr [ebp + 8], ecx		// 0x0005b531
		je L5b55d		// 0x0005b534
		lea ecx, [ebx + 8]		// 0x0005b536
		lea esp, [esp]		// 0x0005b539
L5b540:
		mov edx, dword ptr [eax]		// 0x0005b540
		mov dword ptr [ecx - 8], edx		// 0x0005b542
		mov edx, dword ptr [eax + 4]		// 0x0005b545
		mov dword ptr [ecx - 4], edx		// 0x0005b548
		mov edx, dword ptr [eax + 8]		// 0x0005b54b
		mov dword ptr [ecx], edx		// 0x0005b54e
		mov edx, dword ptr [ebp + 8]		// 0x0005b550
		add eax, 0xc		// 0x0005b553
		add ecx, 0xc		// 0x0005b556
		cmp eax, edx		// 0x0005b559
		jne L5b540		// 0x0005b55b
L5b55d:
		mov eax, dword ptr [esi + 0x5fc]		// 0x0005b55d
		test eax, eax		// 0x0005b563
		je L5b575		// 0x0005b565
		mov ecx, dword ptr [nxP7ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005b567
		mov ecx, dword ptr [ecx]		// 0x0005b56d
		mov edx, dword ptr [ecx]		// 0x0005b56f
		push eax		// 0x0005b571
		call dword ptr [edx + 0x14]		// 0x0005b572
L5b575:
		mov edx, dword ptr [esi + 0x5fc]		// 0x0005b575
		mov ecx, dword ptr [esi + 0x600]		// 0x0005b57b
		lea eax, [ebx + edi]		// 0x0005b581
		mov dword ptr [esi + 0x604], eax		// 0x0005b584
		sub ecx, edx		// 0x0005b58a
		mov eax, 0x2aaaaaab		// 0x0005b58c
		imul ecx		// 0x0005b591
		sar edx, 1		// 0x0005b593
		mov eax, edx		// 0x0005b595
		shr eax, 0x1f		// 0x0005b597
		add eax, edx		// 0x0005b59a
		lea ecx, [eax + eax*2]		// 0x0005b59c
		lea edx, [ebx + ecx*4]		// 0x0005b59f
		mov dword ptr [esi + 0x600], edx		// 0x0005b5a2
		mov dword ptr [esi + 0x5fc], ebx		// 0x0005b5a8
L5b5ae:
		mov eax, dword ptr [esi + 0x600]		// 0x0005b5ae
		mov edx, dword ptr [ebp - 0x34]		// 0x0005b5b4
		mov ecx, dword ptr [ebp - 0x38]		// 0x0005b5b7
		mov dword ptr [eax + 4], edx		// 0x0005b5ba
		mov dword ptr [eax + 8], 2		// 0x0005b5bd
L5b5c4:
		mov dword ptr [eax], ecx		// 0x0005b5c4
		add dword ptr [esi + 0x600], 0xc		// 0x0005b5c6
L5b5cd:
		mov ecx, dword ptr [ebp - 4]		// 0x0005b5cd
		mov eax, dword ptr [ebp - 0x24]		// 0x0005b5d0
		add ecx, 8		// 0x0005b5d3
		dec eax		// 0x0005b5d6
		mov dword ptr [ebp - 4], ecx		// 0x0005b5d7
		mov dword ptr [ebp - 0x24], eax		// 0x0005b5da
		jne L5b273		// 0x0005b5dd
L5b5e3:
		mov ecx, dword ptr [esi + 0x5d8]		// 0x0005b5e3
		mov eax, dword ptr [esi + 0x5d4]		// 0x0005b5e9
		mov dword ptr [esi + 0x5d4], ecx		// 0x0005b5ef
		mov dword ptr [esi + 0x5d8], eax		// 0x0005b5f5
		mov esi, eax		// 0x0005b5fb
		mov edx, dword ptr [esi]		// 0x0005b5fd
		mov dword ptr [esi + 4], edx		// 0x0005b5ff
		lea esp, [ebp - 0x44]		// 0x0005b602
		pop edi		// 0x0005b605
		pop esi		// 0x0005b606
		pop ebx		// 0x0005b607
		mov esp, ebp		// 0x0005b608
		pop ebp		// 0x0005b60a
		ret 4		// 0x0005b60b
		}
	}

// helper: phys_fn_002371 (record unchanged) (0x0005b7e0, 10 B), 4 instructions
__declspec(naked) unsigned __fastcall nxP72371(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [ecx]		// 0x0005b7e0
		push eax		// 0x0005b7e2
		call dword ptr [nxP7ImpImpResetEvent4]		// 0x0005b7e3
		ret		// 0x0005b7e9
		}
	}

// helper: phys_fn_002373 (record unchanged) (0x0005b7f0, 10 B), 4 instructions
__declspec(naked) unsigned __fastcall nxP72373(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [ecx]		// 0x0005b7f0
		push eax		// 0x0005b7f2
		call dword ptr [nxP7ImpImpSetEvent4]		// 0x0005b7f3
		ret		// 0x0005b7f9
		}
	}

// helper: phys_fn_002375 (record unchanged) (0x0005b800, 30 B), 12 instructions
__declspec(naked) unsigned __fastcall nxP72375(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x0005b800
		cmp eax, -1		// 0x0005b804
		jne L5b80b		// 0x0005b807
		or eax, eax		// 0x0005b809
L5b80b:
		push eax		// 0x0005b80b
		mov eax, dword ptr [ecx]		// 0x0005b80c
		push eax		// 0x0005b80e
		call dword ptr [nxP7ImpImpWaitForSingleObject8]		// 0x0005b80f
		neg eax		// 0x0005b815
		sbb al, al		// 0x0005b817
		inc al		// 0x0005b819
		ret 4		// 0x0005b81b
		}
	}

// helper: phys_fn_002387 (record unchanged) (0x0005b8e0, 7 B), 3 instructions
__declspec(naked) unsigned __fastcall nxP72387(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [ecx + 4]		// 0x0005b8e0
		mov al, byte ptr [eax + 8]		// 0x0005b8e3
		ret		// 0x0005b8e6
		}
	}

// helper: phys_fn_002389 (record unchanged) (0x0005b8f0, 32 B), 18 instructions
__declspec(naked) unsigned __fastcall nxP72389(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [ecx + 4]		// 0x0005b8f0
		push 0		// 0x0005b8f3
		mov dword ptr [eax + 4], 2		// 0x0005b8f5
		call dword ptr [nxP7ImpImpExitThread4]		// 0x0005b8fc
		int 3		// 0x0005b902
		int 3		// 0x0005b903
		int 3		// 0x0005b904
		int 3		// 0x0005b905
		int 3		// 0x0005b906
		int 3		// 0x0005b907
		int 3		// 0x0005b908
		int 3		// 0x0005b909
		int 3		// 0x0005b90a
		int 3		// 0x0005b90b
		int 3		// 0x0005b90c
		int 3		// 0x0005b90d
		int 3		// 0x0005b90e
		int 3		// 0x0005b90f
		}
	}

// phys_fn_002400 (0x0005b9e0, 90 B), 26 instructions
__declspec(naked) unsigned __fastcall nxP72400(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x0005b9e0
		mov esi, ecx		// 0x0005b9e1
L5b9e3:
		mov ecx, dword ptr [esi + 8]		// 0x0005b9e3
		push -1		// 0x0005b9e6
		call nxP72375		// 0x0005b9e8
		mov ecx, dword ptr [esi + 8]		// 0x0005b9ed
		call nxP72371		// 0x0005b9f0
		mov ecx, esi		// 0x0005b9f5
		call nxP72387		// 0x0005b9f7
		test al, al		// 0x0005b9fc
		je L5ba07		// 0x0005b9fe
		mov ecx, esi		// 0x0005ba00
		call nxP72389		// 0x0005ba02
L5ba07:
		mov ecx, dword ptr [esi + 0x14]		// 0x0005ba07
		call nxP7CallLock		// 0x0005ba0a
		mov eax, dword ptr [esi + 0x10]		// 0x0005ba0f
		mov ecx, dword ptr [eax + 0x6cc]		// 0x0005ba12
		mov ecx, dword ptr [ecx + 0xc]		// 0x0005ba18
		call nxP7CallLock		// 0x0005ba1b
		mov ecx, dword ptr [esi + 0x10]		// 0x0005ba20
		call nxP7CallSimulateFrame		// 0x0005ba23
		mov ecx, dword ptr [esi + 0x14]		// 0x0005ba28
		call nxP7CallUnlock		// 0x0005ba2b
		mov ecx, dword ptr [esi + 0xc]		// 0x0005ba30
		call nxP72373		// 0x0005ba33
		jmp L5b9e3		// 0x0005ba38
		}
	}

// helper: phys_fn_004031 (record unchanged) (0x00091de0, 31 B), 11 instructions
__declspec(naked) unsigned __fastcall nxP74031(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		test byte ptr [esp + 4], 1		// 0x00091de0
		push esi		// 0x00091de5
		mov esi, ecx		// 0x00091de6
		mov dword ptr [esi], offset nxP7VtShapeDesc		// 0x00091de8
		je L91df9		// 0x00091dee
		push esi		// 0x00091df0
		call nxP7CallFree		// 0x00091df1
		add esp, 4		// 0x00091df6
L91df9:
		mov eax, esi		// 0x00091df9
		pop esi		// 0x00091dfb
		ret 4		// 0x00091dfc
		}
	}
// @@ROWS-END@@
