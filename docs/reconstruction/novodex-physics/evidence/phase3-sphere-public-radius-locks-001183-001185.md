# Sphere public radius lock wrappers (phys_fn_001183 and phys_fn_001185)

The sphere `setRadius` wrapper at oracle RVA `0x24670` uses the write link at `self+0x10`, reports `NXE_INVALID_OPERATION` from `NpSphereShape.cpp` line `0x29` when another thread owns the lock, and otherwise updates the stored radius, owner and dirty state before releasing the lock. The `getRadius` wrapper at `0x24640` acquires the read link at `self+0x14`, calls the internal radius accessor, releases the link, and returns the result.

`NxPhysicsActorDynamicSetterTests` drives a normal update, a foreign-owner write-lock refusal, and the getter read-lock state. The registered oracle/candidate differential is exact (`stdout_delta=0`, both exits 0, exact stderr). Removing the setter try-lock/report is rejected (`stdout_delta=663`); removing the getter lock bracket is independently rejected (`stdout_delta=2`).

The Phase 3 ledger discharges the public setter row through Phase 5. The getter row was already reconstructed; this fixture confirms its read-link behavior. The complete Phase 5 gate passes at 2,761/2,761 assertions. Compressed focused, mutation, and full-gate transcripts plus hashes are in `evidence/raw/phase3-sphere-public-radius-locks-001183-001185-20261015/`.

No public Physics headers changed.
