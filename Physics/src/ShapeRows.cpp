/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// ShapeRows.cpp: the internal shape's own rows that the public shape handles and the scene reach (Shape.cpp in the
// oracle's source tree): phys_fn_001303 to 001345 less those reconstructed in ObjectModel.cpp. Naked listing
// transcriptions; the SDK parameter array (.data 0x10123b18, gParameter), the API reentry flag and its message pointer
// (.data 0x10123c10, 0x10122050) are the candidate's own variables by name. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase3-shape-rows-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxShpCallNxBaseDtor();
extern "C" void nxShpCallRow000738();
extern "C" void nxShpCallUpdateWorldAABB();
extern "C" void* nxShpImpImperrorFoundationSDKNxFoundationSANW4Nx;
extern "C" void* nxShpImpImpinstanceFoundationSDKNxFoundation0PAV;
extern "C" void* nxShpImpImpnxFoundationSDKAllocator3PAVNxUserAll;
#pragma comment(linker, "/alternatename:_nxShpCallNxBaseDtor=?nxBaseDtor@ShapeBase@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxShpCallRow000738=?row000738@Row000738Fixture@@QAEPAXXZ")
#pragma comment(linker, "/alternatename:_nxShpCallUpdateWorldAABB=?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z")
#pragma comment(linker, "/alternatename:_nxShpImpImperrorFoundationSDKNxFoundationSANW4Nx=__imp_?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ")
#pragma comment(linker, "/alternatename:_nxShpImpImpinstanceFoundationSDKNxFoundation0PAV=__imp_?instance@FoundationSDK@NxFoundation@@0PAV12@A")
#pragma comment(linker, "/alternatename:_nxShpImpImpnxFoundationSDKAllocator3PAVNxUserAll=__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A")
extern NxReal gParameter[];
extern "C" unsigned gNxApiReentry;
extern "C" const char* gNxApiReentryMessage;

unsigned __fastcall nxShp1303(void* self, void* edxUnused);
unsigned __fastcall nxShp1305(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1307(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1309(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1311(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1313(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1317(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1319(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1321(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1325(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1331(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxShp1333(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1335(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1337(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1339(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1341(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxShp1345(void* self, void* edxUnused, unsigned a0);
// @@PROTOS-END@@

static const unsigned nxShpK101041ec_4 = 0x3f800000;
static const unsigned nxShpK101041f0_4 = 0x0;
static const char nxShpStr1074c4[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\Shape.cpp";

// phys_fn_001303 (0x000258f0, 98 B), 36 instructions
__declspec(naked) unsigned __fastcall nxShp1303(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x000258f0
		mov esi, ecx		// 0x000258f1
		mov eax, dword ptr [esi + 4]		// 0x000258f3
		mov ecx, dword ptr [eax + 8]		// 0x000258f6
		test ecx, ecx		// 0x000258f9
		je L25906		// 0x000258fb
		call nxShpCallRow000738		// 0x000258fd
		test eax, eax		// 0x00025902
		jne L25950		// 0x00025904
L25906:
		mov ax, word ptr [esi + 0xcc]		// 0x00025906
		push edi		// 0x0002590d
		lea edi, [esi + 0xa4]		// 0x0002590e
		mov esi, dword ptr [esi + 0xc4]		// 0x00025914
		add esi, 4		// 0x0002591a
		cmp ax, 0xffff		// 0x0002591d
		jne L25928		// 0x00025921
		pop edi		// 0x00025923
		xor eax, eax		// 0x00025924
		pop esi		// 0x00025926
		ret		// 0x00025927
L25928:
		test byte ptr [edi + 8], 2		// 0x00025928
		jne L25942		// 0x0002592c
		mov edx, dword ptr [esi + 0x10]		// 0x0002592e
		movzx eax, ax		// 0x00025931
		lea ecx, [eax + eax*2]		// 0x00025934
		lea eax, [edx + ecx*8]		// 0x00025937
		push eax		// 0x0002593a
		mov ecx, edi		// 0x0002593b
		call nxShpCallUpdateWorldAABB		// 0x0002593d
L25942:
		movzx eax, word ptr [edi + 0x28]		// 0x00025942
		mov edx, dword ptr [esi + 0x10]		// 0x00025946
		lea ecx, [eax + eax*2]		// 0x00025949
		lea eax, [edx + ecx*8]		// 0x0002594c
		pop edi		// 0x0002594f
L25950:
		pop esi		// 0x00025950
		ret		// 0x00025951
		}
	}

// phys_fn_001305 (0x00025960, 685 B), 208 instructions
__declspec(naked) unsigned __fastcall nxShp1305(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		fld dword ptr [nxShpK101041f0_4]		// 0x00025960
		sub esp, 0x64		// 0x00025966
		fld dword ptr [gParameter + 0xb0]		// 0x00025969
		push ebx		// 0x0002596f
		mov ebx, dword ptr [esp + 0x6c]		// 0x00025970
		fucompp		// 0x00025974
		push esi		// 0x00025976
		push edi		// 0x00025977
		mov esi, ecx		// 0x00025978
		fnstsw ax		// 0x0002597a
		test ah, 0x44		// 0x0002597c
		jnp L25a93		// 0x0002597f
		fld dword ptr [gParameter + 0x34]		// 0x00025985
		mov eax, dword ptr [esi + 0x24]		// 0x0002598b
		fmul dword ptr [gParameter + 0xb0]		// 0x0002598e
		mov ecx, dword ptr [esi + 0x28]		// 0x00025994
		fld dword ptr [esi + 0xc]		// 0x00025997
		mov dword ptr [esp + 0x14], eax		// 0x0002599a
		fld dword ptr [esi + 0x18]		// 0x0002599e
		mov edx, dword ptr [esi + 0x2c]		// 0x000259a1
		fxch st(1)		// 0x000259a4
		mov dword ptr [esp + 0x2c], ecx		// 0x000259a6
		fmul st(0), st(2)		// 0x000259aa
		mov dword ptr [esp + 0x20], edx		// 0x000259ac
		mov eax, dword ptr [ebx]		// 0x000259b0
		lea edi, [esi + 0x30]		// 0x000259b2
		fstp dword ptr [esp + 0xc]		// 0x000259b5
		push 0xcf0000		// 0x000259b9
		lea ecx, [esp + 0x34]		// 0x000259be
		fmul st(0), st(1)		// 0x000259c2
		push ecx		// 0x000259c4
		fld dword ptr [esp + 0x1c]		// 0x000259c5
		push edi		// 0x000259c9
		fmul st(0), st(2)		// 0x000259ca
		mov ecx, ebx		// 0x000259cc
		fstp dword ptr [esp + 0x20]		// 0x000259ce
		fld dword ptr [esi + 0x10]		// 0x000259d2
		fld dword ptr [esi + 0x1c]		// 0x000259d5
		fxch st(1)		// 0x000259d8
		fmul st(0), st(3)		// 0x000259da
		fstp dword ptr [esp + 0x30]		// 0x000259dc
		fmul st(0), st(2)		// 0x000259e0
		fstp dword ptr [esp + 0x34]		// 0x000259e2
		fld dword ptr [esp + 0x38]		// 0x000259e6
		fmul st(0), st(2)		// 0x000259ea
		fstp dword ptr [esp + 0x38]		// 0x000259ec
		fld dword ptr [esi + 0x14]		// 0x000259f0
		fld dword ptr [esi + 0x20]		// 0x000259f3
		fxch st(1)		// 0x000259f6
		fmul st(0), st(3)		// 0x000259f8
		fstp dword ptr [esp + 0x24]		// 0x000259fa
		fmul st(0), st(2)		// 0x000259fe
		fstp dword ptr [esp + 0x28]		// 0x00025a00
		fld dword ptr [esp + 0x2c]		// 0x00025a04
		fmul st(0), st(2)		// 0x00025a08
		fstp dword ptr [esp + 0x2c]		// 0x00025a0a
		fld dword ptr [esp + 0x18]		// 0x00025a0e
		fadd dword ptr [edi]		// 0x00025a12
		fstp dword ptr [esp + 0x3c]		// 0x00025a14
		fadd dword ptr [edi + 4]		// 0x00025a18
		fstp dword ptr [esp + 0x40]		// 0x00025a1b
		fstp st(0)		// 0x00025a1f
		fld dword ptr [esp + 0x20]		// 0x00025a21
		fadd dword ptr [edi + 8]		// 0x00025a25
		fstp dword ptr [esp + 0x44]		// 0x00025a28
		call dword ptr [eax + 0x20]		// 0x00025a2c
		fld dword ptr [esp + 0x24]		// 0x00025a2f
		mov edx, dword ptr [ebx]		// 0x00025a33
		fadd dword ptr [edi]		// 0x00025a35
		push 0xcf00		// 0x00025a37
		lea eax, [esp + 0x34]		// 0x00025a3c
		push eax		// 0x00025a40
		fstp dword ptr [esp + 0x38]		// 0x00025a41
		push edi		// 0x00025a45
		fld dword ptr [esp + 0x34]		// 0x00025a46
		mov ecx, ebx		// 0x00025a4a
		fadd dword ptr [edi + 4]		// 0x00025a4c
		fstp dword ptr [esp + 0x40]		// 0x00025a4f
		fld dword ptr [esp + 0x38]		// 0x00025a53
		fadd dword ptr [edi + 8]		// 0x00025a57
		fstp dword ptr [esp + 0x44]		// 0x00025a5a
		call dword ptr [edx + 0x20]		// 0x00025a5e
		fld dword ptr [esp + 0x18]		// 0x00025a61
		mov edx, dword ptr [ebx]		// 0x00025a65
		fadd dword ptr [edi]		// 0x00025a67
		push 0xcf		// 0x00025a69
		fstp dword ptr [esp + 0x34]		// 0x00025a6e
		fld dword ptr [esp + 0x20]		// 0x00025a72
		fadd dword ptr [edi + 4]		// 0x00025a76
		fstp dword ptr [esp + 0x38]		// 0x00025a79
		fld dword ptr [esp + 0x24]		// 0x00025a7d
		fadd dword ptr [edi + 8]		// 0x00025a81
		fstp dword ptr [esp + 0x3c]		// 0x00025a84
		lea eax, [esp + 0x34]		// 0x00025a88
		push eax		// 0x00025a8c
		push edi		// 0x00025a8d
		mov ecx, ebx		// 0x00025a8e
		call dword ptr [edx + 0x20]		// 0x00025a90
L25a93:
		fld dword ptr [nxShpK101041f0_4]		// 0x00025a93
		fld dword ptr [gParameter + 0xc0]		// 0x00025a99
		fucompp		// 0x00025a9f
		fnstsw ax		// 0x00025aa1
		test ah, 0x44		// 0x00025aa3
		jnp L25c04		// 0x00025aa6
		mov edx, dword ptr [esi]		// 0x00025aac
		lea eax, [esp + 0x30]		// 0x00025aae
		push eax		// 0x00025ab2
		mov ecx, esi		// 0x00025ab3
		call dword ptr [edx + 0x28]		// 0x00025ab5
		mov eax, dword ptr [esp + 0x38]		// 0x00025ab8
		mov edx, dword ptr [esp + 0x34]		// 0x00025abc
		add esi, 0xc		// 0x00025ac0
		mov ecx, 9		// 0x00025ac3
		lea edi, [esp + 0x40]		// 0x00025ac8
		rep movsd		// 0x00025acc
		mov ecx, dword ptr [esp + 0x30]		// 0x00025ace
		mov dword ptr [esp + 0x6c], eax		// 0x00025ad2
		mov eax, dword ptr [esp + 0x58]		// 0x00025ad6
		mov dword ptr [esp + 0x64], ecx		// 0x00025ada
		mov ecx, dword ptr [esp + 0x40]		// 0x00025ade
		mov dword ptr [esp + 0x20], eax		// 0x00025ae2
		mov eax, dword ptr [esp + 0x5c]		// 0x00025ae6
		mov dword ptr [esp + 0x18], ecx		// 0x00025aea
		mov ecx, dword ptr [esp + 0x44]		// 0x00025aee
		mov dword ptr [esp + 0x68], edx		// 0x00025af2
		mov edx, dword ptr [esp + 0x4c]		// 0x00025af6
		mov dword ptr [esp + 0x2c], eax		// 0x00025afa
		mov eax, dword ptr [esp + 0x60]		// 0x00025afe
		push 0		// 0x00025b02
		mov dword ptr [esp + 0x28], ecx		// 0x00025b04
		mov ecx, dword ptr [esp + 0x4c]		// 0x00025b08
		mov dword ptr [esp + 0x20], edx		// 0x00025b0c
		mov edx, dword ptr [esp + 0x54]		// 0x00025b10
		mov dword ptr [esp + 0x18], eax		// 0x00025b14
		mov eax, dword ptr [esp + 0x40]		// 0x00025b18
		push eax		// 0x00025b1c
		mov dword ptr [esp + 0x14], ecx		// 0x00025b1d
		mov dword ptr [esp + 0x30], edx		// 0x00025b21
		mov edx, dword ptr [esp + 0x5c]		// 0x00025b25
		push 0xffff00ff		// 0x00025b29
		lea ecx, [esp + 0x4c]		// 0x00025b2e
		push ecx		// 0x00025b32
		mov dword ptr [esp + 0x20], edx		// 0x00025b33
		mov edx, dword ptr [ebx]		// 0x00025b37
		push 0x14		// 0x00025b39
		mov ecx, ebx		// 0x00025b3b
		call dword ptr [edx + 0x38]		// 0x00025b3d
		mov eax, dword ptr [esp + 0x28]		// 0x00025b40
		mov ecx, dword ptr [esp + 0x2c]		// 0x00025b44
		mov edx, dword ptr [esp + 0x24]		// 0x00025b48
		mov dword ptr [esp + 0x4c], eax		// 0x00025b4c
		mov eax, dword ptr [esp + 0x10]		// 0x00025b50
		mov dword ptr [esp + 0x58], ecx		// 0x00025b54
		mov ecx, dword ptr [esp + 0x14]		// 0x00025b58
		mov dword ptr [esp + 0x50], eax		// 0x00025b5c
		mov eax, dword ptr [esp + 0x1c]		// 0x00025b60
		push 0		// 0x00025b64
		mov dword ptr [esp + 0x60], ecx		// 0x00025b66
		mov ecx, dword ptr [esp + 0x24]		// 0x00025b6a
		mov dword ptr [esp + 0x44], edx		// 0x00025b6e
		mov edx, dword ptr [esp + 0x10]		// 0x00025b72
		mov dword ptr [esp + 0x58], eax		// 0x00025b76
		mov eax, dword ptr [esp + 0x40]		// 0x00025b7a
		push eax		// 0x00025b7e
		mov dword ptr [esp + 0x68], ecx		// 0x00025b7f
		mov dword ptr [esp + 0x4c], edx		// 0x00025b83
		mov edx, dword ptr [esp + 0x20]		// 0x00025b87
		push 0xffff00ff		// 0x00025b8b
		lea ecx, [esp + 0x4c]		// 0x00025b90
		push ecx		// 0x00025b94
		mov dword ptr [esp + 0x58], edx		// 0x00025b95
		mov edx, dword ptr [ebx]		// 0x00025b99
		push 0x14		// 0x00025b9b
		mov ecx, ebx		// 0x00025b9d
		call dword ptr [edx + 0x38]		// 0x00025b9f
		mov edx, dword ptr [esp + 0xc]		// 0x00025ba2
		mov eax, dword ptr [esp + 0x10]		// 0x00025ba6
		mov ecx, dword ptr [esp + 0x14]		// 0x00025baa
		mov dword ptr [esp + 0x40], edx		// 0x00025bae
		mov edx, dword ptr [esp + 0x18]		// 0x00025bb2
		mov dword ptr [esp + 0x4c], eax		// 0x00025bb6
		mov eax, dword ptr [esp + 0x1c]		// 0x00025bba
		mov dword ptr [esp + 0x58], ecx		// 0x00025bbe
		mov dword ptr [esp + 0x44], edx		// 0x00025bc2
		mov dword ptr [esp + 0x50], eax		// 0x00025bc6
		mov eax, dword ptr [esp + 0x28]		// 0x00025bca
		mov ecx, dword ptr [esp + 0x20]		// 0x00025bce
		mov edx, dword ptr [esp + 0x24]		// 0x00025bd2
		push 0		// 0x00025bd6
		mov dword ptr [esp + 0x60], ecx		// 0x00025bd8
		mov ecx, dword ptr [esp + 0x30]		// 0x00025bdc
		mov dword ptr [esp + 0x58], eax		// 0x00025be0
		mov eax, dword ptr [esp + 0x40]		// 0x00025be4
		push eax		// 0x00025be8
		mov dword ptr [esp + 0x68], ecx		// 0x00025be9
		push 0xffff00ff		// 0x00025bed
		lea ecx, [esp + 0x4c]		// 0x00025bf2
		push ecx		// 0x00025bf6
		mov dword ptr [esp + 0x58], edx		// 0x00025bf7
		mov edx, dword ptr [ebx]		// 0x00025bfb
		push 0x14		// 0x00025bfd
		mov ecx, ebx		// 0x00025bff
		call dword ptr [edx + 0x38]		// 0x00025c01
L25c04:
		pop edi		// 0x00025c04
		pop esi		// 0x00025c05
		pop ebx		// 0x00025c06
		add esp, 0x64		// 0x00025c07
		ret 4		// 0x00025c0a
		}
	}

// phys_fn_001307 (0x00025c10, 846 B), 255 instructions
__declspec(naked) unsigned __fastcall nxShp1307(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0x84		// 0x00025c10
		mov eax, dword ptr [gNxApiReentry]		// 0x00025c16
		test eax, eax		// 0x00025c1b
		mov edx, ecx		// 0x00025c1d
		mov dword ptr [esp + 0x50], 0		// 0x00025c1f
		je L25c5b		// 0x00025c27
		mov eax, dword ptr [nxShpImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00025c29
		cmp dword ptr [eax], 0		// 0x00025c2e
		jne L25c34		// 0x00025c31
		int 3		// 0x00025c33
L25c34:
		mov ecx, dword ptr [gNxApiReentryMessage]		// 0x00025c34
		push ecx		// 0x00025c3a
		push 0		// 0x00025c3b
		push 0x137		// 0x00025c3d
		push offset nxShpStr1074c4		// 0x00025c42
		push 2		// 0x00025c47
		call dword ptr [nxShpImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00025c49
		add esp, 0x14		// 0x00025c4f
		add esp, 0x84		// 0x00025c52
		ret 4		// 0x00025c58
L25c5b:
		mov dword ptr [gNxApiReentry], 1		// 0x00025c5b
		mov ecx, dword ptr [edx + 4]		// 0x00025c65
		mov eax, dword ptr [ecx + 8]		// 0x00025c68
		test eax, eax		// 0x00025c6b
		push esi		// 0x00025c6d
		push edi		// 0x00025c6e
		je L25d5c		// 0x00025c6f
		fld dword ptr [eax + 0x30]		// 0x00025c75
		mov ecx, 9		// 0x00025c78
		fld dword ptr [eax + 0x24]		// 0x00025c7d
		lea esi, [esp + 0x1c]		// 0x00025c80
		fld dword ptr [eax + 0x28]		// 0x00025c84
		lea edi, [esp + 0x5c]		// 0x00025c87
		fld dword ptr [eax + 0x2c]		// 0x00025c8b
		fld st(1)		// 0x00025c8e
		fmul st(0), st(2)		// 0x00025c90
		fadd st(0), st(0)		// 0x00025c92
		fstp dword ptr [esp + 8]		// 0x00025c94
		fld st(0)		// 0x00025c98
		fmul st(0), st(1)		// 0x00025c9a
		fadd st(0), st(0)		// 0x00025c9c
		fld dword ptr [nxShpK101041ec_4]		// 0x00025c9e
		fsub dword ptr [esp + 8]		// 0x00025ca4
		fsub st(0), st(1)		// 0x00025ca8
		fstp dword ptr [esp + 0x1c]		// 0x00025caa
		fld st(2)		// 0x00025cae
		fmul st(0), st(4)		// 0x00025cb0
		fadd st(0), st(0)		// 0x00025cb2
		fld st(2)		// 0x00025cb4
		fmul st(0), st(6)		// 0x00025cb6
		fadd st(0), st(0)		// 0x00025cb8
		fld st(1)		// 0x00025cba
		fsub st(0), st(1)		// 0x00025cbc
		fstp dword ptr [esp + 0x20]		// 0x00025cbe
		fld st(3)		// 0x00025cc2
		fmul st(0), st(6)		// 0x00025cc4
		fadd st(0), st(0)		// 0x00025cc6
		fstp dword ptr [esp + 0xc]		// 0x00025cc8
		fld st(4)		// 0x00025ccc
		fmul st(0), st(7)		// 0x00025cce
		fadd st(0), st(0)		// 0x00025cd0
		fst dword ptr [esp + 0x10]		// 0x00025cd2
		fadd dword ptr [esp + 0xc]		// 0x00025cd6
		fstp dword ptr [esp + 0x24]		// 0x00025cda
		fadd st(0), st(1)		// 0x00025cde
		fstp dword ptr [esp + 0x28]		// 0x00025ce0
		fstp st(0)		// 0x00025ce4
		fld st(3)		// 0x00025ce6
		fmul st(0), st(4)		// 0x00025ce8
		fadd st(0), st(0)		// 0x00025cea
		fsubr dword ptr [nxShpK101041ec_4]		// 0x00025cec
		fst dword ptr [esp + 0x18]		// 0x00025cf2
		fsub st(0), st(1)		// 0x00025cf6
		fstp dword ptr [esp + 0x2c]		// 0x00025cf8
		fstp st(0)		// 0x00025cfc
		fmul st(0), st(1)		// 0x00025cfe
		fadd st(0), st(0)		// 0x00025d00
		fstp dword ptr [esp + 0x14]		// 0x00025d02
		fstp st(0)		// 0x00025d06
		fmulp st(1), st(0)		// 0x00025d08
		fadd st(0), st(0)		// 0x00025d0a
		fld dword ptr [esp + 0x14]		// 0x00025d0c
		fsub st(0), st(1)		// 0x00025d10
		fstp dword ptr [esp + 0x30]		// 0x00025d12
		fld dword ptr [esp + 0xc]		// 0x00025d16
		fsub dword ptr [esp + 0x10]		// 0x00025d1a
		fstp dword ptr [esp + 0x34]		// 0x00025d1e
		fadd dword ptr [esp + 0x14]		// 0x00025d22
		fstp dword ptr [esp + 0x38]		// 0x00025d26
		fld dword ptr [esp + 0x18]		// 0x00025d2a
		fsub dword ptr [esp + 8]		// 0x00025d2e
		fstp dword ptr [esp + 0x3c]		// 0x00025d32
		rep movsd		// 0x00025d36
		mov ecx, dword ptr [eax + 0x18]		// 0x00025d38
		mov dword ptr [esp + 0x80], ecx		// 0x00025d3b
		mov ecx, dword ptr [eax + 0x1c]		// 0x00025d42
		mov eax, dword ptr [eax + 0x20]		// 0x00025d45
		mov dword ptr [esp + 0x88], eax		// 0x00025d48
		mov dword ptr [esp + 0x84], ecx		// 0x00025d4f
		lea eax, [esp + 0x5c]		// 0x00025d56
		jmp L25d5f		// 0x00025d5a
L25d5c:
		lea eax, [ecx + 0x20]		// 0x00025d5c
L25d5f:
		fld dword ptr [eax + 0x24]		// 0x00025d5f
		mov esi, eax		// 0x00025d62
		fld dword ptr [eax + 0x28]		// 0x00025d64
		mov ecx, 9		// 0x00025d67
		lea edi, [esp + 0x1c]		// 0x00025d6c
		rep movsd		// 0x00025d70
		mov ecx, dword ptr [eax + 0x2c]		// 0x00025d72
		mov eax, dword ptr [esp + 0x90]		// 0x00025d75
		mov dword ptr [esp + 0x48], ecx		// 0x00025d7c
		fld dword ptr [eax + 0x24]		// 0x00025d80
		fsub st(0), st(2)		// 0x00025d83
		fstp dword ptr [esp + 0x4c]		// 0x00025d85
		fld dword ptr [eax + 0x28]		// 0x00025d89
		fsub st(0), st(1)		// 0x00025d8c
		fstp st(2)		// 0x00025d8e
		fstp st(0)		// 0x00025d90
		fld dword ptr [eax + 0x2c]		// 0x00025d92
		fsub dword ptr [esp + 0x48]		// 0x00025d95
		fstp dword ptr [esp + 0x54]		// 0x00025d99
		fld dword ptr [esp + 0x34]		// 0x00025d9d
		fmul dword ptr [esp + 0x54]		// 0x00025da1
		fld dword ptr [esp + 0x28]		// 0x00025da5
		fmul st(0), st(2)		// 0x00025da9
		faddp st(1), st(0)		// 0x00025dab
		fld dword ptr [esp + 0x1c]		// 0x00025dad
		fmul dword ptr [esp + 0x4c]		// 0x00025db1
		faddp st(1), st(0)		// 0x00025db5
		fld dword ptr [esp + 0x38]		// 0x00025db7
		fmul dword ptr [esp + 0x54]		// 0x00025dbb
		fld dword ptr [esp + 0x2c]		// 0x00025dbf
		fmul st(0), st(3)		// 0x00025dc3
		faddp st(1), st(0)		// 0x00025dc5
		fld dword ptr [esp + 0x20]		// 0x00025dc7
		fmul dword ptr [esp + 0x4c]		// 0x00025dcb
		faddp st(1), st(0)		// 0x00025dcf
		fld dword ptr [esp + 0x3c]		// 0x00025dd1
		fmul dword ptr [esp + 0x54]		// 0x00025dd5
		fld dword ptr [esp + 0x30]		// 0x00025dd9
		fmul st(0), st(4)		// 0x00025ddd
		faddp st(1), st(0)		// 0x00025ddf
		fld dword ptr [esp + 0x24]		// 0x00025de1
		fmul dword ptr [esp + 0x4c]		// 0x00025de5
		faddp st(1), st(0)		// 0x00025de9
		fstp dword ptr [esp + 0x54]		// 0x00025deb
		mov ecx, dword ptr [esp + 0x54]		// 0x00025def
		fxch st(1)		// 0x00025df3
		mov dword ptr [edx + 0x98], ecx		// 0x00025df5
		fstp dword ptr [edx + 0x90]		// 0x00025dfb
		fstp dword ptr [edx + 0x94]		// 0x00025e01
		fstp st(0)		// 0x00025e07
		fld dword ptr [esp + 0x1c]		// 0x00025e09
		fmul dword ptr [eax + 4]		// 0x00025e0d
		fld dword ptr [esp + 0x34]		// 0x00025e10
		fmul dword ptr [eax + 0x1c]		// 0x00025e14
		faddp st(1), st(0)		// 0x00025e17
		fld dword ptr [esp + 0x28]		// 0x00025e19
		fmul dword ptr [eax + 0x10]		// 0x00025e1d
		faddp st(1), st(0)		// 0x00025e20
		fld dword ptr [esp + 0x34]		// 0x00025e22
		fmul dword ptr [eax + 0x20]		// 0x00025e26
		fld dword ptr [esp + 0x28]		// 0x00025e29
		fmul dword ptr [eax + 0x14]		// 0x00025e2d
		faddp st(1), st(0)		// 0x00025e30
		fld dword ptr [esp + 0x1c]		// 0x00025e32
		fmul dword ptr [eax + 8]		// 0x00025e36
		faddp st(1), st(0)		// 0x00025e39
		fld dword ptr [esp + 0x20]		// 0x00025e3b
		fmul dword ptr [eax]		// 0x00025e3f
		fld dword ptr [esp + 0x38]		// 0x00025e41
		fmul dword ptr [eax + 0x18]		// 0x00025e45
		faddp st(1), st(0)		// 0x00025e48
		fld dword ptr [esp + 0x2c]		// 0x00025e4a
		fmul dword ptr [eax + 0xc]		// 0x00025e4e
		faddp st(1), st(0)		// 0x00025e51
		fstp dword ptr [esp + 8]		// 0x00025e53
		fld dword ptr [esp + 0x20]		// 0x00025e57
		fmul dword ptr [eax + 4]		// 0x00025e5b
		fld dword ptr [esp + 0x38]		// 0x00025e5e
		fmul dword ptr [eax + 0x1c]		// 0x00025e62
		faddp st(1), st(0)		// 0x00025e65
		fld dword ptr [esp + 0x2c]		// 0x00025e67
		fmul dword ptr [eax + 0x10]		// 0x00025e6b
		push 1		// 0x00025e6e
		faddp st(1), st(0)		// 0x00025e70
		fstp dword ptr [esp + 0x1c]		// 0x00025e72
		fld dword ptr [esp + 0x3c]		// 0x00025e76
		mov ecx, dword ptr [esp + 0x1c]		// 0x00025e7a
		fmul dword ptr [eax + 0x20]		// 0x00025e7e
		fld dword ptr [esp + 0x30]		// 0x00025e81
		fmul dword ptr [eax + 0x14]		// 0x00025e85
		faddp st(1), st(0)		// 0x00025e88
		fld dword ptr [esp + 0x24]		// 0x00025e8a
		fmul dword ptr [eax + 8]		// 0x00025e8e
		faddp st(1), st(0)		// 0x00025e91
		fstp dword ptr [esp + 0x18]		// 0x00025e93
		fld dword ptr [esp + 0x28]		// 0x00025e97
		fmul dword ptr [eax]		// 0x00025e9b
		fld dword ptr [esp + 0x40]		// 0x00025e9d
		fmul dword ptr [eax + 0x18]		// 0x00025ea1
		faddp st(1), st(0)		// 0x00025ea4
		fld dword ptr [esp + 0x34]		// 0x00025ea6
		fmul dword ptr [eax + 0xc]		// 0x00025eaa
		faddp st(1), st(0)		// 0x00025ead
		fstp dword ptr [esp + 0x14]		// 0x00025eaf
		fld dword ptr [esp + 0x28]		// 0x00025eb3
		fmul dword ptr [eax + 4]		// 0x00025eb7
		fld dword ptr [esp + 0x40]		// 0x00025eba
		fmul dword ptr [eax + 0x1c]		// 0x00025ebe
		faddp st(1), st(0)		// 0x00025ec1
		fld dword ptr [esp + 0x34]		// 0x00025ec3
		fmul dword ptr [eax + 0x10]		// 0x00025ec7
		faddp st(1), st(0)		// 0x00025eca
		fstp dword ptr [esp + 0x10]		// 0x00025ecc
		fld dword ptr [esp + 0x40]		// 0x00025ed0
		fmul dword ptr [eax + 0x20]		// 0x00025ed4
		fld dword ptr [esp + 0x34]		// 0x00025ed7
		fmul dword ptr [eax + 0x14]		// 0x00025edb
		faddp st(1), st(0)		// 0x00025ede
		fld dword ptr [esp + 0x28]		// 0x00025ee0
		fmul dword ptr [eax + 8]		// 0x00025ee4
		faddp st(1), st(0)		// 0x00025ee7
		fstp dword ptr [esp + 0x5c]		// 0x00025ee9
		fld dword ptr [esp + 0x20]		// 0x00025eed
		fmul dword ptr [eax]		// 0x00025ef1
		fld dword ptr [esp + 0x38]		// 0x00025ef3
		fmul dword ptr [eax + 0x18]		// 0x00025ef7
		faddp st(1), st(0)		// 0x00025efa
		fld dword ptr [esp + 0x2c]		// 0x00025efc
		fmul dword ptr [eax + 0xc]		// 0x00025f00
		mov eax, dword ptr [esp + 0xc]		// 0x00025f03
		mov dword ptr [edx + 0x7c], ecx		// 0x00025f07
		mov ecx, dword ptr [esp + 0x14]		// 0x00025f0a
		faddp st(1), st(0)		// 0x00025f0e
		mov dword ptr [edx + 0x78], eax		// 0x00025f10
		mov eax, dword ptr [esp + 0x18]		// 0x00025f13
		mov dword ptr [edx + 0x84], ecx		// 0x00025f17
		mov ecx, dword ptr [esp + 0x5c]		// 0x00025f1d
		fstp dword ptr [edx + 0x6c]		// 0x00025f21
		fxch st(1)		// 0x00025f24
		mov dword ptr [edx + 0x80], eax		// 0x00025f26
		mov eax, dword ptr [esp + 0x10]		// 0x00025f2c
		fstp dword ptr [edx + 0x70]		// 0x00025f30
		mov dword ptr [edx + 0x8c], ecx		// 0x00025f33
		fstp dword ptr [edx + 0x74]		// 0x00025f39
		mov dword ptr [edx + 0x88], eax		// 0x00025f3c
		mov eax, dword ptr [edx]		// 0x00025f42
		mov ecx, edx		// 0x00025f44
		call dword ptr [eax + 0x18]		// 0x00025f46
		pop edi		// 0x00025f49
		mov dword ptr [gNxApiReentry], 0		// 0x00025f4a
		pop esi		// 0x00025f54
		add esp, 0x84		// 0x00025f55
		ret 4		// 0x00025f5b
		}
	}

// phys_fn_001309 (0x00025f60, 797 B), 229 instructions
__declspec(naked) unsigned __fastcall nxShp1309(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0x84		// 0x00025f60
		mov eax, ecx		// 0x00025f66
		mov ecx, dword ptr [eax + 4]		// 0x00025f68
		mov edx, dword ptr [ecx + 8]		// 0x00025f6b
		test edx, edx		// 0x00025f6e
		push esi		// 0x00025f70
		push edi		// 0x00025f71
		mov dword ptr [esp + 0x4c], 0		// 0x00025f72
		je L26067		// 0x00025f7a
		fld dword ptr [edx + 0x68]		// 0x00025f80
		mov ecx, 9		// 0x00025f83
		fld dword ptr [edx + 0x5c]		// 0x00025f88
		lea esi, [esp + 0x1c]		// 0x00025f8b
		fld dword ptr [edx + 0x60]		// 0x00025f8f
		lea edi, [esp + 0x5c]		// 0x00025f92
		fld dword ptr [edx + 0x64]		// 0x00025f96
		fld st(1)		// 0x00025f99
		fmul st(0), st(2)		// 0x00025f9b
		fadd st(0), st(0)		// 0x00025f9d
		fstp dword ptr [esp + 8]		// 0x00025f9f
		fld st(0)		// 0x00025fa3
		fmul st(0), st(1)		// 0x00025fa5
		fadd st(0), st(0)		// 0x00025fa7
		fld dword ptr [nxShpK101041ec_4]		// 0x00025fa9
		fsub dword ptr [esp + 8]		// 0x00025faf
		fsub st(0), st(1)		// 0x00025fb3
		fstp dword ptr [esp + 0x1c]		// 0x00025fb5
		fld st(2)		// 0x00025fb9
		fmul st(0), st(4)		// 0x00025fbb
		fadd st(0), st(0)		// 0x00025fbd
		fld st(2)		// 0x00025fbf
		fmul st(0), st(6)		// 0x00025fc1
		fadd st(0), st(0)		// 0x00025fc3
		fld st(1)		// 0x00025fc5
		fsub st(0), st(1)		// 0x00025fc7
		fstp dword ptr [esp + 0x20]		// 0x00025fc9
		fld st(3)		// 0x00025fcd
		fmul st(0), st(6)		// 0x00025fcf
		fadd st(0), st(0)		// 0x00025fd1
		fstp dword ptr [esp + 0xc]		// 0x00025fd3
		fld st(4)		// 0x00025fd7
		fmul st(0), st(7)		// 0x00025fd9
		fadd st(0), st(0)		// 0x00025fdb
		fst dword ptr [esp + 0x14]		// 0x00025fdd
		fadd dword ptr [esp + 0xc]		// 0x00025fe1
		fstp dword ptr [esp + 0x24]		// 0x00025fe5
		fadd st(0), st(1)		// 0x00025fe9
		fstp dword ptr [esp + 0x28]		// 0x00025feb
		fstp st(0)		// 0x00025fef
		fld st(3)		// 0x00025ff1
		fmul st(0), st(4)		// 0x00025ff3
		fadd st(0), st(0)		// 0x00025ff5
		fsubr dword ptr [nxShpK101041ec_4]		// 0x00025ff7
		fst dword ptr [esp + 0x10]		// 0x00025ffd
		fsub st(0), st(1)		// 0x00026001
		fstp dword ptr [esp + 0x2c]		// 0x00026003
		fstp st(0)		// 0x00026007
		fmul st(0), st(1)		// 0x00026009
		fadd st(0), st(0)		// 0x0002600b
		fstp dword ptr [esp + 0x18]		// 0x0002600d
		fstp st(0)		// 0x00026011
		fmulp st(1), st(0)		// 0x00026013
		fadd st(0), st(0)		// 0x00026015
		fld dword ptr [esp + 0x18]		// 0x00026017
		fsub st(0), st(1)		// 0x0002601b
		fstp dword ptr [esp + 0x30]		// 0x0002601d
		fld dword ptr [esp + 0xc]		// 0x00026021
		fsub dword ptr [esp + 0x14]		// 0x00026025
		fstp dword ptr [esp + 0x34]		// 0x00026029
		fadd dword ptr [esp + 0x18]		// 0x0002602d
		fstp dword ptr [esp + 0x38]		// 0x00026031
		fld dword ptr [esp + 0x10]		// 0x00026035
		fsub dword ptr [esp + 8]		// 0x00026039
		fstp dword ptr [esp + 0x3c]		// 0x0002603d
		rep movsd		// 0x00026041
		mov ecx, dword ptr [edx + 0x50]		// 0x00026043
		mov dword ptr [esp + 0x80], ecx		// 0x00026046
		mov ecx, dword ptr [edx + 0x54]		// 0x0002604d
		mov edx, dword ptr [edx + 0x58]		// 0x00026050
		mov dword ptr [esp + 0x88], edx		// 0x00026053
		mov dword ptr [esp + 0x84], ecx		// 0x0002605a
		lea edx, [esp + 0x5c]		// 0x00026061
		jmp L2606a		// 0x00026065
L26067:
		lea edx, [ecx + 0x20]		// 0x00026067
L2606a:
		fld dword ptr [edx + 0x24]		// 0x0002606a
		mov esi, edx		// 0x0002606d
		mov ecx, 9		// 0x0002606f
		lea edi, [esp + 0x1c]		// 0x00026074
		rep movsd		// 0x00026078
		fld dword ptr [esp + 0x1c]		// 0x0002607a
		fmul dword ptr [eax + 0x90]		// 0x0002607e
		fld dword ptr [esp + 0x20]		// 0x00026084
		fmul dword ptr [eax + 0x94]		// 0x00026088
		faddp st(1), st(0)		// 0x0002608e
		fld dword ptr [esp + 0x24]		// 0x00026090
		fmul dword ptr [eax + 0x98]		// 0x00026094
		faddp st(1), st(0)		// 0x0002609a
		mov ecx, dword ptr [edx + 0x28]		// 0x0002609c
		fld dword ptr [esp + 0x28]		// 0x0002609f
		mov edx, dword ptr [edx + 0x2c]		// 0x000260a3
		fmul dword ptr [eax + 0x90]		// 0x000260a6
		mov dword ptr [esp + 0x44], ecx		// 0x000260ac
		fld dword ptr [esp + 0x2c]		// 0x000260b0
		mov ecx, dword ptr [esp + 0x90]		// 0x000260b4
		fmul dword ptr [eax + 0x94]		// 0x000260bb
		mov dword ptr [esp + 0x48], edx		// 0x000260c1
		faddp st(1), st(0)		// 0x000260c5
		fld dword ptr [esp + 0x30]		// 0x000260c7
		fmul dword ptr [eax + 0x98]		// 0x000260cb
		faddp st(1), st(0)		// 0x000260d1
		fstp dword ptr [esp + 0x54]		// 0x000260d3
		fld dword ptr [esp + 0x34]		// 0x000260d7
		fmul dword ptr [eax + 0x90]		// 0x000260db
		fld dword ptr [esp + 0x38]		// 0x000260e1
		fmul dword ptr [eax + 0x94]		// 0x000260e5
		faddp st(1), st(0)		// 0x000260eb
		fld dword ptr [esp + 0x3c]		// 0x000260ed
		fmul dword ptr [eax + 0x98]		// 0x000260f1
		faddp st(1), st(0)		// 0x000260f7
		fstp dword ptr [esp + 0x58]		// 0x000260f9
		faddp st(1), st(0)		// 0x000260fd
		fld dword ptr [esp + 0x44]		// 0x000260ff
		fadd dword ptr [esp + 0x54]		// 0x00026103
		fld dword ptr [esp + 0x48]		// 0x00026107
		fadd dword ptr [esp + 0x58]		// 0x0002610b
		fstp dword ptr [esp + 0x58]		// 0x0002610f
		mov edx, dword ptr [esp + 0x58]		// 0x00026113
		fxch st(1)		// 0x00026117
		mov dword ptr [ecx + 0x2c], edx		// 0x00026119
		fstp dword ptr [ecx + 0x24]		// 0x0002611c
		fstp dword ptr [ecx + 0x28]		// 0x0002611f
		fld dword ptr [esp + 0x1c]		// 0x00026122
		fmul dword ptr [eax + 0x70]		// 0x00026126
		fld dword ptr [esp + 0x20]		// 0x00026129
		fmul dword ptr [eax + 0x7c]		// 0x0002612d
		faddp st(1), st(0)		// 0x00026130
		fld dword ptr [esp + 0x24]		// 0x00026132
		fmul dword ptr [eax + 0x88]		// 0x00026136
		faddp st(1), st(0)		// 0x0002613c
		fld dword ptr [esp + 0x20]		// 0x0002613e
		fmul dword ptr [eax + 0x80]		// 0x00026142
		fld dword ptr [esp + 0x24]		// 0x00026148
		fmul dword ptr [eax + 0x8c]		// 0x0002614c
		faddp st(1), st(0)		// 0x00026152
		fld dword ptr [esp + 0x1c]		// 0x00026154
		fmul dword ptr [eax + 0x74]		// 0x00026158
		faddp st(1), st(0)		// 0x0002615b
		fld dword ptr [esp + 0x28]		// 0x0002615d
		fmul dword ptr [eax + 0x6c]		// 0x00026161
		fld dword ptr [esp + 0x2c]		// 0x00026164
		fmul dword ptr [eax + 0x78]		// 0x00026168
		faddp st(1), st(0)		// 0x0002616b
		fld dword ptr [esp + 0x30]		// 0x0002616d
		fmul dword ptr [eax + 0x84]		// 0x00026171
		faddp st(1), st(0)		// 0x00026177
		fstp dword ptr [esp + 8]		// 0x00026179
		fld dword ptr [esp + 0x28]		// 0x0002617d
		fmul dword ptr [eax + 0x70]		// 0x00026181
		fld dword ptr [esp + 0x2c]		// 0x00026184
		fmul dword ptr [eax + 0x7c]		// 0x00026188
		faddp st(1), st(0)		// 0x0002618b
		fld dword ptr [esp + 0x30]		// 0x0002618d
		fmul dword ptr [eax + 0x88]		// 0x00026191
		faddp st(1), st(0)		// 0x00026197
		fstp dword ptr [esp + 0x10]		// 0x00026199
		fld dword ptr [esp + 0x2c]		// 0x0002619d
		mov edx, dword ptr [esp + 0x10]		// 0x000261a1
		fmul dword ptr [eax + 0x80]		// 0x000261a5
		pop edi		// 0x000261ab
		fld dword ptr [esp + 0x2c]		// 0x000261ac
		pop esi		// 0x000261b0
		fmul dword ptr [eax + 0x8c]		// 0x000261b1
		faddp st(1), st(0)		// 0x000261b7
		fld dword ptr [esp + 0x20]		// 0x000261b9
		fmul dword ptr [eax + 0x74]		// 0x000261bd
		faddp st(1), st(0)		// 0x000261c0
		fstp dword ptr [esp + 0x10]		// 0x000261c2
		fld dword ptr [esp + 0x2c]		// 0x000261c6
		fmul dword ptr [eax + 0x6c]		// 0x000261ca
		fld dword ptr [esp + 0x30]		// 0x000261cd
		fmul dword ptr [eax + 0x78]		// 0x000261d1
		faddp st(1), st(0)		// 0x000261d4
		fld dword ptr [esp + 0x34]		// 0x000261d6
		fmul dword ptr [eax + 0x84]		// 0x000261da
		faddp st(1), st(0)		// 0x000261e0
		fstp dword ptr [esp + 0xc]		// 0x000261e2
		fld dword ptr [esp + 0x2c]		// 0x000261e6
		fmul dword ptr [eax + 0x70]		// 0x000261ea
		fld dword ptr [esp + 0x30]		// 0x000261ed
		fmul dword ptr [eax + 0x7c]		// 0x000261f1
		faddp st(1), st(0)		// 0x000261f4
		fld dword ptr [esp + 0x34]		// 0x000261f6
		fmul dword ptr [eax + 0x88]		// 0x000261fa
		faddp st(1), st(0)		// 0x00026200
		fstp dword ptr [esp + 4]		// 0x00026202
		fld dword ptr [esp + 0x30]		// 0x00026206
		fmul dword ptr [eax + 0x80]		// 0x0002620a
		fld dword ptr [esp + 0x34]		// 0x00026210
		fmul dword ptr [eax + 0x8c]		// 0x00026214
		faddp st(1), st(0)		// 0x0002621a
		fld dword ptr [esp + 0x2c]		// 0x0002621c
		fmul dword ptr [eax + 0x74]		// 0x00026220
		faddp st(1), st(0)		// 0x00026223
		fstp dword ptr [esp + 0x44]		// 0x00026225
		fld dword ptr [esp + 0x14]		// 0x00026229
		fmul dword ptr [eax + 0x6c]		// 0x0002622d
		fld dword ptr [esp + 0x18]		// 0x00026230
		fmul dword ptr [eax + 0x78]		// 0x00026234
		faddp st(1), st(0)		// 0x00026237
		fld dword ptr [esp + 0x1c]		// 0x00026239
		fmul dword ptr [eax + 0x84]		// 0x0002623d
		mov eax, dword ptr [esp]		// 0x00026243
		mov dword ptr [ecx + 0xc], eax		// 0x00026246
		mov eax, dword ptr [esp + 0x10]		// 0x00026249
		faddp st(1), st(0)		// 0x0002624d
		mov dword ptr [ecx + 0x10], edx		// 0x0002624f
		mov edx, dword ptr [esp + 0xc]		// 0x00026252
		mov dword ptr [ecx + 0x14], eax		// 0x00026256
		mov eax, dword ptr [esp + 4]		// 0x00026259
		fstp dword ptr [ecx]		// 0x0002625d
		fxch st(1)		// 0x0002625f
		mov dword ptr [ecx + 0x18], edx		// 0x00026261
		mov edx, dword ptr [esp + 0x44]		// 0x00026264
		fstp dword ptr [ecx + 4]		// 0x00026268
		mov dword ptr [ecx + 0x1c], eax		// 0x0002626b
		fstp dword ptr [ecx + 8]		// 0x0002626e
		mov dword ptr [ecx + 0x20], edx		// 0x00026271
		add esp, 0x84		// 0x00026274
		ret 4		// 0x0002627a
		}
	}

// phys_fn_001311 (0x00026280, 424 B), 132 instructions
__declspec(naked) unsigned __fastcall nxShp1311(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0x74		// 0x00026280
		mov eax, ecx		// 0x00026283
		mov ecx, dword ptr [eax + 4]		// 0x00026285
		mov edx, dword ptr [ecx + 8]		// 0x00026288
		test edx, edx		// 0x0002628b
		push esi		// 0x0002628d
		push edi		// 0x0002628e
		mov dword ptr [esp + 0x18], 0		// 0x0002628f
		je L2637b		// 0x00026297
		fld dword ptr [edx + 0x68]		// 0x0002629d
		mov ecx, 9		// 0x000262a0
		fld dword ptr [edx + 0x5c]		// 0x000262a5
		lea esi, [esp + 0x1c]		// 0x000262a8
		fld dword ptr [edx + 0x60]		// 0x000262ac
		lea edi, [esp + 0x4c]		// 0x000262af
		fld dword ptr [edx + 0x64]		// 0x000262b3
		fld st(1)		// 0x000262b6
		fmul st(0), st(2)		// 0x000262b8
		fadd st(0), st(0)		// 0x000262ba
		fstp dword ptr [esp + 0x18]		// 0x000262bc
		fld st(0)		// 0x000262c0
		fmul st(0), st(1)		// 0x000262c2
		fadd st(0), st(0)		// 0x000262c4
		fld dword ptr [nxShpK101041ec_4]		// 0x000262c6
		fsub dword ptr [esp + 0x18]		// 0x000262cc
		fsub st(0), st(1)		// 0x000262d0
		fstp dword ptr [esp + 0x1c]		// 0x000262d2
		fld st(2)		// 0x000262d6
		fmul st(0), st(4)		// 0x000262d8
		fadd st(0), st(0)		// 0x000262da
		fld st(2)		// 0x000262dc
		fmul st(0), st(6)		// 0x000262de
		fadd st(0), st(0)		// 0x000262e0
		fld st(1)		// 0x000262e2
		fsub st(0), st(1)		// 0x000262e4
		fstp dword ptr [esp + 0x20]		// 0x000262e6
		fld st(3)		// 0x000262ea
		fmul st(0), st(6)		// 0x000262ec
		fadd st(0), st(0)		// 0x000262ee
		fstp dword ptr [esp + 8]		// 0x000262f0
		fld st(4)		// 0x000262f4
		fmul st(0), st(7)		// 0x000262f6
		fadd st(0), st(0)		// 0x000262f8
		fst dword ptr [esp + 0xc]		// 0x000262fa
		fadd dword ptr [esp + 8]		// 0x000262fe
		fstp dword ptr [esp + 0x24]		// 0x00026302
		fadd st(0), st(1)		// 0x00026306
		fstp dword ptr [esp + 0x28]		// 0x00026308
		fstp st(0)		// 0x0002630c
		fld st(3)		// 0x0002630e
		fmul st(0), st(4)		// 0x00026310
		fadd st(0), st(0)		// 0x00026312
		fsubr dword ptr [nxShpK101041ec_4]		// 0x00026314
		fst dword ptr [esp + 0x14]		// 0x0002631a
		fsub st(0), st(1)		// 0x0002631e
		fstp dword ptr [esp + 0x2c]		// 0x00026320
		fstp st(0)		// 0x00026324
		fmul st(0), st(1)		// 0x00026326
		fadd st(0), st(0)		// 0x00026328
		fstp dword ptr [esp + 0x10]		// 0x0002632a
		fstp st(0)		// 0x0002632e
		fmulp st(1), st(0)		// 0x00026330
		fadd st(0), st(0)		// 0x00026332
		fld dword ptr [esp + 0x10]		// 0x00026334
		fsub st(0), st(1)		// 0x00026338
		fstp dword ptr [esp + 0x30]		// 0x0002633a
		fld dword ptr [esp + 8]		// 0x0002633e
		fsub dword ptr [esp + 0xc]		// 0x00026342
		fstp dword ptr [esp + 0x34]		// 0x00026346
		fadd dword ptr [esp + 0x10]		// 0x0002634a
		fstp dword ptr [esp + 0x38]		// 0x0002634e
		fld dword ptr [esp + 0x14]		// 0x00026352
		fsub dword ptr [esp + 0x18]		// 0x00026356
		fstp dword ptr [esp + 0x3c]		// 0x0002635a
		rep movsd		// 0x0002635e
		mov ecx, dword ptr [edx + 0x50]		// 0x00026360
		mov dword ptr [esp + 0x70], ecx		// 0x00026363
		mov ecx, dword ptr [edx + 0x54]		// 0x00026367
		mov edx, dword ptr [edx + 0x58]		// 0x0002636a
		mov dword ptr [esp + 0x78], edx		// 0x0002636d
		mov dword ptr [esp + 0x74], ecx		// 0x00026371
		lea edx, [esp + 0x4c]		// 0x00026375
		jmp L2637e		// 0x00026379
L2637b:
		lea edx, [ecx + 0x20]		// 0x0002637b
L2637e:
		fld dword ptr [edx + 0x24]		// 0x0002637e
		mov ecx, 9		// 0x00026381
		fld dword ptr [edx + 0x28]		// 0x00026386
		mov esi, edx		// 0x00026389
		lea edi, [esp + 0x1c]		// 0x0002638b
		rep movsd		// 0x0002638f
		fld dword ptr [esp + 0x28]		// 0x00026391
		fmul dword ptr [eax + 0x90]		// 0x00026395
		fld dword ptr [esp + 0x2c]		// 0x0002639b
		fmul dword ptr [eax + 0x94]		// 0x0002639f
		faddp st(1), st(0)		// 0x000263a5
		fld dword ptr [esp + 0x30]		// 0x000263a7
		fmul dword ptr [eax + 0x98]		// 0x000263ab
		faddp st(1), st(0)		// 0x000263b1
		mov ecx, dword ptr [edx + 0x2c]		// 0x000263b3
		fld dword ptr [esp + 0x34]		// 0x000263b6
		mov dword ptr [esp + 0x48], ecx		// 0x000263ba
		fmul dword ptr [eax + 0x90]		// 0x000263be
		pop edi		// 0x000263c4
		fld dword ptr [esp + 0x34]		// 0x000263c5
		pop esi		// 0x000263c9
		fmul dword ptr [eax + 0x94]		// 0x000263ca
		faddp st(1), st(0)		// 0x000263d0
		fld dword ptr [esp + 0x34]		// 0x000263d2
		fmul dword ptr [eax + 0x98]		// 0x000263d6
		faddp st(1), st(0)		// 0x000263dc
		fstp dword ptr [esp + 0x10]		// 0x000263de
		fld dword ptr [esp + 0x14]		// 0x000263e2
		fmul dword ptr [eax + 0x90]		// 0x000263e6
		fld dword ptr [esp + 0x18]		// 0x000263ec
		fmul dword ptr [eax + 0x94]		// 0x000263f0
		faddp st(1), st(0)		// 0x000263f6
		fld dword ptr [esp + 0x1c]		// 0x000263f8
		fmul dword ptr [eax + 0x98]		// 0x000263fc
		mov eax, dword ptr [esp + 0x78]		// 0x00026402
		faddp st(1), st(0)		// 0x00026406
		fadd st(0), st(3)		// 0x00026408
		fstp dword ptr [eax]		// 0x0002640a
		fxch st(1)		// 0x0002640c
		fadd st(0), st(1)		// 0x0002640e
		fstp dword ptr [eax + 4]		// 0x00026410
		fstp st(0)		// 0x00026413
		fstp st(0)		// 0x00026415
		fld dword ptr [esp + 0x40]		// 0x00026417
		fadd dword ptr [esp + 0x10]		// 0x0002641b
		fstp dword ptr [eax + 8]		// 0x0002641f
		add esp, 0x74		// 0x00026422
		ret 4		// 0x00026425
		}
	}

// phys_fn_001313 (0x00026430, 617 B), 186 instructions
__declspec(naked) unsigned __fastcall nxShp1313(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0x78		// 0x00026430
		mov eax, ecx		// 0x00026433
		mov ecx, dword ptr [eax + 4]		// 0x00026435
		mov edx, dword ptr [ecx + 8]		// 0x00026438
		test edx, edx		// 0x0002643b
		push esi		// 0x0002643d
		push edi		// 0x0002643e
		mov dword ptr [esp + 0x1c], 0		// 0x0002643f
		je L2652b		// 0x00026447
		fld dword ptr [edx + 0x68]		// 0x0002644d
		mov ecx, 9		// 0x00026450
		fld dword ptr [edx + 0x5c]		// 0x00026455
		lea esi, [esp + 0x20]		// 0x00026458
		fld dword ptr [edx + 0x60]		// 0x0002645c
		lea edi, [esp + 0x50]		// 0x0002645f
		fld dword ptr [edx + 0x64]		// 0x00026463
		fld st(1)		// 0x00026466
		fmul st(0), st(2)		// 0x00026468
		fadd st(0), st(0)		// 0x0002646a
		fstp dword ptr [esp + 0x18]		// 0x0002646c
		fld st(0)		// 0x00026470
		fmul st(0), st(1)		// 0x00026472
		fadd st(0), st(0)		// 0x00026474
		fld dword ptr [nxShpK101041ec_4]		// 0x00026476
		fsub dword ptr [esp + 0x18]		// 0x0002647c
		fsub st(0), st(1)		// 0x00026480
		fstp dword ptr [esp + 0x20]		// 0x00026482
		fld st(2)		// 0x00026486
		fmul st(0), st(4)		// 0x00026488
		fadd st(0), st(0)		// 0x0002648a
		fld st(2)		// 0x0002648c
		fmul st(0), st(6)		// 0x0002648e
		fadd st(0), st(0)		// 0x00026490
		fld st(1)		// 0x00026492
		fsub st(0), st(1)		// 0x00026494
		fstp dword ptr [esp + 0x24]		// 0x00026496
		fld st(3)		// 0x0002649a
		fmul st(0), st(6)		// 0x0002649c
		fadd st(0), st(0)		// 0x0002649e
		fstp dword ptr [esp + 8]		// 0x000264a0
		fld st(4)		// 0x000264a4
		fmul st(0), st(7)		// 0x000264a6
		fadd st(0), st(0)		// 0x000264a8
		fst dword ptr [esp + 0xc]		// 0x000264aa
		fadd dword ptr [esp + 8]		// 0x000264ae
		fstp dword ptr [esp + 0x28]		// 0x000264b2
		fadd st(0), st(1)		// 0x000264b6
		fstp dword ptr [esp + 0x2c]		// 0x000264b8
		fstp st(0)		// 0x000264bc
		fld st(3)		// 0x000264be
		fmul st(0), st(4)		// 0x000264c0
		fadd st(0), st(0)		// 0x000264c2
		fsubr dword ptr [nxShpK101041ec_4]		// 0x000264c4
		fst dword ptr [esp + 0x14]		// 0x000264ca
		fsub st(0), st(1)		// 0x000264ce
		fstp dword ptr [esp + 0x30]		// 0x000264d0
		fstp st(0)		// 0x000264d4
		fmul st(0), st(1)		// 0x000264d6
		fadd st(0), st(0)		// 0x000264d8
		fstp dword ptr [esp + 0x10]		// 0x000264da
		fstp st(0)		// 0x000264de
		fmulp st(1), st(0)		// 0x000264e0
		fadd st(0), st(0)		// 0x000264e2
		fld dword ptr [esp + 0x10]		// 0x000264e4
		fsub st(0), st(1)		// 0x000264e8
		fstp dword ptr [esp + 0x34]		// 0x000264ea
		fld dword ptr [esp + 8]		// 0x000264ee
		fsub dword ptr [esp + 0xc]		// 0x000264f2
		fstp dword ptr [esp + 0x38]		// 0x000264f6
		fadd dword ptr [esp + 0x10]		// 0x000264fa
		fstp dword ptr [esp + 0x3c]		// 0x000264fe
		fld dword ptr [esp + 0x14]		// 0x00026502
		fsub dword ptr [esp + 0x18]		// 0x00026506
		fstp dword ptr [esp + 0x40]		// 0x0002650a
		rep movsd		// 0x0002650e
		mov ecx, dword ptr [edx + 0x50]		// 0x00026510
		mov dword ptr [esp + 0x74], ecx		// 0x00026513
		mov ecx, dword ptr [edx + 0x54]		// 0x00026517
		mov edx, dword ptr [edx + 0x58]		// 0x0002651a
		mov dword ptr [esp + 0x78], ecx		// 0x0002651d
		mov dword ptr [esp + 0x7c], edx		// 0x00026521
		lea esi, [esp + 0x50]		// 0x00026525
		jmp L2652e		// 0x00026529
L2652b:
		lea esi, [ecx + 0x20]		// 0x0002652b
L2652e:
		mov ecx, 9		// 0x0002652e
		lea edi, [esp + 0x20]		// 0x00026533
		rep movsd		// 0x00026537
		fld dword ptr [esp + 0x20]		// 0x00026539
		fmul dword ptr [eax + 0x70]		// 0x0002653d
		fld dword ptr [esp + 0x24]		// 0x00026540
		fmul dword ptr [eax + 0x7c]		// 0x00026544
		faddp st(1), st(0)		// 0x00026547
		fld dword ptr [esp + 0x28]		// 0x00026549
		fmul dword ptr [eax + 0x88]		// 0x0002654d
		faddp st(1), st(0)		// 0x00026553
		fld dword ptr [esp + 0x20]		// 0x00026555
		fmul dword ptr [eax + 0x74]		// 0x00026559
		fld dword ptr [esp + 0x24]		// 0x0002655c
		fmul dword ptr [eax + 0x80]		// 0x00026560
		faddp st(1), st(0)		// 0x00026566
		fld dword ptr [esp + 0x28]		// 0x00026568
		fmul dword ptr [eax + 0x8c]		// 0x0002656c
		faddp st(1), st(0)		// 0x00026572
		fld dword ptr [esp + 0x2c]		// 0x00026574
		fmul dword ptr [eax + 0x6c]		// 0x00026578
		fld dword ptr [esp + 0x30]		// 0x0002657b
		fmul dword ptr [eax + 0x78]		// 0x0002657f
		faddp st(1), st(0)		// 0x00026582
		fld dword ptr [esp + 0x34]		// 0x00026584
		fmul dword ptr [eax + 0x84]		// 0x00026588
		faddp st(1), st(0)		// 0x0002658e
		fstp dword ptr [esp + 0x18]		// 0x00026590
		fld dword ptr [esp + 0x2c]		// 0x00026594
		fmul dword ptr [eax + 0x70]		// 0x00026598
		fld dword ptr [esp + 0x30]		// 0x0002659b
		fmul dword ptr [eax + 0x7c]		// 0x0002659f
		faddp st(1), st(0)		// 0x000265a2
		fld dword ptr [esp + 0x34]		// 0x000265a4
		fmul dword ptr [eax + 0x88]		// 0x000265a8
		faddp st(1), st(0)		// 0x000265ae
		fstp dword ptr [esp + 0x14]		// 0x000265b0
		fld dword ptr [esp + 0x2c]		// 0x000265b4
		fmul dword ptr [eax + 0x74]		// 0x000265b8
		fld dword ptr [esp + 0x30]		// 0x000265bb
		fmul dword ptr [eax + 0x80]		// 0x000265bf
		faddp st(1), st(0)		// 0x000265c5
		fld dword ptr [esp + 0x34]		// 0x000265c7
		fmul dword ptr [eax + 0x8c]		// 0x000265cb
		faddp st(1), st(0)		// 0x000265d1
		fstp dword ptr [esp + 0x10]		// 0x000265d3
		fld dword ptr [esp + 0x38]		// 0x000265d7
		fmul dword ptr [eax + 0x6c]		// 0x000265db
		fld dword ptr [esp + 0x3c]		// 0x000265de
		fmul dword ptr [eax + 0x78]		// 0x000265e2
		faddp st(1), st(0)		// 0x000265e5
		fld dword ptr [esp + 0x40]		// 0x000265e7
		fmul dword ptr [eax + 0x84]		// 0x000265eb
		faddp st(1), st(0)		// 0x000265f1
		fstp dword ptr [esp + 0xc]		// 0x000265f3
		fld dword ptr [esp + 0x38]		// 0x000265f7
		fmul dword ptr [eax + 0x70]		// 0x000265fb
		fld dword ptr [esp + 0x3c]		// 0x000265fe
		fmul dword ptr [eax + 0x7c]		// 0x00026602
		faddp st(1), st(0)		// 0x00026605
		fld dword ptr [esp + 0x40]		// 0x00026607
		fmul dword ptr [eax + 0x88]		// 0x0002660b
		faddp st(1), st(0)		// 0x00026611
		fstp dword ptr [esp + 8]		// 0x00026613
		fld dword ptr [esp + 0x38]		// 0x00026617
		fmul dword ptr [eax + 0x74]		// 0x0002661b
		fld dword ptr [esp + 0x3c]		// 0x0002661e
		fmul dword ptr [eax + 0x80]		// 0x00026622
		faddp st(1), st(0)		// 0x00026628
		fld dword ptr [esp + 0x40]		// 0x0002662a
		fmul dword ptr [eax + 0x8c]		// 0x0002662e
		faddp st(1), st(0)		// 0x00026634
		fstp dword ptr [esp + 0x1c]		// 0x00026636
		fld dword ptr [esp + 0x20]		// 0x0002663a
		fmul dword ptr [eax + 0x6c]		// 0x0002663e
		fld dword ptr [esp + 0x24]		// 0x00026641
		fmul dword ptr [eax + 0x78]		// 0x00026645
		faddp st(1), st(0)		// 0x00026648
		fld dword ptr [esp + 0x28]		// 0x0002664a
		fmul dword ptr [eax + 0x84]		// 0x0002664e
		faddp st(1), st(0)		// 0x00026654
		mov eax, dword ptr [esp + 0x84]		// 0x00026656
		mov ecx, dword ptr [esp + 0x18]		// 0x0002665d
		fstp dword ptr [eax]		// 0x00026661
		mov edx, dword ptr [esp + 0x14]		// 0x00026663
		fxch st(1)		// 0x00026667
		mov dword ptr [eax + 0xc], ecx		// 0x00026669
		fstp dword ptr [eax + 4]		// 0x0002666c
		mov ecx, dword ptr [esp + 0x10]		// 0x0002666f
		mov dword ptr [eax + 0x10], edx		// 0x00026673
		fstp dword ptr [eax + 8]		// 0x00026676
		mov edx, dword ptr [esp + 0xc]		// 0x00026679
		mov dword ptr [eax + 0x14], ecx		// 0x0002667d
		mov ecx, dword ptr [esp + 8]		// 0x00026680
		mov dword ptr [eax + 0x18], edx		// 0x00026684
		mov edx, dword ptr [esp + 0x1c]		// 0x00026687
		pop edi		// 0x0002668b
		mov dword ptr [eax + 0x1c], ecx		// 0x0002668c
		mov dword ptr [eax + 0x20], edx		// 0x0002668f
		pop esi		// 0x00026692
		add esp, 0x78		// 0x00026693
		ret 4		// 0x00026696
		}
	}

// phys_fn_001317 (0x00026ad0, 157 B), 33 instructions
__declspec(naked) unsigned __fastcall nxShp1317(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0x30		// 0x00026ad0
		push esi		// 0x00026ad3
		push edi		// 0x00026ad4
		lea eax, [esp + 8]		// 0x00026ad5
		push eax		// 0x00026ad9
		mov dword ptr [esp + 0x38], 0		// 0x00026ada
		mov dword ptr [esp + 0x34], 0		// 0x00026ae2
		mov dword ptr [esp + 0x30], 0		// 0x00026aea
		mov dword ptr [esp + 0xc], 0x3f800000		// 0x00026af2
		mov dword ptr [esp + 0x10], 0		// 0x00026afa
		mov dword ptr [esp + 0x14], 0		// 0x00026b02
		mov dword ptr [esp + 0x18], 0		// 0x00026b0a
		mov dword ptr [esp + 0x1c], 0x3f800000		// 0x00026b12
		mov dword ptr [esp + 0x20], 0		// 0x00026b1a
		mov dword ptr [esp + 0x24], 0		// 0x00026b22
		mov dword ptr [esp + 0x28], 0		// 0x00026b2a
		mov dword ptr [esp + 0x2c], 0x3f800000		// 0x00026b32
		call nxShp1309		// 0x00026b3a
		mov eax, dword ptr [esp + 0x3c]		// 0x00026b3f
		mov edx, dword ptr [esp + 0x30]		// 0x00026b43
		mov ecx, 9		// 0x00026b47
		lea esi, [esp + 8]		// 0x00026b4c
		mov edi, eax		// 0x00026b50
		rep movsd		// 0x00026b52
		mov ecx, dword ptr [esp + 0x2c]		// 0x00026b54
		mov dword ptr [eax + 0x24], ecx		// 0x00026b58
		mov ecx, dword ptr [esp + 0x34]		// 0x00026b5b
		pop edi		// 0x00026b5f
		mov dword ptr [eax + 0x28], edx		// 0x00026b60
		mov dword ptr [eax + 0x2c], ecx		// 0x00026b63
		pop esi		// 0x00026b66
		add esp, 0x30		// 0x00026b67
		ret 4		// 0x00026b6a
		}
	}

// phys_fn_001319 (0x00026b70, 41 B), 13 instructions
__declspec(naked) unsigned __fastcall nxShp1319(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0xc		// 0x00026b70
		lea eax, [esp]		// 0x00026b73
		push eax		// 0x00026b76
		call nxShp1311		// 0x00026b77
		fld dword ptr [esp]		// 0x00026b7c
		mov eax, dword ptr [esp + 0x10]		// 0x00026b7f
		mov ecx, dword ptr [esp + 4]		// 0x00026b83
		fstp dword ptr [eax]		// 0x00026b87
		mov edx, dword ptr [esp + 8]		// 0x00026b89
		mov dword ptr [eax + 4], ecx		// 0x00026b8d
		mov dword ptr [eax + 8], edx		// 0x00026b90
		add esp, 0xc		// 0x00026b93
		ret 4		// 0x00026b96
		}
	}

// phys_fn_001321 (0x00026ba0, 40 B), 15 instructions
__declspec(naked) unsigned __fastcall nxShp1321(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0x24		// 0x00026ba0
		push esi		// 0x00026ba3
		push edi		// 0x00026ba4
		lea eax, [esp + 8]		// 0x00026ba5
		push eax		// 0x00026ba9
		call nxShp1313		// 0x00026baa
		mov eax, dword ptr [esp + 0x30]		// 0x00026baf
		mov ecx, 9		// 0x00026bb3
		lea esi, [esp + 8]		// 0x00026bb8
		mov edi, eax		// 0x00026bbc
		rep movsd		// 0x00026bbe
		pop edi		// 0x00026bc0
		pop esi		// 0x00026bc1
		add esp, 0x24		// 0x00026bc2
		ret 4		// 0x00026bc5
		}
	}

// phys_fn_001325 and phys_fn_001327 (one function) (0x00026c90, 243 B), 91 instructions
__declspec(naked) unsigned __fastcall nxShp1325(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ecx		// 0x00026c90
		mov eax, dword ptr [ecx + 4]		// 0x00026c91
		test eax, eax		// 0x00026c94
		je L26d7f		// 0x00026c96
		mov eax, dword ptr [eax + 4]		// 0x00026c9c
		push ebp		// 0x00026c9f
		mov ebp, dword ptr [ecx + 0xd4]		// 0x00026ca0
		push esi		// 0x00026ca6
		mov esi, dword ptr [eax + 0x48]		// 0x00026ca7
		mov ecx, dword ptr [esi]		// 0x00026caa
		cmp dword ptr [ecx + ebp*4], 0		// 0x00026cac
		mov dword ptr [esp + 8], ebp		// 0x00026cb0
		jne L26d6e		// 0x00026cb4
		mov edx, dword ptr [esi + 0x14]		// 0x00026cba
		mov eax, dword ptr [esi + 0x20]		// 0x00026cbd
		push ebx		// 0x00026cc0
		sub edx, dword ptr [esi + 0x10]		// 0x00026cc1
		sar edx, 2		// 0x00026cc4
		mov dword ptr [eax + ebp*4], edx		// 0x00026cc7
		mov ecx, dword ptr [esi + 0x18]		// 0x00026cca
		mov eax, dword ptr [esi + 0x14]		// 0x00026ccd
		cmp ecx, eax		// 0x00026cd0
		ja L26d61		// 0x00026cd2
		mov edx, dword ptr [esi + 0x10]		// 0x00026cd8
		sub eax, edx		// 0x00026cdb
		sar eax, 2		// 0x00026cdd
		test edx, edx		// 0x00026ce0
		lea eax, [eax + eax + 2]		// 0x00026ce2
		jne L26cec		// 0x00026ce6
		xor ecx, ecx		// 0x00026ce8
		jmp L26cf1		// 0x00026cea
L26cec:
		sub ecx, edx		// 0x00026cec
		sar ecx, 2		// 0x00026cee
L26cf1:
		cmp ecx, eax		// 0x00026cf1
		jae L26d61		// 0x00026cf3
		mov ecx, dword ptr [nxShpImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00026cf5
		mov ecx, dword ptr [ecx]		// 0x00026cfb
		mov edx, dword ptr [ecx]		// 0x00026cfd
		push edi		// 0x00026cff
		lea ebx, [eax*4]		// 0x00026d00
		push 0		// 0x00026d07
		push ebx		// 0x00026d09
		call dword ptr [edx + 8]		// 0x00026d0a
		mov ecx, dword ptr [esi + 0x14]		// 0x00026d0d
		mov edi, eax		// 0x00026d10
		mov eax, dword ptr [esi + 0x10]		// 0x00026d12
		cmp eax, ecx		// 0x00026d15
		mov edx, edi		// 0x00026d17
		je L26d32		// 0x00026d19
		jmp L26d20		// 0x00026d1b
		lea ecx, [ecx]		// 0x00026d1d
L26d20:
		mov ebp, dword ptr [eax]		// 0x00026d20
		mov dword ptr [edx], ebp		// 0x00026d22
		add eax, 4		// 0x00026d24
		add edx, 4		// 0x00026d27
		cmp eax, ecx		// 0x00026d2a
		jne L26d20		// 0x00026d2c
		mov ebp, dword ptr [esp + 0x10]		// 0x00026d2e
L26d32:
		mov eax, dword ptr [esi + 0x10]		// 0x00026d32
		test eax, eax		// 0x00026d35
		je L26d47		// 0x00026d37
		mov ecx, dword ptr [nxShpImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00026d39
		mov ecx, dword ptr [ecx]		// 0x00026d3f
		mov edx, dword ptr [ecx]		// 0x00026d41
		push eax		// 0x00026d43
		call dword ptr [edx + 0x14]		// 0x00026d44
L26d47:
		mov eax, dword ptr [esi + 0x14]		// 0x00026d47
		mov edx, dword ptr [esi + 0x10]		// 0x00026d4a
		add ebx, edi		// 0x00026d4d
		sub eax, edx		// 0x00026d4f
		sar eax, 2		// 0x00026d51
		lea ecx, [edi + eax*4]		// 0x00026d54
		mov dword ptr [esi + 0x10], edi		// 0x00026d57
		mov dword ptr [esi + 0x18], ebx		// 0x00026d5a
		mov dword ptr [esi + 0x14], ecx		// 0x00026d5d
		pop edi		// 0x00026d60
L26d61:
		mov eax, dword ptr [esi + 0x14]		// 0x00026d61
		add eax, 4		// 0x00026d64
		mov dword ptr [esi + 0x14], eax		// 0x00026d67
		mov dword ptr [eax - 4], ebp		// 0x00026d6a
		pop ebx		// 0x00026d6d
L26d6e:
		mov edx, dword ptr [esi]		// 0x00026d6e
		mov ecx, dword ptr [esp + 0x10]		// 0x00026d70
		lea eax, [edx + ebp*4]		// 0x00026d74
		mov edx, dword ptr [eax]		// 0x00026d77
		or edx, ecx		// 0x00026d79
		pop esi		// 0x00026d7b
		mov dword ptr [eax], edx		// 0x00026d7c
		pop ebp		// 0x00026d7e
L26d7f:
		pop ecx		// 0x00026d7f
		ret 4		// 0x00026d80
		}
	}

// phys_fn_001331 (0x00026df0, 53 B), 14 instructions
__declspec(naked) unsigned __fastcall nxShp1331(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov al, byte ptr [esp + 8]		// 0x00026df0
		test al, al		// 0x00026df4
		je L26e0e		// 0x00026df6
		mov ax, word ptr [esp + 4]		// 0x00026df8
		or word ptr [ecx + 0xde], ax		// 0x00026dfd
		push 0x10		// 0x00026e04
		call nxShp1325		// 0x00026e06
		ret 8		// 0x00026e0b
L26e0e:
		mov edx, dword ptr [esp + 4]		// 0x00026e0e
		not edx		// 0x00026e12
		and word ptr [ecx + 0xde], dx		// 0x00026e14
		push 0x10		// 0x00026e1b
		call nxShp1325		// 0x00026e1d
		ret 8		// 0x00026e22
		}
	}

// phys_fn_001333 (0x00026e30, 139 B), 46 instructions
__declspec(naked) unsigned __fastcall nxShp1333(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [gNxApiReentry]		// 0x00026e30
		test eax, eax		// 0x00026e35
		push ebx		// 0x00026e37
		mov ebx, ecx		// 0x00026e38
		je L26e69		// 0x00026e3a
		mov eax, dword ptr [nxShpImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00026e3c
		cmp dword ptr [eax], 0		// 0x00026e41
		jne L26e47		// 0x00026e44
		int 3		// 0x00026e46
L26e47:
		mov ecx, dword ptr [gNxApiReentryMessage]		// 0x00026e47
		push ecx		// 0x00026e4d
		push 0		// 0x00026e4e
		push 0x10d		// 0x00026e50
		push offset nxShpStr1074c4		// 0x00026e55
		push 2		// 0x00026e5a
		call dword ptr [nxShpImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00026e5c
		add esp, 0x14		// 0x00026e62
		pop ebx		// 0x00026e65
		ret 4		// 0x00026e66
L26e69:
		mov edx, dword ptr [esp + 8]		// 0x00026e69
		push esi		// 0x00026e6d
		mov dword ptr [gNxApiReentry], 1		// 0x00026e6e
		lea eax, [ebx + 0x6c]		// 0x00026e78
		push edi		// 0x00026e7b
		mov esi, edx		// 0x00026e7c
		mov edi, eax		// 0x00026e7e
		mov ecx, 9		// 0x00026e80
		rep movsd		// 0x00026e85
		mov ecx, dword ptr [edx + 0x24]		// 0x00026e87
		mov dword ptr [eax + 0x24], ecx		// 0x00026e8a
		mov ecx, dword ptr [edx + 0x28]		// 0x00026e8d
		mov dword ptr [eax + 0x28], ecx		// 0x00026e90
		mov edx, dword ptr [edx + 0x2c]		// 0x00026e93
		push 1		// 0x00026e96
		mov ecx, ebx		// 0x00026e98
		mov dword ptr [eax + 0x2c], edx		// 0x00026e9a
		call nxShp1325		// 0x00026e9d
		mov eax, dword ptr [ebx]		// 0x00026ea2
		push 1		// 0x00026ea4
		mov ecx, ebx		// 0x00026ea6
		call dword ptr [eax + 0x18]		// 0x00026ea8
		pop edi		// 0x00026eab
		pop esi		// 0x00026eac
		mov dword ptr [gNxApiReentry], 0		// 0x00026ead
		pop ebx		// 0x00026eb7
		ret 4		// 0x00026eb8
		}
	}

// phys_fn_001335 (0x00026ec0, 129 B), 37 instructions
__declspec(naked) unsigned __fastcall nxShp1335(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [gNxApiReentry]		// 0x00026ec0
		test eax, eax		// 0x00026ec5
		push esi		// 0x00026ec7
		mov esi, ecx		// 0x00026ec8
		je L26ef9		// 0x00026eca
		mov eax, dword ptr [nxShpImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00026ecc
		cmp dword ptr [eax], 0		// 0x00026ed1
		jne L26ed7		// 0x00026ed4
		int 3		// 0x00026ed6
L26ed7:
		mov ecx, dword ptr [gNxApiReentryMessage]		// 0x00026ed7
		push ecx		// 0x00026edd
		push 0		// 0x00026ede
		push 0x115		// 0x00026ee0
		push offset nxShpStr1074c4		// 0x00026ee5
		push 2		// 0x00026eea
		call dword ptr [nxShpImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00026eec
		add esp, 0x14		// 0x00026ef2
		pop esi		// 0x00026ef5
		ret 4		// 0x00026ef6
L26ef9:
		mov eax, dword ptr [esp + 8]		// 0x00026ef9
		mov dword ptr [gNxApiReentry], 1		// 0x00026efd
		mov edx, dword ptr [eax]		// 0x00026f07
		mov dword ptr [esi + 0x90], edx		// 0x00026f09
		mov ecx, dword ptr [eax + 4]		// 0x00026f0f
		mov dword ptr [esi + 0x94], ecx		// 0x00026f12
		mov edx, dword ptr [eax + 8]		// 0x00026f18
		push 1		// 0x00026f1b
		mov ecx, esi		// 0x00026f1d
		mov dword ptr [esi + 0x98], edx		// 0x00026f1f
		call nxShp1325		// 0x00026f25
		mov eax, dword ptr [esi]		// 0x00026f2a
		push 1		// 0x00026f2c
		mov ecx, esi		// 0x00026f2e
		call dword ptr [eax + 0x18]		// 0x00026f30
		mov dword ptr [gNxApiReentry], 0		// 0x00026f33
		pop esi		// 0x00026f3d
		ret 4		// 0x00026f3e
		}
	}

// phys_fn_001337 (0x00026f50, 117 B), 38 instructions
__declspec(naked) unsigned __fastcall nxShp1337(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [gNxApiReentry]		// 0x00026f50
		test eax, eax		// 0x00026f55
		push ebx		// 0x00026f57
		mov ebx, ecx		// 0x00026f58
		je L26f89		// 0x00026f5a
		mov eax, dword ptr [nxShpImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00026f5c
		cmp dword ptr [eax], 0		// 0x00026f61
		jne L26f67		// 0x00026f64
		int 3		// 0x00026f66
L26f67:
		mov ecx, dword ptr [gNxApiReentryMessage]		// 0x00026f67
		push ecx		// 0x00026f6d
		push 0		// 0x00026f6e
		push 0x11d		// 0x00026f70
		push offset nxShpStr1074c4		// 0x00026f75
		push 2		// 0x00026f7a
		call dword ptr [nxShpImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00026f7c
		add esp, 0x14		// 0x00026f82
		pop ebx		// 0x00026f85
		ret 4		// 0x00026f86
L26f89:
		push esi		// 0x00026f89
		mov esi, dword ptr [esp + 0xc]		// 0x00026f8a
		push edi		// 0x00026f8e
		mov dword ptr [gNxApiReentry], 1		// 0x00026f8f
		lea edi, [ebx + 0x6c]		// 0x00026f99
		mov ecx, 9		// 0x00026f9c
		rep movsd		// 0x00026fa1
		push 1		// 0x00026fa3
		mov ecx, ebx		// 0x00026fa5
		call nxShp1325		// 0x00026fa7
		mov edx, dword ptr [ebx]		// 0x00026fac
		push 1		// 0x00026fae
		mov ecx, ebx		// 0x00026fb0
		call dword ptr [edx + 0x18]		// 0x00026fb2
		pop edi		// 0x00026fb5
		pop esi		// 0x00026fb6
		mov dword ptr [gNxApiReentry], 0		// 0x00026fb7
		pop ebx		// 0x00026fc1
		ret 4		// 0x00026fc2
		}
	}

// phys_fn_001339 (0x00026fd0, 905 B), 258 instructions
__declspec(naked) unsigned __fastcall nxShp1339(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0xb0		// 0x00026fd0
		mov eax, dword ptr [gNxApiReentry]		// 0x00026fd6
		test eax, eax		// 0x00026fdb
		push ebx		// 0x00026fdd
		mov ebx, ecx		// 0x00026fde
		mov dword ptr [esp + 0x34], 0		// 0x00026fe0
		je L2701d		// 0x00026fe8
		mov eax, dword ptr [nxShpImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00026fea
		cmp dword ptr [eax], 0		// 0x00026fef
		jne L26ff5		// 0x00026ff2
		int 3		// 0x00026ff4
L26ff5:
		mov ecx, dword ptr [gNxApiReentryMessage]		// 0x00026ff5
		push ecx		// 0x00026ffb
		push 0		// 0x00026ffc
		push 0x13f		// 0x00026ffe
		push offset nxShpStr1074c4		// 0x00027003
		push 2		// 0x00027008
		call dword ptr [nxShpImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x0002700a
		add esp, 0x14		// 0x00027010
		pop ebx		// 0x00027013
		add esp, 0xb0		// 0x00027014
		ret 4		// 0x0002701a
L2701d:
		mov eax, dword ptr [esp + 0xb8]		// 0x0002701d
		push esi		// 0x00027024
		mov dword ptr [gNxApiReentry], 1		// 0x00027025
		mov edx, dword ptr [eax]		// 0x0002702f
		push edi		// 0x00027031
		lea esi, [ebx + 0xc]		// 0x00027032
		mov ecx, 9		// 0x00027035
		lea edi, [esp + 0x5c]		// 0x0002703a
		rep movsd		// 0x0002703e
		mov ecx, dword ptr [eax + 4]		// 0x00027040
		mov dword ptr [esp + 0x84], ecx		// 0x00027043
		mov ecx, dword ptr [ebx + 4]		// 0x0002704a
		mov dword ptr [esp + 0x80], edx		// 0x0002704d
		mov edx, dword ptr [eax + 8]		// 0x00027054
		mov eax, dword ptr [ecx + 8]		// 0x00027057
		test eax, eax		// 0x0002705a
		mov dword ptr [esp + 0x88], edx		// 0x0002705c
		je L27156		// 0x00027063
		fld dword ptr [eax + 0x30]		// 0x00027069
		mov edx, dword ptr [eax + 0x1c]		// 0x0002706c
		fld dword ptr [eax + 0x24]		// 0x0002706f
		mov ecx, 9		// 0x00027072
		fld dword ptr [eax + 0x28]		// 0x00027077
		lea esi, [esp + 0xc]		// 0x0002707a
		fld dword ptr [eax + 0x2c]		// 0x0002707e
		lea edi, [esp + 0x8c]		// 0x00027081
		fld st(1)		// 0x00027088
		fmul st(0), st(2)		// 0x0002708a
		fadd st(0), st(0)		// 0x0002708c
		fstp dword ptr [esp + 0x3c]		// 0x0002708e
		fld st(0)		// 0x00027092
		fmul st(0), st(1)		// 0x00027094
		fadd st(0), st(0)		// 0x00027096
		fld dword ptr [nxShpK101041ec_4]		// 0x00027098
		fsub dword ptr [esp + 0x3c]		// 0x0002709e
		fsub st(0), st(1)		// 0x000270a2
		fstp dword ptr [esp + 0xc]		// 0x000270a4
		fld st(2)		// 0x000270a8
		fmul st(0), st(4)		// 0x000270aa
		fadd st(0), st(0)		// 0x000270ac
		fld st(2)		// 0x000270ae
		fmul st(0), st(6)		// 0x000270b0
		fadd st(0), st(0)		// 0x000270b2
		fld st(1)		// 0x000270b4
		fsub st(0), st(1)		// 0x000270b6
		fstp dword ptr [esp + 0x10]		// 0x000270b8
		fld st(3)		// 0x000270bc
		fmul st(0), st(6)		// 0x000270be
		fadd st(0), st(0)		// 0x000270c0
		fstp dword ptr [esp + 0x54]		// 0x000270c2
		fld st(4)		// 0x000270c6
		fmul st(0), st(7)		// 0x000270c8
		fadd st(0), st(0)		// 0x000270ca
		fst dword ptr [esp + 0x50]		// 0x000270cc
		fadd dword ptr [esp + 0x54]		// 0x000270d0
		fstp dword ptr [esp + 0x14]		// 0x000270d4
		fadd st(0), st(1)		// 0x000270d8
		fstp dword ptr [esp + 0x18]		// 0x000270da
		fstp st(0)		// 0x000270de
		fld st(3)		// 0x000270e0
		fmul st(0), st(4)		// 0x000270e2
		fadd st(0), st(0)		// 0x000270e4
		fsubr dword ptr [nxShpK101041ec_4]		// 0x000270e6
		fst dword ptr [esp + 0x58]		// 0x000270ec
		fsub st(0), st(1)		// 0x000270f0
		fstp dword ptr [esp + 0x1c]		// 0x000270f2
		fstp st(0)		// 0x000270f6
		fmul st(0), st(1)		// 0x000270f8
		fadd st(0), st(0)		// 0x000270fa
		fstp dword ptr [esp + 0x4c]		// 0x000270fc
		fstp st(0)		// 0x00027100
		fmulp st(1), st(0)		// 0x00027102
		fadd st(0), st(0)		// 0x00027104
		fld dword ptr [esp + 0x4c]		// 0x00027106
		fsub st(0), st(1)		// 0x0002710a
		fstp dword ptr [esp + 0x20]		// 0x0002710c
		fld dword ptr [esp + 0x54]		// 0x00027110
		fsub dword ptr [esp + 0x50]		// 0x00027114
		fstp dword ptr [esp + 0x24]		// 0x00027118
		fadd dword ptr [esp + 0x4c]		// 0x0002711c
		fstp dword ptr [esp + 0x28]		// 0x00027120
		fld dword ptr [esp + 0x58]		// 0x00027124
		fsub dword ptr [esp + 0x3c]		// 0x00027128
		fstp dword ptr [esp + 0x2c]		// 0x0002712c
		rep movsd		// 0x00027130
		mov ecx, dword ptr [eax + 0x18]		// 0x00027132
		mov eax, dword ptr [eax + 0x20]		// 0x00027135
		mov dword ptr [esp + 0xb8], eax		// 0x00027138
		mov dword ptr [esp + 0xb0], ecx		// 0x0002713f
		mov dword ptr [esp + 0xb4], edx		// 0x00027146
		lea eax, [esp + 0x8c]		// 0x0002714d
		jmp L27159		// 0x00027154
L27156:
		lea eax, [ecx + 0x20]		// 0x00027156
L27159:
		fld dword ptr [eax + 0x24]		// 0x00027159
		mov ecx, 9		// 0x0002715c
		fld dword ptr [eax + 0x28]		// 0x00027161
		mov esi, eax		// 0x00027164
		fld dword ptr [esp + 0x80]		// 0x00027166
		lea edi, [esp + 0xc]		// 0x0002716d
		fsub st(0), st(2)		// 0x00027171
		rep movsd		// 0x00027173
		fstp dword ptr [esp + 0x40]		// 0x00027175
		fld dword ptr [esp + 0x84]		// 0x00027179
		fsub st(0), st(1)		// 0x00027180
		fstp st(2)		// 0x00027182
		fstp st(0)		// 0x00027184
		fld dword ptr [esp + 0x88]		// 0x00027186
		mov ecx, dword ptr [eax + 0x2c]		// 0x0002718d
		mov dword ptr [esp + 0x38], ecx		// 0x00027190
		fsub dword ptr [esp + 0x38]		// 0x00027194
		fstp dword ptr [esp + 0x48]		// 0x00027198
		fld dword ptr [esp + 0x24]		// 0x0002719c
		fmul dword ptr [esp + 0x48]		// 0x000271a0
		fld dword ptr [esp + 0x18]		// 0x000271a4
		fmul st(0), st(2)		// 0x000271a8
		faddp st(1), st(0)		// 0x000271aa
		fld dword ptr [esp + 0xc]		// 0x000271ac
		fmul dword ptr [esp + 0x40]		// 0x000271b0
		faddp st(1), st(0)		// 0x000271b4
		fld dword ptr [esp + 0x28]		// 0x000271b6
		fmul dword ptr [esp + 0x48]		// 0x000271ba
		fld dword ptr [esp + 0x1c]		// 0x000271be
		fmul st(0), st(3)		// 0x000271c2
		faddp st(1), st(0)		// 0x000271c4
		fld dword ptr [esp + 0x10]		// 0x000271c6
		fmul dword ptr [esp + 0x40]		// 0x000271ca
		faddp st(1), st(0)		// 0x000271ce
		fld dword ptr [esp + 0x2c]		// 0x000271d0
		fmul dword ptr [esp + 0x48]		// 0x000271d4
		fld dword ptr [esp + 0x20]		// 0x000271d8
		fmul st(0), st(4)		// 0x000271dc
		faddp st(1), st(0)		// 0x000271de
		fld dword ptr [esp + 0x14]		// 0x000271e0
		fmul dword ptr [esp + 0x40]		// 0x000271e4
		faddp st(1), st(0)		// 0x000271e8
		fstp dword ptr [esp + 0x48]		// 0x000271ea
		mov edx, dword ptr [esp + 0x48]		// 0x000271ee
		fxch st(1)		// 0x000271f2
		mov dword ptr [ebx + 0x98], edx		// 0x000271f4
		fstp dword ptr [ebx + 0x90]		// 0x000271fa
		fstp dword ptr [ebx + 0x94]		// 0x00027200
		fstp st(0)		// 0x00027206
		fld dword ptr [esp + 0x24]		// 0x00027208
		fmul dword ptr [esp + 0x74]		// 0x0002720c
		fld dword ptr [esp + 0x18]		// 0x00027210
		fmul dword ptr [esp + 0x68]		// 0x00027214
		faddp st(1), st(0)		// 0x00027218
		fld dword ptr [esp + 0xc]		// 0x0002721a
		fmul dword ptr [esp + 0x5c]		// 0x0002721e
		faddp st(1), st(0)		// 0x00027222
		fstp dword ptr [ebx + 0x6c]		// 0x00027224
		fld dword ptr [esp + 0x24]		// 0x00027227
		fmul dword ptr [esp + 0x78]		// 0x0002722b
		fld dword ptr [esp + 0x18]		// 0x0002722f
		fmul dword ptr [esp + 0x6c]		// 0x00027233
		faddp st(1), st(0)		// 0x00027237
		fld dword ptr [esp + 0xc]		// 0x00027239
		fmul dword ptr [esp + 0x60]		// 0x0002723d
		faddp st(1), st(0)		// 0x00027241
		fstp dword ptr [ebx + 0x70]		// 0x00027243
		fld dword ptr [esp + 0x24]		// 0x00027246
		fmul dword ptr [esp + 0x7c]		// 0x0002724a
		fld dword ptr [esp + 0x18]		// 0x0002724e
		fmul dword ptr [esp + 0x70]		// 0x00027252
		faddp st(1), st(0)		// 0x00027256
		fld dword ptr [esp + 0xc]		// 0x00027258
		fmul dword ptr [esp + 0x64]		// 0x0002725c
		faddp st(1), st(0)		// 0x00027260
		fstp dword ptr [ebx + 0x74]		// 0x00027262
		fld dword ptr [esp + 0x28]		// 0x00027265
		fmul dword ptr [esp + 0x74]		// 0x00027269
		fld dword ptr [esp + 0x1c]		// 0x0002726d
		fmul dword ptr [esp + 0x68]		// 0x00027271
		faddp st(1), st(0)		// 0x00027275
		fld dword ptr [esp + 0x10]		// 0x00027277
		push 1		// 0x0002727b
		fmul dword ptr [esp + 0x60]		// 0x0002727d
		mov ecx, ebx		// 0x00027281
		faddp st(1), st(0)		// 0x00027283
		fstp dword ptr [ebx + 0x78]		// 0x00027285
		fld dword ptr [esp + 0x2c]		// 0x00027288
		fmul dword ptr [esp + 0x7c]		// 0x0002728c
		fld dword ptr [esp + 0x20]		// 0x00027290
		fmul dword ptr [esp + 0x70]		// 0x00027294
		faddp st(1), st(0)		// 0x00027298
		fld dword ptr [esp + 0x14]		// 0x0002729a
		fmul dword ptr [esp + 0x64]		// 0x0002729e
		faddp st(1), st(0)		// 0x000272a2
		fstp dword ptr [ebx + 0x7c]		// 0x000272a4
		fld dword ptr [esp + 0x2c]		// 0x000272a7
		fmul dword ptr [esp + 0x80]		// 0x000272ab
		fld dword ptr [esp + 0x20]		// 0x000272b2
		fmul dword ptr [esp + 0x74]		// 0x000272b6
		faddp st(1), st(0)		// 0x000272ba
		fld dword ptr [esp + 0x14]		// 0x000272bc
		fmul dword ptr [esp + 0x68]		// 0x000272c0
		faddp st(1), st(0)		// 0x000272c4
		fstp dword ptr [ebx + 0x80]		// 0x000272c6
		fld dword ptr [esp + 0x30]		// 0x000272cc
		fmul dword ptr [esp + 0x78]		// 0x000272d0
		fld dword ptr [esp + 0x24]		// 0x000272d4
		fmul dword ptr [esp + 0x6c]		// 0x000272d8
		faddp st(1), st(0)		// 0x000272dc
		fld dword ptr [esp + 0x18]		// 0x000272de
		fmul dword ptr [esp + 0x60]		// 0x000272e2
		faddp st(1), st(0)		// 0x000272e6
		fstp dword ptr [ebx + 0x84]		// 0x000272e8
		fld dword ptr [esp + 0x30]		// 0x000272ee
		fmul dword ptr [esp + 0x7c]		// 0x000272f2
		fld dword ptr [esp + 0x24]		// 0x000272f6
		fmul dword ptr [esp + 0x70]		// 0x000272fa
		faddp st(1), st(0)		// 0x000272fe
		fld dword ptr [esp + 0x18]		// 0x00027300
		fmul dword ptr [esp + 0x64]		// 0x00027304
		faddp st(1), st(0)		// 0x00027308
		fstp dword ptr [ebx + 0x88]		// 0x0002730a
		fld dword ptr [esp + 0x30]		// 0x00027310
		fmul dword ptr [esp + 0x80]		// 0x00027314
		fld dword ptr [esp + 0x24]		// 0x0002731b
		fmul dword ptr [esp + 0x74]		// 0x0002731f
		faddp st(1), st(0)		// 0x00027323
		fld dword ptr [esp + 0x18]		// 0x00027325
		fmul dword ptr [esp + 0x68]		// 0x00027329
		faddp st(1), st(0)		// 0x0002732d
		fstp dword ptr [ebx + 0x8c]		// 0x0002732f
		mov eax, dword ptr [ebx]		// 0x00027335
		call dword ptr [eax + 0x18]		// 0x00027337
		push 2		// 0x0002733a
		mov ecx, ebx		// 0x0002733c
		call nxShp1325		// 0x0002733e
		pop edi		// 0x00027343
		pop esi		// 0x00027344
		mov dword ptr [gNxApiReentry], 0		// 0x00027345
		pop ebx		// 0x0002734f
		add esp, 0xb0		// 0x00027350
		ret 4		// 0x00027356
		}
	}

// phys_fn_001341 (0x00027360, 903 B), 257 instructions
__declspec(naked) unsigned __fastcall nxShp1341(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0xb0		// 0x00027360
		mov eax, dword ptr [gNxApiReentry]		// 0x00027366
		test eax, eax		// 0x0002736b
		push ebx		// 0x0002736d
		mov ebx, ecx		// 0x0002736e
		mov dword ptr [esp + 0x34], 0		// 0x00027370
		je L273ad		// 0x00027378
		mov eax, dword ptr [nxShpImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x0002737a
		cmp dword ptr [eax], 0		// 0x0002737f
		jne L27385		// 0x00027382
		int 3		// 0x00027384
L27385:
		mov ecx, dword ptr [gNxApiReentryMessage]		// 0x00027385
		push ecx		// 0x0002738b
		push 0		// 0x0002738c
		push 0x14b		// 0x0002738e
		push offset nxShpStr1074c4		// 0x00027393
		push 2		// 0x00027398
		call dword ptr [nxShpImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x0002739a
		add esp, 0x14		// 0x000273a0
		pop ebx		// 0x000273a3
		add esp, 0xb0		// 0x000273a4
		ret 4		// 0x000273aa
L273ad:
		push esi		// 0x000273ad
		mov esi, dword ptr [esp + 0xbc]		// 0x000273ae
		mov dword ptr [gNxApiReentry], 1		// 0x000273b5
		mov eax, dword ptr [ebx + 0x34]		// 0x000273bf
		mov edx, dword ptr [ebx + 0x30]		// 0x000273c2
		push edi		// 0x000273c5
		mov ecx, 9		// 0x000273c6
		lea edi, [esp + 0x5c]		// 0x000273cb
		rep movsd		// 0x000273cf
		mov ecx, dword ptr [ebx + 0x38]		// 0x000273d1
		mov dword ptr [esp + 0x88], ecx		// 0x000273d4
		mov ecx, dword ptr [ebx + 4]		// 0x000273db
		mov dword ptr [esp + 0x84], eax		// 0x000273de
		mov eax, dword ptr [ecx + 8]		// 0x000273e5
		test eax, eax		// 0x000273e8
		mov dword ptr [esp + 0x80], edx		// 0x000273ea
		je L274e4		// 0x000273f1
		fld dword ptr [eax + 0x30]		// 0x000273f7
		mov edx, dword ptr [eax + 0x18]		// 0x000273fa
		fld dword ptr [eax + 0x24]		// 0x000273fd
		mov ecx, 9		// 0x00027400
		fld dword ptr [eax + 0x28]		// 0x00027405
		lea esi, [esp + 0xc]		// 0x00027408
		fld dword ptr [eax + 0x2c]		// 0x0002740c
		lea edi, [esp + 0x8c]		// 0x0002740f
		fld st(1)		// 0x00027416
		fmul st(0), st(2)		// 0x00027418
		fadd st(0), st(0)		// 0x0002741a
		fstp dword ptr [esp + 0x3c]		// 0x0002741c
		fld st(0)		// 0x00027420
		fmul st(0), st(1)		// 0x00027422
		fadd st(0), st(0)		// 0x00027424
		fld dword ptr [nxShpK101041ec_4]		// 0x00027426
		fsub dword ptr [esp + 0x3c]		// 0x0002742c
		fsub st(0), st(1)		// 0x00027430
		fstp dword ptr [esp + 0xc]		// 0x00027432
		fld st(2)		// 0x00027436
		fmul st(0), st(4)		// 0x00027438
		fadd st(0), st(0)		// 0x0002743a
		fld st(2)		// 0x0002743c
		fmul st(0), st(6)		// 0x0002743e
		fadd st(0), st(0)		// 0x00027440
		fld st(1)		// 0x00027442
		fsub st(0), st(1)		// 0x00027444
		fstp dword ptr [esp + 0x10]		// 0x00027446
		fld st(3)		// 0x0002744a
		fmul st(0), st(6)		// 0x0002744c
		fadd st(0), st(0)		// 0x0002744e
		fstp dword ptr [esp + 0x54]		// 0x00027450
		fld st(4)		// 0x00027454
		fmul st(0), st(7)		// 0x00027456
		fadd st(0), st(0)		// 0x00027458
		fst dword ptr [esp + 0x50]		// 0x0002745a
		fadd dword ptr [esp + 0x54]		// 0x0002745e
		fstp dword ptr [esp + 0x14]		// 0x00027462
		fadd st(0), st(1)		// 0x00027466
		fstp dword ptr [esp + 0x18]		// 0x00027468
		fstp st(0)		// 0x0002746c
		fld st(3)		// 0x0002746e
		fmul st(0), st(4)		// 0x00027470
		fadd st(0), st(0)		// 0x00027472
		fsubr dword ptr [nxShpK101041ec_4]		// 0x00027474
		fst dword ptr [esp + 0x58]		// 0x0002747a
		fsub st(0), st(1)		// 0x0002747e
		fstp dword ptr [esp + 0x1c]		// 0x00027480
		fstp st(0)		// 0x00027484
		fmul st(0), st(1)		// 0x00027486
		fadd st(0), st(0)		// 0x00027488
		fstp dword ptr [esp + 0x4c]		// 0x0002748a
		fstp st(0)		// 0x0002748e
		fmulp st(1), st(0)		// 0x00027490
		fadd st(0), st(0)		// 0x00027492
		fld dword ptr [esp + 0x4c]		// 0x00027494
		fsub st(0), st(1)		// 0x00027498
		fstp dword ptr [esp + 0x20]		// 0x0002749a
		fld dword ptr [esp + 0x54]		// 0x0002749e
		fsub dword ptr [esp + 0x50]		// 0x000274a2
		fstp dword ptr [esp + 0x24]		// 0x000274a6
		fadd dword ptr [esp + 0x4c]		// 0x000274aa
		fstp dword ptr [esp + 0x28]		// 0x000274ae
		fld dword ptr [esp + 0x58]		// 0x000274b2
		fsub dword ptr [esp + 0x3c]		// 0x000274b6
		fstp dword ptr [esp + 0x2c]		// 0x000274ba
		rep movsd		// 0x000274be
		mov ecx, dword ptr [eax + 0x1c]		// 0x000274c0
		mov dword ptr [esp + 0xb0], edx		// 0x000274c3
		mov edx, dword ptr [eax + 0x20]		// 0x000274ca
		mov dword ptr [esp + 0xb4], ecx		// 0x000274cd
		mov dword ptr [esp + 0xb8], edx		// 0x000274d4
		lea eax, [esp + 0x8c]		// 0x000274db
		jmp L274e7		// 0x000274e2
L274e4:
		lea eax, [ecx + 0x20]		// 0x000274e4
L274e7:
		fld dword ptr [eax + 0x24]		// 0x000274e7
		mov esi, eax		// 0x000274ea
		fld dword ptr [eax + 0x28]		// 0x000274ec
		mov eax, dword ptr [eax + 0x2c]		// 0x000274ef
		fld dword ptr [esp + 0x80]		// 0x000274f2
		mov ecx, 9		// 0x000274f9
		fsub st(0), st(2)		// 0x000274fe
		lea edi, [esp + 0xc]		// 0x00027500
		rep movsd		// 0x00027504
		fstp dword ptr [esp + 0x40]		// 0x00027506
		fld dword ptr [esp + 0x84]		// 0x0002750a
		fsub st(0), st(1)		// 0x00027511
		fstp st(2)		// 0x00027513
		fstp st(0)		// 0x00027515
		fld dword ptr [esp + 0x88]		// 0x00027517
		mov dword ptr [esp + 0x38], eax		// 0x0002751e
		fsub dword ptr [esp + 0x38]		// 0x00027522
		fstp dword ptr [esp + 0x48]		// 0x00027526
		fld dword ptr [esp + 0x24]		// 0x0002752a
		fmul dword ptr [esp + 0x48]		// 0x0002752e
		fld dword ptr [esp + 0x18]		// 0x00027532
		fmul st(0), st(2)		// 0x00027536
		faddp st(1), st(0)		// 0x00027538
		fld dword ptr [esp + 0xc]		// 0x0002753a
		fmul dword ptr [esp + 0x40]		// 0x0002753e
		faddp st(1), st(0)		// 0x00027542
		fld dword ptr [esp + 0x28]		// 0x00027544
		fmul dword ptr [esp + 0x48]		// 0x00027548
		fld dword ptr [esp + 0x1c]		// 0x0002754c
		fmul st(0), st(3)		// 0x00027550
		faddp st(1), st(0)		// 0x00027552
		fld dword ptr [esp + 0x10]		// 0x00027554
		fmul dword ptr [esp + 0x40]		// 0x00027558
		faddp st(1), st(0)		// 0x0002755c
		fld dword ptr [esp + 0x2c]		// 0x0002755e
		fmul dword ptr [esp + 0x48]		// 0x00027562
		fld dword ptr [esp + 0x20]		// 0x00027566
		fmul st(0), st(4)		// 0x0002756a
		faddp st(1), st(0)		// 0x0002756c
		fld dword ptr [esp + 0x14]		// 0x0002756e
		fmul dword ptr [esp + 0x40]		// 0x00027572
		faddp st(1), st(0)		// 0x00027576
		fstp dword ptr [esp + 0x48]		// 0x00027578
		mov ecx, dword ptr [esp + 0x48]		// 0x0002757c
		fxch st(1)		// 0x00027580
		mov dword ptr [ebx + 0x98], ecx		// 0x00027582
		fstp dword ptr [ebx + 0x90]		// 0x00027588
		fstp dword ptr [ebx + 0x94]		// 0x0002758e
		fstp st(0)		// 0x00027594
		fld dword ptr [esp + 0x24]		// 0x00027596
		fmul dword ptr [esp + 0x74]		// 0x0002759a
		fld dword ptr [esp + 0x18]		// 0x0002759e
		fmul dword ptr [esp + 0x68]		// 0x000275a2
		faddp st(1), st(0)		// 0x000275a6
		fld dword ptr [esp + 0xc]		// 0x000275a8
		fmul dword ptr [esp + 0x5c]		// 0x000275ac
		faddp st(1), st(0)		// 0x000275b0
		fstp dword ptr [ebx + 0x6c]		// 0x000275b2
		fld dword ptr [esp + 0x24]		// 0x000275b5
		fmul dword ptr [esp + 0x78]		// 0x000275b9
		fld dword ptr [esp + 0x18]		// 0x000275bd
		fmul dword ptr [esp + 0x6c]		// 0x000275c1
		faddp st(1), st(0)		// 0x000275c5
		fld dword ptr [esp + 0xc]		// 0x000275c7
		fmul dword ptr [esp + 0x60]		// 0x000275cb
		faddp st(1), st(0)		// 0x000275cf
		fstp dword ptr [ebx + 0x70]		// 0x000275d1
		fld dword ptr [esp + 0x24]		// 0x000275d4
		fmul dword ptr [esp + 0x7c]		// 0x000275d8
		fld dword ptr [esp + 0x18]		// 0x000275dc
		fmul dword ptr [esp + 0x70]		// 0x000275e0
		faddp st(1), st(0)		// 0x000275e4
		fld dword ptr [esp + 0xc]		// 0x000275e6
		fmul dword ptr [esp + 0x64]		// 0x000275ea
		faddp st(1), st(0)		// 0x000275ee
		fstp dword ptr [ebx + 0x74]		// 0x000275f0
		fld dword ptr [esp + 0x28]		// 0x000275f3
		fmul dword ptr [esp + 0x74]		// 0x000275f7
		fld dword ptr [esp + 0x1c]		// 0x000275fb
		fmul dword ptr [esp + 0x68]		// 0x000275ff
		faddp st(1), st(0)		// 0x00027603
		fld dword ptr [esp + 0x10]		// 0x00027605
		push 1		// 0x00027609
		fmul dword ptr [esp + 0x60]		// 0x0002760b
		mov ecx, ebx		// 0x0002760f
		faddp st(1), st(0)		// 0x00027611
		fstp dword ptr [ebx + 0x78]		// 0x00027613
		fld dword ptr [esp + 0x2c]		// 0x00027616
		fmul dword ptr [esp + 0x7c]		// 0x0002761a
		fld dword ptr [esp + 0x20]		// 0x0002761e
		fmul dword ptr [esp + 0x70]		// 0x00027622
		faddp st(1), st(0)		// 0x00027626
		fld dword ptr [esp + 0x14]		// 0x00027628
		fmul dword ptr [esp + 0x64]		// 0x0002762c
		faddp st(1), st(0)		// 0x00027630
		fstp dword ptr [ebx + 0x7c]		// 0x00027632
		fld dword ptr [esp + 0x2c]		// 0x00027635
		fmul dword ptr [esp + 0x80]		// 0x00027639
		fld dword ptr [esp + 0x20]		// 0x00027640
		fmul dword ptr [esp + 0x74]		// 0x00027644
		faddp st(1), st(0)		// 0x00027648
		fld dword ptr [esp + 0x14]		// 0x0002764a
		fmul dword ptr [esp + 0x68]		// 0x0002764e
		faddp st(1), st(0)		// 0x00027652
		fstp dword ptr [ebx + 0x80]		// 0x00027654
		fld dword ptr [esp + 0x30]		// 0x0002765a
		fmul dword ptr [esp + 0x78]		// 0x0002765e
		fld dword ptr [esp + 0x24]		// 0x00027662
		fmul dword ptr [esp + 0x6c]		// 0x00027666
		faddp st(1), st(0)		// 0x0002766a
		fld dword ptr [esp + 0x18]		// 0x0002766c
		fmul dword ptr [esp + 0x60]		// 0x00027670
		faddp st(1), st(0)		// 0x00027674
		fstp dword ptr [ebx + 0x84]		// 0x00027676
		fld dword ptr [esp + 0x30]		// 0x0002767c
		fmul dword ptr [esp + 0x7c]		// 0x00027680
		fld dword ptr [esp + 0x24]		// 0x00027684
		fmul dword ptr [esp + 0x70]		// 0x00027688
		faddp st(1), st(0)		// 0x0002768c
		fld dword ptr [esp + 0x18]		// 0x0002768e
		fmul dword ptr [esp + 0x64]		// 0x00027692
		faddp st(1), st(0)		// 0x00027696
		fstp dword ptr [ebx + 0x88]		// 0x00027698
		fld dword ptr [esp + 0x30]		// 0x0002769e
		fmul dword ptr [esp + 0x80]		// 0x000276a2
		fld dword ptr [esp + 0x24]		// 0x000276a9
		fmul dword ptr [esp + 0x74]		// 0x000276ad
		faddp st(1), st(0)		// 0x000276b1
		fld dword ptr [esp + 0x18]		// 0x000276b3
		fmul dword ptr [esp + 0x68]		// 0x000276b7
		faddp st(1), st(0)		// 0x000276bb
		fstp dword ptr [ebx + 0x8c]		// 0x000276bd
		mov edx, dword ptr [ebx]		// 0x000276c3
		call dword ptr [edx + 0x18]		// 0x000276c5
		push 2		// 0x000276c8
		mov ecx, ebx		// 0x000276ca
		call nxShp1325		// 0x000276cc
		pop edi		// 0x000276d1
		pop esi		// 0x000276d2
		mov dword ptr [gNxApiReentry], 0		// 0x000276d3
		pop ebx		// 0x000276dd
		add esp, 0xb0		// 0x000276de
		ret 4		// 0x000276e4
		}
	}

// phys_fn_001345 (0x00027710, 34 B), 13 instructions
__declspec(naked) unsigned __fastcall nxShp1345(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00027710
		mov esi, ecx		// 0x00027711
		call nxShpCallNxBaseDtor		// 0x00027713
		test byte ptr [esp + 8], 1		// 0x00027718
		je L2772c		// 0x0002771d
		mov eax, dword ptr [nxShpImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0002771f
		mov ecx, dword ptr [eax]		// 0x00027724
		mov edx, dword ptr [ecx]		// 0x00027726
		push esi		// 0x00027728
		call dword ptr [edx + 0x14]		// 0x00027729
L2772c:
		mov eax, esi		// 0x0002772c
		pop esi		// 0x0002772e
		ret 4		// 0x0002772f
		}
	}
// @@ROWS-END@@
