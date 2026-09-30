// ContactMeshMesh.cpp, sub-unit N of units/convex-mesh-gap-contract.md.

#include "NxPhysicsSDK.h"

extern "C" void nxMeshMeshCallAabbTreeCollide();
#pragma comment(linker, "/alternatename:_nxMeshMeshCallAabbTreeCollide=?Collide@AABBTreeCollider@Opcode@@QAE_NAAUBVTCache@2@PBVMatrix4x4@IceMaths@@1@Z")

// phys_fn_001870 (0x00046550, 394 B)
// Matrix-B [MESH][MESH] entry. It prepares two world OBBs in the scene's
// OBBCache and calls the vendored OBBCollider::Collide with each mesh's model.
extern "C" __declspec(naked) bool __cdecl nxOverlapMeshMesh(const void*, const void*, void*)
{
	__asm
	{
		sub esp, 80h
		push esi
		mov esi, dword ptr [esp + 90h]
		push edi
		mov edi, dword ptr [esi + 330h]
		or edi, 1
		mov dword ptr [esi + 330h], edi
		mov edx, edi
		and edx, 0FFFFFFFDh
		mov dword ptr [esi + 330h], edx
		mov eax, edx
		lea ecx, [esi + 32Ch]
		and eax, 0FFFFFFEFh
		mov dword ptr [ecx + 4], eax
		mov eax, dword ptr [esp + 8Ch]
		mov edx, dword ptr [eax + 0Ch]
		mov dword ptr [esp + 48h], edx
		mov edx, dword ptr [eax + 10h]
		mov dword ptr [esp + 58h], edx
		mov edx, dword ptr [eax + 14h]
		mov dword ptr [esp + 68h], edx
		mov edx, dword ptr [eax + 18h]
		mov dword ptr [esp + 4Ch], edx
		mov edx, dword ptr [eax + 1Ch]
		mov dword ptr [esp + 5Ch], edx
		mov edx, dword ptr [eax + 20h]
		mov dword ptr [esp + 6Ch], edx
		mov edx, dword ptr [eax + 24h]
		mov dword ptr [esp + 50h], edx
		mov edx, dword ptr [eax + 28h]
		mov dword ptr [esp + 60h], edx
		mov edx, dword ptr [eax + 2Ch]
		mov dword ptr [esp + 70h], edx
		mov edx, dword ptr [eax + 30h]
		mov dword ptr [esp + 78h], edx
		mov edx, dword ptr [eax + 34h]
		mov dword ptr [esp + 7Ch], edx
		mov edx, dword ptr [eax + 38h]
		mov dword ptr [esp + 80h], edx
		mov edx, dword ptr [esp + 90h]
		mov edi, dword ptr [edx + 0Ch]
		mov dword ptr [esp + 8], edi
		mov edi, dword ptr [edx + 10h]
		mov dword ptr [esp + 18h], edi
		mov edi, dword ptr [edx + 14h]
		mov dword ptr [esp + 28h], edi
		mov edi, dword ptr [edx + 18h]
		mov eax, dword ptr [eax + 0E0h]
		mov dword ptr [esp + 0Ch], edi
		mov edi, dword ptr [edx + 1Ch]
		mov dword ptr [esp + 1Ch], edi
		mov edi, dword ptr [edx + 20h]
		mov dword ptr [esp + 2Ch], edi
		mov edi, dword ptr [edx + 24h]
		mov dword ptr [esp + 10h], edi
		mov edi, dword ptr [edx + 28h]
		mov dword ptr [esp + 20h], edi
		mov edi, dword ptr [edx + 2Ch]
		mov dword ptr [esp + 30h], edi
		mov edi, dword ptr [edx + 30h]
		mov dword ptr [esp + 38h], edi
		mov edi, dword ptr [edx + 34h]
		mov dword ptr [esp + 3Ch], edi
		mov edi, dword ptr [edx + 38h]
		mov dword ptr [esp + 74h], 0
		mov dword ptr [esp + 64h], 0
		mov dword ptr [esp + 54h], 0
		mov dword ptr [esp + 84h], 3F800000h
		mov dword ptr [esp + 40h], edi
		mov dword ptr [esp + 34h], 0
		mov dword ptr [esp + 24h], 0
		mov dword ptr [esp + 14h], 0
		mov dword ptr [esp + 44h], 3F800000h
		mov eax, dword ptr [eax + 28h]
		mov dword ptr [esi + 448h], eax
		mov edx, dword ptr [edx + 0E0h]
		mov eax, dword ptr [edx + 28h]
		lea edx, [esp + 8]
		mov dword ptr [esi + 44Ch], eax
		push edx
		lea eax, [esp + 4Ch]
		push eax
		lea edx, [esi + 440h]
		push edx
		call nxMeshMeshCallAabbTreeCollide
		 test al, al
		je nxMeshMeshOverlap_false
		test byte ptr [esi + 330h], 4
		je nxMeshMeshOverlap_false
		pop edi
		mov al, 1
		pop esi
		add esp, 80h
		ret
	nxMeshMeshOverlap_false:
		pop edi
		xor al, al
		pop esi
		add esp, 80h
		ret
	}
}


// Transform pointers installed by the caller before phys_fn_001874.
extern "C" unsigned* nxTask2lSphereMatrixA = 0;
extern "C" unsigned* nxTask2lSphereMatrixB = 0;
static const float nxTask2lSphereOne = 1.0f;
static const float nxTask2lSphereEpsilon = 0.00001f;
extern "C" void nxTask2lCallSphereCtor(void*, unsigned);
extern "C" void nxTask2lCallSphereSetRadius(float);
extern "C" void nxTask2lCallSphereDtor();
#pragma comment(linker, "/alternatename:_nxTask2lCallSphereCtor=??0SphereShape@@QAE@PAXI@Z")
#pragma comment(linker, "/alternatename:_nxTask2lCallSphereSetRadius=?nxSphereSetRadius@SphereShape@@QAEXM@Z")
#pragma comment(linker, "/alternatename:_nxTask2lCallSphereDtor=?nxSphereCallbackDtor@SphereShape@@QAEXXZ")
extern "C" void __stdcall nxMeshContactAccumulate(unsigned, unsigned, unsigned, const unsigned*, const float*);

// phys_fn_001874 (0x00046780, 801 B)
// Mesh/mesh sphere callback, transcribed from the pinned x86 listing.
extern "C" __declspec(naked) bool __cdecl nxMeshMeshSphereCallback(const float*, const float*)
{
    __asm {
        sub esp, 0x1f4
        push esi
        push 0xffffff
        push 0
        lea ecx, [esp + 0x38]
        call nxTask2lCallSphereCtor
        push -1
        push 0
        lea ecx, [esp + 0x11c]
        call nxTask2lCallSphereCtor
        mov esi, dword ptr [esp + 0x200]
        mov ecx, dword ptr [esp + 0x1fc]
        mov eax, dword ptr [esi]
        fld dword ptr [ecx]
        fld dword ptr [ecx + 4]
        mov dword ptr [esp + 8], eax
        mov eax, dword ptr [esi + 8]
        fld dword ptr [ecx + 8]
        fld st(0)
        mov dword ptr [esp + 0x10], eax
        mov eax, dword ptr [nxTask2lSphereMatrixA]
        fmul dword ptr [eax + 0x14]
        mov edx, dword ptr [esi + 4]
        fld st(2)
        mov dword ptr [esp + 0xc], edx
        fmul dword ptr [eax + 0x10]
        faddp st(1), st(0)
        fld st(3)
        fmul dword ptr [eax + 0xc]
        faddp st(1), st(0)
        fstp dword ptr [esp + 0x14]
        fld st(0)
        fmul dword ptr [eax + 0x20]
        fld st(2)
        fmul dword ptr [eax + 0x1c]
        faddp st(1), st(0)
        fld st(3)
        fmul dword ptr [eax + 0x18]
        faddp st(1), st(0)
        fstp dword ptr [esp + 0x18]
        fmul dword ptr [eax + 0x2c]
        fxch st(1)
        fmul dword ptr [eax + 0x28]
        faddp st(1), st(0)
        fxch st(1)
        fmul dword ptr [eax + 0x24]
        faddp st(1), st(0)
        fld dword ptr [esp + 0x14]
        fadd dword ptr [eax + 0x30]
        fld dword ptr [esp + 0x18]
        fadd dword ptr [eax + 0x34]
        fxch st(2)
        fadd dword ptr [eax + 0x38]
        mov eax, dword ptr [nxTask2lSphereMatrixB]
        fstp dword ptr [esp + 0x1c]
        mov edx, dword ptr [esp + 0x1c]
        mov dword ptr [esp + 0x68], edx
        fstp dword ptr [esp + 0x60]
        fstp dword ptr [esp + 0x64]
        fld dword ptr [esp + 0x10]
        fmul dword ptr [eax + 0x14]
        fld dword ptr [esp + 0xc]
        fmul dword ptr [eax + 0x10]
        faddp st(1), st(0)
        fld dword ptr [esp + 8]
        fmul dword ptr [eax + 0xc]
        faddp st(1), st(0)
        fld dword ptr [esp + 0x10]
        fmul dword ptr [eax + 0x20]
        fld dword ptr [esp + 0xc]
        fmul dword ptr [eax + 0x1c]
        faddp st(1), st(0)
        fld dword ptr [esp + 8]
        fmul dword ptr [eax + 0x18]
        faddp st(1), st(0)
        fstp dword ptr [esp + 0x18]
        fld dword ptr [esp + 0x10]
        fmul dword ptr [eax + 0x2c]
        fld dword ptr [esp + 0xc]
        fmul dword ptr [eax + 0x28]
        faddp st(1), st(0)
        fld dword ptr [esp + 8]
        fmul dword ptr [eax + 0x24]
        mov edx, dword ptr [ecx + 0xc]
        mov dword ptr [esp + 4], edx
        faddp st(1), st(0)
        fstp dword ptr [esp + 0x1c]
        fadd dword ptr [eax + 0x30]
        fld dword ptr [esp + 0x18]
        fadd dword ptr [eax + 0x34]
        fld dword ptr [esp + 0x1c]
        fadd dword ptr [eax + 0x38]
        fstp dword ptr [esp + 0x1c]
        mov eax, dword ptr [esp + 0x1c]
        fxch st(1)
        mov dword ptr [esp + 0x14c], eax
        fstp dword ptr [esp + 0x144]
        fstp dword ptr [esp + 0x148]
        fld dword ptr [ecx + 0x10]
        fcomp dword ptr [esp + 4]
        fnstsw ax
        test ah, 0x41
        jne L_100468e0
        mov eax, dword ptr [ecx + 0x10]
        mov dword ptr [esp + 4], eax
L_100468e0:
        fld dword ptr [ecx + 0x14]
        fcomp dword ptr [esp + 4]
        fnstsw ax
        test ah, 0x41
        jne L_100468f5
        mov ecx, dword ptr [ecx + 0x14]
        mov dword ptr [esp + 4], ecx
L_100468f5:
        mov edx, dword ptr [esp + 4]
        push edx
        lea ecx, [esp + 0x34]
        call nxTask2lCallSphereSetRadius
        fld dword ptr [esi + 0x10]
        mov eax, dword ptr [esi + 0xc]
        mov dword ptr [esp + 4], eax
        fcomp dword ptr [esp + 4]
        fnstsw ax
        test ah, 0x41
        jne L_1004691f
        mov ecx, dword ptr [esi + 0x10]
        mov dword ptr [esp + 4], ecx
L_1004691f:
        fld dword ptr [esi + 0x14]
        fcomp dword ptr [esp + 4]
        fnstsw ax
        test ah, 0x41
        jne L_10046934
        mov edx, dword ptr [esi + 0x14]
        mov dword ptr [esp + 4], edx
L_10046934:
        mov eax, dword ptr [esp + 4]
        push eax
        lea ecx, [esp + 0x118]
        call nxTask2lCallSphereSetRadius
        fld dword ptr [esp + 0x144]
        fsub dword ptr [esp + 0x60]
        pop esi
        fstp dword ptr [esp + 4]
        fld dword ptr [esp + 0x144]
        fsub dword ptr [esp + 0x60]
        fstp dword ptr [esp + 8]
        fld dword ptr [esp + 0x148]
        fsub dword ptr [esp + 0x64]
        fst dword ptr [esp + 0xc]
        fmul dword ptr [esp + 0xc]
        fld dword ptr [esp + 8]
        fmul dword ptr [esp + 8]
        faddp st(1), st(0)
        fld dword ptr [esp + 4]
        fmul dword ptr [esp + 4]
        faddp st(1), st(0)
        fstp dword ptr [esp]
        fld dword ptr [esp + 0x10c]
        fadd dword ptr [esp + 0x1f0]
        fst dword ptr [esp + 0x1c]
        fmul dword ptr [esp + 0x1c]
        fcomp dword ptr [esp]
        fnstsw ax
        test ah, 0x41
        jne L_10046a83
        fld dword ptr [esp]
        fcomp dword ptr [nxTask2lSphereEpsilon]
        fnstsw ax
        test ah, 0x41
        jp L_100469e0
        lea ecx, [esp + 0x110]
        call nxTask2lCallSphereDtor
        lea ecx, [esp + 0x2c]
        call nxTask2lCallSphereDtor
        xor al, al
        add esp, 0x1f4
        ret
L_100469e0:
        fld dword ptr [esp]
        mov eax, dword ptr [esp + 0xc8]
        fsqrt
        lea ecx, [esp + 4]
        push ecx
        lea edx, [esp + 0x14]
        push edx
        push ecx
        mov ecx, dword ptr [esp + 0x1b8]
        fld dword ptr [nxTask2lSphereOne]
        fdiv st(0), st(1)
        fld dword ptr [esp + 0x10]
        fmul st(0), st(1)
        fstp dword ptr [esp + 0x10]
        fld dword ptr [esp + 0x14]
        fmul st(0), st(1)
        fstp dword ptr [esp + 0x14]
        fld dword ptr [esp + 0x18]
        fmul st(0), st(1)
        fstp dword ptr [esp + 0x18]
        fstp st(0)
        fld dword ptr [esp + 0x118]
        fmul dword ptr [esp + 0x10]
        fld dword ptr [esp + 0x118]
        fmul dword ptr [esp + 0x14]
        fld dword ptr [esp + 0x118]
        fmul dword ptr [esp + 0x18]
        fstp dword ptr [esp + 0x34]
        fld dword ptr [esp + 0x68]
        fadd st(0), st(2)
        fstp dword ptr [esp + 0x1c]
        fld dword ptr [esp + 0x6c]
        fadd st(0), st(1)
        fstp dword ptr [esp + 0x20]
        fstp st(0)
        fstp st(0)
        fld dword ptr [esp + 0x70]
        fadd dword ptr [esp + 0x34]
        fstp dword ptr [esp + 0x24]
        fsub dword ptr [esp + 0x28]
        fstp dword ptr [esp]
        push eax
        push ecx
        mov ecx, dword ptr [nxTask2lSphereMatrixA]
        call nxMeshContactAccumulate
L_10046a83:
        lea ecx, [esp + 0x110]
        call nxTask2lCallSphereDtor
        lea ecx, [esp + 0x2c]
        call nxTask2lCallSphereDtor
        mov al, 1
        add esp, 0x1f4
        ret
    }
}
