# Scene joint-count getter `phys_fn_000559`

`phys_fn_000559` is the seven-byte Scene joint-count getter at RVA `0x00010860`: it returns the word at `Scene + 0x6c8`. The candidate implementation is `NxSceneInternal::getNbJoints` in `Physics/src/Scene.cpp`.

The registered Phase 7 `NxPhysicsJointStagedPairTests` differential reports each scene's joint count alongside enumeration before and after release. Baseline oracle/candidate output matched exactly. For the row-specific mutation, the candidate getter was changed to read the adjacent effector count at `Scene + 0x6c4`. Joint lifecycle cases then reported `count=0` while `enumerated=1`; both processes exited zero and stderr matched, but the transcript differed by 296 bytes and the runner rejected it. Mutant candidate SHA-256: `a38c09b26742a7903edec37be4ffe3ab8e097f2773a3348129d938fffceb0d71`.

After restoring the `+0x6c8` read byte-for-byte and rebuilding, the same differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored candidate SHA-256: `e0c4b09eb6ac5e4a246779a3f8112f104cc1b7d63db5985c4a4a3ac74bf6fd82`. The full Phase 7 gate remains required to validate this ledger update; terminal Phase 8 closure remains open.
