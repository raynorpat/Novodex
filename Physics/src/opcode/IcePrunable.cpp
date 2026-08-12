/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/*
NovodeX's scene-query prunable base. See IcePrunable.h for why this is NOT in
External/opcode/novodex/.

Reconstructed by P4 Task 2b from twelve census rows, all Phase 4 and all inside
the censused OPCODE span with no OPCODE 1.3 counterpart:

	phys_fn_004874	0x000b54a0	76	Prunable::Prunable
	phys_fn_004876	0x000b54f0	40	SetFlags				[vslot 1]
	phys_fn_004878	0x000b5520	45	ClearFlags				[vslot 2]
	phys_fn_004880	0x000b5550	25	ToggleFlags				[vslot 3]
	phys_fn_004882	0x000b5570	27	SetOrClearFlags			[vslot 4]
	phys_fn_004884	0x000b5590	29	GetWorldAABB
	phys_fn_004886	0x000b55b0	34	UpdateWorldAABB
	phys_fn_004888	0x000b55e0	47	SetPruningType			IcePrunable.cpp:152
	phys_fn_004890	0x000b5610	47	SetPruningSection		IcePrunable.cpp:174
	phys_fn_004892	0x000b5640	39	~Prunable
	phys_fn_004894	0x000b5670	83	GetUpdatedWorldAABB
	phys_fn_004896	0x000b56d0	64	~Prunable				[vslot 0, scalar deleting]

plus the three rows of the unnamed member at +0x0c, also Phase 4 and also inside
the span:

	phys_fn_005297	0x000e7330	23	Prunable0C::Prunable0C
	phys_fn_005299	0x000e7350	7	Prunable0C::~Prunable0C
	phys_fn_005311	0x000e7670	31	Prunable0C::~Prunable0C	[vslot 0, scalar deleting]

TWO THINGS THAT ARE NOT REPRODUCED AND CANNOT BE.

The assert line numbers. The image pushes 0x98 = 152 and 0xae = 174, which are
`__LINE__` in NovodeX's own IcePrunable.cpp. This file is not that file and its
line numbers are its own; padding it out to land the two calls on 152 and 174
would be a coincidence manufactured to look like a measurement. The two numbers
are recorded above and in the correspondence map, and the differential does not
compare them.

The two adapter addresses the constructor installs. The image writes
0x100b5460 and 0x100b5480; this build writes its own two function addresses,
because they are different functions in a different module. What the
differential compares is that the two slots were null before construction and
are not null after it -- which is the whole of what the two writes do that
survives a change of module.
*/

#include "IcePrunable.h"

///////////////////////////////////////////////////////////////////////////////
// The globals. Null here, and null in the image until 0x0002562b runs.

udword	(*gPrunableOwnerQuery)(void* owner)					= 0;
void	(*gPrunableOwnerNotify)(void* owner, AABB* box)		= 0;
void	(*gPrunableOwnerWorldAABB)(void* owner, AABB* box)	= 0;

udword	(*gPrunableAdapterQuery)(Prunable* prunable)			= 0;
void	(*gPrunableAdapterNotify)(Prunable* prunable, AABB* box)	= 0;

///////////////////////////////////////////////////////////////////////////////
// The two adapters. NOT THIS TASK'S ROWS -- 0x000b5460 and 0x000b5480 are below
// the censused span. Written here because the constructor installs them.
//
//	0x000b5460	mov eax,[0x10128470]; test eax,eax; je zero
//				mov ecx,[esp+4]; mov edx,[ecx+4]; mov [esp+4],edx; jmp eax
//		 zero:	xor eax,eax; ret
//	0x000b5480	the same shape on [0x10128474], returning nothing and with a
//				bare `ret` on the null arm.
//
// Both replace their first argument with `argument->+4` and tail-call. On a
// Prunable that member is mOwner, which is what makes them owner adapters.

static udword nxPrunableAdapterQuery(Prunable* prunable)
{
	if(!gPrunableOwnerQuery)	return 0;
	return gPrunableOwnerQuery(prunable->mOwner);
}

static void nxPrunableAdapterNotify(Prunable* prunable, AABB* box)
{
	if(!gPrunableOwnerNotify)	return;
	gPrunableOwnerNotify(prunable->mOwner, box);
}

///////////////////////////////////////////////////////////////////////////////
// The member at +0x0c.

//! 0x000e7330. Installs the vptr and zeroes four dwords. 23 bytes, no purge.
Prunable0C::Prunable0C() : mPrunable(null), mMember14(0), mMember18(0), mMember1C(0)
{
}

//! 0x000e7350. Seven bytes: `mov [ecx], vtable; ret`. It releases nothing --
//! which is what says none of the four dwords is an owning pointer.
Prunable0C::~Prunable0C()
{
}

///////////////////////////////////////////////////////////////////////////////

//! 0x000b54a0, 76 bytes, __thiscall, no purge.
//!
//!		mov [esi+4],0			mOwner
//!		mov [esi],vtable
//!		mov [esi+8],0			mFlags
//!		lea ecx,[esi+0xc]; call 0x000e7330	the member
//!		mov [esi+0x20],0		mPruner
//!		mov [esi+0x2a],0		mPruningType
//!		mov [esi+0x2b],0		mPruningSection
//!		mov [esi+0x24],0xffffffff
//!		mov [esi+0x28],0xffff	mHandle
//!		mov [0x101284fc],0x100b5460
//!		mov [0x10128500],0x100b5480
//!		mov [esi+0x10],esi		mMember0C.mPrunable = this
//!
//! The last write is outside the member's own constructor, so it is a body
//! statement here rather than a member initialiser.
Prunable::Prunable() :
	mOwner				(null),
	mFlags				(0),
	mPruner				(null),
	mPrunable24			(0xffffffff),
	mHandle				(PRUNABLE_INVALID_HANDLE),
	mPruningType		(0),
	mPruningSection		(0)
{
	gPrunableAdapterQuery	= nxPrunableAdapterQuery;
	gPrunableAdapterNotify	= nxPrunableAdapterNotify;

	mMember0C.mPrunable		= this;
}

//! 0x000b5640, 39 bytes. Installs the vptr, then:
//!
//!		if(mHandle != 0xffff && mPruner)	mPruner->[vtable+8](this);
//!		~Prunable0C();		(a tail `jmp 0x000e7350`)
//!
//! It does NOT clear mHandle and does NOT clear mPruner. The guard is the
//! handle first and the pointer second, in that order: 0x000b5643 compares the
//! word before 0x000b5651 loads the pruner.
Prunable::~Prunable()
{
	if(mHandle != PRUNABLE_INVALID_HANDLE && mPruner)
		mPruner->RemoveObject(this);
}

//! 0x000b54f0, 40 bytes, ret 4.
//!
//!		if(flags & mFlags)					return true;
//!		if(flags & 2)						return false;
//!		mFlags |= flags;
//!		return NovodeXSlot5(flags);			(a tail `jmp [vptr+0x14]`)
//!
//! The first test is an intersection, not a subset test: `test edx,eax; je` --
//! ANY bit already set makes the call a no-op that reports success.
bool Prunable::SetFlags(udword flags)
{
	if(flags & mFlags)						return true;
	if(flags & PRUNABLE_FLAG_WORLD_AABB_VALID)	return false;

	mFlags |= flags;
	return NovodeXSlot5(flags);
}

//! 0x000b5520, 45 bytes, ret 4.
//!
//!		if(!(flags & mFlags))				return true;
//!		if(flags & 2)						return false;
//!		mFlags &= ~flags;
//!		return NovodeXSlot5(flags);
//!
//! The first test is the OPPOSITE sense to SetFlags' -- `test eax,edx; jne` --
//! so clearing bits that are already clear is the no-op here.
bool Prunable::ClearFlags(udword flags)
{
	if(!(flags & mFlags))					return true;
	if(flags & PRUNABLE_FLAG_WORLD_AABB_VALID)	return false;

	mFlags &= ~flags;
	return NovodeXSlot5(flags);
}

//! 0x000b5550, 25 bytes, ret 4.
//!
//!		if(flags & 2)						return false;
//!		mFlags ^= flags;
//!		return NovodeXSlot5(flags);
//!
//! No early-out at all: a toggle has no "already in that state" to test.
bool Prunable::ToggleFlags(udword flags)
{
	if(flags & PRUNABLE_FLAG_WORLD_AABB_VALID)	return false;

	mFlags ^= flags;
	return NovodeXSlot5(flags);
}

//! 0x000b5570, 27 bytes, ret 8.
//!
//!		mov al,[esp+8]; mov edx,[esp+4]; test al,al; mov eax,[ecx]; push edx
//!		je clear; call [eax+4]; ret 8
//!	clear:	call [eax+8]; ret 8
//!
//! Both arms are VIRTUAL calls through this object's own vtable, not direct
//! calls to Prunable::SetFlags and Prunable::ClearFlags. A derived class that
//! overrides either one changes what this does, and writing it with unqualified
//! member calls is what keeps that true.
bool Prunable::SetOrClearFlags(udword flags, bool set)
{
	return set ? SetFlags(flags) : ClearFlags(flags);
}

//! 0x000b5590, 29 bytes, no arguments.
//!
//!		if(mHandle == 0xffff)	return null;
//!		return &mPruner->mWorldBoxes[mHandle];
//!
//! `lea eax,[eax+eax*2]; lea eax,[edx+eax*8]` is a stride of 24, which is
//! sizeof(AABB). mPruner is loaded WITHOUT a null test -- the invalid handle is
//! the only guard the row has.
const AABB* Prunable::GetWorldAABB() const
{
	if(mHandle == PRUNABLE_INVALID_HANDLE)	return null;

	return &mPruner->mWorldBoxes[mHandle];
}

//! 0x000b55b0, 34 bytes, ret 4.
//!
//!		if(gPrunableOwnerWorldAABB)	gPrunableOwnerWorldAABB(mOwner, box);
//!		mFlags |= 2;
//!
//! The flag is set whether or not the callback ran: 0x000b55ca is the join of
//! both arms. The two arguments are pushed right to left as
//! `push box; push mOwner; call eax; add esp,8` -- cdecl, and mOwner first.
void Prunable::UpdateWorldAABB(AABB* box)
{
	if(gPrunableOwnerWorldAABB)
		gPrunableOwnerWorldAABB(mOwner, box);

	mFlags |= PRUNABLE_FLAG_WORLD_AABB_VALID;
}

//! 0x000b5670, 83 bytes, no arguments.
//!
//!		if(mHandle == 0xffff)	return null;
//!		if(!(mFlags & 2))
//!		{
//!			if(gPrunableOwnerWorldAABB)
//!				gPrunableOwnerWorldAABB(mOwner, &mPruner->mWorldBoxes[mHandle]);
//!			mFlags |= 2;
//!		}
//!		return &mPruner->mWorldBoxes[mHandle];
//!
//! UpdateWorldAABB is INLINED here, not called: 0x000b5670 has no call to
//! 0x000b55b0 and reaches the global itself at 0x000b568f. Note also that the
//! pruner is loaded at 0x000b5678, BEFORE the handle test at 0x000b567e, so the
//! row dereferences nothing on the invalid path but does read mPruner on it.
const AABB* Prunable::GetUpdatedWorldAABB()
{
	if(mHandle == PRUNABLE_INVALID_HANDLE)	return null;

	if(!(mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
	{
		if(gPrunableOwnerWorldAABB)
			gPrunableOwnerWorldAABB(mOwner, &mPruner->mWorldBoxes[mHandle]);

		mFlags |= PRUNABLE_FLAG_WORLD_AABB_VALID;
	}
	return &mPruner->mWorldBoxes[mHandle];
}

//! 0x000b55e0, 47 bytes, ret 4. IcePrunable.cpp:152 in NovodeX's own tree.
//!
//!		mov eax,[esp+4]; test eax,eax; jl fail; cmp eax,4; jge fail
//!		mov [ecx+0x2a],al; mov al,1; ret 4
//!	fail: push 0x98; push file; push message; call 0x000539b0; add esp,0xc; ret 4
//!
//! The comparison is SIGNED (`jl`, `jge`), so a udword argument with the top bit
//! set is rejected rather than wrapping, and the byte is only written on the
//! accepting path. The failing path returns whatever the reporter returns,
//! which is false.
bool Prunable::SetPruningType(udword type)
{
	if((sdword)type < 0 || (sdword)type >= 4)
		return SetIceError("Invalid pruning type", null);

	mPruningType = (ubyte)type;
	return true;
}

//! 0x000b5610, 47 bytes, ret 4. IcePrunable.cpp:174. The same shape with a
//! bound of 3 and its own message.
bool Prunable::SetPruningSection(udword section)
{
	if((sdword)section < 0 || (sdword)section >= 3)
		return SetIceError("Invalid pruning section", null);

	mPruningSection = (ubyte)section;
	return true;
}
