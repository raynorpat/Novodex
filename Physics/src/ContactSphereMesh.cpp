// ContactSphereMesh.cpp: sphere against triangle mesh, matrix A [SPHERE][MESH]
// (phys_fn_001929, the contact) and matrix B (phys_fn_001925, the overlap), with
// the per-triangle row they share (phys_fn_001927). All three are transcribed from
// the pinned x86 listing in the style of the other mesh-contact rows (the compare
// script is docs/reconstruction/novodex-physics/evidence/phase3-sphere-mesh-listing-compare.py).
//
// The overlap row selects first-contact mode in the pair context's SphereCollider
// (context + 0xb0) and reports OPCODE's contact bit. The contact row asks the
// collider for the triangles the sphere touches in the mesh's frame, and hands each
// to 001927, which finds the nearest point on the triangle, the contact normal (the
// face normal, or the smooth vertex normals for a mesh that has them, or the
// vertical for a heightfield) and emits the contact. The C++ version this replaces
// matched the ordinary-face and one edge case and left the rest open.

#include "NxPhysicsSDK.h"
#include "ContactGeneration.h"

extern "C" void nxSMCallSphereCollide();
extern "C" void nxSMCallShapeOwner();
extern "C" void nxSMCallContinuousCd();
extern "C" void nxSMCallTrianglePlane();
extern "C" void nxSMCallEmit();
extern "C" void nxSMCallPointTriDist();
extern "C" void nxSMCallVertexNormals();
#pragma comment(linker, "/alternatename:_nxSMCallSphereCollide=?Collide@SphereCollider@Opcode@@QAE_NAAUSphereCache@2@ABVSphere@IceMaths@@ABVModel@2@PBVMatrix4x4@5@3@Z")
#pragma comment(linker, "/alternatename:_nxSMCallShapeOwner=?NxShapeOwner@@YIPBXPBUNxCollisionShape@@PAX@Z")
#pragma comment(linker, "/alternatename:_nxSMCallContinuousCd=?NxContinuousCdPair@@YA_NPBUNxCollisionShape@@0PAUNxContactSink@@@Z")
#pragma comment(linker, "/alternatename:_nxSMCallTrianglePlane=?NxTrianglePlane@@YIPAVNxPlane@@PAV1@PAXPBVNxVec3@@22@Z")
#pragma comment(linker, "/alternatename:_nxSMCallEmit=?NxEmitContactFeatures@@YIXPAUNxContactSink@@IPAX1IPBVNxVec3@@2IIII@Z")
#pragma comment(linker, "/alternatename:_nxSMCallPointTriDist=?NxPointTriangleSquareDistance@@YANPBM000PAM1@Z")
#pragma comment(linker, "/alternatename:_nxSMCallVertexNormals=?nxMeshComputeVertexNormals@@YAXXZ")

// The two dword constants the listings read: 0.0f (0x101041f0) and 1.0f (0x101041ec).
static const unsigned nxSMZero = 0x00000000;
static const unsigned nxSMOne = 0x3f800000;

void nxSphereMeshTriangle();

// phys_fn_001925 (0x0004a820, 274 B), 80 instructions
__declspec(naked) bool __cdecl NxOverlapSphereMesh(const NxCollisionShape*, const NxCollisionShape*, void*)
	{
	__asm
		{
		sub esp, 0x5c		// 0x0004a820
		push esi		// 0x0004a823
		mov esi, dword ptr [esp + 0x6c]		// 0x0004a824
		push edi		// 0x0004a828
		mov edi, dword ptr [esi + 0xb4]		// 0x0004a829
		or edi, 1		// 0x0004a82f
		mov dword ptr [esi + 0xb4], edi		// 0x0004a832
		mov edx, edi		// 0x0004a838
		lea ecx, [esi + 0xb0]		// 0x0004a83a
		and edx, 0xfffffffd		// 0x0004a840
		mov dword ptr [ecx + 4], edx		// 0x0004a843
		mov eax, edx		// 0x0004a846
		mov edx, dword ptr [esp + 0x68]		// 0x0004a848
		and eax, 0xffffffef		// 0x0004a84c
		mov dword ptr [ecx + 4], eax		// 0x0004a84f
		fld dword ptr [edx + 0x30]		// 0x0004a852
		mov eax, dword ptr [edx + 0x38]		// 0x0004a855
		fld dword ptr [edx + 0x34]		// 0x0004a858
		mov dword ptr [esp + 0x10], eax		// 0x0004a85b
		fxch st(1)		// 0x0004a85f
		mov eax, dword ptr [esp + 0x6c]		// 0x0004a861
		mov edi, dword ptr [eax + 0xc]		// 0x0004a865
		mov dword ptr [esp + 0x24], edi		// 0x0004a868
		mov edi, dword ptr [eax + 0x10]		// 0x0004a86c
		mov dword ptr [esp + 0x34], edi		// 0x0004a86f
		mov edi, dword ptr [eax + 0x14]		// 0x0004a873
		mov dword ptr [esp + 0x44], edi		// 0x0004a876
		mov edi, dword ptr [eax + 0x18]		// 0x0004a87a
		mov edx, dword ptr [edx + 0xe0]		// 0x0004a87d
		mov dword ptr [esp + 0x28], edi		// 0x0004a883
		mov edi, dword ptr [eax + 0x1c]		// 0x0004a887
		mov dword ptr [esp + 0x38], edi		// 0x0004a88a
		mov edi, dword ptr [eax + 0x20]		// 0x0004a88e
		mov dword ptr [esp + 0x48], edi		// 0x0004a891
		mov edi, dword ptr [eax + 0x24]		// 0x0004a895
		mov dword ptr [esp + 0x2c], edi		// 0x0004a898
		mov edi, dword ptr [eax + 0x28]		// 0x0004a89c
		mov dword ptr [esp + 0x3c], edi		// 0x0004a89f
		mov edi, dword ptr [eax + 0x2c]		// 0x0004a8a3
		mov dword ptr [esp + 0x4c], edi		// 0x0004a8a6
		mov edi, dword ptr [eax + 0x30]		// 0x0004a8aa
		mov dword ptr [esp + 0x54], edi		// 0x0004a8ad
		mov edi, dword ptr [eax + 0x34]		// 0x0004a8b1
		mov dword ptr [esp + 0x58], edi		// 0x0004a8b4
		mov edi, dword ptr [eax + 0x38]		// 0x0004a8b8
		mov eax, dword ptr [eax + 0xe0]		// 0x0004a8bb
		mov dword ptr [esp + 0x5c], edi		// 0x0004a8c1
		mov edi, dword ptr [esp + 0x10]		// 0x0004a8c5
		mov dword ptr [esp + 0x50], 0		// 0x0004a8c9
		mov dword ptr [esp + 0x40], 0		// 0x0004a8d1
		mov dword ptr [esp + 0x30], 0		// 0x0004a8d9
		mov dword ptr [esp + 0x60], 0x3f800000		// 0x0004a8e1
		mov eax, dword ptr [eax + 0x28]		// 0x0004a8e9
		fstp dword ptr [esp + 0x14]		// 0x0004a8ec
		mov dword ptr [esp + 0x20], edx		// 0x0004a8f0
		lea edx, [esp + 0x24]		// 0x0004a8f4
		fstp dword ptr [esp + 0x18]		// 0x0004a8f8
		push edx		// 0x0004a8fc
		push 0		// 0x0004a8fd
		push eax		// 0x0004a8ff
		lea eax, [esp + 0x20]		// 0x0004a900
		push eax		// 0x0004a904
		lea edx, [esi + 0xf4]		// 0x0004a905
		push edx		// 0x0004a90b
		mov dword ptr [esp + 0x30], edi		// 0x0004a90c
		call nxSMCallSphereCollide		// 0x0004a910
		test al, al		// 0x0004a915
		je L4a92a		// 0x0004a917
		test byte ptr [esi + 0xb4], 4		// 0x0004a919
		je L4a92a		// 0x0004a920
		pop edi		// 0x0004a922
		mov al, 1		// 0x0004a923
		pop esi		// 0x0004a925
		add esp, 0x5c		// 0x0004a926
		ret		// 0x0004a929
L4a92a:
		pop edi		// 0x0004a92a
		xor al, al		// 0x0004a92b
		pop esi		// 0x0004a92d
		add esp, 0x5c		// 0x0004a92e
		ret		// 0x0004a931
		}
	}

// phys_fn_001927 (0x0004a940, 2223 B), 682 instructions
__declspec(naked) void nxSphereMeshTriangle()
	{
	__asm
		{
		sub esp, 0x58		// 0x0004a940
		push ebx		// 0x0004a943
		mov ebx, ecx		// 0x0004a944
		mov ecx, dword ptr [esp + 0x60]		// 0x0004a946
		fld dword ptr [ecx]		// 0x0004a94a
		push ebp		// 0x0004a94c
		push esi		// 0x0004a94d
		mov esi, eax		// 0x0004a94e
		fsub dword ptr [esi + 0x24]		// 0x0004a950
		mov ebp, dword ptr [esp + 0x78]		// 0x0004a953
		fld dword ptr [ecx + 4]		// 0x0004a957
		push edi		// 0x0004a95a
		fsub dword ptr [esi + 0x28]		// 0x0004a95b
		mov edi, edx		// 0x0004a95e
		fld dword ptr [ecx + 8]		// 0x0004a960
		fsub dword ptr [esi + 0x2c]		// 0x0004a963
		fld st(2)		// 0x0004a966
		fmul dword ptr [esi]		// 0x0004a968
		fld st(1)		// 0x0004a96a
		fmul dword ptr [esi + 0x18]		// 0x0004a96c
		faddp st(1), st(0)		// 0x0004a96f
		fld st(2)		// 0x0004a971
		fmul dword ptr [esi + 0xc]		// 0x0004a973
		faddp st(1), st(0)		// 0x0004a976
		fstp dword ptr [esp + 0x5c]		// 0x0004a978
		fld st(2)		// 0x0004a97c
		fmul dword ptr [esi + 4]		// 0x0004a97e
		fld st(1)		// 0x0004a981
		fmul dword ptr [esi + 0x1c]		// 0x0004a983
		faddp st(1), st(0)		// 0x0004a986
		fld st(2)		// 0x0004a988
		fmul dword ptr [esi + 0x10]		// 0x0004a98a
		faddp st(1), st(0)		// 0x0004a98d
		fstp dword ptr [esp + 0x60]		// 0x0004a98f
		fmul dword ptr [esi + 0x20]		// 0x0004a993
		fxch st(2)		// 0x0004a996
		fmul dword ptr [esi + 8]		// 0x0004a998
		faddp st(2), st(0)		// 0x0004a99b
		fmul dword ptr [esi + 0x14]		// 0x0004a99d
		faddp st(1), st(0)		// 0x0004a9a0
		fstp dword ptr [esp + 0x64]		// 0x0004a9a2
		fld dword ptr [ebx]		// 0x0004a9a6
		fsub dword ptr [edi]		// 0x0004a9a8
		fstp dword ptr [esp + 0x44]		// 0x0004a9aa
		fld dword ptr [ebx + 4]		// 0x0004a9ae
		fsub dword ptr [edi + 4]		// 0x0004a9b1
		fstp dword ptr [esp + 0x48]		// 0x0004a9b4
		fld dword ptr [ebx + 8]		// 0x0004a9b8
		fsub dword ptr [edi + 8]		// 0x0004a9bb
		fstp dword ptr [esp + 0x4c]		// 0x0004a9be
		fld dword ptr [ebp]		// 0x0004a9c2
		fsub dword ptr [edi]		// 0x0004a9c5
		fstp dword ptr [esp + 0x38]		// 0x0004a9c7
		fld dword ptr [ebp + 4]		// 0x0004a9cb
		fsub dword ptr [edi + 4]		// 0x0004a9ce
		fstp dword ptr [esp + 0x3c]		// 0x0004a9d1
		fld dword ptr [ebp + 8]		// 0x0004a9d5
		fsub dword ptr [edi + 8]		// 0x0004a9d8
		fst dword ptr [esp + 0x40]		// 0x0004a9db
		fmul dword ptr [esp + 0x48]		// 0x0004a9df
		fld dword ptr [esp + 0x3c]		// 0x0004a9e3
		fmul dword ptr [esp + 0x4c]		// 0x0004a9e7
		fsubp st(1), st(0)		// 0x0004a9eb
		fld dword ptr [esp + 0x4c]		// 0x0004a9ed
		fmul dword ptr [esp + 0x38]		// 0x0004a9f1
		fld dword ptr [esp + 0x40]		// 0x0004a9f5
		fmul dword ptr [esp + 0x44]		// 0x0004a9f9
		fsubp st(1), st(0)		// 0x0004a9fd
		fstp dword ptr [esp + 0x54]		// 0x0004a9ff
		fld dword ptr [esp + 0x3c]		// 0x0004aa03
		fmul dword ptr [esp + 0x44]		// 0x0004aa07
		fld dword ptr [esp + 0x48]		// 0x0004aa0b
		fmul dword ptr [esp + 0x38]		// 0x0004aa0f
		fsubp st(1), st(0)		// 0x0004aa13
		fld st(1)		// 0x0004aa15
		fstp dword ptr [esp + 0x2c]		// 0x0004aa17
		fst dword ptr [esp + 0x34]		// 0x0004aa1b
		mov eax, dword ptr [esp + 0x54]		// 0x0004aa1f
		fld st(1)		// 0x0004aa23
		mov dword ptr [esp + 0x30], eax		// 0x0004aa25
		fmul st(0), st(2)		// 0x0004aa29
		fld st(1)		// 0x0004aa2b
		fmul st(0), st(2)		// 0x0004aa2d
		faddp st(1), st(0)		// 0x0004aa2f
		fld dword ptr [esp + 0x54]		// 0x0004aa31
		fmul dword ptr [esp + 0x54]		// 0x0004aa35
		faddp st(1), st(0)		// 0x0004aa39
		fsqrt		// 0x0004aa3b
		fld dword ptr [nxSMZero]		// 0x0004aa3d
		fld st(1)		// 0x0004aa43
		fucompp		// 0x0004aa45
		fnstsw ax		// 0x0004aa47
		test ah, 0x44		// 0x0004aa49
		jnp L4aa6e		// 0x0004aa4c
		fdivr dword ptr [nxSMOne]		// 0x0004aa4e
		fxch st(2)		// 0x0004aa54
		fmul st(0), st(2)		// 0x0004aa56
		fstp dword ptr [esp + 0x2c]		// 0x0004aa58
		fld dword ptr [esp + 0x54]		// 0x0004aa5c
		fmul st(0), st(2)		// 0x0004aa60
		fstp dword ptr [esp + 0x30]		// 0x0004aa62
		fmul st(0), st(1)		// 0x0004aa66
		fstp dword ptr [esp + 0x34]		// 0x0004aa68
		jmp L4aa72		// 0x0004aa6c
L4aa6e:
		fstp st(0)		// 0x0004aa6e
		fstp st(0)		// 0x0004aa70
L4aa72:
		fstp st(0)		// 0x0004aa72
		mov edx, dword ptr [esp + 0x70]		// 0x0004aa74
		fld dword ptr [esp + 0x5c]		// 0x0004aa78
		fsub dword ptr [edi]		// 0x0004aa7c
		fstp dword ptr [esp + 0x50]		// 0x0004aa7e
		fld dword ptr [esp + 0x60]		// 0x0004aa82
		fsub dword ptr [edi + 4]		// 0x0004aa86
		fstp dword ptr [esp + 0x54]		// 0x0004aa89
		fld dword ptr [esp + 0x64]		// 0x0004aa8d
		fsub dword ptr [edi + 8]		// 0x0004aa91
		fstp dword ptr [esp + 0x58]		// 0x0004aa94
		fld dword ptr [esp + 0x50]		// 0x0004aa98
		fmul dword ptr [esp + 0x2c]		// 0x0004aa9c
		fld dword ptr [esp + 0x58]		// 0x0004aaa0
		fmul dword ptr [esp + 0x34]		// 0x0004aaa4
		faddp st(1), st(0)		// 0x0004aaa8
		fld dword ptr [esp + 0x54]		// 0x0004aaaa
		fmul dword ptr [esp + 0x30]		// 0x0004aaae
		faddp st(1), st(0)		// 0x0004aab2
		fst dword ptr [esp + 0x7c]		// 0x0004aab4
		fcomp dword ptr [edx + 0xe0]		// 0x0004aab8
		fnstsw ax		// 0x0004aabe
		test ah, 0x41		// 0x0004aac0
		je L4b1e7		// 0x0004aac3
		fld dword ptr [esp + 0x7c]		// 0x0004aac9
		fcomp dword ptr [nxSMZero]		// 0x0004aacd
		fnstsw ax		// 0x0004aad3
		test ah, 5		// 0x0004aad5
		jnp L4b1e7		// 0x0004aad8
		fld dword ptr [esp + 0x44]		// 0x0004aade
		fmul dword ptr [esp + 0x44]		// 0x0004aae2
		fld dword ptr [esp + 0x4c]		// 0x0004aae6
		fmul dword ptr [esp + 0x4c]		// 0x0004aaea
		faddp st(1), st(0)		// 0x0004aaee
		fld dword ptr [esp + 0x48]		// 0x0004aaf0
		fmul dword ptr [esp + 0x48]		// 0x0004aaf4
		faddp st(1), st(0)		// 0x0004aaf8
		fstp dword ptr [esp + 0x28]		// 0x0004aafa
		fld dword ptr [esp + 0x38]		// 0x0004aafe
		mov eax, dword ptr [esp + 0x28]		// 0x0004ab02
		fmul dword ptr [esp + 0x44]		// 0x0004ab06
		mov dword ptr [esp + 0x20], eax		// 0x0004ab0a
		fld dword ptr [esp + 0x40]		// 0x0004ab0e
		fmul dword ptr [esp + 0x4c]		// 0x0004ab12
		faddp st(1), st(0)		// 0x0004ab16
		fld dword ptr [esp + 0x3c]		// 0x0004ab18
		fmul dword ptr [esp + 0x48]		// 0x0004ab1c
		faddp st(1), st(0)		// 0x0004ab20
		fstp dword ptr [esp + 0x10]		// 0x0004ab22
		fld dword ptr [esp + 0x38]		// 0x0004ab26
		fmul dword ptr [esp + 0x38]		// 0x0004ab2a
		fld dword ptr [esp + 0x40]		// 0x0004ab2e
		fmul dword ptr [esp + 0x40]		// 0x0004ab32
		faddp st(1), st(0)		// 0x0004ab36
		fld dword ptr [esp + 0x3c]		// 0x0004ab38
		fmul dword ptr [esp + 0x3c]		// 0x0004ab3c
		faddp st(1), st(0)		// 0x0004ab40
		fstp dword ptr [esp + 0x24]		// 0x0004ab42
		fld dword ptr [esp + 0x50]		// 0x0004ab46
		mov eax, dword ptr [esp + 0x24]		// 0x0004ab4a
		fmul dword ptr [esp + 0x44]		// 0x0004ab4e
		mov dword ptr [esp + 0x1c], eax		// 0x0004ab52
		fld dword ptr [esp + 0x58]		// 0x0004ab56
		fmul dword ptr [esp + 0x4c]		// 0x0004ab5a
		faddp st(1), st(0)		// 0x0004ab5e
		fld dword ptr [esp + 0x54]		// 0x0004ab60
		fmul dword ptr [esp + 0x48]		// 0x0004ab64
		faddp st(1), st(0)		// 0x0004ab68
		fstp dword ptr [esp + 0x18]		// 0x0004ab6a
		fld dword ptr [esp + 0x50]		// 0x0004ab6e
		fmul dword ptr [esp + 0x38]		// 0x0004ab72
		fld dword ptr [esp + 0x58]		// 0x0004ab76
		fmul dword ptr [esp + 0x40]		// 0x0004ab7a
		faddp st(1), st(0)		// 0x0004ab7e
		fld dword ptr [esp + 0x54]		// 0x0004ab80
		fmul dword ptr [esp + 0x3c]		// 0x0004ab84
		faddp st(1), st(0)		// 0x0004ab88
		fstp dword ptr [esp + 0x14]		// 0x0004ab8a
		fld dword ptr [esp + 0x18]		// 0x0004ab8e
		fmul dword ptr [esp + 0x1c]		// 0x0004ab92
		fld dword ptr [esp + 0x14]		// 0x0004ab96
		fmul dword ptr [esp + 0x10]		// 0x0004ab9a
		fsubp st(1), st(0)		// 0x0004ab9e
		fld dword ptr [esp + 0x14]		// 0x0004aba0
		fmul dword ptr [esp + 0x20]		// 0x0004aba4
		fld dword ptr [esp + 0x18]		// 0x0004aba8
		fmul dword ptr [esp + 0x10]		// 0x0004abac
		fsubp st(1), st(0)		// 0x0004abb0
		fst dword ptr [esp + 0x14]		// 0x0004abb2
		fadd st(0), st(1)		// 0x0004abb6
		fld dword ptr [esp + 0x1c]		// 0x0004abb8
		fmul dword ptr [esp + 0x20]		// 0x0004abbc
		fld dword ptr [esp + 0x10]		// 0x0004abc0
		fmul dword ptr [esp + 0x10]		// 0x0004abc4
		fsubp st(1), st(0)		// 0x0004abc8
		fcompp		// 0x0004abca
		fnstsw ax		// 0x0004abcc
		fcomp dword ptr [nxSMZero]		// 0x0004abce
		test ah, 1		// 0x0004abd4
		fnstsw ax		// 0x0004abd7
		jne L4ad45		// 0x0004abd9
		fld dword ptr [esp + 0x14]		// 0x0004abdf
		test ah, 5		// 0x0004abe3
		fcomp dword ptr [nxSMZero]		// 0x0004abe6
		fnstsw ax		// 0x0004abec
		jp L4ac6f		// 0x0004abee
		test ah, 5		// 0x0004abf0
		jp L4abfc		// 0x0004abf3
		mov eax, edi		// 0x0004abf5
		jmp L4ae2b		// 0x0004abf7
L4abfc:
		fld dword ptr [esp + 0x24]		// 0x0004abfc
		fsqrt		// 0x0004ac00
		fld dword ptr [nxSMZero]		// 0x0004ac02
		fld st(1)		// 0x0004ac08
		fucompp		// 0x0004ac0a
		fnstsw ax		// 0x0004ac0c
		test ah, 0x44		// 0x0004ac0e
		jnp L4ac37		// 0x0004ac11
		fdivr dword ptr [nxSMOne]		// 0x0004ac13
		fld dword ptr [esp + 0x38]		// 0x0004ac19
		fmul st(0), st(1)		// 0x0004ac1d
		fstp dword ptr [esp + 0x38]		// 0x0004ac1f
		fld dword ptr [esp + 0x3c]		// 0x0004ac23
		fmul st(0), st(1)		// 0x0004ac27
		fstp dword ptr [esp + 0x3c]		// 0x0004ac29
		fld dword ptr [esp + 0x40]		// 0x0004ac2d
		fmul st(0), st(1)		// 0x0004ac31
		fstp dword ptr [esp + 0x40]		// 0x0004ac33
L4ac37:
		fstp st(0)		// 0x0004ac37
		fld dword ptr [esp + 0x50]		// 0x0004ac39
		fmul dword ptr [esp + 0x38]		// 0x0004ac3d
		fld dword ptr [esp + 0x58]		// 0x0004ac41
		fmul dword ptr [esp + 0x40]		// 0x0004ac45
		faddp st(1), st(0)		// 0x0004ac49
		fld dword ptr [esp + 0x54]		// 0x0004ac4b
		fmul dword ptr [esp + 0x3c]		// 0x0004ac4f
		faddp st(1), st(0)		// 0x0004ac53
		fld dword ptr [esp + 0x38]		// 0x0004ac55
		fmul st(0), st(1)		// 0x0004ac59
		fstp dword ptr [esp + 0x50]		// 0x0004ac5b
		fld dword ptr [esp + 0x3c]		// 0x0004ac5f
		fmul st(0), st(1)		// 0x0004ac63
		fstp dword ptr [esp + 0x54]		// 0x0004ac65
		fmul dword ptr [esp + 0x40]		// 0x0004ac69
		jmp L4ace9		// 0x0004ac6d
L4ac6f:
		test ah, 5		// 0x0004ac6f
		jp L4ad00		// 0x0004ac72
		fld dword ptr [esp + 0x28]		// 0x0004ac78
		fsqrt		// 0x0004ac7c
		fld dword ptr [nxSMZero]		// 0x0004ac7e
		fld st(1)		// 0x0004ac84
		fucompp		// 0x0004ac86
		fnstsw ax		// 0x0004ac88
		test ah, 0x44		// 0x0004ac8a
		jnp L4acb3		// 0x0004ac8d
		fdivr dword ptr [nxSMOne]		// 0x0004ac8f
		fld dword ptr [esp + 0x44]		// 0x0004ac95
		fmul st(0), st(1)		// 0x0004ac99
		fstp dword ptr [esp + 0x44]		// 0x0004ac9b
		fld dword ptr [esp + 0x48]		// 0x0004ac9f
		fmul st(0), st(1)		// 0x0004aca3
		fstp dword ptr [esp + 0x48]		// 0x0004aca5
		fld dword ptr [esp + 0x4c]		// 0x0004aca9
		fmul st(0), st(1)		// 0x0004acad
		fstp dword ptr [esp + 0x4c]		// 0x0004acaf
L4acb3:
		fstp st(0)		// 0x0004acb3
		fld dword ptr [esp + 0x50]		// 0x0004acb5
		fmul dword ptr [esp + 0x44]		// 0x0004acb9
		fld dword ptr [esp + 0x58]		// 0x0004acbd
		fmul dword ptr [esp + 0x4c]		// 0x0004acc1
		faddp st(1), st(0)		// 0x0004acc5
		fld dword ptr [esp + 0x54]		// 0x0004acc7
		fmul dword ptr [esp + 0x48]		// 0x0004accb
		faddp st(1), st(0)		// 0x0004accf
		fld dword ptr [esp + 0x44]		// 0x0004acd1
		fmul st(0), st(1)		// 0x0004acd5
		fstp dword ptr [esp + 0x50]		// 0x0004acd7
		fld dword ptr [esp + 0x48]		// 0x0004acdb
		fmul st(0), st(1)		// 0x0004acdf
		fstp dword ptr [esp + 0x54]		// 0x0004ace1
		fmul dword ptr [esp + 0x4c]		// 0x0004ace5
L4ace9:
		fld dword ptr [esp + 0x50]		// 0x0004ace9
		fadd dword ptr [edi]		// 0x0004aced
		fld dword ptr [esp + 0x54]		// 0x0004acef
		fadd dword ptr [edi + 4]		// 0x0004acf3
		fxch st(2)		// 0x0004acf6
		fadd dword ptr [edi + 8]		// 0x0004acf8
		jmp L4ae13		// 0x0004acfb
L4ad00:
		fld dword ptr [esp + 0x2c]		// 0x0004ad00
		fmul dword ptr [esp + 0x7c]		// 0x0004ad04
		fld dword ptr [esp + 0x30]		// 0x0004ad08
		fmul dword ptr [esp + 0x7c]		// 0x0004ad0c
		fld dword ptr [esp + 0x34]		// 0x0004ad10
		fmul dword ptr [esp + 0x7c]		// 0x0004ad14
		fstp dword ptr [esp + 0x58]		// 0x0004ad18
		fld dword ptr [esp + 0x5c]		// 0x0004ad1c
		fsub st(0), st(2)		// 0x0004ad20
		fstp dword ptr [esp + 0x44]		// 0x0004ad22
		fld dword ptr [esp + 0x60]		// 0x0004ad26
		fsub st(0), st(1)		// 0x0004ad2a
		fstp dword ptr [esp + 0x48]		// 0x0004ad2c
		fstp st(0)		// 0x0004ad30
		fstp st(0)		// 0x0004ad32
		fld dword ptr [esp + 0x64]		// 0x0004ad34
		fsub dword ptr [esp + 0x58]		// 0x0004ad38
		fstp dword ptr [esp + 0x4c]		// 0x0004ad3c
		jmp L4ae27		// 0x0004ad40
L4ad45:
		test ah, 5		// 0x0004ad45
		jp L4ad51		// 0x0004ad48
		mov eax, ebp		// 0x0004ad4a
		jmp L4ae2b		// 0x0004ad4c
L4ad51:
		fld dword ptr [esp + 0x14]		// 0x0004ad51
		fcomp dword ptr [nxSMZero]		// 0x0004ad55
		fnstsw ax		// 0x0004ad5b
		test ah, 5		// 0x0004ad5d
		jp L4ad69		// 0x0004ad60
		mov eax, ebx		// 0x0004ad62
		jmp L4ae2b		// 0x0004ad64
L4ad69:
		fld dword ptr [ebp]		// 0x0004ad69
		fsub dword ptr [ebx]		// 0x0004ad6c
		fld dword ptr [ebp + 4]		// 0x0004ad6e
		fsub dword ptr [ebx + 4]		// 0x0004ad71
		fld dword ptr [ebp + 8]		// 0x0004ad74
		fsub dword ptr [ebx + 8]		// 0x0004ad77
		fst dword ptr [esp + 0x58]		// 0x0004ad7a
		fmul dword ptr [esp + 0x58]		// 0x0004ad7e
		fld st(1)		// 0x0004ad82
		fmul st(0), st(2)		// 0x0004ad84
		faddp st(1), st(0)		// 0x0004ad86
		fld st(2)		// 0x0004ad88
		fmul st(0), st(3)		// 0x0004ad8a
		faddp st(1), st(0)		// 0x0004ad8c
		fsqrt		// 0x0004ad8e
		fld dword ptr [nxSMZero]		// 0x0004ad90
		fld st(1)		// 0x0004ad96
		fucompp		// 0x0004ad98
		fnstsw ax		// 0x0004ad9a
		test ah, 0x44		// 0x0004ad9c
		jnp L4adbd		// 0x0004ad9f
		fdivr dword ptr [nxSMOne]		// 0x0004ada1
		fxch st(2)		// 0x0004ada7
		fmul st(0), st(2)		// 0x0004ada9
		fxch st(2)		// 0x0004adab
		fxch st(1)		// 0x0004adad
		fmul st(0), st(1)		// 0x0004adaf
		fxch st(1)		// 0x0004adb1
		fld dword ptr [esp + 0x58]		// 0x0004adb3
		fmul st(0), st(1)		// 0x0004adb7
		fstp dword ptr [esp + 0x58]		// 0x0004adb9
L4adbd:
		fstp st(0)		// 0x0004adbd
		fld dword ptr [esp + 0x5c]		// 0x0004adbf
		fsub dword ptr [ebx]		// 0x0004adc3
		fld dword ptr [esp + 0x60]		// 0x0004adc5
		fsub dword ptr [ebx + 4]		// 0x0004adc9
		fld dword ptr [esp + 0x64]		// 0x0004adcc
		fsub dword ptr [ebx + 8]		// 0x0004add0
		fmul dword ptr [esp + 0x58]		// 0x0004add3
		fxch st(1)		// 0x0004add7
		fmul st(0), st(3)		// 0x0004add9
		faddp st(1), st(0)		// 0x0004addb
		fxch st(1)		// 0x0004addd
		fmul st(0), st(3)		// 0x0004addf
		faddp st(1), st(0)		// 0x0004ade1
		fstp dword ptr [esp + 0x7c]		// 0x0004ade3
		fxch st(1)		// 0x0004ade7
		fmul dword ptr [esp + 0x7c]		// 0x0004ade9
		fstp dword ptr [esp + 0x44]		// 0x0004aded
		fmul dword ptr [esp + 0x7c]		// 0x0004adf1
		fstp dword ptr [esp + 0x48]		// 0x0004adf5
		fld dword ptr [esp + 0x58]		// 0x0004adf9
		fmul dword ptr [esp + 0x7c]		// 0x0004adfd
		fld dword ptr [esp + 0x44]		// 0x0004ae01
		fadd dword ptr [ebx]		// 0x0004ae05
		fld dword ptr [esp + 0x48]		// 0x0004ae07
		fadd dword ptr [ebx + 4]		// 0x0004ae0b
		fxch st(2)		// 0x0004ae0e
		fadd dword ptr [ebx + 8]		// 0x0004ae10
L4ae13:
		fstp dword ptr [esp + 0x58]		// 0x0004ae13
		mov eax, dword ptr [esp + 0x58]		// 0x0004ae17
		mov dword ptr [esp + 0x4c], eax		// 0x0004ae1b
		fstp dword ptr [esp + 0x44]		// 0x0004ae1f
		fstp dword ptr [esp + 0x48]		// 0x0004ae23
L4ae27:
		lea eax, [esp + 0x44]		// 0x0004ae27
L4ae2b:
		fld dword ptr [esp + 0x5c]		// 0x0004ae2b
		fsub dword ptr [eax]		// 0x0004ae2f
		fstp dword ptr [esp + 0x50]		// 0x0004ae31
		fld dword ptr [esp + 0x60]		// 0x0004ae35
		fsub dword ptr [eax + 4]		// 0x0004ae39
		fstp dword ptr [esp + 0x54]		// 0x0004ae3c
		fld dword ptr [esp + 0x64]		// 0x0004ae40
		fsub dword ptr [eax + 8]		// 0x0004ae44
		fst dword ptr [esp + 0x58]		// 0x0004ae47
		fmul dword ptr [esp + 0x58]		// 0x0004ae4b
		fld dword ptr [esp + 0x54]		// 0x0004ae4f
		fmul dword ptr [esp + 0x54]		// 0x0004ae53
		faddp st(1), st(0)		// 0x0004ae57
		fld dword ptr [esp + 0x50]		// 0x0004ae59
		fmul dword ptr [esp + 0x50]		// 0x0004ae5d
		faddp st(1), st(0)		// 0x0004ae61
		fstp dword ptr [esp + 0x7c]		// 0x0004ae63
		fld dword ptr [edx + 0xe0]		// 0x0004ae67
		fld st(0)		// 0x0004ae6d
		fmul st(0), st(1)		// 0x0004ae6f
		fld dword ptr [esp + 0x7c]		// 0x0004ae71
		fcompp		// 0x0004ae75
		fnstsw ax		// 0x0004ae77
		fstp st(0)		// 0x0004ae79
		test ah, 0x41		// 0x0004ae7b
		je L4b1e7		// 0x0004ae7e
		fld dword ptr [nxSMZero]		// 0x0004ae84
		fld dword ptr [esp + 0x7c]		// 0x0004ae8a
		fucompp		// 0x0004ae8e
		fnstsw ax		// 0x0004ae90
		test ah, 0x44		// 0x0004ae92
		jnp L4b1e7		// 0x0004ae95
		mov eax, dword ptr [esp + 0x74]		// 0x0004ae9b
		test byte ptr [eax + 0xe4], 1		// 0x0004ae9f
		je L4b081		// 0x0004aea6
		mov eax, dword ptr [esp + 0x78]		// 0x0004aeac
		mov edx, dword ptr [eax + 0x20]		// 0x0004aeb0
		test edx, edx		// 0x0004aeb3
		lea ecx, [eax + 8]		// 0x0004aeb5
		jne L4aec3		// 0x0004aeb8
		call nxSMCallVertexNormals		// 0x0004aeba
		mov eax, dword ptr [esp + 0x78]		// 0x0004aebf
L4aec3:
		mov ecx, dword ptr [eax + 0x20]		// 0x0004aec3
		mov eax, dword ptr [eax + 0x14]		// 0x0004aec6
		mov dword ptr [esp + 0x20], ecx		// 0x0004aec9
		mov ecx, dword ptr [esp + 0x84]		// 0x0004aecd
		lea edx, [ecx + ecx*2]		// 0x0004aed4
		lea ecx, [eax + edx*4]		// 0x0004aed7
		lea edx, [esp + 0x14]		// 0x0004aeda
		push edx		// 0x0004aede
		lea eax, [esp + 0x80]		// 0x0004aedf
		push eax		// 0x0004aee6
		push ebp		// 0x0004aee7
		push ebx		// 0x0004aee8
		mov dword ptr [esp + 0x34], ecx		// 0x0004aee9
		lea ecx, [esp + 0x6c]		// 0x0004aeed
		push edi		// 0x0004aef1
		push ecx		// 0x0004aef2
		call nxSMCallPointTriDist		// 0x0004aef3
		fsqrt		// 0x0004aef8
		mov ebx, dword ptr [esp + 0x88]		// 0x0004aefa
		add esp, 0x18		// 0x0004af01
		fsub dword ptr [ebx + 0xe0]		// 0x0004af04
		fst dword ptr [esp + 0x28]		// 0x0004af0a
		fcomp dword ptr [nxSMZero]		// 0x0004af0e
		fnstsw ax		// 0x0004af14
		test ah, 1		// 0x0004af16
		je L4b1e7		// 0x0004af19
		mov edx, dword ptr [esp + 0x24]		// 0x0004af1f
		fld dword ptr [nxSMOne]		// 0x0004af23
		fsub dword ptr [esp + 0x7c]		// 0x0004af29
		mov eax, dword ptr [edx + 8]		// 0x0004af2d
		mov ecx, dword ptr [edx + 4]		// 0x0004af30
		mov edi, dword ptr [esp + 0x20]		// 0x0004af33
		fsub dword ptr [esp + 0x14]		// 0x0004af37
		mov edx, dword ptr [edx]		// 0x0004af3b
		fld dword ptr [esp + 0x14]		// 0x0004af3d
		lea eax, [eax + eax*2]		// 0x0004af41
		fmul dword ptr [edi + eax*4]		// 0x0004af44
		lea eax, [edi + eax*4]		// 0x0004af47
		fld dword ptr [esp + 0x7c]		// 0x0004af4a
		lea ecx, [ecx + ecx*2]		// 0x0004af4e
		fmul dword ptr [edi + ecx*4]		// 0x0004af51
		lea ecx, [edi + ecx*4]		// 0x0004af54
		lea edx, [edx + edx*2]		// 0x0004af57
		lea edx, [edi + edx*4]		// 0x0004af5a
		faddp st(1), st(0)		// 0x0004af5d
		fld st(1)		// 0x0004af5f
		fmul dword ptr [edx]		// 0x0004af61
		faddp st(1), st(0)		// 0x0004af63
		fld dword ptr [esp + 0x7c]		// 0x0004af65
		fmul dword ptr [ecx + 4]		// 0x0004af69
		fld dword ptr [esp + 0x14]		// 0x0004af6c
		fmul dword ptr [eax + 4]		// 0x0004af70
		faddp st(1), st(0)		// 0x0004af73
		fld st(2)		// 0x0004af75
		fmul dword ptr [edx + 4]		// 0x0004af77
		faddp st(1), st(0)		// 0x0004af7a
		fld dword ptr [esp + 0x7c]		// 0x0004af7c
		fmul dword ptr [ecx + 8]		// 0x0004af80
		fld dword ptr [esp + 0x14]		// 0x0004af83
		fmul dword ptr [eax + 8]		// 0x0004af87
		faddp st(1), st(0)		// 0x0004af8a
		fxch st(3)		// 0x0004af8c
		fmul dword ptr [edx + 8]		// 0x0004af8e
		faddp st(3), st(0)		// 0x0004af91
		fxch st(2)		// 0x0004af93
		fld st(0)		// 0x0004af95
		fmul dword ptr [esi + 8]		// 0x0004af97
		fld st(3)		// 0x0004af9a
		fmul dword ptr [esi + 4]		// 0x0004af9c
		faddp st(1), st(0)		// 0x0004af9f
		fld st(2)		// 0x0004afa1
		fmul dword ptr [esi]		// 0x0004afa3
		faddp st(1), st(0)		// 0x0004afa5
		fstp dword ptr [esp + 0x2c]		// 0x0004afa7
		fld st(1)		// 0x0004afab
		fmul dword ptr [esi + 0xc]		// 0x0004afad
		fld st(1)		// 0x0004afb0
		fmul dword ptr [esi + 0x14]		// 0x0004afb2
		faddp st(1), st(0)		// 0x0004afb5
		fld st(3)		// 0x0004afb7
		fmul dword ptr [esi + 0x10]		// 0x0004afb9
		faddp st(1), st(0)		// 0x0004afbc
		fstp dword ptr [esp + 0x30]		// 0x0004afbe
		fxch st(1)		// 0x0004afc2
		fmul dword ptr [esi + 0x18]		// 0x0004afc4
		fxch st(1)		// 0x0004afc7
		fmul dword ptr [esi + 0x20]		// 0x0004afc9
		faddp st(1), st(0)		// 0x0004afcc
		fxch st(1)		// 0x0004afce
		fmul dword ptr [esi + 0x1c]		// 0x0004afd0
		faddp st(1), st(0)		// 0x0004afd3
		fstp dword ptr [esp + 0x34]		// 0x0004afd5
		fld dword ptr [esp + 0x2c]		// 0x0004afd9
		fmul dword ptr [esp + 0x2c]		// 0x0004afdd
		fld dword ptr [esp + 0x34]		// 0x0004afe1
		fmul dword ptr [esp + 0x34]		// 0x0004afe5
		faddp st(1), st(0)		// 0x0004afe9
		fld dword ptr [esp + 0x30]		// 0x0004afeb
		fmul dword ptr [esp + 0x30]		// 0x0004afef
		faddp st(1), st(0)		// 0x0004aff3
		fsqrt		// 0x0004aff5
		fld dword ptr [nxSMZero]		// 0x0004aff7
		fld st(1)		// 0x0004affd
		fucompp		// 0x0004afff
		fnstsw ax		// 0x0004b001
		test ah, 0x44		// 0x0004b003
		jnp L4b02c		// 0x0004b006
		fdivr dword ptr [nxSMOne]		// 0x0004b008
		fld dword ptr [esp + 0x2c]		// 0x0004b00e
		fmul st(0), st(1)		// 0x0004b012
		fstp dword ptr [esp + 0x2c]		// 0x0004b014
		fld dword ptr [esp + 0x30]		// 0x0004b018
		fmul st(0), st(1)		// 0x0004b01c
		fstp dword ptr [esp + 0x30]		// 0x0004b01e
		fld dword ptr [esp + 0x34]		// 0x0004b022
		fmul st(0), st(1)		// 0x0004b026
		fstp dword ptr [esp + 0x34]		// 0x0004b028
L4b02c:
		mov eax, dword ptr [esp + 0x6c]		// 0x0004b02c
		fstp st(0)		// 0x0004b030
		fld dword ptr [ebx + 0xe0]		// 0x0004b032
		mov edx, dword ptr [esp + 0x70]		// 0x0004b038
		fld dword ptr [esp + 0x2c]		// 0x0004b03c
		fmul st(0), st(1)		// 0x0004b040
		fstp dword ptr [esp + 0x44]		// 0x0004b042
		fld dword ptr [esp + 0x30]		// 0x0004b046
		fmul st(0), st(1)		// 0x0004b04a
		fstp dword ptr [esp + 0x48]		// 0x0004b04c
		fmul dword ptr [esp + 0x34]		// 0x0004b050
		fld dword ptr [eax]		// 0x0004b054
		fsub dword ptr [esp + 0x44]		// 0x0004b056
		fld dword ptr [eax + 4]		// 0x0004b05a
		fsub dword ptr [esp + 0x48]		// 0x0004b05d
		fld dword ptr [eax + 8]		// 0x0004b061
		fsub st(0), st(3)		// 0x0004b064
		fstp dword ptr [esp + 0x4c]		// 0x0004b066
		mov eax, dword ptr [esp + 0x4c]		// 0x0004b06a
		fxch st(1)		// 0x0004b06e
		mov dword ptr [esp + 0x58], eax		// 0x0004b070
		fstp dword ptr [esp + 0x50]		// 0x0004b074
		fstp dword ptr [esp + 0x54]		// 0x0004b078
		jmp L4b16b		// 0x0004b07c
L4b081:
		fld dword ptr [esp + 0x7c]		// 0x0004b081
		fsqrt		// 0x0004b085
		fsub dword ptr [edx + 0xe0]		// 0x0004b087
		fst dword ptr [esp + 0x28]		// 0x0004b08d
		fcomp dword ptr [nxSMZero]		// 0x0004b091
		fnstsw ax		// 0x0004b097
		test ah, 1		// 0x0004b099
		je L4b1e7		// 0x0004b09c
		fld dword ptr [esp + 0x58]		// 0x0004b0a2
		fmul dword ptr [esi + 8]		// 0x0004b0a6
		fld dword ptr [esp + 0x54]		// 0x0004b0a9
		fmul dword ptr [esi + 4]		// 0x0004b0ad
		faddp st(1), st(0)		// 0x0004b0b0
		fld dword ptr [esp + 0x50]		// 0x0004b0b2
		fmul dword ptr [esi]		// 0x0004b0b6
		faddp st(1), st(0)		// 0x0004b0b8
		fstp dword ptr [esp + 0x2c]		// 0x0004b0ba
		fld dword ptr [esp + 0x50]		// 0x0004b0be
		fmul dword ptr [esi + 0xc]		// 0x0004b0c2
		fld dword ptr [esp + 0x58]		// 0x0004b0c5
		fmul dword ptr [esi + 0x14]		// 0x0004b0c9
		faddp st(1), st(0)		// 0x0004b0cc
		fld dword ptr [esp + 0x54]		// 0x0004b0ce
		fmul dword ptr [esi + 0x10]		// 0x0004b0d2
		faddp st(1), st(0)		// 0x0004b0d5
		fstp dword ptr [esp + 0x30]		// 0x0004b0d7
		fld dword ptr [esp + 0x50]		// 0x0004b0db
		fmul dword ptr [esi + 0x18]		// 0x0004b0df
		fld dword ptr [esp + 0x58]		// 0x0004b0e2
		fmul dword ptr [esi + 0x20]		// 0x0004b0e6
		faddp st(1), st(0)		// 0x0004b0e9
		fld dword ptr [esp + 0x54]		// 0x0004b0eb
		fmul dword ptr [esi + 0x1c]		// 0x0004b0ef
		faddp st(1), st(0)		// 0x0004b0f2
		fstp dword ptr [esp + 0x34]		// 0x0004b0f4
		fld dword ptr [ecx]		// 0x0004b0f8
		fsub dword ptr [esp + 0x2c]		// 0x0004b0fa
		fstp dword ptr [esp + 0x50]		// 0x0004b0fe
		fld dword ptr [ecx + 4]		// 0x0004b102
		fsub dword ptr [esp + 0x30]		// 0x0004b105
		fstp dword ptr [esp + 0x54]		// 0x0004b109
		fld dword ptr [ecx + 8]		// 0x0004b10d
		fsub dword ptr [esp + 0x34]		// 0x0004b110
		fstp dword ptr [esp + 0x58]		// 0x0004b114
		fld dword ptr [esp + 0x2c]		// 0x0004b118
		fmul dword ptr [esp + 0x2c]		// 0x0004b11c
		fld dword ptr [esp + 0x34]		// 0x0004b120
		fmul dword ptr [esp + 0x34]		// 0x0004b124
		faddp st(1), st(0)		// 0x0004b128
		fld dword ptr [esp + 0x30]		// 0x0004b12a
		fmul dword ptr [esp + 0x30]		// 0x0004b12e
		faddp st(1), st(0)		// 0x0004b132
		fsqrt		// 0x0004b134
		fld dword ptr [nxSMZero]		// 0x0004b136
		fld st(1)		// 0x0004b13c
		fucompp		// 0x0004b13e
		fnstsw ax		// 0x0004b140
		test ah, 0x44		// 0x0004b142
		jnp L4b16b		// 0x0004b145
		fdivr dword ptr [nxSMOne]		// 0x0004b147
		fld dword ptr [esp + 0x2c]		// 0x0004b14d
		fmul st(0), st(1)		// 0x0004b151
		fstp dword ptr [esp + 0x2c]		// 0x0004b153
		fld dword ptr [esp + 0x30]		// 0x0004b157
		fmul st(0), st(1)		// 0x0004b15b
		fstp dword ptr [esp + 0x30]		// 0x0004b15d
		fld dword ptr [esp + 0x34]		// 0x0004b161
		fmul st(0), st(1)		// 0x0004b165
		fstp dword ptr [esp + 0x34]		// 0x0004b167
L4b16b:
		mov ecx, dword ptr [esp + 0x78]		// 0x0004b16b
		fstp st(0)		// 0x0004b16f
		mov eax, dword ptr [ecx + 0x18]		// 0x0004b171
		test eax, eax		// 0x0004b174
		je L4b185		// 0x0004b176
		mov esi, dword ptr [esp + 0x84]		// 0x0004b178
		mov ax, word ptr [eax + esi*2]		// 0x0004b17f
		jmp L4b18a		// 0x0004b183
L4b185:
		mov eax, 0xffff		// 0x0004b185
L4b18a:
		mov ecx, dword ptr [ecx + 0x1c]		// 0x0004b18a
		test ecx, ecx		// 0x0004b18d
		je L4b1a2		// 0x0004b18f
		mov esi, dword ptr [esp + 0x84]		// 0x0004b191
		mov ecx, dword ptr [ecx + esi*4]		// 0x0004b198
		mov dword ptr [esp + 0x84], ecx		// 0x0004b19b
L4b1a2:
		mov ecx, dword ptr [esp + 0x84]		// 0x0004b1a2
		push ecx		// 0x0004b1a9
		push -1		// 0x0004b1aa
		push eax		// 0x0004b1ac
		xor eax, eax		// 0x0004b1ad
		mov ax, word ptr [edx + 0xda]		// 0x0004b1af
		mov edx, dword ptr [edx + 0x9c]		// 0x0004b1b6
		lea ecx, [esp + 0x38]		// 0x0004b1bc
		push eax		// 0x0004b1c0
		push ecx		// 0x0004b1c1
		mov ecx, dword ptr [esp + 0x3c]		// 0x0004b1c2
		lea eax, [esp + 0x64]		// 0x0004b1c6
		push eax		// 0x0004b1ca
		mov eax, dword ptr [esp + 0x8c]		// 0x0004b1cb
		push ecx		// 0x0004b1d2
		mov ecx, dword ptr [eax + 0x9c]		// 0x0004b1d3
		push ecx		// 0x0004b1d9
		mov ecx, dword ptr [esp + 0xa0]		// 0x0004b1da
		push edx		// 0x0004b1e1
		call nxSMCallEmit		// 0x0004b1e2
L4b1e7:
		pop edi		// 0x0004b1e7
		pop esi		// 0x0004b1e8
		pop ebp		// 0x0004b1e9
		pop ebx		// 0x0004b1ea
		add esp, 0x58		// 0x0004b1eb
		ret		// 0x0004b1ee
		}
	}

// phys_fn_001929 (0x0004b1f0, 1542 B), 440 instructions
__declspec(naked) void __cdecl NxContactSphereMesh(const NxCollisionShape*, const NxCollisionShape*,
	NxContactSink*, void*)
	{
	__asm
		{
		sub esp, 0x100		// 0x0004b1f0
		push ebx		// 0x0004b1f6
		push ebp		// 0x0004b1f7
		mov ebp, dword ptr [esp + 0x110]		// 0x0004b1f8
		push esi		// 0x0004b1ff
		push edi		// 0x0004b200
		mov ecx, ebp		// 0x0004b201
		call nxSMCallShapeOwner		// 0x0004b203
		mov ecx, dword ptr [eax + 8]		// 0x0004b208
		test ecx, ecx		// 0x0004b20b
		jne L4b228		// 0x0004b20d
		mov eax, dword ptr [esp + 0x11c]		// 0x0004b20f
		mov ecx, dword ptr [esp + 0x114]		// 0x0004b216
		push eax		// 0x0004b21d
		push ebp		// 0x0004b21e
		push ecx		// 0x0004b21f
		call nxSMCallContinuousCd		// 0x0004b220
		add esp, 0xc		// 0x0004b225
L4b228:
		mov ecx, dword ptr [esp + 0x120]		// 0x0004b228
		mov eax, dword ptr [ecx + 0xb4]		// 0x0004b22f
		mov edi, dword ptr [esp + 0x114]		// 0x0004b235
		fld dword ptr [edi + 0x30]		// 0x0004b23c
		mov edx, dword ptr [edi + 0x38]		// 0x0004b23f
		lea ebx, [ecx + 0xb0]		// 0x0004b242
		fst dword ptr [esp + 0x7c]		// 0x0004b248
		fld dword ptr [edi + 0x34]		// 0x0004b24c
		add edi, 0x30		// 0x0004b24f
		and eax, 0xfffffffe		// 0x0004b252
		fst dword ptr [esp + 0x80]		// 0x0004b255
		mov esi, eax		// 0x0004b25c
		mov dword ptr [ebx + 4], eax		// 0x0004b25e
		and esi, 0xfffffffd		// 0x0004b261
		mov dword ptr [ebx + 4], esi		// 0x0004b264
		mov eax, dword ptr [ebp + 0xe0]		// 0x0004b267
		mov dword ptr [esp + 0x84], edx		// 0x0004b26d
		cmp dword ptr [eax + 0x7c], 0xff		// 0x0004b274
		mov eax, esi		// 0x0004b27b
		je L4b284		// 0x0004b27d
		or eax, 0x10		// 0x0004b27f
		jmp L4b287		// 0x0004b282
L4b284:
		and eax, 0xffffffef		// 0x0004b284
L4b287:
		mov dword ptr [ebx + 4], eax		// 0x0004b287
		fxch st(1)		// 0x0004b28a
		mov edx, dword ptr [ebp + 0xc]		// 0x0004b28c
		mov eax, dword ptr [ebp + 0x10]		// 0x0004b28f
		lea esi, [ebp + 0xc]		// 0x0004b292
		mov dword ptr [esp + 0xac], edx		// 0x0004b295
		mov edx, dword ptr [esi + 8]		// 0x0004b29c
		mov dword ptr [esp + 0xcc], edx		// 0x0004b29f
		mov edx, dword ptr [esi + 0x10]		// 0x0004b2a6
		mov dword ptr [esp + 0xbc], eax		// 0x0004b2a9
		mov eax, dword ptr [esi + 0xc]		// 0x0004b2b0
		mov dword ptr [esp + 0xc0], edx		// 0x0004b2b3
		mov edx, dword ptr [esi + 0x18]		// 0x0004b2ba
		mov dword ptr [esp + 0xb0], eax		// 0x0004b2bd
		mov eax, dword ptr [esi + 0x14]		// 0x0004b2c4
		mov dword ptr [esp + 0xb4], edx		// 0x0004b2c7
		mov edx, dword ptr [esi + 0x20]		// 0x0004b2ce
		mov dword ptr [esp + 0xd0], eax		// 0x0004b2d1
		mov eax, dword ptr [esi + 0x1c]		// 0x0004b2d8
		mov dword ptr [esp + 0xd4], edx		// 0x0004b2db
		mov edx, dword ptr [esi + 0x28]		// 0x0004b2e2
		mov dword ptr [esp + 0xc4], eax		// 0x0004b2e5
		mov eax, dword ptr [esi + 0x24]		// 0x0004b2ec
		mov dword ptr [esp + 0xe0], edx		// 0x0004b2ef
		mov edx, dword ptr [ebp + 0xe0]		// 0x0004b2f6
		mov dword ptr [esp + 0xdc], eax		// 0x0004b2fc
		mov eax, dword ptr [esi + 0x2c]		// 0x0004b303
		mov dword ptr [esp + 0xe4], eax		// 0x0004b306
		mov dword ptr [esp + 0xd8], 0		// 0x0004b30d
		mov dword ptr [esp + 0xc8], 0		// 0x0004b318
		mov dword ptr [esp + 0xb8], 0		// 0x0004b323
		mov dword ptr [esp + 0xe8], 0x3f800000		// 0x0004b32e
		mov eax, dword ptr [edx + 0x28]		// 0x0004b339
		fstp dword ptr [esp + 0x40]		// 0x0004b33c
		mov edx, dword ptr [esp + 0x84]		// 0x0004b340
		mov dword ptr [esp + 0x48], edx		// 0x0004b347
		fstp dword ptr [esp + 0x44]		// 0x0004b34b
		mov edx, dword ptr [esp + 0x114]		// 0x0004b34f
		mov edx, dword ptr [edx + 0xe0]		// 0x0004b356
		mov dword ptr [esp + 0x4c], edx		// 0x0004b35c
		lea edx, [esp + 0xac]		// 0x0004b360
		push edx		// 0x0004b367
		push 0		// 0x0004b368
		push eax		// 0x0004b36a
		lea eax, [esp + 0x4c]		// 0x0004b36b
		add ecx, 0xf4		// 0x0004b36f
		push eax		// 0x0004b375
		push ecx		// 0x0004b376
		mov ecx, ebx		// 0x0004b377
		call nxSMCallSphereCollide		// 0x0004b379
		test al, al		// 0x0004b37e
		je L4b7eb		// 0x0004b380
		mov ecx, dword ptr [esp + 0x120]		// 0x0004b386
		test byte ptr [ecx + 0xb4], 4		// 0x0004b38d
		je L4b7eb		// 0x0004b394
		mov eax, dword ptr [ebx + 0x10]		// 0x0004b39a
		test eax, eax		// 0x0004b39d
		je L4b3a6		// 0x0004b39f
		mov edx, dword ptr [eax + 4]		// 0x0004b3a1
		jmp L4b3a8		// 0x0004b3a4
L4b3a6:
		xor edx, edx		// 0x0004b3a6
L4b3a8:
		mov ebx, dword ptr [ebx + 0x10]		// 0x0004b3a8
		test ebx, ebx		// 0x0004b3ab
		mov dword ptr [esp + 0x24], edx		// 0x0004b3ad
		je L4b3b8		// 0x0004b3b1
		mov ebx, dword ptr [ebx + 8]		// 0x0004b3b3
		jmp L4b3ba		// 0x0004b3b6
L4b3b8:
		xor ebx, ebx		// 0x0004b3b8
L4b3ba:
		mov ebp, dword ptr [ebp + 0xe0]		// 0x0004b3ba
		mov eax, dword ptr [ebp + 0x7c]		// 0x0004b3c0
		cmp eax, 0xff		// 0x0004b3c3
		mov dword ptr [esp + 0x2c], ebx		// 0x0004b3c8
		mov dword ptr [esp + 0x10], ebp		// 0x0004b3cc
		je L4b77f		// 0x0004b3d0
		lea ecx, [eax*8]		// 0x0004b3d6
		mov eax, 0x1000201		// 0x0004b3dd
		shr eax, cl		// 0x0004b3e2
		mov ecx, eax		// 0x0004b3e4
		movzx eax, ah		// 0x0004b3e6
		and ecx, 0xff		// 0x0004b3e9
		mov dword ptr [esp + 0x28], eax		// 0x0004b3ef
		xor eax, eax		// 0x0004b3f3
		test edx, edx		// 0x0004b3f5
		mov dword ptr [esp + 0x30], ecx		// 0x0004b3f7
		mov dword ptr [esp + 0x34], eax		// 0x0004b3fb
		jbe L4b781		// 0x0004b3ff
		mov ecx, dword ptr [ebp + 0x14]		// 0x0004b405
		mov ebp, dword ptr [ebp + 0x10]		// 0x0004b408
		mov dword ptr [esp + 0x50], ecx		// 0x0004b40b
		nop		// 0x0004b40f
L4b410:
		mov ebx, dword ptr [ebx + eax*4]		// 0x0004b410
		mov eax, dword ptr [esp + 0x50]		// 0x0004b413
		lea edx, [ebx + ebx*2]		// 0x0004b417
		lea edx, [eax + edx*4]		// 0x0004b41a
		mov eax, dword ptr [edx]		// 0x0004b41d
		lea ecx, [eax + eax*2]		// 0x0004b41f
		fld dword ptr [ebp + ecx*4 + 4]		// 0x0004b422
		lea eax, [ebp + ecx*4]		// 0x0004b426
		fmul dword ptr [esi + 4]		// 0x0004b42a
		mov ecx, dword ptr [edx + 4]		// 0x0004b42d
		fld dword ptr [esi]		// 0x0004b430
		lea ecx, [ecx + ecx*2]		// 0x0004b432
		fmul dword ptr [eax]		// 0x0004b435
		lea ecx, [ebp + ecx*4]		// 0x0004b437
		mov edx, dword ptr [edx + 8]		// 0x0004b43b
		lea edx, [edx + edx*2]		// 0x0004b43e
		faddp st(1), st(0)		// 0x0004b441
		lea edx, [ebp + edx*4]		// 0x0004b443
		fld dword ptr [esi + 8]		// 0x0004b447
		fmul dword ptr [eax + 8]		// 0x0004b44a
		faddp st(1), st(0)		// 0x0004b44d
		fld dword ptr [esi + 0x14]		// 0x0004b44f
		fmul dword ptr [eax + 8]		// 0x0004b452
		fld dword ptr [eax + 4]		// 0x0004b455
		fmul dword ptr [esi + 0x10]		// 0x0004b458
		faddp st(1), st(0)		// 0x0004b45b
		fld dword ptr [esi + 0xc]		// 0x0004b45d
		fmul dword ptr [eax]		// 0x0004b460
		faddp st(1), st(0)		// 0x0004b462
		fstp dword ptr [esp + 0xa4]		// 0x0004b464
		fld dword ptr [esi + 0x20]		// 0x0004b46b
		fmul dword ptr [eax + 8]		// 0x0004b46e
		fld dword ptr [eax]		// 0x0004b471
		fmul dword ptr [esi + 0x18]		// 0x0004b473
		faddp st(1), st(0)		// 0x0004b476
		fld dword ptr [eax + 4]		// 0x0004b478
		fmul dword ptr [esi + 0x1c]		// 0x0004b47b
		faddp st(1), st(0)		// 0x0004b47e
		fstp dword ptr [esp + 0xa8]		// 0x0004b480
		fadd dword ptr [esi + 0x24]		// 0x0004b487
		fld dword ptr [esp + 0xa4]		// 0x0004b48a
		fadd dword ptr [esi + 0x28]		// 0x0004b491
		fld dword ptr [esp + 0xa8]		// 0x0004b494
		fadd dword ptr [esi + 0x2c]		// 0x0004b49b
		fstp dword ptr [esp + 0xf4]		// 0x0004b49e
		mov eax, dword ptr [esp + 0xf4]		// 0x0004b4a5
		fxch st(1)		// 0x0004b4ac
		mov dword ptr [esp + 0x60], eax		// 0x0004b4ae
		fstp dword ptr [esp + 0x58]		// 0x0004b4b2
		fstp dword ptr [esp + 0x5c]		// 0x0004b4b6
		fld dword ptr [ecx]		// 0x0004b4ba
		fmul dword ptr [esi]		// 0x0004b4bc
		fld dword ptr [esi + 8]		// 0x0004b4be
		fmul dword ptr [ecx + 8]		// 0x0004b4c1
		faddp st(1), st(0)		// 0x0004b4c4
		fld dword ptr [esi + 4]		// 0x0004b4c6
		fmul dword ptr [ecx + 4]		// 0x0004b4c9
		faddp st(1), st(0)		// 0x0004b4cc
		fld dword ptr [ecx + 8]		// 0x0004b4ce
		fmul dword ptr [esi + 0x14]		// 0x0004b4d1
		fld dword ptr [ecx + 4]		// 0x0004b4d4
		fmul dword ptr [esi + 0x10]		// 0x0004b4d7
		faddp st(1), st(0)		// 0x0004b4da
		fld dword ptr [esi + 0xc]		// 0x0004b4dc
		fmul dword ptr [ecx]		// 0x0004b4df
		faddp st(1), st(0)		// 0x0004b4e1
		fstp dword ptr [esp + 0x98]		// 0x0004b4e3
		fld dword ptr [ecx]		// 0x0004b4ea
		fmul dword ptr [esi + 0x18]		// 0x0004b4ec
		fld dword ptr [esi + 0x20]		// 0x0004b4ef
		fmul dword ptr [ecx + 8]		// 0x0004b4f2
		faddp st(1), st(0)		// 0x0004b4f5
		fld dword ptr [esi + 0x1c]		// 0x0004b4f7
		fmul dword ptr [ecx + 4]		// 0x0004b4fa
		faddp st(1), st(0)		// 0x0004b4fd
		fstp dword ptr [esp + 0x9c]		// 0x0004b4ff
		fadd dword ptr [esi + 0x24]		// 0x0004b506
		fld dword ptr [esp + 0x98]		// 0x0004b509
		fadd dword ptr [esi + 0x28]		// 0x0004b510
		fld dword ptr [esp + 0x9c]		// 0x0004b513
		fadd dword ptr [esi + 0x2c]		// 0x0004b51a
		fstp dword ptr [esp + 0x10c]		// 0x0004b51d
		mov ecx, dword ptr [esp + 0x10c]		// 0x0004b524
		fxch st(1)		// 0x0004b52b
		mov dword ptr [esp + 0x6c], ecx		// 0x0004b52d
		fstp dword ptr [esp + 0x64]		// 0x0004b531
		lea eax, [esp + 0x70]		// 0x0004b535
		push eax		// 0x0004b539
		fstp dword ptr [esp + 0x6c]		// 0x0004b53a
		lea ecx, [esp + 0x68]		// 0x0004b53e
		fld dword ptr [esi + 8]		// 0x0004b542
		push ecx		// 0x0004b545
		fmul dword ptr [edx + 8]		// 0x0004b546
		lea ecx, [esp + 0x48]		// 0x0004b549
		fld dword ptr [esi + 4]		// 0x0004b54d
		fmul dword ptr [edx + 4]		// 0x0004b550
		faddp st(1), st(0)		// 0x0004b553
		fld dword ptr [edx]		// 0x0004b555
		fmul dword ptr [esi]		// 0x0004b557
		faddp st(1), st(0)		// 0x0004b559
		fld dword ptr [esi + 0xc]		// 0x0004b55b
		fmul dword ptr [edx]		// 0x0004b55e
		fld dword ptr [esi + 0x14]		// 0x0004b560
		fmul dword ptr [edx + 8]		// 0x0004b563
		faddp st(1), st(0)		// 0x0004b566
		fld dword ptr [esi + 0x10]		// 0x0004b568
		fmul dword ptr [edx + 4]		// 0x0004b56b
		faddp st(1), st(0)		// 0x0004b56e
		fstp dword ptr [esp + 0x94]		// 0x0004b570
		fld dword ptr [esi + 0x20]		// 0x0004b577
		fmul dword ptr [edx + 8]		// 0x0004b57a
		fld dword ptr [esi + 0x1c]		// 0x0004b57d
		fmul dword ptr [edx + 4]		// 0x0004b580
		faddp st(1), st(0)		// 0x0004b583
		fld dword ptr [edx]		// 0x0004b585
		fmul dword ptr [esi + 0x18]		// 0x0004b587
		faddp st(1), st(0)		// 0x0004b58a
		fstp dword ptr [esp + 0x98]		// 0x0004b58c
		fadd dword ptr [esi + 0x24]		// 0x0004b593
		fld dword ptr [esp + 0x94]		// 0x0004b596
		fadd dword ptr [esi + 0x28]		// 0x0004b59d
		fld dword ptr [esp + 0x98]		// 0x0004b5a0
		fadd dword ptr [esi + 0x2c]		// 0x0004b5a7
		fstp dword ptr [esp + 0x108]		// 0x0004b5aa
		mov edx, dword ptr [esp + 0x108]		// 0x0004b5b1
		fxch st(1)		// 0x0004b5b8
		mov dword ptr [esp + 0x80], edx		// 0x0004b5ba
		fstp dword ptr [esp + 0x78]		// 0x0004b5c1
		lea edx, [esp + 0x60]		// 0x0004b5c5
		push edx		// 0x0004b5c9
		fstp dword ptr [esp + 0x80]		// 0x0004b5ca
		call nxSMCallTrianglePlane		// 0x0004b5d1
		fld dword ptr [esp + 0x44]		// 0x0004b5d6
		fmul dword ptr [edi + 4]		// 0x0004b5da
		fld dword ptr [esp + 0x48]		// 0x0004b5dd
		fmul dword ptr [edi + 8]		// 0x0004b5e1
		faddp st(1), st(0)		// 0x0004b5e4
		fld dword ptr [esp + 0x40]		// 0x0004b5e6
		fmul dword ptr [edi]		// 0x0004b5ea
		faddp st(1), st(0)		// 0x0004b5ec
		fadd dword ptr [esp + 0x4c]		// 0x0004b5ee
		fst dword ptr [esp + 0x54]		// 0x0004b5f2
		fcomp dword ptr [nxSMZero]		// 0x0004b5f6
		fnstsw ax		// 0x0004b5fc
		test ah, 5		// 0x0004b5fe
		jp L4b6f7		// 0x0004b601
		mov ecx, dword ptr [esp + 0x28]		// 0x0004b607
		fld dword ptr [esp + ecx*4 + 0x70]		// 0x0004b60b
		mov edx, dword ptr [esp + 0x30]		// 0x0004b60f
		fsub dword ptr [esp + ecx*4 + 0x58]		// 0x0004b613
		lea eax, [esp + ecx*4 + 0x58]		// 0x0004b617
		fld dword ptr [esp + edx*4 + 0x70]		// 0x0004b61b
		lea ecx, [esp + edx*4 + 0x58]		// 0x0004b61f
		fsub dword ptr [ecx]		// 0x0004b623
		mov edx, dword ptr [esp + 0x28]		// 0x0004b625
		fld dword ptr [esp + edx*4 + 0x64]		// 0x0004b629
		mov edx, dword ptr [esp + 0x30]		// 0x0004b62d
		fsub dword ptr [eax]		// 0x0004b631
		fld dword ptr [esp + edx*4 + 0x64]		// 0x0004b633
		fsub dword ptr [ecx]		// 0x0004b637
		fld st(0)		// 0x0004b639
		fmul st(0), st(1)		// 0x0004b63b
		fld st(2)		// 0x0004b63d
		fmul st(0), st(3)		// 0x0004b63f
		faddp st(1), st(0)		// 0x0004b641
		fstp dword ptr [esp + 0x20]		// 0x0004b643
		fld st(1)		// 0x0004b647
		fmul st(0), st(4)		// 0x0004b649
		fld st(1)		// 0x0004b64b
		fmul st(0), st(4)		// 0x0004b64d
		faddp st(1), st(0)		// 0x0004b64f
		fstp dword ptr [esp + 0x1c]		// 0x0004b651
		fld st(3)		// 0x0004b655
		fmul st(0), st(4)		// 0x0004b657
		fld st(3)		// 0x0004b659
		fmul st(0), st(4)		// 0x0004b65b
		faddp st(1), st(0)		// 0x0004b65d
		fstp dword ptr [esp + 0x3c]		// 0x0004b65f
		fld dword ptr [edi + edx*4]		// 0x0004b663
		fsub dword ptr [ecx]		// 0x0004b666
		mov ecx, dword ptr [esp + 0x28]		// 0x0004b668
		fstp dword ptr [esp + 0x14]		// 0x0004b66c
		fld dword ptr [edi + ecx*4]		// 0x0004b670
		fsub dword ptr [eax]		// 0x0004b673
		fstp dword ptr [esp + 0x38]		// 0x0004b675
		fld dword ptr [esp + 0x14]		// 0x0004b679
		fmul st(0), st(1)		// 0x0004b67d
		fld dword ptr [esp + 0x38]		// 0x0004b67f
		fmul st(0), st(3)		// 0x0004b683
		faddp st(1), st(0)		// 0x0004b685
		fstp dword ptr [esp + 0x18]		// 0x0004b687
		fstp st(0)		// 0x0004b68b
		fstp st(0)		// 0x0004b68d
		fld dword ptr [esp + 0x38]		// 0x0004b68f
		fmulp st(2), st(0)		// 0x0004b693
		fld dword ptr [esp + 0x14]		// 0x0004b695
		fmul st(0), st(1)		// 0x0004b699
		faddp st(2), st(0)		// 0x0004b69b
		fstp st(0)		// 0x0004b69d
		fld dword ptr [esp + 0x18]		// 0x0004b69f
		fmul dword ptr [esp + 0x3c]		// 0x0004b6a3
		fld st(1)		// 0x0004b6a7
		fmul dword ptr [esp + 0x1c]		// 0x0004b6a9
		fsubp st(1), st(0)		// 0x0004b6ad
		fstp dword ptr [esp + 0x14]		// 0x0004b6af
		mov ecx, dword ptr [esp + 0x14]		// 0x0004b6b3
		fmul dword ptr [esp + 0x20]		// 0x0004b6b7
		fld dword ptr [esp + 0x18]		// 0x0004b6bb
		fmul dword ptr [esp + 0x1c]		// 0x0004b6bf
		fsubp st(1), st(0)		// 0x0004b6c3
		fst dword ptr [esp + 0x18]		// 0x0004b6c5
		fadd dword ptr [esp + 0x14]		// 0x0004b6c9
		mov edx, dword ptr [esp + 0x18]		// 0x0004b6cd
		fld dword ptr [esp + 0x3c]		// 0x0004b6d1
		fmul dword ptr [esp + 0x20]		// 0x0004b6d5
		fld dword ptr [esp + 0x1c]		// 0x0004b6d9
		fmul dword ptr [esp + 0x1c]		// 0x0004b6dd
		fsubp st(1), st(0)		// 0x0004b6e1
		fsubp st(1), st(0)		// 0x0004b6e3
		fstp dword ptr [esp + 0x20]		// 0x0004b6e5
		or edx, ecx		// 0x0004b6e9
		mov ecx, dword ptr [esp + 0x20]		// 0x0004b6eb
		not edx		// 0x0004b6ef
		and edx, ecx		// 0x0004b6f1
		test edx, edx		// 0x0004b6f3
		js L4b716		// 0x0004b6f5
L4b6f7:
		mov eax, dword ptr [esp + 0x34]		// 0x0004b6f7
		mov edx, dword ptr [esp + 0x24]		// 0x0004b6fb
		mov ebx, dword ptr [esp + 0x2c]		// 0x0004b6ff
		inc eax		// 0x0004b703
		cmp eax, edx		// 0x0004b704
		mov dword ptr [esp + 0x34], eax		// 0x0004b706
		jb L4b410		// 0x0004b70a
		mov ebp, dword ptr [esp + 0x10]		// 0x0004b710
		jmp L4b77f		// 0x0004b714
L4b716:
		mov ebp, dword ptr [esp + 0x10]		// 0x0004b716
		mov eax, dword ptr [ebp + 0x18]		// 0x0004b71a
		test eax, eax		// 0x0004b71d
		je L4b727		// 0x0004b71f
		mov ax, word ptr [eax + ebx*2]		// 0x0004b721
		jmp L4b72c		// 0x0004b725
L4b727:
		mov eax, 0xffff		// 0x0004b727
L4b72c:
		mov ecx, dword ptr [ebp + 0x1c]		// 0x0004b72c
		test ecx, ecx		// 0x0004b72f
		je L4b736		// 0x0004b731
		mov ebx, dword ptr [ecx + ebx*4]		// 0x0004b733
L4b736:
		push ebx		// 0x0004b736
		push -1		// 0x0004b737
		push eax		// 0x0004b739
		mov eax, dword ptr [esp + 0x120]		// 0x0004b73a
		xor ecx, ecx		// 0x0004b741
		mov cx, word ptr [eax + 0xda]		// 0x0004b743
		lea edx, [esp + 0x4c]		// 0x0004b74a
		push ecx		// 0x0004b74e
		mov ecx, dword ptr [esp + 0x64]		// 0x0004b74f
		push edx		// 0x0004b753
		mov edx, dword ptr [esp + 0x12c]		// 0x0004b754
		push edi		// 0x0004b75b
		push ecx		// 0x0004b75c
		mov ecx, dword ptr [edx + 0x9c]		// 0x0004b75d
		mov edx, dword ptr [eax + 0x9c]		// 0x0004b763
		push ecx		// 0x0004b769
		mov ecx, dword ptr [esp + 0x13c]		// 0x0004b76a
		push edx		// 0x0004b771
		call nxSMCallEmit		// 0x0004b772
		mov edx, dword ptr [esp + 0x24]		// 0x0004b777
		mov ebx, dword ptr [esp + 0x2c]		// 0x0004b77b
L4b77f:
		test edx, edx		// 0x0004b77f
L4b781:
		je L4b7eb		// 0x0004b781
		mov dword ptr [esp + 0x10], edx		// 0x0004b783
L4b787:
		mov eax, dword ptr [ebx]		// 0x0004b787
		mov edx, dword ptr [ebp + 0x14]		// 0x0004b789
		mov edi, dword ptr [ebp + 0x10]		// 0x0004b78c
		push eax		// 0x0004b78f
		lea ecx, [eax + eax*2]		// 0x0004b790
		mov eax, dword ptr [esp + 0x120]		// 0x0004b793
		push eax		// 0x0004b79a
		mov eax, dword ptr [edx + ecx*4 + 8]		// 0x0004b79b
		lea edx, [edx + ecx*4]		// 0x0004b79f
		lea ecx, [eax + eax*2]		// 0x0004b7a2
		lea eax, [edi + ecx*4]		// 0x0004b7a5
		push eax		// 0x0004b7a8
		mov eax, dword ptr [edx + 4]		// 0x0004b7a9
		mov edx, dword ptr [edx]		// 0x0004b7ac
		lea ecx, [eax + eax*2]		// 0x0004b7ae
		mov eax, dword ptr [esp + 0x124]		// 0x0004b7b1
		push ebp		// 0x0004b7b8
		push eax		// 0x0004b7b9
		mov eax, dword ptr [esp + 0x128]		// 0x0004b7ba
		push eax		// 0x0004b7c1
		lea eax, [esp + 0x94]		// 0x0004b7c2
		lea edx, [edx + edx*2]		// 0x0004b7c9
		push eax		// 0x0004b7cc
		lea ecx, [edi + ecx*4]		// 0x0004b7cd
		lea edx, [edi + edx*4]		// 0x0004b7d0
		mov eax, esi		// 0x0004b7d3
		add ebx, 4		// 0x0004b7d5
		call nxSphereMeshTriangle		// 0x0004b7d8
		mov eax, dword ptr [esp + 0x2c]		// 0x0004b7dd
		add esp, 0x1c		// 0x0004b7e1
		dec eax		// 0x0004b7e4
		mov dword ptr [esp + 0x10], eax		// 0x0004b7e5
		jne L4b787		// 0x0004b7e9
L4b7eb:
		pop edi		// 0x0004b7eb
		pop esi		// 0x0004b7ec
		pop ebp		// 0x0004b7ed
		pop ebx		// 0x0004b7ee
		add esp, 0x100		// 0x0004b7ef
		ret		// 0x0004b7f5
		}
	}
