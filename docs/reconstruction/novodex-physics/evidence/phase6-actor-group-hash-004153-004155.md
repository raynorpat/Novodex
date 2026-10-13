# Actor-group pair hash rows 004153 and 004155

Phase 6 now runs `NxPhysicsSDKTests` with two live Scenes. Its public actor-group
pair-flag round trip exercises `phys_fn_004155` insertion, followed by
`phys_fn_004153` lookup in both key orders and after replacing the value with
zero. The test is already required in Phases 2 and 3; Phase 6 registers it to
falsify the shared hash rows under their owning phase.

Independent no-op mutations were rejected by the staged differential:

- `phys_fn_004153` (`NxFindCollisionPairRecord`): `stdout_delta=2`.
- `phys_fn_004155` (`cpmOpen004155`): `stdout_delta=6`.

Both processes exited zero and stderr matched for each mutant. The restored
control is exact (`stdout_delta=0`, `stderr_exact=True`). See the raw mutation,
restored-control, and Phase 6 gate logs in
`evidence/raw/phase6-actor-group-hash-004153-004155/`.

Fresh 2026-10-13 full Phase 6 gate: all staged-pair differentials and oracle proofs pass; coverage is 1,619/1,619. The SDK transcript reports two Scenes, pair values in both key orders, zero after clear, and a surviving lookup after one Scene is released. The staged oracle and candidate hashes are `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` and `fd84ab4d941da2871f6b3016badaef46d692575fce39bc0d58235366746a380e`. Full transcript: `evidence/raw/phase6-actor-group-hash-004153-004155/phase6-gate.log.gz`.
