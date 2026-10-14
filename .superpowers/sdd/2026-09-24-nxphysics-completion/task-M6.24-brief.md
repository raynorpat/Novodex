# Task M6.24 brief — Sleep-group island pair linking (`phys_fn_000724`)

Base: `958d847c`

The Phase 7 simulation target rejects an immediate-return mutation of `Row000724Fixture::row000724` (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=6606`, exact stderr). Restoring the row and rebuilding produces an exact control. The mutation changes existing triangle-mesh contact and broadphase/stack simulation outputs, so no synthetic fixture was needed.

The latest full Phase 7 gate on the same source/test tree passes at 1,798/1,798. No production source or public headers changed. Evidence and raw transcripts are recorded under `docs/reconstruction/novodex-physics/evidence/phase7-island-pair-link-000724.md` and its raw folder.

The packet is fast-forwarded to main at `a61e6c5d`; the post-merge Phase 7 differential set passes for all registered targets. Its compressed transcript and SHA-256 are included in the raw evidence folder.
