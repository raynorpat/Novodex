#ifndef NX_PHYSICS_NPACTOR
#define NX_PHYSICS_NPACTOR
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The NxActor the reconstruction builds. NxActor declares 83 pure virtuals, so a
// concrete class is needed for any actor at all -- and NxJointDesc::isValid()
// makes a virtual call through it (isDynamic, slot 18), which is the fault that
// ran from round 4 to round 11: a null vtable read as [0 + 0x1c30].
//
// isDynamic() returns true; every other body is an UNIMPLEMENTED default, present
// so the class compiles. None is claimed as reconstructed and none is gated.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxActor.h"

class NpActor : public NxActor, public NxAllocateable
	{
	public:
	NpActor();
	~NpActor();

	// The one virtual a reconstructed path calls, and it answers what the
	// harness asks: a descriptor with a body and a density is dynamic.
	virtual bool isDynamic() const;

	virtual void setGlobalPosition(const NxVec3&);
	virtual void setGlobalOrientation(const NxMat33&);
	virtual void setGlobalOrientationQuat(const NxQuat&);
	virtual NxMat34 getGlobalPoseVal() const;
	virtual NxVec3 getGlobalPositionVal() const;
	virtual NxMat33 getGlobalOrientationVal() const;
	virtual NxQuat getGlobalOrientationQuatVal() const;
	virtual const NxMat34 & getGlobalPoseReference() const;
	virtual void moveGlobalPose(const NxMat34&);
	virtual void moveGlobalPosition(const NxVec3&);
	virtual void moveGlobalOrientation(const NxMat33&);
	virtual NxShape* createShape(const NxShapeDesc&);
	virtual void releaseShape(NxShape&);
	virtual NxU32 getNbShapes() const;
	virtual NxShape** getShapes() const;
	virtual void updateMassFromShapes(NxReal density, NxReal totalMass);
	virtual void setDynamic(const NxBodyDesc&);
	virtual void setCMassOffsetLocalPose(const NxMat34&);
	virtual void setCMassOffsetLocalPosition(const NxVec3&);
	virtual void setCMassOffsetLocalOrientation(const NxMat33&);
	virtual void setCMassOffsetGlobalPose(const NxMat34&);
	virtual void setCMassOffsetGlobalPosition(const NxVec3&);
	virtual void setCMassOffsetGlobalOrientation(const NxMat33&);
	virtual void setCMassGlobalPose(const NxMat34&);
	virtual void setCMassGlobalPosition(const NxVec3&);
	virtual void setCMassGlobalOrientation(const NxMat33&);
	virtual NxMat34 getCMassLocalPoseVal() const;
	virtual NxVec3 getCMassLocalPositionVal() const;
	virtual NxMat33 getCMassLocalOrientationVal() const;
	virtual NxMat34 getCMassGlobalPoseVal() const;
	virtual NxVec3 getCMassGlobalPositionVal() const;
	virtual NxMat33 getCMassGlobalOrientationVal() const;
	virtual void setMass(NxReal);
	virtual NxReal getMass() const;
	virtual void setMassSpaceInertiaTensor(const NxVec3& m);
	virtual NxVec3 getMassSpaceInertiaTensorVal() const;
	virtual NxMat33 getGlobalInertiaTensorVal() const;
	virtual NxMat33 getGlobalInertiaTensorInverseVal() const;
	virtual void setLinearDamping(NxReal);
	virtual NxReal getLinearDamping() const;
	virtual void setAngularDamping(NxReal);
	virtual NxReal getAngularDamping() const;
	virtual void setLinearVelocity(const NxVec3&);
	virtual void setAngularVelocity(const NxVec3&);
	virtual NxVec3 getLinearVelocityVal() const;
	virtual NxVec3 getAngularVelocityVal() const;
	virtual void setMaxAngularVelocity(NxReal);
	virtual void setLinearMomentum(const NxVec3&);
	virtual void setAngularMomentum(const NxVec3&);
	virtual NxVec3 getLinearMomentumVal() const;
	virtual NxVec3 getAngularMomentumVal() const;
	virtual void addForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode );
	virtual void addForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode );
	virtual void addLocalForceAtPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode );
	virtual void addLocalForceAtLocalPos(const NxVec3& force, const NxVec3& pos, NxForceMode mode );
	virtual void addForce(const NxVec3&, NxForceMode mode );
	virtual void addLocalForce(const NxVec3&, NxForceMode mode );
	virtual void addTorque(const NxVec3&, NxForceMode mode );
	virtual void addLocalTorque(const NxVec3&, NxForceMode mode );
	virtual NxReal computeKineticEnergy() const;
	virtual NxVec3 getLocalPointVelocityVal(const NxVec3& point) const;
	virtual bool isGroupSleeping() const;
	virtual bool isSleeping() const;
	virtual NxReal getSleepLinearVelocity() const;
	virtual void setSleepLinearVelocity(NxReal threshold);
	virtual NxReal getSleepAngularVelocity() const;
	virtual void setSleepAngularVelocity(NxReal threshold);
	virtual void wakeUp(NxReal wakeCounterValue);
	virtual void putToSleep();
	virtual void raiseActorFlag(NxActorFlag);
	virtual void clearActorFlag(NxActorFlag);
	virtual bool readActorFlag(NxActorFlag) const;
	virtual void raiseBodyFlag(NxBodyFlag);
	virtual void clearBodyFlag(NxBodyFlag);
	virtual bool readBodyFlag(NxBodyFlag) const;
	virtual bool saveBodyToDesc(NxBodyDesc&);
	virtual void saveToDesc(NxActorDescBase&);
	virtual void setName(const char*);
	virtual const char* getName() const;
	virtual void setGroup(NxActorGroup);
	virtual NxActorGroup getGroup() const;
	};

// The oracle allocates 0x50 bytes for the actor object (Scene::createActor's
// literal). A class of a different size would write past its allocation, so the
// size is pinned rather than assumed -- the same guard NpScene has.
static_assert(sizeof(NpActor) == 0x50, "NpActor is 0x50 bytes in the oracle");

#endif
