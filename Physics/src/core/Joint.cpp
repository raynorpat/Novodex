/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/Joint.h"

// Scaffold only (Phase 6 Task 5). Every row below is a declared stub;
// Task 6 replaces the `write` bodies. The rows marked deferred keep their
// NX_ASSERT(0) body permanently once Task 6 records them as such -- Task 5
// does not distinguish write from defer, it only makes every assigned row
// declared and reachable from its class. Nothing constructs a Joint or
// RevoluteJoint until Task 10, so these stubs are not reachable yet.

// phys_fn_004141 (0x00099e60, 464 B)
// (unimplemented)
Joint::Joint(const NxJointDesc& desc, NxU32 typeBit)
	{
	(void)desc;
	(void)typeBit;
	NX_ASSERT(0);
	}

// phys_fn_004095 (0x00095e20, 41 B)
// (unimplemented)
Joint::~Joint()
	{
	NX_ASSERT(0);
	}

// phys_fn_004111 (0x00097fd0, 113 B)
// (unimplemented)
void Joint::row004111(NxU32 a, NxU32 b)
	{
	(void)a;
	(void)b;
	NX_ASSERT(0);
	}

// phys_fn_004087 (0x00095cc0, 87 B)
// (unimplemented)
void Joint::row004087(NxU32 a, NxU32 b, NxU32 c)
	{
	(void)a;
	(void)b;
	(void)c;
	NX_ASSERT(0);
	}

// phys_fn_004133 (0x00099ab0, 134 B)
// (unimplemented)
void Joint::row_slot6(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004135 (0x00099b40, 701 B)
// (unimplemented)
void Joint::row_slot7(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004064 (0x000957a0, 385 B)
// (unimplemented)
void Joint::row004064(NxVec3& out1, NxVec3& out2, const NxVec3& in1, const NxVec3& in2)
	{
	(void)out1;
	(void)out2;
	(void)in1;
	(void)in2;
	NX_ASSERT(0);
	}

// phys_fn_004066 (0x00095930, 266 B)
// (unimplemented)
void Joint::saveToDescBase(NxJointDesc& desc) const
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004070 (0x00095a80, 7 B)
// (unimplemented)
NxJointType Joint::getType() const
	{
	NX_ASSERT(0);
	return NX_JOINT_REVOLUTE;
	}

// phys_fn_004074 (0x00095ab0, 216 B)
// (unimplemented)
void Joint::setBreakable(NxReal maxForce, NxReal maxTorque)
	{
	(void)maxForce;
	(void)maxTorque;
	NX_ASSERT(0);
	}

// phys_fn_004076 (0x00095b90, 21 B)
// (unimplemented)
void Joint::getBreakable(NxReal& maxForce, NxReal& maxTorque) const
	{
	(void)maxForce;
	(void)maxTorque;
	NX_ASSERT(0);
	}

// phys_fn_004078 (0x00095bb0, 10 B)
// (unimplemented)
NxJointState Joint::getState() const
	{
	NX_ASSERT(0);
	return NX_JS_UNBOUND;
	}

// phys_fn_004080 (0x00095bc0, 208 B)
// (unimplemented)
bool Joint::getLimitPoint(NxVec3& worldLimitPoint) const
	{
	(void)worldLimitPoint;
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004081 (0x00095c90, 9 B)
// (unimplemented)
void Joint::resetLimitPlaneIterator()
	{
	NX_ASSERT(0);
	}

// phys_fn_004083 (0x00095ca0, 14 B)
// (unimplemented)
bool Joint::hasMoreLimitPlanes() const
	{
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004089 (0x00095d20, 58 B)
// (unimplemented)
void Joint::purgeLimitPlanes()
	{
	NX_ASSERT(0);
	}

// phys_fn_004093 (0x00095da0, 116 B)
// (unimplemented)
void Joint::row004093(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004097 (0x00095e50, 1176 B)
// (unimplemented)
void Joint::refreshBodyFrame(NxU32 bodyIndex)
	{
	(void)bodyIndex;
	NX_ASSERT(0);
	}

// phys_fn_004099 (0x000962f0, 1112 B)
// (unimplemented)
void Joint::setGlobalAnchor(const NxVec3& anchor)
	{
	(void)anchor;
	NX_ASSERT(0);
	}

// phys_fn_004101 (0x00096750, 5302 B)
// (unimplemented)
void Joint::setGlobalAxis(const NxVec3& axis)
	{
	(void)axis;
	NX_ASSERT(0);
	}

// phys_fn_004107 (0x00097d30, 297 B)
// (unimplemented)
void Joint::row004107(void* body0, void* body1, bool suppressAttach)
	{
	(void)body0;
	(void)body1;
	(void)suppressAttach;
	NX_ASSERT(0);
	}

// phys_fn_004109 (0x00097e60, 366 B)
// (unimplemented)
void Joint::setLimitPoint(const NxVec3& point, bool pointIsOnBody2)
	{
	(void)point;
	(void)pointIsOnBody2;
	NX_ASSERT(0);
	}

// phys_fn_004121 (0x000987a0, 1084 B)
// (unimplemented)
void Joint::loadFromDescBase(const NxJointDesc& desc)
	{
	(void)desc;
	NX_ASSERT(0);
	}

// phys_fn_004123 (0x00098be0, 518 B)
// (unimplemented)
void Joint::row004123(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004125 (0x00098df0, 1940 B)
// (unimplemented)
void Joint::getGlobalAnchor(NxVec3& out) const
	{
	(void)out;
	NX_ASSERT(0);
	}

// phys_fn_004127 (0x00099590, 235 B)
// (unimplemented)
void Joint::row004127(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004129 (0x00099680, 787 B)
// (unimplemented)
void Joint::getGlobalAxis(NxVec3& out) const
	{
	(void)out;
	NX_ASSERT(0);
	}

// phys_fn_004131 (0x000999a0, 260 B)
// (unimplemented)
void Joint::row004131(NxU32 arg)
	{
	(void)arg;
	NX_ASSERT(0);
	}

// phys_fn_004137 (0x00099e00, 41 B)
// (unimplemented)
NxVec3 Joint::getGlobalAnchorVal() const
	{
	NX_ASSERT(0);
	return NxVec3();
	}

// phys_fn_004139 (0x00099e30, 41 B)
// (unimplemented)
NxVec3 Joint::getGlobalAxisVal() const
	{
	NX_ASSERT(0);
	return NxVec3();
	}

// phys_fn_004143 (0x0009a0d0, 860 B)
// (unimplemented)
bool Joint::addLimitPlane(const NxVec3& normal, const NxVec3& pointInPlane)
	{
	(void)normal;
	(void)pointInPlane;
	NX_ASSERT(0);
	return false;
	}

// phys_fn_004145 (0x0009a430, 174 B)
// (unimplemented)
bool Joint::getNextLimitPlane(NxVec3& planeNormal, NxReal& planeD)
	{
	(void)planeNormal;
	(void)planeD;
	NX_ASSERT(0);
	return false;
	}
