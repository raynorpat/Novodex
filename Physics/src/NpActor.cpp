/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The actor object and the concrete class its vtable points at. See NpActor.h for
// why they are two things rather than one.

#include "NpActor.h"
#include "NpActorDynamicMath.h"
#include "NpSceneGuard.h"
#include "PhysicsInternal.h"
#include "ObjectModel.h"
#include "FoundationSDK.h"
#include "BodyCreation.h"
#include "core/JointSupport.h"
#include "Scene.h"
#include "Observable.h"

#include "NxMat34.h"
#include "NxMat33.h"
#include "NxQuat.h"
#include "NxVec3.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxShape.h"
#include "NxBoxShape.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxBox.h"
#include "NxBounds3.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

static unsigned char* nxNpActorBody(void* actor)
	{
	return *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(actor) + 0x14);
	}

static unsigned char* nxNpActorRecord(void* actor)
	{
	unsigned char* body = nxNpActorBody(actor);
	return body ? *reinterpret_cast<unsigned char**>(body + 8) : 0;
	}

static void nxNpActorRotationFromQuaternionGetter(const float* q, float* rows);
static void nxNpActorRefreshCMass(unsigned char* record);
static void nxNpActorNotifyOwnedShapes(unsigned char* body);
// Actor.cpp's createShape/releaseShape (phys_fn_000036/000024, Scene.cpp).
unsigned char* nxActorCreateShape(unsigned char* body, const NxShapeDesc* descriptor);
void nxActorReleaseShape(unsigned char* body, unsigned char* shape);

// The RF operand orders of nxNpActorWorldMassRotation (defined below).
enum NxNpActorRfOrder { NX_RF_134, NX_RF_138, NX_RF_140, NX_RF_144 };
static void nxNpActorWorldMassRotation(const unsigned char* record,
	NxNpActorRfOrder order, float* worldMass);

// The world tensors of 000140 (diagonal +0x18c), 000142 (+0xc4) and 000144
// (+0x18c): W = R F in the row's own order, then 000746(diagonal, W, out).
static NxMat33 nxNpActorInstantTensor(const unsigned char* record,
	unsigned diagonalOffset, NxNpActorRfOrder order)
	{
	NxMat33 out(NX_IDENTITY_MATRIX);
	if(!record) return out;
	float worldMass[9];
	nxNpActorWorldMassRotation(record, order, worldMass);
	float tensor[9];
	nxNpActorWorldTensorRDRt(reinterpret_cast<const float*>(record + diagonalOffset),
		worldMass, tensor);
	memcpy(&out, tensor, sizeof(tensor));
	return out;
	}

static void* nxNpActorContext(void* actor, unsigned offset)
	{
	return *reinterpret_cast<void**>(static_cast<unsigned char*>(actor) + offset);
	}

// The read lock the unguarded readers take in the oracle (contract NG):
// 002362 on [actor+0x10] before the read and 002366 after it, the result
// being formed in between.
struct NxNpActorReadGuard
	{
	void* ctx;
	NxNpActorReadGuard(const void* actor)
		: ctx(nxNpActorContext(const_cast<void*>(actor), 0x10)) { nxNpSceneGuardEnter(ctx); }
	~NxNpActorReadGuard() { nxNpSceneGuardLeave(ctx); }
	};

// The reports every public actor row makes (contract G1 and E1), in the form
// of 000128's warning. Each is written as the joint units write theirs,
// FoundationSDK::getInstance().error(...): the variadic static error is the
// Foundation import and is never inlined, while evaluating getInstance()
// inlines its instance test, so the built code is the oracle's
// `mov eax,[__imp_instance]; cmp [eax],0; jne; int3` ahead of the
// `call [__imp_error]` (oracle 0x10004442-0x1000445c, 0x100070a0-0x100070be;
// checked in the candidate DLL). The file string is 0x10104690 and each
// message is the row's own .rdata string, copied byte for byte.
#define NX_NPACTOR_CPP "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpActor.cpp"

// E1: kind 1 (NXE_INVALID_PARAMETER), the row's line and message.
static void nxNpActorReport(int line, const char* message)
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_NPACTOR_CPP,
		line, 0, message);
	}

// G1: phys_fn_002364 (0x5b730) returns 0 when another thread holds the write
// flag; the row then reports kind 2 (NXE_INVALID_OPERATION) with the message
// at 0x10104760 and returns without unlocking.
static bool nxNpActorWriteTry(void* ctx, int line)
	{
	if(nxNpSceneGuardWriteTry(ctx)) return true;
	NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, NX_NPACTOR_CPP,
		line, 0, "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
	return false;
	}

// The dirty mark every record setter inlines (contract H1), from 000184's
// 0x10008285-0x1000836c; 000170 (0x1000713d-0x1000722f), 000785's four marks
// (0x1001966f-0x1001975e, ...) and the other rows inline the same NxArray
// pushBack with only the register allocation and store order varying. The
// dirty manager is [record+0x120]+0x40: +0x00 the per-id flag words, +0x10/
// +0x14/+0x18 the dirty list's begin/end/capacity, +0x20 the id-to-index
// table. Nothing is null-tested and the id is not bounded. A clean id first
// records its index (the list length), then the list grows when full: to
// 2n+2 entries, unless the current capacity (0 for a null list) already
// covers that, through the imported nxFoundationSDKAllocator ([0x101041bc]
// vtable +8 malloc(size, 0), +0x14 free), copying the live entries and freeing
// the old block only when it is non-null. The id is appended and the mask ORed.
// One recorded difference, kept from the scene-raycast block's copy of this
// helper at the second merge (this is now the only one: BodyCreation.cpp's
// 000793/000795 call it too): the `id < 256` test is not in the image.
// Scene.cpp's model of the manager (002421) keeps fixed 256-entry tables, and
// the test keeps a 257th record from writing past them; below 256 records it
// never fails.
void nxNpActorMarkRecordDirty(unsigned char* record, unsigned mask)
	{
	unsigned char* manager = *reinterpret_cast<unsigned char**>(record + 0x120) + 0x40;
	const unsigned id = *reinterpret_cast<unsigned*>(record + 0x11c);
	if(id >= 256)
		return;
	unsigned* flags = *reinterpret_cast<unsigned**>(manager);
	if(flags[id] == 0)
		{
		unsigned*& begin = *reinterpret_cast<unsigned**>(manager + 0x10);
		unsigned*& end = *reinterpret_cast<unsigned**>(manager + 0x14);
		unsigned*& capacity = *reinterpret_cast<unsigned**>(manager + 0x18);
		unsigned* index = *reinterpret_cast<unsigned**>(manager + 0x20);
		index[id] = static_cast<unsigned>(end - begin);
		if(!(capacity > end))
			{
			const unsigned next = static_cast<unsigned>(end - begin) * 2 + 2;
			const unsigned current = begin ? static_cast<unsigned>(capacity - begin) : 0;
			if(current < next)
				{
				unsigned* grown = static_cast<unsigned*>(nxFoundationSDKAllocator->malloc(
					next * sizeof(unsigned), NX_MEMORY_PERSISTENT));
				unsigned* to = grown;
				for(unsigned* from = begin; from != end; ++from, ++to)
					*to = *from;
				if(begin)
					nxFoundationSDKAllocator->free(begin);
				end = grown + (end - begin);
				capacity = grown + next;
				begin = grown;
				}
			}
		*end++ = id;
		}
	flags[id] |= mask;
	}

// phys_fn_000785 (0x00019620, 1325 B)
// phys_fn_000787 (0x00019b50, 428 B)
// The kinematic transition raiseBodyFlag/clearBodyFlag call before their own
// flag OR/AND (000787 is 000785's tail, 0x19b50-0x19cf9). Enable, when the
// record is not already kinematic: +0xc0 = 0, mark 0x10000; +0xc4..+0xcc = 0,
// mark 0x20000; flags |= 0x80, mark 0x80000; then the 0x20-byte target block
// at +0x118 is allocated if absent (0x1001995d, no null check) and its +0xc
// cleared. Disable, when it is: flags &= ~0x80, mark 0x80000; +0xc0 = 1/mass,
// mark 0x10000; +0xc4..+0xcc = 1/inertia (unguarded fld 1; fdiv), mark
// 0x20000; then the block is freed (0x10019cda) and cleared. Both arms begin
// (0x19643-0x19668, 0x19992-0x199b7) with the island-root refresh: a record
// that is not its own root (+0x1bc) stores its parent's 000712 (the root,
// found with path compression), and when the root has an island object
// (+0x1e0) its +0x1e4 word gets bit 2.
static void nxNpActorRefreshIslandRoot(unsigned char* record)
	{
	unsigned char*& parent = *reinterpret_cast<unsigned char**>(record + 0x1bc);
	if(parent != record)
		parent = reinterpret_cast<unsigned char*>(
			reinterpret_cast<Row000712Fixture*>(parent)->row000712());
	unsigned char* root = parent;
	if(*reinterpret_cast<void**>(root + 0x1e0))
		*reinterpret_cast<unsigned*>(root + 0x1e4) |= 2u;
	}

void nxNpActorTransitionKinematic(unsigned char* record, bool enable)
	{
	unsigned& flags = *reinterpret_cast<unsigned*>(record + 0x10c);
	void*& state = *reinterpret_cast<void**>(record + 0x118);
	float* inverse = reinterpret_cast<float*>(record + 0xc0);
	const float* mass = reinterpret_cast<const float*>(record + 0x188);
	if(enable)
		{
		if(flags & 0x80u) return;
		nxNpActorRefreshIslandRoot(record);
		inverse[0] = 0.0f;
		nxNpActorMarkRecordDirty(record, 0x10000u);
		inverse[1] = 0.0f;
		inverse[2] = 0.0f;
		inverse[3] = 0.0f;
		nxNpActorMarkRecordDirty(record, 0x20000u);
		flags |= 0x80u;
		nxNpActorMarkRecordDirty(record, 0x80000u);
		if(!state)
			state = nxFoundationSDKAllocator->malloc(0x20, NX_MEMORY_PERSISTENT);
		*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(state) + 0xc) = 0;
		}
	else
		{
		if(!(flags & 0x80u)) return;
		nxNpActorRefreshIslandRoot(record);
		flags &= ~0x80u;
		nxNpActorMarkRecordDirty(record, 0x80000u);
		inverse[0] = static_cast<float>(1.0 / mass[0]);
		nxNpActorMarkRecordDirty(record, 0x10000u);
		inverse[1] = static_cast<float>(1.0 / mass[1]);
		inverse[2] = static_cast<float>(1.0 / mass[2]);
		inverse[3] = static_cast<float>(1.0 / mass[3]);
		nxNpActorMarkRecordDirty(record, 0x20000u);
		if(state)
			{
			nxFoundationSDKAllocator->free(state);
			state = 0;
			}
		}
	}

// The vtable word. A single static instance of the concrete class supplies it: the
// object needs a vtable POINTER, not a class instance, so one instance is enough for
// every actor the reconstruction builds.
static NpActorVtable gNpActorVtable;

// The public 0x1c-byte box handle is separate from its 0x228-byte internal
// shape. Its final table has 35 entries in the shipped x86 image; only the
// measured entries below are reconstructed here. An unimplemented entry aborts
// instead of returning a plausible but false result.
static void __fastcall nxUnsupportedBoxMethod(void*, void*) { abort(); }

static unsigned char* nxBoxHandleInternal(void* self)
	{
	unsigned char* shape = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 0x18);
	if(!shape) abort();
	return shape;
	}

static NxActor* __fastcall nxBoxHandleGetActor(void* self, void*);

// phys_fn_10026c90: mark the internal shape for the Scene's deferred update.
void nxSceneMarkShapeDirty(void* shape, unsigned flag);

static void __fastcall nxBoxHandleSetGroup(void* self, void*, NxCollisionGroup group)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	if(group >= 32) return;
	*reinterpret_cast<NxCollisionGroup*>(shape + 0xd8) = group;
	nxSceneMarkShapeDirty(shape, 4);
	*reinterpret_cast<unsigned*>(shape + 0xc8) = 1u << group;
	}

static void __fastcall nxShapeHandleGetWorldBounds(void* self, void*,
	NxBounds3& bounds)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	void** table = *reinterpret_cast<void***>(shape);
	typedef void (__thiscall* BoundsFn)(void*, float*);
	reinterpret_cast<BoundsFn>(table[9])(shape, &bounds.getMin().x);
	}

static void __fastcall nxShapeHandleGetLocalPose(void* self, void*,
	NxMat34& pose)
	{
	memcpy(&pose, nxBoxHandleInternal(self) + 0x6c, sizeof(pose));
	}

static void __fastcall nxShapeHandleGetLocalPosition(void* self, void*,
	NxVec3& position)
	{
	memcpy(&position, nxBoxHandleInternal(self) + 0x90, sizeof(position));
	}

static void __fastcall nxShapeHandleGetLocalOrientation(void* self, void*,
	NxMat33& orientation)
	{
	memcpy(&orientation, nxBoxHandleInternal(self) + 0x6c,
		sizeof(orientation));
	}

static void __fastcall nxShapeHandleSetLocalPosition(void* self, void*,
	const NxVec3& position)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	memcpy(shape + 0x90, &position, sizeof(position));
	static_cast<ShapeBase*>(static_cast<void*>(shape))->nxApplyOwnerUpdate(1);
	}

static void __fastcall nxShapeHandleSetLocalOrientation(void* self, void*,
	const NxMat33& orientation)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	memcpy(shape + 0x6c, &orientation, sizeof(orientation));
	static_cast<ShapeBase*>(static_cast<void*>(shape))->nxApplyOwnerUpdate(1);
	}

static void __fastcall nxShapeHandleSetLocalPose(void* self, void*,
	const NxMat34& pose)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	memcpy(shape + 0x6c, &pose, sizeof(pose));
	static_cast<ShapeBase*>(static_cast<void*>(shape))->nxApplyOwnerUpdate(1);
	}

static void __fastcall nxShapeHandleSetGlobalPosition(void* self, void*,
	const NxVec3& position)
	{
	NxActor* actor = nxBoxHandleGetActor(self, 0);
	NxVec3 local = actor->getGlobalPose() % position;
	nxShapeHandleSetLocalPosition(self, 0, local);
	}

static void __fastcall nxShapeHandleSetGlobalOrientation(void* self, void*,
	const NxMat33& orientation)
	{
	NxActor* actor = nxBoxHandleGetActor(self, 0);
	NxMat33 local;
	local.multiplyTransposeLeft(actor->getGlobalOrientation(), orientation);
	nxShapeHandleSetLocalOrientation(self, 0, local);
	}

static void __fastcall nxShapeHandleSetGlobalPose(void* self, void*,
	const NxMat34& pose)
	{
	NxActor* actor = nxBoxHandleGetActor(self, 0);
	NxMat34 local;
	local.multiplyInverseRTLeft(actor->getGlobalPose(), pose);
	nxShapeHandleSetLocalPose(self, 0, local);
	}

static void __fastcall nxShapeHandleGetGlobalPose(void* self, void*,
	NxMat34& pose)
	{
	static_cast<ShapeBase*>(static_cast<void*>(nxBoxHandleInternal(self)))->nxShapeGlobalPose(
		reinterpret_cast<float*>(&pose));
	}

static void __fastcall nxShapeHandleGetGlobalPosition(void* self, void*,
	NxVec3& position)
	{
	memcpy(&position, nxBoxHandleInternal(self) + 0x30, sizeof(position));
	}

static void __fastcall nxShapeHandleGetGlobalOrientation(void* self, void*,
	NxMat33& orientation)
	{
	memcpy(&orientation, nxBoxHandleInternal(self) + 0x0c,
		sizeof(orientation));
	}

static NxMat34* __fastcall nxShapeHandleGetLocalPoseVal(void* self, void*,
	NxMat34* pose)
	{
	nxShapeHandleGetLocalPose(self, 0, *pose);
	return pose;
	}

static NxVec3* __fastcall nxShapeHandleGetLocalPositionVal(void* self, void*,
	NxVec3* position)
	{
	nxShapeHandleGetLocalPosition(self, 0, *position);
	return position;
	}

static NxMat33* __fastcall nxShapeHandleGetLocalOrientationVal(void* self, void*,
	NxMat33* orientation)
	{
	nxShapeHandleGetLocalOrientation(self, 0, *orientation);
	return orientation;
	}

static NxMat34* __fastcall nxShapeHandleGetGlobalPoseVal(void* self, void*,
	NxMat34* pose)
	{
	nxShapeHandleGetGlobalPose(self, 0, *pose);
	return pose;
	}

static NxVec3* __fastcall nxShapeHandleGetGlobalPositionVal(void* self, void*,
	NxVec3* position)
	{
	nxShapeHandleGetGlobalPosition(self, 0, *position);
	return position;
	}

static NxMat33* __fastcall nxShapeHandleGetGlobalOrientationVal(void* self, void*,
	NxMat33* orientation)
	{
	nxShapeHandleGetGlobalOrientation(self, 0, *orientation);
	return orientation;
	}

static NxCollisionGroup __fastcall nxBoxHandleGetGroup(void* self, void*)
	{
	return *reinterpret_cast<NxCollisionGroup*>(nxBoxHandleInternal(self) + 0xd8);
	}

static void __fastcall nxBoxHandleSetFlag(void* self, void*, NxShapeFlag flag,
	bool value)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	NxU16& flags = *reinterpret_cast<NxU16*>(shape + 0xde);
	const NxU16 mask = static_cast<NxU16>(flag);
	if(value)
		flags |= mask;
	else
		flags &= static_cast<NxU16>(~mask);
	nxSceneMarkShapeDirty(shape, 0x10);
	}

static NX_BOOL __fastcall nxBoxHandleGetFlag(void* self, void*, NxShapeFlag flag)
	{
	return *reinterpret_cast<NxU16*>(nxBoxHandleInternal(self) + 0xde)
		& static_cast<NxU16>(flag);
	}

static void __fastcall nxBoxHandleSetMaterial(void* self, void*, NxMaterialIndex material)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	*reinterpret_cast<NxMaterialIndex*>(shape + 0xda) = material;
	nxSceneMarkShapeDirty(shape, 8);
	}

static NxMaterialIndex __fastcall nxBoxHandleGetMaterial(void* self, void*)
	{
	return *reinterpret_cast<NxMaterialIndex*>(nxBoxHandleInternal(self) + 0xda);
	}

// The oracle's global name map (.data 0x00123c0c) stores pointer pairs for
// actors, shapes and joints: phys_fn_000480 sets or removes a pair and
// phys_fn_000454 looks one up (PhysicsInternal.cpp's nxSetSdkPointerBinding
// and nxGetSdkPointerBinding). A null-name call for a missing object still
// inserts a null association once the table exists; removing its last
// existing association destroys the table. The shape and actor names go
// through the same table as the joint names (NpJointShared.h slot 29, ~Joint):
// an earlier copy of it here kept them in a second table, so the joints' and
// the actors' pairs never shared its growth or its destruction (found at the
// merge of main into the NpActor.cpp completion branch, where the core dump's
// outstanding-block count after the scene releases differed by the second
// table's two blocks).
void nxShapeSetName(void* shape, const char* name)
	{
	nxSetSdkPointerBinding(shape, const_cast<char*>(name));
	}

const char* nxShapeGetName(void* shape)
	{
	return static_cast<const char*>(nxGetSdkPointerBinding(shape));
	}

static void __fastcall nxBoxHandleSetName(void* self, void*, const char* name)
	{
	nxShapeSetName(nxBoxHandleInternal(self), name);
	}

static const char* __fastcall nxBoxHandleGetName(void* self, void*)
	{
	return nxShapeGetName(nxBoxHandleInternal(self));
	}

static NxActor* __fastcall nxBoxHandleGetActor(void* self, void*)
	{
	unsigned char* body = *reinterpret_cast<unsigned char**>(nxBoxHandleInternal(self) + 4);
	if(!body) abort();
	return *reinterpret_cast<NxActor**>(body);
	}

static NxShapeType __fastcall nxBoxHandleGetType(void* self, void*)
	{
	return static_cast<NxShapeType>(*reinterpret_cast<unsigned*>(
		nxBoxHandleInternal(self) + 0xd0));
	}

static void* __fastcall nxBoxHandleIs(void* self, void*, NxShapeType requested)
	{
	return requested == nxBoxHandleGetType(self, 0) ? self : 0;
	}

static const NxVec3* __fastcall nxBoxHandleGetDimensions(void* self, void*)
	{
	return reinterpret_cast<const NxVec3*>(nxBoxHandleInternal(self) + 0xe4);
	}

static void __fastcall nxBoxHandleGetWorldOBB(void* self, void*, NxBox& box)
	{
	// 001073 calls 000933 on the internal shape (0x10023566).
	unsigned char* shape = nxBoxHandleInternal(self);
	static_cast<BoxShape*>(static_cast<void*>(shape))->nxBoxGetWorldOBB(
		&box.center.x);
	}

static bool __fastcall nxBoxHandleSaveToDesc(void* self, void*,
	NxBoxShapeDesc& descriptor)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	memcpy(&descriptor.localPose, shape + 0x6c, sizeof(descriptor.localPose));
	descriptor.shapeFlags = *reinterpret_cast<NxU16*>(shape + 0xde);
	descriptor.group = *reinterpret_cast<NxCollisionGroup*>(shape + 0xd8);
	descriptor.materialIndex = *reinterpret_cast<NxMaterialIndex*>(shape + 0xda);
	descriptor.userData = *reinterpret_cast<void**>(
		static_cast<unsigned char*>(self) + 4);
	memcpy(&descriptor.dimensions, shape + 0xe4, sizeof(descriptor.dimensions));
	return true;
	}

static bool __fastcall nxShapeHandleSaveToDesc(void* self, void*,
	NxShapeDesc& descriptor)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	void** table = *reinterpret_cast<void***>(shape);
	typedef bool (__thiscall* SaveFn)(void*, void*);
	const bool saved = reinterpret_cast<SaveFn>(table[13])(shape, &descriptor);
	return saved;
	}

static NxReal __fastcall nxShapeHandleGetRadius(void* self, void*)
	{
	return *reinterpret_cast<const NxReal*>(nxBoxHandleInternal(self) + 0xe0);
	}

static NxReal __fastcall nxCapsuleHandleGetHeight(void* self, void*)
	{
	return 2.0f * *reinterpret_cast<const NxReal*>(
		nxBoxHandleInternal(self) + 0xe4);
	}

static void __fastcall nxSphereHandleSetRadius(void* self, void*, NxReal radius)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	*reinterpret_cast<NxReal*>(shape + 0xe0) = radius;
	static_cast<ShapeBase*>(static_cast<void*>(shape))->nxApplyOwnerUpdate(1);
	nxSceneMarkShapeDirty(shape, 0x20);
	}

static void __fastcall nxCapsuleHandleSetRadius(void* self, void*, NxReal radius)
	{
	// NpCapsuleShape::setRadius calls the internal shape's slot 14 (000995)
	// through its table (call [edx+0x38] at 0x10023bd5).
	unsigned char* shape = nxBoxHandleInternal(self);
	void** table = *reinterpret_cast<void***>(shape);
	typedef void (__thiscall* SetRadiusFn)(void*, NxReal);
	reinterpret_cast<SetRadiusFn>(table[14])(shape, radius);
	}

static void __fastcall nxCapsuleHandleSetHeight(void* self, void*, NxReal height)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	*reinterpret_cast<NxReal*>(shape + 0xe4) = height * 0.5f;
	static_cast<ShapeBase*>(static_cast<void*>(shape))->nxApplyOwnerUpdate(1);
	}

// phys_fn_000983 (0x000219d0, 62 B)
// NxBoxShape::setDimensions' internal row, called by 001069 on the handle's
// +0x18 shape (0x10023503); __thiscall ret 4 (ecx = the internal box, the
// fastcall's edx unused). The three dims words copied as dwords to
// +0xe4..+0xec (0x100219d7-0x100219ed), the hull rebuild 000973 (call
// 0x10021420 at 0x100219f3), BASE slot 6 through the shape's own table with 1
// (call [edx+0x18] at 0x100219fe), then 001325 with dirty flag 0x40 (call
// 0x10026c90 at 0x10021a05).
static __declspec(noinline) void __fastcall nxBoxSetDimensions(void* self, void*,
	const NxVec3& dimensions)
	{
	unsigned char* shape = static_cast<unsigned char*>(self);
	memcpy(shape + 0xe4, &dimensions.x, 4);
	memcpy(shape + 0xe8, &dimensions.y, 4);
	memcpy(shape + 0xec, &dimensions.z, 4);
	static_cast<BoxShape*>(self)->nxBoxRebuildHull();
	typedef void (__thiscall* OwnerUpdateFn)(void*, unsigned);
	reinterpret_cast<OwnerUpdateFn>((*reinterpret_cast<void***>(shape))[6])(shape, 1);
	nxSceneMarkShapeDirty(shape, 0x40);
	}

// 001069's role (NpBoxShape.cpp in the image; the scene lock and its report
// are not reproduced here): the handle's internal shape through 000983.
static void __fastcall nxBoxHandleSetDimensions(void* self, void*,
	const NxVec3& dimensions)
	{
	nxBoxSetDimensions(nxBoxHandleInternal(self), 0, dimensions);
	}

// The reconstruction's error stream (ObjectModel.cpp).
void nxReport(int kind, const char* file, int line, int code,
	const char* message);

// phys_fn_000993 (0x00021b70, 108 B)
// CapsuleShape::setDimensions, __thiscall `ret 8` (0x10021b70-0x10021bd9),
// called by NpCapsuleShape::setDimensions (001113, 0x10023b78) with
// (radius, height). Defined here rather than in ObjectModel.cpp because it
// calls 001325 (Scene.cpp), which the ObjectModel-only test targets do not link.
__declspec(noinline) void CapsuleShape::nxCapsuleSetDimensions(float radius,
	float height)
	{
	mFloatE0 = radius;						// dword copy, 0x10021b81
	mFloatE4 = height * 0.5f;				// fld; fmul [0x101043cc]; fstp +0xe4
	// fcomp of the stack argument against 0.0 (0x10021b91); `test ah,0x41;
	// jp` skips on greater and on unordered: report (line 0x4f) on <= 0.
	if(radius <= 0.0f)
		nxReport(1, nxSourceFileCapsuleShapeCpp, 0x4f, 0,
			nxMsgCapsuleSetDimensionsRadius);
	void** vtable = static_cast<void**>(mBase.mVptrSlot);
	typedef void (__thiscall* OwnerUpdateFn)(void*, unsigned);
	reinterpret_cast<OwnerUpdateFn>(vtable[6])(this, 1u);	// call [edx+0x18], 0x10021bc9
	nxSceneMarkShapeDirty(this, 0x100);		// 001325(0x100), 0x10021bd3
	}

static void __fastcall nxCapsuleHandleSetDimensions(void* self, void*,
	NxReal radius, NxReal height)
	{
	// 001113 calls 000993 on the internal shape (0x10023b78).
	unsigned char* shape = nxBoxHandleInternal(self);
	static_cast<CapsuleShape*>(static_cast<void*>(shape))->nxCapsuleSetDimensions(
		radius, height);
	}

static void __fastcall nxPlaneHandleSetPlane(void* self, void*,
	const NxVec3& normal, NxReal distance)
	{
	unsigned char* shape = nxBoxHandleInternal(self);
	static_cast<PlaneShape*>(static_cast<void*>(shape))->nxPlaneSetEquation(
		&normal.x, distance);
	nxSceneMarkShapeDirty(shape, 0x80);
	}

void* nxBoxShapePublicVtable()
	{
	struct Table
		{
		void* slots[35];
		Table()
			{
			for(unsigned i = 0; i < 35; ++i)
				slots[i] = reinterpret_cast<void*>(&nxUnsupportedBoxMethod);
			slots[1] = reinterpret_cast<void*>(&nxBoxHandleGetActor);
			slots[2] = reinterpret_cast<void*>(&nxBoxHandleSetGroup);
			slots[3] = reinterpret_cast<void*>(&nxBoxHandleGetGroup);
			slots[4] = reinterpret_cast<void*>(&nxShapeHandleGetWorldBounds);
			slots[5] = reinterpret_cast<void*>(&nxBoxHandleSetFlag);
			slots[6] = reinterpret_cast<void*>(&nxBoxHandleGetFlag);
			slots[7] = reinterpret_cast<void*>(&nxShapeHandleSetLocalPose);
			slots[8] = reinterpret_cast<void*>(&nxShapeHandleSetLocalPosition);
			slots[9] = reinterpret_cast<void*>(&nxShapeHandleSetLocalOrientation);
			slots[10] = reinterpret_cast<void*>(&nxShapeHandleGetLocalPose);
			slots[11] = reinterpret_cast<void*>(&nxShapeHandleGetLocalPosition);
			slots[12] = reinterpret_cast<void*>(&nxShapeHandleGetLocalOrientation);
			slots[13] = reinterpret_cast<void*>(&nxShapeHandleGetLocalPoseVal);
			slots[14] = reinterpret_cast<void*>(&nxShapeHandleGetLocalPositionVal);
			slots[15] = reinterpret_cast<void*>(&nxShapeHandleGetLocalOrientationVal);
			slots[16] = reinterpret_cast<void*>(&nxShapeHandleSetGlobalPose);
			slots[17] = reinterpret_cast<void*>(&nxShapeHandleSetGlobalPosition);
			slots[18] = reinterpret_cast<void*>(&nxShapeHandleSetGlobalOrientation);
			slots[19] = reinterpret_cast<void*>(&nxShapeHandleGetGlobalPose);
			slots[20] = reinterpret_cast<void*>(&nxShapeHandleGetGlobalPosition);
			slots[21] = reinterpret_cast<void*>(&nxShapeHandleGetGlobalOrientation);
			slots[22] = reinterpret_cast<void*>(&nxShapeHandleGetGlobalPoseVal);
			slots[23] = reinterpret_cast<void*>(&nxShapeHandleGetGlobalPositionVal);
			slots[24] = reinterpret_cast<void*>(&nxShapeHandleGetGlobalOrientationVal);
			slots[25] = reinterpret_cast<void*>(&nxBoxHandleSetMaterial);
			slots[26] = reinterpret_cast<void*>(&nxBoxHandleGetMaterial);
			slots[27] = reinterpret_cast<void*>(&nxBoxHandleGetType);
			slots[28] = reinterpret_cast<void*>(&nxBoxHandleIs);
			slots[29] = reinterpret_cast<void*>(&nxBoxHandleSetName);
			slots[30] = reinterpret_cast<void*>(&nxBoxHandleGetName);
			slots[31] = reinterpret_cast<void*>(&nxBoxHandleSetDimensions);
			slots[32] = reinterpret_cast<void*>(&nxBoxHandleGetDimensions);
			slots[33] = reinterpret_cast<void*>(&nxBoxHandleGetWorldOBB);
			slots[34] = reinterpret_cast<void*>(&nxBoxHandleSaveToDesc);
			}
		};
	static Table table;
	return table.slots;
	}

// Public shape handles share the NxShape prefix through slot 30. The final
// geometry slots differ by family and remain explicit unsupported entries
// until their individual implementations are reconstructed.
void* nxShapePublicVtable(unsigned type)
	{
	if(type == 2u) return nxBoxShapePublicVtable();
	struct Tables
		{
		void* slots[3][37];
		Tables()
			{
			void** box = static_cast<void**>(nxBoxShapePublicVtable());
			for(unsigned family = 0; family < 3; ++family)
				{
				for(unsigned slot = 0; slot < 37; ++slot)
					slots[family][slot] =
						slot < 31 ? box[slot]
						: reinterpret_cast<void*>(&nxUnsupportedBoxMethod);
				}
			slots[0][31] = reinterpret_cast<void*>(&nxPlaneHandleSetPlane);
			slots[0][32] = reinterpret_cast<void*>(&nxShapeHandleSaveToDesc);
			slots[1][31] = reinterpret_cast<void*>(&nxSphereHandleSetRadius);
			slots[1][32] = reinterpret_cast<void*>(&nxShapeHandleGetRadius);
			slots[1][33] = reinterpret_cast<void*>(&nxShapeHandleSaveToDesc);
			slots[2][31] = reinterpret_cast<void*>(&nxCapsuleHandleSetDimensions);
			slots[2][32] = reinterpret_cast<void*>(&nxCapsuleHandleSetRadius);
			slots[2][33] = reinterpret_cast<void*>(&nxShapeHandleGetRadius);
			slots[2][34] = reinterpret_cast<void*>(&nxCapsuleHandleSetHeight);
			slots[2][35] = reinterpret_cast<void*>(&nxCapsuleHandleGetHeight);
			slots[2][36] = reinterpret_cast<void*>(&nxShapeHandleSaveToDesc);
			}
		};
	static Tables tables;
	switch(type)
		{
		case 0u: return tables.slots[0];
		case 1u: return tables.slots[1];
		case 3u: return tables.slots[2];
		default: return nullptr;
		}
	}

void NpActorObject::installVtable()
	{
	*reinterpret_cast<void**>(mBytes) = *reinterpret_cast<void**>(&gNpActorVtable);
	}


// phys_fn_000110 (0x00003580, 35 B)
// Dynamic actor vtable slot 19: under the read lock, whether the body at
// +0x14 (not null-tested) has a record at +0x08.
bool NpActorVtable::isDynamic() const
	{
	NxNpActorReadGuard guard(this);
	const unsigned char* body = nxNpActorBody(const_cast<NpActorVtable*>(this));
	return *reinterpret_cast<const unsigned*>(body + 0x08) != 0;
	}

// phys_fn_000196 at 0x00008b00, actor dynamic slot 1. The dynamic path
// marks orientation dirty before position, then refreshes the mass frame
// and owned shape only once for the combined pose.
void NpActorVtable::setGlobalPose(const NxMat34& pose)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x21d)) return;
	unsigned char* body = nxNpActorBody(this);
	if(body)
		{
		unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
		if(record)
			{
			// 0x8b5c-0x8d07: the setter's own conversion, not NxQuat(NxMat33).
			float quaternion[4];
			nxNpActorSetterQuaternionFromMatrix(reinterpret_cast<const float*>(&pose.M), quaternion);
			memcpy(record + 0x5c, quaternion, sizeof(quaternion));
			memcpy(record + 0x24, record + 0x5c, sizeof(quaternion));
			nxNpActorMarkRecordDirty(record, 2);
			memcpy(record + 0x50, &pose.t, sizeof(pose.t));
			memcpy(record + 0x18, record + 0x50, sizeof(pose.t));
			nxNpActorMarkRecordDirty(record, 1);
			nxNpActorRefreshCMass(record);
			}
		else
			{
			memcpy(body + 0x44, &pose.t, sizeof(pose.t));
			memcpy(body + 0x20, &pose.M, sizeof(pose.M));
			}
		nxNpActorNotifyOwnedShapes(body);
		}
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000146, actor dynamic slot 65 at 0x000058f0. The world-space
// point is measured from the transformed mass-frame center.
NxVec3 NpActorVtable::getPointVelocityVal(const NxVec3& point) const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	const unsigned char* record = nxNpActorRecord(self);
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record)
		{
		float rotation[9];
		nxNpActorRotationFromQuaternionGetter(
			reinterpret_cast<const float*>(record + 0x5c), rotation);
		const float* mass = reinterpret_cast<const float*>(record + 0x100);
		const float* position = reinterpret_cast<const float*>(record + 0x50);
		const float* velocity = reinterpret_cast<const float*>(record + 0x6c);
		const float* angular = reinterpret_cast<const float*>(record + 0x78);
#if defined(_MSC_VER) && defined(_M_IX86)
		const float* r = rotation;
		const float* worldPoint = &point.x;
		float* destination = &out.x;
		float dot1, dot2, centerX, radiusX, radiusY, crossX, crossY;
		__asm {
			mov eax, r
			mov edx, mass
			fld dword ptr [eax]
			fmul dword ptr [edx]
			fld dword ptr [eax+4]
			fmul dword ptr [edx+4]
			faddp st(1), st(0)
			fld dword ptr [eax+8]
			fmul dword ptr [edx+8]
			faddp st(1), st(0)
			mov ecx, position
			fadd dword ptr [ecx]
			fstp dword ptr [centerX]
			fld dword ptr [eax+16]
			fmul dword ptr [edx+4]
			fld dword ptr [eax+20]
			fmul dword ptr [edx+8]
			faddp st(1), st(0)
			fld dword ptr [eax+12]
			fmul dword ptr [edx]
			faddp st(1), st(0)
			fstp dword ptr [dot1]
			fld dword ptr [eax+28]
			fmul dword ptr [edx+4]
			fld dword ptr [eax+32]
			fmul dword ptr [edx+8]
			faddp st(1), st(0)
			fld dword ptr [eax+24]
			fmul dword ptr [edx]
			faddp st(1), st(0)
			fstp dword ptr [dot2]
			mov eax, worldPoint
			fld dword ptr [eax+8]
			fld dword ptr [dot2]
			fadd dword ptr [ecx+8]
			fsubp st(1), st(0)
			fld dword ptr [eax+4]
			fld dword ptr [dot1]
			fadd dword ptr [ecx+4]
			fsubp st(1), st(0)
			fstp dword ptr [radiusY]
			fld dword ptr [eax]
			fsub dword ptr [centerX]
			fstp dword ptr [radiusX]
			mov ecx, angular
			fld st(0)
			fmul dword ptr [ecx+4]
			fld dword ptr [radiusY]
			fmul dword ptr [ecx+8]
			fsubp st(1), st(0)
			fstp dword ptr [crossX]
			fld dword ptr [radiusX]
			fmul dword ptr [ecx+8]
			fxch st(1)
			fmul dword ptr [ecx]
			fsubp st(1), st(0)
			fstp dword ptr [crossY]
			fld dword ptr [radiusY]
			fmul dword ptr [ecx]
			fld dword ptr [radiusX]
			fmul dword ptr [ecx+4]
			fsubp st(1), st(0)
			mov ecx, velocity
			fadd dword ptr [ecx+8]
			fld dword ptr [crossY]
			fadd dword ptr [ecx+4]
			fld dword ptr [crossX]
			fadd dword ptr [ecx]
			mov ecx, destination
			fstp dword ptr [ecx]
			fstp dword ptr [ecx+4]
			fstp dword ptr [ecx+8]
		}
#else
		float center[3];
		for(unsigned row = 0; row < 3; ++row)
			center[row] = static_cast<float>(
				static_cast<double>(rotation[row * 3]) * mass[0] +
				static_cast<double>(rotation[row * 3 + 1]) * mass[1] +
				static_cast<double>(rotation[row * 3 + 2]) * mass[2] + position[row]);
		const float radius[3] = { point.x - center[0],
			point.y - center[1], point.z - center[2] };
		out.x = static_cast<float>(static_cast<double>(radius[2]) * angular[1] -
			static_cast<double>(radius[1]) * angular[2] + velocity[0]);
		out.y = static_cast<float>(static_cast<double>(radius[0]) * angular[2] -
			static_cast<double>(radius[2]) * angular[0] + velocity[1]);
		out.z = static_cast<float>(static_cast<double>(radius[1]) * angular[0] -
			static_cast<double>(radius[0]) * angular[1] + velocity[2]);
#endif
		}
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// The owned-shape update every pose and CMass-global setter ends with (000196-
// 000208: `mov ecx,[actor+0x14]; push 1; call 0x10001070`), phys_fn_000004 =
// nxForwardSubobjectCall (ObjectModel.cpp): slot 6 of [body+0x10] with 1, or
// nothing when the body has no shape. Every root the Scene builds carries its
// table: the four families' ObjectModel tables (slot 6 = 001315) and the
// group's own (Scene.cpp, slot 6 = phys_fn_001018: each child's slot 6, then
// 001315 on the group itself -- the group-level call Task 3 left open, S1).
static void nxNpActorNotifyOwnedShapes(unsigned char* body)
	{
	nxForwardSubobjectCall(body, reinterpret_cast<void*>(1));
	}

// phys_fn_000198 at 0x00008f60, actor dynamic slot 2. The body stores a
// cached pose for static actors; dynamic records keep a current and shadow
// translation and refresh their mass-frame center after position changes.
void NpActorVtable::setGlobalPosition(const NxVec3& position)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x232)) return;
	unsigned char* body = nxNpActorBody(this);
	if(body)
		{
		unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
		if(record)
			{
			memcpy(record + 0x50, &position, sizeof(position));
			memcpy(record + 0x18, record + 0x50, sizeof(position));
			nxNpActorMarkRecordDirty(record, 1);
			nxNpActorRefreshCMass(record);
			}
		else
			memcpy(body + 0x44, &position, sizeof(position));
		nxNpActorNotifyOwnedShapes(body);
		}
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000200 at 0x00009110, actor dynamic slot 3. The static body
// retains the nine matrix words; a dynamic record stores the SDK's
// matrix-to-quaternion conversion in its current and shadow poses.
void NpActorVtable::setGlobalOrientation(const NxMat33& orientation)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x242)) return;
	unsigned char* body = nxNpActorBody(this);
	if(body)
		{
		unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
		if(record)
			{
			// The same conversion as setGlobalPose (000196), not NxQuat(NxMat33).
			float quaternion[4];
			nxNpActorSetterQuaternionFromMatrix(reinterpret_cast<const float*>(&orientation), quaternion);
			memcpy(record + 0x5c, quaternion, sizeof(quaternion));
			memcpy(record + 0x24, record + 0x5c, sizeof(quaternion));
			nxNpActorMarkRecordDirty(record, 2);
			nxNpActorRefreshCMass(record);
			}
		else
			memcpy(body + 0x20, &orientation, sizeof(orientation));
		nxNpActorNotifyOwnedShapes(body);
		}
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000202 at 0x00009450, actor dynamic slot 4. Dynamic actors
// mirror the input quaternion into the current and shadow records; static
// actors store a row-major matrix on the outer body.
void NpActorVtable::setGlobalOrientationQuat(const NxQuat& orientation)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x254)) return;
	unsigned char* body = nxNpActorBody(this);
	if(body)
		{
		unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
		if(record)
			{
			memcpy(record + 0x5c, &orientation, sizeof(orientation));
			memcpy(record + 0x24, record + 0x5c, sizeof(orientation));
			nxNpActorMarkRecordDirty(record, 2);
			nxNpActorRefreshCMass(record);
			}
		else
			{
			float rows[9];
			nxNpActorRotationFromQuaternionGetter(&orientation.x, rows);
			memcpy(body + 0x20, rows, sizeof(rows));
			}
		nxNpActorNotifyOwnedShapes(body);
		}
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000130 at 0x00004580, actor vtable slot 5. It returns the same
// matrix and translation exposed by slots 7 and 6, respectively. The public
// drive checks all twelve words, including the quarter-turn precision case.
static NxVec3 nxNpActorGlobalPosition(const void* actor);
static NxMat33 nxNpActorGlobalOrientation(const void* actor);
// Under one read lock, the orientation and position forms inline (NG, Task 3).
NxMat34 NpActorVtable::getGlobalPoseVal() const
	{
	NxNpActorReadGuard guard(this);
	// The pose words are moved as integers (0x466d/0x4697 rep movsd, the
	// translation by mov), never through fld/fstp, which would quiet a
	// signalling NaN (final review I1).
	const NxMat33 orientation = nxNpActorGlobalOrientation(this);
	const NxVec3 position = nxNpActorGlobalPosition(this);
	NxMat34 pose(false);
	memcpy(&pose.M, &orientation, sizeof(pose.M));
	memcpy(&pose.t, &position, sizeof(pose.t));
	return pose;
	}

// phys_fn_000092 at 0x00002ed0, actor vtable slot 6. The oracle reads the
// nested pose translation when body+8 is non-null, otherwise the outer body's
// translation at +0x44. The final actor fallback covers incomplete setup.
// The body is not null-tested (0x2ede-0x2ee4).
static NxVec3 nxNpActorGlobalPosition(const void* actor)
	{
	const unsigned char* body = nxNpActorBody(const_cast<void*>(actor));
	const unsigned char* record = *reinterpret_cast<unsigned char* const*>(body + 0x08);
	NxVec3 result;
	memcpy(&result, record ? record + 0x50 : body + 0x44, sizeof(result));
	return result;
	}

// Under the read lock (NG, Task 3).
NxVec3 NpActorVtable::getGlobalPositionVal() const
	{
	NxNpActorReadGuard guard(this);
	return nxNpActorGlobalPosition(this);
	}

// phys_fn_000132 at 0x000046c0 keeps several quaternion products on the
// Win32 x87 stack while explicitly spilling others to single precision.
static void nxNpActorRotationFromQuaternionGetter(const float* q, float* rows)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	float yy, yz, xz, diagonal, xw;
	const float one = 1.0f;
	__asm {
		mov eax, q
		mov edx, rows
		fld dword ptr [eax+12]
		fld dword ptr [eax]
		fld dword ptr [eax+4]
		fld dword ptr [eax+8]
		fld st(1)
		fmul st, st(2)
		fadd st(0), st(0)
		fstp yy
		fld st(0)
		fmul st, st(1)
		fadd st(0), st(0)
		fld one
		fsub yy
		fsub st, st(1)
		fstp dword ptr [edx]
		fld st(2)
		fmul st, st(4)
		fadd st(0), st(0)
		fld st(2)
		fmul st, st(6)
		fadd st(0), st(0)
		fld st(1)
		fsub st, st(1)
		fstp dword ptr [edx+4]
		fld st(3)
		fmul st, st(6)
		fadd st(0), st(0)
		fstp yz
		fld st(4)
		fmul st, st(7)
		fadd st(0), st(0)
		fst xz
		fadd yz
		fstp dword ptr [edx+8]
		fadd st, st(1)
		fstp dword ptr [edx+12]
		fstp st(0)
		fld st(3)
		fmul st, st(4)
		fadd st(0), st(0)
		fsubr one
		fst diagonal
		fsub st, st(1)
		fstp dword ptr [edx+16]
		fstp st(0)
		fmul st, st(1)
		fadd st(0), st(0)
		fstp xw
		fstp st(0)
		fmulp st(1), st
		fadd st(0), st(0)
		fld xw
		fsub st, st(1)
		fstp dword ptr [edx+20]
		fld yz
		fsub xz
		fstp dword ptr [edx+24]
		fadd xw
		fstp dword ptr [edx+28]
		fld diagonal
		fsub yy
		fstp dword ptr [edx+32]
	}
#else
	const double x = q[0], y = q[1], z = q[2], w = q[3];
	rows[0] = static_cast<float>(1.0 - 2.0 * (y*y + z*z));
	rows[1] = static_cast<float>(2.0 * (x*y - w*z));
	rows[2] = static_cast<float>(2.0 * (x*z + w*y));
	rows[3] = static_cast<float>(2.0 * (x*y + w*z));
	rows[4] = static_cast<float>(1.0 - 2.0 * (x*x + z*z));
	rows[5] = static_cast<float>(2.0 * (y*z - w*x));
	rows[6] = static_cast<float>(2.0 * (x*z - w*y));
	rows[7] = static_cast<float>(2.0 * (y*z + w*x));
	rows[8] = static_cast<float>(1.0 - 2.0 * (x*x + y*y));
#endif
	}

// phys_fn_000132 at 0x000046c0, actor vtable slot 7. The dynamic arm
// converts the quaternion in the nested record; the static arm copies the
// outer body's matrix at +0x20. Lock behavior remains a separate dependency.
static NxMat33 nxNpActorGlobalOrientation(const void* actor)
	{
	const unsigned char* body = nxNpActorBody(const_cast<void*>(actor));
	const unsigned char* record = *reinterpret_cast<unsigned char* const*>(body + 0x08);
	NxMat33 orientation;
	if(record)
		{
		NxQuat quaternion;
		memcpy(&quaternion, record + 0x5c, sizeof(quaternion));
		// Preserve the x87 stack and float store points at RVA 0x46c0.
		float rows[9];
		nxNpActorRotationFromQuaternionGetter(&quaternion.x, rows);
		memcpy(&orientation, rows, sizeof(rows));
		}
	else
		memcpy(&orientation, body + 0x20, sizeof(orientation));
	return orientation;
	}

// Under the read lock (NG, Task 3).
NxMat33 NpActorVtable::getGlobalOrientationVal() const
	{
	NxNpActorReadGuard guard(this);
	return nxNpActorGlobalOrientation(this);
	}

// phys_fn_000094 at 0x00002f30, actor vtable slot 8. The dynamic arm
// copies the record's quaternion; the static arm converts the outer matrix.
// Under the read lock (NG, Task 3). The static arm (0x2f77-0x3109) converts
// the body's matrix at +0x20 with the pose setters' sequence ((m11 + m22)
// spilled, the z arm over float(s), the x and y arms with the reciprocal
// spilled): nxNpActorSetterQuaternionFromMatrix, whose (m22 + m11) is the same
// value.
NxQuat NpActorVtable::getGlobalOrientationQuatVal() const
	{
	NxNpActorReadGuard guard(this);
	const unsigned char* body = nxNpActorBody(const_cast<NpActorVtable*>(this));
	const unsigned char* record = *reinterpret_cast<unsigned char* const*>(body + 0x08);
	NxQuat quaternion;
	if(record)
		memcpy(&quaternion, record + 0x5c, sizeof(quaternion));
	else
		nxNpActorSetterQuaternionFromMatrix(
			reinterpret_cast<const float*>(body + 0x20), &quaternion.x);
	return quaternion;
	}

// phys_fn_000128 (0x00004430, 333 B)
const NxMat34 & NpActorVtable::getGlobalPoseReference() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	static bool warningIssued = false;
	if(!warningIssued)
		{
		warningIssued = true;
		NxFoundation::FoundationSDK::getInstance().error(static_cast<NxErrorCode>(0xd0),
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpActor.cpp", 0x2c0, 0,
			"Warning: deprecated method: Actor::getGlobalPoseReference().  Please use getGlobalPose() instead.\n");
		}
	unsigned char* body = nxNpActorBody(self);
	unsigned char* record = body
		? *reinterpret_cast<unsigned char**>(body + 8) : 0;
	if(record)
		{
		float rotation[9];
		nxNpActorRotationFromQuaternionGetter(
			reinterpret_cast<const float*>(record + 0x5c), rotation);
		memcpy(body + 0x20, rotation, sizeof(rotation));
		// 0x10004553-0x10004568: x and y go through fld/fstp (an SNaN comes
		// out quiet), z is a plain dword move.
		const unsigned char* from = record + 0x50;
		unsigned char* to = body + 0x44;
#if defined(_MSC_VER) && defined(_M_IX86)
		__asm {
			mov eax, from
			mov edx, to
			fld dword ptr [eax]
			fld dword ptr [eax+4]
			fxch st(1)
			fstp dword ptr [edx]
			fstp dword ptr [edx+4]
			mov ecx, dword ptr [eax+8]
			mov dword ptr [edx+8], ecx
		}
#else
		memcpy(to, from, sizeof(NxVec3));
#endif
		}
	nxNpSceneGuardLeave(ctx);
	return *reinterpret_cast<const NxMat34*>(body + 0x20);
	}

static float nxNpActorX87Dot3(const float& a, const float& b, const float& c,
	const float& d, const float& e, const float& f);

// `fadd dword ptr [addend]` onto a value the listing holds on the x87 stack,
// rounded to float at the store: the addend is read in place.
static float nxNpActorX87AddFloat(double value, const float& addend)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	float result;
	__asm {
		fld value
		mov eax, addend
		fadd dword ptr [eax]
		fstp result
	}
	return result;
#else
	return static_cast<float>(value + addend);
#endif
	}
static void nxNpActorWakeAfterCMassWrite(unsigned char* record);

// phys_fn_000784 (0x000194b0, 368 B)
// A row of gap:SceneRaycast.cpp..CapsuleShape.cpp (thiscall on the record,
// (position, quaternion), `ret 8`): the kinematic target writer the three
// moves call. The 0x20-byte block at [record+0x118] is written with no null
// test: a non-null position goes to +0..+8 and ORs 1 into +0xc, a non-null
// quaternion to +0x10..+0x1c and ORs 2. Then the wake every setter has
// (0x1950a-0x1961b): unless +0x114 & 0x100, an ordered +0x84 < 0.39999998f
// raises +0x84 and +0x4c to 0x3ecccccc and marks 0x10. noinline: the image
// calls it as its own function from each move (000090, 000124, 000126); the
// compiler had folded it into two of them (kept at the second merge of main
// into the scene-raycast block, which found the calls missing in its trace).
static __declspec(noinline) void nxNpActorSetKinematicTarget(unsigned char* record,
	const float* position, const float* quaternion)
	{
	if(position)
		{
		unsigned char* target = *reinterpret_cast<unsigned char**>(record + 0x118);
		memcpy(target, position, 3 * sizeof(float));
		*reinterpret_cast<unsigned*>(target + 0xc) |= 1u;
		}
	if(quaternion)
		{
		unsigned char* target = *reinterpret_cast<unsigned char**>(record + 0x118);
		memcpy(target + 0x10, quaternion, 4 * sizeof(float));
		*reinterpret_cast<unsigned*>(target + 0xc) |= 2u;
		}
	nxNpActorWakeAfterCMassWrite(record);
	}

// phys_fn_000124 (0x00003b40, 1075 B)
// The target is the mass frame's world pose, pose * {F = +0xdc, p = +0x100}:
// - 0x3bad-0x3c23: the centre M p + t. Row 0, (p0 m0 + p1 m1) + p2 m2, stays in
//   the register and has t.x added there; rows 1, (m5 p2 + p0 m3) + m4 p1, and
//   2, (p1 m7 + m6 p0) + p2 m8, are spilled to float before t.y/t.z are added;
// - 0x3c27-0x3d6c: G = M F, nine x87 dot products in the listing's operand
//   orders, each rounded once;
// - 0x3d7c-0x3f14: the quaternion of G by the 000801 conversion ((G8 + G4)
//   spilled, register reciprocals);
// - 0x3f16: 000784(position, quaternion).
void NpActorVtable::moveGlobalPose(const NxMat34& pose)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x28f)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(!record || (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		nxNpActorReport(0x291, "Actor::moveGlobalPose: Actor must be kinematic!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	float m[9];
	memcpy(m, &pose.M, sizeof(m));		// bit copy (I1)
	const float* f = reinterpret_cast<const float*>(record + 0xdc);
	const float* p = reinterpret_cast<const float*>(record + 0x100);
	const double c0 = (static_cast<double>(p[0]) * m[0] + static_cast<double>(p[1]) * m[1]) +
		static_cast<double>(p[2]) * m[2];
	const float c1 = static_cast<float>((static_cast<double>(m[5]) * p[2] +
		static_cast<double>(p[0]) * m[3]) + static_cast<double>(m[4]) * p[1]);
	const float c2 = static_cast<float>((static_cast<double>(p[1]) * m[7] +
		static_cast<double>(m[6]) * p[0]) + static_cast<double>(p[2]) * m[8]);
	// 0x3c12-0x3c20: each row is on the stack and takes `fadd dword ptr
	// [pose.t]` from memory, so a NaN row wins over a signalling t (I1).
	float position[3];
	position[0] = nxNpActorX87AddFloat(c0, pose.t.x);
	position[1] = nxNpActorX87AddFloat(c1, pose.t.y);
	position[2] = nxNpActorX87AddFloat(c2, pose.t.z);
	float g[9];
	g[0] = nxNpActorX87Dot3(f[3], m[1], f[0], m[0], m[2], f[6]);
	g[1] = nxNpActorX87Dot3(f[7], m[2], f[4], m[1], f[1], m[0]);
	g[2] = nxNpActorX87Dot3(f[5], m[1], m[0], f[2], f[8], m[2]);
	g[3] = nxNpActorX87Dot3(f[0], m[3], m[5], f[6], m[4], f[3]);
	g[4] = nxNpActorX87Dot3(f[1], m[3], m[5], f[7], m[4], f[4]);
	g[5] = nxNpActorX87Dot3(f[8], m[5], f[5], m[4], f[2], m[3]);
	g[6] = nxNpActorX87Dot3(f[0], m[6], m[8], f[6], f[3], m[7]);
	g[7] = nxNpActorX87Dot3(f[7], m[8], f[4], m[7], f[1], m[6]);
	g[8] = nxNpActorX87Dot3(m[6], f[2], f[8], m[8], f[5], m[7]);
	float quaternion[4];
	nxNpActorBodyQuaternionFromMatrix(g, quaternion);
	nxNpActorSetKinematicTarget(record, position, quaternion);
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000090 (0x00002df0, 211 B)
// 0x2e48-0x2e79: the target position is the input plus the local mass
// position +0x100 (not rotated), each sum rounded; then 000784(&v, 0).
void NpActorVtable::moveGlobalPosition(const NxVec3& position)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x29e)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(!record || (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorReport(0x2a1, "Actor::moveGlobalPosition: Actor must be kinematic!");
	else
		{
		const float* p = reinterpret_cast<const float*>(record + 0x100);
		float target[3];
		target[2] = p[2] + position.z;
		target[1] = p[1] + position.y;
		target[0] = p[0] + position.x;
		nxNpActorSetKinematicTarget(record, target, 0);
		}
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000126 (0x00003f80, 1192 B)
// The same composition as 000124 with the actor's current position +0x50
// as the translation, inline under this row's own lock (G1 0x2ac, E1 0x2ae),
// in its own operand orders:
// - 0x3fed-0x40a1: row 0, (m1 p1 + m2 p2) + m0 p0, in the register added to
//   +0x50; rows 1, (m4 p1 + m5 p2) + m3 p0, and 2, (m7 p1 + m8 p2) + m6 p0,
//   spilled to float before +0x54/+0x58 are added;
// - 0x409d-0x421e: G = M F, each element of row i summed as
//   (m[i][0] F[0][0] + m[i][1] F[1][0]) + m[i][2] F[2][0] in column 0 and
//   (m[i][1] F[1][j] + m[i][2] F[2][j]) + m[i][0] F[0][j] in columns 1 and 2;
// - 0x4231-0x43c9: the 000801 conversion of G; 0x43cb: 000784.
void NpActorVtable::moveGlobalOrientation(const NxMat33& orientation)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x2ac)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(!record || (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		nxNpActorReport(0x2ae, "Actor::moveGlobalOrientation: Actor must be kinematic!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	float m[9];
	memcpy(m, &orientation, sizeof(m));		// 0x3ffa rep movsd (I1)
	const float* f = reinterpret_cast<const float*>(record + 0xdc);
	const float* p = reinterpret_cast<const float*>(record + 0x100);
	const float* t = reinterpret_cast<const float*>(record + 0x50);
	const double c0 = (static_cast<double>(m[1]) * p[1] + static_cast<double>(m[2]) * p[2]) +
		static_cast<double>(m[0]) * p[0];
	const float c1 = static_cast<float>((static_cast<double>(m[4]) * p[1] +
		static_cast<double>(m[5]) * p[2]) + static_cast<double>(m[3]) * p[0]);
	const float c2 = static_cast<float>((static_cast<double>(m[7]) * p[1] +
		static_cast<double>(m[8]) * p[2]) + static_cast<double>(m[6]) * p[0]);
	float position[3];
	position[0] = static_cast<float>(static_cast<double>(t[0]) + c0);
	position[1] = static_cast<float>(static_cast<double>(t[1]) + c1);
	position[2] = static_cast<float>(static_cast<double>(t[2]) + c2);
	float g[9];
	for(unsigned row = 0; row < 3; ++row)
		{
		const float* a = m + row * 3;
		g[row * 3] = nxNpActorX87Dot3(a[0], f[0], a[1], f[3], a[2], f[6]);
		g[row * 3 + 1] = nxNpActorX87Dot3(a[1], f[4], a[2], f[7], a[0], f[1]);
		g[row * 3 + 2] = nxNpActorX87Dot3(a[1], f[5], a[2], f[8], a[0], f[2]);
		}
	float quaternion[4];
	nxNpActorBodyQuaternionFromMatrix(g, quaternion);
	nxNpActorSetKinematicTarget(record, position, quaternion);
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000070 (0x00002a80, 185 B)
// 0x2abc-0x2b03: the descriptor's own isValid() (vtable slot 2) is asked
// first; a false answer reports E1 line 0x1ac and returns 0 after unlocking.
// 0x2b06-0x2b36: Actor.cpp's createShape (000036) on the body [actor+0x14]
// with the descriptor; a built shape's public handle [shape+0x9c] is read
// before the unlock and returned, else 0.
NxShape* NpActorVtable::createShape(const NxShapeDesc& descriptor)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x1ab)) return 0;
	if(!descriptor.isValid())
		{
		nxNpActorReport(0x1ac, "Actor::createShape: desc.isValid() fails!");
		nxNpSceneGuardLeave(ctx);
		return 0;
		}
	unsigned char* shape = nxActorCreateShape(nxNpActorBody(this), &descriptor);
	NxShape* handle = shape ? *reinterpret_cast<NxShape**>(shape + 0x9c) : 0;
	nxNpSceneGuardLeave(ctx);
	return handle;
	}

// phys_fn_000072 (0x00002b40, 90 B)
// 0x2b7a-0x2b90: Actor.cpp's releaseShape (000024) on the body with the
// INTERNAL shape the public handle holds at +8, then the unlock.
void NpActorVtable::releaseShape(NxShape& shape)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x1b3)) return;
	nxActorReleaseShape(nxNpActorBody(this),
		*reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(&shape) + 8));
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000082 (0x00002d00) delegates to the outer body's shape holder.
// A single shape contributes one public handle; kind 5 is the group whose
// child array at +0xe0/+0xe4 determines the count.
// Under the read lock (NG, Task 3).
NxU32 NpActorVtable::getNbShapes() const
	{
	NxNpActorReadGuard guard(this);
	const unsigned char* actor = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(actor + 0x14);
	const unsigned char* shape = body
		? *reinterpret_cast<unsigned char* const*>(body + 0x10) : 0;
	if(!shape) return 0;
	if(*reinterpret_cast<const unsigned*>(shape + 0xd0) != 5) return 1;
	const void* const* first = *reinterpret_cast<void* const* const*>(shape + 0xe0);
	const void* const* last = *reinterpret_cast<void* const* const*>(shape + 0xe4);
	return first ? static_cast<NxU32>(last - first) : 0;
	}

// phys_fn_000084 (0x00002d30) returns the address of the single helper
// pointer, or the group's parallel array of public helper pointers.
// Under the read lock (NG, Task 3).
NxShape** NpActorVtable::getShapes() const
	{
	NxNpActorReadGuard guard(this);
	const unsigned char* actor = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(actor + 0x14);
	const unsigned char* shape = body
		? *reinterpret_cast<unsigned char* const*>(body + 0x10) : 0;
	if(!shape) return 0;
	if(*reinterpret_cast<const unsigned*>(shape + 0xd0) == 5)
		return *reinterpret_cast<NxShape** const*>(shape + 0xf0);
	return reinterpret_cast<NxShape**>(const_cast<unsigned char*>(shape + 0x9c));
	}

// phys_fn_000164 (0x00006520, 1846 B)
// updateMassFromShapes (slot 17). After the write lock (G1 0x98): a density
// or total mass below zero, or unordered (`test ah,1`), E1 0x9a; no record,
// E1 0x9d; no shapes (body +0x10), E1 0x9e; both zero (fucompp, -0.0 is
// zero), E1 0x9f; both nonzero, E1 0xa0. Then Actor.cpp's 000008 on the body
// with the density, the total mass's own argument slot (in and out), an
// identity pose and an unset diagonal; 1 is E1 0xa8 and any other nonzero
// E1 0xa9. Every E1 unlocks. On success, in the listing's order:
// +0x188 = the mass (integer copy), +0xc0 = (float)(1.0 / mass) with no
// test, mark 0x10000; +0x18c.. = the diagonal, its three float inverses
// classified by _fpclass (005666) in order, all zeroed when one is NaN or
// infinite (0x207), mark 0x20000; +0x100.. = pose.t, mark 0x200, +++0x198;
// +0xdc.. = pose.M (nine words), mark 0x400, +++0x198; 000768; unlock. No
// wake and no kinematic test.
int nxActorComputeMassFromShapes(unsigned char* body, float density, float* totalMass,
	NxMat34* pose, NxVec3* diagonal);

void NpActorVtable::updateMassFromShapes(NxReal density, NxReal totalMass)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x98)) return;
	if(!(density >= 0.0f) || !(totalMass >= 0.0f))
		{
		nxNpActorReport(0x9a, "Actor::updateMassFromShapes: density and total Mass of a "
			"shape have to be nonnegative!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	unsigned char* body = nxNpActorBody(this);
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	if(!record)
		{
		nxNpActorReport(0x9d, "Actor::updateMassFromShapes: Actor must be dynamic!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	if(!*reinterpret_cast<void**>(body + 0x10))
		{
		nxNpActorReport(0x9e, "Actor::updateMassFromShapes: Actor must have shapes!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	if(density == 0.0f && totalMass == 0.0f)
		{
		nxNpActorReport(0x9f, "Actor::updateMassFromShapes: density or total mass must "
			"be nonzero!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	if(density != 0.0f && totalMass != 0.0f)
		{
		nxNpActorReport(0xa0, "Actor::updateMassFromShapes: density and total mass may "
			"not both be nonzero!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	NxMat34 pose;
	static const unsigned identity[12] = {
		0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0 };
	memcpy(&pose, identity, sizeof(identity));
	NxVec3 diagonal;
	const int result = nxActorComputeMassFromShapes(body, density, &totalMass, &pose,
		&diagonal);
	if(result == 1)
		{
		nxNpActorReport(0xa8, "Actor::updateMassFromShapes: Compute mesh inertia tensor "
			"failed for one of the actor's mesh shapes! Please change mesh geometry or "
			"supply a tensor manually!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	if(result)
		{
		nxNpActorReport(0xa9, "Actor::updateMassFromShapes: Can't compute mass from "
			"shapes: must have at least one non-trigger shape!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	memcpy(record + 0x188, &totalMass, 4);
	*reinterpret_cast<float*>(record + 0xc0) = static_cast<float>(1.0 / totalMass);
	nxNpActorMarkRecordDirty(record, 0x10000);
	memcpy(record + 0x18c, &diagonal, sizeof(diagonal));
	const float inverseX = 1.0f / diagonal.x;
	const float inverseY = 1.0f / diagonal.y;
	const float inverseZ = 1.0f / diagonal.z;
	float* inverse = reinterpret_cast<float*>(record + 0xc4);
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
	nxNpActorMarkRecordDirty(record, 0x20000);
	memcpy(record + 0x100, &pose.t, sizeof(NxVec3));
	nxNpActorMarkRecordDirty(record, 0x200);
	++*reinterpret_cast<unsigned*>(record + 0x198);
	memcpy(record + 0xdc, &pose.M, 0x24);
	nxNpActorMarkRecordDirty(record, 0x400);
	++*reinterpret_cast<unsigned*>(record + 0x198);
	nxNpActorRefreshCMass(record);
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000122 (0x00003840, 761 B)
// setDynamic (slot 18). After the write lock (G1 0x5b): a negative mass
// (`test ah,5; jnp`: a NaN mass passes) or a massLocalPose word that
// _fpclass (005666, the twelve words in order) finds NaN or infinite is
// E1 0x63; a body without shapes whose tensor is all zero bits is E1 0x66.
// A static body with a shape takes it out of the Scene first (000533) and
// remembers to add it back. 000026 on the body builds and installs the new
// record: 1 is E1 0x7c and any other nonzero result E1 0x7d, both leaving the
// removed shape out of the Scene. With an old record: 000632 removes it from
// the Scene, it notifies its observers with 0x100 (the Foundation import),
// 000776 destroys it and [0x101041bc] frees it. Then 000531 adds the shape
// back as a dynamic one, and the lock is released. Every E1 unlocks.
void nxSceneAddShape(NxSceneInternal* scene, unsigned char* shape, bool hasRecord);
bool nxSceneRemoveStaticShape(NxSceneInternal* scene, unsigned char* shape);
void nxSceneRemoveBody(NxSceneInternal* scene, unsigned char* record);
int nxActorBuildRecord(unsigned char* body, const NxBodyDesc* desc);

void NpActorVtable::setDynamic(const NxBodyDesc& desc)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x5b)) return;
	unsigned char* body = nxNpActorBody(this);
	unsigned char* oldRecord = *reinterpret_cast<unsigned char**>(body + 8);
	bool valid = !(desc.mass < 0.0f);
	const float* pose = reinterpret_cast<const float*>(&desc.massLocalPose);
	for(unsigned i = 0; valid && i < 12; ++i)
		if(_fpclass(pose[i]) & 0x207)
			valid = false;
	if(!valid)
		{
		nxNpActorReport(0x63, "Actor::setDynamic: desc.isValid() fails!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	unsigned char* shape = *reinterpret_cast<unsigned char**>(body + 0x10);
	const unsigned* tensor = reinterpret_cast<const unsigned*>(&desc.massSpaceInertia);
	if(!shape && !tensor[0] && !tensor[1] && !tensor[2])
		{
		nxNpActorReport(0x66, "Actor::setDynamic: we need a valid massSpaceInertia if the "
			"actor has no shapes!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	NxSceneInternal* scene = *reinterpret_cast<NxSceneInternal**>(body + 4);
	bool readd = false;
	if(!oldRecord && shape)
		{
		nxSceneRemoveStaticShape(scene, shape);
		readd = true;
		}
	const int result = nxActorBuildRecord(body, &desc);
	if(result == 1)
		{
		nxNpActorReport(0x7c, "Actor::setDynamic: Compute mesh inertia tensor failed for one "
			"of the actor's mesh shapes! Please change mesh geometry or supply a tensor "
			"manually!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	if(result)
		{
		nxNpActorReport(0x7d, "Actor::setDynamic: Can't compute mass from shapes: must have "
			"at least one non-trigger shape!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	if(oldRecord)
		{
		nxSceneRemoveBody(scene, oldRecord);
		reinterpret_cast<NxFoundation::Observable*>(oldRecord)->notifyObservers(0x100);
		reinterpret_cast<DynamicBody*>(oldRecord)->destruct();
		nxFoundationSDKAllocator->free(oldRecord);
		}
	if(readd)
		nxSceneAddShape(scene, shape, true);
	nxNpSceneGuardLeave(ctx);
	}

// The mass-frame refresh every pose and CMass-offset setter calls last:
// Row 000768 (000196, 000198, 000200, 000202, 000210, 000212, 000214,
// 000218, 000220 and 000222 each `call 0x10017f10` with ecx = the record,
// after their stores and +0x198 increments and before the wake test). It
// writes +0x134, +0x158, +0x124 and +0x164. Joint-open-items Task 4 routed
// this through nxNpActorUpdateMassFrame, the listing's reproduction; the
// earlier sequence was one bit off on rotated bodies.
static void nxNpActorRefreshCMass(unsigned char* record)
	{
	nxNpActorUpdateMassFrame(record);
	}

static void nxNpActorWakeAfterCMassWrite(unsigned char* record)
	{
	if((*reinterpret_cast<unsigned*>(record + 0x114) & 0x100u) == 0 &&
		*reinterpret_cast<float*>(record + 0x84) < 0.39999998f)
		{
		*reinterpret_cast<unsigned*>(record + 0x84) = 0x3eccccccu;
		*reinterpret_cast<unsigned*>(record + 0x4c) = 0x3eccccccu;
		nxNpActorMarkRecordDirty(record, 0x10);
		}
	}

// phys_fn_000210 (0x00009cc0, 998 B)
void NpActorVtable::setCMassOffsetLocalPose(const NxMat34& pose)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x387)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0x100, &pose.t, sizeof(NxVec3));
		nxNpActorMarkRecordDirty(record, 0x200);
		++*reinterpret_cast<unsigned*>(record + 0x198);
		memcpy(record + 0xdc, &pose.M, sizeof(NxMat33));		// 0x9e50 rep movsd (I1)
		nxNpActorMarkRecordDirty(record, 0x400);
		++*reinterpret_cast<unsigned*>(record + 0x198);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	else
		nxNpActorReport(0x388, "Actor::setCMassOffsetLocalPose: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000212 (0x0000a0b0, 739 B)
void NpActorVtable::setCMassOffsetLocalPosition(const NxVec3& position)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x393)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0x100, &position, sizeof(NxVec3));
		nxNpActorMarkRecordDirty(record, 0x200);
		++*reinterpret_cast<unsigned*>(record + 0x198);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	else
		nxNpActorReport(0x394, "Actor::setCMassOffsetLocalPosition: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000214 (0x0000a3a0, 557 B)
// phys_fn_000216 (0x0000a5d0, 163 B)
// 000214's tail (the wake's dirty-list growth loop, the epilogue and the
// E1 0x39f report), entered only by jumps from 000214: it is part of this
// function, not a function of its own.
void NpActorVtable::setCMassOffsetLocalOrientation(const NxMat33& orientation)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x39e)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0xdc, &orientation, sizeof(NxMat33));		// 0xa419 rep movsd (I1)
		nxNpActorMarkRecordDirty(record, 0x400);
		++*reinterpret_cast<unsigned*>(record + 0x198);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	else
		nxNpActorReport(0x39f, "Actor::setCMassOffsetLocalOrientation: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// The global CMass-offset setters (joint-open-items Task 4, from the
// listing). Each forms the body rotation R from +0x5c with the shared inline
// sequence (nxNpActorComposeRotation) and stores R^T (world - t) at +0x100
// or R^T W at +0xdc. The three rows sum each element in their own order:
// - setCMassOffsetGlobalPosition 000220 (0xae07-0xae97) and the position half
//   of setCMassOffsetGlobalPose 000218 (0xa7c6-0xa95c);
// - setCMassOffsetGlobalOrientation 000222 (0xb234-0xb345) and the
//   orientation half of 000218 (0xa83f-0xa94d).
// In every row x - t.x and z - t.z are spilled to float and y - t.y stays in
// the register; each element is one register sum rounded at the store.
static void nxNpActorStoreGlobalMassPosition(unsigned char* record,
	const NxVec3& worldPosition, bool poseRow)
	{
	float r[9];
	nxNpActorComposeRotation(reinterpret_cast<const float*>(record + 0x5c), r);
	const float* t = reinterpret_cast<const float*>(record + 0x50);
	const float dx = static_cast<float>(static_cast<double>(worldPosition.x) - t[0]);
	const double dy = static_cast<double>(worldPosition.y) - t[1];
	const float dz = static_cast<float>(static_cast<double>(worldPosition.z) - t[2]);
	#define NX_P(a, v) (static_cast<double>(r[a]) * (v))
	float* local = reinterpret_cast<float*>(record + 0x100);
	for(unsigned c = 0; c < 3; ++c)
		local[c] = poseRow
			? static_cast<float>((NX_P(c, dx) + NX_P(6 + c, dz)) + NX_P(3 + c, dy))
			: static_cast<float>((NX_P(6 + c, dz) + NX_P(3 + c, dy)) + NX_P(c, dx));
	#undef NX_P
	nxNpActorMarkRecordDirty(record, 0x200);
	++*reinterpret_cast<unsigned*>(record + 0x198);
	}

static void nxNpActorStoreGlobalMassOrientation(unsigned char* record,
	const NxMat33& worldOrientation, bool poseRow)
	{
	float r[9];
	nxNpActorComposeRotation(reinterpret_cast<const float*>(record + 0x5c), r);
	float w[9];
	memcpy(w, &worldOrientation, sizeof(w));
	float l[9];
	#define NX_O(a, b) (static_cast<double>(r[a]) * w[b])
	if(poseRow)
		{
		l[0] = static_cast<float>((NX_O(6, 6) + NX_O(3, 3)) + NX_O(0, 0));
		l[1] = static_cast<float>((NX_O(0, 1) + NX_O(6, 7)) + NX_O(3, 4));
		l[2] = static_cast<float>((NX_O(6, 8) + NX_O(3, 5)) + NX_O(0, 2));
		l[3] = static_cast<float>((NX_O(1, 0) + NX_O(7, 6)) + NX_O(4, 3));
		l[4] = static_cast<float>((NX_O(1, 1) + NX_O(7, 7)) + NX_O(4, 4));
		l[5] = static_cast<float>((NX_O(1, 2) + NX_O(7, 8)) + NX_O(4, 5));
		l[6] = static_cast<float>((NX_O(2, 0) + NX_O(8, 6)) + NX_O(5, 3));
		l[7] = static_cast<float>((NX_O(2, 1) + NX_O(8, 7)) + NX_O(5, 4));
		l[8] = static_cast<float>((NX_O(2, 2) + NX_O(8, 8)) + NX_O(5, 5));
		}
	else
		{
		l[0] = static_cast<float>((NX_O(0, 0) + NX_O(3, 3)) + NX_O(6, 6));
		l[1] = static_cast<float>((NX_O(3, 4) + NX_O(6, 7)) + NX_O(0, 1));
		l[2] = static_cast<float>((NX_O(0, 2) + NX_O(3, 5)) + NX_O(6, 8));
		l[3] = static_cast<float>((NX_O(1, 0) + NX_O(4, 3)) + NX_O(7, 6));
		l[4] = static_cast<float>((NX_O(4, 4) + NX_O(7, 7)) + NX_O(1, 1));
		l[5] = static_cast<float>((NX_O(4, 5) + NX_O(7, 8)) + NX_O(1, 2));
		l[6] = static_cast<float>((NX_O(2, 0) + NX_O(5, 3)) + NX_O(8, 6));
		l[7] = static_cast<float>((NX_O(5, 4) + NX_O(8, 7)) + NX_O(2, 1));
		l[8] = static_cast<float>((NX_O(5, 5) + NX_O(8, 8)) + NX_O(2, 2));
		}
	#undef NX_O
	memcpy(record + 0xdc, l, sizeof(l));
	nxNpActorMarkRecordDirty(record, 0x400);
	++*reinterpret_cast<unsigned*>(record + 0x198);
	}

// phys_fn_000218 (0x0000a680, 1614 B)
void NpActorVtable::setCMassOffsetGlobalPose(const NxMat34& pose)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x3ab)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		nxNpActorStoreGlobalMassPosition(record, pose.t, true);
		nxNpActorStoreGlobalMassOrientation(record, pose.M, true);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	else
		nxNpActorReport(0x3ac, "Actor::setCMassOffsetGlobalPose: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000220 (0x0000acd0, 1059 B)
void NpActorVtable::setCMassOffsetGlobalPosition(const NxVec3& position)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x3b7)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		nxNpActorStoreGlobalMassPosition(record, position, false);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	else
		nxNpActorReport(0x3b8, "Actor::setCMassOffsetGlobalPosition: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000222 (0x0000b100, 1187 B)
void NpActorVtable::setCMassOffsetGlobalOrientation(const NxMat33& orientation)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x3c0)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		nxNpActorStoreGlobalMassOrientation(record, orientation, false);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	else
		nxNpActorReport(0x3c1, "Actor::setCMassOffsetGlobalOrientation: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

static void nxNpActorApplyWorldMassPose(unsigned char* record);

// phys_fn_000204 (0x000096c0, 520 B)
// The static/kinematic check and its E1 report (line 0x268) come before the
// write lock is tried, so that arm never locks or unlocks.
void NpActorVtable::setCMassGlobalPose(const NxMat34& pose)
	{
	unsigned char* record = nxNpActorRecord(this);
	if(!record || (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u))
		{
		nxNpActorReport(0x268, "Actor::setCMassGlobalPose: Actor must be (non-kinematic) dynamic!");
		return;
		}
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x269)) return;
	memcpy(record + 0x158, &pose.t, sizeof(pose.t));
	memcpy(record + 0x134, &pose.M, sizeof(NxMat33));		// 0x975b rep movsd (I1)
	nxNpActorUpdateCMassQuaternion(record);
	nxNpActorApplyWorldMassPose(record);
	nxNpActorWakeAfterCMassWrite(record);
	nxNpActorNotifyOwnedShapes(nxNpActorBody(this));		// 0x987d: 000004(body, 1)
	nxNpSceneGuardLeave(ctx);
	}

// The operands are references so that each fld and fmul reads the caller's
// float in place, as the listings' `fld dword ptr [m]; fmul dword ptr [n]`
// do: a by-value float is copied through fld/fstp under /arch:IA32, which
// quiets a signalling NaN before the multiply sees it (final review I1).
static float nxNpActorX87Dot3(const float& a, const float& b, const float& c,
	const float& d, const float& e, const float& f)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	float result;
	__asm {
		mov eax, a
		mov edx, b
		fld dword ptr [eax]
		fmul dword ptr [edx]
		mov eax, c
		mov edx, d
		fld dword ptr [eax]
		fmul dword ptr [edx]
		faddp st(1), st(0)
		mov eax, e
		mov edx, f
		fld dword ptr [eax]
		fmul dword ptr [edx]
		faddp st(1), st(0)
		fstp result
	}
	return result;
#else
	return static_cast<float>(static_cast<double>(a) * b +
		static_cast<double>(c) * d + static_cast<double>(e) * f);
#endif
	}

static float nxNpActorX87MassPositionX(float actorX, const float* rotation,
	const float* local)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	float result;
	__asm {
		mov eax, rotation
		mov edx, local
		fld actorX
		fld dword ptr [eax+4]
		fmul dword ptr [edx+4]
		fld dword ptr [eax+8]
		fmul dword ptr [edx+8]
		faddp st(1), st
		fld dword ptr [eax]
		fmul dword ptr [edx]
		faddp st(1), st
		faddp st(1), st
		fstp result
	}
	return result;
#else
	return static_cast<float>(actorX +
		static_cast<double>(rotation[1]) * local[1] +
		static_cast<double>(rotation[2]) * local[2] +
		static_cast<double>(rotation[0]) * local[0]);
#endif
	}

// phys_fn_000789 (0x00019d00, 1461 B)
// A row of gap:SceneRaycast.cpp..CapsuleShape.cpp (thiscall on the record),
// the world-mass-pose apply 000204-000208 call after their stores. In the
// listing's order:
// - 0x19d09-0x19d1e: +0x164 = the 000746 world tensor of diag(+0xc4) and the
//   world mass frame W (+0x134);
// - 0x19d23-0x19e5a: the actor rotation A = W F^T (F the local mass frame at
//   +0xdc), nine x87 dot products rounded once each;
// - 0x19e5e-0x19ef7: the actor position t = w - A p (w the world centre at
//   +0x158, p the local mass position at +0x100). Row 0's (A2 p2 + A1 p1) +
//   A0 p0 stays in the register; rows 1 and 2 are spilled to float first.
//   t goes to +0x50 and +0x18; dirty |= 1;
// - 0x19fe1-0x1a2ae: the quaternion of A by the setter conversion, to +0x5c
//   and +0x24; dirty |= 2.
// It does not call 000768.
static void nxNpActorApplyWorldMassPose(unsigned char* record)
	{
	const float* worldMass = reinterpret_cast<const float*>(record + 0x134);
	const float* localMass = reinterpret_cast<const float*>(record + 0xdc);
	const float* localPosition = reinterpret_cast<const float*>(record + 0x100);
	const float* worldPosition = reinterpret_cast<const float*>(record + 0x158);
	nxNpActorWorldTensorRDRt(reinterpret_cast<const float*>(record + 0xc4),
		worldMass, reinterpret_cast<float*>(record + 0x164));
	float a[9];
	a[0] = nxNpActorX87Dot3(localMass[1], worldMass[1], worldMass[2], localMass[2], worldMass[0], localMass[0]);
	a[1] = nxNpActorX87Dot3(localMass[4], worldMass[1], localMass[3], worldMass[0], localMass[5], worldMass[2]);
	a[2] = nxNpActorX87Dot3(localMass[6], worldMass[0], worldMass[2], localMass[8], worldMass[1], localMass[7]);
	a[3] = nxNpActorX87Dot3(worldMass[3], localMass[0], localMass[1], worldMass[4], worldMass[5], localMass[2]);
	a[4] = nxNpActorX87Dot3(worldMass[5], localMass[5], worldMass[4], localMass[4], worldMass[3], localMass[3]);
	a[5] = nxNpActorX87Dot3(worldMass[5], localMass[8], worldMass[4], localMass[7], worldMass[3], localMass[6]);
	a[6] = nxNpActorX87Dot3(worldMass[6], localMass[0], localMass[1], worldMass[7], worldMass[8], localMass[2]);
	a[7] = nxNpActorX87Dot3(worldMass[8], localMass[5], worldMass[7], localMass[4], worldMass[6], localMass[3]);
	a[8] = nxNpActorX87Dot3(worldMass[8], localMass[8], worldMass[7], localMass[7], worldMass[6], localMass[6]);
	#define NX_AP(i, k) (static_cast<double>(a[i]) * localPosition[k])
	const double d0 = (NX_AP(2, 2) + NX_AP(1, 1)) + NX_AP(0, 0);
	const float d1 = static_cast<float>((NX_AP(5, 2) + NX_AP(4, 1)) + NX_AP(3, 0));
	const float d2 = static_cast<float>((NX_AP(8, 2) + NX_AP(7, 1)) + NX_AP(6, 0));
	#undef NX_AP
	float actorPosition[3];
	actorPosition[0] = static_cast<float>(static_cast<double>(worldPosition[0]) - d0);
	actorPosition[1] = static_cast<float>(static_cast<double>(worldPosition[1]) - d1);
	actorPosition[2] = static_cast<float>(static_cast<double>(worldPosition[2]) - d2);
	memcpy(record + 0x50, actorPosition, sizeof(actorPosition));
	memcpy(record + 0x18, actorPosition, sizeof(actorPosition));
	nxNpActorMarkRecordDirty(record, 1);
	float actorQuaternion[4];
	nxNpActorSetterQuaternionFromMatrix(a, actorQuaternion);
	memcpy(record + 0x5c, actorQuaternion, sizeof(actorQuaternion));
	memcpy(record + 0x24, actorQuaternion, sizeof(actorQuaternion));
	nxNpActorMarkRecordDirty(record, 2);
	}

// The getters' world mass rotation W = R F: R from the body quaternion +0x5c
// by the ROT sequence every getter inlines, F the local mass frame +0xdc, and
// each element one x87 dot product rounded once. The rows differ only in the
// operand order of each column's sum (a = row i of R, f = F row-major):
//   134 (000134, 000142): (a0f0 + a1f3) + a2f6, (a1f4 + a2f7) + a0f1,
//                         (a0f2 + a1f5) + a2f8
//   138 (000138):         (a0f0 + a1f3) + a2f6, (a0f1 + a1f4) + a2f7,
//                         (a1f5 + a2f8) + a0f2
//   140 (000140):         (a1f3 + a2f6) + a0f0, (a0f1 + a1f4) + a2f7,
//                         (a0f2 + a1f5) + a2f8
//   144 (000144):         as 140, except row 0's third column, which is
//                         (a1f5 + a2f8) + a0f2
// (000134 0x49c3-0x4b34, 000138 0x4e78-0x4fce, 000140 0x5117-0x527e, 000142
// 0x53e9-0x5550, 000144 0x56d3-0x583b.)
static void nxNpActorWorldMassRotation(const unsigned char* record,
	NxNpActorRfOrder order, float* worldMass)
	{
	float actorRotation[9];
	nxNpActorRotationFromQuaternionGetter(
		reinterpret_cast<const float*>(record + 0x5c), actorRotation);
	const float* f = reinterpret_cast<const float*>(record + 0xdc);
	for(unsigned row = 0; row < 3; ++row)
		{
		const float* a = actorRotation + row * 3;
		float* w = worldMass + row * 3;
		switch(order)
			{
			case NX_RF_134:
				w[0] = nxNpActorX87Dot3(a[0], f[0], a[1], f[3], a[2], f[6]);
				w[1] = nxNpActorX87Dot3(a[1], f[4], a[2], f[7], a[0], f[1]);
				w[2] = nxNpActorX87Dot3(a[0], f[2], a[1], f[5], a[2], f[8]);
				break;
			case NX_RF_138:
				w[0] = nxNpActorX87Dot3(a[0], f[0], a[1], f[3], a[2], f[6]);
				w[1] = nxNpActorX87Dot3(a[0], f[1], a[1], f[4], a[2], f[7]);
				w[2] = nxNpActorX87Dot3(a[1], f[5], a[2], f[8], a[0], f[2]);
				break;
			case NX_RF_140:
			case NX_RF_144:
				w[0] = nxNpActorX87Dot3(a[1], f[3], a[2], f[6], a[0], f[0]);
				w[1] = nxNpActorX87Dot3(a[0], f[1], a[1], f[4], a[2], f[7]);
				w[2] = order == NX_RF_144 && row == 0
					? nxNpActorX87Dot3(a[1], f[5], a[2], f[8], a[0], f[2])
					: nxNpActorX87Dot3(a[0], f[2], a[1], f[5], a[2], f[8]);
				break;
			}
		}
	}

static const unsigned char* nxNpActorDerivedMassFrame(
	const unsigned char* record, unsigned char* scratch, NxNpActorRfOrder order)
	{
	if(!record) return 0;
	memcpy(scratch, record, 0x260);
	float actorRotation[9];
	nxNpActorRotationFromQuaternionGetter(
		reinterpret_cast<const float*>(record + 0x5c), actorRotation);
	nxNpActorWorldMassRotation(record, order, reinterpret_cast<float*>(scratch + 0x134));
	const float* localPosition = reinterpret_cast<const float*>(record + 0x100);
	const float* actorPosition = reinterpret_cast<const float*>(record + 0x50);
	float* worldPosition = reinterpret_cast<float*>(scratch + 0x158);
	for(unsigned row = 0; row < 3; ++row)
		{
		const float* a = actorRotation + row * 3;
		if(row == 0)
			{
			worldPosition[row] = nxNpActorX87MassPositionX(actorPosition[row],
				a, localPosition);
			continue;
			}
		const float displacement = nxNpActorX87Dot3(a[1], localPosition[1],
			a[2], localPosition[2], a[0], localPosition[0]);
		worldPosition[row] = static_cast<float>(static_cast<double>(actorPosition[row]) + displacement);
		}
	return scratch;
	}

// phys_fn_000206 (0x000098d0, 504 B)
// The static/kinematic check and its E1 report (line 0x276) come before the
// write lock is tried, so that arm never locks or unlocks.
void NpActorVtable::setCMassGlobalPosition(const NxVec3& position)
	{
	unsigned char* record = nxNpActorRecord(this);
	if(!record || (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u))
		{
		nxNpActorReport(0x276, "Actor::setCMassGlobalPosition: Actor must be (non-kinematic) dynamic!");
		return;
		}
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x277)) return;
	memcpy(record + 0x158, &position, sizeof(position));
	nxNpActorApplyWorldMassPose(record);
	nxNpActorWakeAfterCMassWrite(record);
	nxNpActorNotifyOwnedShapes(nxNpActorBody(this));		// 0x9a7f: 000004(body, 1)
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000208 (0x00009ad0, 492 B)
// The static/kinematic check and its E1 report (line 0x281) come before the
// write lock is tried, so that arm never locks or unlocks.
void NpActorVtable::setCMassGlobalOrientation(const NxMat33& orientation)
	{
	unsigned char* record = nxNpActorRecord(this);
	if(!record || (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u))
		{
		nxNpActorReport(0x281, "Actor::setCMassGlobalOrientation: Actor must be (non-kinematic) dynamic!");
		return;
		}
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x282)) return;
	memcpy(record + 0x134, &orientation, sizeof(NxMat33));		// 0x9b4c rep movsd (I1)
	nxNpActorUpdateCMassQuaternion(record);
	nxNpActorApplyWorldMassPose(record);
	nxNpActorWakeAfterCMassWrite(record);
	nxNpActorNotifyOwnedShapes(nxNpActorBody(this));		// 0x9c71: 000004(body, 1)
	nxNpSceneGuardLeave(ctx);
	}

static NxMat33 nxNpActorCMassMatrix(const unsigned char* record, unsigned offset)
	{
	NxMat33 result(NX_IDENTITY_MATRIX);
	if(record) memcpy(&result, record + offset, sizeof(result));
	return result;
	}

static NxVec3 nxNpActorCMassPosition(const unsigned char* record, unsigned offset)
	{
	NxVec3 result(0.0f, 0.0f, 0.0f);
	if(record) memcpy(&result, record + offset, sizeof(result));
	return result;
	}

// phys_fn_000096 (0x00003140, 179 B)
NxMat34 NpActorVtable::getCMassLocalPoseVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	const unsigned char* record = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	if(!record)
		nxNpActorReport(0x2f2, "Actor::getCMassLocalPose: Cannot be called on a static actor!");
	// 0x31bc-0x31e2: rep movsd and three movs, bit copies (final review I1);
	// the static fallback is the identity and a zero translation.
	NxMat34 result;
	if(record)
		{
		memcpy(&result.M, record + 0xdc, sizeof(result.M));
		memcpy(&result.t, record + 0x100, sizeof(result.t));
		}
	nxNpSceneGuardLeave(ctx);
	return result;
	}

// phys_fn_000098 (0x00003200, 151 B)
NxVec3 NpActorVtable::getCMassLocalPositionVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	const unsigned char* record = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	if(!record)
		nxNpActorReport(0x2fa, "Actor::getCMassLocalPosition: Cannot be called on a static actor!");
	NxVec3 result = nxNpActorCMassPosition(record, 0x100);
	nxNpSceneGuardLeave(ctx);
	return result;
	}

// phys_fn_000100 (0x000032a0, 108 B)
NxMat33 NpActorVtable::getCMassLocalOrientationVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	const unsigned char* record = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	if(!record)
		nxNpActorReport(0x301, "Actor::getCMassLocalOrientation: Cannot be called on a static actor!");
	NxMat33 result = nxNpActorCMassMatrix(record, 0xdc);
	nxNpSceneGuardLeave(ctx);
	return result;
	}

// phys_fn_000134 (0x000047d0, 907 B)
NxMat34 NpActorVtable::getCMassGlobalPoseVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char scratch[0x260];
	const unsigned char* source = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	if(!source)
		nxNpActorReport(0x30a, "Actor::getCMassGlobalPose: Cannot be called on a static actor!");
	const unsigned char* record = nxNpActorDerivedMassFrame(source, scratch, NX_RF_134);
	// The frame is copied out as integers (final review I1).
	NxMat34 result;
	if(record)
		{
		memcpy(&result.M, record + 0x134, sizeof(result.M));
		memcpy(&result.t, record + 0x158, sizeof(result.t));
		}
	nxNpSceneGuardLeave(ctx);
	return result;
	}

// phys_fn_000136 (0x00004b60, 498 B)
NxVec3 NpActorVtable::getCMassGlobalPositionVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char scratch[0x260];
	const unsigned char* source = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	if(!source)
		nxNpActorReport(0x314, "Actor::getCMassGlobalPosition: Cannot be called on a static actor!");
	NxVec3 result = nxNpActorCMassPosition(
		nxNpActorDerivedMassFrame(source, scratch, NX_RF_138), 0x158);
	nxNpSceneGuardLeave(ctx);
	return result;
	}

// phys_fn_000138 (0x00004d60, 658 B)
NxMat33 NpActorVtable::getCMassGlobalOrientationVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char scratch[0x260];
	const unsigned char* source = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	if(!source)
		nxNpActorReport(0x31d, "Actor::getCMassGlobalOrientation: Cannot be called on a static actor!");
	NxMat33 result = nxNpActorCMassMatrix(
		nxNpActorDerivedMassFrame(source, scratch, NX_RF_138), 0x134);
	nxNpSceneGuardLeave(ctx);
	return result;
	}

// phys_fn_000166 (0x00006c60, 479 B)
// 0x6c9c-0x6d2f: a static actor reports E1 line 0xba; a mass that is not
// greater than zero (NaN included: `test ah,0x41` after fcomp 0.0) reports
// line 0xbb with the mass passed as a double for the %f.
void NpActorVtable::setMass(NxReal mass)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0xb9)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(!record)
		nxNpActorReport(0xba, "Actor::setMass: Actor must be dynamic!");
	else if(!(mass > 0.0f))
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_NPACTOR_CPP,
			0xbb, 0, "Body::setMass: mass is %f, should be positive!",
			static_cast<double>(mass));
	else
		{
		*reinterpret_cast<float*>(record + 0x188) = mass;
		*reinterpret_cast<float*>(record + 0xc0) = 1.0f / mass;
		nxNpActorMarkRecordDirty(record, 0x10000);
		}
	nxNpSceneGuardLeave(ctx);
	}

NxReal NpActorVtable::getMass() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	const NxReal out = record
		? *reinterpret_cast<NxReal*>(record + 0x188) : NxReal();
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000168 (0x00006e40, 577 B)
// 0x6ec9-0x6f85: the tensor is stored, the three inverses 1/m are formed and
// spilled to float, and each spill, widened to a double, is classified by the
// CRT's _fpclass (phys_fn_005666, 0xf4140, which the oracle calls): any
// _FPCLASS_SNAN | QNAN | NINF | PINF (0x207) stores zero in all three
// (+0xc4..+0xcc); otherwise the float inverses are stored as they are, so a
// negative inertia keeps its negative inverse and a zero or denormal one
// (whose inverse overflows) zeroes the three.
void NpActorVtable::setMassSpaceInertiaTensor(const NxVec3& m)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0xc5)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		memcpy(record + 0x18c, &m, sizeof(m));
		float* inverse = reinterpret_cast<float*>(record + 0xc4);
		const float inverseX = 1.0f / m.x;
		const float inverseY = 1.0f / m.y;
		const float inverseZ = 1.0f / m.z;
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
		nxNpActorMarkRecordDirty(record, 0x20000);
		}
	else
		nxNpActorReport(0xc6, "Actor::setMassSpaceInertiaTensor: Actor must be dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000102 (0x00003310, 151 B)
NxVec3 NpActorVtable::getMassSpaceInertiaTensorVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0x328, "Actor::getMassSpaceInertiaTensorVal: Cannot be called on a static actor!");
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record) memcpy(&out, record + 0x18c, sizeof(out));
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000140 (0x00005000, 706 B)
NxMat33 NpActorVtable::getGlobalInertiaTensorVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0x32f, "Actor::getGlobalInertiaTensorVal: Cannot be called on a static actor!");
	NxMat33 out = nxNpActorInstantTensor(record, 0x18c, NX_RF_140);
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000142 (0x000052d0, 691 B)
// The only reader in the unit that takes no scene lock (0x52d0-0x5320 go
// straight to [[this+0x14]+8]); the static arm reports E1 line 0x338.
NxMat33 NpActorVtable::getGlobalInertiaTensorInverseVal() const
	{
	unsigned char* record = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	if(!record)
		nxNpActorReport(0x338, "Actor::getGlobalInertiaTensorInverseVal: Cannot be called on a static actor!");
	return nxNpActorInstantTensor(record, 0xc4, NX_RF_134);
	}

// phys_fn_000170 (0x00007090, 431 B)
// The value is checked before the actor (fcomp 0.0; `test ah,1`: negative or
// unordered reports line 0xd1 even on a static actor), then a static actor
// reports line 0xd2; both arms unlock after the report.
void NpActorVtable::setLinearDamping(NxReal damping)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0xd0)) return;
	if(!(damping >= 0.0f))
		{
		nxNpActorReport(0xd1, "Actor::setLinearDamping: The linear damping must be nonnegative!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<float*>(record + 0xb8) = damping;
		nxNpActorMarkRecordDirty(record, 0x800);
		}
	else
		nxNpActorReport(0xd2, "Actor::setLinearDamping: Actor must be dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000050 (0x00002610, 107 B)
NxReal NpActorVtable::getLinearDamping() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0xd9, "Actor::setLinearDamping: Actor must be dynamic!");
	const NxReal out = record
		? *reinterpret_cast<NxReal*>(record + 0xb8) : NxReal();
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000172 (0x00007240, 431 B)
// The value is checked before the actor (fcomp 0.0; `test ah,1`: negative or
// unordered reports line 0xe0 even on a static actor), then a static actor
// reports line 0xe1; both arms unlock after the report.
void NpActorVtable::setAngularDamping(NxReal damping)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0xdf)) return;
	if(!(damping >= 0.0f))
		{
		nxNpActorReport(0xe0, "Actor::setAngularDamping: The angular damping must be nonnegative!");
		nxNpSceneGuardLeave(ctx);
		return;
		}
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<float*>(record + 0xbc) = damping;
		nxNpActorMarkRecordDirty(record, 0x1000);
		}
	else
		nxNpActorReport(0xe1, "Actor::setAngularDamping: Actor must be dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000052 (0x00002680, 107 B)
NxReal NpActorVtable::getAngularDamping() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0xe8, "Actor::getAngularDamping: Actor must be dynamic!");
	const NxReal out = record
		? *reinterpret_cast<NxReal*>(record + 0xbc) : NxReal();
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// The velocity setters' wake (000174 0x7566-0x76a7, 000176 0x7881-, 000180
// 0x7d68-, 000182 0x8094-): the squared speed, an x87 register sum, is
// compared with fcomp against the sleep threshold (+0xd0 linear, +0xd4
// angular), and `test ah,1; jne` skips the wake when it is below or
// unordered; otherwise the wake every setter has follows (+0x114 & 0x100,
// then an ordered +0x84 < 0.39999998f, then 0x3ecccccc and the 0x10 mark).
static void nxNpActorWakeAboveThreshold(unsigned char* record, double squaredSpeed,
	unsigned thresholdOffset)
	{
	if(!(squaredSpeed >= *reinterpret_cast<const float*>(record + thresholdOffset)))
		return;
	nxNpActorWakeAfterCMassWrite(record);
	}

// phys_fn_000174 (0x000073f0, 770 B)
// After the stores and the 4 mark, the input's (y y + z z) + x x against
// the linear sleep threshold +0xd0.
void NpActorVtable::setLinearVelocity(const NxVec3& velocity)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0xf3)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0x6c, &velocity, sizeof(velocity));
		memcpy(record + 0x34, &velocity, sizeof(velocity));
		nxNpActorMarkRecordDirty(record, 4);
		const double x = velocity.x, y = velocity.y, z = velocity.z;
		nxNpActorWakeAboveThreshold(record, (y * y + z * z) + x * x, 0xd0);
		}
	else
		nxNpActorReport(0xf4, "Actor::setLinearVelocity: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000176 (0x00007700, 786 B)
// As 000174, against the angular sleep threshold +0xd4.
void NpActorVtable::setAngularVelocity(const NxVec3& velocity)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0xfc)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0x78, &velocity, sizeof(velocity));
		memcpy(record + 0x40, &velocity, sizeof(velocity));
		nxNpActorMarkRecordDirty(record, 8);
		const double x = velocity.x, y = velocity.y, z = velocity.z;
		nxNpActorWakeAboveThreshold(record, (y * y + z * z) + x * x, 0xd4);
		}
	else
		nxNpActorReport(0xfd, "Actor::setAngularVelocity: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000104 (0x000033b0, 137 B)
NxVec3 NpActorVtable::getLinearVelocityVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0x343, "Actor::getLinearVelocity: Actor must be dynamic!");
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record) memcpy(&out, record + 0x6c, sizeof(out));
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000106 (0x00003440, 140 B)
NxVec3 NpActorVtable::getAngularVelocityVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0x34a, "Actor::getAngularVelocity: Actor must be dynamic!");
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record) memcpy(&out, record + 0x78, sizeof(out));
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000178 (0x00007a20, 385 B)
void NpActorVtable::setMaxAngularVelocity(NxReal limit)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x108)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<NxReal*>(record + 0xd8) = limit * limit;
		nxNpActorMarkRecordDirty(record, 0x8000);
		}
	else
		nxNpActorReport(0x109, "Actor::setMaxAngularVelocity: Actor must be dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000180 (0x00007bb0, 782 B)
// The stored velocity's (x x + y y) + z z against +0xd0 (0x7d68-0x7d90).
void NpActorVtable::setLinearMomentum(const NxVec3& momentum)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x113)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		const NxReal inverseMass = *reinterpret_cast<NxReal*>(record + 0xc0);
		NxVec3 velocity(inverseMass * momentum.x,
			inverseMass * momentum.y, inverseMass * momentum.z);
		memcpy(record + 0x6c, &velocity, sizeof(velocity));
		memcpy(record + 0x34, &velocity, sizeof(velocity));
		nxNpActorMarkRecordDirty(record, 4);
		const float* v = reinterpret_cast<const float*>(record + 0x6c);
		const double x = v[0], y = v[1], z = v[2];
		nxNpActorWakeAboveThreshold(record, (x * x + y * y) + z * z, 0xd0);
		}
	else
		nxNpActorReport(0x114, "Actor::setLinearMomentum: Actor must be dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000182 (0x00007ec0, 867 B)
// 0x7f32-0x7f9e: w = I L with I the world inverse inertia +0x164, each row
// one x87 sum in the listing's order, (I1 y + I2 z) + I0 x, (I4 y + I3 x) +
// I5 z and (I7 y + I6 x) + I8 z, rounded at the store; then the stored
// velocity's (x x + y y) + z z against +0xd4 (0x8094-0x80bc).
void NpActorVtable::setAngularMomentum(const NxVec3& momentum)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x11c)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		const NxReal* inverse = reinterpret_cast<const NxReal*>(record + 0x164);
		#define NX_IL(i, c) (static_cast<double>(inverse[i]) * momentum.c)
		NxVec3 velocity(
			static_cast<NxReal>((NX_IL(1, y) + NX_IL(2, z)) + NX_IL(0, x)),
			static_cast<NxReal>((NX_IL(4, y) + NX_IL(3, x)) + NX_IL(5, z)),
			static_cast<NxReal>((NX_IL(7, y) + NX_IL(6, x)) + NX_IL(8, z)));
		#undef NX_IL
		memcpy(record + 0x78, &velocity, sizeof(velocity));
		memcpy(record + 0x40, &velocity, sizeof(velocity));
		nxNpActorMarkRecordDirty(record, 8);
		const float* w = reinterpret_cast<const float*>(record + 0x78);
		const double x = w[0], y = w[1], z = w[2];
		nxNpActorWakeAboveThreshold(record, (x * x + y * y) + z * z, 0xd4);
		}
	else
		nxNpActorReport(0x11d, "Actor::setAngularMomentum: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000108 (0x000034d0, 173 B)
NxVec3 NpActorVtable::getLinearMomentumVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0x353, "Actor::getLinearMomentumVal: Cannot be called on a static actor!");
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record)
		{
		const NxReal mass = *reinterpret_cast<NxReal*>(record + 0x188);
		const NxReal* velocity = reinterpret_cast<const NxReal*>(record + 0x6c);
		out = NxVec3(mass * velocity[0], mass * velocity[1], mass * velocity[2]);
		}
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000144 (0x00005590, 849 B)
NxVec3 NpActorVtable::getAngularMomentumVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0x35a, "Actor::getAngularMomentumVal: Cannot be called on a static actor!");
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record)
		{
		NxMat33 world = nxNpActorInstantTensor(record, 0x18c, NX_RF_144);
		float t[9];
		memcpy(t, &world, sizeof(t));
		const float* v = reinterpret_cast<const float*>(record + 0x78);
		// 0x5854-0x58ca: each row (t1 wy + t2 wz) + t0 wx, rounded once.
		out = NxVec3(
			nxNpActorX87Dot3(t[1], v[1], t[2], v[2], t[0], v[0]),
			nxNpActorX87Dot3(t[4], v[1], t[5], v[2], t[3], v[0]),
			nxNpActorX87Dot3(t[7], v[1], t[8], v[2], t[6], v[0]));
		}
	nxNpSceneGuardLeave(ctx);
	return out;
	}

static void nxNpActorAccumulateForce(unsigned char* record,
	const NxVec3& value, NxForceMode mode, bool angular);
static NxVec3 nxNpActorRotateLocalForce(const unsigned char* record,
	const NxVec3& local);

// Row 000791 (Body::addForceAtPos) is core/JointSupport.cpp's
// Row000791Fixture::row000791 (the one definition since the second merge of
// main into the scene-raycast block): the lever and torque in the listing's
// precision, then one 000782 call (nxNpActorApplyForce below) for both
// vectors. The at-position rows 000054/000154/000156/000158 pass wake 1.
static void nxNpActorForceAtPos(unsigned char* record, const NxVec3& force,
	const NxVec3& worldPosition, NxForceMode mode)
	{
	reinterpret_cast<Row000791Fixture*>(record)->row000791(force, worldPosition,
		static_cast<NxU32>(mode), 1);
	}

// phys_fn_000152 (0x00005fa0, 346 B)
// Local point to world (callers 000154, 000158). R is the shared ROT
// sequence (the five float spills, rows stored to float); then, in x87
// registers, x = (R01 py + R02 pz) + R00 px stays unrounded until it is added
// to t.x, while y = (R11 py + R12 pz) + R10 px and z = (R22 pz + R20 px) +
// R21 py are spilled to float (0x100060b4, 0x100060d4) before t.y and t.z are
// added to them.
static NxVec3 nxNpActorLocalPosition(const unsigned char* record,
	const NxVec3& position)
	{
	float r[9];
	nxNpActorRotationFromQuaternionGetter(
		reinterpret_cast<const float*>(record + 0x5c), r);
	const float* t = reinterpret_cast<const float*>(record + 0x50);
	const double x = (static_cast<double>(r[1]) * position.y +
		static_cast<double>(r[2]) * position.z) +
		static_cast<double>(r[0]) * position.x;
	const NxReal y = static_cast<NxReal>((static_cast<double>(r[4]) * position.y +
		static_cast<double>(r[5]) * position.z) +
		static_cast<double>(r[3]) * position.x);
	const NxReal z = static_cast<NxReal>((static_cast<double>(r[8]) * position.z +
		static_cast<double>(r[6]) * position.x) +
		static_cast<double>(r[7]) * position.y);
	return NxVec3(static_cast<NxReal>(x + t[0]),
		static_cast<NxReal>(static_cast<double>(t[1]) + y),
		static_cast<NxReal>(static_cast<double>(t[2]) + z));
	}

// phys_fn_000054 (0x000026f0, 167 B)
void NpActorVtable::addForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x12a)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorForceAtPos(record, force, pos, mode);
	else
		nxNpActorReport(0x12b, "Actor::addForceAtPos: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000154 (0x00006100, 201 B)
void NpActorVtable::addForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x131)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorForceAtPos(record, force,
			nxNpActorLocalPosition(record, pos), mode);
	else
		nxNpActorReport(0x132, "Actor::addForceAtLocalPos: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000156 (0x000061d0, 201 B)
void NpActorVtable::addLocalForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x13b)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorForceAtPos(record,
			nxNpActorRotateLocalForce(record, force), pos, mode);
	else
		nxNpActorReport(0x13c, "Actor::addLocalForceAtPos: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000158 (0x000062a0, 214 B)
void NpActorVtable::addLocalForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x144)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorForceAtPos(record,
			nxNpActorRotateLocalForce(record, force),
			nxNpActorLocalPosition(record, pos), mode);
	else
		nxNpActorReport(0x145, "Actor::addLocalForceAtLocalPos: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000782 (0x00018730, 3428 B)
// A row of gap:SceneRaycast.cpp..CapsuleShape.cpp (thiscall on the record,
// (force, torque, mode, wake), `ret 0x10`): the body's force/torque
// accumulator. A jump table on the mode (0x10018746; a mode above 4 goes
// straight to the wake tail) adds the force, if non-null, and then the
// torque, if non-null, each followed by its dirty mark:
// - NX_FORCE (0) and NX_SMOOTH_IMPULSE (3): +0x88/+0xa0 += f / m, the x
//   product kept in the register (only y and z are spilled), mark 0x20/0x80;
//   +0x94/+0xac += I t with I the world inverse inertia +0x164, each row
//   (I2 z + I1 y) + I0 x, rows 0 and 1 kept in registers and row 2 spilled,
//   mark 0x40/0x100;
// - NX_IMPULSE (1): +0x6c += f / m with all three products spilled (copied to
//   +0x34), mark 4; +0x78 += I t with row 0 in the register and rows 1 and 2
//   spilled (copied to +0x40), mark 8;
// - NX_VELOCITY_CHANGE (2): +0x6c += f (+0x34), mark 4; +0x78 += t (+0x40),
//   mark 8;
// - NX_SMOOTH_VELOCITY_CHANGE (4): +0xa0 += f, mark 0x80; +0xac += t, mark 0x100.
// The tail (0x1001936f) runs once for every mode when the wake argument is
// set: unless +0x114 & 0x100, an ordered +0x84 < 0.39999998f raises +0x84
// and +0x4c to 0x3ecccccc and marks 0x10.
// Not static and noinline: the image calls 000782 as its own function (from
// 000056/58/160/162 and from 000791, core/JointSupport.cpp).
__declspec(noinline) void nxNpActorApplyForce(unsigned char* record, const NxVec3* force,
	const NxVec3* torque, unsigned mode, bool wake)
	{
	const float inverseMass = *reinterpret_cast<const float*>(record + 0xc0);
	const float* inertia = reinterpret_cast<const float*>(record + 0x164);
	#define NX_IT(r, t) ((static_cast<double>(inertia[3 * (r) + 2]) * (t).z + \
		static_cast<double>(inertia[3 * (r) + 1]) * (t).y) + \
		static_cast<double>(inertia[3 * (r)]) * (t).x)
	switch(mode)
		{
		case NX_FORCE:
		case NX_SMOOTH_IMPULSE:
			{
			const bool smooth = mode == NX_SMOOTH_IMPULSE;
			if(force)
				{
				float* d = reinterpret_cast<float*>(record + (smooth ? 0xa0 : 0x88));
				const double x = static_cast<double>(inverseMass) * force->x;
				const float y = inverseMass * force->y;
				const float z = inverseMass * force->z;
				d[0] = static_cast<float>(x + d[0]);
				d[1] = y + d[1];
				d[2] = z + d[2];
				nxNpActorMarkRecordDirty(record, smooth ? 0x80u : 0x20u);
				}
			if(torque)
				{
				float* d = reinterpret_cast<float*>(record + (smooth ? 0xac : 0x94));
				const double row0 = NX_IT(0, *torque);
				const double row1 = NX_IT(1, *torque);
				const float row2 = static_cast<float>(NX_IT(2, *torque));
				d[0] = static_cast<float>(row0 + d[0]);
				d[1] = static_cast<float>(row1 + d[1]);
				d[2] = row2 + d[2];
				nxNpActorMarkRecordDirty(record, smooth ? 0x100u : 0x40u);
				}
			break;
			}
		case NX_IMPULSE:
			if(force)
				{
				float* d = reinterpret_cast<float*>(record + 0x6c);
				const float x = inverseMass * force->x;
				const float y = inverseMass * force->y;
				const float z = inverseMass * force->z;
				d[0] = x + d[0];
				d[1] = y + d[1];
				d[2] = z + d[2];
				memcpy(record + 0x34, d, 3 * sizeof(float));
				nxNpActorMarkRecordDirty(record, 4);
				}
			if(torque)
				{
				float* d = reinterpret_cast<float*>(record + 0x78);
				const double row0 = NX_IT(0, *torque);
				const float row1 = static_cast<float>(NX_IT(1, *torque));
				const float row2 = static_cast<float>(NX_IT(2, *torque));
				d[0] = static_cast<float>(row0 + d[0]);
				d[1] = row1 + d[1];
				d[2] = row2 + d[2];
				memcpy(record + 0x40, d, 3 * sizeof(float));
				nxNpActorMarkRecordDirty(record, 8);
				}
			break;
		case NX_VELOCITY_CHANGE:
			if(force)
				{
				float* d = reinterpret_cast<float*>(record + 0x6c);
				d[0] = force->x + d[0];
				d[1] = d[1] + force->y;
				d[2] = d[2] + force->z;
				memcpy(record + 0x34, d, 3 * sizeof(float));
				nxNpActorMarkRecordDirty(record, 4);
				}
			if(torque)
				{
				float* d = reinterpret_cast<float*>(record + 0x78);
				d[0] = d[0] + torque->x;
				d[1] = d[1] + torque->y;
				d[2] = d[2] + torque->z;
				memcpy(record + 0x40, d, 3 * sizeof(float));
				nxNpActorMarkRecordDirty(record, 8);
				}
			break;
		case NX_SMOOTH_VELOCITY_CHANGE:
			if(force)
				{
				float* d = reinterpret_cast<float*>(record + 0xa0);
				d[0] = force->x + d[0];
				d[1] = force->y + d[1];
				d[2] = force->z + d[2];
				nxNpActorMarkRecordDirty(record, 0x80);
				}
			if(torque)
				{
				float* d = reinterpret_cast<float*>(record + 0xac);
				d[0] = torque->x + d[0];
				d[1] = torque->y + d[1];
				d[2] = torque->z + d[2];
				nxNpActorMarkRecordDirty(record, 0x100);
				}
			break;
		default:
			break;
		}
	#undef NX_IT
	if(wake)
		nxNpActorWakeAfterCMassWrite(record);
	}

// The single-vector rows (000056, 000058, 000160, 000162) call 000782 with
// the other vector null and the wake argument 1.
static void nxNpActorAccumulateForce(unsigned char* record,
	const NxVec3& value, NxForceMode mode, bool angular)
	{
	nxNpActorApplyForce(record, angular ? 0 : &value, angular ? &value : 0,
		static_cast<unsigned>(mode), true);
	}

// phys_fn_000056 (0x000027a0, 165 B)
void NpActorVtable::addForce(const NxVec3& force, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x14d)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorAccumulateForce(record, force, mode, false);
	else
		nxNpActorReport(0x14e, "Actor::addForce: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000150 (0x00005e70, 298 B)
// Local vector to world (callers 000156, 000158, 000160, 000162): the shared
// ROT sequence, then each row summed in x87 registers as (Ri1 y + Ri2 z) +
// Ri0 x and rounded only at the store (0x10005f3c-0x10005f91).
static NxVec3 nxNpActorRotateLocalForce(const unsigned char* record,
	const NxVec3& local)
	{
	float m[9];
	nxNpActorRotationFromQuaternionGetter(
		reinterpret_cast<const float*>(record + 0x5c), m);
	return NxVec3(
		static_cast<NxReal>((static_cast<double>(m[1]) * local.y +
			static_cast<double>(m[2]) * local.z) +
			static_cast<double>(m[0]) * local.x),
		static_cast<NxReal>((static_cast<double>(m[4]) * local.y +
			static_cast<double>(m[5]) * local.z) +
			static_cast<double>(m[3]) * local.x),
		static_cast<NxReal>((static_cast<double>(m[7]) * local.y +
			static_cast<double>(m[8]) * local.z) +
			static_cast<double>(m[6]) * local.x));
	}

// phys_fn_000160 (0x00006380, 198 B)
void NpActorVtable::addLocalForce(const NxVec3& force, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x156)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorAccumulateForce(record,
			nxNpActorRotateLocalForce(record, force), mode, false);
	else
		nxNpActorReport(0x157, "Actor::addLocalForce: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000058 (0x00002850, 165 B)
void NpActorVtable::addTorque(const NxVec3& torque, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x160)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorAccumulateForce(record, torque, mode, true);
	else
		nxNpActorReport(0x161, "Actor::addTorque: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000162 (0x00006450, 198 B)
void NpActorVtable::addLocalTorque(const NxVec3& torque, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x169)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorAccumulateForce(record,
			nxNpActorRotateLocalForce(record, torque), mode, true);
	else
		nxNpActorReport(0x16a, "Actor::addLocalTorque: Actor must be (non-kinematic) dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000060 (0x00002900, 72 B)
NxReal NpActorVtable::computeKineticEnergy() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	NxReal energy = 0.0f;
	if(record)
		{
		const float* linear = reinterpret_cast<const float*>(record + 0x6c);
		const float* angular = reinterpret_cast<const float*>(record + 0x78);
		const float mass = *reinterpret_cast<const float*>(record + 0x188);
		const float* inertia = reinterpret_cast<const float*>(record + 0x18c);
		// Row 000742 (0x00016dd0, 89 B), a row of
		// gap:SceneRaycast.cpp..CapsuleShape.cpp: the three I_k w_k stay in
		// registers, the linear (vz vz + vy vy) + vx vx is scaled by the mass,
		// then ((m v.v + I2 w2 w2) + I1 w1 w1) + I0 w0 w0 is halved; 000060
		// rounds the result to float (fstp [esp+8]) before its unlock.
		const double spin0 = static_cast<double>(inertia[0]) * angular[0];
		const double spin1 = static_cast<double>(inertia[1]) * angular[1];
		const double spin2 = static_cast<double>(inertia[2]) * angular[2];
		const double translational =
			((static_cast<double>(linear[2]) * linear[2] +
			static_cast<double>(linear[1]) * linear[1]) +
			static_cast<double>(linear[0]) * linear[0]) * mass;
		energy = static_cast<NxReal>(
			(((translational + spin2 * angular[2]) + spin1 * angular[1]) +
			spin0 * angular[0]) * 0.5);
		}
	nxNpSceneGuardLeave(ctx);
	return energy;
	}

// phys_fn_000148, actor dynamic slot 66 at 0x00005b40. The local point is rotated through
// both the body quaternion and the mass frame before omega x radius is added
// to the linear velocity. The null-record arm returns zero.
NxVec3 NpActorVtable::getLocalPointVelocityVal(const NxVec3& point) const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	const unsigned char* record = nxNpActorRecord(self);
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record)
		{
		float rotation[9];
		nxNpActorRotationFromQuaternionGetter(
			reinterpret_cast<const float*>(record + 0x5c), rotation);
		const float* frame = reinterpret_cast<const float*>(record + 0xdc);
		float combined[9];
		for(unsigned row = 0; row < 3; ++row)
			for(unsigned col = 0; col < 3; ++col)
				{
				// The first column (and row-zero's second cell) is
				// accumulated 1,2,0; other cells use 0,1,2.
				const bool reordered = col == 0 || (row == 0 && col == 1);
				const unsigned a = reordered ? 1u : 0u;
				const unsigned b = reordered ? 2u : 1u;
				const unsigned c = reordered ? 0u : 2u;
#if defined(_MSC_VER) && defined(_M_IX86)
				const float* r0 = rotation + row * 3 + a;
				const float* r1 = rotation + row * 3 + b;
				const float* r2 = rotation + row * 3 + c;
				const float* f0 = frame + a * 3 + col;
				const float* f1 = frame + b * 3 + col;
				const float* f2 = frame + c * 3 + col;
				float value;
				__asm {
					mov eax, r0
					mov edx, f0
					fld dword ptr [eax]
					fmul dword ptr [edx]
					mov eax, r1
					mov edx, f1
					fld dword ptr [eax]
					fmul dword ptr [edx]
					faddp st(1), st(0)
					mov eax, r2
					mov edx, f2
					fld dword ptr [eax]
					fmul dword ptr [edx]
					faddp st(1), st(0)
					fstp dword ptr [value]
				}
				combined[row * 3 + col] = value;
#else
				combined[row * 3 + col] = static_cast<float>(
					static_cast<double>(rotation[row * 3 + a]) * frame[a * 3 + col] +
					static_cast<double>(rotation[row * 3 + b]) * frame[b * 3 + col] +
					static_cast<double>(rotation[row * 3 + c]) * frame[c * 3 + col]);
#endif
				}
		const float* velocity = reinterpret_cast<const float*>(record + 0x6c);
		const float* angular = reinterpret_cast<const float*>(record + 0x78);
		// The three local-point radii stay on the x87 stack. The first two
		// cross-product components spill to float before velocity is added;
		// the third remains extended through the final addition.
#if defined(_MSC_VER) && defined(_M_IX86)
		const float* m = combined;
		const float* localPoint = &point.x;
		const float* omega = angular;
		const float* linear = velocity;
		float* destination = &out.x;
		float crossX, crossY;
		__asm {
			mov eax, m
			mov edx, localPoint
			fld dword ptr [eax+4]
			fmul dword ptr [edx+4]
			fld dword ptr [eax+8]
			fmul dword ptr [edx+8]
			faddp st(1), st(0)
			fld dword ptr [eax]
			fmul dword ptr [edx]
			faddp st(1), st(0)
			fld dword ptr [eax+12]
			fmul dword ptr [edx]
			fld dword ptr [eax+16]
			fmul dword ptr [edx+4]
			faddp st(1), st(0)
			fld dword ptr [eax+20]
			fmul dword ptr [edx+8]
			faddp st(1), st(0)
			fld dword ptr [eax+24]
			fmul dword ptr [edx]
			fld dword ptr [eax+28]
			fmul dword ptr [edx+4]
			faddp st(1), st(0)
			fld dword ptr [eax+32]
			fmul dword ptr [edx+8]
			faddp st(1), st(0)
			mov ecx, omega
			fld st(0)
			fmul dword ptr [ecx+4]
			fld st(2)
			fmul dword ptr [ecx+8]
			fsubp st(1), st(0)
			fstp dword ptr [crossX]
			fld st(2)
			fmul dword ptr [ecx+8]
			fxch st(1)
			fmul dword ptr [ecx]
			fsubp st(1), st(0)
			fstp dword ptr [crossY]
			fmul dword ptr [ecx]
			fxch st(1)
			fmul dword ptr [ecx+4]
			fsubp st(1), st(0)
			mov ecx, linear
			fadd dword ptr [ecx+8]
			fld dword ptr [crossY]
			fadd dword ptr [ecx+4]
			fld dword ptr [crossX]
			fadd dword ptr [ecx]
			mov ecx, destination
			fstp dword ptr [ecx]
			fstp dword ptr [ecx+4]
			fstp dword ptr [ecx+8]
		}
#else
		const float local[3] = {point.x, point.y, point.z};
		float radius[3];
		for(unsigned row = 0; row < 3; ++row)
			radius[row] = static_cast<float>(
				static_cast<double>(combined[row * 3]) * local[0] +
				static_cast<double>(combined[row * 3 + 2]) * local[2] +
				static_cast<double>(combined[row * 3 + 1]) * local[1]);
		out.x = static_cast<float>(static_cast<double>(radius[2]) * angular[1] -
			static_cast<double>(radius[1]) * angular[2] + velocity[0]);
		out.y = static_cast<float>(static_cast<double>(radius[0]) * angular[2] -
			static_cast<double>(radius[2]) * angular[0] + velocity[1]);
		out.z = static_cast<float>(static_cast<double>(radius[1]) * angular[0] -
			static_cast<double>(radius[0]) * angular[1] + velocity[2]);
#endif
		}
	nxNpSceneGuardLeave(ctx);
	return out;
	}

static unsigned char* nxNpActorGroupRoot(unsigned char* record)
	{
	unsigned char*& parent = *reinterpret_cast<unsigned char**>(record + 0x1e8);
	if(parent != record) parent = nxNpActorGroupRoot(parent);
	return parent;
	}

bool NpActorVtable::isGroupSleeping() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	bool asleep = true;
	if(record)
		{
		unsigned char* root = nxNpActorGroupRoot(record);
		for(unsigned char* member = *reinterpret_cast<unsigned char**>(root + 0x1e8);
			member; member = *reinterpret_cast<unsigned char**>(member + 0x1fc))
			{
			if(*reinterpret_cast<NxReal*>(member + 0x84) > 0.0f)
				{
				asleep = false;
				break;
				}
			}
		}
	nxNpSceneGuardLeave(ctx);
	return asleep;
	}

bool NpActorVtable::isSleeping() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	const bool asleep = !record ||
		*reinterpret_cast<unsigned*>(record + 0x84) == 0;
	nxNpSceneGuardLeave(ctx);
	return asleep;
	}

NxReal NpActorVtable::getSleepLinearVelocity() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	const NxReal threshold = record ? sqrtf(
		*reinterpret_cast<NxReal*>(record + 0xd0)) : NxReal();
	nxNpSceneGuardLeave(ctx);
	return threshold;
	}

// phys_fn_000184 (0x00008230, 333 B)
void NpActorVtable::setSleepLinearVelocity(NxReal threshold)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x195)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<NxReal*>(record + 0xd0) = threshold * threshold;
		nxNpActorMarkRecordDirty(record, 0x2000);
		}
	nxNpSceneGuardLeave(ctx);
	}

NxReal NpActorVtable::getSleepAngularVelocity() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	const NxReal threshold = record ? sqrtf(
		*reinterpret_cast<NxReal*>(record + 0xd4)) : NxReal();
	nxNpSceneGuardLeave(ctx);
	return threshold;
	}

// phys_fn_000186 (0x00008380, 333 B)
void NpActorVtable::setSleepAngularVelocity(NxReal threshold)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x1a2)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<NxReal*>(record + 0xd4) = threshold * threshold;
		nxNpActorMarkRecordDirty(record, 0x4000);
		}
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000192 (0x00008810, 371 B)
void NpActorVtable::wakeUp(NxReal wakeCounterValue)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x207)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		// 0x8869-0x8886: the argument is stored with mov (final review I1).
		memcpy(record + 0x84, &wakeCounterValue, sizeof(wakeCounterValue));
		memcpy(record + 0x4c, &wakeCounterValue, sizeof(wakeCounterValue));
		nxNpActorMarkRecordDirty(record, 0x10);
		*reinterpret_cast<unsigned*>(record + 0x114) &= ~0x100u;
		}
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000194 (0x00008990, 354 B)
void NpActorVtable::putToSleep()
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x211)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<NxReal*>(record + 0x84) = 0.0f;
		*reinterpret_cast<NxReal*>(record + 0x4c) = 0.0f;
		nxNpActorMarkRecordDirty(record, 0x10);
		*reinterpret_cast<unsigned*>(record + 0x114) |= 0x100u;
		}
	nxNpSceneGuardLeave(ctx);
	}

// Concrete actor slots 75-77 store the flag mask in the 0x50-byte body.
// phys_fn_000074 (0x00002ba0, 85 B)
void NpActorVtable::raiseActorFlag(NxActorFlag flag)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x1bb)) return;
	unsigned char* body = nxNpActorBody(this);
	if(body) *reinterpret_cast<unsigned*>(body + 0x14) |=
		static_cast<unsigned>(flag);
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000076 (0x00002c00, 87 B)
void NpActorVtable::clearActorFlag(NxActorFlag flag)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x1c1)) return;
	unsigned char* body = nxNpActorBody(this);
	if(body) *reinterpret_cast<unsigned*>(body + 0x14) &=
		~static_cast<unsigned>(flag);
	nxNpSceneGuardLeave(ctx);
	}

bool NpActorVtable::readActorFlag(NxActorFlag flag) const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* body = nxNpActorBody(self);
	const bool out = body && (*reinterpret_cast<unsigned*>(body + 0x14) &
		static_cast<unsigned>(flag)) != 0;
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000188 (0x000084d0, 407 B)
void NpActorVtable::raiseBodyFlag(NxBodyFlag flag)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x1cf)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		if(static_cast<unsigned>(flag) & 0x80u)
			nxNpActorTransitionKinematic(record, true);
		*reinterpret_cast<unsigned*>(record + 0x10c) |=
			static_cast<unsigned>(flag);
		nxNpActorMarkRecordDirty(record, 0x80000);
		}
	else
		nxNpActorReport(0x1d0, "Actor::raiseBodyFlag: Actor must be dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000190 (0x00008670, 415 B)
void NpActorVtable::clearBodyFlag(NxBodyFlag flag)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x1d9)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		if(static_cast<unsigned>(flag) & 0x80u)
			nxNpActorTransitionKinematic(record, false);
		*reinterpret_cast<unsigned*>(record + 0x10c) &=
			~static_cast<unsigned>(flag);
		nxNpActorMarkRecordDirty(record, 0x80000);
		}
	else
		nxNpActorReport(0x1da, "Actor::clearBodyFlag: Actor must be dynamic!");
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000080 (0x00002c90, 103 B)
bool NpActorVtable::readBodyFlag(NxBodyFlag flag) const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	if(!record)
		nxNpActorReport(0x1e3, "Actor::readBodyFlag: Actor must be dynamic!");
	const bool out = record && (*reinterpret_cast<unsigned*>(record + 0x10c) &
		static_cast<unsigned>(flag)) != 0;
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// phys_fn_000046 at 0x000024c0, actor dynamic vtable slot 81. The
// descriptor gather is shared with the direct object-layout differential.
bool NpActorVtable::saveBodyToDesc(NxBodyDesc& desc)
	{
	void* ctx = nxNpActorContext(this, 0x10);
	nxNpSceneGuardEnter(ctx);
	const bool result = nxGatherDescriptor0046(this,
		reinterpret_cast<unsigned*>(&desc));
	nxNpSceneGuardLeave(ctx);
	return result;
	}

// phys_fn_000120 at 0x00003690, actor vtable slot 82. The oracle writes
// the pose and four actor metadata fields; it leaves body, name and type alone.
void NpActorVtable::saveToDesc(NxActorDescBase& desc)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x22)) return;
	unsigned char* body = nxNpActorBody(this);
	unsigned char* record = body
		? *reinterpret_cast<unsigned char**>(body + 8) : 0;
	if(record)
		{
		float rows[9];
		nxNpActorRotationFromQuaternionGetter(
			reinterpret_cast<const float*>(record + 0x5c), rows);
		memcpy(&desc.globalPose.M, rows, sizeof(rows));
		memcpy(&desc.globalPose.t, record + 0x50, sizeof(NxVec3));
		}
	else if(body)
		memcpy(&desc.globalPose, body + 0x20, sizeof(NxMat34));
	if(body)
		{
		memcpy(&desc.density, body + 0x18, sizeof(desc.density));
		memcpy(&desc.flags, body + 0x14, sizeof(desc.flags));
		memcpy(&desc.group, body + 0x1c, sizeof(desc.group));
		}
	desc.userData = *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(this) + 4);
	nxNpSceneGuardLeave(ctx);
	}

// Concrete actor slots 83/84 address the same body-keyed global name map as
// Actor::loadFromDescInternal (oracle 0x2d90/0x2d60).
// phys_fn_000088 (0x00002d90, 91 B)
// Under the write lock (G1 line 0x1ff), as 0x2d90-0x2de1 are; which table
// the name is bound in is a row-level defect of its own.
void NpActorVtable::setName(const char* name)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x1ff)) return;
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(this) + 0x14);
	// 0x2dce-0x2dd7: the name table call (0x1000edc0) takes [actor+0x14]
	// with no null test.
	nxShapeSetName(body, name);
	nxNpSceneGuardLeave(ctx);
	}

// phys_fn_000086 (0x00002d60, 40 B)
// Under the read lock (NG, Task 3); which table the name is looked up in is
// the row's remaining defect.
const char* NpActorVtable::getName() const
	{
	NxNpActorReadGuard guard(this);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(this) + 0x14);
	return nxShapeGetName(const_cast<unsigned char*>(body));
	}

// Concrete actor slots 85/86 address the body +0x1c group word.
// phys_fn_000112 (0x000035b0, 82 B)
void NpActorVtable::setGroup(NxActorGroup group)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpActorWriteTry(ctx, 0x3cd)) return;
	unsigned char* body = nxNpActorBody(this);
	if(body) *reinterpret_cast<NxActorGroup*>(body + 0x1c) = group;
	nxNpSceneGuardLeave(ctx);
	}

NxActorGroup NpActorVtable::getGroup() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* body = nxNpActorBody(self);
	const NxActorGroup out = body
		? *reinterpret_cast<NxActorGroup*>(body + 0x1c) : NxActorGroup();
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// (unimplemented) setGlobalPose
void NpActorVtable::setGlobalPose(const NxVec3&, const NxMat33&)
	{
	}
