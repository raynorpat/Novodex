// ContactMeshPMap.cpp: mesh contact against a penetration map, phys_fn_001878 and
// phys_fn_001880. Both rows are transcribed from the pinned x86 listing in the
// style of the other mesh-contact rows (the compare script is
// docs/reconstruction/novodex-physics/evidence/phase3-mesh-pmap-listing-compare.py).
//
// phys_fn_001880 (0x00047e50, 64 B) is the entry phys_fn_001876 takes when both
// shapes' meshes carry a penetration map (mesh + 0x9c): it runs 001878 once with
// each shape as the map's owner. phys_fn_001878 (0x000470f0, 3,422 B) walks the
// OTHER shape's mesh: it carries the owner's map box into that mesh's frame as
// six planes, collects the faces the box touches with the scene's PlanesCollider,
// and for each vertex of those faces, once per query, asks the map's cell sampler
// (phys_fn_002029, PenetrationMap::sampleCell) with the vertex's normal as the
// direction; a hit becomes a contact along the sampled displacement.

#include "NxPhysicsSDK.h"
#include "ContactGeneration.h"
#include "NxInternalTriangleMesh.h"
#include "PMap.h"

#include <float.h>
#include <math.h>
#include <string.h>

extern "C" void nxPMapCallObbPlanes();
extern "C" void nxPMapCallPlanesCollide();
extern "C" void nxPMapCallSampleCell();
#pragma comment(linker, "/alternatename:_nxPMapCallObbPlanes=?ComputePlanes@OBB@IceMaths@@QBE_NPAVPlane@2@@Z")
#pragma comment(linker, "/alternatename:_nxPMapCallPlanesCollide=?Collide@PlanesCollider@Opcode@@QAE_NAAUPlanesCache@2@PBVPlane@IceMaths@@IABVModel@2@PBVMatrix4x4@5@@Z")
#pragma comment(linker, "/alternatename:_nxPMapCallSampleCell=?sampleCell@PenetrationMap@@QBE_NIIIPAMPBM11@Z")

NxU32 __fastcall nxScratchStamp(void* scratch);

// The four constants the listing reads: 1.0f, 0.0f, 0.5f and -1.0f.
static const float nxPMapOne = 1.0f;
static const float nxPMapZero = 0.0f;
static const float nxPMapHalf = 0.5f;
static const float nxPMapMinusOne = -1.0f;

// phys_fn_001878 (0x000470f0, 3,422 B). cdecl with one more argument in ebx: the
// mesh shape's pose block (shape + 0x0c). Arguments: the map's owner, the owner's
// pose block (rotation[9], translation[3]), the mesh shape, the sink, the
// context. Reached only from nxMeshPMapContacts.
extern "C" __declspec(naked) void __cdecl nxMeshPMapContact(const NxCollisionShape*, const NxReal*,
	const NxCollisionShape*, NxContactSink*, void*)
	{
	__asm
		{
		push ebp		// 0x000470f0
		mov ebp, esp		// 0x000470f1
		and esp, 0xfffffff8		// 0x000470f3
		sub esp, 0x228		// 0x000470f6
		mov edx, dword ptr [ebp + 8]		// 0x000470fc
		mov eax, dword ptr [edx + 0xe0]		// 0x000470ff
		push esi		// 0x00047105
		push edi		// 0x00047106
		mov edi, dword ptr [eax + 0x9c]		// 0x00047107
		test edi, edi		// 0x0004710d
		mov dword ptr [esp + 0x170], edi		// 0x0004710f
		je L47e48		// 0x00047116
		mov ecx, dword ptr [ebp + 0xc]		// 0x0004711c
		fld dword ptr [ecx + 0x10]		// 0x0004711f
		fmul dword ptr [ecx + 0x20]		// 0x00047122
		fld dword ptr [ecx + 0x1c]		// 0x00047125
		fmul dword ptr [ecx + 0x14]		// 0x00047128
		fsubp st(1), st(0)		// 0x0004712b
		fld dword ptr [ecx + 0x1c]		// 0x0004712d
		fmul dword ptr [ecx + 8]		// 0x00047130
		fld dword ptr [ecx + 4]		// 0x00047133
		fmul dword ptr [ecx + 0x20]		// 0x00047136
		fsubp st(1), st(0)		// 0x00047139
		fstp dword ptr [esp + 0x1c]		// 0x0004713b
		fld dword ptr [ecx + 4]		// 0x0004713f
		fmul dword ptr [ecx + 0x14]		// 0x00047142
		fld dword ptr [ecx + 8]		// 0x00047145
		fmul dword ptr [ecx + 0x10]		// 0x00047148
		fsubp st(1), st(0)		// 0x0004714b
		fst dword ptr [esp + 0xc]		// 0x0004714d
		fmul dword ptr [ecx + 0x18]		// 0x00047151
		fld dword ptr [esp + 0x1c]		// 0x00047154
		fmul dword ptr [ecx + 0xc]		// 0x00047158
		faddp st(1), st(0)		// 0x0004715b
		fld st(1)		// 0x0004715d
		fmul dword ptr [ecx]		// 0x0004715f
		faddp st(1), st(0)		// 0x00047161
		fld dword ptr [nxPMapZero]		// 0x00047163
		fld st(1)		// 0x00047169
		fucompp		// 0x0004716b
		fnstsw ax		// 0x0004716d
		test ah, 0x44		// 0x0004716f
		jp L471d1		// 0x00047172
		fstp st(0)		// 0x00047174
		mov dword ptr [esp + 0xcc], 0		// 0x00047176
		fstp st(0)		// 0x00047181
		mov dword ptr [esp + 0xd0], 0x3f800000		// 0x00047183
		fld dword ptr [nxPMapOne]		// 0x0004718e
		mov dword ptr [esp + 0xd4], 0		// 0x00047194
		fld dword ptr [nxPMapZero]		// 0x0004719f
		mov dword ptr [esp + 0xd8], 0		// 0x000471a5
		fld dword ptr [nxPMapZero]		// 0x000471b0
		mov dword ptr [esp + 0xdc], 0		// 0x000471b6
		mov dword ptr [esp + 0xe0], 0x3f800000		// 0x000471c1
		jmp L47285		// 0x000471cc
L471d1:
		fld dword ptr [nxPMapOne]		// 0x000471d1
		fdiv st(0), st(1)		// 0x000471d7
		fstp dword ptr [esp + 8]		// 0x000471d9
		fstp st(0)		// 0x000471dd
		fmul dword ptr [esp + 8]		// 0x000471df
		fld dword ptr [esp + 8]		// 0x000471e3
		fmul dword ptr [esp + 0x1c]		// 0x000471e7
		fld dword ptr [esp + 8]		// 0x000471eb
		fmul dword ptr [esp + 0xc]		// 0x000471ef
		fld dword ptr [ecx + 0x18]		// 0x000471f3
		fmul dword ptr [ecx + 0x14]		// 0x000471f6
		fld dword ptr [ecx + 0xc]		// 0x000471f9
		fmul dword ptr [ecx + 0x20]		// 0x000471fc
		fsubp st(1), st(0)		// 0x000471ff
		fmul dword ptr [esp + 8]		// 0x00047201
		fstp dword ptr [esp + 0xcc]		// 0x00047205
		fld dword ptr [ecx]		// 0x0004720c
		fmul dword ptr [ecx + 0x20]		// 0x0004720e
		fld dword ptr [ecx + 0x18]		// 0x00047211
		fmul dword ptr [ecx + 8]		// 0x00047214
		fsubp st(1), st(0)		// 0x00047217
		fmul dword ptr [esp + 8]		// 0x00047219
		fstp dword ptr [esp + 0xd0]		// 0x0004721d
		fld dword ptr [ecx + 0xc]		// 0x00047224
		fmul dword ptr [ecx + 8]		// 0x00047227
		fld dword ptr [ecx]		// 0x0004722a
		fmul dword ptr [ecx + 0x14]		// 0x0004722c
		fsubp st(1), st(0)		// 0x0004722f
		fmul dword ptr [esp + 8]		// 0x00047231
		fstp dword ptr [esp + 0xd4]		// 0x00047235
		fld dword ptr [ecx + 0x1c]		// 0x0004723c
		fmul dword ptr [ecx + 0xc]		// 0x0004723f
		fld dword ptr [ecx + 0x18]		// 0x00047242
		fmul dword ptr [ecx + 0x10]		// 0x00047245
		fsubp st(1), st(0)		// 0x00047248
		fmul dword ptr [esp + 8]		// 0x0004724a
		fstp dword ptr [esp + 0xd8]		// 0x0004724e
		fld dword ptr [ecx + 0x18]		// 0x00047255
		fmul dword ptr [ecx + 4]		// 0x00047258
		fld dword ptr [ecx + 0x1c]		// 0x0004725b
		fmul dword ptr [ecx]		// 0x0004725e
		fsubp st(1), st(0)		// 0x00047260
		fmul dword ptr [esp + 8]		// 0x00047262
		fstp dword ptr [esp + 0xdc]		// 0x00047266
		fld dword ptr [ecx]		// 0x0004726d
		fmul dword ptr [ecx + 0x10]		// 0x0004726f
		fld dword ptr [ecx + 0xc]		// 0x00047272
		fmul dword ptr [ecx + 4]		// 0x00047275
		fsubp st(1), st(0)		// 0x00047278
		fmul dword ptr [esp + 8]		// 0x0004727a
		fstp dword ptr [esp + 0xe0]		// 0x0004727e
L47285:
		fld dword ptr [ecx + 0x24]		// 0x00047285
		fmul dword ptr [nxPMapMinusOne]		// 0x00047288
		fld dword ptr [ecx + 0x28]		// 0x0004728e
		fmul dword ptr [nxPMapMinusOne]		// 0x00047291
		fld dword ptr [ecx + 0x2c]		// 0x00047297
		fmul dword ptr [nxPMapMinusOne]		// 0x0004729a
		fld dword ptr [esp + 0xcc]		// 0x000472a0
		fmul st(0), st(3)		// 0x000472a7
		fld dword ptr [esp + 0xd4]		// 0x000472a9
		fmul st(0), st(2)		// 0x000472b0
		faddp st(1), st(0)		// 0x000472b2
		fld dword ptr [esp + 0xd0]		// 0x000472b4
		fmul st(0), st(3)		// 0x000472bb
		faddp st(1), st(0)		// 0x000472bd
		fstp dword ptr [esp + 0xc]		// 0x000472bf
		fld dword ptr [esp + 0xd8]		// 0x000472c3
		fmul st(0), st(3)		// 0x000472ca
		fld dword ptr [esp + 0xe0]		// 0x000472cc
		fmul st(0), st(2)		// 0x000472d3
		faddp st(1), st(0)		// 0x000472d5
		fld dword ptr [esp + 0xdc]		// 0x000472d7
		fmul st(0), st(3)		// 0x000472de
		faddp st(1), st(0)		// 0x000472e0
		fstp dword ptr [esp + 0x1c]		// 0x000472e2
		fxch st(2)		// 0x000472e6
		fmul st(0), st(5)		// 0x000472e8
		fld st(3)		// 0x000472ea
		fmul st(0), st(3)		// 0x000472ec
		faddp st(1), st(0)		// 0x000472ee
		fld st(4)		// 0x000472f0
		fmul st(0), st(2)		// 0x000472f2
		faddp st(1), st(0)		// 0x000472f4
		fstp dword ptr [esp + 0xe4]		// 0x000472f6
		fstp st(1)		// 0x000472fd
		fstp st(0)		// 0x000472ff
		fld st(0)		// 0x00047301
		fmul dword ptr [ebx + 0x2c]		// 0x00047303
		fld st(2)		// 0x00047306
		fmul dword ptr [ebx + 0x28]		// 0x00047308
		faddp st(1), st(0)		// 0x0004730b
		fld st(3)		// 0x0004730d
		fmul dword ptr [ebx + 0x24]		// 0x0004730f
		faddp st(1), st(0)		// 0x00047312
		fld dword ptr [esp + 0xcc]		// 0x00047314
		fmul dword ptr [ebx + 0x24]		// 0x0004731b
		fld dword ptr [esp + 0xd4]		// 0x0004731e
		fmul dword ptr [ebx + 0x2c]		// 0x00047325
		faddp st(1), st(0)		// 0x00047328
		fld dword ptr [esp + 0xd0]		// 0x0004732a
		fmul dword ptr [ebx + 0x28]		// 0x00047331
		faddp st(1), st(0)		// 0x00047334
		fstp dword ptr [esp + 0x14]		// 0x00047336
		fld dword ptr [esp + 0xd8]		// 0x0004733a
		fmul dword ptr [ebx + 0x24]		// 0x00047341
		fld dword ptr [esp + 0xe0]		// 0x00047344
		fmul dword ptr [ebx + 0x2c]		// 0x0004734b
		faddp st(1), st(0)		// 0x0004734e
		fld dword ptr [esp + 0xdc]		// 0x00047350
		fmul dword ptr [ebx + 0x28]		// 0x00047357
		faddp st(1), st(0)		// 0x0004735a
		fstp dword ptr [esp + 0x18]		// 0x0004735c
		fadd dword ptr [esp + 0xe4]		// 0x00047360
		fld dword ptr [esp + 0xc]		// 0x00047367
		fadd dword ptr [esp + 0x14]		// 0x0004736b
		fld dword ptr [esp + 0x1c]		// 0x0004736f
		fadd dword ptr [esp + 0x18]		// 0x00047373
		fstp dword ptr [esp + 0x160]		// 0x00047377
		mov ecx, dword ptr [esp + 0x160]		// 0x0004737e
		fxch st(1)		// 0x00047385
		mov dword ptr [esp + 0x150], ecx		// 0x00047387
		fstp dword ptr [esp + 0x148]		// 0x0004738e
		fstp dword ptr [esp + 0x14c]		// 0x00047395
		fld st(0)		// 0x0004739c
		fmul dword ptr [ebx + 0x18]		// 0x0004739e
		fld st(2)		// 0x000473a1
		fmul dword ptr [ebx + 0xc]		// 0x000473a3
		faddp st(1), st(0)		// 0x000473a6
		fld st(3)		// 0x000473a8
		fmul dword ptr [ebx]		// 0x000473aa
		faddp st(1), st(0)		// 0x000473ac
		fstp dword ptr [esp + 0x124]		// 0x000473ae
		fld st(0)		// 0x000473b5
		mov esi, dword ptr [ebp + 0x10]		// 0x000473b7
		fmul dword ptr [ebx + 0x1c]		// 0x000473ba
		mov ecx, dword ptr [esi + 0xe0]		// 0x000473bd
		fld st(2)		// 0x000473c3
		add ecx, 8		// 0x000473c5
		fmul dword ptr [ebx + 0x10]		// 0x000473c8
		mov dword ptr [esp + 0xc], ecx		// 0x000473cb
		faddp st(1), st(0)		// 0x000473cf
		fld st(3)		// 0x000473d1
		fmul dword ptr [ebx + 4]		// 0x000473d3
		faddp st(1), st(0)		// 0x000473d6
		fstp dword ptr [esp + 0x128]		// 0x000473d8
		fmul dword ptr [ebx + 0x20]		// 0x000473df
		fxch st(1)		// 0x000473e2
		fmul dword ptr [ebx + 0x14]		// 0x000473e4
		faddp st(1), st(0)		// 0x000473e7
		fxch st(1)		// 0x000473e9
		fmul dword ptr [ebx + 8]		// 0x000473eb
		faddp st(1), st(0)		// 0x000473ee
		fstp dword ptr [esp + 0x12c]		// 0x000473f0
		fld dword ptr [esp + 0xd4]		// 0x000473f7
		fmul dword ptr [ebx + 0x18]		// 0x000473fe
		fld dword ptr [esp + 0xd0]		// 0x00047401
		fmul dword ptr [ebx + 0xc]		// 0x00047408
		faddp st(1), st(0)		// 0x0004740b
		fld dword ptr [esp + 0xcc]		// 0x0004740d
		fmul dword ptr [ebx]		// 0x00047414
		faddp st(1), st(0)		// 0x00047416
		fstp dword ptr [esp + 0x130]		// 0x00047418
		fld dword ptr [esp + 0xcc]		// 0x0004741f
		fmul dword ptr [ebx + 4]		// 0x00047426
		fld dword ptr [esp + 0xd4]		// 0x00047429
		fmul dword ptr [ebx + 0x1c]		// 0x00047430
		faddp st(1), st(0)		// 0x00047433
		fld dword ptr [esp + 0xd0]		// 0x00047435
		fmul dword ptr [ebx + 0x10]		// 0x0004743c
		faddp st(1), st(0)		// 0x0004743f
		fstp dword ptr [esp + 0x134]		// 0x00047441
		fld dword ptr [esp + 0xcc]		// 0x00047448
		fmul dword ptr [ebx + 8]		// 0x0004744f
		fld dword ptr [esp + 0xd4]		// 0x00047452
		fmul dword ptr [ebx + 0x20]		// 0x00047459
		faddp st(1), st(0)		// 0x0004745c
		fld dword ptr [esp + 0xd0]		// 0x0004745e
		fmul dword ptr [ebx + 0x14]		// 0x00047465
		faddp st(1), st(0)		// 0x00047468
		fstp dword ptr [esp + 0x138]		// 0x0004746a
		fld dword ptr [esp + 0xdc]		// 0x00047471
		fmul dword ptr [ebx + 0xc]		// 0x00047478
		fld dword ptr [esp + 0xd8]		// 0x0004747b
		fmul dword ptr [ebx]		// 0x00047482
		faddp st(1), st(0)		// 0x00047484
		fld dword ptr [esp + 0xe0]		// 0x00047486
		fmul dword ptr [ebx + 0x18]		// 0x0004748d
		faddp st(1), st(0)		// 0x00047490
		fstp dword ptr [esp + 0x13c]		// 0x00047492
		fld dword ptr [esp + 0xd8]		// 0x00047499
		fmul dword ptr [ebx + 4]		// 0x000474a0
		fld dword ptr [esp + 0xe0]		// 0x000474a3
		fmul dword ptr [ebx + 0x1c]		// 0x000474aa
		faddp st(1), st(0)		// 0x000474ad
		fld dword ptr [esp + 0xdc]		// 0x000474af
		fmul dword ptr [ebx + 0x10]		// 0x000474b6
		faddp st(1), st(0)		// 0x000474b9
		fstp dword ptr [esp + 0x140]		// 0x000474bb
		fld dword ptr [esp + 0xd8]		// 0x000474c2
		fmul dword ptr [ebx + 8]		// 0x000474c9
		fld dword ptr [esp + 0xe0]		// 0x000474cc
		fmul dword ptr [ebx + 0x20]		// 0x000474d3
		faddp st(1), st(0)		// 0x000474d6
		fld dword ptr [esp + 0xdc]		// 0x000474d8
		fmul dword ptr [ebx + 0x14]		// 0x000474df
		faddp st(1), st(0)		// 0x000474e2
		fstp dword ptr [esp + 0x144]		// 0x000474e4
		mov eax, dword ptr [ecx + 0x18]		// 0x000474eb
		test eax, eax		// 0x000474ee
		jne L474fe		// 0x000474f0
		call nxMeshComputeVertexNormals		// 0x000474f2
		mov edx, dword ptr [ebp + 8]		// 0x000474f7
		mov ecx, dword ptr [esp + 0xc]		// 0x000474fa
L474fe:
		mov eax, dword ptr [ecx + 0x18]		// 0x000474fe
		mov dword ptr [esp + 0x174], eax		// 0x00047501
		mov eax, dword ptr [ebp + 0x18]		// 0x00047508
		mov ecx, dword ptr [eax + 0x64]		// 0x0004750b
		and ecx, 0xfffffffe		// 0x0004750e
		mov dword ptr [eax + 0x64], ecx		// 0x00047511
		add eax, 0x60		// 0x00047514
		and ecx, 0xfffffffd		// 0x00047517
		mov dword ptr [eax + 4], ecx		// 0x0004751a
		fld dword ptr [edi + 0x14]		// 0x0004751d
		fsub dword ptr [edi + 8]		// 0x00047520
		mov ecx, dword ptr [esi + 0xe0]		// 0x00047523
		fld dword ptr [edi + 0x18]		// 0x00047529
		mov eax, dword ptr [ecx + 0x28]		// 0x0004752c
		fsub dword ptr [edi + 0xc]		// 0x0004752f
		mov dword ptr [esp + 0xc], eax		// 0x00047532
		fstp dword ptr [esp + 0x24]		// 0x00047536
		fld dword ptr [edi + 0x1c]		// 0x0004753a
		fsub dword ptr [edi + 0x10]		// 0x0004753d
		fstp dword ptr [esp + 0x28]		// 0x00047540
		fmul dword ptr [nxPMapHalf]		// 0x00047544
		fld dword ptr [esp + 0x24]		// 0x0004754a
		fmul dword ptr [nxPMapHalf]		// 0x0004754e
		fld dword ptr [esp + 0x28]		// 0x00047554
		fmul dword ptr [nxPMapHalf]		// 0x00047558
		fstp dword ptr [esp + 0x28]		// 0x0004755e
		mov eax, dword ptr [esp + 0x28]		// 0x00047562
		fld dword ptr [edi + 0x14]		// 0x00047566
		fadd dword ptr [edi + 8]		// 0x00047569
		fld dword ptr [edi + 0x18]		// 0x0004756c
		fadd dword ptr [edi + 0xc]		// 0x0004756f
		fstp dword ptr [esp + 0x14]		// 0x00047572
		fld dword ptr [edi + 0x1c]		// 0x00047576
		fadd dword ptr [edi + 0x10]		// 0x00047579
		mov dword ptr [esp + 0x58], eax		// 0x0004757c
		mov eax, dword ptr [edx + 0x14]		// 0x00047580
		mov dword ptr [esp + 0xa0], eax		// 0x00047583
		fstp dword ptr [esp + 0x18]		// 0x0004758a
		mov eax, dword ptr [edx + 0x20]		// 0x0004758e
		mov dword ptr [esp + 0xa4], eax		// 0x00047591
		fmul dword ptr [nxPMapHalf]		// 0x00047598
		mov eax, dword ptr [edx + 0x2c]		// 0x0004759e
		fld dword ptr [esp + 0x14]		// 0x000475a1
		mov dword ptr [esp + 0xa8], eax		// 0x000475a5
		fmul dword ptr [nxPMapHalf]		// 0x000475ac
		fld dword ptr [esp + 0x18]		// 0x000475b2
		fmul dword ptr [nxPMapHalf]		// 0x000475b6
		fstp dword ptr [esp + 0x18]		// 0x000475bc
		mov ecx, dword ptr [esp + 0x18]		// 0x000475c0
		mov dword ptr [esp + 0x4c], ecx		// 0x000475c4
		fxch st(1)		// 0x000475c8
		xor ecx, ecx		// 0x000475ca
		fstp dword ptr [esp + 0x44]		// 0x000475cc
		mov dword ptr [esp + 0x5c], ecx		// 0x000475d0
		mov dword ptr [esp + 0x60], ecx		// 0x000475d4
		fstp dword ptr [esp + 0x48]		// 0x000475d8
		mov dword ptr [esp + 0x64], ecx		// 0x000475dc
		fxch st(1)		// 0x000475e0
		mov dword ptr [esp + 0x68], ecx		// 0x000475e2
		fstp dword ptr [esp + 0x50]		// 0x000475e6
		mov dword ptr [esp + 0x6c], ecx		// 0x000475ea
		mov dword ptr [esp + 0x70], ecx		// 0x000475ee
		fstp dword ptr [esp + 0x54]		// 0x000475f2
		fld dword ptr [edx + 0xc]		// 0x000475f6
		mov dword ptr [esp + 0x74], ecx		// 0x000475f9
		fld dword ptr [edx + 0x10]		// 0x000475fd
		mov dword ptr [esp + 0x78], ecx		// 0x00047600
		fld dword ptr [edx + 0x18]		// 0x00047604
		mov dword ptr [esp + 0x7c], ecx		// 0x00047607
		mov ecx, dword ptr [edx + 0x1c]		// 0x0004760b
		fld dword ptr [edx + 0x24]		// 0x0004760e
		fld dword ptr [edx + 0x30]		// 0x00047611
		mov dword ptr [esp + 0x94], ecx		// 0x00047614
		mov ecx, dword ptr [edx + 0x28]		// 0x0004761b
		fld dword ptr [edx + 0x34]		// 0x0004761e
		mov dword ptr [esp + 0x98], ecx		// 0x00047621
		mov ecx, dword ptr [edx + 0x38]		// 0x00047628
		mov dword ptr [esp + 0x7c], 0x3f800000		// 0x0004762b
		mov dword ptr [esp + 0x6c], 0x3f800000		// 0x00047633
		mov dword ptr [esp + 0x5c], 0x3f800000		// 0x0004763b
		mov dword ptr [esp + 0xb8], ecx		// 0x00047643
		fld st(5)		// 0x0004764a
		mov edx, dword ptr [esi + 0xc]		// 0x0004764c
		fmul dword ptr [esp + 0x44]		// 0x0004764f
		mov eax, dword ptr [esi + 0x10]		// 0x00047653
		fld dword ptr [esp + 0x4c]		// 0x00047656
		mov ecx, dword ptr [esi + 0x14]		// 0x0004765a
		fmul dword ptr [esp + 0xa0]		// 0x0004765d
		mov dword ptr [esp + 0x198], edx		// 0x00047664
		mov edx, dword ptr [esi + 0x18]		// 0x0004766b
		mov dword ptr [esp + 0x1a8], eax		// 0x0004766e
		faddp st(1), st(0)		// 0x00047675
		mov eax, dword ptr [esi + 0x1c]		// 0x00047677
		fld dword ptr [esp + 0x48]		// 0x0004767a
		mov dword ptr [esp + 0x1b8], ecx		// 0x0004767e
		fmul st(0), st(6)		// 0x00047685
		mov ecx, dword ptr [esi + 0x20]		// 0x00047687
		mov dword ptr [esp + 0x19c], edx		// 0x0004768a
		mov edx, dword ptr [esi + 0x24]		// 0x00047691
		faddp st(1), st(0)		// 0x00047694
		mov dword ptr [esp + 0x1ac], eax		// 0x00047696
		mov eax, dword ptr [esi + 0x28]		// 0x0004769d
		mov dword ptr [esp + 0x1bc], ecx		// 0x000476a0
		fadd st(0), st(2)		// 0x000476a7
		mov ecx, dword ptr [esi + 0x2c]		// 0x000476a9
		mov dword ptr [esp + 0x1a0], edx		// 0x000476ac
		mov edx, dword ptr [esi + 0x30]		// 0x000476b3
		fstp dword ptr [esp + 0x10]		// 0x000476b6
		mov dword ptr [esp + 0x1b0], eax		// 0x000476ba
		fld dword ptr [esp + 0x4c]		// 0x000476c1
		mov eax, dword ptr [esi + 0x34]		// 0x000476c5
		fmul dword ptr [esp + 0xa4]		// 0x000476c8
		mov dword ptr [esp + 0x1c0], ecx		// 0x000476cf
		fld dword ptr [esp + 0x48]		// 0x000476d6
		mov ecx, dword ptr [esi + 0x38]		// 0x000476da
		fmul dword ptr [esp + 0x94]		// 0x000476dd
		mov dword ptr [esp + 0x1c8], edx		// 0x000476e4
		mov edx, dword ptr [esp + 0x50]		// 0x000476eb
		mov dword ptr [esp + 0x1cc], eax		// 0x000476ef
		faddp st(1), st(0)		// 0x000476f6
		mov eax, dword ptr [esp + 0x54]		// 0x000476f8
		fld dword ptr [esp + 0x44]		// 0x000476fc
		mov dword ptr [esp + 0x1d0], ecx		// 0x00047700
		fmul st(0), st(5)		// 0x00047707
		mov ecx, dword ptr [esp + 0x58]		// 0x00047709
		mov dword ptr [esp + 0x200], edx		// 0x0004770d
		mov edx, dword ptr [esp + 0x10]		// 0x00047714
		faddp st(1), st(0)		// 0x00047718
		mov dword ptr [esp + 0x204], eax		// 0x0004771a
		mov dword ptr [esp + 0x208], ecx		// 0x00047721
		mov dword ptr [esp + 0x1f4], edx		// 0x00047728
		fadd st(0), st(1)		// 0x0004772f
		mov edx, dword ptr [esp + 0x94]		// 0x00047731
		mov dword ptr [esp + 0x1c4], 0		// 0x00047738
		mov dword ptr [esp + 0x1b4], 0		// 0x00047743
		fstp dword ptr [esp + 0x14]		// 0x0004774e
		mov eax, dword ptr [esp + 0x14]		// 0x00047752
		mov dword ptr [esp + 0x1f8], eax		// 0x00047756
		fstp st(0)		// 0x0004775d
		mov eax, dword ptr [esp + 0x98]		// 0x0004775f
		fstp st(0)		// 0x00047766
		mov dword ptr [esp + 0x1a4], 0		// 0x00047768
		fld dword ptr [esp + 0x4c]		// 0x00047773
		mov dword ptr [esp + 0x1d4], 0x3f800000		// 0x00047777
		fmul dword ptr [esp + 0xa8]		// 0x00047782
		mov dword ptr [esp + 0xd0], edx		// 0x00047789
		fld dword ptr [esp + 0x48]		// 0x00047790
		fmul dword ptr [esp + 0x98]		// 0x00047794
		faddp st(1), st(0)		// 0x0004779b
		fld dword ptr [esp + 0x44]		// 0x0004779d
		fmul st(0), st(2)		// 0x000477a1
		faddp st(1), st(0)		// 0x000477a3
		fadd dword ptr [esp + 0xb8]		// 0x000477a5
		fstp dword ptr [esp + 0x18]		// 0x000477ac
		mov ecx, dword ptr [esp + 0x18]		// 0x000477b0
		fxch st(3)		// 0x000477b4
		mov dword ptr [esp + 0x1fc], ecx		// 0x000477b6
		fstp dword ptr [esp + 0xc0]		// 0x000477bd
		fstp dword ptr [esp + 0xc4]		// 0x000477c4
		fxch st(1)		// 0x000477cb
		fstp dword ptr [esp + 0xc8]		// 0x000477cd
		fstp dword ptr [esp + 0xcc]		// 0x000477d4
		mov ecx, dword ptr [esp + 0xa0]		// 0x000477db
		fld dword ptr [esp + 0x60]		// 0x000477e2
		mov edx, dword ptr [esp + 0xa4]		// 0x000477e6
		mov dword ptr [esp + 0xd8], ecx		// 0x000477ed
		mov dword ptr [esp + 0xd4], eax		// 0x000477f4
		mov eax, dword ptr [esp + 0xa8]		// 0x000477fb
		mov dword ptr [esp + 0xdc], edx		// 0x00047802
		mov dword ptr [esp + 0xe0], eax		// 0x00047809
		mov ecx, 9		// 0x00047810
		lea esi, [esp + 0xc0]		// 0x00047815
		lea edi, [esp + 0x80]		// 0x0004781c
		rep movsd		// 0x00047823
		fmul dword ptr [esp + 0x8c]		// 0x00047825
		fld dword ptr [esp + 0x64]		// 0x0004782c
		fmul dword ptr [esp + 0x98]		// 0x00047830
		faddp st(1), st(0)		// 0x00047837
		fadd dword ptr [esp + 0x80]		// 0x00047839
		fstp dword ptr [esp + 0xc0]		// 0x00047840
		mov ecx, 9		// 0x00047847
		fld dword ptr [esp + 0x60]		// 0x0004784c
		lea esi, [esp + 0xc0]		// 0x00047850
		fmul dword ptr [esp + 0x90]		// 0x00047857
		lea edi, [esp + 0x20c]		// 0x0004785e
		fld dword ptr [esp + 0x64]		// 0x00047865
		fmul dword ptr [esp + 0x9c]		// 0x00047869
		faddp st(1), st(0)		// 0x00047870
		fadd dword ptr [esp + 0x84]		// 0x00047872
		fstp dword ptr [esp + 0xc4]		// 0x00047879
		fld dword ptr [esp + 0x60]		// 0x00047880
		fmul dword ptr [esp + 0x94]		// 0x00047884
		fld dword ptr [esp + 0x64]		// 0x0004788b
		fmul dword ptr [esp + 0xa0]		// 0x0004788f
		faddp st(1), st(0)		// 0x00047896
		fadd dword ptr [esp + 0x88]		// 0x00047898
		fstp dword ptr [esp + 0xc8]		// 0x0004789f
		fld dword ptr [esp + 0x70]		// 0x000478a6
		fmul dword ptr [esp + 0x98]		// 0x000478aa
		fld dword ptr [esp + 0x68]		// 0x000478b1
		fmul dword ptr [esp + 0x80]		// 0x000478b5
		faddp st(1), st(0)		// 0x000478bc
		fadd dword ptr [esp + 0x8c]		// 0x000478be
		fstp dword ptr [esp + 0xcc]		// 0x000478c5
		fld dword ptr [esp + 0x68]		// 0x000478cc
		fmul dword ptr [esp + 0x84]		// 0x000478d0
		fld dword ptr [esp + 0x70]		// 0x000478d7
		fmul dword ptr [esp + 0x9c]		// 0x000478db
		faddp st(1), st(0)		// 0x000478e2
		fadd dword ptr [esp + 0x90]		// 0x000478e4
		fstp dword ptr [esp + 0xd0]		// 0x000478eb
		fld dword ptr [esp + 0x68]		// 0x000478f2
		fmul dword ptr [esp + 0x88]		// 0x000478f6
		fld dword ptr [esp + 0x70]		// 0x000478fd
		fmul dword ptr [esp + 0xa0]		// 0x00047901
		faddp st(1), st(0)		// 0x00047908
		fadd dword ptr [esp + 0x94]		// 0x0004790a
		fstp dword ptr [esp + 0xd4]		// 0x00047911
		fld dword ptr [esp + 0x78]		// 0x00047918
		fmul dword ptr [esp + 0x8c]		// 0x0004791c
		fld dword ptr [esp + 0x74]		// 0x00047923
		fmul dword ptr [esp + 0x80]		// 0x00047927
		faddp st(1), st(0)		// 0x0004792e
		fadd dword ptr [esp + 0x98]		// 0x00047930
		fstp dword ptr [esp + 0xd8]		// 0x00047937
		fld dword ptr [esp + 0x74]		// 0x0004793e
		fmul dword ptr [esp + 0x84]		// 0x00047942
		fld dword ptr [esp + 0x78]		// 0x00047949
		fmul dword ptr [esp + 0x90]		// 0x0004794d
		faddp st(1), st(0)		// 0x00047954
		fadd dword ptr [esp + 0x9c]		// 0x00047956
		fstp dword ptr [esp + 0xdc]		// 0x0004795d
		fld dword ptr [esp + 0x74]		// 0x00047964
		fmul dword ptr [esp + 0x88]		// 0x00047968
		fld dword ptr [esp + 0x78]		// 0x0004796f
		fmul dword ptr [esp + 0x94]		// 0x00047973
		faddp st(1), st(0)		// 0x0004797a
		fadd dword ptr [esp + 0xa0]		// 0x0004797c
		fstp dword ptr [esp + 0xe0]		// 0x00047983
		rep movsd		// 0x0004798a
		lea ecx, [esp + 0xc0]		// 0x0004798c
		push ecx		// 0x00047993
		lea ecx, [esp + 0x1f8]		// 0x00047994
		call nxPMapCallObbPlanes		// 0x0004799b
		mov eax, dword ptr [esp + 0xc]		// 0x000479a0
		mov esi, dword ptr [ebp + 0x18]		// 0x000479a4
		lea edx, [esp + 0x198]		// 0x000479a7
		push edx		// 0x000479ae
		push eax		// 0x000479af
		push 6		// 0x000479b0
		lea ecx, [esp + 0xcc]		// 0x000479b2
		lea edx, [esi + 0xa8]		// 0x000479b9
		push ecx		// 0x000479bf
		add esi, 0x60		// 0x000479c0
		push edx		// 0x000479c3
		mov ecx, esi		// 0x000479c4
		call nxPMapCallPlanesCollide		// 0x000479c6
		mov eax, dword ptr [esi + 0x10]		// 0x000479cb
		test eax, eax		// 0x000479ce
		je L479d7		// 0x000479d0
		mov eax, dword ptr [eax + 8]		// 0x000479d2
		jmp L479d9		// 0x000479d5
L479d7:
		xor eax, eax		// 0x000479d7
L479d9:
		mov esi, dword ptr [esi + 0x10]		// 0x000479d9
		test esi, esi		// 0x000479dc
		mov edi, eax		// 0x000479de
		je L479e7		// 0x000479e0
		mov esi, dword ptr [esi + 4]		// 0x000479e2
		jmp L479e9		// 0x000479e5
L479e7:
		xor esi, esi		// 0x000479e7
L479e9:
		mov ecx, dword ptr [ebp + 0x18]		// 0x000479e9
		call nxScratchStamp		// 0x000479ec
		test esi, esi		// 0x000479f1
		mov edx, eax		// 0x000479f3
		mov dword ptr [esp + 0xc], edx		// 0x000479f5
		je L47e48		// 0x000479f9
		mov dword ptr [esp + 8], esi		// 0x000479ff
		jmp L47a10		// 0x00047a03
L47a05:
		mov edi, dword ptr [esp + 0x154]		// 0x00047a05
		mov edx, dword ptr [esp + 0xc]		// 0x00047a0c
L47a10:
		mov ecx, dword ptr [ebp + 0x10]		// 0x00047a10
		mov eax, dword ptr [edi]		// 0x00047a13
		mov ecx, dword ptr [ecx + 0xe0]		// 0x00047a15
		mov ecx, dword ptr [ecx + 0x14]		// 0x00047a1b
		lea eax, [eax + eax*2]		// 0x00047a1e
		lea eax, [ecx + eax*4]		// 0x00047a21
		add edi, 4		// 0x00047a24
		mov dword ptr [esp + 0x178], eax		// 0x00047a27
		xor eax, eax		// 0x00047a2e
		mov dword ptr [esp + 0x154], edi		// 0x00047a30
		mov dword ptr [esp + 0x1c], eax		// 0x00047a37
		jmp L47a41		// 0x00047a3b
L47a3d:
		mov edx, dword ptr [esp + 0xc]		// 0x00047a3d
L47a41:
		mov ecx, dword ptr [esp + 0x178]		// 0x00047a41
		mov eax, dword ptr [ecx + eax*4]		// 0x00047a48
		mov ecx, dword ptr [ebp + 0x18]		// 0x00047a4b
		mov ecx, dword ptr [ecx + 8]		// 0x00047a4e
		mov esi, dword ptr [ecx + eax*4]		// 0x00047a51
		cmp esi, edx		// 0x00047a54
		lea ecx, [ecx + eax*4]		// 0x00047a56
		je L47e2c		// 0x00047a59
		mov dword ptr [ecx], edx		// 0x00047a5f
		fld dword ptr [esp + 0x124]		// 0x00047a61
		mov edx, dword ptr [ebp + 0x10]		// 0x00047a68
		mov ecx, dword ptr [edx + 0xe0]		// 0x00047a6b
		mov esi, dword ptr [ecx + 0x10]		// 0x00047a71
		lea eax, [eax + eax*2]		// 0x00047a74
		shl eax, 2		// 0x00047a77
		fmul dword ptr [esi + eax]		// 0x00047a7a
		add esi, eax		// 0x00047a7d
		fld dword ptr [esp + 0x12c]		// 0x00047a7f
		mov dword ptr [esp + 0x17c], eax		// 0x00047a86
		fmul dword ptr [esi + 8]		// 0x00047a8d
		faddp st(1), st(0)		// 0x00047a90
		fld dword ptr [esp + 0x128]		// 0x00047a92
		fmul dword ptr [esi + 4]		// 0x00047a99
		faddp st(1), st(0)		// 0x00047a9c
		fld dword ptr [esp + 0x138]		// 0x00047a9e
		fmul dword ptr [esi + 8]		// 0x00047aa5
		fld dword ptr [esp + 0x134]		// 0x00047aa8
		fmul dword ptr [esi + 4]		// 0x00047aaf
		faddp st(1), st(0)		// 0x00047ab2
		fld dword ptr [esp + 0x130]		// 0x00047ab4
		fmul dword ptr [esi]		// 0x00047abb
		faddp st(1), st(0)		// 0x00047abd
		fld dword ptr [esp + 0x144]		// 0x00047abf
		fmul dword ptr [esi + 8]		// 0x00047ac6
		fld dword ptr [esp + 0x140]		// 0x00047ac9
		fmul dword ptr [esi + 4]		// 0x00047ad0
		faddp st(1), st(0)		// 0x00047ad3
		fld dword ptr [esp + 0x13c]		// 0x00047ad5
		fmul dword ptr [esi]		// 0x00047adc
		faddp st(1), st(0)		// 0x00047ade
		fstp dword ptr [esp + 0x1e4]		// 0x00047ae0
		fxch st(1)		// 0x00047ae7
		fadd dword ptr [esp + 0x148]		// 0x00047ae9
		fstp dword ptr [esp + 0x10]		// 0x00047af0
		mov edx, dword ptr [esp + 0x10]		// 0x00047af4
		fld dword ptr [esp + 0x14c]		// 0x00047af8
		mov dword ptr [esp + 0x180], edx		// 0x00047aff
		fadd st(0), st(1)		// 0x00047b06
		fstp dword ptr [esp + 0x14]		// 0x00047b08
		mov eax, dword ptr [esp + 0x14]		// 0x00047b0c
		mov dword ptr [esp + 0x184], eax		// 0x00047b10
		fstp st(0)		// 0x00047b17
		fld dword ptr [esp + 0x150]		// 0x00047b19
		fadd dword ptr [esp + 0x1e4]		// 0x00047b20
		fstp dword ptr [esp + 0x18]		// 0x00047b27
		mov ecx, dword ptr [esp + 0x18]		// 0x00047b2b
		fld dword ptr [esp + 0x10]		// 0x00047b2f
		mov dword ptr [esp + 0x188], ecx		// 0x00047b33
		mov ecx, dword ptr [esp + 0x170]		// 0x00047b3a
		fcomp dword ptr [ecx + 8]		// 0x00047b41
		fnstsw ax		// 0x00047b44
		test ah, 5		// 0x00047b46
		jnp L47e2c		// 0x00047b49
		fld dword ptr [esp + 0x10]		// 0x00047b4f
		fcomp dword ptr [ecx + 0x14]		// 0x00047b53
		fnstsw ax		// 0x00047b56
		test ah, 0x41		// 0x00047b58
		je L47e2c		// 0x00047b5b
		fld dword ptr [esp + 0x14]		// 0x00047b61
		fcomp dword ptr [ecx + 0xc]		// 0x00047b65
		fnstsw ax		// 0x00047b68
		test ah, 5		// 0x00047b6a
		jnp L47e2c		// 0x00047b6d
		fld dword ptr [esp + 0x14]		// 0x00047b73
		fcomp dword ptr [ecx + 0x18]		// 0x00047b77
		fnstsw ax		// 0x00047b7a
		test ah, 0x41		// 0x00047b7c
		je L47e2c		// 0x00047b7f
		fld dword ptr [esp + 0x18]		// 0x00047b85
		fcomp dword ptr [ecx + 0x10]		// 0x00047b89
		fnstsw ax		// 0x00047b8c
		test ah, 5		// 0x00047b8e
		jnp L47e2c		// 0x00047b91
		fld dword ptr [esp + 0x18]		// 0x00047b97
		fcomp dword ptr [ecx + 0x1c]		// 0x00047b9b
		fnstsw ax		// 0x00047b9e
		test ah, 0x41		// 0x00047ba0
		je L47e2c		// 0x00047ba3
		fld dword ptr [esp + 0x10]		// 0x00047ba9
		mov eax, dword ptr [ecx + 4]		// 0x00047bad
		fsub dword ptr [ecx + 8]		// 0x00047bb0
		fmul dword ptr [ecx + 0x44]		// 0x00047bb3
		fistp qword ptr [esp + 0x38]		// 0x00047bb6
		mov edx, dword ptr [esp + 0x38]		// 0x00047bba
		mov dword ptr [esp + 0x164], edx		// 0x00047bbe
		fld dword ptr [esp + 0x14]		// 0x00047bc5
		fsub dword ptr [ecx + 0xc]		// 0x00047bc9
		fmul dword ptr [ecx + 0x48]		// 0x00047bcc
		fistp qword ptr [esp + 0x38]		// 0x00047bcf
		mov edx, dword ptr [esp + 0x38]		// 0x00047bd3
		mov dword ptr [esp + 0x38], edx		// 0x00047bd7
		mov edx, dword ptr [eax + edx*4]		// 0x00047bdb
		fld dword ptr [esp + 0x18]		// 0x00047bde
		fsub dword ptr [ecx + 0x10]		// 0x00047be2
		fmul dword ptr [ecx + 0x4c]		// 0x00047be5
		fistp qword ptr [esp + 0x168]		// 0x00047be8
		mov edi, dword ptr [esp + 0x168]		// 0x00047bef
		mov dword ptr [esp + 0x168], edi		// 0x00047bf6
		mov edi, dword ptr [eax + edi*4]		// 0x00047bfd
		lea edi, [edx + edi*2]		// 0x00047c00
		mov edx, dword ptr [esp + 0x164]		// 0x00047c03
		mov eax, dword ptr [eax + edx*4]		// 0x00047c0a
		lea eax, [eax + edi*2]		// 0x00047c0d
		mov edi, dword ptr [ecx + 0x70]		// 0x00047c10
		test byte ptr [edi + eax*4 + 3], 0x40		// 0x00047c13
		jne L47e2c		// 0x00047c18
		mov edi, dword ptr [esp + 0x17c]		// 0x00047c1e
		lea eax, [esp + 0x124]		// 0x00047c25
		push eax		// 0x00047c2c
		mov eax, dword ptr [esp + 0x178]		// 0x00047c2d
		add edi, eax		// 0x00047c34
		push edi		// 0x00047c36
		lea eax, [esp + 0x188]		// 0x00047c37
		push eax		// 0x00047c3e
		lea eax, [esp + 0x38]		// 0x00047c3f
		push eax		// 0x00047c43
		mov eax, dword ptr [esp + 0x178]		// 0x00047c44
		push eax		// 0x00047c4b
		mov eax, dword ptr [esp + 0x4c]		// 0x00047c4c
		push eax		// 0x00047c50
		push edx		// 0x00047c51
		call nxPMapCallSampleCell		// 0x00047c52
		test eax, eax		// 0x00047c57
		je L47e2c		// 0x00047c59
		mov eax, dword ptr [esp + 0x2c]		// 0x00047c5f
		test eax, eax		// 0x00047c63
		jne L47c7b		// 0x00047c65
		mov eax, dword ptr [esp + 0x30]		// 0x00047c67
		test eax, eax		// 0x00047c6b
		jne L47c7b		// 0x00047c6d
		mov eax, dword ptr [esp + 0x34]		// 0x00047c6f
		test eax, eax		// 0x00047c73
		je L47e2c		// 0x00047c75
L47c7b:
		mov eax, dword ptr [ebp + 0xc]		// 0x00047c7b
		fld dword ptr [esp + 0x30]		// 0x00047c7e
		fmul dword ptr [eax + 0x10]		// 0x00047c82
		fld dword ptr [esp + 0x2c]		// 0x00047c85
		fmul dword ptr [eax + 0xc]		// 0x00047c89
		faddp st(1), st(0)		// 0x00047c8c
		fld dword ptr [esp + 0x34]		// 0x00047c8e
		fmul dword ptr [eax + 0x14]		// 0x00047c92
		faddp st(1), st(0)		// 0x00047c95
		fld dword ptr [esp + 0x30]		// 0x00047c97
		fmul dword ptr [eax + 0x1c]		// 0x00047c9b
		fld dword ptr [esp + 0x2c]		// 0x00047c9e
		fmul dword ptr [eax + 0x18]		// 0x00047ca2
		faddp st(1), st(0)		// 0x00047ca5
		fld dword ptr [esp + 0x34]		// 0x00047ca7
		fmul dword ptr [eax + 0x20]		// 0x00047cab
		faddp st(1), st(0)		// 0x00047cae
		fld dword ptr [esp + 0x30]		// 0x00047cb0
		fmul dword ptr [eax + 4]		// 0x00047cb4
		fld dword ptr [esp + 0x34]		// 0x00047cb7
		fmul dword ptr [eax + 8]		// 0x00047cbb
		faddp st(1), st(0)		// 0x00047cbe
		fld dword ptr [esp + 0x2c]		// 0x00047cc0
		fmul dword ptr [eax]		// 0x00047cc4
		faddp st(1), st(0)		// 0x00047cc6
		fstp dword ptr [esp + 0x2c]		// 0x00047cc8
		fld st(1)		// 0x00047ccc
		fstp dword ptr [esp + 0x30]		// 0x00047cce
		fst dword ptr [esp + 0x34]		// 0x00047cd2
		fld dword ptr [esp + 0x2c]		// 0x00047cd6
		fmul dword ptr [esp + 0x2c]		// 0x00047cda
		fld st(1)		// 0x00047cde
		fmul st(0), st(2)		// 0x00047ce0
		faddp st(1), st(0)		// 0x00047ce2
		fld st(2)		// 0x00047ce4
		fmul st(0), st(3)		// 0x00047ce6
		faddp st(1), st(0)		// 0x00047ce8
		fsqrt		// 0x00047cea
		fstp dword ptr [esp + 0x38]		// 0x00047cec
		fld dword ptr [esp + 0x2c]		// 0x00047cf0
		fchs		// 0x00047cf4
		fstp dword ptr [esp + 0x20]		// 0x00047cf6
		fxch st(1)		// 0x00047cfa
		fchs		// 0x00047cfc
		fstp dword ptr [esp + 0x24]		// 0x00047cfe
		fchs		// 0x00047d02
		fstp dword ptr [esp + 0x28]		// 0x00047d04
		fld dword ptr [esp + 0x20]		// 0x00047d08
		fmul dword ptr [esp + 0x20]		// 0x00047d0c
		fld dword ptr [esp + 0x28]		// 0x00047d10
		fmul dword ptr [esp + 0x28]		// 0x00047d14
		faddp st(1), st(0)		// 0x00047d18
		fld dword ptr [esp + 0x24]		// 0x00047d1a
		fmul dword ptr [esp + 0x24]		// 0x00047d1e
		faddp st(1), st(0)		// 0x00047d22
		fsqrt		// 0x00047d24
		fld dword ptr [nxPMapZero]		// 0x00047d26
		fld st(1)		// 0x00047d2c
		fucompp		// 0x00047d2e
		fnstsw ax		// 0x00047d30
		test ah, 0x44		// 0x00047d32
		jnp L47d5b		// 0x00047d35
		fdivr dword ptr [nxPMapOne]		// 0x00047d37
		fld dword ptr [esp + 0x20]		// 0x00047d3d
		fmul st(0), st(1)		// 0x00047d41
		fstp dword ptr [esp + 0x20]		// 0x00047d43
		fld dword ptr [esp + 0x24]		// 0x00047d47
		fmul st(0), st(1)		// 0x00047d4b
		fstp dword ptr [esp + 0x24]		// 0x00047d4d
		fmul dword ptr [esp + 0x28]		// 0x00047d51
		fstp dword ptr [esp + 0x28]		// 0x00047d55
		jmp L47d5d		// 0x00047d59
L47d5b:
		fstp st(0)		// 0x00047d5b
L47d5d:
		fld dword ptr [esi + 8]		// 0x00047d5d
		fmul dword ptr [ebx + 8]		// 0x00047d60
		fld dword ptr [esi + 4]		// 0x00047d63
		fmul dword ptr [ebx + 4]		// 0x00047d66
		faddp st(1), st(0)		// 0x00047d69
		fld dword ptr [ebx]		// 0x00047d6b
		fmul dword ptr [esi]		// 0x00047d6d
		faddp st(1), st(0)		// 0x00047d6f
		fld dword ptr [esi]		// 0x00047d71
		fmul dword ptr [ebx + 0xc]		// 0x00047d73
		fld dword ptr [esi + 8]		// 0x00047d76
		fmul dword ptr [ebx + 0x14]		// 0x00047d79
		faddp st(1), st(0)		// 0x00047d7c
		fld dword ptr [esi + 4]		// 0x00047d7e
		fmul dword ptr [ebx + 0x10]		// 0x00047d81
		faddp st(1), st(0)		// 0x00047d84
		fstp dword ptr [esp + 0x190]		// 0x00047d86
		fld dword ptr [esi]		// 0x00047d8d
		fmul dword ptr [ebx + 0x18]		// 0x00047d8f
		fld dword ptr [esi + 8]		// 0x00047d92
		fmul dword ptr [ebx + 0x20]		// 0x00047d95
		faddp st(1), st(0)		// 0x00047d98
		fld dword ptr [esi + 4]		// 0x00047d9a
		fmul dword ptr [ebx + 0x1c]		// 0x00047d9d
		faddp st(1), st(0)		// 0x00047da0
		fstp dword ptr [esp + 0x194]		// 0x00047da2
		fadd dword ptr [ebx + 0x24]		// 0x00047da9
		fld dword ptr [esp + 0x190]		// 0x00047dac
		fadd dword ptr [ebx + 0x28]		// 0x00047db3
		fld dword ptr [esp + 0x194]		// 0x00047db6
		fadd dword ptr [ebx + 0x2c]		// 0x00047dbd
		fstp dword ptr [esp + 0x1f0]		// 0x00047dc0
		mov ecx, dword ptr [esp + 0x1f0]		// 0x00047dc7
		fxch st(1)		// 0x00047dce
		mov dword ptr [esp + 0x160], ecx		// 0x00047dd0
		mov ecx, dword ptr [ebp + 0x10]		// 0x00047dd7
		fstp dword ptr [esp + 0x158]		// 0x00047dda
		mov edx, dword ptr [ecx]		// 0x00047de1
		fstp dword ptr [esp + 0x15c]		// 0x00047de3
		call dword ptr [edx + 0x38]		// 0x00047dea
		mov ecx, dword ptr [ebp + 8]		// 0x00047ded
		mov esi, dword ptr [eax + 0x9c]		// 0x00047df0
		mov eax, dword ptr [ecx]		// 0x00047df6
		call dword ptr [eax + 0x38]		// 0x00047df8
		fld dword ptr [esp + 0x38]		// 0x00047dfb
		mov eax, dword ptr [eax + 0x9c]		// 0x00047dff
		fchs		// 0x00047e05
		push 0xffff		// 0x00047e07
		push 0xffff		// 0x00047e0c
		lea ecx, [esp + 0x28]		// 0x00047e11
		push ecx		// 0x00047e15
		lea edx, [esp + 0x164]		// 0x00047e16
		push edx		// 0x00047e1d
		push ecx		// 0x00047e1e
		mov ecx, dword ptr [ebp + 0x14]		// 0x00047e1f
		fstp dword ptr [esp]		// 0x00047e22
		push esi		// 0x00047e25
		push eax		// 0x00047e26
		call NxEmitContactThiscall		// 0x00047e27
L47e2c:
		mov eax, dword ptr [esp + 0x1c]		// 0x00047e2c
		inc eax		// 0x00047e30
		cmp eax, 3		// 0x00047e31
		mov dword ptr [esp + 0x1c], eax		// 0x00047e34
		jb L47a3d		// 0x00047e38
		dec dword ptr [esp + 8]		// 0x00047e3e
		jne L47a05		// 0x00047e42
L47e48:
		pop edi		// 0x00047e48
		pop esi		// 0x00047e49
		mov esp, ebp		// 0x00047e4a
		pop ebp		// 0x00047e4c
		ret		// 0x00047e4d
		}
	}

// phys_fn_001880 (0x00047e50, 64 B).
extern "C" __declspec(naked) void __cdecl nxMeshPMapContacts(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*)
	{
	__asm
		{
		mov eax, dword ptr [esp + 0x10]		// 0x00047e50
		mov ecx, dword ptr [esp + 0xc]		// 0x00047e54
		push ebx		// 0x00047e58
		push ebp		// 0x00047e59
		mov ebp, dword ptr [esp + 0x10]		// 0x00047e5a
		push esi		// 0x00047e5e
		mov esi, dword ptr [esp + 0x10]		// 0x00047e5f
		push edi		// 0x00047e63
		push eax		// 0x00047e64
		push ecx		// 0x00047e65
		push ebp		// 0x00047e66
		lea edi, [esi + 0xc]		// 0x00047e67
		push edi		// 0x00047e6a
		lea ebx, [ebp + 0xc]		// 0x00047e6b
		push esi		// 0x00047e6e
		call nxMeshPMapContact		// 0x00047e6f
		mov edx, dword ptr [esp + 0x34]		// 0x00047e74
		mov eax, dword ptr [esp + 0x30]		// 0x00047e78
		push edx		// 0x00047e7c
		push eax		// 0x00047e7d
		push esi		// 0x00047e7e
		push ebx		// 0x00047e7f
		push ebp		// 0x00047e80
		mov ebx, edi		// 0x00047e81
		call nxMeshPMapContact		// 0x00047e83
		add esp, 0x28		// 0x00047e88
		pop edi		// 0x00047e8b
		pop esi		// 0x00047e8c
		pop ebp		// 0x00047e8d
		pop ebx		// 0x00047e8e
		ret		// 0x00047e8f
		}
	}
