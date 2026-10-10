# Phase 6 shared joint impulse row `phys_fn_004395`

IDA identifies `phys_fn_004395` at RVA `0x000af790`, size 835 bytes. It is a thiscall row on the 0x50-byte support record in ECX, with two stack words and `ret 8`. The first stack value is the impulse. The second is unused. Bit 10 of the record flags selects the angular-only form; otherwise the row updates linear velocity and then applies the angular Jacobian through the body's row-major 3x3 inverse inertia. Body 0 uses the positive impulse and body 1 the negative impulse. Null bodies and bodies with zero inverse mass are skipped. The candidate retains the oracle's x87 accumulation order for each matrix row.

`NxPhysicsJointSupportTests` calls the oracle address and the candidate helper on twin scratch fixtures for nine cases. The cases cover both flag forms, all four body masks, zero/nonzero inverse masses, signed impulses, `0x027f` and `0x0f7f` control words, and varied opaque second-argument bits. The restored candidate exactly matches the oracle output digest `1e9bf6cbf7c6576a`; both sides report zero mismatches. The fixed input digest is `cf0a288f38c68f7c`.

For mutation validation, the helper's per-body guard was changed so every body update was skipped. The test rejected the candidate with digest `f2647d06b2b61d05`, `mismatches=39`, and exit 1. Restoring the helper and rebuilding returned to the exact oracle digest with `mismatches=0` and exit 0. The existing kind-5 and island-teardown cases also remain exact.

This direct differential isolates the shared row from solver dispatch. The fixture declaration is in the private `Physics/src/include/core/JointSupport.h`; public NxPhysics headers were not changed.
