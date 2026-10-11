# Phase 7 fluid-emitter world-pose recomposition (`phys_fn_003605`)

`phys_fn_003605` at RVA `0x88310` composes an emitter's internal local pose with its optional actor frame and publishes the resulting world pose through the FluidModel `EmitterSetGlobalPose` callback. The reconstructed helper is `nxNpFluidEmitterUpdateGlobalPose` in `Physics/src/NpFluidEmitter.cpp`.

The registered `NxPhysicsFluidEmitterAbiTests` target calls both the pinned oracle entrypoint and the candidate map-resolved helper. Its three cases cover an unattached emitter with a non-identity local rotation, an asymmetric matrix-backed actor frame with non-binary local-pose values, and a dynamic-body actor frame with an asymmetric quaternion and non-binary local pose. It compares all twelve callback pose words, callback arguments, return value, and x86 stack balance. The restored control reports `fluid emitter world-pose cases=3 mismatches=0`.

The asymmetric inputs initially exposed one-ULP differences in the actor-backed cases. The oracle disassembly keeps matrix products and accumulations in x87 extended precision, with specific expression ordering and a float spill between two translation components. The implementation now reproduces those x87 accumulation and spill points explicitly. The exact restored differential passes all three cases.

The mutation adds `1.0f` to the recomposed world-pose X translation for actor-backed emitters. Both actor cases reject it (`mismatches=2`); the no-actor case remains unchanged. After restoring the source, rebuilding and rerunning the focused target returns all cases to zero mismatches.

This closes only the helper row `phys_fn_003605`. Its caller `phys_fn_003624` and the extension-backed post-simulation update lifecycle remain open. Public Physics headers are unchanged.

The registered full Phase 7 gate and reconstruction-tool tests are recorded in the task ledger and completion plan after verification.
