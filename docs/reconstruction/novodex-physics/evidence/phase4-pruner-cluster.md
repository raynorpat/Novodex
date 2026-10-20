# The pruners' cluster: tree build, add/update/remove and the four queries

Phase 4 rows in `Physics/src/opcode/PrunerOctree.cpp`, naked listing transcriptions, 1,518 instructions on top of the
octree and its queries (`phase4-pruner-octree.md`); the whole file is now 44 rows, 2,797 instructions:

* the static pruner (`StaticPruner`, vtable `0x0011b9c8`): `005234` (`0x000e5740`, destructor body), `005236`
  (`0x000e5780`, slot 5, the plane query), `005238` (`0x000e5810`, slot 0, the scalar deleting destructor);
* the bounded dynamic pruner (vtable `0x0011b9f0`): `005240` (`0x000e5870`, constructor), `005244` and `005246`
  (`0x000e58a0`, the tree build, split by the inventory at a jump target), `005248`, `005250`, `005252`
  (`0x000e5a00`, `0x000e5a30`, `0x000e5a90`, AddObject, RemoveObject and UpdateObject), `005254` (`0x000e5ab0`,
  the plane query), `005256` (`0x000e5de0`, the ray and segment query) and `005258` with `005260` (`0x000e6060`, the
  sphere query, split likewise);
* the helpers they reach: `005223` (`0x000e52d0`, the culling walk's collecting callback), `005301` and `005320`
  (the octree header's constructor and release), `002148` (the empty box), the tree-cull walk (`0x000f1350`) and the
  sphere walk and entry (`0x000f1df0`, `0x000f2050`), `SetCenterExtents` and `IsInside`, `_ftol2` (`0x000f7a3c`, the CRT
  conversion the rows call) and the five static-initialiser thunks.

`evidence/phase4-pruner-octree-listing-compare.py` (on `listing_compare_lib.py`) finds every instruction of the 44 rows
equal with 118 call mappings as expected (`raw/phase4-pruner-cluster-20261020/listing-compare.txt`). A block of the
oracle's statics (`0x1012847c` to `0x101284d8`: five function-local containers and their guard words) is named by
offset on both sides (`static_ranges` in the library).

## How they are gated

`NxPhysicsCollisionTests`, `nxDrivePrunerCluster` (family `pruner_cluster`, `--pruner-cluster-only`): twenty scenarios
of the static pruner (three destruction variants) and the bounded pruner (a pool of three sections, owners with boxes,
masks and flags), each under both x87 control words, against the oracle's methods through the vtable and the candidate's
rows, with the SDK allocator bridge logging every malloc size and free:

* adds, updates after the boxes move, removals (every object in every fifth scenario, so the octree empties and is
  released), a second tree build over the existing octree in every fourth;
* the octree header's constructor on poisoned memory (0xcd), the bounded pruner's constructor likewise;
* 2,240 plane, ray, segment and sphere queries (1,699 non-empty);
* the digest folds the pool (counts, boxes, normalised object pointers), the bounds and registration words, the octree
  header and every node record (list heads as object indices), every object's fields and links, the owners' call
  counts, the result containers, the statics' containers and the allocator log.

Pointers are normalised to object indices. A link left in the released node array of a rebuilt tree is not compared
(it is a stale pointer; where the allocator puts the new array differs per run and per side, and the stale pointer can
even land inside it); an object has a node in the digest only if a node of the current tree lists it.

Zero mismatches; the digests are equal across runs.

## Falsification

One instruction of the listing edited by `raw/.../mutate_gen.py` (a jump inverted, an offset moved by four, an
immediate by one, an x87 operation swapped), rebuilt, run against the oracle, restored: 422 mutations in seven sweeps
(`pc_mut.txt` 200 over `0x000e5740` to `0x000e6440`, `pc_mut2.txt` 70 over the tree-cull walk, `pc_mut3.txt` 45 over the
collecting callback, `pc_mut4.txt` 80 over the sphere walk and entry, `pc_mut5.txt` 14 over the empty box, `pc_mut6.txt` 7
over the octree header's constructor and `pc_mut7.txt` 6 over its release). Of the 422, 265 are rejected by mismatches,
134 by an access violation or heap corruption and 1 by a hang; 22 survive.

Survivors (not rejected by the twenty scenarios, listed so that they are not mistaken for proven equivalents):

* stack temporaries moved by four where the neighbouring slot is not read back: `0x000e57e9`, `0x000e57f4`,
  `0x000e52fb`, `0x000f1e2c`;
* flag-word operands whose changed bit the scenarios never test: `0x000e5799` (`and edi, 4` made 5), `0x000e5eb3`
  (`test [esi+8], 2` made 3), `0x000e5eff` (`push 1` made 2), `0x000e60be` (`or edx, 2` made 3), `0x000f1476`
  (`test ecx, 0xfffffffe` made `-1`);
* the first-use guards of the function-local containers (`0x000e5e0d`, `0x000e6087`, a jump inverted), whose
  constructor only clears storage the test has cleared;
* object-field reads that return the same value in every scenario: `0x000e5e9a`, `0x000e5fca`, `0x000e5fe8`,
  `0x000e5978`, `0x000e5985`, `0x000e530c`;
* the octree release's count clearing (`0x000e7c33`, `[esi+0x48]` made `[esi+0x4c]`): the tree is dropped after it;
* the empty box (`0x00053815`, `0x0005381f`, `0x00053821`, `0x00053830`): no scenario builds a tree over a pool with no
  valid box.

Not covered: pools beyond three sections; allocation failure; a tree build over an empty pool.
