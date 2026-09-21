#ifndef NX_PHYSICS_NPJOINT
#define NX_PHYSICS_NPJOINT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The joint object and the concrete class its vtable points at -- the same
// two-part shape as NpActor, and for the same reason: the oracle's joint has a
// vtable, and the harness calls three of its virtuals on the joint createJoint
// returns (10v).

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxJoint.h"
#include "NxJointDesc.h"

/**
The joint OBJECT: 0x17c bytes for a revolute joint, offset-addressed. The size is the
allocation literal in Scene::createJoint; the fields are written by offset because the
transcription addresses them that way.
*/
struct NpJointObject
	{
	static const NxU32 REVOLUTE_SIZE = 0x17c;

	unsigned char mBytes[REVOLUTE_SIZE];

	unsigned char* at(NxU32 byteOffset) { return mBytes + byteOffset; }

	// The vtable word at +0, which is what makes the harness's virtual calls dispatch.
	void installVtable();

	// The descriptor the joint was built from, at +4. The three virtuals the harness
	// calls read their answers from it, because the oracle's joint stores what its
	// descriptor carried and this object has nowhere else to keep it.
	static const NxU32 DESCRIPTOR_OFFSET = 4;
	const NxJointDesc* descriptor() const
		{
		return *reinterpret_cast<const NxJointDesc* const*>(mBytes + DESCRIPTOR_OFFSET);
		}
	void setDescriptor(const NxJointDesc* desc)
		{
		*reinterpret_cast<const NxJointDesc**>(mBytes + DESCRIPTOR_OFFSET) = desc;
		}
	};

/**
The concrete class the vtable points at. Only its vtable is used, so its own size does
not matter. Every body is an UNIMPLEMENTED default except the three the harness calls.
*/
class NpJointVtable : public NxJoint
	{
	public:
	NpJointVtable() {}
	~NpJointVtable() {}

	// The three a reconstructed path calls.
	virtual void getGlobalAnchor(NxVec3&) const;
	virtual void getGlobalAxis(NxVec3&) const;
	virtual NxJointState getState();
	virtual void getActors(NxActor** actor1, NxActor** actor2);
	virtual void setGlobalAnchor(const NxVec3 &);
	virtual void setGlobalAxis(const NxVec3 &);
	virtual NxVec3 getGlobalAnchorVal() const;
	virtual NxVec3 getGlobalAxisVal() const;
	virtual void setBreakable(NxReal maxForce, NxReal maxTorque);
	virtual void getBreakable(NxReal & maxForce, NxReal & maxTorque);
	virtual void setLimitPoint(const NxVec3 & point, bool pointIsOnBody2 );
	virtual bool getLimitPoint(NxVec3 & worldLimitPoint);
	virtual bool addLimitPlane(const NxVec3 & normal, const NxVec3 & pointInPlane);
	virtual void purgeLimitPlanes();
	virtual void resetLimitPlaneIterator();
	virtual bool hasMoreLimitPlanes();
	virtual bool getNextLimitPlane(NxVec3 & planeNormal, NxReal & planeD);
	virtual NxJointType getType() const;
	virtual void* is(NxJointType) const;
	virtual NxRevoluteJoint* isRevoluteJoint();
	virtual NxPointInPlaneJoint* isPointInPlaneJoint();
	virtual NxPointOnLineJoint* isPointOnLineJoint();
	virtual NxPrismaticJoint* isPrismaticJoint();
	virtual NxCylindricalJoint* isCylindricalJoint();
	virtual NxSphericalJoint* isSphericalJoint();
	virtual NxFixedJoint* isFixedJoint();
	virtual NxDistanceJoint* isDistanceJoint();
	virtual NxPulleyJoint* isPulleyJoint();
	virtual void setName(const char*);
	virtual const char* getName() const;
	};

#endif
