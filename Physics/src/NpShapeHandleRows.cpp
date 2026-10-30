/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// NpShapeHandleRows.cpp: the public shape handles' methods (NpBoxShape.cpp, NpPlaneShape.cpp, NpSphereShape.cpp,
// NpCapsuleShape.cpp and NpTriangleMeshShape.cpp in the oracle's tree) that were not yet reconstructed: each takes the
// handle's read or write lock (+0x14, +0x10), calls the internal shape (+0x18) and releases the lock; a refused write
// lock reports through the Foundation error stream. Naked listing transcriptions of phys_fn_001099 to 001235; they are
// wired into the handles' vtables in NpActor.cpp by the oracle tables' slots. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase3-shape-handle-rows-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxFacCallLock();
extern "C" void nxFacCallNxPlaneSetEquation();
extern "C" void nxFacCallNxSetMaterialIndex();
extern "C" void nxFacCallNxShp1307();
extern "C" void nxFacCallNxShp1309();
extern "C" void nxFacCallNxShp1311();
extern "C" void nxFacCallNxShp1313();
extern "C" void nxFacCallNxShp1317();
extern "C" void nxFacCallNxShp1319();
extern "C" void nxFacCallNxShp1331();
extern "C" void nxFacCallNxShp1333();
extern "C" void nxFacCallNxShp1335();
extern "C" void nxFacCallNxShp1337();
extern "C" void nxFacCallNxShp1339();
extern "C" void nxFacCallNxShp1341();
extern "C" void nxFacCallSetValue();
extern "C" void nxFacCallTryLock();
extern "C" void nxFacCallUnlock();
extern "C" void* nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx;
extern "C" void* nxFacImpImpinstanceFoundationSDKNxFoundation0PAV;
#pragma comment(linker, "/alternatename:_nxFacCallLock=?lock@ReadWriteLock@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxFacCallNxPlaneSetEquation=?nxPlaneSetEquation@PlaneShape@@QAEXPBMM@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxSetMaterialIndex=?nxSetMaterialIndex@ShapeBase@@QAEXG@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1307=?nxShp1307@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1309=?nxShp1309@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1311=?nxShp1311@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1313=?nxShp1313@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1317=?nxShp1317@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1319=?nxShp1319@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1331=?nxShp1331@@YIIPAX0II@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1333=?nxShp1333@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1335=?nxShp1335@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1337=?nxShp1337@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1339=?nxShp1339@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallNxShp1341=?nxShp1341@@YIIPAX0I@Z")
#pragma comment(linker, "/alternatename:_nxFacCallSetValue=?setValue@SdkPointerBindingThunk@@QAE_NPAX@Z")
#pragma comment(linker, "/alternatename:_nxFacCallTryLock=?tryLock@ReadWriteLock@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxFacCallUnlock=?unlock@ReadWriteLock@@QAE_NXZ")
#pragma comment(linker, "/alternatename:_nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx=__imp_?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ")
#pragma comment(linker, "/alternatename:_nxFacImpImpinstanceFoundationSDKNxFoundation0PAV=__imp_?instance@FoundationSDK@NxFoundation@@0PAV12@A")
extern "C" unsigned gNxApiReentry;
extern "C" const char* gNxApiReentryMessage;

unsigned __fastcall nxFac1099(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1101(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1103(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1105(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1131(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxFac1133(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1135(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1137(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1139(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1141(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1143(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1145(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1147(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1151(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1153(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1157(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxFac1167(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxFac1181(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1211(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxFac1213(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1215(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1217(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1225(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1227(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1229(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1231(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1233(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxFac1235(void* self, void* edxUnused, unsigned a0);
// @@PROTOS-END@@

static const char nxFacStr104760[] = "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!";
static const char nxFacStr106f0c[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpCapsuleShape.cpp";
static const char nxFacStr10707c[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpPlaneShape.cpp";
static const char nxFacStr1071e0[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpSphereShape.cpp";
static const char nxFacStr10734c[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpTriangleMeshShape.cpp";

// phys_fn_001099 (0x00023940, 39 B), 15 instructions
__declspec(naked) unsigned __fastcall nxFac1099(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023940
		mov esi, ecx		// 0x00023941
		push edi		// 0x00023943
		mov edi, dword ptr [esi + 0x14]		// 0x00023944
		mov ecx, edi		// 0x00023947
		call nxFacCallLock		// 0x00023949
		mov eax, dword ptr [esp + 0xc]		// 0x0002394e
		mov ecx, dword ptr [esi + 0x18]		// 0x00023952
		push eax		// 0x00023955
		call nxFacCallNxShp1311		// 0x00023956
		mov ecx, edi		// 0x0002395b
		call nxFacCallUnlock		// 0x0002395d
		pop edi		// 0x00023962
		pop esi		// 0x00023963
		ret 4		// 0x00023964
		}
	}

// phys_fn_001101 (0x00023970, 39 B), 15 instructions
__declspec(naked) unsigned __fastcall nxFac1101(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023970
		mov esi, ecx		// 0x00023971
		push edi		// 0x00023973
		mov edi, dword ptr [esi + 0x14]		// 0x00023974
		mov ecx, edi		// 0x00023977
		call nxFacCallLock		// 0x00023979
		mov eax, dword ptr [esp + 0xc]		// 0x0002397e
		mov ecx, dword ptr [esi + 0x18]		// 0x00023982
		push eax		// 0x00023985
		call nxFacCallNxShp1313		// 0x00023986
		mov ecx, edi		// 0x0002398b
		call nxFacCallUnlock		// 0x0002398d
		pop edi		// 0x00023992
		pop esi		// 0x00023993
		ret 4		// 0x00023994
		}
	}

// phys_fn_001103 (0x000239a0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1103(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000239a0
		mov esi, ecx		// 0x000239a1
		mov ecx, dword ptr [esi + 0x10]		// 0x000239a3
		call nxFacCallTryLock		// 0x000239a6
		test al, al		// 0x000239ab
		jne L239d7		// 0x000239ad
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x000239af
		cmp dword ptr [eax], 0		// 0x000239b4
		jne L239ba		// 0x000239b7
		int 3		// 0x000239b9
L239ba:
		push offset nxFacStr104760		// 0x000239ba
		push 0		// 0x000239bf
		push 0x10		// 0x000239c1
		push offset nxFacStr106f0c		// 0x000239c3
		push 2		// 0x000239c8
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x000239ca
		add esp, 0x14		// 0x000239d0
		pop esi		// 0x000239d3
		ret 4		// 0x000239d4
L239d7:
		mov ecx, dword ptr [esp + 8]		// 0x000239d7
		push edi		// 0x000239db
		mov edi, dword ptr [esi + 0x10]		// 0x000239dc
		push ecx		// 0x000239df
		mov ecx, dword ptr [esi + 0x18]		// 0x000239e0
		call nxFacCallNxSetMaterialIndex		// 0x000239e3
		mov ecx, edi		// 0x000239e8
		call nxFacCallUnlock		// 0x000239ea
		pop edi		// 0x000239ef
		pop esi		// 0x000239f0
		ret 4		// 0x000239f1
		}
	}

// phys_fn_001105 (0x00023a00, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1105(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023a00
		mov esi, ecx		// 0x00023a01
		mov ecx, dword ptr [esi + 0x10]		// 0x00023a03
		call nxFacCallTryLock		// 0x00023a06
		test al, al		// 0x00023a0b
		jne L23a37		// 0x00023a0d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00023a0f
		cmp dword ptr [eax], 0		// 0x00023a14
		jne L23a1a		// 0x00023a17
		int 3		// 0x00023a19
L23a1a:
		push offset nxFacStr104760		// 0x00023a1a
		push 0		// 0x00023a1f
		push 0x10		// 0x00023a21
		push offset nxFacStr106f0c		// 0x00023a23
		push 2		// 0x00023a28
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00023a2a
		add esp, 0x14		// 0x00023a30
		pop esi		// 0x00023a33
		ret 4		// 0x00023a34
L23a37:
		mov ecx, dword ptr [esp + 8]		// 0x00023a37
		push edi		// 0x00023a3b
		mov edi, dword ptr [esi + 0x10]		// 0x00023a3c
		push ecx		// 0x00023a3f
		mov ecx, dword ptr [esi + 0x18]		// 0x00023a40
		call nxFacCallSetValue		// 0x00023a43
		mov ecx, edi		// 0x00023a48
		call nxFacCallUnlock		// 0x00023a4a
		pop edi		// 0x00023a4f
		pop esi		// 0x00023a50
		ret 4		// 0x00023a51
		}
	}

// phys_fn_001131 (0x00023da0, 89 B), 32 instructions
__declspec(naked) unsigned __fastcall nxFac1131(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		push esi		// 0x00023da0
		mov esi, ecx		// 0x00023da1
		mov ecx, dword ptr [esi + 0x10]		// 0x00023da3
		call nxFacCallTryLock		// 0x00023da6
		test al, al		// 0x00023dab
		jne L23dd7		// 0x00023dad
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00023daf
		cmp dword ptr [eax], 0		// 0x00023db4
		jne L23dba		// 0x00023db7
		int 3		// 0x00023db9
L23dba:
		push offset nxFacStr104760		// 0x00023dba
		push 0		// 0x00023dbf
		push 0x16		// 0x00023dc1
		push offset nxFacStr10707c		// 0x00023dc3
		push 2		// 0x00023dc8
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00023dca
		add esp, 0x14		// 0x00023dd0
		pop esi		// 0x00023dd3
		ret 8		// 0x00023dd4
L23dd7:
		mov ecx, dword ptr [esp + 0xc]		// 0x00023dd7
		mov edx, dword ptr [esp + 8]		// 0x00023ddb
		push edi		// 0x00023ddf
		mov edi, dword ptr [esi + 0x10]		// 0x00023de0
		push ecx		// 0x00023de3
		mov ecx, dword ptr [esi + 0x18]		// 0x00023de4
		push edx		// 0x00023de7
		call nxFacCallNxShp1331		// 0x00023de8
		mov ecx, edi		// 0x00023ded
		call nxFacCallUnlock		// 0x00023def
		pop edi		// 0x00023df4
		pop esi		// 0x00023df5
		ret 8		// 0x00023df6
		}
	}

// phys_fn_001133 (0x00023e00, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1133(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023e00
		mov esi, ecx		// 0x00023e01
		mov ecx, dword ptr [esi + 0x10]		// 0x00023e03
		call nxFacCallTryLock		// 0x00023e06
		test al, al		// 0x00023e0b
		jne L23e37		// 0x00023e0d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00023e0f
		cmp dword ptr [eax], 0		// 0x00023e14
		jne L23e1a		// 0x00023e17
		int 3		// 0x00023e19
L23e1a:
		push offset nxFacStr104760		// 0x00023e1a
		push 0		// 0x00023e1f
		push 0x16		// 0x00023e21
		push offset nxFacStr10707c		// 0x00023e23
		push 2		// 0x00023e28
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00023e2a
		add esp, 0x14		// 0x00023e30
		pop esi		// 0x00023e33
		ret 4		// 0x00023e34
L23e37:
		mov ecx, dword ptr [esp + 8]		// 0x00023e37
		push edi		// 0x00023e3b
		mov edi, dword ptr [esi + 0x10]		// 0x00023e3c
		push ecx		// 0x00023e3f
		mov ecx, dword ptr [esi + 0x18]		// 0x00023e40
		call nxFacCallNxShp1333		// 0x00023e43
		mov ecx, edi		// 0x00023e48
		call nxFacCallUnlock		// 0x00023e4a
		pop edi		// 0x00023e4f
		pop esi		// 0x00023e50
		ret 4		// 0x00023e51
		}
	}

// phys_fn_001135 (0x00023e60, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1135(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023e60
		mov esi, ecx		// 0x00023e61
		mov ecx, dword ptr [esi + 0x10]		// 0x00023e63
		call nxFacCallTryLock		// 0x00023e66
		test al, al		// 0x00023e6b
		jne L23e97		// 0x00023e6d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00023e6f
		cmp dword ptr [eax], 0		// 0x00023e74
		jne L23e7a		// 0x00023e77
		int 3		// 0x00023e79
L23e7a:
		push offset nxFacStr104760		// 0x00023e7a
		push 0		// 0x00023e7f
		push 0x16		// 0x00023e81
		push offset nxFacStr10707c		// 0x00023e83
		push 2		// 0x00023e88
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00023e8a
		add esp, 0x14		// 0x00023e90
		pop esi		// 0x00023e93
		ret 4		// 0x00023e94
L23e97:
		mov ecx, dword ptr [esp + 8]		// 0x00023e97
		push edi		// 0x00023e9b
		mov edi, dword ptr [esi + 0x10]		// 0x00023e9c
		push ecx		// 0x00023e9f
		mov ecx, dword ptr [esi + 0x18]		// 0x00023ea0
		call nxFacCallNxShp1335		// 0x00023ea3
		mov ecx, edi		// 0x00023ea8
		call nxFacCallUnlock		// 0x00023eaa
		pop edi		// 0x00023eaf
		pop esi		// 0x00023eb0
		ret 4		// 0x00023eb1
		}
	}

// phys_fn_001137 (0x00023ec0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1137(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023ec0
		mov esi, ecx		// 0x00023ec1
		mov ecx, dword ptr [esi + 0x10]		// 0x00023ec3
		call nxFacCallTryLock		// 0x00023ec6
		test al, al		// 0x00023ecb
		jne L23ef7		// 0x00023ecd
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00023ecf
		cmp dword ptr [eax], 0		// 0x00023ed4
		jne L23eda		// 0x00023ed7
		int 3		// 0x00023ed9
L23eda:
		push offset nxFacStr104760		// 0x00023eda
		push 0		// 0x00023edf
		push 0x16		// 0x00023ee1
		push offset nxFacStr10707c		// 0x00023ee3
		push 2		// 0x00023ee8
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00023eea
		add esp, 0x14		// 0x00023ef0
		pop esi		// 0x00023ef3
		ret 4		// 0x00023ef4
L23ef7:
		mov ecx, dword ptr [esp + 8]		// 0x00023ef7
		push edi		// 0x00023efb
		mov edi, dword ptr [esi + 0x10]		// 0x00023efc
		push ecx		// 0x00023eff
		mov ecx, dword ptr [esi + 0x18]		// 0x00023f00
		call nxFacCallNxShp1337		// 0x00023f03
		mov ecx, edi		// 0x00023f08
		call nxFacCallUnlock		// 0x00023f0a
		pop edi		// 0x00023f0f
		pop esi		// 0x00023f10
		ret 4		// 0x00023f11
		}
	}

// phys_fn_001139 (0x00023f20, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1139(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023f20
		mov esi, ecx		// 0x00023f21
		mov ecx, dword ptr [esi + 0x10]		// 0x00023f23
		call nxFacCallTryLock		// 0x00023f26
		test al, al		// 0x00023f2b
		jne L23f57		// 0x00023f2d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00023f2f
		cmp dword ptr [eax], 0		// 0x00023f34
		jne L23f3a		// 0x00023f37
		int 3		// 0x00023f39
L23f3a:
		push offset nxFacStr104760		// 0x00023f3a
		push 0		// 0x00023f3f
		push 0x16		// 0x00023f41
		push offset nxFacStr10707c		// 0x00023f43
		push 2		// 0x00023f48
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00023f4a
		add esp, 0x14		// 0x00023f50
		pop esi		// 0x00023f53
		ret 4		// 0x00023f54
L23f57:
		mov ecx, dword ptr [esp + 8]		// 0x00023f57
		push edi		// 0x00023f5b
		mov edi, dword ptr [esi + 0x10]		// 0x00023f5c
		push ecx		// 0x00023f5f
		mov ecx, dword ptr [esi + 0x18]		// 0x00023f60
		call nxFacCallNxShp1307		// 0x00023f63
		mov ecx, edi		// 0x00023f68
		call nxFacCallUnlock		// 0x00023f6a
		pop edi		// 0x00023f6f
		pop esi		// 0x00023f70
		ret 4		// 0x00023f71
		}
	}

// phys_fn_001141 (0x00023f80, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1141(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023f80
		mov esi, ecx		// 0x00023f81
		mov ecx, dword ptr [esi + 0x10]		// 0x00023f83
		call nxFacCallTryLock		// 0x00023f86
		test al, al		// 0x00023f8b
		jne L23fb7		// 0x00023f8d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00023f8f
		cmp dword ptr [eax], 0		// 0x00023f94
		jne L23f9a		// 0x00023f97
		int 3		// 0x00023f99
L23f9a:
		push offset nxFacStr104760		// 0x00023f9a
		push 0		// 0x00023f9f
		push 0x16		// 0x00023fa1
		push offset nxFacStr10707c		// 0x00023fa3
		push 2		// 0x00023fa8
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00023faa
		add esp, 0x14		// 0x00023fb0
		pop esi		// 0x00023fb3
		ret 4		// 0x00023fb4
L23fb7:
		mov ecx, dword ptr [esp + 8]		// 0x00023fb7
		push edi		// 0x00023fbb
		mov edi, dword ptr [esi + 0x10]		// 0x00023fbc
		push ecx		// 0x00023fbf
		mov ecx, dword ptr [esi + 0x18]		// 0x00023fc0
		call nxFacCallNxShp1339		// 0x00023fc3
		mov ecx, edi		// 0x00023fc8
		call nxFacCallUnlock		// 0x00023fca
		pop edi		// 0x00023fcf
		pop esi		// 0x00023fd0
		ret 4		// 0x00023fd1
		}
	}

// phys_fn_001143 (0x00023fe0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1143(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00023fe0
		mov esi, ecx		// 0x00023fe1
		mov ecx, dword ptr [esi + 0x10]		// 0x00023fe3
		call nxFacCallTryLock		// 0x00023fe6
		test al, al		// 0x00023feb
		jne L24017		// 0x00023fed
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00023fef
		cmp dword ptr [eax], 0		// 0x00023ff4
		jne L23ffa		// 0x00023ff7
		int 3		// 0x00023ff9
L23ffa:
		push offset nxFacStr104760		// 0x00023ffa
		push 0		// 0x00023fff
		push 0x16		// 0x00024001
		push offset nxFacStr10707c		// 0x00024003
		push 2		// 0x00024008
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x0002400a
		add esp, 0x14		// 0x00024010
		pop esi		// 0x00024013
		ret 4		// 0x00024014
L24017:
		mov ecx, dword ptr [esp + 8]		// 0x00024017
		push edi		// 0x0002401b
		mov edi, dword ptr [esi + 0x10]		// 0x0002401c
		push ecx		// 0x0002401f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024020
		call nxFacCallNxShp1341		// 0x00024023
		mov ecx, edi		// 0x00024028
		call nxFacCallUnlock		// 0x0002402a
		pop edi		// 0x0002402f
		pop esi		// 0x00024030
		ret 4		// 0x00024031
		}
	}

// phys_fn_001145 (0x00024040, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1145(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024040
		mov esi, ecx		// 0x00024041
		mov ecx, dword ptr [esi + 0x10]		// 0x00024043
		call nxFacCallTryLock		// 0x00024046
		test al, al		// 0x0002404b
		jne L24077		// 0x0002404d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x0002404f
		cmp dword ptr [eax], 0		// 0x00024054
		jne L2405a		// 0x00024057
		int 3		// 0x00024059
L2405a:
		push offset nxFacStr104760		// 0x0002405a
		push 0		// 0x0002405f
		push 0x16		// 0x00024061
		push offset nxFacStr10707c		// 0x00024063
		push 2		// 0x00024068
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x0002406a
		add esp, 0x14		// 0x00024070
		pop esi		// 0x00024073
		ret 4		// 0x00024074
L24077:
		mov ecx, dword ptr [esp + 8]		// 0x00024077
		push edi		// 0x0002407b
		mov edi, dword ptr [esi + 0x10]		// 0x0002407c
		push ecx		// 0x0002407f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024080
		call nxFacCallNxSetMaterialIndex		// 0x00024083
		mov ecx, edi		// 0x00024088
		call nxFacCallUnlock		// 0x0002408a
		pop edi		// 0x0002408f
		pop esi		// 0x00024090
		ret 4		// 0x00024091
		}
	}

// phys_fn_001147 (0x000240a0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1147(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000240a0
		mov esi, ecx		// 0x000240a1
		mov ecx, dword ptr [esi + 0x10]		// 0x000240a3
		call nxFacCallTryLock		// 0x000240a6
		test al, al		// 0x000240ab
		jne L240d7		// 0x000240ad
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x000240af
		cmp dword ptr [eax], 0		// 0x000240b4
		jne L240ba		// 0x000240b7
		int 3		// 0x000240b9
L240ba:
		push offset nxFacStr104760		// 0x000240ba
		push 0		// 0x000240bf
		push 0x16		// 0x000240c1
		push offset nxFacStr10707c		// 0x000240c3
		push 2		// 0x000240c8
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x000240ca
		add esp, 0x14		// 0x000240d0
		pop esi		// 0x000240d3
		ret 4		// 0x000240d4
L240d7:
		mov ecx, dword ptr [esp + 8]		// 0x000240d7
		push edi		// 0x000240db
		mov edi, dword ptr [esi + 0x10]		// 0x000240dc
		push ecx		// 0x000240df
		mov ecx, dword ptr [esi + 0x18]		// 0x000240e0
		call nxFacCallSetValue		// 0x000240e3
		mov ecx, edi		// 0x000240e8
		call nxFacCallUnlock		// 0x000240ea
		pop edi		// 0x000240ef
		pop esi		// 0x000240f0
		ret 4		// 0x000240f1
		}
	}

// phys_fn_001151 (0x00024130, 43 B), 18 instructions
__declspec(naked) unsigned __fastcall nxFac1151(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ebx		// 0x00024130
		push esi		// 0x00024131
		mov esi, ecx		// 0x00024132
		push edi		// 0x00024134
		mov edi, dword ptr [esi + 0x14]		// 0x00024135
		mov ecx, edi		// 0x00024138
		call nxFacCallLock		// 0x0002413a
		mov ebx, dword ptr [esp + 0x10]		// 0x0002413f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024143
		push ebx		// 0x00024146
		call nxFacCallNxShp1317		// 0x00024147
		mov ecx, edi		// 0x0002414c
		call nxFacCallUnlock		// 0x0002414e
		pop edi		// 0x00024153
		pop esi		// 0x00024154
		mov eax, ebx		// 0x00024155
		pop ebx		// 0x00024157
		ret 4		// 0x00024158
		}
	}

// phys_fn_001153 (0x00024160, 43 B), 18 instructions
__declspec(naked) unsigned __fastcall nxFac1153(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ebx		// 0x00024160
		push esi		// 0x00024161
		mov esi, ecx		// 0x00024162
		push edi		// 0x00024164
		mov edi, dword ptr [esi + 0x14]		// 0x00024165
		mov ecx, edi		// 0x00024168
		call nxFacCallLock		// 0x0002416a
		mov ebx, dword ptr [esp + 0x10]		// 0x0002416f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024173
		push ebx		// 0x00024176
		call nxFacCallNxShp1319		// 0x00024177
		mov ecx, edi		// 0x0002417c
		call nxFacCallUnlock		// 0x0002417e
		pop edi		// 0x00024183
		pop esi		// 0x00024184
		mov eax, ebx		// 0x00024185
		pop ebx		// 0x00024187
		ret 4		// 0x00024188
		}
	}

// phys_fn_001157 (0x000241f0, 89 B), 32 instructions
__declspec(naked) unsigned __fastcall nxFac1157(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		push esi		// 0x000241f0
		mov esi, ecx		// 0x000241f1
		mov ecx, dword ptr [esi + 0x10]		// 0x000241f3
		call nxFacCallTryLock		// 0x000241f6
		test al, al		// 0x000241fb
		jne L24227		// 0x000241fd
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x000241ff
		cmp dword ptr [eax], 0		// 0x00024204
		jne L2420a		// 0x00024207
		int 3		// 0x00024209
L2420a:
		push offset nxFacStr104760		// 0x0002420a
		push 0		// 0x0002420f
		push 0x28		// 0x00024211
		push offset nxFacStr10707c		// 0x00024213
		push 2		// 0x00024218
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x0002421a
		add esp, 0x14		// 0x00024220
		pop esi		// 0x00024223
		ret 8		// 0x00024224
L24227:
		mov ecx, dword ptr [esp + 0xc]		// 0x00024227
		mov edx, dword ptr [esp + 8]		// 0x0002422b
		push edi		// 0x0002422f
		mov edi, dword ptr [esi + 0x10]		// 0x00024230
		push ecx		// 0x00024233
		mov ecx, dword ptr [esi + 0x18]		// 0x00024234
		push edx		// 0x00024237
		call nxFacCallNxPlaneSetEquation		// 0x00024238
		mov ecx, edi		// 0x0002423d
		call nxFacCallUnlock		// 0x0002423f
		pop edi		// 0x00024244
		pop esi		// 0x00024245
		ret 8		// 0x00024246
		}
	}

// phys_fn_001167 (0x00024340, 89 B), 32 instructions
__declspec(naked) unsigned __fastcall nxFac1167(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		push esi		// 0x00024340
		mov esi, ecx		// 0x00024341
		mov ecx, dword ptr [esi + 0x10]		// 0x00024343
		call nxFacCallTryLock		// 0x00024346
		test al, al		// 0x0002434b
		jne L24377		// 0x0002434d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x0002434f
		cmp dword ptr [eax], 0		// 0x00024354
		jne L2435a		// 0x00024357
		int 3		// 0x00024359
L2435a:
		push offset nxFacStr104760		// 0x0002435a
		push 0		// 0x0002435f
		push 0x16		// 0x00024361
		push offset nxFacStr1071e0		// 0x00024363
		push 2		// 0x00024368
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x0002436a
		add esp, 0x14		// 0x00024370
		pop esi		// 0x00024373
		ret 8		// 0x00024374
L24377:
		mov ecx, dword ptr [esp + 0xc]		// 0x00024377
		mov edx, dword ptr [esp + 8]		// 0x0002437b
		push edi		// 0x0002437f
		mov edi, dword ptr [esi + 0x10]		// 0x00024380
		push ecx		// 0x00024383
		mov ecx, dword ptr [esi + 0x18]		// 0x00024384
		push edx		// 0x00024387
		call nxFacCallNxShp1331		// 0x00024388
		mov ecx, edi		// 0x0002438d
		call nxFacCallUnlock		// 0x0002438f
		pop edi		// 0x00024394
		pop esi		// 0x00024395
		ret 8		// 0x00024396
		}
	}

// phys_fn_001181 (0x000245e0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1181(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000245e0
		mov esi, ecx		// 0x000245e1
		mov ecx, dword ptr [esi + 0x10]		// 0x000245e3
		call nxFacCallTryLock		// 0x000245e6
		test al, al		// 0x000245eb
		jne L24617		// 0x000245ed
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x000245ef
		cmp dword ptr [eax], 0		// 0x000245f4
		jne L245fa		// 0x000245f7
		int 3		// 0x000245f9
L245fa:
		push offset nxFacStr104760		// 0x000245fa
		push 0		// 0x000245ff
		push 0x16		// 0x00024601
		push offset nxFacStr1071e0		// 0x00024603
		push 2		// 0x00024608
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x0002460a
		add esp, 0x14		// 0x00024610
		pop esi		// 0x00024613
		ret 4		// 0x00024614
L24617:
		mov ecx, dword ptr [esp + 8]		// 0x00024617
		push edi		// 0x0002461b
		mov edi, dword ptr [esi + 0x10]		// 0x0002461c
		push ecx		// 0x0002461f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024620
		call nxFacCallNxSetMaterialIndex		// 0x00024623
		mov ecx, edi		// 0x00024628
		call nxFacCallUnlock		// 0x0002462a
		pop edi		// 0x0002462f
		pop esi		// 0x00024630
		ret 4		// 0x00024631
		}
	}

// phys_fn_001211 (0x00024990, 89 B), 32 instructions
__declspec(naked) unsigned __fastcall nxFac1211(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		push esi		// 0x00024990
		mov esi, ecx		// 0x00024991
		mov ecx, dword ptr [esi + 0x10]		// 0x00024993
		call nxFacCallTryLock		// 0x00024996
		test al, al		// 0x0002499b
		jne L249c7		// 0x0002499d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x0002499f
		cmp dword ptr [eax], 0		// 0x000249a4
		jne L249aa		// 0x000249a7
		int 3		// 0x000249a9
L249aa:
		push offset nxFacStr104760		// 0x000249aa
		push 0		// 0x000249af
		push 0x10		// 0x000249b1
		push offset nxFacStr10734c		// 0x000249b3
		push 2		// 0x000249b8
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x000249ba
		add esp, 0x14		// 0x000249c0
		pop esi		// 0x000249c3
		ret 8		// 0x000249c4
L249c7:
		mov ecx, dword ptr [esp + 0xc]		// 0x000249c7
		mov edx, dword ptr [esp + 8]		// 0x000249cb
		push edi		// 0x000249cf
		mov edi, dword ptr [esi + 0x10]		// 0x000249d0
		push ecx		// 0x000249d3
		mov ecx, dword ptr [esi + 0x18]		// 0x000249d4
		push edx		// 0x000249d7
		call nxFacCallNxShp1331		// 0x000249d8
		mov ecx, edi		// 0x000249dd
		call nxFacCallUnlock		// 0x000249df
		pop edi		// 0x000249e4
		pop esi		// 0x000249e5
		ret 8		// 0x000249e6
		}
	}

// phys_fn_001213 (0x000249f0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1213(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000249f0
		mov esi, ecx		// 0x000249f1
		mov ecx, dword ptr [esi + 0x10]		// 0x000249f3
		call nxFacCallTryLock		// 0x000249f6
		test al, al		// 0x000249fb
		jne L24a27		// 0x000249fd
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x000249ff
		cmp dword ptr [eax], 0		// 0x00024a04
		jne L24a0a		// 0x00024a07
		int 3		// 0x00024a09
L24a0a:
		push offset nxFacStr104760		// 0x00024a0a
		push 0		// 0x00024a0f
		push 0x10		// 0x00024a11
		push offset nxFacStr10734c		// 0x00024a13
		push 2		// 0x00024a18
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00024a1a
		add esp, 0x14		// 0x00024a20
		pop esi		// 0x00024a23
		ret 4		// 0x00024a24
L24a27:
		mov ecx, dword ptr [esp + 8]		// 0x00024a27
		push edi		// 0x00024a2b
		mov edi, dword ptr [esi + 0x10]		// 0x00024a2c
		push ecx		// 0x00024a2f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024a30
		call nxFacCallNxShp1333		// 0x00024a33
		mov ecx, edi		// 0x00024a38
		call nxFacCallUnlock		// 0x00024a3a
		pop edi		// 0x00024a3f
		pop esi		// 0x00024a40
		ret 4		// 0x00024a41
		}
	}

// phys_fn_001215 (0x00024a50, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1215(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024a50
		mov esi, ecx		// 0x00024a51
		mov ecx, dword ptr [esi + 0x10]		// 0x00024a53
		call nxFacCallTryLock		// 0x00024a56
		test al, al		// 0x00024a5b
		jne L24a87		// 0x00024a5d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00024a5f
		cmp dword ptr [eax], 0		// 0x00024a64
		jne L24a6a		// 0x00024a67
		int 3		// 0x00024a69
L24a6a:
		push offset nxFacStr104760		// 0x00024a6a
		push 0		// 0x00024a6f
		push 0x10		// 0x00024a71
		push offset nxFacStr10734c		// 0x00024a73
		push 2		// 0x00024a78
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00024a7a
		add esp, 0x14		// 0x00024a80
		pop esi		// 0x00024a83
		ret 4		// 0x00024a84
L24a87:
		mov ecx, dword ptr [esp + 8]		// 0x00024a87
		push edi		// 0x00024a8b
		mov edi, dword ptr [esi + 0x10]		// 0x00024a8c
		push ecx		// 0x00024a8f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024a90
		call nxFacCallNxShp1335		// 0x00024a93
		mov ecx, edi		// 0x00024a98
		call nxFacCallUnlock		// 0x00024a9a
		pop edi		// 0x00024a9f
		pop esi		// 0x00024aa0
		ret 4		// 0x00024aa1
		}
	}

// phys_fn_001217 (0x00024ab0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1217(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024ab0
		mov esi, ecx		// 0x00024ab1
		mov ecx, dword ptr [esi + 0x10]		// 0x00024ab3
		call nxFacCallTryLock		// 0x00024ab6
		test al, al		// 0x00024abb
		jne L24ae7		// 0x00024abd
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00024abf
		cmp dword ptr [eax], 0		// 0x00024ac4
		jne L24aca		// 0x00024ac7
		int 3		// 0x00024ac9
L24aca:
		push offset nxFacStr104760		// 0x00024aca
		push 0		// 0x00024acf
		push 0x10		// 0x00024ad1
		push offset nxFacStr10734c		// 0x00024ad3
		push 2		// 0x00024ad8
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00024ada
		add esp, 0x14		// 0x00024ae0
		pop esi		// 0x00024ae3
		ret 4		// 0x00024ae4
L24ae7:
		mov ecx, dword ptr [esp + 8]		// 0x00024ae7
		push edi		// 0x00024aeb
		mov edi, dword ptr [esi + 0x10]		// 0x00024aec
		push ecx		// 0x00024aef
		mov ecx, dword ptr [esi + 0x18]		// 0x00024af0
		call nxFacCallNxShp1337		// 0x00024af3
		mov ecx, edi		// 0x00024af8
		call nxFacCallUnlock		// 0x00024afa
		pop edi		// 0x00024aff
		pop esi		// 0x00024b00
		ret 4		// 0x00024b01
		}
	}

// phys_fn_001225 (0x00024ba0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1225(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024ba0
		mov esi, ecx		// 0x00024ba1
		mov ecx, dword ptr [esi + 0x10]		// 0x00024ba3
		call nxFacCallTryLock		// 0x00024ba6
		test al, al		// 0x00024bab
		jne L24bd7		// 0x00024bad
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00024baf
		cmp dword ptr [eax], 0		// 0x00024bb4
		jne L24bba		// 0x00024bb7
		int 3		// 0x00024bb9
L24bba:
		push offset nxFacStr104760		// 0x00024bba
		push 0		// 0x00024bbf
		push 0x10		// 0x00024bc1
		push offset nxFacStr10734c		// 0x00024bc3
		push 2		// 0x00024bc8
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00024bca
		add esp, 0x14		// 0x00024bd0
		pop esi		// 0x00024bd3
		ret 4		// 0x00024bd4
L24bd7:
		mov ecx, dword ptr [esp + 8]		// 0x00024bd7
		push edi		// 0x00024bdb
		mov edi, dword ptr [esi + 0x10]		// 0x00024bdc
		push ecx		// 0x00024bdf
		mov ecx, dword ptr [esi + 0x18]		// 0x00024be0
		call nxFacCallNxShp1307		// 0x00024be3
		mov ecx, edi		// 0x00024be8
		call nxFacCallUnlock		// 0x00024bea
		pop edi		// 0x00024bef
		pop esi		// 0x00024bf0
		ret 4		// 0x00024bf1
		}
	}

// phys_fn_001227 (0x00024c00, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1227(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024c00
		mov esi, ecx		// 0x00024c01
		mov ecx, dword ptr [esi + 0x10]		// 0x00024c03
		call nxFacCallTryLock		// 0x00024c06
		test al, al		// 0x00024c0b
		jne L24c37		// 0x00024c0d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00024c0f
		cmp dword ptr [eax], 0		// 0x00024c14
		jne L24c1a		// 0x00024c17
		int 3		// 0x00024c19
L24c1a:
		push offset nxFacStr104760		// 0x00024c1a
		push 0		// 0x00024c1f
		push 0x10		// 0x00024c21
		push offset nxFacStr10734c		// 0x00024c23
		push 2		// 0x00024c28
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00024c2a
		add esp, 0x14		// 0x00024c30
		pop esi		// 0x00024c33
		ret 4		// 0x00024c34
L24c37:
		mov ecx, dword ptr [esp + 8]		// 0x00024c37
		push edi		// 0x00024c3b
		mov edi, dword ptr [esi + 0x10]		// 0x00024c3c
		push ecx		// 0x00024c3f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024c40
		call nxFacCallNxShp1339		// 0x00024c43
		mov ecx, edi		// 0x00024c48
		call nxFacCallUnlock		// 0x00024c4a
		pop edi		// 0x00024c4f
		pop esi		// 0x00024c50
		ret 4		// 0x00024c51
		}
	}

// phys_fn_001229 (0x00024c60, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1229(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024c60
		mov esi, ecx		// 0x00024c61
		mov ecx, dword ptr [esi + 0x10]		// 0x00024c63
		call nxFacCallTryLock		// 0x00024c66
		test al, al		// 0x00024c6b
		jne L24c97		// 0x00024c6d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00024c6f
		cmp dword ptr [eax], 0		// 0x00024c74
		jne L24c7a		// 0x00024c77
		int 3		// 0x00024c79
L24c7a:
		push offset nxFacStr104760		// 0x00024c7a
		push 0		// 0x00024c7f
		push 0x10		// 0x00024c81
		push offset nxFacStr10734c		// 0x00024c83
		push 2		// 0x00024c88
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00024c8a
		add esp, 0x14		// 0x00024c90
		pop esi		// 0x00024c93
		ret 4		// 0x00024c94
L24c97:
		mov ecx, dword ptr [esp + 8]		// 0x00024c97
		push edi		// 0x00024c9b
		mov edi, dword ptr [esi + 0x10]		// 0x00024c9c
		push ecx		// 0x00024c9f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024ca0
		call nxFacCallNxShp1341		// 0x00024ca3
		mov ecx, edi		// 0x00024ca8
		call nxFacCallUnlock		// 0x00024caa
		pop edi		// 0x00024caf
		pop esi		// 0x00024cb0
		ret 4		// 0x00024cb1
		}
	}

// phys_fn_001231 (0x00024cc0, 39 B), 15 instructions
__declspec(naked) unsigned __fastcall nxFac1231(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024cc0
		mov esi, ecx		// 0x00024cc1
		push edi		// 0x00024cc3
		mov edi, dword ptr [esi + 0x14]		// 0x00024cc4
		mov ecx, edi		// 0x00024cc7
		call nxFacCallLock		// 0x00024cc9
		mov eax, dword ptr [esp + 0xc]		// 0x00024cce
		mov ecx, dword ptr [esi + 0x18]		// 0x00024cd2
		push eax		// 0x00024cd5
		call nxFacCallNxShp1309		// 0x00024cd6
		mov ecx, edi		// 0x00024cdb
		call nxFacCallUnlock		// 0x00024cdd
		pop edi		// 0x00024ce2
		pop esi		// 0x00024ce3
		ret 4		// 0x00024ce4
		}
	}

// phys_fn_001233 (0x00024cf0, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1233(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024cf0
		mov esi, ecx		// 0x00024cf1
		mov ecx, dword ptr [esi + 0x10]		// 0x00024cf3
		call nxFacCallTryLock		// 0x00024cf6
		test al, al		// 0x00024cfb
		jne L24d27		// 0x00024cfd
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00024cff
		cmp dword ptr [eax], 0		// 0x00024d04
		jne L24d0a		// 0x00024d07
		int 3		// 0x00024d09
L24d0a:
		push offset nxFacStr104760		// 0x00024d0a
		push 0		// 0x00024d0f
		push 0x10		// 0x00024d11
		push offset nxFacStr10734c		// 0x00024d13
		push 2		// 0x00024d18
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00024d1a
		add esp, 0x14		// 0x00024d20
		pop esi		// 0x00024d23
		ret 4		// 0x00024d24
L24d27:
		mov ecx, dword ptr [esp + 8]		// 0x00024d27
		push edi		// 0x00024d2b
		mov edi, dword ptr [esi + 0x10]		// 0x00024d2c
		push ecx		// 0x00024d2f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024d30
		call nxFacCallNxSetMaterialIndex		// 0x00024d33
		mov ecx, edi		// 0x00024d38
		call nxFacCallUnlock		// 0x00024d3a
		pop edi		// 0x00024d3f
		pop esi		// 0x00024d40
		ret 4		// 0x00024d41
		}
	}

// phys_fn_001235 (0x00024d50, 84 B), 30 instructions
__declspec(naked) unsigned __fastcall nxFac1235(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x00024d50
		mov esi, ecx		// 0x00024d51
		mov ecx, dword ptr [esi + 0x10]		// 0x00024d53
		call nxFacCallTryLock		// 0x00024d56
		test al, al		// 0x00024d5b
		jne L24d87		// 0x00024d5d
		mov eax, dword ptr [nxFacImpImpinstanceFoundationSDKNxFoundation0PAV]		// 0x00024d5f
		cmp dword ptr [eax], 0		// 0x00024d64
		jne L24d6a		// 0x00024d67
		int 3		// 0x00024d69
L24d6a:
		push offset nxFacStr104760		// 0x00024d6a
		push 0		// 0x00024d6f
		push 0x10		// 0x00024d71
		push offset nxFacStr10734c		// 0x00024d73
		push 2		// 0x00024d78
		call dword ptr [nxFacImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00024d7a
		add esp, 0x14		// 0x00024d80
		pop esi		// 0x00024d83
		ret 4		// 0x00024d84
L24d87:
		mov ecx, dword ptr [esp + 8]		// 0x00024d87
		push edi		// 0x00024d8b
		mov edi, dword ptr [esi + 0x10]		// 0x00024d8c
		push ecx		// 0x00024d8f
		mov ecx, dword ptr [esi + 0x18]		// 0x00024d90
		call nxFacCallSetValue		// 0x00024d93
		mov ecx, edi		// 0x00024d98
		call nxFacCallUnlock		// 0x00024d9a
		pop edi		// 0x00024d9f
		pop esi		// 0x00024da0
		ret 4		// 0x00024da1
		}
	}
// @@ROWS-END@@
