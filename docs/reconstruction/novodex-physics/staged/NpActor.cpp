/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "NpActor.h"

NpActor::NpActor() {}
NpActor::~NpActor() {}

// The virtual NxJointDesc::isValid() calls. A descriptor with a body and a
// density describes a dynamic actor, which is what the harness builds.
bool NpActor::isDynamic() const { return true; }

// (unimplemented) setGlobalPosition
void NpActor::setGlobalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) setGlobalOrientation
void NpActor::setGlobalOrientation(const NxMat33&)
	{
	
	}

// (unimplemented) setGlobalOrientationQuat
void NpActor::setGlobalOrientationQuat(const NxQuat&)
	{
	
	}

// (unimplemented) getGlobalPoseVal
NxMat34 NpActor::getGlobalPoseVal() const
	{
	return 0;
	}

// (unimplemented) getGlobalPositionVal
NxVec3 NpActor::getGlobalPositionVal() const
	{
	return 0;
	}

// (unimplemented) getGlobalOrientationVal
NxMat33 NpActor::getGlobalOrientationVal() const
	{
	return 0;
	}

// (unimplemented) getGlobalOrientationQuatVal
NxQuat NpActor::getGlobalOrientationQuatVal() const
	{
	return 0;
	}

// (unimplemented) getGlobalPoseReference
const NxMat34 & NpActor::getGlobalPoseReference() const
	{
	return 0;
	}

// (unimplemented) moveGlobalPose
void NpActor::moveGlobalPose(const NxMat34&)
	{
	
	}

// (unimplemented) moveGlobalPosition
void NpActor::moveGlobalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) moveGlobalOrientation
void NpActor::moveGlobalOrientation(const NxMat33&)
	{
	
	}

// (unimplemented) createShape
NxShape* NpActor::createShape(const NxShapeDesc&)
	{
	return 0;
	}

// (unimplemented) releaseShape
void NpActor::releaseShape(NxShape&)
	{
	
	}

// (unimplemented) getNbShapes
NxU32 NpActor::getNbShapes() const
	{
	return 0;
	}

// (unimplemented) getShapes
NxShape** NpActor::getShapes() const
	{
	return 0;
	}

// (unimplemented) updateMassFromShapes
void NpActor::updateMassFromShapes(NxReal density, NxReal totalMass)
	{
	
	}

// (unimplemented) setDynamic
void NpActor::setDynamic(const NxBodyDesc&)
	{
	
	}

// (unimplemented) setCMassOffsetLocalPose
void NpActor::setCMassOffsetLocalPose(const NxMat34&)
	{
	
	}

// (unimplemented) setCMassOffsetLocalPosition
void NpActor::setCMassOffsetLocalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) setCMassOffsetLocalOrientation
void NpActor::setCMassOffsetLocalOrientation(const NxMat33&)
	{
	
	}

// (unimplemented) setCMassOffsetGlobalPose
void NpActor::setCMassOffsetGlobalPose(const NxMat34&)
	{
	
	}

// (unimplemented) setCMassOffsetGlobalPosition
void NpActor::setCMassOffsetGlobalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) setCMassOffsetGlobalOrientation
void NpActor::setCMassOffsetGlobalOrientation(const NxMat33&)
	{
	
	}

// (unimplemented) setCMassGlobalPose
void NpActor::setCMassGlobalPose(const NxMat34&)
	{
	
	}

// (unimplemented) setCMassGlobalPosition
void NpActor::setCMassGlobalPosition(const NxVec3&)
	{
	
	}

// (unimplemented) setCMassGlobalOrientation
void NpActor::setCMassGlobalOrientation(const NxMat33&)
	{
	
	}

// (unimplemented) getCMassLocalPoseVal
NxMat34 NpActor::getCMassLocalPoseVal() const
	{
	return 0;
	}

// (unimplemented) getCMassLocalPositionVal
NxVec3 NpActor::getCMassLocalPositionVal() const
	{
	return 0;
	}

// (unimplemented) getCMassLocalOrientationVal
NxMat33 NpActor::getCMassLocalOrientationVal() const
	{
	return 0;
	}

// (unimplemented) getCMassGlobalPoseVal
NxMat34 NpActor::getCMassGlobalPoseVal() const
	{
	return 0;
	}

// (unimplemented) getCMassGlobalPositionVal
NxVec3 NpActor::getCMassGlobalPositionVal() const
	{
	return 0;
	}

// (unimplemented) getCMassGlobalOrientationVal
NxMat33 NpActor::getCMassGlobalOrientationVal() const
	{
	return 0;
	}

// (unimplemented) setMass
void NpActor::setMass(NxReal)
	{
	
	}

// (unimplemented) getMass
NxReal NpActor::getMass() const
	{
	return 0;
	}

// (unimplemented) setMassSpaceInertiaTensor
void NpActor::setMassSpaceInertiaTensor(const NxVec3& m)
	{
	
	}

// (unimplemented) getMassSpaceInertiaTensorVal
NxVec3 NpActor::getMassSpaceInertiaTensorVal() const
	{
	return 0;
	}

// (unimplemented) getGlobalInertiaTensorVal
NxMat33 NpActor::getGlobalInertiaTensorVal() const
	{
	return 0;
	}

// (unimplemented) getGlobalInertiaTensorInverseVal
NxMat33 NpActor::getGlobalInertiaTensorInverseVal() const
	{
	return 0;
	}

// (unimplemented) setLinearDamping
void NpActor::setLinearDamping(NxReal)
	{
	
	}

// (unimplemented) getLinearDamping
NxReal NpActor::getLinearDamping() const
	{
	return 0;
	}

// (unimplemented) setAngularDamping
void NpActor::setAngularDamping(NxReal)
	{
	
	}

// (unimplemented) getAngularDamping
NxReal NpActor::getAngularDamping() const
	{
	return 0;
	}

// (unimplemented) setLinearVelocity
void NpActor::setLinearVelocity(const NxVec3&)
	{
	
	}

// (unimplemented) setAngularVelocity
void NpActor::setAngularVelocity(const NxVec3&)
	{
	
	}

// (unimplemented) getLinearVelocityVal
NxVec3 NpActor::getLinearVelocityVal() const
	{
	return 0;
	}

// (unimplemented) getAngularVelocityVal
NxVec3 NpActor::getAngularVelocityVal() const
	{
	return 0;
	}

// (unimplemented) setMaxAngularVelocity
void NpActor::setMaxAngularVelocity(NxReal)
	{
	
	}

// (unimplemented) setLinearMomentum
void NpActor::setLinearMomentum(const NxVec3&)
	{
	
	}

// (unimplemented) setAngularMomentum
void NpActor::setAngularMomentum(const NxVec3&)
	{
	
	}

// (unimplemented) getLinearMomentumVal
NxVec3 NpActor::getLinearMomentumVal() const
	{
	return 0;
	}

// (unimplemented) getAngularMomentumVal
NxVec3 NpActor::getAngularMomentumVal() const
	{
	return 0;
	}

// (unimplemented) addForceAtPos
void NpActor::addForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	
	}

// (unimplemented) addForceAtLocalPos
void NpActor::addForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	
	}

// (unimplemented) addLocalForceAtPos
void NpActor::addLocalForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	
	}

// (unimplemented) addLocalForceAtLocalPos
void NpActor::addLocalForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode )
	{
	
	}

// (unimplemented) addForce
void NpActor::addForce(const NxVec3&, NxForceMode mode )
	{
	
	}

// (unimplemented) addLocalForce
void NpActor::addLocalForce(const NxVec3&, NxForceMode mode )
	{
	
	}

// (unimplemented) addTorque
void NpActor::addTorque(const NxVec3&, NxForceMode mode )
	{
	
	}

// (unimplemented) addLocalTorque
void NpActor::addLocalTorque(const NxVec3&, NxForceMode mode )
	{
	
	}

// (unimplemented) computeKineticEnergy
NxReal NpActor::computeKineticEnergy() const
	{
	return 0;
	}

// (unimplemented) getPointVelocity
void NpActor::getPointVelocity(const NxVec3& point, NxVec3& result) const { result ) const
	{
	
	}

// (unimplemented) getLocalPointVelocityVal
NxVec3 NpActor::getLocalPointVelocityVal(const NxVec3& point) const
	{
	return 0;
	}

// (unimplemented) isGroupSleeping
bool NpActor::isGroupSleeping() const
	{
	return 0;
	}

// (unimplemented) isSleeping
bool NpActor::isSleeping() const
	{
	return 0;
	}

// (unimplemented) getSleepLinearVelocity
NxReal NpActor::getSleepLinearVelocity() const
	{
	return 0;
	}

// (unimplemented) setSleepLinearVelocity
void NpActor::setSleepLinearVelocity(NxReal threshold)
	{
	
	}

// (unimplemented) getSleepAngularVelocity
NxReal NpActor::getSleepAngularVelocity() const
	{
	return 0;
	}

// (unimplemented) setSleepAngularVelocity
void NpActor::setSleepAngularVelocity(NxReal threshold)
	{
	
	}

// (unimplemented) wakeUp
void NpActor::wakeUp(NxReal wakeCounterValue)
	{
	
	}

// (unimplemented) putToSleep
void NpActor::putToSleep()
	{
	
	}

// (unimplemented) raiseActorFlag
void NpActor::raiseActorFlag(NxActorFlag)
	{
	
	}

// (unimplemented) clearActorFlag
void NpActor::clearActorFlag(NxActorFlag)
	{
	
	}

// (unimplemented) readActorFlag
bool NpActor::readActorFlag(NxActorFlag) const
	{
	return 0;
	}

// (unimplemented) raiseBodyFlag
void NpActor::raiseBodyFlag(NxBodyFlag)
	{
	
	}

// (unimplemented) clearBodyFlag
void NpActor::clearBodyFlag(NxBodyFlag)
	{
	
	}

// (unimplemented) readBodyFlag
bool NpActor::readBodyFlag(NxBodyFlag) const
	{
	return 0;
	}

// (unimplemented) saveBodyToDesc
bool NpActor::saveBodyToDesc(NxBodyDesc&)
	{
	return 0;
	}

// (unimplemented) saveToDesc
void NpActor::saveToDesc(NxActorDescBase&)
	{
	
	}

// (unimplemented) setName
void NpActor::setName(const char*)
	{
	
	}

// (unimplemented) getName
const char* NpActor::getName() const
	{
	return 0;
	}

// (unimplemented) setGroup
void NpActor::setGroup(NxActorGroup)
	{
	
	}

// (unimplemented) getGroup
NxActorGroup NpActor::getGroup() const
	{
	return 0;
	}

