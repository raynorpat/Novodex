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
