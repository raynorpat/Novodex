/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/*
NovodeX's scene-query prunable base.

THIS IS NOT OPCODE. `grep -rn Prunable` over both pinned OPCODE 1.3 trees
returns nothing: no IcePrunable.h and no IcePrunable.cpp ships in either. The
file is NovodeX's own and lives under `src\opcode\` by convention only, which is
what the oracle's own assert literal says --
`\Epic\Novodex\SDKs\Physics\src\opcode\IcePrunable.cpp` at `.rdata:0x0011b5d4`.
It is therefore reconstructed here rather than vendored: nothing under
External/opcode/novodex/ may contain it, because that directory answers the
question "what did NovodeX change in OPCODE?" and the answer must not grow
files OPCODE never had.

Reconstructed by P4 Task 2b from twelve census rows, `0x000b54a0`-`0x000b5710`.
Every member offset below is read off an instruction, and the instruction is
named beside it.
*/

#ifndef NX_PHYSICS_ICEPRUNABLE_H
#define NX_PHYSICS_ICEPRUNABLE_H

#include "Opcode.h"

using namespace IceCore;
using namespace IceMaths;

class Prunable;

///////////////////////////////////////////////////////////////////////////////
// The three NovodeX globals Prunable reads.
//
// All three are written exactly once in the whole image, at `0x0002562b`,
// `0x00025635` and `0x0002563f` -- one row outside both library spans and
// outside this task's population. They are declared here, and left null here,
// because Prunable reads two of them and null is what they hold until that row
// runs. `0x000b55b0` and `0x000b568f` both test for null before dispatching,
// which is what says null is a state the shipped code expects.

//! .data:0x00128470. The shipped value is the folded `xor eax,eax; ret` stub at
//! 0x000213e0. Read only by the adapter at 0x000b5460.
extern udword	(*gPrunableOwnerQuery)(void* owner);

//! .data:0x00128474. The shipped value is 0x00025520, which dispatches
//! `owner->vtable[+0x28](box)`. Read only by the adapter at 0x000b5480.
extern void		(*gPrunableOwnerNotify)(void* owner, AABB* box);

//! .data:0x00128478. The shipped value is 0x00025510, which dispatches
//! `owner->vtable[+0x24](box)` -- the owner recomputing its world box.
//! Read by Prunable::UpdateWorldAABB (0x000b55b0) and by
//! Prunable::GetUpdatedWorldAABB (0x000b568f).
extern void		(*gPrunableOwnerWorldAABB)(void* owner, AABB* box);

///////////////////////////////////////////////////////////////////////////////
// The two slots Prunable::Prunable installs, .data:0x001284fc and
// .data:0x00128500, at 0x000b54d0 and 0x000b54da.
//
// NOT THIS TASK'S ROWS. The two adapters themselves are 0x000b5460
// (phys_fn_004870, 25 bytes) and 0x000b5480 (phys_fn_004872, 23 bytes), which
// sit BELOW the censused OPCODE span and are outside P4 Task 2b's population.
// They are written here only because the constructor installs them and a
// constructor that installs nothing is a different constructor. Nothing claims
// them as recovered.
extern udword	(*gPrunableAdapterQuery)(Prunable* prunable);
extern void		(*gPrunableAdapterNotify)(Prunable* prunable, AABB* box);

///////////////////////////////////////////////////////////////////////////////
//! The pruner Prunable points at.
//!
//! NOT RECONSTRUCTED. The pruner family is 61 census rows behind five nine-slot
//! vtables (`.rdata:0x0011bc18` base, `0x0011b9c8`, `0x0011b9f0`, `0x0011bb98`,
//! `0x0011bbc0`) and P4 Task 2b did not reach it. Declared here is exactly what
//! Prunable reaches into it and nothing else:
//!
//!   * vtable slot 2, called with the prunable, by ~Prunable at 0x000b565b
//!     (`mov eax,[ecx]; push esi; call [eax+8]`) and by the deleting destructor
//!     at 0x000b56eb;
//!   * the world-box array at +0x14, read by GetWorldAABB at 0x000b55a0
//!     (`mov edx,[ecx+0x14]`) and by GetUpdatedWorldAABB at 0x000b5699
//!     (`mov edx,[edi+0x10]` with `edi = mPruner + 4`).
//!
//! The four dwords between the vptr and the array are named for their offsets
//! because nothing this task disassembled writes them.
class Pruner
{
	public:
	virtual						~Pruner()										{}
	virtual	void				NovodeXPrunerSlot1()							= 0;
	//! Slot 2. ~Prunable calls it when the handle is valid.
	virtual	void				RemoveObject(Prunable* object)					= 0;
	// Slots 3-8 exist in the image and are not declared: nothing in Prunable
	// dispatches through them, and declaring a slot nobody measured would be an
	// invention with a vtable index attached to it.

	protected:
			udword				mPruner04;			//!< +0x04, unidentified
			udword				mPruner08;			//!< +0x08, unidentified
			udword				mPruner0C;			//!< +0x0c, unidentified
			udword				mPruner10;			//!< +0x10, unidentified
	public:
			AABB*				mWorldBoxes;		//!< +0x14. 0x000b55a0
};

///////////////////////////////////////////////////////////////////////////////
//! The unnamed 20-byte polymorphic member at Prunable+0x0c.
//!
//! Named for its offset, not guessed at. It is constructed at 0x000e7330 and
//! destroyed at 0x000e7350, both reached from Prunable and from nowhere else in
//! the image; its vtable `.rdata:0x0011ba1c` has exactly ONE slot, the scalar
//! deleting destructor at 0x000e7670, and that slot is the only virtual the
//! class has. Its four dwords are zeroed by its own constructor and Prunable
//! then writes itself into the first of them (0x000b54e4,
//! `mov [esi+0x10], esi`), which is the only thing anything ever writes into
//! the object. Nothing else in the image touches it, so nothing establishes a
//! name for it or for its remaining three dwords.
class Prunable0C
{
	public:
								Prunable0C();
	virtual						~Prunable0C();

			Prunable*			mPrunable;			//!< +0x10 abs. 0x000b54e4
			udword				mMember14;			//!< +0x14 abs, unidentified
			udword				mMember18;			//!< +0x18 abs, unidentified
			udword				mMember1C;			//!< +0x1c abs, unidentified
};

///////////////////////////////////////////////////////////////////////////////

//! The invalid handle. 0x000b5594 `cmp ax, 0xffff`, and the constructor writes
//! it at 0x000b54ca.
#define PRUNABLE_INVALID_HANDLE		0xffff

//! The one flag bit Prunable itself understands. Every one of the four flag
//! members refuses an argument carrying it -- 0x000b5500, 0x000b5530,
//! 0x000b5554 are all `test <arg>, 2` -- and UpdateWorldAABB is the only thing
//! that sets it, at 0x000b55ca and 0x000b56af (`or [esi+8], 2`).
#define PRUNABLE_FLAG_WORLD_AABB_VALID	2

class Prunable
{
	public:
								Prunable();
	virtual						~Prunable();

	// The flag members, vtable `.rdata:0x0011b5a4` slots 1-4. All four are
	// virtual in the image: 0x000b5570 reaches the first two through
	// `call [eax+4]` and `call [eax+8]` rather than by direct call.

	//! Slot 1, 0x000b54f0.
	virtual	bool				SetFlags(udword flags);
	//! Slot 2, 0x000b5520.
	virtual	bool				ClearFlags(udword flags);
	//! Slot 3, 0x000b5550.
	virtual	bool				ToggleFlags(udword flags);
	//! Slot 4, 0x000b5570.
	virtual	bool				SetOrClearFlags(udword flags, bool set);

	//! Slot 5. The shipped body is the five-byte `mov al,1; ret 4` at
	//! 0x0000dee0 -- which is NOT an IcePrunable.cpp row: /OPT:ICF folded it
	//! with phys_fn_000446, a Phase 2 row that was closed there. What this
	//! declaration establishes is the SLOT: the three mutating flag members all
	//! tail-call `[vptr+0x14]` with the same argument they were given
	//! (0x000b5515, 0x000b554a, 0x000b5566) and return its result.
	virtual	bool				NovodeXSlot5(udword /*flags*/)					{ return true; }

	//! 0x000b5590. Not virtual: every call site is a direct `call`.
			const AABB*			GetWorldAABB()			const;
	//! 0x000b55b0. Not virtual.
			void				UpdateWorldAABB(AABB* box);
	//! 0x000b5670. Not virtual.
			const AABB*			GetUpdatedWorldAABB();

	//! 0x000b55e0, IcePrunable.cpp:152.
			bool				SetPruningType(udword type);
	//! 0x000b5610, IcePrunable.cpp:174.
			bool				SetPruningSection(udword section);

	//! 0x000b56fd-0x000b5707. The deleting destructor's free arm calls the
	//! allocator getter at 0x000b4000 and dispatches `[vtable+0x0c]` -- the SDK
	//! allocator, not the CRT. A class operator delete is how a
	//! compiler-generated scalar deleting destructor reaches it.
			void				operator delete(void* memory)	{ opcNovodeXFree(memory); }

	inline_	udword				GetFlags()				const	{ return mFlags;			}
	inline_	uword				GetHandle()				const	{ return mHandle;			}
	inline_	Pruner*				GetPruner()				const	{ return mPruner;			}
	inline_	ubyte				GetPruningType()		const	{ return mPruningType;		}
	inline_	ubyte				GetPruningSection()		const	{ return mPruningSection;	}

	// The layout, offset by offset, each with the instruction that fixes it.
			void*				mOwner;				//!< +0x04. 0x000b55c0 `mov edx,[esi+4]`
			udword				mFlags;				//!< +0x08. 0x000b54f0 `mov eax,[ecx+8]`
			Prunable0C			mMember0C;			//!< +0x0c. 0x000b54a9 `lea ecx,[esi+0xc]`
			Pruner*				mPruner;			//!< +0x20. 0x000b559d `mov ecx,[ecx+0x20]`
			udword				mPrunable24;		//!< +0x24, unidentified. 0x000b54c3 writes 0xffffffff
			uword				mHandle;			//!< +0x28. 0x000b5590 `mov ax,[ecx+0x28]`
			ubyte				mPruningType;		//!< +0x2a. 0x000b55ed `mov [ecx+0x2a],al`
			ubyte				mPruningSection;	//!< +0x2b. 0x000b561d `mov [ecx+0x2b],al`
};

#endif // NX_PHYSICS_ICEPRUNABLE_H
