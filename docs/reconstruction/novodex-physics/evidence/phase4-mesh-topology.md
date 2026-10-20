# The triangle mesh's topology rebuild, release and destructor body

Rows `phys_fn_002067` (RVA `0x00051ef0`, 169 B), `002085` (`0x000523a0`, 17 B) and `002087`, `002089`, `002091`,
`002093` (`0x000523c0`, 2,460 B together: the inventory splits one function at its jump targets and each piece
carries its own row). All in `Physics/src/InternalTriangleMesh.cpp`, naked listing transcriptions, 559 instructions.
`evidence/phase4-mesh-topology-listing-compare.py` disassembles the built candidate and the oracle and finds every
instruction equal, with 20 call mappings as expected (`raw/phase4-mesh-topology-20261020/listing-compare.txt`).

## What the rows are

* `002067`: releases an `InternalTriangleMesh`'s arrays in order: the model (a virtual scalar delete through its
  vtable, `push 1; call [eax]`), the per-triangle data, the vertex normals, the face remap, the materials, the
  triangles and the vertices, each through the Foundation allocator and cleared.
* `002085`: the destructor body: `002067`, then a tail jump to the embedded `MeshInterface`'s no-op at `+0x24`.
* `002087` to `002093`, one function: describes the mesh to `MeshBuilder2` (zero-area faces killed, indexed
  geometry, relative indices, angle-weighted normals), adds every face, builds, replaces the face remap and
  materials with the builder's order, copies the builder's vertices and triangles into new arrays, builds an
  `EdgeList` and, for every edge shared by more than two faces, splits it: a pair of new vertices (the
  endpoints with their words XOR-perturbed by a counter that grows every seven pairs), the extra faces re-pointed with
  `IndexedTriangle::ReplaceVertex`, and the vertex array grown.

The Foundation allocator is the Foundation DLL's own exported global; the rows load it from the candidate's import
slot as the listing loads it from the oracle's. The callers (`TriangleMesh.cpp`) keep the cdecl entries
`nxInternalMeshRelease` and `nxInternalMeshBuildTopology`, which now forward to the rows; `nxInternalMeshDestruct`
is the new fastcall entry for `002085`. The earlier C++ model of the rebuild is replaced. `IndexedTriangle::ReplaceVertex`
(`0x000e4160`, `phys_fn_005187`) and the other callees are the candidate's own rows or the vendored source.

## How they are gated

`NxPhysicsCollisionTests`, `nxDriveMeshTopology`: the oracle's row and the candidate's over nineteen sources (the ten
Task 2i meshes, a cube, a tetrahedron, a cube with doubled vertices, one with a zero-area face, books of 4, 10 and 22
triangles sharing an edge, a single triangle and an empty mesh), each with and without materials and a face
remap, under both x87 control words: 152 rebuilds, 144 succeeding, 24 splitting vertices; and 86 release and
destructor cases over images with 43 of the 128 subsets of the seven slots set (every third) and a fake model object. The return, the
counts, every array word, the model-delete calls and the Foundation allocator's malloc sizes and frees in order
(a new log in `PhysicsCollisionOpcode.cpp`) are equal: zero mismatches.

## Falsification

One instruction of the listing edited, rebuilt, run against the oracle, restored (`raw/.../mt_mut.txt`):

| address | mutation | mismatches |
|---|---|---|
| 0x00052422 | kill-zero-area-faces `1` made `0` | 16 |
| 0x000525e8 | remap allocation `shl edx, 2` made 3 | 8 |
| 0x00052684, 0x000526a6 | vertex or triangle array size `shl eax, 2` made 3 | 144, 144 |
| 0x0005260e | remap/materials branch inverted | access violation |
| 0x00052992 | second ReplaceVertex reads the wrong word | 24 |
| 0x000529ac | triangle pointer step `4` made `8` | 24 |
| 0x000529b5 | loop `jb` made `jbe` | 24 |
| 0x00051efb | model null test inverted | access violation |
| 0x00051eff | model delete flag `1` made `0` | 42 |
| 0x00051f06 | triangle-data slot read at `+0x18` | crash |
| 0x00051f6b | triangles null test inverted | 44 |

Survivors, each unobservable: the builder's material id `0xffffffff` (`0x000524bb`) and smoothing group `1`
(`0x000524c6`) do not affect the output of this configuration; the `EdgeList` epsilon (`0x00052786`) is read only
by the active-edge pass, which a null vertex pointer skips; `0x00052492` (`jbe` made `jb`) differs only for a mesh
with no faces, which fails in `Init` before it; `0x000523a8` (the destructor's `lea ecx`) is ignored by the no-op
it jumps to.

Not covered: allocation failure (the recording allocator never returns null); a model whose destructor is not a
single virtual call.
