# Phase 7 pruning-engine teardown: `phys_fn_004850`

`phys_fn_004850` at RVA `0x000b5060` is the Scene pruning-engine cleanup loop. The 43-byte oracle routine scans four pruner pointers at engine offsets `+0x1c`, `+0x20`, `+0x24`, and `+0x28`. For each non-null pointer it calls vtable slot 0 with scalar-deleting flag `1`, then clears that slot. It leaves the dword at `+0x2c` untouched.

The candidate maps this operation to `nxSceneEngineDestroyPruners` in `Physics/src/opcode/IcePruningEngine.cpp`. `nxPruningEngineDestroyPruner` dispatches the virtual scalar deleting destructor. `Pruner::operator delete` routes that destructor through `opcNovodeXFree`, matching the SDK allocator used by the oracle.

The registered `NxPhysicsSceneRaycastTests` fixture calls the oracle RVA and candidate map symbol directly. Its dense case verifies four destructor calls in order, flag `1`, all slots cleared, and an untouched sentinel. Its sparse case verifies only slots 1 and 3 dispatch, in that order, with all four slots cleared. The restored differential passes exactly: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`.

Mutation testing changed the candidate deleting flag from `1` to `0`. The dense case reported `matched=0`; the staged differential rejected the candidate (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2815`). Restoring flag `1` returned the direct differential to an exact match. Phase 5 passed with 2,926 coverage assertions against a floor of 2,924. Phase 4 passed with 562 coverage assertions against a floor of 560, and Phase 7 passed with 3,062 against 3,060. The Phase 5 result was 2,926 against 2,924. All three registered gates exercised the dense and sparse cases on oracle and candidate. Their complete logs are recorded in the raw evidence manifest.

Raw staged-pair logs and hashes are in `evidence/raw/phase7-pruner-destroy-004850-20261018/`.
