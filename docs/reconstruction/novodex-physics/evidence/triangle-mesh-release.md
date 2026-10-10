# Triangle-mesh release lifetime

`phys_fn_002258` (`TriangleMesh::release`, oracle RVA `0x00055810`, Phase 4) checks the mesh's reference count at `+0x74`. IDA pseudocode reports `NXE_INVALID_OPERATION`, `TriangleMesh.cpp:173`, and `TriangleMesh::release: instances of this mesh still exist!` when the count is nonzero. On zero, it releases the optional convex object through its deleting slot, runs the existing mesh-owned cleanup rows, frees the mesh through the Foundation allocator, and returns true. `MeshShape::nxMeshLoadFromDesc` increments this counter at RVA `0x00027e30`; its deleting destructor decrements it at `0x00028e80`.

The implementation adds the guard to the private `TriangleMesh::release` method and routes `PhysicsSDK::releaseTriangleMesh` through it. The SDK removes the mesh from its registry only after release succeeds. The public Physics headers are unchanged.

The regression is in `tests/PhysicsTriangleMeshApiTests.cpp`. It creates a mesh-backed actor, attempts to release the mesh while that shape is live, checks the exact error code, line, file, and message, then continues the simulation and releases the mesh after destroying the actor and scene. Before the implementation, the new live-shape case observed `refused=0 errors=0` on the candidate and failed; the pinned oracle takes the refusal path.

Validation command:

```powershell
cmake --build build --config Release --target NxPhysicsTriangleMeshApiTests NxPhysics
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsTriangleMeshApiTests -RepoRoot <worktree> -BuildRoot <worktree>\build -OracleRoot D:\FlamingEnt__\Unreal_3 -PairsRoot <worktree>\build\pairs
```

The differential passed against the pinned `NxPhysics.dll` (`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`): oracle and candidate both exited 0, `stdout_delta=0`, and `stderr_exact=True`. Both reported `triangle_mesh release_in_use refused=1 errors=1 code=2 line=173`, then the mesh-backed actor settled at `y=0x3f73332a` with zero velocity and fetched results successfully.

This row is reconstructed and dynamically checked, but remains deferred in the Phase 4 closure ledger until a throwaway-source mutation aimed at the row is measured under that ledger's falsification protocol.


The live-reference refusal guard is independently mutation-falsified through the registered Phase 4 staged pair. Bypassing `mReferenceCount` changes the candidate transcript (`stdout_delta=6`, candidate exit 1) while the oracle exits 0; restoring the guard returns an exact differential. See [phase4-row-002258-release-guard-mutation-2026-10-09.md](phase4-row-002258-release-guard-mutation-2026-10-09.md). The successful-release cleanup helpers remain separate rows.
