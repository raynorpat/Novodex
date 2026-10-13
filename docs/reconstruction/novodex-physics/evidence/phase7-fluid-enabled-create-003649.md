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
