# Phase 3 exports and the geometry differential matrix

Phase 3 Task 1. Tests, cases and evidence only: no production row is implemented
and no Phase 3 row changes state. The deliverable is a differential that is GREEN
against the shipped oracle and RED against the current candidate.

## What Phase 3 owns

Recomputed from `inventory.json` rather than quoted from the plan:

| quantity | value |
| --- | --- |
| function rows | 441 |
| data objects | 102 |
| function bytes | 145,683 |
| data bytes | 4,227 |
| row state | all 441 `discovered` |
| `.text` span | `0x00016c00` .. `0x000b5570` |

Phase provenance: 158 `enclosed_by_one_phase`, 110 `translation_unit`, 90
`callers`, 52 `layout_adjacency`, 31 `export_pin`. Named source files, 78 rows in
total: `NpCapsuleShape.cpp` 14, `NpBoxShape.cpp` 12, `NpPlaneShape.cpp` 12,
`NpSphereShape.cpp` 12, `NpTriangleMeshShape.cpp` 11, `Shape.cpp` 7,
`CapsuleShape.cpp` 2, `SphereShape.cpp` 2, `ContactConvexHeightfield.cpp` 2,
`ContactPlaneMesh.cpp` 2, `ConvexHull.cpp` 1, `ContactMeshMesh.cpp` 1.

The 441 stable IDs are not contiguous: they fall in 426 runs, no run longer than
three IDs, so an enumeration here would be a copy of the inventory rather than a
summary of it. They are addressed by query:

```
python -c "import json;i=json.load(open('inventory.json'));print([f['id'] for f in i['functions'] if f['phase']==3])"
```

The 102 data objects fall in 16 runs, the largest `phys_data_000905`..`000926`.

## The 27 named exports, reconciled

Every one of the 41 named exports of the shipped DLL was resolved to its owning
phase through `exports[].function_id` and that function's `phase`. Phase 3 owns
27 of them, which is exactly the set the plan names, with no additions and no
omissions. Ordinals 15 and 35 (`NxCreatePMap`, `NxReleasePMap`) belong to Phase 4,
16..25 to Phase 2, and 26..27 to Phase 6.

| ordinal | export | stable id | rva | size | cases |
| ---: | --- | --- | --- | ---: | ---: |
| 1 | `NxBoxBoxIntersect` | `phys_fn_001702` | `0x00036690` | 1308 | 12 |
| 2 | `NxBuildSmoothNormals` | `phys_fn_002146` | `0x000533c0` | 1096 | 17 |
| 3 | `NxComputeBoxDensity` | `phys_fn_000809` | `0x0001bb00` | 44 | 9 |
| 4 | `NxComputeBoxInertiaTensor` | `phys_fn_000823` | `0x0001bc30` | 75 | 8 |
| 5 | `NxComputeBoxMass` | `phys_fn_000807` | `0x0001bad0` | 44 | 9 |
| 6 | `NxComputeConeDensity` | `phys_fn_000821` | `0x0001bc10` | 25 | 9 |
| 7 | `NxComputeConeMass` | `phys_fn_000819` | `0x0001bbf0` | 25 | 9 |
| 8 | `NxComputeCylinderDensity` | `phys_fn_000817` | `0x0001bbd0` | 25 | 9 |
| 9 | `NxComputeCylinderMass` | `phys_fn_000815` | `0x0001bbb0` | 25 | 9 |
| 10 | `NxComputeEllipsoidDensity` | `phys_fn_000813` | `0x0001bb70` | 50 | 9 |
| 11 | `NxComputeEllipsoidMass` | `phys_fn_000811` | `0x0001bb30` | 50 | 9 |
| 12 | `NxComputeSphereDensity` | `phys_fn_000805` | `0x0001bab0` | 23 | 9 |
| 13 | `NxComputeSphereInertiaTensor` | `phys_fn_000825` | `0x0001bc80` | 62 | 10 |
| 14 | `NxComputeSphereMass` | `phys_fn_000803` | `0x0001ba90` | 23 | 9 |
| 28 | `NxRayAABBIntersect` | `phys_fn_001722` | `0x00037c80` | 317 | 13 |
| 29 | `NxRayAABBIntersect2` | `phys_fn_001726` | `0x00037e70` | 317 | 13 |
| 30 | `NxRayCapsuleIntersect` | `phys_fn_001734` | `0x000381c0` | 1605 | 13 |
| 31 | `NxRayOBBIntersect` | `phys_fn_001718` | `0x00037820` | 645 | 13 |
| 32 | `NxRayPlaneIntersect` | `phys_fn_001704` | `0x00036bb0` | 162 | 11 |
| 33 | `NxRaySphereIntersect` | `phys_fn_001710` | `0x00036e80` | 198 | 13 |
| 34 | `NxRayTriIntersect` | `phys_fn_001712` | `0x00036f50` | 782 | 15 |
| 36 | `NxSegmentAABBIntersect` | `phys_fn_001720` | `0x00037ab0` | 449 | 12 |
| 37 | `NxSegmentBoxIntersect` | `phys_fn_001714` | `0x00037260` | 737 | 13 |
| 38 | `NxSegmentOBBIntersect` | `phys_fn_001716` | `0x00037550` | 705 | 13 |
| 39 | `NxSegmentPlaneIntersect` | `phys_fn_001706` | `0x00036c60` | 291 | 10 |
| 40 | `NxSeparatingAxis` | `phys_fn_001696` | `0x000360e0` | 269 | 12 |
| 41 | `NxSweptSpheresIntersect` | `phys_fn_001736` | `0x00038810` | 434 | 11 |

`test_geometry_cases.py::test_every_phase_three_export_is_covered` recomputes
this set from `inventory.json` on every run, so the matrix cannot fall behind a
re-phasing of an export.

The plan's export list and the inventory agree in full. One wording note: the
plan says "all `NxCompute*Density`/`Mass`/`InertiaTensor`", and the shipped DLL
exports exactly ten of the first two and two of the third; there is no
`NxComputeCapsule*` or `NxComputeCylinderInertiaTensor` in the export table even
though `NxInertiaTensor.h` groups cylinders and cones with the rest.

## The harness

`D:\github\Novodex\tests\PhysicsGeometryTests.cpp`, target
`NxPhysicsGeometryTests`, registered for Phase 3 in `tools/gate_targets.ps1`.
It loads the pair through the same `PhysicsPairLoader.h` the Phase 2 targets use,
resolves the 27 exports by name, runs 299 cases and prints, per case, the input
words it passed, the return value and every output byte.

Five decisions are load-bearing:

- **Inputs are IEEE-754 bit patterns, not decimal literals.** The comparison is
  bit-exact and decimal does not round-trip. Every float input is a `NxU32` in the
  case table, copied into the argument storage by `memcpy`, so it never passes
  through a floating-point register except where the signature says the value is
  passed by value.
- **Selector and bool columns are integers, not float bit patterns.** See the
  defects below: this is not a stylistic point.
- **Arguments are passed as raw `const float*`, not as `NxVec3`/`NxMat33`.** The
  harness owns the bytes it hands to the DLL, so nothing about the transcript
  depends on the header classes' inline semantics.
- **Every output buffer is poisoned with `0xcdcd0000+i` before the call and the
  two words past the declared end of it are printed with it.** "Wrote nothing",
  "wrote the wrong value" and "wrote past the end" are three different
  transcripts rather than one.
- **A `bool` return is read and printed as the raw byte the ABI leaves in AL.**
  The function pointers declare `unsigned char`, so a return byte that is neither
  0 nor 1 would show as itself rather than collapsing to `00000001`. The shipped
  DLL returns 0 or 1 on all 143 bool cases, so nothing changes today; the point is
  that "compared as bits" has no exception in it.

Output is unbuffered, so a case that faults names itself: the last line printed
is the last case that did not. A fault cannot be gated by a differential, so a
case that faults the oracle has to be found and removed. None of the 299 does;
the oracle exits 0.

## The case matrix

299 cases over 27 exports. Every dimension the brief asks for is present, and
`test_dimensions_come_from_the_vocabulary_and_are_all_exercised` fails if any of
them stops being exercised or if a case grows a tag outside the vocabulary.

| dimension | cases | what it covers |
| --- | ---: | --- |
| `degenerate` | 55 | zero extents, zero radius, zero-length segments, inverted and zero-size boxes, repeated and collinear triangle vertices, zero and non-orthonormal rotation matrices |
| `hit` | 34 | the intersecting configuration, including from inside |
| `non_finite` | 33 | quiet NaN and infinity in each argument position |
| `nominal` | 35 | the ordinary in-domain value, and an asymmetric one where the shape has three extents |
| `miss` | 29 | non-intersecting, too short, beside, parallel |
| `undocumented_domain` | 20 | negative radii, negative extents, negative mass |
| `tangent` | 18 | exact touching: grazing a face or a corner, origin on the surface, a triangle edge or vertex hit, faces exactly in contact |
| `zero_length` | 15 | zero direction vectors, zero-length segments inside and outside, both velocities zero |
| `non_normalized` | 13 | doubled direction vectors, unnormalised plane normals, scaled rotation matrices |
| `overflow` | 12 | `FLT_MAX` inputs whose product leaves the range |
| `underflow` | 10 | the smallest positive denormal |
| `aliasing` | 9 | the output pointer aimed inside the input |
| `undocumented_bool_encoding` | 5 | a `bool` byte of 2, which is neither 0 nor 1 |
| `reversed_direction` | 4 | the direction or the endpoint order flipped |
| `rotated` | 4 | box-box with a 45-degree rotation, with `fullTest` both on and off |
| `null_optional_input` | 2 | `NxBuildSmoothNormals` with both index arrays null, at one triangle and at two |
| `null_optional_output` | 1 | `NxRaySphereIntersect`'s optional `coord` |

Eleven of the 27 exports have outputs. Nine of those eleven carry an aliasing or
null-output case; the two exceptions are `NxComputeBoxInertiaTensor` and
`NxComputeSphereInertiaTensor`, whose only output is a write-only vector with
nothing in the argument list to alias it with.
`test_every_output_carrying_export_has_an_aliasing_or_null_case` names those two
exceptions explicitly, so a third one cannot appear quietly.

## Evidence that the cases can fail

A case that a wrong implementation passes is not a case. Three deliberately wrong
implementations of all 27 exports were built and measured, in a `git archive`
copy of `D:\github\Novodex` under the scratchpad. Neither the implementation tree
nor the evidence tree was mutated for this.

- **M1, stub-false**: every export returns zero or false and writes nothing.
- **M2, stub-true**: every export returns one or true and writes a fixed pattern
  (`7.0f` for scalar returns, `(1,2,3)` for vectors). Deliberately not zero: the
  oracle itself returns `+0` for a good many degenerate inputs, so a second
  zero-returning mutant would have left those cases untested. The first cut of M2
  did return zero, and 23 cases were consequently rejected by no mutant at all.
- **M3, textbook**: plausible implementations from the documented semantics —
  Moller-Trumbore, the slab method, the quadratic sweep, SAT over 6 or 15 axes.

Controls, both in the same run as the measurement:

```
negative control  oracle vs oracle : differing cases = 0 / 299
positive control  oracle vs M1     : differing cases = 230 / 299
positive control  oracle vs M2     : differing cases = 255 / 299
positive control  oracle vs M3     : differing cases = 72 / 299
cases no mutant changed: 0
```

The negative control rules out a transcript that differs from run to run; the
three positive controls rule out an instrument that reports zero because it is
broken. The input words were compared across all five runs and are identical, so
the same table drove every one of them.

| export | cases | M1 stub-false | M2 stub-true | M3 textbook | rejected by at least one |
| --- | ---: | ---: | ---: | ---: | ---: |
| `NxComputeSphereMass` | 9 | 6 | 9 | 0 | 9 |
| `NxComputeSphereDensity` | 9 | 7 | 9 | 0 | 9 |
| `NxComputeBoxMass` | 9 | 8 | 9 | 1 | 9 |
| `NxComputeBoxDensity` | 9 | 7 | 9 | 1 | 9 |
| `NxComputeEllipsoidMass` | 9 | 8 | 9 | 1 | 9 |
| `NxComputeEllipsoidDensity` | 9 | 7 | 9 | 1 | 9 |
| `NxComputeCylinderMass` | 9 | 5 | 9 | 3 | 9 |
| `NxComputeCylinderDensity` | 9 | 7 | 9 | 3 | 9 |
| `NxComputeConeMass` | 9 | 5 | 9 | 0 | 9 |
| `NxComputeConeDensity` | 9 | 7 | 9 | 0 | 9 |
| `NxComputeBoxInertiaTensor` | 8 | 8 | 8 | 1 | 8 |
| `NxComputeSphereInertiaTensor` | 10 | 10 | 10 | 0 | 10 |
| `NxRayPlaneIntersect` | 11 | 8 | 11 | 5 | 11 |
| `NxSegmentPlaneIntersect` | 10 | 10 | 10 | 5 | 10 |
| `NxRaySphereIntersect` | 13 | 11 | 12 | 3 | 13 |
| `NxRayTriIntersect` | 15 | 8 | 15 | 2 | 15 |
| `NxRayAABBIntersect` | 13 | 13 | 13 | 6 | 13 |
| `NxRayAABBIntersect2` | 13 | 12 | 13 | 7 | 13 |
| `NxSegmentBoxIntersect` | 13 | 8 | 13 | 1 | 13 |
| `NxSegmentAABBIntersect` | 12 | 7 | 5 | 0 | 12 |
| `NxRayOBBIntersect` | 13 | 9 | 4 | 0 | 13 |
| `NxSegmentOBBIntersect` | 13 | 9 | 4 | 0 | 13 |
| `NxRayCapsuleIntersect` | 13 | 9 | 13 | 4 | 13 |
| `NxSweptSpheresIntersect` | 11 | 5 | 6 | 2 | 11 |
| `NxBoxBoxIntersect` | 12 | 10 | 2 | 1 | 12 |
| `NxSeparatingAxis` | 12 | 11 | 9 | 10 | 12 |
| `NxBuildSmoothNormals` | 17 | 15 | 17 | 15 | 17 |
| **total** | **299** | 230 | 255 | 72 | **299** |

Every one of the 299 cases rejects at least one wrong implementation. The M3
column is the interesting one for Phase 3 Task 2: a zero there says a textbook
implementation already reproduces the shipped result bit-for-bit on this matrix,
and a high number says the shipped algorithm differs from the textbook one in a
way the matrix sees.

### Where the matrix is blind

Eight exports have a zero M3 column: `NxComputeSphereMass`,
`NxComputeSphereDensity`, `NxComputeConeMass`, `NxComputeConeDensity`,
`NxComputeSphereInertiaTensor`, `NxRayOBBIntersect`, `NxSegmentOBBIntersect`,
`NxSegmentAABBIntersect`. That is **84 of the 299 cases with no discriminating
power against a plausible implementation.** A second, independently written
textbook implementation agrees on the same eight, so this is a property of the
matrix and not of one author's M3. For those eight the disassembly is the sole
authority in Task 2, and the differential will confirm rather than derive.

The converse is also worth carrying: M3's Moller-Trumbore matched the oracle
bit-for-bit on all 15 `NxRayTriIntersect` cases, including `t`, `u`, `v` and the
aliased write, and an independent implementation reproduced that. The shipped
function is very likely reference Moller-Trumbore with the same operation order.

## Three defects this task found in its own instrument

Recorded because each of them produced a transcript that looked plausible.

1. **Two output buffers were never gathered.** `NxComputeBoxInertiaTensor` and
   `NxComputeSphereInertiaTensor` printed an uninitialised local array. It read as
   a credible-looking result and only gave itself away by changing between runs.
2. **Selector and bool columns held float bit patterns.** `hollow`, `cull`,
   `fullTest` and every aliasing/null `mode` word was written as `1.0f`
   (`0x3f800000`). Truncated to a byte that is zero, and compared against the
   integer 1 it is not equal to, so every aliasing case, every null-output case,
   every bool-encoding case and every `fullTest=true` case silently tested the
   opposite of what its name said.
3. **`NxRayTriIntersect`'s aliasing case was on the culled side.** It used a
   configuration that returns false before reaching the write, so the aliased
   pointer was never exercised. The winding was then measured rather than assumed
   and every hitting case rebased onto the side the shipped cull keeps.

The first two were found by reading the transcript; the third by the coverage
check. All three are the reason the mutation table above is worth its cost:
before the second fix, 23 cases rejected no mutant.

### Each of the three is blocked by a committed test

The first statement of this section credited the guards for 1 and 3 to
assertions in the authoring generator, which is not a delivered artefact and
cannot be re-run: only defect 2 was actually enforced by anything committed.
Two further tests close that, and each was verified by re-injecting the defect
into **both** `cases/geometry.json` and `cases/geometry-oracle-transcript.txt`,
so the two stay consistent and only a structural check can catch it:

| re-injected into the committed fixtures | test that fails |
| --- | --- |
| defect 1, an ungathered buffer's uninitialised stack words | `test_guard_words_are_a_poison_ladder_or_an_input` |
| defect 2, `hollow` holding `3f800000` | `test_integer_columns_hold_integers` |
| defect 3, the aliasing case falling back to the culled side | `test_aliasing_cases_reach_the_write_and_null_cases_do_not` |
| an aliasing case whose output region is all poison | `test_aliasing_cases_reach_the_write_and_null_cases_do_not` |
| a null-output case whose buffer was written after all | `test_aliasing_cases_reach_the_write_and_null_cases_do_not` |
| `hollow` re-tagged `float32_bits`, which would switch the integer check off | `test_the_harness_declares_the_same_input_layout` |

The guard-word test holds every guard to being either `cdcd00NN` continuing that
segment's ascending poison ladder, or a word present in that case's own inputs,
which is only permitted for an `aliasing` case. Uninitialised stack cannot spell
a poison ladder. The aliasing test looks only at the **declared** output words and
not the guards, because in an aliasing case the guards are input words and would
satisfy a naive "something is non-poison" check for free.

## Oracle GREEN

```
oracle exit=0 cases=299
negative control  oracle vs oracle : differing cases = 0 / 299
```

The full transcript is `cases/geometry-oracle-transcript.txt`, and
`cases/geometry.json` records the same 299 return values and output byte strings
per case with the inputs that produced them.
`tools/tests/test_geometry_cases.py` holds the two together word for word, and
was itself shown to fail: perturbing one recorded return value from `40860a92`
to `00000000` produces
`SUBFAILED(case='NxComputeSphereMass.00') ... test_every_case_matches_the_transcript_word_for_word`.

No case shows the oracle writing past a declared output. Seven cases print a
non-poison guard word, and all seven are aliasing cases where the guard is an
input word by construction.

## Candidate RED

```
phase=3 resolved_targets=NxPhysicsGeometryTests
differential target=NxPhysicsGeometryTests oracle_exit=0 candidate_exit=0 stdout_delta=652 stderr_exact=True
GATE FAILED: requirement not met: every requested target produces identical oracle and candidate results
```

`run_phase_gate.ps1 -Phase 3` exits 1 on the same line. It previously exited 3
with `status=skipped reason=no_registered_test_targets`.

652 is 326 lines on each side: 27 `export name=` lines that read `present=1`
against the oracle and `present=0` against the candidate, 299 case lines that
carry a result on one side and `skipped=export_missing` on the other. The
candidate implements none of the 27, which is correct for this task: it adds no
production code.

`run_phase_gate.ps1 -Phase 2` still reports `phase_gate=2 status=pass`.

## What the matrix pins about `NxBuildSmoothNormals`

This export needed four more cases, because the first cut described its
behaviour in a way an implementer could follow to the wrong construction. Both
corrections below are measurements from the transcript, not readings of the
disassembly.

**The null-index path is not a sequential triangle soup.** The original single
both-null case had `nbTris=1, nbVerts=3`, which is exactly where "consume the
vertex array three at a time" and "use the constant triangle (0,1,2), `nbTris`
times" give the same answer. At `nbTris=2, nbVerts=6, flip=1` they do not:

```
.13 both index arrays null   ret=1  normals[0..2] = (0,0,-1) x3   normals[3..5] = (0,0,0) x3
.14 the same mesh, indexed   ret=1  normals[0..5] = (0,0,+1) x6
```

Vertices 3..5 are never touched, so the second triangle was `(v0,v1,v2)` again,
not `(v3,v4,v5)`; and `flip=1` is honoured on the indexed path and ignored on the
null one. An implementation indexing `3*i, 3*i+1, 3*i+2` would have been wrong for
every `nbTris > 1` and would still have passed the previous 295 cases.

**The weighting is by interior angle, not by face area and not by plain unit
averaging.** Two cases separate all three, each with two triangles sharing
vertex 0:

```
.15 equal angles (90 and 90), areas 0.5 and 8   normals[0] = (0, 3f3504f3, bf3504f3) = (0, +0.70711, -0.70711)
.16 equal areas, angles 90 and 45              normals[0] = (0, 3ee4f92e, bf64f92e) = (0, +0.44721, -0.89443)
```

`.15` rules out area weighting, which would have given roughly
`(0, 0.998, -0.0624)`. `.16` rules out averaging unit face normals, which would
have given `(0, 0.70711, -0.70711)` again; the measured 1:2 ratio is the 45:90
ratio of the interior angles. The symmetric cube in `.04` cannot show either,
because every weighting scheme agrees on it.

Together with the in-place aliasing case `.11`, which comes out as 24 zero words,
these pin enough of the construction that Task 2 has a specification rather than
a description.

## Nothing was excluded from the comparison

`run_differential.ps1`'s `excluded_from_comparison` list is unchanged. Every line
this harness prints is compared. The one environment-dependent line it produces,
`modules pair=2 trusted_system=N rejected=0`, whose count varies with whether the
loader pulled in `apphelp.dll`, was already excluded by the existing `modules `
prefix from Phase 2 and needed no new entry.
