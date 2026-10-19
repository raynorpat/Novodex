# Mesh contact against a penetration map

Rows `phys_fn_001878` (RVA `0x000470f0`, 3,422 B) and `phys_fn_001880` (`0x00047e50`, 64 B), and a
correction to `phys_fn_002029` (`0x0004f070`, 3,751 B), the cell sampler they call.

## What the rows are

`phys_fn_001876` (`NxContactMeshMesh`) has, between its terrain cases and its convex ones, a branch
the candidate lacked: when both shapes' meshes carry a `PenetrationMap` (`mesh + 0x9c`) it calls
`phys_fn_001880`, which runs `phys_fn_001878` twice, once with each shape as the map's owner and the
other as the mesh. `001878` (cdecl, with the mesh's pose block in `ebx`) does this:

1. inverts the owner's rotation and carries the owner's map box into the mesh's frame as six planes
   (`OBB::ComputePlanes`, `phys_fn_005199`);
2. asks the scene's `PlanesCollider` (`phys_fn_005138`, at context `+0x60`, cache `+0xa8`) for the faces
   the box touches;
3. for each vertex of those faces, once per query by the context stamp (`phys_fn_000505`), tests it against
   the map's bounds and the cell's class bit (bit 30 of the cell word), and asks
   `PenetrationMap::sampleCell` (`phys_fn_002029`) for the displacement, with the vertex's normal (built
   on first use by `phys_fn_002081`) as the direction and the owner's frame as the matrix;
4. emits a contact (`phys_fn_000873`) at the vertex, along the displacement, with the separation the
   negated length. Both shapes are reached through slot 14 of their vtables.

The candidate had no caller of `sampleCell` at all outside its tests; `PMap.h` listed `001878` as open.

## How they were written

Naked listing transcriptions (`Physics/src/ContactMeshPMap.cpp`, `PenetrationMap::sampleCell` in
`Physics/src/PMap.cpp`), generated from the pinned image by a script that renames call targets and
constants and leaves every instruction alone, in the style of the other mesh-contact rows.
`evidence/phase3-mesh-pmap-listing-compare.py` disassembles the built candidate and the oracle and
compares them instruction by instruction: 848 + 31 + 1,054 instructions, zero differing, and the six
callees of `001878`, the one of `001880` and the one of `sampleCell` reach the candidate functions they
should. `NxContactMeshMesh` now takes the branch.

## The sampler was wrong

`phys_fn_002029` was already "dynamically gated": seven hand-built fixtures matched the oracle. The new
harness (`NxPhysicsCollisionTests`) builds a 16-cell `PenetrationMap` for each of the ten harness
meshes (every lattice corner holds the nearest face, with the high bit set behind that face) and,
before running any contact, compares the oracle's `0x0004f070` with the candidate's over 4,000 drawn
cells, points, directions and matrices. The C++ reconstruction disagreed on **1,583 of the 4,000**
(39.6%). Replacing it with the listing transcription makes all 4,000 agree. The old fixtures still
pass (the Phase 4 asset gate is unchanged), which is the lesson: a handful of fixtures does not gate
a 1,054-instruction x87 function.

## How they are gated

`NxPhysicsCollisionTests` (Phases 3 and 7):

* `pmap_sample_dense`: the 4,000 samples above, oracle and candidate digests equal.
* `contact_mesh_pmap` (the oracle's `0x00047e50` against the candidate's `nxMeshPMapContacts`) and
  `contact_mesh_pmap_dispatch` (the oracle's `0x00046ab0` against `NxContactMeshMesh`): 21 fixtures over
  ten mesh pairs and both orientations, 18 with contacts and 3 separated, 155 contacts, zero
  mismatches. Three fixtures give only one shape a map and one gives neither; there the dispatcher
  must take the plain mesh/mesh walk, and the oracle finds no contact where the map path would.
  The harness gives each shape the vtable slot the row calls.

## Falsification

Eleven mutations, each a one-line edit of the listing (found by its address comment) or of the
dispatcher, rebuilt and run against the pinned oracle; the file is restored between runs.

| row | mutation | result |
|---|---|---|
| 001878 | context stamp `je` to `jne` (0x00047a59) | exit 1, mismatches=128 |
| 001878 | cell class mask 0x40 to 0x80 (0x00047c13) | exit 1, mismatches=320 |
| 001878 | per-face vertex count 3 to 2 (0x00047e31) | exit 1, mismatches=584 |
| 001878 | separation 1.0f larger in place of its negation (0x00047e05) | exit 1, mismatches=278 |
| 001878 | plane count 6 to 5 (0x000479b0) | **not rejected** |
| 001878 | separation not negated (0x00047e05) | **not rejected** |
| 001880 | first call's mesh pose from the wrong shape (0x00047e6b) | exit 1, mismatches=1723 |
| 001880 | second call's owner the wrong shape (0x00047e7e) | exit 1, mismatches=274 |
| 002029 | final acceptance `jnp` to `jp` (0x0004fe85) | exit 1, pmap_sample_dense mismatches=2659 |
| 002029 | one accumulation `fadd` to `fsub` (0x0004fe44) | exit 1, pmap_sample_dense mismatches=1848 |
| 001876 | `&&` to `||` in the PMap branch | exit 1, mismatches=16 |

The two that are not rejected are equivalent at the stream: with five planes the box admits faces whose
vertices the map's bounds test then rejects, and the contact stream does not carry the separation's
sign. The 001876 mutant was undetected until the one-map fixtures used meshes with different images
(the harness shares a mesh image between slots when both shapes use the same mesh).

## What this does not close

* The maps are lattice maps built from the harness meshes, not maps cooked by `NxCreatePMap`; the
  sampler and the walk read the same memory either way, but no cooked map is run through the contact
  path.
* `phys_fn_001876` itself stays open in the inventory: this wires one more branch of its dispatcher and
  gates it, and the other branches have their own families.
* `PenetrationMap::sampleCell` is no longer plain C++; its comment and `PMap.h` say so.
