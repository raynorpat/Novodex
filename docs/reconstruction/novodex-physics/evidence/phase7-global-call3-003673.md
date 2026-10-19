# Three-argument fluid-context callback thunk (`phys_fn_003673`)

The thunk at RVA `0x8add0` loads the fluid object from `[this+4]`, forms callback context from `[[fluid+0x7c]+0x30]`, reads the fluid handle at `[fluid+0x80]`, and forwards its caller-supplied third argument through callback slot RVA `0x12642c`. It cleans the three callback arguments and returns with `ret 4`. The candidate implementation is `nxGlobalCallVia4Result3Arg003673` in `Physics/src/ObjectModel.cpp`.

The row-specific `NxPhysicsObjectLayoutTests` fixture compares both oracle callback loads, the forwarded third argument, callback return, and x86 stack balance. Before implementation, the fixture failed to link on the missing helper. Restored output reports `cases=1 stack=1/1 mismatches=0`. A candidate `fluid+0x84` handle mutation is detected with `mismatches=1` and candidate exit 1.

Fresh Win32 Release Phase 5, 6, and 7 gates pass at 2,959/2,959, 1,810/1,810, and 3,095/3,095 coverage assertions. The object-layout oracle source digest and coverage line are refreshed, and the row marker is required by the gate. Red, focused, mutation, restored, and full-gate transcripts are SHA-256 pinned in [the raw evidence bundle](raw/phase7-global-call3-003673-20261018/).
