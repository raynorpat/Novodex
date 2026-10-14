# `NxBodyVisualRecord::visualize` mutation proof

Date: 2026-10-14
Row: `phys_fn_000766` at RVA `0x000179a0`
Gate: registered `NxPhysicsSceneVisualizeTests` staged-pair differential (Phase 7)

The function emits configured body-axis lines, mass-axis boxes, and linear/angular velocity arrows. A throwaway immediate-return mutation omitted the whole body visualization call. The registered differential rejected the candidate with both processes exiting 0, `stdout_delta=2768`, and exact stderr.

After restoring the function and rebuilding, the differential matched exactly (`stdout_delta=0`, `stderr_exact=True`). The restored fixture still reports the expected three contact lines. Raw mutant and restored logs are compressed under `evidence/raw/phase7-body-visualization-000766-20261014/`.
