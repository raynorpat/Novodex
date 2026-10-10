# `NxSceneInternal::addJoint` whole-row mutation proof

Date: 2026-10-09
Row: `phys_fn_000661` at RVA `0x13e00`
Gate: registered `NxPhysicsJointStagedPairTests` staged-pair differential (Phase 7)
Source: disposable archive tree of `5b563775`. The preceding archive was based on `e9de10ca`; the only tracked differences were documentation and generated ledgers. A fresh `git archive` of `5b563775` supplied those changed files, and the Physics/Foundation/External/tests/CMake implementation inputs were verified unchanged.

The x86 Release archive baseline matched the pinned UE3 oracle exactly: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`. Baseline candidate SHA-256: `d81bf3197b0eeec78b5fa2afa446e980d2b1c00f5ede11fa013e78803d57660e`.

For the mutation, inserted an immediate `return;` at the start of `NxSceneInternal::addJoint`, rebuilt `NxPhysics.dll`, and reran the registered staged-pair gate. The gate rejected the mutant: `oracle_exit=0`, `candidate_exit=-1073741819` (access violation), `stdout_delta=3282`, and `stderr_exact=True`. Mutated candidate SHA-256: `7cb3d825553bee3f8fd9c1b39d18abf22223fe736237262813478654615f67a2`.

Restored `Physics/src/Scene.cpp` byte-for-byte from the current main worktree, rebuilt, and reran the same gate. The control matched exactly: both exits were zero, `stdout_delta=0`, and stderr was exact. Restored candidate SHA-256: `b8ee2bb7fca7fc3c62778544f83de1d8a5724b4f692ec92a55968eae21bd121b`.

The no-op suppresses duplicate checks, both scene-list insertions, joint-array growth, and the scene-owner field write. The registered joint lifecycle transcript reaches this row and catches the resulting candidate failure.
