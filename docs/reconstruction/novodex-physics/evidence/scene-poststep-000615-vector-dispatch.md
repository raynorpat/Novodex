# Post-step reconstruction evidence

`phys_fn_000615` (Scene+0x113c0) and `phys_fn_004165` (0x9ace0) are implemented in commit `d4caaab9`.

The oracle listing identifies `000615` as the scene post-step pass: it invokes `000770(body, scene+0x548)` and `000022(body+0x19c, 1)` for each body, then makes a second pass that calls `004165(body+0x1e0)` only when the optional vector is non-null. The helper at `004165` is an ECX-only thiscall with no stack arguments. It walks the vector bounds at +0x10/+0x14, rereads the range each iteration, and invokes each element's vtable slot +0x10 with the element in ECX.

`NxPhysicsObjectLayoutTests` exercises the helper directly through the oracle member-function pointer and the candidate member-function pointer using the same x86 thiscall ABI. For lengths zero through three, it checks call count, visited element order, and object bytes. The focused control reports `vecloop4165 candidate failures=0 provisional=1` and `layout candidate mismatches=0`; Phase 5 passes at 2,597/2,597 assertions and Phase 6 passes at 1,227/1,227 assertions after this oracle test was registered in both phases.

The `004165` mutation probe inserted an immediate return in the helper in a clean archive. For lengths one, two, and three, the candidate visited zero elements against oracle counts one, two, and three, for three failing cases. The Phase 6 gate failed its required `vecloop4165 candidate failures=0 provisional=1` assertion. The Phase 5 gate now has the same required assertion and also rejects this mutant; before the assertion was added, the gate misleadingly passed while the row printed three failures. The archive was restored and rebuilt; its control passed the helper differential with zero vector failures and zero aggregate layout mismatches.

For `000615`, an immediate return in `NxSceneInternal::row000615` was built in a separate clean archive and run through the registered `NxPhysicsSimulationTests` staged-pair differential. Both processes exited zero with exact stderr, but stdout differed by 7,232 bytes. Restoring `row000615` and rebuilding returned `stdout_delta=0` and exact stderr. The Phase 7 gate passed with 1,418/1,418 coverage assertions.

Recorded build and gate transcripts (generated under `build/phase-step-rows/`): `row004165-abi-green-oracle.log`, `row004165-phase5-final.log`, `row004165-phase6-abs.log`, `row004165-phase6-mutation.log`, `row004165-gateassert-red.log`, `row000615-mutation-diff.log`, and `row000615-restored-diff.log`.

Post-merge mainline verification at `c9080352` passed Phase 5 (2,597/2,597), Phase 6 (1,227/1,227), and Phase 7 (1,418/1,418) in the fresh `build/phase-step-main` tree.
