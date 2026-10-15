# Phase 6 row 004810: SDK default allocator free

`phys_fn_004810` at oracle RVA `0x000b4060` forwards the supplied block to the linked runtime free. The candidate already had that implementation; the previous heap fixture invoked free but did not observe the release.

`NxPhysicsObjectLayoutTests` now tests the effect directly. On the pinned Win32 CRT, it allocates a 128-byte block, releases it through each DLL’s default allocator vtable, and immediately allocates the same size again. Oracle and restored candidate each reuse the just-freed address. The registered line is `allocator free row=phys_fn_004810 oracle_reuse=1 candidate_reuse=1 mismatches=0`.

The no-op mutation was built in an isolated `git archive` of `7ef4eb25` with the focused fixture, gate registry, and refreshed oracle harness hash overlaid. The mutated DLL hash is `7392a91f06cba2b9eba497e6eaf1ec2d06e218afd8416eca3e834e482b353f3f`. The target reports `oracle_reuse=1 candidate_reuse=0 mismatches=1` and exits 1. The restored focused differential passes exactly.

Fresh Phase 6 and Phase 7 gates pass at 1,686/1,686 and 2,902/2,902 registered assertions. The oracle-only layout digest remains `774d629d`; its fixture-source hash was refreshed because the harness changed. Raw mutation, restored-control, and gate logs with hashes are in [the evidence manifest](raw/phase6-default-free-004810-20261015/manifest.json).

No product source or public Physics header changed. This closes only `phys_fn_004810`; full DLL reconstruction remains open.
