# The convex mesh's hull subclass

Rows `phys_fn_001411` (RVA `0x00029780`), `001415` (`0x00029940`), `001417` (`0x00029980`), `001418`
(`0x000299a0`) and `001419` (`0x000299c0`), five functions, 208 instructions
(`Physics/src/HullShape.cpp`).

## What the rows are

The convex mesh's hull is a subclass of `ConvexHull` with its own vtable (`0x1010767c`): the base object (0x4c
bytes) followed by a bounding box (`+0x4c` minimum, `+0x58` maximum) and an ICE `Valencies` (`+0x64`).

* `001411` runs after a successful build: it allocates the Valencies, computes it from the hull's vertices and
  faces (and frees it again if that fails), and takes the vertices' bounding box.
* `001417` and `001418` are the overrides of virtual slots 1 and 2: the base's builder or initialiser
  (`HullBuild.cpp`), then `001411` when it succeeded.
* `001415` is the destructor body (the subclass vtable, the Valencies released and cleared, then a tail jump to
  the base release) and `001419` the scalar deleting destructor (the same inline, then the object freed
  through the SDK allocator when bit 0 of the flag is set).

Slot 3 (`001413`, the summed byte count) is reconstructed in `ObjectModel.cpp` and is left empty in the new
vtable. No reconstructed row calls these yet.

## How they were written

Naked listing transcriptions, generated from the pinned image: calls reach the SDK allocator getter, the
candidate's `Valencies` (constructor, destructor, `Compute`) and the base rows. The vtable is a four-slot array
in the file. `evidence/phase3-hull-shape-listing-compare.py` compares the built candidate with the oracle
instruction by instruction: 208 instructions, zero differing, and the 16 call mappings reach the candidate
functions they should.

## How they are gated

`NxPhysicsThirdPartyTests` (Phases 4 and 7), `nxDriveHullShape`: the same trampolines and the same kind of
inputs as `nxDriveHullBuild`, entered through the subclass's vtable slots. The hull, the box and the valencies
(counts and the three arrays) are taped, then the destructor body or the scalar deleting destructor is run and
its result taped.

* `hull_shape_init` (22,064 words): the box, two spheres, a bumpy grid, a flat grid, one triangle and three
  shifted copies (every vertex on one side of zero on each axis, so the bounding box's seeds decide the box),
  plain and with re-wound, duplicate, repeated-vertex and zero-area faces, as dword and 16-bit faces; 42 of 60
  accepted.
* `hull_shape_build` (13,678 words): six clouds with each orient/polygons combination (the polygons-without-
  orientation combination crashes both sides and is not driven), the extras run again by themselves, and the
  scalar deleting destructor with the flag set and clear; 12 of 36 accepted.

Both are exact with zero mismatches.

## Falsification

Mutations as in the other hull rows (one line of the listing, rebuilt, run against the oracle, restored).

| row | mutation | result |
|---|---|---|
| 001411 | `jp` of the first box comparison inverted (0x00029886; also 0x000298af, 0x000298d9) | hull_shape_init 42, hull_shape_build 30 |
| 001411 | minimum or maximum seed zeroed (0x00029830, 0x00029848, 0x00029858) | 4 to 10 |
| 001415 | Valencies pointer set to 1 after the release (0x00029965) | hull_shape_init 42 |
| 001417 | call to the extras dropped (0x00029997) | hull_shape_build 1 |
| 001418 | call to the extras dropped (0x000299b7) | hull_shape_init 1 |
| 001419 | Valencies pointer set to 1 after the release (0x000299e5) | hull_shape_build 18 |
| 001415, 001419 | the `je` that skips the Valencies release inverted (0x0002994f, 0x000299cf, 0x000299f8) | access violations |

Not rejected, and why:

* The failure arm after `Valencies::Compute` (`je` at 0x000297f8): it is taken only when the valencies cannot
  be computed, which the harness's face sets never cause.
* The vtable-pointer stores of the two destructors (0x00029949, 0x000299c9): the base release that follows
  overwrites the first, and the second is on an object that is freed.
* The first harness had no shifted inputs, so the zeroed box seeds (0x00029830 and the two beside it) went
  unseen until the shifted copies were added.

## What this does not close

* `001405` (the raycast) and `001407` (the sweep) of the shape that holds this hull are a separate harness
  and are not in this change.
