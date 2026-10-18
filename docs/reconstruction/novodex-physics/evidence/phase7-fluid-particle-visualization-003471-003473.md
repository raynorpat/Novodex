# Fluid particle visualization rows 003471 and 003473

The two rows cover the per-particle renderer in `Physics/src/SceneVisualize.cpp`. The existing licensed two-fluid fixture drives position crosses, kernel-radius circles and velocity arrows, and compares the pinned oracle against the candidate through the public visualization path. The coverage line names rows `003471.003473.003489` and checks both particle rendering and emitter traversal.

The clean control matches at 39 red, 77 green and 255 blue lines with geometry fingerprints `bca20d6f/997ff147/481e4db8`. A candidate mutation that disables the position-cross branch is detected with `stdout_delta=2` and candidate exit 1. A separate mutation that disables the velocity-arrow branch changes the red geometry fingerprint, produces `stdout_delta=85`, and exits nonzero. The restored differential is exact. Raw runs are recorded in `evidence/raw/phase7-fluid-particle-visualization-003471-003473-20261017/`.

Fresh registered Phase 5 and Phase 7 gates pass at 2,886/2,886 and 3,025/3,025 coverage assertions. The tooling suite is being rerun against this final ledger and target registry.
