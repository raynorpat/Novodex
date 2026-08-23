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

/**
The box's convex-hull descriptor -- the polymorphic subobject embedded at
Shape+0xe0 (abstract wall at .rdata 0x10106a58 during construction,
final twelve-slot table at 0x10106a88). Eight vertices at +0x10, six
36-byte face records at +0x70 (which is Shape+0x150), and three static
tables in .rdata. The face records carry a corner count of 4 and pointers
into two families of static index lists (an 8-corner chained-quad family
and a second topology reaching ids 10/11); their float areas hold
dim-independent face normals. What CONSUMES the hull is still open --
the sweep dispatch of slot 7 is the suspect.

Row map: slot 0 phys_fn_000985 (dtor, untranscribed), 1 phys_fn_000953,
2 phys_fn_000955, 3 phys_fn_000961, 4 phys_fn_000963, 5 phys_fn_000965,
6 phys_fn_000967, 7 phys_fn_000969, 8 phys_fn_000971, 9 phys_fn_000957
(untranscribed), 10 phys_fn_000959 (untranscribed), 11 phys_fn_000975
(support mapping, untranscribed).
*/
struct BoxFaceRecord
	{
	NxU32				mCorners;		//!< 4, store 0x000214e3 and friends
	const NxU32*		mIndexListA;	//!< chained-quad family (.rdata 0x10106998...)
	const NxU32*		mIndexListB;	//!< second family (.rdata 0x101069f8...)
	NxU32				mFloatData[6];	//!< face normal et al, dim-independent
	};

static_assert(sizeof(BoxFaceRecord) == 0x24, "a face record is thirty-six bytes");

class BoxHullFacade
	{
	public:
	//! +0x00, vtable slot, carried opaque.
	void*				mVptrSlot;
	//! +0x04..+0x0f, unestablished.
	NxU8				mGap04[0x0c];
	//! +0x10, the eight corners; phys_fn_000973 fills them from the dims.
	NxU32				mVertices[8 * 3];
	//! +0x70, six per-face records; Shape+0x150 when embedded.
	BoxFaceRecord		mFaces[6];

	public:
	//! phys_fn_000953 (0x00020d20): mov eax,8; ret.
	static const unsigned	kVertexCount = 8;
	//! phys_fn_000961 (0x000213c0): mov eax,6; ret.
	static const unsigned	kFaceCount = 6;
	//! phys_fn_000965 (0x000213e0): xor eax,eax; ret.
	static const unsigned	kZero = 0;

	//! phys_fn_000955 (0x00020d30): lea eax,[ecx+0x10]; ret.
	const NxU32*		vertices() const;
	//! phys_fn_000963 (0x000213d0): lea eax,[eax+eax*8]; lea eax,[ecx+eax*4+0x70].
	const BoxFaceRecord*	face(unsigned index) const;
	//! phys_fn_000967 (0x000213f0): mov eax,0x10122180; ret. Edge pairs of the
	//! eight corners (0-1, 1-2, 2-3, 3-0, 7-6, ...).
	static const NxU32*	edgeTable();
	//! phys_fn_000969 (0x00021400): mov eax,0x101221e0; ret.
	static const NxU32*	faceCornerTable();
	//! phys_fn_000971 (0x00021410): mov eax,0x10122240; ret.
	static const NxU32*	adjacencyTable();
	};

static_assert(offsetof(BoxHullFacade, mVertices) == 0x10, "vertices are at +0x10");
static_assert(offsetof(BoxHullFacade, mFaces) == 0x70, "face records are at +0x70");
static_assert(sizeof(BoxHullFacade) == 0x148, "the facade spans vptr-to-last-record");

#endif
