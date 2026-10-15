# Phase 7 lock-API initializer: phys_fn_002367

`phys_fn_002367` at RVA `0x5b7b0` initializes the lock-API word by calling its callback with `(0, 1, 0, 0)`, storing the returned 32-bit value at object offset zero, and returning the object pointer. The CMake-linked candidate helper is compared with the pinned oracle row for its return, all four callback arguments, and the complete 0x20-byte scratch object.

The row differential previously accumulated failures in a local counter but did not fail `NxPhysicsObjectLayoutTests`. It now reports a dedicated row marker, increments the target's failing candidate count on a mismatch, and is registered as required coverage in Phases 5, 6, and 7. The differential runner now passes the staged oracle hash to this target, verifies the exact loaded oracle identity, and runs it once as a candidate-source harness.

A source mutation changed `fn(0u, 1u, 0u, 0u)` to `fn(0u, 0u, 0u, 0u)`. The oracle process exited 0; the runner rejected the candidate after `newreach row=002367 ... mismatches=1` and `newreach candidate failures=1`. Restoring the source and rebuilding returned the comparison to `mismatches=0` and `layout result=differential-pass`.

The clean Win32 Release Phase 7 gate passed at 2,885/2,885 coverage assertions. Its candidate DLL SHA-256 is `23708b35d0462ea2e936c8d1a123f35a8bb4de657753f9e72277919f4c17e1c3`; the pinned oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. All 816 reconstruction-tool tests passed, and the inventory validator passes after the closure update. The 80-file public-header manifest remains unchanged.

The Phase 7 ledger now records 356 closed and 204 deferred function rows. The row-specific marker and the exact gate transcript are retained in `evidence/raw/phase7-physfn-002367-20261015/`.
