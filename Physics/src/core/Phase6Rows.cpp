/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// Phase6Rows.cpp: Phase 6's remaining rows: phys_fn_004159, 004161 and 004169 with 004171 (an articulation's
// iterations), 004401 (a solver row beside 004403), 004405, 004413, 004537 (the abstract NxJoint's scalar deleting
// destructor), 004759 and 004761 (an event list's constructor and slot 5). The two internal tables they store (the
// oracle's 0x1011a648 and 0x1011b558) are built here, entry for entry, from rows transcribed as helpers (their records
// unchanged): the no-op returns 004248 and 004411 (001583 is IceSupportMaps.cpp's nxSupportMapNoop), the destructor chains 004415 and 004764 over 004409, the
// guarded dispatch 004763 and the list constructor 004407. Naked listing transcriptions. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase6-rows-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxP6CallFree();
extern "C" void nxP6CallRowslot6();
extern "C" void* nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll;
#pragma comment(linker, "/alternatename:_nxP6CallFree=_free")
#pragma comment(linker, "/alternatename:_nxP6CallRowslot6=?row_slot6@Joint@@UAEXM@Z")
#pragma comment(linker, "/alternatename:_nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll=__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A")
extern "C" void* nxSdkInstanceSlot;
#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")
extern "C" unsigned gNxApiReentry;
extern "C" const char* gNxApiReentryMessage;
extern "C" void* nxKnCompleteSorter;
#pragma comment(linker, "/alternatename:_nxKnCompleteSorter=?gCompleteSorter@@3PAVRadixSort@IceCore@@A")
extern "C" void* nxKnBipartiteSorter0;
#pragma comment(linker, "/alternatename:_nxKnBipartiteSorter0=?gBipartiteSorter0@@3PAVRadixSort@IceCore@@A")
extern "C" void* nxKnBipartiteSorter1;
#pragma comment(linker, "/alternatename:_nxKnBipartiteSorter1=?gBipartiteSorter1@@3PAVRadixSort@IceCore@@A")
extern "C" void* nxKnPrunerPool;
#pragma comment(linker, "/alternatename:_nxKnPrunerPool=?gNxPrunerProcessPool@@3PAVNxPrunerProcessPool@@A")
extern "C" void* nxKnOwnerQuery;
#pragma comment(linker, "/alternatename:_nxKnOwnerQuery=?gPrunableOwnerQuery@@3P6AIPAX@ZA")
extern "C" void* nxKnOwnerNotify;
#pragma comment(linker, "/alternatename:_nxKnOwnerNotify=?gPrunableOwnerNotify@@3P6AXPAXPAVAABB@IceMaths@@@ZA")
extern "C" void* nxKnOwnerWorldAABB;
#pragma comment(linker, "/alternatename:_nxKnOwnerWorldAABB=?gPrunableOwnerWorldAABB@@3P6AXPAXPAVAABB@IceMaths@@@ZA")
extern "C" void* nxKnAdapterNotify;
#pragma comment(linker, "/alternatename:_nxKnAdapterNotify=?gPrunableAdapterNotify@@3P6AXPAVPrunable@@PAVAABB@IceMaths@@@ZA")
extern NxReal gParameter[];
extern "C" void* nxKnVtNxJoint;
#pragma comment(linker, "/alternatename:_nxKnVtNxJoint=??_7NxJoint@@6B@")
extern "C" void* nxP6TableA[8];
extern "C" void* nxP6TableB[8];

unsigned __cdecl nxP64159();
unsigned __cdecl nxP64161();
unsigned __cdecl nxP64169();
unsigned __stdcall nxP64248(unsigned a0);
unsigned __cdecl nxP64401();
unsigned __cdecl nxP64405();
unsigned __fastcall nxP64407(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2);
unsigned __fastcall nxP64409(void* self, void* edxUnused);
unsigned __stdcall nxP64411(unsigned a0, unsigned a1, unsigned a2);
unsigned __fastcall nxP64413(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP64415(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP64537(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP64759(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2);
unsigned __fastcall nxP64761(void* self, void* edxUnused, unsigned a0);
unsigned __fastcall nxP64763(void* self, void* edxUnused);
unsigned __fastcall nxP64764(void* self, void* edxUnused, unsigned a0);
// @@PROTOS-END@@
extern "C" void nxP6Noop1583();
#pragma comment(linker, "/alternatename:_nxP6Noop1583=?nxSupportMapNoop@@YIXPAUIceSupportMap@@@Z")
// The two internal tables, entry for entry as the oracle has them (0x1011a648 and 0x1011b558).
extern "C" void* nxP6TableA[8] = { (void*) &nxP64415, (void*) &nxP64411, (void*) &nxP64248, (void*) &nxP64248, (void*) &nxP6Noop1583, (void*) &nxP64248, (void*) &nxP64413, (void*) &nxP64248 };
extern "C" void* nxP6TableB[8] = { (void*) &nxP64764, (void*) &nxP64411, (void*) &nxP64248, (void*) &nxP64248, (void*) &nxP64763, (void*) &nxP64761, (void*) &nxP64413, (void*) &nxP64248 };


static const unsigned nxP6K101041f0_4 = 0x0;

// phys_fn_004159 (0x0009ab00, 167 B), 63 instructions
__declspec(naked) unsigned __cdecl nxP64159()
	{
	__asm
		{
		sub esp, 8		// 0x0009ab00
		mov eax, dword ptr [esp + 0x14]		// 0x0009ab03
		mov ecx, dword ptr [esp + 0xc]		// 0x0009ab07
		push ebx		// 0x0009ab0b
		push ebp		// 0x0009ab0c
		push esi		// 0x0009ab0d
		push edi		// 0x0009ab0e
		mov edi, dword ptr [esp + 0x20]		// 0x0009ab0f
		mov ebp, dword ptr [edi + 0x1e4]		// 0x0009ab13
		push eax		// 0x0009ab19
		push edi		// 0x0009ab1a
		or ebp, 8		// 0x0009ab1b
		push ecx		// 0x0009ab1e
		mov dword ptr [edi + 0x1e4], ebp		// 0x0009ab1f
		call nxP64405		// 0x0009ab25
		mov edx, dword ptr [edi + 0x1d8]		// 0x0009ab2a
		mov ebx, eax		// 0x0009ab30
		mov eax, dword ptr [edi + 0x1dc]		// 0x0009ab32
		add esp, 0xc		// 0x0009ab38
		mov dword ptr [esp + 0x10], edx		// 0x0009ab3b
		mov dword ptr [esp + 0x14], eax		// 0x0009ab3f
		xor ebp, ebp		// 0x0009ab43
L9ab45:
		mov esi, dword ptr [esp + ebp*4 + 0x10]		// 0x0009ab45
		test esi, esi		// 0x0009ab49
		je L9ab97		// 0x0009ab4b
		lea ecx, [ecx]		// 0x0009ab4d
L9ab50:
		cmp esi, dword ptr [esp + 0x24]		// 0x0009ab50
		je L9ab87		// 0x0009ab54
		mov eax, dword ptr [esi + 8]		// 0x0009ab56
		cmp edi, eax		// 0x0009ab59
		jne L9ab60		// 0x0009ab5b
		mov eax, dword ptr [esi + 0xc]		// 0x0009ab5d
L9ab60:
		test eax, eax		// 0x0009ab60
		je L9ab87		// 0x0009ab62
		mov cl, byte ptr [eax + 0x10c]		// 0x0009ab64
		test cl, cl		// 0x0009ab6a
		js L9ab87		// 0x0009ab6c
		test byte ptr [eax + 0x1e4], 8		// 0x0009ab6e
		jne L9ab87		// 0x0009ab75
		mov ecx, dword ptr [esp + 0x28]		// 0x0009ab77
		push ecx		// 0x0009ab7b
		push esi		// 0x0009ab7c
		push eax		// 0x0009ab7d
		push ebx		// 0x0009ab7e
		call nxP64159		// 0x0009ab7f
		add esp, 0x10		// 0x0009ab84
L9ab87:
		test ebp, ebp		// 0x0009ab87
		jne L9ab90		// 0x0009ab89
		mov esi, dword ptr [esi + 0x34]		// 0x0009ab8b
		jmp L9ab93		// 0x0009ab8e
L9ab90:
		mov esi, dword ptr [esi + 0x38]		// 0x0009ab90
L9ab93:
		test esi, esi		// 0x0009ab93
		jne L9ab50		// 0x0009ab95
L9ab97:
		inc ebp		// 0x0009ab97
		cmp ebp, 2		// 0x0009ab98
		jb L9ab45		// 0x0009ab9b
		pop edi		// 0x0009ab9d
		pop esi		// 0x0009ab9e
		pop ebp		// 0x0009ab9f
		mov eax, ebx		// 0x0009aba0
		pop ebx		// 0x0009aba2
		add esp, 8		// 0x0009aba3
		ret		// 0x0009aba6
		}
	}

// phys_fn_004161 (0x0009abb0, 226 B), 88 instructions
__declspec(naked) unsigned __cdecl nxP64161()
	{
	__asm
		{
		mov edx, dword ptr [esp + 4]		// 0x0009abb0
		sub esp, 0x20		// 0x0009abb4
		push ebx		// 0x0009abb7
		push ebp		// 0x0009abb8
		push esi		// 0x0009abb9
		push edi		// 0x0009abba
		mov edi, dword ptr [esp + 0x38]		// 0x0009abbb
		nop		// 0x0009abbf
L9abc0:
		mov ecx, edi		// 0x0009abc0
		sub ecx, edx		// 0x0009abc2
		sar ecx, 6		// 0x0009abc4
		shl ecx, 4		// 0x0009abc7
		add ecx, edx		// 0x0009abca
		cmp edx, edi		// 0x0009abcc
		mov ebx, dword ptr [ecx]		// 0x0009abce
		mov dword ptr [esp + 0x10], ebx		// 0x0009abd0
		mov ebx, dword ptr [ecx + 4]		// 0x0009abd4
		mov dword ptr [esp + 0x14], ebx		// 0x0009abd7
		mov ebx, dword ptr [ecx + 8]		// 0x0009abdb
		mov ecx, dword ptr [ecx + 0xc]		// 0x0009abde
		mov esi, edx		// 0x0009abe1
		mov eax, edi		// 0x0009abe3
		mov dword ptr [esp + 0x18], ebx		// 0x0009abe5
		mov dword ptr [esp + 0x1c], ecx		// 0x0009abe9
		ja L9ac6e		// 0x0009abed
		nop		// 0x0009abef
L9abf0:
		cmp dword ptr [esi + 0xc], ecx		// 0x0009abf0
		jbe L9abff		// 0x0009abf3
L9abf5:
		mov ebx, dword ptr [esi + 0x1c]		// 0x0009abf5
		add esi, 0x10		// 0x0009abf8
		cmp ebx, ecx		// 0x0009abfb
		ja L9abf5		// 0x0009abfd
L9abff:
		cmp ecx, dword ptr [eax + 0xc]		// 0x0009abff
		jbe L9ac0e		// 0x0009ac02
L9ac04:
		mov ebx, dword ptr [eax - 4]		// 0x0009ac04
		sub eax, 0x10		// 0x0009ac07
		cmp ecx, ebx		// 0x0009ac0a
		ja L9ac04		// 0x0009ac0c
L9ac0e:
		cmp esi, eax		// 0x0009ac0e
		ja L9ac6e		// 0x0009ac10
		je L9ac64		// 0x0009ac12
		mov edi, esi		// 0x0009ac14
		mov ecx, dword ptr [edi]		// 0x0009ac16
		mov edx, dword ptr [edi + 4]		// 0x0009ac18
		mov ebx, dword ptr [edi + 8]		// 0x0009ac1b
		mov edi, dword ptr [edi + 0xc]		// 0x0009ac1e
		mov dword ptr [esp + 0x2c], edi		// 0x0009ac21
		mov dword ptr [esp + 0x28], ebx		// 0x0009ac25
		mov edi, eax		// 0x0009ac29
		mov ebp, dword ptr [edi]		// 0x0009ac2b
		mov ebx, esi		// 0x0009ac2d
		mov dword ptr [ebx], ebp		// 0x0009ac2f
		mov ebp, dword ptr [edi + 4]		// 0x0009ac31
		mov dword ptr [ebx + 4], ebp		// 0x0009ac34
		mov ebp, dword ptr [edi + 8]		// 0x0009ac37
		mov dword ptr [ebx + 8], ebp		// 0x0009ac3a
		mov edi, dword ptr [edi + 0xc]		// 0x0009ac3d
		mov dword ptr [ebx + 0xc], edi		// 0x0009ac40
		mov edi, eax		// 0x0009ac43
		mov dword ptr [edi], ecx		// 0x0009ac45
		mov ecx, dword ptr [esp + 0x2c]		// 0x0009ac47
		mov dword ptr [edi + 4], edx		// 0x0009ac4b
		mov edx, dword ptr [esp + 0x28]		// 0x0009ac4e
		mov dword ptr [edi + 8], edx		// 0x0009ac52
		mov edx, dword ptr [esp + 0x34]		// 0x0009ac55
		mov dword ptr [edi + 0xc], ecx		// 0x0009ac59
		mov edi, dword ptr [esp + 0x38]		// 0x0009ac5c
		mov ecx, dword ptr [esp + 0x1c]		// 0x0009ac60
L9ac64:
		add esi, 0x10		// 0x0009ac64
		sub eax, 0x10		// 0x0009ac67
		cmp esi, eax		// 0x0009ac6a
		jbe L9abf0		// 0x0009ac6c
L9ac6e:
		cmp edx, eax		// 0x0009ac6e
		jae L9ac7c		// 0x0009ac70
		push eax		// 0x0009ac72
		push edx		// 0x0009ac73
		call nxP64161		// 0x0009ac74
		add esp, 8		// 0x0009ac79
L9ac7c:
		cmp esi, edi		// 0x0009ac7c
		mov edx, esi		// 0x0009ac7e
		mov dword ptr [esp + 0x34], edx		// 0x0009ac80
		jb L9abc0		// 0x0009ac84
		pop edi		// 0x0009ac8a
		pop esi		// 0x0009ac8b
		pop ebp		// 0x0009ac8c
		pop ebx		// 0x0009ac8d
		add esp, 0x20		// 0x0009ac8e
		ret		// 0x0009ac91
		}
	}

// phys_fn_004169 and phys_fn_004171 (one function) (0x0009adb0, 800 B), 274 instructions
__declspec(naked) unsigned __cdecl nxP64169()
	{
	__asm
		{
		sub esp, 0x24		// 0x0009adb0
		mov eax, dword ptr [esp + 0x28]		// 0x0009adb3
		push ebx		// 0x0009adb7
		push ebp		// 0x0009adb8
		push esi		// 0x0009adb9
		push edi		// 0x0009adba
		xor edi, edi		// 0x0009adbb
		xor ebp, ebp		// 0x0009adbd
		cmp eax, edi		// 0x0009adbf
		mov ebx, eax		// 0x0009adc1
		mov dword ptr [esp + 0x14], edi		// 0x0009adc3
		mov dword ptr [esp + 0x18], ebp		// 0x0009adc7
		mov dword ptr [esp + 0x1c], edi		// 0x0009adcb
		mov dword ptr [esp + 0x38], ebx		// 0x0009adcf
		je L9af7d		// 0x0009add3
		lea esp, [esp]		// 0x0009add9
L9ade0:
		mov edx, dword ptr [ebx + 0x1e4]		// 0x0009ade0
		mov eax, dword ptr [ebx + 0x1d8]		// 0x0009ade6
		and edx, 0xfffffff7		// 0x0009adec
		mov dword ptr [ebx + 0x1e4], edx		// 0x0009adef
		xor edx, edx		// 0x0009adf5
		cmp eax, edi		// 0x0009adf7
		mov dword ptr [esp + 0x30], edx		// 0x0009adf9
		mov dword ptr [esp + 0x2c], edi		// 0x0009adfd
		je L9ae41		// 0x0009ae01
L9ae03:
		mov ecx, dword ptr [eax + 8]		// 0x0009ae03
		cmp ebx, ecx		// 0x0009ae06
		jne L9ae0d		// 0x0009ae08
		mov ecx, dword ptr [eax + 0xc]		// 0x0009ae0a
L9ae0d:
		cmp ecx, edi		// 0x0009ae0d
		je L9ae1a		// 0x0009ae0f
		test byte ptr [ecx + 0x10c], 0x80		// 0x0009ae11
		je L9ae30		// 0x0009ae18
L9ae1a:
		mov ecx, dword ptr [eax + 0x44]		// 0x0009ae1a
		add edx, 0x186a0		// 0x0009ae1d
		cmp ecx, edi		// 0x0009ae23
		je L9ae2a		// 0x0009ae25
		add edx, 0x64		// 0x0009ae27
L9ae2a:
		mov dword ptr [esp + 0x2c], eax		// 0x0009ae2a
		jmp L9ae36		// 0x0009ae2e
L9ae30:
		add edx, 0x3e8		// 0x0009ae30
L9ae36:
		mov eax, dword ptr [eax + 0x34]		// 0x0009ae36
		cmp eax, edi		// 0x0009ae39
		jne L9ae03		// 0x0009ae3b
		mov dword ptr [esp + 0x30], edx		// 0x0009ae3d
L9ae41:
		mov eax, dword ptr [ebx + 0x1dc]		// 0x0009ae41
		cmp eax, edi		// 0x0009ae47
		je L9ae8e		// 0x0009ae49
		jmp L9ae50		// 0x0009ae4b
		lea ecx, [ecx]		// 0x0009ae4d
L9ae50:
		mov ecx, dword ptr [eax + 8]		// 0x0009ae50
		cmp ebx, ecx		// 0x0009ae53
		jne L9ae5a		// 0x0009ae55
		mov ecx, dword ptr [eax + 0xc]		// 0x0009ae57
L9ae5a:
		cmp ecx, edi		// 0x0009ae5a
		je L9ae67		// 0x0009ae5c
		test byte ptr [ecx + 0x10c], 0x80		// 0x0009ae5e
		je L9ae7d		// 0x0009ae65
L9ae67:
		mov ecx, dword ptr [eax + 0x44]		// 0x0009ae67
		add edx, 0x186a0		// 0x0009ae6a
		cmp ecx, edi		// 0x0009ae70
		je L9ae77		// 0x0009ae72
		add edx, 0x64		// 0x0009ae74
L9ae77:
		mov dword ptr [esp + 0x2c], eax		// 0x0009ae77
		jmp L9ae83		// 0x0009ae7b
L9ae7d:
		add edx, 0x3e8		// 0x0009ae7d
L9ae83:
		mov eax, dword ptr [eax + 0x38]		// 0x0009ae83
		cmp eax, edi		// 0x0009ae86
		jne L9ae50		// 0x0009ae88
		mov dword ptr [esp + 0x30], edx		// 0x0009ae8a
L9ae8e:
		cmp dword ptr [esp + 0x1c], ebp		// 0x0009ae8e
		ja L9af51		// 0x0009ae92
		mov ecx, dword ptr [esp + 0x14]		// 0x0009ae98
		mov esi, ebp		// 0x0009ae9c
		sub esi, ecx		// 0x0009ae9e
		sar esi, 4		// 0x0009aea0
		cmp ecx, edi		// 0x0009aea3
		lea eax, [esi + esi + 2]		// 0x0009aea5
		jne L9aeaf		// 0x0009aea9
		xor ecx, ecx		// 0x0009aeab
		jmp L9aec0		// 0x0009aead
L9aeaf:
		mov ebp, dword ptr [esp + 0x14]		// 0x0009aeaf
		mov ecx, dword ptr [esp + 0x1c]		// 0x0009aeb3
		sub ecx, ebp		// 0x0009aeb7
		mov ebp, dword ptr [esp + 0x18]		// 0x0009aeb9
		sar ecx, 4		// 0x0009aebd
L9aec0:
		cmp ecx, eax		// 0x0009aec0
		jae L9af51		// 0x0009aec2
		shl eax, 4		// 0x0009aec8
		mov ebp, eax		// 0x0009aecb
		mov eax, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0009aecd
		mov ecx, dword ptr [eax]		// 0x0009aed2
		mov edx, dword ptr [ecx]		// 0x0009aed4
		push edi		// 0x0009aed6
		push ebp		// 0x0009aed7
		call dword ptr [edx + 8]		// 0x0009aed8
		mov ecx, dword ptr [esp + 0x18]		// 0x0009aedb
		mov edi, eax		// 0x0009aedf
		mov eax, dword ptr [esp + 0x14]		// 0x0009aee1
		cmp eax, ecx		// 0x0009aee5
		je L9af24		// 0x0009aee7
		mov ecx, edi		// 0x0009aee9
		sub ecx, eax		// 0x0009aeeb
		mov dword ptr [esp + 0x10], ecx		// 0x0009aeed
		jmp L9aef7		// 0x0009aef1
L9aef3:
		mov ecx, dword ptr [esp + 0x10]		// 0x0009aef3
L9aef7:
		add ecx, eax		// 0x0009aef7
		mov edx, eax		// 0x0009aef9
		mov ebx, dword ptr [edx]		// 0x0009aefb
		mov dword ptr [ecx], ebx		// 0x0009aefd
		mov ebx, dword ptr [edx + 4]		// 0x0009aeff
		mov dword ptr [ecx + 4], ebx		// 0x0009af02
		mov ebx, dword ptr [edx + 8]		// 0x0009af05
		mov dword ptr [ecx + 8], ebx		// 0x0009af08
		mov edx, dword ptr [edx + 0xc]		// 0x0009af0b
		mov dword ptr [ecx + 0xc], edx		// 0x0009af0e
		mov ecx, dword ptr [esp + 0x18]		// 0x0009af11
		add eax, 0x10		// 0x0009af15
		cmp eax, ecx		// 0x0009af18
		jne L9aef3		// 0x0009af1a
		mov ebx, dword ptr [esp + 0x38]		// 0x0009af1c
		mov eax, dword ptr [esp + 0x14]		// 0x0009af20
L9af24:
		test eax, eax		// 0x0009af24
		je L9af36		// 0x0009af26
		mov ecx, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0009af28
		mov ecx, dword ptr [ecx]		// 0x0009af2e
		mov edx, dword ptr [ecx]		// 0x0009af30
		push eax		// 0x0009af32
		call dword ptr [edx + 0x14]		// 0x0009af33
L9af36:
		mov edx, dword ptr [esp + 0x30]		// 0x0009af36
		add ebp, edi		// 0x0009af3a
		shl esi, 4		// 0x0009af3c
		add esi, edi		// 0x0009af3f
		mov dword ptr [esp + 0x1c], ebp		// 0x0009af41
		mov dword ptr [esp + 0x14], edi		// 0x0009af45
		mov dword ptr [esp + 0x18], esi		// 0x0009af49
		xor edi, edi		// 0x0009af4d
		mov ebp, esi		// 0x0009af4f
L9af51:
		mov ecx, ebp		// 0x0009af51
		mov dword ptr [ecx], ebx		// 0x0009af53
		xor eax, eax		// 0x0009af55
		mov dword ptr [ecx + 4], eax		// 0x0009af57
		mov eax, dword ptr [esp + 0x2c]		// 0x0009af5a
		mov dword ptr [ecx + 8], eax		// 0x0009af5e
		mov dword ptr [ecx + 0xc], edx		// 0x0009af61
		mov ebx, dword ptr [ebx + 0x1d0]		// 0x0009af64
		add ebp, 0x10		// 0x0009af6a
		cmp ebx, edi		// 0x0009af6d
		mov dword ptr [esp + 0x18], ebp		// 0x0009af6f
		mov dword ptr [esp + 0x38], ebx		// 0x0009af73
		jne L9ade0		// 0x0009af77
L9af7d:
		mov edx, dword ptr [esp + 0x14]		// 0x0009af7d
		lea ecx, [ebp - 0x10]		// 0x0009af81
		push ecx		// 0x0009af84
		push edx		// 0x0009af85
		call nxP64161		// 0x0009af86
		mov ebx, dword ptr [esp + 0x1c]		// 0x0009af8b
		add esp, 8		// 0x0009af8f
		xor esi, esi		// 0x0009af92
		cmp ebx, ebp		// 0x0009af94
		mov dword ptr [esp + 0x10], ebx		// 0x0009af96
		je L9b0b5		// 0x0009af9a
L9afa0:
		mov eax, dword ptr [ebx]		// 0x0009afa0
		test byte ptr [eax + 0x1e4], 8		// 0x0009afa2
		jne L9b0a2		// 0x0009afa9
		mov ecx, dword ptr [ebx + 8]		// 0x0009afaf
		lea edx, [esp + 0x38]		// 0x0009afb2
		push edx		// 0x0009afb6
		push ecx		// 0x0009afb7
		push eax		// 0x0009afb8
		push edi		// 0x0009afb9
		mov byte ptr [esp + 0x48], 0		// 0x0009afba
		call nxP64159		// 0x0009afbf
		mov ebp, eax		// 0x0009afc4
		add esp, 0x10		// 0x0009afc6
		cmp ebp, edi		// 0x0009afc9
		mov dword ptr [esp + 0x38], ebp		// 0x0009afcb
		je L9b09e		// 0x0009afcf
		cmp esi, edi		// 0x0009afd5
		jne L9b003		// 0x0009afd7
		mov eax, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0009afd9
		mov ecx, dword ptr [eax]		// 0x0009afde
		mov edx, dword ptr [ecx]		// 0x0009afe0
		push edi		// 0x0009afe2
		push 0x20		// 0x0009afe3
		call dword ptr [edx + 8]		// 0x0009afe5
		cmp eax, edi		// 0x0009afe8
		je L9b001		// 0x0009afea
		mov dword ptr [eax], edi		// 0x0009afec
		mov dword ptr [eax + 4], edi		// 0x0009afee
		mov dword ptr [eax + 8], edi		// 0x0009aff1
		mov dword ptr [eax + 0x10], edi		// 0x0009aff4
		mov dword ptr [eax + 0x14], edi		// 0x0009aff7
		mov dword ptr [eax + 0x18], edi		// 0x0009affa
		mov esi, eax		// 0x0009affd
		jmp L9b003		// 0x0009afff
L9b001:
		xor esi, esi		// 0x0009b001
L9b003:
		mov edx, dword ptr [esi + 0x18]		// 0x0009b003
		mov eax, dword ptr [esi + 0x14]		// 0x0009b006
		cmp edx, eax		// 0x0009b009
		ja L9b095		// 0x0009b00b
		mov ecx, dword ptr [esi + 0x10]		// 0x0009b011
		sub eax, ecx		// 0x0009b014
		sar eax, 2		// 0x0009b016
		cmp ecx, edi		// 0x0009b019
		lea eax, [eax + eax + 2]		// 0x0009b01b
		jne L9b025		// 0x0009b01f
		xor edx, edx		// 0x0009b021
		jmp L9b02a		// 0x0009b023
L9b025:
		sub edx, ecx		// 0x0009b025
		sar edx, 2		// 0x0009b027
L9b02a:
		cmp edx, eax		// 0x0009b02a
		jae L9b095		// 0x0009b02c
		lea ebp, [eax*4]		// 0x0009b02e
		mov eax, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0009b035
		mov ecx, dword ptr [eax]		// 0x0009b03a
		mov edx, dword ptr [ecx]		// 0x0009b03c
		push edi		// 0x0009b03e
		push ebp		// 0x0009b03f
		call dword ptr [edx + 8]		// 0x0009b040
		mov ecx, dword ptr [esi + 0x14]		// 0x0009b043
		mov edi, eax		// 0x0009b046
		mov eax, dword ptr [esi + 0x10]		// 0x0009b048
		cmp eax, ecx		// 0x0009b04b
		mov edx, edi		// 0x0009b04d
		je L9b063		// 0x0009b04f
L9b051:
		mov ebx, dword ptr [eax]		// 0x0009b051
		mov dword ptr [edx], ebx		// 0x0009b053
		add eax, 4		// 0x0009b055
		add edx, 4		// 0x0009b058
		cmp eax, ecx		// 0x0009b05b
		jne L9b051		// 0x0009b05d
		mov ebx, dword ptr [esp + 0x10]		// 0x0009b05f
L9b063:
		mov eax, dword ptr [esi + 0x10]		// 0x0009b063
		test eax, eax		// 0x0009b066
		je L9b078		// 0x0009b068
		mov ecx, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0009b06a
		mov ecx, dword ptr [ecx]		// 0x0009b070
		mov edx, dword ptr [ecx]		// 0x0009b072
		push eax		// 0x0009b074
		call dword ptr [edx + 0x14]		// 0x0009b075
L9b078:
		mov eax, dword ptr [esi + 0x14]		// 0x0009b078
		sub eax, dword ptr [esi + 0x10]		// 0x0009b07b
		add ebp, edi		// 0x0009b07e
		sar eax, 2		// 0x0009b080
		lea ecx, [edi + eax*4]		// 0x0009b083
		mov dword ptr [esi + 0x18], ebp		// 0x0009b086
		mov ebp, dword ptr [esp + 0x38]		// 0x0009b089
		mov dword ptr [esi + 0x10], edi		// 0x0009b08d
		mov dword ptr [esi + 0x14], ecx		// 0x0009b090
		xor edi, edi		// 0x0009b093
L9b095:
		mov edx, dword ptr [esi + 0x14]		// 0x0009b095
		mov dword ptr [edx], ebp		// 0x0009b098
		add dword ptr [esi + 0x14], 4		// 0x0009b09a
L9b09e:
		mov ebp, dword ptr [esp + 0x18]		// 0x0009b09e
L9b0a2:
		add ebx, 0x10		// 0x0009b0a2
		cmp ebx, ebp		// 0x0009b0a5
		mov dword ptr [esp + 0x10], ebx		// 0x0009b0a7
		jne L9afa0		// 0x0009b0ab
		mov ebx, dword ptr [esp + 0x14]		// 0x0009b0b1
L9b0b5:
		cmp ebx, edi		// 0x0009b0b5
		je L9b0c6		// 0x0009b0b7
		mov eax, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0009b0b9
		mov ecx, dword ptr [eax]		// 0x0009b0be
		mov edx, dword ptr [ecx]		// 0x0009b0c0
		push ebx		// 0x0009b0c2
		call dword ptr [edx + 0x14]		// 0x0009b0c3
L9b0c6:
		pop edi		// 0x0009b0c6
		mov eax, esi		// 0x0009b0c7
		pop esi		// 0x0009b0c9
		pop ebp		// 0x0009b0ca
		pop ebx		// 0x0009b0cb
		add esp, 0x24		// 0x0009b0cc
		ret		// 0x0009b0cf
		}
	}

// helper: phys_fn_004248 (record unchanged) (0x000a0f60, 3 B), 1 instructions
__declspec(naked) unsigned __stdcall nxP64248(unsigned a0)
	{
	__asm
		{
		ret 4		// 0x000a0f60
		}
	}

// phys_fn_004401 (0x000afcc0, 794 B), 270 instructions
__declspec(naked) unsigned __cdecl nxP64401()
	{
	__asm
		{
		mov edx, dword ptr [esp + 0xc]		// 0x000afcc0
		sub esp, 0x30		// 0x000afcc4
		push ebx		// 0x000afcc7
		mov ebx, dword ptr [edx + 0xc]		// 0x000afcc8
		test bl, 0x20		// 0x000afccb
		je Lafced		// 0x000afcce
		and ebx, 0x1f		// 0x000afcd0
		cmp bl, 6		// 0x000afcd3
		jne Laffd5		// 0x000afcd6
		mov ecx, dword ptr [edx + 0x30]		// 0x000afcdc
		mov edx, dword ptr [esp + 0x38]		// 0x000afcdf
		mov eax, dword ptr [ecx]		// 0x000afce3
		push edx		// 0x000afce5
		call dword ptr [eax]		// 0x000afce6
		pop ebx		// 0x000afce8
		add esp, 0x30		// 0x000afce9
		ret		// 0x000afcec
Lafced:
		mov ecx, dword ptr [edx + 0x10]		// 0x000afced
		test ecx, ecx		// 0x000afcf0
		je Lafd22		// 0x000afcf2
		fld dword ptr [ecx + 0x18]		// 0x000afcf4
		fmul dword ptr [edx + 0x20]		// 0x000afcf7
		fld dword ptr [ecx + 0x14]		// 0x000afcfa
		fmul dword ptr [edx + 0x1c]		// 0x000afcfd
		faddp st(1), st(0)		// 0x000afd00
		fld dword ptr [ecx + 8]		// 0x000afd02
		fmul dword ptr [edx + 8]		// 0x000afd05
		faddp st(1), st(0)		// 0x000afd08
		fld dword ptr [ecx + 4]		// 0x000afd0a
		fmul dword ptr [edx + 4]		// 0x000afd0d
		faddp st(1), st(0)		// 0x000afd10
		fld dword ptr [ecx]		// 0x000afd12
		fmul dword ptr [edx]		// 0x000afd14
		faddp st(1), st(0)		// 0x000afd16
		fld dword ptr [ecx + 0x10]		// 0x000afd18
		fmul dword ptr [edx + 0x18]		// 0x000afd1b
		faddp st(1), st(0)		// 0x000afd1e
		jmp Lafd28		// 0x000afd20
Lafd22:
		fld dword ptr [nxP6K101041f0_4]		// 0x000afd22
Lafd28:
		mov eax, dword ptr [edx + 0x14]		// 0x000afd28
		test eax, eax		// 0x000afd2b
		je Lafd5d		// 0x000afd2d
		fld dword ptr [eax + 0x18]		// 0x000afd2f
		fmul dword ptr [edx + 0x2c]		// 0x000afd32
		fld dword ptr [eax + 0x14]		// 0x000afd35
		fmul dword ptr [edx + 0x28]		// 0x000afd38
		faddp st(1), st(0)		// 0x000afd3b
		fld dword ptr [eax + 8]		// 0x000afd3d
		fmul dword ptr [edx + 8]		// 0x000afd40
		faddp st(1), st(0)		// 0x000afd43
		fld dword ptr [eax + 4]		// 0x000afd45
		fmul dword ptr [edx + 4]		// 0x000afd48
		faddp st(1), st(0)		// 0x000afd4b
		fld dword ptr [edx + 0x24]		// 0x000afd4d
		fmul dword ptr [eax + 0x10]		// 0x000afd50
		faddp st(1), st(0)		// 0x000afd53
		fld dword ptr [eax]		// 0x000afd55
		fmul dword ptr [edx]		// 0x000afd57
		faddp st(1), st(0)		// 0x000afd59
		fsubp st(1), st(0)		// 0x000afd5b
Lafd5d:
		fadd dword ptr [edx + 0x34]		// 0x000afd5d
		fmul dword ptr [edx + 0x3c]		// 0x000afd60
		fchs		// 0x000afd63
		fst dword ptr [esp + 0x40]		// 0x000afd65
		fadd dword ptr [edx + 0x44]		// 0x000afd69
		fld st(0)		// 0x000afd6c
		fabs		// 0x000afd6e
		fcom dword ptr [edx + 0x48]		// 0x000afd70
		fnstsw ax		// 0x000afd73
		test ah, 0x41		// 0x000afd75
		jne Lafd97		// 0x000afd78
		fld dword ptr [edx + 0x4c]		// 0x000afd7a
		or ebx, 0x40		// 0x000afd7d
		fdiv st(0), st(1)		// 0x000afd80
		mov dword ptr [edx + 0xc], ebx		// 0x000afd82
		fmul dword ptr [edx + 0x48]		// 0x000afd85
		fmulp st(2), st(0)		// 0x000afd88
		fstp st(0)		// 0x000afd8a
		fld st(0)		// 0x000afd8c
		fsub dword ptr [edx + 0x44]		// 0x000afd8e
		fstp dword ptr [esp + 0x40]		// 0x000afd91
		jmp Lafd99		// 0x000afd95
Lafd97:
		fstp st(0)		// 0x000afd97
Lafd99:
		fstp dword ptr [edx + 0x44]		// 0x000afd99
		fld dword ptr [nxP6K101041f0_4]		// 0x000afd9c
		fld dword ptr [esp + 0x40]		// 0x000afda2
		fucompp		// 0x000afda6
		fnstsw ax		// 0x000afda8
		test ah, 0x44		// 0x000afdaa
		jnp Laffba		// 0x000afdad
		test ecx, ecx		// 0x000afdb3
		fld dword ptr [esp + 0x40]		// 0x000afdb5
		fmul dword ptr [edx]		// 0x000afdb9
		fld dword ptr [esp + 0x40]		// 0x000afdbb
		fmul dword ptr [edx + 4]		// 0x000afdbf
		fstp dword ptr [esp + 8]		// 0x000afdc2
		fld dword ptr [esp + 0x40]		// 0x000afdc6
		fmul dword ptr [edx + 8]		// 0x000afdca
		fstp dword ptr [esp + 0xc]		// 0x000afdcd
		je Lafeae		// 0x000afdd1
		fld dword ptr [edx + 0x18]		// 0x000afdd7
		mov eax, dword ptr [edx + 0x20]		// 0x000afdda
		fld dword ptr [edx + 0x1c]		// 0x000afddd
		mov dword ptr [esp + 0x30], eax		// 0x000afde0
		fxch st(1)		// 0x000afde4
		fmul dword ptr [esp + 0x40]		// 0x000afde6
		fstp dword ptr [esp + 0x28]		// 0x000afdea
		fmul dword ptr [esp + 0x40]		// 0x000afdee
		fld dword ptr [esp + 0x30]		// 0x000afdf2
		fmul dword ptr [esp + 0x40]		// 0x000afdf6
		fstp dword ptr [esp + 0x30]		// 0x000afdfa
		fld dword ptr [nxP6K101041f0_4]		// 0x000afdfe
		fld dword ptr [ecx + 0xc]		// 0x000afe04
		fucompp		// 0x000afe07
		fnstsw ax		// 0x000afe09
		test ah, 0x44		// 0x000afe0b
		jnp Lafeac		// 0x000afe0e
		fld dword ptr [ecx + 0xc]		// 0x000afe14
		fld st(0)		// 0x000afe17
		fmul st(0), st(3)		// 0x000afe19
		fld st(1)		// 0x000afe1b
		fmul dword ptr [esp + 8]		// 0x000afe1d
		fstp dword ptr [esp + 0x14]		// 0x000afe21
		fld dword ptr [esp + 0xc]		// 0x000afe25
		fmul st(0), st(2)		// 0x000afe29
		fstp dword ptr [esp + 0x18]		// 0x000afe2b
		fadd dword ptr [ecx]		// 0x000afe2f
		fstp dword ptr [ecx]		// 0x000afe31
		fstp st(0)		// 0x000afe33
		fld dword ptr [esp + 0x14]		// 0x000afe35
		fadd dword ptr [ecx + 4]		// 0x000afe39
		fstp dword ptr [ecx + 4]		// 0x000afe3c
		fld dword ptr [esp + 0x18]		// 0x000afe3f
		fadd dword ptr [ecx + 8]		// 0x000afe43
		fstp dword ptr [ecx + 8]		// 0x000afe46
		fld dword ptr [esp + 0x30]		// 0x000afe49
		fmul dword ptr [ecx + 0x28]		// 0x000afe4d
		fld st(1)		// 0x000afe50
		fmul dword ptr [ecx + 0x24]		// 0x000afe52
		faddp st(1), st(0)		// 0x000afe55
		fld dword ptr [esp + 0x28]		// 0x000afe57
		fmul dword ptr [ecx + 0x20]		// 0x000afe5b
		faddp st(1), st(0)		// 0x000afe5e
		fld dword ptr [esp + 0x30]		// 0x000afe60
		fmul dword ptr [ecx + 0x34]		// 0x000afe64
		fld st(2)		// 0x000afe67
		fmul dword ptr [ecx + 0x30]		// 0x000afe69
		faddp st(1), st(0)		// 0x000afe6c
		fld dword ptr [esp + 0x28]		// 0x000afe6e
		fmul dword ptr [ecx + 0x2c]		// 0x000afe72
		faddp st(1), st(0)		// 0x000afe75
		fld dword ptr [esp + 0x30]		// 0x000afe77
		fmul dword ptr [ecx + 0x40]		// 0x000afe7b
		fxch st(3)		// 0x000afe7e
		fmul dword ptr [ecx + 0x3c]		// 0x000afe80
		faddp st(3), st(0)		// 0x000afe83
		fld dword ptr [esp + 0x28]		// 0x000afe85
		fmul dword ptr [ecx + 0x38]		// 0x000afe89
		faddp st(3), st(0)		// 0x000afe8c
		fxch st(2)		// 0x000afe8e
		fstp dword ptr [esp + 0x18]		// 0x000afe90
		fadd dword ptr [ecx + 0x10]		// 0x000afe94
		fstp dword ptr [ecx + 0x10]		// 0x000afe97
		fadd dword ptr [ecx + 0x14]		// 0x000afe9a
		fstp dword ptr [ecx + 0x14]		// 0x000afe9d
		fld dword ptr [esp + 0x18]		// 0x000afea0
		fadd dword ptr [ecx + 0x18]		// 0x000afea4
		fstp dword ptr [ecx + 0x18]		// 0x000afea7
		jmp Lafeae		// 0x000afeaa
Lafeac:
		fstp st(0)		// 0x000afeac
Lafeae:
		mov ecx, dword ptr [edx + 0x14]		// 0x000afeae
		test ecx, ecx		// 0x000afeb1
		je Laffb8		// 0x000afeb3
		fchs		// 0x000afeb9
		mov eax, dword ptr [edx + 0x2c]		// 0x000afebb
		fstp dword ptr [esp + 0x10]		// 0x000afebe
		mov dword ptr [esp + 0x30], eax		// 0x000afec2
		fld dword ptr [esp + 8]		// 0x000afec6
		fchs		// 0x000afeca
		fld dword ptr [esp + 0xc]		// 0x000afecc
		fchs		// 0x000afed0
		fstp dword ptr [esp + 0x18]		// 0x000afed2
		fld dword ptr [edx + 0x24]		// 0x000afed6
		fld dword ptr [edx + 0x28]		// 0x000afed9
		fld dword ptr [esp + 0x40]		// 0x000afedc
		fchs		// 0x000afee0
		fst dword ptr [esp + 0x40]		// 0x000afee2
		fmul st(0), st(2)		// 0x000afee6
		fstp dword ptr [esp + 0x28]		// 0x000afee8
		fmul dword ptr [esp + 0x40]		// 0x000afeec
		fstp dword ptr [esp + 0x2c]		// 0x000afef0
		fstp st(0)		// 0x000afef4
		fld dword ptr [esp + 0x30]		// 0x000afef6
		fmul dword ptr [esp + 0x40]		// 0x000afefa
		fstp dword ptr [esp + 0x30]		// 0x000afefe
		fld dword ptr [nxP6K101041f0_4]		// 0x000aff02
		fld dword ptr [ecx + 0xc]		// 0x000aff08
		fucompp		// 0x000aff0b
		fnstsw ax		// 0x000aff0d
		test ah, 0x44		// 0x000aff0f
		jnp Laffb8		// 0x000aff12
		fld dword ptr [ecx + 0xc]		// 0x000aff18
		fld dword ptr [esp + 0x10]		// 0x000aff1b
		fmul st(0), st(1)		// 0x000aff1f
		fxch st(2)		// 0x000aff21
		fmul st(0), st(1)		// 0x000aff23
		fstp dword ptr [esp + 8]		// 0x000aff25
		fld dword ptr [esp + 0x18]		// 0x000aff29
		fmul st(0), st(1)		// 0x000aff2d
		fstp dword ptr [esp + 0xc]		// 0x000aff2f
		fxch st(1)		// 0x000aff33
		fadd dword ptr [ecx]		// 0x000aff35
		fstp dword ptr [ecx]		// 0x000aff37
		fstp st(0)		// 0x000aff39
		fld dword ptr [esp + 8]		// 0x000aff3b
		fadd dword ptr [ecx + 4]		// 0x000aff3f
		fstp dword ptr [ecx + 4]		// 0x000aff42
		fld dword ptr [esp + 0xc]		// 0x000aff45
		fadd dword ptr [ecx + 8]		// 0x000aff49
		fstp dword ptr [ecx + 8]		// 0x000aff4c
		fld dword ptr [esp + 0x30]		// 0x000aff4f
		fmul dword ptr [ecx + 0x28]		// 0x000aff53
		fld dword ptr [esp + 0x2c]		// 0x000aff56
		fmul dword ptr [ecx + 0x24]		// 0x000aff5a
		faddp st(1), st(0)		// 0x000aff5d
		fld dword ptr [esp + 0x28]		// 0x000aff5f
		fmul dword ptr [ecx + 0x20]		// 0x000aff63
		faddp st(1), st(0)		// 0x000aff66
		fld dword ptr [esp + 0x30]		// 0x000aff68
		fmul dword ptr [ecx + 0x34]		// 0x000aff6c
		fld dword ptr [esp + 0x2c]		// 0x000aff6f
		fmul dword ptr [ecx + 0x30]		// 0x000aff73
		faddp st(1), st(0)		// 0x000aff76
		fld dword ptr [esp + 0x28]		// 0x000aff78
		fmul dword ptr [ecx + 0x2c]		// 0x000aff7c
		faddp st(1), st(0)		// 0x000aff7f
		fld dword ptr [esp + 0x30]		// 0x000aff81
		fmul dword ptr [ecx + 0x40]		// 0x000aff85
		fld dword ptr [esp + 0x2c]		// 0x000aff88
		fmul dword ptr [ecx + 0x3c]		// 0x000aff8c
		faddp st(1), st(0)		// 0x000aff8f
		fld dword ptr [esp + 0x28]		// 0x000aff91
		fmul dword ptr [ecx + 0x38]		// 0x000aff95
		faddp st(1), st(0)		// 0x000aff98
		fstp dword ptr [esp + 0x18]		// 0x000aff9a
		fxch st(1)		// 0x000aff9e
		fadd dword ptr [ecx + 0x10]		// 0x000affa0
		fstp dword ptr [ecx + 0x10]		// 0x000affa3
		fadd dword ptr [ecx + 0x14]		// 0x000affa6
		fstp dword ptr [ecx + 0x14]		// 0x000affa9
		fld dword ptr [esp + 0x18]		// 0x000affac
		fadd dword ptr [ecx + 0x18]		// 0x000affb0
		fstp dword ptr [ecx + 0x18]		// 0x000affb3
		jmp Laffba		// 0x000affb6
Laffb8:
		fstp st(0)		// 0x000affb8
Laffba:
		cmp dword ptr [esp + 0x3c], 1		// 0x000affba
		jne Laffd5		// 0x000affbf
		mov ecx, dword ptr [edx + 0x30]		// 0x000affc1
		mov eax, dword ptr [ecx]		// 0x000affc4
		push esi		// 0x000affc6
		mov esi, dword ptr [esp + 0x3c]		// 0x000affc7
		push esi		// 0x000affcb
		push edx		// 0x000affcc
		mov edx, dword ptr [edx + 0x44]		// 0x000affcd
		push edx		// 0x000affd0
		call dword ptr [eax + 0xc]		// 0x000affd1
		pop esi		// 0x000affd4
Laffd5:
		pop ebx		// 0x000affd5
		add esp, 0x30		// 0x000affd6
		ret		// 0x000affd9
		}
	}

// phys_fn_004405 (0x000b02e0, 46 B), 19 instructions
__declspec(naked) unsigned __cdecl nxP64405()
	{
	__asm
		{
		mov eax, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000b02e0
		mov ecx, dword ptr [eax]		// 0x000b02e5
		mov edx, dword ptr [ecx]		// 0x000b02e7
		push 0		// 0x000b02e9
		push 0x28		// 0x000b02eb
		call dword ptr [edx + 8]		// 0x000b02ed
		test eax, eax		// 0x000b02f0
		je Lb030b		// 0x000b02f2
		mov ecx, dword ptr [esp + 0xc]		// 0x000b02f4
		mov edx, dword ptr [esp + 8]		// 0x000b02f8
		push ecx		// 0x000b02fc
		mov ecx, dword ptr [esp + 8]		// 0x000b02fd
		push edx		// 0x000b0301
		push ecx		// 0x000b0302
		mov ecx, eax		// 0x000b0303
		call nxP64759		// 0x000b0305
		ret		// 0x000b030a
Lb030b:
		xor eax, eax		// 0x000b030b
		ret		// 0x000b030d
		}
	}

// helper: phys_fn_004407 (record unchanged) (0x000b0310, 79 B), 27 instructions
__declspec(naked) unsigned __fastcall nxP64407(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2)
	{
	__asm
		{
		mov edx, dword ptr [esp + 0xc]		// 0x000b0310
		mov eax, ecx		// 0x000b0314
		mov ecx, dword ptr [esp + 8]		// 0x000b0316
		mov dword ptr [eax + 0x14], ecx		// 0x000b031a
		mov ecx, dword ptr [esp + 4]		// 0x000b031d
		mov dword ptr [eax + 0x18], edx		// 0x000b0321
		xor edx, edx		// 0x000b0324
		cmp ecx, edx		// 0x000b0326
		mov dword ptr [eax], offset nxP6TableA		// 0x000b0328
		mov dword ptr [eax + 0x1c], edx		// 0x000b032e
		mov dword ptr [eax + 0x20], edx		// 0x000b0331
		mov dword ptr [eax + 0x24], edx		// 0x000b0334
		je Lb0353		// 0x000b0337
		push esi		// 0x000b0339
		mov esi, dword ptr [ecx + 0xc]		// 0x000b033a
		mov dword ptr [eax + 0x10], esi		// 0x000b033d
		mov dword ptr [ecx + 0x1c], eax		// 0x000b0340
		mov dword ptr [ecx + 0xc], eax		// 0x000b0343
		mov dword ptr [eax + 8], ecx		// 0x000b0346
		inc dword ptr [ecx + 0x20]		// 0x000b0349
		pop esi		// 0x000b034c
		mov dword ptr [eax + 0xc], edx		// 0x000b034d
		ret 0xc		// 0x000b0350
Lb0353:
		mov dword ptr [eax + 8], edx		// 0x000b0353
		mov dword ptr [eax + 0x10], edx		// 0x000b0356
		mov dword ptr [eax + 0xc], edx		// 0x000b0359
		ret 0xc		// 0x000b035c
		}
	}

// helper: phys_fn_004409 (record unchanged) (0x000b0360, 57 B), 26 instructions
__declspec(naked) unsigned __fastcall nxP64409(void* self, void* edxUnused)
	{
	__asm
		{
		push ebx		// 0x000b0360
		push edi		// 0x000b0361
		mov edi, ecx		// 0x000b0362
		mov ecx, dword ptr [edi + 0xc]		// 0x000b0364
		xor ebx, ebx		// 0x000b0367
		cmp ecx, ebx		// 0x000b0369
		mov dword ptr [edi], offset nxP6TableA		// 0x000b036b
		je Lb0384		// 0x000b0371
		push esi		// 0x000b0373
Lb0374:
		mov eax, dword ptr [ecx]		// 0x000b0374
		mov esi, dword ptr [ecx + 0x10]		// 0x000b0376
		push 1		// 0x000b0379
		call dword ptr [eax]		// 0x000b037b
		cmp esi, ebx		// 0x000b037d
		mov ecx, esi		// 0x000b037f
		jne Lb0374		// 0x000b0381
		pop esi		// 0x000b0383
Lb0384:
		mov dword ptr [edi + 8], ebx		// 0x000b0384
		mov dword ptr [edi + 0xc], ebx		// 0x000b0387
		mov dword ptr [edi + 0x10], ebx		// 0x000b038a
		mov dword ptr [edi + 0x14], ebx		// 0x000b038d
		mov dword ptr [edi + 0x18], ebx		// 0x000b0390
		mov dword ptr [edi + 0x20], ebx		// 0x000b0393
		pop edi		// 0x000b0396
		pop ebx		// 0x000b0397
		ret		// 0x000b0398
		}
	}

// helper: phys_fn_004411 (record unchanged) (0x000b03a0, 3 B), 1 instructions
__declspec(naked) unsigned __stdcall nxP64411(unsigned a0, unsigned a1, unsigned a2)
	{
	__asm
		{
		ret 0xc		// 0x000b03a0
		}
	}

// phys_fn_004413 (0x000b03b0, 408 B), 126 instructions
__declspec(naked) unsigned __fastcall nxP64413(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		sub esp, 0x30		// 0x000b03b0
		push ebx		// 0x000b03b3
		mov ebx, ecx		// 0x000b03b4
		mov eax, dword ptr [ebx + 0x14]		// 0x000b03b6
		test eax, eax		// 0x000b03b9
		push esi		// 0x000b03bb
		push edi		// 0x000b03bc
		mov edi, dword ptr [esp + 0x40]		// 0x000b03bd
		je Lb0523		// 0x000b03c1
		mov ecx, dword ptr [ebx + 8]		// 0x000b03c7
		test ecx, ecx		// 0x000b03ca
		je Lb03ec		// 0x000b03cc
		mov ecx, dword ptr [ecx + 0x14]		// 0x000b03ce
		test ecx, ecx		// 0x000b03d1
		je Lb03ec		// 0x000b03d3
		push 0xa000		// 0x000b03d5
		add ecx, 0x158		// 0x000b03da
		push ecx		// 0x000b03e0
		add eax, 0x158		// 0x000b03e1
		push eax		// 0x000b03e6
		jmp Lb051c		// 0x000b03e7
Lb03ec:
		test eax, eax		// 0x000b03ec
		je Lb0523		// 0x000b03ee
		mov ecx, dword ptr [ebx + 0x14]		// 0x000b03f4
		fld dword ptr [gParameter + 0x68]		// 0x000b03f7
		fmul dword ptr [gParameter + 0x34]		// 0x000b03fd
		mov edx, dword ptr [eax + 0x150]		// 0x000b0403
		fld dword ptr [ecx + 0x134]		// 0x000b0409
		lea esi, [ecx + 0x158]		// 0x000b040f
		fld dword ptr [ecx + 0x140]		// 0x000b0415
		mov ecx, dword ptr [ecx + 0x14c]		// 0x000b041b
		fxch st(1)		// 0x000b0421
		mov dword ptr [esp + 0x2c], ecx		// 0x000b0423
		fmul st(0), st(2)		// 0x000b0427
		mov dword ptr [esp + 0x14], edx		// 0x000b0429
		mov edx, dword ptr [edi]		// 0x000b042d
		push 0xff0000		// 0x000b042f
		fstp dword ptr [esp + 0x28]		// 0x000b0434
		mov ecx, edi		// 0x000b0438
		fmul st(0), st(1)		// 0x000b043a
		fld dword ptr [esp + 0x30]		// 0x000b043c
		fmul st(0), st(2)		// 0x000b0440
		fstp dword ptr [esp + 0x30]		// 0x000b0442
		fld dword ptr [eax + 0x138]		// 0x000b0446
		fld dword ptr [eax + 0x144]		// 0x000b044c
		fxch st(1)		// 0x000b0452
		fmul st(0), st(3)		// 0x000b0454
		fstp dword ptr [esp + 0x10]		// 0x000b0456
		fmul st(0), st(2)		// 0x000b045a
		fstp dword ptr [esp + 0x14]		// 0x000b045c
		fld dword ptr [esp + 0x18]		// 0x000b0460
		fmul st(0), st(2)		// 0x000b0464
		fstp dword ptr [esp + 0x18]		// 0x000b0466
		fld dword ptr [eax + 0x13c]		// 0x000b046a
		fld dword ptr [eax + 0x148]		// 0x000b0470
		mov eax, dword ptr [eax + 0x154]		// 0x000b0476
		fxch st(1)		// 0x000b047c
		mov dword ptr [esp + 0x24], eax		// 0x000b047e
		fmul st(0), st(3)		// 0x000b0482
		lea eax, [esp + 0x34]		// 0x000b0484
		push eax		// 0x000b0488
		push esi		// 0x000b0489
		fstp dword ptr [esp + 0x24]		// 0x000b048a
		fmul st(0), st(2)		// 0x000b048e
		fstp dword ptr [esp + 0x28]		// 0x000b0490
		fld dword ptr [esp + 0x2c]		// 0x000b0494
		fmul st(0), st(2)		// 0x000b0498
		fstp dword ptr [esp + 0x2c]		// 0x000b049a
		fld dword ptr [esp + 0x30]		// 0x000b049e
		fadd dword ptr [esi]		// 0x000b04a2
		fstp dword ptr [esp + 0x3c]		// 0x000b04a4
		fadd dword ptr [esi + 4]		// 0x000b04a8
		fstp dword ptr [esp + 0x40]		// 0x000b04ab
		fstp st(0)		// 0x000b04af
		fld dword ptr [esp + 0x38]		// 0x000b04b1
		fadd dword ptr [esi + 8]		// 0x000b04b5
		fstp dword ptr [esp + 0x44]		// 0x000b04b8
		call dword ptr [edx + 0x20]		// 0x000b04bc
		fld dword ptr [esp + 0xc]		// 0x000b04bf
		mov edx, dword ptr [edi]		// 0x000b04c3
		fadd dword ptr [esi]		// 0x000b04c5
		push 0xff0000		// 0x000b04c7
		lea eax, [esp + 0x34]		// 0x000b04cc
		push eax		// 0x000b04d0
		fstp dword ptr [esp + 0x38]		// 0x000b04d1
		push esi		// 0x000b04d5
		fld dword ptr [esp + 0x1c]		// 0x000b04d6
		mov ecx, edi		// 0x000b04da
		fadd dword ptr [esi + 4]		// 0x000b04dc
		fstp dword ptr [esp + 0x40]		// 0x000b04df
		fld dword ptr [esp + 0x20]		// 0x000b04e3
		fadd dword ptr [esi + 8]		// 0x000b04e7
		fstp dword ptr [esp + 0x44]		// 0x000b04ea
		call dword ptr [edx + 0x20]		// 0x000b04ee
		fld dword ptr [esp + 0x18]		// 0x000b04f1
		push 0xff0000		// 0x000b04f5
		fadd dword ptr [esi]		// 0x000b04fa
		fstp dword ptr [esp + 0x34]		// 0x000b04fc
		fld dword ptr [esp + 0x20]		// 0x000b0500
		fadd dword ptr [esi + 4]		// 0x000b0504
		fstp dword ptr [esp + 0x38]		// 0x000b0507
		fld dword ptr [esp + 0x24]		// 0x000b050b
		fadd dword ptr [esi + 8]		// 0x000b050f
		fstp dword ptr [esp + 0x3c]		// 0x000b0512
		lea eax, [esp + 0x34]		// 0x000b0516
		push eax		// 0x000b051a
		push esi		// 0x000b051b
Lb051c:
		mov edx, dword ptr [edi]		// 0x000b051c
		mov ecx, edi		// 0x000b051e
		call dword ptr [edx + 0x20]		// 0x000b0520
Lb0523:
		mov esi, dword ptr [ebx + 0xc]		// 0x000b0523
		test esi, esi		// 0x000b0526
		je Lb053f		// 0x000b0528
		lea ebx, [ebx]		// 0x000b052a
Lb0530:
		mov edx, dword ptr [esi]		// 0x000b0530
		push edi		// 0x000b0532
		mov ecx, esi		// 0x000b0533
		call dword ptr [edx + 0x18]		// 0x000b0535
		mov esi, dword ptr [esi + 0x10]		// 0x000b0538
		test esi, esi		// 0x000b053b
		jne Lb0530		// 0x000b053d
Lb053f:
		pop edi		// 0x000b053f
		pop esi		// 0x000b0540
		pop ebx		// 0x000b0541
		add esp, 0x30		// 0x000b0542
		ret 4		// 0x000b0545
		}
	}

// helper: phys_fn_004415 (record unchanged) (0x000b0550, 34 B), 13 instructions
__declspec(naked) unsigned __fastcall nxP64415(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000b0550
		mov esi, ecx		// 0x000b0551
		call nxP64409		// 0x000b0553
		test byte ptr [esp + 8], 1		// 0x000b0558
		je Lb056c		// 0x000b055d
		mov eax, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000b055f
		mov ecx, dword ptr [eax]		// 0x000b0564
		mov edx, dword ptr [ecx]		// 0x000b0566
		push esi		// 0x000b0568
		call dword ptr [edx + 0x14]		// 0x000b0569
Lb056c:
		mov eax, esi		// 0x000b056c
		pop esi		// 0x000b056e
		ret 4		// 0x000b056f
		}
	}

// phys_fn_004537 (0x000b15e0, 31 B), 11 instructions
__declspec(naked) unsigned __fastcall nxP64537(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		test byte ptr [esp + 4], 1		// 0x000b15e0
		push esi		// 0x000b15e5
		mov esi, ecx		// 0x000b15e6
		mov dword ptr [esi], offset nxKnVtNxJoint		// 0x000b15e8
		je Lb15f9		// 0x000b15ee
		push esi		// 0x000b15f0
		call nxP6CallFree		// 0x000b15f1
		add esp, 4		// 0x000b15f6
Lb15f9:
		mov eax, esi		// 0x000b15f9
		pop esi		// 0x000b15fb
		ret 4		// 0x000b15fc
		}
	}

// phys_fn_004759 (0x000b38a0, 37 B), 14 instructions
__declspec(naked) unsigned __fastcall nxP64759(void* self, void* edxUnused, unsigned a0, unsigned a1, unsigned a2)
	{
	__asm
		{
		mov eax, dword ptr [esp + 0xc]		// 0x000b38a0
		mov edx, dword ptr [esp + 4]		// 0x000b38a4
		push esi		// 0x000b38a8
		mov esi, ecx		// 0x000b38a9
		mov ecx, dword ptr [esp + 0xc]		// 0x000b38ab
		push eax		// 0x000b38af
		push ecx		// 0x000b38b0
		push edx		// 0x000b38b1
		mov ecx, esi		// 0x000b38b2
		call nxP64407		// 0x000b38b4
		mov dword ptr [esi], offset nxP6TableB		// 0x000b38b9
		mov eax, esi		// 0x000b38bf
		pop esi		// 0x000b38c1
		ret 0xc		// 0x000b38c2
		}
	}

// phys_fn_004761 (0x000b38d0, 71 B), 25 instructions
__declspec(naked) unsigned __fastcall nxP64761(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000b38d0
		mov esi, ecx		// 0x000b38d1
		mov eax, dword ptr [esi + 0x18]		// 0x000b38d3
		test eax, eax		// 0x000b38d6
		push edi		// 0x000b38d8
		mov edi, dword ptr [esp + 0xc]		// 0x000b38d9
		je Lb38fc		// 0x000b38dd
		mov dword ptr [eax + 0x160], 0xffffffff		// 0x000b38df
		mov dword ptr [eax + 0x164], 0		// 0x000b38e9
		mov ecx, dword ptr [esi + 0x18]		// 0x000b38f3
		push edi		// 0x000b38f6
		call nxP6CallRowslot6		// 0x000b38f7
Lb38fc:
		mov esi, dword ptr [esi + 0xc]		// 0x000b38fc
		test esi, esi		// 0x000b38ff
		je Lb3912		// 0x000b3901
Lb3903:
		mov eax, dword ptr [esi]		// 0x000b3903
		push edi		// 0x000b3905
		mov ecx, esi		// 0x000b3906
		call dword ptr [eax + 0x14]		// 0x000b3908
		mov esi, dword ptr [esi + 0x10]		// 0x000b390b
		test esi, esi		// 0x000b390e
		jne Lb3903		// 0x000b3910
Lb3912:
		pop edi		// 0x000b3912
		pop esi		// 0x000b3913
		ret 4		// 0x000b3914
		}
	}

// helper: phys_fn_004763 (record unchanged) (0x000b3920, 96 B), 40 instructions
__declspec(naked) unsigned __fastcall nxP64763(void* self, void* edxUnused)
	{
	__asm
		{
		push esi		// 0x000b3920
		mov esi, ecx		// 0x000b3921
		mov eax, dword ptr [esi + 0x18]		// 0x000b3923
		test eax, eax		// 0x000b3926
		je Lb3967		// 0x000b3928
		mov ecx, dword ptr [eax + 8]		// 0x000b392a
		test ecx, ecx		// 0x000b392d
		mov dl, 0x80		// 0x000b392f
		je Lb393b		// 0x000b3931
		test byte ptr [ecx + 0x10c], dl		// 0x000b3933
		je Lb394a		// 0x000b3939
Lb393b:
		mov ecx, dword ptr [eax + 0xc]		// 0x000b393b
		test ecx, ecx		// 0x000b393e
		je Lb3967		// 0x000b3940
		test byte ptr [ecx + 0x10c], dl		// 0x000b3942
		jne Lb3967		// 0x000b3948
Lb394a:
		mov ecx, dword ptr [eax + 0x44]		// 0x000b394a
		test ecx, ecx		// 0x000b394d
		je Lb3967		// 0x000b394f
		mov eax, dword ptr [eax + 0x2c]		// 0x000b3951
		shr eax, 2		// 0x000b3954
		test al, 1		// 0x000b3957
		jne Lb3967		// 0x000b3959
		mov ecx, dword ptr [esi + 0x18]		// 0x000b395b
		mov eax, dword ptr [esi + 0x14]		// 0x000b395e
		mov edx, dword ptr [ecx]		// 0x000b3961
		push eax		// 0x000b3963
		call dword ptr [edx + 0x20]		// 0x000b3964
Lb3967:
		mov esi, dword ptr [esi + 0xc]		// 0x000b3967
		test esi, esi		// 0x000b396a
		je Lb397e		// 0x000b396c
		mov edi, edi		// 0x000b396e
Lb3970:
		mov edx, dword ptr [esi]		// 0x000b3970
		mov ecx, esi		// 0x000b3972
		call dword ptr [edx + 0x10]		// 0x000b3974
		mov esi, dword ptr [esi + 0x10]		// 0x000b3977
		test esi, esi		// 0x000b397a
		jne Lb3970		// 0x000b397c
Lb397e:
		pop esi		// 0x000b397e
		ret		// 0x000b397f
		}
	}

// helper: phys_fn_004764 (record unchanged) (0x000b3980, 40 B), 14 instructions
__declspec(naked) unsigned __fastcall nxP64764(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x000b3980
		mov esi, ecx		// 0x000b3981
		mov dword ptr [esi], offset nxP6TableB		// 0x000b3983
		call nxP64409		// 0x000b3989
		test byte ptr [esp + 8], 1		// 0x000b398e
		je Lb39a2		// 0x000b3993
		mov eax, dword ptr [nxP6ImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x000b3995
		mov ecx, dword ptr [eax]		// 0x000b399a
		mov edx, dword ptr [ecx]		// 0x000b399c
		push esi		// 0x000b399e
		call dword ptr [edx + 0x14]		// 0x000b399f
Lb39a2:
		mov eax, esi		// 0x000b39a2
		pop esi		// 0x000b39a4
		ret 4		// 0x000b39a5
		}
	}
// @@ROWS-END@@
