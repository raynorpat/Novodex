# FluidModel cached module unload (`phys_fn_003902`)

IDA's body at RVA `0x0008d850` calls `FreeLibrary` with the cached `HMODULE`, clears the cached handle regardless of the call result, and returns the success flag. `nxFluidManagerResolveModel` now preserves the handle returned by the existing loader. `nxFluidModelUnloadLibrary` implements the row, and FluidManager teardown calls it immediately after `nxFluidModelClearResolvedExports`.

The registered `NxPhysicsSimulationTests` fixture resolves all 169 exports, clears them, invokes oracle RVA `0x8d850` or the candidate row, and checks both the BOOL result and module presence. Restored oracle and candidate output report `result=1 module_after=0 matched=1 mismatches=0`; the staged differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

A candidate-only mutation that skips `FreeLibrary` is detected: the candidate reports `result=0 module_after=1 matched=0 mismatches=1` and the differential rejects it (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4104`). Raw mutation/restored logs and all three phase-gate transcripts are compressed under [evidence/raw/phase7-fluid-model-unload-003902-20261016](raw/phase7-fluid-model-unload-003902-20261016/).
