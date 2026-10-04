# Phase 3 Task 3: the shape-pair dispatch matrix and the primitive overlap tests

The dispatch matrix is recovered in full — 72 slots across two 6x6 matrices,
every one checked against the oracle's own constructor at runtime. Nine of the
410 remaining Phase 3 rows are reconstructed and match the shipped DLL bit for
bit over 1,920,000 calls, folded into 3,240,000 digest words. The instrument
that measures them is new, because none of these rows is exported and no
existing instrument could reach them.

| repository | commit | contents |
| --- | --- | --- |
| `D:\github\Novodex` (`main`) | `0b3e61c` | `Physics/src/NarrowPhase.cpp`, `Physics/src/include/NarrowPhase.h`, `tests/PhysicsCollisionTests.cpp`, the CMake target and the `/arch:IA32` rule |
| `D:\github\Novodex` (`main`) | `aa2ac3b` | removes a duplicate architecture rule that made the first one untestable |
| `D:\github\Novodex` (`main`) | `1fb92b9` | drives the two step-reachable exports under both control words, corrects the architecture-rule comment, and replaces the tautological index check with a probe against the oracle's dispatcher |
| `D:\github\Novodex` (`main`) | `da92383`, `e389252` | matrix A `[SPHERE][SPHERE]` and `[SPHERE][BOX]`, the sphere/box contact geometry, the shape owner accessor and the continuous-CD guard, with the four blocks that drive them |

## The recovered dispatch matrix

The oracle keeps **two** 6x6 matrices of function pointers in one heap object.

- `phys_fn_002338` at `0x0005a8e0` is the constructor. It writes a vtable
  pointer at `+0x00`, zeroes 36 dwords from `+0x04` with `rep stosd` and 36 more
  from `+0x94`, then fills 39 of those 72 slots with immediates. 39 non-null and
  33 null is the split, and the harness checks both halves of it.
- The one call site is `phys_fn_000472` at `0x0000e733`, which allocates
  `0x124` bytes — exactly `4 + 2 * 36 * 4` — and stores the result at
  `0x00123c18`.
- `phys_fn_002348` at `0x0005ab80` is the dispatcher, and reads both halves.
- `phys_fn_002350` at `0x0005ae50` is a **second** reader: Phase 7, reached
  from `0x0001399c`, the persistent trigger-pair reconciliation pass. It reads
  the `+0x94` half only, with the same swap at `0x0005b28c`, the same
  `6 * type0 + type1` at `0x0005b2b7` and the same null skip at `0x0005b2c7`.
  Whoever owns trigger enter/stay/leave needs that one, not the dispatcher.

**The reader's rules**, from `0x0005ab88` onward:

1. **Pair symmetry.** If `shape0->type` (at `+0xd0`) is greater than
   `shape1->type`, the two are swapped. Only the ordered pair ever reaches a
   slot, which is why the lower triangle is null by construction rather than by
   omission.
2. **Which matrix.** If neither shape has any of the three low bits of its flag
   byte at `+0xde` set, the `+0x04` matrix is used; otherwise the `+0x94` one.
   The first is contact generation, `(shape0, shape1, contact sink, context)`;
   the second a boolean overlap test, `(shape0, shape1, context)`, result in
   `al`. A shape with a trigger bit gets the second. The two agree on their last
   argument — both receive the dispatcher's own `param_4` — so the overlap
   test's third **is** the contact path's fourth, and what contact generation
   has that the overlap test does not is the *third*: the sink, confirmed at
   `0x0004b876` feeding `call 0x0001d610`.
3. **The index.** `lea edx,[eax+eax*2]; lea edx,[eax+edx*2]` — that is
   `6 * type0 + type1`, row major, `type0 <= type1`.
4. **A null slot is a return, not a fault.** `test eax,eax; je` at `0x0005abd1`
   and `0x0005ac0b`. Plane against plane, and compound against compound in the
   overlap matrix, simply produce nothing.
5. On the overlap path a `true` result appends an **8-byte record** — the two
   shape pointers, stored at `0x0005ad7e` and `0x0005ad80` — to a trigger-pair
   array that grows by doubling. A `COMPOUND` second shape (type 5) is expanded
   into its children at `0x0005ac3b` and each child re-dispatched; the expansion
   is in the dispatcher, not in any matrix entry.

The type numbering is `NxShapeType` from the pinned public `NxShape.h`:
`NX_SHAPE_PLANE 0`, `SPHERE 1`, `BOX 2`, `CAPSULE 3`, `MESH 4`, `COMPOUND 5`,
`NX_SHAPE_COUNT 6`. `NX_SHAPE_COUNT` being 6 is what fixes the matrix order.

### Matrix A, at `+0x04` — contact generation

| | PLANE | SPHERE | BOX | CAPSULE | MESH | COMPOUND |
| --- | --- | --- | --- | --- | --- | --- |
| **PLANE** | *null* | `0x00048a70` `phys_fn_001901` | `0x00047f20` `phys_fn_001883` | `0x00048370` `phys_fn_001891` | `0x00048760` `phys_fn_001895` | `0x0003fa10` `phys_fn_001795` |
| **SPHERE** | | `0x0004b860` `phys_fn_001933` | `0x0004a2d0` `phys_fn_001919` | `0x0004a4b0` `phys_fn_001923` | `0x0004b1f0` `phys_fn_001929` | `0x0003fa10` `phys_fn_001795` |
| **BOX** | | | `0x0003add0` `phys_fn_001749` | `0x0003b260` `phys_fn_001753` | `0x0003d500` `phys_fn_001772` | `0x0003fa10` `phys_fn_001795` |
| **CAPSULE** | | | | `0x0003d9d0` `phys_fn_001775` | `0x0003e530` `phys_fn_001779` | `0x0003fa10` `phys_fn_001795` |
| **MESH** | | | | | `0x00046ab0` `phys_fn_001876` | `0x0003fa10` `phys_fn_001795` |
| **COMPOUND** | | | | | | `0x0003fa30` `phys_fn_001797` |

### Matrix B, at `+0x94` — the boolean overlap test

| | PLANE | SPHERE | BOX | CAPSULE | MESH | COMPOUND |
| --- | --- | --- | --- | --- | --- | --- |
| **PLANE** | *null* | `0x00048a20` `phys_fn_001899` | `0x00047e90` `phys_fn_001881` | `0x00048270` `phys_fn_001889` | `0x00048680` `phys_fn_001893` | `0x0003f570` `phys_fn_001787` |
| **SPHERE** | | `0x0004b800` `phys_fn_001931` | `0x00049e70` `phys_fn_001915` | `0x0004a3e0` `phys_fn_001921` | `0x0004a820` `phys_fn_001925` | `0x0003f5b0` `phys_fn_001789` |
| **BOX** | | | `0x000389d0` `phys_fn_001738` | `0x0003b0e0` `phys_fn_001751` | `0x0003bcd0` `phys_fn_001757` | `0x0003f700` `phys_fn_001791` |
| **CAPSULE** | | | | `0x0003d890` `phys_fn_001774` | `0x0003e370` `phys_fn_001777` | `0x0003f390` `phys_fn_001785` |
| **MESH** | | | | | `0x00046550` `phys_fn_001870` | `0x0003f570` `phys_fn_001787` |
| **COMPOUND** | | | | | | *null* |

Every non-null entry maps to an owned Phase 3 inventory row. There are no
unowned targets and no unexplained slots. Three things in these tables are worth
naming because a reimplementation would not produce them:

- **The two matrices are not symmetric in their null slots.** Compound against
  compound has a handler in A (`phys_fn_001797`, which expands both sides) and
  is **null** in B. A pair of compound triggers produces nothing.
- **`phys_fn_001795` fills five slots of A** and is not a per-pair kernel at
  all: it is what the dispatcher calls for a compound pair on the contact path.
  The expansion itself is in the dispatcher at `0x0005ac3b`, so calling these
  entries "expanders" is wrong — and the matrix B compound entries
  `0x0003f5b0`, `0x0003f700` and `0x0003f390` are ordinary overlap tests.
- **`phys_fn_001787` appears twice in B**, at plane/compound and at
  mesh/compound, and it **always returns false**: its body is an AABB-cache
  refresh, and every path through it reaches `xor al,al; ret` at `0x0003f5a5`.
  So two non-null slots of the overlap matrix can never report an overlap. That
  is a stronger oddity than a shared implementation, and it means a plane or a
  mesh will never fire a trigger against a compound shape.

### How it was recovered, four ways

1. The 39 `mov dword ptr [edx + N], imm32` in `phys_fn_002338`, whose offsets
   are exactly the upper triangle of two 36-entry arrays based at `+0x04` and
   `+0x94`.
2. The allocation size `0x124` at the single call site.
3. The two index computations and the swap in `phys_fn_002348`.
4. **At runtime.** `NxPhysicsCollisionTests` calls the oracle's own constructor
   on a 0x124-byte buffer poisoned with `0xcd` and compares all 72 slots against
   the table above, plus asserts that all 15 lower-triangle slots in each half
   are null. It reports `matrix slots=72 null=33 wrong=0` -- 39 non-null, 33 null.

The fourth is what makes the first three checkable rather than asserted, and it
runs in the gate.

## The instrument, and why it is a third kind of target

The 27 Phase 3 exports were closed by a staged-pair differential: one binary run
against the shipped pair and against the rebuilt pair, resolving everything with
`GetProcAddress`. That cannot reach a single row in this task. None of these
functions is exported, and the public path that would reach them — a scene, an
actor, a shape — is Phase 7, Phase 5 and Phase 4 work that does not exist.

`NxPhysicsCollisionTests` therefore holds the oracle in its own process: it
loads the pinned `NxPhysics.dll`, **re-hashes it and refuses to run unless the
hash is the pin** — which is the only thing that makes a recorded RVA mean
anything — and calls `base + rva` directly. The reconstruction is linked in and
the comparison is the harness's own.

That inverts one property for the better and leaves another unchanged:

- **Better.** A symmetric differential cannot fail on a self-check, because
  whatever the harness stops covering it stops covering against both pairs. This
  harness compares in-process, so a wrong reconstruction fails on its own exit
  code.
- **Unchanged.** It still cannot notice that it has stopped checking anything. A
  generator that degrades to nothing reports zero mismatches and exits 0.

So every generator prints a digest **over the oracle's own answers**, and those
are registered in `$NxRequiredCoverageLines`. An oracle-side digest cannot be
satisfied from the reconstruction side: it moves if the generator changes, if
the inputs change, or if the pinned DLL is not called. `run_phase_gate.ps1`
requires each of the 37 registered lines once (not twice — this target runs
once, not once per pair), and `tools/tests/test_gate_targets.py` now asserts
that every driven block has both a digest line and a coverage line, that every
registration still names a live block, and that **no digest registration reaches
past `oracle=<digest>`**, so a registration that could be satisfied by the
candidate is rejected structurally.

### Proof that the registration bites, by the strong form

Not by editing a pinned constant — that tests the assertion. The generator was
degraded and the gate asked whether it noticed.

The aimed block places the second shape at a separation either side of contact.
The mutant replaces that with a fixed far separation (`40..80` units), built and
run in a `git archive` copy. Its own exit code is **0**: no mismatches, because
both sides agree that nothing overlaps.

```
                        baseline                          de-aimed
sphere_sphere.aimed     true=68652 false=51348            true=0 false=120000
sphere_box.aimed        true=72338 false=47662            true=0 false=120000
sphere_capsule.aimed    true=47640 false=72360            true=0 false=120000
box_box.aimed           true=83894 false=36106            true=0 false=120000
```

The overlap branch is lost entirely on four rows, and the harness still passes
itself. Run through the gate's own coverage assertion:

```
registered=41 unmet=8
GATE FAILED: requirement not met: NxPhysicsCollisionTests reported its recorded oracle-side coverage (0 occurrences): collision name=sphere_sphere.aimed index=7 rva=0x0004b800 owner=phys_fn_001931 checks=120000 oracle=360355218ada0851
GATE FAILED: requirement not met: NxPhysicsCollisionTests reported its recorded oracle-side coverage (0 occurrences): collision coverage name=sphere_sphere.aimed true=68652 false=51348 swap_differs=0
```

against `registered=41 unmet=0` for the baseline transcript in the same session.
Eight of the forty-one, not all of them: this mutation degrades one aimed
generator, and the plane-aimed branch and the two direct-helper blocks have
their own. It is a demonstration that the registration bites, not that every
line of it does.

### Three further gate defects, found in review of the above

All three are the same shape as the twelve before them: a check that could not
fail. They are recorded here because the fix for each is structural.

**1. The new target class could be emptied and the phase still passed — the
fifteenth.** Every coverage assertion in `run_phase_gate.ps1` sits inside a loop
over a target list, and the "nothing registered cannot be gated" guard reads
only the staged-pair list. Removing `NxPhysicsCollisionTests` from
`$NxPhaseOracleDifferentialTargets['3']` skipped the run *and* the loop that
applies its registrations: **0 of 37 assertions evaluated**, `status=pass`,
python suite unchanged. Fixed with `$NxPhaseCoverageFloor`, a per-phase count of
assertions that must actually be evaluated, checked in the runner and pinned
independently in `test_gate_targets.py` so lowering it takes two visible edits.
Verified by the strong form — emptying the list now gives:

```
coverage_assertions_evaluated=18 floor=55
GATE FAILED: requirement not met: at least the recorded number of coverage
assertions ran (18 of 55); a target list that has been emptied evaluates none of them
```

**2. The twelfth defect was still live in the file that described it as fixed.**
`gate_targets.ps1` registered a bare `'hits=89978'`, and **nine** export lines in
`PhysicsKernelFuzzTests.cpp` end with ` hits=%u`, so any of them satisfied it.
The `fuzz coverage name=` lines were given export names to close exactly this
collision; this one line was missed. It is now one registration carrying the
name and the digest.

**3. The test that was supposed to prevent it could not see it.** Every check in
`test_gate_targets.py` compared registered lines *against each other* and never
against what the harness can print. It now parses the printf formats out of both
harnesses and requires each registration to be a prefix of exactly one of them —
which rejects `hits=89978` outright, since a mid-line fragment is a prefix of
nothing.

**4. `validate_inventory.py` did not know the new class.** Its registry parser
matched only `NxPhaseTestTargets|NxPhaseStaticProofTargets` and it had two proof
kinds, so Task 4 would have been rejected trying to close these nine rows
against `NxPhysicsCollisionTests`. It fails closed, so it was not a false pass —
but the class was added to the registry and the gate and not to the validator
that consumes them. There is now an `oracle_differential_falsified` kind whose
detection must be a non-zero `mismatches=` count, because an oracle differential
runs once and has no transcript to take a `stdout_delta` from.

## `/arch:IA32` does extend to `NarrowPhase.cpp`, and for a second reason

**The narrow phase does not run under the CRT default control word.** It is
reached only from the simulation step, and `phys_fn_000659` at `0x00013c40` —
the stepper in `Scene.cpp` — does this on its way in:

```
00013c4a  call 0x100106d0            ; save the caller's control word
00013c55  mov  [esp+4], 0x3ff33333   ; 1.9f
00013c5d  fld  dword [esp+4]
00013c64  fistp qword [esp+4]        ; 2 under round-to-nearest, 1 under chop
00013c76  cmp  [esp+4], 1
00013c7b  je   +6                    ; already chopping: skip
00013c7d  call [NxSetFPURoundingChop]
00013c83  call [NxSetFPUPrecision64]
...
00013df7  fldcw [esp+8]              ; restore on the way out
```

`NxSetFPUPrecision64` sets `PC = 0x0300` and `NxSetFPURoundingChop` sets
`RC = 0x0c00`. So the entire pipeline — broad phase, filter, dispatch, every
narrow-phase kernel — runs at **64-bit precision with round-toward-zero**. The
probe at `0x00013c55` is the oracle detecting the current rounding mode by
rounding 1.9f to an integer, which is also why the rounding call is conditional
and the precision call is not.

No C++ type names a 64-bit x87 significand: MSVC's `long double` is `double`.
The precision therefore cannot be written into the source at all. It does not
need to be — the control word is process state and x87 code follows it, which is
exactly how the oracle gets it. An SSE2 build ignores the control word outright.
So for this file the flag is not an improvement in the last bit; it is the only
way the file can follow the machine the oracle runs on.

**Measured, over the domain the flag governs.** Removing `NarrowPhase.cpp` from
the rule, built in a `git archive` copy:

| row | words compared | words differing with the flag removed |
| --- | ---: | ---: |
| `box_corner` (`phys_fn_000943`) | 360,000 | **82,337** |
| `sphere_capsule.random` (`phys_fn_001921`) | 120,000 | **27** |
| `plane_box.random` (`phys_fn_001881`) | 120,000 | **1** |

and the target exits 1. The oracle-side digests are unmoved, correctly — the
oracle does not change. (`box_corner`'s transcript prints `checks=1,440,000`,
which counts digest *bytes*; the 360,000 above is the number of 32-bit words,
which is what a mismatch counts. The two figures are not comparable and the
transcript should not be read as though they were.)

**What that number does and does not show.** It shows the flag matters. It does
*not* isolate the control word, because removing `/arch:IA32` changes code
generation **and** makes the control word inert at the same time — the two
effects cannot be separated by that mutation. The conclusion that the file must
be x87 rests on the logical argument, not on 82,337: an SSE2 build cannot follow
an x87 control word at all, whatever else it does. The number is evidence that
the codegen difference is observable, which is a weaker and separate claim.

**The first attempt at this measurement said the flag did not matter, and it was
wrong for two reasons, both worth recording.**

1. **A duplicate CMake rule masked it.** `set_source_files_properties` is
   per-directory, not per-target, so a second call naming the same file sets the
   same option again and silently overrides the first. The mutation that removed
   `NarrowPhase.cpp` from the first list moved zero rows until the duplicate was
   deleted. That is a build-level version of the same defect this program has
   caught twelve times at the assertion level: a rule that cannot be tested.
2. **The generator was not producing enough non-finite values.** The mixture
   Task 2 uses is a raw 32-bit word one time in eight, and a uniformly random
   word is NaN or infinity about one time in 256. Over a whole run that put
   **182** non-finite words into the only kernel here that writes floats. The
   NaN payload rule needs two NaN operands to meet, so that rate measures
   nothing — which is precisely how `SmoothNormals.cpp` was wrongly cleared in
   Task 2. The generator now has a branch that is deliberately non-finite, and
   the count went from 182 to **37,792**, which is registered.

The lesson from Task 2 was "check it over the right domain". The lesson from
here is that the *rule* has to be reachable by the experiment as well as the
domain being right.

## The window is wider than the narrow phase, and the matrix is what shows it

Three of the 27 Phase 3 exports are reached **from inside the step**, through
the matrix. Every edge below is a direct `call rel32`:

| export | reached from |
| --- | --- |
| `NxBoxBoxIntersect` (`phys_fn_001702`) | `0x00038a82` in `phys_fn_001738`, matrix B `[BOX][BOX]` |
| `NxBuildSmoothNormals` (`phys_fn_002146`) | `0x00052271` ← `0x0003c344` ← `0x0003d87c` in `phys_fn_001772`, matrix A `[BOX][MESH]` |
| `NxRayTriIntersect` (`phys_fn_001712`) | `0x00036e4b` ← `0x000415a2` ← `0x00042ea2` ← `0x0004366c` ← `0x00047067` in `phys_fn_001876`, matrix A `[MESH][MESH]` |

With the matrix edges removed, none of the 27 is reachable from
`phys_fn_000659`: the step reaches the matrix through a function pointer, so no
direct call edge crosses it. It is the recovered matrix and nothing else that
puts these three inside the window — which is why Task 2 could not have known,
and why its differential runs under the CRT default word only. Two files on the
`/arch:IA32` list, `Geometry.cpp` and `SmoothNormals.cpp`, are therefore inside
the window as well as outside it; only `MassProperties.cpp`, reached from
`Actor.cpp`, is outside it altogether. An earlier version of the comment on that
rule said all three were consumer-facing only, and that was wrong.

`NxBoxBoxIntersect` is now covered under both words through the `box_box` row.
The other two are driven directly, and doing so found something.

### `NxBuildSmoothNormals` is not closed under the simulation control word

```
collision name=step_ray_tri         checks=1560000 mismatches=0
collision coverage name=step_ray_tri         default_mismatches=0 simulate_mismatches=0
collision name=step_smooth_normals  checks=919352  mismatches=24
collision coverage name=step_smooth_normals  default_mismatches=0 simulate_mismatches=24
```

`NxRayTriIntersect` agrees under both. `NxBuildSmoothNormals` agrees on all
459,676 checks under `0x027f` and differs on **24** under `0x0f7f` — most of
them last-bit, and a few a cancellation that lands on exactly zero on the oracle
side only. Every one is on a mesh drawn from the non-finite mixture.

**It is not a transcription error and not a library call.** Two candidate causes
were tested and both moved zero:

- replacing `sqrt()` with the `fsqrt` intrinsic;
- replacing `atan2()` with an inline `fpatan`.

Both of those library routines really do ignore the x87 control word — measured
directly: with the word at `0x0f7f`, `sqrt(2.0)` is `3ff6a09e667f3bcd` from the
CRT and `3ff6a09e667f3bcc` from `fsqrt`, and they differ on four of six probe
values, while under `0x027f` they agree on all six. That is worth knowing for
anything else that calls them from inside the step. It is simply not what moves
these 24, and both experiments were reverted rather than left in unmeasured.

What is left is **where the compiler spills a `double`**. Under 53-bit precision
a spill is invisible: an x87 register and an 8-byte slot hold the same value.
Under 64-bit precision every spill truncates, and no C++ says where MSVC puts
them. `step_ray_tri`, on the same path but with fewer live values, agrees under
both — so the limit bites where register pressure is high rather than
everywhere, and all nine narrow-phase rows in this task agree under both words.

**This is an escalation, not a defect to fix in passing.** `phys_fn_002146` is
closed under one control word and cannot currently be closed under the other,
and the same question now hangs over every reconstructed kernel with enough live
values to force a spill.

### What this means structurally, which is new for this program

A discovery in Task 3 invalidated measurements in Task 2 that two independent
reviews had approved. Nobody was careless. The fact did not exist when those
measurements were made: the only path that puts `NxBuildSmoothNormals` and
`NxRayTriIntersect` under a different control word runs through a function
pointer, and the table that function pointer is read from was not recovered
until this task. Task 2 measured the right thing over the domain it could see.

**This is the risk Phase 2 was guarding against** when it declined to mark
anything `closed` and reserved that rung for the full-census audit. The guard
worked, and it worked for a reason worth naming: evidence in this program is
*relative to the reachability graph known at the time*, and the reachability
graph grows. A row's proof is provisional until the census is complete, because
a later phase can discover a caller that changes the machine model the row runs
under — not its inputs, not its callers' arguments, but the floating-point
environment itself.

Phases 4 through 7 should expect the same shape of event and should not treat it
as a regression when it happens. Two concrete consequences:

1. **A closed row's evidence should record which control word it was measured
   under.** None of Task 2's does, because there was no reason to. Anything
   closed from here on should.
2. **Recovering an indirect-dispatch table is not only a Phase 3 deliverable.**
   Every one of them widens the reachability graph and can move rows other
   phases have already closed. Phases 4, 6 and 7 all have their own vtables and
   pointer tables, and the same reopening should be expected each time one is
   read.

The gate handles it without lying in either direction: the default-word half
gates, and `simulate_mismatches=24` is *registered*, so it fails if it moves
either way — including toward zero, which would mean the generator had stopped
reaching the cases.

## Contact generation: the emitter and the first entry

Two rows, closed the same way the overlap rows were, with the borrowed layout
supplying what the emitter dereferences.

| row | rva | size | what |
| --- | --- | ---: | --- |
| `phys_fn_000873` | `0x0001d610` | 706 | the contact emitter, 22 call sites across 15 inventory rows |
| `phys_fn_001901` | `0x00048a70` | 177 | matrix A `[PLANE][SPHERE]` |

`NxPhysicsCollisionTests` compares **whole streams**, not counts. Each side gets
its own world -- shapes, owner, holder, collision objects, sink and stream -- so
neither can observe the other's addresses, and a run drives a sequence of up to
four pairs into one sink before resetting it. That is what makes the ordering
and orientation rules testable: with one pair per sink only the innermost level
is ever written.

```
collision name=contact_plane_sphere index=1 rva=0x00048a70 owner=phys_fn_001901
    checks=2452600 oracle=cb3db7abc1f39025 mismatches=0
collision coverage name=contact_plane_sphere emitted=59186 normal_blocks=48058
    headers=48058 negated_path=50404
```

`emitted - normal_blocks` is 11,128 contacts that correctly skipped **both** the
header and the normal block -- the ordering rule -- and `negated_path` is 50,404
pairs emitted against the body the sink is not oriented to, which is the
normal-orientation rule. `headers` equalling `normal_blocks` is correct rather
than suspicious: writing a header clears the cached normal at
`0x0001d77c`-`0x0001d782`, so a header always forces a block.

**Four behaviours that were recovered but unmeasured, now measured.** Review
found that each moved zero words, because the harness coupled things the emitter
keeps separate. All four are now driven apart and all four bite:

| behaviour | mutation | words moved |
| --- | --- | ---: |
| the header predicate is an **OR** over the two collision objects | `\|\|` to `&&` | 64,222 + 183,774 |
| the material is read from **shape1's** owner first, shape0's as fallback | swap which is consulted first | 44,874 + 72,508 |
| both feature ids must be real for the fifth word | `&&` to `\|\|` | 433,140 |
| the ids are **swapped** on the negated path | delete the swap | 12,156 |

What made them measurable: independent identities and materials per shape, a
null `owner->[8]` on one side in one call in eight, and a block that drives the
emitter directly with real feature ids — no matrix A entry reachable today
passes anything but `0xffff`, so the whole feature half was dead code.

**A counter that could not fail, replaced.** The coverage line used to report
`normal_blocks` and `headers` as buckets keyed on total words appended, and
`headers == normal_blocks` was offered as evidence that a header always forces a
block. It is not evidence: a header without a block appends 7 words and the
buckets counted it as neither, so the equality would have survived exactly the
failure it was cited against. The line is now a width histogram, and the missing
state does occur — `w7=12`, where the header's normal happened to equal the
cleared cache. `w4=5288`, `w8=2768`, `w11=51420`.

**Mutation.** Removing the separation's sign mask -- `separationBits &
0x7fffffff` becomes `separationBits` -- moves **59,186 of 2,452,600** words and
the target exits 1.

**The instrument, by the strong form.** Cutting the sequence length to one pair
degrades exactly what the block exists to test: the ordering paths stop being
reached, `emitted` falls from 59,186 to 21,808 and `normal_blocks` rises to meet
it, so no contact skips a header any more. Both the oracle-side digest and the
coverage line move and the gate fails on them -- while the harness's own exit
code stays 0, because a shorter sequence still agrees with the oracle.

**One limitation, stated.** The stream is pre-sized so the oracle's growth path
at `0x000b4de0` -- a Phase 2 row reaching the SDK allocator -- is never entered
on either side. That path is unexercised and a matching stream says nothing
about it.

## The eight remaining matrix A entries, surveyed

Surveyed before starting them, because two facts change how the work should be
planned and neither is visible from the dispatch table.

**Two of the nine do not call the emitter at all.** `plane/box`
(`phys_fn_001883`) and `box/box` (`phys_fn_001749`) **inline** the header and
append logic rather than calling `phys_fn_000873` — `plane/box` writes
`sink->[0x34]`, `sink->[0x20]` and `sink->[0x24]` itself at `0x00047fdf`,
`0x00047fec` and `0x00047ff5` and appends through its own `lea esi,[edi+0x38]`.
The other seven call the emitter, which is already closed. So the emitter being
done buys seven entries and not nine, and those two carry a second copy of a
row that is already reconstructed — which is worth knowing before someone plans
them as "transcription plus a driven block".

**The closure cost is not the entry size.** Own size against the new Phase 3
rows each entry drags in, with `phys_fn_000873`, `phys_fn_001901` and
`phys_fn_000943` already closed and excluded:

| entry | own | new Phase 3 rows | new Phase 3 bytes | other phases pulled in |
| --- | ---: | ---: | ---: | --- |
| `plane/box` | 42 (+786 in continuations) | 1 | 42 | — |
| `plane/capsule` | 773 | 1 | 773 | 2, 8 |
| `sphere/capsule` | 879 | 1 | 879 | 2, 8 |
| `capsule/capsule` | 2463 | 1 | 2463 | 2, 5, 8 |
| `sphere/sphere` | 341 | 7 | 4204 | 2, 4, 7, 8 |
| `sphere/box` | 259 | 8 | 5091 | 2, 4, 7, 8 |
| `box/capsule` | 2267 | 5 | 10700 | 2, 4, 8 |
| `box/box` | 775 | 10 | 12102 | 2, 4, 7, 8 |

The two smallest entries by their own size, `sphere/box` at 259 and
`sphere/sphere` at 341, are the **third and fourth most expensive** to close.
`sphere/box` reaches `phys_fn_001917` (969 bytes) for the geometry and
`phys_fn_002266` when a shape's `owner->[8]` is null; `box/box` drags in twelve
thousand bytes. Ordering the remaining work by entry size would take them in
almost exactly the wrong order.

**Two corrections to that last sentence, both from closing the two entries.**
`phys_fn_002266` is **not an error path** — it is the continuous-collision guard,
and the null `owner->[8]` only picks which shape goes first. And 3,802 of
`sphere/sphere`'s 4,204 bytes sit behind that guard, which is closed under the
SDK's own defaults, so the closure cost is real but *conditional*: a byte count
cannot tell a subtree that always runs from one that never does. See the
`sphere/sphere` and `sphere/box` section below.

**Cheapest first, and it is a family split anyway:** `plane/capsule`,
`sphere/capsule` and `capsule/capsule` each bring in exactly one new Phase 3 row
— themselves — and all three call the closed emitter. `plane/box` is 42 bytes
plus its own continuations and no new rows, but needs the inlined emitter
written a second time.

### The survey undercounted: four of the eight dispatch virtually

Found on the first entry of the capsule three, by reading it rather than by
trusting the table above. `plane/capsule` makes a **virtual call** at
`0x000484e6` — `call dword ptr [edx + 0x14]`, slot 5 of the partner shape's
vtable — and a closure walk over direct `call rel32` edges cannot see it. The
numbers in the table are therefore **lower bounds**, and they are lowest for
exactly the entries I recommended.

Four of the eight make such a call, and they are precisely the capsule pairs:

| entry | indirect call sites |
| --- | --- |
| `plane/capsule` | `0x000484e6` |
| `sphere/capsule` | `0x0004a624`, plus `NxComputeSquareDistance` at `0x0004a696` |
| `box/capsule` | `0x0003b3da` |
| `capsule/capsule` | `0x0003dbf9` |

`plane/box`, `sphere/sphere`, `sphere/box` and `box/box` make none.

**Slot 5 resolved per shape type**, from each constructor's vtable store:

| type | vtable | slot 5 | row | phase | size |
| --- | --- | --- | --- | --- | ---: |
| PLANE | `0x00107430` | `0x00025350` | `phys_fn_001261` | 3 | 203 |
| SPHERE | `0x00107528` | `0x00027c70` | `phys_fn_001377` | 3 | 314 |
| BOX | `0x00106ab8` | `0x00020880` | `phys_fn_000949` | **5** | 663 |
| CAPSULE | `0x00106b20` | `0x00022480` | `phys_fn_001010` | 3 | 321 |
| MESH | `0x00107630` | `0x00029230` | `phys_fn_001405` | 3 | 978 |

**Slot 5 is not the only slot in that vtable.** `phys_fn_002264` calls slot 7 at
`0x000562c3` and `0x000564b9`, and nothing in this program has resolved it for
any shape type. That is what makes the continuous-collision sweep a stop; see the
borrowed-layout section.

So the corrected cost of the capsule three, and the reason they are still the
right three:

| entry | own | slot 5 partner | corrected Phase 3 bytes |
| --- | ---: | --- | ---: |
| `plane/capsule` | 773 | `phys_fn_001261` (203) | 976 |
| `sphere/capsule` | 879 | `phys_fn_001377` (314) | 1193 |
| `capsule/capsule` | 2463 | `phys_fn_001010` (321) | 2784 |

All three stay inside Phase 3. **`box/capsule` does not**: its partner's slot 5
is `phys_fn_000949`, a **Phase 5** row of 663 bytes, so it would need either
another borrowing or a stop. That is worth knowing before it is picked up as the
fourth.

**The lesson, and it is the same one twice.** A closure over direct call edges
is not a dependency map for code that dispatches virtually, in the same way that
a reference scan that stops at two of three call sites is not a reference scan.
Both times the number looked authoritative and both times reading the function
corrected it.

### `box/capsule` is a stop, and the next dispatch should arrive knowing it

Its partner's vtable slot 5 is **`phys_fn_000949` at `0x00020880`, a Phase 5 row
of 663 bytes**. `box/capsule` cannot be closed without either borrowing that row
the way this task borrowed the shape and sink layout — with an address for every
offset it reads — or stopping. It is the only one of the eight in that position,
and it is cheap to plan around only if it is known in advance.

### `plane/capsule` decoded, ahead of transcribing it

The whole function is read; what follows is the shape of it, so the next
dispatch starts from a decode rather than from the disassembly.

**A byte flag at `capsule+0xe8` selects between two entirely different
algorithms** — `test al, 1` at `0x000483aa`, branching at `0x00048400`. The
overlap test at `0x00048270` never reads this byte, so nothing in this task's
existing coverage touches it. **What the flag means is not recoverable from this
function**: it is read, masked with 1, and branched on, and nothing here names
it. It should be resolved from whatever writes it, not inferred from which path
looks more plausible.

Both paths first build the two capsule endpoints exactly as the overlap test
does — same column-1 axis, same narrowing of the x half-axis at `0x000483a0`,
same narrowed `-axisZ` at `0x000483c6` — and the radius is stored over the
caller's second argument slot at `0x0004839c` and read back from there twice.

**Flag clear (`0x00048529`).** Plane distance for each endpoint against the
plane's `+0xe0` normal and `+0xec` constant; a contact is emitted for an
endpoint when its distance is **strictly less** than the radius
(`0x0004857f`, `0x000485f8`), so up to two contacts per pair. For each,
`point = endpoint - distance * normal` — the point on the **plane**, which is
the opposite convention to `plane/sphere`, where the point is on the sphere —
and `separation = distance - radius`. The normal handed to the emitter is the
plane's own, passed in place at `0x0004859a`. `dist0` stays in a register
throughout while `dist1` is pushed through the caller's first argument slot at
`0x00048572`, and the separation is written over the already-pushed third
argument at `0x000485db` rather than being pushed.

**Flag set (`0x00048406`).** Normalises the endpoint difference — `fsqrt` at
`0x00048456`, a zero test against the `0.0` at `0x001041f0`, then `1.0 /` the
length — and dispatches to the partner's vtable slot 5 at `0x000484e6` with five
arguments, emitting a single contact only if it returns non-zero. For a plane
partner that slot is `phys_fn_001261` at `0x00025350`, 203 bytes, whose only
callee is `NxRayPlaneIntersect` — a Task 2 export that is already closed. So the
path is a segment/plane raycast, and the row it needs is small and its
dependency is done.

**One dependency checked and cleared.** `sphere/box` calls `phys_fn_001281` at
`0x000257a0`, which is four bytes: `mov eax,[ecx+4]; ret` — `Shape::getOwner`,
reading `shape+0x04`. It then tests `owner->[8]`. Both offsets are already in
the borrowed section (`0x00025543` and `0x0001d729`), so this is not a stop.

## `plane/capsule`, transcribed and closed — two rows, 976 bytes

| row | rva | size | what |
| --- | --- | ---: | --- |
| `phys_fn_001891` | `0x00048370` | 773 | matrix A `[PLANE][CAPSULE]` |
| `phys_fn_001261` | `0x00025350` | 203 | vtable slot 5 of a PLANE shape, the segment raycast the entry dispatches to |

Both are in `Physics/src/ContactGeneration.cpp`. `NxPhysicsCollisionTests`
drives them as `contact_plane_capsule` and `shape_raycast_plane`, both under
`0x027f` and `0x0f7f`, and both agree with the pinned oracle on every check.

```
collision name=shape_raycast_plane index=- rva=0x00025350 owner=phys_fn_001261
    checks=5880000 oracle=569f78e4d5df74ec mismatches=0
collision coverage name=shape_raycast_plane hits=36662 wrote_normal=18252 aimed=22353
collision name=contact_plane_capsule index=3 rva=0x00048370 owner=phys_fn_001891
    checks=2180528 oracle=3d216c67a6297c4b mismatches=0
collision coverage name=contact_plane_capsule emitted=38664 one=14296 two=24368
    swept=49890 swept_emitted=8512 zero_axis=29366 w4=640 w8=1780 w11=13258 w15=22224
```

### The `+0xe8` flag is resolved, from what writes it

The decode stopped at "read, masked with 1, branched on, and nothing here names
it" and said to resolve it from whatever writes it rather than from which path
looks plausible. That was the right call and the answer is one function away.

`phys_fn_000989` at `0x00021ad0` — 110 bytes, a Phase 3 shape-class row — loads
a capsule shape's geometry from its descriptor:

```
00021ad8  mov  eax,[edi+0x4c]        ; desc.radius
00021adb  mov  [esi+0xe0],eax
00021ae1  fld  dword [edi+0x50]      ; desc.height
00021ae4  fmul dword [0x101043cc]    ; * 0.5
00021aea  fstp dword [esi+0xe4]
00021af0  mov  ecx,[edi+0x54]        ; desc.flags
00021af9  mov  [esi+0xe8],ecx
```

`NxCapsuleShapeDesc` declares `NxReal radius; NxReal height; NxU32 flags;` in
that order, so `+0x4c`/`+0x50`/`+0x54` fixes all three. **`capsule+0xe8` is
`NxCapsuleShapeDesc::flags`**, and the only bit `NxCapsuleShapeFlag` defines is
`NX_SWEPT_SHAPE = (1<<0)` — the bit `test al,1` reads. The header's own words
for it are that the capsule "represents a moving sphere, moving along the ray
defined by the capsule's positive Y axis", which is exactly what the flag-set
path does: it raycasts the axis segment. The row that writes it is a Phase 3
shape-class row (110 bytes) and is not reconstructed here; only the write is
quoted.

**Both paths are driven regardless**, as the brief required. The generator sets
bit 0 on half the pairs and randomises the other 31 bits, so "only bit 0 is
tested" is measured rather than assumed: `swept=49890` of roughly 99,800 pairs.

### The operation order, and where it narrows

Common to both paths, and the same endpoint construction as `phys_fn_001889`
with one difference: **`p1.z` is narrowed here** (`fstp [esp+0x44]` at
`0x000483fc`) where the overlap test leaves it in a register. Every one of the
six endpoint components goes through a 32-bit slot. The radius is stored over
the caller's second argument slot at `0x0004839c` — a fourth kernel storing past
its own frame — and the half height into a local at `0x00048388`.

**Flag clear, `0x00048529`.** Two plane distances, then up to two contacts.

1. `dist0 = ((p0.z*n.z + p0.y*n.y) + p0.x*n.x) + d`, left in `st(0)`.
2. `dist1` computed in the order `((p1.y*n.y + p1.z*n.z) + p1.x*n.x) + d` and
   **narrowed through the caller's first argument slot** at `0x00048572`. Every
   later use of it — the comparison, the three products and the separation —
   reads the 32-bit copy back. `dist0` never is. *The two endpoints are not
   treated at the same precision, and which one is wide is not symmetric.*
3. `fcom` + `test ah,5` + `jp` for each: emit iff `dist < radius` **strictly**,
   and an unordered comparison emits nothing.
4. `point = endpoint - dist * n`, so the point is **on the plane**. `x` and `y`
   use the wide product and `z` goes through a slot at `0x000485ac` and is read
   back at `0x000485cc` — the same per-component narrowing plane/sphere has.
5. `separation = dist - radius`, written over the already-pushed third argument
   at `0x000485db` rather than pushed.
6. The normal handed to the emitter is the plane's own, passed in place.

**Flag set, `0x00048406`.** One contact at most.

1. The endpoint difference, each component narrowed; the squared terms read the
   narrowed slots back, `fsqrt` at `0x00048456`, the length narrowed at
   `0x00048458`.
2. `fucompp` against the `0.0f` at `0x101041f0` with `jnp`: **a zero length
   skips the normalisation entirely and the raw difference is what reaches slot
   5**. An unordered comparison normalises.
3. `1.0 / length` is stored **over the radius slot** at `0x0004847b`, which is
   only safe because this path never reads the radius again.
4. `call dword ptr [edx+0x14]` at `0x000484e6` with five arguments: an
   `NxRay` assembled in the frame, the length as the distance limit, two zeros,
   and a **0x30-byte `NxRaycastHit`** at `[esp+0x30]` that exactly fills the
   rest of the frame up to the return address.
5. On a non-zero return, one contact: the point is `hit.worldImpact` and the
   separation is an immediate `push 0` at `0x00048513`, not a computed value.

### `phys_fn_001261`, and the two behaviours the entry cannot reach

`__thiscall`, `ret 0x14`, and it **returns `this`** (`mov eax,ebx` at
`0x00025415`) rather than a boolean. Three gates, in order, and every one lets
the unordered case through:

| gate | address | rule |
| --- | --- | --- |
| facing | `0x00025381` | `test ah,1` on C0 alone: continue iff `n·dir < 0` or unordered |
| ahead | `0x000253bb` | `test ah,0x41` + `jnp`: continue iff `dist > 0` or unordered |
| within | `0x000253cc` | `test ah,0x41` + `je`: continue iff `dist <= maxDist` or unordered |

Then it fills an `NxRaycastHit` whose field offsets match the pinned public
`NxUserRaycastReport.h` exactly — `shape` `+0x00`, `worldImpact` `+0x04`,
`worldNormal` `+0x10`, `faceID` `+0x1c`, `distance` `+0x20`, `u`/`v`
`+0x24`/`+0x28`, `flags` `+0x2c` — with `flags = 0x13`
(`SHAPE|IMPACT|DISTANCE`) and the normal copied and `|4` added only when the
caller's hint flags carry `NX_RAYCAST_NORMAL`. The third argument is never read
and is left unnamed.

**The entry cannot reach two of those.** `phys_fn_001891` always passes
`hintFlags = 0`, so the `NX_RAYCAST_NORMAL` branch at `0x000253fd` — the only
thing that ever writes `hit.worldNormal` — is dead from there; and it always
passes a distance limit equal to the segment length it just measured, so the
`within` gate is never anything but an exact fit. That is the same shape of gap
the feature ids had, so the row has its own block: `wrote_normal=18252` of
120,000 calls, and `aimed=22353` rays aimed at a point that really is on the
plane so the two distance gates are reached rather than rejected at the first.
The hit is `0xcd`-poisoned before every call, which matters here —
`NxRayPlaneIntersect` fills `worldImpact` before either distance gate runs, so a
raycast rejected for being behind the origin still leaves one field written, and
a harness that compared only the return value would not see it.

**No stop.** Between them the two rows read `shape+0x00` (the vtable pointer,
only to reach slot 5), `+0x10`/`+0x1c`/`+0x28` (column 1 of the pose rotation),
`+0x30`..`+0x38` (the translation), `+0x9c` (the collision object) and
`+0xe0`..`+0xec` (the geometry union). Every one is already established, the
first four in the borrowed-layout section and the union by `NarrowPhase.h`.

### Driving the differential through a vtable

The oracle side gets a shape whose slot 5 is the shipped DLL's own
`base + 0x25350`; the candidate side gets one whose slot 5 is the
reconstruction. So the entry and the row it dispatches to are covered together
and neither can be right by borrowing the other's answer. `swept_emitted=8512`
is the measurement that the oracle really does take the indirect call and get a
hit — not an inference from the disassembly.

The distinguishing inputs, each because a random pair does not reach it:

| input | why | how |
| --- | --- | --- |
| axis parallel to the plane normal | the two endpoints straddle, `dist0` and `dist1` differ by the whole length | column 1 set to `±n` |
| axis lying in the plane | both endpoints at the same distance, so the two contacts stand or fall together | column 1 set to `v - (v·n)n` |
| axis-aligned | a fixed axis the rotation never produces | identity rotation, axis `+Y` |
| zero axis | coincident endpoints without a zero half height | rotation zeroed |
| zero half height | coincident endpoints the other way | `geometry[1] = 0` |
| straddling placement | with the capsule centred at random neither endpoint is within a radius of the plane often enough to emit at all | centre at `n * (offset - d)` |

`one=14296` and `two=24368` are the oracle's own contact count moving by one and
by two. **The multiple-contact RED mode is now present**: it was listed absent
because it is a property of matrix A, and this is the first entry that can emit
twice from one call. `w15=22224` is a header, a normal block and two records in
one call, which no other block in this target can produce.

### Mutations, each with a measured delta

Built in a `git archive` copy under the scratchpad; the real tree was asserted
clean and at `HEAD` before and after every run, and the un-mutated copy was
built and run in the same session and read `mismatches=0` on every block.

| row | mutation | words moved |
| --- | --- | ---: |
| `phys_fn_001891` | the point scales the normal by the **radius** instead of the plane distance — the plane/sphere convention | **162,288** of 2,180,528 |
| `phys_fn_001261` | the facing test reads the ray **origin** where the oracle reads the direction | **438,954** of 5,880,000, and 31,400 more through the entry |

Both exit 1. The second moves both blocks because the mutated row is what the
entry's swept path dispatches into, which is itself the evidence that the vtable
edge is real.

### The registrations bite, by the strong form

Not by editing a pinned constant. Each generator was degraded in a copy and the
gate asked whether it noticed. In both cases **the harness's own exit code stays
0**, because a generator that stops reaching a path still agrees with the oracle
on the paths it does reach.

| degradation | effect | unmet registrations |
| --- | --- | --- |
| the generator never sets `NX_SWEPT_SHAPE` | `swept` 49,890 → **0**, `swept_emitted` 8,512 → **0**, digest `3d216c67…` → `49604e43…` | the two `contact_plane_capsule` lines, 2 of 49 |
| the raycast generator never aims | `aimed` 22,353 → **0**, `hits` 36,662 → 30,600, digest `569f78e4…` → `850410ad…` | the two `shape_raycast_plane` lines, 2 of 49 |

Run through the gate itself, the first gives

```
collision coverage name=contact_plane_capsule emitted=61029 one=11977 two=49052
    swept=0 swept_emitted=0 zero_axis=29366 w4=998 w8=4740 w11=10349 w15=42600
GATE FAILED: requirement not met: NxPhysicsCollisionTests reported its recorded
    oracle-side coverage (0 occurrences): collision name=contact_plane_capsule
    index=3 rva=0x00048370 owner=phys_fn_001891 checks=2180528 oracle=3d216c67a6297c4b
```

Two of forty-nine, not all of them, and that is the point: each degradation
loses the block it degrades and nothing else.

### The sixteenth gate that could not fail: a digest that was not independent

`phys_fn_001261`'s mutation moved the **oracle-side** digest of
`contact_plane_capsule`, from `3d216c67a6297c4b` to `16d5d97694b26193`, and its
`checks` from 2,180,528 to 2,077,464. The cause: all three contact blocks
digested **both** sides over the shorter of the two stream counts, so a
reconstruction that emitted fewer words truncated the oracle's own digest.

That could never have produced a false pass — a shortened digest does not equal
the pin either — but `gate_targets.ps1` says of these lines that "nothing on the
reconstruction side can produce it", and that was not true of the *value*. **A
differential whose anchor is not independent of the thing it judges is not a
differential.** Each side is now folded over its own count and only the overlap
is compared. **No pinned digest moved**: on any agreeing run the two counts are
equal, which is why the defect was invisible until a mutant made them differ.
Re-running the same mutation afterwards leaves
`oracle=3d216c67a6297c4b checks=2180528` standing while the candidate half moves.

**How it was found, because that is the part worth copying.** Not by reading the
loop — it had been read, and reviewed, three times. It was found because a
mutation aimed at `phys_fn_001261` moved a number it had no business moving, and
the transcript was read for *everything that changed* rather than only for the
mismatch count the mutation was there to produce. A mutation is a probe of the
instrument as well as of the row: any pinned quantity that moves for a reason
the mutation does not explain is a defect in the instrument, and the moment to
notice is while the mutant is still built.

**The general form, for the next digest-based block.** A digest is offered as
evidence that the oracle was called with these inputs this many times. It is
only that if **every input to the fold comes from the oracle side**. Three ways
this one nearly failed that test, of which the third was live:

1. an address in the digested data — solved by canonicalising the collision
   objects, and by giving `shape_raycast_plane` a made-up `hit.shape` value
   rather than a real pointer, so no load address reaches a pinned number;
2. a count taken from a shared loop bound;
3. **a length taken from the candidate** — the one that shipped.

The rule that catches all three: write the fold so that the candidate's data
structures are not in scope where the oracle's digest is computed. `nxFoldStream`
takes one world.

### Things the oracle does that a correct implementation would not

- **The two endpoints are not symmetric.** `dist0` is used wide and `dist1` is
  narrowed through the caller's argument slot, so the same geometry mirrored
  end-for-end does not produce mirrored separations.
- **The point convention is the opposite of plane/sphere's.** `plane/capsule`
  puts the contact on the plane and `plane/sphere` puts it on the sphere. Two
  entries in one matrix column disagreeing about where a contact point lives is
  not something a reimplementation would produce.
- **The swept path's separation is a literal zero**, not a computed penetration
  depth, so a swept capsule reports every contact as exactly touching.
- **The swept path destroys the radius** to hold `1/length`, and the radius is
  never consulted on that path at all — a swept capsule's radius does not affect
  its contacts.
- **A zero-length capsule is still raycast.** The guard at `0x0004846d` skips
  the normalisation but not the call, so slot 5 receives a zero direction and
  the entry pays for the indirect dispatch to be told nothing. With a finite
  plane the facing test rejects it there and then, since `n·0` is `0` and `0` is
  not less than `0`; with a non-finite plane normal the dot product is a NaN and
  the unordered branch carries it on into `NxRayPlaneIntersect`.
- **`hit.shape` is written from `shape->[0x9c]`**, the collision object, into a
  field the public header documents as the touched `NxShape`. Whether that
  object is what the public API hands out is a Phase 5 question; this row only
  records which pointer is stored.
- **Three of the raycast's gates admit NaN.** A NaN dot product, a NaN distance
  and a NaN limit each take the continuing branch, so a degenerate ray produces
  a hit record rather than a miss — the opposite of the convention four of the
  overlap entries use.

### `sphere/capsule` surveyed from the oracle before it was transcribed

`phys_fn_001923` at `0x0004a4b0` is the **same two-algorithm shape**:
`test byte ptr [edi+0xe8], 1` at `0x0004a4dc` and `je 0x0004a668`, with the same
column-1 endpoint construction, the same `fsqrt`, the same `0.0f` at
`0x101041f0` and the same `1.0f` at `0x101041ec`. Four differences that matter:

1. **Slot 5 is called on the *sphere*, not the capsule.** `esi` is loaded from
   `[esp+0x94]`, the first argument, at `0x0004a5ea`, and the call is
   `call dword ptr [eax+0x14]` at `0x0004a624`. So the partner row is
   `phys_fn_001377` at `0x00027c70`, 314 bytes, Phase 3.
2. **It passes `hintFlags = 4`** — `push 4` at `0x0004a60a`, where
   `plane/capsule` pushes `0`. So `NX_RAYCAST_NORMAL` is set and the normal the
   emitter is handed is `hit.worldNormal`: the hit buffer is `[esp+0x60]`
   (`0x0004a5fd`), the point is `[esp+0x64]` and the normal `[esp+0x70]`, which
   is `hit+0x04` and `hit+0x10`. **That independently confirms the
   `NxRaycastHit` layout** this task recovered from `phys_fn_001261`, from a
   second caller that uses a field the first one never asks for.
3. The separation is `push 0` again at `0x0004a64f`, so the swept convention —
   every swept contact reported as exactly touching — is not specific to planes.
4. The non-swept path at `0x0004a668` sums the two radii, calls Foundation's
   **`NxComputeSquareDistance` through the IAT slot at `0x10104170`**
   (`0x0004a696`) and compares `r*r` against the squared distance with
   `test ah,0x41` + `jne`. That is the same common-mode dependency the caveat
   below records for `phys_fn_001921`: this harness links the reconstruction,
   which links `NxFoundation`, so both sides share that callee and a defect in
   it would cancel.

`phys_fn_001377`'s own callee is a direct `call 0x00036e80` with five `__cdecl`
arguments (`add esp,0x14` at `0x00027c9a`) taking the ray, `ray+0x0c`, the
sphere's translation, its radius and `hit+0x04`. That is **`phys_fn_001710`,
the `NxRaySphereIntersect` export** — the inventory names it and the PE export
table confirms it — which Task 2 closed and drove bit-exact. So the two slot-5
targets are symmetric and both dependencies are done: the PLANE one dispatches
to `NxRayPlaneIntersect` and the SPHERE one to `NxRaySphereIntersect`.

## `sphere/capsule`, transcribed and closed - two rows, 1,193 bytes

| row | rva | size | what |
| --- | --- | ---: | --- |
| `phys_fn_001923` | `0x0004a4b0` | 879 | matrix A `[SPHERE][CAPSULE]` |
| `phys_fn_001377` | `0x00027c70` | 314 | vtable slot 5 of a SPHERE shape |

```
collision name=shape_raycast_sphere index=- rva=0x00027c70 owner=phys_fn_001377
    checks=5880000 oracle=6bee7065d00d060e mismatches=13
collision coverage name=shape_raycast_sphere hits=54437 wrote_normal=27073
    aimed=22568 behind=5666 default_mismatches=0 simulate_mismatches=13
collision name=contact_sphere_capsule index=9 rva=0x0004a4b0 owner=phys_fn_001923
    checks=1802896 oracle=6a6da9567aa6eb2d mismatches=0
collision coverage name=contact_sphere_capsule emitted=38192 swept=50378
    swept_emitted=17834 zero_axis=34458 coincident=4074 beyond_end=11046
    w4=1 w8=3127 w11=35064
```

### The operation order

**Flag clear, `0x0004a668`, one contact.** `radiusSum = sphereRadius +
capsuleRadius` narrowed; `NxComputeSquareDistance(segment, centre, &t)` through
the IAT; `radiusSum * radiusSum > squared` **strictly**, and unordered emits
nothing. Then the closest point at the returned parameter - **x narrowed after
its multiply at `0x0004a6dc`, z narrowed before it at `0x0004a6d2`, y not
narrowed at all** - the normal `closest - centre` with its z left wide, a length
formed from the *narrowed* x and y and the wide z, and **a zero-length normal
returns without emitting** (`0x0004a811`), which is the opposite of the swept
path's guard. `point = centre + sphereRadius * normal`, on the **sphere**,
scaled by the sphere's own radius and not by the sum that decided the test. The
separation is `sqrt(squared) - radiusSum`, with the root re-taken at
`0x0004a7f3` from the narrowed squared distance rather than kept from anything.

**Flag set, `0x0004a541`, at most one contact.** Byte for byte plane/capsule's,
with four differences: slot 5 is called on the **sphere**; it is passed
`hintFlags = 4` (`push 4` at `0x0004a60a`) where plane/capsule passes `0`, so
the emitter's normal is `hit.worldNormal` at `hit+0x10` and the point is
`hit+0x04`; the inverse length is stored over the half height's slot rather than
the radius's; and **the capsule's radius is never read on this path at all**.
The separation is an immediate `push 0` again.

`phys_fn_001377` is not the plane's function with a different intersector. It
has **no facing test and no ahead-of-origin test** - only the distance limit -
it writes `hit.distance` **before** testing that limit (`fst` at `0x00027cc9`,
`fcomp` at `0x00027ccc`), it compares the wide root while the hit gets the
narrowed one, and it computes and normalises its own normal, leaving a
zero-length one unnormalised **with `NX_RAYCAST_NORMAL` still set**
(`0x00027d94`). Its normal length is accumulated x, y, z from the *stored*
components where the impact distance is accumulated z, y, x from registers.

**No stop.** Between them the two rows read the shape vtable, rotation column 1,
the translation, `+0x9c` and `+0xe0`..`+0xec`. Every one is already established.

### Distinguishing inputs

The sphere is placed in the **capsule's own frame**: `along` in half heights, so
`|along| > 1` is past an endpoint (`beyond_end=11046`) and the rest is the
interior, and `across` perpendicular in units of the two radii, with zero
putting the centre exactly on the axis (`coincident=4074`) - the only input that
reaches the zero-length-normal return. Four axis modes including a **skew axis
that is not a rotation column at all**, since only `m[1]`, `m[4]` and `m[7]` are
read and nothing requires them to be unit. `zero_axis=34458` covers a zero half
height and a zeroed rotation, which are not the same input to the endpoint
construction.

### Mutations

| row | mutation | words moved |
| --- | --- | ---: |
| `phys_fn_001923` | the contact point is scaled by the **radius sum** instead of by the sphere's own radius | **54,001** of 1,802,896 |
| `phys_fn_001377` | the normal is taken against the **ray origin** instead of the sphere centre | **47,687** of 5,880,000, 23,766 of them under the gating default word |

Positive control in the same session: the un-mutated `git archive` copy read
`mismatches=0` on every block. Degrading both generators - never setting
`NX_SWEPT_SHAPE`, never aiming a ray - leaves **exactly the four sphere/capsule
registrations unmet, 4 of 53**, while the harness's own exit code is unmoved by
the loss of coverage.

### `NxRaySphereIntersect` is a fourth Task 2 export inside the simulation step

`phys_fn_001377` gates on its **default-word half only**. Under `0x027f` it
agrees on all 2,940,000 checks; under `0x0f7f` it differs on **13**, and every
one of the thirteen is `hit.worldImpact` or the `hit.distance` derived from it -
never a field the row computes for itself. `worldImpact` is written by
`NxRaySphereIntersect`, `phys_fn_001710` at `0x00036e80`, and the only path that
puts it under a different control word is matrix A `[SPHERE][CAPSULE]` to vtable
slot 5 to that export. **That edge did not exist before this task.**

So the list of Task 2 exports the recovered matrix moved is now four, not three:
`NxBoxBoxIntersect`, `NxBuildSmoothNormals`, `NxRayTriIntersect` and
`NxRaySphereIntersect` - and the last of them is reached through **two** levels
of indirection, a function pointer and then a vtable slot, which is why no
closure walk found it. Its reconstruction takes `sqrt()` of the discriminant and
keeps the root wide across a subtraction; the CRT routine ignores the x87
control word, and no C++ hands a 64-bit significand to the operation after it.
That is Task 2's row and the same escalation `NxBuildSmoothNormals` already
carries, so `simulate_mismatches=13` is registered rather than dropped: it fails
if it moves either way, including toward zero.

**One honest limitation on the entry.** `contact_sphere_capsule` reads
`mismatches=0` under both words with the registered generator, but the
*degraded* generator above - which drives the non-swept path harder - reads
**6**. So that zero is a measurement over the driven domain and not a proof of
exactness under `0x0f7f`, and the six are the same class: a root the oracle
keeps at 64 bits and no C++ can.

### Square roots: `fsqrt`, not the CRT

Every square root in `ContactGeneration.cpp` goes through an inline `fsqrt`.
This is not a last-bit nicety. The CRT's `sqrt()` ignores the x87 control word -
Task 3 measured that directly - and under round-toward-zero the difference
crosses float boundaries far more often than under round-to-nearest, because
chopping lands on exactly representable values. Measured: with the CRT routine
`phys_fn_001377` moved **95** words and `phys_fn_001923` **10**; with `fsqrt`
they moved 13 and 0.

**The first form of that helper was wrong, and the instrument caught it.**
Leaving the root in `st(0)` and omitting the `return` is the documented MSVC
idiom for returning a double out of inline assembly. It makes the caller
responsible for popping a register the compiler did not put there, and whether
it does depends on inlining. The helper now stores its result.

## The seventeenth gate that could not fail: a digest that was not reproducible

Found while chasing the above, and it is the more serious of the two found in
this dispatch.

`contact_plane_capsule`'s **oracle-side** digest changed from
`3d216c67a6297c4b` to `98fe2a9a73c62b4b` when `ContactGeneration.cpp` was
recompiled - with every branch count, every width and the word count unchanged.
A pinned oracle digest that moves when the *reconstruction* is rebuilt is not
evidence of anything.

**Four measurements, in order, because each one killed a hypothesis:**

1. **Not an x87 leak.** A stack-depth probe read `TOP` before and after every
   call: always 0.
2. **Not uninitialised stack.** The 8 KB the callee's frame would occupy was
   filled with `0xa5`, `0x00` and `0xcc` from one binary across three runs. The
   digest did not move.
3. **Not the object addresses.** One binary, two runs, the world placed at two
   different addresses. The digest did not move, so `nxCanonical` was not
   rewriting a coordinate that happened to look like a pointer.
4. **Not the candidate.** With the candidate call deleted entirely, the oracle
   alone still produced `98fe...` in one build and `3d21...` in another.

That last one is what named it: the **inputs** were moving. The generator aimed
the capsule by multiplying the plane's own normal - `n[k] * (offset - d)` for
the placement, `±n` and `v - (v·n)n` for the axis - and one draw in four makes
that normal non-finite. SSE propagates the NaN payload of one operand or the
other depending on which the compiler put first, so the *generated shape*
differed between two builds of the harness, and both sides saw the same changed
shape - which is exactly why `mismatches` stayed 0 and every counter matched.

**The fix is a rule, not a patch:** a generator may not do arithmetic on values
it also allows to be non-finite. Both capsule blocks now aim only when the
inputs are finite and fall back to raw draws otherwise, which keeps the
non-finite coverage without putting it through a multiply. Verified by rebuilding
with a deliberate perturbation: the digests `6ddd7e0506885f19` and
`6a6da9567aa6eb2d` now hold.

**Why the older blocks never showed it.** The overlap rows return a boolean, so
a NaN payload cannot reach their digest - only the classification can, and that
is stable. `contact_plane_sphere` and `contact_emit` do arithmetic only on tame
values and take their non-finite inputs as raw bit patterns. `plane/capsule` was
the first block to aim *through* a value it also allowed to be a NaN.

## `capsule/capsule` and the first deferred-proof discharge

`phys_fn_001775` at `0x0003d9d0`, 2,463 bytes, was the third of the three this
dispatch was asked to close. It is **not** closeable the way the first two were,
and the reason is one direct call edge the entry survey saw but did not weigh.

**Its callees, read out of the byte range rather than inferred:**

| target | row | phase | size | state |
| --- | --- | --- | ---: | --- |
| `0x0001d610` | `phys_fn_000873` | 3 | 706 | closed, the emitter |
| `0x00033e80` | `phys_fn_001690` | **2** | **1,836** | **not reconstructed** |
| `0x00001000` | `phys_fn_000001` | 2 | 48 | not reconstructed |
| `0x00001030` | `phys_fn_000002` | 5 | 49 | not reconstructed |
| `[edx+0x14]` at `0x0003dbf9` | `phys_fn_001010` | 3 | 321 | slot 5 of a CAPSULE shape |

`phys_fn_001690` is the 1,836-byte Phase 2 segment/segment distance this file
already names as matrix B's capsule/capsule dependency. **Matrix A needs it
too.**

**This is not the same thing as `box/capsule`'s stop, and calling it one was
wrong.** Phase 2 did not fail to close this row; it *deferred* it, with a
recorded reason (`homeless_shared_code`), a recorded reachability
(`unreachable_from_phase_2`) and a recorded list of phases that could discharge
it — `[3, 4]`, Phase 3 first. Phase 2 closed 59 rows and deferred 1,155, and the
deferral ledger exists so that a later phase can pick one up when it creates the
reachability Phase 2 lacked. `capsule/capsule` **is** that reachability. The
right description is a deferred proof coming due, and the right action is to
reconstruct the row and discharge the deferral — the first time in this program
that has happened.

`box/capsule` is different: its dependency is a Phase 5 row that no ledger has
named Phase 3 as a driver for.

### The mechanism does not currently allow the discharge, and that is a real gap

Checked rather than assumed, because the whole plan rests on it.
`validate_inventory.py` enforces two rules over each phase's closure ledger, and
together they make a cross-phase discharge impossible to record:

```
owned  = {row.id for row in census if row.phase == phase}
stray  = (closed | deferred) - owned      -> "the phase N closure ledger names
                                             rows that are not phase N"
unaccounted = owned - closed - deferred   -> "phase N rows are neither closed
                                             nor deferred"
```

So **Phase 3's ledger may not name `phys_fn_001690` at all** — `stray` rejects
it, because the census says the row is Phase 2's. And **Phase 2's ledger may not
drop it** — `unaccounted` requires every owned row to appear. The two records
would have to disagree, or the validator would reject the phase. `driving_phases`
names who *may* discharge a deferral and nothing anywhere records that one
*has*.

**Where the discharge belongs, and why.** On **Phase 2's ledger**, moving the row
from `deferred` to `closed`, with a new field naming the phase that produced the
proof and the evidence file it lives in. Three reasons, in order of weight:

1. **Ownership is a census fact, not a work assignment.** `stray` exists to keep
   one row owned by exactly one phase; letting a later phase adopt rows would
   dissolve the invariant the whole census rests on. The row stays Phase 2's.
2. **The proof is what moved, not the ownership.** A deferral is a claim that no
   falsifiable proof was available. Discharging it is the claim that one now is.
   That belongs where the original claim was made, so a reader of Phase 2's
   ledger sees the current state without having to search every later phase.
3. **The counts stay honest.** `counts` is recomputed from the ledger, so Phase
   2's closed/deferred totals move with the discharge automatically, and the
   1,155 stops being a number that only ever grows stale.

The mechanical change is small: `CLOSED_KEYS` and `DEFERRED_KEYS` are closed
vocabularies, so a `discharged_by_phase` field has to be added to the schema and
validated — it must name a phase that was in the original `driving_phases`, and
it must not be the owning phase. That is one edit to `validate_inventory.py`, one
to Phase 2's ledger, and a test. **It is not done here**, because it should land
with the row it discharges rather than ahead of it.

This sets the pattern for every later phase that inherits a deferral, which is
why it is written down before anything relies on it.

**It is done now**, and one detail of the plan above turned out to be
insufficient: `discharged_by_phase` alone cannot be checked, because once the
deferred entry is gone there is nothing left to check the named phase against.
The original `driving_phases` travels with the closed entry for that reason. See
`phys_fn_001690, and the first deferred proof discharged` below.

**The survey's "exactly one new Phase 3 row — themselves" was right for
`plane/capsule` and `sphere/capsule` and wrong for this one.** The table did
record "other phases pulled in: 2, 5, 8" for this entry; what it did not do was
say that one of those is 1,836 bytes and sits directly under the kernel rather
than under a leaf. Counting only *new Phase 3* bytes made the third entry look
like the same shape of job as the first two, and it is not.

That is the **third** counting method in this program that looked authoritative
and was incomplete, after the reference scan that stopped at two of three call
sites and the closure over direct call edges that could not see virtual
dispatch. All three failed the same way: **the method answered a narrower
question than the one being asked.** A cost table that counts one phase's bytes
is not a cost table; a scan that stops early is not a scan; a closure over one
edge kind is not a closure. The general guard is to state what a number counts
in the same sentence that reports it.

### What was recovered before stopping, so it is not found twice

1. **The two capsules' flag bytes are OR'ed together.**
   `mov ecx,[ebx+0xe8]; mov esi,[ebp+0xe8]; or ecx,esi; test cl,1` at
   `0x0003da1e`–`0x0003da3c`. So the swept path is taken when **either** capsule
   carries `NX_SWEPT_SHAPE`, not when a particular one does. A generator that
   sets the flag on both together, or on shape0 only, drives three of the four
   combinations into one branch and cannot tell the OR from an AND — which is
   the same defect the emitter's header predicate had.
2. **The receiver of the vtable call is chosen at runtime.**
   `lea edi,[esp + edx*4 + 0xb4]; mov ecx,[edi]; mov edx,[ecx]; call [edx+0x14]`
   at `0x0003dbde`–`0x0003dbf9`. The two shape pointers are held in an indexed
   pair and `edx` selects which one is raycast, so slot 5 is called on *one* of
   the two capsules and which one is a decision the kernel makes. Both orders
   have to be driven.
3. **It passes `hintFlags = 4`** (`push 4` at `0x0003dbcf`), like
   `sphere/capsule` and unlike `plane/capsule`, so the normal comes back in the
   hit.
4. **Both capsules' endpoints are built first**, `0x0003d9e1`–`0x0003dacf`, with
   the same column-1 axis, the same narrowing and the same `-axisZ` as the two
   entries already closed. The second capsule's construction is at
   `0x0003da7f` onward and is byte-for-byte the first's with `ebp` for `ebx`.

### `phys_fn_001010`, the CAPSULE slot 5 — decoded, not transcribed

`__thiscall`, `ret 0x14`, 321 bytes, and it **returns `this`** like the other
two. It builds its own segment from the receiving capsule and calls
`0x000381c0` with four `__cdecl` arguments (`add esp,0x10` at `0x00022521`).

**That target is `phys_fn_001734`, the `NxRayCapsuleIntersect` export**, which
Task 2 closed and drove bit-exact. So this row's dependency is done — and
`NxRayCapsuleIntersect` becomes the **fifth** Task 2 export the recovered matrix
puts inside the simulation step, after `NxBoxBoxIntersect`,
`NxBuildSmoothNormals`, `NxRayTriIntersect` and `NxRaySphereIntersect`. Like the
fourth, it is reached through two levels of indirection and no closure walk
finds it.

Its result is a **three-way** switch on the export's return value
(`0x00022524`–`0x0002252f`): zero returns no hit; `1` takes the first of two
distances directly; anything else compares the two with `fcomp` + `test ah,5` +
`jp` and takes the smaller. Then the chosen distance is compared against the
caller's limit with `test ah,0x41` + `jne` — the same "less, equal or unordered"
rule the other two slot-5 rows use — and the hit is filled with
`flags = 0x13`, `impact = origin + distance * direction`, `faceID`/`u`/`v` zero
and `shape` from `+0x9c`. **It does not write a normal at all**, whatever the
hint flags say, which is a third convention across three slot-5 rows: the
plane's copies the plane's geometry under a flag test, the sphere's computes and
normalises one under the same test, and the capsule's has no normal branch in
its 321 bytes.

That last point matters for whoever transcribes it, because `phys_fn_001775`
passes `hintFlags = 4` and then reads a normal out of the hit that this callee
never wrote. Whether the emitter is handed an uninitialised normal, or the entry
computes one itself between the call and the emit, is the first question to
answer and it was not answered here.

**It is answered below, from the instruction stream, and the answer is that the
emitter is handed uninitialised frame memory** — and then measured twice from
the oracle: the hit's normal words survive a `0xcd` poison on every one of
32,821 hits, and a stack seed the harness alternates comes back out in 10,008 of
10,174 swept contact emissions.

## `phys_fn_001690`, and the first deferred proof discharged

The row this file said was a deferred proof coming due is reconstructed and the
deferral is discharged. It is a **Phase 2** row and it stays one: the census
owns it, Phase 2's closure ledger owns it, and what moved is the proof.

| repository | commit | contents |
| --- | --- | --- |
| `D:\github\Novodex` (`main`) | `b1d79b0` | `NxSegmentSegmentSquareDistance` in `Physics/src/NarrowPhase.cpp`, its declaration, and the `segment_segment` block |
| `D:\github\Novodex` (`main`) | `3adc1b5` | the `canonical_nan` counter the coverage line registers |
| evidence worktree | `1b76961f` | `discharged_by_phase`, its validation and its tests; the Phase 2 ledger, record and programme counters; the two registrations and the coverage floor |

```
collision name=segment_segment index=- rva=0x00033e80 owner=phys_fn_001690
    checks=2160000 oracle=f97e71b61b01a17c candidate=28a5a764fb5fb25a mismatches=662
collision coverage name=segment_segment parallel=8903 degenerate=12015 null_params=7323
    interior=22793 clamped_s=58807 clamped_t=47928 non_finite=22038 canonical_nan=42009
    default_mismatches=0 simulate_mismatches=662
```

### The discharge mechanism, as landed

The gap this file recorded was real and the fix is the one it proposed. The
discharge lands on **Phase 2's** ledger, moving the row from `deferred` to
`closed`, because ownership is a census fact that `stray` exists to protect,
because a deferral is a claim that no falsifiable proof was available and the
retraction belongs where the claim was made, and because `counts` is recomputed
from the ledger so Phase 2's totals correct themselves. Phase 2 now reads
`closed=60 deferred=1154`.

**Two fields, not one.** `discharged_by_phase` names the phase that produced the
proof; the original `driving_phases` travels with the row. The second is what
makes the first checkable — once the deferred entry is gone there is nothing
else left to check a discharging phase against, and a `discharged_by_phase`
nobody can contradict is the same shape of defect as the deleted `observed`
proof kind. `validate_inventory.py` requires that they appear together, that the
discharging phase is one the deferral named, and that it is **not** the owning
phase; a phase closing its own row is a closure and has no business claiming
otherwise. Nothing else is added: the row still needs a mutation and a non-zero
`mismatches=` count like any other closure, and the `gate` field — which must
already be a registered target — is what points a reader at the evidence.

**Proved by the strong form**, by breaking each rule in a copy of the tool and
asking whether the suite noticed:

| rule removed | result |
| --- | --- |
| the discharging phase must be in `driving_phases` | `test_rejects_a_discharge_by_a_phase_the_deferral_never_named` fails: `'the deferral named 3, 4 as the phases that could' not in []` |
| the discharging phase must not be the owner | `test_rejects_a_phase_discharging_its_own_deferral` fails: `'discharged its own deferral' not in []` |
| the two fields must appear together | two tests fail, on a `KeyError` in the broken copy rather than on a stated defect — the shipped version reports it |

There is also a test that the row does **not** move: putting a discharged entry
on the discharging phase's ledger is still rejected by `stray`, and that is the
invariant the whole census rests on.

Python suite **505**, up from 496 by the nine tests above.

### The row

`phys_fn_001690` at `0x00033e80`, 1,836 bytes: the squared distance between two
segments, with the closest-point parameter of each written back through pointers
it null-checks. It is the region decomposition of
`Q(s,t) = a s² + 2b st + c t² + 2d s + 2e t + f` over the unit square, with `s`
and `t` carried scaled by `det = |ac - b²|` until the interior case divides them
down, nine leaves for the non-degenerate case and a **second tree** at
`0x000343cf` for `det < 1e-5f` — the constant at `0x00107938`.

**Where it narrows, and where it does not.** `D0`, segment 0's direction, never
touches memory: all three components stay in x87 registers from `0x00033e8b` to
`0x00033f4b`, so `a`, `b` and `d` are formed from wide operands while `D1`, `W`,
`c` and `e` go through 32-bit slots. `f = |W|²` stays in `st(0)` from
`0x00033f67` until the `faddp` at `0x00034589`, and every one of the nineteen
leaves adds into it in place rather than reloading it.

**Two stores past its own frame**, both into the caller's argument slots: `t` at
`0x00033fdf` and `d` at `0x00033f43`. Both are dead — the two segment pointers
were loaded at `0x00033e83` — and both are writes into the caller's stack.

**Operation order at the two branch points**, because both admit NaN and they
admit it differently:

| test | address | rule |
| --- | --- | --- |
| near-parallel | `0x00033f81` | `fcomp` the **wide** `det` against `1e-5f`, `test ah,1`: the second tree is taken when `det < eps` **or unordered** |
| `s >= 0` | `0x00033fe3` | `test ah,1`: unordered goes to the `s < 0` sub-tree |
| `s <= det` | `0x00033ff7` | `test ah,0x41` + `jp`: unordered goes to the `s > det` sub-tree |
| the edge tests | `0x000342e8`, `0x00034224` | `test ah,5` + `jp`: these continue only on a **strictly** negative edge, so zero and unordered both fall into the clamped leaf |

### Distinguishing inputs driven, and why each is there

| input | why a random pair does not reach it | count |
| --- | --- | ---: |
| near-parallel axes | `det` is under `1e-5f` for essentially no pair of independently drawn segments, so the whole second tree is unreachable without aiming | `parallel=8903` |
| a zero-length segment on either side or both | `a` or `c` is then zero and the leaves that divide by it are entered with a zero divisor — the state a capsule with a zero half height builds | `degenerate=12015` |
| a null output pointer | `test eax,eax` at `0x0003458f` and `0x0003459c`; **no caller in the census passes null**, so nothing but a direct drive reaches those two branches | `null_params=7323` |
| both closest points in the interior | the aimed mode puts segment 1 through a point on segment 0 | `interior=22793` |
| non-finite components | the raw-draw modes | `non_finite=22038` |

`clamped_s=58807` and `clamped_t=47928` are the oracle's own parameters landing
exactly on 0 or 1, which is the boundary half of the decomposition; the
classification is read off the oracle's answers rather than computed here.

Every aim is gated on segment 0's direction being finite, under the rule the
seventeenth gate defect produced: a generator may not do arithmetic on values it
also allows to be non-finite.

### Mutation, with a measured delta and a positive control

Built in a `git archive` copy at `3adc1b5` under the scratchpad; the real tree
was asserted clean and at `HEAD` before and after.

| mutation | words moved |
| --- | ---: |
| `t` reads the **wide** `e` still in `st(0)` where the oracle reloads the narrowed copy at `0x00033fd5` | **75,122** of 2,160,000, **25,887** of them under the gating default word |

The target exits 1. The un-mutated copy of the same tree, built and run in the
same session, read `mismatches=0` — so the zero is a reading and not a stalled
instrument. The mutation was aimed at the row's most distinctive property: `s`
at `0x00033fbc` multiplies the wide `e` and `t` at `0x00033fd5` reloads the
narrowed one, so the same geometry mirrored end for end does not produce
mirrored parameters.

### The registrations bite, by the strong form

Not by editing a pinned constant. The generator was degraded so it never picks
the near-parallel mode, and the gate's own assertion loop was run over the
resulting transcript:

```
collision coverage name=segment_segment parallel=0 degenerate=11882 null_params=7518
    interior=29025 clamped_s=54465 clamped_t=42768 non_finite=22141 canonical_nan=42237
    default_mismatches=0 simulate_mismatches=713
GATE FAILED: requirement not met: NxPhysicsCollisionTests reported its recorded oracle-side
    coverage (0 occurrences): collision name=segment_segment index=- rva=0x00033e80
    owner=phys_fn_001690 checks=2160000 oracle=f97e71b61b01a17c
registered=55 unmet=2
```

against `registered=55 unmet=0` for the control transcript in the same session.
**The degraded harness's own exit code is 0** — a generator that stops reaching
a branch still agrees with the oracle on the branches it does reach — and two of
fifty-five registrations are lost, not all of them.

### Two things this transcription cannot reproduce, both measured

**1. Which of two x87 NaNs comes out.** The rule is the larger significand with
ties going to the **destination** operand, and no C++ names the destination of an
x87 instruction. MSVC emitted `fsubr` where the oracle has `fsub` on the very
first subtraction in the function, and folded `x + (-y)` into `fsub` for the two
negated dot products — and `FSUB` leaves a NaN operand's sign alone where the
oracle's `FADD` of an already-negated NaN carries the flip. Three spellings of
those dot products were built and measured against the oracle:

| spelling | parameter words moved on non-finite inputs | on finite inputs |
| --- | ---: | ---: |
| the literal one, three negated products | 3,500 | 0 |
| one negated sum, `-((a+b)+c)` | 3,500 | 0 |
| an explicit sign-bit flip of the rounded sum | 5,956 | 0 |

All three are exactly equal in IEEE arithmetic, and none of them is what the
binary does, because what the binary does is a property of the instruction
encoding. The row is transcribed in the literal form and the differential
compares NaN against NaN as NaN: `nxCanonicalWide` and `nxCanonicalNarrow`
rewrite a NaN — and only a NaN, never an infinity, whose sign the row *does*
reproduce — to one pattern on both sides.

**This is a deliberate weakening of the instrument, and its cost is measured
rather than assumed.** What is given up is exactly the payload and sign of a NaN
result or parameter, and the price of giving it up is the 3,500 words the
strictest available spelling still moved. What is kept is everything else: which
leaf ran, whether a result is a NaN at all, both infinities, and every finite
value, all still compared bit for bit — and `default_mismatches=0` over 1,080,000
checks is a statement about that domain, not about a domain the comparison was
narrowed to fit. `canonical_nan=42009` is registered so that a generator which
stops producing non-finite inputs fails the gate instead of quietly making the
comparison stricter, which is the only way a weakening like this can be stopped
from spreading.

**2. The row is closed on algorithmic agreement under a control word it does not
execute under, with the real-context divergence pinned at 662.** That is the
whole claim and it should be inherited in those words rather than as a green
row.

Under `0x027f` the reconstruction agrees on all 1,080,000 checks. Under `0x0f7f`
it differs on **662**. The algorithm agrees — every branch, every leaf, every
parameter, and every finite value under the word a consumer would see. What does
not agree is the bit pattern under the word the row runs under, and the source of
the divergence is **code generation, not source semantics**: MSVC spills a
`double` to an 8-byte slot, which truncates a 64-bit significand to 53, and no
C++ construct says where the spills go or stops them.

`phys_fn_001690` is the hard case of the three rows that now carry a pinned
non-zero divergence under `0x0f7f` — 13 for `phys_fn_001377`, 24 for
`phys_fn_002146`, 662 for this one. The first two are also reachable from a
consumer call, so for them `0x027f` is a real context and the gated half means
something on its own. **This row is only ever entered from inside the simulation
step**, so there is no context in which it is both exact and real: the half that
gates is not the half that runs. Its region tree keeps three `double`s live
across the whole decode where `phys_fn_001377` keeps one, which is why 662 rather
than 13.

`simulate_mismatches=662` is registered rather than dropped, so it fails if it
moves in either direction including toward zero. Phase 8 inherits a row whose
algorithm is proven and whose bit-exactness in its own machine model is not.

### Things the oracle does that a correct implementation would not

- **`s` and `t` are not computed symmetrically.** `s` at `0x00033fbc` multiplies
  the *wide* `e` still in `st(0)` after the `fst`; `t` at `0x00033fd5` reloads
  the narrowed copy. That is what the mutation above moved 75,122 words on.
- **The same quotient is evaluated at two precisions in one function.** Four
  leaves keep it in `st(0)` and use it wide for the result while storing the
  narrowed copy as the parameter (`0x0003425c`, `0x000343c2`, `0x00034575`, and
  `0x00034043` for `t`); two others store it and read the narrowed value back
  (`0x0003441a`, `0x00034507`).
- **`e` is computed three times.** Once at `0x00033f92` for the region tree, and
  twice more inside the near-parallel tree, which skips the first: once *wide*
  at `0x0003442d` and once narrowed through the same slot at `0x00034517`. So
  which precision `e` carries depends on which leaf you are in.
- **One comparison uses a `double` zero.** `fcomp qword ptr [0x001076e8]` at
  `0x00034120` compares `t` against an 8-byte zero where every other comparison
  in the function uses the 4-byte one. Same value, different operand size.
- **The result is `fabs`ed on the way out** (`0x000345a6`) rather than being
  known non-negative, so a rounding-negative squared distance comes back
  positive and a negative NaN comes back positive too.
- **Both output pointers are optional** and no caller uses that.
- **A NaN determinant takes the near-parallel branch**, so two segments with a
  non-finite coordinate are treated as parallel rather than rejected.

### What the discharge did not settle

`capsule/capsule` itself — `phys_fn_001775` at `0x0003d9d0`, 2,463 bytes — is
still open. Everything below was read out of the byte range for it and is
recorded so it is not found twice.

**The normal question is answered, from the instruction stream, and the answer
is the bad one.** `phys_fn_001010`'s 321 bytes write `hit+0x00` (the shape, from
`+0x9c`), `hit+0x04`/`+0x08`/`+0x0c` (`worldImpact`), `hit+0x1c` (`faceID`),
`hit+0x20` (`distance`), `hit+0x24`/`+0x28` (`u`/`v`) and `hit+0x2c` (`flags`,
the literal `0x13`). It writes **nothing** at `hit+0x10`..`+0x18`. And
`phys_fn_001775`'s swept path passes `&hit` at `esp+0xbc` (`0x0003dbc3`), then
hands the emitter `esp+0xc0` as the point (`0x0003dc1e`) and **`esp+0xcc` as the
normal** (`0x0003dc10`) — `hit+0x04` and `hit+0x10`. Nothing in the frame writes
`esp+0xcc` before the call. So the emitter is handed **three words of
uninitialised stack** as the contact normal, on every swept capsule/capsule
contact. Both capsules are capsules, so slot 5 is always `phys_fn_001010` and
there is no configuration in which that normal is written.

That is the second confirmed case of the oracle reading uninitialised stack,
after `NxSeparatingAxis` with `fullTest=false`, and it is worse than the first
because it reaches a *contact* rather than a return value. A harness that zeroes
the hit cannot tell it from a computed normal, which is why it was resolved by
reading rather than by matching a digest. Whoever builds the block will need to
make the read deterministic on both sides — a uniform fill of the stack below
the harness frame is the only lever available, and its limitation has to be
stated: a uniform pattern makes the read reproducible without proving the two
frames put the hit at the same offset.

**The rest of the decode**, so it is not repeated:

- **The swept selector is not the swept predicate.** The branch is on the OR of
  the two flag bytes (`or ecx,esi; test cl,1` at `0x0003da3a`), but which capsule
  becomes the ray is `and esi,1` on **shape1's** flag alone at `0x0003dae9`:
  shape1 swept means shape1 is the ray and shape0 the target, and otherwise
  shape0 is the ray. With both swept, shape1 wins. `edx = 1 - esi` picks the
  receiver at `0x0003dbde`. So all four flag combinations are distinct inputs.
- The ray is the selected capsule's own segment; its direction is normalised
  unless the length is exactly zero, in which case the raw difference is passed
  and the indirect call is made anyway.
- The emitter's arguments are `arg0 = rayShape->[0x9c]`, `arg1 =
  receiver->[0x9c]`, separation `push 0`, and both feature ids `0xffff`.
- **The non-swept path is two algorithms in one.** After
  `NxSegmentSegmentSquareDistance(&seg0, &seg1, &s, &t)` at `0x0003dc68` it
  tests `squared < (r0 + r1)²` **strictly** — `fcompp` + `test ah,5` + `jp`, so
  touching is not overlapping and a NaN is a miss — and then branches on
  `dir0·dir1 > 0.9998f` (the constant at `0x00107bf4`, `test ah,0x41` + `jne` at
  `0x0003de31`). Above the threshold it runs a **2×2 endpoint loop**: for each
  capsule and each of the other's two endpoints, project onto the axis, keep it
  if the parameter is inside `[-tol, len + tol]` with `tol = len * 0.001f` (the
  constant at `0x00107690`), and emit when `|v| - (r0 + r1) < 0` strictly with
  `point = pt[1] - radius * normalize(v)`. Up to four contacts. Below the
  threshold — **and also when the loop emitted nothing**, `je 0x0003e184` at
  `0x0003e162` — it emits one contact at the closest points, `point = q0 - r0 *
  normalize(q0 - q1)` and `separation = sqrt(squared) - (r0 + r1)`.
- Two zero-length guards behave differently: a zero-length `v` in the loop
  **skips the contact** (`0x0003e03d`), and a zero-length `q0 - q1` on the
  single-contact path **returns without emitting** (`0x0003e289`).
- `phys_fn_000001` at `0x00001000` and `phys_fn_000002` at `0x00001030` are the
  MSVC vector constructor and destructor iterators, called on `NxVec3 pts[2]` at
  `esp+0xbc` and `NxSegment seg[2]` at `esp+0x50` with a constructor that is
  `mov eax,ecx; ret` and a destructor that is `ret`. Neither is observable in a
  contact stream. The single-contact success path and the early miss return
  **without** running the destructor iterator at all.
- Three-way return from `NxRayCapsuleIntersect`: zero is a miss, one takes
  `s[0]`, anything else takes the smaller of `s[0]` and `s[1]` with the
  unordered case taking `s[1]`. Then `distance <= maxDist`, unordered included.

## `capsule/capsule`, transcribed and closed — two rows, 2,784 bytes

The last of the capsule three, and the only entry in this matrix whose two
halves are not both functions of their arguments.

| row | rva | size | what |
| --- | --- | ---: | --- |
| `phys_fn_001775` | `0x0003d9d0` | 2,463 | matrix A `[CAPSULE][CAPSULE]` |
| `phys_fn_001010` | `0x00022480` | 321 | vtable slot 5 of a CAPSULE shape |

```
collision name=shape_raycast_capsule index=- rva=0x00022480 owner=phys_fn_001010
    checks=5880000 oracle=28ac6dc0d51aa6bd candidate=dd477cdac9c1fa69 mismatches=242
collision coverage name=shape_raycast_capsule hits=32821 untouched_normal=32821
    aimed=22571 zero_axis=15030 default_mismatches=0 simulate_mismatches=242
collision name=contact_capsule_capsule index=21 rva=0x0003d9d0 owner=phys_fn_001775
    checks=1073192 oracle=d94c81f08538ddac candidate=0bfca39be83d9ebb mismatches=43
collision coverage name=contact_capsule_capsule emitted=20238 f00=25362 f01=25044
    f10=24804 f11=25386 swept_emitted=10174 seeded_normal=10008 parallel=37380
    zero_axis=42726 coincident=3830 beyond_end=10224 c1=18889 c2=1305 c3=20 c4=24
    default_mismatches=0 simulate_mismatches=43
```

### The swept half is not a function of its arguments

`phys_fn_001010` writes `hit+0x00`, `+0x04`/`+0x08`/`+0x0c`, `+0x1c`, `+0x20`,
`+0x24`/`+0x28` and `+0x2c` — the last a literal `0x13`, with no `|4` anywhere.
It never touches `hit+0x10`..`+0x18`. `phys_fn_001775` passes `&hit` at
`esp+0xbc` (`0x0003dbc3`), `hintFlags = 4` (`0x0003dbcf`), and then hands the
emitter `esp+0xc0` as the point (`0x0003dc1e`) and **`esp+0xcc` as the normal**
(`0x0003dc10`). Nothing in the swept path writes `esp+0xcc`. Both shapes are
capsules, so slot 5 is always `phys_fn_001010` and there is no configuration in
which that normal is written.

**Every swept capsule/capsule contact therefore carries three words of
uninitialised stack as its contact normal.** That is the second such case in
this programme after `NxSeparatingAxis` with `fullTest = false`, and it is the
worse of the two: that one reached a return value and this one reaches a contact
record that a solver will act on.

**Measured twice, from the oracle, not read off the disassembly.**

1. `untouched_normal=32821` equals `hits=32821` in the direct-drive block. The
   hit is `0xcd`-poisoned before every call and every hint-flag combination is
   driven; the oracle leaves all three words poison on every single hit.
2. `seeded_normal=10008` of `swept_emitted=10174` in the contact block. The
   harness seeds the stack the callee is about to use with one repeated dword
   and then asks whether that dword comes back out in the emitted stream — and
   the seed **alternates** between `0xcdcdcdcd` and `0xa5a5a5a5` per iteration,
   so a match cannot be a constant the kernel happens to write. The 166 that do
   not carry it are contacts where the emitter skipped the normal block because
   the cached normal already matched.

**How it is driven, and the limitation stated with it.** `nxSeedFrameBelow`
reserves 4 KB with `_alloca`, fills it with the pattern and returns, so the bytes
are still there when the callee's frame is laid over them; it runs before *each*
of the two calls, because the first call dirties the region for the second. One
repeated dword rather than a varied fill, deliberately: an unwritten slot then
reads the same value whatever offset each compiler chose for the hit. **That
makes the read reproducible; it does not prove the two frames put the hit at the
same address, and nothing available here could.** What the differential does
prove is that the reconstruction reads an unwritten slot rather than computing
something — a candidate that computed a normal would not track the seed, and the
mutation below shows exactly that.

### The swept selector is not the swept predicate

`or ecx,esi; test cl,1` at `0x0003da3a` branches on the **OR** of the two flag
bytes. But which capsule becomes the ray is `and esi,1` on **shape1's flag
alone** at `0x0003dae9`, and `edx = 1 - esi` at `0x0003dbde` picks the receiver.
So shape1 swept means shape1 is the ray; otherwise shape0 is; and with both
swept, shape1 wins. The predicate and the selector read different things, which
makes all four flag combinations distinct inputs and drives both receiver orders
without the generator choosing either.

`f00=25362 f01=25044 f10=24804 f11=25386` is the four combinations driven
independently, with the other 31 bits of each flag word randomised.

### The non-swept half is two algorithms with a fallthrough

`phys_fn_001690(&seg0, &seg1, &s, &t)` at `0x0003dc68`, then
`squared < (r0 + r1)²` **strictly** (`fcompp` + `test ah,5` + `jp`), so touching
is not overlapping and an unordered comparison emits nothing. Then
`dir0·dir1 > 0.9998f` — the constant at `0x00107bf4`, `test ah,0x41` + `jne` at
`0x0003de31`, strictly greater so an unordered dot takes the other path.

**Above the threshold**, a 2×2 endpoint clip: for each capsule and each of the
other's two endpoints, project onto the axis and keep it when the parameter is
inside `[-tol, len + tol]` with `tol = len * 0.001f` (`0x00107690`) — a tolerance
on the *parameter range*, not on the distance. Up to four contacts, each emitted
only when `|v| - (r0 + r1) < 0` strictly, with `point = pt[1] - radius * v̂` and
`radius` the **other** capsule's.

**And when the clip emits nothing it falls through** (`je 0x0003e184` at
`0x0003e162`) to the single-contact closest-point path, so one call can run both
algorithms. That is not a fallback a reimplementation would write: the entry has
already decided the axes are parallel and then silently changes its mind.

**Two zero-length guards that mean opposite things.** A zero-length normal inside
the clip **skips that contact** (`0x0003e03d`) and carries on; a zero-length
closest-point difference on the single-contact path **returns without emitting**
(`0x0003e289`).

**Two asymmetries in the axis lengths.** `len0` squares the wide x and z still in
registers and `len1` reads all three components back from their 32-bit slots; and
`len0` divides by its narrowed copy while `len1` divides by the wide one. The two
capsules are not treated the same way.

### Distinguishing inputs driven

| input | why | count |
| --- | --- | ---: |
| the two flags independently | the branch is an OR and the selector is shape1's bit alone | `f00`/`f01`/`f10`/`f11`, ~25,000 each |
| near-parallel and anti-parallel axes | the endpoint clip is above a 0.9998f dot product and two independent rotations never reach it | `parallel=37380` |
| matched half heights, axes co-located | three and four contacts need both capsules' endpoints inside the other's range, within a 0.1% parameter tolerance | `c3=20`, `c4=24` |
| the second capsule in the first's frame | past an endpoint, on the interior, and exactly on the axis | `beyond_end=10224`, `coincident=3830` |
| zero half height on either capsule, and a zeroed rotation | three different ways to a zero-length axis | `zero_axis=42726` |
| an alternating stack seed | the swept normal is uninitialised | `seeded_normal=10008` |

`c1=18889` and `c2=1305` are the single-contact path and the two-contact clip.
Every aim is gated on the first capsule's axis being finite.

`phys_fn_001010` is driven at its own address as well, because the entry cannot
reach two of its behaviours: the entry always passes a distance limit equal to
the segment length it just measured, so the limit gate is never anything but an
exact fit, and every hint-flag combination has to be driven to establish that
*none* of them writes a normal.

### Mutations, each with a measured delta

Built in `git archive` copies at `3a2cf83`; the real tree was asserted clean and
at `HEAD` before and after, and the un-mutated copy read `mismatches=0` under the
gating word on every block in the same session.

| row | mutation | words moved |
| --- | --- | ---: |
| `phys_fn_001775` | the ray selector reads the **OR** of the two flags instead of shape1's alone | **24,141** of 1,073,192, 11,959 under the gating word |
| `phys_fn_001010` | the row writes `hit.worldNormal` under `NX_RAYCAST_NORMAL`, the way the sphere's slot 5 does | **65,343** of 5,880,000 on its own block, **and 31,150** on `contact_capsule_capsule` |

Both exit 1. The second is the one that matters: it moves the *contact stream*,
which is the measurement that the uninitialised normal is load-bearing rather
than an oddity. A harness that zeroed the hit, or that did not seed the frame,
would have read zero there.

### The registrations bite, by the strong form

Two degradations, in copies, run through the gate's own assertion loop. **In both
cases the harness's own exit code stays 0.**

| degradation | effect | unmet |
| --- | --- | --- |
| the generator never sets `NX_SWEPT_SHAPE` on either capsule | `f01`/`f10`/`f11` → 0, `swept_emitted` 10,174 → **0**, `seeded_normal` 10,008 → **0**, digest `d94c81f0…` → `46099f8f…` | the two `contact_capsule_capsule` lines, 2 of 59 |
| the generator never aims the two axes parallel | `parallel` 37,380 → **0**, `c3` and `c4` → **0**, `c2` 1,305 → 514, digest → `11aa20bb…` | the two `contact_capsule_capsule` lines, 2 of 59 |

against `registered=59 unmet=0` for the control transcript in the same session.
The two mutations above are unmet on one and two lines respectively, and neither
touches the other twelve registered collision blocks.

### Where it narrows, and what it stores past its own frame

Both capsules' endpoints are built first (`0x0003d9e1`–`0x0003dadf`) with the
same column-1 axis, the same narrowed x half-axis and the same narrowed `-axisZ`
as the three capsule entries already closed; the second construction is
byte-for-byte the first's with `ebp` for `ebx`. On the swept path the ray
direction's x stays wide through the `fst` at `0x0003db07` and is only narrowed
into the ray, while y and z are narrowed twice — once as the raw difference and
again after the scale. The inverse length is itself narrowed
(`fstp dword` at `0x0003db77`) where `plane/capsule` keeps it wide.

**No stop.** Between them the two rows read the shape vtable, rotation column 1,
the translation, `+0x9c` and `+0xe0`..`+0xec`, and `hit` at the offsets the
pinned `NxUserRaycastReport.h` fixes. Every one is already established.

### Things the oracle does that a correct implementation would not

- **A swept contact's normal is uninitialised stack**, and its separation is a
  literal `push 0`, so a swept capsule pair reports a contact that is exactly
  touching with a normal that depends on what the caller did last.
- **The branch predicate and the ray selector read different flags.**
- **The parallel clip falls through to the closest-point path when it emits
  nothing**, so one call can run both algorithms and the "parallel" decision is
  not final.
- **The clip's tolerance is on the parameter range, not the distance**, so it
  scales with the capsule's length: a long capsule accepts endpoints further
  outside itself than a short one, in absolute terms.
- **The emitter always receives `(capsule0, capsule1)` in that order on the
  non-swept path**, whichever way round the clip loop is, while the swept path
  passes the *ray* capsule first — so the two halves disagree about pair order.
- **`len0` and `len1` are formed and used at different precisions**, and the
  first divides by its narrowed copy where the second divides by the wide one.
- **A zero-length axis is still raycast**, and slot 5 receives a zero direction.
- **`phys_fn_001010` has no facing test and no ahead-of-origin test** — only the
  distance limit, which admits equal and unordered — and it writes
  `hit.distance` and `hit.worldImpact` from a root it has already chosen before
  that limit is applied.
- **`hit.flags` is a literal `0x13` on the capsule's slot 5**, so a caller
  asking for `NX_RAYCAST_NORMAL` is told it did not get one, and
  `phys_fn_001775` reads the field anyway.

### Two limits carried, not fixed

Under `0x027f` both rows agree on every check. Under `0x0f7f`
`shape_raycast_capsule` differs on **242** and `contact_capsule_capsule` on
**43**, from the same cause as `phys_fn_001690`'s 662: MSVC spills a `double` to
an 8-byte slot and truncates a 64-bit significand to 53. Both rows are reached
only from inside the simulation step, so — as for `phys_fn_001690` — **they are
closed on algorithmic agreement under a control word they do not execute under,
with the real-context divergence pinned at 242 and 43.** Both counts are
registered so they fail if they move in either direction.

`shape_raycast_capsule`'s 242 are all inside `NxRayCapsuleIntersect`'s own
arithmetic, which is Task 2's row and already carries this escalation as the
fifth export the recovered matrix put inside the step.

## `plane/box`, transcribed and closed — one row, and the duplication it forces

`phys_fn_001883` at `0x00047f20`: 42 bytes plus 786 in two continuations the
census splits at the entry's internal alignment padding and annotates —
`0x00047f50` (9) and `0x00047f60` (777). It is the only entry in matrix A that
does **not** call `phys_fn_000873`, so closing it puts a second copy of the
stream logic in the tree.

```
collision name=contact_plane_box index=2 rva=0x00047f20 owner=phys_fn_001883
    checks=7450176 oracle=192a285f9e8892eb candidate=192a285f9e8892eb mismatches=0
collision coverage name=contact_plane_box emitted=66152 header_rewrite=30298
    zero_normal=3318 below_plane=13072 negated=50410 c1=2470 c2=4330 c4=10012
    c6=45492 c7=0 w11=2470 w15=4330 w31=45492
    default_mismatches=0 simulate_mismatches=0
```

**Bit-exact under both control words.** This row is not in the codegen-divergence
class the other four are: `mismatches=0` under `0x027f` and under `0x0f7f`. It
keeps no `double` live across a branch — the only floating point in it is one
dot product and, on the negated path, three `fchs` — so there is nothing for a
spill to truncate.

### How the two copies are kept in agreement

The question is real: after this row there is an emitter and an inlined
duplicate of it in one tree, and nothing in the compiler forces them to match.

**The answer is that there is nothing left to keep in agreement.** Everything the
two oracle copies do identically is written once — `nxHeaderMaterial`,
`nxAppendPairHeader`, `nxAppendNormalBlock` and `nxAppendContactRecord` in
`ContactGeneration.cpp` — and both rows call them. `NxEmitContact` was rewritten
to use them in the same change, and the check that this was a refactor and not a
rewrite is that **`contact_emit` and `contact_plane_sphere` kept their pinned
oracle digests and every counter**: `deaa1fc557071411` / `5d51a6ed6f586cda`,
`w4=5288 w7=12 w8=2768 w11=51420`, `mismatches=0`.

What `plane/box` duplicates is only the **predicates** — the decisions about
*when* a header and a normal block are written — and those are exactly where the
two oracle copies disagree. So a future edit to an append rule changes both by
construction, and an edit to a predicate changes only the row whose predicate it
is. The differential is the second half: `contact_emit` drives
`phys_fn_000873` and `contact_plane_box` drives the inlined copy, each against
its own counterpart in the pinned DLL.

### The four differences from the emitter, each with the address that fixes it

1. **No header predicate.** The emitter compares `shape1->[0x9c]` against
   `sink->[0x20]` and `shape0->[0x9c]` against `sink->[0x24]` and skips the
   header when both match (`0x0001d67d`, `0x0001d688`). This writes it whenever
   the call emits at all: `test eax,eax; jne` at `0x00047fbc` branches on *this
   call's own* contact count and on nothing the sink holds. Two plane/box calls
   against the same pair therefore write two headers and increment the pair
   counter twice, where the emitter would write one.
2. **No normal predicate.** The emitter compares the normal against the cache
   (`0x0001d78c`–`0x0001d7a3`). This writes the block unconditionally. Because
   the header it just wrote cleared the cache the two agree — except for a plane
   whose normal is bit-for-bit zero, which the emitter skips and this does not.
   That is the same `w7` state the emitter block measures, reached from the other
   side.
3. **The header's feature half is always zero.** `sink->[0x34]` is set to `0` at
   `0x00047fdf` rather than computed, so the fifth contact word is unreachable
   from this entry — and the test for it at `0x000481ed` is emitted and dead.
4. **At most six contacts, from a shape with eight corners.** `cmp eax,6; jae`
   at `0x00048218` returns as soon as the sixth is emitted. A box wholly below a
   plane loses two of its eight corners. **That is the "buffer limits" RED mode,
   which this file listed as absent** — it is not absent, it is inside a kernel
   rather than in a buffer, and `c6=45492 c7=0` is it being reached and never
   exceeded.

### The operation order

Three nested loops over the corner signs, `signZ` innermost, each stepping by 2
from `-1` and each passed to `phys_fn_000943` as a full `int` — `0x00047f50`
seeds the inner one, `0x00048229`, `0x00048239` and `0x0004824d` step the three.
The plane distance is `((c.y*n.y + c.z*n.z) + c.x*n.x) + d`, **y and z before
x**, with `fst` at `0x00047fa3` narrowing a copy while the *wide* value is what
the comparison sees. `test ah,0x41` + `jp`: greater and unordered both skip, so a
corner exactly on the plane is a contact and a NaN corner is not.

**The contact point is the box corner itself**, not a point on the plane — a
third convention in one matrix column, after plane/sphere putting it on the
sphere and plane/capsule on the plane. The separation is the plane distance,
which for a corner below the plane is what the shared mask makes
indistinguishable from its magnitude.

It also **stores a flag byte over the caller's third argument slot** at
`0x00047fcf` — the sink pointer — which is safe only because `edi` already holds
it.

### Distinguishing inputs driven

| input | why | count |
| --- | --- | ---: |
| the same pair driven twice into one sink without an identity change | the only way to reach the missing header predicate | `header_rewrite=30298` |
| a plane normal of exactly zero | the only case where the missing normal predicate shows | `zero_normal=3318` |
| the box placed wholly below the plane | eight corners qualify and the cap takes six | `below_plane=13072`, `c6=45492` |
| the sink oriented to the box | the negated path, three `fchs` on the plane's own normal | `negated=50410` |

`c1`, `c2`, `c4` and the width histogram `w11`/`w15`/`w31` are the partial cuts:
31 words is a header, a normal block and six records in one call, which no other
block in this target produces.

### Mutation and degradation

Built in `git archive` copies at `5b0affc`; the real tree asserted clean and at
`HEAD` before and after; the un-mutated copy read `mismatches=0` in the same
session.

| mutation | words moved |
| --- | ---: |
| the inlined copy is given the **emitter's header predicate** | **452,624** of 7,450,176, 226,312 under the gating word |

The target exits 1. That is the measurement that the missing predicate is a real
recovered difference rather than a transcription convenience — and it is the
mutation that would catch the two copies being "unified" by a future reader who
assumed they were the same.

Degrading the generator so it never repeats a pair takes `header_rewrite` from
30,298 to **0** and moves the digest `192a285f…` → `e4f46ae8…`, while the
harness's own exit code stays 0; the gate's assertion loop then reports
`registered=61 unmet=2` against `unmet=0` for the control.

## `sphere/sphere` and `sphere/box` — five rows closed, four stopped

Both entries are closed and both are **bit-exact under both control words**.
What they drag in behind them is not: of the nine new Phase 3 rows the two share,
**five are closed (1,630 bytes) and four are a stop (3,802 bytes)**, and the line
between them is one `if`.

| row | rva | size | state |
| --- | --- | ---: | --- |
| `phys_fn_001933` | `0x0004b860` | 341 | closed — matrix A `[SPHERE][SPHERE]` |
| `phys_fn_001919` | `0x0004a2d0` | 259 | closed — matrix A `[SPHERE][BOX]` |
| `phys_fn_001917` | `0x00049f00` | 969 | closed — the sphere/box contact geometry |
| `phys_fn_001281` | `0x000257a0` | 4 | closed — `Shape+0x04`, the owner accessor |
| `phys_fn_002266` | `0x00056650` | 57 | closed on its reachable arm — the continuous-CD guard |
| `phys_fn_002264` | `0x00055eb0` | 1943 | **STOP** — the continuous-collision sweep |
| `phys_fn_001653` | `0x00031db0` | 1618 | **STOP** — reached only from it |
| `phys_fn_000772` | `0x00018470` | 207 | **STOP** — reached only from it |
| `phys_fn_000774` | `0x00018540` | 34 | **STOP** — reached only from it |

```
collision name=contact_sphere_sphere index=7 rva=0x0004b860 owner=phys_fn_001933
    checks=2061456 oracle=40ec7aaacfefd69f candidate=40ec7aaacfefd69f mismatches=0
collision coverage name=contact_sphere_sphere emitted=49285 coincident=13352
    coincident_emitted=9126 repeated=14897 static0=4281 static1=4228 negated=24926
    w4=8529 w8=2356 w11=38400 default_mismatches=0 simulate_mismatches=0
collision name=contact_sphere_box index=8 rva=0x0004a2d0 owner=phys_fn_001919
    checks=2453136 oracle=966d357901d1dba9 candidate=966d357901d1dba9 mismatches=0
collision coverage name=contact_sphere_box emitted=65826 centre_inside=8638
    repeated=15287 static0=4173 static1=4171 negated=25490 w4=18442 w8=7236
    w11=40148 default_mismatches=0 simulate_mismatches=0
collision name=sphere_box_contact index=- rva=0x00049f00 owner=phys_fn_001917
    checks=3480000 oracle=97bce5b099aebebf candidate=97bce5b099aebebf mismatches=0
collision coverage name=sphere_box_contact true=82140 centre_inside=22596
    axis_x=6969 axis_y=7042 axis_z=7007 default_mismatches=0 simulate_mismatches=0
collision name=shape_owner index=- rva=0x000257a0 owner=phys_fn_001281 checks=8000
    oracle=87f31ac4e078fb65 candidate=87f31ac4e078fb65 mismatches=0
collision name=ccd_guard index=- rva=0x00056650 owner=phys_fn_002266 checks=16000
    oracle=62da6260b65713a5 candidate=62da6260b65713a5 mismatches=0
collision coverage name=ccd_guard continuous_cd=00000000 probes=8000
    returned_true=8000 sink_untouched=8000 index_probes=58 index_wrong=0
```

### The cost survey was right about the closure and wrong about what kind it is

The survey above costed `sphere/sphere` at 7 new rows and 4,204 bytes and
`sphere/box` at 8 and 5,091. Both numbers reproduce **exactly** — 341 + 4 + 57 +
1943 + 1618 + 34 + 207 and the same with 259 + 969 in place of 341. What it did
not say, because a byte count cannot, is that **3,802 of those bytes hang off one
predicate that is false in every configuration the shipped SDK produces**.

`phys_fn_002266` is the predicate. It is **not an error path** — the survey called
it "an error path that runs when a shape's `owner->[8]` is null" and that is wrong
in both halves. Its 57 bytes are:

```
mov  ecx, [0x10123c04]      the SDK singleton, which phys_fn_000429 never reads
push 0xb                    NX_CONTINUOUS_CD
call 0x0000dc00             PhysicsSDK::getParameter
fld  [0x101041f0]           0.0f
fucompp ; fnstsw ax ; test ah,0x44 ; jp
mov  al, 1 ; ret            equal -> true, and nothing is written
...                         otherwise, tail into phys_fn_002264
```

A null `owner->[8]` selects *which* shape is passed first and nothing else: both
sphere entries test shape0 first and shape1 only if shape0's is non-null, and
pass the shape whose `owner->[8]` is **not** null as the first argument
(`0x0004b874`/`0x0004b88f` and `0x0004a2e3`/`0x0004a2fe`). That reads as "the
moving body first, the static one second", which is what a continuous-collision
sweep wants.

### Why `phys_fn_002264` is a stop, with the address for each reason

Three things it reads that nothing in this program establishes. Any one of them
would be enough.

1. **A second pose at `Shape+0x3c`..`+0x68`.** `0x00055ebe` onward copies
   `esi+0x3c`..`esi+0x68` and `edi+0x3c`..`edi+0x68` into its frame beside
   `+0x0c`..`+0x38`, which is the pose the borrowed-layout section does
   establish. Twenty-four more dwords, in the same shape as an `NxMat34`, that
   no Phase 3 or Phase 5 evidence names.
2. **The sink past `+0x44`.** It reads `[ebp+0xe9]` at `0x00056196`, compares it
   against `0xff` and against `1`, and writes `[ebp+0xdc]`, `[ebp+0xe0]`,
   `[ebp+0xe4]` and `[ebp+0xe9]` at `0x000561c0`..`0x000561d2`. `ebp` is the
   sink. The borrowed layout stops at `0x44` and explains it as a `0x10` prefix
   plus a `0x34` sub-object; it says nothing about `0xdc`.
3. **Vtable slot 7.** `call dword ptr [eax+0x1c]` at `0x000562c3` with
   `eax = [edi]`, the shape's own vtable, and two more at `0x00056485` (slot 5)
   and `0x000564b9` (slot 7). The slot-5 table above was recovered per receiver
   type from each constructor's vtable store; **slot 7 has never been resolved
   for any shape type**, and resolving it is the same per-type job.

Stopping also removes what that subtree drags in with it: `phys_fn_001653`
(1,618, and a Phase 4 row at `0x000e4200` under it), `phys_fn_000774` and
`phys_fn_000772` (241 between them), five Phase 2 rows, and **`phys_fn_000754`,
a 1,027-byte Phase 7 row** reached through `phys_fn_000022`. Nothing outside the
sweep reaches any of them.

### The reachability claim is measured, not assumed

`NX_CONTINUOUS_CD` is index 11 and its shipped default is `0.0f`:
`phys_fn_000472` zeroes `ebp` at `0x0000e1bb` and writes it into the defaults
table at `0x0000e56e`, and `rep movsd` at `0x0000e72c` copies that table over the
live one. So the guard closes and the sweep is unreachable.

That is exactly the kind of claim that goes stale silently, so the harness
measures it three ways against the oracle rather than reading it off the
disassembly:

- it reads parameter 11 back out of the oracle's own live array through the
  **oracle's own** `phys_fn_000429` and prints the word — `continuous_cd=00000000`
  is registered in the gate, so the claim fails if it stops holding;
- it drives `phys_fn_002266` at its own address over a `0xcd`-poisoned 0x300-byte
  sink and checks the answer *and* that every byte is untouched —
  `returned_true=8000 sink_untouched=8000`, over a range that reaches well past
  the `+0xe9` the sweep would write;
- it sets each of the **other 58 parameters** to 1.0f in turn and re-asks, which
  measures that the guard reads index 11 instead of restating the `push 0xb`.
  `index_probes=58 index_wrong=0`.

**What is not measured is the other arm being entered.** Driving it means running
`phys_fn_002264` against a vtable slot nothing has resolved, and there is a
tempting near-miss: `cmp byte ptr [ebp+0xe9],1; jne 0x0005630a` at `0x000561d9`
skips the block containing the first indirect call, so a poisoned sink byte looks
like it would make the call safe. It would not make it *bounded* — two more
indirect calls follow at `0x00056485` and `0x000564b9` — and a harness that
installed stub slots to get past them would be inventing the thing the
borrowed-layout rule exists to stop. So the arm stays open and is stated as open.

### `sphere/sphere`, the operation order

`esi` is shape0, `edi` shape1, `ebx` the sink.

1. Both `owner->[8]` tests and the guard call, above.
2. `delta = centre1 - centre0`, each component narrowed into a slot — except z,
   which is stored with `fst` at `0x0004b8b8` and squared as **wide times
   narrow**. The radius sum at `0x0004b8e4` is the same shape. Neither is a
   nicety: `a - b` of two floats is exact in an 80-bit register only while their
   exponents are close, so a pair whose centres or radii differ by many orders of
   magnitude is where the two spellings part.
3. `distanceSquared = ((dz*dz + dy*dy) + dx*dx)`, **z then y then x**, narrowed
   into the caller's second argument slot at `0x0004b8d4`.
4. `radiusSum²> distanceSquared`, strictly and ordered (`test ah,0x41; jne` at
   `0x0004b8f2`): touching produces nothing and any NaN produces nothing. That
   agrees with `phys_fn_001931`, the overlap test for the same pair.
5. `distanceSquared > 1e-5f` (`0x00107a08`), `jnp` leaving on less and on equal.
   **This is the coincident-centre guard** and it is the only thing standing
   between the divide below and a zero divisor.
6. `fsqrt`, then `1.0f / distance` once, then the three components of the normal
   each narrowed after the multiply.
7. The point is `centre0 + radius0 * normal` — **on sphere0's surface**, a fourth
   convention in this matrix after the sphere, the plane and the box corner. Its
   x keeps the wide product while y and z are read back from slots
   (`0x0004b981` against `0x0004b98a` and `0x0004b995`).
8. The separation is the **wide** root minus the **narrowed** radius sum.

### `sphere/box`, the operation order

`phys_fn_001919` flattens both shapes into the same two stack structures
`phys_fn_001915` builds for the overlap test — which is what fixes their field
order — and calls `phys_fn_001917` with three out-pointers, the last of which is
**the caller's own second argument slot** (`lea eax,[esp+0x7c]` at `0x0004a36f`).

`phys_fn_001917` is `phys_fn_001913` with the answer kept, plus a second
algorithm:

- **Outside** (`0x0004a02b`). The clamped box-space point goes back out through
  the rotation's *rows* where the way in used its *transpose*; the vector from it
  to the sphere centre is the normal and its length the separation. The third
  world component is left in a register while the first two go through slots, so
  the normal's z is formed against a wider number than the point's z was
  (`fst` at `0x0004a08b`, `fsub st(3)` at `0x0004a0a2`). The box centre is added
  back x, y, z with the operands the other way round **on y alone**
  (`0x0004a10a`..`0x0004a11f`).
- **Inside** (`0x0004a131`). There is no closest point, so the contact comes from
  the shallowest face: `extent - |local|` per axis, smallest wins, **ties go to
  z**, and the normal is that axis signed by which side the centre is on. The
  point is then the **sphere centre copied dword for dword**
  (`0x0004a253`..`0x0004a266`) and the separation is `-(depth + radius)`.

The entry then hands the emitter the **sphere as `object1` and the box as
`object0`** (`0x0004a3c7` and `0x0004a3c6`) — shape0 in the slot plane/sphere
gives shape1. Which side lands in which slot decides which owner the header's
material is taken from first and which collision object the ordering rule
compares, so it is a per-entry fact rather than a convention. `capsule/capsule`
is the other entry that does it this way round.

### Distinguishing inputs driven

| input | why | count |
| --- | --- | ---: |
| two centres within 1e-2 of each other, squared to straddle 1e-5f | the only way to the epsilon test, and it has to be crossed in **both** directions or the count is just "entered" | `coincident=13352 coincident_emitted=9126` |
| the same pair driven twice into one sink with no identity change | the ordering rule's third state, where both the header and the normal block are skipped | `repeated=14897`/`15287`, `w4=8529`/`18442` |
| a null `owner->[8]` on **shape0**, alternating with shape1 | the entry tests shape0 first, so the first of its two branches needs the side the harness had never driven null | `static0=4281`/`4173`, `static1=4228`/`4171` |
| the sphere centre placed inside the rotated box **in box space** | the inside algorithm. Placing it near the centre in world coordinates is not the same thing for a rotated box | `centre_inside=22596`/`8638`, `axis_x=6969 axis_y=7042 axis_z=7007` |
| the sink oriented to the second shape | the emitter's negated path, with a normal these entries computed rather than borrowed | `negated=24926`/`25490` |

`nxStageWorld` gained a `nullHolder0` for the third of those. Both null at once
is deliberately not offered: the emitter's material fallback would then
dereference a null holder, and so would the oracle's.

### Mutations, each with a measured delta and a positive control

Built in `git archive` copies at `e389252`; the real tree asserted clean and at
`HEAD` before and after every run; the un-mutated copy built and run in the same
session and read `mismatches=0` on every block.

| row | mutation | words moved |
| --- | --- | ---: |
| `phys_fn_001933` | the 1e-5f coincidence test deleted | **14,814** of 2,061,456 (7,407 per control word) |
| `phys_fn_001919` | the two emitter object slots swapped | **248,352** of 2,453,136 (124,176 per word) |
| `phys_fn_001917` | the inside path picks the **deepest** face | **126,980** of 3,480,000, and 57,508 through the entry |
| `phys_fn_001917` | the inside path's normal through the transpose | **48,894**, and 22,458 through the entry |
| `phys_fn_001917` | the normal's z from the **narrowed** world z | **38,844** (15,051 + 23,793), and 25,895 through the entry |
| `phys_fn_001281` | the accessor reads a different field | **8,000** of 8,000 |
| `phys_fn_002266` | the guard's equality inverted | **8,000** of 16,000 |

The target exits 1 on every one of them.

**One mutation moved nothing, recorded rather than replaced quietly.** Squaring
`phys_fn_001933`'s radius sum from the narrowed value twice instead of the wide
value times the narrowed one moves **zero** words. The sum of two `NxReal`
radii is exact in an 80-bit register for every pair this generator produces, so
the two spellings are the same number; it would take radii many orders of
magnitude apart to separate them and the geometry-aimed half of the generator
does not build those. It is transcribed as the binary has it. This is the same
situation as the three zero-movers recorded for the overlap rows.

### Neither entry is in the codegen-divergence class

`default_mismatches=0 simulate_mismatches=0` on all three new contact blocks, over
2,061,456 + 2,453,136 + 3,480,000 checks. Both entries execute **only** under
`0x0f7f`, since nothing outside the simulation step reaches matrix A, and both
agree there bit for bit — so this is the strong reading, not the `plane/box`
reading where agreement under an unexecuted word had to be qualified.

The `plane/box` diagnosis holds and predicted it: **neither keeps a `double` live
across a branch**. Everything that survives a comparison in `phys_fn_001933` went
through a 32-bit slot on the way. `phys_fn_001917` has one apparent exception —
the clamped z lives in `st(0)` from `0x00049f88` to `0x0004a02b`, across three
branches — and it is not one, because every value that variable can hold is
exactly a float either way, so a spill has nothing to truncate. That is now two
independent confirmations of the same design rule, and it is worth using as a
*prediction* rather than a post-hoc explanation: read the row for a `double`
crossing a branch before transcribing it, and expect the divergence if there is
one.

### The registrations bite, by the strong form

Five degradations, each built the same way, each leaving the harness's own exit
code at **0** — so only the pinned registration catches them. The control run
reports `unmet=0` against 71 registered lines.

| degradation | what moves | unmet |
| --- | --- | ---: |
| `contact_sphere_sphere` never repeats a pair | `repeated` 14,897 → 0, `w4` 8,529 → 0, digest `40ec7aaa…` → `ad3f1508…` | 2 |
| `contact_sphere_sphere` never aims a coincident pair | `coincident` 13,352 → 0, `coincident_emitted` 9,126 → 0, digest → `f617bf43…` | 2 |
| `contact_sphere_box` never repeats a pair | `repeated` 15,287 → 0, `w4` 18,442 → 0, digest `966d3579…` → `27bc8627…` | 2 |
| `sphere_box_contact` never places a centre inside | `centre_inside` 22,596 → 0, all three `axis_*` → 0, `true` 82,140 → 58,780, digest → `31dc41ac…` | 2 |
| `ccd_guard` stops sweeping the other 58 parameters | `index_probes` 58 → 0, digest **unchanged** | 1 |

The last one is the interesting shape: the sweep is deliberately outside the
digest, because it perturbs the oracle's own `.data` and folding it in would make
the pinned digest depend on a write this harness performs. Its coverage line is
the only thing that can fail, and it does.

### One defect found by reading the row back, after the digests were zero

The inside path takes `|local|` per axis. Spelled `x < 0 ? -x : x` that leaves
`-0.0` negative, where the oracle's `fabs` at `0x0004a137` clears the sign bit.
The two differ when an extent is **also** `-0.0`, which the raw-bit half of the
generator can produce: `-0.0 - -0.0` is `+0.0` and `-0.0 - 0.0` is `-0.0`, and
the sign of that zero reaches the stream through the separation. **No draw in
3,480,000 hit it.** Every digest and every counter is unchanged by the fix, which
is what says it was a correctness change on an input nothing reached rather than
a rewrite -- and it is a reminder that a zero mismatch count is a statement about
the inputs that were driven and nothing else.

### Things the oracle does that a correct implementation would not

All reproduced.

- **The contact point is a fourth thing again.** Plane/sphere puts it on the
  sphere, plane/capsule on the plane, plane/box on the box corner, sphere/sphere
  on sphere0's surface — and sphere/box puts it on the box *or*, when the centre
  is inside, **at the sphere's own centre**. Five entries, five conventions.
- **A NaN is a miss in one entry of this pair and a hit in the other.**
  `phys_fn_001933` leaves on unordered at `0x0004b8f2` and produces nothing;
  `phys_fn_001917` continues on unordered at `0x0004a0d7` and emits. That is
  `phys_fn_001913`'s disagreement inherited into contact generation.
- **Three stores past the callee's own frame.** `phys_fn_001933` writes the
  squared distance and the radius sum over the caller's second and first argument
  slots (`0x0004b8d4`, `0x0004b8e4`); `phys_fn_001917` writes `-extents.z` and
  then the sign into the caller's first argument slot (`0x00049ff3`,
  `0x0004a17d`); and `phys_fn_001919` hands `phys_fn_001917` a pointer to **its
  own second argument slot** as the separation out-parameter and reads the
  separation back out of it at `0x0004a39a`. All are dead by then and all three
  are stores into a frame the callee does not own.
- **`point->y` is stored before `point->x`** in the outside path (`0x0004a080`
  against `0x0004a089`) and `normal->y` before `normal->x` in the inside path
  (`0x0004a2ac` against `0x0004a2b5`), each as a raw dword move while the other
  two go through the x87. Nothing observes the order, and it is transcribed
  because a reader who "tidied" it would be changing the row.
- **The inside path's tie-break favours z.** Both comparisons are strict, so a
  cube with the centre exactly at its middle contacts through its z face, and a
  NaN depth is never the smallest.
- **The guard reads a singleton it never uses.** `phys_fn_002266` loads
  `[0x10123c04]` into `ecx` for a call that indexes a file-scope array and
  ignores `this`, so the whole row works with no SDK constructed. The
  reconstruction guards the null rather than reproducing the dereference, and
  says so: that changes no state the oracle can be in, because the only thing
  that fills the array is the constructor and it writes the same zero.

## `box/box` decoded, ahead of transcribing it — and the cost is 8,239 bytes, not 12,102

The last matrix A entry that can be closed, read but not transcribed, so the next
dispatch starts from a decode rather than from the disassembly. That is the same
hand-over `plane/capsule` got, and for the same reason: the entry is cheap and
the rows under it are not.

### The corrected cost, for the third time on this table

The survey costed `box/box` at 10 new Phase 3 rows and 12,102 bytes. Both numbers
are still right and both are still the wrong thing to plan from, because **six of
those ten rows are no longer new**:

| row | size | state |
| --- | ---: | --- |
| `phys_fn_001749` | 775 | **new** — the entry, matrix A `[BOX][BOX]` |
| `phys_fn_001748` | 240 | **new** — a transpose-and-copy shim |
| `phys_fn_001745` | 4271 | **new** — the separating-axis search |
| `phys_fn_001741` | 2953 | **new** — the clipping/manifold leaf |
| `phys_fn_001281` | 4 | closed, by the sphere entries |
| `phys_fn_002266` | 57 | closed, by the sphere entries |
| `phys_fn_002264` | 1943 | **STOP**, already specified |
| `phys_fn_001653` | 1618 | **STOP** |
| `phys_fn_000772` | 207 | **STOP** |
| `phys_fn_000774` | 34 | **STOP** |

`box/box` reaches the continuous-CD guard exactly as the two sphere entries do —
`0x0003ade3`, `0x0003ae04`, `0x0003ae13` — so it inherits both the two closed rows
and the same 3,802-byte stop. **The new work is 4 rows and 8,239 bytes**, of which
7,224 are the two large geometry rows.

That is the third time this table has been read as a plan and the third time the
number needed a qualifier. The general form is now clear enough to state: **a
closure figure is a set union, and a work estimate is that set minus what is
already closed and minus what is behind a stop.** Those are different numbers and
only the second one is a plan.

### The entry is the `plane/box` pattern, and most of it is already written

`phys_fn_001749` is the second entry that **inlines the emitter** rather than
calling `phys_fn_000873`, and it inlines the same three levels the shared helpers
in `ContactGeneration.cpp` already implement. Every one of the four differences
recorded for `plane/box` is present here at its own address:

| difference | `plane/box` | `box/box` |
| --- | --- | --- |
| no header predicate — written whenever the call emits | `0x00047fbc` | `0x0003ae8f`/`0x0003ae98`, written unconditionally |
| no normal predicate — the block always follows | `0x00047fdf` | cleared `0x0003af45`, written `0x0003af81` |
| the header's feature half is always zero | `0x00047fdf` | `0x0003ae82`, and the fifth-word test at `0x0003b08e` is dead the same way |
| a contact cap inside the kernel | `cmp eax,6` at `0x00048218` | **none** — see below |

The two `nxReserve` predicates are identical too: `cmp edx,[esi]; jne` before every
single-word append and `add eax,3; cmp eax,ecx; jbe` before each three-word burst,
at eight sites — `0x0003aeb6`, `0x0003aee0`, `0x0003af1b`, `0x0003af9a`,
`0x0003afd1`, `0x0003b031`, `0x0003b072`, `0x0003b09f`. The material lookup is the
same three loads in the same order at `0x0003aef5`..`0x0003af07`, and the material
reaches the header word through the same `shl ebx,0x18` at `0x0003af28`.

**So the entry's stream half needs no new code.** What is genuinely new in its 775
bytes is four things:

1. **The orientation swap is in the entry, not in the emitter.** `0x0003ae67`
   compares its **first** argument's `owner->[8]` against `sink->[8]`, swaps the
   two shapes into `ebx`/`ebp` on inequality and records a flag byte, and that
   flag negates all three normal components with `fchs` at
   `0x0003af50`..`0x0003af6e`. The emitter does the same thing at `0x0001d632`;
   this is a third copy of it.
2. **It is the only entry in matrix A that emits a manifold.** `phys_fn_001748`
   returns a contact **count** in `eax`, and the loop at
   `0x0003b010`..`0x0003b0c6` walks it: points from a 12-byte-strided array at
   `esp+0x78` (`add ecx,0xc` at `0x0003b0bd`) and separations from a parallel
   array at `esp+0x38`. Every other entry emits one contact, or — for
   `plane/box` — up to six from a fixed corner loop.
3. **No cap.** There is no `cmp eax,6`; the loop runs to whatever count comes
   back. The "buffer limits" RED mode that `plane/box` supplied does not
   generalise, and this row is where the stream growth path could actually be
   reached.
4. **The separation is negated *and* masked.** `fld [esp+ebx*4+0x38]; fchs` at
   `0x0003b013`, then `and ebp,0x7fffffff` at `0x0003b064`. The mask makes the
   negation invisible for a negative separation, which is why the emitter's mask
   read as a negation once already — here both are present in one row.

### A new borrowed offset, and it is established rather than assumed

`lea ebx,[edi+0xe8]` at `0x0003ae1b` passes **`sink+0xe8`** as `phys_fn_001748`'s
last argument, and the entry writes a zero byte there itself on the
no-contact path (`mov byte ptr [ebx], al` at `0x0003ae5d`, with `al` zero from the
test above it). So it is a byte-wide out-parameter. That is one byte away from the
`sink+0xe9` that `phys_fn_002264` reads and writes, which says the sink really does
carry a region around `+0xdc`..`+0xe9` that the borrowed layout stops short of.

**Whether this is a stop is not yet decided and must not be guessed.** The entry
only *writes* it; if `phys_fn_001748` or `phys_fn_001745` reads it back without
writing it first, the row is reading state nothing establishes and the same rule
applies as for `phys_fn_002264`. Reading those two for that is the first thing the
next dispatch should do, before any transcription.

### Two rows are entered through registers, which no C++ declaration expresses

`phys_fn_001748` sets `ebx` immediately before `call 0x00039c10` (`lea ebx,[esp+0x48]`
at `0x0003adbf`) and `phys_fn_001745` uses it without saving it. `phys_fn_002266`
does the same to `phys_fn_002264` with `esi` and `edi`. That is a compiler-chosen
convention for an internal function, and it matters twice: a harness that drives
either row **at its own address** needs a thunk that loads those registers — the
`nxCallSegmentDistance` pattern already in `PhysicsCollisionTests.cpp` is the
model — while the reconstruction's own copy can take ordinary parameters, because
the differential calls the candidate by name and only the oracle side needs the
ABI.

`phys_fn_001748` itself is a shim and nothing more: it copies two 12-dword
structures into its frame with the 3x3 **transposed** (`[eax+0]`, `[eax+0xc]`,
`[eax+0x18]` become three consecutive slots at `0x0003ace8`..`0x0003ad33`) and the
last three dwords straight, then calls `phys_fn_001745`. 240 bytes, no arithmetic.

### The divergence-class forecast, measured — and the instrument that makes it

The `double`-liveness signal says to read a row for a value that must survive a
branch *before* transcribing it. That can be measured rather than eyeballed: walk
the listing and count the x87 stack depth at each conditional branch. Against
rows whose answer is already known:

| row | branches by x87 depth | measured result |
| --- | --- | --- |
| `phys_fn_001883` plane/box | all 18 at depth 0 | bit-exact |
| `phys_fn_001261` plane raycast | all 5 at depth 0 | bit-exact |
| `phys_fn_001933` sphere/sphere | all 4 at depth 0 | bit-exact |
| `phys_fn_001919` sphere/box entry | all 3 at depth 0 | bit-exact |
| `phys_fn_001377` sphere raycast | 1 at depth 1 | **13** words |
| `phys_fn_001010` capsule raycast | 1 at depth 1, 1 at depth 2 | **242** words |
| `phys_fn_001775` capsule/capsule | 3 at depth 1, 2 at depth 2, 1 at depth 3 | **43** words |
| `phys_fn_001690` segment/segment | 11 at depth 1, 3 at depth 2, 1 at depth 3 | **662** words |
| `phys_fn_001917` sphere/box geometry | 4 at depth 1, 1 at 2, 2 at 4, 1 at **12** | **bit-exact** |

**Depth 0 everywhere is sufficient for bit-exact and that is the useful half.**
Depth above zero is not sufficient for divergence, and `phys_fn_001917` is the
counterexample that says so — it carries a value across twelve-deep branches and
agrees bit for bit, because the value it carries is the clamped z, whose every
possible setting is exactly a `float`. A spill has nothing to truncate.

So the rule has two steps, and the second one is the one that does the work:
**count the depth, then read what is being carried.** A copy of a float, a
negated float, an extent, a constant — harmless. A computed intermediate — a dot
product, a quotient, a running distance — is what a spill truncates.

Applied to the two new rows, at the sites the count names:

- **`phys_fn_001745` — 11 branches at depth 1, and the carried value is a
  constant.** At `0x0003a303` it is `fld [0x10106858]`, a 32-bit memory constant,
  still in `st(0)` across the branches at `0x0003a321` and `0x0003a335`. That is
  the `phys_fn_001917` kind. **Expect bit-exact**, subject to the other ten sites
  reading the same way.
- **`phys_fn_001741` — 10 branches at depth 1, and the carried value is
  computed.** At `0x00039450`..`0x00039456` it forms a quotient with
  `fld`/`fsub`/`fdivp` — an edge-clipping parameter — holds it in `st(0)` across
  the branches at `0x00039475` and `0x0003948a`, and consumes it at `0x0003947b`
  with `fmulp`. That is the `phys_fn_001690` kind. **Expect the codegen-divergence
  class**, pin the count, and record that the row executes only under `0x0f7f`.

That is a forecast with an address behind it rather than an expectation, and it
tells the next dispatch which of the two rows to budget a pinned divergence for.
The two remaining unknowns are named above: what `phys_fn_001748` and
`phys_fn_001745` do with `sink+0xe8`, and whether the other ten depth-1 sites in
`phys_fn_001745` carry constants like the one that was read.

**Not transcribed here, deliberately.** 7,224 bytes across two rows, 2,105
instructions of which 1,292 are floating point, is more than one careful dispatch
after two entries have already been closed — and the worst possible boundary to
stop at is halfway through a 4,271-byte function.

## `box/box`: both unknowns settled, the cost corrected again, and the leaf closed

The last dispatch handed over a decode and two named unknowns and said neither
must be guessed. Both are now answered from the instruction stream, one of them
in the opposite direction from the rule that had been written down for it, and
the row count and the byte count were both wrong again.

### `sink+0xe8` is NOT a stop: it is initialised to `0xff` one row up

`phys_fn_001745` **does** read the byte before it writes it — `mov eax,[esp+0xc4]`
at `0x0003a00f` loads the pointer and `mov al, byte ptr [eax]` at `0x0003a016`
reads it, and the only write in that function is at `0x0003a458`, 1,090 bytes
later. `phys_fn_001748` never touches it at all; it forwards the pointer as its
callee's fifth stack argument and does nothing else with it. So the
read-before-write the last dispatch feared is real.

**The rule it was to trigger does not apply, because something does establish the
value.** A byte-wide scan of every `[reg+0xe8]` and `[reg+0xe9]` access in the
image — twelve sites — finds the writer:

```
0x0001efa0  push esi                     phys_fn_000893, a Phase 7 constructor
0x0001efa3  lea  ecx,[esi+0x10]
0x0001efa6  call 0x5b680                 the sink's constructor, whose `this`
                                         is sink+0x10 -- so esi IS the sink
...
0x0001efc6  mov  cl, 0xff
0x0001efc8  mov  byte ptr [esi+0xe8], cl
0x0001efce  mov  byte ptr [esi+0xe9], cl
```

`0x0005b680` calls `0x0005b620` on its own `this` at `0x0005b68d`, and
`0x0005b620` was *measured* in the borrowed-layout section to operate on
`&sink + 0x10`. So `0x0005b680`'s `this` is `sink+0x10`, `0x0001efa3` passes
`esi+0x10`, and `esi` is therefore the sink base. `+0xe8` and `+0xe9` are both
written `0xff` there, and `phys_fn_002264` corroborates it independently: it
reads `[ebp+0xe9]` at `0x00056196` and compares it against `0xff`.

**And `0xff` is exactly the sentinel the reader treats as "nothing cached".**
`0x0003a018` sends `0` to the unbiased search and `0x0003a01e` sends `0xff` there
too; anything else in `1..0x10` goes to `0x0003a2e8`.

What the byte is, with the address for each half:

| what | where |
| --- | --- |
| written `0xff` by the sink's own constructor | `0x0001efc8` |
| written `code + 1` — the index of the axis the search settled on, 1..6 | `0x0003a456`, `0x0003a458` |
| written `0` by the entry when the call produced no contact | `0x0003ae5d` |
| read; `0` and `0xff` both mean "no cache" | `0x0003a016`..`0x0003a01e` |
| the cached axis's overlap multiplied by **0.999f** (`0x10107b30`) so the same axis wins again | `0x0003a2f3`..`0x0003a301` |

It is a **warm start**, and the per-step reset does not clear it: `0x0005b620`
covers `+0x10`..`+0x43` and stops. So the byte persists across simulation steps
*and across shape pairs* — one cached axis index for the whole sink rather than
one per pair. That belongs in the "things the oracle does" list below rather than
in a stop.

The byte is therefore an ordinary input at an established offset: a differential
sets it on both sides and every one of its branches is drivable. **`box/box` is
not a stop.**

### The other ten depth-1 sites in `phys_fn_001745` all carry constants

Read at every site rather than at one. The eleven branches above x87 depth 0 are
`0x0003a321`, `0x0003a335`, `0x0003a356`, `0x0003a366`, `0x0003a38c`,
`0x0003a39c`, `0x0003a3c2`, `0x0003a3d2`, `0x0003a3f8`, `0x0003a408` and
`0x0003a42e`, and they are one loop unrolled six times: a minimum search over the
six face-axis overlaps at `[esp+0x80]`..`[esp+0x94]`, seeded with `FLT_MAX` from
`[0x10106858]`. The value in `st(0)` at every one of the eleven is either that
memory constant or one of the six overlaps, each reached by a bare
`fld dword ptr`. Every possible setting is exactly a `float`, so a spill has
nothing to truncate. **The forecast stands: expect bit-exact.**

`phys_fn_001741`'s ten are the other kind, and the last dispatch read them right.
They are five copies of one clip — `0x00039475`/`0x0003948a`,
`0x00039508`/`0x0003951d`, `0x000395a9`/`0x000395be`, `0x0003963c`/`0x00039651`,
`0x000396bc`/`0x000396d9` — and each carries the `fdivp` quotient formed three
instructions earlier and consumed by an `fmulp` after the branch. **Expect the
divergence class.**

### The cost is 9,737 bytes across six rows, not 8,239 across four

Two rows were missing, and each was missing for a different reason.

| row | rva | size | state |
| --- | --- | ---: | --- |
| `phys_fn_001749` | `0x0003add0` | 775 | new — the entry, matrix A `[BOX][BOX]` |
| `phys_fn_001748` | `0x0003ace0` | 240 | new — the transpose-and-copy shim |
| `phys_fn_001745` | `0x00039c10` | 4,271 | new — the separating-axis search |
| `phys_fn_001741` | `0x00038ba0` | 2,953 | new — the clipping/manifold row |
| `phys_fn_001743` | `0x00039730` | 1,240 | **new, and missed** — its continuation |
| `phys_fn_001739` | `0x00038a90` | 258 | **CLOSED here** — quad containment and depth |

`phys_fn_001743` carries `notes: "continuation of the entry at 0x00038ba0"` and
is reached by `jmp 0x39730` at `0x00039727` over a seven-byte `lea esp,[esp]`
pad — the same alignment split `plane/box` has. A reader has to sum them, and the
last two cost tables did not.

`phys_fn_001739` was missed for a sharper reason, and it is a **third shape of
closure error** after the vtable-slot one and the two-of-three-call-sites one:

> **A continuation row is not a call target, so a call-edge closure never enters
> it, and every call the continuation makes is invisible.**

All four calls to `phys_fn_001739` — `0x000397fb`, `0x0003986f`, `0x000398d6`
and `0x00039933` — are inside `phys_fn_001743`, and every one is a plain
`call rel32`. Nothing exotic hid them; the walk simply never reached the row they
live in. A closure that starts at `phys_fn_001749` and follows direct edges
across *inventory rows* stops at `phys_fn_001741` and never sees the last 1,498
bytes. Phases 4 through 7 build one of these graphs each: **fold every
continuation row into its entry before walking, or the walk stops at the pad.**

So the corrected figures. The survey's 12,102 over ten rows is still an exact sum
and still the wrong thing to plan from; the closure is six new rows and 9,737
bytes, of which 258 are closed here, so **9,479 bytes remain** — alongside the
four unchanged stops (`phys_fn_002264`, `phys_fn_001653`, `phys_fn_000772`,
`phys_fn_000774`, 3,802 bytes) and the two rows the sphere entries already
closed. `phys_fn_001745` also owns a 24-byte switch table, `phys_data_000006` at
`0x0003acc0`, which its `jmp dword ptr [ecx*4+0x1003acc0]` at `0x0003a473` walks.

### The rest of `phys_fn_001745`, decoded

Recorded so the next dispatch does not derive it twice. `phys_fn_001748` copies
two 12-dword poses into its frame with each 3x3 **transposed** and the trailing
translation straight (`0x0003ace8`..`0x0003ad33` and `0x0003ad3e`..`0x0003ad9d`),
then calls `0x00039c10` with five stack arguments and three registers: `ebx` =
the first transposed pose, `eax` = the normal out-parameter, `edx` = box1's
extents.

`phys_fn_001745` is a textbook fifteen-axis OBB test with a fudge factor. It
forms `delta = centre1 - centre0` and `R = A^T B`, adds **1e-6f**
(`[0x10106880]`) to every `fabs(R[i][j])` (`0x00039cbf` onward), and then tests
each axis as `radiusSum - |projection|`, storing the result and branching on its
**sign bit** with a constant `0x80000000` in `eax` — `test eax,ecx; jne 0x3acb3`
at `0x00039ee8`, `0x00039f47`, `0x00039fa7` and `0x0003a007` for the six face
axes, and integer `cmp`/`jb` on masked magnitudes for the nine edge axes
(`0x0003a290`, `0x0003a2de` and the rest). Any separated axis returns 0.

After all fifteen pass it applies the cached-axis bias, runs the minimum search
described above, writes the cache back, takes the sign of the winning
projection's stored copy at `[esp+0x98 + 4*code]`, and dispatches on `code`
through the 24-byte table. Each of the six arms copies the winning axis out of
the pose into the normal out-parameter, negates all three components when the
projection's sign bit is **clear** (`test eax,eax; je` at `0x0003a47c` and its
five siblings), builds a 3x3 in its frame with `rep movsd`, and tail-calls
`phys_fn_001741` with five stack arguments plus `eax`.

`phys_fn_001741` is entered as `(NxVec3* points, NxReal* separations,
const NxReal* rotation3x3, NxReal extentY, NxReal extentZ)` with `eax` holding
the other box's transposed pose. Its four calls to `phys_fn_001739` are the
per-vertex depth query of a Sutherland-Hodgman clip.

### `phys_fn_001739`, transcribed and closed — 258 bytes, the leaf

```
collision name=box_quad_depth index=- rva=0x00038a90 owner=phys_fn_001739
    checks=1200000 oracle=64e0e657f9d2f3bb candidate=64e0e657f9d2f3bb mismatches=0
collision coverage name=box_quad_depth aimed_inside=11250 reversed=22482
    interpolated=36640 non_finite=12737 default_mismatches=0 simulate_mismatches=0
```

**Bit-exact under both control words.** It is the only row under matrix A
`[BOX][BOX]` that calls nothing, so it is where a leaf-first reconstruction of
the subtree starts.

**The operation order.** A containment test in the `(y, z)` plane, then an
interpolation of `x`. `ecx` holds four vertex pointers and the two floats are the
query point. For each corner in turn, with the previous corner starting at
`quad[3]` (`0x00038a90`):

```
first  = (q.y - P.y) * (point.z - P.z)      0x00038aad..0x00038abd
second = (q.z - P.z) * (point.y - P.y)      0x00038abf..0x00038acb
if(first - second >= 0.0) return -1.0f;     0x00038acd..0x00038ade
```

`test ah,1` at `0x00038ad9` reads C0 alone, so a corner exactly on an edge is
outside and a NaN cross product **keeps walking** — which is the only way a NaN
reaches the arithmetic below. `-1.0f` is the constant at `0x1010687c`. The two
corner components go through 32-bit slots by integer move (`0x00038aa9`,
`0x00038ab1`) and are read back with `fld`, and each iteration leaves its own
pair in `st(1)`/`st(0)` for the next (`0x00038ae4`, `0x00038aec`).

Inside, `x` is two edge projections added to `quad[0].x`:

```
partial = (NxReal)( u.x * (u.z*dz + u.y*dy) / ((u.z^2 + u.y^2) + u.x^2) + q0.x )
result  =           v.x * (v.z*dz + v.y*dy) / ((v.z^2 + v.x^2) + v.y^2) + partial
```

with `u = quad[1] - quad[0]`, `v = quad[3] - quad[0]`, `dy = point.y - q0.y` and
`dz = point.z - q0.z`. **Where it narrows:** the first quotient goes through a
32-bit slot at `0x00038b3d` and the second does not, so the two halves of one
answer are not carried at the same width. **And the two denominators are
accumulated in different orders** — `(z^2+y^2)+x^2` at `0x00038b29`..`0x00038b37`
against `(z^2+x^2)+y^2` at `0x00038b63`..`0x00038b71`.

**The ABI.** `ecx` carries the quad, the two floats are on the stack, and the
**caller** cleans (`add esp,8` at `0x0003993e` and at the three other sites). No
C++ declaration expresses that, so the oracle side goes through
`nxCallQuadDepthOracle` and the candidate, which takes ordinary parameters,
through its own thunk. Both spill `st(0)` with `fstp tbyte`: the one caller
compares the register against `0.0f` at `0x00039938` *before* narrowing it with
`fst dword` at `0x0003994c`, so a `double` return would round away the bits that
decide that comparison.

**Distinguishing inputs driven.**

| input | why | count |
| --- | --- | ---: |
| a rectangle wound the way the test accepts, point inside | the only way to reach the interpolation at all | `aimed_inside=11250` |
| the same rectangle wound the other way | rejected at the first edge, every time | `reversed=22482` |
| a point outside a correctly wound quad | walks the loop to a later index before leaving | `interpolated=36640` of 120,000 calls |
| raw 32-bit patterns in every component | a NaN cross product continues, so a NaN reaches the divide | `non_finite=12737` |

Every one of the 12,737 non-finite answers is a NaN; no draw produces an
infinity. **This block does not canonicalise them**, unlike `segment_segment`:
removing the canonicalisation entirely leaves `mismatches=0`, so the two sides
agree on the payloads as well as on which answers are NaNs, and a filter that
hides nothing would only weaken the block. That was found by trying it, and the
registered digest is the un-canonicalised one.

**Mutations, each with a measured delta.** Built in `git archive` copies of the
implementation tree at `51c607e` under the scratchpad; the real tree was asserted
clean and at `HEAD` before and after; the un-mutated copy was configured, built
and run in the same session and read `mismatches=0`.

| mutation | words moved | split |
| --- | ---: | --- |
| the containment test loses its equal case (`>=` becomes `>`) | **544** | 286 default, 258 simulate |
| both denominators accumulated in one order | **10** | 4 default, 6 simulate |
| the first quotient left wide instead of narrowed | **103,619** | 50,230 / 53,389 |
| the second edge taken from `quad[2]` instead of `quad[3]` | **197,404** | 92,376 / 105,028 |

The first is the one worth naming: the boundary really is reached, so the
strictness here is measured rather than resting on the disassembly alone — which
is not true of the three boundary mutations recorded further up this file.

**The registration bites, by the strong form.** Degrading the generator so it
never builds the reversed winding takes `reversed` from 22,482 to **0**, moves
`interpolated` to 59,622 and the oracle digest to `b54a24eb5e5bf45f`, and the
harness's own exit code stays **0**. Neither registered line then appears in the
transcript, so the gate's assertion loop is what fails.

### The forecast held for the rows it was made about, and the instrument that makes it has a blind spot

The depth-at-branch instrument says `phys_fn_001739` should be bit-exact: its one
branch above depth 0 is the loop's back edge at `0x00038af0` and what it carries
is two bare `fld dword` loads. **It was not bit-exact as first written**, and the
reason is worth more than the row is.

| spelling | differs under `0x0f7f` | differs under `0x027f` |
| --- | ---: | ---: |
| `const double` locals for the deltas and the edge components | 88 | 0 |
| every use written out as a subexpression instead | 20,060 | 0 |
| the edge projection in its own function, inlinable | 68 | 0 |
| the same function, `__declspec(noinline)` | **0** | 0 |

Nothing about the arithmetic changed between the four. Every difference is an
8-byte spill slot truncating a 64-bit significand to 53 — the same cause as the
four rows this file closes on a pinned divergence — and **there is no branch
anywhere in the region where it happens**. The oracle holds five values in
`st(1)`..`st(6)` from `0x00038b0d` to `0x00038b39` and never stores one; MSVC
only leaves a `double` in a register while the whole live set fits, and beside
the containment loop it does not fit.

Two things follow, and both correct how this program has been reading its own
signal:

1. **The cause is the spill, and a branch is only its commonest reason.**
   Counting x87 depth at branches is a good filter and it stays — depth 0
   everywhere is still sufficient for bit-exact — but it is blind to register
   pressure in straight-line code. Read a row for *what has to be live at once*,
   not only for what has to survive a branch.
2. **A divergence in that class is not automatically something to pin.** This one
   was removable, by shrinking the live set until MSVC had no reason to spill.
   Four Phase 3 rows are currently closed on a pinned divergence — 662, 242, 43
   and 24 words — and none of them has been retried this way. That is now an open
   question rather than a settled account.

### Things the oracle does here that a correct implementation would not

- **The separating-axis cache is per sink, not per pair.** `sink+0xe8` holds the
  axis the last `box/box` call settled on and biases the next call's search by
  0.999f, and nothing between calls resets it — `0x0005b620` stops at `+0x43`. So
  the axis chosen for one box pair perturbs the axis chosen for the next
  *different* box pair in the same step, and a pair's result depends on what was
  tested before it.
- **`phys_fn_001739` interpolates from three corners and tests four.** `quad[2]`
  reaches the containment loop and never reaches an arithmetic instruction, so a
  quad whose fourth corner is not coplanar with the other three is silently
  treated as if it were.
- **A NaN corner is inside every quad.** The containment test leaves on "greater
  or equal" and continues on "unordered", so a NaN anywhere in a corner or in the
  query point walks all four edges and reaches the divide. 12,737 of the 120,000
  answers in the block above are NaNs for that reason.
- **A degenerate quad divides by zero rather than being rejected.** The
  containment test reads only `y` and `z`, so a `quad[1]` differing from `quad[0]`
  in `x` alone passes it and leaves a denominator of `u.x^2`; two corners equal in
  all three components are rejected by the test itself, which is the only reason
  the finite half of the generator cannot reach a zero denominator.
- **The separation the entry emits is negated and then masked.** `fld`/`fchs` at
  `0x0003b013`..`0x0003b018` and `and ebp,0x7fffffff` at `0x0003b064`, so the
  negation is invisible for every finite value — but the `fld dword` quiets a
  signalling NaN on the way through, which a bare mask would not.

### What the entry still needs, for whoever picks it up

`phys_fn_001749`'s stream half is confirmed to be the `plane/box` pattern down to
the instruction, and the shared helpers in `ContactGeneration.cpp` already cover
it: header written whenever the call emits (`test eax,eax; jne` at `0x0003ae58`),
normal block unconditional, `sink->[0x34]` set to `0` at `0x0003ae82` so the
fifth contact word at `0x0003b08e` is dead, and **no cap** on the manifold loop
at `0x0003b010`..`0x0003b0c6`. The orientation swap is at `0x0003ae67` and the
three `fchs` at `0x0003af50`..`0x0003af6e`. Points come from a 12-byte-strided
array at `esp+0x78` and separations from a parallel array at `esp+0x38`. None of
that can be closed before `phys_fn_001741` and `phys_fn_001745` are, because
every path through the entry that emits anything runs through them.

## `phys_fn_001741` decoded, ahead of transcribing it — 4,193 bytes in five stages

The next row of `box/box` after the leaf, and the one the forecast puts in the
divergence class. Decoded here so the dispatch that transcribes it starts from a
structure rather than from 1,190 instructions, exactly as `plane/capsule` and the
`box/box` entry were handed over.

### The frame and the arguments

`sub esp,0x150` then `push ebx, ebp, esi, edi`, so the body's `esp` is
`entry - 0x160` and the five stack arguments are at `+0x164` .. `+0x174`:

| argument | what | established at |
| --- | --- | --- |
| `a0` `+0x164` | the **points** array the caller reads back, 3 floats per contact | written `0x00038fdc`..`0x00038fef` |
| `a1` `+0x168` | the **separations** array, one float per contact | written `0x00038fd1` |
| `a2` `+0x16c` | a 12-dword pose: the reference box's 3x3 followed by its centre | `ebp`, `0x00038ba8` |
| `a3` `+0x170` | the reference face's **y** half-extent, by value | `esi`, and `0x00039926` |
| `a4` `+0x174` | its **z** half-extent, by value | `edi`, and `0x0003991f` |
| `eax` | a 12-dword pose for the *other* box, by register | read from `0x00038bb3` on |

The register argument is `phys_fn_001745`'s doing (`mov eax,ebp` at `0x0003a5b6`)
and no C++ declaration expresses it, so the oracle side needs a thunk — the
`nxCallQuadDepthOracle` pattern this file already has for the leaf.

**It returns the contact count in `eax`** — `mov eax,ebx` at `0x00039bfe`, and
`ebx` is the running count the eight corner blocks and the clip increment. That
is the count `phys_fn_001749`'s manifold loop walks.

### Stage 1 — the relative rotation and the delta (`0x00038ba0`..`0x00038d10`)

Nine dot products of `a2`'s rows against `eax`'s rows into `[esp+0x10c]`..
`[esp+0x12c]`, each accumulated **y, z, x** (`0x00038baf`..`0x00038bc6` is the
pattern), then `rep movsd` of all nine into a second copy at `[esp+0x130]`
(`0x00038cc8`). The centre difference `eax[0x24..0x2c] - a2[0x24..0x2c]` is formed
at `0x00038cca` and projected onto `a2`'s three rows into `[esp+0x2c]`,
`[esp+0x30]` and the third slot.

Two copies of one matrix is not an accident of transcription: the second is what
stage 5 reads back to return the points to world space.

### Stage 2 — eight corners, unrolled (`0x00038fa8`..`0x000393b7`)

Eight identical blocks, each of the shape

```
jns  <skip>                       the depth's sign: only a penetrating corner counts
mov  eax,[esp+0x58] ; and eax,0x7fffffff ; cmp eax,esi ; ja <skip>   |y| > extentY
mov  eax,[esp+0x5c] ; and eax,0x7fffffff ; cmp eax,edx ; ja <skip>   |z| > extentZ
...                               accept: write the separation and the point
```

**The two extent tests are integer compares on masked float words**, not `fcom` —
`and eax,0x7fffffff` then `cmp`/`ja`, which is an ordering test on the magnitude
that works because an IEEE float's bit pattern is monotonic for non-negative
values. It has two consequences to reproduce: a NaN coordinate compares as a very
large magnitude and is therefore *rejected*, where every `fcom` in this component
lets the unordered case through; and `-0.0` compares equal to `+0.0`.

**The separation is the corner's x in the reference frame**, and it is also the
point's x: `0x00038fc6` loads `[esp+0x54]` once and stores it both to `*a1` and to
`a0[0]`. `a0[1]` and `a0[2]` come from `[esp+0x58]`/`[esp+0x5c]`.

### Stage 3 — the edge clip, five cases (`0x000393c5`..`0x000396fc`)

Each case is the same seven instructions and this is where the divergence-class
forecast lives:

```
fld  [esp+0x170]        ; the extent
fsub [ecx+4]            ; - c.y
fld  [esi+4]
fsub [ecx+4]            ; s.y - c.y
fdivp st(1)             ; t, the clipping parameter -- WIDE, never stored
fld  [esi+8] ; fsub [ecx+8] ; fmul st(1) ; fadd [ecx+8]
fst  [esp+0x10]         ; a narrowed copy; the wide one stays
fabs ; fcomp [esp+0x174] ; test ah,0x41 ; jp   <-- BRANCH 1, t in st(0)
fld  [esi] ; fsub [ecx] ; fmulp st(1)     <-- t consumed here
fadd [ecx] ; fcom 0.0f ; test ah,1 ; jne  <-- BRANCH 2
fst  [edx+ebx*4]        ; the separation
```

at `0x00039446`, `0x000394dc`, `0x0003957a`, `0x00039610` and `0x0003967e`.
**All ten depth-1 branches carry `t`**, an `fdivp` quotient with no store between
the divide and the `fmulp` that consumes it. That is the `phys_fn_001690` kind and
the forecast expects the class — but the leaf and `NxBuildSmoothNormals` both say
to run the carrier search before pinning, and the carrier here is named: it is
`t`, and it is live across exactly two branches with nothing else live but `esi`,
`ecx`, `edx` and `ebx`, all integers. That is the shape a `__declspec(noinline)`
helper *can* fix, unlike `phys_fn_001690`'s `f`.

### Stage 4 — the other box's face vertices (`0x00039730`..`0x0003998c`)

This is the `phys_fn_001743` continuation, reached by `jmp 0x39730` at
`0x00039727` over a seven-byte pad. A loop over a 15-entry table
(`cmp [esp+0x14],0xf; je 0x3998c` at `0x00039730`) driven by the index list at
`0x10107a70`, with **four calls to `phys_fn_001739`** at `0x000397fb`,
`0x0003986f`, `0x000398d6` and `0x00039933`, each passing the reference face's two
half-extents as the query point and a four-pointer quad in `[esp+0x40]`. The
return is rejected by `fcom 0.0f; test ah,1; jne` at `0x00039938`..`0x00039946`,
so a negative interpolated depth — including the `-1.0f` the leaf returns for a
point outside the quad — contributes nothing.

**That row is now closed**, so stage 4 has no unreconstructed callee.

### Stage 5 — back to world space (`0x0003998c`..`0x00039c00`)

Every accumulated point is multiplied by the *second* copy of the matrix and the
reference centre `a2[0x24..0x2c]` is added. Four-way unrolled
(`lea ecx,[ebx-4]; shr ecx,2` at `0x0003999e`) with a remainder loop at
`0x00039b6e`, which is a shape a transcription has to get right only in its
arithmetic — the unrolling itself is not observable.

### Two things the harness must do, which no single-pair block can

1. **Drive two *different* box pairs through one sink.** `sink+0xe8` is a
   warm-start axis index that the per-step reset never clears, so a
   reimplementation that clears it — the natural thing to write — agrees on the
   first pair and diverges from the second. A block that resets the sink between
   pairs cannot see that, and the mutation that would catch it is exactly
   "clear the cache per call".
2. **Plan the manifold inputs with the geometry rows, not after them.**
   Face-face, edge-edge and vertex-face are distinguished by which stage produces
   the contacts — stage 2 for face-face, stage 3 for edge-edge, stage 4 for
   vertex-face — so the counters that register them belong on this row, where the
   stages are separable, as well as on the entry.

## `phys_fn_001741` transcribed and closed — 4,193 bytes, and the carrier was not the one named

```
collision name=box_clip.random index=- rva=0x00038ba0 owner=phys_fn_001741
    checks=2782400 oracle=13358be7c75d880c candidate=13358be7c75d880c mismatches=0
collision name=box_clip.aimed index=- rva=0x00038ba0 owner=phys_fn_001741
    checks=16158496 oracle=3adbe72ad076985c candidate=3adbe72ad076985c mismatches=0
```

**Bit-exact under both control words**, on a raw-bit family and an aimed one. The
full account is in `.superpowers/sdd/p3-capsule-report.md`; four things belong
here because they correct or extend what this file records.

**The decode's argument table is missing a register.** The row reads `[edx]`,
`[edx+4]` and `[edx+8]` at `0x00038d1b`, `0x00038d40` and `0x00038d75` — the
incident box's three half extents. `phys_fn_001745`'s last two dispatch arms set
it with `mov edx,edi` before the call (`0x0003a9de`, `0x0003ac9d`) and its first
four never touch it, so on four of six arms it survives from `phys_fn_001748`
across a 4,271-byte function. Whoever transcribes `phys_fn_001745` has to set it
on every arm.

**The decode named the edge table's address wrongly.** `0x10107a70` is the *face*
list — six faces of four corner indices, walked by the stage 4 loop from a cursor
at `0x10107a78` reading `[ecx-8]`..`[ecx+4]`. The edge list is at `0x10107ad0`,
twelve pairs, and its loop stops when the cursor reaches `0x10107b30` — the same
address as the `0.999f` warm-start bias constant.

**The forecast's named carrier was wrong, and the row still went to zero.** All
ten depth-1 branches do carry an `fdivp` quotient, and MSVC does not spill it:
moving it into a `__declspec(noinline)` helper changed **not one bit**, and so
did three further attempts — the delta projection, the nine dot products and the
stage 4 depth. The 276 differing words were four values held across the
straight-line corner construction (`0x00038dca`, `0x00038de6`..`0x00038f71`),
each used by four corners and each materialised into an 8-byte slot. What found
them was `dumpbin /disasm` on the object and a scan for an 8-byte store whose
previous instruction is arithmetic: exactly four sites, all
`fadd qword ptr [X]; fstp qword ptr [X]`. So:

> **Scan the listing for `fst`/`fstp qword` after an arithmetic instruction
> before spending a build on a guess.** It is one command and it names every
> spill site at once. Three of the four zero-moving attempts here cost a full
> build and run each, and the depth-at-branch instrument pointed at none of them.

That is now in the README beside the find-the-carrier note, together with the
plain statement that the depth heuristic **mispredicted this row**: it named a
real wide value crossing two real branches, and MSVC had not spilled it.

**All four zeroes were re-measured with a forced rebuild**, because a stale
object is exactly how a false zero would look. Taking the closed row and undoing
only the corner-combination fix — leaving the other four helpers in place —
reproduces `candidate=ad5c57fbd5dd9f5c mismatches=276`, byte for byte the digest
the plain spelling gave before any helper existed. So the four together move
nothing, measured in one clean run rather than inferred from four incremental
ones. The eight mutations below were re-measured the same way, with a control
before and after, and every number reproduced.

### The eight mutations, and what the record carries for each

Built in `git archive` copies of the implementation tree at `2580aff` under the
scratchpad, every mutant forcing a rebuild of the file it changes. The un-mutated
copy was built and run in the same session, before the mutants and again after
them, and read `mismatches=0` on both families each time — that zero is the
control in the third column.

| mutation | words moved | control |
| --- | --- | ---: |
| the corner depth test written as a float compare instead of the oracle's integer sign test on the masked word | **584,167** | 0 |
| *site not recorded* | **557,830** | 0 |
| *site not recorded* | **120** and **57,942** on the two blocks | 0 |
| *site not recorded* | **3,053** | 0 |
| the two endpoint swaps' cumulative order | 0 | 0 |
| stage 4's `mask == 0xf` early exit | 0 | 0 |
| stage 5's three product associations | 0 | 0 |
| the fifth clip case's `fchs` numerator | 0 | 0 |

**Three counts are published without a site, and that is deliberate.** Both
committed records state them as bare numbers: `gates/phase3-closure.json` as
"seven further mutations moved 557,830, 120 + 57,942 and 3,053", and the
programme record as "Mutations 584,167 / 557,830 / 57,942 / 3,053 against a
control reading 0 in the same session". Neither says which site each belongs to,
and choosing one from here would be a reconstruction rather than a reading. The
row closes on the first line, which is the count the closure ledger records for
it and the only non-zero delta on this row that names what it perturbed.

**The four zeroes are recorded rather than replaced quietly.** All four are
transcribed as the oracle has them and rest on the disassembly alone, which is
the same position as the three boundary mutations recorded further up this file.

### A probable third shipped defect: the seventeenth contact overwrites a return address

`phys_fn_001741` has no contact cap. Its count is bounded only by its own
structure — eight corners, five clip cases on each of twelve edges, four face
vertices, so 72 — and the arrays it writes into belong to `phys_fn_001749`.

**Verified from the prologue rather than estimated.** `sub esp,0x128` then four
pushes at `0x0003add0`..`0x0003ade0`, so the body's `esp` is `entry - 0x138` and
the frame runs `+0x10`..`+0x138`. The three output buffers are the first three
arguments of the `phys_fn_001748` call at `0x0003ae4a`, and their addresses are
the three `lea`s at `0x0003ae38`, `0x0003ae3d` and `0x0003ae42` resolved through
the pushes already made:

| what | body-relative | extent |
| --- | --- | --- |
| the contact normal | `+0x20` | three floats |
| separations | `+0x38` | `+0x38`..`+0x78`, **16 floats** |
| points | `+0x78` | `+0x78`..`+0x138`, **16 `NxVec3`** |

`+0x138` is `entry + 0`, which is the **return address**. The points array ends
flush against it, and nothing lies between. So:

- the **17th separation** lands on `points[0].x` — silent corruption of a
  contact the caller then emits;
- the **17th point** lands on the return address, and the two dwords after it
  are the caller's first two arguments. `phys_fn_001749` has no `/GS` cookie —
  its prologue is a bare `sub esp,0x128` — so the corrupted address is simply
  returned to.

The differential measures the count rather than assuming it away:
`max_contacts=20 over_sixteen=77` on the raw-bit family and `max_contacts=18
over_sixteen=16` on the aimed one.

**What that does NOT yet establish, and the distinction matters for Phase 8.**
Both families drive `phys_fn_001741` at its own address with a *synthetic*
reference face. In the shipped call chain the face is whichever one
`phys_fn_001745`'s fifteen-axis search settles on — the axis of minimum overlap,
biased toward the cached one — and a minimum-overlap face is exactly the
configuration least likely to admit a large manifold. **Whether a real caller can
reach seventeen is open until `phys_fn_001745` is reconstructed**, and the
measurement to make then is the count distribution when the reference face comes
from the search rather than from a generator. Until that is done this belongs
beside `NxArray::insert`'s heap overflow and `NxSeparatingAxis`'s uninitialised
stack as shipped behaviour to reproduce rather than correct — with the
reachability question recorded as open rather than answered either way.

### The handover: `phys_fn_001745`'s ABI, resolved rather than left to be derived

The remaining 5,286 bytes are one unit — `phys_fn_001748` and `phys_fn_001749`
cannot be driven differentially until `phys_fn_001745` exists, because every path
through them runs into it — so the next dispatch starts here. The call at
`0x0003adc3` is resolved by walking `phys_fn_001748`'s pushes; its body `esp` is
`entry - 0x64` and its own eight arguments come from `phys_fn_001749`'s push
sequence at `0x0003ae21`..`0x0003ae49`.

| `phys_fn_001745` takes | where | what |
| --- | --- | --- |
| stack 0 | `0x0003adbe` | the points array — `phys_fn_001749`'s `esp+0x78` |
| stack 1 | `0x0003adb6` | the separations array — its `esp+0x38` |
| stack 2 | `0x0003adae` | box A's three half extents |
| stack 3 | `0x0003ada9` | the second transposed pose, `phys_fn_001748`'s `esp+0x04` |
| stack 4 | `0x0003ad94` | **`sink+0xe8`**, the warm-start axis cache |
| `ebx` | `0x0003adbf` | the first transposed pose, `phys_fn_001748`'s `esp+0x34` |
| `eax` | `0x0003adb7` | the contact normal out-parameter |
| `edx` | `0x0003adaf` | box B's three half extents |

`add esp,0x14` at `0x0003adc8` confirms the five. So the row needs a thunk with
**three** register arguments, one more than the leaf and one more than the row
closed above, and the harness has the `nxCallClipFaceOracle` pattern for it.

**A new borrowed offset, established here and owned by Phase 5.** The entry reads
each shape's extents from `+0xe4` and its pose from `+0x0c` — `lea eax,[esi+0xe4]`
and `lea edx,[esi+0xc]` at `0x0003ae31` and `0x0003ae2d`, and the same pair on
`ebp`. `+0x0c` is already in the borrowed table as the pose; **`Shape+0xe4` is
three floats, a box's half extents**, and is not. It is added to that section
rather than assumed.

## `phys_fn_001745` transcribed and closed — 4,271 bytes, and the decode needed two corrections

```
collision name=box_axis.random index=- rva=0x00039c10 owner=phys_fn_001745
    checks=2933136 oracle=aa8dab9438d06f35 candidate=aa8dab9438d06f35 mismatches=0
collision name=box_axis.aimed index=- rva=0x00039c10 owner=phys_fn_001745
    checks=10747984 oracle=2e654ff01e6b9df7 candidate=2e654ff01e6b9df7 mismatches=0
```

**Bit-exact under both control words, on the first build.** The full account is
in `.superpowers/sdd/p3-capsule-report.md`; what belongs here is the part that
corrects or extends this file.

### The handover was wrong about `edx`, and about what the cache byte does

Both are the shape the last dispatch predicted: a decode contradicted by the
transcription.

**Three arms reload `edx`, at two addresses.** This file records "the last two
of `phys_fn_001745`'s six dispatch arms load it with `mov edx,edi`
(`0x0003a9de`, `0x0003ac9d`) and its first four never touch it". The two
addresses are right and the arm count is not: `0x0003a9de` is arm 3's own tail
and `0x0003ac9d` is the tail **arms 4 and 5 share**, entered from `0x0003aab0`,
`0x0003ab3d` and `0x0003abf9`. So the first **three** arms leave `edx` holding
what `phys_fn_001748` put there — box B's extents — which is exactly the
incident box for a reference face on box A.

**A warm cache byte is a branch, not a bias.** This file describes `sink+0xe8`
as an index whose overlap is "multiplied by 0.999f so the same axis wins again",
and that is half of it. `cmp al,0xff; jne 0x0003a2e8` at `0x0003a01e` jumps
**past the nine edge-axis tests entirely**, so reading the byte selects between
two different algorithms. A pair whose only separating axis is an edge cross
product is reported as overlapping, with a manifold, whenever the sink is
carrying an index. The differential measures it: the same geometry driven from a
cold byte and from a warm one answers differently 1,276 times in 144,000 aimed
calls and 7,786 times in 144,000 raw-bit ones.

**The carry across pairs is measured too.** Reading the byte as though every call
started cold — the mutation that removes the carry rather than the bias — moves
**105,675** and **79,986** words on the two blocks, against the un-mutated copy
reading `mismatches=0` on both in the same session. That is what says the byte
survives from one shape pair into the next rather than living inside a call, and
it is why the harness drives sequences of pairs through one sink instead of
resetting between them. It is a subsidiary measurement: this row closes on the
1,751,126 below, not on these two.

### The row, in the order it runs

`sub esp,0xa4` then three pushes, so the body's `esp` is `entry - 0xb0`, the
five stack arguments are at `+0xb4`..`+0xc4` and the locals run `+0x0c`..`+0xac`.

1. `delta = poseB.centre - poseA.centre`, in world, narrowed per component
   (`0x00039c1e`).
2. `R[i][j] = dot(A.row i, B.row j)`, **nine entries in nine accumulation
   orders**, each narrowed by its own `fstp dword`; and six projections of
   `delta` onto the two poses' rows, also each in its own order — the last,
   `s2` at `0x00039faf`, is x, z, y where four of the other five are z, y, x.
3. `f[i][j] = fabs(R[i][j]) + 1e-6f` (`[0x10106880]`), narrowed.
4. Six face axes, interleaved with the rows they need, each
   `radiusSum - |projection|` formed entirely in the x87 stack and narrowed once
   by the store at `[esp+0x80 + 4k]`. The test is `mov eax,0x80000000;
   test eax,ecx; jne 0x0003acb3` on the **stored word**, so it is an integer sign
   test: `-0.0f` separates and a NaN with a clear sign bit does not.
5. The cache byte (`0x0003a016`). `0` and `0xff` fall into the nine edge axes;
   anything else jumps to `0x0003a2e8`.
6. Nine edge axes. Each forms `a*b - c*d` and a four-term radius sum, both
   narrowed, and compares them as **unsigned words with only the projection
   masked** (`and ecx,0x7fffffff; cmp [esp+0x18],ecx; jb`). Seven of the nine
   accumulate two of box A's terms and then two of box B's; **edges 7 and 8
   interleave them** (`0x0003a262`, `0x0003a2b0`).
7. The bias (`0x0003a2e8`) and the minimum search over the six overlaps
   (`0x0003a303`), seeded `FLT_MAX` from `[0x10106858]`.
8. `*cache = code + 1` (`0x0003a456`), the winning projection's stored sign bit
   (`0x0003a45e`), and `jmp dword ptr [ecx*4 + 0x1003acc0]`.

The 24-byte table `phys_data_000006` reads
`0x0003a47a, 0x0003a5ca, 0x0003a734, 0x0003a89d, 0x0003a9f4, 0x0003ab42` — six
arm entries and nothing else.

### The arm, once, because all six are the same construction

Reference is box A for `code < 3` and box B otherwise; `i = code % 3`; the pose
handed to `phys_fn_001741` is the reference's rows cyclically from `i` with the
face centre in slots 9..11, and its two range-check extents are
`refExtents[(i+1)%3]` and `refExtents[(i+2)%3]`.

| the sign bit of the winning projection | arms 0..2 | arms 3..5 |
| --- | --- | --- |
| set | rows verbatim, centre `- e*row0` | rows `(-r0,-r1,+r2)`, centre `+ e*row0` |
| clear | rows `(-r0,-r1,+r2)`, centre `+ e*row0` | rows verbatim, centre `- e*row0` |

**The two halves of the table take the opposite branch**, and spelling them the
same way moves 1,751,126 words. Every projection is measured along box A's
frame, which is what makes the reference face the one facing the other box on
both halves. The normal is always the winning row, copied as raw dwords and
negated with three `fchs` when the sign bit is clear — on both halves.

Only the first two rows are negated; the third is a raw copy on every path
(`0x0003a55e`, `0x0003a591`), which leaves the determinant's sign alone.

### The spill scan run as a negative control, and what it should read

The depth-at-branch forecast for this row was bit-exact and it held on the first
build, with no carrier search. The scan was run anyway, and it is worth
recording what a bit-exact row looks like through it:

```
?NxBoxBoxSeparatingAxis@@...   qword-stores=26  after-arithmetic=0
```

**Twenty-six 8-byte x87 stores and not one of them a spilled computed value.**
Every one widens a value that is already exactly a `float` — either freshly
`fld`ed from a 32-bit slot or the narrowed return of `nxBoxBoxDot`, which stores
and reloads its own result through a dword. So the number to read out of the
scan is the second one, not the first: a row with many qword stores can be
bit-exact and a row with four can differ on 276 words. `phys_fn_001741` is the
positive control and this is the negative one.

### The seventeenth contact: the open question, narrowed

This file records that `phys_fn_001741` has no cap, that `phys_fn_001749`'s
frame holds sixteen contacts with the return address flush against the sixteenth
point, and that **reachability from a real caller was open** because both of
that row's families handed it a synthetic reference face.

The face now comes from the search. Across 288,000 calls under both control
words, including an aimed mode built for the deep overlap of two nearly parallel
boxes:

| family | `max_contacts` | `at_sixteen` | `over_sixteen` |
| --- | ---: | ---: | ---: |
| `box_axis.random` | 12 | 0 | 0 |
| `box_axis.aimed` | **16** | **14** | **0** |

against `box_clip`'s 20 and 18 with a synthetic face. So the minimum-overlap
face does compress the manifold — and it **fills the frame exactly**, fourteen
times, with a margin of one contact. The overflow is neither shown reachable nor
shown unreachable, and a generator's coverage is not a proof; what has changed
is that the question is now about one specific configuration rather than about
the reachability graph. Phase 8 inherits it in that form.

### Things the oracle does here that a correct implementation would not

- **A warm cache byte skips the nine edge-axis tests**, so a pair that is
  genuinely separated along an edge cross product is reported as overlapping and
  a full manifold is emitted for it. Measured above.
- **The bias's range check admits 1..16 where six overlaps exist.**
  `cmp al,0x10; ja` / `test al,al; jbe`, and `lea eax,[esp + eax*4 + 0x7c]`
  walks past the overlap array into the stored projections and, at 16, into the
  argument area. Unreachable, because the only writers produce 1..6, `0xff` and
  `0`; recorded rather than reproduced.
- **A `-0.0f` overlap separates the pair and a `+0.0f` overlap does not**, from
  the integer sign test on the stored word.
- **Six NaN overlaps dispatch on arm 0.** The face test rejects only a set sign
  bit and the minimum search rejects the unordered case, so `code` keeps its
  initial 0 and the normal is taken from box A's first row.
- **The edge test is an unsigned compare of two float words with only one side
  masked**, so a NaN projection separates the pair — the opposite of every
  `fcom` in this component — and a negative radius sum separates nothing.
- **One face centre's three components are not carried at the same width**: x
  and y through 32-bit slots, z wide out of `fmul` into `fsub st(3)` or
  `fadd [ebx+0x2c]`.
- **The cache is written before the clip runs and never unwound**, so a pair
  that dispatches and then produces no contact still leaves its axis index for
  the next pair. Only `phys_fn_001749` clears it, to `0`.

### What remains in `box/box`, and it is now drivable

`phys_fn_001748` (240 bytes, the transpose-and-copy shim, no arithmetic) and
`phys_fn_001749` (775 bytes, the entry). Neither could be driven differentially
before this row existed, because every path through them runs into it; both can
now. The entry's stream half is the `plane/box` pattern and the shared helpers
already cover it. What is new in it is the orientation swap at `0x0003ae67` with
its three `fchs` at `0x0003af50`..`0x0003af6e`, the uncapped manifold loop at
`0x0003b010`..`0x0003b0c6`, the separation negated at `0x0003b013` and masked at
`0x0003b064`, and the `0` written to `sink+0xe8` at `0x0003ae5d`.

## `box/box` complete — the shim and the entry, and the overflow question is answered

```
collision name=box_shim index=- rva=0x0003ace0 owner=phys_fn_001748
    checks=2991776 oracle=3d36c90baebbedee candidate=3d36c90baebbedee mismatches=0
collision name=contact_box_box index=14 rva=0x0003add0 owner=phys_fn_001749
    checks=6007270 oracle=f9afbd75702ac4a2 candidate=f9afbd75702ac4a2 mismatches=0
```

**Bit-exact under both control words, both rows, on the first build.** The full
account is in `.superpowers/sdd/p3-capsule-report.md`; what belongs here is the
part that closes an open question this file left, and the four things the entry
does that the emitter does not.

### The seventeenth contact is REACHABLE, and the open question is closed

This file recorded, twice, that `phys_fn_001741` has no cap, that
`phys_fn_001749`'s frame holds sixteen contacts with the return address flush
against the sixteenth point and no `/GS` cookie, and that **reachability from a
real caller was open** because every family that had driven the count handed the
row a synthetic reference face. The search row narrowed it to
`max_contacts=16 at_sixteen=14 over_sixteen=0` over 288,000 calls. This closes
it the other way.

`box_shim` drives `phys_fn_001748` at its own address with **exactly the
arguments `phys_fn_001749` passes it** — `Shape+0xe4` and `Shape+0x0c` per shape,
which is what `lea eax,[esi+0xe4]` and `lea edx,[esi+0xc]` at `0x0003ae31` and
`0x0003ae2d` resolve to — but into the harness's own eighty-slot arrays rather
than the entry's sixteen. That substitution is the whole instrument: it lets an
oversized manifold be *measured* where driving it through the entry would return
the oracle into a contact coordinate.

```
collision coverage name=box_shim aimed=15954 separated=13747 emitted=130986
    carry_warmed=5342 over_sixteen=2 aimed_max=18 max_contacts=18
```

**`aimed_max=18`.** A real rotation matrix, physical extents, a physical
placement, and the reference face the fifteen-axis search itself selects: the
shipped chain returns eighteen contacts into a frame that holds sixteen.
Separations 17 and 18 land on `points[0].x` and `points[0].y`, corrupting a
contact the entry then emits, and point 17 lands on the return address. The rate
is about 2 in 32,000 aimed calls, which is why the search row's own generator
missed it across 288,000.

**The entry's block measures the same thing without driving it.** A pair whose
manifold does not fit does not produce a mismatch — it kills the run — so that
block pre-flights every pair through `phys_fn_001748` with the identical
arguments and a *copy* of the live cache byte, which returns exactly the count
the entry would produce. `overflow_skipped=0 probe_max=16 at_sixteen=18` says
this generator fills the frame exactly and never exceeds it, and the guard is
what stops a later retune from turning a coverage improvement into a crash.

So the third shipped defect this file recorded is no longer "probable, with
reachability open". It is **reachable, at the entry's own input surface, with
the boundary and the rate named** — beside `NxArray::insert`'s heap overflow and
`NxSeparatingAxis`'s uninitialised stack.

### `phys_fn_001748`, and why it is driven as a row of its own

240 bytes, twenty-four integer moves and one call. Each 3x3 is copied
**transposed** and each translation straight: `[eax+0] -> [esp+0x34]`,
`[eax+0xc] -> [esp+0x38]`, `[eax+0x18] -> [esp+0x3c]` says the copy's first row
is the source's first column, so `out[i*3+j] = in[j*3+i]`. Its eight arguments
are all on the stack, `add esp,0x20` at `0x0003ae4f`, so it is the one row of the
four in this subtree that needs no thunk.

It gets its own block because **no caller can distinguish a transpose from its
inverse unless the pose is asymmetric**. Copying without transposing moves
787,813 words in its own block and 1,175,310 through the entry; exchanging the
two transposed poses moves 833,114 and 1,176,674. Because every move is an
integer `mov`, a signalling NaN in a pose passes through it unquieted.

### `phys_fn_001749`: four things that are not the emitter

The stream half is confirmed to be the `plane/box` pattern down to the
instruction, and the shared helpers cover it unchanged. What is its own:

| what | where | measured |
| --- | --- | ---: |
| the orientation swap reads the FIRST shape's owner, not the second's | `0x0003ae67` | 169,456 |
| the pair header has no predicate | `0x0003ae58` is the only gate | 380,158 |
| the normal block has no predicate, and the cache it would consult is cleared first | `0x0003af45`, written `0x0003af81` | — |
| the feature half is always zero, so the fifth contact word is dead | `0x0003ae82`, tested `0x0003b08e` | 1,141,955 |
| the manifold loop has no cap | `0x0003b010`..`0x0003b0c6` | see above |
| the separation is negated AND masked | `0x0003b013`, `0x0003b064` | 0 |
| a pair that produced nothing clears the whole sink's cached axis | `0x0003ae5d` | 36,702 |

The header-predicate figure is the same defect measured a second time: the
`plane/box` row recorded 452,624 words for the identical change, and these are
the only two entries in matrix A that carry a second copy of the emitter.

The negate-then-mask figure is a **zero**, and it is recorded rather than
dropped. The two spellings agree on every finite value and on every infinity;
the only input that separates them is a *signalling* NaN separation, because
`fld dword` quiets one where a bare mask does not, and neither family produced
one. Transcribed as the oracle has it.

### The warm start, end to end

`warm_carried=18611`, `axis_cleared=13132`. The entry's block drives up to four
box pairs into **one sink** without resetting it between them — the state the
shipped pipeline is in, since `0x0005b620` covers `+0x10`..`+0x43` and this
entry is the only writer in the image that ever clears the byte. Treating the
byte as per-call scratch, saved before the call and restored after, moves
**89,102** words, and moves none of them on the first pair of any sequence.

Degrading the generator to one pair per sequence takes `warm_carried` to **0**
and the oracle digest from `f9afbd75702ac4a2` to `d44650efbd3b91c6` while both
sides still agree and the harness still exits 0 — which is the strong form for
exactly the registration a single-pair block could not have.

### Eight of nine matrix A entries, and the ninth is a stop

`plane/sphere`, `plane/box`, `plane/capsule`, `sphere/sphere`, `sphere/box`,
`sphere/capsule`, `capsule/capsule` and `box/box` are closed.
**`box/capsule` (`phys_fn_001753`) is the ninth**, and it is the stop this file
already specified: its partner's vtable slot 5 resolves to `phys_fn_000949` at
`0x00020880`, a Phase 5 row of 663 bytes, and no ledger names Phase 3 as a
driver for it. It is the only one of the nine in that position.

## The four pinned rows, retried against the spill finding

`phys_fn_001739` showed that a divergence in the `double`-liveness class can be
*removed* rather than pinned, so the four rows this file closed on a pinned count
were retried. One of them goes to zero. The other three do not, and what stops
each of them is different and worth more than the counts.

| row | pinned | after | what it turned out to be |
| --- | ---: | ---: | --- |
| `NxBuildSmoothNormals` | 24 | **0** | five spilled `double`s in the angle helper; the dot product was the carrier |
| `phys_fn_001775` capsule/capsule | 43 | 43 | partly inherited from `phys_fn_001690` below |
| `phys_fn_001010` capsule raycast | 242 | 242 | **not this row's at all** — it compiles with zero spills |
| `phys_fn_001690` segment/segment | 662 | 662 | the one spill that matters cannot be removed without adding a call |

### `NxBuildSmoothNormals`: 24 to zero, and it upgrades

```
collision name=step_smooth_normals index=- rva=export owner=phys_fn_002146
    checks=919352 oracle=6d5d4be60a607d94 candidate=6d5d4be60a607d94 mismatches=0
collision coverage name=step_smooth_normals non_finite_words=65973
    default_mismatches=0 simulate_mismatches=0
```

**Bit-exact under both control words**, so this row's closure upgrades from
"algorithmic agreement under a word it does not execute under" to a real
bit-exact closure in its actual execution context. It is one of the four exports
the recovered matrix put inside the simulation step, so `0x0f7f` is the word that
matters and it is now the word it agrees under.

The cause was in `angleAtVertex`, not in `NxBuildSmoothNormals` itself. The
oracle holds all six edge differences in `st(1)`..`st(7)` from `0x00053325` to
`0x000533a5` and stores none of them; MSVC spills five to 8-byte slots. The cross
product survives that because every one of its outputs is narrowed to `NxReal`
anyway — but the **dot product reaches `fpatan` wide**, so it was the single
place where the truncation was observable. Moving it into a
`__declspec(noinline)` helper that recomputes its three pairs took the count to
zero.

**Three things were tried first and every one of them moved exactly nothing**,
which is what identified the dot:

| attempt | result |
| --- | --- |
| `sqrt()` replaced by `fsqrt` at all three sites | candidate digest byte-identical |
| `atan2()` replaced by `fpatan` | byte-identical |
| the sqrt argument computed and rooted in one naked block, removing its 8-byte round trip | byte-identical |

The first two are kept anyway. The oracle uses `fsqrt` at `0x0005338b`,
`0x00053560` and `0x000537ad` and `fpatan` at `0x000533a7`, and the
reconstruction was calling `__CIsqrt` and `__CIatan2` — routines the oracle never
enters. That they agree bit for bit over 919,352 checks is a measurement, not a
licence to keep the wrong call: same footing as the `fabs` correction, and
recorded as a correction that moved no digest rather than dropped. The third was
reverted, because it bought nothing and replaced a documented per-component
expression with assembly.

**Why a root round trip never matters, measured twice.** `nxSqrt` passes its
argument and its result through 8-byte slots where the oracle keeps both in
`st(0)`. Making it `__declspec(naked)` so the result comes back in `st(0)`
un-stored changed **not one bit** in any of the four rows. Rounding a square root
to 64 bits and then to 53 agrees with rounding it to 53 once, so this particular
spill is benign wherever it appears. That is worth knowing before anyone spends
another dispatch on it.

### `phys_fn_001010`: the 242 is not this row's

`NxShapeRaycastCapsule` compiles to **zero** 8-byte spill slots. There is nothing
in it to fix. Its 242 comes from `NxRayCapsuleIntersect`, which is a **Task 2
export in `Geometry.cpp`, closed by its own differential**, and which compiles
with **37** qword stores — `nx/ny/nz`, `qx/qy/qz`, `axisDot`, `tangent`,
`farRoot` and the rest.

That is a new category and it should be stated plainly: **a pinned divergence can
belong to a callee that is itself a separately closed row.** Attributing 242
words to `phys_fn_001010` was wrong, and no amount of work on `phys_fn_001010`
would have moved it. Whoever picks this up starts in `Geometry.cpp` and has to
re-run `NxRayCapsuleIntersect`'s own differential alongside this one.

The same is true in part of `phys_fn_001775`, which calls
`NxSegmentSegmentSquareDistance`: some of its 43 is `phys_fn_001690`'s.

### `phys_fn_001690`: the fix is worse than the defect, and the mechanism is general

The spill that matters is `f`, the quadratic's constant term. The oracle forms it
at `0x00033f67` and keeps it in `st(0)` across the **entire** branch tree to
whichever leaf's final `faddp`; MSVC spills a named `double` that lives that long,
and `f` reaches the returned squared distance wide in all twenty-nine leaves. It
is exactly the shape the `nxQuadEdge` and `nxAngleDot` fixes address.

Applying the same fix — a `__declspec(noinline)` helper recomputing `f` per leaf —
took the divergence from 662 to **29,529**. The reason is general and is the
boundary of this whole technique:

> **A noinline helper removes a spill only when nothing else is live across the
> call.** MSVC must spill every live x87 value before a call, so a helper
> introduced into a region that already has live values trades one spill for
> several. `nxQuadEdge` and `nxAngleDot` worked because at those points nothing
> else was live; `f` is surrounded by `s`, `t`, `e`, `edge` and the running
> `result`, and moving it into a call spills all of them.

So `phys_fn_001690` stays pinned at 662, and the recorded limit is now
"we searched and the search has a named boundary" rather than "no C++ construct
controls it", which is false as stated. What has *not* been tried, and is the
honest next idea rather than a claim: shrinking the live set the other way, by
giving each leaf its own noinline function so that `f` is the only thing crossing
a call. That is a large restructuring of a 1,539-instruction row and it was not
attempted here.

### What the search actually established

1. **The class is not uniformly unfixable.** One of four went to zero, and the
   `box/box` leaf went to zero before it. "No C++ construct controls where MSVC
   spills" is true and is not the same claim as "the divergence cannot be
   removed": the live set *is* controllable, and the spill follows it.
2. **Find the carrier, not the spill.** `NxBuildSmoothNormals` has six spilled
   values and only one of them was observable, because every other one is
   narrowed to `NxReal` before it reaches an output. The diagnostic that works is
   to ask which spilled value reaches the answer while still wide — not to count
   spills.
3. **A `double` that a call sits inside cannot be helped this way.** The
   technique's precondition is an empty live set at the call, and it is worth
   checking before spending a build.
4. **Check whose row the divergence is** before attributing it. One of the four
   was a callee's.

## The rows

Nine rows, 1,650 bytes, every one with a mutation and a measured delta. Each
mutant was built in a `git archive` copy of the implementation tree at `aa2ac3b`
under the scratchpad; the real tree was asserted clean and at `HEAD` before and
after every run, and the un-mutated copy was built and run in the same session
and read `mismatches=0` on all sixteen blocks — so the zeroes are readings and
not a stalled instrument.

| row | rva | size | what it is | mutation | delta |
| --- | --- | ---: | --- | --- | ---: |
| `phys_fn_001899` | `0x00048a20` | 73 | B[PLANE][SPHERE] | `<= 0` becomes `< 0` | 4 |
| `phys_fn_001881` | `0x00047e90` | 136 | B[PLANE][BOX] | corner loop steps by 3 | 8,505 + 16,318 |
| `phys_fn_000943` | `0x00020750` | 139 | box corner in world space | first row kept wide instead of narrowed | 28,009 |
| `phys_fn_001889` | `0x00048270` | 244 | B[PLANE][CAPSULE] | capsule axis from column 0 | 6,372 + 8,628 |
| `phys_fn_001931` | `0x0004b800` | 84 | B[SPHERE][SPHERE] | squared distance narrowed to 32 bits | 70 |
| `phys_fn_001915` | `0x00049e70` | 133 | B[SPHERE][BOX] | two extents swapped in the copy | 5,974 + 17,030 |
| `phys_fn_001913` | `0x00049ca0` | 453 | sphere/box closest point | rotation used instead of its transpose | 5,754 + 16,032 + 8,114 |
| `phys_fn_001921` | `0x0004a3e0` | 196 | B[SPHERE][CAPSULE] | capsule axis from row 1 | 4,923 + 9,414 |
| `phys_fn_001738` | `0x000389d0` | 192 | B[BOX][BOX] | `fullTest` cleared | 3,134 + 4,332 |

Every Phase 3 callee of every row here is in this list. `phys_fn_001738` also
calls `NxBoxBoxIntersect` (`phys_fn_001702`), which is itself a Phase 3 row and
was closed by Task 2, and `phys_fn_001921` calls Foundation's
`NxComputeSquareDistance`, a Phase 2 Foundation row closed by its own
differential. There are no other callees, so the component is closed in the
sense the brief requires.

### Three mutations that moved nothing, recorded rather than replaced quietly

- Making `phys_fn_001931`'s comparison non-strict (`>` to `>=`) moves **zero**.
  The boundary `r² == d²` is never hit exactly by 240,000 randomized pairs. The
  strictness rests on the disassembly alone.
- Narrowing `phys_fn_001889`'s `p1.z` — the one endpoint component the oracle
  leaves in a register — moves **zero**. The plane-aimed generator does not put
  enough pairs within one ulp of the threshold for the last bit to cross it.
- Widening `phys_fn_001921`'s radius sum moves **zero**, and there is a reason:
  `NxComputeSquareDistance` returns `NxF32`, so the comparison is against a
  value already at 32 bits and `r²` essentially never straddles it.

All three are transcribed as the binary has them. This is the same situation
Task 2 recorded for a dot-product reordering: right, and not currently provable
by any instrument.

## Things the oracle does that a correct implementation would not

All reproduced.

- **Touching is not overlapping, except when it is.** `phys_fn_001931`
  (sphere/sphere) and `phys_fn_001921` (sphere/capsule) test `r² > d²`
  **strictly**, so two spheres exactly touching report no overlap.
  `phys_fn_001913` (sphere/box) tests `!(r² < d²)`, which **includes** touching,
  and `phys_fn_001899` (plane/sphere) tests `distance <= 0`, which also
  includes it. `phys_fn_001889` (plane/capsule) is strict again. Four kernels in
  one matrix and three different conventions at the boundary.
- **NaN is a miss on some entries and a hit on others.** Every threshold test is
  a single `fcomp` plus a parity or zero test on the status word, and which way
  the unordered case falls depends on which branch the compiler happened to
  emit. Plane/sphere, plane/box, plane/capsule, sphere/sphere and sphere/capsule
  all return **false** for a NaN. `phys_fn_001913` returns **true**: an
  unordered comparison against the extents leaves the axis unclamped, and a NaN
  z reaching the "is the centre inside the box" test with x and y unclamped
  takes the early `return true`.
- **Three kernels store past their own frame.** `phys_fn_001913` writes
  `-extents.z` into the caller's first argument slot with
  `fstp dword ptr [esp+0x2c]` at `0x00049d92`; `phys_fn_001889` writes the
  capsule half height into the caller's second argument slot at `0x0004828c`;
  `phys_fn_001921` does the same at `0x0004a3f6`. All three slots are dead by
  then, but all three are stores into the caller's stack frame.
- **The narrowing is per component, not per vector.** `phys_fn_000943` pushes
  all three rows of the rotation through 32-bit slots before adding the
  translation. `phys_fn_001889` narrows the x half-axis and keeps y and z, then
  keeps `p1.z` in a register while every other endpoint component goes through a
  slot. `phys_fn_001913` uses the wide separation z for the first row of the
  transform into box space and the narrowed one for the other two, and leaves
  the third world component wide while narrowing the first two. None of these
  is symmetric and none is an accident of transcription.
- **The corner signs are full integers.** `phys_fn_000943` takes three `int`
  parameters and converts them with `fild`; the caller passes `-1` and `+1`, but
  nothing in the callee constrains them.
- **`phys_fn_000943` is called eight times per plane/box test**, rebuilding the
  whole matrix product each time, and the loop counters really do step by 2 from
  `-1`.
- **`phys_fn_001738` always passes `fullTest = true`** to `NxBoxBoxIntersect`.
  That matters here: Task 2 established that the `fullTest = false` path reads
  uninitialised stack and is not a function of its arguments. The one caller in
  the dispatch matrix never takes it, so the defect is unreachable from the
  narrow phase.
- **A plane's `d` is added, not subtracted**, and the plane entries treat the
  normal as whatever the caller left there — nothing renormalises, so a shape
  whose pose has drifted is silently tested against a scaled plane. The
  generator here exercises unnormalised normals for that reason.

## The RED modes, and the four that are not reachable yet

The brief asks for RED modes covering pair symmetry, separation, penetration,
touching, multiple contacts, normal orientation, feature IDs, filtering
decisions, ordering and buffer limits. Six are present and four are not, and the
reason is structural rather than a shortfall of effort.

| mode | state |
| --- | --- |
| pair symmetry | **present, and measured against the oracle.** All 30 lower-triangle slots asserted null against the oracle's own constructor; then the oracle's *dispatcher* is handed a matrix with exactly one slot filled and asked whether it calls it, for each of the 36 ordered type pairs against each of the 36 slots and for both matrix halves — 2,592 probes, 0 wrong. That measures the index rule and the swap rather than restating them. An earlier version compared `NxCollisionPairIndex` against its own body and then against itself, which pinned the enum at 6 and checked nothing the oracle does. The aimed blocks additionally count how often the swapped call disagrees (`swap_differs` reaches 36,169 of 60,000 on sphere/box); the two like-type pairs register `swap_differs=0` because the comparison is guarded `if(type0 != type1)` and never runs for them, and box/box is the one where it would be worth running, since SAT is not obviously order-independent. |
| separation | **present.** The aimed generator places pairs from 0.2 to 1.6 times the sum of the two reaches; every block reports its true/false split and all fourteen are registered. |
| penetration | **present**, same generator: the overlapping half of that sweep, 15,402 to 110,278 true per block. |
| touching | **partial.** Reached statistically, not exactly: the boundary is never hit to the last bit, which is why two of the mutations above move nothing. An exact-touch generator would need to solve for the separation per shape pair and per orientation. |
| filtering decisions | **absent.** The filter is `phys_fn_000529` at `0x00010570`, a **Phase 7** row: it reads the group mask array at `0x00123a98`, calls a pair-record lookup at `0x0009a570`, and takes `this` as a scene. It is reachable by RVA but not constructible without a scene. Its rules are recorded below for whoever owns it. |
| contact ordering | **present**, from the emitter section above: `w4`/`w8`/`w11` is the width histogram of a contact that skipped the header, the normal block, or neither. |
| multiple contacts | **present**, as of `plane/capsule`. `phys_fn_001891` emits one contact per capsule endpoint, and the block counts the oracle's own contact count moving by one (14,296) and by two (24,368). It was absent while the only matrix A entry driven was `plane/sphere`, which can only ever emit once. |
| normal orientation | **present**, from the emitter section above: `negated_path=50004`, and deleting the id swap on that path moves 12,156 words. |
| feature IDs | **present**, from the `contact_emit` block: no matrix A entry reachable today passes anything but `0xffff`, so the emitter is driven directly with real ids. `real_feature_pairs=25106`. |
| buffer limits | **present**, as of `plane/box`, and not where this table looked for it. `phys_fn_001883` returns as soon as its sixth contact is emitted (`cmp eax,6; jae` at `0x00048218`) from a shape with eight corners, so a box wholly below a plane loses two: the limit is inside the kernel, not in a buffer. `c6=45492 c7=0` is it being reached and never exceeded. The *stream* growth path at `0x000b4de0` is still unexercised, because the differential pre-sizes it, and the trigger-pair array at `+0x5d8` still needs a scene. |

Four of those five were properties of contact generation and are now present:
the emitter section closed ordering, normal orientation and feature IDs, and
`plane/capsule` closed multiple contacts. **They were never blocked** — an
earlier draft of this file said they were and it was wrong, in a way worth
correcting precisely because it would have stopped the next dispatch before it
started. What is still absent is filtering, which needs a scene, and the contact
buffer limit, which the differential pre-sizes away.

The extra argument matrix A receives is the **third**, not the fourth. At the A
call site the dispatcher pushes four (`0x0005abd9`–`0x0005abe7`, `add esp,0x10`)
and at the B site it pushes `esi = [esp+0x28]` and two more
(`0x0005ac13`–`0x0005ac1d`, `add esp,0xc`), so B's third argument *is* A's
fourth — both are the dispatcher's own `param_4` — and A's third is the contact
sink. That is confirmed at `0x0004b876`, where the sphere/sphere entry feeds it
to `call 0x0001d610`.

### The emit convention, recovered

There are **two** emitters, not one: `phys_fn_000873` at `0x0001d610` and
`phys_fn_000875` at `0x0001d8e0`. The caller counts quoted elsewhere in this
file — fifteen and ten — are *inventory rows*, not call sites: `phys_fn_000873`
is reached from 22 sites across 15 rows, three of which are continuations of
another row rather than functions in their own right.

`phys_fn_000873` is `__thiscall` on the sink with **seven** stack arguments and
`ret 0x1c`, which agrees with Ghidra's seven-parameter prototype. Read off the
plane/sphere entry's push sequence at `0x00048ab8`–`0x00048aef`:

```
this = the contact sink (matrix A's third argument)
  0  shape1->[0x9c]
  1  shape0->[0x9c]
  2  separation, as float bits
  3  &point   (three floats)
  4  &normal  (three floats -- here the plane's own normal, passed in place)
  5  0xffff   feature id 0
  6  0xffff   feature id 1
```

**The two `0xffff` are the feature IDs**, which answers that RED mode's format
directly: a primitive pair has no feature, and the mesh entries are where real
indices appear.

### The stream format, from the emitter

Every word below has the instruction that appends it. This supersedes an earlier
version of this section that read the format off a probe transcript: that probe
had called `0x0005b620` on the sink, which — as the borrowed section now records
— operates on a different object and corrupted the run. The record half of that
reading survives the correction; the header half did not.

The stream is a flat `NxU32` array, `sink->[0x40]`, with `sink->[0x3c]` words
used. It is written in three nested levels, each opened by a count word that
later appends increment in place.

**Per shape pair**, when either collision object differs from `sink->[0x20]` /
`sink->[0x24]` (`0x0001d68e`); otherwise skipped entirely:

| word | value | appended at |
| --- | --- | --- |
| 0 | `shape1->[0x9c]` | `0x0001d6f6` |
| 1 | `shape0->[0x9c]` | `0x0001d720` |
| 2 | `(material << 24) \| (bothFeatureIdsValid << 16)` — and its **low 16 bits become this pair's normal-block counter**, which is why the material gets only one byte | `0x0001d761`–`0x0001d766`, incremented at `0x0001d81d` |

`material` is `owner->[8]->[0x240]`, taken from shape1's owner when
`shape1->[4]->[8]` is non-null and from shape0's otherwise
(`0x0001d726`–`0x0001d73e`). Writing the header also sets `sink->[0x18]` to the
current count and increments the word at `sink->[0x14]` (`0x0001d775`,
`0x0001d778`) — that is the pair counter.

**Per distinct normal**, when the normal differs from the cached
`sink->[0x28..0x30]` (`0x0001d78c`–`0x0001d7a3`); otherwise skipped:

| word | value | appended at |
| --- | --- | --- |
| 0..2 | `normal.x`, `normal.y`, `normal.z` | `0x0001d7d9`–`0x0001d7e6` |
| 3 | `0`, the contact count for this normal | `0x0001d807` |

Then `sink->[0x1c]` is set to the index of that zero and the word at
`sink->[0x18]` is incremented (`0x0001d811`–`0x0001d81d`).

**Per contact**, always:

| word | value | appended at |
| --- | --- | --- |
| 0..2 | `point.x`, `point.y`, `point.z` | `0x0001d851`–`0x0001d85e` |
| 3 | `separation & 0x7fffffff` | masked `0x0001d86d`, appended `0x0001d886` |
| 4 | `(featureId1 << 16) \| featureId0`, **only if** `sink->[0x34] & 1` | `0x0001d8ad`–`0x0001d8c5` |

and `sink->[0x10]` is incremented (`0x0001d827`) and the word at `sink->[0x1c]`
(`0x0001d895`).

Two things to note, because both were misread before. The separation is stored
with its **sign bit masked off**, not negated — `and ebx, 0x7fffffff` at
`0x0001d86d`. For the negative separations a penetrating contact produces the
two are indistinguishable, which is exactly why the probe reading looked like a
negation. And the feature-id word is **conditional**, so a contact record is four
words for a primitive pair and five where both feature ids are real.

For the plane/sphere entry the point is `centre - radius * normal` — a point on
the *sphere's* surface, not on the plane — from `fld st(0)` at `0x00048ab6`,
which copies the radius rather than the separation.

## BORROWED LAYOUT — Phase 5 and Phase 7 own these rows, not Phase 3

Everything in this section is recovered from the instruction stream, with the
address that establishes each offset. **Phase 3 reads it; Phase 3 does not claim
it.** The rows that implement these objects belong to Phase 5 (shapes, the
collision object, the owner) and Phase 7 (the contact sink), and those phases
must still close them. When they do, they should check their own recovery
against this table and report any disagreement as a finding rather than silently
preferring one version.

The precedent is Phase 2, which wrote four slot bodies belonging to Phases 3 and
6 into `PhysicsInternal.cpp` so an accessor had an object to install, disclosed
the phase of each, and did not count them as Phase 2 rows. Same shape, larger
scale.

**Nothing here is inferred to make a call complete.** Where a field's *type* is
not recoverable from the instruction stream, this table says what the code does
with it and stops there.

**What this section does NOT establish, named because a row already needed it.**
`phys_fn_002264`, the continuous-collision sweep both sphere entries of matrix A
reach through `phys_fn_002266`, reads three things nothing below covers, and that
is why it is a stop rather than a transcription:

| what | where it is read | why it is not here |
| --- | --- | --- |
| a **second pose** at `Shape+0x3c`..`+0x68` | copied out at `0x00055ebe` onward, beside `+0x0c`..`+0x38` | 24 dwords shaped like another `NxMat34`; nothing names it |
| the **sink past `+0x44`** — `+0xdc`, `+0xe0`, `+0xe4`, `+0xe9` | read `0x00056196`, written `0x000561c0`..`0x000561d2` | the table below stops at `0x44` and explains it as a `0x10` prefix plus a `0x34` sub-object |

`box/box` reaches into the same region from the other side: `phys_fn_001749`
passes **`sink+0xe8`** as an out-parameter at `0x0003ae1b` and writes a zero byte
there itself at `0x0003ae5d`. Two independent rows touching `+0xe8` and `+0xe9`
is enough to say the sink really does continue past `0x44`; it is not enough to
say what lives there, so it stays unnamed. Whether `box/box` is a stop on it
depends on whether anything *reads* that byte without writing it first, which is
`phys_fn_001748` and `phys_fn_001745`'s to answer.
| **vtable slot 7** | `call [eax+0x1c]` at `0x000562c3` and `0x000564b9` | the slot-5 table above was resolved per receiver type from each constructor; slot 7 never has been |

Whoever resolves slot 7 should do it the same way slot 5 was done — from the
vtable store in each of the five shape constructors, per receiver type, not once.

### `Shape` — Phase 5

| offset | what | established at |
| --- | --- | --- |
| `+0x00` | vtable; base `Shape` writes `0x10107494` | `0x0002553d` |
| `+0x04` | the owner, taken from the constructor's first argument | `0x00025543` |
| `+0x0c`..`+0x38` | pose: `NxMat33` then `NxVec3`, identity-initialised | `0x00025555`..`0x0002556d` |
| `+0x9c` | pointer to the 0x1c-byte collision object below; zeroed by the base constructor and filled by each derived one | `0x00025533` (zero), `0x00024f0b`, `0x00027805`, `0x00027dff` (fill) |
| `+0xd0` | `NxShapeType`; the sphere constructor writes 1 and the mesh constructor 4 | `0x0002780b`, `0x00027e05` |
| `+0xe4` | three floats, a box's half extents; the `box/box` entry passes `&shape->[0xe4]` to the transpose shim for both shapes | `0x0003ae31`, `0x0003ae26` |

### The 0x1c-byte collision object at `Shape+0x9c` — Phase 5

Allocated through the SDK allocator singleton at `0x101041bc` with an explicit
`push 0x1c`, then constructed with the shape as its only argument. Three
constructors, one per shape family, byte-identical in shape:
`phys_fn_001159` (`0x00024250`), `phys_fn_001193` (`0x000247c0`),
`phys_fn_001241` (`0x00024e40`).

| offset | what | established at |
| --- | --- | --- |
| size `0x1c` | the allocation request | `0x000277f0` (`push 0x1c`), allocator read at `0x000277e4` |
| `+0x00` | vtable, rewritten twice during construction; final value differs per family (`0x101070b0`, `0x10107218`, `0x10107388`) | `0x0002427d`, `0x000247ed`, `0x00024e6d` |
| `+0x04` | zero | `0x000247cb` |
| `+0x08` | **back-pointer to the shape** | `0x000247e9`, `0x00024279`, `0x00024e69` |
| `+0x0c` | a nested sub-object with its own vtable, constructed by `0x0005ba70` | `0x000247d7`, `0x000247e0` |
| `+0x18` | the shape again | `0x000247e6`, `0x00024276`, `0x00024e66` |

### The contact sink — Phase 7

| offset | what | established at |
| --- | --- | --- |
| `+0x08` | the identity the current stream is oriented to; compared against `owner->[8]` | compared `0x0001d62c` |
| `+0x10` | running contact count, incremented once per contact | `0x0001d81f`–`0x0001d827` |
| `+0x14` | index of the stream word holding the pair count | `0x0001d76c`–`0x0001d778` |
| `+0x18` | index of the stream word holding this pair's normal-block count | written `0x0001d775`, used `0x0001d811` |
| `+0x1c` | index of the stream word holding this normal's contact count | written `0x0001d81a`, used `0x0001d88f` |
| `+0x20` | shape1's collision object as of the last header | compared `0x0001d67d`, written `0x0001d6c0` |
| `+0x24` | shape0's collision object as of the last header | compared `0x0001d688`, written `0x0001d6c9` |
| `+0x28`/`+0x2c`/`+0x30` | the **cached contact normal**, three floats | compared `0x0001d78c`/`0x0001d798`/`0x0001d7a0`, written `0x0001d7a7`/`0x0001d7ad`/`0x0001d7b6`, cleared `0x0001d77c`–`0x0001d782` |
| `+0x34` | 1 iff **both** feature ids differ from `0xffff`, else 0 | written `0x0001d6b0`, tested `0x0001d897` |
| `+0x38`/`+0x3c`/`+0x40` | the contact stream array: capacity, count, data | `0x0001d7b3`, `0x0001d7b9`–`0x0001d7ee` |

### How this object is built and reset — and a correction that over-corrected

**`0x0005b620` *is* this object's reset.** `phys_fn_002356` at `0x0005b680`
(Phase 7) is the constructor: it builds the stream sub-object at `+0x28` through
`0x000b4d70` and then calls `phys_fn_002354` at `0x0005b620` (Phase 2) at
`0x0005b68d` to reset it. `0x0005b620` operates on `sink + 0x10`, so every
offset inside those two functions is `0x10` lower than the ones in the table
above — their `+0x28` array is this table's `+0x38` stream.

**Measured, not inferred.** Calling the pinned oracle's `0x0005b620` on
`&sink + 0x10` over a `0xcd`-poisoned sink:

```
+0x00..+0x0c  cdcdcdcd      the 0x10 prefix, untouched -- orientedTo is at +0x08
+0x10..+0x34  00000000      zeroed
+0x38/+0x3c/+0x40  the stream array header, preserved, count set to 1
+0x44         cdcdcdcd      past the sub-object
stream[0] = 0                one word reserved, its index recorded at +0x14
```

That is field for field what `nxResetWorld` in the harness produces, including
the reserved word — which the harness had open-coded with a comment saying it
was improvising.

**This corrects a correction.** An earlier version of this section said
`0x0005b620` was *not* this object's initialiser, on the strength of a reference
scan that found two of its three call sites: `0x0001e91b` and `0x0001d0b3` pass
`this + 0x10`, and `0x0005b68d` — the one the scan missed — passes `this`
directly. The callers corroborate it: `0x0001efa3` does
`lea ecx,[esi+0x10]; call 0x0005b680` and `0x0001fd00` does `lea ecx,[esi+0x14]`
into the `add ecx,0x10` thunk at `0x0001d0b0`.

Two things survive the correction and one is now explained:

- **`+0x28`/`+0x2c`/`+0x30` is still the cached normal**, and that conclusion never
  depended on the false premise: it is established independently by the
  emitter's own compares at `0x0001d78c`/`0x0001d798`/`0x0001d7a0` and copies at
  `0x0001d7a7`/`0x0001d7ad`/`0x0001d7b6`. Only the *reason* given for it — "the
  array reading came from `0x0005b620`'s other object" — was wrong. The right
  reason is that `0x0005b620`'s array is at `sink+0x38`, not `sink+0x28`.
- **The `0x44` minimum survives and is now explained** as a `0x10` prefix plus a
  `0x34` sub-object. `0x48` is still not established by anything here.
- **`orientedTo` at `+0x08` surviving a reset is itself a recovered fact**: it
  lives in the prefix, so it is set by whatever owns the sink and persists
  across the per-step reset.

### What the emitter reads, and what it does with each — the reconciliation list

Phase 5 and Phase 7 will need to reconcile against exactly these.

```
param_1 = shape1->[0x9c]
ebx = param_1->[8]      the shape          0x0001d618   <- faults if +0x9c is not a pointer
edx = ebx->[4]          the shape's owner  0x0001d61b
eax = edx->[8]          owner->[8]         0x0001d61e
```

1. **`owner->[8]` versus `sink->[8]`** (`cmp` at `0x0001d62c`, `je` at
   `0x0001d630`). On inequality the emitter takes the block at
   `0x0001d632`–`0x0001d66e`, which **swaps the two shapes**, **swaps the two
   feature ids** (`0x0001d63c`–`0x0001d64a`) and **negates all three components
   of the normal** into a local, repointing the normal argument at that local
   (`0x0001d648`, `0x0001d654`, `0x0001d65d`, each `fld`/`fchs`/`fstp`, with the
   repoint at `0x0001d666`). *That is the normal-orientation rule, recovered:
   the contact normal is stored relative to whichever body the sink is oriented
   to, and is negated when the emitting pair is the other way round.*
2. **`shape1->[0x9c]` versus `sink->[0x20]` and `shape0->[0x9c]` versus
   `sink->[0x24]`** (`0x0001d67d`, `0x0001d688`, `je` to `0x0001d789` at
   `0x0001d68e`). If both match, the header is skipped and only a record is
   appended. If either differs, a new header is written and the two `+0x20`/
   `+0x24` fields are updated. *That is the contact-ordering rule: contacts are
   grouped per shape pair and a header marks each change of pair.*
3. **The feature ids** decide `sink->[0x34]`: 1 iff neither is `0xffff`
   (`0x0001d694`–`0x0001d6b0`).

4. **`owner->[8]` is dereferenced as well as compared**, which corrects an
   earlier version of this section that said it was only compared. On the header
   path the emitter takes `shape1->[4]->[8]` and, if it is non-null, reads
   `+0x240` from it; otherwise it takes `shape0->[4]->[8]` and reads the same
   offset (`0x0001d726`–`0x0001d73e`). That value becomes the high byte of the
   header's third word. So `owner->[8]` is a real object with at least a
   `+0x240` field, and a null on shape1's side is a supported state that falls
   back to shape0's.

| offset | what | established at |
| --- | --- | --- |
| `owner+0x08` | an object; compared against `sink->[8]`, and on the header path dereferenced | compared `0x0001d62c`, dereferenced `0x0001d729`/`0x0001d73b` |
| `(owner+0x08)+0x240` | the material identifier packed into the header | `0x0001d730`, `0x0001d73e` |

What this recovery still does not establish is the *type* of `owner->[8]` or
what else lives in it — only that it has a `+0x240` the emitter reads and an
identity the sink compares. That is left unnamed rather than guessed, and Phase 5
owns the answer.

### The claim that nine kernels are drivable today is false, measured

An earlier draft of this file said a 0x48-byte sink initialised by `0x0005b620`
was enough. It is not, and the measurement is unambiguous — the emitter
dereferences `shape->[0x9c]` **before it writes anything**:

```
sink zeroed                       FAULT c0000005 at rva 0x0001d618 operand 00000008
sink initialised by 0x0005b620    FAULT c0000005 at rva 0x0001d618 operand 00000008
+0x9c -> a zeroed object          FAULT c0000005 at rva 0x0001d61b operand 00000004
+0x9c -> [8] -> [4] -> [8]        COMPLETED, 8 words appended
```

`0x0001d618` is `mov ebx,[eax+8]` and `0x0001d61b` is `mov edx,[ebx+4]`. So a
contact generator needs a **three-level object graph** hanging off `shape->[0x9c]`
before it will emit, and the emitter reads more than it dereferences: it compares
`shape->[0x9c]->[8]->[0x9c]` against `sink->[0x20]` and `sink->[0x24]`, and
`shape->[0x9c]->[8]->[4]->[8]` against `sink->[8]`, to decide whether the stream
header has to be rewritten. Those objects are Phase 5 and Phase 7 layouts.

**That is why the borrowed-layout section above exists.** The graph is now
recovered rather than invented — every offset in it has the address that
establishes it — so a differential built on it runs the oracle through the
oracle's own structure. The one field the recovery does not name is
`owner->[8]`, and it does not need to be named: the emitter only compares it.

So contact ordering and normal orientation are no longer open questions of
format; both rules are written down above, recovered from the branch at
`0x0001d630` and the compares at `0x0001d67d`/`0x0001d688`. What remains for
them is the reconstruction work itself.

### What was recovered about filtering, for whoever owns `phys_fn_000529`

Not reconstructed, because the row is Phase 7's, but recovered while tracing the
callers of the dispatcher and worth not having to find twice:

1. If either shape has bit 4 of its flag byte at `+0xde` set, the pair is
   rejected outright.
2. Otherwise, if **both** shapes have a collision group at `+0xd8` other than
   `0xffff`, the pair is rejected unless `groupMask[group0] & (1 << group1)`.
   `groupMask` is a 32-entry array of `NxU32` at `0x00123a98`, initialised to
   `0xffffffff` by the SDK constructor at `0x0000e753`. A group of `0xffff` on
   *either* shape skips the group test entirely.
3. Otherwise a per-pair record is looked up by the two shapes' owners at `+0xd4`
   through `0x0009a570`. No record means accept; a record means accept iff bit 0
   of its second dword is clear.

The three broad-phase sweeps that call it are `phys_fn_001793` at `0x0003f8b0`,
`phys_fn_001799` at `0x0003fa80` and `phys_fn_001801` at `0x0003fc20` — all
Phase 3 rows, all unreconstructed. They compare world AABBs at `+0x0c/+0x10/
+0x14` against `+0x00/+0x04/+0x08` of a bounds record with six `fcomp`s that all
leave the unordered case on the continuing branch, so a NaN bound passes the
broad phase.

## A caveat on one row, stated rather than assumed

`phys_fn_001921` calls Foundation's `NxComputeSquareDistance` through the IAT
slot at `0x00104170`. This harness links the reconstruction, which links
`NxFoundation`, so the oracle's import binds to the **rebuilt** Foundation and
not the shipped one. The harness prints which module answered:

```
binding NxComputeSquareDistance path=D:\github\Novodex\build\Release\NxFoundation.dll sha256=cccba0be...
```

Both sides of that one entry therefore share a callee, and a defect inside it
would cancel. It is a Phase 2 Foundation row closed by the Foundation
differential, so the shared callee is evidenced elsewhere — but it is a
common-mode dependency in this differential and the transcript says so rather
than leaving it to be assumed.

## What remains

**381 of the 410 rows.** Nine closed by the overlap work, `phys_fn_000873` and
`phys_fn_001901` by the emitter work, `phys_fn_001891` and `phys_fn_001261` by
`plane/capsule`, `phys_fn_001923` and `phys_fn_001377` by `sphere/capsule`,
`phys_fn_001775` and `phys_fn_001010` by `capsule/capsule`, `phys_fn_001883` by
`plane/box`, and `phys_fn_001933`, `phys_fn_001919`, `phys_fn_001917`,
`phys_fn_001281` and `phys_fn_002266` by the two sphere entries, and
`phys_fn_001739` by the `box/box` leaf, `phys_fn_001741` with its
`phys_fn_001743` continuation by the `box/box` clipping row, and
`phys_fn_001745` by the `box/box` separating-axis search, and `phys_fn_001748`
and `phys_fn_001749` by the shim and the entry that complete it. The two large blocks are the shape classes at
`0x000219d0..0x0002c8eb` (256 rows, 41,842 bytes: `Shape.cpp`, the five
`Np*Shape.cpp`, `CapsuleShape.cpp`, `SphereShape.cpp`, `ConvexHull.cpp`) and the
collision code at `0x000360e0..0x0004c016` (127 rows, 87,401 bytes, which
includes the 27 closed exports and the five `Contact*.cpp` units).

The tractable next steps, in the order their dependencies allow:

1. **The rest of matrix B's primitive half.** `phys_fn_001751` (box/capsule)
   needs three Phase 4 rows at `0x00032840`, `0x00033a50` and `0x00033d00`;
   `phys_fn_001774` (capsule/capsule) needs `phys_fn_001690` at `0x00033e80`, a
   1,836-byte Phase 2 segment/segment distance. Both are single, well-bounded
   dependencies and both are drivable by the instrument that now exists.
2. **The compound entries** `phys_fn_001787`, `phys_fn_001789`,
   `phys_fn_001791`, `phys_fn_001785` and `phys_fn_001797`, which need a
   compound shape's child list at `+0xe0/+0xe4` — a layout question, not a new
   subsystem. `phys_fn_001787` is the cheapest row in the whole matrix: it
   always returns false.
3. **`phys_fn_002264` and the three rows behind it — 3,802 bytes — need vtable
   slot 7 resolved and two layout questions answered before anyone starts.**
   It is the continuous-collision sweep, reached from both sphere entries of
   matrix A when one shape is static and `NX_CONTINUOUS_CD` is not 0.0f. Its
   guard is closed under the SDK's own defaults, which is why those two entries
   close without it, and the harness measures that rather than assuming it:
   `continuous_cd=00000000`, and 58 probes that the guard reads no other
   parameter. Picking it up means resolving slot 7 per receiver type, settling
   `Shape+0x3c`..`+0x68` and settling the sink past `+0x44` — and it also brings
   in a 1,027-byte **Phase 7** row, `phys_fn_000754`, which nothing else reaches.
4. **`box/box`, and 5,286 bytes of it remain.** The leaf `phys_fn_001739` and
   the clipping row `phys_fn_001741` with its `phys_fn_001743` continuation are
   both closed above, both bit-exact under both control words, so the divergence
   this table budgeted for did not have to be pinned. What is left is
   `phys_fn_001745` (4,271 bytes, the separating-axis search, forecast
   bit-exact -- all eleven of its depth-1 branches carry bare 32-bit loads),
   `phys_fn_001748` (240, a transpose-and-copy shim with no arithmetic) and the
   entry `phys_fn_001749` (775, whose stream half the shared append helpers
   already cover). Two things the closed row adds for them: `phys_fn_001745`'s
   tail call must set `edx` to the incident box's extents on every one of its
   six arms, and the entry's differential still needs the `sink+0xe8` warm start
   driven across two *different* box pairs in one sink.
5. **Matrix A's remaining non-mesh entries.** The emit convention and the stream
   record are recovered above, so the record format is not the blocker. The
   blocker is one level of object layout: the emitter faults on `shape->[0x9c]`
   before it writes anything and needs a three-level graph whose fields it
   compares against the sink. Settle what `+0x9c` points at from the Phase 5
   side first — inventing it would put the implementer on both sides of the
   differential. The two emitters `phys_fn_000873` (`0x0001d610`) and
   `phys_fn_000875` (`0x0001d8e0`) are Phase 3 rows and belong in the same
   component as the entries.

   **A correction, because I propagated an alarming version of this.**
   `phys_fn_001883` is *not* a census understatement. `0x00047f20..0x0004826f`
   is contiguous with no overlap, the two interior pads are classified
   `compiler_artifact`, and both continuations carry
   `notes: "continuation of the entry at 0x00047f20"`. 42 is one chunk's size,
   not a wrong extent. The correct statement is that **the census splits a
   function at its internal alignment padding and annotates the continuations**,
   which a reader has to know to sum them — and which is a much smaller thing
   than the inventory being wrong.
6. **The broad phase** and **filtering** need a scene, and are Phase 7's
   dependencies as much as this one's.

Two things that are not row work and should not wait:

- **`NxBuildSmoothNormals` under the simulation control word**, above. It is the
  first evidence that a closed row can be closed under one machine model and not
  another, and the question applies to every kernel Task 2 closed.
- **The fuzz harness's non-finite mixture.** `nxPick(r, mode)` takes `mode` from
  the loop index and the vector block walks fifteen consecutive modes, so only
  one or two draws in fifteen take the raw-bit branch — and `case 6` builds an
  exponent in `110..144`, which never reaches 255, so it cannot produce a NaN at
  all. Over 60,000 iterations **zero** calls received two non-finite inputs. The
  x87 NaN payload rule therefore rests on a single witness, and any future "this
  file does not need the flag" conclusion drawn from that harness would be drawn
  from a domain it does not reach — which is exactly how `SmoothNormals.cpp` was
  wrongly cleared once already. `NxPhysicsCollisionTests` has a dedicated
  non-finite branch and does not share this defect; the fuzz harness has not been
  changed here because doing so re-pins every one of its digests and that is a
  deliberate act, not a side effect of this task.

`run_phase_gate.ps1 -Phase 3` reports `status=pass` and now evaluates 95
coverage assertions — 18 for the fuzz harness and 77 oracle-side — against a
floor of 95. It is still not a statement that Phase 3 is done: it sees 25 of the
410 remaining Phase 3 rows, where before it saw none of them, and one Phase 2
row it has discharged on Phase 2's behalf. Three of those twenty-five and the
Phase 2 row are closed on algorithmic agreement under a control word they do not
execute under, with their real-context divergences pinned at 662, 242 and 43 —
`NxBuildSmoothNormals`'s 24 went to zero when its carrier was found; the rest —
`plane/box`, the five sphere rows and both `box/box` geometry rows included —
are bit-exact under both.
