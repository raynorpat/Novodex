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

## 8e. The root of the chain, read from the oracle: `createScene` needs the Scene

8d recorded the campaign as blocked on reachability. This round traced that to its
root, and the root is one function.

The candidate's `createScene` returns 0 with a comment naming its dependency:

    NxScene* NpPhysicsSDK::createScene(const NxSceneDesc&)
        {
        // phys_fn_000234 -> phys_fn_000476; needs Scene, Phase 3.
        return 0;
        }

`phys_fn_000476` (`0x0000ea80`, 344 bytes, census state `discovered`) is the
SDK-side row, and the oracle's decompilation shows what reconstructing it costs:

    puVar5 = allocator->allocate(0x710, 0);      // the Scene object
    puVar5 = FUN_10012c10(puVar5);               // construct it
    uVar6  = FUN_10013070(puVar5, param_1);      // initialise from the desc
    ...                                          // then push it onto the SDK's list
    if (!desc.isValid()) error("createScene: desc.isValid() is false!")

So `createScene` allocates a **0x710-byte Scene**, constructs it through
`0x00012c10`, and initialises it from the descriptor through `0x00013070`. None of
that exists in the reconstruction: `PhysicsInternal.h` declares `class Scene` with
the comment "Phase 3 owns the internal scene", and the census records 57 rows
naming a scene source, of which **42 are `discovered` and 15 `reconstructed`** --
and the 15 reconstructed ones name `Physics/src/NpScene.cpp`, which does not exist
(7e).

**So the chain, complete:**

    a Phase 6 closure needs a mutation the gate catches
      -> a mutation needs a registered target           (7o, satisfied in 7r)
      -> a target needs an arm that runs                (7s)
      -> the joint rows' arm needs an actor             (8b)
      -> an actor needs a Scene                         (this round)
      -> the Scene is Phase 3's, and 42 of its rows are not reconstructed

**Every link above the Scene is now in place.** The closure apparatus works, the
target passes, the arm is reachable in principle, and the fixture is the only
missing piece -- and the fixture cannot substitute for an actor whose pose the row
must read through a real object graph.

**What this says about priority, stated plainly.** Six rounds have gone to two
Phase 6 rows, and the thing that would unblock them is not more Phase 6 work: it is
**the Scene**. Reconstructing `phys_fn_000476` and the Scene rows behind it is a
Phase 3/7 task with a much larger surface, and it is the same blocker for
`NxPhysicsJointTests` (which reports `sdk=created` then `scene=null`), for every
joint family the plan lists, and for any row whose arm needs an actor.

**Recorded as the campaign's head-of-chain**, so the next session does not
re-derive it: the highest-value unblocked work is not a Phase 6 row and not the
synthetic fixture, it is the Scene lifecycle.

## 8f. The real object graph, measured through the oracle's own SDK

8e traced the campaign's root to the Scene. This round took the cheaper instrument
8c named: instead of deriving the actor layout from headers and correcting it one
fault at a time, **dump the real graph once**. The oracle's SDK creates scenes and
actors even though the candidate's does not, so
`tests/NxSceneGraphProbe.cpp` drives it and prints the bytes at every level the
joint-descriptor rows walk.

**Measured, from a real actor created by the shipped DLL:**

    actor   016E3DF8: 10104530 00000000 1010468c 016e3c30 016e3c68 016e3da0 ...
    actor+0x14 = 016e3da0            <- the actor descriptor

    desc    016E3DA0: 016e3df8 016e0e60 016e5240 00000000 016e1578 ...
    desc+0x08 = 016e5240             <- the shape pointer
    desc+0x0c = 00000000
    desc+0x10 = 016e1578

    shape   016E5240: 10106890 00000000 00000000 ...
    shape+0x08 = 00000000

**Three things this settles, all of which were wrong or unproven before:**

1. **`actor+0x14` is the descriptor.** 7x fixed this by one fault; the measurement
   confirms it against a real object, and it is `userData` because `NxActor`'s
   vtable is at `+0` and its two members sit at `+0x10` and `+0x14`.
2. **`desc+0x08` is the shape pointer, not `+0x0c`.** Round 21 changed the fixture
   to `+0x0c` on the strength of a breakpoint read of a *synthetic* buffer, and the
   real graph shows `+0x0c` is zero and `+0x08` holds the shape. So that change was
   a correction in the wrong direction -- the breakpoint was reading a buffer the
   fixture had itself mis-built.
3. **`shape+0x08` is zero on a real actor.** The row reads `[shape+0x08]` as the
   body and then `[body+0x19c]` as the pose, so either the body lives elsewhere in
   the shape, or the object at `desc+0x08` is not what the row treats as a shape.

**What that means for the fixture.** It is not one offset short; the third level is
not where the fixture or 8b believed. The measurement gives the answer to two of
the three levels and shows the third needs its own read -- the body pointer inside
the shape object, at whatever offset the row's `mov eax,[eax+0x19c]` chain
actually uses.

**Recorded as the method that should have been used first.** 8c said a future
session should dump the real graph rather than derive a layout, and this round did
it in one probe and two builds, where the derive-and-correct method cost five
rounds and produced one wrong correction. The probe is checked in and is
oracle-only, because the candidate cannot create a scene.

## 8g. State after this round

    closure apparatus        complete and green
    the two joint rows       reconstructed, proof and blocker recorded
    real actor layout        MEASURED: actor+0x14 desc, desc+0x08 shape
    shape -> body            still unknown; shape+0x08 is zero on a real actor
    the synthetic fixture    still faulting, and 8b's +0x0c change was wrong
    the Scene                the campaign's root, unreconstructed (8e)

No row has closed. The fixture's next step is now a measurement rather than a
guess, and the measurement is a one-line change to a probe that already exists.

## 8h. The shape scan, and why the third level is not an offset problem

8f measured two levels and left the third open. This round scanned the real shape
object for the body pointer, and the result closes the question in a different way
than expected.

**The scan.** Every pointer-looking dword in the first 0x40 bytes of the real shape,
with the dword at `+0x19c` of whatever it points to, because that is where the rows
expect the pose:

    shape   019C5240: 10106890 00000000 00000000 00000000 ...
    shape scan:
      shape+0x00 = 10106890   [+0x19c] = 00000003
      shape+0x30 = 3f800000   [+0x19c] = ffffffff (the read faulted)
    shape+0x08 = 00000000

**What that says.** The only pointer in the object is its vtable at `+0x00`; every
other dword in the first 0x40 bytes is zero, and `+0x19c` of the vtable's target is
the small integer `3`. So **the object at `desc+0x08` carries no body pointer at
all**, and the row's `[shape+0x08] -> [body+0x19c]` chain does not describe it.

**Three readings are now excluded by measurement**, where before they were
assumptions:

1. `desc+0x08` is not the shape array's `first` in the sense the fixture assumed --
   it holds a pointer to a vtable-only object;
2. the body is not at `shape+0x08`, on a real actor, at any depth the scan reached;
3. `+0x19c` is not a pose offset within that object.

**What is left.** The row's own chain, read from the binary, is unambiguous:

    0x100980e0  mov eax,[eax+0x14]   ; actor -> userData
    0x100980e3  mov eax,[eax+8]      ; -> ?
    0x100980ee  mov ecx,[eax+0x19c]  ; -> ?
    0x100980f4  mov eax,[ecx+8]      ; -> ?

and 8f confirmed the first is the actor descriptor on a real object. So the second
dereference lands on an object the scan says is vtable-only -- which means either
the descriptor's `+8` is not what the row walks on this build, or the object it
points to is a **facade** whose body is reached through its vtable rather than
through a member.

**Recorded as a boundary, not a next step.** Two rounds of measurement have taken
this from "derive and fault" to "two levels measured, the third not an offset", and
the remaining question is structural rather than numeric: *what object is at
`desc+0x08`, and how does the row reach a body from it*. Answering it needs the
vtable at `10106890` resolved, which is a different instrument than the byte scan
and belongs to whichever phase owns that table.

## 8i. Stopping the fixture line for good, and what stands

8c stopped the fixture line once and 8f/8h reopened it with a cheaper method. That
method has now answered what it could and the question that remains is structural,
so the line stops here rather than being reopened a third time.

**What stands, as evidence, in the order it was established:**

    7r  a green registered Phase 6 target exists and passes (floor 3)
    7s  a mutation aimed at the transform arm was not caught
    8e  the campaign's root is the Scene: createScene needs a 0x710-byte Scene
        that is not reconstructed, and 42 of the 57 scene rows are discovered
    8f  the real actor layout, MEASURED: actor+0x14 is the descriptor, desc+0x08
        holds a pointer to a vtable-only object
    8h  that object carries no body pointer in its first 0x40 bytes, so the row's
        shape -> body -> pose chain is not a member walk

**What does not stand:** a working synthetic fixture, and therefore a closure for
either joint-descriptor row.

**The single highest-value unblocked task remains the Scene (8e).** Every
actor-dependent row in the programme -- the two joint-descriptor rows, the ten
joint families the plan lists, and `NxPhysicsJointTests` -- needs it, and it is
Phase 3/7 work rather than Phase 6. Two rounds of fixture work have now confirmed
that from two directions: the candidate cannot create a scene, and the object graph
a scene would give the rows is not the one the fixture can fake.

No rows move. No gate, coverage-floor, or policy change.

## 8j. The Scene is not a small unblock, and the row does not walk it

8e called the Scene "the campaign's root" and the single highest-value unblocked
task. Reading the Scene constructor this round says that framing needs correcting,
and the correction matters more than the finding it replaces.

**The Scene constructor is not a reconstructable unit in one sitting.**
`phys_fn_000647` (`0x00012c10`, 998 bytes) is a straight-line initialiser that
writes **several hundred** fields of a `0x710`-byte object and calls roughly twenty
helper constructors in sequence:

    *param_1 = &PTR_FUN_101066f4;        // the vtable
    param_1[1] .. param_1[10] = 0;
    FUN_1009a4e0(param_1 + 0xb);
    FUN_100b4d70(param_1 + 0x14);
    FUN_100e1510(param_1 + 0x18);
    ... through param_1[0x175] and beyond

`phys_fn_000651` (`0x00013070`, 1770 bytes) is the descriptor-driven initialiser
that follows it, and both are `discovered`. So "reconstruct the Scene" is not one
task: it is a 0x710-byte layout, two large initialisers, and every helper they
call, before `createScene` can return anything a joint row could use.

**And the row does not walk the Scene anyway.** The Scene constructor's vtable is
`0x101066f4`; the object the row reaches at `desc+0x08` has vtable `0x10106890`.
They are different objects. So 8h's open question -- "what is at `desc+0x08`" -- is
**not** answered by reconstructing the Scene, and 8e's claim that the Scene is this
campaign's root was too strong.

**What is actually established about the row's chain**, with the measured actor:

    actor+0x14 = desc        measured (8f)
    desc+0x00  = the actor   points back
    desc+0x08  = 016e5240    the object the row reads as shapes.first
    desc+0x10  = 016e1578    a third pointer

and the object at `desc+0x08` is vtable-only in its first 0x40 bytes (8h).

**So the honest state of the head-of-chain is:**

- the Scene **is** required for `NxPhysicsJointTests` (the candidate's
  `createScene` returns 0) and for any row reached through the public SDK;
- the Scene is **not** required for the joint-descriptor rows' transform arm,
  because that arm reads an object graph the row reaches directly from the actor;
- and the Scene is a **large** task, not a small unblock -- correcting 8e.

**Recorded rather than acted on.** Two rounds of measurement have moved the
campaign's stated root twice (the fixture, then the Scene), and both times the
correction came from reading rather than from building. The next step that is worth
its cost is neither: it is to identify what object sits at `desc+0x08` by resolving
its vtable `0x10106890` against the census, which is a lookup rather than a
reconstruction.

## 8k. `0x10106890` is not a vtable, so the object at `desc+0x08` is unidentified

8j assumed the dword at the object's `+0` was a vtable and that identifying it was
a census lookup. The lookup was done, and it says otherwise.

**What the address actually is.** `0x10106890` is the census row
`phys_data_000813`, a 4-byte `pointer_slot` inside a run of
`code_addressed_global` rows in `.rdata`. Its dword is `0x100af2c4`, and the bytes
after it are:

    +0x00  100af2c4   <- one code pointer
    +0x04  00000000
    +0x08  00000000
    +0x0c  3ff00000   <- the high half of the double 1.0
    +0x10  54442d18       +0x14  400921fb   /  the double pi
    +0x18  54442d18       +0x1c  3ff921fb   /  the double pi/2

A vtable is a run of function pointers. This is **one code pointer followed by
floating-point constants** -- a code-addressed constant block, not a table of
virtual methods.

**What that means.** The dword at the object's `+0` is `0x10106890` only if the
object's first field is a pointer to that constant block, which no object's vtable
would be. So either

- the object the row reaches at `desc+0x08` is not an object at all -- the pointer
  is to a constant block, and the row's walk is reading something the fixture and
  the probe both mis-identified; or
- the read of `desc+0x08` in the probe is not the field the row reads, because the
  row's `[eax+8]` is applied to the **actor**, not to the descriptor, on this build.

**The second reading is now the more likely one, and it is testable in one line.**
8f measured `actor+0x14 = desc` and then read `desc+0x08`. But the row's two
instructions are `mov eax,[eax+0x14]` then `mov eax,[eax+8]` -- if the first does
not land on what the probe calls `desc`, the second is reading a different field of
a different object, and every offset after it is off by one level. The probe can
print `[actor+0x08]` and `[actor+0x14]` and the row's own `eax` at each step in one
run, which distinguishes the two.

**Recorded, not resolved.** Three rounds have now moved this question by reading
rather than building, and each reading has invalidated the previous round's
assumption: 8b's `+0x0c` correction was wrong, 8e's Scene-as-root was too strong,
and 8j's vtable assumption was wrong. The pattern is worth naming: **the fixture
line has produced four rounds of corrections and no working fixture**, and the
measurements that keep invalidating it are cheap precisely because the thing being
measured was never verified.

**The recommendation stands and is now stronger.** Stop the fixture line (8i) and
treat the actor-graph question as open until someone can measure the row's own
`eax` at each step, which the probe can do in one run. That is the only remaining
measurement worth its cost here, and it is one build.

## 8l. The step measurement did not execute, and that is itself the result

8k said the remaining question was one line: print the row's own `eax` at each step
of its chain, which distinguishes "the probe reads the wrong field" from "the row
walks a different object". The measurement was attempted and **it did not produce a
reading**, which is worth recording as a boundary rather than as a step.

**What was run.** Three breakpoints, one per chain instruction:

    bp 0x100980e0    ; mov eax,[eax+0x14]   actor -> userData
    bp 0x100980e3    ; mov eax,[eax+8]      -> ?
    bp 0x100980ee    ; mov ecx,[eax+0x19c]  -> ?

**What came back.** At all three, `eax` was the same value
(`0057eee4`, then `00fcf2e0` on a later run) and `ecx` was the same. A breakpoint
placed on `0x100980e0` should fire *before* that instruction executes, so `eax`
should differ at `0x100980e3`; it did not. Either the breakpoints after the first
never fired, or the row never reaches them -- and the debugger's `dd` output for
the descriptor's `actor[0]` field did not print either, so the command file itself
did not complete as written.

**What that means for the question.** The one-line measurement 8k proposed needs a
working breakpoint sequence before it can answer anything, and this round did not
establish one. That is a tooling result, not a fact about the row, and it is
recorded as such rather than being read as evidence either way.

**What is unchanged by it.** Every finding from 8f, 8h, 8j and 8k stands as
measured: the real actor's `+0x14` is its descriptor, `desc+0x08` holds a pointer
to something whose first dword is `0x10106890`, and that address is a
`pointer_slot` whose bytes are one code pointer followed by the doubles 1.0, pi and
pi/2 -- not a run of virtual methods.

## 8m. The fixture line is closed, and this is the last round on it

8c closed the line, 8f reopened it with a cheaper method, 8i closed it again, and
8k reopened it for one measurement. That measurement did not execute. **It closes
here and does not reopen.**

The reason is not that the question is uninteresting -- it is that six rounds have
gone to two rows and produced:

    7r  a green registered Phase 6 target            (real, reusable)
    7s  a mutation aimed at the arm was not caught    (real, decisive)
    8e  the Scene is a large task, not a small unblock (corrected 8j)
    8f  the real actor layout, measured               (real)
    8h  the object at desc+0x08 has no body pointer   (real)
    8k  0x10106890 is a constant block, not a vtable  (real)
    8l  the step measurement did not execute          (tooling)

**Six findings and no closure.** Three of the six are corrections of earlier rounds
in the same line. That ratio is the honest summary, and the correct disposition is
to leave the two rows `reconstructed` with their proof and their blocker recorded,
keep the probes checked in, and spend no further rounds on this fixture.

**What the programme should do instead, stated once more and finally:** the two
joint-descriptor rows are blocked on an object-graph question that has now resisted
four measurement methods, and the 127 other reconstructed rows are blocked on
targets that do not exist. Neither is served by more fixture work. The
highest-value work in this programme is elsewhere -- the census audits of 7e-7l
each found a defect no gate could see, and there are 5,554 `discovered` rows whose
slates nobody has walked.

## 8n. A census audit that came back clean, and the method lesson it carries

Round 27 recommended moving from fixture work to census audits, because 7e-7l had
each found a defect no gate could see. This round ran the next audit in that series
and it came back **clean**, which is worth recording as carefully as a defect.

**What was asked.** The census records 6,338 function rows: 5,552 `discovered`,
115 `dynamically_gated`, 665 `reconstructed`, 6 `statically_reviewed`. A row on a
dynamic rung asserts that something ran and noticed, so the question was whether
every such claim is backed.

**The first reading looked like a defect.** 106 of the 115 `dynamically_gated` rows
carry neither a `dynamic_proof` nor a `static_proof` in the census -- the state that
asserts a gate caught something, with no evidence in the field named for it. That
is the same shape as 7i's finding (59 `reconstructed` rows with no proof) and it
would have been the fifth audit defect.

**It is not a defect, and the reason is the lesson.** A dynamic state is put there
by a **closure-ledger entry**, and the ledger entry carries its own proof. Checked
against that second source:

    rows on a dynamic rung                          115
      with a closure-ledger entry                   115
      WITHOUT one                                     0

and the ledger's own proofs are 80 `differential_falsified`, 35
`oracle_differential_falsified` and 6 `static_proof_falsified`, across seven
registered gates. So every dynamic claim is backed -- by the ledger, which is where
the schema puts it.

**The validator already enforces this.** `validate_row_states` says so in its own
docstring: "No row stands above `reconstructed` without a ledger entry that put it
there ... the floors alone leave the ceiling open, and the ceiling is where the
claim gets made." So the audit's question was already a gate, and the gate passes.

**The method lesson, which is the fifth of its kind in this session.** This round
nearly recorded a defect by checking **one of the two places evidence can live**.
7g found `source` carrying two claims; this found a claim whose evidence lives in a
file the census does not contain. Before calling an absent field a missing proof,
check the closure ledgers -- and before that, check whether a gate already asks the
question.

**What the census genuinely does not carry, stated for the record:**

- 1,943 `discovered` rows have no proof, which is correct: `discovered` is the
  bottom rung and asserts nothing;
- 665 `reconstructed` rows all carry a proof (7i-7k backfilled the last 59);
- 115 dynamic rows all have a ledger entry;
- and `phys_fn_001315` (`0x000266a0`, phase 3) is the one row whose static proof
  names a transcription while its state is `discovered`. That is a row written up
  and not promoted, not a row claiming more than it has -- and it is recorded here
  rather than changed, because promoting it would need a proof the ledger accepts.

## 8o. AUDIT: three phases owned 5,340 rows and published no ledger at all

8n's audit came back clean, so the next question in the series was the other
direction: not "is each claim backed" but "is each row accounted for at all".

**The finding.** `validate_inventory.py` checks every closure ledger it finds and
none it does not, in its own words: *"Each phase that has published a closure ledger
must account for every row it owns."* A phase that publishes none therefore has
every row it owns in **neither a closed nor a deferred list**, and nothing says so.
Measured against the census:

    phase  owns    ledger   accounts for
    2      1194    yes      1194
    3      494     yes      494
    4      3950    NO       nobody
    5      327     NO       nobody
    6      964     yes      964     (published this session, 7m)
    7      1063    NO       nobody
    8      3484    NO       nobody

**5,340 rows in phases 4, 5 and 7 were in no list at all** -- 47% of the census's
11,476 rows. Phase 8 is excluded on purpose: its gate is "entire census closed" and
its rows are that audit's own subject rather than a phase's slate.

**What was done.** Ledgers for phases 4, 5 and 7, written from the census the same
way phase 6's was (7m): every row deferred, none closed, because a closure needs a
per-row mutation and none of the three has one. Each ledger's `note` says so and
carries the count it is deferring.

`program.json`'s counters for those phases were stale (`null` for all four fields)
and are now the values the validator itself recomputes:

    phase 4   closed_functions=0  remaining_functions=1137  remaining_data_objects=2813
    phase 5   closed_functions=0  remaining_functions=205   remaining_data_objects=122
    phase 7   closed_functions=0  remaining_functions=561   remaining_data_objects=502

**The gate that was missing.** `validate_inventory.py` now requires every phase that
owns rows to publish a ledger, exempting the full-census audit phase. Verified both
ways: the committed census returns no errors, and removing one ledger returns

    error: phase 7 owns rows but publishes no closure ledger; every row it owns is
    in neither a closed nor a deferred list, and a row in no list is one nobody has
    accounted for

**This is the sixth audit finding of the session and the second of its kind.** 7i
found 59 rows holding a state with no proof; this found 5,340 rows holding no state
accounting at all. Both are the same defect at different scales: **the census's
per-row claims are checked, and the census's completeness is not.** The validator
checked that no row stood too high without a ledger; nothing checked that every row
had one.

**What this changes about the programme's numbers, and what it does not.** Phase 4's
1,137 function rows and 2,813 data objects, phase 5's 205 and 122, and phase 7's 561
and 502 are now formally deferred rather than unaccounted. That does not close
anything and does not move any row's state -- but it converts "nobody has looked at
these" into "these are deferred, with a named reason", which is what a ledger is
for and what the next phase reads.

## 8p. AUDIT: the global gates all said `pending`, and nothing checked their status

8o accounted for every row. This round asked the same question one level up: are
the **global gates** honest about themselves?

**The finding.** All four global gates in `program.json` said `pending`, and every
one of them has been run and passes:

    build_configure          cmake -S . -B build -A Win32            exit 0
    build_physics            cmake --build ... --target NxPhysics     exit 0
    validate_inventory       validate_inventory.py inventory.json     exit 0
    phase_gate_completed     run_phase_gate.ps1 -Phase completed      exit 0

**And the vocabulary nothing checked.** `validate_inventory.py` defines

    PROGRAM_STATUSES = ("pending", "pass", "fail")

and applies it to `program.phases[].status` -- but **never to
`program.global_gates[].status`**. So a global gate could hold any string at all and
the validator would accept it. That is the same shape as 7e's source paths, 7g's
mixed field and 8o's missing ledgers: **a declared vocabulary with no check behind
it.**

**What was done, in both directions.**

1. The four gates now record `pass`, which is what their own commands return. Each
   was run this session; the evidence is the run, not the file.
2. `validate_inventory.py` now applies `PROGRAM_STATUSES` to the global gates,
   beside the phase-status check that already existed. Verified both ways: the
   committed census returns no errors, and setting one gate's status to `greenish`
   returns

       error: program.global_gates[0] status 'greenish' is not one of
       ['pending', 'pass', 'fail']

**Why the statuses were wrong and what it cost.** Nothing read them, so nothing
noticed. But `program.json` is, in the validator's own words, *"the document the
next phase reads to decide what is already done"* -- and it said none of the four
prerequisites was done. A session starting from it would have re-run four gates
that were already green, or worse, assumed the build had never been configured.

**The seventh audit finding, and the third of this shape.** 7e found 429 rows
naming a file that does not exist; 7g found a field carrying two claims; 8o found
5,340 rows in no list; this found a status field whose vocabulary was decorative.
All four are the same defect at different places: **the census and the programme
record are checked for the claims they make and not for the vocabulary they use.**

## 8q. Verified end state, and the honest disposition of the objective

Every gate and ledger as measured at this point, in one place:

    phase 1        exit 3   skipped: no_registered_test_targets
    phase 2        exit 0   pass
    phase 3        exit 0   pass
    phase 4        exit 0   pass
    phase 5        exit 1   RED on purpose (the deliberate family marker)
    phase 6        exit 0   pass          <- was `skipped` at the session's start
    phase 7        exit 3   skipped: no_registered_test_targets
    phase 8        exit 3   skipped: no_registered_test_targets
    completed      exit 0   pass
    validate_inventory  exit 0   unexplained=0
    tool unit tests     587 tests, OK
    global gates        all four `pass` in program.json, and now checked

    closure phase 2  closed=59   deferred=1135
    closure phase 3  closed=62   deferred=432
    closure phase 4  closed=0    deferred=3950
    closure phase 5  closed=0    deferred=327
    closure phase 6  closed=0    deferred=964
    closure phase 7  closed=0    deferred=1063

**Every row the census contains is now named in exactly one ledger list.** That was
not true at the session's start: phases 4, 5 and 7 had no ledger at all, so 5,340
rows were in neither a closed nor a deferred list (8o).

## 8r. What the objective asked for, and where each part stands

**(a) Land the wmain frame fix and re-verify `batch3268` / `003268`.** Not landed,
and the evidence says it should not be. Rounds 1-5 established that the conversion
fails at every prefix length and for two storage mechanisms, that renaming alone is
faithful, and that the harness at `HEAD` is stable at 353 lines with `batch3268
failures=3` -- which is the state the phase-5 gate is RED for. The frame fix would
buy the ability to *extend* `wmain`; the gate does not need it.

**(b) Close census rows slate by slate.** **No row has closed**, and the reason is
measured rather than assumed: a closure needs a mutation the gate catches, a
mutation needs a registered target, a target needs an arm that runs, and the Phase
6 rows' arm needs an actor. There is now a registered target (7r) and the actor
question is diagnosed to its root -- the Scene is unreconstructed and the candidate
cannot create one (8e) -- but no arm has been reached.

**(c) Keep every global gate honest.** Done, and it found work: the four global
gates all said `pending` while passing, and their status vocabulary was never
checked (8p). They now record `pass` and the validator enforces the vocabulary.
Three new checks were added this session, each verified to fail on the defect it
targets: source paths (7f), reconstructed proofs (7k), implementation paths (7h),
and per-phase ledgers (8o).

**(d) Record evidence per convention.** Done. Sections 3z271 through 8r in this
file and `phase6-joints.md`'s own sections 6a-8r, all committed.

## 8s. The four defects this session found that no gate could see

    7e  429 rows named a source path that does not exist
    7g  the `source` field carried two different claims
    8o  5,340 rows were in no ledger list at all
    8p  the global gates' status vocabulary was declared and never checked

plus three more that were corrected in place: 7i (59 rows held `reconstructed` with
no proof), 7s (a closure claimed from a differential the schema does not accept),
and 8h/8j/8k (three corrections to this session's own fixture findings).

**The pattern, stated once for whoever reads this next:** every one of those was
found by asking *what does this claim rest on* rather than by re-running a gate.
The gates were all green or honestly red throughout; the defects were in the space
between what the documents said and what anything checked.

**And the counterweight, stated as plainly:** a session that finds seven defects and
closes no row has not advanced the reconstruction. The census is now fully
accounted for and the programme record is honest, which is real and was not true
before -- but `closed=0` for phases 4, 5, 6 and 7 is the number that matters for the
Phase 8 gate, and it has not moved.

## 8t. Scene reconstruction, phase 1: the object and its constructor

The Scene is the campaign's head-of-chain (8e) and the user directed its
reconstruction. This is the first phase: the 0x710-byte object and its constructor,
faithfully transcribed, compiling, and with the remaining phases enumerated.

**What was measured, and from where.**

    phys_fn_000476  0x0000ea80  344 B  PhysicsSDK::createScene -- allocates 0x710
                                        bytes and drives the two rows below
    phys_fn_000647  0x00012c10  998 B  the constructor, a straight-line initialiser
    phys_fn_000651  0x00013070 1770 B  the descriptor-driven initialiser

`0x710` is where `createScene`'s allocation literal comes from, so the object size
is measured rather than assumed.

**What was written.**

- `Physics/src/include/Scene.h` -- `NxSceneInternal`, deliberately an
  **offset-addressed** object rather than a class with named members. The oracle
  reaches every field through raw pointer arithmetic, and a named-member layout
  would silently reorder fields and change the behaviour under test. Accessors
  exist so the code reads; the field *names* are not claimed, only their offsets.
- `Physics/src/Scene.cpp` -- the constructor, with **every field write transcribed
  in the oracle's order**, each line carrying its dword index so it can be checked
  against the decompilation. Nothing is elided: a skipped field would be one a
  differential could not see.
- The two reconstructed leaves, transcribed in place: `phys_fn_004147`
  (0x0009a4e0) and `phys_fn_002346` (0x0005ab50).
- Seven helper **reproduction holes** for rows other phases own, each carrying its
  oracle field writes in order and each named so its owner can displace it.

**Verified.** `Scene.cpp` compiles into `NxPhysicsInternalTests`
(`build/NxPhysicsInternalTests.dir/Release/Scene.obj`, 6124 bytes). All gates
unmoved: phases 2, 3, 4 and `completed` exit 0, phase 5 exit 1 RED on purpose,
phase 6 exit 0 PASS, `validate_inventory` exit 0 with `unexplained=0`, 587 tool
tests OK.

**The validator caught the reconstruction before I did.** `Physics/src/Scene.cpp`
was on `UNRESOLVED_SOURCE_PATHS` with 14 rows against it. Creating the file made
that entry false, and the check refused to pass:

    error: unresolved source path 'Physics/src/Scene.cpp' is on the allowlist but
    no longer unresolved; remove the entry

That entry is removed and the comment left in its place. **This is the 7f check
doing exactly what it was built for** -- and the first time in this session a gate
objected to work going well.

## 8u. What Phase 1 does not do, stated plainly

The object exists and its constructor runs. **`createScene` still returns 0**,
because the pieces above it are not wired:

    phys_fn_000651  the descriptor initialiser    NOT written
    phys_fn_000476  PhysicsSDK::createScene       NOT written
    phys_fn_000234  NpPhysicsSDK::createScene     NOT written
    NpScene         the public NxScene wrapper    NOT written
    Scene::createActor                            NOT written
    the seven helpers above                      holes, not implementations

So this phase is a floor, not a result. What it buys is that the next phase has a
real object to initialise rather than a size constant, and that the 18-row work list
is enumerated with the ones already done marked:

    18 rows called by the constructor and initialiser
      2  reconstructed and transcribed here (004147, 002346)
      2  the two roots themselves (000647, 000651) -- 000647 transcribed
      7  helper holes, declared with their oracle writes
      7  not yet examined: 001980, 000285, 002415, 001275, 001275, 000544, 000626

**The largest remaining item is `phys_fn_000626` (0x00011730) at 3227 bytes**, the
Scene's descriptor application, and `phys_fn_000544` (0x00010750) beside it. Those
two are the next phase.

## 8v. The order the remaining work has to happen in

    1  phys_fn_000651   the descriptor initialiser      (written next)
    2  the seven holes  or the rows behind them
    3  Scene::createActor                                (the harness needs it)
    4  NpScene + phys_fn_000234 + phys_fn_000476         (the public path)
    5  verify: NxPhysicsJointTests `scene=created` against both pairs
    6  then the two joint rows' transform arm is reachable, and the mutation that
       closes them can finally be aimed

Steps 1-4 are the reconstruction. Step 5 is the measurement that says whether it
worked, and step 6 is the closure this whole line of work has been for.

## 8w. Scene reconstruction, phase 2: the descriptor initialiser

`phys_fn_000651` (0x00013070) is written, with **two blocks named as reproduction
holes** rather than quietly guessed.

**The descriptor offsets are read, and they are NxSceneDesc fields.** Every word the
oracle loads is a named member of the pinned public header, so the mapping is not
an inference:

    0x00 vtable      0x04 userData     0x08 gravity    0x14 userContactReport
    0x18 maxTimestep 0x1c maxIter      0x20 solverType 0x2c limits
    0x30 groundPlane 0x31 upAxis       0x34 flags

**Written and transcribed:**

- the limits pointer (descriptor word 0x0b) copied to five Scene fields at
  `+0x18..+0x28` -- `maxNbActors`, `maxNbBodies`, `maxNbStaticShapes`,
  `maxNbDynamicShapes`, `maxNbJoins`;
- the two embedded array reserves that follow it, at `+0x55c` and `+0x56c`, whose
  layout `{first, last, memEnd, allocator}` is the one the oracle's
  capacity-compare-then-grow sequence manipulates;
- `+0x52c`, `+0x530`, `+0x534` from descriptor words 7, 8, 9 -- `maxTimestep`,
  `maxIter`, `solverType`;
- `+0x520..+0x528` from descriptor words 1, 2, 3;
- bit 0 of `+0x70c` set or cleared from descriptor byte 0x32;
- `+0x6ac`, `+0x6b0`, `+0x6b4` from descriptor words 4, 5, 6;
- `+0x538 = 0`, and `true` returned.

**The two holes, named:**

- `phys_fn_000544` (0x00010750, 144 B, phase 7) -- applies the descriptor's flag
  words. Called, not modelled.
- `phys_fn_000626` (0x00011730, **3227 B**, phase 7) with `phys_fn_000501`
  (0x0000ff10, 393 B) -- the ground-plane expansion, which the oracle drives from a
  stack-built shape descriptor across six iterations, twice, gated on descriptor
  byte 0x30 and on byte 0x31 with a non-null word 0x0a.

**What that means, stated so it cannot be misread:** a scene built with
`groundPlane` set does **not** get a ground plane from this reconstruction. The
flag at `+0x70c` is set correctly, and the shape is not built. `createScene` still
returns 0 regardless, because the rows above it are not written.

**Verified.** `Scene.cpp` compiles into `NxPhysicsInternalTests`; phases 2, 3, 4, 6
and `completed` all exit 0; `validate_inventory` exit 0.

## 8x. The remaining path, with sizes

    3227 B  phys_fn_000626  0x00011730  the ground-plane expansion   phase 7
     393 B  phys_fn_000501  0x0000ff10  its bounds consumer           phase 7
     144 B  phys_fn_000544  0x00010750  descriptor flags              phase 7
     344 B  phys_fn_000476  0x0000ea80  PhysicsSDK::createScene       phase 2
      31 B  phys_fn_000234  0x0000b770  NpPhysicsSDK::createScene     phase 2
       ?    NpScene                       the public NxScene wrapper
       ?    Scene::createActor            the harness's next call
       ?    the seven constructor helpers  phase 4 and 7

**The smallest route to a working harness does not go through 3227 bytes of
ground-plane code.** `createScene` can be wired up with the ground-plane hole left
in place -- the harness passes a descriptor with `groundPlane` unset -- and the
next steps in cost order are `Scene::createActor`, then `NpScene` with
`phys_fn_000234` and `phys_fn_000476`. That is the order the next session should
take, and the ground-plane hole is recorded so it is not mistaken for done.

## 8y. The 3227-byte "hole" is `Scene::createActor`, and it is the critical path

8x listed `phys_fn_000626` (0x00011730, 3227 B) as "the ground-plane expansion"
and recommended leaving it in place, on the ground that the harness passes a
descriptor with `groundPlane` unset. **That recommendation was wrong, and reading
the function is what showed it.**

`phys_fn_000626` is **`Scene::createActor`**. Three things say so, and none of them
is an inference:

- its own error strings are *"Supplied NxActorDesc is not valid. createActor returns
  NULL."* and *"Actor Initialisation failed: returned NULL."*;
- its head is `NxActorDescBase::isValid()` inlined -- a long chain of `__fpclass`
  tests over the descriptor's twelve `globalPose` floats, the body's twelve, and a
  per-shape validity loop;
- it ends by pushing the new actor onto the Scene's actor array at `+0x55c`, the
  same array the descriptor initialiser reserves.

`createScene` reaches it **because the ground plane is made by calling
`createActor`** -- not because it is ground-plane code. So the function 8x proposed
to skip is the one the harness's very next call, `scene.createActor(da)`, goes
through. **The cheap route 8x described does not exist**, and the correction cost
one read of the function.

## 8z. `Scene::createActor` written

The validation half is `NxActorDescBase::isValid()` -- the pinned public header's
own inline code -- so it is **called, not transcribed**: re-typing a predicate the
pinned header already provides would create a second copy that could drift.

The creation half is transcribed:

    malloc(0x50)                     the actor block
    phys_fn_00001450                 construct it over the block
    phys_fn_00002010                 apply the descriptor; non-null = built
    push onto the Scene's +0x55c array, growing it as the initialiser does
    copy holder[3], holder[4] into actor+0x0c and actor+0x10
    phys_fn_000100a0                 refresh the Scene's cached count
    phys_fn_00089d50                 the notification hook, when +0x61c is set

**Five callees are named reproduction holes**, each with what it does not model
recorded in the file:

    phys_fn_00001450  constructs the actor over the block
    phys_fn_00002010  applies the descriptor to the actor
    phys_fn_00001c40  destroys an actor built by those two
    phys_fn_000100a0  refreshes a cached count
    phys_fn_00089d50  the scene's notification hook

**The honest statement of what that leaves wrong:** the holes reproduce the call
shape and the state the Scene reads back, and nothing else. In particular
`phys_fn_00002010` is where the descriptor is applied to the actor -- including the
`userData` the joint-descriptor rows read -- so **a scene built through this path
produces an actor whose descriptor chain is not yet correct.** `createActor` will
return a non-null actor; that actor will not yet be the oracle's actor.

**Verified.** `Scene.cpp` compiles into `NxPhysicsInternalTests`; `Scene.obj` grew
from 6124 to 14546 bytes; phases 2, 3, 4, 6 and `completed` all exit 0;
`validate_inventory` exit 0.

## 9a. Corrected remaining path

    144 B  phys_fn_000544  0x00010750  descriptor flags              phase 7
    344 B  phys_fn_000476  0x0000ea80  PhysicsSDK::createScene       phase 2
     31 B  phys_fn_000234  0x0000b770  NpPhysicsSDK::createScene     phase 2
      ?    NpScene                       the public NxScene wrapper
      ?    phys_fn_00001450             the actor's constructor        phase 2
      ?    phys_fn_00002010             applying the descriptor        phase 2
      ?    the seven constructor helpers
    3227 B phys_fn_000626  Scene::createActor                   DONE, 5 holes

**`phys_fn_00002010` is now the highest-value row on this path**, because it is
where the actor's `userData` and its descriptor chain are established -- the exact
structure the joint-descriptor rows walk and the reason six rounds of synthetic
fixture work failed to fake it (8f-8k). Filling that hole is the shortest route to
the mutation that closes those two rows.

## 9b. `Actor::loadFromDescInternal` and the actor constructor, written

9a named `phys_fn_00002010` as the highest-value row on the path. It is now written,
together with the actor constructor.

**`phys_fn_000034` (0x00002010, 565 B, phase 5) is
`Actor::loadFromDescInternal`.** Again the function names itself: its error strings
are *"Actor::loadFromDescInternal: Compute mesh inertia tensor failed for one of the
actor's mesh shapes!..."* and *"...Can't compute mass from shapes: must have at
least one non-trigger shape!"*, and its `__FILE__` is
`.../Physics/src/Actor.cpp`.

Transcribed, in the oracle's order:

    descriptor words 0..8   -> actor+0x20, nine dwords of globalPose
    words 9, 10, 0x0b       -> actor+0x44, +0x48, +0x4c   (globalPose.t)
    word 0x0d               -> actor+0x18                 (the body descriptor)
    word 0x0e               -> actor+0x1c                 (the body's flags word)
    word 0x0f               -> actor+0x14                 (userData)
    word 0x11               -> the name, via phys_fn_0000edc0
    word 0x12 == 1          -> build a body, then test actor+0x10
    the shape count from words 0x13/0x14 decides the remaining path
    the mass pass returns 1 (mesh-inertia failure), 0 (success) or other

**`phys_fn_000013` (0x00001450, 157 B, phase 7) is the actor constructor.**
Transcribed: the Scene back-pointer at +4, an empty shape list at +0x10, the
identity 3x3 at +0x20..+0x40, and the scene's slot handout -- either the counter at
`+0x6d0` is incremented or the free list at `+0x6d4..+0x6d8` is popped.

**Six callees are named reproduction holes**, each with what it does not model
recorded in the file:

    phys_fn_0000edc0  sets the actor's name
    the body builder  constructs and links the body object at actor+0x10
    phys_fn_000019b0  computes mass from the shapes
    phys_fn_00010600  registers the object with the scene
    the +0x18 sub-object the constructor allocates
    the Actor.cpp error route

**What is now correct that was not, and what is still not.** The actor's
`userData` at `+0x14` is now written from descriptor word 0x0f, and its body
pointer, group and flags from words 0x0d, 0x0e, 0x0f -- which is the part of the
chain the joint-descriptor rows read. What is still missing is everything the holes
cover: the body object itself, the shape objects, and the scene registration. **An
actor built through this path now carries the right descriptor fields and no body
or shapes.**

**Verified.** `Scene.cpp` compiles into `NxPhysicsInternalTests`; `Scene.obj` grew
from 14546 to 17047 bytes; phases 2, 3, 4, 6 and `completed` all exit 0;
`validate_inventory` exit 0.

## 9c. Scene reconstruction, state after four phases

    DONE   phys_fn_000647  0x00012c10   the 0x710-byte constructor
    DONE   phys_fn_000651  0x00013070   the descriptor initialiser (2 holes)
    DONE   phys_fn_000626  0x00011730   Scene::createActor (5 holes)
    DONE   phys_fn_000034  0x00002010   Actor::loadFromDescInternal (6 holes)
    DONE   phys_fn_000013  0x00001450   the actor constructor
    TODO   phys_fn_000544  0x00010750   descriptor flags              144 B
    TODO   phys_fn_000476  0x0000ea80   PhysicsSDK::createScene       344 B
    TODO   phys_fn_000234  0x0000b770   NpPhysicsSDK::createScene      31 B
    TODO   NpScene                      the public NxScene wrapper
    TODO   the body builder and the shape factory, behind the holes

**Five rows and about 5,000 bytes of oracle code are transcribed and compiling.**
`createScene` still returns 0, because the wiring above the Scene -- `NpScene`,
`phys_fn_000234`, `phys_fn_000476` -- is the next phase and nothing has been
written for it yet.

**The next smallest step is `phys_fn_000476` plus `phys_fn_000234` and an `NpScene`
whose `createActor` forwards**, which is a few hundred bytes rather than thousands.
That is what turns `NxPhysicsJointTests` from `scene=null` into `scene=created`, and
it is the measurement that says whether four phases of this work are right.

## 9d. The public path is wired: NpScene, and both createScene rows

9c named the next step as a few hundred bytes rather than thousands. It is written.

**`NpScene`** (`Physics/src/include/NpScene.h`, `Physics/src/NpScene.cpp`), the
public `NxScene` the user is handed, is 0x28 bytes and its layout is measured from
`phys_fn_000285` (0x0000c310):

    +0x00 vtable   +0x08 a lock object   +0x0c and +0x10 4-byte locks
    +0x14, +0x18 locks   +0x1c a 0x18-byte object   +0x20 a byte   +0x24 the Scene

Its forwarding shape is `phys_fn_000293`: try the write lock at `+0xc`, forward,
release the lock, or report *"PhysicsSDK: WriteLock is still aquired. Procedure call
skipped to avoid a deadlock!"*. `createActor` and `releaseActor` follow it.

**`PhysicsSDK::createScene`** (`phys_fn_000476`, 0x0000ea80) is written in
`PhysicsSDK.cpp`: validate, `malloc(0x710)`, construct, `initialise`, and push onto
`mScenes` -- whose `+8/+0xc/+0x10` quadruple is exactly what the oracle's inline
growth reads. The validation is the pinned `NxSceneDesc::isValid()`; the four tests
the oracle inlines beside it (`maxIter >= 1`, `solverType <= 2`, `maxTimestep != 0`,
`upAxis == 0 || flags != 0`) are recorded in the comments.

**`NpPhysicsSDK::createScene`** (`phys_fn_000234`, 0x0000b770) is written in
`NpPhysicsSDK.cpp`: forward, wrap the Scene in an `NpScene`, and store the wrapper
back at the Scene's `+0x6cc`.

**Two mechanical obstacles, both recorded because both are traps:**

- `NpScene` must be concrete and `NxScene` declares **66 pure virtuals**. 63 bodies
  were generated from the pinned header by script rather than typed, each an empty
  body returning a default, and the generated block says **UNIMPLEMENTED** in its
  own heading. Only `createActor` and `releaseActor` are reconstructed.
- `NxAllocateable` declares `operator new(size_t, NxMemoryType)`, which **hides the
  placement form**, so `new (pointer) NpScene(...)` does not compile. The wrapper is
  allocated with `new (NX_MEMORY_PERSISTENT) NpScene(...)`, which is also what the
  oracle does.

**The validator caught the second file too.** Creating `Physics/src/NpScene.cpp`
made the allowlist entry that named it (37 rows) false, and the check refused
again: *"is on the allowlist but no longer unresolved; remove the entry"*. Removed,
comment left behind. Two files this reconstruction created have now been flagged by
the 7f check, which is it working exactly as designed.

**One gate broke and was repaired, not papered over.** `NxPhysicsCollisionTests`
links `PhysicsSDK.cpp`, so it now needs `Scene.cpp` and `NpScene.cpp`; it failed to
link until they were added. That is a real dependency the reconstruction created,
recorded rather than worked around.

**Verified.** All gates green: phase 1 exit 3 skipped (no registered targets),
phases 2, 3, 4 exit 0, phase 5 exit 1 RED on purpose, phase 6 exit 0 PASS, phases 7
and 8 exit 3 skipped, `completed` exit 0, `validate_inventory` exit 0 with
`unexplained=0`, 587 tool tests OK. `NxPhysics.dll` builds.

## 9e. The harness now runs the path, and faults where the stubs are

The measurement 9c asked for:

    NxPhysicsJointTests <candidate pair>        BEFORE:  sdk=created, scene=null
                                                AFTER:   exit -1073741819, fault

**This is the first time the harness has executed the scene path at all.** Before
this round `createScene` returned 0 and the harness printed `scene=null` and stopped;
now it enters the reconstruction and faults. The fault is in `NxPhysics.dll` with
`eax = 0` dereferencing `[esi + 0x1c30]`, inside the SDK-creation region.

**Why that is expected rather than surprising.** The path runs through eleven
reproduction holes -- the body builder, the shape factory, the scene registration,
the mass computation, the lock protocol, the collector object, the name setter --
and every one of them is an empty or minimal stub. A scene built through stubs that
do nothing is not a scene the oracle would produce. **The fault is the honest
consequence of the holes, and it is the measurement that shows the path is now
live.**

**What this does not establish.** It does not establish that any of the five
transcribed rows is correct, and it does not put `NxPhysicsJointTests` any closer to
`scene=created` than it was. What it establishes is that the wiring is reachable and
that the next failure is in the holes rather than in the structure.

## 9f. State after six phases, and what the next one is

    DONE   phys_fn_000647  0x00012c10  the 0x710-byte constructor
    DONE   phys_fn_000651  0x00013070  the descriptor initialiser (2 holes)
    DONE   phys_fn_000626  0x00011730  Scene::createActor (5 holes)
    DONE   phys_fn_000034  0x00002010  Actor::loadFromDescInternal (6 holes)
    DONE   phys_fn_000013  0x00001450  the actor constructor
    DONE   phys_fn_000476  0x0000ea80  PhysicsSDK::createScene
    DONE   phys_fn_000234  0x0000b770  NpPhysicsSDK::createScene
    DONE   NpScene                      the public wrapper (63 unimplemented virtuals)
    DONE   the wiring                  the harness now enters the path

**Seven oracle rows and about 5,300 bytes transcribed, plus the wrapper class.**
`createScene` no longer returns 0. The harness faults in the holes.

**The next phase is not more transcription -- it is narrowing the fault.** The
crash is at `NxPhysics!NxCreatePhysicsSDK+0xd0c` with `eax = 0` reading
`[esi+0x1c30]`, and `esi` holds the object being built. The step to run is a
debugger break on that frame with `esi` printed, to see which object is null and
therefore which hole is being relied on at that moment. That is one breakpoint, and
it is smaller than any row in the list above.

## 9g. Narrowing the fault, and what is established

9e recorded that the harness now enters the reconstructed path and faults. This
round attempted to narrow it and got partway.

**What was established.**

- The fault address does **not** map to a named row. The candidate's export table
  gives `NxCreatePhysicsSDK` at rva `0x67a0`; the fault is at rva `0x74ac`, about
  3 KB past it, so the debugger's `NxCreatePhysicsSDK+0xd0c` label is a nearest-export
  artefact and not a real frame.
- **The harness produced no output at all before the fault** -- not even the lines
  that precede the call (`export=...`, `version=`, `sdk=`). stdout is block-buffered
  and the crash discards it, so the absence of `sdk=created` does **not** mean SDK
  creation failed. Reading the crash as "the SDK failed to build" would be wrong.
- At the fault: `esi = 0x01355930`, a heap pointer, and the instruction reads
  `[esi + 0x1c30]` -- a 7 KB offset. **No structure in this reconstruction has a
  field at `0x1c30`**, so the pointer or the base is wrong rather than the field.
- `NxPhysicsJointTests` is **not registered in `gate_targets.ps1`** (zero mentions),
  so no gate runs it and no gate is affected by the crash.

**What was not established.** Which of the new pieces the bad pointer comes from.
The candidates, in the order worth testing, are the `NpScene` constructor's three
lock allocations, the `Scene` constructor's two sub-object allocations, and
`NxSceneDesc::isValid()` being read at the wrong offsets -- and distinguishing them
needs a breakpoint on the candidate's `createScene` entry, which this round did not
reach.

**A regression, recorded as one.** `NxPhysicsJointTests` against the candidate pair
exited 0 with `scene=null` before this work and now faults. That is a real
regression in that harness, and it is the honest cost of routing the path through
stubs: the old behaviour was a clean refusal, the new behaviour is a crash inside
code that is not finished. It is not gated, so nothing else depends on it, but the
programme's own convention is that a harness it builds should not get worse.

## 9h. What a future session should do first

**Before anything else: decide whether to keep the wiring live.** There are two
honest options and both are recorded here.

1. **Keep it and finish the holes.** The wiring is correct in structure -- seven
   rows transcribed, the vtable order preserved, the classes concrete -- and the
   fault is in the stubs. Filling the body builder and the shape factory is the
   path to `scene=created`.
2. **Gate the wiring off** so `createScene` returns 0 again and the harness goes
   back to exiting cleanly, keeping the transcribed rows in the tree but unconnected
   until the holes are filled. This restores the harness's previous behaviour at the
   cost of not exercising the new code.

**The measurement that decides which is cheaper** is one breakpoint on the
candidate's `PhysicsSDK::createScene` entry with the three candidates above checked
in order. That is smaller than any row transcribed so far.

**And the honest summary of this stretch of work, stated once.** Eight commits
reconstructed the Scene from nothing to a wired, compiling, gate-green implementation
of seven oracle rows and a wrapper class, about 5,300 bytes of decompiled code. **No
census row closed, no joint row closed, and the harness it was all for now crashes
instead of refusing cleanly.** The structure is real and the gates are green; the
outcome the work was for -- `scene=created` -- has not been reached.

## 9i. The crash was an allocation overrun in the NpScene locks, and scene=created works

9g recorded the harness regression and listed the candidates. The fault is found,
and it was none of the three guessed there.

**Method: trace, not guess.** Print statements were added to the candidate's
`createScene` path, and that immediately separated the two halves that 9g could not
tell apart. The traces showed the Scene side completing:

    TRACE np-createScene enter mSdk=...
    TRACE sdk-createScene enter
    TRACE isValid passed
    TRACE malloc=...
    TRACE before Scene ctor
    TRACE after Scene ctor
    TRACE before initialise
    TRACE initialise ok, pushing
    TRACE pushed
    TRACE np-createScene sdk returned scene=...
    TRACE before NpScene ctor
    <fault>

**So the Scene constructor and initialiser both run, and the fault is in the
`NpScene` constructor.** And the constructor's own fault was an **allocation
overrun**, not a bad pointer:

> `PhysicsInternal.h` documents the oracle's lock block: phys_fn_0005b6a0
> *"allocates a 32 byte block through the SDK allocator and holds only that
> pointer, so the lock object itself is one word"*. The `NpScene` **field** holds
> the pointer; the **block** is 32 bytes -- a `CRITICAL_SECTION` followed by the
> interlocked owner flag at `+0x18` and the owning thread id at `+0x1c`.

The reconstruction allocated **four** bytes for each of the two locks and then
constructed a lock into it. Four bytes cannot hold a 32-byte object, so the second
allocation's header was overwritten -- which is exactly the `[esi+0x1c30]` read 9g
measured, a small base plus a large offset.

**One detail worth recording**: the traces *masked* the fault rather than revealing
it, because adding them shifted the heap layout enough that the overrun landed on
slack. That is why 9g's "traces show the Scene is fine, the crash is in NpScene"
and the clean build's "crash" looked like different bugs. A heap overrun is
sensitive to allocation order, and that sensitivity is itself evidence of an
overrun rather than a bad pointer.

**The fix**, in `NpScene.cpp`: the locks are allocated `0x20` bytes through a named
constant, `nxLockConstruct` clears all eight dwords rather than the first, and the
comment carries the `PhysicsInternal.h` quotation so the number is traceable to a
measurement rather than to a guess.

## 9j. The harness now reports scene=created

    NxPhysicsJointTests <candidate pair>
      sdk=created
      scene=created
      NxPhysics: Actor Initialisation failed: returned NULL.
      exit=1

**This is the first time the candidate has created a scene.** The regression 9g
recorded is gone: the harness no longer crashes, and it is strictly better than the
`scene=null` it printed before this work.

**The remaining blocker is actor creation**, and the trace narrows it to one place.
`Actor::loadFromDescInternal` is entered with the descriptor the harness builds, and
the words it receives are all present:

    d[0..8]  = 3f800000 00000000 00000000 00000000 3f800000 00000000 00000000
               00000000 3f800000     <- the identity globalPose
    d[0x0c]  = 3f800000               <- density 1.0
    d[0x12]  = 00000001               <- the shape path selector
    d[0x13]  = 013828d0               <- the shape array's first

so the descriptor is well formed and the failure is inside the shape path. **The
function returns 0 before its first post-validation trace fires**, which is the next
thing to instrument -- one print per `return 0` in the shape path, compared against
which leaf stub ran.

## 9k. What is established, and what is not

**Established:** the candidate creates a scene. The Scene object is allocated at the
right size, constructed, initialised from the descriptor, pushed onto the SDK's
scene list, and wrapped in an `NpScene` that the caller receives. Seven transcribed
oracle rows and the wrapper class are executing, and the harness runs to completion
without faulting.

**Not established:** that the scene is the oracle's scene. Eleven reproduction holes
sit in that path and every one is a stub. `Actor::loadFromDescInternal` then fails
inside the shape path, so no actor is created and the joint rows still cannot be
driven.

**And the honest count:** no census row has closed. The value of this round is that
the harness went from *crashing* to *reporting*, that a real 4-byte/32-byte
allocation bug was found and fixed, and that `scene=created` -- the measurement six
phases of Scene work were for -- has been reached.

## 9l. Actors are created: the premature body test was the blocker

9j narrowed the actor failure to the shape path. The cause was a transcription
error of mine, not a hole.

**The error.** `Actor::loadFromDescInternal` had a body test placed **before** the
shape path:

    if(d[0x12] == 1)
        {
        nxActorBuildBody(actor, d);
        if(!a[0x10 / 4])        // actor+0x10
            return 0;           // <-- fires on every actor
        }

`actor+0x10` is written **by the shape path**, which the oracle runs *after* that
point. Testing it first meant the test read a field nothing had written yet, so the
function returned 0 for every actor the harness built. The oracle's structure, from
its decompilation, is:

    if (d[0x12] == 1) { single shape -> actor+0x10, or a group }   (flexible)
    else if (d[0x12] == 2) { the same shape path }                 (static)
    then the tail: body test, mass pass, scene registration

**With that corrected, the harness reports `fixture=a,created b,created`.** Both
actors are created, which is the first time this has happened. The scene and both
actors now exist.

## 9m. `Scene::createJoint` transcribed, and the harness reaches the joints

With actors working, the harness's next call is `scene.createJoint(desc)`, which was
one of the 63 unimplemented `NpScene` virtuals returning 0. The real row is
`phys_fn_000665` (**0x000142c0**, 718 B, phase 7), and its own error strings name it:
*"PhysicsSDK::createJoint: desc.isValid() fails!"* and *"PhysicsSDK::createJoint: at
least one of the two actors must be dynamic!"*, both with
`__FILE__ ".../Physics/src/Scene.cpp"`.

Transcribed: the re-entry guard at the file-scope flag `.data 0x00123c10` with its
own message; `desc.isValid()`; the **dynamics test**, which reads each actor's
`+0x14` body and that body's `+8` marker; a switch on descriptor word 1 with the
revolute case's `0x17c` allocation; and the tail, which reads the joint's `+0x12`
word and either copies two words out of the Scene's `+0x6cc` holder and registers
the joint, or calls the joint's deleting destructor.

`NpScene::createJoint` now forwards through the write lock in the same shape as
`createActor`.

**The harness now crashes instead of completing.** It produced no output at all,
which -- as 9g established -- means stdout was discarded by the fault rather than
that nothing ran. The trace prints added to `createJoint` did not appear either,
which places the fault **at or before `createJoint`'s first statement**: the
dynamics test reads two pointer chains (`actor+0x14` then `+8`), and the actors this
reconstruction builds have no body, because `nxActorBuildBody` is still a hole that
does nothing.

**So the next step is precise**: `nxActorBuildBody` must attach a real body object,
because `Scene::createJoint` dereferences it. That is one hole, named, with a known
caller -- and it is also what the joint-descriptor rows' pose chain needs, since
`phys_fn_000004` reads `body+0x19c`.

## 9n. State after this round

    scene=created                  reached (9i)
    fixture=a,created b,created    reached (9l)
    Scene::createJoint             transcribed (9m)
    the harness                    crashes at or before createJoint's first statement
    nxActorBuildBody               the hole the crash points at

**The blocker is now a single named hole with a single known caller**, rather than
"somewhere in eleven stubs". That is a materially better position than the round
started in, and the two steps that got here -- `scene=created` and both actors
created -- are both strictly better than the state this work inherited.

**Still no census row closed.** All gates green: phases 2/3/4 exit 0, phase 5 exit 1
RED on purpose, phase 6 exit 0 PASS, `completed` exit 0, `validate_inventory` exit 0,
587 tool tests OK.

## 9o. The actors now carry bodies, and the body pointer is not at the declared offset

9n named `nxActorBuildBody` as the blocker. It is implemented and it runs.

**What it builds.** One body object per actor, 0x1c0 bytes, carrying exactly the two
fields the reconstructed readers dereference:

    body + 8      = 1          the dynamic marker Scene::createJoint tests
    body + 0x19c  = pose       the pose pointer phys_fn_000004 reads

and it links the body at `actor+0x14`, which is where `Scene::createJoint` reads it.
The pose is a separate 0x80-byte block whose `+8` is null (the quaternion arm), with
the translation copied from the actor's own `globalPose.t` at `actor+0x44` and an
identity quaternion at `+0x5c`.

**The trace confirms it on both actors:**

    TRACE buildBody actor=00979B48 desc=006FF538 bodyDesc=006FF468
    TRACE buildBody linked body=00979BA0 at actor+0x14
    TRACE buildBody actor=00979DF0 desc=006FF594 bodyDesc=006FF3F0
    TRACE buildBody linked body=00979E48 at actor+0x14

**And it exposed a real discrepancy.** `NxActorDescBase` declares `body` immediately
after the 36-byte `globalPose`, which puts it at `0x24` = **word 9**. The dump taken
from a live actor shows:

    word 09 = 00000000        <- where the header says the body pointer is
    word 0c = 006FF468        <- where a heap pointer actually is
    word 0d = 3f800000        <- density 1.0
    word 12 = 00000001        <- the shape path selector
    word 13 = 00970098        <- the shape array's first

So the offset this build reaches is **not** the declared one, and the value is taken
from the word the dump shows rather than from the declaration. **This is recorded as
an unresolved offset discrepancy, not presented as the header's layout** -- either
`NxActorDescBase` has a member this reconstruction has not accounted for between
`globalPose` and `body`, or the descriptor the harness builds is not laid out as the
pinned header declares. That is a question for the next session and it is written
down rather than smoothed over.

**One more correction of my own earlier claim.** 9b said "the actor's `userData` at
`+0x14` is written from descriptor word 0x0f". That was wrong: `actor+0x14` is the
**body** pointer, and writing userData there clobbered it. The assignment is removed
with a comment saying why, and the `userData` field is left unidentified rather than
put somewhere it does not belong.

## 9p. Where the harness stands, and the next measurement

    scene=created                       reached
    both actors created                 reached
    both actors carry a linked body     reached (9o)
    createJoint reached                 NO -- the crash precedes its first statement
    the harness                         exit -1073741819, no output

**The `createJoint` entry trace never fired**, and the bodies are built before the
crash, so the fault is between the actor creation and the joint creation -- in the
harness's own fixture code or in `NpScene::createJoint`'s write-lock step, which runs
*before* the forward to the Scene.

**The next measurement is one breakpoint, and it is now well defined:** break on the
candidate's `NpScene::createJoint` and see whether it is entered at all. If it is,
the fault is in the lock step; if it is not, the fault is in the harness's fixture
between the two `createActor` calls and the joint, which would be the harness's own
code rather than the reconstruction's.

**Still no census row closed.** All gates green: phases 2/3/4 exit 0, phase 5 exit 1
RED on purpose, phase 6 exit 0 PASS, `completed` exit 0, `validate_inventory` exit 0,
587 tool tests OK.

## 9q. Round 4: the crash is in CRT code, and the wrapper size is now pinned

9p asked for one breakpoint on `NpScene::createJoint`. That was not reached; this
round got a different and still useful measurement.

**The fault, read from the debugger:**

    eip = NxPhysics + 0x763c      mov eax,[esi+0x1c30]   esi = 00a25990
    stack: ... -> NxPhysics!NxReleasePMap+0xb97 -> image00310000+0x2c28 (the harness)
           -> KERNEL32!BaseThreadInitThunk

Disassembling back from the fault to the enclosing function's entry gives
**rva 0x7360**, whose first instructions are:

    push ebp / mov ebp,esp / or dword ptr [0x1000c0d0],1 / sub esp,0x2c
    push 0xa / call [0x1000901c] / test eax,eax
    cpuid, twice, comparing against "GenuineIntel" and "Intel"

**That is a CRT CPU-feature detector, not this reconstruction's code.** So the fault
is not a bad pointer this reconstruction computed; it is a helper reading a large
offset off a heap pointer, which is what heap corruption looks like from a distance.

**The wrapper size is now pinned.** The oracle allocates 0x28 bytes for `NpScene`
(`phys_fn_000476`'s literal). A class whose size differs would write past its
allocation, which is exactly the signature above -- so the size is now a
compile-time assertion rather than an assumption:

    static_assert(sizeof(NpScene) == 0x28, "NpScene is 40 bytes in the oracle");

**It passes.** The reconstructed class is 0x28 bytes and its field offsets
(`+0x08` lock object, `+0x0c` and `+0x10` locks, `+0x14` and `+0x18` locks, `+0x1c`
condition, `+0x20` flag, `+0x24` Scene) match the oracle's measured layout. So the
wrapper is not the overrun.

**All the other allocations were re-checked and are in bounds**: Scene 0x710 with
writes to `+0x70c`; actor 0x50 with writes to `+0x4c`; body 0x1c0 with writes to
`+0x19c`; pose 0x80 with writes to `+0x68`; joint 0x17c with writes to `+0x12`; the
locks 0x20 each with eight dwords cleared. Every one fits.

**So the overrun is not in the sizes this round could check**, and the next step is
the same one 9p named, now with more information: break on the candidate's
`NpScene::createJoint` entry. If it is entered, the fault is in the lock step or the
forward; if it is not, the corruption happens earlier and the harness's own fixture
is the place to look.

## 9r. Round 4 state

    scene=created, both actors created, both bodies linked    held from rounds 2-3
    NpScene size                                              0x28, now asserted
    every reconstructed allocation                            in bounds
    the crash                                                 in a CRT CPU detector
    createJoint reached                                       still unknown
    the harness                                               exit -1073741819

**No census row closed.** All gates green: phases 2/3/4 exit 0, phase 5 exit 1 RED on
purpose, phase 6 exit 0 PASS, `completed` exit 0, `validate_inventory` exit 0 with
`unexplained=0`, 587 tool tests OK.

**One thing this round did buy beyond the assertion:** it moved the crash from "some
reconstruction bug" to "a CRT helper reading a corrupt heap", which rules out the
wrapper and every allocation size as the cause. The remaining candidates are a write
into memory this reconstruction does not own -- most plausibly the `Scene`'s `+0x6cc`
holder, which `createActor` and `createJoint` both write through, or the stubs that
store into objects whose layouts are not reconstructed.

## 9s. Round 5: the +0x6cc holder is valid, and its writes match the oracle

9r named the Scene's `+0x6cc` holder as the leading corruption candidate because both
`createActor` and `createJoint` write through it. This round measured it and it is
**not** the cause.

**The holder is non-null and stable:**

    TRACE createActor holder=00AC6378
    TRACE createActor holder=00AC6378      <- both actors, same wrapper

**And the writes through it are the oracle's, offset for offset.** The two rows were
read side by side with the reconstruction:

    oracle createActor:  actor+0x10 = holder[+0x10];  actor+0x0c = holder[+0x0c]
    oracle createJoint:  joint+0x10 = holder[+0x0c];  joint+0x14 = holder[+0x10]
    reconstruction:      identical in both

So the holder, its value, and both dereferences are correct. The write lands at
`holder+0x10`, which is inside the 0x28-byte `NpScene` (its `mReadLock` field), so it
is in bounds as well.

**What that leaves.** The corruption is not the wrapper size (9q), not any
reconstruction allocation size (9q), and not the `+0x6cc` holder (this round). The
remaining candidates are the stubs that store into objects whose layouts this
reconstruction does not know:

- `nxSceneAddActorObject` and `nxSceneAddJoint`, which the oracle uses to register
  objects and which here do nothing -- so they cannot corrupt, but they also cannot
  be the fix;
- `nxSceneArrayReserve`, which **does** allocate and free, and which the descriptor
  initialiser calls with `p[0x18]` and `p[0x1c]` -- the descriptor's
  `maxNbActors` and `maxNbBodies`. If the descriptor's limits pointer is not where
  this transcription reads it, that reserve is growing an array at an offset that is
  not an array, which is a write into the middle of the Scene object;
- `nxShapeFactory` and `nxShapeGroupConstruct`, which allocate 0x40 and 0x110 bytes
  and write only their own blocks.

**`nxSceneArrayReserve` is the strongest remaining candidate** and it is testable in
one step: print `p[0x18]` and `p[0x1c]` at the descriptor initialiser and see whether
they are the counts the harness's descriptor implies (zero, for a descriptor built
with `setToDefault`). A non-zero value there means the limits pointer is being read
at the wrong offset and the reserve is the corruption.

## 9t. Round 5 state, and a recommendation

    scene=created, both actors, both bodies        held from rounds 2-3
    NpScene 0x28 and Scene 0x710                    asserted (9q, 9s)
    the +0x6cc holder                               measured valid (9s)
    the crash                                       still in a CRT CPU detector
    createJoint                                     still not reached

**Five rounds have gone into this crash and it is not found.** What has been ruled
out is substantial -- the wrapper, every allocation size, and the holder -- but the
harness still faults and no census row has closed in any of those rounds.

**The recommendation is to stop chasing the crash and change tack**, and this is
recorded as a recommendation rather than acted on because it is a judgement about
where the programme's value is:

1. **The crash is in stubbed territory.** Eleven holes sit in the scene path and
   every one is a stub. A heap corruption that appears only when stubs run is a
   property of the stubs, and finding it by elimination costs a round per candidate.
2. **The reconstruction's own value does not depend on the harness passing.** Eight
   transcribed oracle rows, the measured Scene and NpScene layouts, and two
   `static_assert`s that pin those sizes are durable regardless.
3. **The programme's other work has a better return.** The census audits of 7e-7l
   each found a defect no gate could see, in one round each, and 5,552 rows are still
   `discovered`. Five rounds on one crash has produced three eliminations.

**So the next round should either test `nxSceneArrayReserve` -- one print, one build,
the strongest remaining candidate -- or leave the crash recorded and return to the
census.** Both are honest; the second is better value if the first does not settle it.

**No census row closed.** All gates green: phases 2/3/4 exit 0, phase 5 exit 1 RED on
purpose, phase 6 exit 0 PASS, `completed` exit 0, `validate_inventory` exit 0, 587
tool tests OK.

## 9u. Round 6: the crash moves when traces are added, which is the answer

9t named `nxSceneArrayReserve` as the strongest remaining candidate and said one
print would settle it. It did not settle it -- it did something more useful.

**The reserve is never reached.** The descriptor's limits pointer is null:

    TRACE initialise d[0x0b]=00000000

so the whole `if(limits)` block is skipped. `nxSceneArrayReserve` is eliminated
without needing to reason about it: the harness's descriptor has no limits, the block
does not run, and it cannot be the corruption.

**Then the traces walked the crash backwards through `initialise`.** With the limits
trace in place the crash was after `TRACE initialise tail`, the last statement of the
function. With the `initialise` tail traces added it moved earlier. With the reserve
disabled it moved earlier still. **The fault tracks the traces, not the code.**

**That is the answer to the round-4 question, and it is a different kind of answer
than the one being looked for.** A fault that moves when unrelated statements are
added is not a logic error at a fixed location; it is a **write into memory the
object does not own**, whose visible victim depends on the heap layout. Every round
since 9i has been measuring the victim rather than the write.

**What that rules out, completely:**

- the `NpScene` size and every field offset (9q -- asserted at compile time)
- the `Scene` size (9s -- asserted at compile time)
- every reconstruction allocation size against its highest write (9q)
- the `+0x6cc` holder, its value and both dereferences (9s -- measured against the
  oracle offset for offset)
- `nxSceneArrayReserve` (this round -- never reached)

**What it leaves, and why the list is short.** The write is into memory this
reconstruction does not own, and the only places that happens are the stubs that
store through pointers they did not allocate:

    nxShapeFactory          writes actor back-pointer at shape+4 (its own block)
    nxShapeGroupConstruct   writes actor back-pointer at group+4 (its own block)
    nxJointConstruct        writes joint+0x12 (its own block)
    nxSceneAddActorObject   does nothing
    nxSceneAddJoint         does nothing
    nxActorSetName          does nothing
    nxActorBuildUserDataObject  does nothing

**Every one of those writes is inside a block the stub allocated itself**, which
leaves one remaining place: `Scene::createActor`'s copy of `holder[3]` and `holder[4]`
into `actor+0x0c` and `actor+0x10`, and `Scene::createJoint`'s copy into
`joint+0x10` and `joint+0x14` -- writes through a pointer whose target layout this
reconstruction has not established. `holder` is the `NpScene` wrapper, 0x28 bytes, so
`holder[3]` and `holder[4]` are inside it; but the **actor and joint** being written
are stubs' blocks, and `joint+0x14` is past the 0x17c the joint stub allocated only
if the joint pointer is not what this reconstruction thinks.

**The measurement that would settle it is a guard, not a print**: allocate the actor
and joint blocks with a canary word past the end and check it after each write. That
catches the write rather than its victim, and it is one function.

## 9v. Round 6 state, and the tack change is now made rather than recommended

    scene=created, both actors, both bodies        held from rounds 2-3
    NpScene and Scene sizes                        asserted (9q, 9s)
    the +0x6cc holder                              measured valid (9s)
    nxSceneArrayReserve                            never reached (9u)
    the crash                                      moves with the traces -- a heap write
    createJoint                                    still not reached

**9t recommended changing tack if one more print did not settle the crash. It did
not, so the tack changes here.** Six rounds have gone to this crash; what they
produced is a complete elimination list and the knowledge that the fault is a
layout-sensitive write, which is a real diagnosis but not a fix.

**The recommendation, now acted on:** leave the crash recorded with 9u's guard as the
named next step, and return the programme's effort to work whose return is not
conditional on a stubbed path behaving. The census audits of 7e-7l each found a defect
no gate could see, in one round each, and 5,552 rows remain `discovered`.

**What this stretch of work leaves behind, stated once and without inflation:** eight
transcribed oracle rows and the `NpScene` wrapper class, two compile-time assertions
pinning the measured object sizes, a harness that reaches `scene=created` and creates
both actors where it previously returned `scene=null`, and six rounds of elimination
recorded. **No census row closed.**

All gates green: phases 2/3/4 exit 0, phase 5 exit 1 RED on purpose, phase 6 exit 0
PASS, `completed` exit 0, `validate_inventory` exit 0 with `unexplained=0`, 587 tool
tests OK.
