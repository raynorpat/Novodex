# OPCODE 1.3 — what NovodeX changed

Every entry below is a file under `novodex/` that replaces the file of the same
name under `upstream/Opcode/`. Diff the two to see the change; the file's own
`NOVODEX LOCAL MODIFICATION` header carries the addresses that establish it.

None of this is a style preference or a port fix: the pinned trees **compile
unmodified** under a 2026 MSVC. Every one of these exists because the shipped
`NxPhysics.dll` does something stock 1.3 does not.

| file | change | established at |
|---|---|---|
| `OPC_Settings.h` | `OPC_RAYHIT_CALLBACK` switched off; stock ships it defined | `0x000b5770`, `0x000b57ab`, `0x000b57b9` |
| `OPC_RayCollider.h` | one 4-byte member added between `mMaxDist` and `mClosestHit`: a `float`, the culling-arm barycentric tolerance (identified in vendored-correspondence Task 3) | `0x000b5770` (+0x84), `0x000b579d` (+0x8c), `0x000b873b` |
| `OPC_RayCollider.cpp` | the constructor clears the added member (vendored-correspondence Task 3) | `0x000b5736` |
| `OPC_RayTriOverlap.h` | the culling arm accepts U, V >= -tolerance and U, U+V <= det + tolerance, with the tolerance read from the added member; stock rejects on the sign bit and on `det` exactly. V is tested and summed unrounded (a double register lifetime) and only the stored `mStabbedFace.mV` is float, as the image's `fcom; fst [esi+0x58]; fadd st(1)` does (vendored-correspondence Task 3 and its review fix: the candidate had kept the stock tests; the block is inlined at 14 sites) | `0x000b873b`, `0x000b8765`, `0x000b87d9`–`0x000b87ef`; the 14 sites `0x000b5aef`, `0x000b5f75`, `0x000b65be`, `0x000b6bc3`, `0x000b7113`, `0x000b755d`, `0x000b7b30`, `0x000b7f7a`, `0x000b873b`, `0x000b8d50`, `0x000b92d4`, `0x000b9715`, `0x000b9d1d`, `0x000ba15e` |
| `OPC_TreeBuilders.h` | `BuildSettings` 8 → 20 bytes — an extend value (`float`), an extend axis (`sdword`, −1 = off) and an inflate margin (`float`); `AABBTreeBuilder` gains 28 bytes between `mNodeBase` and `mCount` — the root node's captured `AABB` and a one-shot latch; `AABBTreeBuilder` carries the host-allocator class operators (Task 3) | `0x000538c0`, `0x000e92b0`, `0x000e9060`, `0x000e91cc`, `0x000e9100`, `0x000f0ec0`, `0x000f0efe`, `0x000f0f83`, `0x000f0ed7`, `0x000f1187` |
| `OPC_AABBTree.cpp` | `AABBTree::Build` arms the one-shot capture latch; `AABBTreeNode::_BuildHierarchy` gains a block between `ComputeGlobalBox` and `Subdivide` that extends the node's box to a plane along the added axis and then inflates it by the added margin; `mIndices` (a `udword` array) allocated and freed through the host allocator (Task 3) | `0x000f1187`, `0x000f0ec0`–`0x000f1026`, `0x000f10f0`, `0x000f109f` |
| `OPC_AABBTree.h` | `AABBTreeNode` (and so `AABBTree`) carries the host-allocator class operators (Task 3) | `0x000f1147`–`0x000f1170`, `0x000f0890`, `0x000e9191` |
| `OPC_BaseModel.h` | `OPCODECREATE` gains `mDeserializeFrom` at +0x04 (sizeof 16 → 32); `BaseModel` gains three virtuals at slots 4/5/6; `BaseModel` carries the host-allocator class operators (Task 3) | `0x000e9280`, `0x000e95a0`, `0x000e9122`, `.rdata:0x0011bac8`, `.rdata:0x0011bb5c` |
| `OPC_BaseModel.cpp` | stub bodies for those three virtuals; `mSource`/`mTree` are stock `new`/`DELETESINGLE` again, reaching the allocator through the class operators (Task 3: the explicit helpers called a tree's destructor where the image calls its virtual deleting destructor, `0x000e931e`); `OPCODECREATE::OPCODECREATE` clears `mDeserializeFrom` (fixed in vendored-correspondence Task 3: it was left uninitialised while `Model::Build` branches on it); the `Save`/`Load` stubs report through the SetIceError seam before failing (Task 3 review) | `0x000e9420`, `0x000e9440`, `0x000e94c0`, `0x000e949c`, `0x000e92d2` |
| `OPC_OptimizedTree.h` | `AABBOptimizedTree` gains the same three virtuals, **inserted** at 4/5/6, displacing `GetUsedBytes` from slot 4 to slot 7; the four node classes and `AABBOptimizedTree` carry the host-allocator class operators, and the node constructors are empty where stock zeroes the child words (both Task 3) | `0x000f24c4`, `0x000f24df` (constructor `0x00027f00` = `mov eax,ecx; ret`), `0x000f3fb0`, `.rdata:0x0011bc4c`, `0x000e90c0`, `0x000e9420`, `0x000e945f`, `0x000e953d` |
| `OPC_SweepAndPrune.h` | `SweepAndPrune::Init` takes a third argument, a per-object byte array that filters the starting pairs (Task 3) | `0x000e7163`, `0x000e7123` |
| `OPC_SweepAndPrune.cpp` | the SAP element, box and end-point classes and `mArray` through the host allocator; `Init` rejects a zero count or null boxes, frees the previous arrays, keeps its scratch on the stack and filters pairs by the added array; the constructor zeroes and the destructor frees the boxes and lists (all Task 3) | `0x000e6480`, `0x000e6c10`, `0x000e6caa`, `0x000e6cf9`, `0x000e7123`, `0x000e7180`, `0x000e71b0` |
| `OPC_IceHook.h` | `SetIceError` becomes a reporter carrying `__FILE__` and `__LINE__` | `0x000539b0`, `0x000e903b`, `0x000e912f` |
| `OPC_LSSCollider.h` | `LSSCollider` keeps the radius (+0x70) and three precomputed segment Points (+0x4c half direction, +0x58 its absolute value, +0x64 centre) between `mSeg` and `mRadius2`, which moves to +0x74 (Task 3) | `0x000d34e2`, `0x000d34f6`, `0x000d3a1d`, `0x000d3750` |
| `OPC_LSSCollider.cpp` | `InitQuery` stores the radius and, on the path that goes on to a query, fills the three Points (Task 3); `InitQuery` and the hybrid `Collide` take the result `Container` through the cache's pointer (vendored-correspondence Task 5a; see `OPC_VolumeCollider.h`) | `0x000d34df`, `0x000d39ee`–`0x000d3aa9`, `0x000d36f2` |
| `OPC_LSSAABBOverlap.h` | `LSSAABBOverlap` is RayCollider's segment-box separating-axis test against the box inflated by the radius, not the exact segment-box squared distance (Task 3) | `0x000d3ad0`–`0x000d3bde` |
| `OPC_AABBCollider.cpp` | `_Collide(const AABBTreeNode*)`, the vanilla-tree walk, passes the node's extents and centre to `AABBAABBOverlap` in the order every other walk does; stock passes them swapped (found by the vendored-correspondence Task 4 differential, `opcode_aabb_vanilla`); `InitQuery` and the hybrid `Collide` take the result `Container` through the cache's pointer (Task 5a; see `OPC_VolumeCollider.h`) | `0x000eee90`, `0x000eeee1`, `0x000eef35`, `0x000eef44`, `0x000e9c00` |
| `OPC_VolumeCollider.h` | `VolumeCache` holds a `Container*` at +0 and the model at +4 (8 bytes), where stock embeds the `Container` (20 bytes); so each derived cache's own fields start at +8. The constructor nulls both words, and the cache's owner points it at a `Container` of its own; the colliders never allocate one and never test the pointer. Found by the vendored-correspondence Task 4 differential, fixed in Task 5a | the five `InitQuery` loads `0x000d36f2` (LSS), `0x000d57ab` (OBB), `0x000de925` (Sphere), `0x000e173d` (Planes), `0x000e9c00` (AABB); `SphereCache` fields at +4/+8/+0x14/+0x18 (`0x000dea64`, `0x000dea87`, `0x000dea58`, `0x000dea7f`); the owner at `0x000e56d0` (`0x000e56ef`–`0x000e570c` null, `0x000e5731`/`0x000e5734` point at its `Container`) |
| `OPC_SphereCollider.cpp`, `OPC_OBBCollider.cpp`, `OPC_PlanesCollider.cpp` | `InitQuery` takes the result `Container` through the cache's pointer, and the hybrid `Collide` resets and adopts it through the pointer (Task 5a); nothing else changed | `0x000de925`, `0x000d57ab`, `0x000e173d` |
| `OPC_Model.cpp` | the `mDeserializeFrom` guard around the `mLimit` check and `CheckTopology`, plus the loader dispatch; `new AABBTree`/`DELETESINGLE(mSource)` stock again, reaching the allocator through `AABBTreeNode`'s class operators (Task 3) | `0x000e9122`, `0x000e912f` (line 147), `0x000e9161`, `0x000e9191` |
| `Ice/IceContainer.cpp` | the borrowed-buffer guards — `Empty()` frees only when `mGrowthFactor >= 0.0f`, `Resize()` returns false unless `> 0.0f`; allocation through the host allocator | `0x000b4d93`, `0x000b4e93`, `0x000b4f53`, `0x000b4def`, `0x000b4fd5` |
| `Ice/IceContainer.h` | `CONTAINER_STATS` not defined: no global container count or RAM total (vendored-correspondence Task 3; stock defines it and the candidate had compiled it in) | `0x000b4d70`, `0x000b4f00`, `0x000b4f50`, `0x000b4d90`, `0x000b4de0`, `0x000b4e90` |
| `Ice/IceSegment.cpp` | `Segment::SquareDistance` retains wide point deltas and stores only the oracle's selected intermediate products as floats | `0x000f0560`–`0x000f0659` |
| `Ice/IceRevisitedRadix.h` | `mDeleteRanks` added at +0x14; `SetRankBuffers` declared | `0x000e32c0`, `0x000e3ea0` |
| `Ice/IceRevisitedRadix.cpp` | the destructor and `Resize` free only when `mDeleteRanks`; allocation through the host allocator; **`SetRankBuffers`, the added member that clears the marker, reconstructed by P4 Task 2b** | `0x000e32e3`, `0x000e3333`, `0x000e3ea0` |
| `OpcodeNovodeXHost.h` | **added file**, no upstream counterpart: the allocation and error-reporting seam, and (Task 3) `OPC_NOVODEX_ALLOCATEABLE`, the four class operators that route a class's storage through it | `0x000b4000`, `0x000539b0`, `0x000f0890`, `0x000ba6c0` |

**Build parity, not source: `OPC_SphereTriOverlap.h`.** The image evaluates the
edge-region distances `u = -fB0/fA00; SqrDist = fB0*u + fC` (and the fB1/fA11
twins) as `(-1.0/fA)*fB*fB + fC` with nothing rounded to float, and keeps
`SqrDist` on the x87 stack to the final `fabs; fcomp [mRadius2]`
(`0x000de623`, `0x000de710`, `0x000de7c1`). The overlay writes that form with
`SqrDist` a double (a register lifetime); stock divides and rounds `u` to float
first. Like qhull's `geom.c` reciprocals this is the 2003 compiler's arithmetic,
not a NovodeX change (vendored-correspondence Task 3 review fix).

**Build parity, not source.** `External/CMakeLists.txt` compiles `NxOpcode` with
`/Qfast_transcendentals` (vendored-correspondence Task 3): the image's OPCODE
square roots are all inline `fsqrt` (`0x000e3274`), which follows the x87 control
word; the UCRT's `__CIsqrt` a 2026 `/fp:precise` build calls instead takes an
SSE2 path that ignores it, and OPCODE runs under both `0x027f` and `0x0f7f`.

**This list is not proven closed.** It is every modification this project has
established, each with the address that establishes it, and nothing more.
`OPC_AABBTree.cpp` is the reason the qualifier is here: it was absent from an
earlier version of this table, which was presented as exhaustive, while the
stock file compiled into `NxOpcode` and the image's `AABBTree::Build` and
`AABBTreeNode::_BuildHierarchy` carried NovodeX code neither Task 1d nor Task 2a
had disassembled. The search had stopped at `Model::Build`, which *constructs*
the builder, instead of following the one further call edge to the functions
that *use* it. Nothing about the method used to build this table would have
caught it, so nothing about the table can promise the next one.

## The two that bite hardest

**`IceContainer.cpp`.** Vendoring the stock file compiles, links, runs, and
double-frees at every borrowed-buffer site. The marker is `mGrowthFactor` set to
`-1.0f`, and the two guards are *not the same test*: `Empty`, `SetSize` and
`~Container` free at exactly `0.0f` and `Resize` does not. `0x000b4f50` has 39
direct callers spanning Phases 2 to 8 and `0x000b4de0` has 115.

**The vtable shapes.** A stock `OPC_BaseModel.h` gives `Model` four slots where
the image has seven, and a stock `OPC_OptimizedTree.h` gives the tree five where
the image has eight with `GetUsedBytes` displaced to slot 7. Both compile, link
and run, and every indirect call goes to the wrong function.

## Claims checked and found to be STOCK — do not "restore" these

| claim | verdict |
|---|---|
| a single-triangle short circuit in `Model::Build` at `0x000e917b` was added by NovodeX | **stock.** `OPC_Model.cpp:157-165`, announced in `ReadMe.txt:24`'s "New in Opcode 1.3" list. The compiled stock bytes `83 fb 01 / 75 0f / 83 4e 08 04` are the image's bytes |
| `Log` was replaced alongside `SetIceError` | **stock.** `Model::Build`'s `if(NbDegenerate) Log(...)` compiles to nothing in the image: `0x000e9150` calls `CheckTopology` and `0x000e9155` goes straight to `Release` with no test and no call between them |
| `sizeof(AABBTree)` changed | **stock.** `0x000e919a` allocates `0x30` = 48, and a stock compile gives 48 |

## Not applied here — still nobody's

The bodies of the three added virtuals (`0x000e9420`, `0x000e9440`,
`0x000e94c0` and the instantiations behind the four tree classes),
`Container::setExternalBuffer` (`0x000b4f90`) and `IceAdjacencies.cpp` are
NovodeX code with no upstream counterpart. What is applied here is the interface
each of them needs — the vtable slot, the member, the marker — because a stock
header gets those wrong silently. The bodies that appear under `novodex/` are
marked `NOT RECONSTRUCTED` and return zero.

**`RadixSort::SetRankBuffers` (`0x000e3ea0`) is no longer on that list.** P4
Task 2b reconstructed it into `novodex/Ice/IceRevisitedRadix.cpp`, and
`NxPhysicsThirdPartyTests`' `radix_setrankbuffers` family drives it against the
shipped row.

**`IcePrunable.cpp` is not on it either, and never belonged under `novodex/`.**
No `Prunable` exists in either pinned OPCODE tree, so it is not a modification of
OPCODE at all; NovodeX filed it under `src\opcode\` by directory convention. P4
Task 2b reconstructed it at `Physics/src/opcode/IcePrunable.cpp`, on the host
side, which keeps this directory an answer to "what did NovodeX change in
OPCODE?" rather than a place NovodeX's own files accumulate.

## Two divergences found while fixing `OPC_AABBTree.cpp`

**The allocator seam in `OPC_AABBTree.cpp` is compiler-generated, not source.**
*Resolved by vendored-correspondence Task 3; the paragraph below is the original
finding.* The image draws the allocator line by class: the model, builder, tree
and node classes reach the singleton for every `new`/`delete` of them, including
the compiler-generated ones, while the colliders' deleting destructors and
PlanesCollider's `Plane` array use the CRT's `operator delete`/`operator new[]`
(`0x000ba6c0`, `0x000e159a`). Class-scope operators
(`OPC_NOVODEX_ALLOCATEABLE` in `OpcodeNovodeXHost.h`) reproduce that, the
explicit `opcNovodeXNew`/`opcNovodeXDelete` helpers are gone, and only the one
primitive array, `AABBTree::mIndices`, is converted at its sites.
`0x000f114c` allocates `36n+4` bytes through the host allocator, writes the
array cookie at `0x000f1170` and runs MSVC's vector constructor iterator — which
is `new AABBTreeNode[n]` with a *replaced `operator new[]`*, not an edited `new`
expression. `0x000f0890` is the matching vector deleting destructor and
`0x000f10ac` is `DELETEARRAY(mIndices)`. There is no site in that source text to
convert. The same question hangs over the four files where entry 13 of the table
above *was* applied at the `new` sites (`0x000e919e` is `new AABBTree` in the
same compiler-generated shape); that is not re-opened here, because those files
carry green differentials and re-deriving the seam is its own task.

**`IceAABB.h`'s representation is min/max in the image and in this build.**
`External/CMakeLists.txt` now defines `USE_MINMAX` on `NxOpcode` and propagates
it to direct consumers of `Opcode.h`. The image's `AABB::Add` at `0x000e2d20` reads `[this+0]`,
`[this+4]`, `[this+8]` straight into the min with no subtraction, where
centre/extents would emit `mCenter[i] - mExtents[i]`, and
`_BuildHierarchy`'s inflate at `0x000f0f8e` subtracts the margin from the first
`Point` and adds it to the second, which is meaningless on a centre. `sizeof` is
24 either way, so the registered `sizeof_AABB` check cannot see it. The
`complete_pruning` oracle differential at `0x000b4530` revealed the mismatch:
its first min/max direct-field drive differed when both sides received a
centre/extents object, then matched exactly after `USE_MINMAX` was propagated.
This changes all Opcode AABBs, so other object and asset gates still require
regression checks; the single helper's exact result is not proof of them.
