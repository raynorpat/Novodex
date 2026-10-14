# Joint support body-pose mutation proof

Date: 2026-10-14
Row: `phys_fn_000754` at RVA `0x00017010`
Gate: registered `NxPhysicsJointSlotTests` staged-pair differential (Phases 6 and 7)

A throwaway immediate-return mutation of `Row000754Fixture::row000754()` skipped the body pose update from center-of-mass pose. The registered joint-slot differential rejected the candidate: oracle and candidate both exited 0, `stdout_delta=2804`, and stderr matched exactly. The 14 projection cases in the joint-slot fixture exercise this row.

After restoring the implementation and rebuilding `NxPhysics.dll`, the same staged-pair differential passed with both exits 0, `stdout_delta=0`, and exact stderr. The full Phase 7 gate passed with 1,793/1,793 registered coverage assertions. Mutant, restored, and full-gate logs are compressed under `evidence/raw/phase7-joint-support-row-000754-20261014/`.
