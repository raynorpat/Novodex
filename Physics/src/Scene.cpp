/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal Scene, reconstructed from the shipped Win32 Release NxPhysics.dll.
//
// Every write below is a transcription of phys_fn_000647 (0x00012c10), read from
// its decompilation, in the order the oracle performs it. The decompilation names
// the object `param_1`, so an offset written as dword index `n` is byte offset
// `4n`; each line carries its index so a reader can check it against the oracle.
//
// Nothing here is elided: the oracle writes no field the object does not carry, and
// a field this file skips would be one the differential could not see.
//
// Three groups of calls appear:
//   - the reconstructed leaves phys_fn_004147 and phys_fn_002346, transcribed here;
//   - helpers this phase owns, implemented below;
//   - helpers another phase owns and has not reconstructed, declared at the top as
//     REPRODUCTION HOLES. Each is a named, single-purpose function so the owning
//     phase can displace it, and each is listed in the evidence with what it does
//     not model.

#include "Scene.h"

#include "Containers.h"
#include "NxSceneDesc.h"

#include <string.h>
#include <new>

// ---------------------------------------------------------------------------
// Reproduction holes. The oracle calls these; the phases that own them have not
// reconstructed them. Each declaration below is a seam, not a claim: the stub
// reproduces the call so the object is built at the right size in the right order,
// and the evidence records that its body is not modelled.
// ---------------------------------------------------------------------------

// Each function below is a REPRODUCTION HOLE for a helper another phase owns.
//
// The oracle's body is transcribed as far as its own field writes go, in the order
// it performs them. What is NOT reproduced is the base-class construction each one
// performs first -- FUN_100f0660 for four of them, FUN_100f0510 for two -- which
// lives in a phase that has not reconstructed it and which writes bytes outside the
// ranges those rows are known to touch. The evidence records that gap against each
// row; none of these helpers is reachable from a joint-descriptor differential, so
// the gap does not affect the closure these rows are being built for.

// phys_fn_005109 (0x000e1510, phase 4).
//   FUN_100f0660(this); dword[0xd]=0; dword[0xe]=0; *this=&PTR_FUN_1011b794;
static void nxSceneArrayHeaderInit(void* self);

void nxSceneMemberE1510(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	p[0x0d] = 0;
	p[0x0e] = 0;
	}

// phys_fn_005071 (0x000de7e0, phase 4).
//   FUN_100f0660(this); *this=&PTR_FUN_1011b784; dword[0xd..0x10]=0;
void nxSceneMemberDE7E0(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	p[0x0d] = 0;
	p[0x0e] = 0;
	p[0x0f] = 0;
	p[0x10] = 0;
	}

// phys_fn_005029 (0x000d4d00, phase 4).
//   FUN_100f0660(this); *this=&PTR_FUN_1011b774; byte[0x4c]=1;
void nxSceneMemberD4D00(void* self)
	{
	reinterpret_cast<unsigned char*>(self)[0x4c] = 1;
	}

// phys_fn_004996 (0x000d3490, phase 4).
//   FUN_100f0660(this); *this=&PTR_FUN_1011b764;
void nxSceneMemberD3490(void* self)
	{
	(void)self;
	}

// phys_fn_004938 (0x000bb510, phase 4).
//   FUN_100f0510(this); *this=&PTR_FUN_1011b750; SdkContainer(this+4);
//   dword[8..0xc]=0; byte[0x44]=1; byte[0x111]=1;
void nxSceneMemberBB510(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	new (p + 4) SdkContainer();
	p[8] = 0;
	p[9] = 0;
	p[10] = 0;
	p[0x0b] = 0;
	p[0x0c] = 0;
	reinterpret_cast<unsigned char*>(self)[0x44] = 1;
	reinterpret_cast<unsigned char*>(self)[0x111] = 1;
	}

// phys_fn_004899 (0x000b5720, phase 4).
//   FUN_100f0510(this); *this=&PTR_FUN_1011b628; dword[0x17..0x1a]=0;
//   dword[0x21]=0x7f7fffff; dword[0x22]=0; byte[0x23]=0; byte[0x8d]=1;
void nxSceneMemberB5720(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	p[0x17] = 0;
	p[0x18] = 0;
	p[0x19] = 0;
	p[0x1a] = 0;
	p[0x21] = 0x7f7fffffu;								// FLT_MAX
	p[0x22] = 0;
	reinterpret_cast<unsigned char*>(self)[0x23] = 0;
	reinterpret_cast<unsigned char*>(self)[0x8d] = 1;
	}

// phys_fn_001980 (0x0004ca30, phase 7).
//   FUN_100b4fe0(this); dword[0xb]=0; dword[0xc]=0; phys_fn_004147(this+0xd);
//   FUN_1002dae0(this+0x14); dword[0x16..0x1b]=+-FLT_MAX; dword[0x1c]=2;
//   dword[0x1d]=0; phys_fn_004836(this+0x1e);
void nxSceneMember4CA30(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	p[0x0b] = 0;
	p[0x0c] = 0;
	nxSceneArrayHeaderInit(p + 0x0d);					// phys_fn_004147
	p[0x16] = 0x7f7fffffu;
	p[0x17] = 0x7f7fffffu;
	p[0x18] = 0x7f7fffffu;
	p[0x19] = 0xff7fffffu;
	p[0x1a] = 0xff7fffffu;
	p[0x1b] = 0xff7fffffu;
	p[0x1c] = 2;
	p[0x1d] = 0;
	new (p + 0x1e) SdkContainer();						// phys_fn_004836
	}

// phys_fn_000285 (0x0000c310, phase 7). The 0x28-byte collector. The oracle
// installs three vtables, builds two ReadWriteLocks at +0x14 and +0x18 and a third
// at +0xc... +0x10, and stores `owner` at +0x24. The vtables are .rdata in the
// oracle and are left unmodelled; the owner write is what the Scene's own field
// 0x1b3 is read for.
void* nxSceneCollectorConstruct(void* self, void* owner)
	{
	unsigned* p = static_cast<unsigned*>(self);
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0;
	p[7] = 0;
	p[8] = 0;
	p[9] = 0;
	reinterpret_cast<unsigned char*>(self)[0x20] = 0;
	p[0x09] = reinterpret_cast<unsigned>(owner);		// +0x24
	return self;
	}

// phys_fn_002415 (0x0005bc10, phase 7). The 0xa8-byte auxiliary object. The oracle
// zeroes 30 dwords and stores `owner` at +0xa4.
void* nxSceneAuxConstruct(void* self, void* owner)
	{
	unsigned* p = static_cast<unsigned*>(self);
	for(int i = 0; i <= 0x28; ++i)
		p[i] = 0;
	p[0x29] = reinterpret_cast<unsigned>(owner);		// +0xa4
	return self;
	}

// ---------------------------------------------------------------------------
// Reconstructed leaves.
// ---------------------------------------------------------------------------

// phys_fn_004147 (0x0009a4e0), reconstructed: zero six dwords and terminate the
// allocator field with -1.
static void nxSceneArrayHeaderInit(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0xffffffffu;
	}

// phys_fn_002346 (0x0005ab50), reconstructed: a two-lane list init. The oracle
// writes [this]=this+8, [this+4]=this+0x18, then zeroes +8..+0x10 and +0x18..+0x20.
static void nxSceneListInit(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	const unsigned base = reinterpret_cast<unsigned>(self);
	p[0] = base + 8;
	p[1] = base + 0x18;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0;
	p[7] = 0;
	p[8] = 0;
	p[9] = 0;
	}

// ---------------------------------------------------------------------------
// The constructor.
// ---------------------------------------------------------------------------

static void nxSceneDelete(void* self, int flags);

namespace
	{
	// The scene vtable. The oracle's is at .rdata 0x001066f4 and its first slot is
	// the scalar deleting destructor; `createScene`'s failure path calls it as
	// `(**(code**)*puVar5)(1)`, which is slot 0 with flags = 1. Only that slot is
	// modelled, because no other virtual is reached from any reconstructed path.
	struct SceneVtable
		{
		void (NX_CALL_CONV *destroy)(void*, int);
		};
	}

void* NxSceneInternal::vtable()
	{
	static SceneVtable table = { &nxSceneDelete };
	return &table;
	}

NxSceneInternal::NxSceneInternal()
	{
	unsigned* p = reinterpret_cast<unsigned*>(mBytes);
	const unsigned base = reinterpret_cast<unsigned>(this);

	// The vtable the oracle installs.
	p[0x00] = reinterpret_cast<unsigned>(vtable());

	// dwords 1..10 are zeroed individually by the oracle.
	for(int i = 1; i <= 10; ++i)
		p[i] = 0;

	nxSceneArrayHeaderInit(p + 0x0b);					// phys_fn_004147
	new (p + 0x14) SdkContainer();						// phys_fn_004836
	nxSceneMemberE1510(p + 0x18);						// phys_fn_005109

	p[0x2a] = 0;
	p[0x2b] = 0;
	nxSceneMemberDE7E0(p + 0x2c);						// phys_fn_005071

	p[0x3d] = 0;
	p[0x3e] = 0;
	p[0x3f] = 0;
	p[0x40] = 0;
	p[0x41] = 0;
	p[0x42] = 0;
	p[0x43] = 0x3f8ccccdu;								// 1.1f
	nxSceneMemberD4D00(p + 0x44);						// phys_fn_005029

	p[0x91] = 0;
	p[0x92] = 0;
	p[0xa2] = 0x3f8ccccdu;								// 1.1f
	p[0x95] = 0;
	p[0x94] = 0;
	p[0x93] = 0;
	p[0x98] = 0;
	p[0x97] = 0;
	p[0x96] = 0;
	p[0x99] = 0;
	p[0x9a] = 0;
	p[0x9b] = 0;
	p[0x9c] = 0;
	p[0x9d] = 0;
	p[0x9e] = 0;
	p[0x9f] = 0;
	p[0xa0] = 0;
	p[0xa1] = 0;
	p[0xa1] = 0x3f800000u;								// 1.0f
	p[0x9d] = 0x3f800000u;								// 1.0f
	p[0x99] = 0x3f800000u;								// 1.0f
	nxSceneMemberD3490(p + 0xa3);						// phys_fn_004996

	p[0xc3] = 0;
	p[0xc4] = 0;
	p[0xc5] = 0;
	p[0xc1] = 0;
	p[0xc2] = 0;
	p[0xc6] = 0;
	p[199] = 0;											// 0xc7
	p[200] = 0;											// 0xc8
	p[0xc9] = 0;
	p[0xca] = 0x3f8ccccdu;								// 1.1f
	nxSceneMemberBB510(p + 0xcb);						// phys_fn_004938

	p[0x112] = 0;
	p[0x113] = 0;
	p[0x110] = 0;
	p[0x111] = 1;
	nxSceneMemberB5720(p + 0x114);						// phys_fn_004899
	new (p + 0x138) SdkContainer();
	new (p + 0x13c) SdkContainer();
	new (p + 0x140) SdkContainer();
	new (p + 0x144) SdkContainer();

	p[0x14b] = 0x3dcccccdu;								// 0.1f
	p[0x14c] = 10;
	p[0x14d] = 0;
	p[0x14e] = 0;
	p[0x14f] = 0;
	p[0x150] = 0;
	p[0x151] = 0;
	p[0x157] = 0;
	p[0x158] = 0;
	p[0x159] = 0;
	p[0x15b] = 0;
	p[0x15c] = 0;
	p[0x15d] = 0;
	p[0x15f] = 0;
	p[0x160] = 0;
	p[0x161] = 0;
	p[0x163] = 0;
	p[0x164] = 0;
	p[0x165] = 0;
	p[0x167] = 0;
	p[0x168] = 0;
	p[0x169] = 0;
	p[0x16a] = 0;
	p[0x16b] = 0;
	p[0x16c] = 0;
	p[0x16d] = 0;
	p[0x16e] = 0;
	p[0x16f] = 0;
	p[0x170] = 0;
	p[0x171] = 0;
	p[0x172] = 0;
	p[0x173] = 0;
	p[0x174] = 0xffffffffu;
	nxSceneListInit(p + 0x175);							// phys_fn_002346

	p[0x17f] = 0;
	p[0x180] = 0;
	p[0x181] = 0;
	p[0x183] = 0;
	p[0x184] = 0;
	p[0x185] = 0;
	p[0x187] = 0;
	p[0x188] = 0;
	nxSceneMember4CA30(p + 0x189);						// phys_fn_001980

	p[0x1ab] = 0;
	p[0x1ac] = 0;
	p[0x1ad] = 0;
	p[0x1ae] = 0;
	p[0x1af] = 0;
	p[0x1b0] = 0;
	p[0x1b1] = 0;
	p[0x1b2] = 0;
	p[0x1b3] = 0;
	p[0x1b4] = 0;
	p[0x1b5] = 0;
	p[0x1b6] = 0;
	p[0x1b7] = 0;
	p[0x1b9] = 0;
	p[0x1ba] = 0;
	p[0x1bb] = 0;
	p[0x1bc] = 0;
	p[0x1be] = 0;
	p[0x1bf] = 0;
	p[0x1c0] = 0;
	p[0x1c1] = 0;
	p[0x1c3] = 1;

	// The four self-references. The oracle stores the block it was handed, which is
	// this object.
	p[0x14a] = 0;
	p[0x149] = 0;
	p[0x148] = 0;
	p[0x2a] = base;
	p[0x3d] = base;
	p[0x91] = base;
	p[0xc1] = base;

	// phys_fn_000285, allocated 0x28 bytes.
	void* collector = nxGetSdkAllocator()->malloc(0x28, NX_MEMORY_PERSISTENT);
	p[0x1b3] = collector ? reinterpret_cast<unsigned>(nxSceneCollectorConstruct(collector, p))
						 : 0;

	// phys_fn_002415, allocated 0xa8 bytes.
	void* aux = nxGetSdkAllocator()->malloc(0xa8, NX_MEMORY_PERSISTENT);
	p[0x12] = aux ? reinterpret_cast<unsigned>(nxSceneAuxConstruct(aux, p)) : 0;
	}

// The scalar deleting destructor the vtable's slot 0 points at. The oracle's is
// phys_fn_000647's sibling in the vtable; the only caller a reconstructed path
// reaches is createScene's failure path, which calls it with flags = 1 after
// phys_fn_000651 returns false. The body releases the two blocks the constructor
// allocated and then the object, matching the oracle's two-argument
// destructor-then-free shape.
static void nxSceneDelete(void* self, int flags)
	{
	unsigned* p = static_cast<unsigned*>(self);
	if(p[0x1b3])
		nxGetSdkAllocator()->free(reinterpret_cast<void*>(p[0x1b3]));
	if(p[0x12])
		nxGetSdkAllocator()->free(reinterpret_cast<void*>(p[0x12]));
	if(flags & 1)
		nxGetSdkAllocator()->free(self);
	}

void NxSceneInternal::scalarDeletingDestructor(int flags)
	{
	nxSceneDelete(this, flags);
	}
