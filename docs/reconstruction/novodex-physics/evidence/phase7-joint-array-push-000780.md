# Joint island continuation array push (`phys_fn_000780`)

Date: 2026-10-14

Row: `phys_fn_000780` at RVA `0x00018630`

Gate: Phase 7 `NxPhysicsSimulationTests` staged-pair differential

Row 000780 is the continuation of the body-level island dissolve helper 000778. The direct probe resolves the enclosing candidate method through the staged linker map and calls the oracle at pinned RVA 0x185f0. It seeds a root body with a two-link joint list, an array with spare capacity, and identifies the removed joint. The oracle and restored candidate both queue exactly the other joint, clear both links, reset the body, and report `queued=1 linked_cleared=1 body_reset=1`.

Suppressing only `nxJointPointerArrayPush` in the 000780 continuation leaves link clearing and body reset intact but changes the direct result to `queued=0`; the staged differential rejects the mutant (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4000`).

Restoring `Physics/src/core/JointSupport.cpp` byte-for-byte returns an exact control (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). The restored source SHA-256 is `5aa19f7651a86f9a2f370b4164fd7fbdaee73f089b2fefa8313b3367076a2259`; no public header changed. Baseline, mutant and restored logs, source hashes, and the isolated patch are under `evidence/raw/phase7-joint-array-push-000780-20261014/`.

The shared simulation target passes the full gate in each registering phase: Phase 5 at 2,729/2,729, Phase 6 at 1,654/1,654, and Phase 7 at 1,803/1,803 coverage assertions. All three transcripts are preserved in the raw evidence folder.
