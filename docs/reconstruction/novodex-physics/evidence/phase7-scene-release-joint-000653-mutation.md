# `NxSceneInternal::releaseJoint` whole-row mutation proof

Date: 2026-10-09
Row: `phys_fn_000653` at RVA `0x13760`
Gate: registered `NxPhysicsJointStagedPairTests` staged-pair differential (Phase 7)
Source: throwaway `git archive` of `e9de10ca`; the ignored pinned oracle assets were copied into the archive so the committed gate runner could stage its input.

A clean x86 Release build was configured in `D:\Novodex-build-m6-653-archive`. The archive baseline matched the pinned UE3 oracle exactly: both processes exited 0, `stdout_delta=0`, and stderr was exact. Baseline candidate SHA-256: `7d7072c408017af7922d0756734a8d750ef5539df6df9aedf89dbaf90441b383`.

For the mutation, inserted an immediate `return;` at the start of `NxSceneInternal::releaseJoint`, rebuilt `NxPhysics.dll`, and reran the registered staged-pair gate. The gate rejected the mutant: both processes exited 0, `stdout_delta=494`, and stderr was exact. Mutated candidate SHA-256: `2c8351baffee2af4039c180c04ba9e5bf7d62b535692021d94f9f509f397205f`.

Restored `Physics/src/Scene.cpp` byte-for-byte from the archive source, rebuilt, and reran the same gate. The control matched exactly: both processes exited 0, `stdout_delta=0`, and stderr was exact. Restored candidate SHA-256: `d81bf3197b0eeec78b5fa2afa446e980d2b1c00f5ede11fa013e78803d57660e`.

The mutation suppresses the re-entry guard, unlink, deleting destructor, scene count decrement, iterator reset, and guard cleanup as one row-level control. The registered joint lifecycle transcript detects the resulting behavioral difference.
