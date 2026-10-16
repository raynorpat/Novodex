# Phase 7: enabled fluid creation path (`phys_fn_003649`)

This evidence covers a tested slice of the oracle's fluid-manager create path. It does not close the inventory row or claim full fluid reconstruction.

## Oracle path

The pinned `sub_1008A000` validates `NxFluidDesc`, constructs a `Fluid`, copies descriptor state, calls the optional `FluidModel.DLL` `FluidCreate` callback, and appends the internal fluid and its public wrapper to the manager's primary and secondary arrays. `NpScene::getNbFluids` counts the primary internal-fluid array, while `getFluids` exposes the secondary public-wrapper array. Release recovers the internal object through the public wrapper's `+0x14` owner field, dispatches the internal deleting destructor, and calls the backend `FluidDestroy` callback; manager teardown calls `FluidManagerDestroy` when the extension is initialized.

The manager is initialized from the extension's exported callbacks. The test uses an explicit, minimal `FluidModel.dll` fixture staged beside each pair, so it exercises the enabled branch without depending on a local optional plugin install.

## Differential evidence

`NxPhysicsSimulationTests` checks all of the following in one run:

- An invalid descriptor returns null and reports `NXE_INVALID_PARAMETER` at `FluidManager.cpp:140`.
- The enabled manager records its initialized flags and backend handle.
- A valid descriptor returns a fluid, and the public scene list reports exactly one matching fluid.
- Releasing the sole fluid destroys and clears the now-empty manager.
- The fixture observes one each of `FluidManagerCreate`, `FluidCreate`, `FluidDestroy`, and `FluidManagerDestroy`.
- The manager's initial dirty flag drives the first static-collision build: `FluidGetCellSize` runs once, the static-collision object is fetched once and initialized once, and both refresh flags are cleared. The initial object is not destroyed before its first build.

The staged oracle/candidate differential passes with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`. The shared Phase 5, 6, and 7 coverage registry includes the exact output line:

```text
simulation fluid enabled valid=1 invalid=1.1.140 manager-flags=1 listed=1 count=1 manager-cleared=1 callbacks=1.1.1.1 refresh=1.1.0.1 dirty-cleared=1 cell-size=1 errors=0
```

Mutation checks: suppressing the candidate's `FluidCreate` dispatch produces callback counts `1.0.0.1`, exits with code 1, and yields `stdout_delta=52`; the assertion rejects it. Suppressing the initial static-collision builder produces refresh callback counts `1.1.0.0` and is likewise rejected (`candidate_exit=1`, `stdout_delta=52`). Both mutations were reverted before the final passing differential.

## Continuation: license-derived FluidCreate arguments

IDA decompilation of `sub_1008A000` and the `FluidManager` constructor shows two final `FluidCreate` arguments. The first is true only when license bit 1 is set and `simulationMethod == 2`; the second mirrors license bit 2. The manager constructor derives those bits from the product code returned by the FluidKey check (`465 -> 1`, `381 -> 2`, `804 -> 3`).

`NxSceneInternal::createFluid` now forwards those values instead of constant zeros. The registered `NxPhysicsSimulationTests` differential checks product 465 (`sparse=1`, `hardware=0`) and product 804 (`sparse=1`, `hardware=1`). Before the fix, the oracle produced these values while the candidate produced `0,0`; the test failed with `stdout_delta=62`, candidate exit 1, and oracle exit 0. After the fix, both licensed cases match, and the full staged target passes with exact stdout and stderr. Fresh Phase 5 and Phase 7 gates pass at 2,705/2,705 and 1,767/1,767 registered coverage assertions; both gates require the two output markers. The red differential, restored differential, and full gate transcripts are under `evidence/raw/phase7-fluid-create-license-flags-003649/`.

## Continuation: manager fluid-array growth and ordering

The enabled public fixture now creates seven fluids in one scene. This forces the manager's parallel internal-fluid and public-solver arrays through both capacity expansions (2, then 6, then 14 entries). The test checks public count/list order against both internal arrays, releases the third fluid and checks the oracle's swap-remove order, then releases the remaining six and verifies seven backend destroys and manager teardown without errors. Oracle and restored candidate match exactly (`stdout_delta=0`, `stderr_exact=True`). A temporary no-op mutation of `nxFluidManagerArrayPush` is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=68`). The registered marker is required by Phases 5, 6, and 7; their coverage floors are 2,706, 1,639, and 1,768 respectively. Raw staged target transcripts are under `evidence/raw/phase7-fluid-manager-array-growth-003649/`.

## Remaining work for this inventory row

The row remains open. Its initial and shape-change dirty collision refreshes are covered separately by `phase7-fluid-manager-dirty-refresh-003630.md`; descriptor validation, manager/fluid/emitter allocation failures, descriptor emitters, emitter create/list/release, and fluid-array growth/order have registered cases. The complete `003649` constructor/initialization sequence and callback table still need a branch-by-branch audit, including constructor subrows `003497`/`003503`, failure ordering, and teardown interactions. Do not promote the row until those paths have row-specific oracle evidence and the full audit verifies all branches.


## Continuation: null FluidCreate backend result

The fixture can now make the first `FluidCreate` callback return null for a valid descriptor. The oracle still returns and publishes the local fluid wrapper, stores the null backend handle, and inserts the internal/public objects into the manager arrays. Releasing this sole fluid clears the manager; `FluidDestroy` is not called for the null handle. `NxPhysicsSimulationTests` verifies the wrapper identity, both one-element manager arrays, clean teardown, and the callback counts with the row-specific marker `simulation row3649 backend-null published=1 arrays=1 cleanup=1 callbacks=3 exact=1`. A candidate early-return immediately after the null callback result is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=60`); the restored staged differential matches exactly (`stdout_delta=0`, `stderr_exact=True`). The marker is registered in Phase 5/6/7. This closes only this branch; the full FluidManager body remains open pending its instruction-level audit.

## Continuation: descriptor-validation matrix

The registered simulation fixture now checks 48 invalid descriptors. The 13 added cases cover missing velocity/life output strides, nonpositive mesh triangle size, implicit-mesh pointer/count dependencies, missing vertex/normal/index strides, and inconsistent indexed/non-indexed mesh modes. Every case must return null and report `NXE_INVALID_PARAMETER` at line 140. The earlier 35-case `desc.isValid()` bypass mutation was rejected (`stdout_delta=73`); the restored differential was exact. The full Phase 5 gate now exercises this 48-case corpus on both oracle and candidate. This verifies the existing header-backed predicate at the `FluidManager::createFluid` call site without editing public headers. It still does not close the 2,452-byte row.

## Validation

For the null-backend change before this descriptor-matrix expansion, fresh Phase 5, 6, and 7 gates passed at 2,788/2,788, 1,689/1,689, and 2,905/2,905 coverage assertions. The first Phase 5 run had one oracle child exceed its 30-second wait in the unrelated pruner-pool growth-allocation test; an isolated rerun and a complete Phase 5 rerun passed. The tool suite passed all 816 tests, immutable public-header checks passed, and inventory validation reported `unexplained=0`. Those transcripts, including the first timeout diagnostic, are in `evidence/raw/phase7-fluid-create-null-backend-003649-20261015/`.

On main, the approved `tests/PhysicsObjectLayoutTests.cpp` edit is committed with a matching source hash in `oracle-only-baselines.json`; all 14 retained oracle-proof records are rebound to the updated manifest hash. Fresh Phase 5, Phase 6, and Phase 7 gates pass at 2,788/2,788, 1,689/1,689, and 2,905/2,905 coverage assertions. A first Phase 6 run reported transient allocator-reuse mismatches; a direct object-layout rerun and a full Phase 6 rerun both passed with zero mismatches. The reconstruction-tool suite passes 831 tests and 743 subtests; both 80-file public-header trees match, and inventory validation reports `unexplained=0`. The 48-case fixture and baseline/proof updates are committed on main in `fa53527c`, `6b163856`, and `f1ee803a`. The row remains partial and open pending the remaining 2,452-byte instruction-level audit.

## Continuation: FluidCreate callback argument map

The oracle call at `0x86080` forwards fourteen non-pointer arguments from distinct `NxFluidDesc` fields, followed by its packet-size constant and two license-derived flags. The new `NxPhysicsSimulationTests` case supplies distinct valid values for the mapped descriptor fields and checks the callback fixture's captured arguments. Oracle and restored candidate match (`arguments=14`, digest `1d2e206d`, manager present). Swapping the candidate's first descriptor source from word 19 to word 18 changes the candidate digest to `de1a8fc9` and fails the target (`candidate_exit=1`, `stdout_delta=64`). The exact marker is required by Phase 5, 6, and 7. Fresh gates pass at 2,800/2,800, 1,693/1,693, and 2,922/2,922. This closes the argument-forwarding slice only; the full 2,452-byte `phys_fn_003649` row remains open.
