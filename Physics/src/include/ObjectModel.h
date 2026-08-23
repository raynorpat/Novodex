#ifndef NX_PHYSICS_OBJECTMODEL
#define NX_PHYSICS_OBJECTMODEL
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"

/**
The shared object-model rows Phase 5 Task 1 locked: the 0x1c-byte collision
object that Shape+0x9c points at, and the shape's owner accessor. Both are
transcribed here ahead of the classes that own them because the Phase 5
layout gate drives them directly; when Tasks 2 and 3 materialise the owning
classes these rows may migrate, and the gate will follow.

Every offset carries the address that establishes it. See
docs/reconstruction/novodex-physics/evidence/phase5-object-model.md.
*/

/**
The embedded member three containers share. Its base constructor
phys_fn_002404 (0x0005ba70) installs vptr 0x101088b8 and zeroes two words;
every container overwrites the vptr with its own one-slot table -- the actor
at +8 uses 0x1010468c, the collision object at +0xc uses 0x101072a0 -- so the
member's dynamic type differs per container while its shape stays twelve
bytes: one virtual slot plus two words. What the single virtual MEANS is not
established; the class has no recovered name.
*/
struct EmbeddedHookBase
	{
	virtual ~EmbeddedHookBase() {}
	NxU32				mWord04;
	NxU32				mWord08;
	};

/**
The 0x1c-byte collision object. Constructor phys_fn_001193 (0x000247c0,
57 bytes):

	+0x00	vptr			final table 0x10107218, store 0x000247ed
	+0x04	zeroed			store 0x000247cb
	+0x08	first argument	store 0x000247e9 -- ALSO written to +0x18
	+0x0c	embedded member	final vptr 0x101072a0 at 0x000247e0, member ctor
	                        phys_fn_002404 called at 0x000247d7
	+0x18	same argument	store 0x000247e6

The listing shows MSVC's chained-construction dance (an intermediate base
vptr 0x10107158 at 0x000247d1, replaced before the constructor returns).
Those intermediates are never observable after construction returns, so the
transcription writes the final state once rather than replaying them; the
layout gate compares post-construction bytes.
*/
class CollisionObject
	{
	public:
	//! phys_fn_001193 (0x000247c0). The argument is stored twice (+8/+0x18)
	//! and handed nowhere else; what it means is unestablished.
	explicit			CollisionObject(void* argument);

	//! +0x00, the vtable slot, carried opaque like TriangleMesh's.
	void*				mVptrSlot;
	//! +0x04, zeroed by the constructor.
	NxU32				mWord04;
	//! +0x08, first copy of the constructor argument.
	void*				mArgument08;
	//! +0x0c, the embedded member.
	EmbeddedHookBase	mMember;

	private:
	//! +0x18, second copy of the constructor argument.
	void*				mArgument18;
	};

static_assert(sizeof(CollisionObject) == 0x1c, "the collision object is twenty-eight bytes");
static_assert(offsetof(CollisionObject, mWord04) == 0x04, "+0x04 is zeroed");
static_assert(offsetof(CollisionObject, mArgument08) == 0x08, "the argument lands at +0x08");
static_assert(offsetof(CollisionObject, mMember) == 0x0c, "the member is at +0x0c");

/**
phys_fn_001281 (0x000257a0): mov eax,[ecx+4]; ret. Four bytes. Phase 3
borrowed this field as the shape's owner and closed this row as its
accessor; Phase 5 owns the row.
*/
const void* nxShapeOwner(const void* shape);

#endif
