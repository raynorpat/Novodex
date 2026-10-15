# Phase 7 row 001639: identity pose block

`nxIceIdentityPoses` (`phys_fn_001639`, oracle RVA `0x000313e0`) lazily initializes the function-static identity block used by convex-mesh pose calculations. It clears the 4x4 and 3x3 blocks, sets the guard, writes the diagonal ones, and returns the shared block.

The `NxPhysicsThirdPartyTests` pose-pair family compares the candidate implementation with the pinned oracle. Returning the uninitialized global block instead of initializing it, then rebuilding the test, produced 7 mismatches among 127,828 compared words and target exit 1. The target is now registered as a Phase 7 oracle differential; the exact pose-pair coverage line is part of the Phase 7 gate. Restoring and rebuilding returned pose-pair mismatches=0 and target exit 0.
