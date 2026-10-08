# SDK scene wrapper differential coverage

`NxPhysicsSDKTests` already exercised public scene creation, indexed lookup,
release of the first scene, and lookup of the surviving scene. The Phase 2 gate
did not require those transcript lines, so removing that lifecycle coverage
could still leave the recorded coverage floor satisfied. The gate now requires
the creation/indexing and release/survivor lines and raises the Phase 2 floor
from one to three assertions. The registry unit test pins the new floor.

Two temporary source mutations confirmed that the assertions are sensitive to
both wrappers. Replacing `NpPhysicsSDK::createScene` with a null return made the
test fail with `both=0`, failed lookups, `stdout_delta=8`, and candidate exit 1.
Replacing `NpPhysicsSDK::releaseScene` with a no-op left two scenes registered,
failed survivor lookup, and made the test fail with `stdout_delta=6` and
candidate exit 1. Both mutations were reverted before the final gate.

The restored Phase 2 gate passes: the three registered differentials match the
oracle, the internal static proof reports 85 checks passing, and all three
required SDK coverage lines are present on both pairs. The scene wrappers
remain deferred in the Phase 2 closure ledger because their internal Scene
dependencies are still tracked in later phases; this evidence strengthens the
public wrapper gate without claiming that those dependencies are complete.
Public Physics headers are unchanged.
