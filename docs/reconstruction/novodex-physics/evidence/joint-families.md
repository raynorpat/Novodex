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
