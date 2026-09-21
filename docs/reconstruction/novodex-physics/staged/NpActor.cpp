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

// The vtable word. A single static instance of the concrete class supplies it: the
// object needs a vtable POINTER, not a class instance, so one instance is enough for
// every actor the reconstruction builds.
static NpActorVtable gNpActorVtable;

void NpActorObject::installVtable()
	{
	*reinterpret_cast<void**>(mBytes) = *reinterpret_cast<void**>(&gNpActorVtable);
	}


// The one virtual a reconstructed path calls. A descriptor with a body and a density
// describes a dynamic actor, which is what the harness builds.
bool NpActorVtable::isDynamic() const { return true; }

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

// (unimplemented) getGlobalPositionVal
NxVec3 NpActorVtable::getGlobalPositionVal() const
	{
	return NxVec3();
	}

// (unimplemented) getGlobalOrientationVal
NxMat33 NpActorVtable::getGlobalOrientationVal() const
	{
	return NxMat33();
	}

// (unimplemented) getGlobalOrientationQuatVal
NxQuat NpActorVtable::getGlobalOrientationQuatVal() const
	{
	return NxQuat();
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

