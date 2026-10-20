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
// and 005560 (0x000f1d50 and 0x000f1db0, the ray walk and entry). The octree's jump table (0x000e7544, eight cases) is a chain of compares here: the inline
// assembler cannot emit a table of label addresses; the compare script checks the chain against the table.
// Compare script: docs/reconstruction/novodex-physics/evidence/phase4-pruner-octree-listing-compare.py.
#include "Opcode.h"
#include "IcePrunable.h"

extern "C" void nxPOCallAllocator();
extern "C" void nxPOCallMakeCube();
extern "C" void nxPOCallFtol();
extern "C" void nxPQCallContainerResize();
extern "C" void nxPQCallSegmentAABB();
extern "C" void nxPQCallRayAABB();
extern "C" void* nxPOAdapterNotify;
#pragma comment(linker, "/alternatename:_nxPOCallAllocator=?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ")
#pragma comment(linker, "/alternatename:_nxPOCallMakeCube=?MakeCube@AABB@IceMaths@@QBEMAAV12@@Z")
#pragma comment(linker, "/alternatename:_nxPOCallFtol=__ftol2")
#pragma comment(linker, "/alternatename:_nxPQCallContainerResize=?resize@SdkContainer@@QAE_NI@Z")
#pragma comment(linker, "/alternatename:_nxPQCallSegmentAABB=?nxSegmentAABB@@YA_NABVSegment@IceMaths@@ABVPoint@2@1@Z")
#pragma comment(linker, "/alternatename:_nxPQCallRayAABB=?nxRayAABB@@YA_NABVRay@IceMaths@@ABVPoint@2@1@Z")
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

static const unsigned nxPOK101041f0_4 = 0x0;
static const unsigned nxPOK101043cc_4 = 0x3f000000;
static const unsigned nxPOK101066f8_4 = 0x4f800000;
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
		call nxPOCallFtol		// 0x000e7993
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
		call nxPOCallFtol		// 0x000e7a03
		fld dword ptr [esp + 0x50]		// 0x000e7a08
		fadd dword ptr [ebx + 0x1c]		// 0x000e7a0c
		mov dword ptr [esp + 0x24], eax		// 0x000e7a0f
		fmul st(0), st(1)		// 0x000e7a13
		call nxPOCallFtol		// 0x000e7a15
		fld dword ptr [esp + 0x54]		// 0x000e7a1a
		fadd dword ptr [ebx + 0x20]		// 0x000e7a1e
		mov dword ptr [esp + 0x20], eax		// 0x000e7a21
		fmul st(0), st(1)		// 0x000e7a25
		call nxPOCallFtol		// 0x000e7a27
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

