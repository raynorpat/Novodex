# Phase 7 bipartite box pruning (`phys_fn_004814`)

IDA identifies the oracle routine at RVA `0x000b4080` as a two-pool sweep-and-prune pass. Its x86 argument order is `(count0, packed AABB array0, count1, packed AABB array1, Pairs*, Axes*)`; it sorts each pool by the selected primary axis, emits A-to-B pairs first, then B-to-A pairs, and checks overlap on the other two axes. The candidate `Opcode::BipartiteBoxPruning` accepts pointer arrays, so the fixture presents equivalent boxes in each function's expected representation.

The registered `NxPhysicsThirdPartyTests` differential drives 260 deterministic cases, including an empty side, equal-minimum boundaries, all six axis orders (`AXES_XYZ`, `AXES_XZY`, `AXES_YXZ`, `AXES_YZX`, `AXES_ZXY`, and `AXES_ZYX`), and generated AABBs. It compares the return value and ordered pair sequence. The restored oracle and candidate both emit 316 pairs with digest `1cb8ecc2` and zero mismatches. The candidate maps to `_BipartiteBoxPruning` at `0x100b2900` in `build/Release/NxPhysics.map` (`NovodexBoxPruning.obj`).

A candidate-only mutation that skips the B-to-A sweep is rejected: the six-order row marker reports 90 mismatches and the target exits 1. Restoring the sweep returns the exact pair transcript and the target exits 0. The raw differential, mutation, restored-control, Phase 4/7, and tooling logs, with checksums, are in `evidence/raw/phase7-bipartite-box-pruning-axis-orders-20261019/`.

Fresh Phase 4 and Phase 7 gates pass at 567/567 and 3,114/3,114 assertions. Both local and engine public-header manifests pass, inventory validation reports zero unexplained rows, and the reconstruction-tool suite passes all 838 tests. No public headers changed. The complete DLL reconstruction remains open.
