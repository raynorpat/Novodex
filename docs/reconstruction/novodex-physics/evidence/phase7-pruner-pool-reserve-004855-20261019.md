# Phase 7: Scene pruner pool reserve (`phys_fn_004855`)

`phys_fn_004855` at oracle RVA `0x000b5140` is the Scene pruning-engine adapter
that reserves the static pruner pool and the pool selected by the Scene's
pruning mode. The engine's four pruner pointers begin at `+0x1c`. The helper
receives static and dynamic shape limits plus the selected pruner type; for
each nonzero limit it calls `phys_fn_005479` on the selected pruner's pool at
`pruner+4`. It returns `1` regardless of allocation success.

The called pool operation frees existing world-box and object arrays in that
order, stores the requested capacity in its 16-bit capacity field, then
allocates `24 * capacity` bytes for AABBs and `4 * capacity` bytes for object
pointers. It does not copy old entries. This behavior is reconstructed in
`PruningPool::Reserve` in `Physics/src/opcode/IcePruner.cpp`; the Scene adapter
is `nxSceneEngineReservePoolsCandidate` in
`Physics/src/opcode/IcePruningEngine.cpp`, called from
`nxSceneApplyDescriptorFlags` in `Physics/src/Scene.cpp` with scene limits at
`+0x20` and `+0x24`.

The candidate DLL Release map places the adapter at `0x100b2850` in
`IcePruningEngine.obj`. The row-specific differential invokes the pinned
oracle at `0x000b5140` and the reconstructed helper over independently
constructed oracle/candidate pruners. It checks all four selected types,
zero-capacity no-op behavior, and replacement from capacities `8/16` to
`3/5`. The restored run reports four matching modes and zero mismatches. A
mutation disabling both reserve calls reports four mismatches and exits 1.

The complete collision differential also passes with the row marker present.
The full Phase 7 gate passes at 3,115 coverage assertions (floor 3,114), and
the Phase 5 lifecycle gate passes at 2,970/2,970. Raw row and collision outputs are in
`evidence/raw/phase7-pruner-pool-reserve-004855-20261019/`.
