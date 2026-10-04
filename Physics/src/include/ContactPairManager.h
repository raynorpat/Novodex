#ifndef NX_PHYSICS_CONTACT_PAIR_MANAGER
#define NX_PHYSICS_CONTACT_PAIR_MANAGER
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The contact-pair manager: the broadphase pair list and its nodes, the actor
// pair each node carries, the friction patches of an actor pair, the contact
// and friction constraint rows the simulation step builds from a pair's
// contact stream, and the contact reports. Rows 000750..000921 of the
// SceneRaycast.cpp..CapsuleShape.cpp gap (scene-raycast block Task 4, the
// contact-pair manager sub-area); ContactPairManager.cpp has the rows and the
// listing addresses.
//
// Every object here is reached through raw offsets, as the oracle reaches it.
// Only the offsets the rows read or write are named; meanings are the rows',
// not recovered names.
//
// The ABIs are the listing's. A thiscall row is a member of the raw-offset
// struct its `this` points at (MSVC rejects __thiscall on a free function,
// C3865; JointSupport.h's fixtures are the precedent); fastcall, stdcall and
// cdecl rows are free functions with that convention.

#include "Nxp.h"
#include "NxVec3.h"

class NxSceneInternal;
class NxDebugRenderable;
class NxUserContactReport;
class NxMaterial;
struct JointSupportRecord;

// The opaque pair-keyed hash rows 004153 (find), 004155 (insert) and 004157
// (erase) run on: the Scene's at Scene+0x2c and the SDK's actor-group pair
// flags at .data 0x10123c28. 000913/000917 walk it directly: count at +0x10,
// entries at +0x14, eight bytes each.
struct CpmPairHashEntry
	{
	NxU16	key0;		//!< +0x00
	NxU16	key1;		//!< +0x02
	NxU32	value;		//!< +0x04; a record pointer, or a flag word when bit 0 is set
	};

struct CpmPairHash
	{
	NxU8				head[0x10];
	NxU32				count;		//!< +0x10
	CpmPairHashEntry*	entries;	//!< +0x14
	};

void cpmSetActorGroupPairFlags(NxU16 group0, NxU16 group1, NxU32 flags);
NxU32 cpmGetActorGroupPairFlags(NxU16 group0, NxU16 group1);
void cpmResetActorGroupPairFlags();

// The per-actor-pair report record (0x14 bytes, allocated by 000905 through
// nxFoundationSDKAllocator and freed by 000913/000917): +0x00 state (bit 31
// touching this step, bit 30 touching last report, bit 29 kept while stale,
// low bits the pair's report flags), +0x04 frame stamp, +0x08 the pair node,
// +0x0c/+0x10 the node's two objects.

// The friction parameters 000861 (and 000859 on the anisotropic path) fill
// for one patch. 0x24 bytes on 000879's frame (0x1001ddd8 passes esp+0xd4).
struct CpmFrictionParams
	{
	NxU32	spring;					//!< +0x00; (flagsA | flagsB) & 4, NX_MF_SPRING_CONTACT
	NxReal	staticFrictionV;		//!< +0x04; times the normal force
	NxReal	staticFriction;			//!< +0x08; times the normal force
	NxReal	dynamicFriction;		//!< +0x0c; clamped to [0, 1]
	NxReal	dynamicFrictionV;		//!< +0x10; clamped to [0, 1]
	NxVec3	anisotropyDirection;	//!< +0x14; world frame (000859)
	NxU8	enabled;				//!< +0x20
	NxU8	pad[3];
	};

// The restitution 000857 writes: 8 bytes on 000897's frame.
struct CpmRestitution
	{
	NxReal	restitution;	//!< +0x00
	NxU32	spring;			//!< +0x04; (flagsA | flagsB) & 4
	};

// The 0x2c-byte buffered report record 000917 appends to Scene+0x60c.
struct CpmBufferedContact
	{
	void*	actors[2];			//!< +0x00
	void*	stream;				//!< +0x08
	NxU32	sumNormalForce[3];	//!< +0x0c (float bits)
	NxU32	sumFrictionForce[3];//!< +0x18 (float bits)
	void*	record;				//!< +0x24
	NxU32	events;				//!< +0x28
	};

/**
The friction patch: 0x78 bytes with a four-slot vtable (.rdata 0x10106944:
004248 `ret 4`, 001583 `ret`, 005242 `ret 8`, 000867 the accumulator). One is
embedded in every actor pair at +0x4c; the others are allocated by 000895.

+0x04 anchors, two of 0x18 bytes each: (body-0 local, body-1 local)
+0x34 the normal in body 0's frame   +0x40 the normal in body 1's frame
+0x4c the world normal               +0x58 the accumulated friction force
+0x64 the normal force               +0x68 the material ids (lo16 | hi16 << 16)
+0x6c the two shapes                 +0x74 the anchor count (byte)
+0x75 the re-anchor byte (000867 sets it; 000865 and 000891 clear it)
*/
class NxFrictionPatch
	{
	public:
	NxFrictionPatch() {}

	//! Slots 0-2: the folded empty bodies 004248 (`ret 4`), 001583 (`ret`)
	//! and 005242 (`ret 8`). Not claimed here.
	virtual void slot0(NxU32);
	virtual void slot1();
	virtual void slot2(NxU32, NxU32);
	//! Slot 3: phys_fn_000867 (ObjectModel.cpp nxAccumulateByKind0867,
	//! another sub-area's row). The candidate's 000867 is a cdecl free
	//! function; this slot forwards to it so the patch's vtable keeps the
	//! listing's thiscall `ret 0xc` shape.
	virtual void accumulate(NxReal value, void* record, NxReal divisor);

	//! phys_fn_000877: the copy (dwords +0x04..+0x70, bytes +0x74/+0x75; the
	//! vptr is not copied). thiscall, `ret 4`, returns this.
	NxFrictionPatch& operator=(const NxFrictionPatch& other);

	template <class T> T& at(NxU32 offset)
		{ return *reinterpret_cast<T*>(reinterpret_cast<NxU8*>(this) + offset); }
	template <class T> const T& at(NxU32 offset) const
		{ return *reinterpret_cast<const T*>(reinterpret_cast<const NxU8*>(this) + offset); }

	NxU8	mData[0x74];	//!< +0x04..+0x77
	};

/**
The actor pair: 0xec bytes at +0x14 of a pair node (000901 constructs it
there). Its +0x10..+0x47 is the contact stream sub-object the narrow phase
writes (ContactGeneration.h's NxContactSink is the same object from +0):
+0x10 contact count, +0x14/+0x18/+0x1c the stream indices of the pair,
normal and point counters, +0x20/+0x24 the last pair's objects, +0x28 the
last normal, +0x34 the pair's stream flags, +0x38 the stream's SdkContainer
(entries at +0x40).

+0x00/+0x04 the two objects (000893: each shape's +0x04)
+0x08/+0x0c the two bodies (each object's +0x08; either may be null)
+0x48 the patch count     +0x4c the embedded patch
+0xc4/+0xc8/+0xcc the extra patches {begin, end, capacity}
+0xd4 the normal force of the patches dropped this step (float)
+0xd8 the frame stamp     +0xe8/+0xe9 0xff (the narrow phase's cached axes)
*/
struct NxActorPair
	{
	template <class T> T& at(NxU32 offset)
		{ return *reinterpret_cast<T*>(mBytes + offset); }
	template <class T> const T& at(NxU32 offset) const
		{ return *reinterpret_cast<const T*>(mBytes + offset); }

	//! phys_fn_000865: stores up to `count` world points into the patch as
	//! body-local anchors. thiscall, `ret 0xc`.
	void row000865(NxFrictionPatch* patch, const NxReal* const* points, NxU8 count);
	//! phys_fn_000875: the second contact-stream emitter (32-bit features).
	//! thiscall on the pair, `ret 0x24`.
	void row000875(const NxU8* object1, const NxU8* object0, NxU32 separationBits,
		const NxReal* point, const NxReal* normal, NxU16 featureId0, NxU16 featureId1,
		NxU32 feature0, NxU32 feature1);
	//! phys_fn_000879: the friction rows. thiscall, `ret 0xc`.
	void row000879(NxSceneInternal* scene, NxReal forceScale, NxReal errorScale);
	//! phys_fn_000881: the pair's summed normal and friction forces.
	//! thiscall, `ret 8`.
	void row000881(NxVec3* sumNormalForce, NxVec3* sumFrictionForce);
	//! phys_fn_000883 + phys_fn_000885: the spring-contact row. thiscall,
	//! `ret 0x14`; the third and fifth arguments are not read.
	void row000883(NxSceneInternal* scene, NxFrictionPatch* patch, NxU32 separationBits,
		const NxReal* point, const NxReal* normal);
	//! phys_fn_000891: refresh or drop every patch. thiscall, `ret 4`.
	void row000891(NxSceneInternal* scene);
	//! phys_fn_000893: the constructor. thiscall, `ret 8`, returns this.
	NxActorPair* row000893(const NxU8* shape0, const NxU8* shape1);
	//! phys_fn_000895: find or create the patch for a normal. thiscall,
	//! `ret 0x10`.
	NxFrictionPatch* row000895(const NxU8* shape0, const NxU8* shape1, const NxReal* normal,
		NxU32 materialIds);
	//! phys_fn_000897 + phys_fn_000899: the contact and friction rows from
	//! the contact stream. thiscall, `ret 0xc`.
	void row000897(NxSceneInternal* scene, NxReal forceScale, NxReal penaltyScale);
	//! phys_fn_000869: the contact visualisation over the stream (+0x38's
	//! entries at +0x40). thiscall, `ret 4`. Defined in SceneVisualize.cpp.
	void row000869(NxDebugRenderable& renderable);

	NxU8	mBytes[0xec];
	};

struct NxPairList;

/**
The broadphase pair node: 0x108 bytes (000911 allocates them).
+0x00/+0x04 the cached `object->+0x10->+0x08` of each side (000905)
+0x08 next   +0x0c prev   +0x10 the list   +0x14 the actor pair
+0x100 0     +0x104 -1
*/
struct NxPairNode
	{
	template <class T> T& at(NxU32 offset)
		{ return *reinterpret_cast<T*>(mBytes + offset); }
	NxActorPair* pair() { return reinterpret_cast<NxActorPair*>(mBytes + 0x14); }

	//! phys_fn_000901: the constructor; links the node into the list.
	//! thiscall, `ret 0xc`, returns this.
	NxPairNode* row000901(NxU8* element0, NxU8* element1, NxPairList* list);
	//! phys_fn_000905: the per-step refresh. thiscall, `ret 4`.
	void row000905(NxSceneInternal* scene);
	//! phys_fn_000907: `add ecx, 0x14; jmp 000869`, the actor pair's
	//! contact visualisation. Defined in SceneVisualize.cpp.
	void row000907(NxDebugRenderable& renderable);

	NxU8	mBytes[0x108];
	};

// {head, tail}.
struct NxPairList
	{
	NxPairNode*	head;
	NxPairNode*	tail;

	//! phys_fn_000909: refresh the pairs with an awake body. thiscall,
	//! `ret 4`.
	void row000909(NxSceneInternal* scene);
	//! phys_fn_000911: create a node for two broadphase elements. thiscall,
	//! `ret 8`; null when the groups do not collide.
	NxPairNode* row000911(NxU8* element0, NxU8* element1);
	};

// phys_fn_001976's default all-pairs scene path, backed by the current static
// and dynamic pruning pools. Refreshes this Scene's persistent pair map/list
// and generates narrow-phase contacts for live pairs.
void nxSceneRefreshPairs(NxSceneInternal* scene);

// The body the joint-list test runs on (`this` = a body record, +0x1d8 its
// joint list).
struct CpmJointedBody
	{
	//! phys_fn_000750 + phys_fn_000752. thiscall, `ret 4`.
	NxU32 row000750(const CpmJointedBody* other) const;
	};

// Row 000855. stdcall, `ret 0xc`, the result left in st(0).
double __stdcall cpmCombine0855(NxReal a, NxReal b, NxU32 mode);
// Row 000857. stdcall, `ret 8`.
void __stdcall cpmRestitution0857(NxU32 materialIds, CpmRestitution* out);
// Row 000859. stdcall, `ret 0x14`.
void __stdcall cpmAnisotropicFriction0859(const NxU8* shape, const NxMaterial* material,
	const NxMaterial* other, NxReal normalForce, CpmFrictionParams* out);
// Row 000861. stdcall, `ret 0x1c`.
void __stdcall cpmFrictionParams0861(NxU8* const* shapes, NxU32 materialIds, const NxVec3* normal,
	NxReal normalForce, CpmFrictionParams* out, NxVec3* tangent0, NxVec3* tangent1);
// Row 000863. fastcall thunk.
void __fastcall cpmActorPairResetStream0863(NxActorPair* pair);
// Row 000871. cdecl.
NxU32 __cdecl cpmPatchSpring0871(const NxFrictionPatch* patch);
// Rows 000887 + 000889. fastcall.
void __fastcall cpmActorPairRelease0887(NxActorPair* pair);
// Row 000903. fastcall.
void __fastcall cpmPairNodeUnlink0903(NxPairNode* node);
// Row 000913. cdecl.
void __cdecl cpmFireContactReports0913(NxSceneInternal* scene, NxUserContactReport* report,
	CpmPairHash* hash);
// Row 000915. stdcall, `ret 4`.
void __stdcall cpmDeletePairNode0915(NxPairNode* node);
// Rows 000917 + 000919 + 000921. cdecl.
void __cdecl cpmBufferContactReports0917(NxSceneInternal* scene, CpmPairHash* hash);
void cpmSetActorPairFlags(NxSceneInternal* scene, void* actor0, void* actor1, NxU32 flags);
NxU32 cpmGetActorPairFlags(const NxSceneInternal* scene, const void* actor0, const void* actor1);
void cpmSetShapePairFlags(NxSceneInternal* scene, void* shape0, void* shape1, NxU32 flags);
NxU32 cpmGetShapePairFlags(const NxSceneInternal* scene, const void* shape0, const void* shape1);

#endif
