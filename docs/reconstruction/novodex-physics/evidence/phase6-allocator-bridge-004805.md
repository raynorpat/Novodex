# Phase 6 row 004805: SDK allocator bridge setter

`phys_fn_004805` at oracle RVA `0x000b4020` stores its allocator argument in the SDK allocator bridge global and returns true. The reconstruction already lived in `nxSetSdkAllocatorBridge` in `Physics/src/include/NxSdkAllocatorAccess.inl`; this packet adds direct candidate-DLL coverage without changing production code.

`NxPhysicsObjectLayoutTests` now resolves `nxSetSdkAllocatorBridge` and `gSdkAllocator` from the candidate `NxPhysics.map`, calls the pinned oracle row and candidate setter with the same temporary pointer, compares the return and stored value, then restores both original globals. The registered coverage marker is `allocator bridge row=phys_fn_004805 return=1 stored_pointer=1 restored=1 mismatches=0`.

The row was mutation-falsified in a throwaway archive of `6fe57131` by removing only the candidate store. The registered target emitted `mismatches=1` and failed; the mutant DLL SHA-256 is `f91433c8de8e3867f2b38bc25190f323ec98bfb8289e9db307e572a153358cdc`. The restored target passed with an exact staged transcript (`stdout_delta=0`, exact stderr); the restored candidate DLL SHA-256 is `efb4842679e8edddfa65fb36f37d65f1c0b662e1a36f5a494b2e23b6261f5c2c`.

Fresh Phase 5, 6, and 7 gates pass at 2,785/2,785, 1,685/1,685, and 2,901/2,901 coverage assertions. Phase 6 also exercised the approved date/time assertion: the core-dump target validated weekday, month, day, year, 12-hour time, and AM/PM before normalization, and the staged differential was exact. Raw gate and mutation transcripts, with compressed/source hashes, are in [the evidence directory](raw/phase6-allocator-bridge-004805-20261015/manifest.json).

No public SDK header changed. Full DLL reconstruction remains open.
