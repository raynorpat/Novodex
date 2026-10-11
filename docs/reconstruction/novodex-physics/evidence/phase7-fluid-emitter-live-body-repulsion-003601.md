# Live-body fluid emitter repulsion (`phys_fn_003601`)

The test creates a dynamic box actor through the SDK in an isolated oracle pair and an isolated candidate pair. Each process loads its matching Physics/Foundation DLLs. A staged `FluidModel.dll` provides identical deterministic callbacks; the oracle callback IAT slots are patched to equivalent test functions, while the candidate resolves the row through its staged `NxPhysics.map`.

For an enabled emitter with an attached dynamic actor, the differential checks the three callback records and arguments, force/torque vectors, particle-mass bits, x86 stack balance, and the live actor body's velocity/accumulator/wake words. Both pairs report the same state:

```
fluid emitter body repulsion live actor=1 callbacks=3 callback_mismatches=0 applied=1 stack=1 mismatches=0
fluid emitter body repulsion live state=3d08888a.bd88888b.3dccccd0.3f2aaab0.bf2aaab0.3f2aaab3.3d08888a.bd88888b.3dccccd0.3f2aaab0.bf2aaab0.3f2aaab3.3ecccccc.3ecccccc.00000000
```

The row-specific mutation removed the particle-mass scaling from both vectors. Oracle and candidate children still exited zero, but the live-state transcript diverged by two lines' contents (`stdout_delta=2`), so the differential rejected it. Restoring the scaling returned to exact output (`stdout_delta=0`, exact stderr). The full mutation and restored transcripts are `build/FluidGate/phase7-fluid-repulsion-live-mutant.log` and `build/FluidGate/phase7-fluid-repulsion-live-restored.log`.

This also replaces the candidate's hand-written indexed x87 scaling loop with ordinary scalar float multiplication. That loop faulted on the first body-backed vector; the scalar operations preserve the oracle's stored float words and pass the live actor differential.

Pinned oracle NxPhysics SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
Restored candidate NxPhysics SHA-256: `93515f4a6ea0a9cd492640212638bfac8f565a0564e75929ad4c99eda5221326`.
The fixture is deterministic and uses no random seed.

The bodyless-actor callback case also remains covered by `NxPhysicsFluidEmitterAbiTests`. These direct calls cover the callback/bodyless and live-body paths, but do not yet close all row guards or the nonempty emitter caller at `phys_fn_003624`. The dirty static-collision rebuild (`phys_fn_003505`) and complete manager row (`phys_fn_003630`) remain open. Public headers were not changed.

Verification: fresh Release CMake configure/build passed. Phase 5 passed with 2,637/2,637 coverage assertions; Phase 7 passed with 1,664/1,664. The reconstruction tooling suite passed all 810 tests. Both public-header manifests and inventory validation passed (80 headers per tree; 6,338 function rows, 5,138 data rows, zero unexplained). Candidate hash above is from these gate runs.
