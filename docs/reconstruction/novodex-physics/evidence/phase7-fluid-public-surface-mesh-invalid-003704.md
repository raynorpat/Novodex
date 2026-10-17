# Public fluid surface-mesh invalid descriptor path — row 003704

The existing licensed fixture proves the successful path for `phys_fn_003704` (RVA `0x0008b250`). This continuation covers an invalid `NxImplicitMeshDesc`. IDA's public wrapper checks licensing, obtains the Scene write lock, and blocks API reentry before calling the internal initializer. The initializer validates the descriptor and, on failure, reports `NXE_INVALID_PARAMETER` from `Fluid.cpp:278` and returns null. The public wrapper then unconditionally reads the returned manager at offset `0x38`, so the pinned DLL raises access violation `0xc0000005`.

`nxFluidInitializeSurfaceMesh` now follows the same validation/error order, including releasing an existing manager only after validation and license checks. `nxFluidCreateSurfaceMeshPublic` retains the wrapper's unconditional manager read. No public Physics headers changed.

The registered `NxPhysicsSimulationTests` fixture isolates this invalid call in a child process so the expected oracle fault cannot terminate the parent suite. Both pinned oracle and restored candidate reach the call, fault with `0xc0000005`, and report:

```text
simulation fluid public-surface-mesh invalid-child row=003704 exit=c0000005 reached=1 returned=0 exact=1
```

The full staged-pair differential passes (`oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True`). A candidate-only mutation that returned null safely from the public wrapper changed the child outcome to normal exit (`exit=00000000 reached=1 returned=1 exact=0`); the differential rejected it (`candidate_exit=1`, `stdout_delta=82`). Restoring the oracle fault behavior returned the differential to exact.

Fresh Phase 5, Phase 6, and Phase 7 gates pass with 2,835/2,834, 1,720/1,719, and 2,968/2,967 coverage assertions, respectively. The first Phase 6 run had a transient allocator-reuse mismatch in `NxPhysicsObjectLayoutTests`; the same target passed on direct retry, and the complete fresh Phase 6 rerun passed. Raw focused differentials, mutation, and gate transcripts with hashes are in [`evidence/raw/phase7-fluid-public-surface-invalid-003704-20261017/`](raw/phase7-fluid-public-surface-invalid-003704-20261017/).

This proves the invalid-descriptor arm only. The already documented valid licensed arm and this path do not close all behavior in `phys_fn_003704`; full DLL reconstruction remains active.
