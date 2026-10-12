# `NpScene::getNbJoints` forwarding — `phys_fn_000321`

The registered `NxPhysicsJointStagedPairTests` fixture exercises `NpScene::getNbJoints` through the public scene API. The restored oracle/candidate staged-pair control matches exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr).

A throwaway `git archive` applied this mutation: Return zero from NpScene::getNbJoints instead of forwarding the Scene count. The candidate differs on the joint count/enumeration records and the registered target detects it (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=296`, exact stderr). Restoring the wrapper returns exact output. Raw transcripts: [restored control](raw/phase7-scene-phys_fn_000321-wrapper/restored-control.log) and [archive mutant](raw/phase7-scene-phys_fn_000321-wrapper/archive-mutant-differential.log).

The public joint lifecycle fixture checks the scene joint count before and after release. The zero-count mutant is rejected; restored output is exact. This closes only the `NpScene` wrapper row; the wider joint implementation remains tracked separately.

The fresh full Phase 7 Win32 Release gate passes at 1,703/1,703 assertions with the joint and effector scene wrapper rows in the closure ledger. See [the retained gate transcript](raw/phase7-npscene-joint-effector-wrappers-20261012/phase7-gate.log).
