# `phys_fn_000617` (`NxSceneInternal::getSceneStats`) mutation proof

Base commit: `b3029cf0f974b55230cfa61721b9e589bd51de2e`. Oracle `NxPhysics.dll` SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Restored candidate DLL SHA-256: `7b71ce64bba2733f28d00418c5f70069ceb37c1424f177dab2d1c9e0b799c632`; mutant candidate DLL SHA-256: `c92d0b3160a0642f1a69a2b927c7758b23b409eec6a9b2eab2d39a3d2812d681`. The restored and mutant `Scene.cpp` hashes are `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` and `524df7b9243d87e34494dcb05df0f8506ab5413aa48e27cb7f4d2b6e530c9284`.

The registered Phase 7 `NxPhysicsSimulationTests` staged-pair differential exercises scene stats for empty and populated scenes under all three broadphase selectors. The control output is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr).

A throwaway `git archive` of the base was changed only at `NxSceneInternal::getSceneStats`: the active actor count became `actorEnd - actorBegin + 1`. After rebuilding `NxPhysics` and `NxPhysicsSimulationTests`, the populated-scene line reports 9 actors on the mutant and 8 on the oracle for each selector; the differential catches it (`stdout_delta=8`, both exits 0, exact stderr). The unified patch is `phys_fn_000617.patch` and can be checked/applied with `git apply --check --unidiff-zero`.

After restoring the source, a fresh staged-pair control again matches exactly. The three committed logs are focused excerpts copied verbatim from the official runner output; the complete local runner captures remain under the ignored `build/phase7-row617-*.log` files in the worktree.
