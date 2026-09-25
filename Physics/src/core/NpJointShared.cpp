/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/NpJointShared.h"
#include "core/Joint.h"
#include "core/RevoluteJoint.h"
#include "NxRevoluteJoint.h"
#include "core/PrismaticJoint.h"
#include "NxPrismaticJoint.h"
#include "core/CylindricalJoint.h"
#include "NxCylindricalJoint.h"
#include "core/SphericalJoint.h"
#include "NxSphericalJoint.h"
#include "core/PointOnLineJoint.h"
#include "NxPointOnLineJoint.h"
#include "core/PointInPlaneJoint.h"
#include "NxPointInPlaneJoint.h"
#include "core/DistanceJoint.h"
#include "NxDistanceJoint.h"
#include "core/PulleyJoint.h"
#include "NxPulleyJoint.h"
#include "PhysicsInternal.h"
#include "NpSceneGuard.h"

// The 13 NxJoint bodies the oracle keeps as one identical-code-folded copy
// that all ten Np<Family>Joint tables point at (joint-families Task 1; the
// slot split is in units/joint-families-contract.md "## Shared NpJoint
// slots"). Each body reads only the shared 0x1c layout: the read-lock link
// value at +0x14, the internal Joint* at +0x18. Every row here belongs to an
// Np<Family>Joint.cpp unit other than revolute (the owning unit is named on
// each body and in the contract) and is claimed by this file.
//
// The template is instantiated at the bottom of this file, once per family
// that has been reconstructed.

// phys_fn_004539 (0x000b1600, 98 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b1600
// (phys_fn_004539). Owning unit core\NpDistanceJoint.cpp. Slot 1: under the
// read lock, each body's owner (`*[[j+8]+0x19c]`, `*[[j+0xc]+0x19c]`) or 0.
template<class Iface, class Internal>
void NpJointShared<Iface, Internal>::getActors(NxActor** actor1, NxActor** actor2)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	JointBodyRecord* body0 = static_cast<JointBodyRecord*>(mInternal->mBody[0]);
	*actor1 = body0 ? *reinterpret_cast<NxActor**>(body0->mOwner) : 0;
	JointBodyRecord* body1 = static_cast<JointBodyRecord*>(mInternal->mBody[1]);
	if(body1)
		{
		*actor2 = *reinterpret_cast<NxActor**>(body1->mOwner);
		nxNpSceneGuardLeave(link);
		return;
		}
	*actor2 = 0;
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004437 (0x000b0670, 39 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0670
// (phys_fn_004437). Owning unit core\NpD6Joint.cpp. Slot 3: read lock,
// Joint::getGlobalAnchor (phys_fn_004125).
template<class Iface, class Internal>
void NpJointShared<Iface, Internal>::getGlobalAnchor(NxVec3& out) const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	mInternal->getGlobalAnchor(out);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004441 (0x000b0700, 39 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0700
// (phys_fn_004441). Owning unit core\NpD6Joint.cpp. Slot 5: read lock,
// Joint::getGlobalAxis (phys_fn_004129).
template<class Iface, class Internal>
void NpJointShared<Iface, Internal>::getGlobalAxis(NxVec3& out) const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	mInternal->getGlobalAxis(out);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004497 (0x000b0ff0, 43 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0ff0
// (phys_fn_004497). Owning unit core\NpPulleyJoint.cpp. Slot 6: read lock,
// Joint::getGlobalAnchorVal (phys_fn_004137).
template<class Iface, class Internal>
NxVec3 NpJointShared<Iface, Internal>::getGlobalAnchorVal() const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxVec3 out = mInternal->getGlobalAnchorVal();
	nxNpSceneGuardLeave(link);
	return out;
	}

// phys_fn_004499 (0x000b1020, 43 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b1020
// (phys_fn_004499). Owning unit core\NpPulleyJoint.cpp. Slot 7: read lock,
// Joint::getGlobalAxisVal (phys_fn_004139).
template<class Iface, class Internal>
NxVec3 NpJointShared<Iface, Internal>::getGlobalAxisVal() const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxVec3 out = mInternal->getGlobalAxisVal();
	nxNpSceneGuardLeave(link);
	return out;
	}

// phys_fn_004483 (0x000b0dc0, 36 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0dc0
// (phys_fn_004483). Owning unit core\NpPulleyJoint.cpp. Slot 8 (on the joint
// transcript path): read lock, Joint::getState (phys_fn_004078).
template<class Iface, class Internal>
NxJointState NpJointShared<Iface, Internal>::getState()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxJointState state = mInternal->getState();
	nxNpSceneGuardLeave(link);
	return state;
	}

// phys_fn_004573 (0x000b1bd0, 44 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b1bd0
// (phys_fn_004573). Owning unit core\NpPointInPlaneJoint.cpp. Slot 10: read
// lock, Joint::getBreakable (phys_fn_004076).
template<class Iface, class Internal>
void NpJointShared<Iface, Internal>::getBreakable(NxReal& maxForce, NxReal& maxTorque)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	mInternal->getBreakable(maxForce, maxTorque);
	nxNpSceneGuardLeave(link);
	}

// phys_fn_004577 (0x000b1c60, 45 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b1c60
// (phys_fn_004577). Owning unit core\NpPointInPlaneJoint.cpp. Slot 12: read
// lock, Joint::getLimitPoint (phys_fn_004080), result kept in bl across the
// unlock.
template<class Iface, class Internal>
bool NpJointShared<Iface, Internal>::getLimitPoint(NxVec3& worldLimitPoint)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->getLimitPoint(worldLimitPoint);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_004491 (0x000b0f10, 38 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0f10
// (phys_fn_004491). Owning unit core\NpPulleyJoint.cpp. Slot 16: read lock,
// Joint::hasMoreLimitPlanes (phys_fn_004083).
template<class Iface, class Internal>
bool NpJointShared<Iface, Internal>::hasMoreLimitPlanes()
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->hasMoreLimitPlanes();
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_004635 (0x000b25d0, 50 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b25d0
// (phys_fn_004635). Owning unit core\NpSphericalJoint.cpp. Slot 17: read
// lock, Joint::getNextLimitPlane (phys_fn_004145).
template<class Iface, class Internal>
bool NpJointShared<Iface, Internal>::getNextLimitPlane(NxVec3& planeNormal, NxReal& planeD)
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	bool result = mInternal->getNextLimitPlane(planeNormal, planeD);
	nxNpSceneGuardLeave(link);
	return result;
	}

// phys_fn_004443 (0x000b0730, 36 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0730
// (phys_fn_004443). Owning unit core\NpD6Joint.cpp. Slot 18: read lock,
// Joint::getType (phys_fn_004070).
template<class Iface, class Internal>
NxJointType NpJointShared<Iface, Internal>::getType() const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxJointType type = mInternal->getType();
	nxNpSceneGuardLeave(link);
	return type;
	}

// phys_fn_004479 (0x000b0d20, 50 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0d20
// (phys_fn_004479). Owning unit core\NpPulleyJoint.cpp. Slot 19:
// `this & ((arg != getType()) - 1)` (0xb0d3a-0xb0d42): returns `this` when
// the argument equals Joint::getType (phys_fn_004070), else 0 -- written
// here as the equivalent conditional.
template<class Iface, class Internal>
void* NpJointShared<Iface, Internal>::is(NxJointType type) const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	NxJointType actual = mInternal->getType();
	nxNpSceneGuardLeave(link);
	return actual == type ? const_cast<NpJointShared*>(this) : 0;
	}

// phys_fn_004743 (0x000b3670, 40 B)
// Shared NpJoint body; the oracle keeps one folded copy at 0x000b3670
// (phys_fn_004743). Owning unit core\NpPrismaticJoint.cpp. Slot 30: read
// lock, nxGetSdkPointerBinding([np+0x18]) (phys_fn_000454's cdecl lookup,
// 0xb3682).
template<class Iface, class Internal>
const char* NpJointShared<Iface, Internal>::getName() const
	{
	void* link = readLink();
	nxNpSceneGuardEnter(link);
	const char* name = static_cast<const char*>(nxGetSdkPointerBinding(mInternal));
	nxNpSceneGuardLeave(link);
	return name;
	}

// One explicit instantiation per reconstructed family. A family task adds
// its line (and its internal header above).
template class NpJointShared<NxRevoluteJoint, RevoluteJoint>;
template class NpJointShared<NxPrismaticJoint, PrismaticJoint>;
template class NpJointShared<NxCylindricalJoint, CylindricalJoint>;
template class NpJointShared<NxSphericalJoint, SphericalJoint>;
template class NpJointShared<NxPointOnLineJoint, PointOnLineJoint>;
template class NpJointShared<NxPointInPlaneJoint, PointInPlaneJoint>;
template class NpJointShared<NxDistanceJoint, DistanceJoint>;
template class NpJointShared<NxPulleyJoint, PulleyJoint>;
