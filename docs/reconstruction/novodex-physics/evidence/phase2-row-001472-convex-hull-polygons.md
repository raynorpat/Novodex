# Convex hull polygon generation (`phys_fn_001472`)

The registered `NxPhysicsConvexMeshTests` differential exercises precomputed and computed tetrahedron hulls and records polygon counts, triangle data, vertices, and saved output. The clean oracle/candidate run has `stdout_delta=0` and exact stderr.

For the mutation control, the candidate hull polygon count store at `hull+0x24` was changed to zero and `NxPhysics.dll` rebuilt. The precomputed tetrahedron marker changed from `hull_polygons=4/4/0/00000000` to `hull_polygons=0/4/0/00000000`; **stdout_delta=28** and the registered differential rejected the mutant. Restoring the count store returned an exact differential.

The target belongs to the passing Phase 4 gate, so this shared Phase 2 row is discharged by Phase 4. Raw baseline, mutant, and restored transcripts and hashes are in `evidence/raw/phase4-convex-hull-polygons-row-001472-20261019/manifest.json`.
