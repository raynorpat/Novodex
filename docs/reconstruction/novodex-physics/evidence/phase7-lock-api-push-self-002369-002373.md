# Phase 7 lock-API push-self rows 002369 and 002373

`phys_fn_002369` (RVA `0x5b7d0`) and `phys_fn_002373` (RVA `0x5b7f0`) each load the first 32-bit word from `self` and invoke their corresponding lock-API global callback. `Physics/src/ObjectModel.cpp::nxLockApiPushSelf` is the candidate-source implementation used by the two registered source-linked comparisons in `NxPhysicsObjectLayoutTests`; each comparison verifies the callback receives the exact sentinel word.

A temporary mutation changed `fn(v)` to `fn(v ^ 1u)` in `nxLockApiPushSelf`. Both registered row markers changed to `mismatches=1`, `newreach candidate failures=2`, and the differential runner rejected the harness while the oracle exited normally. I restored `ObjectModel.cpp` byte-for-byte, forced recompilation of the test harness and `NxPhysics.dll`, and reran the positive control: both markers returned to zero mismatches and the differential passed.

The clean Win32 Release Phase 5, Phase 6, and Phase 7 gates passed at 2,771/2,771, 1,671/1,671, and 2,887/2,887 coverage assertions respectively. The Phase 7 closure ledger advances to 358 closed / 202 deferred function rows. Public headers and production behavior are unchanged. Raw compressed transcripts and hashes are in [`raw/phase7-lock-api-push-self-002369-002373-20261015/`](raw/phase7-lock-api-push-self-002369-002373-20261015/).
