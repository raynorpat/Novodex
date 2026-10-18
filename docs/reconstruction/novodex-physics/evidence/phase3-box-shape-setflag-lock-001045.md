# Box shape `setFlag` scene-lock behavior (`phys_fn_001045`)

IDA decompilation of the oracle body at RVA `0x000230a0` identifies the public `NpBoxShape::setFlag` wrapper. It reads the scene write-lock link at handle offset `+0x10`, attempts the write lock, and on refusal reports `NXE_INVALID_OPERATION` from `NpBoxShape.cpp:0x0f` without changing the flag. On success it dispatches the internal shape vtable slot 16 with `(flag, value)` and releases the lock.

The candidate implementation is `nxBoxHandleSetFlag` in `Physics/src/NpActor.cpp`, mapped at `0x10036a50` (`NpActor.obj`). It now applies the same try-lock, diagnostic, guarded update, and release sequence. No public headers changed.

`NxPhysicsActorShapeMutationTests` holds the owning scene write lock, calls `setFlag(NX_TRIGGER_ENABLE, true)`, and verifies that the internal flag stays clear and exactly one lock error is reported. After releasing the lock, it verifies the flag is set. The restored oracle and candidate both report `locked_flag=0 locked_reports=1 unlocked_flag=1`; the differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

The mutation bypasses `nxBoxHandleWriteTry`. It is detected: the candidate reports `locked_flag=1 locked_reports=0 unlocked_flag=1`, producing `stdout_delta=5` against the oracle. The mutation patch and mutant/restored transcripts are retained in [raw evidence](raw/phase3-box-shape-setflag-lock-001045-20261018/).

Fresh gates passed with their full registered assertion floors: Phase 3, `984/984`; Phase 5, `2895/2895`; Phase 6, `1752/1752`; and Phase 7, `3034/3034`. The inventory validator reports 6,338 functions, 5,138 data objects, and zero unexplained rows. Both pinned public-header trees pass with 80 files each. The row is owned by Phase 3 and is dynamically gated by the Phase 5 public-object differential; the test also runs in Phases 6 and 7.

Raw differential and gate transcripts are retained in `evidence/raw/phase3-box-shape-setflag-lock-001045-20261018/`.
