# Phase 7 — `NxSceneInternal::getNbPairs` (`phys_fn_000523`)

The row at RVA `0x00010400` returns the scene pair count from `+0x3c`. The registered public `NxPhysicsPairFlagTests` differential reports the count after adding a compound actor pair and after releasing it.

The clean staged-pair control matched the pinned DLL exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). For the mutation run, the getter was changed from `return at<NxU32>(0x3c);` to `return at<NxU32>(0x3c) + 1;`, and `NxPhysics.dll` was explicitly rebuilt before staging. The oracle reported `count=1` and released `count=0`; the mutant reported `count=2` and `count=1`, and the pair-array result changed accordingly. Both processes exited 0 with exact stderr; the registered differential failed with `stdout_delta=6`.

The exact source was restored and `NxPhysics.dll` rebuilt from it. The restored staged-pair differential returned to `stdout_delta=0`, `stderr_exact=True`, and zero exits. Candidate SHA-256: `04a269e9c3577431e6124f937576803590b3a59d16227437b1aa5a7cc81b5e92`. No public header changed.
