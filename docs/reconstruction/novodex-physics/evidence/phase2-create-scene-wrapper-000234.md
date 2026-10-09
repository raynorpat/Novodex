# Phase 2 row proof: `NpPhysicsSDK::createScene` wrapper (`phys_fn_000234`)

On main revision `3c0c6add2193dd9596e3641017213641b3c0c99c`, the 31-byte row at RVA `0x0000b770` maps to `NpPhysicsSDK::createScene` in `Physics/src/NpPhysicsSDK.cpp`. It forwards the descriptor to the internal SDK, returns null on failure, and otherwise returns the public `NxScene` wrapper at internal scene offset `+0x6cc`.

The registered `NxPhysicsSDKTests` staged-pair fixture creates two scenes, checks their count and enumeration order, releases the first, and verifies the survivor. In a throwaway `git archive`, changing this wrapper to return null was caught by that differential: the oracle exited 0, the candidate exited 1, and `stdout_delta=10` (`build/phase2-mut-000234.log`). The candidate DLL hash was `439958386d4747a35c169f2d5101e06ba4dc986fd7109249ebb9e95e6819eb48`.

After restoring the source byte-for-byte and forcing a rebuild, the Phase 2 gate passed: `NxPhysicsSDKTests` had equal zero exits, `stdout_delta=0`, and exact stderr; all four required Phase 2 coverage assertions passed (`build/phase2-control-000234.log`). The restored candidate DLL hash was `9132ab2f4f69cd3af02415b272ea25b7a109ed8156b9b900b79d8b4891055735`.

This closes the wrapper's local differential proof. Phase 2's transitive `Scene` dependency remains deferred while the remaining scene, joint, and simulation rows are reconstructed. Public headers were not changed.
