# Actor owner-pair cleanup (`phys_fn_000521`)

`phys_fn_000521` at RVA `0x103e0` checks the actor root pointer at body offset `0x10`. If present, it forwards the scene and root shape to `NxSceneRemoveOwnerPairRecords` (`phys_fn_000517`) before the root deleting destructor runs. The private member helper is implemented in `Physics/src/Scene.cpp`; no public headers changed.

The direct `NxPhysicsSimulationTests` fixture seeds one pair-map record and compares oracle and candidate for both a populated root and an absent root. Both report `root_removed=1 no_root_preserved=1`; the restored focused differential has oracle_exit=0, candidate_exit=0, stdout_delta=0, and stderr_exact=True.

A targeted mutation disabled the helper's root-present branch. Oracle remained successful, candidate exited 1, and the registered gate measured stdout_delta=4007. Byte-exact source restoration and rebuild returned an exact differential. The fresh complete Phase 7 gate passed on the merged local-main baseline with 1,980/1,980 coverage assertions; candidate NxPhysics.dll SHA-256 `80a572d1d5506108cd102ea8711596e81981a1e6139aa48873a25a74f4ec48b0`. Transcript: `raw/phase7-actor-owner-pair-cleanup-000521-20261014/phase7-gate.log.gz` (uncompressed SHA-256 `A63F9F771E935CFD0D0E132EC0AF8D8B9D92FEED97E54446C27DEE6FD051F96D`). Focused baseline, mutant, and restored transcripts are in the same folder.
