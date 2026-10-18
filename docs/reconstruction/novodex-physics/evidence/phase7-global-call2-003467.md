# Phase 7 two-argument callback thunk (`phys_fn_003467`)

The 24-byte thiscall thunk at oracle RVA `0x00084fb0` reads `[this+0x7c]+0x30` and `[this+0x80]`, invokes the global cdecl callback at `0x10126518`, returns the callback's integer result unchanged, and uses `ret`. The candidate implementation is `nxGlobalCallSelf7c803467` in `Physics/src/ObjectModel.cpp`; its CMake Release map entry resolves in `ObjectModel.obj` at `0x1004adb0`. A fresh IDA database confirms the emitted argument loads, indirect call, and return path.

The registered `NxPhysicsObjectLayoutTests` differential compares oracle RVA `0x84fb0` with the candidate source helper using a seeded recorder. It checks both arguments, the callback return (`0x6e6e4670`), and x86 stack balance: `global-call2 row=003467 cases=1 stack=1/1 mismatches=0`. A row-specific mutation changing the second read from `self+0x80` to `self+0x84` yields `mismatches=1` and exits nonzero.

Fresh Phase 5, 6, and 7 gates pass at 2,954/2,954, 1,805/1,805, and 3,090/3,090 registered assertions. Phase 7 closure advances to 509 closed / 51 deferred functions. Public Physics headers are unchanged. Full DLL reconstruction remains active. Transcripts and hashes are in [the raw evidence bundle](raw/phase7-global-call2-003467-20261018/).
