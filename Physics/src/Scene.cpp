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

#include <stdio.h>
#include <string.h>
#include <new>

// Public shape final and descriptor loader share the oracle's global name map.
void nxShapeSetName(void* shape, const char* name);
void nxShapeFactoryInitializePose(void* shape, const void* localPose);
void nxShapeFactoryRefreshPose(void* shape);
void nxShapeFactoryInstallVtable(void* shape, unsigned type);
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
// The pruning engine at +0x624 (opcode/IcePruningEngine.cpp) and the shape
// prunable (ObjectModel.cpp); scene-raycast block Task 3.
bool nxSceneEngineAddShape(void* engine, void* shape, unsigned type, unsigned section);
bool nxSceneEngineRemoveShape(void* engine, void* shape);
void nxSceneEngineDestroyPruners(void* engine);
void nxSceneEngineSetExternalBuffer(void* engine, unsigned capacity, void* entries);
void nxShapeFactoryInstallPrunable(void* shape);
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
// phys_fn_00001de0 (0x00001de0, phase 5): the shape factory. REPRODUCTION HOLE;
// it returns a non-null shape object, which is what the caller tests.
void* nxShapeFactory(void* shapeDesc, void* actor);
// The multi-shape group builder (0x110 bytes). REPRODUCTION HOLE.
void* nxShapeGroupConstruct(void* actor, const unsigned* shapeDescriptions, unsigned count);


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

bool nxOpcodeEnsurePool()
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

// Registers a body's shape with the engine's pruner of the given type (0
// static, 2 dynamic): a single shape in section 1; a compound's children in
// section 0 and its group shape in section 2 (the pinned DLL's pool layout,
// scene-raycast block Task 3).
static void nxSceneEngineAddRoot(NxSceneInternal* scene, unsigned char* shape, unsigned type)
	{
	void* engine = &scene->at<unsigned char>(0x624);
	if(*reinterpret_cast<unsigned*>(shape + 0xd0) == 5u)
		{
		void** first = *reinterpret_cast<void***>(shape + 0xe0);
		void** last = *reinterpret_cast<void***>(shape + 0xe4);
		for(void** child = first; child && child != last; ++child)
			nxSceneEngineAddShape(engine, *child, type, 0);
		nxSceneEngineAddShape(engine, shape, type, 2);
		}
	else
		nxSceneEngineAddShape(engine, shape, type, 1);
	}

static void nxSceneEngineRemoveRoot(NxSceneInternal* scene, unsigned char* shape)
	{
	void* engine = &scene->at<unsigned char>(0x624);
	if(*reinterpret_cast<unsigned*>(shape + 0xd0) == 5u)
		{
		void** first = *reinterpret_cast<void***>(shape + 0xe0);
		void** last = *reinterpret_cast<void***>(shape + 0xe4);
		for(void** child = first; child && child != last; ++child)
			nxSceneEngineRemoveShape(engine, *child);
		}
	nxSceneEngineRemoveShape(engine, shape);
	}

void nxSceneBroadphaseRegister(NxSceneInternal* scene, void* bodyPointer)
	{
	unsigned char* body = static_cast<unsigned char*>(bodyPointer);
	if(!body || !*reinterpret_cast<void**>(body + 8)) return;
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(!shape) return;
	nxSceneEngineAddRoot(scene, shape, 2);
	nxSceneTrackShape(scene, shape);
	}

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
	nxSceneEngineRemoveRoot(scene, shape);
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
// body graph. Callback and name paths remain separate gaps.
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
	nxSceneBroadphaseUnregister(this, body);
	--last;
	*found = *last;
	at<NxActor**>(0x560) = last;

	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	if(record)
		{
		nxSceneAuxUnregisterRecord(this, record);
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
	const unsigned actorId = *reinterpret_cast<unsigned*>(body + 0xc);
	nxGetSdkAllocator()->free(actor);
	nxShapeSetName(body, 0);
	if(record)
		{
		nxSceneRecycleRecordId(this,
			*reinterpret_cast<unsigned*>(record + 0x11c));
		nxGetSdkAllocator()->free(record);
		}
	nxSceneRecycleActorId(this, actorId);
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(shape && *reinterpret_cast<unsigned*>(shape + 0xd0) == 5u)
		{
		void** shapes = *reinterpret_cast<void***>(shape + 0xe0);
		void** shapesEnd = *reinterpret_cast<void***>(shape + 0xe4);
		void** helpers = *reinterpret_cast<void***>(shape + 0xf0);
		for(unsigned i = 0; i < static_cast<unsigned>(shapesEnd - shapes); ++i)
			{
			const unsigned id = *reinterpret_cast<unsigned*>(
				static_cast<unsigned char*>(shapes[i]) + 0xd4);
			nxSceneAuxUnregisterShape(this, shapes[i]);
			nxGetSdkAllocator()->free(helpers[i]);
			nxShapeSetName(shapes[i], 0);
			nxGetSdkAllocator()->free(shapes[i]);
			nxSceneRecycleShapeId(this, id);
			}
		nxGetSdkAllocator()->free(helpers);
		nxGetSdkAllocator()->free(shapes);
		nxSceneAuxUnregisterShape(this, shape);
		nxSceneRecycleShapeId(this, *reinterpret_cast<unsigned*>(shape + 0xd4));
		nxShapeSetName(shape, 0);
		nxGetSdkAllocator()->free(shape);
		}
	else if(shape)
		{
		const unsigned id = *reinterpret_cast<unsigned*>(shape + 0xd4);
		void* helper = *reinterpret_cast<void**>(shape + 0x9c);
		nxSceneAuxUnregisterShape(this, shape);
		if(helper) nxGetSdkAllocator()->free(helper);
		nxShapeSetName(shape, 0);
		nxGetSdkAllocator()->free(shape);
		nxSceneRecycleShapeId(this, id);
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
	gCreateJointReentry = false;
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
	// The pruning engine's pruners: each one's tree (static), its world boxes
	// and objects, then its storage (opcode/IcePruner.cpp).
	nxSceneEngineDestroyPruners(static_cast<unsigned char*>(self) + 0x624);
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

void nxSceneActorDestroy(NxActor* actor)
	{
	(void)actor;
	}

void nxSceneUpdateActorCount(void* scene, unsigned count)
	{
	unsigned char* bytes = static_cast<unsigned char*>(scene);
	unsigned& capacity = *reinterpret_cast<unsigned*>(bytes + 4);
	if(count <= capacity) return;
	capacity = (count + 0xffu) & ~0xffu;
	for(unsigned offset = 8; offset <= 0xc; offset += 4)
		{
		void* old = *reinterpret_cast<void**>(bytes + offset);
		if(old) nxGetSdkAllocator()->free(old);
		void* next = nxGetSdkAllocator()->malloc(capacity * 4,
			NX_MEMORY_PERSISTENT);
		if(next) memset(next, 0, capacity * 4);
		*reinterpret_cast<void**>(bytes + offset) = next;
		}
	*reinterpret_cast<unsigned*>(bytes + 0x14) = capacity;
	// 0x00010137-0x0001017e: the three embedded containers at +0x50, +0x500
	// (the scene queries' result collector) and +0x510 borrow the +0x0c buffer
	// (phys_fn_004847), and the pruning engine at +0x624 hands it to every
	// pruner through slot 4 (phys_fn_004861); scene-raycast block Task 3.
	NxU32* shared = *reinterpret_cast<NxU32**>(bytes + 0xc);
	reinterpret_cast<SdkContainer*>(bytes + 0x50)->setExternalBuffer(capacity, shared);
	reinterpret_cast<SdkContainer*>(bytes + 0x500)->setExternalBuffer(capacity, shared);
	reinterpret_cast<SdkContainer*>(bytes + 0x510)->setExternalBuffer(capacity, shared);
	nxSceneEngineSetExternalBuffer(bytes + 0x624, capacity, shared);
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


int nxActorComputeMass(void* actor, const unsigned* bodyWord)
	{
	// The dynamic record is built after the shape and its helper, in the
	// order the guarded oracle allocator reports. bodyWord addresses the
	// descriptor's body POINTER at d[0xc], rather than the body descriptor.
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
	memcpy(record + 0x18, body + 0x44, 12);

	// The dynamic record carries a quaternion at +0x5c with w last. Convert
	// the descriptor's matrix already copied to body+0x20 the way the body
	// pose constructor phys_fn_000801 does (0x1b82e-0x1b987): it writes +0x24
	// and copies it to +0x5c. The public NxQuat(NxMat33) conversion rounds its
	// intermediates to float and differs from it in the last bit for a general
	// rotation (joint-open-items Task 4, rotated fixture actor b).
	float quaternion[4];
	nxNpActorBodyQuaternionFromMatrix(reinterpret_cast<const float*>(body + 0x20), quaternion);
	memcpy(record + 0x24, quaternion, sizeof(quaternion));
	memcpy(record + 0x5c, quaternion, sizeof(quaternion));

	*reinterpret_cast<void**>(record + 0x19c) = body;
	*reinterpret_cast<void**>(body + 0x08) = record;
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(actorBytes + 4);
	const NxBodyDesc* bodyDesc = reinterpret_cast<const NxBodyDesc*>(*bodyWord);
	bodyDesc->massLocalPose.M.getRowMajor(reinterpret_cast<float*>(record + 0xdc));
	memcpy(record + 0x100, &bodyDesc->massLocalPose.t, sizeof(NxVec3));
	// The world centre of mass (+0x158) is written with +0x134, +0x124 and
	// +0x164 by nxNpActorUpdateMassFrame below.
	*reinterpret_cast<unsigned*>(record + 0x10c) = bodyDesc->flags;
	*reinterpret_cast<unsigned*>(record + 0x110) =
		bodyDesc->solverIterationCount;
	*reinterpret_cast<unsigned char**>(record + 0x120) =
		scene->at<unsigned char*>(0x48);
	*reinterpret_cast<unsigned*>(record + 0x11c) = nxSceneTakeRecordId(scene);
	*reinterpret_cast<unsigned char**>(record + 0x1bc) = record;
	*reinterpret_cast<unsigned char**>(record + 0x1e8) = record;
	// The body's JointSupportBody pointer: the oracle's body constructor
	// Row 000797 stores 0 at +0x204 (0x1b713), after +0x1e4/+0x1e0. Only
	// the simulation step's phys_fn_000611 (0x11305) points it at an element
	// of the Scene's +0x5ac array; the candidate has no step, so it stays 0
	// (joint-open-items-contract.md "## Body record +0x204"). No allocation.
	reinterpret_cast<JointBodyRecord*>(record)->mUnknown204 = 0;
	*reinterpret_cast<float*>(record + 0xb8) = bodyDesc->linearDamping;
	*reinterpret_cast<float*>(record + 0xbc) = bodyDesc->angularDamping;
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
	float mass = bodyDesc->mass;
	NxVec3 inertia = bodyDesc->massSpaceInertia;
	// The one-box density path in FUN_100019b0/FUN_1001a350 derives mass
	// from the full box extents and diagonal inertia from their squared radii.
	// Rotated/translated and compound geometry still need the full tensor path.
	const unsigned* actorDesc = bodyWord - 0x0c;
	float density;
	memcpy(&density, actorDesc + 0x0d, sizeof(density));
	if(mass == 0.0f && density > 0.0f &&
		actorDesc[0x14] - actorDesc[0x13] == sizeof(void*))
		{
		const NxShapeDesc* shape = *reinterpret_cast<const NxShapeDesc* const*>(
			actorDesc[0x13]);
		if(shape && shape->getType() == NX_SHAPE_BOX)
			{
			const NxVec3& radii = static_cast<const NxBoxShapeDesc*>(shape)->dimensions;
			mass = 8.0f * density * radii.x * radii.y * radii.z;
			const float thirdMass = mass / 3.0f;
			inertia.x = thirdMass * (radii.y * radii.y + radii.z * radii.z);
			inertia.y = thirdMass * (radii.x * radii.x + radii.z * radii.z);
			inertia.z = thirdMass * (radii.x * radii.x + radii.y * radii.y);
			}
		}
	if(mass > 0.0f)
		{
		*reinterpret_cast<float*>(record + 0x188) = mass;
		*reinterpret_cast<float*>(record + 0xc0) = 1.0f / mass;
		}
	if(inertia.x > 0.0f && inertia.y > 0.0f && inertia.z > 0.0f)
		{
		memcpy(record + 0x18c, &inertia, sizeof(NxVec3));
		*reinterpret_cast<float*>(record + 0xc4) =
			1.0f / inertia.x;
		*reinterpret_cast<float*>(record + 0xc8) =
			1.0f / inertia.y;
		*reinterpret_cast<float*>(record + 0xcc) =
			1.0f / inertia.z;
		}
	// The oracle's creation path refreshes the mass frame through
	// Row 000768 (called from 000795 at 0x1b497): +0x134, +0x158, +0x124
	// and +0x164 from the +0x24 quaternion and +0x18 position. Joint-open-items
	// Task 4 found the earlier sequence one bit off on rotated bodies (the
	// fixed and prismatic relative rotations read +0x124/+0x134/+0x158).
	nxNpActorUpdateMassFrame(record);
	*reinterpret_cast<unsigned*>(record + 0x198) = 2;
	nxSceneAuxRegisterRecord(scene, record);

	return 0;
	}

// A static actor's shape goes to the engine's static pruner (type 0), created
// on the first registration; its pool grows from four entries (0x60 bytes of
// boxes, 0x10 of object pointers) by doubling. These allocations occur in
// Actor::loadFromDescInternal, before Scene::createActor grows its public actor
// list.
static void nxSceneStaticPrunerRegister(NxSceneInternal* scene, unsigned char* shape)
	{
	if(!scene || !shape) return;
	nxSceneEngineAddRoot(scene, shape, 0);
	nxSceneTrackShape(scene, shape);
	unsigned char* pruner = scene->at<unsigned char*>(0x640);
	nxSceneUpdateActorCount(scene, pruner ? *reinterpret_cast<unsigned short*>(pruner + 0x10) : 0);
	}

static void nxSceneStaticPrunerUnregister(NxSceneInternal* scene, unsigned char* shape)
	{
	nxSceneUntrackShape(scene, shape);
	if(!shape) return;
	nxSceneEngineRemoveRoot(scene, shape);
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
	if(!record)
		{
		nxSceneStaticPrunerRegister(static_cast<NxSceneInternal*>(scene),
			static_cast<unsigned char*>(object));
		return;
		}
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
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
	unsigned char* bytes = static_cast<unsigned char*>(scene);
	nxSceneArrayReserve(bytes + 0x56c, 1);
	void** last = *reinterpret_cast<void***>(bytes + 0x570);
	if(last)
		{
		*last = record;
		*reinterpret_cast<void***>(bytes + 0x570) = last + 1;
		nxSceneUpdateActorCount(scene, static_cast<unsigned>(
			*reinterpret_cast<void***>(bytes + 0x570) -
			*reinterpret_cast<void***>(bytes + 0x56c)));
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

// phys_fn_00001de0 (0x00001de0, phase 5): the shape factory. REPRODUCTION HOLE.
//
// The oracle builds a shape object from the descriptor and links it into the actor.
// What the caller of this function checks is that it returned something, so this
// returns a small allocation through the SDK allocator rather than null -- an actor
// whose shape list is empty cannot be created, and every downstream path in this
// reconstruction needs creation to succeed before it can be measured.
//
// The body points to the measured 0x228-byte internal shape; its +0x9c word
// reaches a separate 0x1c-byte public handle. Most internal fields and public
// virtual methods remain to be reconstructed.
void* nxShapeFactory(void* shapeDesc, void* actor)
	{
	unsigned char* shape = static_cast<unsigned char*>(
		nxGetSdkAllocator()->malloc(0x228, NX_MEMORY_PERSISTENT));
	if(!shape)
		return 0;
	memset(shape, 0, 0x228);
	nxShapeFactoryInstallPrunable(shape);
	const NxShapeDesc* descriptor = static_cast<const NxShapeDesc*>(shapeDesc);
	if(descriptor)
		{
		nxShapeFactoryInstallVtable(shape,
			static_cast<unsigned>(descriptor->getType()));
		*reinterpret_cast<unsigned*>(shape + 0xd0) =
			static_cast<unsigned>(descriptor->getType());
		*reinterpret_cast<NxCollisionGroup*>(shape + 0xd8) = descriptor->group;
		*reinterpret_cast<NxMaterialIndex*>(shape + 0xda) = descriptor->materialIndex;
		*reinterpret_cast<unsigned*>(shape + 0xc8) = 1u << descriptor->group;
		*reinterpret_cast<NxU16*>(shape + 0xde) =
			static_cast<NxU16>(descriptor->shapeFlags);
		if(descriptor->getType() == NX_SHAPE_BOX)
			memcpy(shape + 0xe4,
				&static_cast<const NxBoxShapeDesc*>(descriptor)->dimensions,
				sizeof(NxVec3));
		else if(descriptor->getType() == NX_SHAPE_SPHERE)
			memcpy(shape + 0xe0,
				&static_cast<const NxSphereShapeDesc*>(descriptor)->radius,
				sizeof(float));
		else if(descriptor->getType() == NX_SHAPE_CAPSULE)
			{
			const NxCapsuleShapeDesc* capsule =
				static_cast<const NxCapsuleShapeDesc*>(descriptor);
			memcpy(shape + 0xe0, &capsule->radius, sizeof(float));
			const float halfHeight = capsule->height * 0.5f;
			memcpy(shape + 0xe4, &halfHeight, sizeof(float));
			}
		else if(descriptor->getType() == NX_SHAPE_PLANE)
			{
			const NxPlaneShapeDesc* plane =
				static_cast<const NxPlaneShapeDesc*>(descriptor);
			nxShapeFactoryInitializePlane(shape, &plane->normal.x, plane->d);
			}
		}
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(
		static_cast<unsigned char*>(actor) + 4);
	*reinterpret_cast<unsigned*>(shape + 0xd4) = nxSceneTakeShapeId(scene);
	nxSceneAuxRegisterShape(scene, shape);
	// The oracle shape does not point at the public 0x18-byte actor wrapper.
	void* helper = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	if(!helper)
		{
		nxSceneAuxUnregisterShape(scene, shape);
		nxSceneRecycleShapeId(scene, *reinterpret_cast<unsigned*>(shape + 0xd4));
		nxGetSdkAllocator()->free(shape);
		return 0;
		}
	memset(helper, 0, 0x1c);
	if(descriptor)
		*reinterpret_cast<void**>(helper) = nxShapePublicVtable(
			static_cast<unsigned>(descriptor->getType()));
	*reinterpret_cast<void**>(shape + 0x9c) = helper;
	*reinterpret_cast<void**>(static_cast<unsigned char*>(helper) + 8) = shape;
	*reinterpret_cast<void**>(static_cast<unsigned char*>(helper) + 0x18) = shape;
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	if(body)
		{
		*reinterpret_cast<void**>(body + 0x10) = shape;
		*reinterpret_cast<void**>(shape + 4) = body;
		nxShapeFactoryInitializePose(shape,
			descriptor ? &descriptor->localPose : nullptr);
		}
	if(descriptor && descriptor->name)
		nxShapeSetName(shape, descriptor->name);
	return shape;
	}

// The multi-shape group builder. The two parallel child arrays match the
// observed two-box layout; shape registration and spatial bookkeeping remain
// reproduction holes.
void* nxShapeGroupConstruct(void* actor, const unsigned* shapeDescriptions, unsigned count)
	{
	unsigned char* group = static_cast<unsigned char*>(
		nxGetSdkAllocator()->malloc(0x110, NX_MEMORY_PERSISTENT));
	if(!group)
		return 0;
	memset(group, 0, 0x110);
	nxShapeFactoryInstallPrunable(group);
	*reinterpret_cast<unsigned*>(group + 0xd0) = 5;
	// Like the oracle, the group retains its 0x50-byte outer body at +4.
	// Its owned children are reached through the two arrays near the end.
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	*reinterpret_cast<void**>(group + 4) = body;
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(
		static_cast<unsigned char*>(actor) + 4);
	*reinterpret_cast<unsigned*>(group + 0xd4) = nxSceneTakeShapeId(scene);
	*reinterpret_cast<unsigned short*>(group + 0xd8) = 0xffffu;
	nxSceneAuxRegisterShape(scene, group);
	void** shapes = 0;
	void** helpers = 0;
	unsigned built = 0;
	for(unsigned i = 0; i < count; ++i)
		{
		unsigned char* child = static_cast<unsigned char*>(nxShapeFactory(
			reinterpret_cast<void*>(shapeDescriptions[i]), actor));
		if(!child) break;
		if(i == 0)
			{
			shapes = static_cast<void**>(nxGetSdkAllocator()->malloc(
				count * sizeof(void*), NX_MEMORY_PERSISTENT));
			helpers = static_cast<void**>(nxGetSdkAllocator()->malloc(
				count * sizeof(void*), NX_MEMORY_PERSISTENT));
			if(!shapes || !helpers)
				{
				nxSceneAuxUnregisterShape(scene, child);
				nxSceneRecycleShapeId(scene, *reinterpret_cast<unsigned*>(child + 0xd4));
				nxGetSdkAllocator()->free(*reinterpret_cast<void**>(child + 0x9c));
				nxGetSdkAllocator()->free(child);
				break;
				}
			*reinterpret_cast<void***>(group + 0xe0) = shapes;
			*reinterpret_cast<void***>(group + 0xe4) = shapes;
			*reinterpret_cast<void***>(group + 0xe8) = shapes + count;
			*reinterpret_cast<void***>(group + 0xf0) = helpers;
			*reinterpret_cast<void***>(group + 0xf4) = helpers;
			*reinterpret_cast<void***>(group + 0xf8) = helpers + count;
			}
		void*** shapesEnd = reinterpret_cast<void***>(group + 0xe4);
		void*** helpersEnd = reinterpret_cast<void***>(group + 0xf4);
		*(*shapesEnd)++ = child;
		*(*helpersEnd)++ = *reinterpret_cast<void**>(child + 0x9c);
		++built;
		}
	if(built != count)
		{
		while(built)
			{
			--built;
			nxSceneAuxUnregisterShape(scene, shapes[built]);
			nxSceneRecycleShapeId(scene,
				*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(shapes[built]) + 0xd4));
			nxGetSdkAllocator()->free(helpers[built]);
			nxGetSdkAllocator()->free(shapes[built]);
			}
		if(helpers) nxGetSdkAllocator()->free(helpers);
		if(shapes) nxGetSdkAllocator()->free(shapes);
		nxSceneAuxUnregisterShape(scene, group);
		nxSceneRecycleShapeId(scene, *reinterpret_cast<unsigned*>(group + 0xd4));
		nxGetSdkAllocator()->free(group);
		if(body) *reinterpret_cast<void**>(body + 0x10) = 0;
		return 0;
		}
	if(body)
		*reinterpret_cast<void**>(body + 0x10) = group;
	return group;
	}

// Runtime single-to-group promotion follows the allocation order of the
// actor's public createShape slot: child, public handle, group, then arrays.
// The factory temporarily links the new child as the body's root; promotion
// replaces that link with the group while retaining the original child.
void* nxActorAppendShape(void* actor, const NxShapeDesc* descriptor)
	{
	if(!actor || !descriptor || !descriptor->isValid()) return 0;
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	if(!body) return 0;
	unsigned char* original = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(!original || *reinterpret_cast<unsigned*>(original + 0xd0) == 5u)
		return 0;
	unsigned char* child = static_cast<unsigned char*>(nxShapeFactory(
		const_cast<NxShapeDesc*>(descriptor), actor));
	if(!child) return 0;
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	unsigned char* group = static_cast<unsigned char*>(
		nxGetSdkAllocator()->malloc(0x110, NX_MEMORY_PERSISTENT));
	void** shapes = group ? static_cast<void**>(
		nxGetSdkAllocator()->malloc(2 * sizeof(void*), NX_MEMORY_PERSISTENT)) : 0;
	void** helpers = shapes ? static_cast<void**>(
		nxGetSdkAllocator()->malloc(2 * sizeof(void*), NX_MEMORY_PERSISTENT)) : 0;
	if(!helpers)
		{
		if(shapes) nxGetSdkAllocator()->free(shapes);
		if(group) nxGetSdkAllocator()->free(group);
		nxSceneAuxUnregisterShape(scene, child);
		nxSceneRecycleShapeId(scene, *reinterpret_cast<unsigned*>(child + 0xd4));
		nxGetSdkAllocator()->free(*reinterpret_cast<void**>(child + 0x9c));
		nxShapeSetName(child, 0);
		nxGetSdkAllocator()->free(child);
		*reinterpret_cast<void**>(body + 0x10) = original;
		return 0;
		}
	memset(group, 0, 0x110);
	nxShapeFactoryInstallPrunable(group);
	*reinterpret_cast<void**>(group + 4) = body;
	*reinterpret_cast<unsigned*>(group + 0xd0) = 5;
	*reinterpret_cast<unsigned*>(group + 0xd4) = nxSceneTakeShapeId(scene);
	*reinterpret_cast<unsigned short*>(group + 0xd8) = 0xffffu;
	nxSceneAuxRegisterShape(scene, group);
	shapes[0] = original;
	shapes[1] = child;
	helpers[0] = *reinterpret_cast<void**>(original + 0x9c);
	helpers[1] = *reinterpret_cast<void**>(child + 0x9c);
	*reinterpret_cast<void***>(group + 0xe0) = shapes;
	*reinterpret_cast<void***>(group + 0xe4) = shapes + 2;
	*reinterpret_cast<void***>(group + 0xe8) = shapes + 2;
	*reinterpret_cast<void***>(group + 0xf0) = helpers;
	*reinterpret_cast<void***>(group + 0xf4) = helpers + 2;
	*reinterpret_cast<void***>(group + 0xf8) = helpers + 2;
	*reinterpret_cast<void**>(body + 0x10) = group;
	// The pruning pools, as the pinned DLL leaves them (scene-raycast block
	// Task 3): the original shape leaves its section 1 slot and returns in
	// section 0 with the new shape, and the group joins section 2, in the
	// actor's pruner (static 0, dynamic 2). Releasing the added shape later
	// leaves its pool entry in place, as the pinned DLL does.
	void* engine = &scene->at<unsigned char>(0x624);
	const unsigned type = *reinterpret_cast<void**>(body + 8) ? 2u : 0u;
	nxSceneEngineRemoveShape(engine, original);
	nxSceneEngineAddShape(engine, original, type, 0);
	nxSceneEngineAddShape(engine, child, type, 0);
	nxSceneEngineAddShape(engine, group, type, 2);
	return helpers[1];
	}

void nxActorRemoveShape(void* actor, void* handle)
	{
	if(!actor || !handle) return;
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	unsigned char* group = body
		? *reinterpret_cast<unsigned char**>(body + 0x10) : 0;
	if(!group || *reinterpret_cast<unsigned*>(group + 0xd0) != 5u)
		return;
	void** shapes = *reinterpret_cast<void***>(group + 0xe0);
	void** helpers = *reinterpret_cast<void***>(group + 0xf0);
	void** end = *reinterpret_cast<void***>(group + 0xf4);
	for(unsigned i = 0, count = static_cast<unsigned>(end - helpers); i < count; ++i)
		if(helpers[i] == handle)
			{
			for(unsigned j = i + 1; j < count; ++j)
				{
				shapes[j - 1] = shapes[j];
				helpers[j - 1] = helpers[j];
				}
			*reinterpret_cast<void***>(group + 0xe4) = shapes + count - 1;
			*reinterpret_cast<void***>(group + 0xf4) = helpers + count - 1;
			return;
			}
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

// phys_fn_000653 (0x00013760, 126 B, phase 7) is Scene::releaseJoint.
// The re-entry flag is createJoint's (.data 0x00123c10); a re-entrant call is
// reported with the message the pointer at .data 0x00122050 names (code 2,
// line 0x4e2). Otherwise: removeJoint, the joint's scalar deleting
// destructor (slot 5 with 1) when the pointer is non-null, --[+0x6c8] and
// the enumeration cursor reset to the list head, then the flag cleared.
void NxSceneInternal::releaseJoint(Joint* joint)
	{
	if(gCreateJointReentry)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_SCENE_CPP, 0x4e2, 0,
			"Reentry check: You may not call this API method from a callback!");
		return;
		}
	gCreateJointReentry = true;
	removeJoint(joint);
	if(joint)
		delete joint;
	--at<NxU32>(0x6c8);
	at<void*>(0x6bc) = at<void*>(0x59c);
	gCreateJointReentry = false;
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
