# `NxSceneInternal::visualize` mutation proof

Date: 2026-10-14
Row: `phys_fn_000657` at RVA `0x000139c0`
Gate: registered `NxPhysicsSceneVisualizeTests` staged-pair differential (Phase 7)
Source: isolated `codex/scene-raycast-row-000700` worktree at `28e325f3`.

For the mutation, inserted an immediate `return` at the start of `NxSceneInternal::visualize()` and rebuilt the Phase 7 candidate. The registered visualization test rejected it: the oracle exited 0, the candidate exited 1, `stdout_delta=1962`, and stderr differed. The candidate reported `FAIL Scene::visualize did not retain a tracked debug renderable`; visualization output and contact lines were absent.

Restored `Physics/src/Scene.cpp` byte-for-byte and rebuilt. The visualization differential matched exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`), including the world-axes points and lines, and the full Phase 7 gate passed with 1,793/1,793 coverage assertions. The restored candidate DLL SHA-256 was `0578c81975a1708359b96debcb5b4836b48e74cbe63050a39d4bd280c35f7cfd`.

Raw mutant and restored gate logs are compressed under `evidence/raw/phase7-scene-visualize-000657-20261014/`. This closes the visualization row; placeholder collision-pruner and fluid draw helpers remain open as separately inventoried rows.
