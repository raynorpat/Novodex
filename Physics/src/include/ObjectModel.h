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
class SphereShape
	{
	public:
	//! phys_fn_001349 (0x000277c0). Same argument pair as the base ctor.
					SphereShape(void* owner, unsigned argument);

	//! +0x00..+0xdf, the base shape subobject.
	ShapeBase			mBase;
	//! +0xe0, the radius; zero until a radius source writes it.
	float				mRadiusE0;
	};

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
	};

static_assert(sizeof(CapsuleShape) == 0xe8, "the capsule is base plus two floats so far");
static_assert(offsetof(CapsuleShape, mFloatE0) == 0xe0, "the first datum is at +0xe0");
static_assert(offsetof(CapsuleShape, mFloatE4) == 0xe4, "the second datum is at +0xe4");

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
	};

static_assert(sizeof(MeshShape) == 0xe8, "the mesh is base plus two words so far");
static_assert(offsetof(MeshShape, mWordE0) == 0xe0, "the first datum is at +0xe0");
static_assert(offsetof(MeshShape, mWordE4) == 0xe4, "the second datum is at +0xe4");

#endif
