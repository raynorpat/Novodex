# Fluid emitter body repulsion callback branch (`phys_fn_003601`)

This probe reaches the internal row directly through the candidate DLL's mapped symbol and through oracle RVA `0x881c0`. It establishes the callback contract for an enabled emitter whose actor exists but has no body. The callback fixture returns asymmetric force/torque float words and a distinct particle-mass value, making argument order and vector/mass reads observable. Oracle and candidate callback records are compared, along with the helper result and x86 stack balance.

The restored differential reports:

```
fluid emitter body repulsion bodyless actor=1 callbacks=3/3 order=1 vectors=1 mass=1 stack=1/1 mismatches=0
fluid emitter aggregate cases=6 mismatches=0
```

A row-specific red-phase mutation changed the candidate enabled check from `<= 0` to `> 0`, forcing the early-return branch for the fixture's enabled value. The test rejected the mutant: oracle callbacks were `3`, candidate callbacks were `1`, and the body-repulsion vector and mass checks diverged. The captured output is `build/FluidGate/phase7-fluid-repulsion-flag-mutant.log`; build output is `build/FluidGate/phase7-fluid-repulsion-flag-mutant-build.log`. Restoring the original condition and rebuilding produced the exact zero-mismatch output above; restored output is `build/FluidGate/phase7-fluid-repulsion-restored.log`.

Pinned oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
Restored candidate DLL SHA-256: `CDFAB521682B8B0BA1103145B166E69BA5DD59125AEEDB25F4F0AC6EFB4B03C2`.
The fixture is deterministic and uses no randomized seed.

This proves only the bodyless-actor callback branch. The live-body path that scales all six force/torque components by particle mass and dispatches `nxNpActorApplyForce` remains unproven. `phys_fn_003601` stays open, as do the nonempty-emitter caller path and dirty static-collision refresh (`phys_fn_003624`, `phys_fn_003505`, and `phys_fn_003630`). Public headers were not changed.

Verification: fresh Release CMake build succeeded. Phase 5 passed at 2,636/2,636 coverage assertions and Phase 7 at 1,663/1,663; each gate also passed inventory validation and both pinned/public header manifests. The reconstruction tooling suite passed all 810 tests. The retained logs are `build/FluidGate/phase5-restored.log`, `build/FluidGate/phase7-restored.log`, and `build/FluidGate/tools-tests.log`.
