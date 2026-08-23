# Phase 3 leaf kernels: the mass, inertia and ray/segment intersection rows

Phase 3 Task 2, first dispatch. Sixteen of the twenty-seven Phase 3 exports are
reconstructed and every case the 299-case matrix has for them matches the
shipped oracle exactly. The differential moved from `stdout_delta=652` to
`stdout_delta=306`; the remaining eleven exports are untouched.

| repository | commit | contents |
| --- | --- | --- |
| `D:\github\Novodex` (`main`) | `6063805` | `Physics/src/MassProperties.cpp`, the `/arch:IA32` rule in `CMakeLists.txt` |
| `D:\github\Novodex` (`main`) | `a18e48a` | `Physics/src/Geometry.cpp` |

## The rows

| export | stable id | rva | cases | matched | mutation delta |
| --- | --- | --- | ---: | ---: | ---: |
| `NxComputeSphereMass` | `phys_fn_000803` | `0x0001ba90` | 9 | 9 | 2 |
| `NxComputeSphereDensity` | `phys_fn_000805` | `0x0001bab0` | 9 | 9 | 2 |
| `NxComputeBoxMass` | `phys_fn_000807` | `0x0001bad0` | 9 | 9 | 5 |
| `NxComputeBoxDensity` | `phys_fn_000809` | `0x0001bb00` | 9 | 9 | 4 |
| `NxComputeEllipsoidMass` | `phys_fn_000811` | `0x0001bb30` | 9 | 9 | 5 |
| `NxComputeEllipsoidDensity` | `phys_fn_000813` | `0x0001bb70` | 9 | 9 | 4 |
| `NxComputeCylinderMass` | `phys_fn_000815` | `0x0001bbb0` | 9 | 9 | 3 |
| `NxComputeCylinderDensity` | `phys_fn_000817` | `0x0001bbd0` | 9 | 9 | 3 |
| `NxComputeConeMass` | `phys_fn_000819` | `0x0001bbf0` | 9 | 9 | 3 |
| `NxComputeConeDensity` | `phys_fn_000821` | `0x0001bc10` | 9 | 9 | 2 |
| `NxComputeBoxInertiaTensor` | `phys_fn_000823` | `0x0001bc30` | 8 | 8 | 5 |
| `NxComputeSphereInertiaTensor` | `phys_fn_000825` | `0x0001bc80` | 10 | 10 | 3 |
| `NxRayPlaneIntersect` | `phys_fn_001704` | `0x00036bb0` | 11 | 11 | 8 |
| `NxSegmentPlaneIntersect` | `phys_fn_001706` | `0x00036c60` | 10 | 10 | 5 |
| `NxRaySphereIntersect` | `phys_fn_001710` | `0x00036e80` | 13 | 13 | 7 |
| `NxRayTriIntersect` | `phys_fn_001712` | `0x00036f50` | 15 | 15 | 6 |
| **total** | | | **157** | **157** | **67** |

All sixteen are leaves. Ghidra's `called` list is empty for every one of them, so
no reconstruction in this dispatch can reach anything else, the SDK lock
included. Inventory row states are not changed here: Phase 2 moved its rows at
its close and Phase 3 Task 4 owns the same step.

## The floating-point model, measured

The 2003 build is x87. Reproducing it needs two facts, and both were measured
against the pinned DLL rather than reasoned about.

**Precision.** `NxComputeSphereMass(0x40505969, 0x404c999e)` returns `0x43e70125`
from the shipped export. Evaluated with a rounding after every operation the same
expression gives `0x43e70124`; evaluated with the intermediates kept at 53-bit
precision it gives `0x43e70125`. Two further inputs agree
(`0x3feda98c/0x3ff1c90d` and `0x403d2baa/0x40496196`), and two positive controls
in the same process agree with both models, which rules out an instrument that
reports a difference for a reason unrelated to the model. The process reports
`_control87 == 0x0009001f`, whose `_MCW_PC` field is `_PC_53`, so an x87 register
holds exactly what a `double` holds.

The reconstruction therefore types every value the oracle keeps in a register
`double` and every value it stores to a 32-bit slot `NxReal`. That is not a
stylistic choice: the first transcription used `NxReal` throughout and the
randomized differential below rejected it on **525,995 of 4,800,000** checks,
every one of them a single ulp, at exactly the places where a `NxReal` temporary
narrowed a value the oracle keeps live.

**NaN payloads.** With the `double` typing in place and the default `/arch:SSE2`,
five checks still differed, all of the form `f(.., 0xfff914c8, 0x7fcba59a, ..)`:
two NaN operands, where x87 propagates the one with the larger significand and
SSE propagates the first source. No amount of C++ expresses that, so
`Geometry.cpp` and `MassProperties.cpp` alone are built `/arch:IA32`. Nothing
else in `NxPhysics` changes architecture, and `run_phase_gate.ps1 -Phase 2` still
passes.

**The 299-case matrix cannot see either difference.** Modelled in Python against
the recorded oracle values, a pure float32 evaluation of all twelve mass and
inertia kernels matches the oracle on every one of the 108 recorded cases, as
does the 53-bit model. Both would have passed the gate. The matrix is blind here
in the same way it is blind on the eight exports Task 1 named, and the blindness
is wider than that list.

## The 299-case matrix is not what closed these sixteen rows

Stated plainly, because the next implementer will otherwise assume a green
matrix is sufficient for the remaining eleven exports. It is not.

A reconstruction that types every intermediate `NxReal` -- narrowing to 32 bits
where the oracle keeps an x87 register live -- passes **all 157** of the matrix
cases these sixteen rows own. That is measured below, not argued. The matrix
confirmed these rows; it did not close them. What closed them is a second
differential, and it is committed.

## The instrument: `NxPhysicsKernelFuzzTests`

`D:\github\Novodex\tests\PhysicsKernelFuzzTests.cpp`, CMake target
`NxPhysicsKernelFuzzTests`, registered for Phase 3 in `tools/gate_targets.ps1`
beside `NxPhysicsGeometryTests`. It runs on the same staged-pair machinery as
the case matrix, so it is re-run by the command the phase plans already use:

```
cmake --build D:\github\Novodex\build --config Release --target NxPhysics NxPhysicsGeometryTests NxPhysicsKernelFuzzTests
powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Phase 3
```

It calls each export over seeded random inputs and prints one FNV-1a 64-bit
digest per export rather than a line per call, because three million lines is
not a transcript anyone reads. A digest that matches means every check behind it
agreed bit for bit, NaN payloads included.

**Reproducible, not merely repeated.** The generator is xorshift32 with the
seeds `13579bdf`, `02468ace` and `feedface`, one per block, and the iteration
counts are constants in the source (`120000`, `40000`, `60000`). The transcript
prints all six numbers on its first two lines, so a reader can see that the run
in front of them is the run described here. Against the shipped pair:

```
fuzz generator=xorshift32 scalar_iterations=120000 vector_iterations=40000 aimed_iterations=60000
fuzz seeds scalar=13579bdf vector=02468ace aimed=feedface
... 17 digest lines, 3,240,000 checks, every one matching the candidate ...
fuzz name=NxRayTriIntersect.aimed present=1 checks=480000 digest=85fb27ffd1c76541 hits=89978
```

The input mixture is deliberate: one branch returns a raw 32-bit pattern, which
produces NaN of both signs and arbitrary payload, infinity, denormals and
negative zero without any of them having to be enumerated; the rest keep the run
anchored on values a caller would really pass.

**The five precision witnesses are printed in full** rather than folded into a
digest, because the point of them is that a reader can see the value. Three
inputs separate the x87 `_PC_53` model from a float32 model in the last bit and
two agree under both models, so a transcript where the controls disagree is an
instrument fault rather than a finding.

**The aimed block exists because the general one was not enough.** A mutation to
`NxRayTriIntersect`'s tail was accepted by the random block, and the reason was
that random rays essentially never hit a random triangle: every early return was
taken, and the second barycentric, the reciprocal of the determinant and all
three writes were never reached. The aimed block puts the ray through a random
barycentric point of the triangle and prints its hit count -- 89,978 of 120,000
calls -- so a generator that stops hitting changes the transcript instead of
reading as a pass.

## Evidence that this instrument bites where the matrix does not

Two mutants, each built in a `git archive` copy of the implementation tree at
`a18e48a`, each run against both instruments in the same session. Neither real
tree was modified and both were asserted clean and at `HEAD` between runs.

**Mutant A -- the first transcription's mistake.** Every `double` register
lifetime in `Geometry.cpp` and `MassProperties.cpp` retyped `NxReal`, which is
exactly what the first cut of this work did.

| instrument | verdict |
| --- | --- |
| the 299-case matrix, 157 owned cases | **0 differ** -- completely green |
| `NxPhysicsKernelFuzzTests` | **11 of 17 digests differ** |

The eleven are `NxComputeBoxMass`, `NxComputeBoxDensity`,
`NxComputeEllipsoidMass`, `NxComputeEllipsoidDensity`,
`NxComputeBoxInertiaTensor`, `NxComputeSphereInertiaTensor`,
`NxRayPlaneIntersect`, `NxSegmentPlaneIntersect`, `NxRaySphereIntersect` and
`NxRayTriIntersect` on both of its blocks. The other six agree because this
mutant leaves `/arch:IA32` on, and for a kernel whose only `double` local is a
copy of an input the expression still evaluates in an x87 register -- those six
are carried by the flag rather than by the typing, and the witnesses agree for
the same reason. In the same run the matrix's positive control shows 142 unowned
cases differing, so its zero is a reading and not a stall.

The figure of **525,995 of 4,800,000** checks quoted elsewhere for the same
mistake came from the pre-commit scratch sweep and is **not reproducible from
anything committed** -- that sweep used a larger iteration count and a
directly-linked harness that no longer exists. It is kept only as the account of
how this instrument came to be written. The table above supersedes it and is the
reproducible form.

**Mutant B -- `/arch:IA32` dropped, everything else identical.**

| instrument | verdict |
| --- | --- |
| the 299-case matrix, 157 owned cases | **1 differs** |
| `NxPhysicsKernelFuzzTests` | **2 of 17 digests differ** |

The one matrix case is `NxSegmentPlaneIntersect.08`, whose plane normal is a
NaN: the oracle writes `7fc00000` into all three components of the point and the
SSE build writes `ffc00000`. That is the NaN-payload rule showing through, and
it corrects something the first draft of this task's report claimed -- the
architecture flag is *not* entirely unpinned by the committed matrix. It is
pinned by exactly one case out of 299, which is thin but not nothing. The fuzz
differential rejects the same mutant on `NxRayPlaneIntersect` and
`NxSegmentPlaneIntersect`. The witnesses agree under this mutant, correctly: the
`double` typing carries the precision on its own and the flag carries only the
NaN rule.

Taken together the two mutants separate the two halves of the model, and show
the case matrix seeing 1 of the 13 digests they move between them.

## What the disassembly says that mathematics does not

These are the places where a correct implementation and the oracle part company.
Each is reproduced.

**The extent product tests bits, not values.** `NxComputeBoxMass`,
`NxComputeBoxDensity`, `NxComputeEllipsoidMass` and `NxComputeEllipsoidDensity`
skip an extent with `cmp dword ptr [eax], 0` and `test ecx, ecx` — an integer
compare of the raw word. `-0.0f` is `0x80000000`, which is not zero, so a
negative-zero extent is multiplied in and flips the sign of the product where a
floating-point `!= 0.0f` would have skipped it. Ghidra renders these as
`extents->x != 0.0`, which is wrong for that one input.

**The first extent replaces the accumulator.** `fld 1.0; cmp; je +4; fstp st(0);
fld [eax]` — the 1.0 is discarded rather than multiplied by.

**`NxComputeBoxInertiaTensor` rounds `x*x` for `z` and not for `y`.**
`fst dword ptr [esp + 0x10]` at `0x0001bc5e` spills the square through a 32-bit
slot — reusing the dead `ylength` parameter slot — and `fld dword ptr [esp +
0x10]` at `0x0001bc6b` reads it back for the `z` component only. `y` uses the
register copy. The three components are not symmetric.

**`NxComputeSphereInertiaTensor` writes `x` twice.** `fld st(0); fstp dword ptr
[eax]` stores the unscaled `mass*r*r` before the `hollow` branch, and the branch
then overwrites it. A caller that aliases the output can observe the
intermediate. The scale multiplies the register copy, so the first store's
rounding does not feed the result, and `y` and `z` are then copied from `x`
*after* it has been narrowed.

**`NxRayTriIntersect` tests a sign bit, not a value.** Both range tests are
`fld st(0); fstp dword ptr [ecx]; mov eax, [ecx]; test eax, eax; js`. That is an
integer sign-bit test on the word just stored, so a barycentric of `-0.0f` is
rejected where `u < 0.0f` would have accepted it. The upper test then reads `u`
back out of the caller's float rather than using the register copy.

**`NxRayTriIntersect`'s two paths are not one path with two epsilons.** The
culled path stores the *unscaled* `u` and `v`, tests them, and only then
multiplies all three outputs by `1/det`; so a caller whose triangle fails the
second test is left with unscaled barycentrics in `u` and `v`. The non-culled
path scales as it goes and tests against `1.0`. The non-culled path also narrows
`1/det` to 32 bits at `0x00037152` where the culled path keeps it in a register.

**`NxSegmentPlaneIntersect` leaves `dist` untouched when the segment is
parallel.** `0x00036d02` copies `v1` into `pointOnPlane` with three integer moves
and returns without writing `dist` at all, so the caller cannot distinguish a
parallel segment from a hit by looking at `dist`.

**Vector results narrow one component and not the others.**
`NxRayPlaneIntersect` keeps `t*dir.x` in a register and pushes `t*dir.y` and
`t*dir.z` through 32-bit slots before adding the origin. `NxRaySphereIntersect`
does the mirror image — `z` is the unnarrowed one — and writes `z` *first*, then
`x`, then `y`.

**The plane kernels compare against a `double`.** `fcomp qword ptr [0x10107888]`
and `fcom qword ptr [0x10107880]` hold `-1e-7` and `+1e-7` as 8-byte constants,
so the parallelism test happens at register precision. The triangle kernel's
`1e-6` at `0x10106880` and `-1e-6` at `0x101079e8` are 4-byte.

**NaN takes the hit side of every threshold test.** `test ah, 0x41` / `test ah,
5` with `jne`/`jp`/`jnp` all leave the unordered case on the continuing branch.
The C++ short-circuit reproduces this without special-casing, which is worth
stating because it is a coincidence of the idiom and not a decision.

## A mutation and a measured delta for every row

Sixteen mutations, one per row, each perturbing that kernel alone. Built in a
`git archive` copy of the implementation tree under the scratchpad; neither the
implementation tree nor the evidence tree was modified for it, and both were
asserted clean and at `HEAD` between runs. The candidate and the mutant were run
against the same harness binary in the same session and both compared to the
committed oracle transcript.

The per-row deltas are in the table above; the totals are:

```
candidate vs oracle, the 157 owned cases : 0 differing
mutant    vs oracle, the 157 owned cases : 67 differing, every row non-zero
positive control, the 142 unowned cases  : 142 differing in the same run
```

The positive control is the point: 142 cases whose exports are not implemented
differ in the same run that reports 0 for the owned ones, so a reading of 0
cannot be an instrument that has stopped looking.

## Where the matrix is blind, beyond the eight Task 1 named

Task 1 recorded eight exports with no discriminating power against a textbook
implementation. Five of them are in this dispatch — `NxComputeSphereMass`,
`NxComputeSphereDensity`, `NxComputeConeMass`, `NxComputeConeDensity` and
`NxComputeSphereInertiaTensor`.

**Those five are no longer blind.** The disassembly is not their sole authority
any more: `NxPhysicsKernelFuzzTests` rejects all five under the realistic
float32 reconstruction — the one an unassisted implementer would actually write
— along with all three precision witnesses, where the 299-case matrix catches
that same reconstruction on 2 of 157 cases and on **0 of its 108** mass and
inertia cases. So the five have an oracle-side instrument that discriminates
them, and it is committed. What they do not have is discrimination *from the
case matrix*, and the two are not the same thing.

Two further blind spots in the matrix were found here:

1. **The floating-point model.** All 108 mass and inertia cases pass under a
   pure float32 evaluation. A reconstruction that rounds after every operation
   would have gone green.
2. **Operation order within a dot product, when the accumulator is wide.**
   Swapping the three terms of `v` in `NxRayTriIntersect`'s non-culled path
   changes nothing observable: the two orders differ by about one ulp of a
   `double`, and that is roughly 2^-28 relative to the `float` the result is
   stored into. The matrix does not see it and neither does a 1.6M-check
   randomized differential with a 75% hit rate. The order recorded in this file
   is the disassembly's, and it is recorded because it is right, not because
   anything can tell.

Both are properties of the case matrix rather than of the kernels, and the
second is a property of the arithmetic that no instrument can fix.

The second is the more useful of the two for the tasks that follow: the same
argument says that for any of these kernels the *summation order* of a dot
product is unobservable wherever the accumulator stays at register precision,
and observable wherever the oracle narrows between terms. That is a property of
the narrowing points, which are visible in the disassembly, and not of the
arithmetic.

## Two limits of this harness, and one departure from the brief

**It was blind to "wrote zero" versus "did not write" until it poisoned.** The
first committed cut zero-initialised every output buffer before each call, which
made those two outcomes the same transcript — and one kernel here turns on
exactly that distinction, since `NxSegmentPlaneIntersect` returns from its
parallel path without touching `dist`. Adding `dist = 0.0f;` to that path, which
is a change to shipped behaviour, moved **0 digests**. It now poisons every
output with the same `cdcd000N` ladder the case matrix uses and folds the poison
into the digest; the same mutant now moves `NxSegmentPlaneIntersect`. Recorded
because the plan nominates this harness as the instrument for the remaining
exports, and a limit that is fixed but undocumented is a limit waiting to come
back.

**Its coverage self-check could not fail the gate, and now can.** The aimed
block prints its hit count, but `run_differential.ps1` runs one binary against
both pairs, so anything the harness stops covering it stops covering
symmetrically: de-aiming the generator drops `hits` from `89978` to `3521` and
still reports `stdout_delta=0 stderr_exact=True`, a pass. No self-check inside a
symmetric differential can fail it — the structure forbids it. The recorded
coverage now lives in `$NxRequiredCoverageLines` in `tools/gate_targets.ps1` and
`run_phase_gate.ps1` requires each line twice, once per pair, before it will
report a pass. Verified by de-aiming the generator and running the gate:

```
GATE FAILED: requirement not met: NxPhysicsKernelFuzzTests reported its recorded coverage on both pairs (0 occurrences): hits=89978
```

The assertion runs before the differential's own exit code is checked, so lost
coverage is reported as itself rather than being hidden behind an unrelated RED.

**No `GeometryInternal.h` or `MassProperties.h` was created.** The brief asks for
both. All sixteen reconstructed functions are declared in the pinned public
headers — `NxInertiaTensor.h`, `NxIntersectionRayPlane.h`,
`NxIntersectionRaySphere.h`, `NxIntersectionRayTriangle.h` — and none of them
has an internal caller inside the reconstruction yet, so a private header would
have had nothing to declare and no one to include it. It is a departure from the
brief and is recorded as one rather than left silent; the first Phase 3 caller
that needs a shared declaration is the point to add it.

## The lock was not reached

`phys_fn_002362`, `phys_fn_002364` and `phys_fn_002366` were not entered. All
sixteen rows are leaves with an empty `called` list, so no path exists from any
of them. The escalation stands unchanged. The Phase 3 export with the only
plausible path to an allocator is `NxBuildSmoothNormals`, which calls
`operator new` at `0x000533dd`; it is not reconstructed in this dispatch.

## What remains

**One thing to carry into Task 3 that nothing else will remind anyone of.** The
`/arch:IA32` rule in `CMakeLists.txt` names three files, not a directory --
two when this paragraph was written, and `SmoothNormals.cpp` was added to it
in the second dispatch after a first measurement wrongly said it was not
needed. The
collision translation units Task 3 adds -- `BroadPhase.cpp`, `NarrowPhase.cpp`,
`ContactGeneration.cpp`, `Filtering.cpp` -- will build SSE2 and silently
disagree with the oracle on NaN payloads, and on the last bit of ordinary
results if they are written with `NxReal` temporaries. The 2003 oracle is x87
throughout, so the rule almost certainly needs extending; the note is repeated
at the rule itself, where whoever adds those files has to look.

Eleven exports, 142 cases, `stdout_delta=306`: `NxRayAABBIntersect`,
`NxRayAABBIntersect2`, `NxRayOBBIntersect`, `NxSegmentAABBIntersect`,
`NxSegmentBoxIntersect`, `NxSegmentOBBIntersect`, `NxRayCapsuleIntersect`,
`NxSweptSpheresIntersect`, `NxBoxBoxIntersect`, `NxSeparatingAxis` and
`NxBuildSmoothNormals`.

---

# Second dispatch: the box, capsule, swept sphere, separating axis and smooth normal rows

The remaining eleven exports. All 299 matrix cases and all 38 fuzz digests now
match the shipped oracle, so both Phase 3 differential targets read
`stdout_delta=0`.

| repository | commit | contents |
| --- | --- | --- |
| `D:\github\Novodex` (`main`) | `2c2665f` | `tests/PhysicsKernelFuzzTests.cpp`, five new blocks |
| `D:\github\Novodex` (`main`) | `c558efb` | `Physics/src/Geometry.cpp`, ten kernels |
| `D:\github\Novodex` (`main`) | `5e27fb1` | `Physics/src/SmoothNormals.cpp` |

## The rows

| export | stable id | rva | cases | matched | mutation delta |
| --- | --- | --- | ---: | ---: | ---: |
| `NxRayAABBIntersect` | `phys_fn_001722` (+`001724`) | `0x00037c80` | 13 | 13 | 2 digests |
| `NxRayAABBIntersect2` | `phys_fn_001726` (+`001728`) | `0x00037e70` | 13 | 13 | 2 digests |
| `NxSegmentAABBIntersect` | `phys_fn_001720` | `0x00037ab0` | 12 | 12 | 2 digests |
| `NxSegmentBoxIntersect` | `phys_fn_001714` | `0x00037260` | 13 | 13 | 4 cases, 2 digests |
| `NxRayOBBIntersect` | `phys_fn_001718` | `0x00037820` | 13 | 13 | 1 digest |
| `NxSegmentOBBIntersect` | `phys_fn_001716` | `0x00037550` | 13 | 13 | 1 digest |
| `NxRayCapsuleIntersect` | `phys_fn_001734` | `0x000381c0` | 13 | 13 | 2 digests |
| `NxSweptSpheresIntersect` | `phys_fn_001736` | `0x00038810` | 11 | 11 | 1 digest |
| `NxBoxBoxIntersect` | `phys_fn_001702` | `0x00036690` | 12 | 12 | 1 case, 2 digests |
| `NxSeparatingAxis` | `phys_fn_001696` (+`001698`, `001700`) | `0x000360e0` | 12 | 12 | 1 case, 2 digests |
| `NxBuildSmoothNormals` | `phys_fn_002146` | `0x000533c0` | 17 | 17 | 1 digest |
| **total** | | | **142** | **142** | |

Every mutant was built in a `git archive` copy under the scratchpad. The real
tree was asserted clean and at `HEAD` before and after each run, and the
un-mutated candidate read 0 differing in the same session that the mutant read
non-zero — so the zeroes are readings and not a stalled instrument.

## Inventory sizes understate three of these functions

The export table row is not the whole function for several of them. The bytes
past the stated size are separate inventory rows reached by a short `jmp` over
alignment padding, and reconstructing the export closes all of them:

| export | inventory size | real extent | note |
| --- | ---: | --- | --- |
| `NxRayAABBIntersect` | 317 | `0x00037c80`..`0x00037e64` | second loop and the out-of-line z branch; the continuation is `phys_fn_001724` at `0x00037dc0`, over the three-byte pad `phys_fn_001723` at `0x00037dbd` |
| `NxRayAABBIntersect2` | 317 | `0x00037e70`..`0x00038047` | same shape; the continuation is `phys_fn_001728` at `0x00037fb0`, over the three-byte pad `phys_fn_001727` at `0x00037fad` |
| `NxSeparatingAxis` | 269 | `0x000360e0`..`0x00036686` | `0x000361eb jmp 0x361f0` into the matrix loop; the body is `phys_fn_001698` and the argmax tail is `phys_fn_001700` |

Anyone disassembling only the stated size for `NxSeparatingAxis` sees a prologue
and no algorithm. Task 3 should expect the same pattern.

## `NxSeparatingAxis` with `fullTest` clear is not a function of its arguments

The most important finding in this dispatch, and it is an escalation rather than
a defect to reproduce.

Clearing `fullTest` skips the nine cross-axis stores at `0x000363d3` but does
**not** shorten the selection loop at `0x00036600`, which still scans all fifteen
slots. Six of them — `d[9]`..`d[14]` at `esp+0x8c`..`esp+0xa0` — hold whatever
the caller last left on the stack. On that path the shipped function's result
depends on call history, and no reimplementation can match it in general.

Three of the nine *are* deterministic and are reproduced: `d[6]`..`d[8]` land on
the prologue's spill of `rotation1`'s third column at `esp+0x80`..`0x88`. That is
directly confirmed by the recorded oracle transcript rather than argued:
`NxSeparatingAxis.05` passes `fullTest = 0` with `rotation1[8] = 1.0f` and
returns `9`, which is index 8 — the stale spill winning the argmax over every
real overlap in the case. Reproducing the spill makes that case pass.

`d[9]`..`d[14]` are seeded with `-FLT_MAX`, the only choice that is a function of
the arguments and the only one that cannot change the answer. Where the residue
happened to exceed the real maximum the oracle returns an axis this cannot.

Consequently the fuzz harness drives `NxSeparatingAxis` with `fullTest` set
only. That is a **measured** restriction: with both settings the digests differed;
with `fullTest` set they match over 60,000 calls. The limitation is recorded here
and at the call site rather than left as an unexplained asymmetry.

## Two asymmetries that no instrument can see

Reported in the same spirit as the first dispatch's finding that a dot-product
reordering in `NxRayTriIntersect` is unobservable.

`NxBoxBoxIntersect` associates `T[0]` as `(x + z) + y` where `T[1]` and `T[2]`
associate `(z + y) + x`, and `NxSeparatingAxis` associates the third column of
its matrix product differently from the first two. Mutations that removed both
asymmetries — making each uniform with its siblings — moved **zero** matrix cases
and **zero** digests. Both functions return a discrete value, a bool and an axis
index, so a one-ulp change to a dot product essentially never crosses a
comparison boundary.

Those two orderings therefore rest on the disassembly alone. They are
transcribed as the binary has them, and the rows were closed instead on
mutations that swap which box's extents build a face-axis radius, which move 1
case and 2 digests each.

## Things the oracle does that a correct implementation would not

All reproduced. Beyond the `NxSeparatingAxis` uninitialised read above:

- `NxRayAABBIntersect2` returns 0 when the ray origin is inside the box — the
  same value as a clean miss — and writes neither output, where
  `NxRayAABBIntersect` returns true and fills `coord` with the origin.
- Both ray/box kernels reject a candidate distance by testing the sign **bit** of
  the stored word at `0x00037da8`, so a quotient that flushed to `-0.0f` on the
  way into its 32-bit slot is rejected where `< 0.0f` would accept it. They also
  test the raw word of each direction component against zero, so `-0.0f` counts
  as a non-zero direction and is divided by.
- `NxSegmentBoxIntersect` writes all three components of `intercept` before
  testing any of them, so a `false` return that reached a candidate block leaves
  a point on a rejected plane in the caller's vector. Only the `q1 & q2` trivial
  rejection leaves it untouched. It also reads the candidate plane back out of
  `intercept` to form its numerator, which makes it sensitive to a caller that
  aliases the output with an input.
- `NxRayCapsuleIntersect`'s companion-vector branch taken when `|n.y| > |n.x|` is
  **missing its square root** — `0x0003826d` has an `fsqrt` and `0x00038291` does
  not — so the local frame is not orthonormal on a path about half of all inputs
  take, and every radius comparison downstream is scaled wrong. A capsule whose
  two points coincide takes the other branch, forms `1/sqrt(0)`, and never
  reports an intersection.
- `NxSweptSpheresIntersect` returns **true** for two stationary, separated
  spheres: the leading coefficient is zero, the reciprocal is an infinity, both
  roots become NaN and every remaining comparison falls the permissive way.
- `NxBuildSmoothNormals` bounds-checks no index, so an out-of-range face index
  read-modify-writes outside the caller's `normals` array. With both index arrays
  null it uses the constants 0, 1, 2 for every triangle rather than sequential
  ones, ignores `flip`, and still returns true. `dFaces` silently shadows
  `wFaces`.
- `NxBoxBoxIntersect` and `NxSegmentAABBIntersect` return **true** for any NaN
  input, on every path: every rejection is a strictly-ordered comparison and an
  unordered one always passes.

## `NxBuildSmoothNormals`: the allocator question, answered

This is the one Phase 3 export with a plausible path to an allocator, and the
first dispatch flagged it as the one to watch. It allocates, and **it does not
reach the SDK allocator**. The call at `0x000533fd` goes through an
incremental-link thunk to the CRT's own `operator new`, which reaches
`_nh_malloc` and then `HeapAlloc` on the CRT heap. There is no `NxUserAllocator`
function pointer and no SDK singleton on the path, and the matching
`operator delete` at `0x000537ef` tail-jumps into CRT `free`. There is no leak:
the only post-allocation early return is the allocation-failure path.

The `static_proof_closures_have_no_oracle_side` escalation is therefore
untouched by this dispatch as well.

## `/arch:IA32` DID need extending, and my first answer was wrong

**Corrected by measurement.** This section previously said `SmoothNormals.cpp`
was measured not to need the flag. The measurement was real and the conclusion
was wrong, in a way worth recording exactly.

`NxBuildSmoothNormals` was the only export in the harness not fed the raw-bit
`nxPick` mixture: `nxRunNormalsBlock` generated `nxUnit(&random) * 4.0f - 2.0f`
and nothing else, and all 17 matrix cases are finite too. So no committed
instrument had ever put a NaN, an infinity or a denormal into it. NaN payload
propagation is the one thing `/arch:IA32` governs, so the domain that was
measured was precisely the domain where the flag cannot matter. The claim that
"the only way a NaN payload is produced here is by propagation from an input
rather than by two NaN operands meeting" was false.

Worse, `SmoothNormals.cpp` already carried a comment asserting the file was
built `/arch:IA32`. The source was right and the build was wrong, and the
contradiction sat in the tree unremarked.

The normals block now draws half its meshes from the same raw-bit mixture every
other block uses. Measured against the shipped DLL, with only the
`set_source_files_properties` line differing between the last two rows:

```
oracle            digest=9218aa661fd0dcd9
candidate (SSE2)  digest=3c601a1d52339881
candidate (+IA32) digest=9218aa661fd0dcd9
iter=1384 word=13  oracle ffcad8b2 / SSE2 ffc00000 / IA32 ffcad8b2
```

Reproduced independently here as a committed mutant: removing
`Physics/src/SmoothNormals.cpp` from the rule moves **1 of 38 digests and 0 of
299 matrix cases**. The matrix cannot see this at all; only the randomized block
can, and only since it began generating non-finite vertices.

`CMakeLists.txt` now names three files. The lesson for Task 3 is not "add the
flag" but **check it over the domain the flag governs**: a build-flag experiment
run on finite inputs measures nothing.

### Two narrowings promoted from unobservable to measured

Improving that generator had a second effect. The two narrowings in
`angleAtVertex` — `C.y` at the cross product and the narrowed `B.z` in the dot
product — were silent against the finite-only generator. Against the current one
each is **independently observable**: removing either alone moves the
`NxBuildSmoothNormals` digest, as does removing both. Neither moves a matrix
case. They are now positively evidenced rather than transcribed on faith.

## A shipped behaviour that only the case matrix could have found

`NxBuildSmoothNormals.12` passes `flip` as a `bool` holding **2**. The oracle
normalises it with `setne dl` at `0x0005341d`, so any nonzero byte becomes
exactly 1. Written as `flip ? 1 : 0` the compiler is entitled to assume the byte
is already 0 or 1 and forward it unchanged; with `f = 2` the two index offsets
become `+3` and `+0`, the face collapses to a zero normal, and the case fails
while all 236,420 fuzz checks still pass — the harness constructs its `flip` from
a single random bit and so can never produce the value. The reconstruction reads
the byte through a `volatile` pointer, which is the only form that survived the
optimiser; a `memcpy` did not.

This is the mirror image of the first dispatch's lesson. The fuzz differential
closed the rows the matrix cannot see; here the matrix caught the one thing the
fuzz differential's generator could not reach.

## What remains

All 27 Phase 3 exports are reconstructed and both differential targets are green.
Phase 3 itself is **not** complete: Task 3's collision units — `BroadPhase.cpp`,
`NarrowPhase.cpp`, `ContactGeneration.cpp`, `Filtering.cpp` — are untouched, and
the Phase 3 function rows are still `discovered` in `inventory.json`.

Note that `run_phase_gate.ps1 -Phase 3` now reports `status=pass`. It is reading
the 27 exports, which are the only Phase 3 rows any differential target reaches;
it is structurally unable to see the remaining rows. A green Phase 3 gate at this
point means the exports are done, not that the phase is.

Task 3 should carry forward: `/arch:IA32` is still a two-file list and its
collision units will build SSE2 by default, and the check for whether that
matters is cheap — pick an input where the two floating-point models differ in
the last bit and compare against the shipped export, as was done here for
`SmoothNormals.cpp`.

## The coverage registration had a value collision and could not fail

Found by review, and it is the twelfth gate in this program caught unable to
fail — inside the fix for the eleventh.

`gate_targets.ps1` pinned bare counts. `hits=34575` is emitted by **both**
`NxSegmentAABBIntersect.aimed` and `NxSegmentBoxIntersect.aimed`, and
`run_phase_gate.ps1` matches by substring and requires two occurrences across
the two pair transcripts. Either export alone satisfied the requirement.
De-aiming only the segment/AABB generator drove it from 34575 to 92 — 99.7% of
the aimed path lost — and the gate still reported `status=pass`.

Three further gaps in the same registration: `NxSegmentBoxIntersect.aimed` had
`checks=` pinned but no hit count at all; `NxSeparatingAxis` and
`NxSeparatingAxis.aimed` had no registered line of any kind; and nothing in the
Python suite touched `$NxRequiredCoverageLines`.

**Why it survived my own liveness check.** I demonstrated the tripwire was live
by editing a pinned value and watching the assertion fail. That tests the
assertion statement, not the instrument. The strong form — degrade the generator
and see whether the gate notices — finds this in one run, and it is the form the
first dispatch used. Using the weak form was the error.

Fixed structurally rather than by correcting the numbers:

- every generator now prints `fuzz coverage name=<export> reached=<n>`, so the
  export name is inside the registered string and each is unique by
  construction;
- all twelve are registered, including the two `NxSeparatingAxis` blocks and
  `NxSegmentBoxIntersect.aimed`;
- `tools/tests/test_gate_targets.py` (5 tests, suite now **487**) asserts that no
  registered line is a substring of another, that every `nxCoverage` call in the
  harness has a registration, and that every registration still corresponds to a
  live generator.

Verified with the strong form: de-aiming the segment/AABB generator in a
`git archive` copy now drops it to `reached=5752` while
`NxSegmentBoxIntersect.aimed` stays at `34575`, and the gate fails naming the
segment/AABB line. The committed harness binary was restored afterwards and the
real source tree was asserted clean and at `HEAD` throughout.

The mutation runner had the same shape of defect: its no-op detector hashed only
`*.cpp` and `*.h`, so the `CMakeLists.txt` mutation above initially reported as
changing nothing. It now hashes every file.

## A warning for Tasks 3 and 4 about the `fullTest = false` matrix cases

`NxSeparatingAxis.05` passes `fullTest = 0` and the reconstruction matches it —
but only because the harness's own stack residue happens not to exceed the real
maximum, so the deterministic spill of `rotation1`'s third column wins the
argmax. That is a property of the *harness*, not of the reconstruction.

Reviewer's dynamic probe against the shipped DLL, identical arguments, only the
residue below the return address differing: dirt `-1e30` gives axis 9, dirt
`1e30` gives **10**, dirt `7.0` gives **10**. Residue of `7.0` is enough to
change the oracle's answer.

So an unrelated future edit to `PhysicsGeometryTests.cpp` — anything that changes
what sits at `esp+0x8c`..`0xa0` when the export is called — can turn Phase 3 red
for a reason nobody can fix in the reconstruction. If that happens, the case is
the thing to change, not the kernel. `NxBoxBoxIntersect` does **not** share the
defect and is safely driven at both `fullTest` values.
