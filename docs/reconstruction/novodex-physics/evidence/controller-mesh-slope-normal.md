# Controller collision against a shallow mesh slope

The public controller fixture cooks a two-triangle ramp with points
`(0,0,0)`, `(0,0,1)`, `(1,0.5,0)`, and `(1,0.5,1)`. A Y-up box controller
starts at `(-2,0.5,0.5)`, has half-extents `(0.2,0.2,0.2)`, and moves four units
along +X.

The first paired run was RED. The oracle stopped at X bits `3eccccd0` (0.4)
with side flag `0x4`; the candidate's SAT found the same face hit at fraction
0.6, but classified the face normal as vertical and removed only vertical
motion. The controller then continued through the ramp to X=2 with flag
`0x2`. For a horizontal-only Y-up mesh sweep, the candidate now resolves the
hit as a side collision and removes motion along the dominant horizontal axis.
The restored paired transcript is exact (`stdout_delta=0`, exact stderr, both
exit 0; `build/controller-slope-green.log`).

The exact position/flag line is registered in Phase 5, 6, and 7 through the
shared `NxPhysicsSimulationTests` target. Phase 5 passes 2,251/2,251 assertions,
Phase 6 passes 1,058/1,058, and Phase 7 passes 1,374/1,374. The Release Viewer
selection passes 48/48 selections across all 39 scenes (43 passed and five
signature-verified pinned-oracle asset cases skipped). The tooling suite passes
770 tests; inventory validation reports 6,338 functions, 5,138 data objects,
and zero unexplained records. This verifies one shallow two-triangle slope and
one horizontal direction. It does not close `phys_fn_002306`: other up axes,
steepness thresholds, transformed meshes, mixed vertical/horizontal slide,
successful step-up, callbacks, and generalized multi-contact response remain
open. No public Physics header changed.
