# Scene destructor resets retained body records (`phys_fn_000604`)

IDA's pinned-oracle listing at RVA `0x000110b0` is a 57-byte walk over
`Scene+[0x56c,0x570)`. It calls `phys_fn_000760` once for each remaining body
record, then continues into the joint-list teardown at RVA `0x000110f0`. The
enclosing `phys_fn_000663` calls this walk after actor teardown, effector and
cached-controller cleanup, and before the joint lists.

`nxSceneResetBodyRecords` now reproduces the bounded pointer walk and dispatches
the existing `Row000760Fixture::row000760` implementation. The helper is called
from `nxSceneDelete` at the oracle-confirmed point. `NxPhysicsInternalTests`
builds a real `NxSceneInternal` and seeds three synthetic body records: two
exercise the exact wake-floor word `0x3ecccccc`, and one verifies that bit `0x100`
suppresses the wake-counter write while the island-root state is still reset.

The initial test link failed because the helper did not exist. After
implementation, the static proof passes all 88 internal checks. An immediate-
return mutation leaves the wake words at their seeded values and is rejected by
the two wake-counter assertions (mutant exit 1); restoring the helper passes.
The Scene destructor's other ownership paths remain open under `phys_fn_000663`.
