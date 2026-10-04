# PMap disconnected-component compute mismatch

Date: 2026-10-02. Oracle: pinned `NxPhysics.dll` SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` from
`D:\FlamingEnt__\Unreal_3\Binaries`.

## Reproduction

Build `NxPhysicsTriangleMeshApiTests` and run the isolated fixture:

```powershell
build\Release\NxPhysicsTriangleMeshApiTests.exe <absolute-pair-directory> disconnected32
```

The fixture creates two disjoint, identically oriented tetrahedra with eight
vertices and eight triangles, then calls `NxCreatePMap` at density 32 after
resetting `rand`. It leaves the cooked arrays untouched because the cached
Opcode model was built during `createTriangleMesh()` and the oracle classifier
uses that model. Oracle and candidate bounds are identical:
`bf800000.bf800000.bf800000.40a00000.3f800000.3f800000`.

## Result

| Pair | Physics SHA-256 | Result | Size | FNV-1a |
|---|---|---:|---:|---|
| Oracle | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` | pass | 9,522 | `847baf05835be7ce` |
| Candidate after the ray/AABB precision correction | `68b9c31e4453805abcd05cb3138ffe5cd434fb5b8c02b003a9143b6ffabbdc62` | fail | 9,567 | `7d53f520a22044cc` |

The cooker emits the same eight triangle geometries in a different face order:
the candidate orders the first tetrahedron before the second, while the oracle
orders the second before the first. Before the ray/AABB precision correction,
normalizing each assigned face id to its triangle's vertex geometry left 30 of
32,768 cell occupancy bits different and 94 semantic face assignments differed
among cells with matching inside/outside state. The current correction changes
the candidate payload hash but the classification/label comparison has not yet
been rerun, so those counts describe the prior candidate build only.

The first directly sampled valid-fixture cell is `(0,0,0)`, at `(-1,-1,-1)`,
exactly a mesh vertex. The prior candidate rejected its random ray at the root
AABB and counted zero hits, while the oracle classifies the cell inside.
`RayAABBOverlap` now keeps each cross-axis expression and radius sum in the
oracle's x87 extended precision through comparison (oracle instructions
`0x000b912d..0x000b913f`). With this change, the candidate traverses the cached
root and reports one ray hit for that sample. This fixes the observed
root-boundary divergence, but the full disconnected fixture remains red at
9,567 / `7d53f520a22044cc`; the next step is to decode and compare the current
raw maps and trace the next divergent sample. The single-tetrahedron density-32
and density-64 fixtures remain exact (`10,444` / `9a70de00aaf0edd4` and
`74,563` / `2c38820e277e9465`).

## Triage

An earlier fixture version overwrote the cooked arrays after the cached Opcode
tree had been built. On that inconsistent state, the oracle emitted 4,490 /
`c2276558dc4be981`; when the arrays were left intact, the same oracle emitted
9,522 / `847baf05835be7ce`. That earlier 4,490-byte expectation was therefore
invalid and has been removed. A temporary change from the candidate's
unquantized Opcode build to a quantized build left the candidate output
unchanged. Enabling ray backface culling on the inconsistent fixture changed
the result to `30,792 / f93a38a59cc17dcf`; this is not valid evidence for the
correct culling state. The current candidate source retains `mNoLeaf=true`,
`mQuantized=false`, and backface culling disabled. The residual remains
unresolved. The first root AABB rejection at cell 0 is corrected; decode the
current maps to identify the next semantic divergence, then trace that cell
through random-ray generation, Opcode traversal, and hit parity before
changing the implementation.

An earlier trace focused on cell 455 and its propagation chain through cells
423 and 391. That was not the first inside/outside divergence: cell 0 is a
directly sampled boundary case and the root AABB rejection described above is
now corrected. Do not treat the older cell-391 observation as the primary
remaining cause. Oracle decompilation resolves the collider state:
`FUN_100b5720` initializes culling at byte `+0x8d` to one, then
`FUN_1004e540` writes zero to that byte and masks the base flags at `+0x4` with
`0xfffffffc`. Candidate first-contact, temporal-coherence, and culling settings
therefore match this oracle helper; those options are not the current cause.
The old cell-455/423/391 propagation chain is not the first divergence and
should not guide the remaining fix.

This is a pinned red fixture for the PMap work queue, not a closed compute
path. Public Physics headers remain unchanged.
