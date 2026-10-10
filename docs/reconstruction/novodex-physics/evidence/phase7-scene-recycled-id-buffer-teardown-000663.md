# Scene recycled-ID buffer teardown (`phys_fn_000663`)

The populated-scene fixture now releases a dynamic actor before releasing its
Scene. Actor release leaves three one-entry recycled-ID arrays allocated:
shape IDs at Scene `+0x6e8`, actor IDs at `+0x6d4`, and dynamic-record IDs at
`+0x6fc`. The fixture retains each allocation pointer across Scene deletion and
checks the tracking allocator afterward. The pinned oracle and rebuilt
candidate both report 8-byte allocations and all three freed; the focused
staged-pair run exits zero on both sides with `stdout_delta=0` and exact stderr.

Each destructor free was omitted independently from the candidate source and
the DLL plus fixture were rebuilt. The test caught all three mutations:
omitting `+0x6fc` reported `body_freed=0`, omitting `+0x6e8` reported
`shape_freed=0`, and omitting `+0x6d4` reported `actor_freed=0`. Each mutant
exited 1 against the oracle's zero exit. The original source was restored and
rebuilt afterward.

The candidate already had these three releases in `nxSceneDelete`; this slice
adds direct ownership coverage and registers its marker in Phases 3 and 7. The
full Phase 3 gate passes 556/556 and the full Phase 7 gate passes 1,645/1,645.
All 810 reconstruction tooling tests pass. No public Physics headers changed.
This closes only these three allocations;
`phys_fn_000663` and the full DLL remain open.
