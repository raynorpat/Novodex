/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// ContactMeshHeightfield.cpp, sub-unit N of units/convex-mesh-gap-contract.md
// (the name is the oracle's own: 001865's __FILE__, line 328). Only 001855 is
// written so far (convex-mesh gap Task 2b), ahead of the mesh/height-field rows
// (Task 2l), because 001762 (Task 2j) and 001844 (Task 2i) call it as well as
// 001859. The .rdata blocks put 001855/001857 in an unnamed unit of their own
// before this file's; placing them here is the contract's choice (open item 5).
//
// x87: on the /arch:IA32 list. A value the listing keeps on the FPU stack is a
// `double`, a value it stores is an `NxReal`, as in Distance.cpp.

#include "NxMeshContactHelpers.h"
#include "NxTriangleDistance.h"
#include "ContactGeneration.h"
#include "X87Sqrt.h"
#include "PhysicsSDK.h"

#include <math.h>

// Listing literals used by the mesh adjacency normal helper.
static const unsigned nxTask2lEdgeOrder[3] = { 0u, 2u, 1u };
static const float nxTask2lZero = 0.0f;
static const float nxTask2lOne = 1.0f;
static const float nxTask2lThird = 0.3333333432674407959f;
static const float nxTask2lTenth = 0.1000000014901161194f;
static const float nxTask2lEpsilon = 0.5f;
static const char nxTask2lOpcodeError[] = "Opcode is not OK.";
static const char nxTask2lSourceFile[] = "ContactMeshHeightfield.cpp";

// Shared storage used by the oracle's mesh/height-field AABB callback pass.
static unsigned char nxTask2lTrianglePairContainer[16] = {};
static unsigned nxTask2lTrianglePairContainerFlag = 0;
extern "C" void nxTask2lCallContainerCtor();
extern "C" void nxTask2lCallContainerDtor();
extern "C" void nxTask2lCallAtexit();
extern "C" void nxConvexMeshCallObbCollide();
extern "C" void nxTask2lCallScratchStamp();
extern "C" void nxTask2lCallMeshComputeVertexNormals();
extern "C" void nxTask2lCallTrianglePlane();
extern "C" void nxTask2lCallChkstk();
extern "C" void* nxConvexMeshFoundationInstanceSlot;
extern "C" void* nxConvexMeshFoundationErrorSlot;
extern "C" void nxMeshMeshCallAabbTreeCollide();
extern "C" void nxMeshHeightfieldTriangleContact(const void*, const void*, const float*, const float*, unsigned, unsigned, void*);
extern "C" void __declspec(naked) nxTask2lTrianglePairContainerCleanup()
	{
	__asm {
		mov ecx, offset nxTask2lTrianglePairContainer
		call nxTask2lCallContainerDtor
		ret
	}
	}

extern "C" void nxTask2lCallCreateAdjacencies();
extern "C" void nxTask2lCallCreateEdgeList();
extern "C" void nxTask2lCallContainerResize();
extern "C" void nxTask2lCallGetDebugRenderable();
#pragma comment(linker, "/alternatename:_nxTask2lCallCreateAdjacencies=?createAdjacencies@TriangleMesh@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxTask2lCallCreateEdgeList=?createEdgeList@TriangleMesh@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxTask2lCallContainerResize=?Resize@Container@IceCore@@AAE_NI@Z")
#pragma comment(linker, "/alternatename:_nxTask2lCallGetDebugRenderable=?getDebugRenderable@PhysicsSDK@@QAEPAVNxDebugRenderable@@XZ")
#pragma comment(linker, "/alternatename:_nxTask2lCallContainerCtor=??0Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxTask2lCallContainerDtor=??1Container@IceCore@@QAE@XZ")
#pragma comment(linker, "/alternatename:_nxTask2lCallAtexit=_atexit")
#pragma comment(linker, "/alternatename:_nxTask2lCallScratchStamp=?nxScratchStamp@@YIIPAX@Z")
#pragma comment(linker, "/alternatename:_nxTask2lCallMeshComputeVertexNormals=?nxMeshComputeVertexNormals@@YAXXZ")
#pragma comment(linker, "/alternatename:_nxTask2lCallTrianglePlane=?NxTrianglePlane@@YIPAVNxPlane@@PAV1@PAXPBVNxVec3@@22@Z")
#pragma comment(linker, "/alternatename:_nxTask2lCallChkstk=__chkstk")
#pragma comment(linker, "/alternatename:_nxConvexMeshFoundationInstanceSlot=__imp_?instance@FoundationSDK@NxFoundation@@0PAV12@A")
#pragma comment(linker, "/alternatename:_nxConvexMeshFoundationErrorSlot=__imp_?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ")

// phys_fn_001861 (0x000456c0, 1693 B)
// Mesh/height-field AABB pass from the pinned listing. The inline-assembly
// body keeps the incoming register ABI and its callback seeds the shared pair Container.
extern "C" __declspec(naked) void __cdecl nxMeshHeightfieldAabbPass()
{
    __asm {
        sub      esp, 0x164
        push     esi
        mov      esi, eax
        mov      eax, dword ptr [esp + 0x16c]
        mov      ecx, dword ptr [eax + 0xe0]
        mov      edx, dword ptr [ecx + 0x28]
        mov      eax, dword ptr [esp + 0x170]
        mov      dword ptr [esi + 0x448], edx
        mov      ecx, dword ptr [eax + 0xe0]
        mov      edx, dword ptr [ecx + 0x28]
        mov      dword ptr [esi + 0x44c], edx
        mov      eax, dword ptr [ebx]
        mov      ecx, dword ptr [ebx + 4]
        mov      edx, dword ptr [ebx + 8]
        mov      dword ptr [esp + 0xa0], eax
        mov      eax, dword ptr [ebx + 0xc]
        mov      dword ptr [esp + 0xb0], ecx
        mov      ecx, dword ptr [ebx + 0x10]
        mov      dword ptr [esp + 0xa4], eax
        mov      eax, dword ptr [ebx + 0x18]
        mov      dword ptr [esp + 0xb4], ecx
        mov      ecx, dword ptr [ebx + 0x1c]
        mov      dword ptr [esp + 0xc0], edx
        mov      edx, dword ptr [ebx + 0x14]
        mov      dword ptr [esp + 0xa8], eax
        mov      eax, dword ptr [ebx + 0x24]
        mov      dword ptr [esp + 0xb8], ecx
        mov      ecx, dword ptr [ebx + 0x28]
        mov      dword ptr [esp + 0xc4], edx
        mov      edx, dword ptr [ebx + 0x20]
        mov      dword ptr [esp + 0xd0], eax
        mov      eax, dword ptr [edi]
        mov      dword ptr [esp + 0xd4], ecx
        mov      ecx, dword ptr [edi + 4]
        mov      dword ptr [esp + 0xc8], edx
        mov      edx, dword ptr [ebx + 0x2c]
        mov      dword ptr [esp + 0xe0], eax
        mov      eax, dword ptr [edi + 0xc]
        mov      dword ptr [esp + 0xf0], ecx
        mov      ecx, dword ptr [edi + 0x10]
        mov      dword ptr [esp + 0xd8], edx
        mov      edx, dword ptr [edi + 8]
        mov      dword ptr [esp + 0xe4], eax
        mov      eax, dword ptr [edi + 0x18]
        mov      dword ptr [esp + 0xf4], ecx
        mov      ecx, dword ptr [edi + 0x1c]
        mov      dword ptr [esp + 0x100], edx
        mov      edx, dword ptr [edi + 0x14]
        mov      dword ptr [esp + 0xe8], eax
        mov      eax, dword ptr [edi + 0x24]
        mov      dword ptr [esp + 0xf8], ecx
        mov      ecx, dword ptr [edi + 0x28]
        mov      dword ptr [esp + 0x104], edx
        mov      edx, dword ptr [edi + 0x20]
        mov      dword ptr [esp + 0x110], eax
        mov      dword ptr [esp + 0x114], ecx
        mov      cl, byte ptr [nxTask2lTrianglePairContainerFlag]
        mov      dword ptr [esp + 0x108], edx
        mov      edx, dword ptr [edi + 0x2c]
        mov      eax, 1
        __emit   0x84
        __emit   0xc8
        mov      dword ptr [esp + 0xcc], 0
        mov      dword ptr [esp + 0xbc], 0
        mov      dword ptr [esp + 0xac], 0
        mov      dword ptr [esp + 0xdc], 0x3f800000
        mov      dword ptr [esp + 0x118], edx
        mov      dword ptr [esp + 0x10c], 0
        mov      dword ptr [esp + 0xfc], 0
        mov      dword ptr [esp + 0xec], 0
        mov      dword ptr [esp + 0x11c], 0x3f800000
        jne      L_1004586f
        mov      edx, dword ptr [nxTask2lTrianglePairContainerFlag]
        or       edx, eax
        mov      ecx, offset nxTask2lTrianglePairContainer
        mov      dword ptr [nxTask2lTrianglePairContainerFlag], edx
        call     nxTask2lCallContainerCtor
        push     offset nxTask2lTrianglePairContainerCleanup
        call     nxTask2lCallAtexit
        add      esp, 4
L_1004586f:
        mov      eax, dword ptr [nxTask2lTrianglePairContainer + 4]
        test     eax, eax
        je       L_10045882
        mov      dword ptr [nxTask2lTrianglePairContainer + 4], 0
L_10045882:
        lea      eax, [esp + 0xe0]
        push     eax
        lea      ecx, [esp + 0xa4]
        push     ecx
        lea      edx, [esi + 0x440]
        push     edx
        lea      ecx, [esi + 0x32c]
        call     nxMeshMeshCallAabbTreeCollide
        test     al, al
        je       L_10045d5b
        test     byte ptr [esi + 0x330], 4
        je       L_10045d5b
        push     ebp
        mov      ebp, dword ptr [esi + 0x344]
        mov      esi, dword ptr [esi + 0x340]
        shr      esi, 1
        test     esi, esi
        mov      dword ptr [esp + 0x54], ebp
        je       L_10045d5a
        mov      dword ptr [esp + 0x50], esi
        jmp      L_100458e0
L_100458e0:
        mov      ecx, dword ptr [ebp]
        mov      eax, dword ptr [esp + 0x170]
        mov      eax, dword ptr [eax + 0xe0]
        mov      eax, dword ptr [eax + 0x14]
        mov      ebp, dword ptr [ebp + 4]
        lea      esi, [ebp + ebp*2]
        lea      edx, [ecx + ecx*2]
        lea      edx, [eax + edx*4]
        mov      eax, dword ptr [esp + 0x174]
        mov      eax, dword ptr [eax + 0xe0]
        mov      ebp, dword ptr [eax + 0x14]
        lea      esi, [ebp + esi*4]
        mov      dword ptr [esp + 0x58], esi
        mov      esi, dword ptr [edx]
        lea      ebp, [esi + esi*2]
        mov      esi, dword ptr [esp + 0x170]
        mov      esi, dword ptr [esi + 0xe0]
        mov      esi, dword ptr [esi + 0x10]
        lea      esi, [esi + ebp*4]
        mov      ebp, dword ptr [esi]
        mov      dword ptr [esp + 8], ebp
        mov      ebp, dword ptr [esi + 4]
        mov      dword ptr [esp + 0xc], ebp
        mov      esi, dword ptr [esi + 8]
        mov      dword ptr [esp + 0x10], esi
        mov      esi, dword ptr [esp + 0x170]
        mov      ebp, dword ptr [esi + 0xe0]
        mov      esi, dword ptr [edx + 4]
        mov      ebp, dword ptr [ebp + 0x10]
        lea      esi, [esi + esi*2]
        lea      esi, [ebp + esi*4]
        mov      ebp, dword ptr [esi]
        mov      dword ptr [esp + 0x14], ebp
        mov      ebp, dword ptr [esi + 4]
        mov      dword ptr [esp + 0x18], ebp
        mov      esi, dword ptr [esi + 8]
        mov      dword ptr [esp + 0x1c], esi
        mov      edx, dword ptr [edx + 8]
        lea      esi, [edx + edx*2]
        mov      edx, dword ptr [esp + 0x170]
        mov      edx, dword ptr [edx + 0xe0]
        mov      edx, dword ptr [edx + 0x10]
        lea      edx, [edx + esi*4]
        mov      esi, dword ptr [edx]
        mov      dword ptr [esp + 0x20], esi
        mov      esi, dword ptr [edx + 4]
        mov      dword ptr [esp + 0x24], esi
        fld      dword ptr [edx + 8]
        fld      dword ptr [esp + 8]
        fmul     dword ptr [ebx]
        fld      dword ptr [esp + 0x10]
        fmul     dword ptr [ebx + 8]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0xc]
        fmul     dword ptr [ebx + 4]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0xc]
        fmul     dword ptr [ebx + 0x10]
        fld      dword ptr [esp + 0x10]
        fmul     dword ptr [ebx + 0x14]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 8]
        fmul     dword ptr [ebx + 0xc]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x6c]
        fld      dword ptr [esp + 0x10]
        fmul     dword ptr [ebx + 0x20]
        fld      dword ptr [esp + 8]
        fmul     dword ptr [ebx + 0x18]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0xc]
        fmul     dword ptr [ebx + 0x1c]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x70]
        fadd     dword ptr [ebx + 0x24]
        fld      dword ptr [esp + 0x6c]
        fadd     dword ptr [ebx + 0x28]
        fld      dword ptr [esp + 0x70]
        fadd     dword ptr [ebx + 0x2c]
        fstp     dword ptr [esp + 0x12c]
        fxch     st(1)
        fstp     dword ptr [esp + 8]
        mov      edx, dword ptr [esp + 0x12c]
        mov      ebp, dword ptr [esp + 0x58]
        fstp     dword ptr [esp + 0xc]
        mov      dword ptr [esp + 0x10], edx
        fld      dword ptr [esp + 0x14]
        fmul     dword ptr [ebx]
        fld      dword ptr [esp + 0x1c]
        fmul     dword ptr [ebx + 8]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x18]
        fmul     dword ptr [ebx + 4]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x18]
        fmul     dword ptr [ebx + 0x10]
        fld      dword ptr [esp + 0x1c]
        fmul     dword ptr [ebx + 0x14]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x14]
        fmul     dword ptr [ebx + 0xc]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x9c]
        fld      dword ptr [esp + 0x1c]
        fmul     dword ptr [ebx + 0x20]
        fld      dword ptr [esp + 0x14]
        fmul     dword ptr [ebx + 0x18]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x18]
        fmul     dword ptr [ebx + 0x1c]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0xa0]
        fadd     dword ptr [ebx + 0x24]
        fld      dword ptr [esp + 0x9c]
        fadd     dword ptr [ebx + 0x28]
        fld      dword ptr [esp + 0xa0]
        fadd     dword ptr [ebx + 0x2c]
        fstp     dword ptr [esp + 0x15c]
        mov      edx, dword ptr [esp + 0x15c]
        fxch     st(1)
        mov      dword ptr [esp + 0x1c], edx
        fstp     dword ptr [esp + 0x14]
        fstp     dword ptr [esp + 0x18]
        fld      dword ptr [esp + 0x20]
        fmul     dword ptr [ebx]
        fld      st(1)
        fmul     dword ptr [ebx + 8]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x24]
        fmul     dword ptr [ebx + 4]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x8c]
        fld      dword ptr [esp + 0x24]
        fmul     dword ptr [ebx + 0x10]
        fld      st(1)
        fmul     dword ptr [ebx + 0x14]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x20]
        fmul     dword ptr [ebx + 0xc]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x90]
        fmul     dword ptr [ebx + 0x20]
        fld      dword ptr [esp + 0x20]
        fmul     dword ptr [ebx + 0x18]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x24]
        fmul     dword ptr [ebx + 0x1c]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x8c]
        fadd     dword ptr [ebx + 0x24]
        fld      dword ptr [esp + 0x90]
        fadd     dword ptr [ebx + 0x28]
        fxch     st(2)
        fadd     dword ptr [ebx + 0x2c]
        fstp     dword ptr [esp + 0x138]
        mov      edx, dword ptr [esp + 0x138]
        mov      dword ptr [esp + 0x28], edx
        fstp     dword ptr [esp + 0x20]
        fstp     dword ptr [esp + 0x24]
        mov      edx, dword ptr [ebp]
        mov      esi, dword ptr [eax + 0x10]
        lea      edx, [edx + edx*2]
        lea      edx, [esi + edx*4]
        mov      esi, dword ptr [edx]
        mov      dword ptr [esp + 0x2c], esi
        mov      esi, dword ptr [edx + 4]
        mov      dword ptr [esp + 0x30], esi
        mov      edx, dword ptr [edx + 8]
        mov      dword ptr [esp + 0x34], edx
        mov      esi, dword ptr [eax + 0x10]
        mov      edx, dword ptr [ebp + 4]
        lea      edx, [edx + edx*2]
        lea      edx, [esi + edx*4]
        mov      esi, dword ptr [edx]
        mov      dword ptr [esp + 0x38], esi
        mov      esi, dword ptr [edx + 4]
        mov      dword ptr [esp + 0x3c], esi
        mov      edx, dword ptr [edx + 8]
        mov      dword ptr [esp + 0x40], edx
        mov      ebp, dword ptr [ebp + 8]
        mov      eax, dword ptr [eax + 0x10]
        lea      edx, [ebp + ebp*2]
        lea      eax, [eax + edx*4]
        mov      edx, dword ptr [eax]
        mov      dword ptr [esp + 0x44], edx
        mov      edx, dword ptr [eax + 4]
        mov      dword ptr [esp + 0x48], edx
        fld      dword ptr [eax + 8]
        fld      dword ptr [esp + 0x30]
        fmul     dword ptr [edi + 4]
        fld      dword ptr [esp + 0x2c]
        fmul     dword ptr [edi]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x34]
        fmul     dword ptr [edi + 8]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x34]
        fmul     dword ptr [edi + 0x14]
        fld      dword ptr [esp + 0x30]
        fmul     dword ptr [edi + 0x10]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x2c]
        fmul     dword ptr [edi + 0xc]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x60]
        fld      dword ptr [esp + 0x30]
        fmul     dword ptr [edi + 0x1c]
        fld      dword ptr [esp + 0x34]
        fmul     dword ptr [edi + 0x20]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x2c]
        fmul     dword ptr [edi + 0x18]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x64]
        fadd     dword ptr [edi + 0x24]
        fld      dword ptr [esp + 0x60]
        fadd     dword ptr [edi + 0x28]
        fld      dword ptr [esp + 0x64]
        fadd     dword ptr [edi + 0x2c]
        fstp     dword ptr [esp + 0x150]
        mov      eax, dword ptr [esp + 0x150]
        fxch     st(1)
        mov      dword ptr [esp + 0x34], eax
        fstp     dword ptr [esp + 0x2c]
        fstp     dword ptr [esp + 0x30]
        fld      dword ptr [esp + 0x3c]
        fmul     dword ptr [edi + 4]
        fld      dword ptr [esp + 0x38]
        fmul     dword ptr [edi]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x40]
        fmul     dword ptr [edi + 8]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x40]
        fmul     dword ptr [edi + 0x14]
        fld      dword ptr [esp + 0x3c]
        fmul     dword ptr [edi + 0x10]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x38]
        fmul     dword ptr [edi + 0xc]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x84]
        fld      dword ptr [esp + 0x3c]
        fmul     dword ptr [edi + 0x1c]
        mov      ebp, dword ptr [esp + 0x54]
        fld      dword ptr [esp + 0x40]
        mov      esi, offset nxTask2lTrianglePairContainer
        fmul     dword ptr [edi + 0x20]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x38]
        fmul     dword ptr [edi + 0x18]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x88]
        fadd     dword ptr [edi + 0x24]
        fld      dword ptr [esp + 0x84]
        fadd     dword ptr [edi + 0x28]
        fld      dword ptr [esp + 0x88]
        fadd     dword ptr [edi + 0x2c]
        fstp     dword ptr [esp + 0x168]
        mov      edx, dword ptr [esp + 0x168]
        fxch     st(1)
        mov      dword ptr [esp + 0x40], edx
        fstp     dword ptr [esp + 0x38]
        mov      edx, dword ptr [esp + 0x178]
        push     edx
        fstp     dword ptr [esp + 0x40]
        mov      edx, dword ptr [esp + 0x178]
        fld      dword ptr [esp + 0x4c]
        fmul     dword ptr [edi + 4]
        fld      dword ptr [esp + 0x48]
        fmul     dword ptr [edi]
        faddp    st(1), st(0)
        fld      st(1)
        fmul     dword ptr [edi + 8]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x78]
        fld      st(0)
        fmul     dword ptr [edi + 0x14]
        fld      dword ptr [esp + 0x4c]
        fmul     dword ptr [edi + 0x10]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x48]
        fmul     dword ptr [edi + 0xc]
        faddp    st(1), st(0)
        fstp     dword ptr [esp + 0x7c]
        fld      dword ptr [esp + 0x4c]
        fmul     dword ptr [edi + 0x1c]
        fxch     st(1)
        fmul     dword ptr [edi + 0x20]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x48]
        fmul     dword ptr [edi + 0x18]
        faddp    st(1), st(0)
        fld      dword ptr [esp + 0x78]
        fadd     dword ptr [edi + 0x24]
        fld      dword ptr [esp + 0x7c]
        fadd     dword ptr [edi + 0x28]
        fxch     st(2)
        fadd     dword ptr [edi + 0x2c]
        fstp     dword ptr [esp + 0x148]
        mov      eax, dword ptr [esp + 0x148]
        mov      dword ptr [esp + 0x50], eax
        mov      eax, dword ptr [ebp + 4]
        fstp     dword ptr [esp + 0x48]
        push     eax
        push     ecx
        fstp     dword ptr [esp + 0x54]
        lea      eax, [esp + 0x38]
        push     eax
        mov      eax, dword ptr [esp + 0x180]
        lea      ecx, [esp + 0x18]
        push     ecx
        push     edx
        push     eax
        call     nxMeshHeightfieldTriangleContact
        mov      eax, dword ptr [esp + 0x6c]
        add      ebp, 8
        add      esp, 0x1c
        dec      eax
        mov      dword ptr [esp + 0x54], ebp
        mov      dword ptr [esp + 0x50], eax
        jne      L_100458e0
L_10045d5a:
        pop      ebp
L_10045d5b:
        pop      esi
        add      esp, 0x164
        ret
    }
}

// phys_fn_001865 (0x00045d70, 1937 B)
// Mesh/height-field OBB pass, transcribed from its two pinned listing fragments.
extern "C" __declspec(naked) void __cdecl nxMeshHeightfieldObbPass()
{
    __asm {
        push     ebp
        lea      ebp, [esp - 0x6c]
        sub      esp, 0x128
        push     edi
        mov      edi, eax
        mov      eax, dword ptr [ebx + 0xe0]
        fld      dword ptr [eax + 0x50]
        add      eax, 0x44
        fsub     dword ptr [eax]
        mov      ecx, dword ptr [ebx + 0xc]
        mov      edx, dword ptr [ebx + 0x10]
        fstp     dword ptr [ebp - 0x8c]
        fld      dword ptr [eax + 0x10]
        fsub     dword ptr [eax + 4]
        fstp     dword ptr [ebp - 0x88]
        fld      dword ptr [eax + 0x14]
        fsub     dword ptr [eax + 8]
        fld      dword ptr [ebp - 0x8c]
        fmul     dword ptr [nxTask2lEpsilon]
        fstp     dword ptr [ebp - 0x8c]
        fld      dword ptr [ebp - 0x88]
        fmul     dword ptr [nxTask2lEpsilon]
        fstp     dword ptr [ebp - 0x88]
        fmul     dword ptr [nxTask2lEpsilon]
        fstp     dword ptr [ebp - 0x84]
        fld      dword ptr [eax]
        fadd     dword ptr [eax + 0xc]
        fld      dword ptr [eax + 0x10]
        fadd     dword ptr [eax + 4]
        fld      dword ptr [eax + 0x14]
        fadd     dword ptr [eax + 8]
        mov      dword ptr [ebp - 0x80], ecx
        mov      ecx, dword ptr [ebx + 0x18]
        mov      dword ptr [ebp - 0x74], edx
        fstp     dword ptr [ebp + 0x24]
        mov      edx, dword ptr [ebx + 0x1c]
        fxch     st(1)
        mov      dword ptr [ebp - 0x7c], ecx
        fmul     dword ptr [nxTask2lEpsilon]
        mov      dword ptr [ebp - 0x70], edx
        fstp     dword ptr [ebp + 0x1c]
        fmul     dword ptr [nxTask2lEpsilon]
        fld      dword ptr [ebp + 0x24]
        fmul     dword ptr [nxTask2lEpsilon]
        fld      st(0)
        fmul     dword ptr [ebx + 0x14]
        fld      st(2)
        fmul     dword ptr [ebx + 0x10]
        faddp    st(1), st
        fld      dword ptr [ebp + 0x1c]
        fmul     dword ptr [ebx + 0xc]
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x2c]
        fld      dword ptr [ebp + 0x1c]
        fmul     dword ptr [ebx + 0x18]
        fld      st(1)
        fmul     dword ptr [ebx + 0x20]
        faddp    st(1), st
        fld      st(2)
        fmul     dword ptr [ebx + 0x1c]
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x30]
        fld      dword ptr [ebp + 0x1c]
        fmul     dword ptr [ebx + 0x24]
        fxch     st(1)
        fmul     dword ptr [ebx + 0x2c]
        faddp    st(1), st
        fxch     st(1)
        fmul     dword ptr [ebx + 0x28]
        faddp    st(1), st
        fld      dword ptr [ebp + 0x2c]
        fadd     dword ptr [ebx + 0x30]
        fld      dword ptr [ebp + 0x30]
        fadd     dword ptr [ebx + 0x34]
        fxch     st(2)
        fadd     dword ptr [ebx + 0x38]
        fstp     dword ptr [ebp + 4]
        mov      eax, dword ptr [ebp + 4]
        mov      dword ptr [ebp - 0x90], eax
        mov      eax, dword ptr [ebx + 0x14]
        fstp     dword ptr [ebp - 0x98]
        mov      dword ptr [ebp - 0x68], eax
        mov      eax, dword ptr [ebx + 0x20]
        fstp     dword ptr [ebp - 0x94]
        mov      ecx, dword ptr [ebx + 0x24]
        mov      edx, dword ptr [ebx + 0x28]
        mov      dword ptr [ebp - 0x6c], edx
        mov      dword ptr [ebp - 0x64], eax
        mov      eax, dword ptr [ebx + 0x2c]
        mov      dword ptr [ebp - 0x60], eax
        mov      dword ptr [ebp - 0x78], ecx
        mov      ecx, dword ptr [ebp + 0x7c]
        mov      eax, dword ptr [ecx + 0x114]
        add      ecx, 0x110
        and      eax, 0xfffffffe
        mov      dword ptr [ecx + 4], eax
        mov      edx, eax
        and      edx, 0xfffffffd
        mov      eax, edx
        mov      dword ptr [ecx + 4], edx
        and      eax, 0xffffffef
        mov      dword ptr [ecx + 4], eax
        mov      eax, dword ptr [ebp + 0x74]
        mov      edx, dword ptr [eax + 0xc]
        mov      dword ptr [ebp - 0x44], edx
        mov      edx, dword ptr [eax + 0x10]
        mov      dword ptr [ebp - 0x34], edx
        mov      edx, dword ptr [eax + 0x14]
        mov      dword ptr [ebp - 0x24], edx
        mov      edx, dword ptr [eax + 0x18]
        mov      dword ptr [ebp - 0x40], edx
        mov      edx, dword ptr [eax + 0x1c]
        mov      dword ptr [ebp - 0x30], edx
        mov      edx, dword ptr [eax + 0x20]
        mov      dword ptr [ebp - 0x20], edx
        mov      edx, dword ptr [eax + 0x24]
        mov      dword ptr [ebp - 0x3c], edx
        mov      edx, dword ptr [eax + 0x28]
        mov      dword ptr [ebp - 0x2c], edx
        mov      edx, dword ptr [eax + 0x2c]
        mov      dword ptr [ebp - 0x1c], edx
        mov      edx, dword ptr [eax + 0x30]
        mov      dword ptr [ebp - 0x14], edx
        mov      edx, dword ptr [eax + 0x34]
        mov      dword ptr [ebp - 0x10], edx
        mov      edx, dword ptr [eax + 0x38]
        mov      eax, dword ptr [eax + 0xe0]
        mov      dword ptr [ebp - 0xc], edx
        lea      edx, [ebp - 0x44]
        push     edx
        mov      edx, dword ptr [ebp + 0x7c]
        push     0
        mov      dword ptr [ebp - 0x18], 0
        mov      dword ptr [ebp - 0x28], 0
        mov      dword ptr [ebp - 0x38], 0
        mov      dword ptr [ebp - 8], 0x3f800000
        mov      eax, dword ptr [eax + 0x28]
        push     eax
        lea      eax, [ebp - 0x98]
        push     eax
        add      edx, 0x244
        push     edx
        call     nxConvexMeshCallObbCollide
        test     al, al
        jne      L_10045f8e
        mov      eax, dword ptr [nxConvexMeshFoundationInstanceSlot]
        cmp      dword ptr [eax], 0
        jne      L_10045f64
        int      3
L_10045f64:
        push     offset nxTask2lOpcodeError
        push     0
        push     0x148
        push     offset nxTask2lSourceFile
        push     4
        call     dword ptr [nxConvexMeshFoundationErrorSlot]
        add      esp, 0x14
        lea      esp, [ebp - 0xc0]
        pop      edi
        add      ebp, 0x6c
        mov      esp, ebp
        pop      ebp
        ret
L_10045f8e:
        mov      edx, dword ptr [ebp + 0x7c]
        test     byte ptr [edx + 0x114], 4
        je       L_100464f9
        mov      eax, dword ptr [edx + 0x120]
        test     eax, eax
        je       L_10045fb0
        mov      ecx, dword ptr [eax + 4]
        mov      dword ptr [ebp + 0x60], ecx
        jmp      L_10045fb7
L_10045fb0:
        mov      dword ptr [ebp + 0x60], 0
L_10045fb7:
        mov      eax, dword ptr [edx + 0x120]
        test     eax, eax
        je       L_10045fc6
        mov      eax, dword ptr [eax + 8]
        jmp      L_10045fc8
L_10045fc6:
        xor      eax, eax
L_10045fc8:
        mov      ecx, dword ptr [ebx + 0xe0]
        mov      dword ptr [ebp + 0x14], eax
        mov      eax, dword ptr [ebp + 0x74]
        mov      eax, dword ptr [eax + 0xe0]
        mov      dword ptr [ebp + 0x68], ecx
        mov      ecx, dword ptr [eax + 0x7c]
        mov      dword ptr [ebp - 0x5c], eax
        shl      ecx, 3
        mov      eax, 0x1000201
        shr      eax, cl
        mov      ecx, eax
        and      ecx, 0xff
        movzx    eax, ah
        mov      dword ptr [ebp + 0xc], ecx
        mov      ecx, edx
        mov      dword ptr [ebp + 0x28], eax
        call     nxTask2lCallScratchStamp
        mov      ecx, dword ptr [ebp + 0x68]
        mov      edx, dword ptr [ecx + 0x20]
        add      ecx, 8
        test     edx, edx
        mov      dword ptr [ebp + 8], eax
        mov      eax, dword ptr [ecx]
        mov      dword ptr [ebp + 0x44], eax
        jne      L_10046022
        call     nxTask2lCallMeshComputeVertexNormals
        mov      eax, dword ptr [ebp + 0x44]
L_10046022:
        mov      ecx, dword ptr [ebp + 0x68]
        lea      eax, [eax + eax*2]
        shl      eax, 2
        mov      dword ptr [ebp + 0x54], eax
        add      ecx, 8
        mov      ecx, dword ptr [ecx + 0x18]
        add      eax, 3
        and      eax, 0xfffffffc
        mov      dword ptr [ebp + 0x50], ecx
        call     nxTask2lCallChkstk
        mov      eax, dword ptr [ebp + 0x54]
        add      eax, 3
        and      eax, 0xfffffffc
        mov      dword ptr [ebp + 0x48], esp
        call     nxTask2lCallChkstk
        mov      eax, dword ptr [ebp + 0x44]
        test     eax, eax
        mov      edx, esp
        mov      dword ptr [ebp + 0x10], edx
        jbe      L_10046178
        mov      ecx, dword ptr [ebp + 0x50]
        mov      eax, dword ptr [ebp + 0x48]
        sub      edx, ecx
        mov      dword ptr [ebp + 0x50], edx
        mov      edx, dword ptr [ebp + 0x48]
        sub      edx, ecx
        mov      dword ptr [ebp + 0x64], edx
        mov      edx, 0xfffffff8
        sub      edx, ecx
        mov      dword ptr [ebp + 0x5c], eax
        lea      eax, [ecx + 8]
        mov      dword ptr [ebp + 0x54], edx
        mov      edx, dword ptr [ebp + 0x48]
        mov      ecx, esp
        sub      ecx, edx
        mov      dword ptr [ebp + 0x38], ecx
        mov      ecx, dword ptr [ebp + 0x44]
        mov      dword ptr [ebp + 0x58], ecx
L_10046096:
        mov      edx, dword ptr [ebp + 0x68]
        mov      ecx, dword ptr [edx + 0x10]
        add      ecx, dword ptr [ebp + 0x54]
        fld      dword ptr [ecx + eax + 8]
        add      ecx, eax
        fmul     dword ptr [edi + 8]
        mov      edx, dword ptr [ebp + 0x38]
        fld      dword ptr [ecx + 4]
        add      eax, 0xc
        fmul     dword ptr [edi + 4]
        faddp    st(1), st
        fld      dword ptr [ecx]
        fmul     dword ptr [edi]
        faddp    st(1), st
        fld      dword ptr [ecx + 8]
        fmul     dword ptr [edi + 0x14]
        fld      dword ptr [ecx + 4]
        fmul     dword ptr [edi + 0x10]
        faddp    st(1), st
        fld      dword ptr [ecx]
        fmul     dword ptr [edi + 0xc]
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x30]
        fld      dword ptr [ecx + 8]
        fmul     dword ptr [edi + 0x20]
        fld      dword ptr [ecx + 4]
        fmul     dword ptr [edi + 0x1c]
        faddp    st(1), st
        fld      dword ptr [edi + 0x18]
        fmul     dword ptr [ecx]
        mov      ecx, dword ptr [ebp + 0x5c]
        add      ecx, 0xc
        mov      dword ptr [ebp + 0x5c], ecx
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x34]
        fadd     dword ptr [edi + 0x24]
        fld      dword ptr [ebp + 0x30]
        fadd     dword ptr [edi + 0x28]
        fld      dword ptr [ebp + 0x34]
        fadd     dword ptr [edi + 0x2c]
        fstp     dword ptr [ebp + 4]
        fxch     st(1)
        fstp     dword ptr [edx + ecx - 0xc]
        mov      edx, dword ptr [ebp + 0x50]
        fstp     dword ptr [edx + eax - 0x10]
        fld      dword ptr [ebp + 4]
        fstp     dword ptr [edx + eax - 0xc]
        mov      edx, dword ptr [ebp + 0x64]
        fld      dword ptr [eax - 0xc]
        fmul     dword ptr [edi + 0x14]
        fld      dword ptr [eax - 0x10]
        fmul     dword ptr [edi + 0x10]
        faddp    st(1), st
        fld      dword ptr [eax - 0x14]
        fmul     dword ptr [edi + 0xc]
        faddp    st(1), st
        fld      dword ptr [edi + 0x18]
        fmul     dword ptr [eax - 0x14]
        fld      dword ptr [eax - 0xc]
        fmul     dword ptr [edi + 0x20]
        faddp    st(1), st
        fld      dword ptr [eax - 0x10]
        fmul     dword ptr [edi + 0x1c]
        faddp    st(1), st
        fld      dword ptr [edi]
        fmul     dword ptr [eax - 0x14]
        fld      dword ptr [eax - 0xc]
        fmul     dword ptr [edi + 8]
        faddp    st(1), st
        fld      dword ptr [eax - 0x10]
        fmul     dword ptr [edi + 4]
        faddp    st(1), st
        fstp     dword ptr [ecx - 0xc]
        fxch     st(1)
        fstp     dword ptr [ecx - 8]
        mov      ecx, dword ptr [ebp + 0x58]
        dec      ecx
        fstp     dword ptr [edx + eax - 0xc]
        mov      dword ptr [ebp + 0x58], ecx
        jne      L_10046096
L_10046178:
        mov      eax, dword ptr [ebp + 0x60]
        test     eax, eax
        je       L_100464f9
        mov      dword ptr [ebp + 0x54], eax
L_10046186:
        mov      ecx, dword ptr [ebp + 0x14]
        mov      eax, dword ptr [ecx]
        add      ecx, 4
        mov      dword ptr [ebp + 0x14], ecx
        lea      ecx, [eax + eax*2]
        mov      eax, dword ptr [ebp - 0x5c]
        mov      edx, dword ptr [eax + 0x14]
        lea      edi, [edx + ecx*4]
        mov      edx, dword ptr [eax + 0x10]
        mov      eax, dword ptr [edi]
        lea      eax, [eax + eax*2]
        fld      dword ptr [edx + eax*4 + 8]
        lea      eax, [edx + eax*4]
        fmul     dword ptr [esi + 8]
        mov      ecx, dword ptr [edi + 4]
        fld      dword ptr [esi]
        lea      ecx, [ecx + ecx*2]
        fmul     dword ptr [eax]
        lea      ecx, [edx + ecx*4]
        mov      edi, dword ptr [edi + 8]
        lea      edi, [edi + edi*2]
        faddp    st(1), st
        lea      edx, [edx + edi*4]
        fld      dword ptr [eax + 4]
        fmul     dword ptr [esi + 4]
        faddp    st(1), st
        fld      dword ptr [eax + 8]
        fmul     dword ptr [esi + 0x14]
        fld      dword ptr [eax + 4]
        fmul     dword ptr [esi + 0x10]
        faddp    st(1), st
        fld      dword ptr [esi + 0xc]
        fmul     dword ptr [eax]
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x30]
        fld      dword ptr [eax + 8]
        fmul     dword ptr [esi + 0x20]
        fld      dword ptr [esi + 0x18]
        fmul     dword ptr [eax]
        faddp    st(1), st
        fld      dword ptr [eax + 4]
        fmul     dword ptr [esi + 0x1c]
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x34]
        fadd     dword ptr [esi + 0x24]
        fld      dword ptr [ebp + 0x30]
        fadd     dword ptr [esi + 0x28]
        fld      dword ptr [ebp + 0x34]
        fadd     dword ptr [esi + 0x2c]
        fstp     dword ptr [ebp - 0xb4]
        mov      eax, dword ptr [ebp - 0xb4]
        fxch     st(1)
        mov      dword ptr [ebp - 0x20], eax
        fstp     dword ptr [ebp - 0x28]
        fstp     dword ptr [ebp - 0x24]
        fld      dword ptr [esi]
        fmul     dword ptr [ecx]
        fld      dword ptr [esi + 8]
        fmul     dword ptr [ecx + 8]
        faddp    st(1), st
        fld      dword ptr [esi + 4]
        fmul     dword ptr [ecx + 4]
        faddp    st(1), st
        fld      dword ptr [esi + 0xc]
        fmul     dword ptr [ecx]
        fld      dword ptr [ecx + 8]
        fmul     dword ptr [esi + 0x14]
        faddp    st(1), st
        fld      dword ptr [ecx + 4]
        fmul     dword ptr [esi + 0x10]
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x20]
        fld      dword ptr [esi + 0x20]
        fmul     dword ptr [ecx + 8]
        fld      dword ptr [esi + 0x1c]
        fmul     dword ptr [ecx + 4]
        faddp    st(1), st
        fld      dword ptr [esi + 0x18]
        fmul     dword ptr [ecx]
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x24]
        fadd     dword ptr [esi + 0x24]
        lea      eax, [ebp - 0x10]
        fld      dword ptr [ebp + 0x20]
        push     eax
        fadd     dword ptr [esi + 0x28]
        fld      dword ptr [ebp + 0x24]
        fadd     dword ptr [esi + 0x2c]
        fstp     dword ptr [ebp - 0x9c]
        mov      ecx, dword ptr [ebp - 0x9c]
        fxch     st(1)
        mov      dword ptr [ebp - 0x14], ecx
        fstp     dword ptr [ebp - 0x1c]
        lea      ecx, [ebp - 0x1c]
        push     ecx
        fstp     dword ptr [ebp - 0x18]
        lea      ecx, [ebp - 0x54]
        fld      dword ptr [esi]
        fmul     dword ptr [edx]
        fld      dword ptr [edx + 8]
        fmul     dword ptr [esi + 8]
        faddp    st(1), st
        fld      dword ptr [edx + 4]
        fmul     dword ptr [esi + 4]
        faddp    st(1), st
        fld      dword ptr [edx + 8]
        fmul     dword ptr [esi + 0x14]
        fld      dword ptr [edx + 4]
        fmul     dword ptr [esi + 0x10]
        faddp    st(1), st
        fld      dword ptr [esi + 0xc]
        fmul     dword ptr [edx]
        faddp    st(1), st
        fstp     dword ptr [ebp]
        fld      dword ptr [edx + 8]
        fmul     dword ptr [esi + 0x20]
        fld      dword ptr [edx + 4]
        fmul     dword ptr [esi + 0x1c]
        faddp    st(1), st
        fld      dword ptr [esi + 0x18]
        fmul     dword ptr [edx]
        faddp    st(1), st
        fstp     dword ptr [ebp + 4]
        fadd     dword ptr [esi + 0x24]
        fld      dword ptr [ebp]
        fadd     dword ptr [esi + 0x28]
        fld      dword ptr [ebp + 4]
        fadd     dword ptr [esi + 0x2c]
        fstp     dword ptr [ebp - 0xa8]
        mov      edx, dword ptr [ebp - 0xa8]
        fxch     st(1)
        mov      dword ptr [ebp - 8], edx
        fstp     dword ptr [ebp - 0x10]
        lea      edx, [ebp - 0x28]
        push     edx
        fstp     dword ptr [ebp - 0xc]
        call     nxTask2lCallTrianglePlane
        mov      eax, dword ptr [ebp + 0x44]
        xor      edi, edi
        test     eax, eax
        jbe      L_100464f0
        mov      ecx, dword ptr [ebp + 0x10]
        mov      eax, dword ptr [ebp + 0x48]
        mov      edx, dword ptr [ebp + 0x28]
        add      eax, 8
        mov      dword ptr [ebp + 0x68], eax
        lea      eax, [ecx + edx*4]
        mov      edx, dword ptr [ebp + 0xc]
        mov      dword ptr [ebp + 0x5c], eax
        lea      eax, [ecx + edx*4]
        mov      edx, dword ptr [ebp + 0x48]
        mov      dword ptr [ebp + 0x58], eax
        mov      eax, ecx
        sub      eax, edx
        mov      edx, dword ptr [ebp + 0x68]
        mov      dword ptr [ebp + 0x50], ecx
        mov      dword ptr [ebp + 0x38], eax
        jmp      L_10046350
L_10046350:
        mov      eax, dword ptr [ebp + 0x7c]
        mov      eax, dword ptr [eax + 8]
        mov      eax, dword ptr [eax + edi*4]
        sub      eax, dword ptr [ebp + 8]
        je       L_100464d0
        fld      dword ptr [ebp - 0x50]
        fmul     dword ptr [edx - 4]
        fld      dword ptr [ebp - 0x54]
        fmul     dword ptr [edx - 8]
        faddp    st(1), st
        fld      dword ptr [ebp - 0x4c]
        fmul     dword ptr [edx]
        faddp    st(1), st
        fcomp    dword ptr [nxTask2lZero]
        fnstsw   ax
        test     ah, 1
        je       L_100464d0
        mov      eax, dword ptr [ebp + 0x38]
        fld      dword ptr [ebp - 0x4c]
        fmul     dword ptr [edx + eax]
        fld      dword ptr [ebp - 0x50]
        fmul     dword ptr [ecx + 4]
        faddp    st(1), st
        fld      dword ptr [ebp - 0x54]
        fmul     dword ptr [ecx]
        faddp    st(1), st
        fadd     dword ptr [ebp - 0x48]
        fst      dword ptr [ebp - 0x58]
        fcomp    dword ptr [nxTask2lZero]
        fnstsw   ax
        test     ah, 5
        jp       L_100464d0
        mov      edx, dword ptr [ebp + 0x28]
        fld      dword ptr [ebp + edx*4 - 0x10]
        lea      eax, [ebp + edx*4 - 0x28]
        fsub     dword ptr [eax]
        mov      dword ptr [ebp + 0x64], eax
        mov      eax, dword ptr [ebp + 0xc]
        fld      dword ptr [ebp + eax*4 - 0x10]
        lea      edx, [ebp + eax*4 - 0x28]
        fsub     dword ptr [edx]
        mov      dword ptr [ebp + 0x3c], edx
        mov      edx, dword ptr [ebp + 0x28]
        fld      dword ptr [ebp + edx*4 - 0x1c]
        mov      edx, dword ptr [ebp + 0x64]
        fsub     dword ptr [edx]
        fld      dword ptr [ebp + eax*4 - 0x1c]
        mov      eax, dword ptr [ebp + 0x3c]
        fsub     dword ptr [eax]
        mov      eax, dword ptr [ebp + 0x58]
        fld      st(0)
        fmul st, st(1)
        fld      st(2)
        fmul st, st(3)
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x64]
        fld      st(0)
        fmul st, st(3)
        fld      st(2)
        fmul st, st(5)
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x60]
        fld      st(2)
        fmul st(0), st(3)
        fld      st(4)
        fmul st(0), st(5)
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x18]
        fld      dword ptr [eax]
        mov      eax, dword ptr [ebp + 0x3c]
        fsub     dword ptr [eax]
        mov      eax, dword ptr [ebp + 0x5c]
        fstp     dword ptr [ebp + 0x3c]
        fld      dword ptr [eax]
        fsub     dword ptr [edx]
        fstp     dword ptr [ebp + 0x4c]
        fld      dword ptr [ebp + 0x3c]
        fmul st(0), st(1)
        fld      dword ptr [ebp + 0x4c]
        fmul st(0), st(3)
        faddp    st(1), st
        fstp     dword ptr [ebp + 0x40]
        fstp     st(0)
        fstp     st(0)
        fld      dword ptr [ebp + 0x3c]
        fmul st(0), st(1)
        fld      dword ptr [ebp + 0x4c]
        fmul st(0), st(3)
        faddp    st(1), st
        fstp     st(2)
        fstp     st(0)
        fld      dword ptr [ebp + 0x40]
        fmul     dword ptr [ebp + 0x18]
        fld      st(1)
        fmul     dword ptr [ebp + 0x60]
        fsubp    st(1), st(0)
        fstp     dword ptr [ebp + 0x4c]
        fmul     dword ptr [ebp + 0x64]
        fld      dword ptr [ebp + 0x40]
        fmul     dword ptr [ebp + 0x60]
        fsubp    st(1), st(0)
        fst      dword ptr [ebp + 0x40]
        fadd     dword ptr [ebp + 0x4c]
        fld      dword ptr [ebp + 0x18]
        fmul     dword ptr [ebp + 0x64]
        fld      dword ptr [ebp + 0x60]
        fmul     dword ptr [ebp + 0x60]
        mov      eax, dword ptr [ebp + 0x4c]
        fsubp    st(1), st(0)
        mov      edx, dword ptr [ebp + 0x40]
        or       edx, eax
        fsubp    st(1), st(0)
        not      edx
        fstp     dword ptr [ebp + 0x64]
        and      edx, dword ptr [ebp + 0x64]
        test     edx, edx
        jns      L_100464cd
        mov      edx, dword ptr [ebp + 0x74]
        push     0xffff
        push     0xffff
        lea      eax, [ebp - 0x54]
        push     eax
        mov      eax, dword ptr [edx + 0x9c]
        push     ecx
        mov      ecx, dword ptr [ebp - 0x58]
        push     ecx
        mov      ecx, dword ptr [ebx + 0x9c]
        push     eax
        push     ecx
        mov      ecx, dword ptr [ebp + 0x78]
        call     NxEmitContact
        mov      edx, dword ptr [ebp + 0x7c]
        mov      ecx, dword ptr [ebp + 8]
        mov      eax, dword ptr [edx + 8]
        mov      dword ptr [eax + edi*4], ecx
        mov      ecx, dword ptr [ebp + 0x50]
L_100464cd:
        mov      edx, dword ptr [ebp + 0x68]
L_100464d0:
        add      dword ptr [ebp + 0x58], 0xc
        add      dword ptr [ebp + 0x5c], 0xc
        mov      eax, dword ptr [ebp + 0x44]
        inc      edi
        add      edx, 0xc
        add      ecx, 0xc
        cmp      edi, eax
        mov      dword ptr [ebp + 0x68], edx
        mov      dword ptr [ebp + 0x50], ecx
        jb       L_10046350
L_100464f0:
        dec      dword ptr [ebp + 0x54]
        jne      L_10046186
L_100464f9:
        lea      esp, [ebp - 0xc0]
        pop      edi
        add      ebp, 0x6c
        mov      esp, ebp
        pop      ebp
        ret
    }
}

// phys_fn_001869 (0x00046510, 64 B)
// Mesh/height-field entry: establish the shared register ABI, then run AABB and OBBCollider passes.
extern "C" __declspec(naked) void __cdecl nxMeshHeightfieldContact()
{
    __asm {
        mov eax, dword ptr [esp + 0x0c]
        push ebx
        push ebp
        mov ebp, dword ptr [esp + 0x0c]
        push esi
        mov esi, dword ptr [esp + 0x14]
        push edi
        push eax
        mov eax, dword ptr [esp + 0x24]
        push esi
        lea ebx, [ebp + 0x0c]
        lea edi, [esi + 0x0c]
        push ebp
        call nxMeshHeightfieldAabbPass
        mov ecx, dword ptr [esp + 0x2c]
        mov edx, dword ptr [esp + 0x28]
        push ecx
        push edx
        push esi
        mov eax, ebx
        mov esi, edi
        mov ebx, ebp
        call nxMeshHeightfieldObbPass
        add esp, 0x18
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    }
}

// phys_fn_001857 (0x00044860, 774 B)
// Smooth the seed normal with the adjacent triangle edge normal.
extern "C" __declspec(naked) void __cdecl nxMeshTriangleEdgeNormal(float*, const void*, const float*, const float*, const void*, unsigned, unsigned)
	{
	__asm {
		mov eax, dword ptr [esp + 0x10]
		mov ecx, dword ptr [eax]
		sub esp, 0x54
		push esi
		push edi
		mov edi, dword ptr [esp + 0x60]
		mov dword ptr [edi], ecx
		mov edx, dword ptr [eax + 4]
		mov dword ptr [edi + 4], edx
		mov eax, dword ptr [eax + 8]
		mov edx, dword ptr [esp + 0x78]
		mov dword ptr [edi + 8], eax
		mov eax, dword ptr [esp + 0x74]
		mov esi, nxTask2lEdgeOrder[edx*4]
		lea ecx, [eax + eax*2]
		mov eax, dword ptr [esp + 0x70]
		mov edx, dword ptr [eax + 4]
		add ecx, esi
		mov eax, dword ptr [edx + ecx*4]
		and eax, 0x1fffffff
		cmp eax, 0x1fffffff
		je L_00044b60
		lea ecx, [eax + eax*2]
		mov eax, dword ptr [esp + 0x64]
		mov edx, dword ptr [eax + 0x14]
		mov esi, dword ptr [eax + 0x10]
		mov eax, dword ptr [edx + ecx*4]
		lea edx, [edx + ecx*4]
		lea eax, [eax + eax*2]
		fld dword ptr [esi + eax*4 + 4]
		lea ecx, [esi + eax*4]
		mov eax, dword ptr [esp + 0x68]
		fmul dword ptr [eax + 4]
		fld dword ptr [eax + 8]
		fmul dword ptr [ecx + 8]
		faddp st(1), st(0)
		fld dword ptr [eax]
		fmul dword ptr [ecx]
		faddp st(1), st(0)
		fld dword ptr [eax + 0x14]
		fmul dword ptr [ecx + 8]
		fld dword ptr [ecx + 4]
		fmul dword ptr [eax + 0x10]
		faddp st(1), st(0)
		fld dword ptr [eax + 0xc]
		fmul dword ptr [ecx]
		faddp st(1), st(0)
		fld dword ptr [eax + 0x20]
		fmul dword ptr [ecx + 8]
		fld dword ptr [ecx]
		fmul dword ptr [eax + 0x18]
		faddp st(1), st(0)
		fld dword ptr [ecx + 4]
		fmul dword ptr [eax + 0x1c]
		faddp st(1), st(0)
		fstp dword ptr [esp + 0x1c]
		fxch st(1)
		fadd dword ptr [eax + 0x24]
		fstp dword ptr [esp + 0x20]
		mov ecx, dword ptr [esp + 0x20]
		mov dword ptr [esp + 0x38], ecx
		fadd dword ptr [eax + 0x28]
		fld dword ptr [esp + 0x1c]
		fadd dword ptr [eax + 0x2c]
		fld st(1)
		fstp dword ptr [esp + 0x3c]
		fst dword ptr [esp + 0x40]
		mov ecx, dword ptr [edx + 4]
		fld dword ptr [eax + 8]
		lea ecx, [ecx + ecx*2]
		fmul dword ptr [esi + ecx*4 + 8]
		lea ecx, [esi + ecx*4]
		fld dword ptr [ecx + 4]
		fmul dword ptr [eax + 4]
		faddp st(1), st(0)
		fld dword ptr [ecx]
		fmul dword ptr [eax]
		faddp st(1), st(0)
		fld dword ptr [ecx]
		fmul dword ptr [eax + 0xc]
		fld dword ptr [ecx + 8]
		fmul dword ptr [eax + 0x14]
		faddp st(1), st(0)
		fld dword ptr [ecx + 4]
		fmul dword ptr [eax + 0x10]
		faddp st(1), st(0)
		fld dword ptr [ecx]
		fmul dword ptr [eax + 0x18]
		fld dword ptr [eax + 0x20]
		fmul dword ptr [ecx + 8]
		faddp st(1), st(0)
		fld dword ptr [eax + 0x1c]
		fmul dword ptr [ecx + 4]
		faddp st(1), st(0)
		fstp dword ptr [esp + 0x1c]
		fxch st(1)
		fadd dword ptr [eax + 0x24]
		fstp dword ptr [esp + 8]
		mov ecx, dword ptr [esp + 8]
		fadd dword ptr [eax + 0x28]
		fld dword ptr [esp + 0x1c]
		fadd dword ptr [eax + 0x2c]
		fst dword ptr [esp + 0x10]
		fld st(1)
		mov dword ptr [esp + 0x44], ecx
		fstp dword ptr [esp + 0x48]
		fstp dword ptr [esp + 0x4c]
		mov edx, dword ptr [edx + 8]
		fld dword ptr [eax + 8]
		lea edx, [edx + edx*2]
		fmul dword ptr [esi + edx*4 + 8]
		lea ecx, [esi + edx*4]
		fld dword ptr [eax]
		fmul dword ptr [ecx]
		faddp st(1), st(0)
		fld dword ptr [ecx + 4]
		fmul dword ptr [eax + 4]
		faddp st(1), st(0)
		fld dword ptr [eax + 0xc]
		fmul dword ptr [ecx]
		fld dword ptr [ecx + 8]
		fmul dword ptr [eax + 0x14]
		faddp st(1), st(0)
		fld dword ptr [ecx + 4]
		fmul dword ptr [eax + 0x10]
		faddp st(1), st(0)
		fld dword ptr [eax + 0x20]
		fmul dword ptr [ecx + 8]
		fld dword ptr [eax + 0x1c]
		fmul dword ptr [ecx + 4]
		faddp st(1), st(0)
		fld dword ptr [ecx]
		fmul dword ptr [eax + 0x18]
		faddp st(1), st(0)
		fstp dword ptr [esp + 0x1c]
		fxch st(1)
		fadd dword ptr [eax + 0x24]
		fstp dword ptr [esp + 0x2c]
		fadd dword ptr [eax + 0x28]
		fld dword ptr [esp + 0x1c]
		fadd dword ptr [eax + 0x2c]
		fld dword ptr [esp + 0x2c]
		fsub dword ptr [esp + 0x20]
		fstp dword ptr [esp + 0x14]
		fxch st(1)
		fsub st(0), st(4)
		fstp dword ptr [esp + 0x18]
		fsub st(0), st(2)
		fstp dword ptr [esp + 0x1c]
		fld dword ptr [esp + 8]
		fsub dword ptr [esp + 0x20]
		fstp dword ptr [esp + 0x20]
		fsub st(0), st(2)
		fstp dword ptr [esp + 0x24]
		fld dword ptr [esp + 0x10]
		fsub st(0), st(1)
		fstp st(2)
		fstp st(0)
		fld dword ptr [esp + 0x24]
		fmul dword ptr [esp + 0x1c]
		fld st(1)
		fmul dword ptr [esp + 0x18]
		fsubp st(1), st(0)
		fstp dword ptr [esp + 8]
		fmul dword ptr [esp + 0x14]
		fld dword ptr [esp + 0x1c]
		fmul dword ptr [esp + 0x20]
		fsubp st(1), st(0)
		fstp dword ptr [esp + 0xc]
		mov eax, dword ptr [esp + 0xc]
		fld dword ptr [esp + 0x18]
		fmul dword ptr [esp + 0x20]
		fld dword ptr [esp + 0x24]
		mov dword ptr [esp + 0x24], eax
		fmul dword ptr [esp + 0x14]
		fsubp st(1), st(0)
		fstp dword ptr [esp + 0x10]
		fld dword ptr [esp + 8]
		fld dword ptr [esp + 8]
		mov ecx, dword ptr [esp + 0x10]
		fmul dword ptr [esp + 8]
		mov dword ptr [esp + 0x28], ecx
		fld dword ptr [esp + 0x10]
		fmul dword ptr [esp + 0x10]
		faddp st(1), st(0)
		fld dword ptr [esp + 0xc]
		fmul dword ptr [esp + 0xc]
		faddp st(1), st(0)
		fsqrt
		fstp dword ptr [esp + 0x6c]
		fld nxTask2lZero
		fld dword ptr [esp + 0x6c]
		fucompp
		fnstsw ax
		test ah, 0x44
		jnp L_00044aef
		fstp st(0)
		fld nxTask2lOne
		fdiv dword ptr [esp + 0x6c]
		fstp dword ptr [esp + 0x6c]
		fld dword ptr [esp + 8]
		fmul dword ptr [esp + 0x6c]
		fld dword ptr [esp + 0xc]
		fmul dword ptr [esp + 0x6c]
		fstp dword ptr [esp + 0x24]
		fld dword ptr [esp + 0x10]
		fmul dword ptr [esp + 0x6c]
		fstp dword ptr [esp + 0x28]
L_00044aef:
		fadd dword ptr [edi]
		fst dword ptr [edi]
		fld dword ptr [esp + 0x24]
		fadd dword ptr [edi + 4]
		fstp dword ptr [esp + 0x6c]
		fld dword ptr [esp + 0x28]
		mov edx, dword ptr [esp + 0x6c]
		fadd dword ptr [edi + 8]
		mov dword ptr [edi + 4], edx
		fst dword ptr [edi + 8]
		fld st(0)
		fmul st(0), st(1)
		fld dword ptr [esp + 0x6c]
		fmul dword ptr [esp + 0x6c]
		faddp st(1), st(0)
		fld st(2)
		fmul st(0), st(3)
		faddp st(1), st(0)
		fsqrt
		fld nxTask2lZero
		fld st(1)
		fucompp
		fnstsw ax
		test ah, 0x44
		jnp L_00044b5a
		fdivr nxTask2lOne
		fld st(0)
		fmul st(0), st(3)
		fstp dword ptr [edi]
		fld dword ptr [esp + 0x6c]
		fmul st(0), st(1)
		fstp dword ptr [edi + 4]
		fmul st(0), st(1)
		fstp dword ptr [edi + 8]
		pop edi
		pop esi
		fstp st(0)
		fstp st(0)
		add esp, 0x54
		ret
L_00044b5a:
		fstp st(0)
		fstp st(0)
		fstp st(0)
L_00044b60:
		pop edi
		pop esi
		add esp, 0x54
		ret
	}
	}

// ContactAccumulator storage owned by the reconstructed 001872. The oracle
// uses the same packed layout at 0x10123d8c; keeping this block contiguous
// makes the callback independently differential-testable.
extern "C" {
unsigned nxMeshContactCount = 0;
float nxMeshContactSums[3] = {};
float nxMeshContactNormalAndMaterial[50] = {};
float nxMeshContactVertices[96] = {};
float nxMeshContactNormals[96] = {};
}

// phys_fn_001872 (0x000466e0, 146 B)
// Accumulate the normal unconditionally, then append one contact while the
// global contact count is below the oracle's fixed limit of 32.
extern "C" __declspec(naked) void __stdcall nxMeshContactAccumulate(
	unsigned, unsigned, unsigned, const unsigned*, const float*)
	{
	__asm {
		mov edx, dword ptr [esp + 14h]
		fld nxMeshContactSums
		fadd dword ptr [edx]
		mov ecx, nxMeshContactCount
		cmp ecx, 20h
		fstp nxMeshContactSums
		fld dword ptr nxMeshContactSums[4]
		fadd dword ptr [edx + 4]
		fstp dword ptr nxMeshContactSums[4]
		fld dword ptr nxMeshContactSums[8]
		fadd dword ptr [edx + 8]
		fstp dword ptr nxMeshContactSums[8]
		jae done
		fld dword ptr [esp + 0ch]
		push esi
		mov esi, dword ptr [esp + 14h]
		push edi
		mov edi, dword ptr [esi]
		lea eax, [ecx + ecx*2]
		shl eax, 2
		mov dword ptr nxMeshContactVertices[eax], edi
		mov edi, dword ptr [esi + 4]
		mov dword ptr nxMeshContactVertices[eax + 4], edi
		mov esi, dword ptr [esi + 8]
		mov dword ptr nxMeshContactVertices[eax + 8], esi
		mov esi, dword ptr [edx]
		mov dword ptr nxMeshContactNormals[eax], esi
		mov esi, dword ptr [edx + 4]
		mov dword ptr nxMeshContactNormals[eax + 4], esi
		mov edx, dword ptr [edx + 8]
		fstp dword ptr nxMeshContactNormalAndMaterial[ecx*4]
		inc ecx
		pop edi
		mov dword ptr nxMeshContactNormals[eax + 8], edx
		mov nxMeshContactCount, ecx
		pop esi
	done:
		ret 14h
	}
}

// phys_fn_001855 (0x00044510, 837 B)
// The segment s0..s1 against the plane through the triangle edge e0..e1 that
// contains `axis` (the triangle's normal at the callers): the plane normal is
// (e1 - e0) x axis, formed with x and z in registers and y stored, then
// normalised unless its length is exactly zero (`fucompp; test ah, 0x44; jnp`,
// so a NaN length normalises; x and z scaled wide, y from the stored copy).
// The segment's two signed distances are formed in registers, the start's x
// term narrowed into the caller's second argument slot (0x000445fa); their
// product > 0 is a miss. The segment's direction keeps x in a register and
// stores y and z (z squared as wide * narrowed, 0x0004465d); a zero
// denominator is a miss. The crossing, the line parameter narrowed at
// 0x000446f8, is written to `hit` with x from the wide product. Then, in the
// plane of the normal's two smallest components (the dominant axis is chosen on
// the absolute values, 0x00044738..0x00044798, ties to the later axis), t is
// the distance along `axis` from the edge's line; it is stored before it is
// tested, so a negative t is a miss that still writes it. `hit` is moved back
// by t * axis, x wide, each component stored with `fst` and kept, and the hit
// is on the edge when (e1 - hit) . (e0 - hit) < 0 over those wide values,
// summed z, x, y.
//
// Under the in-step word 0x0f7f this row does not reproduce every last bit in
// C++. What is narrowed are the wide intermediates themselves -- the normal's x
// and z and the direction's x and z, which the listing keeps in st(n) -- at two
// points: as the operands of each square root, which reach the X87Sqrt.h helper
// through qwords (64 bits cut to 53), and in their reuse after the root, where
// MSVC reloads them from the same 8-byte slots to scale them by 1/root (and dirX
// again for the denominator and the crossing). The oracle's `fsqrt` is inline and
// keeps all of them. The differential measures the effect and pins it (215 words
// with mixed-exponent draws); written as leaves that recompute the wide values
// after the call, the count rose, because MSVC folds the recomputation into the
// spilled copy. The remedy X87Sqrt.h records for 001760 -- the span as one x87
// assembly block -- would be two blocks of about 131 instructions here, and is
// not judged proportionate.
//
// The result is 0 or 1 in the whole of eax, as the listing returns it (`xor eax,
// eax` at 0x000446d7 and 0x00044798, `mov eax, 1` at 0x0004477d and 0x0004484b):
// 001844's naked caller tests eax, not al (`test eax, eax` at 0x000430b2; a
// bool left the upper bytes undefined, convex-mesh gap Task 2i).
__declspec(noinline) NxU32 __cdecl NxSegmentTriangleEdge(const NxReal* e0, const NxReal* e1,
	const NxReal* axis, const NxReal* s0, const NxReal* s1, NxReal* t, NxReal* hit)
	{
	NxReal edge[3];
	edge[0] = (NxReal) ((double) e1[0] - e0[0]);
	edge[1] = (NxReal) ((double) e1[1] - e0[1]);
	edge[2] = (NxReal) ((double) e1[2] - e0[2]);

	const double nxWide = (double) edge[1] * axis[2] - (double) edge[2] * axis[1];
	const NxReal ny = (NxReal) ((double) edge[2] * axis[0] - (double) edge[0] * axis[2]);
	const double nzWide = (double) edge[0] * axis[1] - (double) edge[1] * axis[0];
	NxReal normal[3];
	normal[1] = ny;
	normal[0] = (NxReal) nxWide;
	normal[2] = (NxReal) nzWide;
	const double normalLength = x87FsqrtDot3(nzWide, nzWide, ny, ny, nxWide, nxWide);
	if(normalLength != 0.0)
		{
		const double inverse = 1.0f / normalLength;
		normal[0] = (NxReal) (nxWide * inverse);
		normal[1] = (NxReal) (ny * inverse);
		normal[2] = (NxReal) (nzWide * inverse);
		}

	const NxReal d = (NxReal) (((double) normal[0] * e0[0] + (double) normal[2] * e0[2])
		+ (double) normal[1] * e0[1]);
	const NxReal startX = (NxReal) ((double) normal[0] * s0[0]);
	const double distance1 = (((double) normal[2] * s1[2] + (double) normal[1] * s1[1])
		+ (double) normal[0] * s1[0]) - d;
	const double distance0 = (((double) normal[2] * s0[2] + (double) normal[1] * s0[1])
		+ startX) - d;
	if(distance1 * distance0 > 0.0)
		return 0;

	double dirX = (double) s1[0] - s0[0];
	NxReal dirY = (NxReal) ((double) s1[1] - s0[1]);
	const double dirZWide = (double) s1[2] - s0[2];
	NxReal dirZ = (NxReal) dirZWide;
	const double dirLength = x87FsqrtDot3(dirZWide, dirZ, dirY, dirY, dirX, dirX);
	if(dirLength != 0.0)
		{
		const double inverse = 1.0f / dirLength;
		dirX = dirX * inverse;
		dirY = (NxReal) (dirY * inverse);
		dirZ = (NxReal) (dirZ * inverse);
		}
	const double denominator = ((double) dirZ * normal[2] + (double) dirY * normal[1])
		+ dirX * normal[0];
	if(denominator == 0.0)
		return 0;

	const NxReal along = (NxReal) (((double) d - (((double) normal[2] * s0[2]
		+ (double) normal[1] * s0[1]) + startX)) / denominator);
	const double stepX = dirX * along;
	const NxReal stepY = (NxReal) ((double) dirY * along);
	const NxReal stepZ = (NxReal) ((double) dirZ * along);
	hit[0] = (NxReal) (stepX + s0[0]);
	hit[1] = (NxReal) ((double) stepY + s0[1]);
	hit[2] = (NxReal) ((double) stepZ + s0[2]);

	NxReal magnitude[3];
	magnitude[0] = (NxReal) fabs(normal[0]);
	magnitude[1] = (NxReal) fabs(normal[1]);
	magnitude[2] = (NxReal) fabs(normal[2]);
	const int major = magnitude[0] > magnitude[1] ? 0 : 1;
	int i0;
	int i1;
	if(magnitude[major] < magnitude[2])
		{
		i0 = 0;
		i1 = 1;
		}
	else if(major == 0)
		{
		i0 = 1;
		i1 = 2;
		}
	else
		{
		i0 = 0;
		i1 = 2;
		}

	const double q = (((double) hit[i1] - e0[i1]) * edge[i0] - ((double) hit[i0] - e0[i0]) * edge[i1])
		/ ((double) axis[i1] * edge[i0] - (double) axis[i0] * edge[i1]);
	*t = (NxReal) q;
	if(q < 0.0)
		return 0;

	const double backX = q * axis[0];
	const NxReal backY = (NxReal) (q * axis[1]);
	const NxReal backZ = (NxReal) (q * axis[2]);
	const double px = (double) hit[0] - backX;
	hit[0] = (NxReal) px;
	const double py = (double) hit[1] - backY;
	hit[1] = (NxReal) py;
	const double pz = (double) hit[2] - backZ;
	hit[2] = (NxReal) pz;

	const double inside = (((double) e1[2] - pz) * ((double) e0[2] - pz)
		+ ((double) e0[0] - px) * ((double) e1[0] - px))
		+ ((double) e1[1] - py) * ((double) e0[1] - py);
	return inside < 0.0 ? 1u : 0u;
	}

// phys_fn_001859 (0x00044b70, 2892 B)
// Mesh/height-field triangle-pair callback. The x87 sequence, stack frame,
// lazy adjacency and edge-list creation, edge rejection, contact accumulation,
// debug-line callback and contact emission are transcribed from the pinned row.
// Its caller also seeds ESI with the shared triangle-pair Container at image
// address 0x00123ce4; callers and tests must preserve that register input.
extern "C" __declspec(naked) void __cdecl nxMeshHeightfieldTriangleContact(
	const void*, const void*, const float*, const float*, unsigned, unsigned, void*)
	{
	__asm {
        mov eax, dword ptr [esp + 4]
        sub esp, 0x150
        push ebx
        push ebp
        push edi
        mov edi, dword ptr [eax + 0xe0]
        mov eax, dword ptr [edi + 0x84]
        mov ebp, 1
        cmp eax, ebp
        jne L_10044b9c
        mov dword ptr [esp + 0x78], 0
        jmp L_10044bc1
L_10044b9c:
        test eax, eax
        jne L_10044ba7
        mov ecx, edi
        call nxTask2lCallCreateAdjacencies
L_10044ba7:
        mov eax, dword ptr [edi + 0x84]
        test eax, eax
        jne L_10044bbd
        mov dword ptr [edi + 0x84], ebp
        mov dword ptr [esp + 0x78], eax
        jmp L_10044bc1
L_10044bbd:
        mov dword ptr [esp + 0x78], eax
L_10044bc1:
        mov ebx, dword ptr [esp + 0x164]
        mov edi, dword ptr [ebx + 0xe0]
        mov eax, dword ptr [edi + 0x84]
        cmp eax, ebp
        jne L_10044be2
        mov dword ptr [esp + 0x7c], 0
        jmp L_10044c07
L_10044be2:
        test eax, eax
        jne L_10044bed
        mov ecx, edi
        call nxTask2lCallCreateAdjacencies
L_10044bed:
        mov eax, dword ptr [edi + 0x84]
        test eax, eax
        jne L_10044c03
        mov dword ptr [edi + 0x84], ebp
        mov dword ptr [esp + 0x7c], eax
        jmp L_10044c07
L_10044c03:
        mov dword ptr [esp + 0x7c], eax
L_10044c07:
        mov ecx, dword ptr [esp + 0x160]
        mov edi, dword ptr [ecx + 0xe0]
        mov eax, dword ptr [edi + 0x88]
        test eax, eax
        jne L_10044c25
        mov ecx, edi
        call nxTask2lCallCreateEdgeList
L_10044c25:
        mov eax, dword ptr [esp + 0x170]
        lea edx, [eax + eax*2]
        mov eax, dword ptr [edi + 0x88]
        mov ecx, dword ptr [eax + 0xc]
        mov edi, dword ptr [ebx + 0xe0]
        mov eax, dword ptr [edi + 0x88]
        test eax, eax
        lea edx, [ecx + edx*4]
        mov dword ptr [esp + 0xf4], edx
        jne L_10044c59
        mov ecx, edi
        call nxTask2lCallCreateEdgeList
L_10044c59:
        mov ecx, dword ptr [edi + 0x88]
        mov edi, dword ptr [esp + 0x168]
        mov eax, dword ptr [esp + 0x174]
        fld dword ptr [edi + 0x18]
        fsub dword ptr [edi]
        lea edx, [eax + eax*2]
        mov eax, dword ptr [ecx + 0xc]
        fld dword ptr [edi + 0x1c]
        fsub dword ptr [edi + 4]
        lea ecx, [eax + edx*4]
        fld dword ptr [edi + 0x20]
        mov dword ptr [esp + 0xf0], ecx
        fsub dword ptr [edi + 8]
        lea ecx, [edi + 8]
        fld dword ptr [edi + 0xc]
        fsub dword ptr [edi]
        fstp dword ptr [esp + 0x30]
        fld dword ptr [edi + 0x10]
        fsub dword ptr [edi + 4]
        fstp dword ptr [esp + 0x34]
        fld dword ptr [edi + 0x14]
        fsub dword ptr [ecx]
        fld dword ptr [esp + 0x34]
        fmul st, st(2)
        fld st(1)
        fmul st, st(4)
        fsubp st(1), st
        fstp dword ptr [esp + 0xc]
        mov edx, dword ptr [esp + 0xc]
        mov dword ptr [esp + 0xd8], edx
        fmul st, st(3)
        fxch st(1)
        fmul dword ptr [esp + 0x30]
        fsubp st(1), st
        fstp dword ptr [esp + 0x10]
        mov eax, dword ptr [esp + 0x10]
        mov dword ptr [esp + 0xdc], eax
        fmul dword ptr [esp + 0x30]
        fld dword ptr [esp + 0x34]
        fmul st, st(2)
        fsubp st(1), st
        fstp st(1)
        fst dword ptr [esp + 0xe0]
        fld st(0)
        fmul st, st(1)
        fld dword ptr [esp + 0x10]
        fmul dword ptr [esp + 0x10]
        faddp st(1), st
        fld dword ptr [esp + 0xc]
        fmul dword ptr [esp + 0xc]
        faddp st(1), st
        fsqrt
        fld dword ptr [nxTask2lZero]
        fld st(1)
        fucompp
        fnstsw ax
        test ah, 0x44
        jnp L_10044d4a
        fdivr dword ptr [nxTask2lOne]
        fld dword ptr [esp + 0xc]
        fmul st, st(1)
        fstp dword ptr [esp + 0xd8]
        fld dword ptr [esp + 0x10]
        fmul st, st(1)
        fstp dword ptr [esp + 0xdc]
        fxch st(1)
        fmul st, st(1)
        fstp dword ptr [esp + 0xe0]
        jmp L_10044d4c
L_10044d4a:
        fstp st(0)
L_10044d4c:
        fstp st(0)
        mov ebp, dword ptr [esp + 0x16c]
        fld dword ptr [ebp + 0x18]
        fsub dword ptr [ebp]
        fld dword ptr [ebp + 0x1c]
        fsub dword ptr [ebp + 4]
        fld dword ptr [ebp + 0x20]
        fsub dword ptr [ebp + 8]
        fld dword ptr [ebp + 0xc]
        fsub dword ptr [ebp]
        fstp dword ptr [esp + 0x30]
        fld dword ptr [ebp + 0x10]
        fsub dword ptr [ebp + 4]
        fstp dword ptr [esp + 0x34]
        fld dword ptr [ebp + 0x14]
        fsub dword ptr [ebp + 8]
        fld dword ptr [esp + 0x34]
        fmul st, st(2)
        fld st(1)
        fmul st, st(4)
        fsubp st(1), st
        fstp dword ptr [esp + 0xc]
        mov edx, dword ptr [esp + 0xc]
        mov dword ptr [esp + 0xe4], edx
        fmul st, st(3)
        fxch st(1)
        fmul dword ptr [esp + 0x30]
        fsubp st(1), st
        fstp dword ptr [esp + 0x10]
        mov eax, dword ptr [esp + 0x10]
        mov dword ptr [esp + 0xe8], eax
        fmul dword ptr [esp + 0x30]
        fld dword ptr [esp + 0x34]
        fmul st, st(2)
        fsubp st(1), st
        fstp st(1)
        fst dword ptr [esp + 0xec]
        fld dword ptr [esp + 0xc]
        fmul dword ptr [esp + 0xc]
        fld st(1)
        fmul st, st(2)
        faddp st(1), st
        fld dword ptr [esp + 0x10]
        fmul dword ptr [esp + 0x10]
        faddp st(1), st
        fsqrt
        fld dword ptr [nxTask2lZero]
        fld st(1)
        fucompp
        fnstsw ax
        test ah, 0x44
        jnp L_10044e22
        fdivr dword ptr [nxTask2lOne]
        fld dword ptr [esp + 0xc]
        fmul st, st(1)
        fstp dword ptr [esp + 0xe4]
        fld dword ptr [esp + 0x10]
        fmul st, st(1)
        fstp dword ptr [esp + 0xe8]
        fxch st(1)
        fmul st, st(1)
        fstp dword ptr [esp + 0xec]
        jmp L_10044e24
L_10044e22:
        fstp st(0)
L_10044e24:
        fstp st(0)
        mov dword ptr [esp + 0xa4], ecx
        fld dword ptr [edi]
        fadd dword ptr [edi + 0xc]
        fld dword ptr [edi + 0x10]
        fadd dword ptr [edi + 4]
        fld dword ptr [edi + 0x14]
        fadd dword ptr [ecx]
        fstp dword ptr [esp + 0x88]
        fxch st(1)
        fadd dword ptr [edi + 0x18]
        fstp dword ptr [esp + 0x58]
        fadd dword ptr [edi + 0x1c]
        fstp dword ptr [esp + 0x5c]
        fld dword ptr [esp + 0x88]
        fadd dword ptr [edi + 0x20]
        fld dword ptr [esp + 0x58]
        fmul dword ptr [nxTask2lThird]
        fld dword ptr [esp + 0x5c]
        fmul dword ptr [nxTask2lThird]
        fxch st(2)
        fmul dword ptr [nxTask2lThird]
        fstp dword ptr [esp + 0x88]
        mov edx, dword ptr [esp + 0x88]
        mov dword ptr [esp + 0x140], edx
        fstp dword ptr [esp + 0x138]
        xor edx, edx
        mov dword ptr [esp + 0x74], edx
        fstp dword ptr [esp + 0x13c]
        fld dword ptr [ebp + 0xc]
        fadd dword ptr [ebp]
        fld dword ptr [ebp + 0x10]
        fadd dword ptr [ebp + 4]
        fld dword ptr [ebp + 0x14]
        fadd dword ptr [ebp + 8]
        fstp dword ptr [esp + 0x88]
        fxch st(1)
        fadd dword ptr [ebp + 0x18]
        fstp dword ptr [esp + 0x58]
        fadd dword ptr [ebp + 0x1c]
        fstp dword ptr [esp + 0x5c]
        fld dword ptr [esp + 0x88]
        fadd dword ptr [ebp + 0x20]
        fld dword ptr [esp + 0x58]
        fmul dword ptr [nxTask2lThird]
        fld dword ptr [esp + 0x5c]
        fmul dword ptr [nxTask2lThird]
        fxch st(2)
        fmul dword ptr [nxTask2lThird]
        fstp dword ptr [esp + 0x88]
        mov eax, dword ptr [esp + 0x88]
        mov dword ptr [esp + 0x14c], eax
        fstp dword ptr [esp + 0x144]
        fstp dword ptr [esp + 0x148]
L_10044f15:
        mov eax, dword ptr [esp + 0xf4]
        cmp dword ptr [eax + edx*4], 0
        jns L_1004569a
        lea eax, [edx + 1]
        cmp eax, 3
        mov dword ptr [esp + 0x64], eax
        jne L_10044f3e
        mov dword ptr [esp + 0x64], 0
        mov eax, dword ptr [esp + 0x64]
L_10044f3e:
        fld dword ptr [ecx - 8]
        mov ebx, dword ptr [ecx - 4]
        mov ecx, dword ptr [ecx]
        lea eax, [eax + eax*2]
        fld dword ptr [edi + eax*4 + 4]
        lea eax, [edi + eax*4]
        mov dword ptr [esp + 0x54], ecx
        mov ecx, dword ptr [eax]
        mov eax, dword ptr [eax + 8]
        mov dword ptr [esp + 0xc], ecx
        fld dword ptr [esp + 0xc]
        fsub st, st(2)
        mov dword ptr [esp + 0x50], ebx
        fld st(1)
        mov dword ptr [esp + 0x14], eax
        fsub dword ptr [esp + 0x50]
        fld dword ptr [esp + 0x14]
        fsub dword ptr [esp + 0x54]
        fstp dword ptr [esp + 0xbc]
        fld st(1)
        fmul st, st(2)
        fld dword ptr [esp + 0xbc]
        fmul dword ptr [esp + 0xbc]
        faddp st(1), st
        fld st(1)
        fmul st, st(2)
        faddp st(1), st
        fsqrt
        fld dword ptr [nxTask2lZero]
        fld st(1)
        fucompp
        fnstsw ax
        test ah, 0x44
        jnp L_10044fcf
        fdivr dword ptr [nxTask2lTenth]
        fxch st(2)
        fmul st, st(2)
        fxch st(2)
        fxch st(1)
        fmul st, st(1)
        fxch st(1)
        fld dword ptr [esp + 0xbc]
        fmul st, st(1)
        fstp dword ptr [esp + 0xbc]
L_10044fcf:
        fstp st(0)
        mov ecx, dword ptr [esp + 0x170]
        fxch st(3)
        push edx
        fsub st, st(1)
        mov edx, dword ptr [esp + 0x7c]
        push ecx
        push edx
        fstp dword ptr [esp + 0x58]
        lea eax, [esp + 0xe4]
        fld dword ptr [esp + 0x5c]
        push eax
        fsub st, st(3)
        mov eax, dword ptr [esp + 0x170]
        mov edx, dword ptr [eax + 0xe0]
        lea ecx, [eax + 0xc]
        fstp dword ptr [esp + 0x60]
        push ecx
        fld dword ptr [esp + 0x68]
        push edx
        fsub dword ptr [esp + 0xd4]
        lea eax, [esp + 0x48]
        push eax
        fstp dword ptr [esp + 0x70]
        fadd dword ptr [esp + 0x28]
        fstp dword ptr [esp + 0x28]
        fxch st(1)
        fadd st, st(1)
        fstp dword ptr [esp + 0x2c]
        fstp st(0)
        fld dword ptr [esp + 0xd8]
        fadd dword ptr [esp + 0x30]
        fstp dword ptr [esp + 0x30]
        call nxMeshTriangleEdgeNormal
        lea eax, [ebp + 8]
        add esp, 0x1c
        xor ebx, ebx
        mov ebp, eax
        mov dword ptr [esp + 0x8c], ebx
        mov dword ptr [esp + 0x6c], ebp
        lea ebx, [ebx]
L_10045060:
        mov ecx, dword ptr [esp + 0xf0]
        cmp dword ptr [ecx + ebx*4], 0
        jns L_10045670
        lea edi, [ebx + 1]
        cmp edi, 3
        jne L_1004507b
        xor edi, edi
L_1004507b:
        mov edx, dword ptr [ebp - 8]
        mov eax, dword ptr [ebp - 4]
        mov ecx, dword ptr [ebp]
        mov dword ptr [esp + 0x1c], eax
        mov eax, dword ptr [esp + 0x16c]
        mov dword ptr [esp + 0x18], edx
        lea edx, [edi + edi*2]
        lea eax, [eax + edx*4]
        mov edx, dword ptr [eax + 4]
        mov dword ptr [esp + 0x20], ecx
        mov ecx, dword ptr [eax]
        mov eax, dword ptr [eax + 8]
        push ebx
        mov dword ptr [esp + 0x28], ecx
        mov ecx, dword ptr [esp + 0x178]
        push ecx
        mov dword ptr [esp + 0x30], edx
        mov edx, dword ptr [esp + 0x84]
        push edx
        mov dword ptr [esp + 0x38], eax
        lea eax, [esp + 0xf0]
        push eax
        mov eax, dword ptr [esp + 0x174]
        mov edx, dword ptr [eax + 0xe0]
        lea ecx, [eax + 0xc]
        push ecx
        push edx
        lea eax, [esp + 0x120]
        push eax
        call nxMeshTriangleEdgeNormal
        fld dword ptr [esp + 0x124]
        fmul dword ptr [esp + 0x4c]
        add esp, 0x1c
        fld dword ptr [esp + 0x110]
        fmul dword ptr [esp + 0x38]
        faddp st(1), st
        fld dword ptr [esp + 0x10c]
        fmul dword ptr [esp + 0x34]
        faddp st(1), st
        fcomp dword ptr [nxTask2lTenth]
        fnstsw ax
        test ah, 0x41
        je L_10045669
        fld dword ptr [esp + 0x24]
        fsub dword ptr [esp + 0x18]
        fld dword ptr [esp + 0x28]
        fsub dword ptr [esp + 0x1c]
        fld dword ptr [esp + 0x2c]
        fsub dword ptr [esp + 0x20]
        fstp dword ptr [esp + 0xd4]
        fld st(1)
        fmul st, st(2)
        fld dword ptr [esp + 0xd4]
        fmul dword ptr [esp + 0xd4]
        faddp st(1), st
        fld st(1)
        fmul st, st(2)
        faddp st(1), st
        fsqrt
        fld dword ptr [nxTask2lZero]
        fld st(1)
        fucompp
        fnstsw ax
        test ah, 0x44
        jnp L_10045190
        fdivr dword ptr [nxTask2lTenth]
        fxch st(2)
        fmul st, st(2)
        fxch st(2)
        fxch st(1)
        fmul st, st(1)
        fxch st(1)
        fld dword ptr [esp + 0xd4]
        fmul st, st(1)
        fstp dword ptr [esp + 0xd4]
L_10045190:
        fstp st(0)
        lea ecx, [esp + 0xa8]
        fld dword ptr [esp + 0x18]
        push ecx
        fsub st, st(2)
        lea edx, [esp + 0x4c]
        push edx
        lea eax, [esp + 0x2c]
        fstp dword ptr [esp + 0x20]
        push eax
        fld dword ptr [esp + 0x28]
        lea ecx, [esp + 0x24]
        fsub st, st(1)
        push ecx
        lea edx, [esp + 0x10c]
        push edx
        fstp dword ptr [esp + 0x30]
        lea eax, [esp + 0x20]
        fld dword ptr [esp + 0x34]
        push eax
        fsub dword ptr [esp + 0xec]
        lea ecx, [esp + 0x64]
        push ecx
        mov dword ptr [esp + 0x118], 0
        fstp dword ptr [esp + 0x3c]
        mov dword ptr [esp + 0x11c], 0x3f800000
        fxch st(1)
        mov dword ptr [esp + 0x120], 0
        fadd dword ptr [esp + 0x40]
        fstp dword ptr [esp + 0x40]
        fadd dword ptr [esp + 0x44]
        fstp dword ptr [esp + 0x44]
        fld dword ptr [esp + 0xf0]
        fadd dword ptr [esp + 0x48]
        fstp dword ptr [esp + 0x48]
        call NxSegmentTriangleEdge
        add esp, 0x1c
        test eax, eax
        je L_10045669
        mov eax, dword ptr [esp + 0x74]
        mov ecx, dword ptr [esp + 0x64]
        cmp eax, ecx
        mov dword ptr [esp + 0x68], eax
        mov dword ptr [esp + 0x70], ecx
        jbe L_1004524c
        mov dword ptr [esp + 0x68], ecx
        mov dword ptr [esp + 0x70], eax
L_1004524c:
        cmp ebx, edi
        mov dword ptr [esp + 0x90], ebx
        mov dword ptr [esp + 0x94], edi
        jbe L_1004526c
        mov dword ptr [esp + 0x90], edi
        mov dword ptr [esp + 0x94], ebx
L_1004526c:
        mov edx, dword ptr [esi + 4]
        mov eax, dword ptr [esi + 8]
        shr edx, 2
        test edx, edx
        je L_100452d9
        lea esp, [esp]
L_10045280:
        mov ecx, dword ptr [eax]
        mov edi, dword ptr [eax + 4]
        add eax, 4
        mov ebx, dword ptr [eax + 4]
        add eax, 4
        mov ebp, dword ptr [eax + 4]
        add eax, 4
        mov dword ptr [esp + 0xf8], ebp
        mov ebp, dword ptr [esp + 0x68]
        dec edx
        add eax, 4
        cmp ecx, ebp
        jne L_100452ca
        cmp edi, dword ptr [esp + 0x70]
        jne L_100452ca
        cmp ebx, dword ptr [esp + 0x90]
        jne L_100452ca
        mov ecx, dword ptr [esp + 0x94]
        cmp dword ptr [esp + 0xf8], ecx
        je L_1004565e
L_100452ca:
        test edx, edx
        jne L_10045280
        mov ebp, dword ptr [esp + 0x6c]
        mov ebx, dword ptr [esp + 0x8c]
L_100452d9:
        fld dword ptr [esp + 0xc]
        lea edx, [esp + 0x114]
        fsub dword ptr [esp + 0x4c]
        push edx
        lea eax, [esp + 0x1c]
        push eax
        fstp dword ptr [esp + 0x134]
        lea ecx, [esp + 0x134]
        fld dword ptr [esp + 0x18]
        push ecx
        fsub dword ptr [esp + 0x5c]
        lea edx, [esp + 0x58]
        push edx
        lea eax, [esp + 0x130]
        fstp dword ptr [esp + 0x140]
        push eax
        fld dword ptr [esp + 0x28]
        lea ecx, [esp + 0xd4]
        fsub dword ptr [esp + 0x68]
        push ecx
        fstp dword ptr [esp + 0x14c]
        fld dword ptr [esp + 0x3c]
        fsub dword ptr [esp + 0x30]
        fstp dword ptr [esp + 0x12c]
        fld dword ptr [esp + 0x40]
        fsub dword ptr [esp + 0x34]
        fstp dword ptr [esp + 0x130]
        fld dword ptr [esp + 0x44]
        fsub dword ptr [esp + 0x38]
        fstp dword ptr [esp + 0x134]
        call NxLineLineClosestPoints
        fld dword ptr [esp + 0xd8]
        mov edx, dword ptr [esp + 0xd8]
        fsub dword ptr [esp + 0x138]
        mov eax, dword ptr [esp + 0xdc]
        fld dword ptr [esp + 0xdc]
        mov ecx, dword ptr [esp + 0xe0]
        fsub dword ptr [esp + 0x13c]
        add esp, 0x18
        fld dword ptr [esp + 0xc8]
        mov dword ptr [esp + 0xa8], edx
        fsub dword ptr [esp + 0x128]
        mov dword ptr [esp + 0xac], eax
        mov dword ptr [esp + 0xb0], ecx
        fld st(0)
        fmul st, st(1)
        fld st(2)
        fmul st, st(3)
        faddp st(1), st
        fld st(3)
        fmul st, st(4)
        faddp st(1), st
        fsqrt
        fstp st(3)
        fstp st(0)
        fstp st(0)
        fstp dword ptr [esp + 0x48]
        fld dword ptr [esp + 0x18]
        fsub dword ptr [esp + 0x24]
        fld dword ptr [esp + 0x1c]
        fsub dword ptr [esp + 0x28]
        fld dword ptr [esp + 0x20]
        fsub dword ptr [esp + 0x2c]
        fld dword ptr [esp + 0x4c]
        fsub dword ptr [esp + 0xc]
        fstp dword ptr [esp + 0x58]
        fld dword ptr [esp + 0x50]
        fsub dword ptr [esp + 0x10]
        fstp dword ptr [esp + 0x5c]
        fld dword ptr [esp + 0x54]
        fsub dword ptr [esp + 0x14]
        fld dword ptr [esp + 0x5c]
        fmul st, st(2)
        fld st(1)
        fmul st, st(4)
        fsubp st(1), st
        fstp dword ptr [esp + 0x98]
        fmul st, st(3)
        fxch st(1)
        fmul dword ptr [esp + 0x58]
        mov edx, dword ptr [esp + 0x98]
        mov dword ptr [esp + 0x3c], edx
        fsubp st(1), st
        fstp dword ptr [esp + 0x9c]
        mov eax, dword ptr [esp + 0x9c]
        mov dword ptr [esp + 0x40], eax
        fmul dword ptr [esp + 0x58]
        fld dword ptr [esp + 0x5c]
        fmul st, st(2)
        fsubp st(1), st
        fstp st(1)
        fst dword ptr [esp + 0x44]
        fld dword ptr [esp + 0x98]
        fmul dword ptr [esp + 0x98]
        fld st(1)
        fmul st, st(2)
        faddp st(1), st
        fld dword ptr [esp + 0x9c]
        fmul dword ptr [esp + 0x9c]
        faddp st(1), st
        fsqrt
        fld dword ptr [nxTask2lZero]
        fld st(1)
        fucompp
        fnstsw ax
        test ah, 0x44
        jnp L_100454ba
        fdivr dword ptr [nxTask2lOne]
        fld dword ptr [esp + 0x98]
        fmul st, st(1)
        fstp dword ptr [esp + 0x3c]
        fld dword ptr [esp + 0x9c]
        fmul st, st(1)
        fstp dword ptr [esp + 0x40]
        fxch st(1)
        fmul st, st(1)
        fstp dword ptr [esp + 0x44]
        jmp L_100454bc
L_100454ba:
        fstp st(0)
L_100454bc:
        fstp st(0)
        fld dword ptr [esp + 0x3c]
        fmul dword ptr [esp + 0x30]
        fld dword ptr [esp + 0x44]
        fmul dword ptr [esp + 0x38]
        faddp st(1), st
        fld dword ptr [esp + 0x40]
        fmul dword ptr [esp + 0x34]
        faddp st(1), st
        fcomp dword ptr [nxTask2lZero]
        fnstsw ax
        test ah, 1
        jne L_10045505
        fld dword ptr [esp + 0x3c]
        fchs
        fstp dword ptr [esp + 0x3c]
        fld dword ptr [esp + 0x40]
        fchs
        fstp dword ptr [esp + 0x40]
        fld dword ptr [esp + 0x44]
        fchs
        fstp dword ptr [esp + 0x44]
L_10045505:
        mov ecx, dword ptr [esi + 4]
        cmp ecx, dword ptr [esi]
        jne L_10045515
        push 1
        mov ecx, esi
        call nxTask2lCallContainerResize
L_10045515:
        mov edx, dword ptr [esi + 4]
        mov eax, dword ptr [esi + 8]
        mov ecx, dword ptr [esp + 0x68]
        mov dword ptr [eax + edx*4], ecx
        mov edx, dword ptr [esi + 4]
        mov ecx, dword ptr [esi]
        inc edx
        mov eax, edx
        cmp eax, ecx
        mov dword ptr [esi + 4], edx
        jne L_1004553a
        push 1
        mov ecx, esi
        call nxTask2lCallContainerResize
L_1004553a:
        mov edx, dword ptr [esi + 4]
        mov eax, dword ptr [esi + 8]
        mov ecx, dword ptr [esp + 0x70]
        mov dword ptr [eax + edx*4], ecx
        mov edx, dword ptr [esi + 4]
        mov ecx, dword ptr [esi]
        inc edx
        mov eax, edx
        cmp eax, ecx
        mov dword ptr [esi + 4], edx
        jne L_1004555f
        push 1
        mov ecx, esi
        call nxTask2lCallContainerResize
L_1004555f:
        mov edx, dword ptr [esi + 4]
        mov eax, dword ptr [esi + 8]
        mov ecx, dword ptr [esp + 0x90]
        mov dword ptr [eax + edx*4], ecx
        mov edx, dword ptr [esi + 4]
        mov ecx, dword ptr [esi]
        inc edx
        mov eax, edx
        cmp eax, ecx
        mov dword ptr [esi + 4], edx
        jne L_10045587
        push 1
        mov ecx, esi
        call nxTask2lCallContainerResize
L_10045587:
        mov edx, dword ptr [esi + 4]
        mov eax, dword ptr [esi + 8]
        mov ecx, dword ptr [esp + 0x94]
        mov dword ptr [eax + edx*4], ecx
        inc dword ptr [esi + 4]
        mov ecx, dword ptr [PhysicsSDK::instance]
        call nxTask2lCallGetDebugRenderable
        fld dword ptr [esp + 0x3c]
        fmul dword ptr [esp + 0x48]
        push 0xff00ff00
        fld dword ptr [esp + 0x44]
        lea ecx, [esp + 0x84]
        fmul dword ptr [esp + 0x4c]
        push ecx
        fld dword ptr [esp + 0x4c]
        lea ecx, [esp + 0xb0]
        fmul dword ptr [esp + 0x50]
        push ecx
        mov ecx, eax
        fstp dword ptr [esp + 0x164]
        fxch st(1)
        fadd dword ptr [esp + 0xb4]
        fstp dword ptr [esp + 0x8c]
        fadd dword ptr [esp + 0xb8]
        fstp dword ptr [esp + 0x90]
        fld dword ptr [esp + 0x164]
        fadd dword ptr [esp + 0xbc]
        fstp dword ptr [esp + 0x94]
        mov edx, dword ptr [eax]
        call dword ptr [edx + 0x20]
        fld dword ptr [esp + 0x48]
        push 0xffff
        fchs
        push 0xffff
        lea edx, [esp + 0x44]
        push edx
        lea eax, [esp + 0xb4]
        push eax
        mov eax, dword ptr [esp + 0x170]
        push ecx
        mov ecx, dword ptr [esp + 0x178]
        fstp dword ptr [esp]
        mov edx, dword ptr [ecx + 0x9c]
        mov ecx, dword ptr [eax + 0x9c]
        push edx
        push ecx
        mov ecx, dword ptr [esp + 0x194]
        call NxEmitContact
        jmp L_10045669
L_1004565e:
        mov ebp, dword ptr [esp + 0x6c]
        mov ebx, dword ptr [esp + 0x8c]
L_10045669:
        mov edi, dword ptr [esp + 0x168]
L_10045670:
        inc ebx
        add ebp, 0xc
        cmp ebx, 3
        mov dword ptr [esp + 0x8c], ebx
        mov dword ptr [esp + 0x6c], ebp
        jb L_10045060
        mov ecx, dword ptr [esp + 0xa4]
        mov edx, dword ptr [esp + 0x74]
        mov ebp, dword ptr [esp + 0x16c]
L_1004569a:
        inc edx
        add ecx, 0xc
        cmp edx, 3
        mov dword ptr [esp + 0x74], edx
        mov dword ptr [esp + 0xa4], ecx
        jb L_10044f15
        pop edi
        pop ebp
        pop ebx
        add esp, 0x150
        ret
	}
	}
