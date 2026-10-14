# Triangle-mesh release reverse-unlock continuation (`phys_fn_000246`)

The release wrapper locks each live Scene in order before touching the mesh registry. With two Scenes, the fixture holds the second Scene's write lock on a worker thread, so `releaseTriangleMesh` acquires the first Scene lock and then refuses the second. The oracle reports the pinned lock error and a separate worker can immediately acquire the first Scene lock afterward (`unwind_first=1`), proving the reverse-unlock continuation ran.

The mutation replaced the reverse-unlock loop body with a counter decrement. The oracle still reports `unwind_first=1`; the candidate leaks its first Scene lock and reports `unwind_first=0`. `NxPhysicsTriangleMeshReleaseLockTests` rejects the candidate (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=5`). Restoring the loop produces an exact staged-pair differential (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`), and both sides report `unwind_first=1`.

Phase 2 now passes with eight independent coverage assertions (floor 8), including a second-Scene marker and the full lock-error/unwind marker. Inventory validation records 69 closed and 74 deferred Phase 2 functions, 6,338 code rows, 5,138 data objects, and zero unexplained rows. The public Physics headers remain unchanged. This closes the compiler-outlined wrapper helper only; internal `PhysicsSDK::releaseTriangleMesh` remains separate.

Raw mutation, build, control and Phase 2 gate transcripts are in `evidence/raw/phase2-triangle-mesh-release-unwind-000246-20261014/`.
