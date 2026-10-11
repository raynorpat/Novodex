# Phase 7 fluid post-step reconstruction (`phys_fn_000619`, `phys_fn_003475`)

`NxSceneInternal::finishSimulation` now preserves the existing body post-step
work, then traverses the fluid array owned by the Scene's manager at `+0x61c`.
Each fluid refreshes its bounds, copies its optional implicit-mesh cache,
copies available particle channels, and updates each emitter's transposed
global pose. The implementation is private to `Scene.cpp`; no public Physics
headers changed.

The pinned-oracle IDA pseudocode at `0x100114b0` confirms the Scene manager
array traversal and calls `0x100854d0` once per fluid. The per-fluid row calls
`FluidGetBounds`, conditionally dispatches `ImplicitFluidMeshGetMesh` through
the mesh record, then calls `FluidGetParticles` and the emitter update helper.
The particle helper at `0x1008a9a0` copies 12-byte vectors from 92-byte
particle records, honors each destination stride and caps each channel at the
available stream count. The implicit-mesh helper at `0x1008aa90` copies leading
vectors for record type 4 and independently copies surface vertices and
normals.

The registered `NxPhysicsFluidEmitterAbiTests` differential seeds two fluids
and compares the ordered bounds, mesh, particle, and emitter callback tuples;
the emitter records include the system, fluid, and emitter handles. It also
compares six bounds words per fluid, all particle channels, mesh counts and
three distinct mesh output streams, and each emitter's 12 pose words. The
particle case uses two distinct 92-byte records, capacities 3 and 1 against
two available particles, and separate nontrivial output strides for all
three channels. One fluid has an implicit mesh record and the second has a
null mesh record, proving that the optional callback is skipped while the
other post-step work still runs. The restored control reports:

```text
fluid post-step traversal fluids=2 oracle_callbacks=7 candidate_callbacks=7 particle_counts=3/3 particle_strides=distinct emitter_tuples=2 optional_mesh_null=1 mismatches=0
```

Two row-specific mutations were detected by that probe:

- `phys_fn_000619`: replaced the candidate per-fluid dispatch in the Scene
  manager loop with a no-op. The candidate emitted zero post-step callbacks
  versus seven from the oracle and reported `mismatches=72`.
- `phys_fn_003475`: added an immediate return to the candidate per-fluid
  update. The candidate emitted zero post-step callbacks versus seven from the
  oracle and reported `mismatches=72`. A second mutation that removed only
  the implicit-mesh cache copy was also detected with `mismatches=4`.

Both restored controls returned zero mismatches. Build and probe logs are in
`evidence/raw/phase7-fluid-post-step-000619-003475/`.

Fresh post-merge verification (2026-10-11): Phase 5 passes at 2,633/2,633 registered assertions and Phase 7 passes at 1,660/1,660. The full reconstruction-tool suite passes 810/810 tests; inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained bytes. The refreshed full gate outputs are retained as `phase5-gate.log` and `phase7-gate.log` in the adjacent raw-evidence directory.
