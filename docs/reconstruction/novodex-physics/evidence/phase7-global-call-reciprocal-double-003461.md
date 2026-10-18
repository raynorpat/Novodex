# Phase 7 reciprocal-double callback thunk (`phys_fn_003461`)

The callback thunk at RVA `0x00084f50` is reconstructed as `nxGlobalCallReciprocalDouble` in `Physics/src/ObjectModel.cpp`. The oracle reads the callback object from `[this+0x7c]`, forwards `[object+0x30]` and `[this+0x80]`, then returns `1.0 / callback_result` as an x87 double. The candidate map resolves the helper in `ObjectModel.obj` at `0x1004ad60`; the generated x86 code preserves a double return and has no stack arguments.

`NxPhysicsObjectLayoutTests` calls both the oracle RVA and candidate helper for six nonzero positive, negative, integer, and fractional callback results. It checks exact result bits, both callback arguments, one callback invocation, and x86 stack balance. Oracle coverage is pinned at digest `3acf0dc7`. Returning the callback value without the reciprocal is caught in all six cases (`mismatches=6`); restored control reports `mismatches=0`.

The candidate marker is registered for Phases 5, 6, and 7. Fresh full gates pass with 2,952/2,952, 1,803/1,803, and 3,088/3,088 coverage assertions. The focused oracle/candidate differential passes, the reconstruction tooling suite passes all 823 tests, and inventory validation reports 6,338 functions, 5,138 data objects, and zero unexplained records. Public Physics headers are unchanged; the added declaration is in the private `Physics/src/include` header.

Raw compressed logs and their SHA-256 manifest are in `raw/phase7-global-call-reciprocal-double-003461-20261018/`. Full DLL reconstruction remains active.
