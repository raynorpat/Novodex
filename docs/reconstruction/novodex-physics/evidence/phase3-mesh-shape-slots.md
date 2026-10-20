# The convex mesh shape's raycast and sweep slots

Rows `phys_fn_001405` (RVA `0x00029230`, 978 B) and `phys_fn_001407` (`0x00029610`, 314 B), slots 5 and 7 of the
mesh shape's vtable. Both began as C++ models gated against the oracle; they are now naked listing
transcriptions (`Physics/src/ShapeRaycast.cpp`, `Physics/src/ObjectModel.cpp`), 306 and 102 instructions.
`evidence/phase3-mesh-raycast-listing-compare.py` disassembles the built candidate and the oracle and finds every
instruction equal, with the nine call mappings as expected (output retained as
`raw/phase3-mesh-shape-slots-20261020/transcription-listing-compare.txt`).

## 001405: the raycast

Slot 5 (thiscall, five stack arguments, `ret 0x14`). The row takes the scene's `RayCollider` from the shape's
owner chain (`shape + 4`, then `+ 4`, then `+ 0x450`; the candidate's `Scene` constructs one there), points it at
a one-entry external face buffer on the stack (`SdkContainer` constructor and `setExternalBuffer`), puts it in
closest-hit mode with the distance limit and the OPCODE model at `mesh + 0x28`, collides the world ray through the
shape's pose (`RayCollider::Collide`), and expands the closest face into a hit record: the impact from the
barycentric coordinates, the face (through the optional remap), distance, u, v, and with `NX_RAYCAST_NORMAL` the
normal from the lazily built smooth vertex normals (`phys_fn_002081`, reached as `nxMeshComputeVertexNormals`).
The callees are the candidate's own rows through `/alternatename` aliases (`SdkContainer`, `RayCollider::Collide`).

Gate: `NxPhysicsCollisionTests`, `nxDriveMeshRaycast` (unchanged): the oracle's row against the candidate's over
the six Task 2i meshes, identity and posed shapes, with and without the face remap and the normal, four distance
limits: 1,440 rays, 331 hits, 165 with normals, 163 from posed shapes, zero mismatches.

* The return value, the face, the flags and every word the row does not write are exact.
* The floats the collider computes (impact, normal, distance, u, v) differ from the oracle's by up to 1.2 parts in
  10^7 in 653 words: the candidate compiles the vendored OPCODE collider differently. They are compared to 2e-6;
  the gate prints the count and the worst difference.
* `mesh + 0x70` (copied into the collider at `+ 0x88`) is zero in the harness; with the harness's garbage in it the
  oracle collider misses every ray.

Falsification (one instruction of the listing edited, rebuilt, run against the oracle, restored):

| address | mutation | result |
|---|---|---|
| 0x0002927b | `add ecx, 0x450` made `0x454` | 331 mismatches |
| 0x00029340 | face-buffer test reads `[esp+0x34]` | access violation |
| 0x000293c1 | `fsub` of the barycentric term made `fadd` | 275 |
| 0x0002948e | remap branch inverted | access violation |
| 0x000294ba | flag word `0x3b` made `0x3a` | 331 |
| 0x000294ac | normal-hint mask `4` made `8` | 165 |
| 0x0002945b | translation add reads `+0x34` for `+0x30` | 163 |
| 0x00029298 | mesh word read at `+0x74` for `+0x70` | 331 |
| 0x00029365 | distance-limit branch inverted | 331 |
| 0x0002933e | collide-result branch inverted | 331 |
| 0x00029496 | remap index shifted by one | 158 |

Not covered: the scene's real RayCollider configuration (the harness uses a freshly constructed one, which is what
the scene constructor leaves); meshes beyond the six; a ray whose origin is inside the mesh.

## 001407: the sweep

Slot 7 (thiscall, `ret 8`). With no classifier at `mesh + 0xac` it returns false and copies the mesh's `+ 0x68`
word; with one it carries the point into the mesh's frame, picks the polygon plane the support map names for that
direction (`phys_fn_001556`), builds the hull's polygons first if absent (`phys_fn_001472`) and writes the
ray/plane distance (`NxRayPlaneIntersect`) from the hull's origin.

`NxPhysicsShapeVtableTests` compares the oracle's row at `0x00029610` with the candidate over 24 samples (four with
no classifier, twenty with a hand-built three-plane table, a 54-entry map, and varied pose and bin count). Exact.

| address | mutation | samples differing (of 24) |
|---|---|---|
| 0x0002963d | translation subtract reads `+0x30` for `+0x34` | 20 |
| 0x000296af | plane index read one byte on | 20 |
| 0x000296ac | hull table read at `+0x10` for `+0xc` | access violation |
| 0x00029737 | no-classifier word read at `+0x6c` | 4 |
| 0x0002971b | plane offset `0xc` made `0x10` | 20 |
| 0x0002966a | rotation term `[esi+0x14]` for `[esi+0x10]` | 20 |
| 0x000296c4 | polygon-build test inverted | crash |
| 0x00029625 | classifier test inverted | access violation |

Not covered: a cooked mesh's classifier and hull (the fixture is hand-built).

Logs and the mutation scripts are in `raw/phase3-mesh-shape-slots-20261020/` (`transcription-*`); the earlier
model-based runs are `raycast-mutations.txt` and `sweep-mutations.txt` there.
