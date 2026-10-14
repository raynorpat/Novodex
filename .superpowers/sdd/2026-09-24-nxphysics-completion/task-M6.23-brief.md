# Task M6.23 brief — Dirty island rebuild dispatch (`phys_fn_000720`)

Base: `2f890798`

The public articulated kinematic transitions did not distinguish row 000720 because a later Scene update can rebuild the island. Added a direct DLL differential that resolves the oracle function at pinned RVA `0x16100` and the candidate function through its linker map, seeds a scratch dirty island root, and checks dirty-bit clearing, traversal marking, and island allocation. An immediate-return mutant is rejected (`candidate_exit=1`, `stdout_delta=4000`); restored control is exact.

The full Phase 7 gate passes at 1,798/1,798 assertions. Public headers were not changed. Raw mutation and gate evidence is in `docs/reconstruction/novodex-physics/evidence/raw/phase7-island-dirty-rebuild-000720-20261014/`.
