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
// makes a virtual call through it (isDynamic, slot 19), which is the fault that
// ran from round 4 to round 11: a null vtable read as [0 + 0x1c30].
//
// isDynamic() and the position/orientation getters read the body graph. Most other
// virtuals still have UNIMPLEMENTED defaults so the class compiles.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxActor.h"

/**
The public actor wrapper: 0x18 bytes, offset-addressed.

The page-guarded public-DLL probe measured a 0x18-byte allocation for each
NxActor. Its +0x10 points to a shape-link allocation and +0x14 points to a
separate 0x50-byte pose/body. The latter contains the matrix at +0x20 and
translation at +0x44. NxActor declares no data members, so this wrapper is
written by offsets rather than guessed as member names.
*/
struct NpActorObject
	{
	static const NxU32 SIZE = 0x18;

	unsigned char mBytes[SIZE];

	// The offset is a BYTE offset and the pointer is char*, which is the rule the
	// nxAt helper in Scene.cpp makes explicit.
	unsigned char* at(NxU32 byteOffset) { return mBytes + byteOffset; }
	const unsigned char* at(NxU32 byteOffset) const { return mBytes + byteOffset; }

	// The vtable word, at +0. Installing it is what makes the oracle's virtual calls
	// dispatch instead of reading [0 + slot], which was the fault from round 4 to 11.
	void installVtable();
	// The actor-body transcription initializes the wrapper's adjacent words after
	// allocation; reinstall the constructor's secondary-base vptr afterward.
	void installSecondaryVtable();
	};

static_assert(sizeof(NpActorObject) == NpActorObject::SIZE,
              "the actor wrapper is 0x18 bytes in the oracle");

/**
The concrete class the vtable points at. It is deliberately NOT the object: its own
size is irrelevant, because only its vtable is used. Most virtuals still have
UNIMPLEMENTED defaults; isDynamic and the position/orientation getters run through
the actual DLL by PhysicsActorLifecycleTests.
*/
class NpActorVtable : public NxActor
	{
	public:
	NpActorVtable() {}
	~NpActorVtable() {}

	// The one virtual a reconstructed path calls. NxJointDesc::isValid() asks it;
	// phys_fn_000110 reads the body marker through actor+0x14, body+0x08.
	virtual bool isDynamic() const;
	// NxActor declares setGlobalPose twice. A name-based duplicate filter dropped
	// both, which is why this is written out rather than generated.
	virtual void setGlobalPose(const NxMat34&);
	virtual void setGlobalPose(const NxVec3&, const NxMat33&);


	// Declared in NxActor with an inline sibling that confused the generator;
	// listed here so the class is concrete.
	virtual NxVec3 getPointVelocityVal(const NxVec3& point) const;
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

#endif
