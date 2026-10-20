# Sphere against a triangle mesh

Rows `phys_fn_001925` (RVA `0x0004a820`, 274 B), `phys_fn_001927` (`0x0004a940`, 2,223 B) and
`phys_fn_001929` (`0x0004b1f0`, 1,542 B). The first two were deferred as blocked on Phase 4's
`TriangleMesh`; the Phase 4 mesh fixtures that the capsule/mesh and box/mesh rows already use now drive them.

## What the rows are

* `001929` is matrix A `[SPHERE][MESH]`, the contact. A pair whose mesh shape has no holder
  (`owner + 8`) goes to the continuous-CD entry first. It clears the two transient bits of the pair
  context's flags (`context + 0xb4`), sets or clears the height-field bit (`0x10`) from the mesh's
  vertical axis (`mesh + 0x7c`, `0xff` for an ordinary mesh), carries the sphere into the mesh's frame, and asks
  the context's `SphereCollider` (`context + 0xb0`, cache `+ 0xf4`) for the triangles the sphere touches.
  An ordinary mesh hands every triangle to `001927`; a height field hands it only the first whose
  projection along the vertical axis contains the sphere's centre.
* `001927` is the per-triangle row both paths share (register convention, seven stack words): it finds the
  nearest point on the triangle (`phys_fn_001672`), builds the contact normal (the face normal; or, with the
  smooth-sphere flag in the mesh shape, the vertex normals, built on first use by `phys_fn_002081`), takes the
  sphere radius off the distance, and emits the contact with `NxEmitContactFeatures` (`phys_fn_000875`),
  remapping the face and material through the mesh's tables when it has them.
* `001925` is matrix B `[SPHERE][MESH]`, the overlap: first-contact mode in the same `SphereCollider`, then
  OPCODE's contact bit.

The earlier C++ version of `NxContactSphereMesh` and `NxOverlapSphereMesh` matched ordinary faces and one
edge normal; `001927` had no row of its own and nothing drove any of it against the oracle.

## How they were written

Naked listing transcriptions (`Physics/src/ContactSphereMesh.cpp`), generated from the pinned image, the same way
as the other mesh-contact rows. `evidence/phase3-sphere-mesh-listing-compare.py` compares the built candidate with the
oracle instruction by instruction: 80 + 682 + 440 instructions, zero differing, and the ten call mappings
(`SphereCollider::Collide`, `NxShapeOwner`, `NxContinuousCdPair`, `NxTrianglePlane`, `NxEmitContactFeatures`,
`NxPointTriangleSquareDistance`, `nxMeshComputeVertexNormals` and the `001927` call) reach the candidate functions
they should.

## How they are gated

`NxPhysicsCollisionTests` (Phases 3 and 7), `nxDriveSphereMesh`. The pair context carries a `SphereCollider` and cache
built the way the scene builds them (the oracle's constructor at `0x000de7e0` for the oracle's side). 396 cases: six
harness meshes, three sphere poses (inside, resting on the top, straddling an edge), two radii, and eleven mesh modes:
ordinary, smooth, height fields over z (with and without the smooth flag), y and x, a mesh with no holder, a posed mesh
(random rotation and translation, ordinary and height field), and a mesh with neither material table nor face remap
(ordinary and height field). The pair-context flags start from a drawn value and are compared afterwards.

* `overlap_sphere_mesh`: 396 results, 153 true.
* `contact_sphere_mesh`: 396 contact calls, 112 with contacts, 244 contacts in all; the whole stream (points, normals,
  separations, features, materials, faces) is compared.
The pair-context flag word is compared after every call and folded into both digests. Zero mismatches.

## Falsification

Each mutation is a one-line edit of the listing found by its address comment, rebuilt and run against the pinned oracle,
the file restored between runs.

| row | mutation | result |
|---|---|---|
| 001925 | `je` after the contact-status test inverted (0x0004a920) | exit 1, overlap_sphere_mesh mismatches=396 |
| 001925 | `je` after the collide result inverted (0x0004a917) | mismatches=153 |
| 001925 | flag mask 0xffffffef to 0xffffffff (0x0004a84c) | mismatches=318 |
| 001925 | first-contact bit dropped (0x0004a82f); bit-1 mask dropped (0x0004a840) | 264, 264 (the context-flag comparison) |
| 001927 | `fmul` to `fdiv` in the point-to-triangle setup (0x0004a973) | contact_sphere_mesh mismatches=1815 |
| 001927 | `fmul` to `fdiv` (0x0004aa07, 0x0004aba4, 0x0004ade9, 0x0004afc9, 0x0004b0b6) | 420, 810, 487, 72, 36 |
| 001927 | `je`, `jp`, `jnp` inverted (0x0004aac3, 0x0004ac72, 0x0004af19, 0x0004b145) | 252, 504, 90, 273 |
| 001927 | smooth-normal flag test inverted (0x0004aea6) | 1265 |
| 001927 | `fsub` to `fadd` (0x0004ad38) | 109 |
| 001929 | `fmul` to `fdiv` in the posed-triangle setup (0x0004b57a) | mismatches=287 |
| 001929 | `fmul` to `fdiv` (0x0004b42a, 0x0004b46e, 0x0004b4c9, 0x0004b5e1, 0x0004b63b, 0x0004b67d, 0x0004b6d5) | 95, 103, 77, 127, 76, 72, 138 |
| 001929 | posed translation `fadd` to `fsub` (0x0004b510) | 67 |
| 001929 | height-field bit 0x10 to 0x20 (0x0004b27f) | 249 |
| 001929 | collide-status test inverted (0x0004b394) | 336 |
| 001929 | height-field dispatch inverted (0x0004b3d0) | 110 |
| 001929 | flag masks widened (0x0004b252, 0x0004b261, 0x0004b284) | 229, 486, 90 |
| 001929 | remap load for a table-less height field inverted (0x0004b731) | crash (exit 3221225477) |

Not rejected, and why:

* The `1.0f` stores into the fourth column of the matrix the rows build (`0x0004a8e1`, `0x0004b32e`) and the zero stores
  beside them (`0x0004a8c9`): the collider never reads that column.
* The branch to `NxContinuousCdPair` (`0x0004b20d`): its callee returns `true` at once while the SDK's `NX_CONTINUOUS_CD`
  is zero, and the harness has no SDK. The no-holder mode reaches the branch but cannot show the sweep.
* The first harness posed no mesh, gave every mesh tables and drew no starting flags; mutations of the rotation terms
  (0x0004afc9, 0x0004b0b6), the translation (0x0004b510), the remap load (0x0004b731) and the flag masks (0x0004a840)
  went unseen until the posed, table-less and flag-start modes were added.

## What this does not close

* The continuous-CD sweep is not driven.
* Maps, meshes with real cooked normals and meshes larger than the harness's six are not run; the smooth normals are the
  ones `phys_fn_002081` builds on the harness meshes.
