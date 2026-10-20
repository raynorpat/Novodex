/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// PrunerOctree.cpp: the loose octree the bounded dynamic pruner (the scene's type-1 pruner) keeps: a 0x50-byte
// header (the world box, its centre negated, the root side and six level scales, the depth, the node count and the
// node array at +0x4c) over 32-byte node records (a box, an object count and the head of a list of objects).
// Naked listing transcriptions of phys_fn_005295 (0x000e7250), 005303 (0x000e7380), 005305 (0x000e7570), 005307
// (0x000e75d0), 005309 (0x000e75f0), 005313 (0x000e7690), 005315 (0x000e76c0), 005317 (0x000e7850) and 005318
// (0x000e7b80), with the two helpers they inline from elsewhere (000923, AABB::SetCenterExtents, and 005145,
// AABB::IsInside), and of the queries over it: 005545 (0x000f1970, sphere against a box), 005547 (0x000f1a30, a
// node's objects appended), 005549 (0x000f1a70, a subtree's), 005550 and 005552 (0x000f1ac0 and 0x000f1c60, the
// plane walk and its entry), 005554 and 005556 (0x000f1cb0 and 0x000f1d10, the segment walk and entry) and 005558
// and 005560 (0x000f1d50 and 0x000f1db0, the ray walk and entry), with the oracle's float to integer conversion
// (0x000f7a3c, its _ftol2: a copy of the CRT's rounds differently at the edges), 005562 and 005564 (0x000f1df0 and
// 0x000f2050, the sphere walk and entry).
// The pruners' own rows follow: the static pruner's destructor body (005234), plane query (005236) and scalar deleting
// destructor (005238); the bounded dynamic pruner's constructor (005240), tree build (005244 and 005246, one function),
// AddObject, RemoveObject and UpdateObject (005248, 005250, 005252) and its plane, ray/segment and sphere queries with
// their function-local static result containers (005254, 005256, 005258 and 005260, one function); and what they
// call that is the oracle's own: the tree culling walk (005527) and its collecting callback (005223), the octree's
// constructor and release (005301, 005320), the empty box (002148) and the atexit thunks of the statics (006322 to
// 006330). The statics are the array nxPCStatics, laid out as the oracle's block at 0x1012847c. The octree's jump table (0x000e7544, eight cases) is a chain of compares here: the inline
// assembler cannot emit a table of label addresses; the compare script checks the chain against the table.
// Compare script: docs/reconstruction/novodex-physics/evidence/phase4-pruner-octree-listing-compare.py.
#include "Opcode.h"
#include "IcePrunable.h"

extern "C" void nxPOCallAllocator();
extern "C" void nxPOCallMakeCube();
extern "C" void nxPQCallContainerResize();
extern "C" void nxPQCallSegmentAABB();
extern "C" void nxPQCallRayAABB();
extern "C" void nxPCCallContainerEmpty();
extern "C" void nxPCCallContainerCtor();
extern "C" void nxPCCallTreeDtor();
extern "C" void nxPCCallPrunerDtor();
extern "C" void nxPCCallPrunerCtor();
extern "C" void nxPCCallBuildStaticTree();
extern "C" void nxPCCallUpdateWorldAABB();
extern "C" void nxPCCallAABBAdd();
extern "C" void nxPCCallPoolAdd();
extern "C" void nxPCCallPoolRemove();
extern "C" void nxPCCallAtexit();
extern "C" void* nxPOAdapterNotify;
#pragma comment(linker, "/alternatename:_nxPOCallAllocator=?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ")
#pragma comment(linker, "/alternatename:_nxPOCallMakeCube=?MakeCube@AABB@IceMaths@@QBEMAAV12@@Z")
#pragma comment(linker, "/alternatename:_nxPQCallContainerResize=?resize@SdkContainer@@QAE_NI@Z")
#pragma comment(linker, "/alternatename:_nxPQCallSegmentAABB=?nxSegmentAABB@@YA_NABVSegment@IceMaths@@ABVPoint@2@1@Z")
#pragma comment(linker, "/alternatename:_nxPQCallRayAABB=?nxRayAABB@@YA_NABVRay@IceMaths@@ABVPoint@2@1@Z")
#pragma comment(linker, "/alternatename:_nxPCCallContainerEmpty=?empty@SdkContainer@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxPCCallContainerCtor=??0SdkContainer@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxPCCallTreeDtor=??1AABBTree@Opcode@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxPCCallPrunerDtor=??1Pruner@@UAE@XZ")
#pragma comment(linker, "/alternatename:_nxPCCallPrunerCtor=??0Pruner@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxPCCallBuildStaticTree=?BuildTree@StaticPruner@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxPCCallUpdateWorldAABB=?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z")
#pragma comment(linker, "/alternatename:_nxPCCallAABBAdd=?Add@AABB@IceMaths@@QAEAAV12@ABV12@@Z")
#pragma comment(linker, "/alternatename:_nxPCCallPoolAdd=?AddObject@PruningPool@@QAE_NPAVPrunable@@@Z")
#pragma comment(linker, "/alternatename:_nxPCCallPoolRemove=?RemoveObject@PruningPool@@QAEXPAVPrunable@@@Z")
#pragma comment(linker, "/alternatename:_nxPCCallAtexit=_atexit")
#pragma comment(linker, "/alternatename:_nxPOAdapterNotify=?gPrunableAdapterNotify@@3P6AXPAVPrunable@@PAVAABB@IceMaths@@@ZA")

void __fastcall nxPOSetCenterExtents(void* aabb, void* edxUnused, const void* center, const void* extents);
bool __fastcall nxPOIsInside(const void* self, void* edxUnused, const void* other);
void __fastcall nxPOScaleBounds(void* bounds, void* edxUnused, float scale);
void __fastcall nxPOSetNodeBounds(void* tree, void* edxUnused, unsigned node, const void* bounds);
void __fastcall nxPOWalk(void* tree, void* edxUnused, unsigned node, void* callback, void* user0, void* user1);
bool __cdecl nxPOLeafCallback(void* a, void* b);
bool __fastcall nxPORemove(void* tree, void* edxUnused, void* object);
bool __fastcall nxPOForEach(void* tree, void* edxUnused, void* callback, void* user);
bool __fastcall nxPOBuild(void* tree, void* edxUnused, unsigned depth, const void* bounds);
bool __fastcall nxPOInsert(void* tree, void* edxUnused, void* object, const void* bounds);
bool __fastcall nxPOUpdate(void* tree, void* edxUnused, void* object);
bool __cdecl nxPQSphereAABB(const float* center, float radius, const float* boxMin, const float* boxMax);
void __fastcall nxPQAppendNode(void* node, void* edxUnused, void* container);
void __cdecl nxPQAppendSubtree(unsigned node, unsigned count, void* nodes, void* container);
void __cdecl nxPQPlaneWalk(unsigned node, unsigned mask, void* query);
void __fastcall nxPQPlaneQuery(void* tree, void* edxUnused, const void* planes, unsigned nbPlanes, void* containerA, void* containerB);
void __cdecl nxPQSegmentWalk(unsigned node, void* query);
void __fastcall nxPQSegmentQuery(void* tree, void* edxUnused, const void* segment, void* container);
void __cdecl nxPQRayWalk(unsigned node, void* query);
void __fastcall nxPQRayQuery(void* tree, void* edxUnused, const void* ray, void* container);
void __cdecl nxPQSphereWalk(unsigned node, void* query);
void __fastcall nxPQSphereQuery(void* tree, void* edxUnused, const void* sphere, void* containerA, void* containerB);
void __fastcall nxPCBoxEmpty(void* aabb);
void* __fastcall nxPCOctreeCtor(void* tree);
void __fastcall nxPCOctreeRelease(void* tree);
void __cdecl nxPCCollectCallback(void* a, void* b, void* c, void* d);
void __fastcall nxPCTreeCull(const void* node, void* edxUnused, const void* planes, unsigned mask, void* callback, void* user);
void __cdecl nxPCThunkB();
void __cdecl nxPCThunkA();
void __cdecl nxPCThunkC();
void __cdecl nxPCThunkD();
void __cdecl nxPCThunkE();
void __fastcall nxPCStaticDtorBody(void* pruner);
bool __fastcall nxPCStaticPlanes(void* pruner, void* edxUnused, unsigned a, unsigned b, unsigned c, unsigned d, unsigned e);
void* __fastcall nxPCStaticScalarDelete(void* pruner, void* edxUnused, unsigned flags);
void* __fastcall nxPCBoundedCtor(void* pruner);
bool __fastcall nxPCBuildTree(void* pruner);
bool __fastcall nxPCAddObject(void* pruner, void* edxUnused, void* object);
bool __fastcall nxPCRemoveObject(void* pruner, void* edxUnused, void* object);
bool __fastcall nxPCUpdateObject(void* pruner, void* edxUnused, void* object);
bool __fastcall nxPCQueryPlanes(void* pruner, void* edxUnused, void* result, const void* planes, unsigned nbPlanes, unsigned flags, unsigned mask);
bool __fastcall nxPCQueryRays(void* pruner, void* edxUnused, void* result, const void* ray, float maxDist, unsigned firstContact, unsigned mask);
bool __fastcall nxPCQuerySphere(void* pruner, void* edxUnused, void* result, const void* bounds, unsigned firstContact, unsigned mask);
void __cdecl nxPOFtol();

unsigned nxPCStatics[24];		// the oracle's block 0x1012847c..0x101284d8: five containers and the guard words of the three functions using them
static const unsigned nxPOK101041f0_4 = 0x0;
static const unsigned nxPOK101043cc_4 = 0x3f000000;
static const unsigned nxPOK101066f8_4 = 0x4f800000;
static const unsigned nxPOK10106858_4 = 0x7f7fffff;
static const unsigned nxPOK10113638_4 = 0x3e800000;
static const unsigned nxPOK1011ba20_4 = 0x3fb8aa3b;
static const unsigned nxPOK1011ba24_4 = 0x3c000000;
static const unsigned nxPOK1011ba28_4 = 0x3c800000;
static const unsigned nxPOK1011ba2c_4 = 0x3d000000;
static const unsigned nxPOK1011ba30_4 = 0x3d800000;
static const unsigned nxPOK1011ba34_4 = 0x3e000000;

// phys_fn_000923 (inlined AABB::SetCenterExtents) (0x000203c0, 120 B), 41 instructions
__declspec(naked) void __fastcall nxPOSetCenterExtents(void* aabb, void* edxUnused, const void* center, const void* extents)
	{
	__asm
		{
		sub esp, 0xc		// 0x000203c0
		mov eax, dword ptr [esp + 0x10]		// 0x000203c3
		fld dword ptr [eax]		// 0x000203c7
		mov edx, dword ptr [esp + 0x14]		// 0x000203c9
		fsub dword ptr [edx]		// 0x000203cd
		push esi		// 0x000203cf
		push edi		// 0x000203d0
		mov esi, ecx		// 0x000203d1
		fstp dword ptr [esp + 8]		// 0x000203d3
		mov edi, dword ptr [esp + 8]		// 0x000203d7
		fld dword ptr [eax + 4]		// 0x000203db
		add ecx, 0xc		// 0x000203de
		fsub dword ptr [edx + 4]		// 0x000203e1
		fstp dword ptr [esp + 0xc]		// 0x000203e4
		fld dword ptr [eax + 8]		// 0x000203e8
		fsub dword ptr [edx + 8]		// 0x000203eb
		mov dword ptr [esi], edi		// 0x000203ee
		mov edi, dword ptr [esp + 0xc]		// 0x000203f0
		mov dword ptr [esi + 4], edi		// 0x000203f4
		fstp dword ptr [esp + 0x10]		// 0x000203f7
		mov edi, dword ptr [esp + 0x10]		// 0x000203fb
		mov dword ptr [esi + 8], edi		// 0x000203ff
		fld dword ptr [eax]		// 0x00020402
		fadd dword ptr [edx]		// 0x00020404
		pop edi		// 0x00020406
		pop esi		// 0x00020407
		fstp dword ptr [esp]		// 0x00020408
		fld dword ptr [eax + 4]		// 0x0002040b
		fadd dword ptr [edx + 4]		// 0x0002040e
		fstp dword ptr [esp + 4]		// 0x00020411
		fld dword ptr [edx + 8]		// 0x00020415
		mov edx, dword ptr [esp + 4]		// 0x00020418
		fadd dword ptr [eax + 8]		// 0x0002041c
		mov eax, dword ptr [esp]		// 0x0002041f
		mov dword ptr [ecx], eax		// 0x00020422
		mov dword ptr [ecx + 4], edx		// 0x00020424
		fstp dword ptr [esp + 8]		// 0x00020427
		mov eax, dword ptr [esp + 8]		// 0x0002042b
		mov dword ptr [ecx + 8], eax		// 0x0002042f
		add esp, 0xc		// 0x00020432
		ret 8		// 0x00020435
		}
	}

// phys_fn_005145 (AABB::IsInside) (0x000e2f70, 90 B), 35 instructions
__declspec(naked) bool __fastcall nxPOIsInside(const void* self, void* edxUnused, const void* other)
	{
	__asm
		{
		mov edx, dword ptr [esp + 4]		// 0x000e2f70
		fld dword ptr [edx]		// 0x000e2f74
		fcomp dword ptr [ecx]		// 0x000e2f76
		fnstsw ax		// 0x000e2f78
		test ah, 0x41		// 0x000e2f7a
		je Le2fc0		// 0x000e2f7d
		fld dword ptr [edx + 4]		// 0x000e2f7f
		fcomp dword ptr [ecx + 4]		// 0x000e2f82
		fnstsw ax		// 0x000e2f85
		test ah, 0x41		// 0x000e2f87
		je Le2fc0		// 0x000e2f8a
		fld dword ptr [edx + 8]		// 0x000e2f8c
		fcomp dword ptr [ecx + 8]		// 0x000e2f8f
		fnstsw ax		// 0x000e2f92
		test ah, 0x41		// 0x000e2f94
		je Le2fc0		// 0x000e2f97
		fld dword ptr [edx + 0xc]		// 0x000e2f99
		fcomp dword ptr [ecx + 0xc]		// 0x000e2f9c
		fnstsw ax		// 0x000e2f9f
		test ah, 5		// 0x000e2fa1
		jnp Le2fc0		// 0x000e2fa4
		fld dword ptr [edx + 0x10]		// 0x000e2fa6
		fcomp dword ptr [ecx + 0x10]		// 0x000e2fa9
		fnstsw ax		// 0x000e2fac
		test ah, 5		// 0x000e2fae
		jnp Le2fc0		// 0x000e2fb1
		fld dword ptr [edx + 0x14]		// 0x000e2fb3
		fcomp dword ptr [ecx + 0x14]		// 0x000e2fb6
		fnstsw ax		// 0x000e2fb9
		test ah, 5		// 0x000e2fbb
		jp Le2fc5		// 0x000e2fbe
Le2fc0:
		xor al, al		// 0x000e2fc0
		ret 4		// 0x000e2fc2
Le2fc5:
		mov al, 1		// 0x000e2fc5
		ret 4		// 0x000e2fc7
		}
	}

// phys_fn_005295 (0x000e7250, 217 B), 59 instructions
__declspec(naked) void __fastcall nxPOScaleBounds(void* bounds, void* edxUnused, float scale)
	{
	__asm
		{
		sub esp, 0x24		// 0x000e7250
		fld dword ptr [ecx + 0xc]		// 0x000e7253
		fadd dword ptr [ecx]		// 0x000e7256
		fld dword ptr [ecx + 0x10]		// 0x000e7258
		fadd dword ptr [ecx + 4]		// 0x000e725b
		fld dword ptr [ecx + 0x14]		// 0x000e725e
		fadd dword ptr [ecx + 8]		// 0x000e7261
		fstp dword ptr [esp + 0x20]		// 0x000e7264
		fxch st(1)		// 0x000e7268
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e726a
		fstp dword ptr [esp]		// 0x000e7270
		mov eax, dword ptr [esp]		// 0x000e7273
		mov dword ptr [esp + 0x18], eax		// 0x000e7276
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e727a
		fstp dword ptr [esp + 4]		// 0x000e7280
		mov edx, dword ptr [esp + 4]		// 0x000e7284
		fld dword ptr [esp + 0x20]		// 0x000e7288
		mov dword ptr [esp + 0x1c], edx		// 0x000e728c
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e7290
		fstp dword ptr [esp + 8]		// 0x000e7296
		mov eax, dword ptr [esp + 8]		// 0x000e729a
		fld dword ptr [ecx + 0xc]		// 0x000e729e
		mov dword ptr [esp + 0x20], eax		// 0x000e72a1
		fsub dword ptr [ecx]		// 0x000e72a5
		fld dword ptr [ecx + 0x10]		// 0x000e72a7
		fsub dword ptr [ecx + 4]		// 0x000e72aa
		fld dword ptr [ecx + 0x14]		// 0x000e72ad
		fsub dword ptr [ecx + 8]		// 0x000e72b0
		fstp dword ptr [esp + 0x14]		// 0x000e72b3
		fxch st(1)		// 0x000e72b7
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e72b9
		fstp dword ptr [esp]		// 0x000e72bf
		mov edx, dword ptr [esp]		// 0x000e72c2
		mov dword ptr [esp + 0xc], edx		// 0x000e72c5
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e72c9
		fstp dword ptr [esp + 4]		// 0x000e72cf
		mov eax, dword ptr [esp + 4]		// 0x000e72d3
		fld dword ptr [esp + 0x14]		// 0x000e72d7
		mov dword ptr [esp + 0x10], eax		// 0x000e72db
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e72df
		lea eax, [esp]		// 0x000e72e5
		push eax		// 0x000e72e8
		fstp dword ptr [esp + 0xc]		// 0x000e72e9
		mov edx, dword ptr [esp + 0xc]		// 0x000e72ed
		fld dword ptr [esp + 0x10]		// 0x000e72f1
		mov dword ptr [esp + 0x18], edx		// 0x000e72f5
		fmul dword ptr [esp + 0x2c]		// 0x000e72f9
		lea edx, [esp + 0x1c]		// 0x000e72fd
		push edx		// 0x000e7301
		fstp dword ptr [esp + 8]		// 0x000e7302
		fld dword ptr [esp + 0x18]		// 0x000e7306
		fmul dword ptr [esp + 0x30]		// 0x000e730a
		fstp dword ptr [esp + 0xc]		// 0x000e730e
		fld dword ptr [esp + 0x1c]		// 0x000e7312
		fmul dword ptr [esp + 0x30]		// 0x000e7316
		fstp dword ptr [esp + 0x10]		// 0x000e731a
		call nxPOSetCenterExtents		// 0x000e731e
		add esp, 0x24		// 0x000e7323
		ret 4		// 0x000e7326
		}
	}

// phys_fn_005303 (0x000e7380, 452 B), 137 instructions
__declspec(naked) void __fastcall nxPOSetNodeBounds(void* tree, void* edxUnused, unsigned node, const void* bounds)
	{
	__asm
		{
		sub esp, 0x40		// 0x000e7380
		mov eax, dword ptr [esp + 0x44]		// 0x000e7383
		cmp eax, dword ptr [ecx + 0x48]		// 0x000e7387
		mov dword ptr [esp], ecx		// 0x000e738a
		jae Le753e		// 0x000e738d
		push ebx		// 0x000e7393
		push ebp		// 0x000e7394
		push esi		// 0x000e7395
		mov esi, dword ptr [ecx + 0x4c]		// 0x000e7396
		mov edx, eax		// 0x000e7399
		shl edx, 5		// 0x000e739b
		add esi, edx		// 0x000e739e
		push edi		// 0x000e73a0
		mov edi, dword ptr [esp + 0x58]		// 0x000e73a1
		mov edx, edi		// 0x000e73a5
		mov ebx, dword ptr [edx]		// 0x000e73a7
		mov dword ptr [esi], ebx		// 0x000e73a9
		mov ebx, dword ptr [edx + 4]		// 0x000e73ab
		mov dword ptr [esi + 4], ebx		// 0x000e73ae
		mov ebx, dword ptr [edx + 8]		// 0x000e73b1
		mov dword ptr [esi + 8], ebx		// 0x000e73b4
		mov ebx, dword ptr [edx + 0xc]		// 0x000e73b7
		mov dword ptr [esi + 0xc], ebx		// 0x000e73ba
		mov ebx, dword ptr [edx + 0x10]		// 0x000e73bd
		mov dword ptr [esi + 0x10], ebx		// 0x000e73c0
		mov edx, dword ptr [edx + 0x14]		// 0x000e73c3
		mov dword ptr [esi + 0x14], edx		// 0x000e73c6
		fld dword ptr [edi + 0xc]		// 0x000e73c9
		fadd dword ptr [edi]		// 0x000e73cc
		lea ebx, [edi + 0xc]		// 0x000e73ce
		fld dword ptr [ebx + 4]		// 0x000e73d1
		lea ebp, [eax*8 + 1]		// 0x000e73d4
		fadd dword ptr [edi + 4]		// 0x000e73db
		xor esi, esi		// 0x000e73de
		fld dword ptr [ebx + 8]		// 0x000e73e0
		mov dword ptr [esp + 0x54], 8		// 0x000e73e3
		fadd dword ptr [edi + 8]		// 0x000e73eb
		fstp dword ptr [esp + 0x34]		// 0x000e73ee
		fxch st(1)		// 0x000e73f2
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e73f4
		fstp dword ptr [esp + 0x20]		// 0x000e73fa
		mov edx, dword ptr [esp + 0x20]		// 0x000e73fe
		mov dword ptr [esp + 0x2c], edx		// 0x000e7402
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e7406
		fstp dword ptr [esp + 0x24]		// 0x000e740c
		mov edx, dword ptr [esp + 0x24]		// 0x000e7410
		fld dword ptr [esp + 0x34]		// 0x000e7414
		mov dword ptr [esp + 0x30], edx		// 0x000e7418
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e741c
		fstp dword ptr [esp + 0x28]		// 0x000e7422
		mov edx, dword ptr [esp + 0x28]		// 0x000e7426
		mov dword ptr [esp + 0x34], edx		// 0x000e742a
		jmp Le7434		// 0x000e742e
Le7430:
		mov ecx, dword ptr [esp + 0x10]		// 0x000e7430
Le7434:
		cmp esi, 7		// 0x000e7434
		fld dword ptr [esp + 0x34]		// 0x000e7437
		fld dword ptr [esp + 0x30]		// 0x000e743b
		mov eax, edi		// 0x000e743f
		mov edx, dword ptr [eax]		// 0x000e7441
		fld dword ptr [esp + 0x2c]		// 0x000e7443
		mov dword ptr [esp + 0x14], edx		// 0x000e7447
		mov edx, dword ptr [eax + 4]		// 0x000e744b
		mov eax, dword ptr [eax + 8]		// 0x000e744e
		mov dword ptr [esp + 0x18], edx		// 0x000e7451
		mov dword ptr [esp + 0x1c], eax		// 0x000e7455
		mov edx, ebx		// 0x000e7459
		mov eax, dword ptr [edx]		// 0x000e745b
		mov dword ptr [esp + 0x20], eax		// 0x000e745d
		mov eax, dword ptr [edx + 4]		// 0x000e7461
		mov edx, dword ptr [edx + 8]		// 0x000e7464
		mov dword ptr [esp + 0x24], eax		// 0x000e7467
		mov dword ptr [esp + 0x28], edx		// 0x000e746b
		ja Le74e8		// 0x000e746f
		cmp esi, 0		// jump table 0x000e7544[0]
		je Le7478
		cmp esi, 1		// jump table 0x000e7544[1]
		je Le7486
		cmp esi, 2		// jump table 0x000e7544[2]
		je Le7494
		cmp esi, 3		// jump table 0x000e7544[3]
		je Le74a2
		cmp esi, 4		// jump table 0x000e7544[4]
		je Le74b0
		cmp esi, 5		// jump table 0x000e7544[5]
		je Le74be
		cmp esi, 6		// jump table 0x000e7544[6]
		je Le74cc
		jmp Le74da		// jump table 0x000e7544[7]		// 0x000e7471
Le7478:
		fstp dword ptr [esp + 0x20]		// 0x000e7478
		fstp dword ptr [esp + 0x24]		// 0x000e747c
		fstp dword ptr [esp + 0x28]		// 0x000e7480
		jmp Le74ee		// 0x000e7484
Le7486:
		fstp dword ptr [esp + 0x20]		// 0x000e7486
		fstp dword ptr [esp + 0x24]		// 0x000e748a
		fstp dword ptr [esp + 0x1c]		// 0x000e748e
		jmp Le74ee		// 0x000e7492
Le7494:
		fstp dword ptr [esp + 0x20]		// 0x000e7494
		fstp dword ptr [esp + 0x18]		// 0x000e7498
		fstp dword ptr [esp + 0x28]		// 0x000e749c
		jmp Le74ee		// 0x000e74a0
Le74a2:
		fstp dword ptr [esp + 0x20]		// 0x000e74a2
		fstp dword ptr [esp + 0x18]		// 0x000e74a6
		fstp dword ptr [esp + 0x1c]		// 0x000e74aa
		jmp Le74ee		// 0x000e74ae
Le74b0:
		fstp dword ptr [esp + 0x14]		// 0x000e74b0
		fstp dword ptr [esp + 0x24]		// 0x000e74b4
		fstp dword ptr [esp + 0x28]		// 0x000e74b8
		jmp Le74ee		// 0x000e74bc
Le74be:
		fstp dword ptr [esp + 0x14]		// 0x000e74be
		fstp dword ptr [esp + 0x24]		// 0x000e74c2
		fstp dword ptr [esp + 0x1c]		// 0x000e74c6
		jmp Le74ee		// 0x000e74ca
Le74cc:
		fstp dword ptr [esp + 0x14]		// 0x000e74cc
		fstp dword ptr [esp + 0x18]		// 0x000e74d0
		fstp dword ptr [esp + 0x28]		// 0x000e74d4
		jmp Le74ee		// 0x000e74d8
Le74da:
		fstp dword ptr [esp + 0x14]		// 0x000e74da
		fstp dword ptr [esp + 0x18]		// 0x000e74de
		fstp dword ptr [esp + 0x1c]		// 0x000e74e2
		jmp Le74ee		// 0x000e74e6
Le74e8:
		fstp st(0)		// 0x000e74e8
		fstp st(0)		// 0x000e74ea
		fstp st(0)		// 0x000e74ec
Le74ee:
		mov eax, dword ptr [esp + 0x14]		// 0x000e74ee
		mov edx, dword ptr [esp + 0x18]		// 0x000e74f2
		mov dword ptr [esp + 0x38], eax		// 0x000e74f6
		mov eax, dword ptr [esp + 0x1c]		// 0x000e74fa
		mov dword ptr [esp + 0x40], eax		// 0x000e74fe
		mov eax, dword ptr [esp + 0x24]		// 0x000e7502
		mov dword ptr [esp + 0x3c], edx		// 0x000e7506
		mov edx, dword ptr [esp + 0x20]		// 0x000e750a
		mov dword ptr [esp + 0x48], eax		// 0x000e750e
		lea eax, [esp + 0x38]		// 0x000e7512
		mov dword ptr [esp + 0x44], edx		// 0x000e7516
		mov edx, dword ptr [esp + 0x28]		// 0x000e751a
		push eax		// 0x000e751e
		push ebp		// 0x000e751f
		mov dword ptr [esp + 0x54], edx		// 0x000e7520
		call nxPOSetNodeBounds		// 0x000e7524
		mov eax, dword ptr [esp + 0x54]		// 0x000e7529
		inc esi		// 0x000e752d
		inc ebp		// 0x000e752e
		dec eax		// 0x000e752f
		mov dword ptr [esp + 0x54], eax		// 0x000e7530
		jne Le7430		// 0x000e7534
		pop edi		// 0x000e753a
		pop esi		// 0x000e753b
		pop ebp		// 0x000e753c
		pop ebx		// 0x000e753d
Le753e:
		add esp, 0x40		// 0x000e753e
		ret 8		// 0x000e7541
		}
	}

// phys_fn_005305 (0x000e7570, 89 B), 40 instructions
__declspec(naked) void __fastcall nxPOWalk(void* tree, void* edxUnused, unsigned node, void* callback, void* user0, void* user1)
	{
	__asm
		{
		push ebx		// 0x000e7570
		mov ebx, ecx		// 0x000e7571
		mov eax, dword ptr [ebx + 0x48]		// 0x000e7573
		push ebp		// 0x000e7576
		mov ebp, dword ptr [esp + 0xc]		// 0x000e7577
		cmp ebp, eax		// 0x000e757b
		jae Le75c4		// 0x000e757d
		mov edx, dword ptr [ebx + 0x4c]		// 0x000e757f
		mov eax, dword ptr [esp + 0x18]		// 0x000e7582
		mov ecx, dword ptr [esp + 0x10]		// 0x000e7586
		push edi		// 0x000e758a
		mov edi, ebp		// 0x000e758b
		shl edi, 5		// 0x000e758d
		push eax		// 0x000e7590
		add edi, edx		// 0x000e7591
		push edi		// 0x000e7593
		push ecx		// 0x000e7594
		call dword ptr [esp + 0x24]		// 0x000e7595
		add esp, 0xc		// 0x000e7599
		test al, al		// 0x000e759c
		je Le75c3		// 0x000e759e
		push esi		// 0x000e75a0
		mov esi, 1		// 0x000e75a1
Le75a6:
		mov edx, dword ptr [esp + 0x20]		// 0x000e75a6
		mov eax, dword ptr [esp + 0x1c]		// 0x000e75aa
		push edx		// 0x000e75ae
		push eax		// 0x000e75af
		push edi		// 0x000e75b0
		lea ecx, [esi + ebp*8]		// 0x000e75b1
		push ecx		// 0x000e75b4
		mov ecx, ebx		// 0x000e75b5
		call nxPOWalk		// 0x000e75b7
		inc esi		// 0x000e75bc
		cmp esi, 8		// 0x000e75bd
		jbe Le75a6		// 0x000e75c0
		pop esi		// 0x000e75c2
Le75c3:
		pop edi		// 0x000e75c3
Le75c4:
		pop ebp		// 0x000e75c4
		pop ebx		// 0x000e75c5
		ret 0x10		// 0x000e75c6
		}
	}

// phys_fn_005307 (0x000e75d0, 21 B), 7 instructions
__declspec(naked) bool __cdecl nxPOLeafCallback(void* a, void* b)
	{
	__asm
		{
		mov ecx, dword ptr [esp + 8]		// 0x000e75d0
		test ecx, ecx		// 0x000e75d4
		je Le75e2		// 0x000e75d6
		push 0x40000000		// 0x000e75d8
		call nxPOScaleBounds		// 0x000e75dd
Le75e2:
		mov al, 1		// 0x000e75e2
		ret		// 0x000e75e4
		}
	}

// phys_fn_005309 (0x000e75f0, 117 B), 49 instructions
__declspec(naked) bool __fastcall nxPORemove(void* tree, void* edxUnused, void* object)
	{
	__asm
		{
		push esi		// 0x000e75f0
		mov esi, dword ptr [esp + 8]		// 0x000e75f1
		mov eax, dword ptr [esi + 0x10]		// 0x000e75f5
		test eax, eax		// 0x000e75f8
		jne Le7602		// 0x000e75fa
		xor al, al		// 0x000e75fc
		pop esi		// 0x000e75fe
		ret 4		// 0x000e75ff
Le7602:
		sub eax, dword ptr [ecx + 0x4c]		// 0x000e7602
		shr eax, 5		// 0x000e7605
		test eax, eax		// 0x000e7608
		je Le762a		// 0x000e760a
		push edi		// 0x000e760c
		lea ecx, [ecx]		// 0x000e760d
Le7610:
		mov edi, dword ptr [ecx + 0x4c]		// 0x000e7610
		mov edx, eax		// 0x000e7613
		shl edx, 5		// 0x000e7615
		lea edx, [edx + edi + 0x18]		// 0x000e7618
		mov edi, dword ptr [edx]		// 0x000e761c
		dec edi		// 0x000e761e
		dec eax		// 0x000e761f
		shr eax, 3		// 0x000e7620
		test eax, eax		// 0x000e7623
		mov dword ptr [edx], edi		// 0x000e7625
		jne Le7610		// 0x000e7627
		pop edi		// 0x000e7629
Le762a:
		mov ecx, dword ptr [ecx + 0x4c]		// 0x000e762a
		dec dword ptr [ecx + 0x18]		// 0x000e762d
		mov eax, dword ptr [esi + 8]		// 0x000e7630
		test eax, eax		// 0x000e7633
		je Le763d		// 0x000e7635
		mov ecx, dword ptr [esi + 0xc]		// 0x000e7637
		mov dword ptr [eax + 0xc], ecx		// 0x000e763a
Le763d:
		mov eax, dword ptr [esi + 0xc]		// 0x000e763d
		test eax, eax		// 0x000e7640
		je Le764a		// 0x000e7642
		mov edx, dword ptr [esi + 8]		// 0x000e7644
		mov dword ptr [eax + 8], edx		// 0x000e7647
Le764a:
		mov ecx, dword ptr [esi + 0x10]		// 0x000e764a
		cmp dword ptr [ecx + 0x1c], esi		// 0x000e764d
		jne Le765f		// 0x000e7650
		mov eax, dword ptr [esi + 8]		// 0x000e7652
		test eax, eax		// 0x000e7655
		jne Le765c		// 0x000e7657
		mov eax, dword ptr [esi + 0xc]		// 0x000e7659
Le765c:
		mov dword ptr [ecx + 0x1c], eax		// 0x000e765c
Le765f:
		mov al, 1		// 0x000e765f
		pop esi		// 0x000e7661
		ret 4		// 0x000e7662
		}
	}

// phys_fn_005313 (0x000e7690, 33 B), 13 instructions
__declspec(naked) bool __fastcall nxPOForEach(void* tree, void* edxUnused, void* callback, void* user)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x000e7690
		test eax, eax		// 0x000e7694
		jne Le769d		// 0x000e7696
		xor al, al		// 0x000e7698
		ret 8		// 0x000e769a
Le769d:
		mov edx, dword ptr [esp + 8]		// 0x000e769d
		push edx		// 0x000e76a1
		push eax		// 0x000e76a2
		push 0		// 0x000e76a3
		push 0		// 0x000e76a5
		call nxPOWalk		// 0x000e76a7
		mov al, 1		// 0x000e76ac
		ret 8		// 0x000e76ae
		}
	}

// phys_fn_005315 (0x000e76c0, 391 B), 149 instructions
__declspec(naked) bool __fastcall nxPOBuild(void* tree, void* edxUnused, unsigned depth, const void* bounds)
	{
	__asm
		{
		sub esp, 0x18		// 0x000e76c0
		push ebx		// 0x000e76c3
		push ebp		// 0x000e76c4
		push esi		// 0x000e76c5
		mov esi, ecx		// 0x000e76c6
		push edi		// 0x000e76c8
		mov edi, dword ptr [esi + 0x4c]		// 0x000e76c9
		xor ebx, ebx		// 0x000e76cc
		cmp edi, ebx		// 0x000e76ce
		je Le76e5		// 0x000e76d0
		call nxPOCallAllocator		// 0x000e76d2
		mov edx, dword ptr [eax]		// 0x000e76d7
		add edi, -4		// 0x000e76d9
		push edi		// 0x000e76dc
		mov ecx, eax		// 0x000e76dd
		call dword ptr [edx + 0xc]		// 0x000e76df
		mov dword ptr [esi + 0x4c], ebx		// 0x000e76e2
Le76e5:
		mov ecx, dword ptr [esp + 0x2c]		// 0x000e76e5
		mov dword ptr [esi + 0x44], ecx		// 0x000e76e9
		mov eax, 2		// 0x000e76ec
		shl eax, cl		// 0x000e76f1
		mov ebp, dword ptr [esp + 0x30]		// 0x000e76f3
		mov dword ptr [esi + 0x48], ebx		// 0x000e76f7
		mov ecx, eax		// 0x000e76fa
		imul ecx, eax		// 0x000e76fc
		imul ecx, eax		// 0x000e76ff
		dec ecx		// 0x000e7702
		mov eax, 0x24924925		// 0x000e7703
		mul ecx		// 0x000e7708
		sub ecx, edx		// 0x000e770a
		shr ecx, 1		// 0x000e770c
		add ecx, edx		// 0x000e770e
		shr ecx, 2		// 0x000e7710
		mov edi, ecx		// 0x000e7713
		mov dword ptr [esi + 0x48], edi		// 0x000e7715
		mov eax, ebp		// 0x000e7718
		mov edx, dword ptr [eax]		// 0x000e771a
		mov ecx, esi		// 0x000e771c
		mov dword ptr [ecx], edx		// 0x000e771e
		mov edx, dword ptr [eax + 4]		// 0x000e7720
		mov dword ptr [ecx + 4], edx		// 0x000e7723
		mov edx, dword ptr [eax + 8]		// 0x000e7726
		mov dword ptr [ecx + 8], edx		// 0x000e7729
		mov edx, dword ptr [eax + 0xc]		// 0x000e772c
		mov dword ptr [ecx + 0xc], edx		// 0x000e772f
		mov edx, dword ptr [eax + 0x10]		// 0x000e7732
		mov dword ptr [ecx + 0x10], edx		// 0x000e7735
		mov eax, dword ptr [eax + 0x14]		// 0x000e7738
		mov dword ptr [ecx + 0x14], eax		// 0x000e773b
		call nxPOCallAllocator		// 0x000e773e
		mov edx, dword ptr [eax]		// 0x000e7743
		mov ecx, edi		// 0x000e7745
		shl ecx, 5		// 0x000e7747
		add ecx, 4		// 0x000e774a
		push ebx		// 0x000e774d
		push ecx		// 0x000e774e
		mov ecx, eax		// 0x000e774f
		call dword ptr [edx]		// 0x000e7751
		cmp eax, ebx		// 0x000e7753
		je Le7776		// 0x000e7755
		lea ecx, [edi - 1]		// 0x000e7757
		cmp ecx, ebx		// 0x000e775a
		mov dword ptr [eax], edi		// 0x000e775c
		lea edx, [eax + 4]		// 0x000e775e
		jl Le7772		// 0x000e7761
		lea eax, [edx + 0x1c]		// 0x000e7763
		inc ecx		// 0x000e7766
Le7767:
		mov dword ptr [eax - 4], ebx		// 0x000e7767
		mov dword ptr [eax], ebx		// 0x000e776a
		add eax, 0x20		// 0x000e776c
		dec ecx		// 0x000e776f
		jne Le7767		// 0x000e7770
Le7772:
		mov eax, edx		// 0x000e7772
		jmp Le7778		// 0x000e7774
Le7776:
		xor eax, eax		// 0x000e7776
Le7778:
		cmp eax, ebx		// 0x000e7778
		mov dword ptr [esi + 0x4c], eax		// 0x000e777a
		jne Le778b		// 0x000e777d
		pop edi		// 0x000e777f
		pop esi		// 0x000e7780
		pop ebp		// 0x000e7781
		xor al, al		// 0x000e7782
		pop ebx		// 0x000e7784
		add esp, 0x18		// 0x000e7785
		ret 8		// 0x000e7788
Le778b:
		lea edx, [esp + 0x10]		// 0x000e778b
		push edx		// 0x000e778f
		mov ecx, ebp		// 0x000e7790
		call nxPOCallMakeCube		// 0x000e7792
		fadd st(0), st(0)		// 0x000e7797
		mov edx, dword ptr [esp + 0x10]		// 0x000e7799
		lea eax, [esi + 0x18]		// 0x000e779d
		fst dword ptr [esi + 0x24]		// 0x000e77a0
		mov ecx, eax		// 0x000e77a3
		fld dword ptr [nxPOK101043cc_4]		// 0x000e77a5
		mov dword ptr [ecx], edx		// 0x000e77ab
		fmul st(0), st(1)		// 0x000e77ad
		mov edx, dword ptr [esp + 0x14]		// 0x000e77af
		mov dword ptr [ecx + 4], edx		// 0x000e77b3
		mov edx, dword ptr [esp + 0x18]		// 0x000e77b6
		fstp dword ptr [esi + 0x28]		// 0x000e77ba
		mov dword ptr [ecx + 8], edx		// 0x000e77bd
		fld dword ptr [nxPOK10113638_4]		// 0x000e77c0
		mov ecx, esi		// 0x000e77c6
		fmul st(0), st(1)		// 0x000e77c8
		fstp dword ptr [esi + 0x2c]		// 0x000e77ca
		fld dword ptr [nxPOK1011ba34_4]		// 0x000e77cd
		fmul st(0), st(1)		// 0x000e77d3
		fstp dword ptr [esi + 0x30]		// 0x000e77d5
		fld dword ptr [nxPOK1011ba30_4]		// 0x000e77d8
		fmul st(0), st(1)		// 0x000e77de
		fstp dword ptr [esi + 0x34]		// 0x000e77e0
		fld dword ptr [nxPOK1011ba2c_4]		// 0x000e77e3
		fmul st(0), st(1)		// 0x000e77e9
		fstp dword ptr [esi + 0x38]		// 0x000e77eb
		fld dword ptr [nxPOK1011ba28_4]		// 0x000e77ee
		fmul st(0), st(1)		// 0x000e77f4
		fstp dword ptr [esi + 0x3c]		// 0x000e77f6
		fmul dword ptr [nxPOK1011ba24_4]		// 0x000e77f9
		fstp dword ptr [esi + 0x40]		// 0x000e77ff
		fld dword ptr [eax]		// 0x000e7802
		fchs		// 0x000e7804
		fstp dword ptr [eax]		// 0x000e7806
		fld dword ptr [eax + 4]		// 0x000e7808
		fchs		// 0x000e780b
		fstp dword ptr [eax + 4]		// 0x000e780d
		fld dword ptr [eax + 8]		// 0x000e7810
		fchs		// 0x000e7813
		fstp dword ptr [eax + 8]		// 0x000e7815
		lea eax, [esp + 0x10]		// 0x000e7818
		push eax		// 0x000e781c
		push ebx		// 0x000e781d
		call nxPOSetNodeBounds		// 0x000e781e
		mov ecx, offset nxPOLeafCallback		// 0x000e7823
		test ecx, ecx		// 0x000e7828
		je Le783b		// 0x000e782a
		push ebx		// 0x000e782c
		push offset nxPOLeafCallback		// 0x000e782d
		push ebx		// 0x000e7832
		push ebx		// 0x000e7833
		mov ecx, esi		// 0x000e7834
		call nxPOWalk		// 0x000e7836
Le783b:
		pop edi		// 0x000e783b
		pop esi		// 0x000e783c
		pop ebp		// 0x000e783d
		mov al, 1		// 0x000e783e
		pop ebx		// 0x000e7840
		add esp, 0x18		// 0x000e7841
		ret 8		// 0x000e7844
		}
	}

// phys_fn_005317 (0x000e7850, 816 B), 287 instructions
__declspec(naked) bool __fastcall nxPOInsert(void* tree, void* edxUnused, void* object, const void* bounds)
	{
	__asm
		{
		mov eax, dword ptr [esp + 8]		// 0x000e7850
		sub esp, 0x4c		// 0x000e7854
		push ebx		// 0x000e7857
		push esi		// 0x000e7858
		xor esi, esi		// 0x000e7859
		cmp eax, esi		// 0x000e785b
		mov ebx, ecx		// 0x000e785d
		je Le78c7		// 0x000e785f
		fld dword ptr [eax + 0xc]		// 0x000e7861
		mov dword ptr [esp + 8], eax		// 0x000e7864
		fsub dword ptr [eax]		// 0x000e7868
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e786a
		fstp dword ptr [esp + 0x50]		// 0x000e7870
		fld dword ptr [eax + 0xc]		// 0x000e7874
		fadd dword ptr [eax]		// 0x000e7877
		fld dword ptr [eax + 0x10]		// 0x000e7879
		fadd dword ptr [eax + 4]		// 0x000e787c
		fld dword ptr [eax + 0x14]		// 0x000e787f
		fadd dword ptr [eax + 8]		// 0x000e7882
		fstp dword ptr [esp + 0x34]		// 0x000e7885
		fxch st(1)		// 0x000e7889
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e788b
		fstp dword ptr [esp + 0x20]		// 0x000e7891
		mov ecx, dword ptr [esp + 0x20]		// 0x000e7895
		mov dword ptr [esp + 0x44], ecx		// 0x000e7899
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e789d
		fstp dword ptr [esp + 0x24]		// 0x000e78a3
		mov edx, dword ptr [esp + 0x24]		// 0x000e78a7
		fld dword ptr [esp + 0x34]		// 0x000e78ab
		mov dword ptr [esp + 0x48], edx		// 0x000e78af
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e78b3
		fstp dword ptr [esp + 0x28]		// 0x000e78b9
		mov ecx, dword ptr [esp + 0x28]		// 0x000e78bd
		mov dword ptr [esp + 0x4c], ecx		// 0x000e78c1
		jmp Le7911		// 0x000e78c5
Le78c7:
		mov eax, dword ptr [nxPOAdapterNotify]		// 0x000e78c7
		cmp eax, esi		// 0x000e78cc
		je Le78e2		// 0x000e78ce
		mov ecx, dword ptr [esp + 0x58]		// 0x000e78d0
		lea edx, [esp + 0x44]		// 0x000e78d4
		push edx		// 0x000e78d8
		mov edx, dword ptr [ecx + 4]		// 0x000e78d9
		push edx		// 0x000e78dc
		call eax		// 0x000e78dd
		add esp, 8		// 0x000e78df
Le78e2:
		mov eax, dword ptr [esp + 0x50]		// 0x000e78e2
		mov ecx, eax		// 0x000e78e6
		mov edx, eax		// 0x000e78e8
		mov dword ptr [esp + 0x20], eax		// 0x000e78ea
		mov dword ptr [esp + 0x24], ecx		// 0x000e78ee
		lea eax, [esp + 0x20]		// 0x000e78f2
		push eax		// 0x000e78f6
		lea ecx, [esp + 0x48]		// 0x000e78f7
		push ecx		// 0x000e78fb
		lea ecx, [esp + 0x34]		// 0x000e78fc
		mov dword ptr [esp + 0x30], edx		// 0x000e7900
		call nxPOSetCenterExtents		// 0x000e7904
		lea edx, [esp + 0x2c]		// 0x000e7909
		mov dword ptr [esp + 8], edx		// 0x000e790d
Le7911:
		fld dword ptr [esp + 0x50]		// 0x000e7911
		fcomp dword ptr [ebx + 0x28]		// 0x000e7915
		fnstsw ax		// 0x000e7918
		test ah, 0x41		// 0x000e791a
		je Le7972		// 0x000e791d
		fld dword ptr [esp + 0x44]		// 0x000e791f
		fcomp dword ptr [ebx + 0xc]		// 0x000e7923
		fnstsw ax		// 0x000e7926
		test ah, 0x41		// 0x000e7928
		je Le7972		// 0x000e792b
		fld dword ptr [esp + 0x44]		// 0x000e792d
		fcomp dword ptr [ebx]		// 0x000e7931
		fnstsw ax		// 0x000e7933
		test ah, 5		// 0x000e7935
		jnp Le7972		// 0x000e7938
		fld dword ptr [esp + 0x48]		// 0x000e793a
		fcomp dword ptr [ebx + 0x10]		// 0x000e793e
		fnstsw ax		// 0x000e7941
		test ah, 0x41		// 0x000e7943
		je Le7972		// 0x000e7946
		fld dword ptr [esp + 0x48]		// 0x000e7948
		fcomp dword ptr [ebx + 4]		// 0x000e794c
		fnstsw ax		// 0x000e794f
		test ah, 5		// 0x000e7951
		jnp Le7972		// 0x000e7954
		fld dword ptr [esp + 0x4c]		// 0x000e7956
		fcomp dword ptr [ebx + 0x14]		// 0x000e795a
		fnstsw ax		// 0x000e795d
		test ah, 0x41		// 0x000e795f
		je Le7972		// 0x000e7962
		fld dword ptr [esp + 0x4c]		// 0x000e7964
		fcomp dword ptr [ebx + 8]		// 0x000e7968
		fnstsw ax		// 0x000e796b
		test ah, 5		// 0x000e796d
		jp Le797c		// 0x000e7970
Le7972:
		pop esi		// 0x000e7972
		xor al, al		// 0x000e7973
		pop ebx		// 0x000e7975
		add esp, 0x4c		// 0x000e7976
		ret 8		// 0x000e7979
Le797c:
		fld dword ptr [esp + 0x50]		// 0x000e797c
		push ebp		// 0x000e7980
		fadd st(0), st(0)		// 0x000e7981
		push edi		// 0x000e7983
		fdivr dword ptr [ebx + 0x24]		// 0x000e7984
		fldln2		// 0x000e7987
		fxch st(1)		// 0x000e7989
		fyl2x		// 0x000e798b
		fmul dword ptr [nxPOK1011ba20_4]		// 0x000e798d
		call nxPOFtol		// 0x000e7993
		mov edi, eax		// 0x000e7998
		mov eax, dword ptr [ebx + 0x44]		// 0x000e799a
		cmp edi, eax		// 0x000e799d
		mov dword ptr [esp + 0x14], edi		// 0x000e799f
		jbe Le79ab		// 0x000e79a3
		mov edi, eax		// 0x000e79a5
		mov dword ptr [esp + 0x14], edi		// 0x000e79a7
Le79ab:
		cmp edi, esi		// 0x000e79ab
		jne Le79b5		// 0x000e79ad
		mov dword ptr [esp + 0x18], esi		// 0x000e79af
		jmp Le79dc		// 0x000e79b3
Le79b5:
		lea ecx, [edi - 1]		// 0x000e79b5
		mov eax, 2		// 0x000e79b8
		shl eax, cl		// 0x000e79bd
		mov ecx, eax		// 0x000e79bf
		imul ecx, eax		// 0x000e79c1
		imul ecx, eax		// 0x000e79c4
		dec ecx		// 0x000e79c7
		mov eax, 0x24924925		// 0x000e79c8
		mul ecx		// 0x000e79cd
		sub ecx, edx		// 0x000e79cf
		shr ecx, 1		// 0x000e79d1
		add ecx, edx		// 0x000e79d3
		shr ecx, 2		// 0x000e79d5
		mov dword ptr [esp + 0x18], ecx		// 0x000e79d8
Le79dc:
		mov eax, 1		// 0x000e79dc
		mov ecx, edi		// 0x000e79e1
		shl eax, cl		// 0x000e79e3
		test eax, eax		// 0x000e79e5
		mov dword ptr [esp + 0x64], eax		// 0x000e79e7
		fild dword ptr [esp + 0x64]		// 0x000e79eb
		jge Le79f7		// 0x000e79ef
		fadd dword ptr [nxPOK101066f8_4]		// 0x000e79f1
Le79f7:
		fdiv dword ptr [ebx + 0x24]		// 0x000e79f7
		fld dword ptr [esp + 0x4c]		// 0x000e79fa
		fadd dword ptr [ebx + 0x18]		// 0x000e79fe
		fmul st(0), st(1)		// 0x000e7a01
		call nxPOFtol		// 0x000e7a03
		fld dword ptr [esp + 0x50]		// 0x000e7a08
		fadd dword ptr [ebx + 0x1c]		// 0x000e7a0c
		mov dword ptr [esp + 0x24], eax		// 0x000e7a0f
		fmul st(0), st(1)		// 0x000e7a13
		call nxPOFtol		// 0x000e7a15
		fld dword ptr [esp + 0x54]		// 0x000e7a1a
		fadd dword ptr [ebx + 0x20]		// 0x000e7a1e
		mov dword ptr [esp + 0x20], eax		// 0x000e7a21
		fmul st(0), st(1)		// 0x000e7a25
		call nxPOFtol		// 0x000e7a27
		fstp st(0)		// 0x000e7a2c
		cmp edi, esi		// 0x000e7a2e
		mov dword ptr [esp + 0x1c], eax		// 0x000e7a30
		mov dword ptr [esp + 0x64], esi		// 0x000e7a34
		jbe Le7a8f		// 0x000e7a38
		mov edi, 2		// 0x000e7a3a
		nop		// 0x000e7a3f
Le7a40:
		mov ecx, esi		// 0x000e7a40
		mov edx, 1		// 0x000e7a42
		shl edx, cl		// 0x000e7a47
		mov ecx, dword ptr [esp + 0x1c]		// 0x000e7a49
		mov eax, edx		// 0x000e7a4d
		and eax, ecx		// 0x000e7a4f
		mov ecx, esi		// 0x000e7a51
		shr eax, cl		// 0x000e7a53
		lea ecx, [edi - 2]		// 0x000e7a55
		mov ebp, edx		// 0x000e7a58
		shl eax, cl		// 0x000e7a5a
		and ebp, dword ptr [esp + 0x20]		// 0x000e7a5c
		mov ecx, esi		// 0x000e7a60
		shr ebp, cl		// 0x000e7a62
		lea ecx, [edi - 1]		// 0x000e7a64
		shl ebp, cl		// 0x000e7a67
		and edx, dword ptr [esp + 0x24]		// 0x000e7a69
		mov ecx, esi		// 0x000e7a6d
		shr edx, cl		// 0x000e7a6f
		mov ecx, edi		// 0x000e7a71
		or eax, ebp		// 0x000e7a73
		mov ebp, dword ptr [esp + 0x64]		// 0x000e7a75
		shl edx, cl		// 0x000e7a79
		add edi, 3		// 0x000e7a7b
		or eax, edx		// 0x000e7a7e
		or ebp, eax		// 0x000e7a80
		mov eax, dword ptr [esp + 0x14]		// 0x000e7a82
		inc esi		// 0x000e7a86
		cmp esi, eax		// 0x000e7a87
		mov dword ptr [esp + 0x64], ebp		// 0x000e7a89
		jb Le7a40		// 0x000e7a8d
Le7a8f:
		mov ecx, dword ptr [esp + 0x64]		// 0x000e7a8f
		mov edx, dword ptr [esp + 0x18]		// 0x000e7a93
		lea esi, [ecx + edx]		// 0x000e7a97
		mov edi, esi		// 0x000e7a9a
		lea eax, [esi*8 + 1]		// 0x000e7a9c
		shl edi, 8		// 0x000e7aa3
		mov ebp, 1		// 0x000e7aa6
		mov dword ptr [esp + 0x64], eax		// 0x000e7aab
		add edi, 0x20		// 0x000e7aaf
Le7ab2:
		mov ecx, dword ptr [esp + 0x64]		// 0x000e7ab2
		cmp ecx, dword ptr [ebx + 0x48]		// 0x000e7ab6
		jae Le7ace		// 0x000e7ab9
		mov edx, dword ptr [ebx + 0x4c]		// 0x000e7abb
		mov ecx, dword ptr [esp + 0x10]		// 0x000e7abe
		add edx, edi		// 0x000e7ac2
		push edx		// 0x000e7ac4
		call nxPOIsInside		// 0x000e7ac5
		test al, al		// 0x000e7aca
		jne Le7ae2		// 0x000e7acc
Le7ace:
		mov ecx, dword ptr [esp + 0x64]		// 0x000e7ace
		inc ebp		// 0x000e7ad2
		add edi, 0x20		// 0x000e7ad3
		inc ecx		// 0x000e7ad6
		cmp ebp, 8		// 0x000e7ad7
		mov dword ptr [esp + 0x64], ecx		// 0x000e7ada
		jbe Le7ab2		// 0x000e7ade
		jmp Le7ae6		// 0x000e7ae0
Le7ae2:
		mov esi, dword ptr [esp + 0x64]		// 0x000e7ae2
Le7ae6:
		xor ebp, ebp		// 0x000e7ae6
		cmp esi, ebp		// 0x000e7ae8
		mov edi, esi		// 0x000e7aea
		je Le7b0c		// 0x000e7aec
		mov edi, edi		// 0x000e7aee
Le7af0:
		mov ecx, dword ptr [ebx + 0x4c]		// 0x000e7af0
		mov eax, edi		// 0x000e7af3
		shl eax, 5		// 0x000e7af5
		add eax, ecx		// 0x000e7af8
		mov ecx, dword ptr [esp + 0x10]		// 0x000e7afa
		push eax		// 0x000e7afe
		call nxPOIsInside		// 0x000e7aff
		dec edi		// 0x000e7b04
		shr edi, 3		// 0x000e7b05
		cmp edi, ebp		// 0x000e7b08
		jne Le7af0		// 0x000e7b0a
Le7b0c:
		cmp esi, ebp		// 0x000e7b0c
		mov eax, esi		// 0x000e7b0e
		je Le7b2d		// 0x000e7b10
Le7b12:
		mov edx, dword ptr [ebx + 0x4c]		// 0x000e7b12
		mov ecx, eax		// 0x000e7b15
		shl ecx, 5		// 0x000e7b17
		mov edi, dword ptr [ecx + edx + 0x18]		// 0x000e7b1a
		lea ecx, [ecx + edx + 0x18]		// 0x000e7b1e
		inc edi		// 0x000e7b22
		dec eax		// 0x000e7b23
		shr eax, 3		// 0x000e7b24
		cmp eax, ebp		// 0x000e7b27
		mov dword ptr [ecx], edi		// 0x000e7b29
		jne Le7b12		// 0x000e7b2b
Le7b2d:
		mov eax, dword ptr [ebx + 0x4c]		// 0x000e7b2d
		inc dword ptr [eax + 0x18]		// 0x000e7b30
		mov ecx, dword ptr [ebx + 0x4c]		// 0x000e7b33
		shl esi, 5		// 0x000e7b36
		mov eax, dword ptr [esi + ecx + 0x1c]		// 0x000e7b39
		add esi, ecx		// 0x000e7b3d
		cmp eax, ebp		// 0x000e7b3f
		mov eax, dword ptr [esp + 0x60]		// 0x000e7b41
		jne Le7b5f		// 0x000e7b45
		mov dword ptr [esi + 0x1c], eax		// 0x000e7b47
		pop edi		// 0x000e7b4a
		mov dword ptr [eax + 8], ebp		// 0x000e7b4b
		mov dword ptr [eax + 0xc], ebp		// 0x000e7b4e
		pop ebp		// 0x000e7b51
		mov dword ptr [eax + 0x10], esi		// 0x000e7b52
		pop esi		// 0x000e7b55
		mov al, 1		// 0x000e7b56
		pop ebx		// 0x000e7b58
		add esp, 0x4c		// 0x000e7b59
		ret 8		// 0x000e7b5c
Le7b5f:
		mov dword ptr [eax + 0x10], esi		// 0x000e7b5f
		mov dword ptr [eax + 8], ebp		// 0x000e7b62
		mov ecx, dword ptr [esi + 0x1c]		// 0x000e7b65
		mov dword ptr [eax + 0xc], ecx		// 0x000e7b68
		mov edx, dword ptr [esi + 0x1c]		// 0x000e7b6b
		pop edi		// 0x000e7b6e
		mov dword ptr [edx + 8], eax		// 0x000e7b6f
		pop ebp		// 0x000e7b72
		mov dword ptr [esi + 0x1c], eax		// 0x000e7b73
		pop esi		// 0x000e7b76
		mov al, 1		// 0x000e7b77
		pop ebx		// 0x000e7b79
		add esp, 0x4c		// 0x000e7b7a
		ret 8		// 0x000e7b7d
		}
	}

// phys_fn_005318 (0x000e7b80, 136 B), 52 instructions
__declspec(naked) bool __fastcall nxPOUpdate(void* tree, void* edxUnused, void* object)
	{
	__asm
		{
		sub esp, 0x34		// 0x000e7b80
		push esi		// 0x000e7b83
		mov esi, dword ptr [esp + 0x3c]		// 0x000e7b84
		mov eax, dword ptr [esi + 0x10]		// 0x000e7b88
		test eax, eax		// 0x000e7b8b
		push edi		// 0x000e7b8d
		mov edi, ecx		// 0x000e7b8e
		je Le7be1		// 0x000e7b90
		mov eax, dword ptr [nxPOAdapterNotify]		// 0x000e7b92
		test eax, eax		// 0x000e7b97
		je Le7ba9		// 0x000e7b99
		mov edx, dword ptr [esi + 4]		// 0x000e7b9b
		lea ecx, [esp + 0x14]		// 0x000e7b9e
		push ecx		// 0x000e7ba2
		push edx		// 0x000e7ba3
		call eax		// 0x000e7ba4
		add esp, 8		// 0x000e7ba6
Le7ba9:
		mov eax, dword ptr [esp + 0x20]		// 0x000e7ba9
		mov ecx, eax		// 0x000e7bad
		mov edx, eax		// 0x000e7baf
		mov dword ptr [esp + 8], eax		// 0x000e7bb1
		mov dword ptr [esp + 0xc], ecx		// 0x000e7bb5
		lea eax, [esp + 8]		// 0x000e7bb9
		push eax		// 0x000e7bbd
		lea ecx, [esp + 0x18]		// 0x000e7bbe
		push ecx		// 0x000e7bc2
		lea ecx, [esp + 0x2c]		// 0x000e7bc3
		mov dword ptr [esp + 0x18], edx		// 0x000e7bc7
		call nxPOSetCenterExtents		// 0x000e7bcb
		mov edx, dword ptr [esi + 0x10]		// 0x000e7bd0
		push edx		// 0x000e7bd3
		lea ecx, [esp + 0x28]		// 0x000e7bd4
		call nxPOIsInside		// 0x000e7bd8
		test al, al		// 0x000e7bdd
		je Le7beb		// 0x000e7bdf
Le7be1:
		pop edi		// 0x000e7be1
		xor al, al		// 0x000e7be2
		pop esi		// 0x000e7be4
		add esp, 0x34		// 0x000e7be5
		ret 4		// 0x000e7be8
Le7beb:
		push esi		// 0x000e7beb
		mov ecx, edi		// 0x000e7bec
		call nxPORemove		// 0x000e7bee
		lea eax, [esp + 0x24]		// 0x000e7bf3
		push eax		// 0x000e7bf7
		push esi		// 0x000e7bf8
		mov ecx, edi		// 0x000e7bf9
		call nxPOInsert		// 0x000e7bfb
		pop edi		// 0x000e7c00
		pop esi		// 0x000e7c01
		add esp, 0x34		// 0x000e7c02
		ret 4		// 0x000e7c05
		}
	}

// phys_fn_005545 (0x000f1970, 178 B), 72 instructions
__declspec(naked) bool __cdecl nxPQSphereAABB(const float* center, float radius, const float* boxMin, const float* boxMax)
	{
	__asm
		{
		mov ecx, dword ptr [esp + 4]		// 0x000f1970
		fld dword ptr [nxPOK101041f0_4]		// 0x000f1974
		fld dword ptr [ecx]		// 0x000f197a
		mov edx, dword ptr [esp + 0xc]		// 0x000f197c
		fcomp dword ptr [edx]		// 0x000f1980
		push esi		// 0x000f1982
		mov esi, dword ptr [esp + 0x14]		// 0x000f1983
		fnstsw ax		// 0x000f1987
		test ah, 5		// 0x000f1989
		jp Lf1996		// 0x000f198c
		fstp st(0)		// 0x000f198e
		fld dword ptr [ecx]		// 0x000f1990
		fsub dword ptr [edx]		// 0x000f1992
		jmp Lf19a7		// 0x000f1994
Lf1996:
		fld dword ptr [ecx]		// 0x000f1996
		fcomp dword ptr [esi]		// 0x000f1998
		fnstsw ax		// 0x000f199a
		test ah, 0x41		// 0x000f199c
		jne Lf19ab		// 0x000f199f
		fstp st(0)		// 0x000f19a1
		fld dword ptr [ecx]		// 0x000f19a3
		fsub dword ptr [esi]		// 0x000f19a5
Lf19a7:
		fld st(0)		// 0x000f19a7
		fmulp st(1), st(0)		// 0x000f19a9
Lf19ab:
		fld dword ptr [ecx + 4]		// 0x000f19ab
		fcomp dword ptr [edx + 4]		// 0x000f19ae
		fld dword ptr [ecx + 4]		// 0x000f19b1
		fnstsw ax		// 0x000f19b4
		test ah, 5		// 0x000f19b6
		jp Lf19c0		// 0x000f19b9
		fsub dword ptr [edx + 4]		// 0x000f19bb
		jmp Lf19d0		// 0x000f19be
Lf19c0:
		fcomp dword ptr [esi + 4]		// 0x000f19c0
		fnstsw ax		// 0x000f19c3
		test ah, 0x41		// 0x000f19c5
		jne Lf19d8		// 0x000f19c8
		fld dword ptr [ecx + 4]		// 0x000f19ca
		fsub dword ptr [esi + 4]		// 0x000f19cd
Lf19d0:
		fld st(0)		// 0x000f19d0
		fmul st(0), st(1)		// 0x000f19d2
		faddp st(2), st(0)		// 0x000f19d4
		fstp st(0)		// 0x000f19d6
Lf19d8:
		fld dword ptr [ecx + 8]		// 0x000f19d8
		fcomp dword ptr [edx + 8]		// 0x000f19db
		fld dword ptr [ecx + 8]		// 0x000f19de
		fnstsw ax		// 0x000f19e1
		test ah, 5		// 0x000f19e3
		jp Lf19ed		// 0x000f19e6
		fsub dword ptr [edx + 8]		// 0x000f19e8
		jmp Lf19fd		// 0x000f19eb
Lf19ed:
		fcomp dword ptr [esi + 8]		// 0x000f19ed
		fnstsw ax		// 0x000f19f0
		test ah, 0x41		// 0x000f19f2
		jne Lf1a05		// 0x000f19f5
		fld dword ptr [ecx + 8]		// 0x000f19f7
		fsub dword ptr [esi + 8]		// 0x000f19fa
Lf19fd:
		fld st(0)		// 0x000f19fd
		fmul st(0), st(1)		// 0x000f19ff
		faddp st(2), st(0)		// 0x000f1a01
		fstp st(0)		// 0x000f1a03
Lf1a05:
		fld dword ptr [esp + 0xc]		// 0x000f1a05
		pop esi		// 0x000f1a09
		fmul dword ptr [esp + 8]		// 0x000f1a0a
		fxch st(1)		// 0x000f1a0e
		fcompp		// 0x000f1a10
		fnstsw ax		// 0x000f1a12
		test ah, 0x41		// 0x000f1a14
		jp Lf1a1f		// 0x000f1a17
		mov eax, 1		// 0x000f1a19
		ret		// 0x000f1a1e
Lf1a1f:
		xor eax, eax		// 0x000f1a1f
		ret		// 0x000f1a21
		}
	}

// phys_fn_005547 (0x000f1a30, 56 B), 23 instructions
__declspec(naked) void __fastcall nxPQAppendNode(void* node, void* edxUnused, void* container)
	{
	__asm
		{
		push edi		// 0x000f1a30
		mov edi, dword ptr [ecx + 0x1c]		// 0x000f1a31
		test edi, edi		// 0x000f1a34
		je Lf1a64		// 0x000f1a36
		push esi		// 0x000f1a38
		mov esi, dword ptr [esp + 0xc]		// 0x000f1a39
		lea ecx, [ecx]		// 0x000f1a3d
Lf1a40:
		mov eax, dword ptr [esi + 4]		// 0x000f1a40
		cmp eax, dword ptr [esi]		// 0x000f1a43
		jne Lf1a50		// 0x000f1a45
		push 1		// 0x000f1a47
		mov ecx, esi		// 0x000f1a49
		call nxPQCallContainerResize		// 0x000f1a4b
Lf1a50:
		mov ecx, dword ptr [esi + 4]		// 0x000f1a50
		mov edx, dword ptr [esi + 8]		// 0x000f1a53
		mov dword ptr [edx + ecx*4], edi		// 0x000f1a56
		inc dword ptr [esi + 4]		// 0x000f1a59
		mov edi, dword ptr [edi + 0xc]		// 0x000f1a5c
		test edi, edi		// 0x000f1a5f
		jne Lf1a40		// 0x000f1a61
		pop esi		// 0x000f1a63
Lf1a64:
		pop edi		// 0x000f1a64
		ret 4		// 0x000f1a65
		}
	}

// phys_fn_005549 (0x000f1a70, 80 B), 35 instructions
__declspec(naked) void __cdecl nxPQAppendSubtree(unsigned node, unsigned count, void* nodes, void* container)
	{
	__asm
		{
		mov eax, dword ptr [esp + 8]		// 0x000f1a70
		push edi		// 0x000f1a74
		mov edi, dword ptr [esp + 8]		// 0x000f1a75
		cmp edi, eax		// 0x000f1a79
		jae Lf1abe		// 0x000f1a7b
		mov ecx, edi		// 0x000f1a7d
		push ebp		// 0x000f1a7f
		mov ebp, dword ptr [esp + 0x14]		// 0x000f1a80
		shl ecx, 5		// 0x000f1a84
		mov eax, dword ptr [ecx + ebp + 0x18]		// 0x000f1a87
		add ecx, ebp		// 0x000f1a8b
		test eax, eax		// 0x000f1a8d
		je Lf1abd		// 0x000f1a8f
		push ebx		// 0x000f1a91
		mov ebx, dword ptr [esp + 0x1c]		// 0x000f1a92
		push esi		// 0x000f1a96
		push ebx		// 0x000f1a97
		call nxPQAppendNode		// 0x000f1a98
		mov esi, 1		// 0x000f1a9d
Lf1aa2:
		mov eax, dword ptr [esp + 0x18]		// 0x000f1aa2
		push ebx		// 0x000f1aa6
		push ebp		// 0x000f1aa7
		push eax		// 0x000f1aa8
		lea ecx, [esi + edi*8]		// 0x000f1aa9
		push ecx		// 0x000f1aac
		call nxPQAppendSubtree		// 0x000f1aad
		add esp, 0x10		// 0x000f1ab2
		inc esi		// 0x000f1ab5
		cmp esi, 8		// 0x000f1ab6
		jbe Lf1aa2		// 0x000f1ab9
		pop esi		// 0x000f1abb
		pop ebx		// 0x000f1abc
Lf1abd:
		pop ebp		// 0x000f1abd
Lf1abe:
		pop edi		// 0x000f1abe
		ret		// 0x000f1abf
		}
	}

// phys_fn_005550 (0x000f1ac0, 412 B), 148 instructions
__declspec(naked) void __cdecl nxPQPlaneWalk(unsigned node, unsigned mask, void* query)
	{
	__asm
		{
		sub esp, 0x20		// 0x000f1ac0
		mov ecx, dword ptr [esp + 0x24]		// 0x000f1ac3
		push edi		// 0x000f1ac7
		mov edi, dword ptr [esp + 0x30]		// 0x000f1ac8
		mov eax, dword ptr [edi]		// 0x000f1acc
		cmp ecx, eax		// 0x000f1ace
		mov dword ptr [esp + 8], eax		// 0x000f1ad0
		jae Lf1c57		// 0x000f1ad4
		mov eax, dword ptr [edi + 4]		// 0x000f1ada
		shl ecx, 5		// 0x000f1add
		mov edx, dword ptr [ecx + eax + 0x18]		// 0x000f1ae0
		add ecx, eax		// 0x000f1ae4
		test edx, edx		// 0x000f1ae6
		mov dword ptr [esp + 4], eax		// 0x000f1ae8
		je Lf1c57		// 0x000f1aec
		fld dword ptr [ecx + 0xc]		// 0x000f1af2
		push ebx		// 0x000f1af5
		fadd dword ptr [ecx]		// 0x000f1af6
		mov ebx, dword ptr [esp + 0x30]		// 0x000f1af8
		fld dword ptr [ecx + 0x10]		// 0x000f1afc
		push ebp		// 0x000f1aff
		fadd dword ptr [ecx + 4]		// 0x000f1b00
		xor ebp, ebp		// 0x000f1b03
		fld dword ptr [ecx + 0x14]		// 0x000f1b05
		push esi		// 0x000f1b08
		fadd dword ptr [ecx + 8]		// 0x000f1b09
		mov esi, dword ptr [edi + 0x10]		// 0x000f1b0c
		fstp dword ptr [esp + 0x2c]		// 0x000f1b0f
		fxch st(1)		// 0x000f1b13
		fmul dword ptr [nxPOK101043cc_4]		// 0x000f1b15
		fstp dword ptr [esp + 0x18]		// 0x000f1b1b
		mov edx, dword ptr [esp + 0x18]		// 0x000f1b1f
		mov dword ptr [esp + 0x24], edx		// 0x000f1b23
		fmul dword ptr [nxPOK101043cc_4]		// 0x000f1b27
		fstp dword ptr [esp + 0x1c]		// 0x000f1b2d
		mov edx, dword ptr [esp + 0x1c]		// 0x000f1b31
		fld dword ptr [esp + 0x2c]		// 0x000f1b35
		mov dword ptr [esp + 0x28], edx		// 0x000f1b39
		fmul dword ptr [nxPOK101043cc_4]		// 0x000f1b3d
		fstp dword ptr [esp + 0x20]		// 0x000f1b43
		mov edx, dword ptr [esp + 0x20]		// 0x000f1b47
		mov dword ptr [esp + 0x2c], edx		// 0x000f1b4b
		mov edx, dword ptr [ecx + 0xc]		// 0x000f1b4f
		mov dword ptr [esp + 0x18], edx		// 0x000f1b52
		fld dword ptr [esp + 0x18]		// 0x000f1b56
		fsub dword ptr [esp + 0x24]		// 0x000f1b5a
		mov edx, dword ptr [ecx + 0x10]		// 0x000f1b5e
		mov dword ptr [esp + 0x1c], edx		// 0x000f1b61
		mov edx, dword ptr [ecx + 0x14]		// 0x000f1b65
		fstp dword ptr [esp + 0x18]		// 0x000f1b68
		mov dword ptr [esp + 0x20], edx		// 0x000f1b6c
		fld dword ptr [esp + 0x1c]		// 0x000f1b70
		mov edx, 1		// 0x000f1b74
		cmp ebx, edx		// 0x000f1b79
		fsub dword ptr [esp + 0x28]		// 0x000f1b7b
		fstp dword ptr [esp + 0x1c]		// 0x000f1b7f
		fld dword ptr [esp + 0x20]		// 0x000f1b83
		fsub dword ptr [esp + 0x2c]		// 0x000f1b87
		fstp dword ptr [esp + 0x20]		// 0x000f1b8b
		jb Lf1c03		// 0x000f1b8f
Lf1b91:
		test ebx, edx		// 0x000f1b91
		je Lf1bf2		// 0x000f1b93
		fld dword ptr [esi + 4]		// 0x000f1b95
		fabs		// 0x000f1b98
		fmul dword ptr [esp + 0x1c]		// 0x000f1b9a
		fld dword ptr [esi + 8]		// 0x000f1b9e
		fabs		// 0x000f1ba1
		fxch st(1)		// 0x000f1ba3
		fxch st(1)		// 0x000f1ba5
		fmul dword ptr [esp + 0x20]		// 0x000f1ba7
		faddp st(1), st(0)		// 0x000f1bab
		fld dword ptr [esi]		// 0x000f1bad
		fabs		// 0x000f1baf
		fst dword ptr [esp + 0x3c]		// 0x000f1bb1
		fmul dword ptr [esp + 0x18]		// 0x000f1bb5
		faddp st(1), st(0)		// 0x000f1bb9
		fld dword ptr [esp + 0x28]		// 0x000f1bbb
		fmul dword ptr [esi + 4]		// 0x000f1bbf
		fld dword ptr [esp + 0x2c]		// 0x000f1bc2
		fmul dword ptr [esi + 8]		// 0x000f1bc6
		faddp st(1), st(0)		// 0x000f1bc9
		fld dword ptr [esp + 0x24]		// 0x000f1bcb
		fmul dword ptr [esi]		// 0x000f1bcf
		faddp st(1), st(0)		// 0x000f1bd1
		fadd dword ptr [esi + 0xc]		// 0x000f1bd3
		fld st(1)		// 0x000f1bd6
		fcomp st(1)		// 0x000f1bd8
		fnstsw ax		// 0x000f1bda
		test ah, 5		// 0x000f1bdc
		jnp Lf1c50		// 0x000f1bdf
		fxch st(1)		// 0x000f1be1
		fchs		// 0x000f1be3
		fcomp st(1)		// 0x000f1be5
		fnstsw ax		// 0x000f1be7
		fstp st(0)		// 0x000f1be9
		test ah, 5		// 0x000f1beb
		jp Lf1bf2		// 0x000f1bee
		or ebp, edx		// 0x000f1bf0
Lf1bf2:
		add edx, edx		// 0x000f1bf2
		add esi, 0x10		// 0x000f1bf4
		cmp edx, ebx		// 0x000f1bf7
		jbe Lf1b91		// 0x000f1bf9
		test ebp, ebp		// 0x000f1bfb
		jne Lf1c22		// 0x000f1bfd
		mov eax, dword ptr [esp + 0x10]		// 0x000f1bff
Lf1c03:
		mov ecx, dword ptr [edi + 0xc]		// 0x000f1c03
		mov edx, dword ptr [esp + 0x14]		// 0x000f1c06
		push ecx		// 0x000f1c0a
		push eax		// 0x000f1c0b
		mov eax, dword ptr [esp + 0x3c]		// 0x000f1c0c
		push edx		// 0x000f1c10
		push eax		// 0x000f1c11
		call nxPQAppendSubtree		// 0x000f1c12
		add esp, 0x10		// 0x000f1c17
		pop esi		// 0x000f1c1a
		pop ebp		// 0x000f1c1b
		pop ebx		// 0x000f1c1c
		pop edi		// 0x000f1c1d
		add esp, 0x20		// 0x000f1c1e
		ret		// 0x000f1c21
Lf1c22:
		mov edx, dword ptr [edi + 8]		// 0x000f1c22
		push edx		// 0x000f1c25
		call nxPQAppendNode		// 0x000f1c26
		mov esi, 1		// 0x000f1c2b
Lf1c30:
		mov eax, dword ptr [esp + 0x34]		// 0x000f1c30
		push edi		// 0x000f1c34
		lea ecx, [esi + eax*8]		// 0x000f1c35
		push ebp		// 0x000f1c38
		push ecx		// 0x000f1c39
		call nxPQPlaneWalk		// 0x000f1c3a
		add esp, 0xc		// 0x000f1c3f
		inc esi		// 0x000f1c42
		cmp esi, 8		// 0x000f1c43
		jbe Lf1c30		// 0x000f1c46
		pop esi		// 0x000f1c48
		pop ebp		// 0x000f1c49
		pop ebx		// 0x000f1c4a
		pop edi		// 0x000f1c4b
		add esp, 0x20		// 0x000f1c4c
		ret		// 0x000f1c4f
Lf1c50:
		pop esi		// 0x000f1c50
		fstp st(0)		// 0x000f1c51
		pop ebp		// 0x000f1c53
		fstp st(0)		// 0x000f1c54
		pop ebx		// 0x000f1c56
Lf1c57:
		pop edi		// 0x000f1c57
		add esp, 0x20		// 0x000f1c58
		ret		// 0x000f1c5b
		}
	}

// phys_fn_005552 (0x000f1c60, 70 B), 22 instructions
__declspec(naked) void __fastcall nxPQPlaneQuery(void* tree, void* edxUnused, const void* planes, unsigned nbPlanes, void* containerA, void* containerB)
	{
	__asm
		{
		sub esp, 0x14		// 0x000f1c60
		mov eax, dword ptr [ecx + 0x48]		// 0x000f1c63
		mov ecx, dword ptr [ecx + 0x4c]		// 0x000f1c66
		mov edx, dword ptr [esp + 0x20]		// 0x000f1c69
		mov dword ptr [esp], eax		// 0x000f1c6d
		mov eax, dword ptr [esp + 0x24]		// 0x000f1c70
		mov dword ptr [esp + 4], ecx		// 0x000f1c74
		mov ecx, dword ptr [esp + 0x18]		// 0x000f1c78
		mov dword ptr [esp + 0xc], eax		// 0x000f1c7c
		mov dword ptr [esp + 0x10], ecx		// 0x000f1c80
		mov ecx, dword ptr [esp + 0x1c]		// 0x000f1c84
		mov eax, 1		// 0x000f1c88
		shl eax, cl		// 0x000f1c8d
		mov dword ptr [esp + 8], edx		// 0x000f1c8f
		lea edx, [esp]		// 0x000f1c93
		push edx		// 0x000f1c96
		dec eax		// 0x000f1c97
		push eax		// 0x000f1c98
		push 0		// 0x000f1c99
		call nxPQPlaneWalk		// 0x000f1c9b
		add esp, 0x20		// 0x000f1ca0
		ret 0x10		// 0x000f1ca3
		}
	}

// phys_fn_005554 (0x000f1cb0, 93 B), 40 instructions
__declspec(naked) void __cdecl nxPQSegmentWalk(unsigned node, void* query)
	{
	__asm
		{
		push ebx		// 0x000f1cb0
		mov ebx, dword ptr [esp + 8]		// 0x000f1cb1
		push edi		// 0x000f1cb5
		mov edi, dword ptr [esp + 0x10]		// 0x000f1cb6
		cmp ebx, dword ptr [edi]		// 0x000f1cba
		jae Lf1d0a		// 0x000f1cbc
		mov ecx, dword ptr [edi + 4]		// 0x000f1cbe
		push esi		// 0x000f1cc1
		mov esi, ebx		// 0x000f1cc2
		shl esi, 5		// 0x000f1cc4
		mov eax, dword ptr [esi + ecx + 0x18]		// 0x000f1cc7
		add esi, ecx		// 0x000f1ccb
		test eax, eax		// 0x000f1ccd
		je Lf1d09		// 0x000f1ccf
		mov eax, dword ptr [edi + 0xc]		// 0x000f1cd1
		lea ecx, [esi + 0xc]		// 0x000f1cd4
		push ecx		// 0x000f1cd7
		push esi		// 0x000f1cd8
		push eax		// 0x000f1cd9
		call nxPQCallSegmentAABB		// 0x000f1cda
		add esp, 0xc		// 0x000f1cdf
		test al, al		// 0x000f1ce2
		je Lf1d09		// 0x000f1ce4
		mov edx, dword ptr [edi + 8]		// 0x000f1ce6
		push edx		// 0x000f1ce9
		mov ecx, esi		// 0x000f1cea
		call nxPQAppendNode		// 0x000f1cec
		mov esi, 1		// 0x000f1cf1
Lf1cf6:
		lea eax, [esi + ebx*8]		// 0x000f1cf6
		push edi		// 0x000f1cf9
		push eax		// 0x000f1cfa
		call nxPQSegmentWalk		// 0x000f1cfb
		add esp, 8		// 0x000f1d00
		inc esi		// 0x000f1d03
		cmp esi, 8		// 0x000f1d04
		jbe Lf1cf6		// 0x000f1d07
Lf1d09:
		pop esi		// 0x000f1d09
Lf1d0a:
		pop edi		// 0x000f1d0a
		pop ebx		// 0x000f1d0b
		ret		// 0x000f1d0c
		}
	}

// phys_fn_005556 (0x000f1d10, 49 B), 15 instructions
__declspec(naked) void __fastcall nxPQSegmentQuery(void* tree, void* edxUnused, const void* segment, void* container)
	{
	__asm
		{
		sub esp, 0x10		// 0x000f1d10
		mov eax, dword ptr [ecx + 0x48]		// 0x000f1d13
		mov ecx, dword ptr [ecx + 0x4c]		// 0x000f1d16
		mov edx, dword ptr [esp + 0x18]		// 0x000f1d19
		mov dword ptr [esp + 4], ecx		// 0x000f1d1d
		lea ecx, [esp]		// 0x000f1d21
		mov dword ptr [esp], eax		// 0x000f1d24
		mov eax, dword ptr [esp + 0x14]		// 0x000f1d27
		push ecx		// 0x000f1d2b
		push 0		// 0x000f1d2c
		mov dword ptr [esp + 0x10], edx		// 0x000f1d2e
		mov dword ptr [esp + 0x14], eax		// 0x000f1d32
		call nxPQSegmentWalk		// 0x000f1d36
		add esp, 0x18		// 0x000f1d3b
		ret 8		// 0x000f1d3e
		}
	}

// phys_fn_005558 (0x000f1d50, 93 B), 40 instructions
__declspec(naked) void __cdecl nxPQRayWalk(unsigned node, void* query)
	{
	__asm
		{
		push ebx		// 0x000f1d50
		mov ebx, dword ptr [esp + 8]		// 0x000f1d51
		push edi		// 0x000f1d55
		mov edi, dword ptr [esp + 0x10]		// 0x000f1d56
		cmp ebx, dword ptr [edi]		// 0x000f1d5a
		jae Lf1daa		// 0x000f1d5c
		mov ecx, dword ptr [edi + 4]		// 0x000f1d5e
		push esi		// 0x000f1d61
		mov esi, ebx		// 0x000f1d62
		shl esi, 5		// 0x000f1d64
		mov eax, dword ptr [esi + ecx + 0x18]		// 0x000f1d67
		add esi, ecx		// 0x000f1d6b
		test eax, eax		// 0x000f1d6d
		je Lf1da9		// 0x000f1d6f
		mov eax, dword ptr [edi + 0xc]		// 0x000f1d71
		lea ecx, [esi + 0xc]		// 0x000f1d74
		push ecx		// 0x000f1d77
		push esi		// 0x000f1d78
		push eax		// 0x000f1d79
		call nxPQCallRayAABB		// 0x000f1d7a
		add esp, 0xc		// 0x000f1d7f
		test al, al		// 0x000f1d82
		je Lf1da9		// 0x000f1d84
		mov edx, dword ptr [edi + 8]		// 0x000f1d86
		push edx		// 0x000f1d89
		mov ecx, esi		// 0x000f1d8a
		call nxPQAppendNode		// 0x000f1d8c
		mov esi, 1		// 0x000f1d91
Lf1d96:
		lea eax, [esi + ebx*8]		// 0x000f1d96
		push edi		// 0x000f1d99
		push eax		// 0x000f1d9a
		call nxPQRayWalk		// 0x000f1d9b
		add esp, 8		// 0x000f1da0
		inc esi		// 0x000f1da3
		cmp esi, 8		// 0x000f1da4
		jbe Lf1d96		// 0x000f1da7
Lf1da9:
		pop esi		// 0x000f1da9
Lf1daa:
		pop edi		// 0x000f1daa
		pop ebx		// 0x000f1dab
		ret		// 0x000f1dac
		}
	}

// phys_fn_005560 (0x000f1db0, 49 B), 15 instructions
__declspec(naked) void __fastcall nxPQRayQuery(void* tree, void* edxUnused, const void* ray, void* container)
	{
	__asm
		{
		sub esp, 0x10		// 0x000f1db0
		mov eax, dword ptr [ecx + 0x48]		// 0x000f1db3
		mov ecx, dword ptr [ecx + 0x4c]		// 0x000f1db6
		mov edx, dword ptr [esp + 0x18]		// 0x000f1db9
		mov dword ptr [esp + 4], ecx		// 0x000f1dbd
		lea ecx, [esp]		// 0x000f1dc1
		mov dword ptr [esp], eax		// 0x000f1dc4
		mov eax, dword ptr [esp + 0x14]		// 0x000f1dc7
		push ecx		// 0x000f1dcb
		push 0		// 0x000f1dcc
		mov dword ptr [esp + 0x10], edx		// 0x000f1dce
		mov dword ptr [esp + 0x14], eax		// 0x000f1dd2
		call nxPQRayWalk		// 0x000f1dd6
		add esp, 0x18		// 0x000f1ddb
		ret 8		// 0x000f1dde
		}
	}

// phys_fn_005562 (BoundedDynamicPruner::OverlapQuery recursion) (0x000f1df0, 605 B), 222 instructions
__declspec(naked) void __cdecl nxPQSphereWalk(unsigned node, void* query)
	{
	__asm
		{
		sub esp, 0x1c		// 0x000f1df0
		push ebp		// 0x000f1df3
		mov ebp, dword ptr [esp + 0x24]		// 0x000f1df4
		push edi		// 0x000f1df8
		mov edi, dword ptr [esp + 0x2c]		// 0x000f1df9
		cmp ebp, dword ptr [edi]		// 0x000f1dfd
		jae Lf2047		// 0x000f1dff
		mov ecx, dword ptr [edi + 4]		// 0x000f1e05
		push esi		// 0x000f1e08
		mov esi, ebp		// 0x000f1e09
		shl esi, 5		// 0x000f1e0b
		mov eax, dword ptr [esi + ecx + 0x18]		// 0x000f1e0e
		add esi, ecx		// 0x000f1e12
		test eax, eax		// 0x000f1e14
		je Lf2046		// 0x000f1e16
		mov eax, dword ptr [edi + 0x10]		// 0x000f1e1c
		mov ecx, dword ptr [eax + 0xc]		// 0x000f1e1f
		push ebx		// 0x000f1e22
		lea ebx, [esi + 0xc]		// 0x000f1e23
		push ebx		// 0x000f1e26
		push esi		// 0x000f1e27
		mov edx, ecx		// 0x000f1e28
		push edx		// 0x000f1e2a
		push eax		// 0x000f1e2b
		mov dword ptr [esp + 0x40], ecx		// 0x000f1e2c
		call nxPQSphereAABB		// 0x000f1e30
		add esp, 0x10		// 0x000f1e35
		test al, al		// 0x000f1e38
		je Lf2045		// 0x000f1e3a
		mov ecx, dword ptr [edi + 0x10]		// 0x000f1e40
		fld dword ptr [ecx + 0xc]		// 0x000f1e43
		mov eax, dword ptr [ebx]		// 0x000f1e46
		fld st(0)		// 0x000f1e48
		mov edx, dword ptr [ebx + 4]		// 0x000f1e4a
		fmul st(0), st(1)		// 0x000f1e4d
		mov dword ptr [esp + 0x14], eax		// 0x000f1e4f
		mov eax, dword ptr [ebx + 8]		// 0x000f1e53
		mov dword ptr [esp + 0x1c], eax		// 0x000f1e56
		fstp dword ptr [esp + 0x30]		// 0x000f1e5a
		mov dword ptr [esp + 0x18], edx		// 0x000f1e5e
		mov edx, esi		// 0x000f1e62
		fstp st(0)		// 0x000f1e64
		mov eax, dword ptr [edx]		// 0x000f1e66
		fld dword ptr [ecx + 8]		// 0x000f1e68
		mov dword ptr [esp + 0x20], eax		// 0x000f1e6b
		fsub dword ptr [esp + 0x1c]		// 0x000f1e6f
		mov eax, dword ptr [edx + 4]		// 0x000f1e73
		fld dword ptr [ecx + 4]		// 0x000f1e76
		mov edx, dword ptr [edx + 8]		// 0x000f1e79
		fsub dword ptr [esp + 0x18]		// 0x000f1e7c
		mov dword ptr [esp + 0x24], eax		// 0x000f1e80
		fld dword ptr [ecx]		// 0x000f1e84
		mov dword ptr [esp + 0x28], edx		// 0x000f1e86
		fsub dword ptr [esp + 0x14]		// 0x000f1e8a
		fld st(0)		// 0x000f1e8e
		fmul st(0), st(1)		// 0x000f1e90
		fstp dword ptr [esp + 0x34]		// 0x000f1e92
		fstp st(0)		// 0x000f1e96
		fld st(0)		// 0x000f1e98
		fmul st(0), st(1)		// 0x000f1e9a
		fld st(2)		// 0x000f1e9c
		fmul st(0), st(3)		// 0x000f1e9e
		faddp st(1), st(0)		// 0x000f1ea0
		fadd dword ptr [esp + 0x34]		// 0x000f1ea2
		fcomp dword ptr [esp + 0x30]		// 0x000f1ea6
		fnstsw ax		// 0x000f1eaa
		fstp st(0)		// 0x000f1eac
		test ah, 1		// 0x000f1eae
		fstp st(0)		// 0x000f1eb1
		je Lf2022		// 0x000f1eb3
		fld dword ptr [ecx + 8]		// 0x000f1eb9
		fsub dword ptr [esp + 0x1c]		// 0x000f1ebc
		fld dword ptr [ecx + 4]		// 0x000f1ec0
		fsub dword ptr [esp + 0x18]		// 0x000f1ec3
		fld dword ptr [ecx]		// 0x000f1ec7
		fsub dword ptr [esp + 0x20]		// 0x000f1ec9
		fld st(0)		// 0x000f1ecd
		fmul st(0), st(1)		// 0x000f1ecf
		fstp dword ptr [esp + 0x10]		// 0x000f1ed1
		fstp st(0)		// 0x000f1ed5
		fld st(0)		// 0x000f1ed7
		fmul st(0), st(1)		// 0x000f1ed9
		fld st(2)		// 0x000f1edb
		fmul st(0), st(3)		// 0x000f1edd
		faddp st(1), st(0)		// 0x000f1edf
		fadd dword ptr [esp + 0x10]		// 0x000f1ee1
		fcomp dword ptr [esp + 0x30]		// 0x000f1ee5
		fnstsw ax		// 0x000f1ee9
		fstp st(0)		// 0x000f1eeb
		test ah, 1		// 0x000f1eed
		fstp st(0)		// 0x000f1ef0
		je Lf2022		// 0x000f1ef2
		fld dword ptr [ecx + 8]		// 0x000f1ef8
		fsub dword ptr [esp + 0x1c]		// 0x000f1efb
		fld dword ptr [ecx + 4]		// 0x000f1eff
		fsub dword ptr [esp + 0x24]		// 0x000f1f02
		fld st(1)		// 0x000f1f06
		fmul st(0), st(2)		// 0x000f1f08
		fld st(1)		// 0x000f1f0a
		fmul st(0), st(2)		// 0x000f1f0c
		faddp st(1), st(0)		// 0x000f1f0e
		fadd dword ptr [esp + 0x34]		// 0x000f1f10
		fcomp dword ptr [esp + 0x30]		// 0x000f1f14
		fnstsw ax		// 0x000f1f18
		fstp st(0)		// 0x000f1f1a
		test ah, 1		// 0x000f1f1c
		fstp st(0)		// 0x000f1f1f
		je Lf2022		// 0x000f1f21
		fld dword ptr [ecx + 8]		// 0x000f1f27
		fsub dword ptr [esp + 0x1c]		// 0x000f1f2a
		fld dword ptr [ecx + 4]		// 0x000f1f2e
		fsub dword ptr [esp + 0x24]		// 0x000f1f31
		fld st(0)		// 0x000f1f35
		fmul st(0), st(1)		// 0x000f1f37
		fld st(2)		// 0x000f1f39
		fmul st(0), st(3)		// 0x000f1f3b
		faddp st(1), st(0)		// 0x000f1f3d
		fadd dword ptr [esp + 0x10]		// 0x000f1f3f
		fcomp dword ptr [esp + 0x30]		// 0x000f1f43
		fnstsw ax		// 0x000f1f47
		fstp st(0)		// 0x000f1f49
		test ah, 1		// 0x000f1f4b
		fstp st(0)		// 0x000f1f4e
		je Lf2022		// 0x000f1f50
		fld dword ptr [ecx + 8]		// 0x000f1f56
		fsub dword ptr [esp + 0x28]		// 0x000f1f59
		fld dword ptr [ecx + 4]		// 0x000f1f5d
		fsub dword ptr [esp + 0x18]		// 0x000f1f60
		fld st(0)		// 0x000f1f64
		fmul st(0), st(1)		// 0x000f1f66
		fld st(2)		// 0x000f1f68
		fmul st(0), st(3)		// 0x000f1f6a
		faddp st(1), st(0)		// 0x000f1f6c
		fadd dword ptr [esp + 0x34]		// 0x000f1f6e
		fcomp dword ptr [esp + 0x30]		// 0x000f1f72
		fnstsw ax		// 0x000f1f76
		fstp st(0)		// 0x000f1f78
		test ah, 1		// 0x000f1f7a
		fstp st(0)		// 0x000f1f7d
		je Lf2022		// 0x000f1f7f
		fld dword ptr [ecx + 8]		// 0x000f1f85
		fsub dword ptr [esp + 0x28]		// 0x000f1f88
		fld dword ptr [ecx + 4]		// 0x000f1f8c
		fsub dword ptr [esp + 0x18]		// 0x000f1f8f
		fld st(0)		// 0x000f1f93
		fmul st(0), st(1)		// 0x000f1f95
		fld st(2)		// 0x000f1f97
		fmul st(0), st(3)		// 0x000f1f99
		faddp st(1), st(0)		// 0x000f1f9b
		fadd dword ptr [esp + 0x10]		// 0x000f1f9d
		fcomp dword ptr [esp + 0x30]		// 0x000f1fa1
		fnstsw ax		// 0x000f1fa5
		fstp st(0)		// 0x000f1fa7
		test ah, 1		// 0x000f1fa9
		fstp st(0)		// 0x000f1fac
		je Lf2022		// 0x000f1fae
		fld dword ptr [ecx + 8]		// 0x000f1fb0
		fsub dword ptr [esp + 0x28]		// 0x000f1fb3
		fld dword ptr [ecx + 4]		// 0x000f1fb7
		fsub dword ptr [esp + 0x24]		// 0x000f1fba
		fld st(0)		// 0x000f1fbe
		fmul st(0), st(1)		// 0x000f1fc0
		fld st(2)		// 0x000f1fc2
		fmul st(0), st(3)		// 0x000f1fc4
		faddp st(1), st(0)		// 0x000f1fc6
		fadd dword ptr [esp + 0x34]		// 0x000f1fc8
		fcomp dword ptr [esp + 0x30]		// 0x000f1fcc
		fnstsw ax		// 0x000f1fd0
		fstp st(0)		// 0x000f1fd2
		test ah, 1		// 0x000f1fd4
		fstp st(0)		// 0x000f1fd7
		je Lf2022		// 0x000f1fd9
		fld dword ptr [ecx + 8]		// 0x000f1fdb
		fsub dword ptr [esp + 0x28]		// 0x000f1fde
		fld dword ptr [ecx + 4]		// 0x000f1fe2
		fsub dword ptr [esp + 0x24]		// 0x000f1fe5
		fld st(0)		// 0x000f1fe9
		fmul st(0), st(1)		// 0x000f1feb
		fld st(2)		// 0x000f1fed
		fmul st(0), st(3)		// 0x000f1fef
		faddp st(1), st(0)		// 0x000f1ff1
		fadd dword ptr [esp + 0x10]		// 0x000f1ff3
		fcomp dword ptr [esp + 0x30]		// 0x000f1ff7
		fnstsw ax		// 0x000f1ffb
		fstp st(0)		// 0x000f1ffd
		test ah, 1		// 0x000f1fff
		fstp st(0)		// 0x000f2002
		je Lf2022		// 0x000f2004
		mov eax, dword ptr [edi + 0xc]		// 0x000f2006
		mov ecx, dword ptr [edi + 4]		// 0x000f2009
		mov edx, dword ptr [edi]		// 0x000f200c
		push eax		// 0x000f200e
		push ecx		// 0x000f200f
		push edx		// 0x000f2010
		push ebp		// 0x000f2011
		call nxPQAppendSubtree		// 0x000f2012
		add esp, 0x10		// 0x000f2017
		pop ebx		// 0x000f201a
		pop esi		// 0x000f201b
		pop edi		// 0x000f201c
		pop ebp		// 0x000f201d
		add esp, 0x1c		// 0x000f201e
		ret		// 0x000f2021
Lf2022:
		mov eax, dword ptr [edi + 8]		// 0x000f2022
		push eax		// 0x000f2025
		mov ecx, esi		// 0x000f2026
		call nxPQAppendNode		// 0x000f2028
		mov esi, 1		// 0x000f202d
Lf2032:
		lea ecx, [esi + ebp*8]		// 0x000f2032
		push edi		// 0x000f2035
		push ecx		// 0x000f2036
		call nxPQSphereWalk		// 0x000f2037
		add esp, 8		// 0x000f203c
		inc esi		// 0x000f203f
		cmp esi, 8		// 0x000f2040
		jbe Lf2032		// 0x000f2043
Lf2045:
		pop ebx		// 0x000f2045
Lf2046:
		pop esi		// 0x000f2046
Lf2047:
		pop edi		// 0x000f2047
		pop ebp		// 0x000f2048
		add esp, 0x1c		// 0x000f2049
		ret		// 0x000f204c
		}
	}

// phys_fn_005564 (0x000f2050, 57 B), 17 instructions
__declspec(naked) void __fastcall nxPQSphereQuery(void* tree, void* edxUnused, const void* sphere, void* containerA, void* containerB)
	{
	__asm
		{
		sub esp, 0x14		// 0x000f2050
		mov eax, dword ptr [ecx + 0x48]		// 0x000f2053
		mov edx, dword ptr [esp + 0x1c]		// 0x000f2056
		mov ecx, dword ptr [ecx + 0x4c]		// 0x000f205a
		mov dword ptr [esp + 8], edx		// 0x000f205d
		lea edx, [esp]		// 0x000f2061
		mov dword ptr [esp], eax		// 0x000f2064
		mov eax, dword ptr [esp + 0x20]		// 0x000f2067
		mov dword ptr [esp + 4], ecx		// 0x000f206b
		mov ecx, dword ptr [esp + 0x18]		// 0x000f206f
		push edx		// 0x000f2073
		push 0		// 0x000f2074
		mov dword ptr [esp + 0x14], eax		// 0x000f2076
		mov dword ptr [esp + 0x18], ecx		// 0x000f207a
		call nxPQSphereWalk		// 0x000f207e
		add esp, 0x1c		// 0x000f2083
		ret 0xc		// 0x000f2086
		}
	}

// phys_fn_002148 (0x00053810, 98 B), 23 instructions
__declspec(naked) void __fastcall nxPCBoxEmpty(void* aabb)
	{
	__asm
		{
		sub esp, 0x18		// 0x00053810
		mov eax, ecx		// 0x00053813
		mov dword ptr [esp], 0x7f7fffff		// 0x00053815
		mov edx, dword ptr [esp]		// 0x0005381c
		mov dword ptr [eax], edx		// 0x0005381f
		mov dword ptr [esp + 4], 0x7f7fffff		// 0x00053821
		mov edx, dword ptr [esp + 4]		// 0x00053829
		mov dword ptr [eax + 4], edx		// 0x0005382d
		mov dword ptr [esp + 8], 0x7f7fffff		// 0x00053830
		mov edx, dword ptr [esp + 8]		// 0x00053838
		mov dword ptr [eax + 8], edx		// 0x0005383c
		mov dword ptr [esp + 0xc], 0xff7fffff		// 0x0005383f
		mov eax, dword ptr [esp + 0xc]		// 0x00053847
		add ecx, 0xc		// 0x0005384b
		mov dword ptr [ecx], eax		// 0x0005384e
		mov dword ptr [esp + 0x10], 0xff7fffff		// 0x00053850
		mov edx, dword ptr [esp + 0x10]		// 0x00053858
		mov dword ptr [esp + 0x14], 0xff7fffff		// 0x0005385c
		mov eax, dword ptr [esp + 0x14]		// 0x00053864
		mov dword ptr [ecx + 4], edx		// 0x00053868
		mov dword ptr [ecx + 8], eax		// 0x0005386b
		add esp, 0x18		// 0x0005386e
		ret		// 0x00053871
		}
	}

// phys_fn_005301 (0x000e7360, 26 B), 10 instructions
__declspec(naked) void* __fastcall nxPCOctreeCtor(void* tree)
	{
	__asm
		{
		mov eax, ecx		// 0x000e7360
		xor ecx, ecx		// 0x000e7362
		mov dword ptr [eax + 0x20], ecx		// 0x000e7364
		mov dword ptr [eax + 0x1c], ecx		// 0x000e7367
		mov dword ptr [eax + 0x18], ecx		// 0x000e736a
		mov dword ptr [eax + 0x24], ecx		// 0x000e736d
		mov dword ptr [eax + 0x44], ecx		// 0x000e7370
		mov dword ptr [eax + 0x48], ecx		// 0x000e7373
		mov dword ptr [eax + 0x4c], ecx		// 0x000e7376
		ret		// 0x000e7379
		}
	}

// phys_fn_005320 (0x000e7c10, 44 B), 17 instructions
__declspec(naked) void __fastcall nxPCOctreeRelease(void* tree)
	{
	__asm
		{
		push esi		// 0x000e7c10
		mov esi, ecx		// 0x000e7c11
		push edi		// 0x000e7c13
		mov edi, dword ptr [esi + 0x4c]		// 0x000e7c14
		test edi, edi		// 0x000e7c17
		je Le7c32		// 0x000e7c19
		call nxPOCallAllocator		// 0x000e7c1b
		mov edx, dword ptr [eax]		// 0x000e7c20
		add edi, -4		// 0x000e7c22
		push edi		// 0x000e7c25
		mov ecx, eax		// 0x000e7c26
		call dword ptr [edx + 0xc]		// 0x000e7c28
		mov dword ptr [esi + 0x4c], 0		// 0x000e7c2b
Le7c32:
		pop edi		// 0x000e7c32
		mov dword ptr [esi + 0x48], 0		// 0x000e7c33
		pop esi		// 0x000e7c3a
		ret		// 0x000e7c3b
		}
	}

// phys_fn_005223 (0x000e52d0, 270 B), 110 instructions
__declspec(naked) void __cdecl nxPCCollectCallback(void* a, void* b, void* c, void* d)
	{
	__asm
		{
		push ecx		// 0x000e52d0
		push ebx		// 0x000e52d1
		push ebp		// 0x000e52d2
		mov ebp, dword ptr [esp + 0x1c]		// 0x000e52d3
		mov eax, dword ptr [ebp + 4]		// 0x000e52d7
		mov dword ptr [esp + 8], eax		// 0x000e52da
		mov eax, dword ptr [esp + 0x18]		// 0x000e52de
		test eax, eax		// 0x000e52e2
		push esi		// 0x000e52e4
		mov esi, dword ptr [ebp]		// 0x000e52e5
		push edi		// 0x000e52e8
		je Le534f		// 0x000e52e9
		mov eax, dword ptr [esp + 0x18]		// 0x000e52eb
		test eax, eax		// 0x000e52ef
		je Le53d8		// 0x000e52f1
		mov ebx, dword ptr [esp + 0x1c]		// 0x000e52f7
		mov dword ptr [esp + 0x20], eax		// 0x000e52fb
		nop		// 0x000e52ff
Le5300:
		mov ecx, dword ptr [ebx]		// 0x000e5300
		mov eax, dword ptr [ebp + 8]		// 0x000e5302
		mov edx, dword ptr [esp + 0x10]		// 0x000e5305
		mov edi, dword ptr [edx + ecx*4]		// 0x000e5309
		add ebx, 4		// 0x000e530c
		test eax, eax		// 0x000e530f
		jne Le5319		// 0x000e5311
		test byte ptr [edi + 8], 1		// 0x000e5313
		jne Le5343		// 0x000e5317
Le5319:
		mov al, byte ptr [ebp + 0x18]		// 0x000e5319
		mov ecx, dword ptr [esi + 4]		// 0x000e531c
		mov byte ptr [esp + 0x24], al		// 0x000e531f
		cmp ecx, dword ptr [esi]		// 0x000e5323
		jne Le5330		// 0x000e5325
		push 1		// 0x000e5327
		mov ecx, esi		// 0x000e5329
		call nxPQCallContainerResize		// 0x000e532b
Le5330:
		movzx edx, byte ptr [esp + 0x24]		// 0x000e5330
		mov eax, dword ptr [esi + 4]		// 0x000e5335
		mov ecx, dword ptr [esi + 8]		// 0x000e5338
		or edx, edi		// 0x000e533b
		mov dword ptr [ecx + eax*4], edx		// 0x000e533d
		inc dword ptr [esi + 4]		// 0x000e5340
Le5343:
		dec dword ptr [esp + 0x20]		// 0x000e5343
		jne Le5300		// 0x000e5347
		pop edi		// 0x000e5349
		pop esi		// 0x000e534a
		pop ebp		// 0x000e534b
		pop ebx		// 0x000e534c
		pop ecx		// 0x000e534d
		ret		// 0x000e534e
Le534f:
		mov eax, dword ptr [ebp + 8]		// 0x000e534f
		test eax, eax		// 0x000e5352
		je Le5397		// 0x000e5354
		mov ebx, dword ptr [esp + 0x18]		// 0x000e5356
		test ebx, ebx		// 0x000e535a
		je Le53d8		// 0x000e535c
		mov edi, dword ptr [esp + 0x1c]		// 0x000e535e
Le5362:
		mov eax, dword ptr [esp + 0x10]		// 0x000e5362
		mov edx, dword ptr [edi]		// 0x000e5366
		mov ebp, dword ptr [eax + edx*4]		// 0x000e5368
		mov ecx, dword ptr [esi + 4]		// 0x000e536b
		cmp ecx, dword ptr [esi]		// 0x000e536e
		jne Le537b		// 0x000e5370
		push 1		// 0x000e5372
		mov ecx, esi		// 0x000e5374
		call nxPQCallContainerResize		// 0x000e5376
Le537b:
		mov edx, dword ptr [esi + 4]		// 0x000e537b
		mov eax, dword ptr [esi + 8]		// 0x000e537e
		mov dword ptr [eax + edx*4], ebp		// 0x000e5381
		mov edx, dword ptr [esi + 4]		// 0x000e5384
		inc edx		// 0x000e5387
		add edi, 4		// 0x000e5388
		dec ebx		// 0x000e538b
		mov dword ptr [esi + 4], edx		// 0x000e538c
		jne Le5362		// 0x000e538f
		pop edi		// 0x000e5391
		pop esi		// 0x000e5392
		pop ebp		// 0x000e5393
		pop ebx		// 0x000e5394
		pop ecx		// 0x000e5395
		ret		// 0x000e5396
Le5397:
		mov eax, dword ptr [esp + 0x18]		// 0x000e5397
		test eax, eax		// 0x000e539b
		je Le53d8		// 0x000e539d
		mov ebp, dword ptr [esp + 0x1c]		// 0x000e539f
		mov ebx, eax		// 0x000e53a3
Le53a5:
		mov ecx, dword ptr [ebp]		// 0x000e53a5
		mov edx, dword ptr [esp + 0x10]		// 0x000e53a8
		mov edi, dword ptr [edx + ecx*4]		// 0x000e53ac
		mov al, byte ptr [edi + 8]		// 0x000e53af
		add ebp, 4		// 0x000e53b2
		test al, 1		// 0x000e53b5
		jne Le53d5		// 0x000e53b7
		mov eax, dword ptr [esi + 4]		// 0x000e53b9
		cmp eax, dword ptr [esi]		// 0x000e53bc
		jne Le53c9		// 0x000e53be
		push 1		// 0x000e53c0
		mov ecx, esi		// 0x000e53c2
		call nxPQCallContainerResize		// 0x000e53c4
Le53c9:
		mov ecx, dword ptr [esi + 4]		// 0x000e53c9
		mov edx, dword ptr [esi + 8]		// 0x000e53cc
		mov dword ptr [edx + ecx*4], edi		// 0x000e53cf
		inc dword ptr [esi + 4]		// 0x000e53d2
Le53d5:
		dec ebx		// 0x000e53d5
		jne Le53a5		// 0x000e53d6
Le53d8:
		pop edi		// 0x000e53d8
		pop esi		// 0x000e53d9
		pop ebp		// 0x000e53da
		pop ebx		// 0x000e53db
		pop ecx		// 0x000e53dc
		ret		// 0x000e53dd
		}
	}

// phys_fn_005527 (0x000f1350, 418 B), 149 instructions
__declspec(naked) void __fastcall nxPCTreeCull(const void* node, void* edxUnused, const void* planes, unsigned mask, void* callback, void* user)
	{
	__asm
		{
		sub esp, 0x30		// 0x000f1350
		push ebx		// 0x000f1353
		push ebp		// 0x000f1354
		mov ebp, dword ptr [esp + 0x48]		// 0x000f1355
		push esi		// 0x000f1359
		push edi		// 0x000f135a
		mov edi, dword ptr [esp + 0x48]		// 0x000f135b
		mov esi, ecx		// 0x000f135f
Lf1361:
		fld dword ptr [esi + 0xc]		// 0x000f1361
		xor ebx, ebx		// 0x000f1364
		fadd dword ptr [esi]		// 0x000f1366
		fld dword ptr [esi + 0x10]		// 0x000f1368
		fadd dword ptr [esi + 4]		// 0x000f136b
		fld dword ptr [esi + 0x14]		// 0x000f136e
		fadd dword ptr [esi + 8]		// 0x000f1371
		fstp dword ptr [esp + 0x3c]		// 0x000f1374
		fxch st(1)		// 0x000f1378
		fmul dword ptr [nxPOK101043cc_4]		// 0x000f137a
		fstp dword ptr [esp + 0x28]		// 0x000f1380
		mov eax, dword ptr [esp + 0x28]		// 0x000f1384
		mov dword ptr [esp + 0x1c], eax		// 0x000f1388
		fmul dword ptr [nxPOK101043cc_4]		// 0x000f138c
		mov eax, dword ptr [esi + 0xc]		// 0x000f1392
		mov dword ptr [esp + 0x10], eax		// 0x000f1395
		fstp dword ptr [esp + 0x2c]		// 0x000f1399
		mov ecx, dword ptr [esp + 0x2c]		// 0x000f139d
		fld dword ptr [esp + 0x3c]		// 0x000f13a1
		mov dword ptr [esp + 0x20], ecx		// 0x000f13a5
		fmul dword ptr [nxPOK101043cc_4]		// 0x000f13a9
		mov ecx, dword ptr [esi + 0x10]		// 0x000f13af
		mov dword ptr [esp + 0x14], ecx		// 0x000f13b2
		mov ecx, 1		// 0x000f13b6
		cmp edi, ecx		// 0x000f13bb
		fstp dword ptr [esp + 0x30]		// 0x000f13bd
		fld dword ptr [esp + 0x10]		// 0x000f13c1
		mov edx, dword ptr [esp + 0x30]		// 0x000f13c5
		fsub dword ptr [esp + 0x1c]		// 0x000f13c9
		mov dword ptr [esp + 0x24], edx		// 0x000f13cd
		mov edx, dword ptr [esi + 0x14]		// 0x000f13d1
		mov dword ptr [esp + 0x18], edx		// 0x000f13d4
		fstp dword ptr [esp + 0x10]		// 0x000f13d8
		fld dword ptr [esp + 0x14]		// 0x000f13dc
		fsub dword ptr [esp + 0x20]		// 0x000f13e0
		fstp dword ptr [esp + 0x14]		// 0x000f13e4
		fld dword ptr [esp + 0x18]		// 0x000f13e8
		fsub dword ptr [esp + 0x24]		// 0x000f13ec
		fstp dword ptr [esp + 0x18]		// 0x000f13f0
		jb Lf14ad		// 0x000f13f4
		mov edx, dword ptr [esp + 0x44]		// 0x000f13fa
		add edx, 4		// 0x000f13fe
Lf1401:
		test edi, ecx		// 0x000f1401
		je Lf1466		// 0x000f1403
		fld dword ptr [edx + 4]		// 0x000f1405
		fabs		// 0x000f1408
		fmul dword ptr [esp + 0x18]		// 0x000f140a
		fld dword ptr [edx]		// 0x000f140e
		fabs		// 0x000f1410
		fxch st(1)		// 0x000f1412
		fxch st(1)		// 0x000f1414
		fmul dword ptr [esp + 0x14]		// 0x000f1416
		faddp st(1), st(0)		// 0x000f141a
		fld dword ptr [edx - 4]		// 0x000f141c
		fabs		// 0x000f141f
		fst dword ptr [esp + 0x50]		// 0x000f1421
		fmul dword ptr [esp + 0x10]		// 0x000f1425
		faddp st(1), st(0)		// 0x000f1429
		fld dword ptr [esp + 0x24]		// 0x000f142b
		fmul dword ptr [edx + 4]		// 0x000f142f
		fld dword ptr [esp + 0x1c]		// 0x000f1432
		fmul dword ptr [edx - 4]		// 0x000f1436
		faddp st(1), st(0)		// 0x000f1439
		fld dword ptr [esp + 0x20]		// 0x000f143b
		fmul dword ptr [edx]		// 0x000f143f
		faddp st(1), st(0)		// 0x000f1441
		fadd dword ptr [edx + 8]		// 0x000f1443
		fld st(1)		// 0x000f1446
		fcomp st(1)		// 0x000f1448
		fnstsw ax		// 0x000f144a
		test ah, 5		// 0x000f144c
		jnp Lf14e4		// 0x000f144f
		fxch st(1)		// 0x000f1455
		fchs		// 0x000f1457
		fcomp st(1)		// 0x000f1459
		fnstsw ax		// 0x000f145b
		fstp st(0)		// 0x000f145d
		test ah, 5		// 0x000f145f
		jp Lf1466		// 0x000f1462
		or ebx, ecx		// 0x000f1464
Lf1466:
		add ecx, ecx		// 0x000f1466
		add edx, 0x10		// 0x000f1468
		cmp ecx, edi		// 0x000f146b
		jbe Lf1401		// 0x000f146d
		test ebx, ebx		// 0x000f146f
		je Lf14ad		// 0x000f1471
		mov ecx, dword ptr [esi + 0x18]		// 0x000f1473
		test ecx, 0xfffffffe		// 0x000f1476
		push ebp		// 0x000f147c
		je Lf14c9		// 0x000f147d
		mov eax, dword ptr [esp + 0x50]		// 0x000f147f
		mov edx, dword ptr [esp + 0x48]		// 0x000f1483
		push eax		// 0x000f1487
		push ebx		// 0x000f1488
		push edx		// 0x000f1489
		and ecx, 0xfffffffe		// 0x000f148a
		call nxPCTreeCull		// 0x000f148d
		mov esi, dword ptr [esi + 0x18]		// 0x000f1492
		and esi, 0xfffffffe		// 0x000f1495
		je Lf14a4		// 0x000f1498
		add esi, 0x24		// 0x000f149a
		mov edi, ebx		// 0x000f149d
		jmp Lf1361		// 0x000f149f
Lf14a4:
		xor esi, esi		// 0x000f14a4
		mov edi, ebx		// 0x000f14a6
		jmp Lf1361		// 0x000f14a8
Lf14ad:
		mov eax, dword ptr [esi + 0x1c]		// 0x000f14ad
		mov ecx, dword ptr [esi + 0x20]		// 0x000f14b0
		push ebp		// 0x000f14b3
		push 0		// 0x000f14b4
		push eax		// 0x000f14b6
		push ecx		// 0x000f14b7
		call dword ptr [esp + 0x5c]		// 0x000f14b8
		add esp, 0x10		// 0x000f14bc
		pop edi		// 0x000f14bf
		pop esi		// 0x000f14c0
		pop ebp		// 0x000f14c1
		pop ebx		// 0x000f14c2
		add esp, 0x30		// 0x000f14c3
		ret 0x10		// 0x000f14c6
Lf14c9:
		mov edx, dword ptr [esi + 0x1c]		// 0x000f14c9
		mov eax, dword ptr [esi + 0x20]		// 0x000f14cc
		push 1		// 0x000f14cf
		push edx		// 0x000f14d1
		push eax		// 0x000f14d2
		call dword ptr [esp + 0x5c]		// 0x000f14d3
		add esp, 0x10		// 0x000f14d7
		pop edi		// 0x000f14da
		pop esi		// 0x000f14db
		pop ebp		// 0x000f14dc
		pop ebx		// 0x000f14dd
		add esp, 0x30		// 0x000f14de
		ret 0x10		// 0x000f14e1
Lf14e4:
		pop edi		// 0x000f14e4
		fstp st(0)		// 0x000f14e5
		pop esi		// 0x000f14e7
		fstp st(0)		// 0x000f14e8
		pop ebp		// 0x000f14ea
		pop ebx		// 0x000f14eb
		add esp, 0x30		// 0x000f14ec
		ret 0x10		// 0x000f14ef
		}
	}

// phys_fn_006324 (0x00103060, 10 B), 2 instructions
__declspec(naked) void __cdecl nxPCThunkB()
	{
	__asm
		{
		mov ecx, offset nxPCStatics + 16		// 0x00103060
		jmp nxPCCallContainerEmpty		// 0x00103065
		}
	}

// phys_fn_006322 (0x00103050, 10 B), 2 instructions
__declspec(naked) void __cdecl nxPCThunkA()
	{
	__asm
		{
		mov ecx, offset nxPCStatics + 0		// 0x00103050
		jmp nxPCCallContainerEmpty		// 0x00103055
		}
	}

// phys_fn_006326 (0x00103070, 10 B), 2 instructions
__declspec(naked) void __cdecl nxPCThunkC()
	{
	__asm
		{
		mov ecx, offset nxPCStatics + 36		// 0x00103070
		jmp nxPCCallContainerEmpty		// 0x00103075
		}
	}

// phys_fn_006328 (0x00103080, 10 B), 2 instructions
__declspec(naked) void __cdecl nxPCThunkD()
	{
	__asm
		{
		mov ecx, offset nxPCStatics + 56		// 0x00103080
		jmp nxPCCallContainerEmpty		// 0x00103085
		}
	}

// phys_fn_006330 (0x00103090, 10 B), 2 instructions
__declspec(naked) void __cdecl nxPCThunkE()
	{
	__asm
		{
		mov ecx, offset nxPCStatics + 72		// 0x00103090
		jmp nxPCCallContainerEmpty		// 0x00103095
		}
	}

// phys_fn_005234 (0x000e5740, 61 B), 21 instructions
__declspec(naked) void __fastcall nxPCStaticDtorBody(void* pruner)
	{
	__asm
		{
		push esi		// 0x000e5740
		mov esi, ecx		// 0x000e5741
		push edi		// 0x000e5743
		mov edi, dword ptr [esi + 0x3c]		// 0x000e5744
		test edi, edi		// 0x000e5747
		mov dword ptr [esi], 0x1011b9c8		// 0x000e5749
		je Le576c		// 0x000e574f
		mov ecx, edi		// 0x000e5751
		call nxPCCallTreeDtor		// 0x000e5753
		call nxPOCallAllocator		// 0x000e5758
		mov edx, dword ptr [eax]		// 0x000e575d
		push edi		// 0x000e575f
		mov ecx, eax		// 0x000e5760
		call dword ptr [edx + 0xc]		// 0x000e5762
		mov dword ptr [esi + 0x3c], 0		// 0x000e5765
Le576c:
		lea ecx, [esi + 0x40]		// 0x000e576c
		call nxPCCallContainerEmpty		// 0x000e576f
		pop edi		// 0x000e5774
		mov ecx, esi		// 0x000e5775
		pop esi		// 0x000e5777
		jmp nxPCCallPrunerDtor		// 0x000e5778
		}
	}

// phys_fn_005236 (0x000e5780, 141 B), 55 instructions
__declspec(naked) bool __fastcall nxPCStaticPlanes(void* pruner, void* edxUnused, unsigned a, unsigned b, unsigned c, unsigned d, unsigned e)
	{
	__asm
		{
		sub esp, 0x1c		// 0x000e5780
		push ebx		// 0x000e5783
		mov ebx, dword ptr [esp + 0x30]		// 0x000e5784
		push ebp		// 0x000e5788
		push esi		// 0x000e5789
		push edi		// 0x000e578a
		mov esi, ebx		// 0x000e578b
		mov edi, ebx		// 0x000e578d
		mov ebp, ecx		// 0x000e578f
		mov eax, dword ptr [ebp + 0x3c]		// 0x000e5791
		shr ebx, 1		// 0x000e5794
		and esi, 1		// 0x000e5796
		and edi, 4		// 0x000e5799
		and bl, 1		// 0x000e579c
		test eax, eax		// 0x000e579f
		jne Le57a8		// 0x000e57a1
		call nxPCCallBuildStaticTree		// 0x000e57a3
Le57a8:
		mov edx, dword ptr [ebp + 0x3c]		// 0x000e57a8
		test edx, edx		// 0x000e57ab
		jne Le57bb		// 0x000e57ad
		pop edi		// 0x000e57af
		pop esi		// 0x000e57b0
		pop ebp		// 0x000e57b1
		xor al, al		// 0x000e57b2
		pop ebx		// 0x000e57b4
		add esp, 0x1c		// 0x000e57b5
		ret 0x14		// 0x000e57b8
Le57bb:
		mov ecx, dword ptr [ebp + 0x18]		// 0x000e57bb
		mov eax, dword ptr [esp + 0x30]		// 0x000e57be
		mov dword ptr [esp + 0x18], esi		// 0x000e57c2
		lea esi, [esp + 0x10]		// 0x000e57c6
		push esi		// 0x000e57ca
		mov dword ptr [esp + 0x18], ecx		// 0x000e57cb
		mov ecx, dword ptr [esp + 0x3c]		// 0x000e57cf
		mov esi, 1		// 0x000e57d3
		shl esi, cl		// 0x000e57d8
		push offset nxPCCollectCallback		// 0x000e57da
		mov dword ptr [esp + 0x18], eax		// 0x000e57df
		mov eax, dword ptr [esp + 0x3c]		// 0x000e57e3
		dec esi		// 0x000e57e7
		push esi		// 0x000e57e8
		mov dword ptr [esp + 0x30], ecx		// 0x000e57e9
		push eax		// 0x000e57ed
		mov ecx, edx		// 0x000e57ee
		mov dword ptr [esp + 0x2c], edi		// 0x000e57f0
		mov dword ptr [esp + 0x30], eax		// 0x000e57f4
		mov byte ptr [esp + 0x38], bl		// 0x000e57f8
		call nxPCTreeCull		// 0x000e57fc
		pop edi		// 0x000e5801
		pop esi		// 0x000e5802
		pop ebp		// 0x000e5803
		mov al, 1		// 0x000e5804
		pop ebx		// 0x000e5806
		add esp, 0x1c		// 0x000e5807
		ret 0x14		// 0x000e580a
		}
	}

// phys_fn_005238 (0x000e5810, 86 B), 30 instructions
__declspec(naked) void* __fastcall nxPCStaticScalarDelete(void* pruner, void* edxUnused, unsigned flags)
	{
	__asm
		{
		push esi		// 0x000e5810
		mov esi, ecx		// 0x000e5811
		push edi		// 0x000e5813
		mov edi, dword ptr [esi + 0x3c]		// 0x000e5814
		test edi, edi		// 0x000e5817
		mov dword ptr [esi], 0x1011b9c8		// 0x000e5819
		je Le583c		// 0x000e581f
		mov ecx, edi		// 0x000e5821
		call nxPCCallTreeDtor		// 0x000e5823
		call nxPOCallAllocator		// 0x000e5828
		mov edx, dword ptr [eax]		// 0x000e582d
		push edi		// 0x000e582f
		mov ecx, eax		// 0x000e5830
		call dword ptr [edx + 0xc]		// 0x000e5832
		mov dword ptr [esi + 0x3c], 0		// 0x000e5835
Le583c:
		lea ecx, [esi + 0x40]		// 0x000e583c
		call nxPCCallContainerEmpty		// 0x000e583f
		mov ecx, esi		// 0x000e5844
		call nxPCCallPrunerDtor		// 0x000e5846
		test byte ptr [esp + 0xc], 1		// 0x000e584b
		je Le585f		// 0x000e5850
		call nxPOCallAllocator		// 0x000e5852
		mov edx, dword ptr [eax]		// 0x000e5857
		push esi		// 0x000e5859
		mov ecx, eax		// 0x000e585a
		call dword ptr [edx + 0xc]		// 0x000e585c
Le585f:
		pop edi		// 0x000e585f
		mov eax, esi		// 0x000e5860
		pop esi		// 0x000e5862
		ret 4		// 0x000e5863
		}
	}

// phys_fn_005240 (0x000e5870, 25 B), 8 instructions
__declspec(naked) void* __fastcall nxPCBoundedCtor(void* pruner)
	{
	__asm
		{
		push esi		// 0x000e5870
		mov esi, ecx		// 0x000e5871
		call nxPCCallPrunerCtor		// 0x000e5873
		mov dword ptr [esi], 0x1011b9f0		// 0x000e5878
		mov dword ptr [esi + 0x3c], 0		// 0x000e587e
		mov eax, esi		// 0x000e5885
		pop esi		// 0x000e5887
		ret		// 0x000e5888
		}
	}

// phys_fn_005244 and 005246 (one function) (0x000e58a0, 341 B), 129 instructions
__declspec(naked) bool __fastcall nxPCBuildTree(void* pruner)
	{
	__asm
		{
		sub esp, 0x1c		// 0x000e58a0
		push ebx		// 0x000e58a3
		push ebp		// 0x000e58a4
		push esi		// 0x000e58a5
		push edi		// 0x000e58a6
		mov edi, ecx		// 0x000e58a7
		mov esi, dword ptr [edi + 0x3c]		// 0x000e58a9
		xor ebp, ebp		// 0x000e58ac
		cmp esi, ebp		// 0x000e58ae
		je Le58c9		// 0x000e58b0
		mov ecx, esi		// 0x000e58b2
		call nxPCOctreeRelease		// 0x000e58b4
		call nxPOCallAllocator		// 0x000e58b9
		mov edx, dword ptr [eax]		// 0x000e58be
		push esi		// 0x000e58c0
		mov ecx, eax		// 0x000e58c1
		call dword ptr [edx + 0xc]		// 0x000e58c3
		mov dword ptr [edi + 0x3c], ebp		// 0x000e58c6
Le58c9:
		mov ebx, dword ptr [edi + 8]		// 0x000e58c9
		add ebx, dword ptr [edi + 4]		// 0x000e58cc
		je Le59eb		// 0x000e58cf
		mov eax, dword ptr [edi + 0x18]		// 0x000e58d5
		mov dword ptr [esp + 0x10], eax		// 0x000e58d8
		call nxPOCallAllocator		// 0x000e58dc
		mov edx, dword ptr [eax]		// 0x000e58e1
		push ebp		// 0x000e58e3
		push 0x50		// 0x000e58e4
		mov ecx, eax		// 0x000e58e6
		call dword ptr [edx]		// 0x000e58e8
		cmp eax, ebp		// 0x000e58ea
		je Le58f7		// 0x000e58ec
		mov ecx, eax		// 0x000e58ee
		call nxPCOctreeCtor		// 0x000e58f0
		jmp Le58f9		// 0x000e58f5
Le58f7:
		xor eax, eax		// 0x000e58f7
Le58f9:
		cmp eax, ebp		// 0x000e58f9
		mov dword ptr [edi + 0x3c], eax		// 0x000e58fb
		jne Le590a		// 0x000e58fe
		pop edi		// 0x000e5900
		pop esi		// 0x000e5901
		pop ebp		// 0x000e5902
		xor al, al		// 0x000e5903
		pop ebx		// 0x000e5905
		add esp, 0x1c		// 0x000e5906
		ret		// 0x000e5909
Le590a:
		lea ecx, [esp + 0x14]		// 0x000e590a
		call nxPCBoxEmpty		// 0x000e590e
		test ebx, ebx		// 0x000e5913
		jbe Le596b		// 0x000e5915
		jmp Le5920		// 0x000e5917
		lea esp, [esp]		// 0x000e5919
Le5920:
		mov eax, dword ptr [esp + 0x10]		// 0x000e5920
		mov esi, dword ptr [eax + ebp*4]		// 0x000e5924
		mov ax, word ptr [esi + 0x28]		// 0x000e5927
		cmp ax, 0xffff		// 0x000e592b
		jne Le5935		// 0x000e592f
		xor eax, eax		// 0x000e5931
		jmp Le595c		// 0x000e5933
Le5935:
		test byte ptr [esi + 8], 2		// 0x000e5935
		jne Le594f		// 0x000e5939
		mov edx, dword ptr [edi + 0x14]		// 0x000e593b
		movzx eax, ax		// 0x000e593e
		lea ecx, [eax + eax*2]		// 0x000e5941
		lea eax, [edx + ecx*8]		// 0x000e5944
		push eax		// 0x000e5947
		mov ecx, esi		// 0x000e5948
		call nxPCCallUpdateWorldAABB		// 0x000e594a
Le594f:
		movzx eax, word ptr [esi + 0x28]		// 0x000e594f
		mov edx, dword ptr [edi + 0x14]		// 0x000e5953
		lea ecx, [eax + eax*2]		// 0x000e5956
		lea eax, [edx + ecx*8]		// 0x000e5959
Le595c:
		push eax		// 0x000e595c
		lea ecx, [esp + 0x18]		// 0x000e595d
		call nxPCCallAABBAdd		// 0x000e5961
		inc ebp		// 0x000e5966
		cmp ebp, ebx		// 0x000e5967
		jb Le5920		// 0x000e5969
Le596b:
		fld dword ptr [edi + 0x1c]		// 0x000e596b
		fcomp dword ptr [edi + 0x28]		// 0x000e596e
		fnstsw ax		// 0x000e5971
		test ah, 0x41		// 0x000e5973
		je Le59bc		// 0x000e5976
		fld dword ptr [edi + 0x20]		// 0x000e5978
		fcomp dword ptr [edi + 0x2c]		// 0x000e597b
		fnstsw ax		// 0x000e597e
		test ah, 0x41		// 0x000e5980
		je Le59bc		// 0x000e5983
		fld dword ptr [edi + 0x24]		// 0x000e5985
		fcomp dword ptr [edi + 0x30]		// 0x000e5988
		fnstsw ax		// 0x000e598b
		test ah, 0x41		// 0x000e598d
		je Le59bc		// 0x000e5990
		mov eax, dword ptr [edi + 0x1c]		// 0x000e5992
		mov ecx, dword ptr [edi + 0x20]		// 0x000e5995
		mov edx, dword ptr [edi + 0x24]		// 0x000e5998
		mov dword ptr [esp + 0x14], eax		// 0x000e599b
		mov eax, dword ptr [edi + 0x28]		// 0x000e599f
		mov dword ptr [esp + 0x18], ecx		// 0x000e59a2
		mov ecx, dword ptr [edi + 0x2c]		// 0x000e59a6
		mov dword ptr [esp + 0x1c], edx		// 0x000e59a9
		mov edx, dword ptr [edi + 0x30]		// 0x000e59ad
		mov dword ptr [esp + 0x20], eax		// 0x000e59b0
		mov dword ptr [esp + 0x24], ecx		// 0x000e59b4
		mov dword ptr [esp + 0x28], edx		// 0x000e59b8
Le59bc:
		mov ecx, dword ptr [edi + 0x3c]		// 0x000e59bc
		lea eax, [esp + 0x14]		// 0x000e59bf
		push eax		// 0x000e59c3
		push 5		// 0x000e59c4
		call nxPOBuild		// 0x000e59c6
		xor esi, esi		// 0x000e59cb
		test ebx, ebx		// 0x000e59cd
		jbe Le59eb		// 0x000e59cf
Le59d1:
		mov ecx, dword ptr [esp + 0x10]		// 0x000e59d1
		mov edx, dword ptr [ecx + esi*4]		// 0x000e59d5
		mov ecx, dword ptr [edi + 0x3c]		// 0x000e59d8
		push 0		// 0x000e59db
		add edx, 0xc		// 0x000e59dd
		push edx		// 0x000e59e0
		call nxPOInsert		// 0x000e59e1
		inc esi		// 0x000e59e6
		cmp esi, ebx		// 0x000e59e7
		jb Le59d1		// 0x000e59e9
Le59eb:
		pop edi		// 0x000e59eb
		pop esi		// 0x000e59ec
		pop ebp		// 0x000e59ed
		mov al, 1		// 0x000e59ee
		pop ebx		// 0x000e59f0
		add esp, 0x1c		// 0x000e59f1
		ret		// 0x000e59f4
		}
	}

// phys_fn_005248 (0x000e5a00, 40 B), 17 instructions
__declspec(naked) bool __fastcall nxPCAddObject(void* pruner, void* edxUnused, void* object)
	{
	__asm
		{
		push esi		// 0x000e5a00
		mov esi, ecx		// 0x000e5a01
		mov ecx, dword ptr [esi + 0x3c]		// 0x000e5a03
		test ecx, ecx		// 0x000e5a06
		push edi		// 0x000e5a08
		mov edi, dword ptr [esp + 0xc]		// 0x000e5a09
		je Le5a1a		// 0x000e5a0d
		push 0		// 0x000e5a0f
		lea eax, [edi + 0xc]		// 0x000e5a11
		push eax		// 0x000e5a14
		call nxPOInsert		// 0x000e5a15
Le5a1a:
		push edi		// 0x000e5a1a
		lea ecx, [esi + 4]		// 0x000e5a1b
		call nxPCCallPoolAdd		// 0x000e5a1e
		pop edi		// 0x000e5a23
		pop esi		// 0x000e5a24
		ret 4		// 0x000e5a25
		}
	}

// phys_fn_005250 (0x000e5a30, 93 B), 37 instructions
__declspec(naked) bool __fastcall nxPCRemoveObject(void* pruner, void* edxUnused, void* object)
	{
	__asm
		{
		push ebx		// 0x000e5a30
		mov ebx, dword ptr [esp + 8]		// 0x000e5a31
		push esi		// 0x000e5a35
		mov esi, ecx		// 0x000e5a36
		mov ecx, dword ptr [esi + 0x3c]		// 0x000e5a38
		test ecx, ecx		// 0x000e5a3b
		je Le5a7a		// 0x000e5a3d
		push edi		// 0x000e5a3f
		lea eax, [ebx + 0xc]		// 0x000e5a40
		push eax		// 0x000e5a43
		call nxPORemove		// 0x000e5a44
		mov edi, dword ptr [esi + 0x3c]		// 0x000e5a49
		mov eax, dword ptr [edi + 0x4c]		// 0x000e5a4c
		test eax, eax		// 0x000e5a4f
		je Le5a5a		// 0x000e5a51
		mov ecx, dword ptr [eax + 0x18]		// 0x000e5a53
		test ecx, ecx		// 0x000e5a56
		jne Le5a79		// 0x000e5a58
Le5a5a:
		test edi, edi		// 0x000e5a5a
		je Le5a79		// 0x000e5a5c
		mov ecx, edi		// 0x000e5a5e
		call nxPCOctreeRelease		// 0x000e5a60
		call nxPOCallAllocator		// 0x000e5a65
		mov edx, dword ptr [eax]		// 0x000e5a6a
		push edi		// 0x000e5a6c
		mov ecx, eax		// 0x000e5a6d
		call dword ptr [edx + 0xc]		// 0x000e5a6f
		mov dword ptr [esi + 0x3c], 0		// 0x000e5a72
Le5a79:
		pop edi		// 0x000e5a79
Le5a7a:
		inc dword ptr [esi + 0x38]		// 0x000e5a7a
		push ebx		// 0x000e5a7d
		lea ecx, [esi + 4]		// 0x000e5a7e
		call nxPCCallPoolRemove		// 0x000e5a81
		pop esi		// 0x000e5a86
		mov al, 1		// 0x000e5a87
		pop ebx		// 0x000e5a89
		ret 4		// 0x000e5a8a
		}
	}

// phys_fn_005252 (0x000e5a90, 28 B), 10 instructions
__declspec(naked) bool __fastcall nxPCUpdateObject(void* pruner, void* edxUnused, void* object)
	{
	__asm
		{
		inc dword ptr [ecx + 0x38]		// 0x000e5a90
		mov ecx, dword ptr [ecx + 0x3c]		// 0x000e5a93
		test ecx, ecx		// 0x000e5a96
		je Le5aa7		// 0x000e5a98
		mov eax, dword ptr [esp + 4]		// 0x000e5a9a
		add eax, 0xc		// 0x000e5a9e
		push eax		// 0x000e5aa1
		call nxPOUpdate		// 0x000e5aa2
Le5aa7:
		mov al, 1		// 0x000e5aa7
		ret 4		// 0x000e5aa9
		}
	}

// phys_fn_005254 (0x000e5ab0, 807 B), 259 instructions
__declspec(naked) bool __fastcall nxPCQueryPlanes(void* pruner, void* edxUnused, void* result, const void* planes, unsigned nbPlanes, unsigned flags, unsigned mask)
	{
	__asm
		{
		sub esp, 0x40		// 0x000e5ab0
		mov eax, dword ptr [esp + 0x50]		// 0x000e5ab3
		push ebp		// 0x000e5ab7
		mov ebp, eax		// 0x000e5ab8
		and eax, 2		// 0x000e5aba
		push esi		// 0x000e5abd
		mov dword ptr [esp + 0x14], eax		// 0x000e5abe
		mov al, byte ptr [nxPCStatics + 32]		// 0x000e5ac2
		and ebp, 1		// 0x000e5ac7
		test al, 1		// 0x000e5aca
		mov esi, ecx		// 0x000e5acc
		push edi		// 0x000e5ace
		mov dword ptr [esp + 0xc], esi		// 0x000e5acf
		mov dword ptr [esp + 0x14], ebp		// 0x000e5ad3
		jne Le5aff		// 0x000e5ad7
		mov edx, dword ptr [nxPCStatics + 32]		// 0x000e5ad9
		or edx, 1		// 0x000e5adf
		mov ecx, offset nxPCStatics + 16		// 0x000e5ae2
		mov dword ptr [nxPCStatics + 32], edx		// 0x000e5ae7
		call nxPCCallContainerCtor		// 0x000e5aed
		push offset nxPCThunkB		// 0x000e5af2
		call nxPCCallAtexit		// 0x000e5af7
		add esp, 4		// 0x000e5afc
Le5aff:
		test byte ptr [nxPCStatics + 32], 2		// 0x000e5aff
		jne Le5b2e		// 0x000e5b06
		mov edx, dword ptr [nxPCStatics + 32]		// 0x000e5b08
		or edx, 2		// 0x000e5b0e
		mov ecx, offset nxPCStatics + 0		// 0x000e5b11
		mov dword ptr [nxPCStatics + 32], edx		// 0x000e5b16
		call nxPCCallContainerCtor		// 0x000e5b1c
		push offset nxPCThunkA		// 0x000e5b21
		call nxPCCallAtexit		// 0x000e5b26
		add esp, 4		// 0x000e5b2b
Le5b2e:
		mov eax, dword ptr [esi + 0x3c]		// 0x000e5b2e
		xor edi, edi		// 0x000e5b31
		cmp eax, edi		// 0x000e5b33
		jne Le5b4e		// 0x000e5b35
		mov ecx, esi		// 0x000e5b37
		call nxPCBuildTree		// 0x000e5b39
		cmp dword ptr [esi + 0x3c], edi		// 0x000e5b3e
		jne Le5b4e		// 0x000e5b41
		pop edi		// 0x000e5b43
		pop esi		// 0x000e5b44
		xor al, al		// 0x000e5b45
		pop ebp		// 0x000e5b47
		add esp, 0x40		// 0x000e5b48
		ret 0x14		// 0x000e5b4b
Le5b4e:
		cmp dword ptr [nxPCStatics + 20], edi		// 0x000e5b4e
		je Le5b5c		// 0x000e5b54
		mov dword ptr [nxPCStatics + 20], edi		// 0x000e5b56
Le5b5c:
		cmp dword ptr [nxPCStatics + 4], edi		// 0x000e5b5c
		je Le5b6a		// 0x000e5b62
		mov dword ptr [nxPCStatics + 4], edi		// 0x000e5b64
Le5b6a:
		mov eax, dword ptr [esp + 0x58]		// 0x000e5b6a
		mov ecx, dword ptr [esp + 0x54]		// 0x000e5b6e
		push ebx		// 0x000e5b72
		push offset nxPCStatics + 0		// 0x000e5b73
		push offset nxPCStatics + 16		// 0x000e5b78
		push eax		// 0x000e5b7d
		push ecx		// 0x000e5b7e
		mov ecx, dword ptr [esi + 0x3c]		// 0x000e5b7f
		call nxPQPlaneQuery		// 0x000e5b82
		mov eax, dword ptr [nxPCStatics + 20]		// 0x000e5b87
		xor ecx, ecx		// 0x000e5b8c
		cmp eax, edi		// 0x000e5b8e
		mov edi, dword ptr [esp + 0x54]		// 0x000e5b90
		mov dword ptr [esp + 0x60], eax		// 0x000e5b94
		mov dword ptr [esp + 0x14], ecx		// 0x000e5b98
		jbe Le5d7b		// 0x000e5b9c
Le5ba2:
		mov edx, dword ptr [nxPCStatics + 24]		// 0x000e5ba2
		mov eax, dword ptr [edx + ecx*4]		// 0x000e5ba8
		mov ebp, dword ptr [eax + 4]		// 0x000e5bab
		mov eax, dword ptr [esp + 0x18]		// 0x000e5bae
		test eax, eax		// 0x000e5bb2
		jne Le5bc0		// 0x000e5bb4
		test byte ptr [ebp + 8], 1		// 0x000e5bb6
		jne Le5d62		// 0x000e5bba
Le5bc0:
		mov ax, word ptr [ebp + 0x28]		// 0x000e5bc0
		cmp ax, 0xffff		// 0x000e5bc4
		jne Le5bce		// 0x000e5bc8
		xor esi, esi		// 0x000e5bca
		jmp Le5bf5		// 0x000e5bcc
Le5bce:
		test byte ptr [ebp + 8], 2		// 0x000e5bce
		jne Le5be8		// 0x000e5bd2
		mov edx, dword ptr [esi + 0x14]		// 0x000e5bd4
		movzx eax, ax		// 0x000e5bd7
		lea ecx, [eax + eax*2]		// 0x000e5bda
		lea eax, [edx + ecx*8]		// 0x000e5bdd
		push eax		// 0x000e5be0
		mov ecx, ebp		// 0x000e5be1
		call nxPCCallUpdateWorldAABB		// 0x000e5be3
Le5be8:
		movzx eax, word ptr [ebp + 0x28]		// 0x000e5be8
		mov edx, dword ptr [esi + 0x14]		// 0x000e5bec
		lea ecx, [eax + eax*2]		// 0x000e5bef
		lea esi, [edx + ecx*8]		// 0x000e5bf2
Le5bf5:
		fld dword ptr [esi]		// 0x000e5bf5
		mov ecx, dword ptr [esp + 0x5c]		// 0x000e5bf7
		fadd dword ptr [esi + 0xc]		// 0x000e5bfb
		lea eax, [esi + 0xc]		// 0x000e5bfe
		fld dword ptr [eax + 4]		// 0x000e5c01
		mov edx, 1		// 0x000e5c04
		fadd dword ptr [esi + 4]		// 0x000e5c09
		shl edx, cl		// 0x000e5c0c
		fld dword ptr [eax + 8]		// 0x000e5c0e
		xor ebx, ebx		// 0x000e5c11
		fadd dword ptr [esi + 8]		// 0x000e5c13
		dec edx		// 0x000e5c16
		fstp dword ptr [esp + 0x4c]		// 0x000e5c17
		fxch st(1)		// 0x000e5c1b
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e5c1d
		fstp dword ptr [esp + 0x38]		// 0x000e5c23
		mov ecx, dword ptr [esp + 0x38]		// 0x000e5c27
		mov dword ptr [esp + 0x2c], ecx		// 0x000e5c2b
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e5c2f
		fstp dword ptr [esp + 0x3c]		// 0x000e5c35
		mov ecx, dword ptr [esp + 0x3c]		// 0x000e5c39
		fld dword ptr [esp + 0x4c]		// 0x000e5c3d
		mov dword ptr [esp + 0x30], ecx		// 0x000e5c41
		fmul dword ptr [nxPOK101043cc_4]		// 0x000e5c45
		fstp dword ptr [esp + 0x40]		// 0x000e5c4b
		mov ecx, dword ptr [esp + 0x40]		// 0x000e5c4f
		mov dword ptr [esp + 0x34], ecx		// 0x000e5c53
		mov ecx, dword ptr [eax]		// 0x000e5c57
		mov dword ptr [esp + 0x20], ecx		// 0x000e5c59
		fld dword ptr [esp + 0x20]		// 0x000e5c5d
		fsub dword ptr [esp + 0x2c]		// 0x000e5c61
		mov ecx, dword ptr [eax + 4]		// 0x000e5c65
		mov eax, dword ptr [eax + 8]		// 0x000e5c68
		mov dword ptr [esp + 0x24], ecx		// 0x000e5c6b
		fstp dword ptr [esp + 0x20]		// 0x000e5c6f
		mov dword ptr [esp + 0x28], eax		// 0x000e5c73
		fld dword ptr [esp + 0x24]		// 0x000e5c77
		mov ecx, 1		// 0x000e5c7b
		cmp edx, ecx		// 0x000e5c80
		fsub dword ptr [esp + 0x30]		// 0x000e5c82
		fstp dword ptr [esp + 0x24]		// 0x000e5c86
		fld dword ptr [esp + 0x28]		// 0x000e5c8a
		fsub dword ptr [esp + 0x34]		// 0x000e5c8e
		fstp dword ptr [esp + 0x28]		// 0x000e5c92
		jb Le5d0a		// 0x000e5c96
		mov esi, dword ptr [esp + 0x58]		// 0x000e5c98
		add esi, 4		// 0x000e5c9c
		nop		// 0x000e5c9f
Le5ca0:
		test edx, ecx		// 0x000e5ca0
		je Le5d01		// 0x000e5ca2
		fld dword ptr [esi + 4]		// 0x000e5ca4
		fabs		// 0x000e5ca7
		fmul dword ptr [esp + 0x28]		// 0x000e5ca9
		fld dword ptr [esi]		// 0x000e5cad
		fabs		// 0x000e5caf
		fxch st(1)		// 0x000e5cb1
		fxch st(1)		// 0x000e5cb3
		fmul dword ptr [esp + 0x24]		// 0x000e5cb5
		faddp st(1), st(0)		// 0x000e5cb9
		fld dword ptr [esi - 4]		// 0x000e5cbb
		fabs		// 0x000e5cbe
		fst dword ptr [esp + 0x54]		// 0x000e5cc0
		fmul dword ptr [esp + 0x20]		// 0x000e5cc4
		faddp st(1), st(0)		// 0x000e5cc8
		fld dword ptr [esp + 0x34]		// 0x000e5cca
		fmul dword ptr [esi + 4]		// 0x000e5cce
		fld dword ptr [esp + 0x2c]		// 0x000e5cd1
		fmul dword ptr [esi - 4]		// 0x000e5cd5
		faddp st(1), st(0)		// 0x000e5cd8
		fld dword ptr [esp + 0x30]		// 0x000e5cda
		fmul dword ptr [esi]		// 0x000e5cde
		faddp st(1), st(0)		// 0x000e5ce0
		fadd dword ptr [esi + 8]		// 0x000e5ce2
		fld st(1)		// 0x000e5ce5
		fcomp st(1)		// 0x000e5ce7
		fnstsw ax		// 0x000e5ce9
		test ah, 5		// 0x000e5ceb
		jnp Le5d5a		// 0x000e5cee
		fxch st(1)		// 0x000e5cf0
		fchs		// 0x000e5cf2
		fcomp st(1)		// 0x000e5cf4
		fnstsw ax		// 0x000e5cf6
		fstp st(0)		// 0x000e5cf8
		test ah, 5		// 0x000e5cfa
		jp Le5d01		// 0x000e5cfd
		or ebx, ecx		// 0x000e5cff
Le5d01:
		add ecx, ecx		// 0x000e5d01
		add esi, 0x10		// 0x000e5d03
		cmp ecx, edx		// 0x000e5d06
		jbe Le5ca0		// 0x000e5d08
Le5d0a:
		mov eax, dword ptr [esp + 0x1c]		// 0x000e5d0a
		test eax, eax		// 0x000e5d0e
		mov eax, dword ptr [edi]		// 0x000e5d10
		je Le5d3c		// 0x000e5d12
		mov ecx, dword ptr [edi + 4]		// 0x000e5d14
		test ebx, ebx		// 0x000e5d17
		setne bl		// 0x000e5d19
		cmp ecx, eax		// 0x000e5d1c
		jne Le5d29		// 0x000e5d1e
		push 1		// 0x000e5d20
		mov ecx, edi		// 0x000e5d22
		call nxPQCallContainerResize		// 0x000e5d24
Le5d29:
		mov eax, dword ptr [edi + 4]		// 0x000e5d29
		mov ecx, dword ptr [edi + 8]		// 0x000e5d2c
		movzx edx, bl		// 0x000e5d2f
		or edx, ebp		// 0x000e5d32
		mov dword ptr [ecx + eax*4], edx		// 0x000e5d34
		inc dword ptr [edi + 4]		// 0x000e5d37
		jmp Le5d5e		// 0x000e5d3a
Le5d3c:
		mov edx, dword ptr [edi + 4]		// 0x000e5d3c
		cmp edx, eax		// 0x000e5d3f
		jne Le5d4c		// 0x000e5d41
		push 1		// 0x000e5d43
		mov ecx, edi		// 0x000e5d45
		call nxPQCallContainerResize		// 0x000e5d47
Le5d4c:
		mov eax, dword ptr [edi + 4]		// 0x000e5d4c
		mov ecx, dword ptr [edi + 8]		// 0x000e5d4f
		mov dword ptr [ecx + eax*4], ebp		// 0x000e5d52
		inc dword ptr [edi + 4]		// 0x000e5d55
		jmp Le5d5e		// 0x000e5d58
Le5d5a:
		fstp st(0)		// 0x000e5d5a
		fstp st(0)		// 0x000e5d5c
Le5d5e:
		mov esi, dword ptr [esp + 0x10]		// 0x000e5d5e
Le5d62:
		mov ecx, dword ptr [esp + 0x14]		// 0x000e5d62
		mov eax, dword ptr [esp + 0x60]		// 0x000e5d66
		inc ecx		// 0x000e5d6a
		cmp ecx, eax		// 0x000e5d6b
		mov dword ptr [esp + 0x14], ecx		// 0x000e5d6d
		jb Le5ba2		// 0x000e5d71
		mov ebp, dword ptr [esp + 0x18]		// 0x000e5d77
Le5d7b:
		mov eax, dword ptr [nxPCStatics + 4]		// 0x000e5d7b
		xor ebx, ebx		// 0x000e5d80
		test eax, eax		// 0x000e5d82
		mov dword ptr [esp + 0x60], eax		// 0x000e5d84
		jbe Le5dcb		// 0x000e5d88
		lea ebx, [ebx]		// 0x000e5d8a
Le5d90:
		test ebp, ebp		// 0x000e5d90
		mov edx, dword ptr [nxPCStatics + 8]		// 0x000e5d92
		mov eax, dword ptr [edx + ebx*4]		// 0x000e5d98
		mov esi, dword ptr [eax + 4]		// 0x000e5d9b
		jne Le5da6		// 0x000e5d9e
		test byte ptr [esi + 8], 1		// 0x000e5da0
		jne Le5dc2		// 0x000e5da4
Le5da6:
		mov ecx, dword ptr [edi + 4]		// 0x000e5da6
		cmp ecx, dword ptr [edi]		// 0x000e5da9
		jne Le5db6		// 0x000e5dab
		push 1		// 0x000e5dad
		mov ecx, edi		// 0x000e5daf
		call nxPQCallContainerResize		// 0x000e5db1
Le5db6:
		mov edx, dword ptr [edi + 4]		// 0x000e5db6
		mov eax, dword ptr [edi + 8]		// 0x000e5db9
		mov dword ptr [eax + edx*4], esi		// 0x000e5dbc
		inc dword ptr [edi + 4]		// 0x000e5dbf
Le5dc2:
		mov eax, dword ptr [esp + 0x60]		// 0x000e5dc2
		inc ebx		// 0x000e5dc6
		cmp ebx, eax		// 0x000e5dc7
		jb Le5d90		// 0x000e5dc9
Le5dcb:
		pop ebx		// 0x000e5dcb
		pop edi		// 0x000e5dcc
		pop esi		// 0x000e5dcd
		mov al, 1		// 0x000e5dce
		pop ebp		// 0x000e5dd0
		add esp, 0x40		// 0x000e5dd1
		ret 0x14		// 0x000e5dd4
		}
	}

// phys_fn_005256 (0x000e5de0, 633 B), 207 instructions
__declspec(naked) bool __fastcall nxPCQueryRays(void* pruner, void* edxUnused, void* result, const void* ray, float maxDist, unsigned firstContact, unsigned mask)
	{
	__asm
		{
		sub esp, 0x28		// 0x000e5de0
		push esi		// 0x000e5de3
		mov esi, ecx		// 0x000e5de4
		mov eax, dword ptr [esi + 0x3c]		// 0x000e5de6
		test eax, eax		// 0x000e5de9
		mov dword ptr [esp + 4], esi		// 0x000e5deb
		jne Le5e06		// 0x000e5def
		call nxPCBuildTree		// 0x000e5df1
		mov eax, dword ptr [esi + 0x3c]		// 0x000e5df6
		test eax, eax		// 0x000e5df9
		jne Le5e06		// 0x000e5dfb
		xor al, al		// 0x000e5dfd
		pop esi		// 0x000e5dff
		add esp, 0x28		// 0x000e5e00
		ret 0x14		// 0x000e5e03
Le5e06:
		test byte ptr [nxPCStatics + 52], 1		// 0x000e5e06
		jne Le5e35		// 0x000e5e0d
		mov edx, dword ptr [nxPCStatics + 52]		// 0x000e5e0f
		or edx, 1		// 0x000e5e15
		mov ecx, offset nxPCStatics + 36		// 0x000e5e18
		mov dword ptr [nxPCStatics + 52], edx		// 0x000e5e1d
		call nxPCCallContainerCtor		// 0x000e5e23
		push offset nxPCThunkC		// 0x000e5e28
		call nxPCCallAtexit		// 0x000e5e2d
		add esp, 4		// 0x000e5e32
Le5e35:
		mov eax, dword ptr [nxPCStatics + 40]		// 0x000e5e35
		test eax, eax		// 0x000e5e3a
		je Le5e48		// 0x000e5e3c
		mov dword ptr [nxPCStatics + 40], 0		// 0x000e5e3e
Le5e48:
		fld dword ptr [nxPOK10106858_4]		// 0x000e5e48
		push ebx		// 0x000e5e4e
		fld dword ptr [esp + 0x3c]		// 0x000e5e4f
		push ebp		// 0x000e5e53
		fucompp		// 0x000e5e54
		push edi		// 0x000e5e56
		push offset nxPCStatics + 36		// 0x000e5e57
		fnstsw ax		// 0x000e5e5c
		test ah, 0x44		// 0x000e5e5e
		mov eax, dword ptr [esp + 0x44]		// 0x000e5e61
		jp Le5f38		// 0x000e5e65
		mov ecx, dword ptr [esi + 0x3c]		// 0x000e5e6b
		push eax		// 0x000e5e6e
		call nxPQRayQuery		// 0x000e5e6f
		mov eax, dword ptr [nxPCStatics + 40]		// 0x000e5e74
		test eax, eax		// 0x000e5e79
		mov ebp, dword ptr [nxPCStatics + 44]		// 0x000e5e7b
		mov ebx, eax		// 0x000e5e81
		je Le604d		// 0x000e5e83
		mov edi, dword ptr [esp + 0x3c]		// 0x000e5e89
		lea ecx, [ecx]		// 0x000e5e8d
Le5e90:
		mov eax, dword ptr [ebp]		// 0x000e5e90
		mov esi, dword ptr [eax + 4]		// 0x000e5e93
		mov ecx, dword ptr [esp + 0x4c]		// 0x000e5e96
		mov eax, dword ptr [esi + 0x24]		// 0x000e5e9a
		dec ebx		// 0x000e5e9d
		add ebp, 4		// 0x000e5e9e
		test ecx, eax		// 0x000e5ea1
		je Le5f24		// 0x000e5ea3
		mov ax, word ptr [esi + 0x28]		// 0x000e5ea5
		cmp ax, 0xffff		// 0x000e5ea9
		jne Le5eb3		// 0x000e5ead
		xor eax, eax		// 0x000e5eaf
		jmp Le5ee2		// 0x000e5eb1
Le5eb3:
		test byte ptr [esi + 8], 2		// 0x000e5eb3
		jne Le5ed1		// 0x000e5eb7
		movzx eax, ax		// 0x000e5eb9
		lea edx, [eax + eax*2]		// 0x000e5ebc
		mov eax, dword ptr [esp + 0x10]		// 0x000e5ebf
		mov ecx, dword ptr [eax + 0x14]		// 0x000e5ec3
		lea edx, [ecx + edx*8]		// 0x000e5ec6
		push edx		// 0x000e5ec9
		mov ecx, esi		// 0x000e5eca
		call nxPCCallUpdateWorldAABB		// 0x000e5ecc
Le5ed1:
		movzx eax, word ptr [esi + 0x28]		// 0x000e5ed1
		mov ecx, dword ptr [esp + 0x10]		// 0x000e5ed5
		mov edx, dword ptr [ecx + 0x14]		// 0x000e5ed9
		lea eax, [eax + eax*2]		// 0x000e5edc
		lea eax, [edx + eax*8]		// 0x000e5edf
Le5ee2:
		mov edx, dword ptr [esp + 0x40]		// 0x000e5ee2
		lea ecx, [eax + 0xc]		// 0x000e5ee6
		push ecx		// 0x000e5ee9
		push eax		// 0x000e5eea
		push edx		// 0x000e5eeb
		call nxPQCallRayAABB		// 0x000e5eec
		add esp, 0xc		// 0x000e5ef1
		test al, al		// 0x000e5ef4
		je Le5f24		// 0x000e5ef6
		mov eax, dword ptr [edi + 4]		// 0x000e5ef8
		cmp eax, dword ptr [edi]		// 0x000e5efb
		jne Le5f08		// 0x000e5efd
		push 1		// 0x000e5eff
		mov ecx, edi		// 0x000e5f01
		call nxPQCallContainerResize		// 0x000e5f03
Le5f08:
		mov ecx, dword ptr [edi + 4]		// 0x000e5f08
		mov edx, dword ptr [edi + 8]		// 0x000e5f0b
		mov al, byte ptr [esp + 0x48]		// 0x000e5f0e
		mov dword ptr [edx + ecx*4], esi		// 0x000e5f12
		mov ecx, dword ptr [edi + 4]		// 0x000e5f15
		inc ecx		// 0x000e5f18
		test al, al		// 0x000e5f19
		mov dword ptr [edi + 4], ecx		// 0x000e5f1b
		jne Le604d		// 0x000e5f1e
Le5f24:
		test ebx, ebx		// 0x000e5f24
		jne Le5e90		// 0x000e5f26
		pop edi		// 0x000e5f2c
		pop ebp		// 0x000e5f2d
		pop ebx		// 0x000e5f2e
		mov al, 1		// 0x000e5f2f
		pop esi		// 0x000e5f31
		add esp, 0x28		// 0x000e5f32
		ret 0x14		// 0x000e5f35
Le5f38:
		fld dword ptr [esp + 0x48]		// 0x000e5f38
		mov ecx, dword ptr [eax]		// 0x000e5f3c
		fmul dword ptr [eax + 0xc]		// 0x000e5f3e
		mov edx, dword ptr [eax + 4]		// 0x000e5f41
		fld dword ptr [esp + 0x48]		// 0x000e5f44
		mov dword ptr [esp + 0x24], ecx		// 0x000e5f48
		fmul dword ptr [eax + 0x10]		// 0x000e5f4c
		mov dword ptr [esp + 0x28], edx		// 0x000e5f4f
		lea edx, [esp + 0x24]		// 0x000e5f53
		push edx		// 0x000e5f57
		fstp dword ptr [esp + 0x20]		// 0x000e5f58
		fld dword ptr [esp + 0x4c]		// 0x000e5f5c
		fmul dword ptr [eax + 0x14]		// 0x000e5f60
		fstp dword ptr [esp + 0x24]		// 0x000e5f63
		fadd dword ptr [eax]		// 0x000e5f67
		fld dword ptr [esp + 0x20]		// 0x000e5f69
		fadd dword ptr [eax + 4]		// 0x000e5f6d
		fld dword ptr [esp + 0x24]		// 0x000e5f70
		fadd dword ptr [eax + 8]		// 0x000e5f74
		mov eax, dword ptr [eax + 8]		// 0x000e5f77
		mov dword ptr [esp + 0x30], eax		// 0x000e5f7a
		fstp dword ptr [esp + 0x24]		// 0x000e5f7e
		mov ecx, dword ptr [esp + 0x24]		// 0x000e5f82
		fxch st(1)		// 0x000e5f86
		mov dword ptr [esp + 0x3c], ecx		// 0x000e5f88
		mov ecx, dword ptr [esi + 0x3c]		// 0x000e5f8c
		fstp dword ptr [esp + 0x34]		// 0x000e5f8f
		fstp dword ptr [esp + 0x38]		// 0x000e5f93
		call nxPQSegmentQuery		// 0x000e5f97
		mov eax, dword ptr [nxPCStatics + 40]		// 0x000e5f9c
		test eax, eax		// 0x000e5fa1
		mov ebp, dword ptr [nxPCStatics + 44]		// 0x000e5fa3
		mov ebx, eax		// 0x000e5fa9
		je Le604d		// 0x000e5fab
		mov edi, dword ptr [esp + 0x3c]		// 0x000e5fb1
Le5fb5:
		mov eax, dword ptr [ebp]		// 0x000e5fb5
		mov esi, dword ptr [eax + 4]		// 0x000e5fb8
		mov eax, dword ptr [esp + 0x4c]		// 0x000e5fbb
		mov ecx, dword ptr [esi + 0x24]		// 0x000e5fbf
		dec ebx		// 0x000e5fc2
		add ebp, 4		// 0x000e5fc3
		test eax, ecx		// 0x000e5fc6
		je Le6045		// 0x000e5fc8
		mov ax, word ptr [esi + 0x28]		// 0x000e5fca
		cmp ax, 0xffff		// 0x000e5fce
		jne Le5fd8		// 0x000e5fd2
		xor eax, eax		// 0x000e5fd4
		jmp Le6007		// 0x000e5fd6
Le5fd8:
		test byte ptr [esi + 8], 2		// 0x000e5fd8
		jne Le5ff6		// 0x000e5fdc
		mov edx, dword ptr [esp + 0x10]		// 0x000e5fde
		movzx eax, ax		// 0x000e5fe2
		lea ecx, [eax + eax*2]		// 0x000e5fe5
		mov eax, dword ptr [edx + 0x14]		// 0x000e5fe8
		lea ecx, [eax + ecx*8]		// 0x000e5feb
		push ecx		// 0x000e5fee
		mov ecx, esi		// 0x000e5fef
		call nxPCCallUpdateWorldAABB		// 0x000e5ff1
Le5ff6:
		movzx eax, word ptr [esi + 0x28]		// 0x000e5ff6
		lea edx, [eax + eax*2]		// 0x000e5ffa
		mov eax, dword ptr [esp + 0x10]		// 0x000e5ffd
		mov ecx, dword ptr [eax + 0x14]		// 0x000e6001
		lea eax, [ecx + edx*8]		// 0x000e6004
Le6007:
		lea edx, [eax + 0xc]		// 0x000e6007
		push edx		// 0x000e600a
		push eax		// 0x000e600b
		lea eax, [esp + 0x28]		// 0x000e600c
		push eax		// 0x000e6010
		call nxPQCallSegmentAABB		// 0x000e6011
		add esp, 0xc		// 0x000e6016
		test al, al		// 0x000e6019
		je Le6045		// 0x000e601b
		mov ecx, dword ptr [edi + 4]		// 0x000e601d
		cmp ecx, dword ptr [edi]		// 0x000e6020
		jne Le602d		// 0x000e6022
		push 1		// 0x000e6024
		mov ecx, edi		// 0x000e6026
		call nxPQCallContainerResize		// 0x000e6028
Le602d:
		mov edx, dword ptr [edi + 4]		// 0x000e602d
		mov eax, dword ptr [edi + 8]		// 0x000e6030
		mov dword ptr [eax + edx*4], esi		// 0x000e6033
		mov ecx, dword ptr [edi + 4]		// 0x000e6036
		mov al, byte ptr [esp + 0x48]		// 0x000e6039
		inc ecx		// 0x000e603d
		test al, al		// 0x000e603e
		mov dword ptr [edi + 4], ecx		// 0x000e6040
		jne Le604d		// 0x000e6043
Le6045:
		test ebx, ebx		// 0x000e6045
		jne Le5fb5		// 0x000e6047
Le604d:
		pop edi		// 0x000e604d
		pop ebp		// 0x000e604e
		pop ebx		// 0x000e604f
		mov al, 1		// 0x000e6050
		pop esi		// 0x000e6052
		add esp, 0x28		// 0x000e6053
		ret 0x14		// 0x000e6056
		}
	}

// phys_fn_005258 and 005260 (one function) (0x000e6060, 460 B), 150 instructions
__declspec(naked) bool __fastcall nxPCQuerySphere(void* pruner, void* edxUnused, void* result, const void* bounds, unsigned firstContact, unsigned mask)
	{
	__asm
		{
		push ecx		// 0x000e6060
		push ebx		// 0x000e6061
		push ebp		// 0x000e6062
		mov ebp, ecx		// 0x000e6063
		mov eax, dword ptr [ebp + 0x3c]		// 0x000e6065
		xor ebx, ebx		// 0x000e6068
		cmp eax, ebx		// 0x000e606a
		jne Le6080		// 0x000e606c
		call nxPCBuildTree		// 0x000e606e
		cmp dword ptr [ebp + 0x3c], ebx		// 0x000e6073
		jne Le6080		// 0x000e6076
		pop ebp		// 0x000e6078
		xor al, al		// 0x000e6079
		pop ebx		// 0x000e607b
		pop ecx		// 0x000e607c
		ret 0x10		// 0x000e607d
Le6080:
		test byte ptr [nxPCStatics + 88], 1		// 0x000e6080
		jne Le60af		// 0x000e6087
		mov edx, dword ptr [nxPCStatics + 88]		// 0x000e6089
		or edx, 1		// 0x000e608f
		mov ecx, offset nxPCStatics + 72		// 0x000e6092
		mov dword ptr [nxPCStatics + 88], edx		// 0x000e6097
		call nxPCCallContainerCtor		// 0x000e609d
		push offset nxPCThunkE		// 0x000e60a2
		call nxPCCallAtexit		// 0x000e60a7
		add esp, 4		// 0x000e60ac
Le60af:
		test byte ptr [nxPCStatics + 88], 2		// 0x000e60af
		jne Le60de		// 0x000e60b6
		mov edx, dword ptr [nxPCStatics + 88]		// 0x000e60b8
		or edx, 2		// 0x000e60be
		mov ecx, offset nxPCStatics + 56		// 0x000e60c1
		mov dword ptr [nxPCStatics + 88], edx		// 0x000e60c6
		call nxPCCallContainerCtor		// 0x000e60cc
		push offset nxPCThunkD		// 0x000e60d1
		call nxPCCallAtexit		// 0x000e60d6
		add esp, 4		// 0x000e60db
Le60de:
		cmp dword ptr [nxPCStatics + 76], ebx		// 0x000e60de
		je Le60ec		// 0x000e60e4
		mov dword ptr [nxPCStatics + 76], ebx		// 0x000e60e6
Le60ec:
		cmp dword ptr [nxPCStatics + 60], ebx		// 0x000e60ec
		je Le60fa		// 0x000e60f2
		mov dword ptr [nxPCStatics + 60], ebx		// 0x000e60f4
Le60fa:
		mov eax, dword ptr [esp + 0x14]		// 0x000e60fa
		mov ecx, dword ptr [ebp + 0x3c]		// 0x000e60fe
		push esi		// 0x000e6101
		push edi		// 0x000e6102
		push offset nxPCStatics + 56		// 0x000e6103
		push offset nxPCStatics + 72		// 0x000e6108
		push eax		// 0x000e610d
		call nxPQSphereQuery		// 0x000e610e
		mov eax, dword ptr [nxPCStatics + 76]		// 0x000e6113
		cmp eax, ebx		// 0x000e6118
		mov esi, dword ptr [esp + 0x18]		// 0x000e611a
		mov dword ptr [esp + 0x10], eax		// 0x000e611e
		jbe Le61d1		// 0x000e6122
		jmp Le6130		// 0x000e6128
		lea ebx, [ebx]		// 0x000e612a
Le6130:
		mov ecx, dword ptr [nxPCStatics + 80]		// 0x000e6130
		mov edx, dword ptr [ecx + ebx*4]		// 0x000e6136
		mov edi, dword ptr [edx + 4]		// 0x000e6139
		mov eax, dword ptr [esp + 0x24]		// 0x000e613c
		test dword ptr [edi + 0x24], eax		// 0x000e6140
		je Le61c4		// 0x000e6143
		mov ax, word ptr [edi + 0x28]		// 0x000e6145
		cmp ax, 0xffff		// 0x000e6149
		jne Le6153		// 0x000e614d
		xor eax, eax		// 0x000e614f
		jmp Le617a		// 0x000e6151
Le6153:
		test byte ptr [edi + 8], 2		// 0x000e6153
		jne Le616d		// 0x000e6157
		mov edx, dword ptr [ebp + 0x14]		// 0x000e6159
		movzx eax, ax		// 0x000e615c
		lea ecx, [eax + eax*2]		// 0x000e615f
		lea eax, [edx + ecx*8]		// 0x000e6162
		push eax		// 0x000e6165
		mov ecx, edi		// 0x000e6166
		call nxPCCallUpdateWorldAABB		// 0x000e6168
Le616d:
		movzx eax, word ptr [edi + 0x28]		// 0x000e616d
		mov edx, dword ptr [ebp + 0x14]		// 0x000e6171
		lea ecx, [eax + eax*2]		// 0x000e6174
		lea eax, [edx + ecx*8]		// 0x000e6177
Le617a:
		mov ecx, dword ptr [esp + 0x1c]		// 0x000e617a
		mov edx, dword ptr [ecx + 0xc]		// 0x000e617e
		mov dword ptr [esp + 0x18], edx		// 0x000e6181
		lea edx, [eax + 0xc]		// 0x000e6185
		push edx		// 0x000e6188
		push eax		// 0x000e6189
		mov eax, dword ptr [esp + 0x20]		// 0x000e618a
		push eax		// 0x000e618e
		push ecx		// 0x000e618f
		call nxPQSphereAABB		// 0x000e6190
		add esp, 0x10		// 0x000e6195
		test al, al		// 0x000e6198
		je Le61c4		// 0x000e619a
		mov ecx, dword ptr [esi + 4]		// 0x000e619c
		cmp ecx, dword ptr [esi]		// 0x000e619f
		jne Le61ac		// 0x000e61a1
		push 1		// 0x000e61a3
		mov ecx, esi		// 0x000e61a5
		call nxPQCallContainerResize		// 0x000e61a7
Le61ac:
		mov edx, dword ptr [esi + 4]		// 0x000e61ac
		mov eax, dword ptr [esi + 8]		// 0x000e61af
		mov dword ptr [eax + edx*4], edi		// 0x000e61b2
		mov ecx, dword ptr [esi + 4]		// 0x000e61b5
		mov al, byte ptr [esp + 0x20]		// 0x000e61b8
		inc ecx		// 0x000e61bc
		test al, al		// 0x000e61bd
		mov dword ptr [esi + 4], ecx		// 0x000e61bf
		jne Le6222		// 0x000e61c2
Le61c4:
		mov eax, dword ptr [esp + 0x10]		// 0x000e61c4
		inc ebx		// 0x000e61c8
		cmp ebx, eax		// 0x000e61c9
		jb Le6130		// 0x000e61cb
Le61d1:
		mov eax, dword ptr [nxPCStatics + 60]		// 0x000e61d1
		xor ebp, ebp		// 0x000e61d6
		test eax, eax		// 0x000e61d8
		mov ebx, eax		// 0x000e61da
		jbe Le6222		// 0x000e61dc
		mov edi, edi		// 0x000e61de
Le61e0:
		mov ecx, dword ptr [nxPCStatics + 64]		// 0x000e61e0
		mov edx, dword ptr [ecx + ebp*4]		// 0x000e61e6
		mov edi, dword ptr [edx + 4]		// 0x000e61e9
		mov eax, dword ptr [esp + 0x24]		// 0x000e61ec
		test dword ptr [edi + 0x24], eax		// 0x000e61f0
		je Le621d		// 0x000e61f3
		mov ecx, dword ptr [esi + 4]		// 0x000e61f5
		cmp ecx, dword ptr [esi]		// 0x000e61f8
		jne Le6205		// 0x000e61fa
		push 1		// 0x000e61fc
		mov ecx, esi		// 0x000e61fe
		call nxPQCallContainerResize		// 0x000e6200
Le6205:
		mov edx, dword ptr [esi + 4]		// 0x000e6205
		mov eax, dword ptr [esi + 8]		// 0x000e6208
		mov dword ptr [eax + edx*4], edi		// 0x000e620b
		mov ecx, dword ptr [esi + 4]		// 0x000e620e
		mov al, byte ptr [esp + 0x20]		// 0x000e6211
		inc ecx		// 0x000e6215
		test al, al		// 0x000e6216
		mov dword ptr [esi + 4], ecx		// 0x000e6218
		jne Le6222		// 0x000e621b
Le621d:
		inc ebp		// 0x000e621d
		cmp ebp, ebx		// 0x000e621e
		jb Le61e0		// 0x000e6220
Le6222:
		pop edi		// 0x000e6222
		pop esi		// 0x000e6223
		pop ebp		// 0x000e6224
		mov al, 1		// 0x000e6225
		pop ebx		// 0x000e6227
		pop ecx		// 0x000e6228
		ret 0x10		// 0x000e6229
		}
	}

// phys_fn_005825 (the oracle's _ftol2) (0x000f7a3c, 117 B), 37 instructions
__declspec(naked) void __cdecl nxPOFtol()
	{
	__asm
		{
		push ebp		// 0x000f7a3c
		mov ebp, esp		// 0x000f7a3d
		sub esp, 0x20		// 0x000f7a3f
		and esp, 0xfffffff0		// 0x000f7a42
		fld st(0)		// 0x000f7a45
		fst dword ptr [esp + 0x18]		// 0x000f7a47
		fistp qword ptr [esp + 0x10]		// 0x000f7a4b
		fild qword ptr [esp + 0x10]		// 0x000f7a4f
		mov edx, dword ptr [esp + 0x18]		// 0x000f7a53
		mov eax, dword ptr [esp + 0x10]		// 0x000f7a57
		test eax, eax		// 0x000f7a5b
		je Lf7a9b		// 0x000f7a5d
Lf7a5f:
		fsubp st(1), st(0)		// 0x000f7a5f
		test edx, edx		// 0x000f7a61
		jns Lf7a83		// 0x000f7a63
		fstp dword ptr [esp]		// 0x000f7a65
		mov ecx, dword ptr [esp]		// 0x000f7a68
		xor ecx, 0x80000000		// 0x000f7a6b
		add ecx, 0x7fffffff		// 0x000f7a71
		adc eax, 0		// 0x000f7a77
		mov edx, dword ptr [esp + 0x14]		// 0x000f7a7a
		adc edx, 0		// 0x000f7a7e
		jmp Lf7aaf		// 0x000f7a81
Lf7a83:
		fstp dword ptr [esp]		// 0x000f7a83
		mov ecx, dword ptr [esp]		// 0x000f7a86
		add ecx, 0x7fffffff		// 0x000f7a89
		sbb eax, 0		// 0x000f7a8f
		mov edx, dword ptr [esp + 0x14]		// 0x000f7a92
		sbb edx, 0		// 0x000f7a96
		jmp Lf7aaf		// 0x000f7a99
Lf7a9b:
		mov edx, dword ptr [esp + 0x14]		// 0x000f7a9b
		test edx, 0x7fffffff		// 0x000f7a9f
		jne Lf7a5f		// 0x000f7aa5
		fstp dword ptr [esp + 0x18]		// 0x000f7aa7
		fstp dword ptr [esp + 0x18]		// 0x000f7aab
Lf7aaf:
		leave		// 0x000f7aaf
		ret		// 0x000f7ab0
		}
	}

