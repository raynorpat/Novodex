# Phase 7 friction-patch assignment (`phys_fn_000877`)

`NxFrictionPatch::operator=` at RVA `0x0001dc80` copies the payload at `+0x04..+0x75`, leaves the primary vptr and two trailing bytes untouched, and returns the destination. The registered `NxPhysicsFluidEmitterAbiTests` probe calls the pinned oracle and candidate implementation directly on three patterned scratch source/destination pairs. The restored control reports `cases=3`, matching payloads in all three cases, preserved vptr and tail padding, matching return pointers, balanced x86 stacks, and `mismatches=0`.

A no-op assignment mutation was rebuilt and rejected: all three payload comparisons failed (`mismatches=3`) while vptr, padding, return, and stack checks remained intact. Restoring the implementation returned the direct oracle differential to zero mismatches. The public headers are unchanged.

The focused transcripts and build records are in [raw evidence](raw/phase7-friction-patch-assignment-000877/). The full Phase 7 gate is recorded in the completion-plan continuation.

Fresh Phase 5 validation on 2026-10-13 passes at 2,699/2,699 assertions, including the new direct assignment marker and all five secondary-destructor families. The focused gate transcript is [phase5-gate.log](raw/phase7-friction-patch-assignment-000877/phase5-gate.log).

Fresh Phase 7 validation on 2026-10-13 passes at 1,741/1,741 assertions. The candidate Physics DLL used in both full gates has SHA-256 `dc9b5a15ccaed6e014fca2f30d2ebfc1bb9d31037665e3d044908637ce6ef909`. The focused Phase 7 transcript is [phase7-gate.log](raw/phase7-friction-patch-assignment-000877/phase7-gate.log).
