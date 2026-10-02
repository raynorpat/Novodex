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
| Candidate | `c29be3942e3dbefee3241191014c853a732b54fc78dfdd85b79b4d76cb24f06f` | fail | 9,567 | `3345ee85533724e0` |

The cooker emits the same eight triangle geometries in a different face order:
the candidate orders the first tetrahedron before the second, while the oracle
orders the second before the first. Normalizing each assigned face id to its
triangle's vertex geometry leaves 30 of 32,768 cell occupancy bits different;
among cells whose inside/outside bit agrees, 94 have a different semantic face
assignment. Thus raw payload comparison includes the separately open cooker
ordering difference, but there is also a PMap classification/label residual.
The density-64 authored single-tetrahedron fixture remains exact
(`74,563`, `2c38820e277e9465`).

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
unresolved. Trace the first divergent unclassified cell through random-ray
generation, Opcode traversal, and hit parity before changing the implementation.

For the first normalized occupancy difference, serialized cell 455 was already
classified by propagation from cell 423, which was propagated from directly
sampled cell 391. Candidate cell 391 has sample point bits
`3eb5ad6a/be6739d0/bf800000`, nearest face 3, squared distance `0.00832457`,
ray direction bits `bea1b385/3f2f7cda/bf27f02e`, and zero ray hits, so it seeds
the propagated outside label. Oracle decompilation resolves the collider state:
`FUN_100b5720` initializes culling at byte `+0x8d` to one, then
`FUN_1004e540` writes zero to that byte and masks the base flags at `+0x4` with
`0xfffffffc`. Candidate first-contact, temporal-coherence, and culling settings
therefore match this oracle helper; those options are not the current cause.
The next trace target is the oracle helper's hit count for this sample and the
OPCODE traversal/triangle calculation that produced it.

This is a pinned red fixture for the PMap work queue, not a closed compute
path. Public Physics headers remain unchanged.
