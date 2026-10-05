# Scene spatial overlap queries

This checkpoint reconstructs the public `NxScene::checkOverlapAABB`,
`NxScene::checkOverlapSphere`, `NxScene::overlapAABBShapes`, and
`NxScene::overlapSphereShapes` routes from the pinned Win32 DLL
(`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`). The
public Physics headers are unchanged.

## Oracle contract

Capstone rows `phys_fn_000384` (`NpScene::checkOverlapSphere`, RVA `0x0000d500`)
and `phys_fn_000386` (`NpScene::checkOverlapAABB`, RVA `0x0000d580`) acquire the
scene read lock at `NpScene+0x10`, call the internal scene at `NpScene+0x24`,
release the lock, and return its boolean. Their internal callees are
`phys_fn_000672` (RVA `0x000147d0`) and `phys_fn_000674` (RVA `0x000148d0`). The
adjacent row `phys_fn_000382` at RVA `0x0000d480` is `cullShapes`, which remains
unimplemented; this keeps that attribution separate from the overlap routes.

Both internal routines select pruning pools from `NxShapesType`: static shapes
select pruning type 0, while dynamic shapes select types 1 through 3. The AABB
route reports whether any selected prunable's world AABB intersects the query;
it refreshes stale cached world boxes before testing. Plane shapes are
unbounded broadphase candidates. The sphere route first gathers AABB candidates
and then performs a shape-specific overlap test.

## Candidate implementation and evidence

`NpScene.cpp` now reproduces both read-lock wrappers. `SceneRaycast.cpp` walks
the selected scene pruner pools, refreshes stale bounds, and implements the
AABB query. Its sphere narrow tests cover planes, spheres, oriented boxes, and
capsules; the recorded differential includes a sphere/box broadphase-corner
rejection as well as positive sphere/sphere and sphere/box cases.

The differential `build/scene-overlap-capsule.log` compares the pinned oracle
and candidate with `stdout_delta=0` and exact stderr; the follow-up
`build/scene-overlap-capsule-positive.log` adds a positive capsule case and is
`build/scene-overlap-tangencies.log` adds exact and just-separated box/sphere
boundaries and is also exact. Together, the cases exercise all three shape selectors,
static/dynamic results, misses, the unbounded plane, the oriented-box corner
rejection, positive/negative capsule results, and tangent hit/miss behavior. The
refreshed Phase 7 gate passes at 1,276 coverage assertions
(`build/phase7-scene-overlap-final.log`).
Phase 6 passes at 867 (`build/phase6-after-spatial-query.log`) and Phase 5 passes
at 2,042 (`build/phase5-after-spatial-query.log`).

## AABB shape collection

`NpScene::overlapAABBShapes` (oracle row `phys_fn_000380`, RVA `0x0000d400`)
holds the scene read lock and forwards to internal row `phys_fn_000670`
(RVA `0x000145f0`). The static pruner now uses the same cached OPCODE AABB tree
and collider as oracle slot `phys_fn_005229`; touched candidates are returned
in tree traversal order, with unbounded plane shapes following them. Other
selected pruner pools retain their recovered candidate walk. The collector
supports a bounded user buffer and callback-only batches. Full array output,
one-slot truncation, callback-only reporting, and a 12-object permuted-insert
tree-order case match exactly (`stdout_delta=0`, `stderr_exact=True`;
`build/overlap-callback-contract.log` and
`build/bounded-query-differential.log`). The latest focused differential has
`tree_11` through `tree_00` in oracle order followed by `s_plane`.

## Sphere shape collection

`NpScene::overlapSphereShapes` (oracle row `phys_fn_000378`, RVA `0x0000d390`)
and internal row `phys_fn_000678` (RVA `0x00014990`) now use the same read-lock
and reporting contract with sphere narrow tests for the reconstructed
primitive families. A sphere query returning a box and the unbounded plane
through both an output array and callback matches exactly (`stdout_delta=0`,
`stderr_exact=True`; `build/sphere-overlap-callback.log`). The full Phase 7 gate
passes at 1,282 coverage assertions (`build/phase7-scene-collections.log`).
The Release Viewer sweep also passes 48/48 registered tests across the
available scenes and focused physics cases (43 passed, five pinned-oracle
skips; `build/viewer-scenes-scene-collections.log`).

## Plane culling

`NpScene::cullShapes` (oracle row `phys_fn_000382`, RVA `0x0000d480`) now uses
the recovered scene read-lock wrapper and internal row `phys_fn_000671` (RVA
`0x000146e0`). The collector keeps bounded shapes whose minimum-support point
is in each plane's non-positive half-space and preserves unbounded plane
candidates. The differential covers the included side, excluded side, and
callback-only output (`stdout_delta=0`, `stderr_exact=True`;
`build/cull-callback.log`).
The fresh Phase 7 gate passes all registered targets at 1,285 coverage
assertions (`build/phase7-cull-final.log`). The ten wrapper/internal census
rows now carry reconstruction and differential evidence; they remain
reconstructed rather than closed because row-specific mutation falsifications
have not yet been recorded.

## AABB triangle query

`NpScene::overlapAABBTriangles` (oracle wrapper `phys_fn_000388`, RVA
`0x0000d5c0`, and internal query `phys_fn_000676`, RVA `0x00014930`) now walks
scene mesh shapes, reads the cooked index and vertex buffers, transforms each
triangle by its shape pose, and uses the triangle/AABB separating axes before
appending a hit. A caller-prefilled array remains intact and the returned count
includes it; an empty query leaves an empty array. The public case covers a
two-triangle mesh, a miss, a query inside the triangle's AABB but outside the
triangle itself, a query returning every triangle in both registered meshes,
and a translated mesh actor. The
focused `NxPhysicsSceneRaycastTests` differential is exact
(`stdout_delta=0`, `stderr_exact=True`; `build/overlap-tri-diff.log`). Phase 7
coverage now requires these five query markers on both pairs. The full Phase 7
gate passes at 1,290/1,264 coverage assertions (`build/phase7-overlap-tri.log`),
and the Release Viewer selection passes 48/48, with 43 passed and five
verified pinned-oracle skips (`build/viewer-scenes-overlap-tri.log`).

Triangle ordering and exact triangle-versus-box edge cases still need broader
oracle coverage; the current candidate follows reverse cooked-index order for
the reconstructed mesh walk.

## Mesh sphere overlap

`checkOverlapSphere` now includes triangle-mesh shapes by measuring the sphere
against each mesh shape's transformed world bounds. The oracle reports a hit
for a sphere inside the mesh bounds even where it lies outside the triangle
itself, so this differs intentionally from `overlapAABBTriangles`' triangle
level SAT test. The regression covers static/all selection, exclusion under
the dynamic-only selector, a translated mesh, and that bounds-only hit. The
focused staged-pair differential is exact (`stdout_delta=0`, `stderr_exact=True`;
`build/sphere-mesh-overlap.log`).

The follow-up staged-pair differential adds a 90-degree rotated mesh at a
translated pose. It confirms a sphere in the rotated world bounds but away from
the mesh triangles hits, a zero-radius query tangent to the transformed AABB
hits, and a small sphere just outside it misses. Oracle and candidate match
exactly (`stdout_delta=0`, `stderr_exact=True`;
`build/rotated-mesh-query-differential.log`). All three observations are
required by Phase 7; the refreshed gate passes at 1,297 evaluated assertions
against a 1,297 floor (`build/phase7-rotated-mesh-overlap-final.log`).

The broader pruning-engine query slots, row-specific mutation falsification,
and additional mesh transform/near-boundary cases remain open. This checkpoint
does not close the scene spatial-query subsystem or the full DLL.
