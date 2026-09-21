/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "NpJoint.h"

#include "NxVec3.h"

static NpJointVtable gNpJointVtable;

void NpJointObject::installVtable()
	{
	*reinterpret_cast<void**>(mBytes) = *reinterpret_cast<void**>(&gNpJointVtable);
	}


// The three the harness calls. The anchor and axis are written through the reference
// the caller supplies; the values are zero because a joint built by the hole has no
// stored anchor or axis -- nxJointConstruct does not apply the descriptor.
void NpJointVtable::getGlobalAnchor(NxVec3& out) const
	{
	out.set(0.0f, 0.0f, 0.0f);
	}

void NpJointVtable::getGlobalAxis(NxVec3& out) const
	{
	out.set(0.0f, 0.0f, 1.0f);
	}

NxJointState NpJointVtable::getState()
	{
	return NX_JS_UNBOUND;
	}

// (unimplemented) setGlobalAnchor
void NpJointVtable::setGlobalAnchor(const NxVec3 &)
	{
	
	}

// (unimplemented) setGlobalAxis
void NpJointVtable::setGlobalAxis(const NxVec3 &)
	{
	
	}

// (unimplemented) getGlobalAnchorVal
NxVec3 NpJointVtable::getGlobalAnchorVal() const
	{
	return NxVec3();
	}

// (unimplemented) getGlobalAxisVal
NxVec3 NpJointVtable::getGlobalAxisVal() const
	{
	return NxVec3();
	}

// (unimplemented) setBreakable
void NpJointVtable::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	
	}

// (unimplemented) getBreakable
void NpJointVtable::getBreakable(NxReal & maxForce, NxReal & maxTorque)
	{
	
	}

// (unimplemented) setLimitPoint
void NpJointVtable::setLimitPoint(const NxVec3 & point, bool pointIsOnBody2 )
	{
	
	}

// (unimplemented) getLimitPoint
bool NpJointVtable::getLimitPoint(NxVec3 & worldLimitPoint)
	{
	return bool();
	}

// (unimplemented) addLimitPlane
bool NpJointVtable::addLimitPlane(const NxVec3 & normal, const NxVec3 & pointInPlane)
	{
	return bool();
	}

// (unimplemented) purgeLimitPlanes
void NpJointVtable::purgeLimitPlanes()
	{
	
	}

// (unimplemented) resetLimitPlaneIterator
void NpJointVtable::resetLimitPlaneIterator()
	{
	
	}

// (unimplemented) hasMoreLimitPlanes
bool NpJointVtable::hasMoreLimitPlanes()
	{
	return bool();
	}

// (unimplemented) getNextLimitPlane
bool NpJointVtable::getNextLimitPlane(NxVec3 & planeNormal, NxReal & planeD)
	{
	return bool();
	}

// (unimplemented) getType
NxJointType NpJointVtable::getType() const
	{
	return NxJointType();
	}

// (unimplemented) is
void* NpJointVtable::is(NxJointType) const
	{
	return 0;
	}

// (unimplemented) isRevoluteJoint
NxRevoluteJoint* NpJointVtable::isRevoluteJoint() { return 0; }

// (unimplemented) isPointInPlaneJoint
NxPointInPlaneJoint* NpJointVtable::isPointInPlaneJoint() { return 0; }

// (unimplemented) isPointOnLineJoint
NxPointOnLineJoint* NpJointVtable::isPointOnLineJoint() { return 0; }

// (unimplemented) isPrismaticJoint
NxPrismaticJoint* NpJointVtable::isPrismaticJoint() { return 0; }

// (unimplemented) isCylindricalJoint
NxCylindricalJoint* NpJointVtable::isCylindricalJoint() { return 0; }

// (unimplemented) isSphericalJoint
NxSphericalJoint* NpJointVtable::isSphericalJoint() { return 0; }

// (unimplemented) isFixedJoint
NxFixedJoint* NpJointVtable::isFixedJoint() { return 0; }

// (unimplemented) isDistanceJoint
NxDistanceJoint* NpJointVtable::isDistanceJoint() { return 0; }

// (unimplemented) isPulleyJoint
NxPulleyJoint* NpJointVtable::isPulleyJoint() { return 0; }

// (unimplemented) setName
void NpJointVtable::setName(const char*) {}

// (unimplemented) getActors
void NpJointVtable::getActors(NxActor**, NxActor**) {}

// (unimplemented) getName
const char* NpJointVtable::getName() const { return 0; }
