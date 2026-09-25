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
// 004389/004393 decision notes), and deferred stubs for rows owned by units
// outside the pilot (000022, 000571, 000633, 000758) so pilot rows can call
// them through a named function. Every name below is by offset or row ID:
// the listing establishes these fields, not their meaning.

#include "Nxp.h"
#include "NxVec3.h"

#include <cstddef>

// What the record's two pointers (+0x10/+0x14) point at. phys_fn_004389
// multiplies +0x00..+0x08 with the record's +0x00 vector and +0x10..+0x18
// with the record's +0x18/+0x24 vectors (or, on the bit-10 path, with +0x00).
struct JointSupportBody
	{
	NxVec3				mUnknown000;
	NxReal				mUnknown00c;
	NxVec3				mUnknown010;
	};

static_assert(offsetof(JointSupportBody, mUnknown010) == 0x10, "second vector at +0x10");

// The record phys_fn_004389/004391/004393 run on (`this` in ecx). Size
// unknown; only the offsets these rows touch are declared.
struct JointSupportRecord
	{
	NxVec3				mUnknown000;	//!< +0x00
	NxU32				mFlags;			//!< +0x0c; bit 10 selects 004389's second form
	JointSupportBody*	mBody[2];		//!< +0x10 / +0x14; either may be null
	NxVec3				mUnknown018;	//!< +0x18; paired with mBody[0]
	NxVec3				mUnknown024;	//!< +0x24; paired with mBody[1]
	NxU8				mUnknown030[0xc];
	NxReal				mUnknown03c;	//!< +0x3c; written by 004393
	NxReal				mUnknown040;	//!< +0x40; written by 004393

	//! phys_fn_004389 (0x000af2d0, 227 B). `this` in ecx, no stack
	//! arguments, plain `ret`: a thiscall member with no arguments. The
	//! result is left unrounded in st(0), so it is declared NxF64; the
	//! caller rounds where it stores.
	NxF64				row004389() const;

	//! phys_fn_004391 (0x000af3c0, 837 B; deferred: solver slots 6/7).
	//! Thiscall, two pointer arguments, `ret 8`; 004393 passes two locals
	//! and reads only the second afterwards.
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

// phys_fn_000022 (deferred: owner gap <start>..Actor.cpp).
void row000022();

// phys_fn_000571 (deferred: owner Scene.cpp). Thiscall on the Scene, one
// stack argument (the joint), `ret 4`. MSVC rejects __thiscall on a free
// function (C3865), so the row is a member of a non-virtual fixture whose
// pointer bits stand in for the Scene until a later task gives it a real
// receiver type (the ObjectModel.h row004165/3413 convention).
struct Row000571Fixture
	{
	void row000571(void* joint);
	};

// phys_fn_000633 (deferred: owner Scene.cpp, joint removal). Thiscall on
// the Scene (`mov ecx,[joint+0x30]; push joint`), `ret 4`; same fixture
// convention as 000571.
struct Row000633Fixture
	{
	void row000633(void* joint);
	};

// phys_fn_000758 (deferred: owner gap SceneRaycast..CapsuleShape).
void row000758();

#endif
