# Phase 6 mutation evidence: `phys_fn_004143` — add joint limit plane

`phys_fn_004143` (`Joint::addLimitPlane`, RVA `0x0009a0d0`) transforms and normalizes a limit-plane normal, computes its signed distance, tests it against the joint's limit point, and links accepted planes. `NxPhysicsCoreDumpTests` adds planes to several joint families and serializes their observable normal/distance and in-front results.

The clean staged-pair differential passed with both exits 0, `stdout_delta=0`, and exact stderr. I temporarily added `1.0f` to the computed plane distance. The registered core-dump differential caught the mutation with both exits 0, `stdout_delta=4452`, and exact stderr. The mutant candidate DLL SHA-256 was `2a657062a7b5d93fd59c13ba304c588f8dc9da0d3428a17eee38730c1aa3dba7`.

Restoring `Joint.cpp` and rebuilding with `--clean-first` returned the differential to exact output (`stdout_delta=0`, exact stderr). The restored candidate DLL SHA-256 was `8e5f244603cbfd7d74a8d9fb465c43c0d131d20998bf2ed3700def778310a6ad`; the pinned oracle is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The one-line mutation patch and full mutant/restored transcripts are retained beside this evidence. No public headers changed, and the production source was restored before the final clean build.

The full Phase 6 gate passed after this closure with all registered staged-pair targets exact and 1,554 coverage assertions run (minimum required: 1,267); inventory validation reports 100 closed and 333 deferred function rows.

Reproduction: build `NxPhysics` and `NxPhysicsCoreDumpTests` in Release, run `run_differential.ps1 -Targets NxPhysicsCoreDumpTests`; for the negative control add one to `plane->d` in `Joint::addLimitPlane`, rebuild, and rerun. Restore `Joint.cpp`, rebuild with `--clean-first`, and rerun the differential.
