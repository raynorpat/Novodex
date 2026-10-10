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
#include "ContactPairManager.h"

#include "Containers.h"
#include "NxPhysicsBackend.h"
#if !NX_PHYSICS_USE_X87
#include "NxSceneContactMembers.h"
#include "NxSceneVisitedBuffers.h"
#include "NxSceneActorIdPool.h"
#endif
#include "NxSceneDesc.h"
#include "NxSceneStats.h"
#include "NxBounds3.h"
#include "NxIntersectionRayTriangle.h"
#include "NxBox.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxShapeDesc.h"
#include "NxBoxShape.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxTriangleMesh.h"
#include "NxTriangleMeshShape.h"
#include "ObjectModel.h"
#include "TriangleMesh.h"
#include "NxActor.h"
#include "NpActor.h"
#include "NpActorDynamicMath.h"
#include "BodyStep.h"
#include "core/JointSupport.h"
#include "NxFPU.h"
#include "BodyCreation.h"
#include "NpScene.h"
#include "NxJointDesc.h"
#include "NxJoint.h"
#include "NxUserNotify.h"
#include "NxUserContactReport.h"
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
#include "core/SpringAndDamperEffector.h"
#include "core/ActorMass.h"
#include "NxSpringAndDamperEffectorDesc.h"
#include "Observable.h"
#include "PhysicsSDK.h"

// The oracle's __FILE__ strings for the reports this file makes on their
// behalf (.rdata 0x10106420 and 0x10104278).
#define NX_SCENE_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\Scene.cpp"
#define NX_ACTOR_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\Actor.cpp"
#include "NxMat33.h"
#include "NxQuat.h"
#include "FoundationSDK.h"
#include "NxAllocateable.h"
#include "NxUtilities.h"
#include "SceneVisualize.h"
#include "ContactPairManager.h"
#include "NxScene.h"
#include "fluids/NxFluidDesc.h"
#include "opcode/IcePruner.h"
#include "NxDebugRenderable.h"
#include "NarrowPhase.h"
#include "ContactGeneration.h"
void nxContainerAddThunk(void* innerThis);

#include <string.h>
#include <new>

class NxGroupsMask;

void nxAggregateAABB1030(void* self, float* out);

// phys_data_000980 (0x1012718c): the largest 000611 island-body count seen by
// the joint-record solver. The original global remains zero until a live
// island contributes bodies.
static NxU32 nxSceneMaximumStepBodies = 0;

static void nxSceneEnsureStepBodies(NxSceneInternal* scene, NxU32 count)
	{
	scene->at<NxU32>(0x5b0) = count;
	if(scene->at<NxU32>(0x5b4) >= count)
		return;
	NxU8* oldRecords = scene->at<NxU8*>(0x5ac);
	if(oldRecords)
		nxFoundationSDKAllocator->free(oldRecords - sizeof(NxU32));
	NxU8* allocation = static_cast<NxU8*>(nxFoundationSDKAllocator->malloc(
		count * sizeof(JointSupportBody) + sizeof(NxU32), NX_MEMORY_PERSISTENT));
	scene->at<NxU8*>(0x5ac) = allocation ? allocation + sizeof(NxU32) : 0;
	scene->at<NxU32>(0x5b4) = count;
	if(allocation)
		{
		*reinterpret_cast<NxU32*>(allocation) = count;
		memset(allocation + sizeof(NxU32), 0, count * sizeof(JointSupportBody));
		}
	}

// phys_fn_000517 (0x00010370, 24 B). The oracle body has a separate loop
// extent at 0x10390; removing a record compacts the last live pair into this
// slot, so the cursor advances only when the current pair is retained.
void NxSceneRemoveOwnerPairRecords(void* scene, const void* shape)
	{
	NxU8* bytes = (NxU8*) scene;
	NxU8* pairMap = bytes + 0x2c;
	NxU8* shapeBytes = (NxU8*) shape;
	const NxU16 owner = (NxU16) *(const NxU32*) (shapeBytes + 0xd4);
	NxU8* record = (NxU8*) *(void**) (pairMap + 0x14);
	NxU32 remaining = *(NxU32*) (pairMap + 0x10);
	// phys_fn_000519 (0x00010390, 78 B), the loop continuation in the oracle body.
	while(remaining--)
		{
		const NxU16 owner0 = *(NxU16*) record;
		const NxU16 owner1 = *(NxU16*) (record + 2);
		if(owner0 == owner || owner1 == owner)
			{
			void* payload = *(void**) (record + 4);
			if(payload && ((NxU32) payload & 1u) == 0)
				nxFoundationSDKAllocator->free(payload);
			NxRemoveCollisionPairRecord(pairMap, owner0, owner1);
			}
		else
			record += 8;
		}
	}

// phys_fn_000889 (0x0001e940, 99 B)
// Continuation of 000887: release the pruner's object vector and empty its
// embedded sink Container. The oracle's register epilogue belongs to 000887.
extern "C" __declspec(noinline) void __fastcall NxScenePrunerRecordCleanupTail(void* record)
	{
	NxU8* bytes = static_cast<NxU8*>(record);
	void** begin = *reinterpret_cast<void***>(bytes + 0xc4);
	void** end = *reinterpret_cast<void***>(bytes + 0xc8);
	const NxI32 byteDistance = static_cast<NxI32>(reinterpret_cast<NxU32>(end) - reinterpret_cast<NxU32>(begin));
	const NxU32 count = static_cast<NxU32>(byteDistance >> 2);
	for(NxU32 i = 0; i < count; ++i)
		if(begin[i]) nxFoundationSDKAllocator->free(begin[i]);
	*reinterpret_cast<NxU32*>(bytes + 0x48) = 0;
	if(begin) nxFoundationSDKAllocator->free(begin);
	*reinterpret_cast<void**>(bytes + 0xc4) = 0;
	*reinterpret_cast<void**>(bytes + 0xc8) = 0;
	*reinterpret_cast<NxU32*>(bytes + 0xcc) = 0;
	nxContainerAddThunk(bytes + 0x10);
	}

// phys_fn_000887 (0x0001e910, 41 B)
// 000887's entry and 000889 continuation together clean the embedded pruner sink.
extern "C" __declspec(noinline) void __fastcall NxScenePrunerRecordCleanup(void* record)
	{
	NxU8* bytes = static_cast<NxU8*>(record);
	NxContactSinkResetState(reinterpret_cast<NxU32*>(bytes + 0x10));
	NxScenePrunerRecordCleanupTail(record);
	}

extern "C" void __fastcall NxScenePrunerNodeRemove(void* node);

// phys_fn_000915 (0x00020020, 33 B)
// Release a pruner node: unlink and clean it before returning its storage to
// the Foundation SDK allocator used by the pruner.
extern "C" __declspec(noinline) void __stdcall NxScenePrunerNodeDestroy(void* node)
	{
	if(!node) return;
	NxScenePrunerNodeRemove(node);
	nxFoundationSDKAllocator->free(node);
	}

// phys_fn_001955 (0x0004bde0, 153 B)
// Remove every pruning pair whose embedded node belongs to this shape owner,
// releasing its node and pair-map key, then compact the owner's live shape IDs.
extern "C" __declspec(noinline) void __fastcall NxScenePrunerShapeRemove(void* group, void* owner)
	{
	NxU8* bytes = static_cast<NxU8*>(group);
	const NxU32 ownerWord = *reinterpret_cast<const NxU32*>(static_cast<const NxU8*>(owner) + 0x10);
	NxU16* record = *reinterpret_cast<NxU16**>(bytes + 0x48);
	for(NxU32 remaining = *reinterpret_cast<NxU32*>(bytes + 0x44); remaining; --remaining)
		{
		void* node = *reinterpret_cast<void**>(record + 2);
		const NxU8* nodeBytes = static_cast<const NxU8*>(node);
		const NxU8* firstOwner = *reinterpret_cast<const NxU8* const*>(nodeBytes + 0x14);
		const NxU8* secondOwner = *reinterpret_cast<const NxU8* const*>(nodeBytes + 0x18);
		if(*reinterpret_cast<const NxU32*>(firstOwner + 0x10) == ownerWord ||
			*reinterpret_cast<const NxU32*>(secondOwner + 0x10) == ownerWord)
			{
			NxScenePrunerNodeDestroy(node);
			NxRemoveCollisionPairRecord(bytes + 0x34, record[0], record[1]);
			}
		else
			record += 4;
		}
	NxU32 index = 0;
	NxU32 count = *reinterpret_cast<NxU32*>(bytes + 0x7c);
	NxU32* owners = *reinterpret_cast<NxU32**>(bytes + 0x80);
	while(index < count && owners[index] != ownerWord)
		++index;
	if(index < count)
		{
		--count;
		*reinterpret_cast<NxU32*>(bytes + 0x7c) = count;
		owners[index] = owners[count];
		}
	}

// phys_fn_000903 (0x0001fb30, 120 B)
// Unlink a pruner node from its owning intrusive list before cleaning its payload.
extern "C" __declspec(noinline) void __fastcall NxScenePrunerNodeRemove(void* node)
	{
	NxU8* bytes = static_cast<NxU8*>(node);
	NxU8* previous = *reinterpret_cast<NxU8**>(bytes + 0x0c);
	NxU8* next = *reinterpret_cast<NxU8**>(bytes + 8);
	NxU8* owner = *reinterpret_cast<NxU8**>(bytes + 0x10);
	if(!previous)
		{
		if(*reinterpret_cast<NxU8**>(owner + 4) == bytes)
			*reinterpret_cast<NxU8**>(owner + 4) = 0;
		*reinterpret_cast<NxU8**>(owner) = next;
		if(next)
			*reinterpret_cast<NxU8**>(next + 0x0c) = 0;
		}
	else if(!next)
		{
		if(*reinterpret_cast<NxU8**>(owner + 4) == bytes)
			*reinterpret_cast<NxU8**>(owner + 4) = previous;
		*reinterpret_cast<NxU8**>(previous + 8) = 0;
		}
	else
		{
		*reinterpret_cast<NxU8**>(previous + 8) = next;
		*reinterpret_cast<NxU8**>(next + 0x0c) = previous;
		}
	*reinterpret_cast<NxU8**>(bytes + 0x0c) = 0;
	*reinterpret_cast<NxU8**>(bytes + 8) = 0;
	NxScenePrunerRecordCleanup(bytes + 0x14);
	}
// Public shape final and descriptor loader share the oracle's global name map.
void nxShapeSetName(void* shape, const char* name);
void nxShapeFactoryInitializePose(void* shape, const void* localPose);
void nxShapeFactoryRefreshPose(void* shape);
void nxShapeFactoryInstallVtable(void* shape, unsigned type);
void* nxShapeFactoryConstructMeshHandle(void* memory, void* shape);
// ShapeBase::nxApplyOwnerUpdate (phys_fn_001315, ObjectModel.cpp) on a shape.
void nxShapeApplyOwnerUpdate(void* shape, unsigned flags);
// ObjectModel.cpp's rows the Task 4 chain calls: phys_fn_000012 (the id pool),
// phys_fn_000028 (its push) and phys_fn_002344 (the Scene+0x5d4 pair removal).
unsigned nxIdAllocNext(void* container);
void nxU32VectorPushBack(void* vecHeader, NxU32 value);
void nxSceneRemovePairs(void* container, void* shape);
void nxSceneProcessTriggerPairs(NxSceneInternal* scene);
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
void nxSceneApplyDescriptorFlags(NxSceneInternal* scene, NxU32 broadPhase,
	const NxBounds3* bounds);
void nxReport(int kind, const char* file, int line, int code, const char* message);
// phys_fn_000626 (0x00011730, phase 7) and phys_fn_000501 (0x0000ff10, phase 7):
// the scene descriptor's ground and bounds-plane actor builders.
void nxSceneBuildGroundPlane(void* scene);
void nxSceneBuildBoundsPlanes(void* scene, const NxBounds3* bounds);
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
void nxSceneEngineSetExternalBuffer(void* engine, unsigned capacity, void* entries);
void nxShapeFactoryInstallPrunable(void* shape);
bool nxShapeFactoryLoadBox(void* shape, const void* descriptor);
// 002421 and 002411, keyed on the manager: BodyCreation.cpp's 000801 and
// 000799 pass the record's +0x120.
void nxSceneAuxRegisterRecord(void* aux, void* record);
void nxSceneAuxUnregisterRecord(void* aux, void* record);
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
#if NX_PHYSICS_USE_X87
	nxDword(p, 0x1c) = 0;
	nxDword(p, 0x20) = 0;
	nxDword(p, 0x24) = 0;
	nxDword(p, 0x28) = 0;
#else
	nxScenePrunerCollectionConstruct(nxAt(p, 0x1c));
#endif
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
void* nxSceneCreateActorBody(void* memory, void* scene);

NxActor* nxSceneActorConstruct(void* memory, void* scene);
// phys_fn_00002010 (0x00002010, phase 2): applies the descriptor to the actor and
// returns the actor's vtable word. Modelled as "applied, non-null".
void* nxSceneActorInitialise(NxActor* actor, const void* desc);
// Actor.cpp's actor destructor body, row 000030 (defined below).
void nxActorDestroy(unsigned char* body);
// phys_fn_000100a0 (0x000100a0, phase 7): refreshes a cached count from an array.
void nxSceneUpdateActorCount(void* scene, unsigned count);
// phys_fn_00089d50 (0x00089d50, phase 6): the scene's notification hook.
void nxSceneNotifyActorCreated(void* hook, NxActor* actor);
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
#if !NX_PHYSICS_USE_X87
NxSceneActorIdPool& nxSceneActorIds(NxSceneInternal& scene)
	{
	static_assert(0x6d0 % alignof(NxSceneActorIdPool) == 0,
		"Scene actor-ID member storage alignment");
	return *reinterpret_cast<NxSceneActorIdPool*>(scene.bytes() + 0x6d0);
	}
#endif

// phys_fn_000647 (0x00012c10). Every store below is the listing's, at the byte
// offset the listing names, in the listing's order; the listing address of the
// first store of each run is given so the two can be read side by side.
NxSceneInternal::NxSceneInternal()
	{
	unsigned* p = reinterpret_cast<unsigned*>(mBytes);
	const unsigned base = reinterpret_cast<unsigned>(this);

	// 0x12c18: the vtable the oracle installs.
#if NX_PHYSICS_USE_X87
	nxDword(p, 0x000) = reinterpret_cast<unsigned>(vtable());

	// 0x12c21: +0x04..+0x28 zeroed.
	for(unsigned offset = 0x04; offset <= 0x28; offset += 4)
		nxDword(p, offset) = 0;
#else
	nxSceneScratchConstruct(nxAt(p, 0x000), reinterpret_cast<NxU32>(vtable()));
	for(unsigned offset = 0x18; offset <= 0x28; offset += 4)
		nxDword(p, offset) = 0;
#endif

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
#if NX_PHYSICS_USE_X87
	nxSceneMemberB5720(nxAt(p, 0x450));						// phys_fn_004899
	new (nxAt(p, 0x4e0)) SdkContainer();					// phys_fn_004836
	new (nxAt(p, 0x4f0)) SdkContainer();
#else
	nxSceneContactMembersConstruct(nxAt(p, 0x450));
#endif
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

#if NX_PHYSICS_USE_X87
	for(unsigned offset = 0x6ac; offset <= 0x6dc; offset += 4)	// 0x12ee5
		nxDword(p, offset) = 0;
#else
	for(unsigned offset = 0x6ac; offset <= 0x6cc; offset += 4)
		nxDword(p, offset) = 0;
	nxSceneActorIdPoolConstruct(nxAt(p, 0x6d0));
#endif
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

static void* __fastcall nxFluidManagerDeletingDestructor(void* manager, void*, unsigned char deleteObject);

static void* gNxFluidManagerVtable[] = {
	reinterpret_cast<void*>(nxFluidManagerDeletingDestructor)
	};

// phys_fn_003645 (0x00089f50) lays out FluidManager's arrays, backend flags,
// Scene link, and virtual dispatch pointer. The pinned build has no fluid
// extension, so its extension flags are zero; the constructor leaves the
// conditional backend storage untouched while that feature is unavailable.
static void* nxSceneCreateDisabledFluidManager(NxSceneInternal* scene)
	{
	void* memory = nxFoundationSDKAllocator->malloc(0x34, NX_MEMORY_PERSISTENT);
	if(!memory)
		return 0;
	*reinterpret_cast<void***>(memory) = gNxFluidManagerVtable;
	unsigned char* bytes = static_cast<unsigned char*>(memory);
	*reinterpret_cast<unsigned*>(bytes + 4) = 0;
	*reinterpret_cast<unsigned*>(bytes + 8) = 0;
	*reinterpret_cast<unsigned*>(bytes + 0xc) = 0;
	*reinterpret_cast<unsigned*>(bytes + 0x14) = 0;
	*reinterpret_cast<unsigned*>(bytes + 0x18) = 0;
	*reinterpret_cast<unsigned*>(bytes + 0x1c) = 0;
	*reinterpret_cast<void**>(bytes + 0x24) = scene;
	bytes[0x2a] = 0;
	bytes[0x2b] = 0;
	bytes[0x29] = 1;
	return memory;
	}

// phys_fn_003641 (0x00089e00) switches to the base FluidManager vtable, runs
// each primary entry's scalar deleting destructor, then frees and clears both
// SDK array headers. The pinned backend is unavailable, so its conditional
// callback and extension-library cleanup have no live state to observe.
static void nxFluidManagerDestroyArrays(void* manager)
	{
	if(manager)
		{
		unsigned char* bytes = static_cast<unsigned char*>(manager);
		*reinterpret_cast<void***>(manager) = gNxFluidManagerVtable;
		unsigned first = *reinterpret_cast<unsigned*>(bytes + 4);
		unsigned last = *reinterpret_cast<unsigned*>(bytes + 8);
		unsigned count = last >= first ? (last - first) >> 2 : 0;
		void** fluids = reinterpret_cast<void**>(first);
		for(unsigned i = 0; i != count; ++i)
			{
			void* fluid = fluids[i];
			if(fluid)
				{
				void** vtable = *reinterpret_cast<void***>(fluid);
				typedef void* (__thiscall *DeletingDestructor)(void*, unsigned char);
				DeletingDestructor destroy = reinterpret_cast<DeletingDestructor>(vtable[0]);
				destroy(fluid, 1);
				}
			}
		// The enabled extension path calls a global function pointer with the
		// value stored at +0x30; this disabled build never sets the flag at +0x2b.
		for(unsigned offset = 0x14; ; offset = 4)
			{
			void* entries = *reinterpret_cast<void**>(bytes + offset);
			if(entries)
				nxFoundationSDKAllocator->free(entries);
			*reinterpret_cast<unsigned*>(bytes + offset) = 0;
			*reinterpret_cast<unsigned*>(bytes + offset + 4) = 0;
			*reinterpret_cast<unsigned*>(bytes + offset + 8) = 0;
			if(offset == 4)
				break;
			}
		}
	}

// phys_fn_003647 (0x00089fd0): scalar deleting destructor, vtable slot 0.
static void* __fastcall nxFluidManagerDeletingDestructor(void* manager, void*, unsigned char deleteObject)
	{
	nxFluidManagerDestroyArrays(manager);
	if((deleteObject & 1) != 0)
		nxFoundationSDKAllocator->free(manager);
	return manager;
	}

static void nxSceneReleaseDisabledFluidManager(void* manager)
	{
	if(manager)
		{
		void** vtable = *reinterpret_cast<void***>(manager);
		typedef void* (__thiscall *DeletingDestructor)(void*, unsigned char);
		DeletingDestructor destroy = reinterpret_cast<DeletingDestructor>(vtable[0]);
		destroy(manager, 1);
		}
	}

static bool gNxSceneFluidReleaseInProgress = false;

// phys_fn_003643 (0x00089e90): the disabled manager warns, then searches its
// fluid pointer array. When the pinned backend is disabled, this array is
// empty; the matching-entry and deleting-dispatch path is also exercised with
// a test-seeded fake object.
static void nxFluidManagerReleaseDisabledFluid(void* manager, void* fluidInternal)
	{
	unsigned char* bytes = static_cast<unsigned char*>(manager);
	if(bytes[0x2b] == 0)
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\FluidManager.cpp",
			0xb7, 0, "NxScene::releaseFluid(): Feature not available!");
	unsigned first = *reinterpret_cast<unsigned*>(bytes + 4);
	unsigned last = *reinterpret_cast<unsigned*>(bytes + 8);
	unsigned count = last >= first ? (last - first) >> 2 : 0;
	for(unsigned i = 0; i != count; ++i)
		{
		unsigned* fluids = reinterpret_cast<unsigned*>(first);
		if(reinterpret_cast<void*>(fluids[i]) != fluidInternal)
			continue;
		fluids[i] = fluids[count - 1];
		*reinterpret_cast<unsigned*>(bytes + 8) = last - 4;
		unsigned secondaryFirst = *reinterpret_cast<unsigned*>(bytes + 0x14);
		unsigned secondaryLast = *reinterpret_cast<unsigned*>(bytes + 0x18);
		unsigned secondaryCount = secondaryLast >= secondaryFirst ?
			(secondaryLast - secondaryFirst) >> 2 : 0;
		if(i < secondaryCount)
			{
			unsigned* secondary = reinterpret_cast<unsigned*>(secondaryFirst);
			secondary[i] = secondary[secondaryCount - 1];
			*reinterpret_cast<unsigned*>(bytes + 0x18) = secondaryLast - 4;
			}
		if(fluidInternal)
			{
			void** fluidVtable = *reinterpret_cast<void***>(fluidInternal);
			typedef void* (__thiscall *DeletingDestructor)(void*, unsigned char);
			DeletingDestructor destroy = reinterpret_cast<DeletingDestructor>(fluidVtable[0]);
			destroy(fluidInternal, 1);
			}
		break;
		}
	}

// phys_fn_003637 (0x00089d50): the oracle walks the manager's primary fluid
// array and calls a three-byte nullsub with `actor` for every entry. The call
// has no side effects, and NxScene::createActor ignores this helper's result,
// so warning-only behavior is equivalent for empty and populated valid arrays.
static void nxFluidManagerNotifyActorCreatedDisabled(void* manager, NxActor* actor)
	{
	(void)actor;
	unsigned char* bytes = static_cast<unsigned char*>(manager);
	if(bytes[0x2b] == 0)
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\FluidManager.cpp",
			0x107, 0, "NxScene::fluidsNotifyCreateActor(): Feature not available!");
	}

static void nxFluidManagerNotifyActorReleasedDisabled(void* manager, void* body)
	{
	(void)body;
	unsigned char* bytes = static_cast<unsigned char*>(manager);
	if(bytes[0x2b] == 0)
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\FluidManager.cpp",
			0xfa, 0, "NxScene::fluidsNotifyReleaseActor(): Feature not available!");
	}

// phys_fn_003630 (0x00089bd0) is called once after each completed scene
// substep. The available=false branch reports the pinned backend warning;
// extension-backed collision updates and live-fluid iteration remain open.
static void nxFluidManagerStepDisabled(void* manager, NxReal timestep)
	{
	(void)timestep;
	unsigned char* bytes = static_cast<unsigned char*>(manager);
	if(bytes[0x2b] == 0)
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\FluidManager.cpp",
			0xcf, 0, "NxScene::stepFluids(): Feature not available!");
	}

// phys_fn_003632 (0x00089c80) runs once after the scene's substep loop.
// Its available=false warning is independent of whether the fluid array is
// empty; enabled extension dispatch is still not reconstructed here.
static void nxFluidManagerGenerateSurfaceMeshesDisabled(void* manager)
	{
	unsigned char* bytes = static_cast<unsigned char*>(manager);
	if(bytes[0x2b] == 0)
		NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\FluidManager.cpp",
			0xea, 0, "NxScene::generateSurfaceMeshes(): Feature not available!");
	}

// phys_fn_000622 (0x00011620): guard reentry, release the requested fluid,
// then destroy and clear an empty manager.
void NxSceneInternal::releaseFluid(void* fluidInternal)
	{
	if(gNxSceneFluidReleaseInProgress)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\Scene.cpp", 0xc52, 0,
			"Reentry check: You may not call this API method from a callback!");
		return;
		}
	gNxSceneFluidReleaseInProgress = true;
	void*& manager = at<void*>(0x61c);
	if(manager)
		{
		nxFluidManagerReleaseDisabledFluid(manager, fluidInternal);
		unsigned first = *reinterpret_cast<unsigned*>(static_cast<unsigned char*>(manager) + 4);
		unsigned last = *reinterpret_cast<unsigned*>(static_cast<unsigned char*>(manager) + 8);
		if(first == last)
			{
			nxSceneReleaseDisabledFluidManager(manager);
			manager = 0;
			}
		}
	gNxSceneFluidReleaseInProgress = false;
	}

// phys_fn_000645 (0x00012b80): create the manager lazily, then forward the
// descriptor. FluidManager's constructor records +0x2b=0 when the backend is
// unavailable; createFluid reports the shipped warning and returns null.
NxFluid* NxSceneInternal::createFluid(const NxFluidDesc& desc)
	{
	void*& manager = at<void*>(0x61c);
	if(!manager)
		manager = nxSceneCreateDisabledFluidManager(this);
	if(!manager)
		return 0;
	// The enabled FluidManager validates the descriptor before allocating a
	// fluid or entering the extension-backed creation path.
	if(static_cast<unsigned char*>(manager)[0x2b] != 0 && !desc.isValid())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\FluidManager.cpp",
			0x8c, 0, "Supplied NxFluidDesc is not valid. createFluid returns NULL.");
		return 0;
		}
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\FluidManager.cpp",
		0x8a, 0, "NxScene::createFluid(): Feature not available!");
	return 0;
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
		nxFoundationSDKAllocator->malloc(capacity * sizeof(unsigned), NX_MEMORY_PERSISTENT));
	if(!grown)
		return;
	for(unsigned i = 0; i < count; ++i)
		grown[i] = first[i];
	if(first)
		nxFoundationSDKAllocator->free(first);
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
#if NX_PHYSICS_USE_X87
	unsigned char* header = scene->bytes() + 0x6d4;
	nxSceneArrayReserve(header, 1);
	unsigned* last = scene->at<unsigned*>(0x6d8);
	if(!last) return;
	*last = id;
	scene->at<unsigned*>(0x6d8) = last + 1;
#else
	nxSceneActorIds(*scene).returnId(id);
#endif
	}

// Dynamic-record IDs use the same LIFO vector layout at Scene+0x6fc, with the
// next fresh ID at +0x6f8 (000028's container). The record constructor 000797
// takes one (inlined there, BodyCreation.cpp); the record destructor 000776
// returns its +0x11c ID through 000028 (nxU32VectorPushBack) before 000030
// frees the record.

// The first dynamic record initializes five 256-slot arrays in the Scene's
// 0xa8-byte auxiliary manager. Three are prepared through a temporary 0x800
// staging buffer, then copied to retained 0x400-byte arrays. This follows
// the oracle's allocation/free sequence and measured array headers. The
// per-record slots at +0x40/+0x50/+0x60/+0x80 are updated below.
static bool nxSceneAuxPrepareStagedArray(unsigned char* aux, unsigned offset,
	unsigned fill, unsigned firstValue)
	{
	unsigned* staging = static_cast<unsigned*>(
		nxFoundationSDKAllocator->malloc(0x800, NX_MEMORY_PERSISTENT));
	if(!staging) return false;
	for(unsigned i = 0; i < 512; ++i) staging[i] = fill;
	staging[0] = firstValue;
	unsigned* retained = static_cast<unsigned*>(
		nxFoundationSDKAllocator->malloc(0x400, NX_MEMORY_PERSISTENT));
	if(!retained)
		{
		nxFoundationSDKAllocator->free(staging);
		return false;
		}
	memcpy(retained, staging, 0x400);
	nxFoundationSDKAllocator->free(staging);
	*reinterpret_cast<unsigned**>(aux + offset) = retained;
	*reinterpret_cast<unsigned**>(aux + offset + 4) = retained + 256;
	*reinterpret_cast<unsigned**>(aux + offset + 8) = retained + 256;
	return true;
	}

static bool nxSceneAuxEnsureIndexedArray(unsigned char* aux, unsigned offset,
	unsigned slot, unsigned fill)
	{
	unsigned*& first = *reinterpret_cast<unsigned**>(aux + offset);
	unsigned*& last = *reinterpret_cast<unsigned**>(aux + offset + 4);
	unsigned*& capacity = *reinterpret_cast<unsigned**>(aux + offset + 8);
	const unsigned size = first ? static_cast<unsigned>(last - first) : 0;
	if(slot < size) return true;
	const unsigned target = (slot + 256u) & ~255u;
	if(target == 0) return false;
	if(first && capacity && static_cast<unsigned>(capacity - first) >= target)
		{
		for(unsigned i = size; i < target; ++i) first[i] = fill;
		last = first + target;
		return true;
		}
	unsigned* grown = static_cast<unsigned*>(nxFoundationSDKAllocator->malloc(
		target * sizeof(unsigned), NX_MEMORY_PERSISTENT));
	if(!grown) return false;
	if(size) memcpy(grown, first, size * sizeof(unsigned));
	for(unsigned i = size; i < target; ++i) grown[i] = fill;
	if(first) nxFoundationSDKAllocator->free(first);
	first = grown;
	last = grown + target;
	capacity = grown + target;
	return true;
	}

static bool nxSceneAuxEnsureRecordIndex(unsigned char* aux, unsigned offset,
	unsigned slot)
	{
	unsigned* first = *reinterpret_cast<unsigned**>(aux + offset);
	unsigned* capacity = *reinterpret_cast<unsigned**>(aux + offset + 8);
	const unsigned target = (slot + 256u) & ~255u;
	if(target == 0) return false;
	if(first && capacity && static_cast<unsigned>(capacity - first) >= target)
		return true;
	unsigned* grown = static_cast<unsigned*>(nxFoundationSDKAllocator->malloc(
		target * sizeof(unsigned), NX_MEMORY_PERSISTENT));
	if(!grown) return false;
	unsigned* last = *reinterpret_cast<unsigned**>(aux + offset + 4);
	const unsigned size = first ? static_cast<unsigned>(last - first) : 0;
	if(size) memcpy(grown, first, size * sizeof(unsigned));
	if(first) nxFoundationSDKAllocator->free(first);
	*reinterpret_cast<unsigned**>(aux + offset) = grown;
	*reinterpret_cast<unsigned**>(aux + offset + 4) = grown + size;
	*reinterpret_cast<unsigned**>(aux + offset + 8) = grown + target;
	return true;
	}

// phys_fn_002417 (0x0005bc90) is the shared indexed-manager insertion behavior
// modeled by the body and shape registration paths in this file.
// phys_fn_002421 (0x0005c160): DynamicBodyBase::construct inserts the record
// by its +0x104 ID into the Scene auxiliary manager and grows its sparse slot.
void nxSceneAuxRegisterRecord(void* auxPointer, void* recordPointer)
	{
	unsigned char* aux = static_cast<unsigned char*>(auxPointer);
	unsigned char* record = static_cast<unsigned char*>(recordPointer);
	if(!aux || !record) return;
	const unsigned slot = *reinterpret_cast<unsigned*>(record + 0x11c);
	if(!*reinterpret_cast<void**>(aux + 0x80))
		{
		// IDs start at zero in a new Scene. The first record establishes the
		// five parallel arrays and active-list entry in one allocation sequence.
		if(slot != 0) return;
		if(!nxSceneAuxPrepareStagedArray(aux, 0x80, 0,
			reinterpret_cast<unsigned>(record + 0x18))) return;
		if(!nxSceneAuxPrepareStagedArray(aux, 0x40, 0, 0xffffffffu)) return;
		if(!nxSceneAuxPrepareStagedArray(aux, 0x60, 0xd00beed0u, 0)) return;
		unsigned* active = static_cast<unsigned*>(
			nxFoundationSDKAllocator->malloc(0x400, NX_MEMORY_PERSISTENT));
		if(!active) return;
		memset(active, 0, 0x400);
		*reinterpret_cast<unsigned**>(aux + 0x50) = active;
		*reinterpret_cast<unsigned**>(aux + 0x54) = active + 1;
		*reinterpret_cast<unsigned**>(aux + 0x58) = active + 256;
		unsigned* vacant = static_cast<unsigned*>(
			nxFoundationSDKAllocator->malloc(0x400, NX_MEMORY_PERSISTENT));
		if(!vacant) return;
		memset(vacant, 0, 0x400);
		*reinterpret_cast<unsigned**>(aux + 0x70) = vacant;
		*reinterpret_cast<unsigned**>(aux + 0x74) = vacant;
		*reinterpret_cast<unsigned**>(aux + 0x78) = vacant + 256;
		return;
		}
	if(!nxSceneAuxEnsureIndexedArray(aux, 0x80, slot, 0) ||
		!nxSceneAuxEnsureIndexedArray(aux, 0x40, slot, 0) ||
		!nxSceneAuxEnsureIndexedArray(aux, 0x60, slot, 0xd00beed0u) ||
		!nxSceneAuxEnsureRecordIndex(aux, 0x70, slot)) return;
	unsigned* active = *reinterpret_cast<unsigned**>(aux + 0x50);
	unsigned* activeEnd = *reinterpret_cast<unsigned**>(aux + 0x54);
	unsigned* activeCapacity = *reinterpret_cast<unsigned**>(aux + 0x58);
	if(!active || !activeEnd || !activeCapacity) return;
	if(activeEnd == activeCapacity)
		{
		const unsigned count = static_cast<unsigned>(activeEnd - active);
		const unsigned next = count * 2 + 2;
		unsigned* grown = static_cast<unsigned*>(nxFoundationSDKAllocator->malloc(
			next * sizeof(unsigned), NX_MEMORY_PERSISTENT));
		if(!grown) return;
		if(count) memcpy(grown, active, count * sizeof(unsigned));
		nxFoundationSDKAllocator->free(active);
		active = grown;
		activeEnd = grown + count;
		activeCapacity = grown + next;
		*reinterpret_cast<unsigned**>(aux + 0x50) = active;
		*reinterpret_cast<unsigned**>(aux + 0x54) = activeEnd;
		*reinterpret_cast<unsigned**>(aux + 0x58) = activeCapacity;
		}
	unsigned* occupied = *reinterpret_cast<unsigned**>(aux + 0x40);
	if(occupied[slot]) return;
	const unsigned activeIndex = static_cast<unsigned>(activeEnd - active);
	occupied[slot] = 0xffffffffu;
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
			nxFoundationSDKAllocator->malloc(0x400, NX_MEMORY_PERSISTENT));
		if(!active) return;
		memset(active, 0, 0x400);
		*reinterpret_cast<unsigned**>(aux + 0x10) = active;
		*reinterpret_cast<unsigned**>(aux + 0x14) = active + 1;
		*reinterpret_cast<unsigned**>(aux + 0x18) = active + 256;
		unsigned* vacant = static_cast<unsigned*>(
			nxFoundationSDKAllocator->malloc(0x400, NX_MEMORY_PERSISTENT));
		if(!vacant) return;
		memset(vacant, 0, 0x400);
		*reinterpret_cast<unsigned**>(aux + 0x30) = vacant;
		*reinterpret_cast<unsigned**>(aux + 0x34) = vacant;
		*reinterpret_cast<unsigned**>(aux + 0x38) = vacant + 256;
		return;
		}
	unsigned* active = *reinterpret_cast<unsigned**>(aux + 0x10);
	unsigned* activeEnd = *reinterpret_cast<unsigned**>(aux + 0x14);
	if(!active || !activeEnd || !nxSceneAuxEnsureShapeSlot(aux,
		*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(shapePointer) + 0xd4))) return;
	const unsigned slot = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(shapePointer) + 0xd4);
	unsigned* activeCapacity = *reinterpret_cast<unsigned**>(aux + 0x18);
	if(!activeCapacity) return;
	if(activeEnd == activeCapacity)
		{
		const unsigned count = static_cast<unsigned>(activeEnd - active);
		const unsigned next = count * 2 + 2;
		unsigned* grown = static_cast<unsigned*>(nxFoundationSDKAllocator->malloc(
			next * sizeof(unsigned), NX_MEMORY_PERSISTENT));
		if(!grown) return;
		if(count) memcpy(grown, active, count * sizeof(unsigned));
		nxFoundationSDKAllocator->free(active);
		active = grown;
		activeEnd = grown + count;
		activeCapacity = grown + next;
		*reinterpret_cast<unsigned**>(aux + 0x10) = active;
		*reinterpret_cast<unsigned**>(aux + 0x14) = activeEnd;
		*reinterpret_cast<unsigned**>(aux + 0x18) = activeCapacity;
		}
	unsigned* flags = *reinterpret_cast<unsigned**>(aux);
	if(flags[slot]) return;
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
	if(!flags || id >= static_cast<unsigned>(
		*reinterpret_cast<unsigned**>(aux + 8) - flags)) return;
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
			unsigned* grown = static_cast<unsigned*>(nxFoundationSDKAllocator->malloc(
				next * sizeof(unsigned), NX_MEMORY_PERSISTENT));
			if(!grown) return;
			memcpy(grown, active, count * sizeof(unsigned));
			nxFoundationSDKAllocator->free(active);
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

void nxSceneAuxUnregisterRecord(void* auxPointer, void* recordPointer)
	{
	unsigned char* aux = static_cast<unsigned char*>(auxPointer);
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
// dynamic. Its process-wide header owns four initial buffers. Unlike the Scene
// rows, the pool (0x000b4cc0) and its buffers (0x000ef270) allocate through
// phys_fn_004803, so this helper keeps nxGetSdkAllocator.
#if NX_PHYSICS_USE_X87
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

#endif
// The pending-shape array at Scene+0x69c..0x6a4 is +0x624's +0x78 header, which
// 0x0004bb9c grows through phys_fn_004840 -- a phys_fn_004803 container -- so it
// stays on nxGetSdkAllocator, as do the 0x3c/0x90 pruners built by 0x000b5090 and
// their 0x60/0x10 entry buffers from 0x000effc0.
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

// ---------------------------------------------------------------------------
// The pruning collection at Scene+0x624 (phys_fn_001980 builds it) is the
// OPCODE pruning engine, opcode/IcePruningEngine.cpp: its pruners sit at
// +0x1c + 4 type (the static one, type 0, at Scene+0x640; the dynamic one,
// type 2, at Scene+0x648), created on first use; +0x70 holds the type a
// dynamic prunable takes (2). A shape's prunable is the sub-object at +0xa4:
// +0xc4 its pruner, +0xcc its handle (0xffff when it has none), +0xce its type
// (phys_fn_004888), +0xcf its section (phys_fn_004890: 2 for a group root, 1
// for a single root, 0 for a group child). The engine's add and remove are
// the rows phys_fn_004857 and phys_fn_004859 (scene-raycast block Task 3); the
// NpActor.cpp completion's model of the pruners (its table allocation, growth
// and swap-removal) was replaced by them at the second merge of main into the
// scene-raycast block.
// ---------------------------------------------------------------------------

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

// Row 004857 on a shape whose prunable type (+0xce) and section (+0xcf)
// the caller has set (001941, 001943).
static void nxScenePrunerInsert(NxSceneInternal* scene, unsigned char* shape)
	{
	nxSceneEngineAddShape(&scene->at<unsigned char>(0x624), shape, shape[0xce], shape[0xcf]);
	}

// Row 004859 on a shape's prunable (001945).
static void nxScenePrunerErase(NxSceneInternal* scene, unsigned char* shape)
	{
	if(!shape) return;
	nxSceneEngineRemoveShape(&scene->at<unsigned char>(0x624), shape);
	}

// The actor-creation registration of a dynamic body (this file's model of
// the 000034 -> 000531 path): the root takes the pruning collection at +0xa0
// and joins its +0x78 list; its pruner type comes from the descriptor-selected
// bounded/unbounded dynamic pool. A group's children enter section 0 before
// the group in section 2; a single root goes in section 1.
void nxSceneBroadphaseRegister(NxSceneInternal* scene, void* bodyPointer)
	{
	unsigned char* body = static_cast<unsigned char*>(bodyPointer);
	if(!body || !*reinterpret_cast<void**>(body + 8)) return;
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(!shape) return;
	*reinterpret_cast<void**>(shape + 0xa0) = scene->bytes() + 0x624;
	const unsigned type = scene->at<unsigned>(0x624 + 0x70);
	nxSceneEngineAddRoot(scene, shape, type);
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
			const NxShapeDesc* shapeDesc = reinterpret_cast<const NxShapeDesc*>(
				*reinterpret_cast<const unsigned*>(d[0x13]));
			void* shape = nxShapeFactory(const_cast<NxShapeDesc*>(shapeDesc), actor);
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
		// The constructor uses +4 as its temporary Scene link. The body has
		// already copied that link; this is NxActor::userData on the public
		// wrapper, and the descriptor promises to initialise it.
		a[1] = d[0x10];
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
		a[1] = d[0x10];
		return 1;
		}
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\Actor.cpp", 0xe6, 0,
		"Actor::loadFromDescInternal: Can't compute mass from shapes: "
		"must have at least one non-trigger shape!");
	return 0;
	}

// phys_fn_000013 (0x00001450) is the actor constructor.
void* nxSceneCreateActorBody(void* memory, void* scene)
	{
	unsigned* a = static_cast<unsigned*>(memory);
	unsigned* s = static_cast<unsigned*>(scene);

	a[1] = reinterpret_cast<unsigned>(scene);
	a[0x10 / 4] = 0;				// shape list empty
	a[0x14 / 4] = 0;
	a[8 / 4] = 0;

	// The scene hands out a slot id: either the counter at +0x6d0 is incremented, or
	// the free list at +0x6d4..+0x6d8 is popped.
#if NX_PHYSICS_USE_X87
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
#else
	const unsigned slot = nxSceneActorIds(*static_cast<NxSceneInternal*>(scene)).take();
#endif
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
	// twelve globalPose floats and the body's twelve, plus a shape validity loop,
	// once per descriptor type ([desc+0x48]: 1 at 0x11e99, 2 at 0x11a2d, any
	// other at 0x1175f). For the two shape-list types it is the whole of the
	// pinned header's NxActorDesc::isValid(), isValidInternal included (0x11d23:
	// exactly one of density, mass, or mass with tensor) -- NxActorDescBase::
	// isValid() is not virtual, so calling it through the base reference, as
	// this transcription did, skipped that half (actor-mass Task 1: a density
	// with an explicit mass was accepted). The two list types share one layout,
	// which Actor::loadFromDescInternal reads the same way for both. The
	// report goes through FoundationSDK::error with Scene.cpp's __FILE__ and
	// the arm's line: 0x203, 0x209, 0x21b.
	const NxU32 descType = static_cast<NxU32>(desc.getType());
	const bool shapeList = descType == NX_ADT_DEFAULT || descType == NX_ADT_ALLOCATOR;
	if(shapeList ? !static_cast<const NxActorDesc&>(desc).isValid() : !desc.isValid())
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_SCENE_CPP,
			descType == NX_ADT_DEFAULT ? 0x203 : descType == NX_ADT_ALLOCATOR ? 0x209 : 0x21b,
			0, "Supplied NxActorDesc is not valid. createActor returns NULL.");
		return 0;
		}

	// The oracle allocates the 0x50-byte body before the public 0x18-byte
	// actor wrapper. Keep that order; the guarded allocator records it.
	void* outerMemory = nxFoundationSDKAllocator->malloc(0x50, NX_MEMORY_PERSISTENT);
	if(!outerMemory)
		return 0;
	void* actorMemory = nxFoundationSDKAllocator->malloc(0x18, NX_MEMORY_PERSISTENT);
	if(!actorMemory)
		{
		nxFoundationSDKAllocator->free(outerMemory);
		return 0;
		}

	// phys_fn_00001450 constructs the public wrapper. The 0x50-byte internal
	// actor remains the storage behind its +0x14 pointer.
	static_cast<NpActorObject*>(actorMemory)->installVtable();
	NxActor* actor = static_cast<NxActor*>(nxSceneCreateActorBody(actorMemory, this));
	if(!actor)
		{
		nxFoundationSDKAllocator->free(actorMemory);
		nxFoundationSDKAllocator->free(outerMemory);
		return 0;
		}
	static_cast<NpActorObject*>(actorMemory)->installSecondaryVtable();
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
		nxSceneNotifyActorCreated(reinterpret_cast<void*>(p[0x61c / 4]), actor);

	return actor;
	}

// phys_fn_000628 (0x000123d0, 241 B)
// Scene::releaseActor(body). Under the API reentry flag (.data 0x10123c10;
// set: code 2, line 0x492, the message at 0x10122050): the public actor
// ([body]) is searched for in the +0x55c array; not found is code 2, line
// 0x4ae, "Scene::releaseActor: double deletion detected!". Found: the last
// entry takes its place and the array shrinks; a fluid manager at +0x61c
// takes 003635 (0x1248c-0x12497), which is NOT written: 003635 walks the
// manager's fluids through 003485 into the emitter rows 003593/003622. The
// disabled-manager warning path is reconstructed and dynamically checked;
// notifications for live fluids still depend on the open 003485/003593/003622
// chain. Actor.cpp's 000030 destroys the actor and the body is freed through
// [0x101041bc]; the flag is cleared.
void nxActorDestroy(unsigned char* body);

// .data 0x10123c10: the one API reentry flag. Scene::createJoint and
// releaseJoint, Scene::createEffector and releaseEffector, Scene::releaseActor
// and Actor.cpp's createShape and
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
	if(void* fluidManager = at<void*>(0x61c))
		nxFluidManagerNotifyActorReleasedDisabled(fluidManager, body);
	nxActorDestroy(body);
	nxFoundationSDKAllocator->free(body);
	gNxApiReentry = false;
	}

// phys_fn_000305/000307 (Controller.cpp, public NxScene slots 18/19) expose a
// controller object whose public primary base starts at allocation+0 and whose
// controller-list node starts at allocation+8. The controller API header is not
// part of this SDK tree, so preserve that ABI privately rather than inventing a
// public declaration. The embedded proxy's scalar deleting destructor owns the
// 0x4c allocation; the oracle's release path does not release its generated
// actor, so that actor deliberately remains registered in the Scene.
namespace
	{
	// Continuous SAT for an axis-aligned controller box translated past one
	// static triangle. The candidate path uses the same 13 separating axes as a
	// static AABB/triangle test, but clips the overlap interval along displacement.
	static bool nxControllerSweepAABBTriangle(const NxVec3& start, const NxVec3& displacement,
		const NxVec3& extents, const NxVec3 triangle[3], NxReal& hitFraction,
		NxVec3& hitNormal)
		{
		const NxVec3 worldAxis[3] = {
			NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f), NxVec3(0.0f, 0.0f, 1.0f)};
		const NxVec3 edge[3] = {
			triangle[1] - triangle[0], triangle[2] - triangle[1], triangle[0] - triangle[2]};
		NxVec3 axes[13];
		NxU32 axisCount = 0;
		for(NxU32 i = 0; i != 3; ++i)
			axes[axisCount++] = worldAxis[i];
		axes[axisCount++] = edge[0] ^ edge[1];
		for(NxU32 i = 0; i != 3; ++i)
			for(NxU32 j = 0; j != 3; ++j)
				axes[axisCount++] = edge[i] ^ worldAxis[j];

		NxReal enter = 0.0f;
		NxReal leave = 1.0f;
		NxU32 entryAxis = 0;
		bool startsOverlapped = true;
		for(NxU32 i = 0; i != axisCount; ++i)
			{
			const NxVec3& axis = axes[i];
			if(axis.magnitudeSquared() < 1.0e-12f)
				continue;
			NxReal triangleMin = triangle[0].dot(axis);
			NxReal triangleMax = triangleMin;
			for(NxU32 vertex = 1; vertex != 3; ++vertex)
				{
				const NxReal projection = triangle[vertex].dot(axis);
				if(projection < triangleMin) triangleMin = projection;
				if(projection > triangleMax) triangleMax = projection;
				}
			const NxReal centerProjection = start.dot(axis);
			const NxReal deltaProjection = displacement.dot(axis);
			const NxReal radius = NxMath::abs(axis.x) * extents.x +
				NxMath::abs(axis.y) * extents.y + NxMath::abs(axis.z) * extents.z;
			if(centerProjection + radius < triangleMin ||
				centerProjection - radius > triangleMax)
				startsOverlapped = false;
			if(deltaProjection == 0.0f)
				{
				if(centerProjection + radius < triangleMin ||
					centerProjection - radius > triangleMax)
					return false;
				continue;
				}
			NxReal first = (triangleMin - radius - centerProjection) / deltaProjection;
			NxReal last = (triangleMax + radius - centerProjection) / deltaProjection;
			if(first > last)
				{
				const NxReal swap = first;
				first = last;
				last = swap;
				}
			if(first > enter)
				{
				enter = first;
				entryAxis = i;
				}
			if(last < leave)
				leave = last;
			if(enter > leave)
				return false;
			}
		// The pinned controller resolver ignores an obstacle triangle that
		// already intersects the controller at the start of this sweep. Its
		// movement therefore escapes the initial mesh overlap in either direction.
		if(startsOverlapped)
			return false;
		// The controller's mesh query is one-sided: a reversed-winding copy of
		// the same face is ignored when motion approaches from its back side.
		const NxVec3 triangleNormal = edge[0] ^ edge[1];
		if(triangleNormal.dot(displacement) >= 0.0f)
			return false;
		if(leave < 0.0f || enter < 0.0f || enter > 1.0f)
			return false;
		hitFraction = enter;
		hitNormal = axes[entryAxis];
		return true;
		}

	// The native mesh callback first sweeps the controller's eight vertices
	// against each triangle before checking face and edge crossings. Keep this
	// vertex path separate from the SAT fallback so correction probes can use
	// the callback's vertex-hit point and face normal.
	static bool nxControllerSweepBoxVerticesTriangle(const NxVec3& start,
		const NxVec3& displacement, const NxVec3& extents, const NxVec3 triangle[3],
		NxReal& hitDistance, NxVec3& hitNormal)
		{
		const NxReal travelDistance = NxMath::sqrt(displacement.magnitudeSquared());
		if(travelDistance == 0.0f)
			return false;
		const NxVec3 rayDirection = displacement * (1.0f / travelDistance);
		NxReal closest = hitDistance < travelDistance ? hitDistance : travelDistance;
		NxVec3 normal = (triangle[1] - triangle[0]) ^ (triangle[2] - triangle[0]);
		const NxReal normalLengthSquared = normal.magnitudeSquared();
		if(normalLengthSquared == 0.0f)
			return false;
		normal *= 1.0f / NxMath::sqrt(normalLengthSquared);
		const NxVec3 triangleCenter(
			((triangle[0].x + triangle[1].x) + triangle[2].x) * 0.33333334f,
			((triangle[0].y + triangle[1].y) + triangle[2].y) * 0.33333334f,
			((triangle[0].z + triangle[1].z) + triangle[2].z) * 0.33333334f);
		NxVec3 expandedTriangle[3];
		for(NxU32 vertex = 0; vertex != 3; ++vertex)
			{
			const NxVec3 offset = triangle[vertex] - triangleCenter;
			const NxReal offsetX = offset.x * 0.02f;
			const NxReal offsetY = offset.y * 0.02f;
			const NxReal offsetZ = offset.z * 0.02f;
			expandedTriangle[vertex].set(triangle[vertex].x + offsetX,
				triangle[vertex].y + offsetY, triangle[vertex].z + offsetZ);
			}
		bool found = false;
		for(NxU32 corner = 0; corner != 8; ++corner)
			{
			const NxVec3 vertexStart(
				start.x + ((corner & 1u) ? extents.x : -extents.x),
				start.y + ((corner & 2u) ? extents.y : -extents.y),
				start.z + ((corner & 4u) ? extents.z : -extents.z));
			NxReal distance = closest;
			NxReal barycentricU = 0.0f;
			NxReal barycentricV = 0.0f;
			if(NxRayTriIntersect(vertexStart, rayDirection, expandedTriangle[0],
				expandedTriangle[1], expandedTriangle[2], distance, barycentricU,
				barycentricV, true) &&
				distance >= 0.0f && distance < closest)
				{
				closest = distance;
				found = true;
				}
			}
		if(!found)
			return false;
		if(normal.dot(displacement) > 0.0f)
			normal *= -1.0f;
		hitDistance = closest;
		hitNormal = normal;
		return true;
		}

	// Match sub_10058870's x87/double normalization and float spill order without
	// inline-assembly aggregate operands that MSVC miscompiles in this helper.
	static void nxControllerResponseTangent(const NxVec3& direction,
		const NxVec3& normal, NxVec3& tangent, NxVec3& normalPart)
		{
		NxReal reflectedDot = normal.z * direction.z;
		reflectedDot = reflectedDot + normal.y * direction.y;
		reflectedDot = reflectedDot + normal.x * direction.x;
		NxVec3 reflected(
			direction.x - (normal.x + normal.x) * reflectedDot,
			direction.y - (normal.y + normal.y) * reflectedDot,
			direction.z - (normal.z + normal.z) * reflectedDot);
	const double reflectedLengthSquared =
		static_cast<double>(reflected.z) * reflected.z +
		static_cast<double>(reflected.y) * reflected.y +
		static_cast<double>(reflected.x) * reflected.x;
	if(reflectedLengthSquared != 0.0)
		{
		const double reflectedInverseLength = 1.0 / sqrt(reflectedLengthSquared);
		reflected.set(
			static_cast<NxReal>(static_cast<double>(reflected.x) * reflectedInverseLength),
			static_cast<NxReal>(static_cast<double>(reflected.y) * reflectedInverseLength),
			static_cast<NxReal>(static_cast<double>(reflected.z) * reflectedInverseLength));
		}
	const double normalProjection =
		static_cast<double>(normal.y) * reflected.y +
		static_cast<double>(reflected.z) * normal.z +
		static_cast<double>(normal.x) * reflected.x;
	const NxReal normalPartX = static_cast<NxReal>(static_cast<double>(normal.x) * normalProjection);
	const NxReal normalPartY = static_cast<NxReal>(static_cast<double>(normal.y) * normalProjection);
	const NxReal normalPartZ = static_cast<NxReal>(static_cast<double>(normal.z) * normalProjection);
	normalPart.set(normalPartX, normalPartY, normalPartZ);
		tangent.set(reflected.x - normalPart.x,
			reflected.y - normalPart.y, reflected.z - normalPart.z);
	const double normalPartLengthSquared =
		static_cast<double>(normalPart.z) * normalPart.z +
		static_cast<double>(normalPart.y) * normalPart.y +
		static_cast<double>(normalPart.x) * normalPart.x;
	if(normalPartLengthSquared != 0.0)
		{
		const double inverseLength = 1.0 / sqrt(normalPartLengthSquared);
		normalPart.set(
			static_cast<NxReal>(static_cast<double>(normalPart.x) * inverseLength),
			static_cast<NxReal>(static_cast<double>(normalPart.y) * inverseLength),
			static_cast<NxReal>(static_cast<double>(normalPart.z) * inverseLength));
		}
	const double tangentLengthSquared =
		static_cast<double>(tangent.z) * tangent.z +
		static_cast<double>(tangent.y) * tangent.y +
		static_cast<double>(tangent.x) * tangent.x;
	if(tangentLengthSquared != 0.0)
		{
		const double inverseLength = 1.0 / sqrt(tangentLengthSquared);
		tangent.set(
			static_cast<NxReal>(static_cast<double>(tangent.x) * inverseLength),
			static_cast<NxReal>(static_cast<double>(tangent.y) * inverseLength),
			static_cast<NxReal>(static_cast<double>(tangent.z) * inverseLength));
		}
		}


	// The pinned three-slot primary vtable is scalar-deleting destructor,
	// move(NxVec3, activeGroups, minDistance, collisionFlags), and getPosition.
	// The getter is reconstructed below. The collision-aware move algorithm is
	// still open; keep the slot present so callers dispatch through the recovered
	// ABI while that behavior is completed.
	struct NxControllerCore
		{
		virtual ~NxControllerCore() {}
		virtual void move(const NxVec3& displacement, NxU32 activeGroups,
			NxReal minDistance, NxU32& collisionFlags, NxReal sharpness,
			const NxGroupsMask* groupsMask)
			{
			(void)sharpness;
			(void)groupsMask;
			// Reconstruct the axis-aligned box sweep used by the recovered
	// controller resolver. General transformed/convex sweeps and slide/step
			// response remain separate open paths.
			unsigned char* bytes = reinterpret_cast<unsigned char*>(this);
			NxVec3& position = *reinterpret_cast<NxVec3*>(bytes + 0x28);
			const bool stepProbeEnabled = *reinterpret_cast<const NxU32*>(bytes + 0x3c) != 0;
			collisionFlags = 0;
			const NxReal x = displacement.x;
			const NxReal y = displacement.y;
			const NxReal z = displacement.z;
			const NxReal distanceSquared = x * x + y * y + z * z;
			if(distanceSquared < minDistance * minDistance)
				return;

			NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(bytes + 0x34);
			NxActor* actor = *reinterpret_cast<NxActor**>(bytes + 0x24);
			const NxVec3& extents = *reinterpret_cast<const NxVec3*>(bytes + 0x40);
			const NxU32 upAxis = *reinterpret_cast<const NxU32*>(bytes + 0x0c);
			const NxReal stepOffset = *reinterpret_cast<const NxReal*>(bytes + 0x18);
			const NxReal requestedUpMotion = displacement[upAxis];
			const NxReal initialUpProbe = requestedUpMotion > 0.0f ? 0.0f : stepOffset;
			NxVec3 finalProbeNormal(0.0f, 0.0f, 0.0f);
			NxVec3 finalProbeSlopeNormal(0.0f, 0.0f, 0.0f);
			NxVec3 finalProbeTriangle[3];
			bool finalProbeHitTriangle = false;
			NxVec3 motionPhases[3] = {
				NxVec3(0.0f, 0.0f, 0.0f), displacement,
				NxVec3(0.0f, 0.0f, 0.0f)};
			motionPhases[0][upAxis] = initialUpProbe;
			motionPhases[1][upAxis] = 0.0f;
			motionPhases[2][upAxis] = requestedUpMotion - initialUpProbe;
			const NxU32 motionPhaseFlags[3] = {2u, 1u, 4u};
			const NxU32 motionPhaseCount = 3;
			for(NxU32 phase = 0; phase != motionPhaseCount; ++phase)
				{
				NxVec3 remaining = motionPhases[phase];
				for(NxU32 iteration = 0; iteration != 4; ++iteration)
					{
					const NxReal endX = position.x + remaining.x;
					const NxReal endY = position.y + remaining.y;
					const NxReal endZ = position.z + remaining.z;
					const NxReal moveX = endX - position.x;
					const NxReal moveY = endY - position.y;
					const NxReal moveZ = endZ - position.z;
					const NxReal stepDistance = NxMath::sqrt(
						moveX * moveX + moveY * moveY + moveZ * moveZ);
					if(stepDistance < minDistance)
						break;
					NxReal fraction = 1.0f;
					NxU32 hitAxis = 3;
					bool hitTriangle = false;
					NxVec3 triangleHitNormal(0.0f, 0.0f, 0.0f);
					NxVec3 triangleHitSlopeNormal(0.0f, 0.0f, 0.0f);
					NxVec3 triangleHitVertices[3];
					if(scene)
						{
					NxBounds3 sweptBounds;
					sweptBounds.set(
						(position.x < endX ? position.x : endX) - extents.x,
						(position.y < endY ? position.y : endY) - extents.y,
						(position.z < endZ ? position.z : endZ) - extents.z,
						(position.x > endX ? position.x : endX) + extents.x,
						(position.y > endY ? position.y : endY) + extents.y,
						(position.z > endZ ? position.z : endZ) + extents.z);
					NxShape* candidates[128];
					const NxU32 candidateCount = scene->overlapAABBShapes(
						sweptBounds, NX_ALL_SHAPES, 128, candidates, 0);
					for(NxU32 i = 0; i < candidateCount; ++i)
						{
						NxShape* shape = candidates[i];
						if(!shape || (actor && &shape->getActor() == actor))
							continue;
						const NxU32 group = shape->getGroup();
						if(group >= 32 || !(activeGroups & (1u << group)))
							continue;
						if(shape->getType() == NX_SHAPE_BOX)
							{
							NxBoxShape* box = shape->isBox();
							NxBox worldBox;
							box->getWorldOBB(worldBox);
							const NxMat33& orientation = worldBox.GetRot();
							const NxVec3 boxAxis[3] = {
								orientation.getColumn(0), orientation.getColumn(1),
								orientation.getColumn(2)};
							const bool oriented =
								NxMath::abs(boxAxis[0].y) > 0.0001f ||
								NxMath::abs(boxAxis[0].z) > 0.0001f ||
								NxMath::abs(boxAxis[1].x) > 0.0001f ||
								NxMath::abs(boxAxis[1].z) > 0.0001f ||
								NxMath::abs(boxAxis[2].x) > 0.0001f ||
								NxMath::abs(boxAxis[2].y) > 0.0001f;
							if(oriented)
								{
								const NxVec3& boxPosition = worldBox.GetCenter();
								const NxVec3& boxExtents = worldBox.GetExtents();
								const NxVec3 relativeStart = position - boxPosition;
								const NxVec3 worldAxis[3] = {
									NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f),
									NxVec3(0.0f, 0.0f, 1.0f)};
								NxVec3 sweepAxis[15];
								NxU32 sweepAxisCount = 0;
								for(NxU32 i = 0; i != 3; ++i)
									sweepAxis[sweepAxisCount++] = worldAxis[i];
								for(NxU32 i = 0; i != 3; ++i)
									sweepAxis[sweepAxisCount++] = boxAxis[i];
								for(NxU32 i = 0; i != 3; ++i)
									for(NxU32 j = 0; j != 3; ++j)
										sweepAxis[sweepAxisCount++] = worldAxis[i] ^ boxAxis[j];
								NxReal enter = 0.0f;
								NxReal leave = 1.0f;
								NxU32 entryAxis = 0;
								bool intersects = true;
								bool startsOverlapping = true;
								for(NxU32 axisIndex = 0; axisIndex != sweepAxisCount; ++axisIndex)
									{
									const NxVec3& axis = sweepAxis[axisIndex];
									if(axis.magnitudeSquared() < 1.0e-12f)
										continue;
									const NxReal startProjection = relativeStart.dot(axis);
									const NxReal deltaProjection = remaining.dot(axis);
									const NxReal controllerRadius =
										NxMath::abs(axis.x) * extents.x +
										NxMath::abs(axis.y) * extents.y +
										NxMath::abs(axis.z) * extents.z;
									const NxReal boxRadius = axisIndex >= 3 && axisIndex < 6 ?
										boxExtents[axisIndex - 3] :
										NxMath::abs(boxAxis[0].dot(axis)) * boxExtents.x +
										NxMath::abs(boxAxis[1].dot(axis)) * boxExtents.y +
										NxMath::abs(boxAxis[2].dot(axis)) * boxExtents.z;
									NxReal radius = controllerRadius + boxRadius;
									// Round the projected contact interval outward so a boundary
									// hit is not lost to the final float addition.
									radius += radius * 1.1920928955078125e-7f;
									if(startProjection < -radius || startProjection > radius)
										startsOverlapping = false;
									if(deltaProjection == 0.0f)
										{
										if(startProjection < -radius || startProjection > radius)
											intersects = false;
										continue;
										}
									NxReal first = (-radius - startProjection) / deltaProjection;
									NxReal last = (radius - startProjection) / deltaProjection;
									if(first > last)
										{
										const NxReal swap = first;
										first = last;
										last = swap;
										}
									if(first > enter)
										{
										enter = first;
										entryAxis = axisIndex;
										}
									if(last < leave)
										leave = last;
									if(enter > leave)
										intersects = false;
									}
								if(startsOverlapping)
									// The pinned rotated-box resolver accepts either direction
									// when a move begins inside the expanded OBB.
									continue;
								if(intersects && leave >= 0.0f && enter >= 0.0f && enter < fraction)
									{
									fraction = enter;
									const NxVec3& hitNormal = sweepAxis[entryAxis];
									hitAxis = hitNormal.y * hitNormal.y >
										0.5f * hitNormal.magnitudeSquared() ? 1u : 0u;
									}
								continue;
								}
							}
						// The pinned Controller resolver (0x00058ea0) only builds
						// sweep candidates for boxes (type 2) and triangle meshes
						// (type 4). Other shape bounds are not controller blockers.
						const NxShapeType obstacleType = shape->getType();
						if(obstacleType != NX_SHAPE_BOX && obstacleType != NX_SHAPE_MESH)
							continue;
						if(obstacleType == NX_SHAPE_MESH)
							{
							NxTriangleMesh& mesh = shape->isTriangleMesh()->getTriangleMesh();
							const NxMat34 pose = shape->getGlobalPose();
							for(NxU32 submesh = 0; submesh < mesh.getSubmeshCount(); ++submesh)
								{
								const NxU32 vertexCount = mesh.getCount(submesh, NX_ARRAY_VERTICES);
								const NxU32 triangleCount = mesh.getCount(submesh, NX_ARRAY_TRIANGLES);
								const NxU32 vertexStride = mesh.getStride(submesh, NX_ARRAY_VERTICES);
								const NxU32 triangleStride = mesh.getStride(submesh, NX_ARRAY_TRIANGLES);
								const NxU8* const vertexData = static_cast<const NxU8*>(
									mesh.getBase(submesh, NX_ARRAY_VERTICES));
								const NxU8* const triangleData = static_cast<const NxU8*>(
									mesh.getBase(submesh, NX_ARRAY_TRIANGLES));
								const NxInternalFormat indexFormat = mesh.getFormat(submesh, NX_ARRAY_TRIANGLES);
								if(mesh.getFormat(submesh, NX_ARRAY_VERTICES) != NX_FORMAT_FLOAT ||
									!vertexData || !triangleData || vertexStride < sizeof(NxVec3))
									continue;
								for(NxU32 triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
									{
									const NxU8* const indices = triangleData + triangleIndex * triangleStride;
									NxU32 vertexIndices[3];
									if(indexFormat == NX_FORMAT_INT && triangleStride >= 3 * sizeof(NxU32))
										memcpy(vertexIndices, indices, sizeof(vertexIndices));
									else if(indexFormat == NX_FORMAT_SHORT && triangleStride >= 3 * sizeof(NxU16))
										{
										NxU16 shortIndices[3];
										memcpy(shortIndices, indices, sizeof(shortIndices));
										vertexIndices[0] = shortIndices[0];
										vertexIndices[1] = shortIndices[1];
										vertexIndices[2] = shortIndices[2];
										}
									else
										continue;
									if(vertexIndices[0] >= vertexCount || vertexIndices[1] >= vertexCount ||
										vertexIndices[2] >= vertexCount)
										continue;
									NxVec3 triangle[3];
									for(NxU32 vertex = 0; vertex != 3; ++vertex)
										{
										NxVec3 localVertex;
										memcpy(&localVertex, vertexData + vertexIndices[vertex] * vertexStride,
											sizeof(localVertex));
										triangle[vertex] = pose * localVertex;
										}
									NxReal meshFraction;
									NxVec3 meshNormal;
									if(nxControllerSweepAABBTriangle(position, remaining, extents,
										triangle, meshFraction, meshNormal) && meshFraction < fraction)
										{
										fraction = meshFraction;
										hitTriangle = true;
										triangleHitNormal = meshNormal;
										// Preserve the actual face normal separately for slope
										// classification; SAT still supplies response arithmetic.
										triangleHitSlopeNormal = (triangle[1] - triangle[0]) ^
											(triangle[2] - triangle[0]);
										const NxReal triangleNormalLengthSquared =
											triangleHitSlopeNormal.magnitudeSquared();
										if(triangleNormalLengthSquared > 0.0f)
											triangleHitSlopeNormal *= 1.0f / NxMath::sqrt(
												triangleNormalLengthSquared);
										for(NxU32 vertex = 0; vertex != 3; ++vertex)
											triangleHitVertices[vertex] = triangle[vertex];
									const bool horizontalOnlyYUp = upAxis == 1 && remaining.y == 0.0f &&
										(remaining.x != 0.0f || remaining.z != 0.0f);
									if(horizontalOnlyYUp)
										hitAxis = NxMath::abs(remaining.x) >= NxMath::abs(remaining.z) ? 0u : 2u;
									else
										hitAxis = meshNormal.y * meshNormal.y >
											0.5f * meshNormal.magnitudeSquared() ? 1u : 0u;
										}
									}
								}
							continue;
							}
						NxBounds3 shapeBounds;
						shape->getWorldBounds(shapeBounds);
						const NxVec3& lo = shapeBounds.getMin();
						const NxVec3& hi = shapeBounds.getMax();
						const NxReal expandedMin[3] = {
							lo.x - extents.x, lo.y - extents.y, lo.z - extents.z};
						const NxReal expandedMax[3] = {
							hi.x + extents.x, hi.y + extents.y, hi.z + extents.z};
						const NxReal start[3] = {position.x, position.y, position.z};
						const NxReal delta[3] = {remaining.x, remaining.y, remaining.z};
						NxU32 entryAxis = 0;
						bool startsInside = true;
						for(NxU32 axis = 0; axis != 3; ++axis)
							if(start[axis] <= expandedMin[axis] || start[axis] >= expandedMax[axis])
								startsInside = false;
						if(startsInside)
							{
							NxU32 escapeAxis = 0;
							NxReal escapeDirection = -1.0f;
							NxReal escapeDistance = start[0] - expandedMin[0];
							for(NxU32 axis = 0; axis != 3; ++axis)
								{
								const NxReal towardMax = expandedMax[axis] - start[axis];
								if(towardMax < escapeDistance)
									{
									escapeAxis = axis;
									escapeDirection = 1.0f;
									escapeDistance = towardMax;
									}
								const NxReal towardMin = start[axis] - expandedMin[axis];
								if(towardMin < escapeDistance)
									{
									escapeAxis = axis;
									escapeDirection = -1.0f;
									escapeDistance = towardMin;
									}
								}
							if(delta[escapeAxis] * escapeDirection > 0.0f)
								continue;
							}
						NxReal enter = 0.0f;
						NxReal leave = 1.0f;
						NxU32 axis = 0;
						bool intersects = true;
						for(; axis != 3; ++axis)
							{
							if(delta[axis] == 0.0f)
								{
								if(start[axis] <= expandedMin[axis] || start[axis] >= expandedMax[axis])
									intersects = false;
								continue;
								}
							NxReal first = (expandedMin[axis] - start[axis]) / delta[axis];
							NxReal last = (expandedMax[axis] - start[axis]) / delta[axis];
							if(first > last)
								{
								const NxReal swap = first;
								first = last;
								last = swap;
								}
							// A contact exactly at the start of the sweep is still
							// entering when motion points into that face. Preserve its
							// axis instead of leaving the default X hit normal.
							const bool inwardBoundaryContact = first == 0.0f &&
								((start[axis] == expandedMin[axis] && delta[axis] > 0.0f) ||
								 (start[axis] == expandedMax[axis] && delta[axis] < 0.0f));
							if(!startsInside && (first > enter || inwardBoundaryContact))
								{
								enter = first;
								entryAxis = axis;
								}
							if(last < leave)
								leave = last;
							if(enter > leave)
								intersects = false;
							}
						if(intersects && leave >= 0.0f && enter >= 0.0f && enter < fraction)
							{
							fraction = enter;
							hitAxis = entryAxis;
							}
						}
					}
					position.set(position.x + remaining.x * fraction,
						position.y + remaining.y * fraction,
						position.z + remaining.z * fraction);
					if(fraction >= 1.0f)
						break;
					if(phase == 2 && hitTriangle)
						{
						finalProbeHitTriangle = true;
						finalProbeNormal = triangleHitNormal;
						finalProbeSlopeNormal = triangleHitSlopeNormal;
						for(NxU32 vertex = 0; vertex != 3; ++vertex)
							finalProbeTriangle[vertex] = triangleHitVertices[vertex];
						}
					collisionFlags |= motionPhaseFlags[phase];
					remaining.set(remaining.x * (1.0f - fraction),
						remaining.y * (1.0f - fraction),
						remaining.z * (1.0f - fraction));
					if(hitTriangle)
						remaining.set(0.0f, 0.0f, 0.0f);
					else if(hitAxis == 0)
						remaining.x = 0.0f;
					else if(hitAxis == 1)
						remaining.y = 0.0f;
					else if(hitAxis == 2)
						remaining.z = 0.0f;
					}
				}
			// The resolver's fourth query is enabled only for a descending final
			// probe on a surface below the stored slope threshold. It advances to
			// the triangle hit, then projects the remaining distance onto the
			// contact tangent while temporary correction mode is active.
			const NxReal correctionSlopeThreshold =
				*reinterpret_cast<const NxReal*>(bytes + 0x10);
			if(stepProbeEnabled && finalProbeHitTriangle && requestedUpMotion < 0.0f)
				{
				const NxReal normalLengthSquared = finalProbeSlopeNormal.magnitudeSquared();
				if(normalLengthSquared > 0.0f)
					{
					finalProbeSlopeNormal *= 1.0f / NxMath::sqrt(normalLengthSquared);
					const NxReal responseNormalLengthSquared = finalProbeNormal.magnitudeSquared();
					if(responseNormalLengthSquared > 0.0f)
						finalProbeNormal *= 1.0f / NxMath::sqrt(responseNormalLengthSquared);
					const NxReal normalUp = finalProbeSlopeNormal[upAxis];
					if(normalUp >= 0.0f && normalUp < correctionSlopeThreshold)
						{
						NxVec3 correctionDirection(0.0f, 0.0f, 0.0f);
						correctionDirection[upAxis] = -1.0f;
						const NxReal heightTravel = stepOffset > initialUpProbe ?
							stepOffset - initialUpProbe : 0.0f;
						const NxReal correctionDistance = heightTravel +
							NxMath::abs(requestedUpMotion);
						if(correctionDistance >= minDistance)
							{
							const NxVec3 probeStart = position;
							NxReal probeDistance = correctionDistance;
							NxVec3 probeNormal(0.0f, 0.0f, 0.0f);
							if(nxControllerSweepBoxVerticesTriangle(probeStart,
								correctionDirection * correctionDistance, extents,
								finalProbeTriangle, probeDistance, probeNormal))
								{
								const NxReal probeNormalLengthSquared =
									probeNormal.magnitudeSquared();
								if(probeNormalLengthSquared > 0.0f)
									probeNormal *= 1.0f / NxMath::sqrt(probeNormalLengthSquared);
								if(probeNormal.dot(correctionDirection) > 0.0f)
									probeNormal *= -1.0f;
								const NxVec3 correctionTarget = position +
									correctionDirection * correctionDistance;
								position += correctionDirection *
									probeDistance;
								NxVec3 tangent(0.0f, 0.0f, 0.0f);
								NxVec3 normalPart(0.0f, 0.0f, 0.0f);
								nxControllerResponseTangent(correctionDirection,
									finalProbeNormal, tangent, normalPart);
								const NxReal remainingX = correctionTarget.x - position.x;
								const NxReal remainingY = correctionTarget.y - position.y;
								const NxReal remainingZ = correctionTarget.z - position.z;
								const NxReal remainingDistance = static_cast<NxReal>(sqrt(
									static_cast<double>(remainingY) * remainingY +
									static_cast<double>(remainingZ) * remainingZ +
									static_cast<double>(remainingX) * remainingX));
								if(remainingDistance >= minDistance)
									position.set(
										static_cast<NxReal>(static_cast<double>(tangent.x) * remainingDistance + position.x),
										static_cast<NxReal>(static_cast<double>(tangent.y) * remainingDistance + position.y),
										static_cast<NxReal>(static_cast<double>(tangent.z) * remainingDistance + position.z));
								}
							// A qualifying face correction consumes the final downward
							// probe flag even when the corner-ray subquery has no hit.
							collisionFlags &= ~4u;
							}
						}
					}
				}
			if(actor)
				actor->moveGlobalPosition(position);
			}
		virtual const NxVec3& getPosition() const
			{
			return *reinterpret_cast<const NxVec3*>(
				reinterpret_cast<const unsigned char*>(this) + 0x28);
			}
		static void operator delete(void* memory)
			{
			if(memory && nxFoundationSDKAllocator)
				nxFoundationSDKAllocator->free(memory);
			}
		};

	struct NxControllerProxy
		{
		virtual ~NxControllerProxy() {}
		static void operator delete(void* memory)
			{
			if(memory && nxFoundationSDKAllocator)
				nxFoundationSDKAllocator->free(static_cast<unsigned char*>(memory) - 8);
			}
		};
	}

NxController* NxSceneInternal::createController(const NxControllerDesc& desc)
	{
	const unsigned char* descriptor = reinterpret_cast<const unsigned char*>(&desc);
	// The pinned Controller.cpp accepts the descriptor only when its type word
	// at +8 is zero. Other descriptor kinds return null without allocating.
	if(*reinterpret_cast<const NxU32*>(descriptor + 8) != 0)
		return 0;

	unsigned char* memory = static_cast<unsigned char*>(
		nxFoundationSDKAllocator->malloc(0x4c, NX_MEMORY_PERSISTENT));
	if(!memory)
		return 0;
	memset(memory, 0, 0x4c);
	NxControllerCore* core = new (memory) NxControllerCore();
	NxControllerProxy* proxy = new (memory + 8) NxControllerProxy();
	*reinterpret_cast<void**>(memory + 4) = proxy;
	*reinterpret_cast<NxSceneInternal**>(memory + 0x34) = this;

	// Controller::Controller copies its position components from descriptor
	// +0x0c..+0x14 to object +0x28 and the dimensions from +0x30. It creates a
	// kinematic box actor with a 10-unit density and 1.1x controller extents.
	// Using the existing actor
	// factory retains its scene array, shape, body, and notification semantics.
	memcpy(memory + 0x28, descriptor + 0x0c, sizeof(NxReal) * 3);
	// Controller::Controller copies descriptor +0x1c != 0 into object +0x34;
	// that probe-enable byte is independent of the step-offset value at +0x2c.
	*reinterpret_cast<NxU32*>(memory + 0x3c) =
		*reinterpret_cast<const NxReal*>(descriptor + 0x1c) != 0.0f ? 1u : 0u;
	// Controller::move selects its up axis from descriptor +0x18 (private +0x0c).
	*reinterpret_cast<NxU32*>(memory + 0x0c) =
		*reinterpret_cast<const NxU32*>(descriptor + 0x18);
	*reinterpret_cast<NxReal*>(memory + 0x10) =
		*reinterpret_cast<const NxReal*>(descriptor + 0x1c);
	// Preserve the constructor's contiguous descriptor-derived controller state.
	*reinterpret_cast<NxU32*>(memory + 0x14) =
		*reinterpret_cast<const NxU32*>(descriptor + 0x20);
	*reinterpret_cast<NxReal*>(memory + 0x18) =
		*reinterpret_cast<const NxReal*>(descriptor + 0x24);
	*reinterpret_cast<NxReal*>(memory + 0x20) =
		*reinterpret_cast<const NxReal*>(descriptor + 0x2c);
	const NxReal* dimensions = reinterpret_cast<const NxReal*>(descriptor + 0x30);
	NxBoxShapeDesc box;
	box.dimensions.set(dimensions[0] * 1.1f, dimensions[1] * 1.1f,
		dimensions[2] * 1.1f);
	NxBodyDesc body;
	body.flags = NX_BF_KINEMATIC;
	NxActorDesc actorDesc;
	actorDesc.globalPose.t.set(
		*reinterpret_cast<const NxReal*>(descriptor + 0x0c),
		*reinterpret_cast<const NxReal*>(descriptor + 0x10),
		*reinterpret_cast<const NxReal*>(descriptor + 0x14));
	actorDesc.body = &body;
	actorDesc.density = 10.0f;
	actorDesc.shapes.pushBack(&box);
	NxActor* actor = createActor(actorDesc);
	*reinterpret_cast<NxActor**>(memory + 0x24) = actor;
	memcpy(memory + 0x40, dimensions, sizeof(NxReal) * 3);

	// Scene::createController links the embedded object (allocation+8), with its
	// next pointer at node+0x30, to Scene+0x5a8.
	*reinterpret_cast<void**>(memory + 0x38) = at<void*>(0x5a8);
	at<void*>(0x5a8) = memory + 8;
	(void)core;
	return reinterpret_cast<NxController*>(memory);
	}

void NxSceneInternal::releaseController(NxController& controller)
	{
	if(gNxApiReentry)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\Controller.cpp", 97, 0,
			"Reentry check: You may not call this API method from a callback!");
		return;
		}
	gNxApiReentry = true;
	unsigned char* memory = reinterpret_cast<unsigned char*>(&controller);
	unsigned char* node = *reinterpret_cast<unsigned char**>(memory + 4);
	void* head = at<void*>(0x5a8);
	if(head == node)
		at<void*>(0x5a8) = *reinterpret_cast<void**>(node + 0x30);
	else
		{
		unsigned char* previous = static_cast<unsigned char*>(head);
		while(previous && *reinterpret_cast<void**>(previous + 0x30) != node)
			previous = *reinterpret_cast<unsigned char**>(previous + 0x30);
		if(previous)
			*reinterpret_cast<void**>(previous + 0x30) =
				*reinterpret_cast<void**>(node + 0x30);
		else
			NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
				"\\Epic\\Novodex\\SDKs\\Physics\\src\\Controller.cpp", 124, 0,
				"Scene::removeController: controller is not in the scene.");
		}
	*reinterpret_cast<void**>(node + 0x30) = 0;
	// Calling delete through the embedded proxy reproduces Controller.cpp's
	// scalar deleting-destructor dispatch and frees allocation+0 as the oracle.
	delete reinterpret_cast<NxControllerProxy*>(node);
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
//   0x00 broadPhase   0x04 gravity   0x10 userNotify   0x14 userTriggerReport
//   0x18 userContactReport   0x1c maxTimestep   0x20 maxIter   0x24 timeStepMethod
//   0x28 maxBounds   0x2c limits   0x30 groundPlane   0x31 boundsPlanes
//   0x32 collisionDetection   0x34 userData
//
// The descriptor-driven plane expansion calls phys_fn_000626
// (0x00011730, 3227 bytes) to create plane actors and phys_fn_000501
// (0x0000ff10, 393 bytes) to derive the six equations from maxBounds.
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

	// phys_fn_000544 (0x00010750, phase 7) maps the public broad-phase selector
	// and forwards the optional bounds to the pruning engine.
	nxSceneApplyDescriptorFlags(this, d[0], reinterpret_cast<const NxBounds3*>(d[0x0a]));

	// The default ground plane is a static actor containing one default plane
	// shape. The bounds-plane path below creates its six actors separately.
	if(reinterpret_cast<const unsigned char*>(&desc)[0x30] != 0)
		nxSceneBuildGroundPlane(this);

	// phys_fn_000501 converts maxBounds into the six inward-facing AABB plane
	// equations. The oracle's loop passes each equation to createActor.
	if(reinterpret_cast<const unsigned char*>(&desc)[0x31] != 0 && d[0x0a] != 0)
		nxSceneBuildBoundsPlanes(this,
			reinterpret_cast<const NxBounds3*>(d[0x0a]));

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


// phys_fn_000544 (0x00010750) and phys_fn_001973 (0x0004c1b0): map the public
// broad-phase enum to the pruning-engine selector and copy an optional scene
// bounds box into both engine bounds records. The engine selector uses 1 for
// quadratic, 2 for full and 3 for coherent; zero is the internal all-pairs
// path and is not a public NxBroadPhaseType.
void nxSceneApplyDescriptorFlags(NxSceneInternal* scene, NxU32 broadPhase,
	const NxBounds3* bounds)
	{
	unsigned char* const engine = scene->bytes() + 0x624;
	NxU32 engineMode;
	if(broadPhase == NX_BROADPHASE_QUADRATIC)
		engineMode = 1;
	else if(broadPhase == NX_BROADPHASE_FULL)
		engineMode = 2;
	else if(broadPhase == NX_BROADPHASE_COHERENT)
		engineMode = 3;
	else
		{
		nxReport(1, "\\Epic\\Novodex\\SDKs\\Physics\\src\\Scene.cpp", 0x63e, 0,
			"Scene::createBroadPhase: invalid broad phase type!");
		return;
		}
	*reinterpret_cast<NxU32*>(engine + 0x30) = engineMode;
	*reinterpret_cast<void**>(engine + 0x2c) = 0;
	if(bounds)
		{
		memcpy(engine + 0x04, bounds, sizeof(NxBounds3));
		memcpy(engine + 0x58, bounds, sizeof(NxBounds3));
		*reinterpret_cast<NxU32*>(engine + 0x70) = 1;
		}
	else
		{
		const NxU32 unbounded[6] = {
			0x7f7fffffu, 0x7f7fffffu, 0x7f7fffffu,
			0xff7fffffu, 0xff7fffffu, 0xff7fffffu };
		memcpy(engine + 0x58, unbounded, sizeof(unbounded));
		*reinterpret_cast<NxU32*>(engine + 0x70) = 2;
		}
	*reinterpret_cast<NxU32*>(engine + 0x74) = 0;
	}

// phys_fn_000626 (0x00011730) and phys_fn_000501 (0x0000ff10).
// Both descriptor-driven plane paths ultimately create static actors through
// Scene::createActor. The six bounded planes are the AABB faces, in X-, X+,
// Y-, Y+, Z-, Z+ order. NxPlaneShapeDesc uses n dot x = d.
void nxSceneBuildGroundPlane(void* scene)
	{
	NxPlaneShapeDesc plane;
	NxActorDesc actor;
	actor.shapes.pushBack(&plane);
	static_cast<NxSceneInternal*>(scene)->createActor(actor);
	}

void nxSceneBuildBoundsPlanes(void* scene, const NxBounds3* bounds)
	{
	if(!scene || !bounds)
		return;
	const NxReal* const b = reinterpret_cast<const NxReal*>(bounds);
	const NxVec3 normals[6] = {
		NxVec3(-1.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f),
		NxVec3(0.0f, -1.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f),
		NxVec3(0.0f, 0.0f, -1.0f), NxVec3(0.0f, 0.0f, 1.0f) };
	const NxReal distances[6] = { -b[3], b[0], -b[4], b[1], -b[5], b[2] };
	NxSceneInternal* const internalScene = static_cast<NxSceneInternal*>(scene);
	for(unsigned i = 0; i != 6; ++i)
		{
		NxPlaneShapeDesc plane;
		plane.normal = normals[i];
		plane.d = distances[i];
		NxActorDesc actor;
		actor.shapes.pushBack(&plane);
		internalScene->createActor(actor);
		}
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
	// phys_fn_000663 advances the Scene stamp then calls 001953 on the
	// pruning engine before it tears down actors. Every pair from the last
	// simulation step is stale now; unlink and free it before destroying roots.
	++scene->at<NxU32>(0x540);
	cpmRetireStaleScenePairs(scene);
	// phys_fn_000663 calls phys_fn_000913 immediately after stale-pair
	// retirement and before actor teardown. This delivers the final end-touch
	// event from each retained contact record while both actor wrappers live.
	if(NxUserContactReport* report = scene->at<NxUserContactReport*>(0x6b4))
		cpmFireContactReports0913(scene, report,
			reinterpret_cast<CpmPairHash*>(scene->bytes() + 0x2c));
	// phys_fn_000596 walks the actor range as it existed at entry. This is not
	// the public releaseActor path: it does not swap-remove actor pointers or
	// notify the optional fluid manager. Each body destructor removes its own
	// scene record and the separate body allocation is then freed.
	NxActor** actors = scene->at<NxActor**>(0x55c);
	NxActor** actorEnd = scene->at<NxActor**>(0x560);
	const unsigned actorCount = actors && actorEnd
		? static_cast<unsigned>(actorEnd - actors) : 0;
	for(unsigned index = 0; index < actorCount; ++index)
		{
		unsigned char* actor = reinterpret_cast<unsigned char*>(actors[index]);
		unsigned char* body = *reinterpret_cast<unsigned char**>(actor + 0x14);
		if(body)
			{
			nxActorDestroy(body);
			nxFoundationSDKAllocator->free(body);
			}
		}
	// The effectors (phys_fn_000575, which the oracle's Scene destructor
	// phys_fn_000663 calls at 0x13f90, after the actors and before the
	// joint lists). The actor loop above has already nulled every record
	// pointer an effector held (the 0x100 notify in releaseActor).
	scene->releaseEffectors();
	// phys_fn_002320 destroys the controller cache rooted at Scene+0x5a8.
	// Controllers remain attached when a caller releases a Scene directly, so
	// this list owns proxy allocations their actor teardown does not free. The
	// oracle clears the cache after effectors and before the joint lists.
	nxDestroyCachedList(self);
	// phys_fn_000604 walks the body-record range after actor and controller
	// teardown, resetting each record's island state before 000606 destroys the
	// remaining joint lists.
	nxSceneResetBodyRecords(scene);
	// The joints still registered (phys_fn_000606, the continuation of
	// phys_fn_000604 that the oracle's Scene destructor phys_fn_000663 calls at
	// 0x13f9e, after the actors): for each list, +0x59c then +0x5a0, the head
	// joint's link, mScene and flag bit 0 are cleared (so its ~Joint does not
	// call removeJoint), it is destroyed through slot 5 with 1, and the head
	// moves to the saved link (0x110f0-0x1118a). 000604's first loop (000760
	// over the +0x56c records) was executed above, before this joint-list loop.
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
	// phys_fn_000602 follows the joint-list pass at 0x13fa5. It destroys and
	// frees body records that remained in Scene+[0x56c, 0x570) after actor
	// teardown, including records without a remaining actor pose link.
	nxSceneDestroyBodyRecords(scene);
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
	nxSceneEngineDestroyPrunableOwners(static_cast<unsigned char*>(self) + 0x624);
	nxSceneEngineReleaseCoherent(static_cast<unsigned char*>(self) + 0x624);
	// The debug renderable phys_fn_000579 creates (0x14066-0x14092): released
	// through the Foundation's releaseDebugRenderable (slot +0x20, which takes
	// the field by reference), then the field is cleared. Scene-raycast block
	// Task 4 (visualisation); the rest of 000663 is not re-walked here.
	if(scene->at<NxDebugRenderable*>(0x6b8))
		{
		static_cast<NxFoundationSDK&>(NxFoundation::FoundationSDK::getInstance())
			.releaseDebugRenderable(scene->at<NxDebugRenderable*>(0x6b8));
		scene->at<NxDebugRenderable*>(0x6b8) = 0;
		}
	if(void* fluids = scene->at<void*>(0x61c))
		{
		nxSceneReleaseDisabledFluidManager(fluids);
		scene->at<void*>(0x61c) = 0;
		}
#if NX_PHYSICS_USE_X87
	for(unsigned offset = 8; offset <= 0xc; offset += 4)
		{
		void*& entries = *reinterpret_cast<void**>(
			static_cast<unsigned char*>(self) + offset);
		if(entries)
			{
			nxFoundationSDKAllocator->free(entries);
			entries = 0;
			}
		}
#else
	nxSceneScratchReleaseBuffers(*reinterpret_cast<NxPolygonScratch*>(scene->bytes()));
#endif
	if(p[0x12])
		{
		unsigned char* aux = reinterpret_cast<unsigned char*>(p[0x12]);
		for(int offset = 0x90; offset >= 0; offset -= 0x10)
			{
			void*& entries = *reinterpret_cast<void**>(aux + offset);
			if(entries)
				{
				nxFoundationSDKAllocator->free(entries);
				entries = 0;
				}
			}
		nxFoundationSDKAllocator->free(aux);
		}
	// The three ID arrays grow through the Foundation allocator; the pending
	// shape array at +0x6a4 (+0x624's +0x78) grows through phys_fn_004840,
	// which uses phys_fn_004803. Each block goes back to the allocator it
	// came from.
#if NX_PHYSICS_USE_X87
	const unsigned arrayOffsets[] = {0x6fc, 0x6e8, 0x6d4, 0x6a4};
	for(unsigned offset : arrayOffsets)
		{
		void*& entries = *reinterpret_cast<void**>(
			static_cast<unsigned char*>(self) + offset);
		if(entries)
			{
			if(offset == 0x6a4)
				nxGetSdkAllocator()->free(entries);
			else
				nxFoundationSDKAllocator->free(entries);
			entries = 0;
			}
		}
#else
	const unsigned otherIdArrayOffsets[] = {0x6fc, 0x6e8};
	for(unsigned offset : otherIdArrayOffsets)
		{
		void*& entries = *reinterpret_cast<void**>(static_cast<unsigned char*>(self) + offset);
		if(entries) nxFoundationSDKAllocator->free(entries);
		entries = 0;
		}
	nxSceneActorIdPoolDestroy(&nxSceneActorIds(*scene));
	void*& pendingShapes = scene->at<void*>(0x6a4);
	if(pendingShapes) nxGetSdkAllocator()->free(pendingShapes);
	pendingShapes = 0;
#endif
	// The pruning engine's pruners: each one's tree (static), its world boxes
	// and objects, then its storage (opcode/IcePruner.cpp).
	cpmDestroyScenePairStorage(scene);
	nxSceneEngineDestroyPruners(static_cast<unsigned char*>(self) + 0x624);
	const unsigned objectArrayOffsets[] = {0x56c, 0x55c};
	for(unsigned offset : objectArrayOffsets)
		{
		void*& entries = *reinterpret_cast<void**>(
			static_cast<unsigned char*>(self) + offset);
		if(entries)
			{
			nxFoundationSDKAllocator->free(entries);
			entries = 0;
			}
		}
	// Scalar owns actual member lifetimes in the original +450..+500 window.
#if !NX_PHYSICS_USE_X87
	nxSceneContactMembersDestroy(
		reinterpret_cast<NxSceneContactMembers*>(scene->bytes() + 0x450));
	reinterpret_cast<NxScenePrunerCollection*>(scene->bytes() + 0x640)->~NxScenePrunerCollection();
	nxSceneScratchDestroy(reinterpret_cast<NxPolygonScratch*>(scene->bytes()));
#endif
	if(flags & 1)
		nxFoundationSDKAllocator->free(self);
	}

// phys_fn_000604 (0x000110b0, 57 B): for each remaining body record in
// [Scene+0x56c, Scene+0x570), dispatch phys_fn_000760. The array may be empty
// after ordinary actor teardown; controller-owned and other retained records
// still take this path before the joint-list cleanup.
void nxSceneResetBodyRecords(NxSceneInternal* scene)
	{
	void** records = scene->at<void**>(0x56c);
	void** end = scene->at<void**>(0x570);
	if(!records || !end)
		return;
	for(; records != end; ++records)
		reinterpret_cast<Row000760Fixture*>(*records)->row000760();
	}

// phys_fn_000602 (0x00011060, 68 B): after the retained-joint lists, destroy
// every non-null dynamic body record left in the Scene range and release its
// storage through the Foundation allocator.
void nxSceneDestroyBodyRecords(NxSceneInternal* scene)
	{
	void** records = scene->at<void**>(0x56c);
	void** end = scene->at<void**>(0x570);
	if(!records || !end)
		return;
	for(; records != end; ++records)
		if(DynamicBody* body = static_cast<DynamicBody*>(*records))
			{
			body->destruct();
			nxFoundationSDKAllocator->free(body);
			}
	}

void NxSceneInternal::scalarDeletingDestructor(int flags)
	{
	nxSceneDelete(this, flags);
	}

// ---------------------------------------------------------------------------
// Supporting routines for Scene::createActor. Actor initialization and other
// listed callees remain partial; the notification helper below reproduces the
// unavailable FluidManager contract when its backend is disabled.
// ---------------------------------------------------------------------------

NxActor* nxSceneActorConstruct(void* memory, void* scene)
	{
	return reinterpret_cast<NxActor*>(nxSceneCreateActorBody(memory, scene));
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
#if NX_PHYSICS_USE_X87
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
	// 0x00010137-0x0001017e: the three embedded containers at +0x50, +0x500
	// (the scene queries' result collector) and +0x510 borrow the +0x0c buffer
	// (phys_fn_004847), and the pruning engine at +0x624 hands it to every
	// pruner through slot 4 (phys_fn_004861); scene-raycast block Task 3.
	NxU32* buffer = static_cast<NxU32*>(second);
	reinterpret_cast<SdkContainer*>(bytes + 0x50)->setExternalBuffer(capacity, buffer);
	reinterpret_cast<SdkContainer*>(bytes + 0x500)->setExternalBuffer(capacity, buffer);
	reinterpret_cast<SdkContainer*>(bytes + 0x510)->setExternalBuffer(capacity, buffer);
	nxSceneEngineSetExternalBuffer(bytes + 0x624, capacity, buffer);
#else
	unsigned char* bytes = static_cast<unsigned char*>(scene);
	nxSceneVisitedBuffersUpdate(*reinterpret_cast<NxPolygonScratch*>(bytes),
		*reinterpret_cast<SdkContainer*>(bytes + 0x50),
		*reinterpret_cast<SdkContainer*>(bytes + 0x500),
		*reinterpret_cast<SdkContainer*>(bytes + 0x510),
		*reinterpret_cast<NxScenePrunerCollection*>(bytes + 0x640), count);
#endif
	}

void nxSceneNotifyActorCreated(void* hook, NxActor* actor)
	{
	nxFluidManagerNotifyActorCreatedDisabled(hook, actor);
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

// The record constructor 000797 (with 000801, 000793/000795 and 000748) and
// its destructor 000776 (with 000799) are BodyCreation.cpp's DynamicBody
// members (scene-raycast block Task 4); 000026 and 000030 below call them.
// The NpActor.cpp completion's models of them (nxBodyRecordConstruct,
// nxBodyRecordApplyDesc, nxBodyRecordDestroy) were folded into those at the
// second merge of main into the scene-raycast block.

// phys_fn_000010 (0x00001390, 150 B)
// NxBodyDesc copy constructor: exact 0x78-byte copy, including the solver
// count at +0x74. Keep the boundary explicit because phys_fn_000026 calls it
// before it adjusts the copied mass properties.
struct Row000010Fixture
	{
	void row000010(const NxBodyDesc* source);
	};

__declspec(noinline) void Row000010Fixture::row000010(const NxBodyDesc* source)
	{
	memcpy(this, source, 0x78);
	}

// phys_fn_000026 (0x000019b0, 465 B)
// Actor.cpp's record build on the body (thiscall (desc), `ret 4`), shared by
// the creation path (000034) and setDynamic (000122). The descriptor is
// copied (000010); a tensor whose three words are all zero bits takes
// 000008 with the body's density (+0x18) into the copy's mass (in and out),
// mass pose and tensor, and its nonzero result is returned. The pose is the
// old record's (its +0x5c quaternion by the five-spill rotation, ROT, and
// its +0x50 position) when the body has one, else the body's own (+0x20).
// 0x260 bytes through [0x101041bc], 000797 on them (BodyCreation.cpp
// DynamicBody::construct), +8 = the record (0 when the allocation failed),
// 000630 on the Scene with it; 0. One recorded difference: the block is
// zeroed first. The image constructs over the allocator's bytes; the
// constructor chain writes every word but +0x10, +0x14, +0x23c and +0x240,
// which nothing before a simulation step reads.
int nxActorBuildRecord(unsigned char* body, const NxBodyDesc* desc)
	{
	NxBodyDesc local;
	reinterpret_cast<Row000010Fixture*>(&local)->row000010(desc);
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
		{
		memset(record, 0, 0x260);
		reinterpret_cast<DynamicBody*>(record)->construct(body,
			reinterpret_cast<const NxReal*>(&pose), &local);
		}
	*reinterpret_cast<unsigned char**>(body + 8) = record;
	nxSceneAddBody(*reinterpret_cast<NxSceneInternal**>(body + 4), record);
	return 0;
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
// and 000776 (BodyCreation.cpp DynamicBody::destruct) destroys it before
// [0x101041bc] frees it. The actor id (+0xc)
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
		reinterpret_cast<DynamicBody*>(record)->destruct();
		nxFoundationSDKAllocator->free(record);
		}
#if NX_PHYSICS_USE_X87
	nxU32VectorPushBack(scene->bytes() + 0x6d0, *reinterpret_cast<unsigned*>(body + 0xc));
#else
	nxSceneActorIds(*scene).returnId(*reinterpret_cast<unsigned*>(body + 0xc));
#endif
	unsigned char* root = *reinterpret_cast<unsigned char**>(body + 0x10);
	if(root)
		nxRuntimeShapeDeleteRoot(root);
	}

// A static actor's shape goes to the engine's static pruner (type 0), created
// on the first registration; its pool grows from four entries (0x60 bytes of
// boxes, 0x10 of object pointers) by doubling. These allocations occur in
// Actor::loadFromDescInternal, before Scene::createActor grows its public
// actor list. The root takes the pruning collection at +0xa0 (001943), and a
// static group's children are registered too, in section 0 (001943 adds them).
static void nxSceneStaticPrunerRegister(NxSceneInternal* scene, unsigned char* shape)
	{
	if(!scene || !shape) return;
	*reinterpret_cast<void**>(shape + 0xa0) = scene->bytes() + 0x624;
	nxSceneEngineAddRoot(scene, shape, 0);
	nxSceneTrackShape(scene, shape);
	unsigned char* pruner = scene->at<unsigned char*>(0x640);
	nxSceneUpdateActorCount(scene, pruner ? *reinterpret_cast<unsigned short*>(pruner + 0x10) : 0);
	}

// A static root and its current children leave the static pruner, the root
// its +0x78 list entry first.
static void nxSceneStaticPrunerUnregister(NxSceneInternal* scene, unsigned char* shape)
	{
	nxSceneUntrackShape(scene, shape);
	if(!shape) return;
	nxSceneEngineRemoveRoot(scene, shape);
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
			// The dynamic actor record is installed after the group's constructor.
			// Refresh through the group slot so its children and root both compose
			// against the live body pose before the root enters the broadphase.
			nxRuntimeShapeSlot6(shape, 1);
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
// file's model). The prunable is built in place by ObjectModel.cpp's
// nxShapeFactoryInstallPrunable (004874 with its vptr and the owner hooks,
// scene-raycast block Task 3); its 005297 member is not carried.
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
	nxShapeFactoryInstallPrunable(shape);
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
		if(type == NX_SHAPE_MESH)
			nxShapeFactoryConstructMeshHandle(handle, shape);
		else
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
	// ShapeBase::nxApplyDescriptor (phys_fn_001347) copies descriptor+0x40
	// into the public NxShape handle at +0x04. Keep this in the common load
	// path so every concrete family, including mesh's slot-12 loader, retains
	// the descriptor's userData.
	unsigned char* publicShape = *reinterpret_cast<unsigned char**>(shape + 0x9c);
	if(publicShape)
		*reinterpret_cast<void**>(publicShape + 4) = descriptor->userData;
	if(descriptor->getType() == NX_SHAPE_MESH)
		{
		void** table = *reinterpret_cast<void***>(shape);
		typedef bool (__thiscall* LoadFn)(void*, const void*);
		return reinterpret_cast<LoadFn>(table[12])(shape, descriptor);
		}
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
		// The capsule's own flags (desc +0x54) go to +0xe8, as the capsule
		// loader phys_fn_000989 stores them (0x00021af0) and its saveToDesc
		// (slot 13) reads them back; the core dump hands this word to its
		// trigger writer 004017 (effector-and-coredump Task 5).
		memcpy(shape + 0xe8, &capsule->flags, sizeof(NxU32));
		}
	else if(descriptor->getType() == NX_SHAPE_PLANE)
		{
		const NxPlaneShapeDesc* plane = static_cast<const NxPlaneShapeDesc*>(descriptor);
		nxShapeFactoryInitializePlane(shape, &plane->normal.x, plane->d);
		}
	else if(descriptor->getType() == NX_SHAPE_MESH)
		{
		const NxTriangleMeshShapeDesc* meshDesc =
			static_cast<const NxTriangleMeshShapeDesc*>(descriptor);
		if(!meshDesc->meshData) return false;
		void* wrapper = meshDesc->meshData;
		void* internalMesh = *reinterpret_cast<void**>(
			static_cast<unsigned char*>(wrapper) + 4);
		if(!internalMesh) return false;
		*reinterpret_cast<void**>(shape + 0xe0) = internalMesh;
		*reinterpret_cast<unsigned*>(shape + 0xe4) = meshDesc->meshFlags;
		++*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(internalMesh) + 0x74);
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
		NxSceneRemoveOwnerPairRecords(scene, shape);
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
	if(*reinterpret_cast<unsigned*>(shape + 0xd0) == NX_SHAPE_MESH)
		{
		void** table = *reinterpret_cast<void***>(shape);
		reinterpret_cast<NxRuntimeShapeDeleteFn>(table[0])(shape, 1);
		return;
		}
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
// back to the pool (000028, 0x1fe5). Type 4 uses its 0xe8-byte shape, final
// MESH table and slot-12 descriptor loader. Compound type 5 and types above
// it still take the no-shape arm.
static unsigned char* nxActorShapeFactory(const NxShapeDesc* descriptor, unsigned char* body)
	{
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	const unsigned id = nxIdAllocNext(scene->bytes() + 0x6e4);
	const unsigned type = static_cast<unsigned>(descriptor->getType());
	unsigned char* shape = 0;
	if(type <= NX_SHAPE_MESH)
		{
		static const unsigned sizes[5] = { 0x10c, 0xe4, 0x228, 0xec, 0xe8 };
		void* memory = nxFoundationSDKAllocator->malloc(sizes[type], NX_MEMORY_PERSISTENT);
		if(memory)
			{
			shape = nxRuntimeShapeConstruct(memory, sizes[type], type, body, id);
			// The MESH path uses its final-family slot 12, while the box's slot 12
			// is 000981 (ObjectModel.cpp nxBoxLoadFromDesc, through
			// nxShapeFactoryLoadBox, which also stores 000977's facade table at
			// +0xe0): the dims, the hull rebuild 000973 and BASE slot 1, its al the
			// load's result (0x10001efd-0x10001f02; scene-raycast block Task 4, box
			// hull). nxRuntimeShapeLoad then applies this file's model of the
			// remaining descriptor fields, the same words for a box.
			const bool boxLoaded = type != 2 || nxShapeFactoryLoadBox(shape, descriptor);
			if(!boxLoaded || !nxRuntimeShapeLoad(shape, descriptor) ||
				!*reinterpret_cast<void**>(shape + 0x9c))
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
				if(type == NX_SHAPE_MESH)
					{
					// Row 000032's case 4 increments Scene+0x10 and reserves
					// the scene's mesh-work buffers for the larger internal mesh count.
					++scene->at<unsigned>(0x10);
					const unsigned char* const mesh =
						*reinterpret_cast<unsigned char**>(shape + 0xe0);
					if(mesh)
						{
						const unsigned vertices = *reinterpret_cast<const unsigned*>(mesh + 8);
						const unsigned triangles = *reinterpret_cast<const unsigned*>(mesh + 0x0c);
						nxSceneUpdateActorCount(scene,
							vertices > triangles ? vertices : triangles);
						}
					}
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
static void __fastcall nxShapeGroupAggregateAABB(void* self, void*, float* out);

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

// Shape-group slot 9 is phys_fn_001030 (0x22bf0): aggregate the six-float
// bounds from every child. The oracle's table has this entry at slot 9;
// leaving it null crashes when a compound actor enters the scene update path.
static void __fastcall nxShapeGroupAggregateAABB(void* self, void*, float* out)
	{
	nxAggregateAABB1030(self, out);
	}

static void** nxShapeGroupTable()
	{
	static void* table[15] = {
		reinterpret_cast<void*>(&nxShapeGroupDeletingDtor), 0, 0, 0,
		reinterpret_cast<void*>(&nxShapeGroupAccumulateMass), 0,
		reinterpret_cast<void*>(&nxShapeGroupOwnerUpdate), 0, 0,
		reinterpret_cast<void*>(&nxShapeGroupAggregateAABB), 0, 0, 0, 0, 0 };
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

// phys_fn_000590 (0x00010dc0) validates the shape references before entering
// the pair hash. The hash path does not support a key whose two shape IDs are
// equal; the oracle reports the invalid parameter and leaves the table intact.
void NxSceneInternal::setShapePairFlags(void* shape0, void* shape1, NxU32 flags)
	{
	if(shape0 == shape1)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_SCENE_CPP, 0x388, 0,
			"Scene::setShapePairFlags: The two shape references must not reference the same shape.");
		return;
		}
	cpmSetShapePairFlags(this, shape0, shape1, flags);
	}

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
// The re-entry flag is gNxApiReentry (.data 0x10123c10); a re-entrant call is
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

// phys_fn_000577 (0x000109c0): dispatch each queued joint break before
// freeing its event object. The event callback can enqueue another event, so
// advance from the current Scene head after dispatch, as the oracle does.
void NxSceneInternal::processJointBreakEvents()
	{
	JointBreakEvent* event = at<JointBreakEvent*>(0x620);
	while(event)
		{
		event->row004113();
		at<JointBreakEvent*>(0x620) = event->mNext;
		nxFoundationSDKAllocator->free(event);
		event = at<JointBreakEvent*>(0x620);
		}
	at<JointBreakEvent*>(0x620) = 0;
	}

// phys_fn_000640 (0x00012a90): deliver trigger callbacks, drain joint-break
// events, then deliver buffered actor-contact callbacks. The end pointers are
// reset only when the corresponding user callback is installed, matching the
// oracle's guarded list-clearing behavior.
void NxSceneInternal::processSimulationCallbacks()
	{
	NxU8* const triggerBegin = at<NxU8*>(0x5fc);
	NxU8* const triggerEnd = at<NxU8*>(0x600);
	NxUserTriggerReport* const triggerReport = at<NxUserTriggerReport*>(0x6b0);
	const NxU32 triggerCount = triggerBegin && triggerEnd
		? (NxU32)(triggerEnd - triggerBegin) / 0x0c : 0;
	if(triggerReport && triggerCount)
		{
		for(NxU32 index = 0; index != triggerCount; ++index)
			{
			NxU8* const event = triggerBegin + index * 0x0c;
			NxShape* const trigger = *reinterpret_cast<NxShape**>(
				*reinterpret_cast<NxU8**>(event + 0x00) + 0x9c);
			NxShape* const other = *reinterpret_cast<NxShape**>(
				*reinterpret_cast<NxU8**>(event + 0x04) + 0x9c);
			const NxTriggerFlag flags = (NxTriggerFlag)*reinterpret_cast<NxU32*>(event + 0x08);
			triggerReport->onTrigger(*trigger, *other, flags);
			}
		at<NxU8*>(0x600) = triggerBegin;
		}

	processJointBreakEvents();

	NxU8* const contactBegin = at<NxU8*>(0x60c);
	NxU8* const contactEnd = at<NxU8*>(0x610);
	NxUserContactReport* const contactReport = at<NxUserContactReport*>(0x6b4);
	const NxU32 contactCount = contactBegin && contactEnd
		? (NxU32)(contactEnd - contactBegin) / 0x2c : 0;
	if(contactReport && contactCount)
		{
		for(NxU32 index = 0; index != contactCount; ++index)
			{
			NxU8* const record = contactBegin + index * 0x2c;
			NxContactPair& pair = *reinterpret_cast<NxContactPair*>(record);
			const NxU32 events = *reinterpret_cast<NxU32*>(record + 0x28);
			contactReport->onContactNotify(pair, events);
			}
		at<NxU8*>(0x610) = contactBegin;
		}
	}

// phys_fn_004113 (0x00098050): offer the break to the Scene user notify. A
// handled break releases the joint; otherwise detach it from its bodies and
// keep the broken joint on the Scene's no-body list.
void JointBreakEvent::row004113()
	{
	Joint* joint = mJoint;
	NxSceneInternal* scene = static_cast<NxSceneInternal*>(joint->mScene);
	NxUserNotify* notify = scene->at<NxUserNotify*>(0x6ac);
	if(notify)
		{
		gNxApiReentry = true;
		const bool handled = notify->onJointBreak(mUnknown00c,
			*static_cast<NxJoint*>(joint->mPublicObject));
		gNxApiReentry = false;
		if(handled)
			{
			scene->releaseJoint(joint);
			return;
			}
		}
	joint->handleBreakEvent();
	}

// phys_fn_000559 (0x00010860, 7 B, phase 7): the joint count at +0x6c8. It
// counts createJoint's calls that reach the type switch (every exit after it
// increments the count, 0x14529) less releaseJoint's, not the list.
NxU32 NxSceneInternal::getNbJoints() const
	{
	return at<NxU32>(0x6c8);
	}

// phys_fn_000617 (0x00011440). The image reuses one process-wide stats object,
// clearing it on every call and reporting only actor and static-shape counts.
NxSceneStats* NxSceneInternal::getSceneStats()
	{
	static NxSceneStats stats;
	stats.reset();
	const NxActor* const* actorBegin = at<NxActor**>(0x55c);
	const NxActor* const* actorEnd = at<NxActor**>(0x560);
	stats.numActors = actorBegin
		? static_cast<NxI32>(actorEnd - actorBegin)
		: 0;
	stats.numStaticShapes = static_cast<NxI32>(nxPruningCountFirst(bytes() + 0x624));
	return &stats;
	}

// phys_fn_000621 (0x000115b0). These fields are backing-array counts read by
// the oracle, including the pruner counts and the primary joint list.
void NxSceneInternal::getLimits(NxSceneLimits& limits) const
	{
	const void* const* actorBegin = at<void**>(0x55c);
	const void* const* actorEnd = at<void**>(0x560);
	const void* const* bodyBegin = at<void**>(0x56c);
	const void* const* bodyEnd = at<void**>(0x570);
	limits.maxNbActors = actorBegin ? static_cast<NxU32>(actorEnd - actorBegin) : 0;
	limits.maxNbBodies = bodyBegin ? static_cast<NxU32>(bodyEnd - bodyBegin) : 0;
	limits.maxNbStaticShapes = nxPruningCountFirst(bytes() + 0x624);
	limits.maxNbDynamicShapes = nxPruningCountIndexed(bytes() + 0x624);
	limits.maxNbJoints = 0;
	for(Joint* joint = at<Joint*>(0x59c); joint; joint = static_cast<Joint*>(joint->mNextJoint))
		++limits.maxNbJoints;
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

// ---------------------------------------------------------------------------
// Debug visualisation (scene-raycast block Task 4, visualisation sub-area;
// the chain is in SceneVisualize.h). Both rows run at API time under 0x027f.

// phys_fn_000579 (0x00010a10, 55 B)
// The Scene's debug renderable at +0x6b8: created on first use through the
// Foundation instance's createDebugRenderable (slot +0x1c of its
// NxFoundationSDK part at +0x14; getInstance's `int 3` when there is no
// instance is the listing's 0x10a1d-0x10a27), then returned. Out of line, as
// the image calls it (0x100139ff).
__declspec(noinline) NxDebugRenderable* NxSceneInternal::getDebugRenderable()
	{
	if(!at<NxDebugRenderable*>(0x6b8))
		at<NxDebugRenderable*>(0x6b8) =
			static_cast<NxFoundationSDK&>(NxFoundation::FoundationSDK::getInstance()).createDebugRenderable();
	return at<NxDebugRenderable*>(0x6b8);
	}

// The SDK parameter array (.data 0x10123b18, element 4 * index), read through
// PhysicsSDK::getParameter as the joint rows do.
static NxReal nxSceneVisParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// phys_fn_000657 (0x000139c0, 636 B)
// Scene::visualize, __thiscall, no arguments. Nothing when +0x70c bit 1 is
// set. Otherwise the renderable, if there is one, is cleared (slot +0x18);
// with NX_VISUALIZATION_SCALE 0.0f that is all (a NaN goes on). Then:
// - 000579 makes sure the renderable exists (its result is not used; every
//   later use re-reads +0x6b8);
// - 001978 on the pruning engine (+0x624);
// - NX_VISUALIZE_WORLD_AXES (0x13a16-0x13ae2): addBasis (slot +0x34) at the
//   origin, identity columns, lengths (1, 1, 1), scale the raw parameter,
//   colours 0xffff0000, 0xff00ff00, 0xff0000ff;
// - every actor on the array at +0x55c/+0x560 (the count taken once, as a
//   signed byte difference >> 2, compared unsigned): 000020 on its +0x14;
// - every joint on the list at +0x59c (link +0x10): slot +0x10;
// - every contact pair node (ContactPairManager.h's NxPairNode) on the list
//   at +0x674 (link +0x08) whose stamp (+0x104) equals the Scene's (+0x540):
//   000907;
// - NX_VISUALIZE_COLLISION_AABBS: 000638(engine, renderable, 0xffffff00, 0);
//   NX_VISUALIZE_COLLISION_COMPOUNDS: 000638(engine, renderable, 0xffff00ff,
//   1); any of NX_VISUALIZE_COLLISION_SHAPES, _AXES, _SPHERES: 000581;
// - the fluid manager at +0x61c, when there is one: 003639.
// Each parameter test is fucompp against 0.0f, so a NaN counts as set.
// 001978, 000638, 000581 and 003639 are unwritten placeholders
// (SceneVisualize.cpp).
void NxSceneInternal::visualize()
	{
	if(at<NxU32>(0x70c) & 2u)
		return;
	if(NxDebugRenderable* renderable = at<NxDebugRenderable*>(0x6b8))
		renderable->clear();
	if(nxSceneVisParameter(NX_VISUALIZATION_SCALE) == 0.0f)
		return;

	getDebugRenderable();
	nxSceneVisualizeCollisionPruners(&at<unsigned char>(0x624), at<NxDebugRenderable*>(0x6b8));

	if(nxSceneVisParameter(NX_VISUALIZE_WORLD_AXES) != 0.0f)
		{
		NxU32 colours[3] = { 0xffff0000u, 0xff00ff00u, 0xff0000ffu };
		NxVec3 lengths(1.0f, 1.0f, 1.0f);
		NxMat33 columns;
		columns.id();
		NxVec3 origin(0.0f, 0.0f, 0.0f);
		at<NxDebugRenderable*>(0x6b8)->addBasis(origin, columns, lengths,
			nxSceneVisParameter(NX_VISUALIZE_WORLD_AXES), colours);
		}

	NxActor** actors = at<NxActor**>(0x55c);
	const NxU32 actorCount = static_cast<NxU32>(static_cast<NxI32>(
		reinterpret_cast<char*>(at<NxActor**>(0x560)) - reinterpret_cast<char*>(actors)) >> 2);
	for(NxU32 i = 0; i < actorCount; i++)
		{
		NxDebugRenderable* renderable = at<NxDebugRenderable*>(0x6b8);
		(*reinterpret_cast<NxActorVisualRecord**>(reinterpret_cast<unsigned char*>(actors[i]) + 0x14))
			->visualize(*renderable);
		}

	for(Joint* joint = at<Joint*>(0x59c); joint; joint = static_cast<Joint*>(joint->mNextJoint))
		joint->row_slot4(*at<NxDebugRenderable*>(0x6b8));

	for(NxPairNode* node = at<NxPairNode*>(0x674); node; node = node->at<NxPairNode*>(0x08))
		if(node->at<NxU32>(0x104) == at<NxU32>(0x540))
			node->row000907(*at<NxDebugRenderable*>(0x6b8));

	if(nxSceneVisParameter(NX_VISUALIZE_COLLISION_AABBS) != 0.0f)
		nxSceneVisualizeCollisionBounds(&at<unsigned char>(0x624), at<NxDebugRenderable*>(0x6b8), 0xffffff00u, false);
	if(nxSceneVisParameter(NX_VISUALIZE_COLLISION_COMPOUNDS) != 0.0f)
		nxSceneVisualizeCollisionBounds(&at<unsigned char>(0x624), at<NxDebugRenderable*>(0x6b8), 0xffff00ffu, true);
	if(nxSceneVisParameter(NX_VISUALIZE_COLLISION_SHAPES) != 0.0f
		|| nxSceneVisParameter(NX_VISUALIZE_COLLISION_AXES) != 0.0f
		|| nxSceneVisParameter(NX_VISUALIZE_COLLISION_SPHERES) != 0.0f)
		nxSceneVisualizeCollisionShapes(&at<unsigned char>(0x624), at<NxDebugRenderable*>(0x6b8));
	if(void* fluids = at<void*>(0x61c))
		nxSceneVisualizeFluids(fluids, at<NxDebugRenderable*>(0x6b8));
	}

// ---------------------------------------------------------------------------
// The effector rows (effector-and-coredump Task 2,
// units/effector-coredump-contract.md "### Scene and NpScene rows").

// phys_fn_000587 (0x00010c90, 187 B, phase 7) is
// Scene::createSpringAndDamperEffector. Allocates (0x68, 0) through the
// Foundation allocator's slot +8 and constructs (003960); a constructed
// effector is pushed at the head of the +0x5a4 list. The count +0x6c4 is
// incremented and the cursor +0x6c0 reset to the head on every path, the
// failed allocation included (0x10ced jumps back to 0x10cc5), and the
// setters then run on the result, null or not, as in the listing: bodies
// from each desc actor's +0x14 (0 for a null actor) with desc.pos1/pos2
// (003962), the spring words desc+0x20..+0x30 (003966), the damper words
// desc+0x34..+0x40 (003968). The desc's isValid() is not called.
SpringAndDamperEffector* NxSceneInternal::createSpringAndDamperEffector(const NxSpringAndDamperEffectorDesc& desc)
	{
	void* memory = nxFoundationSDKAllocator->malloc(0x68, NX_MEMORY_PERSISTENT);
	SpringAndDamperEffector* effector = 0;
	if(memory)
		{
		effector = new(memory) SpringAndDamperEffector(this);
		if(effector)
			{
			effector->mNext = at<Effector*>(0x5a4);
			at<Effector*>(0x5a4) = effector;
			}
		}
	++at<NxU32>(0x6c4);
	at<void*>(0x6c0) = at<void*>(0x5a4);
	void* body1 = desc.body1 ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(desc.body1) + 0x14) : 0;
	void* body2 = desc.body2 ? *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(desc.body2) + 0x14) : 0;
	effector->setBodies(body1, desc.pos1, body2, desc.pos2);
	effector->setLinearSpring(desc.springDistCompressSaturate, desc.springDistRelaxed,
		desc.springDistStretchSaturate, desc.springMaxCompressForce, desc.springMaxStretchForce);
	effector->setLinearDamper(desc.damperVelCompressSaturate, desc.damperVelStretchSaturate,
		desc.damperMaxCompressForce, desc.damperMaxStretchForce);
	return effector;
	}

// phys_fn_000573 (0x00010900, 109 B, phase 7) is Scene::removeEffector.
// Unlinks the effector from the +0x5a4 list (head or successor) and clears
// its link; an effector not in the list is reported (code 2, line 0x84c)
// and left as it is. noinline: the oracle calls it as its own function
// (000594, 0x10ecb), so a breakpoint on the row sees it run.
__declspec(noinline) void NxSceneInternal::removeEffector(Effector* effector)
	{
	Effector* head = at<Effector*>(0x5a4);
	if(effector == head)
		{
		at<Effector*>(0x5a4) = effector->mNext;
		effector->mNext = 0;
		return;
		}
	for(Effector* link = head; link; link = link->mNext)
		{
		if(link->mNext == effector)
			{
			link->mNext = effector->mNext;
			effector->mNext = 0;
			return;
			}
		}
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_SCENE_CPP, 0x84c, 0,
		"Scene::removeEffector: effector is not in the scene.");
	}

// phys_fn_000575 (0x00010970, 68 B, phase 7): every effector in the +0x5a4
// list, head first: the head's link is saved and cleared, the head is
// destroyed through slot 1 with 1 and the head word zeroed, then the head
// becomes the saved link. The count and cursor are not touched. noinline:
// the Scene destructor 000663 calls it as a function (0x13f90).
__declspec(noinline) void NxSceneInternal::releaseEffectors()
	{
	while(at<Effector*>(0x5a4))
		{
		Effector* next = at<Effector*>(0x5a4)->mNext;
		at<Effector*>(0x5a4)->mNext = 0;
		if(at<Effector*>(0x5a4))
			{
			delete at<Effector*>(0x5a4);
			at<Effector*>(0x5a4) = 0;
			}
		at<Effector*>(0x5a4) = next;
		}
	}

// phys_fn_000594 (0x00010e80, 126 B, phase 7) is Scene::releaseEffector.
// The re-entry flag is gNxApiReentry (.data 0x10123c10); a re-entrant call is
// reported with the message the pointer at .data 0x00122050 names (code 2,
// line 0x4ec). Otherwise: removeEffector, the effector's scalar deleting
// destructor (slot 1 with 1) when the pointer is non-null, --[+0x6c4] and
// the cursor reset to the list head, then the flag cleared.
void NxSceneInternal::releaseEffector(Effector* effector)
	{
	if(gNxApiReentry)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_SCENE_CPP, 0x4ec, 0,
			"Reentry check: You may not call this API method from a callback!");
		return;
		}
	gNxApiReentry = true;
	removeEffector(effector);
	if(effector)
		delete effector;
	--at<NxU32>(0x6c4);
	at<void*>(0x6c0) = at<void*>(0x5a4);
	gNxApiReentry = false;
	}

// phys_fn_000509 (0x00010200, 33 B, phase 2): the three gravity words at
// +0x520..+0x528 copied out as words (`ret 4`). The core dump's `PsGravity`
// line.
void NxSceneInternal::getGravity(NxVec3& gravity) const
	{
	NxU32* out = reinterpret_cast<NxU32*>(&gravity.x);
	out[0] = at<NxU32>(0x520);
	out[1] = at<NxU32>(0x524);
	out[2] = at<NxU32>(0x528);
	}

// phys_fn_000507 (0x000101d0, 33 B): stores the three gravity words at
// Scene+0x520, +0x524 and +0x528 in order.
void NxSceneInternal::setGravity(const NxVec3& gravity)
	{
	const NxU32* in = reinterpret_cast<const NxU32*>(&gravity.x);
	at<NxU32>(0x520) = in[0];
	at<NxU32>(0x524) = in[1];
	at<NxU32>(0x528) = in[2];
	}

// phys_fn_000540/000542 (0x000106f0/0x00010720): copy the timing triplet
// between the caller's values and Scene+0x52c/+0x530/+0x534.
void NxSceneInternal::setTiming(NxReal maxTimestep, NxU32 maxIter, NxU32 method)
	{
	at<NxReal>(0x52c) = maxTimestep;
	at<NxU32>(0x530) = maxIter;
	at<NxU32>(0x534) = method;
	}

void NxSceneInternal::getTiming(NxReal& maxTimestep, NxU32& maxIter, NxU32& method) const
	{
	maxTimestep = at<NxReal>(0x52c);
	maxIter = at<NxU32>(0x530);
	method = at<NxU32>(0x534);
	}

namespace
	{
struct NxSceneTriggerPairs
	{
	NxCollisionShape** begin;
	NxCollisionShape** end;
	NxCollisionShape** capacity;
	};

struct NxSceneTriggerEvent
	{
	NxShape* trigger;
	NxShape* other;
	NxU32 event;
	};

static void nxSceneAppendTriggerEvent(NxSceneInternal* scene, NxShape* trigger,
	NxShape* other, NxU32 event)
	{
	NxU8* bytes = scene->bytes();
	NxSceneTriggerEvent*& begin = *reinterpret_cast<NxSceneTriggerEvent**>(bytes + 0x5fc);
	NxSceneTriggerEvent*& end = *reinterpret_cast<NxSceneTriggerEvent**>(bytes + 0x600);
	NxSceneTriggerEvent*& capacity = *reinterpret_cast<NxSceneTriggerEvent**>(bytes + 0x604);
	if(capacity <= end)
		{
		const NxU32 count = begin ? static_cast<NxU32>(end - begin) : 0;
		const NxU32 held = begin ? static_cast<NxU32>(capacity - begin) : 0;
		const NxU32 wanted = count * 2 + 2;
		if(held < wanted)
			{
			NxSceneTriggerEvent* grown = static_cast<NxSceneTriggerEvent*>(
				nxFoundationSDKAllocator->malloc(wanted * sizeof(*grown), NX_MEMORY_PERSISTENT));
			for(NxU32 index = 0; index < count; ++index)
				grown[index] = begin[index];
			if(begin)
				nxFoundationSDKAllocator->free(begin);
			begin = grown;
			end = grown + count;
			capacity = grown + wanted;
			}
		}
	end->trigger = trigger;
	end->other = other;
	end->event = event;
	++end;
	}

struct NxSceneTriggerPairIndex
	{
	NxSceneTriggerPairs* pairs;
	NxI32* buckets;
	NxI32* next;
	NxU8* matched;
	NxU32 bucketMask;
	NxU32 count;
	};

static NxU32 nxSceneTriggerArithmeticShiftRight(NxU32 value, NxU32 shift)
	{
	return (value >> shift) |
		((value & 0x80000000u) ? (~NxU32(0) << (32 - shift)) : 0);
	}

static NxU32 nxSceneTriggerPairHash(const NxCollisionShape* first,
	const NxCollisionShape* second)
	{
	const NxU32 firstId = *reinterpret_cast<const NxU32*>(
		reinterpret_cast<const NxU8*>(first) + 0xd4) & 0xffffu;
	const NxU32 secondId = *reinterpret_cast<const NxU32*>(
		reinterpret_cast<const NxU8*>(second) + 0xd4);
	const NxU32 key = firstId | (secondId << 16);
	const NxU32 firstMix = ~(key << 15) + key;
	const NxU32 salted = 9u * (firstMix ^ nxSceneTriggerArithmeticShiftRight(firstMix, 10));
	const NxU32 folded = nxSceneTriggerArithmeticShiftRight(salted, 6) ^ salted;
	const NxU32 secondMix = ~(folded << 11) + folded;
	return secondMix ^ nxSceneTriggerArithmeticShiftRight(secondMix, 16);
	}

static NxSceneTriggerPairIndex nxSceneBuildTriggerPairIndex(
	NxSceneTriggerPairs* pairs, NxI32* buckets, NxI32* next, NxU8* matched,
	NxU32 bucketCount)
	{
	NxSceneTriggerPairIndex index;
	index.pairs = pairs;
	index.count = pairs->begin
		? static_cast<NxU32>((pairs->end - pairs->begin) / 2) : 0;
	index.bucketMask = bucketCount - 1;
	index.buckets = buckets;
	index.next = next;
	index.matched = matched;
	memset(index.buckets, 0xff, bucketCount * sizeof(*index.buckets));
	memset(index.matched, 0, index.count ? index.count : 1);
	for(NxU32 pair = 0; pair < index.count; ++pair)
		{
		NxCollisionShape* first = pairs->begin[pair * 2];
		NxCollisionShape* second = pairs->begin[pair * 2 + 1];
		const NxU32 bucket = nxSceneTriggerPairHash(first, second) & index.bucketMask;
		index.next[pair] = index.buckets[bucket];
		index.buckets[bucket] = static_cast<NxI32>(pair);
		}
	return index;
	}

static NxI32 nxSceneFindTriggerPair(const NxSceneTriggerPairIndex& index,
	const NxCollisionShape* first, const NxCollisionShape* second)
	{
	const NxU32 bucket = nxSceneTriggerPairHash(first, second) & index.bucketMask;
	for(NxI32 pair = index.buckets[bucket]; pair >= 0; pair = index.next[pair])
		if(index.pairs->begin[pair * 2] == first &&
			index.pairs->begin[pair * 2 + 1] == second)
			return pair;
	return -1;
	}

static bool nxSceneTriggerPairStillOverlaps(const NxCollisionShape* first,
	const NxCollisionShape* second)
	{
	if(first->type > second->type)
		{
		const NxCollisionShape* swap = first;
		first = second;
		second = swap;
		}
	NxShapeOverlapFn* const overlaps = reinterpret_cast<NxShapeOverlapFn*>(
		static_cast<NxU8*>(NxGetCollisionDispatchMatrix()) + 0x94);
	NxShapeOverlapFn const overlap = overlaps[NxCollisionPairIndex(first->type, second->type)];
	return overlap && overlap(first, second);
	}

static void nxSceneAppendTriggerPair(NxSceneTriggerPairs* pairs,
	NxCollisionShape* first, NxCollisionShape* second)
	{
	if(pairs->capacity <= pairs->end)
		{
		const NxU32 count = pairs->begin
			? static_cast<NxU32>((pairs->end - pairs->begin) / 2) : 0;
		const NxU32 held = pairs->begin
			? static_cast<NxU32>((pairs->capacity - pairs->begin) / 2) : 0;
		const NxU32 wanted = count * 2 + 2;
		if(held < wanted)
			{
		NxCollisionShape** grown = static_cast<NxCollisionShape**>(
			nxFoundationSDKAllocator->malloc(wanted * 2 * sizeof(*grown),
				NX_MEMORY_PERSISTENT));
		for(NxU32 index = 0; index < count * 2; ++index)
			grown[index] = pairs->begin[index];
		if(pairs->begin)
			nxFoundationSDKAllocator->free(pairs->begin);
		pairs->begin = grown;
		pairs->end = grown + count * 2;
		pairs->capacity = grown + wanted * 2;
		}
		}
	*pairs->end++ = first;
	*pairs->end++ = second;
	}

static void nxSceneReportTriggerTransition(NxSceneInternal* scene,
	NxCollisionShape* first, NxCollisionShape* second, NxU32 event)
	{
	NxCollisionShape* trigger = (*(reinterpret_cast<NxU8*>(first) + 0xde) & 7) ? first : second;
	NxCollisionShape* other = trigger == first ? second : first;
	const NxU8 flags = *(reinterpret_cast<NxU8*>(trigger) + 0xde);
	// phys_fn_002350 and its callback drain only test that at least one
	// trigger event bit is enabled; the pinned build reports every transition.
	if((flags & 7u) != 0)
		nxSceneAppendTriggerEvent(scene, reinterpret_cast<NxShape*>(trigger),
			reinterpret_cast<NxShape*>(other), event);
	}
	}

void nxSceneProcessTriggerPairs(NxSceneInternal* scene)
	{
	NxU8* const bytes = scene->bytes();
	NxSceneTriggerPairs* const previous = *reinterpret_cast<NxSceneTriggerPairs**>(bytes + 0x5d4);
	NxSceneTriggerPairs* const current = *reinterpret_cast<NxSceneTriggerPairs**>(bytes + 0x5d8);
	if(!previous || !current)
		return;
	const NxU32 previousCount = previous->begin
		? static_cast<NxU32>((previous->end - previous->begin) / 2) : 0;
	NxU32 bucketCount = 1;
	while(bucketCount <= previousCount)
		bucketCount <<= 1;
	const NxU32 scratchCount = previousCount ? previousCount : 1;
	NxI32* const buckets = static_cast<NxI32*>(
		_alloca(bucketCount * sizeof(*buckets)));
	NxI32* const next = static_cast<NxI32*>(
		_alloca(scratchCount * sizeof(*next)));
	NxU8* const matched = static_cast<NxU8*>(_alloca(scratchCount));
	NxSceneTriggerPairIndex previousIndex = nxSceneBuildTriggerPairIndex(
		previous, buckets, next, matched, bucketCount);
	for(NxCollisionShape** item = current->begin; item && item != current->end; item += 2)
		{
		const NxI32 previousPair = nxSceneFindTriggerPair(previousIndex, item[0], item[1]);
		if(previousPair >= 0)
			previousIndex.matched[previousPair] = 1;
		nxSceneReportTriggerTransition(scene, item[0], item[1],
			previousPair >= 0 ? 4u : 1u);
		}
	for(NxU32 pair = 0; pair < previousIndex.count; ++pair)
		if(!previousIndex.matched[pair])
			{
			NxCollisionShape* const first = previous->begin[pair * 2];
			NxCollisionShape* const second = previous->begin[pair * 2 + 1];
			if(nxSceneTriggerPairStillOverlaps(first, second))
				{
				nxSceneAppendTriggerPair(current, first, second);
				nxSceneReportTriggerTransition(scene, first, second, 4u);
				}
			else
				nxSceneReportTriggerTransition(scene, first, second, 2u);
			}

	// phys_fn_002350 consumes the second pair list, then swaps the two embedded
	// list headers and resets the new current list for the next substep.
	NxSceneTriggerPairs* const oldPrevious = previous;
	*reinterpret_cast<NxSceneTriggerPairs**>(bytes + 0x5d4) = current;
	*reinterpret_cast<NxSceneTriggerPairs**>(bytes + 0x5d8) = oldPrevious;
	oldPrevious->end = oldPrevious->begin;
	}

// phys_fn_000610 (0x00011210): integrate the active sleep groups. The original
// thiscall receives only Scene in ecx; the scene carries dt/invDt at +0x548/+0x54c.
void NxSceneInternal::row000610()
	{
	void** roots = at<void**>(0x57c);
	void** rootsEnd = at<void**>(0x580);
	const NxReal timestep = at<NxReal>(0x548);
	const NxReal inverseTimestep = at<NxReal>(0x54c);
	for(void** root = roots; root && root != rootsEnd; ++root)
		for(unsigned char* body = static_cast<unsigned char*>(*root); body;
			body = *reinterpret_cast<unsigned char**>(body + 0x1fc))
			reinterpret_cast<Row000726Fixture*>(body)->row000726(timestep, inverseTimestep);
	}

// phys_fn_000611 (0x00011260): prepare and solve each active island. Its final
// 74-byte continuation starts at 000613/0x11370 and copies solved records back.
void NxSceneInternal::row000611()
	{
	void** roots = at<void**>(0x57c);
	void** rootsEnd = at<void**>(0x580);
	const NxReal timestep = at<NxReal>(0x548);
	const NxReal inverseTimestep = at<NxReal>(0x54c);
	at<NxU32>(0x70c) |= 4u;
	for(void** root = roots; root && root != rootsEnd; ++root)
		{
		unsigned char* island = static_cast<unsigned char*>(*root);
		if(*reinterpret_cast<NxU32*>(island + 0x1f0) == 0)
			continue;
		const NxU32 bodyCount = *reinterpret_cast<NxU32*>(island + 0x1f4);
		nxSceneEnsureStepBodies(this, bodyCount);
		JointSupportBody* records = at<JointSupportBody*>(0x5ac);
		for(unsigned char* body = island; body;
			body = *reinterpret_cast<unsigned char**>(body + 0x1fc))
			{
			JointSupportBody* record = records++;
			memcpy(&record->mUnknown000, body + 0x34, sizeof(NxVec3));
			memcpy(&record->mUnknown00c, body + 0xc0, sizeof(NxReal));
			memcpy(&record->mUnknown010, body + 0x40, sizeof(NxVec3));
			record->mUnknown01c = body;
			memcpy(record->mUnknown020, body + 0x164, sizeof(record->mUnknown020));
			memcpy(&record->mUnknown05c, body + 0x110, sizeof(NxU32));
			*reinterpret_cast<JointSupportBody**>(body + 0x204) = record;
			if(nxSceneMaximumStepBodies < record->mUnknown05c)
				nxSceneMaximumStepBodies = record->mUnknown05c;
			}
		reinterpret_cast<Row000730Fixture*>(island)->row000730(timestep, inverseTimestep);
		if(at<NxU32>(0x5bc))
			{
			cpmSolveSceneContactRecords(this, nxSceneMaximumStepBodies);
			nxSolveJointSupportRecords(this, timestep, nxSceneMaximumStepBodies, false);
			nxSceneMaximumStepBodies = 0;
			}
		for(unsigned char* body = island; body;
			body = *reinterpret_cast<unsigned char**>(body + 0x1fc))
			reinterpret_cast<Row000708Fixture*>(body)->row000708();
		at<NxU32>(0x5bc) = 0;
		}
	at<NxU32>(0x70c) &= ~4u;
	}

// phys_fn_000636 (0x00012890): refresh each body's post-step velocity record,
// then reset the active-root range for the next substep.
void NxSceneInternal::row000636()
	{
	void** bodies = at<void**>(0x56c);
	void** bodiesEnd = at<void**>(0x570);
	const NxReal timestep = at<NxReal>(0x548);
	for(void** item = bodies; item && item != bodiesEnd; ++item)
		reinterpret_cast<Row000732Fixture*>(*item)->row000732(timestep, 0.0f);
	at<void**>(0x580) = at<void**>(0x57c);
	}

// phys_fn_000615 (0x000113c0): advance every body pose and notify its actor,
// then project each root node in the body's articulation island at +0x1e0.
void NxSceneInternal::row000615()
	{
	void** bodies = at<void**>(0x56c);
	void** bodiesEnd = at<void**>(0x570);
	const NxReal timestep = at<NxReal>(0x548);
	for(void** item = bodies; item && item != bodiesEnd; ++item)
		{
		unsigned char* body = static_cast<unsigned char*>(*item);
		reinterpret_cast<Row000770Fixture*>(body)->row000770(timestep, 0.0f);
		reinterpret_cast<Row000022Fixture*>(
			*reinterpret_cast<void**>(body + 0x19c))->row000022(1);
		}
	for(void** item = bodies; item && item != bodiesEnd; ++item)
		{
		unsigned char* body = static_cast<unsigned char*>(*item);
		void* articulationIsland = *reinterpret_cast<void**>(body + 0x1e0);
		if(articulationIsland)
			nxBodyIslandProject004165(articulationIsland);
		}
	}

// phys_fn_000659 (0x00013c40): select fixed or variable stepping under the
// oracle's x87 precision-64/round-toward-zero mode, run each requested body
// substep, then restore the caller's control word. The scheduler fields are
// Scene+0x52c..+0x558 as measured in the listing.
void NxSceneInternal::simulateFrame()
	{
	unsigned short savedControlWord = 0;
	__asm fnstcw savedControlWord
	NxSetFPURoundingChop();
	NxSetFPUPrecision64();

	const NxReal elapsedTime = at<NxReal>(0x544);
	NxReal timestep = 0.0f;
	NxReal inverseTimestep = 0.0f;
	NxU32 iterations = 0;
	if(at<NxU32>(0x534) == 1)
		{
		timestep = elapsedTime;
		inverseTimestep = 1.0f / timestep;
		at<NxU32>(0x550) = 1;
		at<NxReal>(0x554) = 1.0f;
		at<NxU32>(0x558) = 0;
		at<NxReal>(0x548) = timestep;
		at<NxReal>(0x54c) = inverseTimestep;
		at<NxReal>(0x53c) += timestep;
		iterations = 1;
		}
	else
		{
		timestep = at<NxReal>(0x52c);
		// Preserve the oracle's exact x87 order. It stores the rounded sum to
		// +0x538 while retaining the extended value on the x87 stack, then
		// computes/stores the rounded inverse and multiplies the extended
		// reciprocal by that still-extended sum before FISTP.
		NxReal* accumulatedAddress = &at<NxReal>(0x538);
		NxReal* inverseAddress = &at<NxReal>(0x54c);
		unsigned __int64 rawIterations = 0;
		__asm
			{
			mov eax, accumulatedAddress
			mov edx, inverseAddress
			fld elapsedTime
			fadd dword ptr [eax]
			fst dword ptr [eax]
			fld1
			fdiv dword ptr [timestep]
			fst dword ptr [edx]
			fmul st(0), st(1)
			fistp qword ptr [rawIterations]
			fstp st(0)
			}
		inverseTimestep = *inverseAddress;
		iterations = static_cast<NxU32>(rawIterations);
		if(iterations > at<NxU32>(0x530))
			iterations = at<NxU32>(0x530);
		at<NxU32>(0x550) = iterations;
		at<NxReal>(0x554) = iterations ? 1.0f / static_cast<NxReal>(iterations) : 0.0f;
		at<NxU32>(0x558) = 0;
		at<NxReal>(0x548) = timestep;
		at<NxReal>(0x54c) = inverseTimestep;
		}

	for(NxU32 iteration = 0; iteration < iterations; ++iteration)
		{
		// The contact-patch cache stamps each pair against this per-substep
		// counter. Advance it for both variable and fixed timestep methods.
		++at<NxU32>(0x540);
		// phys_fn_000608 -> 001976 refreshes broadphase pairs and their contact
		// streams before the active islands build their solver rows.
		nxSceneRefreshPairs(this);

		// phys_fn_000635 (0x127e0): refresh dirty joint islands, then reset
		// every body's sleep-group links before collision pairs are rebuilt.
		Joint** joints = at<Joint**>(0x58c);
		Joint** jointsEnd = at<Joint**>(0x590);
		for(Joint** item = joints; item && item != jointsEnd; ++item)
			{
				void* joint = *item;
				void* body = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + 8);
				if(!body)
					body = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + 0xc);
				if(body)
					reinterpret_cast<Row000762Fixture*>(body)->row000762(joint);
			}
		for(Joint** item = joints; item && item != jointsEnd; ++item)
			{
				void* joint = *item;
				void* body = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + 8);
				if(!body)
					body = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + 0xc);
				reinterpret_cast<Row000720Fixture*>(body)->row000720();
			}
		at<Joint**>(0x590) = joints;
		void** bodies = at<void**>(0x56c);
		void** bodiesEnd = at<void**>(0x570);
		for(void** item = bodies; item && item != bodiesEnd; ++item)
			reinterpret_cast<Row000764Fixture*>(*item)->row000764();
		// 001976's dirty-prunable prepass reaches 001949/001303 before the
		// pair-list refresh. The public shape slot 6 refreshes its world pose and
		// advances ShapeBase+0x08 to this Scene frame; 000905 uses that stamp to
		// detect the changed pair geometry and dispatch narrow phase.
		void** trackedShapes = at<void**>(0x6a4);
		const NxU32 trackedShapeCount = at<NxU32>(0x6a0);
		for(NxU32 i = 0; trackedShapes && i < trackedShapeCount; ++i)
			nxRuntimeShapeSlot6(static_cast<unsigned char*>(trackedShapes[i]), 1);

		// phys_fn_000608's post-broadphase auxiliary-pair walk calls 000724 for
		// each live pair with contacts. Rebuild those per-body links after 000764
		// clears them and before active roots are collected for 000611.
		NxPairList* contactPairs = reinterpret_cast<NxPairList*>(mBytes + 0x674);
		for(NxPairNode* node = contactPairs->head; node;
			node = node->at<NxPairNode*>(8))
			{
			NxActorPair* pair = node->pair();
			if(!pair->at<NxU32>(0x10))
				continue;
			void* body0 = pair->at<void*>(8);
			void* body1 = pair->at<void*>(0xc);
			if(body0)
				reinterpret_cast<Row000724Fixture*>(body0)->row000724(body1, node);
			else if(body1)
				reinterpret_cast<Row000724Fixture*>(body1)->row000724(0, node);
			}

		// 000655's active-island collection follows 000608. Keep only the
		// self-parented, awake sleep-group roots in Scene+0x57c..+0x580.
		void**& rootFirst = at<void**>(0x57c);
		void**& rootLast = at<void**>(0x580);
		void**& rootEnd = at<void**>(0x584);
		for(void** item = bodies; item && item != bodiesEnd; ++item)
			{
				void* body = *item;
				void* root = reinterpret_cast<Row000713Fixture*>(body)->row000713();
				if(root != body || *reinterpret_cast<NxReal*>(
					static_cast<NxU8*>(root) + 0x1f8) == 0.0f)
					continue;
				if(rootEnd <= rootLast)
					{
					const NxU32 count = rootFirst
						? static_cast<NxU32>(rootLast - rootFirst) : 0;
					const NxU32 capacity = count * 2 + 2;
					const NxU32 held = rootFirst
						? static_cast<NxU32>(rootEnd - rootFirst) : 0;
					if(held < capacity)
						{
						void** grown = static_cast<void**>(nxFoundationSDKAllocator->malloc(
							capacity * sizeof(void*), NX_MEMORY_PERSISTENT));
						for(NxU32 i = 0; i < count; ++i)
							grown[i] = rootFirst[i];
						if(rootFirst)
							nxFoundationSDKAllocator->free(rootFirst);
						rootFirst = grown;
						rootLast = grown + count;
						rootEnd = grown + capacity;
						}
					}
				*rootLast++ = body;
			}

		// phys_fn_000655 applies effectors after active roots are collected but
		// before phys_fn_000610 integrates their bodies. The spring impulse must
		// enter angular velocity before 000726 applies angular damping.
		for(Effector* effector = at<Effector*>(0x5a4); effector;
			effector = effector->mNext)
			effector->tick();

		// phys_fn_000610 walks active island roots (+0x57c) and each root's
		// sleep-group chain (+0x1fc); inactive bodies must not be integrated.
		row000610();
		row000611();

		// 000636 performs post-step velocity bookkeeping and clears the active
		// root range before 000615 advances each body's COM/quaternion.
		row000636();

		// 000615 advances poses, notifies actors, and projects articulation
		// islands after all of the active roots have been retired.
		row000615();
		// phys_fn_000655 calls 000917 once per completed substep. 000905 has
		// stamped active contact-report records during the broadphase refresh;
		// buffer their event flags and solved force totals for fetchResults.
		nxSceneProcessTriggerPairs(this);
		if(at<NxUserContactReport*>(0x6b4))
			{
			CpmPairHash* const reportHash = reinterpret_cast<CpmPairHash*>(mBytes + 0x2c);
			cpmBufferContactReports0917(this, reportHash);
			}
		if(void* fluidManager = at<void*>(0x61c))
			nxFluidManagerStepDisabled(fluidManager, timestep);
		++at<NxU32>(0x558);
		at<NxReal>(0x538) -= timestep;
		}
	if(at<NxU32>(0x534) != 1 && timestep < at<NxReal>(0x538))
		at<NxReal>(0x538) = timestep;
	if(void* fluidManager = at<void*>(0x61c))
		nxFluidManagerGenerateSurfaceMeshesDisabled(fluidManager);
	__asm fldcw savedControlWord
	}

// phys_fn_000619 (0x000114b0): refresh each body's gravity and preserve the
// completed pose for the next fetch/simulation cycle.
void NxSceneInternal::finishSimulation()
	{
	void** bodies = at<void**>(0x56c);
	void** bodiesEnd = at<void**>(0x570);
	for(void** item = bodies; item && item != bodiesEnd; ++item)
		{
			unsigned char* body = static_cast<unsigned char*>(*item);
			reinterpret_cast<Row000710Fixture*>(body)->row000710(&at<NxVec3>(0x520));
			if(*reinterpret_cast<NxReal*>(body + 0x84)
				+ *reinterpret_cast<NxReal*>(body + 0x4c) != 0.0f)
				for(NxU32 offset = 0x18; offset <= 0x4c; offset += 4)
					*reinterpret_cast<NxU32*>(body + 0x50 + offset - 0x18)
						= *reinterpret_cast<NxU32*>(body + offset);
		}
	}

// phys_fn_000523 (0x00010400, 4 B, phase 7): return the pair-flag count at +0x3c.
// The public pair-flag API and Scene::getPairFlagArray both consume this count.
NxU32 NxSceneInternal::getNbPairs() const
	{
	return at<NxU32>(0x3c);
	}

// phys_fn_000525 (0x00010410) and its continuation phys_fn_000527 (0x00010450).
// Index the static roots and the selected dynamic roots by their shape IDs,
// then translate each flagged hash entry into public shape or actor handles.
bool NxSceneInternal::getPairFlagArray(NxPairFlag* userArray, NxU32 numPairs) const
	{
	if(numPairs == 0)
		return false;
	const auto publicActorForBody = [this](const void* body) -> void*
		{
		if(!body)
			return 0;
		NxActor** actors = 0;
		NxActor** actorsEnd = 0;
		memcpy(&actors, bytes() + 0x55c, sizeof(actors));
		memcpy(&actorsEnd, bytes() + 0x560, sizeof(actorsEnd));
		for(NxActor** actor = actors; actor && actor != actorsEnd; ++actor)
			if(*actor && *reinterpret_cast<void**>(reinterpret_cast<NxU8*>(*actor) + 0x14) == body)
				return *actor;
		return 0;
		};
	void** shapeById = static_cast<void**>(_alloca(0x4000c));
	memset(shapeById, 0, 0x4000c);

	const unsigned char* const engine = bytes() + 0x624;
	const unsigned char selectedType = at<unsigned char>(0x624 + 0x70);
	const unsigned prunerOffsets[] = { 0x1cu, 0x1cu + static_cast<unsigned>(selectedType) * 4u };
	for(unsigned pass = 0; pass < 2; ++pass)
		{
			const Pruner* const pruner = *reinterpret_cast<Pruner* const*>(engine + prunerOffsets[pass]);
			if(!pruner)
				continue;
			const PruningPool& pool = pruner->mPool;
			const unsigned count = pool.mNbObjects[1] + pool.mNbObjects[2];
			for(unsigned index = 0; index < count; ++index)
				{
					Prunable* const prunable = pool.mObjects[pool.mNbObjects[0] + index];
					unsigned char* const shape = *reinterpret_cast<unsigned char**>(
						reinterpret_cast<unsigned char*>(prunable) + 4);
					shapeById[*reinterpret_cast<unsigned*>(shape + 0xd4)] = shape;
				}
		}

	const CpmPairHashEntry* entry = at<CpmPairHashEntry*>(0x40);
	NxU32 remainingHashEntries = at<NxU32>(0x3c);
	bool keepWalking = true;
	bool everyEntryHasPairFlags = true;
	while(remainingHashEntries && keepWalking)
		{
			--remainingHashEntries;
			const NxU32 value = entry->value;
			NxU32 storedFlags = 0;
			// The entry value is a tagged immediate for flags containing bit 0,
			// otherwise it points at a 0x14-byte record. Confirm the record's
			// marker too: contact-report records share this hash, and a relocated
			// 32-bit heap pointer can itself occupy the marker's address range.
			if(value & 0x20000000u)
				storedFlags = (value & 1u) ? value : *reinterpret_cast<const NxU32*>(value);
			if(storedFlags & 0x20000000u)
				{
					NxPairFlag& pair = *userArray++;
					pair.flags = (value & 1u) ? 1u : (storedFlags & 0x1fffffffu);
					unsigned char* const shape0 = static_cast<unsigned char*>(shapeById[entry->key0]);
					unsigned char* const shape1 = static_cast<unsigned char*>(shapeById[entry->key1]);
					if(*reinterpret_cast<NxU32*>(shape0 + 0xd0) != 5u &&
						*reinterpret_cast<NxU32*>(shape1 + 0xd0) != 5u)
						{
							pair.objects[0] = *reinterpret_cast<void**>(shape0 + 0x9c);
							pair.objects[1] = *reinterpret_cast<void**>(shape1 + 0x9c);
						}
					else
						{
							pair.objects[0] = publicActorForBody(*reinterpret_cast<void**>(shape0 + 4));
							pair.objects[1] = publicActorForBody(*reinterpret_cast<void**>(shape1 + 4));
							pair.flags |= 0x80000000u;
						}
					keepWalking = --numPairs != 0;
				}
			else
				everyEntryHasPairFlags = false;
			++entry;
		}
	return everyEntryHasPairFlags && remainingHashEntries == 0 && numPairs == 0;
	}

// phys_fn_000561 (0x00010870, 7 B, phase 7): the effector count at +0x6c4.
NxU32 NxSceneInternal::getNbEffectors() const
	{
	return at<NxU32>(0x6c4);
	}

// phys_fn_000565 (0x00010890, 13 B, phase 7): the cursor at +0x6c0 = the
// list head at +0x5a4.
void NxSceneInternal::resetEffectorIterator()
	{
	at<void*>(0x6c0) = at<void*>(0x5a4);
	}

// phys_fn_000569 (0x000108c0, 23 B, phase 7): the effector under the cursor,
// which advances through Effector +0x18; 0 at the end.
Effector* NxSceneInternal::getNextEffector()
	{
	Effector* effector = at<Effector*>(0x6c0);
	if(!effector)
		return 0;
	at<Effector*>(0x6c0) = effector->mNext;
	return effector;
	}
