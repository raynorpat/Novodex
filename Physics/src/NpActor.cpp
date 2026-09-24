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
#include <string.h>

// The vtable word. A single static instance of the concrete class supplies it: the
// object needs a vtable POINTER, not a class instance, so one instance is enough for
// every actor the reconstruction builds.
static NpActorVtable gNpActorVtable;

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

// (unimplemented) getGlobalPoseVal
NxMat34 NpActorVtable::getGlobalPoseVal() const
	{
	return NxMat34();
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

// (unimplemented) getNbShapes
NxU32 NpActorVtable::getNbShapes() const
	{
	return NxU32();
	}

// (unimplemented) getShapes
NxShape** NpActorVtable::getShapes() const
	{
	return 0;
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
