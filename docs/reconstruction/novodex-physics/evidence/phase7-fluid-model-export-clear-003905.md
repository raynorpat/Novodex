# FluidModel export clear (`phys_fn_003905`)

The row at RVA `0x0008e4e0` clears all 169 FluidModel function-pointer globals and returns 1. `nxFluidModelClearResolvedExports` clears the recovered export table, the candidate’s typed callback cache, and the loaded/initialized flags. FluidManager array teardown invokes the clear after the backend manager callback.

`NxPhysicsSimulationTests` resolves the 169 exports, calls the candidate implementation or the pinned oracle row directly, then checks each oracle global RVA or candidate accessor. Both restored processes report `result=1`, `cleared=169`, `callbacks_remaining=0`, and `mismatches=0`.

A candidate-only mutation retained `gNxFluidManagerCreateCallback`. The registered differential rejected it (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4104`). Restoring the clear produced an exact staged-pair result (`stdout_delta=0`, `stderr_exact=True`). Full raw mutation and restored-control transcripts are compressed under [evidence/raw/phase7-fluid-model-export-clear-003905-20261016](raw/phase7-fluid-model-export-clear-003905-20261016/).

Fresh Phase 5, 6, and 7 gates pass at 2,827/2,827, 1,716/1,716, and 2,960/2,960 assertions. Each full gate transcript is retained as a compressed raw log alongside the targeted mutation and restored differential transcripts.

All 817 reconstruction-tool unit tests pass after regenerating the completion snapshot and updating its closure-ledger hashes. Inventory validation reports 6,338 function rows, 5,138 data objects, and zero unexplained entries.
