#ifndef NX_PHYSICS_BODYSTEP
#define NX_PHYSICS_BODYSTEP
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// Declarations for Physics/src/Island.cpp (the body record's island and
// sleep-group rows) and Physics/src/BodyStep.cpp (the per-body step math and
// the continuous-collision pose rows), scene-raycast Task 4. Every row here
// runs on the dynamic body record (the record whose +0x1bc is the joint-island
// parent and +0x1e8 the sleep-group parent; see core/JointSupport.h and the
// scene-raycast contract's rows 000708-000774), and every offset is taken from
// the listing: the listing establishes these fields, not their meaning.
//
// As in core/JointSupport.h, each thiscall row is a member of a non-virtual
// fixture struct whose pointer bits stand in for the record it runs on (MSVC
// rejects __thiscall on a free function, C3865). None is ever constructed.
//
// Who calls them: the simulation step (NxScene::simulate -> 002400 -> 000659
// -> 000655 and its phases 000608/000610/000611/000613/000615/000619/000635/
// 000636), which the candidate does not have (NpScene::simulate is a stub),
// and, for 000772/000774, the continuous-collision sweep 002264, which the
// candidate does not have either. So nothing in the candidate calls these
// rows yet; they are kept by /OPT:NOREF like core/JointSupport.cpp's
// Row000738Fixture.

#include "Nxp.h"
#include "NxVec3.h"

// ---------------------------------------------------------------------------
// Island.cpp

// Row 000708 (0x00015c20, 130 B). Thiscall, no stack arguments, plain
// `ret`: copies the step's JointSupportBody (+0x204) back into the record.
struct Row000708Fixture
	{
	void row000708();
	};

// Row 000714 (0x00015d70, 328 B). Thiscall, one float, `ret 4`: the
// per-body wake-counter update.
struct Row000714Fixture
	{
	void row000714(NxReal dt);
	};

// Row 000716 (0x00015ec0, 277 B). Thiscall, one pointer, `ret 4`:
// joint-island union by rank (+0x1bc parent, +0x1c0 rank).
struct Row000716Fixture
	{
	void row000716(void* other);
	};

// Row 000718 (0x00015fe0, 285 B). Thiscall, one pointer, `ret 4`:
// sleep-group union by rank (+0x1e8 parent, +0x1ec rank).
struct Row000718Fixture
	{
	void row000718(void* other);
	};

// Row 000720 (0x00016100, 46 B). Thiscall, no stack arguments, plain
// `ret`: hands a dirty island root to 004172.
struct Row000720Fixture
	{
	void row000720();
	};

// Row 000724 (0x000161b0, 175 B). Thiscall, two pointers, `ret 8`:
// links a contact pair into the sleep-group graph.
struct Row000724Fixture
	{
	void row000724(void* other, void* pair);
	};

// Row 000728 (0x000167c0, 108 B). Thiscall, two stack words, `ret 8`:
// prepares the constraints of one body's joints and contact pairs. The two
// words are the Scene's +0x548 / +0x54c (the step's dt and its inverse:
// 000772 multiplies by +0x548, 000726 scales a displacement by +0x54c into a
// velocity), passed on as they are.
struct Row000728Fixture
	{
	void row000728(NxReal dt, NxReal invDt);
	};

// Row 000730 (0x00016830, 42 B). Thiscall (this may be null), two
// stack words, `ret 8`: 000728 over the sleep-group chain (+0x1fc).
struct Row000730Fixture
	{
	void row000730(NxReal dt, NxReal invDt);
	};

// Row 000762 (0x000177c0, 357 B). Thiscall, one pointer, `ret 4`:
// links a joint into the joint-island graph.
struct Row000762Fixture
	{
	void row000762(void* joint);
	};

// Row 000764 (0x00017930, 100 B). The record in ecx, no stack
// arguments, plain `ret` (fastcall on one argument and thiscall with none
// are the same ABI): releases a dirty island root's island object, then
// tail-jumps to 000722.
struct Row000764Fixture
	{
	void row000764();
	};

// ---------------------------------------------------------------------------
// BodyStep.cpp

// Row 000710 (0x00015cb0, 122 B). Thiscall, one pointer, `ret 4`:
// gravity (or zero) into +0x88, the accumulators +0x94..+0xb4 cleared.
struct Row000710Fixture
	{
	void row000710(const NxVec3* gravity);
	};

// Row 000726 (0x00016260, 1371 B). Thiscall, two floats, `ret 8`: the
// per-body velocity update (kinematic target or dynamic integration).
struct Row000726Fixture
	{
	void row000726(NxReal dt, NxReal invDt);
	};

// Row 000732 (0x00016860, 399 B). Thiscall, two floats (the second
// unread), `ret 8`: the per-body post-step velocity bookkeeping.
struct Row000732Fixture
	{
	void row000732(NxReal dt, NxReal unused);
	};

// Row 000734 (0x000169f0, 101 B). Thiscall, one float, `ret 4`:
// +0x158 += dt * the saved linear velocity +0x1a0.
struct Row000734Fixture
	{
	void row000734(NxReal dt);
	};

// Row 000736 (0x00016a60, 406 B). Thiscall, a quaternion pointer and a
// float, `ret 8`, eax 1 or 0: integrates the quaternion by the saved angular
// velocity +0x1ac.
struct Row000736Fixture
	{
	NxU32 row000736(NxReal* q, NxReal dt);
	};

// Row 000740 (0x00016c20, 423 B). Thiscall, one float, `ret 4`: the
// swept bounds +0x244..+0x258 that 000738 hands out.
struct Row000740Fixture
	{
	void row000740(NxReal dt);
	};

// Row 000770 (0x000183a0, 205 B). Thiscall, two stack words (the
// second unread), `ret 8`: saves the continuous-collision start pose and
// advances the body.
struct Row000770Fixture
	{
	void row000770(NxReal dt, NxReal unused);
	};

// Row 000772 (0x00018470, 207 B). Thiscall, one float, `ret 4`,
// result in al: restores the saved pose and re-advances by the time of
// impact when it is earlier than the one recorded.
struct Row000772Fixture
	{
	bool row000772(NxReal toi);
	};

// Row 000774 (0x00018540, 34 B). Thiscall, one float, `ret 4`:
// 000772, then the owning actor body's pose notification 000022.
struct Row000774Fixture
	{
	void row000774(NxReal toi);
	};

// The object at the actor body's +0x10 (the actor body is the record's
// +0x19c; phys_fn_000022 tail-calls slot 6 of the same object). 000740 calls
// slot 10 (`call [edx+0x28]`, 0x16c79) with a pointer to four floats it
// reads back as a centre and a radius. Declared as an interface so the call
// is a real virtual dispatch; nothing implements it here.
struct Row000740Target
	{
	virtual void slot0() = 0;
	virtual void slot1() = 0;
	virtual void slot2() = 0;
	virtual void slot3() = 0;
	virtual void slot4() = 0;
	virtual void slot5() = 0;
	virtual void slot6() = 0;
	virtual void slot7() = 0;
	virtual void slot8() = 0;
	virtual void slot9() = 0;
	virtual void slot10(NxReal* sphere) = 0;
	};

// ---------------------------------------------------------------------------
// Callees outside this sub-area. 000722 is core/JointSupport.h's
// Row000722Fixture::row000722 (core/JointSupport.cpp; 000764 tail-jumps to
// it) and 000897 is ContactPairManager.h's NxActorPair::row000897 (000728
// calls it on pair node + 0x14). 004172 has no candidate anywhere: Island.cpp
// defines an open stand-in (NX_ASSERT(0), no stable-ID line), below.

// Row 004172 (0x0009b0d0, 69 B): cdecl on an island root; clears
// +0x1e4 bit 1 and rebuilds the root's island object at +0x1e0 through
// 0x9adb0. Owner gap Joint.cpp..D6Joint.cpp (000720 and 000764 call it).
// OPEN: not reconstructed; the stand-in does nothing in Release.
void __cdecl nxBodyIslandRebuild004172Open(void* root);

#endif
