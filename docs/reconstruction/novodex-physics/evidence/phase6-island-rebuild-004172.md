# Phase 6 closure: articulation island rebuild

`phys_fn_004172` clears the root body’s island-dirty bit, rebuilds the articulation island through `phys_fn_004169`, reports the oracle warning when no articulation group is available, and stores the rebuilt island at root offset `+0x1e0`. Replacing the function with a no-op changed the registered `NxPhysicsSimulationTests` staged-pair transcript with `stdout_delta=226`; both processes exited 0 and stderr matched exactly. The restored control returned `stdout_delta=0` with exact stderr.

Mutant candidate SHA-256: `c29ed3245f78c7ab2507c7df1ba2bf33f27a2e61dc90a8da67a4f0d50d4ab6c5`. Restored candidate SHA-256: `57f328a441ff5f677e8f0359b6bd1f09e428632c7858e8745c574ca4e899387c`. The source implementation and direct internal probes pre-existed; this closes the census entry with a registered DLL-level simulation mutation.
