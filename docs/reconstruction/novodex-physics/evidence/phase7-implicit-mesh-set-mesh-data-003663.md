# Phase 7 implicit-mesh setMeshData helper and public wrapper

IDA shows that `phys_fn_003663` at RVA `0x0008ac50` is the private data-record validator/copy helper, not the public API method. The helper validates `NxMeshData`, copies its 0x30 bytes into the embedded record at `this+0x08`, and reports the exact invalid-parameter error from `ImplicitMesh.cpp:106`. The public vtable method is `phys_fn_003874` at RVA `0x0008d230`; it acquires the write lock, checks callback reentry, and dispatches the helper through the internal record pointer at `this+0x14`. The public SDK headers remain unchanged.

`NxPhysicsFluidEmitterAbiTests` now compares the helper against the pinned oracle for three valid and eight invalid descriptors, including exact error code/file/line/message, object bytes, and x86 stack balance. It separately calls the public wrapper with valid and invalid records, a rejected write-lock state, and the shared API reentry flag already active. The probe compares wrapper state, internal-record bytes, lock state/release, exact diagnostics, and stack balance. The candidate wrapper uses the same process-wide reentry flag as the reconstructed Scene APIs. The wrapper probe restores the oracle lock imports around this call because the enclosing fluid-emitter fixture replaces those imports with deterministic stubs.

The restored direct results are:

```text
implicit mesh setMeshData public wrapper valid=1 invalid=1 lock_rejected=1 reentry_rejected=1 stack_checks=4 mismatches=0
implicit mesh setMeshData valid_cases=3 invalid_cases=8 errors=8 stack_checks=11 mismatches=0
fluid emitter aggregate cases=6 mismatches=0
```

The helper copy-destination mutation from `this+0x08` to `this+0x0c` was rejected by the direct helper probe with `mismatches=3`; restoring the correct destination returned zero mismatches. A no-op public-wrapper dispatch was rejected with `public wrapper ... mismatches=2`; the restored control returned zero mismatches. Both process transcripts are retained as `public-wrapper-mutant.log` and `public-wrapper-restored.log` under `evidence/raw/phase7-implicit-mesh-set-mesh-data-003663/`.

The coverage line is registered in Phase 5 and Phase 7. The recorded floors now match the registered lines: the full Phase 5 gate passed at 2,631/2,631 assertions, and the full Phase 7 gate passed at 1,658/1,658 assertions with the lock-refusal and shared-reentry paths included. Their final transcripts are retained as `phase5-final-gate.log` and `phase7-final-gate.log` beside the public-wrapper mutation logs; the expanded focused wrapper transcript is `public-wrapper-guard-cases-restored.log`. Both gates verified the public Physics headers unchanged. Inventory validation reports Phase 7 at 95 closed / 466 remaining, with 6,338 function rows, 5,138 data objects, and zero unexplained bytes.
