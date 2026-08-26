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
#include "IcePrunable.h"

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
	//! +0x04..+0x0f, the three dimensions. Named by the box constructor
	//! phys_fn_000977, which stores 1.0f into each word (0x00021926..36)
	//! after ShapeBase leaves them poisoned; consumers read them as floats.
	float				mDims04[3];
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

	//! phys_fn_000975 (0x000217c0). Projects all eight vertices through
	//! `pose` (a column-major 3x4: rotation words then translation at word
	//! 12..14) and folds them with `direction`, keeping the min into
	//! `outMin` and the max into `outMax`. Six stack arguments in the row;
	//! the first and last are read by nothing.
	void				supportBounds(const float* direction, float* outMin,
							float* outMax, const float* pose) const;

	//! phys_fn_000985 (0x00021a10), slot 0. A once-guarded lazy init: zeroes
	//! a twelve-byte .data global (.data 0x10123c64), runs an initializer
	//! array whose entries are bare `ret` stubs (.rdata 0x10103010), and
	//! returns the global's address -- which therefore stays all-zero for
	//! the life of the process. Ignores `this`.
	static const void*	sharedHook();
	};

static_assert(offsetof(BoxHullFacade, mVertices) == 0x10, "vertices are at +0x10");
static_assert(offsetof(BoxHullFacade, mFaces) == 0x70, "face records are at +0x70");
static_assert(sizeof(BoxHullFacade) == 0x148, "the facade spans vptr-to-last-record");

/**
One column-major 3x4 pose as the base constructor initialises it: nine
rotation words at +0x00..+0x20 and the translation at +0x24..+0x2c. The
constructor stores the identity diagonal (1.0f at words 0/4/8 -- the
`mov ebx,0x3f800000` at 0x00025535 feeding stores like 0x00025555) and zeroes
everything else, three times over.
*/
struct ShapePose
	{
	NxU32				mRotation[9];	//!< +0x00 relative; diag words carry 1.0f bits
	float				mTranslation[3];//!< +0x24 relative; zeroed
	};

static_assert(sizeof(ShapePose) == 0x30, "a pose is twelve words");

/**
The base-shape class its six finals embed first. Constructor phys_fn_001273
(0x00025530, 424 bytes, __thiscall, `ret 8`: two stack arguments):

	+0x000	vptr			final BASE table .rdata 0x10107494, store 0x0002553d
	+0x004	first argument	the owner nxShapeOwner reads back out of +0x04,
	                        store 0x00025543; when non-null, 0x0002561f..26
	                        reaches [owner+4], loads word +0x48 of it and calls
	                        phys_fn_002423 (0x0005c390) with the shape pushed --
	                        owner registration (driven with Task 4's actors)
	+0x008	zeroed			store 0x00025549
	+0x00c	pose one		identity, stores 0x00025555..0x000255cd
	+0x03c	pose two		identity, same instruction run
	+0x06c	pose three		identity, same instruction run -- the third pose the
	                        phase 3 escalation asked about exists and starts here
	+0x09c	zeroed			store 0x000255d3
	+0x0a0	zeroed			store 0x000255d9; a later helper (0x00025760) reads it
	                        as a pointer and null-tests it
	+0x0a4	Prunable		ctor phys_fn_004874 called at 0x000255df; immediately
	                        afterwards the shape stores ITSELF into the prunable's
	                        mOwner (0x00025649) -- the shape is the owner its
	                        prunable reports
	+0x0d0	0x7fffffff		store 0x000255ed
	+0x0d4	second argument	store 0x000255f7: [esp+0x14] after three pushes is the
	                        entry [esp+8] slot (`ret 8` fixes the count at two)
	+0x0d8	word			zeroed halfwords, stores 0x000255fd / 0x00025604
	+0x0dc	6				halfword store 0x0002560b
	+0x0de	8				halfword store 0x00025614

The image writes the three identity poses TWICE (0x00025555..cd and again
0x0002564f..cd, the second pass after the hook stores); the transcription
writes them once because both passes store identical bytes. The ctor also
installs the three Prunable owner adapters into `.data` (0x10128470/74/78 --
stores 0x0002562b..3f), which Phase 4 had to leave null because their only
writer sat outside that task's population; this constructor is that writer.

Derived classes continue at +0xe0: the box embeds its BoxHullFacade there.
*/
class ShapeBase
	{
	public:
	//! phys_fn_001273 (0x00025530). The first argument is the owner; the
	//! second is stored raw at +0xd4 and named by nothing yet.
					ShapeBase(void* owner, unsigned argument);

	// The BASE vtable's stub rows (.rdata 0x10107494), transcribed as members
	// ahead of the class going polymorphic. Each carries its slot number.

	//! Slot 4, phys_fn_001249 (0x00024f70): `xor al,al; ret 0xc` -- two stack
	//! arguments, always false. Consumer unestablished.
	bool				nxBaseSlot4(void* argument1, void* argument2);
	//! Slot 5, phys_fn_004812 (0x000b4070): `xor eax,eax; ret 0x14` -- four
	//! stack arguments, always null.
	void*				nxBaseSlot5(void* argument1, void* argument2,
							void* argument3, void* argument4);
	//! Slot 7, phys_fn_001035 (0x00022dd0): `xor al,al; ret 8` -- one stack
	//! argument, always false. This is the continuous-collision sweep entry
	//! Phase 3 left unresolved: the base shape cannot sweep, so the row is a
	//! stub here while every final overrides it.
	bool				nxBaseSlot7(void* argument1);
	//! Slot 2, phys_fn_001277 (0x000256f0): save-to-descriptor. Copies the
	//! third pose (12 words) to record+8, the +0xde halfword zero-extended to
	//! record+0x38, the +0xd8/+0xda halfwords to record+0x3c/0x3e, and
	//! [collision-object+4] to record+0x40; returns true. Reads [colobj+4]
	//! UNCONDITIONALLY, exactly like the image -- a shape whose allocation
	//! failed faults here identically.
	bool				nxBaseSaveState(void* record);

	//! BASE-level helper phys_fn_001329 (0x00026d90): apply a GROUP.
	//! Validates against 0x20 through the error stream (Task 2 owns the
	//! reporter), stores the group at +0xd8, then marks dirty-flag 0x04 via
	//! 0x26c90 -- a null-owner no-op on a detached shape. Lives on ShapeBase
	//! because every family's apply/load rows reach it.
	void				nxApplyGroup(unsigned short group);

	//! BASE-table slot 6, phys_fn_001315 (0x000266a0): owner update.
	//! For a detached shape (owner == null) this is a proven no-op that
	//! returns immediately through the early exit at 0x00026abb.
	void				nxApplyOwnerUpdate(unsigned flags);

	//! The base dtor's owner arms (0x26bd0 body, 0x00026be1..0x00026c35):
	//! scene dirty flag |= 2, then removers #1/#2/#3 against [scene+0x48],
	//! [scene]+0x5d4 and [scene]+0x6e4 respectively. No-op when detached.
	void				nxBaseDtorOwnerArms(void);

	//! BASE-level registry helper phys_fn_000480 (0x000edc0): associate or
	//! dissociate a shape with a debug name through the global list at
	//! .data 0x10123c0c. Called from apply-desc (slot 1) with the
	//! descriptor's name field.
	bool				nxShapeNameRegistry(void* shape, void* name);

	//! BASE-table slot 1, phys_fn_001347 (0x00027740): apply-from-descriptor.
	//! Copies the twelve-word pose from record+8 into shape+0x6c, mirrors
	//! halfwords record+0x38 -> +0xde, record+0x3e -> +0xda, writes
	//! record+0x40 into [colobj+4] when present, applies the group through
	//! nxApplyGroup, then registers/dissociates the shape against
	//! record+0x44 (the debug name) through the registry helper -- a no-op
	//! when the name is NULL and the list is empty.
	bool				nxApplyDescriptor(const void* record);

	//! +0x00, carried opaque like every other vtable slot in this model.
	void*				mVptrSlot;
	//! +0x04, first argument: the shape's owner.
	void*				mOwner04;
	//! +0x08, zeroed.
	NxU32				mWord08;
	//! +0x0c, +0x3c, +0x6c: three identity poses.
	ShapePose			mPose0C;
	ShapePose			mPose3C;
	ShapePose			mPose6C;
	//! +0x9c, zeroed.
	NxU32				mWord9C;
	//! +0xa0, zeroed; read as a pointer by the 0x00025760 helper.
	NxU32				mWordA0;
	//! +0xa4, the embedded scene-query prunable; its mOwner is set back to
	//! this shape before the constructor returns.
	Prunable			mPrunable;
	//! +0xd0, 0x7fffffff -- an INT_MAX-shaped sentinel, consumer unestablished.
	NxU32				mSentinelD0;
	//! +0xd4, the second constructor argument verbatim.
	NxU32				mArgumentD4;
	//! +0xd8, two zeroed halfwords.
	NxU16				mHalfwordD8;
	NxU16				mHalfwordDA;
	//! +0xdc = 6, +0xde = 8.
	NxU16				mHalfwordDC;
	NxU16				mHalfwordDE;
	};

static_assert(sizeof(ShapeBase) == 0xe0, "the base shape spans to where the hull embeds");
static_assert(offsetof(ShapeBase, mOwner04) == 0x04, "the owner sits at +0x04, as nxShapeOwner reads it");
static_assert(offsetof(ShapeBase, mPose0C) == 0x0c, "the first pose is at +0x0c");
static_assert(offsetof(ShapeBase, mPose3C) == 0x3c, "the second pose is at +0x3c");
static_assert(offsetof(ShapeBase, mPose6C) == 0x6c, "the third pose is at +0x6c");
static_assert(offsetof(ShapeBase, mWord9C) == 0x9c, "+0x9c is zeroed");
static_assert(offsetof(ShapeBase, mPrunable) == 0xa4, "the prunable is at +0xa4");
static_assert(offsetof(ShapeBase, mSentinelD0) == 0xd0, "the sentinel is at +0xd0");
static_assert(offsetof(ShapeBase, mArgumentD4) == 0xd4, "the second argument lands at +0xd4");
static_assert(offsetof(ShapeBase, mHalfwordDC) == 0xdc, "the 6/8 halfwords close the object");

/**
The box shape. Constructor phys_fn_000977 (0x00021870, 207 bytes,
__thiscall, `ret 8`), which forwards BOTH arguments unchanged to
ShapeBase::ShapeBase (0x0002187c..80: arg1 re-read from [esp+0xc] after two
pushes, pushed first, then arg2) and then:

	+0x000	vptr			final BOX primary table .rdata 0x10106ab8, store
	                        0x0002188f -- slot map resolved through the merged
	                        run at 0x106a58 (see evidence/phase5-object-model.md):
	                        0 phys_fn_000979, 1 phys_fn_001347(p3),
	                        2 phys_fn_001277(p3), 3 phys_fn_000945,
	                        4 phys_fn_000947, 5 phys_fn_000949 (the raycast
	                        partner Phase 3 named), 6 phys_fn_001315(p3),
	                        7 phys_fn_000951 (the sweep entry), 8 phys_fn_000941,
	                        9 phys_fn_000935, 10 phys_fn_000937, 11 phys_fn_000939,
	                        12 phys_fn_000981, 13 phys_fn_000927,
	                        14-16 phys_fn_001391(p3)
	+0x0e0	hull facade		final twelve-slot table .rdata 0x10106a88 at
	                        0x00021895; the abstract wall 0x10106a58 is stored
	                        first (0x00021885) as a chained-construction
	                        intermediate and never observable after return
	+0x150..+0x227	faces	the first THREE words of each of the six records are
	                        zeroed (corners, both index lists -- stores
	                        0x000218a1..ee); the float data stays whatever the
	                        memory held, so a fresh object's records carry
	                        poison until the face builder runs
	+0x09c	collision object	a fresh 0x1c-byte block through the SDK allocator
	                        ([0x101041bc] malloc slot, size 0x1c, flag 0 --
	                        0x000218f1..fe), constructed by phys_fn_001075
	                        (0x00023580: same body as phys_fn_001193 with the
	                        box-family tables 0x10106dc8/0x10106e54; the box is
	                        stored at +0x08 AND +0x18) and its address stored at
	                        +0x9c (0x00021911); null if the allocation failed
	+0x0d0	2				OVERWRITES the base ctor's 0x7fffffff sentinel
	                        (store 0x0002191c)
	+0x0e4..+0x0ec	dims	the facade gap words become 1.0f, 1.0f, 1.0f
	                        (stores 0x00021926..36)

The vertices (+0xf0..+0x14f) and every face-record float word stay untouched.
*/
class MassFrame;	// declared in full below its first pointer-using member

class BoxShape
	{
	public:
	//! phys_fn_000977 (0x00021870). Same argument pair as the base ctor.
					BoxShape(void* owner, unsigned argument);

	//! BOX-table slot 10, phys_fn_000937 (0x00020670): writes the pose-one
	//! translation (+0x30/+0x34/+0x38) to out[0..2] and a sqrt-of-squared-
	//! dims value to out[3]. The x87 association of the sum is unestablished
	//! for general dims; for the constructor defaults (1,1,1) every
	//! association yields the same bits, which is what the gate drives.
	void				nxBoxCenterAndDiagonal(float* out) const;

	//! BOX-table slot 11, phys_fn_000939 (0x000206c0): zeroes out[0..2] and
	//! writes the same sqrt-of-squared-dims value to out[3].
	void				nxBoxZeroCenterAndDiagonal(float* out) const;

	//! BOX-table slot 13, phys_fn_000927 (0x00020450): writes the facade
	//! dims (+0xe4/+0xe8/+0xec) to record+0x4c/0x50/0x54, then tail-jumps to
	//! the BASE save-to-descriptor row -- the descriptor's dims slot sits at
	//! exactly the offset the constructor read them back from.
	bool				nxBoxSaveState(void* record);

	//! BOX-table slot 12, phys_fn_000981: loadFromDesc -- stores dimensions
	//! from desc+0x4c/50/54 into the facade dims, recomputes derived data
	//! via helper 0x21420, then applies BASE fields.
	void				nxBoxLoadFromDesc(const void* record);

	//! BOX-table slot 8, phys_fn_000941 (0x00020700): local AABB -- out[0..2]
	//! = negated dims, out[3..5] = raw dims.
	void				nxBoxLocalAABB(float* out) const;

	//! BOX-table slot 9, phys_fn_000935 (0x000205a0): world AABB from pose
	//! one -- extents |rot-row . dims| per axis, then min = t - ext,
	//! max = t + ext. Exact for the identity pose the gate drives; the
	//! image's mixed single/extended rounding of partial sums is left open
	//! for general poses.
	void				nxBoxWorldAABB(float* out) const;

	//! BOX-table slots 14, 15 and 16, phys_fn_001391 (0x00027f00): a 3-byte
	//! `mov eax,ecx; ret` -- returns this, ignores every argument. The same
	//! row fills all three slots.
	BoxShape*			nxBoxSelf() const { return const_cast<BoxShape*>(this); }

	//! BOX-table slot 4, phys_fn_000849 (0x0001c8c0), __thiscall
	//! `ret 0xc`: the compute-mass row. Builds the unit-density solid-box
	//! frame from the three half-extents into a local MassFrame, folds the
	//! optional payload pair when `extra` is non-null, scales by `density`
	//! unless it equals the .rdata 1.0f sentinel (fucompp), then merges
	//! into *dest. Argument order as pushed: density, halfExtents, extra.
	void				nxBoxComputeMassFrame(MassFrame* dest, float density,
							const float* halfExtents, const void* extra);


	//! BOX-table slot 0, phys_fn_000979 (0x00021940): the scalar deleting
	//! destructor. Destroys the embedded collision object through its own
	//! vtable with flag 1 UNCONDITIONALLY (the colobj's own deleting row is
	/// separate and not yet transcribed), runs the base-dtor chain -- owner
	//! arms, +0xa0 arms, Prunable destruction -- and frees this through the
	//! SDK allocator slot +0x14 only when flags&1.
	void				nxBoxScalarDeletingDtor(unsigned flags);

	//! +0x00..+0xdf, the base shape subobject.
	ShapeBase			mBase;
	//! +0xe0..+0x227, the convex-hull descriptor.
	BoxHullFacade		mHull;
	};

static_assert(sizeof(BoxShape) == 0x228, "the box spans base plus hull facade");
static_assert(offsetof(BoxShape, mHull) == 0xe0, "the hull embeds where the base ends");

/**
The sphere shape. Constructor phys_fn_001349 (0x000277c0, 91 bytes,
__thiscall, `ret 8`), which forwards BOTH arguments unchanged to
ShapeBase::ShapeBase (0x000277bb..cf) and then:

	+0x000	vptr			final SPHERE table .rdata 0x10107528, store 0x000277d4
	+0x0e0	radius			ZEROED (store 0x000277da). The sphere has no secondary
	                        base: where the box embeds its hull facade the
	                        sphere keeps plain data, and its first datum is the
	                        radius. Construction leaves it 0; the descriptor
	                        path writes the real value here (0x00027850 reads
	                        the descriptor's word +0x4c straight into it).
	+0x09c	collision object	a fresh 0x1c-byte block through the SDK allocator
	                        (malloc(0x1c,0), 0x00027de4..f2) built by
	                        phys_fn_001193 ITSELF -- the generic collision-object
	                        constructor, not a per-type variant -- and stored at
	                        +0x9c (0x00027805); null if the allocation failed
	+0x0d0	1				sentinel, OVERWRITING the base ctor's 0x7fffffff
	                        (store 0x0002780b)

The per-shape sentinel values now read: box 2, sphere 1 -- plausibly an
NxShapeType tag, consumer unestablished.
*/
/**
The 0x34-byte mass frame the shape tables' slot-4 rows accumulate: a 3x3
inertia tensor at +0x00..+0x20, a center-of-mass offset at +0x24..+0x2c and a
mass at +0x30. Decoded out of the SPHERE slot-4 chain -- evidence section
3p carries the full disassembly walk. This is the structure the exported
Phase 3 mass kernels feed: the same .rdata constants (0x101068d8 = 4pi/3,
0x101068e4 = 2/5) participate on both sides.
*/
class MassFrame
	{
	public:
	//! phys_fn_000843 (0x0001c750), __thiscall `ret 8`: unit-density solid
	//! sphere of `radius`. mass = (4pi/3)r^3, diagonal inertia =
	//! (2/5)mass r^2, every other word zeroed. When `extra` is non-null two
	//! point-mass payloads at extra+0 and extra+0x24 fold in through
	//! phys_fn_000831/000833 -- not yet transcribed, so this transcription
	//! documents rather than reproduces that arm; the differential drives
	//! null.
	void				nxMassFrameBuildSphere(float radius, const void* extra);

	//! phys_fn_000837 (0x0001c5c0), __thiscall `ret 4`: multiply the nine
	//! inertia words and the mass by `s`. The COM offset does not
	//! participate -- scaling a frame by a density keeps its center.
	void				nxMassFrameScale(float s);

	//! phys_fn_000839 (0x0001c630), __thiscall `ret 4`: merge `other` into
	//! this. mass sums; the offset becomes the mass-weighted average
	//! divided through the .rdata 1.0f literal at 0x101041ec (a full x87
	//! division whose quotient is 1/sum, not a no-op); inertia accumulates
	//! componentwise. this and other must not alias.
	void				nxMassFrameMerge(const MassFrame& other);

	//! phys_fn_000829 (0x0001bd00), __thiscall `ret 4`: unit-density solid
	//! box over three HALF-extents. The accumulator opens at the .rdata
	//! 1.0f literal and each non-zero extent replaces-or-multiplies into
	//! it (an integer word-compare -- -0.0f counts as non-zero); mass =
	//! 8*hx*hy*hz ([0x101068f0]), diagonal = (mass/3) ([0x101068ec]) times
	//! the pairwise squared-extent sums, every other word integer-zeroed.
	void				nxMassFrameBuildBox(const float* halfExtents);

	//! phys_fn_000845 (0x0001c7c0), __thiscall `ret 0xc`: unit-density
	//! solid CYLINDER -- the hemispherical caps contribute nothing here.
	//! mass = pi*r^2*(2c); the axial diagonal gets mass*r^2/2, the other
	//! two the full cylinder transverse mass*(3r^2+4c^2)/12 over the
	//! .rdata 3 ([0x101068f8]) / 4 ([0x101068f4]) / one-twelfth constants.
	//! `axisSelector` picks the axial diagonal (0=x, 1=y, >=2=z). The
	//! selector==1 path never writes +0x00 -- an image hole reproduced by
	//! leaving that word untouched; drives use 0 and 2.
	void				nxMassFrameBuildCapsule(unsigned axisSelector,
							float radius, float cylHalfHeight);

	//! phys_fn_000831 (0x0001bdc0), __thiscall `ret 4`: the payload fold
	//! step the slot-4 wrappers run when their `extra` pointer is non-null.
	//! The 0x24-byte payload record is {Vec3 d; SymMat3 K} (upper triangle
	//! packed k00/k01/k02/k11/k12/k22 at +0x0c..+0x20). STRAIGHT-LINE
	//! TRANSFORM, not an accumulate: all nine inertia words are OVERWRITTEN
	//! from products of the old inertia with d and K, the COM offset becomes
	//! {o^T K col0, o^T K col1, o . d}, and the mass word is untouched.
	//! Transcribed op-for-op; section 3r carries the formula table.
	void				nxMassFrameFoldPayload(const void* payload);

	//! phys_fn_000847 (0x0001c880), __thiscall `ret 4`: conditionally zero
	//! the entire frame. When the byte argument is non-zero every word is
	//! integer-zeroed; when it is zero the frame is left untouched.
	void				nxMassFrameConditionalZero(unsigned flag);

	//! +0x00..+0x20, stored row-major as three column triples.
	NxF32				mInertia[9];
	//! +0x24..+0x2c.
	NxVec3				mOffset;
	//! +0x30.
	NxF32				mMass;		};

static_assert(sizeof(MassFrame) == 0x34, "the mass frame is thirteen floats");

/**
The 0x48-byte material record the SDK stores BY VALUE in its materials
array (stride 0x48 -- phase2-sdk.md section 4.6) and keeps as a statically
initialised default template at .data 0x1220a0. Field order is the pinned
NxMaterial header's; setToDefault matches both the pinned inline and
phys_fn_000472's eighteen-word write (all zero except dirOfAnisotropy.x and
dirOfMotion.x = 1.0f). The image then sets bit 31 of flags ON THE TEMPLATE
after copying it into the array (0x0000e9ee) -- modelled separately so the
fresh-record and shipped-template states stay distinguishable.
*/
class NxMaterialRecord
	{
	public:
					NxMaterialRecord(void);
	void			setToDefault(void);
	void			setInternalFlagBit31(void);

	NxF32				dynamicFriction;			//!< +0x00
	NxF32				staticFriction;				//!< +0x04
	NxF32				spinFriction;				//!< +0x08, not yet implemented
	NxF32				rollFriction;				//!< +0x0c, not yet implemented
	NxF32				restitution;				//!< +0x10
	NxF32				dynamicFrictionV;			//!< +0x14
	NxF32				staticFrictionV;			//!< +0x18
	NxVec3				dirOfAnisotropy;			//!< +0x1c
	NxVec3				dirOfMotion;				//!< +0x28
	NxF32				speedOfMotion;				//!< +0x34
	NxU32				flags;						//!< +0x38
	NxU32				frictionCombineMode;		//!< +0x3c, NX_CM_AVERAGE
	NxU32				restitutionCombineMode;		//!< +0x40, NX_CM_AVERAGE
	void*				programData;				//!< +0x44
	};

static_assert(sizeof(NxMaterialRecord) == 0x48, "the material record is 72 bytes");
static_assert(offsetof(NxMaterialRecord, dirOfAnisotropy) == 0x1c, "+0x1c carries the 1.0f");
static_assert(offsetof(NxMaterialRecord, flags) == 0x38, "flags at +0x38 per the pinned header");

/**
The error stream. The image reports warnings through an indirect cdecl
call with five arguments -- (kind, sourceFile, line, code, message) --
through a function-pointer slot guarded by a non-zero flag word; the
literals live in .rdata ("\\Epic\\Novodex\\SDKs\\Physics\\src\\SphereShape.cpp",
"SphereShape::setRadius: radius should be positive!", line 0x4a). See
evidence section 3t.
*/
typedef void(__cdecl* NxReportFn)(int kind, const char* file, int line,
	int code, const char* message);

//! Install the reconstruction's report sink (null restores silence).
void					nxInstallReportSink(NxReportFn sink);

//! Task 4 scaffolding: the scene shape-array insert the base ctor's owned
//! arm performs. Reproduces the three observable writes -- shape pointer
//! into the +0x90 vector, free-list sentinel -1 into the +0x00 vector, and
//! the +0x10/14 vector's count into the +0x20 vector -- all indexed by the
//! shape's scene slot. Pre-sized containers only: growth is not modelled.
void					nxSceneInsertShape(void* container, void* shape,
							NxU32 slot);

//! Task 4 scaffolding: remover #1 (phys_fn_002410's release arm + the
//! 0x5bbe0 clear). Freelist push of the scene slot when not already freed,
//! then swap-remove across the sentinel/count/mirror arrays with the
//! 0xD00BEED0 poison, then shapes[slot] = 0.
void					nxSceneRemoveShape(void* container, void* shape);

//! phys_fn_002410 (0x5bac0) alone, without the shapes-array clear its
//! callers add: sentinel != -1 gates ONLY the free-vector push at +0x30
//! (full growth arm), and sentinel == 0 gates only the unlink -- so a
//! virgin index skips the push but still unlinks, and a released index
//! pushes a duplicate while skipping the unlink. The unlink swaps the count
//! vector's last value across the +0x10/+0x14/+0x20 arrays and poisons the
//! mirror word with 0xD00BEED0.
void					nxSceneReleaseIndex(void* hdr, NxU32 idx);

//! Task 4 scaffolding: remover #2 (0x5aae0). `container` is the ADDRESS OF
//! THE SCENE'S +0x5d4 FIELD (0x00026c13 add ecx,0x5d4); *[container] names
//! the {begin,end} header of the stride-8 pair array. Removes every pair
//! referencing `shape` from either half via swap-with-last.
void					nxSceneRemovePairs(void* container, void* shape);

//! phys_fn_000028 (0x1b90): dword-vector push_back over the VC9 layout
//! {_Myproxy@+0x00 untouched by this row, _Myfirst@+0x04, _Mylast@+0x08,
//! _Myend@+0x0c}. Full fidelity including the growth arm -- new capacity
//! 2*size + 2 dwords allocated through nxGetSdkAllocator, elements copied,
//! old block released -- so the differential can drive both sides into
//! realloc and fold the allocation stream.
void					nxU32VectorPushBack(void* vecHeader, NxU32 value);

//! Task 4 scaffolding: remover #3 = nxU32VectorPushBack on the container's
//! own header; kept as a name because the deregistration chain reads in
//! terms of it.
void					nxSceneSlotFree(void* container, NxU32 slot);

//! The image's own literal pair, exposed so tests can pin against them.
extern const char* const	nxSourceFileSphereShapeCpp;
extern const char* const	nxMsgSetRadiusPositive;
//! And Shape.cpp's pair for the group-validation arm.
extern const char* const	nxSourceFileShapeCpp;
extern const char* const	nxMsgGroupBelow32;
//! SphereShape.cpp's loadFromDesc pair (line 0x35).
extern const char* const	nxSourceFileSphereLoadCpp;
extern const char* const	nxMsgSphereLoadRadius;
//! CapsuleShape.cpp's loadFromDesc pair (line 0x37).
extern const char* const	nxSourceFileCapsuleShapeCpp;
extern const char* const	nxMsgCapsuleLoadRadius;

/**
The sphere shape. Constructor phys_fn_001349 (0x000277c0).
*/
class SphereShape
	{
	public:
	//! phys_fn_001349 (0x000277c0). Same argument pair as the base ctor.
					SphereShape(void* owner, unsigned argument);

	//! +0x00..+0xdf, the base shape subobject.
	ShapeBase			mBase;
	//! +0xe0, the radius; zero until a radius source writes it.
	float				mRadiusE0;

	//! SPHERE-table slot 15, phys_fn_001359 (0x00027920): `fld [+0xe0]; ret`.
	float				nxSphereGetRadius() const
							{ return mRadiusE0; }

	//! SPHERE-table slot 13, phys_fn_001355 (0x000278a0): radius to
	//! record+0x4c, then the BASE save-to-descriptor row.
	bool				nxSphereSaveState(void* record);

	//! SPHERE-table slot 9, phys_fn_001361 (0x00027930): world AABB --
	//! min = t - r, max = t + r per axis (six floats).
	void				nxSphereWorldAABB(float* out) const;

	//! SPHERE-table slot 8, phys_fn_001367 (0x000279d0): local AABB --
	//! negated radius to out[0..2], raw radius to out[3..5]. On a fresh
	//! sphere the mins are -0.0f (sign bit set), which is exactly what the
	//! differential pins.
	void				nxSphereLocalAABB(float* out) const;

	//! SPHERE-table slot 14, phys_fn_001357 (0x000278c0): set-radius.
	//! Stores unconditionally, then (image order) reports an invalid
	//! argument through the error stream when the fcomp against the
	//! zero-global fails, notifies the owner through BASE slot 6, and marks
	//! dirty-flag 0x20 -- all three null-owner/Task-4 no-ops on a fresh
	//! shape.
	void				nxSphereSetRadius(float radius);

	//! SPHERE-table slot 0, phys_fn_001375 (0x00027c30): scalar deleting
	//! destructor -- destroys the embedded collision object unconditionally,
	//! runs the base-dtor chain, frees this through allocator slot +0x14
	//! when flags&1.
	void				nxSphereScalarDeletingDtor(unsigned flags);

	//! SPHERE-table slot 12, phys_fn_001353 (0x00027850): loadFromDesc.
	//! Reads radius from record+0x4c (validating through the error stream),
	//! stores at +0xe0, then tail-calls the BASE apply-desc row.
	void				nxSphereLoadFromDesc(const void* record);

	//! SPHERE-table slot 4, phys_fn_000851 (0x0001c930), __thiscall
	//! `ret 0xc`: the compute-mass row. Builds the unit-density solid-sphere
	//! frame into a local MassFrame, folds the optional payload pair when
	//! `extra` is non-null, scales by `density` unless it equals the .rdata
	//! 1.0f sentinel (fucompp -- an unordered density still scales), then
	//! merges into *dest. Argument order as pushed: density, radius, extra.
	void				nxSphereComputeMassFrame(MassFrame* dest, float density,
							float radius, const void* extra);

	//! SPHERE-table slot 11, phys_fn_001365 (0x000279b0): zeroes out[0..2],
	//! radius to out[3].
	void				nxSphereZeroCenterRadius(float* out) const;

	//! SPHERE-table slot 10, phys_fn_001363 (0x00027980): pose-one
	//! translation to out[0..2], radius to out[3].
	void				nxSphereCenterRadius(float* out) const;	};

static_assert(sizeof(SphereShape) == 0xe4, "the sphere is base plus one float so far");
static_assert(offsetof(SphereShape, mRadiusE0) == 0xe0, "the radius is the first word past the base");

/**
The capsule shape. Constructor phys_fn_000987 (0x00021a60, 98 bytes,
__thiscall, `ret 8`), which forwards BOTH arguments unchanged to
ShapeBase::ShapeBase (0x00021a67..6f) and then:

	+0x000	vptr			final CAPSULE table .rdata 0x10106b20, store 0x00021a74
	+0x09c	collision object	a fresh 0x1c-byte block through the SDK allocator
	                        (malloc(0x1c,0), 0x00021a8e..9c) built by
	                        phys_fn_001123 (0x00023cb0, the capsule-family variant
	                        of the shared collision-object constructor) and stored
	                        at +0x9c (0x00021aaf); null if allocation failed
	+0x0d0	3				sentinel = NX_SHAPE_CAPSULE, overwriting the base's
	                        0x7fffffff (store 0x00021ab5)
	+0x0e0, +0x0e4	two zeroed floats -- by analogy with the sphere's radius
	                        these are the capsule's first data words (radius and
	                        half-height candidates); the descriptor path names
	                        them, this constructor only zeroes them.
*/
class CapsuleShape
	{
	public:
	//! phys_fn_000987 (0x00021a60). Same argument pair as the base ctor.
					CapsuleShape(void* owner, unsigned argument);

	//! +0x00..+0xdf, the base shape subobject.
	ShapeBase			mBase;
	//! +0x0e0, +0x0e4, two data words construction zeroes; unnamed yet.
	float				mFloatE0;
	float				mFloatE4;

	//! +0x0e8, a third data word. Its existence is fixed by the slot-13
	//! save row phys_fn_000991, which copies it raw into descriptor+0x54;
	//! the constructor leaves it poisoned, unnamed, and untouched.
	NxU32				mWordE8;

	//! CAPSULE-table slot 13, phys_fn_000991 (0x00021b40): radius to
	//! record+0x4c, TWICE mFloatE4 to record+0x50 (the descriptor's height is
	//! the full height; the stored word is the half-height), mWordE8 raw to
	//! record+0x54, then the BASE save-to-descriptor row.
	bool				nxCapsuleSaveState(void* record);

	//! CAPSULE-table slot 14, phys_fn_000995 (0x00021be0): set-radius.
	//! Stores to +0xe0 then tail-jumps through BASE slot 6 -- a null-owner
	//! no-op on a detached shape.
	void				nxCapsuleSetRadius(float radius);

	//! CAPSULE-table slot 0, phys_fn_001014 (0x000225e0): scalar deleting
	//! destructor -- destroys colobj unconditionally, base-dtor chain,
	//! self-free through allocator slot +0x14 when flags&1.
	void				nxCapsuleScalarDeletingDtor(unsigned flags);

	//! CAPSULE-table slot 8, phys_fn_001004 (0x00021c80): local AABB --
	//! min = (-r, -(hh+r), -r), max = (+r, +(hh+r), +r) where r = radius
	//! and hh = stored half-height.
	void				nxCapsuleLocalAABB(float* out) const;

	//! CAPSULE-table slot 12, phys_fn_000989 (0x00021ad0): loadFromDesc --
	//! reads radius (+0xe0), half-height (desc+0x50 * 0.5f), third word
	//! (+0xe8) from the descriptor, then applies base fields through
	//! BASE slot 1.
	void				nxCapsuleLoadFromDesc(const void* record);

	//! CAPSULE-table slot 4, phys_fn_000853 (0x0001c980), __thiscall
	//! `ret 0x14`: the compute-mass row. Builds the unit-density cylinder
	//! frame into a local MassFrame, folds the optional payload pair when
	//! `extra` is non-null, scales by `density` unless it equals the .rdata
	//! 1.0f sentinel, then merges into *dest. Argument order as pushed:
	//! density, axisSelector, radius, cylHalfHeight, extra.
	void				nxCapsuleComputeMassFrame(MassFrame* dest, float density,
							unsigned axisSelector, float radius,
							float cylHalfHeight, const void* extra);

	//! CAPSULE-table slot 10, phys_fn_001001 (0x00021c30): pose-one
	//! translation to out[0..2], halfHeight+radius to out[3].
	void				nxCapsuleCenterRadius(float* out) const;

	//! CAPSULE-table slot 11, phys_fn_001003 (0x00021c60): zeroes out[0..2],
	//! halfHeight+radius to out[3].
	void				nxCapsuleZeroCenterRadius(float* out) const;
	};

static_assert(sizeof(CapsuleShape) == 0xec, "the capsule is base plus three data words");
static_assert(offsetof(CapsuleShape, mFloatE0) == 0xe0, "the first datum is at +0xe0");
static_assert(offsetof(CapsuleShape, mFloatE4) == 0xe4, "the second datum is at +0xe4");
static_assert(offsetof(CapsuleShape, mWordE8) == 0xe8, "the third datum is at +0xe8");

/**
The plane shape. Constructor phys_fn_001247 (0x00024ed0, 141 bytes,
__thiscall, `ret 8`), which forwards BOTH arguments unchanged to
ShapeBase::ShapeBase (0x00024edb..df) and then:

	+0x000	vptr			final PLANE table .rdata 0x10107430, store 0x00024ee4
	+0x09c	collision object	a fresh 0x1c-byte block through the SDK allocator
	                        (malloc(0x1c,0), 0x00024eea..f8) built by
	                        phys_fn_001159 (0x00024250, the plane-family variant
	                        of the shared collision-object constructor) and
	                        stored at +0x9c (0x00024f0b); null if allocation failed
	+0x0d0	0				sentinel = NX_SHAPE_PLANE, overwriting the base's
	                        0x7fffffff (store 0x00024f24). The +0xd0 word is now
	                        PROVEN to be the NxShapeType tag: sphere 1, box 2,
	                        capsule 3, mesh 4, plane 0 -- exactly the pinned
	                        NxShape.h enumeration.
	+0x0e0..+0xeb	plane equation	NxPlane { normal(0,1,0), D=0 }: stores
	                        0x00024f2f/35/3c (normal words; y carries
	                        0x3f800000) and 0x00024f44 (D)
	+0x0f0..+0xfb	tangent u	filled by NxNormalToTangents(normal, u, v),
	                        called through the NxFoundation import at
	                        .rdata 0x1010418c (call 0x00024f4e)
	+0x0fc..+0x107	tangent v	filled by the same call
	+0x108	1				store 0x00024f57

The shape-type tags also identify phys_fn_001033 (0x22d60): sentinel 5 =
NX_SHAPE_COMPOUND -- that constructor stores table .rdata 0x10106c2c,
zeroes two vec3-sized triplets at +0xe0/+0xf0 (skipping +0xec), writes
halfword +0xd8 = 0xffff and float -1.0f at +0x10c, and builds NO collision
object. It stays untranscribed here.
*/
class PlaneShape
	{
	public:
	//! phys_fn_001247 (0x00024ed0). Same argument pair as the base ctor.
					PlaneShape(void* owner, unsigned argument);

	//! +0x00..+0xdf, the base shape subobject.
	ShapeBase			mBase;
	//! +0x0e0..+0x0eb, the plane equation: normal then distance.
	float				mNormalE0[3];
	float				mDistanceEC;
	//! +0x0f0..+0x0fb, first tangent from NxNormalToTangents.
	float				mTangentF0[3];
	//! +0x0fc..+0x107, second tangent.
	float				mBinormalFC[3];
	//! +0x108, one.
	NxU32				mWord108;

	//! PLANE-table slot 13, phys_fn_001251 (0x00024f80): writes the normal to
	//! record+0x4c/50/54 and the NEGATED distance to record+0x58 -- the
	//! descriptor stores -D, which is exactly the transform the ctor's
	//! load path inverts -- then tail-jumps to the BASE save-to-descriptor
	//! row.
	bool				nxPlaneSaveState(void* record);

	//! PLANE-table slot 0, phys_fn_001263 (0x00025420): scalar deleting
	//! destructor -- colobj destroyed unconditionally, base-dtor chain,
	//! self-free through allocator slot +0x14 when flags&1.
	void				nxPlaneScalarDeletingDtor(unsigned flags);

	//! PLANE-table slot 12, phys_fn_001265: loadFromDesc -- stores the
	//! descriptor normal/D through helper 0x24fc0 then applies BASE fields.
	void				nxPlaneLoadFromDesc(const void* record);

	//! PLANE-table slots 9 and 11, phys_fn_001257 (0x000251d0): one row
	//! filling two slots -- zeroes out[0..2] and writes +FLT_MAX (the
	//! plane's unbounded reach) to out[3].
	void				nxPlaneExtentRow(float* out) const;
	};

static_assert(sizeof(PlaneShape) == 0x10c, "the plane spans base plus its basis frame");
static_assert(offsetof(PlaneShape, mNormalE0) == 0xe0, "the normal starts where the base ends");
static_assert(offsetof(PlaneShape, mDistanceEC) == 0xec, "the distance follows the normal");
static_assert(offsetof(PlaneShape, mTangentF0) == 0xf0, "the first tangent is at +0xf0");
static_assert(offsetof(PlaneShape, mBinormalFC) == 0xfc, "the second tangent is at +0xfc");

/**
The mesh shape. Constructor phys_fn_001379 (0x00027db0, 101 bytes,
__thiscall, `ret 8`), which forwards BOTH arguments unchanged to
ShapeBase::ShapeBase (0x00027dbb..bf) and then:

	+0x000	vptr			final MESH table .rdata 0x10107630, store 0x00027dc4
	+0x09c	collision object	a fresh 0x1c-byte block through the SDK allocator
	                        (malloc(0x1c,0), 0x00027dde..ec) built by
	                        phys_fn_001241 (0x00024e40, the mesh-family variant
	                        of the shared collision-object constructor) and
	                        stored at +0x9c (0x00027dff); null if allocation failed
	+0x0d0	4				sentinel = NX_SHAPE_MESH, overwriting the base's
	                        0x7fffffff (store 0x00027e05)
	+0x0e0, +0x0e4	two zeroed words -- by every analogue so far these are the
	                        mesh's first data words (a triangle-mesh pointer and
	                        a flag word are the natural candidates); this
	                        constructor only zeroes them, and nothing in it
	                        names them.

Like every family so far, the mesh writes its own tail (vptr 0x00027dc4,
zeroes 0x00027dca/dd4) after the forwarded base construction returns.
*/
class MeshShape
	{
	public:
	//! phys_fn_001379 (0x00027db0). Same argument pair as the base ctor.
					MeshShape(void* owner, unsigned argument);

	//! +0x00..+0xdf, the base shape subobject.
	ShapeBase			mBase;
	//! +0x0e0, +0x0e4, two data words construction zeroes; unnamed yet.
	NxU32				mWordE0;
	NxU32				mWordE4;

	//! MESH-table slot 17, phys_fn_001381 (0x00027e20): dereferences the
	//! mesh-data pointer at +0xe0 and returns its word at +0xe4. The probe
	//  plants its own record because the real assignment is a later task.
	NxU32				nxMeshGetMeshWord() const
							{ return *reinterpret_cast<const NxU32*>(
								reinterpret_cast<const unsigned char*>(mWordE0) + 0xe4); }

	//! MESH-table slot 0, phys_fn_001399 (0x00028e80): scalar deleting
	//! destructor -- colobj destroyed unconditionally, the bound mesh's
	//! refcount at mesh+0x74 DECREMENTED when non-null, base-dtor chain,
	//! self-free through allocator slot +0x14 when flags&1.
	void				nxMeshScalarDeletingDtor(unsigned flags);

	//! MESH-table slot 13, phys_fn_001385 (0x00027e60): the mesh's +0xe4
	//! word to record+0x4c, mWordE4 to record+0x50, then the BASE
	//! save-to-descriptor row.
	bool				nxMeshSaveState(void* record);

	//! MESH-table slot 11, phys_fn_001387 (0x00027e90): copies four words
	//! from mesh+0x5c into out.
	void				nxMeshGetWords5C(unsigned* out) const;

	//! MESH-table slot 8, phys_fn_001389 (0x00027ec0): copies six words from
	//! meshptr+0x44 into out.
	void				nxMeshGetWords44(unsigned* out) const;

	//! MESH-table slot 12, phys_fn_001383: loadFromDesc. The record holds
	//! a wrapper pointer; stores *(wrapper+4) at +0xe0 and increments the
	//! inner object refcount at +0x74.
	bool				nxMeshLoadFromDesc(const void* record);
	};

static_assert(sizeof(MeshShape) == 0xe8, "the mesh is base plus two words so far");
static_assert(offsetof(MeshShape, mWordE0) == 0xe0, "the first datum is at +0xe0");
static_assert(offsetof(MeshShape, mWordE4) == 0xe4, "the second datum is at +0xe4");

#endif
