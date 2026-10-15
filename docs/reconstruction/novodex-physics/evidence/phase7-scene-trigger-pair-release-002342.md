# Phase 7: Scene trigger-pair release (`phys_fn_002342`)

IDA identifies RVA `0x0005aa90` as the allocator-singleton cleanup for two embedded three-word vectors. For each vector it releases the begin pointer when non-null, then clears begin, end, and capacity unconditionally. The implementation is `nxAllocReleaseClear2342` in `Physics/src/ObjectModel.cpp`, called from the Scene teardown path in `Physics/src/Scene.cpp`.

The registered `NxPhysicsPopulatedSceneTeardownTests` staged-pair fixture creates a trigger overlap, retains both trigger-pair arrays through Scene destruction, and checks that both allocator-owned buffers were freed. The pinned UE3 oracle and restored candidate both report `previous_freed=1 current_freed=1`; the restored differential exits zero with identical stdout and stderr.

For row-specific falsification, only the candidate release for the first embedded vector was skipped (`offset == 0x18`) and the CMake `NxPhysics` target was explicitly rebuilt. The oracle exits zero. The mutant reports `previous_freed=0 current_freed=1`, trips the teardown assertion, and exits 1 (`stdout_delta=15`). Restoring the release, rebuilding, and rerunning returns both exits to zero with `stdout_delta=0` and exact stderr. Raw baseline, mutant, restored transcripts, and the exact mutation are retained in `evidence/raw/phase7-scene-trigger-pair-release-002342-20261015/`.

This closes only the cleanup row; the rest of Scene teardown and the Phase 7 backlog remain open. No public header changed. The existing teardown coverage assertion already exercises this behavior, so the Phase 7 coverage floor is unchanged.
