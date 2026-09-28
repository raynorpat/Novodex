/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal Scene, reconstructed from the shipped Win32 Release NxPhysics.dll.
//
// The constructor is a transcription of phys_fn_000647 (0x00012c10) from its
// Capstone listing, in the order the oracle performs it. Every offset is a BYTE
// offset, written through nxDword/nxAt, and the listing address of each run of
// stores is given beside it so a reader can check it against the oracle.
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
#include "NxBodyDesc.h"
#include "NxShapeDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxActor.h"
#include "NpActor.h"
#include "NpActorDynamicMath.h"
#include "NpScene.h"
#include "NxJointDesc.h"
#include "NxJoint.h"
#include "core/RevoluteJoint.h"
#include "core/PrismaticJoint.h"
#include "core/CylindricalJoint.h"
#include "core/SphericalJoint.h"
#include "core/PointOnLineJoint.h"
#include "core/PointInPlaneJoint.h"
#include "core/DistanceJoint.h"
#include "core/PulleyJoint.h"
#include "core/FixedJoint.h"
#include "core/D6Joint.h"
#include "NxMat33.h"
#include "NxQuat.h"
#include "FoundationSDK.h"
#include "NxAllocateable.h"
#include "NxUtilities.h"
#include "Observable.h"

#include <stdio.h>
#include <string.h>
#include <new>

// Public shape final and descriptor loader share the oracle's global name map.
void nxShapeSetName(void* shape, const char* name);
void nxShapeFactoryInitializePose(void* shape, const void* localPose);
void nxShapeFactoryRefreshPose(void* shape);
void nxShapeFactoryInstallVtable(void* shape, unsigned type);
// ShapeBase::nxApplyOwnerUpdate (phys_fn_001315, ObjectModel.cpp) on a shape.
void nxShapeApplyOwnerUpdate(void* shape, unsigned flags);
// ObjectModel.cpp's rows the Task 4 chain calls: phys_fn_000012 (the id pool),
// phys_fn_000028 (its push) and phys_fn_002344 (the Scene+0x5d4 pair removal).
unsigned nxIdAllocNext(void* container);
void nxU32VectorPushBack(void* vecHeader, NxU32 value);
void nxSceneRemovePairs(void* container, void* shape);
void nxShapeFactoryInitializePlane(void* shape, const float* normal,
	float distance);

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
// Each helper makes its row's field writes and those of the base-class construction
// it performs first (0x000f0660 or 0x000f0510, and 0x000b4fe0/0x0002dae0 for
// phys_fn_001980), in the listing's order. Only the vtable words those constructors
// install are not written; see the comment above nxSceneSubobjectRootInit.

// phys_fn_000544 (0x00010750, phase 7): applies the descriptor's flags.
void nxSceneApplyDescriptorFlags(void* scene, const unsigned* descWords, unsigned debug);
// phys_fn_000626 (0x00011730, phase 7) with phys_fn_000501 (0x0000ff10, phase 7):
// the ground-plane expansion.
void nxSceneBuildGroundPlane(void* scene);
// phys_fn_000651's array reserve, above.
void nxSceneArrayReserve(void* arrayHeader, unsigned needed);
void nxSceneRecycleActorId(NxSceneInternal* scene, unsigned id);
void* nxBoxShapePublicVtable();
void* nxShapePublicVtable(unsigned type);
void nxSceneBroadphaseRegister(NxSceneInternal* scene, void* body);
void nxSceneBroadphaseUnregister(NxSceneInternal* scene, void* body);
static void nxSceneStaticPrunerUnregister(NxSceneInternal* scene, unsigned char* shape);
void nxSceneAuxRegisterRecord(NxSceneInternal* scene, void* record);
void nxSceneAuxUnregisterRecord(NxSceneInternal* scene, void* record);
unsigned nxSceneTakeShapeId(NxSceneInternal* scene);
void nxSceneRecycleShapeId(NxSceneInternal* scene, unsigned id);

// phys_fn_004147's declaration; defined with the reconstructed leaves below.
static void nxSceneArrayHeaderInit(void* self);

// The sub-object constructors phys_fn_000647 calls. Every offset below is a BYTE
// offset read from the Capstone listing of the named row (and of the base-class
// constructors it calls first), and every store the listing makes is made here,
// in the listing's order, with one exception: the vtable word each sub-object
// installs at its +0 (and the intermediate base-class vtables) points into the
// oracle's .rdata and has no counterpart in this reconstruction, so it is not
// written. No reconstructed path reads a sub-object's vtable word.
//
// An earlier conversion turned the decompilation's dword indices into byte
// offsets without scaling them (`dword[0xd]` became byte 0xd), so every one of
// these helpers, and the Scene constructor itself, wrote into the wrong bytes
// and left the right ones to whatever the allocator returned. A zeroing
// allocator hid it; units/joint-open-items-contract.md "## Scene
// initialisation" records the fix.

// 0x000f0510, the root base: vtable at +0, then +4, +8, +0xc zeroed.
static void nxSceneSubobjectRootInit(void* self)
	{
	nxDword(static_cast<unsigned*>(self), 0x04) = 0;
	nxDword(static_cast<unsigned*>(self), 0x08) = 0;
	nxDword(static_cast<unsigned*>(self), 0x0c) = 0;
	}

// 0x000f0660: 0x000f0510, then +0x10, +0x2c, +0x30 zeroed and its own vtable.
static void nxSceneSubobjectBaseInit(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	nxSceneSubobjectRootInit(self);
	nxDword(p, 0x10) = 0;
	nxDword(p, 0x2c) = 0;
	nxDword(p, 0x30) = 0;
	}

// phys_fn_005109 (0x000e1510, phase 4): 0x000f0660, then +0x34, +0x38 zeroed.
void nxSceneMemberE1510(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	nxSceneSubobjectBaseInit(self);
	nxDword(p, 0x34) = 0;
	nxDword(p, 0x38) = 0;
	}

// phys_fn_005071 (0x000de7e0, phase 4): 0x000f0660, then +0x3c, +0x38, +0x34,
// +0x40 zeroed, in that order.
void nxSceneMemberDE7E0(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	nxSceneSubobjectBaseInit(self);
	nxDword(p, 0x3c) = 0;
	nxDword(p, 0x38) = 0;
	nxDword(p, 0x34) = 0;
	nxDword(p, 0x40) = 0;
	}

// phys_fn_005029 (0x000d4d00, phase 4): 0x000f0660, then byte +0x130 = 1.
void nxSceneMemberD4D00(void* self)
	{
	nxSceneSubobjectBaseInit(self);
	static_cast<unsigned char*>(self)[0x130] = 1;
	}

// phys_fn_004996 (0x000d3490, phase 4): 0x000f0660 and its own vtable only.
void nxSceneMemberD3490(void* self)
	{
	nxSceneSubobjectBaseInit(self);
	}

// phys_fn_004938 (0x000bb510, phase 4): 0x000f0510, SdkContainer at +0x10
// (phys_fn_004836), +0x20..+0x30 zeroed, bytes +0x110 and +0x111 = 1.
void nxSceneMemberBB510(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	nxSceneSubobjectRootInit(self);
	new (nxAt(p, 0x10)) SdkContainer();
	nxDword(p, 0x20) = 0;
	nxDword(p, 0x24) = 0;
	nxDword(p, 0x28) = 0;
	nxDword(p, 0x2c) = 0;
	nxDword(p, 0x30) = 0;
	static_cast<unsigned char*>(self)[0x110] = 1;
	static_cast<unsigned char*>(self)[0x111] = 1;
	}

// phys_fn_004899 (0x000b5720, phase 4): 0x000f0510, +0x5c..+0x68 and +0x88
// zeroed, byte +0x8c = 0, +0x84 = FLT_MAX, byte +0x8d = 1.
void nxSceneMemberB5720(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	nxSceneSubobjectRootInit(self);
	nxDword(p, 0x5c) = 0;
	nxDword(p, 0x60) = 0;
	nxDword(p, 0x64) = 0;
	nxDword(p, 0x68) = 0;
	nxDword(p, 0x88) = 0;
	static_cast<unsigned char*>(self)[0x8c] = 0;
	nxDword(p, 0x84) = 0x7f7fffffu;								// FLT_MAX
	static_cast<unsigned char*>(self)[0x8d] = 1;
	}

// phys_fn_001980 (0x0004ca30, phase 7). Its base 0x000b4fe0 zeroes +0 and
// +0x1c..+0x28 and writes the bounds +4..+0x18 as +-FLT_MAX; then +0x2c, +0x30
// zeroed, phys_fn_004147 at +0x34, 0x0002dae0 at +0x50 (+0x50, +0x54 zeroed),
// +0x58..+0x6c = +-FLT_MAX, +0x70 = 2, +0x74 = 0, phys_fn_004836 at +0x78.
void nxSceneMember4CA30(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	nxDword(p, 0x00) = 0;
	nxDword(p, 0x1c) = 0;
	nxDword(p, 0x20) = 0;
	nxDword(p, 0x24) = 0;
	nxDword(p, 0x28) = 0;
	nxDword(p, 0x04) = 0x7f7fffffu;
	nxDword(p, 0x08) = 0x7f7fffffu;
	nxDword(p, 0x0c) = 0x7f7fffffu;
	nxDword(p, 0x10) = 0xff7fffffu;
	nxDword(p, 0x14) = 0xff7fffffu;
	nxDword(p, 0x18) = 0xff7fffffu;
	nxDword(p, 0x2c) = 0;
	nxDword(p, 0x30) = 0;
	nxSceneArrayHeaderInit(nxAt(p, 0x34));					// phys_fn_004147
	nxDword(p, 0x50) = 0;									// 0x0002dae0
	nxDword(p, 0x54) = 0;
	nxDword(p, 0x58) = 0x7f7fffffu;
	nxDword(p, 0x5c) = 0x7f7fffffu;
	nxDword(p, 0x60) = 0x7f7fffffu;
	nxDword(p, 0x64) = 0xff7fffffu;
	nxDword(p, 0x68) = 0xff7fffffu;
	nxDword(p, 0x6c) = 0xff7fffffu;
	nxDword(p, 0x70) = 2;
	nxDword(p, 0x74) = 0;
	new (nxAt(p, 0x78)) SdkContainer();						// phys_fn_004836
	}

// phys_fn_002415 (0x0005bc10, phase 7). The 0xa8-byte auxiliary object. The
// oracle zeroes 30 dwords -- the first three of each 16-byte group from +0 to
// +0x98; +0x0c, +0x1c, ..., +0x9c and +0xa0 are left to the allocation -- and
// stores `owner` at +0xa4.
void* nxSceneAuxConstruct(void* self, void* owner)
	{
	unsigned* p = static_cast<unsigned*>(self);
	for(unsigned group = 0; group < 0xa0; group += 0x10)
		{
		nxDword(p, group + 0x0) = 0;
		nxDword(p, group + 0x4) = 0;
		nxDword(p, group + 0x8) = 0;
		}
	nxDword(p, 0xa4) = reinterpret_cast<unsigned>(owner);
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
// zeroes +8..+0x10 and +0x18..+0x20, then writes [this]=this+8 and
// [this+4]=this+0x18. +0x14 and +0x24 are not written.
static void nxSceneListInit(void* self)
	{
	unsigned* p = static_cast<unsigned*>(self);
	const unsigned base = reinterpret_cast<unsigned>(self);
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[6] = 0;
	p[7] = 0;
	p[8] = 0;
	p[0] = base + 8;
	p[1] = base + 0x18;
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
// Actor::loadFromDescInternal's use of the shape factory phys_fn_000032 (the
// factory and the runtime shapes are defined with the Task 4 chain below).
void* nxShapeFactory(void* shapeDesc, void* actor);
// The multi-shape group of the actor-creation path (0x110 bytes).
void* nxShapeGroupConstruct(void* actor, const unsigned* shapeDescriptions, unsigned count);


void nxActorSetName(void* actor, unsigned name);
void nxActorBuildBody(void* actor, const unsigned* desc);
int nxActorBuildRecord(unsigned char* body, const NxBodyDesc* desc);
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
// Actor.cpp's actor destructor body, row 000030 (defined below).
void nxActorDestroy(unsigned char* body);
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

// phys_fn_000647 (0x00012c10). Every store below is the listing's, at the byte
// offset the listing names, in the listing's order; the listing address of the
// first store of each run is given so the two can be read side by side.
NxSceneInternal::NxSceneInternal()
	{
	unsigned* p = reinterpret_cast<unsigned*>(mBytes);
	const unsigned base = reinterpret_cast<unsigned>(this);

	// 0x12c18: the vtable the oracle installs.
	nxDword(p, 0x000) = reinterpret_cast<unsigned>(vtable());

	// 0x12c21: +0x04..+0x28 zeroed.
	for(unsigned offset = 0x04; offset <= 0x28; offset += 4)
		nxDword(p, offset) = 0;

	nxSceneArrayHeaderInit(nxAt(p, 0x02c));					// phys_fn_004147
	new (nxAt(p, 0x050)) SdkContainer();					// phys_fn_004836
	nxSceneMemberE1510(nxAt(p, 0x060));						// phys_fn_005109

	nxDword(p, 0x0a8) = 0;										// 0x12c5f
	nxDword(p, 0x0ac) = 0;
	nxSceneMemberDE7E0(nxAt(p, 0x0b0));						// phys_fn_005071

	nxDword(p, 0x0f4) = 0;										// 0x12c70
	nxDword(p, 0x0f8) = 0;
	nxDword(p, 0x0fc) = 0;
	nxDword(p, 0x100) = 0;
	nxDword(p, 0x104) = 0;
	nxDword(p, 0x108) = 0;
	nxDword(p, 0x10c) = 0x3f8ccccdu;							// 1.1f
	nxSceneMemberD4D00(nxAt(p, 0x110));						// phys_fn_005029

	nxDword(p, 0x244) = 0;										// 0x12caa
	nxDword(p, 0x248) = 0;
	nxDword(p, 0x288) = 0x3f8ccccdu;							// 1.1f
	nxDword(p, 0x254) = 0;
	nxDword(p, 0x250) = 0;
	nxDword(p, 0x24c) = 0;
	nxDword(p, 0x260) = 0;
	nxDword(p, 0x25c) = 0;
	nxDword(p, 0x258) = 0;
	for(unsigned offset = 0x264; offset <= 0x284; offset += 4)	// 0x12cea
		nxDword(p, offset) = 0;
	nxDword(p, 0x284) = 0x3f800000u;							// 1.0f
	nxDword(p, 0x274) = 0x3f800000u;							// 1.0f
	nxDword(p, 0x264) = 0x3f800000u;							// 1.0f
	nxSceneMemberD3490(nxAt(p, 0x28c));						// phys_fn_004996

	nxDword(p, 0x30c) = 0;										// 0x12d24
	nxDword(p, 0x310) = 0;
	nxDword(p, 0x314) = 0;
	nxDword(p, 0x304) = 0;
	nxDword(p, 0x308) = 0;
	nxDword(p, 0x318) = 0;
	nxDword(p, 0x31c) = 0;
	nxDword(p, 0x320) = 0;
	nxDword(p, 0x324) = 0;
	nxDword(p, 0x328) = 0x3f8ccccdu;							// 1.1f
	nxSceneMemberBB510(nxAt(p, 0x32c));						// phys_fn_004938

	nxDword(p, 0x448) = 0;										// 0x12d84
	nxDword(p, 0x44c) = 0;
	nxDword(p, 0x440) = 0;
	nxDword(p, 0x444) = 1;
	nxSceneMemberB5720(nxAt(p, 0x450));						// phys_fn_004899
	new (nxAt(p, 0x4e0)) SdkContainer();					// phys_fn_004836
	new (nxAt(p, 0x4f0)) SdkContainer();
	new (nxAt(p, 0x500)) SdkContainer();
	new (nxAt(p, 0x510)) SdkContainer();

	nxDword(p, 0x52c) = 0x3dcccccdu;							// 0.1f, 0x12dcd
	nxDword(p, 0x530) = 10;
	nxDword(p, 0x534) = 0;
	nxDword(p, 0x538) = 0;
	nxDword(p, 0x53c) = 0;
	nxDword(p, 0x540) = 0;
	nxDword(p, 0x544) = 0;
	nxDword(p, 0x55c) = 0;										// 0x12dff
	nxDword(p, 0x560) = 0;
	nxDword(p, 0x564) = 0;
	nxDword(p, 0x56c) = 0;
	nxDword(p, 0x570) = 0;
	nxDword(p, 0x574) = 0;
	nxDword(p, 0x57c) = 0;
	nxDword(p, 0x580) = 0;
	nxDword(p, 0x584) = 0;
	nxDword(p, 0x58c) = 0;
	nxDword(p, 0x590) = 0;
	nxDword(p, 0x594) = 0;
	for(unsigned offset = 0x59c; offset <= 0x5cc; offset += 4)	// 0x12e4d
		nxDword(p, offset) = 0;
	nxDword(p, 0x5d0) = 0xffffffffu;
	nxSceneListInit(nxAt(p, 0x5d4));							// phys_fn_002346

	nxDword(p, 0x5fc) = 0;										// 0x12eaa
	nxDword(p, 0x600) = 0;
	nxDword(p, 0x604) = 0;
	nxDword(p, 0x60c) = 0;
	nxDword(p, 0x610) = 0;
	nxDword(p, 0x614) = 0;
	nxDword(p, 0x61c) = 0;
	nxDword(p, 0x620) = 0;
	nxSceneMember4CA30(nxAt(p, 0x624));						// phys_fn_001980

	for(unsigned offset = 0x6ac; offset <= 0x6dc; offset += 4)	// 0x12ee5
		nxDword(p, offset) = 0;
	nxDword(p, 0x6e4) = 0;							// next shape ID
	nxDword(p, 0x6e8) = 0;							// shape-ID recycle array
	nxDword(p, 0x6ec) = 0;
	nxDword(p, 0x6f0) = 0;
	nxDword(p, 0x6f8) = 0;							// next record ID
	nxDword(p, 0x6fc) = 0;							// record-ID recycle array
	nxDword(p, 0x700) = 0;
	nxDword(p, 0x704) = 0;
	nxDword(p, 0x70c) = 1;
	// 0x12f69 calls 0x0002ea70, whose body is a bare `ret`.

	nxDword(p, 0x528) = 0;										// 0x12f6e
	nxDword(p, 0x524) = 0;
	nxDword(p, 0x520) = 0;
	// 0x12f80: four fields take the address of the SdkContainer at +0x50 (the
	// listing's ebx, loaded at 0x12c47), not the Scene's own address.
	nxDword(p, SELF_0) = base + 0x50;
	nxDword(p, SELF_1) = base + 0x50;
	nxDword(p, SELF_2) = base + 0x50;
	nxDword(p, SELF_3) = base + 0x50;

	// phys_fn_000285: the Scene constructs and owns its public wrapper (0x12f98).
	NpScene* wrapper = new (NX_MEMORY_PERSISTENT) NpScene(this);
	nxDword(p, 0x6cc) = reinterpret_cast<unsigned>(wrapper);

	// phys_fn_002415, allocated 0xa8 bytes (0x12fbe).
	void* aux = nxGetSdkAllocator()->malloc(0xa8, NX_MEMORY_PERSISTENT);
	nxDword(p, 0x048) = aux ? reinterpret_cast<unsigned>(nxSceneAuxConstruct(aux, p)) : 0;
	}



// Reserves an embedded NxArraySDK<T> to `needed` entries through
// nxGetSdkAllocator() (the oracle's sequence uses the Foundation allocator,
// `[[0x101041bc]]`; that allocator gap is tracked separately). This follows the
// oracle's capacity-compare-then-grow sequence at 0x00013070: it tests `last` against `memEnd`, doubles when it must, copies
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

// Scene+0x6e4 is the next shape ID. Released IDs live in a LIFO array at
// +0x6e8: the two-shape oracle drive reuses 0 and 3 before issuing ID 4.
unsigned nxSceneTakeShapeId(NxSceneInternal* scene)
	{
	unsigned* first = scene->at<unsigned*>(0x6e8);
	unsigned* last = scene->at<unsigned*>(0x6ec);
	if(first && last != first)
		{
		--last;
		scene->at<unsigned*>(0x6ec) = last;
		return *last;
		}
	return scene->at<unsigned>(0x6e4)++;
	}

void nxSceneRecycleShapeId(NxSceneInternal* scene, unsigned id)
	{
	unsigned char* header = scene->bytes() + 0x6e8;
	nxSceneArrayReserve(header, 1);
	unsigned* last = scene->at<unsigned*>(0x6ec);
	if(!last) return;
	*last = id;
	scene->at<unsigned*>(0x6ec) = last + 1;
	}

void nxSceneRecycleActorId(NxSceneInternal* scene, unsigned id)
	{
	unsigned char* header = scene->bytes() + 0x6d4;
	nxSceneArrayReserve(header, 1);
	unsigned* last = scene->at<unsigned*>(0x6d8);
	if(!last) return;
	*last = id;
	scene->at<unsigned*>(0x6d8) = last + 1;
	}

// Dynamic-record IDs use the same LIFO vector layout at Scene+0x6fc,
// with the next fresh ID at +0x6f8. The record destructor returns its +0x11c
// ID through this vector before releasing the 0x260-byte record.
static unsigned nxSceneTakeRecordId(NxSceneInternal* scene)
	{
	unsigned* first = scene->at<unsigned*>(0x6fc);
	unsigned* last = scene->at<unsigned*>(0x700);
	if(first && last != first)
		{
		--last;
		scene->at<unsigned*>(0x700) = last;
		return *last;
		}
	return scene->at<unsigned>(0x6f8)++;
	}

static void nxSceneRecycleRecordId(NxSceneInternal* scene, unsigned id)
	{
	unsigned char* header = scene->bytes() + 0x6fc;
	nxSceneArrayReserve(header, 1);
	unsigned* last = scene->at<unsigned*>(0x700);
	if(!last) return;
	*last = id;
	scene->at<unsigned*>(0x700) = last + 1;
	}

// The first dynamic record initializes five 256-slot arrays in the Scene's
// 0xa8-byte auxiliary manager. Three are prepared through a temporary 0x800
// staging buffer, then copied to retained 0x400-byte arrays. This follows
// the oracle's allocation/free sequence and measured array headers. The
// per-record slots at +0x40/+0x50/+0x60/+0x80 are updated below.
static bool nxSceneAuxPrepareStagedArray(unsigned char* aux, unsigned offset,
	unsigned fill, unsigned firstValue)
	{
	unsigned* staging = static_cast<unsigned*>(
		nxGetSdkAllocator()->malloc(0x800, NX_MEMORY_PERSISTENT));
	if(!staging) return false;
	for(unsigned i = 0; i < 512; ++i) staging[i] = fill;
	staging[0] = firstValue;
	unsigned* retained = static_cast<unsigned*>(
		nxGetSdkAllocator()->malloc(0x400, NX_MEMORY_PERSISTENT));
	if(!retained)
		{
		nxGetSdkAllocator()->free(staging);
		return false;
		}
	memcpy(retained, staging, 0x400);
	nxGetSdkAllocator()->free(staging);
	*reinterpret_cast<unsigned**>(aux + offset) = retained;
	*reinterpret_cast<unsigned**>(aux + offset + 4) = retained + 256;
	*reinterpret_cast<unsigned**>(aux + offset + 8) = retained + 256;
	return true;
	}

void nxSceneAuxRegisterRecord(NxSceneInternal* scene, void* recordPointer)
	{
	unsigned char* aux = scene->at<unsigned char*>(0x48);
	unsigned char* record = static_cast<unsigned char*>(recordPointer);
	if(!aux || !record) return;
	if(!*reinterpret_cast<void**>(aux + 0x80))
		{
		if(!nxSceneAuxPrepareStagedArray(aux, 0x80, 0,
			reinterpret_cast<unsigned>(record + 0x18))) return;
		if(!nxSceneAuxPrepareStagedArray(aux, 0x40, 0, 0xffffffffu)) return;
		if(!nxSceneAuxPrepareStagedArray(aux, 0x60, 0xd00beed0u, 0)) return;
		unsigned* active = static_cast<unsigned*>(
			nxGetSdkAllocator()->malloc(0x400, NX_MEMORY_PERSISTENT));
		if(!active) return;
		memset(active, 0, 0x400);
		*reinterpret_cast<unsigned**>(aux + 0x50) = active;
		*reinterpret_cast<unsigned**>(aux + 0x54) = active + 1;
		*reinterpret_cast<unsigned**>(aux + 0x58) = active + 256;
		unsigned* vacant = static_cast<unsigned*>(
			nxGetSdkAllocator()->malloc(0x400, NX_MEMORY_PERSISTENT));
		if(!vacant) return;
		memset(vacant, 0, 0x400);
		*reinterpret_cast<unsigned**>(aux + 0x70) = vacant;
		*reinterpret_cast<unsigned**>(aux + 0x74) = vacant;
		*reinterpret_cast<unsigned**>(aux + 0x78) = vacant + 256;
		return;
		}
	unsigned* active = *reinterpret_cast<unsigned**>(aux + 0x50);
	unsigned* activeEnd = *reinterpret_cast<unsigned**>(aux + 0x54);
	if(!active || !activeEnd || activeEnd - active >= 256) return;
	unsigned* occupied = *reinterpret_cast<unsigned**>(aux + 0x40);
	unsigned slot = 0;
	while(slot < 256 && occupied[slot]) ++slot;
	if(slot == 256) return;
	const unsigned activeIndex = static_cast<unsigned>(activeEnd - active);
	(*reinterpret_cast<unsigned**>(aux + 0x40))[slot] = 0xffffffffu;
	active[activeIndex] = slot;
	(*reinterpret_cast<unsigned**>(aux + 0x60))[slot] = activeIndex;
	(*reinterpret_cast<unsigned**>(aux + 0x80))[slot] =
		reinterpret_cast<unsigned>(record + 0x18);
	*reinterpret_cast<unsigned**>(aux + 0x54) = activeEnd + 1;
	}

// Every internal shape, including a multi-shape group, occupies a physical
// slot in the Scene's first auxiliary table. The table uses the same five-array
// layout as dynamic records, but is initialized by the first shape.
static void nxSceneAuxRegisterShape(NxSceneInternal* scene, void* shapePointer)
	{
	unsigned char* aux = scene->at<unsigned char*>(0x48);
	if(!aux || !shapePointer) return;
	if(!*reinterpret_cast<void**>(aux + 0x90))
		{
		if(!nxSceneAuxPrepareStagedArray(aux, 0x90, 0,
			reinterpret_cast<unsigned>(shapePointer))) return;
		if(!nxSceneAuxPrepareStagedArray(aux, 0, 0, 0xffffffffu)) return;
		if(!nxSceneAuxPrepareStagedArray(aux, 0x20, 0xd00beed0u, 0)) return;
		unsigned* active = static_cast<unsigned*>(
			nxGetSdkAllocator()->malloc(0x400, NX_MEMORY_PERSISTENT));
		if(!active) return;
		memset(active, 0, 0x400);
		*reinterpret_cast<unsigned**>(aux + 0x10) = active;
		*reinterpret_cast<unsigned**>(aux + 0x14) = active + 1;
		*reinterpret_cast<unsigned**>(aux + 0x18) = active + 256;
		unsigned* vacant = static_cast<unsigned*>(
			nxGetSdkAllocator()->malloc(0x400, NX_MEMORY_PERSISTENT));
		if(!vacant) return;
		memset(vacant, 0, 0x400);
		*reinterpret_cast<unsigned**>(aux + 0x30) = vacant;
		*reinterpret_cast<unsigned**>(aux + 0x34) = vacant;
		*reinterpret_cast<unsigned**>(aux + 0x38) = vacant + 256;
		return;
		}
	unsigned* active = *reinterpret_cast<unsigned**>(aux + 0x10);
	unsigned* activeEnd = *reinterpret_cast<unsigned**>(aux + 0x14);
	if(!active || !activeEnd || activeEnd - active >= 256) return;
	unsigned* flags = *reinterpret_cast<unsigned**>(aux);
	const unsigned slot = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(shapePointer) + 0xd4);
	if(slot >= 256 || flags[slot]) return;
	const unsigned activeIndex = static_cast<unsigned>(activeEnd - active);
	flags[slot] = 0xffffffffu;
	active[activeIndex] = slot;
	(*reinterpret_cast<unsigned**>(aux + 0x20))[slot] = activeIndex;
	(*reinterpret_cast<unsigned**>(aux + 0x90))[slot] =
		reinterpret_cast<unsigned>(shapePointer);
	*reinterpret_cast<unsigned**>(aux + 0x14) = activeEnd + 1;
	}

static void nxSceneAuxUnregisterShape(NxSceneInternal* scene, void* shapePointer)
	{
	unsigned char* aux = scene->at<unsigned char*>(0x48);
	if(!aux || !shapePointer) return;
	unsigned* active = *reinterpret_cast<unsigned**>(aux + 0x10);
	unsigned* activeEnd = *reinterpret_cast<unsigned**>(aux + 0x14);
	unsigned* shapes = *reinterpret_cast<unsigned**>(aux + 0x90);
	if(!active || !activeEnd || !shapes || active == activeEnd) return;
	const unsigned count = static_cast<unsigned>(activeEnd - active);
	unsigned activeIndex = 0;
	while(activeIndex < count && shapes[active[activeIndex]] !=
		reinterpret_cast<unsigned>(shapePointer)) ++activeIndex;
	if(activeIndex == count) return;
	const unsigned slot = active[activeIndex];
	const unsigned last = count - 1;
	unsigned* flags = *reinterpret_cast<unsigned**>(aux);
	unsigned* indices = *reinterpret_cast<unsigned**>(aux + 0x20);
	if(activeIndex != last)
		{
		active[activeIndex] = active[last];
		indices[active[activeIndex]] = activeIndex;
		}
	flags[slot] = 0;
	indices[slot] = 0xd00beed0u;
	shapes[slot] = 0;
	*reinterpret_cast<unsigned**>(aux + 0x14) = activeEnd - 1;
	}

// phys_fn_10026c90 follows shape+4 -> outer body+4 -> Scene+0x48. The
// registration path normally leaves flags[id] at all ones; a cleared entry is
// queued before its requested dirty bits are set.
void nxSceneMarkShapeDirty(void* shapePointer, unsigned flag)
	{
	unsigned char* shape = static_cast<unsigned char*>(shapePointer);
	if(!shape) return;
	unsigned char* body = *reinterpret_cast<unsigned char**>(shape + 4);
	if(!body) return;
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	if(!scene) return;
	unsigned char* aux = scene->at<unsigned char*>(0x48);
	if(!aux) return;
	unsigned* flags = *reinterpret_cast<unsigned**>(aux);
	const unsigned id = *reinterpret_cast<unsigned*>(shape + 0xd4);
	if(!flags || id >= 256) return;
	if(!flags[id])
		{
		unsigned* active = *reinterpret_cast<unsigned**>(aux + 0x10);
		unsigned* end = *reinterpret_cast<unsigned**>(aux + 0x14);
		unsigned* capacity = *reinterpret_cast<unsigned**>(aux + 0x18);
		if(!active || !end || !capacity) return;
		if(end == capacity)
			{
			const unsigned count = static_cast<unsigned>(end - active);
			const unsigned next = count * 2 + 2;
			unsigned* grown = static_cast<unsigned*>(nxGetSdkAllocator()->malloc(
				next * sizeof(unsigned), NX_MEMORY_PERSISTENT));
			if(!grown) return;
			memcpy(grown, active, count * sizeof(unsigned));
			nxGetSdkAllocator()->free(active);
			active = grown;
			end = grown + count;
			*reinterpret_cast<unsigned**>(aux + 0x10) = active;
			*reinterpret_cast<unsigned**>(aux + 0x18) = grown + next;
			}
		const unsigned index = static_cast<unsigned>(end - active);
		active[index] = id;
		*reinterpret_cast<unsigned**>(aux + 0x14) = end + 1;
		(*reinterpret_cast<unsigned**>(aux + 0x20))[id] = index;
		}
	flags[id] |= flag;
	}

void nxSceneAuxUnregisterRecord(NxSceneInternal* scene, void* recordPointer)
	{
	unsigned char* aux = scene->at<unsigned char*>(0x48);
	unsigned char* record = static_cast<unsigned char*>(recordPointer);
	if(!aux || !record) return;
	unsigned* active = *reinterpret_cast<unsigned**>(aux + 0x50);
	unsigned* activeEnd = *reinterpret_cast<unsigned**>(aux + 0x54);
	unsigned* records = *reinterpret_cast<unsigned**>(aux + 0x80);
	if(!active || !activeEnd || !records || active == activeEnd) return;
	const unsigned count = static_cast<unsigned>(activeEnd - active);
	unsigned activeIndex = 0;
	while(activeIndex < count && records[active[activeIndex]] !=
		reinterpret_cast<unsigned>(record + 0x18)) ++activeIndex;
	if(activeIndex == count) return;
	const unsigned slot = active[activeIndex];
	const unsigned last = count - 1;
	unsigned* occupied = *reinterpret_cast<unsigned**>(aux + 0x40);
	unsigned* indices = *reinterpret_cast<unsigned**>(aux + 0x60);
	if(activeIndex != last)
		{
		active[activeIndex] = active[last];
		indices[active[activeIndex]] = activeIndex;
		}
	occupied[slot] = 0;
	indices[slot] = 0xd00beed0u;
	records[slot] = 0;
	*reinterpret_cast<unsigned**>(aux + 0x54) = activeEnd - 1;
	}

// OPCODE's pool is initialized by the first pruning object, whether static or
// dynamic. Its process-wide header owns four initial buffers.
static unsigned char* gNxOpcodePool = 0;

static bool nxOpcodeEnsurePool()
	{
	if(gNxOpcodePool) return true;
	gNxOpcodePool = static_cast<unsigned char*>(nxGetSdkAllocator()->malloc(
		0x1c, NX_MEMORY_PERSISTENT));
	if(!gNxOpcodePool) return false;
	memset(gNxOpcodePool, 0, 0x1c);
	const unsigned sizes[4] = {8, 4, 4, 4};
	const unsigned offsets[4] = {0, 0xc, 0x10, 0x14};
	for(unsigned i = 0; i < 4; ++i)
		{
		void* block = nxGetSdkAllocator()->malloc(sizes[i],
			NX_MEMORY_PERSISTENT);
		if(!block) return false;
		memset(block, i == 1 || i == 2 ? 0xff : 0, sizes[i]);
		*reinterpret_cast<void**>(gNxOpcodePool + offsets[i]) = block;
		}
	*reinterpret_cast<unsigned*>(gNxOpcodePool + 8) = 2;
	return true;
	}

void nxOpcodeReleasePool()
	{
	if(!gNxOpcodePool) return;
	const unsigned offsets[4] = {0x14, 0x10, 0xc, 0};
	for(unsigned offset : offsets)
		{
		void*& block = *reinterpret_cast<void**>(gNxOpcodePool + offset);
		if(block)
			{
			nxGetSdkAllocator()->free(block);
			block = 0;
			}
		}
	nxGetSdkAllocator()->free(gNxOpcodePool);
	gNxOpcodePool = 0;
	}

// The oracle's dynamic broadphase table is a 0x3c-byte object stored at
// Scene+0x648. Its subcontainer begins at +4: count and capacity are the
// 16-bit words at +0x10/+0x12, followed by parallel 0x18-byte-entry and
// 4-byte-reference buffers at +0x14/+0x18. A single dynamic shape registers
// one entry; a two-shape group registers its group plus both children.
// The entry payload and the table's other fields remain to be reconstructed.
static void nxSceneTrackShape(NxSceneInternal* scene, unsigned char* shape)
	{
	unsigned& count = scene->at<unsigned>(0x6a0);
	unsigned& capacity = scene->at<unsigned>(0x69c);
	void**& entries = scene->at<void**>(0x6a4);
	if(count == capacity)
		{
		const unsigned next = capacity ? capacity * 2 : 2;
		void** grown = static_cast<void**>(nxGetSdkAllocator()->malloc(
			next * sizeof(void*), NX_MEMORY_PERSISTENT));
		if(!grown) return;
		memset(grown, 0, next * sizeof(void*));
		if(entries)
			{
			memcpy(grown, entries, count * sizeof(void*));
			nxGetSdkAllocator()->free(entries);
			}
		entries = grown;
		capacity = next;
		}
	entries[count++] = shape;
	}

static void nxSceneUntrackShape(NxSceneInternal* scene, unsigned char* shape)
	{
	void** entries = scene->at<void**>(0x6a4);
	unsigned& count = scene->at<unsigned>(0x6a0);
	for(unsigned i = 0; entries && i < count; ++i)
		if(entries[i] == shape)
			{
			entries[i] = entries[--count];
			return;
			}
	}

static void nxSceneInvalidateBroadphaseEntry(unsigned char* table,
	unsigned char* entries, unsigned index)
	{
	static const unsigned emptyBounds[6] = {
		0x7f7fffffu, 0x7f7fffffu, 0x7f7fffffu,
		0xff7fffffu, 0xff7fffffu, 0xff7fffffu };
	memcpy(entries + index * 0x18, emptyBounds, sizeof(emptyBounds));
	++*reinterpret_cast<unsigned*>(table + 0x38);
	}

// ---------------------------------------------------------------------------
// The candidate's model of the pruning collection at Scene+0x624 (phys_fn_001980
// builds it). Its pruners sit at +0x1c + 4 type: the static one (type 0,
// Scene+0x640, a 0x90-byte object) and the dynamic one (type 2, Scene+0x648, a
// 0x3c-byte object); +0x70 holds the type a dynamic prunable takes (2). A
// shape's prunable is the sub-object at +0xa4: +0xc4 its pruner, +0xcc its
// handle (0xffff when it has none), +0xce its type (phys_fn_004888), +0xcf its
// kind (phys_fn_004890: 2 for a group root, 1 for a single root, 0 for a
// group child). The OPCODE pruners themselves (phys_fn_004852, 004857, 004859
// and the pruner classes) belong to the opcode units and are not transcribed:
// the model keeps what the staged pairs observe -- the allocation sizes, the
// growth points (4 entries, then doubling, when an insertion would exceed the
// capacity) and the count 001957/001960 read. That count, +8 of the pruner
// (+0xc stays 0 in the model), is the number of prunables with a nonzero
// kind, one per actor root: the oracle keeps it at one per actor through
// promotion and group appends. Entry order and handle assignment are the
// model's (append, swap-with-last removal), not verified against OPCODE.
// ---------------------------------------------------------------------------

// Creates the pruner of a type on first use, with the allocations the
// candidate measured: the static pruner's 0x90-byte object, the OPCODE pool
// and its first four entries; the dynamic pruner's 0x3c-byte object and the
// pool (its entries come with the first insertion).
static unsigned char* nxScenePrunerFor(NxSceneInternal* scene, unsigned type)
	{
	if(type == 0)
		{
		unsigned char*& manager = scene->at<unsigned char*>(0x640);
		if(manager) return manager;
		manager = static_cast<unsigned char*>(nxGetSdkAllocator()->malloc(
			0x90, NX_MEMORY_PERSISTENT));
		if(!manager) return 0;
		memset(manager, 0, 0x90);
		if(!nxOpcodeEnsurePool()) return 0;
		void* entries = nxGetSdkAllocator()->malloc(0x60, NX_MEMORY_PERSISTENT);
		void* references = nxGetSdkAllocator()->malloc(0x10, NX_MEMORY_PERSISTENT);
		if(!entries || !references) return 0;
		memset(entries, 0, 0x60);
		memset(references, 0, 0x10);
		*reinterpret_cast<void**>(manager + 0x14) = entries;
		*reinterpret_cast<void**>(manager + 0x18) = references;
		*reinterpret_cast<unsigned short*>(manager + 0x12) = 4;
		for(unsigned i = 0; i < 3; ++i)
			{
			*reinterpret_cast<unsigned*>(manager + 0x1c + i * 4) = 0x7f7fffffu;
			*reinterpret_cast<unsigned*>(manager + 0x28 + i * 4) = 0xff7fffffu;
			}
		*reinterpret_cast<unsigned*>(manager + 0x4c) = 0xbf800000u;
		*reinterpret_cast<unsigned*>(manager + 0x68) = 0x3f8ccccdu;
		*reinterpret_cast<unsigned*>(manager + 0x8c) = 0x3f8ccccdu;
		*reinterpret_cast<void**>(manager + 0x50) = manager + 0x40;
		return manager;
		}
	if(type == 2)
		{
		unsigned char*& table = scene->at<unsigned char*>(0x648);
		if(table) return table;
		table = static_cast<unsigned char*>(
			nxGetSdkAllocator()->malloc(0x3c, NX_MEMORY_PERSISTENT));
		if(!table) return 0;
		memset(table, 0, 0x3c);
		if(!nxOpcodeEnsurePool()) return 0;
		return table;
		}
	return 0;
	}

// Grows a pruner's parallel 0x18-byte entry and pointer arrays so `needed`
// entries fit: 4 when empty, else doubled. Both new blocks are allocated
// before the old ones are released (entries first each time).
static bool nxScenePrunerReserve(unsigned char* table, unsigned needed)
	{
	unsigned short& count = *reinterpret_cast<unsigned short*>(table + 0x10);
	unsigned short& capacity = *reinterpret_cast<unsigned short*>(table + 0x12);
	if(needed <= capacity) return true;
	unsigned next = capacity ? capacity * 2u : 4u;
	while(next < needed) next *= 2u;
	unsigned char* entries = static_cast<unsigned char*>(
		nxGetSdkAllocator()->malloc(next * 0x18, NX_MEMORY_PERSISTENT));
	void** references = static_cast<void**>(
		nxGetSdkAllocator()->malloc(next * sizeof(void*), NX_MEMORY_PERSISTENT));
	if(!entries || !references)
		{
		if(entries) nxGetSdkAllocator()->free(entries);
		if(references) nxGetSdkAllocator()->free(references);
		return false;
		}
	memset(entries, 0, next * 0x18);
	memset(references, 0, next * sizeof(void*));
	unsigned char* oldEntries = *reinterpret_cast<unsigned char**>(table + 0x14);
	void** oldReferences = *reinterpret_cast<void***>(table + 0x18);
	if(oldEntries) memcpy(entries, oldEntries, count * 0x18);
	if(oldReferences) memcpy(references, oldReferences, count * sizeof(void*));
	if(oldEntries) nxGetSdkAllocator()->free(oldEntries);
	if(oldReferences) nxGetSdkAllocator()->free(oldReferences);
	*reinterpret_cast<unsigned char**>(table + 0x14) = entries;
	*reinterpret_cast<void***>(table + 0x18) = references;
	capacity = static_cast<unsigned short>(next);
	return true;
	}

// The model of phys_fn_004857 (0x000b5180): a prunable without a handle
// whose type (+0xce, set before the call) is below 4 is added to the pruner
// of its type, created on first use; the add ends with the slot-3 update the
// model counts at +0x38. The static pruner's entry starts zeroed, the
// dynamic one's with the empty bounds.
static void nxScenePrunerInsert(NxSceneInternal* scene, unsigned char* shape)
	{
	if(*reinterpret_cast<unsigned short*>(shape + 0xcc) != 0xffffu) return;
	const unsigned type = shape[0xce];
	if(type >= 4) return;
	unsigned char* table = nxScenePrunerFor(scene, type);
	if(!table) return;
	unsigned short& count = *reinterpret_cast<unsigned short*>(table + 0x10);
	if(!nxScenePrunerReserve(table, count + 1u)) return;
	unsigned char* entries = *reinterpret_cast<unsigned char**>(table + 0x14);
	void** references = *reinterpret_cast<void***>(table + 0x18);
	const unsigned index = count;
	references[index] = shape + 0xa4;
	*reinterpret_cast<void**>(shape + 0xc4) = table;
	*reinterpret_cast<unsigned short*>(shape + 0xcc) = static_cast<unsigned short>(index);
	if(type == 2)
		nxSceneInvalidateBroadphaseEntry(table, entries, index);
	else
		{
		memset(entries + index * 0x18, 0, 0x18);
		++*reinterpret_cast<unsigned*>(table + 0x38);
		*reinterpret_cast<void**>(table + 0x48) = scene->at<void*>(0xc);
		*reinterpret_cast<unsigned*>(table + 0x40) = scene->at<unsigned>(4);
		}
	count = static_cast<unsigned short>(index + 1);
	if(shape[0xcf])
		++*reinterpret_cast<unsigned*>(table + 8);
	}

// The model of phys_fn_004859 (0x000b5260): the prunable leaves its pruner
// (found by search, so a stale handle cannot remove another shape's entry),
// the last entry moving into its place.
static void nxScenePrunerErase(NxSceneInternal* scene, unsigned char* shape)
	{
	if(!shape) return;
	const unsigned type = shape[0xce];
	unsigned char* table = type == 0 ? scene->at<unsigned char*>(0x640)
		: type == 2 ? scene->at<unsigned char*>(0x648) : 0;
	if(!table) return;
	unsigned short& count = *reinterpret_cast<unsigned short*>(table + 0x10);
	void** references = *reinterpret_cast<void***>(table + 0x18);
	unsigned char* entries = *reinterpret_cast<unsigned char**>(table + 0x14);
	unsigned index = 0;
	while(index < count && references[index] != shape + 0xa4) ++index;
	if(index == count) return;
	const unsigned last = count - 1u;
	if(index != last)
		{
		references[index] = references[last];
		memcpy(entries + index * 0x18, entries + last * 0x18, 0x18);
		unsigned char* moved = static_cast<unsigned char*>(references[index]) - 0xa4;
		*reinterpret_cast<unsigned short*>(moved + 0xcc) =
			static_cast<unsigned short>(index);
		}
	count = static_cast<unsigned short>(last);
	if(shape[0xcf])
		--*reinterpret_cast<unsigned*>(table + 8);
	if(type == 0)
		++*reinterpret_cast<unsigned*>(table + 0x38);
	*reinterpret_cast<unsigned short*>(shape + 0xcc) = 0xffffu;
	}

// The actor-creation registration of a dynamic body (this file's model of
// the 000034 -> 000531 path). The root and a group's children enter the
// dynamic pruner with the prunable kinds of phys_fn_001943, and the root
// takes the pruning collection at +0xa0 and joins its +0x78 list; the entry
// layout (children first) is this model's, kept from the measured group
// actors.
void nxSceneBroadphaseRegister(NxSceneInternal* scene, void* bodyPointer)
	{
	unsigned char* body = static_cast<unsigned char*>(bodyPointer);
	if(!body || !*reinterpret_cast<void**>(body + 8)) return;
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(!shape) return;
	const bool group = *reinterpret_cast<unsigned*>(shape + 0xd0) == 5u;
	const unsigned childCount = group ? static_cast<unsigned>(
		*reinterpret_cast<void***>(shape + 0xe4) -
		*reinterpret_cast<void***>(shape + 0xe0)) : 0;
	const unsigned added = group ? childCount + 1 : 1;
	unsigned char* table = nxScenePrunerFor(scene, 2);
	if(!table) return;
	unsigned short& count = *reinterpret_cast<unsigned short*>(table + 0x10);
	if(!nxScenePrunerReserve(table, static_cast<unsigned>(count) + added)) return;
	void** references = *reinterpret_cast<void***>(table + 0x18);
	unsigned char* entries = *reinterpret_cast<unsigned char**>(table + 0x14);
	if(group)
		{
		memmove(references + childCount, references, count * sizeof(void*));
		memmove(entries + childCount * 0x18, entries, count * 0x18);
		}
	for(unsigned i = 0; i < childCount; ++i)
		{
		unsigned char* object = static_cast<unsigned char*>(
			(*reinterpret_cast<void***>(shape + 0xe0))[i]);
		references[i] = object + 0xa4;
		*reinterpret_cast<void**>(object + 0xc4) = table;
		*reinterpret_cast<unsigned short*>(object + 0xcc) =
			static_cast<unsigned short>(i);
		*reinterpret_cast<unsigned char*>(object + 0xce) = 2;
		*reinterpret_cast<unsigned char*>(object + 0xcf) = 0;
		nxSceneInvalidateBroadphaseEntry(table, entries, i);
		}
	references[count + childCount] = shape + 0xa4;
	*reinterpret_cast<void**>(shape + 0xc4) = table;
	*reinterpret_cast<unsigned short*>(shape + 0xcc) =
		static_cast<unsigned short>(count + childCount);
	*reinterpret_cast<unsigned char*>(shape + 0xce) = 2;
	*reinterpret_cast<unsigned char*>(shape + 0xcf) = group ? 2 : 1;
	*reinterpret_cast<void**>(shape + 0xa0) = scene->bytes() + 0x624;
	nxSceneInvalidateBroadphaseEntry(table, entries, count + childCount);
	count = static_cast<unsigned short>(count + added);
	++*reinterpret_cast<unsigned*>(table + 8);
	nxSceneTrackShape(scene, shape);
	}

// The actor-release removal of a body's shapes: the root leaves the +0x78
// list, then the root and each current child leave their pruner (this
// file's model of phys_fn_001955 and phys_fn_001945, which 000533/000535
// reach through 001279). A static root goes through the same removal.
void nxSceneBroadphaseUnregister(NxSceneInternal* scene, void* bodyPointer)
	{
	unsigned char* body = static_cast<unsigned char*>(bodyPointer);
	if(!body) return;
	if(!*reinterpret_cast<void**>(body + 8))
		{
		nxSceneStaticPrunerUnregister(scene,
			*reinterpret_cast<unsigned char**>(body + 0x10));
		return;
		}
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(!shape) return;
	nxSceneUntrackShape(scene, shape);
	nxScenePrunerErase(scene, shape);
	if(*reinterpret_cast<unsigned*>(shape + 0xd0) == 5u)
		{
		void** child = *reinterpret_cast<void***>(shape + 0xe0);
		void** end = *reinterpret_cast<void***>(shape + 0xe4);
		for(; child && child != end; ++child)
			nxScenePrunerErase(scene, static_cast<unsigned char*>(*child));
		}
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
	// oracle does not store userData there: descriptor word 0x10 is
	// userData and reaches the actor through a different field, which this
	// transcription has not identified. Writing it here would clobber the body.
	(void)d[0x10];

	// The body. Scene::createJoint and the joint-descriptor rows both reach it
	// through actor+0x14, so it is built here rather than left to the shape path.
	nxActorBuildBody(actor, d);
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	if(body)
		{
		*reinterpret_cast<unsigned*>(body + 0x14) = d[0x0e];
		// Actor::saveToDesc reads the density back from this body word.
		*reinterpret_cast<unsigned*>(body + 0x18) = d[0x0d];
		*reinterpret_cast<unsigned short*>(body + 0x1c) =
			*reinterpret_cast<const unsigned short*>(d + 0x0f);
		}

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
	// and the multi-shape case allocates a 0x110-byte group instead. Both paths
	// still have unmodelled shape semantics; the group now owns the observed
	// parallel shape and helper arrays for a two-box actor.
	const unsigned shapeCount = (d[0x14] - d[0x13]) >> 2;
	if(d[0x12] == 1 || d[0x12] == 2)
		{
		if(shapeCount == 1)
			{
			void* shape = nxShapeFactory(
				reinterpret_cast<void*>(*reinterpret_cast<const unsigned*>(d[0x13])), actor);
			a[0x10 / 4] = reinterpret_cast<unsigned>(shape);
			if(!shape)
					return 0;
			}
		else if(shapeCount > 1)
			{
			// The multi-shape group. The oracle allocates 0x110 bytes and links each
			// shape through the group-owned arrays at +0xe0 and +0xf0.
			void* group = nxShapeGroupConstruct(actor,
				reinterpret_cast<const unsigned*>(d[0x13]), shapeCount);
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

	// 000026 on the body (0x21cd), with the descriptor's body. Its record
	// reaches the Scene's +0x56c array through 000630 (a dynamic actor built
	// without shapes too: the push and 000503 precede the public actor
	// array's growth, NpActor.cpp completion Task 4, t4_bare_create). 1 and
	// any other nonzero result are the Actor.cpp reports 0xe5 and 0xe6.
	const int mass = nxActorBuildRecord(body,
		reinterpret_cast<const NxBodyDesc*>(d[0x0c]));
	if(mass == 1)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\Actor.cpp", 0xe5, 0,
			"Actor::loadFromDescInternal: Compute mesh inertia tensor "
			"failed for one of the actor's mesh shapes! Please change mesh geometry or "
			"supply a tensor manually!");
		return 0;
		}
	if(mass == 0)
		{
		nxSceneAddActorObject(reinterpret_cast<void*>(a[1]),
			reinterpret_cast<void*>(a[0x10 / 4]), actor);
		return 1;
		}
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\Actor.cpp", 0xe6, 0,
		"Actor::loadFromDescInternal: Can't compute mass from shapes: "
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
		// 000030 on the body (it deletes the public actor through its
		// slot 0), then the body freed through [0x101041bc].
		nxActorDestroy(static_cast<unsigned char*>(outerMemory));
		nxFoundationSDKAllocator->free(outerMemory);
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\Scene.cpp", 0x228, 0,
			"Actor Initialisation failed: returned NULL.");
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

// phys_fn_000628 (0x000123d0, 241 B)
// Scene::releaseActor(body). Under the API reentry flag (.data 0x10123c10;
// set: code 2, line 0x492, the message at 0x10122050): the public actor
// ([body]) is searched for in the +0x55c array; not found is code 2, line
// 0x4ae, "Scene::releaseActor: double deletion detected!". Found: the last
// entry takes its place and the array shrinks; a fluid manager at +0x61c
// takes 003635 (0x1248c-0x12497), which is NOT written: 003635 walks the
// manager's fluids through 003485 into the emitter rows 003593/003622, none
// of them written, and +0x61c is only set by createFluid (000645/000400),
// which the candidate stubs. The row stays `discovered` until that chain is
// (NpActor completion final review I2). Actor.cpp's 000030 destroys the
// actor and the body is freed through [0x101041bc]; the flag is cleared.
void nxActorDestroy(unsigned char* body);

// .data 0x10123c10: the one API reentry flag. Scene::createJoint and
// releaseJoint, Scene::releaseActor and Actor.cpp's createShape and
// releaseShape (000036/000024) test, set and clear it.
static bool gNxApiReentry = false;

void NxSceneInternal::releaseActor(void* bodyPointer)
	{
	if(gNxApiReentry)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\Scene.cpp", 0x492, 0,
			"Reentry check: You may not call this API method from a callback!");
		return;
		}
	gNxApiReentry = true;
	unsigned char* body = static_cast<unsigned char*>(bodyPointer);
	NxActor* actor = *reinterpret_cast<NxActor**>(body);
	NxActor** first = at<NxActor**>(0x55c);
	const unsigned count = static_cast<unsigned>(at<NxActor**>(0x560) - first);
	unsigned index = 0;
	while(index < count && first[index] != actor)
		++index;
	if(index == count)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\Scene.cpp", 0x4ae, 0,
			"Scene::releaseActor: double deletion detected!");
		gNxApiReentry = false;
		return;
		}
	if(index != count - 1)
		first[index] = at<NxActor**>(0x560)[-1];
	--at<NxActor**>(0x560);
	nxActorDestroy(body);
	nxFoundationSDKAllocator->free(body);
	gNxApiReentry = false;
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
//   a switch on descriptor word 1 (the joint type) through the table at
//     0x14590. Case 0 (NX_JOINT_PRISMATIC) allocates 0x17c bytes and constructs
//     through phys_fn_004380 (0xad6e0); case 1 (NX_JOINT_REVOLUTE, target
//     0x143c2) allocates 0x204 bytes (0x143ce) and constructs through
//     phys_fn_004366 (0x143e1), which builds the 0x1c-byte NpRevoluteJoint at
//     internal +0x48;
//   then the joint's byte +0x48 (0x14502 `mov eax,[esi+0x48]`): if null, the
//     joint's slot 5 (scalar deleting destructor) with 1 and a return of 0
//     (0x14581-0x1458c); otherwise [[Scene+0x6cc]+0xc] -> np+0x10 and
//     [[Scene+0x6cc]+0x10] -> np+0x14 (0x14509-0x14521) and phys_fn_000661
//     to register it (0x14524);
//   on every exit after the switch, ++[Scene+0x6c8] and [Scene+0x6bc] =
//     [Scene+0x59c] (0x14529-0x1453f) before the re-entry flag is cleared.
//     A type above 9 takes the switch's `ja 0x14529` (0x1438a-0x1438d) straight
//     to that tail with esi = 0, so it returns 0 without allocating.
//
// All ten joint types (prismatic, revolute, cylindrical, spherical,
// point-on-line, point-in-plane, distance, pulley, fixed and D6) run the
// reconstructed rows (core/PrismaticJoint.cpp,
// core/NpPrismaticJoint.cpp, core/RevoluteJoint.cpp, core/NpRevoluteJoint.cpp,
// core/CylindricalJoint.cpp, core/NpCylindricalJoint.cpp,
// core/SphericalJoint.cpp, core/NpSphericalJoint.cpp,
// core/PointOnLineJoint.cpp, core/NpPointOnLineJoint.cpp,
// core/PointInPlaneJoint.cpp, core/NpPointInPlaneJoint.cpp,
// core/DistanceJoint.cpp, core/NpDistanceJoint.cpp, core/PulleyJoint.cpp,
// core/NpPulleyJoint.cpp, core/FixedJoint.cpp, core/NpFixedJoint.cpp,
// core/D6Joint.cpp, core/NpD6Joint.cpp). A type outside the ten allocates
// nothing and runs the same tail, as the oracle's switch default does (the
// generic stand-in that used to serve unreconstructed types was removed by
// joint-open-items Task 1). The oracle's Scene::createJoint
// returns the internal joint and its NpScene::createJoint (phys_fn_000297)
// returns [internal+0x48]; here that load is done at the end of this function,
// so NpScene::createJoint keeps returning what this returns for every type.
// ---------------------------------------------------------------------------

// .data 0x10123c10 is gNxApiReentry (defined above Scene::releaseActor).

NxJoint* NxSceneInternal::createJoint(const NxJointDesc& desc)
	{
	unsigned* p = reinterpret_cast<unsigned*>(mBytes);
	const unsigned* d = reinterpret_cast<const unsigned*>(&desc);

	if(gNxApiReentry)
		{
		nxSceneReportErrorA("Reentry check: You may not call this function "
			"recursively. Scene.cpp:0x245");
		return 0;
		}
	gNxApiReentry = true;

	// desc.isValid() is the pinned header's own inline predicate, reached through
	// the descriptor's vtable in the oracle and called directly here.
	if(!desc.isValid())
		{
		nxSceneReportErrorA("PhysicsSDK::createJoint: desc.isValid() fails!");
		gNxApiReentry = false;
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
		gNxApiReentry = false;
		return 0;
		}

	// The reconstructed families. Each case is the oracle's switch arm: the
	// Foundation allocator's ([[0x101041bc]]) slot +8 with (size, 0), the family constructor on the block
	// (null on allocation failure), then the shared tail at 0x144fc.
	//   NX_JOINT_PRISMATIC: case 0, target 0x1439a; (0x17c, 0) at 0x143a3-0x143aa,
	//     phys_fn_004380 at 0x143b8 (joint-families Task 3a).
	//   NX_JOINT_REVOLUTE: case 1, target 0x143c2; (0x204, 0) at 0x143cc-0x143d3,
	//     phys_fn_004366 at 0x143e1 (revolute pilot, Task 10).
	//   NX_JOINT_CYLINDRICAL: case 2, target 0x143eb; (0x16c, 0) at 0x143f5-0x143fc,
	//     phys_fn_004320 at 0x1440a (joint-families Task 3b).
	//   NX_JOINT_SPHERICAL: case 3, target 0x14414; (0x23c, 0) at 0x1441e-0x14425,
	//     phys_fn_004300 at 0x14433 (joint-families Task 3c).
	//   NX_JOINT_POINT_ON_LINE: case 4, target 0x1443d; (0x16c, 0) at 0x14447-0x1444e,
	//     phys_fn_004276 at 0x1445c (joint-families Task 3d).
	//   NX_JOINT_POINT_IN_PLANE: case 5, target 0x14466; (0x16c, 0) at 0x14470-0x14477,
	//     phys_fn_004262 at 0x14485 (joint-families Task 3e).
	//   NX_JOINT_DISTANCE: case 6, target 0x144b2; (0x184, 0) at 0x144bc-0x144c3,
	//     phys_fn_004234 at 0x144d1 (joint-families Task 3f).
	//   NX_JOINT_PULLEY: case 7, target 0x144d8; (0x1e0, 0) at 0x144e2-0x144e9,
	//     phys_fn_004222 at 0x144f7 (joint-families Task 3g).
	//   NX_JOINT_FIXED: case 8, target 0x1448c; (0x188, 0) at 0x14496-0x1449d,
	//     phys_fn_004250 at 0x144ab (joint-families Task 3h).
	//   NX_JOINT_D6: case 9, target 0x14554; (0x270, 0) at 0x1455e-0x14565,
	//     phys_fn_004210 at 0x1456f (joint-families Task 3i). This arm repeats
	//     the tail in place (0x14574-0x1458c) rather than jumping to 0x144fc;
	//     the behaviour is the same.
	//   Any other type: `cmp eax,9; ja 0x14529` (0x1438a-0x1438d), no
	//     allocation, internal stays 0 and the tail below returns 0. desc.isValid()
	//     has already rejected type >= NX_JOINT_COUNT, so only a descriptor whose
	//     own isValid() override accepts such a type gets here.
	Joint* internal = 0;
	if(d[1] == NX_JOINT_PRISMATIC)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(PrismaticJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) PrismaticJoint(static_cast<const NxPrismaticJointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_CYLINDRICAL)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(CylindricalJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) CylindricalJoint(static_cast<const NxCylindricalJointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_SPHERICAL)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(SphericalJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) SphericalJoint(static_cast<const NxSphericalJointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_POINT_ON_LINE)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(PointOnLineJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) PointOnLineJoint(static_cast<const NxPointOnLineJointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_POINT_IN_PLANE)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(PointInPlaneJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) PointInPlaneJoint(static_cast<const NxPointInPlaneJointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_DISTANCE)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(DistanceJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) DistanceJoint(static_cast<const NxDistanceJointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_PULLEY)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(PulleyJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) PulleyJoint(static_cast<const NxPulleyJointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_FIXED)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(FixedJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) FixedJoint(static_cast<const NxFixedJointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_D6)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(D6Joint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) D6Joint(static_cast<const NxD6JointDesc&>(desc));
		}
	else if(d[1] == NX_JOINT_REVOLUTE)
		{
		void* memory = nxFoundationSDKAllocator->malloc(sizeof(RevoluteJoint), NX_MEMORY_PERSISTENT);
		if(memory)
			internal = new(memory) RevoluteJoint(static_cast<const NxRevoluteJointDesc&>(desc));
		}

	NxJoint* result = 0;
	if(internal)
		{
		if(internal->mPublicObject)
			{
			// 0x14502: the public object at byte +0x48 (the same Joint field
			// for every family). 0x14509-0x14521: the NpScene's write-lock and
			// read-lock links into np+0x10 / np+0x14; then phys_fn_000661
			// (0x14524). phys_fn_000297 (0xc5ae-0xc5b9) returns
			// [internal+0x48], which the family's attach helper returns here.
			// `holder` is dereferenced without a null check, as the oracle does at
			// 0x14509.
			const unsigned* holder = reinterpret_cast<const unsigned*>(p[0x6cc / 4]);
			void* writeLink = reinterpret_cast<void*>(holder[3]);
			void* readLink = reinterpret_cast<void*>(holder[4]);
			if(d[1] == NX_JOINT_PRISMATIC)
				result = nxPrismaticJointAttachScene(static_cast<PrismaticJoint*>(internal), writeLink, readLink);
			else if(d[1] == NX_JOINT_CYLINDRICAL)
				result = nxCylindricalJointAttachScene(static_cast<CylindricalJoint*>(internal), writeLink, readLink);
			else if(d[1] == NX_JOINT_SPHERICAL)
				result = nxSphericalJointAttachScene(static_cast<SphericalJoint*>(internal), writeLink, readLink);
			else if(d[1] == NX_JOINT_POINT_ON_LINE)
				result = nxPointOnLineJointAttachScene(static_cast<PointOnLineJoint*>(internal), writeLink, readLink);
			else if(d[1] == NX_JOINT_POINT_IN_PLANE)
				result = nxPointInPlaneJointAttachScene(static_cast<PointInPlaneJoint*>(internal), writeLink, readLink);
			else if(d[1] == NX_JOINT_DISTANCE)
				result = nxDistanceJointAttachScene(static_cast<DistanceJoint*>(internal), writeLink, readLink);
			else if(d[1] == NX_JOINT_PULLEY)
				result = nxPulleyJointAttachScene(static_cast<PulleyJoint*>(internal), writeLink, readLink);
			else if(d[1] == NX_JOINT_FIXED)
				result = nxFixedJointAttachScene(static_cast<FixedJoint*>(internal), writeLink, readLink);
			else if(d[1] == NX_JOINT_D6)
				result = nxD6JointAttachScene(static_cast<D6Joint*>(internal), writeLink, readLink);
			else
				result = nxRevoluteJointAttachScene(static_cast<RevoluteJoint*>(internal), writeLink, readLink);
			addJoint(internal);
			}
		else
			{
			// 0x14581-0x1458c: internal slot 5 with 1 (the family's scalar
			// deleting destructor: phys_fn_004382 prismatic, phys_fn_004368
			// revolute, phys_fn_004322 cylindrical, phys_fn_004302 spherical,
			// phys_fn_004278 point-on-line, phys_fn_004264 point-in-plane,
			// phys_fn_004236 distance, phys_fn_004224 pulley, phys_fn_004252
			// fixed, phys_fn_004202 D6), then `xor esi,esi`.
			delete internal;
			}
		}

	// 0x14529-0x1453f, on every exit after the switch (success, allocation
	// failure, null +0x48, a type above 9): ++[Scene+0x6c8], [Scene+0x6bc] = [Scene+0x59c],
	// then the re-entry flag is cleared.
	++p[0x6c8 / 4];
	p[0x6bc / 4] = p[0x59c / 4];
	gNxApiReentry = false;
	return result;
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

// The scalar deleting destructor the vtable's slot 0 points at. Empty-scene
// release now follows the oracle's wrapper, auxiliary-manager, Scene free order.
// Populated-scene array and pruner ownership is released here; the oracle's
// actor-specific teardown allocations and remaining cache behavior are open.
static void nxSceneDelete(void* self, int flags)
	{
	unsigned* p = static_cast<unsigned*>(self);
	if(p[0x1b3])
		delete reinterpret_cast<NpScene*>(p[0x1b3]);
	NxSceneInternal* scene = static_cast<NxSceneInternal*>(self);
	while(NxActor** actors = scene->at<NxActor**>(0x55c))
		{
		NxActor** end = scene->at<NxActor**>(0x560);
		if(!end || actors == end) break;
		unsigned char* actor = reinterpret_cast<unsigned char*>(*actors);
		if(!actor) break;
		unsigned char* body = *reinterpret_cast<unsigned char**>(actor + 0x14);
		if(!body) break;
		scene->releaseActor(body);
		if(scene->at<NxActor**>(0x560) == end) break;
		}
	// The joints still registered (phys_fn_000606, the continuation of
	// phys_fn_000604 that the oracle's Scene destructor phys_fn_000663 calls at
	// 0x13f9e, after the actors): for each list, +0x59c then +0x5a0, the head
	// joint's link, mScene and flag bit 0 are cleared (so its ~Joint does not
	// call removeJoint), it is destroyed through slot 5 with 1, and the head
	// moves to the saved link (0x110f0-0x1118a). 000604's first loop (000760
	// over the +0x56c records) is not reproduced here.
	for(unsigned listOffset = 0x59c; listOffset <= 0x5a0; listOffset += 4)
		{
		while(Joint* joint = scene->at<Joint*>(listOffset))
			{
			void* next = joint->mNextJoint;
			joint->mNextJoint = 0;
			scene->at<Joint*>(listOffset)->mScene = 0;
			scene->at<Joint*>(listOffset)->mFlags &= ~1u;
			if(scene->at<Joint*>(listOffset))
				{
				delete scene->at<Joint*>(listOffset);
				scene->at<void*>(listOffset) = 0;
				}
			scene->at<void*>(listOffset) = next;
			}
		}
	// The joint record array (0x13ffb-0x14013) and the joint pointer array
	// (0x14195-0x141b9, which also zeroes end and capacity). These three
	// frees go through nxFoundationSDKAllocator (`[[0x101041bc]]` slot
	// +0x14), as 000663 does and as their allocators 000598, 000661 and
	// 000600 do.
	if(scene->at<void*>(0x5b8))
		{
		nxFoundationSDKAllocator->free(scene->at<void*>(0x5b8));
		scene->at<void*>(0x5b8) = 0;
		}
	// The JointSupportBody array (0x14019-0x14034): phys_fn_000600 allocates
	// it with a count word in front, so the free is of [+0x5ac]-4. Only the
	// simulation step grows it, so in the candidate it is always null here.
	if(scene->at<unsigned char*>(0x5ac))
		{
		nxFoundationSDKAllocator->free(scene->at<unsigned char*>(0x5ac) - 4);
		scene->at<void*>(0x5ac) = 0;
		}
	if(scene->at<void*>(0x58c))
		nxFoundationSDKAllocator->free(scene->at<void*>(0x58c));
	scene->at<void*>(0x58c) = 0;
	scene->at<void*>(0x590) = 0;
	scene->at<void*>(0x594) = 0;
	for(unsigned offset = 8; offset <= 0xc; offset += 4)
		{
		void*& entries = *reinterpret_cast<void**>(
			static_cast<unsigned char*>(self) + offset);
		if(entries)
			{
			nxGetSdkAllocator()->free(entries);
			entries = 0;
			}
		}
	if(p[0x12])
		{
		unsigned char* aux = reinterpret_cast<unsigned char*>(p[0x12]);
		for(int offset = 0x90; offset >= 0; offset -= 0x10)
			{
			void*& entries = *reinterpret_cast<void**>(aux + offset);
			if(entries)
				{
				nxGetSdkAllocator()->free(entries);
				entries = 0;
				}
			}
		nxGetSdkAllocator()->free(aux);
		}
	const unsigned arrayOffsets[] = {0x6fc, 0x6e8, 0x6d4, 0x6a4};
	for(unsigned offset : arrayOffsets)
		{
		void*& entries = *reinterpret_cast<void**>(
			static_cast<unsigned char*>(self) + offset);
		if(entries)
			{
			nxGetSdkAllocator()->free(entries);
			entries = 0;
			}
		}
	const unsigned tableOffsets[] = {0x640, 0x648};
	for(unsigned offset : tableOffsets)
		{
		void*& table = *reinterpret_cast<void**>(
			static_cast<unsigned char*>(self) + offset);
		if(!table) continue;
		unsigned char* container = static_cast<unsigned char*>(table);
		const unsigned childOffsets[] = {0x14, 0x18};
		for(unsigned childOffset : childOffsets)
			{
			void*& child = *reinterpret_cast<void**>(container + childOffset);
			if(child)
				{
				nxGetSdkAllocator()->free(child);
				child = 0;
				}
			}
		nxGetSdkAllocator()->free(table);
		table = 0;
		}
	const unsigned objectArrayOffsets[] = {0x56c, 0x55c};
	for(unsigned offset : objectArrayOffsets)
		{
		void*& entries = *reinterpret_cast<void**>(
			static_cast<unsigned char*>(self) + offset);
		if(entries)
			{
			nxGetSdkAllocator()->free(entries);
			entries = 0;
			}
		}
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

// phys_fn_000503 (0x000100a0, 232 B)
// A row of gap:PhysicsSDK.cpp..Scene.cpp. The Scene's per-object buffers follow a count: nothing when +4 already
// holds it; else +4 = (count + 0x100) & ~0xff, the +8 block freed and
// reallocated, then the +0xc block (both through [0x101041bc], capacity * 4
// bytes), only +8 zeroed (rep stosd over edi = [+8]), +0x14 = the capacity,
// and the SdkContainers at +0x50, +0x500 and +0x510 take (capacity, [+0xc])
// as their external buffer (004847). The last call, 004861 on the pruning
// collection (each pruner's slot 4 with the same pair), is not carried: the
// candidate's pruners (the model above) have no table.
void nxSceneUpdateActorCount(void* scene, unsigned count)
	{
	unsigned char* bytes = static_cast<unsigned char*>(scene);
	unsigned& capacity = *reinterpret_cast<unsigned*>(bytes + 4);
	if(capacity >= count) return;
	capacity = (count + 0x100u) & ~0xffu;
	void*& first = *reinterpret_cast<void**>(bytes + 8);
	if(first)
		{
		nxFoundationSDKAllocator->free(first);
		first = 0;
		}
	first = nxFoundationSDKAllocator->malloc(capacity * 4, NX_MEMORY_PERSISTENT);
	void*& second = *reinterpret_cast<void**>(bytes + 0xc);
	if(second)
		{
		nxFoundationSDKAllocator->free(second);
		second = 0;
		}
	second = nxFoundationSDKAllocator->malloc(capacity * 4, NX_MEMORY_PERSISTENT);
	memset(first, 0, capacity * 4);
	*reinterpret_cast<unsigned*>(bytes + 0x14) = capacity;
	NxU32* buffer = static_cast<NxU32*>(second);
	reinterpret_cast<SdkContainer*>(bytes + 0x50)->setExternalBuffer(capacity, buffer);
	reinterpret_cast<SdkContainer*>(bytes + 0x500)->setExternalBuffer(capacity, buffer);
	reinterpret_cast<SdkContainer*>(bytes + 0x510)->setExternalBuffer(capacity, buffer);
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

void nxActorSetName(void* actor, unsigned name)
	{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	if(body) nxShapeSetName(body, reinterpret_cast<const char*>(name));
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
	*reinterpret_cast<unsigned*>(body + 0xc) = *reinterpret_cast<unsigned*>(actorBytes + 0xc);
	memcpy(body + 0x20, desc, sizeof(NxMat34));
	}


// A float moved through the x87 (fld; fstp) and one negated there (fld;
// fchs; fstp), as the listings do: loading a signalling NaN quiets it, which
// an SSE move or sign flip would not.
static void nxX87MoveFloat(const void* from, void* to)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	__asm {
		mov eax, from
		mov edx, to
		fld dword ptr [eax]
		fstp dword ptr [edx]
	}
#else
	memcpy(to, from, 4);
#endif
	}

static void nxX87NegateFloat(const void* from, void* to)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	__asm {
		mov eax, from
		mov edx, to
		fld dword ptr [eax]
		fchs
		fstp dword ptr [edx]
	}
#else
	float value;
	memcpy(&value, from, 4);
	value = -value;
	memcpy(to, &value, 4);
#endif
	}

int nxActorComputeMassFromShapes(unsigned char* body, float density, float* totalMass,
	NxMat34* pose, NxVec3* diagonal);

// phys_fn_000630 (0x000124d0, 233 B)
// Scene::addBody(record): the record is pushed on the +0x56c array, which
// grows when full (capacity <= end) to 2n + 2 entries through [0x101041bc]
// unless its capacity already covers that, the live entries copied before
// the old block is freed; then 000503 with the new count. A null record (a
// failed allocation in 000026) is pushed too.
void nxSceneAddBody(NxSceneInternal* scene, unsigned char* record)
	{
	unsigned char* bytes = scene->bytes();
	void**& first = *reinterpret_cast<void***>(bytes + 0x56c);
	void**& last = *reinterpret_cast<void***>(bytes + 0x570);
	void**& end = *reinterpret_cast<void***>(bytes + 0x574);
	if(!(end > last))
		{
		const unsigned count = static_cast<unsigned>(last - first);
		const unsigned capacity = count + count + 2;
		const unsigned held = first ? static_cast<unsigned>(end - first) : 0;
		if(held < capacity)
			{
			void** grown = static_cast<void**>(nxFoundationSDKAllocator->malloc(
				capacity * sizeof(void*), NX_MEMORY_PERSISTENT));
			void** to = grown;
			for(void** from = first; from != last; ++from)
				*to++ = *from;
			if(first)
				nxFoundationSDKAllocator->free(first);
			end = grown + capacity;
			last = grown + count;
			first = grown;
			}
		}
	*last = record;
	++last;
	nxSceneUpdateActorCount(scene, static_cast<unsigned>(last - first));
	}

// phys_fn_000632 (0x000125c0, 160 B)
// Scene::removeBody(record): a record found in the +0x56c array is replaced
// by the last entry (when it is not the last) and the array shrinks; then,
// found or not, 000778 dissolves its island into the Scene's +0x58c joint
// array (no joint excepted), 000760 resets it, and every joint of that array
// runs 004103 with the record; a joint 004103 took out of the array (the
// entry changed) is not stepped over.
void nxSceneRemoveBody(NxSceneInternal* scene, unsigned char* record)
	{
	unsigned char* bytes = scene->bytes();
	void** first = *reinterpret_cast<void***>(bytes + 0x56c);
	const unsigned count = static_cast<unsigned>(*reinterpret_cast<void***>(bytes + 0x570) - first);
	for(unsigned i = 0; i < count; ++i)
		if(first[i] == record)
			{
			void**& last = *reinterpret_cast<void***>(bytes + 0x570);
			if(i != count - 1)
				first[i] = last[-1];
			--last;
			break;
			}
	void** joints = &scene->at<void*>(0x58c);
	reinterpret_cast<Row000778Fixture*>(record)->row000778(0, joints);
	reinterpret_cast<Row000760Fixture*>(record)->row000760();
	for(unsigned i = 0; i < static_cast<unsigned>(
		scene->at<Joint**>(0x590) - scene->at<Joint**>(0x58c)); ++i)
		{
		Joint* joint = scene->at<Joint**>(0x58c)[i];
		joint->row004103(record);
		if(joint != scene->at<Joint**>(0x58c)[i])
			--i;
		}
	}

// phys_fn_000797 (0x0001b5c0, 402 B)
// The body record's constructor (thiscall (body, &pose, &desc), `ret 0xc`;
// a row of gap:SceneRaycast.cpp..CapsuleShape.cpp), on the 0x260 bytes
// 000026 allocated. The record id comes first, from the Scene's pool at
// +0x6f8 (000012's pop, inlined at 0x1b5c9-0x1b5f5). 000801 builds the pose
// sub-object at +0x18 with the Scene's dirty manager [scene+0x48] (this
// file's model: the position at +0x18/+0x50, the 000801 quaternion of
// pose.M at +0x24/+0x5c, +0x120, the id at +0x11c and the manager's slot
// registration). The Observable base (+0x00..+0x14) is left zeroed: its
// vptr and the record's own (0x10106890) are not carried, and an empty
// observer list is what the import's constructor leaves. Then the listing's
// stores (0x1b61a-0x1b6f1): +0x134 and +0x20c identity, +0x158 zero, the
// bounds +0x244.. = FLT_MAX and +0x250.. = -FLT_MAX, +0x19c = the body,
// +0x198 and +0x1a0..+0x1b4 zero, +0x1b8 = 1; 000760 and 000722; +0x1e4,
// +0x1e0 and +0x204 zeroed; +0x20c.. = +0x134.. (nine words) and +0x230.. =
// +0x158..; and 000793 applies the descriptor. The candidate's record
// memory starts zeroed (the oracle's malloc does not zero it).
static void nxBodyRecordApplyDesc(unsigned char* record, const NxBodyDesc* bodyDesc);

static void nxBodyRecordConstruct(unsigned char* record, unsigned char* body,
	const NxMat34& pose, const NxBodyDesc* desc)
	{
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	memset(record, 0, 0x260);
	const unsigned id = nxIdAllocNext(scene->bytes() + 0x6f8);
	memcpy(record + 0x18, &pose.t, sizeof(NxVec3));
	float quaternion[4];
	nxNpActorBodyQuaternionFromMatrix(reinterpret_cast<const float*>(&pose.M), quaternion);
	memcpy(record + 0x24, quaternion, sizeof(quaternion));
	memcpy(record + 0x50, &pose.t, sizeof(NxVec3));
	memcpy(record + 0x5c, quaternion, sizeof(quaternion));
	*reinterpret_cast<unsigned char**>(record + 0x120) = scene->at<unsigned char*>(0x48);
	*reinterpret_cast<unsigned*>(record + 0x11c) = id;
	nxSceneAuxRegisterRecord(scene, record);

	static const unsigned identity[9] = {
		0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0x3f800000u };
	memcpy(record + 0x134, identity, sizeof(identity));
	memcpy(record + 0x20c, identity, sizeof(identity));
	for(unsigned offset = 0x244; offset < 0x250; offset += 4)
		*reinterpret_cast<unsigned*>(record + offset) = 0x7f7fffffu;
	for(unsigned offset = 0x250; offset < 0x25c; offset += 4)
		*reinterpret_cast<unsigned*>(record + offset) = 0xff7fffffu;
	*reinterpret_cast<unsigned char**>(record + 0x19c) = body;
	*reinterpret_cast<unsigned*>(record + 0x1b8) = 1;
	reinterpret_cast<Row000760Fixture*>(record)->row000760();
	reinterpret_cast<Row000722Fixture*>(record)->row000722();
	*reinterpret_cast<unsigned*>(record + 0x1e4) = 0;
	*reinterpret_cast<unsigned*>(record + 0x1e0) = 0;
	// The body's JointSupportBody pointer: 0 here (0x1b713); only the
	// simulation step's phys_fn_000611 (0x11305) points it at an element of
	// the Scene's +0x5ac array, and the candidate has no step
	// (joint-open-items-contract.md "## Body record +0x204").
	reinterpret_cast<JointBodyRecord*>(record)->mUnknown204 = 0;
	memcpy(record + 0x20c, record + 0x134, 0x24);
	memcpy(record + 0x230, record + 0x158, sizeof(NxVec3));
	nxBodyRecordApplyDesc(record, desc);
	}

// phys_fn_000793 (0x0001a350, 1613 B)
// With its continuation phys_fn_000795 (0x0001a9a0), the record's descriptor
// application; this file's model of it follows.
// Its mass block is the listing's (0x1a353-0x1a44d, 0x1a83a-0x1aaa6): +0x188
// = desc.mass and +0xc0 = (float)(1.0 / mass) with no test; a tensor with
// any nonzero bit is stored at +0x18c.. and its float inverses classified by
// _fpclass in order (a NaN or infinity zeroes all three), a zero tensor
// stores 1.0f three times and the inverses of 1.0 (the 0x10106898 double);
// +0xdc.. = the nine words of massLocalPose.M and +0x100.. its t. Its dirty
// marks are not carried (a new record's flags word is already set by the
// manager's registration). The other fields keep this model's earlier
// values: the flags, solver count, damping, wake counter, sleep thresholds
// (their defaults for a negative descriptor value), maximum angular velocity
// and velocities. 000768 refreshes the mass frame (0x1b497) and +0x198 is 2
// (its two increments).
static void nxBodyRecordApplyDesc(unsigned char* record, const NxBodyDesc* bodyDesc)
	{
	nxX87MoveFloat(&bodyDesc->mass, record + 0x188);		// fld [+0x3c]; fst [+0x188]
	*reinterpret_cast<float*>(record + 0xc0) = static_cast<float>(1.0 / bodyDesc->mass);
	*reinterpret_cast<float*>(record + 0xb8) = bodyDesc->linearDamping;
	*reinterpret_cast<float*>(record + 0xbc) = bodyDesc->angularDamping;
	memcpy(record + 0x100, &bodyDesc->massLocalPose.t, sizeof(NxVec3));
	memcpy(record + 0xdc, &bodyDesc->massLocalPose.M, 0x24);
	const unsigned* tensor = reinterpret_cast<const unsigned*>(&bodyDesc->massSpaceInertia);
	float* inverse = reinterpret_cast<float*>(record + 0xc4);
	if(tensor[0] || tensor[1] || tensor[2])
		{
		memcpy(record + 0x18c, tensor, 12);
		const float inverseX = static_cast<float>(1.0 / bodyDesc->massSpaceInertia.x);
		const float inverseY = static_cast<float>(1.0 / bodyDesc->massSpaceInertia.y);
		const float inverseZ = static_cast<float>(1.0 / bodyDesc->massSpaceInertia.z);
		if((_fpclass(inverseX) & 0x207) || (_fpclass(inverseY) & 0x207) ||
			(_fpclass(inverseZ) & 0x207))
			{
			inverse[0] = 0.0f;
			inverse[1] = 0.0f;
			inverse[2] = 0.0f;
			}
		else
			{
			inverse[0] = inverseX;
			inverse[1] = inverseY;
			inverse[2] = inverseZ;
			}
		}
	else
		{
		for(unsigned offset = 0x18c; offset < 0x198; offset += 4)
			*reinterpret_cast<unsigned*>(record + offset) = 0x3f800000u;
		inverse[0] = 1.0f;
		inverse[1] = 1.0f;
		inverse[2] = 1.0f;
		}
	*reinterpret_cast<unsigned*>(record + 0x10c) = bodyDesc->flags;
	*reinterpret_cast<unsigned*>(record + 0x110) = bodyDesc->solverIterationCount;
	*reinterpret_cast<float*>(record + 0x84) = bodyDesc->wakeUpCounter;
	*reinterpret_cast<float*>(record + 0x4c) = bodyDesc->wakeUpCounter;
	// FUN_1001a350 uses pinned SDK defaults when descriptor thresholds are
	// negative. These are 0.15^2 and 0.14^2 in the shipped binary.
	*reinterpret_cast<unsigned*>(record + 0xd0) = 0x3cb851ecu;
	*reinterpret_cast<unsigned*>(record + 0xd4) = 0x3ca0902eu;
	const float maxAngularVelocity = bodyDesc->maxAngularVelocity > 0.0f
		? bodyDesc->maxAngularVelocity : 7.0f;
	*reinterpret_cast<float*>(record + 0xd8) =
		maxAngularVelocity * maxAngularVelocity;
	if(bodyDesc->sleepLinearVelocity > 0.0f)
		*reinterpret_cast<float*>(record + 0xd0) =
			bodyDesc->sleepLinearVelocity * bodyDesc->sleepLinearVelocity;
	if(bodyDesc->sleepAngularVelocity > 0.0f)
		*reinterpret_cast<float*>(record + 0xd4) =
			bodyDesc->sleepAngularVelocity * bodyDesc->sleepAngularVelocity;
	memcpy(record + 0x6c, &bodyDesc->linearVelocity, sizeof(NxVec3));
	memcpy(record + 0x34, &bodyDesc->linearVelocity, sizeof(NxVec3));
	memcpy(record + 0x78, &bodyDesc->angularVelocity, sizeof(NxVec3));
	memcpy(record + 0x40, &bodyDesc->angularVelocity, sizeof(NxVec3));
	// The oracle's creation path refreshes the mass frame through
	// phys_fn_000768 (called from 000795 at 0x1b497): +0x134, +0x158, +0x124
	// and +0x164 from the +0x24 quaternion and +0x18 position.
	nxNpActorUpdateMassFrame(record);
	*reinterpret_cast<unsigned*>(record + 0x198) = 2;
	}

// phys_fn_000026 (0x000019b0, 465 B)
// Actor.cpp's record build on the body (thiscall (desc), `ret 4`), shared by
// the creation path (000034) and setDynamic (000122). The descriptor is
// copied (000010); a tensor whose three words are all zero bits takes
// 000008 with the body's density (+0x18) into the copy's mass (in and out),
// mass pose and tensor, and its nonzero result is returned. The pose is the
// old record's (its +0x5c quaternion by the five-spill rotation, ROT, and
// its +0x50 position) when the body has one, else the body's own (+0x20).
// 0x260 bytes through [0x101041bc], 000797 on them, +8 = the record (0 when
// the allocation failed), 000630 on the Scene with it; 0.
int nxActorBuildRecord(unsigned char* body, const NxBodyDesc* desc)
	{
	NxBodyDesc local = *desc;
	const unsigned* tensor = reinterpret_cast<const unsigned*>(&desc->massSpaceInertia);
	if(!tensor[0] && !tensor[1] && !tensor[2])
		{
		float density;
		memcpy(&density, body + 0x18, sizeof(density));
		const int result = nxActorComputeMassFromShapes(body, density, &local.mass,
			&local.massLocalPose, &local.massSpaceInertia);
		if(result)
			return result;
		}
	NxMat34 pose;
	const unsigned char* old = *reinterpret_cast<unsigned char**>(body + 8);
	if(old)
		{
		float rows[9];
		nxNpActorComposeRotation(reinterpret_cast<const float*>(old + 0x5c), rows);
		memcpy(&pose.M, rows, sizeof(rows));
		memcpy(&pose.t, old + 0x50, sizeof(NxVec3));
		}
	else
		memcpy(&pose, body + 0x20, sizeof(NxMat34));
	unsigned char* record = static_cast<unsigned char*>(
		nxFoundationSDKAllocator->malloc(0x260, NX_MEMORY_PERSISTENT));
	if(record)
		nxBodyRecordConstruct(record, body, pose, &local);
	*reinterpret_cast<unsigned char**>(body + 8) = record;
	nxSceneAddBody(*reinterpret_cast<NxSceneInternal**>(body + 4), record);
	return 0;
	}

// phys_fn_000776 (0x00018570, 117 B)
// The record's destructor body (a row of gap:SceneRaycast.cpp..CapsuleShape.cpp;
// the deleting caller frees the record). The record vptr store (0x10106890)
// is not carried. The id (+0x11c) goes back to the Scene's +0x6f8 pool
// (000028); the island cache +0x1e8 is compressed through 000713 when it is
// not the record; each record of the chain from +0x1e8 through +0x1fc
// (read before the call) runs 000722; then 000760 and 000722 on the record,
// the Observable destructor (the import; an empty observer list frees
// nothing, so the candidate's zeroed words need no call), and 000799 on the
// +0x18 sub-object: the dirty manager's slot released (this file's
// nxSceneAuxUnregisterRecord models 0x5bbb0) and the kinematic block at
// +0x118 freed through [0x101041bc] and cleared.
unsigned nxBodyRecordFixRoot(void* rec);

void nxBodyRecordDestroy(unsigned char* record)
	{
	unsigned char* body = *reinterpret_cast<unsigned char**>(record + 0x19c);
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	nxU32VectorPushBack(scene->bytes() + 0x6f8, *reinterpret_cast<unsigned*>(record + 0x11c));
	unsigned char*& cache = *reinterpret_cast<unsigned char**>(record + 0x1e8);
	if(cache != record)
		cache = reinterpret_cast<unsigned char*>(nxBodyRecordFixRoot(cache));
	for(unsigned char* member = cache; member; )
		{
		unsigned char* next = *reinterpret_cast<unsigned char**>(member + 0x1fc);
		reinterpret_cast<Row000722Fixture*>(member)->row000722();
		member = next;
		}
	reinterpret_cast<Row000760Fixture*>(record)->row000760();
	reinterpret_cast<Row000722Fixture*>(record)->row000722();
	// phys_fn_000799 (0x0001b760, 51 B)
	// On the +0x18 sub-object.
	nxSceneAuxUnregisterRecord(scene, record);
	void*& kinematic = *reinterpret_cast<void**>(record + 0x118);
	if(kinematic)
		{
		nxFoundationSDKAllocator->free(kinematic);
		kinematic = 0;
		}
	}

// phys_fn_000030 (0x00001c40, 403 B)
// Actor.cpp's actor destructor body, thiscall on the body, plain `ret`. The
// public actor ([body]) is deleted through its slot 0 with 1 (000118; the
// candidate's wrapper has no destructor to run, so it is freed) and [body]
// cleared; the name is dropped (000480 with 0); the root, when there is
// one, leaves the Scene (000535 with a record, else 000533: 000006's
// test). With a record: its pose goes back to the body (+0x20: the +0x5c
// quaternion by ROT, +0x50/+0x54 through the x87 and +0x58 as a word),
// 000632 removes it from the Scene, it notifies its observers with 0x100,
// and 000776 destroys it before [0x101041bc] frees it. The actor id (+0xc)
// goes back to the Scene's +0x6d0 pool (000028); 000521 hands the root to
// 000517, whose pass over the Scene's +0x3c/+0x40 pair table the candidate
// does not carry (it keeps none); the root is deleted through its slot 0
// with 1.
static void nxActorRemoveRootFromScene(unsigned char* body);
static void nxRuntimeShapeDeleteRoot(unsigned char* shape);

void nxActorDestroy(unsigned char* body)
	{
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	void*& actor = *reinterpret_cast<void**>(body);
	if(actor)
		{
		nxGetSdkAllocator()->free(actor);
		actor = 0;
		}
	nxShapeSetName(body, 0);
	nxActorRemoveRootFromScene(body);
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	if(record)
		{
		float rows[9];
		nxNpActorComposeRotation(reinterpret_cast<const float*>(record + 0x5c), rows);
		memcpy(body + 0x20, rows, sizeof(rows));
		nxX87MoveFloat(record + 0x50, body + 0x44);
		nxX87MoveFloat(record + 0x54, body + 0x48);
		memcpy(body + 0x4c, record + 0x58, 4);
		nxSceneRemoveBody(scene, record);
		reinterpret_cast<NxFoundation::Observable*>(record)->notifyObservers(0x100);
		nxBodyRecordDestroy(record);
		nxFoundationSDKAllocator->free(record);
		}
	nxU32VectorPushBack(scene->bytes() + 0x6d0, *reinterpret_cast<unsigned*>(body + 0xc));
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(root)
		nxRuntimeShapeDeleteRoot(root);
	}

// OPCODE's first static pruner is a 0x90-byte object. Its constructor also
// initializes a process-wide 0x1c-byte pool on first use. The first insertion
// gives it four 0x18-byte entries and four pointer references. These allocations
// occur in Actor::loadFromDescInternal, before Scene::createActor grows its
// public actor list. The root takes the kind phys_fn_001943 gives it and the
// pruning collection at +0xa0; a static group's children are not registered
// by this model of the creation path (the oracle's 001943 adds them too).
static void nxSceneStaticPrunerRegister(NxSceneInternal* scene, unsigned char* shape)
	{
	if(!scene || !shape) return;
	*reinterpret_cast<unsigned char*>(shape + 0xce) = 0;
	*reinterpret_cast<unsigned char*>(shape + 0xcf) =
		*reinterpret_cast<unsigned*>(shape + 0xd0) == 5u ? 2 : 1;
	*reinterpret_cast<void**>(shape + 0xa0) = scene->bytes() + 0x624;
	*reinterpret_cast<unsigned short*>(shape + 0xcc) = 0xffffu;
	nxScenePrunerInsert(scene, shape);
	unsigned char* manager = scene->at<unsigned char*>(0x640);
	if(!manager || *reinterpret_cast<unsigned short*>(shape + 0xcc) == 0xffffu) return;
	nxSceneTrackShape(scene, shape);
	nxSceneUpdateActorCount(scene, *reinterpret_cast<unsigned short*>(manager + 0x10));
	*reinterpret_cast<void**>(manager + 0x48) = scene->at<void*>(0xc);
	*reinterpret_cast<unsigned*>(manager + 0x40) = scene->at<unsigned>(4);
	}

// A static root and its current children leave the static pruner, the root
// its +0x78 list entry first.
static void nxSceneStaticPrunerUnregister(NxSceneInternal* scene, unsigned char* shape)
	{
	nxSceneUntrackShape(scene, shape);
	if(!shape) return;
	nxScenePrunerErase(scene, shape);
	if(*reinterpret_cast<unsigned*>(shape + 0xd0) == 5u)
		{
		void** child = *reinterpret_cast<void***>(shape + 0xe0);
		void** end = *reinterpret_cast<void***>(shape + 0xe4);
		for(; child && child != end; ++child)
			nxScenePrunerErase(scene, static_cast<unsigned char*>(*child));
		}
	}

static void nxRuntimeShapeSlot6(unsigned char* shape, unsigned flags);

void nxSceneAddActorObject(void* scene, void* object, void* actorPointer)
	{
	// Register the dynamic record in the Scene's +0x56c array. Static actors
	// have no record and do not enter this array; the fourth box actor grows
	// its capacity from two entries to six, matching the pinned DLL.
	unsigned char* actor = static_cast<unsigned char*>(actorPointer);
	if(!actor) return;
	unsigned char* body = *reinterpret_cast<unsigned char**>(actor + 0x14);
	unsigned char* record = body
		? *reinterpret_cast<unsigned char**>(body + 8) : 0;
	if(!object && !record) return;
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(!record)
		{
		// 000531's first step on a static group root: its slot 6 with 1
		// (001018: each child's 001315, then the group's own, which leaves
		// the group's +0xdc at 2). A single root took 001315 with 1 in the
		// factory wrapper.
		if(shape && *reinterpret_cast<unsigned*>(shape + 0xd0) == 5u)
			nxRuntimeShapeSlot6(shape, 1);
		nxSceneStaticPrunerRegister(static_cast<NxSceneInternal*>(scene),
			static_cast<unsigned char*>(object));
		return;
		}
	if(shape)
		{
		if(*reinterpret_cast<unsigned*>(shape + 0xd0) == 5u)
			{
			void** first = *reinterpret_cast<void***>(shape + 0xe0);
			void** last = *reinterpret_cast<void***>(shape + 0xe4);
			for(void** child = first; child && child != last; ++child)
				nxShapeFactoryRefreshPose(*child);
			}
		else
			nxShapeFactoryRefreshPose(shape);
		}
	nxSceneBroadphaseRegister(static_cast<NxSceneInternal*>(scene), body);
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

// ---------------------------------------------------------------------------
// The runtime shapes and Actor.cpp's shape add/remove chain (NpActor.cpp
// completion Task 4; units/npactor-contract.md "## Task 4: shape add/remove"
// names each row's owning unit).
//
// The shapes an actor holds are raw allocations of the oracle's sizes whose
// words are the listing's: the families' final classes in ObjectModel.cpp
// (ShapeBase, BoxShape, SphereShape, ...) are the listing models the layout
// differentials drive, and their installed tables give the runtime shapes
// the reconstructed virtual rows (slot 6 is ShapeBase::nxApplyOwnerUpdate,
// 001315). The families' slot 0 there models the listing's object, so the
// runtime shape's deleting destructor is nxRuntimeShapeDelete below; the
// group is this file's own object with its own table.
// ---------------------------------------------------------------------------

#define NX_ACTOR_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\Actor.cpp"

// .data 0x10123c10 is gNxApiReentry (with Scene::createJoint's rows above).
// The message .data 0x10122050 points at.
static const char gNxActorReentryMessage[] =
	"Reentry check: You may not call this API method from a callback!";

static void nxActorCppReport(NxErrorCode code, int line, const char* message)
	{
	NxFoundation::FoundationSDK::getInstance().error(code, NX_ACTOR_CPP, line, 0, message);
	}

typedef void (__thiscall* NxRuntimeShapeSlot6Fn)(void* shape, unsigned flags);
typedef void* (__thiscall* NxRuntimeShapeDeleteFn)(void* shape, unsigned flags);

static void nxRuntimeShapeSlot6(unsigned char* shape, unsigned flags)
	{
	void** table = *reinterpret_cast<void***>(shape);
	reinterpret_cast<NxRuntimeShapeSlot6Fn>(table[6])(shape, flags);
	}

// phys_fn_001273 (0x00025530, 424 B)
// The base-shape constructor on a runtime shape (a row of
// gap:NpTriangleMeshShape.cpp..Shape.cpp; ShapeBase::ShapeBase in
// ObjectModel.cpp is its listing model): the owner body
// at +4, +8 zeroed, the three identity poses, +0x9c/+0xa0 zeroed, the
// prunable at +0xa4 (phys_fn_004874 at 0x255df: +8, +0x20 and the type and
// kind bytes zeroed, +0x24 = -1, handle +0x28 = 0xffff, +0x10 = the
// prunable; the shape then stores itself as its owner, 0x25649), the
// 0x7fffffff sentinel, the id at +0xd4, halfwords +0xd8/+0xda zeroed,
// +0xdc = 6, +0xde = 8, and with an owner the Scene registration through
// [scene+0x48] (phys_fn_002423 at 0x25626; nxSceneAuxRegisterShape is this
// file's model). The prunable's vptr and its 005297 member are not carried.
static void nxRuntimeShapeBaseInit(unsigned char* shape, unsigned char* body, unsigned id)
	{
	static const unsigned identity[12] = {
		0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0 };
	*reinterpret_cast<unsigned char**>(shape + 4) = body;
	*reinterpret_cast<unsigned*>(shape + 8) = 0;
	memcpy(shape + 0x0c, identity, sizeof(identity));
	memcpy(shape + 0x3c, identity, sizeof(identity));
	memcpy(shape + 0x6c, identity, sizeof(identity));
	*reinterpret_cast<unsigned*>(shape + 0x9c) = 0;
	*reinterpret_cast<unsigned*>(shape + 0xa0) = 0;
	*reinterpret_cast<unsigned*>(shape + 0xac) = 0;
	*reinterpret_cast<unsigned char**>(shape + 0xb4) = shape + 0xa4;
	*reinterpret_cast<unsigned*>(shape + 0xc4) = 0;
	shape[0xce] = 0;
	shape[0xcf] = 0;
	*reinterpret_cast<unsigned*>(shape + 0xc8) = 0xffffffffu;
	*reinterpret_cast<unsigned short*>(shape + 0xcc) = 0xffffu;
	*reinterpret_cast<unsigned char**>(shape + 0xa8) = shape;
	*reinterpret_cast<unsigned*>(shape + 0xd0) = 0x7fffffffu;
	*reinterpret_cast<unsigned*>(shape + 0xd4) = id;
	*reinterpret_cast<unsigned short*>(shape + 0xd8) = 0;
	*reinterpret_cast<unsigned short*>(shape + 0xda) = 0;
	*reinterpret_cast<unsigned short*>(shape + 0xdc) = 6;
	*reinterpret_cast<unsigned short*>(shape + 0xde) = 8;
	if(body)
		nxSceneAuxRegisterShape(*reinterpret_cast<NxSceneInternal**>(body + 4), shape);
	}

// The family constructors phys_fn_000032 calls on its allocation: 001247
// plane (0x10c B), 001349 sphere (0xe4 B), 000977 box (0x228 B) and 000987
// capsule (0xec B). Each runs 001273, installs its final table, allocates
// its 0x1c-byte collision object -- the public NxShape handle -- through
// [0x101041bc] (box 0x218f1, sphere 0x277e4) and stores it at +0x9c, and
// writes its sentinel at +0xd0; the box also sets its dimensions to 1.0f
// (0x21926-0x21932). The handle carries the public table, and the shape at
// +8 and +0x18 (phys_fn_001193's two stores). Words the listings leave
// unwritten start zeroed here.
static unsigned char* nxRuntimeShapeConstruct(void* memory, unsigned size, unsigned type,
	unsigned char* body, unsigned id)
	{
	unsigned char* shape = static_cast<unsigned char*>(memory);
	memset(shape, 0, size);
	nxRuntimeShapeBaseInit(shape, body, id);
	nxShapeFactoryInstallVtable(shape, type);
	void* handle = nxFoundationSDKAllocator->malloc(0x1c, NX_MEMORY_PERSISTENT);
	if(handle)
		{
		memset(handle, 0, 0x1c);
		*reinterpret_cast<void**>(handle) = nxShapePublicVtable(type);
		*reinterpret_cast<void**>(static_cast<unsigned char*>(handle) + 8) = shape;
		*reinterpret_cast<void**>(static_cast<unsigned char*>(handle) + 0x18) = shape;
		}
	*reinterpret_cast<void**>(shape + 0x9c) = handle;
	*reinterpret_cast<unsigned*>(shape + 0xd0) = type;
	if(type == 2)
		{
		const float one = 1.0f;
		memcpy(shape + 0xe4, &one, 4);
		memcpy(shape + 0xe8, &one, 4);
		memcpy(shape + 0xec, &one, 4);
		}
	return shape;
	}

// The families' slot-12 descriptor load (vtable +0x30) on a runtime shape:
// the collision group (+0xd8) and its mask (+0xc8), the material (+0xda),
// the shape flags (+0xde), the family's geometry, the local pose into the
// third pose (+0x6c, the base apply-desc 001347) and the name registration.
// SphereShape::nxSphereLoadFromDesc and its siblings are the listing models.
static bool nxRuntimeShapeLoad(unsigned char* shape, const NxShapeDesc* descriptor)
	{
	*reinterpret_cast<NxCollisionGroup*>(shape + 0xd8) = descriptor->group;
	*reinterpret_cast<NxMaterialIndex*>(shape + 0xda) = descriptor->materialIndex;
	*reinterpret_cast<unsigned*>(shape + 0xc8) = 1u << descriptor->group;
	*reinterpret_cast<NxU16*>(shape + 0xde) = static_cast<NxU16>(descriptor->shapeFlags);
	if(descriptor->getType() == NX_SHAPE_BOX)
		memcpy(shape + 0xe4,
			&static_cast<const NxBoxShapeDesc*>(descriptor)->dimensions, sizeof(NxVec3));
	else if(descriptor->getType() == NX_SHAPE_SPHERE)
		memcpy(shape + 0xe0,
			&static_cast<const NxSphereShapeDesc*>(descriptor)->radius, sizeof(float));
	else if(descriptor->getType() == NX_SHAPE_CAPSULE)
		{
		const NxCapsuleShapeDesc* capsule = static_cast<const NxCapsuleShapeDesc*>(descriptor);
		memcpy(shape + 0xe0, &capsule->radius, sizeof(float));
		const float halfHeight = capsule->height * 0.5f;
		memcpy(shape + 0xe4, &halfHeight, sizeof(float));
		}
	else if(descriptor->getType() == NX_SHAPE_PLANE)
		{
		const NxPlaneShapeDesc* plane = static_cast<const NxPlaneShapeDesc*>(descriptor);
		nxShapeFactoryInitializePlane(shape, &plane->normal.x, plane->d);
		}
	memcpy(shape + 0x6c, &descriptor->localPose, 0x30);
	if(descriptor->name)
		nxShapeSetName(shape, descriptor->name);
	return true;
	}

static void nxPruningRemoveRootPairs(unsigned char* pruning, unsigned char* body);
static void nxPruningRemoveBody(unsigned char* pruning, unsigned char* body);

// phys_fn_001323 (0x00026bd0, 182 B)
// The base-shape destructor body (Shape.cpp) on a runtime shape: the name
// registration dropped (000480 with a null name),
// with an owner the Scene's +0x70c |= 2, the [scene+0x48] registration
// (002413; nxSceneAuxUnregisterShape models it), the Scene+0x5d4 pair list
// (002344) and the id returned to Scene+0x6e4 (000028); then, while +0xa0
// names the pruning collection, 001955 and 001945 on the owner and +0xa0
// cleared. Not carried: 000517's pass over the Scene's +0x3c/+0x40 pair
// table (the candidate keeps none) and the prunable's destructor 004892.
static void nxRuntimeShapeBaseDestroy(unsigned char* shape)
	{
	nxShapeSetName(shape, 0);
	unsigned char* body = *reinterpret_cast<unsigned char**>(shape + 4);
	NxSceneInternal* scene = body ? *reinterpret_cast<NxSceneInternal**>(body + 4) : 0;
	if(body)
		scene->at<unsigned>(0x70c) |= 2u;
	if(body)
		nxSceneAuxUnregisterShape(scene, shape);
	if(body)
		nxSceneRemovePairs(scene->bytes() + 0x5d4, shape);
	if(body)
		nxU32VectorPushBack(scene->bytes() + 0x6e4, *reinterpret_cast<unsigned*>(shape + 0xd4));
	if(*reinterpret_cast<unsigned char**>(shape + 0xa0))
		nxPruningRemoveRootPairs(*reinterpret_cast<unsigned char**>(shape + 0xa0), body);
	if(*reinterpret_cast<unsigned char**>(shape + 0xa0))
		{
		nxPruningRemoveBody(*reinterpret_cast<unsigned char**>(shape + 0xa0), body);
		*reinterpret_cast<unsigned char**>(shape + 0xa0) = 0;
		}
	}

// The four families' deleting destructors with flag 1 (sphere 001375 at
// 0x27c30 and its siblings): the collision object's deleting destructor
// with 1 (001197: its hook teardown 002406 is not carried), then 001323,
// then the shape freed through [0x101041bc].
static void nxRuntimeShapeDelete(unsigned char* shape)
	{
	void* handle = *reinterpret_cast<void**>(shape + 0x9c);
	if(handle)
		nxFoundationSDKAllocator->free(handle);
	nxRuntimeShapeBaseDestroy(shape);
	nxFoundationSDKAllocator->free(shape);
	}

// phys_fn_000032 (0x00001de0, 539 B)
// Actor.cpp's shape factory, cdecl (desc, body). The id comes first, from
// the Scene's pool at +0x6e4 (000012 inlined, 0x1de8-0x1e1a); the jump
// table on the descriptor's type (0x1e2c) allocates the family through
// [0x101041bc] and constructs it; slot 12 loads the descriptor, and a load
// that fails or a shape without its collision object is deleted (slot 0
// with 1). A built shape's handle takes the NpScene's two lock links
// ([[scene+0x6cc]+0xc]/+0x10 to handle +0x10/+0x14, 0x1f16-0x1f31) and the
// shape +8 = [scene+0x540] - 1 (0x1fd1-0x1fdc); without a shape the id goes
// back to the pool (000028, 0x1fe5). Type 4 (the triangle mesh, 0xe8 B,
// 001379, which also counts Scene+0x10 and calls 000503) has no runtime
// family in the candidate and takes the no-shape arm, like types above 4.
static unsigned char* nxActorShapeFactory(const NxShapeDesc* descriptor, unsigned char* body)
	{
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	const unsigned id = nxIdAllocNext(scene->bytes() + 0x6e4);
	const unsigned type = static_cast<unsigned>(descriptor->getType());
	unsigned char* shape = 0;
	if(type <= 3)
		{
		static const unsigned sizes[4] = { 0x10c, 0xe4, 0x228, 0xec };
		void* memory = nxFoundationSDKAllocator->malloc(sizes[type], NX_MEMORY_PERSISTENT);
		if(memory)
			{
			shape = nxRuntimeShapeConstruct(memory, sizes[type], type, body, id);
			if(!nxRuntimeShapeLoad(shape, descriptor) || !*reinterpret_cast<void**>(shape + 0x9c))
				{
				nxRuntimeShapeDelete(shape);
				shape = 0;
				}
			else
				{
				unsigned char* handle = *reinterpret_cast<unsigned char**>(shape + 0x9c);
				unsigned char* npScene = scene->at<unsigned char*>(0x6cc);
				*reinterpret_cast<unsigned*>(handle + 0x10) = *reinterpret_cast<unsigned*>(npScene + 0xc);
				*reinterpret_cast<unsigned*>(handle + 0x14) = *reinterpret_cast<unsigned*>(npScene + 0x10);
				}
			}
		}
	if(!shape)
		{
		nxU32VectorPushBack(scene->bytes() + 0x6e4, id);
		return 0;
		}
	*reinterpret_cast<unsigned*>(shape + 8) = scene->at<unsigned>(0x540) - 1u;
	return shape;
	}

// Actor::loadFromDescInternal's use of the factory (row 000034, this
// file's model above): the new shape becomes the body's root, and its
// poses are composed here (the oracle's 000034 reaches 001315 through
// 000531); a group built by the creation path re-links its children.
void* nxShapeFactory(void* shapeDesc, void* actor)
	{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	const NxShapeDesc* descriptor = static_cast<const NxShapeDesc*>(shapeDesc);
	if(!body || !descriptor) return 0;
	unsigned char* shape = nxActorShapeFactory(descriptor, body);
	if(!shape) return 0;
	*reinterpret_cast<void**>(body + 0x10) = shape;
	nxShapeFactoryInitializePose(shape, &descriptor->localPose);
	return shape;
	}

// ---------------------------------------------------------------------------
// The shape group (NX_SHAPE_COMPOUND, sentinel 5), rows of
// gap:CapsuleShape.cpp..NpBoxShape.cpp. +0xe0/+0xe4/+0xe8 are the child
// array's begin/end/capacity, +0xf0/+0xf4/+0xf8 the parallel array of the
// children's public handles, +0x10c a float the adds and removes reset to
// -1.0f. Its table (.rdata 0x10106c2c) is this file's gNxShapeGroupTable:
// slot 0 the deleting destructor 001039, slot 4 the mass walk 001024 and
// slot 6 the owner update 001018 are the slots the runtime reaches; the
// others (001347, 001277, 001022, 001030, ...) are not wired and stay null.
// ---------------------------------------------------------------------------

static void* __fastcall nxShapeGroupDeletingDtor(void* self, void*, unsigned flags);
static void __fastcall nxShapeGroupOwnerUpdate(void* self, void*, unsigned flags);

// The group's slot 4 is 001024 (ObjectModel.cpp nxArrayVtCall3Args1024,
// thiscall `ret 0xc`): each child whose +0xde has none of the low three bits
// runs its own slot 4 with the same three arguments, and the first false
// return ends the walk with false. Reached by 000008.
unsigned char nxArrayVtCall3Args1024(void* self, unsigned a1, unsigned a2, unsigned a3);

static bool __fastcall nxShapeGroupAccumulateMass(void* self, void*, void* frame,
	unsigned density, void* reserved)
	{
	return nxArrayVtCall3Args1024(self, reinterpret_cast<unsigned>(frame), density,
		reinterpret_cast<unsigned>(reserved)) != 0;
	}

static void** nxShapeGroupTable()
	{
	static void* table[15] = {
		reinterpret_cast<void*>(&nxShapeGroupDeletingDtor), 0, 0, 0,
		reinterpret_cast<void*>(&nxShapeGroupAccumulateMass), 0,
		reinterpret_cast<void*>(&nxShapeGroupOwnerUpdate), 0, 0, 0, 0, 0, 0, 0, 0 };
	return table;
	}

// phys_fn_001033 (0x00022d60, 99 B)
// The group's constructor (thiscall (body, id)): 001273, the group table,
// both arrays emptied, sentinel 5, +0xd8 = 0xffff and +0x10c = -1.0f.
static unsigned char* nxShapeGroupConstructAt(void* memory, unsigned char* body, unsigned id)
	{
	unsigned char* group = static_cast<unsigned char*>(memory);
	memset(group, 0, 0x110);
	nxRuntimeShapeBaseInit(group, body, id);
	*reinterpret_cast<void***>(group) = nxShapeGroupTable();
	for(unsigned offset = 0xe0; offset <= 0xf8; offset += 4)
		if(offset != 0xec)
			*reinterpret_cast<unsigned*>(group + offset) = 0;
	*reinterpret_cast<unsigned*>(group + 0xd0) = 5;
	*reinterpret_cast<unsigned short*>(group + 0xd8) = 0xffffu;
	*reinterpret_cast<unsigned*>(group + 0x10c) = 0xbf800000u;
	return group;
	}

// 001041's inline pointer-array push: a full array (capacity <= end) grows
// to 2n + 2 entries through [0x101041bc] unless it already holds that many,
// the live entries copied before the old block is released.
static void nxShapeGroupArrayPush(unsigned char* group, unsigned offset, void* value)
	{
	void**& first = *reinterpret_cast<void***>(group + offset);
	void**& last = *reinterpret_cast<void***>(group + offset + 4);
	void**& end = *reinterpret_cast<void***>(group + offset + 8);
	if(!(end > last))
		{
		const unsigned count = static_cast<unsigned>(last - first);
		const unsigned capacity = count + count + 2;
		const unsigned held = first ? static_cast<unsigned>(end - first) : 0;
		if(held < capacity)
			{
			void** grown = static_cast<void**>(nxFoundationSDKAllocator->malloc(
				capacity * sizeof(void*), NX_MEMORY_PERSISTENT));
			void** to = grown;
			for(void** from = first; from != last; ++from)
				*to++ = *from;
			if(first)
				nxFoundationSDKAllocator->free(first);
			end = grown + capacity;
			last = grown + count;
			first = grown;
			}
		}
	*last = value;
	++last;
	}

// phys_fn_001041 (0x00022e80, 442 B)
// Group add: the child onto +0xe0, its handle ([child+0x9c]) onto +0xf0,
// the child's +0xdc bit 0 set, +0x10c = -1.0f and the group marked with
// 0x100 (001325).
static void nxShapeGroupAddChild(unsigned char* group, unsigned char* child)
	{
	nxShapeGroupArrayPush(group, 0xe0, child);
	nxShapeGroupArrayPush(group, 0xf0, *reinterpret_cast<void**>(child + 0x9c));
	child[0xdc] |= 1;
	*reinterpret_cast<unsigned*>(group + 0x10c) = 0xbf800000u;
	nxSceneMarkShapeDirty(group, 0x100);
	}

// phys_fn_001028 (0x00022b50, 152 B)
// Group remove: a child the +0xe0 array does not hold returns false with
// nothing written. Otherwise, unless it is the last entry, the last child
// and the last handle move into its slots (swap-remove in both arrays),
// both ends drop by one, the child's +0xdc bit 0 is cleared and +0x10c =
// -1.0f. The child is unlinked only: nothing is freed or unregistered.
static __declspec(noinline) bool nxShapeGroupRemoveChild(unsigned char* group, unsigned char* child)
	{
	void** first = *reinterpret_cast<void***>(group + 0xe0);
	const unsigned count = static_cast<unsigned>(*reinterpret_cast<void***>(group + 0xe4) - first);
	unsigned index = 0;
	while(index < count && first[index] != child)
		++index;
	if(index == count)
		return false;
	if(index != count - 1)
		{
		first[index] = (*reinterpret_cast<void***>(group + 0xe4))[-1];
		(*reinterpret_cast<void***>(group + 0xf0))[index] =
			(*reinterpret_cast<void***>(group + 0xf4))[-1];
		}
	*reinterpret_cast<void***>(group + 0xe4) -= 1;
	*reinterpret_cast<void***>(group + 0xf4) -= 1;
	child[0xdc] &= 0xfe;
	*reinterpret_cast<unsigned*>(group + 0x10c) = 0xbf800000u;
	return true;
	}

// phys_fn_001018 (0x000227d0, 62 B)
// Group slot 6: every child's slot 6 with the argument ((end - begin) / 4
// entries, no null test), then 001315 on the group itself.
static void __fastcall nxShapeGroupOwnerUpdate(void* self, void*, unsigned flags)
	{
	unsigned char* group = static_cast<unsigned char*>(self);
	unsigned char** child = *reinterpret_cast<unsigned char***>(group + 0xe0);
	unsigned count = static_cast<unsigned>(
		*reinterpret_cast<unsigned char***>(group + 0xe4) - child);
	for(; count; --count)
		nxRuntimeShapeSlot6(*child++, flags);
	nxShapeApplyOwnerUpdate(self, flags);
	}

// phys_fn_001032 (0x00022d00, 96 B)
// The children deleted (slot 0 with 1, each slot then cleared), both ends
// reset to the begins, the group marked 0x100.
static __declspec(noinline) void nxShapeGroupDeleteChildren(unsigned char* group)
	{
	unsigned char** child = *reinterpret_cast<unsigned char***>(group + 0xe0);
	unsigned count = static_cast<unsigned>(
		*reinterpret_cast<unsigned char***>(group + 0xe4) - child);
	for(; count; --count, ++child)
		if(*child)
			{
			nxRuntimeShapeDelete(*child);
			*child = 0;
			}
	*reinterpret_cast<void**>(group + 0xe4) = *reinterpret_cast<void**>(group + 0xe0);
	*reinterpret_cast<void**>(group + 0xf4) = *reinterpret_cast<void**>(group + 0xf0);
	nxSceneMarkShapeDirty(group, 0x100);
	}

// phys_fn_001039 (0x00022e50, 34 B)
// phys_fn_001037 (0x00022de0, 110 B)
// The group's deleting destructor (001039) over its destructor body (001037): 001032, the handle array then the child
// array freed through [0x101041bc] and zeroed, 001323, and with flag bit 0
// the group freed.
static void* __fastcall nxShapeGroupDeletingDtor(void* self, void*, unsigned flags)
	{
	unsigned char* group = static_cast<unsigned char*>(self);
	nxShapeGroupDeleteChildren(group);
	if(*reinterpret_cast<void**>(group + 0xf0))
		nxFoundationSDKAllocator->free(*reinterpret_cast<void**>(group + 0xf0));
	*reinterpret_cast<unsigned*>(group + 0xf0) = 0;
	*reinterpret_cast<unsigned*>(group + 0xf4) = 0;
	*reinterpret_cast<unsigned*>(group + 0xf8) = 0;
	if(*reinterpret_cast<void**>(group + 0xe0))
		nxFoundationSDKAllocator->free(*reinterpret_cast<void**>(group + 0xe0));
	*reinterpret_cast<unsigned*>(group + 0xe0) = 0;
	*reinterpret_cast<unsigned*>(group + 0xe4) = 0;
	*reinterpret_cast<unsigned*>(group + 0xe8) = 0;
	nxRuntimeShapeBaseDestroy(group);
	if(flags & 1u)
		nxFoundationSDKAllocator->free(group);
	return group;
	}

// Slot 0 with 1 on a body's root: the group's table, or the families'
// deleting destructors as modelled above.
static void nxRuntimeShapeDeleteRoot(unsigned char* shape)
	{
	if(*reinterpret_cast<unsigned*>(shape + 0xd0) == 5u)
		reinterpret_cast<NxRuntimeShapeDeleteFn>((*reinterpret_cast<void***>(shape))[0])(shape, 1);
	else
		nxRuntimeShapeDelete(shape);
	}

// The multi-shape group of the actor-creation path, 000034's group arm
// (0x2137-0x21a9): 0x110 bytes through [0x101041bc], an id from the Scene's
// pool (000012, after the allocation), 001033, the group becomes the body's
// root with +8 = [scene+0x540] - 1; then each descriptor's shape from the
// factory (000032) is appended by 001041 (its arrays growing to 2n + 2 as
// they fill; the child's +0xdc bit 0). The children keep this model's pose
// initialisation (the factory wrapper's 001315 with 1). A failed child ends
// the arm: the oracle returns with the group installed, the candidate
// deletes it and clears the root.
void* nxShapeGroupConstruct(void* actor, const unsigned* shapeDescriptions, unsigned count)
	{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(
		static_cast<unsigned char*>(actor) + 4);
	void* memory = nxFoundationSDKAllocator->malloc(0x110, NX_MEMORY_PERSISTENT);
	if(!memory)
		return 0;
	unsigned char* group = nxShapeGroupConstructAt(memory, body,
		nxIdAllocNext(scene->bytes() + 0x6e4));
	*reinterpret_cast<unsigned char**>(body + 0x10) = group;
	*reinterpret_cast<unsigned*>(group + 8) = scene->at<unsigned>(0x540) - 1u;
	for(unsigned i = 0; i < count; ++i)
		{
		const NxShapeDesc* descriptor =
			reinterpret_cast<const NxShapeDesc*>(shapeDescriptions[i]);
		unsigned char* child = nxActorShapeFactory(descriptor, body);
		if(!child)
			{
			nxRuntimeShapeDeleteRoot(group);
			*reinterpret_cast<void**>(body + 0x10) = 0;
			return 0;
			}
		nxShapeFactoryInitializePose(child, &descriptor->localPose);
		nxShapeGroupAddChild(group, child);
		}
	return group;
	}

// ---------------------------------------------------------------------------
// The Scene's shape registration: Scene.cpp rows 000531/000533/000535 and
// the pruning collection's rows of gap:ContactPlaneMesh.cpp..PenetrationMap.cpp
// (001941, 001943, 001945, 001955, 001957, 001960). The pruners are the
// model above (nxScenePrunerInsert/Erase); the collection's +0x78 list is
// the SdkContainer phys_fn_001980 builds there (Scene+0x69c).
// ---------------------------------------------------------------------------

static NxSceneInternal* nxPruningScene(unsigned char* pruning)
	{
	return reinterpret_cast<NxSceneInternal*>(pruning - 0x624);
	}

// phys_fn_001957 (0x0004be80, 16 B)
// The first pruner's two counts.
static unsigned nxPruningCountFirst(const unsigned char* pruning)
	{
	const unsigned char* pruner = *reinterpret_cast<unsigned char* const*>(pruning + 0x1c);
	return pruner ? *reinterpret_cast<const unsigned*>(pruner + 0xc) +
		*reinterpret_cast<const unsigned*>(pruner + 8) : 0;
	}

// phys_fn_001960 (0x0004bec0, 20 B)
// The counts of the pruner +0x70 selects.
static unsigned nxPruningCountIndexed(const unsigned char* pruning)
	{
	const unsigned char* pruner = *reinterpret_cast<unsigned char* const*>(
		pruning + *reinterpret_cast<const unsigned*>(pruning + 0x70) * 4 + 0x1c);
	return pruner ? *reinterpret_cast<const unsigned*>(pruner + 0xc) +
		*reinterpret_cast<const unsigned*>(pruner + 8) : 0;
	}

// phys_fn_003628 (0x00089bb0, 22 B)
// A row of gap:fluids\Fluid.cpp..fluids\FluidManager.cpp: the fluid
// manager's shape-change flag, byte +0x28 = 1 when byte +0x2b is set and the shape
// is static (second argument false).
static void nxFluidManagerShapeChanged(unsigned char* manager, void* shape, bool hasRecord)
	{
	(void)shape;
	if(manager[0x2b] && !hasRecord)
		manager[0x28] = 1;
	}

// phys_fn_001941 (0x0004ba80, 59 B)
// One shape into the collection: its prunable takes the type (+0x70 for a
// dynamic owner, else 0; 004888) and kind 0 (004890), then 004857.
static __declspec(noinline) void nxPruningAddShape(unsigned char* pruning, unsigned char* shape, bool hasRecord)
	{
	shape[0xce] = static_cast<unsigned char>(
		hasRecord ? *reinterpret_cast<unsigned*>(pruning + 0x70) : 0u);
	shape[0xcf] = 0;
	nxScenePrunerInsert(nxPruningScene(pruning), shape);
	}

// phys_fn_001943 (0x0004bac0, 270 B)
// A body's root into the collection: the root's +0xa0 = the collection,
// its prunable the type and kind 2 (group) or 1, then 004857; a group's
// children follow with kind 0; the root is appended to the +0x78 list
// (grown through 004840 when full). The cached object at +0x2c the row
// releases first is never set by the candidate.
static void nxPruningAddBody(unsigned char* pruning, unsigned char* body, bool hasRecord)
	{
	const unsigned char type = static_cast<unsigned char>(
		hasRecord ? *reinterpret_cast<unsigned*>(pruning + 0x70) : 0u);
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	const bool group = *reinterpret_cast<unsigned*>(root + 0xd0) == 5u;
	*reinterpret_cast<unsigned char**>(root + 0xa0) = pruning;
	root[0xce] = type;
	root[0xcf] = group ? 2 : 1;
	nxScenePrunerInsert(nxPruningScene(pruning), root);
	if(group)
		{
		unsigned char** child = *reinterpret_cast<unsigned char***>(root + 0xe0);
		const unsigned count = static_cast<unsigned>(
			*reinterpret_cast<unsigned char***>(root + 0xe4) - child);
		for(unsigned i = 0; i < count; ++i)
			{
			child[i][0xce] = type;
			child[i][0xcf] = 0;
			nxScenePrunerInsert(nxPruningScene(pruning), child[i]);
			}
		}
	SdkContainer* list = reinterpret_cast<SdkContainer*>(pruning + 0x78);
	if(list->mCount == list->mCapacity)
		list->resize(1);
	list->mEntries[list->mCount] = reinterpret_cast<NxU32>(root);
	++list->mCount;
	}

// phys_fn_001945 (0x0004bbd0, 74 B)
// A body's root and a group's current children leave their pruners (004859).
static void nxPruningRemoveBody(unsigned char* pruning, unsigned char* body)
	{
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	nxScenePrunerErase(nxPruningScene(pruning), root);
	if(*reinterpret_cast<unsigned*>(root + 0xd0) == 5u)
		{
		unsigned char** child = *reinterpret_cast<unsigned char***>(root + 0xe0);
		const unsigned count = static_cast<unsigned>(
			*reinterpret_cast<unsigned char***>(root + 0xe4) - child);
		for(unsigned i = 0; i < count; ++i)
			nxScenePrunerErase(nxPruningScene(pruning), child[i]);
		}
	}

// phys_fn_001955 (0x0004bde0, 153 B)
// The collection's pair records (+0x44 count, +0x48 array) that name the
// root are released first (0x4bdeb-0x4be37; the candidate keeps none, so
// the count is always 0 here); then the root leaves the +0x78 list, the
// last entry moving into its place.
static void nxPruningRemoveRootPairs(unsigned char* pruning, unsigned char* body)
	{
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	SdkContainer* list = reinterpret_cast<SdkContainer*>(pruning + 0x78);
	for(unsigned i = 0; i < list->mCount; ++i)
		if(list->mEntries[i] == reinterpret_cast<NxU32>(root))
			{
			--list->mCount;
			list->mEntries[i] = list->mEntries[list->mCount];
			return;
			}
	}

// phys_fn_001279 (0x00025760, 53 B)
// A row of gap:NpTriangleMeshShape.cpp..Shape.cpp. While +0xa0 names the collection: 001955 and 001945 on the owner body,
// then +0xa0 cleared.
static void nxShapeLeavePruning(unsigned char* shape)
	{
	unsigned char* body = *reinterpret_cast<unsigned char**>(shape + 4);
	if(*reinterpret_cast<unsigned char**>(shape + 0xa0))
		nxPruningRemoveRootPairs(*reinterpret_cast<unsigned char**>(shape + 0xa0), body);
	if(*reinterpret_cast<unsigned char**>(shape + 0xa0))
		{
		nxPruningRemoveBody(*reinterpret_cast<unsigned char**>(shape + 0xa0), body);
		*reinterpret_cast<unsigned char**>(shape + 0xa0) = 0;
		}
	}

// phys_fn_000531 (0x00010600, 97 B)
// Scene::addShape(shape, hasRecord): the shape's slot 6 with 1; 001943 on
// the collection with the shape's owner; 000503 with 001960 + 001957; and
// with a fluid manager at +0x61c, 003628.
void nxSceneAddShape(NxSceneInternal* scene, unsigned char* shape, bool hasRecord)
	{
	nxRuntimeShapeSlot6(shape, 1);
	unsigned char* pruning = scene->bytes() + 0x624;
	nxPruningAddBody(pruning, *reinterpret_cast<unsigned char**>(shape + 4), hasRecord);
	const unsigned indexed = nxPruningCountIndexed(pruning);
	nxSceneUpdateActorCount(scene, indexed + nxPruningCountFirst(pruning));
	if(scene->at<unsigned char*>(0x61c))
		nxFluidManagerShapeChanged(scene->at<unsigned char*>(0x61c), shape, hasRecord);
	}

// phys_fn_000533 (0x00010670, 40 B)
// A static shape leaves the Scene: 001279, then 003628 with false; returns
// true.
bool nxSceneRemoveStaticShape(NxSceneInternal* scene, unsigned char* shape)
	{
	nxShapeLeavePruning(shape);
	if(scene->at<unsigned char*>(0x61c))
		nxFluidManagerShapeChanged(scene->at<unsigned char*>(0x61c), shape, false);
	return true;
	}

// phys_fn_000535 (0x000106a0, 40 B)
// The same for a dynamic shape, 003628 with true.
static __declspec(noinline) bool nxSceneRemoveDynamicShape(NxSceneInternal* scene, unsigned char* shape)
	{
	nxShapeLeavePruning(shape);
	if(scene->at<unsigned char*>(0x61c))
		nxFluidManagerShapeChanged(scene->at<unsigned char*>(0x61c), shape, true);
	return true;
	}

// phys_fn_000006 (0x00001080, 30 B)
// A row of gap:<start>..Actor.cpp. The body's root, when it has one, leaves the Scene: 000535 when the body
// has a dynamic record, else 000533.
static void nxActorRemoveRootFromScene(unsigned char* body)
	{
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(!root) return;
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	if(*reinterpret_cast<void**>(body + 8))
		nxSceneRemoveDynamicShape(scene, root);
	else
		nxSceneRemoveStaticShape(scene, root);
	}

// phys_fn_000008 (0x000010a0, 751 B)
// A row of gap:<start>..Actor.cpp: the body's mass from its shapes,
// thiscall on the body (density, &totalMass, &pose, &diagonal), `ret 0x10`.
// The frame (0x34 B: the tensor at +0, the centre at +0x24, the mass at
// +0x30) is zeroed by 000847 with 1; the root's slot 4 (vtable +0x10) adds
// each family's frame at unit density (1.0f) with a zeroed vector as its
// third argument. A false return is 1 (the mesh-inertia failure); a mass
// that is not above zero (ordered: NaN passes, `test ah,0x41; jp`) is 2.
// Otherwise pose.t = the centre (integer copies), the frame moves to its
// centre (0x1c720, 000841: 000833 with the negated centre), and the
// tensor is scaled into the nine-word local:
// - density > 0 and totalMass > 0 (0x118e): each word times the density;
// - density > 0 only (0x1242): *totalMass = mass * density, then each word
//   times the density;
// - otherwise (density not above zero, NaN included; 0x12e6): the register
//   ratio totalMass / mass (not rounded to float) times each word.
// Each product is rounded once at its fstp. The import [0x101041b8]
// NxDiagonalizeInertiaTensor(tensor, diagonal, pose.M) ends it (its result
// is not tested); 0x2ea70, the frame's destructor, is an empty `ret`.
typedef bool (__thiscall* NxRuntimeShapeMassFn)(void* shape, void* frame, float density,
	void* reserved);

// ObjectModel.cpp's MassFrame (its layout) and the two frame rows 000008
// calls: 000847 (0x1c880, conditional zero) and 000833 (0x1c040, translate).
struct NxActorMassFrame
	{
	float inertia[9];
	float offset[3];
	float mass;
	};
void nxMassFrameConditionalZeroAt(void* frame, unsigned flag);
void nxMassFrameTranslateAt(void* frame, const void* displacement);

int nxActorComputeMassFromShapes(unsigned char* body, float density, float* totalMass,
	NxMat34* pose, NxVec3* diagonal)
	{
	NxActorMassFrame frame;
	nxMassFrameConditionalZeroAt(&frame, 1);
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	unsigned reserved[3] = { 0, 0, 0 };
	void** table = *reinterpret_cast<void***>(root);
	if(!reinterpret_cast<NxRuntimeShapeMassFn>(table[4])(root, &frame, 1.0f, reserved))
		return 1;
	if(frame.mass <= 0.0f)
		return 2;
	memcpy(&pose->t, frame.offset, sizeof(NxVec3));
	// 0x1c720 (row 000841): fld; fchs; fstp of each centre word, then 000833.
	float centre[3];
	for(unsigned i = 0; i < 3; ++i)
		nxX87NegateFloat(&frame.offset[i], &centre[i]);
	nxMassFrameTranslateAt(&frame, centre);
	float tensor[9];
	if(density > 0.0f)
		{
		if(!(*totalMass > 0.0f))
			*totalMass = static_cast<float>(static_cast<double>(frame.mass) * density);
		for(unsigned i = 0; i < 9; ++i)
			tensor[i] = static_cast<float>(static_cast<double>(frame.inertia[i]) * density);
		}
	else
		{
		const double ratio = static_cast<double>(*totalMass) / frame.mass;
		for(unsigned i = 0; i < 9; ++i)
			tensor[i] = static_cast<float>(static_cast<double>(frame.inertia[i]) * ratio);
		}
	NxMat33 dense;
	memcpy(&dense, tensor, sizeof(tensor));
	NxDiagonalizeInertiaTensor(dense, *diagonal, pose->M);
	return 0;
	}

// phys_fn_000036 (0x00002250, 420 B)
// Actor::createShape(desc) on the body (Actor.cpp). Under the reentry flag
// (set: report code 2, line 0x150, return 0), the factory builds the
// shape, then by the body's root (+0x10):
// - none (0x23c8): the shape becomes the root and, when built, 000531 adds
//   it with hasRecord = (body+8 != 0);
// - a group (0x22b3): 001041 appends it, its slot 6 runs with 1, 001941
//   adds it to the collection, 000503 takes 001957 + 001960 and a fluid
//   manager gets 003628;
// - a single shape (0x2344): the root leaves the Scene (000535 or 000533),
//   a 0x110-byte group is allocated through [0x101041bc] and constructed
//   (001033) with an id from the pool (000012), it becomes the root with
//   +8 = [scene+0x540] - 1, 001041 adds the old root and then the new
//   shape, and 000531 adds the group.
// The flag is cleared and the new shape (not the group) returned. The
// oracle does not test the factory's result on the group arms (001041
// reads [shape+0x9c]) nor the group allocation (+8 is written through it);
// the candidate returns before either would fault.
unsigned char* nxActorCreateShape(unsigned char* body, const NxShapeDesc* descriptor)
	{
	if(gNxApiReentry)
		{
		nxActorCppReport(NXE_INVALID_OPERATION, 0x150, gNxActorReentryMessage);
		return 0;
		}
	gNxApiReentry = true;
	unsigned char* shape = nxActorShapeFactory(descriptor, body);
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	if(!root)
		{
		*reinterpret_cast<unsigned char**>(body + 0x10) = shape;
		if(shape)
			nxSceneAddShape(scene, shape, *reinterpret_cast<void**>(body + 8) != 0);
		}
	else if(!shape)
		{
		}
	else if(*reinterpret_cast<unsigned*>(root + 0xd0) == 5u)
		{
		nxShapeGroupAddChild(root, shape);
		nxRuntimeShapeSlot6(shape, 1);
		const bool hasRecord = *reinterpret_cast<void**>(body + 8) != 0;
		unsigned char* pruning = scene->bytes() + 0x624;
		nxPruningAddShape(pruning, shape, hasRecord);
		const unsigned first = nxPruningCountFirst(pruning);
		nxSceneUpdateActorCount(scene, first + nxPruningCountIndexed(pruning));
		if(scene->at<unsigned char*>(0x61c))
			nxFluidManagerShapeChanged(scene->at<unsigned char*>(0x61c), shape, hasRecord);
		}
	else
		{
		if(*reinterpret_cast<void**>(body + 8))
			nxSceneRemoveDynamicShape(scene, root);
		else
			nxSceneRemoveStaticShape(scene, root);
		void* memory = nxFoundationSDKAllocator->malloc(0x110, NX_MEMORY_PERSISTENT);
		if(memory)
			{
			unsigned char* group = nxShapeGroupConstructAt(memory, body,
				nxIdAllocNext(scene->bytes() + 0x6e4));
			*reinterpret_cast<unsigned char**>(body + 0x10) = group;
			*reinterpret_cast<unsigned*>(group + 8) = scene->at<unsigned>(0x540) - 1u;
			nxShapeGroupAddChild(group, root);
			nxShapeGroupAddChild(group, shape);
			nxSceneAddShape(scene, group, *reinterpret_cast<void**>(body + 8) != 0);
			}
		else
			*reinterpret_cast<unsigned char**>(body + 0x10) = 0;
		}
	gNxApiReentry = false;
	return shape;
	}

// phys_fn_000024 (0x00001860, 328 B)
// Actor::releaseShape(internal shape) on the body (Actor.cpp). Under the
// reentry flag (report code 2, line 0x186); every other report is code 1:
// - no root: line 0x1a4, "shape not found!";
// - a group holding one child on a static body: line 0x18f, the static
//   actor may not be left without shapes (nothing is removed, even when
//   the shape is not that child);
// - a group otherwise: 001028 unlinks the shape (a shape it does not hold
//   is ignored silently); a group left empty leaves the Scene (000006), is
//   deleted (slot 0 with 1) and the root cleared;
// - a single root on a static body: line 0x19c (the same message as 0x18f);
// - a single root that is not the shape: line 0x19d, "shape not found!";
// - the single root itself: 000006, the shape deleted, the root cleared.
void nxActorReleaseShape(unsigned char* body, unsigned char* shape)
	{
	if(gNxApiReentry)
		{
		nxActorCppReport(NXE_INVALID_OPERATION, 0x186, gNxActorReentryMessage);
		return;
		}
	gNxApiReentry = true;
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	const bool hasRecord = *reinterpret_cast<void**>(body + 8) != 0;
	if(!root)
		nxActorCppReport(NXE_INVALID_PARAMETER, 0x1a4, "Actor::releaseShape: shape not found!");
	else if(*reinterpret_cast<unsigned*>(root + 0xd0) == 5u)
		{
		const unsigned bytes = static_cast<unsigned>(
			*reinterpret_cast<unsigned char**>(root + 0xe4) -
			*reinterpret_cast<unsigned char**>(root + 0xe0));
		if((bytes & ~3u) == 4 && !hasRecord)
			nxActorCppReport(NXE_INVALID_PARAMETER, 0x18f, "Actor::releaseShape: "
				"Can't release shape: A static actor can't be left with no shapes!");
		else if(nxShapeGroupRemoveChild(root, shape))
			{
			const unsigned left = static_cast<unsigned>(
				*reinterpret_cast<unsigned char**>(root + 0xe4) -
				*reinterpret_cast<unsigned char**>(root + 0xe0));
			if(!(left & ~3u))
				{
				nxActorRemoveRootFromScene(body);
				unsigned char* emptied = *reinterpret_cast<unsigned char**>(body + 0x10);
				if(emptied)
					{
					nxRuntimeShapeDeleteRoot(emptied);
					*reinterpret_cast<unsigned char**>(body + 0x10) = 0;
					}
				}
			}
		}
	else if(!hasRecord)
		nxActorCppReport(NXE_INVALID_PARAMETER, 0x19c, "Actor::releaseShape: "
			"Can't release shape: A static actor can't be left with no shapes!");
	else if(root != shape)
		nxActorCppReport(NXE_INVALID_PARAMETER, 0x19d, "Actor::releaseShape: shape not found!");
	else
		{
		nxActorRemoveRootFromScene(body);
		unsigned char* single = *reinterpret_cast<unsigned char**>(body + 0x10);
		if(single)
			{
			nxRuntimeShapeDeleteRoot(single);
			*reinterpret_cast<unsigned char**>(body + 0x10) = 0;
			}
		}
	gNxApiReentry = false;
	}

// ---------------------------------------------------------------------------
// The Scene's joint rows (joint-open-items Task 2; units/joint-open-items-
// contract.md "## Scene joint rows"). Every one is a thiscall member on the
// Scene, transcribed from the Capstone listing. Joint fields by offset:
// +0x08/+0x0c mBody, +0x10 mNextJoint, +0x2c mFlags (bit 0: on the +0x59c
// list), +0x30 mScene, +0x34 the island link 000778 clears.
// ---------------------------------------------------------------------------

#define NX_SCENE_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\Scene.cpp"

// phys_fn_000661 (0x00013e00, 290 B, phase 7) is Scene::addJoint.
// A joint whose flag bit 0 is already set is reported (the Foundation
// instance test with int3, code 2, line 0x752) and left alone. Otherwise:
// bit 0 set, the joint pushed on the +0x59c list through +0x10, appended to
// the +0x58c pointer array (grown to 2n + 2 entries when full,
// 0x13e53-0x13f12), and mScene = this written last (0x13f1a). noinline:
// the oracle calls it as its own function (0x14524, 0x97e4e), and the
// compiler otherwise folds it into createJoint, where no breakpoint on the
// row can see it run.
__declspec(noinline) void NxSceneInternal::addJoint(Joint* joint)
	{
	if(joint->mFlags & 1)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_SCENE_CPP, 0x752, 0,
			"Scene::addJoint: joint is already in a scene.");
		return;
		}
	joint->mFlags |= 1;
	joint->mNextJoint = at<Joint*>(0x59c);
	at<Joint*>(0x59c) = joint;
	nxJointPointerArrayPush(&at<void*>(0x58c), joint);
	joint->mScene = this;
	}

// phys_fn_000633 (0x00012660, 370 B, phase 7) is Scene::removeJoint.
// Flag bit 0 clear: the joint is unlinked from the +0x5a0 list (head or
// successor), or reported as not in the scene (code 0xce, NXE_DB_WARNING,
// line 0x77c); nothing else is written. Bit 0 set: the island of the
// joint's first non-null body is dissolved through phys_fn_000778 into the
// +0x58c array; every occurrence of the joint in that array is replaced by
// the last entry (the swapped-in entry is not re-tested, as the listing's
// index advances past it); the joint is unlinked from the +0x59c list, or
// reported (code 2, line 0x7a6) and left as it is; then its link, flag bit
// 0 and mScene are cleared (0x1276e-0x12779).
void NxSceneInternal::removeJoint(Joint* joint)
	{
	if(!(joint->mFlags & 1))
		{
		Joint* head = at<Joint*>(0x5a0);
		if(joint == head)
			{
			at<void*>(0x5a0) = joint->mNextJoint;
			return;
			}
		for(Joint* link = head; link; link = static_cast<Joint*>(link->mNextJoint))
			{
			if(link->mNextJoint == joint)
				{
				link->mNextJoint = joint->mNextJoint;
				return;
				}
			}
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING, NX_SCENE_CPP, 0x77c, 0,
			"Scene::removeJoint: joint is not in the scene.");
		return;
		}

	void* body = joint->mBody[0];
	if(!body)
		body = joint->mBody[1];
	if(body)
		reinterpret_cast<Row000778Fixture*>(body)->row000778(joint, &at<void*>(0x58c));

	for(NxU32 i = 0; i < (NxU32)((at<NxU8*>(0x590) - at<NxU8*>(0x58c)) >> 2); i++)
		{
		Joint** entries = at<Joint**>(0x58c);
		if(entries[i] != joint)
			continue;
		const NxU32 last = (NxU32)((at<NxU8*>(0x590) - reinterpret_cast<NxU8*>(entries)) >> 2) - 1;
		if(i != last)
			entries[i] = entries[last];
		at<NxU8*>(0x590) -= 4;
		}

	Joint* head = at<Joint*>(0x59c);
	if(joint == head)
		{
		at<void*>(0x59c) = joint->mNextJoint;
		}
	else
		{
		Joint* link = head;
		for(; link; link = static_cast<Joint*>(link->mNextJoint))
			if(link->mNextJoint == joint)
				break;
		if(!link)
			{
			NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_SCENE_CPP, 0x7a6, 0,
				"Scene::removeJoint: joint is not in the scene.");
			return;
			}
		link->mNextJoint = joint->mNextJoint;
		}
	joint->mNextJoint = 0;
	joint->mFlags &= ~1u;
	joint->mScene = 0;
	}

// phys_fn_000557 (0x00010840, 22 B)
// The joint is pushed on the +0x5a0 list through its +0x10 link.
void NxSceneInternal::pushJointWithoutBodies(Joint* joint)
	{
	joint->mNextJoint = at<Joint*>(0x5a0);
	at<Joint*>(0x5a0) = joint;
	}

// phys_fn_000653 (0x00013760, 126 B, phase 7) is Scene::releaseJoint.
// The re-entry flag is createJoint's (.data 0x00123c10); a re-entrant call is
// reported with the message the pointer at .data 0x00122050 names (code 2,
// line 0x4e2). Otherwise: removeJoint, the joint's scalar deleting
// destructor (slot 5 with 1) when the pointer is non-null, --[+0x6c8] and
// the enumeration cursor reset to the list head, then the flag cleared.
void NxSceneInternal::releaseJoint(Joint* joint)
	{
	if(gNxApiReentry)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_SCENE_CPP, 0x4e2, 0,
			"Reentry check: You may not call this API method from a callback!");
		return;
		}
	gNxApiReentry = true;
	removeJoint(joint);
	if(joint)
		delete joint;
	--at<NxU32>(0x6c8);
	at<void*>(0x6bc) = at<void*>(0x59c);
	gNxApiReentry = false;
	}

// phys_fn_000598 (0x00010f50, 138 B, phase 7) grows the 0x50-byte record
// array phys_fn_004093 takes records from: capacity (+0x5c0) doubled, or 4
// from empty, stored first; a new block of capacity * 0x50 bytes; the
// +0x5bc used records copied (rep movsd/movsb); the old block freed and the
// pointer zeroed before the new one is stored. noinline: the oracle calls it
// as its own function (004093, 0x95db7), and the compiler otherwise folds it
// into the joint rows, where no breakpoint on the row can see it run.
__declspec(noinline) void NxSceneInternal::growJointRecords()
	{
	NxU32 capacity = at<NxU32>(0x5c0);
	capacity = capacity ? capacity + capacity : 4;
	at<NxU32>(0x5c0) = capacity;
	void* block = nxFoundationSDKAllocator->malloc(capacity * 0x50, NX_MEMORY_PERSISTENT);
	const NxU32 count = at<NxU32>(0x5bc);
	if(count)
		memcpy(block, at<void*>(0x5b8), count * 0x50);
	if(at<void*>(0x5b8))
		{
		nxFoundationSDKAllocator->free(at<void*>(0x5b8));
		at<void*>(0x5b8) = 0;
		}
	at<void*>(0x5b8) = block;
	}

// phys_fn_000571 (0x000108e0, 22 B, phase 7): the break event's +4 takes the
// old head of the +0x620 list and the event becomes the head. The event is
// not tested for null, as in the listing. noinline: the oracle calls it as
// its own function (004111 0x98029/0x98038, 004374, 004308).
__declspec(noinline) void NxSceneInternal::addJointBreakEvent(JointBreakEvent* event)
	{
	event->mNext = at<JointBreakEvent*>(0x620);
	at<JointBreakEvent*>(0x620) = event;
	}

// phys_fn_000559 (0x00010860, 7 B, phase 7): the joint count at +0x6c8. It
// counts createJoint's calls that reach the type switch (every exit after it
// increments the count, 0x14529) less releaseJoint's, not the list.
NxU32 NxSceneInternal::getNbJoints() const
	{
	return at<NxU32>(0x6c8);
	}

// phys_fn_000563 (0x00010880, 13 B, phase 7): the cursor at +0x6bc = the list
// head at +0x59c.
void NxSceneInternal::resetJointIterator()
	{
	at<void*>(0x6bc) = at<void*>(0x59c);
	}

// phys_fn_000567 (0x000108a0, 23 B, phase 7): the joint under the cursor,
// which advances through Joint +0x10; 0 at the end.
Joint* NxSceneInternal::getNextJoint()
	{
	Joint* joint = at<Joint*>(0x6bc);
	if(!joint)
		return 0;
	at<void*>(0x6bc) = joint->mNextJoint;
	return joint;
	}
