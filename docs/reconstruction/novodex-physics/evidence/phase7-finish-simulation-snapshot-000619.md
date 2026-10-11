# Phase 7 partial verification: post-step pose snapshot in `phys_fn_000619`

At mainline commit `d332580a702fe7346b9a47c398753ed71ad44ecc`, the public
`NxPhysicsSimulationTests` oracle/candidate differential passes exactly after a
fresh Win32 Release CMake build. The tested candidate DLL SHA-256 is
`934a63c2a9a27c85f70dc095bb2a571ef51504710fa66f24e8c8f840f8f42c15`.

To test the existing post-step snapshot behavior in
`NxSceneInternal::finishSimulation` (`phys_fn_000619`, RVA `0x114b0`), a
temporary mutation disabled the conditional copy of the body's current pose
words at `+0x18..+0x48` into the prior-pose words at `+0x50..+0x80`. The
registered staged-pair differential detected it while both processes exited
successfully: `stdout_delta=7248`, `stderr_exact=True`. The mutant DLL SHA-256
was `083f3233431f91557d616d4938d28e73a93fe5a74858e1a44096e44525852f07`.

The source was restored, `NxPhysics.dll` rebuilt, and the same differential
returned `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr.
Full transcripts are retained in the ignored build directory as
`row619-snapshot-mutant.log` and `row619-snapshot-restored.log`.

This proves the registered simulation corpus observes the body-snapshot branch
of `phys_fn_000619`. It does not close the full row: the oracle's separate
iteration over the fluid-manager list at `Scene+0x61c` and its per-fluid update
calls are still absent from the candidate. The inventory row remains open.
Public Physics headers are unchanged.
