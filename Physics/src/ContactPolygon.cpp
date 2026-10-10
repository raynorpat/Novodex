#include "ContactGeneration.h"
#include "NxPhysicsBackend.h"

#if NX_PHYSICS_USE_X87
extern "C" void nxContactCallContainerResize();
#pragma comment(linker, "/alternatename:_nxContactCallContainerResize=?Resize@Container@IceCore@@AAE_NI@Z")
// ---------------------------------------------------------------------------
// convex-mesh gap Task 2g: prerequisites P-Emit (000875) and P-Plane (001903,
// 001907, 001909 with their continuations 001905 and 001911), from the Capstone
// listing (units/convex-mesh-gap-contract.md). 000875 sits after 000873 in the
// image and the P-Plane rows after 001901 (plane/sphere), both in this file's
// units, so they are written here.
//
// Every row is the listing's instructions, naked (branch targets are labels
// named by their oracle RVA), because each keeps values on the x87 stack across
// narrowing stores and mixes register and stack arguments no C++ declaration
// expresses. They call: the vendored Container::Resize (004840, a private
// member, reached through an /alternatename alias of its decorated name, as
// ConvexHull.cpp reaches the Container's constructor), the CRT's stack probe
// (005695, `_chkstk`: eax the byte count, esp moved by it -- the same contract
// as the oracle's static-CRT copy) and the Foundation export
// NxFindRotationMatrix through its import slot (`call dword ptr
// [__imp__NxFindRotationMatrix]`, the listing's `call dword ptr [0x10104174]`).

extern "C" void _chkstk();								// 005695, the stack probe
extern "C" void* _imp__NxFindRotationMatrix;			// the import slot 0x10104174

// .rdata 0x101041f0 (0.0f), 0x101041ec (1.0f), 0x10107880 / 0x10107888 (the
// doubles 1e-7 and -1e-7).
static const float kContactZero = 0.0f;
static const float kContactOne = 1.0f;
static const double kContactTenth7 = 1e-7;
static const double kContactMinusTenth7 = -1e-7;

// phys_fn_000875 (0x0001d8e0, 915 B)
// The contact emitter with feature words (thiscall on the sink, nine stack
// arguments, `ret 0x24`): the two collision objects, the separation's bits, the
// point, the normal, two 16-bit feature ids and two 32-bit feature words. As
// 000873 it reads each object's shape at +0x08 and, when shape1's owner's +0x08
// differs from the sink's +0x08, swaps the objects, the two ids and the two
// words and negates the normal into a local (`fld; fchs; fstp`: a signalling
// NaN is quieted). The pair header is written when either collision object
// differs from the sink's last pair (+0x20, +0x24): its flag word (+0x34) is 1
// when both ids are real (not 0xffff) or'd with 4 when either shape's +0xde has
// bit 0x20, shifted into bits 16..31 of the header word, whose top byte is the
// material from shape1's owner's holder (+0x240) or, with a null holder,
// shape0's; the two objects, then the header word (whose stream index goes to
// +0x18, the pair counter at +0x14 incremented), and the cached normal cleared.
// The normal block is written when the normal's words differ from the cached
// ones (+0x28..+0x30): three words and a zero count (its index to +0x1c, the
// header's count incremented). Then the contact: the counter at +0x10, the
// point's three words, the separation word with its sign bit replaced by bit
// 31 set when either 32-bit feature word exceeds 0xffff, the count at +0x1c
// incremented; with flag 1 the ids' word (id1 << 16 | id0); with flag 4 either
// the two feature words (when one exceeds 0xffff) or (wb << 16 | wa). Every
// append grows the stream (the Container at +0x38) through 004840 when full:
// by 1, or by 3 before the three-word appends.
__declspec(naked) void __fastcall NxEmitContactFeatures(NxContactSink* /*sink*/, NxU32 /*edx*/,
	void* /*object1*/, void* /*object0*/, NxU32 /*separationBits*/, const NxVec3* /*point*/,
	const NxVec3* /*normal*/, NxU32 /*featureId0*/, NxU32 /*featureId1*/, NxU32 /*featureWord0*/,
	NxU32 /*featureWord1*/)
	{
	__asm
		{
		mov	eax, dword ptr [esp + 4]		// 0x0001d8e0
		sub	esp, 0xc		// 0x0001d8e4
		push	ebx		// 0x0001d8e7
		mov	ebx, dword ptr [eax + 8]		// 0x0001d8e8
		mov	edx, dword ptr [ebx + 4]		// 0x0001d8eb
		mov	eax, dword ptr [edx + 8]		// 0x0001d8ee
		push	ebp		// 0x0001d8f1
		push	esi		// 0x0001d8f2
		mov	esi, ecx		// 0x0001d8f3
		mov	ecx, dword ptr [esp + 0x20]		// 0x0001d8f5
		mov	ebp, dword ptr [ecx + 8]		// 0x0001d8f9
		cmp	eax, dword ptr [esi + 8]		// 0x0001d8fc
		push	edi		// 0x0001d8ff
		je	L1d950		// 0x0001d900
		mov	ecx, dword ptr [esp + 0x40]		// 0x0001d902
		mov	edx, dword ptr [esp + 0x38]		// 0x0001d906
		mov	eax, ebx		// 0x0001d90a
		mov	ebx, ebp		// 0x0001d90c
		mov	ebp, eax		// 0x0001d90e
		mov	eax, dword ptr [esp + 0x3c]		// 0x0001d910
		mov	dword ptr [esp + 0x40], eax		// 0x0001d914
		mov	eax, dword ptr [esp + 0x34]		// 0x0001d918
		mov	dword ptr [esp + 0x38], eax		// 0x0001d91c
		mov	eax, dword ptr [esp + 0x30]		// 0x0001d920
		fld	dword ptr [eax]		// 0x0001d924
		mov	dword ptr [esp + 0x3c], ecx		// 0x0001d926
		fchs		// 0x0001d92a
		mov	dword ptr [esp + 0x34], edx		// 0x0001d92c
		fstp	dword ptr [esp + 0x10]		// 0x0001d930
		fld	dword ptr [eax + 4]		// 0x0001d934
		fchs		// 0x0001d937
		fstp	dword ptr [esp + 0x14]		// 0x0001d939
		fld	dword ptr [eax + 8]		// 0x0001d93d
		lea	eax, [esp + 0x10]		// 0x0001d940
		fchs		// 0x0001d944
		mov	dword ptr [esp + 0x30], eax		// 0x0001d946
		fstp	dword ptr [esp + 0x18]		// 0x0001d94a
		jmp	L1d958		// 0x0001d94e
L1d950:
		mov	eax, dword ptr [esp + 0x30]		// 0x0001d950
		mov	dword ptr [esp + 0x30], eax		// 0x0001d954
L1d958:
		mov	ecx, dword ptr [esi + 0x20]		// 0x0001d958
		cmp	ecx, dword ptr [ebx + 0x9c]		// 0x0001d95b
		jne	L1d972		// 0x0001d961
		mov	edx, dword ptr [esi + 0x24]		// 0x0001d963
		cmp	edx, dword ptr [ebp + 0x9c]		// 0x0001d966
		je	L1da86		// 0x0001d96c
L1d972:
		mov	eax, 0xffff		// 0x0001d972
		cmp	word ptr [esp + 0x34], ax		// 0x0001d977
		je	L1d98c		// 0x0001d97c
		cmp	word ptr [esp + 0x38], ax		// 0x0001d97e
		je	L1d98c		// 0x0001d983
		mov	ecx, 1		// 0x0001d985
		jmp	L1d98e		// 0x0001d98a
L1d98c:
		xor	ecx, ecx		// 0x0001d98c
L1d98e:
		mov	dl, byte ptr [ebx + 0xde]		// 0x0001d98e
		mov	al, 0x20		// 0x0001d994
		_emit	0x84
		_emit	0xd0		// 0x0001d996 test al, dl
		jne	L1d9a6		// 0x0001d998
		test	byte ptr [ebp + 0xde], al		// 0x0001d99a
		jne	L1d9a6		// 0x0001d9a0
		xor	eax, eax		// 0x0001d9a2
		jmp	L1d9ab		// 0x0001d9a4
L1d9a6:
		mov	eax, 4		// 0x0001d9a6
L1d9ab:
		or	eax, ecx		// 0x0001d9ab
		mov	dword ptr [esi + 0x34], eax		// 0x0001d9ad
		shl	eax, 0x10		// 0x0001d9b0
		mov	dword ptr [esp + 0x24], eax		// 0x0001d9b3
		mov	eax, dword ptr [ebx + 0x9c]		// 0x0001d9b7
		mov	dword ptr [esi + 0x20], eax		// 0x0001d9bd
		mov	ecx, dword ptr [ebp + 0x9c]		// 0x0001d9c0
		mov	dword ptr [esi + 0x24], ecx		// 0x0001d9c6
		mov	eax, dword ptr [esi + 0x3c]		// 0x0001d9c9
		mov	ecx, dword ptr [esi + 0x38]		// 0x0001d9cc
		cmp	eax, ecx		// 0x0001d9cf
		mov	edx, dword ptr [ebx + 0x9c]		// 0x0001d9d1
		lea	edi, [esi + 0x38]		// 0x0001d9d7
		mov	dword ptr [esp + 0x20], edx		// 0x0001d9da
		jne	L1d9e9		// 0x0001d9de
		push	1		// 0x0001d9e0
		mov	ecx, edi		// 0x0001d9e2
		call	nxContactCallContainerResize		// 0x0001d9e4
L1d9e9:
		mov	ecx, dword ptr [edi + 4]		// 0x0001d9e9
		mov	eax, dword ptr [esp + 0x20]		// 0x0001d9ec
		mov	edx, dword ptr [edi + 8]		// 0x0001d9f0
		mov	dword ptr [edx + ecx*4], eax		// 0x0001d9f3
		inc	dword ptr [edi + 4]		// 0x0001d9f6
		mov	edx, dword ptr [edi + 4]		// 0x0001d9f9
		cmp	edx, dword ptr [edi]		// 0x0001d9fc
		mov	ecx, dword ptr [ebp + 0x9c]		// 0x0001d9fe
		mov	dword ptr [esp + 0x20], ecx		// 0x0001da04
		jne	L1da13		// 0x0001da08
		push	1		// 0x0001da0a
		mov	ecx, edi		// 0x0001da0c
		call	nxContactCallContainerResize		// 0x0001da0e
L1da13:
		mov	eax, dword ptr [edi + 4]		// 0x0001da13
		mov	edx, dword ptr [esp + 0x20]		// 0x0001da16
		mov	ecx, dword ptr [edi + 8]		// 0x0001da1a
		mov	dword ptr [ecx + eax*4], edx		// 0x0001da1d
		inc	dword ptr [edi + 4]		// 0x0001da20
		mov	eax, dword ptr [ebx + 4]		// 0x0001da23
		mov	eax, dword ptr [eax + 8]		// 0x0001da26
		test	eax, eax		// 0x0001da29
		je	L1da35		// 0x0001da2b
		mov	ebx, dword ptr [eax + 0x240]		// 0x0001da2d
		jmp	L1da41		// 0x0001da33
L1da35:
		mov	ecx, dword ptr [ebp + 4]		// 0x0001da35
		mov	edx, dword ptr [ecx + 8]		// 0x0001da38
		mov	ebx, dword ptr [edx + 0x240]		// 0x0001da3b
L1da41:
		mov	eax, dword ptr [edi + 4]		// 0x0001da41
		cmp	eax, dword ptr [edi]		// 0x0001da44
		mov	ebp, dword ptr [esi + 0x3c]		// 0x0001da46
		jne	L1da54		// 0x0001da49
		push	1		// 0x0001da4b
		mov	ecx, edi		// 0x0001da4d
		call	nxContactCallContainerResize		// 0x0001da4f
L1da54:
		mov	ecx, dword ptr [edi + 4]		// 0x0001da54
		mov	edx, dword ptr [edi + 8]		// 0x0001da57
		mov	eax, dword ptr [esp + 0x24]		// 0x0001da5a
		shl	ebx, 0x18		// 0x0001da5e
		or	ebx, eax		// 0x0001da61
		mov	dword ptr [edx + ecx*4], ebx		// 0x0001da63
		inc	dword ptr [edi + 4]		// 0x0001da66
		mov	ecx, dword ptr [esi + 0x40]		// 0x0001da69
		mov	eax, dword ptr [esi + 0x14]		// 0x0001da6c
		lea	eax, [ecx + eax*4]		// 0x0001da6f
		mov	dword ptr [esi + 0x18], ebp		// 0x0001da72
		inc	dword ptr [eax]		// 0x0001da75
		xor	eax, eax		// 0x0001da77
		mov	dword ptr [esi + 0x30], eax		// 0x0001da79
		mov	dword ptr [esi + 0x2c], eax		// 0x0001da7c
		mov	dword ptr [esi + 0x28], eax		// 0x0001da7f
		mov	eax, dword ptr [esp + 0x30]		// 0x0001da82
L1da86:
		mov	edx, dword ptr [esi + 0x28]		// 0x0001da86
		cmp	edx, dword ptr [eax]		// 0x0001da89
		jne	L1da9d		// 0x0001da8b
		mov	ecx, dword ptr [esi + 0x2c]		// 0x0001da8d
		cmp	ecx, dword ptr [eax + 4]		// 0x0001da90
		jne	L1da9d		// 0x0001da93
		mov	edx, dword ptr [esi + 0x30]		// 0x0001da95
		cmp	edx, dword ptr [eax + 8]		// 0x0001da98
		je	L1db1a		// 0x0001da9b
L1da9d:
		mov	ecx, dword ptr [eax]		// 0x0001da9d
		mov	dword ptr [esi + 0x28], ecx		// 0x0001da9f
		mov	edx, dword ptr [eax + 4]		// 0x0001daa2
		mov	dword ptr [esi + 0x2c], edx		// 0x0001daa5
		mov	ecx, dword ptr [eax + 8]		// 0x0001daa8
		lea	edi, [esi + 0x38]		// 0x0001daab
		mov	dword ptr [esi + 0x30], ecx		// 0x0001daae
		mov	edx, dword ptr [edi + 4]		// 0x0001dab1
		mov	ecx, dword ptr [edi]		// 0x0001dab4
		add	edx, 3		// 0x0001dab6
		cmp	edx, ecx		// 0x0001dab9
		jbe	L1daca		// 0x0001dabb
		push	3		// 0x0001dabd
		mov	ecx, edi		// 0x0001dabf
		call	nxContactCallContainerResize		// 0x0001dac1
		mov	eax, dword ptr [esp + 0x30]		// 0x0001dac6
L1daca:
		mov	ecx, dword ptr [edi + 4]		// 0x0001daca
		mov	edx, dword ptr [edi + 8]		// 0x0001dacd
		lea	ecx, [edx + ecx*4]		// 0x0001dad0
		mov	edx, dword ptr [eax]		// 0x0001dad3
		mov	dword ptr [ecx], edx		// 0x0001dad5
		mov	edx, dword ptr [eax + 4]		// 0x0001dad7
		mov	dword ptr [ecx + 4], edx		// 0x0001dada
		mov	eax, dword ptr [eax + 8]		// 0x0001dadd
		mov	dword ptr [ecx + 8], eax		// 0x0001dae0
		mov	ecx, dword ptr [edi + 4]		// 0x0001dae3
		add	ecx, 3		// 0x0001dae6
		mov	dword ptr [edi + 4], ecx		// 0x0001dae9
		cmp	ecx, dword ptr [edi]		// 0x0001daec
		mov	ebx, dword ptr [esi + 0x3c]		// 0x0001daee
		jne	L1dafc		// 0x0001daf1
		push	1		// 0x0001daf3
		mov	ecx, edi		// 0x0001daf5
		call	nxContactCallContainerResize		// 0x0001daf7
L1dafc:
		mov	edx, dword ptr [edi + 4]		// 0x0001dafc
		mov	eax, dword ptr [edi + 8]		// 0x0001daff
		mov	dword ptr [eax + edx*4], 0		// 0x0001db02
		inc	dword ptr [edi + 4]		// 0x0001db09
		mov	ecx, dword ptr [esi + 0x18]		// 0x0001db0c
		mov	edx, dword ptr [esi + 0x40]		// 0x0001db0f
		lea	eax, [edx + ecx*4]		// 0x0001db12
		mov	dword ptr [esi + 0x1c], ebx		// 0x0001db15
		inc	dword ptr [eax]		// 0x0001db18
L1db1a:
		mov	eax, dword ptr [esp + 0x28]		// 0x0001db1a
		mov	ecx, dword ptr [esp + 0x3c]		// 0x0001db1e
		mov	dword ptr [esp + 0x30], eax		// 0x0001db22
		mov	eax, 0xffff		// 0x0001db26
		cmp	ecx, eax		// 0x0001db2b
		ja	L1db39		// 0x0001db2d
		cmp	dword ptr [esp + 0x40], eax		// 0x0001db2f
		ja	L1db39		// 0x0001db33
		xor	ebp, ebp		// 0x0001db35
		jmp	L1db3e		// 0x0001db37
L1db39:
		mov	ebp, 0x80000000		// 0x0001db39
L1db3e:
		inc	dword ptr [esi + 0x10]		// 0x0001db3e
		mov	ecx, dword ptr [esi + 0x3c]		// 0x0001db41
		mov	eax, dword ptr [esi + 0x38]		// 0x0001db44
		lea	edi, [esi + 0x38]		// 0x0001db47
		add	ecx, 3		// 0x0001db4a
		cmp	ecx, eax		// 0x0001db4d
		jbe	L1db5a		// 0x0001db4f
		push	3		// 0x0001db51
		mov	ecx, edi		// 0x0001db53
		call	nxContactCallContainerResize		// 0x0001db55
L1db5a:
		mov	eax, dword ptr [edi + 4]		// 0x0001db5a
		mov	ecx, dword ptr [edi + 8]		// 0x0001db5d
		mov	edx, dword ptr [esp + 0x2c]		// 0x0001db60
		mov	ebx, dword ptr [esp + 0x30]		// 0x0001db64
		lea	eax, [ecx + eax*4]		// 0x0001db68
		mov	ecx, dword ptr [edx]		// 0x0001db6b
		mov	dword ptr [eax], ecx		// 0x0001db6d
		mov	ecx, dword ptr [edx + 4]		// 0x0001db6f
		mov	dword ptr [eax + 4], ecx		// 0x0001db72
		mov	edx, dword ptr [edx + 8]		// 0x0001db75
		mov	dword ptr [eax + 8], edx		// 0x0001db78
		mov	ecx, dword ptr [edi + 4]		// 0x0001db7b
		add	ecx, 3		// 0x0001db7e
		and	ebx, 0x7fffffff		// 0x0001db81
		mov	eax, ecx		// 0x0001db87
		mov	dword ptr [edi + 4], ecx		// 0x0001db89
		mov	ecx, dword ptr [edi]		// 0x0001db8c
		or	ebx, ebp		// 0x0001db8e
		cmp	eax, ecx		// 0x0001db90
		jne	L1db9d		// 0x0001db92
		push	1		// 0x0001db94
		mov	ecx, edi		// 0x0001db96
		call	nxContactCallContainerResize		// 0x0001db98
L1db9d:
		mov	ecx, dword ptr [edi + 4]		// 0x0001db9d
		mov	edx, dword ptr [edi + 8]		// 0x0001dba0
		mov	dword ptr [edx + ecx*4], ebx		// 0x0001dba3
		inc	dword ptr [edi + 4]		// 0x0001dba6
		mov	ecx, dword ptr [esi + 0x40]		// 0x0001dba9
		mov	eax, dword ptr [esi + 0x1c]		// 0x0001dbac
		lea	eax, [ecx + eax*4]		// 0x0001dbaf
		inc	dword ptr [eax]		// 0x0001dbb2
		test	byte ptr [esi + 0x34], 1		// 0x0001dbb4
		je	L1dbe5		// 0x0001dbb8
		mov	edx, dword ptr [edi + 4]		// 0x0001dbba
		cmp	edx, dword ptr [edi]		// 0x0001dbbd
		jne	L1dbca		// 0x0001dbbf
		push	1		// 0x0001dbc1
		mov	ecx, edi		// 0x0001dbc3
		call	nxContactCallContainerResize		// 0x0001dbc5
L1dbca:
		movzx	eax, word ptr [esp + 0x38]		// 0x0001dbca
		movzx	ecx, word ptr [esp + 0x34]		// 0x0001dbcf
		mov	edx, dword ptr [edi + 4]		// 0x0001dbd4
		shl	eax, 0x10		// 0x0001dbd7
		or	eax, ecx		// 0x0001dbda
		mov	ecx, dword ptr [edi + 8]		// 0x0001dbdc
		mov	dword ptr [ecx + edx*4], eax		// 0x0001dbdf
		inc	dword ptr [edi + 4]		// 0x0001dbe2
L1dbe5:
		test	byte ptr [esi + 0x34], 4		// 0x0001dbe5
		je	L1dc69		// 0x0001dbe9
		test	ebp, ebp		// 0x0001dbeb
		mov	eax, dword ptr [edi]		// 0x0001dbed
		je	L1dc40		// 0x0001dbef
		mov	edx, dword ptr [edi + 4]		// 0x0001dbf1
		cmp	edx, eax		// 0x0001dbf4
		jne	L1dc01		// 0x0001dbf6
		push	1		// 0x0001dbf8
		mov	ecx, edi		// 0x0001dbfa
		call	nxContactCallContainerResize		// 0x0001dbfc
L1dc01:
		mov	eax, dword ptr [edi + 4]		// 0x0001dc01
		mov	ecx, dword ptr [edi + 8]		// 0x0001dc04
		mov	edx, dword ptr [esp + 0x3c]		// 0x0001dc07
		mov	dword ptr [ecx + eax*4], edx		// 0x0001dc0b
		mov	edx, dword ptr [edi + 4]		// 0x0001dc0e
		inc	edx		// 0x0001dc11
		mov	dword ptr [edi + 4], edx		// 0x0001dc12
		mov	ecx, dword ptr [edi]		// 0x0001dc15
		mov	eax, edx		// 0x0001dc17
		cmp	eax, ecx		// 0x0001dc19
		jne	L1dc26		// 0x0001dc1b
		push	1		// 0x0001dc1d
		mov	ecx, edi		// 0x0001dc1f
		call	nxContactCallContainerResize		// 0x0001dc21
L1dc26:
		mov	ecx, dword ptr [edi + 4]		// 0x0001dc26
		mov	edx, dword ptr [edi + 8]		// 0x0001dc29
		mov	eax, dword ptr [esp + 0x40]		// 0x0001dc2c
		mov	dword ptr [edx + ecx*4], eax		// 0x0001dc30
		inc	dword ptr [edi + 4]		// 0x0001dc33
		pop	edi		// 0x0001dc36
		pop	esi		// 0x0001dc37
		pop	ebp		// 0x0001dc38
		pop	ebx		// 0x0001dc39
		add	esp, 0xc		// 0x0001dc3a
		ret	0x24		// 0x0001dc3d
L1dc40:
		mov	ecx, dword ptr [edi + 4]		// 0x0001dc40
		cmp	ecx, eax		// 0x0001dc43
		jne	L1dc50		// 0x0001dc45
		push	1		// 0x0001dc47
		mov	ecx, edi		// 0x0001dc49
		call	nxContactCallContainerResize		// 0x0001dc4b
L1dc50:
		mov	eax, dword ptr [esp + 0x40]		// 0x0001dc50
		mov	ecx, dword ptr [esp + 0x3c]		// 0x0001dc54
		mov	edx, dword ptr [edi + 4]		// 0x0001dc58
		shl	eax, 0x10		// 0x0001dc5b
		or	eax, ecx		// 0x0001dc5e
		mov	ecx, dword ptr [edi + 8]		// 0x0001dc60
		mov	dword ptr [ecx + edx*4], eax		// 0x0001dc63
		inc	dword ptr [edi + 4]		// 0x0001dc66
L1dc69:
		pop	edi		// 0x0001dc69
		pop	esi		// 0x0001dc6a
		pop	ebp		// 0x0001dc6b
		pop	ebx		// 0x0001dc6c
		add	esp, 0xc		// 0x0001dc6d
		ret	0x24		// 0x0001dc70
		}
	}

// phys_fn_001903 (0x00048b30, 45 B)
// phys_fn_001905 (0x00048b60, 111 B)
// Whether a point lies inside a convex polygon in its plane's frame (the row
// and its continuation, the loop, which the contract's list lacked). Register
// arguments: eax the vertex count, ecx the vertices (12-byte stride, x and y
// read); the point's x and y on the stack, cleaned by the caller. Each edge
// from the previous vertex (the last, first) whose ends lie on opposite sides
// of the point's y (`fcomp; test ah, 1`: a NaN y counts as below) is tested for
// the side the point is on ((prev.y - cur.y)(cur.x - x) against (prev.x -
// cur.x)(cur.y - y), `test ah, 0x41; jp`); a crossing on the side that matches
// the edge's direction is counted, and a second one returns 0. Returns the
// count's low bit.
__declspec(naked) NxU32 nxPolygonContainsPoint(float /*x*/, float /*y*/)
	{
	__asm
		{
		push	ebx		// 0x00048b30
		push	ebp		// 0x00048b31
		push	esi		// 0x00048b32
		push	edi		// 0x00048b33
		mov	edi, eax		// 0x00048b34
		lea	eax, [edi + edi*2]		// 0x00048b36
		fld	dword ptr [ecx + eax*4 - 8]		// 0x00048b39
		lea	esi, [ecx + eax*4 - 0xc]		// 0x00048b3d
		fcomp	dword ptr [esp + 0x18]		// 0x00048b41
		fnstsw	ax		// 0x00048b45
		test	ah, 1		// 0x00048b47
		jne	L48b53		// 0x00048b4a
		mov	ebx, 1		// 0x00048b4c
		jmp	L48b55		// 0x00048b51
L48b53:
		xor	ebx, ebx		// 0x00048b53
L48b55:
		xor	ebp, ebp		// 0x00048b55
		test	edi, edi		// 0x00048b57
		je	L48bbe		// 0x00048b59
		jmp	L48b60		// 0x00048b5b
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x00048b5d lea ecx, [ecx]
L48b60:
		fld	dword ptr [ecx + 4]		// 0x00048b60
		dec	edi		// 0x00048b63
		fcomp	dword ptr [esp + 0x18]		// 0x00048b64
		fnstsw	ax		// 0x00048b68
		test	ah, 1		// 0x00048b6a
		jne	L48b76		// 0x00048b6d
		mov	edx, 1		// 0x00048b6f
		jmp	L48b78		// 0x00048b74
L48b76:
		xor	edx, edx		// 0x00048b76
L48b78:
		cmp	ebx, edx		// 0x00048b78
		je	L48bb3		// 0x00048b7a
		fld	dword ptr [esi]		// 0x00048b7c
		fsub	dword ptr [ecx]		// 0x00048b7e
		fld	dword ptr [ecx + 4]		// 0x00048b80
		fsub	dword ptr [esp + 0x18]		// 0x00048b83
		fmulp	st(1), st		// 0x00048b87
		fld	dword ptr [esi + 4]		// 0x00048b89
		fsub	dword ptr [ecx + 4]		// 0x00048b8c
		fld	dword ptr [ecx]		// 0x00048b8f
		fsub	dword ptr [esp + 0x14]		// 0x00048b91
		fmulp	st(1), st		// 0x00048b95
		fcompp		// 0x00048b97
		fnstsw	ax		// 0x00048b99
		test	ah, 0x41		// 0x00048b9b
		jp	L48ba7		// 0x00048b9e
		mov	eax, 1		// 0x00048ba0
		jmp	L48ba9		// 0x00048ba5
L48ba7:
		xor	eax, eax		// 0x00048ba7
L48ba9:
		cmp	eax, edx		// 0x00048ba9
		jne	L48bb3		// 0x00048bab
		cmp	ebp, 1		// 0x00048bad
		je	L48bc8		// 0x00048bb0
		inc	ebp		// 0x00048bb2
L48bb3:
		mov	esi, ecx		// 0x00048bb3
		add	ecx, 0xc		// 0x00048bb5
		test	edi, edi		// 0x00048bb8
		mov	ebx, edx		// 0x00048bba
		jne	L48b60		// 0x00048bbc
L48bbe:
		pop	edi		// 0x00048bbe
		pop	esi		// 0x00048bbf
		mov	eax, ebp		// 0x00048bc0
		pop	ebp		// 0x00048bc2
		and	eax, 1		// 0x00048bc3
		pop	ebx		// 0x00048bc6
		ret		// 0x00048bc7
L48bc8:
		pop	edi		// 0x00048bc8
		pop	esi		// 0x00048bc9
		pop	ebp		// 0x00048bca
		xor	eax, eax		// 0x00048bcb
		pop	ebx		// 0x00048bcd
		ret		// 0x00048bce
		}
	}

// phys_fn_001907 (0x00048bd0, 607 B)
// An edge of one polygon clipped against the plane through an edge of the
// other (register arguments edx, ecx, esi and ebx, five stack arguments
// cleaned by the caller; 0 or 1 in eax). The two ends' plane values are
// multiplied and tested against 0.0f (`test ah, 0x41`: touching, crossing and
// NaN go on, a product above 0.0f returns 0); the edge's direction is
// normalised when its squared length is not 0.0f (`test ah, 0x44; jnp`); the
// two larger axes of the plane normal (compared as words with their sign bits
// cleared) choose the 2D solve for the crossing parameter, which is stored
// through the last argument and must be above 0.0f (`test ah, 5; jnp`); the
// point is written through esi and the result is 1 when its plane value is not
// above 0.0f.
__declspec(naked) NxU32 nxClipEdgeToPolygonPlane()
	{
	__asm
		{
		sub	esp, 0x18		// 0x00048bd0
		fld	dword ptr [edx]		// 0x00048bd3
		push	ebp		// 0x00048bd5
		fmul	dword ptr [ecx]		// 0x00048bd6
		push	edi		// 0x00048bd8
		mov	edi, dword ptr [esp + 0x30]		// 0x00048bd9
		mov	ebp, dword ptr [esp + 0x2c]		// 0x00048bdd
		fstp	dword ptr [esp + 0x30]		// 0x00048be1
		fld	dword ptr [ecx + 4]		// 0x00048be5
		fmul	dword ptr [edi + 4]		// 0x00048be8
		fld	dword ptr [edi + 8]		// 0x00048beb
		fmul	dword ptr [ecx + 8]		// 0x00048bee
		faddp	st(1), st		// 0x00048bf1
		fld	dword ptr [edi]		// 0x00048bf3
		fmul	dword ptr [ecx]		// 0x00048bf5
		faddp	st(1), st		// 0x00048bf7
		fadd	dword ptr [ecx + 0xc]		// 0x00048bf9
		fld	dword ptr [ecx + 4]		// 0x00048bfc
		fmul	dword ptr [edx + 4]		// 0x00048bff
		fld	dword ptr [edx + 8]		// 0x00048c02
		fmul	dword ptr [ecx + 8]		// 0x00048c05
		faddp	st(1), st		// 0x00048c08
		fadd	dword ptr [esp + 0x30]		// 0x00048c0a
		fadd	dword ptr [ecx + 0xc]		// 0x00048c0e
		fmulp	st(1), st		// 0x00048c11
		fcomp	dword ptr kContactZero		// 0x00048c13
		fnstsw	ax		// 0x00048c19
		test	ah, 0x41		// 0x00048c1b
		je	L48cb0		// 0x00048c1e
		fld	dword ptr [edi]		// 0x00048c24
		fsub	dword ptr [edx]		// 0x00048c26
		fld	dword ptr [edi + 4]		// 0x00048c28
		fsub	dword ptr [edx + 4]		// 0x00048c2b
		fstp	dword ptr [esp + 0xc]		// 0x00048c2e
		fld	dword ptr [edi + 8]		// 0x00048c32
		fsub	dword ptr [edx + 8]		// 0x00048c35
		fst	dword ptr [esp + 0x10]		// 0x00048c38
		fmul	dword ptr [esp + 0x10]		// 0x00048c3c
		fld	st(1)		// 0x00048c40
		fmul	st, st(2)		// 0x00048c42
		faddp	st(1), st		// 0x00048c44
		fld	dword ptr [esp + 0xc]		// 0x00048c46
		fmul	dword ptr [esp + 0xc]		// 0x00048c4a
		faddp	st(1), st		// 0x00048c4e
		fld	dword ptr kContactZero		// 0x00048c50
		fld	st(1)		// 0x00048c56
		fucompp		// 0x00048c58
		fnstsw	ax		// 0x00048c5a
		test	ah, 0x44		// 0x00048c5c
		jnp	L48c83		// 0x00048c5f
		fsqrt		// 0x00048c61
		fdivr	dword ptr kContactOne		// 0x00048c63
		fxch	st(1)		// 0x00048c69
		fmul	st, st(1)		// 0x00048c6b
		fxch	st(1)		// 0x00048c6d
		fld	dword ptr [esp + 0xc]		// 0x00048c6f
		fmul	st, st(1)		// 0x00048c73
		fstp	dword ptr [esp + 0xc]		// 0x00048c75
		fld	dword ptr [esp + 0x10]		// 0x00048c79
		fmul	st, st(1)		// 0x00048c7d
		fstp	dword ptr [esp + 0x10]		// 0x00048c7f
L48c83:
		fstp	st(0)		// 0x00048c83
		fld	dword ptr [esp + 0xc]		// 0x00048c85
		fmul	dword ptr [ecx + 4]		// 0x00048c89
		fld	dword ptr [esp + 0x10]		// 0x00048c8c
		fmul	dword ptr [ecx + 8]		// 0x00048c90
		faddp	st(1), st		// 0x00048c93
		fld	st(1)		// 0x00048c95
		fmul	dword ptr [ecx]		// 0x00048c97
		faddp	st(1), st		// 0x00048c99
		fld	dword ptr kContactZero		// 0x00048c9b
		fld	st(1)		// 0x00048ca1
		fucompp		// 0x00048ca3
		fnstsw	ax		// 0x00048ca5
		test	ah, 0x44		// 0x00048ca7
		jp	L48cb8		// 0x00048caa
		fstp	st(0)		// 0x00048cac
L48cae:
		fstp	st(0)		// 0x00048cae
L48cb0:
		pop	edi		// 0x00048cb0
		xor	eax, eax		// 0x00048cb1
		pop	ebp		// 0x00048cb3
		add	esp, 0x18		// 0x00048cb4
		ret		// 0x00048cb7
L48cb8:
		fld	dword ptr [ecx + 4]		// 0x00048cb8
		mov	eax, esi		// 0x00048cbb
		fmul	dword ptr [edx + 4]		// 0x00048cbd
		fld	dword ptr [edx + 8]		// 0x00048cc0
		fmul	dword ptr [ecx + 8]		// 0x00048cc3
		faddp	st(1), st		// 0x00048cc6
		fadd	dword ptr [esp + 0x30]		// 0x00048cc8
		fadd	dword ptr [ecx + 0xc]		// 0x00048ccc
		fdiv	st, st(1)		// 0x00048ccf
		fstp	dword ptr [esp + 0x30]		// 0x00048cd1
		fstp	st(0)		// 0x00048cd5
		fmul	dword ptr [esp + 0x30]		// 0x00048cd7
		fld	dword ptr [esp + 0x30]		// 0x00048cdb
		fmul	dword ptr [esp + 0xc]		// 0x00048cdf
		fld	dword ptr [esp + 0x10]		// 0x00048ce3
		fmul	dword ptr [esp + 0x30]		// 0x00048ce7
		fstp	dword ptr [esp + 0x1c]		// 0x00048ceb
		fld	dword ptr [edx]		// 0x00048cef
		fsub	st, st(2)		// 0x00048cf1
		fstp	dword ptr [esp + 8]		// 0x00048cf3
		fld	dword ptr [edx + 4]		// 0x00048cf7
		fsub	st, st(1)		// 0x00048cfa
		fstp	dword ptr [esp + 0xc]		// 0x00048cfc
		fstp	st(0)		// 0x00048d00
		fstp	st(0)		// 0x00048d02
		fld	dword ptr [edx + 8]		// 0x00048d04
		mov	edx, dword ptr [esp + 8]		// 0x00048d07
		fsub	dword ptr [esp + 0x1c]		// 0x00048d0b
		mov	dword ptr [eax], edx		// 0x00048d0f
		mov	edx, dword ptr [esp + 0xc]		// 0x00048d11
		mov	dword ptr [eax + 4], edx		// 0x00048d15
		fstp	dword ptr [esp + 0x10]		// 0x00048d18
		mov	edx, dword ptr [esp + 0x10]		// 0x00048d1c
		mov	dword ptr [eax + 8], edx		// 0x00048d20
		mov	edx, dword ptr [ecx + 4]		// 0x00048d23
		mov	edi, dword ptr [ecx]		// 0x00048d26
		and	edx, 0x7fffffff		// 0x00048d28
		and	edi, 0x7fffffff		// 0x00048d2e
		xor	eax, eax		// 0x00048d34
		cmp	edx, edi		// 0x00048d36
		jbe	L48d3f		// 0x00048d38
		mov	eax, 1		// 0x00048d3a
L48d3f:
		mov	edx, dword ptr [ecx + eax*4]		// 0x00048d3f
		mov	ecx, dword ptr [ecx + 8]		// 0x00048d42
		and	edx, 0x7fffffff		// 0x00048d45
		and	ecx, 0x7fffffff		// 0x00048d4b
		cmp	ecx, edx		// 0x00048d51
		ja	L48d6a		// 0x00048d53
		test	eax, eax		// 0x00048d55
		mov	ecx, 2		// 0x00048d57
		jne	L48d65		// 0x00048d5c
		mov	eax, 1		// 0x00048d5e
		jmp	L48d71		// 0x00048d63
L48d65:
		cmp	eax, 1		// 0x00048d65
		je	L48d6f		// 0x00048d68
L48d6a:
		mov	ecx, 1		// 0x00048d6a
L48d6f:
		xor	eax, eax		// 0x00048d6f
L48d71:
		fld	dword ptr [esi + ecx*4]		// 0x00048d71
		mov	edx, dword ptr [esp + 0x24]		// 0x00048d74
		fsub	dword ptr [edx + ecx*4]		// 0x00048d78
		fmul	dword ptr [ebx + eax*4]		// 0x00048d7b
		fld	dword ptr [esi + eax*4]		// 0x00048d7e
		fsub	dword ptr [edx + eax*4]		// 0x00048d81
		fmul	dword ptr [ebx + ecx*4]		// 0x00048d84
		fsubp	st(1), st		// 0x00048d87
		fld	dword ptr [ebp + ecx*4]		// 0x00048d89
		fmul	dword ptr [ebx + eax*4]		// 0x00048d8d
		fld	dword ptr [ebp + eax*4]		// 0x00048d90
		fmul	dword ptr [ebx + ecx*4]		// 0x00048d94
		mov	eax, dword ptr [esp + 0x34]		// 0x00048d97
		fsubp	st(1), st		// 0x00048d9b
		fdivp	st(1), st		// 0x00048d9d
		fld	st(0)		// 0x00048d9f
		fstp	dword ptr [eax]		// 0x00048da1
		fcom	dword ptr kContactZero		// 0x00048da3
		fnstsw	ax		// 0x00048da9
		test	ah, 5		// 0x00048dab
		jnp	L48cae		// 0x00048dae
		fld	st(0)		// 0x00048db4
		mov	eax, dword ptr [esp + 0x28]		// 0x00048db6
		fmul	dword ptr [ebp]		// 0x00048dba
		fld	st(1)		// 0x00048dbd
		fmul	dword ptr [ebp + 4]		// 0x00048dbf
		fstp	dword ptr [esp + 0xc]		// 0x00048dc2
		fxch	st(1)		// 0x00048dc6
		fmul	dword ptr [ebp + 8]		// 0x00048dc8
		fstp	dword ptr [esp + 0x10]		// 0x00048dcb
		fsubr	dword ptr [esi]		// 0x00048dcf
		fst	dword ptr [esi]		// 0x00048dd1
		fld	dword ptr [esi + 4]		// 0x00048dd3
		fsub	dword ptr [esp + 0xc]		// 0x00048dd6
		fst	dword ptr [esi + 4]		// 0x00048dda
		fld	dword ptr [esi + 8]		// 0x00048ddd
		fsub	dword ptr [esp + 0x10]		// 0x00048de0
		fst	dword ptr [esi + 8]		// 0x00048de4
		fld	dword ptr [edx + 8]		// 0x00048de7
		fsub	st, st(1)		// 0x00048dea
		fld	dword ptr [eax + 8]		// 0x00048dec
		fsub	st, st(2)		// 0x00048def
		fmulp	st(1), st		// 0x00048df1
		fld	dword ptr [edx + 4]		// 0x00048df3
		fsub	st, st(3)		// 0x00048df6
		fld	dword ptr [eax + 4]		// 0x00048df8
		fsub	st, st(4)		// 0x00048dfb
		fmulp	st(1), st		// 0x00048dfd
		faddp	st(1), st		// 0x00048dff
		fld	dword ptr [edx]		// 0x00048e01
		fsub	st, st(4)		// 0x00048e03
		fld	dword ptr [eax]		// 0x00048e05
		fsub	st, st(5)		// 0x00048e07
		fmulp	st(1), st		// 0x00048e09
		faddp	st(1), st		// 0x00048e0b
		fcomp	dword ptr kContactZero		// 0x00048e0d
		fstp	st(0)		// 0x00048e13
		fnstsw	ax		// 0x00048e15
		fstp	st(0)		// 0x00048e17
		test	ah, 5		// 0x00048e19
		fstp	st(0)		// 0x00048e1c
		jp	L48cb0		// 0x00048e1e
		pop	edi		// 0x00048e24
		mov	eax, 1		// 0x00048e25
		pop	ebp		// 0x00048e2a
		add	esp, 0x18		// 0x00048e2b
		ret		// 0x00048e2e
		}
	}

// phys_fn_001909 (0x00048e30, 733 B)
// phys_fn_001911 (0x00049110, 2,952 B)
// Contacts between two convex polygons (cdecl, 22 arguments; the row and its
// continuation, which the contract's list lacked): the two polygons' counts,
// vertices, references, poses and planes, the contact normal and the frames,
// the two shapes and the sink, and the ids and feature words 000875 is given
// (0, 0, 0xffff, 0xffff, 0, 0 from 001818). The polygon with more vertices
// sizes the `_chkstk` blocks (12 bytes a vertex); the normal is negated into a
// local and NxFindRotationMatrix takes the given axis to (0, 0, 1); each
// polygon's vertices are projected into that frame (four at a time, then the
// rest). A vertex of one polygon is emitted through 000875 (at its world
// position, with its depth) when its plane value against the other polygon
// passes the tests on the doubles -1e-7 and 1e-7 (0x10107888 / 0x10107880) or
// its crossing parameter is not below 0.0f, and 001903 finds it inside the other
// polygon; then every edge pair is clipped by 001907 and emitted. The vertex
// references are not checked against the vertex arrays.
__declspec(naked) void NxConvexPolygonContacts()
	{
	__asm
		{
		push	ebp		// 0x00048e30
		lea	ebp, [esp - 0x20]		// 0x00048e31
		sub	esp, 0xcc		// 0x00048e35
		mov	eax, dword ptr [ebp + 0x50]		// 0x00048e3b
		fld	dword ptr [eax]		// 0x00048e3e
		push	ebx		// 0x00048e40
		fchs		// 0x00048e41
		push	esi		// 0x00048e43
		fstp	dword ptr [ebp - 0x3c]		// 0x00048e44
		push	edi		// 0x00048e47
		fld	dword ptr [eax + 4]		// 0x00048e48
		mov	edi, dword ptr [ebp + 0x3c]		// 0x00048e4b
		fchs		// 0x00048e4e
		fstp	dword ptr [ebp - 0x38]		// 0x00048e50
		fld	dword ptr [eax + 8]		// 0x00048e53
		mov	eax, dword ptr [ebp + 0x28]		// 0x00048e56
		cmp	eax, edi		// 0x00048e59
		fchs		// 0x00048e5b
		fstp	dword ptr [ebp - 0x34]		// 0x00048e5d
		ja	L48e64		// 0x00048e60
		mov	eax, edi		// 0x00048e62
L48e64:
		lea	eax, [eax + eax*2]		// 0x00048e64
		shl	eax, 2		// 0x00048e67
		add	eax, 3		// 0x00048e6a
		and	eax, 0xfffffffc		// 0x00048e6d
		call	_chkstk		// 0x00048e70
		mov	edx, dword ptr [ebp + 0x4c]		// 0x00048e75
		mov	esi, esp		// 0x00048e78
		lea	eax, [ebp - 0x10]		// 0x00048e7a
		push	eax		// 0x00048e7d
		lea	ecx, [ebp - 0x48]		// 0x00048e7e
		push	ecx		// 0x00048e81
		push	edx		// 0x00048e82
		mov	dword ptr [ebp - 0x14], esi		// 0x00048e83
		mov	dword ptr [ebp - 0x48], 0		// 0x00048e86
		mov	dword ptr [ebp - 0x44], 0		// 0x00048e8d
		mov	dword ptr [ebp - 0x40], 0x3f800000		// 0x00048e94
		call	dword ptr _imp__NxFindRotationMatrix		// 0x00048e9b
		add	esp, 0xc		// 0x00048ea1
		xor	ebx, ebx		// 0x00048ea4
		cmp	edi, 4		// 0x00048ea6
		jl	L48fc2		// 0x00048ea9
		mov	edx, dword ptr [ebp + 0x44]		// 0x00048eaf
		add	edi, -4		// 0x00048eb2
		shr	edi, 2		// 0x00048eb5
		add	edx, 8		// 0x00048eb8
		inc	edi		// 0x00048ebb
		lea	eax, [esi + 0x1c]		// 0x00048ebc
		lea	ebx, [edi*4]		// 0x00048ebf
L48ec6:
		mov	ecx, dword ptr [edx - 8]		// 0x00048ec6
		fld	dword ptr [ebp - 0xc]		// 0x00048ec9
		mov	esi, dword ptr [ebp + 0x40]		// 0x00048ecc
		lea	ecx, [ecx + ecx*2]		// 0x00048ecf
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00048ed2
		lea	ecx, [esi + ecx*4]		// 0x00048ed6
		fld	dword ptr [ebp - 8]		// 0x00048ed9
		fmul	dword ptr [ecx + 8]		// 0x00048edc
		faddp	st(1), st		// 0x00048edf
		fld	dword ptr [ebp - 0x10]		// 0x00048ee1
		fmul	dword ptr [ecx]		// 0x00048ee4
		faddp	st(1), st		// 0x00048ee6
		fstp	dword ptr [eax - 0x1c]		// 0x00048ee8
		fld	dword ptr [ebp - 4]		// 0x00048eeb
		fmul	dword ptr [ecx]		// 0x00048eee
		fld	dword ptr [ebp]		// 0x00048ef0
		fmul	dword ptr [ecx + 4]		// 0x00048ef3
		faddp	st(1), st		// 0x00048ef6
		fld	dword ptr [ebp + 4]		// 0x00048ef8
		fmul	dword ptr [ecx + 8]		// 0x00048efb
		faddp	st(1), st		// 0x00048efe
		fstp	dword ptr [eax - 0x18]		// 0x00048f00
		mov	ecx, dword ptr [edx - 4]		// 0x00048f03
		fld	dword ptr [ebp - 0xc]		// 0x00048f06
		lea	ecx, [ecx + ecx*2]		// 0x00048f09
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00048f0c
		lea	ecx, [esi + ecx*4]		// 0x00048f10
		fld	dword ptr [ebp - 8]		// 0x00048f13
		fmul	dword ptr [ecx + 8]		// 0x00048f16
		faddp	st(1), st		// 0x00048f19
		fld	dword ptr [ebp - 0x10]		// 0x00048f1b
		fmul	dword ptr [ecx]		// 0x00048f1e
		faddp	st(1), st		// 0x00048f20
		fstp	dword ptr [eax - 0x10]		// 0x00048f22
		fld	dword ptr [ebp - 4]		// 0x00048f25
		fmul	dword ptr [ecx]		// 0x00048f28
		fld	dword ptr [ebp]		// 0x00048f2a
		fmul	dword ptr [ecx + 4]		// 0x00048f2d
		faddp	st(1), st		// 0x00048f30
		fld	dword ptr [ebp + 4]		// 0x00048f32
		fmul	dword ptr [ecx + 8]		// 0x00048f35
		faddp	st(1), st		// 0x00048f38
		fstp	dword ptr [eax - 0xc]		// 0x00048f3a
		mov	ecx, dword ptr [edx]		// 0x00048f3d
		fld	dword ptr [ebp - 0xc]		// 0x00048f3f
		lea	ecx, [ecx + ecx*2]		// 0x00048f42
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00048f45
		lea	ecx, [esi + ecx*4]		// 0x00048f49
		fld	dword ptr [ebp - 8]		// 0x00048f4c
		fmul	dword ptr [ecx + 8]		// 0x00048f4f
		faddp	st(1), st		// 0x00048f52
		fld	dword ptr [ebp - 0x10]		// 0x00048f54
		fmul	dword ptr [ecx]		// 0x00048f57
		faddp	st(1), st		// 0x00048f59
		fstp	dword ptr [eax - 4]		// 0x00048f5b
		fld	dword ptr [ebp - 4]		// 0x00048f5e
		fmul	dword ptr [ecx]		// 0x00048f61
		fld	dword ptr [ebp]		// 0x00048f63
		fmul	dword ptr [ecx + 4]		// 0x00048f66
		faddp	st(1), st		// 0x00048f69
		fld	dword ptr [ebp + 4]		// 0x00048f6b
		fmul	dword ptr [ecx + 8]		// 0x00048f6e
		faddp	st(1), st		// 0x00048f71
		fstp	dword ptr [eax]		// 0x00048f73
		mov	ecx, dword ptr [edx + 4]		// 0x00048f75
		fld	dword ptr [ebp - 0xc]		// 0x00048f78
		lea	ecx, [ecx + ecx*2]		// 0x00048f7b
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00048f7e
		lea	ecx, [esi + ecx*4]		// 0x00048f82
		fld	dword ptr [ebp - 8]		// 0x00048f85
		fmul	dword ptr [ecx + 8]		// 0x00048f88
		faddp	st(1), st		// 0x00048f8b
		fld	dword ptr [ebp - 0x10]		// 0x00048f8d
		fmul	dword ptr [ecx]		// 0x00048f90
		faddp	st(1), st		// 0x00048f92
		fstp	dword ptr [eax + 8]		// 0x00048f94
		fld	dword ptr [ebp - 4]		// 0x00048f97
		fmul	dword ptr [ecx]		// 0x00048f9a
		fld	dword ptr [ebp]		// 0x00048f9c
		fmul	dword ptr [ecx + 4]		// 0x00048f9f
		faddp	st(1), st		// 0x00048fa2
		fld	dword ptr [ebp + 4]		// 0x00048fa4
		add	edx, 0x10		// 0x00048fa7
		fmul	dword ptr [ecx + 8]		// 0x00048faa
		add	eax, 0x30		// 0x00048fad
		dec	edi		// 0x00048fb0
		faddp	st(1), st		// 0x00048fb1
		fstp	dword ptr [eax - 0x24]		// 0x00048fb3
		jne	L48ec6		// 0x00048fb6
		mov	esi, dword ptr [ebp - 0x14]		// 0x00048fbc
		mov	edi, dword ptr [ebp + 0x3c]		// 0x00048fbf
L48fc2:
		cmp	ebx, edi		// 0x00048fc2
		jae	L49018		// 0x00048fc4
		lea	edx, [ebx + ebx*2]		// 0x00048fc6
		lea	ecx, [esi + edx*4]		// 0x00048fc9
		_emit	0x8d
		_emit	0x64
		_emit	0x24
		_emit	0x00		// 0x00048fcc lea esp, [esp]
L48fd0:
		mov	eax, dword ptr [ebp + 0x44]		// 0x00048fd0
		fld	dword ptr [ebp - 0xc]		// 0x00048fd3
		mov	eax, dword ptr [eax + ebx*4]		// 0x00048fd6
		lea	edx, [eax + eax*2]		// 0x00048fd9
		mov	eax, dword ptr [ebp + 0x40]		// 0x00048fdc
		fmul	dword ptr [eax + edx*4 + 4]		// 0x00048fdf
		lea	eax, [eax + edx*4]		// 0x00048fe3
		fld	dword ptr [ebp - 8]		// 0x00048fe6
		inc	ebx		// 0x00048fe9
		fmul	dword ptr [eax + 8]		// 0x00048fea
		add	ecx, 0xc		// 0x00048fed
		cmp	ebx, edi		// 0x00048ff0
		faddp	st(1), st		// 0x00048ff2
		fld	dword ptr [ebp - 0x10]		// 0x00048ff4
		fmul	dword ptr [eax]		// 0x00048ff7
		faddp	st(1), st		// 0x00048ff9
		fstp	dword ptr [ecx - 0xc]		// 0x00048ffb
		fld	dword ptr [ebp - 4]		// 0x00048ffe
		fmul	dword ptr [eax]		// 0x00049001
		fld	dword ptr [ebp]		// 0x00049003
		fmul	dword ptr [eax + 4]		// 0x00049006
		faddp	st(1), st		// 0x00049009
		fld	dword ptr [ebp + 4]		// 0x0004900b
		fmul	dword ptr [eax + 8]		// 0x0004900e
		faddp	st(1), st		// 0x00049011
		fstp	dword ptr [ecx - 8]		// 0x00049013
		jb	L48fd0		// 0x00049016
L49018:
		mov	ecx, dword ptr [ebp + 0x44]		// 0x00049018
		fld	dword ptr [ebp + 0xc]		// 0x0004901b
		mov	eax, dword ptr [ecx]		// 0x0004901e
		mov	esi, dword ptr [ebp + 0x54]		// 0x00049020
		lea	edx, [eax + eax*2]		// 0x00049023
		mov	eax, dword ptr [ebp + 0x40]		// 0x00049026
		fmul	dword ptr [eax + edx*4 + 4]		// 0x00049029
		lea	eax, [eax + edx*4]		// 0x0004902d
		fld	dword ptr [ebp + 0x10]		// 0x00049030
		mov	ebx, dword ptr [ebp + 0x48]		// 0x00049033
		fmul	dword ptr [eax + 8]		// 0x00049036
		faddp	st(1), st		// 0x00049039
		fld	dword ptr [ebp + 8]		// 0x0004903b
		fmul	dword ptr [eax]		// 0x0004903e
		xor	eax, eax		// 0x00049040
		mov	dword ptr [ebp + 0x54], eax		// 0x00049042
		faddp	st(1), st		// 0x00049045
		fstp	dword ptr [ebp - 0x58]		// 0x00049047
		fld	dword ptr [ebp + 0xc]		// 0x0004904a
		fmul	dword ptr [esi + 4]		// 0x0004904d
		fld	dword ptr [ebp + 0x10]		// 0x00049050
		fmul	dword ptr [esi + 8]		// 0x00049053
		faddp	st(1), st		// 0x00049056
		fld	dword ptr [ebp + 8]		// 0x00049058
		fmul	dword ptr [esi]		// 0x0004905b
		faddp	st(1), st		// 0x0004905d
		fstp	dword ptr [ebp - 0x98]		// 0x0004905f
		fld	dword ptr [ebp + 0x10]		// 0x00049065
		fmul	dword ptr [esi + 0x18]		// 0x00049068
		fld	dword ptr [ebp + 8]		// 0x0004906b
		fmul	dword ptr [esi + 0x10]		// 0x0004906e
		faddp	st(1), st		// 0x00049071
		fld	dword ptr [ebp + 0xc]		// 0x00049073
		fmul	dword ptr [esi + 0x14]		// 0x00049076
		faddp	st(1), st		// 0x00049079
		fstp	dword ptr [ebp - 0x88]		// 0x0004907b
		fld	dword ptr [ebp + 8]		// 0x00049081
		fmul	dword ptr [esi + 0x20]		// 0x00049084
		fld	dword ptr [ebp + 0xc]		// 0x00049087
		fmul	dword ptr [esi + 0x24]		// 0x0004908a
		faddp	st(1), st		// 0x0004908d
		fld	dword ptr [ebp + 0x10]		// 0x0004908f
		fmul	dword ptr [esi + 0x28]		// 0x00049092
		faddp	st(1), st		// 0x00049095
		fstp	dword ptr [ebp - 0x78]		// 0x00049097
		fld	dword ptr [ebp + 0x10]		// 0x0004909a
		fmul	dword ptr [esi + 0x38]		// 0x0004909d
		fld	dword ptr [ebp + 8]		// 0x000490a0
		fmul	dword ptr [esi + 0x30]		// 0x000490a3
		faddp	st(1), st		// 0x000490a6
		fld	dword ptr [ebp + 0xc]		// 0x000490a8
		fmul	dword ptr [esi + 0x34]		// 0x000490ab
		faddp	st(1), st		// 0x000490ae
		fstp	dword ptr [ebp - 0x68]		// 0x000490b0
		fld	dword ptr [ebp - 0x34]		// 0x000490b3
		fmul	dword ptr [ebx + 8]		// 0x000490b6
		fld	dword ptr [ebp - 0x38]		// 0x000490b9
		fmul	dword ptr [ebx + 4]		// 0x000490bc
		faddp	st(1), st		// 0x000490bf
		fld	dword ptr [ebp - 0x3c]		// 0x000490c1
		fmul	dword ptr [ebx]		// 0x000490c4
		faddp	st(1), st		// 0x000490c6
		fstp	dword ptr [ebp - 0x20]		// 0x000490c8
		fld	dword ptr [ebp - 0x34]		// 0x000490cb
		fmul	dword ptr [ebx + 0x18]		// 0x000490ce
		fld	dword ptr [ebp - 0x3c]		// 0x000490d1
		fmul	dword ptr [ebx + 0x10]		// 0x000490d4
		faddp	st(1), st		// 0x000490d7
		fld	dword ptr [ebp - 0x38]		// 0x000490d9
		fmul	dword ptr [ebx + 0x14]		// 0x000490dc
		faddp	st(1), st		// 0x000490df
		fstp	dword ptr [ebp - 0x1c]		// 0x000490e1
		fld	dword ptr [ebp - 0x3c]		// 0x000490e4
		fmul	dword ptr [ebx + 0x20]		// 0x000490e7
		fld	dword ptr [ebp - 0x38]		// 0x000490ea
		fmul	dword ptr [ebx + 0x24]		// 0x000490ed
		faddp	st(1), st		// 0x000490f0
		fld	dword ptr [ebp - 0x34]		// 0x000490f2
		fmul	dword ptr [ebx + 0x28]		// 0x000490f5
		faddp	st(1), st		// 0x000490f8
		fstp	dword ptr [ebp - 0x18]		// 0x000490fa
		mov	ecx, dword ptr [ebp + 0x28]		// 0x000490fd
		test	ecx, ecx		// 0x00049100
		mov	edi, dword ptr [ebp + 0x34]		// 0x00049102
		jbe	L491f1		// 0x00049105
		jmp	L49110		// 0x0004910b
		_emit	0x8d
		_emit	0x49
		_emit	0x00		// 0x0004910d lea ecx, [ecx]
L49110:
		mov	ecx, dword ptr [ebp + 0x30]		// 0x00049110
		fld	dword ptr [ebp - 0x78]		// 0x00049113
		mov	eax, dword ptr [ecx + eax*4]		// 0x00049116
		lea	edx, [eax + eax*2]		// 0x00049119
		mov	eax, dword ptr [ebp + 0x2c]		// 0x0004911c
		fmul	dword ptr [eax + edx*4 + 8]		// 0x0004911f
		lea	ecx, [eax + edx*4]		// 0x00049123
		fld	dword ptr [ebp - 0x88]		// 0x00049126
		mov	dword ptr [ebp + 0x48], ecx		// 0x0004912c
		fmul	dword ptr [ecx + 4]		// 0x0004912f
		faddp	st(1), st		// 0x00049132
		fld	dword ptr [ebp - 0x98]		// 0x00049134
		fmul	dword ptr [ecx]		// 0x0004913a
		faddp	st(1), st		// 0x0004913c
		fadd	dword ptr [ebp - 0x68]		// 0x0004913e
		fcomp	dword ptr [ebp - 0x58]		// 0x00049141
		fnstsw	ax		// 0x00049144
		test	ah, 5		// 0x00049146
		jp	L491df		// 0x00049149
		fld	dword ptr [ecx]		// 0x0004914f
		fmul	dword ptr [esi]		// 0x00049151
		fld	dword ptr [ecx + 4]		// 0x00049153
		fmul	dword ptr [esi + 0x10]		// 0x00049156
		faddp	st(1), st		// 0x00049159
		fld	dword ptr [esi + 0x20]		// 0x0004915b
		fmul	dword ptr [ecx + 8]		// 0x0004915e
		faddp	st(1), st		// 0x00049161
		fadd	dword ptr [esi + 0x30]		// 0x00049163
		fstp	dword ptr [ebp + 0x14]		// 0x00049166
		fld	dword ptr [ecx]		// 0x00049169
		fmul	dword ptr [esi + 4]		// 0x0004916b
		fld	dword ptr [ecx + 4]		// 0x0004916e
		fmul	dword ptr [esi + 0x14]		// 0x00049171
		faddp	st(1), st		// 0x00049174
		fld	dword ptr [esi + 0x24]		// 0x00049176
		fmul	dword ptr [ecx + 8]		// 0x00049179
		faddp	st(1), st		// 0x0004917c
		fadd	dword ptr [esi + 0x34]		// 0x0004917e
		fstp	dword ptr [ebp + 0x18]		// 0x00049181
		fld	dword ptr [ecx + 8]		// 0x00049184
		fmul	dword ptr [esi + 0x28]		// 0x00049187
		fld	dword ptr [ecx + 4]		// 0x0004918a
		fmul	dword ptr [esi + 0x18]		// 0x0004918d
		faddp	st(1), st		// 0x00049190
		fld	dword ptr [ecx]		// 0x00049192
		mov	ecx, dword ptr [ebp + 0x4c]		// 0x00049194
		fmul	dword ptr [esi + 8]		// 0x00049197
		faddp	st(1), st		// 0x0004919a
		fadd	dword ptr [esi + 0x38]		// 0x0004919c
		fstp	dword ptr [ebp + 0x1c]		// 0x0004919f
		fld	dword ptr [ebp - 0x1c]		// 0x000491a2
		fmul	dword ptr [ecx + 4]		// 0x000491a5
		fld	dword ptr [ebp - 0x18]		// 0x000491a8
		fmul	dword ptr [ecx + 8]		// 0x000491ab
		faddp	st(1), st		// 0x000491ae
		fld	dword ptr [ebp - 0x20]		// 0x000491b0
		fmul	dword ptr [ecx]		// 0x000491b3
		faddp	st(1), st		// 0x000491b5
		fld	st(0)		// 0x000491b7
		fld	qword ptr kContactMinusTenth7		// 0x000491b9
		fcomp	st(1)		// 0x000491bf
		fnstsw	ax		// 0x000491c1
		test	ah, 5		// 0x000491c3
		jp	L49921		// 0x000491c6
		fcomp	qword ptr kContactTenth7		// 0x000491cc
		fnstsw	ax		// 0x000491d2
		test	ah, 5		// 0x000491d4
		jp	L49923		// 0x000491d7
		fstp	st(0)		// 0x000491dd
L491df:
		mov	eax, dword ptr [ebp + 0x54]		// 0x000491df
		mov	ecx, dword ptr [ebp + 0x28]		// 0x000491e2
		inc	eax		// 0x000491e5
		cmp	eax, ecx		// 0x000491e6
		mov	dword ptr [ebp + 0x54], eax		// 0x000491e8
		jb	L49110		// 0x000491eb
L491f1:
		mov	ecx, dword ptr [ebp + 0x38]		// 0x000491f1
		lea	edx, [ebp - 0x10]		// 0x000491f4
		push	edx		// 0x000491f7
		lea	eax, [ebp - 0x54]		// 0x000491f8
		push	eax		// 0x000491fb
		push	ecx		// 0x000491fc
		mov	dword ptr [ebp - 0x54], 0		// 0x000491fd
		mov	dword ptr [ebp - 0x50], 0		// 0x00049204
		mov	dword ptr [ebp - 0x4c], 0x3f800000		// 0x0004920b
		call	dword ptr _imp__NxFindRotationMatrix		// 0x00049212
		mov	ecx, dword ptr [ebp + 0x28]		// 0x00049218
		add	esp, 0xc		// 0x0004921b
		xor	edx, edx		// 0x0004921e
		cmp	ecx, 4		// 0x00049220
		jl	L49347		// 0x00049223
		mov	edx, dword ptr [ebp + 0x30]		// 0x00049229
		mov	eax, dword ptr [ebp - 0x14]		// 0x0004922c
		add	ecx, -4		// 0x0004922f
		shr	ecx, 2		// 0x00049232
		add	edx, 8		// 0x00049235
		add	eax, 0x1c		// 0x00049238
		inc	ecx		// 0x0004923b
		mov	dword ptr [ebp + 0x50], ecx		// 0x0004923c
		shl	ecx, 2		// 0x0004923f
		mov	dword ptr [ebp + 0x54], ecx		// 0x00049242
L49245:
		mov	ecx, dword ptr [edx - 8]		// 0x00049245
		fld	dword ptr [ebp - 0xc]		// 0x00049248
		mov	esi, dword ptr [ebp + 0x2c]		// 0x0004924b
		lea	ecx, [ecx + ecx*2]		// 0x0004924e
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x00049251
		lea	ecx, [esi + ecx*4]		// 0x00049255
		fld	dword ptr [ebp - 8]		// 0x00049258
		fmul	dword ptr [ecx + 8]		// 0x0004925b
		faddp	st(1), st		// 0x0004925e
		fld	dword ptr [ebp - 0x10]		// 0x00049260
		fmul	dword ptr [ecx]		// 0x00049263
		faddp	st(1), st		// 0x00049265
		fstp	dword ptr [eax - 0x1c]		// 0x00049267
		fld	dword ptr [ebp - 4]		// 0x0004926a
		fmul	dword ptr [ecx]		// 0x0004926d
		fld	dword ptr [ebp]		// 0x0004926f
		fmul	dword ptr [ecx + 4]		// 0x00049272
		faddp	st(1), st		// 0x00049275
		fld	dword ptr [ebp + 4]		// 0x00049277
		fmul	dword ptr [ecx + 8]		// 0x0004927a
		faddp	st(1), st		// 0x0004927d
		fstp	dword ptr [eax - 0x18]		// 0x0004927f
		mov	ecx, dword ptr [edx - 4]		// 0x00049282
		fld	dword ptr [ebp - 0xc]		// 0x00049285
		lea	ecx, [ecx + ecx*2]		// 0x00049288
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x0004928b
		lea	ecx, [esi + ecx*4]		// 0x0004928f
		fld	dword ptr [ebp - 8]		// 0x00049292
		fmul	dword ptr [ecx + 8]		// 0x00049295
		faddp	st(1), st		// 0x00049298
		fld	dword ptr [ebp - 0x10]		// 0x0004929a
		fmul	dword ptr [ecx]		// 0x0004929d
		faddp	st(1), st		// 0x0004929f
		fstp	dword ptr [eax - 0x10]		// 0x000492a1
		fld	dword ptr [ebp - 4]		// 0x000492a4
		fmul	dword ptr [ecx]		// 0x000492a7
		fld	dword ptr [ebp]		// 0x000492a9
		fmul	dword ptr [ecx + 4]		// 0x000492ac
		faddp	st(1), st		// 0x000492af
		fld	dword ptr [ebp + 4]		// 0x000492b1
		fmul	dword ptr [ecx + 8]		// 0x000492b4
		faddp	st(1), st		// 0x000492b7
		fstp	dword ptr [eax - 0xc]		// 0x000492b9
		mov	ecx, dword ptr [edx]		// 0x000492bc
		fld	dword ptr [ebp - 0xc]		// 0x000492be
		lea	ecx, [ecx + ecx*2]		// 0x000492c1
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x000492c4
		lea	ecx, [esi + ecx*4]		// 0x000492c8
		fld	dword ptr [ebp - 8]		// 0x000492cb
		fmul	dword ptr [ecx + 8]		// 0x000492ce
		faddp	st(1), st		// 0x000492d1
		fld	dword ptr [ebp - 0x10]		// 0x000492d3
		fmul	dword ptr [ecx]		// 0x000492d6
		faddp	st(1), st		// 0x000492d8
		fstp	dword ptr [eax - 4]		// 0x000492da
		fld	dword ptr [ebp - 4]		// 0x000492dd
		fmul	dword ptr [ecx]		// 0x000492e0
		fld	dword ptr [ebp]		// 0x000492e2
		fmul	dword ptr [ecx + 4]		// 0x000492e5
		faddp	st(1), st		// 0x000492e8
		fld	dword ptr [ebp + 4]		// 0x000492ea
		fmul	dword ptr [ecx + 8]		// 0x000492ed
		faddp	st(1), st		// 0x000492f0
		fstp	dword ptr [eax]		// 0x000492f2
		mov	ecx, dword ptr [edx + 4]		// 0x000492f4
		fld	dword ptr [ebp - 0xc]		// 0x000492f7
		lea	ecx, [ecx + ecx*2]		// 0x000492fa
		fmul	dword ptr [esi + ecx*4 + 4]		// 0x000492fd
		lea	ecx, [esi + ecx*4]		// 0x00049301
		fld	dword ptr [ebp - 8]		// 0x00049304
		fmul	dword ptr [ecx + 8]		// 0x00049307
		faddp	st(1), st		// 0x0004930a
		fld	dword ptr [ebp - 0x10]		// 0x0004930c
		fmul	dword ptr [ecx]		// 0x0004930f
		faddp	st(1), st		// 0x00049311
		fstp	dword ptr [eax + 8]		// 0x00049313
		fld	dword ptr [ebp - 4]		// 0x00049316
		fmul	dword ptr [ecx]		// 0x00049319
		fld	dword ptr [ebp]		// 0x0004931b
		fmul	dword ptr [ecx + 4]		// 0x0004931e
		faddp	st(1), st		// 0x00049321
		fld	dword ptr [ebp + 4]		// 0x00049323
		add	edx, 0x10		// 0x00049326
		fmul	dword ptr [ecx + 8]		// 0x00049329
		mov	ecx, dword ptr [ebp + 0x50]		// 0x0004932c
		add	eax, 0x30		// 0x0004932f
		dec	ecx		// 0x00049332
		faddp	st(1), st		// 0x00049333
		mov	dword ptr [ebp + 0x50], ecx		// 0x00049335
		fstp	dword ptr [eax - 0x24]		// 0x00049338
		jne	L49245		// 0x0004933b
		mov	ecx, dword ptr [ebp + 0x28]		// 0x00049341
		mov	edx, dword ptr [ebp + 0x54]		// 0x00049344
L49347:
		cmp	edx, ecx		// 0x00049347
		jae	L4939f		// 0x00049349
		mov	ecx, dword ptr [ebp - 0x14]		// 0x0004934b
		lea	eax, [edx + edx*2]		// 0x0004934e
		lea	ecx, [ecx + eax*4]		// 0x00049351
L49354:
		mov	eax, dword ptr [ebp + 0x30]		// 0x00049354
		fld	dword ptr [ebp - 0xc]		// 0x00049357
		mov	eax, dword ptr [eax + edx*4]		// 0x0004935a
		mov	esi, dword ptr [ebp + 0x2c]		// 0x0004935d
		lea	eax, [eax + eax*2]		// 0x00049360
		fmul	dword ptr [esi + eax*4 + 4]		// 0x00049363
		lea	eax, [esi + eax*4]		// 0x00049367
		fld	dword ptr [ebp - 8]		// 0x0004936a
		inc	edx		// 0x0004936d
		fmul	dword ptr [eax + 8]		// 0x0004936e
		add	ecx, 0xc		// 0x00049371
		faddp	st(1), st		// 0x00049374
		fld	dword ptr [ebp - 0x10]		// 0x00049376
		fmul	dword ptr [eax]		// 0x00049379
		faddp	st(1), st		// 0x0004937b
		fstp	dword ptr [ecx - 0xc]		// 0x0004937d
		fld	dword ptr [ebp - 4]		// 0x00049380
		fmul	dword ptr [eax]		// 0x00049383
		fld	dword ptr [ebp]		// 0x00049385
		fmul	dword ptr [eax + 4]		// 0x00049388
		faddp	st(1), st		// 0x0004938b
		fld	dword ptr [ebp + 4]		// 0x0004938d
		fmul	dword ptr [eax + 8]		// 0x00049390
		mov	eax, dword ptr [ebp + 0x28]		// 0x00049393
		cmp	edx, eax		// 0x00049396
		faddp	st(1), st		// 0x00049398
		fstp	dword ptr [ecx - 8]		// 0x0004939a
		jb	L49354		// 0x0004939d
L4939f:
		mov	ecx, dword ptr [ebp + 0x30]		// 0x0004939f
		fld	dword ptr [ebp + 0xc]		// 0x000493a2
		mov	eax, dword ptr [ecx]		// 0x000493a5
		mov	esi, dword ptr [ebp + 0x58]		// 0x000493a7
		lea	edx, [eax + eax*2]		// 0x000493aa
		mov	eax, dword ptr [ebp + 0x2c]		// 0x000493ad
		fmul	dword ptr [eax + edx*4 + 4]		// 0x000493b0
		lea	eax, [eax + edx*4]		// 0x000493b4
		fld	dword ptr [ebp + 0x10]		// 0x000493b7
		fmul	dword ptr [eax + 8]		// 0x000493ba
		faddp	st(1), st		// 0x000493bd
		fld	dword ptr [ebp + 8]		// 0x000493bf
		fmul	dword ptr [eax]		// 0x000493c2
		faddp	st(1), st		// 0x000493c4
		fstp	dword ptr [ebp + 0x50]		// 0x000493c6
		fld	dword ptr [ebp - 0x10]		// 0x000493c9
		fmul	dword ptr [esi]		// 0x000493cc
		fld	dword ptr [ebp - 0xc]		// 0x000493ce
		fmul	dword ptr [esi + 4]		// 0x000493d1
		faddp	st(1), st		// 0x000493d4
		fld	dword ptr [ebp - 8]		// 0x000493d6
		fmul	dword ptr [esi + 8]		// 0x000493d9
		faddp	st(1), st		// 0x000493dc
		fstp	dword ptr [ebp - 0xa0]		// 0x000493de
		fld	dword ptr [ebp]		// 0x000493e4
		fmul	dword ptr [esi + 4]		// 0x000493e7
		fld	dword ptr [ebp + 4]		// 0x000493ea
		fmul	dword ptr [esi + 8]		// 0x000493ed
		faddp	st(1), st		// 0x000493f0
		fld	dword ptr [ebp - 4]		// 0x000493f2
		fmul	dword ptr [esi]		// 0x000493f5
		faddp	st(1), st		// 0x000493f7
		fstp	dword ptr [ebp - 0x9c]		// 0x000493f9
		fld	dword ptr [ebp + 0xc]		// 0x000493ff
		fmul	dword ptr [esi + 4]		// 0x00049402
		fld	dword ptr [ebp + 0x10]		// 0x00049405
		fmul	dword ptr [esi + 8]		// 0x00049408
		faddp	st(1), st		// 0x0004940b
		fld	dword ptr [ebp + 8]		// 0x0004940d
		fmul	dword ptr [esi]		// 0x00049410
		faddp	st(1), st		// 0x00049412
		fstp	dword ptr [ebp - 0x98]		// 0x00049414
		fld	dword ptr [ebp - 0x10]		// 0x0004941a
		fmul	dword ptr [esi + 0x10]		// 0x0004941d
		fld	dword ptr [ebp - 8]		// 0x00049420
		fmul	dword ptr [esi + 0x18]		// 0x00049423
		faddp	st(1), st		// 0x00049426
		fld	dword ptr [ebp - 0xc]		// 0x00049428
		fmul	dword ptr [esi + 0x14]		// 0x0004942b
		faddp	st(1), st		// 0x0004942e
		fstp	dword ptr [ebp - 0x90]		// 0x00049430
		fld	dword ptr [ebp + 4]		// 0x00049436
		fmul	dword ptr [esi + 0x18]		// 0x00049439
		fld	dword ptr [ebp - 4]		// 0x0004943c
		fmul	dword ptr [esi + 0x10]		// 0x0004943f
		faddp	st(1), st		// 0x00049442
		fld	dword ptr [ebp]		// 0x00049444
		fmul	dword ptr [esi + 0x14]		// 0x00049447
		faddp	st(1), st		// 0x0004944a
		fstp	dword ptr [ebp - 0x8c]		// 0x0004944c
		fld	dword ptr [ebp + 0x10]		// 0x00049452
		fmul	dword ptr [esi + 0x18]		// 0x00049455
		fld	dword ptr [ebp + 8]		// 0x00049458
		fmul	dword ptr [esi + 0x10]		// 0x0004945b
		faddp	st(1), st		// 0x0004945e
		fld	dword ptr [ebp + 0xc]		// 0x00049460
		fmul	dword ptr [esi + 0x14]		// 0x00049463
		faddp	st(1), st		// 0x00049466
		fstp	dword ptr [ebp - 0x88]		// 0x00049468
		fld	dword ptr [ebp - 0x10]		// 0x0004946e
		fmul	dword ptr [esi + 0x20]		// 0x00049471
		fld	dword ptr [ebp - 8]		// 0x00049474
		fmul	dword ptr [esi + 0x28]		// 0x00049477
		faddp	st(1), st		// 0x0004947a
		fld	dword ptr [ebp - 0xc]		// 0x0004947c
		fmul	dword ptr [esi + 0x24]		// 0x0004947f
		faddp	st(1), st		// 0x00049482
		fstp	dword ptr [ebp - 0x80]		// 0x00049484
		fld	dword ptr [ebp + 4]		// 0x00049487
		fmul	dword ptr [esi + 0x28]		// 0x0004948a
		fld	dword ptr [ebp - 4]		// 0x0004948d
		fmul	dword ptr [esi + 0x20]		// 0x00049490
		mov	ecx, dword ptr [ebp + 0x3c]		// 0x00049493
		xor	eax, eax		// 0x00049496
		test	ecx, ecx		// 0x00049498
		faddp	st(1), st		// 0x0004949a
		mov	dword ptr [ebp + 0x54], eax		// 0x0004949c
		fld	dword ptr [ebp]		// 0x0004949f
		fmul	dword ptr [esi + 0x24]		// 0x000494a2
		faddp	st(1), st		// 0x000494a5
		fstp	dword ptr [ebp - 0x7c]		// 0x000494a7
		fld	dword ptr [ebp + 0x10]		// 0x000494aa
		fmul	dword ptr [esi + 0x28]		// 0x000494ad
		fld	dword ptr [ebp + 8]		// 0x000494b0
		fmul	dword ptr [esi + 0x20]		// 0x000494b3
		faddp	st(1), st		// 0x000494b6
		fld	dword ptr [ebp + 0xc]		// 0x000494b8
		fmul	dword ptr [esi + 0x24]		// 0x000494bb
		faddp	st(1), st		// 0x000494be
		fstp	dword ptr [ebp - 0x78]		// 0x000494c0
		fld	dword ptr [ebp - 0x10]		// 0x000494c3
		fmul	dword ptr [esi + 0x30]		// 0x000494c6
		fld	dword ptr [ebp - 8]		// 0x000494c9
		fmul	dword ptr [esi + 0x38]		// 0x000494cc
		faddp	st(1), st		// 0x000494cf
		fld	dword ptr [ebp - 0xc]		// 0x000494d1
		fmul	dword ptr [esi + 0x34]		// 0x000494d4
		faddp	st(1), st		// 0x000494d7
		fstp	dword ptr [ebp - 0x70]		// 0x000494d9
		fld	dword ptr [ebp + 4]		// 0x000494dc
		fmul	dword ptr [esi + 0x38]		// 0x000494df
		fld	dword ptr [ebp - 4]		// 0x000494e2
		fmul	dword ptr [esi + 0x30]		// 0x000494e5
		faddp	st(1), st		// 0x000494e8
		fld	dword ptr [ebp]		// 0x000494ea
		fmul	dword ptr [esi + 0x34]		// 0x000494ed
		faddp	st(1), st		// 0x000494f0
		fstp	dword ptr [ebp - 0x6c]		// 0x000494f2
		fld	dword ptr [ebp + 0x10]		// 0x000494f5
		fmul	dword ptr [esi + 0x38]		// 0x000494f8
		fld	dword ptr [ebp + 8]		// 0x000494fb
		fmul	dword ptr [esi + 0x30]		// 0x000494fe
		faddp	st(1), st		// 0x00049501
		fld	dword ptr [ebp + 0xc]		// 0x00049503
		fmul	dword ptr [esi + 0x34]		// 0x00049506
		faddp	st(1), st		// 0x00049509
		fstp	dword ptr [ebp - 0x68]		// 0x0004950b
		jbe	L49659		// 0x0004950e
L49514:
		mov	ecx, dword ptr [ebp + 0x44]		// 0x00049514
		fld	dword ptr [ebp - 0x78]		// 0x00049517
		mov	eax, dword ptr [ecx + eax*4]		// 0x0004951a
		lea	edx, [eax + eax*2]		// 0x0004951d
		mov	eax, dword ptr [ebp + 0x40]		// 0x00049520
		fmul	dword ptr [eax + edx*4 + 8]		// 0x00049523
		lea	ecx, [eax + edx*4]		// 0x00049527
		fld	dword ptr [ebp - 0x88]		// 0x0004952a
		mov	dword ptr [ebp + 0x48], ecx		// 0x00049530
		fmul	dword ptr [ecx + 4]		// 0x00049533
		faddp	st(1), st		// 0x00049536
		fld	dword ptr [ebp - 0x98]		// 0x00049538
		fmul	dword ptr [ecx]		// 0x0004953e
		faddp	st(1), st		// 0x00049540
		fadd	dword ptr [ebp - 0x68]		// 0x00049542
		fst	dword ptr [ebp - 0x18]		// 0x00049545
		fcomp	dword ptr [ebp + 0x50]		// 0x00049548
		fnstsw	ax		// 0x0004954b
		test	ah, 5		// 0x0004954d
		jp	L49647		// 0x00049550
		fld	dword ptr [ebp - 0x80]		// 0x00049556
		mov	eax, dword ptr [ebp + 0x28]		// 0x00049559
		fmul	dword ptr [ecx + 8]		// 0x0004955c
		fld	dword ptr [ebp - 0x90]		// 0x0004955f
		fmul	dword ptr [ecx + 4]		// 0x00049565
		faddp	st(1), st		// 0x00049568
		fld	dword ptr [ebp - 0xa0]		// 0x0004956a
		fmul	dword ptr [ecx]		// 0x00049570
		faddp	st(1), st		// 0x00049572
		fadd	dword ptr [ebp - 0x70]		// 0x00049574
		fstp	dword ptr [ebp - 0x20]		// 0x00049577
		mov	edx, dword ptr [ebp - 0x20]		// 0x0004957a
		fld	dword ptr [ebp - 0x7c]		// 0x0004957d
		fmul	dword ptr [ecx + 8]		// 0x00049580
		fld	dword ptr [ebp - 0x8c]		// 0x00049583
		fmul	dword ptr [ecx + 4]		// 0x00049589
		faddp	st(1), st		// 0x0004958c
		fld	dword ptr [ebp - 0x9c]		// 0x0004958e
		fmul	dword ptr [ecx]		// 0x00049594
		faddp	st(1), st		// 0x00049596
		fadd	dword ptr [ebp - 0x6c]		// 0x00049598
		fstp	dword ptr [ebp - 0x1c]		// 0x0004959b
		mov	ecx, dword ptr [ebp - 0x1c]		// 0x0004959e
		push	ecx		// 0x000495a1
		mov	ecx, dword ptr [ebp - 0x14]		// 0x000495a2
		push	edx		// 0x000495a5
		call	nxPolygonContainsPoint		// 0x000495a6
		add	esp, 8		// 0x000495ab
		test	eax, eax		// 0x000495ae
		je	L49647		// 0x000495b0
		mov	eax, dword ptr [ebp + 0x48]		// 0x000495b6
		fld	dword ptr [ebx + 0x20]		// 0x000495b9
		fmul	dword ptr [eax + 8]		// 0x000495bc
		mov	ecx, dword ptr [ebp + 0x78]		// 0x000495bf
		fld	dword ptr [eax]		// 0x000495c2
		mov	edx, dword ptr [ebp + 0x74]		// 0x000495c4
		fmul	dword ptr [ebx]		// 0x000495c7
		faddp	st(1), st		// 0x000495c9
		fld	dword ptr [ebx + 0x10]		// 0x000495cb
		fmul	dword ptr [eax + 4]		// 0x000495ce
		faddp	st(1), st		// 0x000495d1
		fadd	dword ptr [ebx + 0x30]		// 0x000495d3
		fstp	dword ptr [ebp - 0x54]		// 0x000495d6
		fld	dword ptr [ebx + 0x24]		// 0x000495d9
		fmul	dword ptr [eax + 8]		// 0x000495dc
		fld	dword ptr [ebx + 0x14]		// 0x000495df
		fmul	dword ptr [eax + 4]		// 0x000495e2
		faddp	st(1), st		// 0x000495e5
		fld	dword ptr [ebx + 4]		// 0x000495e7
		fmul	dword ptr [eax]		// 0x000495ea
		faddp	st(1), st		// 0x000495ec
		fadd	dword ptr [ebx + 0x34]		// 0x000495ee
		fstp	dword ptr [ebp - 0x50]		// 0x000495f1
		fld	dword ptr [ebx + 0x28]		// 0x000495f4
		fmul	dword ptr [eax + 8]		// 0x000495f7
		fld	dword ptr [ebx + 8]		// 0x000495fa
		fmul	dword ptr [eax]		// 0x000495fd
		faddp	st(1), st		// 0x000495ff
		fld	dword ptr [ebx + 0x18]		// 0x00049601
		fmul	dword ptr [eax + 4]		// 0x00049604
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00049607
		push	eax		// 0x0004960a
		mov	eax, dword ptr [ebp + 0x70]		// 0x0004960b
		push	ecx		// 0x0004960e
		faddp	st(1), st		// 0x0004960f
		push	edx		// 0x00049611
		push	eax		// 0x00049612
		fadd	dword ptr [ebx + 0x38]		// 0x00049613
		mov	eax, dword ptr [ebp + 0x60]		// 0x00049616
		lea	ecx, [ebp - 0x3c]		// 0x00049619
		push	ecx		// 0x0004961c
		fstp	dword ptr [ebp - 0x4c]		// 0x0004961d
		lea	edx, [ebp - 0x54]		// 0x00049620
		fld	dword ptr [ebp - 0x18]		// 0x00049623
		push	edx		// 0x00049626
		fsub	dword ptr [ebp + 0x50]		// 0x00049627
		mov	edx, dword ptr [ebp + 0x5c]		// 0x0004962a
		push	ecx		// 0x0004962d
		mov	ecx, dword ptr [eax + 0x9c]		// 0x0004962e
		mov	eax, dword ptr [edx + 0x9c]		// 0x00049634
		fstp	dword ptr [esp]		// 0x0004963a
		push	ecx		// 0x0004963d
		mov	ecx, dword ptr [ebp + 0x64]		// 0x0004963e
		push	eax		// 0x00049641
		call	NxEmitContactFeatures		// 0x00049642
L49647:
		mov	eax, dword ptr [ebp + 0x54]		// 0x00049647
		mov	ecx, dword ptr [ebp + 0x3c]		// 0x0004964a
		inc	eax		// 0x0004964d
		cmp	eax, ecx		// 0x0004964e
		mov	dword ptr [ebp + 0x54], eax		// 0x00049650
		jb	L49514		// 0x00049653
L49659:
		lea	eax, [ecx + ecx*2]		// 0x00049659
		shl	eax, 2		// 0x0004965c
		add	eax, 3		// 0x0004965f
		and	eax, 0xfffffffc		// 0x00049662
		call	_chkstk		// 0x00049665
		mov	eax, dword ptr [ebp + 0x3c]		// 0x0004966a
		mov	ebx, esp		// 0x0004966d
		xor	ecx, ecx		// 0x0004966f
		cmp	eax, 4		// 0x00049671
		mov	dword ptr [ebp + 0x50], ebx		// 0x00049674
		jl	L49874		// 0x00049677
		mov	ecx, dword ptr [ebp + 0x44]		// 0x0004967d
		add	eax, -4		// 0x00049680
		shr	eax, 2		// 0x00049683
		add	ecx, 8		// 0x00049686
		inc	eax		// 0x00049689
		mov	dword ptr [ebp + 0x54], eax		// 0x0004968a
		shl	eax, 2		// 0x0004968d
		lea	edx, [ebx + 0x18]		// 0x00049690
		mov	dword ptr [ebp + 0x48], eax		// 0x00049693
L49696:
		mov	eax, dword ptr [ecx - 8]		// 0x00049696
		mov	ebx, dword ptr [ebp + 0x40]		// 0x00049699
		lea	eax, [eax + eax*2]		// 0x0004969c
		fld	dword ptr [ebx + eax*4]		// 0x0004969f
		lea	eax, [ebx + eax*4]		// 0x000496a2
		fmul	dword ptr [esi]		// 0x000496a5
		fld	dword ptr [esi + 0x10]		// 0x000496a7
		fmul	dword ptr [eax + 4]		// 0x000496aa
		faddp	st(1), st		// 0x000496ad
		fld	dword ptr [eax + 8]		// 0x000496af
		fmul	dword ptr [esi + 0x20]		// 0x000496b2
		faddp	st(1), st		// 0x000496b5
		fadd	dword ptr [esi + 0x30]		// 0x000496b7
		fstp	dword ptr [ebp + 0x14]		// 0x000496ba
		mov	ebx, dword ptr [ebp + 0x14]		// 0x000496bd
		fld	dword ptr [esi + 0x14]		// 0x000496c0
		fmul	dword ptr [eax + 4]		// 0x000496c3
		fld	dword ptr [esi + 4]		// 0x000496c6
		fmul	dword ptr [eax]		// 0x000496c9
		faddp	st(1), st		// 0x000496cb
		fld	dword ptr [eax + 8]		// 0x000496cd
		fmul	dword ptr [esi + 0x24]		// 0x000496d0
		faddp	st(1), st		// 0x000496d3
		fadd	dword ptr [esi + 0x34]		// 0x000496d5
		fstp	dword ptr [ebp + 0x18]		// 0x000496d8
		fld	dword ptr [esi + 0x28]		// 0x000496db
		fmul	dword ptr [eax + 8]		// 0x000496de
		fld	dword ptr [esi + 8]		// 0x000496e1
		fmul	dword ptr [eax]		// 0x000496e4
		faddp	st(1), st		// 0x000496e6
		fld	dword ptr [esi + 0x18]		// 0x000496e8
		fmul	dword ptr [eax + 4]		// 0x000496eb
		lea	eax, [edx - 0x18]		// 0x000496ee
		faddp	st(1), st		// 0x000496f1
		fadd	dword ptr [esi + 0x38]		// 0x000496f3
		mov	dword ptr [eax], ebx		// 0x000496f6
		mov	ebx, dword ptr [ebp + 0x18]		// 0x000496f8
		mov	dword ptr [eax + 4], ebx		// 0x000496fb
		fstp	dword ptr [ebp + 0x1c]		// 0x000496fe
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x00049701
		mov	dword ptr [eax + 8], ebx		// 0x00049704
		mov	eax, dword ptr [ecx - 4]		// 0x00049707
		mov	ebx, dword ptr [ebp + 0x40]		// 0x0004970a
		lea	eax, [eax + eax*2]		// 0x0004970d
		fld	dword ptr [ebx + eax*4]		// 0x00049710
		lea	eax, [ebx + eax*4]		// 0x00049713
		fmul	dword ptr [esi]		// 0x00049716
		fld	dword ptr [esi + 0x10]		// 0x00049718
		fmul	dword ptr [eax + 4]		// 0x0004971b
		faddp	st(1), st		// 0x0004971e
		fld	dword ptr [eax + 8]		// 0x00049720
		fmul	dword ptr [esi + 0x20]		// 0x00049723
		faddp	st(1), st		// 0x00049726
		fadd	dword ptr [esi + 0x30]		// 0x00049728
		fstp	dword ptr [ebp + 0x14]		// 0x0004972b
		mov	ebx, dword ptr [ebp + 0x14]		// 0x0004972e
		fld	dword ptr [esi + 0x14]		// 0x00049731
		fmul	dword ptr [eax + 4]		// 0x00049734
		fld	dword ptr [esi + 4]		// 0x00049737
		fmul	dword ptr [eax]		// 0x0004973a
		faddp	st(1), st		// 0x0004973c
		fld	dword ptr [eax + 8]		// 0x0004973e
		fmul	dword ptr [esi + 0x24]		// 0x00049741
		faddp	st(1), st		// 0x00049744
		fadd	dword ptr [esi + 0x34]		// 0x00049746
		fstp	dword ptr [ebp + 0x18]		// 0x00049749
		fld	dword ptr [esi + 0x28]		// 0x0004974c
		fmul	dword ptr [eax + 8]		// 0x0004974f
		fld	dword ptr [esi + 8]		// 0x00049752
		fmul	dword ptr [eax]		// 0x00049755
		faddp	st(1), st		// 0x00049757
		fld	dword ptr [esi + 0x18]		// 0x00049759
		fmul	dword ptr [eax + 4]		// 0x0004975c
		lea	eax, [edx - 0xc]		// 0x0004975f
		faddp	st(1), st		// 0x00049762
		fadd	dword ptr [esi + 0x38]		// 0x00049764
		mov	dword ptr [eax], ebx		// 0x00049767
		mov	ebx, dword ptr [ebp + 0x18]		// 0x00049769
		mov	dword ptr [eax + 4], ebx		// 0x0004976c
		fstp	dword ptr [ebp + 0x1c]		// 0x0004976f
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x00049772
		mov	dword ptr [eax + 8], ebx		// 0x00049775
		mov	eax, dword ptr [ecx]		// 0x00049778
		mov	ebx, dword ptr [ebp + 0x40]		// 0x0004977a
		lea	eax, [eax + eax*2]		// 0x0004977d
		fld	dword ptr [ebx + eax*4]		// 0x00049780
		lea	eax, [ebx + eax*4]		// 0x00049783
		fmul	dword ptr [esi]		// 0x00049786
		fld	dword ptr [esi + 0x10]		// 0x00049788
		fmul	dword ptr [eax + 4]		// 0x0004978b
		faddp	st(1), st		// 0x0004978e
		fld	dword ptr [eax + 8]		// 0x00049790
		fmul	dword ptr [esi + 0x20]		// 0x00049793
		faddp	st(1), st		// 0x00049796
		fadd	dword ptr [esi + 0x30]		// 0x00049798
		fstp	dword ptr [ebp + 0x14]		// 0x0004979b
		mov	ebx, dword ptr [ebp + 0x14]		// 0x0004979e
		fld	dword ptr [esi + 0x14]		// 0x000497a1
		fmul	dword ptr [eax + 4]		// 0x000497a4
		fld	dword ptr [esi + 4]		// 0x000497a7
		fmul	dword ptr [eax]		// 0x000497aa
		faddp	st(1), st		// 0x000497ac
		fld	dword ptr [eax + 8]		// 0x000497ae
		fmul	dword ptr [esi + 0x24]		// 0x000497b1
		faddp	st(1), st		// 0x000497b4
		fadd	dword ptr [esi + 0x34]		// 0x000497b6
		fstp	dword ptr [ebp + 0x18]		// 0x000497b9
		fld	dword ptr [esi + 0x28]		// 0x000497bc
		fmul	dword ptr [eax + 8]		// 0x000497bf
		fld	dword ptr [esi + 8]		// 0x000497c2
		fmul	dword ptr [eax]		// 0x000497c5
		faddp	st(1), st		// 0x000497c7
		fld	dword ptr [esi + 0x18]		// 0x000497c9
		fmul	dword ptr [eax + 4]		// 0x000497cc
		mov	eax, edx		// 0x000497cf
		faddp	st(1), st		// 0x000497d1
		fadd	dword ptr [esi + 0x38]		// 0x000497d3
		mov	dword ptr [eax], ebx		// 0x000497d6
		mov	ebx, dword ptr [ebp + 0x18]		// 0x000497d8
		mov	dword ptr [eax + 4], ebx		// 0x000497db
		fstp	dword ptr [ebp + 0x1c]		// 0x000497de
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x000497e1
		mov	dword ptr [eax + 8], ebx		// 0x000497e4
		mov	eax, dword ptr [ecx + 4]		// 0x000497e7
		mov	ebx, dword ptr [ebp + 0x40]		// 0x000497ea
		lea	eax, [eax + eax*2]		// 0x000497ed
		fld	dword ptr [ebx + eax*4]		// 0x000497f0
		lea	eax, [ebx + eax*4]		// 0x000497f3
		fmul	dword ptr [esi]		// 0x000497f6
		fld	dword ptr [esi + 0x10]		// 0x000497f8
		fmul	dword ptr [eax + 4]		// 0x000497fb
		faddp	st(1), st		// 0x000497fe
		fld	dword ptr [eax + 8]		// 0x00049800
		fmul	dword ptr [esi + 0x20]		// 0x00049803
		faddp	st(1), st		// 0x00049806
		fadd	dword ptr [esi + 0x30]		// 0x00049808
		fstp	dword ptr [ebp + 0x14]		// 0x0004980b
		mov	ebx, dword ptr [ebp + 0x14]		// 0x0004980e
		fld	dword ptr [esi + 0x14]		// 0x00049811
		fmul	dword ptr [eax + 4]		// 0x00049814
		fld	dword ptr [esi + 4]		// 0x00049817
		fmul	dword ptr [eax]		// 0x0004981a
		faddp	st(1), st		// 0x0004981c
		fld	dword ptr [eax + 8]		// 0x0004981e
		fmul	dword ptr [esi + 0x24]		// 0x00049821
		faddp	st(1), st		// 0x00049824
		fadd	dword ptr [esi + 0x34]		// 0x00049826
		fstp	dword ptr [ebp + 0x18]		// 0x00049829
		fld	dword ptr [esi + 0x28]		// 0x0004982c
		fmul	dword ptr [eax + 8]		// 0x0004982f
		fld	dword ptr [esi + 8]		// 0x00049832
		fmul	dword ptr [eax]		// 0x00049835
		faddp	st(1), st		// 0x00049837
		fld	dword ptr [esi + 0x18]		// 0x00049839
		fmul	dword ptr [eax + 4]		// 0x0004983c
		lea	eax, [edx + 0xc]		// 0x0004983f
		faddp	st(1), st		// 0x00049842
		fadd	dword ptr [esi + 0x38]		// 0x00049844
		mov	dword ptr [eax], ebx		// 0x00049847
		mov	ebx, dword ptr [ebp + 0x18]		// 0x00049849
		mov	dword ptr [eax + 4], ebx		// 0x0004984c
		fstp	dword ptr [ebp + 0x1c]		// 0x0004984f
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x00049852
		mov	dword ptr [eax + 8], ebx		// 0x00049855
		mov	eax, dword ptr [ebp + 0x54]		// 0x00049858
		add	ecx, 0x10		// 0x0004985b
		add	edx, 0x30		// 0x0004985e
		dec	eax		// 0x00049861
		mov	dword ptr [ebp + 0x54], eax		// 0x00049862
		jne	L49696		// 0x00049865
		mov	ecx, dword ptr [ebp + 0x48]		// 0x0004986b
		mov	eax, dword ptr [ebp + 0x3c]		// 0x0004986e
		mov	ebx, dword ptr [ebp + 0x50]		// 0x00049871
L49874:
		cmp	ecx, eax		// 0x00049874
		jae	L49903		// 0x00049876
		lea	edx, [ecx + ecx*2]		// 0x0004987c
		lea	edx, [ebx + edx*4]		// 0x0004987f
L49882:
		mov	eax, dword ptr [ebp + 0x44]		// 0x00049882
		mov	eax, dword ptr [eax + ecx*4]		// 0x00049885
		mov	ebx, dword ptr [ebp + 0x40]		// 0x00049888
		lea	eax, [eax + eax*2]		// 0x0004988b
		fld	dword ptr [ebx + eax*4]		// 0x0004988e
		lea	eax, [ebx + eax*4]		// 0x00049891
		fmul	dword ptr [esi]		// 0x00049894
		inc	ecx		// 0x00049896
		fld	dword ptr [esi + 0x10]		// 0x00049897
		fmul	dword ptr [eax + 4]		// 0x0004989a
		faddp	st(1), st		// 0x0004989d
		fld	dword ptr [eax + 8]		// 0x0004989f
		fmul	dword ptr [esi + 0x20]		// 0x000498a2
		faddp	st(1), st		// 0x000498a5
		fadd	dword ptr [esi + 0x30]		// 0x000498a7
		fstp	dword ptr [ebp + 0x14]		// 0x000498aa
		mov	ebx, dword ptr [ebp + 0x14]		// 0x000498ad
		fld	dword ptr [esi + 0x14]		// 0x000498b0
		fmul	dword ptr [eax + 4]		// 0x000498b3
		fld	dword ptr [esi + 4]		// 0x000498b6
		fmul	dword ptr [eax]		// 0x000498b9
		faddp	st(1), st		// 0x000498bb
		fld	dword ptr [eax + 8]		// 0x000498bd
		fmul	dword ptr [esi + 0x24]		// 0x000498c0
		faddp	st(1), st		// 0x000498c3
		fadd	dword ptr [esi + 0x34]		// 0x000498c5
		fstp	dword ptr [ebp + 0x18]		// 0x000498c8
		fld	dword ptr [esi + 0x28]		// 0x000498cb
		fmul	dword ptr [eax + 8]		// 0x000498ce
		fld	dword ptr [esi + 8]		// 0x000498d1
		fmul	dword ptr [eax]		// 0x000498d4
		faddp	st(1), st		// 0x000498d6
		fld	dword ptr [esi + 0x18]		// 0x000498d8
		fmul	dword ptr [eax + 4]		// 0x000498db
		mov	eax, edx		// 0x000498de
		add	edx, 0xc		// 0x000498e0
		faddp	st(1), st		// 0x000498e3
		fadd	dword ptr [esi + 0x38]		// 0x000498e5
		mov	dword ptr [eax], ebx		// 0x000498e8
		mov	ebx, dword ptr [ebp + 0x18]		// 0x000498ea
		mov	dword ptr [eax + 4], ebx		// 0x000498ed
		fstp	dword ptr [ebp + 0x1c]		// 0x000498f0
		mov	ebx, dword ptr [ebp + 0x1c]		// 0x000498f3
		mov	dword ptr [eax + 8], ebx		// 0x000498f6
		mov	eax, dword ptr [ebp + 0x3c]		// 0x000498f9
		cmp	ecx, eax		// 0x000498fc
		jb	L49882		// 0x000498fe
		mov	ebx, dword ptr [ebp + 0x50]		// 0x00049900
L49903:
		test	eax, eax		// 0x00049903
		jbe	L49c88		// 0x00049905
		mov	edx, 1		// 0x0004990b
		lea	esi, [ebx + 4]		// 0x00049910
		mov	dword ptr [ebp + 0x58], edx		// 0x00049913
		mov	dword ptr [ebp + 0x54], esi		// 0x00049916
		mov	dword ptr [ebp - 0x14], eax		// 0x00049919
		jmp	L49a71		// 0x0004991c
L49921:
		fstp	st(0)		// 0x00049921
L49923:
		fld	dword ptr [ebp + 0x1c]		// 0x00049923
		fmul	dword ptr [ecx + 8]		// 0x00049926
		fld	dword ptr [ebp + 0x14]		// 0x00049929
		fmul	dword ptr [ecx]		// 0x0004992c
		faddp	st(1), st		// 0x0004992e
		fld	dword ptr [ebp + 0x18]		// 0x00049930
		fmul	dword ptr [ecx + 4]		// 0x00049933
		faddp	st(1), st		// 0x00049936
		fld	dword ptr [ecx + 0xc]		// 0x00049938
		fchs		// 0x0004993b
		fsubp	st(1), st		// 0x0004993d
		fdiv	st, st(1)		// 0x0004993f
		fstp	dword ptr [ebp + 0x50]		// 0x00049941
		fstp	st(0)		// 0x00049944
		fld	dword ptr [ebp + 0x50]		// 0x00049946
		fcomp	dword ptr kContactZero		// 0x00049949
		fnstsw	ax		// 0x0004994f
		test	ah, 5		// 0x00049951
		jp	L491df		// 0x00049954
		fld	dword ptr [ebp - 0x20]		// 0x0004995a
		mov	eax, dword ptr [ebp + 0x3c]		// 0x0004995d
		fmul	dword ptr [ebp + 0x50]		// 0x00049960
		fld	dword ptr [ebp - 0x1c]		// 0x00049963
		fmul	dword ptr [ebp + 0x50]		// 0x00049966
		fld	dword ptr [ebp - 0x18]		// 0x00049969
		fmul	dword ptr [ebp + 0x50]		// 0x0004996c
		fstp	dword ptr [ebp - 0x24]		// 0x0004996f
		fld	dword ptr [ebp + 0x14]		// 0x00049972
		fsub	st, st(2)		// 0x00049975
		fstp	dword ptr [ebp - 0xac]		// 0x00049977
		fld	dword ptr [ebp + 0x18]		// 0x0004997d
		fsub	st, st(1)		// 0x00049980
		fstp	st(2)		// 0x00049982
		fstp	st(0)		// 0x00049984
		fld	dword ptr [ebp + 0x1c]		// 0x00049986
		fsub	dword ptr [ebp - 0x24]		// 0x00049989
		fld	st(1)		// 0x0004998c
		fmul	dword ptr [ebp - 0xc]		// 0x0004998e
		fld	st(1)		// 0x00049991
		fmul	dword ptr [ebp - 8]		// 0x00049993
		faddp	st(1), st		// 0x00049996
		fld	dword ptr [ebp - 0xac]		// 0x00049998
		fmul	dword ptr [ebp - 0x10]		// 0x0004999e
		faddp	st(1), st		// 0x000499a1
		fstp	dword ptr [ebp - 0x48]		// 0x000499a3
		mov	edx, dword ptr [ebp - 0x48]		// 0x000499a6
		fxch	st(1)		// 0x000499a9
		fmul	dword ptr [ebp]		// 0x000499ab
		fxch	st(1)		// 0x000499ae
		fmul	dword ptr [ebp + 4]		// 0x000499b0
		faddp	st(1), st		// 0x000499b3
		fld	dword ptr [ebp - 4]		// 0x000499b5
		fmul	dword ptr [ebp - 0xac]		// 0x000499b8
		faddp	st(1), st		// 0x000499be
		fstp	dword ptr [ebp - 0x44]		// 0x000499c0
		mov	ecx, dword ptr [ebp - 0x44]		// 0x000499c3
		push	ecx		// 0x000499c6
		mov	ecx, dword ptr [ebp - 0x14]		// 0x000499c7
		push	edx		// 0x000499ca
		call	nxPolygonContainsPoint		// 0x000499cb
		add	esp, 8		// 0x000499d0
		test	eax, eax		// 0x000499d3
		je	L491df		// 0x000499d5
		mov	eax, dword ptr [ebp + 0x48]		// 0x000499db
		fld	dword ptr [edi + 0x20]		// 0x000499de
		fmul	dword ptr [eax + 8]		// 0x000499e1
		mov	ecx, dword ptr [ebp + 0x78]		// 0x000499e4
		fld	dword ptr [edi + 0x10]		// 0x000499e7
		mov	edx, dword ptr [ebp + 0x74]		// 0x000499ea
		fmul	dword ptr [eax + 4]		// 0x000499ed
		faddp	st(1), st		// 0x000499f0
		fld	dword ptr [eax]		// 0x000499f2
		fmul	dword ptr [edi]		// 0x000499f4
		faddp	st(1), st		// 0x000499f6
		fadd	dword ptr [edi + 0x30]		// 0x000499f8
		fstp	dword ptr [ebp - 0x54]		// 0x000499fb
		fld	dword ptr [edi + 0x24]		// 0x000499fe
		fmul	dword ptr [eax + 8]		// 0x00049a01
		fld	dword ptr [edi + 0x14]		// 0x00049a04
		fmul	dword ptr [eax + 4]		// 0x00049a07
		faddp	st(1), st		// 0x00049a0a
		fld	dword ptr [edi + 4]		// 0x00049a0c
		fmul	dword ptr [eax]		// 0x00049a0f
		faddp	st(1), st		// 0x00049a11
		fadd	dword ptr [edi + 0x34]		// 0x00049a13
		fstp	dword ptr [ebp - 0x50]		// 0x00049a16
		fld	dword ptr [edi + 0x28]		// 0x00049a19
		fmul	dword ptr [eax + 8]		// 0x00049a1c
		fld	dword ptr [edi + 0x18]		// 0x00049a1f
		fmul	dword ptr [eax + 4]		// 0x00049a22
		faddp	st(1), st		// 0x00049a25
		fld	dword ptr [edi + 8]		// 0x00049a27
		fmul	dword ptr [eax]		// 0x00049a2a
		mov	eax, dword ptr [ebp + 0x7c]		// 0x00049a2c
		push	eax		// 0x00049a2f
		mov	eax, dword ptr [ebp + 0x70]		// 0x00049a30
		push	ecx		// 0x00049a33
		faddp	st(1), st		// 0x00049a34
		push	edx		// 0x00049a36
		push	eax		// 0x00049a37
		mov	eax, dword ptr [ebp + 0x50]		// 0x00049a38
		fadd	dword ptr [edi + 0x38]		// 0x00049a3b
		lea	ecx, [ebp - 0x3c]		// 0x00049a3e
		push	ecx		// 0x00049a41
		mov	ecx, dword ptr [ebp + 0x60]		// 0x00049a42
		fstp	dword ptr [ebp - 0x4c]		// 0x00049a45
		lea	edx, [ebp - 0x54]		// 0x00049a48
		push	edx		// 0x00049a4b
		mov	edx, dword ptr [ecx + 0x9c]		// 0x00049a4c
		push	eax		// 0x00049a52
		mov	eax, dword ptr [ebp + 0x5c]		// 0x00049a53
		mov	ecx, dword ptr [eax + 0x9c]		// 0x00049a56
		push	edx		// 0x00049a5c
		push	ecx		// 0x00049a5d
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00049a5e
		call	NxEmitContactFeatures		// 0x00049a61
		jmp	L491df		// 0x00049a66
L49a6b:
		mov	eax, dword ptr [ebp + 0x3c]		// 0x00049a6b
		mov	ebx, dword ptr [ebp + 0x50]		// 0x00049a6e
L49a71:
		cmp	edx, eax		// 0x00049a71
		mov	ecx, edx		// 0x00049a73
		jb	L49a79		// 0x00049a75
		xor	ecx, ecx		// 0x00049a77
L49a79:
		lea	ecx, [ecx + ecx*2]		// 0x00049a79
		fld	dword ptr [ebx + ecx*4]		// 0x00049a7c
		lea	ebx, [ebx + ecx*4]		// 0x00049a7f
		fsub	dword ptr [esi - 4]		// 0x00049a82
		mov	ecx, dword ptr [ebp + 0x38]		// 0x00049a85
		mov	dword ptr [ebp - 0x58], ebx		// 0x00049a88
		fstp	dword ptr [ebp - 0x20]		// 0x00049a8b
		fld	dword ptr [ebx + 4]		// 0x00049a8e
		fsub	dword ptr [esi]		// 0x00049a91
		fstp	dword ptr [ebp - 0x1c]		// 0x00049a93
		fld	dword ptr [ebx + 8]		// 0x00049a96
		fsub	dword ptr [esi + 4]		// 0x00049a99
		fstp	dword ptr [ebp - 0x18]		// 0x00049a9c
		fld	dword ptr [ebp - 0x1c]		// 0x00049a9f
		fmul	dword ptr [ecx + 8]		// 0x00049aa2
		fld	dword ptr [ebp - 0x18]		// 0x00049aa5
		fmul	dword ptr [ecx + 4]		// 0x00049aa8
		fsubp	st(1), st		// 0x00049aab
		fstp	dword ptr [ebp - 0x48]		// 0x00049aad
		mov	eax, dword ptr [ebp - 0x48]		// 0x00049ab0
		fld	dword ptr [ebp - 0x18]		// 0x00049ab3
		mov	dword ptr [ebp - 0x30], eax		// 0x00049ab6
		fmul	dword ptr [ecx]		// 0x00049ab9
		fld	dword ptr [ebp - 0x20]		// 0x00049abb
		fmul	dword ptr [ecx + 8]		// 0x00049abe
		fsubp	st(1), st		// 0x00049ac1
		fstp	dword ptr [ebp - 0x44]		// 0x00049ac3
		mov	eax, dword ptr [ebp - 0x44]		// 0x00049ac6
		fld	dword ptr [ebp - 0x20]		// 0x00049ac9
		mov	dword ptr [ebp - 0x2c], eax		// 0x00049acc
		fmul	dword ptr [ecx + 4]		// 0x00049acf
		fld	dword ptr [ebp - 0x1c]		// 0x00049ad2
		fmul	dword ptr [ecx]		// 0x00049ad5
		fsubp	st(1), st		// 0x00049ad7
		fstp	dword ptr [ebp - 0x40]		// 0x00049ad9
		mov	eax, dword ptr [ebp - 0x40]		// 0x00049adc
		mov	dword ptr [ebp - 0x28], eax		// 0x00049adf
		fld	dword ptr [ebp - 0x28]		// 0x00049ae2
		fmul	dword ptr [ebp - 0x28]		// 0x00049ae5
		fld	dword ptr [ebp - 0x2c]		// 0x00049ae8
		fmul	dword ptr [ebp - 0x2c]		// 0x00049aeb
		faddp	st(1), st		// 0x00049aee
		fld	dword ptr [ebp - 0x30]		// 0x00049af0
		fmul	dword ptr [ebp - 0x30]		// 0x00049af3
		faddp	st(1), st		// 0x00049af6
		fld	dword ptr kContactZero		// 0x00049af8
		fld	st(1)		// 0x00049afe
		fucompp		// 0x00049b00
		fnstsw	ax		// 0x00049b02
		test	ah, 0x44		// 0x00049b04
		jnp	L49b29		// 0x00049b07
		fsqrt		// 0x00049b09
		fdivr	dword ptr kContactOne		// 0x00049b0b
		fld	dword ptr [ebp - 0x30]		// 0x00049b11
		fmul	st, st(1)		// 0x00049b14
		fstp	dword ptr [ebp - 0x30]		// 0x00049b16
		fld	dword ptr [ebp - 0x2c]		// 0x00049b19
		fmul	st, st(1)		// 0x00049b1c
		fstp	dword ptr [ebp - 0x2c]		// 0x00049b1e
		fld	dword ptr [ebp - 0x28]		// 0x00049b21
		fmul	st, st(1)		// 0x00049b24
		fstp	dword ptr [ebp - 0x28]		// 0x00049b26
L49b29:
		mov	eax, dword ptr [ebp + 0x28]		// 0x00049b29
		fstp	st(0)		// 0x00049b2c
		test	eax, eax		// 0x00049b2e
		fld	dword ptr [ebp - 0x28]		// 0x00049b30
		fmul	dword ptr [esi + 4]		// 0x00049b33
		fld	dword ptr [ebp - 0x30]		// 0x00049b36
		fmul	dword ptr [esi - 4]		// 0x00049b39
		faddp	st(1), st		// 0x00049b3c
		fld	dword ptr [ebp - 0x2c]		// 0x00049b3e
		fmul	dword ptr [esi]		// 0x00049b41
		faddp	st(1), st		// 0x00049b43
		fchs		// 0x00049b45
		fstp	dword ptr [ebp - 0x24]		// 0x00049b47
		jbe	L49c71		// 0x00049b4a
		mov	edx, dword ptr [ebp + 0x30]		// 0x00049b50
		mov	dword ptr [ebp + 0x44], edx		// 0x00049b53
		mov	edx, dword ptr [ebp + 0x28]		// 0x00049b56
		mov	eax, 1		// 0x00049b59
		mov	dword ptr [ebp + 0x40], eax		// 0x00049b5e
		mov	dword ptr [ebp + 0x48], edx		// 0x00049b61
		jmp	L49b70		// 0x00049b64
L49b66:
		mov	eax, dword ptr [ebp + 0x40]		// 0x00049b66
		mov	ebx, dword ptr [ebp - 0x58]		// 0x00049b69
		mov	ecx, dword ptr [ebp + 0x38]		// 0x00049b6c
		_emit	0x90		// 0x00049b6f nop 
L49b70:
		cmp	eax, dword ptr [ebp + 0x28]		// 0x00049b70
		mov	dword ptr [ebp - 0x5c], eax		// 0x00049b73
		jb	L49b7c		// 0x00049b76
		xor	eax, eax		// 0x00049b78
		jmp	L49b7f		// 0x00049b7a
L49b7c:
		mov	eax, dword ptr [ebp - 0x5c]		// 0x00049b7c
L49b7f:
		lea	edx, [ebp - 0x60]		// 0x00049b7f
		push	edx		// 0x00049b82
		mov	edx, dword ptr [ebp + 0x30]		// 0x00049b83
		mov	eax, dword ptr [edx + eax*4]		// 0x00049b86
		mov	edx, dword ptr [ebp + 0x2c]		// 0x00049b89
		lea	eax, [eax + eax*2]		// 0x00049b8c
		lea	eax, [edx + eax*4]		// 0x00049b8f
		push	eax		// 0x00049b92
		mov	eax, dword ptr [ebp + 0x44]		// 0x00049b93
		mov	eax, dword ptr [eax]		// 0x00049b96
		lea	eax, [eax + eax*2]		// 0x00049b98
		push	ecx		// 0x00049b9b
		lea	edx, [edx + eax*4]		// 0x00049b9c
		lea	eax, [esi - 4]		// 0x00049b9f
		push	ebx		// 0x00049ba2
		push	eax		// 0x00049ba3
		lea	esi, [ebp + 0x14]		// 0x00049ba4
		lea	ecx, [ebp - 0x30]		// 0x00049ba7
		lea	ebx, [ebp - 0x20]		// 0x00049baa
		call	nxClipEdgeToPolygonPlane		// 0x00049bad
		add	esp, 0x14		// 0x00049bb2
		test	eax, eax		// 0x00049bb5
		je	L49c4e		// 0x00049bb7
		fld	dword ptr [ebp + 0x18]		// 0x00049bbd
		mov	ecx, dword ptr [ebp + 0x7c]		// 0x00049bc0
		fmul	dword ptr [edi + 0x14]		// 0x00049bc3
		mov	edx, dword ptr [ebp + 0x78]		// 0x00049bc6
		fld	dword ptr [ebp + 0x1c]		// 0x00049bc9
		mov	eax, dword ptr [ebp + 0x74]		// 0x00049bcc
		fmul	dword ptr [edi + 0x24]		// 0x00049bcf
		push	ecx		// 0x00049bd2
		mov	ecx, dword ptr [ebp + 0x70]		// 0x00049bd3
		push	edx		// 0x00049bd6
		faddp	st(1), st		// 0x00049bd7
		push	eax		// 0x00049bd9
		fld	dword ptr [ebp + 0x14]		// 0x00049bda
		push	ecx		// 0x00049bdd
		fmul	dword ptr [edi + 4]		// 0x00049bde
		lea	edx, [ebp - 0x3c]		// 0x00049be1
		push	edx		// 0x00049be4
		mov	eax, esi		// 0x00049be5
		faddp	st(1), st		// 0x00049be7
		push	eax		// 0x00049be9
		mov	eax, dword ptr [ebp + 0x5c]		// 0x00049bea
		push	ecx		// 0x00049bed
		fadd	dword ptr [edi + 0x34]		// 0x00049bee
		mov	ecx, dword ptr [ebp + 0x60]		// 0x00049bf1
		fld	dword ptr [ebp + 0x18]		// 0x00049bf4
		mov	edx, dword ptr [ecx + 0x9c]		// 0x00049bf7
		fmul	dword ptr [edi + 0x18]		// 0x00049bfd
		mov	ecx, dword ptr [eax + 0x9c]		// 0x00049c00
		fld	dword ptr [ebp + 0x1c]		// 0x00049c06
		fmul	dword ptr [edi + 0x28]		// 0x00049c09
		faddp	st(1), st		// 0x00049c0c
		fld	dword ptr [ebp + 0x14]		// 0x00049c0e
		fmul	dword ptr [edi + 8]		// 0x00049c11
		faddp	st(1), st		// 0x00049c14
		fadd	dword ptr [edi + 0x38]		// 0x00049c16
		fld	dword ptr [ebp + 0x18]		// 0x00049c19
		fmul	dword ptr [edi + 0x10]		// 0x00049c1c
		fld	dword ptr [ebp + 0x1c]		// 0x00049c1f
		fmul	dword ptr [edi + 0x20]		// 0x00049c22
		faddp	st(1), st		// 0x00049c25
		fld	dword ptr [ebp + 0x14]		// 0x00049c27
		fmul	dword ptr [edi]		// 0x00049c2a
		faddp	st(1), st		// 0x00049c2c
		fadd	dword ptr [edi + 0x30]		// 0x00049c2e
		fstp	dword ptr [ebp + 0x14]		// 0x00049c31
		fxch	st(1)		// 0x00049c34
		fstp	dword ptr [ebp + 0x18]		// 0x00049c36
		fstp	dword ptr [ebp + 0x1c]		// 0x00049c39
		fld	dword ptr [ebp - 0x60]		// 0x00049c3c
		fchs		// 0x00049c3f
		fstp	dword ptr [esp]		// 0x00049c41
		push	edx		// 0x00049c44
		push	ecx		// 0x00049c45
		mov	ecx, dword ptr [ebp + 0x64]		// 0x00049c46
		call	NxEmitContactFeatures		// 0x00049c49
L49c4e:
		mov	edx, dword ptr [ebp + 0x40]		// 0x00049c4e
		mov	ecx, dword ptr [ebp + 0x44]		// 0x00049c51
		mov	eax, dword ptr [ebp + 0x48]		// 0x00049c54
		mov	esi, dword ptr [ebp + 0x54]		// 0x00049c57
		inc	edx		// 0x00049c5a
		add	ecx, 4		// 0x00049c5b
		dec	eax		// 0x00049c5e
		mov	dword ptr [ebp + 0x40], edx		// 0x00049c5f
		mov	dword ptr [ebp + 0x44], ecx		// 0x00049c62
		mov	dword ptr [ebp + 0x48], eax		// 0x00049c65
		jne	L49b66		// 0x00049c68
		mov	edx, dword ptr [ebp + 0x58]		// 0x00049c6e
L49c71:
		mov	eax, dword ptr [ebp - 0x14]		// 0x00049c71
		inc	edx		// 0x00049c74
		add	esi, 0xc		// 0x00049c75
		dec	eax		// 0x00049c78
		mov	dword ptr [ebp + 0x58], edx		// 0x00049c79
		mov	dword ptr [ebp + 0x54], esi		// 0x00049c7c
		mov	dword ptr [ebp - 0x14], eax		// 0x00049c7f
		jne	L49a6b		// 0x00049c82
L49c88:
		lea	esp, [ebp - 0xb8]		// 0x00049c88
		pop	edi		// 0x00049c8e
		pop	esi		// 0x00049c8f
		pop	ebx		// 0x00049c90
		add	ebp, 0x20		// 0x00049c91
		mov	esp, ebp		// 0x00049c94
		pop	ebp		// 0x00049c96
		ret		// 0x00049c97
		}
	}

#else
#include "portable/ContactPolygonScalar.inl"
#endif
