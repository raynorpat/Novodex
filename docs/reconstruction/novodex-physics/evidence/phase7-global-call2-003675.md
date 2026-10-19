# Two-argument fluid-context callback thunk (`phys_fn_003675`)

The thunk at RVA `0x8ae00` reads the fluid object from `[this+4]`, callback context from `[[fluid+0x7c]+0x30]`, and the fluid handle from `[fluid+0x80]`. It calls callback slot RVA `0x126444` with those two arguments and returns the callback's integer result. The candidate implementation is `nxGlobalCallVia4Result003675` in `Physics/src/ObjectModel.cpp`.

The row-specific `NxPhysicsObjectLayoutTests` fixture compares both callback arguments, return value, and x86 stack balance against the oracle. The test first failed to link on the absent helper. The restored differential reports `cases=1 stack=1/1 mismatches=0`. A candidate mutation that reads `fluid+0x84` is rejected (`mismatches=1`, candidate exit 1).

Fresh Win32 Release Phase 5, 6, and 7 gates pass at 2,960/2,960, 1,811/1,811, and 3,096/3,096 coverage assertions. The object-layout oracle source digest and coverage baseline are refreshed, and Phase 7 records 515 closed / 45 deferred rows. Public headers are unchanged. Red, focused, mutation, restored, and full-gate transcripts are SHA-256 pinned in [the raw evidence bundle](raw/phase7-global-call2-003675-20261018/).
