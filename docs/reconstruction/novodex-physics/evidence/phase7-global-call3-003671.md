# Three-argument fluid-context callback thunk (`phys_fn_003671`)

The thunk at RVA `0x8ada0` reads the fluid object from `[this+4]`, forms the callback context from `[[fluid+0x7c]+0x30]`, reads the handle at `[fluid+0x80]`, and forwards its caller argument to the global callback. It cleans the three callback arguments and returns with `ret 4`. The candidate implementation is `nxGlobalCallVia4Result3Arg003671` in `Physics/src/ObjectModel.cpp`.

`NxPhysicsObjectLayoutTests` compares both oracle callback loads, the forwarded third argument, callback result, and x86 stack balance. Restored output reports `cases=1 stack=1/1 mismatches=0`. Changing the candidate handle read to `fluid+0x84` is detected with `mismatches=1` and candidate exit 1.

Fresh Win32 Release Phase 5, 6, and 7 gates pass at 2,958/2,958, 1,809/1,809, and 3,094/3,094 coverage assertions. The row marker is pinned in the shared gate registry; the object-layout oracle source digest and coverage baseline are refreshed. The focused, mutation, restored-control, and full-gate transcripts are SHA-256 pinned in [the raw evidence bundle](raw/phase7-global-call3-003671-20261018/).
