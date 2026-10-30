/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// ContactMeshMeshRow.cpp: phys_fn_001876 (0x00046ab0, 1597 B), matrix A [MESH][MESH], a naked listing transcription.
// Its statics are the candidate's ContactAccumulator block (001872) and 001874's matrix slots by name. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase3-mesh-mesh-contact-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxMMCallCollideNoLeafCallback();
extern "C" void nxMMCallNxContactConvexConvex();
extern "C" void nxMMCallNxContactConvexHeightfield();
extern "C" void nxMMCallNxContactConvexMesh();
extern "C" void nxMMCallNxEmitContactThiscall();
extern "C" void nxMMCallNxMeshHeightfieldContact();
extern "C" void nxMMCallNxMeshPMapContacts();
extern "C" void* nxMMImpImpNxNormalToTangents;
extern "C" void* nxMMImpImperrorFoundationSDKNxFoundationSANW4Nx;
extern "C" void* nxMMImpImpinstanceFoundationSDKNxFoundation0PAV;
#pragma comment(linker, "/alternatename:_nxMMCallCollideNoLeafCallback=?CollideNoLeafCallback@NxCallbackTreeCollider@Opcode@@QAE_NP6AXPBVAABBNoLeafNode@2@0@ZHPBVModel@2@2PBVMatrix4x4@IceMaths@@3@Z")
#pragma comment(linker, "/alternatename:_nxMMCallNxContactConvexConvex=?NxContactConvexConvex@@YAXPBUNxCollisionShape@@0PAUNxContactSink@@PAX@Z")
#pragma comment(linker, "/alternatename:_nxMMCallNxContactConvexHeightfield=?NxContactConvexHeightfield@@YAXPBUNxCollisionShape@@0PAUNxContactSink@@PAX@Z")
#pragma comment(linker, "/alternatename:_nxMMCallNxContactConvexMesh=?NxContactConvexMesh@@YAXPBUNxCollisionShape@@0PAUNxContactSink@@PAX@Z")
#pragma comment(linker, "/alternatename:_nxMMCallNxEmitContactThiscall=?NxEmitContactThiscall@@YIXPAUNxContactSink@@IPAX1IPBVNxVec3@@2GG@Z")
#pragma comment(linker, "/alternatename:_nxMMCallNxMeshHeightfieldContact=_nxMeshHeightfieldContact")
#pragma comment(linker, "/alternatename:_nxMMCallNxMeshPMapContacts=_nxMeshPMapContacts")
#pragma comment(linker, "/alternatename:_nxMMImpImpNxNormalToTangents=__imp__NxNormalToTangents")
#pragma comment(linker, "/alternatename:_nxMMImpImperrorFoundationSDKNxFoundationSANW4Nx=__imp_?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ")
#pragma comment(linker, "/alternatename:_nxMMImpImpinstanceFoundationSDKNxFoundation0PAV=__imp_?instance@FoundationSDK@NxFoundation@@0PAV12@A")
extern NxReal gParameter[];
extern "C" unsigned* nxTask2lSphereMatrixA;
extern "C" unsigned* nxTask2lSphereMatrixB;
extern "C" unsigned nxMeshContactCount;
extern "C" float nxMeshContactSums[3];
extern "C" float nxMeshContactNormalAndMaterial[50];
extern "C" float nxMeshContactVertices[96];
extern "C" float nxMeshContactNormals[96];
extern "C" bool __cdecl nxMeshMeshSphereCallback(const float*, const float*);
// .data 0x10123d80 and 0x10123d88: two words only this row writes.
static unsigned nxMeshMeshStatic80, nxMeshMeshStatic88;

void __cdecl nxMeshMeshContactRow(const void* shape0, const void* shape1, void* sink, void* context);
// @@PROTOS-END@@

static const unsigned nxMMK101041ec_4 = 0x3f800000;
static const unsigned nxMMK101041f0_4 = 0x0;
static const char nxMMStr107bc4[] = "Opcode is not OK.";
static const char nxMMStr107d5c[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\ContactMeshMesh.cpp";

// phys_fn_001876 (0x00046ab0, 1597 B), 450 instructions
__declspec(naked) void __cdecl nxMeshMeshContactRow(const void* shape0, const void* shape1, void* sink, void* context)
	{
	__asm
		{
		sub esp, 0xd8		// 0x00046ab0
		push ebx		// 0x00046ab6
		push ebp		// 0x00046ab7
		push esi		// 0x00046ab8
		mov esi, dword ptr [esp + 0xe8]		// 0x00046ab9
		mov eax, dword ptr [esi + 0xe0]		// 0x00046ac0
		mov ebp, dword ptr [eax + 0xa0]		// 0x00046ac6
		test ebp, ebp		// 0x00046acc
		push edi		// 0x00046ace
		mov edi, dword ptr [esp + 0xf0]		// 0x00046acf
		mov ecx, dword ptr [edi + 0xe0]		// 0x00046ad6
		mov ebx, dword ptr [ecx + 0xa0]		// 0x00046adc
		je L46b0d		// 0x00046ae2
		test ebx, ebx		// 0x00046ae4
		je L46b0d		// 0x00046ae6
		mov eax, dword ptr [esp + 0xf8]		// 0x00046ae8
		mov ecx, dword ptr [esp + 0xf4]		// 0x00046aef
		push eax		// 0x00046af6
		push ecx		// 0x00046af7
		push edi		// 0x00046af8
		push esi		// 0x00046af9
		call nxMMCallNxContactConvexConvex		// 0x00046afa
		add esp, 0x10		// 0x00046aff
		pop edi		// 0x00046b02
		pop esi		// 0x00046b03
		pop ebp		// 0x00046b04
		pop ebx		// 0x00046b05
		add esp, 0xd8		// 0x00046b06
		ret		// 0x00046b0c
L46b0d:
		cmp dword ptr [eax + 0x7c], 0xff		// 0x00046b0d
		setne dl		// 0x00046b14
		test dl, dl		// 0x00046b17
		jne L47051		// 0x00046b19
		cmp dword ptr [ecx + 0x7c], 0xff		// 0x00046b1f
		jne L4709f		// 0x00046b26
		mov edx, dword ptr [eax + 0x9c]		// 0x00046b2c
		test edx, edx		// 0x00046b32
		je L46b65		// 0x00046b34
		mov eax, dword ptr [ecx + 0x9c]		// 0x00046b36
		test eax, eax		// 0x00046b3c
		je L46b65		// 0x00046b3e
		mov edx, dword ptr [esp + 0xf8]		// 0x00046b40
		mov eax, dword ptr [esp + 0xf4]		// 0x00046b47
		push edx		// 0x00046b4e
		push eax		// 0x00046b4f
		push edi		// 0x00046b50
		push esi		// 0x00046b51
		call nxMMCallNxMeshPMapContacts		// 0x00046b52
		add esp, 0x10		// 0x00046b57
		pop edi		// 0x00046b5a
		pop esi		// 0x00046b5b
		pop ebp		// 0x00046b5c
		pop ebx		// 0x00046b5d
		add esp, 0xd8		// 0x00046b5e
		ret		// 0x00046b64
L46b65:
		test ebp, ebp		// 0x00046b65
		jne L47007		// 0x00046b67
		test ebx, ebx		// 0x00046b6d
		jne L4702c		// 0x00046b6f
		mov eax, dword ptr [esp + 0xf8]		// 0x00046b75
		mov edx, dword ptr [eax + 0x330]		// 0x00046b7c
		and edx, 0xfffffffe		// 0x00046b82
		mov dword ptr [eax + 0x330], edx		// 0x00046b85
		fld dword ptr [gParameter + 0x20]		// 0x00046b8b
		fistp qword ptr [esp + 0x50]		// 0x00046b91
		mov edx, dword ptr [esp + 0xf4]		// 0x00046b95
		mov dword ptr [nxMeshContactSums + 8], ebx		// 0x00046b9c
		mov dword ptr [nxMeshContactSums + 4], ebx		// 0x00046ba2
		mov dword ptr [nxMeshContactSums], ebx		// 0x00046ba8
		mov dword ptr [nxMeshMeshStatic80], edx		// 0x00046bae
		mov edx, dword ptr [esi + 0xc]		// 0x00046bb4
		mov dword ptr [esp + 0xa8], edx		// 0x00046bb7
		mov edx, dword ptr [esi + 0x10]		// 0x00046bbe
		mov dword ptr [esp + 0xb8], edx		// 0x00046bc1
		mov edx, dword ptr [esi + 0x14]		// 0x00046bc8
		mov dword ptr [esp + 0xc8], edx		// 0x00046bcb
		mov edx, dword ptr [esi + 0x18]		// 0x00046bd2
		lea ecx, [eax + 0x32c]		// 0x00046bd5
		mov dword ptr [esp + 0xac], edx		// 0x00046bdb
		mov edx, dword ptr [esi + 0x1c]		// 0x00046be2
		mov dword ptr [esp + 0xbc], edx		// 0x00046be5
		mov edx, dword ptr [esi + 0x20]		// 0x00046bec
		mov dword ptr [esp + 0xcc], edx		// 0x00046bef
		mov edx, dword ptr [esi + 0x24]		// 0x00046bf6
		mov dword ptr [esp + 0xb0], edx		// 0x00046bf9
		mov edx, dword ptr [esi + 0x28]		// 0x00046c00
		mov dword ptr [esp + 0xc0], edx		// 0x00046c03
		mov edx, dword ptr [esi + 0x2c]		// 0x00046c0a
		mov dword ptr [esp + 0xd0], edx		// 0x00046c0d
		mov edx, dword ptr [esi + 0x30]		// 0x00046c14
		mov dword ptr [esp + 0xd8], edx		// 0x00046c17
		mov edx, dword ptr [esi + 0x34]		// 0x00046c1e
		mov dword ptr [esp + 0xdc], edx		// 0x00046c21
		mov edx, dword ptr [esi + 0x38]		// 0x00046c28
		mov dword ptr [esp + 0xe0], edx		// 0x00046c2b
		mov edx, dword ptr [edi + 0xc]		// 0x00046c32
		mov dword ptr [esp + 0x68], edx		// 0x00046c35
		mov edx, dword ptr [edi + 0x10]		// 0x00046c39
		mov dword ptr [esp + 0x78], edx		// 0x00046c3c
		mov edx, dword ptr [edi + 0x14]		// 0x00046c40
		mov dword ptr [esp + 0x88], edx		// 0x00046c43
		mov edx, dword ptr [edi + 0x18]		// 0x00046c4a
		mov dword ptr [esp + 0x6c], edx		// 0x00046c4d
		mov edx, dword ptr [edi + 0x1c]		// 0x00046c51
		mov dword ptr [esp + 0x7c], edx		// 0x00046c54
		mov edx, dword ptr [edi + 0x20]		// 0x00046c58
		mov dword ptr [esp + 0x8c], edx		// 0x00046c5b
		mov edx, dword ptr [edi + 0x24]		// 0x00046c62
		mov dword ptr [esp + 0x70], edx		// 0x00046c65
		mov edx, dword ptr [edi + 0x28]		// 0x00046c69
		mov dword ptr [esp + 0x80], edx		// 0x00046c6c
		mov edx, dword ptr [edi + 0x2c]		// 0x00046c73
		mov dword ptr [esp + 0x90], edx		// 0x00046c76
		mov edx, dword ptr [edi + 0x30]		// 0x00046c7d
		mov dword ptr [esp + 0x98], edx		// 0x00046c80
		mov edx, dword ptr [edi + 0x34]		// 0x00046c87
		mov dword ptr [esp + 0x9c], edx		// 0x00046c8a
		mov edx, dword ptr [edi + 0x38]		// 0x00046c91
		lea ebp, [esp + 0x68]		// 0x00046c94
		mov dword ptr [esp + 0xd4], ebx		// 0x00046c98
		mov dword ptr [esp + 0xc4], ebx		// 0x00046c9f
		mov dword ptr [esp + 0xb4], ebx		// 0x00046ca6
		mov dword ptr [esp + 0xa0], edx		// 0x00046cad
		mov edx, dword ptr [edi + 0xe0]		// 0x00046cb4
		mov dword ptr [esp + 0x94], ebx		// 0x00046cba
		mov dword ptr [esp + 0x84], ebx		// 0x00046cc1
		mov dword ptr [esp + 0x74], ebx		// 0x00046cc8
		mov dword ptr [nxMeshContactCount], ebx		// 0x00046ccc
		mov ebx, dword ptr [esi + 0xe0]		// 0x00046cd2
		push ebp		// 0x00046cd8
		mov dword ptr [esp + 0xe8], 0x3f800000		// 0x00046cd9
		mov dword ptr [esp + 0xa8], 0x3f800000		// 0x00046ce4
		mov edx, dword ptr [edx + 0x28]		// 0x00046cef
		mov ebx, dword ptr [ebx + 0x28]		// 0x00046cf2
		lea ebp, [esp + 0xac]		// 0x00046cf5
		mov dword ptr [nxMeshMeshStatic88], eax		// 0x00046cfc
		mov eax, dword ptr [esp + 0x54]		// 0x00046d01
		mov dword ptr [nxTask2lSphereMatrixA], esi		// 0x00046d05
		mov dword ptr [nxTask2lSphereMatrixB], edi		// 0x00046d0b
		push ebp		// 0x00046d11
		push edx		// 0x00046d12
		push ebx		// 0x00046d13
		push eax		// 0x00046d14
		push offset nxMeshMeshSphereCallback		// 0x00046d15
		call nxMMCallCollideNoLeafCallback		// 0x00046d1a
		test al, al		// 0x00046d1f
		je L46fd4		// 0x00046d21
		mov eax, dword ptr [nxMeshContactCount]		// 0x00046d27
		test eax, eax		// 0x00046d2c
		je L470e2		// 0x00046d2e
		fld dword ptr [nxMeshContactSums + 4]		// 0x00046d34
		fmul dword ptr [nxMeshContactSums + 4]		// 0x00046d3a
		fld dword ptr [nxMeshContactSums + 8]		// 0x00046d40
		fmul dword ptr [nxMeshContactSums + 8]		// 0x00046d46
		faddp st(1), st(0)		// 0x00046d4c
		fld dword ptr [nxMeshContactSums]		// 0x00046d4e
		fmul dword ptr [nxMeshContactSums]		// 0x00046d54
		faddp st(1), st(0)		// 0x00046d5a
		fsqrt		// 0x00046d5c
		fld dword ptr [nxMMK101041f0_4]		// 0x00046d5e
		fld st(1)		// 0x00046d64
		fucompp		// 0x00046d66
		fnstsw ax		// 0x00046d68
		test ah, 0x44		// 0x00046d6a
		jnp L46d9f		// 0x00046d6d
		fdivr dword ptr [nxMMK101041ec_4]		// 0x00046d6f
		fld dword ptr [nxMeshContactSums]		// 0x00046d75
		fmul st(0), st(1)		// 0x00046d7b
		fstp dword ptr [nxMeshContactSums]		// 0x00046d7d
		fld dword ptr [nxMeshContactSums + 4]		// 0x00046d83
		fmul st(0), st(1)		// 0x00046d89
		fstp dword ptr [nxMeshContactSums + 4]		// 0x00046d8b
		fmul dword ptr [nxMeshContactSums + 8]		// 0x00046d91
		fstp dword ptr [nxMeshContactSums + 8]		// 0x00046d97
		jmp L46da1		// 0x00046d9d
L46d9f:
		fstp st(0)		// 0x00046d9f
L46da1:
		lea eax, [esp + 0x5c]		// 0x00046da1
		push eax		// 0x00046da5
		lea ecx, [esp + 0x54]		// 0x00046da6
		push ecx		// 0x00046daa
		push offset nxMeshContactSums		// 0x00046dab
		call dword ptr [nxMMImpImpNxNormalToTangents]		// 0x00046db0
		mov ebx, dword ptr [nxMeshContactCount]		// 0x00046db6
		or eax, 0xffffffff		// 0x00046dbc
		add esp, 0xc		// 0x00046dbf
		xor ecx, ecx		// 0x00046dc2
		test ebx, ebx		// 0x00046dc4
		mov dword ptr [esp + 0x10], 0x60ad78ec		// 0x00046dc6
		mov dword ptr [esp + 0x14], 0xe0ad78ec		// 0x00046dce
		mov dword ptr [esp + 0x18], 0x60ad78ec		// 0x00046dd6
		mov dword ptr [esp + 0x1c], 0xe0ad78ec		// 0x00046dde
		mov dword ptr [esp + 0x20], 0x60ad78ec		// 0x00046de6
		mov dword ptr [esp + 0x24], 0xe0ad78ec		// 0x00046dee
		mov dword ptr [esp + 0x28], 0x60ad78ec		// 0x00046df6
		mov dword ptr [esp + 0x2c], 0xe0ad78ec		// 0x00046dfe
		mov dword ptr [esp + 0x30], eax		// 0x00046e06
		mov dword ptr [esp + 0x34], eax		// 0x00046e0a
		mov dword ptr [esp + 0x38], eax		// 0x00046e0e
		mov dword ptr [esp + 0x3c], eax		// 0x00046e12
		mov dword ptr [esp + 0x40], eax		// 0x00046e16
		mov dword ptr [esp + 0x44], eax		// 0x00046e1a
		mov dword ptr [esp + 0x48], eax		// 0x00046e1e
		mov dword ptr [esp + 0x4c], eax		// 0x00046e22
		jbe L46f0d		// 0x00046e26
		mov edx, offset nxMeshContactVertices + 8		// 0x00046e2c
L46e31:
		fld dword ptr [esp + 0x50]		// 0x00046e31
		fmul dword ptr [edx - 8]		// 0x00046e35
		fld dword ptr [esp + 0x54]		// 0x00046e38
		fmul dword ptr [edx - 4]		// 0x00046e3c
		faddp st(1), st(0)		// 0x00046e3f
		fld dword ptr [esp + 0x58]		// 0x00046e41
		fmul dword ptr [edx]		// 0x00046e45
		faddp st(1), st(0)		// 0x00046e47
		fcom dword ptr [esp + 0x10]		// 0x00046e49
		fnstsw ax		// 0x00046e4d
		test ah, 5		// 0x00046e4f
		jp L46e5c		// 0x00046e52
		fst dword ptr [esp + 0x10]		// 0x00046e54
		mov dword ptr [esp + 0x30], ecx		// 0x00046e58
L46e5c:
		fcom dword ptr [esp + 0x20]		// 0x00046e5c
		fnstsw ax		// 0x00046e60
		test ah, 5		// 0x00046e62
		jp L46e6f		// 0x00046e65
		fst dword ptr [esp + 0x20]		// 0x00046e67
		mov dword ptr [esp + 0x40], ecx		// 0x00046e6b
L46e6f:
		fcom dword ptr [esp + 0x14]		// 0x00046e6f
		fnstsw ax		// 0x00046e73
		test ah, 0x41		// 0x00046e75
		jne L46e82		// 0x00046e78
		fst dword ptr [esp + 0x14]		// 0x00046e7a
		mov dword ptr [esp + 0x34], ecx		// 0x00046e7e
L46e82:
		fcom dword ptr [esp + 0x24]		// 0x00046e82
		fnstsw ax		// 0x00046e86
		test ah, 0x41		// 0x00046e88
		jne L46e97		// 0x00046e8b
		fstp dword ptr [esp + 0x24]		// 0x00046e8d
		mov dword ptr [esp + 0x44], ecx		// 0x00046e91
		jmp L46e99		// 0x00046e95
L46e97:
		fstp st(0)		// 0x00046e97
L46e99:
		fld dword ptr [esp + 0x5c]		// 0x00046e99
		fmul dword ptr [edx - 8]		// 0x00046e9d
		fld dword ptr [esp + 0x60]		// 0x00046ea0
		fmul dword ptr [edx - 4]		// 0x00046ea4
		faddp st(1), st(0)		// 0x00046ea7
		fld dword ptr [esp + 0x64]		// 0x00046ea9
		fmul dword ptr [edx]		// 0x00046ead
		faddp st(1), st(0)		// 0x00046eaf
		fcom dword ptr [esp + 0x18]		// 0x00046eb1
		fnstsw ax		// 0x00046eb5
		test ah, 5		// 0x00046eb7
		jp L46ec4		// 0x00046eba
		fst dword ptr [esp + 0x18]		// 0x00046ebc
		mov dword ptr [esp + 0x38], ecx		// 0x00046ec0
L46ec4:
		fcom dword ptr [esp + 0x28]		// 0x00046ec4
		fnstsw ax		// 0x00046ec8
		test ah, 5		// 0x00046eca
		jp L46ed7		// 0x00046ecd
		fst dword ptr [esp + 0x28]		// 0x00046ecf
		mov dword ptr [esp + 0x48], ecx		// 0x00046ed3
L46ed7:
		fcom dword ptr [esp + 0x1c]		// 0x00046ed7
		fnstsw ax		// 0x00046edb
		test ah, 0x41		// 0x00046edd
		jne L46eea		// 0x00046ee0
		fst dword ptr [esp + 0x1c]		// 0x00046ee2
		mov dword ptr [esp + 0x3c], ecx		// 0x00046ee6
L46eea:
		fcom dword ptr [esp + 0x2c]		// 0x00046eea
		fnstsw ax		// 0x00046eee
		test ah, 0x41		// 0x00046ef0
		jne L46eff		// 0x00046ef3
		fstp dword ptr [esp + 0x2c]		// 0x00046ef5
		mov dword ptr [esp + 0x4c], ecx		// 0x00046ef9
		jmp L46f01		// 0x00046efd
L46eff:
		fstp st(0)		// 0x00046eff
L46f01:
		inc ecx		// 0x00046f01
		add edx, 0xc		// 0x00046f02
		cmp ecx, ebx		// 0x00046f05
		jb L46e31		// 0x00046f07
L46f0d:
		xor ebx, ebx		// 0x00046f0d
		nop		// 0x00046f0f
L46f10:
		xor cl, cl		// 0x00046f10
		xor eax, eax		// 0x00046f12
		test ebx, ebx		// 0x00046f14
		jbe L46f31		// 0x00046f16
		mov edx, dword ptr [esp + ebx*4 + 0x30]		// 0x00046f18
		lea esp, [esp]		// 0x00046f1c
L46f20:
		cmp dword ptr [esp + eax*4 + 0x30], edx		// 0x00046f20
		jne L46f28		// 0x00046f24
		mov cl, 1		// 0x00046f26
L46f28:
		inc eax		// 0x00046f28
		cmp eax, ebx		// 0x00046f29
		jb L46f20		// 0x00046f2b
		test cl, cl		// 0x00046f2d
		jne L46f65		// 0x00046f2f
L46f31:
		mov eax, dword ptr [esp + ebx*4 + 0x30]		// 0x00046f31
		mov ecx, dword ptr [esi + 0x9c]		// 0x00046f35
		push 0xffff		// 0x00046f3b
		lea eax, [eax + eax*2]		// 0x00046f40
		push 0xffff		// 0x00046f43
		shl eax, 2		// 0x00046f48
		lea edx, [eax + nxMeshContactNormals]		// 0x00046f4b
		push edx		// 0x00046f51
		mov edx, dword ptr [edi + 0x9c]		// 0x00046f52
		lea eax, [eax + nxMeshContactVertices]		// 0x00046f58
		push eax		// 0x00046f5e
		push 0		// 0x00046f5f
		push ecx		// 0x00046f61
		push edx		// 0x00046f62
		jmp L46fb3		// 0x00046f63
L46f65:
		mov eax, dword ptr [esp + ebx*4 + 0x40]		// 0x00046f65
		xor cl, cl		// 0x00046f69
		xor ebp, ebp		// 0x00046f6b
		lea ecx, [ecx]		// 0x00046f6d
L46f70:
		cmp dword ptr [esp + ebp*4 + 0x30], eax		// 0x00046f70
		jne L46f78		// 0x00046f74
		mov cl, 1		// 0x00046f76
L46f78:
		inc ebp		// 0x00046f78
		cmp ebp, ebx		// 0x00046f79
		jb L46f70		// 0x00046f7b
		test cl, cl		// 0x00046f7d
		jne L46fbf		// 0x00046f7f
		mov dword ptr [esp + ebx*4 + 0x30], eax		// 0x00046f81
		push 0xffff		// 0x00046f85
		lea eax, [eax + eax*2]		// 0x00046f8a
		shl eax, 2		// 0x00046f8d
		push 0xffff		// 0x00046f90
		lea ecx, [eax + nxMeshContactNormals]		// 0x00046f95
		push ecx		// 0x00046f9b
		mov ecx, dword ptr [edi + 0x9c]		// 0x00046f9c
		lea edx, [eax + nxMeshContactVertices]		// 0x00046fa2
		mov eax, dword ptr [esi + 0x9c]		// 0x00046fa8
		push edx		// 0x00046fae
		push 0		// 0x00046faf
		push eax		// 0x00046fb1
		push ecx		// 0x00046fb2
L46fb3:
		mov ecx, dword ptr [esp + 0x110]		// 0x00046fb3
		call nxMMCallNxEmitContactThiscall		// 0x00046fba
L46fbf:
		inc ebx		// 0x00046fbf
		cmp ebx, 4		// 0x00046fc0
		jb L46f10		// 0x00046fc3
		pop edi		// 0x00046fc9
		pop esi		// 0x00046fca
		pop ebp		// 0x00046fcb
		pop ebx		// 0x00046fcc
		add esp, 0xd8		// 0x00046fcd
		ret		// 0x00046fd3
L46fd4:
		mov edx, dword ptr [nxMMImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00046fd4
		cmp dword ptr [edx], 0		// 0x00046fda
		jne L46fe0		// 0x00046fdd
		int 3		// 0x00046fdf
L46fe0:
		push offset nxMMStr107bc4		// 0x00046fe0
		push 0		// 0x00046fe5
		push 0x1ca		// 0x00046fe7
		push offset nxMMStr107d5c		// 0x00046fec
		push 4		// 0x00046ff1
		call dword ptr [nxMMImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00046ff3
		add esp, 0x14		// 0x00046ff9
		pop edi		// 0x00046ffc
		pop esi		// 0x00046ffd
		pop ebp		// 0x00046ffe
		pop ebx		// 0x00046fff
		add esp, 0xd8		// 0x00047000
		ret		// 0x00047006
L47007:
		mov eax, dword ptr [esp + 0xf8]		// 0x00047007
		mov ecx, dword ptr [esp + 0xf4]		// 0x0004700e
		push eax		// 0x00047015
		push ecx		// 0x00047016
		push edi		// 0x00047017
		push esi		// 0x00047018
		call nxMMCallNxContactConvexMesh		// 0x00047019
		add esp, 0x10		// 0x0004701e
		pop edi		// 0x00047021
		pop esi		// 0x00047022
		pop ebp		// 0x00047023
		pop ebx		// 0x00047024
		add esp, 0xd8		// 0x00047025
		ret		// 0x0004702b
L4702c:
		mov edx, dword ptr [esp + 0xf8]		// 0x0004702c
		mov eax, dword ptr [esp + 0xf4]		// 0x00047033
		push edx		// 0x0004703a
		push eax		// 0x0004703b
		push esi		// 0x0004703c
		push edi		// 0x0004703d
		call nxMMCallNxContactConvexMesh		// 0x0004703e
		add esp, 0x10		// 0x00047043
		pop edi		// 0x00047046
		pop esi		// 0x00047047
		pop ebp		// 0x00047048
		pop ebx		// 0x00047049
		add esp, 0xd8		// 0x0004704a
		ret		// 0x00047050
L47051:
		test ebx, ebx		// 0x00047051
		je L4707a		// 0x00047053
		mov ecx, dword ptr [esp + 0xf8]		// 0x00047055
		mov edx, dword ptr [esp + 0xf4]		// 0x0004705c
		push ecx		// 0x00047063
		push edx		// 0x00047064
		push esi		// 0x00047065
		push edi		// 0x00047066
		call nxMMCallNxContactConvexHeightfield		// 0x00047067
		add esp, 0x10		// 0x0004706c
		pop edi		// 0x0004706f
		pop esi		// 0x00047070
		pop ebp		// 0x00047071
		pop ebx		// 0x00047072
		add esp, 0xd8		// 0x00047073
		ret		// 0x00047079
L4707a:
		mov eax, dword ptr [esp + 0xf8]		// 0x0004707a
		mov ecx, dword ptr [esp + 0xf4]		// 0x00047081
		push eax		// 0x00047088
		push ecx		// 0x00047089
		push esi		// 0x0004708a
		push edi		// 0x0004708b
		call nxMMCallNxMeshHeightfieldContact		// 0x0004708c
		add esp, 0x10		// 0x00047091
		pop edi		// 0x00047094
		pop esi		// 0x00047095
		pop ebp		// 0x00047096
		pop ebx		// 0x00047097
		add esp, 0xd8		// 0x00047098
		ret		// 0x0004709e
L4709f:
		test ebp, ebp		// 0x0004709f
		je L470c8		// 0x000470a1
		mov edx, dword ptr [esp + 0xf8]		// 0x000470a3
		mov eax, dword ptr [esp + 0xf4]		// 0x000470aa
		push edx		// 0x000470b1
		push eax		// 0x000470b2
		push edi		// 0x000470b3
		push esi		// 0x000470b4
		call nxMMCallNxContactConvexHeightfield		// 0x000470b5
		add esp, 0x10		// 0x000470ba
		pop edi		// 0x000470bd
		pop esi		// 0x000470be
		pop ebp		// 0x000470bf
		pop ebx		// 0x000470c0
		add esp, 0xd8		// 0x000470c1
		ret		// 0x000470c7
L470c8:
		mov ecx, dword ptr [esp + 0xf8]		// 0x000470c8
		mov edx, dword ptr [esp + 0xf4]		// 0x000470cf
		push ecx		// 0x000470d6
		push edx		// 0x000470d7
		push edi		// 0x000470d8
		push esi		// 0x000470d9
		call nxMMCallNxMeshHeightfieldContact		// 0x000470da
		add esp, 0x10		// 0x000470df
L470e2:
		pop edi		// 0x000470e2
		pop esi		// 0x000470e3
		pop ebp		// 0x000470e4
		pop ebx		// 0x000470e5
		add esp, 0xd8		// 0x000470e6
		ret		// 0x000470ec
		}
	}
// @@ROWS-END@@
