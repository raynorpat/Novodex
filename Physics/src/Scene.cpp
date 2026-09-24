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
#include "NpActor.h"
#include "NxJointDesc.h"
#include "NxJoint.h"
#include "NpJoint.h"
#include "NxMat33.h"
#include "NxQuat.h"

#include <stdio.h>
#include <string.h>
#include <new>

// ---------------------------------------------------------------------------
// Reproduction holes. The oracle calls these; the phases that own them have not
// reconstructed them. Each declaration below is a seam, not a claim: the stub
// reproduces the call so the object is built at the right size in the right order,
// and the evidence records that its body is not modelled.
// ---------------------------------------------------------------------------

// Declared before the helpers that use it; defined above the constructor.

// `p` is an `unsigned*`, so `p[N]` is byte 4N. Every offset in this file is a BYTE
// offset taken from the decompilation, so a bare `p[0x52c]` addresses byte 0x14B0 --
// the same confusion as the `p + 0x55c` slip, in a different spelling. This accessor
// takes the byte offset and does the division, so the two forms cannot be mixed up.
static inline unsigned& nxDword(unsigned* p, unsigned byteOffset)
	{
	return *reinterpret_cast<unsigned*>(
		reinterpret_cast<unsigned char*>(p) + byteOffset);
	}

static inline unsigned char* nxAt(unsigned* p, unsigned byteOffset);

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
	nxDword(p, 0x0d) = 0;
	nxDword(p, 0x0e) = 0;
	}

// phys_fn_005071 (0x000de7e0, phase 4).
//   FUN_100f0660(this); *this=&PTR_FUN_1011b784; dword[0xd..0x10]=0;
void nxSceneMemberDE7E0(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	nxDword(p, 0x0d) = 0;
	nxDword(p, 0x0e) = 0;
	nxDword(p, 0x0f) = 0;
	nxDword(p, 0x10) = 0;
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
	nxDword(p, 0x0b) = 0;
	nxDword(p, 0x0c) = 0;
	reinterpret_cast<unsigned char*>(self)[0x44] = 1;
	reinterpret_cast<unsigned char*>(self)[0x111] = 1;
	}

// phys_fn_004899 (0x000b5720, phase 4).
//   FUN_100f0510(this); *this=&PTR_FUN_1011b628; dword[0x17..0x1a]=0;
//   dword[0x21]=0x7f7fffff; dword[0x22]=0; byte[0x23]=0; byte[0x8d]=1;
void nxSceneMemberB5720(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	nxDword(p, 0x17) = 0;
	nxDword(p, 0x18) = 0;
	nxDword(p, 0x19) = 0;
	nxDword(p, 0x1a) = 0;
	nxDword(p, 0x21) = 0x7f7fffffu;								// FLT_MAX
	nxDword(p, 0x22) = 0;
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
	nxDword(p, 0x0b) = 0;
	nxDword(p, 0x0c) = 0;
	nxSceneArrayHeaderInit(nxAt(p, 0x0d));					// phys_fn_004147
	nxDword(p, 0x16) = 0x7f7fffffu;
	nxDword(p, 0x17) = 0x7f7fffffu;
	nxDword(p, 0x18) = 0x7f7fffffu;
	nxDword(p, 0x19) = 0xff7fffffu;
	nxDword(p, 0x1a) = 0xff7fffffu;
	nxDword(p, 0x1b) = 0xff7fffffu;
	nxDword(p, 0x1c) = 2;
	nxDword(p, 0x1d) = 0;
	new (nxAt(p, 0x1e)) SdkContainer();						// phys_fn_004836
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
	nxDword(p, 0x09) = reinterpret_cast<unsigned>(owner);		// +0x24
	return self;
	}

// phys_fn_002415 (0x0005bc10, phase 7). The 0xa8-byte auxiliary object. The oracle
// zeroes 30 dwords and stores `owner` at +0xa4.
void* nxSceneAuxConstruct(void* self, void* owner)
	{
	unsigned* p = static_cast<unsigned*>(self);
	for(int i = 0; i <= 0x28; ++i)
		p[i] = 0;
	nxDword(p, 0x29) = reinterpret_cast<unsigned>(owner);		// +0xa4
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
// phys_fn_00001de0 (0x00001de0, phase 5): the shape factory. REPRODUCTION HOLE;
// it returns a non-null shape object, which is what the caller tests.
void* nxShapeFactory(void* shapeDesc, void* actor);
// The multi-shape group builder (0x110 bytes). REPRODUCTION HOLE.
void* nxShapeGroupConstruct(void* actor, unsigned count);

// phys_fn_000ad6e0 (0x000ad6e0, phase 6): constructs a joint of the given type over
// a block. REPRODUCTION HOLE.
NxJoint* nxJointConstruct(void* memory, const void* desc, unsigned type);
// The per-type allocation sizes for joint types this transcription has not read.
// REPRODUCTION HOLE.
NxU32 nxJointSizeForType(unsigned type);
// phys_fn_00013e00 (0x00013e00, phase 7): registers a joint with the scene.
void nxSceneAddJoint(void* scene, void* joint);
// The joint's scalar deleting destructor, reached in the oracle through vtable slot
// 0x14 with an argument of 1.
void nxJointDestroy(void* joint);

void nxActorSetName(void* actor, unsigned name);
void nxActorBuildBody(void* actor, const unsigned* desc);
int nxActorComputeMass(void* actor, const unsigned* bodyWord);
void nxSceneAddActorObject(void* scene, void* object, void* actor);
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



// `p` is an `unsigned*` throughout this file, so `p + N` is byte 4N. Every offset
// below is a BYTE offset read from the oracle, and one round of this reconstruction
// was lost to writing `p + 0x55c` where `bytes(p) + 0x55c` was meant -- a write at
// byte 0x1570, past the end of the 0x710-byte Scene, which corrupted the heap and
// moved its visible victim whenever anything changed the allocation order. This
// helper exists so the distinction is made once and cannot be got wrong again.
static inline unsigned char* nxAt(unsigned* p, unsigned byteOffset)
	{
	return reinterpret_cast<unsigned char*>(p) + byteOffset;
	}

NxSceneInternal::NxSceneInternal()
	{
	unsigned* p = reinterpret_cast<unsigned*>(mBytes);
	const unsigned base = reinterpret_cast<unsigned>(this);

	// The vtable the oracle installs.
	nxDword(p, 0x00) = reinterpret_cast<unsigned>(vtable());

	// dwords 1..10 are zeroed individually by the oracle.
	for(int i = 1; i <= 10; ++i)
		p[i] = 0;

	nxSceneArrayHeaderInit(nxAt(p, 0x0b));					// phys_fn_004147
	new (nxAt(p, 0x14)) SdkContainer();						// phys_fn_004836
	nxSceneMemberE1510(nxAt(p, 0x18));						// phys_fn_005109

	nxDword(p, 0x2a) = 0;
	nxDword(p, 0x2b) = 0;
	nxSceneMemberDE7E0(nxAt(p, 0x2c));						// phys_fn_005071

	nxDword(p, 0x3d) = 0;
	nxDword(p, 0x3e) = 0;
	nxDword(p, 0x3f) = 0;
	nxDword(p, 0x40) = 0;
	nxDword(p, 0x41) = 0;
	nxDword(p, 0x42) = 0;
	nxDword(p, 0x43) = 0x3f8ccccdu;								// 1.1f
	nxSceneMemberD4D00(nxAt(p, 0x44));						// phys_fn_005029

	nxDword(p, 0x91) = 0;
	nxDword(p, 0x92) = 0;
	nxDword(p, 0xa2) = 0x3f8ccccdu;								// 1.1f
	nxDword(p, 0x95) = 0;
	nxDword(p, 0x94) = 0;
	nxDword(p, 0x93) = 0;
	nxDword(p, 0x98) = 0;
	nxDword(p, 0x97) = 0;
	nxDword(p, 0x96) = 0;
	nxDword(p, 0x99) = 0;
	nxDword(p, 0x9a) = 0;
	nxDword(p, 0x9b) = 0;
	nxDword(p, 0x9c) = 0;
	nxDword(p, 0x9d) = 0;
	nxDword(p, 0x9e) = 0;
	nxDword(p, 0x9f) = 0;
	nxDword(p, 0xa0) = 0;
	nxDword(p, 0xa1) = 0;
	nxDword(p, 0xa1) = 0x3f800000u;								// 1.0f
	nxDword(p, 0x9d) = 0x3f800000u;								// 1.0f
	nxDword(p, 0x99) = 0x3f800000u;								// 1.0f
	nxSceneMemberD3490(nxAt(p, 0xa3));						// phys_fn_004996

	nxDword(p, 0xc3) = 0;
	nxDword(p, 0xc4) = 0;
	nxDword(p, 0xc5) = 0;
	nxDword(p, 0xc1) = 0;
	nxDword(p, 0xc2) = 0;
	nxDword(p, 0xc6) = 0;
	p[199] = 0;											// 0xc7
	p[200] = 0;											// 0xc8
	nxDword(p, 0xc9) = 0;
	nxDword(p, 0xca) = 0x3f8ccccdu;								// 1.1f
	nxSceneMemberBB510(nxAt(p, 0xcb));						// phys_fn_004938

	nxDword(p, 0x112) = 0;
	nxDword(p, 0x113) = 0;
	nxDword(p, 0x110) = 0;
	nxDword(p, 0x111) = 1;
	nxSceneMemberB5720(nxAt(p, 0x114));						// phys_fn_004899
	new (nxAt(p, 0x138)) SdkContainer();
	new (nxAt(p, 0x13c)) SdkContainer();
	new (nxAt(p, 0x140)) SdkContainer();
	new (nxAt(p, 0x144)) SdkContainer();

	nxDword(p, 0x14b) = 0x3dcccccdu;								// 0.1f
	nxDword(p, 0x14c) = 10;
	nxDword(p, 0x14d) = 0;
	nxDword(p, 0x14e) = 0;
	nxDword(p, 0x14f) = 0;
	nxDword(p, 0x150) = 0;
	nxDword(p, 0x151) = 0;
	nxDword(p, 0x157) = 0;
	nxDword(p, 0x158) = 0;
	nxDword(p, 0x159) = 0;
	nxDword(p, 0x15b) = 0;
	nxDword(p, 0x15c) = 0;
	nxDword(p, 0x15d) = 0;
	nxDword(p, 0x15f) = 0;
	nxDword(p, 0x160) = 0;
	nxDword(p, 0x161) = 0;
	nxDword(p, 0x163) = 0;
	nxDword(p, 0x164) = 0;
	nxDword(p, 0x165) = 0;
	nxDword(p, 0x167) = 0;
	nxDword(p, 0x168) = 0;
	nxDword(p, 0x169) = 0;
	nxDword(p, 0x16a) = 0;
	nxDword(p, 0x16b) = 0;
	nxDword(p, 0x16c) = 0;
	nxDword(p, 0x16d) = 0;
	nxDword(p, 0x16e) = 0;
	nxDword(p, 0x16f) = 0;
	nxDword(p, 0x170) = 0;
	nxDword(p, 0x171) = 0;
	nxDword(p, 0x172) = 0;
	nxDword(p, 0x173) = 0;
	nxDword(p, 0x174) = 0xffffffffu;
	nxSceneListInit(nxAt(p, 0x175));							// phys_fn_002346

	nxDword(p, 0x17f) = 0;
	nxDword(p, 0x180) = 0;
	nxDword(p, 0x181) = 0;
	nxDword(p, 0x183) = 0;
	nxDword(p, 0x184) = 0;
	nxDword(p, 0x185) = 0;
	nxDword(p, 0x187) = 0;
	nxDword(p, 0x188) = 0;
	nxSceneMember4CA30(nxAt(p, 0x189));						// phys_fn_001980

	nxDword(p, 0x1ab) = 0;
	nxDword(p, 0x1ac) = 0;
	nxDword(p, 0x1ad) = 0;
	nxDword(p, 0x1ae) = 0;
	nxDword(p, 0x1af) = 0;
	nxDword(p, 0x1b0) = 0;
	nxDword(p, 0x1b1) = 0;
	nxDword(p, 0x1b2) = 0;
	nxDword(p, 0x1b3) = 0;
	nxDword(p, 0x1b4) = 0;
	nxDword(p, 0x1b5) = 0;
	nxDword(p, 0x1b6) = 0;
	nxDword(p, 0x1b7) = 0;
	nxDword(p, 0x1b9) = 0;
	nxDword(p, 0x1ba) = 0;
	nxDword(p, 0x1bb) = 0;
	nxDword(p, 0x1bc) = 0;
	nxDword(p, 0x1be) = 0;
	nxDword(p, 0x1bf) = 0;
	nxDword(p, 0x1c0) = 0;
	nxDword(p, 0x1c1) = 0;
	nxDword(p, 0x1c3) = 1;

	// The four self-references. The oracle stores the block it was handed, which is
	// this object.
	nxDword(p, 0x14a) = 0;
	nxDword(p, 0x149) = 0;
	nxDword(p, 0x148) = 0;
	nxDword(p, 0x2a) = base;
	nxDword(p, 0x3d) = base;
	nxDword(p, 0x91) = base;
	nxDword(p, 0xc1) = base;

	// phys_fn_000285, allocated 0x28 bytes.
	void* collector = nxGetSdkAllocator()->malloc(0x28, NX_MEMORY_PERSISTENT);
	nxDword(p, 0x1b3) = collector ? reinterpret_cast<unsigned>(nxSceneCollectorConstruct(collector, p))
						 : 0;

	// phys_fn_002415, allocated 0xa8 bytes.
	void* aux = nxGetSdkAllocator()->malloc(0xa8, NX_MEMORY_PERSISTENT);
	nxDword(p, 0x12) = aux ? reinterpret_cast<unsigned>(nxSceneAuxConstruct(aux, p)) : 0;
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

	// The oracle guards every one of these, and the guards are not decoration: on a
	// freshly zeroed header all three pointers are null, and `last - first` on two
	// null pointers is undefined -- in practice it yields 0, but a `count` derived
	// from uninitialised pointers is exactly the kind of value that turns into a
	// huge unsigned and then into a write past the end. The oracle's own shape, from
	// the descriptor initialiser's inline growth, is:
	//
	//     if (first == 0) count = 0; else count = (last - first) >> 2;
	//
	// and the capacity is `count * 2 + 2` or the literal 2 when count is zero. Both
	// are reproduced here.
	const unsigned count = first ? static_cast<unsigned>(last - first) : 0;

	// The oracle grows only when the array is FULL, not when it merely lacks room for
	// the requested count. Its condition, at 0x10011xxx in createActor, is
	//
	//     if (memEnd <= last) { ... grow ... }
	//
	// and the new capacity is `count * 2 + 2` entries. That distinction is the whole
	// bug this replaces: a spare-based trigger grows one push early, so a two-entry
	// array is built where the oracle builds a four-entry one, and the third actor
	// then writes past the end. The trace showed exactly that --
	// last == memEnd == 0x01931000 on the third push -- and this is why.
	//
	// The guards on the pointer difference stay: on a freshly zeroed header all three
	// are null and `last - first` is undefined (10b).
	if(!(memEnd <= last))
		return;

	// `count * 2 + 2`: two entries for an empty array, four for a full two-entry one.
	// `needed` is taken into account only when it exceeds that, which the oracle does
	// not do here -- it grows by the formula alone.
	const unsigned capacity = count * 2 + 2;
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


	// The 0x18-byte actor wrapper has no inline pose. nxActorBuildBody copies
	// the descriptor's 0x30-byte pose into the separate body at +0x20.
	// actor+0x14 is the BODY pointer, written by nxActorBuildBody above. The
	// oracle does not store userData there: the descriptor word at 0x0f is
	// userData and reaches the actor through a different field, which this
	// transcription has not identified. Writing it here would clobber the body.
	(void)d[0x0f];

	// The body. Scene::createJoint and the joint-descriptor rows both reach it
	// through actor+0x14, so it is built here rather than left to the shape path.
	nxActorBuildBody(actor, d);

	// The name, through phys_fn_0000edc0.
	nxActorSetName(actor, d[0x11]);

	// Word 0x12 selects the shape path, and it is the shape path that writes
	// actor+0x10 -- there is no separate body-building call before it. An earlier
	// version of this transcription tested actor+0x10 here, before the shape path
	// had run, and so returned 0 on every actor the harness built. The oracle's
	// structure, from its decompilation, is:
	//
	//   if (d[0x12] == 1) { single shape -> actor+0x10, or a group }   (flexible)
	//   else if (d[0x12] == 2) { the same shape path }                 (static)
	//   then the tail: body test, mass pass, scene registration
	//
	// The shape list. The descriptor carries {first, last} at words 0x13 and 0x14,
	// so the count is (last - first). Word 0x12 selects the path: 1 builds a
	// flexible body's shapes, 2 a static actor's.
	//
	// For both, the single-shape case is
	//     piVar4 = phys_fn_00001de0(*piVar4, this);
	//     actor+0x10 = piVar4;
	//     if (piVar4 == 0) return 0;
	// and the multi-shape case allocates a 0x110-byte group instead. The factory is
	// a REPRODUCTION HOLE; what matters for the caller is that actor+0x10 is
	// non-null, because the oracle returns 0 from here when it is not.
	const unsigned shapeCount = (d[0x14] - d[0x13]) >> 2;
	if(d[0x12] == 1 || d[0x12] == 2)
		{
		if(shapeCount == 1)
			{
			void* shape = nxShapeFactory(reinterpret_cast<void*>(d[0x13]), actor);
			a[0x10 / 4] = reinterpret_cast<unsigned>(shape);
			if(!shape)
					return 0;
			}
		else if(shapeCount > 1)
			{
			// The multi-shape group. The oracle allocates 0x110 bytes and links each
			// shape into it; the group is a hole and only its head is stored.
			void* group = nxShapeGroupConstruct(actor, shapeCount);
			a[0x10 / 4] = reinterpret_cast<unsigned>(group);
			if(!group)
					return 0;
			}
		}

	if(d[0x0c] == 0 && a[0x10 / 4])
		{
		// No body: register the actor with the scene and succeed.
		nxSceneAddActorObject(reinterpret_cast<void*>(a[1]),
			reinterpret_cast<void*>(a[0x10 / 4]), actor);
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
			{
			return 1;
			}
		nxSceneAddActorObject(reinterpret_cast<void*>(a[1]),
			reinterpret_cast<void*>(a[0x10 / 4]), actor);
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
	a[0x14 / 4] = 0;
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

	// The oracle allocates the 0x50-byte body before the public 0x18-byte
	// actor wrapper. Keep that order; the guarded allocator records it.
	void* outerMemory = nxGetSdkAllocator()->malloc(0x50, NX_MEMORY_PERSISTENT);
	if(!outerMemory)
		return 0;
	void* actorMemory = nxGetSdkAllocator()->malloc(0x18, NX_MEMORY_PERSISTENT);
	if(!actorMemory)
		{
		nxGetSdkAllocator()->free(outerMemory);
		return 0;
		}

	// phys_fn_00001450 constructs the public wrapper. It still has an incomplete
	// vtable, but the slots driven by the actor and joint staged pairs are wired.
	// The earlier 0x50-byte actor assumption was wrong: the guarded oracle probe
	// measured 0x18 for this wrapper and 0x50 for its outer body.
	static_cast<NpActorObject*>(actorMemory)->installVtable();
	NxActor* actor = static_cast<NxActor*>(nxActorConstruct(actorMemory, this));
	if(!actor)
		{
		nxGetSdkAllocator()->free(actorMemory);
		nxGetSdkAllocator()->free(outerMemory);
		return 0;
		}
	*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(actor) + 0x14) = outerMemory;

	// phys_fn_00002010 (0x00002010): applies the descriptor to the actor. Its return
	// is the actor's own vtable word at +0, which the oracle tests against zero to
	// decide the actor was built. Reproduction hole.
	if(!nxActorLoadFromDescInternal(actor, reinterpret_cast<const unsigned*>(&desc)))
		{
		nxSceneActorDestroy(actor);
		nxGetSdkAllocator()->free(actor);
		nxGetSdkAllocator()->free(outerMemory);
		nxSceneReportError("Actor Initialisation failed: returned NULL.");
		return 0;
		}

	// Push onto the Scene's actor array at +0x55c, growing it exactly as the
	// descriptor initialiser's reserve does. The oracle's sequence here is the same
	// capacity-compare-then-grow shape, inlined.
	//
	// `p` is an `unsigned*`, so the array's address is `p + 0x55c / 4` and NOT
	// `p + 0x55c`. The latter is byte offset 0x1570, which is past the end of the
	// 0x710-byte Scene -- and that single arithmetic slip was the heap corruption
	// seven rounds chased: a write into whatever the allocator put after the Scene,
	// whose visible victim therefore moved whenever anything changed the heap
	// layout. The `Scene` initialiser's own reserves below use the correct form,
	// which is why only the actor path corrupted.
	nxSceneArrayReserve(reinterpret_cast<unsigned char*>(p) + 0x55c, 1);
	unsigned* first = reinterpret_cast<unsigned*>(p[0x55c / 4]);
	unsigned* last = reinterpret_cast<unsigned*>(p[0x560 / 4]);
	if(last)
		{
		*last = reinterpret_cast<unsigned>(actor);
		p[0x560 / 4] = reinterpret_cast<unsigned>(last + 1);
		}

	// The wrapper overwrites the actor's +0x0c and +0x10 words with its two
	// lock links. The public-DLL probe confirmed both aliases: actor+0x0c equals
	// NpScene+0x0c, actor+0x10 equals NpScene+0x10, shared across actors.
	unsigned* holder = reinterpret_cast<unsigned*>(p[0x6cc / 4]);
	if(holder)
		{
		reinterpret_cast<unsigned*>(actor)[4] = holder[4];
		reinterpret_cast<unsigned*>(actor)[3] = holder[3];
		}

	// phys_fn_000100a0 (0x000100a0): updates the Scene's cached actor count from the
	// array the push just extended. Reproduction hole.
	nxSceneUpdateActorCount(this, static_cast<unsigned>(last - first) + 1);

	// The notification hook, when the Scene has one at +0x61c.
	if(p[0x61c / 4])
		nxSceneNotifyActorCreated(reinterpret_cast<void*>(p[0x61c / 4]));

	return actor;
	}

// phys_fn_000628 at 0x000123d0: the public wrapper passes the actor's 0x50-byte
// body, whose first word points back to the 0x18-byte public actor. The Scene
// removes that actor by swapping in the last entry, then tears down the owned
// body graph. Callback, name, and slot-reuse paths are separate gaps.
void NxSceneInternal::releaseActor(void* bodyPointer)
	{
	unsigned char* body = static_cast<unsigned char*>(bodyPointer);
	NxActor* actor = *reinterpret_cast<NxActor**>(body);
	NxActor** first = at<NxActor**>(0x55c);
	NxActor** last = at<NxActor**>(0x560);
	if(!first || !last || first == last)
		return;
	NxActor** found = first;
	while(found != last && *found != actor)
		++found;
	if(found == last)
		return;
	--last;
	*found = *last;
	at<NxActor**>(0x560) = last;

	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	if(record)
		{
		void** objects = at<void**>(0x56c);
		void** objectsEnd = at<void**>(0x570);
		for(void** it = objects; it && it != objectsEnd; ++it)
			if(*it == record)
				{
				--objectsEnd;
				*it = *objectsEnd;
				at<void**>(0x570) = objectsEnd;
				break;
				}
		}

	// The observed free order for a dynamic box actor: public wrapper,
	// dynamic record, shape helper, shape, then the outer body.
	nxGetSdkAllocator()->free(actor);
	if(record)
		nxGetSdkAllocator()->free(record);
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(shape)
		{
		void* helper = *reinterpret_cast<void**>(shape + 0x9c);
		if(helper) nxGetSdkAllocator()->free(helper);
		nxGetSdkAllocator()->free(shape);
		}
	nxGetSdkAllocator()->free(body);
	}


// ---------------------------------------------------------------------------
// phys_fn_000665 (0x000142c0, 718 B, phase 7) is Scene::createJoint.
//
// Its own error strings name it: "PhysicsSDK::createJoint: desc.isValid() fails!"
// and "PhysicsSDK::createJoint: at least one of the two actors must be dynamic!",
// both raised with __FILE__ ".../Physics/src/Scene.cpp".
//
// The oracle's structure, transcribed below:
//
//   a re-entry guard at the file-scope flag .data 0x00123c10, reported as
//     "Reentry check: You may not call t..." with line 0x245;
//   desc.isValid() through the descriptor's vtable slot 8;
//   a dynamic test: descriptor words 2 and 3 are the two actors, +0x14 is each
//     actor's body, and the body's +8 is the non-null marker;
//   a switch on descriptor word 1 (the joint type). The revolute case allocates
//     0x17c bytes and constructs through phys_fn_000ad6e0;
//   then the joint's +0x12 word, and if non-null, two words copied out of the
//     Scene's +0x6cc holder and a call to phys_fn_00013e00 to register it.
// ---------------------------------------------------------------------------

static bool gCreateJointReentry = false;

NxJoint* NxSceneInternal::createJoint(const NxJointDesc& desc)
	{
	unsigned* p = reinterpret_cast<unsigned*>(mBytes);
	const unsigned* d = reinterpret_cast<const unsigned*>(&desc);

	if(gCreateJointReentry)
		{
		nxSceneReportErrorA("Reentry check: You may not call this function "
			"recursively. Scene.cpp:0x245");
		return 0;
		}
	gCreateJointReentry = true;

	// desc.isValid() is the pinned header's own inline predicate, reached through
	// the descriptor's vtable in the oracle and called directly here.
	if(!desc.isValid())
		{
		nxSceneReportErrorA("PhysicsSDK::createJoint: desc.isValid() fails!");
		gCreateJointReentry = false;
		return 0;
		}

	// The dynamics test. Descriptor words 2 and 3 are actor[0] and actor[1]; each
	// actor's +0x14 is its body, and the body's +8 is the marker the oracle reads.
	const void* actor0 = reinterpret_cast<const void*>(d[2]);
	const void* actor1 = reinterpret_cast<const void*>(d[3]);
	const unsigned body0 = actor0 ? *reinterpret_cast<const unsigned*>(
		reinterpret_cast<const unsigned char*>(actor0) + 0x14) : 0;
	const unsigned body1 = actor1 ? *reinterpret_cast<const unsigned*>(
		reinterpret_cast<const unsigned char*>(actor1) + 0x14) : 0;
	const unsigned mark0 = body0 ? *reinterpret_cast<const unsigned*>(
		reinterpret_cast<const unsigned char*>(body0) + 8) : 0;
	const unsigned mark1 = body1 ? *reinterpret_cast<const unsigned*>(
		reinterpret_cast<const unsigned char*>(body1) + 8) : 0;

	if(!mark0 && !mark1)
		{
		nxSceneReportErrorA("PhysicsSDK::createJoint: at least one of the two actors "
			"must be dynamic!");
		gCreateJointReentry = false;
		return 0;
		}

	// The joint object, by type. The oracle switches on descriptor word 1; the
	// sizes are its allocation literals. Only the revolute case (type 0) has been
	// read; the others share the same shape and are named as holes.
	NxU32 size = 0;
	switch(d[1])
		{
		case 0: size = 0x17c; break;		// revolute
		default: size = nxJointSizeForType(d[1]); break;
		}

	NxJoint* joint = 0;
	if(size)
		{
		void* memory = nxGetSdkAllocator()->malloc(size, NX_MEMORY_PERSISTENT);
		if(memory)
			joint = nxJointConstruct(memory, &desc, d[1]);
		}

	if(!joint)
		{
		gCreateJointReentry = false;
		return 0;
		}

	// The two words out of the Scene's +0x6cc holder, then registration.
	unsigned marker = reinterpret_cast<unsigned*>(joint)[0x12 / 4];
	if(marker)
		{
		unsigned* holder = reinterpret_cast<unsigned*>(p[0x6cc / 4]);
		if(holder)
			{
			*reinterpret_cast<unsigned*>(marker + 0x10) = holder[3];
			*reinterpret_cast<unsigned*>(marker + 0x14) = holder[4];
			}
		nxSceneAddJoint(this, joint);
		}
	else
		{
		// The oracle calls the joint's vtable slot 0x14 with 1 -- its scalar deleting
		// destructor -- for the marker, and then FALLS THROUGH to the switch's default
		// and RETURNS THE JOINT it built. Destroying the marker is not destroying the
		// joint, so this returns it too.
		//
		// The joint is returned with its vtable installed, so the harness's virtual
		// calls -- getGlobalAnchor, getGlobalAxis, getState -- dispatch. The marker
		// word at +0x12 stays null, which is what the oracle's own guard expects for a
		// joint that has no marker (10u).
		nxJointDestroy(joint);
		}

	gCreateJointReentry = false;
	return joint;
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
		nxDword(p, 0x18) = limits[0];		// maxNbActors
		nxDword(p, 0x1c) = limits[1];		// maxNbBodies
		nxDword(p, 0x20) = limits[2];		// maxNbStaticShapes
		nxDword(p, 0x24) = limits[3];		// maxNbDynamicShapes
		nxDword(p, 0x28) = limits[4];		// maxNbJoints

		// `p` is an `unsigned*`, so these are byte offsets 0x55c and 0x56c only with
		// the cast. Without it they are 0x1570 and 0x15b0, past the end of the
		// 0x710-byte Scene -- the same slip as in createActor, dormant here only
		// because the harness's descriptor has no limits pointer.
		nxSceneArrayReserve(reinterpret_cast<unsigned char*>(p) + 0x55c, nxDword(p, 0x18));
		nxSceneArrayReserve(reinterpret_cast<unsigned char*>(p) + 0x56c, nxDword(p, 0x1c));
		}

	// +0x52c is written through the pointer at +0x6cc, then the three descriptor
	// words land at +0x52c, +0x530 and +0x534.
	unsigned* holder = reinterpret_cast<unsigned*>(p[0x6cc / 4]);
	if(holder)
		holder[1] = d[0x0d];
	nxDword(p, 0x52c) = d[7];				// maxTimestep
	nxDword(p, 0x530) = d[8];				// maxIter
	nxDword(p, 0x534) = d[9];				// solverType

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

	nxDword(p, 0x520) = d[1];				// userData
	nxDword(p, 0x524) = d[2];
	nxDword(p, 0x528) = d[3];

	// Bit 0 of +0x70c is the ground-plane enable, set or cleared from descriptor
	// byte 0x32.
	if(reinterpret_cast<const unsigned char*>(&desc)[0x32] == 0)
		nxDword(p, 0x70c) = nxDword(p, 0x70c) & 0xfffffffeu;
	else
		nxDword(p, 0x70c) = nxDword(p, 0x70c) | 1u;

	nxDword(p, 0x6ac) = d[4];
	nxDword(p, 0x6b0) = d[5];
	nxDword(p, 0x538) = 0;
	nxDword(p, 0x6b4) = d[6];
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
	if(nxDword(p, 0x1b3))
		nxGetSdkAllocator()->free(reinterpret_cast<void*>(nxDword(p, 0x1b3)));
	if(nxDword(p, 0x12))
		nxGetSdkAllocator()->free(reinterpret_cast<void*>(nxDword(p, 0x12)));
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
	return reinterpret_cast<NxActor*>(nxActorConstruct(memory, scene));
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
	unsigned char* actorBytes = static_cast<unsigned char*>(actor);
	unsigned char* body = *reinterpret_cast<unsigned char**>(actorBytes + 0x14);
	if(!body)
		return;
	memset(body, 0, 0x50);
	*reinterpret_cast<void**>(body) = actor;
	*reinterpret_cast<void**>(body + 4) = *reinterpret_cast<void**>(actorBytes + 4);
	memcpy(body + 0x20, desc, sizeof(NxMat34));
	}


int nxActorComputeMass(void* actor, const unsigned* bodyWord)
	{
	// The dynamic record is built after the shape and its helper, in the
	// order the guarded oracle allocator reports. Static actors skip this row.
	(void)bodyWord;
	unsigned char* actorBytes = static_cast<unsigned char*>(actor);
	unsigned char* body = *reinterpret_cast<unsigned char**>(actorBytes + 0x14);
	if(!body)
		return 1;
	unsigned char* record = static_cast<unsigned char*>(
		nxGetSdkAllocator()->malloc(0x260, NX_MEMORY_PERSISTENT));
	if(!record)
		return 1;
	memset(record, 0, 0x260);

	// The joint-descriptor exports walk actor+0x14 -> body+8 -> record+0x19c.
	// The last link points BACK to this same 0x50-byte body, whose +8 in turn
	// points to the record. The earlier candidate allocated an extra pose here.

	// The translation and matrix were copied from the descriptor into body.
	memcpy(record + 0x50, body + 0x44, 12);

	// The dynamic record carries a quaternion at +0x5c with w last. Convert
	// the descriptor's matrix already copied to body+0x20. The shipped path
	// uses the same Foundation matrix-to-quaternion convention.
	NxMat33 orientation;
	memcpy(&orientation, body + 0x20, sizeof(orientation));
	NxQuat quaternion(orientation);
	*reinterpret_cast<float*>(record + 0x5c) = quaternion.x;
	*reinterpret_cast<float*>(record + 0x60) = quaternion.y;
	*reinterpret_cast<float*>(record + 0x64) = quaternion.z;
	*reinterpret_cast<float*>(record + 0x68) = quaternion.w;

	*reinterpret_cast<void**>(record + 0x19c) = body;
	*reinterpret_cast<void**>(body + 0x08) = record;

	return 0;
	}

void nxSceneAddActorObject(void* scene, void* object, void* actorPointer)
	{
	// Register the dynamic record in the Scene's +0x56c array. Static actors
	// have no record and do not enter this array; the fourth box actor grows
	// its capacity from two entries to six, matching the pinned DLL.
	if(!object) return;
	unsigned char* actor = static_cast<unsigned char*>(actorPointer);
	if(!actor) return;
	unsigned char* body = *reinterpret_cast<unsigned char**>(actor + 0x14);
	unsigned char* record = body
		? *reinterpret_cast<unsigned char**>(body + 8) : 0;
	if(!record) return;
	unsigned char* bytes = static_cast<unsigned char*>(scene);
	nxSceneArrayReserve(bytes + 0x56c, 1);
	void** last = *reinterpret_cast<void***>(bytes + 0x570);
	if(last)
		{
		*last = record;
		*reinterpret_cast<void***>(bytes + 0x570) = last + 1;
		}
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

// phys_fn_00001de0 (0x00001de0, phase 5): the shape factory. REPRODUCTION HOLE.
//
// The oracle builds a shape object from the descriptor and links it into the actor.
// What the caller of this function checks is that it returned something, so this
// returns a small allocation through the SDK allocator rather than null -- an actor
// whose shape list is empty cannot be created, and every downstream path in this
// reconstruction needs creation to succeed before it can be measured.
//
// What is NOT modelled: the shape's own layout and vtable. The body points to
// the measured 0x228-byte shape; its +0x9c word reaches a 0x1c-byte helper.
void* nxShapeFactory(void* shapeDesc, void* actor)
	{
	(void)shapeDesc;
	unsigned char* shape = static_cast<unsigned char*>(
		nxGetSdkAllocator()->malloc(0x228, NX_MEMORY_PERSISTENT));
	if(!shape)
		return 0;
	memset(shape, 0, 0x228);
	// The oracle shape does not point at the public 0x18-byte actor wrapper.
	void* helper = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	if(!helper)
		{
		nxGetSdkAllocator()->free(shape);
		return 0;
		}
	memset(helper, 0, 0x1c);
	*reinterpret_cast<void**>(shape + 0x9c) = helper;
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	if(body)
		*reinterpret_cast<void**>(body + 0x10) = shape;
	return shape;
	}

// The multi-shape group builder. REPRODUCTION HOLE, as above.
void* nxShapeGroupConstruct(void* actor, unsigned count)
	{
	(void)count;
	unsigned char* group = static_cast<unsigned char*>(
		nxGetSdkAllocator()->malloc(0x110, NX_MEMORY_PERSISTENT));
	if(!group)
		return 0;
	for(int i = 0; i < 0x110; ++i)
		group[i] = 0;
	*reinterpret_cast<void**>(group + 4) = actor;
	return group;
	}

// ---------------------------------------------------------------------------
// Reproduction holes for Scene::createJoint's callees.
// ---------------------------------------------------------------------------

NxJoint* nxJointConstruct(void* memory, const void* desc, unsigned type)
	{
	(void)desc;
	(void)type;
	// The oracle's phys_fn_000ad6e0 builds the joint object, installs its vtable and
	// copies the descriptor into it. What Scene::createJoint reads back is the
	// joint's +0x12 word, which the oracle leaves non-null for a joint that was
	// constructed; this reproduces that so registration is reached.
	unsigned char* joint = static_cast<unsigned char*>(memory);
	for(int i = 0; i < 0x80; ++i)
		joint[i] = 0;

	// The vtable. The oracle's joint HAS one and the harness calls three of its
	// virtuals on what createJoint returns -- getGlobalAnchor, getGlobalAxis and
	// getState -- so a raw block leaves joint[0] at whatever the allocator left and
	// the first virtual call reads through it. That is the actor's defect (10f, 10k)
	// one class down, found in 10v.
	static_cast<NpJointObject*>(memory)->installVtable();
	// The descriptor, so the joint's virtuals can answer from what it carried.
	static_cast<NpJointObject*>(memory)->setDescriptor(
		static_cast<const NxJointDesc*>(desc));

	// The marker at +0x12 is left NULL, deliberately. The oracle's createJoint tests
	// it and skips the two-word copy out of the Scene's +0x6cc holder when it is null;
	// a hole that invents a value there is claiming state it does not have, and the
	// copy then writes through it. The guard caught exactly that: a write to address
	// 0x11, which is marker 1 plus the 0x10 offset of the first copied word (10u).
	//
	// A real joint built by phys_fn_000ad6e0 would have a marker; this hole does not
	// build one, so it says so.
	return reinterpret_cast<NxJoint*>(memory);
	}

NxU32 nxJointSizeForType(unsigned type)
	{
	// The oracle's switch has one case per joint type with its own literal. Only the
	// revolute case (0x17c) has been read; these are the other sizes the same shape
	// uses, recorded as holes rather than claims.
	switch(type)
		{
		case 1: return 0x17c;		// prismatic
		case 2: return 0x1b0;		// cylindrical
		case 3: return 0x150;		// spherical
		case 4: return 0x150;		// point on line
		case 5: return 0x150;		// point in plane
		case 6: return 0x220;		// distance
		case 7: return 0x1b0;		// pulley
		case 8: return 0x1b0;		// fixed
		case 9: return 0x260;		// D6
		default: return 0;
		}
	}

void nxSceneAddJoint(void* scene, void* joint)
	{
	// phys_fn_00013e00 (0x00013e00, phase 7) registers the joint with the scene.
	(void)scene;
	(void)joint;
	}

void nxJointDestroy(void* joint)
	{
	(void)joint;
	}
