# Phase 6 row 003997: SceneDumpSetting::store

`phys_fn_003997` is the 1,757-byte function at RVA `0x0008ffa0`, implemented in `Physics/src/core/SceneDump.cpp`. The existing registered `NxPhysicsCoreDumpTests` staged-pair target serializes core-dump settings and provides row-specific dynamic coverage.

For the falsification run, an unconditional `return false;` was inserted at the start of `SceneDumpSetting::store`, then the candidate DLL and test target were rebuilt. Oracle and candidate both exited 0 and stderr was identical, while the transcript differed by 3,776 bytes. The mutant source hash was `aa954d3506ae2bf9096cc2c37adef45ee192c1f1b73560bdf1c2d609f08f53a1`.

The original source was restored byte-for-byte (`76b271743d0b5b6dc29a682f0e804528a48fd6774ada92cc98ea3efec651ea19`). Rebuilt candidate and oracle both exited 0, stderr was identical, and `stdout_delta=0`. The oracle DLL hash is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Compressed baseline, mutant, and restored transcripts plus source hashes are in `evidence/raw/phase6-scene-dump-setting-003997-20261014/manifest.json`.

This closes one Phase 6 reconstructed row through a detected staged-pair mutation.
