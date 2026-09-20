# Phase 6 joints and effectors — reconstruction evidence

This file records Phase 6 evidence. It begins with the harness and an audit of
the export contract, because the second turned out to bound the first.

## 6a. State on entry

`program.json` records phases 4 through 8 as `pending`. The oracle census is
complete: 6,338 function rows and 5,138 data objects, 0 unexplained bytes. Of the
6,338 function rows, 663 are `reconstructed`, 115 `dynamically_gated`, 6
`statically_reviewed` and 5,554 `discovered`; no row is `closed`, and `closed` is
the Phase 8 audit's to grant.

`gate_targets.ps1` maps phases 6, 7 and 8 to empty differential target arrays, so
`run_phase_gate.ps1 -Phase 6` reports `status=skipped
reason=no_registered_test_targets`. Phase 6 has no harness and no closure ledger.

Phase 6's own plan (`docs/superpowers/plans/2026-08-09-novodex-physics-phase6-joints-effectors.md`)
calls for `tests/PhysicsJointTests.cpp` and `joint_model.json`. Neither existed.

## 6b. The harness exists, and it is a new translation unit

Created `tests/PhysicsJointTests.cpp` and wired it as the `NxPhysicsJointTests`
target in `CMakeLists.txt`.

**Why a new file rather than more cases in `PhysicsObjectLayoutTests.cpp`.** That
harness's `wmain` carries a ~250 KB frame against the CRT's 1 MB stack, and any
change to it arms a jump through a 0xA5 fixture fill. Five rounds of conversion
work did not remove it, and it is recorded across
`evidence/phase5-object-model.md` 3z262-3z289. A new translation unit has no such
history; this is the unblocked path Phase 6's plan already specified.

**What it does.** It loads the pair through `PhysicsPairLoader.h` -- the same
isolated-directory, module-audit, SHA-256-identity machinery every other
differential uses -- creates the SDK through the exported `NxCreatePhysicsSDK`,
builds a two-actor dynamic fixture, drives the revolute family over four
anchor/axis cases, and prints every input and output word as raw hexadecimal. It
decides nothing; `run_differential.ps1` compares its transcript between the two
pairs.

**Two implementation notes that cost a build each**, recorded so they are not
rediscovered:

- `NxJointDesc::setGlobalAnchor` and `setGlobalAxis` are inline and call two
  exported rows, so using the inline methods adds imports for
  `__imp__NxJointDesc_SetGlobalAnchor` and `__imp__NxJointDesc_SetGlobalAxis`.
  This harness loads the pair by `LoadLibraryEx` and links against neither side's
  import library, so both rows are resolved with `GetProcAddress` and called
  through function pointers.
- `NxActorDesc::isValid()` accepts either a body with a mass *and* a mass-space
  inertia, or a non-zero density with at least one shape. A default `NxBodyDesc`
  carries mass 0 and zero inertia, so a fixture that sets neither makes
  `createActor` return null. The fixture takes the density route: `density = 1.0`
  with one box shape.

**Measured against the oracle.** Exit 0, and the transcript is byte-identical
across three runs (SHA-256 `9804d0d6ae18ce3b…`), so it is a usable differential
transcript rather than a noisy one. It reports all four revolute cases created,
anchor and axis reading back as passed, actors matching, and release clean. One
case is informative: index 3 passes axis `3f000000.3f000000.3f000000`
(0.5, 0.5, 0.5) and reads back `3f13cd3a.3f13cd3a.3f13cd3a` -- the oracle
normalises the axis, and the transcript pins that.

## 6c. The export contract is missing three rows, and two are Phase 6's

Audited with `dumpbin /exports`:

    oracle Binaries/NxPhysics.dll          41 named exports
    candidate build/Release/NxPhysics.dll  38 named exports

    missing from the candidate:
      NxJointDesc_SetGlobalAnchor
      NxJointDesc_SetGlobalAxis
      NxCreatePMap

`program.json` pins `oracle.named_exports: 41`, and Phase 8's gate requires the
candidate's export table to equal the oracle's with ordinals. A three-row gap
therefore fails that gate on its own, independently of any behavioural evidence.

**Two of the three are genuinely unimplemented.** Searching the whole of
`Physics/src` for `NxJointDesc_SetGlobalAnchor` and `NxJointDesc_SetGlobalAxis`
returns nothing, so the source does not define them at all. They are the two
Phase 6 rows the plan names explicitly ("include the exported
`NxJointDesc_SetGlobalAnchor` and `NxJointDesc_SetGlobalAxis` rows and record
exact transform/evaluation order"), and the joint harness cannot run against the
candidate until they exist: it reports
`export=NxJointDesc_SetGlobalAnchor present=no` and stops.

**The third is an export-list gap, not a missing body.** `NxReleasePMap` is both
defined and exported (`Physics/src/PMap.cpp`), while `NxCreatePMap` appears in
that file only inside comments. Whether it is unexported, misnamed, or a body
that was never written is not established here and is not claimed.

**The `pairs/candidate` staging is also stale.** Its `NxPhysics.dll` is 37,376
bytes and contains neither `NxCreatePMap` nor `NxReleasePMap`, while the current
`build/Release/NxPhysics.dll` contains `NxReleasePMap`. Any differential run
against that staged pair measures an older candidate than the tree, so the
staging has to be refreshed before its transcripts mean anything. This is the
same class of hazard as 3z266 and 3z276: an instrument returning a reading that
belongs to an earlier state.

## 6d. What is not done

- The nine other joint families are **not driven**: spherical, prismatic,
  cylindrical, point-on-line, point-in-plane, D6, distance, fixed and pulley.
  `PhysicsJointTests.cpp` says so in its own header rather than reporting a
  coverage figure that overstates.
- The two exported rows are **not implemented**.
- `joint_model.json` does **not** exist.
- `NxPhysicsJointTests` is **not registered** as a differential target, because
  it cannot pass against the candidate until the two rows exist and the staging
  is refreshed. Registering it now would turn phase 6 from `skipped` into a
  failure without adding evidence.

No rows move. No gate, coverage-floor, or policy change.
## 6e. The two missing rows are substantial, and that bounds the next step

`NxJointDesc_SetGlobalAnchor` is not a setter. At RVA `0x980b0` it is a 460-byte
routine that, for each of the descriptor's two actors, reconstructs the actor's
world pose from its stored position and orientation, composes a rotation matrix
from the quaternion terms with the usual doubled products (the
`fadd st(0), st(0)` / `fsub` sequence at `0x1009811e`-`0x10098138`), and
transforms the passed world anchor into that actor's local frame, writing
`localAnchor[0]` and `localAnchor[1]`. It walks `desc->actor[i]`, dereferences the
actor's shape/pose chain, and bails to a common exit when either side is absent.

`NxJointDesc_SetGlobalAxis` at `0x982e0` is the same shape over the axis.

So Phase 6's first implementation task is not a two-line setter but a
world-to-local transform whose float behaviour must match the oracle exactly, and
the joint harness's `out_anchor` / `out_axis` lines already pin the normalisation
the oracle applies (revolute case index 3). Implementation has not started, and no
row moves.

## 6f. What the next session should do, in order

1. Refresh the staged candidate pair from `build/Release`, because the current
   one predates `NxReleasePMap` and any differential against it measures an older
   candidate.
2. Transcribe the two exported rows from their disassembly and drive them through
   `NxPhysicsJointTests`. The four revolute cases already produce `out_anchor` and
   `out_axis` words that move if the transform is wrong, so this is a real
   differential rather than a smoke test.
3. Only then extend the harness to the other nine families, and only then
   register `NxPhysicsJointTests` as a phase-6 differential target.

Registering it before step 2 would turn phase 6 from `skipped` into a failure
without adding evidence, which is the shape of gate this programme has already
been caught building.

## 6g. Both rows are implemented, and the differential judges them

`Physics/src/JointDesc.cpp` implements the two exported rows:

    NxJointDesc_SetGlobalAnchor  (census phys_fn_004115, 0x980b0)
    NxJointDesc_SetGlobalAxis    (census phys_fn_004117, 0x982e0)

The candidate's export set goes from 38 to **40** named exports; `NxCreatePMap`
is the only remaining gap against the oracle's 41.

**The transcript is the judge, and it is now a real one.**
`tests/PhysicsJointDescTests.cpp` was added because `NxPhysicsJointTests` drives
the whole SDK lifecycle (create SDK, create scene, create actors, create a joint)
and so is sensitive to rows these two do not depend on -- the candidate's
`createScene` currently returns null, which stops that harness before it reaches a
joint. The descriptor differential constructs the descriptor by hand, sets the
actor pointers, and calls the two rows through their exported addresses, so a
difference in it is a difference in these two rows and nothing else.

It prints the whole descriptor surface before and after each call, over four
cases: a generic anchor/axis, a zero axis, a NaN axis, and an already-unit axis.
The NaN case supplies its bits directly rather than through an arithmetic
expression, per 3z266.

**What the differential proves.** `localAxis` is **identical to the oracle on
every case**, including the two non-finite ones. The world-to-local transform --
the actor-graph walk, the quaternion-to-matrix composition, the translation
subtraction and the row-by-row product -- is therefore transcribed correctly. The
zero-axis case is the sharpest evidence: the oracle leaves the zero axis
unnormalised and writes it straight through, and the transcription does too.

**A probe rather than a guess.** `tests/NxNormalToTangentsProbe.cpp` prints what
the pinned `NxNormalToTangents` returns for each axis, which attributes the
oracle's `localNormal` words to a specific output instead of leaving them to
inspection. It shows the oracle's `localNormal` is the SECOND tangent (`t2`), not
the first:

    case0  n=3f13cd3a.3f13cd3a.3f13cd3a   t2=bed105ec.bed105ec.3f5105ec
    case3  n=00000000.3f800000.00000000   t2=80000000.80000000.3f800000

and both match the oracle's `localNormal` for those cases word for word. The
oracle's own call site agrees: it pushes the first tangent at `[esp+0xc]`, the
normal at `[esp+0x48]` and the second at `[esp+0x30]`, and the `localNormal`
stores read the `[esp+0x30]` block.

## 6h. Open: one ULP on `localNormal`, and one non-finite sign

The differential is **not yet green**. Two differences remain, both confined to
`localNormal`, with `localAxis` exact everywhere:

- **Case 0, one ULP.** Oracle `bed105ec.bed105ec.3f5105ec` against candidate
  `bf3504f4.3f3504f4.00000000`. The probe shows the oracle's value is `t2` for
  the normalised axis `3f13cd3a.3f13cd3a.3f13cd3a` exactly, so the remaining
  difference is in the axis handed to `NxNormalToTangents`, not in which tangent
  is used. Matching the oracle's x87 sequence (`fdivr` by 1.0 then three
  `fmul st(1)` before any 32-bit store, at `0x10098339`-`0x10098359`) by holding
  the reciprocal and the products in `double` did not close it, so the difference
  is upstream of the reciprocal and is not yet located.
- **Non-finite sign.** Case 1 and case 2 show `7fc00000` where the candidate
  produces `ffc00000` in the x lane. This is the NaN sign/payload class the
  README's generator warning is about, and it is the same class Phase 3 met.

Neither is a reason to move a row's state, and no row moves: both rows are
`discovered` in the census and stay there until the differential is green and the
closure ledger records the proof.

## 6i. What the next session should do

1. Locate the case-0 ULP. The probe is the instrument: print the normalised axis
   the row actually computes and compare it with `3f13cd3a.3f13cd3a.3f13cd3a`.
   The oracle's three component products are `fld [esp+0x10] / fmul st(1)` in
   that order, so the operand order is already known and only the rounding
   differs.
2. Decide the non-finite sign question as the README directs: reproduce the
   oracle's bit pattern, do not normalise it away.
3. Only then drive the two rows from `NxPhysicsJointTests` as well, and only then
   extend to the other nine families.
