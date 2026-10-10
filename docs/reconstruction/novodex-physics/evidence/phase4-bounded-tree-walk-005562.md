# Phase 4 bounded-tree traversal: phys_fn_005562

`phys_fn_005562` is the 605-byte oracle routine at RVA `0x000f1df0`. IDA shows a recursive bounded dynamic-pruner query: reject disjoint node bounds, filter node objects by query mask and live handle, collect fully contained subtrees, and recurse through child slots 1 through 8 in order. The candidate implementation is `BoundedDynamicPruner::OverlapQuery` and its folded `nxBoundedTreeQueryNode` helper in `Physics/src/opcode/IcePruner.cpp`.

The existing `NxPhysicsSceneRaycastTests` fixture exercises the real scene-query path and records exact traversal order for 12 bounded objects, partial octants, moved objects, expanded query bounds, and a sphere query. The unmutated `bounded_tree all` result is count 12 with order `bounded_05.bounded_10.bounded_00.bounded_02.bounded_04.bounded_03.bounded_01.bounded_09.bounded_08.bounded_06.bounded_07.bounded_11`. The fixture is registered in the Phase 4 gate.

For row-specific falsification, changed only the contained-subtree loop bound from `child <= 8` to `child < 8`, rebuilt `NxPhysics` and `NxPhysicsSceneRaycastTests`, and ran `run_differential.ps1 -Targets NxPhysicsSceneRaycastTests`. The mutation omitted child-slot-8 results from the all, expanded and sphere transcripts. Both processes exited zero, but the registered differential detected `stdout_delta=8`; the pinned oracle remained unchanged.

Restored `child <= 8`, rebuilt, and ran the complete Phase 4 gate. It passed with `coverage_assertions_evaluated=518` against a floor of `269` and `phase_gate=4 status=pass`. This closes the recursive query-walk row. Public-DLL routing and remaining Phase 4 rows stay open.
