# `PhysicsSDK::createTriangleMesh` row proof (`phys_fn_000478`)

The Phase 4 staged-pair `NxPhysicsTriangleMeshApiTests` fixture creates an authored PMap mesh and verifies the returned public handle and descriptor outputs. This exercises the successful SDK path at RVA `0x0000ebe0`, including registration and returning the created mesh.

A row-local mutation changed the final successful `return mesh` to `return 0`. The pinned oracle passed while the candidate failed the authored PMap fixture (`stdout_delta=26`). After restoring and rebuilding the candidate, both sides exited zero with `stdout_delta=0` and exact stderr.

This closes the `PhysicsSDK::createTriangleMesh` wrapper row only. Its internal TriangleMesh dependencies (`phys_fn_002060`, `002065`, `002067`, `002085`, `002140`, and `002239`) remain open independently. Raw build and paired-run transcripts are compressed under `evidence/raw/phase2-create-triangle-mesh-000478-20261015/`.
