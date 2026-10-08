# Phase 5 BOX slot-3 debug-render dispatcher — phys_fn_000945

The registered `NxPhysicsObjectLayoutTests` oracle contract exercises 64 combinations across the BOX enable bit, all eight low flag values, and four live visualization-parameter values. It checks descriptor contents, color, renderer dispatch, and ordering. The restored candidate passes all 64 contract cases.

Mutation: reversed the live `NX_VISUALIZE_COLLISION_SHAPES` comparison in `BoxShape::nxDebugRenderDispatch` from `== 0.0f` to `!= 0.0f`. The first affected case failed with `FAIL slot3 candidate enabled=1 low=0 guard=0` and `slot3 mismatches=1` (exit 1). Restoring the guard and rebuilding reports `slot3 candidate masks=64 agree`, `slot3 order case agree`, and `layout candidate mismatches=0` (exit 0).

This closes the BOX slot-3 dispatcher behavior measured by the registered fixture; the nested debug-render callee remains separately scoped.
