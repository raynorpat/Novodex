# Actor force accumulator (`phys_fn_000782`)

The registered `NxPhysicsActorForceTests` differential covers force and torque accumulation for all five `NxForceMode` values, including dirty flags and both linear/angular accumulators. The clean oracle/candidate run has `stdout_delta=0` and exact stderr.

For the mutation control, the candidate impulse-mode X term `inverseMass * force->x` was replaced with zero and `NxPhysics.dll` rebuilt. The mode-1 linear accumulator changed and the registered target rejected the candidate (`stdout_delta=18`). Restoring the impulse scaling and rebuilding returned an exact differential.

The target is part of the passing Phase 5 gate, so this shared Phase 2 row is discharged by Phase 5. Raw baseline, mutant, and restored transcripts and hashes are in `evidence/raw/phase5-force-accumulator-row-000782-20261018/manifest.json`.
