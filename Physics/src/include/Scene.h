#ifndef NX_PHYSICS_SCENE
#define NX_PHYSICS_SCENE
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The internal Scene. Phase 3/7 owns it; it is reconstructed here because every
// actor-dependent row in the programme is blocked on it.
//
// The layout below is MEASURED from the shipped Win32 Release NxPhysics.dll, from
// the two rows that build the object:
//
//   phys_fn_000476 (0x0000ea80, PhysicsSDK::createScene)
//       allocates 0x710 bytes through the SDK allocator, then constructs and
//       initialises it; that 0x710 is where the size comes from.
//   phys_fn_000647 (0x00012c10)  the constructor -- a straight-line initialiser
//       that writes every field in increasing offset order.
//   phys_fn_000651 (0x00013070)  the descriptor-driven initialiser.
//
// The offsets are the ones those rows actually write. Fields that appear in one
// and not the other are marked with the row that writes them. See
// evidence/phase6-joints.md 8t onward for the write-by-write derivation.
//
// This is deliberately an offset-addressed object rather than a class with named
// members: the oracle reads and writes it through raw pointer arithmetic, and a
// named-member layout would silently reorder fields and change the behaviour under
// test. The accessors below exist so the code reads, without pretending the field
// names are recovered -- only their offsets are.

#include "Nxp.h"
#include "PhysicsInternal.h"

class Scene;
class NxSceneDesc;
class NxActor;
class NxActorDescBase;
class NxJointDesc;
class NxJoint;
class Joint;
class JointBreakEvent;
class Effector;
class SpringAndDamperEffector;
class NxSpringAndDamperEffectorDesc;

/**
The 0x710-byte scene object.

Only the members the reconstruction needs to reach are named; everything else is
reached through `at<T>(offset)` so that no offset is lost behind a guess. The
fields that are known by more than their offset are documented individually.
*/
class NxSceneInternal
	{
	public:
	// phys_fn_000647 (0x00012c10). Constructs in place. The oracle's prototype
	// takes the block and returns it, because the caller allocates.
	NxSceneInternal();

	// phys_fn_000651 (0x00013070). Applies a descriptor. Returns true on success;
	// on false the caller destroys the object it just built.
	bool initialise(const NxSceneDesc& desc);

	// phys_fn_000626 (0x00011730). The actor factory. createScene reaches it too,
	// because the ground plane is made by calling this.
	NxActor* createActor(const NxActorDescBase& desc);
	void releaseActor(void* body);

	// phys_fn_000665 (0x000142c0). The joint factory. Needs at least one of the
	// two actors dynamic, read through each actor's +0x14 body.
	NxJoint* createJoint(const NxJointDesc& desc);

	// The joint rows (units/joint-open-items-contract.md "## Scene joint rows").
	// The Scene keeps its joints three ways: a list through Joint +0x10 headed
	// at +0x59c (joints with +0x2c bit 0 set; a second list at +0x5a0 holds
	// joints without it), a {begin, end, capacity} pointer array at
	// +0x58c/+0x590/+0x594, and the count at +0x6c8 that getNbJoints reads;
	// +0x6bc is the enumeration cursor.
	// phys_fn_000661 (0x00013e00). Scene::addJoint.
	void addJoint(Joint* joint);
	// phys_fn_000633 (0x00012660). Scene::removeJoint.
	void removeJoint(Joint* joint);
	// phys_fn_000653 (0x00013760). Scene::releaseJoint.
	void releaseJoint(Joint* joint);
	// phys_fn_000598 (0x00010f50). Grows the 0x50-byte record array at +0x5b8.
	void growJointRecords();
	// phys_fn_000571 (0x000108e0). Links a break event into the list at +0x620.
	void addJointBreakEvent(JointBreakEvent* event);
	// phys_fn_000559 (0x00010860), phys_fn_000563 (0x00010880) and
	// phys_fn_000567 (0x000108a0).
	NxU32 getNbJoints() const;
	void resetJointIterator();
	Joint* getNextJoint();

	// The effector rows (units/effector-coredump-contract.md "### Scene and
	// NpScene rows"). The Scene keeps its effectors in a list through
	// Effector +0x18 headed at +0x5a4, with the count at +0x6c4 and the
	// enumeration cursor at +0x6c0.
	// phys_fn_000587 (0x00010c90). Scene::createSpringAndDamperEffector.
	SpringAndDamperEffector* createSpringAndDamperEffector(const NxSpringAndDamperEffectorDesc& desc);
	// phys_fn_000594 (0x00010e80). Scene::releaseEffector.
	void releaseEffector(Effector* effector);
	// phys_fn_000573 (0x00010900). Scene::removeEffector.
	void removeEffector(Effector* effector);
	// phys_fn_000575 (0x00010970). Releases every effector (Scene teardown).
	void releaseEffectors();
	// phys_fn_000561 (0x00010870), phys_fn_000565 (0x00010890) and
	// phys_fn_000569 (0x000108c0).
	NxU32 getNbEffectors() const;
	void resetEffectorIterator();
	Effector* getNextEffector();

	// The raw object. `at` is the only sanctioned way to reach a field whose name
	// is not recovered.
	unsigned char* bytes() { return mBytes; }
	const unsigned char* bytes() const { return mBytes; }

	template <class T> T& at(NxU32 offset)
		{ return *reinterpret_cast<T*>(mBytes + offset); }
	template <class T> const T& at(NxU32 offset) const
		{ return *reinterpret_cast<const T*>(mBytes + offset); }

	// The measured fields, by the constant the oracle writes them with.
	static const NxU32 SIZE = 0x710;

	// Four fields the constructor writes last, from the register that holds the
	// address of the SdkContainer at +0x50 (0x00012c47 `lea ebx, [esi + 0x50]`,
	// stored at 0x00012f80..0x00012f92): dwords 0x2a, 0x3d, 0x91 and 0xc1.
	static const NxU32 SELF_0 = 0x2a * 4;
	static const NxU32 SELF_1 = 0x3d * 4;
	static const NxU32 SELF_2 = 0x91 * 4;
	static const NxU32 SELF_3 = 0xc1 * 4;

	// The vtable the constructor installs, at .rdata 0x001066f4. The oracle's
	// phys_fn_000647 writes it as its first action (`*param_1 = &PTR_FUN_101066f4`).
	// Only the first slot is modelled -- the scalar deleting destructor reached
	// by failed creation and by public SDK scene release.
	// The public NxScene wrapper, kept at +0x6cc. getScene reads it back and
	// createActor copies it into each actor. Measured in createScene's decompilation.
	void setPublicScene(void* wrapper) { at<void*>(0x6cc) = wrapper; }
	void* publicScene() const { return at<void*>(0x6cc); }

	static void* vtable();
	// The scalar deleting destructor at the vtable's slot 0.
	void scalarDeletingDestructor(int flags);

	private:
	unsigned char mBytes[SIZE];
	};

// The oracle allocates 0x710 bytes for this object (phys_fn_000476's literal) and
// the highest field the reconstruction writes is +0x70c. A different size means the
// allocation and the object disagree, and a write past the end lands in whatever the
// allocator put next -- which is what the round-4 crash in an unrelated CRT helper
// looks like from a distance.
static_assert(sizeof(NxSceneInternal) == NxSceneInternal::SIZE,
              "the Scene object is 0x710 bytes in the oracle");

#endif
