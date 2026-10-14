# Phase 7 continuation: SDK cache release (`phys_fn_000492`)

After `PhysicsSDK` destroys its remaining registered Scenes, teardown now releases the three cache families reached by the pinned SDK destructor: its process-wide OPCODE pool (`phys_fn_004834`), the three BoxPruning radix sorters (`phys_fn_004828`), and the per-Scene shape-pair hash storage (`phys_fn_004149`). Scene teardown also frees both embedded trigger-pair vectors.

The populated teardown fixture checks all three shape-pair hash allocations, both trigger-pair vectors, the cached radix buffers, and the allocator's final outstanding-block count. Four independent mutations are detected: skipping hash release leaves the recorded arrays live (`stdout_delta=13`); skipping trigger-vector release fails (`stdout_delta=10`); skipping radix-cache cleanup leaves two 12-byte arrays (`stdout_delta=4`); and skipping the OPCODE pool release leaves five tracked blocks (`stdout_delta=2`). After restoring the source, the focused staged differential is exact (`stdout_delta=0`, exact stderr), and both oracle and candidate report zero allocator blocks after SDK release.

The fresh Phase 7 gate passes at 1,980/1,980 assertions with the pinned oracle pair. The first run accidentally used a build-local stale oracle pair and crashed in `NxPhysicsSimulationTests`; it is retained only as a setup diagnostic and is not counted as validation. The passing rerun explicitly used `D:\FlamingEnt__\novodex-analysis\pairs`.

This completes the cache-release portion of the SDK destructor reconstruction but does not close `phys_fn_000492` or its scalar deleting thunk `phys_fn_000494`; remaining destructor behavior and row-specific closure evidence are still required. No public Physics headers changed.

Compressed raw transcripts: `evidence/raw/phase7-sdk-cache-release-000492-20261014/`.


## Scene and mesh destruction order

A live triangle-mesh actor was added to an SDK-owned Scene. The old mesh-first order failed the differential (`sdk_scene_before_mesh=0`, `stdout_delta=2`); oracle order is Scene first, mesh second. The restored control matches exactly, and the fresh Phase 7 gate still passes at 1,980/1,980. See [the ordering evidence](phase7-sdk-scene-before-mesh-000492.md).
