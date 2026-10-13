# Phase 7 closure: contact-stream visualization (`phys_fn_000869`)

`phys_fn_000869` at RVA `0x0001d2d0` walks the actor pair's contact stream and renders the configured contact vector and contact-point cross. It decodes pair headers, normal blocks, point records, and optional payloads selected by flags 1, 2, and 4. Its recovered implementation is `NxActorPair::row000869` in `Physics/src/SceneVisualize.cpp`.

The registered `NxPhysicsSceneVisualizeTests` target creates a static plane and dynamic sphere through the public SDK, simulates and fetches 60 steps, clears unrelated visualization parameters, and renders only contact points. Oracle and restored candidate each render three contact lines and match exactly (`stdout_delta=0`, both exits zero, exact stderr).

A row-specific mutation inserts an immediate return at the start of `row000869`. The candidate renders zero contact lines, prints the fixture's expected failure, and exits 1 while the oracle exits 0 (`stdout_delta=9`). The registered paired differential rejects the mutant. Restoring the row returns the control to three contact lines with exact output and stderr.

Mutant worktree candidate DLL SHA-256: `f89956eb04c0e5626ce7591aa39c2e37074d57dd0403d96d84b7de78f31c2ccf`. Restored worktree candidate DLL SHA-256: `60e9fe12e114d8de18d39480357be01d37d5c61c8fe41c100d7c54ba9002096e`. Pinned oracle DLL SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

The fresh Phase 7 gate on the worktree passes at 1,711/1,711 assertions. After commit `ac035f67`, the same gate was rerun on local `main` and passed at 1,711/1,711. The mainline candidate DLL SHA-256 is `6e64587d3369ee31fa14dd0afcaf9ea7b640f8fbeef708590ad416d752145a3c`. Raw mutant, restored-control, worktree-gate, and post-merge mainline-gate transcripts are retained under `evidence/raw/phase7-contact-visualization-000869-20261012/`.
