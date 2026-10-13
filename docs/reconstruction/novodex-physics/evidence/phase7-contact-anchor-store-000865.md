# Phase 7 closure: contact-anchor storage (`phys_fn_000865`)

`phys_fn_000865` at RVA `0x0001d0c0` stores up to the selected contact points in a friction patch. For each point it writes body 0 and body 1 local coordinates into the two anchor halves; when a body pointer is null it copies the world point unchanged. The routine stores the byte count at patch `+0x74`, uses that stored count as the loop bound, and clears the re-anchor byte at `+0x75`. The recovered implementation is `NxActorPair::row000865` in `Physics/src/ContactPairManager.cpp`.

The registered `NxPhysicsSimulationTests` target reaches this path through its public SDK multi-step contact scenarios: static plane/dynamic sphere, box/plane, dynamic sphere pair, and tangential-friction contact. Oracle and restored candidate match exactly (`stdout_delta=0`, both exits zero, exact stderr).

A row-specific mutation inserts an immediate return at the start of `row000865`, omitting all anchor/count writes. The candidate transcript changes by 120 bytes while both processes exit zero and stderr remains exact; the registered differential rejects the mutant. Restoring the implementation returns the control to `stdout_delta=0` with exact stderr.

Mutant candidate DLL SHA-256: `cbbf9867d07fe33e0500129169c234a93da0fd76c4af69689601ddb84c5a3dca`. Restored worktree candidate DLL SHA-256: `e36034bce6fa5b16c7d2e4e4ec8af52208a14167a787fc5c80698cbff6f83a53`. Pinned oracle DLL SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

The fresh Phase 7 gate on the worktree passes at 1,711/1,711 assertions. After commit `0ed8187a`, the same gate was rerun on local `main` and passed at 1,711/1,711. The mainline candidate DLL SHA-256 is `d6239d16603fbf309b9860f96b5c2005bc6f7fc3c159ac22f77a509c5f5033a8`. Raw mutant, restored-control, worktree-gate, and post-merge mainline-gate transcripts are retained under `evidence/raw/phase7-contact-anchor-store-000865-20261012/`.
