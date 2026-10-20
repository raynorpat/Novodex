# The P-Hull class's builder, initialiser and clean-up

Rows `phys_fn_001443`, `001447`, `001451`, `001453`, `001467`, `001469`, `001471`, `001474`, `001476` to
`001492` (one function) and `001494`, 18 rows in ten functions, 2,120 instructions
(`Physics/src/HullBuild.cpp`).

## What the rows are

The class is `ConvexHull`, vtable `0x1010769c`: slot 0 is the scalar deleting destructor (`001474`), slot 1 the
point-cloud builder (`001476` to `001492`) and slot 2 the from-faces initialiser (`001494`). The rows already
reconstructed (`001455`, `001459`, `001461`, `001463`, `001465`, `001472`, `001496` and the rest of
`ConvexHull.cpp`) are its constructor and its polygon, centroid, support and edge methods.

* `001494`, the initialiser (thiscall, one create block `{vertex count, vertices, face count, dword faces, 16-bit
  faces}`): copies the vertices and the faces into SDK-allocated arrays, runs the clean-up (`001451` drop
  duplicate faces, `001467` fix the winding, `001453` collapse tiny edges and drop faces left with a repeated
  index), validates with `001469`, and builds the polygons (`001472`) and the centroid (`001459`).
* `001476` to `001492`, the builder: a single 2.9 KB function that the listing splits into nine rows at its
  loop headers (the rows end and begin inside loops). It takes a point cloud (create block `{count, vertices,
  unused, orient byte, polygons byte}`), copies the vertices into doubles after `ReducedVertices` has removed
  the duplicates, builds a hull from an initial simplex by repeated horizon expansion, and finishes the same
  way the initialiser does. It reports failure unless the polygons were asked for (create `+0xd`).
* `001443` and `001447` are the two triangle tests under the clean-up (the unnormalised normal of a 16-bit
  indexed triangle; whether a point lies on or in front of it), `001469` the validating pass over the three
  clean-up helpers, `001471` the release of every array the hull owns.

No reconstructed row calls any of these yet.

## How they were written

Naked listing transcriptions, generated from the pinned image. Calls reach the SDK allocator getter, the
vendored `Container` destructor and the CRT's `operator delete`, `__alloca_probe` and vector constructor
iterator, `ReducedVertices` (`IceMeshTools.cpp`), and the candidate's own `nxHullComputePolygons` and
`nxHullComputeCentroid`; the class's vtable is a three-slot array in the file. Read-only doubles and floats
are named data. `evidence/phase3-hull-build-listing-compare.py` compares the built candidate with the oracle
instruction by instruction: 2,120 instructions, zero differing, and the 71 call mappings reach the candidate
functions they should.

## How they are gated

`NxPhysicsThirdPartyTests` (Phases 4 and 7), `nxDriveHullBuild`. Each row is entered with the oracle's
registers by a trampoline, once for the oracle's address and once for the candidate's, over the same inputs;
each side's arrays come from its own SDK allocator, and the hull object it leaves is taped field by field
(counts, faces, vertices, centroid, polygon counts and references; the polygon planes, which belong to rows
`001463` and `001472`, are left to their own divergent family).

* `hull_init` (12,330 words): the box, two latitude/longitude spheres, a bumpy grid, a flat grid and a single
  triangle, each plain and with every face wound the other way, every third face wound the other way, duplicate
  faces, faces with a repeated vertex and a zero-area face, with the faces as dword triples and as 16-bit
  triples; 30 of 48 accepted. Each is released afterwards.
* `hull_build` (14,411 words): the box, the spheres, a flat grid, fourteen random clouds of 10 to 127 points
  stretched along different axes, a box with interior points and two spheres with repeated and near-repeated
  vertices, with each of the orient and polygons bytes; 20 of 63 accepted (those that ask for the polygons).
* `hull_delete` (44 words): the scalar deleting destructor with the flag clear and set, on an empty and a
  built hull.

All three are exact with zero mismatches.

## Falsification

Each mutation is a one-line edit of the listing found by its address comment, rebuilt and run against the
pinned oracle, the file restored between runs. A mutant that hangs the process is stopped by a 60-second limit.
About 150 mutants were run (the clean-up helpers, the release, the initialiser and the delete with eight each
or all; the builder with 55 spread over its nine rows); the count shown is the best mismatch count among the
families, and `1` means the tape differs from the first word on (the oracle and the candidate then disagree
on whether the call succeeded).

| row | mutation | result |
|---|---|---|
| 001443 | `je` of the null-vertices test inverted (0x0002a6ea) | hull_delete 6 |
| 001447 | `fsub` to `fadd` (0x0002a85f) | hull_delete 6 |
| 001451 | `jbe` inverted (0x0002aa2c) | hull_init 388 |
| 001453 | `je` of the null-arguments test inverted (0x0002aab0) | hull_delete 6 |
| 001467 | `fmul` to `fdiv` in the centroid sum (0x0002b483) | hull_init 384 |
| 001469 | `jne` after the first pass inverted (0x0002b595) | hull_delete 6 |
| 001471 | `je` before the edge-axes release inverted (0x0002b6d7) | hull_init 48 |
| 001474 | returned pointer zeroed (0x0002b9ac) | hull_delete 4 |
| 001476 | `jle` inverted (0x0002bab9) | hull_build 1 |
| 001478 | `fsub` to `fadd` (0x0002bb45) | hull_build 1 |
| 001480 | `jge` inverted (0x0002bbd6) | hull_build 6,627 |
| 001482 | `fadd` to `fsub` (0x0002bcbf) | hull_build 1 |
| 001484 | `jl` inverted (0x0002bef5) | hull_build 1 |
| 001486 | `fadd` to `fsub` (0x0002c0b5) | hull_build 10,445 |
| 001488 | `fmul` to `fdiv` (0x0002c20c) | hull_build 1 |
| 001490 | `jne` inverted (0x0002c34f) | hull_build 1 |
| 001492 | `je` inverted (0x0002c6b9) | hull_build 1 |
| 001494 | `je` inverted (0x0002c8c9) | hull_init 30 |

Many more mutants of every row end in an access violation or a hang (a hull whose faces are wrong is walked
off the end of its arrays), which also rejects them.

Not rejected, and why:

* The release's free-if-set branches (0x0002b5f7, 0x0002b60f, 0x0002b65b) and the builder's matching ones
  (0x0002c35b, 0x0002c38a, 0x0002c3b3, 0x0002c3c8) when inverted: they call `free(0)` or skip a free, which
  nothing observable reflects; the pointer-clearing stores beside them (0x0002c36f) likewise.
* The builder's NaN branches (`jp` at 0x0002bb53) and two comparison edits in the extent scan (`fadd` to `fsub`
  at 0x0002bbd8 and 0x0002bc12, `jle` at 0x0002c3f9): no cloud in the harness has a NaN or a tie there.

## What this does not close

* Polygons requested without the orientation pass crash both the oracle and the candidate at the same place
  (a callee of `001472`); that combination is not driven.
* The vtable has no reconstructed caller, so slot 1's and slot 2's argument blocks are the harness's reading
  of the listing (the three create fields the code reads), not a recovered public structure.
* The point clouds are the harness's; no cooked mesh is routed through them.
