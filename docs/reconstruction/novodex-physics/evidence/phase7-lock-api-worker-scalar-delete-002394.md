# Phase 7 continuation: lock-API scalar deleting thunk (`phys_fn_002394`)

IDA at oracle VA `0x1005b980` (RVA `0x5b980`) confirms the x86 MSVC scalar deleting thunk: preserve `this`, call `phys_fn_002392`, test the stack delete-flags argument's bit 0, call the CRT global delete thunk only when set, return the original pointer, and `ret 4`. `nxLockApiWorkerScalarDelete2394` implements that ABI privately; no public SDK header changed.

The registered `NxPhysicsObjectLayoutTests` oracle/source fixture directly invokes flags 0 and 1. It checks return identity, record teardown, handle closure, owner state, and the owner allocation's lifetime. A row-local mutant returning `nullptr` instead of `this` reports `mismatches=2` and exits 1; restoring the original-pointer return reports `mismatches=0` and the layout differential passes. An inverted delete condition was also independently caught by candidate heap corruption (`0xC0000374`). The fixture source and all oracle-only proof baseline hashes are refreshed.

Phase 7 advances to 367 closed / 193 deferred. The row marker adds one assertion to each shared phase registration: Phase 5 2,780; Phase 6 1,680; Phase 7 2,896. Raw mutation and restored-control logs are stored alongside this note.
