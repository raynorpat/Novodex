# Phase 7 closure: per-substep scene step (`phys_fn_000655`)

`phys_fn_000655` at RVA `0x000137e0` is implemented by
`NxSceneInternal::simulateFrame` in `Physics/src/Scene.cpp`. The approved
off-center spring/damper simulation runs this step with the effector tick before
body integration and observes the resulting linear velocity, angular velocity,
and orientation over three steps. The test is part of the registered
`NxPhysicsSimulationTests` staged-pair differential.

For whole-row falsification, a clean `git archive` of mainline commit
`e3faa461` was built as Win32 Release. Adding an immediate return at the start
of `simulateFrame` caused the registered differential to reject the candidate:
`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=3829`, and
`stderr_exact=False` (the mutant also fails the registered simulation
assertions). Removing the mutation, rebuilding, and rerunning produced
`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`.

This closes the tested behavior of the row against the current simulation
corpus. The Phase 7 ledger still defers the other reconstructed rows and all
untranslated rows; this is not phase completion.
