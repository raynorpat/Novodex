# Phase 7 — Scene fluid creation (`phys_fn_000645`)

Date: 2026-10-15
Oracle: pinned `NxPhysics.dll` SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
Candidate: source-built Win32 Release `NxPhysics.dll`; the map resolves `?createFluid@NxSceneInternal@@QAEPAVNxFluid@@ABVNxFluidDesc@@@Z` to `0x1005ff10` in `Scene.obj`.

IDA places the 137-byte oracle row at RVA `0x12b80`. It installs the Scene's FluidManager lazily with a 0x34-byte Foundation allocation, returns null if manager creation fails, calls the manager's create operation with the descriptor, attaches the Scene links to successful results, and releases an internally created fluid if its solver record was not produced. The candidate implementation is `NxSceneInternal::createFluid` in `Physics/src/Scene.cpp`.

The row-specific `NxPhysicsSimulationTests` assertion covers two contexts. With the pinned backend disabled, creation returns null, installs the manager, leaves enumeration empty, and reports the expected unavailable-feature warning. The enabled staged `FluidModel.dll` fixture covers a valid fluid and list insertion, the manager/fluid/emitter callbacks, 14 invalid descriptors, manager/fluid/emitter allocation failures, and teardown. The existing test also compares the broader emitted state against the oracle.

Mutation proof: in a throwaway candidate source build, an early return for an enabled manager forced the enabled creation result to null. The row marker reported `enabled=0` and `exact=0`; `NxPhysicsSimulationTests` rejected the candidate (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=94`). After restoring `Scene.cpp`, its SHA-256 matched the saved pre-mutation bytes. The first incremental build skipped compilation because the restored source timestamp was older than the mutated object. Touching only the restored source forced `Scene.cpp` to rebuild. The restored staged-pair control then reported `disabled=1 enabled=1 listed=1 callbacks=1 validation=14 allocation-failures=1 exact=1` on both sides and matched (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

This closes the Scene-level create row only. The separate 2,452-byte FluidManager create row `phys_fn_003649` remains partial: initial-particle paths, full callback-table handling, all failure orderings, and teardown combinations still need reconstruction. Real Unreal/FluidModel integration remains part of the later consumer audit.

Fresh Release Phase 5/6/7 gates pass at 2,787/2,787, 1,688/1,688, and 2,904/2,904 assertions. The reconstruction-tool suite passes all 816 tests. Both public-header trees pass the pinned 80-file manifest, and the inventory validator reports 6,338 functions, 5,138 data objects, and zero unexplained records. The full-image completion report remains at 2,786 open code rows. The compressed gate, differential, and build transcripts plus the mutation and restored diffs are in `evidence/raw/phase7-scene-fluid-create-000645-20261015/`.
