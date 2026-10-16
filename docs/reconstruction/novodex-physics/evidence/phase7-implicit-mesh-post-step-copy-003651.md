# Implicit-mesh post-step copy (`phys_fn_003651`)

`NxPhysicsSimulationTests` now exercises the real licensed fluid surface-mesh path. The staged `FluidModel.dll` supplies one indexed stream, one vertex position, and one normal through `ImplicitFluidMeshGetMesh`; the test seeds the public `NxMeshData` buffers, advances and fetches one simulation step, then checks all three streams and both output counts against the oracle fixture values.

The control is exact: oracle and candidate each report `matched=1`, with vertex count 1, triangle count 1, and matching position, normal, and index words. A candidate mutation that makes `nxFluidCopyImplicitMeshData` return immediately leaves the candidate buffers and counts zero. The registered differential rejects it (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=66`, `stderr_exact=False`). Restoring the function returns the staged differential to exact output (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

The registered Phase 5, 6, and 7 coverage floors advance by one assertion each. This closes the row-specific behavior slice; the broader full-DLL reconstruction and Phase 8 census remain open.

Raw control and mutation transcripts are retained in `evidence/raw/phase7-implicit-mesh-post-step-copy-003651-20261016/`.
