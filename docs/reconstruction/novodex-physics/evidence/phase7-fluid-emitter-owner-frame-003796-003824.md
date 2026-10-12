# Fluid emitter owner and frame-actor getters

Implemented the two remaining pointer getters on the private `NpFluidEmitter`
wrapper. `phys_fn_003796` returns the public `NxFluid*` at owner-record offset
`+0x54`; `phys_fn_003824` follows the emitter's frame-body pointer and returns
the public actor pointer stored at body offset `+0`. Both operations hold the
wrapper read lock while reading the pointer chain. The public headers remain
unchanged.

`NxPhysicsFluidEmitterAbiTests` now compares each result from the pinned oracle,
the in-process candidate, and the actual rebuilt candidate DLL. The restored
run reports `oracle_owner=1 candidate_owner=1 dll_owner=1`,
`oracle_frame_actor=1 candidate_frame_actor=1 dll_frame_actor=1`, and
`mismatches=0`.

Each getter has a row-specific mutation. Reading the owner pointer from
`record+0x50` instead of `record+0x54` produces `mismatches=2`; returning the
word at `body+0x04` instead of `body+0x00` also produces `mismatches=2`. Both
failures are detected across the in-process and candidate-DLL paths, and the
restored build returns to zero mismatches.

The registered owner/frame coverage line is included in Phase 5 and Phase 7.
The fresh Phase 5 gate passed `2,666/2,666`; the fresh Phase 7 gate passed
`1,693/1,693`. See the [Phase 5 transcript](raw/phase7-fluid-emitter-owner-frame-003796-003824-20261012/phase5-gate.log)
and [Phase 7 transcript](raw/phase7-fluid-emitter-owner-frame-003796-003824-20261012/phase7-gate.log).
The [restored direct probe](raw/phase7-fluid-emitter-owner-frame-003796-003824-20261012/restored-run.log)
and [getFluid mutation run](raw/phase7-fluid-emitter-owner-frame-003796-003824-20261012/mutant-get-fluid-run.log)
and [getFrameActor mutation run](raw/phase7-fluid-emitter-owner-frame-003796-003824-20261012/mutant-get-frame-actor-run.log)
are retained with their build logs.
