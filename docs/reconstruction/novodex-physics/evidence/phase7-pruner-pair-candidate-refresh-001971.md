# Persistent pruner pair refresh (`phys_fn_001971`)

IDA's oracle routine at RVA `0x0004c100` selects the two pruner-pool entries, resolves their shapes and pair IDs, finds or inserts the persistent `NxPairNode`, and writes the current frame from `a3 + 0x0c` to the node's `+0x104` stamp. The candidate implementation is `cpmRefreshPairCandidate` in `Physics/src/ContactPairManager.cpp`, linked at `0x10017220` in `ContactPairManager.obj`.

The registered `NxPhysicsSimulationTests` differential now checks the coherent broadphase across two consecutive fetched frames. It verifies that overlapping pair nodes remain live and that every node stamp matches the scene frame. The row marker is `simulation broadphase pair-refresh row=001971 frames=2 retained=1 stamped=1`.

A candidate-only mutation changed the stamp to `frame ^ 1u`. The Phase 7 simulation differential rejected it through the trigger enter/stay/leave sequence (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4100`, `stderr_exact=False`). After restoring the stamp, the staged oracle/candidate differential matched exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

The restored candidate DLL SHA-256 is `8eac8031962767d01f270152ab8279e4b464831fb58fc7806e80f95406aae71c`; the pinned oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Focused, mutation, and restored transcripts are compressed under [evidence/raw/phase7-pruner-pair-candidate-refresh-001971-20261017](raw/phase7-pruner-pair-candidate-refresh-001971-20261017/).

Fresh Win32 Release Phase 5, 6, and 7 gates pass at 2,862/2,862, 1,729/1,729, and 3,001/3,001 assertions. The full reconstruction tooling suite passes all 817 tests.
