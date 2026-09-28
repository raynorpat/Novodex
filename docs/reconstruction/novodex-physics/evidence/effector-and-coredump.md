# Effector and core dump

Measurement note for the effector-and-coredump plan
(`docs/superpowers/plans/2026-09-28-effector-and-coredump.md`), which continues the joint
families (`evidence/joint-families.md`): the spring-and-damper effector
(`NxScene::createSpringAndDamperEffector`, `releaseEffector`, the effector enumeration and the
`NxSpringAndDamperEffector` API) and the scene core-dump writer behind
`NxPhysicsSDK::coreDump`, reconstructed as real source and checked by staged-pair
differentials. The contract is `units/effector-coredump-contract.md`. Each task appends one
row to the timing table below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T07:58:00 (approx.; the first build started just before 08:00) | 2026-09-28T08:30:00 | 0 | 0 | Contract only. Fresh configure and Release build of the worktree (`build/`); Phase 6 gate baseline `status=pass` (403/403 coverage assertions). Bundles for `gap:fluids\NpImplicitMesh.cpp..NpSpringAndDamperEffector.cpp`, `NpSpringAndDamperEffector.cpp`, `gap:NpSpringAndDamperEffector.cpp..Joint.cpp` generated. Supplement rerun with the 34 existing plus 9 new requests (0x8ed50 0x8edb0 0x8f100 0x8fc00 0x8fc50 0x8fcb0 0x8fd00 0x91940 0x91de0), 43/43 `ok`, existing entries unchanged. Effector: 30 rows 3,325 B plus 12 Scene/NpScene rows 887 B; the candidate body record has no Observable part, which the effector's observer calls need (Task 2 prerequisite). Core dump: 41 rows 22,965 B plus 003981; always returns false; text and binary differ only in the float token; the file embeds the date and heap pointers, which the test must normalise. Split: 2 = 4,212 B, 3a = 11,793 B (24 rows incl. 003981), 3b = 11,630 B, 4 = 221 B. |
