# Phase 5 public-DLL actor lifecycle drive — 2026-09-24

`NxPhysicsActorLifecycleTests` loads the pinned oracle or rebuilt DLL together
with its Foundation dependency through `PhysicsPairLoader.h`. The Win32 Release
probe creates a scene, then a static box actor and a dynamic box actor with
nonidentity translations. The staged-pair runner compares the whole transcript
and checks that each loaded module came from the selected pair directory.

Before the fixes, the candidate returned `isDynamic()=1` for the static actor,
while the oracle returned 0. The oracle's dynamic-actor vtable slot 19 points to
`phys_fn_000110` at `0x00003580`: it tests the pointer at actor+0x14, then
whether the word at body+8 is non-null. The candidate's literal `true` was
replaced with that read. A deliberate return-to-`true` mutation changed the
staged differential from `stdout_delta=0` to `stdout_delta=2`; the oracle's
static line remained `actor static dynamic=0`, while the candidate printed 1.

The oracle's slot 6 points to `phys_fn_000092` at `0x00002ed0`. It copies three
translation words from the nested object at body+8, offsets +0x50/+0x54/+0x58,
or from the outer body at +0x44/+0x48/+0x4c when the nested pointer is null.
The candidate's former default-return stub produced unstable words. The new
implementation matches the oracle's two public results exactly:

| actor | oracle and candidate position words |
| --- | --- |
| static | `40000000.bf800000.40800000` |
| dynamic | `c0400000.40000000.3f800000` |

Zeroing the returned X component after the copy yielded `stdout_delta=4` in
the staged differential: both the static and dynamic position lines changed
on the candidate side. Restoring the code returned `stdout_delta=0`.

The same page-guarded allocator exposed an independent object-layout gap.
Both oracle actors point at a distinct 0x50-byte outer body allocation through
actor+0x14. The static body's +8 is null, and the dynamic body's +8 points to
a 0x260-byte nested allocation. Before this packet the candidate static actor
had no outer body and the dynamic actor used a 0x1c0-byte placeholder. The
static holder now allocates 0x50 bytes, keeps +8 null, and copies the fallback
translation at +0x44; the registered staged-pair test checks all three facts.
The dynamic allocation graph is **still incomplete**. Its public position and
`isDynamic` results agree, but the candidate's dynamic allocation sizes and
the joint-descriptor path remain separate work; these passing values do not
prove the dynamic object layout.

The Phase 5 gate evaluates 135 of 135 registered coverage assertions, including
nine lines from the new staged-pair target. It still exits 1 on the pre-existing
`CANDIDATE-MISSING family=vtables` marker for final shape/actor tables. Phase 6
continues to pass after the body-pointer change.
