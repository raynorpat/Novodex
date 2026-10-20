# The bounded dynamic pruner's loose octree and its queries

Eighteen Phase 4 rows in `Physics/src/opcode/PrunerOctree.cpp`, naked listing transcriptions, 1,279 instructions with
the two AABB helpers they inline (`phys_fn_000923` and `005145`, which are not new rows):

* the octree: `005295` (`0x000e7250`, a box scaled about its centre), `005303` (`0x000e7380`, the node boxes by
  recursion), `005305` (`0x000e7570`, a walk with a callback), `005307` (`0x000e75d0`, the callback that loosens a
  node's box), `005309` (`0x000e75f0`, unlinking an object), `005313` (`0x000e7690`, a walk from the root), `005315`
  (`0x000e76c0`, the build), `005317` (`0x000e7850`, inserting an object) and `005318` (`0x000e7b80`, updating one);
* the queries over it: `005545` (`0x000f1970`, a sphere against a box), `005547` and `005549` (`0x000f1a30` and
  `0x000f1a70`, the objects of a node and of a subtree appended to a container), `005550` and `005552`
  (`0x000f1ac0`, the plane-culling walk, and `0x000f1c60`, its entry), `005554` and `005556` (`0x000f1cb0` and
  `0x000f1d10`, the segment walk and entry) and `005558` and `005560` (`0x000f1d50` and `0x000f1db0`, the ray walk
  and entry).

`evidence/phase4-pruner-octree-listing-compare.py` (on `listing_compare_lib.py`) disassembles the built candidate and
the oracle and finds every instruction equal, with 38 call mappings as expected
(`raw/phase4-pruner-octree-20261020/listing-compare.txt`).

## What the rows are

The scene's type-1 pruner (`BoundedDynamicPruner`) keeps a loose octree: a 0x50-byte header (the world box at `+0`,
its centre negated at `+0x18`, the root side at `+0x24`, six level scales, the depth at `+0x44`, the node count at
`+0x48`, the node array at `+0x4c`) over 32-byte node records (a box, an object count at `+0x18`, the head of a list
of objects at `+0x1c`). An object is the 0x14-byte member of a `Prunable` at `+0xc` (the prunable at `+4`, the list
links at `+8` and `+0xc`, its node at `+0x10`). Insertion finds the level from the object's size and the cell from
a Morton code of its centre; the box comes from the caller or from the adapter hook (`gPrunableAdapterNotify`, which
the rows call with the prunable and a 16-byte centre and radius). The queries walk the nodes with an explicit state
{count, nodes, two words} and fill a container (the `SdkContainer` layout the Foundation allocator grows).

Three places differ from a plain copy:

* The octree's jump table (eight octant cases, `0x000e7544`) is a chain of compares in the candidate, because the
  inline assembler cannot emit a table of label addresses. The compare script collapses the chain and checks its
  targets against the table.
* `_ftol2` is the candidate's CRT conversion in place of the oracle's (`0x000f7a3c`); the arguments are in range.
* The adapter hook, the container growth (`SdkContainer::resize`), `nxSegmentAABB`, `nxRayAABB` and `MakeCube` are
  the candidate's own rows, reached by `/alternatename` aliases.

## How they are gated

`NxPhysicsCollisionTests`, `nxDrivePrunerOctree` and `nxDrivePrunerOctreeQueries` (families `pruner_octree` and
`pruner_octree_query`), under both x87 control words, with an SDK allocator bridge that logs every malloc size and free
(both sides' `0x000b4000`-style getter reaches it).

* `pruner_octree`: 12 tree configurations (depth 0 to 5) with 48 objects of five sizes, a fraction outside the root:
  the build, 48 inserts (the box given for a third, the hook asked for the rest), whole and cut-short walks and walks
  from inner nodes, an update of every object after the hook's boxes move, removal of every third and twice of some,
  the loosening callback over the tree and a rebuild. 24 cases, 1,078 inserts accepted and 74 rejected, 1,846 hook
  calls, plus 600 direct cases of the box scaling, `SetCenterExtents` and `IsInside` with NaN and tied words.
* `pruner_octree_query`: 10 trees, 24 plane, segment and ray queries each (480 queries), the node and subtree appends
  from the first nodes, and 120 sphere-box tests; 690 non-empty results.

The header, every node record (the list heads as object indices), every object's links, the return values, the walk
callbacks' arguments, the hook's calls, the containers' contents and the allocator log are equal: zero mismatches.

## Falsification

One instruction of the listing edited by `raw/.../mutate_gen.py` (a jump inverted, an offset moved by four, an
immediate by one, an x87 operation swapped), rebuilt, run against the oracle, restored: 55 mutations over the octree rows
(`po_mut.txt`), 40 over the queries (`pq_mut.txt`) and 3 over the loosening callback (`po_mut_leaf.txt`). Of the 98, 66
are rejected by mismatches, 29 by an access violation and 4 by a hang; three survive.

Survivors, each unobservable: `0x000e7434` (`cmp esi, 7` made 8 before the case jump, whose index never reaches 8);
`0x000e79f1` (the unsigned correction of `fild` for a negative `1 << depth`, never negative); `0x000e75e2` (the leaf
callback's `mov al, 1` made 2, only tested for zero).

Not covered: a tree whose node count exceeds 37,449 (depth above 5); `Prunable` objects (the rows see only their
members); allocation failure.
