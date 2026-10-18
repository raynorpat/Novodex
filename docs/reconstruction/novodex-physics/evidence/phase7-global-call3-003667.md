# Phase 7 fluid-context three-argument callback thunk (`phys_fn_003667`)

The 34-byte thiscall thunk at oracle RVA `0x0008ad50` dereferences `[this+4]` to the fluid, reads context from `[fluid+0x7c]+0x30` and handle from `[fluid+0x80]`, forwards its stack argument to the global cdecl callback at `0x101264fc`, returns the callback result, and uses `ret 4`. The candidate implementation is `nxGlobalCallVia4Result3Arg003667`; its CMake Release map entry in `ObjectModel.obj` is `0x1004adf0`. A fresh IDA load confirms the receiver, stack-argument and context/handle loads, indirect call, and cleanup.

The registered `NxPhysicsObjectLayoutTests` differential verifies the two fluid values, third callback argument, return `0x6b6b5044`, and x86 stack balance: `global-call3 row=003667 cases=1 stack=1/1 mismatches=0`. A row-specific mutation shifting the handle read from `fluid+0x80` to `fluid+0x84` yields `mismatches=1` and fails the differential.

Fresh Phase 5/6/7 gates pass at 2,956/2,956, 1,807/1,807, and 3,092/3,092 registered assertions. Phase 7 closes 511 functions with 49 deferred. Public Physics headers are unchanged; the full DLL reconstruction remains active. Transcripts and hashes are in [the raw evidence bundle](raw/phase7-global-call3-003667-20261018/).
