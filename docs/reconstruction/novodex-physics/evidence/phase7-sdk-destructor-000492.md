# Phase 7 row 000492: PhysicsSDK destructor

`phys_fn_000492` is the 364-byte `PhysicsSDK` destructor at oracle RVA `0x0000fb70`. The reconstruction in `Physics/src/PhysicsSDK.cpp` releases the public wrapper, remaining registered Scenes, SDK-owned triangle meshes, global caches, and the SDK's allocator-backed arrays while the Foundation allocator remains available.

The populated teardown fixture exercises these ownership paths and their ordering. It checks the Scene wrapper and mesh allocations, Scene-before-mesh release, actor-group pair-cache order, shape-pair and trigger storage, radix-sorter caches, and zero outstanding SDK allocations. The pinned oracle and restored candidate match exactly.

For the row-specific negative control, the CMake candidate's destructor entry `??1PhysicsSDK@@UAE@XZ` at RVA `0x00053f50` was changed to an immediate `ret`. The registered fixture detects the still-registered Scene (`freed=0`), incorrect release ordering, three remaining radix-sort buffers, and `sdk_release outstanding=53`; it exits 1 while the oracle exits 0 (`stdout_delta=15`, non-exact stderr). The mutant DLL SHA-256 was `975afb4fb10d1f987d17c365ff1417aa2688e55c1847c21865cae7722a768b07`; the restored candidate is `089bc262987fa599810c88c45ef3620b49943c415c8b78d7897a6884fb9cbe46`.

The restored focused differential is exact. The fresh full Phase 7 gate passes at `1,989/1,989` coverage assertions, and inventory validation records 335 Phase 7 functions closed / 225 deferred, with 6,338 code rows, 5,138 data objects, and zero unexplained bytes. Raw mutant, restored, and gate transcripts are in `evidence/raw/phase7-sdk-destructor-000492-20261014/`.

No public Physics headers changed. Full-DLL reconstruction remains open.
