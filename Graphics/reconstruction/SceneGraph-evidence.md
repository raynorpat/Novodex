# Scene graph reconstruction

Oracle: original VC6 COFF members extracted from `GraphicsLib.lib`; loaded through
IDA MCP with explicit per-member database IDs. Full Hex-Rays dumps are under the
task's `graphics-oracle` directory on C: to conserve workspace disk space.

## Object.obj

| Address | Function | Observation |
| --- | --- | --- |
| 0x000c | constructor | Identity matrices/quaternion, zero position, scale zero (no scale), null children and primitive; supplied parent recorded only. |
| 0x0110 | destructor | Notify Scene, recursively own children and primitive; model remains unowned. |
| 0x01b0 | getBoundingBox | Model dimensions or `(1,1,1)`; nonzero scale multiplies dimensions. |
| 0x0214 / 0x0264 | createChild / addChild | Fill left/middle/right; when full create a transparent `bNullXform` storage node. Original chooses branches with rand and contains a broken right-branch rewrite. |
| 0x0404 / 0x0504 | deleteChild / removeChild | Recursive subtree search; delete owns whole matched subtree, remove leaves memory intact. |
| 0x05b4 / 0x05f4 | setPosition | Clear null-transform flag and update translation immediately. |
| 0x0634 | updateMatrix | Copy orientation 3x3 and position into local matrix unless transparent. |
| 0x06a4 / 0x07f8 | setOrientation | Quaternion conversion; Euler vector is degrees, with x mapping to yaw about Z, y pitch about Y, z roll about X. |
| 0x08bc / 0x08dc / 0x0b1c | visibility / lookAt / scale | Visibility hides model only; lookAt uses -Z forward and cross-product basis; zero scale is sentinel. |
| 0x0b3c | render | Local matrix then nonzero uniform scale, optional wireframe/two-sided attributes; children rendered independently of hidden model. |
| 0x0cbc / 0x0d7c | light / camera | Light uses position and fixed defaults; camera loads inverse local and ancestor rigid transforms, ignoring scale and transparent ancestors. |
| 0x0fdc / 0x108c | collapse / executeTransform | Shipped binary has placeholder bodies (returns supplied model / no-op). Header contracts implemented as a documented extension. |
| 0x0fec / 0x106c | deleteSubtree / removeSubtree | Delete children recursively / disconnect the three roots only. |

Safety deviations: null/cycle/duplicate attachment rejection, reparenting detaches
old owner, removal clears parent pointers, destruction detaches parent and does
not instantiate Scene. Deterministic left-branch overflow storage avoids the
oracle's broken random branch. Objects continue to own only children/primitive.
Public Object/Primitive destructors are virtual in the rebuilt ABI so owned
derived objects and primitives receive their complete destruction.
Camera inversion includes cumulative uniform scale; light positions use the
cumulative hierarchy rather than only the local position. Collapse deep-copies
models into root or parent space. executeTransform creates a separate model/mesh
per modeled node, bakes cumulative geometry into parent space, then resets all
subtree transforms; shared originals remain unchanged. Created resources are
registered with Scene. Generic primitives retain their drawing through an owned
transform wrapper. Custom virtual models without mesh data retain a per-object
model transform, keeping their callbacks and shared identity. Both paths support
repeated baking and preserve appearance; see `Offscreen-and-baking.md`.

## Iterator.obj

Constructors 0x000c/0x00a8/0x0144 initialize traversal for Object/Model/Mesh.
init 0x01e0, destructor 0x0210, flags 0x0240, cumulative transform 0x0260,
depth-first Object traversal 0x0280, model 0x0450, materials 0x0470, mesh 0x04b0,
submesh 0x04d0, transform descent 0x0510, delayed ascent 0x07f4, visits
0x0804/0x0814/0x0834. Order is root, left, middle, right recursively. The
root transform can be omitted, producing no matrix for the first root visit.
Nonzero scales multiply only the basis columns. Reconstruction recomputes the
current ancestor path for clarity and correct sibling transitions, ignores
transparent storage-node transforms, and makes direct Mesh iteration work.
Null mesh/model visits are safe (oracle dereferences some null pointers).

## Model.obj

Constructor 0x000c nulls mesh; copy 0x003c shallow-copies mesh/material references;
destructor 0x022c frees STL members only. Load 0x028c parses `Mesh`, `Terrain`,
`Scale`, `TexCoords`, `ForceNormals`, `ForceTexCoords`, `Groups`; resources are
reused by name through Scene and newly loaded meshes/materials registered there.
Terrain is explicitly disabled in oracle. Groups entries identify mesh groups
and contain material filenames; material vector grows with null placeholders.
getBoundingBox 0x07a8 delegates to mesh; render 0x07b8 activates material, renders
matching submesh, deactivates material. Merge 0x0808 is a shipped no-op.

Extensions: userData initialization, copy retains name, empty model bounding box
is zero, TexCoords is honored (oracle reads but passes false), missing group
materials leave geometry renderable. Merge deep-copies indexed eight-float
vertices/submeshes, applies transform to positions/normals, preserves material
alignment or collapses geometry. New meshes/models register in Scene.
Terrain directives now load CHU8/CHU9 or mesh aliases through Terrain.cpp;
see `Terrain.md` for the independently grounded format and fidelity limits.

## Bounds3d.obj

include 0x0000, intersect 0x00f0, combine 0x01d0, intersects 0x02d0, contain
0x0340 implement closed AABBs with an empty flag. transform 0x03a0 uses a
transformed center and absolute rotation times extents. The decompiled oracle
overwrites its center with the minimum before forming max, so its upper bound
is center rather than center+extent; reconstruction corrects this mathematical
defect and tests both transformed extreme corners.
