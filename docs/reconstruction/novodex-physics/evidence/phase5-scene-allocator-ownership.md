# Scene, actor and shape allocator ownership — 2026-09-26

The candidate allocated the Scene, its actors, shapes, dynamic records, groups,
NpScene's lock objects and the Scene's auxiliary manager through
`nxGetSdkAllocator()` (phys_fn_004803, the `SdkAllocatorBridge` to the allocator
passed to `NxCreatePhysicsSDK`, or the CRT when none). The pinned oracle
allocates all of them through the imported `nxFoundationSDKAllocator`
(`mov reg,[0x101041bc]`, malloc slot `+8` with hint 0 = `NX_MEMORY_PERSISTENT`,
free slot `+0x14`). This packet moves every such site, malloc and free together,
onto `nxFoundationSDKAllocator`, and leaves the sites the oracle routes through
004803 where they were. No public header changed.

## Method

Static, from `oracle/capstone/manifest.json`. Every decoded instruction whose
operand names `0x101041bc` is a Foundation read (530); every direct `call` to
`0x000b4000` is a 004803 call (286). Each was assigned to its inventory row and
paired with the next indirect call, whose slot gives malloc or free (Foundation
`+8`/`+0x14`; bridge `[reg]`/`+0xc`) and whose preceding pushes give the size
when it is an immediate. 004803 calls occur only in 0x29000–0x33000,
0x4b000–0x56000, 0xb3000–0xb5700 and 0xe3000+; Foundation reads cover
0x1000–0x28e80 and 0x5a000–0x5c400 with no 004803 call, and also appear in
0x4f000–0x55d00 and 0x85000–0xb3a00, so the ranges are per-row rather than
exclusive.

## Sites moved to nxFoundationSDKAllocator

| Candidate | Oracle row (all Foundation) |
|---|---|
| `PhysicsSDK::createScene` 0x710 + failure free | 000476 (0x0000eaf2 `push 0x710`) |
| `NxSceneInternal` aux manager 0xa8 | 000647 (0x00012fcd `push 0xa8`) |
| `nxSceneArrayReserve` (0x55c, 0x56c, 0x6d4, 0x6e8, 0x6fc arrays) | 000651, 000626, 000630 inline growth |
| aux staged 0x800/0x400 arrays (records and shapes) | 002417, 002421, 002423 (0x5bd2b…0x5c354) |
| `nxSceneMarkShapeDirty`, `nxNpActorMarkRecordDirty` growth | 001325 / 000785 |
| `createActor` 0x50 body, 0x18 wrapper | 000626 (0x00011de4), 000013 (0x000014cc) |
| `releaseActor` wrapper/record/shape/helper/body frees | 000628, 000503, shape deleting rows |
| `createJoint` joint block | 000665 |
| `nxSceneUpdateActorCount` 0x400 cache arrays | 000503 (0x000100a0) |
| dynamic record 0x260 | 000026 (0x00001b47) |
| shape 0x228, public handle 0x1c | 000032 (0x00001ebb); 000977/000987/001247/001349/001379 |
| group 0x110 and its child arrays | 000034 (0x00002097, 0x00002148), 000036 (0x0000236f) |
| Scene teardown: +8/+0xc, aux arrays, aux, 0x6fc/0x6e8/0x6d4, 0x56c/0x55c, self | 000663 (Scene destructor, 15 Foundation frees, no 004803 call) |
| `NpScene` links 4/4, condition 0x18 | 000285 (0x0000c365, c387, c3a9) |
| `NpScene` lock blocks 0x20, condition state 0x14, destructor frees | 002358 (0x0005b6b0), 002377 (0x0005b836), 000281/002360/002392/002402 |
| kinematic 0x20 transition block | 000785 (0x0001996a) |
| shape/actor name table 0x10 and entries | 000480 (0x0000edfc), 000478/000484 |
| `ObjectModel.cpp` deleting destructors, collision objects, `nxU32VectorPushBack` | 001079, 000118, 002326, 002328, 002340, 001263, 001375, 001014, 000979, 001399, 000028 |

## Sites kept on phys_fn_004803

| Candidate | Oracle row |
|---|---|
| OPCODE pool 0x1c and its 8/4/4/4 buffers | 004830 (0x000b4ce0), 005438 (0x000ef270) |
| static 0x90 and dynamic 0x3c pruners | 004852 (0x000b50b1 `push 0x3c`, 0x000b50ee `push 0x90`) |
| pruner 0x60/0x10 entry/reference buffers and their growth | 005481 (0x000effc0) |
| pending-shape array at Scene+0x69c..0x6a4 | 001943 grows +0x624's +0x78 header via 004840 (0x0004bbad) |
| Scene teardown of +0x640/+0x648 tables and the +0x6a4 array | same owners |
| `Containers.cpp`, `MemoryStream.cpp`, `ThirdPartyHost.cpp` | SdkContainer / 0xb3000 range, unchanged |

The Scene destructor loop that freed the 0x6fc, 0x6e8, 0x6d4 and 0x6a4 arrays
through one allocator was split, so no block is freed through the allocator
that did not allocate it. The comment on `nxSceneArrayReserve`, which already
said "Foundation allocator" while the code called 004803, is now true and names
the rows.

## Test

`NxPhysicsSceneAllocatorTests` (`tests/PhysicsSceneAllocatorTests.cpp`) creates
the Foundation with counting allocator A, then calls `NxCreatePhysicsSDK` with
counting allocator B. Both fill blocks with 0xCD and tag them with their owner.
For each phase (foundation_create, sdk_create, scene_create, static_create,
dynamic_create, multi_create, multi/dynamic/static_release, scene_release,
sdk_release) it prints per-allocator counts, the allocation and free size lists,
and `cross_frees`, the number of blocks freed through the wrong allocator.

Static prediction, not a measurement: `scene_create` puts all ten blocks the
registered `actor empty_scene_alloc_sizes=710.28.4.20.4.20.18.14.a8.8` line
records on A and none on B, and every phase reports `cross_frees=0`. The static,
dynamic and multi creations put the pruner, OPCODE pool, entry/reference
buffers and pending array on B and everything else on A.

## Open

This packet was prepared in a Linux container without the pinned oracle DLL or
a Win32 toolchain. The harness has not been run against either DLL, so no lines
are registered in `tools/gate_targets.ps1`, no floor changed and no Python pin
moved. The existing staged harnesses pass one allocator to
`NxCreatePhysicsSDK` and create no Foundation of their own, so both allocators
there are the same user allocator; their registered lines should not move.
Before gating: build both pairs, run the new target against each, confirm
`stdout_delta=0`, register the oracle's lines on Phase 5 and raise its floor by
their count, and re-run gates 2–7 (5 expected to fail only on its vtables
marker).
