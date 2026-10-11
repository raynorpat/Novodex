# M7.1 brief — fluid emitter world-pose recomposition

Plan: `docs/superpowers/plans/2026-09-24-nxphysics-completion.md` (M7 residual tree)
Base: `76d3a9ad` (`Reconstruct fluid emitter actor attachment`)

## Scope
Reconstruct `phys_fn_003605` at RVA `0x88310` as a candidate-DLL helper that publishes an emitter's world pose. The oracle reads a local 3x3+translation pose from the internal emitter at `+0x18`, resolves the optional frame actor at `+0x14`, uses either its matrix frame (`actor+0x20`) or dynamic-body quaternion/translation (`actor+0x08`, body state `[9..12]` / `[6..8]`), composes world pose, then calls `EmitterSetGlobalPose` with the manager's context/fluid/emitter handles.

Directly test the exported oracle RVA and map-resolved candidate helper with no actor, a rotated matrix actor, and a dynamic-body quaternion actor. Compare the 12 pose words, return value, callback argument tuple/order, stack balance, and a non-axis transform. Never change public headers.

## Steps
1. Extend the focused test first and observe the new candidate symbol lookup fail against the baseline DLL.
2. Implement the private helper in `Physics/src/NpFluidEmitter.cpp` and resolve it through the candidate map.
3. Run the focused ABI differential and a mutation that corrupts one world-pose result; require the mutation to fail and restore exact baseline behavior.
4. Run the Phase 7 gate, inventory validation, and immutable public-header validation; record the row evidence and backlog counts.

## Expected
The exact oracle cases pass with zero mismatches. A world-pose translation/rotation mutation is detected by the focused differential. Phase 7 coverage floor includes the new row output.

## Review focus
Check matrix row/column convention, actor frame source, dynamic-body quaternion ordering, x87 rounding/spill behavior, callback arguments, and `phys_fn_003624`'s caller dependency. This packet closes only `phys_fn_003605`; the caller and wider fluid extension lifecycle remain open.
