# Phase 5 mutation proof: `phys_fn_001941`

`nxPruningAddShape` writes prunable type and kind bytes before inserting the shape into the scene pruner. To falsify the kind-byte write, the candidate mutation changed `shape[0xcf] = 0` to `shape[0xcf] = 1`. The registered `NxPhysicsActorShapeMutationTests` staged-pair differential caught the mutation with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=58`, and `stderr_exact=True` (`build/phase5-001941-mutant.log`).

After restoring the exact kind-zero write, the same target returned `stdout_delta=0`, `stderr_exact=True` (`build/phase5-001941-restored.log`). No production behavior change remains. Public headers were not changed.
