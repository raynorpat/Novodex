# `NxSceneInternal::simulateFrame` whole-row mutation proof

Date: 2026-10-09
Row: `phys_fn_000659` at RVA `0x13c40`
Gate: registered `NxPhysicsSimulationTests` staged-pair differential (Phase 7)
Source: throwaway `git archive` of `cc1118dc`; the ignored pinned oracle assets were copied into the archive so the committed gate runner could stage its input.

A clean x86 Release build was configured in `D:\Novodex-build-m6-659-archive`. The archive baseline matched the pinned UE3 oracle exactly: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`. Baseline candidate SHA-256: `b2f31dcab6ad89bbe6a509e0687afb2dad4d8c0fc8fd76563d250477b165102b`.

For the mutation, inserted an immediate `return;` at the start of `NxSceneInternal::simulateFrame`, rebuilt `NxPhysics.dll`, and reran the same registered staged-pair gate. The gate rejected the mutant: `oracle_exit=0`, `candidate_exit=1`, `stdout_delta=3829`, and `stderr_exact=False`. Mutated candidate SHA-256: `904e3c8c892370a46283f750033a0925b60ce4c2056cc22a598d3df0b8e64515`.

Restored `Physics/src/Scene.cpp` byte-for-byte from the archive source, rebuilt, and reran the same gate. The control again matched exactly: both exits were zero, `stdout_delta=0`, and stderr was exact. Restored candidate SHA-256: `c2bccb7487c9e48c6a183651b4bdb7b5ab41841febc70cf954983886bfc92d70`.

The mutation removes the frame scheduler body itself and is caught by the registered simulation corpus. This closes only the scheduler row; the separately tracked manager and fluid-step rows remain open where listed.
