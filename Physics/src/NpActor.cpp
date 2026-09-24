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

#include "NxMat34.h"
#include "NxMat33.h"
#include "NxQuat.h"
#include "NxVec3.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxShape.h"
#include "NxBoxShape.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

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

static NxMat33 nxNpActorInstantTensor(const unsigned char* record,
	unsigned diagonalOffset, bool roundedQuaternionProducts)
	{
	// The momentum getter stores quaternion products as floats before its
	// matrix multiply; the public tensor getters retain x87 precision longer.
	NxMat33 out(NX_IDENTITY_MATRIX);
	if(!record) return out;
	float bodyElements[9];
	if(roundedQuaternionProducts)
		{
		NxQuat quaternion;
		memcpy(&quaternion, record + 0x5c, sizeof(quaternion));
		NxMat33 fromQuaternion(quaternion);
		fromQuaternion.getRowMajor(bodyElements);
		}
	else
		nxNpActorRotationFromQuaternion(record, bodyElements);
	NxMat33 bodyRotation;
	bodyRotation.setRowMajor(bodyElements);
	NxMat33 inertiaFrame;
	inertiaFrame.setRowMajor(reinterpret_cast<const float*>(record + 0xdc));
	NxMat33 worldRotation;
	worldRotation.multiply(bodyRotation, inertiaFrame);
	float rotation[9];
	worldRotation.getRowMajor(rotation);
	float tensor[9];
	nxNpActorWorldTensor(reinterpret_cast<const float*>(record + diagonalOffset),
		rotation, tensor);
	out.setRowMajor(tensor);
	return out;
	}

static void* nxNpActorContext(void* actor, unsigned offset)
	{
	return *reinterpret_cast<void**>(static_cast<unsigned char*>(actor) + offset);
	}

static void nxNpActorMarkRecordDirty(unsigned char* record, unsigned mask)
	{
	if(!record) return;
	unsigned char* aux = *reinterpret_cast<unsigned char**>(record + 0x120);
	if(!aux) return;
	unsigned* flags = *reinterpret_cast<unsigned**>(aux + 0x40);
	const unsigned id = *reinterpret_cast<unsigned*>(record + 0x11c);
	if(!flags || id >= 256) return;
	if(!flags[id])
		{
		unsigned* active = *reinterpret_cast<unsigned**>(aux + 0x50);
		unsigned* end = *reinterpret_cast<unsigned**>(aux + 0x54);
		unsigned* capacity = *reinterpret_cast<unsigned**>(aux + 0x58);
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
			*reinterpret_cast<unsigned**>(aux + 0x50) = active;
			*reinterpret_cast<unsigned**>(aux + 0x58) = grown + next;
			}
		const unsigned index = static_cast<unsigned>(end - active);
		active[index] = id;
		*reinterpret_cast<unsigned**>(aux + 0x54) = end + 1;
		(*reinterpret_cast<unsigned**>(aux + 0x60))[id] = index;
		}
	flags[id] |= mask;
	}

// The kinematic branch at 0x19620 runs before the ordinary body-flag OR/AND.
// The explicit-mass path keeps inverse mass/inertia at +0xc0..+0xcc and a
// 0x20-byte transition block at +0x118. Scene dirties are independent bits.
static void nxNpActorTransitionKinematic(unsigned char* record, bool enable)
	{
	unsigned& flags = *reinterpret_cast<unsigned*>(record + 0x10c);
	if(enable ? (flags & 0x80u) != 0 : (flags & 0x80u) == 0)
		return;
	if(enable)
		{
		flags |= 0x80u;
		memset(record + 0xc0, 0, 4 * sizeof(float));
		void*& state = *reinterpret_cast<void**>(record + 0x118);
		if(!state)
			state = nxGetSdkAllocator()->malloc(0x20, NX_MEMORY_PERSISTENT);
		if(state) *reinterpret_cast<unsigned*>(
			static_cast<unsigned char*>(state) + 0xc) = 0;
		}
	else
		{
		flags &= ~0x80u;
		const unsigned massOffsets[4] = {0x188, 0x18c, 0x190, 0x194};
		for(unsigned i = 0; i < 4; ++i)
			{
			const float mass = *reinterpret_cast<float*>(record + massOffsets[i]);
			*reinterpret_cast<float*>(record + 0xc0 + i * 4) =
				mass > 0.0f ? 1.0f / mass : 0.0f;
			}
		void*& state = *reinterpret_cast<void**>(record + 0x118);
		if(state)
			{
			nxGetSdkAllocator()->free(state);
			state = 0;
			}
		}
	nxNpActorMarkRecordDirty(record, 0x80000u | 0x10000u | 0x20000u);
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

// The oracle's global name map stores pointer pairs for actors and shapes. A
// null-name lookup for a missing object still inserts a null association once
// the table exists; removing its last existing association destroys the table.
struct NxShapeNamePair
	{
	void* shape;
	const char* name;
	};

struct NxShapeNameTable
	{
	NxShapeNamePair* entries;
	unsigned count;
	unsigned capacity;
	unsigned reserved;
	};

static NxShapeNameTable* gNxShapeNames = 0;

void nxShapeSetName(void* shape, const char* name)
	{
	if(!shape) return;
	if(!gNxShapeNames)
		{
		if(!name) return;
		gNxShapeNames = static_cast<NxShapeNameTable*>(
			nxGetSdkAllocator()->malloc(sizeof(NxShapeNameTable), NX_MEMORY_PERSISTENT));
		if(!gNxShapeNames) return;
		memset(gNxShapeNames, 0, sizeof(*gNxShapeNames));
		}
	for(unsigned i = 0; i < gNxShapeNames->count; ++i)
		if(gNxShapeNames->entries[i].shape == shape)
			{
			if(name)
				{
				gNxShapeNames->entries[i].name = name;
				return;
				}
			gNxShapeNames->entries[i] =
				gNxShapeNames->entries[--gNxShapeNames->count];
			if(!gNxShapeNames->count)
				{
				nxGetSdkAllocator()->free(gNxShapeNames->entries);
				nxGetSdkAllocator()->free(gNxShapeNames);
				gNxShapeNames = 0;
				}
			return;
			}
	if(gNxShapeNames->count == gNxShapeNames->capacity)
		{
		const unsigned capacity = gNxShapeNames->count * 2 + 2;
		NxShapeNamePair* entries = static_cast<NxShapeNamePair*>(
			nxGetSdkAllocator()->malloc(
				capacity * sizeof(NxShapeNamePair), NX_MEMORY_PERSISTENT));
		if(!entries) return;
		if(gNxShapeNames->count)
			memcpy(entries, gNxShapeNames->entries,
				gNxShapeNames->count * sizeof(NxShapeNamePair));
		if(gNxShapeNames->entries)
			nxGetSdkAllocator()->free(gNxShapeNames->entries);
		gNxShapeNames->entries = entries;
		gNxShapeNames->capacity = capacity;
		}
	gNxShapeNames->entries[gNxShapeNames->count].shape = shape;
	gNxShapeNames->entries[gNxShapeNames->count].name = name;
	++gNxShapeNames->count;
	}

void nxShapeReleaseNameTable()
	{
	if(!gNxShapeNames) return;
	if(gNxShapeNames->entries)
		nxGetSdkAllocator()->free(gNxShapeNames->entries);
	nxGetSdkAllocator()->free(gNxShapeNames);
	gNxShapeNames = 0;
	}

const char* nxShapeGetName(void* shape)
	{
	if(!gNxShapeNames || !shape) return 0;
	for(unsigned i = 0; i < gNxShapeNames->count; ++i)
		if(gNxShapeNames->entries[i].shape == shape)
			return gNxShapeNames->entries[i].name;
	return 0;
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
			slots[5] = reinterpret_cast<void*>(&nxBoxHandleSetFlag);
			slots[6] = reinterpret_cast<void*>(&nxBoxHandleGetFlag);
			slots[25] = reinterpret_cast<void*>(&nxBoxHandleSetMaterial);
			slots[26] = reinterpret_cast<void*>(&nxBoxHandleGetMaterial);
			slots[27] = reinterpret_cast<void*>(&nxBoxHandleGetType);
			slots[28] = reinterpret_cast<void*>(&nxBoxHandleIs);
			slots[29] = reinterpret_cast<void*>(&nxBoxHandleSetName);
			slots[30] = reinterpret_cast<void*>(&nxBoxHandleGetName);
			slots[32] = reinterpret_cast<void*>(&nxBoxHandleGetDimensions);
			}
		};
	static Table table;
	return table.slots;
	}

void NpActorObject::installVtable()
	{
	*reinterpret_cast<void**>(mBytes) = *reinterpret_cast<void**>(&gNpActorVtable);
	}


// phys_fn_000110 at 0x00003580, dynamic actor vtable slot 19. The shipped
// implementation tests the actor's body at +0x14, then its marker at +0x08.
// The lock calls around that read are an independent scene-lock dependency.
bool NpActorVtable::isDynamic() const
	{
	const unsigned char* actor = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(actor + 0x14);
	return body && *reinterpret_cast<const unsigned*>(body + 0x08) != 0;
	}

// (unimplemented) setGlobalPose
void NpActorVtable::setGlobalPose(const NxMat34&)
	{
	}

// (unimplemented) getPointVelocityVal
NxVec3 NpActorVtable::getPointVelocityVal(const NxVec3& point) const
	{
	(void)point;
	return NxVec3(0.0f, 0.0f, 0.0f);
	}

// (unimplemented) setGlobalPosition
void NpActorVtable::setGlobalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) setGlobalOrientation
void NpActorVtable::setGlobalOrientation(const NxMat33&)
	{
	
	}

// (unimplemented) setGlobalOrientationQuat
void NpActorVtable::setGlobalOrientationQuat(const NxQuat&)
	{
	
	}

// phys_fn_000130 at 0x00004580, actor vtable slot 5. It returns the same
// matrix and translation exposed by slots 7 and 6, respectively. The public
// drive checks all twelve words, including the quarter-turn precision case.
NxMat34 NpActorVtable::getGlobalPoseVal() const
	{
	NxMat34 pose;
	pose.M = getGlobalOrientationVal();
	pose.t = getGlobalPositionVal();
	return pose;
	}

// phys_fn_000092 at 0x00002ed0, actor vtable slot 6. The oracle reads the
// nested pose translation when body+8 is non-null, otherwise the outer body's
// translation at +0x44. The final actor fallback covers incomplete setup.
NxVec3 NpActorVtable::getGlobalPositionVal() const
	{
	const unsigned char* actor = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(actor + 0x14);
	const unsigned char* record = body
		? *reinterpret_cast<unsigned char* const*>(body + 0x08) : 0;
	const unsigned char* translation = record ? record + 0x50
		: (body ? body + 0x44 : actor + 0x44);
	NxVec3 result;
	memcpy(&result, translation, sizeof(result));
	return result;
	}

// phys_fn_000132 at 0x000046c0, actor vtable slot 7. The dynamic arm
// converts the quaternion in the nested record; the static arm copies the
// outer body's matrix at +0x20. Lock behavior remains a separate dependency.
NxMat33 NpActorVtable::getGlobalOrientationVal() const
	{
	const unsigned char* actor = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(actor + 0x14);
	const unsigned char* record = body
		? *reinterpret_cast<unsigned char* const*>(body + 0x08) : 0;
	NxMat33 orientation;
	if(record)
		{
		NxQuat quaternion;
		memcpy(&quaternion, record + 0x5c, sizeof(quaternion));
		// phys_fn_000132 evaluates the products on the x87 stack before
		// storing each float. Float intermediates in NxMat33::fromQuat move
		// the quarter-turn diagonal by several ULPs.
		const double x = quaternion.x, y = quaternion.y;
		const double z = quaternion.z, w = quaternion.w;
		float rows[9] = {
			static_cast<float>(1.0 - 2.0 * (y*y + z*z)),
			static_cast<float>(2.0 * (x*y - w*z)),
			static_cast<float>(2.0 * (x*z + w*y)),
			static_cast<float>(2.0 * (x*y + w*z)),
			static_cast<float>(1.0 - 2.0 * (x*x + z*z)),
			static_cast<float>(2.0 * (y*z - w*x)),
			static_cast<float>(2.0 * (x*z - w*y)),
			static_cast<float>(2.0 * (y*z + w*x)),
			static_cast<float>(1.0 - 2.0 * (x*x + y*y))
		};
		orientation.setRowMajor(rows);
		}
	else
		memcpy(&orientation, body ? body + 0x20 : actor + 0x20,
			sizeof(orientation));
	return orientation;
	}

// phys_fn_000094 at 0x00002f30, actor vtable slot 8. The dynamic arm
// copies the record's quaternion; the static arm converts the outer matrix.
NxQuat NpActorVtable::getGlobalOrientationQuatVal() const
	{
	const unsigned char* actor = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(actor + 0x14);
	const unsigned char* record = body
		? *reinterpret_cast<unsigned char* const*>(body + 0x08) : 0;
	if(record)
		{
		NxQuat quaternion;
		memcpy(&quaternion, record + 0x5c, sizeof(quaternion));
		return quaternion;
		}
	NxMat33 orientation;
	memcpy(&orientation, body ? body + 0x20 : actor + 0x20,
		sizeof(orientation));
	return NxQuat(orientation);
	}

// (unimplemented) getGlobalPoseReference
const NxMat34 & NpActorVtable::getGlobalPoseReference() const
	{
	static NxMat34 sValue; return sValue;
	}

// (unimplemented) moveGlobalPose
void NpActorVtable::moveGlobalPose(const NxMat34&)
	{
	
	}

// (unimplemented) moveGlobalPosition
void NpActorVtable::moveGlobalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) moveGlobalOrientation
void NpActorVtable::moveGlobalOrientation(const NxMat33&)
	{
	
	}

// (unimplemented) createShape
NxShape* NpActorVtable::createShape(const NxShapeDesc&)
	{
	return 0;
	}

// (unimplemented) releaseShape
void NpActorVtable::releaseShape(NxShape&)
	{
	
	}

// phys_fn_000082 (0x00002d00) delegates to the outer body's shape holder.
// A single shape contributes one public handle; kind 5 is the group whose
// child array at +0xe0/+0xe4 determines the count.
NxU32 NpActorVtable::getNbShapes() const
	{
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
NxShape** NpActorVtable::getShapes() const
	{
	const unsigned char* actor = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(actor + 0x14);
	const unsigned char* shape = body
		? *reinterpret_cast<unsigned char* const*>(body + 0x10) : 0;
	if(!shape) return 0;
	if(*reinterpret_cast<const unsigned*>(shape + 0xd0) == 5)
		return *reinterpret_cast<NxShape** const*>(shape + 0xf0);
	return reinterpret_cast<NxShape**>(const_cast<unsigned char*>(shape + 0x9c));
	}

// (unimplemented) updateMassFromShapes
void NpActorVtable::updateMassFromShapes(NxReal density, NxReal totalMass)
	{
	
	}

// (unimplemented) setDynamic
void NpActorVtable::setDynamic(const NxBodyDesc&)
	{
	
	}

static void nxNpActorRefreshCMass(unsigned char* record)
	{
	float rotation[9];
	nxNpActorRotationFromQuaternionAt(record, 0x24, rotation);
	const float* local = reinterpret_cast<const float*>(record + 0x100);
	const float* actorPosition = reinterpret_cast<const float*>(record + 0x18);
	float* world = reinterpret_cast<float*>(record + 0x158);
	world[0] = static_cast<float>(
		static_cast<double>(rotation[2]) * local[2] +
		static_cast<double>(rotation[1]) * local[1] +
		static_cast<double>(rotation[0]) * local[0] + actorPosition[0]);
	const volatile float yTranslationAndX = static_cast<float>(actorPosition[1] +
		static_cast<double>(rotation[3]) * local[0]);
	world[1] = static_cast<float>(static_cast<double>(yTranslationAndX) +
		static_cast<double>(rotation[5]) * local[2] +
		static_cast<double>(rotation[4]) * local[1]);
	world[2] = static_cast<float>(actorPosition[2] +
		static_cast<double>(rotation[6]) * local[0] +
		static_cast<double>(rotation[8]) * local[2] +
		static_cast<double>(rotation[7]) * local[1]);
	nxNpActorUpdateInertiaMatrices(record);
	nxNpActorUpdateCMassQuaternion(record);
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

void NpActorVtable::setCMassOffsetLocalPose(const NxMat34& pose)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0x100, &pose.t, sizeof(NxVec3));
		nxNpActorMarkRecordDirty(record, 0x200);
		++*reinterpret_cast<unsigned*>(record + 0x198);
		pose.M.getRowMajor(reinterpret_cast<float*>(record + 0xdc));
		nxNpActorMarkRecordDirty(record, 0x400);
		++*reinterpret_cast<unsigned*>(record + 0x198);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::setCMassOffsetLocalPosition(const NxVec3& position)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0x100, &position, sizeof(NxVec3));
		nxNpActorMarkRecordDirty(record, 0x200);
		++*reinterpret_cast<unsigned*>(record + 0x198);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::setCMassOffsetLocalOrientation(const NxMat33& orientation)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		orientation.getRowMajor(reinterpret_cast<float*>(record + 0xdc));
		nxNpActorMarkRecordDirty(record, 0x400);
		++*reinterpret_cast<unsigned*>(record + 0x198);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	nxNpSceneGuardLeave(ctx);
	}

static void nxNpActorStoreGlobalMassPosition(unsigned char* record,
	const NxVec3& worldPosition)
	{
	float rotation[9];
	nxNpActorRotationFromQuaternion(record, rotation);
	const float* actorPosition = reinterpret_cast<const float*>(record + 0x50);
	const float delta[3] = {worldPosition.x - actorPosition[0],
		worldPosition.y - actorPosition[1],
		worldPosition.z - actorPosition[2]};
	float* local = reinterpret_cast<float*>(record + 0x100);
	for(unsigned col = 0; col < 3; ++col)
		local[col] = static_cast<float>(
			static_cast<double>(rotation[col]) * delta[0] +
			static_cast<double>(rotation[3 + col]) * delta[1] +
			static_cast<double>(rotation[6 + col]) * delta[2]);
	nxNpActorMarkRecordDirty(record, 0x200);
	++*reinterpret_cast<unsigned*>(record + 0x198);
	}

static void nxNpActorStoreGlobalMassOrientation(unsigned char* record,
	const NxMat33& worldOrientation)
	{
	float rotation[9];
	float requested[9];
	nxNpActorRotationFromQuaternion(record, rotation);
	worldOrientation.getRowMajor(requested);
	float* local = reinterpret_cast<float*>(record + 0xdc);
	for(unsigned row = 0; row < 3; ++row)
		for(unsigned col = 0; col < 3; ++col)
			local[row * 3 + col] = static_cast<float>(
				static_cast<double>(rotation[6 + row]) * requested[6 + col] +
				static_cast<double>(rotation[3 + row]) * requested[3 + col] +
				static_cast<double>(rotation[row]) * requested[col]);
	nxNpActorMarkRecordDirty(record, 0x400);
	++*reinterpret_cast<unsigned*>(record + 0x198);
	}

void NpActorVtable::setCMassOffsetGlobalPose(const NxMat34& pose)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		nxNpActorStoreGlobalMassPosition(record, pose.t);
		nxNpActorStoreGlobalMassOrientation(record, pose.M);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::setCMassOffsetGlobalPosition(const NxVec3& position)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		nxNpActorStoreGlobalMassPosition(record, position);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::setCMassOffsetGlobalOrientation(const NxMat33& orientation)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		nxNpActorStoreGlobalMassOrientation(record, orientation);
		nxNpActorRefreshCMass(record);
		nxNpActorWakeAfterCMassWrite(record);
		}
	nxNpSceneGuardLeave(ctx);
	}

// (unimplemented) setCMassGlobalPose
void NpActorVtable::setCMassGlobalPose(const NxMat34&)
	{
	
	}

// (unimplemented) setCMassGlobalPosition
void NpActorVtable::setCMassGlobalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) setCMassGlobalOrientation
void NpActorVtable::setCMassGlobalOrientation(const NxMat33&)
	{
	
	}

static NxMat33 nxNpActorCMassMatrix(const unsigned char* record, unsigned offset)
	{
	NxMat33 result(NX_IDENTITY_MATRIX);
	if(record) result.setRowMajor(reinterpret_cast<const float*>(record + offset));
	return result;
	}

static NxVec3 nxNpActorCMassPosition(const unsigned char* record, unsigned offset)
	{
	NxVec3 result(0.0f, 0.0f, 0.0f);
	if(record) memcpy(&result, record + offset, sizeof(result));
	return result;
	}

NxMat34 NpActorVtable::getCMassLocalPoseVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	const unsigned char* record = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	NxMat34 result(nxNpActorCMassMatrix(record, 0xdc),
		nxNpActorCMassPosition(record, 0x100));
	nxNpSceneGuardLeave(ctx);
	return result;
	}

NxVec3 NpActorVtable::getCMassLocalPositionVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	NxVec3 result = nxNpActorCMassPosition(
		nxNpActorRecord(const_cast<NpActorVtable*>(this)), 0x100);
	nxNpSceneGuardLeave(ctx);
	return result;
	}

NxMat33 NpActorVtable::getCMassLocalOrientationVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	NxMat33 result = nxNpActorCMassMatrix(
		nxNpActorRecord(const_cast<NpActorVtable*>(this)), 0xdc);
	nxNpSceneGuardLeave(ctx);
	return result;
	}

NxMat34 NpActorVtable::getCMassGlobalPoseVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	const unsigned char* record = nxNpActorRecord(const_cast<NpActorVtable*>(this));
	NxMat34 result(nxNpActorCMassMatrix(record, 0x134),
		nxNpActorCMassPosition(record, 0x158));
	nxNpSceneGuardLeave(ctx);
	return result;
	}

NxVec3 NpActorVtable::getCMassGlobalPositionVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	NxVec3 result = nxNpActorCMassPosition(
		nxNpActorRecord(const_cast<NpActorVtable*>(this)), 0x158);
	nxNpSceneGuardLeave(ctx);
	return result;
	}

NxMat33 NpActorVtable::getCMassGlobalOrientationVal() const
	{
	void* ctx = nxNpActorContext(const_cast<NpActorVtable*>(this), 0x10);
	nxNpSceneGuardEnter(ctx);
	NxMat33 result = nxNpActorCMassMatrix(
		nxNpActorRecord(const_cast<NpActorVtable*>(this)), 0x134);
	nxNpSceneGuardLeave(ctx);
	return result;
	}

void NpActorVtable::setMass(NxReal mass)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && mass > 0.0f)
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

void NpActorVtable::setMassSpaceInertiaTensor(const NxVec3& m)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		memcpy(record + 0x18c, &m, sizeof(m));
		float* inverse = reinterpret_cast<float*>(record + 0xc4);
		if(m.x > 0.0f && m.y > 0.0f && m.z > 0.0f)
			{
			inverse[0] = 1.0f / m.x;
			inverse[1] = 1.0f / m.y;
			inverse[2] = 1.0f / m.z;
			}
		else
			memset(inverse, 0, sizeof(NxVec3));
		nxNpActorMarkRecordDirty(record, 0x20000);
		}
	nxNpSceneGuardLeave(ctx);
	}

NxVec3 NpActorVtable::getMassSpaceInertiaTensorVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record) memcpy(&out, record + 0x18c, sizeof(out));
	nxNpSceneGuardLeave(ctx);
	return out;
	}

NxMat33 NpActorVtable::getGlobalInertiaTensorVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	NxMat33 out = nxNpActorInstantTensor(nxNpActorRecord(self), 0x18c, false);
	nxNpSceneGuardLeave(ctx);
	return out;
	}

NxMat33 NpActorVtable::getGlobalInertiaTensorInverseVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	NxMat33 out = nxNpActorInstantTensor(nxNpActorRecord(self), 0xc4, false);
	nxNpSceneGuardLeave(ctx);
	return out;
	}

void NpActorVtable::setLinearDamping(NxReal damping)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && damping >= 0.0f)
		{
		*reinterpret_cast<float*>(record + 0xb8) = damping;
		nxNpActorMarkRecordDirty(record, 0x800);
		}
	nxNpSceneGuardLeave(ctx);
	}

NxReal NpActorVtable::getLinearDamping() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	const NxReal out = record
		? *reinterpret_cast<NxReal*>(record + 0xb8) : NxReal();
	nxNpSceneGuardLeave(ctx);
	return out;
	}

void NpActorVtable::setAngularDamping(NxReal damping)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && damping >= 0.0f)
		{
		*reinterpret_cast<float*>(record + 0xbc) = damping;
		nxNpActorMarkRecordDirty(record, 0x1000);
		}
	nxNpSceneGuardLeave(ctx);
	}

NxReal NpActorVtable::getAngularDamping() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	const NxReal out = record
		? *reinterpret_cast<NxReal*>(record + 0xbc) : NxReal();
	nxNpSceneGuardLeave(ctx);
	return out;
	}

void NpActorVtable::setLinearVelocity(const NxVec3& velocity)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0x6c, &velocity, sizeof(velocity));
		memcpy(record + 0x34, &velocity, sizeof(velocity));
		nxNpActorMarkRecordDirty(record, 4);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::setAngularVelocity(const NxVec3& velocity)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		memcpy(record + 0x78, &velocity, sizeof(velocity));
		memcpy(record + 0x40, &velocity, sizeof(velocity));
		nxNpActorMarkRecordDirty(record, 8);
		}
	nxNpSceneGuardLeave(ctx);
	}

NxVec3 NpActorVtable::getLinearVelocityVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record) memcpy(&out, record + 0x6c, sizeof(out));
	nxNpSceneGuardLeave(ctx);
	return out;
	}

NxVec3 NpActorVtable::getAngularVelocityVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record) memcpy(&out, record + 0x78, sizeof(out));
	nxNpSceneGuardLeave(ctx);
	return out;
	}

void NpActorVtable::setMaxAngularVelocity(NxReal limit)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<NxReal*>(record + 0xd8) = limit * limit;
		nxNpActorMarkRecordDirty(record, 0x8000);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::setLinearMomentum(const NxVec3& momentum)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		const NxReal inverseMass = *reinterpret_cast<NxReal*>(record + 0xc0);
		NxVec3 velocity(inverseMass * momentum.x,
			inverseMass * momentum.y, inverseMass * momentum.z);
		memcpy(record + 0x6c, &velocity, sizeof(velocity));
		memcpy(record + 0x34, &velocity, sizeof(velocity));
		nxNpActorMarkRecordDirty(record, 4);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::setAngularMomentum(const NxVec3& momentum)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		{
		const NxReal* inverse = reinterpret_cast<const NxReal*>(record + 0x164);
		NxVec3 velocity(
			static_cast<NxReal>(static_cast<double>(inverse[0]) * momentum.x +
				static_cast<double>(inverse[2]) * momentum.z +
				static_cast<double>(inverse[1]) * momentum.y),
			static_cast<NxReal>(static_cast<double>(inverse[5]) * momentum.z +
				static_cast<double>(inverse[3]) * momentum.x +
				static_cast<double>(inverse[4]) * momentum.y),
			static_cast<NxReal>(static_cast<double>(inverse[8]) * momentum.z +
				static_cast<double>(inverse[6]) * momentum.x +
				static_cast<double>(inverse[7]) * momentum.y));
		memcpy(record + 0x78, &velocity, sizeof(velocity));
		memcpy(record + 0x40, &velocity, sizeof(velocity));
		nxNpActorMarkRecordDirty(record, 8);
		}
	nxNpSceneGuardLeave(ctx);
	}

NxVec3 NpActorVtable::getLinearMomentumVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
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

NxVec3 NpActorVtable::getAngularMomentumVal() const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	NxVec3 out(0.0f, 0.0f, 0.0f);
	if(record)
		{
		NxMat33 world = nxNpActorInstantTensor(record, 0x18c, true);
		float tensor[9];
		world.getRowMajor(tensor);
		const float* v = reinterpret_cast<const float*>(record + 0x78);
		out = NxVec3(
			tensor[0] * v[0] + tensor[1] * v[1] + tensor[2] * v[2],
			tensor[3] * v[0] + tensor[4] * v[1] + tensor[5] * v[2],
			tensor[6] * v[0] + tensor[7] * v[1] + tensor[8] * v[2]);
		}
	nxNpSceneGuardLeave(ctx);
	return out;
	}

static void nxNpActorAccumulateForce(unsigned char* record,
	const NxVec3& value, NxForceMode mode, bool angular);
static NxVec3 nxNpActorRotateLocalForce(const unsigned char* record,
	const NxVec3& local);

static void nxNpActorForceAtPos(unsigned char* record, const NxVec3& force,
	const NxVec3& worldPosition, NxForceMode mode)
	{
	const float* center = reinterpret_cast<const float*>(record + 0x158);
	const NxVec3 lever(worldPosition.x - center[0],
		worldPosition.y - center[1], worldPosition.z - center[2]);
	const NxVec3 torque(lever.y * force.z - lever.z * force.y,
		lever.z * force.x - lever.x * force.z,
		lever.x * force.y - lever.y * force.x);
	nxNpActorAccumulateForce(record, force, mode, false);
	nxNpActorAccumulateForce(record, torque, mode, true);
	}

static NxVec3 nxNpActorLocalPosition(const unsigned char* record,
	const NxVec3& position)
	{
	float rotation[9];
	nxNpActorRotationFromQuaternion(record, rotation);
	const float* translation = reinterpret_cast<const float*>(record + 0x50);
	return NxVec3(
		static_cast<float>(static_cast<double>(rotation[0]) * position.x +
			static_cast<double>(rotation[2]) * position.z +
			static_cast<double>(rotation[1]) * position.y + translation[0]),
		static_cast<float>(static_cast<double>(rotation[3]) * position.x +
			static_cast<double>(rotation[5]) * position.z +
			static_cast<double>(rotation[4]) * position.y + translation[1]),
		static_cast<float>(static_cast<double>(rotation[6]) * position.x +
			static_cast<double>(rotation[8]) * position.z +
			static_cast<double>(rotation[7]) * position.y + translation[2]));
	}

void NpActorVtable::addForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorForceAtPos(record, force, pos, mode);
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::addForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorForceAtPos(record, force,
			nxNpActorLocalPosition(record, pos), mode);
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::addLocalForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorForceAtPos(record,
			nxNpActorRotateLocalForce(record, force), pos, mode);
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::addLocalForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorForceAtPos(record,
			nxNpActorRotateLocalForce(record, force),
			nxNpActorLocalPosition(record, pos), mode);
	nxNpSceneGuardLeave(ctx);
	}

static void nxNpActorAccumulateForce(unsigned char* record,
	const NxVec3& value, NxForceMode mode, bool angular)
	{
	unsigned target;
	unsigned mask;
	switch(mode)
		{
		case NX_FORCE:
			target = angular ? 0x94 : 0x88;
			mask = angular ? 0x40 : 0x20;
			break;
		case NX_IMPULSE:
		case NX_VELOCITY_CHANGE:
			target = angular ? 0x78 : 0x6c;
			mask = angular ? 8 : 4;
			break;
		case NX_SMOOTH_IMPULSE:
		case NX_SMOOTH_VELOCITY_CHANGE:
			target = angular ? 0xac : 0xa0;
			mask = angular ? 0x100 : 0x80;
			break;
		default:
			return;
		}
	const float input[3] = { value.x, value.y, value.z };
	float increment[3];
	if(mode == NX_VELOCITY_CHANGE || mode == NX_SMOOTH_VELOCITY_CHANGE)
		memcpy(increment, input, sizeof(increment));
	else if(!angular)
		{
		const float inverseMass = *reinterpret_cast<float*>(record + 0xc0);
		for(unsigned i = 0; i < 3; ++i) increment[i] = inverseMass * input[i];
		}
	else
		{
		const float* inverse = reinterpret_cast<const float*>(record + 0x164);
		for(unsigned i = 0; i < 3; ++i)
			increment[i] = static_cast<float>(
				static_cast<double>(inverse[i * 3]) * input[0] +
				static_cast<double>(inverse[i * 3 + 1]) * input[1] +
				static_cast<double>(inverse[i * 3 + 2]) * input[2]);
		}
	float* destination = reinterpret_cast<float*>(record + target);
	for(unsigned i = 0; i < 3; ++i)
		destination[i] += increment[i];
	if(target == 0x6c || target == 0x78)
		memcpy(record + (angular ? 0x40 : 0x34), destination, sizeof(NxVec3));
	nxNpActorMarkRecordDirty(record, mask);
	if((*reinterpret_cast<unsigned*>(record + 0x114) & 0x100u) == 0 &&
		*reinterpret_cast<float*>(record + 0x84) < 0.39999998f)
		{
		*reinterpret_cast<unsigned*>(record + 0x84) = 0x3eccccccu;
		*reinterpret_cast<unsigned*>(record + 0x4c) = 0x3eccccccu;
		nxNpActorMarkRecordDirty(record, 0x10);
		}
	}

void NpActorVtable::addForce(const NxVec3& force, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorAccumulateForce(record, force, mode, false);
	nxNpSceneGuardLeave(ctx);
	}

static NxVec3 nxNpActorRotateLocalForce(const unsigned char* record,
	const NxVec3& local)
	{
	NxQuat quaternion;
	memcpy(&quaternion, record + 0x5c, sizeof(quaternion));
	NxMat33 rotation(quaternion);
	float m[9];
	rotation.getRowMajor(m);
	return NxVec3(
		static_cast<float>(static_cast<double>(m[0]) * local.x +
			static_cast<double>(m[2]) * local.z +
			static_cast<double>(m[1]) * local.y),
		static_cast<float>(static_cast<double>(m[3]) * local.x +
			static_cast<double>(m[5]) * local.z +
			static_cast<double>(m[4]) * local.y),
		static_cast<float>(static_cast<double>(m[6]) * local.x +
			static_cast<double>(m[8]) * local.z +
			static_cast<double>(m[7]) * local.y));
	}

void NpActorVtable::addLocalForce(const NxVec3& force, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorAccumulateForce(record,
			nxNpActorRotateLocalForce(record, force), mode, false);
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::addTorque(const NxVec3& torque, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorAccumulateForce(record, torque, mode, true);
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::addLocalTorque(const NxVec3& torque, NxForceMode mode )
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record && (*reinterpret_cast<unsigned*>(record + 0x10c) & 0x80u) == 0)
		nxNpActorAccumulateForce(record,
			nxNpActorRotateLocalForce(record, torque), mode, true);
	nxNpSceneGuardLeave(ctx);
	}

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
		const double rotational =
			static_cast<double>(inertia[0]) * angular[0] * angular[0] +
			static_cast<double>(inertia[1]) * angular[1] * angular[1] +
			static_cast<double>(inertia[2]) * angular[2] * angular[2];
		const double translational =
			(static_cast<double>(linear[0]) * linear[0] +
			static_cast<double>(linear[1]) * linear[1] +
			static_cast<double>(linear[2]) * linear[2]) * mass;
		energy = static_cast<NxReal>((rotational + translational) * 0.5);
		}
	nxNpSceneGuardLeave(ctx);
	return energy;
	}

// (unimplemented) getLocalPointVelocityVal
NxVec3 NpActorVtable::getLocalPointVelocityVal(const NxVec3& point) const
	{
	return NxVec3();
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

void NpActorVtable::setSleepLinearVelocity(NxReal threshold)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
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

void NpActorVtable::setSleepAngularVelocity(NxReal threshold)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<NxReal*>(record + 0xd4) = threshold * threshold;
		nxNpActorMarkRecordDirty(record, 0x4000);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::wakeUp(NxReal wakeCounterValue)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		*reinterpret_cast<NxReal*>(record + 0x84) = wakeCounterValue;
		*reinterpret_cast<NxReal*>(record + 0x4c) = wakeCounterValue;
		nxNpActorMarkRecordDirty(record, 0x10);
		*reinterpret_cast<unsigned*>(record + 0x114) &= ~0x100u;
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::putToSleep()
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
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
void NpActorVtable::raiseActorFlag(NxActorFlag flag)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* body = nxNpActorBody(this);
	if(body) *reinterpret_cast<unsigned*>(body + 0x14) |=
		static_cast<unsigned>(flag);
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::clearActorFlag(NxActorFlag flag)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
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

void NpActorVtable::raiseBodyFlag(NxBodyFlag flag)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		if(static_cast<unsigned>(flag) & 0x80u)
			nxNpActorTransitionKinematic(record, true);
		*reinterpret_cast<unsigned*>(record + 0x10c) |=
			static_cast<unsigned>(flag);
		nxNpActorMarkRecordDirty(record, 0x80000);
		}
	nxNpSceneGuardLeave(ctx);
	}

void NpActorVtable::clearBodyFlag(NxBodyFlag flag)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
	unsigned char* record = nxNpActorRecord(this);
	if(record)
		{
		if(static_cast<unsigned>(flag) & 0x80u)
			nxNpActorTransitionKinematic(record, false);
		*reinterpret_cast<unsigned*>(record + 0x10c) &=
			~static_cast<unsigned>(flag);
		nxNpActorMarkRecordDirty(record, 0x80000);
		}
	nxNpSceneGuardLeave(ctx);
	}

bool NpActorVtable::readBodyFlag(NxBodyFlag flag) const
	{
	void* self = const_cast<NpActorVtable*>(this);
	void* ctx = nxNpActorContext(self, 0x10);
	nxNpSceneGuardEnter(ctx);
	unsigned char* record = nxNpActorRecord(self);
	const bool out = record && (*reinterpret_cast<unsigned*>(record + 0x10c) &
		static_cast<unsigned>(flag)) != 0;
	nxNpSceneGuardLeave(ctx);
	return out;
	}

// (unimplemented) saveBodyToDesc
bool NpActorVtable::saveBodyToDesc(NxBodyDesc&)
	{
	return bool();
	}

// (unimplemented) saveToDesc
void NpActorVtable::saveToDesc(NxActorDescBase&)
	{
	
	}

// Concrete actor slots 83/84 address the same body-keyed global name map as
// Actor::loadFromDescInternal (oracle 0x2d90/0x2d60).
void NpActorVtable::setName(const char* name)
	{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(this) + 0x14);
	if(body) nxShapeSetName(body, name);
	}

const char* NpActorVtable::getName() const
	{
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(this) + 0x14);
	return nxShapeGetName(const_cast<unsigned char*>(body));
	}

// Concrete actor slots 85/86 address the body +0x1c group word.
void NpActorVtable::setGroup(NxActorGroup group)
	{
	void* ctx = nxNpActorContext(this, 0xc);
	if(!nxNpSceneGuardWriteTry(ctx)) return;
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
