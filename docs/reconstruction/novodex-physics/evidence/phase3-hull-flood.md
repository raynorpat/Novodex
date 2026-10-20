# The hull's triangle clustering

Rows `phys_fn_001423` (RVA `0x00029a30`), `001425` (`0x00029a70`), `001427` + `001429` (`0x00029c80`),
`001431` (`0x0002a0a0`), `001433` (`0x0002a160`), `001435` + `001437` (`0x0002a190`) and the queue pop
`phys_fn_001655` (`0x00032410`), nine rows in seven functions, 1,060 instructions.

## What the rows are

They are one algorithm: the grouping of a triangulated hull's triangles into planar polygons by a flood
fill over the edge adjacency. The object they share (the "clusterer") holds the triangles, the vertices, an
`EdgeList` (the edge topology the edge_list family already compares), two Containers (the triangles and the
vertices of the polygon being grown), a byte per edge (set when the edge is sharp), and a polygon id per
triangle.

* `001435` + `001437` is the pass. For each edge it decides whether it is flat (two faces whose opposite
  vertices lie within 0.01 of each other's plane) or sharp, groups the triangles that share no sharp edge,
  orders the groups with a radix sort, and floods from the first triangle of each group. The listing is one
  function split across two rows: `001437` is the loop tail the first row jumps into.
* `001433` is the entry that starts a flood (false for a start triangle past the bound), `001431` the driver
  that queues the start triangle and runs the flood on each (triangle, vertex) pair it pops (`001655` is the
  pop), and `001427` + `001429` the flood step: claim the triangle for the polygon if `001425` finds it
  coplanar with it, then queue each unclaimed neighbour across a non-sharp edge together with the neighbour's
  vertex opposite the edge. `001429` is the third edge's arm of the same function, a second row because the
  listing's first arm jumps into it.
* `001425` is the coplanarity test: the plane of every triangle already in the polygon (`Plane::Set`), every
  candidate vertex within 0.001 of it, and on success the triangle and its vertices added to the lists.
* `001423` frees the clusterer's two owned buffers.

No reconstructed row calls any of this yet: the caller of `001435` is `phys_fn_002247`, still open.

## How they were written

Naked listing transcriptions (`Physics/src/HullFlood.cpp`), generated from the pinned image. The calls reach
the vendored `Container`, `RadixSort` and `Plane::Set`, the CRT's `operator new`/`delete` and
`__alloca_probe`; the one global the flood bumps (the stamp at `0x10123c74`) is a named variable.
`evidence/phase3-hull-flood-listing-compare.py` compares the built candidate with the oracle instruction by
instruction: 1,060 instructions, zero differing, and the 39 call mappings reach the candidate functions they
should.

## How they are gated

`NxPhysicsThirdPartyTests` (Phases 4 and 7), `nxDriveHullFlood`. Each row is entered with the oracle's
registers by a trampoline, once for the oracle's address and once for the candidate's, over the same meshes
and the same (read-only) edge lists. The containers each side uses are built, grown and freed by that side's
own constructor, `Resize` and destructor, so the storage belongs to its allocator. Twelve meshes: the six
harness meshes, and six more with the triangles cyclically rotated and a quarter (or half) wound the other way
(a mesh whose neighbours disagree on the winding takes every orientation of the flood's shared-edge search).

* `hull_queue_pop` (540 words), `hull_free` (36), `hull_coplanar` (5,824: 360 cases, 323 accepted, containers
  both exactly full and with room, a null list on either side), `hull_flood` (28,136), `hull_flood_start`
  (13,410), `hull_flood_entry` (12,246), `hull_cluster` (5,960: 48 runs, 1,516 polygons).

All seven are exact with zero mismatches.

## Falsification

Each mutation is a one-line edit of the listing found by its address comment, rebuilt and run against the
pinned oracle, the file restored between runs. A mutant that hangs the process is stopped by a 60-second limit.

| row | mutation | result |
|---|---|---|
| 001655 | `jne` that wraps the read index inverted (0x00032436) | hull_queue_pop mismatches=154 |
| 001423 | `je` that skips the first free inverted (0x00029a38, 0x00029a4f) | hull_free mismatches=6 |
| 001425 | `fmul` to `fdiv` in the plane distance (0x00029afb) | hull_coplanar mismatches=915 |
| 001425 | five more branch and arithmetic edits (0x00029b22, 0x00029b2e, 0x00029b96, 0x00029bb5, 0x00029c09, 0x00029a8b) | tape mismatches, or an access violation |
| 001427 | second edge's neighbour test inverted (0x00029ea7) | hull_flood mismatches=57 |
| 001427 | claimed-triangle test, polygon-claim, stamp store and queue-push edits (0x00029d0c, 0x00029d7b, 0x00029dcb, 0x00029eed, 0x00029c9b, 0x00029cc4) | rejected (the claim-store edit crashes) |
| 001427 | the eighteen opposite-vertex selections (`mov edi, ...` at 0x00029d6d to 0x0002a02b) | all rejected |
| 001429 | third edge's neighbour test inverted (0x00029fd9) | hull_flood mismatches=46 |
| 001431 | second push swapped (0x0002a134) | hull_cluster mismatches=1388, hull_flood_start 1 |
| 001431 | start-pair sentinel zeroed (0x0002a0ff), loop tests inverted (0x0002a10a, 0x0002a148, 0x0002a0bf) | 1,300 cluster mismatches, two hangs, an access violation |
| 001433 | wrong register stored to the stamp (0x0002a177) | hull_flood_entry mismatches=105 |
| 001433 | bound test inverted (0x0002a168), count cleared in the wrong field (0x0002a174) | crash, crash |
| 001435 | `jne` inverted (0x0002a229) | hull_cluster mismatches=4,584 |
| 001435 | edge-flatness and sort-key edits (0x0002a278, 0x0002a2de, 0x0002a2ea, 0x0002a1ac) | 1 to 3,116 |
| 001437 | `sub` to `add` in the group-rank index (0x0002a5a9) | hull_cluster mismatches=2,224 |
| 001437 | group-id store to the wrong field (0x0002a5ae) | hull_cluster mismatches=2,236 |

Not rejected, and why:

* The three `or edi, 0xffffffff` fallbacks of the opposite-vertex search (0x00029dc1, 0x00029ef3, 0x0002a02f):
  they are taken only when a neighbour listed for an edge shares neither orientation of it, which an edge list
  built from the mesh never produces.
* The first resize test of `001431` (0x0002a0ea): a pushed pair never meets a full container there, so
  inverting it only grows the container early.
* Before the rotated and flipped meshes, three branches of the vertex search (0x00029d7b and two others)
  were also unseen: the six harness meshes wind their neighbours one way.

## What this does not close

* The caller `phys_fn_002247` and the rest of the cooker around it; no reconstructed row reaches these.
* Meshes beyond the twelve: non-manifold edges are present only in the degenerate harness mesh.
