# Body-base destructor row 000799 (`phys_fn_000799`)

The existing body-creation differential releases a record created kinematic and reports the allocator traffic. The oracle and restored candidate both perform six frees in that path. An empty-body mutation of `DynamicBodyBase::destruct` performs five; the staged differential rejects it with both processes exiting zero and `stdout_delta=8`. Restoring `Physics/src/BodyCreation.cpp` byte-for-byte returns an exact control (`stdout_delta=0`, `stderr_exact=True`).

`NxPhysicsBodyCreationTests` is now registered in Phase 7, so this release path is required there. The full Phase 7 gate evaluates and passes 1,975/1,975 coverage assertions. Its pre-floor and raised-floor transcripts, the baseline/mutant/restored differentials, source hashes, and isolated mutation patch are in `evidence/raw/phase7-body-base-dtor-000799-20261014/`. No public headers or production sources changed.
