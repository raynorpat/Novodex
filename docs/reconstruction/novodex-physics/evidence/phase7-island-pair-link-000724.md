# Sleep-group island pair linking (`phys_fn_000724`)

Date: 2026-10-14

Row: `phys_fn_000724` at RVA `0x000161b0`

Gate: registered Phase 7 `NxPhysicsSimulationTests` staged-pair differential

The row increments member/root pair counters, pushes the new contact pair onto the selected sleep-group list, and merges the two groups when a partner body is supplied. An immediate-return mutation in a clean Win32 Release archive is rejected by the existing simulation target (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=6606`, `stderr_exact=True`). The mutation changes the triangle-mesh box-contact result and broadphase/stack trajectories; the public simulation fixture therefore exercises the row through the reconstructed Scene step.

The archive source was restored to the repository source bytes (SHA-256 `4c02a1a7e5d7321d9bbead4b5c4b7e7cd5efdb4a1956d28d490891395734995a`), the DLL was rebuilt, and the registered oracle differential returned an exact control (`stdout_delta=0`, `stderr_exact=True`). Raw mutant, restored build/differential logs, source hashes, and the row-only mutation are under `evidence/raw/phase7-island-pair-link-000724-20261014/`.

No production source or public header changed for this closure; row 000724's existing reconstruction is now dynamically falsified. The latest full Phase 7 gate on this same source and test tree passed at 1,798/1,798 assertions after the fast-forward to main (`958d847c`); its transcript is preserved under the adjacent row 000720 evidence folder.
