# Public `NxFluid` surface-mesh accessors

`phys_fn_003706` and `phys_fn_003708` now occupy the public `releaseSurfaceMesh` and `getSurfaceMesh` vtable slots. The getter checks license bit 2, reports the oracle's line-76 warning and returns null when unavailable, or reads the public mesh pointer from the surface manager at internal-fluid+`0x24` and manager+`0x38` under the Scene read lock. The release method reports the line-66 warning when unavailable, takes the Scene write lock, honors the shared reentry guard, dispatches the surface manager's deleting destructor with flag 1, and clears the slot.

The registered simulation fixture exercises both product-code-465 unavailable-feature warnings and a product-code-804 licensed path. It seeds a surface manager with a test deleting destructor, checks the public pointer, verifies one deleting dispatch, and confirms the internal slot is cleared. Independent no-op getter and no-op release mutants each fail the staged differential (`candidate_exit=1`, `stdout_delta=60`); the restored differential matches exactly.

Phase 5/6/7 coverage floors include this public path. Raw differential and phase-gate logs are under `evidence/raw/phase7-npfluid-surface-mesh-003706-003708/`.
