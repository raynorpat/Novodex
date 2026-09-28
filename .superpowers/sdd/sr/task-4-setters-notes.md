# Task 4, sub-area setters: notes

Branch `claude/sr-t4-setters` from `claude/scene-raycast-block` at 6bd2529. Written in the isolated worktree
`agent-a1d9a00831d6930db`. Not touched: inventory.json, the ledgers, the contract, gate_targets.ps1 and the
timing table. The integrator records these results from the notes below.

## Where the rows live

| Row | B | Candidate (stable-ID line) | ABI |
|---|---:|---|---|
| 000782 | 3428 | `DynamicBody::addForce(const NxVec3* force, const NxVec3* torque, NxU32 mode, bool wake)`, Physics/src/BodyCreation.cpp | thiscall, `ret 0x10`, as the row |
| 000784 | 368 | `DynamicBody::setKinematicTarget(const NxVec3*, const NxQuat*)`, BodyCreation.cpp | thiscall, `ret 8` |
| 000785+000787 | 1325+428 | `DynamicBody::setKinematic(NxU32 enable)`, BodyCreation.cpp (one function with both lines) | thiscall, `ret 4`; the argument is tested as a dword (`test eax,eax`, 0x1962c) |
| 000789 | 1461 | `DynamicBody::setPoseFromCMass()`, BodyCreation.cpp | thiscall, plain `ret` |
| 000791 | 133 | `Row000791Fixture::row000791(const NxVec3&, const NxVec3&, NxU32 word3, NxU32 word4)`, Physics/src/core/JointSupport.cpp | thiscall, `ret 0x10` |

All members are `__declspec(noinline)`: the image calls each as its own function.

Why BodyCreation.cpp: the rows are Body:: methods of the 0x260-byte record that `DynamicBody` already models,
and that unit already has `bodyMarkDirty`, the image's inline dirty-mark sequence. The rows run at API time
(0x027f), which is that unit's architecture (default, doubles at 53 bits). NpActor.cpp keeps only small
forwarders, which shrinks the NpActor-session merge hazard:
- `nxNpActorAccumulateForce(record, value, mode, angular)` becomes
  `addForce(angular ? 0 : &value, angular ? &value : 0, mode, true)`. The NpActor-unit callers 000056, 000058,
  000160 and 000162 push exactly that (for example 0x100027fa-0x10002800: `push 1; push mode; push 0; push force`).
- `nxNpActorForceAtPos` becomes `row000791(force, pos, mode, 1)`. The callers 000054 and 000154-000158 push wake 1.
- `nxNpActorTransitionKinematic` becomes `setKinematic(enable ? 1 : 0)`. Its callers are 000188/000190.
- `nxNpActorApplyWorldMassPose` becomes `setPoseFromCMass()`. Its callers are 000204, 000206 and 000208.

No NpActor-unit row body or call site is changed. NpActor.cpp gains two includes, `BodyCreation.h` and
`core/JointSupport.h`. 000795 (`DynamicBody::loadFromBodyDesc`) now calls `setKinematic((flags >> 7) & 1)`
directly, as 0x1b49c-0x1b4a8 does. Its forward declaration of `nxNpActorTransitionKinematic` is gone.

000791 uses main's placeholder name and signature (`git show main:Physics/src/core/JointSupport.cpp`, line 502).
It is placed as on main, just before 000760. The header struct is main's, with its comment's first line reworded
to `// Row 000791 ...` (the marker rule). The definition gains `__declspec(noinline)`. Main's
SpringAndDamperEffector (003979) calls it with (applied, point, 1, 0). With this branch merged, that stub
becomes the faithful row.

## Listing walk (Capstone, authoritative)

- **000782**, 0x18730-0x19491, all of it.
  - Mode above 4 goes to the wake block (`cmp eax,4; ja 0x1936f`). Otherwise the jump table 0x10019494
    {0x1874d, 0x189fd, 0x18c87, 0x18ec3, 0x19138}.
  - Every arm tests each pointer (`test eax,eax; je`) before using it.
  - Mode 0 (0x18759-0x1879b) and mode 3 (0x18ecf-0x18f11), linear: the inverse mass stays in a register.
    The x product stays unrounded; the y and z products are spilled to float; then each is added to the field.
  - Modes 0 and 3, angular (0x1888b-0x1890d, 0x1900b-0x1908d): each row sums
    (I[r][2] tz + I[r][1] ty) + I[r][0] tx. x and y stay in registers; z is spilled.
  - Mode 1 (0x18a09-0x18a55, 0x18b44-0x18bd9): all three linear products are spilled. On the angular side,
    x stays in the register and y and z are spilled. The sums go to +0x6c/+0x78, and their copies go to
    +0x34/+0x40 (the x word from the register, y and z as word copies).
  - Mode 2 (0x18c93, 0x18daa) and mode 4 (0x19144, 0x19260) add the vectors unscaled.
  - Dirty marks: 0x20/0x40, 4/8, 4/8, 0x80/0x100, 0x80/0x100. The shared tails (0x18c82 `jmp 0x18e78`,
    0x19368) were followed.
  - Wake (0x1936f-0x19488) when the low byte of `wake` is set: +0x114 bit 8 clear and an ordered
    +0x84 < [0x101053d4] (= 0x3ecccccc, read from the oracle image) lead to +0x84 = +0x4c = 0x3ecccccc, mark 0x10.
- **000784**, 0x194b0-0x1961d, all of it.
  - The position goes to [+0x118]+0..8 and ORs 1 into [+0x118]+0xc (0x194d5).
  - The quaternion goes to +0x10..0x1c and ORs 2 (0x19506).
  - Each is gated by a null test of its own pointer; the block pointer is never tested. The wake block
    runs unconditionally.
- **000785+000787**, 0x19620-0x19cf9, all of it.
  - Both paths test the bit first (`test al,al; js/jns`), then run the island step that 000748 inlines
    (0x19643-0x1966f, 0x19992-0x199be; the 000712 call is at 0x1964d/0x1999c).
  - Enter: +0xc0 = 0, mark 0x10000; +0xc4..cc = 0, mark 0x20000; +0x10c |= 0x80, mark 0x80000. The block
    is taken from [0x101041bc] when +0x118 is null (0x1995d), and its +0xc = 0.
  - Leave: +0x10c &= ~0x80, mark 0x80000; +0xc0 = 1.0f/+0x188 (0x19abb), mark 0x10000. Three unconditional
    divisions follow (0x19bb6-0x19bce), with z spilled (0x19bd4) and stores in the order x, z, y; then
    mark 0x20000 and a free through [0x101041bc] (0x19cda), then null.
  - Constants read from the oracle image: [0x101041ec] = 1.0f, [0x101041f0] = 0.0f, [0x101043cc] = 0.5f.
- **000789**, 0x19d00-0x1a2b4, all of it.
  - 000746 is called first (0x19d1e) on (+0xc4, +0x134, +0x164).
  - M = C F^T into a float local, each element in the listing's sum order (0x19d23-0x19e5a).
  - Displacement: the x row stays in the register; the y and z rows are spilled (0x19e5e-0x19ec8).
    Then t = centre - d; +0x50..58 and +0x18..20; mark 1.
  - Quaternion (0x19fe1-0x1a1ce): the same arms as the pose setters' conversion. The (m22 + m11) spill
    is at 0x19ff3. The z arm spills s (0x1a0a7); the y and x arms spill 0.5/s (0x1a114, 0x1a16c).
  - Store order: +0x64 (from the register), +0x68, +0x5c, +0x60, then the +0x24..30 copies; mark 2.
- **000791**, 0x1a2c0-0x1a342, all of it.
  - dx stays in the register; dy and dz are spilled.
  - The torque is formed x, y, z (each `fmul; fmul; fsubp`) into a float local. One call to 000782 passes
    (force, &torque, mode, wake) through unchanged.
  - Checked in the built JointSupport.obj: the same dx/dy/dz roundings and a single `call addForce`.

The Ghidra decompiles were not needed; the listing is unambiguous for all five rows.

## Reproduced / divergences

Reproduced: everything above, including the x87 groupings, the spills and the store orders. The roots use
X87Sqrt.h: `x87FsqrtSum4` for the trace arm, `x87FsqrtDiag` for the z and y arms, and `x87FsqrtSum3` over the
float spill for the x arm. No CRT math remains in these rows. 000789 calls `nxNpActorWorldTensorRDRt` (000746)
with its listing's spill pattern. The kinematic block uses the Foundation allocator.

Defects of the old emulations that the rows fix:
- 000782: mode 0/1/3 x87 grouping; wake argument ignored (0x1936f); mode > 4 skipped the wake (0x18740).
- 000784: its wake block and OR 1 / OR 2 were missing from the inline emulation. That emulation stays at the
  NpActor call sites (see Handover below).
- 000785/000787: island step missing; inverses were guarded where the image divides unconditionally.
- 000789: wrong tensor helper; displacement grouping and x-row rounding; wrong quaternion helper with CRT
  sqrt (`nxNpActorQuaternionFromMatrix` and the local y-arm patch with `sqrt`).
- 000791: lever.x was rounded to float; force and torque went through two 000782 calls.

Recorded differences that remain (none reachable by a target):
- The dirty-mark queue growth goes through `nxGetSdkAllocator()` where the image uses [0x101041bc], and the
  `id < 256` test is not in the image (`bodyMarkDirty`; Task 2's deferred whole-lifetime change).
- 000791 passes `(NxU8)word4 != 0` as the wake bool where the image passes the word through. 000782 reads only
  its low byte, so the effect is the same.
- The in-step callers of 000782 (003601, and 003979 through 000791) are not reproduced. If they come to run
  under the step's 0x0f7f, 000782 (BodyCreation.cpp, SSE2) would need x87 codegen for 64-bit intermediates.
  000791 is already in an /arch:IA32 unit.

`nxNpActorQuaternionFromMatrix` (NpActorDynamicMath.h, CRT sqrt) no longer has a user. It was left in place;
the header was not edited.

## Dynamic path (cdb, hand-staged pairs `D:\FlamingEnt__\novodex-analysis\pairs-setters`)

Candidate NxPhysics.dll sha256 85d9012bc3520f9c050030f89e76ebe3e03040c748e995964d3b33dd7d17c565. Oracle
4b7db3e1... . Breakpoints were set on the oracle rows' RVAs and on the candidate's map symbols (addForce 0x1120,
setKinematic 0x4d80, setKinematicTarget 0x5480, setPoseFromCMass 0x5600, row000791 0x3df80; 000746 context at
0x4b50 BodyCreation.obj and 0x17170 NpActor.obj). Hit counts per side (oracle = candidate unless noted):

| Target | 000782 | 000785 | 000789 | 000791 | 000784 | 000746x |
|---|---:|---:|---:|---:|---:|---:|
| ActorForce | 26 | 13 | | 6 | | 11 |
| ActorMomentum | | 5 | | | | 17 vs 3 (000140/000142, NpActor handover) |
| ActorDynamics | | 10 | | | 5 vs 0 (not wired) | 2 |
| ActorDynamicSetter | | 6 | | | | 15 |
| ActorCMass | | 10 | 3 | | | 90 |
| ActorBodyFlag | | 1 | | | | 1 |
| ActorLifecycle | | 6 | | | | 3 |
| BodyCreation (with the new cases) | 1 | 9 | | | | 7 |
| JointTests / JointStagedPair | | 8 | | | | 5 |
| JointSlot | | 27 | | | | 55 |
| JointAllocator | | 2 | | | | 2 |
| DynamicFirst | | 1 | | | | 1 |

- 000787 (0x19b50, reached only by the queue-growth `jmp`) had 0 hits on the oracle side.
- CMass's 000746 count now matches: the 3 hits that 000789 adds were among the 17 extra hits the handover
  recorded.

The trace logs were not committed (scratchpad). If the integrator promotes, the cdb excerpt still has to be
committed as `evidence/scene-raycast-trace-task4-setters.txt`, per the method.

## Staged-target results (hand-staged, run_differential.ps1's normalization)

Every target has stdout_delta=0, stderr_exact, exits 0/0: ActorForce (39 lines), ActorMomentum (54),
ActorDynamics (21), ActorDynamicSetter (133), ActorCMass (569), ActorBodyFlag (10), ActorLifecycle (312),
BodyCreation (162 = 149 + 13 new), JointTests (3104), JointSlot (1475), JointStagedPair (3104),
JointAllocator (23), DynamicFirst (5). NxPhysicsJointDescTests is oracle-only (Phase 6 usage differs), so it was
not run as a pair. The tool tests pass (753) and the validator reports unexplained=0 (inventory unchanged).
Stable-ID check: 6 new lines in exact form, RVA and size equal to the inventory, no duplicates.

Powershell was refused in this sandbox, so run_phase_gate.ps1 was not run. Gates 2-7 must be run by the
integrator.

## New cases (NxPhysicsBodyCreationTests, new lines only)

Before the final releases:
- raiseBodyFlag/clearBodyFlag(NX_BF_KINEMATIC) on the zero-element tensor body `d`. This checks the inverse
  zeroing, the block allocation and free, and the unconditional 1/0 = inf.
- A body created with wakeUpCounter 0.125, then `addForce(v, (NxForceMode)5)`. This checks that mode > 4
  still wakes.

The old candidate would have printed `...0.3f000000` (inverse 0) and `3e000000.3e000000`. Proposed registered
lines (oracle-sourced, verbatim; the Phase 5 floor would go 1020 -> 1033):

```
bodycreate setters_enter_traffic=1.0 a.260 f
bodycreate setters_enter_inverses=0.0.0.0
bodycreate setters_enter_flags=180
bodycreate setters_enter_block=0
bodycreate setters_leave_traffic=0.1 a f.50
bodycreate setters_leave_inverses=3f800000.3f800000.7f800000.3f000000
bodycreate setters_leave_flags=100
bodycreate setters_leave_block=0
bodycreate setters_drowsy_created=1
bodycreate setters_drowsy_wake=3e000000.3e000000
bodycreate setters_mode5_wake=3ecccccc.3ecccccc
bodycreate setters_mode5_accumulators=0.0.0.0.0.0.0.0.0.0.0.0
bodycreate setters_mode5_velocity=0.0.0.0.0.0
```

## Ready-to-paste static_proof sentences

- 000782: "Listing 0x18730-0x19491 walked in full (setters sub-area): thiscall ret 0x10; mode > 4 falls to the
  wake block; jump table 0x10019494 arms reproduced with the listing's x87 grouping (modes 0/3: linear x kept
  in the register, y/z spilled; angular x/y kept, z spilled; mode 1: linear all spilled, angular x kept) and
  dirty masks 0x20/0x40, 4/8, 4/8, 0x80/0x100, 0x80/0x100; wake on the argument's low byte (0x1936f). Candidate
  BodyCreation.cpp DynamicBody::addForce; the only difference is the dirty-mark queue growth's allocator
  (nxGetSdkAllocator where the image uses [0x101041bc]; Task 2 deferred list)."
- 000784: "Listing 0x194b0-0x1961d walked in full: thiscall ret 8; position/quaternion into the +0x118 block
  with +0xc |= 1 / |= 2, then the wake block. Candidate BodyCreation.cpp DynamicBody::setKinematicTarget.
  Not called: its callers 000090/000124/000126 are NpActor-unit rows whose inline emulation remains (handover);
  source-only."
- 000785/000787: "Listing 0x19620-0x19cf9 walked in full as one function (000787 is the leave path's tail):
  thiscall ret 4, dword-tested argument; inlined 000748 island step; enter zeroes +0xc0..cc and allocates the
  0x20-byte block from [0x101041bc]; leave divides 1.0f by +0x188..0x194 unconditionally (z spilled; stores
  x, z, y) and frees the block through [0x101041bc]. Candidate BodyCreation.cpp DynamicBody::setKinematic."
- 000789: "Listing 0x19d00-0x1a2b4 walked in full: thiscall, no args; 000746 first; M = C F^T in the listing's
  sum order; displacement x kept, y/z spilled; quaternion arms with fsqrt through X87Sqrt.h
  (x87FsqrtSum4/Diag/Diag/Sum3) and the listing's spills (0x19ff3, 0x1a0a7, 0x1a114, 0x1a16c); marks 1 and 2.
  Candidate BodyCreation.cpp DynamicBody::setPoseFromCMass."
- 000791: "Listing 0x1a2c0-0x1a342 walked in full: thiscall ret 0x10; dx kept in the register, dy/dz spilled;
  torque d x force formed x, y, z; one call to 000782 with the mode and wake words passed through. Candidate
  core/JointSupport.cpp Row000791Fixture::row000791 (main's name and signature)."

Proposed state: 000782, 000785/000787, 000789 and 000791 have hit counts equal to the oracle's on every target,
so they are eligible for `reconstructed` with dynamic proof once the trace excerpt is committed. 000787's own
address has 0 hits on the oracle side (the growth `jmp`), so it follows 000785 as body-creation's 000795
followed 000793. 000784 is source-only `reconstructed` (static) or left `discovered` until its call sites are
wired; that is the integrator's choice (its callers are not ours).

## Handover (NpActor session)

- **000784**: replace the inline emulation in 000090/000124/000126 (moveGlobalPosition/Pose/Orientation) with
  `reinterpret_cast<DynamicBody*>(record)->setKinematicTarget(pos or 0, quat or 0)`. The oracle side hits it
  5 times on ActorDynamics; the candidate side has 0.
- The four NpActor.cpp forwarders above are the only NpActor.cpp changes. When merging, keep the forwarders
  and do not merge the old emulation bodies back.
- `nxNpActorQuaternionFromMatrix` (NpActorDynamicMath.h) is now unused. `nxNpActorSetterQuaternionFromMatrix`
  (000196/000200) still uses CRT sqrt; 000789's conversion is the same sequence through X87Sqrt.h
  (`bodyPoseQuaternionFromMatrix`, BodyCreation.cpp).
- 000140/000142 still use `nxNpActorWorldTensor` (Momentum: 14 missing 000746 hits).
