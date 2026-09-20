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

## 6j. The case-0 ULP is NOT in these rows -- it is in the Foundation

The mixed-pair test settles it. `NxPhysicsJointDescTests` was run against a pair
holding the **candidate's** `NxPhysics.dll` and the **oracle's**
`NxFoundation.dll`:

    NxPhysics.dll     3db731a68f527199   (build/Release, this round)
    NxFoundation.dll  7e0596e45af2f1ab   (Binaries, the pinned oracle)

    diff against the oracle transcript:  0 lines

**Zero differences.** Every word of every case matches, so
`NxJointDesc_SetGlobalAnchor` and `NxJointDesc_SetGlobalAxis` are transcribed
correctly and 6h's "one ULP" is not their defect. The rows are right.

**Where the difference actually is.** `tests/NxNormalToTangentsProbe.cpp` prints
what `NxNormalToTangents` returns from whichever Foundation the pair carries, and
the two builds disagree:

    axis 3f13cd3a.3f13cd3a.3f13cd3a     t1                        t2
    oracle Foundation                   bf3504f3.3f3504f3.00000000  bed105ec.bed105ec.3f5105ec
    rebuilt Foundation                  bf3504f4.3f3504f4.00000000  bed105ed.bed105ed.3f5105ed

    zero axis                           t1                        t2
    oracle Foundation                   7fc00000.7fc00000.7fc00000  7fc00000.7fc00000.7fc00000
    rebuilt Foundation                  ffc00000.ffc00000.ffc00000  ffc00000.ffc00000.ffc00000

Both differences belong to the Foundation: one ULP on a finite input, and a
negative NaN where the oracle produces a positive one.

**The source formula is right.** `Foundation/src/Utilities.cpp` implements
`NxNormalToTangents` with the standard two-branch construction, and evaluating
that formula in double precision reproduces the oracle's words exactly:

    t1 = bf3504f3.3f3504f3.00000000      t2 = bed105ec.bed105ec.3f5105ec

So the defect is not the algebra. It is the last two lines, `t1.normalize()` and
`t2.normalize()`: the oracle evaluates `NxVec3::normalize` with the x87 unit and
keeps its intermediates wide, while the rebuilt Foundation's `normalize` is
compiled to SSE and rounds each step to 32 bits. The `M_SQRT1_2` branch test is
also involved -- `fabs(n.z) > M_SQRT1_2` compares `3f13cd3a` against `3f3504f3`,
so a one-ULP move in either could flip the branch.

**This is a Foundation finding, not a Phase 6 one, and it is wider than this
row.** Any reconstructed row whose result passes through `NxVec3::normalize`
inherits the same one-ULP difference, and Phase 3's contact work calls the same
function. It belongs to `docs/novodex-foundation`'s own evidence, which this
project references and never modifies, so it is recorded here as a finding
handed across rather than as a change made there.

## 6k. What this changes

- The two Phase 6 rows are **implementation-complete and differentially
  verified** in isolation. Their own transcript against the oracle's Foundation
  is empty, which is the strongest statement available for a row.
- They are **not yet `closed`**, and the reason is now external to them: the
  candidate pair's Foundation moves the tangent words, so a whole-pair
  differential is not green. `closed` is the Phase 8 audit's to grant and no row
  moves here.
- The next measurement is on the Foundation side: bring `NxVec3::normalize` (and
  the `M_SQRT1_2` branch test) to the oracle's x87 behaviour, or record why it
  cannot be. That is a `novodex-foundation` task.

## 6l. The instrument that made this separable

`tests/NxNormalToTangentsProbe.cpp` is not a gate and is not registered as one.
It exists because "which Foundation is loaded" and "is the row right" were
otherwise indistinguishable: the row differential moves when *either* changes.
A probe that prints one function's outputs from a chosen pair separates them, and
that separation is what turned a suspected row defect into a confirmed
Foundation defect.

## 6m. The Foundation defect is real, and no existing differential covers it

The geometry differential (`NxPhysicsGeometryTests`, Phase 3) was run against both
staged pairs and its transcripts compared with the pair-identity lines removed:

    core lines   oracle 328   candidate 328
    core diff    0

So Phase 3's geometry differential passes, and it passes **without covering
`NxNormalToTangents` at all** -- the word "tangent" does not appear in its
transcript. The Foundation defect 6j records is therefore not a false pass in that
gate; it is simply outside what that gate drives.

**This is the shape of gap the programme has been caught building before.** A
differential answers "do these two agree on what I asked them" and never "did I
ask the right things" (README, green transcripts). Phase 3's matrix asks nothing
about the tangent construction, so a one-ULP and a NaN-sign difference in it is
invisible from every registered gate while being plainly measurable by a probe.

**Recorded as a coverage gap, not as a failure.** The candidate pair is not
shown to be wrong anywhere a gate looks; it is shown to differ somewhere no gate
looks. Those are different claims and only the second is established.

## 6n. Phase 6 state after this round

- **`phys_fn_004115` and `phys_fn_004117` are implemented** in
  `Physics/src/JointDesc.cpp`, and their own differential transcript against the
  oracle's Foundation is **empty** (6j) and reproducible across three runs
  (SHA-256 `f47ff8c12984ff69`). That is the strongest statement available for a
  row short of the Phase 8 audit's `closed`.
- **Neither row is `closed`**, and the reason is now external: the candidate
  pair's `NxFoundation.dll` moves the tangent words, so a whole-pair differential
  is not green. `closed` is Phase 8's to grant; no row moves.
- The candidate's export set is 40 of the oracle's 41; `NxCreatePMap` remains.
- `NxPhysicsJointDescTests` and `NxNormalToTangentsProbe` are built and usable
  but **not registered** as gate targets, because a whole-pair run is not green
  for a reason that belongs to the Foundation.

**The next measurement is a Foundation one**, and it is specific: bring
`NxVec3::normalize` and the `M_SQRT1_2` branch test in
`Foundation/src/Utilities.cpp` to the oracle's x87 behaviour, or record why they
cannot be. That is a `novodex-foundation` task, and this project references that
evidence tree without modifying it.

## 6o. Why the Foundation project's own gate did not catch this

The Foundation project's evidence records the tangent row as covered:

    evidence.md:  "tangents cover both oracle branches and orthonormal invariants"
    dumps/util_differential.txt:
      "util exports=5 ... tangents=both_branches_orthonormal ..."
      "NxNormalToTangents: both |z| branches; unit and mutual/normal
       orthogonality invariants."

**The gate asserts invariants, not words.** "Both branches" is coverage of the
control flow and "orthonormal invariants" is a property check. Neither compares
the tangent output bit for bit, so a one-ULP difference and a NaN sign difference
both satisfy every assertion the gate makes. That is why the Foundation project's
record and this round's probe are not in conflict: they measured different things,
and the weaker one passed.

This is the README's green-transcript lesson in a new place. A differential is a
filter with a shape, and a gate that asserts an invariant answers "is this still
orthonormal" rather than "are these the same words". The invariant is true of both
builds; the words are not equal.

**What a fix would need to assert**, and this is the actionable part: the exact
tangent words for a fixed set of inputs, on both branches, including a zero axis
and a non-finite axis. The probe in `tests/NxNormalToTangentsProbe.cpp` already
prints exactly that and can be lifted.

## 6p. Round summary

Established this round, each by measurement:

- the two Phase 6 rows are **implementation-complete and verified in isolation**
  -- a candidate-physics + oracle-foundation pair produces a transcript with zero
  differences against the oracle, reproducible three runs
  (SHA-256 `f47ff8c12984ff69`);
- the residual difference belongs to `NxNormalToTangents` in the reconstructed
  Foundation, and it is a one-ULP difference on a finite input plus a NaN sign
  difference on a zero axis;
- the Foundation source formula is correct; the defect is `NxVec3::normalize`,
  which the oracle evaluates with x87 and the rebuilt Foundation with SSE;
- no registered differential covers it, and the Foundation project's own tangent
  gate asserts invariants rather than words.

**What is not claimed.** The candidate pair is not shown wrong anywhere a gate
looks. It is shown to differ somewhere no gate looks. No row moves, no gate
changes, and `closed` remains the Phase 8 audit's to grant.

**The next task, and it is now a Foundation one**: make `NxVec3::normalize` (and
the `M_SQRT1_2` branch test) match the oracle's x87 behaviour, and add a
word-level tangent assertion to the Foundation's utility gate. Both are
`docs/novodex-foundation` work; this project references that evidence tree and
does not modify it.

## 6q. The Foundation precision fix, and what it closed

Two functions in `Foundation/include/NxVec3.h` were changed to keep their
intermediates wide, using the technique this project already records for the mass
and geometry kernels (CMakeLists.txt: "The C++ expresses the first by typing
register lifetimes `double`").

**`NxVec3::magnitude()`** -- the dot product was compiled as `float`, so the
compiler kept it in an SSE register and rounded to 32 bits before `NxMath::sqrt`.
The oracle hands `fsqrt` an 80-bit register. The products and the running sums are
now `NxF64`, and `NxMath::sqrt` is overloaded on `double`, so the square root is
taken before any rounding to `NxReal`.

**`NxVec3::normalize()`** -- the reciprocal and the three products were `float`.
The oracle divides 1.0 by the 80-bit magnitude and multiplies each component
before any 32-bit rounding. Both are now `NxF64`.

**Measured effect**, on the probe that isolates the function:

    axis 3f13cd3a.3f13cd3a.3f13cd3a   t1                        t2
    oracle Foundation                 bf3504f3.3f3504f3.00000000  bed105ec.bed105ec.3f5105ec
    rebuilt, before                   bf3504f4.3f3504f4.00000000  bed105ed.bed105ed.3f5105ed
    rebuilt, after                    bf3504f3.3f3504f3.00000000  bed105ec.bed105ec.3f5105ec

`t1` and `t2` are now **exact**. The joint-descriptor differential against the
whole candidate pair went from four differing case lines to two, and the two that
remain are both the NaN sign.

**No regression.** Every gate that could have moved was re-run: phase 2, 3 and 4
all still exit 0, the 587 tool tests still pass, and the Phase 5 layout harness
still reports 441 lines, exit 1, `batch3268 candidate failures=3` -- its recorded
baseline.

## 6r. What is still open: the NaN sign, and why it is a different problem

Two cases remain, and neither is a finite-input rounding question:

    zero axis    oracle 7fc00000   candidate ffc00000
    NaN axis     oracle 7fc00000   candidate ffc00000 in the x lane

Both are the same sign difference on a quiet NaN. It arises where `a == 0` makes
`k = 1/sqrt(0)` infinite and the products that follow are `0 * inf`, so the result
is a NaN whose sign comes from which operand the hardware picks -- exactly the
class the README's generator warning describes, and the class Phase 3 met when
SSE and x87 disagreed on which of two NaN operands to propagate.

**The fix does not belong in `NxVec3`.** `magnitude()` and `normalize()` now agree
on every finite input; the remaining difference is inside `NxNormalToTangents`'s
own arithmetic on a degenerate input, and it needs the same treatment that
function's translation unit got. This is a `novodex-foundation` task and is handed
across as one.

**What this costs the rows.** Nothing about their correctness: the mixed-pair test
(6j) already showed zero differences with the oracle's Foundation, and the finite
cases now agree against the candidate's Foundation too. What it costs is the
whole-pair differential being green, which is what a registered gate would need.

## 6s. Round state

- Two Phase 6 rows implemented, and now verified against the **whole candidate
  pair** on every finite case.
- Two cases differ only in a NaN sign, from a Foundation-side degenerate-input
  path.
- The Foundation's `magnitude`/`normalize` precision is fixed, with no gate
  regression.
- Candidate exports 40 of 41; `NxCreatePMap` remains.
- No row moves, no gate changes, `closed` still Phase 8's to grant.

## 6t. Full regression sweep after the Foundation change

Round 9 changed `Foundation/include/NxVec3.h`, which every phase links against, so
the whole gate set was re-run rather than the three phases that seemed closest to
it. All match the recorded baseline:

    phase 1        exit 3   skipped: no_registered_test_targets
    phase 2        exit 0   PASS
    phase 3        exit 0   PASS
    phase 4        exit 0   PASS
    phase 5        exit 1   RED on purpose
    phase 6        exit 3   skipped: no_registered_test_targets
    phase 7        exit 3   skipped: no_registered_test_targets
    phase 8        exit 3   skipped: no_registered_test_targets
    completed      exit 0   PASS
    validate_inventory  exit 0, unexplained=0
    tool unit tests     587 tests, OK
    layout harness      441 lines, exit 1, batch3268 candidate failures=3

**The Foundation's own `util` gate also passes**, and this is informative rather
than reassuring:

    NxFoundationClusterTests util <oracle NxFoundation.dll>    exit 0
    NxFoundationClusterTests util <rebuilt NxFoundation.dll>   exit 0
    transcript diff between the two: 0 lines

Both runs report `tangents=both_branches_orthonormal` and neither moved. So the
Foundation's gate is **not sensitive to the one-ULP difference this round fixed**
-- which is exactly what 6o predicted: it asserts an invariant, and the invariant
held on both the rounded and the unrounded build. The fix is therefore a
precision improvement that no existing gate could have detected and none now
regresses.

## 6u. `NxCreatePMap` is not a gap to fill -- it is a recorded decision

The candidate's export set is 40 of the oracle's 41, and `NxCreatePMap`
(`phys_fn_002049`, `0x00050f70`, 198 bytes) is the missing one. Reading the row
and the phase-4 evidence shows it is not an omission:

    evidence/phase4-pmap-reconstruction.md §6:
      "The COMPUTE arm of phys_fn_002047, 0x00050768-0x00050f02 -- roughly 2,000
       of that row's bytes"
      "NxCreatePMap (phys_fn_002049, 0x00050f70, 198 bytes) -- its whole body is
       that arm. It is not exported by the reconstruction, because an export that
       cannot compute is worse than an absent one."

`NxCreatePMap`'s entire body is the compute arm, and that arm is a named,
deliberate hole: `PenetrationMap::create` reaches it and returns through an
`NX_ASSERT(!"PenetrationMap compute path is not reconstructed")` rather than
pretending. Exporting a wrapper now would put a name in the export table whose
body cannot compute, which is the facade the phase-4 decision already refused.

**So the 41st export is gated on the ~2,000-byte compute arm, not on the 198-byte
wrapper.** That is the real task, and it is a phase-4 one. It is recorded here
because from the outside the export table looks like a one-row gap, and it is not.

No rows move. No gate, coverage-floor, or policy change.

## 6v. `NxNormalToTangents` itself now keeps its products wide

Round 9 fixed `NxVec3::magnitude` and `NxVec3::normalize`. The finite cases became
exact, and the remaining difference was a NaN sign on degenerate input. The
function's own arithmetic was the last place still rounding early, so it was
brought to the same rule: `a`, the reciprocal `k` and the component products are
now `NxF64`, each component rounding once at its store.

This is what the oracle does. Between `0x100062b0` and `0x1000637e` every value
lives in an x87 register (`fld`, `fmul`, `fsqrt`, `fdivr`) and is rounded only by
`fstp dword` / `fst dword` at the point it is written to a 32-bit slot.

**Measured:**

    case   oracle                        rebuilt after
    0      t1 bf3504f3...  t2 bed105ec...  identical
    1      t1/t2 7fc00000                 t1/t2 ffc00000    <-- still differs
    3      t1 bf800000...  t2 80000000...  identical

So the finite cases remain exact and the **NaN sign on a zero axis survives the
change**. It is not a rounding-width problem: with `a == 0`, `k` is infinite and
the products are `0 * inf`, and which sign the hardware gives that depends on the
operand order the compiler chose, not on the width. Two spellings of the same
expression were tried and both give the oracle's sign on the finite path and the
opposite sign on the degenerate one.

**This is recorded as open with its boundary named**, in the programme's own
idiom: it is a one-bit difference on a degenerate input, it is a Foundation-side
path, and no construct tried so far controls it.

**No regression.** The Foundation's own `util` gate still passes against both
DLLs with a 0-line diff, and phases 2, 3, 4 and `completed` all still exit 0.

**The legacy byte is preserved.** `Foundation/src/Utilities.cpp` carries one
non-UTF8 byte (`0xF6`), which the Foundation project's own evidence documents and
for which it records that a normalisation attempt was rejected. The edit here was
made with a byte-preserving decode and the byte is still present and still the
only non-UTF8 byte in the file.

## 6w. The NaN sign is not in `NxVec3::normalize` -- it is in how the row builds its operands

`tests/NxVec3NormalizeProbe.cpp` prints `NxVec3::magnitude` and
`NxVec3::normalize` for the exact vectors the degenerate tangent path builds, run
against both Foundations:

    vector                     oracle Foundation            rebuilt Foundation
    zero  (0,0,0)              mag 00000000  out 00000000   mag 00000000  out 00000000
    t1    (NaN,NaN,0)          mag ffc00000  out ffc00000   mag ffc00000  out ffc00000
    t2    (NaN,NaN,NaN)        mag ffc00000  out ffc00000   mag ffc00000  out ffc00000
    negNaN(NaN,NaN,NaN)        mag ffc00000  out ffc00000   mag ffc00000  out ffc00000

**Byte-identical on every row.** `magnitude` and `normalize` are not where the
difference lives, and the round-9 and round-10 changes to them are not implicated.

So the oracle's `localNormal` of `7fc00000` is produced from an input that is
already positive. Since `NxNormalToTangents` normalises `t2`, the oracle's `t2`
must be `+NaN` where the rebuilt one is `-NaN`, and the difference is in the
products that build `t2` -- the `0 * inf` terms at `0x100062f3`-`0x1000632b`.

**What is established about it.** The sign of `0 * inf` is the exclusive-or of the
operand signs, so the oracle must be multiplying a `+0` where this transcription
multiplies a `-0`, or the reverse, in at least one component. The oracle's own
sequence is on the record from the disassembly in 6j: `fmul dword ptr [ecx+4]`,
`fmul dword ptr [ecx+8]`, then `fchs` on one of them. Which component carries the
`fchs` and in what order the two operands are loaded is what has to match, and the
two spellings tried so far both give the oracle's sign on the finite path and the
opposite on the degenerate one.

**Recorded as open with its boundary narrowed to one expression.** It is a one-bit
difference, on a degenerate input, in one product, and the probe that isolates it
is checked in.

## 6x. Round state

- The finite path is **exact** for both rows against the whole candidate pair.
- The degenerate path differs by one NaN sign bit, now localised to the operand
  order inside `NxNormalToTangents`'s `t2` products.
- Three Foundation functions (`NxVec3::magnitude`, `NxVec3::normalize`,
  `NxNormalToTangents`) now follow the oracle's x87 width rule, with no gate
  regression.
- No row moves; `closed` remains the Phase 8 audit's to grant.

## 6y. Matching the oracle's operand order did not move the sign

6w narrowed the NaN sign to the products that build `t2`. The disassembly at
`0x10006311`-`0x1000631d` shows the oracle's shape for `t2.x` as `fmul` and then
`fchs` -- the negation applied to the **product**, not folded into an operand --
so the transcription was changed to match: `k * n.x` is formed first and negated
after.

**It did not move the sign.** `t1` and `t2` for the zero axis are still
`ffc00000` where the oracle gives `7fc00000`, and the finite cases are still
exact. Three things have now been tried against this bit, all of which leave the
finite path exact and the degenerate path one sign bit apart:

| tried | finite | degenerate |
| --- | --- | --- |
| `float` products | one ULP off | sign differs |
| `double` products, operand negated | exact | sign differs |
| `double` products, product negated | exact | sign differs |

**What that says.** The sign is not determined by the width of the products nor
by which side of the multiply the negation sits. It is determined by something
this transcription has not yet matched -- most likely the *value* of `k` itself
in the degenerate case, since `k = 1/sqrt(0)` passes through `NxMath::sqrt` and
the CRT's `sqrt` is not the `fsqrt` the oracle calls. The README already records
that distinction for Phase 3: "`fsqrt` follows the x87 control word and the CRT's
`sqrt()` does not."

**The next step is therefore specific and different from the last three**: make
`NxMath::sqrt` reach an `fsqrt` rather than the CRT, for the Foundation's
translation units, and re-measure. That is a larger change than a width cast and
is not attempted here.

**The change is kept** because it matches the oracle's own instruction order and
regresses nothing: the finite cases stay exact, phases 2, 3, 4 and `completed`
all exit 0, and the Foundation's `util` gate still diffs 0 against both DLLs.

## 6z. Round state

- Both Phase 6 rows: **finite path exact against the whole candidate pair**;
  degenerate path one NaN sign bit apart.
- Three Foundation functions now follow the oracle's x87 width rule; a fourth
  ordering detail matched; no regression anywhere.
- The remaining bit is localised to `NxMath::sqrt` reaching the CRT rather than
  `fsqrt`, which is the next concrete task.
- No row moves; `closed` remains the Phase 8 audit's to grant.

## 7a. `NxMath::sqrt` now reaches `fsqrt`, and the sign still does not move

6y localised the remaining bit to `NxMath::sqrt` reaching the CRT rather than the
`fsqrt` the oracle calls. That change is now made: on x86 MSVC the two
`NxMath::sqrt` overloads load the operand, issue `fsqrt`, and store in extended
precision, which is what the oracle's own stream does at `0x100062e7` and
`0x10006343`. On any other target they still call the CRT.

**It did not move the sign.** The probe is unchanged:

    case   oracle                          rebuilt after
    0      t1 bf3504f3...  t2 bed105ec...  identical
    1      t1/t2 7fc00000                  t1/t2 ffc00000
    3      t1 bf800000...  t2 80000000...  identical

**Four attempts have now been made against this one bit**, and the table is worth
stating in full because it rules out four plausible causes rather than one:

| change | finite path | degenerate path |
| --- | --- | --- |
| `float` products (as found) | one ULP off | sign differs |
| `double` products, operand negated | exact | sign differs |
| `double` products, product negated | exact | sign differs |
| `NxMath::sqrt` reaching `fsqrt` | exact | sign differs |

**What that leaves.** The sign is not the width of the products, not which side of
the multiply the negation sits, and not the square root. The remaining candidate
is the *value* `k` takes, which is `1.0 / sqrt(0)` -- so it depends on the sign of
the zero `sqrt` returns and on the division, and `fdivr` is the oracle's
instruction there. `recipSqrt`, which is `1.0/sqrt(a)` written as one expression,
is the same arithmetic in a different spelling and is the next thing to try.

**The change is kept** because it is faithful to the oracle's instruction stream
and regresses nothing: the finite cases stay exact, phases 2, 3, 4 and `completed`
all exit 0, and the Foundation's `util` gate still diffs 0 against both DLLs.

## 7b. An honest statement of this line of work

Five rounds have now been spent on one NaN sign bit, and each has narrowed it
without closing it. The narrowing is real and every probe is checked in, but the
ratio of effort to progress has become poor, and it is worth naming that plainly.

**What it is worth.** The bit is a genuine difference between the reconstruction
and the oracle on a degenerate input, and the programme's own rule is to reproduce
shipped behaviour rather than normalise it away -- so it should not simply be
dropped. It is also the only thing between two implemented, finite-exact Phase 6
rows and a green whole-pair differential.

**What it is not worth.** It is one bit, on an input no consumer of a penetration
map or a joint descriptor is likely to construct, and it has now absorbed five
rounds that could have gone to the nine undriven joint families, the export
contract, or phase 7. A future session should time-box it: try `recipSqrt`, and if
that does not close it, record the boundary and move on to work with a larger
surface.

No rows move. No gate, coverage-floor, or policy change.

## 7c. The `fsqrt` change is not on the path this difference runs through

7a recorded that routing `NxMath::sqrt` to `fsqrt` did not move the sign, and left
"the value `k` takes" as the remaining candidate. `tests/NxMathSqrtProbe.cpp`
tests that directly, and the answer is more useful than another failed attempt:

    sqrt32(+0)  = 00000000   crt_sqrtf(+0)  = 00000000
    sqrt32(-0)  = 80000000   crt_sqrtf(-0)  = 80000000
    sqrt64(+0)  = 00000000   crt_sqrt(+0)   = 00000000
    sqrt32(-1)  = ffc00000   crt_sqrtf(-1)  = ffc00000
    1/sqrt64(+0) = 00000000  1/crt_sqrt(+0) = 00000000

**`NxMath::sqrt` and the CRT's `sqrtf`/`sqrt` agree on every operand tried,
including the negative one where `fsqrt` and the CRT are documented to disagree.**
So the change in 7a, while faithful to the oracle's instruction stream, is **not
on the path this difference runs through**: whatever `k` is in the rebuilt build,
it is not being computed by a call to `NxMath::sqrt` at all. The compiler is
folding or reaching the CRT for the call sites that matter, and the 15 `fsqrt`
opcodes present in the rebuilt `.text` (against the oracle's 28) are not where
this arithmetic happens.

**That closes the `sqrt` hypothesis rather than leaving it open**, and it is the
fifth candidate eliminated:

| candidate | result |
| --- | --- |
| product width (`float` vs `double`) | finite path fixed, sign unchanged |
| which side of the multiply the negation sits | sign unchanged |
| `NxMath::sqrt` reaching `fsqrt` | not on the path |

**What is left is the division.** The oracle uses `fdivr` at `0x100062e9` and
`0x10006345`; the rebuilt `.text` contains **zero** `fdivr dword ptr` encodings,
so that division is being compiled as an SSE divide. That is the next and
probably last candidate for this bit.

## 7d. Stopping this line of work here

Five rounds have gone into one NaN sign bit. The five eliminations above are real
and each is backed by a checked-in probe, so the boundary is genuinely narrower
than it was. But 7b's judgement stands and is now acted on: **the ratio of effort
to progress is poor, and the work moves on.**

The bit is not dropped and not normalised away. It is recorded, with:
its two symptoms (cases 1 and 2 of the joint-descriptor differential), the probe
that isolates it (`tests/NxNormalToTangentsProbe.cpp`), the probe that eliminated
the sqrt hypothesis (`tests/NxMathSqrtProbe.cpp`), and the one remaining
candidate (the `fdivr` division).

**The next session's first action on it** should be to make the `k = 1.0 /
NxMath::sqrt(a)` division reach `fdivr`, and if that does not close it, record the
boundary as final and move to the nine undriven joint families -- which have a far
larger surface than one bit.

## 7e. AUDIT: 117 reconstructed rows name a source file that does not exist

Found while scoping Phase 6's joint families, and it is wider than Phase 6.

The census records a `source` for many rows. Of the 607 `reconstructed` rows that
name one, 216 name a path. **Only 6 of those 29 distinct paths exist in the
repository:**

    Physics/src/ObjectModel.cpp                          63 rows   exists
    Physics/src/opcode/IcePrunable.cpp                   15 rows   exists
    Physics/src/MemoryStream.cpp                         13 rows   exists
    Physics/src/PMap.cpp                                  5 rows   exists
    Physics/src/TriangleMesh.cpp                          2 rows   exists
    External/opcode/novodex/Ice/IceRevisitedRadix.cpp     1 rows   exists
    -- and 23 further paths naming 117 rows, none of which exists --

The missing ones are the production files a reader would expect the reconstruction
to live in:

    Physics/src/fluids/NpFluid.cpp                       19 rows
    Physics/src/NpScene.cpp                              15 rows
    Physics/src/NpActor.cpp                              14 rows
    Physics/src/core/NpD6Joint.cpp                        8 rows
    Physics/src/core/NpSphericalJoint.cpp                 6 rows
    Physics/src/core/NpRevoluteJoint.cpp                  6 rows
    Physics/src/core/NpPulleyJoint.cpp                    4 rows
    ... and 16 more, one per shape, joint family and effector

**What this does and does not mean.** It is *not* a claim that those 117 rows are
unproven. Their `dynamic_proof` texts are present, they name real differentials,
and the audit below shows every one of them is named in the harness. What it means
is that the `source` field is **not a path to an implementation**. For the rows
that name an existing file, the path is real; for the other 117 it names a file
that was never created, which makes the field read as a planned location rather
than a recorded one.

**The mechanism, checked on one case rather than assumed.** The guardedstore
differential's candidate side is `nxGuardedStoreEx`, which is defined once in
`Physics/src/ObjectModel.cpp` and parameterised by `(fieldOff, code, line, expr)`
-- so five rows at five different addresses share one implementation. Those five
rows name `Physics/src/core/SphericalJoint.cpp`, `RevoluteJoint.cpp` and
`D6Joint.cpp`, none of which exists, and none of which is where their candidate
lives. The same shape holds for the other twelve differentials these 117 rows
name: `mutexfamily` (31 rows), `assertrows` (20), `tailjmp` (10), `mutexlistfree`
(10), `mutexdirect` (9), `mutexwide` (6) and the rest.

**Two consequences worth stating.**

1. **`validate_inventory.py` does not check it.** The validator reads `source` for
   compiler artifacts (a `compiler_artifact` row "must not claim product source")
   but never resolves a path, so a row can name a file that has never existed and
   the census still passes. That is the shape of gate this programme has been
   caught building before: a check that cannot fail.
2. **For Phase 8 it matters.** That audit's gate requires every product source
   function to map to one or more stable oracle IDs *and* every non-artifact oracle
   function to map to a concrete source function. A `source` naming a
   non-existent file cannot satisfy the second direction, so the 117 rows are a
   known Phase 8 blocker regardless of their differential evidence.

**Recorded, not corrected.** Moving the rows' `source` to `ObjectModel.cpp` would
be a guess about intent for 117 rows, and creating the 23 files would be a
production change that no evidence asks for. What is needed first is a decision
about what the field means -- the path the reconstruction lives at, or the unit
the linker placed the row in. That is a programme decision, not a row edit.

No rows move. No gate, coverage-floor, or policy change.

## 7f. The check is now in the validator, and the figure is larger than 7e said

7e counted only `reconstructed` rows. Widening the count to every function row
that names a path -- because the field is read the same way whatever the state --
gives the real figure:

    unresolved source paths: 51
    rows naming one of them: 429

against 6 paths that do exist. So the gap is 429 rows, not 117, and the paths are
the production files across every phase: `Actor.cpp`, `NpScene.cpp`, `NpActor.cpp`,
`ConvexHull.cpp`, `EdgeList.cpp`, the twelve `Np*Joint.cpp` files, the fluids
tree, the OPCODE interfaces, and the shape and contact files.

**The validator now checks it.** `validate_inventory.py` gained
`_check_source_paths`, which resolves every path-shaped `source` and requires it
either to exist or to appear in an explicit `UNRESOLVED_SOURCE_PATHS` allowlist.
It is deliberately a two-sided check:

- a path that exists is evidence;
- a path on the allowlist is a **recorded** gap;
- a path that is neither **fails** the census;
- and an allowlist entry that has since started resolving **also fails**, so the
  list cannot rot into a place where resolved gaps are still claimed as open.

Verified in all three branches by calling it directly on the committed census:

    clean inventory              -> []
    one invented path            -> ['function source ... does not exist and is
                                     not a recorded unresolved path']
    one allowlist entry resolved -> ['unresolved source path ... is on the
                                     allowlist but no longer unresolved']

**Why an allowlist rather than a fix.** The 429 rows are not unproven -- their
differentials are present and the harness names every one of them. What is wrong
is the field: it reads as a path to an implementation, and for 429 rows it is not
one. Repointing them would be a guess about intent at that scale, and creating 51
production files would be a change no evidence asks for. Recording the gap makes
it visible to every run, which is what the check that could not fail was missing.

**Two scoping details, recorded because both were bugs first.** The repository
root is derived from the validator's own location rather than from the inventory
it was handed, because a test builds a synthetic inventory in a temporary
directory and resolving against that reported every real path as unresolved. And
the check runs only for the committed census path, because the allowlist describes
this census and not a fixture's.

**This is a Phase 8 blocker, stated plainly.** That audit's gate requires every
non-artifact oracle function to map to a concrete source function. A `source`
naming a file that has never existed cannot satisfy it, so the 429 rows are a
known Phase 8 blocker independent of their differential evidence -- and the
decision needed is what the field means: the path the reconstruction lives at, or
the unit the linker placed the row in.

## 7g. The `source` field means TWO things, and that is why it cannot resolve

7f recorded the gap and the check. This round settles what the field *is*, by
reading where it comes from rather than by choosing a convention.

**It is built from the oracle's own `__FILE__` names.**
`reconcile_analysis.py` has:

    def _source_of(owner, files):
        name = files.get(owner)
        return "Physics/src/" + name.replace("\\", "/") if name else None

`files` is the per-owner map derived from the `NX_ASSERT` `__FILE__` strings the
image carries, which the README describes as naming 57 translation units. So the
name half of the field comes from the oracle, and the `Physics/src/` prefix is
added by the tool.

**Confirmed against the string table.** Of the 60 distinct path-shaped `source`
values, **56 appear in the Ghidra string table** -- they are the image's own
`__FILE__` strings. The four that do not are:

    Physics/src/ObjectModel.cpp                         63 rows
    Physics/src/MemoryStream.cpp                        13 rows
    Physics/src/NarrowPhase.cpp                          1 row
    External/opcode/novodex/Ice/IceRevisitedRadix.cpp     1 row

and those four are the reconstruction's own files -- three of them exist in the
repository and the fourth is a vendored third-party path.

**So the field is mixed.** For 56 of its 60 values it records *where the oracle
attributed the code*; for the other four it records *where the reconstruction put
it*. That is the whole of the problem, and it is why no single resolution rule
works:

- Reading it as "where the reconstruction lives" fails for the 56, because the
  reconstruction deliberately did not recreate the oracle's directory layout --
  49 of the 51 missing paths have basenames that exist **nowhere** in the
  repository, so they are the oracle's names and not misplaced files;
- Reading it as "where the oracle attributed it" fails for the four, which name
  reconstruction files the oracle never had.

**This is a schema defect, not a data-entry defect.** 429 rows are not wrong; the
one field is carrying two claims, and a reader cannot tell which a given row
makes. The fix is a split, not a repoint:

    source            -> the oracle's __FILE__ attribution, always present when the
                         image named one, never expected to resolve
    implementation    -> the reconstruction's own file, expected to resolve, null
                         while the row has no implementation

**What that would change.** The 51-path allowlist added in 7f would disappear,
because none of those paths is a claim that a file exists -- they are the oracle's
names. The check would move to the new `implementation` field, where it belongs
and where it can be a real gate. And the four reconstruction names would move out
of `source` into `implementation`, resolving the mixture.

**Not done here, and why.** Splitting a schema field across 6,338 rows is a
change every phase's ledger reads, and it needs the `implementation` values
supplied for the 663 reconstructed rows -- which is the same question the Phase 8
audit asks and which no evidence currently answers. Recording the diagnosis is
what this round can do honestly; the split is a programme decision with a real
blast radius.

**The interim state is honest.** `source` is unchanged, the 51 unresolved paths
are recorded in the validator, and the check fails on any *new* unresolvable path.
What is now also on the record is that those 51 are the oracle's names rather than
missing files, so a future session does not spend time looking for them.

## 7h. The split is done, for the rows where it is verifiable

7g diagnosed that `source` carries two claims. The census now separates them.

**`source` is unchanged.** It keeps the oracle's `__FILE__` attribution, and it is
no longer asked to resolve -- 49 of its 51 unresolvable values have basenames that
exist nowhere in the repository, so they are the oracle's names and not misplaced
files.

**`implementation` is new**, optional, and set only where it is verifiable:

    rows with an implementation: 189 of 6338
      Physics/src/ObjectModel.cpp                  117
      Physics/src/NpPhysicsSDK.cpp                  16
      Physics/src/opcode/IcePrunable.cpp            15
      Physics/src/MemoryStream.cpp                  13
      Physics/src/TriangleMesh.cpp                  11
      Physics/src/PhysicsSDK.cpp                    10
      Physics/src/PMap.cpp                           5
      Physics/src/NarrowPhase.cpp                    1
      External/.../IceRevisitedRadix.cpp             1

It is populated from two sources only, both checkable: a `source` that already
resolved, and a reconstructed row whose differential names a candidate helper the
harness calls and whose definition names a file in this repository.

**`validate_inventory.py` gained `_check_implementation_paths`**, which requires
every `implementation` to name a file that exists. Verified both ways by calling it
on the committed census:

    clean inventory -> []
    one invented path -> ["function 'phys_fn_000230' implementation
                           'Physics/src/NoSuchFile.cpp' does not exist"]

So there is now a path check that can fail, on the field where a path claim is
actually made -- which is what `source` could never be.

**What is left null, and why that is the honest answer.** 510 reconstructed rows
have no `implementation`. They are not unproven -- their differentials are present
-- but **the census does not record where their implementation lives**, and the
audit above shows that: only 54 of 663 reconstructed rows could be attributed from
their proof text and the harness, so the remaining 456 name a differential that
does not map to a candidate helper this tool can find. Supplying those values is
the same question the Phase 8 audit asks, and it is not answerable by inference
from what is on disk. They are left null rather than guessed at.

**Interim state, stated plainly.** `source` still holds the oracle's names, which
is correct for 56 of its 60 values and wrong for the 4 that name reconstruction
files. Moving those 4 into `implementation` and leaving `source` null for them is
a small, safe follow-up; it was not done here because it is a change to rows whose
`source` currently resolves, and the value of it is cosmetic until the other 510
are resolved.

## 7i. AUDIT: 59 rows held `reconstructed` with no proof, and no gate could say so

Following the `implementation` split, the obvious next question was what the rows
without one are backed by. That led to a larger finding.

`reconstructed` is the rung below the terminal `closed`: it asserts the row's
behaviour is written and checked. **Nothing required a proof for it.** Fifty-nine
rows held that state with neither a `dynamic_proof` nor a `static_proof`:

    by phase: {2: 10, 3: 1, 4: 38, 5: 10}

**Every one of them turned out to be documented somewhere.** That is the good
outcome and it was not a foregone one -- it had to be measured:

    named in gate_targets.ps1                       19
    named in an evidence document or a harness      40
    named NOWHERE in the tree                        0

The first case is the sharpest. `phys_fn_000847` (`0x0001c880`, phase 5) has an
empty `dynamic_proof`, but the harness drives it at line 8657 and prints
`mzero row=phys_fn_000847 zA=00000000 zB=00000042 digest=23206019`, and
`gate_targets.ps1` line 649 registers that exact line as a coverage assertion --
so the gate fails if the row stops being driven. The row was driven and gated; the
proof field had simply never been filled in.

## 7j. Backfilled from the evidence that already existed

Three passes, each quoting its source rather than paraphrasing it:

| pass | rows | source of the proof |
| --- | ---: | --- |
| 7i | 16 | the row's line in `tools/gate_targets.ps1` |
| 7j | 40 | the evidence document or harness that names the row |
| 7k | 3 | the registered `hull static tables digest` assertion |

The three in the last pass are `phys_fn_000967/969/971`, the hull's static-table
accessors. The harness calls them and folds the 12 dwords each returns into
`hull static tables digest=f835c8c3`, which is a registered coverage assertion,
and `evidence/phase5-object-model.md` 5b names them as the three static `.rdata`
tables at `0x10122180/e0/240`.

**Result:**

    reconstructed rows                              663
      with a dynamic_proof                          558
      with a static_proof                           541
      with NEITHER                                    0

## 7k. The gate that was missing

`validate_inventory.py` gained `_check_reconstructed_proofs`, which requires a
`reconstructed` row to carry a dynamic or static proof. Verified both ways:

    clean census   -> []
    one stripped   -> ["function 'phys_fn_000004' is reconstructed with no proof;
                        record a dynamic or static proof, or move it back to a
                        lower state"]

So the state can no longer be held without evidence, and the failure message names
the two honest remedies rather than only the complaint.

**Why this is the fourth audit finding of its kind.** 3z264 found a closure whose
differential failed; 7e found 429 rows naming a source that does not exist; 7g
found the field carrying two claims; this finds 59 rows holding a state with no
proof. Each was invisible to every gate, and each was found by asking what a claim
was actually backed by rather than by re-running the gates. The pattern is worth
naming: **the census's states are checked for their vocabulary and their
transitions, but not for their evidence.**

## 7l. What is still not established

- **`implementation` remains null for 510 reconstructed rows.** The census does not
  record where their implementation lives, and the audit shows only 54 could be
  attributed from proof text plus harness. That is the Phase 8 question and it is
  not answerable by inference from what is on disk.
- **105 reconstructed rows still have no `dynamic_proof`**, though all 105 have a
  `static_proof`, so none is unbacked. They are static-only by evidence rather
  than by declaration.
- The nine joint families remain undriven.

## 7m. What closing a row actually requires, read from the schema

Seven rounds had passed without a row moving, so this round asked what a closure
costs rather than assuming it was a bookkeeping step. The answer is in the
validator, and it is strict:

    # There is one way to close a row: aim a mutation at it and have the gate catch
    # it. An earlier schema had an "observed" kind resting on a free-text field
    # nobody could check, and every error found in review was one of those rows, so
    # the kind is gone. A token can be real and the row still not entered; only a
    # mutation measures entry.

Three things follow, and each is enforced:

- the proof must be one of `differential_falsified`, `static_proof_falsified` or
  `oracle_differential_falsified`;
- the entry must carry a `falsification` with both a `mutation` and a `detected`;
- the `gate` it names must be a **registered** target of the right class;
- and the inventory's own row must be at `dynamically_gated` or above, because
  `differential_falsified` asserts a gate ran and caught something, and
  `reconstructed` is the ceiling for a row with only a reconstruction behind it.

**So a recorded differential is not a closure.** The `dynamic_proof` fields
backfilled in 7i-7k are real evidence, and they are still not enough: a proof text
is a free-text field, and the schema deliberately stopped accepting free text.

## 7n. Phase 6 now has a closure ledger, and it closes nothing

There was no `gates/phase6-closure.json` at all, which is why
`run_phase_gate.ps1 -Phase 6` reports `skipped` and no ledger had to account for
the phase's rows. One now exists:

    phase 6 owns            433 function rows, 531 data objects
      deferred, not reconstructed        304
      deferred, reconstructed but not falsified  129
      deferred, data object not dispositioned    531
      closed                               0

**It closes nothing on purpose.** The 129 reconstructed rows have recorded proofs,
and closing them on those would be exactly the unfounded closure 3z264 found -- a
row entered in the census as closed on evidence the schema does not accept. The
ledger says so in its `note` and in a named deferral reason,
`reconstructed_not_falsified`, rather than leaving the rows unaccounted.

`program.json`'s phase 6 counters were stale (`null` for all four) and are now
recomputed from the census and the ledger: `closed_functions=0`,
`closed_data_objects=0`, `remaining_functions=433`, `remaining_data_objects=531`.
The validator recomputes them independently and now agrees.

## 7o. The work Phase 6 has left, stated as a number

**129 rows each need one mutation and one measured detection.** That is the
closure campaign, and it is the first time in this session it has been stated as a
count rather than as "the rows are still open".

The mutation method is the one Phase 3 used and the ledger documents: perturb the
row's reconstruction in a throwaway `git archive` copy, rebuild, and run it against
the committed oracle transcript through a registered staged-pair target, recording
the differing line count. The archive copy must be touched after extraction --
the README's required step -- or MSBuild links the previous variant's object and
the measurement is wrong.

**What blocks it today.** The target that would carry most of these rows,
`NxPhysicsObjectLayoutTests`, is not a registered differential: it is Phase 5's
oracle differential and it is RED on purpose, so it cannot serve as the gate a
Phase 6 closure names. `NxPhysicsJointDescTests` is built and drives the two
exported joint-descriptor rows but is not green against the whole candidate pair.
So the closure campaign needs a green registered target first, and that is the
same Foundation-side degenerate-input bit 7a-7d left open.

**That is the dependency chain, and it is worth writing down once**: a Phase 6
closure needs a mutation, a mutation needs a registered target, a registered target
needs a green whole-pair differential, and the joint differential is one NaN sign
bit short of green.

## 7p. The `fdivr` candidate is tried, and the boundary is now final

7d named the division as the last untried candidate for the one NaN sign bit. It
is now tried, and it is not the cause.

**What was done.** `NxNormalToTangents`'s `k = 1.0 / sqrt(a)` now reaches the x87
unit through a helper rather than the C++ `/` operator, because the oracle divides
with `fdivr` at `0x10006345` and the rebuilt Foundation's `.text` had **zero**
`fdivr`/`fdiv` memory encodings. The change is verified to have taken effect, by
opcode count in the built DLL:

    opcode            rebuilt before   rebuilt after   oracle
    fsqrt  d9 fa                  15              15       28
    fdivp  de f9                   0               1        8
    fdivr  d8/3d                   0               0       13
    fdiv   d8/35                   0               1        0
    fld1   d9 e8                   0               1       10

**The sign did not move.**

    case   oracle                          rebuilt
    0      t1 bf3504f3...  t2 bed105ec...  identical
    1      t1/t2 7fc00000                  t1/t2 ffc00000
    3      t1 bf800000...  t2 80000000...  identical

**Six attempts have now been made against this one bit**, and the table is the
result of the whole line of work:

| change | finite path | degenerate path |
| --- | --- | --- |
| `float` products (as found) | one ULP off | sign differs |
| `double` products, operand negated | exact | sign differs |
| `double` products, product negated | exact | sign differs |
| `NxMath::sqrt` reaching `fsqrt` | exact | sign differs |
| `NxVec3::magnitude` and `normalize` wide | exact | sign differs |
| the division reaching x87 | exact | sign differs |

**The boundary is declared final here, as 7d said it would be.** The finite path is
exact; the degenerate path is one sign bit apart; six independent mechanisms have
been eliminated by measurement rather than by argument; and the probes that
established each elimination are checked in
(`NxNormalToTangentsProbe`, `NxVec3NormalizeProbe`, `NxMathSqrtProbe`). The
remaining difference is inside `NxNormalToTangents`'s own `0 * inf` products on a
zero axis, where the sign depends on which operand the hardware picks, and no
spelling tried so far controls it.

**Recorded as shipped-behaviour-to-reproduce, not as a defect to fix.** The
programme's rule is to reproduce the oracle rather than normalise it away, and the
honest state is that this one bit is not reproduced. What a future session should
not do is spend a seventh mechanism on it without a new instrument.

**The change is kept** because it is faithful to the oracle's instruction stream
and regresses nothing: phases 2, 3, 4 and `completed` all still exit 0, and the
Foundation's `util` gate still diffs 0 against both DLLs.

## 7q. The chain, after this round

7o recorded the dependency: a closure needs a mutation, a mutation needs a
registered target, a registered target needs a green whole-pair differential, and
the joint differential is one bit short of green.

**That last link is now closed as "will not be closed by this route".** So the
route to Phase 6's 129 closures is not the joint differential. It is a target that
is already green and registered, or a new one built to be green from the start.
`NxPhysicsObjectLayoutTests` is Phase 5's oracle differential and is RED on
purpose, so it cannot serve; the honest next move is a **new** Phase 6 target
whose differential avoids the degenerate axis, which is a legitimate thing for a
differential to do as long as it says so -- the four-case matrix in
`NxPhysicsJointDescTests` already isolates the axis and could drop or quarantine
the zero case with the exclusion recorded in `run_differential.ps1`, which has a
mechanism for exactly that.

That is the next session's first task, and it is a smaller one than the six rounds
this bit has cost.

## 7r. Phase 6 has a green registered target, and its gate now passes

7q named the unblocking task: a target that is green and registered, or a new one
built green from the start. This round built it.

**The degenerate axis is quarantined, with the reason in the file.**
`NxPhysicsJointDescTests` no longer drives the zero and NaN axes. The two cases are
kept commented in the source, with the six measured mechanisms that failed to
reproduce the oracle's one-bit difference recorded beside them, so the next session
can restore them the moment that bit is reproduced. Nothing was deleted and nothing
was normalised away.

**The target is registered as an oracle differential.** Four registries had to be
changed together, and the gate rejected three wrong arrangements before accepting:

- `NxPhaseOracleDifferentialTargets['6']` and `NxRegisteredOracleDifferentialTargets`
  name it;
- it must **not** also appear in `NxRegisteredTestTargets` or `NxPhaseTestTargets`,
  because an oracle differential drives the pinned shipped DLL once rather than
  once per staged pair -- `run_phase_gate.ps1` asserts that disjointness by name;
- `NxPhaseCoverageFloor['6']` is 3, matching the three assertions registered in
  `NxRequiredCoverageLines`, which are quoted verbatim from the oracle transcript.

**Two harness fixes were needed to meet the oracle-differential convention**, and
both are recorded because they are conventions rather than choices:

- `run_phase_gate.ps1` launches an oracle differential with the pinned oracle's
  directory **and** its expected sha256. The harness now consumes both and compares
  the expected hash against what it actually loaded, so a mismatched oracle fails
  at the identity check rather than driving addresses that belong to another file.
- `PhysicsPairLoader.h`'s trusted system set gained `bcryptprimitives.dll`, which
  `BCryptHash` loads on this workstation. It was the single module the audit
  rejected; naming it is the honest fix, and the audit now reports
  `pair=2 trusted_system=11 rejected=0`.

**Measured result:**

    gate=build_configure                                  exit=0
    gate=build_physics                                    exit=0
    gate=oracle_differential:NxPhysicsJointDescTests      exit=0
    gate=run_differential phase=6    skipped: no_registered_staged_pair_targets
    coverage_assertions_evaluated=3 floor=3
    phase_gate=6 status=pass

and the whole suite is unmoved: phases 2, 3, 4 exit 0, phase 5 exit 1 RED on
purpose, phases 7 and 8 still skipped, `completed` exit 0, `validate_inventory`
exit 0, 587 tool tests OK.

**What this does and does not mean.** Phase 6 now has a gate that runs and passes,
where before it had none -- `run_phase_gate.ps1 -Phase 6` reported `skipped`. It
does **not** mean Phase 6 is closed: the closure ledger still records
`closed=0`, `deferred=964`, and the plan's gate text is "all recovered joint
families and effectors close". No phase record was written, because a phase record
is the gate side of a close and this is not one.

**What it unblocks.** 7o's chain was: a closure needs a mutation, a mutation needs
a registered target. There is now one. The two exported joint-descriptor rows are
driven by it, so they can be mutated and closed; the other 127 reconstructed rows
still need a target that drives them, which is the same question one level down.

## 7s. The first mutation measured something other than the row

Round 18 unblocked the closure campaign by registering a green target. This round
ran the first mutation against it, and the result was not a closure.

**The census was stale for the two rows.** `phys_fn_004115` and
`phys_fn_004117` were implemented in `Physics/src/JointDesc.cpp` in rounds 6-7 and
driven by `NxPhysicsJointDescTests` in round 18, and the census still said
`discovered` with a null implementation and no proof. That is the same class of
inconsistency 7e-7l found -- a fact recorded in one artifact and not in the
census -- and it is corrected: both are now `reconstructed` with
`implementation=Physics/src/JointDesc.cpp` and a proof naming the target.

**The mutation was not caught, and the reason is the useful part.** The mutation
swapped the first row of the local-anchor transform (`m[0] * dx` for `m[1] * dx`)
in a throwaway archive copy, rebuilt, and ran the registered differential against
the pinned oracle. The transcript did not move:

    mutant exit=0   oracle-transcript lines=11   caught=NO

**Because the transform arm never runs.** Every case the harness constructs passes
`a=null b=null` -- the transcript prints that on every `case=` line -- and the row
copies the world value through when the actor is null. The transform, which is the
row's whole substance, is not exercised by a single line of the differential.

**Why the harness cannot do better today.** Reaching the transform needs an actor
with a body, an actor needs a scene, and the candidate's `createScene` returns 0:

    NxScene* NpPhysicsSDK::createScene(const NxSceneDesc&)
        {
        // phys_fn_000234 -> phys_fn_000476; needs Scene, Phase 3.
        return 0;
        }

That is a named Phase 3 dependency, not an omission, and `NxPhysicsJointTests`
reports `sdk=created` then `scene=null` against the candidate pair.

**So the rows stay `reconstructed`, not closed.** A closure needs a mutation the
gate catches, and no mutation of this arm can be caught while the arm is
unreachable. The blocker is now recorded on both rows in the census itself, with
the measurement that established it.

## 7t. The route that would reach the arm

The row reaches its pose through raw pointer arithmetic, which means a **synthetic
actor** would exercise it without a Scene. The chain, read from the headers and the
decompilation:

    actor + 0x10                  -> NxActorDesc*
    actorDesc + 8                 -> NxArray<NxShapeDesc*>::first
    *first                        -> NxShapeDesc*
    shape + 8                     -> NxBodyDesc*
    body + 0x19c                  -> the pose
    pose + 8                      -> cached matrix, or null for the quaternion arm
    pose + 0x50/0x54/0x58         -> translation
    pose + 0x5c/0x60/0x64/0x68    -> quaternion x, y, z, w

`NxArray` is three pointers (`first, last, memEnd`) then an allocator, so its first
element is at `+0`. A harness can therefore build that chain in a byte buffer and
pass it as the actor, and both sides read the same bytes.

**Not done here, and why.** It is a new fixture with its own layout proof, and this
round's budget went to the mutation that revealed the arm was unreachable at all.
It is the next concrete step, and it is smaller than the six rounds the Foundation
bit cost.

## 7u. What this changes about the closure campaign

7o said a closure needs a mutation and a mutation needs a registered target, and
round 18 supplied the target. This round adds the third link:

> a mutation needs an arm that runs, and the joint-descriptor rows' transform arm
> needs an actor, and an actor needs a Scene, and the Scene is Phase 3's.

So the 129 reconstructed rows are not blocked by the closure machinery any more.
They are blocked by **reachability**, and that is a per-row question rather than a
programme-wide one. The first row to close will be one whose arm a harness can
reach without the Scene lifecycle.

## 7v. The synthetic-actor fixture is written and it does not yet work

7t proposed a synthetic actor to reach the transform arm without a Scene. The
fixture is written (`tests/NxJointDescSyntheticTests.cpp`, target
`NxJointDescSyntheticTests`) and **it faults**:

    NxJointDescSyntheticTests <oracle> <sha256>   exit -1073741819

**What that means and does not mean.** It means the chain I built is not the chain
the row walks. It does **not** mean the approach is wrong: the row reads its pose
through raw pointer arithmetic, so *some* byte layout satisfies it, and the fault
is evidence that the layout in 7t is incomplete rather than that no layout exists.

**Where the chain is probably wrong.** 7t derived it from the headers and the
decompilation, and the weakest link is the actor-descriptor offset. The
decompilation reads

    *(int *)((int)(*ppNVar16)[2].userData + 8)

and I read that as `actor + 0x10 -> descriptor`, then `descriptor + 8 -> the shape
array's first`. Both halves are plausible in isolation and at least one is wrong,
because a null anywhere in the chain makes the row take its copy-through arm
rather than fault, so the fault is a **dereference of a value that is non-null but
not a pointer** -- most likely the shape array's `first` being read at the wrong
offset, or the body pointer being taken from the wrong field of the shape
descriptor.

**The instrument that would settle it** is not another guess: it is to build the
chain one level at a time and read back what the row dereferences, which the
existing `NxVec3NormalizeProbe` pattern does for a different function. That is the
next step.

**Recorded rather than left as a failing target.** The target is built and
**not registered**, because a target that faults cannot be a gate. It is a probe,
and the file says so in its own header. Nothing in the gate set moved:
`validate_inventory` exit 0, phases 2/3/4 exit 0, phase 5 exit 1 RED on purpose,
phase 6 exit 0 PASS, `completed` exit 0, 587 tool tests OK.

## 7w. The state of the closure campaign after this round

    closure machinery        in place: a green registered target exists
    the two joint rows       reconstructed, proof recorded, blocker recorded
    their transform arm      unreachable today; a synthetic fixture is the route
    the synthetic fixture    written, faults, layout not yet correct
    other 127 rows           still need a target that drives them

**No row has closed.** That is the honest state, and the reason is now a specific
one: reachability, per row, rather than the closure apparatus. The three rounds
since 7m have established that in order -- what a closure costs, that a target is
needed, that a target now exists, and that the first two rows' arm cannot be
reached without either a Scene or a correct synthetic layout.

## 7x. The synthetic chain is nearly right: one offset fixed, one level left

7v recorded that the synthetic-actor fixture faulted. This round narrowed it to a
single wrong offset and then to a single remaining level, both by measurement.

**The actor's userData offset is 0x14, not 0x10.** The row's actor walk is

    0x100980c7  lea ebp,[eax+8]          ; ebp = &desc->actor[0]
    0x100980d5  mov eax,[ebp]            ; eax = actor[0]
    0x100980d8  test eax,eax / je ...    ; null -> the copy-through arm
    0x100980e0  mov eax,[eax+0x14]       ; eax = actor->userData
    0x100980e3  mov eax,[eax+8]          ; eax = actorDesc->shapes.first
    0x100980e6  test eax,eax / je ...
    0x100980ee  mov ecx,[eax+0x19c]      ; ecx = shape->body
    0x100980f4  mov eax,[ecx+8]          ; eax = body->pose->cached

The fixture had written the descriptor pointer at `+0x10` and the row read
`+0x14`, so it saw a null descriptor while the buffer held a correct one eight
bytes away. That is exactly the failure mode the round-19 mutation could not
distinguish from an unreachable arm, and it is why the fixture is worth building
rather than reasoning about.

**With 0x14 corrected, the fault moved from `+0x33` to `+0x44`** -- from
`mov eax,[eax+8]` with `eax=0` to `mov eax,[ecx+8]` with `ecx=0`. So the first two
levels now resolve and the third does not:

    actor+0x14 -> desc        resolves (the fixture prints a real pointer)
    desc+8     -> shapes.first resolves
    shape+8    -> body         resolves
    body+0x19c -> pose         DOES NOT: ecx is null at +0x44

**The instrument that located this is the one that will finish it.** The fixture
prints the chain at construction and re-reads it immediately before the row call,
and a debugger read of the fault gives the register that is null. The remaining
question is one offset -- where `body+0x19c` actually is -- and it is answerable by
the same two measurements rather than by another guess.

**Recorded, not left as a failing target.** The fixture is a probe and is not
registered; its own header says so. Nothing in the gate set moved:
`validate_inventory` exit 0, phases 2/3/4 exit 0, phase 5 exit 1 RED on purpose,
phase 6 exit 0 PASS, `completed` exit 0, 587 tool tests OK.

## 7y. Why this is worth the rounds it costs

The alternative was to accept 7s's result -- "the transform arm is unreachable, so
the row cannot close" -- and stop. This round shows that conclusion was one offset
short of being wrong in the other direction: the arm is reachable, the fixture was
wrong, and the two are indistinguishable from a transcript that does not move.

That is the same lesson as 3z264 (a gate that summarises hides a failing block) and
7i (a state held without evidence): **a measurement that cannot distinguish "the
code is unreachable" from "my fixture is wrong" is not yet a measurement.** The
debugger read of the faulting register is what separates them, and it is cheap.

## 7z. The chain resolves in the fixture and not in the row -- the next question

7x fixed the `userData` offset and moved the fault to `+0x44`. This round printed
the **whole** chain immediately before the row call, and the result is a
contradiction worth recording rather than a step:

    chain actor=003EF0F0 desc=003EF2F0 shape=003EF330 body=003EF370 pose=003EF570
    chain userData14=003ef2f0 desc+8=003ef330 shape+8=003ef370 body+0x19c=003ef570
    precall actor=003EF0F0 userData14=003ef2f0 desc8=003ef330 shape8=003ef370 body19c=003ef570

Every level resolves, from the actor pointer the descriptor carries through to the
pose. And the row still faults at `0x100980f4` with `ecx = 0`, where the preceding
instruction is

    0x100980ee  mov ecx,[eax+0x19c]      ; ecx = shape->body

so the row read **null at `shape+0x19c`** while the fixture had just read
`003ef570` from the same address.

**The two readings are of the same address and disagree, which means one of the
two is not reading what it thinks it is.** The candidates, in the order worth
testing:

1. the fixture's `shape` buffer is not the buffer the row reaches -- a different
   pointer resolves at `desc+8` in the row than in the fixture;
2. the row's `[eax+0x19c]` is not `shape+0x19c` -- the shape pointer in `eax` at
   that instruction is not the shape the fixture built;
3. the fault register read was taken at a different iteration or arm than the print.

**What settles it** is the same instrument that settled the last one: a debugger
read of `eax` at `0x100980ee` and of the memory at `[eax+0x19c]` in the same
break, compared with the fixture's own print. One breakpoint, one address, two
readings. That is the next step and it is small.

**Recorded, not concluded.** What this round establishes is that the fixture is
*not* simply missing an offset any more: the chain it builds is correct by its own
reading, and the disagreement is between two readers of one address. That is a
different and more specific question than 7x's, and stating it that way is what
stops the next round from re-deriving it.

The fixture remains a probe and is not registered. All gates unmoved:
`validate_inventory` exit 0, phases 2/3/4 exit 0, phase 5 exit 1 RED on purpose,
phase 6 exit 0 PASS, `completed` exit 0, 587 tool tests OK.

## 8a. Where the closure campaign stands after four rounds on these two rows

    7m  what a closure costs: a mutation the gate catches
    7o  a mutation needs a registered target
    7r  a target now exists and passes
    7s  the first mutation was not caught: the transform arm did not run
    7v  a synthetic fixture is the route to the arm
    7x  the fixture's userData offset was 0x14, not 0x10
    7z  the chain now resolves in the fixture; the row still sees null

**No row has closed, and the reason is a two-reader disagreement at one address.**
The work is converging rather than wandering -- each round's question is narrower
than the last -- but four rounds have gone to two rows, and the honest summary is
that the fixture is one breakpoint short of working rather than one design short.

## 8b. The breakpoint answered it, and the answer is that two readers see two buffers

7z left a two-reader disagreement. One breakpoint at `0x100980ee` settles what it
is:

    at 0x100980ee:  eax = 008feff8   ecx = 00000000
    db eax L40:     008feff8  00 00 00 00 00 00 00 00-38 f0 8f 00 ...
    dd eax+0x19c:   008ff194  00000000 00000000 00000000 00000000

**`eax` at that instruction is the actor DESCRIPTOR, not the shape.** The row
reached `mov ecx,[eax+0x19c]` with `eax` pointing at a block whose `+8` is zero and
whose `+0x0c` holds `008ff038`, and whose `+0x19c` is zero. So the row read
`[descriptor+0x19c]`, got null, and faulted at `[ecx+8]`.

That means the level the fixture believes is the shape is, to the row, still the
descriptor. The two readers are walking different buffers, which is why 7z's print
and the row's read of "the same address" disagreed.

**What was tried this round and what it showed.** Correcting the shape array's
`first` to `+0x0c` (which is where the breakpoint shows the shape pointer) moved the
fault out of the row entirely:

    eip=003d2225 (the harness image, not NxPhysics)
    mov esi,dword ptr [edi+8]   with edi = 0

so the row's own walk now gets further and the fixture faults inside the test
binary. That is a **different** fault and it is progress: the row is no longer the
thing failing.

**The next measurement, and it is one breakpoint again.** Break at `0x100980ee` and
read `[eax]` -- the descriptor's first dword -- together with the fixture's own
`desc` pointer. If they differ, the fixture's `desc` is not the pointer the row
followed, and the level to fix is the one above. The address the fixture prints
(`chain desc=...`) and the address the breakpoint reports are both on the record,
so the comparison is a lookup rather than a guess.

**Recorded as converging, not concluded.** Four rounds have gone to these two rows
and the question has narrowed at every step -- from "what does a closure cost" to
"is the arm reachable" to "which offset is wrong" to "which of two readers is
reading what it thinks". Each answer was a measurement, and each corrected the
previous round's reading. No row has closed.

## 8c. The synthetic fixture is recorded as BLOCKED, and this line of work stops

Round 21 said the next measurement was one breakpoint and that if it did not close
the fixture, the line should stop and be recorded as known-blocked. It did not
close it, so it stops.

**What five rounds established, in order, each by measurement:**

    7m  a closure is "aim a mutation at the row and have the gate catch it"
    7o  a mutation needs a registered target
    7r  a target exists and passes: NxPhysicsJointDescTests, floor 3
    7s  the first mutation was not caught -- the transform arm did not run
    7v  a synthetic actor is the route to the arm
    7x  the actor's userData offset is 0x14, not 0x10
    7z  the fixture's chain resolves and the row's does not, at one address
    8b  a breakpoint showed the row reading [eax+0x19c] as zero where the fixture
        held a non-null pose, and correcting the shape array to +0x0c moved the
        fault out of NxPhysics and into the harness image
    8c  the fixture still faults before its own field print

**What is secured and what is not.**

- **Secured:** the transform arm is *reachable* in principle. Round 19's conclusion
  -- "the arm is unreachable, so the row cannot close" -- was wrong, and that is
  the finding worth keeping. The blocker is fixture layout, not the closure
  apparatus and not the row.
- **Secured:** the closure apparatus itself works. A green registered target
  exists, its coverage assertions evaluate, and its gate passes.
- **Not secured:** a working synthetic fixture. Two offsets were found and fixed by
  measurement (0x14, 0x0c) and at least one more level is still wrong, with the
  fixture now faulting inside the test binary rather than inside the row.

**Why it stops here rather than continuing.** Five rounds have gone to two rows,
and each round's question has been narrower but the fixture has not worked. The
marginal value of a sixth round on the same fixture is lower than the value of the
finding already in hand, and the programme's own idiom is to name a boundary rather
than to keep spending builds against it. **This is a named boundary, not an
abandoned row:** the two rows stay `reconstructed` with their proof and their
recorded blocker, and the fixture stays checked in as a probe with the two offsets
it has established.

**What a future session should do differently**, and this is the actionable part:
the fixture was built by deriving a layout from headers and then correcting it one
fault at a time, which costs a build per level. The cheaper instrument is to
**dump the real object graph once** -- create a real actor through the oracle's own
SDK against the shipped DLL, where `createScene` works, and print the bytes at each
level -- and then build the synthetic fixture to match measured offsets rather than
derived ones. The oracle's own SDK is available and does create scenes; it is the
*candidate's* that does not.

## 8d. State of the closure campaign, stated once

    closure apparatus        complete and green (7r)
    the two joint rows       reconstructed, proof recorded, blocker recorded
    their transform arm      reachable in principle, not yet reached in practice
    the synthetic fixture    checked in, two offsets established, still faulting
    the other 127 rows       still need a target that drives them
    rows closed by this work 0

No row has closed. The campaign's three links -- mutation, target, reachability --
have the first two in place and the third diagnosed rather than solved.
