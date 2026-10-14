# `NxSceneInternal::NxSceneInternal` mutation proof

Date: 2026-10-14
Row: `phys_fn_000647` at RVA `0x00012c10`
Gate: registered `NxPhysicsSceneConstructorTests` staged-pair differential (Phase 7)
Source: isolated `codex/scene-raycast-row-000700` worktree at `bfd64006`.

The constructor stores its allocated public `NpScene` wrapper at Scene byte offset `+0x6cc`. The registered constructor test creates a scene, reads the internal-scene link from the wrapper, and requires `scene constructor internal_link=1`.

For the mutation, changed only the `+0x6cc` store to zero and rebuilt the Phase 7 candidate. `NxPhysicsSceneConstructorTests` rejected it: the oracle exited 0, the candidate exited 1, `stdout_delta=6`, and stderr differed because the candidate reported `FAIL scene creation failed`. The broken constructor also caused dependent Phase 7 fixtures to fail, as expected for a Scene-construction mutation; the full mutant phase gate therefore failed.

Restored `Physics/src/Scene.cpp` byte-for-byte and rebuilt the same gate. The constructor differential then matched exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`), reported `scene constructor internal_link=1` on both pairs, and the full Phase 7 gate passed with 1,793/1,793 coverage assertions. The restored candidate DLL SHA-256 was `39438a79693f7bb57b3ffca001d924e9fa7862f03889f30b1236ec705f6f4368`.

Raw mutant and restored gate logs are compressed under `evidence/raw/phase7-scene-constructor-000647-20261014/`. This closes the constructor initialization row; other Scene initialization and lifecycle rows remain independently tracked.
