# Plane/mesh rows 001893, 001895, and 001897

Base: `6a2a861f` (main reconstruction snapshot). Oracle: NxPhysics.dll SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Restored candidate DLL SHA-256: `7b71ce64bba2733f28d00418c5f70069ceb37c1424f177dab2d1c9e0b799c632`. Restored ContactPlaneMesh.cpp SHA-256: `a53bc8ec4cda4eca163aa393e5b0630ce4566309a9d72afe8ce71b2dfd512888`.

The NxPhysicsCollisionTests `--plane-mesh-only` target was built from a throwaway git archive of the base. Its clean control matched the pinned oracle: overlap 8/8, ordered contact comparisons 872/872, 38 contacts, digest `3dc2d3277f3fc523`, zero mismatches. Each row-specific mutation below was rebuilt independently in the archive and then restored. The final archived source hash matches the worktree source hash.

- `phys_fn_001893` (NxOverlapPlaneMesh, RVA `0x00048680`): force the return to false. The fixture detects 8 mismatches (all overlap checks).
- `phys_fn_001895` (NxContactPlaneMesh, RVA `0x00048760`): return before traversing touched faces. The fixture detects 16 mismatches.
- `phys_fn_001897` (transformed-vertex continuation, RVA `0x000488f0`): add 1.0 to transformed world X. The fixture detects 54 mismatches; candidate digest changes to `1cdce4ea696f8aad`.

The restored archive control again matches exactly. The public NxPhysicsTriangleMeshApiTests differential also matches the actual rebuilt DLL (oracle_exit=0, candidate_exit=0, stdout_delta=0, exact stderr; settled Y=`3f73332a`). Its mesh-settle case alone does not cover the overlap entry: the false-return mutant still produces an exact public transcript. That negative result is retained so the public integration case is not overstated; the row-level internal oracle differential supplies the falsification evidence for these unexported entries.

Raw baseline, mutation, and restored outputs are adjacent to this record. The public DLL negative-detection result is `public-fixture-non-detection.log`.
