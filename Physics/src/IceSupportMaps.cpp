/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The support-vertex maps, sub-unit B of units/convex-mesh-gap-contract.md
// (0x0002e160..0x0002eb41; the file name is the contract's choice: the tables
// sit after IceAdjacencies' strings with no .rdata block of their own, so the
// file includes no Foundation header, and which ICE file it was is not
// established). Convex-mesh gap Task 2f writes the eight rows that were not
// started (001550, 001556, 001558 with its continuation 001560, 001567, 001569,
// 001573, 001579, 001581) from the Capstone listing, and the product forms of
// the small rows the census had closed as ObjectModel.cpp models (001552,
// 001554, 001563, 001565, 001571, 001575, 001577, 001583, 001585, 001587,
// 001589), which the tables and constructors need.
//
// The float rows (001550, 001556, 001558/001560, 001573, 001581) are the
// listing's instructions, naked (the precedent of 001712 and 001651), because
// their values live on the x87 stack across stores that narrow and across the
// integer conversions (`fild` with the 2^32 fix-up, `fistp qword`). Branch
// targets are labels named by their oracle RVA. 001558's switch on the cube
// face jumps through a table (0x1002e550 in the image, just after the row);
// MSVC's inline assembler cannot emit a table of code addresses, so the table
// is gIceSupportMapFaceCases in .rdata, whose entries are the row's address
// plus the offsets of its three case labels in the assembled row (measured on
// the build and checked by evidence/convex-mesh-gap-2f-listing-compare.py,
// which compares each entry by the instruction it lands on). The integer rows
// are C++. Every allocation goes through the 004803 getter (slot 0 type 0,
// slot 3 free), as the listings' do.
//
// x87: on the /arch:IA32 list. Built /EHs-c- with the other ICE-shaped files.

#include "IceSupportMaps.h"

#include <stdlib.h>

extern "C" int __cdecl _purecall(void);

// The constants the float rows read: 0.0f (0x101041f0), 1.0f (0x101041ec), 0.5f
// (0x101043cc), 2^32 (0x101066f8, the unsigned fix-up of `fild`), FLT_MAX
// (0x10106858) and the doubles 1e-7 and -1e-7 (0x10107880, 0x10107888).
static const float kIceSupportMapZero = 0.0f;
static const float kIceSupportMapOne = 1.0f;
static const float kIceSupportMapHalf = 0.5f;
static const float kIceSupportMapTwo32 = 4294967296.0f;
static const float kIceSupportMapMax = 3.402823466e+38f;
static const double kIceSupportMapEpsilon = 1e-7;
static const double kIceSupportMapMinusEpsilon = -1e-7;

// The tables.
const void* const gIceSupportMapBaseTable[4] =
	{ (const void*) &nxSupportMapBaseDelete, (const void*) &_purecall, (const void*) &_purecall,
	(const void*) &nxSupportMapNoop };
const void* const gIceSupportMapHullTable[4] =
	{ (const void*) &nxSupportMapHullDelete, (const void*) &nxSupportMapHullAllocate,
	(const void*) &nxSupportMapHullCompute, (const void*) &nxSupportMapNoop };
const void* const gIceSupportMapPlaneTable[4] =
	{ (const void*) &nxSupportMapPlaneDelete, (const void*) &nxSupportMapHullAllocate,
	(const void*) &nxSupportMapPlaneCompute, (const void*) &nxSupportMapNoop };
const void* const gIceSupportMapVertexTable[4] =
	{ (const void*) &nxSupportMapVertexDelete, (const void*) &nxSupportMapVertexAllocate,
	(const void*) &nxSupportMapVertexCompute, (const void*) &nxSupportMapNoop };

// 001558's switch table (0x1002e550): faces 0 and 1 to 0x0002e380, 2 and 3 to
// 0x0002e3df, 4 and 5 to 0x0002e43c, as offsets into the assembled row.
static const NxU32 kIceSupportMapCaseX = 0x90;	// L2e380
static const NxU32 kIceSupportMapCaseY = 0xef;	// L2e3df
static const NxU32 kIceSupportMapCaseZ = 0x14c;	// L2e43c
extern "C" const void* const gIceSupportMapFaceCases[6] =
	{
	(const char*) &nxSupportMapInit + kIceSupportMapCaseX, (const char*) &nxSupportMapInit + kIceSupportMapCaseX,
	(const char*) &nxSupportMapInit + kIceSupportMapCaseY, (const char*) &nxSupportMapInit + kIceSupportMapCaseY,
	(const char*) &nxSupportMapInit + kIceSupportMapCaseZ, (const char*) &nxSupportMapInit + kIceSupportMapCaseZ,
	};

static inline void nxSupportMapFree(void* memory)
	{
	nxGetSdkAllocator()->free(memory);
	}

// phys_fn_001550 (0x0002e160, 131 B)
// The cube face of a direction (cdecl: direction, u, v; returns the face). The
// dominant axis is chosen on the magnitudes' bits (`and 0x7fffffff`, unsigned
// compares): y over x only when strictly larger, then z over that only when
// strictly larger. 1 / |dominant| (fabs of the loaded word; a load that quiets
// a signalling NaN) scales the next two components in the order the byte table
// 0x01000201 >> (axis * 8) gives -- (y, z) for x, (z, x) for y, (x, y) for z --
// into u and v. The face is axis * 2 + the dominant component's sign bit. The
// listing's instructions, naked.
__declspec(naked) NxU32 nxSupportMapCubeFace(const IceMaths::Point* /*dir*/, float* /*u*/, float* /*v*/)
	{
	__asm
		{
		mov	eax, dword ptr [esp + 4]		// 0x0002e160
		mov	ecx, dword ptr [eax + 4]		// 0x0002e164
		mov	edx, dword ptr [eax]		// 0x0002e167
		push	esi		// 0x0002e169
		and	ecx, 0x7fffffff		// 0x0002e16a
		and	edx, 0x7fffffff		// 0x0002e170
		xor	esi, esi		// 0x0002e176
		cmp	ecx, edx		// 0x0002e178
		push	edi		// 0x0002e17a
		jbe	L2e182		// 0x0002e17b
		mov	esi, 1		// 0x0002e17d
L2e182:
		mov	ecx, dword ptr [eax + esi*4]		// 0x0002e182
		mov	edx, dword ptr [eax + 8]		// 0x0002e185
		and	ecx, 0x7fffffff		// 0x0002e188
		and	edx, 0x7fffffff		// 0x0002e18e
		cmp	edx, ecx		// 0x0002e194
		jbe	L2e19d		// 0x0002e196
		mov	esi, 2		// 0x0002e198
L2e19d:
		fld	dword ptr [eax + esi*4]		// 0x0002e19d
		mov	edi, dword ptr [eax + esi*4]		// 0x0002e1a0
		fabs		// 0x0002e1a3
		lea	ecx, [esi*8]		// 0x0002e1a5
		fdivr	dword ptr kIceSupportMapOne		// 0x0002e1ac
		mov	edx, 0x1000201		// 0x0002e1b2
		shr	edx, cl		// 0x0002e1b7
		shr	edi, 0x1f		// 0x0002e1b9
		mov	ecx, edx		// 0x0002e1bc
		and	ecx, 0xff		// 0x0002e1be
		movzx	edx, dh		// 0x0002e1c4
		fld	st(0)		// 0x0002e1c7
		fmul	dword ptr [eax + ecx*4]		// 0x0002e1c9
		mov	ecx, dword ptr [esp + 0x10]		// 0x0002e1cc
		fstp	dword ptr [ecx]		// 0x0002e1d0
		fmul	dword ptr [eax + edx*4]		// 0x0002e1d2
		mov	eax, dword ptr [esp + 0x14]		// 0x0002e1d5
		fstp	dword ptr [eax]		// 0x0002e1d9
		lea	eax, [esi + esi]		// 0x0002e1db
		or	eax, edi		// 0x0002e1de
		pop	edi		// 0x0002e1e0
		pop	esi		// 0x0002e1e1
		ret		// 0x0002e1e2
		}
	}

// phys_fn_001552 (0x0002e1f0, 17 B)
// The base constructor: the base table, n and the sample count zeroed. (The
// census closed the row as an ObjectModel.cpp model; this is its product form.)
__declspec(noinline) void* __fastcall nxSupportMapBaseConstruct(IceSupportMap* map)
	{
	map->mVtable = gIceSupportMapBaseTable;
	map->mSubdiv = 0;
	map->mNbSamples = 0;
	return map;
	}

// phys_fn_001554 (0x0002e210, 7 B)
// The base table stored (the destructors' last step). (Product form of the
// ObjectModel.cpp model.)
__declspec(noinline) void __fastcall nxSupportMapBaseTable(IceSupportMap* map)
	{
	map->mVtable = gIceSupportMapBaseTable;
	}

// phys_fn_001556 (0x0002e220, 196 B)
// The sample of a direction (thiscall, `ret 4`): the cube face and the two
// in-face coordinates (001550; u is written over the argument's own slot),
// h = (n - 1) * 0.5 (the unsigned `fild` fix-up), each coordinate mapped to
// (c + 1) * h and rounded by `fistp qword` under the caller's rounding mode;
// then, per coordinate, when (c + 1) * h - rounded > 0.5 the index is
// incremented (a strict test, NaN never). u's scaled value is stored narrow
// before it is rounded and v's stays wide on the stack; u's fix-up is tested
// first. The sample is (face * n + u) * n + v. The listing's instructions,
// naked.
__declspec(naked) NxU32 __fastcall nxSupportMapLookup(const IceSupportMap* /*map*/, NxU32 /*edx*/,
	const IceMaths::Point* /*dir*/)
	{
	__asm
		{
		sub	esp, 0xc		// 0x0002e220
		mov	edx, dword ptr [esp + 0x10]		// 0x0002e223
		push	esi		// 0x0002e227
		mov	esi, dword ptr [ecx + 4]		// 0x0002e228
		push	edi		// 0x0002e22b
		lea	eax, [esp + 8]		// 0x0002e22c
		push	eax		// 0x0002e230
		lea	ecx, [esp + 0x1c]		// 0x0002e231
		push	ecx		// 0x0002e235
		push	edx		// 0x0002e236
		call	nxSupportMapCubeFace		// 0x0002e237
		mov	edi, eax		// 0x0002e23c
		lea	eax, [esi - 1]		// 0x0002e23e
		add	esp, 0xc		// 0x0002e241
		test	eax, eax		// 0x0002e244
		mov	dword ptr [esp + 0xc], eax		// 0x0002e246
		fild	dword ptr [esp + 0xc]		// 0x0002e24a
		jge	L2e256		// 0x0002e24e
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e250
L2e256:
		fmul	dword ptr kIceSupportMapHalf		// 0x0002e256
		fld	dword ptr [esp + 0x18]		// 0x0002e25c
		fadd	dword ptr kIceSupportMapOne		// 0x0002e260
		fmul	st, st(1)		// 0x0002e266
		fstp	dword ptr [esp + 0x18]		// 0x0002e268
		fld	dword ptr [esp + 8]		// 0x0002e26c
		fadd	dword ptr kIceSupportMapOne		// 0x0002e270
		fmulp	st(1), st		// 0x0002e276
		fld	dword ptr [esp + 0x18]		// 0x0002e278
		fistp	qword ptr [esp + 0xc]		// 0x0002e27c
		mov	edx, dword ptr [esp + 0xc]		// 0x0002e280
		fld	st(0)		// 0x0002e284
		fistp	qword ptr [esp + 0xc]		// 0x0002e286
		mov	ecx, dword ptr [esp + 0xc]		// 0x0002e28a
		test	ecx, ecx		// 0x0002e28e
		fild	dword ptr [esp + 0xc]		// 0x0002e290
		jge	L2e29c		// 0x0002e294
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e296
L2e29c:
		test	edx, edx		// 0x0002e29c
		fsubp	st(1), st		// 0x0002e29e
		mov	dword ptr [esp + 0xc], edx		// 0x0002e2a0
		fild	dword ptr [esp + 0xc]		// 0x0002e2a4
		jge	L2e2b0		// 0x0002e2a8
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e2aa
L2e2b0:
		fsubr	dword ptr [esp + 0x18]		// 0x0002e2b0
		fcomp	dword ptr kIceSupportMapHalf		// 0x0002e2b4
		fnstsw	ax		// 0x0002e2ba
		test	ah, 0x41		// 0x0002e2bc
		jne	L2e2c2		// 0x0002e2bf
		inc	edx		// 0x0002e2c1
L2e2c2:
		fcomp	dword ptr kIceSupportMapHalf		// 0x0002e2c2
		fnstsw	ax		// 0x0002e2c8
		test	ah, 0x41		// 0x0002e2ca
		jne	L2e2d0		// 0x0002e2cd
		inc	ecx		// 0x0002e2cf
L2e2d0:
		mov	eax, edi		// 0x0002e2d0
		imul	eax, esi		// 0x0002e2d2
		add	eax, edx		// 0x0002e2d5
		imul	eax, esi		// 0x0002e2d7
		pop	edi		// 0x0002e2da
		add	eax, ecx		// 0x0002e2db
		pop	esi		// 0x0002e2dd
		add	esp, 0xc		// 0x0002e2de
		ret	4		// 0x0002e2e1
		}
	}

// phys_fn_001558 (0x0002e2f0, 122 B)
// phys_fn_001560 (0x0002e370, 478 B)
// Init(n) (thiscall, `ret 4`), the row and its continuation (the six-face
// switch body). n and 6 * n * n are stored and slot 1 (allocate) called; false
// when it fails. The spacing is 1 / ((n - 1) * 0.5) (an unsigned `fild`,
// stored narrow; for n = 1 a division by zero). Per face, row j and column i
// the direction is +-1 on the face's axis (-1 for faces 0, 2, 4) and
// 1 - i * s, 1 - j * s on the other two (y, z for x; z, x for y; x, y for z),
// normalised unless its squared length is zero (`fucompp; test ah, 0x44; jnp`:
// NaN normalises), and slot 2 is called with the sample face * n * n + j + i * n
// and the direction; false as soon as it fails (slot 3 is not called then).
// After the six faces slot 3, and true. The listing's instructions, naked; the
// switch jumps through gIceSupportMapFaceCases.
__declspec(naked) bool __fastcall nxSupportMapInit(IceSupportMap* /*map*/, NxU32 /*edx*/, NxU32 /*subdiv*/)
	{
	__asm
		{
		sub	esp, 0x1c		// 0x0002e2f0
		push	ebp		// 0x0002e2f3
		mov	ebp, dword ptr [esp + 0x24]		// 0x0002e2f4
		mov	eax, ebp		// 0x0002e2f8
		imul	eax, ebp		// 0x0002e2fa
		push	esi		// 0x0002e2fd
		mov	esi, ecx		// 0x0002e2fe
		mov	edx, dword ptr [esi]		// 0x0002e300
		lea	eax, [eax + eax*2]		// 0x0002e302
		shl	eax, 1		// 0x0002e305
		mov	dword ptr [esp + 0xc], esi		// 0x0002e307
		mov	dword ptr [esi + 4], ebp		// 0x0002e30b
		mov	dword ptr [esi + 8], eax		// 0x0002e30e
		call	dword ptr [edx + 4]		// 0x0002e311
		test	al, al		// 0x0002e314
		jne	L2e320		// 0x0002e316
		pop	esi		// 0x0002e318
		pop	ebp		// 0x0002e319
		add	esp, 0x1c		// 0x0002e31a
		ret	4		// 0x0002e31d
L2e320:
		lea	eax, [ebp - 1]		// 0x0002e320
		test	eax, eax		// 0x0002e323
		push	ebx		// 0x0002e325
		mov	dword ptr [esp + 0x2c], eax		// 0x0002e326
		fild	dword ptr [esp + 0x2c]		// 0x0002e32a
		push	edi		// 0x0002e32e
		jge	L2e337		// 0x0002e32f
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e331
L2e337:
		fmul	dword ptr kIceSupportMapHalf		// 0x0002e337
		xor	ebx, ebx		// 0x0002e33d
		fstp	dword ptr [esp + 0x30]		// 0x0002e33f
L2e343:
		xor	edi, edi		// 0x0002e343
		test	ebp, ebp		// 0x0002e345
		jbe	L2e525		// 0x0002e347
		mov	eax, ebx		// 0x0002e34d
		imul	eax, ebp		// 0x0002e34f
		imul	eax, ebp		// 0x0002e352
		mov	dword ptr [esp + 0x18], eax		// 0x0002e355
		jmp	L2e360		// 0x0002e359
L2e35b:
		mov	eax, dword ptr [esp + 0x18]		// 0x0002e35b
		nop		// 0x0002e35f
L2e360:
		xor	esi, esi		// 0x0002e360
		add	eax, edi		// 0x0002e362
		mov	dword ptr [esp + 0x10], eax		// 0x0002e364
		jmp	L2e370		// 0x0002e368
		_emit	0x8d
		_emit	0x9b
		_emit	0x00
		_emit	0x00
		_emit	0x00
		_emit	0x00		// 0x0002e36a lea ebx, [ebx]
L2e370:
		cmp	ebx, 5		// 0x0002e370
		ja	L2e499		// 0x0002e373
		jmp	dword ptr [ebx*4 + gIceSupportMapFaceCases]		// 0x0002e379
L2e380:
		test	ebx, ebx		// 0x0002e380
		mov	dword ptr [esp + 0x20], 0xbf800000		// 0x0002e382
		je	L2e394		// 0x0002e38a
		mov	dword ptr [esp + 0x20], 0x3f800000		// 0x0002e38c
L2e394:
		test	esi, esi		// 0x0002e394
		fld	dword ptr kIceSupportMapOne		// 0x0002e396
		fdiv	dword ptr [esp + 0x30]		// 0x0002e39c
		mov	dword ptr [esp + 0x1c], esi		// 0x0002e3a0
		fild	dword ptr [esp + 0x1c]		// 0x0002e3a4
		jge	L2e3b0		// 0x0002e3a8
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e3aa
L2e3b0:
		test	edi, edi		// 0x0002e3b0
		fmul	st, st(1)		// 0x0002e3b2
		mov	dword ptr [esp + 0x1c], edi		// 0x0002e3b4
		fsubr	dword ptr kIceSupportMapOne		// 0x0002e3b8
		fstp	dword ptr [esp + 0x24]		// 0x0002e3be
		fild	dword ptr [esp + 0x1c]		// 0x0002e3c2
		jge	L2e3ce		// 0x0002e3c6
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e3c8
L2e3ce:
		fmul	st, st(1)		// 0x0002e3ce
		fsubr	dword ptr kIceSupportMapOne		// 0x0002e3d0
		fstp	dword ptr [esp + 0x28]		// 0x0002e3d6
		jmp	L2e497		// 0x0002e3da
L2e3df:
		cmp	ebx, 2		// 0x0002e3df
		mov	dword ptr [esp + 0x24], 0xbf800000		// 0x0002e3e2
		je	L2e3f4		// 0x0002e3ea
		mov	dword ptr [esp + 0x24], 0x3f800000		// 0x0002e3ec
L2e3f4:
		test	esi, esi		// 0x0002e3f4
		fld	dword ptr kIceSupportMapOne		// 0x0002e3f6
		fdiv	dword ptr [esp + 0x30]		// 0x0002e3fc
		mov	dword ptr [esp + 0x1c], esi		// 0x0002e400
		fild	dword ptr [esp + 0x1c]		// 0x0002e404
		jge	L2e410		// 0x0002e408
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e40a
L2e410:
		test	edi, edi		// 0x0002e410
		fmul	st, st(1)		// 0x0002e412
		mov	dword ptr [esp + 0x1c], edi		// 0x0002e414
		fsubr	dword ptr kIceSupportMapOne		// 0x0002e418
		fstp	dword ptr [esp + 0x28]		// 0x0002e41e
		fild	dword ptr [esp + 0x1c]		// 0x0002e422
		jge	L2e42e		// 0x0002e426
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e428
L2e42e:
		fmul	st, st(1)		// 0x0002e42e
		fsubr	dword ptr kIceSupportMapOne		// 0x0002e430
		fstp	dword ptr [esp + 0x20]		// 0x0002e436
		jmp	L2e497		// 0x0002e43a
L2e43c:
		cmp	ebx, 4		// 0x0002e43c
		mov	dword ptr [esp + 0x28], 0xbf800000		// 0x0002e43f
		je	L2e451		// 0x0002e447
		mov	dword ptr [esp + 0x28], 0x3f800000		// 0x0002e449
L2e451:
		test	esi, esi		// 0x0002e451
		fld	dword ptr kIceSupportMapOne		// 0x0002e453
		fdiv	dword ptr [esp + 0x30]		// 0x0002e459
		mov	dword ptr [esp + 0x1c], esi		// 0x0002e45d
		fild	dword ptr [esp + 0x1c]		// 0x0002e461
		jge	L2e46d		// 0x0002e465
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e467
L2e46d:
		test	edi, edi		// 0x0002e46d
		fmul	st, st(1)		// 0x0002e46f
		mov	dword ptr [esp + 0x1c], edi		// 0x0002e471
		fsubr	dword ptr kIceSupportMapOne		// 0x0002e475
		fstp	dword ptr [esp + 0x20]		// 0x0002e47b
		fild	dword ptr [esp + 0x1c]		// 0x0002e47f
		jge	L2e48b		// 0x0002e483
		fadd	dword ptr kIceSupportMapTwo32		// 0x0002e485
L2e48b:
		fmul	st, st(1)		// 0x0002e48b
		fsubr	dword ptr kIceSupportMapOne		// 0x0002e48d
		fstp	dword ptr [esp + 0x24]		// 0x0002e493
L2e497:
		fstp	st(0)		// 0x0002e497
L2e499:
		fld	dword ptr [esp + 0x28]		// 0x0002e499
		fmul	dword ptr [esp + 0x28]		// 0x0002e49d
		fld	dword ptr [esp + 0x24]		// 0x0002e4a1
		fmul	dword ptr [esp + 0x24]		// 0x0002e4a5
		faddp	st(1), st		// 0x0002e4a9
		fld	dword ptr [esp + 0x20]		// 0x0002e4ab
		fmul	dword ptr [esp + 0x20]		// 0x0002e4af
		faddp	st(1), st		// 0x0002e4b3
		fld	dword ptr kIceSupportMapZero		// 0x0002e4b5
		fld	st(1)		// 0x0002e4bb
		fucompp		// 0x0002e4bd
		fnstsw	ax		// 0x0002e4bf
		test	ah, 0x44		// 0x0002e4c1
		jnp	L2e4ec		// 0x0002e4c4
		fsqrt		// 0x0002e4c6
		fdivr	dword ptr kIceSupportMapOne		// 0x0002e4c8
		fld	dword ptr [esp + 0x20]		// 0x0002e4ce
		fmul	st, st(1)		// 0x0002e4d2
		fstp	dword ptr [esp + 0x20]		// 0x0002e4d4
		fld	dword ptr [esp + 0x24]		// 0x0002e4d8
		fmul	st, st(1)		// 0x0002e4dc
		fstp	dword ptr [esp + 0x24]		// 0x0002e4de
		fmul	dword ptr [esp + 0x28]		// 0x0002e4e2
		fstp	dword ptr [esp + 0x28]		// 0x0002e4e6
		jmp	L2e4ee		// 0x0002e4ea
L2e4ec:
		fstp	st(0)		// 0x0002e4ec
L2e4ee:
		mov	ecx, dword ptr [esp + 0x14]		// 0x0002e4ee
		mov	edx, dword ptr [ecx]		// 0x0002e4f2
		lea	eax, [esp + 0x20]		// 0x0002e4f4
		push	eax		// 0x0002e4f8
		mov	eax, dword ptr [esp + 0x14]		// 0x0002e4f9
		push	eax		// 0x0002e4fd
		call	dword ptr [edx + 8]		// 0x0002e4fe
		test	al, al		// 0x0002e501
		je	L2e542		// 0x0002e503
		mov	ecx, dword ptr [esp + 0x10]		// 0x0002e505
		inc	esi		// 0x0002e509
		add	ecx, ebp		// 0x0002e50a
		cmp	esi, ebp		// 0x0002e50c
		mov	dword ptr [esp + 0x10], ecx		// 0x0002e50e
		jb	L2e370		// 0x0002e512
		inc	edi		// 0x0002e518
		cmp	edi, ebp		// 0x0002e519
		jb	L2e35b		// 0x0002e51b
		mov	esi, dword ptr [esp + 0x14]		// 0x0002e521
L2e525:
		inc	ebx		// 0x0002e525
		cmp	ebx, 6		// 0x0002e526
		jb	L2e343		// 0x0002e529
		mov	edx, dword ptr [esi]		// 0x0002e52f
		mov	ecx, esi		// 0x0002e531
		call	dword ptr [edx + 0xc]		// 0x0002e533
		pop	edi		// 0x0002e536
		pop	ebx		// 0x0002e537
		pop	esi		// 0x0002e538
		mov	al, 1		// 0x0002e539
		pop	ebp		// 0x0002e53b
		add	esp, 0x1c		// 0x0002e53c
		ret	4		// 0x0002e53f
L2e542:
		pop	edi		// 0x0002e542
		pop	ebx		// 0x0002e543
		pop	esi		// 0x0002e544
		xor	al, al		// 0x0002e545
		pop	ebp		// 0x0002e547
		add	esp, 0x1c		// 0x0002e548
		ret	4		// 0x0002e54b
		}
	}

// phys_fn_001563 (0x0002e570, 35 B)
// The base scalar deleting destructor (thiscall, `ret 4`): the base table, and
// the object freed through the getter when bit 0 of the flags is set. (Product
// form of the ObjectModel.cpp model.)
__declspec(noinline) IceSupportMap* __fastcall nxSupportMapBaseDelete(IceSupportMap* map, NxU32 /*edx*/,
	NxU32 flags)
	{
	map->mVtable = gIceSupportMapBaseTable;
	if(flags & 1)
		nxSupportMapFree(map);
	return map;
	}

// phys_fn_001565 (0x0002e5a0, 34 B)
// Constructor A (thiscall, `ret 4`): the base constructor, the hull, table A,
// no samples. (Product form of the ObjectModel.cpp model.)
__declspec(noinline) IceSupportMap* __fastcall nxSupportMapHullConstruct(IceSupportMap* map, NxU32 /*edx*/,
	ConvexHull* hull)
	{
	nxSupportMapBaseConstruct(map);
	map->mHull = hull;
	map->mVtable = gIceSupportMapHullTable;
	map->mSamples = 0;
	return map;
	}

// phys_fn_001567 (0x0002e5d0, 63 B)
// Slot 1 of A and B: the hull's polygons built when their count is zero
// (001472); false above 255 polygons (a byte per sample); else the sample bytes
// (slot 0, type 0) and whether they were allocated.
__declspec(noinline) bool __fastcall nxSupportMapHullAllocate(IceSupportMap* map)
	{
	ConvexHull* hull = map->mHull;
	if(!hull->mNbPolygons)
		nxHullComputePolygons(hull);
	if(hull->mNbPolygons > 0xff)
		return false;
	map->mSamples = (NxU8*) nxGetSdkAllocator()->malloc(map->mNbSamples, NX_MEMORY_PERSISTENT);
	return map->mSamples != 0;
	}

// phys_fn_001569 (0x0002e610, 34 B)
// Slot 2 of A (thiscall, `ret 8`): the hull's polygon furthest along the
// direction, without a pose (001496), as the sample's byte; true.
__declspec(noinline) bool __fastcall nxSupportMapHullCompute(IceSupportMap* map, NxU32 /*edx*/, NxU32 sample,
	const IceMaths::Point* dir)
	{
	map->mSamples[sample] = (NxU8) nxHullSupportPolygon(map->mHull, 0, dir, 0);
	return true;
	}

// phys_fn_001571 (0x0002e640, 34 B)
// Constructor B: as A with table B. (Product form of the ObjectModel.cpp model.)
__declspec(noinline) IceSupportMap* __fastcall nxSupportMapPlaneConstruct(IceSupportMap* map, NxU32 /*edx*/,
	ConvexHull* hull)
	{
	nxSupportMapBaseConstruct(map);
	map->mHull = hull;
	map->mVtable = gIceSupportMapPlaneTable;
	map->mSamples = 0;
	return map;
	}

// phys_fn_001573 (0x0002e670, 323 B)
// Slot 2 of B (thiscall, `ret 8`): the polygon through which the ray from the
// hull's centre along the direction leaves. The centre and the direction are
// copied; the polygons are built when absent (001472, the count and then the
// array tested). Per polygon facing the direction (n . d, (n.y d.y + n.z d.z)
// + n.x d.x, not below 0.0f; `test ah, 1`, so NaN skips), whose n . d over the
// copy, (d.x n.x + d.z n.z) + d.y n.y, lies outside (-1e-7, 1e-7) (the double
// constants; NaN passes), t = -((n.z c.z + n.x c.x) + n.y c.y + D) / (n . d)
// replaces the best when strictly less (from FLT_MAX; NaN never). The best
// index (0xff when none) is the sample's byte; true. Each pass reloads the hull
// from `this` and tests its polygons again. The listing's instructions, naked.
__declspec(naked) bool __fastcall nxSupportMapPlaneCompute(IceSupportMap* /*map*/, NxU32 /*edx*/,
	NxU32 /*sample*/, const IceMaths::Point* /*dir*/)
	{
	__asm
		{
		sub	esp, 0x24		// 0x0002e670
		push	ebp		// 0x0002e673
		mov	ebp, ecx		// 0x0002e674
		push	esi		// 0x0002e676
		mov	esi, dword ptr [ebp + 0x10]		// 0x0002e677
		mov	eax, dword ptr [esi + 0x18]		// 0x0002e67a
		mov	dword ptr [esp + 0x14], eax		// 0x0002e67d
		mov	ecx, dword ptr [esi + 0x1c]		// 0x0002e681
		mov	dword ptr [esp + 0x18], ecx		// 0x0002e684
		mov	edx, dword ptr [esi + 0x20]		// 0x0002e688
		push	edi		// 0x0002e68b
		mov	edi, dword ptr [esp + 0x38]		// 0x0002e68c
		mov	eax, dword ptr [edi]		// 0x0002e690
		mov	ecx, dword ptr [edi + 4]		// 0x0002e692
		mov	dword ptr [esp + 0x20], edx		// 0x0002e695
		mov	edx, dword ptr [edi + 8]		// 0x0002e699
		mov	dword ptr [esp + 0x24], eax		// 0x0002e69c
		mov	dword ptr [esp + 0x28], ecx		// 0x0002e6a0
		mov	dword ptr [esp + 0x2c], edx		// 0x0002e6a4
		mov	eax, dword ptr [esi + 0x24]		// 0x0002e6a8
		test	eax, eax		// 0x0002e6ab
		mov	dword ptr [esp + 0xc], ebp		// 0x0002e6ad
		mov	dword ptr [esp + 0x38], 0x7f7fffff		// 0x0002e6b1
		mov	dword ptr [esp + 0x10], 0xffffffff		// 0x0002e6b9
		jne	L2e6ca		// 0x0002e6c1
		mov	ecx, esi		// 0x0002e6c3
		call	nxHullComputePolygons		// 0x0002e6c5
L2e6ca:
		mov	edx, dword ptr [esi + 0x24]		// 0x0002e6ca
		xor	esi, esi		// 0x0002e6cd
		test	edx, edx		// 0x0002e6cf
		mov	dword ptr [esp + 0x14], edx		// 0x0002e6d1
		jbe	L2e79a		// 0x0002e6d5
		push	ebx		// 0x0002e6db
		xor	ebx, ebx		// 0x0002e6dc
		mov	edi, edi		// 0x0002e6de
L2e6e0:
		mov	ebp, dword ptr [ebp + 0x10]		// 0x0002e6e0
		mov	eax, dword ptr [ebp + 0x28]		// 0x0002e6e3
		test	eax, eax		// 0x0002e6e6
		jne	L2e6f5		// 0x0002e6e8
		mov	ecx, ebp		// 0x0002e6ea
		call	nxHullComputePolygons		// 0x0002e6ec
		mov	edx, dword ptr [esp + 0x18]		// 0x0002e6f1
L2e6f5:
		mov	ecx, dword ptr [ebp + 0x28]		// 0x0002e6f5
		fld	dword ptr [ecx + ebx + 0x10]		// 0x0002e6f8
		add	ecx, ebx		// 0x0002e6fc
		fmul	dword ptr [edi + 4]		// 0x0002e6fe
		fld	dword ptr [ecx + 0x14]		// 0x0002e701
		fmul	dword ptr [edi + 8]		// 0x0002e704
		faddp	st(1), st		// 0x0002e707
		fld	dword ptr [edi]		// 0x0002e709
		fmul	dword ptr [ecx + 0xc]		// 0x0002e70b
		faddp	st(1), st		// 0x0002e70e
		fcomp	dword ptr kIceSupportMapZero		// 0x0002e710
		fnstsw	ax		// 0x0002e716
		test	ah, 1		// 0x0002e718
		jne	L2e789		// 0x0002e71b
		fld	dword ptr [esp + 0x28]		// 0x0002e71d
		fmul	dword ptr [ecx + 0xc]		// 0x0002e721
		fld	dword ptr [esp + 0x30]		// 0x0002e724
		fmul	dword ptr [ecx + 0x14]		// 0x0002e728
		faddp	st(1), st		// 0x0002e72b
		fld	dword ptr [esp + 0x2c]		// 0x0002e72d
		fmul	dword ptr [ecx + 0x10]		// 0x0002e731
		faddp	st(1), st		// 0x0002e734
		fld	st(0)		// 0x0002e736
		fcomp	qword ptr kIceSupportMapMinusEpsilon		// 0x0002e738
		fnstsw	ax		// 0x0002e73e
		test	ah, 0x41		// 0x0002e740
		jne	L2e752		// 0x0002e743
		fcom	qword ptr kIceSupportMapEpsilon		// 0x0002e745
		fnstsw	ax		// 0x0002e74b
		test	ah, 5		// 0x0002e74d
		jnp	L2e787		// 0x0002e750
L2e752:
		fld	dword ptr [esp + 0x24]		// 0x0002e752
		fmul	dword ptr [ecx + 0x14]		// 0x0002e756
		fld	dword ptr [esp + 0x1c]		// 0x0002e759
		fmul	dword ptr [ecx + 0xc]		// 0x0002e75d
		faddp	st(1), st		// 0x0002e760
		fld	dword ptr [esp + 0x20]		// 0x0002e762
		fmul	dword ptr [ecx + 0x10]		// 0x0002e766
		faddp	st(1), st		// 0x0002e769
		fadd	dword ptr [ecx + 0x18]		// 0x0002e76b
		fdivrp	st(1), st		// 0x0002e76e
		fchs		// 0x0002e770
		fcom	dword ptr [esp + 0x3c]		// 0x0002e772
		fnstsw	ax		// 0x0002e776
		test	ah, 5		// 0x0002e778
		jp	L2e787		// 0x0002e77b
		fstp	dword ptr [esp + 0x3c]		// 0x0002e77d
		mov	dword ptr [esp + 0x14], esi		// 0x0002e781
		jmp	L2e789		// 0x0002e785
L2e787:
		fstp	st(0)		// 0x0002e787
L2e789:
		mov	ebp, dword ptr [esp + 0x10]		// 0x0002e789
		inc	esi		// 0x0002e78d
		add	ebx, 0x24		// 0x0002e78e
		cmp	esi, edx		// 0x0002e791
		jb	L2e6e0		// 0x0002e793
		pop	ebx		// 0x0002e799
L2e79a:
		mov	eax, dword ptr [ebp + 0xc]		// 0x0002e79a
		mov	ecx, dword ptr [esp + 0x34]		// 0x0002e79d
		mov	dl, byte ptr [esp + 0x10]		// 0x0002e7a1
		pop	edi		// 0x0002e7a5
		pop	esi		// 0x0002e7a6
		mov	byte ptr [ecx + eax], dl		// 0x0002e7a7
		mov	al, 1		// 0x0002e7aa
		pop	ebp		// 0x0002e7ac
		add	esp, 0x24		// 0x0002e7ad
		ret	8		// 0x0002e7b0
		}
	}

// phys_fn_001575 (0x0002e7c0, 35 B)
// Constructor C (thiscall, `ret 4`): the base constructor, both maps null, the
// vertex source, table C. (Product form of the ObjectModel.cpp model.)
__declspec(noinline) IceSupportMap* __fastcall nxSupportMapVertexConstruct(IceSupportMap* map, NxU32 /*edx*/,
	const ConvexHull* source)
	{
	nxSupportMapBaseConstruct(map);
	map->mSamples = 0;
	map->mSamples2 = 0;
	map->mVertexSource = source;
	map->mVtable = gIceSupportMapVertexTable;
	return map;
	}

// phys_fn_001577 (0x0002e7f0, 77 B)
// C's destructor body: table C, the second map and then the first freed and
// cleared, and the base table stored (a tail jump to 001554). (Product form of
// the ObjectModel.cpp model.)
__declspec(noinline) void __fastcall nxSupportMapVertexRelease(IceSupportMap* map)
	{
	map->mVtable = gIceSupportMapVertexTable;
	if(map->mSamples2)
		{
		nxSupportMapFree(map->mSamples2);
		map->mSamples2 = 0;
		}
	if(map->mSamples)
		{
		nxSupportMapFree(map->mSamples);
		map->mSamples = 0;
		}
	nxSupportMapBaseTable(map);
	}

// phys_fn_001579 (0x0002e840, 70 B)
// Slot 1 of C: false above 255 vertices; else both maps (slot 0, type 0, a
// byte per sample), false when the first fails, and whether the second was
// allocated.
__declspec(noinline) bool __fastcall nxSupportMapVertexAllocate(IceSupportMap* map)
	{
	if(map->mVertexSource->mNbVerts > 0xff)
		return false;
	map->mSamples = (NxU8*) nxGetSdkAllocator()->malloc(map->mNbSamples, NX_MEMORY_PERSISTENT);
	if(!map->mSamples)
		return false;
	map->mSamples2 = (NxU8*) nxGetSdkAllocator()->malloc(map->mNbSamples, NX_MEMORY_PERSISTENT);
	return map->mSamples2 != 0;
	}

// phys_fn_001581 (0x0002e890, 472 B)
// Slot 2 of C (thiscall, `ret 8`): over the source's vertices, p = (v.y d.y +
// v.x d.x) + v.z d.z; the least p (from FLT_MAX; `fst` when strictly below,
// `test ah, 5; jp`) gives the first byte and the greatest -p (kept on the
// stack from FLT_MAX: -p compared with it, replaced when strictly below) gives
// the second; four vertices at a time while at least four remain (with each
// unrolled step's own term order), then one at a time. Both indices start at 0.
// The listing's instructions, naked.
__declspec(naked) bool __fastcall nxSupportMapVertexCompute(IceSupportMap* /*map*/, NxU32 /*edx*/,
	NxU32 /*sample*/, const IceMaths::Point* /*dir*/)
	{
	__asm
		{
		sub	esp, 0x10		// 0x0002e890
		fld	dword ptr kIceSupportMapMax		// 0x0002e893
		push	ebx		// 0x0002e899
		mov	ebx, dword ptr [esp + 0x1c]		// 0x0002e89a
		push	ebp		// 0x0002e89e
		push	esi		// 0x0002e89f
		push	edi		// 0x0002e8a0
		mov	edi, ecx		// 0x0002e8a1
		mov	eax, dword ptr [edi + 0x14]		// 0x0002e8a3
		mov	ebp, dword ptr [eax + 0xc]		// 0x0002e8a6
		mov	esi, dword ptr [eax + 0x10]		// 0x0002e8a9
		xor	ecx, ecx		// 0x0002e8ac
		xor	edx, edx		// 0x0002e8ae
		cmp	ebp, 4		// 0x0002e8b0
		mov	dword ptr [esp + 0x1c], edi		// 0x0002e8b3
		mov	dword ptr [esp + 0x18], esi		// 0x0002e8b7
		mov	dword ptr [esp + 0x10], 0x7f7fffff		// 0x0002e8bb
		mov	dword ptr [esp + 0x14], ecx		// 0x0002e8c3
		jl	L2e9f4		// 0x0002e8c7
		mov	edi, 2		// 0x0002e8cd
		add	esi, 0x10		// 0x0002e8d2
L2e8d5:
		fld	dword ptr [esi - 0xc]		// 0x0002e8d5
		fmul	dword ptr [ebx + 4]		// 0x0002e8d8
		fld	dword ptr [esi - 0x10]		// 0x0002e8db
		fmul	dword ptr [ebx]		// 0x0002e8de
		faddp	st(1), st		// 0x0002e8e0
		fld	dword ptr [esi - 8]		// 0x0002e8e2
		fmul	dword ptr [ebx + 8]		// 0x0002e8e5
		faddp	st(1), st		// 0x0002e8e8
		fcom	dword ptr [esp + 0x10]		// 0x0002e8ea
		fnstsw	ax		// 0x0002e8ee
		test	ah, 5		// 0x0002e8f0
		jp	L2e8fd		// 0x0002e8f3
		fst	dword ptr [esp + 0x10]		// 0x0002e8f5
		mov	dword ptr [esp + 0x14], edx		// 0x0002e8f9
L2e8fd:
		fchs		// 0x0002e8fd
		fst	dword ptr [esp + 0x28]		// 0x0002e8ff
		fcomp	st(1)		// 0x0002e903
		fnstsw	ax		// 0x0002e905
		test	ah, 5		// 0x0002e907
		jp	L2e914		// 0x0002e90a
		fstp	st(0)		// 0x0002e90c
		mov	ecx, edx		// 0x0002e90e
		fld	dword ptr [esp + 0x28]		// 0x0002e910
L2e914:
		fld	dword ptr [esi - 4]		// 0x0002e914
		fmul	dword ptr [ebx]		// 0x0002e917
		fld	dword ptr [esi + 4]		// 0x0002e919
		fmul	dword ptr [ebx + 8]		// 0x0002e91c
		faddp	st(1), st		// 0x0002e91f
		fld	dword ptr [esi]		// 0x0002e921
		fmul	dword ptr [ebx + 4]		// 0x0002e923
		faddp	st(1), st		// 0x0002e926
		fcom	dword ptr [esp + 0x10]		// 0x0002e928
		fnstsw	ax		// 0x0002e92c
		test	ah, 5		// 0x0002e92e
		jp	L2e93e		// 0x0002e931
		lea	eax, [edi - 1]		// 0x0002e933
		fst	dword ptr [esp + 0x10]		// 0x0002e936
		mov	dword ptr [esp + 0x14], eax		// 0x0002e93a
L2e93e:
		fchs		// 0x0002e93e
		fst	dword ptr [esp + 0x28]		// 0x0002e940
		fcomp	st(1)		// 0x0002e944
		fnstsw	ax		// 0x0002e946
		test	ah, 5		// 0x0002e948
		jp	L2e956		// 0x0002e94b
		fstp	st(0)		// 0x0002e94d
		lea	ecx, [edi - 1]		// 0x0002e94f
		fld	dword ptr [esp + 0x28]		// 0x0002e952
L2e956:
		fld	dword ptr [esi + 0xc]		// 0x0002e956
		fmul	dword ptr [ebx + 4]		// 0x0002e959
		fld	dword ptr [esi + 0x10]		// 0x0002e95c
		fmul	dword ptr [ebx + 8]		// 0x0002e95f
		faddp	st(1), st		// 0x0002e962
		fld	dword ptr [esi + 8]		// 0x0002e964
		fmul	dword ptr [ebx]		// 0x0002e967
		faddp	st(1), st		// 0x0002e969
		fcom	dword ptr [esp + 0x10]		// 0x0002e96b
		fnstsw	ax		// 0x0002e96f
		test	ah, 5		// 0x0002e971
		jp	L2e97e		// 0x0002e974
		fst	dword ptr [esp + 0x10]		// 0x0002e976
		mov	dword ptr [esp + 0x14], edi		// 0x0002e97a
L2e97e:
		fchs		// 0x0002e97e
		fst	dword ptr [esp + 0x28]		// 0x0002e980
		fcomp	st(1)		// 0x0002e984
		fnstsw	ax		// 0x0002e986
		test	ah, 5		// 0x0002e988
		jp	L2e995		// 0x0002e98b
		fstp	st(0)		// 0x0002e98d
		mov	ecx, edi		// 0x0002e98f
		fld	dword ptr [esp + 0x28]		// 0x0002e991
L2e995:
		fld	dword ptr [esi + 0x18]		// 0x0002e995
		fmul	dword ptr [ebx + 4]		// 0x0002e998
		fld	dword ptr [esi + 0x1c]		// 0x0002e99b
		fmul	dword ptr [ebx + 8]		// 0x0002e99e
		faddp	st(1), st		// 0x0002e9a1
		fld	dword ptr [esi + 0x14]		// 0x0002e9a3
		fmul	dword ptr [ebx]		// 0x0002e9a6
		faddp	st(1), st		// 0x0002e9a8
		fcom	dword ptr [esp + 0x10]		// 0x0002e9aa
		fnstsw	ax		// 0x0002e9ae
		test	ah, 5		// 0x0002e9b0
		jp	L2e9c0		// 0x0002e9b3
		lea	eax, [edi + 1]		// 0x0002e9b5
		fst	dword ptr [esp + 0x10]		// 0x0002e9b8
		mov	dword ptr [esp + 0x14], eax		// 0x0002e9bc
L2e9c0:
		fchs		// 0x0002e9c0
		fst	dword ptr [esp + 0x28]		// 0x0002e9c2
		fcomp	st(1)		// 0x0002e9c6
		fnstsw	ax		// 0x0002e9c8
		test	ah, 5		// 0x0002e9ca
		jp	L2e9d8		// 0x0002e9cd
		fstp	st(0)		// 0x0002e9cf
		lea	ecx, [edi + 1]		// 0x0002e9d1
		fld	dword ptr [esp + 0x28]		// 0x0002e9d4
L2e9d8:
		add	edx, 4		// 0x0002e9d8
		lea	eax, [ebp - 3]		// 0x0002e9db
		add	esi, 0x30		// 0x0002e9de
		add	edi, 4		// 0x0002e9e1
		cmp	edx, eax		// 0x0002e9e4
		jb	L2e8d5		// 0x0002e9e6
		mov	esi, dword ptr [esp + 0x18]		// 0x0002e9ec
		mov	edi, dword ptr [esp + 0x1c]		// 0x0002e9f0
L2e9f4:
		cmp	edx, ebp		// 0x0002e9f4
		jae	L2ea46		// 0x0002e9f6
		lea	eax, [edx + edx*2]		// 0x0002e9f8
		lea	esi, [esi + eax*4 + 4]		// 0x0002e9fb
		nop		// 0x0002e9ff
L2ea00:
		fld	dword ptr [esi - 4]		// 0x0002ea00
		fmul	dword ptr [ebx]		// 0x0002ea03
		fld	dword ptr [esi + 4]		// 0x0002ea05
		fmul	dword ptr [ebx + 8]		// 0x0002ea08
		faddp	st(1), st		// 0x0002ea0b
		fld	dword ptr [ebx + 4]		// 0x0002ea0d
		fmul	dword ptr [esi]		// 0x0002ea10
		faddp	st(1), st		// 0x0002ea12
		fcom	dword ptr [esp + 0x10]		// 0x0002ea14
		fnstsw	ax		// 0x0002ea18
		test	ah, 5		// 0x0002ea1a
		jp	L2ea27		// 0x0002ea1d
		fst	dword ptr [esp + 0x10]		// 0x0002ea1f
		mov	dword ptr [esp + 0x14], edx		// 0x0002ea23
L2ea27:
		fchs		// 0x0002ea27
		fst	dword ptr [esp + 0x28]		// 0x0002ea29
		fcomp	st(1)		// 0x0002ea2d
		fnstsw	ax		// 0x0002ea2f
		test	ah, 5		// 0x0002ea31
		jp	L2ea3e		// 0x0002ea34
		fstp	st(0)		// 0x0002ea36
		mov	ecx, edx		// 0x0002ea38
		fld	dword ptr [esp + 0x28]		// 0x0002ea3a
L2ea3e:
		inc	edx		// 0x0002ea3e
		add	esi, 0xc		// 0x0002ea3f
		cmp	edx, ebp		// 0x0002ea42
		jb	L2ea00		// 0x0002ea44
L2ea46:
		mov	edx, dword ptr [edi + 0xc]		// 0x0002ea46
		fstp	st(0)		// 0x0002ea49
		mov	eax, dword ptr [esp + 0x24]		// 0x0002ea4b
		mov	bl, byte ptr [esp + 0x14]		// 0x0002ea4f
		mov	byte ptr [eax + edx], bl		// 0x0002ea53
		mov	edx, dword ptr [edi + 0x10]		// 0x0002ea56
		pop	edi		// 0x0002ea59
		pop	esi		// 0x0002ea5a
		pop	ebp		// 0x0002ea5b
		mov	byte ptr [eax + edx], cl		// 0x0002ea5c
		mov	al, 1		// 0x0002ea5f
		pop	ebx		// 0x0002ea61
		add	esp, 0x10		// 0x0002ea62
		ret	8		// 0x0002ea65
		}
	}

// phys_fn_001583 (0x0002ea70, 1 B)
// Slot 3 of every table: `ret`. (Also written as nxNoop1583 in ObjectModel.cpp
// and inline in core/Joint.h for the tables there.)
__declspec(naked) void __fastcall nxSupportMapNoop(IceSupportMap* /*map*/)
	{
	__asm
		{
		_emit	0xc3		// 0x0002ea70 ret (the inline assembler writes `ret` here as `ret 0`)
		}
	}

// phys_fn_001585 (0x0002ea80, 72 B)
// A's scalar deleting destructor (thiscall, `ret 4`): table A, the samples
// freed and cleared, the base table (001554), and the object freed when bit 0
// of the flags is set. (Product form of the ObjectModel.cpp model.)
__declspec(noinline) IceSupportMap* __fastcall nxSupportMapHullDelete(IceSupportMap* map, NxU32 /*edx*/,
	NxU32 flags)
	{
	map->mVtable = gIceSupportMapHullTable;
	if(map->mSamples)
		{
		nxSupportMapFree(map->mSamples);
		map->mSamples = 0;
		}
	nxSupportMapBaseTable(map);
	if(flags & 1)
		nxSupportMapFree(map);
	return map;
	}

// phys_fn_001587 (0x0002ead0, 72 B)
// B's scalar deleting destructor: as A's with table B. (Product form of the
// ObjectModel.cpp model.)
__declspec(noinline) IceSupportMap* __fastcall nxSupportMapPlaneDelete(IceSupportMap* map, NxU32 /*edx*/,
	NxU32 flags)
	{
	map->mVtable = gIceSupportMapPlaneTable;
	if(map->mSamples)
		{
		nxSupportMapFree(map->mSamples);
		map->mSamples = 0;
		}
	nxSupportMapBaseTable(map);
	if(flags & 1)
		nxSupportMapFree(map);
	return map;
	}

// phys_fn_001589 (0x0002eb20, 34 B)
// C's scalar deleting destructor (thiscall, `ret 4`): the body (001577), and
// the object freed when bit 0 of the flags is set. (Product form of the
// ObjectModel.cpp model.)
__declspec(noinline) IceSupportMap* __fastcall nxSupportMapVertexDelete(IceSupportMap* map, NxU32 /*edx*/,
	NxU32 flags)
	{
	nxSupportMapVertexRelease(map);
	if(flags & 1)
		nxSupportMapFree(map);
	return map;
	}
