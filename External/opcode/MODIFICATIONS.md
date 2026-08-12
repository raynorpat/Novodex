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
| `OPC_RayCollider.h` | one 4-byte member added between `mMaxDist` and `mClosestHit` | `0x000b5770` (+0x84), `0x000b579d` (+0x8c) |
| `OPC_TreeBuilders.h` | `BuildSettings` 8 → 20 bytes; `AABBTreeBuilder` gains 28 bytes between `mNodeBase` and `mCount` | `0x000e92b0`, `0x000e9060`, `0x000e91cc`, `0x000e9100` |
| `OPC_BaseModel.h` | `OPCODECREATE` gains `mDeserializeFrom` at +0x04 (sizeof 16 → 32); `BaseModel` gains three virtuals at slots 4/5/6 | `0x000e9122`, `.rdata:0x0011bac8`, `.rdata:0x0011bb5c` |
| `OPC_BaseModel.cpp` | stub bodies for those three virtuals; `mSource`/`mTree` through the host allocator | `0x000e9420`, `0x000e9440`, `0x000e94c0`, `0x000e949c` |
| `OPC_OptimizedTree.h` | `AABBOptimizedTree` gains the same three virtuals, **inserted** at 4/5/6, displacing `GetUsedBytes` from slot 4 to slot 7 | `.rdata:0x0011bc4c`, `0x000e90c0`, `0x000e9420`, `0x000e945f`, `0x000e953d` |
| `OPC_IceHook.h` | `SetIceError` becomes a reporter carrying `__FILE__` and `__LINE__` | `0x000539b0`, `0x000e903b`, `0x000e912f` |
| `OPC_Model.cpp` | the `mDeserializeFrom` guard around the `mLimit` check and `CheckTopology`, plus the loader dispatch; `AABBTree` through the host allocator | `0x000e9122`, `0x000e912f` (line 147), `0x000e9161`, `0x000e9191` |
| `Ice/IceContainer.cpp` | the borrowed-buffer guards — `Empty()` frees only when `mGrowthFactor >= 0.0f`, `Resize()` returns false unless `> 0.0f`; allocation through the host allocator | `0x000b4d93`, `0x000b4e93`, `0x000b4f53`, `0x000b4de7`, `0x000b4fd5` |
| `Ice/IceRevisitedRadix.h` | `mDeleteRanks` added at +0x14; `SetRankBuffers` declared | `0x000e32c0`, `0x000e3ea0` |
| `Ice/IceRevisitedRadix.cpp` | the destructor and `Resize` free only when `mDeleteRanks`; allocation through the host allocator; **`SetRankBuffers`, the added member that clears the marker, reconstructed by P4 Task 2b** | `0x000e32e3`, `0x000e3333`, `0x000e3ea0` |
| `OpcodeNovodeXHost.h` | **added file**, no upstream counterpart: the allocation and error-reporting seam | `0x000b4000`, `0x000539b0` |

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
