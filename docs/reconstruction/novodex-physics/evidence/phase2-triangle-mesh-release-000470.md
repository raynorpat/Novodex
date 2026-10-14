# SDK triangle-mesh release: foreign handle (`phys_fn_000470`)

The public release path previously dropped a missing mesh handle. IDA's implementation at RVA `0x0000e130` reports `NXE_INVALID_OPERATION` with the original `PhysicsSDK.cpp:569` diagnostic if the registry is empty or the handle is absent. A staged public-API fixture uses a minimal foreign `NxTriangleMesh` object, first with an empty registry and then with a real registered mesh present. The object is only compared by address by the API path; none of its virtual methods are called.

`PhysicsSDK::releaseTriangleMesh` now reports that exact error when no registered entry matches. Existing behavior remains in place for a registered mesh whose release is refused while referenced, and for successful release.

The first new fixture was red against the pinned oracle: oracle reported `rejected=1 errors=1 code=2 line=569`; the candidate returned without reporting an error (`stdout_delta=27`, candidate exit 1). After the private implementation change, both foreign-handle cases match the oracle. The full Phase 4 gate passed with 529/529 required assertions; its mesh API staged differential reports both zero exits, zero stdout delta, and exact stderr.

A row-specific no-op mutation (`if(false)` around the missing-entry error call) failed the registered test: the oracle reported the error, the mutant did not, and the target exited 1 (`stdout_delta=27`). The restored control returned to zero delta and exact stderr. Raw red, mutation, control, build, and Phase 4 gate logs are in `evidence/raw/phase2-triangle-mesh-release-000470-20261014/`.

The only public-facing files changed are test/gate registrations; frozen Physics public headers are untouched. This packet closes the missing-entry error branch of `phys_fn_000470`; mesh ownership and broader triangle-mesh lifecycle behavior remain open elsewhere in the completion plan.


After registering the row in the Phase 2 ledger, the fresh Phase 2 gate also passes at 7/7 assertions; inventory validation passes at Phase 2 66 closed / 77 deferred.
