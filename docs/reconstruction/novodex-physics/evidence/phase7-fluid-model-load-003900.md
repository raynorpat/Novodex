# Optional FluidModel loader — row 003900

IDA identifies `phys_fn_003900` at RVA `0x8d820` as the `FluidModel.DLL` load helper. It saves and restores the x87 environment around `LoadLibraryA`, stores the module handle, and returns whether the load succeeded. The CMake implementation is integrated in `nxFluidManagerResolveModel` in `Physics/src/Scene.cpp`.

The staged `NxPhysicsSimulationTests` fluid lifecycle fixture prints a row marker after manager callbacks are observed. A second probe runs in a fresh child process, enables a fixture `DllMain` that changes the x87 control word during `DLL_PROCESS_ATTACH`, and verifies the loader restores the caller's state:

```text
simulation fluid-model-load row=003900 module=1 callbacks=1
simulation fluid-model-load-x87 row=003900 loaded=1 initialized=1 staged=1 control_before=027f control_after=027f matched=1
```

Replacing the candidate load result with null is mutation-rejected: the lifecycle marker becomes `module=0 callbacks=0`, the enabled fluid lifecycle fails, and the staged differential reports `oracle_exit=0`, `candidate_exit=1`, `stdout_delta=118`. A separate mutant replaces `fldenv` with `fninit`; the x87 probe observes `control_after=037f` instead of `027f` and the differential exits 1 for the candidate. The restored implementation saves the 28-byte x87 environment with `fstenv`, loads the optional DLL, then restores the environment with `fldenv`. The child limits lookup to the staged pair and checks the loaded module path, requires resolver success, and fails after 30 seconds if it hangs. Both probes pass against the staged oracle and candidate with `stdout_delta=0` and exact stderr.

Phase 5/6/7 gates, the full reconstruction-tool suite, and evidence manifests are recorded in [`evidence/raw/phase7-implicit-mesh-loader-003900-20261016/`](raw/phase7-implicit-mesh-loader-003900-20261016/).

The fresh gates pass at 2,825/2,825, 1,714/1,714, and 2,955/2,955 coverage assertions. The full reconstruction-tool suite passes 835 tests and 743 subtests; inventory validation reports zero unexplained records.
