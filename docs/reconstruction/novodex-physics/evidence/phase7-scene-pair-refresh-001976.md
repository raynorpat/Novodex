# Phase 7 row 001976: scene pair refresh

`phys_fn_001976` at oracle RVA `0x0004c290` rebuilds the active Scene's broadphase candidate set. The candidate implementation is `nxSceneRefreshPairs` in `Physics/src/ContactPairManager.cpp` (candidate map symbol `?nxSceneRefreshPairs@@YAXPAVNxSceneInternal@@@Z`, preferred address `0x10018070`). It walks the static and selected dynamic pruning pools, selects the configured broadphase path, deduplicates pairs by shape IDs, and stamps retained pair nodes for the current frame.

The registered Phase 7 `NxPhysicsSimulationTests` differential exercises all three public broadphase selectors, separated and overlapping actors, static/static rejection, dense and bounded pairs, plane contacts, pair ordering, and coherent-pruner separation/rejoin. The restored candidate matches the pinned UE3 DLL exactly: both exits 0, `stdout_delta=0`, exact stderr. The restored candidate DLL SHA-256 was `4c2985b1acbc7fe6e4ebb41c0d93a004c9fde4ab810e03fdf6a6f575510eb655`; the pinned oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

For row-specific falsification, an early return was inserted at the entry to `nxSceneRefreshPairs`, then `NxPhysics.dll` and the registered simulation target were rebuilt. The staged-pair differential rejected the mutation (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4000`, stderr differed). Restoring the source and rebuilding returned the exact control above. Raw baseline and mutation transcripts are in the local ignored build directory at `build/tmp/pair-refresh-restored.log` and `build/tmp/pair-refresh-mutant.log`.

This closes the Phase 7 row through the existing registered simulation differential. No public header changed.
