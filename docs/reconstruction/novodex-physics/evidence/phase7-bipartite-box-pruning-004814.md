# Phase 7 bipartite box pruning (`phys_fn_004814`)

IDA identifies the oracle routine at RVA `0x000b4080` as a two-pool sweep-and-prune pass. Its x86 argument order is `(count0, packed AABB array0, count1, packed AABB array1, Pairs*, Axes*)`; it sorts each pool by the selected primary axis, emits A-to-B pairs first, then B-to-A pairs, and checks overlap on the other two axes. The candidate `Opcode::BipartiteBoxPruning` accepts pointer arrays, so the fixture presents equivalent boxes in each function's expected representation.

The registered `NxPhysicsThirdPartyTests` differential drives 260 deterministic cases, including an empty side, equal-minimum boundaries, three axis orders, and generated AABBs. It compares the return value and ordered pair sequence. The restored oracle and candidate both emit 316 pairs with digest `27496242` and zero mismatches. The candidate maps to `_BipartiteBoxPruning` at `0x100b2900` in `build/Release/NxPhysics.map` (`NovodexBoxPruning.obj`).

A candidate-only mutation that skips the B-to-A sweep is rejected: the oracle emits 1,152 words while the candidate emits 904; the row marker reports 85 mismatches and the target exits 1. Restoring the sweep returns the exact pair transcript and the target exits 0. The raw baseline and mutation logs, with checksums, are in `evidence/raw/phase7-bipartite-box-pruning-004814-20261019/`.

Fresh Phase 4 and Phase 7 gates pass at 567/567 and 3,114/3,114 assertions. Both local and engine public-header manifests pass, inventory validation reports zero unexplained rows, and the reconstruction-tool suite passes all 838 tests. No public headers changed. The complete DLL reconstruction remains open.
