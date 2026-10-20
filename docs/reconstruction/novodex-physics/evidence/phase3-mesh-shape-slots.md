# The convex mesh shape's raycast and sweep slots

Rows `phys_fn_001405` (RVA `0x00029230`, 978 B) and `phys_fn_001407` (`0x00029610`, 314 B), slots 5 and 7 of the
mesh shape's vtable. Both already had a C++ model in the library; neither was driven against the oracle for
its own sake in the closure ledger. This change gates them and closes them.

## 001405: the raycast

Slot 5 (thiscall, five stack arguments, `ret 0x14`). The oracle takes the scene's `RayCollider` from the
shape's owner chain (`shape + 4`, then `+ 4`, then `+ 0x450`), puts it in closest-hit mode with the distance
limit and the OPCODE model at `mesh + 0x28`, collides the world ray through the shape's pose, and expands the
closest face into a hit record: the impact from the barycentric coordinates, the face (through the optional
remap), distance, u, v, and with `NX_RAYCAST_NORMAL` the normal from the lazily built smooth vertex normals
(`phys_fn_002081`). The candidate (`NxShapeRaycastMesh`, `ShapeRaycast.cpp`) is a model of this: it keeps a
`RayCollider` of its own in place of the scene's.

`NxPhysicsCollisionTests`, `nxDriveMeshRaycast`: the oracle's row (with a `RayCollider` built by the oracle's
constructor in a stand-in scene block) against the candidate's, over the six Task 2i meshes, identity and
posed shapes, with and without the face remap, with and without the normal in the hint, and four distance
limits: 1,440 rays, 331 hits, 165 of them with normals, 163 from posed shapes.

* The return value, the face, the flags and every word the row does not write are exact.
* The floats the collider computes (impact, normal, distance, u, v) differ from the oracle's by up to
  1.7 parts in 10^7 relative to `1 + |a| + |b|` in 792 words: the candidate compiles the vendored OPCODE
  collider differently. They are compared to 2e-6, which is twelve times that; the gate prints the count and the
  worst difference.
* Setting `mesh + 0x70` (a word the row copies into the collider at `+ 0x88`) to the harness's garbage made the
  oracle collider miss every ray; a real mesh has it zero, and so does the harness.

Falsification (a one-line edit of `ShapeRaycast.cpp`, rebuilt, run against the oracle, restored):

| mutation | mismatches |
|---|---|
| third barycentric weight `1 - u + v` | 275 |
| face remap read at index 0 | 131 |
| distance limit halved | 14 |
| closest-hit mode off | 1 |
| impact y uses `r[4]` for `r[3]` | 331 |
| normal x mixes n1 and n2 the other way | 68 |
| the UV flag dropped | 331 |
| `hit->u = v` | 331 |
| the pose's third column `r[7]` for `r[6]` | 175 |
| the remap test inverted | access violation |

Not covered: the scene's real RayCollider configuration (the harness uses a freshly constructed one, which is
what the scene constructor leaves); meshes beyond the six; a ray whose origin is inside the mesh.

## 001407: the sweep

Slot 7 (thiscall, `ret 8`). With no classifier at `mesh + 0xac` it returns false and copies the mesh's
`+ 0x68` word; with one it carries the point into the mesh's frame, picks the polygon plane the support map
names for that direction (`phys_fn_001556`), builds the hull's polygons first if absent (`phys_fn_001472`) and
writes the ray/plane distance from the hull's origin. `MeshShape::nxMeshSweepPrepared` (`ObjectModel.cpp`)
models it with the two product rows.

`NxPhysicsShapeVtableTests` already compared the oracle's row at `0x00029610` with the model over 24 samples
(four with no classifier, twenty with a hand-built three-plane table, a 54-entry map, and varied pose and bin
count). Six mutations of the model are rejected there:

| mutation | samples differing (of 24) |
|---|---|
| rotation row read with `r[7]` for `r[6]` | 20 |
| plane index taken as `map[0]` | 11 |
| ray origin read at `tree + 0x1c` | 20 |
| the translation added to the point instead of subtracted | 20 |
| the no-classifier word read at `+ 0x6c` | 4 |
| the null-classifier test inverted | access violation |

Not covered: a cooked mesh's classifier and hull (the fixture is hand-built).
