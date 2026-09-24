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

#include "NxMat34.h"
#include "NxMat33.h"
#include "NxVec3.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxShape.h"
#include "NxBoxShape.h"
#include <string.h>
#include <stdlib.h>

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

// (unimplemented) setCMassOffsetLocalPose
void NpActorVtable::setCMassOffsetLocalPose(const NxMat34&)
	{
	
	}

// (unimplemented) setCMassOffsetLocalPosition
void NpActorVtable::setCMassOffsetLocalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) setCMassOffsetLocalOrientation
void NpActorVtable::setCMassOffsetLocalOrientation(const NxMat33&)
	{
	
	}

// (unimplemented) setCMassOffsetGlobalPose
void NpActorVtable::setCMassOffsetGlobalPose(const NxMat34&)
	{
	
	}

// (unimplemented) setCMassOffsetGlobalPosition
void NpActorVtable::setCMassOffsetGlobalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) setCMassOffsetGlobalOrientation
void NpActorVtable::setCMassOffsetGlobalOrientation(const NxMat33&)
	{
	
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

// (unimplemented) getCMassLocalPoseVal
NxMat34 NpActorVtable::getCMassLocalPoseVal() const
	{
	return NxMat34();
	}

// (unimplemented) getCMassLocalPositionVal
NxVec3 NpActorVtable::getCMassLocalPositionVal() const
	{
	return NxVec3();
	}

// (unimplemented) getCMassLocalOrientationVal
NxMat33 NpActorVtable::getCMassLocalOrientationVal() const
	{
	return NxMat33();
	}

// (unimplemented) getCMassGlobalPoseVal
NxMat34 NpActorVtable::getCMassGlobalPoseVal() const
	{
	return NxMat34();
	}

// (unimplemented) getCMassGlobalPositionVal
NxVec3 NpActorVtable::getCMassGlobalPositionVal() const
	{
	return NxVec3();
	}

// (unimplemented) getCMassGlobalOrientationVal
NxMat33 NpActorVtable::getCMassGlobalOrientationVal() const
	{
	return NxMat33();
	}

// (unimplemented) setMass
void NpActorVtable::setMass(NxReal)
	{
	
	}

// (unimplemented) getMass
NxReal NpActorVtable::getMass() const
	{
	return NxReal();
	}

// (unimplemented) setMassSpaceInertiaTensor
void NpActorVtable::setMassSpaceInertiaTensor(const NxVec3& m)
	{
	
	}

// (unimplemented) getMassSpaceInertiaTensorVal
NxVec3 NpActorVtable::getMassSpaceInertiaTensorVal() const
	{
	return NxVec3();
	}

// (unimplemented) getGlobalInertiaTensorVal
NxMat33 NpActorVtable::getGlobalInertiaTensorVal() const
	{
	return NxMat33();
	}

// (unimplemented) getGlobalInertiaTensorInverseVal
NxMat33 NpActorVtable::getGlobalInertiaTensorInverseVal() const
	{
	return NxMat33();
	}

// (unimplemented) setLinearDamping
void NpActorVtable::setLinearDamping(NxReal)
	{
	
	}

// (unimplemented) getLinearDamping
NxReal NpActorVtable::getLinearDamping() const
	{
	return NxReal();
	}

// (unimplemented) setAngularDamping
void NpActorVtable::setAngularDamping(NxReal)
	{
	
	}

// (unimplemented) getAngularDamping
NxReal NpActorVtable::getAngularDamping() const
	{
	return NxReal();
	}

// (unimplemented) setLinearVelocity
void NpActorVtable::setLinearVelocity(const NxVec3&)
	{
	
	}

// (unimplemented) setAngularVelocity
void NpActorVtable::setAngularVelocity(const NxVec3&)
	{
	
	}

// (unimplemented) getLinearVelocityVal
NxVec3 NpActorVtable::getLinearVelocityVal() const
	{
	return NxVec3();
	}

// (unimplemented) getAngularVelocityVal
NxVec3 NpActorVtable::getAngularVelocityVal() const
	{
	return NxVec3();
	}

// (unimplemented) setMaxAngularVelocity
void NpActorVtable::setMaxAngularVelocity(NxReal)
	{
	
	}

// (unimplemented) setLinearMomentum
void NpActorVtable::setLinearMomentum(const NxVec3&)
	{
	
	}

// (unimplemented) setAngularMomentum
void NpActorVtable::setAngularMomentum(const NxVec3&)
	{
	
	}

// (unimplemented) getLinearMomentumVal
NxVec3 NpActorVtable::getLinearMomentumVal() const
	{
	return NxVec3();
	}

// (unimplemented) getAngularMomentumVal
NxVec3 NpActorVtable::getAngularMomentumVal() const
	{
	return NxVec3();
	}

// (unimplemented) addForceAtPos
void NpActorVtable::addForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	
	}

// (unimplemented) addForceAtLocalPos
void NpActorVtable::addForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	
	}

// (unimplemented) addLocalForceAtPos
void NpActorVtable::addLocalForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	
	}

// (unimplemented) addLocalForceAtLocalPos
void NpActorVtable::addLocalForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	
	}

// (unimplemented) addForce
void NpActorVtable::addForce(const NxVec3&, NxForceMode mode )
	{
	
	}

// (unimplemented) addLocalForce
void NpActorVtable::addLocalForce(const NxVec3&, NxForceMode mode )
	{
	
	}

// (unimplemented) addTorque
void NpActorVtable::addTorque(const NxVec3&, NxForceMode mode )
	{
	
	}

// (unimplemented) addLocalTorque
void NpActorVtable::addLocalTorque(const NxVec3&, NxForceMode mode )
	{
	
	}

// (unimplemented) computeKineticEnergy
NxReal NpActorVtable::computeKineticEnergy() const
	{
	return NxReal();
	}

// (unimplemented) getLocalPointVelocityVal
NxVec3 NpActorVtable::getLocalPointVelocityVal(const NxVec3& point) const
	{
	return NxVec3();
	}

// (unimplemented) isGroupSleeping
bool NpActorVtable::isGroupSleeping() const
	{
	return bool();
	}

// (unimplemented) isSleeping
bool NpActorVtable::isSleeping() const
	{
	return bool();
	}

// (unimplemented) getSleepLinearVelocity
NxReal NpActorVtable::getSleepLinearVelocity() const
	{
	return NxReal();
	}

// (unimplemented) setSleepLinearVelocity
void NpActorVtable::setSleepLinearVelocity(NxReal threshold)
	{
	
	}

// (unimplemented) getSleepAngularVelocity
NxReal NpActorVtable::getSleepAngularVelocity() const
	{
	return NxReal();
	}

// (unimplemented) setSleepAngularVelocity
void NpActorVtable::setSleepAngularVelocity(NxReal threshold)
	{
	
	}

// (unimplemented) wakeUp
void NpActorVtable::wakeUp(NxReal wakeCounterValue)
	{
	
	}

// (unimplemented) putToSleep
void NpActorVtable::putToSleep()
	{
	
	}

// (unimplemented) raiseActorFlag
void NpActorVtable::raiseActorFlag(NxActorFlag)
	{
	
	}

// (unimplemented) clearActorFlag
void NpActorVtable::clearActorFlag(NxActorFlag)
	{
	
	}

// (unimplemented) readActorFlag
bool NpActorVtable::readActorFlag(NxActorFlag) const
	{
	return bool();
	}

// (unimplemented) raiseBodyFlag
void NpActorVtable::raiseBodyFlag(NxBodyFlag)
	{
	
	}

// (unimplemented) clearBodyFlag
void NpActorVtable::clearBodyFlag(NxBodyFlag)
	{
	
	}

// (unimplemented) readBodyFlag
bool NpActorVtable::readBodyFlag(NxBodyFlag) const
	{
	return bool();
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

// (unimplemented) setName
void NpActorVtable::setName(const char*)
	{
	
	}

// (unimplemented) getName
const char* NpActorVtable::getName() const
	{
	return 0;
	}

// (unimplemented) setGroup
void NpActorVtable::setGroup(NxActorGroup)
	{
	
	}

// (unimplemented) getGroup
NxActorGroup NpActorVtable::getGroup() const
	{
	return NxActorGroup();
	}

// (unimplemented) setGlobalPose
void NpActorVtable::setGlobalPose(const NxVec3&, const NxMat33&)
	{
	}
