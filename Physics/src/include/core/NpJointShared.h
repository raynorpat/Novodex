#ifndef NX_PHYSICS_CORE_NPJOINTSHARED
#define NX_PHYSICS_CORE_NPJOINTSHARED
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The NxJoint-level part every public Np<Family>Joint shares. See
// docs/reconstruction/novodex-physics/units/joint-families-contract.md
// "## Shared NpJoint slots" for the slot split this class encodes.
//
// Layout (0x1c bytes, the same in all ten families): the public
// Nx<Family>Joint (vptr, userData, appData; 0xc bytes) as the primary base,
// the 12-byte hook base (EmbeddedHookBase: vptr, scene write-lock link at
// +0x10, scene read-lock link at +0x14) as the secondary base, then the
// internal <Family>Joint* at +0x18.
//
// Two kinds of NxJoint slot are shared:
//
// * The 13 bodies the oracle keeps as ONE identical-code-folded copy that
//   every family's table points at (getActors, getGlobalAnchor, ...,
//   getName). They are the virtual overrides below, defined once in
//   core/NpJointShared.cpp and explicitly instantiated there per family.
// * The NxJoint setters every family has its OWN row for (setGlobalAnchor,
//   setGlobalAxis, setBreakable, setLimitPoint, addLimitPlane,
//   purgeLimitPlanes, resetLimitPlaneIterator, setName), which differ only
//   in the __FILE__/line their write-lock report pushes. Their shared body is
//   the protected forward* helper of the same name; each family class
//   defines the virtual itself (so the row keeps its stable-ID line in the
//   family's own file) as a one-line call to the helper with its own file
//   string and line.
//
// Every family class derives from NpJointShared<Nx<Family>Joint,
// <Family>Joint> and adds only its family-specific methods. This class
// overrides NxJoint virtuals only and declares no new virtual, so MSVC keeps
// the public interface's slot order for the derived class's tables.
// `__declspec(novtable)` keeps this intermediate class from installing its
// own tables in the constructor/destructor chain: the oracle installs the
// NxJoint / Nx<Family>Joint tables and then the family's final pair, nothing
// between them.

#include "Nxp.h"
#include "PhysicsInternal.h"
#include "NxJoint.h"
#include "ObjectModel.h"
#include "NpSceneGuard.h"

#ifdef _MSC_VER
#define NX_NPJOINT_NOVTABLE	__declspec(novtable)
#else
#define NX_NPJOINT_NOVTABLE
#endif

template<class Iface, class Internal>
class NX_NPJOINT_NOVTABLE NpJointShared : public Iface, public EmbeddedHookBase
	{
	public:
	//! The compiler-generated scalar deleting destructor of every family
	//! frees through `operator delete` when its flag bit is set; the
	//! oracle's free is the Foundation allocator's (`[[0x101041bc]]` slot +0x14, the
	//! tail of each family's slot-0 row), so this routes there rather than to
	//! the global operator delete.
	static void operator delete(void* p) { nxFoundationSDKAllocator->free(p); }

	// --- The 13 folded NxJoint bodies (core/NpJointShared.cpp) ---

	virtual void getActors(NxActor** actor1, NxActor** actor2);			// slot 1
	virtual void getGlobalAnchor(NxVec3&) const;						// slot 3
	virtual void getGlobalAxis(NxVec3&) const;							// slot 5
	virtual NxVec3 getGlobalAnchorVal() const;							// slot 6
	virtual NxVec3 getGlobalAxisVal() const;							// slot 7
	virtual NxJointState getState();									// slot 8
	virtual void getBreakable(NxReal& maxForce, NxReal& maxTorque);		// slot 10
	virtual bool getLimitPoint(NxVec3& worldLimitPoint);				// slot 12
	virtual bool hasMoreLimitPlanes();									// slot 16
	virtual bool getNextLimitPlane(NxVec3& planeNormal, NxReal& planeD);	// slot 17
	virtual NxJointType getType() const;								// slot 18
	virtual void* is(NxJointType) const;								// slot 19
	virtual const char* getName() const;								// slot 30

	//! Slots 20-28 (isRevoluteJoint..isPulleyJoint): compiler-generated from
	//! the inline bodies in NxJoint.h (phys_fn_004417..004433, not Np rows);
	//! not overridden.

	//! +0x18. The internal <Family>Joint* every accessor forwards to (also
	//! duplicated into NxJoint::appData at +0x08 by the constructor).
	Internal*			mInternal;

	protected:
	//! The shared half of each family's constructor (phys_fn_004725 for
	//! revolute): NxJoint()'s inline constructor has already zeroed
	//! userData/appData and the base tables are installed by ordinary C++
	//! construction; this zeroes the hook base's two words (phys_fn_002404's
	//! zeroing -- EmbeddedHookBase has no constructor of its own), then
	//! stores `internal` at +0x18 and again at +0x08.
	explicit NpJointShared(Internal* internal)
		{
		mWord04 = 0;
		mWord08 = 0;
		mInternal = internal;
		this->appData = internal;
		}

	//! The write-lock link value (the VALUE stored at +0x10/mWord04, not its
	//! address) -- what every setter passes to nxNpSceneGuardWriteTry/Leave.
	void*			writeLink() const { return reinterpret_cast<void*>(mWord04); }
	//! The read-lock link value (the VALUE stored at +0x14/mWord08) -- what
	//! every getter passes to nxNpSceneGuardEnter/Leave.
	void*			readLink() const { return reinterpret_cast<void*>(mWord08); }

	//! The failed-tryLock report every write-locked Np row makes: error code
	//! 2 (NXE_INVALID_OPERATION), the row's own __FILE__ and line, the
	//! shared message.
	static void		reportWriteLocked(const char* file, int line)
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION, file, line, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		}

	// --- Shared bodies of the per-family NxJoint setter rows. Each family's
	//     row is its own virtual override calling one of these with its own
	//     file string and line (0xe for revolute). ---

	//! Slot 2: write lock, Joint::setGlobalAnchor (phys_fn_004099).
	void forwardSetGlobalAnchor(const char* file, int line, const NxVec3& anchor)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		mInternal->setGlobalAnchor(anchor);
		nxNpSceneGuardLeave(link);
		}

	//! Slot 4: write lock, Joint::setGlobalAxis (phys_fn_004101).
	void forwardSetGlobalAxis(const char* file, int line, const NxVec3& axis)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		mInternal->setGlobalAxis(axis);
		nxNpSceneGuardLeave(link);
		}

	//! Slot 9: write lock, Joint::setBreakable (phys_fn_004074).
	void forwardSetBreakable(const char* file, int line, NxReal maxForce, NxReal maxTorque)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		mInternal->setBreakable(maxForce, maxTorque);
		nxNpSceneGuardLeave(link);
		}

	//! Slot 11: write lock, Joint::setLimitPoint (phys_fn_004109).
	void forwardSetLimitPoint(const char* file, int line, const NxVec3& point, bool pointIsOnBody2)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		mInternal->setLimitPoint(point, pointIsOnBody2);
		nxNpSceneGuardLeave(link);
		}

	//! Slot 13: write lock, Joint::addLimitPlane (phys_fn_004143).
	bool forwardAddLimitPlane(const char* file, int line, const NxVec3& normal, const NxVec3& pointInPlane)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return false;
			}
		bool result = mInternal->addLimitPlane(normal, pointInPlane);
		nxNpSceneGuardLeave(link);
		return result;
		}

	//! Slot 14: write lock, Joint::purgeLimitPlanes (phys_fn_004089),
	//! tail-jumps to unlock.
	void forwardPurgeLimitPlanes(const char* file, int line)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		mInternal->purgeLimitPlanes();
		nxNpSceneGuardLeave(link);
		}

	//! Slot 15: write lock, Joint::resetLimitPlaneIterator
	//! (phys_fn_004081), tail-jumps to unlock.
	void forwardResetLimitPlaneIterator(const char* file, int line)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		mInternal->resetLimitPlaneIterator();
		nxNpSceneGuardLeave(link);
		}

	//! Slot 29: write lock, nxSetSdkPointerBinding([np+0x18], name).
	void forwardSetName(const char* file, int line, const char* name)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		nxSetSdkPointerBinding(mInternal, const_cast<char*>(name));
		nxNpSceneGuardLeave(link);
		}

	//! Slots 31/32 of every family (loadFromDesc/saveToDesc): write lock,
	//! the internal object's own virtual (internal slot 9/10), unlock. The
	//! descriptor type is the family's.
	template<class Desc>
	void forwardLoadFromDesc(const char* file, int line, const Desc& desc)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		mInternal->loadFromDesc(desc);
		nxNpSceneGuardLeave(link);
		}

	template<class Desc>
	void forwardSaveToDesc(const char* file, int line, Desc& desc)
		{
		void* link = writeLink();
		if(!nxNpSceneGuardWriteTry(link))
			{
			reportWriteLocked(file, line);
			return;
			}
		mInternal->saveToDesc(desc);
		nxNpSceneGuardLeave(link);
		}
	};

#endif
