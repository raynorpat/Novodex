# Two-argument fluid-context callback thunk (`phys_fn_003669`)

The 27-byte thunk at RVA `0x8ad80` reads the fluid object from `[this+4]`, forms the callback context from `[[fluid+0x7c]+0x30]`, forwards the handle at `[fluid+0x80]`, invokes the global callback, and returns its integer result. The candidate implementation is `nxGlobalCallVia4Result003669` in `Physics/src/ObjectModel.cpp`.

`NxPhysicsObjectLayoutTests` directly compares oracle and candidate callback context, forwarded handle, return value, and x86 stack balance. The restored row reports `cases=1 stack=1/1 mismatches=0`. Changing the candidate handle read from `fluid+0x80` to `fluid+0x84` is detected with `mismatches=1` and candidate exit 1.

Fresh Win32 Release Phase 5, 6, and 7 gates pass at 2,957/2,957, 1,808/1,808, and 3,093/3,093 coverage assertions. The marker is registered to all three phase gates; the object-layout oracle fixture source hash and coverage baseline were refreshed. The five compressed focused, mutation, and full-gate transcripts are pinned in [the raw evidence bundle](raw/phase7-global-call2-003669-20261018/).
