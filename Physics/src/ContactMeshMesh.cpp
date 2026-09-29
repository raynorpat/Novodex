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
