/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// PMapRows.cpp: the penetration map's cell-run coder as the oracle has it, over .data statics (the cell cursor at
// 0x101222e8 and 32 words at 0x101240a0) rather than the candidate PMap.cpp's cursor struct: phys_fn_001988 (the
// statics reset), 001992 to 002006 (the encoder, one function the inventory splits eight ways), 002010 to 002014 (the
// cell decoder, three ways) and 002031 (the scalar deleting destructor). With two helpers the rows call that the
// candidate has only in other shapes: the tail of 002008 from its loop at 0x0004dc22 (002010 calls that label) and
// 005159 as a thiscall. Naked listing transcriptions; PMap.cpp keeps its own coder, so nothing calls them yet.
// Compare script: docs/reconstruction/novodex-physics/evidence/phase4-pmap-rows-listing-compare.py.
#include "NxPhysics.h"

extern "C" void nxPMCallEmpty();
extern "C" void nxPMCallFree();
extern "C" void nxPMCallReadByte();
extern "C" void nxPMCallResize();
extern "C" void nxPMCallStoreByte();
extern "C" void* nxPMImpImpnxFoundationSDKAllocator3PAVNxUserAll;
#pragma comment(linker, "/alternatename:_nxPMCallEmpty=?empty@SdkContainer@@QAEXXZ")
#pragma comment(linker, "/alternatename:_nxPMCallFree=_free")
#pragma comment(linker, "/alternatename:_nxPMCallReadByte=?readByte@MemoryStream@@QAEEXZ")
#pragma comment(linker, "/alternatename:_nxPMCallResize=?resize@SdkContainer@@QAE_NI@Z")
#pragma comment(linker, "/alternatename:_nxPMCallStoreByte=?storeByte@MemoryStream@@QAEPAV1@E@Z")
#pragma comment(linker, "/alternatename:_nxPMImpImpnxFoundationSDKAllocator3PAVNxUserAll=__imp_?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A")
extern "C" void* nxSdkInstanceSlot;
#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")
extern "C" unsigned gNxApiReentry;
extern "C" const char* gNxApiReentryMessage;
// .data 0x101222e8 (three words, the cell cursor) and 0x101240a0 (32 words), the cell-run coder statics.
unsigned nxPMapCursor[3];
unsigned nxPMapCodeCounts[32];
extern "C" void nxPMCallSdkAllocator();
#pragma comment(linker, "/alternatename:_nxPMCallSdkAllocator=?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ")
extern "C" void* nxPMapVtable;
#pragma comment(linker, "/alternatename:_nxPMapVtable=??_7PenetrationMap@@6B@")

unsigned __cdecl nxPMapCoderReset();
unsigned __fastcall nxPMapEncodeCellRun(void* self, void* edxUnused);
unsigned __fastcall nxPMapDecodeCellRun(void* self, void* edxUnused, unsigned a0, unsigned a1);
unsigned __fastcall nxPMapScalarDelete(void* self, void* edxUnused, unsigned a0);
void __fastcall nxPMapRelease5159(void* self);
// @@PROTOS-END@@


// phys_fn_001988 (0x0004cba0, 181 B), 38 instructions
__declspec(naked) unsigned __cdecl nxPMapCoderReset()
	{
	__asm
		{
		or eax, 0xffffffff		// 0x0004cba0
		mov dword ptr [nxPMapCursor], eax		// 0x0004cba3
		mov dword ptr [nxPMapCursor + 4], eax		// 0x0004cba8
		mov dword ptr [nxPMapCursor + 8], eax		// 0x0004cbad
		xor eax, eax		// 0x0004cbb2
		mov dword ptr [nxPMapCodeCounts], eax		// 0x0004cbb4
		mov dword ptr [nxPMapCodeCounts + 4], eax		// 0x0004cbb9
		mov dword ptr [nxPMapCodeCounts + 8], eax		// 0x0004cbbe
		mov dword ptr [nxPMapCodeCounts + 12], eax		// 0x0004cbc3
		mov dword ptr [nxPMapCodeCounts + 16], eax		// 0x0004cbc8
		mov dword ptr [nxPMapCodeCounts + 20], eax		// 0x0004cbcd
		mov dword ptr [nxPMapCodeCounts + 24], eax		// 0x0004cbd2
		mov dword ptr [nxPMapCodeCounts + 28], eax		// 0x0004cbd7
		mov dword ptr [nxPMapCodeCounts + 32], eax		// 0x0004cbdc
		mov dword ptr [nxPMapCodeCounts + 36], eax		// 0x0004cbe1
		mov dword ptr [nxPMapCodeCounts + 40], eax		// 0x0004cbe6
		mov dword ptr [nxPMapCodeCounts + 44], eax		// 0x0004cbeb
		mov dword ptr [nxPMapCodeCounts + 48], eax		// 0x0004cbf0
		mov dword ptr [nxPMapCodeCounts + 52], eax		// 0x0004cbf5
		mov dword ptr [nxPMapCodeCounts + 56], eax		// 0x0004cbfa
		mov dword ptr [nxPMapCodeCounts + 60], eax		// 0x0004cbff
		mov dword ptr [nxPMapCodeCounts + 64], eax		// 0x0004cc04
		mov dword ptr [nxPMapCodeCounts + 68], eax		// 0x0004cc09
		mov dword ptr [nxPMapCodeCounts + 72], eax		// 0x0004cc0e
		mov dword ptr [nxPMapCodeCounts + 76], eax		// 0x0004cc13
		mov dword ptr [nxPMapCodeCounts + 80], eax		// 0x0004cc18
		mov dword ptr [nxPMapCodeCounts + 84], eax		// 0x0004cc1d
		mov dword ptr [nxPMapCodeCounts + 88], eax		// 0x0004cc22
		mov dword ptr [nxPMapCodeCounts + 92], eax		// 0x0004cc27
		mov dword ptr [nxPMapCodeCounts + 96], eax		// 0x0004cc2c
		mov dword ptr [nxPMapCodeCounts + 100], eax		// 0x0004cc31
		mov dword ptr [nxPMapCodeCounts + 104], eax		// 0x0004cc36
		mov dword ptr [nxPMapCodeCounts + 108], eax		// 0x0004cc3b
		mov dword ptr [nxPMapCodeCounts + 112], eax		// 0x0004cc40
		mov dword ptr [nxPMapCodeCounts + 116], eax		// 0x0004cc45
		mov dword ptr [nxPMapCodeCounts + 120], eax		// 0x0004cc4a
		mov dword ptr [nxPMapCodeCounts + 124], eax		// 0x0004cc4f
		ret		// 0x0004cc54
		}
	}

// phys_fn_001992 and phys_fn_001994 and phys_fn_001996 and phys_fn_001998 and phys_fn_002000 and phys_fn_002002 and phys_fn_002004 and phys_fn_002006 (one function) (0x0004cee0, 3262 B), 1102 instructions
__declspec(naked) unsigned __fastcall nxPMapEncodeCellRun(void* self, void* edxUnused)
	{
	__asm
		{
L4cee0:
		mov edx, dword ptr [esp + 0x18]		// 0x0004cee0
		mov eax, dword ptr [esp + 0x30]		// 0x0004cee4
		mov eax, dword ptr [eax + edx*4]		// 0x0004cee8
		mov edx, dword ptr [esp + 0x5c]		// 0x0004ceeb
		mov edx, dword ptr [edx + eax*4]		// 0x0004ceef
		mov dword ptr [esp + 0xc], edx		// 0x0004cef2
		mov edx, dword ptr [esp + 0x4c]		// 0x0004cef6
		mov edx, dword ptr [edx + eax*4]		// 0x0004cefa
		mov dword ptr [esp + 0x10], edx		// 0x0004cefd
		mov edx, dword ptr [esp + 0x3c]		// 0x0004cf01
		mov eax, dword ptr [edx + eax*4]		// 0x0004cf05
		mov edx, dword ptr [esp + 0xc]		// 0x0004cf08
		sub edx, ecx		// 0x0004cf0c
		mov ecx, dword ptr [esp + 0x10]		// 0x0004cf0e
		mov dword ptr [esp + 0x14], eax		// 0x0004cf12
		sub ecx, edi		// 0x0004cf16
		sub eax, ebx		// 0x0004cf18
		cmp edx, -1		// 0x0004cf1a
		jne L4cf66		// 0x0004cf1d
		test ecx, ecx		// 0x0004cf1f
		jne L4d0db		// 0x0004cf21
		test eax, eax		// 0x0004cf27
		jne L4d33e		// 0x0004cf29
		mov edi, 0x10		// 0x0004cf2f
L4cf34:
		mov bl, byte ptr [esi + 0x19]		// 0x0004cf34
		mov dl, byte ptr [esi + 0x18]		// 0x0004cf37
		shl bl, 1		// 0x0004cf3a
		xor eax, eax		// 0x0004cf3c
		inc dl		// 0x0004cf3e
		mov cl, dl		// 0x0004cf40
		cmp cl, 8		// 0x0004cf42
		mov byte ptr [esi + 0x19], bl		// 0x0004cf45
		mov al, bl		// 0x0004cf48
		mov byte ptr [esi + 0x18], dl		// 0x0004cf4a
		jne L4cf5b		// 0x0004cf4d
		push eax		// 0x0004cf4f
		mov ecx, esi		// 0x0004cf50
		mov byte ptr [esi + 0x18], 0		// 0x0004cf52
		call nxPMCallStoreByte		// 0x0004cf56
L4cf5b:
		shr edi, 1		// 0x0004cf5b
		test edi, edi		// 0x0004cf5d
		jne L4cf34		// 0x0004cf5f
		jmp L4db34		// 0x0004cf61
L4cf66:
		cmp edx, 1		// 0x0004cf66
		jne L4cfb4		// 0x0004cf69
		test ecx, ecx		// 0x0004cf6b
		jne L4d127		// 0x0004cf6d
		test eax, eax		// 0x0004cf73
		jne L4d38e		// 0x0004cf75
		mov ebx, 0x10		// 0x0004cf7b
L4cf80:
		mov cl, byte ptr [esi + 0x19]		// 0x0004cf80
		mov dl, byte ptr [esi + 0x18]		// 0x0004cf83
		shl cl, 1		// 0x0004cf86
		mov al, bl		// 0x0004cf88
		and al, 1		// 0x0004cf8a
		or al, cl		// 0x0004cf8c
		inc dl		// 0x0004cf8e
		mov cl, dl		// 0x0004cf90
		cmp cl, 8		// 0x0004cf92
		mov byte ptr [esi + 0x19], al		// 0x0004cf95
		mov byte ptr [esi + 0x18], dl		// 0x0004cf98
		jne L4cfa9		// 0x0004cf9b
		push eax		// 0x0004cf9d
		mov ecx, esi		// 0x0004cf9e
		mov byte ptr [esi + 0x18], 0		// 0x0004cfa0
		call nxPMCallStoreByte		// 0x0004cfa4
L4cfa9:
		shr ebx, 1		// 0x0004cfa9
		test ebx, ebx		// 0x0004cfab
		jne L4cf80		// 0x0004cfad
		jmp L4db34		// 0x0004cfaf
L4cfb4:
		test edx, edx		// 0x0004cfb4
		jne L4d6c6		// 0x0004cfb6
		cmp ecx, -1		// 0x0004cfbc
		jne L4d004		// 0x0004cfbf
		test eax, eax		// 0x0004cfc1
		jne L4d216		// 0x0004cfc3
		mov ebx, 0x10		// 0x0004cfc9
		mov edi, edi		// 0x0004cfce
L4cfd0:
		mov dl, byte ptr [esi + 0x19]		// 0x0004cfd0
		shr ebx, 1		// 0x0004cfd3
		shl dl, 1		// 0x0004cfd5
		mov al, bl		// 0x0004cfd7
		and al, 1		// 0x0004cfd9
		or al, dl		// 0x0004cfdb
		mov dl, byte ptr [esi + 0x18]		// 0x0004cfdd
		inc dl		// 0x0004cfe0
		mov cl, dl		// 0x0004cfe2
		cmp cl, 8		// 0x0004cfe4
		mov byte ptr [esi + 0x19], al		// 0x0004cfe7
		mov byte ptr [esi + 0x18], dl		// 0x0004cfea
		jne L4cffb		// 0x0004cfed
		push eax		// 0x0004cfef
		mov ecx, esi		// 0x0004cff0
		mov byte ptr [esi + 0x18], 0		// 0x0004cff2
		call nxPMCallStoreByte		// 0x0004cff6
L4cffb:
		test ebx, ebx		// 0x0004cffb
		jne L4cfd0		// 0x0004cffd
		jmp L4db34		// 0x0004cfff
L4d004:
		cmp ecx, 1		// 0x0004d004
		jne L4d04c		// 0x0004d007
		test eax, eax		// 0x0004d009
		jne L4d25a		// 0x0004d00b
		mov ebx, 0x10		// 0x0004d011
L4d016:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d016
		mov dl, byte ptr [esi + 0x18]		// 0x0004d019
		test bl, 3		// 0x0004d01c
		setne al		// 0x0004d01f
		shl cl, 1		// 0x0004d022
		or al, cl		// 0x0004d024
		inc dl		// 0x0004d026
		mov cl, dl		// 0x0004d028
		cmp cl, 8		// 0x0004d02a
		mov byte ptr [esi + 0x19], al		// 0x0004d02d
		mov byte ptr [esi + 0x18], dl		// 0x0004d030
		jne L4d041		// 0x0004d033
		push eax		// 0x0004d035
		mov ecx, esi		// 0x0004d036
		mov byte ptr [esi + 0x18], 0		// 0x0004d038
		call nxPMCallStoreByte		// 0x0004d03c
L4d041:
		shr ebx, 1		// 0x0004d041
		test ebx, ebx		// 0x0004d043
		jne L4d016		// 0x0004d045
		jmp L4db34		// 0x0004d047
L4d04c:
		test ecx, ecx		// 0x0004d04c
		jne L4d76c		// 0x0004d04e
		cmp eax, -1		// 0x0004d054
		jne L4d097		// 0x0004d057
		mov edi, 0x10		// 0x0004d059
		mov edi, edi		// 0x0004d05e
L4d060:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d060
		mov eax, edi		// 0x0004d063
		shr eax, 2		// 0x0004d065
		shl dl, 1		// 0x0004d068
		and al, 1		// 0x0004d06a
		or al, dl		// 0x0004d06c
		mov dl, byte ptr [esi + 0x18]		// 0x0004d06e
		inc dl		// 0x0004d071
		mov cl, dl		// 0x0004d073
		cmp cl, 8		// 0x0004d075
		mov byte ptr [esi + 0x19], al		// 0x0004d078
		mov byte ptr [esi + 0x18], dl		// 0x0004d07b
		jne L4d08c		// 0x0004d07e
		push eax		// 0x0004d080
		mov ecx, esi		// 0x0004d081
		mov byte ptr [esi + 0x18], 0		// 0x0004d083
		call nxPMCallStoreByte		// 0x0004d087
L4d08c:
		shr edi, 1		// 0x0004d08c
		test edi, edi		// 0x0004d08e
		jne L4d060		// 0x0004d090
		jmp L4db34		// 0x0004d092
L4d097:
		cmp eax, 1		// 0x0004d097
		jne L4d80c		// 0x0004d09a
		mov ebx, 0x10		// 0x0004d0a0
L4d0a5:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d0a5
		mov dl, byte ptr [esi + 0x18]		// 0x0004d0a8
		test bl, 5		// 0x0004d0ab
		setne al		// 0x0004d0ae
		shl cl, 1		// 0x0004d0b1
		or al, cl		// 0x0004d0b3
		inc dl		// 0x0004d0b5
		mov cl, dl		// 0x0004d0b7
		cmp cl, 8		// 0x0004d0b9
		mov byte ptr [esi + 0x19], al		// 0x0004d0bc
		mov byte ptr [esi + 0x18], dl		// 0x0004d0bf
		jne L4d0d0		// 0x0004d0c2
		push eax		// 0x0004d0c4
		mov ecx, esi		// 0x0004d0c5
		mov byte ptr [esi + 0x18], 0		// 0x0004d0c7
		call nxPMCallStoreByte		// 0x0004d0cb
L4d0d0:
		shr ebx, 1		// 0x0004d0d0
		test ebx, ebx		// 0x0004d0d2
		jne L4d0a5		// 0x0004d0d4
		jmp L4db34		// 0x0004d0d6
L4d0db:
		cmp ecx, -1		// 0x0004d0db
		jne L4d176		// 0x0004d0de
		test eax, eax		// 0x0004d0e4
		jne L4d46f		// 0x0004d0e6
		mov ebx, 0x10		// 0x0004d0ec
L4d0f1:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d0f1
		test bl, 6		// 0x0004d0f4
		setne al		// 0x0004d0f7
		shl dl, 1		// 0x0004d0fa
		or al, dl		// 0x0004d0fc
		mov dl, byte ptr [esi + 0x18]		// 0x0004d0fe
		inc dl		// 0x0004d101
		mov cl, dl		// 0x0004d103
		cmp cl, 8		// 0x0004d105
		mov byte ptr [esi + 0x19], al		// 0x0004d108
		mov byte ptr [esi + 0x18], dl		// 0x0004d10b
		jne L4d11c		// 0x0004d10e
		push eax		// 0x0004d110
		mov ecx, esi		// 0x0004d111
		mov byte ptr [esi + 0x18], 0		// 0x0004d113
		call nxPMCallStoreByte		// 0x0004d117
L4d11c:
		shr ebx, 1		// 0x0004d11c
		test ebx, ebx		// 0x0004d11e
		jne L4d0f1		// 0x0004d120
		jmp L4db34		// 0x0004d122
L4d127:
		cmp ecx, 1		// 0x0004d127
		jne L4d1c7		// 0x0004d12a
		test eax, eax		// 0x0004d130
		jne L4d4bf		// 0x0004d132
		mov ebx, 0x10		// 0x0004d138
		lea ecx, [ecx]		// 0x0004d13d
L4d140:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d140
		mov dl, byte ptr [esi + 0x18]		// 0x0004d143
		test bl, 7		// 0x0004d146
		setne al		// 0x0004d149
		shl cl, 1		// 0x0004d14c
		or al, cl		// 0x0004d14e
		inc dl		// 0x0004d150
		mov cl, dl		// 0x0004d152
		cmp cl, 8		// 0x0004d154
		mov byte ptr [esi + 0x19], al		// 0x0004d157
		mov byte ptr [esi + 0x18], dl		// 0x0004d15a
		jne L4d16b		// 0x0004d15d
		push eax		// 0x0004d15f
		mov ecx, esi		// 0x0004d160
		mov byte ptr [esi + 0x18], 0		// 0x0004d162
		call nxPMCallStoreByte		// 0x0004d166
L4d16b:
		shr ebx, 1		// 0x0004d16b
		test ebx, ebx		// 0x0004d16d
		jne L4d140		// 0x0004d16f
		jmp L4db34		// 0x0004d171
L4d176:
		cmp ecx, 1		// 0x0004d176
		jne L4d336		// 0x0004d179
		test eax, eax		// 0x0004d17f
		jne L4d5ef		// 0x0004d181
		mov edi, 0x10		// 0x0004d187
		lea esp, [esp]		// 0x0004d18c
L4d190:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d190
		mov eax, edi		// 0x0004d193
		shr eax, 3		// 0x0004d195
		shl dl, 1		// 0x0004d198
		and al, 1		// 0x0004d19a
		or al, dl		// 0x0004d19c
		mov dl, byte ptr [esi + 0x18]		// 0x0004d19e
		inc dl		// 0x0004d1a1
		mov cl, dl		// 0x0004d1a3
		cmp cl, 8		// 0x0004d1a5
		mov byte ptr [esi + 0x19], al		// 0x0004d1a8
		mov byte ptr [esi + 0x18], dl		// 0x0004d1ab
		jne L4d1bc		// 0x0004d1ae
		push eax		// 0x0004d1b0
		mov ecx, esi		// 0x0004d1b1
		mov byte ptr [esi + 0x18], 0		// 0x0004d1b3
		call nxPMCallStoreByte		// 0x0004d1b7
L4d1bc:
		shr edi, 1		// 0x0004d1bc
		test edi, edi		// 0x0004d1be
		jne L4d190		// 0x0004d1c0
		jmp L4db34		// 0x0004d1c2
L4d1c7:
		cmp ecx, -1		// 0x0004d1c7
		jne L4d386		// 0x0004d1ca
		test eax, eax		// 0x0004d1d0
		jne L4d59f		// 0x0004d1d2
		mov ebx, 0x10		// 0x0004d1d8
		lea ecx, [ecx]		// 0x0004d1dd
L4d1e0:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d1e0
		mov dl, byte ptr [esi + 0x18]		// 0x0004d1e3
		test bl, 9		// 0x0004d1e6
		setne al		// 0x0004d1e9
		shl cl, 1		// 0x0004d1ec
		or al, cl		// 0x0004d1ee
		inc dl		// 0x0004d1f0
		mov cl, dl		// 0x0004d1f2
		cmp cl, 8		// 0x0004d1f4
		mov byte ptr [esi + 0x19], al		// 0x0004d1f7
		mov byte ptr [esi + 0x18], dl		// 0x0004d1fa
		jne L4d20b		// 0x0004d1fd
		push eax		// 0x0004d1ff
		mov ecx, esi		// 0x0004d200
		mov byte ptr [esi + 0x18], 0		// 0x0004d202
		call nxPMCallStoreByte		// 0x0004d206
L4d20b:
		shr ebx, 1		// 0x0004d20b
		test ebx, ebx		// 0x0004d20d
		jne L4d1e0		// 0x0004d20f
		jmp L4db34		// 0x0004d211
L4d216:
		cmp eax, -1		// 0x0004d216
		jne L4d2a6		// 0x0004d219
		mov ebx, 0x10		// 0x0004d21f
L4d224:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d224
		test bl, 0xa		// 0x0004d227
		setne al		// 0x0004d22a
		shl dl, 1		// 0x0004d22d
		or al, dl		// 0x0004d22f
		mov dl, byte ptr [esi + 0x18]		// 0x0004d231
		inc dl		// 0x0004d234
		mov cl, dl		// 0x0004d236
		cmp cl, 8		// 0x0004d238
		mov byte ptr [esi + 0x19], al		// 0x0004d23b
		mov byte ptr [esi + 0x18], dl		// 0x0004d23e
		jne L4d24f		// 0x0004d241
		push eax		// 0x0004d243
		mov ecx, esi		// 0x0004d244
		mov byte ptr [esi + 0x18], 0		// 0x0004d246
		call nxPMCallStoreByte		// 0x0004d24a
L4d24f:
		shr ebx, 1		// 0x0004d24f
		test ebx, ebx		// 0x0004d251
		jne L4d224		// 0x0004d253
		jmp L4db34		// 0x0004d255
L4d25a:
		cmp eax, 1		// 0x0004d25a
		jne L4d2ea		// 0x0004d25d
		mov ebx, 0x10		// 0x0004d263
		jmp L4d270		// 0x0004d268
		lea ebx, [ebx]		// 0x0004d26a
L4d270:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d270
		mov dl, byte ptr [esi + 0x18]		// 0x0004d273
		test bl, 0xb		// 0x0004d276
		setne al		// 0x0004d279
		shl cl, 1		// 0x0004d27c
		or al, cl		// 0x0004d27e
		inc dl		// 0x0004d280
		mov cl, dl		// 0x0004d282
		cmp cl, 8		// 0x0004d284
		mov byte ptr [esi + 0x19], al		// 0x0004d287
		mov byte ptr [esi + 0x18], dl		// 0x0004d28a
		jne L4d29b		// 0x0004d28d
		push eax		// 0x0004d28f
		mov ecx, esi		// 0x0004d290
		mov byte ptr [esi + 0x18], 0		// 0x0004d292
		call nxPMCallStoreByte		// 0x0004d296
L4d29b:
		shr ebx, 1		// 0x0004d29b
		test ebx, ebx		// 0x0004d29d
		jne L4d270		// 0x0004d29f
		jmp L4db34		// 0x0004d2a1
L4d2a6:
		cmp eax, 1		// 0x0004d2a6
		jne L4d76c		// 0x0004d2a9
		mov ebx, 0x10		// 0x0004d2af
L4d2b4:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d2b4
		test bl, 0xc		// 0x0004d2b7
		setne al		// 0x0004d2ba
		shl dl, 1		// 0x0004d2bd
		or al, dl		// 0x0004d2bf
		mov dl, byte ptr [esi + 0x18]		// 0x0004d2c1
		inc dl		// 0x0004d2c4
		mov cl, dl		// 0x0004d2c6
		cmp cl, 8		// 0x0004d2c8
		mov byte ptr [esi + 0x19], al		// 0x0004d2cb
		mov byte ptr [esi + 0x18], dl		// 0x0004d2ce
		jne L4d2df		// 0x0004d2d1
		push eax		// 0x0004d2d3
		mov ecx, esi		// 0x0004d2d4
		mov byte ptr [esi + 0x18], 0		// 0x0004d2d6
		call nxPMCallStoreByte		// 0x0004d2da
L4d2df:
		shr ebx, 1		// 0x0004d2df
		test ebx, ebx		// 0x0004d2e1
		jne L4d2b4		// 0x0004d2e3
		jmp L4db34		// 0x0004d2e5
L4d2ea:
		cmp eax, -1		// 0x0004d2ea
		jne L4d76c		// 0x0004d2ed
		mov ebx, 0x10		// 0x0004d2f3
		jmp L4d300		// 0x0004d2f8
		lea ebx, [ebx]		// 0x0004d2fa
L4d300:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d300
		mov dl, byte ptr [esi + 0x18]		// 0x0004d303
		test bl, 0xd		// 0x0004d306
		setne al		// 0x0004d309
		shl cl, 1		// 0x0004d30c
		or al, cl		// 0x0004d30e
		inc dl		// 0x0004d310
		mov cl, dl		// 0x0004d312
		cmp cl, 8		// 0x0004d314
		mov byte ptr [esi + 0x19], al		// 0x0004d317
		mov byte ptr [esi + 0x18], dl		// 0x0004d31a
		jne L4d32b		// 0x0004d31d
		push eax		// 0x0004d31f
		mov ecx, esi		// 0x0004d320
		mov byte ptr [esi + 0x18], 0		// 0x0004d322
		call nxPMCallStoreByte		// 0x0004d326
L4d32b:
		shr ebx, 1		// 0x0004d32b
		test ebx, ebx		// 0x0004d32d
		jne L4d300		// 0x0004d32f
		jmp L4db34		// 0x0004d331
L4d336:
		test ecx, ecx		// 0x0004d336
		jne L4d466		// 0x0004d338
L4d33e:
		cmp eax, -1		// 0x0004d33e
		jne L4d3d6		// 0x0004d341
		mov ebx, 0x10		// 0x0004d347
		lea esp, [esp]		// 0x0004d34c
L4d350:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d350
		test bl, 0xe		// 0x0004d353
		setne al		// 0x0004d356
		shl dl, 1		// 0x0004d359
		or al, dl		// 0x0004d35b
		mov dl, byte ptr [esi + 0x18]		// 0x0004d35d
		inc dl		// 0x0004d360
		mov cl, dl		// 0x0004d362
		cmp cl, 8		// 0x0004d364
		mov byte ptr [esi + 0x19], al		// 0x0004d367
		mov byte ptr [esi + 0x18], dl		// 0x0004d36a
		jne L4d37b		// 0x0004d36d
		push eax		// 0x0004d36f
		mov ecx, esi		// 0x0004d370
		mov byte ptr [esi + 0x18], 0		// 0x0004d372
		call nxPMCallStoreByte		// 0x0004d376
L4d37b:
		shr ebx, 1		// 0x0004d37b
		test ebx, ebx		// 0x0004d37d
		jne L4d350		// 0x0004d37f
		jmp L4db34		// 0x0004d381
L4d386:
		test ecx, ecx		// 0x0004d386
		jne L4d4b6		// 0x0004d388
L4d38e:
		cmp eax, 1		// 0x0004d38e
		jne L4d41b		// 0x0004d391
		mov ebx, 0x10		// 0x0004d397
		lea esp, [esp]		// 0x0004d39c
L4d3a0:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d3a0
		mov dl, byte ptr [esi + 0x18]		// 0x0004d3a3
		test bl, 0xf		// 0x0004d3a6
		setne al		// 0x0004d3a9
		shl cl, 1		// 0x0004d3ac
		or al, cl		// 0x0004d3ae
		inc dl		// 0x0004d3b0
		mov cl, dl		// 0x0004d3b2
		cmp cl, 8		// 0x0004d3b4
		mov byte ptr [esi + 0x19], al		// 0x0004d3b7
		mov byte ptr [esi + 0x18], dl		// 0x0004d3ba
		jne L4d3cb		// 0x0004d3bd
		push eax		// 0x0004d3bf
		mov ecx, esi		// 0x0004d3c0
		mov byte ptr [esi + 0x18], 0		// 0x0004d3c2
		call nxPMCallStoreByte		// 0x0004d3c6
L4d3cb:
		shr ebx, 1		// 0x0004d3cb
		test ebx, ebx		// 0x0004d3cd
		jne L4d3a0		// 0x0004d3cf
		jmp L4db34		// 0x0004d3d1
L4d3d6:
		cmp eax, 1		// 0x0004d3d6
		jne L4d6ce		// 0x0004d3d9
		mov edi, 0x10		// 0x0004d3df
L4d3e4:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d3e4
		mov eax, edi		// 0x0004d3e7
		shr eax, 4		// 0x0004d3e9
		shl dl, 1		// 0x0004d3ec
		and al, 1		// 0x0004d3ee
		or al, dl		// 0x0004d3f0
		mov dl, byte ptr [esi + 0x18]		// 0x0004d3f2
		inc dl		// 0x0004d3f5
		mov cl, dl		// 0x0004d3f7
		cmp cl, 8		// 0x0004d3f9
		mov byte ptr [esi + 0x19], al		// 0x0004d3fc
		mov byte ptr [esi + 0x18], dl		// 0x0004d3ff
		jne L4d410		// 0x0004d402
		push eax		// 0x0004d404
		mov ecx, esi		// 0x0004d405
		mov byte ptr [esi + 0x18], 0		// 0x0004d407
		call nxPMCallStoreByte		// 0x0004d40b
L4d410:
		shr edi, 1		// 0x0004d410
		test edi, edi		// 0x0004d412
		jne L4d3e4		// 0x0004d414
		jmp L4db34		// 0x0004d416
L4d41b:
		cmp eax, -1		// 0x0004d41b
		jne L4d6ce		// 0x0004d41e
		mov ebx, 0x10		// 0x0004d424
		lea esp, [esp]		// 0x0004d429
L4d430:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d430
		mov dl, byte ptr [esi + 0x18]		// 0x0004d433
		test bl, 0x11		// 0x0004d436
		setne al		// 0x0004d439
		shl cl, 1		// 0x0004d43c
		or al, cl		// 0x0004d43e
		inc dl		// 0x0004d440
		mov cl, dl		// 0x0004d442
		cmp cl, 8		// 0x0004d444
		mov byte ptr [esi + 0x19], al		// 0x0004d447
		mov byte ptr [esi + 0x18], dl		// 0x0004d44a
		jne L4d45b		// 0x0004d44d
		push eax		// 0x0004d44f
		mov ecx, esi		// 0x0004d450
		mov byte ptr [esi + 0x18], 0		// 0x0004d452
		call nxPMCallStoreByte		// 0x0004d456
L4d45b:
		shr ebx, 1		// 0x0004d45b
		test ebx, ebx		// 0x0004d45d
		jne L4d430		// 0x0004d45f
		jmp L4db34		// 0x0004d461
L4d466:
		cmp ecx, -1		// 0x0004d466
		jne L4d5e6		// 0x0004d469
L4d46f:
		cmp eax, -1		// 0x0004d46f
		jne L4d506		// 0x0004d472
		mov ebx, 0x10		// 0x0004d478
		lea ecx, [ecx]		// 0x0004d47d
L4d480:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d480
		test bl, 0x12		// 0x0004d483
		setne al		// 0x0004d486
		shl dl, 1		// 0x0004d489
		or al, dl		// 0x0004d48b
		mov dl, byte ptr [esi + 0x18]		// 0x0004d48d
		inc dl		// 0x0004d490
		mov cl, dl		// 0x0004d492
		cmp cl, 8		// 0x0004d494
		mov byte ptr [esi + 0x19], al		// 0x0004d497
		mov byte ptr [esi + 0x18], dl		// 0x0004d49a
		jne L4d4ab		// 0x0004d49d
		push eax		// 0x0004d49f
		mov ecx, esi		// 0x0004d4a0
		mov byte ptr [esi + 0x18], 0		// 0x0004d4a2
		call nxPMCallStoreByte		// 0x0004d4a6
L4d4ab:
		shr ebx, 1		// 0x0004d4ab
		test ebx, ebx		// 0x0004d4ad
		jne L4d480		// 0x0004d4af
		jmp L4db34		// 0x0004d4b1
L4d4b6:
		cmp ecx, 1		// 0x0004d4b6
		jne L4d596		// 0x0004d4b9
L4d4bf:
		cmp eax, 1		// 0x0004d4bf
		jne L4d54a		// 0x0004d4c2
		mov ebx, 0x10		// 0x0004d4c8
		lea ecx, [ecx]		// 0x0004d4cd
L4d4d0:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d4d0
		mov dl, byte ptr [esi + 0x18]		// 0x0004d4d3
		test bl, 0x13		// 0x0004d4d6
		setne al		// 0x0004d4d9
		shl cl, 1		// 0x0004d4dc
		or al, cl		// 0x0004d4de
		inc dl		// 0x0004d4e0
		mov cl, dl		// 0x0004d4e2
		cmp cl, 8		// 0x0004d4e4
		mov byte ptr [esi + 0x19], al		// 0x0004d4e7
		mov byte ptr [esi + 0x18], dl		// 0x0004d4ea
		jne L4d4fb		// 0x0004d4ed
		push eax		// 0x0004d4ef
		mov ecx, esi		// 0x0004d4f0
		mov byte ptr [esi + 0x18], 0		// 0x0004d4f2
		call nxPMCallStoreByte		// 0x0004d4f6
L4d4fb:
		shr ebx, 1		// 0x0004d4fb
		test ebx, ebx		// 0x0004d4fd
		jne L4d4d0		// 0x0004d4ff
		jmp L4db34		// 0x0004d501
L4d506:
		cmp eax, 1		// 0x0004d506
		jne L4d8a4		// 0x0004d509
		mov ebx, 0x10		// 0x0004d50f
L4d514:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d514
		test bl, 0x14		// 0x0004d517
		setne al		// 0x0004d51a
		shl dl, 1		// 0x0004d51d
		or al, dl		// 0x0004d51f
		mov dl, byte ptr [esi + 0x18]		// 0x0004d521
		inc dl		// 0x0004d524
		mov cl, dl		// 0x0004d526
		cmp cl, 8		// 0x0004d528
		mov byte ptr [esi + 0x19], al		// 0x0004d52b
		mov byte ptr [esi + 0x18], dl		// 0x0004d52e
		jne L4d53f		// 0x0004d531
		push eax		// 0x0004d533
		mov ecx, esi		// 0x0004d534
		mov byte ptr [esi + 0x18], 0		// 0x0004d536
		call nxPMCallStoreByte		// 0x0004d53a
L4d53f:
		shr ebx, 1		// 0x0004d53f
		test ebx, ebx		// 0x0004d541
		jne L4d514		// 0x0004d543
		jmp L4db34		// 0x0004d545
L4d54a:
		cmp eax, -1		// 0x0004d54a
		jne L4d8a4		// 0x0004d54d
		mov ebx, 0x10		// 0x0004d553
		jmp L4d560		// 0x0004d558
		lea ebx, [ebx]		// 0x0004d55a
L4d560:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d560
		mov dl, byte ptr [esi + 0x18]		// 0x0004d563
		test bl, 0x15		// 0x0004d566
		setne al		// 0x0004d569
		shl cl, 1		// 0x0004d56c
		or al, cl		// 0x0004d56e
		inc dl		// 0x0004d570
		mov cl, dl		// 0x0004d572
		cmp cl, 8		// 0x0004d574
		mov byte ptr [esi + 0x19], al		// 0x0004d577
		mov byte ptr [esi + 0x18], dl		// 0x0004d57a
		jne L4d58b		// 0x0004d57d
		push eax		// 0x0004d57f
		mov ecx, esi		// 0x0004d580
		mov byte ptr [esi + 0x18], 0		// 0x0004d582
		call nxPMCallStoreByte		// 0x0004d586
L4d58b:
		shr ebx, 1		// 0x0004d58b
		test ebx, ebx		// 0x0004d58d
		jne L4d560		// 0x0004d58f
		jmp L4db34		// 0x0004d591
L4d596:
		cmp ecx, -1		// 0x0004d596
		jne L4d6c6		// 0x0004d599
L4d59f:
		cmp eax, -1		// 0x0004d59f
		jne L4d636		// 0x0004d5a2
		mov ebx, 0x10		// 0x0004d5a8
		lea ecx, [ecx]		// 0x0004d5ad
L4d5b0:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d5b0
		test bl, 0x16		// 0x0004d5b3
		setne al		// 0x0004d5b6
		shl dl, 1		// 0x0004d5b9
		or al, dl		// 0x0004d5bb
		mov dl, byte ptr [esi + 0x18]		// 0x0004d5bd
		inc dl		// 0x0004d5c0
		mov cl, dl		// 0x0004d5c2
		cmp cl, 8		// 0x0004d5c4
		mov byte ptr [esi + 0x19], al		// 0x0004d5c7
		mov byte ptr [esi + 0x18], dl		// 0x0004d5ca
		jne L4d5db		// 0x0004d5cd
		push eax		// 0x0004d5cf
		mov ecx, esi		// 0x0004d5d0
		mov byte ptr [esi + 0x18], 0		// 0x0004d5d2
		call nxPMCallStoreByte		// 0x0004d5d6
L4d5db:
		shr ebx, 1		// 0x0004d5db
		test ebx, ebx		// 0x0004d5dd
		jne L4d5b0		// 0x0004d5df
		jmp L4db34		// 0x0004d5e1
L4d5e6:
		cmp ecx, 1		// 0x0004d5e6
		jne L4d6c6		// 0x0004d5e9
L4d5ef:
		cmp eax, 1		// 0x0004d5ef
		jne L4d67a		// 0x0004d5f2
		mov ebx, 0x10		// 0x0004d5f8
		lea ecx, [ecx]		// 0x0004d5fd
L4d600:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d600
		mov dl, byte ptr [esi + 0x18]		// 0x0004d603
		test bl, 0x17		// 0x0004d606
		setne al		// 0x0004d609
		shl cl, 1		// 0x0004d60c
		or al, cl		// 0x0004d60e
		inc dl		// 0x0004d610
		mov cl, dl		// 0x0004d612
		cmp cl, 8		// 0x0004d614
		mov byte ptr [esi + 0x19], al		// 0x0004d617
		mov byte ptr [esi + 0x18], dl		// 0x0004d61a
		jne L4d62b		// 0x0004d61d
		push eax		// 0x0004d61f
		mov ecx, esi		// 0x0004d620
		mov byte ptr [esi + 0x18], 0		// 0x0004d622
		call nxPMCallStoreByte		// 0x0004d626
L4d62b:
		shr ebx, 1		// 0x0004d62b
		test ebx, ebx		// 0x0004d62d
		jne L4d600		// 0x0004d62f
		jmp L4db34		// 0x0004d631
L4d636:
		cmp eax, 1		// 0x0004d636
		jne L4d8a4		// 0x0004d639
		mov ebx, 0x10		// 0x0004d63f
L4d644:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d644
		test bl, 0x18		// 0x0004d647
		setne al		// 0x0004d64a
		shl dl, 1		// 0x0004d64d
		or al, dl		// 0x0004d64f
		mov dl, byte ptr [esi + 0x18]		// 0x0004d651
		inc dl		// 0x0004d654
		mov cl, dl		// 0x0004d656
		cmp cl, 8		// 0x0004d658
		mov byte ptr [esi + 0x19], al		// 0x0004d65b
		mov byte ptr [esi + 0x18], dl		// 0x0004d65e
		jne L4d66f		// 0x0004d661
		push eax		// 0x0004d663
		mov ecx, esi		// 0x0004d664
		mov byte ptr [esi + 0x18], 0		// 0x0004d666
		call nxPMCallStoreByte		// 0x0004d66a
L4d66f:
		shr ebx, 1		// 0x0004d66f
		test ebx, ebx		// 0x0004d671
		jne L4d644		// 0x0004d673
		jmp L4db34		// 0x0004d675
L4d67a:
		cmp eax, -1		// 0x0004d67a
		jne L4d8a4		// 0x0004d67d
		mov ebx, 0x10		// 0x0004d683
		jmp L4d690		// 0x0004d688
		lea ebx, [ebx]		// 0x0004d68a
L4d690:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d690
		mov dl, byte ptr [esi + 0x18]		// 0x0004d693
		test bl, 0x19		// 0x0004d696
		setne al		// 0x0004d699
		shl cl, 1		// 0x0004d69c
		or al, cl		// 0x0004d69e
		inc dl		// 0x0004d6a0
		mov cl, dl		// 0x0004d6a2
		cmp cl, 8		// 0x0004d6a4
		mov byte ptr [esi + 0x19], al		// 0x0004d6a7
		mov byte ptr [esi + 0x18], dl		// 0x0004d6aa
		jne L4d6bb		// 0x0004d6ad
		push eax		// 0x0004d6af
		mov ecx, esi		// 0x0004d6b0
		mov byte ptr [esi + 0x18], 0		// 0x0004d6b2
		call nxPMCallStoreByte		// 0x0004d6b6
L4d6bb:
		shr ebx, 1		// 0x0004d6bb
		test ebx, ebx		// 0x0004d6bd
		jne L4d690		// 0x0004d6bf
		jmp L4db34		// 0x0004d6c1
L4d6c6:
		test ecx, ecx		// 0x0004d6c6
		jne L4d75c		// 0x0004d6c8
L4d6ce:
		test eax, eax		// 0x0004d6ce
		jne L4d75c		// 0x0004d6d0
		mov ebx, 0x10		// 0x0004d6d6
		jmp L4d6e0		// 0x0004d6db
		lea ecx, [ecx]		// 0x0004d6dd
L4d6e0:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d6e0
		test bl, 0x1a		// 0x0004d6e3
		setne al		// 0x0004d6e6
		shl dl, 1		// 0x0004d6e9
		or al, dl		// 0x0004d6eb
		mov dl, byte ptr [esi + 0x18]		// 0x0004d6ed
		inc dl		// 0x0004d6f0
		mov cl, dl		// 0x0004d6f2
		cmp cl, 8		// 0x0004d6f4
		mov byte ptr [esi + 0x19], al		// 0x0004d6f7
		mov byte ptr [esi + 0x18], dl		// 0x0004d6fa
		jne L4d70b		// 0x0004d6fd
		push eax		// 0x0004d6ff
		mov ecx, esi		// 0x0004d700
		mov byte ptr [esi + 0x18], 0		// 0x0004d702
		call nxPMCallStoreByte		// 0x0004d706
L4d70b:
		shr ebx, 1		// 0x0004d70b
		test ebx, ebx		// 0x0004d70d
		jne L4d6e0		// 0x0004d70f
		lea ecx, [ebp - 1]		// 0x0004d711
		mov edi, 1		// 0x0004d714
		shl edi, cl		// 0x0004d719
		test edi, edi		// 0x0004d71b
		je L4db34		// 0x0004d71d
		mov ebx, dword ptr [esp + 0xc]		// 0x0004d723
L4d727:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d727
		mov dl, byte ptr [esi + 0x18]		// 0x0004d72a
		test ebx, edi		// 0x0004d72d
		setne al		// 0x0004d72f
		shl cl, 1		// 0x0004d732
		or al, cl		// 0x0004d734
		inc dl		// 0x0004d736
		mov cl, dl		// 0x0004d738
		cmp cl, 8		// 0x0004d73a
		mov byte ptr [esi + 0x19], al		// 0x0004d73d
		mov byte ptr [esi + 0x18], dl		// 0x0004d740
		jne L4d751		// 0x0004d743
		push eax		// 0x0004d745
		mov ecx, esi		// 0x0004d746
		mov byte ptr [esi + 0x18], 0		// 0x0004d748
		call nxPMCallStoreByte		// 0x0004d74c
L4d751:
		shr edi, 1		// 0x0004d751
		test edi, edi		// 0x0004d753
		jne L4d727		// 0x0004d755
		jmp L4db34		// 0x0004d757
L4d75c:
		test edx, edx		// 0x0004d75c
		jne L4d89c		// 0x0004d75e
		test ecx, ecx		// 0x0004d764
		je L4d7fc		// 0x0004d766
L4d76c:
		test eax, eax		// 0x0004d76c
		jne L4d7fc		// 0x0004d76e
		mov ebx, 0x10		// 0x0004d774
		lea esp, [esp]		// 0x0004d779
L4d780:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d780
		test bl, 0x1b		// 0x0004d783
		setne al		// 0x0004d786
		shl dl, 1		// 0x0004d789
		or al, dl		// 0x0004d78b
		mov dl, byte ptr [esi + 0x18]		// 0x0004d78d
		inc dl		// 0x0004d790
		mov cl, dl		// 0x0004d792
		cmp cl, 8		// 0x0004d794
		mov byte ptr [esi + 0x19], al		// 0x0004d797
		mov byte ptr [esi + 0x18], dl		// 0x0004d79a
		jne L4d7ab		// 0x0004d79d
		push eax		// 0x0004d79f
		mov ecx, esi		// 0x0004d7a0
		mov byte ptr [esi + 0x18], 0		// 0x0004d7a2
		call nxPMCallStoreByte		// 0x0004d7a6
L4d7ab:
		shr ebx, 1		// 0x0004d7ab
		test ebx, ebx		// 0x0004d7ad
		jne L4d780		// 0x0004d7af
		lea ecx, [ebp - 1]		// 0x0004d7b1
		mov edi, 1		// 0x0004d7b4
		shl edi, cl		// 0x0004d7b9
		test edi, edi		// 0x0004d7bb
		je L4db34		// 0x0004d7bd
		mov ebx, dword ptr [esp + 0x10]		// 0x0004d7c3
L4d7c7:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d7c7
		mov dl, byte ptr [esi + 0x18]		// 0x0004d7ca
		test ebx, edi		// 0x0004d7cd
		setne al		// 0x0004d7cf
		shl cl, 1		// 0x0004d7d2
		or al, cl		// 0x0004d7d4
		inc dl		// 0x0004d7d6
		mov cl, dl		// 0x0004d7d8
		cmp cl, 8		// 0x0004d7da
		mov byte ptr [esi + 0x19], al		// 0x0004d7dd
		mov byte ptr [esi + 0x18], dl		// 0x0004d7e0
		jne L4d7f1		// 0x0004d7e3
		push eax		// 0x0004d7e5
		mov ecx, esi		// 0x0004d7e6
		mov byte ptr [esi + 0x18], 0		// 0x0004d7e8
		call nxPMCallStoreByte		// 0x0004d7ec
L4d7f1:
		shr edi, 1		// 0x0004d7f1
		test edi, edi		// 0x0004d7f3
		jne L4d7c7		// 0x0004d7f5
		jmp L4db34		// 0x0004d7f7
L4d7fc:
		test edx, edx		// 0x0004d7fc
		jne L4d89c		// 0x0004d7fe
		test ecx, ecx		// 0x0004d804
		jne L4da3a		// 0x0004d806
L4d80c:
		test eax, eax		// 0x0004d80c
		je L4da3a		// 0x0004d80e
		mov ebx, 0x10		// 0x0004d814
		lea esp, [esp]		// 0x0004d819
L4d820:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d820
		test bl, 0x1c		// 0x0004d823
		setne al		// 0x0004d826
		shl dl, 1		// 0x0004d829
		or al, dl		// 0x0004d82b
		mov dl, byte ptr [esi + 0x18]		// 0x0004d82d
		inc dl		// 0x0004d830
		mov cl, dl		// 0x0004d832
		cmp cl, 8		// 0x0004d834
		mov byte ptr [esi + 0x19], al		// 0x0004d837
		mov byte ptr [esi + 0x18], dl		// 0x0004d83a
		jne L4d84b		// 0x0004d83d
		push eax		// 0x0004d83f
		mov ecx, esi		// 0x0004d840
		mov byte ptr [esi + 0x18], 0		// 0x0004d842
		call nxPMCallStoreByte		// 0x0004d846
L4d84b:
		shr ebx, 1		// 0x0004d84b
		test ebx, ebx		// 0x0004d84d
		jne L4d820		// 0x0004d84f
		lea ecx, [ebp - 1]		// 0x0004d851
		mov edi, 1		// 0x0004d854
		shl edi, cl		// 0x0004d859
		test edi, edi		// 0x0004d85b
		je L4db34		// 0x0004d85d
		mov ebx, dword ptr [esp + 0x14]		// 0x0004d863
L4d867:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d867
		mov dl, byte ptr [esi + 0x18]		// 0x0004d86a
		test ebx, edi		// 0x0004d86d
		setne al		// 0x0004d86f
		shl cl, 1		// 0x0004d872
		or al, cl		// 0x0004d874
		inc dl		// 0x0004d876
		mov cl, dl		// 0x0004d878
		cmp cl, 8		// 0x0004d87a
		mov byte ptr [esi + 0x19], al		// 0x0004d87d
		mov byte ptr [esi + 0x18], dl		// 0x0004d880
		jne L4d891		// 0x0004d883
		push eax		// 0x0004d885
		mov ecx, esi		// 0x0004d886
		mov byte ptr [esi + 0x18], 0		// 0x0004d888
		call nxPMCallStoreByte		// 0x0004d88c
L4d891:
		shr edi, 1		// 0x0004d891
		test edi, edi		// 0x0004d893
		jne L4d867		// 0x0004d895
		jmp L4db34		// 0x0004d897
L4d89c:
		test ecx, ecx		// 0x0004d89c
		je L4d967		// 0x0004d89e
L4d8a4:
		test eax, eax		// 0x0004d8a4
		jne L4d967		// 0x0004d8a6
		mov ebx, 0x10		// 0x0004d8ac
L4d8b1:
		mov dl, byte ptr [esi + 0x19]		// 0x0004d8b1
		test bl, 0x1d		// 0x0004d8b4
		setne al		// 0x0004d8b7
		shl dl, 1		// 0x0004d8ba
		or al, dl		// 0x0004d8bc
		mov dl, byte ptr [esi + 0x18]		// 0x0004d8be
		inc dl		// 0x0004d8c1
		mov cl, dl		// 0x0004d8c3
		cmp cl, 8		// 0x0004d8c5
		mov byte ptr [esi + 0x19], al		// 0x0004d8c8
		mov byte ptr [esi + 0x18], dl		// 0x0004d8cb
		jne L4d8dc		// 0x0004d8ce
		push eax		// 0x0004d8d0
		mov ecx, esi		// 0x0004d8d1
		mov byte ptr [esi + 0x18], 0		// 0x0004d8d3
		call nxPMCallStoreByte		// 0x0004d8d7
L4d8dc:
		shr ebx, 1		// 0x0004d8dc
		test ebx, ebx		// 0x0004d8de
		jne L4d8b1		// 0x0004d8e0
		lea ecx, [ebp - 1]		// 0x0004d8e2
		mov ebx, 1		// 0x0004d8e5
		shl ebx, cl		// 0x0004d8ea
		mov edi, ebx		// 0x0004d8ec
		test edi, edi		// 0x0004d8ee
		je L4d926		// 0x0004d8f0
L4d8f2:
		mov eax, dword ptr [esp + 0xc]		// 0x0004d8f2
		mov cl, byte ptr [esi + 0x19]		// 0x0004d8f6
		mov dl, byte ptr [esi + 0x18]		// 0x0004d8f9
		test edi, eax		// 0x0004d8fc
		setne al		// 0x0004d8fe
		shl cl, 1		// 0x0004d901
		or al, cl		// 0x0004d903
		inc dl		// 0x0004d905
		mov cl, dl		// 0x0004d907
		cmp cl, 8		// 0x0004d909
		mov byte ptr [esi + 0x19], al		// 0x0004d90c
		mov byte ptr [esi + 0x18], dl		// 0x0004d90f
		jne L4d920		// 0x0004d912
		push eax		// 0x0004d914
		mov ecx, esi		// 0x0004d915
		mov byte ptr [esi + 0x18], 0		// 0x0004d917
		call nxPMCallStoreByte		// 0x0004d91b
L4d920:
		shr edi, 1		// 0x0004d920
		test edi, edi		// 0x0004d922
		jne L4d8f2		// 0x0004d924
L4d926:
		mov edi, ebx		// 0x0004d926
		test edi, edi		// 0x0004d928
		je L4db34		// 0x0004d92a
L4d930:
		test dword ptr [esp + 0x10], edi		// 0x0004d930
		mov dl, byte ptr [esi + 0x19]		// 0x0004d934
		setne al		// 0x0004d937
		shl dl, 1		// 0x0004d93a
		or al, dl		// 0x0004d93c
		mov dl, byte ptr [esi + 0x18]		// 0x0004d93e
		inc dl		// 0x0004d941
		mov cl, dl		// 0x0004d943
		cmp cl, 8		// 0x0004d945
		mov byte ptr [esi + 0x19], al		// 0x0004d948
		mov byte ptr [esi + 0x18], dl		// 0x0004d94b
		jne L4d95c		// 0x0004d94e
		push eax		// 0x0004d950
		mov ecx, esi		// 0x0004d951
		mov byte ptr [esi + 0x18], 0		// 0x0004d953
		call nxPMCallStoreByte		// 0x0004d957
L4d95c:
		shr edi, 1		// 0x0004d95c
		test edi, edi		// 0x0004d95e
		jne L4d930		// 0x0004d960
		jmp L4db34		// 0x0004d962
L4d967:
		test edx, edx		// 0x0004d967
		je L4da3a		// 0x0004d969
		test ecx, ecx		// 0x0004d96f
		jne L4da3a		// 0x0004d971
		test eax, eax		// 0x0004d977
		je L4da3a		// 0x0004d979
		mov ebx, 0x10		// 0x0004d97f
L4d984:
		mov cl, byte ptr [esi + 0x19]		// 0x0004d984
		mov dl, byte ptr [esi + 0x18]		// 0x0004d987
		test bl, 0x1e		// 0x0004d98a
		setne al		// 0x0004d98d
		shl cl, 1		// 0x0004d990
		or al, cl		// 0x0004d992
		inc dl		// 0x0004d994
		mov cl, dl		// 0x0004d996
		cmp cl, 8		// 0x0004d998
		mov byte ptr [esi + 0x19], al		// 0x0004d99b
		mov byte ptr [esi + 0x18], dl		// 0x0004d99e
		jne L4d9af		// 0x0004d9a1
		push eax		// 0x0004d9a3
		mov ecx, esi		// 0x0004d9a4
		mov byte ptr [esi + 0x18], 0		// 0x0004d9a6
		call nxPMCallStoreByte		// 0x0004d9aa
L4d9af:
		shr ebx, 1		// 0x0004d9af
		test ebx, ebx		// 0x0004d9b1
		jne L4d984		// 0x0004d9b3
		lea ecx, [ebp - 1]		// 0x0004d9b5
		mov ebx, 1		// 0x0004d9b8
		shl ebx, cl		// 0x0004d9bd
		mov edi, ebx		// 0x0004d9bf
		test edi, edi		// 0x0004d9c1
		je L4d9f7		// 0x0004d9c3
L4d9c5:
		test dword ptr [esp + 0xc], edi		// 0x0004d9c5
		mov dl, byte ptr [esi + 0x19]		// 0x0004d9c9
		setne al		// 0x0004d9cc
		shl dl, 1		// 0x0004d9cf
		or al, dl		// 0x0004d9d1
		mov dl, byte ptr [esi + 0x18]		// 0x0004d9d3
		inc dl		// 0x0004d9d6
		mov cl, dl		// 0x0004d9d8
		cmp cl, 8		// 0x0004d9da
		mov byte ptr [esi + 0x19], al		// 0x0004d9dd
		mov byte ptr [esi + 0x18], dl		// 0x0004d9e0
		jne L4d9f1		// 0x0004d9e3
		push eax		// 0x0004d9e5
		mov ecx, esi		// 0x0004d9e6
		mov byte ptr [esi + 0x18], 0		// 0x0004d9e8
		call nxPMCallStoreByte		// 0x0004d9ec
L4d9f1:
		shr edi, 1		// 0x0004d9f1
		test edi, edi		// 0x0004d9f3
		jne L4d9c5		// 0x0004d9f5
L4d9f7:
		mov edi, ebx		// 0x0004d9f7
		test edi, edi		// 0x0004d9f9
		je L4db34		// 0x0004d9fb
L4da01:
		mov ebx, dword ptr [esp + 0x14]		// 0x0004da01
		mov cl, byte ptr [esi + 0x19]		// 0x0004da05
		mov dl, byte ptr [esi + 0x18]		// 0x0004da08
		test edi, ebx		// 0x0004da0b
		setne al		// 0x0004da0d
		shl cl, 1		// 0x0004da10
		or al, cl		// 0x0004da12
		inc dl		// 0x0004da14
		mov cl, dl		// 0x0004da16
		cmp cl, 8		// 0x0004da18
		mov byte ptr [esi + 0x19], al		// 0x0004da1b
		mov byte ptr [esi + 0x18], dl		// 0x0004da1e
		jne L4da2f		// 0x0004da21
		push eax		// 0x0004da23
		mov ecx, esi		// 0x0004da24
		mov byte ptr [esi + 0x18], 0		// 0x0004da26
		call nxPMCallStoreByte		// 0x0004da2a
L4da2f:
		shr edi, 1		// 0x0004da2f
		test edi, edi		// 0x0004da31
		jne L4da01		// 0x0004da33
		jmp L4db34		// 0x0004da35
L4da3a:
		mov ebx, 0x10		// 0x0004da3a
		nop		// 0x0004da3f
L4da40:
		mov dl, byte ptr [esi + 0x19]		// 0x0004da40
		test bl, 0x1f		// 0x0004da43
		setne al		// 0x0004da46
		shl dl, 1		// 0x0004da49
		or al, dl		// 0x0004da4b
		mov dl, byte ptr [esi + 0x18]		// 0x0004da4d
		inc dl		// 0x0004da50
		mov cl, dl		// 0x0004da52
		cmp cl, 8		// 0x0004da54
		mov byte ptr [esi + 0x19], al		// 0x0004da57
		mov byte ptr [esi + 0x18], dl		// 0x0004da5a
		jne L4da6b		// 0x0004da5d
		push eax		// 0x0004da5f
		mov ecx, esi		// 0x0004da60
		mov byte ptr [esi + 0x18], 0		// 0x0004da62
		call nxPMCallStoreByte		// 0x0004da66
L4da6b:
		shr ebx, 1		// 0x0004da6b
		test ebx, ebx		// 0x0004da6d
		jne L4da40		// 0x0004da6f
		lea ecx, [ebp - 1]		// 0x0004da71
		mov ebx, 1		// 0x0004da74
		shl ebx, cl		// 0x0004da79
		mov edi, ebx		// 0x0004da7b
		test edi, edi		// 0x0004da7d
		je L4dab5		// 0x0004da7f
L4da81:
		mov eax, dword ptr [esp + 0xc]		// 0x0004da81
		mov cl, byte ptr [esi + 0x19]		// 0x0004da85
		mov dl, byte ptr [esi + 0x18]		// 0x0004da88
		test edi, eax		// 0x0004da8b
		setne al		// 0x0004da8d
		shl cl, 1		// 0x0004da90
		or al, cl		// 0x0004da92
		inc dl		// 0x0004da94
		mov cl, dl		// 0x0004da96
		cmp cl, 8		// 0x0004da98
		mov byte ptr [esi + 0x19], al		// 0x0004da9b
		mov byte ptr [esi + 0x18], dl		// 0x0004da9e
		jne L4daaf		// 0x0004daa1
		push eax		// 0x0004daa3
		mov ecx, esi		// 0x0004daa4
		mov byte ptr [esi + 0x18], 0		// 0x0004daa6
		call nxPMCallStoreByte		// 0x0004daaa
L4daaf:
		shr edi, 1		// 0x0004daaf
		test edi, edi		// 0x0004dab1
		jne L4da81		// 0x0004dab3
L4dab5:
		mov edi, ebx		// 0x0004dab5
		test edi, edi		// 0x0004dab7
		je L4daf2		// 0x0004dab9
		jmp L4dac0		// 0x0004dabb
		lea ecx, [ecx]		// 0x0004dabd
L4dac0:
		test dword ptr [esp + 0x10], edi		// 0x0004dac0
		mov dl, byte ptr [esi + 0x19]		// 0x0004dac4
		setne al		// 0x0004dac7
		shl dl, 1		// 0x0004daca
		or al, dl		// 0x0004dacc
		mov dl, byte ptr [esi + 0x18]		// 0x0004dace
		inc dl		// 0x0004dad1
		mov cl, dl		// 0x0004dad3
		cmp cl, 8		// 0x0004dad5
		mov byte ptr [esi + 0x19], al		// 0x0004dad8
		mov byte ptr [esi + 0x18], dl		// 0x0004dadb
		jne L4daec		// 0x0004dade
		push eax		// 0x0004dae0
		mov ecx, esi		// 0x0004dae1
		mov byte ptr [esi + 0x18], 0		// 0x0004dae3
		call nxPMCallStoreByte		// 0x0004dae7
L4daec:
		shr edi, 1		// 0x0004daec
		test edi, edi		// 0x0004daee
		jne L4dac0		// 0x0004daf0
L4daf2:
		mov edi, ebx		// 0x0004daf2
		test edi, edi		// 0x0004daf4
		je L4db34		// 0x0004daf6
		jmp L4db00		// 0x0004daf8
		lea ebx, [ebx]		// 0x0004dafa
L4db00:
		mov ebx, dword ptr [esp + 0x14]		// 0x0004db00
		mov cl, byte ptr [esi + 0x19]		// 0x0004db04
		mov dl, byte ptr [esi + 0x18]		// 0x0004db07
		test edi, ebx		// 0x0004db0a
		setne al		// 0x0004db0c
		shl cl, 1		// 0x0004db0f
		or al, cl		// 0x0004db11
		inc dl		// 0x0004db13
		mov cl, dl		// 0x0004db15
		cmp cl, 8		// 0x0004db17
		mov byte ptr [esi + 0x19], al		// 0x0004db1a
		mov byte ptr [esi + 0x18], dl		// 0x0004db1d
		jne L4db2e		// 0x0004db20
		push eax		// 0x0004db22
		mov ecx, esi		// 0x0004db23
		mov byte ptr [esi + 0x18], 0		// 0x0004db25
		call nxPMCallStoreByte		// 0x0004db29
L4db2e:
		shr edi, 1		// 0x0004db2e
		test edi, edi		// 0x0004db30
		jne L4db00		// 0x0004db32
L4db34:
		mov eax, dword ptr [esp + 0x18]		// 0x0004db34
		mov edx, dword ptr [esp + 0x1c]		// 0x0004db38
		mov ecx, dword ptr [esp + 0xc]		// 0x0004db3c
		mov edi, dword ptr [esp + 0x10]		// 0x0004db40
		mov ebx, dword ptr [esp + 0x14]		// 0x0004db44
		inc eax		// 0x0004db48
		cmp eax, edx		// 0x0004db49
		mov dword ptr [nxPMapCursor], ecx		// 0x0004db4b
		mov dword ptr [nxPMapCursor + 4], edi		// 0x0004db51
		mov dword ptr [nxPMapCursor + 8], ebx		// 0x0004db57
		mov dword ptr [esp + 0x18], eax		// 0x0004db5d
		jb L4cee0		// 0x0004db61
		lea ecx, [esp + 0x74]		// 0x0004db67
		call nxPMapRelease5159		// 0x0004db6b
		lea ecx, [esp + 0x34]		// 0x0004db70
		call nxPMCallEmpty		// 0x0004db74
		lea ecx, [esp + 0x44]		// 0x0004db79
		call nxPMCallEmpty		// 0x0004db7d
		lea ecx, [esp + 0x54]		// 0x0004db82
		call nxPMCallEmpty		// 0x0004db86
		lea ecx, [esp + 0x64]		// 0x0004db8b
		call nxPMCallEmpty		// 0x0004db8f
		pop edi		// 0x0004db94
		pop ebp		// 0x0004db95
		pop ebx		// 0x0004db96
		add esp, 0x80		// 0x0004db97
		ret		// 0x0004db9d
		}
	}

// phys_fn_002008 with 002010, 002012 and 002014 (one function: 002008 falls through into the rows the inventory splits off; 0x0004e0fc calls its own loop label 0x0004dc22) (0x0004dba0, 1394 B), 445 instructions
__declspec(naked) unsigned __fastcall nxPMapDecodeCellRun(void* self, void* edxUnused, unsigned a0, unsigned a1)
	{
	__asm
		{
		mov eax, dword ptr [esp + 8]		// 0x0004dba0
		sub esp, 8		// 0x0004dba4
		push ebx		// 0x0004dba7
		push ebp		// 0x0004dba8
		xor ebx, ebx		// 0x0004dba9
		cmp eax, 0x20		// 0x0004dbab
		push edi		// 0x0004dbae
		jne L4dbb8		// 0x0004dbaf
		mov ebx, 5		// 0x0004dbb1
		jmp L4dbce		// 0x0004dbb6
L4dbb8:
		cmp eax, 0x40		// 0x0004dbb8
		jne L4dbc4		// 0x0004dbbb
		mov ebx, 6		// 0x0004dbbd
		jmp L4dbce		// 0x0004dbc2
L4dbc4:
		cmp eax, 0x50		// 0x0004dbc4
		jne L4dbce		// 0x0004dbc7
		mov ebx, 7		// 0x0004dbc9
L4dbce:
		xor ebp, ebp		// 0x0004dbce
		mov edi, 0x20		// 0x0004dbd0
L4dbd5:
		mov al, byte ptr [esi + 0x18]		// 0x0004dbd5
		shl ebp, 1		// 0x0004dbd8
		test al, al		// 0x0004dbda
		jne L4dbec		// 0x0004dbdc
		mov ecx, esi		// 0x0004dbde
		call nxPMCallReadByte		// 0x0004dbe0
		mov byte ptr [esi + 0x19], al		// 0x0004dbe5
		mov byte ptr [esi + 0x18], 0x80		// 0x0004dbe8
L4dbec:
		mov al, byte ptr [esi + 0x18]		// 0x0004dbec
		test byte ptr [esi + 0x19], al		// 0x0004dbef
		setne cl		// 0x0004dbf2
		shr al, 1		// 0x0004dbf5
		mov byte ptr [esi + 0x18], al		// 0x0004dbf7
		movzx eax, cl		// 0x0004dbfa
		or ebp, eax		// 0x0004dbfd
		dec edi		// 0x0004dbff
		jne L4dbd5		// 0x0004dc00
		mov eax, dword ptr [esp + 0x18]		// 0x0004dc02
		mov edx, dword ptr [eax + 4]		// 0x0004dc06
		xor ecx, ecx		// 0x0004dc09
		cmp edx, ecx		// 0x0004dc0b
		mov dword ptr [esp + 0x10], ebp		// 0x0004dc0d
		je L4dc16		// 0x0004dc11
		mov dword ptr [eax + 4], ecx		// 0x0004dc13
L4dc16:
		cmp ebp, ecx		// 0x0004dc16
		mov dword ptr [esp + 0xc], ecx		// 0x0004dc18
		jbe L4e109		// 0x0004dc1c
L4dc22:
		xor edi, edi		// 0x0004dc22
		mov ebp, 5		// 0x0004dc24
		lea esp, [esp]		// 0x0004dc29
L4dc30:
		mov al, byte ptr [esi + 0x18]		// 0x0004dc30
		shl edi, 1		// 0x0004dc33
		test al, al		// 0x0004dc35
		jne L4dc47		// 0x0004dc37
		mov ecx, esi		// 0x0004dc39
		call nxPMCallReadByte		// 0x0004dc3b
		mov byte ptr [esi + 0x19], al		// 0x0004dc40
		mov byte ptr [esi + 0x18], 0x80		// 0x0004dc43
L4dc47:
		mov al, byte ptr [esi + 0x18]		// 0x0004dc47
		test byte ptr [esi + 0x19], al		// 0x0004dc4a
		setne cl		// 0x0004dc4d
		movzx ecx, cl		// 0x0004dc50
		shr al, 1		// 0x0004dc53
		or edi, ecx		// 0x0004dc55
		dec ebp		// 0x0004dc57
		mov byte ptr [esi + 0x18], al		// 0x0004dc58
		jne L4dc30		// 0x0004dc5b
		mov ecx, dword ptr [edi*4 + nxPMapCodeCounts]		// 0x0004dc5d
		inc ecx		// 0x0004dc64
		cmp edi, 0x1f		// 0x0004dc65
		mov dword ptr [edi*4 + nxPMapCodeCounts], ecx		// 0x0004dc68
		ja L4e0a3		// 0x0004dc6f
		cmp edi, 0		// jump table 0x0004e114[0]
		je L4dc7c
		cmp edi, 1		// jump table 0x0004e114[1]
		je L4dc87
		cmp edi, 2		// jump table 0x0004e114[2]
		je L4dcd9
		cmp edi, 3		// jump table 0x0004e114[3]
		je L4dc92
		cmp edi, 4		// jump table 0x0004e114[4]
		je L4de17
		cmp edi, 5		// jump table 0x0004e114[5]
		je L4dd76
		cmp edi, 6		// jump table 0x0004e114[6]
		je L4dc9d
		cmp edi, 7		// jump table 0x0004e114[7]
		je L4dca6
		cmp edi, 8		// jump table 0x0004e114[8]
		je L4dcaf
		cmp edi, 9		// jump table 0x0004e114[9]
		je L4dccc
		cmp edi, 10		// jump table 0x0004e114[10]
		je L4dd5e
		cmp edi, 11		// jump table 0x0004e114[11]
		je L4dd70
		cmp edi, 12		// jump table 0x0004e114[12]
		je L4dd87
		cmp edi, 13		// jump table 0x0004e114[13]
		je L4de0a
		cmp edi, 14		// jump table 0x0004e114[14]
		je L4dce4
		cmp edi, 15		// jump table 0x0004e114[15]
		je L4dd01
		cmp edi, 16		// jump table 0x0004e114[16]
		je L4dd1e
		cmp edi, 17		// jump table 0x0004e114[17]
		je L4dd3b
		cmp edi, 18		// jump table 0x0004e114[18]
		je L4dd58
		cmp edi, 19		// jump table 0x0004e114[19]
		je L4dd6a
		cmp edi, 20		// jump table 0x0004e114[20]
		je L4dd81
		cmp edi, 21		// jump table 0x0004e114[21]
		je L4dda4
		cmp edi, 22		// jump table 0x0004e114[22]
		je L4ddad
		cmp edi, 23		// jump table 0x0004e114[23]
		je L4ddc3
		cmp edi, 24		// jump table 0x0004e114[24]
		je L4ddd3
		cmp edi, 25		// jump table 0x0004e114[25]
		je L4ddfd
		cmp edi, 26		// jump table 0x0004e114[26]
		je L4de22
		cmp edi, 27		// jump table 0x0004e114[27]
		je L4de68
		cmp edi, 28		// jump table 0x0004e114[28]
		je L4dea8
		cmp edi, 29		// jump table 0x0004e114[29]
		je L4dee6
		cmp edi, 30		// jump table 0x0004e114[30]
		je L4df68
		jmp L4dfe2		// jump table 0x0004e114[31]		// 0x0004dc75
L4dc7c:
		dec dword ptr [nxPMapCursor]		// 0x0004dc7c
		jmp L4e0a3		// 0x0004dc82
L4dc87:
		inc dword ptr [nxPMapCursor]		// 0x0004dc87
		jmp L4e0a3		// 0x0004dc8d
L4dc92:
		inc dword ptr [nxPMapCursor + 4]		// 0x0004dc92
		jmp L4e0a3		// 0x0004dc98
L4dc9d:
		mov ecx, dword ptr [nxPMapCursor]		// 0x0004dc9d
		dec ecx		// 0x0004dca3
		jmp L4dcd3		// 0x0004dca4
L4dca6:
		mov ecx, dword ptr [nxPMapCursor]		// 0x0004dca6
		inc ecx		// 0x0004dcac
		jmp L4dcb6		// 0x0004dcad
L4dcaf:
		mov ecx, dword ptr [nxPMapCursor]		// 0x0004dcaf
		dec ecx		// 0x0004dcb5
L4dcb6:
		mov eax, dword ptr [nxPMapCursor + 4]		// 0x0004dcb6
		inc eax		// 0x0004dcbb
		mov dword ptr [nxPMapCursor], ecx		// 0x0004dcbc
		mov dword ptr [nxPMapCursor + 4], eax		// 0x0004dcc2
		jmp L4e0a3		// 0x0004dcc7
L4dccc:
		mov ecx, dword ptr [nxPMapCursor]		// 0x0004dccc
		inc ecx		// 0x0004dcd2
L4dcd3:
		mov dword ptr [nxPMapCursor], ecx		// 0x0004dcd3
L4dcd9:
		dec dword ptr [nxPMapCursor + 4]		// 0x0004dcd9
		jmp L4e0a3		// 0x0004dcdf
L4dce4:
		mov ecx, dword ptr [nxPMapCursor]		// 0x0004dce4
		mov eax, dword ptr [nxPMapCursor + 8]		// 0x0004dcea
		dec ecx		// 0x0004dcef
		dec eax		// 0x0004dcf0
		mov dword ptr [nxPMapCursor], ecx		// 0x0004dcf1
		mov dword ptr [nxPMapCursor + 8], eax		// 0x0004dcf7
		jmp L4e0a3		// 0x0004dcfc
L4dd01:
		mov ecx, dword ptr [nxPMapCursor]		// 0x0004dd01
		mov eax, dword ptr [nxPMapCursor + 8]		// 0x0004dd07
		inc ecx		// 0x0004dd0c
		inc eax		// 0x0004dd0d
		mov dword ptr [nxPMapCursor], ecx		// 0x0004dd0e
		mov dword ptr [nxPMapCursor + 8], eax		// 0x0004dd14
		jmp L4e0a3		// 0x0004dd19
L4dd1e:
		mov ecx, dword ptr [nxPMapCursor]		// 0x0004dd1e
		mov eax, dword ptr [nxPMapCursor + 8]		// 0x0004dd24
		dec ecx		// 0x0004dd29
		inc eax		// 0x0004dd2a
		mov dword ptr [nxPMapCursor], ecx		// 0x0004dd2b
		mov dword ptr [nxPMapCursor + 8], eax		// 0x0004dd31
		jmp L4e0a3		// 0x0004dd36
L4dd3b:
		mov ecx, dword ptr [nxPMapCursor]		// 0x0004dd3b
		mov eax, dword ptr [nxPMapCursor + 8]		// 0x0004dd41
		inc ecx		// 0x0004dd46
		dec eax		// 0x0004dd47
		mov dword ptr [nxPMapCursor], ecx		// 0x0004dd48
		mov dword ptr [nxPMapCursor + 8], eax		// 0x0004dd4e
		jmp L4e0a3		// 0x0004dd53
L4dd58:
		dec dword ptr [nxPMapCursor]		// 0x0004dd58
L4dd5e:
		mov ecx, dword ptr [nxPMapCursor + 4]		// 0x0004dd5e
		dec ecx		// 0x0004dd64
		jmp L4de11		// 0x0004dd65
L4dd6a:
		inc dword ptr [nxPMapCursor]		// 0x0004dd6a
L4dd70:
		inc dword ptr [nxPMapCursor + 4]		// 0x0004dd70
L4dd76:
		inc dword ptr [nxPMapCursor + 8]		// 0x0004dd76
		jmp L4e0a3		// 0x0004dd7c
L4dd81:
		dec dword ptr [nxPMapCursor]		// 0x0004dd81
L4dd87:
		mov ecx, dword ptr [nxPMapCursor + 4]		// 0x0004dd87
		mov eax, dword ptr [nxPMapCursor + 8]		// 0x0004dd8d
		dec ecx		// 0x0004dd92
		inc eax		// 0x0004dd93
		mov dword ptr [nxPMapCursor + 4], ecx		// 0x0004dd94
		mov dword ptr [nxPMapCursor + 8], eax		// 0x0004dd9a
		jmp L4e0a3		// 0x0004dd9f
L4dda4:
		mov edx, dword ptr [nxPMapCursor]		// 0x0004dda4
		inc edx		// 0x0004ddaa
		jmp L4de04		// 0x0004ddab
L4ddad:
		mov edx, dword ptr [nxPMapCursor]		// 0x0004ddad
		mov ecx, dword ptr [nxPMapCursor + 4]		// 0x0004ddb3
		inc edx		// 0x0004ddb9
		mov dword ptr [nxPMapCursor], edx		// 0x0004ddba
		dec ecx		// 0x0004ddc0
		jmp L4de11		// 0x0004ddc1
L4ddc3:
		mov edx, dword ptr [nxPMapCursor]		// 0x0004ddc3
		mov ecx, dword ptr [nxPMapCursor + 4]		// 0x0004ddc9
		dec edx		// 0x0004ddcf
		inc ecx		// 0x0004ddd0
		jmp L4dde1		// 0x0004ddd1
L4ddd3:
		mov edx, dword ptr [nxPMapCursor]		// 0x0004ddd3
		mov ecx, dword ptr [nxPMapCursor + 4]		// 0x0004ddd9
		inc edx		// 0x0004dddf
		dec ecx		// 0x0004dde0
L4dde1:
		mov eax, dword ptr [nxPMapCursor + 8]		// 0x0004dde1
		inc eax		// 0x0004dde6
		mov dword ptr [nxPMapCursor], edx		// 0x0004dde7
		mov dword ptr [nxPMapCursor + 4], ecx		// 0x0004dded
		mov dword ptr [nxPMapCursor + 8], eax		// 0x0004ddf3
		jmp L4e0a3		// 0x0004ddf8
L4ddfd:
		mov edx, dword ptr [nxPMapCursor]		// 0x0004ddfd
		dec edx		// 0x0004de03
L4de04:
		mov dword ptr [nxPMapCursor], edx		// 0x0004de04
L4de0a:
		mov ecx, dword ptr [nxPMapCursor + 4]		// 0x0004de0a
		inc ecx		// 0x0004de10
L4de11:
		mov dword ptr [nxPMapCursor + 4], ecx		// 0x0004de11
L4de17:
		dec dword ptr [nxPMapCursor + 8]		// 0x0004de17
		jmp L4e0a3		// 0x0004de1d
L4de22:
		xor edi, edi		// 0x0004de22
		test ebx, ebx		// 0x0004de24
		je L4de5d		// 0x0004de26
		mov ebp, ebx		// 0x0004de28
		lea ebx, [ebx]		// 0x0004de2a
L4de30:
		mov al, byte ptr [esi + 0x18]		// 0x0004de30
		shl edi, 1		// 0x0004de33
		test al, al		// 0x0004de35
		jne L4de47		// 0x0004de37
		mov ecx, esi		// 0x0004de39
		call nxPMCallReadByte		// 0x0004de3b
		mov byte ptr [esi + 0x19], al		// 0x0004de40
		mov byte ptr [esi + 0x18], 0x80		// 0x0004de43
L4de47:
		mov al, byte ptr [esi + 0x18]		// 0x0004de47
		test byte ptr [esi + 0x19], al		// 0x0004de4a
		setne cl		// 0x0004de4d
		movzx edx, cl		// 0x0004de50
		shr al, 1		// 0x0004de53
		or edi, edx		// 0x0004de55
		dec ebp		// 0x0004de57
		mov byte ptr [esi + 0x18], al		// 0x0004de58
		jne L4de30		// 0x0004de5b
L4de5d:
		mov dword ptr [nxPMapCursor], edi		// 0x0004de5d
		jmp L4e0a3		// 0x0004de63
L4de68:
		xor edi, edi		// 0x0004de68
		test ebx, ebx		// 0x0004de6a
		je L4de9d		// 0x0004de6c
		mov ebp, ebx		// 0x0004de6e
L4de70:
		mov al, byte ptr [esi + 0x18]		// 0x0004de70
		shl edi, 1		// 0x0004de73
		test al, al		// 0x0004de75
		jne L4de87		// 0x0004de77
		mov ecx, esi		// 0x0004de79
		call nxPMCallReadByte		// 0x0004de7b
		mov byte ptr [esi + 0x19], al		// 0x0004de80
		mov byte ptr [esi + 0x18], 0x80		// 0x0004de83
L4de87:
		mov al, byte ptr [esi + 0x18]		// 0x0004de87
		test byte ptr [esi + 0x19], al		// 0x0004de8a
		setne cl		// 0x0004de8d
		shr al, 1		// 0x0004de90
		mov byte ptr [esi + 0x18], al		// 0x0004de92
		movzx eax, cl		// 0x0004de95
		or edi, eax		// 0x0004de98
		dec ebp		// 0x0004de9a
		jne L4de70		// 0x0004de9b
L4de9d:
		mov dword ptr [nxPMapCursor + 4], edi		// 0x0004de9d
		jmp L4e0a3		// 0x0004dea3
L4dea8:
		xor edi, edi		// 0x0004dea8
		test ebx, ebx		// 0x0004deaa
		je L4e09d		// 0x0004deac
		mov ebp, ebx		// 0x0004deb2
L4deb4:
		mov al, byte ptr [esi + 0x18]		// 0x0004deb4
		shl edi, 1		// 0x0004deb7
		test al, al		// 0x0004deb9
		jne L4decb		// 0x0004debb
		mov ecx, esi		// 0x0004debd
		call nxPMCallReadByte		// 0x0004debf
		mov byte ptr [esi + 0x19], al		// 0x0004dec4
		mov byte ptr [esi + 0x18], 0x80		// 0x0004dec7
L4decb:
		mov al, byte ptr [esi + 0x18]		// 0x0004decb
		test byte ptr [esi + 0x19], al		// 0x0004dece
		setne cl		// 0x0004ded1
		movzx ecx, cl		// 0x0004ded4
		shr al, 1		// 0x0004ded7
		or edi, ecx		// 0x0004ded9
		dec ebp		// 0x0004dedb
		mov byte ptr [esi + 0x18], al		// 0x0004dedc
		jne L4deb4		// 0x0004dedf
		jmp L4e09d		// 0x0004dee1
L4dee6:
		xor edi, edi		// 0x0004dee6
		test ebx, ebx		// 0x0004dee8
		je L4df1d		// 0x0004deea
		mov ebp, ebx		// 0x0004deec
		mov edi, edi		// 0x0004deee
L4def0:
		mov al, byte ptr [esi + 0x18]		// 0x0004def0
		shl edi, 1		// 0x0004def3
		test al, al		// 0x0004def5
		jne L4df07		// 0x0004def7
		mov ecx, esi		// 0x0004def9
		call nxPMCallReadByte		// 0x0004defb
		mov byte ptr [esi + 0x19], al		// 0x0004df00
		mov byte ptr [esi + 0x18], 0x80		// 0x0004df03
L4df07:
		mov al, byte ptr [esi + 0x18]		// 0x0004df07
		test byte ptr [esi + 0x19], al		// 0x0004df0a
		setne cl		// 0x0004df0d
		movzx edx, cl		// 0x0004df10
		shr al, 1		// 0x0004df13
		or edi, edx		// 0x0004df15
		dec ebp		// 0x0004df17
		mov byte ptr [esi + 0x18], al		// 0x0004df18
		jne L4def0		// 0x0004df1b
L4df1d:
		mov dword ptr [nxPMapCursor], edi		// 0x0004df1d
		xor edi, edi		// 0x0004df23
		test ebx, ebx		// 0x0004df25
		je L4df5d		// 0x0004df27
		mov ebp, ebx		// 0x0004df29
		jmp L4df30		// 0x0004df2b
		lea ecx, [ecx]		// 0x0004df2d
L4df30:
		mov al, byte ptr [esi + 0x18]		// 0x0004df30
		shl edi, 1		// 0x0004df33
		test al, al		// 0x0004df35
		jne L4df47		// 0x0004df37
		mov ecx, esi		// 0x0004df39
		call nxPMCallReadByte		// 0x0004df3b
		mov byte ptr [esi + 0x19], al		// 0x0004df40
		mov byte ptr [esi + 0x18], 0x80		// 0x0004df43
L4df47:
		mov al, byte ptr [esi + 0x18]		// 0x0004df47
		test byte ptr [esi + 0x19], al		// 0x0004df4a
		setne cl		// 0x0004df4d
		shr al, 1		// 0x0004df50
		mov byte ptr [esi + 0x18], al		// 0x0004df52
		movzx eax, cl		// 0x0004df55
		or edi, eax		// 0x0004df58
		dec ebp		// 0x0004df5a
		jne L4df30		// 0x0004df5b
L4df5d:
		mov dword ptr [nxPMapCursor + 4], edi		// 0x0004df5d
		jmp L4e0a3		// 0x0004df63
L4df68:
		xor edi, edi		// 0x0004df68
		test ebx, ebx		// 0x0004df6a
		je L4df9d		// 0x0004df6c
		mov ebp, ebx		// 0x0004df6e
L4df70:
		mov al, byte ptr [esi + 0x18]		// 0x0004df70
		shl edi, 1		// 0x0004df73
		test al, al		// 0x0004df75
		jne L4df87		// 0x0004df77
		mov ecx, esi		// 0x0004df79
		call nxPMCallReadByte		// 0x0004df7b
		mov byte ptr [esi + 0x19], al		// 0x0004df80
		mov byte ptr [esi + 0x18], 0x80		// 0x0004df83
L4df87:
		mov al, byte ptr [esi + 0x18]		// 0x0004df87
		test byte ptr [esi + 0x19], al		// 0x0004df8a
		setne cl		// 0x0004df8d
		movzx ecx, cl		// 0x0004df90
		shr al, 1		// 0x0004df93
		or edi, ecx		// 0x0004df95
		dec ebp		// 0x0004df97
		mov byte ptr [esi + 0x18], al		// 0x0004df98
		jne L4df70		// 0x0004df9b
L4df9d:
		mov dword ptr [nxPMapCursor], edi		// 0x0004df9d
		xor edi, edi		// 0x0004dfa3
		test ebx, ebx		// 0x0004dfa5
		je L4e09d		// 0x0004dfa7
		mov ebp, ebx		// 0x0004dfad
		nop		// 0x0004dfaf
L4dfb0:
		mov al, byte ptr [esi + 0x18]		// 0x0004dfb0
		shl edi, 1		// 0x0004dfb3
		test al, al		// 0x0004dfb5
		jne L4dfc7		// 0x0004dfb7
		mov ecx, esi		// 0x0004dfb9
		call nxPMCallReadByte		// 0x0004dfbb
		mov byte ptr [esi + 0x19], al		// 0x0004dfc0
		mov byte ptr [esi + 0x18], 0x80		// 0x0004dfc3
L4dfc7:
		mov al, byte ptr [esi + 0x18]		// 0x0004dfc7
		test byte ptr [esi + 0x19], al		// 0x0004dfca
		setne cl		// 0x0004dfcd
		movzx edx, cl		// 0x0004dfd0
		shr al, 1		// 0x0004dfd3
		or edi, edx		// 0x0004dfd5
		dec ebp		// 0x0004dfd7
		mov byte ptr [esi + 0x18], al		// 0x0004dfd8
		jne L4dfb0		// 0x0004dfdb
		jmp L4e09d		// 0x0004dfdd
L4dfe2:
		xor edi, edi		// 0x0004dfe2
		test ebx, ebx		// 0x0004dfe4
		je L4e01d		// 0x0004dfe6
		mov ebp, ebx		// 0x0004dfe8
		lea ebx, [ebx]		// 0x0004dfea
L4dff0:
		mov al, byte ptr [esi + 0x18]		// 0x0004dff0
		shl edi, 1		// 0x0004dff3
		test al, al		// 0x0004dff5
		jne L4e007		// 0x0004dff7
		mov ecx, esi		// 0x0004dff9
		call nxPMCallReadByte		// 0x0004dffb
		mov byte ptr [esi + 0x19], al		// 0x0004e000
		mov byte ptr [esi + 0x18], 0x80		// 0x0004e003
L4e007:
		mov al, byte ptr [esi + 0x18]		// 0x0004e007
		test byte ptr [esi + 0x19], al		// 0x0004e00a
		setne cl		// 0x0004e00d
		shr al, 1		// 0x0004e010
		mov byte ptr [esi + 0x18], al		// 0x0004e012
		movzx eax, cl		// 0x0004e015
		or edi, eax		// 0x0004e018
		dec ebp		// 0x0004e01a
		jne L4dff0		// 0x0004e01b
L4e01d:
		mov dword ptr [nxPMapCursor], edi		// 0x0004e01d
		xor edi, edi		// 0x0004e023
		test ebx, ebx		// 0x0004e025
		je L4e05d		// 0x0004e027
		mov ebp, ebx		// 0x0004e029
		jmp L4e030		// 0x0004e02b
		lea ecx, [ecx]		// 0x0004e02d
L4e030:
		mov al, byte ptr [esi + 0x18]		// 0x0004e030
		shl edi, 1		// 0x0004e033
		test al, al		// 0x0004e035
		jne L4e047		// 0x0004e037
		mov ecx, esi		// 0x0004e039
		call nxPMCallReadByte		// 0x0004e03b
		mov byte ptr [esi + 0x19], al		// 0x0004e040
		mov byte ptr [esi + 0x18], 0x80		// 0x0004e043
L4e047:
		mov al, byte ptr [esi + 0x18]		// 0x0004e047
		test byte ptr [esi + 0x19], al		// 0x0004e04a
		setne cl		// 0x0004e04d
		movzx ecx, cl		// 0x0004e050
		shr al, 1		// 0x0004e053
		or edi, ecx		// 0x0004e055
		dec ebp		// 0x0004e057
		mov byte ptr [esi + 0x18], al		// 0x0004e058
		jne L4e030		// 0x0004e05b
L4e05d:
		mov dword ptr [nxPMapCursor + 4], edi		// 0x0004e05d
		xor edi, edi		// 0x0004e063
		test ebx, ebx		// 0x0004e065
		je L4e09d		// 0x0004e067
		mov ebp, ebx		// 0x0004e069
		jmp L4e070		// 0x0004e06b
		lea ecx, [ecx]		// 0x0004e06d
L4e070:
		mov al, byte ptr [esi + 0x18]		// 0x0004e070
		shl edi, 1		// 0x0004e073
		test al, al		// 0x0004e075
		jne L4e087		// 0x0004e077
		mov ecx, esi		// 0x0004e079
		call nxPMCallReadByte		// 0x0004e07b
		mov byte ptr [esi + 0x19], al		// 0x0004e080
		mov byte ptr [esi + 0x18], 0x80		// 0x0004e083
L4e087:
		mov al, byte ptr [esi + 0x18]		// 0x0004e087
		test byte ptr [esi + 0x19], al		// 0x0004e08a
		setne cl		// 0x0004e08d
		movzx edx, cl		// 0x0004e090
		shr al, 1		// 0x0004e093
		or edi, edx		// 0x0004e095
		dec ebp		// 0x0004e097
		mov byte ptr [esi + 0x18], al		// 0x0004e098
		jne L4e070		// 0x0004e09b
L4e09d:
		mov dword ptr [nxPMapCursor + 8], edi		// 0x0004e09d
L4e0a3:
		mov eax, dword ptr [esp + 0x1c]		// 0x0004e0a3
		mov edi, dword ptr [nxPMapCursor + 8]		// 0x0004e0a7
		mov ecx, dword ptr [nxPMapCursor + 4]		// 0x0004e0ad
		imul edi, eax		// 0x0004e0b3
		mov ebp, dword ptr [nxPMapCursor]		// 0x0004e0b6
		add edi, ecx		// 0x0004e0bc
		imul edi, eax		// 0x0004e0be
		mov eax, dword ptr [esp + 0x18]		// 0x0004e0c1
		mov ecx, dword ptr [eax + 4]		// 0x0004e0c5
		mov edx, dword ptr [eax]		// 0x0004e0c8
		add edi, ebp		// 0x0004e0ca
		cmp ecx, edx		// 0x0004e0cc
		jne L4e0d9		// 0x0004e0ce
		push 1		// 0x0004e0d0
		mov ecx, eax		// 0x0004e0d2
		call nxPMCallResize		// 0x0004e0d4
L4e0d9:
		mov eax, dword ptr [esp + 0x18]		// 0x0004e0d9
		mov edx, dword ptr [eax + 4]		// 0x0004e0dd
		mov ecx, dword ptr [eax + 8]		// 0x0004e0e0
		mov dword ptr [ecx + edx*4], edi		// 0x0004e0e3
		mov ebp, dword ptr [eax + 4]		// 0x0004e0e6
		mov ecx, dword ptr [esp + 0xc]		// 0x0004e0e9
		inc ebp		// 0x0004e0ed
		mov dword ptr [eax + 4], ebp		// 0x0004e0ee
		mov eax, dword ptr [esp + 0x10]		// 0x0004e0f1
		inc ecx		// 0x0004e0f5
		cmp ecx, eax		// 0x0004e0f6
		mov dword ptr [esp + 0xc], ecx		// 0x0004e0f8
		jb L4dc22		// 0x0004e0fc
		pop edi		// 0x0004e102
		pop ebp		// 0x0004e103
		pop ebx		// 0x0004e104
		add esp, 8		// 0x0004e105
		ret		// 0x0004e108
L4e109:
		pop edi		// 0x0004e109
		mov eax, ebp		// 0x0004e10a
		pop ebp		// 0x0004e10c
		pop ebx		// 0x0004e10d
		add esp, 8		// 0x0004e10e
		ret		// 0x0004e111
		}
	}

// phys_fn_002031 (0x0004ff20, 81 B), 27 instructions
__declspec(naked) unsigned __fastcall nxPMapScalarDelete(void* self, void* edxUnused, unsigned a0)
	{
	__asm
		{
		push esi		// 0x0004ff20
		mov esi, ecx		// 0x0004ff21
		mov eax, dword ptr [esi + 0x70]		// 0x0004ff23
		test eax, eax		// 0x0004ff26
		mov dword ptr [esi], offset nxPMapVtable		// 0x0004ff28
		je L4ff40		// 0x0004ff2e
		push eax		// 0x0004ff30
		call nxPMCallFree		// 0x0004ff31
		add esp, 4		// 0x0004ff36
		mov dword ptr [esi + 0x70], 0		// 0x0004ff39
L4ff40:
		mov eax, dword ptr [esi + 4]		// 0x0004ff40
		test eax, eax		// 0x0004ff43
		je L4ff57		// 0x0004ff45
		push eax		// 0x0004ff47
		call nxPMCallFree		// 0x0004ff48
		add esp, 4		// 0x0004ff4d
		mov dword ptr [esi + 4], 0		// 0x0004ff50
L4ff57:
		test byte ptr [esp + 8], 1		// 0x0004ff57
		je L4ff6b		// 0x0004ff5c
		mov eax, dword ptr [nxPMImpImpnxFoundationSDKAllocator3PAVNxUserAll]		// 0x0004ff5e
		mov ecx, dword ptr [eax]		// 0x0004ff63
		mov edx, dword ptr [ecx]		// 0x0004ff65
		push esi		// 0x0004ff67
		call dword ptr [edx + 0x14]		// 0x0004ff68
L4ff6b:
		mov eax, esi		// 0x0004ff6b
		pop esi		// 0x0004ff6d
		ret 4		// 0x0004ff6e
		}
	}

// phys_fn_005159 as a thiscall (the candidate has it only as a harness helper): the byte-guarded release of +8 and +4 through the SDK allocator (0x000e32e0, 72 B), 27 instructions
__declspec(naked) void __fastcall nxPMapRelease5159(void* self)
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
		call nxPMCallSdkAllocator		// 0x000e32f1
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
		call nxPMCallSdkAllocator		// 0x000e330f
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
