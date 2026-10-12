# Fluid manager constructor (`phys_fn_003645`)

Public fluid creation constructs a scene-owned 0x34-byte FluidManager. The fixture checks that the manager points back to its scene, begins with empty primary and secondary arrays, and records the expected disabled-backend state. The candidate helper is `nxSceneCreateFluidManager`, resolved from the built candidate map.

A row-specific mutation that clears the candidate owner field changes the marker from `owner=1` to `owner=0` and causes the candidate to access-violate later in the registered Phase 7 `NxPhysicsSimulationTests` differential. The measured result is `candidate_exit=-1073741819`, `stdout_delta=53`, and `stderr_exact=True`. Restoring the owner field returns the full target transcript to an exact match.
