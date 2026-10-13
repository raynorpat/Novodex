# Phase 7 closure: spring-patch classification (`phys_fn_000871`)

`phys_fn_000871` at RVA `0x0001d590` returns `(materialA.flags | materialB.flags) & NX_MF_SPRING_CONTACT` from the two material IDs stored in a friction patch. Its result controls both patch retention/re-anchoring in `NxActorPair::row000891` and normal-force initialization in `row000895`. The recovered implementation is `cpmPatchSpring0871` in `Physics/src/ContactPairManager.cpp`.

The registered `NxPhysicsSimulationTests` public contact and tangential-friction scenarios reach the patch classifier. Oracle and restored candidate match exactly (`stdout_delta=0`, both exits zero, exact stderr).

A row-specific mutation changes the helper to return 4 unconditionally, incorrectly treating every ordinary material patch as spring contact. The candidate transcript changes by 116 bytes while both processes exit zero and stderr remains exact; the registered differential rejects the mutant. Restoring the material flag expression returns the control to `stdout_delta=0` with exact stderr.

Mutant worktree candidate DLL SHA-256: `283871b0384a4ac5ea706a5de1d1069870440aaf6f7db0c1cef1f37e45fb32db`. Restored worktree candidate DLL SHA-256: `e25caa24ff4a50ee8a6e503ce5694d8941fcc49aa666663cbd8170b5a5772f4e`. Pinned oracle DLL SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

The fresh Phase 7 gate passes at 1,711/1,711 assertions. Raw mutant, restored-control, and full-gate transcripts are retained under `evidence/raw/phase7-contact-spring-patch-classification-000871-20261012/`.
