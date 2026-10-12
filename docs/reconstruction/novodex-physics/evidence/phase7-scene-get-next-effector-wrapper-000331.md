# `NpScene::getNextEffector` forwarding — `phys_fn_000331`

The registered `NxPhysicsEffectorTests` fixture exercises `NpScene::getNextEffector` through the public scene API. The restored oracle/candidate staged-pair control matches exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr).

A throwaway `git archive` applied this mutation: Return null from NpScene::getNextEffector instead of forwarding the Scene iterator result. The registered Phase 7 staged-pair differential detects the changed count/iteration output (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=14`, exact stderr). Restoring the wrapper returns exact output. Raw transcripts: [restored control](raw/phase7-scene-phys_fn_000331-wrapper/restored-control.log) and [archive mutant](raw/phase7-scene-phys_fn_000331-wrapper/archive-mutant-differential.log).

The public effector fixture checks ordered list contents through create, release, and actor-release transitions. The null-return mutant is rejected; restored output is exact. This closes only the `NpScene` wrapper row; the wider effector implementation remains tracked separately.

The fresh full Phase 7 Win32 Release gate passes at 1,703/1,703 assertions with the joint and effector scene wrapper rows in the closure ledger. See [the retained gate transcript](raw/phase7-npscene-joint-effector-wrappers-20261012/phase7-gate.log).
