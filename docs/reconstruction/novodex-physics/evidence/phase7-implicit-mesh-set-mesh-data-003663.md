# Phase 7 implicit mesh data setter (`phys_fn_003663`)

IDA identifies `phys_fn_003663` at RVA `0x0008ac50` as `NxImplicitMesh::setMeshData(NxMeshData&)`. The public SDK header is unchanged. The private implementation validates the 0x30-byte descriptor with the SDK validity predicate, copies valid records to the embedded member at `this+0x08`, and reports the oracle's exact invalid-parameter error for invalid records without changing object state.

`NxPhysicsFluidEmitterAbiTests` calls the pinned oracle at RVA `0x0008ac50` and resolves the candidate implementation from the built DLL map. The differential covers three valid descriptors, eight representative invalid descriptors, exact error code/file/line/message, object bytes, and x86 stack balance. The restored direct result is:

```text
implicit mesh setMeshData valid_cases=3 invalid_cases=8 errors=8 stack_checks=11 mismatches=0
fluid emitter aggregate cases=6 mismatches=0
```

The targeted mutation changed the candidate's copy destination from `this+0x08` to `this+0x0c`. The direct probe rejected it with `valid_cases=0`, `invalid_cases=8`, `errors=8`, `stack_checks=11`, and `mismatches=3`. Restoring `this+0x08` returned zero mismatches. Raw mutant and restored logs are retained in `evidence/raw/phase7-implicit-mesh-set-mesh-data-003663/`.

The direct target is registered in both Phase 5 and Phase 7 coverage. Fresh Phase 5 passed with `phase_gate=5 status=pass` and all 2,630/2,630 registered coverage assertions evaluated. Fresh Phase 7 passed with `phase_gate=7 status=pass` and all 1,657/1,657 registered assertions evaluated. Exact gate logs are retained under `evidence/raw/phase7-implicit-mesh-set-mesh-data-003663/`. The full reconstruction tooling suite passed all 810 tests; `validate_inventory.py` reports 6,338 functions, 5,138 data objects, zero unexplained bytes, and Phase 7 at 94 closed / 467 remaining. The completion backlog was regenerated and the immutable public Physics header check passed within both phase gates.
