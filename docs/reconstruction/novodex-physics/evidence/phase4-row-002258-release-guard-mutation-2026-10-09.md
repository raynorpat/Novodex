# `phys_fn_002258` triangle-mesh release guard mutation

`phys_fn_002258` (`TriangleMesh::release`, RVA `0x00055810`) now has a
row-specific staged-pair mutation proof. The `NxPhysicsTriangleMeshApiTests`
fixture creates a mesh-backed actor, calls `NxPhysicsSDK::releaseTriangleMesh`
while the actor still owns the mesh, and requires the exact refusal line before
continuing simulation. Phase 4 now requires that line in its coverage registry.

The clean Phase 4 gate passed at `269/269` required assertions. Its oracle and
candidate both reported `triangle_mesh release_in_use refused=1 errors=1
code=2 line=173`, and the focused target matched with zero stdout delta and
exact stderr.

For the mutant, only `Physics/src/TriangleMesh.cpp`'s `TriangleMesh::release`
guard was changed from `if(mReferenceCount)` to `if(false && mReferenceCount)`.
The rebuilt candidate DLL SHA-256 was
`9e68d82d958401b03bce2c7a592038b9e728a32141022dcad0ff6b54eeb3c32e`. The
oracle remained the pinned DLL (`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`).
The target observed `refused=0 errors=0 code=0 line=0`; it stopped at the guard
assertion before simulation. Oracle exit was 0, candidate exit was 1, and the
staged differential reported `stdout_delta=6` with the expected failure
diagnostic.

After restoring the guard and rebuilding, the focused staged pair returned to
zero stdout delta, both zero exits, and exact stderr. The restored DLL SHA-256
was `d1ad87350bc1ea757feaf1c676c7cfb5e2353d02550b7f1a30e16c59ce88a571`.
The full transcript logs were written outside the repository under
`D:\Novodex-build-release-row-2258-phase4-control.log`,
`D:\Novodex-build-release-row-2258-phase4-mutant.log`, and
`D:\Novodex-build-release-row-2258-phase4-restored.log`.

This proves the public reference-count refusal branch and its observed error
report through the rebuilt DLL. It does not independently close the cleanup
helpers called by the successful-release branch; those remain separate rows.
