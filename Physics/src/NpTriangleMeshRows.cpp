/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// NpTriangleMeshRows.cpp: the public triangle mesh's methods (the oracle's NpTriangleMesh.cpp, vtable at .rdata
// 0x00108298): each one takes every scene's lock (the SDK's scene list), calls the internal mesh at +4 through its
// table and releases the locks. Naked listing transcriptions of phys_fn_002095 to 002138, each a thunk-and-body pair
// the inventory splits at the loop's jump target. The candidate has no public triangle-mesh facade object yet, so
// nothing calls them. Compare script: docs/reconstruction/novodex-physics/evidence/phase4-np-triangle-mesh-rows-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxNpMeshCallGetNbScenes();
extern "C" void nxNpMeshCallGetScene();
extern "C" void nxNpMeshCallLock();
extern "C" void nxNpMeshCallTryLock();
extern "C" void nxNpMeshCallUnlock();
extern "C" void* nxNpMeshImpImperrorFoundationSDKNxFoundationSANW4Nx;
extern "C" void* nxNpMeshImpImpinstanceFoundationSDKNxFoundation0PAV;
#pragma comment(linker, "/alternatename:_nxNpMeshCallGetNbScenes=?getNbScenes@PhysicsSDK@@QBEIXZ")
#pragma comment(linker, "/alternatename:_nxNpMeshCallGetScene=?getScene@PhysicsSDK@@QAEPAVScene@@I@Z")
#pragma comment(linker, "/alternatename:_nxNpMeshCallLock=?lock@ReadWriteLock@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxNpMeshCallTryLock=?tryLock@ReadWriteLock@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxNpMeshCallUnlock=?unlock@ReadWriteLock@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxNpMeshImpImperrorFoundationSDKNxFoundationSANW4Nx=__imp_?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ")
#pragma comment(linker, "/alternatename:_nxNpMeshImpImpinstanceFoundationSDKNxFoundation0PAV=__imp_?instance@FoundationSDK@NxFoundation@@0PAV12@A")
extern "C" void* nxSdkInstanceSlot;
#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")
extern "C" unsigned gNxApiReentry;
extern "C" const char* gNxApiReentryMessage;

unsigned __fastcall nxNpMesh2095(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxNpMesh2098(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxNpMesh2101(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxNpMesh2104(void* self, void* edxUnused);
unsigned __fastcall nxNpMesh2108(void* self, void* edxUnused);
unsigned __fastcall nxNpMesh2112(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxNpMesh2116(void* self, void* edxUnused);
unsigned __fastcall nxNpMesh2120(void* self, void* edxUnused);
unsigned __fastcall nxNpMesh2124(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxNpMesh2128(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxNpMesh2132(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxNpMesh2136(void* self, void* edxUnused, unsigned a0, unsigned a1);
// @@PROTOS-END@@

static const char nxNpMeshStr104760[] = "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!";
static const char nxNpMeshStr1082d0[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpTriangleMesh.cpp";

// phys_fn_002095 and phys_fn_002097 (one function) (0x00052a60, 240 B), 80 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2095(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00052a60
		push edi		// 0x00052a61
		mov edi, ecx		// 0x00052a62
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052a64
		xor esi, esi		// 0x00052a6a
		call nxNpMeshCallGetNbScenes		// 0x00052a6c
		test eax, eax		// 0x00052a71
		jbe L52aae		// 0x00052a73
		jmp L52a80		// 0x00052a75
		lea esp, [esp]		// 0x00052a77
		mov edi, edi		// 0x00052a7e
L52a80:
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052a80
		push esi		// 0x00052a86
		call nxNpMeshCallGetScene		// 0x00052a87
		mov eax, dword ptr [eax + 0x6cc]		// 0x00052a8c
		mov ecx, dword ptr [eax + 0xc]		// 0x00052a92
		call nxNpMeshCallTryLock		// 0x00052a95
		test al, al		// 0x00052a9a
		je L52b01		// 0x00052a9c
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052a9e
		inc esi		// 0x00052aa4
		call nxNpMeshCallGetNbScenes		// 0x00052aa5
		cmp esi, eax		// 0x00052aaa
		jb L52a80		// 0x00052aac
L52aae:
		mov ecx, dword ptr [edi + 4]		// 0x00052aae
		mov eax, dword ptr [esp + 0xc]		// 0x00052ab1
		mov edx, dword ptr [ecx]		// 0x00052ab5
		push ebx		// 0x00052ab7
		push eax		// 0x00052ab8
		call dword ptr [edx + 8]		// 0x00052ab9
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052abc
		mov edi, dword ptr [ecx + 4]		// 0x00052ac2
		mov ecx, dword ptr [edi + 4]		// 0x00052ac5
		mov bl, al		// 0x00052ac8
		xor esi, esi		// 0x00052aca
		call nxNpMeshCallGetNbScenes		// 0x00052acc
		test eax, eax		// 0x00052ad1
		jbe L52af9		// 0x00052ad3
L52ad5:
		mov ecx, dword ptr [edi + 4]		// 0x00052ad5
		push esi		// 0x00052ad8
		call nxNpMeshCallGetScene		// 0x00052ad9
		mov edx, dword ptr [eax + 0x6cc]		// 0x00052ade
		mov ecx, dword ptr [edx + 0xc]		// 0x00052ae4
		call nxNpMeshCallUnlock		// 0x00052ae7
		mov ecx, dword ptr [edi + 4]		// 0x00052aec
		inc esi		// 0x00052aef
		call nxNpMeshCallGetNbScenes		// 0x00052af0
		cmp esi, eax		// 0x00052af5
		jb L52ad5		// 0x00052af7
L52af9:
		mov al, bl		// 0x00052af9
		pop ebx		// 0x00052afb
		pop edi		// 0x00052afc
		pop esi		// 0x00052afd
		ret 4		// 0x00052afe
L52b01:
		test esi, esi		// 0x00052b01
		jbe L52b24		// 0x00052b03
L52b05:
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052b05
		dec esi		// 0x00052b0b
		push esi		// 0x00052b0c
		call nxNpMeshCallGetScene		// 0x00052b0d
		mov eax, dword ptr [eax + 0x6cc]		// 0x00052b12
		mov ecx, dword ptr [eax + 0xc]		// 0x00052b18
		call nxNpMeshCallUnlock		// 0x00052b1b
		test esi, esi		// 0x00052b20
		ja L52b05		// 0x00052b22
L52b24:
		mov ecx, dword ptr [nxNpMeshImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00052b24
		cmp dword ptr [ecx], 0		// 0x00052b2a
		jne L52b30		// 0x00052b2d
		int 3		// 0x00052b2f
L52b30:
		push offset nxNpMeshStr104760		// 0x00052b30
		push 0		// 0x00052b35
		push 0x23		// 0x00052b37
		push offset nxNpMeshStr1082d0		// 0x00052b39
		push 2		// 0x00052b3e
		call dword ptr [nxNpMeshImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00052b40
		add esp, 0x14		// 0x00052b46
		pop edi		// 0x00052b49
		xor al, al		// 0x00052b4a
		pop esi		// 0x00052b4c
		ret 4		// 0x00052b4d
		}
	}

// phys_fn_002098 and phys_fn_002100 (one function) (0x00052b50, 240 B), 80 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2098(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00052b50
		push edi		// 0x00052b51
		mov edi, ecx		// 0x00052b52
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052b54
		xor esi, esi		// 0x00052b5a
		call nxNpMeshCallGetNbScenes		// 0x00052b5c
		test eax, eax		// 0x00052b61
		jbe L52b9e		// 0x00052b63
		jmp L52b70		// 0x00052b65
		lea esp, [esp]		// 0x00052b67
		mov edi, edi		// 0x00052b6e
L52b70:
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052b70
		push esi		// 0x00052b76
		call nxNpMeshCallGetScene		// 0x00052b77
		mov eax, dword ptr [eax + 0x6cc]		// 0x00052b7c
		mov ecx, dword ptr [eax + 0xc]		// 0x00052b82
		call nxNpMeshCallTryLock		// 0x00052b85
		test al, al		// 0x00052b8a
		je L52bf1		// 0x00052b8c
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052b8e
		inc esi		// 0x00052b94
		call nxNpMeshCallGetNbScenes		// 0x00052b95
		cmp esi, eax		// 0x00052b9a
		jb L52b70		// 0x00052b9c
L52b9e:
		mov ecx, dword ptr [edi + 4]		// 0x00052b9e
		mov eax, dword ptr [esp + 0xc]		// 0x00052ba1
		mov edx, dword ptr [ecx]		// 0x00052ba5
		push ebx		// 0x00052ba7
		push eax		// 0x00052ba8
		call dword ptr [edx + 0xc]		// 0x00052ba9
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052bac
		mov edi, dword ptr [ecx + 4]		// 0x00052bb2
		mov ecx, dword ptr [edi + 4]		// 0x00052bb5
		mov bl, al		// 0x00052bb8
		xor esi, esi		// 0x00052bba
		call nxNpMeshCallGetNbScenes		// 0x00052bbc
		test eax, eax		// 0x00052bc1
		jbe L52be9		// 0x00052bc3
L52bc5:
		mov ecx, dword ptr [edi + 4]		// 0x00052bc5
		push esi		// 0x00052bc8
		call nxNpMeshCallGetScene		// 0x00052bc9
		mov edx, dword ptr [eax + 0x6cc]		// 0x00052bce
		mov ecx, dword ptr [edx + 0xc]		// 0x00052bd4
		call nxNpMeshCallUnlock		// 0x00052bd7
		mov ecx, dword ptr [edi + 4]		// 0x00052bdc
		inc esi		// 0x00052bdf
		call nxNpMeshCallGetNbScenes		// 0x00052be0
		cmp esi, eax		// 0x00052be5
		jb L52bc5		// 0x00052be7
L52be9:
		mov al, bl		// 0x00052be9
		pop ebx		// 0x00052beb
		pop edi		// 0x00052bec
		pop esi		// 0x00052bed
		ret 4		// 0x00052bee
L52bf1:
		test esi, esi		// 0x00052bf1
		jbe L52c14		// 0x00052bf3
L52bf5:
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052bf5
		dec esi		// 0x00052bfb
		push esi		// 0x00052bfc
		call nxNpMeshCallGetScene		// 0x00052bfd
		mov eax, dword ptr [eax + 0x6cc]		// 0x00052c02
		mov ecx, dword ptr [eax + 0xc]		// 0x00052c08
		call nxNpMeshCallUnlock		// 0x00052c0b
		test esi, esi		// 0x00052c10
		ja L52bf5		// 0x00052c12
L52c14:
		mov ecx, dword ptr [nxNpMeshImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00052c14
		cmp dword ptr [ecx], 0		// 0x00052c1a
		jne L52c20		// 0x00052c1d
		int 3		// 0x00052c1f
L52c20:
		push offset nxNpMeshStr104760		// 0x00052c20
		push 0		// 0x00052c25
		push 0x33		// 0x00052c27
		push offset nxNpMeshStr1082d0		// 0x00052c29
		push 2		// 0x00052c2e
		call dword ptr [nxNpMeshImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00052c30
		add esp, 0x14		// 0x00052c36
		pop edi		// 0x00052c39
		xor al, al		// 0x00052c3a
		pop esi		// 0x00052c3c
		ret 4		// 0x00052c3d
		}
	}

// phys_fn_002101 and phys_fn_002103 (one function) (0x00052c40, 240 B), 80 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2101(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00052c40
		push edi		// 0x00052c41
		mov edi, ecx		// 0x00052c42
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052c44
		xor esi, esi		// 0x00052c4a
		call nxNpMeshCallGetNbScenes		// 0x00052c4c
		test eax, eax		// 0x00052c51
		jbe L52c8e		// 0x00052c53
		jmp L52c60		// 0x00052c55
		lea esp, [esp]		// 0x00052c57
		mov edi, edi		// 0x00052c5e
L52c60:
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052c60
		push esi		// 0x00052c66
		call nxNpMeshCallGetScene		// 0x00052c67
		mov eax, dword ptr [eax + 0x6cc]		// 0x00052c6c
		mov ecx, dword ptr [eax + 0xc]		// 0x00052c72
		call nxNpMeshCallTryLock		// 0x00052c75
		test al, al		// 0x00052c7a
		je L52ce1		// 0x00052c7c
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052c7e
		inc esi		// 0x00052c84
		call nxNpMeshCallGetNbScenes		// 0x00052c85
		cmp esi, eax		// 0x00052c8a
		jb L52c60		// 0x00052c8c
L52c8e:
		mov ecx, dword ptr [edi + 4]		// 0x00052c8e
		mov eax, dword ptr [esp + 0xc]		// 0x00052c91
		mov edx, dword ptr [ecx]		// 0x00052c95
		push ebx		// 0x00052c97
		push eax		// 0x00052c98
		call dword ptr [edx + 0x24]		// 0x00052c99
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052c9c
		mov edi, dword ptr [ecx + 4]		// 0x00052ca2
		mov ecx, dword ptr [edi + 4]		// 0x00052ca5
		mov bl, al		// 0x00052ca8
		xor esi, esi		// 0x00052caa
		call nxNpMeshCallGetNbScenes		// 0x00052cac
		test eax, eax		// 0x00052cb1
		jbe L52cd9		// 0x00052cb3
L52cb5:
		mov ecx, dword ptr [edi + 4]		// 0x00052cb5
		push esi		// 0x00052cb8
		call nxNpMeshCallGetScene		// 0x00052cb9
		mov edx, dword ptr [eax + 0x6cc]		// 0x00052cbe
		mov ecx, dword ptr [edx + 0xc]		// 0x00052cc4
		call nxNpMeshCallUnlock		// 0x00052cc7
		mov ecx, dword ptr [edi + 4]		// 0x00052ccc
		inc esi		// 0x00052ccf
		call nxNpMeshCallGetNbScenes		// 0x00052cd0
		cmp esi, eax		// 0x00052cd5
		jb L52cb5		// 0x00052cd7
L52cd9:
		mov al, bl		// 0x00052cd9
		pop ebx		// 0x00052cdb
		pop edi		// 0x00052cdc
		pop esi		// 0x00052cdd
		ret 4		// 0x00052cde
L52ce1:
		test esi, esi		// 0x00052ce1
		jbe L52d04		// 0x00052ce3
L52ce5:
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052ce5
		dec esi		// 0x00052ceb
		push esi		// 0x00052cec
		call nxNpMeshCallGetScene		// 0x00052ced
		mov eax, dword ptr [eax + 0x6cc]		// 0x00052cf2
		mov ecx, dword ptr [eax + 0xc]		// 0x00052cf8
		call nxNpMeshCallUnlock		// 0x00052cfb
		test esi, esi		// 0x00052d00
		ja L52ce5		// 0x00052d02
L52d04:
		mov ecx, dword ptr [nxNpMeshImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00052d04
		cmp dword ptr [ecx], 0		// 0x00052d0a
		jne L52d10		// 0x00052d0d
		int 3		// 0x00052d0f
L52d10:
		push offset nxNpMeshStr104760		// 0x00052d10
		push 0		// 0x00052d15
		push 0x40		// 0x00052d17
		push offset nxNpMeshStr1082d0		// 0x00052d19
		push 2		// 0x00052d1e
		call dword ptr [nxNpMeshImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00052d20
		add esp, 0x14		// 0x00052d26
		pop edi		// 0x00052d29
		xor al, al		// 0x00052d2a
		pop esi		// 0x00052d2c
		ret 4		// 0x00052d2d
		}
	}

// phys_fn_002104 and phys_fn_002106 (one function) (0x00052d30, 142 B), 51 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2104(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052d30
		push ebx		// 0x00052d35
		push esi		// 0x00052d36
		push edi		// 0x00052d37
		mov edi, dword ptr [eax + 4]		// 0x00052d38
		mov ebx, ecx		// 0x00052d3b
		mov ecx, dword ptr [edi + 4]		// 0x00052d3d
		xor esi, esi		// 0x00052d40
		call nxNpMeshCallGetNbScenes		// 0x00052d42
		test eax, eax		// 0x00052d47
		jbe L52d74		// 0x00052d49
		jmp L52d50		// 0x00052d4b
		lea ecx, [ecx]		// 0x00052d4d
L52d50:
		mov ecx, dword ptr [edi + 4]		// 0x00052d50
		push esi		// 0x00052d53
		call nxNpMeshCallGetScene		// 0x00052d54
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052d59
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052d5f
		call nxNpMeshCallLock		// 0x00052d62
		mov ecx, dword ptr [edi + 4]		// 0x00052d67
		inc esi		// 0x00052d6a
		call nxNpMeshCallGetNbScenes		// 0x00052d6b
		cmp esi, eax		// 0x00052d70
		jb L52d50		// 0x00052d72
L52d74:
		mov ecx, dword ptr [ebx + 4]		// 0x00052d74
		mov edx, dword ptr [ecx]		// 0x00052d77
		call dword ptr [edx + 0x28]		// 0x00052d79
		mov bl, al		// 0x00052d7c
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052d7e
		mov edi, dword ptr [eax + 4]		// 0x00052d83
		mov ecx, dword ptr [edi + 4]		// 0x00052d86
		xor esi, esi		// 0x00052d89
		call nxNpMeshCallGetNbScenes		// 0x00052d8b
		test eax, eax		// 0x00052d90
		jbe L52db8		// 0x00052d92
L52d94:
		mov ecx, dword ptr [edi + 4]		// 0x00052d94
		push esi		// 0x00052d97
		call nxNpMeshCallGetScene		// 0x00052d98
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052d9d
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052da3
		call nxNpMeshCallUnlock		// 0x00052da6
		mov ecx, dword ptr [edi + 4]		// 0x00052dab
		inc esi		// 0x00052dae
		call nxNpMeshCallGetNbScenes		// 0x00052daf
		cmp esi, eax		// 0x00052db4
		jb L52d94		// 0x00052db6
L52db8:
		pop edi		// 0x00052db8
		pop esi		// 0x00052db9
		mov al, bl		// 0x00052dba
		pop ebx		// 0x00052dbc
		ret		// 0x00052dbd
		}
	}

// phys_fn_002108 and phys_fn_002110 (one function) (0x00052dc0, 142 B), 51 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2108(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052dc0
		push ebx		// 0x00052dc5
		push esi		// 0x00052dc6
		push edi		// 0x00052dc7
		mov edi, dword ptr [eax + 4]		// 0x00052dc8
		mov ebx, ecx		// 0x00052dcb
		mov ecx, dword ptr [edi + 4]		// 0x00052dcd
		xor esi, esi		// 0x00052dd0
		call nxNpMeshCallGetNbScenes		// 0x00052dd2
		test eax, eax		// 0x00052dd7
		jbe L52e04		// 0x00052dd9
		jmp L52de0		// 0x00052ddb
		lea ecx, [ecx]		// 0x00052ddd
L52de0:
		mov ecx, dword ptr [edi + 4]		// 0x00052de0
		push esi		// 0x00052de3
		call nxNpMeshCallGetScene		// 0x00052de4
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052de9
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052def
		call nxNpMeshCallLock		// 0x00052df2
		mov ecx, dword ptr [edi + 4]		// 0x00052df7
		inc esi		// 0x00052dfa
		call nxNpMeshCallGetNbScenes		// 0x00052dfb
		cmp esi, eax		// 0x00052e00
		jb L52de0		// 0x00052e02
L52e04:
		mov ecx, dword ptr [ebx + 4]		// 0x00052e04
		mov edx, dword ptr [ecx]		// 0x00052e07
		call dword ptr [edx + 0x2c]		// 0x00052e09
		mov ebx, eax		// 0x00052e0c
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052e0e
		mov edi, dword ptr [eax + 4]		// 0x00052e13
		mov ecx, dword ptr [edi + 4]		// 0x00052e16
		xor esi, esi		// 0x00052e19
		call nxNpMeshCallGetNbScenes		// 0x00052e1b
		test eax, eax		// 0x00052e20
		jbe L52e48		// 0x00052e22
L52e24:
		mov ecx, dword ptr [edi + 4]		// 0x00052e24
		push esi		// 0x00052e27
		call nxNpMeshCallGetScene		// 0x00052e28
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052e2d
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052e33
		call nxNpMeshCallUnlock		// 0x00052e36
		mov ecx, dword ptr [edi + 4]		// 0x00052e3b
		inc esi		// 0x00052e3e
		call nxNpMeshCallGetNbScenes		// 0x00052e3f
		cmp esi, eax		// 0x00052e44
		jb L52e24		// 0x00052e46
L52e48:
		pop edi		// 0x00052e48
		pop esi		// 0x00052e49
		mov eax, ebx		// 0x00052e4a
		pop ebx		// 0x00052e4c
		ret		// 0x00052e4d
		}
	}

// phys_fn_002112 and phys_fn_002114 (one function) (0x00052e50, 156 B), 54 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2112(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052e50
		push ebx		// 0x00052e55
		push esi		// 0x00052e56
		push edi		// 0x00052e57
		mov edi, dword ptr [eax + 4]		// 0x00052e58
		mov ebx, ecx		// 0x00052e5b
		mov ecx, dword ptr [edi + 4]		// 0x00052e5d
		xor esi, esi		// 0x00052e60
		call nxNpMeshCallGetNbScenes		// 0x00052e62
		test eax, eax		// 0x00052e67
		jbe L52e94		// 0x00052e69
		jmp L52e70		// 0x00052e6b
		lea ecx, [ecx]		// 0x00052e6d
L52e70:
		mov ecx, dword ptr [edi + 4]		// 0x00052e70
		push esi		// 0x00052e73
		call nxNpMeshCallGetScene		// 0x00052e74
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052e79
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052e7f
		call nxNpMeshCallLock		// 0x00052e82
		mov ecx, dword ptr [edi + 4]		// 0x00052e87
		inc esi		// 0x00052e8a
		call nxNpMeshCallGetNbScenes		// 0x00052e8b
		cmp esi, eax		// 0x00052e90
		jb L52e70		// 0x00052e92
L52e94:
		mov ecx, dword ptr [ebx + 4]		// 0x00052e94
		mov eax, dword ptr [esp + 0x10]		// 0x00052e97
		mov edx, dword ptr [ecx]		// 0x00052e9b
		push eax		// 0x00052e9d
		call dword ptr [edx + 0x30]		// 0x00052e9e
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00052ea1
		mov edi, dword ptr [ecx + 4]		// 0x00052ea7
		mov ecx, dword ptr [edi + 4]		// 0x00052eaa
		mov bl, al		// 0x00052ead
		xor esi, esi		// 0x00052eaf
		call nxNpMeshCallGetNbScenes		// 0x00052eb1
		test eax, eax		// 0x00052eb6
		jbe L52ee4		// 0x00052eb8
		lea ebx, [ebx]		// 0x00052eba
L52ec0:
		mov ecx, dword ptr [edi + 4]		// 0x00052ec0
		push esi		// 0x00052ec3
		call nxNpMeshCallGetScene		// 0x00052ec4
		mov edx, dword ptr [eax + 0x6cc]		// 0x00052ec9
		mov ecx, dword ptr [edx + 0x10]		// 0x00052ecf
		call nxNpMeshCallUnlock		// 0x00052ed2
		mov ecx, dword ptr [edi + 4]		// 0x00052ed7
		inc esi		// 0x00052eda
		call nxNpMeshCallGetNbScenes		// 0x00052edb
		cmp esi, eax		// 0x00052ee0
		jb L52ec0		// 0x00052ee2
L52ee4:
		pop edi		// 0x00052ee4
		pop esi		// 0x00052ee5
		mov al, bl		// 0x00052ee6
		pop ebx		// 0x00052ee8
		ret 4		// 0x00052ee9
		}
	}

// phys_fn_002116 and phys_fn_002118 (one function) (0x00052ef0, 142 B), 51 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2116(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052ef0
		push ebx		// 0x00052ef5
		push esi		// 0x00052ef6
		push edi		// 0x00052ef7
		mov edi, dword ptr [eax + 4]		// 0x00052ef8
		mov ebx, ecx		// 0x00052efb
		mov ecx, dword ptr [edi + 4]		// 0x00052efd
		xor esi, esi		// 0x00052f00
		call nxNpMeshCallGetNbScenes		// 0x00052f02
		test eax, eax		// 0x00052f07
		jbe L52f34		// 0x00052f09
		jmp L52f10		// 0x00052f0b
		lea ecx, [ecx]		// 0x00052f0d
L52f10:
		mov ecx, dword ptr [edi + 4]		// 0x00052f10
		push esi		// 0x00052f13
		call nxNpMeshCallGetScene		// 0x00052f14
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052f19
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052f1f
		call nxNpMeshCallLock		// 0x00052f22
		mov ecx, dword ptr [edi + 4]		// 0x00052f27
		inc esi		// 0x00052f2a
		call nxNpMeshCallGetNbScenes		// 0x00052f2b
		cmp esi, eax		// 0x00052f30
		jb L52f10		// 0x00052f32
L52f34:
		mov ecx, dword ptr [ebx + 4]		// 0x00052f34
		mov edx, dword ptr [ecx]		// 0x00052f37
		call dword ptr [edx + 0x34]		// 0x00052f39
		mov ebx, eax		// 0x00052f3c
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052f3e
		mov edi, dword ptr [eax + 4]		// 0x00052f43
		mov ecx, dword ptr [edi + 4]		// 0x00052f46
		xor esi, esi		// 0x00052f49
		call nxNpMeshCallGetNbScenes		// 0x00052f4b
		test eax, eax		// 0x00052f50
		jbe L52f78		// 0x00052f52
L52f54:
		mov ecx, dword ptr [edi + 4]		// 0x00052f54
		push esi		// 0x00052f57
		call nxNpMeshCallGetScene		// 0x00052f58
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052f5d
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052f63
		call nxNpMeshCallUnlock		// 0x00052f66
		mov ecx, dword ptr [edi + 4]		// 0x00052f6b
		inc esi		// 0x00052f6e
		call nxNpMeshCallGetNbScenes		// 0x00052f6f
		cmp esi, eax		// 0x00052f74
		jb L52f54		// 0x00052f76
L52f78:
		pop edi		// 0x00052f78
		pop esi		// 0x00052f79
		mov eax, ebx		// 0x00052f7a
		pop ebx		// 0x00052f7c
		ret		// 0x00052f7d
		}
	}

// phys_fn_002120 and phys_fn_002122 (one function) (0x00052f80, 138 B), 49 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2120(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052f80
		push ebx		// 0x00052f85
		push esi		// 0x00052f86
		push edi		// 0x00052f87
		mov edi, dword ptr [eax + 4]		// 0x00052f88
		mov ebx, ecx		// 0x00052f8b
		mov ecx, dword ptr [edi + 4]		// 0x00052f8d
		xor esi, esi		// 0x00052f90
		call nxNpMeshCallGetNbScenes		// 0x00052f92
		test eax, eax		// 0x00052f97
		jbe L52fc4		// 0x00052f99
		jmp L52fa0		// 0x00052f9b
		lea ecx, [ecx]		// 0x00052f9d
L52fa0:
		mov ecx, dword ptr [edi + 4]		// 0x00052fa0
		push esi		// 0x00052fa3
		call nxNpMeshCallGetScene		// 0x00052fa4
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052fa9
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052faf
		call nxNpMeshCallLock		// 0x00052fb2
		mov ecx, dword ptr [edi + 4]		// 0x00052fb7
		inc esi		// 0x00052fba
		call nxNpMeshCallGetNbScenes		// 0x00052fbb
		cmp esi, eax		// 0x00052fc0
		jb L52fa0		// 0x00052fc2
L52fc4:
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00052fc4
		mov edi, dword ptr [eax + 4]		// 0x00052fc9
		mov edx, dword ptr [ebx + 4]		// 0x00052fcc
		mov ecx, dword ptr [edi + 4]		// 0x00052fcf
		mov ebx, dword ptr [edx + 0xc]		// 0x00052fd2
		xor esi, esi		// 0x00052fd5
		call nxNpMeshCallGetNbScenes		// 0x00052fd7
		test eax, eax		// 0x00052fdc
		jbe L53004		// 0x00052fde
L52fe0:
		mov ecx, dword ptr [edi + 4]		// 0x00052fe0
		push esi		// 0x00052fe3
		call nxNpMeshCallGetScene		// 0x00052fe4
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00052fe9
		mov ecx, dword ptr [ecx + 0x10]		// 0x00052fef
		call nxNpMeshCallUnlock		// 0x00052ff2
		mov ecx, dword ptr [edi + 4]		// 0x00052ff7
		inc esi		// 0x00052ffa
		call nxNpMeshCallGetNbScenes		// 0x00052ffb
		cmp esi, eax		// 0x00053000
		jb L52fe0		// 0x00053002
L53004:
		pop edi		// 0x00053004
		pop esi		// 0x00053005
		mov eax, ebx		// 0x00053006
		pop ebx		// 0x00053008
		ret		// 0x00053009
		}
	}

// phys_fn_002124 and phys_fn_002126 (one function) (0x00053010, 156 B), 56 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2124(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00053010
		push ebx		// 0x00053015
		push esi		// 0x00053016
		push edi		// 0x00053017
		mov edi, dword ptr [eax + 4]		// 0x00053018
		mov ebx, ecx		// 0x0005301b
		mov ecx, dword ptr [edi + 4]		// 0x0005301d
		xor esi, esi		// 0x00053020
		call nxNpMeshCallGetNbScenes		// 0x00053022
		test eax, eax		// 0x00053027
		jbe L53054		// 0x00053029
		jmp L53030		// 0x0005302b
		lea ecx, [ecx]		// 0x0005302d
L53030:
		mov ecx, dword ptr [edi + 4]		// 0x00053030
		push esi		// 0x00053033
		call nxNpMeshCallGetScene		// 0x00053034
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00053039
		mov ecx, dword ptr [ecx + 0x10]		// 0x0005303f
		call nxNpMeshCallLock		// 0x00053042
		mov ecx, dword ptr [edi + 4]		// 0x00053047
		inc esi		// 0x0005304a
		call nxNpMeshCallGetNbScenes		// 0x0005304b
		cmp esi, eax		// 0x00053050
		jb L53030		// 0x00053052
L53054:
		mov eax, dword ptr [esp + 0x14]		// 0x00053054
		mov ecx, dword ptr [ebx + 4]		// 0x00053058
		mov edx, dword ptr [ecx]		// 0x0005305b
		push eax		// 0x0005305d
		mov eax, dword ptr [esp + 0x14]		// 0x0005305e
		push eax		// 0x00053062
		call dword ptr [edx + 0x14]		// 0x00053063
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00053066
		mov edi, dword ptr [ecx + 4]		// 0x0005306c
		mov ecx, dword ptr [edi + 4]		// 0x0005306f
		mov ebx, eax		// 0x00053072
		xor esi, esi		// 0x00053074
		call nxNpMeshCallGetNbScenes		// 0x00053076
		test eax, eax		// 0x0005307b
		jbe L530a4		// 0x0005307d
		nop		// 0x0005307f
L53080:
		mov ecx, dword ptr [edi + 4]		// 0x00053080
		push esi		// 0x00053083
		call nxNpMeshCallGetScene		// 0x00053084
		mov edx, dword ptr [eax + 0x6cc]		// 0x00053089
		mov ecx, dword ptr [edx + 0x10]		// 0x0005308f
		call nxNpMeshCallUnlock		// 0x00053092
		mov ecx, dword ptr [edi + 4]		// 0x00053097
		inc esi		// 0x0005309a
		call nxNpMeshCallGetNbScenes		// 0x0005309b
		cmp esi, eax		// 0x000530a0
		jb L53080		// 0x000530a2
L530a4:
		pop edi		// 0x000530a4
		pop esi		// 0x000530a5
		mov eax, ebx		// 0x000530a6
		pop ebx		// 0x000530a8
		ret 8		// 0x000530a9
		}
	}

// phys_fn_002128 and phys_fn_002130 (one function) (0x000530b0, 156 B), 56 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2128(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x000530b0
		push ebx		// 0x000530b5
		push esi		// 0x000530b6
		push edi		// 0x000530b7
		mov edi, dword ptr [eax + 4]		// 0x000530b8
		mov ebx, ecx		// 0x000530bb
		mov ecx, dword ptr [edi + 4]		// 0x000530bd
		xor esi, esi		// 0x000530c0
		call nxNpMeshCallGetNbScenes		// 0x000530c2
		test eax, eax		// 0x000530c7
		jbe L530f4		// 0x000530c9
		jmp L530d0		// 0x000530cb
		lea ecx, [ecx]		// 0x000530cd
L530d0:
		mov ecx, dword ptr [edi + 4]		// 0x000530d0
		push esi		// 0x000530d3
		call nxNpMeshCallGetScene		// 0x000530d4
		mov ecx, dword ptr [eax + 0x6cc]		// 0x000530d9
		mov ecx, dword ptr [ecx + 0x10]		// 0x000530df
		call nxNpMeshCallLock		// 0x000530e2
		mov ecx, dword ptr [edi + 4]		// 0x000530e7
		inc esi		// 0x000530ea
		call nxNpMeshCallGetNbScenes		// 0x000530eb
		cmp esi, eax		// 0x000530f0
		jb L530d0		// 0x000530f2
L530f4:
		mov eax, dword ptr [esp + 0x14]		// 0x000530f4
		mov ecx, dword ptr [ebx + 4]		// 0x000530f8
		mov edx, dword ptr [ecx]		// 0x000530fb
		push eax		// 0x000530fd
		mov eax, dword ptr [esp + 0x14]		// 0x000530fe
		push eax		// 0x00053102
		call dword ptr [edx + 0x18]		// 0x00053103
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00053106
		mov edi, dword ptr [ecx + 4]		// 0x0005310c
		mov ecx, dword ptr [edi + 4]		// 0x0005310f
		mov ebx, eax		// 0x00053112
		xor esi, esi		// 0x00053114
		call nxNpMeshCallGetNbScenes		// 0x00053116
		test eax, eax		// 0x0005311b
		jbe L53144		// 0x0005311d
		nop		// 0x0005311f
L53120:
		mov ecx, dword ptr [edi + 4]		// 0x00053120
		push esi		// 0x00053123
		call nxNpMeshCallGetScene		// 0x00053124
		mov edx, dword ptr [eax + 0x6cc]		// 0x00053129
		mov ecx, dword ptr [edx + 0x10]		// 0x0005312f
		call nxNpMeshCallUnlock		// 0x00053132
		mov ecx, dword ptr [edi + 4]		// 0x00053137
		inc esi		// 0x0005313a
		call nxNpMeshCallGetNbScenes		// 0x0005313b
		cmp esi, eax		// 0x00053140
		jb L53120		// 0x00053142
L53144:
		pop edi		// 0x00053144
		pop esi		// 0x00053145
		mov eax, ebx		// 0x00053146
		pop ebx		// 0x00053148
		ret 8		// 0x00053149
		}
	}

// phys_fn_002132 and phys_fn_002134 (one function) (0x00053150, 156 B), 56 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2132(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x00053150
		push ebx		// 0x00053155
		push esi		// 0x00053156
		push edi		// 0x00053157
		mov edi, dword ptr [eax + 4]		// 0x00053158
		mov ebx, ecx		// 0x0005315b
		mov ecx, dword ptr [edi + 4]		// 0x0005315d
		xor esi, esi		// 0x00053160
		call nxNpMeshCallGetNbScenes		// 0x00053162
		test eax, eax		// 0x00053167
		jbe L53194		// 0x00053169
		jmp L53170		// 0x0005316b
		lea ecx, [ecx]		// 0x0005316d
L53170:
		mov ecx, dword ptr [edi + 4]		// 0x00053170
		push esi		// 0x00053173
		call nxNpMeshCallGetScene		// 0x00053174
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00053179
		mov ecx, dword ptr [ecx + 0x10]		// 0x0005317f
		call nxNpMeshCallLock		// 0x00053182
		mov ecx, dword ptr [edi + 4]		// 0x00053187
		inc esi		// 0x0005318a
		call nxNpMeshCallGetNbScenes		// 0x0005318b
		cmp esi, eax		// 0x00053190
		jb L53170		// 0x00053192
L53194:
		mov eax, dword ptr [esp + 0x14]		// 0x00053194
		mov ecx, dword ptr [ebx + 4]		// 0x00053198
		mov edx, dword ptr [ecx]		// 0x0005319b
		push eax		// 0x0005319d
		mov eax, dword ptr [esp + 0x14]		// 0x0005319e
		push eax		// 0x000531a2
		call dword ptr [edx + 0x1c]		// 0x000531a3
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x000531a6
		mov edi, dword ptr [ecx + 4]		// 0x000531ac
		mov ecx, dword ptr [edi + 4]		// 0x000531af
		mov ebx, eax		// 0x000531b2
		xor esi, esi		// 0x000531b4
		call nxNpMeshCallGetNbScenes		// 0x000531b6
		test eax, eax		// 0x000531bb
		jbe L531e4		// 0x000531bd
		nop		// 0x000531bf
L531c0:
		mov ecx, dword ptr [edi + 4]		// 0x000531c0
		push esi		// 0x000531c3
		call nxNpMeshCallGetScene		// 0x000531c4
		mov edx, dword ptr [eax + 0x6cc]		// 0x000531c9
		mov ecx, dword ptr [edx + 0x10]		// 0x000531cf
		call nxNpMeshCallUnlock		// 0x000531d2
		mov ecx, dword ptr [edi + 4]		// 0x000531d7
		inc esi		// 0x000531da
		call nxNpMeshCallGetNbScenes		// 0x000531db
		cmp esi, eax		// 0x000531e0
		jb L531c0		// 0x000531e2
L531e4:
		pop edi		// 0x000531e4
		pop esi		// 0x000531e5
		mov eax, ebx		// 0x000531e6
		pop ebx		// 0x000531e8
		ret 8		// 0x000531e9
		}
	}

// phys_fn_002136 and phys_fn_002138 (one function) (0x000531f0, 156 B), 56 instructions
__declspec(naked) unsigned __fastcall nxNpMesh2136(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [nxSdkInstanceSlot]		// 0x000531f0
		push ebx		// 0x000531f5
		push esi		// 0x000531f6
		push edi		// 0x000531f7
		mov edi, dword ptr [eax + 4]		// 0x000531f8
		mov ebx, ecx		// 0x000531fb
		mov ecx, dword ptr [edi + 4]		// 0x000531fd
		xor esi, esi		// 0x00053200
		call nxNpMeshCallGetNbScenes		// 0x00053202
		test eax, eax		// 0x00053207
		jbe L53234		// 0x00053209
		jmp L53210		// 0x0005320b
		lea ecx, [ecx]		// 0x0005320d
L53210:
		mov ecx, dword ptr [edi + 4]		// 0x00053210
		push esi		// 0x00053213
		call nxNpMeshCallGetScene		// 0x00053214
		mov ecx, dword ptr [eax + 0x6cc]		// 0x00053219
		mov ecx, dword ptr [ecx + 0x10]		// 0x0005321f
		call nxNpMeshCallLock		// 0x00053222
		mov ecx, dword ptr [edi + 4]		// 0x00053227
		inc esi		// 0x0005322a
		call nxNpMeshCallGetNbScenes		// 0x0005322b
		cmp esi, eax		// 0x00053230
		jb L53210		// 0x00053232
L53234:
		mov eax, dword ptr [esp + 0x14]		// 0x00053234
		mov ecx, dword ptr [ebx + 4]		// 0x00053238
		mov edx, dword ptr [ecx]		// 0x0005323b
		push eax		// 0x0005323d
		mov eax, dword ptr [esp + 0x14]		// 0x0005323e
		push eax		// 0x00053242
		call dword ptr [edx + 0x20]		// 0x00053243
		mov ecx, dword ptr [nxSdkInstanceSlot]		// 0x00053246
		mov edi, dword ptr [ecx + 4]		// 0x0005324c
		mov ecx, dword ptr [edi + 4]		// 0x0005324f
		mov ebx, eax		// 0x00053252
		xor esi, esi		// 0x00053254
		call nxNpMeshCallGetNbScenes		// 0x00053256
		test eax, eax		// 0x0005325b
		jbe L53284		// 0x0005325d
		nop		// 0x0005325f
L53260:
		mov ecx, dword ptr [edi + 4]		// 0x00053260
		push esi		// 0x00053263
		call nxNpMeshCallGetScene		// 0x00053264
		mov edx, dword ptr [eax + 0x6cc]		// 0x00053269
		mov ecx, dword ptr [edx + 0x10]		// 0x0005326f
		call nxNpMeshCallUnlock		// 0x00053272
		mov ecx, dword ptr [edi + 4]		// 0x00053277
		inc esi		// 0x0005327a
		call nxNpMeshCallGetNbScenes		// 0x0005327b
		cmp esi, eax		// 0x00053280
		jb L53260		// 0x00053282
L53284:
		pop edi		// 0x00053284
		pop esi		// 0x00053285
		mov eax, ebx		// 0x00053286
		pop ebx		// 0x00053288
		ret 8		// 0x00053289
		}
	}
// @@ROWS-END@@
