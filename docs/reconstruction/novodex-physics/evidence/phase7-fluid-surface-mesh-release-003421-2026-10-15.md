# Fluid surface-mesh owner-slot release (`phys_fn_003421`)

IDA identifies RVA `0x84a60` as a thiscall release helper. It reads the owned object at `this+0x24`; a null slot returns zero. Otherwise it invokes vtable slot 0 with deleting flag 1, clears the owner slot after the destructor returns, and returns that result.

`Physics/src/Scene.cpp` reconstructs the helper and uses it from `NxFluid::releaseSurfaceMesh` and the surface-mesh replacement path. The registered `NxPhysicsSimulationTests` fixture directly calls oracle RVA `0x84a60` and the candidate function resolved from `NxPhysics.map`. A fake deleting destructor verifies one call, the object and flag, the cleared slot, and the forwarded return sentinel. The restored staged-pair differential passes with `stdout_delta=0` and exact stderr.

The row mutation makes the helper a no-op. The registered differential rejects it (`candidate_exit=1`, `stdout_delta=4083`); the restored control is exact. The candidate map lookup is intentional because adding a linked method moves private candidate RVAs.

Fresh formal gates on 2026-10-15 pass at the pinned floors: Phase 5 2,791/2,791, Phase 6 1,692/1,692, and Phase 7 2,908/2,908. Compressed transcripts are under evidence/raw/phase7-fluid-surface-mesh-release-003421-20261015/.
