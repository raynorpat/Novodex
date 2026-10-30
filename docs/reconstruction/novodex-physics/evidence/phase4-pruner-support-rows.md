# Phase 4's remaining rows at 0x000b3d20 to 0x000f3ee4

Forty-six rows (42 Phase 4, 004799 of Phase 6, 004820 to 004824 of Phase 7) in
`Physics/src/opcode/PrunerSupportRows.cpp`, with four helper functions and two `atexit` thunks. They are naked
listing transcriptions, 2,257 instructions in 48 functions.

| Rows | What they are |
|---|---|
| 004790 to 004801 | the penetration map's file read and write (the CRT's `fopen`, `fseek`, `ftell`, `fwrite`, `fclose` with the oracle's `"rb"`/`"wb"`) and the memory stream's flushing and block appends |
| 004820 to 004828 | the box-pruning sorters: complete and bipartite pruning over `RadixSort` objects held in `.data`, and their release |
| 004830 to 004834 | the pruner process pool's creation and registration, unregistration and release |
| 004852 | the pruners' factory (a four-way switch, a compare chain here) |
| 004870, 004872 | the prunable owner adapters |
| 005231, 005262, 005263, 005279, 005448 to 005479, 005539 | the static, bounded dynamic, dynamic and type-3 pruners' remaining slots, constructors and destructors |
| 005566, 005568 | a tree walk and its entry |
| 005380, 005386, 005404 | OBB and model rows on the OPCODE side |
| 005586 to 005646 | the pruning pool's save and load through the memory stream |

`raw/phase4-pruner-support-rows-20261030/manifest.json` lists each row's calls and the `.data` it touches, as read
from the listing.

## What the transcription names

* **The oracle's statics are the candidate's own variables by name:**
  * `nxPCStatics`, now 32 words, the oracle's whole block `0x1012847c` to `0x101284fc`;
  * the three sorters (`gCompleteSorter`, `gBipartiteSorter0`/`1`, no longer in an anonymous namespace);
  * the pruner process pool (`gNxPrunerProcessPool`, no longer `static`);
  * the prunable owner and adapter hooks.
* **The oracle's tables are the candidate's**, through aliases: `StaticPruner`, `BoundedDynamicPruner`,
  `DynamicPruner`, `Type3StaticPruner` and `Pruner` (matched slot by slot), `SphereCollider` and
  `AABBTreeOfAABBsBuilder`.
* **Vendored OPCODE code is reached at its vendored symbols** from `opcode_match.csv`: the colliders' constructors,
  destructors and queries, `AABBTree::Refit2` and `BaseModel::CreateTree`. `autocfg.py` now reads those CSVs.
* **CRT routines** are the candidate CRT's: `fopen`, `fclose`, `fwrite`, `fseek`, `ftell`, `operator new`, `free`,
  `atexit` and `__chkstk`. The oracle's `_ftol2` is `PrunerOctree.cpp`'s transcription of it.
* **Helpers** (records unchanged): 004776 and 004778 (the memory stream's float and block readers, which the
  candidate's `MemoryStream` does not have), 005229 and 005477. The thunks 006332 and 006334 run the `atexit`
  release of the two containers 005262 adds to the statics.

`evidence/phase4-pruner-support-rows-listing-compare.py` finds every instruction equal, with 160 call mappings as
expected (`raw/phase4-pruner-support-rows-20261030/listing-compare.txt`). Table addresses are compared as pointers
into a section; the script does not check which table they are.

**Not wired.** The candidate's pruners, pool and box pruning keep their C++ implementations, so nothing calls these
rows. They are listed for the testing pass.

Rows with indirect calls: most of them (the SDK allocator's slots, the pruners' and colliders' own tables).

The rows await the testing pass.
