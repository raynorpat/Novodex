# NovodeX's depth-limited callback walk over two no-leaf trees

Rows `phys_fn_004967` (RVA `0x000ca5a0`, 6,315 B), `phys_fn_004969` (`0x000cbe50`, 6,315 B),
`phys_fn_004971` (`0x000cd700`, 12,843 B) and `phys_fn_004982` (`0x000d12b0`, 140 B).

## What the rows are

The census carried the first three as `vendored_not_falsified` "second instantiations" of OPCODE's
`_CollideTriBox`, `_CollideBoxTri` and `_Collide(AABBNoLeafNode*, AABBNoLeafNode*)`, graded `probable`
in `phase4-third-party-map/opcode_map.csv`, and 004982 as a second `Collide(AABBNoLeafTree*, ...)`.
Reading the listings shows they are not stock OPCODE. Each takes a callback and a depth that stock
OPCODE does not have, and the walk stops differently:

| | stock `AABBTreeCollider` | the oracle's rows |
|---|---|---|
| arguments | node(s) | node(s), callback, depth (004967 and 004969 also take the node of the other tree) |
| triangle fetch | `MeshInterface::GetTriangle` through its strides | straight from the mesh interface's index array (+8) and vertex array (+0xc), stride 12 |
| triangle/triangle hit | `mPairs.Add(id0).Add(id1)` and `OPC_CONTACT` | `OPC_CONTACT` only; no pair is recorded |
| early out | `ContactFound()`: `OPC_FIRST_CONTACT` and `OPC_CONTACT` | `OPC_CONTACT` alone |
| reporting | none | `callback(node0, node1)` |

The depth rule, which is the whole of the rows' own behaviour, is the same after every child step
(a recursion into a node pair, or a leaf test):

* a leaf step with `depth > 0` that leaves `OPC_CONTACT` set reports the pair and the walk goes on;
* otherwise, with `depth < 1` and `OPC_CONTACT` set, the walk stops, and reports the pair itself
  when `depth == 0`;
* every recursion passes `depth - 1`.

`phys_fn_004982` is the entry. It stores the two models' mesh interfaces at `this+0x20/0x24`,
refuses a null interface and any model that has leaf nodes or is quantized, runs `InitQuery`
(`phys_fn_004944`) and enters 004971 on the two node arrays. Its only caller is `phys_fn_001876`
(`0x00046ab0`, mesh/mesh contact), which passes `phys_fn_001874` as the callback and depth zero. At
depth zero the callback therefore fires once, with the root pair, after the first triangle pair
found to touch. That is the behaviour `NxContactMeshMesh` approximates today; this closes the walk
under it, not that caller.

## The candidate

`Physics/src/opcode/NovodexTreeColliderCallback.{h,cpp}`: `Opcode::NxCallbackTreeCollider`
derives from `AABBTreeCollider` for access to its protected state and adds no data. It reuses the
vendored `BoxBoxOverlap`, `TriBoxOverlap` and `TriTriOverlap` (the headers are included as
`OPC_TreeCollider.cpp` includes them; `CoplanarTriTri` is renamed for this unit because the header
defines it outside the class). `LayoutMatchesOracle()` and `sizeof` are registered layout checks:
every field the rows touch is at the oracle's offset and the object is 276 bytes.

## How it is gated

All in `NxPhysicsThirdPartyTests`, on the oracle's own built models, with the candidate walking the
same models:

* `opcode_treecollider_callback` enters the oracle's `0x000d12b0` and the candidate's
  `CollideNoLeafCallback` with the same models, worlds, settings and depth (-1, 0, 1, 2, 3 and 100),
  taping the return value, the contact bit, all three test counters and every callback pair in
  order. It also sends the three other tree kinds on either side, and a model with a null mesh
  interface on either side, to reach each of the four refusals alone.
* `opcode_treecollider_callback_collide`, `_tribox` and `_boxtri` enter `0x000cd700`, `0x000ca5a0` and
  `0x000cbe50` directly on a collider the harness has set up the way 004982 and 004971 leave it
  (`InitQuery` run, mesh interfaces set, a triangle in the leaf slot from the candidate's own fetch
  copied into the oracle's), on random node pairs and triangles.

The exact families are 0 mismatches (91,354, 20,584, 13,166 and 13,266 words). The **`_boundary`
families** hold the queries that involve the flat grid (mesh 2) and the degenerate set (mesh 3)
against a rotated copy of itself. They are not exact: the grid's vertices line up on the same x
and y lines as the other meshes', and the oracle's inline copies of the triangle test each
associate their sums differently (the first copy adds y, x, z; the second x, z, y), so a verdict
that is decided by the last bit goes the other way, and every later word of that family's tape then
moves. This is the cause the existing `opcode_treecollider_boundary` family records for the first
instantiation. The ceilings in `kDivergentCeilings` are the measured values and any run over them
fails: 19,486 / 3,785 / 1,286 / 325 words, of which 19,471 / 3,107 / 655 / 165 discrete, with the
tapes 16 / 4 / 2 words longer, shorter or equal as recorded in `kLengthCeilings`.

## Falsification

Twenty mutations, each a one-line edit to the candidate in a throwaway state of the file, rebuilt
and run against the pinned oracle; the file is restored and the control rebuilt between runs
(control: `thirdparty candidate mismatches=0 layout_failures=0`, exit 0).

| row | mutation | result |
|---|---|---|
| 004967 | box rejection dropped | exit 1; callback 1, collide 1, tribox 1 |
| 004967 | recursion without depth-1 | exit 1; callback 1, collide 1, tribox 1 |
| 004967 | first child's leaf flag forced false | exit 1; callback 1, collide 1, tribox 1 |
| 004967 | negative leaf read from the positive primitive | crash (the candidate walks past the mesh), exit 3221225477 |
| 004969 | callback pair swapped after the first child | exit 1; callback 6,804, collide 1,108, boxtri 364 |
| 004969 | recursion without depth-1 | exit 1; callback 1, collide 1, boxtri 1 |
| 004969 | negative child's leaf flag forced true | exit 1; callback 1, collide 1, boxtri 1 |
| 004969 | box rejection dropped | exit 1; callback 1, collide 1, boxtri 1 |
| 004971 | last recursion without depth-1 | exit 1; callback 1, collide 1 |
| 004971 | leaf slot through the wrong rotation | exit 1; callback 1, collide 1 |
| 004971 | a step's leaf flag inverted | exit 1; callback 1, collide 1 |
| 004971 | wrong tree-0 node handed down | exit 1; callback 1,178, collide 254 |
| 004971 | box test arguments exchanged | exit 1; callback 1, collide 1 |
| 004971 | tree-1 negative leaf through the wrong rotation | exit 1; callback 1, collide 1 |
| 004982 | worlds exchanged for `InitQuery` | exit 1; callback 1 |
| 004982 | leaf-node refusal dropped (model 0), quantized refusal dropped (models 0 and 1), leaf-node refusal dropped (model 1) | crash, exit 3221225477 |
| 004982 | null-mesh refusal weakened to require both | crash, exit 3221225477 |

A count of 1 is the harness's "tape length differs" word. The crashes are real rejections (the
target exits non-zero) but not clean ones: a candidate that stops refusing walks the oracle-built
tree as the wrong kind. The one mutation not rejected in the first pass, the null-mesh check, was
not rejected because no query had a null interface; the query was added and it is now rejected.

## What this does not close

* `phys_fn_001876` (`NxContactMeshMesh`) now enters the walk: for two plain no-leaf models it runs
  `NxCallbackTreeCollider::CollideNoLeafCallback` with depth zero and a callback that hands `phys_fn_001874` the
  two nodes' boxes, replacing the earlier hand-written traversal (and its oriented-box, triangle/box and
  triangle/triangle helpers, now deleted). The 23-fixture `contact_mesh_mesh` differential in
  `NxPhysicsCollisionTests` is exact before and after (four contact cases, nineteen separated), and a depth-one
  edit of the call makes five of those fixtures differ (exit 1). The other tree layouts keep the vendored
  collider's pair path, where the oracle reports an error. `phys_fn_001876` itself, `001878` and `001880` stay
  open in the inventory: this wires one branch of the dispatcher and does not mutate-close it.
* Whole-DLL routing of these rows is untouched; no public header changed.
* The `_boundary` families are bounded, not exact.
