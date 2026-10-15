# Capsule height rows 000997 and 000999

IDA identifies `phys_fn_000997` at RVA `0x21c00` as the internal `NxCapsuleShape::setHeight` row: it stores the input multiplied by 0.5 at shape offset `+0xe4`, then calls BASE vtable slot 6 with update flag 1. `phys_fn_000999` at RVA `0x21c20` loads the same float, doubles it, and returns the public height. The public wrappers in `NpActor.cpp` now dispatch to the named `CapsuleShape::nxCapsuleSetHeight` and `CapsuleShape::nxCapsuleGetHeight` methods. No public headers changed.

The registered `NxPhysicsActorDynamicSetterTests` oracle/candidate differential now records `setter capsule_height_row=40200000.3fa00000.1`: the public getter returns 2.5, internal `+0xe4` stores 1.25, and the owning pruner epoch advances once. The restored candidate control matches the oracle exactly (`stdout_delta=0`, exact stderr).

Each row was mutation-tested independently against the same registered target. Replacing only the setter's 0.5 conversion with 0.25 changed the candidate marker from `40200000.3fa00000.1` to `3fa00000.3f200000.1` and was rejected (`stdout_delta=4`). Replacing only the getter's doubling expression with a direct half-height return changed the getter word from `40200000` to `3fa00000` and was rejected (`stdout_delta=10`). The source was restored and rebuilt after each mutation; the final restored differential is exact.

Phase 5 discharges the two Phase 3 rows. The closure records them as `differential_falsified` on the Phase 5 registered target, with `discharged_by_phase: 5`.
