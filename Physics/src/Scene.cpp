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
#include "NxActorDesc.h"
#include "NxActor.h"

#include <stdio.h>
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

// phys_fn_000544 (0x00010750, phase 7): applies the descriptor's flags.
void nxSceneApplyDescriptorFlags(void* scene, const unsigned* descWords, unsigned debug);
// phys_fn_000626 (0x00011730, phase 7) with phys_fn_000501 (0x0000ff10, phase 7):
// the ground-plane expansion.
void nxSceneBuildGroundPlane(void* scene);
// phys_fn_000651's array reserve, above.
void nxSceneArrayReserve(void* arrayHeader, unsigned needed);

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

// ---------------------------------------------------------------------------
// The callees Scene::createActor needs. Every one is a REPRODUCTION HOLE: a named
// function standing in for a row another phase owns and has not reconstructed.
// ---------------------------------------------------------------------------

// The deadlock report the wrapper slots print when a scene lock cannot be taken.
void nxSceneDeadlockReport();

// phys_fn_00001450 (0x00001450, phase 2): constructs the actor over a 0x50-byte
// block with a Scene pointer. Modelled only as far as storing the Scene so the
// object has the shape the caller expects.
// ---------------------------------------------------------------------------
// Rows Actor::loadFromDescInternal and the actor constructor call. REPRODUCTION
// HOLES, as above.
// ---------------------------------------------------------------------------
void nxActorSetName(void* actor, unsigned name);
void nxActorBuildBody(void* actor, const unsigned* desc);
int nxActorComputeMass(void* actor, const unsigned* bodyWord);
void nxSceneAddActorObject(void* scene, void* object);
void nxActorBuildUserDataObject(void* actor);
void nxSceneReportErrorA(const char* message);

// phys_fn_000034 (0x00002010), Actor::loadFromDescInternal. Returns 0 on failure
// and 1 on success, matching the oracle's return contract.
int nxActorLoadFromDescInternal(void* actor, const unsigned* descWords);
// phys_fn_000013 (0x00001450), the actor constructor.
void* nxActorConstruct(void* memory, void* scene);

NxActor* nxSceneActorConstruct(void* memory, void* scene);
// phys_fn_00002010 (0x00002010, phase 2): applies the descriptor to the actor and
// returns the actor's vtable word. Modelled as "applied, non-null".
void* nxSceneActorInitialise(NxActor* actor, const void* desc);
// phys_fn_00001c40 (0x00001c40, phase 2): destroys an actor built by the two above.
void nxSceneActorDestroy(NxActor* actor);
// phys_fn_000100a0 (0x000100a0, phase 7): refreshes a cached count from an array.
void nxSceneUpdateActorCount(void* scene, unsigned count);
// phys_fn_00089d50 (0x00089d50, phase 6): the scene's notification hook.
void nxSceneNotifyActorCreated(void* hook);
// The scene's error reporter.
void nxSceneReportError(const char* message);

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



// Reserves an embedded NxArraySDK<T> to `needed` entries using the Foundation
// allocator, which is what the oracle's capacity-compare-then-grow sequence at
// 0x00013070 does: it tests `last` against `memEnd`, doubles when it must, copies
// the live entries and releases the old block. The array is {first, last, memEnd,
// allocator}, so its three pointers are at +0, +4 and +8.
void nxSceneArrayReserve(void* arrayHeader, unsigned needed)
	{
	unsigned* a = static_cast<unsigned*>(arrayHeader);
	unsigned* first = reinterpret_cast<unsigned*>(a[0]);
	unsigned* last = reinterpret_cast<unsigned*>(a[1]);
	unsigned* memEnd = reinterpret_cast<unsigned*>(a[2]);
	const unsigned count = static_cast<unsigned>(last - first);

	if(static_cast<unsigned>(memEnd - last) >= needed - count && needed > count)
		return;

	const unsigned capacity = (needed > (count ? count * 2 : 2)) ? needed : (count ? count * 2 : 2);
	unsigned* grown = static_cast<unsigned*>(
		nxGetSdkAllocator()->malloc(capacity * sizeof(unsigned), NX_MEMORY_PERSISTENT));
	if(!grown)
		return;
	for(unsigned i = 0; i < count; ++i)
		grown[i] = first[i];
	if(first)
		nxGetSdkAllocator()->free(first);
	a[0] = reinterpret_cast<unsigned>(grown);
	a[1] = reinterpret_cast<unsigned>(grown + count);
	a[2] = reinterpret_cast<unsigned>(grown + capacity);
	}



// ---------------------------------------------------------------------------
// phys_fn_000034 (0x00002010) is Actor::loadFromDescInternal.
//
// Its error strings name it: "Actor::loadFromDescInternal: Compute mesh inertia
// tensor failed...", "...Can't compute mass from shapes: must have at least one
// non-trigger shape!", and its __FILE__ is ".../Physics/src/Actor.cpp".
//
// It is where the descriptor reaches the actor, and therefore where the actor's
// +0x14 (userData) and its shape list at +0x10 come from.
//
// The descriptor offsets below are NxActorDescBase fields, not inferred:
//   0x20..0x44  globalPose (9 dwords = the 3x3, the translation is word 9..0xb)
//   0x0c density?  -- words are named where the public header names them
// ---------------------------------------------------------------------------

int nxActorLoadFromDescInternal(void* actor, const unsigned* d)
	{
	unsigned* a = static_cast<unsigned*>(actor);

	// Nine dwords of global pose, copied from descriptor words 0..8 to actor+0x20.
	for(int i = 0; i < 9; ++i)
		a[(0x20 / 4) + i] = d[i];

	a[0x44 / 4] = d[9];				// globalPose.t.x
	a[0x48 / 4] = d[10];			// globalPose.t.y
	a[0x4c / 4] = d[0x0b];			// globalPose.t.z
	a[0x18 / 4] = d[0x0d];			// the body descriptor pointer
	a[0x1c / 4] = d[0x0e];			// the body descriptor's flags word
	a[0x14 / 4] = d[0x0f];			// userData

	// The name, through phys_fn_0000edc0.
	nxActorSetName(actor, d[0x11]);

	// A flexible body builds a body object first.
	if(d[0x12] == 1)
		{
		nxActorBuildBody(actor, d);
		if(!a[0x10 / 4])
			return 0;
		}

	// The shape list. The descriptor carries {first, last} at words 0x13 and 0x14,
	// and the count decides which path is taken.
	const unsigned count = d[0x14] - d[0x13];
	if(d[0x0c] == 0 && a[0x10 / 4])
		{
		// No body: register the actor with the scene and succeed.
		nxSceneAddActorObject(reinterpret_cast<void*>(a[1]), reinterpret_cast<void*>(a[0x10 / 4]));
		return 1;
		}

	const int mass = nxActorComputeMass(actor, &d[0x0c]);
	if(mass == 1)
		{
		nxSceneReportErrorA("Actor::loadFromDescInternal: Compute mesh inertia tensor "
			"failed for one of the actor's mesh shapes! Please change mesh geometry or "
			"supply a tensor manually!");
		return 0;
		}
	if(mass == 0)
		{
		if(!a[0x10 / 4])
			return 1;
		nxSceneAddActorObject(reinterpret_cast<void*>(a[1]), reinterpret_cast<void*>(a[0x10 / 4]));
		return 1;
		}
	nxSceneReportErrorA("Actor::loadFromDescInternal: Can't compute mass from shapes: "
		"must have at least one non-trigger shape!");
	return 0;
	}

// phys_fn_000013 (0x00001450) is the actor constructor.
void* nxActorConstruct(void* memory, void* scene)
	{
	unsigned* a = static_cast<unsigned*>(memory);
	unsigned* s = static_cast<unsigned*>(scene);

	a[1] = reinterpret_cast<unsigned>(scene);
	a[0x10 / 4] = 0;				// shape list empty
	a[0x4c / 4] = 0;
	a[0x48 / 4] = 0;
	a[0x44 / 4] = 0;

	// The identity 3x3 at +0x20..+0x40.
	a[0x20 / 4] = 0x3f800000u;
	a[0x30 / 4] = 0x3f800000u;
	a[0x40 / 4] = 0x3f800000u;
	a[0x24 / 4] = 0;
	a[0x28 / 4] = 0;
	a[0x2c / 4] = 0;
	a[0x34 / 4] = 0;
	a[0x38 / 4] = 0;
	a[0x3c / 4] = 0;
	a[8 / 4] = 0;

	// The scene hands out a slot id: either the counter at +0x6d0 is incremented, or
	// the free list at +0x6d4..+0x6d8 is popped.
	unsigned slot;
	const unsigned freeCount = (s[0x6d8 / 4] - s[0x6d4 / 4]) >> 2;
	if(freeCount == 0)
		{
		slot = s[0x6d0 / 4];
		s[0x6d0 / 4] = slot + 1;
		}
	else
		{
		slot = *reinterpret_cast<unsigned*>(s[0x6d4 / 4] + (freeCount - 1) * 4);
		s[0x6d8 / 4] = s[0x6d8 / 4] - 4;
		}
	a[0xc / 4] = slot;

	// The 0x18-byte sub-object the oracle allocates next. Reproduction hole.
	nxActorBuildUserDataObject(memory);
	return memory;
	}

// ---------------------------------------------------------------------------
// phys_fn_000626 (0x00011730) is Scene::createActor.
//
// It was listed in 8w as "the ground-plane expansion" because createScene drives
// it. Reading it shows it is the actor factory: its own error strings are
// "Supplied NxActorDesc is not valid. createActor returns NULL." and
// "Actor Initialisation failed: returned NULL.", and it ends by pushing the new
// actor onto the Scene's actor array at +0x55c. createScene reaches it because the
// ground plane is created by calling createActor.
//
// The validation half is NxActorDescBase::isValid(), which is the pinned public
// header's own inline code, so it is called rather than transcribed. The creation
// half is transcribed. The callees below are REPRODUCTION HOLES: each is a named
// function belonging to a phase that has not reconstructed it, and the evidence
// records what each does not model.
// ---------------------------------------------------------------------------

NxActor* NxSceneInternal::createActor(const NxActorDescBase& desc)
	{
	unsigned* p = reinterpret_cast<unsigned*>(mBytes);

	// The oracle inlines isValid() here as a long chain of __fpclass tests over the
	// twelve globalPose floats and the body's twelve, plus a shape validity loop.
	// The public header's isValid() is the same predicate, so it is used directly.
	if(!desc.isValid())
		{
		nxSceneReportError("Supplied NxActorDesc is not valid. createActor returns NULL.");
		return 0;
		}

	// The actor object: 0x50 bytes from the SDK allocator.
	void* actorMemory = nxGetSdkAllocator()->malloc(0x50, NX_MEMORY_PERSISTENT);
	if(!actorMemory)
		return 0;

	// phys_fn_00001450 (0x00001450): constructs the actor over the block, taking the
	// Scene pointer. Reproduction hole.
	NxActor* actor = static_cast<NxActor*>(nxActorConstruct(actorMemory, this));
	if(!actor)
		{
		nxGetSdkAllocator()->free(actorMemory);
		return 0;
		}

	// phys_fn_00002010 (0x00002010): applies the descriptor to the actor. Its return
	// is the actor's own vtable word at +0, which the oracle tests against zero to
	// decide the actor was built. Reproduction hole.
	if(!nxActorLoadFromDescInternal(actor, reinterpret_cast<const unsigned*>(&desc)))
		{
		nxSceneActorDestroy(actor);
		nxGetSdkAllocator()->free(actor);
		nxSceneReportError("Actor Initialisation failed: returned NULL.");
		return 0;
		}

	// Push onto the Scene's actor array at +0x55c, growing it exactly as the
	// descriptor initialiser's reserve does. The oracle's sequence here is the same
	// capacity-compare-then-grow shape, inlined.
	nxSceneArrayReserve(p + 0x55c, 1);
	unsigned* first = reinterpret_cast<unsigned*>(p[0x55c / 4]);
	unsigned* last = reinterpret_cast<unsigned*>(p[0x560 / 4]);
	if(last)
		{
		*last = reinterpret_cast<unsigned>(actor);
		p[0x560 / 4] = reinterpret_cast<unsigned>(last + 1);
		}

	// The two words copied out of the Scene's +0x6cc holder into the actor.
	unsigned* holder = reinterpret_cast<unsigned*>(p[0x6cc / 4]);
	if(holder)
		{
		reinterpret_cast<unsigned*>(actor)[4] = holder[4];		// actor+0x10
		reinterpret_cast<unsigned*>(actor)[3] = holder[3];		// actor+0x0c
		}

	// phys_fn_000100a0 (0x000100a0): updates the Scene's cached actor count from the
	// array the push just extended. Reproduction hole.
	nxSceneUpdateActorCount(this, static_cast<unsigned>(last - first) + 1);

	// The notification hook, when the Scene has one at +0x61c.
	if(p[0x61c / 4])
		nxSceneNotifyActorCreated(reinterpret_cast<void*>(p[0x61c / 4]));

	return actor;
	}

// ---------------------------------------------------------------------------
// phys_fn_000651 (0x00013070): the descriptor-driven initialiser.
//
// The oracle reads the descriptor at these offsets, all of which are NxSceneDesc
// fields rather than guesses:
//   0x00 vtable   0x04 userData   0x08 gravity   0x14 userContactReport
//   0x18 maxTimestep   0x1c maxIter   0x20 solverType   0x2c limits
//   0x30 groundPlane   0x31 upAxis   0x34 flags
//
// One block is a REPRODUCTION HOLE and is named as such below: the ground-plane
// expansion, which calls phys_fn_000626 (0x00011730, 3227 bytes) and
// phys_fn_000501 (0x0000ff10, 393 bytes). Both belong to phase 7 and neither is
// reconstructed. Everything outside that block is transcribed.
// ---------------------------------------------------------------------------

bool NxSceneInternal::initialise(const NxSceneDesc& desc)
	{
	const unsigned* d = reinterpret_cast<const unsigned*>(&desc);
	unsigned* p = reinterpret_cast<unsigned*>(mBytes);

	// The limits pointer is stored as five counts at dwords 6, 7, 8, 9 and 10 of
	// the descriptor, and the oracle copies them to +0x18..+0x28. It then reserves
	// two embedded arrays to the new actor and body counts, which is the
	// capacity-compare-then-grow sequence at 0x00013070's head.
	const unsigned* limits = reinterpret_cast<const unsigned*>(d[0x0b]);
	if(limits)
		{
		p[0x18] = limits[0];		// maxNbActors
		p[0x1c] = limits[1];		// maxNbBodies
		p[0x20] = limits[2];		// maxNbStaticShapes
		p[0x24] = limits[3];		// maxNbDynamicShapes
		p[0x28] = limits[4];		// maxNbJoints

		nxSceneArrayReserve(p + 0x55c, p[0x18]);
		nxSceneArrayReserve(p + 0x56c, p[0x1c]);
		}

	// +0x52c is written through the pointer at +0x6cc, then the three descriptor
	// words land at +0x52c, +0x530 and +0x534.
	unsigned* holder = reinterpret_cast<unsigned*>(p[0x6cc / 4]);
	if(holder)
		holder[1] = d[0x0d];
	p[0x52c] = d[7];				// maxTimestep
	p[0x530] = d[8];				// maxIter
	p[0x534] = d[9];				// solverType

	// phys_fn_000544 (0x00010750, phase 7) applies the flags and the debug word.
	// It is a reproduction hole.
	nxSceneApplyDescriptorFlags(this, d, d[0x0a]);

	// The ground-plane expansion. REPRODUCTION HOLE: the oracle builds a default
	// ground-plane shape descriptor on the stack, feeds it to phys_fn_000626 and
	// writes the resulting bounds back through phys_fn_000501. Neither row is
	// reconstructed, so this block reproduces the call and the byte flag it is
	// gated on, and nothing else. It is recorded in the evidence against this row.
	if(reinterpret_cast<const unsigned char*>(&desc)[0x30] != 0)
		nxSceneBuildGroundPlane(this);

	// The second ground-plane path, gated on upAxis != 0 and a non-null pointer at
	// descriptor word 0x0a. Same hole, six iterations in the oracle.
	if(reinterpret_cast<const unsigned char*>(&desc)[0x31] != 0 && d[0x0a] != 0)
		nxSceneBuildGroundPlane(this);

	p[0x520] = d[1];				// userData
	p[0x524] = d[2];
	p[0x528] = d[3];

	// Bit 0 of +0x70c is the ground-plane enable, set or cleared from descriptor
	// byte 0x32.
	if(reinterpret_cast<const unsigned char*>(&desc)[0x32] == 0)
		p[0x70c] = p[0x70c] & 0xfffffffeu;
	else
		p[0x70c] = p[0x70c] | 1u;

	p[0x6ac] = d[4];
	p[0x6b0] = d[5];
	p[0x538] = 0;
	p[0x6b4] = d[6];
	return true;
	}


// phys_fn_000544 (0x00010750, phase 7). REPRODUCTION HOLE. The oracle's body
// applies the descriptor's flag words and its debug value; it is 144 bytes and is
// not reconstructed. This reproduces the call and nothing else.
void nxSceneApplyDescriptorFlags(void* scene, const unsigned* descWords, unsigned debug)
	{
	(void)scene; (void)descWords; (void)debug;
	}

// phys_fn_000626 (0x00011730, phase 7) and phys_fn_000501 (0x0000ff10, phase 7).
// REPRODUCTION HOLE. The oracle builds a default ground-plane shape descriptor on
// the stack, calls 000626 with it, and feeds the result to 000501; 000626 is 3227
// bytes. This reproduces the call and nothing else, so a scene built with
// groundPlane set does NOT get a ground plane from this reconstruction.
void nxSceneBuildGroundPlane(void* scene)
	{
	(void)scene;
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

// ---------------------------------------------------------------------------
// Reproduction holes for Scene::createActor's callees. Each reproduces the call
// shape and the state the Scene reads back, and nothing else. Every one is
// recorded in the evidence with what it does not model.
// ---------------------------------------------------------------------------

NxActor* nxSceneActorConstruct(void* memory, void* scene)
	{
	// The actor's +0x14 is its userData, which the joint-descriptor rows read and
	// which the descriptor initialiser sets -- not this function. The only state
	// this hole has to establish is that the block is non-null and remembers its
	// Scene, so the caller's zero test behaves as the oracle's does.
	reinterpret_cast<unsigned*>(memory)[0x24 / 4] = reinterpret_cast<unsigned>(scene);
	return reinterpret_cast<NxActor*>(memory);
	}

void* nxSceneActorInitialise(NxActor* actor, const void* desc)
	{
	(void)actor;
	(void)desc;
	// The oracle returns the actor's vtable word here and the caller tests it
	// against zero. Returning a non-null constant reproduces that decision.
	return const_cast<void*>(desc);
	}

void nxSceneActorDestroy(NxActor* actor)
	{
	(void)actor;
	}

void nxSceneUpdateActorCount(void* scene, unsigned count)
	{
	// The oracle stores the array length into a Scene cache. The field is not one
	// this reconstruction has identified, so the call is reproduced and the store
	// is not.
	(void)scene;
	(void)count;
	}

void nxSceneNotifyActorCreated(void* hook)
	{
	(void)hook;
	}

void nxSceneReportError(const char* message)
	{
	// The oracle routes this through NxFoundation::FoundationSDK::error with its own
	// __FILE__ and line. The message is kept so the behaviour is greppable; the
	// Foundation error route is not called because the pinned line number is not
	// recovered for this row.
	printf("NxPhysics: %s\n", message);
	}

// Reproduction holes for Actor::loadFromDescInternal's callees.
void nxActorSetName(void* actor, unsigned name)
	{
	// The oracle (phys_fn_0000edc0) releases any previous name block and stores the
	// new one. The actor field is not one this reconstruction has identified.
	(void)actor; (void)name;
	}

void nxActorBuildBody(void* actor, const unsigned* desc)
	{
	// The oracle builds the body object and links it at actor+0x10. Not modelled.
	(void)actor; (void)desc;
	}

int nxActorComputeMass(void* actor, const unsigned* bodyWord)
	{
	// The oracle (phys_fn_000019b0) computes the mass from the shapes and returns
	// 1 for a mesh-inertia failure, 0 for success, and something else for "no
	// non-trigger shape". Returning 0 reproduces the success path.
	(void)actor; (void)bodyWord;
	return 0;
	}

void nxSceneAddActorObject(void* scene, void* object)
	{
	// The oracle (phys_fn_00010600) registers the object with the scene. Not
	// modelled.
	(void)scene; (void)object;
	}

void nxActorBuildUserDataObject(void* actor)
	{
	(void)actor;
	}

void nxSceneReportErrorA(const char* message)
	{
	printf("NxPhysics: %s\n", message);
	}

// The wrapper slots' deadlock report, with the oracle's own text.
void nxSceneDeadlockReport()
	{
	printf("NxPhysics: PhysicsSDK: WriteLock is still aquired. Procedure call skipped "
		"to avoid a deadlock!\n");
	}
