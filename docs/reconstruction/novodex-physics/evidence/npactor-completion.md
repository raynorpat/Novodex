# NpActor.cpp completion

Measurement note for the NpActor.cpp completion plan
(`docs/superpowers/plans/2026-09-28-npactor-completion.md`), which closes the public actor API
unit: 87 code rows (evidenced span 0x2610-0xb100), 53 of them (30,673 B) still `discovered`
after the Phase 5 packets implemented and tested most of them against the oracle without
promoting them. The contract is `units/npactor-contract.md`; the execution evidence is
`evidence/npactor-trace.txt`. Each task appends one row to the timing table below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T08:00:00 (approx.; the configure log is stamped 08:01:22) | 2026-09-28T08:23:50 | 0 | 0 | Audit, contract, trace; no source change, no inventory change. Actor table phys_data_000679 (0x10104530) read from the image and mapped to `NxActor.h` order (setStatic is commented out; slots 63/64 are the inline getPointVelocity pair in the gap; word 87 is 000116, the member table's thunk). All 87 rows reviewed against their Capstone listings in seven passes. The 53 discovered rows: 37 implemented, 14 partial, 2 missing (000122 setDynamic, 000164 updateMassFromShapes, which also needs the unreconstructed 000008); 1 faithful (000128), 52 defect (4 H1-only, 13 E1-only, 4 S1, 29 substantive, 2 missing). Cross-cutting: G1 lock-failure report absent everywhere; E1 precondition reports absent; H1 the shared dirty-mark helper drops marks on an unallocated list or body id >= 256; NpActor.cpp is compiled SSE (not on the /arch:IA32 list). The inventory's "partial" 000196-000202 reduce to G1/S1/H1. 000216 is 000214's tail. cdb trace of the 12 Phase 5 actor targets (85 breakpoints, candidate sha256 fb64a931...): 50 of the 53 hit; 000122, 000164 (no case calls them) and 000216 (no function of its own) not. Gates 2, 3, 4, 6, 7 pass; Phase 5 red only on its CANDIDATE-MISSING family=vtables marker, 12/12 actor staged pairs stdout_delta=0. |
