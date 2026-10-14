# Phase 7 continuation: SDK cache release order (`phys_fn_000492`)

The oracle releases the process-wide actor-group-pair cache after Scene and triangle-mesh teardown. The candidate previously reset it before the Scene loop. The teardown fixture now initializes that cache through `setActorGroupPairFlags`, records its three allocator blocks, and checks their free sequence against the SDK-owned mesh.

The old order is rejected (`actor_group_pair_cache_after_mesh=0`, candidate exit 1, `stdout_delta=2`). Moving the cache reset after the OPCODE pool and BoxPruning sorter cleanup restores the expected ordering; the focused staged differential is exact (`stdout_delta=0`, exact stderr), and both DLLs finish with zero tracked blocks. The full Phase 7 gate passes at 1,980/1,980 assertions.

The SDK destructor and scalar deleting thunk remain open pending complete row-level accounting. No public Physics headers changed. Compressed raw transcripts are in `evidence/raw/phase7-sdk-cache-order-000492-20261014/`.
