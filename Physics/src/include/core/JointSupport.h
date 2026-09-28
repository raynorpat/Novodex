#ifndef NX_PHYSICS_CORE_JOINTSUPPORT
#define NX_PHYSICS_CORE_JOINTSUPPORT
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Declarations for Physics/src/core/JointSupport.cpp (revolute pilot, Task 6).
// Two groups: the constraint-record rows phys_fn_004389/004391/004393, which
// are not Joint members (see revolute-contract.md "## Row assignment", the
// 004389/004393 decision notes), and the body-record rows from the gap units
// the joint code reaches (000022, 000712, 000758, 000760, 000778, written by
// joint-open-items Task 2; 000754 is also written below, by Task 6 (21b275d),
// with 004167 still deferred). The Scene
// rows the joint code calls (000571, 000598, 000633, 000661) are NxSceneInternal
// members in Physics/src/Scene.cpp (units/joint-open-items-contract.md
// "## Scene joint rows"). Every name below is by offset or row ID: the listing
// establishes these fields, not their meaning.

#include "Nxp.h"
#include "NxVec3.h"
#include "PhysicsInternal.h"

#include <cstddef>

// What the record's two pointers (+0x10/+0x14) point at: the record a body's
// JointBodyRecord::mUnknown204 points to (phys_fn_004360 and phys_fn_004362
// store body+0x204 into the record's +0x10/+0x14, 0xaa0ac/0xab1e2/0xab1f9).
// phys_fn_004389 multiplies +0x00..+0x08 with the record's +0x00 vector and
// +0x10..+0x18 with the record's +0x18/+0x24 vectors (or, on the bit-10 path,
// with +0x00); phys_fn_004358 forms +0x00 + (+0x10 x r); phys_fn_004374 adds
// +0x0c times an impulse to +0x00 and the +0x20 3x3 times r x impulse to
// +0x10. Meanings unknown; declared by offset.
//
// It is one 0x60-byte element of the Scene's per-step array at +0x5ac
// (count word at [+0x5ac]-4, used count +0x5b0, capacity +0x5b4), grown by
// phys_fn_000600 (`count * 0x60 + 4` bytes, 0x11023-0x1102e). phys_fn_000611,
// on the simulation thread's step (002400 -> 000659 -> 000655 -> 000611),
// fills one element per island body and is the only writer of body+0x204
// (0x11305). The elements are per island, not per body: 000611 reloads the
// base from +0x5ac for each island (0x112b6) and starts again at element 0,
// so bodies in different islands share elements, and 000600 may reallocate
// the array between islands; phys_fn_000613 then copies +0x00/+0x10/+0x44/+0x50 back to the
// body through phys_fn_000708. The body constructor phys_fn_000797 stores 0
// at +0x204 (0x1b713). See units/joint-open-items-contract.md
// "## Body record +0x204".
struct JointSupportBody
	{
	NxVec3				mUnknown000;	//!< +0x00; body+0x34..+0x3c (000611), back to body+0x34 (000708)
	NxReal				mUnknown00c;	//!< +0x0c; body+0xc0 (000611); scales +0x00's increment (004374)
	NxVec3				mUnknown010;	//!< +0x10; body+0x40..+0x48 (000611), back to body+0x40 (000708)
	void*				mUnknown01c;	//!< +0x1c; the body record itself (000611 0x112ec, read by 000613 0x11372)
	NxReal				mUnknown020[9];	//!< +0x20; row-major 3x3, body+0x164 (000611 rep movsd; 004374 0xad345-0xad3a9)
	NxVec3				mUnknown044;	//!< +0x44; copy of +0x00 after the record passes (004174 0x9b1a0); to body+0x1a0 (000708)
	NxVec3				mUnknown050;	//!< +0x50; copy of +0x10 after the record passes (004174); to body+0x1ac (000708)
	NxU32				mUnknown05c;	//!< +0x5c; body+0x110 (000611 0x1130b); pass filter in 004174
	};

static_assert(offsetof(JointSupportBody, mUnknown00c) == 0x0c, "scale at +0x0c");
static_assert(offsetof(JointSupportBody, mUnknown010) == 0x10, "second vector at +0x10");
static_assert(offsetof(JointSupportBody, mUnknown01c) == 0x1c, "body back-pointer at +0x1c");
static_assert(offsetof(JointSupportBody, mUnknown020) == 0x20, "3x3 at +0x20");
static_assert(offsetof(JointSupportBody, mUnknown044) == 0x44, "vector at +0x44");
static_assert(offsetof(JointSupportBody, mUnknown050) == 0x50, "vector at +0x50");
static_assert(offsetof(JointSupportBody, mUnknown05c) == 0x5c, "word at +0x5c");
static_assert(sizeof(JointSupportBody) == 0x60, "000600/000611 step the array by 0x60");

// The record phys_fn_004389/004391/004393 run on (`this` in ecx). 0x50
// bytes: phys_fn_004093 hands out element `index` of the Scene's array at
// +0x5b8 as `[Scene+0x5b8] + index * 0x50` (0x95dde-0x95df5). Only the
// offsets the pilot rows touch are named; meanings unknown.
struct JointSupportRecord
	{
	NxVec3				mUnknown000;	//!< +0x00
	NxU32				mFlags;			//!< +0x0c; bits 0-4 kind, bit 10 selects 004389's second form
	JointSupportBody*	mBody[2];		//!< +0x10 / +0x14; either may be null
	NxVec3				mUnknown018;	//!< +0x18; paired with mBody[0]
	NxVec3				mUnknown024;	//!< +0x24; paired with mBody[1]
	void*				mUnknown030;	//!< +0x30; the joint that filled the record (004360, 004362)
	NxReal				mUnknown034;	//!< +0x34
	NxReal				mUnknown038;	//!< +0x38
	NxReal				mUnknown03c;	//!< +0x3c; written by 004393
	NxReal				mUnknown040;	//!< +0x40; written by 004391 and 004393
	NxU32				mUnknown044;	//!< +0x44 (zeroed)
	NxReal				mUnknown048;	//!< +0x48
	NxU32				mUnknown04c;	//!< +0x4c (zeroed)

	//! phys_fn_004389 (0x000af2d0, 227 B). `this` in ecx, no stack
	//! arguments, plain `ret`: a thiscall member with no arguments. The
	//! result is left unrounded in st(0), so it is declared NxF64; the
	//! caller rounds where it stores.
	NxF64				row004389() const;

	//! phys_fn_004391 (0x000af3c0, 837 B; write: every family's solver
	//! slots and the Joint base's slot 7). Thiscall, two pointer arguments,
	//! `ret 8`: out0 = the record's effective mass term (bit 10 clear: the
	//! +0x00/+0x18/+0x24 vectors through both bodies' +0x0c scale and +0x20
	//! 3x3; set: +0x00 through the 3x3s only), out1 = 1 / out0, or 0 when
	//! out0 is 0. 004393 passes two locals and reads only the second.
	void				row004391(NxReal& out0, NxReal& out1);

	//! phys_fn_004393 (0x000af710, 122 B). Thiscall, two float arguments,
	//! `ret 8`.
	void				row004393(NxReal arg0, NxReal arg1);
	};

static_assert(offsetof(JointSupportRecord, mFlags) == 0x0c, "flags at +0x0c");
static_assert(offsetof(JointSupportRecord, mBody) == 0x10, "body pointers at +0x10");
static_assert(offsetof(JointSupportRecord, mUnknown018) == 0x18, "vector at +0x18");
static_assert(offsetof(JointSupportRecord, mUnknown024) == 0x24, "vector at +0x24");
static_assert(offsetof(JointSupportRecord, mUnknown03c) == 0x3c, "output at +0x3c");
static_assert(offsetof(JointSupportRecord, mUnknown040) == 0x40, "output at +0x40");
static_assert(offsetof(JointSupportRecord, mUnknown030) == 0x30, "joint at +0x30");
static_assert(offsetof(JointSupportRecord, mUnknown048) == 0x48, "float at +0x48");
static_assert(sizeof(JointSupportRecord) == 0x50, "records are 0x50 bytes (phys_fn_004093)");

// The receiver types below are fixtures: MSVC rejects __thiscall on a free
// function (C3865), so each thiscall row is a member of a non-virtual struct
// whose pointer bits stand in for the record it runs on (the ObjectModel.h
// row004165/3413 convention). None is ever constructed.

// phys_fn_000022 (0x00001840, 27 B; owner gap <start>..Actor.cpp). Thiscall
// on the 0x50-byte actor body (JointBodyRecord::mOwner, `mov ecx,[body+0x19c];
// push 1`), one stack argument, `ret 4`: phys_fn_000754 on the body's +0x08
// record, then, when +0x10 is set, a tail jump to that object's slot 6 with
// the same argument.
struct Row000022Fixture
	{
	void row000022(NxU32 arg);
	};

// The object at actor body +0x10 that phys_fn_000022 tail-calls: slot 6
// (`jmp [eax+0x18]`) with one stack argument. Declared as an interface so the
// call is a real virtual dispatch; nothing implements it here.
struct Row000022Target
	{
	virtual void slot0() = 0;
	virtual void slot1() = 0;
	virtual void slot2() = 0;
	virtual void slot3() = 0;
	virtual void slot4() = 0;
	virtual void slot5() = 0;
	virtual void slot6(NxU32 arg) = 0;
	};

// phys_fn_000754 (0x00017010, 1027 B; owner gap SceneRaycast..CapsuleShape;
// written by joint-open-items Task 6, 21b275d). Thiscall on the body record,
// no stack arguments, plain `ret`: rebuilds the body's pose (+0x18 position,
// +0x24 quaternion) from its centre-of-mass pose (x87).
struct Row000754Fixture
	{
	void row000754();
	};

// phys_fn_000758 (0x00017630, 214 B; owner gap SceneRaycast..CapsuleShape).
// `this` (the body record) in ecx, no stack arguments, plain `ret`: rebuilds
// the record's +0x134 3x3 from its +0x124 quaternion (x, y, z, w).
struct Row000758Fixture
	{
	void row000758();
	};

// phys_fn_000712 (0x00015d30, 32 B; owner gap SceneRaycast..CapsuleShape).
// Thiscall on a body record, no stack arguments, plain `ret`: the island
// root at +0x1bc, found recursively with path compression.
struct Row000712Fixture
	{
	Row000712Fixture* row000712();
	};

// phys_fn_000760 (0x00017710, 168 B; owner gap SceneRaycast..CapsuleShape).
// Thiscall on a body record, no stack arguments, plain `ret`: resets the
// record's island fields (+0x1bc..+0x1e4) to a single-body island, frees the
// island object at +0x1e0 when the record is its own root, and raises the
// +0x4c wake counter to 0.4f as phys_fn_004107 does.
struct Row000760Fixture
	{
	void row000760();
	};

// phys_fn_000722 (0x00016130, 127 B; owner gap SceneRaycast..CapsuleShape).
// Thiscall on a body record, no stack arguments, plain `ret`: the record's
// island snapshot. The root (+0x1bc, refreshed through 000712 when the
// record is not its own) is read; a record that is its own root stores the
// largest +0x4c over its island chain (+0x1d0 links, starting from 0.0f) at
// +0x1cc, any other stores 0x4b7afafa there; the seven words +0x1bc..+0x1d4
// are copied to +0x1e8..+0x200, and +0x25c and +0x208 are zeroed. The record
// constructor 000797 and destructor 000776 call it.
struct Row000722Fixture
	{
	void row000722();
	};

// phys_fn_000778 (0x000185f0, 58 B, with its continuation phys_fn_000780 at
// 0x00018630, 243 B; owner gap SceneRaycast..CapsuleShape). Thiscall on a
// body record, two stack arguments, `ret 8`: dissolves the body's island.
// Every joint on each island body's +0x1d8 list (linked through Joint +0x34)
// other than `joint` is pushed back onto `jointArray` (the Scene's +0x58c
// array) and unlinked, and each island body is reset through 000760.
// phys_fn_000633 calls it on the joint's first non-null body.
struct Row000778Fixture
	{
	void row000778(void* joint, void** jointArray);
	};

// phys_fn_004167 (0x0009ad10, 156 B; owner gap Joint.cpp..D6Joint.cpp;
// deferred). Thiscall on the island object at body record +0x1e0, no stack
// arguments; phys_fn_000760 calls it before freeing the object. Its stub
// is NX_ASSERT(0): a silent no-op in Release (/DNDEBUG).
struct Row004167Fixture
	{
	void row004167();
	};

// The pointer-array push that phys_fn_000661 (0x13e53-0x13f12) and
// phys_fn_000780 (0x1864c-0x186e6) both inline. `array` is {begin, end,
// capacity}. With no room (capacity <= end) the array grows to 2n + 2
// entries unless its capacity already covers that: a new block from the Foundation
// allocator (slot +8 with (bytes, 0)), the old entries copied, the old block
// freed (slot +0x14), then end = new + n and capacity = new + bytes. The
// value is stored at end and end advances by one entry.
inline void nxJointPointerArrayPush(void** array, void* value)
	{
	void** begin = static_cast<void**>(array[0]);
	void** end = static_cast<void**>(array[1]);
	void** capacity = static_cast<void**>(array[2]);
	if(!(capacity > end))
		{
		const NxI32 wanted = (NxI32)(((NxU8*)end - (NxU8*)begin) >> 2) * 2 + 2;
		const NxI32 held = begin ? (NxI32)(((NxU8*)capacity - (NxU8*)begin) >> 2) : 0;
		if((NxU32)held < (NxU32)wanted)
			{
			const NxU32 bytes = (NxU32)wanted * 4;
			void** block = static_cast<void**>(nxFoundationSDKAllocator->malloc(bytes, NX_MEMORY_PERSISTENT));
			void** from = static_cast<void**>(array[0]);
			void** to = block;
			while(from != end)
				*to++ = *from++;
			if(array[0])
				nxFoundationSDKAllocator->free(array[0]);
			const NxI32 count = (NxI32)(((NxU8*)array[1] - (NxU8*)array[0]) >> 2);
			array[2] = (NxU8*)block + bytes;
			array[1] = block + count;
			array[0] = block;
			}
		}
	void** slot = static_cast<void**>(array[1]);
	*slot = value;
	array[1] = slot + 1;
	}

// The three unit vectors at .data 0x10122054, 0x10122060 and 0x1012206c
// (phys_data_003036, 003039, 003042): (1,0,0), (0,1,0), (0,0,1) in the image.
// The joint rows read them from memory (the prismatic solver slot
// phys_fn_004386 copies them into its angular records, 0xaf065-0xaf217; the
// fixed, spherical and D6 rows multiply by them, 0xa05ab-0xa46bd), so they are
// data, not constants the compiler may fold. Joint-families Task 3a; defined
// in core/JointSupport.cpp.
extern NxVec3 gJointUnitAxis[3];

// The three-word .data triple at 0x10123c1c, zero in a fresh image (the NpActor
// getters' static defaults copy it too; see Physics/src/ObjectModel.cpp "Actor
// slate 7"). The fixed solver slot phys_fn_004246 reads it from memory as the
// second lever of its three linear records and subtracts it from the linear
// error (0xa04d9-0xa04fd, 0xa060d-0xa0659), so it is data here as well.
// Joint-families Task 3h; defined in core/JointSupport.cpp.
extern NxVec3 gJointZeroVector;

#endif
