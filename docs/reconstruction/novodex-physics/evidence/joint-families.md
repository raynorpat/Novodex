# Joint families

Measurement note for the joint-families plan (`docs/superpowers/plans/2026-09-25-joint-families.md`),
which continues the revolute translation-unit pilot (`evidence/unit-pilot-revolute.md`): the
shared NpJoint base, the shared joint base rows, then the nine remaining joint families
(prismatic, cylindrical, spherical, point-on-line, point-in-plane, distance, pulley, fixed,
D6) reconstructed as real source and wired into `Scene::createJoint` behind the byte-exact
staged-pair joint test. The contract is `units/joint-families-contract.md`. Each task appends
one row to the timing table below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-25T06:53:58 | 2026-09-25T07:04:52 | 13 | 591 | Shared NpJoint base: `NpJointShared<Iface, Internal>` (novtable) in `core/NpJointShared.h`; the 13 folded bodies (004539 004437 004441 004497 004499 004483 004573 004577 004491 004635 004443 004479 004743, owned by NpD6/NpPulley/NpDistance/NpPointInPlane/NpSpherical/NpPrismatic units) claimed in `core/NpJointShared.cpp`, 6 of them discovered -> reconstructed; revolute's 10 per-family NxJoint rows now call shared `forward*` helpers. Revolute vtable slot targets unchanged apart from the 13 moved members; joint gate stdout_delta=0; no transcript defects. |
| 2 | 2026-09-25T07:10:01 | 2026-09-25T07:39:10 | 12 | 10506 | Shared joint base: `## Shared rows` classifies all 142 rows of Joint.cpp and the three gaps (effector 19 rows, scene-dump writer 41, articulation/scene helpers 29, list helpers 5 -> defer; 004417-004433, 004115/004117, 004119 -> reuse). Written: 004064 004091 004093 004099 004101 004109 004111 004123 004133 004135 004143 (core/Joint.cpp), 004391 (core/JointSupport.cpp); new stub 000598. No family creation/getter path needed a new row. Deferred: 004085 004103 004105 004113 004068 004072, Scene rows 000022 000571 000598 000633 000758. Joint gate stdout_delta=0. An uncommitted setter experiment matched the oracle except where the rebuilt Foundation NxNormalToTangents differs (defect outside this task). |
