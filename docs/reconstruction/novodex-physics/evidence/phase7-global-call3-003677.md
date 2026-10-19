# Three-argument fluid-context callback thunk (`phys_fn_003677`)

The thunk at RVA `0x8ae20` reads the fluid from `[this+4]`, callback context from `[[fluid+0x7c]+0x30]`, and handle from `[fluid+0x80]`. It forwards its caller-supplied argument through callback slot RVA `0x1263bc`, cleans the three callback arguments, and returns with `ret 4`. The candidate implementation is `nxGlobalCallVia4Result3Arg003677` in `Physics/src/ObjectModel.cpp`.

The row-specific `NxPhysicsObjectLayoutTests` fixture compares callback context, handle, forwarded argument, return value, and x86 stack balance. The fixture failed to link before the helper existed. The restored differential reports `cases=1 stack=1/1 mismatches=0`. A candidate `fluid+0x84` handle mutation is rejected (`mismatches=1`, candidate exit 1).

Fresh Win32 Release Phase 5, 6, and 7 gates pass at 2,961/2,961, 1,812/1,812, and 3,097/3,097 coverage assertions. The object-layout oracle source digest and coverage line are refreshed, and Phase 7 records 516 closed / 44 deferred rows. Public headers are unchanged. Red, focused, mutation, restored, and full-gate transcripts are SHA-256 pinned in [the raw evidence bundle](raw/phase7-global-call3-003677-20261018/).
