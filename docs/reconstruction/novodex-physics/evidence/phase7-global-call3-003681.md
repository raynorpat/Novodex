# Three-argument fluid-context callback thunk (`phys_fn_003681`)

IDA confirms the thunk at RVA `0x8ae70` reads the fluid through `[this+4]`, obtains callback context from `[[fluid+0x7c]+0x30]`, reads the handle at `[fluid+0x80]`, and forwards the caller argument to callback slot RVA `0x1264b8`. It cleans the 12 callback bytes and returns with `ret 4`. The private candidate helper is `nxGlobalCallVia4Result3Arg003681` in `Physics/src/ObjectModel.cpp`; public headers were not changed.

The dedicated object-layout fixture checks callback context, handle, forwarded argument, return value, and x86 stack balance. It fails to link before the helper exists. The restored differential reports `cases=1 stack=1/1 mismatches=0`; a `fluid+0x84` handle mutation is rejected with one mismatch and candidate exit 1.

Fresh Phase 5, 6, and 7 gates pass at 2,962/2,962, 1,813/1,813, and 3,098/3,098 coverage assertions. Phase 7 now records 517 closed / 43 deferred rows. The raw compiler, mutation, restored, and gate transcripts are SHA-256 pinned in [the raw evidence bundle](raw/phase7-global-call3-003681-20261018/). Full-DLL reconstruction remains open.
