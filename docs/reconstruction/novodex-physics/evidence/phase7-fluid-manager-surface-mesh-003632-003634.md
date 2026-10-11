# FluidManager surface-mesh traversal (phys_fn_003632 / phys_fn_003634)

`phys_fn_003632` at RVA `0x89c80` reports the unavailable-backend warning when
the manager's `+0x2b` flag is clear, then continues into `phys_fn_003634` at
`0x89cd0`. The 16-byte continuation walks the primary fluid pointer array at
manager `+0x04..+0x08` and calls `phys_fn_003421` (`0x84a50`) for each entry.
That per-fluid helper returns without a callback when the fluid's implicit-mesh
pointer at `+0x24` is null; otherwise it forwards the mesh context, fluid
handle, mesh handle, and triangle count to `ImplicitFluidMeshRun`.

`nxFluidManagerGenerateSurfaceMeshesDisabled` now preserves the disabled
warning and walks every entry through the reconstructed
`nxNpFluidEmitterGenerateSurfaceMesh` helper. The registered
`NxPhysicsFluidEmitterAbiTests` direct oracle differential seeds two fluid
internals, each with a live mesh record, and checks the callback count and all
four callback arguments. Its green output is
`fluid manager surface-mesh traversal fluids=2 oracle_callbacks=2 candidate_callbacks=2 mismatches=0`.

Mutation sensitivity: in a rebuilt candidate, replacing the helper dispatch
with an increment-only loop made the oracle report two callbacks and the
candidate report zero (`mismatches=3`); the test exited nonzero. Restoring the
dispatch returned both callback counts and payloads to an exact match. The
existing disabled-manager simulation case continues to pin the warning path
with an empty array. This closes these two rows only; the post-step per-fluid
simulation update and FluidModel-backed static-collision refresh remain open.
