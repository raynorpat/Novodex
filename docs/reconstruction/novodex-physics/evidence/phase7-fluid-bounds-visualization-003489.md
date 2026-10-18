# Fluid bounds visualization (`phys_fn_003489`)

The candidate implements the manager traversal and licensed cached-bounds branch of the oracle function at RVA `0x00085800`. The licensed simulation fixture creates two fluids, seeds their cached bounds, enables `NX_VISUALIZE_FLUID_BOUNDS`, and compares the red AABB line count.

The pinned oracle and candidate both report `fluids=2 bounds=2 red-lines=24 matched=1`; their differential output is exact. A temporary candidate mutation omitting `addAABB` is rejected: the oracle reports 24 red lines and a match, while the candidate reports zero and fails the row marker. The mutation was reverted and the candidate rebuilt; output again matches exactly.

Fresh Phase 5, 6, and 7 gates pass with 2,881, 1,741, and 3,020 coverage assertions, respectively. Raw red, green, mutation, restored, and gate transcripts are archived under `evidence/raw/phase7-fluid-bounds-visualization-003489-20261017/`.

This bounds-only record predates the particle and emitter traversal continuations. Current branch coverage and its remaining closure gap are recorded in [phase7-fluid-visualization-003489-20261017.md](phase7-fluid-visualization-003489-20261017.md). The row remains open pending an isolated, row-specific emitter traversal mutation and stricter coordinate fidelity.
