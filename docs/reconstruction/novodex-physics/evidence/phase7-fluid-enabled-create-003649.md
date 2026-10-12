# Phase 7: enabled fluid creation path (`phys_fn_003649`)

This evidence covers a tested slice of the oracle's fluid-manager create path. It does not close the inventory row or claim full fluid reconstruction.

## Oracle path

The pinned `sub_1008A000` validates `NxFluidDesc`, constructs a `Fluid`, copies descriptor state, calls the optional `FluidModel.DLL` `FluidCreate` callback, and appends the resulting object to the manager's primary and secondary arrays. `NpScene::getNbFluids` and `getFluids` expose the primary array. Release dispatches the fluid deleting destructor and the backend `FluidDestroy` callback; manager teardown calls `FluidManagerDestroy` when the extension is initialized.

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

## Remaining work for this inventory row

The initial manager dirty flag and first static-collision build are now exercised through public `createFluid`. The shape-change dirty refresh in `phys_fn_003630` (destroy/re-fetch/rebuild during stepping), broader callback resolution and dispatch, allocation failures, descriptor validation cases, emitter interactions, and teardown combinations still need reconstruction and oracle coverage. `phys_fn_003649` therefore remains `discovered`.
