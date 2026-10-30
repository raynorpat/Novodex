/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// TriangleMeshRows.cpp: the internal triangle mesh's rows not yet reconstructed (the oracle's TriangleMesh.cpp):
// phys_fn_001524 and 001528 (the hull subclass the mesh builds for its convex form: its constructor over 001409 and
// its scalar deleting destructor, with its table nxTMHullTable, the oracle's 0x10107724), 002077, 002178, 002184,
// 002190 to 002194, 002200, 002205, 002239, 002243 with 002245, 002247, 002255 and 002256. Naked listing
// transcriptions. The small rows they call that the candidate has only as harness models or with another calling
// convention are transcribed here as helpers (marked "helper" below; their own records are unchanged). The oracle's
// vtables are the candidate's tables by name (aliases). Compare script:
// docs/reconstruction/novodex-physics/evidence/phase4-triangle-mesh-rows-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxTMCallCIpow();
extern "C" void nxTMCallCreateEdgeList();
extern "C" void nxTMCallNxBuildSmoothNormals();
extern "C" void nxTMCallNxGetSdkAllocator();
extern "C" void nxTMCallNxHullCluster();
extern "C" void nxTMCallNxHullComputePolygons();
extern "C" void nxTMCallNxHullFreeBuffers();
extern "C" void nxTMCallNxHullShapeDestruct();
extern "C" void nxTMCallNxMTBuildTopologyRow();
extern "C" void nxTMCallNxMTReleaseRow();
extern "C" void nxTMCallNxSupportMapHullConstruct();
extern "C" void nxTMCallNxSupportMapInit();
extern "C" void nxTMCallNxSupportMapPlaneConstruct();
extern "C" void nxTMCallNxSupportMapVertexConstruct();
extern "C" void nxTMCallNxTriangleMeshSortEdgeRecords();
extern "C" void nxTMCallSort();
extern "C" void* nxTMImpImpNxComputeBounds;
extern "C" void* nxTMImpImpNxComputeSphere;
extern "C" void* nxTMImpImperrorFoundationSDKNxFoundationSANW4Nx;
extern "C" void* nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll;
#pragma comment(linker, "/alternatename:_nxTMCallCIpow=__CIpow")
#pragma comment(linker, "/alternatename:_nxTMCallCreateEdgeList=?createEdgeList@TriangleMesh@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxTMCallNxBuildSmoothNormals=_NxBuildSmoothNormals")
#pragma comment(linker, "/alternatename:_nxTMCallNxGetSdkAllocator=?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ")
#pragma comment(linker, "/alternatename:_nxTMCallNxHullCluster=?nxHullCluster@@YAXXZ")
#pragma comment(linker, "/alternatename:_nxTMCallNxHullComputePolygons=?nxHullComputePolygons@@YI_NPAVConvexHull@@@Z")
#pragma comment(linker, "/alternatename:_nxTMCallNxHullFreeBuffers=?nxHullFreeBuffers@@YAXXZ")
#pragma comment(linker, "/alternatename:_nxTMCallNxHullShapeDestruct=?nxHullShapeDestruct@@YAXXZ")
#pragma comment(linker, "/alternatename:_nxTMCallNxMTBuildTopologyRow=?nxMTBuildTopologyRow@@YI_NPAX@Z")
#pragma comment(linker, "/alternatename:_nxTMCallNxMTReleaseRow=?nxMTReleaseRow@@YIXPAX@Z")
#pragma comment(linker, "/alternatename:_nxTMCallNxSupportMapHullConstruct=?nxSupportMapHullConstruct@@YIPAUIceSupportMap@@PAU1@IPAVConvexHull@@@Z")
#pragma comment(linker, "/alternatename:_nxTMCallNxSupportMapInit=?nxSupportMapInit@@YI_NPAUIceSupportMap@@II@Z")
#pragma comment(linker, "/alternatename:_nxTMCallNxSupportMapPlaneConstruct=?nxSupportMapPlaneConstruct@@YIPAUIceSupportMap@@PAU1@IPAVConvexHull@@@Z")
#pragma comment(linker, "/alternatename:_nxTMCallNxSupportMapVertexConstruct=?nxSupportMapVertexConstruct@@YIPAUIceSupportMap@@PAU1@IPBVConvexHull@@@Z")
#pragma comment(linker, "/alternatename:_nxTMCallNxTriangleMeshSortEdgeRecords=_nxTriangleMeshSortEdgeRecords")
#pragma comment(linker, "/alternatename:_nxTMCallSort=?Sort@RadixSort@IceCore@@QAEAAV12@PBIIW4RadixHint@2@@Z")
#pragma comment(linker, "/alternatename:_nxTMImpImpNxComputeBounds=__imp__NxComputeBounds")
#pragma comment(linker, "/alternatename:_nxTMImpImpNxComputeSphere=__imp__NxComputeSphere")
#pragma comment(linker, "/alternatename:_nxTMImpImperrorFoundationSDKNxFoundationSANW4Nx=__imp_?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ")
#pragma comment(linker, "/alternatename:_nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll=__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A")
extern "C" void* nxSdkInstanceSlot;
#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")
extern "C" unsigned gNxApiReentry;
extern "C" const char* gNxApiReentryMessage;
extern "C" void* nxTMIdentityCtor;
#pragma comment(linker, "/alternatename:_nxTMIdentityCtor=_nxIceIdentityConstruct")
extern "C" void* nxTMVtHullShape;
#pragma comment(linker, "/alternatename:_nxTMVtHullShape=?nxHullShapeVtable@@3PAPAXA")
extern "C" void* nxTMVtHull;
#pragma comment(linker, "/alternatename:_nxTMVtHull=?nxHullVtable@@3PAPAXA")
extern "C" void* nxTMVtSupportBase;
#pragma comment(linker, "/alternatename:_nxTMVtSupportBase=?gIceSupportMapBaseTable@@3QBQBXB")
extern "C" void* nxTMVtSupportHull;
#pragma comment(linker, "/alternatename:_nxTMVtSupportHull=?gIceSupportMapHullTable@@3QBQBXB")
extern "C" void* nxTMVtSupportPlane;
#pragma comment(linker, "/alternatename:_nxTMVtSupportPlane=?gIceSupportMapPlaneTable@@3QBQBXB")
extern "C" void* nxTMVtSupportVertex;
#pragma comment(linker, "/alternatename:_nxTMVtSupportVertex=?gIceSupportMapVertexTable@@3QBQBXB")

unsigned __stdcall nxTM0001(unsigned a0, unsigned a1, unsigned a2, unsigned a3);
unsigned __fastcall nxTM1409(void* self, void* edxUnused);
unsigned __fastcall nxTM1413(void* self, void* edxUnused);
unsigned __fastcall nxTM1421(void* self, void* edxUnused);
unsigned __fastcall nxTM1455(void* self, void* edxUnused);
unsigned __fastcall nxTM1457(void* self, void* edxUnused);
unsigned __fastcall nxTM1524(void* self, void* edxUnused);
unsigned __fastcall nxTM1526(void* self, void* edxUnused);
unsigned __fastcall nxTM1528(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxTM1544(void* self, void* edxUnused);
unsigned __fastcall nxTM1668(void* self, void* edxUnused);
unsigned __fastcall nxTM2060(void* self, void* edxUnused);
unsigned __fastcall nxTM2069(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxTM2071(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxTM2073(void* self, void* edxUnused);
unsigned __fastcall nxTM2077(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxTM2079(void* self, void* edxUnused);
unsigned __fastcall nxTM2081(void* self, void* edxUnused);
unsigned __fastcall nxTM2178(void* self, void* edxUnused);
unsigned __fastcall nxTM2184(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxTM2190(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxTM2200(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxTM2205(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxTM2239(void* self, void* edxUnused);
unsigned __fastcall nxTM2243(void* self, void* edxUnused);
unsigned __fastcall nxTM2247(void* self, void* edxUnused);
unsigned __fastcall nxTM2255(void* self, void* edxUnused);
unsigned __fastcall nxTM2256(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxTM5155(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2);
unsigned __fastcall nxTM5157(void* self, void* edxUnused);
unsigned __fastcall nxTM5159(void* self, void* edxUnused);
// @@PROTOS-END@@
// The hull subclass's table (the oracle's 0x10107724): the scalar deleting destructor, the builder and the
// initialiser overrides (HullShape.cpp's 001417 and 001418) and the byte count (001526).
extern "C" void nxTMHullBuild();
extern "C" void nxTMHullInit();
#pragma comment(linker, "/alternatename:_nxTMHullBuild=?nxHullShapeBuild@@YAXXZ")
#pragma comment(linker, "/alternatename:_nxTMHullInit=?nxHullShapeInit@@YAXXZ")
extern "C" void* nxTMHullTable[4] = { (void*) &nxTM1528, (void*) &nxTMHullBuild, (void*) &nxTMHullInit, (void*) &nxTM1526 };


static const unsigned nxTMK101041ec_4 = 0x3f800000;
static const unsigned nxTMK101041f0_4 = 0x0;
static const unsigned long long nxTMK101084f0_8 = 0xc036000000000000ull;
static const unsigned long long nxTMK101084f8_8 = 0x4000000000000000ull;
static const char nxTMStr108458[] = "heightFieldVerticalExtent is not outside of the vertex coordinate range of the mesh along the heightFieldVerticalAxis!  HeightField mode ignored!";
static const char nxTMStr108364[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\TriangleMesh.cpp";
static const char nxTMStr1085a4[] = "TriangleMesh: mesh is too big for this algo!";

// helper: phys_fn_000001 (record unchanged) (0x00001000, 48 B), 22 instructions
__declspec(naked) unsigned __stdcall nxTM0001(unsigned a0, unsigned a1, unsigned a2, unsigned a3)
	{
	__asm
		{
		mov eax, dword ptr [esp + 0xc]		// 0x00001000
		dec eax		// 0x00001004
		js L102d		// 0x00001005
		push ebx		// 0x00001007
		mov ebx, dword ptr [esp + 0x14]		// 0x00001008
		push ebp		// 0x0000100c
		mov ebp, dword ptr [esp + 0x10]		// 0x0000100d
		push esi		// 0x00001011
		mov esi, dword ptr [esp + 0x10]		// 0x00001012
		push edi		// 0x00001016
		lea edi, [eax + 1]		// 0x00001017
		lea ebx, [ebx]		// 0x0000101a
L1020:
		mov ecx, esi		// 0x00001020
		call ebx		// 0x00001022
		add esi, ebp		// 0x00001024
		dec edi		// 0x00001026
		jne L1020		// 0x00001027
		pop edi		// 0x00001029
		pop esi		// 0x0000102a
		pop ebp		// 0x0000102b
		pop ebx		// 0x0000102c
L102d:
		ret 0x10		// 0x0000102d
		}
	}

// helper: phys_fn_001409 (record unchanged) (0x00029750, 41 B), 15 instructions
__declspec(naked) unsigned __fastcall nxTM1409(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x00029750
		mov esi, ecx		// 0x00029751
		call nxTM1455		// 0x00029753
		xor eax, eax		// 0x00029758
		mov dword ptr [esi + 0x64], eax		// 0x0002975a
		mov dword ptr [esi], offset nxTMVtHullShape		// 0x0002975d
		mov dword ptr [esi + 0x68], eax		// 0x00029763
		mov dword ptr [esi + 0x6c], eax		// 0x00029766
		mov dword ptr [esi + 0x70], eax		// 0x00029769
		mov dword ptr [esi + 0x74], eax		// 0x0002976c
		mov dword ptr [esi + 0x78], eax		// 0x0002976f
		mov dword ptr [esi + 0x7c], eax		// 0x00029772
		mov eax, esi		// 0x00029775
		pop esi		// 0x00029777
		ret		// 0x00029778
		}
	}

// helper: phys_fn_001413 (record unchanged) (0x00029920, 30 B), 14 instructions
__declspec(naked) unsigned __fastcall nxTM1413(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x00029920
		push edi		// 0x00029921
		mov esi, ecx		// 0x00029922
		call nxTM1457		// 0x00029924
		mov ecx, dword ptr [esi + 0x64]		// 0x00029929
		test ecx, ecx		// 0x0002992c
		mov edi, eax		// 0x0002992e
		je L29939		// 0x00029930
		call nxTM1668		// 0x00029932
		add edi, eax		// 0x00029937
L29939:
		mov eax, edi		// 0x00029939
		pop edi		// 0x0002993b
		pop esi		// 0x0002993c
		ret		// 0x0002993d
		}
	}

// helper: phys_fn_001421 (record unchanged) (0x00029a10, 17 B), 7 instructions
__declspec(naked) unsigned __fastcall nxTM1421(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, ecx		// 0x00029a10
		xor ecx, ecx		// 0x00029a12
		mov dword ptr [eax + 0x24], ecx		// 0x00029a14
		mov dword ptr [eax + 0x28], ecx		// 0x00029a17
		mov dword ptr [eax + 0x2c], ecx		// 0x00029a1a
		mov dword ptr [eax + 0x30], ecx		// 0x00029a1d
		ret		// 0x00029a20
		}
	}

// helper: phys_fn_001455 (record unchanged) (0x0002ace0, 65 B), 22 instructions
__declspec(naked) unsigned __fastcall nxTM1455(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, ecx		// 0x0002ace0
		xor ecx, ecx		// 0x0002ace2
		mov dword ptr [eax], offset nxTMVtHull		// 0x0002ace4
		mov dword ptr [eax + 4], ecx		// 0x0002acea
		mov dword ptr [eax + 8], ecx		// 0x0002aced
		mov dword ptr [eax + 0xc], ecx		// 0x0002acf0
		mov dword ptr [eax + 0x10], ecx		// 0x0002acf3
		mov dword ptr [eax + 0x14], ecx		// 0x0002acf6
		mov dword ptr [eax + 0x24], ecx		// 0x0002acf9
		mov dword ptr [eax + 0x28], ecx		// 0x0002acfc
		mov dword ptr [eax + 0x2c], ecx		// 0x0002acff
		mov dword ptr [eax + 0x30], ecx		// 0x0002ad02
		mov dword ptr [eax + 0x34], ecx		// 0x0002ad05
		mov dword ptr [eax + 0x38], ecx		// 0x0002ad08
		mov dword ptr [eax + 0x3c], ecx		// 0x0002ad0b
		mov dword ptr [eax + 0x40], ecx		// 0x0002ad0e
		mov dword ptr [eax + 0x44], ecx		// 0x0002ad11
		mov dword ptr [eax + 0x48], ecx		// 0x0002ad14
		mov dword ptr [eax + 0x20], ecx		// 0x0002ad17
		mov dword ptr [eax + 0x1c], ecx		// 0x0002ad1a
		mov dword ptr [eax + 0x18], ecx		// 0x0002ad1d
		ret		// 0x0002ad20
		}
	}

// helper: phys_fn_001457 (record unchanged) (0x0002ad30, 34 B), 14 instructions
__declspec(naked) unsigned __fastcall nxTM1457(void* self, void* edxUnused)
	{
	__asm
		{
		mov edx, dword ptr [ecx + 8]		// 0x0002ad30
		xor eax, eax		// 0x0002ad33
		test edx, edx		// 0x0002ad35
		je L2ad41		// 0x0002ad37
		mov eax, dword ptr [ecx + 4]		// 0x0002ad39
		lea eax, [eax + eax*2]		// 0x0002ad3c
		shl eax, 1		// 0x0002ad3f
L2ad41:
		mov edx, dword ptr [ecx + 0x10]		// 0x0002ad41
		test edx, edx		// 0x0002ad44
		je L2ad51		// 0x0002ad46
		mov ecx, dword ptr [ecx + 0xc]		// 0x0002ad48
		lea ecx, [ecx + ecx*2]		// 0x0002ad4b
		lea eax, [eax + ecx*4]		// 0x0002ad4e
L2ad51:
		ret		// 0x0002ad51
		}
	}

// phys_fn_001524 (0x0002d940, 43 B), 15 instructions
__declspec(naked) unsigned __fastcall nxTM1524(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x0002d940
		mov esi, ecx		// 0x0002d941
		call nxTM1409		// 0x0002d943
		mov dword ptr [esi], offset nxTMHullTable		// 0x0002d948
		xor eax, eax		// 0x0002d94e
		lea ecx, [esi + 0x80]		// 0x0002d950
		mov dword ptr [ecx], eax		// 0x0002d956
		mov dword ptr [ecx + 4], eax		// 0x0002d958
		mov dword ptr [ecx + 8], eax		// 0x0002d95b
		mov dword ptr [ecx + 0xc], eax		// 0x0002d95e
		mov dword ptr [ecx + 0x10], eax		// 0x0002d961
		mov dword ptr [ecx + 0x14], eax		// 0x0002d964
		mov eax, esi		// 0x0002d967
		pop esi		// 0x0002d969
		ret		// 0x0002d96a
		}
	}

// helper: phys_fn_001526 (record unchanged) (0x0002d970, 9 B), 3 instructions
__declspec(naked) unsigned __fastcall nxTM1526(void* self, void* edxUnused)
	{
	__asm
		{
		call nxTM1413		// 0x0002d970
		add eax, 0x18		// 0x0002d975
		ret		// 0x0002d978
		}
	}

// phys_fn_001528 (0x0002d980, 40 B), 14 instructions
__declspec(naked) unsigned __fastcall nxTM1528(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x0002d980
		mov esi, ecx		// 0x0002d981
		mov dword ptr [esi], offset nxTMHullTable		// 0x0002d983
		call nxTMCallNxHullShapeDestruct		// 0x0002d989
		test byte ptr [esp + 8], 1		// 0x0002d98e
		je L2d9a2		// 0x0002d993
		call nxTMCallNxGetSdkAllocator		// 0x0002d995
		mov edx, dword ptr [eax]		// 0x0002d99a
		push esi		// 0x0002d99c
		mov ecx, eax		// 0x0002d99d
		call dword ptr [edx + 0xc]		// 0x0002d99f
L2d9a2:
		mov eax, esi		// 0x0002d9a2
		pop esi		// 0x0002d9a4
		ret 4		// 0x0002d9a5
		}
	}

// helper: phys_fn_001544 (record unchanged) (0x0002dec0, 37 B), 16 instructions
__declspec(naked) unsigned __fastcall nxTM1544(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x0002dec0
		mov esi, ecx		// 0x0002dec1
		push edi		// 0x0002dec3
		mov edi, dword ptr [esi + 4]		// 0x0002dec4
		test edi, edi		// 0x0002dec7
		je L2dee2		// 0x0002dec9
		call nxTMCallNxGetSdkAllocator		// 0x0002decb
		mov edx, dword ptr [eax]		// 0x0002ded0
		add edi, -4		// 0x0002ded2
		push edi		// 0x0002ded5
		mov ecx, eax		// 0x0002ded6
		call dword ptr [edx + 0xc]		// 0x0002ded8
		mov dword ptr [esi + 4], 0		// 0x0002dedb
L2dee2:
		pop edi		// 0x0002dee2
		pop esi		// 0x0002dee3
		ret		// 0x0002dee4
		}
	}

// helper: phys_fn_001668 (record unchanged) (0x00032810, 40 B), 17 instructions
__declspec(naked) unsigned __fastcall nxTM1668(void* self, void* edxUnused)
	{
	__asm
		{
		mov edx, dword ptr [ecx + 8]		// 0x00032810
		xor eax, eax		// 0x00032813
		test edx, edx		// 0x00032815
		je L3281e		// 0x00032817
		mov eax, dword ptr [ecx]		// 0x00032819
		shl eax, 2		// 0x0003281b
L3281e:
		mov edx, dword ptr [ecx + 0xc]		// 0x0003281e
		test edx, edx		// 0x00032821
		je L3282a		// 0x00032823
		mov edx, dword ptr [ecx]		// 0x00032825
		lea eax, [eax + edx*4]		// 0x00032827
L3282a:
		mov edx, dword ptr [ecx + 0x10]		// 0x0003282a
		test edx, edx		// 0x0003282d
		je L32837		// 0x0003282f
		mov ecx, dword ptr [ecx + 4]		// 0x00032831
		lea eax, [eax + ecx*4]		// 0x00032834
L32837:
		ret		// 0x00032837
		}
	}

// helper: phys_fn_002060 (record unchanged) (0x000515d0, 112 B), 48 instructions
__declspec(naked) unsigned __fastcall nxTM2060(void* self, void* edxUnused)
	{
	__asm
		{
		push ebx		// 0x000515d0
		push esi		// 0x000515d1
		mov esi, ecx		// 0x000515d2
		mov eax, dword ptr [esi + 0x14]		// 0x000515d4
		xor ebx, ebx		// 0x000515d7
		cmp eax, ebx		// 0x000515d9
		push edi		// 0x000515db
		je L515f1		// 0x000515dc
		call nxTMCallNxGetSdkAllocator		// 0x000515de
		mov ecx, dword ptr [esi + 0x14]		// 0x000515e3
		mov edx, dword ptr [eax]		// 0x000515e6
		push ecx		// 0x000515e8
		mov ecx, eax		// 0x000515e9
		call dword ptr [edx + 0xc]		// 0x000515eb
		mov dword ptr [esi + 0x14], ebx		// 0x000515ee
L515f1:
		mov edi, dword ptr [esi + 0x10]		// 0x000515f1
		cmp edi, ebx		// 0x000515f4
		je L51608		// 0x000515f6
		call nxTMCallNxGetSdkAllocator		// 0x000515f8
		mov edx, dword ptr [eax]		// 0x000515fd
		push edi		// 0x000515ff
		mov ecx, eax		// 0x00051600
		call dword ptr [edx + 0xc]		// 0x00051602
		mov dword ptr [esi + 0x10], ebx		// 0x00051605
L51608:
		mov edi, dword ptr [esi + 4]		// 0x00051608
		cmp edi, ebx		// 0x0005160b
		je L51622		// 0x0005160d
		call nxTMCallNxGetSdkAllocator		// 0x0005160f
		mov edx, dword ptr [eax]		// 0x00051614
		add edi, -4		// 0x00051616
		push edi		// 0x00051619
		mov ecx, eax		// 0x0005161a
		call dword ptr [edx + 0xc]		// 0x0005161c
		mov dword ptr [esi + 4], ebx		// 0x0005161f
L51622:
		mov edi, dword ptr [esi + 0xc]		// 0x00051622
		cmp edi, ebx		// 0x00051625
		je L5163c		// 0x00051627
		call nxTMCallNxGetSdkAllocator		// 0x00051629
		mov edx, dword ptr [eax]		// 0x0005162e
		add edi, -4		// 0x00051630
		push edi		// 0x00051633
		mov ecx, eax		// 0x00051634
		call dword ptr [edx + 0xc]		// 0x00051636
		mov dword ptr [esi + 0xc], ebx		// 0x00051639
L5163c:
		pop edi		// 0x0005163c
		pop esi		// 0x0005163d
		pop ebx		// 0x0005163e
		ret		// 0x0005163f
		}
	}

// helper: phys_fn_002069 (record unchanged) (0x00051fa0, 38 B), 15 instructions
__declspec(naked) unsigned __fastcall nxTM2069(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x00051fa0
		push esi		// 0x00051fa4
		mov esi, ecx		// 0x00051fa5
		mov dword ptr [esi], eax		// 0x00051fa7
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00051fa9
		mov ecx, dword ptr [ecx]		// 0x00051faf
		mov edx, dword ptr [ecx]		// 0x00051fb1
		lea eax, [eax + eax*2]		// 0x00051fb3
		push 0		// 0x00051fb6
		shl eax, 2		// 0x00051fb8
		push eax		// 0x00051fbb
		call dword ptr [edx + 8]		// 0x00051fbc
		mov dword ptr [esi + 8], eax		// 0x00051fbf
		pop esi		// 0x00051fc2
		ret 4		// 0x00051fc3
		}
	}

// helper: phys_fn_002071 (record unchanged) (0x00051fd0, 39 B), 15 instructions
__declspec(naked) unsigned __fastcall nxTM2071(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x00051fd0
		push esi		// 0x00051fd4
		mov esi, ecx		// 0x00051fd5
		mov dword ptr [esi + 4], eax		// 0x00051fd7
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00051fda
		mov ecx, dword ptr [ecx]		// 0x00051fe0
		mov edx, dword ptr [ecx]		// 0x00051fe2
		lea eax, [eax + eax*2]		// 0x00051fe4
		push 0		// 0x00051fe7
		shl eax, 2		// 0x00051fe9
		push eax		// 0x00051fec
		call dword ptr [edx + 8]		// 0x00051fed
		mov dword ptr [esi + 0xc], eax		// 0x00051ff0
		pop esi		// 0x00051ff3
		ret 4		// 0x00051ff4
		}
	}

// helper: phys_fn_002073 (record unchanged) (0x00052000, 35 B), 17 instructions
__declspec(naked) unsigned __fastcall nxTM2073(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x00052000
		mov esi, ecx		// 0x00052001
		mov eax, dword ptr [esi + 4]		// 0x00052003
		test eax, eax		// 0x00052006
		jne L5200c		// 0x00052008
		pop esi		// 0x0005200a
		ret		// 0x0005200b
L5200c:
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005200c
		mov ecx, dword ptr [ecx]		// 0x00052012
		mov edx, dword ptr [ecx]		// 0x00052014
		push 0		// 0x00052016
		add eax, eax		// 0x00052018
		push eax		// 0x0005201a
		call dword ptr [edx + 8]		// 0x0005201b
		mov dword ptr [esi + 0x10], eax		// 0x0005201e
		pop esi		// 0x00052021
		ret		// 0x00052022
		}
	}

// phys_fn_002077 (0x00052060, 331 B), 133 instructions
__declspec(naked) unsigned __fastcall nxTM2077(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ecx		// 0x00052060
		push esi		// 0x00052061
		mov esi, ecx		// 0x00052062
		mov eax, dword ptr [esi + 4]		// 0x00052064
		test eax, eax		// 0x00052067
		je L521a6		// 0x00052069
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005206f
		mov ecx, dword ptr [ecx]		// 0x00052075
		mov edx, dword ptr [ecx]		// 0x00052077
		push ebx		// 0x00052079
		push ebp		// 0x0005207a
		push edi		// 0x0005207b
		lea eax, [eax + eax*2]		// 0x0005207c
		push 0		// 0x0005207f
		shl eax, 2		// 0x00052081
		push eax		// 0x00052084
		call dword ptr [edx + 8]		// 0x00052085
		mov ecx, dword ptr [esi + 4]		// 0x00052088
		mov edi, dword ptr [esp + 0x18]		// 0x0005208b
		mov ebx, eax		// 0x0005208f
		xor eax, eax		// 0x00052091
		test ecx, ecx		// 0x00052093
		mov dword ptr [esp + 0x10], ebx		// 0x00052095
		jbe L520cd		// 0x00052099
		mov edx, ebx		// 0x0005209b
		lea ecx, [ecx]		// 0x0005209d
L520a0:
		mov ecx, dword ptr [edi + eax*4]		// 0x000520a0
		mov ebx, dword ptr [esi + 0xc]		// 0x000520a3
		lea ecx, [ecx + ecx*2]		// 0x000520a6
		lea ecx, [ebx + ecx*4]		// 0x000520a9
		mov ebp, dword ptr [ecx]		// 0x000520ac
		mov ebx, edx		// 0x000520ae
		mov dword ptr [ebx], ebp		// 0x000520b0
		mov ebp, dword ptr [ecx + 4]		// 0x000520b2
		mov dword ptr [ebx + 4], ebp		// 0x000520b5
		mov ecx, dword ptr [ecx + 8]		// 0x000520b8
		mov dword ptr [ebx + 8], ecx		// 0x000520bb
		mov ecx, dword ptr [esi + 4]		// 0x000520be
		inc eax		// 0x000520c1
		add edx, 0xc		// 0x000520c2
		cmp eax, ecx		// 0x000520c5
		jb L520a0		// 0x000520c7
		mov ebx, dword ptr [esp + 0x10]		// 0x000520c9
L520cd:
		mov eax, dword ptr [esi + 0xc]		// 0x000520cd
		xor ebp, ebp		// 0x000520d0
		cmp eax, ebp		// 0x000520d2
		je L520e7		// 0x000520d4
		mov edx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000520d6
		mov ecx, dword ptr [edx]		// 0x000520dc
		mov edx, dword ptr [ecx]		// 0x000520de
		push eax		// 0x000520e0
		call dword ptr [edx + 0x14]		// 0x000520e1
		mov dword ptr [esi + 0xc], ebp		// 0x000520e4
L520e7:
		cmp dword ptr [esi + 0x10], ebp		// 0x000520e7
		mov dword ptr [esi + 0xc], ebx		// 0x000520ea
		je L52141		// 0x000520ed
		mov eax, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000520ef
		mov ecx, dword ptr [eax]		// 0x000520f4
		mov eax, dword ptr [esi + 4]		// 0x000520f6
		mov edx, dword ptr [ecx]		// 0x000520f9
		push ebp		// 0x000520fb
		shl eax, 1		// 0x000520fc
		push eax		// 0x000520fe
		call dword ptr [edx + 8]		// 0x000520ff
		mov ecx, dword ptr [esi + 4]		// 0x00052102
		mov ebx, eax		// 0x00052105
		xor eax, eax		// 0x00052107
		cmp ecx, ebp		// 0x00052109
		jbe L52126		// 0x0005210b
		lea ecx, [ecx]		// 0x0005210d
L52110:
		mov ecx, dword ptr [edi + eax*4]		// 0x00052110
		mov edx, dword ptr [esi + 0x10]		// 0x00052113
		mov cx, word ptr [edx + ecx*2]		// 0x00052116
		mov word ptr [ebx + eax*2], cx		// 0x0005211a
		mov ecx, dword ptr [esi + 4]		// 0x0005211e
		inc eax		// 0x00052121
		cmp eax, ecx		// 0x00052122
		jb L52110		// 0x00052124
L52126:
		mov eax, dword ptr [esi + 0x10]		// 0x00052126
		cmp eax, ebp		// 0x00052129
		je L5213e		// 0x0005212b
		mov edx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005212d
		mov ecx, dword ptr [edx]		// 0x00052133
		mov edx, dword ptr [ecx]		// 0x00052135
		push eax		// 0x00052137
		call dword ptr [edx + 0x14]		// 0x00052138
		mov dword ptr [esi + 0x10], ebp		// 0x0005213b
L5213e:
		mov dword ptr [esi + 0x10], ebx		// 0x0005213e
L52141:
		mov eax, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00052141
		mov ecx, dword ptr [eax]		// 0x00052146
		mov eax, dword ptr [esi + 4]		// 0x00052148
		mov edx, dword ptr [ecx]		// 0x0005214b
		push ebp		// 0x0005214d
		shl eax, 2		// 0x0005214e
		push eax		// 0x00052151
		call dword ptr [edx + 8]		// 0x00052152
		mov ebp, eax		// 0x00052155
		mov eax, dword ptr [esi + 4]		// 0x00052157
		xor edx, edx		// 0x0005215a
		test eax, eax		// 0x0005215c
		jbe L52184		// 0x0005215e
		mov ebx, ebp		// 0x00052160
		mov eax, edi		// 0x00052162
		sub ebx, edi		// 0x00052164
L52166:
		mov ecx, dword ptr [esi + 0x14]		// 0x00052166
		test ecx, ecx		// 0x00052169
		je L52174		// 0x0005216b
		mov edi, dword ptr [eax]		// 0x0005216d
		mov ecx, dword ptr [ecx + edi*4]		// 0x0005216f
		jmp L52176		// 0x00052172
L52174:
		mov ecx, dword ptr [eax]		// 0x00052174
L52176:
		mov dword ptr [ebx + eax], ecx		// 0x00052176
		mov ecx, dword ptr [esi + 4]		// 0x00052179
		inc edx		// 0x0005217c
		add eax, 4		// 0x0005217d
		cmp edx, ecx		// 0x00052180
		jb L52166		// 0x00052182
L52184:
		mov eax, dword ptr [esi + 0x14]		// 0x00052184
		test eax, eax		// 0x00052187
		je L521a0		// 0x00052189
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0005218b
		mov ecx, dword ptr [ecx]		// 0x00052191
		mov edx, dword ptr [ecx]		// 0x00052193
		push eax		// 0x00052195
		call dword ptr [edx + 0x14]		// 0x00052196
		mov dword ptr [esi + 0x14], 0		// 0x00052199
L521a0:
		pop edi		// 0x000521a0
		mov dword ptr [esi + 0x14], ebp		// 0x000521a1
		pop ebp		// 0x000521a4
		pop ebx		// 0x000521a5
L521a6:
		pop esi		// 0x000521a6
		pop ecx		// 0x000521a7
		ret 4		// 0x000521a8
		}
	}

// helper: phys_fn_002079 (record unchanged) (0x000521b0, 133 B), 58 instructions
__declspec(naked) unsigned __fastcall nxTM2079(void* self, void* edxUnused)
	{
	__asm
		{
		push ebp		// 0x000521b0
		push esi		// 0x000521b1
		mov esi, ecx		// 0x000521b2
		mov eax, dword ptr [esi + 0x1c]		// 0x000521b4
		xor ebp, ebp		// 0x000521b7
		cmp eax, ebp		// 0x000521b9
		je L521ce		// 0x000521bb
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000521bd
		mov ecx, dword ptr [ecx]		// 0x000521c3
		mov edx, dword ptr [ecx]		// 0x000521c5
		push eax		// 0x000521c7
		call dword ptr [edx + 0x14]		// 0x000521c8
		mov dword ptr [esi + 0x1c], ebp		// 0x000521cb
L521ce:
		mov eax, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000521ce
		mov ecx, dword ptr [eax]		// 0x000521d3
		mov eax, dword ptr [esi + 4]		// 0x000521d5
		mov edx, dword ptr [ecx]		// 0x000521d8
		push ebp		// 0x000521da
		shl eax, 4		// 0x000521db
		push eax		// 0x000521de
		call dword ptr [edx + 8]		// 0x000521df
		mov dword ptr [esi + 0x1c], eax		// 0x000521e2
		cmp dword ptr [esi + 4], ebp		// 0x000521e5
		jbe L52230		// 0x000521e8
		push ebx		// 0x000521ea
		push edi		// 0x000521eb
		xor ebx, ebx		// 0x000521ec
		xor edi, edi		// 0x000521ee
L521f0:
		mov eax, dword ptr [esi + 0xc]		// 0x000521f0
		mov edx, dword ptr [eax + edi + 8]		// 0x000521f3
		mov ecx, dword ptr [esi + 8]		// 0x000521f7
		add eax, edi		// 0x000521fa
		lea edx, [edx + edx*2]		// 0x000521fc
		lea edx, [ecx + edx*4]		// 0x000521ff
		push edx		// 0x00052202
		mov edx, dword ptr [eax + 4]		// 0x00052203
		mov eax, dword ptr [eax]		// 0x00052206
		lea edx, [edx + edx*2]		// 0x00052208
		lea edx, [ecx + edx*4]		// 0x0005220b
		lea eax, [eax + eax*2]		// 0x0005220e
		lea ecx, [ecx + eax*4]		// 0x00052211
		push edx		// 0x00052214
		push ecx		// 0x00052215
		mov ecx, dword ptr [esi + 0x1c]		// 0x00052216
		add ecx, ebx		// 0x00052219
		call nxTM5155		// 0x0005221b
		mov eax, dword ptr [esi + 4]		// 0x00052220
		inc ebp		// 0x00052223
		add edi, 0xc		// 0x00052224
		add ebx, 0x10		// 0x00052227
		cmp ebp, eax		// 0x0005222a
		jb L521f0		// 0x0005222c
		pop edi		// 0x0005222e
		pop ebx		// 0x0005222f
L52230:
		pop esi		// 0x00052230
		mov al, 1		// 0x00052231
		pop ebp		// 0x00052233
		ret		// 0x00052234
		}
	}

// helper: phys_fn_002081 (record unchanged) (0x00052240, 59 B), 27 instructions
__declspec(naked) unsigned __fastcall nxTM2081(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00052240
		push esi		// 0x00052245
		mov esi, ecx		// 0x00052246
		mov ecx, dword ptr [eax]		// 0x00052248
		mov eax, dword ptr [esi]		// 0x0005224a
		mov edx, dword ptr [ecx]		// 0x0005224c
		lea eax, [eax + eax*2]		// 0x0005224e
		push 0		// 0x00052251
		shl eax, 2		// 0x00052253
		push eax		// 0x00052256
		call dword ptr [edx + 8]		// 0x00052257
		mov ecx, dword ptr [esi + 0xc]		// 0x0005225a
		mov edx, dword ptr [esi + 8]		// 0x0005225d
		push 1		// 0x00052260
		push eax		// 0x00052262
		push 0		// 0x00052263
		push ecx		// 0x00052265
		mov ecx, dword ptr [esi + 4]		// 0x00052266
		mov dword ptr [esi + 0x18], eax		// 0x00052269
		mov eax, dword ptr [esi]		// 0x0005226c
		push edx		// 0x0005226e
		push eax		// 0x0005226f
		push ecx		// 0x00052270
		call nxTMCallNxBuildSmoothNormals		// 0x00052271
		add esp, 0x1c		// 0x00052276
		pop esi		// 0x00052279
		ret		// 0x0005227a
		}
	}

// phys_fn_002178 (0x00053f40, 511 B), 154 instructions
__declspec(naked) unsigned __fastcall nxTM2178(void* self, void* edxUnused)
	{
	__asm
		{
		sub esp, 0x18		// 0x00053f40
		push esi		// 0x00053f43
		mov esi, ecx		// 0x00053f44
		mov eax, dword ptr [esi + 0xa0]		// 0x00053f46
		test eax, eax		// 0x00053f4c
		je L53f64		// 0x00053f4e
		mov ecx, dword ptr [eax + 0x10]		// 0x00053f50
		mov edx, dword ptr [eax + 0xc]		// 0x00053f53
		push ecx		// 0x00053f56
		push edx		// 0x00053f57
		lea eax, [esp + 0xc]		// 0x00053f58
		push eax		// 0x00053f5c
		lea ecx, [esp + 0x1c]		// 0x00053f5d
		push ecx		// 0x00053f61
		jmp L53f76		// 0x00053f62
L53f64:
		mov edx, dword ptr [esi + 0x10]		// 0x00053f64
		mov eax, dword ptr [esi + 8]		// 0x00053f67
		push edx		// 0x00053f6a
		push eax		// 0x00053f6b
		lea ecx, [esp + 0xc]		// 0x00053f6c
		push ecx		// 0x00053f70
		lea edx, [esp + 0x1c]		// 0x00053f71
		push edx		// 0x00053f75
L53f76:
		call dword ptr [nxTMImpImpNxComputeBounds]		// 0x00053f76
		fld qword ptr [nxTMK101084f8_8]		// 0x00053f7c
		fld qword ptr [nxTMK101084f0_8]		// 0x00053f82
		add esp, 0x10		// 0x00053f88
		call nxTMCallCIpow		// 0x00053f8b
		fld dword ptr [esp + 4]		// 0x00053f90
		fcomp dword ptr [esp + 8]		// 0x00053f94
		mov eax, dword ptr [esp + 4]		// 0x00053f98
		mov dword ptr [esi + 0x70], eax		// 0x00053f9c
		fnstsw ax		// 0x00053f9f
		test ah, 0x41		// 0x00053fa1
		jne L53fac		// 0x00053fa4
		fld dword ptr [esp + 4]		// 0x00053fa6
		jmp L53fb0		// 0x00053faa
L53fac:
		fld dword ptr [esp + 8]		// 0x00053fac
L53fb0:
		fcom dword ptr [esp + 0xc]		// 0x00053fb0
		fst dword ptr [esi + 0x70]		// 0x00053fb4
		fnstsw ax		// 0x00053fb7
		test ah, 0x41		// 0x00053fb9
		je L53fc4		// 0x00053fbc
		fstp st(0)		// 0x00053fbe
		fld dword ptr [esp + 0xc]		// 0x00053fc0
L53fc4:
		mov eax, dword ptr [esi + 0x7c]		// 0x00053fc4
		fmul st(0), st(1)		// 0x00053fc7
		cmp eax, 0xff		// 0x00053fc9
		fstp dword ptr [esi + 0x70]		// 0x00053fce
		fstp st(0)		// 0x00053fd1
		je L54066		// 0x00053fd3
		test eax, eax		// 0x00053fd9
		jne L54008		// 0x00053fdb
		fld dword ptr [esi + 0x80]		// 0x00053fdd
		fcomp dword ptr [nxTMK101041f0_4]		// 0x00053fe3
		fnstsw ax		// 0x00053fe9
		test ah, 5		// 0x00053feb
		jp L53ffc		// 0x00053fee
		mov ecx, dword ptr [esi + 0x80]		// 0x00053ff0
		mov dword ptr [esp + 0x10], ecx		// 0x00053ff6
		jmp L54066		// 0x00053ffa
L53ffc:
		mov edx, dword ptr [esi + 0x80]		// 0x00053ffc
		mov dword ptr [esp + 4], edx		// 0x00054002
		jmp L54066		// 0x00054006
L54008:
		cmp eax, 1		// 0x00054008
		jne L54038		// 0x0005400b
		fld dword ptr [esi + 0x80]		// 0x0005400d
		fcomp dword ptr [nxTMK101041f0_4]		// 0x00054013
		fnstsw ax		// 0x00054019
		test ah, 5		// 0x0005401b
		jp L5402c		// 0x0005401e
		mov eax, dword ptr [esi + 0x80]		// 0x00054020
		mov dword ptr [esp + 0x14], eax		// 0x00054026
		jmp L54066		// 0x0005402a
L5402c:
		mov ecx, dword ptr [esi + 0x80]		// 0x0005402c
		mov dword ptr [esp + 8], ecx		// 0x00054032
		jmp L54066		// 0x00054036
L54038:
		cmp eax, 2		// 0x00054038
		jne L54066		// 0x0005403b
		fld dword ptr [esi + 0x80]		// 0x0005403d
		fcomp dword ptr [nxTMK101041f0_4]		// 0x00054043
		fnstsw ax		// 0x00054049
		test ah, 5		// 0x0005404b
		jp L5405c		// 0x0005404e
		mov edx, dword ptr [esi + 0x80]		// 0x00054050
		mov dword ptr [esp + 0x18], edx		// 0x00054056
		jmp L54066		// 0x0005405a
L5405c:
		mov eax, dword ptr [esi + 0x80]		// 0x0005405c
		mov dword ptr [esp + 0xc], eax		// 0x00054062
L54066:
		mov ecx, dword ptr [esp + 0x10]		// 0x00054066
		mov edx, dword ptr [esp + 0x14]		// 0x0005406a
		mov eax, dword ptr [esp + 0x18]		// 0x0005406e
		mov dword ptr [esi + 0x44], ecx		// 0x00054072
		mov ecx, dword ptr [esp + 4]		// 0x00054075
		mov dword ptr [esi + 0x48], edx		// 0x00054079
		mov edx, dword ptr [esp + 8]		// 0x0005407c
		mov dword ptr [esi + 0x4c], eax		// 0x00054080
		mov eax, dword ptr [esp + 0xc]		// 0x00054083
		mov dword ptr [esi + 0x50], ecx		// 0x00054087
		mov dword ptr [esi + 0x54], edx		// 0x0005408a
		mov dword ptr [esi + 0x58], eax		// 0x0005408d
		mov ecx, dword ptr [esi + 0x7c]		// 0x00054090
		cmp ecx, 0xff		// 0x00054093
		je L54101		// 0x00054099
		fld dword ptr [esp + ecx*4 + 0x10]		// 0x0005409b
		fcomp dword ptr [esi + 0x80]		// 0x0005409f
		fnstsw ax		// 0x000540a5
		test ah, 1		// 0x000540a7
		jne L540b1		// 0x000540aa
		mov dword ptr [esi + 0x78], ecx		// 0x000540ac
		jmp L54101		// 0x000540af
L540b1:
		fld dword ptr [esp + ecx*4 + 4]		// 0x000540b1
		fcomp dword ptr [esi + 0x80]		// 0x000540b5
		fnstsw ax		// 0x000540bb
		test ah, 0x41		// 0x000540bd
		jp L540ca		// 0x000540c0
		or ecx, 8		// 0x000540c2
		mov dword ptr [esi + 0x78], ecx		// 0x000540c5
		jmp L54101		// 0x000540c8
L540ca:
		push offset nxTMStr108458		// 0x000540ca
		push 0		// 0x000540cf
		push 0x382		// 0x000540d1
		push offset nxTMStr108364		// 0x000540d6
		push 0xce		// 0x000540db
		call dword ptr [nxTMImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x000540e0
		add esp, 0x14		// 0x000540e6
		mov dword ptr [esi + 0x7c], 0xff		// 0x000540e9
		mov dword ptr [esi + 0x80], 0		// 0x000540f0
		mov dword ptr [esi + 0x78], 0xff		// 0x000540fa
L54101:
		mov eax, dword ptr [esi + 0xa0]		// 0x00054101
		test eax, eax		// 0x00054107
		je L54125		// 0x00054109
		mov ecx, dword ptr [eax + 0x10]		// 0x0005410b
		mov edx, dword ptr [eax + 0xc]		// 0x0005410e
		push ecx		// 0x00054111
		push edx		// 0x00054112
		add esi, 0x5c		// 0x00054113
		push esi		// 0x00054116
		call dword ptr [nxTMImpImpNxComputeSphere]		// 0x00054117
		add esp, 0xc		// 0x0005411d
		pop esi		// 0x00054120
		add esp, 0x18		// 0x00054121
		ret		// 0x00054124
L54125:
		mov eax, dword ptr [esi + 0x10]		// 0x00054125
		mov ecx, dword ptr [esi + 8]		// 0x00054128
		push eax		// 0x0005412b
		push ecx		// 0x0005412c
		add esi, 0x5c		// 0x0005412d
		push esi		// 0x00054130
		call dword ptr [nxTMImpImpNxComputeSphere]		// 0x00054131
		add esp, 0xc		// 0x00054137
		pop esi		// 0x0005413a
		add esp, 0x18		// 0x0005413b
		ret		// 0x0005413e
		}
	}

// phys_fn_002184 (0x00054270, 337 B), 122 instructions
__declspec(naked) unsigned __fastcall nxTM2184(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		mov eax, dword ptr [ecx + 0xc]		// 0x00054270
		sub esp, 0xc		// 0x00054273
		cmp eax, -1		// 0x00054276
		je L543bb		// 0x00054279
		mov eax, dword ptr [ecx + 8]		// 0x0005427f
		push ebx		// 0x00054282
		push ebp		// 0x00054283
		mov ebp, dword ptr [esp + 0x18]		// 0x00054284
		mov edx, dword ptr [ebp + 0x3c]		// 0x00054288
		push esi		// 0x0005428b
		mov esi, eax		// 0x0005428c
		shr eax, 0x1e		// 0x0005428e
		mov dword ptr [esp + 0xc], eax		// 0x00054291
		mov eax, dword ptr [ecx + 0xc]		// 0x00054295
		push edi		// 0x00054298
		mov edi, eax		// 0x00054299
		shr eax, 0x1e		// 0x0005429b
		mov ecx, eax		// 0x0005429e
		and edi, 0x3fffffff		// 0x000542a0
		mov eax, edi		// 0x000542a6
		shl eax, 4		// 0x000542a8
		mov bl, 1		// 0x000542ab
		shl bl, cl		// 0x000542ad
		lea edx, [edx + eax + 0xc]		// 0x000542af
		mov dword ptr [esp + 0x14], ecx		// 0x000542b3
		or byte ptr [edx], bl		// 0x000542b7
		mov edx, dword ptr [ebp + 0x3c]		// 0x000542b9
		fld dword ptr [edx + eax + 8]		// 0x000542bc
		and esi, 0x3fffffff		// 0x000542c0
		shl esi, 4		// 0x000542c6
		fmul dword ptr [edx + esi + 8]		// 0x000542c9
		lea ecx, [edx + esi + 8]		// 0x000542cd
		fld dword ptr [edx + eax + 4]		// 0x000542d1
		lea ebx, [edx + esi + 4]		// 0x000542d5
		fmul dword ptr [ebx]		// 0x000542d9
		mov dword ptr [esp + 0x20], ecx		// 0x000542db
		mov dword ptr [esp + 0x18], ebx		// 0x000542df
		faddp st(1), st(0)		// 0x000542e3
		fld dword ptr [edx + esi]		// 0x000542e5
		fmul dword ptr [edx + eax]		// 0x000542e8
		faddp st(1), st(0)		// 0x000542eb
		fld dword ptr [nxTMK101041ec_4]		// 0x000542ed
		fsub dword ptr [ebp + 0x6c]		// 0x000542f3
		fcompp		// 0x000542f6
		fnstsw ax		// 0x000542f8
		test ah, 5		// 0x000542fa
		jp L5431d		// 0x000542fd
		mov ecx, dword ptr [esp + 0x10]		// 0x000542ff
		mov eax, edx		// 0x00054303
		mov dl, 1		// 0x00054305
		lea eax, [eax + esi + 0xc]		// 0x00054307
		shl dl, cl		// 0x0005430b
		mov cl, byte ptr [eax]		// 0x0005430d
		pop edi		// 0x0005430f
		pop esi		// 0x00054310
		or cl, dl		// 0x00054311
		pop ebp		// 0x00054313
		mov byte ptr [eax], cl		// 0x00054314
		pop ebx		// 0x00054316
		add esp, 0xc		// 0x00054317
		ret 4		// 0x0005431a
L5431d:
		mov ecx, dword ptr [ebp + 0x14]		// 0x0005431d
		lea eax, [edi + edi*2]		// 0x00054320
		lea edi, [ecx + eax*4]		// 0x00054323
		mov eax, dword ptr [esp + 0x14]		// 0x00054326
		sub eax, 0		// 0x0005432a
		je L54347		// 0x0005432d
		dec eax		// 0x0005432f
		je L5433e		// 0x00054330
		dec eax		// 0x00054332
		jne L54350		// 0x00054333
		mov eax, 1		// 0x00054335
		xor ebx, ebx		// 0x0005433a
		jmp L54358		// 0x0005433c
L5433e:
		xor eax, eax		// 0x0005433e
		mov ebx, 1		// 0x00054340
		jmp L54358		// 0x00054345
L54347:
		xor eax, eax		// 0x00054347
		mov ebx, 2		// 0x00054349
		jmp L54358		// 0x0005434e
L54350:
		mov eax, dword ptr [esp + 0x20]		// 0x00054350
		mov ebx, dword ptr [esp + 0x20]		// 0x00054354
L54358:
		mov eax, dword ptr [edi + eax*4]		// 0x00054358
		mov ecx, dword ptr [ebp + 0x10]		// 0x0005435b
		mov edi, dword ptr [edi + ebx*4]		// 0x0005435e
		lea eax, [eax + eax*2]		// 0x00054361
		lea eax, [ecx + eax*4]		// 0x00054364
		lea edi, [edi + edi*2]		// 0x00054367
		fld dword ptr [ecx + edi*4]		// 0x0005436a
		lea ecx, [ecx + edi*4]		// 0x0005436d
		fsub dword ptr [eax]		// 0x00054370
		fld dword ptr [ecx + 4]		// 0x00054372
		fsub dword ptr [eax + 4]		// 0x00054375
		fld dword ptr [ecx + 8]		// 0x00054378
		mov ecx, dword ptr [esp + 0x20]		// 0x0005437b
		fsub dword ptr [eax + 8]		// 0x0005437f
		fxch st(2)		// 0x00054382
		fmul dword ptr [edx + esi]		// 0x00054384
		mov edx, dword ptr [esp + 0x18]		// 0x00054387
		fxch st(2)		// 0x0005438b
		fmul dword ptr [ecx]		// 0x0005438d
		faddp st(2), st(0)		// 0x0005438f
		fmul dword ptr [edx]		// 0x00054391
		faddp st(1), st(0)		// 0x00054393
		fcomp dword ptr [nxTMK101041f0_4]		// 0x00054395
		fnstsw ax		// 0x0005439b
		test ah, 0x41		// 0x0005439d
		jne L543b7		// 0x000543a0
		mov eax, dword ptr [ebp + 0x3c]		// 0x000543a2
		mov ecx, dword ptr [esp + 0x10]		// 0x000543a5
		mov dl, 1		// 0x000543a9
		lea esi, [eax + esi + 0xc]		// 0x000543ab
		mov al, byte ptr [esi]		// 0x000543af
		shl dl, cl		// 0x000543b1
		or al, dl		// 0x000543b3
		mov byte ptr [esi], al		// 0x000543b5
L543b7:
		pop edi		// 0x000543b7
		pop esi		// 0x000543b8
		pop ebp		// 0x000543b9
		pop ebx		// 0x000543ba
L543bb:
		add esp, 0xc		// 0x000543bb
		ret 4		// 0x000543be
		}
	}

// phys_fn_002190 and phys_fn_002192 and phys_fn_002194 (one function) (0x00054500, 281 B), 96 instructions
__declspec(naked) unsigned __fastcall nxTM2190(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push ebx		// 0x00054500
		mov ebx, dword ptr [esp + 8]		// 0x00054501
		push ebp		// 0x00054505
		push esi		// 0x00054506
		push edi		// 0x00054507
		mov esi, ecx		// 0x00054508
		push ebx		// 0x0005450a
		lea ecx, [esi + 8]		// 0x0005450b
		call nxTM2077		// 0x0005450e
		mov eax, dword ptr [esi + 0x94]		// 0x00054513
		test eax, eax		// 0x00054519
		je L54596		// 0x0005451b
		mov eax, dword ptr [esi + 0xc]		// 0x0005451d
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00054520
		mov ecx, dword ptr [ecx]		// 0x00054526
		mov edx, dword ptr [ecx]		// 0x00054528
		push 0		// 0x0005452a
		shl eax, 2		// 0x0005452c
		push eax		// 0x0005452f
		call dword ptr [edx + 8]		// 0x00054530
		mov edi, eax		// 0x00054533
		mov eax, dword ptr [esi + 0xc]		// 0x00054535
		xor ecx, ecx		// 0x00054538
		test eax, eax		// 0x0005453a
		jbe L5456e		// 0x0005453c
		mov edx, ebx		// 0x0005453e
		mov eax, edi		// 0x00054540
		sub edx, edi		// 0x00054542
		jmp L54550		// 0x00054544
		lea esp, [esp]		// 0x00054546
		lea ecx, [ecx]		// 0x0005454d
L54550:
		mov ebx, dword ptr [edx + eax]		// 0x00054550
		mov ebp, dword ptr [esi + 0x94]		// 0x00054553
		mov ebx, dword ptr [ebp + ebx*4]		// 0x00054559
		mov dword ptr [eax], ebx		// 0x0005455d
		mov ebx, dword ptr [esi + 0xc]		// 0x0005455f
		inc ecx		// 0x00054562
		add eax, 4		// 0x00054563
		cmp ecx, ebx		// 0x00054566
		jb L54550		// 0x00054568
		mov ebx, dword ptr [esp + 0x14]		// 0x0005456a
L5456e:
		mov eax, dword ptr [esi + 0x94]		// 0x0005456e
		test eax, eax		// 0x00054574
		je L54590		// 0x00054576
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00054578
		mov ecx, dword ptr [ecx]		// 0x0005457e
		mov edx, dword ptr [ecx]		// 0x00054580
		push eax		// 0x00054582
		call dword ptr [edx + 0x14]		// 0x00054583
		mov dword ptr [esi + 0x94], 0		// 0x00054586
L54590:
		mov dword ptr [esi + 0x94], edi		// 0x00054590
L54596:
		mov eax, dword ptr [esi + 0x98]		// 0x00054596
		test eax, eax		// 0x0005459c
		je L54612		// 0x0005459e
		mov eax, dword ptr [esi + 0xc]		// 0x000545a0
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000545a3
		mov ecx, dword ptr [ecx]		// 0x000545a9
		mov edx, dword ptr [ecx]		// 0x000545ab
		push 0		// 0x000545ad
		shl eax, 2		// 0x000545af
		push eax		// 0x000545b2
		call dword ptr [edx + 8]		// 0x000545b3
		mov edi, eax		// 0x000545b6
		mov eax, dword ptr [esi + 0xc]		// 0x000545b8
		xor ecx, ecx		// 0x000545bb
		test eax, eax		// 0x000545bd
		jbe L545ea		// 0x000545bf
		mov edx, ebx		// 0x000545c1
		mov eax, edi		// 0x000545c3
		sub edx, edi		// 0x000545c5
		jmp L545d0		// 0x000545c7
		lea esp, [esp]		// 0x000545c9
L545d0:
		mov ebx, dword ptr [edx + eax]		// 0x000545d0
		mov ebp, dword ptr [esi + 0x98]		// 0x000545d3
		mov ebx, dword ptr [ebp + ebx*4]		// 0x000545d9
		mov dword ptr [eax], ebx		// 0x000545dd
		mov ebx, dword ptr [esi + 0xc]		// 0x000545df
		inc ecx		// 0x000545e2
		add eax, 4		// 0x000545e3
		cmp ecx, ebx		// 0x000545e6
		jb L545d0		// 0x000545e8
L545ea:
		mov eax, dword ptr [esi + 0x98]		// 0x000545ea
		test eax, eax		// 0x000545f0
		je L5460c		// 0x000545f2
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000545f4
		mov ecx, dword ptr [ecx]		// 0x000545fa
		mov edx, dword ptr [ecx]		// 0x000545fc
		push eax		// 0x000545fe
		call dword ptr [edx + 0x14]		// 0x000545ff
		mov dword ptr [esi + 0x98], 0		// 0x00054602
L5460c:
		mov dword ptr [esi + 0x98], edi		// 0x0005460c
L54612:
		pop edi		// 0x00054612
		pop esi		// 0x00054613
		pop ebp		// 0x00054614
		pop ebx		// 0x00054615
		ret 4		// 0x00054616
		}
	}

// phys_fn_002200 (0x00054640, 89 B), 31 instructions
__declspec(naked) unsigned __fastcall nxTM2200(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x00054640
		test eax, eax		// 0x00054644
		jne L5466e		// 0x00054646
		mov eax, dword ptr [esp + 8]		// 0x00054648
		cmp eax, 4		// 0x0005464c
		ja L5466e		// 0x0005464f
		cmp eax, 0		// jump table 0x0005469c[0]
		je L54658
		cmp eax, 1		// jump table 0x0005469c[1]
		je L5465e
		cmp eax, 2		// jump table 0x0005469c[2]
		je L5465e
		cmp eax, 3		// jump table 0x0005469c[3]
		je L54664
		jmp L54673		// jump table 0x0005469c[4]		// 0x00054651
L54658:
		mov eax, dword ptr [ecx + 0xc]		// 0x00054658
		ret 8		// 0x0005465b
L5465e:
		mov eax, dword ptr [ecx + 8]		// 0x0005465e
		ret 8		// 0x00054661
L54664:
		mov ecx, dword ptr [ecx + 0xa0]		// 0x00054664
		test ecx, ecx		// 0x0005466a
		jne L54658		// 0x0005466c
L5466e:
		xor eax, eax		// 0x0005466e
		ret 8		// 0x00054670
L54673:
		push esi		// 0x00054673
		mov esi, dword ptr [ecx + 0xa0]		// 0x00054674
		test esi, esi		// 0x0005467a
		jne L54684		// 0x0005467c
		xor eax, eax		// 0x0005467e
		pop esi		// 0x00054680
		ret 8		// 0x00054681
L54684:
		mov eax, dword ptr [esi + 0x24]		// 0x00054684
		test eax, eax		// 0x00054687
		jne L54692		// 0x00054689
		mov ecx, esi		// 0x0005468b
		call nxTMCallNxHullComputePolygons		// 0x0005468d
L54692:
		mov eax, dword ptr [esi + 0x24]		// 0x00054692
		pop esi		// 0x00054695
		ret 8		// 0x00054696
		}
	}

// phys_fn_002205 (0x00054720, 115 B), 41 instructions
__declspec(naked) unsigned __fastcall nxTM2205(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]		// 0x00054720
		test eax, eax		// 0x00054724
		push esi		// 0x00054726
		mov esi, ecx		// 0x00054727
		jne L5478d		// 0x00054729
		mov eax, dword ptr [esp + 0xc]		// 0x0005472b
		cmp eax, 4		// 0x0005472f
		ja L5478d		// 0x00054732
		cmp eax, 0		// jump table 0x00054794[0]
		je L5473b
		cmp eax, 1		// jump table 0x00054794[1]
		je L54764
		cmp eax, 2		// jump table 0x00054794[2]
		je L54742
		cmp eax, 3		// jump table 0x00054794[3]
		je L5475a
		jmp L5476b		// jump table 0x00054794[4]		// 0x00054734
L5473b:
		mov eax, dword ptr [esi + 0x14]		// 0x0005473b
		pop esi		// 0x0005473e
		ret 8		// 0x0005473f
L54742:
		mov eax, dword ptr [esi + 0x20]		// 0x00054742
		add esi, 8		// 0x00054745
		test eax, eax		// 0x00054748
		jne L54753		// 0x0005474a
		mov ecx, esi		// 0x0005474c
		call nxTM2081		// 0x0005474e
L54753:
		mov eax, dword ptr [esi + 0x18]		// 0x00054753
		pop esi		// 0x00054756
		ret 8		// 0x00054757
L5475a:
		mov esi, dword ptr [esi + 0xa0]		// 0x0005475a
		test esi, esi		// 0x00054760
		je L5478d		// 0x00054762
L54764:
		mov eax, dword ptr [esi + 0x10]		// 0x00054764
		pop esi		// 0x00054767
		ret 8		// 0x00054768
L5476b:
		mov eax, dword ptr [esi + 0xa0]		// 0x0005476b
		test eax, eax		// 0x00054771
		je L5478d		// 0x00054773
		mov eax, dword ptr [esi + 4]		// 0x00054775
		lea ecx, [esi + 4]		// 0x00054778
		push 0		// 0x0005477b
		call dword ptr [eax + 0x10]		// 0x0005477d
		mov ecx, dword ptr [esi + 0xa0]		// 0x00054780
		mov eax, dword ptr [ecx + 0x2c]		// 0x00054786
		pop esi		// 0x00054789
		ret 8		// 0x0005478a
L5478d:
		xor eax, eax		// 0x0005478d
		pop esi		// 0x0005478f
		ret 8		// 0x00054790
		}
	}

// phys_fn_002239 (0x00054a80, 290 B), 97 instructions
__declspec(naked) unsigned __fastcall nxTM2239(void* self, void* edxUnused)
	{
	__asm
		{
		push ebx		// 0x00054a80
		push esi		// 0x00054a81
		mov esi, ecx		// 0x00054a82
		push edi		// 0x00054a84
		lea ecx, [esi + 8]		// 0x00054a85
		call nxTMCallNxMTReleaseRow		// 0x00054a88
		mov eax, dword ptr [esi + 0x98]		// 0x00054a8d
		xor edi, edi		// 0x00054a93
		cmp eax, edi		// 0x00054a95
		je L54aad		// 0x00054a97
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00054a99
		mov ecx, dword ptr [ecx]		// 0x00054a9f
		mov edx, dword ptr [ecx]		// 0x00054aa1
		push eax		// 0x00054aa3
		call dword ptr [edx + 0x14]		// 0x00054aa4
		mov dword ptr [esi + 0x98], edi		// 0x00054aa7
L54aad:
		mov eax, dword ptr [esi + 0x94]		// 0x00054aad
		cmp eax, edi		// 0x00054ab3
		je L54acb		// 0x00054ab5
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00054ab7
		mov ecx, dword ptr [ecx]		// 0x00054abd
		mov edx, dword ptr [ecx]		// 0x00054abf
		push eax		// 0x00054ac1
		call dword ptr [edx + 0x14]		// 0x00054ac2
		mov dword ptr [esi + 0x94], edi		// 0x00054ac5
L54acb:
		mov ecx, dword ptr [esi + 0xac]		// 0x00054acb
		cmp ecx, edi		// 0x00054ad1
		je L54ae1		// 0x00054ad3
		mov eax, dword ptr [ecx]		// 0x00054ad5
		push 1		// 0x00054ad7
		call dword ptr [eax]		// 0x00054ad9
		mov dword ptr [esi + 0xac], edi		// 0x00054adb
L54ae1:
		mov ecx, dword ptr [esi + 0xa8]		// 0x00054ae1
		cmp ecx, edi		// 0x00054ae7
		je L54af7		// 0x00054ae9
		mov edx, dword ptr [ecx]		// 0x00054aeb
		push 1		// 0x00054aed
		call dword ptr [edx]		// 0x00054aef
		mov dword ptr [esi + 0xa8], edi		// 0x00054af1
L54af7:
		mov ecx, dword ptr [esi + 0xa4]		// 0x00054af7
		cmp ecx, edi		// 0x00054afd
		je L54b0d		// 0x00054aff
		mov eax, dword ptr [ecx]		// 0x00054b01
		push 1		// 0x00054b03
		call dword ptr [eax]		// 0x00054b05
		mov dword ptr [esi + 0xa4], edi		// 0x00054b07
L54b0d:
		mov ecx, dword ptr [esi + 0xa0]		// 0x00054b0d
		cmp ecx, edi		// 0x00054b13
		je L54b23		// 0x00054b15
		mov edx, dword ptr [ecx]		// 0x00054b17
		push 1		// 0x00054b19
		call dword ptr [edx]		// 0x00054b1b
		mov dword ptr [esi + 0xa0], edi		// 0x00054b1d
L54b23:
		mov ecx, dword ptr [esi + 0x9c]		// 0x00054b23
		cmp ecx, edi		// 0x00054b29
		je L54b39		// 0x00054b2b
		mov eax, dword ptr [ecx]		// 0x00054b2d
		push 1		// 0x00054b2f
		call dword ptr [eax]		// 0x00054b31
		mov dword ptr [esi + 0x9c], edi		// 0x00054b33
L54b39:
		mov ebx, dword ptr [esi + 0x88]		// 0x00054b39
		cmp ebx, edi		// 0x00054b3f
		je L54b5d		// 0x00054b41
		mov ecx, ebx		// 0x00054b43
		call nxTM2060		// 0x00054b45
		call nxTMCallNxGetSdkAllocator		// 0x00054b4a
		mov edx, dword ptr [eax]		// 0x00054b4f
		push ebx		// 0x00054b51
		mov ecx, eax		// 0x00054b52
		call dword ptr [edx + 0xc]		// 0x00054b54
		mov dword ptr [esi + 0x88], edi		// 0x00054b57
L54b5d:
		mov eax, dword ptr [esi + 0x3c]		// 0x00054b5d
		cmp eax, edi		// 0x00054b60
		je L54b75		// 0x00054b62
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00054b64
		mov ecx, dword ptr [ecx]		// 0x00054b6a
		mov edx, dword ptr [ecx]		// 0x00054b6c
		push eax		// 0x00054b6e
		call dword ptr [edx + 0x14]		// 0x00054b6f
		mov dword ptr [esi + 0x3c], edi		// 0x00054b72
L54b75:
		mov ebx, dword ptr [esi + 0x84]		// 0x00054b75
		cmp ebx, 1		// 0x00054b7b
		je L54b9e		// 0x00054b7e
		cmp ebx, edi		// 0x00054b80
		je L54b9e		// 0x00054b82
		mov ecx, ebx		// 0x00054b84
		call nxTM1544		// 0x00054b86
		call nxTMCallNxGetSdkAllocator		// 0x00054b8b
		mov edx, dword ptr [eax]		// 0x00054b90
		push ebx		// 0x00054b92
		mov ecx, eax		// 0x00054b93
		call dword ptr [edx + 0xc]		// 0x00054b95
		mov dword ptr [esi + 0x84], edi		// 0x00054b98
L54b9e:
		pop edi		// 0x00054b9e
		pop esi		// 0x00054b9f
		pop ebx		// 0x00054ba0
		ret		// 0x00054ba1
		}
	}

// phys_fn_002243 and phys_fn_002245 (one function) (0x00054e70, 779 B), 283 instructions
__declspec(naked) unsigned __fastcall nxTM2243(void* self, void* edxUnused)
	{
	__asm
		{
		sub esp, 0x20		// 0x00054e70
		push ebx		// 0x00054e73
		push ebp		// 0x00054e74
		mov ebp, ecx		// 0x00054e75
		mov eax, dword ptr [ebp + 0x3c]		// 0x00054e77
		test eax, eax		// 0x00054e7a
		push edi		// 0x00054e7c
		mov dword ptr [esp + 0xc], ebp		// 0x00054e7d
		je L54e98		// 0x00054e81
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00054e83
		mov ecx, dword ptr [ecx]		// 0x00054e89
		mov edx, dword ptr [ecx]		// 0x00054e8b
		push eax		// 0x00054e8d
		call dword ptr [edx + 0x14]		// 0x00054e8e
		mov dword ptr [ebp + 0x3c], 0		// 0x00054e91
L54e98:
		mov ebx, dword ptr [ebp + 0xc]		// 0x00054e98
		cmp ebx, 0x40000000		// 0x00054e9b
		mov edi, dword ptr [ebp + 0x14]		// 0x00054ea1
		mov dword ptr [esp + 0x1c], edi		// 0x00054ea4
		jb L54ecd		// 0x00054ea8
		push offset nxTMStr1085a4		// 0x00054eaa
		push 0		// 0x00054eaf
		push 0x3a0		// 0x00054eb1
		push offset nxTMStr108364		// 0x00054eb6
		push 1		// 0x00054ebb
		call dword ptr [nxTMImpImperrorFoundationSDKNxFoundationSANW4Nx]		// 0x00054ebd
		add esp, 0x14		// 0x00054ec3
		pop edi		// 0x00054ec6
		pop ebp		// 0x00054ec7
		pop ebx		// 0x00054ec8
		add esp, 0x20		// 0x00054ec9
		ret		// 0x00054ecc
L54ecd:
		mov eax, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00054ecd
		mov ecx, dword ptr [eax]		// 0x00054ed2
		mov edx, dword ptr [ecx]		// 0x00054ed4
		push esi		// 0x00054ed6
		mov eax, ebx		// 0x00054ed7
		push 0		// 0x00054ed9
		shl eax, 4		// 0x00054edb
		push eax		// 0x00054ede
		call dword ptr [edx + 8]		// 0x00054edf
		mov esi, eax		// 0x00054ee2
		test esi, esi		// 0x00054ee4
		je L54ef8		// 0x00054ee6
		push offset nxTMIdentityCtor		// 0x00054ee8
		push ebx		// 0x00054eed
		push 0x10		// 0x00054eee
		push esi		// 0x00054ef0
		call nxTM0001		// 0x00054ef1
		jmp L54efa		// 0x00054ef6
L54ef8:
		xor esi, esi		// 0x00054ef8
L54efa:
		test ebx, ebx		// 0x00054efa
		mov dword ptr [ebp + 0x3c], esi		// 0x00054efc
		jbe L55006		// 0x00054eff
		xor ebp, ebp		// 0x00054f05
		mov dword ptr [esp + 0x1c], ebx		// 0x00054f07
		jmp L54f10		// 0x00054f0b
		lea ecx, [ecx]		// 0x00054f0d
L54f10:
		mov edx, dword ptr [esp + 0x10]		// 0x00054f10
		mov edx, dword ptr [edx + 0x10]		// 0x00054f14
		mov ecx, edi		// 0x00054f17
		mov esi, dword ptr [ecx + 4]		// 0x00054f19
		mov eax, dword ptr [ecx]		// 0x00054f1c
		mov ecx, dword ptr [ecx + 8]		// 0x00054f1e
		lea esi, [esi + esi*2]		// 0x00054f21
		fld dword ptr [edx + esi*4]		// 0x00054f24
		lea esi, [edx + esi*4]		// 0x00054f27
		lea eax, [eax + eax*2]		// 0x00054f2a
		fsub dword ptr [edx + eax*4]		// 0x00054f2d
		lea eax, [edx + eax*4]		// 0x00054f30
		fld dword ptr [esi + 4]		// 0x00054f33
		lea ecx, [ecx + ecx*2]		// 0x00054f36
		fsub dword ptr [eax + 4]		// 0x00054f39
		lea ecx, [edx + ecx*4]		// 0x00054f3c
		fld dword ptr [esi + 8]		// 0x00054f3f
		mov edx, dword ptr [esp + 0x10]		// 0x00054f42
		fsub dword ptr [eax + 8]		// 0x00054f46
		add edi, 0xc		// 0x00054f49
		fld dword ptr [ecx]		// 0x00054f4c
		fsub dword ptr [eax]		// 0x00054f4e
		fstp dword ptr [esp + 0x24]		// 0x00054f50
		fld dword ptr [ecx + 4]		// 0x00054f54
		fsub dword ptr [eax + 4]		// 0x00054f57
		fstp dword ptr [esp + 0x28]		// 0x00054f5a
		fld dword ptr [ecx + 8]		// 0x00054f5e
		mov ecx, dword ptr [edx + 0x3c]		// 0x00054f61
		fsub dword ptr [eax + 8]		// 0x00054f64
		add ecx, ebp		// 0x00054f67
		fld st(0)		// 0x00054f69
		fmul st(0), st(3)		// 0x00054f6b
		fld dword ptr [esp + 0x28]		// 0x00054f6d
		fmul st(0), st(3)		// 0x00054f71
		fsubp st(1), st(0)		// 0x00054f73
		fst dword ptr [esp + 0x14]		// 0x00054f75
		fstp dword ptr [ecx]		// 0x00054f79
		fxch st(1)		// 0x00054f7b
		fmul dword ptr [esp + 0x24]		// 0x00054f7d
		fxch st(1)		// 0x00054f81
		fmul st(0), st(3)		// 0x00054f83
		fsubp st(1), st(0)		// 0x00054f85
		fst dword ptr [esp + 0x18]		// 0x00054f87
		fstp dword ptr [ecx + 4]		// 0x00054f8b
		fld dword ptr [esp + 0x28]		// 0x00054f8e
		fmulp st(2), st(0)		// 0x00054f92
		fmul dword ptr [esp + 0x24]		// 0x00054f94
		fsubp st(1), st(0)		// 0x00054f98
		fst dword ptr [ecx + 8]		// 0x00054f9a
		fld st(0)		// 0x00054f9d
		fmul st(0), st(1)		// 0x00054f9f
		fld dword ptr [esp + 0x18]		// 0x00054fa1
		fmul dword ptr [esp + 0x18]		// 0x00054fa5
		faddp st(1), st(0)		// 0x00054fa9
		fld dword ptr [esp + 0x14]		// 0x00054fab
		fmul dword ptr [esp + 0x14]		// 0x00054faf
		faddp st(1), st(0)		// 0x00054fb3
		fsqrt		// 0x00054fb5
		fld dword ptr [nxTMK101041f0_4]		// 0x00054fb7
		fld st(1)		// 0x00054fbd
		fucompp		// 0x00054fbf
		fnstsw ax		// 0x00054fc1
		test ah, 0x44		// 0x00054fc3
		jnp L54fe8		// 0x00054fc6
		fdivr dword ptr [nxTMK101041ec_4]		// 0x00054fc8
		fld dword ptr [esp + 0x14]		// 0x00054fce
		fmul st(0), st(1)		// 0x00054fd2
		fstp dword ptr [ecx]		// 0x00054fd4
		fld dword ptr [esp + 0x18]		// 0x00054fd6
		fmul st(0), st(1)		// 0x00054fda
		fstp dword ptr [ecx + 4]		// 0x00054fdc
		fxch st(1)		// 0x00054fdf
		fmul st(0), st(1)		// 0x00054fe1
		fstp dword ptr [ecx + 8]		// 0x00054fe3
		jmp L54fea		// 0x00054fe6
L54fe8:
		fstp st(0)		// 0x00054fe8
L54fea:
		mov eax, dword ptr [esp + 0x1c]		// 0x00054fea
		fstp st(0)		// 0x00054fee
		add ebp, 0x10		// 0x00054ff0
		dec eax		// 0x00054ff3
		mov byte ptr [ecx + 0xc], 0		// 0x00054ff4
		mov dword ptr [esp + 0x1c], eax		// 0x00054ff8
		jne L54f10		// 0x00054ffc
		mov edi, dword ptr [esp + 0x20]		// 0x00055002
L55006:
		mov eax, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00055006
		mov ecx, dword ptr [eax]		// 0x0005500b
		mov edx, dword ptr [ecx]		// 0x0005500d
		lea esi, [ebx + ebx*2]		// 0x0005500f
		mov eax, esi		// 0x00055012
		push 0		// 0x00055014
		shl eax, 4		// 0x00055016
		push eax		// 0x00055019
		mov dword ptr [esp + 0x24], esi		// 0x0005501a
		call dword ptr [edx + 8]		// 0x0005501e
		mov ebp, eax		// 0x00055021
		xor edx, edx		// 0x00055023
		test ebx, ebx		// 0x00055025
		mov dword ptr [esp + 0x18], ebp		// 0x00055027
		jbe L550bb		// 0x0005502b
		jmp L55037		// 0x00055031
L55033:
		mov edi, dword ptr [esp + 0x20]		// 0x00055033
L55037:
		mov ecx, edi		// 0x00055037
		mov esi, dword ptr [ecx + 4]		// 0x00055039
		add edi, 0xc		// 0x0005503c
		mov dword ptr [esp + 0x20], edi		// 0x0005503f
		mov edi, dword ptr [ecx]		// 0x00055043
		cmp edi, esi		// 0x00055045
		jae L55050		// 0x00055047
		mov dword ptr [eax], edi		// 0x00055049
		mov dword ptr [eax + 4], esi		// 0x0005504b
		jmp L55055		// 0x0005504e
L55050:
		mov dword ptr [eax], esi		// 0x00055050
		mov dword ptr [eax + 4], edi		// 0x00055052
L55055:
		or ebp, 0xffffffff		// 0x00055055
		mov dword ptr [eax + 8], edx		// 0x00055058
		mov dword ptr [eax + 0xc], ebp		// 0x0005505b
		mov esi, dword ptr [ecx + 8]		// 0x0005505e
		mov edi, dword ptr [ecx]		// 0x00055061
		add eax, 0x10		// 0x00055063
		cmp edi, esi		// 0x00055066
		jae L55071		// 0x00055068
		mov dword ptr [eax], edi		// 0x0005506a
		mov dword ptr [eax + 4], esi		// 0x0005506c
		jmp L55076		// 0x0005506f
L55071:
		mov dword ptr [eax], esi		// 0x00055071
		mov dword ptr [eax + 4], edi		// 0x00055073
L55076:
		mov esi, edx		// 0x00055076
		or esi, 0x40000000		// 0x00055078
		mov dword ptr [eax + 8], esi		// 0x0005507e
		mov dword ptr [eax + 0xc], ebp		// 0x00055081
		mov esi, dword ptr [ecx + 8]		// 0x00055084
		mov ecx, dword ptr [ecx + 4]		// 0x00055087
		add eax, 0x10		// 0x0005508a
		cmp ecx, esi		// 0x0005508d
		jae L55098		// 0x0005508f
		mov dword ptr [eax], ecx		// 0x00055091
		mov dword ptr [eax + 4], esi		// 0x00055093
		jmp L5509d		// 0x00055096
L55098:
		mov dword ptr [eax], esi		// 0x00055098
		mov dword ptr [eax + 4], ecx		// 0x0005509a
L5509d:
		mov ecx, edx		// 0x0005509d
		or ecx, 0x80000000		// 0x0005509f
		mov dword ptr [eax + 8], ecx		// 0x000550a5
		mov dword ptr [eax + 0xc], ebp		// 0x000550a8
		add eax, 0x10		// 0x000550ab
		inc edx		// 0x000550ae
		cmp edx, ebx		// 0x000550af
		jb L55033		// 0x000550b1
		mov esi, dword ptr [esp + 0x1c]		// 0x000550b3
		mov ebp, dword ptr [esp + 0x18]		// 0x000550b7
L550bb:
		lea edx, [esi - 1]		// 0x000550bb
		push edx		// 0x000550be
		push 0		// 0x000550bf
		push ebp		// 0x000550c1
		call nxTMCallNxTriangleMeshSortEdgeRecords		// 0x000550c2
		add esp, 0xc		// 0x000550c7
		xor edx, edx		// 0x000550ca
		cmp esi, 1		// 0x000550cc
		jbe L5514f		// 0x000550cf
		mov esi, dword ptr [esp + 0x18]		// 0x000550d1
		mov eax, dword ptr [esp + 0x1c]		// 0x000550d5
		add esi, 0x18		// 0x000550d9
		dec eax		// 0x000550dc
		mov edi, ebp		// 0x000550dd
		mov dword ptr [esp + 0x1c], eax		// 0x000550df
L550e3:
		mov ecx, dword ptr [edi]		// 0x000550e3
		cmp ecx, dword ptr [esi - 8]		// 0x000550e5
		jne L5511f		// 0x000550e8
		mov eax, dword ptr [edi + 4]		// 0x000550ea
		cmp eax, dword ptr [esi - 4]		// 0x000550ed
		jne L5511f		// 0x000550f0
		cmp dword ptr [edi + 0xc], -1		// 0x000550f2
		mov ecx, dword ptr [esi]		// 0x000550f6
		jne L550ff		// 0x000550f8
		mov dword ptr [edi + 0xc], ecx		// 0x000550fa
		jmp L5513d		// 0x000550fd
L550ff:
		mov ebx, dword ptr [esp + 0x10]		// 0x000550ff
		mov ebx, dword ptr [ebx + 0x3c]		// 0x00055103
		mov eax, ecx		// 0x00055106
		and eax, 0x3fffffff		// 0x00055108
		shl eax, 4		// 0x0005510d
		lea eax, [eax + ebx + 0xc]		// 0x00055110
		shr ecx, 0x1e		// 0x00055114
		mov bl, 1		// 0x00055117
		shl bl, cl		// 0x00055119
		or byte ptr [eax], bl		// 0x0005511b
		jmp L5513d		// 0x0005511d
L5511f:
		mov ecx, dword ptr [esi - 8]		// 0x0005511f
		inc edx		// 0x00055122
		add ebp, 0x10		// 0x00055123
		mov eax, ebp		// 0x00055126
		mov dword ptr [eax], ecx		// 0x00055128
		mov ecx, dword ptr [esi - 4]		// 0x0005512a
		mov dword ptr [eax + 4], ecx		// 0x0005512d
		mov ecx, dword ptr [esi]		// 0x00055130
		mov dword ptr [eax + 8], ecx		// 0x00055132
		mov ecx, dword ptr [esi + 4]		// 0x00055135
		mov edi, ebp		// 0x00055138
		mov dword ptr [eax + 0xc], ecx		// 0x0005513a
L5513d:
		mov eax, dword ptr [esp + 0x1c]		// 0x0005513d
		add esi, 0x10		// 0x00055141
		dec eax		// 0x00055144
		mov dword ptr [esp + 0x1c], eax		// 0x00055145
		jne L550e3		// 0x00055149
		mov ebp, dword ptr [esp + 0x18]		// 0x0005514b
L5514f:
		mov esi, ebp		// 0x0005514f
		lea edi, [edx + 1]		// 0x00055151
L55154:
		mov edx, dword ptr [esp + 0x10]		// 0x00055154
		push edx		// 0x00055158
		mov ecx, esi		// 0x00055159
		call nxTM2184		// 0x0005515b
		add esi, 0x10		// 0x00055160
		dec edi		// 0x00055163
		jne L55154		// 0x00055164
		mov eax, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x00055166
		mov ecx, dword ptr [eax]		// 0x0005516b
		mov edx, dword ptr [ecx]		// 0x0005516d
		push ebp		// 0x0005516f
		call dword ptr [edx + 0x14]		// 0x00055170
		pop esi		// 0x00055173
		pop edi		// 0x00055174
		pop ebp		// 0x00055175
		pop ebx		// 0x00055176
		add esp, 0x20		// 0x00055177
		ret		// 0x0005517a
		}
	}

// phys_fn_002247 (0x00055180, 309 B), 99 instructions
__declspec(naked) unsigned __fastcall nxTM2247(void* self, void* edxUnused)
	{
	__asm
		{
		sub esp, 0x4c		// 0x00055180
		push ebx		// 0x00055183
		mov ebx, ecx		// 0x00055184
		push esi		// 0x00055186
		lea ecx, [esp + 0x20]		// 0x00055187
		call nxTM1421		// 0x0005518b
		mov eax, dword ptr [ebx + 0x88]		// 0x00055190
		test eax, eax		// 0x00055196
		jne L551a1		// 0x00055198
		mov ecx, ebx		// 0x0005519a
		call nxTMCallCreateEdgeList		// 0x0005519c
L551a1:
		mov eax, dword ptr [ebx + 0x88]		// 0x000551a1
		mov ecx, dword ptr [ebx + 0x10]		// 0x000551a7
		mov edx, dword ptr [ebx + 8]		// 0x000551aa
		push edi		// 0x000551ad
		push eax		// 0x000551ae
		mov eax, dword ptr [ebx + 0x14]		// 0x000551af
		push ecx		// 0x000551b2
		mov ecx, dword ptr [ebx + 0xc]		// 0x000551b3
		push edx		// 0x000551b6
		push eax		// 0x000551b7
		push ecx		// 0x000551b8
		lea ecx, [esp + 0x38]		// 0x000551b9
		call nxTMCallNxHullCluster		// 0x000551bd
		mov edx, dword ptr [esp + 0x48]		// 0x000551c2
		mov eax, dword ptr [esp + 0x50]		// 0x000551c6
		mov dword ptr [ebx + 0x90], eax		// 0x000551ca
		mov eax, dword ptr [ebx + 0xc]		// 0x000551d0
		mov dword ptr [ebx + 0x8c], edx		// 0x000551d3
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000551d9
		mov ecx, dword ptr [ecx]		// 0x000551df
		mov edx, dword ptr [ecx]		// 0x000551e1
		push 0		// 0x000551e3
		shl eax, 2		// 0x000551e5
		push eax		// 0x000551e8
		call dword ptr [edx + 8]		// 0x000551e9
		mov dword ptr [ebx + 0x94], eax		// 0x000551ec
		mov eax, dword ptr [ebx + 0xc]		// 0x000551f2
		mov ecx, dword ptr [nxTMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000551f5
		mov ecx, dword ptr [ecx]		// 0x000551fb
		mov edx, dword ptr [ecx]		// 0x000551fd
		push 0		// 0x000551ff
		shl eax, 2		// 0x00055201
		push eax		// 0x00055204
		call dword ptr [edx + 8]		// 0x00055205
		mov ecx, dword ptr [ebx + 0xc]		// 0x00055208
		mov esi, dword ptr [esp + 0x4c]		// 0x0005520b
		mov edi, dword ptr [ebx + 0x94]		// 0x0005520f
		shl ecx, 2		// 0x00055215
		mov edx, ecx		// 0x00055218
		shr ecx, 2		// 0x0005521a
		mov dword ptr [ebx + 0x98], eax		// 0x0005521d
		rep movsd		// 0x00055223
		mov ecx, edx		// 0x00055225
		and ecx, 3		// 0x00055227
		rep movsb		// 0x0005522a
		mov ecx, dword ptr [ebx + 0xc]		// 0x0005522c
		mov esi, dword ptr [esp + 0x54]		// 0x0005522f
		mov edi, dword ptr [ebx + 0x98]		// 0x00055233
		shl ecx, 2		// 0x00055239
		mov eax, ecx		// 0x0005523c
		shr ecx, 2		// 0x0005523e
		rep movsd		// 0x00055241
		mov ecx, eax		// 0x00055243
		and ecx, 3		// 0x00055245
		rep movsb		// 0x00055248
		lea ecx, [esp + 0xc]		// 0x0005524a
		call nxTM5157		// 0x0005524e
		mov eax, dword ptr [ebx + 0xc]		// 0x00055253
		mov ecx, dword ptr [ebx + 0x94]		// 0x00055256
		push 1		// 0x0005525c
		push eax		// 0x0005525e
		push ecx		// 0x0005525f
		lea ecx, [esp + 0x18]		// 0x00055260
		call nxTMCallSort		// 0x00055264
		mov edx, dword ptr [eax + 4]		// 0x00055269
		push edx		// 0x0005526c
		mov ecx, ebx		// 0x0005526d
		call nxTM2190		// 0x0005526f
		mov esi, dword ptr [ebx + 0x88]		// 0x00055274
		test esi, esi		// 0x0005527a
		pop edi		// 0x0005527c
		je L5529d		// 0x0005527d
		mov ecx, esi		// 0x0005527f
		call nxTM2060		// 0x00055281
		call nxTMCallNxGetSdkAllocator		// 0x00055286
		mov edx, dword ptr [eax]		// 0x0005528b
		push esi		// 0x0005528d
		mov ecx, eax		// 0x0005528e
		call dword ptr [edx + 0xc]		// 0x00055290
		mov dword ptr [ebx + 0x88], 0		// 0x00055293
L5529d:
		lea ecx, [esp + 8]		// 0x0005529d
		call nxTM5159		// 0x000552a1
		lea ecx, [esp + 0x20]		// 0x000552a6
		call nxTMCallNxHullFreeBuffers		// 0x000552aa
		pop esi		// 0x000552af
		pop ebx		// 0x000552b0
		add esp, 0x4c		// 0x000552b1
		ret		// 0x000552b4
		}
	}

// phys_fn_002255 (0x000555b0, 272 B), 86 instructions
__declspec(naked) unsigned __fastcall nxTM2255(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x000555b0
		mov esi, ecx		// 0x000555b1
		call nxTM2243		// 0x000555b3
		mov eax, dword ptr [esi + 0xa0]		// 0x000555b8
		test eax, eax		// 0x000555be
		je L556b0		// 0x000555c0
		mov ecx, dword ptr [esi + 0xa4]		// 0x000555c6
		test ecx, ecx		// 0x000555cc
		je L555e0		// 0x000555ce
		mov eax, dword ptr [ecx]		// 0x000555d0
		push 1		// 0x000555d2
		call dword ptr [eax]		// 0x000555d4
		mov dword ptr [esi + 0xa4], 0		// 0x000555d6
L555e0:
		call nxTMCallNxGetSdkAllocator		// 0x000555e0
		mov edx, dword ptr [eax]		// 0x000555e5
		push 0		// 0x000555e7
		push 0x14		// 0x000555e9
		mov ecx, eax		// 0x000555eb
		call dword ptr [edx]		// 0x000555ed
		test eax, eax		// 0x000555ef
		je L55603		// 0x000555f1
		mov ecx, dword ptr [esi + 0xa0]		// 0x000555f3
		push ecx		// 0x000555f9
		mov ecx, eax		// 0x000555fa
		call nxTMCallNxSupportMapHullConstruct		// 0x000555fc
		jmp L55605		// 0x00055601
L55603:
		xor eax, eax		// 0x00055603
L55605:
		push 0x10		// 0x00055605
		mov ecx, eax		// 0x00055607
		mov dword ptr [esi + 0xa4], eax		// 0x00055609
		call nxTMCallNxSupportMapInit		// 0x0005560f
		mov ecx, dword ptr [esi + 0xa8]		// 0x00055614
		test ecx, ecx		// 0x0005561a
		je L5562e		// 0x0005561c
		mov edx, dword ptr [ecx]		// 0x0005561e
		push 1		// 0x00055620
		call dword ptr [edx]		// 0x00055622
		mov dword ptr [esi + 0xa8], 0		// 0x00055624
L5562e:
		call nxTMCallNxGetSdkAllocator		// 0x0005562e
		mov edx, dword ptr [eax]		// 0x00055633
		push 0		// 0x00055635
		push 0x18		// 0x00055637
		mov ecx, eax		// 0x00055639
		call dword ptr [edx]		// 0x0005563b
		test eax, eax		// 0x0005563d
		je L55651		// 0x0005563f
		mov ecx, dword ptr [esi + 0xa0]		// 0x00055641
		push ecx		// 0x00055647
		mov ecx, eax		// 0x00055648
		call nxTMCallNxSupportMapVertexConstruct		// 0x0005564a
		jmp L55653		// 0x0005564f
L55651:
		xor eax, eax		// 0x00055651
L55653:
		push 0x10		// 0x00055653
		mov ecx, eax		// 0x00055655
		mov dword ptr [esi + 0xa8], eax		// 0x00055657
		call nxTMCallNxSupportMapInit		// 0x0005565d
		mov ecx, dword ptr [esi + 0xac]		// 0x00055662
		test ecx, ecx		// 0x00055668
		je L5567c		// 0x0005566a
		mov edx, dword ptr [ecx]		// 0x0005566c
		push 1		// 0x0005566e
		call dword ptr [edx]		// 0x00055670
		mov dword ptr [esi + 0xac], 0		// 0x00055672
L5567c:
		call nxTMCallNxGetSdkAllocator		// 0x0005567c
		mov edx, dword ptr [eax]		// 0x00055681
		push 0		// 0x00055683
		push 0x14		// 0x00055685
		mov ecx, eax		// 0x00055687
		call dword ptr [edx]		// 0x00055689
		test eax, eax		// 0x0005568b
		je L5569f		// 0x0005568d
		mov ecx, dword ptr [esi + 0xa0]		// 0x0005568f
		push ecx		// 0x00055695
		mov ecx, eax		// 0x00055696
		call nxTMCallNxSupportMapPlaneConstruct		// 0x00055698
		jmp L556a1		// 0x0005569d
L5569f:
		xor eax, eax		// 0x0005569f
L556a1:
		push 0x10		// 0x000556a1
		mov ecx, eax		// 0x000556a3
		mov dword ptr [esi + 0xac], eax		// 0x000556a5
		call nxTMCallNxSupportMapInit		// 0x000556ab
L556b0:
		mov ecx, esi		// 0x000556b0
		call nxTM2178		// 0x000556b2
		lea ecx, [esi + 8]		// 0x000556b7
		pop esi		// 0x000556ba
		jmp nxTM2079		// 0x000556bb
		}
	}

// phys_fn_002256 (0x000556c0, 330 B), 121 instructions
__declspec(naked) unsigned __fastcall nxTM2256(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 8		// 0x000556c0
		push ebx		// 0x000556c3
		push ebp		// 0x000556c4
		mov ebx, ecx		// 0x000556c5
		push esi		// 0x000556c7
		push edi		// 0x000556c8
		lea edi, [ebx + 8]		// 0x000556c9
		mov ecx, edi		// 0x000556cc
		mov dword ptr [esp + 0x10], ebx		// 0x000556ce
		call nxTMCallNxMTReleaseRow		// 0x000556d2
		mov esi, dword ptr [esp + 0x1c]		// 0x000556d7
		mov eax, dword ptr [esi]		// 0x000556db
		push eax		// 0x000556dd
		mov ecx, edi		// 0x000556de
		call nxTM2069		// 0x000556e0
		mov ecx, dword ptr [esi + 4]		// 0x000556e5
		push ecx		// 0x000556e8
		mov ecx, edi		// 0x000556e9
		mov ebp, eax		// 0x000556eb
		call nxTM2071		// 0x000556ed
		mov edx, dword ptr [esi + 8]		// 0x000556f2
		mov edi, dword ptr [edi]		// 0x000556f5
		test edi, edi		// 0x000556f7
		mov dword ptr [esp + 0x14], edx		// 0x000556f9
		mov edx, dword ptr [esi + 0x10]		// 0x000556fd
		mov ecx, ebp		// 0x00055700
		je L55734		// 0x00055702
		mov dword ptr [esp + 0x1c], edi		// 0x00055704
L55708:
		mov edi, edx		// 0x00055708
		mov ebp, dword ptr [edi]		// 0x0005570a
		mov ebx, ecx		// 0x0005570c
		mov dword ptr [ebx], ebp		// 0x0005570e
		mov ebp, dword ptr [edi + 4]		// 0x00055710
		mov dword ptr [ebx + 4], ebp		// 0x00055713
		mov edi, dword ptr [edi + 8]		// 0x00055716
		mov dword ptr [ebx + 8], edi		// 0x00055719
		mov ebx, dword ptr [esp + 0x14]		// 0x0005571c
		mov edi, dword ptr [esp + 0x1c]		// 0x00055720
		add ecx, 0xc		// 0x00055724
		add edx, ebx		// 0x00055727
		dec edi		// 0x00055729
		mov dword ptr [esp + 0x1c], edi		// 0x0005572a
		jne L55708		// 0x0005572e
		mov ebx, dword ptr [esp + 0x10]		// 0x00055730
L55734:
		mov edx, dword ptr [esi + 0x18]		// 0x00055734
		mov ecx, dword ptr [ebx + 0xc]		// 0x00055737
		lea ecx, [ecx + ecx*2]		// 0x0005573a
		mov edi, edx		// 0x0005573d
		and edi, 1		// 0x0005573f
		test dl, 2		// 0x00055742
		lea ebp, [eax + ecx*4]		// 0x00055745
		mov ecx, dword ptr [esi + 0x14]		// 0x00055748
		je L55782		// 0x0005574b
		cmp eax, ebp		// 0x0005574d
		jae L557ad		// 0x0005574f
		mov edx, 2		// 0x00055751
		sub edx, edi		// 0x00055756
		shl edx, 1		// 0x00055758
		lea ebx, [ebx]		// 0x0005575a
L55760:
		movzx ebx, word ptr [ecx]		// 0x00055760
		mov dword ptr [eax], ebx		// 0x00055763
		movzx ebx, word ptr [ecx + edi*2 + 2]		// 0x00055765
		mov dword ptr [eax + 4], ebx		// 0x0005576a
		movzx ebx, word ptr [edx + ecx]		// 0x0005576d
		mov dword ptr [eax + 8], ebx		// 0x00055771
		mov ebx, dword ptr [esi + 0xc]		// 0x00055774
		add eax, 0xc		// 0x00055777
		add ecx, ebx		// 0x0005577a
		cmp eax, ebp		// 0x0005577c
		jb L55760		// 0x0005577e
		jmp L557ad		// 0x00055780
L55782:
		cmp eax, ebp		// 0x00055782
		jae L557ad		// 0x00055784
		mov edx, 2		// 0x00055786
		sub edx, edi		// 0x0005578b
		shl edx, 2		// 0x0005578d
L55790:
		mov ebx, dword ptr [ecx]		// 0x00055790
		mov dword ptr [eax], ebx		// 0x00055792
		mov ebx, dword ptr [ecx + edi*4 + 4]		// 0x00055794
		mov dword ptr [eax + 4], ebx		// 0x00055798
		mov ebx, dword ptr [edx + ecx]		// 0x0005579b
		mov dword ptr [eax + 8], ebx		// 0x0005579e
		mov ebx, dword ptr [esi + 0xc]		// 0x000557a1
		add eax, 0xc		// 0x000557a4
		add ecx, ebx		// 0x000557a7
		cmp eax, ebp		// 0x000557a9
		jb L55790		// 0x000557ab
L557ad:
		mov eax, dword ptr [esi + 0x20]		// 0x000557ad
		test eax, eax		// 0x000557b0
		mov ebx, dword ptr [esp + 0x10]		// 0x000557b2
		je L557de		// 0x000557b6
		lea ecx, [ebx + 8]		// 0x000557b8
		call nxTM2073		// 0x000557bb
		mov edx, dword ptr [ebx + 0xc]		// 0x000557c0
		test edx, edx		// 0x000557c3
		mov edi, dword ptr [esi + 0x1c]		// 0x000557c5
		mov ecx, dword ptr [esi + 0x20]		// 0x000557c8
		je L557de		// 0x000557cb
		lea ecx, [ecx]		// 0x000557cd
L557d0:
		mov bp, word ptr [ecx]		// 0x000557d0
		mov word ptr [eax], bp		// 0x000557d3
		add eax, 2		// 0x000557d6
		add ecx, edi		// 0x000557d9
		dec edx		// 0x000557db
		jne L557d0		// 0x000557dc
L557de:
		lea ecx, [ebx + 8]		// 0x000557de
		call nxTMCallNxMTBuildTopologyRow		// 0x000557e1
		test al, al		// 0x000557e6
		je L55800		// 0x000557e8
		test byte ptr [esi + 0x18], 4		// 0x000557ea
		jne L55800		// 0x000557ee
		cmp dword ptr [esi + 0x24], 0xff		// 0x000557f0
		jne L55800		// 0x000557f7
		mov ecx, ebx		// 0x000557f9
		call nxTM2247		// 0x000557fb
L55800:
		pop edi		// 0x00055800
		pop esi		// 0x00055801
		pop ebp		// 0x00055802
		pop ebx		// 0x00055803
		add esp, 8		// 0x00055804
		ret 4		// 0x00055807
		}
	}

// helper: phys_fn_005155 (record unchanged) (0x000e31c0, 245 B), 92 instructions
__declspec(naked) unsigned __fastcall nxTM5155(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2)
	{
	__asm
		{
		sub esp, 0x18		// 0x000e31c0
		mov eax, dword ptr [esp + 0x20]		// 0x000e31c3
		fld dword ptr [eax]		// 0x000e31c7
		mov edx, dword ptr [esp + 0x1c]		// 0x000e31c9
		fsub dword ptr [edx]		// 0x000e31cd
		push esi		// 0x000e31cf
		fld dword ptr [eax + 4]		// 0x000e31d0
		fsub dword ptr [edx + 4]		// 0x000e31d3
		fld dword ptr [eax + 8]		// 0x000e31d6
		mov eax, dword ptr [esp + 0x28]		// 0x000e31d9
		fsub dword ptr [edx + 8]		// 0x000e31dd
		fld dword ptr [eax]		// 0x000e31e0
		fsub dword ptr [edx]		// 0x000e31e2
		fstp dword ptr [esp + 4]		// 0x000e31e4
		fld dword ptr [eax + 4]		// 0x000e31e8
		fsub dword ptr [edx + 4]		// 0x000e31eb
		fstp dword ptr [esp + 8]		// 0x000e31ee
		fld dword ptr [eax + 8]		// 0x000e31f2
		mov eax, ecx		// 0x000e31f5
		fsub dword ptr [edx + 8]		// 0x000e31f7
		fld st(0)		// 0x000e31fa
		fmul st(0), st(3)		// 0x000e31fc
		fld dword ptr [esp + 8]		// 0x000e31fe
		fmul st(0), st(3)		// 0x000e3202
		fsubp st(1), st(0)		// 0x000e3204
		fstp dword ptr [esp + 0x10]		// 0x000e3206
		mov esi, dword ptr [esp + 0x10]		// 0x000e320a
		fxch st(1)		// 0x000e320e
		mov dword ptr [eax], esi		// 0x000e3210
		fmul dword ptr [esp + 4]		// 0x000e3212
		fxch st(1)		// 0x000e3216
		fmul st(0), st(3)		// 0x000e3218
		fsubp st(1), st(0)		// 0x000e321a
		fstp dword ptr [esp + 0x14]		// 0x000e321c
		mov esi, dword ptr [esp + 0x14]		// 0x000e3220
		fld dword ptr [esp + 8]		// 0x000e3224
		mov dword ptr [eax + 4], esi		// 0x000e3228
		fmul st(0), st(2)		// 0x000e322b
		fld dword ptr [esp + 4]		// 0x000e322d
		fmul st(0), st(2)		// 0x000e3231
		fsubp st(1), st(0)		// 0x000e3233
		fstp dword ptr [esp + 0x18]		// 0x000e3235
		mov esi, dword ptr [esp + 0x18]		// 0x000e3239
		mov dword ptr [eax + 8], esi		// 0x000e323d
		fstp st(0)		// 0x000e3240
		pop esi		// 0x000e3242
		fstp st(0)		// 0x000e3243
		fld dword ptr [ecx + 8]		// 0x000e3245
		fld dword ptr [ecx + 4]		// 0x000e3248
		fld dword ptr [ecx]		// 0x000e324b
		fld st(0)		// 0x000e324d
		fmul st(0), st(1)		// 0x000e324f
		fld st(2)		// 0x000e3251
		fmul st(0), st(3)		// 0x000e3253
		faddp st(1), st(0)		// 0x000e3255
		fld st(3)		// 0x000e3257
		fmul st(0), st(4)		// 0x000e3259
		faddp st(1), st(0)		// 0x000e325b
		fstp st(3)		// 0x000e325d
		fstp st(0)		// 0x000e325f
		fstp st(0)		// 0x000e3261
		fld dword ptr [nxTMK101041f0_4]		// 0x000e3263
		fld st(1)		// 0x000e3269
		fucompp		// 0x000e326b
		fnstsw ax		// 0x000e326d
		test ah, 0x44		// 0x000e326f
		jnp Le3292		// 0x000e3272
		fsqrt		// 0x000e3274
		fdivr dword ptr [nxTMK101041ec_4]		// 0x000e3276
		fld st(0)		// 0x000e327c
		fmul dword ptr [ecx]		// 0x000e327e
		fstp dword ptr [ecx]		// 0x000e3280
		fld st(0)		// 0x000e3282
		fmul dword ptr [ecx + 4]		// 0x000e3284
		fstp dword ptr [ecx + 4]		// 0x000e3287
		fmul dword ptr [ecx + 8]		// 0x000e328a
		fstp dword ptr [ecx + 8]		// 0x000e328d
		jmp Le3294		// 0x000e3290
Le3292:
		fstp st(0)		// 0x000e3292
Le3294:
		fld dword ptr [edx + 8]		// 0x000e3294
		mov eax, ecx		// 0x000e3297
		fmul dword ptr [ecx + 8]		// 0x000e3299
		fld dword ptr [edx]		// 0x000e329c
		fmul dword ptr [ecx]		// 0x000e329e
		faddp st(1), st(0)		// 0x000e32a0
		fld dword ptr [edx + 4]		// 0x000e32a2
		fmul dword ptr [ecx + 4]		// 0x000e32a5
		faddp st(1), st(0)		// 0x000e32a8
		fchs		// 0x000e32aa
		fstp dword ptr [ecx + 0xc]		// 0x000e32ac
		add esp, 0x18		// 0x000e32af
		ret 0xc		// 0x000e32b2
		}
	}

// helper: phys_fn_005157 (record unchanged) (0x000e32c0, 27 B), 9 instructions
__declspec(naked) unsigned __fastcall nxTM5157(void* self, void* edxUnused)
	{
	__asm
		{
		mov eax, ecx		// 0x000e32c0
		xor ecx, ecx		// 0x000e32c2
		mov dword ptr [eax + 4], ecx		// 0x000e32c4
		mov dword ptr [eax + 8], ecx		// 0x000e32c7
		mov dword ptr [eax + 0xc], ecx		// 0x000e32ca
		mov dword ptr [eax + 0x10], ecx		// 0x000e32cd
		mov byte ptr [eax + 0x14], 1		// 0x000e32d0
		mov dword ptr [eax], 0x80000000		// 0x000e32d4
		ret		// 0x000e32da
		}
	}

// helper: phys_fn_005159 (record unchanged) (0x000e32e0, 72 B), 27 instructions
__declspec(naked) unsigned __fastcall nxTM5159(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x000e32e0
		mov esi, ecx		// 0x000e32e1
		mov al, byte ptr [esi + 0x14]		// 0x000e32e3
		test al, al		// 0x000e32e6
		je Le3326		// 0x000e32e8
		mov eax, dword ptr [esi + 8]		// 0x000e32ea
		test eax, eax		// 0x000e32ed
		je Le3308		// 0x000e32ef
		call nxTMCallNxGetSdkAllocator		// 0x000e32f1
		mov ecx, dword ptr [esi + 8]		// 0x000e32f6
		mov edx, dword ptr [eax]		// 0x000e32f9
		push ecx		// 0x000e32fb
		mov ecx, eax		// 0x000e32fc
		call dword ptr [edx + 0xc]		// 0x000e32fe
		mov dword ptr [esi + 8], 0		// 0x000e3301
Le3308:
		mov eax, dword ptr [esi + 4]		// 0x000e3308
		test eax, eax		// 0x000e330b
		je Le3326		// 0x000e330d
		call nxTMCallNxGetSdkAllocator		// 0x000e330f
		mov ecx, dword ptr [esi + 4]		// 0x000e3314
		mov edx, dword ptr [eax]		// 0x000e3317
		push ecx		// 0x000e3319
		mov ecx, eax		// 0x000e331a
		call dword ptr [edx + 0xc]		// 0x000e331c
		mov dword ptr [esi + 4], 0		// 0x000e331f
Le3326:
		pop esi		// 0x000e3326
		ret		// 0x000e3327
		}
	}
// @@ROWS-END@@
