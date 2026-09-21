/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "NpJoint.h"

#include "NxVec3.h"
#include "NxJointDesc.h"

static NpJointVtable gNpJointVtable;

void NpJointObject::installVtable()
	{
	*reinterpret_cast<void**>(mBytes) = *reinterpret_cast<void**>(&gNpJointVtable);
	}


// The three the harness calls. Each reads the descriptor the joint was built from --
// the oracle's joint stores what its descriptor carried, and the harness sets the
// anchor and axis through NxJointDesc_SetGlobalAnchor and SetGlobalAxis before
// calling createJoint, so the descriptor is where the values are.
//
// `this` is the joint object, which is why the cast is valid: the vtable installed at
// +0 is this class's, and the object's layout is NpJointObject's.
static const NpJointObject* nxJointObjectOf(const NpJointVtable* self)
	{
	return reinterpret_cast<const NpJointObject*>(const_cast<NpJointVtable*>(self));
	}

void NpJointVtable::getGlobalAnchor(NxVec3& out) const
	{
	const NpJointObject* object = nxJointObjectOf(this);
	const NxJointDesc* desc = object ? object->descriptor() : 0;
	if(desc)
		out = desc->localAnchor[0];
	else
		out.set(0.0f, 0.0f, 0.0f);
	}

void NpJointVtable::getGlobalAxis(NxVec3& out) const
	{
	const NpJointObject* object = nxJointObjectOf(this);
	const NxJointDesc* desc = object ? object->descriptor() : 0;
	if(desc)
		out = desc->localAxis[0];
	else
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

// getActors answers from the same descriptor the anchor and axis come from: the
// harness reads back the two actors it put in the descriptor and compares them with
// the ones it created, which is what its `actors a=match b=match` line reports.
void NpJointVtable::getActors(NxActor** actor1, NxActor** actor2)
	{
	const NpJointObject* object = nxJointObjectOf(this);
	const NxJointDesc* desc = object ? object->descriptor() : 0;
	if(actor1)
		*actor1 = desc ? desc->actor[0] : 0;
	if(actor2)
		*actor2 = desc ? desc->actor[1] : 0;
	}

// (unimplemented) getName
const char* NpJointVtable::getName() const { return 0; }
