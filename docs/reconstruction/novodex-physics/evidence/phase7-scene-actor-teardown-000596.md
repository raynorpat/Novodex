# Phase 7 actor-array teardown: `phys_fn_000596`

`phys_fn_000596` (`0x10f00`, 74 bytes) snapshots the Scene actor range at entry. For each actor it reads the body at actor `+0x14`, dispatches the body destructor through the vtable's `+0x14` slot, and frees the body through `NxFoundationSDKAllocator`. It does not call the public `releaseActor` path, which swap-removes actor pointers and sends the optional fluid-manager notification.

`nxSceneDelete` now mirrors that helper: it snapshots `[Scene+0x55c, Scene+0x560)`, destroys each non-null body with `nxActorDestroy`, and frees each body. The five-actor Scene B fixture is the discriminating case. The registered `NxPhysicsCoreDumpTests` now emits the allocation counts and first 16 free sizes during this release; Scene A also records its release allocation/free totals.

The mutation control rebuilt the prior swap-removing `releaseActor` loop as the candidate DLL (`SHA-256 75771d31224a3f3b757fe3bf4fc2fec8ea0fd19979109553f273cc66150f628f`) and ran the registered `run_differential.ps1` target against the pinned shipped oracle. Both processes exited 0, but the gate rejected the mutant with `stdout_delta=2`. The oracle's Scene B free prefix ended `...,608,28,228,80`; the mutant ended `...,608,28,552,28`, pinpointing the first allocator event changed by the wrong teardown path. The other release totals remained equal, so the mismatch isolates event ordering rather than a missing free.

After restoring the snapshot/destructor loop and rebuilding (`SHA-256 d3f77ee1ad3f639c8d42b4da6e2f2b8fe3a784c6dc9ae889b27c99493596f126`), the registered differential passed: both processes exited 0, `stdout_delta=0`, and `stderr_exact=True`. The relevant transcripts are retained at `D:\Novodex-build-m6-659\core-663-actual-old-loop-mutation.log` and `D:\Novodex-build-m6-659\core-663-restored-direct-loop.log`.

This closes the helper row `phys_fn_000596`; it does not close the larger Scene destructor row `phys_fn_000663`, whose remaining teardown behavior is still tracked separately.
